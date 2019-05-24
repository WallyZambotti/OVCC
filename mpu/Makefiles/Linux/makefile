TARGET = libmpu.so
 
CFLAGS += -fPIC
LDFLAGS += -shared
INCDIR = -I../CoCo

_OBJS = mpu.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/mpu.o: mpu.c mpu.h defines.h ../CoCo/fileops.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
