#ifdef __OPENCL_VERSION__
  #define ANNOTATE()
  #error "Compiling for opencl device code"

#if defined( _OPENMP ) || defined(__HIP__)
#error "Unexpected language macro within opencl region"
#endif
#endif



#include <stdint.h>
#include <stdio.h>

#ifdef __HIP__
#include "hip/hip_runtime.h"
#endif

#ifdef __HIP__
#define ANNOTATE() __attribute__((host, device))
#else
#define ANNOTATE()
#endif

ANNOTATE()
extern "C" void test(uint32_t N, const char *src, char *dst);

#include "target_openmp.hpp"
#include "target_hip.hpp"
#include "target_opencl.hpp"

void host(uint32_t N, const char *src, char *dst) { test(N, src, dst); }

int main() {

  constexpr uint32_t N = 16;
  uint32_t src[N];
  uint32_t dst[N];
  uint32_t res[N];

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

  return 0;
}

ANNOTATE() // on the declaration is apparently not sufficient
void test(uint32_t N, const char *src, char *dst) {
  const uint32_t *s = (const uint32_t *)src;
  uint32_t *d = (uint32_t *)dst;

  for (uint32_t i = 0; i < N / sizeof(uint32_t); i++) {
    d[i] = 2 * __builtin_popcount(s[i]);
  }
}
