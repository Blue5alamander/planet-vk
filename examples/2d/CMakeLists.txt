if(TARGET planet-examples)
    add_executable(2d-example 2d-example.cpp)
    target_link_libraries(2d-example PUBLIC
            planet-vk-engine
        )
    install(TARGETS 2d-example EXPORT 2d-example RUNTIME DESTINATION bin)
endif()
