#include <planet/vk/engine2d/ui.hpp>


/// ## `planet::vk::engine2d::ui::on_screen`


planet::vk::engine2d::ui::on_screen::on_screen()
: reflowable{"planet::vk::engine2d::ui::on_screen"} {}
planet::vk::engine2d::ui::on_screen::on_screen(std::string_view const n)
: reflowable{n} {}
planet::vk::engine2d::ui::on_screen::on_screen(vk::texture tx)
: reflowable{"planet::vk::engine2d::ui::on_screen"}, texture{std::move(tx)} {}
planet::vk::engine2d::ui::on_screen::on_screen(
        std::string_view const n, vk::texture tx)
: reflowable{n}, texture{std::move(tx)} {}
planet::vk::engine2d::ui::on_screen::on_screen(
        vk::texture tx, vk::colour const &c)
: reflowable{"planet::vk::engine2d::ui::on_screen"},
  texture{std::move(tx)},
  colour{c} {}
planet::vk::engine2d::ui::on_screen::on_screen(
        std::string_view const n, vk::texture tx, vk::colour const &c)
: reflowable{n}, texture{std::move(tx)}, colour{c} {}


void planet::vk::engine2d::ui::on_screen::draw(renderer &r) {
    r.screen.draw(texture, position(), colour);
}


auto planet::vk::engine2d::ui::on_screen::do_reflow(constrained_type const &c)
        -> constrained_type {
    return planet::ui::scaling(texture.image.extents(), c, texture.fit);
}


void planet::vk::engine2d::ui::on_screen::move_sub_elements(
        affine::rectangle2d const &) {}
