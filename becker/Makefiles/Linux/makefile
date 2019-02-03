TARGET = libbecker.so
 
CFLAGS += -fPIC
LDFLAGS += -shared

_OBJS =	becker.o iniman.o

include ../Makefile.common

$(ODIR)/becker.o: becker.c becker.h
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
