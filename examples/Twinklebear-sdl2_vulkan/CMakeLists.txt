if(TARGET planet-examples)
    add_executable(sdl2-example main.cpp)
    vk_shader(sdl2-example fragment.frag)
    vk_shader(sdl2-example vertex.vert)
    target_link_libraries(sdl2-example PUBLIC
            planet-vk-sdl
        )
    install(TARGETS sdl2-example EXPORT sdl2-example RUNTIME DESTINATION bin)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/fragment.frag.spirv
            ${CMAKE_CURRENT_BINARY_DIR}/vertex.vert.spirv
        DESTINATION share)
endif()
