#include "baseline.hpp"

#ifndef __OPENCL_VERSION__

#include <stdint.h>

#include "target_hip.hpp"
#include "target_opencl.hpp"
#include "target_openmp.hpp"

void host(unsigned N, GLOBAL() const char *src, GLOBAL() char *dst) {
  test(N, src, dst);
}

#include "util.hpp"
#include <stdio.h>

int main() {

  uint32_t N = 16;
  uint32_t src[N];

  for (uint32_t i = 0; i < N; i++) {
    src[i] = i;
  }

  uint32_t bytes = N * sizeof(uint32_t);

  int rc = compare(__FILE__, host, target, bytes, (const char *)&src[0]);
  if (rc == 0) {
    printf("Ran successfully\n");
  }

  return rc;
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
