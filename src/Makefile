CC = gcc
CFLAGS = -O -march=native -mtune=native -I/opt/include
INCLUDE = meas.h
OBJS = aspect.o bnc565.o bruker-er023.o bruker-er032.o dg535.o dk240.o \
       fl3000.o gpib.o graphics.o hp-34401a.o hp-53131a.o hp-5350b.o \
       hp-5384a.o itc503.o lpt-ttl.o matrix.o matrixwrapper.o mettler.o \
       misc.o newport_is.o pdr2000.o pi-max-wrapper.o scanmate_pro.o serial.o \
       sr245.o tr5211.o varian-e500.o wavetek80.o

all: libmeas.a

libmeas.a: $(OBJS)
	ar cr libmeas.a $(OBJS)

%.o:	%.c %.h $(INCLUDE)
	cc -c $(CFLAGS) $< 

install: libmeas.a
	cp -f libmeas.a /usr/lib
	ranlib /usr/lib/libmeas.a
	-mkdir /usr/include/meas
	cp -f *.h /usr/include/meas

clean:
	-rm -f *.o *~ *.a $(PROGS)
