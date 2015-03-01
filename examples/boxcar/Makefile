include /usr/include/meas/make.conf

PROGS = read

all: $(PROGS)

read: read.o
	$(CC) -o read read.o $(LDFLAGS)

read.o: read.c

clean:
	-rm -f *.o *~ *.a $(PROGS)
