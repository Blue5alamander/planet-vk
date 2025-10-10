cmake_minimum_required(VERSION 3.12)
project(planet-vk)


set(PLANET_VK_VALIDATION NO CACHE BOOL
        "Enable VK_LAYER_KHRONOS_validation")


if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    add_custom_target(felspar-check)
    set_property(TARGET felspar-check PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(felspar-stress)
    set_property(TARGET felspar-stress PROPERTY EXCLUDE_FROM_ALL TRUE)
    add_custom_target(planet-examples)
    set_property(TARGET planet-examples PROPERTY EXCLUDE_FROM_ALL TRUE)

    include(requirements.cmake)
    add_compile_options(
            -Werror
            -fdiagnostics-color=always
            -fdiagnostics-show-option
            -Wall -Wextra -Wpedantic
        )
endif()


macro(vk_shader target input)
    if(${ARGC} GREATER 2)
        add_custom_command(OUTPUT ${input}.${ARGV2}.spirv
            COMMAND glslc
                    -D${ARGV2}=main
                    ${CMAKE_CURRENT_SOURCE_DIR}/${input}
                    -o ${input}.${ARGV2}.spirv
            MAIN_DEPENDENCY ${input})
        add_custom_target(${target}-${input}.${ARGV2}.spirv
                DEPENDS ${input}.${ARGV2}.spirv)
        add_dependencies(${target} ${target}-${input}.${ARGV2}.spirv)
    else()
        add_custom_command(OUTPUT ${input}.spirv
            COMMAND glslc
                    ${CMAKE_CURRENT_SOURCE_DIR}/${input}
                    -o ${input}.spirv
            MAIN_DEPENDENCY ${input})
        add_custom_target(${target}-${input}.spirv DEPENDS ${input}.spirv)
        add_dependencies(${target} ${target}-${input}.spirv)
    endif()
endmacro()


add_subdirectory(examples)
add_subdirectory(src)
