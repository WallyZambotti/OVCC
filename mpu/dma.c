#include "mpu.h"
#include "gpu.h"

unsigned short int ReadCoCoInt(unsigned short int address)
{
    return MemRead(address)<<8 | MemRead(address+1);
}

void WriteCoCoInt(unsigned short int address, unsigned short int data)
{
	MemWrite(data>>8, address);
	MemWrite(data & 0xff, address+1);
}

int ReadPDP14bytes(unsigned short int address)
{
	PDP1data pdp1data;

	pdp1data.bytes[3] = MemRead(address);
	pdp1data.bytes[2] = MemRead(address+1);
	pdp1data.bytes[1] = MemRead(address+2);
	pdp1data.bytes[0] = MemRead(address+3);

	return pdp1data.lval;
}

void WritePDP14bytes(unsigned short int address, PDP1data pdp1data)
{
	MemWrite(pdp1data.bytes[3], address);
	MemWrite(pdp1data.bytes[2], address+1);
	MemWrite(pdp1data.bytes[1], address+2);
	MemWrite(pdp1data.bytes[0], address+3);
}

PDP1data ReadPDP18bytes(unsigned short int address)
{
	PDP1data PDP1data;

	PDP1data.bytes[7] = MemRead(address);
	PDP1data.bytes[6] = MemRead(address+1);
	PDP1data.bytes[5] = MemRead(address+2);
	PDP1data.bytes[4] = MemRead(address+3);
	PDP1data.bytes[3] = MemRead(address+4);
	PDP1data.bytes[2] = MemRead(address+5);
	PDP1data.bytes[1] = MemRead(address+6);
	PDP1data.bytes[0] = MemRead(address+7);

	return PDP1data;
}

void WritePDP18bytes(unsigned short int address, PDP1data PDP1data)
{
	MemWrite(PDP1data.bytes[7], address);
	MemWrite(PDP1data.bytes[6], address+1);
	MemWrite(PDP1data.bytes[5], address+2);
	MemWrite(PDP1data.bytes[4], address+3);
	MemWrite(PDP1data.bytes[3], address+4);
	MemWrite(PDP1data.bytes[2], address+5);
	MemWrite(PDP1data.bytes[1], address+6);
	MemWrite(PDP1data.bytes[0], address+7);
}