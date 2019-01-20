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
WARN=-Wall
# Include Path 
INCDIR=
# Obj Dir
ODIR = obj

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=#`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=#`agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD) 

_OBJS=	orch90.o #fileops.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o liborch90.so
	$(LD) -o $@ $< $(LIBS)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $(SDLINC) $< -o $@

liborch90.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@
    
$(ODIR)/orch90.o: orch90.c orch90.h
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) libmpi.so

install:
	sudo cp liborch90.so /usr/lib
	sudo ldconfig
	cp liborch90.so ../CoCo/libs

test:
	./testlib liborch90.so ModuleName

