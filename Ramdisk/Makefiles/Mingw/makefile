# change application name here (executable output name)
TARGET=ramdisk.dll
 
# compiler
#CC=gcc
# linker
#LD=gcc
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

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libramdisk.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=	ramdisk.o memboard.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

ramdisk.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/ramdisk.o: ramdisk.c memboard.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/memboard.o: memboard.c memboard.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET) ramdisk.dll

install:
	cp ramdisk.dll ../CoCo/libs

test:
	.\testlib.exe ramdisk.dll ModuleName
