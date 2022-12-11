#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>

#include <felspar/memory/small_vector.hpp>


/**
 * ## planet::vk::extensions
 */


planet::vk::extensions::extensions() {
#ifndef NDEBUG
    validation_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
}


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
    pdevices.reserve(devices.size());
    surface = mksurface(handle);
    for (auto dh : devices) { pdevices.emplace_back(dh, surface); }

    bool const has_discrete_gpu =
            std::find_if(
                    pdevices.begin(), pdevices.end(),
                    [](auto const &d) {
                        return d.properties.deviceType
                                == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    })
            != pdevices.end();

    for (auto const &d : pdevices) {
        bool const is_suitable =
                d.has_queue_families() and d.has_adequate_swap_chain_support();
        if (is_suitable and has_discrete_gpu
            and d.properties.deviceType
                    == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_in_use = &d;
            break;
        } else if (
                is_suitable and not has_discrete_gpu
                and d.properties.deviceType
                        == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_in_use = &d;
            break;
        }
    }
    if (not gpu_in_use) {
        throw felspar::stdexcept::runtime_error{
                "No suitable GPU has been found"};
    }
}


void planet::vk::instance::reset() noexcept {
    if (handle) {
        if (surface) { vkDestroySurfaceKHR(handle, surface, nullptr); }
        vkDestroyInstance(handle, nullptr);
    }
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

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            handle, surface, &surface_capabilities);
    surface_formats = fetch_vector<
            vkGetPhysicalDeviceSurfaceFormatsKHR, VkSurfaceFormatKHR>(
            handle, surface);
    present_modes = fetch_vector<
            vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR>(
            handle, surface);
}


/**
 * ## planet::vk::device
 */


planet::vk::device::device(
        vk::instance const &instance, vk::extensions const &extensions) {
    felspar::memory::small_vector<VkDeviceQueueCreateInfo, 2> queue_create_infos;
    const float queue_priority = 1.f;
    for (auto const q : std::array{
                 instance.gpu().graphics_queue_index(),
                 instance.gpu().presentation_queue_index()}) {
        queue_create_infos.emplace_back();
        queue_create_infos.back().sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos.back().queueFamilyIndex = q;
        queue_create_infos.back().queueCount = 1;
        queue_create_infos.back().pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = queue_create_infos.size();
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.enabledLayerCount = extensions.validation_layers.size();
    create_info.ppEnabledLayerNames = extensions.validation_layers.data();
    create_info.enabledExtensionCount = extensions.device_extensions.size();
    create_info.ppEnabledExtensionNames = extensions.device_extensions.data();
    create_info.pEnabledFeatures = &device_features;
    planet::vk::worked(vkCreateDevice(
            instance.gpu().get(), &create_info, nullptr, &handle));

    vkGetDeviceQueue(
            handle, instance.gpu().graphics_queue_index(), 0, &graphics_queue);
    vkGetDeviceQueue(
            handle, instance.gpu().presentation_queue_index(), 0,
            &present_queue);
}


planet::vk::device::~device() {
    if (handle) {
        vkDeviceWaitIdle(handle);
        vkDestroyDevice(handle, nullptr);
    }
}
