set term postscript color eps enhanced 22
set output 'tputlat.eps'
load "../styles.inc"
set size 1,0.6

set xlabel "Latency (ms)"
set ylabel "Throughput (K tx/s) "
set grid y
#set ytics 100
set logscale y 2
#set mytics 5
#set xrange [90:410]
#set yrange [:40000]
set key above horizontal width -1 font ",14" 



plot "data/kauri2.dat" using 2:($3/1000) with linespoints ls 20 title "Kauri (h=2)", \
      "" using 2:($3/1000):1 with labels offset -0.7,-0.5  font ",10" notitle, \
      "data/kauri3.dat" using 2:($3/1000) with linespoints ls 200 title "Kauri (h=3)", \
      "" using 2:($3/1000):1 with labels offset 0.7,0.5  font ",10" notitle, \
      "data/hotbls.dat" using 2:($3/1000) with linespoints ls 22 title "HotStuff-bls", \
      "" using 2:($3/1000):1 with labels offset -0.7,-0.5  font ",10" notitle, \
      "data/hotsecp.dat" using 2:($3/1000) with linespoints ls 23 title "HotStuff-secp" ,\
      "" using 2:($3/1000):1 with labels offset 0.5,0.5  font ",10" notitle

!epstopdf "tputlat.eps"
!rm "tputlat.eps"
