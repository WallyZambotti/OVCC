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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memboard.h"

union
{
	unsigned int Address;
	struct
	{
		unsigned char lswlsb,lswmsb,mswlsb,mswmsb;
	} Byte;
} IndexAddress;//BufferAddress;

static unsigned char *RamBuffer=NULL;

bool InitMemBoard(void)
{
	IndexAddress.Address=0;
	if (RamBuffer!=NULL)
		free(RamBuffer);
	RamBuffer=(unsigned char *)malloc(RAMSIZE);
	if (RamBuffer==NULL)
		return(true);
	memset(RamBuffer,0,RAMSIZE);
	return(false);
}

bool WritePort(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
	case 0x40:
		IndexAddress.Byte.lswlsb=Data;
		break;
	case 0x41:
		IndexAddress.Byte.lswmsb=Data;
		break;
	case 0x42:
		IndexAddress.Byte.mswlsb=(Data & 0x7);
		break;
	}
	return(false);
}

bool WriteArray(unsigned char Data)
{
	RamBuffer[IndexAddress.Address]=Data;
	return(false);
}

unsigned char ReadArray(void)
{
	return(RamBuffer[IndexAddress.Address]);
}



