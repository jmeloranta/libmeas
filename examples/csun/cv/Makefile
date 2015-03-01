include /usr/include/meas/make.conf

PROGS = cv cv2 pulse-cv

all: $(PROGS)

install: all
	cp $(PROGS) /usr/local/bin
	cd /usr/local/bin
	/bin/chown root:root $(PROGS)
	/bin/chmod u+s $(PROGS)
cv: cv.o
	$(CC) -o cv cv.o $(LDFLAGS)

cv2: cv2.o
	$(CC) -o cv2 cv2.o $(LDFLAGS)

pulse-cv: pulse-cv.o
	$(CC) -o pulse-cv pulse-cv.o $(LDFLAGS)

cv.o: cv.c
cv2.o: cv2.c
pulse-cv.o: pulse-cv.c

clean:
	-rm -f *.o *~ *.a $(PROGS)
