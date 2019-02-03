TARGET=libsuperide.so
 
PTHREAD=-pthread

CFLAGS += -fPIC
LDFLAGS += -shared $(PTHREAD)

_OBJS = superide.o cloud9.o fileops.o idebus.o logger.o iniman.o

include ../Makefile.common

$(ODIR)/superide.o: superide.c superide.h defines.h idebus.h cloud9.h logger.h ../CoCo/fileops.h
$(ODIR)/cloud9.o: cloud9.c cloud9.h 
$(ODIR)/fileops.o: ../CoCo/fileops.c ../CoCo/fileops.h
$(ODIR)/idebus.o: idebus.c idebus.h logger.h 
$(ODIR)/logger.o: logger.c logger.h 
$(ODIR)/iniman.o: ../CoCo/iniman.c ../CoCo/iniman.h
