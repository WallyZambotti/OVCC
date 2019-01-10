#ifndef __TCC1014GRAPHICSSDL_H__
#define __TCC1014GRAPHICSSDL_H__
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


void UpdateScreenSDL(SystemState2 *);

void DrawBottomBoarderSDL(SystemState2 *);

void DrawTopBoarderSDL(SystemState2 *);


void SetGimeVdgOffsetSDL (unsigned char );
void SetGimeVdgModeSDL (unsigned char);
void SetGimeVdgMode2SDL (unsigned char);

void SetVerticalOffsetRegisterSDL(unsigned short);
void SetCompatModeSDL(unsigned char);
void SetGimePalletSDL(unsigned char,unsigned char);
void SetGimeVmodeSDL(unsigned char);
void SetGimeVresSDL(unsigned char);
void SetGimeHorzOffsetSDL(unsigned char);
void SetGimeBoarderColorSDL(unsigned char);
void SetVidMaskSDL(unsigned int);
void InvalidateBoarderSDL(void);

void GimeInitSDL(void);
void GimeResetSDL(void);
void SetVideoBankSDL(unsigned char);
unsigned char SetMonitorTypeSDL(unsigned char );
void SetBoarderChangeSDL (unsigned char);
//unsigned char SetArtifactsSDL(unsigned char);
//unsigned char SetColorInvertSDL(unsigned char);

//void Cls(unsigned int);
unsigned char SetScanLinesSDL(unsigned char);
void SetBlinkStateSDL(unsigned char);
#define MRGB	1
#define MCMP	0

//void Set8BitPalette(void);

extern unsigned char Lpf[4];
extern unsigned char VcenterTable[4];

#endif
