#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine::ui {


    /// ## Draw wrapper that automatically handles one instance per frame for us
    template<typename T>
    struct per_frame final : public planet::ui::reflowable {
        per_frame() : reflowable{"planet::vk::engine::ui::per_frame"} {}
        template<typename... Args>
        per_frame(Args &&...args)
        : reflowable{"planet::vk::engine::ui::per_frame"},
          frame{T{args...}, T{args...}, T{args...}} {}

        using constrained_type = planet::ui::reflowable::constrained_type;

        std::array<T, max_frames_in_flight> frame;

        void
                draw(renderer &r,
                     felspar::source_location const &loc =
                             felspar::source_location::current()) {
            frame[r.current_frame].draw(r, loc);
        }

      private:
        constrained_type do_reflow(constrained_type const &c) override {
            std::array<constrained_type, max_frames_in_flight> const cs{
                    frame[0].reflow(c), frame[1].reflow(c), frame[2].reflow(c)};

            /// TODO Include all the frames in the constraint calculation
            return {{std::max(cs[0].width.min(), cs[1].width.min()),
                     std::max(cs[0].width.value(), cs[1].width.value()),
                     std::min(cs[0].width.max(), cs[1].width.max())},
                    {std::max(cs[0].height.min(), cs[1].height.min()),
                     std::max(cs[0].height.value(), cs[1].height.value()),
                     std::min(cs[0].height.max(), cs[1].height.max())}};
        }
        void move_sub_elements(affine::rectangle2d const &r) override {
            for (auto &f : frame) { f.move_to(r); }
        }
    };


}
