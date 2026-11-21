// Verifies that a very long GPUError message does not overflow the Wasm stack when it is
// delivered to a WGpuDeviceErrorCallback. Browsers echo the offending source line in shader
// validation errors, so e.g. a minified single-line WGSL shader easily produces an error message
// that is larger than the default 64 KB Wasm stack. The library marshals the message via the Wasm
// stack, and truncates messages that are longer than 32767 bytes (plus a null terminator).
// To make the message length deterministic across browsers, a synthetic GPUUncapturedErrorEvent
// that carries a GPUValidationError with a known message is dispatched on the device.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_MESSAGE_BYTES 32767

struct TestCase
{
  int numChars;      // Number of characters in the error message that is sent.
  int bytesPerChar;  // 1: message consists of 'x' characters, 2: message consists of U+00E4 characters.
  int expectedBytes; // Number of bytes that the callback is expected to receive, excluding the null terminator.
};

static const TestCase kTests[] = {
  {    100, 1,               100 }, // Short messages arrive intact.
  {  32767, 1, MAX_MESSAGE_BYTES }, // The longest message that still arrives intact.
  {  32768, 1, MAX_MESSAGE_BYTES }, // One byte too long: truncated.
  {  70000, 1, MAX_MESSAGE_BYTES }, // Slightly larger than the default Wasm stack.
  { 200000, 1, MAX_MESSAGE_BYTES }, // Far larger than the default Wasm stack.
  {  20000, 2,             32766 }, // Truncation does not split a multibyte UTF-8 sequence.
};
#define NUM_TESTS ((int)(sizeof(kTests)/sizeof(kTests[0])))

static int testIndex = 0;
static int numCallbackCalls = 0;
static WGpuDevice gDevice;

// Static data that is located next to the stack in linear memory. In optimized builds these
// guard words are the first thing to be overwritten if the message is written below the stack.
static uint32_t guard[4096];

void UncapturedError(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  assert(device == gDevice);
  assert(userData == (void*)42);
  assert(errorType == WGPU_ERROR_TYPE_VALIDATION);
  assert(errorMessage);

  const TestCase &t = kTests[testIndex];
  size_t len = strlen(errorMessage);
  printf("Received error message of %d bytes (sent %d bytes)\n", (int)len, t.numChars * t.bytesPerChar);
  assert(len == (size_t)t.expectedBytes);

  // The delivered message must be an uncorrupted prefix of the original message.
  if (t.bytesPerChar == 1)
  {
    for(size_t i = 0; i < len; ++i)
      assert(errorMessage[i] == 'x');
  }
  else
  {
    assert(len % 2 == 0);
    for(size_t i = 0; i < len; i += 2)
    {
      assert((uint8_t)errorMessage[i] == 0xC3);
      assert((uint8_t)errorMessage[i+1] == 0xA4);
    }
  }

  for(int i = 0; i < 4096; ++i)
    assert(guard[i] == 0xDEADBEEFu);

  ++numCallbackCalls;
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  for(int i = 0; i < 4096; ++i)
    guard[i] = 0xDEADBEEFu;

  wgpu_device_set_uncapturederror_callback(device, UncapturedError, (void*)42);

  for(testIndex = 0; testIndex < NUM_TESTS; ++testIndex)
  {
    // dispatchEvent() invokes the uncapturederror handler synchronously.
    EM_ASM({
      wgpu[$0].dispatchEvent(new GPUUncapturedErrorEvent('uncapturederror', { 'error': new GPUValidationError(String.fromCharCode($2 == 1 ? 120 : 228).repeat($1)) }));
    }, device, kTests[testIndex].numChars, kTests[testIndex].bytesPerChar);

    assert(numCallbackCalls == testIndex + 1);
  }

  printf("Test OK\n");
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
