#pragma once


#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/texture.hpp>
#include <planet/ui/button.hpp>


namespace planet::vk::engine::ui {


    template<
            typename R,
            typename G = planet::vk::engine::ui::tx<>,
            typename Q = queue::pmc<R>>
    class button final : public planet::ui::button<renderer, R, G, Q> {
        using superclass = planet::ui::button<renderer, R, G, Q>;

      public:
        using value_type = R;
        using graphics_type = G;
        using output_type = Q;

        using superclass::graphic;
        using superclass::move_to;
        using superclass::position;
        using superclass::reflow;

        button(output_type &o, value_type v)
        : superclass{"planet::vk::engine::ui::button", o, std::move(v)} {}
        button(std::string_view const n, output_type &o, value_type v)
        : superclass{n, o, std::move(v)} {}
        button(graphics_type g, output_type &o, value_type v)
        : superclass{
                "planet::vk::engine::ui::button", std::move(g), o,
                std::move(v)} {}
        button(std::string_view const n,
               graphics_type g,
               output_type &o,
               value_type v)
        : superclass{n, std::move(g), o, std::move(v)} {}

        button(button &&b) : superclass{std::move(b)} {
            if (superclass::baseplate) {
                superclass::response.post(superclass::behaviour());
            }
        }

      private:
        void do_draw() override { graphic.draw(); }
    };


}
