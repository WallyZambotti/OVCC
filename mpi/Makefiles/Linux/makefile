# change application name here (executable output name)
TARGET=testlib
 
# compiler
CC=gcc
# linker
LD=gcc
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

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD) #-Wl,--out-implib,libharddisk.a # -export-dynamic 

UNUSEDOBJS= 

_OBJS=	mpi.o fileops.o profport.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o libmpi.so
	$(LD) -o $@ $< -ldl $(SDLLIB)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $(SDLINC) $< -o $@

libmpi.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/mpi.o: mpi.c mpi.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/profport.o: ../CoCo/profport.c ../CoCo/profport.h
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) libmpi.so

install:
	sudo cp libmpi.so /usr/lib
	sudo ldconfig
	cp libmpi.so ../CoCo

test:
	./testlib libmpi.so ModuleName