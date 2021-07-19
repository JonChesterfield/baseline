#!/bin/bash
set -x
set -e
set -o pipefail


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


$CLANGXX -O2 -fopenmp -fopenmp-targets=amdgcn-amd-amdhsa -Xopenmp-target=amdgcn-amd-amdhsa -march=$GFX example.cpp -o example.openmp

