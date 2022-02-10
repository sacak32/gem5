#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 16 -e 10"
outdir=home/pr_16_10/l2size

for a in '1MB' '2MB' '4MB' '8MB' '16MB'
do
    for t in "none" "base" "alloc" "noalloc"
    do
        build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t --l2_size=$a -r 1
        mv m5out/stats.txt $outdir/$t\_$a
    done
done

