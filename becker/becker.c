#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <agar/core.h>
#include <agar/gui.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "becker.h"

#define MAX_PATH 260

#ifndef __MINGW32__
typedef int boolean;
#endif
typedef int BOOL;

static char moduleName[17] = { "HDBDOS/DW/Becker" };

// socket
static int dwSocket = 0;

// vcc stuff
typedef void (*SETCART)(unsigned char);
typedef void (*SETCARTPOINTER)(SETCART);
static void (*PakSetCart)(unsigned char)=NULL;
static char IniFile[MAX_PATH]="";
static unsigned char HDBRom[8192];
static bool DWTCPEnabled = false;

// are we retrying tcp conn
static bool retry = false;

// circular buffer for socket io
static char InBuffer[BUFFER_SIZE];
static int InReadPos = 0;
static int InWritePos = 0;

// statistics
static int BytesWrittenSince = 0;
static int BytesReadSince = 0;
static long LastStats = 0;
static float ReadSpeed = 0;
static float WriteSpeed = 0;

// hostname and port

static char dwaddress[MAX_PATH];
static unsigned short dwsport = 65504;
static char curaddress[MAX_PATH];
static unsigned short curport = 65504;
static char serverPort[16];


//thread handle
static AG_Thread hDWTCPThread;

// scratchpad for msgs
char msg[MAX_PATH];

// log lots of stuff...
static boolean logging = false;

static void (*DynamicMenuCallback)( char *,int, int)=NULL;
unsigned char LoadExtRom(char *);
void SetDWTCPConnectionEnable(unsigned int enable);
int dw_setaddr(char *bufdwaddr);
int dw_setport(char *bufdwport);
void WriteLog(char *Message,unsigned char Type);
void BuildDynaMenu(void);
void LoadConfig(void);
void SaveConfig(void);

AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemConfig = NULL;
AG_MenuItem *itemSeperator = NULL;


void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("becker is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("becker is exited\n"); 
}

// coco checks for data
unsigned char dw_status( void )
{
	// check for input data waiting

	if (retry | (dwSocket == 0) | (InReadPos == InWritePos))
		return(0);
	else
		return(1);
}

// coco reads a byte
unsigned char dw_read( void )
{
	// increment buffer read pos, return next byte
	unsigned char dwdata = InBuffer[InReadPos];

	InReadPos++;

	if (InReadPos == BUFFER_SIZE)
		InReadPos = 0;

	BytesReadSince++;

	return(dwdata);
}

// coco writes a byte
int dw_write( char dwdata)
{
	// send the byte if we're connected
	if ((dwSocket != 0) & (!retry))
	{	
		int res = send(dwSocket, &dwdata, 1, 0);
		if (res != 1)
		{
			sprintf(msg,"dw_write: socket error\n");
			fprintf(stderr, msg);
#ifdef __MINGW32__
			closesocket(dwSocket);
#else
			close(dwSocket);
#endif
			dwSocket = 0;        
		}
		else
		{
			BytesWrittenSince++;
		}
	}
		else
	{
		sprintf(msg,"coco write but null socket\n");
		fprintf(stderr, msg);
	}

	return(0);
}

void killDWTCPThread(void)
{
	// close socket to cause io thread to die
	if (dwSocket != 0)
#ifdef __MINGW32__
			closesocket(dwSocket);
#else
			close(dwSocket);
#endif

	dwSocket = 0;
	
	// reset buffer po
	InReadPos = 0;
	InWritePos = 0;
}

// set our hostname, called from config.c
int dw_setaddr(char *bufdwaddr)
{
	strcpy(dwaddress,bufdwaddr);
	return(0);
}


// set our port, called from config.c
int dw_setport(char *bufdwport)
{
	dwsport = (unsigned short)atoi(bufdwport);

	if ((dwsport != curport) || (strcmp(dwaddress,curaddress) != 0))
	{
		// host or port has changed, kill open connection
		killDWTCPThread();
	}

	return(0);
}

// try to connect with DW server
void attemptDWConnection( void )
{

	retry = true;
	BOOL bOptValTrue = true;
	int iOptValTrue = 1;

	strcpy(curaddress, dwaddress);
	curport= dwsport;

	sprintf(msg,"Connecting to %s:%d... \n",dwaddress,dwsport);
	fprintf(stderr, msg);

	// resolve hostname
	struct hostent *dwSrvHost = gethostbyname(dwaddress);
	
	if (dwSrvHost == NULL)
	{
		// invalid hostname/no dns
		retry = false;
//              WriteLog("failed to resolve hostname.\n",TOCONS);
	}
	
	// allocate socket
#ifdef __MINGW32__
	dwSocket = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP);
#else
	dwSocket = socket (AF_INET,SOCK_STREAM,0);
#endif
	if (dwSocket == -1)
	{
		// no deal
		retry = false;
		fprintf(stderr,"invalid socket.\n");
	}

	// set options
#ifdef __MINGW32__
	setsockopt(dwSocket,IPPROTO_TCP,SO_REUSEADDR,(char *)&bOptValTrue,sizeof(bOptValTrue));
	setsockopt(dwSocket,IPPROTO_TCP,TCP_NODELAY,(char *)&iOptValTrue,sizeof(iOptValTrue));  
#else
	setsockopt(dwSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&bOptValTrue,sizeof(bOptValTrue));
	//setsockopt(dwSocket,SOL_SOCKET,TCP_NODELAY,(char *)&iOptValTrue,sizeof(iOptValTrue));  
#endif
	// build server address

#ifdef __MINGW32__
	SOCKADDR_IN dwSrvAddress;
#else
	struct sockaddr_in dwSrvAddress;
#endif

	dwSrvAddress.sin_family = AF_INET;
	dwSrvAddress.sin_addr = *((struct in_addr*)*dwSrvHost->h_addr_list);
	dwSrvAddress.sin_port = htons(dwsport);
	
	// try to connect...

#ifdef __MINGW32__
	int rc = connect(dwSocket, (LPSOCKADDR)&dwSrvAddress, sizeof(dwSrvAddress));
#else
	int rc = connect(dwSocket, &dwSrvAddress, sizeof(dwSrvAddress));
#endif

	retry = false;

#ifdef __MINGW32__
	if (rc==SOCKET_ERROR)
#else
	if (rc==-1)
#endif
	{
		// no deal
//              WriteLog("failed to connect.\n",TOCONS);
#ifdef __MINGW32__
		closesocket(dwSocket);
#else
		close(dwSocket);
#endif
		dwSocket = 0;
	}
	
}

// TCP connection thread
unsigned  DWTCPThread(void)
{
#ifdef __MINGW32__
	WSADATA wsaData;
#endif

	int sz;
	int res;

		// Request Winsock version 2.2

#ifdef __MINGW32__
	if ((WSAStartup(0x202, &wsaData)) != 0)
	{
		fprintf(stderr, "WSAStartup() failed, DWTCPConnection thread exiting\n");
		WSACleanup();
		return(0);
	}
#endif

	while(DWTCPEnabled)
	{
		// get connected
		attemptDWConnection();

		// keep trying...
		while ((dwSocket == 0) & DWTCPEnabled)
		{
			attemptDWConnection();

			// after 2 tries, sleep between attempts
			usleep(TCP_RETRY_DELAY*1000);
		}
		
		while ((dwSocket != 0) & DWTCPEnabled)
		{
			// we have a connection, lets chew through some i/o
			
			// always read as much as possible, 
			// max read is writepos to readpos or buffersize
			// depending on positions of read and write ptrs

			if (InReadPos > InWritePos)
				sz = InReadPos - InWritePos;
			else
				sz = BUFFER_SIZE - InWritePos;

			// read the data
			res = recv(dwSocket,(char *)InBuffer + InWritePos, sz, 0);

			if (res < 1)
			{
				// no good, bail out
#ifdef __MINGW32__
				closesocket(dwSocket);
#else
				close(dwSocket);
#endif
				dwSocket = 0;
			} 
			else
			{
				// good recv, inc ptr
				InWritePos += res;
				if (InWritePos == BUFFER_SIZE)
					InWritePos = 0;
						
			}

		}

	}

	// close socket if necessary
	if (dwSocket != 0)
#ifdef __MINGW32__
		closesocket(dwSocket);
#else
		close(dwSocket);
#endif
			
	dwSocket = 0;

	return(0);
}

// called from config.c/UpdateConfig
void SetDWTCPConnectionEnable(unsigned int enable)
{
	// turning us on?
	if ((enable == 1) & (!DWTCPEnabled))
	{
		DWTCPEnabled = true;

		// WriteLog("DWTCPConnection has been enabled.\n",TOCONS);

		// reset buffer pointers
		InReadPos = 0;
		InWritePos = 0;

		// start it up...
	
		if (AG_ThreadTryCreate(&hDWTCPThread, DWTCPThread, NULL)!=0)
		{
			fprintf(stderr, "Cannot start DWTCPConnection thread!\n");
			return;
		}

		sprintf(msg,"DWTCPConnection thread started\n");
		fprintf(stderr, msg);

	}
	else if ((enable != 1) & DWTCPEnabled)
	{
		// we were running but have been turned off
		DWTCPEnabled = false;

		killDWTCPThread();

		// WriteLog("DWTCPConnection has been disabled.\n",TOCONS);
	
	}

}

// dll exported functions

void ADDCALL ModuleName(char *ModName, AG_MenuItem *Temp)
{

	menuAnchor = Temp;
	strcpy(ModName, moduleName);

	if (menuAnchor != NULL) 
	{
		BuildMenu();
	}

	return ;
}

void ADDCALL PackPortWrite(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
		// write data 
		case 0x42:
			dw_write(Data);
			break;
	}
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	switch (Port)
	{
		// read status
		case 0x41:
			if (dw_status() != 0)
				return(2);
			else
				return(0);
			break;
		// read data 
		case 0x42:
			return(dw_read());
			break;
	}

	return 0;
}
/*
	__declspec(dllexport) unsigned char ModuleReset(void)
	{
		if (PakSetCart!=NULL)
			PakSetCart(1);
		return(0);
	}
*/
unsigned char ADDCALL SetCart(SETCART Pointer)
{
	
	PakSetCart=Pointer;
	return(0);
}

unsigned char ADDCALL PakMemRead8(unsigned short Address)
{
	//sprintf(msg,"PalMemRead8: addr %d  val %d\n",(Address & 8191), Rom[Address & 8191]);
	//WriteLog(msg,TOCONS);
	return(HDBRom[Address & 8191]);

}

void ADDCALL HeartBeat(void)
{
	// flush write buffer in the future..?
	return;
}

void ADDCALL ModuleStatus(char *DWStatus)
{
	// calculate speed
	struct timespec now;

	#define CLOCK_MONOTONIC 1 // Not picking up define from time.h for some reason?

	clock_gettime(CLOCK_MONOTONIC, &now);

	long sinceCalc = ((now.tv_sec * 1000) + (now.tv_nsec / 1000000)) - LastStats;
	
	if (sinceCalc > STATS_PERIOD_MS)
	{
		LastStats += sinceCalc;
		
		ReadSpeed = 8.0f * (BytesReadSince / (1000.0f - sinceCalc));
		WriteSpeed = 8.0f * (BytesWrittenSince / (1000.0f - sinceCalc));

		BytesReadSince = 0;
		BytesWrittenSince = 0;
	}
        
	if (DWTCPEnabled)
	{
		if (retry)
		{
				sprintf(DWStatus,"DW: Try %s", curaddress);
		}
		else if (dwSocket == 0)
		{
				sprintf(DWStatus,"DW: ConError");
		}
		else
		{
			int buffersize = InWritePos - InReadPos;
			if (InReadPos > InWritePos)
				buffersize = BUFFER_SIZE - InReadPos + InWritePos;

			
			sprintf(DWStatus,"DW: ConOK  R:%04.1f W:%04.1f", ReadSpeed , WriteSpeed);
		}
	}
	else
	{
		sprintf(DWStatus,"");
	}
	return;
}

void OKCallback(AG_Event *event)
{
	dw_setaddr(dwaddress);
	dw_setport(serverPort);
	SaveConfig();
	AG_CloseFocusedWindow();
}

void ConfigBecker(AG_Event *event)
{
	AG_Window *win;

    if ((win = AG_WindowNewNamedS(0, "DriveWire Server")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 420, 150);
    AG_WindowSetCaptionS(win, "DriveWire Server");
    AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);

	// Server Address

	AG_Textbox *tbx = AG_TextboxNew(win, AG_TEXTBOX_HFILL, "Server Address:", NULL);
	AG_TextboxBindASCII(tbx, dwaddress, sizeof(dwaddress));
	AG_TextboxSizeHint(tbx, "127.0.0.1 or some long name");

	// Server Port

	sprintf(serverPort, "%d", dwsport);
	tbx = AG_TextboxNew(win, AG_TEXTBOX_HFILL, "Server Port:", NULL);
	AG_TextboxBindASCII(tbx, serverPort, sizeof(serverPort));
	AG_TextboxSizeHint(tbx, "65504 or whatever");
	
	// OK & Cancel buttons

	AG_HBox* hbox = AG_BoxNewHoriz(win, 0);

	AG_ButtonNewFn(hbox, 0, "OK", OKCallback, NULL);
	AG_ButtonNewFn(hbox, 0, "Cancel", AGWINDETACH(win));

	AG_WindowShow(win);
}

void BuildMenu(void)
{
	if (itemConfig == NULL)
	{
        itemSeperator = AG_MenuSeparator(menuAnchor);
	    itemConfig = AG_MenuNode(menuAnchor, "DriveWire Server", NULL);
		AG_MenuAction(itemConfig, "Config", NULL, ConfigBecker, NULL);
	}
}

void ADDCALL ModuleConfig(unsigned char func)
{
	switch(func)
	{
	case 0: // Destroy Menus
	{
		AG_MenuDel(itemConfig);
		AG_MenuDel(itemSeperator);
	}
	break;

	default:
		break;
	}

	return;
}

void ADDCALL SetIniPath(char *IniFilePath)
{
	strcpy(IniFile,IniFilePath);
	LoadConfig();
	SetDWTCPConnectionEnable(1);
	return;
}

void LoadConfig(void)
{
	char saddr[MAX_LOADSTRING]="";
	char sport[MAX_LOADSTRING]="";
	char DiskRomPath[MAX_PATH];

	GetPrivateProfileString(moduleName,"DWServerAddr","",saddr,MAX_PATH,IniFile);
	GetPrivateProfileString(moduleName,"DWServerPort","",sport,MAX_PATH,IniFile);
	
	if (strlen(saddr) > 0)
		dw_setaddr(saddr);
	else
		dw_setaddr("127.0.0.1");

	if (strlen(sport) > 0)
		dw_setport(sport);
	else
		dw_setport("65504");
	
    getcwd(DiskRomPath, MAX_PATH);
	strcat(DiskRomPath, "/hdbdwbck.rom");
	LoadExtRom(DiskRomPath);
}

void SaveConfig(void)
{
	WritePrivateProfileString(moduleName,"DWServerAddr",dwaddress,IniFile);
	sprintf(msg, "%d", dwsport);
	WritePrivateProfileString(moduleName,"DWServerPort",msg,IniFile);
	return;
}

unsigned char LoadExtRom(char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle = NULL;
	unsigned short index = 0;
	unsigned char RetVal = 0;

	rom_handle = fopen(FilePath, "rb");
	if (rom_handle == NULL)
		memset(HDBRom, 0xFF, 8192);
	else
	{
		while ((feof(rom_handle) == 0) & (index<8192))
			HDBRom[index++] = fgetc(rom_handle);
		RetVal = 1;
		fclose(rom_handle);
	}
	return(RetVal);
}