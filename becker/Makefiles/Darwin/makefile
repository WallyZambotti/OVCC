TARGET = libbecker.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS =	becker.o iniman.o

include ../Makefile.common

$(ODIR)/becker.o: becker.c becker.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
