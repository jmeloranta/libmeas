include ../../make.conf

all: video

video: video.o
	$(CC) $(CFLAGS) -o video video.o $(LDFLAGS)

video.o: video.c
	$(CC) $(CFLAGS) -c video.c

clean:
	-rm video.o video *~
