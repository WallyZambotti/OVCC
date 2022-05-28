TARGET = libharddisk.so
 
CFLAGS += -fPIC
LDFLAGS += -shared
INCDIR = -I../CoCo

_OBJS = harddisk.o cloud9.o cc3vhd.o fileops.o iniman.o

include ../Makefile.common

$(ODIR)/harddisk.o: harddisk.c harddisk.h defines.h cloud9.h ../CoCo/fileops.h
$(ODIR)/cloud9.o: cloud9.c cloud9.h
$(ODIR)/cc3vhd.o: cc3vhd.c cc3vhd.h defines.h harddisk.h ../CoCo/fileops.h
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h

install:
	install -m 644 $(TARGET) ../ovcc.app/Contents/modules/$(TARGET)
	dylibbundler -od -b -x ../ovcc.app/Contents/modules/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
