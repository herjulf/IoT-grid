set term pbm color small
#set term png
set key textcolor rgb '#FFFFFF' 
set terminal png font arial x000000 
set output "vsc.png"

set style line 1 lt 1 lw 1 pt 3 linecolor rgb "red"
set style line 2 lt 1 lw 1 pt 3 linecolor rgb "blue"
set style line 3 lt 1 lw 1 pt 3 linecolor rgb "green"
set style line 4 lt 1 lw 1 pt 3 linecolor rgb "yellow"
set style line 5 lt 1 lw 1 pt 3 linecolor rgb "brown"
set style line 6 lt 1 lw 1 pt 3 linecolor rgb "white"
set datafile missing "85.00"

set yrange [ 0 : 20 ] noreverse nowriteback
set xrange [ 0 : 150] noreverse nowriteback
set ytics nomirror tc rgb "white"
set xtics nomirror tc rgb "white"

set border  lc rgb "white"

set title "Voltage Source Converters (VSC)\n\
using droop policy\n" tc rgb "white"

set ylabel "Grid Voltage" textcolor rgb "white"
set xlabel "Power Watt" textcolor rgb "white"

plot "vsc.dat" using ($2):($3) title "Converter 1"  with line ls 1,\
 "vsc.dat" using ($4):($5) title "Converter 2"  with line ls 2 ;


pause -1;
