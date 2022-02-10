#!/bin/bash

exec=cpu_tests/graph500/cc
input="-s 6 -e 3"
outdir=m5out
cpu=MinorCPU
debug_flags="BFS"

for t in "alloc"
do
    build/ARM/gem5.opt --debug-flags=$debug_flags --debug-file=out configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=$cpu  --prefetch-type=$t --prefetch-distance=16
    mv m5out/stats.txt $outdir/$t
done

