CC = cc
CFLAGS = -O
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -L/opt/lib -lforms -lX11 -lgpib -lusb -lrt

all: video2 video2-diode

install: video2
	cp -f video2 video2-diode /usr/bin
	chmod u+s /usr/bin/video2 /usr/bin/video2-diode

video2: video2.o
	$(CC) -o video2 video2.o $(LDFLAGS) $(LGPIB)
video2.o: video2.c

video2-diode: video2-diode.o
	$(CC) -o video2-diode video2-diode.o $(LDFLAGS) $(LGPIB)
video2-diode.o: video2-diode.c

clean:
	-rm -f *.o *~ *.a video2 video2-diode
