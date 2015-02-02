CC = cc
CFLAGS = -O
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -L/opt/lib -lforms -lX11 -lgpib -lusb -lrt

all: video2 video2-view video2-diode

install: video2
	cp -f video2 /usr/bin
	chmod u+s /usr/bin/video2

video2: video2.o
	$(CC) -o video2 video2.o $(LDFLAGS) $(LGPIB)

viode2-diode: video2-diode.o
	$(CC) -o video2-diode video2-diode.c $(LDFLAGS) $(LGPIB)

video2.o: video2.c
video2-diode.o: video2-diode.c

video2-view: video2-view.o
	$(CC) -o video2-view video2-view.o $(LDFLAGS) $(LGPIB)

video2-view.o: video2-view.c

clean:
	-rm -f *.o *~ *.a video2 video2-view
