#!/bin/bash

algos=("pr" "bfs" "sssp" "cc")
scale=18

for a in "${algos[@]}"
do
    ./scripts/m5out_to_csv.sh $a $scale "l1size" '8kB' '16kB' '32kB' '64kB' '128kB'
    ./scripts/m5out_to_csv.sh $a $scale "distance" 16 32 64 128 256
    ./scripts/m5out_to_csv.sh $a $scale "l1assoc" 1 2 4 8
    ./scripts/m5out_to_csv.sh $a $scale "l1mshrs" 4 8 16 32 64
    ./scripts/m5out_to_csv.sh $a $scale "l1latency" 1 2 4 8
    ./scripts/m5out_to_csv.sh $a $scale "l2latency" 8 16 32 64 128
    #./scripts/m5out_to_csv.sh $a $scale "l2size" '1MB' '2MB' '4MB' '8MB' '16MB'
done
