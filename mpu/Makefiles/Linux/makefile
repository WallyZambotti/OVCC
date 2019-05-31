TARGET = libmpu.so
 
CFLAGS += -fPIC -pthread
LDFLAGS += -shared
INCDIR = -I../CoCo

_OBJS = mpu.o fpu.o gpu.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/mpu.o: mpu.c mpu.h defines.h ../CoCo/fileops.h
$(ODIR)/fpu.o: fpu.c mpu.h
$(ODIR)/gpu.o: gpu.c mpu.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
