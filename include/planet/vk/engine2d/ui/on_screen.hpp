#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/colour.hpp>
#include <planet/vk/engine2d/forward.hpp>


namespace planet::vk::engine2d::ui {


    /// ### Draws a texture in the screen space coordinates for layout
    struct on_screen final : public planet::ui::reflowable {
        on_screen();
        on_screen(vk::texture);
        on_screen(vk::texture, vk::colour const &);

        using constrained_type = planet::ui::reflowable::constrained_type;

        vk::texture texture;
        vk::colour colour = white;

        void draw(renderer &);

      private:
        constrained_type do_reflow(constrained_type const &) override;
        void move_sub_elements(affine::rectangle2d const &) override;
    };


}
