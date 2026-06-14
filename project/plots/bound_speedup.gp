set terminal pngcairo size 1200,560
set output "bound_speedup.png"

datafile = "../tests/bounded_speedup.txt"

SV = "#1f77b4"
SH = "#ff7f0e"
DV = "#2ca02c"
ES = "#d62728"

set datafile commentschars "#"
set style line 11 lc rgb "#3a3a3a" lw 1.2 dt solid
set style line 12 lc rgb "#d9d9d9" lw 0.7 dt solid
set grid ytics linestyle 11, linestyle 12
set yzeroaxis linestyle 11

set title "Speedup using Two-Pass"
set xlabel "Thread Count"
set ylabel "Multiplicative Speed Increase"
set key outside right

thread_pos(t) = (t == 1 ? 1 : (t == 12 ? 2 : t))
signed_speed(bf, tp) = (bf >= tp ? (bf/tp - 1.0) : -(tp/bf - 1.0))

set xtics ("1" 1, "12" 2)
set xrange [0.55:2.45]
set boxwidth 0.10 absolute

plot datafile using (strcol(2) eq "Standard" ? thread_pos($1) - 0.18 : 1/0):(signed_speed($3, $4)):(0.10) with boxes lc rgb SV fs solid 0.9 title "Standard", \
	datafile using (strcol(2) eq "Seahorse" ? thread_pos($1) - 0.06 : 1/0):(signed_speed($3, $4)):(0.10) with boxes lc rgb SH fs solid 0.9 title "Seahorse", \
	datafile using (strcol(2) eq "Void" ? thread_pos($1) + 0.06 : 1/0):(signed_speed($3, $4)):(0.10) with boxes lc rgb DV fs solid 0.9 title "Void", \
	datafile using (strcol(2) eq "Empty" ? thread_pos($1) + 0.18 : 1/0):(signed_speed($3, $4)):(0.10) with boxes lc rgb ES fs solid 0.9 title "Empty"
