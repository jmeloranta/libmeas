include /usr/include/meas/make.conf

all: video-3diode

install: video-3diode
	cp -f video-3diode /usr/bin
#	chmod u+s /usr/bin/video-3diode

video-3diode: video-3diode.o
	$(CC) $(CFLAGS) -o video-3diode video-3diode.o $(LDFLAGS)

video-3diode.o: video-3diode.c

clean:
	-rm -f *.o *~ *.a video-3diode
