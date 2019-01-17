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

#define DIRECTINPUT_VERSION 0x0800

#include <stdio.h>

#include "defines.h"
#include "resource.h"
#include "config.h"
#include "tcc1014graphicsSDL.h"
#include "mc6821.h"
#include "vcc.h"
#include "tcc1014mmu.h"
#include "audio.h"
#include "pakinterface.h"
#include "vcc.h"
#include "joystickinputSDL.h"
#include "keyboard.h"
#include "fileops.h"
#include "cassette.h"

//#include "logger.h"
#include <assert.h>

extern JoyStick	LeftSDL;
extern JoyStick RightSDL;

extern char *GlobalExecFolder;

//
// forward declarations
//
unsigned char XY2Disp(unsigned char, unsigned char);
void Disp2XY(unsigned char *, unsigned char *, unsigned char);

int SelectFile(char *);
void RefreshJoystickStatus(void);

#define MAXCARDS 12

//
//	global variables
//
static unsigned short int	Ramchoice[4]={IDC_128K,IDC_512K,IDC_2M,IDC_8M};
static unsigned  int	LeftJoystickEmulation[3] = { IDC_LEFTSTANDARD,IDC_LEFTTHIRES,IDC_LEFTCCMAX };
static unsigned int	RightJoystickEmulation[3] = { IDC_RIGHTSTANDARD,IDC_RIGHTTHRES,IDC_RIGHTCCMAX };
static unsigned short int	Cpuchoice[2]={IDC_6809,IDC_6309};
static unsigned short int	Monchoice[2]={IDC_COMPOSITE,IDC_RGB};
static unsigned char temp=0,temp2=0;
static char IniFileName[]="Vcc.ini";
static char IniFilePath[MAX_PATH]="";
static char TapeFileName[MAX_PATH]="";
static char ExecDirectory[MAX_PATH]="";
static char SerialCaptureFile[MAX_PATH]="";
static char TextMode=1,PrtMon=0;;
static unsigned char NumberofJoysticks=0;
char OutBuffer[MAX_PATH]="";
char AppName[MAX_LOADSTRING]="";
STRConfig CurrentConfig;
static STRConfig TempConfig;
extern SystemState2 EmuState2;
extern char *StickName[MAXSTICKS];

static unsigned int TapeCounter=0;
static unsigned char Tmode=STOP;
char Tmodes[4][10]={"STOP","PLAY","REC","STOP"};
static int NumberOfSoundCards=0;

static JoyStick TempLeft, TempRight;

static SndCardList SoundCards[MAXCARDS];

#define SCAN_TRANS_COUNT	375

unsigned short _TranslateDisp2Scan[SCAN_TRANS_COUNT] = 
	{
		AG_KEY_NONE, AG_KEY_ESCAPE, AG_KEY_1, AG_KEY_2, AG_KEY_3, AG_KEY_4, AG_KEY_5, AG_KEY_6, AG_KEY_7, AG_KEY_8, AG_KEY_9, AG_KEY_0,
		AG_KEY_MINUS, AG_KEY_EQUALS, AG_KEY_BACKSPACE, AG_KEY_TAB,
		AG_KEY_A, AG_KEY_B, AG_KEY_C, AG_KEY_D, AG_KEY_E, AG_KEY_F, AG_KEY_G, AG_KEY_H, AG_KEY_I, AG_KEY_J, AG_KEY_K, AG_KEY_L, AG_KEY_M,
		AG_KEY_N, AG_KEY_O, AG_KEY_P, AG_KEY_Q, AG_KEY_R, AG_KEY_S, AG_KEY_T, AG_KEY_U, AG_KEY_V, AG_KEY_W, AG_KEY_X, AG_KEY_Y, AG_KEY_Z,
		AG_KEY_LEFTBRACKET, AG_KEY_RIGHTBRACKET, AG_KEY_BACKSLASH, AG_KEY_SEMICOLON, AG_KEY_QUOTE, AG_KEY_COMMA, AG_KEY_PERIOD,
		AG_KEY_SLASH, AG_KEY_CAPSLOCK, AG_KEY_LSHIFT, AG_KEY_LCTRL, AG_KEY_LALT, AG_KEY_SPACE, AG_KEY_RETURN, AG_KEY_INSERT,
		AG_KEY_DELETE, AG_KEY_HOME, AG_KEY_END, AG_KEY_PAGEUP, AG_KEY_PAGEDOWN, AG_KEY_LEFT, AG_KEY_RIGHT, AG_KEY_UP, AG_KEY_DOWN,
		AG_KEY_F1, AG_KEY_F2
	};
unsigned short _TranslateScan2Disp[SCAN_TRANS_COUNT];

unsigned short TranslateDisp2Scan(int x)
{
	assert(x >= 0 && x < SCAN_TRANS_COUNT);

	return _TranslateDisp2Scan[x];
}

unsigned short TranslateScan2Disp(int x)
{
	assert(x >= 0 && x < SCAN_TRANS_COUNT);

	return _TranslateScan2Disp[x];
}

void buildTransScan2DispTable()
{
	for (int Index = 0; Index < SCAN_TRANS_COUNT; Index++)
	{
		for (int Index2 = SCAN_TRANS_COUNT - 1; Index2 >= 0; Index2--)
		{
			if (Index2 == _TranslateDisp2Scan[Index])
			{
				_TranslateScan2Disp[Index2] = (unsigned char)Index;
			}
		}
	}
}

/*****************************************************************************/

void LoadConfig(SystemState2 *LCState)
{
	extern const char *GlobalShortName;

	buildTransScan2DispTable();

	strcpy(AppName, GlobalShortName);
	strcpy(ExecDirectory, GlobalExecFolder);
	strcpy(CurrentConfig.PathtoExe,ExecDirectory);
	strcat(CurrentConfig.PathtoExe,GetPathDelimStr());
	strcat(CurrentConfig.PathtoExe,AppName);
	strcpy(IniFilePath,ExecDirectory);
	strcat(IniFilePath,GetPathDelimStr());
	strcat(IniFilePath,IniFileName);
	LCState->ScanLines = 0;
	NumberOfSoundCards = GetSoundCardListSDL(SoundCards);
	ReadIniFile();
	CurrentConfig.RebootNow=0;
	UpdateConfig();
	RefreshJoystickStatus();
	SoundInitSDL(SoundCards[CurrentConfig.SndOutDev].sdlID, CurrentConfig.AudioRate);
	InsertModule (CurrentConfig.ModulePath);	// Should this be here?

	if (!AG_FileExists(IniFilePath))
		WriteIniFile();
}

unsigned char WriteNamedIniFile(char *iniFilePath)
{
	//fprintf(stderr, "Write Ini File : %s\n", iniFilePath);

	GetCurrentModule(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ExternalBasicImage);
	WritePrivateProfileString("Version","Release",AppName,iniFilePath);

	WritePrivateProfileInt("CPU","DoubleSpeedClock",CurrentConfig.CPUMultiplyer,iniFilePath);
	WritePrivateProfileInt("CPU","FrameSkip",CurrentConfig.FrameSkip,iniFilePath);
	WritePrivateProfileInt("CPU","Throttle",CurrentConfig.SpeedThrottle,iniFilePath);
	WritePrivateProfileInt("CPU","CpuType",CurrentConfig.CpuType,iniFilePath);
	WritePrivateProfileInt("CPU", "MaxOverClock", CurrentConfig.MaxOverclock, iniFilePath);
	WritePrivateProfileInt("CPU","MmuType",CurrentConfig.MmuType,iniFilePath);

	WritePrivateProfileString("Audio","SndCard",CurrentConfig.SoundCardName,iniFilePath);
	WritePrivateProfileInt("Audio","Rate",CurrentConfig.AudioRate,iniFilePath);

	WritePrivateProfileInt("Video","MonitorType",CurrentConfig.MonitorType,iniFilePath);
	WritePrivateProfileInt("Video","ScanLines",CurrentConfig.ScanLines,iniFilePath);
	WritePrivateProfileInt("Video","AllowResize",CurrentConfig.Resize,iniFilePath);
	WritePrivateProfileInt("Video","ForceAspect",CurrentConfig.Aspect,iniFilePath);

	WritePrivateProfileInt("Memory","RamSize",CurrentConfig.RamSize,iniFilePath);
	WritePrivateProfileString("Memory", "ExternalBasicImage", CurrentConfig.ExternalBasicImage, iniFilePath);

	WritePrivateProfileInt("Misc","AutoStart",CurrentConfig.AutoStart,iniFilePath);
	WritePrivateProfileInt("Misc","CartAutoStart",CurrentConfig.CartAutoStart,iniFilePath);
	WritePrivateProfileInt("Misc","KeyMapIndex",CurrentConfig.KeyMap,iniFilePath);

	ValidatePath(CurrentConfig.ModulePath);
	WritePrivateProfileString("Module", "OnBoot", CurrentConfig.ModulePath, iniFilePath);

	WritePrivateProfileInt("LeftJoyStick","UseMouse",LeftSDL.UseMouse,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Left",LeftSDL.Left,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Right",LeftSDL.Right,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Up",LeftSDL.Up,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Down",LeftSDL.Down,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire1",LeftSDL.Fire1,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire2",LeftSDL.Fire2,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick","DiDevice",LeftSDL.DiDevice,iniFilePath);
	WritePrivateProfileInt("LeftJoyStick", "HiResDevice", LeftSDL.HiRes, iniFilePath);
	WritePrivateProfileInt("RightJoyStick","UseMouse",RightSDL.UseMouse,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Left",RightSDL.Left,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Right",RightSDL.Right,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Up",RightSDL.Up,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Down",RightSDL.Down,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire1",RightSDL.Fire1,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire2",RightSDL.Fire2,iniFilePath);
	WritePrivateProfileInt("RightJoyStick","DiDevice",RightSDL.DiDevice,iniFilePath);
	WritePrivateProfileInt("RightJoyStick", "HiResDevice", RightSDL.HiRes, iniFilePath);

	return(0);
}

unsigned char WriteIniFile(void)
{
	return WriteNamedIniFile(IniFilePath);
}

void UpdateOnBoot(char *modname)
{
	AG_Strlcpy(CurrentConfig.ModulePath, modname, sizeof(CurrentConfig.ModulePath));
}

unsigned char ReadNamedIniFile(char *iniFilePath)
{
	unsigned char Index = 0;

	//fprintf(stderr, "Read Ini File : %s\n", IniFilePath);

	//Loads the config structure from the hard disk
	CurrentConfig.CPUMultiplyer = GetPrivateProfileInt("CPU","DoubleSpeedClock",2,iniFilePath);
	CurrentConfig.FrameSkip = GetPrivateProfileInt("CPU","FrameSkip",1,iniFilePath);
	CurrentConfig.SpeedThrottle = GetPrivateProfileInt("CPU","Throttle",1,iniFilePath);
	CurrentConfig.CpuType = GetPrivateProfileInt("CPU","CpuType",0,iniFilePath);
	CurrentConfig.MaxOverclock = GetPrivateProfileInt("CPU", "MaxOverClock",100, iniFilePath);
	CurrentConfig.MmuType = GetPrivateProfileInt("CPU","MmuType",0,iniFilePath);

	CurrentConfig.AudioRate = GetPrivateProfileInt("Audio","Rate",3,iniFilePath);
	GetPrivateProfileString("Audio","SndCard","",CurrentConfig.SoundCardName,CARDNAME_LEN-1,iniFilePath);

	CurrentConfig.MonitorType = GetPrivateProfileInt("Video","MonitorType",1,iniFilePath);
	CurrentConfig.ScanLines = GetPrivateProfileInt("Video","ScanLines",0,iniFilePath);
	CurrentConfig.Resize = GetPrivateProfileInt("Video","AllowResize",0,iniFilePath);	
	CurrentConfig.Aspect = GetPrivateProfileInt("Video","ForceAspect",0,iniFilePath);

	CurrentConfig.AutoStart = GetPrivateProfileInt("Misc","AutoStart",1,iniFilePath);
	CurrentConfig.CartAutoStart = GetPrivateProfileInt("Misc","CartAutoStart",1,iniFilePath);

	CurrentConfig.RamSize = GetPrivateProfileInt("Memory","RamSize",1,iniFilePath);
	GetPrivateProfileString("Memory","ExternalBasicImage","",CurrentConfig.ExternalBasicImage,MAX_PATH,iniFilePath);

	GetPrivateProfileString("Module","OnBoot","",CurrentConfig.ModulePath,MAX_PATH,iniFilePath);
	
	CurrentConfig.KeyMap = GetPrivateProfileInt("Misc","KeyMapIndex",0,iniFilePath);
	if (CurrentConfig.KeyMap>3)
		CurrentConfig.KeyMap = 0;	//Default to DECB Mapping
	vccKeyboardBuildRuntimeTableSDL((keyboardlayout_e)CurrentConfig.KeyMap);

	CheckPath(CurrentConfig.ModulePath);
	CheckPath(CurrentConfig.ExternalBasicImage);

	LeftSDL.UseMouse = GetPrivateProfileInt("LeftJoyStick","UseMouse",1,iniFilePath);
	LeftSDL.Left = GetPrivateProfileInt("LeftJoyStick","Left",AG_KEY_LEFT,iniFilePath);
	LeftSDL.Right = GetPrivateProfileInt("LeftJoyStick","Right",AG_KEY_RIGHT,iniFilePath);
	LeftSDL.Up = GetPrivateProfileInt("LeftJoyStick","Up",AG_KEY_UP,iniFilePath);
	LeftSDL.Down = GetPrivateProfileInt("LeftJoyStick","Down",AG_KEY_DOWN,iniFilePath);
	LeftSDL.Fire1 = GetPrivateProfileInt("LeftJoyStick","Fire1",AG_KEY_F1,iniFilePath);
	LeftSDL.Fire2 = GetPrivateProfileInt("LeftJoyStick","Fire2",AG_KEY_F2,iniFilePath);
	LeftSDL.DiDevice = GetPrivateProfileInt("LeftJoyStick","DiDevice",0,iniFilePath);
	LeftSDL.HiRes =  GetPrivateProfileInt("LeftJoyStick", "HiResDevice", 0, iniFilePath);
	RightSDL.UseMouse = GetPrivateProfileInt("RightJoyStick","UseMouse",1,iniFilePath);
	RightSDL.Left = GetPrivateProfileInt("RightJoyStick","Left",AG_KEY_LEFT,iniFilePath);
	RightSDL.Right = GetPrivateProfileInt("RightJoyStick","Right",AG_KEY_RIGHT,iniFilePath);
	RightSDL.Up = GetPrivateProfileInt("RightJoyStick","Up",AG_KEY_UP,iniFilePath);
	RightSDL.Down = GetPrivateProfileInt("RightJoyStick","Down",AG_KEY_DOWN,iniFilePath);
	RightSDL.Fire1 = GetPrivateProfileInt("RightJoyStick","Fire1",AG_KEY_F1,iniFilePath);
	RightSDL.Fire2 = GetPrivateProfileInt("RightJoyStick","Fire2",AG_KEY_F2,iniFilePath);
	RightSDL.DiDevice = GetPrivateProfileInt("RightJoyStick","DiDevice",0,iniFilePath);
	RightSDL.HiRes = GetPrivateProfileInt("RightJoyStick", "HiResDevice", 0, iniFilePath);

	CurrentConfig.SndOutDev = 0;
	for (Index = 0; Index < NumberOfSoundCards; Index++)
	{
		if (!strcmp(SoundCards[Index].CardName, CurrentConfig.SoundCardName))
		{
			CurrentConfig.SndOutDev = Index;
			break;
		}
	}

	if (Index == NumberOfSoundCards)
	{
		CurrentConfig.SndOutDev = 0;
		
		if (Index)
		{
			strcpy(CurrentConfig.SoundCardName, SoundCards[0].CardName);
		}
		else
		{
			strcpy(CurrentConfig.SoundCardName, "");
		}
	}

	TempConfig = CurrentConfig;
	//fprintf(stderr, "InsertModule : %s\n", CurrentConfig.ModulePath);
	//InsertModule (CurrentConfig.ModulePath);	// Should this be here?
	
	return(0);
}

unsigned char ReadIniFile(void)
{
	return ReadNamedIniFile(IniFilePath);
}

char * BasicRomName(void)
{
	return(CurrentConfig.ExternalBasicImage); 
}

void JoyStickConfigRecordState()
{
	TempLeft = LeftSDL;
	TempRight = RightSDL;
}

void ConfigOKApply(int close)
{
	EmuState2.ResetPending=4;

	if ((CurrentConfig.RamSize != TempConfig.RamSize) | (CurrentConfig.CpuType != TempConfig.CpuType))
	{
		EmuState2.ResetPending=2;
	}

	if ((CurrentConfig.SndOutDev != TempConfig.SndOutDev) | (CurrentConfig.AudioRate != TempConfig.AudioRate))
	{
		SoundInitSDL(SoundCards[TempConfig.SndOutDev].sdlID, TempConfig.AudioRate);
	}

	CurrentConfig=TempConfig;

	vccKeyboardBuildRuntimeTableSDL((keyboardlayout_e)CurrentConfig.KeyMap);

	RightSDL = TempRight;
	LeftSDL = TempLeft;

	SetStickNumbersSDL(LeftSDL.DiDevice,RightSDL.DiDevice);

	if (close) EmuState2.ConfigDialog = NULL;
}

void GetIniFilePath( char *Path)
{
	strcpy(Path,IniFilePath);
	return;
}

void UpdateConfig (void)
{
	SetResizeSDL(CurrentConfig.Resize);
	SetAspectSDL(CurrentConfig.Aspect);
	SetScanLinesSDL(CurrentConfig.ScanLines);
	SetFrameSkip(CurrentConfig.FrameSkip);
	SetAutoStart(CurrentConfig.AutoStart);
	SetSpeedThrottle(CurrentConfig.SpeedThrottle);
	SetCPUMultiplyer(CurrentConfig.CPUMultiplyer);
	SetRamSize(CurrentConfig.RamSize);
	SetCpuType(CurrentConfig.CpuType);
	SetMmuType(CurrentConfig.MmuType);
	SetMonitorTypeSDL(CurrentConfig.MonitorType);
	SetCartAutoStart(CurrentConfig.CartAutoStart);
	if (CurrentConfig.RebootNow)
		DoReboot();
	CurrentConfig.RebootNow = 0;
}

void CPUConfigSpeed(int multiplier)
{
	TempConfig.CPUMultiplyer = (unsigned short)multiplier;
}

void CPUConfigMemSize(int size)
{
	TempConfig.RamSize = (unsigned char)size;
}

void CPUConfigCPU(int cpu)
{
	TempConfig.CpuType = (unsigned char)cpu;
}

void MiscConfigAutoStart(int start)
{
	TempConfig.AutoStart = (unsigned char)start;
}

void MiscConfigCartAutoStart(int start)
{
	TempConfig.CartAutoStart = (unsigned char)start;
}

void TapeConfigButton(int button)
{
	#define REWIND 4

	switch (button)
	{
		case PLAY:
		case REC:
		case STOP:
		case EJECT:
			Tmode = (unsigned char)button;
			SetTapeMode(Tmode);
		break;

		case REWIND:
			TapeCounter = 0;
			SetTapeCounter(TapeCounter);
		break;
	}
}

void TapeConfigLoadTape()
{
	TapeCounter=0;
	SetTapeCounter(TapeCounter);
}

void AudioConfigGetAudioDevices(int *num, int * currentdev, SndCardList **DevList)
{
	*num = NumberOfSoundCards;
	*DevList = SoundCards;
	*currentdev = CurrentConfig.SndOutDev;
}

void AudioConfigGetJoyStickDevices(int *num, int *currentleft, int *currentright, char *StickNames[])
{
	*num = NumberofJoysticks;
	*StickNames = &StickName;
	*currentleft = LeftSDL.DiDevice;
	*currentright = RightSDL.DiDevice;
}

void AudioConfigSetDevice(int device)
{
	TempConfig.SndOutDev = (unsigned char)device;
}

void AudioConfigSetRate(int rate)
{
	TempConfig.AudioRate = (unsigned char)rate;
}

void DisplayConfigFrameSkip(int skip)
{
	TempConfig.FrameSkip = (unsigned char)skip;
}

void DisplayConfigResize(int onoff)
{
	TempConfig.Resize = (unsigned char)onoff;
}

void DisplayConfigAspect(int onoff)
{
	TempConfig.Aspect = (unsigned char)onoff;
}

void DisplayConfigScanLines(int onoff)
{
	TempConfig.ScanLines = (unsigned char)onoff;
}

void DisplayConfigThrottle(int onoff)
{
	TempConfig.SpeedThrottle = (unsigned char)onoff;
}

void DisplayConfigMonitorType(int type)
{
	TempConfig.MonitorType = (unsigned char)type;
}

void InputConfigKeyMap(int keymap)
{
	TempConfig.KeyMap = (unsigned char)keymap;
}

void JoyStickConfigLeftJoyStick(int type)
{
	TempLeft.UseMouse = (unsigned char)type;
}

void JoyStickConfigLeftJoyStickDevice(int dev)
{
	TempLeft.DiDevice = (unsigned char)dev;
}

void joyStickConfigLeftKeyLeft(int key)
{
	TempLeft.Left = TranslateDisp2Scan(key);
}

void joyStickConfigLeftKeyRight(int key)
{
	TempLeft.Right = TranslateDisp2Scan(key);
}

void joyStickConfigLeftKeyUp(int key)
{
	TempLeft.Up = TranslateDisp2Scan(key);
}

void joyStickConfigLeftKeyDown(int key)
{
	TempLeft.Down = TranslateDisp2Scan(key);
}

void joyStickConfigLeftKeyFire1(int key)
{
	TempLeft.Fire1 = TranslateDisp2Scan(key);
}

void joyStickConfigLeftKeyFire2(int key)
{
	TempLeft.Fire2 = TranslateDisp2Scan(key);
}

void JoyStickConfigRightJoyStick(int type)
{
	TempRight.UseMouse = (unsigned char)type;
}

void JoyStickConfigRightJoyStickDevice(int dev)
{
	TempRight.DiDevice = (unsigned char)dev;
}

void joyStickConfigRightKeyLeft(int key)
{
	TempRight.Left = TranslateDisp2Scan(key);
}

void joyStickConfigRightKeyRight(int key)
{
	TempRight.Right = TranslateDisp2Scan(key);
}

void joyStickConfigRightKeyUp(int key)
{
	TempRight.Up = TranslateDisp2Scan(key);
}

void joyStickConfigRightKeyDown(int key)
{
	TempRight.Down = TranslateDisp2Scan(key);
}

void joyStickConfigRightKeyFire1(int key)
{
	TempRight.Fire1 = TranslateDisp2Scan(key);
}

void joyStickConfigRightKeyFire2(int key)
{
	TempRight.Fire2 = TranslateDisp2Scan(key);
}

unsigned char XY2Disp (unsigned char Row,unsigned char Col)
{
	switch (Row)
	{
	case 0:
		return(0);
	case 1:
		return(1+Col);
	case 2:
		return(9+Col);
	case 4:
		return(17+Col);
	case 8:
		return (25+Col);
	case 16:
		return(33+Col);
	case 32:
		return (41+Col);
	case 64:
		return (49+Col);
	default:
		return (0);
	}
}

void Disp2XY(unsigned char *Col,unsigned char *Row,unsigned char Disp)
{
	unsigned char temp=0;
	if (Disp ==0)
	{
		Col=0;
		Row=0;
		return;
	}
	Disp-=1;
	temp= Disp & 56;
	temp = temp >>3;
	*Row = 1<<temp;
	*Col=Disp & 7;
return;
}

void RefreshJoystickStatus(void)
{
	NumberofJoysticks = EnumerateJoysticksSDL();

	if (RightSDL.DiDevice>(NumberofJoysticks-1))
	{
		RightSDL.DiDevice=0;
	}
	if (LeftSDL.DiDevice>(NumberofJoysticks-1))
	{
		LeftSDL.DiDevice=0;
	}

	SetStickNumbersSDL(LeftSDL.DiDevice, RightSDL.DiDevice);

	if (NumberofJoysticks == 0)	//Use Mouse input if no Joysticks present
	{
		if (LeftSDL.UseMouse==JOYSTICK_JOYSTICK)
		{
			LeftSDL.UseMouse=JOYSTICK_MOUSE;
		}
		if (RightSDL.UseMouse==JOYSTICK_JOYSTICK)
		{
			RightSDL.UseMouse=JOYSTICK_MOUSE;
		}
	}
	return;
}

void UpdateSoundBar (unsigned short Left,unsigned short Right)
{
	extern void UpdateSoundMeters(unsigned short, unsigned short);

	UpdateSoundMeters(Left, Right);
}

void UpdateTapeCounter(unsigned int Counter,unsigned char TapeMode)
{
	extern void UpdateTapeWidgets(int, char *, char *);

	GetTapeName(TapeFileName);
	PathStripPath(TapeFileName);  
	UpdateTapeWidgets(Counter, TapeMode, TapeFileName);
}

void BitBangerConfigOpen(char *file)
{
	AG_Strlcpy(SerialCaptureFile, file, sizeof(SerialCaptureFile));
}

void BitBangerConfigClose()
{
	ClosePrintFile();
	strcpy(SerialCaptureFile,"No Capture File");
	PrtMon=FALSE;
	SetMonState(PrtMon);
}

void BitBangerConfigLineFeed(int state)
{
	SetSerialParams((unsigned char)state);
}

void BitBangerConfigPrintMonitor(int state)
{
	SetMonState((unsigned char)state);
}
