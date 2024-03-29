include(FetchContent)

FetchContent_Declare(
        felspar-test
        GIT_REPOSITORY https://github.com/Felspar/test.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-test)
if(NOT felspar-test_POPULATED)
    FetchContent_Populate(felspar-test)
    add_subdirectory(${felspar-test_SOURCE_DIR} ${felspar-test_BINARY_DIR})
endif()


FetchContent_Declare(
        felspar-coro
        GIT_REPOSITORY https://github.com/Felspar/coro.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-coro)
if(NOT felspar-coro_POPULATED)
    FetchContent_Populate(felspar-coro)
    add_subdirectory(${felspar-coro_SOURCE_DIR} ${felspar-coro_BINARY_DIR})
endif()

FetchContent_Declare(
        felspar-exceptions
        GIT_REPOSITORY https://github.com/Felspar/exceptions.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-exceptions)
if(NOT felspar-exceptions_POPULATED)
    FetchContent_Populate(felspar-exceptions)
    add_subdirectory(${felspar-exceptions_SOURCE_DIR} ${felspar-exceptions_BINARY_DIR})
endif()

FetchContent_Declare(
        felspar-io
        GIT_REPOSITORY https://github.com/Felspar/io.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-io)
if(NOT felspar-io_POPULATED)
    FetchContent_Populate(felspar-io)
    add_subdirectory(${felspar-io_SOURCE_DIR} ${felspar-io_BINARY_DIR})
endif()

FetchContent_Declare(
        felspar-memory
        GIT_REPOSITORY https://github.com/Felspar/memory.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-memory)
if(NOT felspar-memory_POPULATED)
    FetchContent_Populate(felspar-memory)
    add_subdirectory(${felspar-memory_SOURCE_DIR} ${felspar-memory_BINARY_DIR})
endif()

FetchContent_Declare(
        felspar-parse
        GIT_REPOSITORY https://github.com/Felspar/parse.git
        GIT_TAG main
    )
FetchContent_GetProperties(felspar-parse)
if(NOT felspar-parse_POPULATED)
    FetchContent_Populate(felspar-parse)
    add_subdirectory(${felspar-parse_SOURCE_DIR} ${felspar-parse_BINARY_DIR})
endif()

FetchContent_Declare(
        planet
        GIT_REPOSITORY https://github.com/Blue5alamander/planet.git
        GIT_TAG main
    )
FetchContent_GetProperties(planet)
if(NOT planet_POPULATED)
    FetchContent_Populate(planet)
    add_subdirectory(${planet_SOURCE_DIR} ${planet_BINARY_DIR})
endif()

FetchContent_Declare(
        planet-sdl
        GIT_REPOSITORY https://github.com/Blue5alamander/planet-sdl.git
        GIT_TAG main
    )
FetchContent_GetProperties(planet-sdl)
if(NOT planet-sdl_POPULATED)
    FetchContent_Populate(planet-sdl)
    add_subdirectory(${planet-sdl_SOURCE_DIR} ${planet-sdl_BINARY_DIR})
endif()
