#pragma once


#include <planet/vk/engine/autodelete.hpp>


namespace planet::vk::engine {


    /// ## Automatically update a widget
    /**
     * As well as the renderer you pass it either a value to watch for changes,
     * or a lambda that returns the value to watch for changes. Then you must
     * pass a lambda that returns the graphics widget that is to be shown.
     *
     * The value is then watched for changes (or the lambda is called which
     * returns the current value) and if the value is different from the last
     * display then the graphics generation lambda is called again to create a
     * new graphical element. The old graphic is held and deleted after any
     * frames using it have been fully rendered.
     *
     * The graphic is updated during the layout phase, so in order to actually
     * see any updates any UI parts containing a `autoupdater` need to be
     * reflowed each frame.
     */

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
            if (needs_reflow() or not graphic) { graphic.assign(generate()); }
            return graphic->reflow(p, c);
        }
        auto move_to(reflow_parameters const &p, affine::rectangle2d const &r) {
            return graphic->move_to(p, r);
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
            if (needs_reflow() or not graphic) { graphic.assign(generate()); }
            return graphic->reflow(p, c);
        }
        auto move_to(reflow_parameters const &p, affine::rectangle2d const &r) {
            return graphic->move_to(p, r);
        }
    };

    template<typename V, typename G>
    autoupdater(renderer &, V &&, G) -> autoupdater<V, G>;


}
