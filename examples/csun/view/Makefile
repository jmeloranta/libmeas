include /usr/include/meas/make.conf

all: view-img view-pgm view-img2

install: view-img video-view view-img2
	cp -f view-img view-ppm view-img2 /usr/bin

view-img: view-img.o
	$(CC) $(CFLAGS) -o view-img view-img.o $(LDFLAGS)
view-img.o: view-img.c

view-pgm: view-pgm.o
	$(CC) $(CFLAGS) -o view-pgm view-pgm.o $(LDFLAGS)
view-pgm.o: view-pgm.c

view-img2: view-img2.o
	$(CC) $(CFLAGS) -o view-img2 view-img2.o $(LDFLAGS)
view-img2.o: view-img2.c

clean:
	-rm -f *.o *~ *.a view-img view-pgm view-img2

