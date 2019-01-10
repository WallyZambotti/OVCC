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
INCDIR=# -I /usr/include/boost/predef/os
# Obj Dir
ODIR = obj

PTHREAD=#-pthread

SDLINC=`sdl2-config --cflags`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)

CCFLAGS=$(IDIR) $(DEBUG) $(OPT) $(WARN) -fPIC
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(SDLLIB) $(AGARLIB)

LDFLAGS=-shared $(PTHREAD) $(LIBS) #-Wl,--out-implib,libharddisk.a # -export-dynamic 

UNUSEDOBJS= 

_OBJS=	fd502.o distortc.o wd1793.o fileops.o profport.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

testlib: obj/testlib.o libfd502.so
	$(LD) -o $@ $< $(LIBS)

$(ODIR)/testlib.o: testlib.c
	$(CC) -O0 -g -c $< -o $@

libfd502.so: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR)/fd502.o: fd502.c fd502.h resource.h distortc.h wd1793.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/distortc.o: distortc.c distortc.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/wd1793.o: wd1793.c wd1793.h fd502.h defines.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/profport.o: ../CoCo/profport.c ../CoCo/profport.h
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET) libfd502.so

install:
	sudo cp libfd502.so /usr/lib
	sudo ldconfig
	cp libfd502.so ../CoCo

test:
	./testlib libfd502.so ModuleName