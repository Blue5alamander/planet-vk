#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>


/**
 * ## planet::vk::instance
 */


VkApplicationInfo planet::vk::application_info() {
    VkApplicationInfo i = {};
    i.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    i.pApplicationName = "Vulkan application";
    i.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    i.pEngineName = "Planet";
    i.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    i.apiVersion = VK_API_VERSION_1_1;
    return i;
}


VkInstanceCreateInfo planet::vk::instance::info(
        extensions const &exts, VkApplicationInfo const &app_info) {
    VkInstanceCreateInfo i = {};
    i.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    i.pApplicationInfo = &app_info;
    i.enabledExtensionCount = exts.vulkan_extensions.size();
    i.ppEnabledExtensionNames = exts.vulkan_extensions.data();
    i.enabledLayerCount = exts.validation_layers.size();
    i.ppEnabledLayerNames = exts.validation_layers.data();
    return i;
}


planet::vk::instance::instance(
        VkInstanceCreateInfo const &info,
        std::function<VkSurfaceKHR(VkInstance)> mksurface) {
    planet::vk::worked(vkCreateInstance(&info, nullptr, &handle));
    auto devices = planet::vk::fetch_vector<
            vkEnumeratePhysicalDevices, VkPhysicalDevice>(handle);
    physical_devices.reserve(devices.size());
    surface = mksurface(handle);
    for (auto dh : devices) { physical_devices.emplace_back(dh, surface); }

    bool const has_discrete_gpu =
            std::find_if(
                    physical_devices.begin(), physical_devices.end(),
                    [](auto const &d) {
                        return d.properties.deviceType
                                == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    })
            != physical_devices.end();

    for (auto const &d : physical_devices) {
        if (has_discrete_gpu
            and d.properties.deviceType
                    == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_in_use = &d;
        } else if (
                not has_discrete_gpu
                and d.properties.deviceType
                        == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_in_use = &d;
        }
    }
    if (not gpu_in_use) {
        throw felspar::stdexcept::runtime_error{
                "No suitable GPU has been found"};
    }
}


void planet::vk::instance::reset() noexcept {
    if (surface) { vkDestroySurfaceKHR(handle, surface, nullptr); }
    if (handle) { vkDestroyInstance(handle, nullptr); }
}


/**
 * ## planet::vk::physical_device
 */


planet::vk::physical_device::physical_device(
        VkPhysicalDevice h, VkSurfaceKHR surface)
: handle{h} {
    vkGetPhysicalDeviceProperties(handle, &properties);
    vkGetPhysicalDeviceFeatures(handle, &features);
    vkGetPhysicalDeviceMemoryProperties(handle, &memory_properties);
    queue_family_properties = fetch_vector<
            vkGetPhysicalDeviceQueueFamilyProperties, VkQueueFamilyProperties>(
            handle);

    for (std::uint32_t index = {}; const auto &qf : queue_family_properties) {
        if (qf.queueFlags bitand VK_QUEUE_GRAPHICS_BIT) { graphics = index; }

        VkBool32 presentf = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(handle, index, surface, &presentf);
        if (presentf) { present = index; }

        ++index;
    }
}
