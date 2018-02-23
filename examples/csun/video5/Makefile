include /usr/include/meas/make.conf

all: video5

install: video5
	cp -f video5 /usr/bin
#	chmod u+s /usr/bin/video5

video5: video5.o
	$(CC) $(CFLAGS) -o video5 video5.o $(LDFLAGS)

video5.o: video5.c

clean:
	-rm -f *.o *~ *.a video5
