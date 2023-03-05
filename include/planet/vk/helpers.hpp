#pragma once


#include <felspar/exceptions.hpp>

#include <vulkan/vulkan.h>


namespace planet::vk {


    class device;


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
        O owner_handle = VK_NULL_HANDLE;
        T handle = VK_NULL_HANDLE;
        VkAllocationCallbacks const *allocator = nullptr;

        owned_handle(O o, T h, VkAllocationCallbacks const *a) noexcept
        : owner_handle{o}, handle{h}, allocator{a} {}

      public:
        owned_handle() noexcept {}
        owned_handle(owned_handle const &) = delete;
        owned_handle(owned_handle &&h) noexcept
        : owner_handle{std::exchange(h.owner_handle, VK_NULL_HANDLE)},
          handle{std::exchange(h.handle, VK_NULL_HANDLE)},
          allocator{std::exchange(h.allocator, nullptr)} {}
        ~owned_handle() noexcept { reset(); }

        owned_handle &operator=(owned_handle const &) = delete;
        owned_handle &operator=(owned_handle &&h) {
            reset();
            owner_handle = std::exchange(h.owner_handle, VK_NULL_HANDLE);
            handle = std::exchange(h.handle, VK_NULL_HANDLE);
            allocator = std::exchange(h.allocator, nullptr);
            return *this;
        }

        T get() const noexcept { return handle; }
        T const *address() const noexcept { return &handle; }
        O owner() const noexcept { return owner_handle; }

        /// Remove the current content (if any)
        void reset() noexcept {
            if (owner_handle and handle) {
                D(std::exchange(owner_handle, VK_NULL_HANDLE),
                  std::exchange(handle, VK_NULL_HANDLE),
                  std::exchange(allocator, nullptr));
            }
        }

        template<auto C, typename I>
        void
                create(O h,
                       I const &info,
                       VkAllocationCallbacks const *alloc = nullptr) {
            reset();
            owner_handle = h;
            allocator = alloc;
            worked(C(h, &info, alloc, &handle));
        }
        static owned_handle bind(O o, T h) noexcept { return {o, h, nullptr}; }
    };


    /// Shorter names for common owners of handles
    template<typename T, void (*D)(VkDevice, T, VkAllocationCallbacks const *)>
    using device_handle = owned_handle<VkDevice, T, D>;


    /// ## View
    template<typename Underlying>
    class view {
        Underlying *pd = nullptr;

      public:
        view() {}
        view(Underlying &d) : pd{&d} {}

        view(view &&v) : pd{std::exchange(v.pd, nullptr)} {}
        view(view const &) = default;
        view &operator=(view &&v) {
            pd = std::exchange(v.pd, nullptr);
            return *this;
        }
        view &operator=(view const &) = default;

        auto get() const { return pd->get(); }
        operator Underlying &() { return *pd; }
        operator Underlying const &() const { return *pd; }

        Underlying &operator()() { return *pd; }
    };

    using device_view = view<device>;


}
