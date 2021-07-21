#ifndef TARGET_OPENCL_HPP_INCLUDED
#define TARGET_OPENCL_HPP_INCLUDED

#ifdef __OPENCL__
#include "CL/cl.h"
#include "CL/cl_ext.h"
#include <stdio.h>

#include "util.hpp"

#define CaseReturnString(x)                                                    \
  case x:                                                                      \
    return #x

static const char *opencl_errstr(cl_int err) {
  switch (err) {
    CaseReturnString(CL_SUCCESS);
    CaseReturnString(CL_DEVICE_NOT_FOUND);
    CaseReturnString(CL_DEVICE_NOT_AVAILABLE);
    CaseReturnString(CL_COMPILER_NOT_AVAILABLE);
    CaseReturnString(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    CaseReturnString(CL_OUT_OF_RESOURCES);
    CaseReturnString(CL_OUT_OF_HOST_MEMORY);
    CaseReturnString(CL_PROFILING_INFO_NOT_AVAILABLE);
    CaseReturnString(CL_MEM_COPY_OVERLAP);
    CaseReturnString(CL_IMAGE_FORMAT_MISMATCH);
    CaseReturnString(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    CaseReturnString(CL_BUILD_PROGRAM_FAILURE);
    CaseReturnString(CL_MAP_FAILURE);
    CaseReturnString(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    CaseReturnString(CL_COMPILE_PROGRAM_FAILURE);
    CaseReturnString(CL_LINKER_NOT_AVAILABLE);
    CaseReturnString(CL_LINK_PROGRAM_FAILURE);
    CaseReturnString(CL_DEVICE_PARTITION_FAILED);
    CaseReturnString(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
    CaseReturnString(CL_INVALID_VALUE);
    CaseReturnString(CL_INVALID_DEVICE_TYPE);
    CaseReturnString(CL_INVALID_PLATFORM);
    CaseReturnString(CL_INVALID_DEVICE);
    CaseReturnString(CL_INVALID_CONTEXT);
    CaseReturnString(CL_INVALID_QUEUE_PROPERTIES);
    CaseReturnString(CL_INVALID_COMMAND_QUEUE);
    CaseReturnString(CL_INVALID_HOST_PTR);
    CaseReturnString(CL_INVALID_MEM_OBJECT);
    CaseReturnString(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    CaseReturnString(CL_INVALID_IMAGE_SIZE);
    CaseReturnString(CL_INVALID_SAMPLER);
    CaseReturnString(CL_INVALID_BINARY);
    CaseReturnString(CL_INVALID_BUILD_OPTIONS);
    CaseReturnString(CL_INVALID_PROGRAM);
    CaseReturnString(CL_INVALID_PROGRAM_EXECUTABLE);
    CaseReturnString(CL_INVALID_KERNEL_NAME);
    CaseReturnString(CL_INVALID_KERNEL_DEFINITION);
    CaseReturnString(CL_INVALID_KERNEL);
    CaseReturnString(CL_INVALID_ARG_INDEX);
    CaseReturnString(CL_INVALID_ARG_VALUE);
    CaseReturnString(CL_INVALID_ARG_SIZE);
    CaseReturnString(CL_INVALID_KERNEL_ARGS);
    CaseReturnString(CL_INVALID_WORK_DIMENSION);
    CaseReturnString(CL_INVALID_WORK_GROUP_SIZE);
    CaseReturnString(CL_INVALID_WORK_ITEM_SIZE);
    CaseReturnString(CL_INVALID_GLOBAL_OFFSET);
    CaseReturnString(CL_INVALID_EVENT_WAIT_LIST);
    CaseReturnString(CL_INVALID_EVENT);
    CaseReturnString(CL_INVALID_OPERATION);
    CaseReturnString(CL_INVALID_GL_OBJECT);
    CaseReturnString(CL_INVALID_BUFFER_SIZE);
    CaseReturnString(CL_INVALID_MIP_LEVEL);
    CaseReturnString(CL_INVALID_GLOBAL_WORK_SIZE);
    CaseReturnString(CL_INVALID_PROPERTY);
    CaseReturnString(CL_INVALID_IMAGE_DESCRIPTOR);
    CaseReturnString(CL_INVALID_COMPILER_OPTIONS);
    CaseReturnString(CL_INVALID_LINKER_OPTIONS);
    CaseReturnString(CL_INVALID_DEVICE_PARTITION_COUNT);
    CaseReturnString(CL_PLATFORM_NOT_FOUND_KHR);
  default:
    return "Unknown OpenCL error code";
  }
}

const char *kernel_source =
    "#ifndef __OPENCL_VERSION__\n"
    "#error \"Expected OPENCL_VERSION define\"\n"
    "#endif\n"
    "void test(unsigned N, global const char *src, global char *dst);\n"
    "kernel void target_opencl_kernel(unsigned N, global const char *src, "
    "global char *dst) {\n"
    "   test(N, src, dst);\n"
    "}\n";

#define ERRRET()                                                               \
  if (err != CL_SUCCESS) {                                                     \
    printf("opencl target L:%u failed %d [0x%x] (%s)\n", __LINE__, err, err,   \
           opencl_errstr(err));                                                \
    return 1;                                                                  \
  }

static int target(const char *srcfile, uint32_t bytes, const char *src,
                  char *dst) {
  // copying from
  // https://rocmdocs.amd.com/en/latest/Programming_Guides/Opencl-programming-guide.html#build-run-opencl

  // 1. Get a platform.
  cl_platform_id platform;
  cl_int err;

  err = clGetPlatformIDs(1, &platform, NULL);
  ERRRET()

  // 2. Find a gpu device.
  cl_device_id device;
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
  ERRRET()

  // 3. Create a context and command queue on that device.
  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  ERRRET()

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
  ERRRET()

  char *f = from_file(srcfile);
  struct onexit_ {
    onexit_(char *f) : f(f) {}
    ~onexit_() { free(f); }
    char *f;
  } onexit(f);

  if (!f) {
    printf("opencl failed to read file %s\n", srcfile);
    return 1;
  }

  const char *sources[] = {
      kernel_source,
      f,
  };
  size_t Nsources = sizeof(sources) / sizeof(sources[0]);

  printf("sources[1] %s\n", sources[1]);

  cl_program program =
      clCreateProgramWithSource(context, Nsources, sources, NULL, &err);
  ERRRET()

  err = clBuildProgram(program, 1, &device, "-I.", NULL, NULL);

  if (err == CL_BUILD_PROGRAM_FAILURE) {
    // Determine the size of the log
    size_t log_size;
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL,
                                &log_size);
    ERRRET()

    // Allocate memory for the log
    char *log = (char *)malloc(log_size);

    // Get the log
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size,
                                log, NULL);
    ERRRET()
    // Print the log
    printf("%s\n", log);

    free(log);
    err = CL_BUILD_PROGRAM_FAILURE;
  }

  ERRRET()

  cl_kernel kernel = clCreateKernel(program, "target_opencl_kernel", &err);
  ERRRET()
  // 5. Create a data buffer.
  cl_mem dst_buffer =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, bytes, NULL, NULL);

  cl_mem src_buffer =
      clCreateBuffer(context, CL_MEM_READ_ONLY, bytes, NULL, NULL);

  // 6. Launch the kernel. Let OpenCL pick the local work size.
  size_t global_work_size = 64;
  clSetKernelArg(kernel, 0, sizeof(bytes), (void *)&bytes);
  clSetKernelArg(kernel, 1, sizeof(src_buffer), (void *)&src_buffer);
  clSetKernelArg(kernel, 2, sizeof(dst_buffer), (void *)&dst_buffer);

  err = clEnqueueWriteBuffer(queue, src_buffer, CL_TRUE, 0, bytes, src, 0, NULL,
                             NULL);
  ERRRET()

  err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_work_size, NULL,
                               0, NULL, NULL);
  ERRRET()

  // 7. Look at the results via synchronous buffer map.
  err = clEnqueueReadBuffer(queue, dst_buffer, CL_TRUE, 0, bytes, dst, 0, NULL,
                            NULL);
  ERRRET()

  clFinish(queue);

  return 0;
}

#endif

#endif
