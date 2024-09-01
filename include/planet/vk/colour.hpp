#pragma once


#include <vulkan/vulkan.h>


namespace planet::vk {


    /// ## Linear colour format
    /**
     * The colours described by this time are intended for use as a linear
     * colour space, so colour manipulation is a simple matter of multiplying
     * the values. This should be the default when using the [surface's
     * `best_format`](../../../src/render.engine.cpp)
     */
    struct colour {
        float r{}, g{}, b{}, a = {1};


        static colour const black, white;


        operator VkClearValue() const noexcept {
            VkClearValue c;
            c.color.float32[0] = r;
            c.color.float32[1] = g;
            c.color.float32[2] = b;
            c.color.float32[3] = a;
            return c;
        }


        /// ### Multiple the RGB values
        friend colour operator*(colour const &c, float const m) noexcept {
            return {c.r * m, c.g * m, c.b * m, c.a};
        }
    };

    inline constexpr colour colour::black{};
    inline constexpr colour colour::white{1.0f, 1.0f, 1.0f};


}
