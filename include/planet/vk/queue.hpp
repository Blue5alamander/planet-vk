#pragma once


#include <planet/vk/forward.hpp>


namespace planet::vk {


    /// ## Vulkan Queue
    /**
     * The surface will only create these for transfer queues that are not used
     * for graphics or presentation.
     *
     * The Vulkan queues are owned by the instance and we only see the index
     * into some other data structure.
     */
    class queue final {
        friend class surface;

        vk::surface *surface = nullptr;
        std::uint32_t index = {};


        queue(vk::surface *, std::uint32_t);


      public:
        /// ### Construction
        queue();
        queue(queue const &) = delete;
        queue(queue &&);
        ~queue();

        queue &operator=(queue const &) = delete;
        queue &operator=(queue &&) = delete;


        /// ### Conversions
        explicit operator bool() const noexcept;

        /// #### Fetch the index
        /**
         * This will throw if the `queue` doesn't contain a valid index.
         */
        std::uint32_t get() const;
    };


}
