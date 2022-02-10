#!/bin/bash

exec=cpu_tests/gapbs/pr
input="-s 16 -e 10"
outdir=home/pr_16_10/clock

for d in 16 32 128 256
do
    for a in "1GHz" "2GHz" "3GHz" "4GHz"
    do
        for t in "none" "base" "alloc" "noalloc"
        do
            build/ARM/gem5.fast configs/example/se.py -c $exec -o "$input" --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t --cpu-clock=$a --l1d_size='4kB' --l1i_size='4kB' --l2_size='128kB' --prefetch-distance=$d -r 1
            mv m5out/stats.txt $outdir/$t\_$a\_$d
        done
    done
done
