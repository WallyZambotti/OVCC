/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

/*--------------------------------------------------------------------------*/


#include <stdio.h>
#include <unistd.h>
#include "defines.h"
#include "fileops.h"
#include "resource.h"
#include "joystickinputSDL.h"
#include "vcc.h"
#include "tcc1014mmu.h"
#include "tcc1014graphicsSDL.h"
#include "tcc1014registers.h"
#include "hd6309.h"
#include "mc6809.h"
#include "mc6821.h"
#include "keyboard.h"
#include "coco3.h"
#include "pakinterface.h"
#include "audio.h"
#include "config.h"
#include "quickload.h"
#include "throttle.h"
#include "logger.h"
#include "sdl2driver.h"

SystemState2 EmuState2;
static bool DialogOpen=false;
static unsigned char Throttle=0;
static unsigned char AutoStart=1;
static unsigned char Qflag=0;
static char CpuName[20]="CPUNAME";
static char MmuName[20]="MMUNAME";

char QuickLoadFile[256];
/***Forward declarations of functions included in this code module*****/

static void SoftReset(void);
void LoadIniFile(void);
void EmuLoop(void);
void CartLoad(void);
void (*CPUInit)(void)=NULL;
int  (*CPUExec)( int)=NULL;
void (*CPUReset)(void)=NULL;
void (*CPUAssertInterupt)(unsigned char,unsigned char)=NULL;
void (*CPUDeAssertInterupt)(unsigned char)=NULL;
void (*CPUForcePC)(unsigned short)=NULL;
void FullScreenToggle(void);

// Message handlers

// Globals

char *GlobalExecFolder;
char *GlobalFullName;
char *GlobalShortName;
void HandleSDLevent(SDL_Event);
void FullScreenToggleSDL(void);
void InvalidateBoarderSDL(void);

static char g_szAppName[MAX_LOADSTRING] = "";
bool BinaryRunning;
static unsigned char FlagEmuStop=TH_RUNNING;

static AG_DriverSDL2Ghost *sdl;

void DecorateWindow(SystemState2 *);
void PrepareEventCallBacks(SystemState2 *);

/*--------------------------------------------------------------------------*/


int main(int argc, char **argv)
{
	char cwd[260];
	char name[260];

	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		GlobalExecFolder = cwd;
	} 
	else
	{
		GlobalExecFolder = argv[0];
		PathRemoveFileSpec(GlobalExecFolder);
	}

	GlobalFullName = argv[0];

	char temp1[MAX_PATH]="";
	char temp2[MAX_PATH]=" Running on ";
	AG_Thread threadID;

	// This Application Name
	strcpy(name, argv[0]);
	PathStripPath(name);
	GlobalShortName = name;

	// fprintf(stderr, "argv[0] : %s\n", GlobalFullName);
	// fprintf(stderr, "cwd     : %s\n", GlobalExecFolder);
	// fprintf(stderr, "name    : %s\n", GlobalShortName);

	strcpy(g_szAppName, GlobalShortName);

	if (argc > 1 && strlen(argv[1]) !=0)
	{
		strcpy(QuickLoadFile, argv[1]);
		strcpy(temp1, argv[1]);
		PathStripPath(temp1);
		SDL_strlwr(temp1);
		temp1[0]=toupper(temp1[0]);
		strcat (temp1, temp2);
		strcat(temp1, g_szAppName);
		strcpy(g_szAppName, temp1);
	}

	EmuState2.WindowSize.x=640;
	EmuState2.WindowSize.y=480;
	
	if (!CreateSDLWindow(&EmuState2))
	{
		fprintf(stderr,"Can't create SDL Window\n");
	}
	
	DecorateWindow(&EmuState2);
	AG_WindowShow(EmuState2.agwin);

    sdl = (AG_DriverSDL2Ghost *)((AG_Widget *)EmuState2.agwin)->drv;
    EmuState2.Window = sdl->w;
    EmuState2.Renderer = sdl->r;
	EmuState2.Texture = SDL_CreateTexture(sdl->r, sdl->f, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    EmuState2.SurfacePitch = 640;

	if (EmuState2.Texture == NULL)
	{
		fprintf(stderr, "Cannot create SDL Texture! : %s\n", SDL_GetError());
		return (0);
	}

	PrepareEventCallBacks(&EmuState2);

	ClsSDL(0, &EmuState2);

	LoadConfig(&EmuState2);			//Loads the default config file Vcc.ini from the exec directory
	EmuState2.ResetPending=2;
	SetClockSpeed(1);	//Default clock speed .89 MHZ	
	BinaryRunning = true;
	EmuState2.EmulationRunning=AutoStart;

	if ((argc > 1 && strlen(argv[1]) != 0))
	{
		Qflag=255;
		EmuState2.EmulationRunning=1;
	}

	if (AG_ThreadTryCreate(&threadID, EmuLoop, NULL) != 0)
	{
		fprintf(stderr, "Can't Start main Emulation Thread!\n");
		return(0);
	}

	EmuState2.emuThread = threadID;
	
    AG_EventLoop();
	
	//EmuState2.Pixels = NULL;
	//EmuState2.Renderer = NULL;
	//EmuState2.EmulationRunning = 0;

	//AG_ThreadCancel(threadID);

	//UnloadDll(0);
	//SoundDeInitSDL();
	//WriteIniFile(); //Save Any changes to ini FileS

	return 0;
}

/* Call backs for Events from GUI System */

void DoHardResetF9()
{
	//EmuState2.EmulationRunning=!EmuState2.EmulationRunning;
	if ( EmuState2.EmulationRunning )
		EmuState2.ResetPending=2;
	else
		SetStatusBarText("",&EmuState2);
}

void DoSoftReset()
{
	if ( EmuState2.EmulationRunning )
	{
		EmuState2.ResetPending = 1;
	}
}

void ToggleMonitorType()
{
	SetMonitorTypeSDL(!SetMonitorTypeSDL(QUERY));
}

void ToggleThrottleSpeed()
{
	SetSpeedThrottle(!SetSpeedThrottle(QUERY));
}

void ToggleScreenStatus()
{
	SetInfoBandSDL(!SetInfoBandSDL(QUERY));
    InvalidateBoarderSDL();
}

void ToggleFullScreen()
{
	if (FlagEmuStop == TH_RUNNING)
	{
		FlagEmuStop = TH_REQWAIT;
		EmuState2.FullScreen = !EmuState2.FullScreen;
	}
}

void DoKeyBoardEvent(unsigned short key, unsigned short scancode, unsigned short state)
{
	vccKeyboardHandleKeySDL(key, scancode, state);
}

void DoMouseMotion(int ex, int ey)
{
	if (EmuState2.EmulationRunning)
	{
		int x = ex;
		int y = ey;
		x /= 10;
		y /= 7.5;
		joystickSDL(x,y);
		//fprintf(stderr, "Mouse @ %i - %i\n", x, y);
	}
}

void DoButton(int button, int state)
{
	switch (button)
	{
		case SDL_BUTTON_LEFT:
			SetButtonStatusSDL(0, state);
		break;

		case SDL_BUTTON_RIGHT:
			SetButtonStatusSDL(1, state);
		break;
	}
}

void SetCPUMultiplyerFlag (unsigned char double_speed)
{
	SetClockSpeed(1); 
	EmuState2.DoubleSpeedFlag=double_speed;
	if (EmuState2.DoubleSpeedFlag)
		SetClockSpeed( EmuState2.DoubleSpeedMultiplyer * EmuState2.TurboSpeedFlag);
	EmuState2.CPUCurrentSpeed= .894;
	if (EmuState2.DoubleSpeedFlag)
		EmuState2.CPUCurrentSpeed*=(EmuState2.DoubleSpeedMultiplyer*EmuState2.TurboSpeedFlag);
	return;
}

void SetTurboMode(unsigned char data)
{
	EmuState2.TurboSpeedFlag=(data&1)+1;
	SetClockSpeed(1); 
	if (EmuState2.DoubleSpeedFlag)
		SetClockSpeed( EmuState2.DoubleSpeedMultiplyer * EmuState2.TurboSpeedFlag);
	EmuState2.CPUCurrentSpeed= .894;
	if (EmuState2.DoubleSpeedFlag)
		EmuState2.CPUCurrentSpeed*=(EmuState2.DoubleSpeedMultiplyer*EmuState2.TurboSpeedFlag);
	return;
}

unsigned char SetCPUMultiplyer(unsigned short Multiplyer)
{
	if (Multiplyer!=QUERY)
	{
		EmuState2.DoubleSpeedMultiplyer=Multiplyer;
		SetCPUMultiplyerFlag (EmuState2.DoubleSpeedFlag);
	}
	return(EmuState2.DoubleSpeedMultiplyer);
}

void DoHardReset(SystemState2* const HRState)
{	
	HRState->RamBuffer=MmuInit(HRState->RamSize);	//Alocate RAM/ROM & copy ROM Images from source
	HRState->WRamBuffer=(unsigned short *)HRState->RamBuffer;
	EmuState2.RamBuffer=HRState->RamBuffer;
	EmuState2.WRamBuffer=HRState->WRamBuffer;
	if (HRState->RamBuffer == NULL)
	{
		fprintf(stderr,"Can't allocate enough RAM, Out of memory\n");
		exit(0);
	}
	if (HRState->CpuType==1)
	{
		CPUInit=HD6309Init;
		CPUExec=HD6309Exec;
		CPUReset=HD6309Reset;
		CPUAssertInterupt=HD6309AssertInterupt;
		CPUDeAssertInterupt=HD6309DeAssertInterupt;
		CPUForcePC=HD6309ForcePC;
	}
	else
	{
		CPUInit=MC6809Init;
		CPUExec=MC6809Exec;
		CPUReset=MC6809Reset;
		CPUAssertInterupt=MC6809AssertInterupt;
		CPUDeAssertInterupt=MC6809DeAssertInterupt;
		CPUForcePC=MC6809ForcePC;
	}
	PiaReset();
	mc6883_reset();	//Captures interal rom pointer for CPU Interupt Vectors
	CPUInit();
	CPUReset();		// Zero all CPU Registers and sets the PC to VRESET
	GimeResetSDL();
	UpdateBusPointer();
	EmuState2.TurboSpeedFlag=1;
	EmuState2.TurboSpeedFlag=1;
	ResetBus();
	SetClockSpeed(1);
	return;
}

static void SoftReset(void)
{
	mc6883_reset(); 
	PiaReset();
	CPUReset();
	GimeResetSDL();
	MmuReset();
	CopyRom();
	ResetBus();
	EmuState2.TurboSpeedFlag=1;
	return;
}

unsigned char SetRamSize(unsigned char Size)
{
	if (Size!=QUERY)
	{
		EmuState2.RamSize=Size;
		EmuState2.RamSize=Size;
	}
	return(EmuState2.RamSize);
}

unsigned char SetSpeedThrottle(unsigned char throttle)
{
	if (throttle!=QUERY)
		Throttle=throttle;
	return(Throttle);
}

unsigned char SetFrameSkip(unsigned char Skip)
{
	if (Skip!=QUERY)
		EmuState2.FrameSkip=Skip;
	return(EmuState2.FrameSkip);
}

unsigned char SetCpuType( unsigned char Tmp)
{
	switch (Tmp)
	{
	case 0:
		EmuState2.CpuType=0;
		strcpy(CpuName,"MC6809");
		break;

	case 1:
		EmuState2.CpuType=1;
		strcpy(CpuName,"HD6309");
		break;
	}
	return(EmuState2.CpuType);
}

unsigned char SetMmuType(unsigned char Tmp)
{
	switch (Tmp)
	{
	case 0:
		EmuState2.MmuType=0;
		strcpy(MmuName,"Software Simulation");
		break;

	case 1:
		EmuState2.MmuType=1;
		strcpy(MmuName,"Hardware Emulation");
		break;
	}
	return(EmuState2.CpuType);
}

void DoReboot(void)
{
	EmuState2.ResetPending=2;
	return;
}

unsigned char SetAutoStart(unsigned char Tmp)
{
	if (Tmp != QUERY)
		AutoStart=Tmp;
	return(AutoStart);
}

/* Here starts the main Emulation Loop*/

void EmuLoop(void)
{
	static float FPS;
	static unsigned int FrameCounter=0;	
	CalibrateThrottle();
	AG_Delay(30);
	unsigned long LC = 0;

	//printf("Entering Emu Loop : Skip %i - Reset : %i\n", (int)EmuState2.FrameSkip, (int)EmuState2.ResetPending);

	while (1) 
	{
		if (FlagEmuStop==TH_REQWAIT)
		{
			//printf("delaying\n");
			FlagEmuStop=TH_WAITING;	//Signal Main thread we are waiting
			while(FlagEmuStop==TH_WAITING)
				AG_Delay(1);
			//printf("finished delaying\n");
		}

		FPS=0;
		if ((Qflag==255) & (FrameCounter==30))
		{
			Qflag=0;
			QuickLoad(QuickLoadFile);
		}

		StartRender();
		for (uint8_t Frames = 1; Frames <= EmuState2.FrameSkip; Frames++)
		{
			FrameCounter++;
			if (EmuState2.ResetPending != 0) {
				switch (EmuState2.ResetPending)
				{
				case 1:	//Soft Reset
					//printf("soft reset\n");
					SoftReset();
					break;

				case 2:	//Hard Reset
					//printf("hard reset\n");
					UpdateConfig();
					DoClsSDL(&EmuState2);
					DoHardReset(&EmuState2);
					break;

				case 3:
					//printf("docls\n");
					DoClsSDL(&EmuState2);
					break;

				case 4:
					//printf("upd conf\n");
					UpdateConfig();
					DoClsSDL(&EmuState2);
					break;

				default:
					break;
				}
				EmuState2.ResetPending = 0;
			}

			if (EmuState2.EmulationRunning == 1) {
				FPS += RenderFrame(&EmuState2, LC);
			} else {
				FPS += StaticSDL(&EmuState2);
			}
		}

		EndRender(EmuState2.FrameSkip);
		FPS/=EmuState2.FrameSkip;
		GetModuleStatus(&EmuState2);

		char ttbuff[256];
		sprintf(ttbuff,"Skip:%2.2i|FPS:%3.0f|%s@%3.2fMhz|%s",EmuState2.FrameSkip,FPS,CpuName,EmuState2.CPUCurrentSpeed,EmuState2.StatusLine);
		SetStatusBarText(ttbuff,&EmuState2);

		if (Throttle )	//Do nothing until the frame is over returning unused time to OS
			FrameWait();
	} //Still Emulating
	return;
}

void FullScreenToggleSDL(void)
{
	bool SDLrecreateTexture(SystemState2*);

	EmuState2.EmulationRunning = 0;
	PauseAudioSDL(true);

	if (EmuState2.FullScreen)
	{	
		SDL_SetWindowFullscreen(EmuState2.Window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else{
		SDL_SetWindowFullscreen(EmuState2.Window, 0);
	}

	EmuState2.Resizing = 0;
	EmuState2.EmulationRunning = 1;
	InvalidateBoarderSDL();
	EmuState2.ConfigDialog=NULL;
	PauseAudioSDL(false);
	return;
}
