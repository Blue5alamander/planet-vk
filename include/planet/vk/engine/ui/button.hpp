#pragma once


#include <planet/queue/forward.hpp>
#include <planet/ui/button.hpp>
#include <planet/vk/engine/renderer.hpp>
#include <planet/vk/texture.hpp>


namespace planet::vk::engine::ui {


    template<typename R, planet::ui::drawable G, typename Q>
    class button final : public planet::ui::button<R, G, Q> {
        using superclass = planet::ui::button<R, G, Q>;


      public:
        using value_type = R;
        using graphics_type = G;
        using output_type = Q;

        using superclass::enable;
        using superclass::graphic;
        using superclass::move_to;
        using superclass::position;
        using superclass::reflow;
        using superclass::visible;


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

    template<planet::ui::drawable G, typename Q>
    class button<void, G, Q> final : public planet::ui::button<void, G, Q> {
        using superclass = planet::ui::button<void, G, Q>;


      public:
        using value_type = void;
        using graphics_type = G;
        using output_type = Q;

        using superclass::graphic;
        using superclass::move_to;
        using superclass::position;
        using superclass::reflow;


        button(output_type &o)
        : superclass{"planet::vk::engine::ui::button", o} {}
        button(std::string_view const n, output_type &o) : superclass{n, o} {}
        button(graphics_type g, output_type &o)
        : superclass{"planet::vk::engine::ui::button", std::move(g), o} {}
        button(std::string_view const n, graphics_type g, output_type &o)
        : superclass{n, std::move(g), o} {}

        button(button &&b) : superclass{std::move(b)} {
            if (superclass::baseplate) {
                superclass::response.post(superclass::behaviour());
            }
        }


      private:
        void do_draw() override { graphic.draw(); }
    };


    template<planet::ui::drawable G, typename R>
    button(G, queue::pmc<R>, R) -> button<R, G, queue::pmc<R>>;

    template<planet::ui::drawable G, typename R>
    button(G, queue::psc<R>, R) -> button<R, G, queue::pmc<R>>;
    template<planet::ui::drawable G>
    button(G, queue::psc<void>) -> button<void, G, queue::psc<void>>;

    template<planet::ui::drawable G, typename R>
    button(G, felspar::coro::future<R>, R)
            -> button<R, G, felspar::coro::future<R>>;


    template<planet::ui::drawable G, typename R>
    button(std::string_view, G, queue::pmc<R>, R)
            -> button<R, G, queue::pmc<R>>;

    template<planet::ui::drawable G, typename R>
    button(std::string_view, G, queue::psc<R>, R)
            -> button<R, G, queue::pmc<R>>;
    template<planet::ui::drawable G>
    button(std::string_view, G, queue::psc<void>)
            -> button<void, G, queue::psc<void>>;

    template<planet::ui::drawable G, typename R>
    button(std::string_view, G, felspar::coro::future<R>, R)
            -> button<R, G, felspar::coro::future<R>>;


}
