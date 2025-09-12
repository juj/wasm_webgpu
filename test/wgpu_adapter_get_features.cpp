// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGPU_FEATURES_BITFIELD features = wgpu_adapter_get_features(adapter);

  assert(features != 0); // Surely each implementation supports some features at least?
  assert(features >= 0);
  assert(features < WGPU_FEATURE_FIRST_UNUSED_BIT);

  printf("Supported features:\n");
  if ((features & WGPU_FEATURE_DEPTH_CLIP_CONTROL                ) != 0) printf("WGPU_FEATURE_DEPTH_CLIP_CONTROL                \n");
  if ((features & WGPU_FEATURE_DEPTH32FLOAT_STENCIL8             ) != 0) printf("WGPU_FEATURE_DEPTH32FLOAT_STENCIL8             \n");
  if ((features & WGPU_FEATURE_TEXTURE_COMPRESSION_BC            ) != 0) printf("WGPU_FEATURE_TEXTURE_COMPRESSION_BC            \n");
  if ((features & WGPU_FEATURE_TEXTURE_COMPRESSION_BC_SLICED_3D  ) != 0) printf("WGPU_FEATURE_TEXTURE_COMPRESSION_BC_SLICED_3D  \n");
  if ((features & WGPU_FEATURE_TEXTURE_COMPRESSION_ETC2          ) != 0) printf("WGPU_FEATURE_TEXTURE_COMPRESSION_ETC2          \n");
  if ((features & WGPU_FEATURE_TEXTURE_COMPRESSION_ASTC          ) != 0) printf("WGPU_FEATURE_TEXTURE_COMPRESSION_ASTC          \n");
  if ((features & WGPU_FEATURE_TEXTURE_COMPRESSION_ASTC_SLICED_3D) != 0) printf("WGPU_FEATURE_TEXTURE_COMPRESSION_ASTC_SLICED_3D\n");
  if ((features & WGPU_FEATURE_TIMESTAMP_QUERY                   ) != 0) printf("WGPU_FEATURE_TIMESTAMP_QUERY                   \n");
  if ((features & WGPU_FEATURE_INDIRECT_FIRST_INSTANCE           ) != 0) printf("WGPU_FEATURE_INDIRECT_FIRST_INSTANCE           \n");
  if ((features & WGPU_FEATURE_SHADER_F16                        ) != 0) printf("WGPU_FEATURE_SHADER_F16                        \n");
  if ((features & WGPU_FEATURE_RG11B10UFLOAT_RENDERABLE          ) != 0) printf("WGPU_FEATURE_RG11B10UFLOAT_RENDERABLE          \n");
  if ((features & WGPU_FEATURE_BGRA8UNORM_STORAGE                ) != 0) printf("WGPU_FEATURE_BGRA8UNORM_STORAGE                \n");
  if ((features & WGPU_FEATURE_FLOAT32_FILTERABLE                ) != 0) printf("WGPU_FEATURE_FLOAT32_FILTERABLE                \n");
  if ((features & WGPU_FEATURE_FLOAT32_BLENDABLE                 ) != 0) printf("WGPU_FEATURE_FLOAT32_BLENDABLE                 \n");
  if ((features & WGPU_FEATURE_CLIP_DISTANCES                    ) != 0) printf("WGPU_FEATURE_CLIP_DISTANCES                    \n");
  if ((features & WGPU_FEATURE_DUAL_SOURCE_BLENDING              ) != 0) printf("WGPU_FEATURE_DUAL_SOURCE_BLENDING              \n");
  if ((features & WGPU_FEATURE_SUBGROUPS                         ) != 0) printf("WGPU_FEATURE_SUBGROUPS                         \n");
  if ((features & WGPU_FEATURE_TEXTURE_FORMATS_TIER1             ) != 0) printf("WGPU_FEATURE_TEXTURE_FORMATS_TIER1             \n");

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
