#pragma once


#include <planet/affine/matrix3d.hpp>
#include <planet/vk/ubo/coherent.hpp>

#include <cstring>


namespace planet::vk::ubo {


    /// A UBO for handling screen space
    struct coordinate_space {
        template<std::size_t Frames>
        using ubo_type = coherent<coordinate_space, Frames>;


        affine::matrix3d world = {};
        affine::matrix3d screen = {};
        affine::matrix3d perspective = {};


        void copy_to_gpu_memory(std::byte *memory) const {
            std::memcpy(memory, this, sizeof(coordinate_space));
        }
    };


}
