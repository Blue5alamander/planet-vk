find_package (Vulkan REQUIRED)

add_library(planet-vk
        commands.cpp
        descriptors.cpp
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
vk_shader(planet-vk-engine2d 2dmesh.frag)
vk_shader(planet-vk-engine2d 2dmesh.vert)
target_link_libraries(planet-vk-engine2d planet-vk-sdl)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/2dmesh.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/2dmesh.vert.spirv
    DESTINATION share/planet-vk-engine2d)
