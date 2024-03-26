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
#include <math.h>
#include "mpu.h"
#include "gpu.h"
#include "dma.h"

double ConvertDBLPDP1toIEEE754(PDP1data PDP1data)
{
	IEEE754data iee754;
	unsigned long long signbit, exp, mantissa;

	if (PDP1data.llval == 0) 
	{
		return 0.0;
	}

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

	// IEEE floats can have a negative zero that PDP1 floats cannot have
	// if the value is zero then we make it a good zero

	if (IEEE754data.dval == 0.0)
	{
		IEEE754data.llval = 0;
		return IEEE754data;
	}

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

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
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

	MemWrite(result, param0);
}

void MultDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 * dvalue2;

	//fprintf(stderr, "MPU : MultDbl &p0 = %x, &p1 = %x, &p2 = %x\n", param0, param1, param2);
	//fprintf(stderr, "MPU : MultDbl %f = %f * %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void DivDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 / dvalue2;

	// fprintf(stderr, "MPU : DivDbl %f = %f / %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void AddDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 + dvalue2;

	// fprintf(stderr, "MPU : AddltDbl %f = %f + %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void SubDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = dvalue1 - dvalue2;

	// fprintf(stderr, "MPU : SubDbl %f = %f - %f\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void NegDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue = 0.0, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = -dvalue1;

	// fprintf(stderr, "MPU : NegDbl %f = %f\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void PowDbl(unsigned short param0, unsigned short param1, unsigned short param2)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	PDP1data = ReadCoCo8bytes(param2);
	dvalue2 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = pow(dvalue1, dvalue2);

	// fprintf(stderr, "MPU : PowDbl %f = pow(%f, %f)\n", dvalue, dvalue1, dvalue2);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void SqrtDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = sqrt(dvalue1);

	// fprintf(stderr, "MPU : SqrtDbl %f = sqrt(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void ExpDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = exp(dvalue1);

	// fprintf(stderr, "MPU : ExpDbl %f = exp(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void LogDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = log(dvalue1);
	
	// fprintf(stderr, "MPU : LogDbl %f = log(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void Log10Dbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = log10(dvalue1);
	
	// fprintf(stderr, "MPU : Log10Dbl %f = log10(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void InvDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = 1.0 / dvalue1;
	
	// fprintf(stderr, "MPU : InvDbl %f = inv(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void SinDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = sin(dvalue1);
	
	// fprintf(stderr, "MPU : SinDbl %f = sin(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void CosDbl(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue, dvalue1, dvalue2;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue1 = ConvertDBLPDP1toIEEE754(PDP1data);

	dvalue = cos(dvalue1);
	
	// fprintf(stderr, "MPU : CosDbl %f = cos(%f)\n", dvalue, dvalue1);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void ltod(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	int lvalue;
	double dvalue;

	lvalue = ReadCoCo4byte(param1);
	dvalue = (double)lvalue;

	// fprintf(stderr, "MPU : lotd %f = %d\n", dvalue, lvalue);

	PDP1data = ConvertDblIEEE754toPDP1(dvalue);
	WriteCoCo8bytes(param0, PDP1data);
}

void dtol(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	double dvalue;

	PDP1data = ReadCoCo8bytes(param1);
	dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	PDP1data.lval = dvalue;

	// fprintf(stderr, "MPU : dotl %ld = %lf\n", PDP1data.lval, dvalue);

	WriteCoCo4bytes(param0, PDP1data);
}

void ftod(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	lvalue = ReadCoCo4byte(param1);
	PDP1data.lval = lvalue;
	PDP1data.bytes[7] = PDP1data.bytes[3];
	PDP1data.bytes[6] = PDP1data.bytes[2];
	PDP1data.bytes[5] = PDP1data.bytes[1];
	PDP1data.bytes[4] = 0;
	PDP1data.bytes[3] = 0;
	PDP1data.bytes[2] = 0;

	// dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	// fprintf(stderr, "MPU : ftod %f\n", dvalue);

	WriteCoCo8bytes(param0, PDP1data);
}

void dtof(unsigned short param0, unsigned short param1)
{
	PDP1data PDP1data;
	long lvalue;
	double dvalue;

	PDP1data = ReadCoCo8bytes(param1);
	// dvalue = ConvertDBLPDP1toIEEE754(PDP1data);
	PDP1data.lval = PDP1data.lval;
	PDP1data.bytes[3] = PDP1data.bytes[7];
	PDP1data.bytes[2] = PDP1data.bytes[6];
	PDP1data.bytes[1] = PDP1data.bytes[5];

	
	// fprintf(stderr, "MPU : dtof %f\n", dvalue);

	WriteCoCo4bytes(param0, PDP1data);
}
