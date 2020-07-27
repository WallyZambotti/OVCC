# OVCC

(This is the previous guide for OVCC upto 1.2.1 that was dependent on a custom version of AGAR 1.5.0 that included SDL2 driver support)

The portable and open Virtual Colo(u)r Computer. GNU General Public License.

Developed from VCC 1.43 (2.01) by Joseph Forgione. GNU General Public License.


Compiling guide.


OVCC Dependancies:

the original AGAR 1.5.0 source is available from https://libagar.org/download.html

AGAR also has its dependancies so read the compilation documentation for the relevant platforms here https://libagar.org/docs/

OVCC requires additional AGAR SDL2 drivers and feature patches in order to compile.

The source for those additional patches and drivers are located here:

https://drive.google.com/drive/folders/1AwqpwM3JeLzOXsBsxrChQUqCO-TRxheG?usp=sharing

Alternatively you can download a copy of the source (for Linux/OSX) with the patches already installed from: 

https://github.com/WallyZambotti/AGAR-SDL2  (does not contain the Docs directory)

Look in the Docs directory for instructions on including the patches for your platform. (Mingw or Linux/OSX)  The github version does not contain the Docs directory but there is an included readme that the github repo brings to your attention.

Build AGAR as per AGAR documentation with the exception/addition of the configuration options mentioned in the forementioned Docs.

./configure --with-sdl2 --without-sdl  # if you don't intend to debug AGAR

./configure --enable-debug --with-sdl2 --without-sdl # if you intend to DEBUG AGAR

./configure --with-sdl2 --without-sdl --with-libtool=/usr/bin/libtool # if the configuration can't locate the libtool

( sdl2 driver cannot coexist with the sdl driver -
  you may need to explicitly define the "libtool" for the configuration to use if the 'make depends all' fails with libtool errors)
  
  Once AGAR is built and installed clone this OVCC repository change into the top directory and locate the makefile.
  
  Edit the two first lines of the makefile to reflect you environment:
  
export TARGETOS = Linux  # options are Linux or Mingw

export TARGETARCH = AMD  # options are AMD (Intel) or ARM

Then make.

After OVCC and all device libraries are built OVCC needs to be able to find the libraries.  The easiest approach is to copy the ovcc(.exe) executable to a clean directory and place the libraries in a sub directory of that folder.

There are some required roms to get OVCC working and these are compatible with VCC roms. (Not supplied)  They should be in same directory as the ovcc executable.  Optional roms can be placed in a roms sub directory.

OVCC configures itself from an ini file (Vcc.ini) in the same directory as the executable. If one is not supplied it will be generated (and this is fine).

Floppy disk images and hard disk images can also be placed in sub folders for convenience.
```
Linux/OSX
.../OVCC/
        ovcc
        coco3.rom
        disk11.rom
        rgbdos.rom
        Vcc.ini
        libs/
              libmpi.so
              libharddisk.so
              lib_etc_.so
        roms/
              orch90.rom
              hdblba.rom
              hdbdw3bc3 w-offset 5A000.rom
              etc_etc.rom
        dsks/
              floppy_image_etc.dsk
        vhds/
              VCCEmuDisk.vhd
```
```
Mingw(Windows)
...\OVCC\
        ovcc.exe
        coco3.rom
        disk11.rom
        rgbdos.rom
        Vcc.ini
        libs\
              mpi.dll
              harddisk.dll
              etc_.dll
        roms\
              orch90.rom
              hdblba.rom
              hdbdw3bc3 w-offset 5A000.rom
              etc_etc.rom
        dsks\
              floppy_image_etc.dsk
        vhds\
              VCCEmuDisk.vhd
```
When loading roms and devices into ovcc you will be asked to navigate and select the necessary rom/device(library) so they could be anywhere. However having an organised folder structure will make manually editing the Vcc.ini (if required) easier.

Ovcc can be started from a terminal or added to your desktop GUI menu. On windows it should be started from a bat file that defines the paths to necessary Mingw libraries

The necessary Windows (Mingw runtime libraries) can be found here:

https://drive.google.com/drive/folders/1V2a27j_n9BoMDaHfvzDtAXcfv_IL_TBV?usp=sharing

If you have built your Mingw environment correctly you should not need them.  However if you are copying the Mingw binaries to another system the above link identifies all the libraries you will also need to copy.

Linux & Windows 10 PS4 Joystick support

You can find instructions for setting up Linux driver support for PS4 controllers here:

https://github.com/chrippa/ds4drv

And for Windows here :

http://ds4windows.com
