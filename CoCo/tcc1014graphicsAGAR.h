#ifndef __TCC1014GRAPHICSAGAR_H__
#define __TCC1014GRAPHICSAGAR_H__
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


void UpdateScreen(SystemState2 *);

void DrawBottomBoarderAGAR(SystemState2 *);

void DrawTopBoarderAGAR(SystemState2 *);


void SetGimeVdgOffsetAGAR (unsigned char );
void SetGimeVdgModeAGAR (unsigned char);
void SetGimeVdgMode2AGAR (unsigned char);

void SetVerticalOffsetRegisterAGAR(unsigned short);
void SetCompatModeAGAR(unsigned char);
void SetGimePalletAGAR(unsigned char,unsigned char);
void SetGimeVmodeAGAR(unsigned char);
void SetGimeVresAGAR(unsigned char);
void SetGimeHorzOffsetAGAR(unsigned char);
void SetGimeBoarderColorAGAR(unsigned char);
void SetVidMaskAGAR(unsigned int);
void InvalidateBoarderAGAR(void);

void GimeInitAGAR(void);
void GimeResetAGAR(void);
void SetVideoBankAGAR(unsigned char);
unsigned char SetMonitorTypeAGAR(unsigned char );
void SetBoarderChangeAGAR (unsigned char);
//unsigned char SetArtifactsAGAR(unsigned char);
//unsigned char SetColorInvertAGAR(unsigned char);

//void Cls(unsigned int);
unsigned char SetScanLinesAGAR(unsigned char);
void SetBlinkStateAGAR(unsigned char);
#define MRGB	1
#define MCMP	0

//void Set8BitPalette(void);

extern unsigned char Lpf[4];
extern unsigned char VcenterTable[4];

#endif
