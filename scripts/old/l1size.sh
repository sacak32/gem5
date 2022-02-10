#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 16 -e 10"
outdir=home/pr_16_10/l1size

for a in '2kB' '8kB' '16kB' '32kB'
do
    for t in "none" "base" "alloc" "noalloc"
    do
        build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t --l1d_size=$a -r 1
        mv m5out/stats.txt $outdir/$t\_$a
    done
done

