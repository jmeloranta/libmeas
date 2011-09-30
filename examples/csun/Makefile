CC = cc
CFLAGS = -O
PROGS = abs abs2 takesp ftakesp takesp2 exp abs-kin abs2-kin rd eap eap2 cv cv2 pulse-cv
LDFLAGS = -lmeas -lm -lpvcam -lraw1394 -ldl -lplplotd -lgpib -lusb -lrt

all: $(PROGS)

install: all
	cp $(PROGS) /usr/local/bin
	cd /usr/local/bin
	/bin/chown root:root $(PROGS)
	/bin/chmod u+s $(PROGS)

eap: eap.o
	$(CC) -o eap eap.o $(LDFLAGS)

eap.o: eap.c

eap2: eap2.o
	$(CC) -o eap2 eap2.o $(LDFLAGS)

eap2.o: eap2.c

rd: rd.o
	$(CC) -o rd rd.o lasers.o $(LDFLAGS)

exp: exp.o experiment.o lasers.o
	$(CC) -o exp exp.o experiment.o lasers.o $(LDFLAGS)

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

cv: cv.o
	$(CC) -o cv cv.o $(LDFLAGS)

cv2: cv2.o
	$(CC) -o cv2 cv2.o $(LDFLAGS)

pulse-cv: pulse-cv.o
	$(CC) -o pulse-cv pulse-cv.o $(LDFLAGS)

abs.o: abs.c csun.h
abs-kin.o: abs-kin.c csun.h
	cc -c $(CFLAGS) -DNEWPORT -o abs-kin.o abs-kin.c
abs2.o: abs2.c csun.h
abs2-kin.o: abs-kin.c csun.h
	cc -c $(CFLAGS) -DMATRIX -o abs2-kin.o abs-kin.c
exp.o: exp.c csun.h
takesp.o: takesp.c csun.h
ftakesp.o: ftakesp.c csun.h
takesp2.o: takesp2.c csun.h
experiment.o: experiment.c csun.h
lasers.o: lasers.c csun.h
cv.o: cv.c
pulse-cv.o: pulse-cv.c

clean:
	-rm -f *.o *~ *.a $(PROGS)
