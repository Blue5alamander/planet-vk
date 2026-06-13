#include <planet/vk/headless.hpp>
#include <planet/vk/helpers.hpp>

#include <algorithm>
#include <string_view>


/// ## `planet::vk::headless`


namespace {


    /// Does the Vulkan loader advertise the named instance extension?
    bool instance_extension_available(std::string_view const name) {
        auto const available = planet::vk::fetch_vector<
                vkEnumerateInstanceExtensionProperties, VkExtensionProperties>(
                nullptr);
        return std::find_if(
                       available.begin(), available.end(),
                       [name](VkExtensionProperties const &ext) {
                           return name == ext.extensionName;
                       })
                != available.end();
    }


    /// Build the instance extensions needed for a headless surface
    planet::vk::extensions headless_extensions() {
        /**
         * `vkCreateInstance` fails with `VK_ERROR_EXTENSION_NOT_PRESENT` -- a
         * generic Vulkan error -- if we enable an extension the loader doesn't
         * advertise, so check for `VK_EXT_headless_surface` up front and throw
         * the dedicated exception. The runtime proc-address check below would
         * never be reached on a driver that omits the extension because
         * instance creation throws first.
         */
        if (not instance_extension_available(
                    VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME)) {
            throw planet::vk::headless_not_available{};
        }
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
        if (not create) { throw planet::vk::headless_not_available{}; }
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


std::unique_ptr<planet::vk::headless> planet::vk::headless::make_if_available() {
    try {
        return std::make_unique<headless>();
    } catch (headless_not_available const &) { return nullptr; }
}


/// ## `planet::vk::headless_not_available`


planet::vk::headless_not_available::headless_not_available(
        std::source_location const loc)
: felspar::stdexcept::runtime_error{
          "vkCreateHeadlessSurfaceEXT is not available -- the VK_EXT_headless_surface extension is not supported by this Vulkan implementation",
          loc} {}
