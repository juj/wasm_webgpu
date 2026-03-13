// Verifies that wgpu_is_external_texture() returns true for a WGpuExternalTexture and false for a WGpuDevice.
// An HTMLVideoElement fed by a canvas captureStream() is used as the source so no network
// access is required. A RAF polling loop waits until the video has decoded its first frame
// (readyState >= HAVE_CURRENT_DATA) before importing.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice;
static WGpuObjectBase gVideoId;

WGPU_BOOL PollVideoReady(double time, void *userData)
{
  // HTMLMediaElement.HAVE_CURRENT_DATA == 2: the video has decoded at least one frame
  // and importExternalTexture() is permitted.
  if (!EM_ASM_INT({ return wgpuTestVideo && wgpuTestVideo.readyState >= 2; }))
    return WGPU_TRUE; // not ready yet, keep polling

  WGpuExternalTextureDescriptor desc = WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.source = gVideoId;
  WGpuExternalTexture ext = wgpu_device_import_external_texture(gDevice, &desc);

  assert(ext);
  assert(wgpu_is_external_texture(ext));
  assert(!wgpu_is_external_texture(gDevice)); // a device is not an external texture

  printf("Test OK\n");
  EM_ASM(window.close());
  return WGPU_FALSE; // stop polling
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;

  // Create a 4x4 canvas, draw a solid colour, and feed it to an HTMLVideoElement via
  // captureStream(). wgpuStore() registers the element in the wgpu[] JS object table
  // and returns the integer ID used as WGpuExternalTextureDescriptor.source.
  // The element is also kept as window.wgpuTestVideo for readyState polling.
  gVideoId = EM_ASM_INT({
    var canvas = document.createElement('canvas');
    canvas.width = canvas.height = 4;
    var ctx = canvas.getContext('2d');
    ctx.fillStyle = '#ff0000';
    ctx.fillRect(0, 0, 4, 4);

    var video = document.createElement('video');
    video.muted = true;
    video.srcObject = canvas.captureStream(25);
    video.play();

    window.wgpuTestVideo = video; // kept alive for readyState polling
    return wgpuStore(video);      // returns the integer wgpu[] table ID
  });

  wgpu_request_animation_frame_loop(PollVideoReady, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
