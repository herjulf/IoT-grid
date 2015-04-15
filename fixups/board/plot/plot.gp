set size square
set xlabel "Voltmeter"
set ylabel "Board reported voltage"
#set terminal postscript eps color
#set output "plot.eps"
set term png
set output "io.png"

f(x) = k * x + l
fit f(x) "io2.dat" using 1:2 via k,l

plot \
 "io1.dat" using 1:2 with line ls 1, \
 "io2.dat" using 1:2 with line ls 2;

