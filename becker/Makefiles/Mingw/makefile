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
LIBS=$(AGARLIB) $(SDLLIB) -lws2_32

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libbecker.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=	becker.o profport.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o becker.dll 
	$(LD) -o $@ $< $(SDLLIB) -Wl,--subsystem,console

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $(SDLINC) $< -o $@

becker.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/becker.o: becker.c becker.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/profport.o: ../CoCo/profport.c ../CoCo/profport.h
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) becker.dll

install:
	cp becker.dll ../CoCo/libs

test:
	./testlib.exe becker.dll ModuleName
