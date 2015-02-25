include /usr/include/meas/make.conf

all: video-diode video-view

install: video-diode video-view
	cp -f video-diode video-view /usr/bin
#	chmod u+s /usr/bin/video-diode

video-diode: video-diode.o
	$(CC) $(CFLAGS) -o video-diode video-diode.o $(LDFLAGS)

video-diode.o: video-diode.c

video-view: video-view.o
	$(CC) $(CFLAGS) -o video-view video-view.o $(LDFLAGS)

video-view.o: video-view.c

clean:
	-rm -f *.o *~ *.a video-diode video-view
