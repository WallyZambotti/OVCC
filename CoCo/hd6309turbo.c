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
// 
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
unsigned char cc_s[8];
unsigned char *ureg8_s[8]; 
unsigned char ccbits_s,mdbits_s;
unsigned short *xfreg16_s[8];
volatile int CycleCounter=0;
volatile unsigned char SyncWaiting_s=0;
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
volatile char InInterupt_s=0;
volatile int gCycleFor;
static short *instcycl1, *instcycl2, *instcycl3;
static unsigned long instcnt1[256], instcnt2[256], instcnt3[256];
//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
static unsigned short CalculateEA(unsigned char);
void MSABI InvalidInsHandler_s(void);
void MSABI DivbyZero_s(void);
static void ErrorVector(void);
extern void MSABI setcc_s(unsigned char);
extern unsigned char MSABI getcc_s(void);
void MSABI setmd_s(unsigned char);
static void cpu_firq(void);
static void cpu_irq(void);
static void cpu_nmi(void);
static unsigned char GetSorceReg(unsigned char);
static void Page_2(void);
static void Page_3(void);
void MemWrite8(unsigned char, unsigned short);
void MemWrite16(unsigned short, unsigned short);
unsigned char MemRead8(unsigned short);
unsigned short MemRead16(unsigned short);

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
	mdbits_s=0;
	setmd_s(mdbits_s);
	dp_s.Reg=0;
	cc_s[I]=1;
	cc_s[F]=1;
	SyncWaiting_s=0;
	PC_REG=MemRead16(VRESET);	//PC gets its reset vector
	SetMapType(0);	//shouldn't be here
	for (int i = 0 ; i < 256 ; i++)
	{
		//printf("%02x 0:%ld 1:%ld 2:%ld\n", i, instcnt1[i], instcnt2[i], instcnt3[i]);
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

	cc_s[I]=1;
	cc_s[F]=1;
	return;
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
	1, // Divd_D 9D + does own cycles
	1, // Divq_D 9E + does own cycles
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
	0, // Divd_X AD + does own cycles
	0, // Divq_X AE + does own cycles
	30, // Muld_X AF
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
	1, // Divd_E BD + does own cycles
	1, // Divq_E BE + does own cycles
	31, // Muld_E BF
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
	0, // Divd_X AD + does own cycles
	0, // Divq_X AE + does own cycles
	30, // Muld_X AF
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
	0, // Divd_E BD + does own cycles
	0, // Divq_E BE + does own cycles
	30, // Muld_E BF
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
	HD6309Reset_s,		// 3E
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
	Eora_X_A,		// a8
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
	InvalidInsHandler_s,		// C7
	Eorb_M_A,		// C8
	Adcb_M_A,		// C9
	Orb_M_A,		// CA
	Addb_M_A,		// CB
	Ldd_M_A,		// CC
	Ldq_M_A,		// CD
	Ldu_M_A,		// CE
	InvalidInsHandler_s,		// CF
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
	LBrn_R_A,		// 21
	LBhi_R_A,		// 22
	LBls_R_A,		// 23
	LBhs_R_A,		// 24
	LBcs_R_A,		// 25
	LBne_R_A,		// 26
	LBeq_R_A,		// 27
	LBvc_R_A,		// 28
	LBvs_R_A,		// 29
	LBpl_R_A,		// 2A
	LBmi_R_A,		// 2B
	LBge_R_A,		// 2C
	LBlt_R_A,		// 2D
	LBgt_R_A,		// 2E
	LBle_R_A,		// 2F
	Addr_A,		// 30
	Adcr_A,		// 31
	Subr_A,		// 32
	Sbcr_A,		// 33
	Andr_A,		// 34
	Orr_A,		// 35
	Eorr_A,		// 36
	Cmpr_A,		// 37
	Pshsw_A,		// 38
	Pulsw_A,		// 39
	Pshuw_A,		// 3A
	Puluw_A,		// 3B
	InvalidInsHandler_s,		// 3C
	InvalidInsHandler_s,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi2_I_A,		// 3F
	Negd_I_A,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Comd_I_A,		// 43
	Lsrd_I_A,		// 44
	InvalidInsHandler_s,		// 45
	Rord_I_A,		// 46
	Asrd_I_A,		// 47
	Asld_I_A,		// 48
	Rold_I_A,		// 49
	Decd_I_A,		// 4A
	InvalidInsHandler_s,		// 4B
	Incd_I_A,		// 4C
	Tstd_I_A,		// 4D
	InvalidInsHandler_s,		// 4E
	Clrd_I_A,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comw_I_A,		// 53
	Lsrw_I_A,		// 54
	InvalidInsHandler_s,		// 55
	Rorw_I_A,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	Rolw_I_A,		// 59
	Decw_I_A,		// 5A
	InvalidInsHandler_s,		// 5B
	Incw_I_A,		// 5C
	Tstw_I_A,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrw_I_A,		// 5F
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
	Subw_M_A,		// 80
	Cmpw_M_A,		// 81
	Sbcd_M_A,		// 82
	Cmpd_M_A,		// 83
	Andd_M_A,		// 84
	Bitd_M_A,		// 85
	Ldw_M_A,		// 86
	InvalidInsHandler_s,		// 87
	Eord_M_A,		// 88
	Adcd_M_A,		// 89
	Ord_M_A,		// 8A
	Addw_M_A,		// 8B
	Cmpy_M_A,		// 8C
	InvalidInsHandler_s,		// 8D
	Ldy_M_A,		// 8E
	InvalidInsHandler_s,		// 8F
	Subw_D_A,		// 90
	Cmpw_D_A,		// 91
	Sbcd_D_A,		// 92
	Cmpd_D_A,		// 93
	Andd_D_A,		// 94
	Bitd_D_A,		// 95
	Ldw_D_A,		// 96
	Stw_D_A,		// 97
	Eord_D_A,		// 98
	Adcd_D_A,		// 99
	Ord_D_A,		// 9A
	Addw_D_A,		// 9B
	Cmpy_D_A,		// 9C
	InvalidInsHandler_s,		// 9D
	Ldy_D_A,		// 9E
	Sty_D_A,		// 9F
	Subw_X_A,		// A0
	Cmpw_X_A,		// A1
	Sbcd_X_A,		// A2
	Cmpd_X_A,		// A3
	Andd_X_A,		// A4
	Bitd_X_A,		// A5
	Ldw_X_A,		// A6
	Stw_X_A,		// A7
	Eord_X_A,		// A8
	Adcd_X_A,		// A9
	Ord_X_A,		// AA
	Addw_X_A,		// AB
	Cmpy_X_A,		// AC
	InvalidInsHandler_s,		// AD
	Ldy_X_A,		// AE
	Sty_X_A,		// AF
	Subw_E_A,		// B0
	Cmpw_E_A,		// B1
	Sbcd_E_A,		// B2
	Cmpd_E_A,		// B3
	Andd_E_A,		// B4
	Bitd_E_A,		// B5
	Ldw_E_A,		// B6
	Stw_E_A,		// B7
	Eord_E_A,		// B8
	Adcd_E_A,		// B9
	Ord_E_A,		// BA
	Addw_E_A,		// BB
	Cmpy_E_A,		// BC
	InvalidInsHandler_s,		// BD
	Ldy_E_A,		// BE
	Sty_E_A,		// BF
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
	Lds_M_A,		// CE
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
	Ldq_D_A,		// DC
	Stq_D_A,		// DD
	Lds_D_A,		// DE
	Sts_D_A,		// DF
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
	Ldq_X_A,		// EC
	Stq_X_A,		// ED
	Lds_X_A,		// EE
	Sts_X_A,		// EF
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
	Ldq_E_A,		// FC
	Stq_E_A,		// FD
	Lds_E_A,		// FE
	Sts_E_A,		// FF
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
	Band_A,		// 30
	Biand_A,		// 31
	Bor_A,		// 32
	Bior_A,		// 33
	Beor_A,		// 34
	Bieor_A,		// 35
	Ldbt_A,		// 36
	Stbt_A,		// 37
	Tfm1_A,		// 38
	Tfm2_A,		// 39
	Tfm3_A,		// 3A
	Tfm4_A,		// 3B
	Bitmd_M_A,	// 3C
	Ldmd_M_A,		// 3D
	InvalidInsHandler_s,		// 3E
	Swi3_I_A,		// 3F
	InvalidInsHandler_s,		// 40
	InvalidInsHandler_s,		// 41
	InvalidInsHandler_s,		// 42
	Come_I_A,		// 43
	InvalidInsHandler_s,		// 44
	InvalidInsHandler_s,		// 45
	InvalidInsHandler_s,		// 46
	InvalidInsHandler_s,		// 47
	InvalidInsHandler_s,		// 48
	InvalidInsHandler_s,		// 49
	Dece_I_A,		// 4A
	InvalidInsHandler_s,		// 4B
	Ince_I_A,		// 4C
	Tste_I_A,		// 4D
	InvalidInsHandler_s,		// 4E
	Clre_I_A,		// 4F
	InvalidInsHandler_s,		// 50
	InvalidInsHandler_s,		// 51
	InvalidInsHandler_s,		// 52
	Comf_I_A,		// 53
	InvalidInsHandler_s,		// 54
	InvalidInsHandler_s,		// 55
	InvalidInsHandler_s,		// 56
	InvalidInsHandler_s,		// 57
	InvalidInsHandler_s,		// 58
	InvalidInsHandler_s,		// 59
	Decf_I_A,		// 5A
	InvalidInsHandler_s,		// 5B
	Incf_I_A,		// 5C
	Tstf_I_A,		// 5D
	InvalidInsHandler_s,		// 5E
	Clrf_I_A,		// 5F
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
	Sube_M_A,		// 80
	Cmpe_M_A,		// 81
	InvalidInsHandler_s,		// 82
	Cmpu_M_A,		// 83
	InvalidInsHandler_s,		// 84
	InvalidInsHandler_s,		// 85
	Lde_M_A,		// 86
	InvalidInsHandler_s,		// 87
	InvalidInsHandler_s,		// 88
	InvalidInsHandler_s,		// 89
	InvalidInsHandler_s,		// 8A
	Adde_M_A,		// 8B
	Cmps_M_A,		// 8C
	Divd_M_A,		// 8D
	Divq_M_A,		// 8E
	Muld_M_A,		// 8F
	Sube_D_A,		// 90
	Cmpe_D_A,		// 91
	InvalidInsHandler_s,		// 92
	Cmpu_D_A,		// 93
	InvalidInsHandler_s,		// 94
	InvalidInsHandler_s,		// 95
	Lde_D_A,		// 96
	Ste_D_A,		// 97
	InvalidInsHandler_s,		// 98
	InvalidInsHandler_s,		// 99
	InvalidInsHandler_s,		// 9A
	Adde_D_A,		// 9B
	Cmps_D_A,		// 9C
	Divd_D_A,		// 9D
	Divq_D_A,		// 9E
	Muld_D_A,		// 9F
	Sube_X_A,		// A0
	Cmpe_X_A,		// A1
	InvalidInsHandler_s,		// A2
	Cmpu_X_A,		// A3
	InvalidInsHandler_s,		// A4
	InvalidInsHandler_s,		// A5
	Lde_X_A,		// A6
	Ste_X_A,		// A7
	InvalidInsHandler_s,		// A8
	InvalidInsHandler_s,		// A9
	InvalidInsHandler_s,		// AA
	Adde_X_A,		// AB
	Cmps_X_A,		// AC
	Divd_X_A,		// AD
	Divq_X_A,		// AE
	Muld_X_A,		// AF
	Sube_E_A,		// B0
	Cmpe_E_A,		// B1
	InvalidInsHandler_s,		// B2
	Cmpu_E_A,		// B3
	InvalidInsHandler_s,		// B4
	InvalidInsHandler_s,		// B5
	Lde_E_A,		// B6
	Ste_E_A,		// B7
	InvalidInsHandler_s,		// B8
	InvalidInsHandler_s,		// B9
	InvalidInsHandler_s,		// BA
	Adde_E_A,		// BB
	Cmps_E_A,		// BC
	Divd_E_A,		// BD
	Divq_E_A,		// BE
	Muld_E_A,		// BF
	Subf_M_A,		// C0
	Cmpf_M_A,		// C1
	InvalidInsHandler_s,		// C2
	InvalidInsHandler_s,		// C3
	InvalidInsHandler_s,		// C4
	InvalidInsHandler_s,		// C5
	Ldf_M_A,		// C6
	InvalidInsHandler_s,		// C7
	InvalidInsHandler_s,		// C8
	InvalidInsHandler_s,		// C9
	InvalidInsHandler_s,		// CA
	Addf_M_A,		// CB
	InvalidInsHandler_s,		// CC
	InvalidInsHandler_s,		// CD
	InvalidInsHandler_s,		// CE
	InvalidInsHandler_s,		// CF
	Subf_D_A,		// D0
	Cmpf_D_A,		// D1
	InvalidInsHandler_s,		// D2
	InvalidInsHandler_s,		// D3
	InvalidInsHandler_s,		// D4
	InvalidInsHandler_s,		// D5
	Ldf_D_A,		// D6
	Stf_D_A,		// D7
	InvalidInsHandler_s,		// D8
	InvalidInsHandler_s,		// D9
	InvalidInsHandler_s,		// DA
	Addf_D_A,		// DB
	InvalidInsHandler_s,		// DC
	InvalidInsHandler_s,		// DD
	InvalidInsHandler_s,		// DE
	InvalidInsHandler_s,		// DF
	Subf_X_A,		// E0
	Cmpf_X_A,		// E1
	InvalidInsHandler_s,		// E2
	InvalidInsHandler_s,		// E3
	InvalidInsHandler_s,		// E4
	InvalidInsHandler_s,		// E5
	Ldf_X_A,		// E6
	Stf_X_A,		// E7
	InvalidInsHandler_s,		// E8
	InvalidInsHandler_s,		// E9
	InvalidInsHandler_s,		// EA
	Addf_X_A,		// EB
	InvalidInsHandler_s,		// EC
	InvalidInsHandler_s,		// ED
	InvalidInsHandler_s,		// EE
	InvalidInsHandler_s,		// EF
	Subf_E_A,		// F0
	Cmpf_E_A,		// F1
	InvalidInsHandler_s,		// F2
	InvalidInsHandler_s,		// F3
	InvalidInsHandler_s,		// F4
	InvalidInsHandler_s,		// F5
	Ldf_E_A,		// F6
	Stf_E_A,		// F7
	InvalidInsHandler_s,		// F8
	InvalidInsHandler_s,		// F9
	InvalidInsHandler_s,		// FA
	Addf_E_A,		// FB
	InvalidInsHandler_s,		// FC
	InvalidInsHandler_s,		// FD
	InvalidInsHandler_s,		// FE
	InvalidInsHandler_s,		// FF
};

int HD6309Exec_s(int CycleFor)
{

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
		JmpVec1[memByte](); // Execute instruction pointed to by PC_REG
		CycleCounter += instcycl1[memByte]; // Add instruction cycles
		//instcnt1[memByte]++;
	}//End While

	return(CycleFor - CycleCounter);
}

static void Page_2(void) //10
{
	unsigned char memByte = MemRead8(PC_REG++);
	JmpVec2[memByte](); // Execute instruction pointed to by PC_REG
	CycleCounter += instcycl2[memByte]; // Add instruction cycles
	//instcnt2[memByte]++;
}

static void Page_3(void) //11
{
	unsigned char memByte = MemRead8(PC_REG++);
	JmpVec3[memByte](); // Execute instruction pointed to by PC_REG
	CycleCounter += instcycl3[memByte]; // Add instruction cycles
	//instcnt3[memByte]++;
}

static void cpu_firq(void)
{
	
	if (!cc_s[F])
	{
		InInterupt_s=1; //Flag to indicate FIRQ has been asserted
		switch (MD_FIRQMODE)
		{
		case 0:
			cc_s[E]=0; // Turn E flag off
			MemWrite8( pc_s.B.lsb,--S_REG);
			MemWrite8( pc_s.B.msb,--S_REG);
			MemWrite8(getcc_s(),--S_REG);
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
			MemWrite8(getcc_s(),--S_REG);
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
		MemWrite8(getcc_s(),--S_REG);
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
	MemWrite8(getcc_s(),--S_REG);
	cc_s[I]=1;
	cc_s[F]=1;
	PC_REG=MemRead16(VNMI);
	PendingInterupts=PendingInterupts & 251;
	return;
}

extern void SetNatEmuStat(unsigned char);

void MSABI setmd_s (unsigned char binmd)
{
	mdbits_s = binmd;

	if (binmd & 1) // In nativemode
	{
		instcycl1 = instcyclnat1;
		instcycl2 = instcyclnat2;
		instcycl3 = instcyclnat3;
		SetNatEmuStat(2);
	}
	else // emulation mode
	{
		instcycl1 = instcyclemu1;
		instcycl2 = instcyclemu2;
		instcycl3 = instcyclemu3;
		SetNatEmuStat(1);
	}

	return;
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

void MSABI InvalidInsHandler_s(void)
{	
	mdbits_s |= MD_ILLEGALINST_BIT;
	ErrorVector();
	return;
}

void MSABI DivbyZero_s(void)
{
	mdbits_s |= MD_DIVBYZERO_BIT;
	ErrorVector();
	return;
}

static void ErrorVector(void)
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
	MemWrite8(getcc_s(),--S_REG);
	PC_REG=MemRead16(VTRAP);
	CycleCounter+=(13 - MD_NATIVE6309);	//One for each byte +overhead? Guessing from PSHS
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
