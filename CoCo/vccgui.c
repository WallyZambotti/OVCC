/*
Copyright 2018 by Walter Zambotti
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

#include "defines.h"
#include <agar/core.h>
#include <agar/gui.h>
#include <agar/core/types.h>
#include "AGARInterface.h"
#include "audio.h"
#include "config.h"
#include "keyboard.h"
#include "joystickinputSDL.h"
#include "throttle.h"

#include "xdebug.h"

#ifndef Ulong
#define Ulong unsigned long
#endif

int redrawfx = 0;

void SetStatusBarText(const char *, SystemState2 *);

extern STRConfig CurrentConfig;
extern JoyStick	LeftSDL;
extern JoyStick RightSDL;

static char *keyNames[] = 
{
    "NILL", "ESC",                                                                                                                      // 0, 1
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",                                                                                   // 2 - 11
    "-", "=", "BackSp", "Tab",                                                                                                          // 12 - 15
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",   // 16 - 41
    "[", "]", "\\", ";", "'", ",", ".", "/", "Caps", "Shift", "Ctrl", "Alt", "Space", "Enter", "Insert", "Delete", "Home",              // 42 - 58
    "End", "PgUp", "PgDn", "Left", "Right", "Up", "Down", "F1", "F2"                                                                    // 59 - 67
};                                                                  

static AG_Window *win = NULL;

static AG_Combo *comLeftAudio, *comRightAudio, *comLeftJoystick, *comRightJoystick;
static AG_Combo *comLeftKeyLeft, *comLeftKeyRight, *comLeftKeyUp, *comLeftKeyDown, *comLeftKeyFire1, *comLeftKeyFire2;
static AG_Combo *comRightKeyLeft, *comRightKeyRight, *comRightKeyUp, *comRightKeyDown, *comRightKeyFire1, *comRightKeyFire2;
static AG_Combo *comDev, *comQual;

static AG_Menu *menu;
static AG_MenuItem *itemCartridge = NULL, *itemEjectCart = NULL;
static AG_Label *status = NULL;
//static AG_Fixed *fx = NULL;

static int soundDev = 0;
static int soundQuality = 3;
static int leftVolPrc = 25;
static int rightVolPrc = 75;
static int volMin = 0;
static int volMax = 100;
static double cpuOCval = 0.894;
static double cpuMin = 0.894;
static double cpuMax = 511.368;
static int MemSize = 1;
static int CPU = 0;
static int MMU = 0;
static int MonitorType = 0;
static int frameSkip = 1;
static int frameMin = 1;
static int frameMax = 6;
static int scanLines = 0;
static int forceAspect = 0;
static int allowResize = 0;
static int throttleSpeed = 1;
static int keyboardMap = 0;
static int leftJoystick = 2;
static int leftAnalogDevice = 0;
static int leftJoystickDevice = 0;
static int leftKeyLeft = 0;
static int leftKeyRight = 0;
static int leftKeyUp = 0;
static int leftKeyDown = 0;
static int leftKeyFire1 = 0;
static int leftKeyFire2 = 0;
static int leftEmulation = 0;
static int rightJoystick = 2;
static int rightAnalogDevice = 0;
static int rightJoystickDevice = 0;
static int rightKeyLeft = 0;
static int rightKeyRight = 0;
static int rightKeyUp = 0;
static int rightKeyDown = 0;
static int rightKeyFire1 = 0;
static int rightKeyFire2 = 0;
static int rightEmulation = 0;
static int showLeftJoystickValue = 0;
static int showRightJoystickValue = 0;
static int autoStartEmu = 1;
static int autoStartCart = 1;
static char tapefile[512] = "<No Tape File!>";
static int tapecounter = 0;
static char tapeMode[64] = "STOP";
static char bitbangerfile[512] = "<No Capture File!>";
static int AddLFtoCR = 1;
static int PrintMonitor = 0;

void _MessageBox(const char *msg)
{
    AG_TextMsg(AG_MSG_INFO, "%s", msg);
}

void Run(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

    state->EmulationRunning = TRUE;

    AG_TextMsg(AG_MSG_INFO, "Emulation State set to running!");
    DoClsAGAR(state);
}

void Halt(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

    state->EmulationRunning = FALSE;

    AG_TextMsg(AG_MSG_INFO, "Emulation State set to halt!");
}

void LoadIniFile(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);
	char *file = AG_STRING(2);
	AG_FileType *ft = AG_PTR(3);

    extern void  ConfigLoadIniFile(char *);

    ConfigLoadIniFile(file);
}

void LoadConf(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

    AG_Window *fdw = AG_WindowNew(0);
    AG_WindowSetCaption(fdw, "Select Ini File");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Confifuration File", "*.ini",	LoadIniFile, "%p", state);
    AG_WindowShow(fdw);
}

void SaveIniFile(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);
	char *file = AG_STRING(2);
	AG_FileType *ft = AG_PTR(3);

    extern unsigned char SaveCurrentConfigToFile(char *);
    extern unsigned char WriteIniFile(void);
    extern char IniFilePath[MAX_PATH];

    if (strcmp(file, IniFilePath) == 0)
    {
        WriteIniFile();
        AG_TextMsg(AG_MSG_INFO, "Config (ini) file saved!");
        return ;
    }

    if(SaveCurrentConfigToFile(file))
    {
        AG_TextMsg(AG_MSG_INFO, "Config (ini) file saved!");
    }
    else 
    {
        AG_TextMsg(AG_MSG_INFO, "Could not save Config (ini) file!");        
    }
}

void SaveConf(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

    AG_Window *fdw = AG_WindowNew(0);
    AG_WindowSetCaption(fdw, "Select Ini File");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Configuration File", "*.ini",	SaveIniFile, "%p", state);
    AG_WindowShow(fdw);
}

void HardReset(AG_Event *ev)
{
    extern void DoHardResetF9();

    DoHardResetF9();
}

void SoftReset(AG_Event *ev)
{
    extern void DoSoftReset();

    DoSoftReset();
}

void ExitF7(AG_Event *event)
{
    SystemState2 *SState = AG_PTR(1);
    AG_Window *win = SState->agwin;

    AG_CloseFocusedWindow();    

    //AGWINDETACH(win);
}

void SoftResetF5(AG_Event *event)
{
    extern void DoSoftReset();

    DoSoftReset();
}

void MonitorChangeF6(AG_Event *ev)
{
    extern void ToggleMonitorType();

    ToggleMonitorType();
}

void CPUIncDecF3F4(AG_Event *event)
{
    extern void ConfigOKApply(int);
    extern void CPUConfigSpeed(int);
    extern void JoyStickConfigRecordState();
    
    int incdec = AG_INT(1);

    cpuMax = (double)CurrentConfig.MaxOverclock * cpuMin;
    cpuOCval = (double)CurrentConfig.CPUMultiplyer * cpuMin;

    if (cpuOCval <= cpuMin || cpuOCval >= cpuMax)
    {
        return;
    }

    cpuOCval += cpuMin * (double)incdec;

    CPUConfigSpeed((int)(cpuOCval/cpuMin));
    JoyStickConfigRecordState();
    ConfigOKApply(0);
}

void ThrottleSpeedF8(AG_Event *ev)
{
    extern void ToggleThrottleSpeed();
    
    ToggleThrottleSpeed();
}

void  HardResetF9(AG_Event *event)
{
    extern void DoHardResetF9();

    DoHardResetF9();
}

void FullScreenStatusF10(AG_Event *ev)
{
    extern void ToggleScreenStatus();

    ToggleScreenStatus();
}

void FullScreenF11(AG_Event *ev)
{
    extern void ToggleFullScreen();

    ToggleFullScreen();
}

void Apply(AG_Event *event)
{
#define DO_CLOSE 1
#define STAY_OPEN 0

    int OpenClose = AG_INT(1);

    extern void ConfigOKApply(int);

    ConfigOKApply(OpenClose);

    if (OpenClose == DO_CLOSE)
    {
        AG_WindowHide(win);
    }
}

void Cancel(AG_Event *event)
{
    AG_WindowHide(win);
}

void DevComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);
    extern void AudioConfigSetDevice(int);
    AG_TlistItem *item = NULL;

    soundDev = ti->u;
    AudioConfigSetDevice(soundDev);
}

void QualityComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);
    extern void AudioConfigSetRate(int);

    soundQuality = ti->u;
    AudioConfigSetRate(soundQuality);
}

void UpdateSoundMeters(unsigned short left, unsigned short right)
{
    leftVolPrc = (int)left / 655; // Assuming input values 0-65535 then dividing by 655 will give 0-100 percent
    rightVolPrc = (int)right / 655;
}

void CPUspeedChange(AG_Event *event)
{
    extern void CPUConfigSpeed(int);

    CPUConfigSpeed((int)(cpuOCval/cpuMin));
}

void MemSizeChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void CPUConfigMemSize(int);

    CPUConfigMemSize(MemSize);
}

void CPUChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void CPUConfigCPU(int);

    CPUConfigCPU(CPU);
}

void MMUChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);

#ifdef __MINGW32__
    if (MMU > 0)
    {
        AG_TextMsg(AG_MSG_INFO, "Feature only available on Linux");
        MMU = 0;
        return;
    }
#endif
    
    extern void MMUConfigMMU(int);

    MMUConfigMMU(MMU);
}

void MonitorChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void DisplayConfigMonitorType(int);

    DisplayConfigMonitorType(MonitorType);
}

void FrameSkipChanged(AG_Event *event)
{
    extern void DisplayConfigFrameSkip(int);

    DisplayConfigFrameSkip(frameSkip);
}

void ScanLinesChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void DisplayConfigScanLines(int);

    DisplayConfigScanLines(scanLines);
}

void ForceAspectChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void DisplayConfigAspect(int);

    DisplayConfigAspect(forceAspect);
}

void AllowResizeChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void DisplayConfigResize(int);

    DisplayConfigResize(allowResize);
}

void ThrottleSpeedChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void DisplayConfigThrottle(int);

    DisplayConfigThrottle(throttleSpeed);
}

void KeyboardMapChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void InputConfigKeyMap(int);

    InputConfigKeyMap(keyboardMap);
}

void LeftJoystickChange(AG_Event *event)
{
    // int newSelection = AG_INT(1);
    extern void JoyStickConfigLeftJoyStick(int);

    JoyStickConfigLeftJoyStick(leftJoystick);

    switch (leftJoystick)
    {
        case JOYSTICK_AUDIO:
            AG_WidgetEnable(comLeftAudio);
            AG_WidgetDisable(comLeftJoystick);
            AG_WidgetDisable(comLeftKeyDown);
            AG_WidgetDisable(comLeftKeyUp);
            AG_WidgetDisable(comLeftKeyLeft);
            AG_WidgetDisable(comLeftKeyRight);
            AG_WidgetDisable(comLeftKeyFire1);
            AG_WidgetDisable(comLeftKeyFire2);
            break;
            
        case JOYSTICK_JOYSTICK:
            AG_WidgetEnable(comLeftJoystick);
            AG_WidgetDisable(comLeftAudio);
            AG_WidgetDisable(comLeftKeyDown);
            AG_WidgetDisable(comLeftKeyUp);
            AG_WidgetDisable(comLeftKeyLeft);
            AG_WidgetDisable(comLeftKeyRight);
            AG_WidgetDisable(comLeftKeyFire1);
            AG_WidgetDisable(comLeftKeyFire2);
            break;
            
        case JOYSTICK_MOUSE:
            AG_WidgetDisable(comLeftAudio);
            AG_WidgetDisable(comLeftJoystick);
            AG_WidgetDisable(comLeftKeyDown);
            AG_WidgetDisable(comLeftKeyUp);
            AG_WidgetDisable(comLeftKeyLeft);
            AG_WidgetDisable(comLeftKeyRight);
            AG_WidgetDisable(comLeftKeyFire1);
            AG_WidgetDisable(comLeftKeyFire2);
            break;
            
        case JOYSTICK_KEYBOARD:
            AG_WidgetDisable(comLeftAudio);
            AG_WidgetDisable(comLeftJoystick);
            AG_WidgetEnable(comLeftKeyDown);
            AG_WidgetEnable(comLeftKeyUp);
            AG_WidgetEnable(comLeftKeyLeft);
            AG_WidgetEnable(comLeftKeyRight);
            AG_WidgetEnable(comLeftKeyFire1);
            AG_WidgetEnable(comLeftKeyFire2);
            break;
    }
}

void LeftAnalogComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    AG_TextMsg(AG_MSG_INFO, "Feature no implemented!");

    leftAnalogDevice = ti->u;
}

void LeftJoystickComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_TLISTITEM_PTR(1);   

    extern void JoyStickConfigLeftJoyStickDevice(int);

    leftJoystickDevice = ti->u;

    JoyStickConfigLeftJoyStickDevice(leftJoystickDevice);
}

void LeftKeyLeftComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_TLISTITEM_PTR(1);   

    extern void joyStickConfigLeftKeyLeft(int);
    
    leftKeyLeft = ti->u;

    joyStickConfigLeftKeyLeft(leftKeyLeft);
}

void LeftKeyRightComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigLeftKeyRight(int);
    
    leftKeyRight = ti->u;

    joyStickConfigLeftKeyRight(leftKeyRight);
}

void LeftKeyUpComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigLeftKeyUp(int);
    
    leftKeyUp = ti->u;

    joyStickConfigLeftKeyUp(leftKeyUp);
}

void LeftKeyDownComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigLeftKeyDown(int);
    
    leftKeyDown = ti->u;

    joyStickConfigLeftKeyDown(leftKeyDown);
}

void LeftKeyFire1ComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigLeftKeyFire1(int);
    
    leftKeyFire1 = ti->u;

    joyStickConfigLeftKeyFire1(leftKeyFire1);
}

void LeftKeyFire2ComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigLeftKeyFire2(int);
    
    leftKeyFire2 = ti->u;

    joyStickConfigLeftKeyFire2(leftKeyFire2);
}

void LeftEmulationChange(AG_Event *event)
{
    int newSelection = AG_INT(1);

    extern void JoyStickConfigLeftEmulation(int);

    // AG_TextMsg(AG_MSG_INFO, "Feature not implemented!");

    JoyStickConfigLeftEmulation(leftEmulation);
}

void RightJoystickChange(AG_Event *event)
{
    // int newSelection = AG_INT(1);
    extern void JoyStickConfigRightJoyStick(int);

    JoyStickConfigRightJoyStick(rightJoystick);

    switch (rightJoystick)
    {
        case JOYSTICK_AUDIO:
            AG_WidgetEnable(comRightAudio);
            AG_WidgetDisable(comRightJoystick);
            AG_WidgetDisable(comRightKeyDown);
            AG_WidgetDisable(comRightKeyUp);
            AG_WidgetDisable(comRightKeyLeft);
            AG_WidgetDisable(comRightKeyRight);
            AG_WidgetDisable(comRightKeyFire1);
            AG_WidgetDisable(comRightKeyFire2);
            break;
            
        case JOYSTICK_JOYSTICK:
            AG_WidgetEnable(comRightJoystick);
            AG_WidgetDisable(comRightAudio);
            AG_WidgetDisable(comRightKeyDown);
            AG_WidgetDisable(comRightKeyUp);
            AG_WidgetDisable(comRightKeyLeft);
            AG_WidgetDisable(comRightKeyRight);
            AG_WidgetDisable(comRightKeyFire1);
            AG_WidgetDisable(comRightKeyFire2);
            break;
            
        case JOYSTICK_MOUSE:
            AG_WidgetDisable(comRightAudio);
            AG_WidgetDisable(comRightJoystick);
            AG_WidgetDisable(comRightKeyDown);
            AG_WidgetDisable(comRightKeyUp);
            AG_WidgetDisable(comRightKeyLeft);
            AG_WidgetDisable(comRightKeyRight);
            AG_WidgetDisable(comRightKeyFire1);
            AG_WidgetDisable(comRightKeyFire2);
            break;
            
        case JOYSTICK_KEYBOARD:
            AG_WidgetDisable(comRightAudio);
            AG_WidgetDisable(comRightJoystick);
            AG_WidgetEnable(comRightKeyDown);
            AG_WidgetEnable(comRightKeyUp);
            AG_WidgetEnable(comRightKeyLeft);
            AG_WidgetEnable(comRightKeyRight);
            AG_WidgetEnable(comRightKeyFire1);
            AG_WidgetEnable(comRightKeyFire2);
            break;
    }
}

void RightAnalogComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    AG_TextMsg(AG_MSG_INFO, "Feature no implemented!");

    rightAnalogDevice = ti->u;
}

void RightJoystickComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void JoyStickConfigRightJoyStickDevice(int);

    rightJoystickDevice = ti->u;

    JoyStickConfigRightJoyStickDevice(rightJoystickDevice);
}

void RightKeyLeftComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyLeft(int);
    
    rightKeyLeft = ti->u;

    joyStickConfigRightKeyLeft(rightKeyLeft);
}

void RightKeyRightComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyRight(int);
    
    rightKeyRight = ti->u;

    joyStickConfigRightKeyRight(rightKeyRight);
}

void RightKeyUpComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyUp(int);
    
    rightKeyUp = ti->u;

    joyStickConfigRightKeyUp(rightKeyUp);
}

void RightKeyDownComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyDown(int);
    
    rightKeyDown = ti->u;

    joyStickConfigRightKeyDown(rightKeyDown);
}

void RightKeyFire1ComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyFire1(int);
    
    rightKeyFire1 = ti->u;

    joyStickConfigRightKeyFire1(rightKeyFire1);
}

void RightKeyFire2ComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    extern void joyStickConfigRightKeyFire2(int);
    
    rightKeyFire2 = ti->u;

    joyStickConfigRightKeyFire2(rightKeyFire2);
}

void RightEmulationChange(AG_Event *event)
{
    int newSelection = AG_INT(1);

    extern void JoyStickConfigRightEmulation(int);

    //AG_TextMsg(AG_MSG_INFO, "Feature not implemented!");

    JoyStickConfigRightEmulation(rightEmulation);
}

void ShowLeftJoystickValueChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void SetShowLeftJoystickValue(int);

    SetShowLeftJoystickValue(showLeftJoystickValue);
}

void ShowRightJoystickValueChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void SetShowRightJoystickValue(int);

    SetShowRightJoystickValue(showRightJoystickValue);
}

void AutoStartEmuChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void MiscConfigAutoStart(int);

    MiscConfigAutoStart(autoStartEmu);
}

void AutoStartCartChange(AG_Event *event)
{
    //int newSelection = AG_INT(1);
    extern void MiscConfigCartAutoStart(int);

    MiscConfigCartAutoStart(autoStartCart);
}

void LoadCasette(AG_Event *event)
{
	char *file = AG_STRING(1);
	AG_FileType *ft = AG_PTR(2);

    extern int MountTape(char *);
    extern void TapeConfigLoadTape();

    AG_Strlcpy(tapefile, file, sizeof(tapefile));

    if (MountTape(file) == 0)
    {
        AG_TextMsg(AG_MSG_ERROR, "Can't open tape file %s", tapefile);        
    }

    TapeConfigLoadTape();
}

void BrowseTape(AG_Event *event)
{
    AG_Window *fdw = AG_WindowNew(0);
    AG_WindowSetCaption(fdw, "Select Tape");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Casette File", "*.cas,*.wav",	LoadCasette, NULL);
    AG_WindowShow(fdw);
}

void UpdateTapeWidgets(int counter, char mode, char *file)
{
    char modestr[2] = { mode, 0 };

    tapecounter = counter;
    AG_Strlcpy(tapeMode, modestr, sizeof(tapeMode));
    AG_Strlcpy(tapefile, file, sizeof(tapefile));
}

void SelectBitBangerFile(AG_Event *event)
{
	char *file = AG_STRING(1);
	AG_FileType *ft = AG_PTR(2);

    extern void BitBangerConfigOpen(char*);
    extern int OpenPrintFile(char *);

    AG_Strlcpy(bitbangerfile, file, sizeof(bitbangerfile));

    if (!(OpenPrintFile(file)))
    {
		AG_TextMsg(AG_MSG_ERROR, "Can't open BitBanger file %s", bitbangerfile);
    }

    BitBangerConfigOpen(bitbangerfile);
}

void OpenBitBanger()
{
    AG_Window *fdw = AG_WindowNew(0);
    AG_WindowSetCaption(fdw, "Select BitBanger");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo BitBanger File", "*.bit,*.txt", SelectBitBangerFile, NULL);
    AG_WindowShow(fdw);
}

void TapeFn(AG_Event *event)
{
    #define TAPE_STOP 0
    #define TAPE_PLAY 1
    #define TAPE_REC 2
    #define TAPE_EJECT 3
    #define TAPE_REWIND 4

    extern void TapeConfigButton(int);
    int tapefn = AG_INT(1);

    TapeConfigButton(tapefn);
}

void BitBangerFn(AG_Event *event)
{
#define BITB_OPEN 1
#define BITB_CLOSE 2

    extern void BitBangerConfigClose();

    int bitbfn = AG_INT(1);

    switch (bitbfn)
    {
        case BITB_OPEN:
            OpenBitBanger();
            break;

        case BITB_CLOSE:
            BitBangerConfigClose();
            PrintMonitor = 0;
            break;

        default:
            AG_TextMsg(AG_MSG_INFO, "Selected unknown BitBanger function");
            break;
    }
}

void AddLFtoCRChange(AG_Event *event)
{
    extern void BitBangerConfigLineFeed(int);

    BitBangerConfigLineFeed(AddLFtoCR);
}

void PrintMonitorChange(AG_Event *event)
{
    extern void BitBangerConfigPrintMonitor(int);

    BitBangerConfigPrintMonitor(PrintMonitor);
}

void SetStatusBarText(const char *text, SystemState2 *EmuState2)
{
    // char newtext[240];
    // sprintf(newtext, "%s[%d,%d](%d,%d)", text, EmuState2->fx->wid.w, EmuState2->fx->wid.h, EmuState2->agwin->wid.w, EmuState2->agwin->wid.h); 
    AG_LabelText(status, "%s", text);
}

void PopulateAudioDevices(AG_Event *event)
{
    AG_Combo *com = AG_COMBO_SELF(); 
    int numOfDevs = 0, dev = 0, currentdev = 0;
    SndCardList *cardList;

    extern void AudioConfigGetAudioDevices(int *, int*, SndCardList **);

    AudioConfigGetAudioDevices(&numOfDevs, &currentdev, &cardList);

    AG_ComboSizeHint(com, "Speakers (High Definition Audio Device", 4);

    for (dev = 0 ; dev < numOfDevs ; dev++)
    {
        AG_TlistItem *item = AG_TlistAddS(com->list, NULL, cardList[dev].CardName);
        item->u = dev;
    }

    soundDev = currentdev;

    AG_TlistItem *item = AG_TlistFindByIndex(com->list, soundDev+1);
    if (item != NULL) AG_ComboSelect(com, item);
}

void PopulateAudioQualities(AG_Event *event)
{
    AG_Combo *com = AG_COMBO_SELF(); 

    AG_ComboSizeHint(com, "192000", 4);

    AG_TlistItem *item = AG_TlistAddS(com->list, NULL, "Mute");
    item->u = 0;
    item = AG_TlistAddS(com->list, NULL, "11025");
    item->u = 1;
    item = AG_TlistAddS(com->list, NULL, "22050");
    item->u = 2;
    item = AG_TlistAddS(com->list, NULL, "44100");
    item->u = 3;

    item = AG_TlistFindByIndex(com->list, soundQuality+1);
    if (item != NULL) AG_ComboSelect(com, item);
}

void PopulateDummyAnalogDevices(AG_Event *event)
{
    AG_Combo *com = AG_COMBO_SELF(); 

    AG_ComboSizeHint(com, "Feature Not Implemented!", 4);
    AG_TlistAddS(com->list, NULL, "Feature Not Implemented!");

    AG_TlistItem *item = AG_TlistFindByIndex(com->list, 1);
    if (item != NULL) AG_ComboSelect(com, item);
}

void PopulateJoystickDevices(AG_Event *event)
{
    AG_Combo *com = AG_COMBO_SELF(); 

    int numOfSticks = 0, stick = 0, currentleft = 0, currentright;
    char **sticknames;

    extern void AudioConfigGetJoyStickDevices(int *, int*, int*, char **[]);

    AudioConfigGetJoyStickDevices(&numOfSticks, &currentleft, &currentright, &sticknames);

    AG_ComboSizeHint(com, "Digital Joystick 1", 4);
    
    for (stick = 0 ; stick < numOfSticks ; stick++)
    {
        AG_TlistItem *item = AG_TlistAddS(com->list, NULL, sticknames[stick]);
        item->u = stick;
    }

    leftJoystickDevice = currentleft;
    rightJoystickDevice = currentright;

    AG_TlistItem *item = AG_TlistFindByIndex(com->list, LeftSDL.DiDevice+1);
    if (item != NULL) AG_ComboSelect(com, item);
}

void PopulateKeysList(AG_Event *event)
{
    AG_Combo *com = AG_COMBO_SELF(); 
    int index = AG_INT(1);
    char *keyName;
    char listTxt[8];
    int nkeys = sizeof(keyNames) / sizeof(keyName);
    int key;

    AG_ComboSizeHint(com, "12345678", 8);

    for (key = 0; key < nkeys; key++)
    {
        AG_Strlcpy(listTxt, keyNames[key], sizeof(listTxt));
        AG_TlistItem *item = AG_TlistAddS(com->list, NULL, listTxt);
        item->u = key;
    }

    AG_TlistItem *item = AG_TlistFindByIndex(com->list, index);
    if (item != NULL) AG_ComboSelect(com, item);
}

void Configure(AG_Event *ev)
{
    char path[AG_PATHNAME_MAX];
    AG_Box *hb;
    AG_Textbox *tbox;
    AG_Notebook *nb;
    AG_NotebookTab *tab;

    if (win != NULL)
    {
        AG_WindowShow(win);
        return;
    }

    if ((win = AG_WindowNewNamedS(0, "OVCC_Config")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 682, 366);
    AG_WindowSetCaptionS(win, "OVCC Options");
    AG_WindowSetCloseAction(win, AG_WINDOW_HIDE);

    nb = AG_NotebookNew(win, AG_NOTEBOOK_EXPAND);
    tab = AG_NotebookAdd(nb, "Audio", AG_BOX_HORIZ);
    {
        AG_Box *box, *lbox, *rbox;
        AG_ProgressBar *leftVol, *rightVol;

        // Sound Output Device Combo

        box = AG_BoxNewHoriz(tab, AG_BOX_EXPAND | AG_BOX_FRAME);
        lbox = AG_BoxNewVert(box, AG_BOX_EXPAND);
        AG_LabelNew(lbox, 0, "Output Device");

        comDev = AG_ComboNew(lbox, AG_COMBO_HFILL, NULL);
        AG_SetEvent(comDev, "combo-expanded", PopulateAudioDevices, NULL);
        AG_SetEvent(comDev, "combo-selected", DevComboSelected, NULL);

        // Sound Quality Combo

        AG_LabelNew(lbox, 0, "Sound Quality");

        comQual = AG_ComboNew(lbox, AG_COMBO_HFILL, NULL);
        AG_SetEvent(comQual, "combo-expanded", PopulateAudioQualities, NULL);
        AG_SetEvent(comQual, "combo-selected", QualityComboSelected, NULL);

        rbox = AG_BoxNewHoriz(box, 0);

        // Left & Right Sound Bar Meters

        leftVol = AG_ProgressBarNewInt(rbox, AG_PROGRESS_BAR_VERT, AG_PROGRESS_BAR_VFILL | AG_PROGRESS_BAR_EXCL,
                                       &leftVolPrc, &volMin, &volMax);
        rightVol = AG_ProgressBarNewInt(rbox, AG_PROGRESS_BAR_VERT, AG_PROGRESS_BAR_VFILL | AG_PROGRESS_BAR_EXCL,
                                        &rightVolPrc, &volMin, &volMax);
    }

    tab = AG_NotebookAdd(nb, "CPU", AG_BOX_VERT);
    {
        AG_Box *box, *hbox, *vbox;
        AG_Slider *sl;
        AG_Label *lbl;

        // CPU Over Clock Slider

        AG_LabelNew(tab, 0, "Over-Clocking");
        box = AG_BoxNewHoriz(tab, AG_BOX_HFILL | AG_BOX_FRAME);

        cpuMax = (double)CurrentConfig.MaxOverclock * cpuMin;
        sl = AG_SliderNew(box, AG_SLIDER_HORIZ, AG_SLIDER_EXCL | AG_SLIDER_HFILL);
        AG_BindDouble(sl, "value", &cpuOCval);
        AG_BindDouble(sl, "min", &cpuMin);
        AG_BindDouble(sl, "max", &cpuMax);
        AG_SetDouble(sl, "inc", 0.894);

        AG_Numerical *sb = AG_NumericalNewDblR(box, 0, NULL, NULL, &cpuOCval, cpuMin, cpuMax);
        AG_SetDouble(sb, "inc", 0.894);

        hbox = AG_BoxNewHoriz(tab, AG_HBOX_EXPAND);

        AG_SetEvent(sl, "slider-changed", CPUspeedChange, NULL);
        AG_SetEvent(sb, "numerical-changed", CPUspeedChange, NULL);
        AG_SetEvent(sb, "numerical-return", CPUspeedChange, NULL);
        cpuOCval = (double)CurrentConfig.CPUMultiplyer * cpuMin;

        // Mem Size Radio Buttons

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "Memory Size");

        const char *radioItems[] = 
        {
            "128KB",
            "512KB",
            "2048KB",
            "8096KB",
            NULL
        };

        AG_Radio *radio = AG_RadioNewFn(vbox, AG_RADIO_VFILL, radioItems, MemSizeChange, NULL);
        AG_BindInt(radio, "value", &MemSize);
        MemSize = (int)CurrentConfig.RamSize;

        // CPU Type Radio Buttons

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "CPU");

        const char *radioItems2[] = 
        {
            "Motorola MC6809",
            "Hitachi HD6309",
            NULL
        };

        radio = AG_RadioNewFn(vbox, AG_RADIO_VFILL, radioItems2, CPUChange, NULL);
        AG_BindInt(radio, "value", &CPU);
        CPU = (int)CurrentConfig.CpuType;

        // MMU Type Radio Buttons

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "MMU");

        const char *radioItems3[] = 
        {
            "Software Simulation",
            "Hardware Emulation",
            NULL
        };

        radio = AG_RadioNewFn(vbox, AG_RADIO_VFILL, radioItems3, MMUChange, NULL);
        AG_BindInt(radio, "value", &MMU);
        MMU = (int)CurrentConfig.MmuType;
    }

    tab = AG_NotebookAdd(nb, "Display", AG_BOX_VERT);
    {
        AG_Box *box, *hbox, *vbox;
        AG_Numerical *sb;
        AG_Checkbox *xb;
        AG_Label *lbl;

        hbox = AG_BoxNewHoriz(tab, AG_HBOX_EXPAND);

        // [F6] Monitor Type Radio Buttons

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "Monitor Type");

        const char *radioItems[] = {
            "Composite",
            "RGB",
            NULL
        };

        AG_Radio *radio = AG_RadioNewFn(vbox, AG_RADIO_VFILL, radioItems, MonitorChange, NULL);
        AG_BindInt(radio, "value", &MonitorType);
        MonitorType = (int)CurrentConfig.MonitorType;

        // Frame Skip Numeric Spinner

        box = AG_BoxNewHoriz(hbox, AG_BOX_EXPAND | AG_BOX_FRAME);

        sb = AG_NumericalNewIntR(box, 0, NULL, "Frame Skip", &frameSkip, 1, 6);

        AG_SetEvent(sb, "numerical-changed", FrameSkipChanged, NULL);
        AG_SetEvent(sb, "numerical-return", FrameSkipChanged, NULL);
        frameSkip = (int)CurrentConfig.FrameSkip;

        // Various Option Checkboxes

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);

        xb = AG_CheckboxNewFn(vbox, 0, "Scan Lines", ScanLinesChange, NULL);
        AG_BindInt(xb, "state", &scanLines);
        scanLines = (int)CurrentConfig.ScanLines;

        xb = AG_CheckboxNewFn(vbox, 0, "Force Aspect", ForceAspectChange, NULL);
        AG_BindInt(xb, "state", &forceAspect);
        forceAspect = (int)CurrentConfig.Aspect;

        xb = AG_CheckboxNewFn(vbox, 0, "Allow Resize", AllowResizeChange, NULL);
        AG_BindInt(xb, "state", &allowResize);
        allowResize = (int)CurrentConfig.Resize;

        xb = AG_CheckboxNewFn(vbox, 0, "ThrottleSpeed", ThrottleSpeedChange, NULL);
        AG_BindInt(xb, "state", &throttleSpeed);
        throttleSpeed = (int)CurrentConfig.SpeedThrottle;
    }

    tab = AG_NotebookAdd(nb, "Keyboard", AG_BOX_VERT);
    {
        AG_Box *box;
        AG_Radio *radio;

        // Keyboard Mapping Radio Buttons

        box = AG_BoxNewVert(tab, AG_VBOX_EXPAND | AG_BOX_FRAME);
        AG_LabelNew(box, 0, "Keyboard Mapping");

        const char *radioItems[] = 
        {
            "CoCo (DECB)",
            "Natural (OS-9)",
            "Compact (OS-9)",
            NULL
        };

        radio = AG_RadioNewFn(box, AG_RADIO_VFILL, radioItems, KeyboardMapChange, NULL);
        AG_BindInt(radio, "value", &keyboardMap);
        keyboardMap = (int)CurrentConfig.KeyMap;
    }

    tab = AG_NotebookAdd(nb, "Joysticks", AG_BOX_HORIZ);
    {
        AG_Box *hbox, *vbox, *vbox1, *vbox2, *vboxr;
        AG_Radio *radio;
        AG_TlistItem *item;
        AG_Combo *com;

        // Left Joystick Config

        vbox = AG_BoxNewVert(tab, AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "Left Joystick Input                                              ");
        hbox = AG_BoxNewHoriz(vbox, AG_HBOX_HFILL);

        vbox1 = AG_BoxNewVert(hbox, 0);
        vbox2 = AG_BoxNewVert(hbox, AG_VBOX_HFILL);

        // Left Joystick device Radio

        const char *radioItems[] = 
        {
            "Audio",
            "Joystick",
            "Mouse",
            "Keyboard",
            NULL
        };

        vboxr = AG_BoxNewVert(vbox1, 0);
        radio = AG_RadioNewFn(vboxr, AG_RADIO_VFILL, radioItems, LeftJoystickChange, NULL);
        AG_BindInt(radio, "value", &leftJoystick);
        leftJoystick = (int)LeftSDL.UseMouse;

        // The Emulation logic doesn't appear to have been implement so has been disabled

        // Left Emulation Radio

        AG_SeparatorNew(vbox1, AG_SEPARATOR_HORIZ);

        const char *radioItems2[] = 
        {
            "Standard",
            "Tandy Hi-res",
            "CC-MAX",
            NULL
        };

        vboxr = AG_BoxNewVert(vbox1, 0);
        radio = AG_RadioNewFn(vboxr, AG_RADIO_VFILL, radioItems2, LeftEmulationChange, NULL);
        AG_BindInt(radio, "value", &leftEmulation);
        leftEmulation = (int)LeftSDL.HiRes;
        //AG_WidgetDisable(radio);

        // Left Joystick Audio (Analog) Combo & again doesn't appear to have been implemented

        com = comLeftAudio = AG_ComboNew(vbox2, AG_COMBO_HFILL, NULL);
        AG_ComboSizeHint(com, "Analog Joystick 1", 4);
        AG_SetEvent(com, "combo-expanded", PopulateDummyAnalogDevices, NULL);
        AG_SetEvent(com, "combo-selected", LeftAnalogComboSelected, NULL);
        if (leftJoystick == JOYSTICK_AUDIO) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Joystick Digital Combo

        com = comLeftJoystick = AG_ComboNew(vbox2, AG_COMBO_HFILL, NULL);
        AG_SetEvent(com, "combo-expanded", PopulateJoystickDevices, NULL);
        AG_SetEvent(com, "combo-selected", LeftJoystickComboSelected, NULL);
        if (leftJoystick == JOYSTICK_JOYSTICK) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        AG_SeparatorNew(vbox2, AG_SEPARATOR_HORIZ);

        // Left Key Left Combo

        extern unsigned short TranslateScan2Disp(int);

        com = comLeftKeyLeft = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Left  ");
        int index = TranslateScan2Disp(LeftSDL.Left)+1;
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", index);
        AG_SetEvent(com, "combo-selected", LeftKeyLeftComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Key Right Combo

        com = comLeftKeyRight = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Right");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(LeftSDL.Right)+1);
        AG_SetEvent(com, "combo-selected", LeftKeyRightComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Key Up Combo

        com = comLeftKeyUp = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Up    ");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(LeftSDL.Up)+1);
        AG_SetEvent(com, "combo-selected", LeftKeyUpComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Key Down Combo

        com = comLeftKeyDown = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Down");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(LeftSDL.Down)+1);
        AG_SetEvent(com, "combo-selected", LeftKeyDownComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Key Fire 1 Combo

        com = comLeftKeyFire1 = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Fire 1");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(LeftSDL.Fire1)+1);
        AG_SetEvent(com, "combo-selected", LeftKeyFire1ComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Left Key Fire 2 Combo

        com = comLeftKeyFire2 = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Fire 2");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(LeftSDL.Fire2)+1);
        AG_SetEvent(com, "combo-selected", LeftKeyFire2ComboSelected, NULL);
        if (leftJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Option for display left joystick values in status bar

        AG_Checkbox *xb = AG_CheckboxNewFn(vbox, 0, "Monitor left joystick", ShowLeftJoystickValueChange, NULL);
        AG_BindInt(xb, "state", &showLeftJoystickValue);        

        // Right Joystick Config

        vbox = AG_BoxNewVert(tab, AG_BOX_FRAME);
        AG_LabelNew(vbox, 0, "Right Joystick Input                                             ");
        hbox = AG_BoxNewHoriz(vbox, AG_HBOX_HFILL);

        vbox1 = AG_BoxNewVert(hbox, 0);
        vbox2 = AG_BoxNewVert(hbox, AG_VBOX_HFILL);

        // Right Joystick device Radio

        vboxr = AG_BoxNewVert(vbox1, 0);
        radio = AG_RadioNewFn(vboxr, AG_RADIO_VFILL, radioItems, RightJoystickChange, NULL);
        AG_BindInt(radio, "value", &rightJoystick);
        rightJoystick = (int)RightSDL.UseMouse;

        // Right Emulation Radio

        AG_SeparatorNew(vbox1, AG_SEPARATOR_HORIZ);

        vboxr = AG_BoxNewVert(vbox1, 0);
        radio = AG_RadioNewFn(vboxr, AG_RADIO_VFILL, radioItems2, RightEmulationChange, NULL);
        AG_BindInt(radio, "value", &rightEmulation);
        //AG_WidgetDisable(radio);

        // Right Joystick Audio (Analog) Combo

        com = comRightAudio = AG_ComboNew(vbox2, AG_COMBO_HFILL, NULL);
        AG_SetEvent(com, "combo-expanded", PopulateDummyAnalogDevices, NULL);
        AG_SetEvent(com, "combo-selected", RightAnalogComboSelected, NULL);
        if (rightJoystick == JOYSTICK_AUDIO) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Joystick Digital Combo

        com = comRightJoystick = AG_ComboNew(vbox2, AG_COMBO_HFILL, NULL);
        AG_SetEvent(com, "combo-expanded", PopulateJoystickDevices, "%i", RightSDL.DiDevice+1);
        AG_SetEvent(com, "combo-selected", RightJoystickComboSelected, NULL);
        if (rightJoystick == JOYSTICK_JOYSTICK) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        AG_SeparatorNew(vbox2, AG_SEPARATOR_HORIZ);

        // Right Key Left Combo

        com = comRightKeyLeft = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Left  ");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Left)+1);
        AG_SetEvent(com, "combo-selected", RightKeyLeftComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Key Right Combo

        com = comRightKeyRight = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Right");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Right)+1);
        AG_SetEvent(com, "combo-selected", RightKeyRightComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Key Up Combo

        com = comRightKeyUp = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Up    ");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Up)+1);
        AG_SetEvent(com, "combo-selected", RightKeyUpComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Key Down Combo

        com = comRightKeyDown = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Down");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Down)+1);
        AG_SetEvent(com, "combo-selected", RightKeyDownComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Key Fire 1 Combo

        com = comRightKeyFire1 = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Fire 1");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Fire1)+1);
        AG_SetEvent(com, "combo-selected", RightKeyFire1ComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Right Key Fire 2 Combo

        com = comRightKeyFire2 = AG_ComboNew(vbox2, AG_COMBO_HFILL, "Fire 2");
        AG_SetEvent(com, "combo-expanded", PopulateKeysList, "%i", TranslateScan2Disp(RightSDL.Fire2)+1);
        AG_SetEvent(com, "combo-selected", RightKeyFire2ComboSelected, NULL);
        if (rightJoystick == JOYSTICK_KEYBOARD) AG_WidgetEnable(com); else AG_WidgetDisable(com);

        // Option for display right joystick values in status bar

        xb = AG_CheckboxNewFn(vbox, 0, "Monitor right joystick", ShowRightJoystickValueChange, NULL);
        AG_BindInt(xb, "state", &showRightJoystickValue);        

        extern void JoyStickConfigRecordState();
        JoyStickConfigRecordState();
    }

    tab = AG_NotebookAdd(nb, "Misc", AG_BOX_VERT);
    {
        AG_Box *vbox;
        AG_Checkbox *xb;

        // Misc Checkbox Buttons

        vbox = AG_BoxNewVert(tab, AG_VBOX_VFILL | AG_BOX_FRAME);

        AG_LabelNew(vbox, 0, "Misc");

        xb = AG_CheckboxNewFn(vbox, 0, "AutoStart Emulation", AutoStartEmuChange, NULL);
        AG_BindInt(xb, "state", &autoStartEmu);

        xb = AG_CheckboxNewFn(vbox, 0, "AutoStart Cart", AutoStartCartChange, NULL);
        AG_BindInt(xb, "state", &autoStartCart);
    }

    tab = AG_NotebookAdd(nb, "Tape", AG_BOX_VERT);
    {
        AG_Box *hbox, *vbox;

        // Tape File Browser

        hbox = AG_BoxNewHoriz(tab, AG_VBOX_HFILL | AG_BOX_FRAME);

        AG_LabelNewPolled(hbox, AG_LABEL_HFILL | AG_LABEL_FRAME, "%s", tapefile);
        AG_ButtonNewFn(hbox, 0, "Browse", BrowseTape, NULL);

        // Tape Recorder Buttons

        hbox = AG_BoxNewHoriz(tab, AG_VBOX_HFILL | AG_BOX_FRAME);

        AG_ButtonNewFn(hbox, 0, "Rec", TapeFn, "%i,", TAPE_REC);
        AG_ButtonNewFn(hbox, 0, "Play", TapeFn, "%i,", TAPE_PLAY);
        AG_ButtonNewFn(hbox, 0, "Stop", TapeFn, "%i,", TAPE_STOP);
        AG_ButtonNewFn(hbox, 0, "Eject", TapeFn, "%i,", TAPE_EJECT);
        AG_ButtonNewFn(hbox, 0, "Rewind", TapeFn, "%i,", TAPE_REWIND);
        
        // Tape Information

        hbox = AG_BoxNewHoriz(tab, AG_VBOX_EXPAND | AG_BOX_FRAME);
        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);

        AG_Label *lbl = AG_LabelNewPolled(vbox, AG_LABEL_FRAME, "%s", tapeMode);
        AG_LabelSizeHint(lbl, 1, "REWIND_");
        AG_LabelNew(vbox, 0, "   Mode   ");

        vbox = AG_BoxNewVert(hbox, AG_VBOX_VFILL | AG_BOX_FRAME);

        lbl = AG_LabelNewPolled(vbox, AG_LABEL_FRAME, "%i", &tapecounter);
        AG_LabelSizeHint(lbl, 1, "0000000000");
        AG_LabelNew(vbox, 0, "   Counter   ");
    }

    tab = AG_NotebookAdd(nb, "Bitbanger", AG_BOX_VERT);
    {
        AG_Box *hbox, *vbox;
        AG_Checkbox *xb;

        // Capture File Browser

        vbox = AG_BoxNewVert(tab, AG_VBOX_EXPAND | AG_BOX_FRAME);
    
        AG_Label *lbl = AG_LabelNewPolled(vbox, AG_LABEL_HFILL | AG_LABEL_FRAME, "%s", bitbangerfile);

        hbox = AG_BoxNewHoriz(vbox, AG_VBOX_HFILL | AG_BOX_FRAME);

        AG_ButtonNewFn(hbox, 0, "Open", BitBangerFn, "%i,", BITB_OPEN);
        AG_ButtonNewFn(hbox, 0, "Close", BitBangerFn, "%i,", BITB_CLOSE);

        // Bitbanger options
        
        xb = AG_CheckboxNewFn(vbox, 0, "Add LF to CR", AddLFtoCRChange, NULL);
        AG_BindInt(xb, "state", &AddLFtoCR);

        xb = AG_CheckboxNewFn(vbox, 0, "Print Monitor Window", PrintMonitorChange, NULL);
        AG_BindInt(xb, "state", &PrintMonitor);
    }

    hb = AG_BoxNewHoriz(win, AG_BOX_HOMOGENOUS | AG_BOX_HFILL);
    {
        AG_ButtonNewFn(hb, 0, "OK", Apply, "%i", DO_CLOSE);
        AG_ButtonNewFn(hb, 0, "Apply", Apply, "%i", STAY_OPEN);
        AG_ButtonNewFn(hb, 0, "Cancel", Cancel, NULL);
        //AG_ButtonNewFn(hb, 0, "Cancel", AGWINDETACH(win));
    }

    AG_WindowShow(win);

    return;
}

static int inLoadCart = 0;
static char modulefile[512];

AG_MenuItem *GetMenuAnchor()
{
    return itemCartridge;
}

void *CartLoad(void *p)
{
    extern int InsertModule(char *);
    SystemState2 *state = p;

    InsertModule(modulefile);

	state->EmulationRunning = TRUE;
	inLoadCart = 0;

    return state;
}

void LoadPack(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

	AG_Thread threadID;

	char *file = AG_STRING(2);
	AG_FileType *ft = AG_PTR(3);

    if (AG_FileExists(file))
    {
        AG_Strlcpy(modulefile, file, sizeof(modulefile));

	    AG_ThreadTryCreate(&threadID, CartLoad, state);

        if (threadID == (AG_Thread)NULL)
        {
            fprintf(stderr, "Can't Start Cart Load Thread!\n");
            return;
        }
    }
}

static void LoadCart(AG_Event *event)
{
    SystemState2 *state = AG_PTR(1);

    if (inLoadCart) return;
    
    AG_Window *fdw = AG_WindowNew(0);
    AG_WindowSetCaption(fdw, "Load Pak/Cartridge");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Pak or Cartridge", "*.rom,*.ccc,*.so,*.dll",	LoadPack, "%p", state);
    AG_WindowShow(fdw);
}

void RefreshCartridgeMenu(void)
{
    AG_WidgetUpdate(itemCartridge);
}

void UpdateCartridgeEject(char *cartname)
{
    if (cartname != NULL) 
    {
        AG_MenuSetLabel(itemEjectCart, "Eject Cart : %s", cartname);
    }

    //RefreshCartridgeMenu();
}

static void EjectCart(AG_Event *ev)
{
    extern void UnloadPack(void);

    UnloadPack();
}

void About(AG_Event *ev)
{
    static    AG_Window *AboutWin = NULL;

    if (AboutWin != NULL)
    {
        AG_WindowShow(AboutWin);
        return;
    }

    AboutWin = AG_WindowNew(0);
    AG_WindowSetCaption(AboutWin, "OVCC About");
    AG_WindowSetGeometryAligned(AboutWin, AG_WINDOW_ALIGNMENT_NONE, 500, 250);
    AG_WindowSetCloseAction(AboutWin, AG_WINDOW_HIDE);

    AG_MPane *mpane = AG_MPaneNew(AboutWin, AG_MPANE1T2B, AG_MPANE_EXPAND|AG_MPANE_FORCE_DIV);

    AG_Label *lbl1 = AG_LabelNewPolled(mpane->panes[0], AG_LABEL_FRAME | AG_LABEL_EXPAND, "%s", 
        "OVCC 1.6.1\n"
        "Walter Zambotti\n"
        "using AGAR 1.7.1 GUI\n"
        "Forked from VCC 2.01B (1.43)\n"
        "Copy Righted Joseph Forgione (GNU General Public License)\n"
        "\n"
        "Keyboard Shortcuts"
    );

    AG_Label *lbl2 = AG_LabelNewPolled(mpane->panes[1], AG_LABEL_FRAME | AG_LABEL_EXPAND, "%s", 
        "F3 - Inc CPU Frequency\n"
        "F7 - Exit OVCC\n"
        "F5 - Soft Reset\n"
        "F6 - Monitor Type\n"
        "F8 - Throttle"
    );

    AG_Label *lbl3 = AG_LabelNewPolled(mpane->panes[2], AG_LABEL_FRAME | AG_LABEL_EXPAND, "%s", 
        "F4 - Dec CPU Frequency\n"
        "F12 - About\n"
        "F9 - Hard Reset\n"
        "F10 - Fullscreen Status\n"
        "F11 - Fullscreen"
    );

    AG_ActionFn(AboutWin, "F12", About, NULL);
    AG_ActionOnKeyUp(AboutWin, AG_KEY_F12, AG_KEYMOD_ANY, "F12");

    AG_WindowShow(AboutWin);
}

void MouseMotion(AG_Event *event)
{
    AG_Fixed *fx = AG_SELF();
    int mx = AG_INT(1);
    int my = AG_INT(2);
    int mb = AG_INT(5);
    int x, y;

    // Ignore out of bound mouse coords

    if (mx < 0 || mx > fx->wid.w || my < 0 || my > fx->wid.h)
    {
        return;
    }

    extern void DoMouseMotion(int, int);

    x = (int)((float)mx/(float)fx->wid.w*640.0);
    y = (int)((float)my/(float)fx->wid.h*480.0);

    DoMouseMotion(x, y);

    //fprintf(stderr, "Mouse Motion x : %i, y : %i, buttons : %i\n", x, y, mb);
}

void KeyDownUp(AG_Event *event)
{
	AG_Widget *w = AG_SELF();
    unsigned short updown = (Uint16)AG_INT(1);
	unsigned short kb = AG_INT(2);
    int mod = AG_INT(3);
    unsigned short uc = AG_ULONG(4);

    extern void DoKeyBoardEvent(unsigned short, unsigned short, unsigned short);

#ifdef DARWIN
    // **** Added for latest MacOS ****
    static int capslocked;

    if (kb == AG_KEY_CAPSLOCK)
    {
        if (updown)
        {
            capslocked = 1;
            XTRACE("CAPS locked\n");
        }
        else
        {
            capslocked = 0;
            XTRACE("CAPS unlocked\n");
        }
        updown = kEventKeyDown;
        DoKeyBoardEvent(uc, kb, updown);
        XTRACE("key %x - mod %x - unicode %lx - updown %x\n", kb, mod, uc, updown);
        updown = kEventKeyUp;
        goto event;
    }

    // make the shift-alpha keys work
    switch (kb) {
    case AG_KEY_A - 0x20 ... AG_KEY_Z - 0x20:
        kb += 0x20;
        if (capslocked)
        {
            // fake a shift key
            DoKeyBoardEvent(uc, AG_KEY_LSHIFT, updown);
            XTRACE("faked a SHIFT key\n");
        }
        break;
    default:
        break;
    }

    // make the ctrl-alpha keys work
    if (mod & (AG_KEYMOD_LCTRL | AG_KEYMOD_RCTRL))
    {
        switch (kb)
        {
        case AG_KEY_ASCII_START ... AG_KEY_ESCAPE:
            kb += 0x60;
            break;
        default:
            break;
        }
    }

event:
    // **** End of additions ****
#endif

    DoKeyBoardEvent(uc, kb, updown);
	//fprintf(stderr, "key %d - scancode %d - mod %d - unicode %ld - updown %i,\n", kb&0xf, sc&0xff, mod, uc, updown);
    XTRACE("key %x - mod %x - unicode %lx - updown %x\n", kb, mod, uc, updown);
}

void ButtonDownUp(AG_Event *event)
{
	AG_Widget *w = AG_SELF();
    int state = AG_INT(1) ? 0 : 1; // AGAR Button state is Opposite OVCC requirements
	int b = AG_INT(2);

    extern void DoButton(int button, int state);

    DoButton(b, state);

	//fprintf(stderr, "%s: Button %d, updown %i,\n", AGOBJECT(w)->name, b, state);
    XTRACE("%s: Button %d, updown %i,\n", AGOBJECT(w)->name, b, state);
}

void LockTexture(AG_Event *event)
{
    SystemState2 *SState = AG_PTR(1);
}

void WindowDetached(AG_Event *event)
{
    extern void UnloadDll(short int);

    SystemState2 *SState = AG_PTR(1);

    AG_ThreadCancel(SState->emuThread);
#ifdef ISOCPU
    AG_ThreadCancel(SState->cpuThread);
#endif
	SState->Pixels = NULL;
    SState->EmulationRunning = 0;
    
	WriteIniFile(); //Save Any changes to ini File
	UnloadDll(0);
	SoundDeInitSDL();

    //printf("WindowDetached\n");
}

void DecorateWindow(SystemState2 *EmuState2)
{
    AG_Window *win = EmuState2->agwin;

	// Menus

    menu = AG_MenuNew(win, AG_MENU_HFILL);
    AG_MenuItem *itemFile = AG_MenuNode(menu->root, "File", NULL);
    {
        AG_MenuAction(itemFile, "Run", NULL, Run, "%p", EmuState2);
        AG_MenuAction(itemFile, "Halt", NULL, Halt, "%p", EmuState2);
        AG_MenuAction(itemFile, "Save Config", NULL, SaveConf, "%p", EmuState2);
        AG_MenuAction(itemFile, "Load Config", NULL, LoadConf, "%p", EmuState2);
        AG_MenuSeparator(itemFile);
        AG_MenuAction(itemFile, "[F9] Hard Reset", NULL, HardReset, NULL);
        AG_MenuAction(itemFile, "[F5] Soft Reset", NULL, SoftReset, NULL);
        AG_MenuSeparator(itemFile);
        AG_MenuAction(itemFile, "Exit", NULL, AGWINDETACH(win));
    }

    AG_MenuItem *itemConf = AG_MenuNode(menu->root, "Configuration", NULL);
    {
        AG_MenuAction(itemConf, "Config", NULL, Configure, NULL);
    }

    itemCartridge = AG_MenuNode(menu->root, "Cartridge", NULL);
    {
        AG_MenuItem *itemCart = AG_MenuNode(itemCartridge, "Cartridge", NULL);
        {
            AG_MenuAction(itemCart, "Load Cart", NULL, LoadCart, "%p", EmuState2);
            itemEjectCart = AG_MenuAction(itemCart, 
                "Eject Cart                                                                                       ", 
                NULL, EjectCart, NULL);
        }
    }

    AG_MenuItem *itemHelp = AG_MenuNode(menu->root, "Help", NULL);
    {
        AG_MenuAction(itemHelp, "About OVCC", NULL, About, NULL);
    }

    // The primary CoCo drawing surface is the surface of a rescalable Pixmap widget.

    AG_Surface *flip = AG_SurfaceRGB(640, 480, 32, AG_SURFACE_PACKED /*| AG_SURFACE_MAPPED */, 0xFF, 0xFF00, 0xFF0000);

    AG_Pixmap *fx = AG_PixmapFromSurface(win, AG_PIXMAP_EXPAND | AG_PIXMAP_RESCALE, flip);
    AG_PixmapSetSurface(fx, 0); // set surface to 1 but pixels to surface 0
    AG_BindInt(fx, "redrawfx", &redrawfx);
    AG_RedrawOnChange(fx, 1000/60, "redrawfx");

	EmuState2->fx = fx;
	EmuState2->Pixels = fx->wid.surfaces[0]->pixels;
	EmuState2->PTRsurface32=(unsigned int *)EmuState2->Pixels;
    EmuState2->SurfacePitch = 640;

    // The Status Bar

    AG_Statusbar *statusBar = AG_StatusbarNew(win, AG_BOX_FRAME | AG_STATUSBAR_HFILL);
    status = AG_StatusbarAddLabel(statusBar, "Ready");
}

#define MAX_CART_MENU 24

void PadDummyCartMenus(void)
{
    int dummyTot = 0;

    if ((CurrentConfig.dummyMenuPadMax - itemCartridge->nSubItems) < 1) return;

    AG_MenuItem **DummyItems = malloc(sizeof(void*)*(CurrentConfig.dummyMenuPadMax - itemCartridge->nSubItems));

    for (int dummycnt = itemCartridge->nSubItems ; dummycnt < CurrentConfig.dummyMenuPadMax ; dummycnt++)
    {
        DummyItems[dummyTot] = AG_MenuNode(itemCartridge, "", NULL);
        dummyTot++;
    }

    AG_MenuExpand(menu, itemCartridge, 0, 0);
    AG_MenuCollapse(itemCartridge);

    for (int dummycnt = 0 ; dummycnt < dummyTot ; dummycnt++)
    {
        AG_MenuDel(DummyItems[dummycnt]);
    }

    free(DummyItems);
}

void PrepareEventCallBacks(SystemState2 *EmuState2)
{
    // Events

    if (EmuState2->fx == NULL) return;

    // Fixed Widget

    AGWIDGET(EmuState2->fx)->flags |= 
        AG_WIDGET_FOCUSABLE |
        AG_WIDGET_UNFOCUSED_MOTION |
        AG_WIDGET_UNFOCUSED_KEYDOWN |
        AG_WIDGET_UNFOCUSED_KEYUP |
        AG_WIDGET_UNFOCUSED_BUTTONDOWN |
        AG_WIDGET_UNFOCUSED_BUTTONUP ;
    AG_SetEvent(EmuState2->fx, "mouse-motion", MouseMotion, NULL);
	AG_SetEvent(EmuState2->fx, "key-down", KeyDownUp, "%i", AG_KEY_PRESSED);
	AG_SetEvent(EmuState2->fx, "key-up", KeyDownUp, "%i", AG_KEY_RELEASED);
	AG_SetEvent(EmuState2->fx, "mouse-button-down", ButtonDownUp, "%i", AG_BUTTON_PRESSED);
	AG_SetEvent(EmuState2->fx, "mouse-button-up", ButtonDownUp, "%i", AG_BUTTON_RELEASED);
    AG_SetEvent(EmuState2->fx, "lock-texture", LockTexture, "%p", EmuState2);
    AG_SetEvent(EmuState2->agwin, "window-detached", WindowDetached, "%p", EmuState2);

   // VCC Hot FN Keys

    AG_ActionFn(EmuState2->agwin, "F3", CPUIncDecF3F4, "%i", 1);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F3, AG_KEYMOD_ANY, "F3");
    AG_ActionFn(EmuState2->agwin, "F4", CPUIncDecF3F4, "%i", -1);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F4, AG_KEYMOD_ANY, "F4");
    AG_ActionFn(EmuState2->agwin, "F5", SoftResetF5, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F5, AG_KEYMOD_ANY, "F5");
    AG_ActionFn(EmuState2->agwin, "F6", MonitorChangeF6, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F6, AG_KEYMOD_ANY, "F6");
    AG_ActionFn(EmuState2->agwin, "F7", ExitF7, "%p", EmuState2);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F7, AG_KEYMOD_ANY, "F7");
    AG_ActionFn(EmuState2->agwin, "F8", ThrottleSpeedF8, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F8, AG_KEYMOD_ANY, "F8");
    AG_ActionFn(EmuState2->agwin, "F9", HardResetF9, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F9, AG_KEYMOD_ANY, "F9");
    AG_ActionFn(EmuState2->agwin, "F10", FullScreenStatusF10, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F10, AG_KEYMOD_ANY, "F10");
    AG_ActionFn(EmuState2->agwin, "F11", FullScreenF11, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F11, AG_KEYMOD_ANY, "F11");
    AG_ActionFn(EmuState2->agwin, "F12", About, NULL);
    AG_ActionOnKeyUp(EmuState2->agwin, AG_KEY_F12, AG_KEYMOD_ANY, "F12");
}
