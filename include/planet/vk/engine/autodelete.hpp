#pragma once


#include <planet/vk/engine/renderer.hpp>

#include <felspar/coro/eager.hpp>
#include <felspar/memory/any_buffer.hpp>


namespace planet::vk::engine {


    /// ## Auto-delete resources after waiting a full frame cycle
    template<typename C = std::vector<felspar::memory::any_buffer<512>>>
    class autodelete final {
        using collection_type = C;


        /// ### Hold management
        engine::renderer &renderer;
        std::array<collection_type, max_frames_in_flight> holds;
        collection_type new_holds;


      public:
        autodelete(engine::renderer &r)
        : renderer{r}, deleter{delete_holds()} {}

        autodelete(autodelete const &) = delete;
        autodelete(autodelete &&ad)
        : renderer{ad.renderer}, deleter{delete_holds()} {
            assert_empty(ad);
            ad.deleter.destroy();
        }

        autodelete &operator=(autodelete const &) = delete;
        autodelete &operator=(autodelete &&) = delete;


        /// ### Autodelete an asset
        template<typename W>
        void manage(W w)
            requires requires { new_holds.emplace_back(std::move(w)); }
        {
            new_holds.emplace_back(std::move(w));
        }
        template<typename W>
        void manage(W w)
            requires requires { new_holds.emplace(std::move(w)); }
        {
            new_holds.emplace(std::move(w));
        }
        template<typename W>
        void manage(W &o, W w) {
            manage(std::exchange(o, std::move(w)));
        }


      private:
        felspar::coro::eager<>::task_type delete_holds() {
            while (true) {
                auto const frame_index =
                        co_await renderer.next_frame_prestart();
                std::swap(holds[frame_index], new_holds);
                reset_new_hold();
            }
        }
        felspar::coro::eager<> deleter;


        void reset_new_hold()
            requires requires { new_holds.clear(); }
        {
            new_holds.clear();
        }
        void reset_new_hold()
            requires requires { new_holds.reset(); }
        {
            new_holds.reset();
        }

        static void assert_empty(autodelete &ad)
            requires requires { ad.new_holds.empty(); }
        {
            if (not ad.new_holds.empty() or not ad.holds[0].empty()
                or not ad.holds[1].empty() or not ad.holds[2].empty()) {
                throw felspar::stdexcept::logic_error{
                        "The autodeleter being moved from must be empty to be "
                        "moved"};
            }
        }
        static void assert_empty(autodelete &ad)
            requires requires { ad.new_holds.has_value(); }
        {
            if (ad.new_holds.has_value() or ad.holds[0].has_value()
                or ad.holds[1].has_value() or ad.holds[2].has_value()) {
                throw felspar::stdexcept::logic_error{
                        "The autodeleter being moved from must be empty to be "
                        "moved"};
            }
        }
    };


}
