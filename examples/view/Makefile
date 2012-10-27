include ../../make.conf

all: view view2 slice slice2 subtract

view: view.o
	$(CC) $(CFLAGS) -o view view.o $(LDFLAGS)

view.o: view.c
	$(CC) $(CFLAGS) -c view.c

view2: view2.o
	$(CC) $(CFLAGS) -o view2 view2.o $(LDFLAGS)

view2.o: view2.c
	$(CC) $(CFLAGS) -c view2.c

slice: slice.o
	$(CC) $(CFLAGS) -o slice slice.o $(LDFLAGS)

slice.o: slice.c
	$(CC) $(CFLAGS) -c slice.c

slice2: slice2.o
	$(CC) $(CFLAGS) -o slice2 slice2.o $(LDFLAGS)

slice2.o: slice2.c
	$(CC) $(CFLAGS) -c slice2.c

subtract: subtract.o
	$(CC) $(CFLAGS) -o subtract subtract.o $(LDFLAGS)

subtract.o: subtract.c
	$(CC) $(CFLAGS) -c subtract.c

clean:
	-rm *.o *~ slice slice2 subtract view view2
