#pragma once


#include <felspar/exceptions.hpp>

#include <vulkan/vulkan.h>


namespace planet::vk {


    /// Checks that an API has worked, if it has not then throw an exception
    inline VkResult
            worked(VkResult result,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        if (result != VK_SUCCESS) {
            throw felspar::stdexcept::runtime_error{"Vulkan call failed", loc};
        } else {
            return result;
        }
    }


    /// Patterns for fetching a vector of data from Vulkan
    template<auto Api, typename Array, typename... A>
    inline std::vector<Array> fetch_vector(A &&...arg) {
        std::uint32_t count{};
        Api(arg..., &count, nullptr);
        std::vector<Array> items{count};
        Api(std::forward<A>(arg)..., &count, items.data());
        return items;
    }


    /// A Vulkan handle that is owned by some other handle
    /**
     * Examples of these include the surface, which is owned by an Instance and
     * semaphore, which is owned by a device.
     */
    template<typename O, typename T, void (*D)(O, T, VkAllocationCallbacks const *)>
    class owned_handle final {
        O owner = VK_NULL_HANDLE;
        T handle = VK_NULL_HANDLE;
        VkAllocationCallbacks const *allocator = nullptr;

        owned_handle(O o, T h, VkAllocationCallbacks const *a) noexcept
        : owner{o}, handle{h}, allocator{a} {}

        void reset() noexcept {
            if (owner and handle) { D(owner, handle, allocator); }
        }

      public:
        owned_handle() noexcept {}
        owned_handle(owned_handle &&h) noexcept
        : owner{std::exchange(h.owner, VK_NULL_HANDLE)},
          handle{std::exchange(h.handle, VK_NULL_HANDLE)},
          allocator{std::exchange(h.allocator, nullptr)} {}
        ~owned_handle() noexcept { reset(); }

        static owned_handle bind(O o, T h) noexcept { return {o, h, nullptr}; }
    };


}
