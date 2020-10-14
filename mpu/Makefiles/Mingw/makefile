# change application name here (executable output name)
TARGET=mpu.dll
 
# compiler
#C=gcc
# linker
#LD=gcc
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

LDFLAGS=-shared $(PTHREAD) -s -Wl,--out-implib,libmpu.a

UNUSEDOBJS= 

_OBJS=mpu.o fpu.o gpu.o iniman.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))

mpu.dll: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@ 
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/mpu.o: mpu.c mpu.h defines.h ../CoCo/fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/fpu.o: fpu.c mpu.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/gpu.o: gpu.c mpu.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
	$(CC) $(CCFLAGS) -c $< -o $@
    
$(ODIR)/iniman.o: ../CoCo/iniman.c ../Coco/iniman.h
	$(CC) $(CCFLAGS) -I../CoCo -c $< -o $@
    
clean:
	rm -f $(ODIR)/*.o $(TARGET) mpu.dll mpu.a

install:
	cp mpu.dll ../CoCo/libs

test:
	.\testlib.exe mpu.dll ModuleName
