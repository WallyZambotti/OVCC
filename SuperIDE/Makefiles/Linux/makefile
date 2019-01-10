# change application name here (executable output name)
TARGET=testlib
 
# compiler
CC=gcc
# linker
LD=gcc
# Resource Compiler
# RC=windres
# debug
DEBUG=#-g
# optimisation
OPT=-O2
# warnings
WARN=#-Wall
# Include Path 
INCDIR=# -I /usr/include/boost/predef/os
# Obj Dir
ODIR = obj

PTHREAD=#-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
POSSIBLE_LIBS= -lm -lWinmm -lgdi32 -ldxguid -lddraw -ldsound -ldinput -ldinput8 -lComctl32 -lole32
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD)  #-Wl,--out-implib,libharddisk.a # -export-dynamic 

UNUSEDOBJS= 

_OBJS=	ramdisk.o memboard.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o libramdisk.so
	$(LD) -o $@ $< $(SDLLIB)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $< -o $@

libramdisk.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/ramdisk.o: ramdisk.c memboard.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/memboard.o: memboard.c memboard.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET) libramdisk.so

install:
	sudo cp libramdisk.so /usr/lib
	sudo ldconfig
	cp libramdisk.so ../CoCo

test:
	./testlib libramdisk.so ModuleName
