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


    /// Patterns for fetching a vector of data from Vulkan
    template<auto Api, typename Array, typename... A>
    inline std::vector<Array> fetch_vector(A &&...arg) {
        std::uint32_t count{};
        Api(arg..., &count, nullptr);
        std::vector<Array> items{count};
        Api(std::forward<A>(arg)..., &count, items.data());
        return items;
    }


}
