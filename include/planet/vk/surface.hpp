#pragma once


#include <planet/vk/forward.hpp>
#include <planet/vk/helpers.hpp>

#include <mutex>


namespace planet::vk {


    /// ## Surface
    class surface final {
        friend class instance;
        friend class queue;


        VkSurfaceKHR handle;
        surface(vk::instance const &, VkSurfaceKHR);


        std::optional<std::uint32_t> graphics, present;
        std::mutex transfer_mutex;
        std::size_t transfer_count = {};
        std::vector<std::uint32_t> transfer;
        void return_queue_index(std::uint32_t);


      public:
        ~surface();

        surface(surface const &) = delete;
        surface(surface &&) = delete;
        surface &operator=(surface const &) = delete;
        surface &operator=(surface &&i) = delete;

        VkSurfaceKHR get() const noexcept { return handle; }

        vk::instance const &instance;


        /// ### Surface characteristics

        /// Refreshes the characteristics of the surface for the specified GPU
        void refresh_characteristics(physical_device const &);


        /// ### Queue indexes and properties
        std::vector<VkQueueFamilyProperties> queue_family_properties;

        bool has_queue_families() const noexcept {
            return graphics.has_value() and present.has_value();
        }
        std::uint32_t graphics_queue_index() const noexcept {
            return graphics.value();
        }
        std::uint32_t presentation_queue_index() const noexcept {
            return present.value();
        }

        /// #### Return a transfer queue index
        /**
         * It's possible that there is no transfer queue available, in which
         * case the `queue` returned will evaluate to `false`.
         */
        queue transfer_queue();


        /// ### Swap chain support
        /**
         * These are only filled in if the physical device supports the required
         * queues.
         */
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        VkSurfaceFormatKHR best_format = {};
        std::vector<VkPresentModeKHR> present_modes;
        VkPresentModeKHR best_present_mode = {};

        bool has_adequate_swap_chain_support() const noexcept {
            return not formats.empty() and not present_modes.empty();
        }
    };


}
