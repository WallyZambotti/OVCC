# change application name here (executable output name)
TARGET=becker.dll
 
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
INCDIR=-I../CoCo
# Obj Dir
ODIR = obj

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
POSSIBLE_LIBS= -lm -lWinmm -lgdi32 -ldxguid -lddraw -ldsound -ldinput -ldinput8 -lComctl32 -lole32
LIBS=$(AGARLIB) $(SDLLIB) -lws2_32 -lregex

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libbecker.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=	becker.o iniman.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

becker.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/becker.o: becker.c becker.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) becker.dll

install:
	cp becker.dll ../CoCo/libs
