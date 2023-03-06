find_package (Vulkan REQUIRED)

add_library(planet-vk
        commands.cpp
        descriptors.cpp
        frame_buffer.cpp
        image.cpp
        init.cpp
        memory.cpp
        pipeline.cpp
        render_pass.cpp
        shader-pipeline.cpp
        surface.cpp
        swap_chain.cpp
        synchronisation.cpp
        texture.cpp
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
        mesh.pipeline.cpp
        renderer.engine2d.cpp
        textured.pipeline.cpp
    )
vk_shader(planet-vk-engine2d mesh.frag)
vk_shader(planet-vk-engine2d mesh.vert)
vk_shader(planet-vk-engine2d textured.frag)
vk_shader(planet-vk-engine2d textured.vert)
target_link_libraries(planet-vk-engine2d planet-vk-sdl)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/textured.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/textured.vert.spirv
    DESTINATION share/planet-vk-engine2d)
