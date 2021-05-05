set term postscript color eps enhanced 22
set output 'latency.eps'
load "../styles.inc"
set size 1,0.6

set xlabel "Bandwidth (Mb/s)"
set ylabel "Latency (ms)"

set ytics 200
set grid ytics lw 2
set style data histogram 
set boxwidth 1
set style histogram clustered gap 1
set style fill solid 0.8


plot "data/latency.dat" using 2:xtic(1) fill noborder lc rgb "black" title "Kauri" ,\
      ""                using 3:xtic(1) fill noborder lc rgb "dark-grey" title "HotStuff-secp"



!epstopdf latency.eps
!rm latency.eps
quit
