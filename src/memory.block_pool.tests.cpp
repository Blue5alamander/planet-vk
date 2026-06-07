#include <planet/log.hpp>
#include <planet/vk/headless.hpp>
#include <planet/vk/memory_block_pool.hpp>

#include <felspar/test.hpp>

#include <thread>
#include <vector>


namespace {


    constexpr std::size_t block_64mib = 64u << 20;
    constexpr std::size_t block_32mib = 32u << 20;


    /// ### Find a memory type index that can back a driver allocation
    std::uint32_t any_memory_type(planet::vk::headless const &vulkan) {
        VkMemoryRequirements requirements{};
        requirements.memoryTypeBits = ~std::uint32_t{};
        return vulkan.instance.find_memory_type(
                requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }


    auto const suite =
            felspar::testsuite("vulkan::device_memory_block_pool", []() {
                planet::log::active = planet::log::level::error;
            });


    /// ### An empty pool allocates from the driver, then recycles
    /**
     * The first `acquire` of a `(memory_type_index, block_size)` with an empty
     * free list must perform exactly one driver allocation. After the block is
     * `release`d, the next `acquire` of the same key must reuse it with no
     * further driver allocation.
     */
    auto const recycles = suite.test("recycles-released-block", [](auto check) {
        planet::vk::headless vulkan;
        auto const mti = any_memory_type(vulkan);

        planet::vk::device_memory_block_pool pool{"recycles"};
        check(pool.driver_blocks_allocated()) == 0u;

        auto block = pool.acquire(vulkan.device, mti, block_64mib);
        check(pool.driver_blocks_allocated()) == 1u;

        pool.release(std::move(block), mti, block_64mib);
        check(pool.driver_blocks_allocated()) == 1u;

        auto reused = pool.acquire(vulkan.device, mti, block_64mib);
        check(pool.driver_blocks_allocated())
                == 1u; // reused, no new driver allocation

        pool.release(std::move(reused), mti, block_64mib);
        pool.clear();
    });


    /// ### Blocks of a different key are never interchanged
    /**
     * A free list is keyed by `(memory_type_index, block_size)`, so releasing a
     * block of one key must not satisfy an `acquire` of a different key -- the
     * second `acquire` must hit the driver.
     */
    auto const keyed = suite.test("keyed-by-type-and-size", [](auto check) {
        planet::vk::headless vulkan;
        auto const mti = any_memory_type(vulkan);

        planet::vk::device_memory_block_pool pool{"keyed"};

        // Differ by block_size: release a 64 MiB block, acquire a 32 MiB one.
        auto big = pool.acquire(vulkan.device, mti, block_64mib);
        check(pool.driver_blocks_allocated()) == 1u;
        pool.release(std::move(big), mti, block_64mib);

        auto small = pool.acquire(vulkan.device, mti, block_32mib);
        check(pool.driver_blocks_allocated())
                == 2u; // different size -> not reused
        pool.release(std::move(small), mti, block_32mib);

        // Differ by memory_type_index when the GPU exposes more than one type.
        // A 64 MiB block released under `mti` is still pooled, so an acquire of
        // the same size under a different type must hit the driver, while a
        // later acquire under `mti` reuses the pooled block.
        auto const &gpump = vulkan.instance.gpu().memory_properties;
        if (gpump.memoryTypeCount > 1u) {
            auto const other = mti == 0u ? 1u : 0u;
            auto const before = pool.driver_blocks_allocated();

            auto b = pool.acquire(vulkan.device, other, block_64mib);
            check(pool.driver_blocks_allocated())
                    == before + 1u; // different type -> not reused
            pool.release(std::move(b), other, block_64mib);

            auto a = pool.acquire(vulkan.device, mti, block_64mib);
            check(pool.driver_blocks_allocated())
                    == before + 1u; // same key -> pooled block reused
            pool.release(std::move(a), mti, block_64mib);
        }

        pool.clear();
    });


    /// ### `clear` frees every held block
    /**
     * Every block returned to the pool is freed by `clear`, so the count of
     * driver deallocations matches the count of driver allocations.
     */
    auto const clears = suite.test("clear-frees-all", [](auto check) {
        planet::vk::headless vulkan;
        auto const mti = any_memory_type(vulkan);

        planet::vk::device_memory_block_pool pool{"clears"};

        // Hold three blocks at once so each `acquire` hits the driver rather
        // than reusing a just-released block.
        auto block0 = pool.acquire(vulkan.device, mti, block_64mib);
        auto block1 = pool.acquire(vulkan.device, mti, block_64mib);
        auto block2 = pool.acquire(vulkan.device, mti, block_64mib);
        check(pool.driver_blocks_allocated()) == 3u;

        pool.release(std::move(block0), mti, block_64mib);
        pool.release(std::move(block1), mti, block_64mib);
        pool.release(std::move(block2), mti, block_64mib);
        check(pool.driver_blocks_freed()) == 0u;

        pool.clear();
        check(pool.driver_blocks_freed()) == 3u;
        check(pool.driver_bytes_in_use()) == 0;
    });


    /// ### Concurrent `acquire`/`release` neither double-frees nor races
    /**
     * Many threads churn blocks of the same key through the pool. The run must
     * be TSan-clean, and once every thread has returned its block, `clear` must
     * free exactly the blocks the pool allocated.
     */
    auto const concurrent =
            suite.test("concurrent-acquire-release", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                planet::vk::device_memory_block_pool pool{"concurrent"};

                constexpr std::size_t thread_count = 4u;
                constexpr std::size_t iterations = 32u;
                std::vector<std::thread> threads;
                for (std::size_t t{}; t < thread_count; ++t) {
                    threads.emplace_back([&]() {
                        for (std::size_t n{}; n < iterations; ++n) {
                            auto block = pool.acquire(
                                    vulkan.device, mti, block_64mib);
                            pool.release(std::move(block), mti, block_64mib);
                        }
                    });
                }
                for (auto &&thread : threads) { thread.join(); }

                // Every block the pool allocated is still held by the pool (all
                // were released back), so `clear` frees exactly that many.
                check(pool.driver_blocks_freed()) == 0u;
                pool.clear();
                check(pool.driver_blocks_freed())
                        == pool.driver_blocks_allocated();
                check(pool.driver_bytes_in_use()) == 0;
            });


}


/// ## Leak sanitiser suppressions
/**
 * The Mesa Vulkan driver allocates one-time global and per-thread state during
 * instance creation (behind `pthread_once`) that it never frees before the
 * process exits. That isn't a leak in our code, but LSan can't tell, so
 * suppress allocations made on the driver's one-time-initialisation paths. Our
 * own RAII wrappers are still leak-checked.
 */
extern "C" char const *__lsan_default_suppressions() {
    return "leak:libvulkan\n"
           "leak:__pthread_once_slow\n";
}
