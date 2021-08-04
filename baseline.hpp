#ifndef BASELINE_HPP_INCLUDED
#define BASELINE_HPP_INCLUDED

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

#endif
