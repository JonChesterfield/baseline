#ifndef TARGET_HIP_HPP_INCLUDED
#define TARGET_HIP_HPP_INCLUDED

#ifdef __HIP__
#include "hip/hip_runtime.h"

// Define target() that runs on device under hip
__attribute__((global)) void target_hip_global(uint32_t N, const char *src,
                                               char *dst) {
  test(N, src, dst);
}

static int target(uint32_t bytes, const char *src, char *dst) {

  struct hfi {
    hfi(char *ptr) : ptr(ptr) {}
    ~hfi() { hipFree(ptr); }

  private:
    char *ptr;
  };

  char *gpu_src;
  char *gpu_dst;

  hipError_t error;

  error = hipInit(0);
  if (error != hipSuccess) {
    return 1;
  }

  error = hipMalloc(&gpu_src, bytes);
  if (error != hipSuccess) {
    return 1;
  }
  hfi free_src(gpu_src);

  error = hipMalloc(&gpu_dst, bytes);
  if (error != hipSuccess) {
    return 1;
  }
  hfi free_dst(gpu_dst);

  error = hipMemcpy(gpu_src, src, bytes, hipMemcpyHostToDevice);
  if (error != hipSuccess) {
    return 1;
  }

  const unsigned blocks = 1;
  const unsigned threadsPerBlock = 64;
  size_t dynamic_shared = 0;
  hipStream_t stream = 0;

  hipLaunchKernelGGL(target_hip_global, dim3(blocks), dim3(threadsPerBlock),
                     dynamic_shared, stream, bytes, gpu_src, gpu_dst);

  error = hipMemcpy(dst, gpu_dst, bytes, hipMemcpyDeviceToHost);
  if (error != hipSuccess) {
    return 1;
  }

  return 0;
}


#endif


#endif
