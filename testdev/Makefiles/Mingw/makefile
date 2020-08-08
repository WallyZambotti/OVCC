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
INCDIR=
# Obj Dir
ODIR = obj

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(AGARLIB) $(SDLLIB) 

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libtestdev.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=	testdev.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o testdev.dll
	$(LD) -o $@ $< $(LIBS) -Wl,--subsystem,console

$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/testlib.o: ../testlib.c | $(ODIR)
	$(CC) -O0 -g -c $(AGARINC) $(SDLINC) $< -o $@

testdev.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) testdev.def $(LIBS) -o $@ 
    
$(ODIR)/testdev.o: testdev.c
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) testdev.dll

install:
	cp ramdisk.dll ../CoCo/libs

test:
	.\testlib.exe testdev.dll ModuleName
