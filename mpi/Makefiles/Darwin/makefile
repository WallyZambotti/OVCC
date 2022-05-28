TARGET=libmpi.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS = mpi.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/mpi.o: mpi.c mpi.h ../CoCo/fileops.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
