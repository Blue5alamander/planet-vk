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


planet::vk::instance::instance(VkInstanceCreateInfo const &info) {
    planet::vk::worked(vkCreateInstance(&info, nullptr, &handle));
}


void planet::vk::instance::reset() noexcept {
    if (handle) { vkDestroyInstance(handle, nullptr); }
}
