@mkdir out >NUL 2>&1
emcc hello_triangle\hello_triangle.c -o out\hello_triangle.html --js-library lib\lib_webgpu.js lib\lib_webgpu.cpp -Ilib -s MINIMAL_RUNTIME=1 -O3 -g2 -s ASSERTIONS=1 -s SINGLE_FILE=1 -s ENVIRONMENT=web
