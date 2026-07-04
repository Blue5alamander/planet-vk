if(NOT ${BUILD_VULKAN_LOADER} AND NOT TARGET Vulkan::Vulkan)
    find_package(Vulkan REQUIRED)
endif()

add_library(planet-vk
        commands.cpp
        descriptors.cpp
        frame_buffer.cpp
        headless.cpp
        image.cpp
        init.cpp
        memory.cpp
        memory.block_pool.cpp
        pipeline.cpp
        render_pass.cpp
        shader-pipeline.cpp
        surface.cpp
        swap_chain.cpp
        synchronisation.cpp
        texture.cpp
    )
target_include_directories(planet-vk PUBLIC ../include)
if(${BUILD_VULKAN_LOADER})
    target_link_libraries(planet-vk PUBLIC
            planet-sdl
            Vulkan::Headers
            Vulkan::Vulkan
        )
else()
    target_link_libraries(planet-vk PUBLIC
            planet-sdl
            Vulkan::Vulkan
        )
endif()

install(TARGETS planet-vk LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

target_sources(planet-vk PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ../include/
  FILES
        ../include/planet/vk/buffer.hpp
        ../include/planet/vk/colour.hpp
        ../include/planet/vk/commands.hpp
        ../include/planet/vk/debug_messenger.hpp
        ../include/planet/vk/descriptors.hpp
        ../include/planet/vk/device.hpp
        ../include/planet/vk/extensions.hpp
        ../include/planet/vk/forward.hpp
        ../include/planet/vk/frame_buffer.hpp
        ../include/planet/vk/headless.hpp
        ../include/planet/vk/helpers.hpp
        ../include/planet/vk.hpp
        ../include/planet/vk/image.hpp
        ../include/planet/vk/instance.hpp
        ../include/planet/vk/memory_block_pool.hpp
        ../include/planet/vk/memory.hpp
        ../include/planet/vk/owned_handle.hpp
        ../include/planet/vk/physical_device.hpp
        ../include/planet/vk/pipeline.hpp
        ../include/planet/vk/queue.hpp
        ../include/planet/vk/render_pass.hpp
        ../include/planet/vk/shader_module.hpp
        ../include/planet/vk/surface.hpp
        ../include/planet/vk/swap_chain.hpp
        ../include/planet/vk/synchronisation.hpp
        ../include/planet/vk/texture.hpp
        ../include/planet/vk/ubo/coherent.hpp
        ../include/planet/vk/ubo/coordinate_space.hpp
        ../include/planet/vk/ubo/textures.hpp
        ../include/planet/vk/vertex/bindings.hpp
        ../include/planet/vk/vertex/coloured.hpp
        ../include/planet/vk/vertex/coloured_textured.hpp
        ../include/planet/vk/vertex.hpp
        ../include/planet/vk/vertex/normal.hpp
        ../include/planet/vk/vertex/normal_textured.hpp
        ../include/planet/vk/view.hpp
    )
set_property(TARGET planet-vk PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)
add_dependencies(check planet-vk_verify_interface_header_sets)
if (PLANET_VK_VALIDATION)
    target_compile_definitions(planet-vk PRIVATE
        PLANET_VK_VALIDATION=1)
endif()

add_test_run(check planet-vk TESTS
        init.tests.cpp
        memory.block_pool.tests.cpp
        memory.tests.cpp
    )


add_library(planet-vk-sdl
        sdl.cpp
    )
target_link_libraries(planet-vk-sdl planet-vk)
install(TARGETS planet-vk-sdl LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

target_sources(planet-vk-sdl PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ../include/
  FILES
        ../include/planet/vk-sdl.hpp
        ../include/planet/vk/sdl/texture.hpp
        ../include/planet/vk/sdl/window.hpp
    )
set_property(TARGET planet-vk-sdl PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)
add_dependencies(check planet-vk-sdl_verify_interface_header_sets)


add_library(planet-vk-engine
        app.engine.cpp
        attachments.engine.cpp
        blank.engine.cpp
        glow.postprocess.cpp
        mesh.pipeline.cpp
        renderer.engine.cpp
        sprite.pipeline.cpp
        textured_quad.pipeline.cpp
    )
target_link_libraries(planet-vk-engine planet-vk-sdl)
install(TARGETS planet-vk-engine LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

target_sources(planet-vk-engine PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ../include/
  FILES
        ../include/planet/vk/engine/app.hpp
        ../include/planet/vk/engine/autodelete.hpp
        ../include/planet/vk/engine/blank.hpp
        ../include/planet/vk/engine/colour_attachment.hpp
        ../include/planet/vk/engine/depth_buffer.hpp
        ../include/planet/vk/engine/forward.hpp
        ../include/planet/vk/engine.hpp
        ../include/planet/vk/engine/memory/pooled-vector-map.hpp
        ../include/planet/vk/engine/pipeline/mesh.hpp
        ../include/planet/vk/engine/pipeline/sprite.hpp
        ../include/planet/vk/engine/pipeline/textured_quad.hpp
        ../include/planet/vk/engine/postprocess/glow.hpp
        ../include/planet/vk/engine/renderer.hpp
        ../include/planet/vk/engine/render_parameters.hpp
        ../include/planet/vk/engine/textured.draw.hpp
        ../include/planet/vk/engine/ui/autoupdater.hpp
        ../include/planet/vk/engine/ui.hpp
        ../include/planet/vk/engine/ui/tx.hpp
    )
set_property(TARGET planet-vk-engine PROPERTY VERIFY_INTERFACE_HEADER_SETS ON)
add_dependencies(check planet-vk-engine_verify_interface_header_sets)

add_test_run(check planet-vk-engine TESTS
        pooled-vector-map.tests.cpp
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
    DESTINATION share/planet-vk-engine COMPONENT share)
