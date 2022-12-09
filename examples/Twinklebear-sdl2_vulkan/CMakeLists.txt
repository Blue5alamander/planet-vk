if(TARGET planet-examples)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake")

    find_package(Threads REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(Vulkan REQUIRED)

    add_spirv_embed_library(spirv_shaders vert.vert frag.frag)

    add_executable(sdl2-example main.cpp)
    target_link_libraries(sdl2-example PUBLIC
            spirv_shaders
            planet-vk
        )
endif()
