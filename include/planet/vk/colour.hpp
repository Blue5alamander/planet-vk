#pragma once


#include <vulkan/vulkan.h>


namespace planet::vk {


    struct colour {
        float r{}, g{}, b{}, a = {1};

        operator VkClearValue() const noexcept {
            VkClearValue c;
            c.color.float32[0] = r;
            c.color.float32[1] = g;
            c.color.float32[2] = b;
            c.color.float32[3] = a;
            return c;
        }
    };


}
