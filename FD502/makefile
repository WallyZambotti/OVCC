# change application name here (executable output name)
TARGET=fd502.dll
 
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
INCDIR=-I../CoCo
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

LDFLAGS=-shared $(PTHREAD) $(LIBS) -Wl,--out-implib,libfd502.a 

_OBJS=	fd502.o distortc.o wd1793.o fileops.o iniman.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

fd502.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/fd502.o: fd502.c fd502.h distortc.h wd1793.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/distortc.o: distortc.c distortc.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
#$(ODIR)/logger.o: logger.c logger.h ../CoCo/tcc1014mmu.h
#	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/wd1793.o: wd1793.c wd1793.h fd502.h defines.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/iniman.o: ../CoCo/iniman.c ../Coco/iniman.h
	$(CC) $(CCFLAGS) -I../CoCo -c $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET) fd502.dll

install:
	cp fd502.dll ../CoCo/libs

test:
	.\testlib.exe fd502.dll ModuleName