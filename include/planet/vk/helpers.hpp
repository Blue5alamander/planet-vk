#pragma once


#include <felspar/exceptions.hpp>

#include <vulkan/vulkan.h>


namespace planet::vk {


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


}
