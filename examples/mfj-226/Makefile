include /usr/include/meas/make.conf

all: sweep

sweep: sweep.o
	$(CC) $(CFLAGS) -o sweep sweep.o $(LDFLAGS)

sweep.o: sweep.c
	$(CC) $(CFLAGS) -c sweep.c

clean:
	-rm sweep.o sweep *~
