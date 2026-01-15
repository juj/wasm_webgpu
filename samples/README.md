# 🧪 Samples

To build the samples, first install Emscripten via [Emsdk](https://github.com/emscripten-core/emsdk), then enter Emsdk command line environment (`emsdk_env`), and type

```bash
cd path/to/wasm_webgpu
mkdir build
cd build
emcmake cmake ../samples -DCMAKE_BUILD_TYPE=Debug # Or MinSizeRel, RelWithDebInfo or Release
make -j
```

On Windows, the last `make` command is not available, so either install Mingw32-make via emsdk and run `mingw32-make -j`, or install Ninja via emsdk, then pass `-G Ninja` to the emcmake command line, and then run `ninja` instead of `make`.

### clear_screen

![clear_screen](./screenshots/clear_screen.png)

For the smallest Clear Screen "hello world" example, see [clear_screen.c/clear_screen.c](samples/clear_screen/clear_screen.c).

There is also an Emscripten JSPI-enabled variant of the same demo, at [clear_screen.c/clear_screen_sync.c](samples/clear_screen/clear_screen.c).

### failing_shader_compilation

The demo [failing_shader_compilation/failing_shader_compilation.c](samples/failing_shader_compilation/failing_shader_compilation.c) tests handling of shader compilation errors.

### gpu_oom

The demo [gpu_oom/gpu_oom.c](samples/gpu_oom/gpu_oom.c) exhausts the GPU VRAM, testing handling of GPU OOM events.

### hello_triangle

![hello_triangle](./screenshots/hello_triangle.png)

The demo [hello_triangle/hello_triangle_minimal.c](samples/hello_triangle/hello_triangle_minimal.c) contains the smallest triangle rendering demo.

The variant [hello_triangle/hello_triangle_verbose.c](samples/hello_triangle/hello_triangle_verbose.c) offers the same, but with verbose debug logging.

### offscreen_canvas

The demo [offscreen_canvas/offscreen_canvas.c](samples/offscreen_canvas/offscreen_canvas.c) shows how to perform WebGPU rendering using OffscreenCanvas from a Wasm Worker.

If you are using pthreads, the variant [offscreen_canvas/offscreen_canvas_pthread.c](samples/offscreen_canvas/offscreen_canvas_pthread.c) illustrates OffscreenCanvas rendering by using a pthread instead.

Finally, if you are using pthreads with the Emscripten `-sPROXY_TO_PTHREAD` build option, then check out the [offscreen_canvas/offscreen_canvas_proxy_to_pthread.c](samples/offscreen_canvas/offscreen_canvas_proxy_to_pthread.c) code sample.

### texture

![texture](./screenshots/texture.png)

The sample [texture/texture.c](samples/texture/texture.c) tests the `wgpu_load_image_bitmap_from_url_async()` API.

### vertex_buffer

![vertex_buffer](./screenshots/vertex_buffer.png)

The test [vertex_buffer/vertex_buffer.c](samples/vertex_buffer/vertex_buffer.c) shows an example of how to map a GPU buffer and use the function `wgpu_buffer_write_mapped_range()`.
