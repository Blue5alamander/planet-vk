#pragma once


#include <planet/vk/engine2d/renderer.hpp>
#include <planet/vk/engine2d/ui/on_screen.hpp>
#include <planet/vk/texture.hpp>
#include <planet/ui/button.hpp>


namespace planet::vk::engine2d::ui {


    template<typename R, typename G = on_screen>
    class button final : public planet::ui::button<renderer, R, G> {
        using superclass = planet::ui::button<renderer, R, G>;

      public:
        using renderer_type = R;
        using graphics_type = G;

        using superclass::graphic;
        using superclass::position;
        using superclass::reflow;

        button(felspar::coro::bus<R> &o, R v)
        : superclass{"planet::vk::engine2d::ui::button", o, std::move(v)} {}
        button(G g, felspar::coro::bus<R> &o, R v)
        : superclass{
                "planet::vk::engine2d::ui::button", std::move(g), o,
                std::move(v)} {}

      private:
        void do_draw(renderer &r) override { graphic.draw(r); }
    };


}
