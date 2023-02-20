#include <planet/vk/instance.hpp>


/// ## `planet::vk::surface`


planet::vk::surface::surface(vk::instance const &i, VkSurfaceKHR h)
: handle{h}, instance{i} {}


planet::vk::surface::~surface() {
    if (handle) { vkDestroySurfaceKHR(instance.get(), handle, nullptr); }
}


void planet::vk::surface::refresh_characteristics(physical_device const &device) {
    graphics = {};
    present = {};

    queue_family_properties = fetch_vector<
            vkGetPhysicalDeviceQueueFamilyProperties, VkQueueFamilyProperties>(
            device.get());

    for (std::uint32_t index = {}; const auto &qf : queue_family_properties) {
        if (qf.queueFlags bitand VK_QUEUE_GRAPHICS_BIT) { graphics = index; }

        VkBool32 presentf = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
                device.get(), index, handle, &presentf);
        if (presentf) { present = index; }

        ++index;
    }

    if (has_queue_families()) {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                device.get(), handle, &capabilities);
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
        for (auto const &m : present_modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
                best_present_mode = m;
                break;
            }
        }
    }
}
