#pragma once


namespace planet::vk::vertex {


    template<typename Vertex>
    auto binding_description();
    template<typename Vertex>
    auto attribute_description();


    struct coloured;
    struct pos2d;
    struct textured;


}
