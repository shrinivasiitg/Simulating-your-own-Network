set xlabel 'delay on the second link'
set ylabel 'throughput of the two connections'
set yrange [0:500]
set title 'delay v/s throughput for the two connections'
set term gif
set output "q4.gif"
plot "q4_plot.txt" using 1:2 with linespoints, "q4_plot.txt" using 1:3 with linespoints

set term gif
set output "q4_a.gif"
set xlabel 'time'
set ylabel 'throughput'
set yrange [0:1000]
plot "q4_a_plot.txt" using 1:2 with linespoints, "q4_a_plot.txt" using 1:3 with linespoints

