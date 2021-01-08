include /usr/include/meas/make.conf

all: video5a

install: video5a
	cp -f video5a /usr/bin
#	chmod u+s /usr/bin/video5a

video5a: video5a.o
	$(CC) $(CFLAGS) -o video5a video5a.o $(LDFLAGS)

video5a.o: video5a.c

clean:
	-rm -f *.o *~ *.a video5a
