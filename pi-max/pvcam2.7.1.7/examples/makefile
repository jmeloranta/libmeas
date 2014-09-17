LIBX = -lX11
LIBP = -lpvcam -ldl

all: pvt param latest oldest getparam setparam stdacq


pvt:
	gcc -o pvtest pvtest.c complxw.c -g -DLINUX $(LIBP) $(LIBX)

param:
	gcc -o pvparam pvparam.c -g -DLINUX $(LIBP)

latest:
	gcc -o circbuflatest CircBufLatest.c -g $(LIBP)

oldest:
	gcc -o circbufoldest CircBufOldest.c -g $(LIBP)

getparam:
	gcc -o getparam GetParam_GetEnum.c -g $(LIBP)

setparam:
	gcc -o setparam SetParam.c -g $(LIBP)

stdacq:
	gcc -o stdacq StandardAcq.c -g $(LIBP)

