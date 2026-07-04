cmake_minimum_required(VERSION 3.24)
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
        set(spirv_file ${input}.${ARGV2}.spirv)
        add_custom_command(OUTPUT ${spirv_file}
            COMMAND glslc
                    -D${ARGV2}=main
                    ${CMAKE_CURRENT_SOURCE_DIR}/${input}
                    -o ${spirv_file}
            MAIN_DEPENDENCY ${input})
    else()
        set(spirv_file ${input}.spirv)
        add_custom_command(OUTPUT ${spirv_file}
            COMMAND glslc
                    ${CMAKE_CURRENT_SOURCE_DIR}/${input}
                    -o ${spirv_file}
            MAIN_DEPENDENCY ${input})
    endif()
    add_custom_target(${target}-${spirv_file} DEPENDS ${spirv_file})
    add_dependencies(${target} ${target}-${spirv_file})
endmacro()


add_subdirectory(examples)
add_subdirectory(src)
