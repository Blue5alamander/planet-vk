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
