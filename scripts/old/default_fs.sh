#!/bin/bash

exec=cpu_tests/graph500/cc
input="-s 16 -e 10"
outdir=home/bfs

for t in "none" "base" "alloc" "noalloc"
do
    build/ARM/gem5.fast configs/example/fs.py --disk-image=fs_images/arm/ubuntu-18.04-arm64-docker.img --kernel=fs_images/arm/binaries/vmlinux.arm64 --bootloader=fs_images/arm/binaries/boot_v2.arm64 --script=programmableconfigs/pr.rcS --caches --l2cache --cpu-type=MinorCPU --restore-with-cpu=MinorCPU --prefetch-type=$t -r 1
    mv m5out/stats.txt $outdir/$t
done

