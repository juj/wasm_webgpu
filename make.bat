@mkdir out >NUL 2>&1

@set ARGS_DEBUG_DEEP=-O0 -g2 -s ASSERTIONS=1
@set ARGS_DEBUG_OPTIMIZED=-DNDEBUG -O3 -g2 -s WASM=0
@set ARGS_RELEASE=-DNDEBUG --closure 0 --closure-args "--externs lib/webgpu_closure_externs.js" -O3

@set ARGS=--js-library lib\lib_webgpu.js lib\lib_webgpu.cpp -Ilib -s MINIMAL_RUNTIME=2 -s ENVIRONMENT=web -s TEXTDECODER=2 -s SUPPORT_ERRNO=0 --shell-file shell.html -s TOTAL_STACK=16KB -s INITIAL_MEMORY=128KB

call emcc hello_triangle\hello_triangle_verbose.c -o out\hello_triangle_verbose.html %ARGS% %ARGS_DEBUG%
call emcc hello_triangle\hello_triangle_minimal.c -o out\hello_triangle_minimal.html %ARGS% %ARGS_RELEASE%
call emcc clear_screen\clear_screen.c -o out\clear_screen.html %ARGS% %ARGS_DEBUG_OPTIMIZED%
