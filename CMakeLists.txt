cmake_minimum_required(VERSION 3.12)
project(planet-audio)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_custom_target(felspar-check)
    set_property(TARGET felspar-check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(planet-examples)
    set_property(TARGET planet-examples PROPERTY EXCLUDE_FROM_ALL TRUE)
    include(requirements.cmake)
endif()

if(NOT TARGET check)
    add_custom_target(check)
endif()


macro(vk_shader target input)
    add_custom_target(${target}-${input}.spirv
        COMMAND glslc
            ${CMAKE_CURRENT_SOURCE_DIR}/${input} -o ${input}.spirv
        DEPENDS ${input})
    add_dependencies(${target} ${target}-${input}.spirv)
endmacro()


add_subdirectory(examples)
add_subdirectory(src)
