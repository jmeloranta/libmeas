include ../../../make.conf

all: video video-diode video-view

install: video video-diode video-view
	cp -f video video-diode video-view /usr/bin
#	chmod u+s /usr/bin/video

video: video.o lasers.o
	$(CC) $(CFLAGS) -o video video.o lasers.o $(LDFLAGS)

video.o: video.c csun.h

video-diode: video-diode.o lasers.o
	$(CC) $(CFLAGS) -o video-diode video-diode.o lasers.o $(LDFLAGS)

video.o: video.c csun.h

video-diode.o: video-diode.c csun.h

laser.o: laser.c csun.h

video-view: video-view.o
	$(CC) $(CFLAGS) -o video-view video-view.o $(LDFLAGS)

video-view.o: video-view.c csun.h

clean:
	-rm -f *.o *~ *.a video video-diode video-view
