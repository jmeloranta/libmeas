include /usr/include/meas/make.conf

all: smeter scan

smeter: smeter.o
	$(CC) $(CFLAGS) -o smeter smeter.o $(LDFLAGS)

smeter.o: smeter.c
	$(CC) $(CFLAGS) -c smeter.c

scan: scan.o
	$(CC) $(CFLAGS) -o scan scan.o $(LDFLAGS)

scan.o: scan.c
	$(CC) $(CFLAGS) -c scan.c

clean:
	-rm smeter.o smeter scan.o scan *~
