#include <stdio.h>
#include "tcc1014mmu.h"

/*****************************************************************
* 16/32 bit memory handling routines                                *
*****************************************************************/

unsigned short MemRead16(unsigned short addr)
{
	return (MemRead8(addr)<<8 | MemRead8(addr+1));
}

void MemWrite16(unsigned short data,unsigned short addr)
{
	MemWrite8( data >>8,addr);
	MemWrite8( data & 0xFF,addr+1);
	return;
}

unsigned int MemRead32(unsigned short Address)
{
	return ( (MemRead16(Address)<<16) | MemRead16(Address+2) );
}

void MemWrite32(unsigned int data,unsigned short Address)
{
	MemWrite16( data>>16,Address);
	MemWrite16( data & 0xFFFF,Address+2);
	return;
}
