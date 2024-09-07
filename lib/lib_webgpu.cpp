#include "lib_webgpu.h"

extern "C" {

const char *wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE type)
{
  return type == WGPU_COMPILATION_MESSAGE_TYPE_WARNING ? "warning"
        : (type == WGPU_COMPILATION_MESSAGE_TYPE_INFO ? "info" : "error");
}

} // ~extern "C"
