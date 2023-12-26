#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine::ui {


    /// ### Draws a texture in the screen space coordinates for layout
    template<template Pipeline>
    struct label final : public planet::ui::reflowable {
        label(vk::pipeline &);
        label(vk::pipeline &, std::string_view);
        label(vk::pipeline &, vk::texture);
        label(vk::pipeline &, std::string_view, vk::texture);
        label(vk::pipeline &, vk::texture, vk::colour const &);
        label(vk::pipeline &, std::string_view, vk::texture, vk::colour const &);


        using constrained_type = planet::ui::reflowable::constrained_type;


        Pipeline &pl;
        vk::texture texture;
        vk::colour colour = white;


        void draw();


      private:
        constrained_type do_reflow(constrained_type const &) override;
        void move_sub_elements(affine::rectangle2d const &) override;
    };


}
