# change application name here (executable output name)
TARGET=superide.dll
 
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
INCDIR = 
# Obj Dir
ODIR = obj

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(AGARLIB) $(SDLLIB) -lregex

LDFLAGS=-shared $(PTHREAD) $(LIBS) -s -Wl,--out-implib,libsuperide.a,--subsystem,console

UNUSEDOBJS= 

_OBJS=superide.o cloud9.o fileops.o idebus.o logger.o iniman.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

superide.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/superide.o: superide.c superide.h defines.h idebus.h cloud9.h logger.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/cloud9.o: cloud9.c cloud9.h 
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/idebus.o: idebus.c idebus.h logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/logger.o: logger.c logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET)

install:
	cp superide.dll ../CoCo/libs

test:
	.\testlib.exe superide.dll ModuleName
