#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

void* getFunctionPointer(void* lib, const char* funcName) {
 //
 // Get the function pointer to the function
    void* fptr = SDL_LoadFunction(lib, funcName);
    if (!fptr) {
      fprintf(stderr, "Could not get function pointer for %s\n  error is: %s\n", funcName, SDL_GetError());
      return NULL;
    }
    return fptr;
}

int main(int argc, char** argv) 
{
   char moduleName[128] = "";

 //
 // Declare the function pointers:
    void (*fptr_null      )(int);
    void (*fptr_ModuleName)(char*, void*);

 //
 // Open the dynamic library
    void* hdd_lib = SDL_LoadObject(argv[1]);

    if (!hdd_lib) {
     //
     // Apparently, the library could not be opened
        fprintf(stderr, "Could not open %s\n", argv[1]);
        fprintf(stderr, "%s\n", SDL_GetError());
        exit(1);
    }

 //
 // Get the pointers to the functions within the library:
    fptr_null      =getFunctionPointer(hdd_lib, "doesNotExist");
    fptr_ModuleName=getFunctionPointer(hdd_lib, argv[2]);

    if (fptr_ModuleName) fptr_ModuleName(moduleName, NULL);

   fprintf(stderr, "Module name : %s\n", moduleName);
}
