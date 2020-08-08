#include <agar/core.h>
#include <stdio.h>
#include <string.h>

#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

static char moduleName[8] = { "testdev" };


void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("ramdisk is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("ramdisk is exited\n"); 
}

void ADDCALL ModuleName(char *ModName, void *Temp)
{
	ModName = strcpy(ModName, moduleName);

    extern AG_Object agDrivers; 
    fprintf(stderr, "&agDrivers %p\n", &agDrivers);

	return ;
}
