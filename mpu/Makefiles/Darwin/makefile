TARGET = libmpu.so
 
CFLAGS += -fPIC -pthread
LDFLAGS += -shared
INCDIR = -I../CoCo

_OBJS = mpu.o fpu.o gpu.o gpuprimitives.o gputextures.o gpuclock.o linkedlists.o dma.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/mpu.o: mpu.c mpu.h defines.h ../CoCo/fileops.h
$(ODIR)/fpu.o: fpu.c mpu.h fpu.h dma.h
$(ODIR)/gpu.o: gpu.c mpu.h gpu.h gpuprimitives.h linkedlists.h
$(ODIR)/dma.o: dma.c mpu.h dma.h gpu.h
$(ODIR)/gpuprimitives.o: gpuprimitives.c gpuprimitives.h mpu.h gpu.h linkedlists.h
$(ODIR)/gputextures.o: gputextures.c gputextures.h gpuprimitives.h dma.h mpu.h linkedlists.h
$(ODIR)/gpuclock.o: gpuclock.c gpuclock.h dma.h
$(ODIR)/linkedlists.o: linkedlists.c linkedlists.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
