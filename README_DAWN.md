# Using wasm_webgpu with Dawn

It is also possible to target WebGPU via Dawn with the wasm_webgpu headers. This can be useful when you are developing an application that primarily targets the web, but you would still like to leverage native C/C++ debuggers to better see what is going on.

# Building Dawn on Windows

Google does not ship precompiled releases of Dawn libraries for any platform, but one must build Dawn from source. See Dawn build instructions at Dawn repository: [docs/building.md](https://dawn.googlesource.com/dawn/+/refs/heads/main/docs/building.md).

The instructions here detail the necessary steps for building Dawn on Windows:

1. Download Google's [depot_tools.zip](https://storage.googleapis.com/chrome-infra/depot_tools.zip), uncompress it to e.g. `C:\depot_tools`. (if you encounter errors, see [Google depot_tools tutorial](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)).
2. Install [CMake](https://cmake.org/download/) if you don't have it yet. Make sure to add CMake to PATH when prompted during installation.
3. Download [Ninja build system](https://github.com/ninja-build/ninja/releases) if you don't have it yet. Unzip and add somewhere to PATH.
4. `git clone https://dawn.googlesource.com/dawn` somewhere you'd like.
5. In a Visual Studio 2019 x64 Developer Command Prompt, run
```
cd dawn
copy scripts/standalone.gclient .gclient
set DEPOT_TOOLS_WIN_TOOLCHAIN=0
gclient sync
mkdir out\Debug
cd out\Debug
set PATH=C:\depot_tools;%PATH%
cmake -G Ninja ..\..
ninja
```

When doing so, also compile the dawn-specific file with your project:

 - [lib/lib_webgpu_dawn.cpp](https://github.com/juj/wasm_webgpu/blob/main/lib/lib_webgpu_dawn.cpp)

TODO: add more instructions about targeting Dawn natively.

# Building wasm_webgpu samples against Dawn on Windows

1. Run on Windows terminal

```
cd wasm_webgpu
mkdir build_dawn
cd build_dawn
cmake ..\samples -DDAWN=path/to/dawn/out/Debug
```
then open Visual Studio to build and run.
