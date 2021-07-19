#include "platform_detect.hpp" // copied from hostrpc
#include <stdint.h>
#include <stdio.h>

int setup();
int teardown();

#ifdef __HIP__
#include "hip/hip_runtime.h"

int setup() {
  hipError_t error = hipInit(0);
  if (error != hipSuccess) {
    printf("hipInit failed?\n");
    return 1;
  }

  return 0;
}

int teardown() { return 0; }

#endif

#ifdef _OPENMP
int setup() { return 0; }
int teardown() { return 0; }
#endif

#ifdef __HIP__
#define ANNOTATE() __attribute__((host, device))
#else
#define ANNOTATE()
#endif

ANNOTATE()
void test(uint32_t N, const char *src, char *dst);

#ifdef HOSTRPC_GPU

#ifdef _OPENMP

int target(uint32_t N, const char *src, char *dst) {
  // can't map void ?
#pragma omp target map(to : src [0:N]) map(from : dst [0:N])
  test(N, src, dst);

  return 0;
}

#endif

#ifdef __HIP__

__attribute__((global)) void target_hip_global(uint32_t N, const char *src,
                                               char *dst) {
  test(N, src, dst);
}

int target(uint32_t bytes, const char *src, char *dst) {

  struct hfi {
    hfi(char *ptr) : ptr(ptr) {}
    ~hfi() { hipFree(ptr); }
  private:
    char *ptr;    
  };

  char *gpu_src;
  char *gpu_dst;

  hipError_t error;

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
  
  hipLaunchKernelGGL(target_hip_global, dim3(blocks), dim3(threadsPerBlock), dynamic_shared,
                     stream, bytes, gpu_src, gpu_dst);

  error = hipMemcpy(dst, gpu_dst, bytes, hipMemcpyDeviceToHost);
  if (error != hipSuccess) {
    return 1;
  }

  return 0;
}

#endif

#endif

void host(uint32_t N, const char *src, char *dst) { test(N, src, dst); }

int main() {

  if (setup() != 0) {
    printf("setup failed\n");
    return 1;
  }

  uint32_t src[128];
  uint32_t dst[128];
  uint32_t res[128];
  uint32_t N = 128;

  for (uint32_t i = 0; i < N; i++) {
    src[i] = i;
  }

  uint32_t bytes = N * sizeof(uint32_t);

  host(bytes, (const char *)&src[0], (char *)&res[0]);

  int rc = target(bytes, (const char *)&src[0], (char *)&dst[0]);
  if (rc != 0) {
    printf("target offload failed\n");
    return 1;
  }

  uint32_t err = 0;
  for (uint32_t i = 0; i < N; i++) {
    if (dst[i] != res[i]) {
      err++;
      printf("error at %u, %u != %u\n", i, dst[i], res[i]);
    }
  }

  if (err != 0) {
    return 1;
  }

  printf("Ran successfully\n");

  if (teardown() != 0) {
    printf("teardown failed\n");
    return 1;
  }
  return 0;
}

ANNOTATE() // on the declaration is apparently not sufficient
void test(uint32_t N, const char *src, char *dst) {
  const uint32_t *s = (const uint32_t *)src;
  uint32_t *d = (uint32_t *)dst;

  for (uint32_t i = 0; i < N / sizeof(uint32_t); i++) {
    d[i] = 2 * s[i];
  }
}
