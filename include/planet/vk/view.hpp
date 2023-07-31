#pragma once


#include <utility>

#include <vulkan/vulkan.h>


namespace planet::vk {


    class device;


    /// ## View
    template<typename Underlying>
    class view {
        Underlying *pd = nullptr;

      public:
        view() noexcept {}
        view(Underlying &d) noexcept : pd{&d} {}

        view(view &&v) : pd{std::exchange(v.pd, nullptr)} {}
        view(view const &) noexcept = default;
        view &operator=(view &&v) noexcept {
            pd = std::exchange(v.pd, nullptr);
            return *this;
        }
        view &operator=(view const &) noexcept = default;

        auto get() const noexcept { return pd->get(); }
        operator Underlying &() noexcept { return *pd; }
        operator Underlying const &() const noexcept { return *pd; }

        Underlying *operator->() noexcept { return pd; }
        Underlying &operator()() noexcept { return *pd; }
    };

    using device_view = view<device>;


}
