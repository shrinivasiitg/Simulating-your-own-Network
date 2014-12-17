set term gif
set output 'q3_throughput.gif'
set xlabel 'time'
set ylabel 'throughput'
set title 'throughput v/s time'
plot "q3_con1.txt" using 1:2 with linespoints, "q3_con2.txt" using 1:2 with linespoints, "q3_con3.txt" using 1:2 with linespoints, "q3_con4.txt" using 1:2 with linespoints, "q3_con5.txt" using 1:2 with linespoints

set output 'q3_queue_size.gif'
set xlabel 'time'
set ylabel 'queue size'
set title 'queue size v/s time'
set yrange [0:150]
plot "q3_qsize.txt" using 1:2 with linespoints
