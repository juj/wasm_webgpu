// Verifies that wgpu_object_get_label() returns the number of bytes written, correctly copies the label string, and truncates safely when the destination buffer is too small.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBufferDescriptor bdesc = {
    .size = 64,
    .usage = WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bdesc);
  assert(buffer);

  // Set a label and verify the integer return value of get_label
  const char *testLabel = "hello-label";
  wgpu_object_set_label(buffer, testLabel);

  // Saving label does not work in Firefox: https://bugzilla.mozilla.org/show_bug.cgi?id=2023421
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    char dst[64];
    int n = wgpu_object_get_label(buffer, dst, sizeof(dst));
    // Return value should be the number of bytes written (excluding null terminator)
    assert(n == (int)strlen(testLabel));
    assert(strcmp(dst, testLabel) == 0);

    // Test with a truncating buffer (4 bytes: "hel\0")
    char small[4];
    int m = wgpu_object_get_label(buffer, small, sizeof(small));
    // Truncated return: at most sizeof(small)-1 chars
    assert(m <= (int)(sizeof(small) - 1));
    assert(small[sizeof(small)-1] == '\0');
  }

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
