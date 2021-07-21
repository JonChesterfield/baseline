#ifndef TARGET_OPENMP_HPP_INCLUDED
#define TARGET_OPENMP_HPP_INCLUDED

#ifdef _OPENMP
// Define target() that runs on device under openmp
static int target(uint32_t N, const char *src, char *dst) {
  // can't map void ?
#pragma omp target map(to : src [0:N]) map(from : dst [0:N])
  test(N, src, dst);

  return 0;
}
#endif
#endif
