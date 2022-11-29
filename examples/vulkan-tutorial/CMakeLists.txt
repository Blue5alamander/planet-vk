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
    target_include_directories (vulkan-tutorial PRIVATE ${STB_INCLUDEDIR})
    target_link_libraries(vulkan-tutorial PUBLIC
            planet-vk

            glfw
            glm::glm
            tinyobjloader::tinyobjloader
        )

endif()
