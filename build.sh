#!/bin/bash
set -x
set -e
set -o pipefail

for i in example.cpp *.hpp; do
    clang-format -i $i
done

if true; then
    # Aomp
    RDIR=$HOME/rocm/aomp
    GFX=`$RDIR/bin/mygpu -d gfx906` # lost the entry for gfx750 at some point
else
    # trunk
    RDIR=$HOME/llvm-install
    GFX=`$RDIR/bin/amdgpu-arch | uniq`
fi

echo "Using RDIR $RDIR"
echo "For GFX $GFX"


CLANG="$RDIR/bin/clang"
CLANGXX="$RDIR/bin/clang++"
LLC="$RDIR/bin/llc"
DIS="$RDIR/bin/llvm-dis"
LINK="$RDIR/bin/llvm-link"
OPT="$RDIR/bin/opt"


$CLANGXX -std=c++11 -O2 -fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=$GFX example.cpp -o example.openmp

$CLANGXX -std=c++11 -x hip example.cpp --offload-arch=$GFX -L$RDIR/lib -Wl,-rpath=$RDIR/lib  -lamdhip64  -o example.hip

# $CLANGXX -x cl -Xclang -cl-std=clc++ -Dcl_khr_fp64 -Dcl_khr_fp16 -Dcl_khr_subgroups -Dcl_khr_int64_base_atomics -Dcl_khr_int64_extended_atomics -D__OPENCL__ -D__OPENCL_C_VERSION__=200 example.cpp -o example.opencl

echo libamdocl64.so > amdocl64.icd
# trailing / is necessary, as loader/linux/icd_linux just pastes the contents of that file onto the evar
# it would not be necessary with the icd_linux source from khronos
export OCL_ICD_VENDORS=`pwd`/

# 220 is the default, according to a message in cl_version.h
$CLANGXX -std=c++11  -D__OPENCL__ -D__OPENCL_C_VERSION__=220 -DCL_TARGET_OPENCL_VERSION=220 example.cpp -I$RDIR/include/ -o example.opencl -Wno-deprecated-declarations -L$RDIR/lib -Wl,-rpath=$RDIR/lib -lOpenCL 

./example.opencl
