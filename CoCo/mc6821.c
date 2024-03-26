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

/*
$FF00 (65280) PIA 0 side A data
Bit 7 Joystick Comparison Input
Bit 6 Keyboard Row 7
Bit 5 Row 6
Bit 4 Row 5
Bit 3 Row 4 & Left Joystick Switch 2
Bit 2 Row 3 & Right Joystick Switch 2
Bit 1 Row 2 & Left Joystick Switch 1
Bit 0 Row 1 & Right Joystick Switch 1

$FF01 (65281) PIA 0 side A control
Bit 7 HSYNC Flag
Bit 6 Unused
Bit 5 1
Bit 4 1
Bit 3 Select Line LSB of MUX
Bit 2 DATA DIRECTION TOGGLE 0 sets data direction 1 = normal
Bit 1 IRQ POLARITY 0 = flag set on falling edge 1=set on rising edge
Bit 0 HSYNC IRQ 0 = disabled 1 = enabled

$FF02 (65282) PIA 0 side B data
Bit 7 KEYBOARD COLUMN 8
Bit 6 7 / RAM SIZE OUTPUT
Bit 5 6
Bit 4 5
Bit 3 4
Bit 2 3
Bit 1 2
Bit 0 KEYBOARD COLUMN 1

$FF03 (65283) PIA 0 side B control
Bit 7 VSYNC FLAG
Bit 6 N/A
Bit 5 1
Bit 4 1
Bit 3 SELECT LINE MSB of MUX
Bit 2 data direction 1=normal
Bit 1 IRQ POLARITY 0=flag set on falling edge 1=set on rising edge
Bit 0 VSYNC IRQ 0=disabled 1=enabled

$FF20 (65312) PIA 1 side A data
Bit 7 6 BIT DAC MSB
Bit 6
Bit 5
Bit 4
Bit 3
Bit 2 6 BIT DAC LSB
Bit 1 RS-232C DATA OUTPUT
Bit 0 CASSETTE DATA INPUT

$FF21 (65313) PIA 1 side A control
Bit 7 CD FIRQ FLAG
Bit 6 N/A
Bit 5 1
Bit 4 1
Bit 3 CASSETTE MOTOR CONTROL 0=OFF 1=ON
Bit 2 data direction 1=normal
Bit 1 FIRQ POLARITY 0=falling 1=rising
Bit 0 CD FIRQ (RS-232C) 0=FIRQ disabled 1=enabled

$FF22 (65314) PIA 1 side B data reg
Bit 7 VDG CONTROL A/G Alphanum = 0, graphics =1
Bit 6 VDG CONTROL GM2
Bit 5 GM1 & invert
Bit 4 VDG CONTROL GM0 & shift toggle
Bit 3 RGB Monitor sensing (INPUT) CSS - Color Set Select 0,1
Bit 2 RAM SIZE INPUT
Bit 1 SINGLE BIT SOUND OUTPUT
Bit 0 RS-232C DATA INPUT

$FF23 (65315) PIA 1 side B control
Bit 7 CART FIRQ FLAG
Bit 6 N/A
Bit 5 1
Bit 4 1
Bit 3 SOUND ENABLE
Bit 2 $FF22 data direction 1 = normal
Bit 1 FIRQ POLARITY 0 = falling 1 = rising
Bit 0 CART FIRQ 0 = FIRQ disabled 1 = enabled
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "defines.h"
#include "mc6821.h"
#include "hd6309.h"
#include "keyboard.h"
#include "tcc1014graphicsAGAR.h"
#include "tcc1014registers.h"
#include "coco3.h"
#include "pakinterface.h"
#include "cassette.h"
#include "logger.h"
#include "resource.h"

#include "xdebug.h"

short int DACdischarging = 0;

extern int gWindow;

static unsigned char rega[4]={0,0,0,0};	//PIA0
static unsigned char regb[4]={0,0,0,0};	//PIA1
static unsigned char rega_dd[4]={0,0,0,0};
static unsigned char regb_dd[4]={0,0,0,0};
static unsigned char LeftChannel=0,RightChannel=0;
static unsigned char Asample=0,Ssample=0,Csample=0;
static unsigned char CartInserted=0,CartAutoStart=1;
static unsigned char AddLF=0;
static AG_DataSource *hPrintFile=NULL;
static AG_DataSource *hout=NULL;
void WritePrintMon(char *);
static BOOL MonState=FALSE;

void CaptureBit(unsigned char Sample);//static unsigned char CoutSample=0;
//extern STRConfig CurrentConfig;
// Shift Row Col
unsigned char pia0_read(unsigned char port)
{
	unsigned char dda,ddb;
	dda=(rega[1] & 4);
	ddb=(rega[3] & 4);

	switch (port)
	{
		case 1:	// cpu read FF01
			return(rega[port]);
		break;

		case 3:	// cpu read FF03
			return(rega[port]);
		break;

		case 0:	// cpu read FF00
			if (dda)
			{
				unsigned char scan;
				rega[1]=(rega[1] & 63);
				scan = vccKeyboardGetScanAGAR(rega[2]);
				if (scan != 0xFF)
				{
					//XTRACE("dda<%2X>\n", scan);
				}
				return (scan); //Read
			}
			else
			{
				unsigned char scan;
				scan = rega_dd[port];
				//XTRACE("ddb<%2X>\n", scan);
				return(scan);
			}
		break;

		case 2:	// cpu read FF02
			if (ddb)
			{
				rega[3]=(rega[3] & 63);
				return(rega[port] & rega_dd[port]);
			}
			else
				return(rega_dd[port]);
		break;
	}
	return(0);
}

unsigned char pia1_read(unsigned char port)
{
	static unsigned int Flag=0,Flag2=0;
	unsigned char dda,ddb;
	port-=0x20;
	dda=(regb[1] & 4);
	ddb=(regb[3] & 4);

	switch (port)
	{
		case 1:	// cpu read FF21
		//	return(0);
		case 3:	// cpu read FF23
			return(regb[port]);
		break;

		case 2:	// cpu read FF22
			if (ddb)
			{
				regb[3]= (regb[3] & 63);

				return(regb[port] & regb_dd[port]);
			}
			else
				return(regb_dd[port]);
		break;

		case 0:	// cpu read FF20
			if (dda)
			{
				regb[1]=(regb[1] & 63); //Cass In
				Flag=regb[port] ;//& regb_dd[port];
				return(Flag);
			}
			else {
				return(regb_dd[port]);
			}
		break;
	}
	return(0);
}

void pia0_write(unsigned char data,unsigned char port)
{
	unsigned char dda,ddb;
	dda=(rega[1] & 4);
	ddb=(rega[3] & 4);

	switch (port)
	{
	case 0:	// cpu write FF00
		if (dda)
		{
			rega[port]=data;
			//fprintf(stdout, "%2x  ", data); fflush(stdout);
		}
		else
		{
			rega_dd[port]=data;
			//fprintf(stdout, "%2d-", data); fflush(stdout);
		}
		return;
	case 2:	// cpu write FF02
		if (ddb)
			rega[port]=data;
		else
			rega_dd[port]=data;
		return;
	break;

	case 1:	// cpu write FF01
		rega[port]= (data & 0x3F);
		return;
	break;

	case 3:	// cpu write FF03
		rega[port]= (data & 0x3F);
		return;
	break;
	}
	return;
}

void pia1_write(unsigned char data,unsigned char port)
{
	unsigned char dda,ddb;
	static unsigned short LastSS=0;
	static unsigned short chargeHigh = 0;
	port-=0x20;

	dda=(regb[1] & 4);
	ddb=(regb[3] & 4);
	switch (port)
	{
	case 0:	// cpu write FF20
		if (dda)
		{
			regb[port]=data;
			//fprintf(stdout, "%2d+", data); fflush(stdout);
			CaptureBit((regb[0]&2)>>1);
			if (GetMuxState()==0)
				if ((regb[3] & 8)!=0)//==0 for cassette writes
					Asample	= (regb[0] & 0xFC)>>1; //0 to 127
				else
					Csample = (regb[0] & 0xFC);

			if (chargeHigh && (data >> 2) == 0)
			{
				// A discharge from Max to Min has occurred.  Possible Hi-res joystick device in use
				DACdischarging = 1;
				// clear the comparator
				regb[port] = regb[port] & 0X7F;
				extern unsigned char ComparatorSetByDischarge;
				ComparatorSetByDischarge = 0;

				// fprintf(stdout, "*"); fflush(stdout);
			}

			chargeHigh = ((data >> 2) == 63);
		}
		else
			regb_dd[port]=data;
		return;
	break;

	case 2:	// cpu write FF22
		if (ddb)
		{
			regb[port]=(data & regb_dd[port]);
			SetGimeVdgMode2AGAR( (regb[2] & 248) >>3);
			Ssample=(regb[port] & 2)<<6;
		}
		else
			regb_dd[port]=data;
		return;
	break;

	case 1:	// cpu write FF21
		regb[port]= (data & 0x3F);
		Motor((data & 8)>>3);
		return;
	break;

	case 3:	// cpu write FF22
		regb[port]= (data & 0x3F);
		return;
	break;
	}
	return;
}

unsigned char VDG_Mode(void)
{
	return( (regb[2] & 248) >>3);
}


void irq_hs(int phase)	//63.5 uS
{

	switch (phase)
	{
	case FALLING:	//HS went High to low
		if ( (rega[1] & 2) ) //IRQ on low to High transition
			return;
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;

	case RISING:	//HS went Low to High
		if ( !(rega[1] & 2) ) //IRQ  High to low transition
		return;
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;

	case ANY:
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;
	} //END switch

	return;
}

void irq_fs(int phase)	//60HZ Vertical sync pulse 16.667 mS
{
	if ((CartInserted==1) & (CartAutoStart==1))
	{
		AssertCart();
	}
	switch (phase)
	{
		case 0:	//FS went High to low
			if ( (rega[3] & 2)==0 ) //IRQ on High to low transition
			{
				rega[3]=(rega[3] | 128);
				if (rega[3] & 1)
				{
					//write(0, "v", 1);
					CPUAssertInterupt(IRQ,1);
				}
			}
			return;
		break;

		case 1:	//FS went Low to High
			if ( (rega[3] & 2) ) //IRQ  Low to High transition
			{
				rega[3]=(rega[3] | 128);
				if (rega[3] & 1)
				{
					//write(0, "^", 1);
					CPUAssertInterupt(IRQ,1);
				}
			}
			return;
		break;
	} //END switch

	return;
}

void AssertCart(void)
{
	regb[3]=(regb[3] | 128);
	if (regb[3] & 1)
		CPUAssertInterupt(FIRQ,0);
	else
		CPUDeAssertInterupt(FIRQ); //Kludge but working
}

void PiaReset()
{
	// Clear the PIA registers
	for (uint8_t index=0; index<4; index++)
	{
		rega[index]=0;
		regb[index]=0;
		rega_dd[index]=0;
		regb_dd[index]=0;
	}
}

unsigned char GetMuxState(void)
{
	return ( ((rega[1] & 8)>>3) + ((rega[3] & 8) >>2));
}

unsigned char DACState(void)
{
	return (regb[0]>>2);
}

void SetCart(unsigned char cart)
{
	CartInserted=cart;
	return;
}

unsigned int GetDACSample(void)
{
	static unsigned int RetVal=0;
	static unsigned short SampleLeft=0,SampleRight=0,PakSample=0;
	static unsigned short OutLeft=0,OutRight=0;
	static unsigned short LastLeft=0,LastRight=0;
	PakSample=PackAudioSample();
	SampleLeft=(PakSample>>8)+Asample+Ssample;
	SampleRight=(PakSample & 0xFF)+Asample+Ssample; //9 Bits each
	SampleLeft=SampleLeft<<6;	//Conver to 16 bit values
	SampleRight=SampleRight<<6;	//For Max volume
	if (SampleLeft==LastLeft)	//Simulate a slow high pass filter
	{
		if (OutLeft)
			OutLeft--;
	}
	else
	{
		OutLeft=SampleLeft;
		LastLeft=SampleLeft;
	}

	if (SampleRight==LastRight)
	{
		if (OutRight)
			OutRight--;
	}
	else
	{
		OutRight=SampleRight;
		LastRight=SampleRight;
	}

	RetVal=(OutLeft<<16)+(OutRight);
	return(RetVal);
}

unsigned char SetCartAutoStart(unsigned char Tmp)
{
	if (Tmp !=QUERY)
		CartAutoStart=Tmp;
	return(CartAutoStart);
}

unsigned char GetCasSample(void)
{
	return(Csample);
}

void SetCassetteSample(unsigned char Sample)
{
	regb[0]=regb[0] & 0xFE;
	if (Sample>0x7F)
		regb[0]=regb[0] | 1;
}

void CaptureBit(unsigned char Sample)
{
	unsigned long BytesMoved=0;
	static unsigned char BitMask=1,StartWait=1;
	static char Byte=0;
	if (hPrintFile==NULL)
		return;
	if (StartWait & Sample)	//Waiting for start bit
		return;
	if (StartWait)
	{
		StartWait=0;
		return;
	}
	if (Sample)
		Byte|=BitMask;
	BitMask=BitMask<<1;
	if (!BitMask)
	{
		BitMask=1;
		StartWait=1;
		AG_WriteP(hPrintFile, &Byte, 1, &BytesMoved);
		if (MonState)
			WritePrintMon(&Byte);
		if ((Byte==0x0D) & AddLF)
		{
			Byte=0x0A;
			AG_WriteP(hPrintFile, &Byte, 1, &BytesMoved);
		}
		Byte=0;
	}
	return;
}

int OpenPrintFile(char *FileName)
{

	hPrintFile=AG_OpenFile(FileName, "w+");
	if (hPrintFile==NULL)
		return(0);
	return(1);
}

void ClosePrintFile(void)
{
	AG_CloseFile(hPrintFile);
	hPrintFile=NULL;
	hout=NULL;
	return;
}

void SetSerialParams(unsigned char TextMode)
{
	AddLF=TextMode;
	return;
}

void SetMonState(BOOL State)
{
	if (MonState & !State)
	{
		hout=NULL;
	}
	MonState=State;
	return;
}
void WritePrintMon(char *Data)
{
	unsigned long dummy;
	if (hout==NULL)
	{
		hout = AG_OpenFileHandle(stderr);
	}
	AG_WriteP(hout, Data, 1, &dummy);
	// if (Data[0]==0x0D)
	// {
	// 	Data[0]=0x0A;
	// 	WriteConsole(hout,Data,1,&dummy,0);
	// }
}

