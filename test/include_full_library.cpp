// Verifies that a program that uses lib_webgpu.js can be linked with -sINCLUDE_FULL_LIBRARY,
// which makes Emscripten include every JS library function in the build (-sMAIN_MODULE=1 does
// the same). That requires each function in lib_webgpu.js to have its __deps available in every
// build mode, so the functions that depend on symbols that exist only with JSPI/ASYNCIFY
// ($wgpu_async), with Wasm Workers ($_wasmWorkers) or with pthreads ($PThread) must be defined
// only in the build modes that have those symbols. This test builds without any of the three.
// flags: -sINCLUDE_FULL_LIBRARY

#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  assert(navigator_gpu_available());

  EM_ASM(window.close());
}
