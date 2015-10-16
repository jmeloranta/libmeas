include /usr/include/meas/make.conf

all: view-img view-pgm

install: view-img video-view
	cp -f view-img view-ppm /usr/bin

view-img: view-img.o
	$(CC) $(CFLAGS) -o view-img view-img.o $(LDFLAGS)

view-img.o: view-img.c

view-pgm: view-pgm.o
	$(CC) $(CFLAGS) -o view-pgm view-pgm.o $(LDFLAGS)

view-pgm.o: view-pgm.c

clean:
	-rm -f *.o *~ *.a view-img view-pgm

