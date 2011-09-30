CC = cc
CFLAGS = -g
PROGS = exp
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -lplplotd -lgpib -lusb -lrt

all: $(PROGS)

install: all
	cp $(PROGS) /usr/local/bin
	cd /usr/local/bin
	/bin/chown root:root $(PROGS)
	/bin/chmod u+s $(PROGS)

exp: exp.o experiment.o lasers.o
	$(CC) -o exp exp.o experiment.o lasers.o $(LDFLAGS)

exp.o: exp.c uci.h
experiment.o: experiment.c uci.h
lasers.o: lasers.c uci.h

clean:
	-rm -f *.o *~ *.a $(PROGS)
