include /usr/include/meas/make.conf

PROGS = example1 example2

all: $(PROGS)

example1: example1.o
	$(CC) -o example1 example1.o $(LDFLAGS)

example2: example2.o
	$(CC) -o example2 example2.o $(LDFLAGS)

example1.o: example1.c
example2.o: example2.c

clean:
	-rm -f *.o *~ *.a $(PROGS)
