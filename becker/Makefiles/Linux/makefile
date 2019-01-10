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

SDLINC=`pkg-config --cflags sdl2`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD) $(LIBS) #-Wl,--out-implib,libharddisk.a # -export-dynamic 

UNUSEDOBJS= 

_OBJS=	becker.o profport.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o libbecker.so
	$(LD) -o $@ $< $(LIBS)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $< -o $@

libbecker.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/becker.o: becker.c becker.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/profport.o: ../CoCo/profport.c ../CoCo/profport.h
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) libmpi.so

install:
	sudo cp libbecker.so /usr/lib
	sudo ldconfig
	cp libbecker.so ../CoCo

test:
	./testlib libbecker.so ModuleName
