include ../make.conf
CFLAGS += -std=gnu89 -DEXPORT="" -I.

OBJS = aspect.o bnc565.o bruker-er023.o bruker-er032.o dg535.o dk240.o \
       fl3000.o gpib.o graphics.o hp-34401a.o hp-53131a.o hp-5350b.o \
       hp-5384a.o itc503.o lpt-ttl.o matrix.o matrixwrapper.o mettler.o \
       misc.o newport_is.o pdr2000.o pi-max-wrapper.o scanmate_pro.o serial.o \
       sr245.o tr5211.o varian-e500.o wavetek80.o pdr900.o video.o image.o tty.o \
       mfj-226.o gpio.o pulsegen.o tds.o endian.o

all: libmeas.a

libmeas.a: prototypes $(OBJS)
	ar cr libmeas.a $(OBJS)

prototypes:
	egrep -h \^EXPORT *.c | tr \{ \; | sed -e "s/EXPORT //g" > proto.h
	ls *.h | grep -v meas.h | gawk -e '{print "#include <meas/" $$1 ">"}' > meas.h

%.o:	%.c %.h
	cc -c $(CFLAGS) $< 

%.o:	%.c
	cc -c $(CFLAGS) $< 

install:
	cp -f libmeas.a $(ROOT)/lib
	ranlib $(ROOT)/lib/libmeas.a
	-mkdir $(ROOT)/include/meas
	cp -f *.h $(ROOT)/include/meas
	cp -f ../make.conf $(ROOT)/include/meas

clean:
	-rm -f *.o *~ *.a proto.h meas.h $(PROGS)
