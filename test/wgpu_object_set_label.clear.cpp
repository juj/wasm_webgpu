// Verifies that setting a label with wgpu_object_set_label() and then clearing it with an empty string results in wgpu_object_get_label() returning 0 with an empty string.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBufferDescriptor desc = {
    .size = 64,
    .usage = WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // Set a label then clear it with an empty string
  wgpu_object_set_label(buffer, "my-buffer");

  // Saving label does not work in Firefox: https://bugzilla.mozilla.org/show_bug.cgi?id=2023421
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    char label[64];
    int n = wgpu_object_get_label(buffer, label, sizeof(label));
    assert(n > 0);
    assert(strcmp(label, "my-buffer") == 0);

    // Clear the label with ""
    wgpu_object_set_label(buffer, "");
    n = wgpu_object_get_label(buffer, label, sizeof(label));
    // After clearing, label should be empty string
    assert(n == 0);
    assert(label[0] == '\0');
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
