CC = cc
CFLAGS = -g
PROGS = fluorescence
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -lplplotd -lgpib -lusb -lrt

all: $(PROGS)

install: all
	cp $(PROGS) /usr/local/bin
	cd /usr/local/bin
	/bin/chown root:root $(PROGS)
	/bin/chmod u+s $(PROGS)

fluorescence: fluorescence.o experiment.o lasers.o
	$(CC) -o fluorescence fluorescence.o experiment.o lasers.o $(LDFLAGS) $(LGPIB)

exp.o: fluorescence.c csun.h

clean:
	-rm -f *.o *~ *.a $(PROGS)
