include /usr/include/meas/make.conf

all: video-laser

install: video-laser
	cp -f video-laser /usr/bin
#	chmod u+s /usr/bin/video-laser

video-laser: video-laser.o
	$(CC) $(CFLAGS) -o video-laser video-laser.o $(LDFLAGS)
video-laser.o: video-laser.c

clean:
	-rm -f *.o *~ *.a video-laser
