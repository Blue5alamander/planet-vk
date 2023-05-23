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
if (PLANET_VK_VALIDATION)
    target_compile_definitions(planet-vk PRIVATE
        PLANET_VK_VALIDATION=1)
endif()


add_library(planet-vk-sdl
        sdl.cpp
    )
target_link_libraries(planet-vk-sdl planet-vk)


add_library(planet-vk-engine
        app.engine.cpp
        mesh.pipeline.cpp
        renderer.engine.cpp
        sprite.pipeline.cpp
        textured.pipeline.cpp
        ui.engine.cpp
    )
vk_shader(planet-vk-engine mesh.frag)
vk_shader(planet-vk-engine mesh.vert)
vk_shader(planet-vk-engine sprite.frag)
vk_shader(planet-vk-engine sprite.screen.vert)
vk_shader(planet-vk-engine sprite.world.vert)
vk_shader(planet-vk-engine textured.frag)
vk_shader(planet-vk-engine texture.screen.vert)
vk_shader(planet-vk-engine texture.world.vert)
target_link_libraries(planet-vk-engine planet-vk-sdl)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.screen.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.world.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/textured.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/texture.screen.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/texture.world.vert.spirv
    DESTINATION share/planet-vk-engine)
