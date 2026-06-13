#pragma once


#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>

#include <felspar/exceptions/runtime_error.hpp>


namespace planet::vk {


    /// ## The `VK_EXT_headless_surface` extension is unavailable
    /**
     * Thrown when the Vulkan implementation does not advertise the
     * `VK_EXT_headless_surface` instance extension -- NVIDIA's proprietary
     * driver omits it, for example -- so no window-less surface can be created.
     *
     * It is a distinct exception type so that tests and tools that need a
     * headless device can catch *this specific case* and skip, while genuine
     * Vulkan failures still propagate as errors.
     */
    class headless_not_available final :
    public felspar::stdexcept::runtime_error {
      public:
        explicit headless_not_available(
                std::source_location = std::source_location::current());
    };


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
        /// ### Construction
        headless();

        /// #### Create a headless context, if the platform supports it
        /**
         * Returns a heap-allocated `headless`, or `nullptr` when the Vulkan
         * implementation does not advertise `VK_EXT_headless_surface` so no
         * headless device can be created -- NVIDIA's proprietary driver, for
         * example. Genuine Vulkan failures still propagate as exceptions.
         *
         * Tests and tools can use this to degrade to a skip on drivers that
         * omit the extension, rather than catching `headless_not_available`
         * directly.
         */
        static std::unique_ptr<headless> make_if_available();

        /// #### Not movable or copyable
        headless(headless const &) = delete;
        headless(headless &&) = delete;
        headless &operator=(headless const &) = delete;
        headless &operator=(headless &&) = delete;


        /// ### Attributes

        /// #### Extensions
        vk::extensions extensions;
        /**
         * The instance extensions needed for a headless surface are added here
         */

        /// #### Instance
        vk::instance instance;
        /**
         * The instance, created with a `VK_EXT_headless_surface` surface
         */

        /// #### Device
        vk::device device{instance, extensions};
        /**
         * The logical device using the headless surface's queues
         */
    };


}
