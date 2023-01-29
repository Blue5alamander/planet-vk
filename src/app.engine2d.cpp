#include <planet/vk/engine2d/app.hpp>
#include <SDL_vulkan.h>


planet::vk::engine2d::app::app(int, char const *argv[], char const *name)
: warden{std::make_unique<felspar::io::poll_warden>()},
  asset_manager{argv[0]},
  window{sdl, name, SDL_WINDOW_FULLSCREEN_DESKTOP},
  instance{[&]() {
      auto app_info = planet::vk::application_info();
      app_info.pApplicationName = name;
      app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
      auto info = planet::vk::instance::info(extensions, app_info);
      return planet::vk::instance{
              extensions, info, [&](VkInstance instance_handle) {
                  VkSurfaceKHR surface_handle = VK_NULL_HANDLE;
                  if (not SDL_Vulkan_CreateSurface(
                              window.get(), instance_handle, &surface_handle)) {
                      throw felspar::stdexcept::runtime_error{
                              "SDL_Vulkan_CreateSurface failed"};
                  }
                  return surface_handle;
              }};
  }()} {}
