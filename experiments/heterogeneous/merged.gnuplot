set term postscript color eps enhanced 22
set encoding utf8
set output "merged.eps"
load "../styles.inc"
NX=2
NY=1
# Size of graphs
SX=0.6
SY=0.57

# Margins
MX=0.06
MY=0.1
# Space between graphs
IX=0
IY=0
# Space for legends
LX=0.05
LY=0.01


#set obj 9 rect from screen 0.2,0.505 to screen 0.45,0.595 dt 3 behind
#set obj 9 fillstyle empty
#set obj 10 rect from screen 0.62,0.505 to screen 0.87,0.595 dt 3 behind
#set obj 10 fillstyle empty

set ylabel "Throughput (K tx/s)"
#set xlabel "Clients"
#set yrange [0:675]
#set xrange [-0.5:]
#set ytics 0,100,600
#set mytics 10
#set title "{/bold Original}" offset 0,0.2

#set grid ytics lw 2
set style data histogram 
set style histogram clustered gap 1 rowstacked

#set xtics 1 offset 0,-0.5
#set format y "%.0f"
#set format x
set boxwidth 0.9
#set style histogram clustered gap 1
set style fill solid 1 noborder

set size 1.0,0.6

set lmargin MX+0.5
set rmargin MX+12

set tmargin MY+4
set bmargin MY+0

set multiplot

set ytics nomirror
set xtics nomirror
set grid y




set origin MX+LX+0*(IX+SX),MY+0*(IY+SY)+LY
set size 0.6,SY
#set label 1001 "B = BFT-SMaRT" font "Arial, 16" at 11.7,780
#set label 1002 "W = Wheat" font "Arial, 16"		at 13,730

#set label 1001 "Kauri"   font ", 12" rotate by 25 right at   -6.8,-19 
#set label 1002 "Motor"   font ", 12" rotate by 25 right at   -5.8,-19 
#set label 1003 "HotStuff-secp"   font ", 12" rotate by 25 right at   -4.8,-19 
#set label 1004 "HotStuff-bls"   font ", 12" rotate by 25 right at   -3.8,-19 





set key maxrows 1 at 11,70 font ",17"

set xrange [-1:4]
plot\
	 "data/kauri.dat" using ($3/1000):xtic("") fill noborder lc rgb "black" title "Kauri", \
	 "data/motor.dat" using ($3/1000):xtic("") fill noborder lc rgb "#808080" title "Motor*", \
	 "data/hot-s.dat" using ($3/1000):xtic("") fill noborder lc rgb "dark-grey" title "HotStuff-secp", \
	 "data/hot-b.dat" using ($3/1000):xtic("") fill noborder lc rgb "grey" title "HotStuff-bls"

#	 "data/merged.dat" using (($4/1000)-($3/1000)):xtic("") fill solid 1.0 ls 1102 title "90th"




#############################
#############################
#############################

set origin MX+LX+1*(IX+SX)-0.1,MY+0*(IY+SY)+LY
set size 0.6,SY
set ylabel "Latency (ms)" offset 1.8,0
#set title "{/bold Kollaps}"
#unset label 1001
#unset label 1002
#set yrange [0:1]



#set ytics ("" 0, "" 100, "" 200, "" 300, "" 400, "" 500, "" 600)
#set key outside center vertical maxrows 1 sample 0.5 width 0.1 top
plot\
	 "data/kauri.dat" using ($4):xtic("") fill noborder lc rgb "black" notitle, \
	 "data/motor.dat" using ($4):xtic("") fill noborder lc rgb "#808080" notitle, \
	 "data/hot-s.dat" using ($4):xtic("") fill noborder lc rgb "dark-grey" notitle, \
	 "data/hot-b.dat" using ($4):xtic("") fill noborder lc rgb "grey" notitle



#	 "data/merged.dat" using ($4):xtic("") fill noborder lc rgb "black" notitle 

	 #"data/merged.dat" using ($4):xtic("") fill solid 0.9 ls 1104  title "50th"
#	 ,\
#	 "data/merged.dat" using (($4/1000)-($3/1000)):xtic("") fill solid 0.5 ls 1104 title "90th"

		
!epstopdf "merged.eps"
!rm "merged.eps"
quit
