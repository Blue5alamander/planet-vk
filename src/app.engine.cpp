#include <planet/vk/engine/app.hpp>
#include <planet/vk/engine/renderer.hpp>

#include <planet/platform.hpp>

#include <SDL3/SDL_vulkan.h>

#include <cstdlib>
#include <filesystem>


namespace {


    /// ## Configure the process before the Vulkan window is created
    /**
     * On macOS a double-clicked `.app` runs with no wrapper script, so nothing
     * has exported `VK_ICD_FILENAMES` -- and the source-built Khronos loader
     * does not search macOS's default `icd.d` locations, so it would come up
     * with no driver and window creation would fail. Derive the bundled
     * MoltenVK ICD manifest's path from the executable's own location
     * (`Contents/MacOS/<exe>` to `Contents/Resources/vulkan/icd.d/`, where
     * make-macos-app puts the manifest and driver -- the manifest is not code,
     * so it can't live under `Frameworks/`) and set it. `setenv`'s
     * `overwrite = 0` leaves any existing value alone, so the dev
     * `run`/`run.sh` wrappers still win. Returns `exe` unchanged so this can
     * wrap the first member's initialiser and run before the window member.
     *
     * Scoped to `platform::macos`, not all of Apple: the
     * `Contents/MacOS`/`Contents/Frameworks` layout is the macOS bundle
     * structure, which iOS (a flat bundle, executable and `Frameworks/` at the
     * root) does not share.
     */
    std::filesystem::path configure_bundle(std::filesystem::path exe) {
        if constexpr (planet::current_platform == planet::platform::macos) {
            auto const icd = (exe.parent_path() / ".." / "Resources" / "vulkan"
                              / "icd.d" / "MoltenVK_icd.json")
                                     .lexically_normal();
            /**
             * Only when the bundled manifest is actually present, so a flat
             * layout or a dev run (where the `run` wrappers export their own
             * path) is never handed a path that does not exist.
             */
            if (std::filesystem::exists(icd)) {
#ifndef _WIN32
                /**
                 * POSIX `::setenv` won't build on Windows so this has to be
                 * hidden to the Windows compiler.
                 */
                ::setenv("VK_ICD_FILENAMES", icd.c_str(), 0);
#endif
            }
        }
        return exe;
    }


}


planet::vk::engine::app::app(
        int,
        char const *argv[],
        planet::sdl::init &s,
        planet::version const &version)
: asset_manager{configure_bundle(argv[0])},
  sdl{s},
  /**
   * SDL3 dropped `SDL_WINDOW_FULLSCREEN_DESKTOP`: a plain
   * `SDL_WINDOW_FULLSCREEN` window whose fullscreen mode is left unset (the
   * default, `NULL`) is the borderless-fullscreen-desktop equivalent.
   */
  window{sdl, version.application_id.c_str(), SDL_WINDOW_FULLSCREEN},
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
                   * SDL3's `SDL_Vulkan_CreateSurface` takes an extra
                   * `VkAllocationCallbacks*` (NULL selects Vulkan's default
                   * allocator) before the surface handle, and returns a
                   * falsy-on-failure bool.
                   */
                  if (not SDL_Vulkan_CreateSurface(
                              window.get(), instance_handle, nullptr,
                              &surface_handle)) {
                      throw felspar::stdexcept::runtime_error{
                              "SDL_Vulkan_CreateSurface failed"};
                  }
                  return surface_handle;
              }};
  }()} {}


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
