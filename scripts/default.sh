#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 18 -e 10"
outdir=home/cc_20_10/default

mkdir -p $outdir
for t in "none" "base" "alloc" "noalloc"
do
    build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t -r 1
    mv m5out/stats.txt $outdir/$t
done

