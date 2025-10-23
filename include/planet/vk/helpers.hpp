#pragma once


#include <planet/vk/forward.hpp>

#include <felspar/test/source.hpp>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>


namespace planet::vk {


    /**
     * ## Checks that an API has worked
     *
     * If it has not then throw an exception.
     */
    namespace detail {
        std::string error(VkResult);
        [[noreturn]] void
                throw_result_error(VkResult, felspar::source_location const &);
    }
    inline VkResult
            worked(VkResult const result,
                   felspar::source_location const &loc =
                           felspar::source_location::current()) {
        if (result != VK_SUCCESS) {
            detail::throw_result_error(result, loc);
        } else {
            return result;
        }
    }


    /// ## Fetching an array of data
    template<auto Api, typename Array, typename... A>
    inline std::vector<Array> fetch_vector(A &&...arg) {
        std::uint32_t count{};
        Api(arg..., &count, nullptr);
        std::vector<Array> items;
        items.resize(count);
        Api(std::forward<A>(arg)..., &count, items.data());
        return items;
    }


}
