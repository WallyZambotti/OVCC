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
#include "hd6309.h"
#include "hd6309defs.h"
#include "tcc1014mmu.h"
#include "hd6309turbo.h"
#include "logger.h"

#if defined(_WIN64)
#define MSABI 
#else
#define MSABI __attribute__((ms_abi))
#endif

//Replace md[] with mdbits_s
#define MD_NATIVE6309_BIT 0x01
#define MD_FIRQMODE_BIT 0x02
#define MD_ILLEGALINST_BIT 0x40
#define MD_DIVBYZERO_BIT 0x80

#define MD_NATIVE6309 (mdbits_s & MD_NATIVE6309_BIT)
#define MD_FIRQMODE (mdbits_s & MD_FIRQMODE_BIT)
#define MD_ILLEGALINST (mdbits_s & MD_ILLEGALINST_BIT)
#define MD_DIVBYZERO (mdbits_s & MD_DIVBYZERO_BIT)


//Global variables for CPU Emulation-----------------------
#define NTEST8(r) r>0x7F;
#define NTEST16(r) r>0x7FFF;
#define NTEST32(r) r>0x7FFFFFFF;
#define OVERFLOW8(c,a,b,r) c ^ (((a^b^r)>>7) &1);
#define OVERFLOW16(c,a,b,r) c ^ (((a^b^r)>>15)&1);
#define ZTEST(r) !r;

#define DPADDRESS(r) (dp_s.Reg |MemRead8(r))
#define IMMADDRESS(r) MemRead16(r)
#define INDADDRESS(r) CalculateEA(MemRead8(r))

#define M65		0
#define M64		1
#define M32		2
#define M21		3
#define M54		4
#define M97		5
#define M85		6
#define M51		7
#define M31		8
#define M1110	9
#define M76		10
#define M75		11
#define M43		12
#define M87		13
#define M86		14
#define M98		15
#define M2726	16
#define M3635	17
#define M3029	18
#define M2827	19
#define M3726	20
#define M3130	21

typedef union
{
	unsigned short Reg;
	struct
	{
		unsigned char lsb,msb;
	} B;
} cpuregister;

typedef union
{
	unsigned int Reg;
	struct
	{
		unsigned short msw,lsw;
	} Word;
	struct
	{
		unsigned char mswlsb,mswmsb,lswlsb,lswmsb;	//Might be backwards
	} Byte;
} wideregister;

#define D_REG	q_s.Word.lsw
#define W_REG	q_s.Word.msw
#define PC_REG	pc_s.Reg
#define X_REG	x_s.Reg
#define Y_REG	y_s.Reg
#define U_REG	u_s.Reg
#define S_REG	s_s.Reg
#define A_REG	q_s.Byte.lswmsb
#define B_REG	q_s.Byte.lswlsb
#define E_REG	q_s.Byte.mswmsb
#define F_REG	q_s.Byte.mswlsb	
#define Q_REG	q_s.Reg
#define V_REG	v_s.Reg
#define O_REG	z_s.Reg
static char RegName[16][10]={"D","X","Y","U","S","PC","W","V","A","B","CC","DP","ZERO","ZERO","E","F"};

wideregister q_s;
cpuregister pc_s, x_s, y_s, u_s, s_s, dp_s, v_s, z_s;
static unsigned char InsCycles[2][25];
unsigned char cc_s[8];
static unsigned int md[8];
unsigned char *ureg8_s[8]; 
unsigned char ccbits_s,mdbits_s;
unsigned short *xfreg16_s[8];
int CycleCounter=0;
unsigned int SyncWaiting_s=0;
unsigned short temp16;
static signed short stemp16;
static signed char stemp8;
static unsigned int  temp32;
static int stemp32;
static unsigned char temp8; 
static unsigned char PendingInterupts=0;
static unsigned char IRQWaiter=0;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
char InInterupt_s=0;
int gCycleFor;
static short *instcycl1, *instcycl2, *instcycl3;
static unsigned long instcnt1[256], instcnt2[256], instcnt3[0];
//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
unsigned int MemRead32_s(unsigned short);
void MemWrite32_s(unsigned int, unsigned short);
static unsigned short CalculateEA(unsigned char);
void InvalidInsHandler_s(void);
void DivbyZero_s(void);
static void ErrorVector(void);
static void setcc (unsigned char);
static unsigned char getcc(void);
void setmd_s (unsigned char);
static unsigned char getmd(void);
static void cpu_firq(void);
static void cpu_irq(void);
static void cpu_nmi(void);
static unsigned char GetSorceReg(unsigned char);
static void Page_2(void);
static void Page_3(void);
extern void MemWrite8(unsigned char, unsigned short);
extern void MemWrite16(unsigned short, unsigned short);
extern unsigned char MemRead8(unsigned short);
extern unsigned short MemRead16(unsigned short);

//unsigned char GetDestReg(unsigned char);
//END Fuction Prototypes-----------------------------------

void HD6309Reset_s(void)
{
	char index;
	for(index=0;index<=6;index++)		//Set all register to 0 except V
		*xfreg16_s[index] = 0;
	for(index=0;index<=7;index++)
		*ureg8_s[index]=0;
	for(index=0;index<=7;index++)
		cc_s[index]=0;
	//for(index=0;index<=7;index++)
	//	md[index]=0;
	mdbits_s=0;
	//mdbits_s=getmd();
	setmd_s(mdbits_s);
	dp_s.Reg=0;
	cc_s[I]=1;
	cc_s[F]=1;
	SyncWaiting_s=0;
	PC_REG=MemRead16(VRESET);	//PC gets its reset vector
	SetMapType(0);	//shouldn't be here
	for (int i = 0 ; i < 256 ; i++)
	{
		// printf("%02x 0:%ld 1:%ld 2:%ld\n", i, instcnt1[i], instcnt2[i], instcnt3[i]);
		instcnt1[i] = 0;
		instcnt2[i] = 0;
		instcnt3[i] = 0;
	}
	return;
}

void HD6309Init_s(void)
{	//Call this first or RESET will core!
	// reg pointers for TFR and EXG and LEA ops
	xfreg16_s[0] = &D_REG;
	xfreg16_s[1] = &X_REG;
	xfreg16_s[2] = &Y_REG;
	xfreg16_s[3] = &U_REG;
	xfreg16_s[4] = &S_REG;
	xfreg16_s[5] = &PC_REG;
	xfreg16_s[6] = &W_REG;
	xfreg16_s[7] = &V_REG;

	ureg8_s[0]=(unsigned char*)&A_REG;		
	ureg8_s[1]=(unsigned char*)&B_REG;		
	ureg8_s[2]=(unsigned char*)&ccbits_s;
	ureg8_s[3]=(unsigned char*)&dp_s.B.msb;
	ureg8_s[4]=(unsigned char*)&O_REG;
	ureg8_s[5]=(unsigned char*)&O_REG;
	ureg8_s[6]=(unsigned char*)&E_REG;
	ureg8_s[7]=(unsigned char*)&F_REG;

	//This handles the disparity between 6309 and 6809 Instruction timing
	InsCycles[0][M65]=6;	//6-5
	InsCycles[1][M65]=5;
	InsCycles[0][M64]=6;	//6-4
	InsCycles[1][M64]=4;
	InsCycles[0][M32]=3;	//3-2
	InsCycles[1][M32]=2;
	InsCycles[0][M21]=2;	//2-1
	InsCycles[1][M21]=1;
	InsCycles[0][M54]=5;	//5-4
	InsCycles[1][M54]=4;
	InsCycles[0][M97]=9;	//9-7
	InsCycles[1][M97]=7;
	InsCycles[0][M85]=8;	//8-5
	InsCycles[1][M85]=5;
	InsCycles[0][M51]=5;	//5-1
	InsCycles[1][M51]=1;
	InsCycles[0][M31]=3;	//3-1
	InsCycles[1][M31]=1;
	InsCycles[0][M1110]=11;	//11-10
	InsCycles[1][M1110]=10;
	InsCycles[0][M76]=7;	//7-6
	InsCycles[1][M76]=6;
	InsCycles[0][M75]=7;	//7-5
	InsCycles[1][M75]=5;
	InsCycles[0][M43]=4;	//4-3
	InsCycles[1][M43]=3;
	InsCycles[0][M87]=8;	//8-7
	InsCycles[1][M87]=7;
	InsCycles[0][M86]=8;	//8-6
	InsCycles[1][M86]=6;
	InsCycles[0][M98]=9;	//9-8
	InsCycles[1][M98]=8;
	InsCycles[0][M2726]=27;	//27-26
	InsCycles[1][M2726]=26;
	InsCycles[0][M3635]=36;	//36-25
	InsCycles[1][M3635]=35;	
	InsCycles[0][M3029]=30;	//30-29
	InsCycles[1][M3029]=29;	
	InsCycles[0][M2827]=28;	//28-27
	InsCycles[1][M2827]=27;	
	InsCycles[0][M3726]=37;	//37-26
	InsCycles[1][M3726]=26;		
	InsCycles[0][M3130]=31;	//31-30
	InsCycles[1][M3130]=30;		
	cc_s[I]=1;
	cc_s[F]=1;
	return;
}

static void Neg_D(void)
{ //0
	temp16 = DPADDRESS(PC_REG++);
	postbyte = MemRead8(temp16);
	temp8 = 0 - postbyte;
	cc_s[C] = temp8 > 0;
	cc_s[V] = (postbyte == 0x80);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8, temp16);
	CycleCounter += InsCycles[MD_NATIVE6309][M65];
}

static void Oim_D(void)
{//1 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Aim_D(void)
{//2 Phase 2 6309
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Com_D(void)
{ //03
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8); 
	cc_s[C] = 1;
	cc_s[V] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Lsr_D(void)
{ //04 S2
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8 = temp8 >>1;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Eim_D(void)
{ //05 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Ror_D(void)
{ //06 S2
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte= cc_s[C]<<7;
	cc_s[C] = temp8 & 1;
	temp8 = (temp8 >> 1)| postbyte;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Asr_D(void)
{ //7
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8 = (temp8 & 0x80) | (temp8 >>1);
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Asl_D(void)
{ //8 
	temp16 = DPADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc_s[C] = (temp8 & 0x80) >>7;
	cc_s[V] = cc_s[C] ^ ((temp8 & 0x40) >> 6);
	temp8 = temp8 <<1;
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Rol_D(void)
{	//9
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	postbyte=cc_s[C];
	cc_s[C] =(temp8 & 0x80)>>7;
	cc_s[V] = cc_s[C] ^ ((temp8 & 0x40) >>6);
	temp8 = (temp8<<1) | postbyte;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Dec_D(void)
{ //A
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16)-1;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Tim_D(void)
{	//B 6309 Untested wcreate
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(DPADDRESS(PC_REG++));
	postbyte&=temp8;
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Inc_D(void)
{ //C
	temp16=(DPADDRESS(PC_REG++));
	temp8 = MemRead8(temp16)+1;
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = temp8==0x80;
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Tst_D(void)
{ //D
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M64];
}

static void Jmp_D(void)
{	//E
	PC_REG= ((dp_s.Reg |MemRead8(PC_REG)));
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Clr_D(void)
{	//F
	MemWrite8(0,DPADDRESS(PC_REG++));
	cc_s[Z] = 1;
	cc_s[N] = 0;
	cc_s[V] = 0;
	cc_s[C] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void LBeq_R(void)
{ //1027
	if (cc_s[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBrn_R(void)
{ //1021
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBhi_R(void)
{ //1022
	if  (!(cc_s[C] | cc_s[Z]))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBls_R(void)
{ //1023
	if (cc_s[C] | cc_s[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBhs_R(void)
{ //1024
	if (!cc_s[C])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=6;
}

static void LBcs_R(void)
{ //1025
	if (cc_s[C])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBne_R(void)
{ //1026
	if (!cc_s[Z])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBvc_R(void)
{ //1028
	if ( !cc_s[V])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBvs_R(void)
{ //1029
	if ( cc_s[V])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBpl_R(void)
{ //102A
if (!cc_s[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBmi_R(void)
{ //102B
if ( cc_s[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBge_R(void)
{ //102C
	if (! (cc_s[N] ^ cc_s[V]))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBlt_R(void)
{ //102D
	if ( cc_s[V] ^ cc_s[N])
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBgt_R(void)
{ //102E
	if ( !( cc_s[Z] | (cc_s[N]^cc_s[V] ) ))
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void LBle_R(void)
{	//102F
	if ( cc_s[Z] | (cc_s[N]^cc_s[V]) )
	{
		*spostword=IMMADDRESS(PC_REG);
		PC_REG+=*spostword;
		CycleCounter+=1;
	}
	PC_REG+=2;
	CycleCounter+=5;
}

static void Addr(void)
{ //1030 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2)
				source8 = getcc();
			else 
				source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp16 = source8 + dest8;
		if (Dest == 2) 
			setcc((unsigned char)temp16);
		*ureg8_s[Dest] = (unsigned char)temp16;
		cc_s[C] = (temp16 & 0x100) >> 8;
		cc_s[V] = OVERFLOW8(cc_s[C], source8, dest8, temp16);
		cc_s[N] = NTEST8(*ureg8_s[Dest]);
		cc_s[Z] = ZTEST(*ureg8_s[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = source16 + dest16;
		*xfreg16_s[Dest] = (unsigned short)temp32;
		cc_s[C] = (temp32 & 0x10000) >> 16;
		cc_s[V] = OVERFLOW16(cc_s[C], source16, dest16, temp32);
		cc_s[N] = NTEST16(*xfreg16_s[Dest]);
		cc_s[Z] = ZTEST(*xfreg16_s[Dest]);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	CycleCounter += 4;
}

static void Adcr(void)
{ //1031 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp16 = source8 + dest8 + cc_s[C];
		if (Dest == 2) setcc((unsigned char)temp16);
		*ureg8_s[Dest] = (unsigned char)temp16;
		cc_s[C] = (temp16 & 0x100) >> 8;
		cc_s[V] = OVERFLOW8(cc_s[C], source8, dest8, temp16);
		cc_s[N] = NTEST8(*ureg8_s[Dest]);
		cc_s[Z] = ZTEST(*ureg8_s[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = source16 + dest16 + cc_s[C];
		*xfreg16_s[Dest] = (unsigned short)temp32;
		cc_s[C] = (temp32 & 0x10000) >> 16;
		cc_s[V] = OVERFLOW16(cc_s[C], source16, dest16, temp32);
		cc_s[N] = NTEST16(*xfreg16_s[Dest]);
		cc_s[Z] = ZTEST(*xfreg16_s[Dest]);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	CycleCounter += 4;
}

static void Subr(void)
{ //1032 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp16 = dest8 - source8;
		if (Dest == 2) setcc((unsigned char)temp16);
		*ureg8_s[Dest] = (unsigned char)temp16;
		cc_s[C] = (temp16 & 0x100) >> 8;
		cc_s[V] = cc_s[C] ^ ((dest8 ^ *ureg8_s[Dest] ^ source8) >> 7);
		cc_s[N] = *ureg8_s[Dest] >> 7;
		cc_s[Z] = ZTEST(*ureg8_s[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16;
		cc_s[C] = (temp32 & 0x10000) >> 16;
		cc_s[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		*xfreg16_s[Dest] = (unsigned short)temp32;
		cc_s[N] = (temp32 & 0x8000) >> 15;
		cc_s[Z] = ZTEST(temp32);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	CycleCounter += 4;
}

static void Sbcr(void)
{ //1033 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp16 = dest8 - source8 - cc_s[C];
		if (Dest == 2) setcc((unsigned char)temp16);
		*ureg8_s[Dest] = (unsigned char)temp16;
		cc_s[C] = (temp16 & 0x100) >> 8;
		cc_s[V] = cc_s[C] ^ ((dest8 ^ *ureg8_s[Dest] ^ source8) >> 7);
		cc_s[N] = *ureg8_s[Dest] >> 7;
		cc_s[Z] = ZTEST(*ureg8_s[Dest]);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16 - cc_s[C];
		cc_s[C] = (temp32 & 0x10000) >> 16;
		cc_s[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		*xfreg16_s[Dest] = (unsigned short)temp32;
		cc_s[N] = (temp32 & 0x8000) >> 15;
		cc_s[Z] = ZTEST(temp32);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	CycleCounter += 4;
}

static void Andr(void)
{ //1034 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp8 = dest8 & source8;
		if (Dest == 2) setcc((unsigned char)temp8);
		else *ureg8_s[Dest] = temp8;
		cc_s[N] = temp8 >> 7;
		cc_s[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 & source16;
		*xfreg16_s[Dest] = temp16;
		cc_s[N] = temp16 >> 15;
		cc_s[Z] = ZTEST(temp16);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	cc_s[V] = 0;
	CycleCounter += 4;
}

static void Orr(void)
{ //1035 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp8 = dest8 | source8;
		if (Dest == 2) setcc((unsigned char)temp8);
		else *ureg8_s[Dest] = temp8;
		cc_s[N] = temp8 >> 7;
		cc_s[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 | source16;
		*xfreg16_s[Dest] = temp16;
		cc_s[N] = temp16 >> 15;
		cc_s[Z] = ZTEST(temp16);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	cc_s[V] = 0;
	CycleCounter += 4;
}

static void Eorr(void)
{ //1036 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp8 = dest8 ^ source8;
		if (Dest == 2) setcc((unsigned char)temp8);
		else *ureg8_s[Dest] = temp8;
		cc_s[N] = temp8 >> 7;
		cc_s[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp16 = dest16 ^ source16;
		*xfreg16_s[Dest] = temp16;
		cc_s[N] = temp16 >> 15;
		cc_s[Z] = ZTEST(temp16);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	cc_s[V] = 0;
	CycleCounter += 4;
}

static void Cmpr(void)
{ //1037 6309 - WallyZ 2019
	unsigned char dest8, source8;
	unsigned short dest16, source16;
	temp8 = MemRead8(PC_REG++);
	Source = temp8 >> 4;
	Dest = temp8 & 15;

	if (Dest > 7) // 8 bit dest
	{
		Dest &= 7;
		if (Dest == 2) dest8 = getcc();
		else dest8 = *ureg8_s[Dest];

		if (Source > 7) // 8 bit source
		{
			Source &= 7;
			if (Source == 2) source8 = getcc();
			else source8 = *ureg8_s[Source];
		}
		else // 16 bit source - demote to 8 bit
		{
			Source &= 7;
			source8 = (unsigned char)*xfreg16_s[Source];
		}

		temp16 = dest8 - source8;
		temp8 = (unsigned char)temp16;
		cc_s[C] = (temp16 & 0x100) >> 8;
		cc_s[V] = cc_s[C] ^ ((dest8 ^ temp8 ^ source8) >> 7);
		cc_s[N] = temp8 >> 7;
		cc_s[Z] = ZTEST(temp8);
	}
	else // 16 bit dest
	{
		dest16 = *xfreg16_s[Dest];

		if (Source < 8) // 16 bit source
		{
			source16 = *xfreg16_s[Source];
		}
		else // 8 bit source - promote to 16 bit
		{
			Source &= 7;
			switch (Source)
			{
			case 0: case 1: source16 = D_REG; break; // A & B Reg
			case 2:	        source16 = (unsigned short)getcc(); break; // CC
			case 3:	        source16 = (unsigned short)dp_s.Reg; break; // DP
			case 4: case 5: source16 = 0; break; // Zero Reg
			case 6: case 7: source16 = W_REG; break; // E & F Reg
			}
		}

		temp32 = dest16 - source16;
		cc_s[C] = (temp32 & 0x10000) >> 16;
		cc_s[V] = !!((dest16 ^ source16 ^ temp32 ^ (temp32 >> 1)) & 0x8000);
		cc_s[N] = (temp32 & 0x8000) >> 15;
		cc_s[Z] = ZTEST(temp32);
		O_REG = 0; // In case the Dest is the zero reg which can never be changed
	}
	CycleCounter += 4;
}

static void Pshsw(void)
{ //1038 DONE 6309
	MemWrite8((F_REG),--S_REG);
	MemWrite8((E_REG),--S_REG);
	CycleCounter+=6;
}

static void Pulsw(void)
{	//1039 6309 Untested wcreate
	E_REG=MemRead8( S_REG++);
	F_REG=MemRead8( S_REG++);
	CycleCounter+=6;
}

static void Pshuw(void)
{ //103A 6309 Untested
	MemWrite8((F_REG),--U_REG);
	MemWrite8((E_REG),--U_REG);
	CycleCounter+=6;
}

static void Puluw(void)
{ //103B 6309 Untested
	E_REG=MemRead8( U_REG++);
	F_REG=MemRead8( U_REG++);
	CycleCounter+=6;
}

static void Swi2_I(void)
{ //103F
	cc_s[E]=1;
	MemWrite8( pc_s.B.lsb,--S_REG);
	MemWrite8( pc_s.B.msb,--S_REG);
	MemWrite8( u_s.B.lsb,--S_REG);
	MemWrite8( u_s.B.msb,--S_REG);
	MemWrite8( y_s.B.lsb,--S_REG);
	MemWrite8( y_s.B.msb,--S_REG);
	MemWrite8( x_s.B.lsb,--S_REG);
	MemWrite8( x_s.B.msb,--S_REG);
	MemWrite8( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI2);
	CycleCounter+=20;
}

static void Negd_I(void)
{ //1040 Phase 5 6309
	temp16= 0-D_REG;
	cc_s[C] = temp16>0;
	cc_s[V] = D_REG==0x8000;
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	D_REG= temp16;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Comd_I(void)
{ //1043 6309
	D_REG = 0xFFFF- D_REG;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Lsrd_I(void)
{ //1044 6309
	cc_s[C] = D_REG & 1;
	D_REG = D_REG>>1;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Rord_I(void)
{ //1046 6309 Untested
	postword=cc_s[C]<<15;
	cc_s[C] = D_REG & 1;
	D_REG = (D_REG>>1) | postword;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Asrd_I(void)
{ //1047 6309 Untested TESTED NITRO MULTIVUE
	cc_s[C] = D_REG & 1;
	D_REG = (D_REG & 0x8000) | (D_REG >> 1);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Asld_I(void)
{ //1048 6309
	cc_s[C] = D_REG >>15;
	cc_s[V] =  cc_s[C] ^((D_REG & 0x4000)>>14);
	D_REG = D_REG<<1;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Rold_I(void)
{ //1049 6309 Untested
	postword=cc_s[C];
	cc_s[C] = D_REG >>15;
	cc_s[V] = cc_s[C] ^ ((D_REG & 0x4000)>>14);
	D_REG= (D_REG<<1) | postword;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Decd_I(void)
{ //104A 6309
	D_REG--;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = D_REG==0x7FFF;
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Incd_I(void)
{ //104C 6309
	D_REG++;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = D_REG==0x8000;
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Tstd_I(void)
{ //104D 6309
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Clrd_I(void)
{ //104F 6309
	D_REG= 0;
	cc_s[C] = 0;
	cc_s[V] = 0;
	cc_s[N] = 0;
	cc_s[Z] = 1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Comw_I(void)
{ //1053 6309 Untested
	W_REG= 0xFFFF- W_REG;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Lsrw_I(void)
{ //1054 6309 Untested
	cc_s[C] = W_REG & 1;
	W_REG= W_REG>>1;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Rorw_I(void)
{ //1056 6309 Untested
	postword=cc_s[C]<<15;
	cc_s[C] = W_REG & 1;
	W_REG= (W_REG>>1) | postword;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Rolw_I(void)
{ //1059 6309
	postword=cc_s[C];
	cc_s[C] = W_REG >>15;
	cc_s[V] = cc_s[C] ^ ((W_REG & 0x4000)>>14);
	W_REG= ( W_REG<<1) | postword;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Decw_I(void)
{ //105A 6309
	W_REG--;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[V] = W_REG==0x7FFF;
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Incw_I(void)
{ //105C 6309
	W_REG++;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[V] = W_REG==0x8000;
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Tstw_I(void)
{ //105D Untested 6309 wcreate
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;	
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Clrw_I(void)
{ //105F 6309
	W_REG = 0;
	cc_s[C] = 0;
	cc_s[V] = 0;
	cc_s[N] = 0;
	cc_s[Z] = 1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Subw_M(void)
{ //1080 6309 CHECK
	postword=IMMADDRESS(PC_REG);
	temp16= W_REG-postword;
	cc_s[C] = temp16 > W_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],temp16,W_REG,postword);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	W_REG= temp16;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpw_M(void)
{ //1081 6309 CHECK
	postword=IMMADDRESS(PC_REG);
	temp16= W_REG-postword;
	cc_s[C] = temp16 > W_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],temp16,W_REG,postword);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Sbcd_M(void)
{ //1082 P6309
	postword=IMMADDRESS(PC_REG);
	temp32=D_REG-postword-cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,D_REG,postword);
	D_REG= temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpd_M(void)
{ //1083
	postword=IMMADDRESS(PC_REG);
	temp16 = D_REG-postword;
	cc_s[C] = temp16 > D_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,D_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Andd_M(void)
{ //1084 6309
	D_REG &= IMMADDRESS(PC_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Bitd_M(void)
{ //1085 6309 Untested
	temp16= D_REG & IMMADDRESS(PC_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ldw_M(void)
{ //1086 6309
	W_REG=IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Eord_M(void)
{ //1088 6309 Untested
	D_REG ^= IMMADDRESS(PC_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Adcd_M(void)
{ //1089 6309
	postword=IMMADDRESS(PC_REG);
	temp32= D_REG + postword + cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp32,D_REG);
	cc_s[H] = ((D_REG ^ temp32 ^ postword) & 0x100)>>8;
	D_REG = temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ord_M(void)
{ //108A 6309 Untested
	D_REG |= IMMADDRESS(PC_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Addw_M(void)
{ //108B Phase 5 6309
	temp16=IMMADDRESS(PC_REG);
	temp32= W_REG+ temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpy_M(void)
{ //108C
	postword=IMMADDRESS(PC_REG);
	temp16 = Y_REG-postword;
	cc_s[C] = temp16 > Y_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,Y_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}
	
static void Ldy_M(void)
{ //108E
	Y_REG = IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Subw_D(void)
{ //1090 Untested 6309
	temp16= MemRead16(DPADDRESS(PC_REG++));
	temp32= W_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Cmpw_D(void)
{ //1091 6309 Untested
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= W_REG - postword ;
	cc_s[C] = temp16 > W_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,W_REG); 
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Sbcd_D(void)
{ //1092 6309
	postword= MemRead16(DPADDRESS(PC_REG++));
	temp32=D_REG-postword-cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,D_REG,postword);
	D_REG= temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Cmpd_D(void)
{ //1093
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= D_REG - postword ;
	cc_s[C] = temp16 > D_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,D_REG); 
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Andd_D(void)
{ //1094 6309 Untested
	postword=MemRead16(DPADDRESS(PC_REG++));
	D_REG&=postword;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Bitd_D(void)
{ //1095 6309 Untested
	temp16= D_REG & MemRead16(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Ldw_D(void)
{ //1096 6309
	W_REG = MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Stw_D(void)
{ //1097 6309
	MemWrite16(W_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Eord_D(void)
{ //1098 6309 Untested
	D_REG^=MemRead16(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Adcd_D(void)
{ //1099 6309
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp32= D_REG + postword + cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp32,D_REG);
	cc_s[H] = ((D_REG ^ temp32 ^ postword) & 0x100)>>8;
	D_REG = temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Ord_D(void)
{ //109A 6309 Untested
	D_REG|=MemRead16(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Addw_D(void)
{ //109B 6309
	temp16=MemRead16( DPADDRESS(PC_REG++));
	temp32= W_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG = temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Cmpy_D(void)
{	//109C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= Y_REG - postword ;
	cc_s[C] = temp16 > Y_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,Y_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Ldy_D(void)
{ //109E
	Y_REG=MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Y_REG);	
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}
	
static void Sty_D(void)
{ //109F
	MemWrite16(Y_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Subw_X(void)
{ //10A0 6309 MODDED
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32=W_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Cmpw_X(void)
{ //10A1 6309
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= W_REG - postword ;
	cc_s[C] = temp16 > W_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,W_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Sbcd_X(void)
{ //10A2 6309
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp32=D_REG-postword-cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp32,D_REG);
	D_REG= temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Cmpd_X(void)
{ //10A3
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= D_REG - postword ;
	cc_s[C] = temp16 > D_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,D_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Andd_X(void)
{ //10A4 6309
	D_REG&=MemRead16(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Bitd_X(void)
{ //10A5 6309 Untested
	temp16= D_REG & MemRead16(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldw_X(void)
{ //10A6 6309
	W_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Stw_X(void)
{ //10A7 6309
	MemWrite16(W_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Eord_X(void)
{ //10A8 6309 Untested TESTED NITRO 
	D_REG ^= MemRead16(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Adcd_X(void)
{ //10A9 6309 untested
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp32 = D_REG + postword + cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp32,D_REG);
	cc_s[H] = (((D_REG ^ temp32 ^ postword) & 0x100)>>8);
	D_REG = temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ord_X(void)
{ //10AA 6309 Untested wcreate
	D_REG |= MemRead16(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Addw_X(void)
{ //10AB 6309 Untested TESTED NITRO CHECK no Half carry?
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32= W_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Cmpy_X(void)
{ //10AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= Y_REG - postword ;
	cc_s[C] = temp16 > Y_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,Y_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldy_X(void)
{ //10AE
	Y_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
	}

static void Sty_X(void)
{ //10AF
	MemWrite16(Y_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Subw_E(void)
{ //10B0 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=W_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Cmpw_E(void)
{ //10B1 6309 Untested
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = W_REG-postword;
	cc_s[C] = temp16 > W_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,W_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Sbcd_E(void)
{ //10B2 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=D_REG-temp16-cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Cmpd_E(void)
{ //10B3
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = D_REG-postword;
	cc_s[C] = temp16 > D_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,D_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Andd_E(void)
{ //10B4 6309 Untested
	D_REG &= MemRead16(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Bitd_E(void)
{ //10B5 6309 Untested CHECK NITRO
	temp16= D_REG & MemRead16(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Ldw_E(void)
{ //10B6 6309
	W_REG=MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Stw_E(void)
{ //10B7 6309
	MemWrite16(W_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Eord_E(void)
{ //10B8 6309 Untested
	D_REG ^= MemRead16(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Adcd_E(void)
{ //10B9 6309 untested
	postword = MemRead16(IMMADDRESS(PC_REG));
	temp32 = D_REG + postword + cc_s[C];
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp32,D_REG);
	cc_s[H] = (((D_REG ^ temp32 ^ postword) & 0x100)>>8);
	D_REG = temp32;
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Ord_E(void)
{ //10BA 6309 Untested
	D_REG |= MemRead16(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST16(D_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Addw_E(void)
{ //10BB 6309 Untested
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32= W_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,W_REG);
	W_REG= temp32;
	cc_s[Z] = ZTEST(W_REG);
	cc_s[N] = NTEST16(W_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Cmpy_E(void)
{ //10BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = Y_REG-postword;
	cc_s[C] = temp16 > Y_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,Y_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Ldy_E(void)
{ //10BE
	Y_REG=MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Sty_E(void)
{ //10BF
	MemWrite16(Y_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(Y_REG);
	cc_s[N] = NTEST16(Y_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Lds_I(void)
{  //10CE
	S_REG=IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=4;
}

static void Ldq_D(void)
{ //10DC 6309
	Q_REG=MemRead32_s(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M87];
}

static void Stq_D(void)
{ //10DD 6309
	MemWrite32_s(Q_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M87];
}

static void Lds_D(void)
{ //10DE
	S_REG=MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Sts_D(void)
{ //10DF 6309
	MemWrite16(S_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Ldq_X(void)
{ //10EC 6309
	Q_REG=MemRead32_s(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	CycleCounter+=8;
}

static void Stq_X(void)
{ //10ED 6309 DONE
	MemWrite32_s(Q_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	CycleCounter+=8;
}


static void Lds_X(void)
{ //10EE
	S_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Sts_X(void)
{ //10EF 6309
	MemWrite16(S_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Ldq_E(void)
{ //10FC 6309
	Q_REG=MemRead32_s(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M98];
}

static void Stq_E(void)
{ //10FD 6309
	MemWrite32_s(Q_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M98];
}

static void Lds_E(void)
{ //10FE
	S_REG=MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Sts_E(void)
{ //10FF 6309
	MemWrite16(S_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(S_REG);
	cc_s[N] = NTEST16(S_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Band(void)
{ //1130 6309 untested
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;
	
	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0 : // A Reg
    case 1 : // B Reg
      *ureg8_s[postbyte] &= ~(1 << Dest);
      break;
    case 2 : // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
	}
	// Else nothing changes
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Biand(void)
{ //1131 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] &= ~(1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
  }
	// Else do nothing
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Bor(void)
{ //1132 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
	}
	// Else do nothing
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Bior(void)
{ //1133 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
  }
	// Else do nothing
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Beor(void)
{ //1134 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] ^= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() ^ (1 << Dest));
      break;
    }
	}
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Bieor(void)
{ //1135 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) == 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] ^= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() ^ (1 << Dest));
      break;
    }
  }
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldbt(void)
{ //1136 6309
	postbyte = MemRead8(PC_REG++);
	temp8 = MemRead8(DPADDRESS(PC_REG++));
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

	if ((temp8 & (1 << Source)) != 0)
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] |= (1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() | (1 << Dest));
      break;
    }
  }
	else
	{
    switch (postbyte)
    {
    case 0: // A Reg
    case 1: // B Reg
      *ureg8_s[postbyte] &= ~(1 << Dest);
      break;
    case 2: // CC Reg
      setcc(getcc() & ~(1 << Dest));
      break;
    }
	}
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Stbt(void)
{ //1137 6309
	postbyte = MemRead8(PC_REG++);
	temp16 = DPADDRESS(PC_REG++);
	temp8 = MemRead8(temp16);
	Source = (postbyte >> 3) & 7;
	Dest = (postbyte) & 7;
	postbyte >>= 6;

	if (postbyte == 3)
	{
		InvalidInsHandler_s();
		return;
	}

  switch (postbyte)
  {
  case 0: // A Reg
  case 1: // B Reg
    postbyte = *ureg8_s[postbyte];
    break;
  case 2: // CC Reg
    postbyte = getcc();
    break;
  }

	if ((postbyte & (1 << Source)) != 0)
	{
		temp8 |= (1 << Dest);
	}
	else
	{
		temp8 &= ~(1 << Dest);
	}
	MemWrite8(temp8, temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M87];
}

static void Tfm1(void)
{ //1138 TFM R+,R+ 6309
	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler_s();
		return;
	}

  temp8 = MemRead8(*xfreg16_s[Source]);
  MemWrite8(temp8, *xfreg16_s[Dest]);
  (*xfreg16_s[Dest])++;
  (*xfreg16_s[Source])++;
  W_REG--;

	if ((W_REG)!=0)
	{
    CycleCounter += 3;
    PC_REG -= 2;
  }
	else
	{
    CycleCounter += 6;
    PC_REG++;
  }
}

static void Tfm2(void)
{ //1139 TFM R-,R- Phase 3
	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler_s();
		return;
	}

  temp8 = MemRead8(*xfreg16_s[Source]);
  MemWrite8(temp8, *xfreg16_s[Dest]);
  (*xfreg16_s[Dest])--;
  (*xfreg16_s[Source])--;
  W_REG--;

	if (W_REG!=0)
	{
		CycleCounter+=3;
		PC_REG-=2;
	}
	else
	{
		CycleCounter+=6;
		PC_REG++;
	}			
}

static void Tfm3(void)
{ //113A 6309 TFM R+,R 6309
	postbyte = MemRead8(PC_REG);
	Source = postbyte >> 4;
	Dest = postbyte & 15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler_s();
		return;
	}

  temp8 = MemRead8(*xfreg16_s[Source]);
  MemWrite8(temp8, *xfreg16_s[Dest]);
  (*xfreg16_s[Source])++;
  W_REG--;

  if (W_REG!=0)
  {
    PC_REG -= 2; //Hit the same instruction on the next loop if not done copying
		CycleCounter += 3;
	}
	else
	{
		CycleCounter += 6;
		PC_REG++;
	}
}

static void Tfm4(void)
{ //113B TFM R,R+ 6309 
	postbyte=MemRead8(PC_REG);
	Source=postbyte>>4;
	Dest=postbyte&15;

	if (Source > 4 || Dest > 4)
	{
		InvalidInsHandler_s();
		return;
	}

  temp8 = MemRead8(*xfreg16_s[Source]);
  MemWrite8(temp8, *xfreg16_s[Dest]);
  (*xfreg16_s[Dest])++;
  W_REG--;

	if (W_REG!=0)
	{
		PC_REG-=2; //Hit the same instruction on the next loop if not done copying
		CycleCounter+=3;
	}
	else
	{
		CycleCounter+=6;
		PC_REG++;
	}
}

static void Bitmd_M(void)
{ //113C  6309
	postbyte = MemRead8(PC_REG++) & 0xC0;
	temp8 = getmd() & postbyte;
	cc_s[Z] = ZTEST(temp8);
	if (temp8 & 0x80) mdbits_s &= 0x7F; // md[7] = 0;
	if (temp8 & 0x40) mdbits_s &= 0xBF; // md[6] = 0;
	CycleCounter+=4;
}

static void Ldmd_M(void)
{ //113D DONE 6309
	mdbits_s= MemRead8(PC_REG++)&0x03;
	setmd_s(mdbits_s);
	CycleCounter+=5;
}

static void Swi3_I(void)
{ //113F
	cc_s[E]=1;
	MemWrite8( pc_s.B.lsb,--S_REG);
	MemWrite8( pc_s.B.msb,--S_REG);
	MemWrite8( u_s.B.lsb,--S_REG);
	MemWrite8( u_s.B.msb,--S_REG);
	MemWrite8( y_s.B.lsb,--S_REG);
	MemWrite8( y_s.B.msb,--S_REG);
	MemWrite8( x_s.B.lsb,--S_REG);
	MemWrite8( x_s.B.msb,--S_REG);
	MemWrite8( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI3);
	CycleCounter+=20;
}

static void Come_I(void)
{ //1143 6309 Untested
	E_REG = 0xFF- E_REG;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Dece_I(void)
{ //114A 6309
	E_REG--;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = E_REG==0x7F;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Ince_I(void)
{ //114C 6309
	E_REG++;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = E_REG==0x80;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Tste_I(void)
{ //114D 6309 Untested TESTED NITRO
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Clre_I(void)
{ //114F 6309
	E_REG= 0;
	cc_s[C] = 0;
	cc_s[V] = 0;
	cc_s[N] = 0;
	cc_s[Z] = 1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Comf_I(void)
{ //1153 6309 Untested
	F_REG= 0xFF- F_REG;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Decf_I(void)
{ //115A 6309
	F_REG--;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = F_REG==0x7F;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Incf_I(void)
{ //115C 6309 Untested
	F_REG++;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = F_REG==0x80;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Tstf_I(void)
{ //115D 6309
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Clrf_I(void)
{ //115F 6309 Untested wcreate
	F_REG= 0;
	cc_s[C] = 0;
	cc_s[V] = 0;
	cc_s[N] = 0;
	cc_s[Z] = 1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Sube_M(void)
{ //1180 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = E_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	CycleCounter+=3;
}

static void Cmpe_M(void)
{ //1181 6309
	postbyte=MemRead8(PC_REG++);
	temp8= E_REG-postbyte;
	cc_s[C] = temp8 > E_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,E_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=3;
}

static void Cmpu_M(void)
{ //1183
	postword=IMMADDRESS(PC_REG);
	temp16 = U_REG-postword;
	cc_s[C] = temp16 > U_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,U_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;	
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Lde_M(void)
{ //1186 6309
	E_REG= MemRead8(PC_REG++);
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=3;
}

static void Adde_M(void)
{ //118B 6309
	postbyte=MemRead8(PC_REG++);
	temp16=E_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] =OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(E_REG);
	cc_s[Z] = ZTEST(E_REG);
	CycleCounter+=3;
}

static void Cmps_M(void)
{ //118C
	postword=IMMADDRESS(PC_REG);
	temp16 = S_REG-postword;
	cc_s[C] = temp16 > S_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,S_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Divd_M(void)
{ //118D 6309
	postbyte = MemRead8(PC_REG++);

	if (postbyte == 0)
	{
		CycleCounter+=3;
		DivbyZero_s();
		return;			
	}

	postword = D_REG;
	stemp16 = (signed short)postword / (signed char)postbyte;

	if ((stemp16 > 255) || (stemp16 < -256)) //Abort
	{
		cc_s[V] = 1;
		cc_s[N] = 0;
		cc_s[Z] = 0;
		cc_s[C] = 0;
		CycleCounter+=17;
		return;
	}

	A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
	B_REG = stemp16;

	if ((stemp16 > 127) || (stemp16 < -128)) 
	{
		cc_s[V] = 1;
		cc_s[N] = 1;
	}
	else
	{
		cc_s[Z] = ZTEST(B_REG);
		cc_s[N] = NTEST8(B_REG);
		cc_s[V] = 0;
	}
	cc_s[C] = B_REG & 1;
	CycleCounter+=25;
}

static void Divq_M(void)
{ //118E 6309
	postword = MemRead16(PC_REG);
	PC_REG+=2;

	if(postword == 0)
	{
		CycleCounter+=4;
		DivbyZero_s();
		return;			
	}

	temp32 = Q_REG;
	stemp32 = (signed int)temp32 / (signed short int)postword;

	if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
	{
		cc_s[V] = 1;
		cc_s[N] = 0;
		cc_s[Z] = 0;
		cc_s[C] = 0;
		CycleCounter+=34-21;
		return;
	}

	D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
	W_REG = stemp32;
	if ((stemp16 > 32767) || (stemp16 < -32768)) 
	{
		cc_s[V] = 1;
		cc_s[N] = 1;
	}
	else
	{
		cc_s[Z] = ZTEST(W_REG);
		cc_s[N] = NTEST16(W_REG);
		cc_s[V] = 0;
	}
	cc_s[C] = B_REG & 1;
	CycleCounter+=34;
}

static void Muld_M(void)
{ //118F Phase 5 6309
	Q_REG =  (signed short)D_REG * (signed short)IMMADDRESS(PC_REG);
	cc_s[C] = 0; 
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[V] = 0;
	cc_s[N] = NTEST32(Q_REG);
	PC_REG+=2;
	CycleCounter+=28;
}

static void Sube_D(void)
{ //1190 6309 Untested HERE
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = E_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpe_D(void)
{ //1191 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= E_REG-postbyte;
	cc_s[C] = temp8 > E_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,E_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpu_D(void)
{ //1193
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= U_REG - postword ;
	cc_s[C] = temp16 > U_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,U_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
	}

static void Lde_D(void)
{ //1196 6309
	E_REG= MemRead8(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ste_D(void)
{ //1197 Phase 5 6309
	MemWrite8( E_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Adde_D(void)
{ //119B 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=E_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(E_REG);
	cc_s[Z] =ZTEST(E_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmps_D(void)
{ //119C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= S_REG - postword ;
	cc_s[C] = temp16 > S_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,S_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Divd_D(void)
{ //119D 6309 02292008
	postbyte = MemRead8(DPADDRESS(PC_REG++));

	if (postbyte == 0)
	{
		CycleCounter+=3;
		DivbyZero_s();
		return;			
	}

	postword = D_REG;
	stemp16 = (signed short)postword / (signed char)postbyte;

	if ((stemp16 > 255) || (stemp16 < -256)) //Abort
	{
		cc_s[V] = 1;
		cc_s[N] = 0;
		cc_s[Z] = 0;
		cc_s[C] = 0;
		CycleCounter+=19;
		return;
	}

	A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
	B_REG = stemp16;

	if ((stemp16 > 127) || (stemp16 < -128)) 
	{
		cc_s[V] = 1;
		cc_s[N] = 1;
	}
	else
	{
		cc_s[Z] = ZTEST(B_REG);
		cc_s[N] = NTEST8(B_REG);
		cc_s[V] = 0;
	}
	cc_s[C] = B_REG & 1;
	CycleCounter+=27;
}

static void Divq_D(void)
{ //119E 6309
	postword = MemRead16(DPADDRESS(PC_REG++));

	if(postword == 0)
	{
		CycleCounter+=4;
		DivbyZero_s();
		return;			
	}

	temp32 = Q_REG;
	stemp32 = (signed int)temp32 / (signed short int)postword;

	if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
	{
		cc_s[V] = 1;
		cc_s[N] = 0;
		cc_s[Z] = 0;
		cc_s[C] = 0;
		CycleCounter+=InsCycles[MD_NATIVE6309][M3635]-21;
		return;
	}

	D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
	W_REG = stemp32;
	if ((stemp16 > 32767) || (stemp32 < -32768)) 
	{
		cc_s[V] = 1;
		cc_s[N] = 1;
	}
	else
	{
		cc_s[Z] = ZTEST(W_REG);
		cc_s[N] = NTEST16(W_REG);
		cc_s[V] = 0;
	}
	cc_s[C] = B_REG & 1;
  CycleCounter+=InsCycles[MD_NATIVE6309][M3635];
}

static void Muld_D(void)
{ //119F 6309 02292008
	Q_REG = (signed short)D_REG * (signed short)MemRead16(DPADDRESS(PC_REG++));
	cc_s[C] = 0;
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[V] = 0;
	cc_s[N] = NTEST32(Q_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M3029];
}

static void Sube_X(void)
{ //11A0 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = E_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	CycleCounter+=5;
}

static void Cmpe_X(void)
{ //11A1 6309
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= E_REG-postbyte;
	cc_s[C] = temp8 > E_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,E_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=5;
}

static void Cmpu_X(void)
{ //11A3
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= U_REG - postword ;
	cc_s[C] = temp16 > U_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,U_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Lde_X(void)
{ //11A6 6309
	E_REG= MemRead8(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Ste_X(void)
{ //11A7 6309
	MemWrite8(E_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Adde_X(void)
{ //11AB 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=E_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] =OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(E_REG);
	cc_s[Z] = ZTEST(E_REG);
	CycleCounter+=5;
}

static void Cmps_X(void)
{  //11AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= S_REG - postword ;
	cc_s[C] = temp16 > S_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,S_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Divd_X(void)
{ //11AD wcreate  6309
	postbyte = MemRead8(INDADDRESS(PC_REG++));

  if (postbyte == 0)
  {
    CycleCounter += 3;
    DivbyZero_s();
    return;
  }

  postword = D_REG;
  stemp16 = (signed short)postword / (signed char)postbyte;

  if ((stemp16 > 255) || (stemp16 < -256)) //Abort
  {
    cc_s[V] = 1;
    cc_s[N] = 0;
    cc_s[Z] = 0;
    cc_s[C] = 0;
    CycleCounter += 19;
    return;
  }

  A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
  B_REG = stemp16;

  if ((stemp16 > 127) || (stemp16 < -128))
  {
    cc_s[V] = 1;
    cc_s[N] = 1;
  }
  else
  {
    cc_s[Z] = ZTEST(B_REG);
    cc_s[N] = NTEST8(B_REG);
    cc_s[V] = 0;
  }
  cc_s[C] = B_REG & 1;
  CycleCounter += 27;
}

static void Divq_X(void)
{ //11AE Phase 5 6309 CHECK
	postword = MemRead16(INDADDRESS(PC_REG++));

  if (postword == 0)
  {
    CycleCounter += 4;
    DivbyZero_s();
    return;
  }

  temp32 = Q_REG;
  stemp32 = (signed int)temp32 / (signed short int)postword;

  if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
  {
    cc_s[V] = 1;
    cc_s[N] = 0;
    cc_s[Z] = 0;
    cc_s[C] = 0;
    CycleCounter += InsCycles[MD_NATIVE6309][M3635] - 21;
    return;
  }

  D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
  W_REG = stemp32;
  if ((stemp16 > 32767) || (stemp16 < -32768))
  {
    cc_s[V] = 1;
    cc_s[N] = 1;
  }
  else
  {
    cc_s[Z] = ZTEST(W_REG);
    cc_s[N] = NTEST16(W_REG);
    cc_s[V] = 0;
  }
  cc_s[C] = B_REG & 1;
  CycleCounter += InsCycles[MD_NATIVE6309][M3635];
}

static void Muld_X(void)
{ //11AF 6309 CHECK
	Q_REG=  (signed short)D_REG * (signed short)MemRead16(INDADDRESS(PC_REG++));
	cc_s[C] = 0;
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[V] = 0;
	cc_s[N] = NTEST32(Q_REG);
	CycleCounter+=30;
}

static void Sube_E(void)
{ //11B0 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = E_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Cmpe_E(void)
{ //11B1 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= E_REG-postbyte;
	cc_s[C] = temp8 > E_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,E_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Cmpu_E(void)
{ //11B3
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = U_REG-postword;
	cc_s[C] = temp16 > U_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,U_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Lde_E(void)
{ //11B6 6309
	E_REG= MemRead8(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Ste_E(void)
{ //11B7 6309
	MemWrite8(E_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(E_REG);
	cc_s[N] = NTEST8(E_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Adde_E(void)
{ //11BB 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=E_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((E_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,E_REG);
	E_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(E_REG);
	cc_s[Z] = ZTEST(E_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Cmps_E(void)
{ //11BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = S_REG-postword;
	cc_s[C] = temp16 > S_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,S_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M86];
}

static void Divd_E(void)
{ //11BD 6309 02292008 Untested
	postbyte = MemRead8(IMMADDRESS(PC_REG));
	PC_REG+=2;

  if (postbyte == 0)
  {
    CycleCounter += 3;
    DivbyZero_s();
    return;
  }

  postword = D_REG;
  stemp16 = (signed short)postword / (signed char)postbyte;

  if ((stemp16 > 255) || (stemp16 < -256)) //Abort
  {
    cc_s[V] = 1;
    cc_s[N] = 0;
    cc_s[Z] = 0;
    cc_s[C] = 0;
    CycleCounter += 17;
    return;
  }

  A_REG = (unsigned char)((signed short)postword % (signed char)postbyte);
  B_REG = stemp16;

  if ((stemp16 > 127) || (stemp16 < -128))
  {
    cc_s[V] = 1;
    cc_s[N] = 1;
  }
  else
  {
    cc_s[Z] = ZTEST(B_REG);
    cc_s[N] = NTEST8(B_REG);
    cc_s[V] = 0;
  }
  cc_s[C] = B_REG & 1;
  CycleCounter += 25;
}

static void Divq_E(void)
{ //11BE Phase 5 6309 CHECK
	postword = MemRead16(IMMADDRESS(PC_REG));
	PC_REG+=2;

  if (postword == 0)
  {
    CycleCounter += 4;
    DivbyZero_s();
    return;
  }

  temp32 = Q_REG;
  stemp32 = (signed int)temp32 / (signed short int)postword;

  if ((stemp32 > 65535) || (stemp32 < -65536)) //Abort
  {
    cc_s[V] = 1;
    cc_s[N] = 0;
    cc_s[Z] = 0;
    cc_s[C] = 0;
    CycleCounter += InsCycles[MD_NATIVE6309][M3635] - 21;
    return;
  }

  D_REG = (unsigned short)((signed int)temp32 % (signed short int)postword);
  W_REG = stemp32;
  if ((stemp16 > 32767) || (stemp16 < -32768))
  {
    cc_s[V] = 1;
    cc_s[N] = 1;
  }
  else
  {
    cc_s[Z] = ZTEST(W_REG);
    cc_s[N] = NTEST16(W_REG);
    cc_s[V] = 0;
  }
  cc_s[C] = B_REG & 1;
  CycleCounter += InsCycles[MD_NATIVE6309][M3635];
}

static void Muld_E(void)
{ //11BF 6309
	Q_REG=  (signed short)D_REG * (signed short)MemRead16(IMMADDRESS(PC_REG));
	PC_REG+=2;
	cc_s[C] = 0;
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[V] = 0;
	cc_s[N] = NTEST32(Q_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M3130];
}

static void Subf_M(void)
{ //11C0 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16 = F_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	CycleCounter+=3;
}

static void Cmpf_M(void)
{ //11C1 6309
	postbyte=MemRead8(PC_REG++);
	temp8= F_REG-postbyte;
	cc_s[C] = temp8 > F_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,F_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=3;
}

static void Ldf_M(void)
{ //11C6 6309
	F_REG= MemRead8(PC_REG++);
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=3;
}

static void Addf_M(void)
{ //11CB 6309 Untested
	postbyte=MemRead8(PC_REG++);
	temp16=F_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(F_REG);
	cc_s[Z] = ZTEST(F_REG);
	CycleCounter+=3;
}

static void Subf_D(void)
{ //11D0 6309 Untested
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = F_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpf_D(void)
{ //11D1 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= F_REG-postbyte;
	cc_s[C] = temp8 > F_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,F_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ldf_D(void)
{ //11D6 6309 Untested wcreate
	F_REG= MemRead8(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Stf_D(void)
{ //11D7 Phase 5 6309
	MemWrite8(F_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Addf_D(void)
{ //11DB 6309 Untested
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=F_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] =OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(F_REG);
	cc_s[Z] = ZTEST(F_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Subf_X(void)
{ //11E0 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = F_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	CycleCounter+=5;
}

static void Cmpf_X(void)
{ //11E1 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= F_REG-postbyte;
	cc_s[C] = temp8 > F_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,F_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=5;
}

static void Ldf_X(void)
{ //11E6 6309
	F_REG=MemRead8(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Stf_X(void)
{ //11E7 Phase 5 6309
	MemWrite8(F_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Addf_X(void)
{ //11EB 6309 Untested
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=F_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(F_REG);
	cc_s[Z] = ZTEST(F_REG);
	CycleCounter+=5;
}

static void Subf_E(void)
{ //11F0 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = F_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Cmpf_E(void)
{ //11F1 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= F_REG-postbyte;
	cc_s[C] = temp8 > F_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,F_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Ldf_E(void)
{ //11F6 6309
	F_REG= MemRead8(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Stf_E(void)
{ //11F7 Phase 5 6309
	MemWrite8(F_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(F_REG);
	cc_s[N] = NTEST8(F_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Addf_E(void)
{ //11FB 6309 Untested
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=F_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((F_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,F_REG);
	F_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(F_REG);
	cc_s[Z] = ZTEST(F_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Nop_I(void)
{	//12
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Sync_I(void)
{ //13
	CycleCounter=gCycleFor;
	SyncWaiting_s=1;
}

static void Sexw_I(void)
{ //14 6309 CHECK
	if (W_REG & 32768)
		D_REG=0xFFFF;
	else
		D_REG=0;
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=4;
}

static void Lbra_R(void)
{ //16
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	PC_REG+=*spostword;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Lbsr_R(void)
{ //17
	*spostword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc_s.B.lsb,S_REG--);
	MemWrite8(pc_s.B.msb,S_REG);
	PC_REG+=*spostword;
	CycleCounter+=InsCycles[MD_NATIVE6309][M97];
}

static void Daa_I(void)
{ //19
	static unsigned char msn, lsn;

	msn=(A_REG & 0xF0) ;
	lsn=(A_REG & 0xF);
	temp8=0;
	if ( cc_s[H] ||  (lsn >9) )
		temp8 |= 0x06;

	if ( (msn>0x80) && (lsn>9)) 
		temp8|=0x60;
	
	if ( (msn>0x90) || cc_s[C] )
		temp8|=0x60;

	temp16= A_REG+temp8;
	cc_s[C]|= ((temp16 & 0x100)>>8);
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Orcc_M(void)
{ //1A
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 | postbyte);
	setcc(temp8);
	CycleCounter+=InsCycles[MD_NATIVE6309][M32];
}

static void Andcc_M(void)
{ //1C
	postbyte=MemRead8(PC_REG++);
	temp8=getcc();
	temp8 = (temp8 & postbyte);
	setcc(temp8);
	CycleCounter+=3;
}

static void Sex_I(void)
{ //1D
	A_REG= 0-(B_REG>>7);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = D_REG >> 15;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Exg_M(void)
{ //1E
	postbyte = MemRead8(PC_REG++);
	Source = postbyte >> 4;
	Dest = postbyte & 15;

	ccbits_s = getcc();
	if ((Source & 0x08) == (Dest & 0x08)) //Verify like size registers
	{
		if (Dest & 0x08) //8 bit EXG
		{
			Source &= 0x07;
			Dest &= 0x07;
			temp8 = (*ureg8_s[Source]);
			if (Source != 4 && Source != 5) (*ureg8_s[Source]) = (*ureg8_s[Dest]);
			if (Dest!=4 && Dest!=5) (*ureg8_s[Dest]) = temp8;
		}
		else // 16 bit EXG
		{
			Source &= 0x07;
			Dest &= 0x07;
			temp16 = (*xfreg16_s[Source]);
			(*xfreg16_s[Source]) = (*xfreg16_s[Dest]);
			(*xfreg16_s[Dest]) = temp16;
		}
	}
	else
	{
		if (Dest & 0x08) // Swap 16 to 8 bit exchange to be 8 to 16 bit exchange (for convenience)
		{
			temp8 = Dest; Dest = Source; Source = temp8;
		}

		Source &= 0x07;
		Dest &= 0x07;

		switch (Source)
		{
		case 0x04: // Z
		case 0x05: // Z
			(*xfreg16_s[Dest]) = 0; // Source is Zero reg. Just zero the Destination.
			break;
		case 0x00: // A
		case 0x03: // DP
		case 0x06: // E
			temp8 = *ureg8_s[Source];
			temp16 = (temp8 << 8) | temp8;
			(*ureg8_s[Source]) = (*xfreg16_s[Dest]) >> 8; // A, DP, E get high byte of 16 bit Dest
			(*xfreg16_s[Dest]) = temp16; // Place 8 bit source in both halves of 16 bit Dest
			break;
		case 0x01: // B
		case 0x02: // CC
		case 0x07: // F
			temp8 = *ureg8_s[Source];
			temp16 = (temp8 << 8) | temp8;
			(*ureg8_s[Source]) = (*xfreg16_s[Dest]) & 0xFF; // B, CC, F get low byte of 16 bit Dest
			(*xfreg16_s[Dest]) = temp16; // Place 8 bit source in both halves of 16 bit Dest
			break;
		}
	}
	setcc(ccbits_s);
	CycleCounter += InsCycles[MD_NATIVE6309][M85];
}

static void Tfr_M(void)
{ //1F
	postbyte=MemRead8(PC_REG++);
	Source= postbyte>>4;
	Dest=postbyte & 15;
	ccbits_s = getcc();

	if (Dest < 8)
		if (Source < 8)
			*xfreg16_s[Dest] = *xfreg16_s[Source];
		else
			*xfreg16_s[Dest] = (*ureg8_s[Source & 7] << 8) | *ureg8_s[Source & 7];
	else
	{
		if (Source < 8)
			switch (Dest)
			{
			case 8:
			case 11:
			case 14:
				*ureg8_s[Dest & 7] = *xfreg16_s[Source] >> 8;
				break;
			case 9:
			case 10:
			case 15:
				*ureg8_s[Dest & 7] = *xfreg16_s[Source] & 0xFF;
				break;
			}
		else
			*ureg8_s[Dest & 7] = *ureg8_s[Source & 7];

		setcc(ccbits_s);
	}
	
	CycleCounter+=InsCycles[MD_NATIVE6309][M64];
}

static void Bra_R(void)
{ //20
	*spostbyte=MemRead8(PC_REG++);
	PC_REG+=*spostbyte;
	CycleCounter+=3;
}

static void Brn_R(void)
{ //21
	CycleCounter+=3;
	PC_REG++;
}

static void Bhi_R(void)
{ //22
	if  (!(cc_s[C] | cc_s[Z]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bls_R(void)
{ //23
	if (cc_s[C] | cc_s[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bhs_R(void)
{ //24
	if (!cc_s[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Blo_R(void)
{ //25
	if (cc_s[C])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bne_R(void)
{ //26
	if (!cc_s[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Beq_R(void)
{ //27
	if (cc_s[Z])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bvc_R(void)
{ //28
	if (!cc_s[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bvs_R(void)
{ //29
	if ( cc_s[V])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bpl_R(void)
{ //2A
	if (!cc_s[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bmi_R(void)
{ //2B
	if ( cc_s[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bge_R(void)
{ //2C
	if (! (cc_s[N] ^ cc_s[V]))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Blt_R(void)
{ //2D
	if ( cc_s[V] ^ cc_s[N])
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Bgt_R(void)
{ //2E
	if ( !( cc_s[Z] | (cc_s[N]^cc_s[V] ) ))
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Ble_R(void)
{ //2F
	if ( cc_s[Z] | (cc_s[N]^cc_s[V]) )
		PC_REG+=(signed char)MemRead8(PC_REG);
	PC_REG++;
	CycleCounter+=3;
}

static void Leax_X(void)
{ //30
	X_REG=INDADDRESS(PC_REG++);
	cc_s[Z] = ZTEST(X_REG);
	CycleCounter+=4;
}

static void Leay_X(void)
{ //31
	Y_REG=INDADDRESS(PC_REG++);
	cc_s[Z] = ZTEST(Y_REG);
	CycleCounter+=4;
}

static void Leas_X(void)
{ //32
	S_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
}

static void Leau_X(void)
{ //33
	U_REG=INDADDRESS(PC_REG++);
	CycleCounter+=4;
}

static void Pshs_M(void)
{ //34
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x80)
	{
		MemWrite8( pc_s.B.lsb,--S_REG);
		MemWrite8( pc_s.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		MemWrite8( u_s.B.lsb,--S_REG);
		MemWrite8( u_s.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		MemWrite8( y_s.B.lsb,--S_REG);
		MemWrite8( y_s.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		MemWrite8( x_s.B.lsb,--S_REG);
		MemWrite8( x_s.B.msb,--S_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		MemWrite8( dp_s.B.msb,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		MemWrite8(B_REG,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		MemWrite8(A_REG,--S_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		MemWrite8(getcc(),--S_REG);
		CycleCounter+=1;
	}

	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Puls_M(void)
{ //35
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(MemRead8(S_REG++));
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		A_REG=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		B_REG=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		dp_s.B.msb=MemRead8(S_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		x_s.B.msb=MemRead8( S_REG++);
		x_s.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		y_s.B.msb=MemRead8( S_REG++);
		y_s.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		u_s.B.msb=MemRead8( S_REG++);
		u_s.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		pc_s.B.msb=MemRead8( S_REG++);
		pc_s.B.lsb=MemRead8( S_REG++);
		CycleCounter+=2;
	}
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Pshu_M(void)
{ //36
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x80)
	{
		MemWrite8( pc_s.B.lsb,--U_REG);
		MemWrite8( pc_s.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		MemWrite8( s_s.B.lsb,--U_REG);
		MemWrite8( s_s.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		MemWrite8( y_s.B.lsb,--U_REG);
		MemWrite8( y_s.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x10)
	{
		MemWrite8( x_s.B.lsb,--U_REG);
		MemWrite8( x_s.B.msb,--U_REG);
		CycleCounter+=2;
	}
	if (postbyte & 0x08)
	{
		MemWrite8( dp_s.B.msb,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		MemWrite8(B_REG,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		MemWrite8(A_REG,--U_REG);
		CycleCounter+=1;
	}
	if (postbyte & 0x01)
	{
		MemWrite8(getcc(),--U_REG);
		CycleCounter+=1;
	}
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Pulu_M(void)
{ //37
	postbyte=MemRead8(PC_REG++);
	if (postbyte & 0x01)
	{
		setcc(MemRead8(U_REG++));
		CycleCounter+=1;
	}
	if (postbyte & 0x02)
	{
		A_REG=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x04)
	{
		B_REG=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x08)
	{
		dp_s.B.msb=MemRead8(U_REG++);
		CycleCounter+=1;
	}
	if (postbyte & 0x10)
	{
		x_s.B.msb=MemRead8( U_REG++);
		x_s.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x20)
	{
		y_s.B.msb=MemRead8( U_REG++);
		y_s.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x40)
	{
		s_s.B.msb=MemRead8( U_REG++);
		s_s.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	if (postbyte & 0x80)
	{
		pc_s.B.msb=MemRead8( U_REG++);
		pc_s.B.lsb=MemRead8( U_REG++);
		CycleCounter+=2;
	}
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Rts_I(void)
{ //39
	pc_s.B.msb=MemRead8(S_REG++);
	pc_s.B.lsb=MemRead8(S_REG++);
	CycleCounter+=InsCycles[MD_NATIVE6309][M51];
}

static void Abx_I(void)
{ //3A
	X_REG=X_REG+B_REG;
	CycleCounter+=InsCycles[MD_NATIVE6309][M31];
}

static void Rti_I(void)
{ //3B
	setcc(MemRead8(S_REG++));
	CycleCounter+=6;
	InInterupt_s=0;
	if (cc_s[E])
	{
		A_REG=MemRead8(S_REG++);
		B_REG=MemRead8(S_REG++);
		if (MD_NATIVE6309)
		{
			(E_REG)=MemRead8(S_REG++);
			(F_REG)=MemRead8(S_REG++);
			CycleCounter+=2;
		}
		dp_s.B.msb=MemRead8(S_REG++);
		x_s.B.msb=MemRead8(S_REG++);
		x_s.B.lsb=MemRead8(S_REG++);
		y_s.B.msb=MemRead8(S_REG++);
		y_s.B.lsb=MemRead8(S_REG++);
		u_s.B.msb=MemRead8(S_REG++);
		u_s.B.lsb=MemRead8(S_REG++);
		CycleCounter+=9;
	}
	pc_s.B.msb=MemRead8(S_REG++);
	pc_s.B.lsb=MemRead8(S_REG++);
}

static void Cwai_I(void)
{ //3C
	postbyte=MemRead8(PC_REG++);
	ccbits_s=getcc();
	ccbits_s = ccbits_s & postbyte;
	setcc(ccbits_s);
	CycleCounter=gCycleFor;
	SyncWaiting_s=1;
}

static void Mul_I(void)
{ //3D
	D_REG = A_REG * B_REG;
	cc_s[C] = B_REG >0x7F;
	cc_s[Z] = ZTEST(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M1110];
}

static void Reset(void) // 3E
{	//Undocumented
	HD6309Reset_s();
}

static void Swi1_I(void)
{ //3F
	cc_s[E]=1;
	MemWrite8( pc_s.B.lsb,--S_REG);
	MemWrite8( pc_s.B.msb,--S_REG);
	MemWrite8( u_s.B.lsb,--S_REG);
	MemWrite8( u_s.B.msb,--S_REG);
	MemWrite8( y_s.B.lsb,--S_REG);
	MemWrite8( y_s.B.msb,--S_REG);
	MemWrite8( x_s.B.lsb,--S_REG);
	MemWrite8( x_s.B.msb,--S_REG);
	MemWrite8( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VSWI);
	CycleCounter+=19;
	cc_s[I]=1;
	cc_s[F]=1;
}

static void Nega_I(void)
{ //40
	temp8= 0-A_REG;
	cc_s[C] = temp8>0;
	cc_s[V] = A_REG==0x80; //cc_s[C] ^ ((A_REG^temp8)>>7);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	A_REG= temp8;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Coma_I(void)
{ //43
	A_REG= 0xFF- A_REG;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Lsra_I(void)
{ //44
	cc_s[C] = A_REG & 1;
	A_REG= A_REG>>1;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Rora_I(void)
{ //46
	postbyte=cc_s[C]<<7;
	cc_s[C] = A_REG & 1;
	A_REG= (A_REG>>1) | postbyte;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Asra_I(void)
{ //47
	cc_s[C] = A_REG & 1;
	A_REG= (A_REG & 0x80) | (A_REG >> 1);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Asla_I(void)
{ //48
	cc_s[C] = A_REG > 0x7F;
	cc_s[V] =  cc_s[C] ^((A_REG & 0x40)>>6);
	A_REG= A_REG<<1;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Rola_I(void)
{ //49
	postbyte=cc_s[C];
	cc_s[C] = A_REG > 0x7F;
	cc_s[V] = cc_s[C] ^ ((A_REG & 0x40)>>6);
	A_REG= (A_REG<<1) | postbyte;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Deca_I(void)
{ //4A
	A_REG--;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = A_REG==0x7F;
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Inca_I(void)
{ //4C
	A_REG++;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = A_REG==0x80;
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Tsta_I(void)
{ //4D
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Clra_I(void)
{ //4F
	A_REG= 0;
	cc_s[C] =0;
	cc_s[V] = 0;
	cc_s[N] =0;
	cc_s[Z] =1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Negb_I(void)
{ //50
	temp8= 0-B_REG;
	cc_s[C] = temp8>0;
	cc_s[V] = B_REG == 0x80;	
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	B_REG= temp8;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Comb_I(void)
{ //53
	B_REG= 0xFF- B_REG;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[C] = 1;
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Lsrb_I(void)
{ //54
	cc_s[C] = B_REG & 1;
	B_REG= B_REG>>1;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Rorb_I(void)
{ //56
	postbyte=cc_s[C]<<7;
	cc_s[C] = B_REG & 1;
	B_REG= (B_REG>>1) | postbyte;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Asrb_I(void)
{ //57
	cc_s[C] = B_REG & 1;
	B_REG= (B_REG & 0x80) | (B_REG >> 1);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Aslb_I(void)
{ //58
	cc_s[C] = B_REG > 0x7F;
	cc_s[V] =  cc_s[C] ^((B_REG & 0x40)>>6);
	B_REG= B_REG<<1;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Rolb_I(void)
{ //59
	postbyte=cc_s[C];
	cc_s[C] = B_REG > 0x7F;
	cc_s[V] = cc_s[C] ^ ((B_REG & 0x40)>>6);
	B_REG= (B_REG<<1) | postbyte;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Decb_I(void)
{ //5A
	B_REG--;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = B_REG==0x7F;
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Incb_I(void)
{ //5C
	B_REG++;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = B_REG==0x80; 
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Tstb_I(void)
{ //5D
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Clrb_I(void)
{ //5F
	B_REG=0;
	cc_s[C] =0;
	cc_s[N] =0;
	cc_s[V] = 0;
	cc_s[Z] =1;
	CycleCounter+=InsCycles[MD_NATIVE6309][M21];
}

static void Neg_X(void)
{ //60
	temp16=INDADDRESS(PC_REG++);	
	postbyte=MemRead8(temp16);
	temp8= 0-postbyte;
	cc_s[C] = temp8>0;
	cc_s[V] = postbyte == 0x80;
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Oim_X(void)
{ //61 6309 DONE
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte |= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Aim_X(void)
{ //62 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte &= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=6;
}

static void Com_X(void)
{ //63
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8= 0xFF-temp8;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = 0;
	cc_s[C] = 1;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Lsr_X(void)
{ //64
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8= temp8 >>1;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = 0;
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Eim_X(void)
{ //65 6309 Untested TESTED NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=INDADDRESS(PC_REG++);
	postbyte ^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=7;
}

static void Ror_X(void)
{ //66
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte=cc_s[C]<<7;
	cc_s[C] = (temp8 & 1);
	temp8= (temp8>>1) | postbyte;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Asr_X(void)
{ //67
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Asl_X(void)
{ //68 
	temp16=INDADDRESS(PC_REG++);
	temp8= MemRead8(temp16);
	cc_s[C] = temp8 > 0x7F;
	cc_s[V] = cc_s[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);	
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Rol_X(void)
{ //69
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	postbyte=cc_s[C];
	cc_s[C] = temp8 > 0x7F;
	cc_s[V] = ( cc_s[C] ^ ((temp8 & 0x40)>>6));
	temp8= ((temp8<<1) | postbyte);
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Dec_X(void)
{ //6A
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8--;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = (temp8==0x7F);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Tim_X(void)
{ //6B 6309
	postbyte=MemRead8(PC_REG++);
	temp8=MemRead8(INDADDRESS(PC_REG++));
	postbyte&=temp8;
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	CycleCounter+=7;
}

static void Inc_X(void)
{ //6C
	temp16=INDADDRESS(PC_REG++);
	temp8=MemRead8(temp16);
	temp8++;
	cc_s[V] = (temp8 == 0x80);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	CycleCounter+=6;
}

static void Tst_X(void)
{ //6D
	temp8=MemRead8(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Jmp_X(void)
{ //6E
	PC_REG=INDADDRESS(PC_REG++);
	CycleCounter+=3;
}

static void Clr_X(void)
{ //6F
	MemWrite8(0,INDADDRESS(PC_REG++));
	cc_s[C] = 0;
	cc_s[N] = 0;
	cc_s[V] = 0;
	cc_s[Z] = 1; 
	CycleCounter+=6;
}

static void Neg_E(void)
{ //70
	temp16=IMMADDRESS(PC_REG);
	postbyte=MemRead8(temp16);
	temp8=0-postbyte;
	cc_s[C] = temp8>0;
	cc_s[V] = postbyte == 0x80;
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Oim_E(void)
{ //71 6309 Phase 2
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte|= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

static void Aim_E(void)
{ //72 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

static void Com_E(void)
{ //73
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8=0xFF-temp8;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[C] = 1;
	cc_s[V] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Lsr_E(void)
{  //74
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8= temp8>>1;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = 0;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Eim_E(void)
{ //75 6309 Untested CHECK NITRO
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte^= MemRead8(temp16);
	MemWrite8(postbyte,temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

static void Ror_E(void)
{ //76
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	postbyte=cc_s[C]<<7;
	cc_s[C] = temp8 & 1;
	temp8= (temp8>>1) | postbyte;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Asr_E(void)
{ //77
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	cc_s[C] = temp8 & 1;
	temp8= (temp8 & 0x80) | (temp8 >>1);
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Asl_E(void)
{ //78 6309
	temp16=IMMADDRESS(PC_REG);
	temp8= MemRead8(temp16);
	cc_s[C] = temp8 > 0x7F;
	cc_s[V] = cc_s[C] ^ ((temp8 & 0x40)>>6);
	temp8= temp8<<1;
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	MemWrite8(temp8,temp16);	
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Rol_E(void)
{ //79
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	postbyte=cc_s[C];
	cc_s[C] = temp8 > 0x7F;
	cc_s[V] = cc_s[C] ^  ((temp8 & 0x40)>>6);
	temp8 = ((temp8<<1) | postbyte);
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Dec_E(void)
{ //7A
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8--;
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = temp8==0x7F;
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Tim_E(void)
{ //7B 6309 NITRO 
	postbyte=MemRead8(PC_REG++);
	temp16=IMMADDRESS(PC_REG);
	postbyte&=MemRead8(temp16);
	cc_s[N] = NTEST8(postbyte);
	cc_s[Z] = ZTEST(postbyte);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=7;
}

static void Inc_E(void)
{ //7C
	temp16=IMMADDRESS(PC_REG);
	temp8=MemRead8(temp16);
	temp8++;
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = temp8==0x80;
	cc_s[N] = NTEST8(temp8);
	MemWrite8(temp8,temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Tst_E(void)
{ //7D
	temp8=MemRead8(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(temp8);
	cc_s[N] = NTEST8(temp8);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Jmp_E(void)
{ //7E
	PC_REG=IMMADDRESS(PC_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Clr_E(void)
{ //7F
	MemWrite8(0,IMMADDRESS(PC_REG));
	cc_s[C] = 0;
	cc_s[N] = 0;
	cc_s[V] = 0;
	cc_s[Z] = 1;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Suba_M(void)
{ //80
	postbyte=MemRead8(PC_REG++);
	temp16 = A_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=2;
}

static void Cmpa_M(void)
{ //81
	postbyte=MemRead8(PC_REG++);
	temp8= A_REG-postbyte;
	cc_s[C] = temp8 > A_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,A_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=2;
}

static void Sbca_M(void)
{  //82
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

static void Subd_M(void)
{ //83
	temp16=IMMADDRESS(PC_REG);
	temp32=D_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Anda_M(void)
{ //84
	A_REG = A_REG & MemRead8(PC_REG++);
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Bita_M(void)
{ //85
	temp8 = A_REG & MemRead8(PC_REG++);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Lda_M(void)
{ //86
	A_REG = MemRead8(PC_REG++);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Eora_M(void)
{ //88
	A_REG = A_REG ^ MemRead8(PC_REG++);
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Adca_M(void)
{ //89
	postbyte=MemRead8(PC_REG++);
	temp16= A_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	cc_s[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

static void Ora_M(void)
{ //8A
	A_REG = A_REG | MemRead8(PC_REG++);
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Adda_M(void)
{ //8B
	postbyte=MemRead8(PC_REG++);
	temp16=A_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=2;
}

static void Cmpx_M(void)
{ //8C
	postword=IMMADDRESS(PC_REG);
	temp16 = X_REG-postword;
	cc_s[C] = temp16 > X_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,X_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Bsr_R(void)
{ //8D
	*spostbyte=MemRead8(PC_REG++);
	S_REG--;
	MemWrite8(pc_s.B.lsb,S_REG--);
	MemWrite8(pc_s.B.msb,S_REG);
	PC_REG+=*spostbyte;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldx_M(void)
{ //8E
	X_REG = IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

static void Suba_D(void)
{ //90
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}	

static void Cmpa_D(void)
{ //91
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8 = A_REG-postbyte;
	cc_s[C] = temp8 > A_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,A_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Scba_D(void)
{ //92
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Subd_D(void)
{ //93
	temp16= MemRead16(DPADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M64];
}

static void Anda_D(void)
{ //94
	A_REG = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Bita_D(void)
{ //95
	temp8 = A_REG & MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Lda_D(void)
{ //96
	A_REG = MemRead8(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Sta_D(void)
{ //97
	MemWrite8( A_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Eora_D(void)
{ //98
	A_REG= A_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Adca_D(void)
{ //99
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	cc_s[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Ora_D(void)
{ //9A
	A_REG = A_REG | MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Adda_D(void)
{ //9B
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=A_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Cmpx_D(void)
{ //9C
	postword=MemRead16(DPADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc_s[C] = temp16 > X_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,X_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M64];
}

static void Jsr_D(void)
{ //9D
	temp16 = DPADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc_s.B.lsb,S_REG--);
	MemWrite8(pc_s.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldx_D(void)
{ //9E
	X_REG=MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(X_REG);	
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Stx_D(void)
{ //9F
	MemWrite16(X_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Suba_X(void)
{ //A0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = A_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	CycleCounter+=4;
}	

static void Cmpa_X(void)
{ //A1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8= A_REG-postbyte;
	cc_s[C] = temp8 > A_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,A_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=4;
}

static void Sbca_X(void)
{ //A2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=A_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

static void Subd_X(void)
{ //A3
	temp16= MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Anda_X(void)
{ //A4
	A_REG = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Bita_X(void)
{  //A5
	temp8 = A_REG & MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Lda_X(void)
{ //A6
	A_REG = MemRead8(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Sta_X(void)
{ //A7
	MemWrite8(A_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Eora_X(void)
{ //A8
	A_REG= A_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Adca_X(void)
{ //A9	
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	cc_s[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

static void Ora_X(void)
{ //AA
	A_REG= A_REG | MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Adda_X(void)
{ //AB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= A_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	CycleCounter+=4;
}

static void Cmpx_X(void)
{ //AC
	postword=MemRead16(INDADDRESS(PC_REG++));
	temp16= X_REG - postword ;
	cc_s[C] = temp16 > X_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,X_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Jsr_X(void)
{ //AD
	temp16=INDADDRESS(PC_REG++);
	S_REG--;
	MemWrite8(pc_s.B.lsb,S_REG--);
	MemWrite8(pc_s.B.msb,S_REG);
	PC_REG=temp16;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Ldx_X(void)
{ //AE
	X_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Stx_X(void)
{ //AF
	MemWrite16(X_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Suba_E(void)
{ //B0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = A_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpa_E(void)
{ //B1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= A_REG-postbyte;
	cc_s[C] = temp8 > A_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,A_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Sbca_E(void)
{ //B2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Subd_E(void)
{ //B3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32=D_REG-temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Anda_E(void)
{ //B4
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	A_REG = A_REG & postbyte;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Bita_E(void)
{ //B5
	temp8 = A_REG & MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Lda_E(void)
{ //B6
	A_REG= MemRead8(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Sta_E(void)
{ //B7
	MemWrite8(A_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(A_REG);
	cc_s[N] = NTEST8(A_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Eora_E(void)
{  //B8
	A_REG = A_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Adca_E(void)
{ //B9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= A_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	cc_s[H] = ((A_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	A_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ora_E(void)
{ //BA
	A_REG = A_REG | MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Adda_E(void)
{ //BB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=A_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((A_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,A_REG);
	A_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(A_REG);
	cc_s[Z] = ZTEST(A_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpx_E(void)
{ //BC
	postword=MemRead16(IMMADDRESS(PC_REG));
	temp16 = X_REG-postword;
	cc_s[C] = temp16 > X_REG;
	cc_s[V] = OVERFLOW16(cc_s[C],postword,temp16,X_REG);
	cc_s[N] = NTEST16(temp16);
	cc_s[Z] = ZTEST(temp16);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M75];
}

static void Bsr_E(void)
{ //BD
	postword=IMMADDRESS(PC_REG);
	PC_REG+=2;
	S_REG--;
	MemWrite8(pc_s.B.lsb,S_REG--);
	MemWrite8(pc_s.B.msb,S_REG);
	PC_REG=postword;
	CycleCounter+=InsCycles[MD_NATIVE6309][M87];
}

static void Ldx_E(void)
{ //BE
	X_REG=MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Stx_E(void)
{ //BF
	MemWrite16(X_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(X_REG);
	cc_s[N] = NTEST16(X_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Subb_M(void)
{ //C0
	postbyte=MemRead8(PC_REG++);
	temp16 = B_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=2;
}

static void Cmpb_M(void)
{ //C1
	postbyte=MemRead8(PC_REG++);
	temp8= B_REG-postbyte;
	cc_s[C] = temp8 > B_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,B_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=2;
}

static void Sbcb_M(void)
{ //C2
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

static void Addd_M(void)
{ //C3
	temp16=IMMADDRESS(PC_REG);
	temp32= D_REG+ temp16;
	cc_s[C] = (temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Andb_M(void)
{ //C4 LOOK
	B_REG = B_REG & MemRead8(PC_REG++);
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Bitb_M(void)
{ //C5
	temp8 = B_REG & MemRead8(PC_REG++);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Ldb_M(void)
{ //C6
	B_REG=MemRead8(PC_REG++);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Eorb_M(void)
{ //C8
	B_REG = B_REG ^ MemRead8(PC_REG++);
	cc_s[N] =NTEST8(B_REG);
	cc_s[Z] =ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Adcb_M(void)
{ //C9
	postbyte=MemRead8(PC_REG++);
	temp16= B_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	cc_s[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

static void Orb_M(void)
{ //CA
	B_REG= B_REG | MemRead8(PC_REG++);
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=2;
}

static void Addb_M(void)
{ //CB
	postbyte=MemRead8(PC_REG++);
	temp16=B_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=2;
}

static void Ldd_M(void)
{ //CC
	D_REG=IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

static void Ldq_M(void)
{ //CD 6309 WORK
	Q_REG = MemRead32_s(PC_REG);
	PC_REG+=4;
	cc_s[Z] = ZTEST(Q_REG);
	cc_s[N] = NTEST32(Q_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Ldu_M(void)
{ //CE
	U_REG = IMMADDRESS(PC_REG);
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=3;
}

static void Subb_D(void)
{ //D0
	postbyte=MemRead8( DPADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc_s[C] =(temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Cmpb_D(void)
{ //D1
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp8= B_REG-postbyte;
	cc_s[C] = temp8 > B_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,B_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Sbcb_D(void)
{ //D2
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Addd_D(void)
{ //D3
	temp16=MemRead16( DPADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M64];
}

static void Andb_D(void)
{ //D4 
	B_REG = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] =NTEST8(B_REG);
	cc_s[Z] =ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Bitb_D(void)
{ //D5
	temp8 = B_REG & MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] =ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Ldb_D(void)
{ //D6
	B_REG= MemRead8( DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Stb_D(void)
{ //D7
	MemWrite8( B_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Eorb_D(void)
{ //D8	
	B_REG = B_REG ^ MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Adcb_D(void)
{ //D9
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	cc_s[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Orb_D(void)
{ //DA
	B_REG = B_REG | MemRead8(DPADDRESS(PC_REG++));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Addb_D(void)
{ //DB
	postbyte=MemRead8(DPADDRESS(PC_REG++));
	temp16= B_REG+postbyte;
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M43];
}

static void Ldd_D(void)
{ //DC
	D_REG = MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Std_D(void)
{ //DD
	MemWrite16(D_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ldu_D(void)
{ //DE
	U_REG = MemRead16(DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Stu_D(void)
{ //DF
	MemWrite16(U_REG,DPADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Subb_X(void)
{ //E0
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16 = B_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	CycleCounter+=4;
}

static void Cmpb_X(void)
{ //E1
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp8 = B_REG-postbyte;
	cc_s[C] = temp8 > B_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,B_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	CycleCounter+=4;
}

static void Sbcb_X(void)
{ //E2
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

static void Addd_X(void)
{ //E3 
	temp16=MemRead16(INDADDRESS(PC_REG++));
	temp32= D_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG= temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Andb_X(void)
{ //E4
	B_REG = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Bitb_X(void)
{ //E5 
	temp8 = B_REG & MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Ldb_X(void)
{ //E6
	B_REG=MemRead8(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Stb_X(void)
{ //E7
	MemWrite8(B_REG,CalculateEA( MemRead8(PC_REG++)));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Eorb_X(void)
{ //E8
	B_REG= B_REG ^ MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Adcb_X(void)
{ //E9
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16= B_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	cc_s[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

static void Orb_X(void)
{ //EA 
	B_REG = B_REG | MemRead8(INDADDRESS(PC_REG++));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	CycleCounter+=4;
}

static void Addb_X(void)
{ //EB
	postbyte=MemRead8(INDADDRESS(PC_REG++));
	temp16=B_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	CycleCounter+=4;
}

static void Ldd_X(void)
{ //EC
	D_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Std_X(void)
{ //ED
	MemWrite16(D_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Ldu_X(void)
{ //EE
	U_REG=MemRead16(INDADDRESS(PC_REG++));
	cc_s[Z] =ZTEST(U_REG);
	cc_s[N] =NTEST16(U_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Stu_X(void)
{ //EF
	MemWrite16(U_REG,INDADDRESS(PC_REG++));
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	CycleCounter+=5;
}

static void Subb_E(void)
{ //F0
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16 = B_REG - postbyte;
	cc_s[C] = (temp16 & 0x100)>>8; 
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Cmpb_E(void)
{ //F1
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp8= B_REG-postbyte;
	cc_s[C] = temp8 > B_REG;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp8,B_REG);
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Sbcb_E(void)
{ //F2
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG-postbyte-cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG = (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Addd_E(void)
{ //F3
	temp16=MemRead16(IMMADDRESS(PC_REG));
	temp32= D_REG+ temp16;
	cc_s[C] =(temp32 & 0x10000)>>16;
	cc_s[V] = OVERFLOW16(cc_s[C],temp32,temp16,D_REG);
	D_REG = temp32;
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M76];
}

static void Andb_E(void)
{  //F4
	B_REG = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Bitb_E(void)
{ //F5
	temp8 = B_REG & MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(temp8);
	cc_s[Z] = ZTEST(temp8);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ldb_E(void)
{ //F6
	B_REG=MemRead8(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Stb_E(void)
{ //F7 
	MemWrite8(B_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(B_REG);
	cc_s[N] = NTEST8(B_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Eorb_E(void)
{ //F8
	B_REG = B_REG ^ MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Adcb_E(void)
{ //F9
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16= B_REG + postbyte + cc_s[C];
	cc_s[C] = (temp16 & 0x100)>>8;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	cc_s[H] = ((B_REG ^ temp16 ^ postbyte) & 0x10)>>4;
	B_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Orb_E(void)
{ //FA
	B_REG = B_REG | MemRead8(IMMADDRESS(PC_REG));
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] = ZTEST(B_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Addb_E(void)
{ //FB
	postbyte=MemRead8(IMMADDRESS(PC_REG));
	temp16=B_REG+postbyte;
	cc_s[C] =(temp16 & 0x100)>>8;
	cc_s[H] = ((B_REG ^ postbyte ^ temp16) & 0x10)>>4;
	cc_s[V] = OVERFLOW8(cc_s[C],postbyte,temp16,B_REG);
	B_REG= (unsigned char)temp16;
	cc_s[N] = NTEST8(B_REG);
	cc_s[Z] =ZTEST(B_REG);
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M54];
}

static void Ldd_E(void)
{ //FC
	D_REG=MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Std_E(void)
{ //FD
	MemWrite16(D_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(D_REG);
	cc_s[N] = NTEST16(D_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Ldu_E(void)
{ //FE
	U_REG= MemRead16(IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

static void Stu_E(void)
{ //FF
	MemWrite16(U_REG,IMMADDRESS(PC_REG));
	cc_s[Z] = ZTEST(U_REG);
	cc_s[N] = NTEST16(U_REG);
	cc_s[V] = 0;
	PC_REG+=2;
	CycleCounter+=InsCycles[MD_NATIVE6309][M65];
}

short instcyclemu1[256] = 
{
	6, // Neg_D 00
	6, // Oim_D 01
	6, // Aim_D 02
	6, // Com_D 03
	6, // Lsr_D 04
	6, // Eim_D 05
	6, // Ror_D 06
	6, // Asr_D 07
	6, // Asl_D 08
	6, // Rol_D 09
	6, // Dec_D 0A
	6, // Tim_D 0B
	6, // Inc_D 0C
	6, // Tst_D 0D
	3, // Jmp_D 0E
	6, // Clr_D 0F
	0, // Page2 10
	0, // Page3 11
	2, // Nop 12
	0, // Sync 13
	4, // Sexw 14
	20, // Invalid 15
	5, // Lbra 16
	9, // Lbsr 17
	19, // Invalid 18
	2, // Daa 19
	3, // Orcc 1A
	19, // Invalid 1B
	3, // Andcc 1C
	2, // Sex 1D
	8, // Exg 1E
	6, // Tfr 1F
	3, // Bra 20
	3, // Brn 21
	3, // Bhi 22
	3, // Bls 23
	3, // Bhs 24
	3, // Blo 25
	3, // Bne 26
	3, // Beq 27
	3, // Bvc 28
	3, // Bvs 29
	3, // Bpl 2A
	3, // Bmi 2B
	3, // Bge 2C
	3, // Blt 2D
	3, // Bgt 2E
	3, // Ble 2F
	4, // Leax 30
	4, // Leay 31
	4, // Leas 32
	4, // Leau 33
	5, // Pshs 34
	5, // Puls 35
	5, // Pshu 36
	5, // Puls 37
	19, // Invalid 38
	5, // Rts 39
	3, // Abx 3A
	6, // Rti 3B
	22, // Cwai 3C
	11, // Mul_I 3D
	19, // Invalid 3E
	19, // Swi1 3F
	2, // Nega_I 40
	19, // Invalid 41
	19, // Invalid 42
	2, // Coma_I 43
	2, // Lsra_I 44
	19, // Invalid 45
	2, // Rora_I 46
	2, // Asra_I 47
	2, // Asla_I 48
	2, // Rola_I 49
	2, // Deca_I 4A
	19, // Invalid 4B
	2, // Inca_I 4C
	2, // Tsta_I 4D
	19, // Invalid 4E
	2, // Clra_I 4F
	2, // Negb_I 50
	19, // Invalid 51
	19, // Invalid 52
	2, // Comb_I 53
	2, // Lsrb_I 54
	19, // Invalid 55
	2, // Rorb_I 56
	2, // Asrb_I 57
	2, // Aslb_I 58
	2, // Rolb_I 59
	2, // Decb_I 5A
	19, // Invalid 5B
	2, // Incb_I 5C
	2, // Tstb_I 5D
	19, // Invalid 5E
	2, // Clrb_I 5F
	6, // Neg_X 60
	7, // Oim_X 61
	7, // Aim_X 62
	6, // Com_X 63
	6, // Lsr_X 64
	7, // Eim_X 65
	6, // Ror_X 66
	6, // Asr_X 67
	6, // Asl_X 68
	6, // Rol_X 69
	6, // Dec_X 6A
	7, // Tim_X 6B
	6, // Inc_X 6C
	6, // Tst_X 6D
	3, // Jmp_X 6E
	6, // Clr_X 6F
	7, // Neg_E 70
	7, // Oim_E 71
	7, // Aim_E 72
	7, // Com_E 73
	7, // Lsr_E 74
	7, // Eim_E 75
	7, // Ror_E 76
	7, // Asr_E 77 
	7, // Asl_E 78
	7, // Rol_E 79
	7, // Dec_E 7A
	7, // Tim_E 7B
	7, // Inc_E 7C
	7, // Tst_E 7D
	4, // Jmp_E 7E
	7, // Clr_E 7F
	2, // Suba_M 80
	2, // Cmpa_M 81
	2, // Sbca_M 82
	4, // Subd_M 83
	2, // Anda_M 84
	2, // Bita_M 85
	2, // Lda_M 86
	19, // Invalid 87
	2, // Eora_M 88
	2, // Adca_M 89
	2, // Ora_M 8A
	2, // Adda_M 8B
	4, // Cmpx_M 8C
	7, // Bsr 8D
	3, // Ldx_M 8E
	19, // Invalid 8F
	4, // Suba_D 90
	4, // Cmpa_D 91
	4, // Sbca_D 92
	6, // Subd_D 93
	4, // Anda_D 94
	4, // Bita_D 95
	4, // Lda_D 96
	4, // Sta_D 97
	4, // Eora_D 98
	4, // Adca_D 99
	4, // Ora_D 9A
	4, // Adda_D 9B
	6, // Cmpx_D 9C
	7, // Jsr_D 9D
	5, // Ldx_D 9E
	5, // Stx_D 9F
	4, // Suba_X A0
	4, // Cmpa_X A1
	4, // Sbca_X A2
	6, // Subd_X A3
	4, // Anda_X A4
	4, // Bita_X A5
	4, // Lda_X A6
	4, // Sta_X A7
	4, // Eora_X A8
	4, // Adca_X A9
	4, // Ora_X AA
	4, // Adda_X AB
	6, // Cmpx_X AC
	7, // Jsr_X AD
	5, // Ldx_X AE
	5, // Stx_X AF
	5, // Suba_E B0
	5, // Cmpa_E B1
	5, // Sbca_E B2
	6, // Subd_E B3
	5, // Anda_E B4
	5, // Bita_E B5
	5, // Lda_E B6
	5, // Sta_E B7
	5, // Eora_E B8
	5, // Adca_E B9
	5, // Ora_E BA
	5, // Adda_E BB
	7, // Cmpx_E BC
	8, // Jsr_E BD
	6, // Ldx_E BE
	6, // Stx_E BF
	2, // Subb_M C0
	2, // Cmpb_M C1
	2, // Sbcb_M C2
	4, // Addd_M C3
	2, // Andb_M C4
	2, // Bitb_M C5
	2, // Ldb_M C6
	19, // Invalid C7
	2, // Eorb_M C8
	2, // Adcb_M C9
	2, // Orb_M CA
	2, // Addb_M CB
	3, // Ldd_M CC
	5, // Ldq_M CD
	3, // Ldu_M CE
	19, // Invalid CF
	4, // Subb_D D0
	4, // Cmpb_D D1
	4, // Sbcb_D D2
	6, // Addd_D D3
	4, // Andb_D D4
	4, // Bitb_D D5
	4, // Ldb_D D6
	4, // Stb_D D7
	4, // Eorb_D D8
	4, // Adcb_D D9
	4, // Orb_D DA
	4, // Addb_D DB
	5, // Ldd_D DC
	5, // Std_D DD
	5, // Ldu_D DE
	5, // Stu_D DF
	4, // Subb_X E0
	4, // Cmpb_X E1
	4, // Sbcb_X E2
	6, // Addd_X E3
	4, // Andb_X E4
	4, // Bitb_X E5
	4, // Ldb_X E6
	4, // Stb_X E7
	4, // Eorb_X E8
	4, // Adcb_X E9
	4, // Orb_X EA
	4, // Addb_X EB
	5, // Ldd_X EC
	5, // Std_X ED
	5, // Ldu_X EE
	5, // Stu_X EF
	5, // Subb_E F0
	5, // Cmpb_E F1
	5, // Sbcb_E F2
	7, // Addd_E F3
	5, // Andb_E F4
	5, // Bitb_E F5
	5, // Ldb_E F6
	5, // Stb_E F7
	5, // Eorb_E F8
	5, // Adcb_E F9
	5, // Orb_E FA
	5, // Addb_E FB
	6, // Ldd_E FC
	6, // Std_E FD
	6, // Ldu_E FE
	6 // Stu_E FF
};

short instcyclnat1[256] =
{
	5, // Neg_D 00
	6, // Oim_D 01
	6, // Aim_D 02
	5, // Com_D 03
	5, // Lsr_D 04
	6, // Eim_D 05
	5, // Ror_D 06
	5, // Asr_D 07
	5, // Asl_D 08
	5, // Rol_D 09
	5, // Dec_D 0A
	6, // Tim_D 0B
	5, // Inc_D 0C
	4, // Tst_D 0D
	2, // Jmp_D 0E
	5, // Clr_D 0F
	0, // Page2 10
	0, // Page3 11
	1, // Nop 12
	0, // Sync 13
	4, // Sexw 14
	20, // Invalid 15
	4, // Lbra 16
	7, // Lbsr 17
	20, // Invalid 18
	1, // Daa 19
	3, // Orcc 1A
	20, // Invalid 1B
	3, // Andcc 1C
	1, // Sex 1D
	5, // Exg 1E
	4, // Tfr 1F
	3, // Bra 20
	3, // Brn 21
	3, // Bhi 22
	3, // Bls 23
	3, // Bhs 24
	3, // Blo 25
	3, // Bne 26
	3, // Beq 27
	3, // Bvc 28
	3, // Bvs 29
	3, // Bpl 2A
	3, // Bmi 2B
	3, // Bge 2C
	3, // Blt 2D
	3, // Bgt 2E
	3, // Ble 2F
	4, // Leax 30
	4, // Leay 31
	4, // Leas 32
	4, // Leau 33
	4, // Pshs 34
	4, // Puls 35
	4, // Pshu 36
	4, // Puls 37
	20, // Invalid 38
	4, // Rts 39
	1, // Abx 3A
	6, // Rti 3B
	20, // Cwai 3C
	10, // Mul_I 3D
	20, // Invalid 3E
	20, // Swi1 3F
	1, // Nega_I 40
	20, // Invalid 41
	20, // Invalid 42
	1, // Coma_I 43
	1, // Lsra_I 44
	20, // Invalid 45
	1, // Rora_I 46
	1, // Asra_I 47
	1, // Asla_I 48
	1, // Rola_I 49
	1, // Deca_I 4A
	20, // Invalid 4B
	1, // Inca_I 4C
	1, // Tsta_I 4D
	20, // Invalid 4E
	1, // Clra_I 4F
	1, // Negb_I 50
	20, // Invalid 51
	20, // Invalid 52
	1, // Comb_I 53
	1, // Lsrb_I 54
	20, // Invalid 55
	1, // Rorb_I 56
	1, // Asrb_I 57
	1, // Aslb_I 58
	1, // Rolb_I 59
	1, // Decb_I 5A
	20, // Invalid 5B
	1, // Incb_I 5C
	1, // Tstb_I 5D
	20, // Invalid 5E
	1, // Clrb_I 5F
	6, // Neg_X 60
	7, // Oim_X 61
	7, // Aim_X 62
	6, // Com_X 63
	6, // Lsr_X 64
	7, // Eim_X 65
	6, // Ror_X 66
	6, // Asr_X 67
	6, // Asl_X 68
	6, // Rol_X 69
	6, // Dec_X 6A
	7, // Tim_X 6B
	6, // Inc_X 6C
	5, // Tst_X 6D
	3, // Jmp_X 6E
	6, // Clr_X 6F
	6, // Neg_X 70
	7, // Oim_E 71
	7, // Aim_E 72
	6, // Com_E 73
	6, // Lsr_E 74
	7, // Eim_E 75
	6, // Ror_E 76
	6, // Asr_E 77
	6, // Asl_E 78
	6, // Rol_E 79
	6, // Dec_E 7A
	7, // Tim_E 7B
	6, // Inc_E 7C
	5, // Tst_E 7D
	3, // Jmp_E 7E
	6, // Clr_E 7F
	2, // Suba_M 80
	2, // Cmpa_M 81
	2, // Sbca_M 82
	3, // Subd_M 83
	2, // Anda_M 84
	2, // Bita_M 85
	2, // Lda_M 86
	20, // Invalid 87
	2, // Eora_M 88
	2, // Adca_M 89
	2, // Ora_M 8A
	2, // Adda_M 8B
	3, // Cmpx_M 8C
	6, // Bsr 8D
	3, // Ldx_M 8E
	20, // Invalid 8F
	3, // Suba_D 90
	3, // Cmpa_D 91
	3, // Sbca_D 92
	4, // Subd_D 93
	3, // Anda_D 94
	3, // Bita_D 95
	3, // Lda_D 96
	3, // Sta_D 97
	3, // Eora_D 98
	3, // Adca_D 99
	3, // Ora_D 9A
	3, // Adda_D 9B
	4, // Cmpx_D 9C
	6, // Jsr_D 9D
	4, // Ldx_D 9E
	4, // Stx_D 9F
	4, // Suba_X A0
	4, // Cmpa_X A1
	4, // Sbca_X A2
	5, // Subd_X A3
	4, // Anda_X A4
	4, // Bita_X A5
	4, // Lda_X A6
	4, // Sta_X A7
	4, // Eora_X A8
	4, // Adca_X A9
	4, // Ora_X AA
	4, // Adda_X AB
	5, // Cmpx_X AC
	6, // Jsr_X AD
	5, // Ldx_X AE
	5, // Stx_X AF
	4, // Suba_E B0
	4, // Cmpa_E B1
	4, // Sbca_E B2
	5, // Subd_E B3
	4, // Anda_E B4
	4, // Bita_E B5
	4, // Lda_E B6
	4, // Sta_E B7
	4, // Eora_E B8
	4, // Adca_E B9
	4, // Ora_E BA
	4, // Adda_E BB
	5, // Cmpx_E BC
	7, // Jsr_E BD
	5, // Ldx_E BE
	5, // Stx_E BF
	2, // Subb_M C0
	2, // Cmpb_M C1
	2, // Sbcb_M C2
	3, // Addd_M C3
	2, // Andb_M C4
	2, // Bitb_M C5
	2, // Ldb_M C6
	20, // Invalid C7
	2, // Eorb_M C8
	2, // Adcb_M C9
	2, // Orb_M CA
	2, // Addb_M CB
	3, // Ldd_M CC
	5, // Ldq_M CD
	3, // Ldu_M CE
	20, // Invalid CF
	3, // Subb_D D0
	3, // Cmpb_D D1
	3, // Sbcb_D D2
	4, // Addd_D D3
	3, // Andb_D D4
	3, // Bitb_D D5
	3, // Ldb_D D6
	3, // Stb_D D7
	3, // Eorb_D D8
	3, // Adcb_D D9
	3, // Orb_D DA
	3, // Addb_D DB
	4, // Ldd_D DC
	4, // Std_D DD
	4, // Ldu_D DE
	4, // Stu_D DF
	4, // Subb_X E0
	4, // Cmpb_X E1
	4, // Sbcb_X E2
	5, // Addd_X E3
	4, // Andb_X E4
	4, // Bitb_X E5
	4, // Ldb_X E6
	4, // Stb_X E7
	4, // Eorb_X E8
	4, // Adcb_X E9
	4, // Orb_X EA
	4, // Addb_X EB
	5, // Ldd_X EC
	5, // Std_X ED
	5, // Ldu_X EE
	5, // Stu_X EF
	4, // Subb_E F0
	4, // Cmpb_E F1
	4, // Sbcb_E F2
	5, // Addd_E F3
	4, // Andb_E F4
	4, // Bitb_E F5
	4, // Ldb_E F6
	4, // Stb_E F7
	4, // Eorb_E F8
	4, // Adcb_E F9
	4, // Orb_E FA
	4, // Addb_E FB
	5, // Ldd_E FC
	5, // Std_E FD
	5, // Ldu_E FE
	5, // Stu_E FF
};

short instcyclemu2[256] =
{
	19, // Invalid 00
	19, // Invalid 01
	19, // Invalid 02
	19, // Invalid 03
	19, // Invalid 04
	19, // Invalid 05
	19, // Invalid 06
	19, // Invalid 07
	19, // Invalid 08
	19, // Invalid 09
	19, // Invalid 0A
	19, // Invalid 0B
	19, // Invalid 0C
	19, // Invalid 0D
	19, // Invalid 0E
	19, // Invalid 0F
	19, // Invalid 10
	19, // Invalid 11
	19, // Invalid 12
	19, // Invalid 13
	19, // Invalid 14
	19, // Invalid 15
	19, // Invalid 16
	19, // Invalid 17
	19, // Invalid 18
	19, // Invalid 19
	19, // Invalid 1A
	19, // Invalid 1B
	19, // Invalid 1C
	19, // Invalid 1D
	19, // Invalid 1E
	19, // Invalid 1F
	19, // Invalid 20
	5, // Lbrn 21
	5, // Lbhi 22
	5, // Lbls 23
	5, // Lbhs 24
	5, // Lbcs 25
	5, // Lbne 26
	5, // Lbeq 27
	5, // Lbvc 28
	5, // Lbvs 29
	5, // Lbpl 2A
	5, // Lbmi 2B
	5, // Lbge 2C
	5, // Lblt 2D
	5, // Lbgt 2E
	5, // Lble 2F
	4, // Addr 30
	4, // Adcr 31
	4, // Subr 32
	4, // Sbcr 33
	4, // Andr 34
	4, // Orr 35
	4, // Eorr 36
	4, // Cmpr 37
	6, // pshsw 38
	6, // pulsw 39
	6, // pshuw 3A
	6, // puluw 3B
	19, // Invalid 3C
	19, // Invalid 3D
	19, // Invalid 3E
	20, // Swi2 3F
	3, // Negd 40
	19, // Invalid 41
	19, // Invalid 42
	3, // Comd 43
	3, // Lsrd 44
	19, // Invalid 45
	3, // Rord 46
	3, // Asrd 47
	3, // Asld 48
	3, // Rold 49
	3, // Decd 4A
	19, // Invalid 4B
	3, // Incd 4C
	3, // Tstd 4D
	19, // Invalid 4E
	3, // Clrd 4F
	19, // Invalid 50
	19, // Invalid 51
	19, // Invalid 52
	3, // Comw 53
	3, // Lsrw 54
	19, // Invalid 55
	3, // Rorw 56
	19, // Invalid 57
	19, // Invalid 58
	3, // Rolw 59
	3, // Decw 5A
	19, // Invalid 5B
	3, // Incw 5C
	3, // Tstw 5D
	19, // Invalid 5E
	3, // Clrw 5F
	19, // Invalid 60
	19, // Invalid 61
	19, // Invalid 62
	19, // Invalid 63
	19, // Invalid 64
	19, // Invalid 65
	19, // Invalid 66
	19, // Invalid 67
	19, // Invalid 68
	19, // Invalid 69
	19, // Invalid 6A
	19, // Invalid 6B
	19, // Invalid 6C
	19, // Invalid 6D
	19, // Invalid 6E
	19, // Invalid 6F
	19, // Invalid 70
	19, // Invalid 71
	19, // Invalid 72
	19, // Invalid 73
	19, // Invalid 74
	19, // Invalid 75
	19, // Invalid 76
	19, // Invalid 77
	19, // Invalid 78
	19, // Invalid 79
	19, // Invalid 7A
	19, // Invalid 7B
	19, // Invalid 7C
	19, // Invalid 7D
	19, // Invalid 7E
	19, // Invalid 7F
	5, // Subw_M 80
	5, // Cmpw_M 81
	5, // Sbcw_M 82
	5, // Cmpd_M 83
	5, // Andd_M 84
	5, // Bitd_M 85
	4, // Ldw_M 86
	19, // Invalid 87
	5, // Eord_M 88
	5, // Adcd_M 89
	5, // Ord_M 8A
	5, // Addw_M 8B
	5, // Cmpy_M 8C
	19, // Invalid 8D
	4, // Ldy_M 8E
	19, // Invalid 8F
	7, // Subw_D 90
	7, // Cmpw_D 91
	7, // Sbcd_D 92
	7, // Cmpd_D 93
	7, // Andd_D 94
	7, // Bitd_D 95
	6, // Ldw_D 96
	6, // Stw_D 97
	7, // Eord_D 98
	7, // Adcd_D 99
	7, // Ord_D 9A
	7, // Addw_D 9B
	7, // Cmpy_D 9C
	19, // Invalid 9D
	6, // ldy_D 9E
	6, // Sty_D 9F
	7, // Subw_X A0
	7, // Cmpw_X A1
	7, // Sbcd_X A2
	7, // Cmpd_X A3
	7, // Andd_X A4
	7, // Bitd_X A5
	6, // Ldw_X A6
	6, // Stw_X A7
	7, // Eord_X A8
	7, // Adcd_X A9
	7, // Ord_X AA
	7, // Addw_X AB
	7, // Cmpy_X AC
	19, // Invalid AD
	6, // Ldy_X AE
	6, // Sty_X AF
	8, // Subw_E B0
	8, // Cmpw_E B1
	8, // Sbcd_E B2
	8, // Cmpd_E B3
	8, // Andd_E B4
	8, // Bitd_E B5
	7, // Ldw_E B6
	7, // Stw_E B7
	8, // Eord_E B8
	8, // Adcd_E B9
	8, // Ord_E BA
	8, // Addw_E BB
	8, // Cmpy_E BC
	19, // Invalid BD
	7, // Ldy_E BE
	7, // Sty_E BF
	19, // Invalid C0
	19, // Invalid C1
	19, // Invalid C2
	19, // Invalid C3
	19, // Invalid C4
	19, // Invalid C5
	19, // Invalid C6
	19, // Invalid C7
	19, // Invalid C8
	19, // Invalid C9
	19, // Invalid CA
	19, // Invalid CB
	19, // Invalid CC
	19, // Invalid CD
	4, // Lds_M CE
	19, // Invalid CF
	19, // Invalid D0
	19, // Invalid D1
	19, // Invalid D2
	19, // Invalid D3
	19, // Invalid D4
	19, // Invalid D5
	19, // Invalid D6
	19, // Invalid D7
	19, // Invalid D8
	19, // Invalid D9
	19, // Invalid DA
	19, // Invalid DB
	8, // Ldq_D DC
	8, // Stq_D DD
	6, // Lds_D DE
	6, // Sts_D DF
	19, // Invalid E0
	19, // Invalid E1
	19, // Invalid E2
	19, // Invalid E3
	19, // Invalid E4
	19, // Invalid E5
	19, // Invalid E6
	19, // Invalid E7
	19, // Invalid E8
	19, // Invalid E9
	19, // Invalid EA
	19, // Invalid EB
	8, // Ldq_X EC
	8, // Stq_X ED
	6, // Lds_X EE
	6, // Sts_X EF
	19, // Invalid F0
	19, // Invalid F1
	19, // Invalid F2
	19, // Invalid F3
	19, // Invalid F4
	19, // Invalid F5
	19, // Invalid F6
	19, // Invalid F7
	19, // Invalid F8
	19, // Invalid F9
	19, // Invalid FA
	19, // Invalid FB
	9, // Ldq_E FC
	9, // Stq_E FD
	7, // Lds_E FE
	7 // Sts_E FF
};

short instcyclnat2[256] =
{
	20, // Invalid 00
	20, // Invalid 01
	20, // Invalid 02
	20, // Invalid 03
	20, // Invalid 04
	20, // Invalid 05
	20, // Invalid 06
	20, // Invalid 07
	20, // Invalid 08
	20, // Invalid 09
	20, // Invalid 0A
	20, // Invalid 0B
	20, // Invalid 0C
	20, // Invalid 0D
	20, // Invalid 0E
	20, // Invalid 0F
	20, // Invalid 10
	20, // Invalid 11
	20, // Invalid 12
	20, // Invalid 13
	20, // Invalid 14
	20, // Invalid 15
	20, // Invalid 16
	20, // Invalid 17
	20, // Invalid 18
	20, // Invalid 19
	20, // Invalid 1A
	20, // Invalid 1B
	20, // Invalid 1C
	20, // Invalid 1D
	20, // Invalid 1E
	20, // Invalid 1F
	20, // Invalid 20
	5, // Lbrn 21
	5, // Lbhi 22
	5, // Lbls 23
	5, // Lbhs 24
	5, // Lbcs 25
	5, // Lbne 26
	5, // Lbeq 27
	5, // Lbvc 28
	5, // Lbvs 29
	5, // Lbpl 2A
	5, // Lbmi 2B
	5, // Lbge 2C
	5, // Lblt 2D
	5, // Lbgt 2E
	5, // Lble 2F
	4, // Addr 30
	4, // Adcr 31
	4, // Subr 32
	4, // Sbcr 33
	4, // Andr 34
	4, // Orr 35
	4, // Eorr 36
	4, // Cmpr 37
	6, // pshsw 38
	6, // pulsw 39
	6, // pshuw 3A
	6, // puluw 3B
	19, // Invalid 3C
	19, // Invalid 3D
	19, // Invalid 3E
	22, // Swi2 3F
	2, // Negd 40
	20, // Invalid 41
	20, // Invalid 42
	2, // Comd 43
	2, // Lsrd 44
	20, // Invalid 45
	2, // Rord 46
	2, // Asrd 47
	2, // Asld 48
	2, // Rold 49
	2, // Decd 4A
	20, // Invalid 4B
	2, // Incd 4C
	2, // Tstd 4D
	20, // Invalid 4E
	2, // Clrd 4F
	20, // Invalid 50
	20, // Invalid 51
	20, // Invalid 52
	2, // Comw 53
	2, // Lsrw 54
	20, // Invalid 55
	2, // Rorw 56
	20, // Invalid 57
	20, // Invalid 58
	2, // Rolw 59
	2, // Decw 5A
	20, // Invalid 5B
	2, // Incw 5C
	2, // Tstw 5D
	10, // Invalid 5E
	2, // Clrw 5F
	20, // Invalid 60
	20, // Invalid 61
	20, // Invalid 62
	20, // Invalid 63
	20, // Invalid 64
	20, // Invalid 65
	20, // Invalid 66
	20, // Invalid 67
	20, // Invalid 68
	20, // Invalid 69
	20, // Invalid 6A
	20, // Invalid 6B
	20, // Invalid 6C
	20, // Invalid 6D
	20, // Invalid 6E
	20, // Invalid 6F
	20, // Invalid 70
	20, // Invalid 71
	20, // Invalid 72
	20, // Invalid 73
	20, // Invalid 74
	20, // Invalid 75
	20, // Invalid 76
	20, // Invalid 77
	20, // Invalid 78
	20, // Invalid 79
	20, // Invalid 7A
	20, // Invalid 7B
	20, // Invalid 7C
	20, // Invalid 7D
	20, // Invalid 7E
	20, // Invalid 7F
	4, // Subw_M 80
	4, // Cmpw_M 81
	4, // Sbcw_M 82
	4, // Cmpw_M 83
	4, // Andd_M 84
	4, // Bitd_M 85
	4, // Ldw_M 86
	20, // Invalid 87
	4, // Eord_M 88
	4, // Adcd_M 89
	4, // Ord_M 8A
	4, // Addw_M 8B
	4, // Cmpy_M 8C
	20, // Invalid 8D
	4, // Ldy_M 8E
	20, // Invalid 8F
	5, // Subw_D 90
	5, // Cmpw_D 91
	5, // Sbcd_D 92
	5, // Cmpd_D 93
	5, // Andd_D 94
	5, // Bitd_D 95
	5, // Ldw_D 96
	5, // Stw_D 97
	5, // Eord_D 98
	5, // Adcd_D 99
	5, // Ord_D 9A
	5, // Addw_D 9B
	5, // Cmpy_D 9C
	20, // Invalid 9D
	5, // ldy_D 9E
	5, // Sty_D 9F
	6, // Subw_X A0
	6, // Cmpw_X A1
	6, // Sbcd_X A2
	6, // Cmpd_X A3
	6, // Andd_X A4
	6, // Bitd_X A5
	6, // Ldw_X A6
	6, // Stw_X A7
	6, // Eord_X A8
	6, // Adcd_X A9
	6, // Ord_X AA
	6, // Addw_X AB
	6, // Cmpy_X AC
	20, // Invalid AD
	6, // Ldy_X AE
	6, // Sty_X AF
	6, // Subw_E B0
	6, // Cmpw_E B1
	6, // Sbcd_E B2
	6, // Cmpd_E B3
	6, // Andd_E B4
	6, // Bitd_E B5
	6, // Ldw_E B6
	6, // Stw_E B7
	6, // Eord_E B8
	6, // Adcd_E B9
	6, // Ord_E BA
	6, // Addw_E BB
	6, // Cmpy_E BC
	20, // Invalid BD
	6, // Ldy_E BE
	6, // Sty_E BF
	20, // Invalid C0
	20, // Invalid C1
	20, // Invalid C2
	20, // Invalid C3
	20, // Invalid C4
	20, // Invalid C5
	20, // Invalid C6
	20, // Invalid C7
	20, // Invalid C8
	20, // Invalid C9
	20, // Invalid CA
	20, // Invalid CB
	20, // Invalid CC
	20, // Invalid CD
	4, // Lds_M CE
	20, // Invalid D0
	20, // Invalid D1
	20, // Invalid D2
	20, // Invalid D3
	20, // Invalid D4
	20, // Invalid D5
	20, // Invalid D6
	20, // Invalid D7
	20, // Invalid D8
	20, // Invalid D9
	20, // Invalid DA
	20, // Invalid DB
	7, // Ldq_D DC
	7, // Stq_D DD
	5, // Lds_D DE
	5, // Sts_D DF
	20, // Invalid E0
	20, // Invalid E1
	20, // Invalid E2
	20, // Invalid E3
	20, // Invalid E4
	20, // Invalid E5
	20, // Invalid E6
	20, // Invalid E7
	20, // Invalid E8
	20, // Invalid E9
	20, // Invalid EA
	20, // Invalid EB
	8, // Ldq_X EC
	8, // Stq_X ED
	6, // Lds_X EE
	6, // Sts_X EF
	20, // Invalid F0
	20, // Invalid F1
	20, // Invalid F2
	20, // Invalid F3
	20, // Invalid F4
	20, // Invalid F5
	20, // Invalid F6
	20, // Invalid F7
	20, // Invalid F8
	20, // Invalid F9
	20, // Invalid FA
	20, // Invalid FB
	8, // Ldq_E FC
	8, // Stq_E FD
	6, // Lds_E FE
	6 // Sts_E FF
};

short instcyclemu3[256] =
{
	19, // Invalid 00
	19, // Invalid 01
	19, // Invalid 02
	19, // Invalid 03
	19, // Invalid 04
	19, // Invalid 05
	19, // Invalid 06
	19, // Invalid 07
	19, // Invalid 08
	19, // Invalid 09
	19, // Invalid 0A
	19, // Invalid 0B
	19, // Invalid 0C
	19, // Invalid 0D
	19, // Invalid 0E
	19, // Invalid 0F
	19, // Invalid 10
	19, // Invalid 11
	19, // Invalid 12
	19, // Invalid 13
	19, // Invalid 14
	19, // Invalid 15
	19, // Invalid 16
	19, // Invalid 17
	19, // Invalid 18
	19, // Invalid 19
	19, // Invalid 1A
	19, // Invalid 1B
	19, // Invalid 1C
	19, // Invalid 1D
	19, // Invalid 1E
	19, // Invalid 1F
	19, // Invalid 20
	19, // Invalid 21
	19, // Invalid 22
	19, // Invalid 23
	19, // Invalid 24
	19, // Invalid 25
	19, // Invalid 26
	19, // Invalid 27
	19, // Invalid 28
	19, // Invalid 29
	19, // Invalid 2A
	19, // Invalid 2B
	19, // Invalid 2C
	19, // Invalid 2D
	19, // Invalid 2E
	19, // Invalid 2F
	7, // Band 30
	7, // Biand 31
	7, // Bor 32
	7, // Bior 33
	7, // Beor 34
	7, // Bieor 35
	7, // Ldbt 36
	8, // Stbt 37
	6, // Tfm1 38
	6, // Tfm2 39
	6, // Tfm3 3A
	6, // Tfm4 3B
	4, // Bitmd 3C
	5, // ldmd 3D
	19, // Invalid 3E
	20, // Swi3 3F
	19, // Invalid 40
	19, // Invalid 41
	19, // Invalid 42
	3, // Come_I 43
	19, // Invalid 44
	19, // Invalid 45
	19, // Invalid 46
	19, // Invalid 47
	19, // Invalid 48
	19, // Invalid 49
	3, // Dece_I 4A
	19, // Invalid 4B
	3, // Ince_I 4C
	3, // Tste_I 4D
	19, // Invalid 4E
	3, // Clre_I 4F
	19, // Invalid 50
	19, // Invalid 51
	19, // Invalid 52
	3, // Comf_I 53
	19, // Invalid 54
	19, // Invalid 55
	19, // Invalid 56
	19, // Invalid 57
	19, // Invalid 58
	19, // Invalid 59
	3, // Decf_I 5A
	19, // Invalid 5B
	3, // Incf_I 5C
	3, // Tstf_I 5D
	19, // Invalid 5E
	3, // Clrf_I 5F
	19, // Invalid 60
	19, // Invalid 61
	19, // Invalid 62
	19, // Invalid 63
	19, // Invalid 64
	19, // Invalid 65
	19, // Invalid 66
	19, // Invalid 67
	19, // Invalid 68
	19, // Invalid 69
	19, // Invalid 6A
	19, // Invalid 6B
	19, // Invalid 6C
	19, // Invalid 6D
	19, // Invalid 6E
	19, // Invalid 6F
	19, // Invalid 70
	19, // Invalid 71
	19, // Invalid 72
	19, // Invalid 73
	19, // Invalid 74
	19, // Invalid 75
	19, // Invalid 76
	19, // Invalid 77
	19, // Invalid 78
	19, // Invalid 79
	19, // Invalid 7A
	19, // Invalid 7B
	19, // Invalid 7C
	19, // Invalid 7D
	19, // Invalid 7E
	19, // Invalid 7F
	3, // Sube_M 80
	3, // Cmpe_M 81
	19, // Invalid 82
	5, // Cmpu_M 83
	19, // Invalid 84
	19, // Invalid 85
	3, // Lde_M 86
	19, // Invalid 87
	19, // Invalid 88
	19, // Invalid 89
	19, // Invalid 8A
	3, // Adde_M 8B
	5, // Cmps_M 8C
	0, // Divd_M 8D + does own cycles
	0, // Divq_M 8E + does own cycles
	28, // Muld 8F
	5, // Sube_D 90
	5, // Cmpe_D 91
	19, // Invalid 92
	7, // Cmpu_D 93
	19, // Invalid 94
	19, // Invalid 95
	5, // Lde_D 96
	5, // Ste_D 97
	19, // Invalid 98
	19, // Invalid 99
	19, // Invalid 9A
	5, // Adde_D 9B
	7, // Cmps_D 9C
	0, // Divd_D 9D + does own cycles
	0, // Divq_D 9E + does own cycles
	30, // Muld_D 9F
	5, // Sube_X A0
	5, // Cmpe_X A1
	19, // Invalid A2
	7, // Cmpu_X A3
	19, // Invalid A4
	19, // Invalid A5
	5, // lde_X A6
	5, // Ste_X A7
	19, // Invalid A8
	19, // Invalid A9
	19, // Invalid AA
	5, // Adde_X AB
	7, // Cmps_X AC
	0, // Divd_D AD + does own cycles
	0, // Divq_D AE + does own cycles
	30, // Muld_D AF
	6, // Sube_E B0
	6, // Cmpe_E B1
	19, // Invalid B2
	8, // Cmpu_E B3
	19, // Invalid B4
	19, // Invalid B5
	6, // Lde_E B6
	6, // Ste_E B7
	19, // Invalid B8
	19, // Invalid B9
	19, // Invalid BA
	6, // Adde_E BB
	6, // Cmpe_E BC
	0, // Divd_D BD + does own cycles
	0, // Divq_D BE + does own cycles
	31, // Muld_D BF
	3, // Subf_M C0
	3, // Cmpf_M C1
	19, // Invalid C2
	19, // Invalid C3
	19, // Invalid C4
	19, // Invalid C5
	3, // Ldf_M C6
	19, // Invalid C7
	19, // Invalid C8
	19, // Invalid C9
	19, // Invalid CA
	3, // Addf_M CB
	19, // Invalid CC
	19, // Invalid CD
	19, // Invalid CE
	19, // Invalid CF
	5, // Subf_D D0
	5, // Cmpf_D D1
	19, // Invalid D2
	19, // Invalid D3
	19, // Invalid D4
	19, // Invalid D5
	5, // Ldf_D D6
	5, // Stf_D D7
	19, // Invalid D8
	19, // Invalid D9
	19, // Invalid DA
	5, // Addf_D DB
	19, // Invalid DC
	19, // Invalid DD
	19, // Invalid DE
	19, // Invalid DF
	5, // Subf_X E0
	5, // Cmpf_X E1
	19, // Invalid E2
	19, // Invalid E3
	19, // Invalid E4
	19, // Invalid E5
	5, // Ldf_X E6
	5, // Stf_X E7
	19, // Invalid E8
	19, // Invalid E9
	19, // Invalid EA
	5, // Addf_X EB
	19, // Invalid EC
	19, // Invalid ED
	19, // Invalid EE
	19, // Invalid EF
	6, // Subf_E F0
	6, // Cmpf_E F1
	19, // Invalid F2
	19, // Invalid F3
	19, // Invalid F4
	19, // Invalid F5
	6, // Ldf_E F6
	6, // Stf_E F7
	19, // Invalid F8
	19, // Invalid F9
	19, // Invalid FA
	6, // Addf_E FB
	19, // Invalid FC
	19, // Invalid FD
	19, // Invalid FE
	19, // Invalid FF
};

short instcyclnat3[256] = 
{
	20, // Invalid 00
	20, // Invalid 01
	20, // Invalid 02
	20, // Invalid 03
	20, // Invalid 04
	20, // Invalid 05
	20, // Invalid 06
	20, // Invalid 07
	20, // Invalid 08
	20, // Invalid 09
	20, // Invalid 0A
	20, // Invalid 0B
	20, // Invalid 0C
	20, // Invalid 0D
	20, // Invalid 0E
	20, // Invalid 0F
	20, // Invalid 10
	20, // Invalid 11
	20, // Invalid 12
	20, // Invalid 13
	20, // Invalid 14
	20, // Invalid 15
	20, // Invalid 16
	20, // Invalid 17
	20, // Invalid 18
	20, // Invalid 19
	20, // Invalid 1A
	20, // Invalid 1B
	20, // Invalid 1C
	20, // Invalid 1D
	20, // Invalid 1E
	20, // Invalid 1F
	20, // Invalid 20
	20, // Invalid 21
	20, // Invalid 22
	20, // Invalid 23
	20, // Invalid 24
	20, // Invalid 25
	20, // Invalid 26
	20, // Invalid 27
	20, // Invalid 28
	20, // Invalid 29
	20, // Invalid 2A
	20, // Invalid 2B
	20, // Invalid 2C
	20, // Invalid 2D
	20, // Invalid 2E
	20, // Invalid 2F
	6, // Band 30
	6, // Biand 31
	6, // Bor 32
	6, // Bior 33
	6, // Beor 34
	6, // Bieor 35
	6, // Ldbt 36
	7, // Stbt 37
	6, // Tfm1 38
	6, // Tfm2 39
	6, // Tfm3 3A
	6, // Tfm4 3B
	4, // Bitmd 3C
	5, // ldmd 3D
	20, // Invalid 3E
	22, // Swi3 3F
	20, // Invalid 40
	20, // Invalid 41
	20, // Invalid 42
	2, // Come_I 43
	20, // Invalid 44
	20, // Invalid 45
	20, // Invalid 46
	20, // Invalid 47
	20, // Invalid 48
	20, // Invalid 49
	2, // Dece_I 4A
	20, // Invalid 4B
	2, // Ince_I 4C
	2, // Tste_I 4D
	20, // Invalid 4E
	2, // Clrf_I 4F
	20, // Invalid 50
	20, // Invalid 51
	20, // Invalid 52
	2, // Comf_I 53
	20, // Invalid 54
	20, // Invalid 55
	20, // Invalid 56
	20, // Invalid 57
	20, // Invalid 58
	20, // Invalid 59
	2, // Decf_I 5A
	20, // Invalid 5B
	2, // Incf_I 5C
	2, // Tstf_I 5D
	20, // Invalid 5E
	2, // Clrf_I 5F
	20, //Invalid 60
	20, //Invalid 61
	20, //Invalid 62
	20, //Invalid 63
	20, //Invalid 64
	20, //Invalid 65
	20, //Invalid 66
	20, //Invalid 67
	20, //Invalid 68
	20, //Invalid 69
	20, //Invalid 6A
	20, //Invalid 6B
	20, //Invalid 6C
	20, //Invalid 6D
	20, //Invalid 6E
	20, //Invalid 6F
	20, //Invalid 70
	20, //Invalid 71
	20, //Invalid 72
	20, //Invalid 73
	20, //Invalid 74
	20, //Invalid 75
	20, //Invalid 76
	20, //Invalid 77
	20, //Invalid 78
	20, //Invalid 79
	20, //Invalid 7A
	20, //Invalid 7B
	20, //Invalid 7C
	20, //Invalid 7D
	20, //Invalid 7E
	20, //Invalid 7F
	3, // Sube_M 80
	3, // Cmpe_M 81
	20, //Invalid 82
	4, // Cmpu_M 83
	20, //Invalid 84
	20, //Invalid 85
	3, // Lde_M 86
	20, //Invalid 87
	20, //Invalid 88
	20, //Invalid 89
	20, //Invalid 8A
	3, // Adde_M 8B
	4, // Cmps_M 8C
	0, // Divd_M 8D + does own cycles
	0, // Divq_M 8E + does own cycles
	28, // Muld 8F
	4, // Sube_D 90
	4, // Cmpe_D 91
	20, //Invalid 92
	4, // Cmpu_D 93
	20, //Invalid 94
	20, //Invalid 95
	4, // Lde_D 96
	4, // Ste_D 97
	20, //Invalid 98
	20, //Invalid 99
	20, //Invalid 9A
	4, // Adde_D 9B
	5, // Cmps_D 9C
	0, // Divd_D 9D + does own cycles
	0, // Divq_D 9E + does own cycles
	29, // Muld_D 9F
	5, // Sube_X A0
	5, // Cmpe_X A1
	20, //Invalid A2
	6, // Cmpu_X A3
	20, //Invalid A4
	20, //Invalid A5
	5, // lde_X A6
	5, // Ste_X A7
	20, //Invalid A8
	20, //Invalid A9
	20, //Invalid AA
	5, // Adde_X AB
	6, // Cmps_X AC
	0, // Divd_D AD + does own cycles
	0, // Divq_D AE + does own cycles
	30, // Muld_D AF
	5, // Sube_E B0
	5, // Cmpe_E B1
	20, //Invalid B2
	6, // Cmpu_E B3
	20, //Invalid B4
	20, //Invalid B5
	5, // Lde_E B6
	5, // Ste_E B7
	20, //Invalid B8
	20, //Invalid B9
	20, //Invalid BA
	5, // Adde_E BB
	5, // Cmpe_E BC
	0, // Divd_D BD + does own cycles
	0, // Divq_D BE + does own cycles
	30, // Muld_D BF
	3, // Subf_M C0
	3, // Cmpf_M C1
	20, // Invalid C2
	20, // Invalid C3
	20, // Invalid C4
	20, // Invalid C5
	3, // Ldf_M C6
	20, // Invalid C7
	20, // Invalid C8
	20, // Invalid C9
	20, // Invalid CA
	3, // Addf_M CB
	20, // Invalid CC
	20, // Invalid CD
	20, // Invalid CE
	20, // Invalid CF
	4, // Subf_D D0
	4, // Cmpf_D D1
	20, // Invalid D2
	20, // Invalid D3
	20, // Invalid D4
	20, // Invalid D5
	4, // Ldf_D D6
	4, // Stf_D D7
	20, // Invalid D8
	20, // Invalid D9
	20, // Invalid DA
	4, // Addf_D DB
	20, // Invalid DC
	20, // Invalid DD
	20, // Invalid DE
	20, // Invalid DF
	5, // Subf_X E0
	5, // Cmpf_X E1
	20, // Invalid E2
	20, // Invalid E3
	20, // Invalid E4
	20, // Invalid E5
	5, // Ldf_X E6
	5, // Stf_X E7
	20, // Invalid E8
	20, // Invalid E9
	20, // Invalid EA
	5, // Addf_X EB
	20, // Invalid EC
	20, // Invalid ED
	20, // Invalid EE
	20, // Invalid EF
	5, // Subf_E F0
	5, // Cmpf_E F1
	20, // Invalid F2
	20, // Invalid F3
	20, // Invalid F4
	20, // Invalid F5
	5, // Ldf_E F6
	5, // Stf_E F7
	20, // Invalid F8
	20, // Invalid F9
	20, // Invalid FA
	5, // Addf_E FB
	20, // Invalid FC
	20, // Invalid FD
	20, // Invalid FE
	20, // Invalid FF
};

/*
static void(*JmpVec1[256])(void) = {
	Neg_D_A,		// 00
	Oim_D_A,		// 01
	Aim_D_A,		// 02
	Com_D_A,		// 03
	Lsr_D_A,		// 04
	Eim_D_A,		// 05
	Ror_D_A,		// 06
	Asr_D_A,		// 07
	Asl_D_A,		// 08
	Rol_D_A,		// 09
	Dec_D,		// 0A
	Tim_D,		// 0B
	Inc_D,		// 0C
	Tst_D,		// 0D
	Jmp_D,		// 0E
	Clr_D,		// 0F
	Page_2,		// 10
	Page_3,		// 11
	Nop_I,		// 1
	Sync_I,		// 13
	Sexw_I,		// 14
	InvalidInsHandler_s,	// 15
	Lbra_R,		// 16
	Lbsr_R,		// 17
	InvalidInsHandler_s,	// 18
	Daa_I,		// 19
	Orcc_M,		// 1A
	InvalidInsHandler_s,	// 1B
	Andcc_M,	// 1C
	Sex_I,		// 1D
	Exg_M,		// 1E
	Tfr_M,		// 1F
	Bra_R,		// 20
	Brn_R,		// 21
	Bhi_R,		// 22
	Bls_R,		// 23
	Bhs_R,		// 24
	Blo_R,		// 25
	Bne_R,		// 26
	Beq_R,		// 27
	Bvc_R,		// 28
	Bvs_R,		// 29
	Bpl_R,		// 2A
	Bmi_R,		// 2B
	Bge_R,		// 2C
	Blt_R,		// 2D
	Bgt_R,		// 2E
	Ble_R,		// 2F
	Leax_X,		// 30
	Leay_X,		// 31
	Leas_X,		// 32
	Leau_X,		// 33
	Pshs_M,		// 34
	Puls_M,		// 35
	Pshu_M,		// 36
	Pulu_M,		// 37
	InvalidInsHandler_s,	// 38
	Rts_I,		// 39
	Abx_I,		// 3A
	Rti_I,		// 3B
	Cwai_I,		// 3C
	Mul_I,		// 3D
	Reset,		// 3E
	Swi1_I,		// 3F
	Nega_I,		// 40
	InvalidInsHandler_s,	// 41
	InvalidInsHandler_s,	// 42
	Coma_I,		// 43
	Lsra_I,		// 44
	InvalidInsHandler_s,	// 45
	Rora_I,		// 46
	Asra_I,		// 47
	Asla_I,		// 48
	Rola_I,		// 49
	Deca_I,		// 4A
	InvalidInsHandler_s,	// 4B
	Inca_I,		// 4C
	Tsta_I,		// 4D
	InvalidInsHandler_s,	// 4E
	Clra_I,		// 4F
	Negb_I,		// 50
	InvalidInsHandler_s,	// 51
	InvalidInsHandler_s,	// 52
	Comb_I,		// 53
	Lsrb_I,		// 54
	InvalidInsHandler_s,	// 55
	Rorb_I,		// 56
	Asrb_I,		// 57
	Aslb_I,		// 58
	Rolb_I,		// 59
	Decb_I,		// 5A
	InvalidInsHandler_s,	// 5B
	Incb_I,		// 5C
	Tstb_I,		// 5D
	InvalidInsHandler_s,	// 5E
	Clrb_I,		// 5F
	Neg_X,		// 60
	Oim_X,		// 61
	Aim_X,		// 62
	Com_X,		// 63
	Lsr_X,		// 64
	Eim_X,		// 65
	Ror_X,		// 66
	Asr_X,		// 67
	Asl_X,		// 68
	Rol_X,		// 69
	Dec_X,		// 6A
	Tim_X,		// 6B
	Inc_X,		// 6C
	Tst_X,		// 6D
	Jmp_X,		// 6E
	Clr_X,		// 6F
	Neg_E,		// 70
	Oim_E,		// 71
	Aim_E,		// 72
	Com_E,		// 73
	Lsr_E,		// 74
	Eim_E,		// 75
	Ror_E,		// 76
	Asr_E,		// 77
	Asl_E,		// 78
	Rol_E,		// 79
	Dec_E,		// 7A
	Tim_E,		// 7B
	Inc_E,		// 7C
	Tst_E,		// 7D
	Jmp_E,		// 7E
	Clr_E,		// 7F
	Suba_M,		// 80
	Cmpa_M,		// 81
	Sbca_M,		// 82
	Subd_M,		// 83
	Anda_M,		// 84
	Bita_M,		// 85
	Lda_M,		// 86
	InvalidInsHandler_s,	// 87
	Eora_M,		// 88
	Adca_M,		// 89
	Ora_M,		// 8A
	Adda_M,		// 8B
	Cmpx_M,		// 8C
	Bsr_R,		// 8D
	Ldx_M,		// 8E
	InvalidInsHandler_s,	// 8F
	Suba_D,		// 90
	Cmpa_D,		// 91
	Scba_D,		// 92
	Subd_D,		// 93
	Anda_D,		// 94
	Bita_D,		// 95
	Lda_D,		// 96
	Sta_D,		// 97
	Eora_D,		// 98
	Adca_D,		// 99
	Ora_D,		// 9A
	Adda_D,		// 9B
	Cmpx_D,		// 9C
	Jsr_D,		// 9D
	Ldx_D,		// 9E
	Stx_D,		// 9A
	Suba_X,		// A0
	Cmpa_X,		// A1
	Sbca_X,		// A2
	Subd_X,		// A3
	Anda_X,		// A4
	Bita_X,		// A5
	Lda_X,		// A6
	Sta_X,		// A7
	Eora_X,		// a8
	Adca_X,		// A9
	Ora_X,		// AA
	Adda_X,		// AB
	Cmpx_X,		// AC
	Jsr_X,		// AD
	Ldx_X,		// AE
	Stx_X,		// AF
	Suba_E,		// B0
	Cmpa_E,		// B1
	Sbca_E,		// B2
	Subd_E,		// B3
	Anda_E,		// B4
	Bita_E,		// B5
	Lda_E,		// B6
	Sta_E,		// B7
	Eora_E,		// B8
	Adca_E,		// B9
	Ora_E,		// BA
	Adda_E,		// BB
	Cmpx_E,		// BC
	Bsr_E,		// BD
	Ldx_E,		// BE
	Stx_E,		// BF
	Subb_M,		// C0
	Cmpb_M,		// C1
	Sbcb_M,		// C2
	Addd_M,		// C3
	Andb_M,		// C4
	Bitb_M,		// C5
	Ldb_M,		// C6
	InvalidInsHandler_s,		// C7
	Eorb_M,		// C8
	Adcb_M,		// C9
	Orb_M,		// CA
	Addb_M,		// CB
	Ldd_M,		// CC
	Ldq_M,		// CD
	Ldu_M,		// CE
	InvalidInsHandler_s,		// CF
	Subb_D,		// D0
	Cmpb_D,		// D1
	Sbcb_D,		// D2
	Addd_D,		// D3
	Andb_D,		// D4
	Bitb_D,		// D5
	Ldb_D,		// D6
	Stb_D,		// D7
	Eorb_D,		// D8
	Adcb_D,		// D9
	Orb_D,		// DA
	Addb_D,		// DB
	Ldd_D,		// DC
	Std_D,		// DD
	Ldu_D,		// DE
	Stu_D,		// DF
	Subb_X,		// E0
	Cmpb_X,		// E1
	Sbcb_X,		// E2
	Addd_X,		// E3
	Andb_X,		// E4
	Bitb_X,		// E5
	Ldb_X,		// E6
	Stb_X,		// E7
	Eorb_X,		// E8
	Adcb_X,		// E9
	Orb_X,		// EA
	Addb_X,		// EB
	Ldd_X,		// EC
	Std_X,		// ED
	Ldu_X,		// EE
	Stu_X,		// EF
	Subb_E,		// F0
	Cmpb_E,		// F1
	Sbcb_E,		// F2
	Addd_E,		// F3
	Andb_E,		// F4
	Bitb_E,		// F5
	Ldb_E,		// F6
	Stb_E,		// F7
	Eorb_E,		// F8
	Adcb_E,		// F9
	Orb_E,		// FA
	Addb_E,		// FB
	Ldd_E,		// FC
	Std_E,		// FD
	Ldu_E,		// FE
	Stu_E,		// FF
};
*/


static void(*JmpVec1[256])(void) = {
	Neg_D_A,		// 00
	Oim_D_A,		// 01
	Aim_D_A,		// 02
	Com_D_A,		// 03
	Lsr_D_A,		// 04
	Eim_D_A,		// 05
	Ror_D_A,		// 06
	Asr_D_A,		// 07
	Asl_D_A,		// 08
	Rol_D_A,		// 09
	Dec_D_A,		// 0A
	Tim_D_A,		// 0B
	Inc_D_A,		// 0C
	Tst_D_A,		// 0D
	Jmp_D_A,		// 0E
	Clr_D_A,		// 0F
	Page_2,		// 10
	Page_3,		// 11
	Nop_I_A,		// 12
	Sync_I_A,		// 13
	Sexw_I_A,		// 14
	InvalidInsHandler_s,	// 15
	Lbra_R_A,		// 16
	Lbsr_R_A,		// 17
	InvalidInsHandler_s,	// 18
	Daa_I_A,		// 19
	Orcc_M_A,		// 1A
	InvalidInsHandler_s,	// 1B
	Andcc_M_A,	// 1C
	Sex_I_A,		// 1D
	Exg_M_A,		// 1E
	Tfr_M_A,		// 1F
	Bra_R_A,		// 20
	Brn_R_A,		// 21
	Bhi_R_A,		// 22
	Bls_R_A,		// 23
	Bhs_R_A,		// 24
	Blo_R_A,		// 25
	Bne_R_A,		// 26
	Beq_R_A,		// 27
	Bvc_R_A,		// 28
	Bvs_R_A,		// 29
	Bpl_R_A,		// 2A
	Bmi_R_A,		// 2B
	Bge_R_A,		// 2C
	Blt_R_A,		// 2D
	Bgt_R_A,		// 2E
	Ble_R_A,		// 2F
	Leax_X_A,		// 30
	Leay_X_A,		// 31
	Leas_X_A,		// 32
	Leau_X_A,		// 33
	Pshs_M_A,		// 34
	Puls_M_A,		// 35
	Pshu_M_A,		// 36
	Pulu_M_A,		// 37
	InvalidInsHandler_s,	// 38
	Rts_I_A,		// 39
	Abx_I_A,		// 3A
	Rti_I_A,		// 3B
	Cwai_I_A,		// 3C
	Mul_I_A,		// 3D
	Reset,		// 3E
	Swi1_I_A,		// 3F
	Nega_I_A,		// 40
	InvalidInsHandler_s,  // 41
	InvalidInsHandler_s,	// 42
	Coma_I_A,		// 43
	Lsra_I_A,		// 44
	InvalidInsHandler_s,	// 45
	Rora_I_A,		// 46
	Asra_I_A,		// 47
	Asla_I_A,		// 48
	Rola_I_A,		// 49
	Deca_I_A,		// 4A
	InvalidInsHandler_s,	// 4B
	Inca_I_A,		// 4C
	Tsta_I_A,		// 4D
	InvalidInsHandler_s,	// 4E
	Clra_I_A,		// 4F
	Negb_I_A,		// 50
	InvalidInsHandler_s,	// 51
	InvalidInsHandler_s,	// 52
	Comb_I_A,		// 53
	Lsrb_I_A,		// 54
	InvalidInsHandler_s,	// 55
	Rorb_I_A,		// 56
	Asrb_I_A,		// 57
	Aslb_I_A,		// 58
	Rolb_I_A,		// 59
	Decb_I_A,		// 5A
	InvalidInsHandler_s,	// 5B
	Incb_I_A,		// 5C
	Tstb_I_A,		// 5D
	InvalidInsHandler_s,	// 5E
	Clrb_I_A,		// 5F
	Neg_X_A,		// 60
	Oim_X_A,		// 61
	Aim_X_A,		// 62
	Com_X_A,		// 63
	Lsr_X_A,		// 64
	Eim_X_A,		// 65
	Ror_X_A,		// 66
	Asr_X_A,		// 67
	Asl_X_A,		// 68
	Rol_X_A,		// 69
	Dec_X_A,		// 6A
	Tim_X_A,		// 6B
	Inc_X_A,		// 6C
	Tst_X_A,		// 6D
	Jmp_X_A,		// 6E
	Clr_X_A,		// 6F
	Neg_E_A,		// 70
	Oim_E_A,		// 71
	Aim_E_A,		// 72
	Com_E_A,		// 73
	Lsr_E_A,		// 74
	Eim_E_A,		// 75
	Ror_E_A,		// 76
	Asr_E_A,		// 77
	Asl_E_A,		// 78
	Rol_E_A,		// 79
	Dec_E_A,		// 7A
	Tim_E_A,		// 7B
	Inc_E_A,		// 7C
	Tst_E_A,		// 7D
	Jmp_E_A,		// 7E
	Clr_E_A,		// 7F
	Suba_M_A,		// 80
	Cmpa_M_A,		// 81
	Sbca_M_A,		// 82
	Subd_M_A,		// 83
	Anda_M_A,		// 84
	Bita_M_A,		// 85
	Lda_M_A,		// 86
	InvalidInsHandler_s,	// 87
	Eora_M_A,		// 88
	Adca_M_A,		// 89
	Ora_M_A,		// 8A
	Adda_M_A,		// 8B
	Cmpx_M_A,		// 8C
	Bsr_R_A,		// 8D
	Ldx_M_A,		// 8E
	InvalidInsHandler_s,	// 8F
	Suba_D_A,		// 90
	Cmpa_D_A,		// 91
	Sbca_D_A,		// 92
	Subd_D_A,		// 93
	Anda_D_A,		// 94
	Bita_D_A,		// 95
	Lda_D_A,		// 96
	Sta_D_A,		// 97
	Eora_D_A,		// 98
	Adca_D_A,		// 99
	Ora_D_A,		// 9A
	Adda_D_A,		// 9B
	Cmpx_D_A,		// 9C
	Jsr_D_A,		// 9D
	Ldx_D_A,		// 9E
	Stx_D_A,		// 9F
	Suba_X_A,		// A0
	Cmpa_X_A,		// A1
	Sbca_X_A,		// A2
	Subd_X_A,		// A3
	Anda_X_A,		// A4
	Bita_X_A,		// A5
	Lda_X_A,		// A6
	Sta_X_A,		// A7
	Eora_X_A,		// A8
	Adca_X_A,		// A9
	Ora_X_A,		// AA
	Adda_X_A,		// AB
	Cmpx_X_A,		// AC
	Jsr_X_A,		// AD
	Ldx_X_A,		// AE
	Stx_X_A,		// AF
	Suba_E_A,		// B0
	Cmpa_E_A,		// B1
	Sbca_E_A,		// B2
	Subd_E_A,		// B3
	Anda_E_A,		// B4
	Bita_E_A,		// B5
	Lda_E_A,		// B6
	Sta_E_A,		// B7
	Eora_E_A,		// B8
	Adca_E_A,		// B9
	Ora_E_A,		// BA
	Adda_E_A,		// BB
	Cmpx_E_A,		// BC
	Jsr_E_A,		// BD
	Ldx_E_A,		// BE
	Stx_E_A,		// BF
	Subb_M_A,		// C0
	Cmpb_M_A,		// C1
	Sbcb_M_A,		// C2
	Addd_M_A,		// C3
	Andb_M_A,		// C4
	Bitb_M_A,		// C5
	Ldb_M_A,		// C6
	InvalidInsHandler_s,	// C7
	Eorb_M_A,		// C8
	Adcb_M_A,		// C9
	Orb_M_A,		// CA
	Addb_M_A,		// CB
	Ldd_M_A,		// CC
	Ldq_M_A,		// CD
	Ldu_M_A,		// CE
	InvalidInsHandler_s,	// CF
	Subb_D_A,		// D0
	Cmpb_D_A,		// D1
	Sbcb_D_A,		// D2
	Addd_D_A,		// D3
	Andb_D_A,		// D4
	Bitb_D_A,		// D5
	Ldb_D_A,		// D6
	Stb_D_A,		// D7
	Eorb_D_A,		// D8
	Adcb_D_A,		// D9
	Orb_D_A,		// DA
	Addb_D_A,		// DB
	Ldd_D_A,		// DC
	Std_D_A,		// DD
	Ldu_D_A,		// DE
	Stu_D_A,		// DF
	Subb_X_A,		// E0
	Cmpb_X_A,		// E1
	Sbcb_X_A,		// E2
	Addd_X_A,		// E3
	Andb_X_A,		// E4
	Bitb_X_A,		// E5
	Ldb_X_A,		// E6
	Stb_X_A,		// E7
	Eorb_X_A,		// E8
	Adcb_X_A,		// E9
	Orb_X_A,		// EA
	Addb_X_A,		// EB
	Ldd_X_A,		// EC
	Std_X_A,		// ED
	Ldu_X_A,		// EE
	Stu_X_A,		// EF
	Subb_E_A,		// F0
	Cmpb_E_A,		// F1
	Sbcb_E_A,		// F2
	Addd_E_A,		// F3
	Andb_E_A,		// F4
	Bitb_E_A,		// F5
	Ldb_E_A,		// F6
	Stb_E_A,		// F7
	Eorb_E_A,		// F8
	Adcb_E_A,		// F9
	Orb_E_A,		// FA
	Addb_E_A,		// FB
	Ldd_E_A,		// FC
	Std_E_A,		// FD
	Ldu_E_A,		// FE
	Stu_E_A,		// FF
};


/*
static void(*JmpVec1_old[256])(void) = {
	Neg_D,		// 00
	Oim_D,		// 01
	Aim_D,		// 02
	Com_D,		// 03
	Lsr_D,		// 04
	Eim_D,		// 05
	Ror_D,		// 06
	Asr_D,		// 07
	Asl_D,		// 08
	Rol_D,		// 09
	Dec_D,		// 0A
	Tim_D,		// 0B
	Inc_D,		// 0C
	Tst_D,		// 0D
	Jmp_D,		// 0E
	Clr_D,		// 0F
	Page_2,		// 10
	Page_3,		// 11
	Nop_I,		// 12
	Sync_I,		// 13
	Sexw_I,		// 14
	InvalidInsHandler_s,	// 15
	Lbra_R,		// 16
	Lbsr_R,		// 17
	InvalidInsHandler_s,	// 18
	Daa_I,		// 19
	Orcc_M,		// 1A
	InvalidInsHandler_s,	// 1B
	Andcc_M,	// 1C
	Sex_I,		// 1D
	Exg_M,		// 1E
	Tfr_M,		// 1F
	Bra_R,		// 20
	Brn_R,		// 21
	Bhi_R,		// 22
	Bls_R,		// 23
	Bhs_R,		// 24
	Blo_R,		// 25
	Bne_R,		// 26
	Beq_R,		// 27
	Bvc_R,		// 28
	Bvs_R,		// 29
	Bpl_R,		// 2A
	Bmi_R,		// 2B
	Bge_R,		// 2C
	Blt_R,		// 2D
	Bgt_R,		// 2E
	Ble_R,		// 2F
	Leax_X,		// 30
	Leay_X,		// 31
	Leas_X,		// 32
	Leau_X,		// 33
	Pshs_M,		// 34
	Puls_M,		// 35
	Pshu_M,		// 36
	Pulu_M,		// 37
	InvalidInsHandler_s,	// 38
	Rts_I,		// 39
	Abx_I,		// 3A
	Rti_I,		// 3B
	Cwai_I,		// 3C
	Mul_I,		// 3D
	Reset,		// 3E
	Swi1_I,		// 3F
	Nega_I,		// 40
	InvalidInsHandler_s,	// 41
	InvalidInsHandler_s,	// 42
	Coma_I,		// 43
	Lsra_I,		// 44
	InvalidInsHandler_s,	// 45
	Rora_I,		// 46
	Asra_I,		// 47
	Asla_I,		// 48
	Rola_I,		// 49
	Deca_I,		// 4A
	InvalidInsHandler_s,	// 4B
	Inca_I,		// 4C
	Tsta_I,		// 4D
	InvalidInsHandler_s,	// 4E
	Clra_I,		// 4F
	Negb_I,		// 50
	InvalidInsHandler_s,	// 51
	InvalidInsHandler_s,	// 52
	Comb_I,		// 53
	Lsrb_I,		// 54
	InvalidInsHandler_s,	// 55
	Rorb_I,		// 56
	Asrb_I,		// 57
	Aslb_I,		// 58
	Rolb_I,		// 59
	Decb_I,		// 5A
	InvalidInsHandler_s,	// 5B
	Incb_I,		// 5C
	Tstb_I,		// 5D
	InvalidInsHandler_s,	// 5E
	Clrb_I,		// 5F
	Neg_X,		// 60
	Oim_X,		// 61
	Aim_X,		// 62
	Com_X,		// 63
	Lsr_X,		// 64
	Eim_X,		// 65
	Ror_X,		// 66
	Asr_X,		// 67
	Asl_X,		// 68
	Rol_X,		// 69
	Dec_X,		// 6A
	Tim_X,		// 6B
	Inc_X,		// 6C
	Tst_X,		// 6D
	Jmp_X,		// 6E
	Clr_X,		// 6F
	Neg_E,		// 70
	Oim_E,		// 71
	Aim_E,		// 72
	Com_E,		// 73
	Lsr_E,		// 74
	Eim_E,		// 75
	Ror_E,		// 76
	Asr_E,		// 77
	Asl_E,		// 78
	Rol_E,		// 79
	Dec_E,		// 7A
	Tim_E,		// 7B
	Inc_E,		// 7C
	Tst_E,		// 7D
	Jmp_E,		// 7E
	Clr_E,		// 7F
	Suba_M,		// 80
	Cmpa_M,		// 81
	Sbca_M,		// 82
	Subd_M,		// 83
	Anda_M,		// 84
	Bita_M,		// 85
	Lda_M,		// 86
	InvalidInsHandler_s,	// 87
	Eora_M,		// 88
	Adca_M,		// 89
	Ora_M,		// 8A
	Adda_M,		// 8B
	Cmpx_M,		// 8C
	Bsr_R,		// 8D
	Ldx_M,		// 8E
	InvalidInsHandler_s,	// 8F
	Suba_D,		// 90
	Cmpa_D,		// 91
	Scba_D,		// 92
	Subd_D,		// 93
	Anda_D,		// 94
	Bita_D,		// 95
	Lda_D,		// 96
	Sta_D,		// 97
	Eora_D,		// 98
	Adca_D,		// 99
	Ora_D,		// 9A
	Adda_D,		// 9B
	Cmpx_D,		// 9C
	Jsr_D,		// 9D
	Ldx_D,		// 9E
	Stx_D,		// 9A
	Suba_X,		// A0
	Cmpa_X,		// A1
	Sbca_X,		// A2
	Subd_X,		// A3
	Anda_X,		// A4
	Bita_X,		// A5
	Lda_X,		// A6
	Sta_X,		// A7
	Eora_X,		// a8
	Adca_X,		// A9
	Ora_X,		// AA
	Adda_X,		// AB
	Cmpx_X,		// AC
	Jsr_X,		// AD
	Ldx_X,		// AE
	Stx_X,		// AF
	Suba_E,		// B0
	Cmpa_E,		// B1
	Sbca_E,		// B2
	Subd_E,		// B3
	Anda_E,		// B4
	Bita_E,		// B5
	Lda_E,		// B6
	Sta_E,		// B7
	Eora_E,		// B8
	Adca_E,		// B9
	Ora_E,		// BA
	Adda_E,		// BB
	Cmpx_E,		// BC
	Bsr_E,		// BD
	Ldx_E,		// BE
	Stx_E,		// BF
	Subb_M,		// C0
	Cmpb_M,		// C1
	Sbcb_M,		// C2
	Addd_M,		// C3
	Andb_M,		// C4
	Bitb_M,		// C5
	Ldb_M,		// C6
	InvalidInsHandler_s,		// C7
	Eorb_M,		// C8
	Adcb_M,		// C9
	Orb_M,		// CA
	Addb_M,		// CB
	Ldd_M,		// CC
	Ldq_M,		// CD
	Ldu_M,		// CE
	InvalidInsHandler_s,		// CF
	Subb_D,		// D0
	Cmpb_D,		// D1
	Sbcb_D,		// D2
	Addd_D,		// D3
	Andb_D,		// D4
	Bitb_D,		// D5
	Ldb_D,		// D6
	Stb_D,		// D7
	Eorb_D,		// D8
	Adcb_D,		// D9
	Orb_D,		// DA
	Addb_D,		// DB
	Ldd_D,		// DC
	Std_D,		// DD
	Ldu_D,		// DE
	Stu_D,		// DF
	Subb_X,		// E0
	Cmpb_X,		// E1
	Sbcb_X,		// E2
	Addd_X,		// E3
	Andb_X,		// E4
	Bitb_X,		// E5
	Ldb_X,		// E6
	Stb_X,		// E7
	Eorb_X,		// E8
	Adcb_X,		// E9
	Orb_X,		// EA
	Addb_X,		// EB
	Ldd_X,		// EC
	Std_X,		// ED
	Ldu_X,		// EE
	Stu_X,		// EF
	Subb_E,		// F0
	Cmpb_E,		// F1
	Sbcb_E,		// F2
	Addd_E,		// F3
	Andb_E,		// F4
	Bitb_E,		// F5
	Ldb_E,		// F6
	Stb_E,		// F7
	Eorb_E,		// F8
	Adcb_E,		// F9
	Orb_E,		// FA
	Addb_E,		// FB
	Ldd_E,		// FC
	Std_E,		// FD
	Ldu_E,		// FE
	Stu_E,		// FF
};
*/

static void(*JmpVec2[256])(void) = {
	InvalidInsHandler_s,		// 00
	InvalidInsHandler_s,		// 01
	InvalidInsHandler_s,		// 02
	InvalidInsHandler_s,		// 03
	InvalidInsHandler_s,		// 04
	InvalidInsHandler_s,		// 05
	InvalidInsHandler_s,		// 06
	InvalidInsHandler_s,		// 07
	InvalidInsHandler_s,		// 08
	InvalidInsHandler_s,		// 09
	InvalidInsHandler_s,		// 0A
	InvalidInsHandler_s,		// 0B
	InvalidInsHandler_s,		// 0C
	InvalidInsHandler_s,		// 0D
	InvalidInsHandler_s,		// 0E
	InvalidInsHandler_s,		// 0F
	InvalidInsHandler_s,		// 10
	InvalidInsHandler_s,		// 11
	InvalidInsHandler_s,		// 12
	InvalidInsHandler_s,		// 13
	InvalidInsHandler_s,		// 14
	InvalidInsHandler_s,		// 15
	InvalidInsHandler_s,		// 16
	InvalidInsHandler_s,		// 17
	InvalidInsHandler_s,		// 18
	InvalidInsHandler_s,		// 19
	InvalidInsHandler_s,		// 1A
	InvalidInsHandler_s,		// 1B
	InvalidInsHandler_s,		// 1C
	InvalidInsHandler_s,		// 1D
	InvalidInsHandler_s,		// 1E
	InvalidInsHandler_s,		// 1F
	InvalidInsHandler_s,		// 20
	LBrn_R,		// 21
	LBhi_R,		// 22
	LBls_R,		// 23
	LBhs_R,		// 24
	LBcs_R,		// 25
	LBne_R,		// 26
	LBeq_R,		// 27
	LBvc_R,		// 28
	LBvs_R,		// 29
	LBpl_R,		// 2A
	LBmi_R,		// 2B
	LBge_R,		// 2C
	LBlt_R,		// 2D
	LBgt_R,		// 2E
	LBle_R,		// 2F
	Addr,		// 30
	Adcr,		// 31
	Subr,		// 32
	Sbcr,		// 33
	Andr,		// 34
	Orr,		// 35
	Eorr,		// 36
	Cmpr,		// 37
	Pshsw,		// 38
	Pulsw,		// 39
	Pshuw,		// 3A
	Puluw,		// 3B
	InvalidInsHandler_s,		// 3C
	InvalidInsHandler_s,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi2_I,		// 3F
	Negd_I,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Comd_I,		// 43
	Lsrd_I,		// 44
	InvalidInsHandler_s,		// 45
	Rord_I,		// 46
	Asrd_I,		// 47
	Asld_I,		// 48
	Rold_I,		// 49
	Decd_I,		// 4A
	InvalidInsHandler_s,		// 4B
	Incd_I,		// 4C
	Tstd_I,		// 4D
	InvalidInsHandler_s,		// 4E
	Clrd_I,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comw_I,		// 53
	Lsrw_I,		// 54
	InvalidInsHandler_s,		// 55
	Rorw_I,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	Rolw_I,		// 59
	Decw_I,		// 5A
	InvalidInsHandler_s,		// 5B
	Incw_I,		// 5C
	Tstw_I,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrw_I,		// 5F
	InvalidInsHandler_s,		// 60
	InvalidInsHandler_s,		// 61
	InvalidInsHandler_s,		// 62
	InvalidInsHandler_s,		// 63
	InvalidInsHandler_s,		// 64
	InvalidInsHandler_s,		// 65
	InvalidInsHandler_s,		// 66
	InvalidInsHandler_s,		// 67
	InvalidInsHandler_s,		// 68
	InvalidInsHandler_s,		// 69
	InvalidInsHandler_s,		// 6A
	InvalidInsHandler_s,		// 6B
	InvalidInsHandler_s,		// 6C
	InvalidInsHandler_s,		// 6D
	InvalidInsHandler_s,		// 6E
	InvalidInsHandler_s,		// 6F
	InvalidInsHandler_s,		// 70
	InvalidInsHandler_s,		// 71
	InvalidInsHandler_s,		// 72
	InvalidInsHandler_s,		// 73
	InvalidInsHandler_s,		// 74
	InvalidInsHandler_s,		// 75
	InvalidInsHandler_s,		// 76
	InvalidInsHandler_s,		// 77
	InvalidInsHandler_s,		// 78
	InvalidInsHandler_s,		// 79
	InvalidInsHandler_s,		// 7A
	InvalidInsHandler_s,		// 7B
	InvalidInsHandler_s,		// 7C
	InvalidInsHandler_s,		// 7D
	InvalidInsHandler_s,		// 7E
	InvalidInsHandler_s,		// 7F
	Subw_M,		// 80
	Cmpw_M,		// 81
	Sbcd_M,		// 82
	Cmpd_M,		// 83
	Andd_M,		// 84
	Bitd_M,		// 85
	Ldw_M,		// 86
	InvalidInsHandler_s,		// 87
	Eord_M,		// 88
	Adcd_M,		// 89
	Ord_M,		// 8A
	Addw_M,		// 8B
	Cmpy_M,		// 8C
	InvalidInsHandler_s,		// 8D
	Ldy_M,		// 8E
	InvalidInsHandler_s,		// 8F
	Subw_D,		// 90
	Cmpw_D,		// 91
	Sbcd_D,		// 92
	Cmpd_D,		// 93
	Andd_D,		// 94
	Bitd_D,		// 95
	Ldw_D,		// 96
	Stw_D,		// 97
	Eord_D,		// 98
	Adcd_D,		// 99
	Ord_D,		// 9A
	Addw_D,		// 9B
	Cmpy_D,		// 9C
	InvalidInsHandler_s,		// 9D
	Ldy_D,		// 9E
	Sty_D,		// 9F
	Subw_X,		// A0
	Cmpw_X,		// A1
	Sbcd_X,		// A2
	Cmpd_X,		// A3
	Andd_X,		// A4
	Bitd_X,		// A5
	Ldw_X,		// A6
	Stw_X,		// A7
	Eord_X,		// A8
	Adcd_X,		// A9
	Ord_X,		// AA
	Addw_X,		// AB
	Cmpy_X,		// AC
	InvalidInsHandler_s,		// AD
	Ldy_X,		// AE
	Sty_X,		// AF
	Subw_E,		// B0
	Cmpw_E,		// B1
	Sbcd_E,		// B2
	Cmpd_E,		// B3
	Andd_E,		// B4
	Bitd_E,		// B5
	Ldw_E,		// B6
	Stw_E,		// B7
	Eord_E,		// B8
	Adcd_E,		// B9
	Ord_E,		// BA
	Addw_E,		// BB
	Cmpy_E,		// BC
	InvalidInsHandler_s,		// BD
	Ldy_E,		// BE
	Sty_E,		// BF
	InvalidInsHandler_s,		// C0
	InvalidInsHandler_s,		// C1
	InvalidInsHandler_s,		// C2
	InvalidInsHandler_s,		// C3
	InvalidInsHandler_s,		// C4
	InvalidInsHandler_s,		// C5
	InvalidInsHandler_s,		// C6
	InvalidInsHandler_s,		// C7
	InvalidInsHandler_s,		// C8
	InvalidInsHandler_s,		// C9
	InvalidInsHandler_s,		// CA
	InvalidInsHandler_s,		// CB
	InvalidInsHandler_s,		// CC
	InvalidInsHandler_s,		// CD
	Lds_I,		// CE
	InvalidInsHandler_s,		// CF
	InvalidInsHandler_s,		// D0
	InvalidInsHandler_s,		// D1
	InvalidInsHandler_s,		// D2
	InvalidInsHandler_s,		// D3
	InvalidInsHandler_s,		// D4
	InvalidInsHandler_s,		// D5
	InvalidInsHandler_s,		// D6
	InvalidInsHandler_s,		// D7
	InvalidInsHandler_s,		// D8
	InvalidInsHandler_s,		// D9
	InvalidInsHandler_s,		// DA
	InvalidInsHandler_s,		// DB
	Ldq_D,		// DC
	Stq_D,		// DD
	Lds_D,		// DE
	Sts_D,		// DF
	InvalidInsHandler_s,		// E0
	InvalidInsHandler_s,		// E1
	InvalidInsHandler_s,		// E2
	InvalidInsHandler_s,		// E3
	InvalidInsHandler_s,		// E4
	InvalidInsHandler_s,		// E5
	InvalidInsHandler_s,		// E6
	InvalidInsHandler_s,		// E7
	InvalidInsHandler_s,		// E8
	InvalidInsHandler_s,		// E9
	InvalidInsHandler_s,		// EA
	InvalidInsHandler_s,		// EB
	Ldq_X,		// EC
	Stq_X,		// ED
	Lds_X,		// EE
	Sts_X,		// EF
	InvalidInsHandler_s,		// F0
	InvalidInsHandler_s,		// F1
	InvalidInsHandler_s,		// F2
	InvalidInsHandler_s,		// F3
	InvalidInsHandler_s,		// F4
	InvalidInsHandler_s,		// F5
	InvalidInsHandler_s,		// F6
	InvalidInsHandler_s,		// F7
	InvalidInsHandler_s,		// F8
	InvalidInsHandler_s,		// F9
	InvalidInsHandler_s,		// FA
	InvalidInsHandler_s,		// FB
	Ldq_E,		// FC
	Stq_E,		// FD
	Lds_E,		// FE
	Sts_E,		// FF
};

static void(*JmpVec3[256])(void) = {
	InvalidInsHandler_s,		// 00
	InvalidInsHandler_s,		// 01
	InvalidInsHandler_s,		// 02
	InvalidInsHandler_s,		// 03
	InvalidInsHandler_s,		// 04
	InvalidInsHandler_s,		// 05
	InvalidInsHandler_s,		// 06
	InvalidInsHandler_s,		// 07
	InvalidInsHandler_s,		// 08
	InvalidInsHandler_s,		// 09
	InvalidInsHandler_s,		// 0A
	InvalidInsHandler_s,		// 0B
	InvalidInsHandler_s,		// 0C
	InvalidInsHandler_s,		// 0D
	InvalidInsHandler_s,		// 0E
	InvalidInsHandler_s,		// 0F
	InvalidInsHandler_s,		// 10
	InvalidInsHandler_s,		// 11
	InvalidInsHandler_s,		// 12
	InvalidInsHandler_s,		// 13
	InvalidInsHandler_s,		// 14
	InvalidInsHandler_s,		// 15
	InvalidInsHandler_s,		// 16
	InvalidInsHandler_s,		// 17
	InvalidInsHandler_s,		// 18
	InvalidInsHandler_s,		// 19
	InvalidInsHandler_s,		// 1A
	InvalidInsHandler_s,		// 1B
	InvalidInsHandler_s,		// 1C
	InvalidInsHandler_s,		// 1D
	InvalidInsHandler_s,		// 1E
	InvalidInsHandler_s,		// 1F
	InvalidInsHandler_s,		// 20
	InvalidInsHandler_s,		// 21
	InvalidInsHandler_s,		// 22
	InvalidInsHandler_s,		// 23
	InvalidInsHandler_s,		// 24
	InvalidInsHandler_s,		// 25
	InvalidInsHandler_s,		// 26
	InvalidInsHandler_s,		// 27
	InvalidInsHandler_s,		// 28
	InvalidInsHandler_s,		// 29
	InvalidInsHandler_s,		// 2A
	InvalidInsHandler_s,		// 2B
	InvalidInsHandler_s,		// 2C
	InvalidInsHandler_s,		// 2D
	InvalidInsHandler_s,		// 2E
	InvalidInsHandler_s,		// 2F
	Band,		// 30
	Biand,		// 31
	Bor,		// 32
	Bior,		// 33
	Beor,		// 34
	Bieor,		// 35
	Ldbt,		// 36
	Stbt,		// 37
	Tfm1,		// 38
	Tfm2,		// 39
	Tfm3,		// 3A
	Tfm4,		// 3B
	Bitmd_M,	// 3C
	Ldmd_M,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi3_I,		// 3F
	InvalidInsHandler_s,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Come_I,		// 43
	InvalidInsHandler_s,		// 44
	InvalidInsHandler_s,		// 45
	InvalidInsHandler_s,		// 46
	InvalidInsHandler_s,		// 47
	InvalidInsHandler_s,		// 48
	InvalidInsHandler_s,		// 49
	Dece_I,		// 4A
	InvalidInsHandler_s,		// 4B
	Ince_I,		// 4C
	Tste_I,		// 4D
	InvalidInsHandler_s,		// 4E
	Clre_I,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comf_I,		// 53
	InvalidInsHandler_s,		// 54
	InvalidInsHandler_s,		// 55
	InvalidInsHandler_s,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	InvalidInsHandler_s,		// 59
	Decf_I,		// 5A
	InvalidInsHandler_s,		// 5B
	Incf_I,		// 5C
	Tstf_I,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrf_I,		// 5F
	InvalidInsHandler_s,		// 60
	InvalidInsHandler_s,		// 61
	InvalidInsHandler_s,		// 62
	InvalidInsHandler_s,		// 63
	InvalidInsHandler_s,		// 64
	InvalidInsHandler_s,		// 65
	InvalidInsHandler_s,		// 66
	InvalidInsHandler_s,		// 67
	InvalidInsHandler_s,		// 68
	InvalidInsHandler_s,		// 69
	InvalidInsHandler_s,		// 6A
	InvalidInsHandler_s,		// 6B
	InvalidInsHandler_s,		// 6C
	InvalidInsHandler_s,		// 6D
	InvalidInsHandler_s,		// 6E
	InvalidInsHandler_s,		// 6F
	InvalidInsHandler_s,		// 70
	InvalidInsHandler_s,		// 71
	InvalidInsHandler_s,		// 72
	InvalidInsHandler_s,		// 73
	InvalidInsHandler_s,		// 74
	InvalidInsHandler_s,		// 75
	InvalidInsHandler_s,		// 76
	InvalidInsHandler_s,		// 77
	InvalidInsHandler_s,		// 78
	InvalidInsHandler_s,		// 79
	InvalidInsHandler_s,		// 7A
	InvalidInsHandler_s,		// 7B
	InvalidInsHandler_s,		// 7C
	InvalidInsHandler_s,		// 7D
	InvalidInsHandler_s,		// 7E
	InvalidInsHandler_s,		// 7F
	Sube_M,		// 80
	Cmpe_M,		// 81
	InvalidInsHandler_s,		// 82
	Cmpu_M,		// 83
	InvalidInsHandler_s,		// 84
	InvalidInsHandler_s,		// 85
	Lde_M,		// 86
	InvalidInsHandler_s,		// 87
	InvalidInsHandler_s,		// 88
	InvalidInsHandler_s,		// 89
	InvalidInsHandler_s,		// 8A
	Adde_M,		// 8B
	Cmps_M,		// 8C
	Divd_M,		// 8D
	Divq_M,		// 8E
	Muld_M,		// 8F
	Sube_D,		// 90
	Cmpe_D,		// 91
	InvalidInsHandler_s,		// 92
	Cmpu_D,		// 93
	InvalidInsHandler_s,		// 94
	InvalidInsHandler_s,		// 95
	Lde_D,		// 96
	Ste_D,		// 97
	InvalidInsHandler_s,		// 98
	InvalidInsHandler_s,		// 99
	InvalidInsHandler_s,		// 9A
	Adde_D,		// 9B
	Cmps_D,		// 9C
	Divd_D,		// 9D
	Divq_D,		// 9E
	Muld_D,		// 9F
	Sube_X,		// A0
	Cmpe_X,		// A1
	InvalidInsHandler_s,		// A2
	Cmpu_X,		// A3
	InvalidInsHandler_s,		// A4
	InvalidInsHandler_s,		// A5
	Lde_X,		// A6
	Ste_X,		// A7
	InvalidInsHandler_s,		// A8
	InvalidInsHandler_s,		// A9
	InvalidInsHandler_s,		// AA
	Adde_X,		// AB
	Cmps_X,		// AC
	Divd_X,		// AD
	Divq_X,		// AE
	Muld_X,		// AF
	Sube_E,		// B0
	Cmpe_E,		// B1
	InvalidInsHandler_s,		// B2
	Cmpu_E,		// B3
	InvalidInsHandler_s,		// B4
	InvalidInsHandler_s,		// B5
	Lde_E,		// B6
	Ste_E,		// B7
	InvalidInsHandler_s,		// B8
	InvalidInsHandler_s,		// B9
	InvalidInsHandler_s,		// BA
	Adde_E,		// BB
	Cmps_E,		// BC
	Divd_E,		// BD
	Divq_E,		// BE
	Muld_E,		// BF
	Subf_M,		// C0
	Cmpf_M,		// C1
	InvalidInsHandler_s,		// C2
	InvalidInsHandler_s,		// C3
	InvalidInsHandler_s,		// C4
	InvalidInsHandler_s,		// C5
	Ldf_M,		// C6
	InvalidInsHandler_s,		// C7
	InvalidInsHandler_s,		// C8
	InvalidInsHandler_s,		// C9
	InvalidInsHandler_s,		// CA
	Addf_M,		// CB
	InvalidInsHandler_s,		// CC
	InvalidInsHandler_s,		// CD
	InvalidInsHandler_s,		// CE
	InvalidInsHandler_s,		// CF
	Subf_D,		// D0
	Cmpf_D,		// D1
	InvalidInsHandler_s,		// D2
	InvalidInsHandler_s,		// D3
	InvalidInsHandler_s,		// D4
	InvalidInsHandler_s,		// D5
	Ldf_D,		// D6
	Stf_D,		// D7
	InvalidInsHandler_s,		// D8
	InvalidInsHandler_s,		// D9
	InvalidInsHandler_s,		// DA
	Addf_D,		// DB
	InvalidInsHandler_s,		// DC
	InvalidInsHandler_s,		// DD
	InvalidInsHandler_s,		// DE
	InvalidInsHandler_s,		// DF
	Subf_X,		// E0
	Cmpf_X,		// E1
	InvalidInsHandler_s,		// E2
	InvalidInsHandler_s,		// E3
	InvalidInsHandler_s,		// E4
	InvalidInsHandler_s,		// E5
	Ldf_X,		// E6
	Stf_X,		// E7
	InvalidInsHandler_s,		// E8
	InvalidInsHandler_s,		// E9
	InvalidInsHandler_s,		// EA
	Addf_X,		// EB
	InvalidInsHandler_s,		// EC
	InvalidInsHandler_s,		// ED
	InvalidInsHandler_s,		// EE
	InvalidInsHandler_s,		// EF
	Subf_E,		// F0
	Cmpf_E,		// F1
	InvalidInsHandler_s,		// F2
	InvalidInsHandler_s,		// F3
	InvalidInsHandler_s,		// F4
	InvalidInsHandler_s,		// F5
	Ldf_E,		// F6
	Stf_E,		// F7
	InvalidInsHandler_s,		// F8
	InvalidInsHandler_s,		// F9
	InvalidInsHandler_s,		// FA
	Addf_E,		// FB
	InvalidInsHandler_s,		// FC
	InvalidInsHandler_s,		// FD
	InvalidInsHandler_s,		// FE
	InvalidInsHandler_s,		// FF
};

void getflags(char *flags)
{
	flags[0] = cc_s[H] == 1 ? 'H' : '_';
	flags[1] = cc_s[N] == 1 ? 'N' : '_';
	flags[2] = cc_s[Z] == 1 ? 'Z' : '_';
	flags[3] = cc_s[V] == 1 ? 'V' : '_';
	flags[4] = cc_s[C] == 1 ? 'C' : '_';
	flags[5] = 0;
}

int HD6309Exec_s(int CycleFor)
{
char flags[6] ;

	//static unsigned char opcode = 0;
	CycleCounter = 0;
	gCycleFor = CycleFor;
	while (CycleCounter < CycleFor) {

		if (PendingInterupts)
		{
			if (PendingInterupts & 4)
				cpu_nmi();

			if (PendingInterupts & 2)
				cpu_firq();

			if (PendingInterupts & 1)
			{
				if (IRQWaiter == 0)	// This is needed to fix a subtle timming problem
					cpu_irq();		// It allows the CPU to see $FF03 bit 7 high before
				else				// The IRQ is asserted.
					IRQWaiter -= 1;
			}
		}

		if (SyncWaiting_s == 1)	//Abort the run nothing happens asyncronously from the CPU
			break; //return(0); // WDZ - Experimental SyncWaiting_s should still return used cycles (and not zero) by breaking from loop

		unsigned char memByte = MemRead8(PC_REG++);
		// unsigned char postbyte, membyte;
		// unsigned short memaddr;
		// if (memByte == 0x09)
		// {
		// 	postbyte = MemRead8(PC_REG);
		// 	memaddr = dp_s.Reg | postbyte;
		// 	membyte = MemRead8(memaddr);
		// 	getflags(flags);
		// 	printf("Rol_D %02x (%04x) %02x %02x %s = ", postbyte, memaddr, membyte, cc_s[C], flags);
		// }
		JmpVec1[memByte](); // Execute instruction pointed to by PC_REG
		// if (memByte == 0x09)
		// {
		// 	membyte = MemRead8(memaddr);
		// 	getflags(flags);
		// 	printf("%02x %s\n", membyte, flags);
		// }
		if (memByte < 10) CycleCounter += instcycl1[memByte]; // Add instruction cycles
		//instcnt1[memByte]++;
		//JmpVec1[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
	}//End While

	return(CycleFor - CycleCounter);
}

static void Page_2(void) //10
{
	unsigned char memByte = MemRead8(PC_REG++);
	JmpVec2[memByte](); // Execute instruction pointed to by PC_REG
	// CycleCounter += instcycl2[memByte]; // Add instruction cycles
	//instcnt2[memByte]++;
	//JmpVec2[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

static void Page_3(void) //11
{
	unsigned char memByte = MemRead8(PC_REG++);
	JmpVec3[memByte](); // Execute instruction pointed to by PC_REG
	// CycleCounter += instcycl2[memByte]; // Add instruction cycles
	//instcnt3[memByte]++;
	//JmpVec3[MemRead8(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

static void cpu_firq(void)
{
	
	if (!cc_s[F])
	{
		InInterupt_s=1; //Flag to indicate FIRQ has been asserted
		switch (MD_FIRQMODE /* md[FIRQMODE] */)
		{
		case 0:
			cc_s[E]=0; // Turn E flag off
			MemWrite8( pc_s.B.lsb,--S_REG);
			MemWrite8( pc_s.B.msb,--S_REG);
			MemWrite8(getcc(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16(VFIRQ);
		break;

		case MD_FIRQMODE_BIT:		//6309
			cc_s[E]=1;
			MemWrite8( pc_s.B.lsb,--S_REG);
			MemWrite8( pc_s.B.msb,--S_REG);
			MemWrite8( u_s.B.lsb,--S_REG);
			MemWrite8( u_s.B.msb,--S_REG);
			MemWrite8( y_s.B.lsb,--S_REG);
			MemWrite8( y_s.B.msb,--S_REG);
			MemWrite8( x_s.B.lsb,--S_REG);
			MemWrite8( x_s.B.msb,--S_REG);
			MemWrite8( dp_s.B.msb,--S_REG);
			if (MD_NATIVE6309)
			{
				MemWrite8((F_REG),--S_REG);
				MemWrite8((E_REG),--S_REG);
			}
			MemWrite8(B_REG,--S_REG);
			MemWrite8(A_REG,--S_REG);
			MemWrite8(getcc(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16(VFIRQ);
		break;
		}
	}
	PendingInterupts=PendingInterupts & 253;
	return;
}

static void cpu_irq(void)
{
	if (InInterupt_s==1) //If FIRQ is running postpone the IRQ
		return;			
	if ((!cc_s[I]) )
	{
		cc_s[E]=1;
		MemWrite8( pc_s.B.lsb,--S_REG);
		MemWrite8( pc_s.B.msb,--S_REG);
		MemWrite8( u_s.B.lsb,--S_REG);
		MemWrite8( u_s.B.msb,--S_REG);
		MemWrite8( y_s.B.lsb,--S_REG);
		MemWrite8( y_s.B.msb,--S_REG);
		MemWrite8( x_s.B.lsb,--S_REG);
		MemWrite8( x_s.B.msb,--S_REG);
		MemWrite8( dp_s.B.msb,--S_REG);
		if (MD_NATIVE6309)
		{
			MemWrite8((F_REG),--S_REG);
			MemWrite8((E_REG),--S_REG);
		}
		MemWrite8(B_REG,--S_REG);
		MemWrite8(A_REG,--S_REG);
		MemWrite8(getcc(),--S_REG);
		PC_REG=MemRead16(VIRQ);
		cc_s[I]=1; 
	} //Fi I test
	PendingInterupts=PendingInterupts & 254;
	return;
}

static void cpu_nmi(void)
{
	cc_s[E]=1;
	MemWrite8( pc_s.B.lsb,--S_REG);
	MemWrite8( pc_s.B.msb,--S_REG);
	MemWrite8( u_s.B.lsb,--S_REG);
	MemWrite8( u_s.B.msb,--S_REG);
	MemWrite8( y_s.B.lsb,--S_REG);
	MemWrite8( y_s.B.msb,--S_REG);
	MemWrite8( x_s.B.lsb,--S_REG);
	MemWrite8( x_s.B.msb,--S_REG);
	MemWrite8( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	cc_s[I]=1;
	cc_s[F]=1;
	PC_REG=MemRead16(VNMI);
	PendingInterupts=PendingInterupts & 251;
	return;
}

static unsigned short CalculateEA(unsigned char postbyte)
{
	static unsigned short int ea = 0;
	static signed char byte = 0;
	static unsigned char Register;

	Register = ((postbyte >> 5) & 3) + 1;

	if (postbyte & 0x80)
	{
		switch (postbyte & 0x1F)
		{
		case 0:
			ea = (*xfreg16_s[Register]);
			(*xfreg16_s[Register])++;
			CycleCounter += 2;
			break;

		case 1:
			ea = (*xfreg16_s[Register]);
			(*xfreg16_s[Register]) += 2;
			CycleCounter += 3;
			break;

		case 2:
			(*xfreg16_s[Register]) -= 1;
			ea = (*xfreg16_s[Register]);
			CycleCounter += 2;
			break;

		case 3:
			(*xfreg16_s[Register]) -= 2;
			ea = (*xfreg16_s[Register]);
			CycleCounter += 3;
			break;

		case 4:
			ea = (*xfreg16_s[Register]);
			break;

		case 5:
			ea = (*xfreg16_s[Register]) + ((signed char)B_REG);
			CycleCounter += 1;
			break;

		case 6:
			ea = (*xfreg16_s[Register]) + ((signed char)A_REG);
			CycleCounter += 1;
			break;

		case 7:
			ea = (*xfreg16_s[Register]) + ((signed char)E_REG);
			CycleCounter += 1;
			break;

		case 8:
			ea = (*xfreg16_s[Register]) + (signed char)MemRead8(PC_REG++);
			CycleCounter += 1;
			break;

		case 9:
			ea = (*xfreg16_s[Register]) + IMMADDRESS(PC_REG);
			CycleCounter += 4;
			PC_REG += 2;
			break;

		case 10:
			ea = (*xfreg16_s[Register]) + ((signed char)F_REG);
			CycleCounter += 1;
			break;

		case 11:
			ea = (*xfreg16_s[Register]) + D_REG; //Changed to unsigned 03/14/2005 NG Was signed
			CycleCounter += 4;
			break;

		case 12:
			ea = (signed short)PC_REG + (signed char)MemRead8(PC_REG) + 1;
			CycleCounter += 1;
			PC_REG++;
			break;

		case 13: //MM
			ea = PC_REG + IMMADDRESS(PC_REG) + 2;
			CycleCounter += 5;
			PC_REG += 2;
			break;

		case 14:
			ea = (*xfreg16_s[Register]) + W_REG;
			CycleCounter += 4;
			break;

		case 15: //01111
			byte = (postbyte >> 5) & 3;
			switch (byte)
			{
			case 0:
				ea = W_REG;
				break;
			case 1:
				ea = W_REG + IMMADDRESS(PC_REG);
				PC_REG += 2;
				CycleCounter += 2;
				break;
			case 2:
				ea = W_REG;
				W_REG += 2;
				CycleCounter += 1;
				break;
			case 3:
				W_REG -= 2;
				ea = W_REG;
				CycleCounter += 1;
				break;
			}
			break;

		case 16: //10000
			byte = (postbyte >> 5) & 3;
			switch (byte)
			{
			case 0:
				ea = MemRead16(W_REG);
				CycleCounter += 3;
				break;
			case 1:
				ea = MemRead16(W_REG + IMMADDRESS(PC_REG));
				PC_REG += 2;
				CycleCounter += 5;
				break;
			case 2:
				ea = MemRead16(W_REG);
				W_REG += 2;
				CycleCounter += 4;
				break;
			case 3:
				W_REG -= 2;
				ea = MemRead16(W_REG);
				CycleCounter += 4;
				break;
			}
			break;


		case 17: //10001
			ea = (*xfreg16_s[Register]);
			(*xfreg16_s[Register]) += 2;
			ea = MemRead16(ea);
			CycleCounter += 6;
			break;

		case 18: //10010
			CycleCounter += 6;
			break;

		case 19: //10011
			(*xfreg16_s[Register]) -= 2;
			ea = (*xfreg16_s[Register]);
			ea = MemRead16(ea);
			CycleCounter += 6;
			break;

		case 20: //10100
			ea = (*xfreg16_s[Register]);
			ea = MemRead16(ea);
			CycleCounter += 3;
			break;

		case 21: //10101
			ea = (*xfreg16_s[Register]) + ((signed char)B_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 22: //10110
			ea = (*xfreg16_s[Register]) + ((signed char)A_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 23: //10111
			ea = (*xfreg16_s[Register]) + ((signed char)E_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 24: //11000
			ea = (*xfreg16_s[Register]) + (signed char)MemRead8(PC_REG++);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 25: //11001
			ea = (*xfreg16_s[Register]) + IMMADDRESS(PC_REG);
			ea = MemRead16(ea);
			CycleCounter += 7;
			PC_REG += 2;
			break;
		case 26: //11010
			ea = (*xfreg16_s[Register]) + ((signed char)F_REG);
			ea = MemRead16(ea);
			CycleCounter += 4;
			break;

		case 27: //11011
			ea = (*xfreg16_s[Register]) + D_REG;
			ea = MemRead16(ea);
			CycleCounter += 7;
			break;

		case 28: //11100
			ea = (signed short)PC_REG + (signed char)MemRead8(PC_REG) + 1;
			ea = MemRead16(ea);
			CycleCounter += 4;
			PC_REG++;
			break;

		case 29: //11101
			ea = PC_REG + IMMADDRESS(PC_REG) + 2;
			ea = MemRead16(ea);
			CycleCounter += 8;
			PC_REG += 2;
			break;

		case 30: //11110
			ea = (*xfreg16_s[Register]) + W_REG;
			ea = MemRead16(ea);
			CycleCounter += 7;
			break;

		case 31: //11111
			ea = IMMADDRESS(PC_REG);
			ea = MemRead16(ea);
			CycleCounter += 8;
			PC_REG += 2;
			break;

		} //END Switch
	}
	else
	{
		byte = (postbyte & 31);
		byte = (byte << 3);
		byte = byte / 8;
		ea = *xfreg16_s[Register] + byte; //Was signed
		CycleCounter += 1;
	}
	return(ea);
}

static void setcc (unsigned char bincc)
{
	unsigned char bit;
	for (bit=0;bit<=7;bit++)
		cc_s[bit]=!!(bincc & (1<<bit));
	return;
}

static unsigned char getcc(void)
{
	unsigned char bincc=0,bit=0;
	for (bit=0;bit<=7;bit++)
		if (cc_s[bit])
			bincc=bincc | (1<<bit);
		return(bincc);
}

void setmd_s (unsigned char binmd)
{
	// unsigned char bit;
	// for (bit=0;bit<=1;bit++)
	// 	md[bit]=!!(binmd & (1<<bit));
	mdbits_s = binmd;

	if (binmd & 1) // In nativemode
	{
		instcycl1 = instcyclnat1;
		instcycl2 = instcyclnat2;
		instcycl3 = instcyclnat3;
	}
	else // emulation mode
	{
		instcycl1 = instcyclemu1;
		instcycl2 = instcyclemu2;
		instcycl3 = instcyclemu3;
	}

	return;
}

static unsigned char getmd(void)
{
	// unsigned char binmd=0,bit=0;
	// for (bit=6;bit<=7;bit++)
	// 	if (md[bit])
	// 		binmd=binmd | (1<<bit);
	// 	return(binmd);
	return mdbits_s;
}
	
void HD6309AssertInterupt_s(unsigned char Interupt,unsigned char waiter)// 4 nmi 2 firq 1 irq
{
	SyncWaiting_s=0;
	PendingInterupts=PendingInterupts | (1<<(Interupt-1));
	IRQWaiter=waiter;
	return;
}

void HD6309DeAssertInterupt_s(unsigned char Interupt)// 4 nmi 2 firq 1 irq
{
	PendingInterupts=PendingInterupts & ~(1<<(Interupt-1));
	InInterupt_s=0;
	return;
}

void InvalidInsHandler_s(void)
{	
	mdbits_s |= MD_ILLEGALINST_BIT; // md[ILLEGAL]=1;
	mdbits_s=getmd();
	ErrorVector();
	return;
}

void DivbyZero_s(void)
{
	mdbits_s |= MD_DIVBYZERO_BIT; // md[ZERODIV]=1;
	mdbits_s=getmd();
	ErrorVector();
	return;
}

static void  ErrorVector(void)
{
	cc_s[E]=1;
	MemWrite8( pc_s.B.lsb,--S_REG);
	MemWrite8( pc_s.B.msb,--S_REG);
	MemWrite8( u_s.B.lsb,--S_REG);
	MemWrite8( u_s.B.msb,--S_REG);
	MemWrite8( y_s.B.lsb,--S_REG);
	MemWrite8( y_s.B.msb,--S_REG);
	MemWrite8( x_s.B.lsb,--S_REG);
	MemWrite8( x_s.B.msb,--S_REG);
	MemWrite8( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8((F_REG),--S_REG);
		MemWrite8((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8(B_REG,--S_REG);
	MemWrite8(A_REG,--S_REG);
	MemWrite8(getcc(),--S_REG);
	PC_REG=MemRead16(VTRAP);
	CycleCounter+=(12 + InsCycles[MD_NATIVE6309][M54]);	//One for each byte +overhead? Guessing from PSHS
	return;
}

unsigned int MemRead32_s(unsigned short Address)
{
	return ( (MemRead16(Address)<<16) | MemRead16(Address+2) );

}

void MemWrite32_s(unsigned int data,unsigned short Address)
{
	MemWrite16( data>>16,Address);
	MemWrite16( data & 0xFFFF,Address+2);
	return;
}

static unsigned char GetSorceReg(unsigned char Tmp)
{
	unsigned char Source=(Tmp>>4);
	unsigned char Dest= Tmp & 15;
	unsigned char Translate[]={0,0};
	if ( (Source & 8) == (Dest & 8) ) //like size registers
		return(Source );
return(0);
}

void HD6309ForcePC_s(unsigned short NewPC)
{
	PC_REG=NewPC;
	PendingInterupts=0;
	SyncWaiting_s=0;
	return;
}

// unsigned short GetPC(void)
// {
// 	return(PC_REG);
// }


