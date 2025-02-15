#include <planet/serialise.hpp>
#include <planet/vk/colour.hpp>


void planet::vk::save(planet::serialise::save_buffer &sb, colour const &c) {
    sb.save_box(c.box, c.r, c.g, c.b, c.a);
}
void planet::vk::load(planet::serialise::box &box, colour &c) {
    box.named(c.box, c.r, c.g, c.b, c.a);
}
