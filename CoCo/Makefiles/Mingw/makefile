# change application name here (executable output name)
TARGET=ovcc
 
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

SDLINC=`pkg-config --cflags sdl2`
AGARINC=`agar-config --cflags`
IDIR=$(INCDIR) $(SDLINC) $(AGARINC)
 
CCFLAGS=$(IDIR) $(DEBUG) $(OPT) # $(WARN) $(PTHREAD) -pipe
 
SDLLIB=`sdl2-config --libs`
AGARLIB=`agar-config --libs`
LIBS=$(AGARLIB) $(SDLLIB) -lregex

LDFLAGS=$(LIBS) #-Wl,-export-all-symbols # -export-dynamic 

_OBJS=	vcc.o coco3.o config.o fileops.o hd6309.o hd6309turbo.o iobus.o logger.o mc6809.o \
	mc6821.o pakinterface.o quickload.o tcc1014mmu.o tcc1014registers.o \
	throttle.o _xdebug.o cassette.o SDLInterface.o tcc1014graphicsSDL.o keyboardSDL.o \
	keyboardLayoutSDL.o joystickinputSDL.o audioSDL.o vccgui.o iniman.o \
	hd6309asmP0-00-7F.o hd6309asmP0-80-FF.o hd6309asmP1-00-FF.o hd6309asmP2-00-FF.o \
	hd6309asmReg2Reg.o hd6309cea.o hd6309pshpul.o hd6309saveflags.o hd6309tfrexg.o
_UNUSED_OBJS = hd6309-a.o fixedtexture.o
OBJS= $(patsubst %,$(ODIR)/%,$(_OBJS))
 
ovcc: $(OBJS)
	$(LD) -o $(TARGET) $(OBJS) $(LDFLAGS) -Wl,--subsystem,console
    
$(ODIR):
	mkdir -p $@

$(OBJS): | $(ODIR)

$(ODIR)/vcc.o: vcc.c fileops.h defines.h resource.h vcc.h tcc1014mmu.h tcc1014graphicsSDL.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h keyboard.h coco3.h pakinterface.h audio.h \
		config.h quickload.h throttle.h logger.h iniman.o
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/coco3.o: coco3.c defines.h tcc1014graphicsSDL.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h coco3.h audio.h \
		throttle.h logger.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/config.o: config.c iniman.h config.h defines.h resource.h tcc1014graphicsSDL.h \
		mc6821.h audio.h tcc1014mmu.h joystickinputSDL.h vcc.h fileops.h \
		pakinterface.h keyboard.h cassette.h
	$(CC) -c $(CCFLAGS) $< -o $@
    
$(ODIR)/vccgui.o: vccgui.c defines.h joystickinputSDL.h keyboard.h audio.h config.h 
	$(CC) -c $(CCFLAGS) $< -o $@

#$(ODIR)/fixedtexture.o: fixedtexture.c fixedtexture.h
#	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/fileops.o: fileops.c fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

#$(ODIR)/hd6309-a.o: hd6309-a.asm
#	/C/msys64/usr/bin/jwasm/jwasm -q -10 -elf64 $< -Fo $@
#	mv hd6309-a.obj $@

$(ODIR)/hd6309turbo.o: hd6309turbo.c hd6309.h hd6309defs.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/hd6309.o: hd6309.c hd6309.h hd6309defs.h tcc1014mmu.h logger.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/iobus.o: iobus.c defines.h iobus.h mc6821.h pakinterface.h tcc1014registers.h tcc1014mmu.h logger.h config.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/logger.o: logger.c logger.h
	$(CC) -c $(CCFLAGS) $< $(LIBS) -o $@

$(ODIR)/mc6809.o: mc6809.c mc6809.h mc6809defs.h tcc1014mmu.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/mc6821.o: mc6821.c defines.h mc6821.h hd6309.h keyboard.h tcc1014graphicsSDL.h tcc1014registers.h coco3.h \
	pakinterface.h cassette.h logger.h resource.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/pakinterface.o: pakinterface.c defines.h tcc1014mmu.h pakinterface.h config.h vcc.h mc6821.h logger.h fileops.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/quickload.o: quickload.c defines.h pakinterface.h vcc.h coco3.h tcc1014mmu.h fileops.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/tcc1014mmu.o: tcc1014mmu.c defines.h tcc1014mmu.h iobus.h tcc1014graphicsSDL.h config.h pakinterface.h hd6309.h \
	logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/tcc1014registers.o: tcc1014registers.c defines.h tcc1014registers.h tcc1014graphicsSDL.h coco3.h keyboard.h tcc1014mmu.h \
	vcc.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/throttle.o: throttle.c throttle.h audio.h defines.h vcc.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/_xdebug.o: _xdebug.c xdebug.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/SDLInterface.o: SDLInterface.c resource.h tcc1014graphicsSDL.h throttle.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/cassette.o: cassette.c cassette.h defines.h config.h coco3.h logger.h resource.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/keyboardSDL.o: keyboardSDL.c keyboard.h mc6821.h tcc1014registers.h vcc.h joystickinputSDL.h keyboardLayout.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/keyboardLayoutSDL.o: keyboardLayoutSDL.c keyboardLayout.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/joystickinputSDL.o: joystickinputSDL.c joystickinputSDL.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/audioSDL.o: audioSDL.c audio.h defines.h vcc.h config.h coco3.h logger.h
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/tcc1014graphicsSDL.o: tcc1014graphicsSDL.c defines.h tcc1014graphicsSDL.h coco3.h cc2font.h cc3font.h config.h \
	logger.h 
	$(CC) -c $(CCFLAGS) $< -o $@

$(ODIR)/hd6309asmP0-00-7F.o: hd6309asmP0-00-7F.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP0-00-7F.obj $@

$(ODIR)/hd6309asmP0-80-FF.o: hd6309asmP0-80-FF.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP0-80-FF.obj $@

$(ODIR)/hd6309asmP1-00-FF.o: hd6309asmP1-00-FF.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP1-00-FF.obj $@

$(ODIR)/hd6309asmP2-00-FF.o: hd6309asmP2-00-FF.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP2-00-FF.obj $@

$(ODIR)/hd6309asmReg2Reg.o: hd6309asmReg2Reg.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmReg2Reg.obj $@

$(ODIR)/hd6309cea.o: hd6309cea.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309cea.obj $@

$(ODIR)/hd6309tfrexg.o: hd6309tfrexg.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309tfrexg.obj $@

$(ODIR)/hd6309pshpul.o: hd6309pshpul.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309pshpul.obj $@

$(ODIR)/hd6309saveflags.o: hd6309saveflags.asm
	jwasm -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309saveflags.obj $@

$(ODIR)/iniman.o: iniman.c iniman.h
	$(CC) -c $(CCFLAGS) $< -o $@

clean:
	rm -f $(ODIR)/*.o $(TARGET)
