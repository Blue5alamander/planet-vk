#pragma once


#include <planet/ui/reflowable.hpp>
#include <planet/vk/engine/forward.hpp>


namespace planet::vk::engine::ui {


    /// ## Draw wrapper that automatically handles one instance per frame for us
    template<typename T>
    struct [[deprecated("Use the `autoupdater` instead")]] per_frame final :
    public planet::ui::reflowable {
        per_frame() : reflowable{"planet::vk::engine::ui::per_frame"} {}
        template<typename... Args>
        per_frame(Args &&...args)
        : reflowable{"planet::vk::engine::ui::per_frame"},
          frame{T{args...}, T{args...}, T{args...}} {}
        template<typename... Args>
        per_frame(std::string_view const n, Args &&...args)
        : reflowable{n}, frame{T{args...}, T{args...}, T{args...}} {}

        using constrained_type = planet::ui::reflowable::constrained_type;


        std::array<T, max_frames_in_flight> frame;
        std::size_t current_frame = {};


        void draw() {
            frame[current_frame].draw();
            current_frame = (current_frame + 1) % max_frames_in_flight;
        }


      private:
        constrained_type do_reflow(
                reflow_parameters const &p,
                constrained_type const &c) override {
            std::array<constrained_type, max_frames_in_flight> const cs{
                    frame[0].reflow(p, c), frame[1].reflow(p, c),
                    frame[2].reflow(p, c)};

            auto max = [](auto a, auto b, auto c) {
                return std::max(a, std::max(b, c));
            };
            auto min = [](auto a, auto b, auto c) {
                return std::min(a, std::min(b, c));
            };

            return {{max(cs[0].width.min(), cs[1].width.min(),
                         cs[2].width.min()),
                     max(cs[0].width.value(), cs[1].width.value(),
                         cs[2].width.value()),
                     min(cs[0].width.max(), cs[1].width.max(),
                         cs[2].width.max())},
                    {max(cs[0].height.min(), cs[1].height.min(),
                         cs[2].height.min()),
                     max(cs[0].height.value(), cs[1].height.value(),
                         cs[2].height.value()),
                     min(cs[0].height.max(), cs[1].height.max(),
                         cs[2].height.max())}};
        }
        affine::rectangle2d move_sub_elements(
                reflow_parameters const &p,
                affine::rectangle2d const &r) override {
            for (auto &f : frame) { f.move_to(p, r); }
            /// TODO Should be union of the frames
            return r;
        }
    };


}
