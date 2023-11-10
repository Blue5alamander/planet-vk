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


        using constrained_type = planet::ui::reflowable::constrained_type;


        pipeline_type &pl;
        texture_type texture;
        vk::colour colour = white;


        void
                draw(renderer &,
                     felspar::source_location const & =
                             felspar::source_location::current());


      private:
        constrained_type do_reflow(constrained_type const &c) override {
            if (texture) {
                return planet::ui::scaling(
                        texture.image.extents(), c, texture.fit);
            } else {
                return {};
            }
        }
        void move_sub_elements(affine::rectangle2d const &) override {}
    };


    template<>
    inline void tx<pipeline::textured, vk::texture>::draw(
            renderer &, felspar::source_location const &) {
        pl.this_frame.draw(texture, position(), colour);
    }
    template<>
    inline void tx<pipeline::textured, vk::texture *>::draw(
            renderer &, felspar::source_location const &) {
        pl.this_frame.draw(*texture, position(), colour);
    }


}
