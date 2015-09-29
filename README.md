# IntenseLogic Demos

This repository contains several demonstrations of what you can do
with [IntenseLogic](https://github.com/TheCodeLab/IntenseLogic).

- Bouncing Lights: A heightmap with several bouncing spheres, every
  single one is a point light source. The physics is performed by
  Bullet Physics.
- Shadertoy: A clone of the functionality offered by
  [shadertoy.com](http://shadertoy.com/). Many of the uniforms are
  provided. When you save a file, it instantly detects this and
  reloads the shader while it's running. Detecting shader reloads is
  performed through libuv.
- Lighting: An attempt at demonstrating the physically-based rendering
  pipeline in IL.

# Dependencies

- Bullet
- SDL 2
- libuv
- libepoxy
- libpng
- tup

# Building

Just run `tup`.

# Running

The only one of these which really has command line options is
shadertoy, which is invoked like:

    shadertoy -f=some_shader.frag

This will load any shaders in the shadertoys/ directory, which is
where you should put any new shaders you make.

You don't need to restart shadertoy if you edit your shader, it will
reload automatically.
