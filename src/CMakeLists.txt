find_package (Vulkan REQUIRED)

add_library(planet-vk INTERFACE)
target_include_directories(planet-vk INTERFACE ../include)
target_link_libraries(planet-vk INTERFACE
        planet-sdl
        Vulkan::Vulkan
    )

add_library(planet-vk-sdl
        sdl.cpp
    )
target_link_libraries(planet-vk-sdl planet-vk)
