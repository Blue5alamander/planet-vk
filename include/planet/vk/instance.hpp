#pragma once


#include <planet/vk/surface.hpp>
#include <planet/vk/physical_device.hpp>

#include <span>
#include <vector>


namespace planet::vk {


    struct extensions;


    /// Return a filled in structure that can be used to create the instance
    VkApplicationInfo application_info();


    /// The Vulkan instance
    class instance final {
        struct instance_handle {
            VkInstance h;
            ~instance_handle();
        } handle;

        std::vector<physical_device> pdevices;
        physical_device const *gpu_in_use = nullptr;

      public:
        static VkInstanceCreateInfo
                info(extensions const &, VkApplicationInfo const &);
        instance(
                VkInstanceCreateInfo const &,
                std::function<VkSurfaceKHR(VkInstance)>);

        instance(instance const &) = delete;
        instance(instance &&) = delete;
        instance &operator=(instance const &) = delete;
        instance &operator=(instance &&i) = delete;

        VkInstance get() const noexcept { return handle.h; }

        vk::surface surface;

        std::span<physical_device const> physical_devices() const {
            return pdevices;
        }
        /// The GPU that is currently chosen for use
        physical_device const &gpu() const noexcept { return *gpu_in_use; }
        physical_device const &use_gpu(physical_device const &d) noexcept {
            surface.refresh_characteristics(d);
            gpu_in_use = &d;
            return *gpu_in_use;
        }
    };


}
