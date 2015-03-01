include /usr/include/meas/make.conf

all: plot

plot: plot.o
	$(CC) $(CFLAGS) -o plot plot.o $(LDFLAGS)

plot.o: plot.c
	$(CC) $(CFLAGS) -c plot.c

clean:
	-rm plot.o plot *~
