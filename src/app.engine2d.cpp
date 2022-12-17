#include <planet/vk/engine2d/app.hpp>
#include <SDL_vulkan.h>


planet::vk::engine2d::app::app(int argc, char const *argv[], char const *name)
: warden{std::make_unique<felspar::io::poll_warden>()},
  asset_manager{argv[0]},
  window{sdl, name, 800, 600},
  instance{[&]() {
      auto app_info = planet::vk::application_info();
      app_info.pApplicationName = name;
      app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      return planet::vk::instance{
              planet::vk::instance::info(extensions, app_info),
              [&](VkInstance h) {
                  VkSurfaceKHR vk_surface = VK_NULL_HANDLE;
                  if (not SDL_Vulkan_CreateSurface(
                              window.get(), h, &vk_surface)) {
                      throw felspar::stdexcept::runtime_error{
                              "SDL_Vulkan_CreateSurface failed"};
                  }
                  return vk_surface;
              }};
  }()} {}
