#include <planet/serialise.hpp>
#include <planet/vk/vertex.hpp>


/// ## 'planet::vk::vertex::coloured`


void planet::vk::vertex::save(
        planet::serialise::save_buffer &sb, coloured const &v) {
    sb.save_box(v.box, v.p, v.col);
}
void planet::vk::vertex::load(planet::serialise::box &box, coloured &v) {
    box.named(v.box, v.p, v.col);
}


/// ## `planet::vk::vertex::coloured_textured`


void planet::vk::vertex::save(
        serialise::save_buffer &sb, coloured_textured const &t) {
    sb.save_box(t.box, t.p, t.col, t.uv);
}
void planet::vk::vertex::load(serialise::box &box, coloured_textured &t) {
    box.named(t.box, t.p, t.col, t.uv);
}


/// ## `planet::vk::vertex::normal`


void planet::vk::vertex::save(serialise::save_buffer &sb, normal const &n) {
    sb.save_box(n.box, n.p, n.n);
}
void planet::vk::vertex::load(serialise::box &box, normal &n) {
    box.named(n.box, n.p, n.n);
}


/// ## `planet::vk::vertex::normal_textured`


void planet::vk::vertex::save(
        serialise::save_buffer &sb, normal_textured const &n) {
    sb.save_box(n.box, n.p, n.n, n.uv);
}
void planet::vk::vertex::load(serialise::box &box, normal_textured &n) {
    box.named(n.box, n.p, n.n, n.uv);
}


/// ## `planet::vk::vertex::uvpos`


void planet::vk::vertex::save(serialise::save_buffer &sb, uvpos const &p) {
    sb.save_box(p.box, p.u, p.v);
}
void planet::vk::vertex::load(serialise::box &box, uvpos &p) {
    box.named(p.box, p.u, p.v);
}
