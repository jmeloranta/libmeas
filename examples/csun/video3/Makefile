include /usr/include/meas/make.conf

all: video-laser video-view

install: video-laser video-view
	cp -f video-laser video-view /usr/bin
#	chmod u+s /usr/bin/video-laser

video-laser: video-laser.o
	$(CC) $(CFLAGS) -o video-laser video-laser.o $(LDFLAGS)

video-laser.o: video-laser.c

video-view: video-view.o
	$(CC) $(CFLAGS) -o video-view video-view.o $(LDFLAGS)

video-view.o: video-view.c

clean:
	-rm -f *.o *~ *.a video-laser video-view
