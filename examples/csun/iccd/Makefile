CC = cc
CFLAGS = -O
PROGS = iccd
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -lplplotd -lgpib -lusb -lrt

all: $(PROGS)

install: iccd
	cp -f iccd /usr/bin
	chmod u+s /usr/bin/iccd

iccd: iccd.o experiment.o lasers.o
	$(CC) -o iccd iccd.o experiment.o lasers.o $(LDFLAGS) $(LGPIB)

iccd.o: iccd.c csun.h

lasers.o: lasers.c csun.h

clean:
	-rm -f *.o *~ *.a $(PROGS)
