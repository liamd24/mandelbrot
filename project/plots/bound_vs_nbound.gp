set terminal pngcairo size 1200,560
set output "bound_vs_nbound_performance.png"

datafile = "../tests/bound_vs_nbound_times.txt"

SV = "#1f77b4"
SH = "#ff7f0e"
DV = "#2ca02c"
ES = "#d62728"

set style line 11 lc rgb "#3a3a3a" lw 1.2 dt solid
set style line 12 lc rgb "#d9d9d9" lw 0.7 dt solid
set ytics 0,0.5
set logscale y
set mytics 4
set grid ytics mytics linestyle 11, linestyle 12
set xlabel "Thread Count"
set ylabel "Time (ms)"
thread_pos(t) = (t == 1 ? 1 : (t == 12 ? 2 : t))

set xtics ("1" 1, "12" 2)
set xrange [0.55:2.45]
set boxwidth 0.055 absolute
set yrange [1:3000]
set title "Algorithm Performance Comparison"
set rmargin 24
set key off

# Colors represent regions
set label 1 "Region" at screen 0.82,0.89 font ",11"
set object 1 rect at screen 0.83,0.85 size screen 0.02,0.02 fc rgb SV fs solid 1.0 border lc rgb "black"
set label 2 "Standard View" at screen 0.86,0.852 front
set object 2 rect at screen 0.83,0.81 size screen 0.02,0.02 fc rgb SH fs solid 1.0 border lc rgb "black"
set label 3 "Seahorse Valley" at screen 0.86,0.812 front
set object 3 rect at screen 0.83,0.77 size screen 0.02,0.02 fc rgb DV fs solid 1.0 border lc rgb "black"
set label 4 "Deep Void" at screen 0.86,0.772 front
set object 4 rect at screen 0.83,0.73 size screen 0.02,0.02 fc rgb ES fs solid 1.0 border lc rgb "black"
set label 5 "Empty Space" at screen 0.86,0.732 front

# Patterns represent algorithm
set label 6 "Algorithm" at screen 0.82,0.66 font ",11"
set object 5 rect at screen 0.83,0.62 size screen 0.02,0.02 fc rgb "black" fs pattern 1 border lc rgb "black"
set label 7 "Brute Force" at screen 0.86,0.622 front
set object 6 rect at screen 0.83,0.58 size screen 0.02,0.02 fc rgb "black" fs pattern 4 border lc rgb "black"
set label 8 "Two-Pass" at screen 0.86,0.582 front

plot datafile using (strcol(2) eq "SV" ? thread_pos($1) - 0.33 : 1/0):3:(0.055) with boxes lc rgb SV fs pattern 1 title "SV BruteForce", \
	datafile using (strcol(2) eq "SV" ? thread_pos($1) - 0.27 : 1/0):4:(0.055) with boxes lc rgb SV fs pattern 4 title "SV TwoPass", \
	datafile using (strcol(2) eq "SH" ? thread_pos($1) - 0.13 : 1/0):3:(0.055) with boxes lc rgb SH fs pattern 1 title "SH BruteForce", \
	datafile using (strcol(2) eq "SH" ? thread_pos($1) - 0.07 : 1/0):4:(0.055) with boxes lc rgb SH fs pattern 4 title "SH TwoPass", \
	datafile using (strcol(2) eq "DV" ? thread_pos($1) + 0.07 : 1/0):3:(0.055) with boxes lc rgb DV fs pattern 1 title "DV BruteForce", \
	datafile using (strcol(2) eq "DV" ? thread_pos($1) + 0.13 : 1/0):4:(0.055) with boxes lc rgb DV fs pattern 4 title "DV TwoPass", \
	datafile using (strcol(2) eq "ES" ? thread_pos($1) + 0.27 : 1/0):3:(0.055) with boxes lc rgb ES fs pattern 1 title "ES BruteForce", \
	datafile using (strcol(2) eq "ES" ? thread_pos($1) + 0.33 : 1/0):4:(0.055) with boxes lc rgb ES fs pattern 4 title "ES TwoPass"

