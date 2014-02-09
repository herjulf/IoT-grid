CFLAGS?=-Wall -DWITH_COAP=13
LDFLAGS?=-static
CC=gcc
LIBS=-lcurses

all: dummy iotgrid iotgridd

dummy:
	cp iotgrid.c iotgridd.c
	
iotgrid: iotgrid.c
	$(CC)  $(CFLAGS) -o $@ $^ $(LIBS)
	
iotgridd: iotgridd.c
	$(CC)  $(CFLAGS) -o $@ $^ $(LIBS)
	    	
#iotgrid:	iotgrid.o 
#install:	iotgrid
#iotgridd:	iotgridd.o 
#install:	iotgridd
#	strip iotgridd
#	mkdir -p $(DESTDIR)/$(PREFIX)/bin
#	cp -p iotgridd $(DESTDIR)/$(PREFIX)/bin
clean:
	rm -f *.o iotgrid iotgridd
