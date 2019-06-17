TARGET = libmpu.so
 
CFLAGS += -fPIC -pthread
LDFLAGS += -shared
INCDIR = -I../CoCo

_OBJS = mpu.o fpu.o gpu.o gpuprimitives.o gputextures.o linkedlists.o dma.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/mpu.o: mpu.c mpu.h defines.h ../CoCo/fileops.h
$(ODIR)/fpu.o: fpu.c mpu.h fpu.h dma.h
$(ODIR)/gpu.o: gpu.c mpu.h gpu.h dma.h linkedlists.h
$(ODIR)/dma.o: dma.c mpu.h dma.h
$(ODIR)/gpuprimitives.o: gpuprimitives.c gpuprimitives.h mpu.h linkedlists.h
$(ODIR)/gputextures.o: gputextures.c mpu.h linkedlists.h
$(ODIR)/linkedlists.o: linkedlists.c linkedlists.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
