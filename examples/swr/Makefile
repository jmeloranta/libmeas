include /usr/include/meas/make.conf

all: swr

swr: swr.o
	$(CC) $(CFLAGS) -o swr swr.o $(LDFLAGS)

swr.o: swr.c
	$(CC) $(CFLAGS) -c swr.c

clean:
	-rm swr.o swr *~
