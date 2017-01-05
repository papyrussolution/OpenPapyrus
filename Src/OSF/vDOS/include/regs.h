#ifndef VDOS_REGS_H
#define VDOS_REGS_H

#include "mem.h"

#define FLAG_CF		0x00000001
#define FLAG_PF		0x00000004
#define FLAG_AF		0x00000010
#define FLAG_ZF		0x00000040
#define FLAG_SF		0x00000080
#define FLAG_OF		0x00000800

#define FLAG_TF		0x00000100
#define FLAG_IF		0x00000200
#define FLAG_DF		0x00000400

#define FLAG_IOPL	0x00003000
#define FLAG_NT		0x00004000
#define FLAG_VM		0x00020000
#define FLAG_AC		0x00040000

#define FMASK_TEST		(FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF | FLAG_OF)
#define FMASK_NORMAL	(FMASK_TEST | FLAG_DF | FLAG_TF | FLAG_IF)	
#define FMASK_ALL		(FMASK_NORMAL | FLAG_IOPL | FLAG_NT)

#define SETFLAGBIT(TYPE,TEST) if (TEST) reg_flags |= FLAG_ ## TYPE; else reg_flags &= ~FLAG_ ## TYPE
//#define SETFLAGBIT(TYPE,TEST) reg_flags&=~FLAG_ ## TYPE; if (TEST) reg_flags|=FLAG_ ## TYPE
#define GETFLAG(TYPE) (reg_flags&FLAG_ ## TYPE)
#define GETFLAG_IOPL ((reg_flags&FLAG_IOPL)>>12)

enum SegNames {es = 0, cs, ss, ds, fs, gs};

struct Segments {
	Bitu val[8];
	PhysPt phys[8];
};

union GenReg32 {
	Bit32u dword[1];
	Bit16u word[2];
	Bit8u byte[4];
};

#define DW_INDEX 0
#define W_INDEX 0
#define BH_INDEX 1
#define BL_INDEX 0

struct CPU_Regs {
	GenReg32 regs[8], ip;
	Bitu flags;
};

extern Segments Segs;
extern CPU_Regs cpu_regs;

__forceinline PhysPt SegPhys(SegNames index)
	{
	return Segs.phys[index];
	}

__forceinline Bit16u SegValue(SegNames index)
	{
	return (Bit16u)Segs.val[index];
	}
	
__forceinline RealPt RealMakeSeg(SegNames index, Bit16u off)
	{
	return SegOff2dWord(SegValue(index), off);	
	}

__forceinline void SegSet16(Bitu index, Bit16u val)
	{
	Segs.val[index] = val;
	Segs.phys[index] = val<<4;
	}

enum {
	REGI_AX, REGI_CX, REGI_DX, REGI_BX,
	REGI_SP, REGI_BP, REGI_SI, REGI_DI
};


#define reg_al cpu_regs.regs[REGI_AX].byte[BL_INDEX]
#define reg_ah cpu_regs.regs[REGI_AX].byte[BH_INDEX]
#define reg_ax cpu_regs.regs[REGI_AX].word[W_INDEX]
#define reg_eax cpu_regs.regs[REGI_AX].dword[DW_INDEX]

#define reg_bl cpu_regs.regs[REGI_BX].byte[BL_INDEX]
#define reg_bh cpu_regs.regs[REGI_BX].byte[BH_INDEX]
#define reg_bx cpu_regs.regs[REGI_BX].word[W_INDEX]
#define reg_ebx cpu_regs.regs[REGI_BX].dword[DW_INDEX]

#define reg_cl cpu_regs.regs[REGI_CX].byte[BL_INDEX]
#define reg_ch cpu_regs.regs[REGI_CX].byte[BH_INDEX]
#define reg_cx cpu_regs.regs[REGI_CX].word[W_INDEX]
#define reg_ecx cpu_regs.regs[REGI_CX].dword[DW_INDEX]

#define reg_dl cpu_regs.regs[REGI_DX].byte[BL_INDEX]
#define reg_dh cpu_regs.regs[REGI_DX].byte[BH_INDEX]
#define reg_dx cpu_regs.regs[REGI_DX].word[W_INDEX]
#define reg_edx cpu_regs.regs[REGI_DX].dword[DW_INDEX]

#define reg_si cpu_regs.regs[REGI_SI].word[W_INDEX]
#define reg_esi cpu_regs.regs[REGI_SI].dword[DW_INDEX]

#define reg_di cpu_regs.regs[REGI_DI].word[W_INDEX]
#define reg_edi cpu_regs.regs[REGI_DI].dword[DW_INDEX]

#define reg_sp cpu_regs.regs[REGI_SP].word[W_INDEX]
#define reg_esp cpu_regs.regs[REGI_SP].dword[DW_INDEX]

#define reg_bp cpu_regs.regs[REGI_BP].word[W_INDEX]
#define reg_ebp cpu_regs.regs[REGI_BP].dword[DW_INDEX]

#define reg_ip cpu_regs.ip.word[W_INDEX]
#define reg_eip cpu_regs.ip.dword[DW_INDEX]

#define reg_flags cpu_regs.flags

#endif
