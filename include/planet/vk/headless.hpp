#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>


namespace planet::vk {


    /// ## Headless Vulkan context
    /**
     * Brings up a full `vk::instance` and `vk::device` backed by a
     * `VK_EXT_headless_surface` surface, so a device can be created with no
     * window, display, or compositor. Useful for tests, CI, and off-screen
     * rendering.
     *
     * A headless surface is a perfectly ordinary `VkSurfaceKHR`, so the normal
     * `instance` → `surface` → `device` path -- including a presentation queue
     * and swap chain support -- works unchanged; only the origin of the surface
     * differs from the windowed (SDL) path.
     *
     * Requires the Vulkan implementation to advertise the
     * `VK_EXT_headless_surface` instance extension. This is present on Mesa's
     * `radv` and the `llvmpipe` software rasteriser, but some proprietary
     * drivers omit it; in that case the constructor throws.
     */
    struct headless final {
        headless();

        headless(headless const &) = delete;
        headless(headless &&) = delete;
        headless &operator=(headless const &) = delete;
        headless &operator=(headless &&) = delete;


        /// The instance extensions needed for a headless surface are added here
        vk::extensions extensions;

        /// The instance, created with a `VK_EXT_headless_surface` surface
        vk::instance instance;

        /// The logical device using the headless surface's queues
        vk::device device{instance, extensions};
    };


}
