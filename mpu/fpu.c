// /*
// Copyright 2019 by Walter Zambotti
// This file is part of VCC (Virtual Color Computer).

//     VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.

//     VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
// */
// // 

#include <stdio.h>
#include "mpu.h"

union Data
{
	unsigned long long llval;
	double dval;
	unsigned long lval;
	unsigned char bytes[8];
};

typedef union Data PDP1data;
typedef union Data IEEE754data;

long ReadPDP14bytes(unsigned short int address)
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
	MemWrite8(pdp1data.bytes[3], address);
	MemWrite8(pdp1data.bytes[2], address+1);
	MemWrite8(pdp1data.bytes[1], address+2);
	MemWrite8(pdp1data.bytes[0], address+3);
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
	MemWrite8(PDP1data.bytes[7], address);
	MemWrite8(PDP1data.bytes[6], address+1);
	MemWrite8(PDP1data.bytes[5], address+2);
	MemWrite8(PDP1data.bytes[4], address+3);
	MemWrite8(PDP1data.bytes[3], address+4);
	MemWrite8(PDP1data.bytes[2], address+5);
	MemWrite8(PDP1data.bytes[1], address+6);
	MemWrite8(PDP1data.bytes[0], address+7);
}

double ConvertDBLPDP1toIEEE754(PDP1data PDP1data)
{
	IEEE754data iee754;
	unsigned long long signbit, exp, mantissa;

	signbit  =  PDP1data.llval & 0x8000000000000000;
	exp      = (PDP1data.llval & 0x00000000000000ff) + 0x37e;
	mantissa =  PDP1data.llval & 0x7fffffffffffff00;

	iee754.llval = signbit | (exp<<52) | (mantissa>>11);

	return iee754.dval;
}

PDP1data ConvertDblIEEE754toPDP1(double dvalue)
{
	PDP1data PDP1data;
	IEEE754data IEEE754data;
	unsigned long long signbit, exp, mantissa;

	IEEE754data.dval = dvalue;

	signbit  =   IEEE754data.llval & 0x8000000000000000;
	exp      = ((IEEE754data.llval & 0x7ff0000000000000)>>52) - 0x37e;
	mantissa =   IEEE754data.llval & 0x000fffffffffffff;

	PDP1data.llval = signbit | (exp) | (mantissa<<11);

	return PDP1data;
}

void CompareDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;
	char result = 0;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 - dvalue2;

	if (dvalue < 0)
	{
		result = -1;
	}
	else if (dvalue > 0)
	{
		result = 1;
	}

	// fprintf(stderr, "MPU : CommpareDbl %d = %f - %f\n", result, dvalue1, dvalue2);

	MemWrite8(result, param0);
}

void MultDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 * dvalue2;

	// fprintf(stderr, "MPU : MultDbl %f = %f * %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void DivDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 / dvalue2;

	// fprintf(stderr, "MPU : DivDbl %f = %f / %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void AddDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 + dvalue2;

	// fprintf(stderr, "MPU : AddltDbl %f = %f + %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void SubDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 - dvalue2;

	// fprintf(stderr, "MPU : SubDbl %f = %f - %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void NegDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = -dvalue1;

	// fprintf(stderr, "MPU : NegDbl %f = %f\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void PowDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadPDP18bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = pow(dvalue1, dvalue2);

	// fprintf(stderr, "MPU : PowDbl %f = pow(%f, %f)\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void SqrtDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = sqrt(dvalue1);

	// fprintf(stderr, "MPU : SqrtDbl %f = sqrt(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void ExpDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = exp(dvalue1);

	// fprintf(stderr, "MPU : ExpDbl %f = exp(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void LogDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = log(dvalue1);
	
	// fprintf(stderr, "MPU : LogDbl %f = log(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void Log10Dbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = log10(dvalue1);
	
	// fprintf(stderr, "MPU : Log10Dbl %f = log10(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void InvDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadPDP18bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = 1.0 / dvalue1;
	
	// fprintf(stderr, "MPU : InvDbl %f = inv(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void ltod(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	lvalue = ReadPDP14bytes(param1);
	dvalue = lvalue;

	// fprintf(stderr, "MPU : lotd %f = %d\n", dvalue, lvalue);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WritePDP18bytes(param0, PDP1data);
}

void dtol(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	PDP1data = ReadPDP18bytes(param1);
	dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	lvalue = dvalue;
	PDP1data.lval = lvalue;

	// fprintf(stderr, "MPU : dotl %ld = %lf\n", lvalue, dvalue);

	WritePDP14bytes(param0, PDP1data);
}

void ftod(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	lvalue = ReadPDP14bytes(param1);
	PDP1data.lval = lvalue;
	PDP1data.bytes[7] = PDP1data.bytes[3];
	PDP1data.bytes[6] = PDP1data.bytes[2];
	PDP1data.bytes[5] = PDP1data.bytes[1];
	PDP1data.bytes[4] = 0;
	PDP1data.bytes[3] = 0;
	PDP1data.bytes[2] = 0;

	// dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	// fprintf(stderr, "MPU : ftod %f\n", dvalue);

	WritePDP18bytes(param0, PDP1data);
}

void dtof(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	PDP1data = ReadPDP18bytes(param1);
	// dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	PDP1data.lval = PDP1data.lval;
	PDP1data.bytes[3] = PDP1data.bytes[7];
	PDP1data.bytes[2] = PDP1data.bytes[6];
	PDP1data.bytes[1] = PDP1data.bytes[5];

	
	// fprintf(stderr, "MPU : dtof %f\n", dvalue);

	WritePDP14bytes(param0, PDP1data);
}
