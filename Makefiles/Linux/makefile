xcoco:
	if [ ! -f ./CoCo/makefile ]; then cp ./CoCo/Makefiles/Linux/makefile ./CoCo ; fi
	if [ ! -d ./CoCo/obj ]; then mkdir ./CoCo/obj ; fi
	$(MAKE) -C ./CoCo

xbecker:
	if [ ! -f ./becker/makefile ]; then cp ./becker/Makefiles/Linux/makefile ./becker ; fi 
	if [ ! -d ./becker/obj ]; then mkdir ./becker/obj ; fi
	$(MAKE) -C becker

xfd502:
	if [ ! -f ./FD502/makefile ]; then cp ./FD502/Makefiles/Linux/makefile ./FD502 ; fi 
	if [ ! -d ./FD502/obj ]; then mkdir ./FD502/obj ; fi
	$(MAKE) -C FD502

xharddisk:
	if [ ! -f ./HardDisk/makefile ]; then cp ./HardDisk/Makefiles/Linux/makefile ./HardDisk ; fi 
	if [ ! -d ./HardDisk/obj ]; then mkdir ./HardDisk/obj ; fi
	$(MAKE) -C HardDisk

xmpi:
	if [ ! -f ./mpi/makefile ]; then cp ./mpi/Makefiles/Linux/makefile ./mpi ; fi 
	if [ ! -d ./mpi/obj ]; then mkdir ./mpi/obj ; fi
	$(MAKE) -C mpi

xorch90:
	if [ ! -f ./orch90/makefile ]; then cp ./orch90/Makefiles/Linux/makefile ./orch90 ; fi 
	if [ ! -d ./orch90/obj ]; then mkdir ./orch90/obj ; fi
	$(MAKE) -C orch90

xramdisk:
	if [ ! -f ./Ramdisk/makefile ]; then cp ./Ramdisk/Makefiles/Linux/makefile ./Ramdisk ; fi 
	if [ ! -d ./Ramdisk/obj ]; then mkdir ./Ramdisk/obj ; fi
	$(MAKE) -C Ramdisk

xsuperide:
	if [ ! -f ./SuperIDE/makefile ]; then cp ./SuperIDE/Makefiles/Linux/makefile ./SuperIDE ; fi 
	if [ ! -d ./SuperIDE/obj ]; then mkdir ./SuperIDE/obj ; fi
	$(MAKE) -C SuperIDE

ovcc: 
	$(MAKE) xcoco
	$(MAKE) xbecker
	$(MAKE) xfd502
	$(MAKE) xharddisk
	$(MAKE) xmpi
	$(MAKE) xorch90
	$(MAKE) xramdisk
	$(MAKE) xsuperide

install:
	$(MAKE) -C becker install
	$(MAKE) -C FD502 install
	$(MAKE) -C HardDisk install
	$(MAKE) -C mpi install
	$(MAKE) -C orch90 install
	$(MAKE) -C Ramdisk install
	$(MAKE) -C SuperIDE install
