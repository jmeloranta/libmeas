include /usr/include/meas/make.conf

all: gpio

gpio: gpio.o
	$(CC) $(CFLAGS) -o gpio gpio.o $(LDFLAGS)

gpio.o: gpio.c
	$(CC) $(CFLAGS) -c gpio.c

clean:
	-rm gpio.o gpio *~
