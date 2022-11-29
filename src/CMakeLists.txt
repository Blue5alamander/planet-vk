find_package (Vulkan REQUIRED)

add_library(planet-vk INTERFACE)
target_include_directories(planet-vk INTERFACE ../include)
target_link_libraries(planet-vk INTERFACE
        felspar-memory
        Vulkan::Vulkan
    )
