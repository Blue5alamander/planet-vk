#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine::ui {


    /// ### Draws a texture in the screen space coordinates for layout
    template<typename Pipeline>
    struct tx final : public planet::ui::reflowable {
        tx(Pipeline &p)
        : reflowable{"planet::vk::engine::ui::on_screen"}, pl{p} {}
        tx(Pipeline &p, std::string_view const n) : reflowable{n}, pl{p} {}
        tx(Pipeline &p, vk::texture tx)
        : reflowable{"planet::vk::engine::ui::on_screen"},
          pl{p},
          texture{std::move(tx)} {}
        tx(Pipeline &p, std::string_view const n, vk::texture tx)
        : reflowable{n}, pl{p}, texture{std::move(tx)} {}
        tx(Pipeline &p, vk::texture tx, vk::colour const &c)
        : reflowable{"planet::vk::engine::ui::on_screen"},
          pl{p} texture{std::move(tx)},
          colour{c} {}
        tx(Pipeline &p,
           std::string_view const n,
           vk::texture tx,
           vk::colour const &c)
        : reflowable{n}, pl{p}, texture{std::move(tx)}, colour{c} {}

        using constrained_type = planet::ui::reflowable::constrained_type;

        Pipeline &pl;
        vk::texture texture;
        vk::colour colour = white;

        void draw(renderer &) { pl.draw(texture, position(), colour); }

      private:
        constrained_type do_reflow(constrained_type const &c) override {
            return planet::ui::scaling(texture.image.extents(), c, texture.fit);
        }
        void move_sub_elements(affine::rectangle2d const &) override {}
    };


}
