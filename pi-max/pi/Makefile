TARGET = pipci
OBJS = pipci.o
MDIR = drivers/misc

CURRENT = $(shell uname -r)
EXTRA_CFLAGS = -DEXPORT_SYMTAB -I/usr/src/kernels/$(CURRENT)/include
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)
DEST = /lib/modules/$(CURRENT)/kernel/$(MDIR)

obj-m      := $(TARGET).o

pipci:
	make -C $(KDIR) SUBDIRS=$(PWD)


default:
	make -C $(KDIR) SUBDIRS=$(PWD) modules

$(TARGET).o: $(OBJS)
	$(LD) $(LD_RFLAG) -r -o $@ $(OBJS)

ifneq (,$(findstring 2.4.,$(CURRENT)))
install:
	su -c "cp -v $(TARGET).o $(DEST) && /sbin/depmod -a"
else
install:
	su -c "cp -v $(TARGET).ko $(DEST) && /sbin/depmod -a"
endif

clean:
	-rm -f *.o *.ko .*.cmd .*.flags *.mod.c modules.order Module.symvers *~
-include $(KDIR)/Rules.make
