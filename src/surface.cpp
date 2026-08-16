#include <planet/log.hpp>
#include <planet/platform.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/queue.hpp>


namespace {
    /// ### Readable name for a present mode
    /**
     * Which mode the surface settled on decides frame pacing, so it belongs in
     * the log rather than being inferred from the frame rate afterwards.
     */
    std::string_view present_mode_name(VkPresentModeKHR const mode) noexcept {
        switch (mode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR: return "immediate";
        case VK_PRESENT_MODE_MAILBOX_KHR: return "mailbox";
        case VK_PRESENT_MODE_FIFO_KHR: return "fifo";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "fifo-relaxed";
        default: return "unknown";
        }
    }
}


/// ## `planet::vk::surface`


planet::vk::surface::surface(vk::instance const &i, VkSurfaceKHR h)
: handle{h}, instance{i} {}


planet::vk::surface::~surface() {
    /// TODO Can't we use our handle type that's owned by the instance here?
    if (handle) { vkDestroySurfaceKHR(instance.get(), handle, nullptr); }
}


void planet::vk::surface::refresh_characteristics(physical_device const &device) {
    graphics = {};
    present = {};
    transfer = {};

    queue_family_properties = fetch_vector<
            vkGetPhysicalDeviceQueueFamilyProperties, VkQueueFamilyProperties>(
            device.get());

    /**
     * TODO OK, we can't share queues across threads, so we really need to be
     * able to find out if we have multiple queues and then hand out queues to
     * the threads that need them. Presumably this can be done by having the
     * `command_pool` own the queue that it uses for as long as it lives. If no
     * queue is available for it then it should throw -- i.e. only create a
     * `command_pool ` if you know that there is a queue for it to use
     * (presumably pass in the queue).
     *
     * The "main" thread should presumably be able to use the present queue (if
     * suitable).
     *
     * All of this implies a queue wrapper that is able to deal with these
     * things, and place itself back into the "available queues" structure we're
     * going to need here.
     */
    for (std::uint32_t index = {}; auto const &qf : queue_family_properties) {
        bool const is_graphics_queue =
                qf.queueFlags bitand VK_QUEUE_GRAPHICS_BIT;
        bool const is_transfer_queue =
                qf.queueFlags bitand VK_QUEUE_TRANSFER_BIT;

        VkBool32 is_presentation_queue = false;
        worked(vkGetPhysicalDeviceSurfaceSupportKHR(
                device.get(), index, handle, &is_presentation_queue));

        if (is_graphics_queue) { graphics = index; }
        if (is_presentation_queue) { present = index; }
        if (is_transfer_queue and is_graphics_queue and index != graphics
            and index != present) {
            transfer = index;
        }

        planet::log::debug(
                "Surface queue", index, "queueCount", qf.queueCount,
                "is_graphics_queue", is_graphics_queue, "is_presentation_queue",
                static_cast<bool>(is_presentation_queue), "is_transfer_queue",
                is_transfer_queue);

        ++index;
    }
    if (not transfer and graphics and present) {
        /**
         * We didn't find a suitable dedicated transfer queue family. There are
         * enough queues in the graphics family (which always has the transfer
         * ability), so we can piggy back on that if there are enough of them.
         *
         * If the presentation and graphics queue familiies are the same then
         * we'll actually only use one queue for both, so we only need one exta
         * for our transfer queue.
         */
        std::uint32_t const required_count = 2;
        if (queue_family_properties[*graphics].queueCount >= required_count) {
            transfer = graphics;
        }
    }

    if (has_queue_families()) {
        worked(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                device.get(), handle, &capabilities));
        formats = fetch_vector<
                vkGetPhysicalDeviceSurfaceFormatsKHR, VkSurfaceFormatKHR>(
                device.get(), handle);
        present_modes = fetch_vector<
                vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR>(
                device.get(), handle);

        if (not formats.empty()) {
            best_format = formats[0];
            for (auto const &f : formats) {
                if (f.format == VK_FORMAT_B8G8R8A8_SRGB
                    and f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    best_format = f;
                    break;
                }
            }
        }
        /// ### Present mode
        /**
         * `FIFO` is the only mode the specification requires every
         * implementation to support, and it paces the game to the display.
         * `MAILBOX` trades memory and wasted frames for lower latency, which is
         * worth having elsewhere.
         *
         * Android stays on `FIFO`. Its `MAILBOX` left the compositor holding a
         * buffer for minutes at a time -- presents completed and the swap chain
         * images cycled, but nothing new reached the screen, so the picture sat
         * whole screens behind the game. Rendering ahead of the panel also
         * bought nothing there: the phone ran above 80fps against a 60Hz
         * display and discarded the difference.
         */
        best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        if (current_platform != platform::android) {
            for (auto const &m : present_modes) {
                if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                    best_present_mode = m;
                    break;
                }
            }
        }
        planet::log::info(
                "Surface present mode", present_mode_name(best_present_mode),
                "chosen from", present_modes.size(), "supported");
    }
}
