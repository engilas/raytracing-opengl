### Realtime RayTracing OpenGL

Implemented with OpenGL fragment shaders.

Scene setup in main.cpp source file.

#### Controls:

Rotate camera with mouse

Movement:

- WASD
- Space - up
- Ctrl - down
- Shift (hold) - boost
- Alt (hold) - slowdown

#### Build
For windows: only x86, but you can replace glwf binaries (common/glfw) to x64 if you want to build for x64
```sh
mkdir bin
cd bin
cmake -A Win32 ..
cmake --build . --config Release
```

#### Requirements

* CMake (>= 3.0.2)
* GPU with OpenGL (>= 3.1) support
* GLFW, should be automatically found by CMake.

#### Screenshots

![](media/animation.gif)
