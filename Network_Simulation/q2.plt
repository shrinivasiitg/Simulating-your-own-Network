set xlabel 'time'
set ylabel 'percentage of packets dropped'
set yrange [0:50]
set title 'packet dropped v/s time'
set term gif
set output "q2.gif"
plot "q2_plot.txt" using 1:2 with linespoints, "q2_plot.txt" using 1:3 with linespoints, "q2_plot.txt" using 1:4 with linespoints, "q2_plot.txt" using 1:5 with linespoints
