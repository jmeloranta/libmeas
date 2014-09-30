CC = cc
CFLAGS = -O
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -L/opt/lib -lforms -lX11 -lgpib -lusb -lrt

all: video2

install: video2
	cp -f video2 /usr/bin
	chmod u+s /usr/bin/video2

video2: video2.o lasers.o
	$(CC) -o video2 video2.o lasers.o $(LDFLAGS) $(LGPIB)

video2.o: video2.c
lasers.o: lasers.c csun.h

clean:
	-rm -f *.o *~ *.a $(PROGS)
