#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>

#include <felspar/memory/small_vector.hpp>

#include <iostream>


/// ## `planet::vk::debug_messenger`


namespace {
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT,
            VkDebugUtilsMessageTypeFlagsEXT,
            VkDebugUtilsMessengerCallbackDataEXT const *data,
            void *) {
        std::cerr << "Validation message: " << data->pMessage << '\n';
        return VK_FALSE;
    }
}


VkDebugUtilsMessengerCreateInfoEXT planet::vk::debug_messenger::create_info(
        PFN_vkDebugUtilsMessengerCallbackEXT cb, void *ud) {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = cb;
    info.pUserData = ud;
    return info;
}


planet::vk::debug_messenger::debug_messenger(
        instance const &i, PFN_vkDebugUtilsMessengerCallbackEXT cb, void *ud)
: instance_handle{i.get()} {
    auto info = create_info(cb, ud);
    if (auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(
                        instance_handle, "vkCreateDebugUtilsMessengerEXT"));
        func) {
        worked(func(instance_handle, &info, nullptr, &handle));
    } else {
        throw felspar::stdexcept::runtime_error{
                "vkCreateDebugUtilsMessengerEXT extension is not present"};
    }
}


planet::vk::debug_messenger::~debug_messenger() {
    if (instance_handle and handle) {
        if (auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(
                            instance_handle, "vkDestroyDebugUtilsMessengerEXT"));
            func) {
            func(instance_handle, handle, nullptr);
        }
    }
}


/// ## `planet::vk::detail`


/**
 * Error code numbers can be found at
 * <https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkResult.html>
 */
std::string planet::vk::detail::error(VkResult const code) {
    return "Vulkan error: " + [code]() -> std::string {
        switch (code) {
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        default: return "Unknown error " + std::to_string(code);
        }
    }();
}


/// ## `planet::vk::device`


planet::vk::device::device(
        vk::instance const &i, vk::extensions const &extensions)
: instance{i} {
    felspar::memory::small_vector<VkDeviceQueueCreateInfo, 2> queue_create_infos;
    const float queue_priority = 1.f;
    for (auto const q : std::array{
                 instance.surface.graphics_queue_index(),
                 instance.surface.presentation_queue_index()}) {
        queue_create_infos.emplace_back();
        queue_create_infos.back().sType =
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos.back().queueFamilyIndex = q;
        queue_create_infos.back().queueCount = 1;
        queue_create_infos.back().pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    if (instance.surface.graphics_queue_index()
        == instance.surface.presentation_queue_index()) {
        info.queueCreateInfoCount = 1;
    } else {
        info.queueCreateInfoCount = queue_create_infos.size();
    }
    info.pQueueCreateInfos = queue_create_infos.data();
    info.enabledLayerCount = extensions.validation_layers.size();
    info.ppEnabledLayerNames = extensions.validation_layers.data();
    info.enabledExtensionCount = extensions.device_extensions.size();
    info.ppEnabledExtensionNames = extensions.device_extensions.data();
    info.pEnabledFeatures = &device_features;
    planet::vk::worked(
            vkCreateDevice(instance.gpu().get(), &info, nullptr, &handle));

    vkGetDeviceQueue(
            handle, instance.surface.graphics_queue_index(), 0,
            &graphics_queue);
    vkGetDeviceQueue(
            handle, instance.surface.presentation_queue_index(), 0,
            &present_queue);
}


planet::vk::device::~device() {
    staging_memory.clear_without_check();
    startup_memory.clear_without_check();
    if (handle) {
        vkDeviceWaitIdle(handle);
        vkDestroyDevice(handle, nullptr);
    }
}


/// ## `planet::vk::extensions`


planet::vk::extensions::extensions() {
#ifndef NDEBUG
    validation_layers.push_back("VK_LAYER_KHRONOS_validation");
#endif
}


/// ## `planet::vk::instance`


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
        extensions &exts, VkApplicationInfo const &app_info) {
    if (not exts.validation_layers.empty()) {
        exts.vulkan_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
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
        extensions const &exts,
        VkInstanceCreateInfo &info,
        std::function<VkSurfaceKHR(VkInstance)> mksurface)
: handle{[&]() {
      VkInstance h = VK_NULL_HANDLE;
      auto debug_info = debug_messenger::create_info(debug_callback);
      info.pNext = &debug_info;
      planet::vk::worked(vkCreateInstance(&info, nullptr, &h));
      return h;
  }()},
  surface{*this, mksurface(handle.h)} {
    if (not exts.validation_layers.empty()) {
        debug_messenger = {*this, debug_callback};
    }
    auto devices = planet::vk::fetch_vector<
            vkEnumeratePhysicalDevices, VkPhysicalDevice>(handle.h);
    pdevices.reserve(devices.size());
    for (auto dh : devices) { pdevices.emplace_back(dh, surface.get()); }

    bool const has_discrete_gpu =
            std::find_if(
                    pdevices.begin(), pdevices.end(),
                    [](auto const &d) {
                        return d.properties.deviceType
                                == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
                    })
            != pdevices.end();

    for (auto const &d : pdevices) {
        surface.refresh_characteristics(d);
        bool const is_suitable = surface.has_queue_families()
                and surface.has_adequate_swap_chain_support();
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


planet::vk::instance::instance_handle::~instance_handle() {
    if (h) { vkDestroyInstance(h, nullptr); }
}


std::uint32_t planet::vk::instance::find_memory_type(
        VkMemoryRequirements const mr,
        VkMemoryPropertyFlags const properties) const {
    auto const &gpump = gpu().memory_properties;
    for (std::uint32_t index{}; index < gpump.memoryTypeCount; ++index) {
        bool const type_is_correct = (mr.memoryTypeBits bitand (1 << index));
        bool const properties_match =
                (gpump.memoryTypes[index].propertyFlags bitand properties);
        if (type_is_correct and properties_match) { return index; }
    }
    throw felspar::stdexcept::runtime_error{
            "No matching GPU memory was found for allocation"};
}


/// ## `planet::vk::physical_device`


planet::vk::physical_device::physical_device(VkPhysicalDevice h, VkSurfaceKHR)
: handle{h} {
    vkGetPhysicalDeviceProperties(handle, &properties);
    vkGetPhysicalDeviceFeatures(handle, &features);
    vkGetPhysicalDeviceMemoryProperties(handle, &memory_properties);
}


VkFormatProperties planet::vk::physical_device::format_properties(
        VkFormat const format) const {
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(get(), format, &properties);
    return properties;
}
