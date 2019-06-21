#include "mpu.h"
#include "gpu.h"
#include "dma.h"

unsigned short int ReadCoCoInt(unsigned short int address)
{
    return MemRead(address)<<8 | MemRead(address+1);
}

void WriteCoCoInt(unsigned short int address, unsigned short int data)
{
	MemWrite(data>>8, address);
	MemWrite(data & 0xff, address+1);
}

int ReadCoCo4byte(unsigned short int address)
{
	Data cocodata;

	cocodata.bytes[3] = MemRead(address);
	cocodata.bytes[2] = MemRead(address+1);
	cocodata.bytes[1] = MemRead(address+2);
	cocodata.bytes[0] = MemRead(address+3);

	return cocodata.lval;
}

void WriteCoCo4bytes(unsigned short int address, Data cocodata)
{
	MemWrite(cocodata.bytes[3], address);
	MemWrite(cocodata.bytes[2], address+1);
	MemWrite(cocodata.bytes[1], address+2);
	MemWrite(cocodata.bytes[0], address+3);
}

Data ReadCoCo8bytes(unsigned short int address)
{
	Data cocodata;

	cocodata.bytes[7] = MemRead(address);
	cocodata.bytes[6] = MemRead(address+1);
	cocodata.bytes[5] = MemRead(address+2);
	cocodata.bytes[4] = MemRead(address+3);
	cocodata.bytes[3] = MemRead(address+4);
	cocodata.bytes[2] = MemRead(address+5);
	cocodata.bytes[1] = MemRead(address+6);
	cocodata.bytes[0] = MemRead(address+7);

	return cocodata;
}

void WriteCoCo8bytes(unsigned short int address, Data cocodata)
{
	MemWrite(cocodata.bytes[7], address);
	MemWrite(cocodata.bytes[6], address+1);
	MemWrite(cocodata.bytes[5], address+2);
	MemWrite(cocodata.bytes[4], address+3);
	MemWrite(cocodata.bytes[3], address+4);
	MemWrite(cocodata.bytes[2], address+5);
	MemWrite(cocodata.bytes[1], address+6);
	MemWrite(cocodata.bytes[0], address+7);
}