#include <planet/vk/device.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/swap_chain.hpp>


VkExtent2D planet::vk::swap_chain::extents(
        vk::device const &device, affine::extents2d const ex) {
    if (device.instance.surface.capabilities.currentExtent.width
        != std::numeric_limits<std::uint32_t>::max()) {
        return device.instance.surface.capabilities.currentExtent;
    } else {
        return VkExtent2D{
                std::clamp<std::uint32_t>(
                        ex.width,
                        device.instance.surface.capabilities.minImageExtent.width,
                        device.instance.surface.capabilities.maxImageExtent
                                .width),
                std::clamp<std::uint32_t>(
                        ex.height,
                        device.instance.surface.capabilities.minImageExtent
                                .height,
                        device.instance.surface.capabilities.maxImageExtent
                                .height)};
    }
}
