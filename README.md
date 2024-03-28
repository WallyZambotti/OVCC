# OVCC 1.6.1

The portable and open Virtual Colo(u)r Computer. GNU General Public License.

Developed from VCC 1.43 (2.01) by Joseph Forgione. GNU General Public License.

Special thanks to Paul Zimmerman for providing Mac patches for OVCC and AGAR.  OVCC patches are pre-integrated.

Binaries for Linux and Windows and Mac can be downloaded from here:

https://drive.google.com/drive/folders/1xL8jWnwWLKL9D6K5ZNHIYRGNySIipEKV?usp=sharing

A Mac binary is hosted here (By Paul Zimmerman):

https://drive.google.com/drive/folders/1Va9Vq35dOAkd4joCtuB7zRYQ8CY6mssN?usp=sharing

OVCC Dependancies:

AGAR (libraries)

The binary library for Windows are available from the AGAR download site. https://libagar.org/download.html
As of version 1.7.1 AGAR no longer provides the binary versions.

The binary libraries for Ubuntu 18.04 & 20.04 are available from:

https://drive.google.com/drive/folders/1V2a27j_n9BoMDaHfvzDtAXcfv_IL_TBV?usp=sharing

SDL2 (libraries) (libSDL2-dev)

Install SDL2 (dev) as per your distribution guidelines.

Compiling AGAR.

Included with the OVCC is a directory containing patches for AGAR for Mac builds.  Not required if building for anything else.
They are located ovcc/Patches/AGAR.  Apply the patches before building AGAR. Patches kindly provided by Paul Zimmerman.

If compiling the original AGAR 1.7.0 source is available from https://libagar.org/download.html
however it is recommended that you download the source from the github site https://github.com/JulNadeauCA/libagar.git
this is version 1.7.1 and contains fixes that affect ovcc.

AGAR also has its dependancies so read the compilation documentation for the relevant platforms here https://libagar.org/docs/

Build AGAR as per AGAR documentation with the exception/addition of the configuration options mentioned in the forementioned Docs.

Compiling OVCC

Once AGAR 1.7.1 and SDL2 are installed clone this OVCC repository change into the top directory make.
  
$ make

------------

To make the isolated CPU version you will need to follow these manual steps:

1. After completeling a success make.
2. cd into the CoCo dir (cd CoCo)
3. rm obj/coco3.o obj/vcc.o obj/vccgui.o
4. make -f Makefiles/Linux/makefile-isocpu

The executable ovcc-isocpu should be created.

------------

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
When loading roms and devices into ovcc you will be asked to navigate and select the necessary rom/device(library) so they could be anywhere. However having an organised folder structure will make manualy editing the Vcc.ini (if required) easier.

Ovcc can be started from a terminal or added to your desktop GUI menu. On windows it should be started from a bat file that defines the paths to necessary Mingw libraries

------------

The necessary Windows (Mingw runtime libraries) can be found here:

https://drive.google.com/drive/folders/1V2a27j_n9BoMDaHfvzDtAXcfv_IL_TBV?usp=sharing

If you have built your Mingw environment correctly you should not need them.  However if you are copying the Mingw binaries to another system the above link identifies all the libraries you will also need to copy.

------------

Linux & Windows 10 PS4 Joystick support

You can find instructions for setting up Linux driver support for PS4 controllers here:

https://github.com/chrippa/ds4drv

And for Windows here :

http://ds4windows.com

------------

Mac developers need to be aware that OVCC uses the cocoa graphics driver.  This means that if ovcc is started from a terminal the keyboard I/O will not be directed to the application but will instead remain with the terminal that started ovcc.

To overcome this ovcc should be started from a bundle.  The ovcc repo now provides support for building on Mac OS using xCode. Install xCode and the xCode developement extension tools.  Use brew to install the necessary libraries as described on the LIB AGAR site. Build AGAR as well.  Install the dylibbuilder uitility found here: https://github.com/auriamg/macdylibbundler.git and apply the fix found here: https://github.com/auriamg/macdylibbundler/issues/77.  I found it quite tricky to setup xCode to make it build and install OVCC.  It requires creating targets and editing the schema.  It is easer to just do it from a terminal.  If everything builds cleanly then make install will build the bundle.  Launch OVCC by double clicking on the ovcc.app (bundle) file.  The ovcc.app can be copied to the Applications folder.

------------

For Ubuntu developers there were a number of missing dependencies not mentioned on the AGAR web site.  These missing dependencies will not stop AGAR from building but will mean certain options like OpenGL support are
missing:

ligopengl-dev (provides the obvious)
mesa-common-dev (provides gl.h)
libglu1-mesa-dev (provides glu.h)

These other optional dependencies may also be usefull:

```libfreetype6-dev (very useful)
libfontconfig-dev (very useful)
libjpeg8-dev (very useful)
libjpeg8-turbo-dev (very useful)
libpng-dev (very useful)
libxinerama-dev  (optional)
libsndio-dev
libsndio6.1 (or 6,  SDL2 will not work with version 7)
```

Once all these dependencies have been added there is no need to change the AGAR configure. Just do this :

```$ cd libagar
$ export CFLAGS=-O2
$ ./configure
$ make depend all
$ sudo make install
```

(The export CFLAGS will ensure the build is performed with compiler optimizations.)
