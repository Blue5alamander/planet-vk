if(TARGET planet-examples)
    add_executable(sdl2-example main.cpp)
    vk_shader(sdl2-example frag.frag)
    vk_shader(sdl2-example vert.vert)
    target_link_libraries(sdl2-example PUBLIC
            planet-vk
        )
    install(TARGETS sdl2-example EXPORT sdl2-example RUNTIME DESTINATION bin)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/frag.frag.spirv
            ${CMAKE_CURRENT_BINARY_DIR}/vert.vert.spirv
        DESTINATION share)
endif()
