#include <planet/log.hpp>
#include <planet/vk/extensions.hpp>
#include <planet/vk/headless.hpp>
#include <planet/vk/instance.hpp>
#include <planet/vk/physical_device.hpp>
#include <planet/vk/swap_chain.hpp>

#include <felspar/test.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <vector>


namespace {


    auto const suite = felspar::testsuite("vulkan::device", []() {
        planet::log::active = planet::log::level::error;
    });


    /// Is `VK_KHR_portability_subset` named in a list of enabled extensions?
    bool requests_portability_subset(std::span<char const *const> const list) {
        return std::any_of(
                list.begin(), list.end(), [](char const *const name) {
                    return std::string_view{name}
                    == planet::vk::portability_subset_extension_name;
                });
    }
    /// Does a device's advertised-extension list contain the portability subset?
    bool advertises_portability_subset(
            std::span<VkExtensionProperties const> const list) {
        return std::any_of(list.begin(), list.end(), [](auto const &ex) {
            return std::string_view{ex.extensionName}
            == planet::vk::portability_subset_extension_name;
        });
    }


    /// ### Create a logical device with no window or display
    /**
     * `vk::headless` brings up an instance and device through a
     * `VK_EXT_headless_surface` surface, so the whole `instance` → `surface` →
     * `device` path works with no SDL window and no compositor.
     *
     * This will only run where the Vulkan implementation advertises
     * `VK_EXT_headless_surface` (it does on Mesa's `radv` and the `llvmpipe`
     * software rasteriser; some proprietary drivers omit it).
     */
    auto const headless = suite.test("headless-device", [](auto check) {
        /// Skip rather than fail where `VK_EXT_headless_surface` is unavailable
        auto const vk = planet::vk::headless::make_if_available();
        if (not vk) { return; }
        auto &vulkan = *vk;

        check(vulkan.device.get() != VK_NULL_HANDLE) == true;
        check(vulkan.device.graphics_queue != VK_NULL_HANDLE) == true;
        check(vulkan.device.present_queue != VK_NULL_HANDLE) == true;

        /**
         * The headless surface should advertise
         * `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, so a swap chain that requests it
         * gets `transfer_source::available` and its images can be copied out
         * (the basis for screenshot/readback support).
         */
        planet::vk::swap_chain swap_chain{
                vulkan.device, planet::affine::extents2d{1920, 1080},
                planet::vk::transfer_source::requested};

        check(swap_chain.transfer == planet::vk::transfer_source::available)
                == true;
    });


    /// ### Portability enumeration is opted into only on macOS
    /**
     * MoltenVK is a portability (non-conformant) ICD, which the Khronos loader
     * hides unless the application enables the `VK_KHR_portability_enumeration`
     * instance extension *and* sets the matching
     * `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` create flag. Without
     * both, `vkCreateInstance` enumerates no physical device and fails on
     * macOS. On every other platform the flag and extension must stay absent,
     * so `instance::info` gates them behind `__APPLE__`.
     */
    auto const portability =
            suite.test("instance-info-portability", [](auto check) {
                planet::vk::extensions exts;
                auto const app_info = planet::vk::application_info();
                auto const info = planet::vk::instance::info(exts, app_info);

                bool const has_portability_extension = std::any_of(
                        exts.vulkan_extensions.begin(),
                        exts.vulkan_extensions.end(),
                        [](char const *const name) {
                            return std::string_view{name}
                            == VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
                        });
                bool const has_portability_flag =
                        (info.flags
                         bitand VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR)
                        != 0;

#if defined(__APPLE__)
                check(has_portability_extension) == true;
                check(has_portability_flag) == true;
#else
                check(has_portability_extension) == false;
                check(has_portability_flag) == false;
#endif
            });


    /// ### `VK_KHR_portability_subset` is requested exactly when advertised
    /**
     * A portability ICD such as MoltenVK advertises the
     * `VK_KHR_portability_subset` device extension, and the Vulkan spec
     * *requires* it be enabled at device creation when present. Conformant
     * drivers (Linux `radv`/`llvmpipe`, Windows) never advertise it, so it must
     * not be requested there. `required_device_extensions` decides this from
     * what the device reports rather than from a platform macro, so both
     * outcomes can be exercised on any host.
     */
    auto const portability_subset =
            suite.test("device-extensions-portability", [](auto check) {
                auto const make_ext = [](std::string_view const name) {
                    VkExtensionProperties e{};
                    name.copy(e.extensionName, sizeof(e.extensionName) - 1u);
                    return e;
                };

                std::vector<char const *> const base{
                        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

                /// A device that does not advertise it -> not requested
                std::vector<VkExtensionProperties> const without{
                        make_ext(VK_KHR_SWAPCHAIN_EXTENSION_NAME)};
                auto const conformant =
                        planet::vk::required_device_extensions(without, base);
                check(requests_portability_subset(conformant)) == false;
                check(conformant.size()) == base.size();

                /// A device that advertises it -> appended, base preserved
                std::vector<VkExtensionProperties> const with{
                        make_ext(VK_KHR_SWAPCHAIN_EXTENSION_NAME),
                        make_ext(planet::vk::portability_subset_extension_name)};
                auto const portable =
                        planet::vk::required_device_extensions(with, base);
                check(requests_portability_subset(portable)) == true;
                check(portable.size()) == base.size() + 1u;
                check(requests_portability_subset(base)) == false;
            });


    /// ### The chosen device requests portability-subset iff it advertises it
    /**
     * Ties `required_device_extensions` to the real device the headless context
     * selected: on Linux `radv`/`llvmpipe` it is absent (so not requested); on
     * MoltenVK it is present (so requested). Either way the two must agree,
     * which is the invariant logical-device creation relies on.
     *
     * Skips where `VK_EXT_headless_surface` is unavailable, like the other
     * device tests.
     */
    auto const portability_subset_device =
            suite.test("device-extensions-portability-device", [](auto check) {
                auto const vk = planet::vk::headless::make_if_available();
                if (not vk) { return; }

                auto const enabled = planet::vk::required_device_extensions(
                        vk->instance.gpu().extensions,
                        vk->extensions.device_extensions);

                check(requests_portability_subset(enabled))
                        == advertises_portability_subset(
                                vk->instance.gpu().extensions);
            });


}


/// ## Leak sanitiser suppressions
/**
 * The Mesa Vulkan driver allocates one-time global and per-thread state during
 * instance creation (behind `pthread_once`) that it never frees before the
 * process exits. That isn't a leak in our code, but LSan can't tell, so
 * suppress allocations made on the driver's one-time-initialisation paths. Our
 * own RAII wrappers are still leak-checked.
 */
extern "C" char const *__lsan_default_suppressions() {
    return "leak:libvulkan\n"
           "leak:__pthread_once_slow\n";
}


/// ## Disable the end-of-run leak scan
/**
 * Suppressing the driver's allocations (above) keeps the leak *report* clean,
 * but on these GPU tests the leak *scan* is itself the problem. LSan's
 * end-of-run check stops the world -- suspending every live thread to scan it
 * for roots -- and intermittently deadlocks trying to suspend a Mesa driver
 * thread parked in a DRM `ioctl`. The hang then trips the test runner's 30s
 * watchdog, which kills the process with exit code 127 long after every test
 * has already passed.
 *
 * So turn the leak check off for this binary. Every other AddressSanitizer
 * check stays active; we lose only the leak scan, and only here where the
 * driver makes it unrunnable. Comment this out to re-enable the scan -- the
 * suppressions above still apply -- and check whether the driver still wedges
 * it.
 */
extern "C" int __lsan_is_turned_off() { return 1; }
