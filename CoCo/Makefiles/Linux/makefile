TARGET = ovcc

 ifeq ($(TARGETARCH),AMD)
_OBJS = vcc.o coco3.o config.o fileops.o hd6309.o iobus.o logger.o mc6809.o \
	mc6821.o pakinterface.o quickload.o tcc1014mmu.o tcc1014registers.o \
	throttle.o _xdebug.o cassette.o SDLInterface.o tcc1014graphicsSDL.o keyboardSDL.o \
	keyboardLayoutSDL.o joystickinputSDL.o audioSDL.o vccgui.o iniman.o hd6309turbo.o \
	hd6309asmP0-00-7F.o hd6309asmP0-80-FF.o hd6309asmP1-00-FF.o hd6309asmP2-00-FF.o \
	hd6309asmReg2Reg.o hd6309cea.o hd6309pshpul.o hd6309saveflags.o hd6309tfrexg.o
else
_OBJS = vcc.o coco3.o config.o fileops.o hd6309.o iobus.o logger.o mc6809.o \
	mc6821.o pakinterface.o quickload.o tcc1014mmu.o tcc1014registers.o \
	throttle.o _xdebug.o cassette.o SDLInterface.o tcc1014graphicsSDL.o keyboardSDL.o \
	keyboardLayoutSDL.o joystickinputSDL.o audioSDL.o vccgui.o iniman.o 
endif

_UNUSED_OBJS = hd6309-a.o fixedtexture.o

include ../Makefile.common

$(ODIR)/vcc.o: vcc.c fileops.h defines.h resource.h vcc.h tcc1014mmu.h tcc1014graphicsSDL.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h keyboard.h coco3.h pakinterface.h audio.h \
		config.h quickload.h throttle.h logger.h
    
$(ODIR)/coco3.o: coco3.c defines.h tcc1014graphicsSDL.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h coco3.h audio.h \
		throttle.h logger.h
    
$(ODIR)/config.o: config.c iniman.h config.h defines.h resource.h tcc1014graphicsSDL.h \
		mc6821.h audio.h tcc1014mmu.h joystickinputSDL.h vcc.h fileops.h \
		pakinterface.h keyboard.h cassette.h
    
$(ODIR)/vccgui.o: vccgui.c defines.h joystickinputSDL.h keyboard.h audio.h config.h 

#$(ODIR)/fixedtexture.o: fixedtexture.c fixedtexture.h

$(ODIR)/fileops.o: fileops.c fileops.h

#$(ODIR)/hd6309-a.o: hd6309-a.asm
#	/C/msys64/usr/bin/jwasm/jwasm -q -10 -elf64 $< -Fo $@
#	mv hd6309-a.obj $@

$(ODIR)/hd6309.o: hd6309.c hd6309.h hd6309defs.h tcc1014mmu.h logger.h

$(ODIR)/iobus.o: iobus.c defines.h iobus.h mc6821.h pakinterface.h tcc1014registers.h tcc1014mmu.h logger.h config.h

$(ODIR)/logger.o: logger.c logger.h

$(ODIR)/mc6809.o: mc6809.c mc6809.h mc6809defs.h tcc1014mmu.h

$(ODIR)/mc6821.o: mc6821.c defines.h mc6821.h hd6309.h keyboard.h tcc1014graphicsSDL.h tcc1014registers.h coco3.h \
	pakinterface.h cassette.h logger.h resource.h

$(ODIR)/pakinterface.o: pakinterface.c defines.h tcc1014mmu.h pakinterface.h config.h vcc.h mc6821.h logger.h fileops.h

$(ODIR)/quickload.o: quickload.c defines.h pakinterface.h vcc.h coco3.h tcc1014mmu.h fileops.h 

$(ODIR)/tcc1014mmu.o: tcc1014mmu.c defines.h tcc1014mmu.h iobus.h tcc1014graphicsSDL.h config.h pakinterface.h hd6309.h \
	logger.h 

$(ODIR)/tcc1014registers.o: tcc1014registers.c defines.h tcc1014registers.h tcc1014graphicsSDL.h coco3.h keyboard.h tcc1014mmu.h \
	vcc.h

$(ODIR)/throttle.o: throttle.c throttle.h audio.h defines.h vcc.h 

$(ODIR)/_xdebug.o: _xdebug.c xdebug.h 

$(ODIR)/SDLInterface.o: SDLInterface.c resource.h tcc1014graphicsSDL.h throttle.h

$(ODIR)/cassette.o: cassette.c cassette.h defines.h config.h coco3.h logger.h resource.h

$(ODIR)/keyboardSDL.o: keyboardSDL.c keyboard.h mc6821.h tcc1014registers.h vcc.h joystickinputSDL.h keyboardLayout.h

$(ODIR)/keyboardLayoutSDL.o: keyboardLayoutSDL.c keyboardLayout.h 

$(ODIR)/joystickinputSDL.o: joystickinputSDL.c joystickinputSDL.h 

$(ODIR)/audioSDL.o: audioSDL.c audio.h defines.h vcc.h config.h coco3.h logger.h

$(ODIR)/tcc1014graphicsSDL.o: tcc1014graphicsSDL.c defines.h tcc1014graphicsSDL.h coco3.h cc2font.h cc3font.h config.h \
	logger.h 

$(ODIR)/hd6309asmP0-00-7F.o: hd6309asmP0-00-7F.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP0-00-7F.o $@

$(ODIR)/hd6309turbo.o: hd6309turbo.c hd6309.h hd6309defs.h

$(ODIR)/hd6309asmP0-80-FF.o: hd6309asmP0-80-FF.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP0-80-FF.o $@

$(ODIR)/hd6309asmP1-00-FF.o: hd6309asmP1-00-FF.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP1-00-FF.o $@

$(ODIR)/hd6309asmP2-00-FF.o: hd6309asmP2-00-FF.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmP2-00-FF.o $@

$(ODIR)/hd6309asmReg2Reg.o: hd6309asmReg2Reg.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309asmReg2Reg.o $@

$(ODIR)/hd6309cea.o: hd6309cea.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309cea.o $@

$(ODIR)/hd6309tfrexg.o: hd6309tfrexg.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309tfrexg.o $@

$(ODIR)/hd6309pshpul.o: hd6309pshpul.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309pshpul.o $@

$(ODIR)/hd6309saveflags.o: hd6309saveflags.asm
	$(ASM) -q -10 -elf64 -mf -zf0 $< -Fo $@
	mv hd6309saveflags.o $@

$(ODIR)/iniman.o: iniman.c iniman.h

install:
	install -D $(TARGET) $(EXEDIR)/$(TARGET)
test:
	# No test for $(TARGET)
