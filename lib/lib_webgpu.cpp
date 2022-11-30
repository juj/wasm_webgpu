#include "lib_webgpu.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE type)
{
  return type == WGPU_COMPILATION_MESSAGE_TYPE_WARNING ? "warning"
        : (type == WGPU_COMPILATION_MESSAGE_TYPE_INFO ? "info" : "error");
}

#ifdef __cplusplus
} // ~extern "C"
#endif
