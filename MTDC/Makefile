CFLAGS?=-Wall

# It is safe to link static binaries on most platforms. Recommended.
# But a problem with recent Ubuntu 14.X has been spotted. So for now
# -static commented out.

LDFLAGS?=-static
vsc:	vsc.o 


run:	vsc
	./vsc > vsc.dat
	cat vsc.dat
	gnuplot < vsc.gp
	display vsc.png

clean:
	rm -f vsc.o vsc
