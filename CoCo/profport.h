/***************************************************************************
 PORTABLE ROUTINES FOR WRITING PRIVATE PROFILE STRINGS --  by Joseph J. Graf
 Header file containing prototypes and compile-time configuration.
***************************************************************************/
#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

#define MAX_LINE_LENGTH    400

int ADDCALL GetPrivateProfileInt(char *, char *, int,    char *);
int ADDCALL GetPrivateProfileString(char *, char *, char *, char *, int, char *);
int ADDCALL WritePrivateProfileString(char *, char *, char *, char *);
int ADDCALL WritePrivateProfileInt(char *, char *, int, char *);
