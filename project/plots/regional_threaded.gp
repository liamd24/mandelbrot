# Set terminal and output
set terminal pngcairo size 900,600
set output "regional_thread_performance.png"

# Titles and labels
set title "Execution Time vs Threads"
set xlabel "Thread Count"
set ylabel "Time (ns)"
set grid
set key inside

# Data format
set datafile separator "\t"

set xrange [0:12]
set logscale y

# Plot (skip header row)
set style line 1 lt 1 lw 2 pt 7
set style line 2 lt 2 lw 2 pt 7
set style line 3 lt 3 lw 2 pt 7
set style line 4 lt 4 lw 2 pt 7

plot "../tests/region_thread_times.txt" using 1:2 with linespoints ls 1 title "Standard View", \
     "" using 1:3 with linespoints ls 2 title "Seahorse Valley", \
     "" using 1:4 with linespoints ls 3 title "Deep Void", \
     "" using 1:5 with linespoints ls 4 title "Empty Space"