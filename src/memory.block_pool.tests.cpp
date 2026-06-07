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


    /// ## Whole-block free list


    /// ### A released block is recycled rather than re-allocated
    /**
     * The first `acquire` of a `(memory_type_index, block_size)` on an empty
     * pool performs one driver allocation. Once that block is `release`d, the
     * next `acquire` of the same key reuses it without touching the driver.
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


    /// ### Blocks are kept separate by memory type and size
    /**
     * Free lists are keyed by `(memory_type_index, block_size)`. A released
     * block only satisfies an `acquire` of the identical key; a request that
     * differs in either size or memory type allocates a fresh block from the
     * driver, while the pooled block stays available for its own key.
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


    /// ### Blocks larger than the threshold are never pooled
    /**
     * A block larger than `driver_block_size` is allocated straight from the
     * driver and freed straight back on `release`, never entering a free list:
     * `driver_bytes_in_use` rises on `acquire` and falls again on `release`. A
     * second `acquire` of the same size therefore allocates afresh rather than
     * finding a retained block.
     */
    auto const oversized_bypasses =
            suite.test("oversized-block-bypasses-pool", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                // A small threshold keeps the test's driver allocations tiny.
                constexpr std::size_t threshold = 1u << 20;
                planet::vk::device_memory_block_pool pool{
                        "oversized", threshold};

                constexpr std::size_t big = 2u << 20; // > threshold

                auto block = pool.acquire(vulkan.device, mti, big);
                check(pool.driver_blocks_allocated()) == 1u;
                check(pool.driver_bytes_in_use())
                        == static_cast<std::int64_t>(big);

                pool.release(std::move(block), mti, big);
                check(pool.driver_blocks_freed()) == 1u;
                check(pool.driver_bytes_in_use()) == 0;

                // Nothing was pooled, so the next acquire hits the driver again.
                auto again = pool.acquire(vulkan.device, mti, big);
                check(pool.driver_blocks_allocated()) == 2u;
                pool.release(std::move(again), mti, big);
                check(pool.driver_blocks_freed()) == 2u;

                pool.clear();
            });


    /// ### Blocks up to the threshold are pooled and retained
    /**
     * A block whose size is at most `driver_block_size` is retained on
     * `release` and reused by the next `acquire` with no fresh driver
     * allocation; it is freed only by `clear`.
     */
    auto const threshold_pooled =
            suite.test("block-at-threshold-is-pooled", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                constexpr std::size_t threshold = 1u << 20;
                planet::vk::device_memory_block_pool pool{
                        "poolable", threshold};

                // Exactly at the threshold: poolable.
                auto block = pool.acquire(vulkan.device, mti, threshold);
                check(pool.driver_blocks_allocated()) == 1u;

                pool.release(std::move(block), mti, threshold);
                check(pool.driver_blocks_freed()) == 0u; // retained, not freed

                auto reused = pool.acquire(vulkan.device, mti, threshold);
                check(pool.driver_blocks_allocated()) == 1u; // reused
                pool.release(std::move(reused), mti, threshold);

                pool.clear();
                check(pool.driver_blocks_freed()) == 1u; // freed only at clear
            });


    /// ### `clear` frees every pooled block
    /**
     * Blocks held by the pool are freed only by `clear`, at which point the
     * driver-deallocation count matches the number of blocks allocated and the
     * in-use byte count returns to zero.
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


    /// ### Concurrent `acquire`/`release` is race-free
    /**
     * Many threads churn blocks of one key through the pool without
     * double-freeing or racing (TSan-clean). Once every thread has returned its
     * block, `clear` frees exactly the blocks that were allocated.
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

                check(pool.driver_blocks_freed()) == 0u;
                pool.clear();
                check(pool.driver_blocks_freed())
                        == pool.driver_blocks_allocated();
                check(pool.driver_bytes_in_use()) == 0;
            });


    /// ## Pool hosted on the device


    /// ### Consumers of one key share the device-hosted pool
    /**
     * The pool lives on `vk::device`, so any consumer of the same
     * `(memory_type_index, block_size)` draws from one shared free list. A
     * block one consumer acquires and releases is reused by the next consumer
     * of the same key, so the driver-allocation count does not move.
     */
    auto const device_hosted =
            suite.test("device-hosted-shared-pool", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                auto &pool = vulkan.device.block_pool;
                auto const before = pool.driver_blocks_allocated();

                auto first = pool.acquire(vulkan.device, mti, block_64mib);
                check(pool.driver_blocks_allocated()) == before + 1u;
                pool.release(std::move(first), mti, block_64mib);

                auto second = pool.acquire(vulkan.device, mti, block_64mib);
                check(pool.driver_blocks_allocated()) == before + 1u;
                pool.release(std::move(second), mti, block_64mib);
            });


    /// ### The device destructor frees retained blocks
    /**
     * A block released back to the device-hosted pool is retained, not handed
     * to the driver. The device destructor clears the pool before destroying
     * the device, so the retained driver memory is freed cleanly with no leak
     * reported at teardown. Acquiring and releasing here leaves a block pooled
     * for the device destructor to reclaim.
     */
    auto const teardown =
            suite.test("device-teardown-frees-pool", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                auto &pool = vulkan.device.block_pool;
                auto block = pool.acquire(vulkan.device, mti, block_64mib);
                check(pool.driver_blocks_allocated()) >= 1u;

                pool.release(std::move(block), mti, block_64mib);
                check(pool.driver_blocks_freed()) == 0u;
            });


    /// ## Allocators draw from the shared pool


    /// ### Two allocators share one driver block
    /**
     * A block one `device_memory_allocator` draws from the shared device pool
     * and releases is reused by a different, later allocator with no new
     * `vkAllocateMemory`. Requesting more than the allocator's
     * `allocation_block_size` routes the block through the shared pool, rather
     * than the allocator's own free list, on both allocation and deallocation.
     */
    auto const allocators_share =
            suite.test("allocators-share-driver-block", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);
                auto &pool = vulkan.device.block_pool;

                planet::vk::device_memory_allocator_configuration config;
                config.allocation_block_size = 1u << 20; // 1 MiB
                config.split = false;
                // Larger than the block size, so it flows through the pool
                // rather than the allocator's local free list.
                constexpr std::size_t oversized = 2u << 20;
                constexpr std::size_t alignment = 1u << 10;

                auto const before = pool.driver_blocks_allocated();
                {
                    planet::vk::device_memory_allocator first{
                            "share-first", vulkan.device, config};
                    auto memory = first.allocate(oversized, mti, alignment);
                    check(pool.driver_blocks_allocated()) == before + 1u;
                    // `memory` is released and `first` torn down at scope exit,
                    // handing the block back to the shared pool.
                }

                {
                    planet::vk::device_memory_allocator second{
                            "share-second", vulkan.device, config};
                    auto memory = second.allocate(oversized, mti, alignment);
                    check(pool.driver_blocks_allocated()) == before + 1u;
                }
            });


    /// ### A destroyed allocator returns its blocks to the pool
    /**
     * An allocator draws whole blocks from the shared pool but recycles
     * normal-sized ones into its own free list, handing them back to the shared
     * pool only when it is destroyed. After one allocator is destroyed, a
     * second allocator of the same key reuses the returned block with no new
     * driver allocation -- if the destroyed allocator had instead dropped the
     * block to the driver, this second `acquire` would allocate afresh.
     */
    auto const teardown_returns_blocks =
            suite.test("allocator-teardown-returns-blocks", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);
                auto &pool = vulkan.device.block_pool;

                planet::vk::device_memory_allocator_configuration config;
                // A block size of its own so the delta below is not perturbed
                // by the device's other allocators.
                config.allocation_block_size = 3u << 20;
                // A small request: the whole block lands in the allocator's
                // local free list, reaching the shared pool only at teardown.
                constexpr std::size_t small = 1u << 10;
                constexpr std::size_t alignment = 1u << 10;

                auto const allocated_before = pool.driver_blocks_allocated();
                {
                    planet::vk::device_memory_allocator first{
                            "teardown-first", vulkan.device, config};
                    auto memory = first.allocate(small, mti, alignment);
                    check(pool.driver_blocks_allocated())
                            == allocated_before + 1u;
                    // `memory` and then `first` are torn down here; the whole
                    // block must land back in the pool, not the driver.
                }

                {
                    planet::vk::device_memory_allocator second{
                            "teardown-second", vulkan.device, config};
                    auto memory = second.allocate(small, mti, alignment);
                    check(pool.driver_blocks_allocated())
                            == allocated_before + 1u;
                }
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
