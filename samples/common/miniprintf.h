/*
 * Copyright 2014 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#pragma once

#include <emscripten/html5.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EM_STDIO_HANDLE int
#define EM_STDOUT ((EM_STDIO_HANDLE)1)
#define EM_STDERR ((EM_STDIO_HANDLE)2)

// Minimal fprintf() implementation that prints to either stdout or stderr streams.
// Pass in EM_STDOUT to print using console.error, and
// EM_STDERR to print using console.log. (the regular stdout/stderr streams do work too
// here, but they pull in extra size to the build that can be undesirable)
void emscripten_mini_stdio_fprintf(EM_STDIO_HANDLE outputStream, const char *format, ...);

// Minimal printf() implementation that prints to stdout (console.log). Identical to calling
// emscripten_mini_stdio_fprintf(EM_STDOUT, ...);
void emscripten_mini_stdio_printf(const char *format, ...);

#ifdef __cplusplus
} // ~extern "C"
#endif
