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

#include "defines.h"
#include "tcc1014graphicsSDL.h"
#include "coco3.h"
#include "cc2font.h"
#include "cc3font.h"
#include "config.h"
#include "logger.h"
#include "math.h"
#include <stdio.h>

void SetupDisplay(void); //This routine gets called every time a software video register get updated.
void MakeRGBPalette (void);
void MakeCMPpalette(void);
static unsigned char ColorValues[4]={0,85,170,255};
static unsigned char ColorTable16Bit[4]={0,10,21,31};	//Color brightness at 0 1 2 and 3 (2 bits)
static unsigned char ColorTable32Bit[4]={0,85,170,255};	
static unsigned short Afacts16[2][4]={0,0xF800,0x001F,0xFFFF,0,0x001F,0xF800,0xFFFF};
static unsigned char Afacts8[2][4]={0,0xA4,0x89,0xBF,0,137,164,191};
static unsigned int Afacts32[2][4]={0,0xFF0000,0xFF,0xFFFFFF,0,0xFF,0xFF0000,0xFFFFFF}; //FIX ME
static unsigned char  Pallete[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};		//Coco 3 6 bit colors
static unsigned char  Pallete8Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned short Pallete16Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	//Color values translated to 16bit 32BIT
static unsigned int   Pallete32Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	//Color values translated to 24/32 bits

static unsigned int VidMask=0x1FFFF;
static unsigned char VresIndex=0;
static unsigned char CC2Offset=0 ,CC2VDGMode=0, CC2VDGPiaMode=0;
static unsigned short VerticalOffsetRegister=0;
static unsigned char CompatMode=0;
static unsigned char  PalleteLookup8[2][64];	//0 = RGB 1=comp 8BIT
static unsigned short PalleteLookup16[2][64];	//0 = RGB 1=comp 16BIT
static unsigned int   PalleteLookup32[2][64];	//0 = RGB 1=comp 32BIT
static unsigned char MonType=1;
static unsigned char CC3Vmode=0, CC3Vres=0, CC3BoarderColor=0;
static unsigned int StartofVidram=0 ,Start=0 ,NewStartofVidram=0;
static unsigned char LinesperScreen=0;
static unsigned char Bpp=0;
static unsigned char LinesperRow=1 ,BytesperRow=32;
static unsigned char GraphicsMode=0;
static unsigned char TextFGColor=0 ,TextBGColor=0;
static unsigned char TextFGPallete=0, TextBGPallete=0;
static unsigned char PalleteIndex=0;
static unsigned short PixelsperLine=0, VPitch=32;
static unsigned char Stretch=0, PixelsperByte=0;
static unsigned char HorzCenter=0, VertCenter=0;
static unsigned char LowerCase=0, InvertAll=0, ExtendedText=1;
static unsigned char HorzOffsetReg=0;
static unsigned char Hoffset=0;
static unsigned short TagY=0;
static unsigned short HorzBeam=0;
static unsigned int BoarderColor32=0;
static unsigned short BoarderColor16=0;
static unsigned char BoarderColor8=0;
static unsigned int DistoOffset=0;
static unsigned char BoarderChange=3;
static unsigned char MasterMode=0;
static unsigned char ColorInvert=1;
static unsigned char BlinkState=1;

unsigned char Lpf[4]={192,199,225,225}; // 2 is really undefined but I gotta put something here.
unsigned char VcenterTable[4]={29,23,12,12};

// BEGIN of 32 Bit render loop *****************************************************************************************
void UpdateScreenSDL (SystemState2 *USState32)
{
	register unsigned int YStride=0;
	unsigned char Pixel=0;
	unsigned char Character=0,Attributes=0;
	unsigned int TextPallete[2]={0,0};
	unsigned short * WideBuffer=(unsigned short *)USState32->RamBuffer;
	unsigned char *buffer=USState32->RamBuffer;
	unsigned short WidePixel=0;
	char Pix=0,Bit=0,Sphase=0;
	static char Carry1=0,Carry2=0;
	static char Pcolor=0;
	unsigned int *szSurface32=USState32->PTRsurface32;
	unsigned short y=USState32->LineCounter;
	long Xpitch=USState32->SurfacePitch;
	Carry1=1;
	Pcolor=0;
	
	if ( (HorzCenter!=0) & (BoarderChange>0) )
		for (unsigned short x=0;x<HorzCenter;x++)
		{
			szSurface32[x +(((y+VertCenter)*2)*Xpitch)]=BoarderColor32;
			if (!USState32->ScanLines)
				szSurface32[x +(((y+VertCenter)*2+1)*Xpitch)]=BoarderColor32;
			szSurface32[x + (PixelsperLine * (Stretch+1)) +HorzCenter+(((y+VertCenter)*2)*Xpitch)]=BoarderColor32;
			if (!USState32->ScanLines)
				szSurface32[x + (PixelsperLine * (Stretch+1))+HorzCenter+(((y+VertCenter)*2+1)*Xpitch)]=BoarderColor32;
		}

	if (LinesperRow < 13)
		TagY++;
	
	if (!y)
	{
		StartofVidram=NewStartofVidram;
		TagY=y;
	}
	Start=StartofVidram+(TagY/LinesperRow)*(VPitch*ExtendedText);
	YStride=(((y+VertCenter)*2)*Xpitch)+(HorzCenter*1)-1;

	switch (MasterMode) // (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
	{
		// case 0:		//Hi Res text GraphicsMode=0 CompatMode=0 Ignore Bpp and Stretch
		case 0: //Width 80
			Attributes=0;
			for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
			{									
				Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
				Pixel=cc3Fontdata8x12[Character * 12 + (y%LinesperRow)]; 

				if (ExtendedText==2)
				{
					Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
					if  ( (Attributes & 64) && (y%LinesperRow==(LinesperRow-1)) )	//UnderLine
						Pixel=255;
					if ((!BlinkState) & !!(Attributes & 128))
						Pixel=0;
				}
				TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
				TextPallete[0]=Pallete32Bit[Attributes & 7];
				szSurface32[YStride+=1]=TextPallete[Pixel >>7 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
				szSurface32[YStride+=1]=TextPallete[Pixel&1];
				if (!USState32->ScanLines)
				{
					YStride-=(8);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=TextPallete[Pixel >>7 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[Pixel&1];
					YStride-=Xpitch;
				}
			} 

		break;
		case 1:
		case 2: //Width 40
			Attributes=0;
			for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
			{									
				Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
				Pixel=cc3Fontdata8x12[Character  * 12 + (y%LinesperRow)]; 
				if (ExtendedText==2)
				{
					Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
					if  ( (Attributes & 64) && (y%LinesperRow==(LinesperRow-1)) )	//UnderLine
						Pixel=255;
					if ((!BlinkState) & !!(Attributes & 128))
						Pixel=0;
				}
				TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
				TextPallete[0]=Pallete32Bit[Attributes & 7];
				szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
				szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
					szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					YStride-=Xpitch;
				}
			} 
		break;

		//	case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:		
		case 17:		
		case 18:
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
		case 31:
		case 32:		
		case 33:			
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 50:
		case 51:
		case 52:
		case 53:
		case 54:
		case 55:
		case 56:
		case 57:
		case 58:
		case 59:
		case 60:
		case 61:
		case 62:
		case 63:
			return;
			for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
			{									
				Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
				if (ExtendedText==2)
					Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
				else
					Attributes=0;
				Pixel=cc3Fontdata8x12[(Character & 127) * 8 + (y%8)]; 
				if  ( (Attributes & 64) && (y%8==7) )	//UnderLine
					Pixel=255;
				if ((!BlinkState) & !!(Attributes & 128))
					Pixel=0;				
				TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
				TextPallete[0]=Pallete32Bit[Attributes & 7];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 128)/128 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 64)/64];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 32)/32 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 16)/16];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 8)/8 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 4)/4];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 2)/2 ];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
			} 
		break;

		//******************************************************************** Low Res Text
		case 64:		//Low Res text GraphicsMode=0 CompatMode=1 Ignore Bpp and Stretch
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:
		case 72:
		case 73:
		case 74:
		case 75:
		case 76:
		case 77:
		case 78:
		case 79:
		case 80:		
		case 81:		
		case 82:
		case 83:
		case 84:
		case 85:
		case 86:
		case 87:
		case 88:
		case 89:
		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:		
		case 97:	
		case 98:
		case 99:
		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
		case 108:
		case 109:
		case 110:
		case 111:
		case 112:		
		case 113:			
		case 114:
		case 115:
		case 116:
		case 117:
		case 118:
		case 119:
		case 120:
		case 121:
		case 122:
		case 123:
		case 124:
		case 125:
		case 126:
		case 127:

			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam++)
			{										
				Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
				switch ((Character & 192) >> 6)
				{
				case 0:
					Character = Character & 63;
					TextPallete[0]=Pallete32Bit[TextBGPallete];
					TextPallete[1]=Pallete32Bit[TextFGPallete];
					if (LowerCase & (Character < 32))
						Pixel=ntsc_round_fontdata8x12[(Character + 80 )*12+ (y%12)];
					else
						Pixel=~ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
				break;

				case 1:
					Character = Character & 63;
					TextPallete[0]=Pallete32Bit[TextBGPallete];
					TextPallete[1]=Pallete32Bit[TextFGPallete];
					Pixel=ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
				break;

				case 2:
				case 3:
					TextPallete[1] = Pallete32Bit[(Character & 112) >> 4];
					TextPallete[0] = Pallete32Bit[8];
					Character = 64 + (Character & 0xF);
					Pixel=ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
				break;

				} //END SWITCH
				szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
				szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					YStride-=Xpitch;
				}
			
			} // Next HorzBeam

		break;

		case 128+0: //Bpp=0 Sr=0 1BPP Stretch=1

			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
					YStride-=Xpitch;
			}
		}
		break;
		//case 192+1:
		case 128+1: //Bpp=0 Sr=1 1BPP Stretch=2
		case 128+2:	//Bpp=0 Sr=2 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(32);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
					YStride-=Xpitch;
			}
		}
			break;

		case 128+3: //Bpp=0 Sr=3 1BPP Stretch=4
		case 128+4: //Bpp=0 Sr=4
		case 128+5: //Bpp=0 Sr=5
		case 128+6: //Bpp=0 Sr=6
		for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
		{
			WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

			if (!USState32->ScanLines)
			{
				YStride-=(64);
				YStride+=Xpitch;
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				YStride-=Xpitch;
			}
		}
		break;
		case 128+7: //Bpp=0 Sr=7 1BPP Stretch=8 
		case 128+8: //Bpp=0 Sr=8
		case 128+9: //Bpp=0 Sr=9
		case 128+10: //Bpp=0 Sr=10
		case 128+11: //Bpp=0 Sr=11
		case 128+12: //Bpp=0 Sr=12
		case 128+13: //Bpp=0 Sr=13
		case 128+14: //Bpp=0 Sr=14
		for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
		{
			WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

			if (!USState32->ScanLines)
			{
				YStride-=(128);
				YStride+=Xpitch;
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
				YStride-=Xpitch;
			}
		}
		break;

		case 128+15: //Bpp=0 Sr=15 1BPP Stretch=16
		case 128+16: //BPP=1 Sr=0  2BPP Stretch=1
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				if (!USState32->ScanLines)
				{
					YStride-=(8);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+17: //Bpp=1 Sr=1  2BPP Stretch=2
		case 128+18: //Bpp=1 Sr=2
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+19: //Bpp=1 Sr=3  2BPP Stretch=4
		case 128+20: //Bpp=1 Sr=4
		case 128+21: //Bpp=1 Sr=5
		case 128+22: //Bpp=1 Sr=6
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(32);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+23: //Bpp=1 Sr=7  2BPP Stretch=8
		case 128+24: //Bpp=1 Sr=8
		case 128+25: //Bpp=1 Sr=9 
		case 128+26: //Bpp=1 Sr=10 
		case 128+27: //Bpp=1 Sr=11 
		case 128+28: //Bpp=1 Sr=12 
		case 128+29: //Bpp=1 Sr=13 
		case 128+30: //Bpp=1 Sr=14
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(64);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;
			
		case 128+31: //Bpp=1 Sr=15 2BPP Stretch=16 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(128);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+32: //Bpp=2 Sr=0 4BPP Stretch=1
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=1
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				if (!USState32->ScanLines)
				{
					YStride-=(4);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+33: //Bpp=2 Sr=1 4BPP Stretch=2 
		case 128+34: //Bpp=2 Sr=2
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=2
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				if (!USState32->ScanLines)
				{
					YStride-=(8);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+35: //Bpp=2 Sr=3 4BPP Stretch=4
		case 128+36: //Bpp=2 Sr=4 
		case 128+37: //Bpp=2 Sr=5 
		case 128+38: //Bpp=2 Sr=6 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=4
			{
				WidePixel=WideBuffer[(VidMask & (Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+39: //Bpp=2 Sr=7 4BPP Stretch=8
		case 128+40: //Bpp=2 Sr=8 
		case 128+41: //Bpp=2 Sr=9 
		case 128+42: //Bpp=2 Sr=10 
		case 128+43: //Bpp=2 Sr=11 
		case 128+44: //Bpp=2 Sr=12 
		case 128+45: //Bpp=2 Sr=13 
		case 128+46: //Bpp=2 Sr=14 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=8
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(32);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;

		case 128+47: //Bpp=2 Sr=15 4BPP Stretch=16
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=16
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
				szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

				if (!USState32->ScanLines)
				{
					YStride-=(64);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
					YStride-=Xpitch;
				}
			}
		break;	
		case 128+48: //Bpp=3 Sr=0  Unsupported 
		case 128+49: //Bpp=3 Sr=1 
		case 128+50: //Bpp=3 Sr=2 
		case 128+51: //Bpp=3 Sr=3 
		case 128+52: //Bpp=3 Sr=4 
		case 128+53: //Bpp=3 Sr=5 
		case 128+54: //Bpp=3 Sr=6 
		case 128+55: //Bpp=3 Sr=7 
		case 128+56: //Bpp=3 Sr=8 
		case 128+57: //Bpp=3 Sr=9 
		case 128+58: //Bpp=3 Sr=10 
		case 128+59: //Bpp=3 Sr=11 
		case 128+60: //Bpp=3 Sr=12 
		case 128+61: //Bpp=3 Sr=13 
		case 128+62: //Bpp=3 Sr=14 
		case 128+63: //Bpp=3 Sr=15 
			return;
			break;
		//	XXXXXXXXXXXXXXXXXXXXXX;
		case 192+0: //Bpp=0 Sr=0 1BPP Stretch=1
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
					YStride-=Xpitch;
			}
		}
		break;

		case 192+1: //Bpp=0 Sr=1 1BPP Stretch=2
		case 192+2:	//Bpp=0 Sr=2 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		//************************************************************************************
				if (!MonType)
				{ //Pcolor
					for (Bit=7;Bit>=0;Bit--)
					{
						Pix=(1 & (WidePixel>>Bit) );
						Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
						switch(Sphase)
						{
						case 0:
						case 4:
						case 6:
							Pcolor=0;
							break;
						case 1:
						case 5:
							Pcolor=(Bit &1)+1;
							break;
						case 2:
						//	Pcolor=(!(Bit &1))+1; Use last color
							break;
						case 3:
							Pcolor=3;
							szSurface32[YStride-1]=Afacts32[ColorInvert][3];
							if (!USState32->ScanLines)
								szSurface32[YStride+Xpitch-1]=Afacts32[ColorInvert][3];
							szSurface32[YStride]=Afacts32[ColorInvert][3];
							if (!USState32->ScanLines)
								szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][3];
							break;
						case 7:
							Pcolor=3;
							break;
						} //END Switch

						szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
						if (!USState32->ScanLines)
							szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
						szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
						if (!USState32->ScanLines)
							szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
						Carry2=Carry1;
						Carry1=Pix;
					}

					for (Bit=15;Bit>=8;Bit--)
					{
						Pix=(1 & (WidePixel>>Bit) );
						Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
						switch(Sphase)
						{
						case 0:
						case 4:
						case 6:
							Pcolor=0;
							break;
						case 1:
						case 5:
							Pcolor=(Bit &1)+1;
							break;
						case 2:
						//	Pcolor=(!(Bit &1))+1; Use last color
							break;
						case 3:
							Pcolor=3;
							szSurface32[YStride-1]=Afacts32[ColorInvert][3];
							if (!USState32->ScanLines)
								szSurface32[YStride+Xpitch-1]=Afacts32[ColorInvert][3];
							szSurface32[YStride]=Afacts32[ColorInvert][3];
							if (!USState32->ScanLines)
								szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][3];
							break;
						case 7:
							Pcolor=3;
							break;
						} //END Switch

						szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
						if (!USState32->ScanLines)
							szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
						szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
						if (!USState32->ScanLines)
							szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
						Carry2=Carry1;
						Carry1=Pix;
					}

				}
					else
					{
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
					if (!USState32->ScanLines)
					{
						YStride-=(32);
						YStride+=Xpitch;
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
						szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
						YStride-=Xpitch;
					}
				}

		}
			break;

		case 192+3: //Bpp=0 Sr=3 1BPP Stretch=4
		case 192+4: //Bpp=0 Sr=4
		case 192+5: //Bpp=0 Sr=5
		case 192+6: //Bpp=0 Sr=6
		for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
		{
			WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];

			if (!USState32->ScanLines)
			{
				YStride-=(64);
				YStride+=Xpitch;
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				YStride-=Xpitch;
			}
		}
		break;
		case 192+7: //Bpp=0 Sr=7 1BPP Stretch=8 
		case 192+8: //Bpp=0 Sr=8
		case 192+9: //Bpp=0 Sr=9
		case 192+10: //Bpp=0 Sr=10
		case 192+11: //Bpp=0 Sr=11
		case 192+12: //Bpp=0 Sr=12
		case 192+13: //Bpp=0 Sr=13
		case 192+14: //Bpp=0 Sr=14
		for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
		{
			WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];

			if (!USState32->ScanLines)
			{
				YStride-=(128);
				YStride+=Xpitch;
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				YStride-=Xpitch;
			}
		}
		break;

		case 192+15: //Bpp=0 Sr=15 1BPP Stretch=16
		case 192+16: //BPP=1 Sr=0  2BPP Stretch=1
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				if (!USState32->ScanLines)
				{
					YStride-=(8);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					YStride-=Xpitch;
				}
			}
		break;

		case 192+17: //Bpp=1 Sr=1  2BPP Stretch=2
		case 192+18: //Bpp=1 Sr=2
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

				if (!USState32->ScanLines)
				{
					YStride-=(16);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					YStride-=Xpitch;
				}
			}
		break;

		case 192+19: //Bpp=1 Sr=3  2BPP Stretch=4
		case 192+20: //Bpp=1 Sr=4
		case 192+21: //Bpp=1 Sr=5
		case 192+22: //Bpp=1 Sr=6
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

				if (!USState32->ScanLines)
				{
					YStride-=(32);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					YStride-=Xpitch;
				}
			}
		break;

		case 192+23: //Bpp=1 Sr=7  2BPP Stretch=8
		case 192+24: //Bpp=1 Sr=8
		case 192+25: //Bpp=1 Sr=9 
		case 192+26: //Bpp=1 Sr=10 
		case 192+27: //Bpp=1 Sr=11 
		case 192+28: //Bpp=1 Sr=12 
		case 192+29: //Bpp=1 Sr=13 
		case 192+30: //Bpp=1 Sr=14
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

				if (!USState32->ScanLines)
				{
					YStride-=(64);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					YStride-=Xpitch;
				}
			}
		break;
			
		case 192+31: //Bpp=1 Sr=15 2BPP Stretch=16 
			for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
			{
				WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

				if (!USState32->ScanLines)
				{
					YStride-=(128);
					YStride+=Xpitch;
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
					YStride-=Xpitch;
				}
			}
		break;

		case 192+32: //Bpp=2 Sr=0 4BPP Stretch=1 Unsupport with Compat set
		case 192+33: //Bpp=2 Sr=1 4BPP Stretch=2 
		case 192+34: //Bpp=2 Sr=2
		case 192+35: //Bpp=2 Sr=3 4BPP Stretch=4
		case 192+36: //Bpp=2 Sr=4 
		case 192+37: //Bpp=2 Sr=5 
		case 192+38: //Bpp=2 Sr=6 
		case 192+39: //Bpp=2 Sr=7 4BPP Stretch=8
		case 192+40: //Bpp=2 Sr=8 
		case 192+41: //Bpp=2 Sr=9 
		case 192+42: //Bpp=2 Sr=10 
		case 192+43: //Bpp=2 Sr=11 
		case 192+44: //Bpp=2 Sr=12 
		case 192+45: //Bpp=2 Sr=13 
		case 192+46: //Bpp=2 Sr=14 
		case 192+47: //Bpp=2 Sr=15 4BPP Stretch=16
		case 192+48: //Bpp=3 Sr=0  Unsupported 
		case 192+49: //Bpp=3 Sr=1 
		case 192+50: //Bpp=3 Sr=2 
		case 192+51: //Bpp=3 Sr=3 
		case 192+52: //Bpp=3 Sr=4 
		case 192+53: //Bpp=3 Sr=5 
		case 192+54: //Bpp=3 Sr=6 
		case 192+55: //Bpp=3 Sr=7 
		case 192+56: //Bpp=3 Sr=8 
		case 192+57: //Bpp=3 Sr=9 
		case 192+58: //Bpp=3 Sr=10 
		case 192+59: //Bpp=3 Sr=11 
		case 192+60: //Bpp=3 Sr=12 
		case 192+61: //Bpp=3 Sr=13 
		case 192+62: //Bpp=3 Sr=14 
		case 192+63: //Bpp=3 Sr=15 
		//	return;
		break;

	} //END SWITCH
	
	return;
}
// END of 32 Bit render loop *****************************************************************************************


void DrawTopBoarderSDL(SystemState2 *DTState)
{

	unsigned short x;
	if (BoarderChange==0)
		return;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface32[x +((DTState->LineCounter*2)*DTState->SurfacePitch)]=BoarderColor32;
		if (!DTState->ScanLines)
			DTState->PTRsurface32[x +((DTState->LineCounter*2+1)*DTState->SurfacePitch)]=BoarderColor32;
	}
	return;
}



void DrawBottomBoarderSDL(SystemState2 *DTState)
{
	if (BoarderChange==0)
		return;	
	unsigned short x;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface32[x + (2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor32;
		if (!DTState->ScanLines)
			DTState->PTRsurface32[x + DTState->SurfacePitch+(2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor32;
	}
	return;
}

void SetBlinkStateSDL(unsigned char Tmp)
{
	BlinkState=Tmp;
	return;
}

// These grab the Video info for all COCO 2 modes
void SetGimeVdgOffsetSDL (unsigned char Offset)
{
	if ( CC2Offset != Offset)
	{
		CC2Offset=Offset;
		SetupDisplaySDL();
	}
	return;
}

void SetGimeVdgModeSDL (unsigned char VdgMode) //3 bits from SAM Registers
{
	if (CC2VDGMode != VdgMode)
	{
		CC2VDGMode=VdgMode;
		SetupDisplaySDL();
		BoarderChange=3;
	}
	return;
}

void SetGimeVdgMode2SDL (unsigned char Vdgmode2) //5 bits from PIA Register
{
	if (CC2VDGPiaMode != Vdgmode2)
	{
		CC2VDGPiaMode=Vdgmode2;
		SetupDisplaySDL();
		BoarderChange=3;
	}
	return;
}

//These grab the Video info for all COCO 3 modes

void SetVerticalOffsetRegisterSDL(unsigned short Register)
{
	if (VerticalOffsetRegister != Register)
	{
		VerticalOffsetRegister=Register;

		SetupDisplaySDL();
	}
	return;
}

void SetCompatModeSDL( unsigned char Register)
{
	if (CompatMode != Register)
	{
		CompatMode=Register;
		SetupDisplaySDL();
		BoarderChange=3;
	}
	return;
}

void SetGimePalletSDL(unsigned char pallete,unsigned char color)
{
	// Convert the 6bit rgbrgb value to rrrrrggggggbbbbb for the Real video hardware.
	//	unsigned char r,g,b;
	Pallete[pallete]=((color &63));
	Pallete8Bit[pallete]= PalleteLookup8[MonType][color & 63]; 
	Pallete16Bit[pallete]=PalleteLookup16[MonType][color & 63];
	Pallete32Bit[pallete]=PalleteLookup32[MonType][color & 63];
	return;
}

void SetGimeVmodeSDL(unsigned char vmode)
{
	if (CC3Vmode != vmode)
	{
		CC3Vmode=vmode;
		SetupDisplaySDL();
		BoarderChange=3;

	}
	return;
}

void SetGimeVresSDL(unsigned char vres)
{
	if (CC3Vres != vres)
	{
		CC3Vres=vres;
		SetupDisplaySDL();
		BoarderChange=3;
	}
	return;
}

void SetGimeHorzOffsetSDL(unsigned char data)
{
	if (HorzOffsetReg != data)
	{
		Hoffset=(data<<1);
		HorzOffsetReg=data;
		SetupDisplaySDL();
	}
	return;
}
void SetGimeBoarderColorSDL(unsigned char data)
{

	if (CC3BoarderColor != (data & 63) )
	{
		CC3BoarderColor= data & 63;
		SetupDisplaySDL();
		BoarderChange=3;
	}
	return;
}

void SetBoarderChangeSDL(unsigned char data)
{
	if (BoarderChange >0)
		BoarderChange--;
	
	return;
}

void InvalidateBoarderSDL(void)
{
	BoarderChange=5;
	return;
}


void SetupDisplaySDL(void)
{

	static unsigned char CC2Bpp[8]={1,0,1,0,1,0,1,0};
	static unsigned char CC2LinesperRow[8]={12,3,3,2,2,1,1,1};
	static unsigned char CC3LinesperRow[8]={1,1,2,8,9,10,11,200};
	static unsigned char CC2BytesperRow[8]={16,16,32,16,32,16,32,32};
	static unsigned char CC3BytesperRow[8]={16,20,32,40,64,80,128,160};
	static unsigned char CC3BytesperTextRow[8]={32,40,32,40,64,80,64,80};
	static unsigned char CC2PaletteSet[4]={8,0,10,4};
	static unsigned char CCPixelsperByte[4]={8,4,2,2};
	static unsigned char ColorSet=0,Temp1; 
	ExtendedText=1;
	switch (CompatMode)
	{
	case 0:		//Color Computer 3 Mode
		NewStartofVidram=VerticalOffsetRegister*8; 
		GraphicsMode=(CC3Vmode & 128)>>7;
		VresIndex=(CC3Vres & 96) >> 5;
		CC3LinesperRow[7]=LinesperScreen;	// For 1 pixel high modes
		Bpp=CC3Vres & 3;
		LinesperRow=CC3LinesperRow[CC3Vmode & 7];
		BytesperRow=CC3BytesperRow[(CC3Vres & 28)>> 2];
		PalleteIndex=0;
		if (!GraphicsMode)
		{
			if (CC3Vres & 1)
				ExtendedText=2;
			Bpp=0;
			BytesperRow=CC3BytesperTextRow[(CC3Vres & 28)>> 2];
		}
		break;

	case 1:					//Color Computer 2 Mode
		CC3BoarderColor=0;	//Black for text modes
		BoarderChange=3;
		NewStartofVidram= (512*CC2Offset)+(VerticalOffsetRegister & 0xE0FF)*8; 
		GraphicsMode=( CC2VDGPiaMode & 16 )>>4; //PIA Set on graphics clear on text
		VresIndex=0;
		LinesperRow= CC2LinesperRow[CC2VDGMode];

		if (GraphicsMode)
		{
			CC3BoarderColor=63;
			BoarderChange=3;
			Bpp=CC2Bpp[ (CC2VDGPiaMode & 15) >>1 ];
			BytesperRow=CC2BytesperRow[ (CC2VDGPiaMode & 15) >>1 ];
			Temp1= (CC2VDGPiaMode &1) << 1 | (Bpp & 1);
			PalleteIndex=CC2PaletteSet[Temp1];
		}
		else
		{	//Setup for 32x16 text Mode
			Bpp=0;
			BytesperRow=32;
			InvertAll= (CC2VDGPiaMode & 4)>>2;
			LowerCase= (CC2VDGPiaMode & 2)>>1;
			ColorSet = (CC2VDGPiaMode & 1);
			Temp1= ( (ColorSet<<1) | InvertAll);
			switch (Temp1)
			{
			case 0:
				TextFGPallete=12;
				TextBGPallete=13;
				break;
			case 1:
				TextFGPallete=13;
				TextBGPallete=12;
				break;
			case 2:
				TextFGPallete=14;
				TextBGPallete=15;
				break;
			case 3:
				TextFGPallete=15;
				TextBGPallete=14;
				break;
			}
		}
		break;
	}
	ColorInvert= (CC3Vmode & 32)>>5;
	LinesperScreen=Lpf[VresIndex];
	SetLinesperScreen(VresIndex);
	VertCenter=VcenterTable[VresIndex]-4; //4 unrendered top lines
	PixelsperLine= BytesperRow*CCPixelsperByte[Bpp];
	PixelsperByte=CCPixelsperByte[Bpp];

	if (PixelsperLine % 40)
	{
		Stretch = (512/PixelsperLine)-1;
		HorzCenter=64;
	}
	else
	{
		Stretch = (640/PixelsperLine)-1;
		HorzCenter=0;
	}
	VPitch=BytesperRow;
	if (HorzOffsetReg & 128)
		VPitch=256;
	BoarderColor8=((CC3BoarderColor & 63) |128);
	BoarderColor16=PalleteLookup16[MonType][CC3BoarderColor & 63];
	BoarderColor32=PalleteLookup32[MonType][CC3BoarderColor & 63];
	NewStartofVidram =(NewStartofVidram & VidMask)+DistoOffset; //DistoOffset for 2M configuration
	MasterMode= (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
	return;
}


void GimeInitSDL(void)
{
	//Nothing but good to have.
	return;
}



void GimeResetSDL(void)
{
	CC3Vmode=0;
	CC3Vres=0;
	StartofVidram=0;
	NewStartofVidram=0;
	GraphicsMode=0;
	LowerCase=0;
	InvertAll=0;
	ExtendedText=1;
	HorzOffsetReg=0;
	TagY=0;
	DistoOffset=0;
	MakeRGBPaletteSDL();
	MakeCMPpaletteSDL();
	BoarderChange=3;
	CC2Offset=0;
	Hoffset=0;
	VerticalOffsetRegister=0;
	MiscReset();
	return;
}

void SetVidMaskSDL(unsigned int data)
{
	VidMask=data;
	return;
}

void SetVideoBankSDL(unsigned char data)
{
	DistoOffset= data * (512*1024);
	SetupDisplaySDL();
	return;
}


void MakeRGBPaletteSDL(void)
{
	unsigned char Index=0;
	unsigned char r,g,b;
	for (Index=0;Index<64;Index++)
	{

		PalleteLookup8[1][Index]= Index |128;

		r= ColorTable16Bit [(Index & 32) >> 4 | (Index & 4) >> 2];
		g= ColorTable16Bit [(Index & 16) >> 3 | (Index & 2) >> 1];
		b= ColorTable16Bit [(Index & 8 ) >> 2 | (Index & 1) ];
		PalleteLookup16[1][Index]= (r<<11) | (g << 6) | b;
	//32BIT
		r= ColorTable32Bit [(Index & 32) >> 4 | (Index & 4) >> 2];	
		g= ColorTable32Bit [(Index & 16) >> 3 | (Index & 2) >> 1];	
		b= ColorTable32Bit [(Index & 8 ) >> 2 | (Index & 1) ];		
		PalleteLookup32[1][Index]= (r* 65536) + (g* 256) + b;
		
		
		
	}
	return;
}


void MakeCMPpaletteSDL(void)	//Stolen from M.E.S.S.
{
	double saturation, brightness, contrast;
	int offset;
	double w;
	double r,g,b;
	unsigned char rr,gg,bb;
	unsigned char Index=0;
	for (Index=0;Index<=63;Index++)
	{
		switch(Index)
		{
			case 0:
				r = g = b = 0;
				break;

			case 16:
				r = g = b = 47;
				break;

			case 32:
				r = g = b = 120;
				break;

			case 48:
			case 63:
				r = g = b = 255;
				break;

			default:
				w = .4195456981879*1.01;
				contrast = 70;
				saturation = 92;
				brightness = -20;
				brightness += ((Index / 16) + 1) * contrast;
				offset = (Index % 16) - 1 + (Index / 16)*15;
				r = cos(w*(offset +  9.2)) * saturation + brightness;
				g = cos(w*(offset + 14.2)) * saturation + brightness;
				b = cos(w*(offset + 19.2)) * saturation + brightness;

				if (r < 0)
					r = 0;
				else if (r > 255)
					r = 255;

				if (g < 0)
					g = 0;
				else if (g > 255)
					g = 255;

				if (b < 0)
					b = 0;
				else if (b > 255)
					b = 255;
				break;
		}

		rr= (unsigned char)r;
		gg= (unsigned char)g;
		bb= (unsigned char)b;
		PalleteLookup32[0][Index]= (rr<<16) | (gg<<8) | bb;
		rr=rr>>3;
		gg=gg>>3;
		bb=bb>>3;
		PalleteLookup16[0][Index]= (rr<<11) | (gg<<6) | bb;
		rr=rr>>3;
		gg=gg>>3;
		bb=bb>>3;
		PalleteLookup8[0][Index]=0x80 | ((rr & 2)<<4) | ((gg & 2)<<3) | ((bb & 2)<<2) | ((rr & 1)<<2) | ((gg & 1)<<1) | (bb & 1);
	}
}

unsigned char SetMonitorTypeSDL(unsigned char Type)
{
	unsigned char PalNum=0;
	if (Type != QUERY)
	{
		MonType=Type & 1;
		for (PalNum=0;PalNum<16;PalNum++)
		{
			Pallete16Bit[PalNum]=PalleteLookup16[MonType][Pallete[PalNum]];
			Pallete32Bit[PalNum]=PalleteLookup32[MonType][Pallete[PalNum]];
			Pallete8Bit[PalNum]= PalleteLookup8[MonType][Pallete[PalNum]];
		}
//		CurrentConfig.MonitorType=MonType;
	}
	return(MonType);
}

unsigned char SetScanLinesSDL(unsigned char Lines)
{
	extern SystemState2 EmuState2;
	if (Lines!=QUERY)
	{
		EmuState2.ScanLines=Lines;
		ClsSDL(0,&EmuState2);
		BoarderChange=3;
	}
	return(0);
}
/*
unsigned char SetArtifacts(unsigned char Tmp)
{
	if (Tmp!=QUERY)
	Artifacts=Tmp;
	return(Artifacts);
}
*/
/*
unsigned char SetColorInvert(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		ColorInvert=Tmp;
	return(ColorInvert);
}
*/


