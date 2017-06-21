#! /bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:build/libs/clbkenlm/shared
#export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
ASAN_OPTIONS=symbolize=1 build/exe/ngq/ngq
