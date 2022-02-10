#!/bin/bash

# Input 1: algorithm (e.g. pr, bfs)
# Input 2: scale (e.g. 16)
# Input 3: parameter (e.g. distance, l1assoc)
# Input rest: possible values of the parameter (16 32 64 128 256 for distance)

# Call it in home directory

# Define variables
out="default.csv"
dir="cc_21_10/default"
types=("base" "noalloc" "alloc")

# Go to directory
cd $dir

# Extract
mkdir intermediate 

for f in ./*
do
    if [ -f $f ]
    then 
        sed '4q;d' $f | awk '{print $2}' > intermediate/$f
    fi
done

# Create table
cd intermediate

if [ -f "$out" ] 
then
    rm $out
fi 

echo ",Base Prefetcher,Exclusive VB,Inclusive VB" >> $out


    for s in "${types[@]}"
    do
        x=`cat none`
        y=`cat $s`
        echo -n ",\"`echo "scale = 2; $x / $y" | bc`\"" >> $out
    done

    echo "" >> $out

sed -i "s/\./,/g" $out

# Cleanup
mkdir -p ../../results
mv $out ../../results
cd ..
rm -rf intermediate
