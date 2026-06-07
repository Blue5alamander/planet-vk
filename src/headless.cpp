#include <planet/vk/headless.hpp>

#include <felspar/exceptions/runtime_error.hpp>


namespace {


    /// Build the instance extensions needed for a headless surface
    planet::vk::extensions headless_extensions() {
        planet::vk::extensions exts;
        /// `VK_EXT_headless_surface` depends on the base `VK_KHR_surface`
        exts.vulkan_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        exts.vulkan_extensions.push_back(
                VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
        return exts;
    }


    /// Create a window-less surface using `VK_EXT_headless_surface`
    VkSurfaceKHR make_headless_surface(VkInstance const handle) {
        /// The extension entry point has to be fetched at runtime
        auto const create = reinterpret_cast<PFN_vkCreateHeadlessSurfaceEXT>(
                vkGetInstanceProcAddr(handle, "vkCreateHeadlessSurfaceEXT"));
        if (not create) {
            throw felspar::stdexcept::runtime_error{
                    "vkCreateHeadlessSurfaceEXT is not available -- the "
                    "VK_EXT_headless_surface extension is not supported by this "
                    "Vulkan implementation"};
        }
        VkHeadlessSurfaceCreateInfoEXT const info{
                .sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT,
                .pNext = nullptr,
                .flags = {}};
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        planet::vk::worked(create(handle, &info, nullptr, &surface));
        return surface;
    }


}


planet::vk::headless::headless()
: extensions{headless_extensions()}, instance{[this]() {
      auto app_info = vk::application_info();
      auto info = vk::instance::info(extensions, app_info);
      return vk::instance{extensions, info, make_headless_surface};
  }()} {}
