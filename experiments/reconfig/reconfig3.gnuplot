set term postscript color eps enhanced 22
set output 'reconfig3.eps'
load "../styles.inc"
set size 1,0.6

#set title "Throughput/Latency real vs. emulated" offset 0,-0.5

set xlabel "Time (s)"
set ylabel "Throughput (K tx/s) "
set grid y
#set ytics 100
#set logscale y 2
#set mytics 5
#set xrange [90:410]
#set yrange [:40000]
set key above horizontal width -1 font ",14" 



set arrow from 40, graph 0 to 40, graph 1 nohead
set label "Leader\n failure\n   (3X)" at 27,5


plot "data/reconfig3.dat" using 1:($2/1000) with lines ls 20 title "Kauri", \
     ""                  using 1:($3/1000) with lines ls 23 title "HotStuff-secp"

!epstopdf "reconfig3.eps"
!rm "reconfig3.eps"
