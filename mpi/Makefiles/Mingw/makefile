# change application name here (executable output name)
TARGET=mpi.dll
 
# compiler
#CC=gcc
# linker
#LD=gcc
# Resource Compiler
# RC=windres
# debug
DEBUG=#-g
# optimisation
OPT=-O2
# warnings
WARN=#-Wall
# Include Path 
INCDIR=../CoCo
# Obj Dir
ODIR = obj

PTHREAD=#-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC

SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(AGARLIB) $(SDLLIB) -lregex

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libmpi.a

UNUSEDOBJS= 

_OBJS=mpi.o fileops.o iniman.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

mpi.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/mpi.o: mpi.c mpi.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) $(CCFLAGS) -c $< -o $@
    
$(ODIR)/iniman.o: ../CoCo/iniman.c ../Coco/iniman.h
	$(CC) $(CCFLAGS) -I../CoCo -c $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET) libmpi.so

install:
	cp mpi.dll ../CoCo/libs

test:
	.\testlib.exe mpi.dll ModuleName
