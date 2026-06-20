# Planet Vk

[![Documentation](https://badgen.net/static/docs/blue5alamander.com)](https://blue5alamander.com/open-source/planet-vk/)
[![GitHub](https://badgen.net/badge/Github/planet-vk/green?icon=github)](https://github.com/Blue5alamander/planet-vk/)
[![License](https://badgen.net/github/license/Blue5alamander/planet-vk)](https://github.com/Blue5alamander/planet-vk/blob/main/LICENSE_1_0.txt)
[![Discord](https://badgen.net/badge/icon/discord?icon=discord&label)](https://discord.gg/tKSabUa52v)

A set of C++ wrappers that can be used with the Vulkan APIs and provide some convenience, like RAII, together with a game engine.


## Libraries available

Planet-vk is split into three parts:

* `planet-vk` -- General Vulkan library. This is general Vulkan code.
* `planet-vk-sdl` -- Integration with SDL. Contains the critical integration between Vulkan and the Windowing system.
* `planet-vk-engine` -- A game engine. The engine used for Blue 5alamander games and software.

The exact boundary between `planet-vk` and `planet-vk-engine` is fluid. There are likely many things that different people would want to place in different layers. The basic idea is to be able to make use of the `planet-vk` facilities without also being forced to use the game application and renderer abstractions.


## Build requirements

Packages needed on Debian systems:

```bash
sudo apt-get install libvulkan-dev glslc vulkan-utility-libraries-dev
```

To build the examples you'll also need `libglfw3-dev`, `libglm-dev`, `libstb-dev` and `libtinyobjloader-dev`

On Windows you'll need to download and install the SDK.
