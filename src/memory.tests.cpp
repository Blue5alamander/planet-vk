#include <planet/log.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/headless.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/memory.hpp>

#include <felspar/test.hpp>


namespace {


    /// ### Find a memory type index that can back a driver allocation
    std::uint32_t any_memory_type(planet::vk::headless const &vulkan) {
        VkMemoryRequirements requirements{};
        requirements.memoryTypeBits = ~std::uint32_t{};
        return vulkan.instance.find_memory_type(
                requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }


    auto const suite =
            felspar::testsuite("vulkan::device_memory_allocator", []() {
                planet::log::active = planet::log::level::error;
            });


    /// ## Sub-allocation through `split`


    /// ### Sequential same-alignment allocations carve up one block
    /**
     * Two requests that fit inside a single `allocation_block_size` block draw
     * from the same driver block: both have the same memory handle and the
     * second does not force a fresh driver allocation.
     */
    auto const carves = suite.test("carves-single-block", [](auto check) {
        planet::vk::headless vulkan;
        auto const mti = any_memory_type(vulkan);
        auto const before = vulkan.device.block_pool.driver_blocks_allocated();

        planet::vk::device_memory_allocator_configuration config;
        config.allocation_block_size = 64u << 10; // 64 KiB
        config.split = true;
        constexpr std::size_t alignment = 1u << 10; // 1 KiB

        planet::vk::device_memory_allocator allocator{
                "carves", vulkan.device, config};

        auto first = allocator.allocate(4u << 10, mti, alignment);
        auto second = allocator.allocate(4u << 10, mti, alignment);

        check(first.size()) == (4u << 10);
        check(second.size()) == (4u << 10);
        check(first.get()) == second.get(); // same driver block
        check(vulkan.device.block_pool.driver_blocks_allocated())
                == before + 1u;
    });


    /// ### A larger alignment than the block's current offset still allocates
    /**
     * A second allocation that asks for a *larger* alignment than an earlier
     * one leaves the splitting block's free region starting at an offset that
     * is not aligned for the new request. The allocator must satisfy the
     * request -- pulling a fresh block when the current one cannot fit the
     * aligned sub-allocation -- rather than throwing.
     *
     * The sizes are chosen so the leftover region (`byte_count`) is large
     * enough to pass the allocator's `splitting.size() < bytes` check yet too
     * small to hold `bytes` once realigned: with a block of 3072 bytes, a first
     * 1 KiB / 1 KiB-aligned allocation leaves 2048 bytes starting at offset
     * 1024, and a 2 KiB / 2 KiB-aligned request needs to realign to offset 2048
     * first, so 2048 + (2048 - 1024) = 3072 bytes are required from only 2048
     * left.
     */
    auto const realigns =
            suite.test("larger-alignment-pulls-fresh-block", [](auto check) {
                planet::vk::headless vulkan;
                auto const mti = any_memory_type(vulkan);

                planet::vk::device_memory_allocator_configuration config;
                config.allocation_block_size = 3u << 10; // 3 KiB
                config.split = true;

                planet::vk::device_memory_allocator allocator{
                        "realigns", vulkan.device, config};

                auto first = allocator.allocate(1u << 10, mti, 1u << 10);
                check(first.size()) == (1u << 10);

                // Must not throw -- the request is satisfiable from a fresh
                // block even though it does not fit the current splitting block
                // once the offset is realigned to 2 KiB.
                auto second = allocator.allocate(2u << 10, mti, 2u << 10);
                check(second.size()) == (2u << 10);
                check(second.get()) != VK_NULL_HANDLE;
            });


}
