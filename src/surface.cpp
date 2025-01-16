#include <planet/log.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/queue.hpp>


/// ## `planet::vk::queue`


planet::vk::queue::queue() {}
planet::vk::queue::queue(queue &&q)
: surface{std::exchange(q.surface, nullptr)},
  index{std::exchange(q.index, 0)} {}

planet::vk::queue::queue(vk::surface *const s, std::uint32_t const i)
: surface{s}, index{i} {}

planet::vk::queue::~queue() {
    if (surface) { surface->return_queue_index(index); }
}


planet::vk::queue::operator bool() const noexcept { return surface != nullptr; }

std::uint32_t planet::vk::queue::get() const {
    if (not surface) {
        throw felspar::stdexcept::logic_error{"This queue is empty"};
    }
    return index;
}


/// ## `planet::vk::surface`


planet::vk::surface::surface(vk::instance const &i, VkSurfaceKHR h)
: handle{h}, instance{i} {}


planet::vk::surface::~surface() {
    /// TODO Can't we use our handle type that's owned by the instance here?
    if (handle) { vkDestroySurfaceKHR(instance.get(), handle, nullptr); }
}


void planet::vk::surface::refresh_characteristics(physical_device const &device) {
    std::scoped_lock _{transfer_mutex};

    graphics = {};
    present = {};
    if (transfer_count != transfer.size()) {
        throw felspar::stdexcept::logic_error{
                "Cannot refresh the surface's characterstics whilst there are transfer queues being used"};
    } else {
        transfer_count = 0;
        transfer.clear();
    }

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
    for (std::uint32_t index = {}; const auto &qf : queue_family_properties) {
        bool const is_graphics_queue =
                qf.queueFlags bitand VK_QUEUE_GRAPHICS_BIT;
        bool const is_transfer_queue =
                qf.queueFlags bitand VK_QUEUE_TRANSFER_BIT;

        VkBool32 is_presentation_queue = false;
        worked(vkGetPhysicalDeviceSurfaceSupportKHR(
                device.get(), index, handle, &is_presentation_queue));

        if (is_graphics_queue) { graphics = index; }
        if (is_presentation_queue) { present = index; }
        if (is_transfer_queue and index != graphics and index != present) {
            transfer.push_back(index);
            transfer_count = transfer.size();
        }

        planet::log::debug(
                "Surface queue", index, "is_graphics_queue", is_graphics_queue,
                "is_presentation_queue",
                static_cast<bool>(is_presentation_queue), "is_transfer_queue",
                is_transfer_queue);

        ++index;
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
        best_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto const &m : present_modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                best_present_mode = m;
                break;
            }
        }
    }
}


auto planet::vk::surface::transfer_queue() -> queue {
    std::scoped_lock _{transfer_mutex};
    if (not transfer.empty()) {
        auto const index = transfer.back();
        transfer.pop_back();
        return queue{this, index};
    } else {
        return {};
    }
}

void planet::vk::surface::return_queue_index(std::uint32_t const index) {
    std::scoped_lock _{transfer_mutex};
    transfer.push_back(index);
}
