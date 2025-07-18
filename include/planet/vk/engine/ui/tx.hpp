#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/engine/textured.pipeline.hpp>
#include <planet/vk/texture.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine::ui {


    /// ## Draws a texture in the screen space coordinates for layout
    template<typename Pipeline, typename Texture>
    struct tx final : public planet::ui::reflowable {
        using constrained_type = planet::ui::reflowable::constrained_type;
        using pipeline_type = Pipeline;
        using texture_type = Texture;


        tx(pipeline_type &p)
        : reflowable{"planet::vk::engine::ui::on_screen"}, pl{p} {}
        tx(pipeline_type &p, std::string_view const n) : reflowable{n}, pl{p} {}
        tx(pipeline_type &p, texture_type tx)
        : reflowable{"planet::vk::engine::ui::on_screen"},
          pl{p},
          texture{std::move(tx)} {}
        tx(pipeline_type &p, std::string_view const n, texture_type tx)
        : reflowable{n}, pl{p}, texture{std::move(tx)} {}
        tx(pipeline_type &p, texture_type tx, vk::colour const &c)
        : reflowable{"planet::vk::engine::ui::on_screen"},
          pl{p},
          texture{std::move(tx)},
          colour{c} {}
        tx(pipeline_type &p,
           std::string_view const n,
           texture_type tx,
           vk::colour const &c)
        : reflowable{n}, pl{p}, texture{std::move(tx)}, colour{c} {}


        pipeline_type &pl;
        texture_type texture;
        vk::colour colour = colour::white;


        void draw();


      private:
        constrained_type do_reflow(
                reflow_parameters const &p, constrained_type const &c) override;
        affine::rectangle2d move_sub_elements(
                reflow_parameters const &,
                affine::rectangle2d const &r) override {
            return {r.top_left, constraints().extents()};
        }
    };


    template<>
    inline void tx<pipeline::textured, vk::texture>::draw() {
        if (texture) { pl.draw(texture, position(), colour); }
    }
    template<>
    inline void tx<pipeline::textured, vk::texture *>::draw() {
        if (texture and *texture) { pl.draw(*texture, position(), colour); }
    }
    template<>
    inline void tx<pipeline::textured, sub_texture>::draw() {
        if (texture.first) { pl.draw(texture, position(), colour); }
    }


    template<>
    inline auto tx<pipeline::textured, vk::texture>::do_reflow(
            reflow_parameters const &, constrained_type const &c)
            -> constrained_type {
        if (texture) {
            auto const r = planet::ui::scaling(
                    texture.image.extents(), c, texture.fit);
            return r;
        } else {
            return {};
        }
    }
    template<>
    inline auto tx<pipeline::textured, vk::texture *>::do_reflow(
            reflow_parameters const &, constrained_type const &c)
            -> constrained_type {
        if (texture and *texture) {
            return planet::ui::scaling(
                    texture->image.extents(), c, texture->fit);
        } else {
            return {};
        }
    }
    template<>
    inline auto tx<pipeline::textured, sub_texture>::do_reflow(
            reflow_parameters const &, constrained_type const &c)
            -> constrained_type {
        if (texture.first) {
            auto const width =
                    texture.second.extents.width * texture.first.image.width;
            auto const height =
                    texture.second.extents.height * texture.first.image.height;
            return planet::ui::scaling({width, height}, c, texture.first.fit);
        } else {
            return {};
        }
    }


}
