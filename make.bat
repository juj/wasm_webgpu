@mkdir out >NUL 2>&1
@set ARGS=--js-library lib\lib_webgpu.js lib\lib_webgpu.cpp -Ilib -s MINIMAL_RUNTIME=2 -O3 -s ENVIRONMENT=web -s DYNCALLS=1 -s DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=["$dynCall"] -s TEXTDECODER=2 -s SUPPORT_ERRNO=0

call emcc hello_triangle\hello_triangle_verbose.c -o out\hello_triangle_verbose.html %ARGS% -g2 -s ASSERTIONS=1
call emcc hello_triangle\hello_triangle_minimal.c -o out\hello_triangle_minimal.html %ARGS% -DNDEBUG --closure 1 --closure-args "--externs lib/webgpu_closure_externs.js"
