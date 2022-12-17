if(TARGET planet-examples)
    add_executable(2d-example 2d-example.cpp)
        vk_shader(2d-example 2d-example.frag)
        vk_shader(2d-example 2d-example.vert)
    target_link_libraries(2d-example PUBLIC
            planet-vk-engine2d
        )
    install(TARGETS 2d-example EXPORT 2d-example RUNTIME DESTINATION bin)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/2d-example.frag.spirv
            ${CMAKE_CURRENT_BINARY_DIR}/2d-example.vert.spirv
        DESTINATION share)
endif()
