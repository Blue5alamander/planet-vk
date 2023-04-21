#pragma once


#include <planet/vk/engine2d/renderer.hpp>
#include <planet/vk/texture.hpp>
#include <planet/ui/widget.hpp>


namespace planet::vk::engine2d::ui {


    template<typename R, typename G = pipeline::on_screen>
    class button : public planet::ui::widget<renderer> {
        R press_value;
        felspar::coro::bus<R> &output_to;
        std::optional<affine::rectangle2d> position;

      public:
        using renderer_type = R;
        using graphics_type = G;

        button(felspar::coro::bus<R> &o, R v)
        : press_value{std::move(v)}, output_to{o} {}
        button(G g, felspar::coro::bus<R> &o, R v)
        : press_value{std::move(v)}, output_to{o}, graphic{std::move(g)} {}

        affine::extents2d extents(affine::extents2d const &ex) {
            return graphic.extents(ex);
        }

        G graphic;

      private:
        void do_draw_within(
                renderer &r, affine::rectangle2d const outer) override {
            graphic.draw_within(r, outer);
            affine::rectangle2d const p{outer.top_left, graphic.extents()};
            if (p != position) {
                panel.move_to(p);
                position = p;
            }
        }

        felspar::coro::task<void> behaviour() override {
            while (true) {
                co_await panel.clicks.next();
                output_to.push(press_value);
            }
        }
    };


}
