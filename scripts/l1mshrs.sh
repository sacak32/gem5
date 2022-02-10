#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 16 -e 10"
outdir=home/sssp_18_10/l1mshrs

mkdir -p $outdir
for a in 4 8 16 32 64
do
    for t in "noalloc"
    do
        build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t --l1_mshrs=$a -r 1
        mv m5out/stats.txt $outdir/$t\_$a
    done
done

