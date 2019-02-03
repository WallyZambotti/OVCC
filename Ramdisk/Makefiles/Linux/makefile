TARGET = libramdisk.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS = ramdisk.o memboard.o

include ../Makefile.common

$(ODIR)/ramdisk.o: ramdisk.c memboard.h
$(ODIR)/memboard.o: memboard.c memboard.h
