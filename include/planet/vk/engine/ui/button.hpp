#pragma once


#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/texture.hpp>
#include <planet/ui/button.hpp>


namespace planet::vk::engine::ui {


    template<typename R, typename G = planet::vk::engine::ui::tx<>>
    class button final : public planet::ui::button<renderer, R, G> {
        using superclass = planet::ui::button<renderer, R, G>;

      public:
        using value_type = R;
        using graphics_type = G;

        using superclass::graphic;
        using superclass::move_to;
        using superclass::position;
        using superclass::reflow;

        button(felspar::coro::bus<value_type> &o, value_type v)
        : superclass{"planet::vk::engine::ui::button", o, std::move(v)} {}
        button(std::string_view const n,
               felspar::coro::bus<value_type> &o,
               value_type v)
        : superclass{n, o, std::move(v)} {}
        button(graphics_type g, felspar::coro::bus<value_type> &o, value_type v)
        : superclass{
                "planet::vk::engine::ui::button", std::move(g), o,
                std::move(v)} {}
        button(std::string_view const n,
               graphics_type g,
               felspar::coro::bus<value_type> &o,
               value_type v)
        : superclass{n, std::move(g), o, std::move(v)} {}

      private:
        void do_draw(renderer &r) override { graphic.draw(r); }
    };


}
