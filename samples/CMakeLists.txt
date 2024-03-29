cmake_minimum_required(VERSION 3.20)

project(wasm_webgpu)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE "Debug") # Default to Debug build if not specified
endif()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
  message(STATUS "Enabling use of ccache from ${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

macro(append_compiler_flags FLAGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
endmacro()

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

cmake_path(SET LIB_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/../lib")
cmake_path(SET COMMON_DIR NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/common")

if (EMSCRIPTEN)
	set(CMAKE_EXECUTABLE_SUFFIX ".html")

	append_compiler_flags("-Wall")
	append_compiler_flags_debug("-g3")

	append_linker_flags("-Wall")
	append_linker_flags_debug("-g3")
	append_linker_flags_debug("-sASSERTIONS=1 -jsDWEBGPU_DEBUG=1")

	append_linker_flags("--js-library ${LIB_DIR}/lib_webgpu.js")
	append_linker_flags("--js-library ${COMMON_DIR}/lib_demo.js")
	append_linker_flags("--js-library ${COMMON_DIR}/library_miniprintf.js")
	append_linker_flags("-sMINIMAL_RUNTIME=2 -sTEXTDECODER=2 -sABORTING_MALLOC=0")
	append_linker_flags("-sENVIRONMENT=web -sMIN_FIREFOX_VERSION=87 -sMIN_SAFARI_VERSION=140000 -sMIN_CHROME_VERSION=90")
	append_linker_flags("--shell-file \"${CMAKE_SOURCE_DIR}/shell.html\"")
	append_linker_flags("-sTOTAL_STACK=16KB -sINITIAL_MEMORY=128KB")
	append_linker_flags("-sALLOW_MEMORY_GROWTH=1")
	append_linker_flags("-sUSE_SDL=0 -sFILESYSTEM=0 -sAUTO_JS_LIBRARIES=0 -sDISABLE_EXCEPTION_THROWING=1")
	append_linker_flags("-lmath.js -lhtml5.js -lint53.js")
	append_linker_flags("--closure-args=--externs=${LIB_DIR}/webgpu-closure-externs.js")

	append_linker_flags("-sINITIAL_MEMORY=16MB -sMALLOC=emmalloc")

	#append_linker_flags("-sMAXIMUM_MEMORY=4GB")

	# Manual debugging:
	#append_linker_flags("-g2 -sSINGLE_FILE=1")

	append_linker_flags_opts("--closure 1")

	append_linker_flags_minsizerel("-Oz")
endif()

include_directories("${LIB_DIR}")
include_directories("${COMMON_DIR}")

add_library(webgpu STATIC ${LIB_DIR}/lib_webgpu.cpp ${LIB_DIR}/lib_webgpu_cpp20.cpp)

add_executable(clear_screen clear_screen/clear_screen.c)
target_link_libraries(clear_screen webgpu)

add_executable(clear_screen_sync clear_screen/clear_screen_sync.c)
target_link_libraries(clear_screen_sync webgpu)

# Enable ASYNCIFY for clear_screen_sync example.
target_link_options(clear_screen_sync PRIVATE "-sASYNCIFY=1")

add_executable(buffer_map_sync buffer_map_sync/buffer_map_sync.c)
target_link_libraries(buffer_map_sync webgpu)

# Enable JSPI (ASYNCIFY=2) for buffer_map_sync example
target_link_options(buffer_map_sync PRIVATE "-sASYNCIFY=2")

# Currently ASYNCIFY does not support MINIMAL_RUNTIME, so have to disable it for this example.
target_link_options(clear_screen_sync PRIVATE "-sMINIMAL_RUNTIME=0")
target_link_options(clear_screen_sync PRIVATE "--shell-file=${EMSCRIPTEN_ROOT_PATH}/src/shell.html")
target_link_options(clear_screen_sync PRIVATE "-jsDWEBGPU_NO_BW_COMPAT=1") # Test that no backwards compatibility variant builds ok.

target_link_options(buffer_map_sync PRIVATE "-sMINIMAL_RUNTIME=0")
target_link_options(buffer_map_sync PRIVATE "--shell-file=${EMSCRIPTEN_ROOT_PATH}/src/shell.html")

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
