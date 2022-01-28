include /usr/include/meas/make.conf

all: smeter smeter2 scan

smeter: smeter.o
	$(CC) $(CFLAGS) -o smeter smeter.o $(LDFLAGS)

smeter.o: smeter.c
	$(CC) $(CFLAGS) -c smeter.c

smeter2: smeter2.o
	$(CC) $(CFLAGS) -o smeter2 smeter2.o $(LDFLAGS)

smeter2.o: smeter2.c
	$(CC) $(CFLAGS) -c smeter2.c

scan: scan.o
	$(CC) $(CFLAGS) -o scan scan.o $(LDFLAGS)

scan.o: scan.c
	$(CC) $(CFLAGS) -c scan.c

clean:
	-rm smeter.o smeter smeter2.o smeter2 scan.o scan *~
