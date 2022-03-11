TARGET = liborch90.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS = orch90.o

include ../Makefile.common

$(ODIR)/orch90.o: orch90.c orch90.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
