#include <planet/log.hpp>
#include <planet/vk/device.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/instance.hpp>

#include <felspar/exceptions.hpp>
#include <felspar/memory/small_vector.hpp>


/// ## `planet::vk::debug_messenger`


namespace {
    VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
            VkDebugUtilsMessageSeverityFlagBitsEXT const severity,
            VkDebugUtilsMessageTypeFlagsEXT,
            VkDebugUtilsMessengerCallbackDataEXT const *data,
            void *) {
        /// TODO Include the objects in the log messages
        switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            planet::log::debug("Vulkan", data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            planet::log::info("Vulkan", data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            planet::log::warning("Vulkan", data->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            planet::log::error("Vulkan", data->pMessage);
            break;
        default:
            planet::log::warning("Vulkan (unknown severity)", data->pMessage);
            break;
        }
        return VK_FALSE;
    }
}


VkDebugUtilsMessengerCreateInfoEXT planet::vk::debug_messenger::create_info(
        PFN_vkDebugUtilsMessengerCallbackEXT cb, void *ud) {
    VkDebugUtilsMessengerCreateInfoEXT info{};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
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


void planet::vk::detail::throw_result_error(
        VkResult const result, felspar::source_location const &loc) {
    throw felspar::stdexcept::runtime_error{detail::error(result), loc};
}


/// ## `planet::vk::device`


planet::vk::device::device(
        vk::instance const &i, vk::extensions const &extensions)
: instance{i} {
    felspar::memory::small_vector<VkDeviceQueueCreateInfo, 3> queue_create_infos;
    auto const graphics_family = instance.surface.graphics_queue_family_index();
    auto const presentation_family =
            instance.surface.presentation_queue_family_index();
    auto const transfer_family = instance.surface.transfer_queue_family_index();

    static auto constexpr queue_priority = std::array{1.0f, 1.0f};
    queue_create_infos.push_back(
            {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
             .pNext = nullptr,
             .flags = {},
             .queueFamilyIndex = graphics_family,
             .queueCount = static_cast<std::uint32_t>(
                     transfer_family == graphics_family ? 2 : 1),
             .pQueuePriorities = queue_priority.data()});
    if (graphics_family != presentation_family) {
        queue_create_infos.push_back(
                {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                 .pNext = nullptr,
                 .flags = {},
                 .queueFamilyIndex = presentation_family,
                 .queueCount = 1,
                 .pQueuePriorities = queue_priority.data()});
    }
    if (transfer_family and transfer_family != graphics_family) {
        queue_create_infos.push_back(
                {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                 .pNext = nullptr,
                 .flags = {},
                 .queueFamilyIndex = *transfer_family,
                 .queueCount = 1,
                 .pQueuePriorities = queue_priority.data()});
    }

    VkPhysicalDeviceFeatures device_features = {};
    device_features.independentBlend = VK_TRUE;
    device_features.samplerAnisotropy = VK_TRUE;
    device_features.sampleRateShading = VK_TRUE;

    VkDeviceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    info.queueCreateInfoCount = queue_create_infos.size();
    info.pQueueCreateInfos = queue_create_infos.data();
    info.enabledLayerCount = extensions.validation_layers.size();
    info.ppEnabledLayerNames = extensions.validation_layers.data();
    info.enabledExtensionCount = extensions.device_extensions.size();
    info.ppEnabledExtensionNames = extensions.device_extensions.data();
    info.pEnabledFeatures = &device_features;
    planet::log::info(
            "Creating device with validation layers:",
            extensions.validation_layers,
            "and device extensions:", extensions.device_extensions);
    planet::vk::worked(
            vkCreateDevice(instance.gpu().get(), &info, nullptr, &handle));

    vkGetDeviceQueue(handle, graphics_family, 0, &graphics_queue);
    vkGetDeviceQueue(handle, presentation_family, 0, &present_queue);
    if (transfer_family) {
        VkQueue tq = {};
        vkGetDeviceQueue(
                handle, *transfer_family,
                static_cast<std::uint32_t>(
                        transfer_family == graphics_family ? 1 : 0),
                &tq);
        held_transfer_queue = std::pair{tq, *transfer_family};
    }
}

planet::vk::device::~device() {
    staging_memory.clear_without_check();
    startup_memory.clear_without_check();
    if (handle) {
        planet::log::debug("Destructing Vulkan device");
        wait_idle();
        vkDestroyDevice(handle, nullptr);
        planet::log::debug("Device now destroyed");
    }
}


planet::vk::queue planet::vk::device::transfer_queue() {
    std::scoped_lock _{transfer_queue_mutex};
    if (held_transfer_queue) {
        vk::queue q{
                this, held_transfer_queue->first, held_transfer_queue->second};
        held_transfer_queue.reset();
        return q;
    } else {
        return {};
    }
}
void planet::vk::device::return_transfer_queue(
        VkQueue const q, std::uint32_t const i) {
    std::scoped_lock _{transfer_queue_mutex};
    held_transfer_queue = std::pair{q, i};
}


void planet::vk::device::wait_idle() const {
    worked(vkDeviceWaitIdle(handle));
    planet::log::info("GPU device now idle");
}


/// ## `planet::vk::extensions`


planet::vk::extensions::extensions() {
#if defined(PLANET_VK_VALIDATION) or not defined(NDEBUG)
    static constexpr std::string_view vkl_validation =
            "VK_LAYER_KHRONOS_validation";
    auto const layers = supported_validation_layers();
    if (std::find_if(
                layers.begin(), layers.end(),
                [&](auto const vl) { return vl.layerName == vkl_validation; })
        != layers.end()) {
        validation_layers.push_back(vkl_validation.data());
    } else {
        planet::log::warning(
                "We wanted validation layer", vkl_validation,
                "but didn't find it");
    }
#endif
}


std::span<VkLayerProperties const>
        planet::vk::extensions::supported_validation_layers() {
    static auto const layers = []() {
        auto l = planet::vk::fetch_vector<
                vkEnumerateInstanceLayerProperties, VkLayerProperties>();
        std::vector<char const *> names;
        names.reserve(l.size());
        for (auto const &layer : l) { names.push_back(layer.layerName); }
        planet::log::info("Supported validation layers", names);
        return l;
    }();
    return layers;
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
    auto const devices = planet::vk::fetch_vector<
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

    if (pdevices.empty()) {
        planet::log::critical("No GPU physical devices found");
    } else {
        planet::log::info(
                "Found", pdevices.size(), "devices - has_discrete_gpu",
                has_discrete_gpu);
    }
    /**
     * TODO Ideally we'd sort all found GPUs by their suitability and then
     * choose the best one
     */
    for (auto const &d : pdevices) {
        surface.refresh_characteristics(d);
        bool const has_queue_families = surface.has_queue_families();
        bool const surface_supports_swap_chain =
                surface.has_adequate_swap_chain_support();
        bool const is_suitable =
                has_queue_families and surface_supports_swap_chain;
        bool const is_discrete =
                d.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        bool const is_integrated = d.properties.deviceType
                == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

        std::vector<char const *> names;
        names.reserve(d.extensions.size());
        for (auto const &ex : d.extensions) {
            names.push_back(ex.extensionName);
        }
        planet::log::info(
                "GPU", d.properties.deviceName, "is_discrete", is_discrete,
                "is_integrated", is_integrated, "has_queue_families",
                has_queue_families, "surface_supports_swap_chain",
                surface_supports_swap_chain, "extensions", names);

        if (is_suitable and has_discrete_gpu and is_discrete) {
            gpu_in_use = &d;
            break;
        } else if (is_suitable and not has_discrete_gpu) {
            gpu_in_use = &d;
            break;
        }
    }
    if (not gpu_in_use and pdevices.size() == 1) {
        planet::log::warning(
                "Only one GPU found, going to try it, but expect errors");
    }
    if (not gpu_in_use) {
        throw felspar::stdexcept::runtime_error{
                "No suitable GPU has been found"};
    } else {
        planet::log::info(
                "GPU", gpu_in_use->properties.deviceName,
                "has been selected for use");
    }
    // surface.refresh_characteristics(*gpu_in_use);
}


planet::vk::instance::instance_handle::~instance_handle() {
    if (h) {
        planet::log::debug("Destructing Vulkan instance");
        vkDestroyInstance(h, nullptr);
    }
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

    VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts
            bitand properties.limits.framebufferDepthSampleCounts;

    if (counts bitand VK_SAMPLE_COUNT_64_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_64_BIT;
    } else if (counts bitand VK_SAMPLE_COUNT_32_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_32_BIT;
    } else if (counts bitand VK_SAMPLE_COUNT_16_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_16_BIT;
    } else if (counts bitand VK_SAMPLE_COUNT_8_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_8_BIT;
    } else if (counts bitand VK_SAMPLE_COUNT_4_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_4_BIT;
    } else if (counts bitand VK_SAMPLE_COUNT_2_BIT) {
        msaa_samples = VK_SAMPLE_COUNT_2_BIT;
    } else {
        msaa_samples = VK_SAMPLE_COUNT_1_BIT;
    }

    extensions = planet::vk::fetch_vector<
            vkEnumerateDeviceExtensionProperties, VkExtensionProperties>(
            handle, nullptr);
}


VkFormatProperties planet::vk::physical_device::format_properties(
        VkFormat const format) const {
    VkFormatProperties properties{};
    vkGetPhysicalDeviceFormatProperties(get(), format, &properties);
    return properties;
}


/// ## `planet::vk::queue`


planet::vk::queue::queue() {}
planet::vk::queue::queue(queue &&q)
: device{std::exchange(q.device, nullptr)},
  handle{std::exchange(q.handle, VK_NULL_HANDLE)},
  index{std::exchange(q.index, 0)} {}

planet::vk::queue::queue(vk::device *const d, VkQueue q, std::uint32_t const i)
: device{d}, handle{q}, index{i} {}

planet::vk::queue::~queue() {
    if (device) { device->return_transfer_queue(handle, index); }
}


planet::vk::queue::operator bool() const noexcept { return device != nullptr; }

VkQueue planet::vk::queue::get() const {
    if (not device) {
        throw felspar::stdexcept::logic_error{"This queue is empty"};
    }
    return handle;
}

std::uint32_t planet::vk::queue::family_index() const {
    if (not device) {
        throw felspar::stdexcept::logic_error{"This queue is empty"};
    }
    return index;
}
