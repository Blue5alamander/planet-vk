#pragma once


#include <planet/vk/helpers.hpp>
#include <planet/vk/physical_device.hpp>

#include <span>
#include <vector>


namespace planet::vk {


    struct extensions;


    /// Return a filled in structure that can be used to create the instance
    VkApplicationInfo application_info();


    /// The Vulkan instance
    class instance final {
        VkInstance handle = VK_NULL_HANDLE;
        void reset() noexcept;

        std::vector<physical_device> physical_devices;
        physical_device const *gpu_in_use = nullptr;

      public:
        static VkInstanceCreateInfo
                info(extensions const &, VkApplicationInfo const &);
        instance() noexcept {}
        explicit instance(VkInstanceCreateInfo const &);
        ~instance() { reset(); }

        instance &operator=(instance &&i) noexcept {
            reset();
            handle = std::exchange(i.handle, VK_NULL_HANDLE);
            physical_devices = std::move(i.physical_devices);
            gpu_in_use = std::exchange(i.gpu_in_use, nullptr);
            return *this;
        }

        VkInstance get() noexcept { return handle; }

        std::span<physical_device const> devices() const {
            return physical_devices;
        }
        physical_device const &gpu() const noexcept { return *gpu_in_use; }
        physical_device const &use_gpu(physical_device const &d) noexcept {
            gpu_in_use = &d;
            return *gpu_in_use;
        }
    };


}
