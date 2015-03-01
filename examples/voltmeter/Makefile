include /usr/include/meas/make.conf

all: volt

volt: volt.o
	$(CC) $(CFLAGS) -o volt volt.o $(LDFLAGS)

volt.o: volt.c
	$(CC) $(CFLAGS) -c volt.c

clean:
	-rm volt.o volt *~
