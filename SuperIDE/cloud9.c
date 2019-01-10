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
/************************************************************************
*	Basically simulates the Dallas DS1315 Real Time Clock				*
*																		*
*																		*
*																		*
*																		*
*																		*
************************************************************************/

#include <time.h>
#include <stdint.h>
#include "cloud9.h"

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

static time_t rawtime;
static struct timespec tspec;
static struct tm *now;
static unsigned char time_register=0;
static uint64_t InBuffer=0;
static uint64_t OutBuffer=0;
static uint64_t CurrentBit=0;
static unsigned char BitCounter=0;
static unsigned char TempHour=0;
static unsigned char AmPmBit=0;
static unsigned char FormatBit=0; //1 = 12Hour Mode
static unsigned char CookieRecived=0;
static unsigned char WriteEnabled=0;
void SetTime(void);

unsigned char ReadTime(unsigned short port)
{
	unsigned char ret_val=0;

	//FF78=0 FF79=1 FF7C talkback
	switch (port)
	{
	case 0x78:
	case 0x79:
		ret_val=0;
		CurrentBit=port &1;
		InBuffer=InBuffer>>1;
		InBuffer= InBuffer | (CurrentBit<<63);
		if (CookieRecived==1)
		{
			if (BitCounter==0)
			{
				SetTime();
				CookieRecived=0;
			}
			BitCounter--;
		}
		if (InBuffer==0x5CA33AC55CA33AC5) 
		{
			clock_gettime(CLOCK_REALTIME, &tspec);
			rawtime = tspec.tv_sec;
			now = localtime(&rawtime);

			OutBuffer=0;
			OutBuffer=((now->tm_year%100)/10)+10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_year%10;
			OutBuffer<<=4;
			OutBuffer|=(now->tm_mon+1)/10;
			OutBuffer<<=4;
			OutBuffer|=(now->tm_mon+1)%10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_mday/10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_mday%10;
			OutBuffer<<=4;
			//Skip Osc and Reset
			OutBuffer<<=4;
			OutBuffer|=now->tm_wday;
			OutBuffer<<=4;
			TempHour=(unsigned char)now->tm_hour;
			AmPmBit=0;
			if ((FormatBit==1) & (TempHour>12)) //1=12 hour mode 1=PM
			{
				TempHour-=12;
				AmPmBit=2;
			}

			OutBuffer|= (3 & ((TempHour/10) | AmPmBit));
			OutBuffer<<=4;

			OutBuffer|=TempHour%10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_min/10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_min%10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_sec/10;
			OutBuffer<<=4;
			OutBuffer|=now->tm_sec%10;
			OutBuffer<<=4;

			long ms = tspec.tv_nsec / 1000000;
			OutBuffer|=ms/10;
			OutBuffer<<=4;
			OutBuffer|=ms%10;
			InBuffer=0;
			BitCounter=63; //Flag indicating the cookie was recived
			CookieRecived=1;
		}
		break;

	case 0x7C:
		ret_val= (unsigned char)OutBuffer & 1;
		
		if (BitCounter>0)
		{
			OutBuffer = (OutBuffer>>1);
			BitCounter--;
		}
		else
			CookieRecived=0;
		break;
	} //End of port switch


	return(ret_val);
}


void SetTime(void)
{
	tspec.tv_nsec = (InBuffer & 15);
	InBuffer>>=4;
	tspec.tv_nsec += ((InBuffer & 15)*10);
	tspec.tv_nsec *= 1000000;
	InBuffer>>=4;
	now->tm_sec = (unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	now->tm_sec += (unsigned short)((InBuffer & 15)*10);
	InBuffer>>=4;
	now->tm_min = (unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	now->tm_min += (unsigned short)((InBuffer & 15)*10);
	InBuffer>>=4;
	now->tm_hour =(unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	TempHour=(unsigned char)(InBuffer & 15);	//Here fix me

	FormatBit= (TempHour & 8)>>3;	//1 = 12Hour Mode
	if (FormatBit==1)
	{
		AmPmBit=(TempHour & 2);
		now->tm_hour+=(TempHour &1) *10;	//12 Hour Mode
		if (AmPmBit==2)
			now->tm_hour+=12;				//convert to 24hour clock
	}
	else
		now->tm_hour+=(TempHour &3)*10;	//24 Hour Mode

	InBuffer>>=4;
	now->tm_wday=(unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	InBuffer>>=4;	//Skip OSC and RESET
	now->tm_mday=(unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	now->tm_mday+=(unsigned short)((InBuffer & 15)*10);
	InBuffer>>=4;	
	now->tm_mon=(unsigned short)((InBuffer & 15)-1);
	InBuffer>>=4;
	now->tm_mon+=(unsigned short)(((InBuffer & 15)-1)*10);
	InBuffer>>=4;
	now->tm_year=(unsigned short)(InBuffer & 15);
	InBuffer>>=4;
	now->tm_year+=(unsigned short)((InBuffer & 15)*10);
	now->tm_year+=1900;
	if (WriteEnabled)
		//SetLocalTime(&now); //Allow the emulator to fuck with host's time, 
	return;
}


unsigned char SetClockWrite(unsigned char Flag)
{
	if (Flag==0xFF)
		return(WriteEnabled);
	WriteEnabled=Flag&1;
	return(WriteEnabled);
}












