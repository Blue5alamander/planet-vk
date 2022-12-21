find_package (Vulkan REQUIRED)

add_library(planet-vk
        commands.cpp
        frame_buffer.cpp
        init.cpp
        memory.cpp
        pipeline.cpp
        render_pass.cpp
        shader-pipeline.cpp
        surface.cpp
        swap_chain.cpp
        synchronisation.cpp
    )
target_include_directories(planet-vk PUBLIC ../include)
target_link_libraries(planet-vk PUBLIC
        planet-sdl
        Vulkan::Vulkan
    )

add_library(planet-vk-sdl
        sdl.cpp
    )
target_link_libraries(planet-vk-sdl planet-vk)


add_library(planet-vk-engine2d
        app.engine2d.cpp
        renderer.engine2d.cpp
    )
target_link_libraries(planet-vk-engine2d planet-vk-sdl)
