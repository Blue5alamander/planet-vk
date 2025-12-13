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
install(TARGETS planet-vk LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
if (PLANET_VK_VALIDATION)
    target_compile_definitions(planet-vk PRIVATE
        PLANET_VK_VALIDATION=1)
endif()


add_library(planet-vk-sdl
        sdl.cpp
    )
target_link_libraries(planet-vk-sdl planet-vk)
install(TARGETS planet-vk-sdl LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)


add_library(planet-vk-engine
        app.engine.cpp
        attachments.engine.cpp
        blank.engine.cpp
        glow.postprocess.cpp
        mesh.pipeline.cpp
        renderer.engine.cpp
        sprite.pipeline.cpp
        textured.pipeline.cpp
    )
vk_shader(planet-vk-engine mesh.frag)
vk_shader(planet-vk-engine mesh.screen.vert)
vk_shader(planet-vk-engine mesh.world.vert)
vk_shader(planet-vk-engine postprocess.blur.frag horizontal)
vk_shader(planet-vk-engine postprocess.blur.frag vertical)
vk_shader(planet-vk-engine postprocess.copy.frag)
vk_shader(planet-vk-engine postprocess.glow.frag)
vk_shader(planet-vk-engine postprocess.vert)
vk_shader(planet-vk-engine sprite.frag)
vk_shader(planet-vk-engine sprite.screen.vert)
vk_shader(planet-vk-engine sprite.world.vert)
vk_shader(planet-vk-engine textured.frag)
vk_shader(planet-vk-engine textured.glow.frag)
vk_shader(planet-vk-engine texture.screen.vert)
vk_shader(planet-vk-engine texture.world.vert)
target_link_libraries(planet-vk-engine planet-vk-sdl)
install(TARGETS planet-vk-engine LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)


install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.screen.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/mesh.world.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/postprocess.blur.frag.horizontal.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/postprocess.blur.frag.vertical.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/postprocess.copy.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/postprocess.glow.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/postprocess.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.screen.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/sprite.world.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/textured.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/textured.glow.frag.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/texture.screen.vert.spirv
        ${CMAKE_CURRENT_BINARY_DIR}/texture.world.vert.spirv
    DESTINATION share/planet-vk-engine)
