# change application name here (executable output name)
TARGET=superide.dll
 
# compiler
CC=gcc
# linker
LD=gcc
# Resource Compiler
RC=windres
# debug
DEBUG=#-g
# optimisation
OPT=-O2
# warnings
WARN=-Wall
# Include Path 
INCDIR = 
# Obj Dir
ODIR = obj

PTHREAD=-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB= `sdl2-config --libs`
AGARLIB= `agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD)  #-mwindows -Wl,-export-all-symbols -Wl,--out-implib,libsuperide.a # -export-dynamic 

UNUSEDOBJS= 

_OBJS=	superide.o cloud9.o fileops.o idebus.o logger.o profport.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o libsuperide.so
	$(LD) -o $@ $< $(SDLLIB)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $< -o $@

libsuperide.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/superide.o: superide.c superide.h defines.h idebus.h cloud9.h logger.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/cloud9.o: cloud9.c cloud9.h 
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/idebus.o: idebus.c idebus.h logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/logger.o: logger.c logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/profport.o: ../CoCo/profport.c ../CoCo/profport.h
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET)

install:
	sudo cp libsuperide.so /usr/lib
	sudo ldconfig
	cp libsuperide.so ../CoCo

test:
	./testlib libsuperide.so ModuleName
