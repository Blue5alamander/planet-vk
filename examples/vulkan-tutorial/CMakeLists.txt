if(TARGET planet-examples)

    find_package (glfw3 REQUIRED)
    find_package (glm REQUIRED)
    find_package (tinyobjloader REQUIRED)

    find_package (PkgConfig)
    pkg_get_variable (STB_INCLUDEDIR stb includedir)
    if (NOT STB_INCLUDEDIR)
        unset (STB_INCLUDEDIR)
        find_path (STB_INCLUDEDIR stb_image.h PATH_SUFFIXES stb)
    endif ()

    add_executable(vulkan-tutorial 30_multisampling.cpp)
    vk_shader(vulkan-tutorial 27_shader_depth.frag)
    vk_shader(vulkan-tutorial 27_shader_depth.vert)
    target_include_directories (vulkan-tutorial PRIVATE ${STB_INCLUDEDIR})
    target_link_libraries(vulkan-tutorial PUBLIC
            planet-vk-engine

            glfw
            glm::glm
            tinyobjloader::tinyobjloader
        )
    install(TARGETS vulkan-tutorial EXPORT vulkan-tutorial RUNTIME DESTINATION bin)
    install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/viking_room.obj
            ${CMAKE_CURRENT_SOURCE_DIR}/viking_room.png
            ${CMAKE_CURRENT_BINARY_DIR}/27_shader_depth.frag.spirv
            ${CMAKE_CURRENT_BINARY_DIR}/27_shader_depth.vert.spirv
        DESTINATION share)
endif()
