TARGET = libramdisk.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS = ramdisk.o memboard.o

include ../Makefile.common

$(ODIR)/ramdisk.o: ramdisk.c memboard.h
$(ODIR)/memboard.o: memboard.c memboard.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
