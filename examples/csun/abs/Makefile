include /usr/include/meas/make.conf

PROGS = abs abs2 takesp ftakesp takesp2

all: $(PROGS)

install: all
	cp $(PROGS) /usr/local/bin
	cd /usr/local/bin
	/bin/chown root:root $(PROGS)
	/bin/chmod u+s $(PROGS)

takesp: takesp.o
	$(CC) -o takesp takesp.o $(LDFLAGS)

ftakesp: ftakesp.o
	$(CC) -o ftakesp ftakesp.o $(LDFLAGS)

takesp2: takesp2.o
	$(CC) -o takesp2 takesp2.o $(LDFLAGS)

abs: abs.o
	$(CC) -o abs abs.o $(LDFLAGS)

abs-kin: abs-kin.o
	$(CC) -o abs-kin abs-kin.o $(LDFLAGS)

abs2: abs2.o
	$(CC) -o abs2 abs2.o $(LDFLAGS)

abs2-kin: abs2-kin.o
	$(CC) -o abs2-kin abs2-kin.o $(LDFLAGS)

abs.o: abs.c
abs-kin.o: abs-kin.c
	cc -c $(CFLAGS) -DNEWPORT -o abs-kin.o abs-kin.c
abs2.o: abs2.c
abs2-kin.o: abs-kin.c
	cc -c $(CFLAGS) -DMATRIX -o abs2-kin.o abs-kin.c
takesp.o: takesp.c
ftakesp.o: ftakesp.c
takesp2.o: takesp2.c

clean:
	-rm -f *.o *~ *.a $(PROGS)
