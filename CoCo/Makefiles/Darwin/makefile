TARGET = ovcc

_OBJS = vcc.o coco3.o config.o fileops.o hd6309.o iobus.o logger.o mc6809.o \
	mc6821.o pakinterface.o quickload.o tcc1014registers.o \
	tcc1014mmu_nomm.o tcc1014mmu_mm.o tcc1014mmu_common.o \
	throttle.o _xdebug.o cassette.o AGARInterface.o tcc1014graphicsAGAR.o keyboardAGAR.o \
	keyboardLayoutAGAR.o joystickinputSDL.o audioSDL.o vccgui.o iniman.o

include ../Makefile.common

$(ODIR)/vcc.o: vcc.c fileops.h defines.h resource.h vcc.h tcc1014mmu.h tcc1014graphicsAGAR.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h keyboard.h coco3.h pakinterface.h audio.h \
		config.h quickload.h throttle.h logger.h

$(ODIR)/coco3.o: coco3.c defines.h tcc1014graphicsAGAR.h \
		tcc1014registers.h hd6309.h mc6809.h mc6821.h coco3.h audio.h \
		throttle.h logger.h

$(ODIR)/config.o: config.c iniman.h config.h defines.h resource.h tcc1014graphicsAGAR.h \
		mc6821.h audio.h tcc1014mmu.h joystickinputSDL.h vcc.h fileops.h \
		pakinterface.h keyboard.h cassette.h

$(ODIR)/vccgui.o: vccgui.c defines.h joystickinputSDL.h keyboard.h audio.h config.h 

$(ODIR)/fileops.o: fileops.c fileops.h

$(ODIR)/hd6309.o: hd6309.c hd6309.h hd6309defs.h tcc1014mmu.h logger.h

$(ODIR)/iobus.o: iobus.c defines.h iobus.h mc6821.h pakinterface.h tcc1014registers.h tcc1014mmu.h logger.h config.h

$(ODIR)/logger.o: logger.c logger.h

$(ODIR)/mc6809.o: mc6809.c mc6809.h mc6809defs.h tcc1014mmu.h

$(ODIR)/mc6821.o: mc6821.c defines.h mc6821.h hd6309.h keyboard.h tcc1014graphicsAGAR.h tcc1014registers.h coco3.h \
	pakinterface.h cassette.h logger.h resource.h

$(ODIR)/pakinterface.o: pakinterface.c defines.h tcc1014mmu.h pakinterface.h config.h vcc.h mc6821.h logger.h fileops.h

$(ODIR)/quickload.o: quickload.c defines.h pakinterface.h vcc.h coco3.h tcc1014mmu.h fileops.h 

$(ODIR)/tcc1014mmu_common.o: tcc1014mmu_common.c tcc1014mmu.h

$(ODIR)/tcc1014mmu_nomm.o: tcc1014mmu_nomm.c defines.h tcc1014mmu.h iobus.h tcc1014graphicsAGAR.h config.h pakinterface.h hd6309.h \
	logger.h

$(ODIR)/tcc1014mmu_mm.o: tcc1014mmu_mm.c defines.h tcc1014mmu.h iobus.h tcc1014graphicsAGAR.h config.h pakinterface.h hd6309.h \
	logger.h 

$(ODIR)/tcc1014registers.o: tcc1014registers.c defines.h tcc1014registers.h tcc1014graphicsAGAR.h coco3.h keyboard.h tcc1014mmu.h \
	vcc.h

$(ODIR)/throttle.o: throttle.c throttle.h audio.h defines.h vcc.h 

$(ODIR)/_xdebug.o: _xdebug.c xdebug.h 

$(ODIR)/AGARInterface.o: AGARInterface.c resource.h tcc1014graphicsAGAR.h throttle.h

$(ODIR)/cassette.o: cassette.c cassette.h defines.h config.h coco3.h logger.h resource.h

$(ODIR)/keyboardAGAR.o: keyboardAGAR.c keyboard.h mc6821.h tcc1014registers.h vcc.h joystickinputSDL.h keyboardLayout.h

$(ODIR)/keyboardLayoutAGAR.o: keyboardLayoutAGAR.c keyboardLayout.h 

$(ODIR)/joystickinputSDL.o: joystickinputSDL.c joystickinputSDL.h 

$(ODIR)/audioSDL.o: audioSDL.c audio.h defines.h vcc.h config.h coco3.h logger.h

$(ODIR)/tcc1014graphicsAGAR.o: tcc1014graphicsAGAR.c defines.h tcc1014graphicsAGAR.h coco3.h cc2font.h cc3font.h config.h \
	logger.h 

$(ODIR)/iniman.o: iniman.c iniman.h

install:
	install $(TARGET) ../ovcc.app/Contents/$(TARGET)
	dylibbundler -of -b -x ../ovcc.app/Contents/$(TARGET) -d ../ovcc.app/Contents/libs -s /usr/local/lib

test:
# No test for $(TARGET)
