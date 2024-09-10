#pragma once


#include <planet/vk/engine/autodelete.hpp>


namespace planet::vk::engine {


    /// ## Automatically update a widget
    template<typename V, std::invocable<> G>
    struct autoupdater final {
        using generator_type = G;
        using watching_type = std::decay_t<V>;
        using graphic_type =
                std::decay_t<decltype(std::declval<generator_type>()())>;

        using constrained_type = typename graphic_type::constrained_type;
        using reflow_parameters = typename graphic_type::reflow_parameters;


        engine::renderer &renderer;
        watching_type const &watching;
        generator_type generate;
        watching_type last_value = watching;
        felspar::memory::holding_pen<graphic_type> graphic{};
        autodelete<felspar::memory::holding_pen<graphic_type>> autodeleter{
                renderer};


        bool needs_reflow() {
            if (watching != last_value) {
                last_value = watching;
                if (graphic) {
                    autodeleter.manage(std::move(*graphic));
                    graphic.reset();
                }
                return true;
            } else {
                return false;
            }
        }

        void draw() { graphic->draw(); }


        constrained_type
                reflow(reflow_parameters const &p, constrained_type const &c) {
            if (not graphic) { graphic.assign(generate()); }
            return graphic->reflow(p, c);
        }
        auto move_to(affine::rectangle2d const &r) {
            return graphic->move_to(r);
        }
        auto constraints() const { return graphic->constraints(); }
    };

    template<std::invocable<> V, std::invocable<> G>
    struct autoupdater<V, G> final {
        using generator_type = G;
        using watching_lambda_type = V;
        using watching_type = decltype(std::declval<watching_lambda_type>()());
        using graphic_type =
                std::decay_t<decltype(std::declval<generator_type>()())>;

        using constrained_type = typename graphic_type::constrained_type;
        using reflow_parameters = typename graphic_type::reflow_parameters;


        engine::renderer &renderer;
        watching_lambda_type watcher;
        generator_type generate;
        watching_type last_value = watcher();
        felspar::memory::holding_pen<graphic_type> graphic{};
        autodelete<felspar::memory::holding_pen<graphic_type>> autodeleter{
                renderer};


        bool needs_reflow() {
            if (auto nv = watcher(); nv != last_value) {
                last_value = nv;
                if (graphic) {
                    autodeleter.manage(std::move(*graphic));
                    graphic.reset();
                }
                return true;
            } else {
                return false;
            }
        }

        void draw() { graphic->draw(); }


        constrained_type
                reflow(reflow_parameters const &p, constrained_type const &c) {
            if (not graphic) { graphic.assign(generate()); }
            return graphic->reflow(p, c);
        }
        auto move_to(affine::rectangle2d const &r) {
            return graphic->move_to(r);
        }
    };

    template<typename V, typename G>
    autoupdater(renderer &, V &&, G) -> autoupdater<V, G>;


}
