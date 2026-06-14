#!/bin/bash

# Thread counts
threads=(1 2 4 8 12)

# Regions
categories=("standard" "seahorse" "void" "empty")

# Zoom and coords
zoom=(
  "1 -0.5 0"                        # standard
  "1000 -0.7436438870 0.1318259042" # seahorse
  "2000 -0.1984584369 1.1002709224" # void
  "10 -1.2338866889 -0.7041311504"  # empty
)

for t in "${threads[@]}"; do
    export OMP_NUM_THREADS=$t
    echo "=== Threads: $t ==="

    for i in "${!categories[@]}"; do
        output_file="tests/output_${categories[i]}_${t}.png"
        echo "Running: ./mandelbrot $output_file ${zoom[i]}"

        ./mandelbrot "$output_file" ${zoom[i]}
    done
done