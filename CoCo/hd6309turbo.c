/*
Copyright 2018 by Walter Zambotti
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
#include <stdint.h>
#include "hd6309.h"
#include "hd6309defs.h"

#if defined(_WIN64)
#define MSABI 
#else
#define MSABI __attribute__((ms_abi))
#endif

#define UINT8 uint8_t
#define UINT16 uint16_t
#define UINT32 uint32_t
#define UINT64 uint64_t

//Global variables for CPU Emulation-----------------------
#define NTEST8(r) r>0x7F;
#define NTEST16(r) r>0x7FFF;
#define NTEST32(r) r>0x7FFFFFFF;
#define OVERFLOW8(c,a,b,r) c ^ (((a^b^r)>>7) &1);
#define OVERFLOW16(c,a,b,r) c ^ (((a^b^r)>>15)&1);
#define ZTEST(r) !r;

#define DPADDRESS(r) (dp.Reg |MemRead8_s(r))
#define IMMADDRESS(r) MemRead16_s(r)
#define INDADDRESS(r) CalculateEA(MemRead8_s(r))

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

extern void InvalidInsHandler(void);
extern void Neg_D_A(void);
extern void Oim_D_A(void);
extern void Aim_D_A(void);
extern void Com_D_A(void);
extern void Lsr_D_A(void);
extern void Eim_D_A(void);
extern void Ror_D_A(void);
extern void Asr_D_A(void);
extern void Asl_D_A(void);
extern void Rol_D_A(void);
extern void Dec_D_A(void);
extern void Tim_D_A(void);
extern void Inc_D_A(void);
extern void Tst_D_A(void);
extern void Jmp_D_A(void);
extern void Clr_D_A(void);
extern void Nop_I_A(void);
extern void Sync_I_A(void);
extern void Sexw_I_A(void);
extern void Lbra_R_A(void);
extern void Lbsr_R_A(void);
extern void Daa_I_A(void);
extern void Orcc_M_A(void);
extern void Andcc_M_A(void);
extern void Sex_I_A(void);
extern void Exg_M_A(void);
extern void Tfr_M_A(void);
extern void Bra_R_A(void);
extern void Brn_R_A(void);
extern void Bhi_R_A(void);
extern void Bls_R_A(void);
extern void Bhs_R_A(void);
extern void Blo_R_A(void);
extern void Bne_R_A(void);
extern void Beq_R_A(void);
extern void Bvc_R_A(void);
extern void Bvs_R_A(void);
extern void Bpl_R_A(void);
extern void Bmi_R_A(void);
extern void Bge_R_A(void);
extern void Blt_R_A(void);
extern void Bgt_R_A(void);
extern void Ble_R_A(void);
extern void Leax_X_A(void);
extern void Leay_X_A(void);
extern void Leas_X_A(void);
extern void Leau_X_A(void);
extern void Pshs_M_A(void);
extern void Puls_M_A(void);
extern void Pshu_M_A(void);
extern void Pulu_M_A(void);
extern void Rts_I_A(void);
extern void Abx_I_A(void);
extern void Rti_I_A(void);
extern void Cwai_I_A(void);
extern void Mul_I_A(void);
extern void Swi1_I_A(void);
extern void Nega_I_A(void);
extern void Coma_I_A(void);
extern void Lsra_I_A(void);
extern void Rora_I_A(void);
extern void Asra_I_A(void);
extern void Asla_I_A(void);
extern void Rola_I_A(void);
extern void Deca_I_A(void);
extern void Inca_I_A(void);
extern void Tsta_I_A(void);
extern void Clra_I_A(void);
extern void Negb_I_A(void);
extern void Comb_I_A(void);
extern void Lsrb_I_A(void);
extern void Rorb_I_A(void);
extern void Asrb_I_A(void);
extern void Aslb_I_A(void);
extern void Rolb_I_A(void);
extern void Decb_I_A(void);
extern void Incb_I_A(void);
extern void Tstb_I_A(void);
extern void Clrb_I_A(void);
extern void Neg_X_A(void);
extern void Oim_X_A(void);
extern void Aim_X_A(void);
extern void Com_X_A(void);
extern void Lsr_X_A(void);
extern void Eim_X_A(void);
extern void Ror_X_A(void);
extern void Asr_X_A(void);
extern void Asl_X_A(void);
extern void Rol_X_A(void);
extern void Dec_X_A(void);
extern void Tim_X_A(void);
extern void Inc_X_A(void);
extern void Tst_X_A(void);
extern void Jmp_X_A(void);
extern void Clr_X_A(void);
extern void Neg_E_A(void);
extern void Oim_E_A(void);
extern void Aim_E_A(void);
extern void Com_E_A(void);
extern void Lsr_E_A(void);
extern void Eim_E_A(void);
extern void Ror_E_A(void);
extern void Asr_E_A(void);
extern void Asl_E_A(void);
extern void Rol_E_A(void);
extern void Dec_E_A(void);
extern void Tim_E_A(void);
extern void Inc_E_A(void);
extern void Tst_E_A(void);
extern void Jmp_E_A(void);
extern void Clr_E_A(void);
extern void Suba_M_A(void);
extern void Cmpa_M_A(void);
extern void Sbca_M_A(void);
extern void Suba_M_A(void);
extern void Subd_M_A(void);
extern void Anda_M_A(void);
extern void Bita_M_A(void);
extern void Lda_M_A(void);
extern void Eora_M_A(void);
extern void Adca_M_A(void);
extern void Ora_M_A(void);
extern void Adda_M_A(void);
extern void Cmpx_M_A(void);
extern void Bsr_R_A(void);
extern void Ldx_M_A(void);
extern void Suba_D_A(void);
extern void Cmpa_D_A(void);
extern void Sbca_D_A(void);
extern void Subd_D_A(void);
extern void Anda_D_A(void);
extern void Bita_D_A(void);
extern void Lda_D_A(void);
extern void Sta_D_A(void);
extern void Eora_D_A(void);
extern void Adca_D_A(void);
extern void Ora_D_A(void);
extern void Adda_D_A(void);
extern void Cmpx_D_A(void);
extern void Bsr_D_A(void);
extern void Ldx_D_A(void);
extern void Stx_D_A(void);
extern void Suba_X_A(void);
extern void Cmpa_X_A(void);
extern void Sbca_X_A(void);
extern void Subd_X_A(void);
extern void Anda_X_A(void);
extern void Bita_X_A(void);
extern void Lda_X_A(void);
extern void Sta_X_A(void);
extern void Eora_X_A(void);
extern void Adca_X_A(void);
extern void Ora_X_A(void);
extern void Adda_X_A(void);
extern void Cmpx_X_A(void);
extern void Bsr_X_A(void);
extern void Ldx_X_A(void);
extern void Stx_X_A(void);
extern void Suba_E_A(void);
extern void Cmpa_E_A(void);
extern void Sbca_E_A(void);
extern void Subd_E_A(void);
extern void Anda_E_A(void);
extern void Bita_E_A(void);
extern void Lda_E_A(void);
extern void Sta_E_A(void);
extern void Eora_E_A(void);
extern void Adca_E_A(void);
extern void Ora_E_A(void);
extern void Adda_E_A(void);
extern void Cmpx_E_A(void);
extern void Bsr_E_A(void);
extern void Ldx_E_A(void);
extern void Stx_E_A(void);
extern void Subb_M_A(void);
extern void Cmpb_M_A(void);
extern void Sbcb_M_A(void);
extern void Addd_M_A(void);
extern void Andb_M_A(void);
extern void Bitb_M_A(void);
extern void Ldb_M_A(void);
extern void Eorb_M_A(void);
extern void Adcb_M_A(void);
extern void Orb_M_A(void);
extern void Addb_M_A(void);
extern void Ldd_M_A(void);
extern void Ldq_M_A(void);
extern void Ldu_M_A(void);
extern void Subb_D_A(void);
extern void Cmpb_D_A(void);
extern void Sbcb_D_A(void);
extern void Addd_D_A(void);
extern void Andb_D_A(void);
extern void Bitb_D_A(void);
extern void Ldb_D_A(void);
extern void Stb_D_A(void);
extern void Eorb_D_A(void);
extern void Adcb_D_A(void);
extern void Orb_D_A(void);
extern void Addb_D_A(void);
extern void Ldd_D_A(void);
extern void Std_D_A(void);
extern void Ldu_D_A(void);
extern void Stu_D_A(void);
extern void Subb_X_A(void);
extern void Cmpb_X_A(void);
extern void Sbcb_X_A(void);
extern void Addd_X_A(void);
extern void Andb_X_A(void);
extern void Bitb_X_A(void);
extern void Ldb_X_A(void);
extern void Stb_X_A(void);
extern void Eorb_X_A(void);
extern void Adcb_X_A(void);
extern void Orb_X_A(void);
extern void Addb_X_A(void);
extern void Ldd_X_A(void);
extern void Std_X_A(void);
extern void Ldu_X_A(void);
extern void Stu_X_A(void);
extern void Subb_E_A(void);
extern void Cmpb_E_A(void);
extern void Sbcb_E_A(void);
extern void Addd_E_A(void);
extern void Andb_E_A(void);
extern void Bitb_E_A(void);
extern void Ldb_E_A(void);
extern void Stb_E_A(void);
extern void Eorb_E_A(void);
extern void Adcb_E_A(void);
extern void Orb_E_A(void);
extern void Addb_E_A(void);
extern void Ldd_E_A(void);
extern void Std_E_A(void);
extern void Ldu_E_A(void);
extern void Stu_E_A(void);
extern void LBrn_R_A(void);
extern void LBhi_R_A(void);
extern void LBls_R_A(void);
extern void LBhs_R_A(void);
extern void LBcs_R_A(void);
extern void LBne_R_A(void);
extern void LBeq_R_A(void);
extern void LBvc_R_A(void);
extern void LBvs_R_A(void);
extern void LBpl_R_A(void);
extern void LBmi_R_A(void);
extern void LBge_R_A(void);
extern void LBlt_R_A(void);
extern void LBgt_R_A(void);
extern void LBle_R_A(void);
extern void Addr_A(void);
extern void Adcr_A(void);
extern void Subr_A(void);
extern void Sbcr_A(void);
extern void Andr_A(void);
extern void Orr_A(void);
extern void Eorr_A(void);
extern void Cmpr_A(void);
extern void Pshsw_A(void);
extern void Pulsw_A(void);
extern void Pshuw_A(void);
extern void Puluw_A(void);
extern void Swi2_I_A(void);
extern void Negd_I_A(void);
extern void Comd_I_A(void);
extern void Lsrd_I_A(void);
extern void Rord_I_A(void);
extern void Asrd_I_A(void);
extern void Asld_I_A(void);
extern void Rold_I_A(void);
extern void Decd_I_A(void);
extern void Incd_I_A(void);
extern void Tstd_I_A(void);
extern void Clrd_I_A(void);
extern void Comw_I_A(void);
extern void Lsrw_I_A(void);
extern void Rorw_I_A(void);
extern void Rolw_I_A(void);
extern void Decw_I_A(void);
extern void Incw_I_A(void);
extern void Tstw_I_A(void);
extern void Clrw_I_A(void);
extern void Subw_M_A(void);
extern void Cmpw_M_A(void);
extern void Sbcd_M_A(void);
extern void Cmpd_M_A(void);
extern void Andd_M_A(void);
extern void Bitd_M_A(void);
extern void Ldw_M_A(void);
extern void Eord_M_A(void);
extern void Adcd_M_A(void);
extern void Ord_M_A(void);
extern void Addw_M_A(void);
extern void Cmpy_M_A(void);
extern void Ldy_M_A(void);
extern void Subw_D_A(void);
extern void Cmpw_D_A(void);
extern void Sbcd_D_A(void);
extern void Cmpd_D_A(void);
extern void Andd_D_A(void);
extern void Bitd_D_A(void);
extern void Ldw_D_A(void);
extern void Stw_D_A(void);
extern void Eord_D_A(void);
extern void Adcd_D_A(void);
extern void Ord_D_A(void);
extern void Addw_D_A(void);
extern void Cmpy_D_A(void);
extern void Ldy_D_A(void);
extern void Sty_D_A(void);
extern void Subw_X_A(void);
extern void Cmpw_X_A(void);
extern void Sbcd_X_A(void);
extern void Cmpd_X_A(void);
extern void Andd_X_A(void);
extern void Bitd_X_A(void);
extern void Ldw_X_A(void);
extern void Stw_X_A(void);
extern void Eord_X_A(void);
extern void Adcd_X_A(void);
extern void Ord_X_A(void);
extern void Addw_X_A(void);
extern void Cmpy_X_A(void);
extern void Ldy_X_A(void);
extern void Sty_X_A(void);
extern void Subw_E_A(void);
extern void Cmpw_E_A(void);
extern void Sbcd_E_A(void);
extern void Cmpd_E_A(void);
extern void Andd_E_A(void);
extern void Bitd_E_A(void);
extern void Ldw_E_A(void);
extern void Stw_E_A(void);
extern void Eord_E_A(void);
extern void Adcd_E_A(void);
extern void Ord_E_A(void);
extern void Addw_E_A(void);
extern void Cmpy_E_A(void);
extern void Ldy_E_A(void);
extern void Sty_E_A(void);
extern void Lds_M_A(void);
extern void Ldq_D_A(void);
extern void Stq_D_A(void);
extern void Lds_D_A(void);
extern void Sts_D_A(void);
extern void Ldq_X_A(void);
extern void Stq_X_A(void);
extern void Lds_X_A(void);
extern void Sts_X_A(void);
extern void Ldq_E_A(void);
extern void Stq_E_A(void);
extern void Lds_E_A(void);
extern void Sts_E_A(void);
extern void Band_A(void);
extern void Biand_A(void);
extern void Bor_A(void);
extern void Bior_A(void);
extern void Beor_A(void);
extern void Bieor_A(void);
extern void Ldbt_A(void);
extern void Stbt_A(void);
extern void Tfm1_A(void);
extern void Tfm2_A(void);
extern void Tfm3_A(void);
extern void Tfm4_A(void);
extern void Bitmd_M_A(void);
extern void Ldmd_M_A(void);
extern void Swi3_I_A(void);
extern void Come_I_A(void);
extern void Dece_I_A(void);
extern void Ince_I_A(void);
extern void Tste_I_A(void);
extern void Clre_I_A(void);
extern void Comf_I_A(void);
extern void Decf_I_A(void);
extern void Incf_I_A(void);
extern void Tstf_I_A(void);
extern void Clrf_I_A(void);
extern void Sube_M_A(void);
extern void Cmpe_M_A(void);
extern void Cmpu_M_A(void);
extern void Lde_M_A(void);
extern void Adde_M_A(void);
extern void Cmps_M_A(void);
extern void Divd_M_A(void);
extern void Divq_M_A(void);
extern void Muld_M_A(void);
extern void Sube_D_A(void);
extern void Cmpe_D_A(void);
extern void Cmpu_D_A(void);
extern void Lde_D_A(void);
extern void Ste_D_A(void);
extern void Adde_D_A(void);
extern void Cmps_D_A(void);
extern void Divd_D_A(void);
extern void Divq_D_A(void);
extern void Muld_D_A(void);
extern void Sube_X_A(void);
extern void Cmpe_X_A(void);
extern void Cmpu_X_A(void);
extern void Lde_X_A(void);
extern void Ste_X_A(void);
extern void Adde_X_A(void);
extern void Cmps_X_A(void);
extern void Divd_X_A(void);
extern void Divq_X_A(void);
extern void Muld_X_A(void);
extern void Sube_E_A(void);
extern void Cmpe_E_A(void);
extern void Cmpu_E_A(void);
extern void Lde_E_A(void);
extern void Ste_E_A(void);
extern void Adde_E_A(void);
extern void Cmps_E_A(void);
extern void Divd_E_A(void);
extern void Divq_E_A(void);
extern void Muld_E_A(void);
extern void Subf_M_A(void);
extern void Cmpf_M_A(void);
extern void Ldf_M_A(void);
extern void Addf_M_A(void);
extern void Subf_D_A(void);
extern void Cmpf_D_A(void);
extern void Ldf_D_A(void);
extern void Stf_D_A(void);
extern void Addf_D_A(void);
extern void Subf_X_A(void);
extern void Cmpf_X_A(void);
extern void Ldf_X_A(void);
extern void Stf_X_A(void);
extern void Addf_X_A(void);
extern void Subf_E_A(void);
extern void Cmpf_E_A(void);
extern void Ldf_E_A(void);
extern void Stf_E_A(void);
extern void Addf_E_A(void);

#define CFx68 0x01
#define VFx68 0x02
#define ZFx68 0x04
#define NFx68 0x08
#define IFx68 0x10
#define HFx68 0x20
#define FFx68 0x40
#define EFx68 0x80
#define CFx86 0x01
#define HFx86 0x10
#define ZFx86 0x40
#define NFx86 0x80
#define VFx86 0x800

UINT16 x86Flags;
UINT8 SyncWaiting_s = 0;
unsigned short *xfreg16_s[8];
unsigned char *ureg8_s[8];

wideregister q_s;
cpuregister pc_s, x_s, y_s, u_s, s_s, dp_s, v_s, z_s;

#define MD_NATIVE6309_BIT 0x01
#define MD_FIRQMODE_BIT 0x02
#define MD_ILLEGALINST_BIT 0x40
#define MD_DIVBYZERO_BIT 0x80

#define MD_NATIVE6309 (mdbits_s & MD_NATIVE6309_BIT)
#define MD_FIRQMODE (mdbits_s & MD_FIRQMODE_BIT)
#define MD_ILLEGALINST (mdbits_s & MD_ILLEGALINST_BIT)
#define MD_DIVBYZERO (mdbits_s & MD_DIVBYZERO_BIT)

static unsigned char InsCycles[2][25];
unsigned char cc_s[8];
//static unsigned int md_s[8];
//static unsigned char *ureg8[8]; 
//static unsigned char ccbits,mdbits;
unsigned char ccbits_s, mdbits_s;
//static unsigned short *xfreg16[8];
static int CycleCounter=0;
//static unsigned int SyncWaiting=0;
unsigned short temp16;
static signed short stemp16;
static signed char stemp8;
static unsigned int  temp32;
static unsigned char temp8; 
static unsigned char PendingInterupts=0;
static unsigned char IRQWaiter=0;
static unsigned char Source=0,Dest=0;
static unsigned char postbyte=0;
static short unsigned postword=0;
static signed char *spostbyte=(signed char *)&postbyte;
static signed short *spostword=(signed short *)&postword;
char InInterupt_s=0;
static int gCycleFor;
//END Global variables for CPU Emulation-------------------

//Fuction Prototypes---------------------------------------
void IgnoreInsHandler(void);
unsigned short CalculateEA_s(unsigned char);
extern void MSABI InvalidInsHandler_s(void);
void MSABI DivbyZero_s(void);
void MSABI ErrorVector_s(void);
void setcc_s(UINT8);
UINT8 getcc_s(void);
void setmd_s(UINT8);
UINT8 getmd_s(void);
static void cpu_firq_s(void);
static void cpu_irq_s(void);
static void cpu_nmi_s(void);
unsigned char GetSorceReg_s(unsigned char);
void Page_2_s(void);
void Page_3_s(void);
void MSABI MemWrite8_s(unsigned char, unsigned short);
void MSABI MemWrite16_s(unsigned short, unsigned short);
void MSABI MemWrite32_s(unsigned int, unsigned short);
// unsigned char MemRead8_s(unsigned short);
// unsigned short MemRead16_s(unsigned short);
// unsigned int MemRead32_s(unsigned short);
extern UINT8 /*MemRead8(UINT16),*/ MSABI MemRead8_s(UINT16);
extern UINT16 /*MemRead16(UINT16),*/ MSABI MemRead16_s(UINT16);
extern UINT32 /*MemRead32(UINT16),*/ MSABI MemRead32_s(UINT16);

//unsigned char GetDestReg(unsigned char);
//END Fuction Prototypes-----------------------------------

void HD6309Reset_s(void)
{
	char index;
	for(index=0;index<=6;index++)		//Set all register to 0 except V
		*xfreg16_s[index] = 0;
	for(index=0;index<=7;index++)
		*ureg8_s[index]=0;
	x86Flags = 0;
	//for(index=0;index<=7;index++)
	//	md_s[index]=0;
	mdbits_s =getmd_s();
	mdbits_s = 0;
	dp_s.Reg=0;
	cc_s[I]=1;
	cc_s[F]=1;
	SyncWaiting_s=0;
	PC_REG=MemRead16_s(VRESET);	//PC gets its reset vector // this needs to be uncommented
	SetMapType(0);	//shouldn't be here
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

void(*JmpVec1_s[256])(void) = {
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
	Page_2_s,		// 10
	Page_3_s,		// 11
	Nop_I_A,		// 12
	Sync_I_A,		// 13
	Sexw_I_A,		// 14
	InvalidInsHandler,	// 15
	Lbra_R_A,		// 16
	Lbsr_R_A,		// 17
	InvalidInsHandler,	// 18
	Daa_I_A,		// 19
	Orcc_M_A,		// 1A
	InvalidInsHandler,	// 1B
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
	InvalidInsHandler,	// 38
	Rts_I_A,		// 39
	Abx_I_A,		// 3A
	Rti_I_A,		// 3B
	Cwai_I_A,		// 3C
	Mul_I_A,		// 3D
	IgnoreInsHandler,	// Reset,		// 3E
	Swi1_I_A,		// 3F
	Nega_I_A,		// 40
	IgnoreInsHandler,	// InvalidInsHandler,	// 41
	IgnoreInsHandler,	// InvalidInsHandler,	// 42
	Coma_I_A,		// 43
	Lsra_I_A,		// 44
	IgnoreInsHandler,	// InvalidInsHandler,	// 45
	Rora_I_A,		// 46
	Asra_I_A,		// 47
	Asla_I_A,		// 48
	Rola_I_A,		// 49
	Deca_I_A,		// 4A
	IgnoreInsHandler,	// InvalidInsHandler,	// 4B
	Inca_I_A,		// 4C
	Tsta_I_A,		// 4D
	IgnoreInsHandler,	// InvalidInsHandler,	// 4E
	Clra_I_A,		// 4F
	Negb_I_A,		// 50
	IgnoreInsHandler,	// InvalidInsHandler,	// 51
	IgnoreInsHandler,	// InvalidInsHandler,	// 52
	Comb_I_A,		// 53
	Lsrb_I_A,		// 54
	InvalidInsHandler,	// 55
	Rorb_I_A,		// 56
	Asrb_I_A,		// 57
	Aslb_I_A,		// 58
	Rolb_I_A,		// 59
	Decb_I_A,		// 5A
	InvalidInsHandler,	// 5B
	Incb_I_A,		// 5C
	Tstb_I_A,		// 5D
	IgnoreInsHandler,	// InvalidInsHandler,	// 5E
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
	IgnoreInsHandler,	// InvalidInsHandler,	// 87
	Eora_M_A,		// 88
	Adca_M_A,		// 89
	Ora_M_A,		// 8A
	Adda_M_A,		// 8B
	Cmpx_M_A,		// 8C
	Bsr_R_A,		// 8D
	Ldx_M_A,		// 8E
	IgnoreInsHandler,	// InvalidInsHandler,	// 8F
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
	Bsr_D_A,		// 9D
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
	Bsr_X_A,		// AD
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
	Bsr_E_A,		// BD
	Ldx_E_A,		// BE
	Stx_E_A,		// BF
	Subb_M_A,		// C0
	Cmpb_M_A,		// C1
	Sbcb_M_A,		// C2
	Addd_M_A,		// C3
	Andb_M_A,		// C4
	Bitb_M_A,		// C5
	Ldb_M_A,		// C6
	IgnoreInsHandler,	// InvalidInsHandler,		// C7
	Eorb_M_A,		// C8
	Adcb_M_A,		// C9
	Orb_M_A,		// CA
	Addb_M_A,		// CB
	Ldd_M_A,		// CC
	Ldq_M_A,		// CD
	Ldu_M_A,		// CE
	IgnoreInsHandler,	// InvalidInsHandler,		// CF
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

void(*JmpVec2_s[256])(void) = {
	IgnoreInsHandler,	// InvalidInsHandler,		// 00
	IgnoreInsHandler,	// InvalidInsHandler,		// 01
	IgnoreInsHandler,	// InvalidInsHandler,		// 02
	IgnoreInsHandler,	// InvalidInsHandler,		// 03
	IgnoreInsHandler,	// InvalidInsHandler,		// 04
	IgnoreInsHandler,	// InvalidInsHandler,		// 05
	IgnoreInsHandler,	// InvalidInsHandler,		// 06
	IgnoreInsHandler,	// InvalidInsHandler,		// 07
	IgnoreInsHandler,	// InvalidInsHandler,		// 08
	IgnoreInsHandler,	// InvalidInsHandler,		// 09
	IgnoreInsHandler,	// InvalidInsHandler,		// 0A
	IgnoreInsHandler,	// InvalidInsHandler,		// 0B
	IgnoreInsHandler,	// InvalidInsHandler,		// 0C
	IgnoreInsHandler,	// InvalidInsHandler,		// 0D
	IgnoreInsHandler,	// InvalidInsHandler,		// 0E
	IgnoreInsHandler,	// InvalidInsHandler,		// 0F
	IgnoreInsHandler,	// InvalidInsHandler,		// 10
	IgnoreInsHandler,	// InvalidInsHandler,		// 11
	IgnoreInsHandler,	// InvalidInsHandler,		// 12
	IgnoreInsHandler,	// InvalidInsHandler,		// 13
	IgnoreInsHandler,	// InvalidInsHandler,		// 14
	IgnoreInsHandler,	// InvalidInsHandler,		// 15
	IgnoreInsHandler,	// InvalidInsHandler,		// 16
	IgnoreInsHandler,	// InvalidInsHandler,		// 17
	IgnoreInsHandler,	// InvalidInsHandler,		// 18
	IgnoreInsHandler,	// InvalidInsHandler,		// 19
	IgnoreInsHandler,	// InvalidInsHandler,		// 1A
	IgnoreInsHandler,	// InvalidInsHandler,		// 1B
	IgnoreInsHandler,	// InvalidInsHandler,		// 1C
	IgnoreInsHandler,	// InvalidInsHandler,		// 1D
	IgnoreInsHandler,	// InvalidInsHandler,		// 1E
	IgnoreInsHandler,	// InvalidInsHandler,		// 1F
	IgnoreInsHandler,	// InvalidInsHandler,		// 20
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
	IgnoreInsHandler,	// InvalidInsHandler,		// 3C
	IgnoreInsHandler,	// InvalidInsHandler,		// 3D
	IgnoreInsHandler,	// InvalidInsHandler,		// 3E
	Swi2_I_A,		// 3F
	Negd_I_A,		// 40
	IgnoreInsHandler,	// InvalidInsHandler,		// 41
	IgnoreInsHandler,	// InvalidInsHandler,		// 42
	Comd_I_A,		// 43
	Lsrd_I_A,		// 44
	IgnoreInsHandler,	// InvalidInsHandler,		// 45
	Rord_I_A,		// 46
	Asrd_I_A,		// 47
	Asld_I_A,		// 48
	Rold_I_A,		// 49
	Decd_I_A,		// 4A
	IgnoreInsHandler,	// InvalidInsHandler,		// 4B
	Incd_I_A,		// 4C
	Tstd_I_A,		// 4D
	IgnoreInsHandler,	// InvalidInsHandler,		// 4E
	Clrd_I_A,		// 4F
	IgnoreInsHandler,	// InvalidInsHandler,		// 50
	IgnoreInsHandler,	// InvalidInsHandler,		// 51
	IgnoreInsHandler,	// InvalidInsHandler,		// 52
	Comw_I_A,		// 53
	Lsrw_I_A,		// 54
	IgnoreInsHandler,	// InvalidInsHandler,		// 55
	Rorw_I_A,		// 56
	IgnoreInsHandler,	// InvalidInsHandler,		// 57
	IgnoreInsHandler,	// InvalidInsHandler,		// 58
	Rolw_I_A,		// 59
	Decw_I_A,		// 5A
	IgnoreInsHandler,	// InvalidInsHandler,		// 5B
	Incw_I_A,		// 5C
	Tstw_I_A,		// 5D
	IgnoreInsHandler,	// InvalidInsHandler,		// 5E
	Clrw_I_A,		// 5F
	IgnoreInsHandler,	// InvalidInsHandler,		// 60
	IgnoreInsHandler,	// InvalidInsHandler,		// 61
	IgnoreInsHandler,	// InvalidInsHandler,		// 62
	IgnoreInsHandler,	// InvalidInsHandler,		// 63
	IgnoreInsHandler,	// InvalidInsHandler,		// 64
	IgnoreInsHandler,	// InvalidInsHandler,		// 65
	IgnoreInsHandler,	// InvalidInsHandler,		// 66
	IgnoreInsHandler,	// InvalidInsHandler,		// 67
	IgnoreInsHandler,	// InvalidInsHandler,		// 68
	IgnoreInsHandler,	// InvalidInsHandler,		// 69
	IgnoreInsHandler,	// InvalidInsHandler,		// 6A
	IgnoreInsHandler,	// InvalidInsHandler,		// 6B
	IgnoreInsHandler,	// InvalidInsHandler,		// 6C
	IgnoreInsHandler,	// InvalidInsHandler,		// 6D
	IgnoreInsHandler,	// InvalidInsHandler,		// 6E
	IgnoreInsHandler,	// InvalidInsHandler,		// 6F
	IgnoreInsHandler,	// InvalidInsHandler,		// 70
	IgnoreInsHandler,	// InvalidInsHandler,		// 71
	IgnoreInsHandler,	// InvalidInsHandler,		// 72
	IgnoreInsHandler,	// InvalidInsHandler,		// 73
	IgnoreInsHandler,	// InvalidInsHandler,		// 74
	IgnoreInsHandler,	// InvalidInsHandler,		// 75
	IgnoreInsHandler,	// InvalidInsHandler,		// 76
	IgnoreInsHandler,	// InvalidInsHandler,		// 77
	IgnoreInsHandler,	// InvalidInsHandler,		// 78
	IgnoreInsHandler,	// InvalidInsHandler,		// 79
	IgnoreInsHandler,	// InvalidInsHandler,		// 7A
	IgnoreInsHandler,	// InvalidInsHandler,		// 7B
	IgnoreInsHandler,	// InvalidInsHandler,		// 7C
	IgnoreInsHandler,	// InvalidInsHandler,		// 7D
	IgnoreInsHandler,	// InvalidInsHandler,		// 7E
	IgnoreInsHandler,	// InvalidInsHandler,		// 7F
	Subw_M_A,		// 80
	Cmpw_M_A,		// 81
	Sbcd_M_A,		// 82
	Cmpd_M_A,		// 83
	Andd_M_A,		// 84
	Bitd_M_A,		// 85
	Ldw_M_A,		// 86
	IgnoreInsHandler,	// InvalidInsHandler,		// 87
	Eord_M_A,		// 88
	Adcd_M_A,		// 89
	Ord_M_A,		// 8A
	Addw_M_A,		// 8B
	Cmpy_M_A,		// 8C
	IgnoreInsHandler,	// InvalidInsHandler,		// 8D
	Ldy_M_A,		// 8E
	IgnoreInsHandler,	// InvalidInsHandler,		// 8F
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
	IgnoreInsHandler,	// InvalidInsHandler,		// 9D
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
	IgnoreInsHandler,	// InvalidInsHandler,		// AD
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
	IgnoreInsHandler,	// InvalidInsHandler,		// BD
	Ldy_E_A,		// BE
	Sty_E_A,		// BF
	IgnoreInsHandler,	// InvalidInsHandler,		// C0
	IgnoreInsHandler,	// InvalidInsHandler,		// C1
	IgnoreInsHandler,	// InvalidInsHandler,		// C2
	IgnoreInsHandler,	// InvalidInsHandler,		// C3
	IgnoreInsHandler,	// InvalidInsHandler,		// C4
	IgnoreInsHandler,	// InvalidInsHandler,		// C5
	IgnoreInsHandler,	// InvalidInsHandler,		// C6
	IgnoreInsHandler,	// InvalidInsHandler,		// C7
	IgnoreInsHandler,	// InvalidInsHandler,		// C8
	IgnoreInsHandler,	// InvalidInsHandler,		// C9
	IgnoreInsHandler,	// InvalidInsHandler,		// CA
	IgnoreInsHandler,	// InvalidInsHandler,		// CB
	IgnoreInsHandler,	// InvalidInsHandler,		// CC
	IgnoreInsHandler,	// InvalidInsHandler,		// CD
	Lds_M_A,		// CE - was Lds_I
	IgnoreInsHandler,	// InvalidInsHandler,		// CF
	IgnoreInsHandler,	// InvalidInsHandler,		// D0
	IgnoreInsHandler,	// InvalidInsHandler,		// D1
	IgnoreInsHandler,	// InvalidInsHandler,		// D2
	IgnoreInsHandler,	// InvalidInsHandler,		// D3
	IgnoreInsHandler,	// InvalidInsHandler,		// D4
	IgnoreInsHandler,	// InvalidInsHandler,		// D5
	IgnoreInsHandler,	// InvalidInsHandler,		// D6
	IgnoreInsHandler,	// InvalidInsHandler,		// D7
	IgnoreInsHandler,	// InvalidInsHandler,		// D8
	IgnoreInsHandler,	// InvalidInsHandler,		// D9
	IgnoreInsHandler,	// InvalidInsHandler,		// DA
	IgnoreInsHandler,	// InvalidInsHandler,		// DB
	Ldq_D_A,		// DC
	Stq_D_A,		// DD
	Lds_D_A,		// DE
	Sts_D_A,		// DF
	IgnoreInsHandler,	// InvalidInsHandler,		// E0
	IgnoreInsHandler,	// InvalidInsHandler,		// E1
	IgnoreInsHandler,	// InvalidInsHandler,		// E2
	IgnoreInsHandler,	// InvalidInsHandler,		// E3
	IgnoreInsHandler,	// InvalidInsHandler,		// E4
	IgnoreInsHandler,	// InvalidInsHandler,		// E5
	IgnoreInsHandler,	// InvalidInsHandler,		// E6
	IgnoreInsHandler,	// InvalidInsHandler,		// E7
	IgnoreInsHandler,	// InvalidInsHandler,		// E8
	IgnoreInsHandler,	// InvalidInsHandler,		// E9
	IgnoreInsHandler,	// InvalidInsHandler,		// EA
	IgnoreInsHandler,	// InvalidInsHandler,		// EB
	Ldq_X_A,		// EC
	Stq_X_A,		// ED
	Lds_X_A,		// EE
	Sts_X_A,		// EF
	IgnoreInsHandler,	// InvalidInsHandler,		// F0
	IgnoreInsHandler,	// InvalidInsHandler,		// F1
	IgnoreInsHandler,	// InvalidInsHandler,		// F2
	IgnoreInsHandler,	// InvalidInsHandler,		// F3
	IgnoreInsHandler,	// InvalidInsHandler,		// F4
	IgnoreInsHandler,	// InvalidInsHandler,		// F5
	IgnoreInsHandler,	// InvalidInsHandler,		// F6
	IgnoreInsHandler,	// InvalidInsHandler,		// F7
	IgnoreInsHandler,	// InvalidInsHandler,		// F8
	IgnoreInsHandler,	// InvalidInsHandler,		// F9
	IgnoreInsHandler,	// InvalidInsHandler,		// FA
	IgnoreInsHandler,	// InvalidInsHandler,		// FB
	Ldq_E_A,		// FC
	Stq_E_A,		// FD
	Lds_E_A,		// FE
	Sts_E_A,		// FF
};

void(*JmpVec3_s[256])(void) = {
	IgnoreInsHandler,	// InvalidInsHandler,		// 00
	IgnoreInsHandler,	// InvalidInsHandler,		// 01
	IgnoreInsHandler,	// InvalidInsHandler,		// 02
	IgnoreInsHandler,	// InvalidInsHandler,		// 03
	IgnoreInsHandler,	// InvalidInsHandler,		// 04
	IgnoreInsHandler,	// InvalidInsHandler,		// 05
	IgnoreInsHandler,	// InvalidInsHandler,		// 06
	IgnoreInsHandler,	// InvalidInsHandler,		// 07
	IgnoreInsHandler,	// InvalidInsHandler,		// 08
	IgnoreInsHandler,	// InvalidInsHandler,		// 09
	IgnoreInsHandler,	// InvalidInsHandler,		// 0A
	IgnoreInsHandler,	// InvalidInsHandler,		// 0B
	IgnoreInsHandler,	// InvalidInsHandler,		// 0C
	IgnoreInsHandler,	// InvalidInsHandler,		// 0D
	IgnoreInsHandler,	// InvalidInsHandler,		// 0E
	IgnoreInsHandler,	// InvalidInsHandler,		// 0F
	IgnoreInsHandler,	// InvalidInsHandler,		// 10
	IgnoreInsHandler,	// InvalidInsHandler,		// 11
	IgnoreInsHandler,	// InvalidInsHandler,		// 12
	IgnoreInsHandler,	// InvalidInsHandler,		// 13
	IgnoreInsHandler,	// InvalidInsHandler,		// 14
	IgnoreInsHandler,	// InvalidInsHandler,		// 15
	IgnoreInsHandler,	// InvalidInsHandler,		// 16
	IgnoreInsHandler,	// InvalidInsHandler,		// 17
	IgnoreInsHandler,	// InvalidInsHandler,		// 18
	IgnoreInsHandler,	// InvalidInsHandler,		// 19
	IgnoreInsHandler,	// InvalidInsHandler,		// 1A
	IgnoreInsHandler,	// InvalidInsHandler,		// 1B
	IgnoreInsHandler,	// InvalidInsHandler,		// 1C
	IgnoreInsHandler,	// InvalidInsHandler,		// 1D
	IgnoreInsHandler,	// InvalidInsHandler,		// 1E
	IgnoreInsHandler,	// InvalidInsHandler,		// 1F
	IgnoreInsHandler,	// InvalidInsHandler,		// 20
	IgnoreInsHandler,	// InvalidInsHandler,		// 21
	IgnoreInsHandler,	// InvalidInsHandler,		// 22
	IgnoreInsHandler,	// InvalidInsHandler,		// 23
	IgnoreInsHandler,	// InvalidInsHandler,		// 24
	IgnoreInsHandler,	// InvalidInsHandler,		// 25
	IgnoreInsHandler,	// InvalidInsHandler,		// 26
	IgnoreInsHandler,	// InvalidInsHandler,		// 27
	IgnoreInsHandler,	// InvalidInsHandler,		// 28
	IgnoreInsHandler,	// InvalidInsHandler,		// 29
	IgnoreInsHandler,	// InvalidInsHandler,		// 2A
	IgnoreInsHandler,	// InvalidInsHandler,		// 2B
	IgnoreInsHandler,	// InvalidInsHandler,		// 2C
	IgnoreInsHandler,	// InvalidInsHandler,		// 2D
	IgnoreInsHandler,	// InvalidInsHandler,		// 2E
	IgnoreInsHandler,	// InvalidInsHandler,		// 2F
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
	IgnoreInsHandler,	// InvalidInsHandler,		// 3E
	Swi3_I_A,		// 3F
	IgnoreInsHandler,	// InvalidInsHandler,		// 40
	IgnoreInsHandler,	// InvalidInsHandler,		// 41
	IgnoreInsHandler,	// InvalidInsHandler,		// 42
	Come_I_A,		// 43
	IgnoreInsHandler,	// InvalidInsHandler,		// 44
	IgnoreInsHandler,	// InvalidInsHandler,		// 45
	IgnoreInsHandler,	// InvalidInsHandler,		// 46
	IgnoreInsHandler,	// InvalidInsHandler,		// 47
	IgnoreInsHandler,	// InvalidInsHandler,		// 48
	IgnoreInsHandler,	// InvalidInsHandler,		// 49
	Dece_I_A,		// 4A
	IgnoreInsHandler,	// InvalidInsHandler,		// 4B
	Ince_I_A,		// 4C
	Tste_I_A,		// 4D
	IgnoreInsHandler,	// InvalidInsHandler,		// 4E
	Clre_I_A,		// 4F
	IgnoreInsHandler,	// InvalidInsHandler,		// 50
	IgnoreInsHandler,	// InvalidInsHandler,		// 51
	IgnoreInsHandler,	// InvalidInsHandler,		// 52
	Comf_I_A,		// 53
	IgnoreInsHandler,	// InvalidInsHandler,		// 54
	IgnoreInsHandler,	// InvalidInsHandler,		// 55
	IgnoreInsHandler,	// InvalidInsHandler,		// 56
	IgnoreInsHandler,	// InvalidInsHandler,		// 57
	IgnoreInsHandler,	// InvalidInsHandler,		// 58
	IgnoreInsHandler,	// InvalidInsHandler,		// 59
	Decf_I_A,		// 5A
	IgnoreInsHandler,	// InvalidInsHandler,		// 5B
	Incf_I_A,		// 5C
	Tstf_I_A,		// 5D
	IgnoreInsHandler,	// InvalidInsHandler,		// 5E
	Clrf_I_A,		// 5F
	IgnoreInsHandler,	// InvalidInsHandler,		// 60
	IgnoreInsHandler,	// InvalidInsHandler,		// 61
	IgnoreInsHandler,	// InvalidInsHandler,		// 62
	IgnoreInsHandler,	// InvalidInsHandler,		// 63
	IgnoreInsHandler,	// InvalidInsHandler,		// 64
	IgnoreInsHandler,	// InvalidInsHandler,		// 65
	IgnoreInsHandler,	// InvalidInsHandler,		// 66
	IgnoreInsHandler,	// InvalidInsHandler,		// 67
	IgnoreInsHandler,	// InvalidInsHandler,		// 68
	IgnoreInsHandler,	// InvalidInsHandler,		// 69
	IgnoreInsHandler,	// InvalidInsHandler,		// 6A
	IgnoreInsHandler,	// InvalidInsHandler,		// 6B
	IgnoreInsHandler,	// InvalidInsHandler,		// 6C
	IgnoreInsHandler,	// InvalidInsHandler,		// 6D
	IgnoreInsHandler,	// InvalidInsHandler,		// 6E
	IgnoreInsHandler,	// InvalidInsHandler,		// 6F
	IgnoreInsHandler,	// InvalidInsHandler,		// 70
	IgnoreInsHandler,	// InvalidInsHandler,		// 71
	IgnoreInsHandler,	// InvalidInsHandler,		// 72
	IgnoreInsHandler,	// InvalidInsHandler,		// 73
	IgnoreInsHandler,	// InvalidInsHandler,		// 74
	IgnoreInsHandler,	// InvalidInsHandler,		// 75
	IgnoreInsHandler,	// InvalidInsHandler,		// 76
	IgnoreInsHandler,	// InvalidInsHandler,		// 77
	IgnoreInsHandler,	// InvalidInsHandler,		// 78
	IgnoreInsHandler,	// InvalidInsHandler,		// 79
	IgnoreInsHandler,	// InvalidInsHandler,		// 7A
	IgnoreInsHandler,	// InvalidInsHandler,		// 7B
	IgnoreInsHandler,	// InvalidInsHandler,		// 7C
	IgnoreInsHandler,	// InvalidInsHandler,		// 7D
	IgnoreInsHandler,	// InvalidInsHandler,		// 7E
	IgnoreInsHandler,	// InvalidInsHandler,		// 7F
	Sube_M_A,		// 80
	Cmpe_M_A,		// 81
	IgnoreInsHandler,	// InvalidInsHandler,		// 82
	Cmpu_M_A,		// 83
	IgnoreInsHandler,	// InvalidInsHandler,		// 84
	IgnoreInsHandler,	// InvalidInsHandler,		// 85
	Lde_M_A,		// 86
	IgnoreInsHandler,	// InvalidInsHandler,		// 87
	IgnoreInsHandler,	// InvalidInsHandler,		// 88
	IgnoreInsHandler,	// InvalidInsHandler,		// 89
	IgnoreInsHandler,	// InvalidInsHandler,		// 8A
	Adde_M_A,		// 8B
	Cmps_M_A,		// 8C
	Divd_M_A,		// 8D
	Divq_M_A,		// 8E
	Muld_M_A,		// 8F
	Sube_D_A,		// 90
	Cmpe_D_A,		// 91
	IgnoreInsHandler,	// InvalidInsHandler,		// 92
	Cmpu_D_A,		// 93
	IgnoreInsHandler,	// InvalidInsHandler,		// 94
	IgnoreInsHandler,	// InvalidInsHandler,		// 95
	Lde_D_A,		// 96
	Ste_D_A,		// 97
	IgnoreInsHandler,	// InvalidInsHandler,		// 98
	IgnoreInsHandler,	// InvalidInsHandler,		// 99
	IgnoreInsHandler,	// InvalidInsHandler,		// 9A
	Adde_D_A,		// 9B
	Cmps_D_A,		// 9C
	Divd_D_A,		// 9D
	Divq_D_A,		// 9E
	Muld_D_A,		// 9F
	Sube_X_A,		// A0
	Cmpe_X_A,		// A1
	IgnoreInsHandler,	// InvalidInsHandler,		// A2
	Cmpu_X_A,		// A3
	IgnoreInsHandler,	// InvalidInsHandler,		// A4
	IgnoreInsHandler,	// InvalidInsHandler,		// A5
	Lde_X_A,		// A6
	Ste_X_A,		// A7
	IgnoreInsHandler,	// InvalidInsHandler,		// A8
	IgnoreInsHandler,	// InvalidInsHandler,		// A9
	IgnoreInsHandler,	// InvalidInsHandler,		// AA
	Adde_X_A,		// AB
	Cmps_X_A,		// AC
	Divd_X_A,		// AD
	Divq_X_A,		// AE
	Muld_X_A,		// AF
	Sube_E_A,		// B0
	Cmpe_E_A,		// B1
	IgnoreInsHandler,	// InvalidInsHandler,		// B2
	Cmpu_E_A,		// B3
	IgnoreInsHandler,	// InvalidInsHandler,		// B4
	IgnoreInsHandler,	// InvalidInsHandler,		// B5
	Lde_E_A,		// B6
	Ste_E_A,		// B7
	IgnoreInsHandler,	// InvalidInsHandler,		// B8
	IgnoreInsHandler,	// InvalidInsHandler,		// B9
	IgnoreInsHandler,	// InvalidInsHandler,		// BA
	Adde_E_A,		// BB
	Cmps_E_A,		// BC
	Divd_E_A,		// BD
	Divq_E_A,		// BE
	Muld_E_A,		// BF
	Subf_M_A,		// C0
	Cmpf_M_A,		// C1
	IgnoreInsHandler,	// InvalidInsHandler,		// C2
	IgnoreInsHandler,	// InvalidInsHandler,		// C3
	IgnoreInsHandler,	// InvalidInsHandler,		// C4
	IgnoreInsHandler,	// InvalidInsHandler,		// C5
	Ldf_M_A,		// C6
	IgnoreInsHandler,	// InvalidInsHandler,		// C7
	IgnoreInsHandler,	// InvalidInsHandler,		// C8
	IgnoreInsHandler,	// InvalidInsHandler,		// C9
	IgnoreInsHandler,	// InvalidInsHandler,		// CA
	Addf_M_A,		// CB
	IgnoreInsHandler,	// InvalidInsHandler,		// CC
	IgnoreInsHandler,	// InvalidInsHandler,		// CD
	IgnoreInsHandler,	// InvalidInsHandler,		// CE
	IgnoreInsHandler,	// InvalidInsHandler,		// CF
	Subf_D_A,		// D0
	Cmpf_D_A,		// D1
	IgnoreInsHandler,	// InvalidInsHandler,		// D2
	IgnoreInsHandler,	// InvalidInsHandler,		// D3
	IgnoreInsHandler,	// InvalidInsHandler,		// D4
	IgnoreInsHandler,	// InvalidInsHandler,		// D5
	Ldf_D_A,		// D6
	Stf_D_A,		// D7
	IgnoreInsHandler,	// InvalidInsHandler,		// D8
	IgnoreInsHandler,	// InvalidInsHandler,		// D9
	IgnoreInsHandler,	// InvalidInsHandler,		// DA
	Addf_D_A,		// DB
	IgnoreInsHandler,	// InvalidInsHandler,		// DC
	IgnoreInsHandler,	// InvalidInsHandler,		// DD
	IgnoreInsHandler,	// InvalidInsHandler,		// DE
	IgnoreInsHandler,	// InvalidInsHandler,		// DF
	Subf_X_A,		// E0
	Cmpf_X_A,		// E1
	IgnoreInsHandler,	// InvalidInsHandler,		// E2
	IgnoreInsHandler,	// InvalidInsHandler,		// E3
	IgnoreInsHandler,	// InvalidInsHandler,		// E4
	IgnoreInsHandler,	// InvalidInsHandler,		// E5
	Ldf_X_A,		// E6
	Stf_X_A,		// E7
	IgnoreInsHandler,	// InvalidInsHandler,		// E8
	IgnoreInsHandler,	// InvalidInsHandler,		// E9
	IgnoreInsHandler,	// InvalidInsHandler,		// EA
	Addf_X_A,		// EB
	IgnoreInsHandler,	// InvalidInsHandler,		// EC
	IgnoreInsHandler,	// InvalidInsHandler,		// ED
	IgnoreInsHandler,	// InvalidInsHandler,		// EE
	IgnoreInsHandler,	// InvalidInsHandler,		// EF
	Subf_E_A,		// F0
	Cmpf_E_A,		// F1
	IgnoreInsHandler,	// InvalidInsHandler,		// F2
	IgnoreInsHandler,	// InvalidInsHandler,		// F3
	IgnoreInsHandler,	// InvalidInsHandler,		// F4
	IgnoreInsHandler,	// InvalidInsHandler,		// F5
	Ldf_E_A,		// F6
	Stf_E_A,		// F7
	IgnoreInsHandler,	// InvalidInsHandler,		// F8
	IgnoreInsHandler,	// InvalidInsHandler,		// F9
	IgnoreInsHandler,	// InvalidInsHandler,		// FA
	Addf_E_A,		// FB
	IgnoreInsHandler,	// InvalidInsHandler,		// FC
	IgnoreInsHandler,	// InvalidInsHandler,		// FD
	IgnoreInsHandler,	// InvalidInsHandler,		// FE
	IgnoreInsHandler,	// InvalidInsHandler,		// FF
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
				cpu_nmi_s();

			if (PendingInterupts & 2)
				cpu_firq_s();

			if (PendingInterupts & 1)
			{
				if (IRQWaiter == 0)	// This is needed to fix a subtle timming problem
					cpu_irq_s();		// It allows the CPU to see $FF03 bit 7 high before
				else				// The IRQ is asserted.
					IRQWaiter -= 1;
			}
		}

		if (SyncWaiting_s == 1)	//Abort the run nothing happens asyncronously from the CPU
			return(0);

		unsigned char memByte = MemRead8_s(PC_REG++);
		JmpVec1_s[memByte](); // Execute instruction pointed to by PC_REG
		CycleCounter += 5;
	}//End While

	return(CycleFor - CycleCounter);
}

void Page_2_s(void) //10
{
	//JmpVec2[MemRead8_s(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void Page_3_s(void) //11
{
	//JmpVec3[MemRead8_s(PC_REG++)](); // Execute instruction pointed to by PC_REG
}

void cpu_firq_s(void)
{
	
	if (!cc_s[F])
	{
		InInterupt_s=1; //Flag to indicate FIRQ has been asserted
		switch (MD_FIRQMODE)
		{
		case 0:
			cc_s[E]=0; // Turn E flag off
			MemWrite8_s( pc_s.B.lsb,--S_REG);
			MemWrite8_s( pc_s.B.msb,--S_REG);
			MemWrite8_s(getcc_s(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16_s(VFIRQ);
		break;

		case 1:		//6309
			cc_s[E]=1;
			MemWrite8_s( pc_s.B.lsb,--S_REG);
			MemWrite8_s( pc_s.B.msb,--S_REG);
			MemWrite8_s( u_s.B.lsb,--S_REG);
			MemWrite8_s( u_s.B.msb,--S_REG);
			MemWrite8_s( y_s.B.lsb,--S_REG);
			MemWrite8_s( y_s.B.msb,--S_REG);
			MemWrite8_s( x_s.B.lsb,--S_REG);
			MemWrite8_s( x_s.B.msb,--S_REG);
			MemWrite8_s( dp_s.B.msb,--S_REG);
			if (MD_NATIVE6309)
			{
				MemWrite8_s((F_REG),--S_REG);
				MemWrite8_s((E_REG),--S_REG);
			}
			MemWrite8_s(B_REG,--S_REG);
			MemWrite8_s(A_REG,--S_REG);
			MemWrite8_s(getcc_s(),--S_REG);
			cc_s[I]=1;
			cc_s[F]=1;
			PC_REG=MemRead16_s(VFIRQ);
		break;
		}
	}
	PendingInterupts=PendingInterupts & 253;
	return;
}

void cpu_irq_s(void)
{
	if (InInterupt_s==1) //If FIRQ is running postpone the IRQ
		return;			
	if ((!cc_s[I]) )
	{
		cc_s[E]=1;
		MemWrite8_s( pc_s.B.lsb,--S_REG);
		MemWrite8_s( pc_s.B.msb,--S_REG);
		MemWrite8_s( u_s.B.lsb,--S_REG);
		MemWrite8_s( u_s.B.msb,--S_REG);
		MemWrite8_s( y_s.B.lsb,--S_REG);
		MemWrite8_s( y_s.B.msb,--S_REG);
		MemWrite8_s( x_s.B.lsb,--S_REG);
		MemWrite8_s( x_s.B.msb,--S_REG);
		MemWrite8_s( dp_s.B.msb,--S_REG);
		if (MD_NATIVE6309)
		{
			MemWrite8_s((F_REG),--S_REG);
			MemWrite8_s((E_REG),--S_REG);
		}
		MemWrite8_s(B_REG,--S_REG);
		MemWrite8_s(A_REG,--S_REG);
		MemWrite8_s(getcc_s(),--S_REG);
		PC_REG=MemRead16_s(VIRQ);
		cc_s[I]=1; 
	} //Fi I test
	PendingInterupts=PendingInterupts & 254;
	return;
}

void cpu_nmi_s(void)
{
	cc_s[E]=1;
	MemWrite8_s( pc_s.B.lsb,--S_REG);
	MemWrite8_s( pc_s.B.msb,--S_REG);
	MemWrite8_s( u_s.B.lsb,--S_REG);
	MemWrite8_s( u_s.B.msb,--S_REG);
	MemWrite8_s( y_s.B.lsb,--S_REG);
	MemWrite8_s( y_s.B.msb,--S_REG);
	MemWrite8_s( x_s.B.lsb,--S_REG);
	MemWrite8_s( x_s.B.msb,--S_REG);
	MemWrite8_s( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8_s((F_REG),--S_REG);
		MemWrite8_s((E_REG),--S_REG);
	}
	MemWrite8_s(B_REG,--S_REG);
	MemWrite8_s(A_REG,--S_REG);
	MemWrite8_s(getcc_s(),--S_REG);
	cc_s[I]=1;
	cc_s[F]=1;
	PC_REG=MemRead16_s(VNMI);
	PendingInterupts=PendingInterupts & 251;
	return;
}

// unsigned short CalculateEA_s(unsigned char postbyte)
// {
// 	static unsigned short int ea = 0;
// 	static signed char byte = 0;
// 	static unsigned char Register;

// 	Register = ((postbyte >> 5) & 3) + 1;
	
// 	if (postbyte & 0x80)
// 	{
// 		switch (postbyte & 0x1F)
// 		{
// 		case 0: // Post-inc by 1
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register])++;
// 			//CycleCounter += 2;
// 			break;

// 		case 1: // Post-inc by 2
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register]) += 2;
// 			//CycleCounter += 3;
// 			break;

// 		case 2: // Pre-dec by 1
// 			(*xfreg16_s[Register]) -= 1;
// 			ea = (*xfreg16_s[Register]);
// 			//CycleCounter += 2;
// 			break;

// 		case 3: // Pre-dec by 2
// 			(*xfreg16_s[Register]) -= 2;
// 			ea = (*xfreg16_s[Register]);
// 			//CycleCounter += 3;
// 			break;

// 		case 4: // No offest
// 			ea = (*xfreg16_s[Register]);
// 			break;

// 		case 5: // B.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswlsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 6: // A.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswmsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 7: // E.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswmsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 8: // 8 bit offset
// 			ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
// 			//CycleCounter += 1;
// 			break;

// 		case 9: // 16 bit ofset
// 			ea = (*xfreg16_s[Register]) + IMMADDRESS(pc_s.Reg);
// 			//CycleCounter += 4;
// 			pc_s.Reg += 2;
// 			break;

// 		case 10: // F.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswlsb);
// 			//CycleCounter += 1;
// 			break;

// 		case 11: // D.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.lsw; //Changed to unsigned 03/14/2005 NG Was signed
// 			//CycleCounter += 4;
// 			break;

// 		case 12: // 8 bit PC Relative
// 			ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
// 			//CycleCounter += 1;
// 			pc_s.Reg++;
// 			break;

// 		case 13: // 16 bit PC Relative
// 			ea = PC_REG + IMMADDRESS(pc_s.Reg) + 2;
// 			//CycleCounter += 5;
// 			pc_s.Reg += 2;
// 			break;

// 		case 14: // W.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.msw;
// 			//CycleCounter += 4;
// 			break;

// 		case 15: // W.reg
// 			byte = (postbyte >> 5) & 3;
// 			switch (byte)
// 			{
// 			case 0: // No offset from W.reg
// 				ea = q_s.Word.msw;
// 				break;
// 			case 1: // 16 bit offset from W.reg
// 				ea = q_s.Word.msw + IMMADDRESS(pc_s.Reg);
// 				pc_s.Reg += 2;
// 				break;
// 			case 2: // Post-inc by 2 from W.reg
// 				ea = q_s.Word.msw;
// 				q_s.Word.msw += 2;
// 				break;
// 			case 3: // Pre-dec by 2 from W.reg
// 				q_s.Word.msw -= 2;
// 				ea = q_s.Word.msw;
// 				break;
// 			}
// 			break;

// 		case 16: // W.reg
// 			byte = (postbyte >> 5) & 3;
// 			switch (byte)
// 			{
// 			case 0: // Indirect no offset from W.reg
// 				ea = MemRead16_s(q_s.Word.msw);
// 				break;
// 			case 1: // Indirect 16 bit offset from W.reg
// 				ea = MemRead16_s(q_s.Word.msw + IMMADDRESS(pc_s.Reg));
// 				pc_s.Reg += 2;
// 				break;
// 			case 2: // Indirect post-inc by 2 from W.reg
// 				ea = MemRead16_s(q_s.Word.msw);
// 				q_s.Word.msw += 2;
// 				break;
// 			case 3: // Indirect pre-dec by 2 from W.reg
// 				q_s.Word.msw -= 2;
// 				ea = MemRead16_s(q_s.Word.msw);
// 				break;
// 			}
// 			break;


// 		case 17: // Indirect Post-inc by 2
// 			ea = (*xfreg16_s[Register]);
// 			(*xfreg16_s[Register]) += 2;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 6;
// 			break;

// 		case 18: //10010
// 			//CycleCounter += 6;
// 			break;

// 		case 19: // Indirect Pre-dec by 2
// 			(*xfreg16_s[Register]) -= 2;
// 			ea = (*xfreg16_s[Register]);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 6;
// 			break;

// 		case 20: // Indirect no offset
// 			ea = (*xfreg16_s[Register]);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 3;
// 			break;

// 		case 21: // Indirect B.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswlsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 22: // Indirect A.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.lswmsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 23: // Indirect E.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswmsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 24: // Indirect 8 bit offset
// 			ea = (*xfreg16_s[Register]) + (signed char)MemRead8_s(pc_s.Reg++);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 25: // Indirect 16 bit offset
// 			ea = (*xfreg16_s[Register]) + IMMADDRESS(pc_s.Reg);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			pc_s.Reg += 2;
// 			break;
// 		case 26: // Indirect F.reg offset
// 			ea = (*xfreg16_s[Register]) + ((signed char)q_s.Byte.mswlsb);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			break;

// 		case 27: // Indirect D.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.lsw;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			break;

// 		case 28: // Indirect 8 bit PC relative
// 			ea = (signed short)pc_s.Reg + (signed char)MemRead8_s(pc_s.Reg) + 1;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 4;
// 			pc_s.Reg++;
// 			break;

// 		case 29: // Indirect 16 bit PC relative
// 			ea = pc_s.Reg + IMMADDRESS(pc_s.Reg) + 2;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 8;
// 			pc_s.Reg += 2;
// 			break;

// 		case 30: // Indirect W.reg offset
// 			ea = (*xfreg16_s[Register]) + q_s.Word.msw;
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 7;
// 			break;

// 		case 31: // Indirect extended
// 			ea = IMMADDRESS(pc_s.Reg);
// 			ea = MemRead16_s(ea);
// 			//CycleCounter += 8;
// 			pc_s.Reg += 2;
// 			break;

// 		} //END Switch
// 	}
// 	else // 5 bit offset
// 	{
// 		byte = (postbyte & 31);
// 		byte = (byte << 3);
// 		byte = byte / 8;
// 		ea = *xfreg16_s[Register] + byte; //Was signed
// 		//CycleCounter += 1;
// 	}
// 	return(ea);
// }

//void setcc_s(UINT8 bincc)
//{
//	cc_s[C] = !!(bincc & CFx68);
//	cc_s[V] = !!(bincc & VFx68);
//	cc_s[Z] = !!(bincc & ZFx68);
//	cc_s[N] = !!(bincc & NFx68);
//	cc_s[I] = !!(bincc & IFx68);
//	cc_s[H] = !!(bincc & HFx68);
//	cc_s[F] = !!(bincc & FFx68);
//	cc_s[E] = !!(bincc & EFx68);
//	return;
//}

//UINT8 getcc_s(void)
//{
//	UINT8 bincc = 0;
//	bincc |= cc_s[C] == 1 ? CFx68 : 0;
//	bincc |= cc_s[V] == 1 ? VFx68 : 0;
//	bincc |= cc_s[Z] == 1 ? ZFx68 : 0;
//	bincc |= cc_s[N] == 1 ? NFx68 : 0;
//	bincc |= cc_s[I] == 1 ? IFx68 : 0;
//	bincc |= cc_s[H] == 1 ? HFx68 : 0;
//	bincc |= cc_s[F] == 1 ? FFx68 : 0;
//	bincc |= cc_s[E] == 1 ? EFx68 : 0;
//	return bincc;
//}

//void setcc_s(UINT8 bincc)
//{
//	x86Flags = 0;
//	x86Flags |= (bincc & CFx68) == CFx68 ? CFx86 : 0;
//	x86Flags |= (bincc & VFx68) == VFx68 ? VFx86 : 0;
//	x86Flags |= (bincc & ZFx68) == ZFx68 ? ZFx86 : 0;
//	x86Flags |= (bincc & NFx68) == NFx68 ? NFx86 : 0;
//	x86Flags |= (bincc & HFx68) == HFx68 ? HFx86 : 0;
//	cc_s[I] = !!(bincc & (IFx68));
//	cc_s[F] = !!(bincc & (FFx68));
//	cc_s[E] = !!(bincc & (EFx68));
//	return;
//}
//
//UINT8 getcc_s(void)
//{
//	UINT8 bincc = 0;
//	bincc |= (x86Flags & CFx86) == CFx86 ? CFx68 : 0;
//	bincc |= (x86Flags & VFx86) == VFx86 ? VFx68 : 0;
//	bincc |= (x86Flags & ZFx86) == ZFx86 ? ZFx68 : 0;
//	bincc |= (x86Flags & NFx86) == NFx86 ? NFx68 : 0;
//	bincc |= (x86Flags & HFx86) == HFx86 ? HFx68 : 0;
//	bincc |= cc_s[I] == 1 ? IFx68 : 0;
//	bincc |= cc_s[F] == 1 ? FFx68 : 0;
//	bincc |= cc_s[E] == 1 ? EFx68 : 0;
//	return bincc;
//}

void setmd_s (UINT8 binmd)
{
	//unsigned char bit;
	//for (bit=0;bit<=7;bit++)
	//	md_s[bit]=!!(binmd & (1<<bit));
	mdbits_s = binmd & 3;
	return;
}

UINT8 getmd_s(void)
{
	//unsigned char binmd=0,bit=0;
	//for (bit=0;bit<=7;bit++)
	//	if (md_s[bit])
	//		binmd=binmd | (1<<bit);
	//	return(binmd);
	return mdbits_s & 0xc0;
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

MSABI void InvalidInsHandler_s(void)
{	
	mdbits_s |= MD_ILLEGALINST_BIT;
	// mdbits_s=getmd_s();
	ErrorVector_s();
	return;
}

void IgnoreInsHandler(void)
{
	return;
}

void MSABI DivbyZero_s(void)
{
	mdbits_s |= MD_DIVBYZERO_BIT;
	// mdbits_s=getmd_s();
	ErrorVector_s();
	return;
}

void MSABI ErrorVector_s(void)
{
	cc_s[E]=1;
	MemWrite8_s( pc_s.B.lsb,--S_REG);
	MemWrite8_s( pc_s.B.msb,--S_REG);
	MemWrite8_s( u_s.B.lsb,--S_REG);
	MemWrite8_s( u_s.B.msb,--S_REG);
	MemWrite8_s( y_s.B.lsb,--S_REG);
	MemWrite8_s( y_s.B.msb,--S_REG);
	MemWrite8_s( x_s.B.lsb,--S_REG);
	MemWrite8_s( x_s.B.msb,--S_REG);
	MemWrite8_s( dp_s.B.msb,--S_REG);
	if (MD_NATIVE6309)
	{
		MemWrite8_s((F_REG),--S_REG);
		MemWrite8_s((E_REG),--S_REG);
		CycleCounter+=2;
	}
	MemWrite8_s(B_REG,--S_REG);
	MemWrite8_s(A_REG,--S_REG);
	MemWrite8_s(getcc_s(),--S_REG);
	PC_REG=MemRead16_s(VTRAP);
	CycleCounter+=(12 + InsCycles[MD_NATIVE6309][M54]);	//One for each byte +overhead? Guessing from PSHS
	return;
}

unsigned int MSABI MemRead32_s(unsigned short Address)
{
	return ( (MemRead16_s(Address)<<16) | MemRead16(Address+2) );

}
void MSABI MemWrite32_s(unsigned int data,unsigned short Address)
{
	MemWrite16_s( data>>16,Address);
	MemWrite16_s( data & 0xFFFF,Address+2);
	return;
}

unsigned char GetSorceReg_s(unsigned char Tmp)
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

unsigned short GetPC_s(void)
{
	return(PC_REG);
}


