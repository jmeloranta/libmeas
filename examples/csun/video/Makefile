include ../../../make.conf

all: video

install: video
	cp -f video /usr/bin
#	chmod u+s /usr/bin/video

video: video.o lasers.o
	$(CC) -o video video.o lasers.o $(LDFLAGS)

video.o: video.c csun.h

lasers.o: lasers.c csun.h

clean:
	-rm -f *.o *~ *.a $(PROGS)
