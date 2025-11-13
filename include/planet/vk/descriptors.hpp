#pragma once


#include <planet/vk/owned_handle.hpp>
#include <planet/vk/view.hpp>

#include <span>


namespace planet::vk {


    class device;


    /// ## Descriptor pool
    class descriptor_pool final {
        using handle_type =
                device_handle<VkDescriptorPool, vkDestroyDescriptorPool>;
        handle_type handle;

      public:
        /// ### General purpose creation of a pool for the described sizes
        descriptor_pool(
                vk::device &,
                std::span<VkDescriptorPoolSize const>,
                std::uint32_t max_sets);
        /// ### Create a pool for a single type with the requested size
        descriptor_pool(vk::device &, VkDescriptorType, std::uint32_t count);

        device_view device;
        VkDescriptorPool get() const noexcept { return handle.get(); }
    };


    /// ## Descriptor set layout
    class descriptor_set_layout final {
        using handle_type =
                device_handle<VkDescriptorSetLayout, vkDestroyDescriptorSetLayout>;
        handle_type handle;

      public:
        /// ### Construction
        /// #### General purpose set layout creation
        descriptor_set_layout(
                vk::device &, VkDescriptorSetLayoutCreateInfo const &);
        /// #### Create a set layout for only a single binding
        descriptor_set_layout(
                vk::device &, VkDescriptorSetLayoutBinding const &);
        /// #### For multiple bindings
        descriptor_set_layout(
                vk::device &, std::span<VkDescriptorSetLayoutBinding const>);
        template<std::size_t N>
        descriptor_set_layout(
                vk::device &d,
                std::array<VkDescriptorSetLayoutBinding, N> const &a)
        : descriptor_set_layout{d, std::span{a}} {}
        /// #### For a uniform buffer object
        static descriptor_set_layout for_uniform_buffer_object(vk::device &);


        device_view device;

        VkDescriptorSetLayout get() const noexcept { return handle.get(); }
        VkDescriptorSetLayout const *address() const noexcept {
            return handle.address();
        }
    };


    /// ## Descriptor sets
    class descriptor_sets final {
        std::vector<VkDescriptorSet> sets = {};

      public:
        /// ### Create sets for a single layout, but multiple frames
        descriptor_sets() {}
        descriptor_sets(
                descriptor_pool const &,
                descriptor_set_layout const &,
                std::uint32_t count,
                std::source_location const & = std::source_location::current());


        descriptor_sets &operator=(descriptor_sets &&) = default;


        /// ### Access to individual sets
        auto &operator[](std::uint32_t const index) { return sets.at(index); }
        auto const &operator[](std::uint32_t const index) const {
            return sets.at(index);
        }

        std::size_t size() const noexcept { return sets.size(); }
        VkDescriptorSet *data() noexcept { return sets.data(); }
        VkDescriptorSet const *data() const noexcept { return sets.data(); }
    };


}
