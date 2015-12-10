include /usr/include/meas/make.conf

all: video-diode

install: video-diode
	cp -f video-diode /usr/bin
#	chmod u+s /usr/bin/video-diode

video-diode: video-diode.o
	$(CC) $(CFLAGS) -o video-diode video-diode.o $(LDFLAGS)

video-diode.o: video-diode.c

clean:
	-rm -f *.o *~ *.a video-diode
