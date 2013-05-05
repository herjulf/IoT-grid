CFLAGS?=-Wall
LDFLAGS?=-static
iotgridd:	iotgridd.o 
install:	iotgridd
	strip iotgridd
	mkdir -p $(DESTDIR)/$(PREFIX)/bin
	cp -p iotgridd $(DESTDIR)/$(PREFIX)/bin
clean:
	rm -f *.o iotgridd
