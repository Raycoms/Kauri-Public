set term postscript color eps enhanced 22
set output 'pipelining.eps'
load "../styles.inc"
set size 1,0.6

#set title "Throughput/Latency real vs. emulated" offset 0,-0.5

set xlabel "Pipelining Stretch"
set ylabel "Throughput (tx/s)"
set grid y
#set ytics 100
#set mytics 5
##set xrange[35:200]
set key bottom right

plot "data/Pipelining50k.dat"  with linespoints ls 1 title "50Kb", \
     "data/Pipelining100k.dat" with linespoints ls 2 title "100Kb", \
     "data/Pipelining200k.dat" with linespoints ls 3 title "200Kb", \
     "data/Pipelining250k.dat" with linespoints ls 4 title "250Kb"

!epstopdf "pipelining.eps"
!rm "pipelining.eps"
