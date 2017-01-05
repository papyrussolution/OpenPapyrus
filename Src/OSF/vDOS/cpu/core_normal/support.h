#define LoadMbs(off) (Bit8s)(Mem_Lodsb(off))
#define LoadMws(off) (Bit16s)(Mem_Lodsw(off))
#define LoadMds(off) (Bit32s)(Mem_Lodsd(off))

#define LoadRb(reg) reg
#define LoadRw(reg) reg
#define LoadRd(reg) reg

#define SaveRb(reg, val)	reg = val
#define SaveRw(reg, val)	reg = val
#define SaveRd(reg, val)	reg = val

#define Fetchbs()	((Bit8s)(Fetchb()))
#define Fetchws()	((Bit16s)(Fetchw()))
#define Fetchds()	((Bit32s)(Fetchd()))

#define RUNEXCEPTION()										\
	{														\
	CPU_Exception(cpu.exception.which,cpu.exception.error);	\
	continue;												\
	}

#define EXCEPTION(blah)								\
	{												\
	CPU_Exception(blah);							\
	continue;										\
	}

#define JumpCond16_b(COND)							\
	{												\
	SAVEIP;											\
	if (COND)										\
		reg_ip += Fetchbs();						\
	reg_ip += 1;									\
	continue;										\
	}

#define JumpCond16_w(COND)							\
	{												\
	SAVEIP;											\
	if (COND)										\
		reg_ip += Fetchws();						\
	reg_ip += 2;									\
	continue;										\
}

#define JumpCond32_b(COND)							\
	{												\
	SAVEIP;											\
	if (COND)										\
		reg_eip += Fetchbs();						\
	reg_eip += 1;									\
	continue;										\
	}

#define JumpCond32_d(COND)							\
	{												\
	SAVEIP;											\
	if (COND)										\
		reg_eip += Fetchds();						\
	reg_eip += 4;									\
	continue;										\
	}

#define SETcc(cc)									\
	{												\
	GetRM;											\
	if (rm >= 0xc0) {GetEArb;*earb=(cc) ? 1 : 0;}	\
	else {GetEAa;Mem_Stosb(eaa,(cc) ? 1 : 0);}		\
	}
/*
#define SETcc(cc)									\
	{												\
	GetRM;											\
	if (rm >= 0xc0)									\
		{											\
		GetEArb;									\
		*earb = cc;									\
		}											\
	else											\
		{											\
		GetEAa;										\
		Mem_Stosb(eaa, cc);							\
		}											\
	}
*/
#include "helpers.h"
#include "table_ea.h"
#include "../modrm.h"


