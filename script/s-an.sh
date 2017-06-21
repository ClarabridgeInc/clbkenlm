#! /bin/sh
shopt -s nullglob
# -analyze-headers
# -v -v
OPT='-x c++ -c -fPIC -DKENLM_MAX_ORDER=6 -std=c++0x -O0 -g -fno-omit-frame-pointer -I /data/wrk/clbkenlm/src/main/cpp -m64'

for f in src/main/cpp/*/*.cc
do
    echo "processing: $f"
    scan-build -analyze-headers clang++ $OPT $f
done
