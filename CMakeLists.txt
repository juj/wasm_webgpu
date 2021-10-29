cmake_minimum_required(VERSION 3.10)

project(wasm_webgpu)

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
	message(STATUS "Enabling use of ccache from ${CCACHE_PROGRAM}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

macro(append_compiler_flags_debug FLAGS)
  set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${FLAGS}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${FLAGS}")
endmacro()

macro(append_linker_flags FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAGS}")
endmacro()

macro(append_linker_flags_debug FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${FLAGS}")
endmacro()

macro(append_linker_flags_opts FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} ${FLAGS}")
endmacro()

macro(append_linker_flags_minsizerel FLAGS)
  set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} ${FLAGS}")
endmacro()

if (EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	append_compiler_flags_debug("-g3")

	append_linker_flags_debug("-g3")
	append_linker_flags_debug("-s ASSERTIONS=1 -jsDWEBGPU_DEBUG=1")

	append_linker_flags("--js-library ${CMAKE_CURRENT_LIST_DIR}/lib/lib_webgpu.js")
	append_linker_flags("--js-library ${CMAKE_CURRENT_LIST_DIR}/lib/lib_demo.js")
	append_linker_flags("--js-library ${CMAKE_CURRENT_LIST_DIR}/lib/library_miniprintf.js")
	append_linker_flags("-s MINIMAL_RUNTIME=2 -s TEXTDECODER=2 -s ABORTING_MALLOC=0 -s SUPPORT_ERRNO=0")
	append_linker_flags("-s ENVIRONMENT=web -s MIN_FIREFOX_VERSION=87 -s MIN_SAFARI_VERSION=140000 -s MIN_EDGE_VERSION=90 -s MIN_CHROME_VERSION=90")
	append_linker_flags("--shell-file \"${CMAKE_SOURCE_DIR}/shell.html\"")
	append_linker_flags("-s TOTAL_STACK=16KB -s INITIAL_MEMORY=128KB")
	append_linker_flags("-s ALLOW_MEMORY_GROWTH=1")
	append_linker_flags("-s USE_SDL=0 -s FILESYSTEM=0 -s AUTO_JS_LIBRARIES=0 -s DISABLE_EXCEPTION_THROWING=1")
	append_linker_flags("-lmath.js -lhtml5.js -lint53.js")

	append_linker_flags("-s INITIAL_MEMORY=16MB -s MALLOC=emmalloc --memory-init-file 0")

	#append_linker_flags("-s MAXIMUM_MEMORY=4GB")

	# Manual debugging:
	#append_linker_flags("-g2 -s SINGLE_FILE=1")

	append_linker_flags_opts("--closure 1 --closure-args \"--externs ${CMAKE_CURRENT_LIST_DIR}/lib/webgpu_closure_externs.js\"")

	append_linker_flags_minsizerel("-Oz")
endif()

include_directories("${CMAKE_CURRENT_LIST_DIR}/lib")

file(GLOB_RECURSE lib lib/*.cpp)
add_library(webgpu STATIC ${lib})

add_executable(clear_screen clear_screen/clear_screen.c)
target_link_libraries(clear_screen webgpu)

add_executable(hello_triangle_minimal hello_triangle/hello_triangle_minimal.c)
target_link_libraries(hello_triangle_minimal webgpu)

add_executable(hello_triangle_verbose hello_triangle/hello_triangle_verbose.c)
target_link_libraries(hello_triangle_verbose webgpu)

add_executable(vertex_buffer vertex_buffer/vertex_buffer.c)
target_link_libraries(vertex_buffer webgpu)

add_executable(gpu_oom gpu_oom/gpu_oom.c)
target_link_libraries(gpu_oom webgpu)

add_executable(texture texture/texture.c)
target_link_libraries(texture webgpu)

add_executable(failing_shader_compilation failing_shader_compilation/failing_shader_compilation.c)
target_link_libraries(failing_shader_compilation webgpu)
