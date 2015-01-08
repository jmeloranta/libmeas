include ../../../make.conf

all: video video-diode video-view

install: video video-diode video-view
	cp -f video video-diode video-view /usr/bin
#	chmod u+s /usr/bin/video

video: video.o
	$(CC) $(CFLAGS) -o video video.o $(LDFLAGS)

video.o: video.c conf.h ../lasers.c

video-diode: video-diode.o
	$(CC) $(CFLAGS) -o video-diode video-diode.o $(LDFLAGS)

video.o: video.c conf.h ../lasers.c

video-diode.o: video-diode.c conf.h ../lasers.c

video-view: video-view.o
	$(CC) $(CFLAGS) -o video-view video-view.o $(LDFLAGS)

video-view.o: video-view.c conf.h

clean:
	-rm -f *.o *~ *.a video video-diode video-view
