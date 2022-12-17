#include <planet/vk/engine2d.hpp>


int main(int const argc, char const *argv[]) {
    planet::vk::engine2d::app app{argc, argv, "2d example"};
    planet::vk::engine2d::renderer renderer{app};

    return 0;
}
