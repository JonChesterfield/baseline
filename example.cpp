#ifdef __HIP__
#define ANNOTATE() __attribute__((host, device))
#else
#define ANNOTATE()
#endif

#ifdef __OPENCL_VERSION__
#define GLOBAL() global
#else
#define GLOBAL()
#endif

ANNOTATE()
#ifdef _cplusplus
extern "C"
#endif
    void
    test(unsigned N, GLOBAL() const char *src, GLOBAL() char *dst);

#ifdef __OPENCL_VERSION__

#if defined(_OPENMP) || defined(__HIP__)
#error "Unexpected language macro within opencl region"
#endif

#endif

#ifndef __OPENCL_VERSION__

#include <stdint.h>

// TODO: Manage this via headers
inline const char *SRCFILE() { return __FILE__; }

#include "target_hip.hpp"
#include "target_opencl.hpp"
#include "target_openmp.hpp"

void host(unsigned N, GLOBAL() const char *src, GLOBAL() char *dst) {
  test(N, src, dst);
}

#include <stdio.h>

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

#endif

ANNOTATE() // on the declaration is apparently not sufficient
void test(unsigned N, GLOBAL() const char *src, GLOBAL() char *dst) {
  GLOBAL() const unsigned *s = (GLOBAL() const unsigned *)src;
  GLOBAL() unsigned *d = (GLOBAL() unsigned *)dst;

  for (unsigned i = 0; i < N / sizeof(unsigned); i++) {
    d[i] = 2 * __builtin_popcount(s[i]);
  }
}
