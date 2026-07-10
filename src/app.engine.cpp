#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/renderer.hpp>

#if PLANET_SDL3
#include <SDL3/SDL_vulkan.h>
#else
#include <SDL_vulkan.h>
#endif


planet::vk::engine::app::app(
        int,
        char const *argv[],
        planet::sdl::init &s,
        planet::version const &version)
: asset_manager{argv[0]},
  sdl{s},
  /**
   * SDL3 dropped `SDL_WINDOW_FULLSCREEN_DESKTOP`: a plain
   * `SDL_WINDOW_FULLSCREEN` window whose fullscreen mode is left unset (the
   * default, `NULL`) is the borderless-fullscreen-desktop equivalent.
   */
  window{sdl, version.application_id.c_str(),
#if PLANET_SDL3
         SDL_WINDOW_FULLSCREEN
#else
         SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
  },
  instance{[&]() {
      auto app_info = planet::vk::application_info();
      app_info.pApplicationName = version.application_id.c_str();
      app_info.applicationVersion = VK_MAKE_VERSION(
              version.semver.major, version.semver.minor, version.semver.patch);
      auto info = planet::vk::instance::info(extensions, app_info);
      return planet::vk::instance{
              extensions, info, [&](VkInstance instance_handle) {
                  VkSurfaceKHR surface_handle = VK_NULL_HANDLE;
                  /**
                   * Under SDL3 `SDL_Vulkan_CreateSurface` takes an extra
                   * `VkAllocationCallbacks*` (NULL selects Vulkan's default
                   * allocator) before the surface handle. Both SDL2 and SDL3
                   * return a falsy-on-failure bool, so the shared `not` check
                   * still holds.
                   */
                  if (not SDL_Vulkan_CreateSurface(
                              window.get(), instance_handle,
#if PLANET_SDL3
                              nullptr,
#endif
                              &surface_handle)) {
                      throw felspar::stdexcept::runtime_error{
                              "SDL_Vulkan_CreateSurface failed"};
                  }
                  return surface_handle;
              }};
  }()} {
}


int planet::vk::engine::app::run(
        felspar::coro::task<int> (*co_main)(app &, renderer &)) {
    auto const wrapper = [](felspar::io::warden &, app *papp,
                            felspar::coro::task<int> (*cm)(app &, renderer &))
            -> felspar::io::warden::task<int> {
        try {
            planet::vk::engine::renderer renderer{*papp};
            co_return co_await cm(*papp, renderer);
        } catch (std::exception const &e) {
            planet::log::critical("Exception caught", e.what());
        }
    };
    return sdl.io.run(+wrapper, this, co_main);
}
