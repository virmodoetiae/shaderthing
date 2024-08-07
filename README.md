# ShaderThing <img src=".gitassets/logo.gif" alt="Icon" style="height:64px; vertical-align:middle;"> 

ShaderThing is a cross-platform GUI-based tool for live shader editing written in C++, leveraging [OpenGL](https://www.opengl.org/) for graphics and [ImGui](https://github.com/ocornut/imgui) for the UI. It can be thought of as a (much) more flexible and capable off-line version of [Shadertoy](https://www.shadertoy.com/), featuring:

* Layer-based shader system: each shader consists of a layer, which can render its content to:
    * the main window, with support for multiple layers rendering to the screen at the same time and transparency-based merging and blending;
    * its own framebuffer, which can be further manipulated by other layers.
* Live shader uniform editor with mouse/keyboard input support: click and drag to set floats, vectors, textures, cube-maps, pick colors and more!
* Live shader storage in the form of a shared SSBO coupled with a custom data viewer: store and inspect any shader quantity in real-time (very useful for, but not limited to, debugging purposes)
* Resource manager: load your own textures, manage framebuffer-rendered layers and generate cube-maps.
* Exporter: export static images, GIFs or video frames (to be packaged into videos by your third-party software of choice) with a variety of output rendering controls.
* Built-in high-performance compute-shader-based post-processing effects: K-Means color quantizer with optional dithering and live palette manipulation; bloom effect based on [industry-proven approaches](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/); two-pass sub-pixel-stepped gaussian blur effect.
* Built-in library of useful GLSL code snippets!

Compared to Shadertoy, some features are currently missing, primarily sound input/output-related as well as 3D-texture uniforms, though support for these will probably be added at some point in the future. 

Whether you are merely curious about or already have solid skills in the field of shaders, give it a try! Both the source code and the [executables](https://github.com/virmodoetiae/shaderthing/releases/) are distributed under a permissive modified [zlib/linpng license](https://opensource.org/license/zlib/).

# Requirements for running

ShaderThing does not directly deal with OpenGL code, that is what the vir library is for (it can be thought of as a (fairly) rudimentary "engine"). Nonetheless, the only graphics API currently supported by the vir library is OpenGL.

As of the latest release, ShaderThing requires a GPU supporting at least **OpenGL v3.3**, which is the case for virtually all consumer GPUs built in the last decade (as of early-mid 2024). Please consider, howerever, that OpenGL v4.3 or greater is recommended for access to the GIF exporter tool and all of the layer post-processing effects, since they are all based on compute shaders. If a version of OpenGL >=v4.3 is not available, the code will run as expected but said features will be unavailable.

# Repository structure

This repository consists of:
* the top-level ShaderThing app (in shaderthing/);
* the vir library (a high-level wrapper for graphics, window & input management, in vir/);
* a collection of third-party libraries (ranging from [GLFW](https://www.glfw.org/) to ImGui), compiled into a single one.

The vir and third-party libraries are statically linked to the final ShaderThing executable, making it stand-alone. 

The vir library (whose development both predates and partially led to the development of ShaderThing) currently only wraps OpenGL (for graphics management) and GLFW (for window & input management), but it is structured to possibly accomodate for other rendering and window/input-management platforms (though no such plans exist for the time being).

# How to compile

The code base (shaderthing-proper and the vir library) and related third-party libraries are entirely cross-platform and *should* compile on most platforms. Currently, only compilation on Windows 10 (Windows 10 Pro, build 19045) and Ubuntu 24.04 LTS (running on WSL) have been tested so far with the GNU GCC compiler (v12.2.0 for Windows, via [MinGW](https://www.mingw-w64.org/)). Regardless of the platform, the intended compilation approach is via [cmake](https://cmake.org). 

## On Windows 10

From within the root of this repository, from a terminal or PowerShell, run:

~~~
cmake -S shaderthing/ -B shaderthing/build -DCMAKE_BUILD_TYPE=Release
~~~

Depending on your cmake configuration (see [cmake-generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)), if you intend to use a MinGW-bundled GNU GCC compiler, it may be necessary to specify the MinGW generator (only if the previous command returns any generator-related errors) by running:

~~~
cmake -S shaderthing/ -B shaderthing/build -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles"
~~~

Once the build files have been built, compile by running:

~~~
cmake --build shaderthing/build --parallel N
~~~

Replace N with the desired number of cores to be used for the compilation. On reasonably modern hardware, it should take about a minute on 4 cores. The executable will then be located in shaderthing/build/shaderthing.exe.

## On Linux-based systems

The procedure is exactly the same as on Windows 10, tested on Ubuntu 24.04 LTS running on WSL.
