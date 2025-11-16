#pragma once


#include <planet/colour.hpp>

#include <vulkan/vulkan.h>


namespace planet::vk {


    inline auto as_VkClearValue(colour const o) {
        VkClearValue c;
        c.color.float32[0] = o.r;
        c.color.float32[1] = o.g;
        c.color.float32[2] = o.b;
        c.color.float32[3] = o.a;
        return c;
    }


}
