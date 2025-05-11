# Coordinate spaces

Planet uses a left handed coordinate system, with x going right along the screen and y going up the screen. This matches the standard mathematical axes. The z axis extends out of the screen. This makes it a left-handed coordinate system.

Vulkan has the y axis extend down the screen, and maps 1 to -1 along each axis to the edge of the screen. Because monitors are generally not square, this means that the scale of the two axies is also not square. The [`renderer`](./renderer.hpp) sets the viewport to reverse the y-axis, and the renderer also contains transforms for two coordinate spaces:

* *Screen space* pixel coordinates (`screen_space`) -- A pixel coordinate system that has it's origin at the top left, meaning that the y axis extends *downwards*. This is generally more convenient for UI layout. The number of pixels depends on the resolution of the user's screen, and can change at any time. Going `into` the space transforms from screen to the Vulkan space and going `outof` transforms to the pixel coordinate.
* Square aspect *logical Vulkan space* `logical_vulkan_space` -- Corrects for the non-square pixels of the Vulkan screen coordinate system. Going `into` the space corrects the aspect conversion and going `outof` undoes the correction.

In addition it's usual to have a camera. The camera defines its own coordinate space, typically at the origin looking along the z axis. Going `into` the camera takes a world coordinate and places it in the logical Vulkan space and going `outof` takes a logical coordinate and projects it into the world. The exact APIs for this vary from camera to camera and will be different for 2D and 3D cameras and may be further restricted for cameras that have a perspective projection.

Because Planet uses column vectors the composition of matrices under multiply happens from right to left. So if you're building matrices yourself then `M2 * M1` means that the transformation `M1` happens before the transformation `M2`. The `transform2d` and `transform3d` should generally be used to compose both `into` and `outof` directions together where chained callls are used in the order of the transformations wanted.


## Example conversions between spaces

In order to place a 2D UI element at a particular world location the following steps are needed:

1. Send the world coordinate location `into` the camera to give the logical Vulkan coordinate.
2. Next send the point `into` the logical Vulkan space to fix the aspect ratio.
3. Finally the x and y coordinates of the point can be sent `outof` the screen to give the pixel coordinates.

To work out which world coordinate the mouse is over (for a 2D camera):

1. Take the mouse coordinate `into` the screen space.
2. Undo the aspect ration by going `outof` the logical Vulkan space.
3. Work out the world coordinate by going `outof` the camera space.


## Shaders

The `renderer` exposes three matrices to the shaders:

* `world` -- The camera's `into` direction. By default this is the identity matrix.
* `pixel` -- The screen space's `into` direction. By default this will be set to the renderer's `screen_space.into()`.
* `perspective` -- The perspective projection matrix. By default this is set to the renderer's  `logical_vulkan_space.into()`.


### 2D games

For a 2D game, the coordinate space passed on to shaders should be set using the renderer's `reset_world_coordinates` API and should include the aspect ratio correction:

```cpp
renderer.reset_world_coordinates(camera.view.into());
```


### 3D games

The 3D case is very similar except that we must set both a world and projection matrix, with the projection matrix taking into account the aspect ratio correction that's needed as well:

```cpp
renderer.reset_world_coordinates(
        camera.view.into(),
        renderer.logical_vulkan_space.into()
                * camera.perspective.into());
```

Note that this corrects the aspect ratio after the perspective projection is done.


### In the shader

The vertex shader can use the UBO provided

```glsl
layout(set = 0, binding = 0) uniform CoordinateSpace {
    mat4 world;
    mat4 pixel;
    mat4 perspective;
} coordinates;
```

And then convert world to the Vulkan screen space using:

```glsl
coordinates.perspective * coordinates.world * position
```

This same code will work for both 2D and 3D cameras if the matrices are set as recommended.
