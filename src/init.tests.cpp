#include <planet/log.hpp>
#include <planet/vk/headless.hpp>
#include <planet/vk/swap_chain.hpp>

#include <felspar/test.hpp>


namespace {


    auto const suite = felspar::testsuite("vulkan::device", []() {
        planet::log::active = planet::log::level::error;
    });


    /// ### Create a logical device with no window or display
    /**
     * `vk::headless` brings up an instance and device through a
     * `VK_EXT_headless_surface` surface, so the whole `instance` → `surface` →
     * `device` path works with no SDL window and no compositor.
     *
     * This will only run where the Vulkan implementation advertises
     * `VK_EXT_headless_surface` (it does on Mesa's `radv` and the `llvmpipe`
     * software rasteriser; some proprietary drivers omit it).
     */
    auto const headless = suite.test("headless-device", [](auto check) {
        planet::vk::headless vulkan;

        check(vulkan.device.get() != VK_NULL_HANDLE) == true;
        check(vulkan.device.graphics_queue != VK_NULL_HANDLE) == true;
        check(vulkan.device.present_queue != VK_NULL_HANDLE) == true;

        /**
         * The headless surface should advertise
         * `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, so a swap chain that requests it
         * gets `transfer_source::available` and its images can be copied out
         * (the basis for screenshot/readback support).
         */
        planet::vk::swap_chain swap_chain{
                vulkan.device, planet::affine::extents2d{1920, 1080},
                planet::vk::transfer_source::requested};

        check(swap_chain.transfer == planet::vk::transfer_source::available)
                == true;
    });


}
