#pragma once


#include <planet/telemetry/counter.hpp>
#include <planet/vk/helpers.hpp>

#include <utility>

#include <vulkan/vulkan.h>


namespace planet::vk {


    /// ## A Vulkan handle that is owned by some other handle
    /**
     * Examples of these include the surface, which is owned by an Instance and
     * semaphore, which is owned by a device.
     */
    template<typename O, typename T, void (*D)(O, T, VkAllocationCallbacks const *)>
    class owned_handle final {
        O owner_handle = VK_NULL_HANDLE;
        T handle = VK_NULL_HANDLE;
        VkAllocationCallbacks const *allocator = nullptr;
        telemetry::counter *dealloc_counter = nullptr;


        owned_handle(
                O o,
                T h,
                VkAllocationCallbacks const *a,
                telemetry::counter *d) noexcept
        : owner_handle{o}, handle{h}, allocator{a}, dealloc_counter{d} {}


      public:
        owned_handle() noexcept {}
        owned_handle(owned_handle const &) = delete;
        owned_handle(owned_handle &&h) noexcept
        : owner_handle{std::exchange(h.owner_handle, VK_NULL_HANDLE)},
          handle{std::exchange(h.handle, VK_NULL_HANDLE)},
          allocator{std::exchange(h.allocator, nullptr)},
          dealloc_counter{std::exchange(h.dealloc_counter, nullptr)} {}
        ~owned_handle() noexcept { reset(); }

        owned_handle &operator=(owned_handle const &) = delete;
        owned_handle &operator=(owned_handle &&h) noexcept {
            reset();
            owner_handle = std::exchange(h.owner_handle, VK_NULL_HANDLE);
            handle = std::exchange(h.handle, VK_NULL_HANDLE);
            allocator = std::exchange(h.allocator, nullptr);
            dealloc_counter = std::exchange(h.dealloc_counter, nullptr);
            return *this;
        }


        T get() const noexcept { return handle; }
        T const *address() const noexcept { return &handle; }
        O owner() const noexcept { return owner_handle; }


        /// ### Remove the current content (if any)
        void reset() noexcept {
            if (owner_handle and handle) {
                D(std::exchange(owner_handle, VK_NULL_HANDLE),
                  std::exchange(handle, VK_NULL_HANDLE),
                  std::exchange(allocator, nullptr));
                if (dealloc_counter) { ++*dealloc_counter; }
            }
        }


        template<auto C, typename I>
        void
                create(O h,
                       I const &info,
                       VkAllocationCallbacks const *alloc = nullptr,
                       telemetry::counter *d = nullptr) {
            reset();
            owner_handle = h;
            allocator = alloc;
            dealloc_counter = d;
            worked(C(h, &info, alloc, &handle));
        }
        static owned_handle
                bind(O o, T h, telemetry::counter *d = nullptr) noexcept {
            return {o, h, nullptr, d};
        }
    };


    /// ## Shorter names for common owners of handles
    template<typename T, void (*D)(VkDevice, T, VkAllocationCallbacks const *)>
    using device_handle = owned_handle<VkDevice, T, D>;


}
