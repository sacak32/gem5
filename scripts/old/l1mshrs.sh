#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 16 -e 10"
outdir=home/pr_16_10/l1mshrs

for a in 4 8 16 32
do
    for t in "none" "base" "alloc" "noalloc"
    do
        build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t --l1d_mshrs=$a -r 1
        mv m5out/stats.txt $outdir/$t\_$a
    done
done

