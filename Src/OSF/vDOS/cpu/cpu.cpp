#include "vDos.h"
#include "cpu.h"
#include "paging.h"
#include "lazyflags.h"
#include "support.h"

#if defined (_MSC_VER)
#define LOG(X,Y)
#else
#define LOG(X,Y) CPU_LOG
#define CPU_LOG(...)
#endif

CPU_Regs cpu_regs;
CPUBlock cpu;
Segments Segs;

CPU_Decoder * cpudecoder;

#define CPU_CHECK_IGNORE 1

#if defined(CPU_CHECK_IGNORE)
#define CPU_CHECK_COND(cond, msg, exc, sel) {	\
	if (cond) do {} while (0);					\
}
#elif defined(CPU_CHECK_EXCEPT)
#define CPU_CHECK_COND(cond,msg,exc,sel) {	\
	if (cond) {					\
		CPU_Exception(exc,sel);		\
		return;				\
	}					\
}
#else
#define CPU_CHECK_COND(cond,msg,exc,sel) {	\
	if (cond) E_Exit(msg);			\
}
#endif

void Descriptor::Load(PhysPt address)
	{
	Bit32u* data = (Bit32u*)&saved;
	*data = Mem_Lodsd(address);
	*(data+1) = Mem_Lodsd(address+4);
	}

void Descriptor:: Save(PhysPt address)
	{
	Bit32u* data = (Bit32u*)&saved;
	Mem_Stosd(address, *data);
	Mem_Stosd(address+4, *(data+1));
	}

void CPU_Push16(Bitu value)
	{
	Bit32u new_esp = (reg_esp&cpu.stack.notmask)|((reg_esp-2)&cpu.stack.mask);
	Mem_Stosw(SegPhys(ss)+(new_esp&cpu.stack.mask), value);
	reg_esp = new_esp;
	}

void CPU_Push32(Bitu value)
	{
	Bit32u new_esp = (reg_esp&cpu.stack.notmask)|((reg_esp-4)&cpu.stack.mask);
	Mem_Stosd(SegPhys(ss)+(new_esp & cpu.stack.mask), value);
	reg_esp = new_esp;
	}

Bitu CPU_Pop16(void)
	{
	Bitu val = Mem_Lodsw(SegPhys(ss)+(reg_esp&cpu.stack.mask));
	reg_esp = (reg_esp&cpu.stack.notmask)|((reg_esp+2)&cpu.stack.mask);
	return val;
	}

Bitu CPU_Pop32(void)
	{
	Bitu val = Mem_Lodsd(SegPhys(ss)+(reg_esp&cpu.stack.mask));
	reg_esp = (reg_esp&cpu.stack.notmask)|((reg_esp+4)&cpu.stack.mask);
	return val;
	}

void CPU_SetFlags(Bitu word, Bitu mask)
	{
	reg_flags = (reg_flags & ~mask)|(word & mask)|2;
	cpu.direction = 1-((reg_flags & FLAG_DF) >> 9);
	}

bool CPU_PrepareException(Bitu which, Bitu error)
	{
	cpu.exception.which = which;
	cpu.exception.error = error;
	return true;
	}

bool CPU_pCLI(void)
	{
	if (GETFLAG_IOPL < cpu.cpl)
		return CPU_PrepareException(EXCEPTION_GP, 0);
	SETFLAGBIT(IF, false);
	return false;
	}

bool CPU_pSTI(void)
	{
	if (GETFLAG_IOPL < cpu.cpl)
		return CPU_PrepareException(EXCEPTION_GP, 0);
	SETFLAGBIT(IF, true);
	return false;
	}

void CPU_POPF(Bitu use32)
	{
	Bitu mask = FMASK_ALL;
	// IOPL field can only be modified when CPL=0 or in real mode:
	if (cpu.pmode)
		{
		if (cpu.cpl > 0)
			mask &= (~FLAG_IOPL);
		if (GETFLAG_IOPL < cpu.cpl)
			mask &= (~FLAG_IF);
		}
	if (use32)
		CPU_SetFlags(CPU_Pop32(), mask);
	else
		CPU_SetFlags(CPU_Pop16(), mask&0xffff);
	DestroyConditionFlags();
	}

void CPU_CheckSegments(void)
	{
	bool needs_invalidation = false;
	Descriptor desc;
	if (!cpu.gdt.GetDescriptor(SegValue(es), desc))
		needs_invalidation = true;
	else
		switch (desc.Type())
			{
		case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			if (cpu.cpl > desc.DPL())
				needs_invalidation = true;
			break;
		default:
			break;
			}
	if (needs_invalidation)
		CPU_SetSegGeneral(es, 0);

	needs_invalidation = false;
	if (!cpu.gdt.GetDescriptor(SegValue(ds), desc))
		needs_invalidation = true;
	else
		switch (desc.Type())
			{
		case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			if (cpu.cpl > desc.DPL())
				needs_invalidation = true;
			break;
		default:
			break;
			}
	if (needs_invalidation)
		CPU_SetSegGeneral(ds, 0);

	needs_invalidation=false;
	if (!cpu.gdt.GetDescriptor(SegValue(fs), desc))
		needs_invalidation=true;
	else
		switch (desc.Type())
			{
		case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			if (cpu.cpl > desc.DPL())
				needs_invalidation = true;
			break;
		default:
			break;
			}
	if (needs_invalidation)
		CPU_SetSegGeneral(fs, 0);

	needs_invalidation=false;
	if (!cpu.gdt.GetDescriptor(SegValue(gs), desc))
		needs_invalidation = true;
	else
		switch (desc.Type())
			{
		case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			if (cpu.cpl > desc.DPL())
				needs_invalidation = true;
			break;
		default:
			break;
			}
	if (needs_invalidation)
		CPU_SetSegGeneral(gs, 0);
	}


class TaskStateSegment
	{
public:
	TaskStateSegment()
		{
		valid = false;
		}
	bool IsValid(void)
		{
		return valid;
		}
	void SaveSelector(void)
		{
		cpu.gdt.SetDescriptor(selector, desc);
		}
	void Get_SSx_ESPx(Bitu level, Bitu & _ss, Bitu & _esp)
		{
		if (is386)
			{
			PhysPt where = base+offsetof(TSS_32, esp0)+level*8;
			_esp = Mem_Lodsd(where);
			_ss = Mem_Lodsw(where+4);
			}
		else
			{
			PhysPt where = base+offsetof(TSS_16, sp0)+level*4;
			_esp = Mem_Lodsw(where);
			_ss = Mem_Lodsw(where+2);
			}
		}
	bool SetSelector(Bitu new_sel)
		{
		valid = false;
		if ((new_sel & 0xfffc) == 0)
			{
			selector = 0;
			base = 0;
			limit = 0;
			is386 = 1;
			return true;
			}
		if (new_sel&4)
			return false;
		if (!cpu.gdt.GetDescriptor(new_sel, desc))
			return false;
		switch (desc.Type())
			{
		case DESC_286_TSS_A:		case DESC_286_TSS_B:
		case DESC_386_TSS_A:		case DESC_386_TSS_B:
			break;
		default:
			return false;
			}
		if (!desc.saved.seg.p)
			return false;
		selector = new_sel;
		valid = true;
		base = desc.GetBase();
		limit = desc.GetLimit();
		is386 = desc.Is386();
		return true;
		}
	TSS_Descriptor desc;
	Bitu selector;
	PhysPt base;
	Bitu limit;
	Bitu is386;
	bool valid;
	};

TaskStateSegment cpu_tss;

bool CPU_IO_Exception(Bitu port, Bitu size)
	{
	if (GETFLAG_IOPL < cpu.cpl)
		{
		if (!cpu_tss.is386)
			goto doexception;
		PhysPt bwhere = cpu_tss.base+0x66;
		Bitu ofs = Mem_Lodsw(bwhere);
		if (ofs > cpu_tss.limit)
			goto doexception;
		bwhere = cpu_tss.base+ofs+(port/8);
		Bitu map = Mem_Lodsw(bwhere);
		Bitu mask = (0xffff>>(16-size)) << (port&7);
		if (map & mask)
			goto doexception;
		}
	return false;
doexception:
	return CPU_PrepareException(EXCEPTION_GP, 0);
	}

void CPU_Exception(Bitu which, Bitu error)
	{
	cpu.exception.error = error;
	CPU_Interrupt(which, CPU_INT_EXCEPTION | ((which >= 8) ? CPU_INT_HAS_ERROR : 0), reg_eip);
	}

void CPU_Interrupt(Bitu num, Bitu type, Bitu oldeip)
	{
	FillFlags();
	if (!cpu.pmode)
		{
		// Save everything on a 16-bit stack
		CPU_Push16(reg_flags&0xffff);
		CPU_Push16(SegValue(cs));
		CPU_Push16(oldeip);
		SETFLAGBIT(IF, false);
		SETFLAGBIT(TF, false);
		// Get the new CS:IP from vector table
		PhysPt base = cpu.idt.GetBase();
		reg_eip = Mem_Lodsw(base+(num << 2));
		Segs.val[cs] = Mem_Lodsw(base+(num << 2)+2);
		Segs.phys[cs] = Segs.val[cs]<<4;
		cpu.code.big = false;
		return;
		}
	else
		{
		Descriptor gate;
		if (!cpu.idt.GetDescriptor(num<<3, gate))
			{
			// zone66
			CPU_Exception(EXCEPTION_GP, num*8+2+(type&CPU_INT_SOFTWARE) ? 0 : 1);
			return;
			}
		if ((type&CPU_INT_SOFTWARE) && (gate.DPL() < cpu.cpl))
			{
			// zone66, win3.x e
			CPU_Exception(EXCEPTION_GP, num*8+2);
			return;
			}
		switch (gate.Type())
			{
		case DESC_286_INT_GATE:		case DESC_386_INT_GATE:
		case DESC_286_TRAP_GATE:	case DESC_386_TRAP_GATE:
			{
			CPU_CHECK_COND(!gate.saved.seg.p, "INT:Gate segment not present", EXCEPTION_NP, num*8+2+(type&CPU_INT_SOFTWARE) ? 0 : 1)
			Descriptor cs_desc;
			Bitu gate_sel = gate.GetSelector();
			Bitu gate_off = gate.GetOffset();
			CPU_CHECK_COND((gate_sel & 0xfffc) == 0, "INT:Gate with CS zero selector", EXCEPTION_GP, (type&CPU_INT_SOFTWARE) ? 0 : 1)
			CPU_CHECK_COND(!cpu.gdt.GetDescriptor(gate_sel,cs_desc), "INT:Gate with CS beyond limit", EXCEPTION_GP, (gate_sel & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)
			Bitu cs_dpl = cs_desc.DPL();
			CPU_CHECK_COND(cs_dpl > cpu.cpl, "Interrupt to higher privilege", EXCEPTION_GP, (gate_sel & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)
			switch (cs_desc.Type())
				{
			case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
			case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
				if (cs_dpl < cpu.cpl)
					{
					// Prepare for gate to inner level
					CPU_CHECK_COND(!cs_desc.saved.seg.p, "INT:Inner level:CS segment not present", EXCEPTION_NP, (gate_sel & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)
					Bitu n_ss, n_esp;
					Bitu o_ss, o_esp;
					o_ss = SegValue(ss);
					o_esp = reg_esp;
					cpu_tss.Get_SSx_ESPx(cs_dpl, n_ss, n_esp);
					CPU_CHECK_COND((n_ss & 0xfffc)==0, 	"INT:Gate with SS zero selector", EXCEPTION_TS,(type&CPU_INT_SOFTWARE) ? 0 : 1)
					Descriptor n_ss_desc;
					CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_ss,n_ss_desc), "INT:Gate with SS beyond limit", EXCEPTION_TS,(n_ss & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)
					CPU_CHECK_COND(((n_ss & 3)!=cs_dpl) || (n_ss_desc.DPL()!=cs_dpl), "INT:Inner level with CS_DPL!=SS_DPL and SS_RPL", EXCEPTION_TS,(n_ss & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)

					// check if stack segment is a writable data segment
					switch (n_ss_desc.Type())
						{
					case DESC_DATA_EU_RW_NA:		case DESC_DATA_EU_RW_A:
					case DESC_DATA_ED_RW_NA:		case DESC_DATA_ED_RW_A:
						break;
					default:
						E_Exit("INT-Inner level: Stack segment not writable.");		// or #TS(ss_sel+EXT)
						}
						CPU_CHECK_COND(!n_ss_desc.saved.seg.p, "INT-Inner level with nonpresent SS", EXCEPTION_SS,(n_ss & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)

					// commit point
					Segs.phys[ss] = n_ss_desc.GetBase();
					Segs.val[ss] = n_ss;
					if (n_ss_desc.Big())
						{
						cpu.stack.big = true;
						cpu.stack.mask = 0xffffffff;
						cpu.stack.notmask = 0;
						reg_esp = n_esp;
						}
					else
						{
						cpu.stack.big = false;
						cpu.stack.mask = 0xffff;
						cpu.stack.notmask = 0xffff0000;
						reg_sp = n_esp & 0xffff;
						}

					cpu.cpl = cs_dpl;
					if (gate.Type() & 0x8)
						{					// 32-bit Gate
						CPU_Push32(o_ss);
						CPU_Push32(o_esp);
						}
					else
						{					// 16-bit Gate
						CPU_Push16(o_ss);
						CPU_Push16(o_esp);
						}
					goto do_interrupt;
					} 
				if (cs_dpl != cpu.cpl)
					E_Exit("Non-conforming intra privilege INT with DPL!=CPL");
			case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
			case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
				// Prepare stack for gate to same priviledge
				CPU_CHECK_COND(!cs_desc.saved.seg.p, "INT:Same level:CS segment not present", EXCEPTION_NP,(gate_sel & 0xfffc)+(type&CPU_INT_SOFTWARE) ? 0 : 1)

				// commit point
do_interrupt:
				if (gate.Type() & 0x8)
					{					// 32-bit Gate
					CPU_Push32(reg_flags);
					CPU_Push32(SegValue(cs));
					CPU_Push32(oldeip);
					if (type & CPU_INT_HAS_ERROR)
						CPU_Push32(cpu.exception.error);
					}
				else
					{					// 16-bit gate
					CPU_Push16(reg_flags & 0xffff);
					CPU_Push16(SegValue(cs));
					CPU_Push16(oldeip);
					if (type & CPU_INT_HAS_ERROR)
						CPU_Push16(cpu.exception.error);
					}
				break;		
			default:
				E_Exit("INT: Gate Selector points to illegal descriptor with type %x", cs_desc.Type());
				}

			Segs.val[cs] = (gate_sel&0xfffc) | cpu.cpl;
			Segs.phys[cs] = cs_desc.GetBase();
			cpu.code.big = cs_desc.Big()>0;
			reg_eip = gate_off;

			if (!(gate.Type()&1))
				SETFLAGBIT(IF, false);
			SETFLAGBIT(TF, false);
			SETFLAGBIT(NT, false);
			SETFLAGBIT(VM, false);
			return;
			}
		default:
			E_Exit("INT: Illegal descriptor type %X for int %X", gate.Type(), num);
			}
		}
	}


void CPU_IRET(bool use32)
	{
	if (!cpu.pmode)
		{					// RealMode IRET
		if (use32)
			{
			reg_eip = CPU_Pop32();
			SegSet16(cs, CPU_Pop32());
			CPU_SetFlags(CPU_Pop32(), FMASK_ALL);
			}
		else
			{
			reg_eip = CPU_Pop16();
			SegSet16(cs, CPU_Pop16());
			CPU_SetFlags(CPU_Pop16(), FMASK_ALL & 0xffff);
			}
		cpu.code.big = false;
		DestroyConditionFlags();
		return;
		}
	// Check if this is task IRET
	if (GETFLAG(NT))
		E_Exit("Pmode IRET not supported");
	Bitu n_cs_sel, n_eip, n_flags;
	Bit32u tempesp;
	if (use32)
		{
		n_eip = Mem_Lodsd(SegPhys(ss) + (reg_esp & cpu.stack.mask));
		tempesp = (reg_esp&cpu.stack.notmask)|((reg_esp+4)&cpu.stack.mask);
		n_cs_sel = Mem_Lodsd(SegPhys(ss) + (tempesp & cpu.stack.mask)) & 0xffff;
		tempesp = (tempesp&cpu.stack.notmask)|((tempesp+4)&cpu.stack.mask);
		n_flags = Mem_Lodsd(SegPhys(ss) + (tempesp & cpu.stack.mask));
		tempesp = (tempesp&cpu.stack.notmask)|((tempesp+4)&cpu.stack.mask);
		}
	else
		{
		n_eip = Mem_Lodsw(SegPhys(ss) + (reg_esp & cpu.stack.mask));
		tempesp = (reg_esp&cpu.stack.notmask)|((reg_esp+2)&cpu.stack.mask);
		n_cs_sel = Mem_Lodsw(SegPhys(ss) + (tempesp & cpu.stack.mask));
		tempesp = (tempesp&cpu.stack.notmask)|((tempesp+2)&cpu.stack.mask);
		n_flags = Mem_Lodsw(SegPhys(ss) + (tempesp & cpu.stack.mask));
		n_flags|=(reg_flags & 0xffff0000);
		tempesp=(tempesp&cpu.stack.notmask)|((tempesp+2)&cpu.stack.mask);
		}
	CPU_CHECK_COND((n_cs_sel & 0xfffc)==0, "IRET:CS selector zero", EXCEPTION_GP,0)
	Bitu n_cs_rpl=n_cs_sel & 3;
	Descriptor n_cs_desc;
	CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_cs_sel,n_cs_desc), "IRET:CS selector beyond limits", EXCEPTION_GP,n_cs_sel & 0xfffc)
	CPU_CHECK_COND(n_cs_rpl<cpu.cpl, "IRET to lower privilege", EXCEPTION_GP,n_cs_sel & 0xfffc)

	switch (n_cs_desc.Type())
		{
	case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
		CPU_CHECK_COND(n_cs_rpl!=n_cs_desc.DPL(), "IRET:NC:DPL!=RPL", EXCEPTION_GP,n_cs_sel & 0xfffc)
		break;
	case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
		CPU_CHECK_COND(n_cs_desc.DPL()>n_cs_rpl, "IRET:C:DPL>RPL", EXCEPTION_GP,n_cs_sel & 0xfffc)
		break;
	default:
		E_Exit("IRET: Illegal descriptor type %X",n_cs_desc.Type());
		}
	CPU_CHECK_COND(!n_cs_desc.saved.seg.p, "IRET with nonpresent code segment", EXCEPTION_NP,n_cs_sel & 0xfffc)

	if (n_cs_rpl==cpu.cpl)
		{
		// Return to same level
		// commit point
		reg_esp=tempesp;
		Segs.phys[cs]=n_cs_desc.GetBase();
		cpu.code.big=n_cs_desc.Big()>0;
		Segs.val[cs]=n_cs_sel;
		reg_eip=n_eip;

		Bitu mask=cpu.cpl ? (FMASK_NORMAL | FLAG_NT) : FMASK_ALL;
		if (GETFLAG_IOPL<cpu.cpl)
			mask &= (~FLAG_IF);
		CPU_SetFlags(n_flags,mask);
		DestroyConditionFlags();
		}
	else
		{
		// Return to outer level
		Bitu n_ss,n_esp;
		if (use32)
			{
			n_esp = Mem_Lodsd(SegPhys(ss) + (tempesp & cpu.stack.mask));
			tempesp = (tempesp&cpu.stack.notmask)|((tempesp+4)&cpu.stack.mask);
			n_ss = Mem_Lodsd(SegPhys(ss) + (tempesp & cpu.stack.mask)) & 0xffff;
			}
		else
			{
			n_esp = Mem_Lodsw(SegPhys(ss) + (tempesp & cpu.stack.mask));
			tempesp = (tempesp&cpu.stack.notmask)|((tempesp+2)&cpu.stack.mask);
			n_ss = Mem_Lodsw(SegPhys(ss) + (tempesp & cpu.stack.mask));
			}
		CPU_CHECK_COND((n_ss & 0xfffc)==0, "IRET:Outer level:SS selector zero", EXCEPTION_GP,0)
		CPU_CHECK_COND((n_ss & 3)!=n_cs_rpl, "IRET:Outer level:SS rpl!=CS rpl", EXCEPTION_GP,n_ss & 0xfffc)
		Descriptor n_ss_desc;
		CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_ss,n_ss_desc), "IRET:Outer level:SS beyond limit", EXCEPTION_GP,n_ss & 0xfffc)
		CPU_CHECK_COND(n_ss_desc.DPL()!=n_cs_rpl, "IRET:Outer level:SS dpl!=CS rpl", EXCEPTION_GP,n_ss & 0xfffc)

		// check if stack segment is a writable data segment
		switch (n_ss_desc.Type())
			{
		case DESC_DATA_EU_RW_NA:		case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RW_NA:		case DESC_DATA_ED_RW_A:
			break;
		default:
			E_Exit("IRET-Outer level: Stack segment not writable");		// or #GP(ss_sel)
			}
		CPU_CHECK_COND(!n_ss_desc.saved.seg.p, "IRET-Outer level: Stack segment not present", EXCEPTION_NP,n_ss & 0xfffc)

		// commit point
		Segs.phys[cs]=n_cs_desc.GetBase();
		cpu.code.big=n_cs_desc.Big()>0;
		Segs.val[cs]=n_cs_sel;

		Bitu mask=cpu.cpl ? (FMASK_NORMAL | FLAG_NT) : FMASK_ALL;
		if (GETFLAG_IOPL<cpu.cpl)
			mask &= (~FLAG_IF);
		CPU_SetFlags(n_flags,mask);
		DestroyConditionFlags();

		cpu.cpl = n_cs_rpl;
		reg_eip=n_eip;

		Segs.val[ss]=n_ss;
		Segs.phys[ss]=n_ss_desc.GetBase();
		if (n_ss_desc.Big())
			{
			cpu.stack.big = true;
			cpu.stack.mask = 0xffffffff;
			cpu.stack.notmask = 0;
			reg_esp = n_esp;
			}
		else
			{
			cpu.stack.big = false;
			cpu.stack.mask = 0xffff;
			cpu.stack.notmask = 0xffff0000;
			reg_sp = n_esp & 0xffff;
			}

		// borland extender, zrdx
		CPU_CheckSegments();
		}
	return;
	}


void CPU_JMP(bool use32, Bitu selector, Bitu offset)
	{
	if (!cpu.pmode)
		{
		if (!use32)
			reg_eip = offset&0xffff;
		else
			reg_eip = offset;
		SegSet16(cs, selector);
		cpu.code.big = false;
		return;
		}
	CPU_CHECK_COND((selector & 0xfffc) == 0, "JMP:CS selector zero", EXCEPTION_GP, 0)
	Bitu rpl = selector & 3;
	Descriptor desc;
	CPU_CHECK_COND(!cpu.gdt.GetDescriptor(selector,desc), "JMP:CS beyond limits", EXCEPTION_GP, selector & 0xfffc)
	switch (desc.Type())
		{
	case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
		CPU_CHECK_COND(rpl>cpu.cpl, "JMP:NC:RPL>CPL", EXCEPTION_GP,selector & 0xfffc)
		CPU_CHECK_COND(cpu.cpl!=desc.DPL(), "JMP:NC:RPL != DPL", EXCEPTION_GP, selector & 0xfffc)
		goto CODE_jmp;
	case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
		CPU_CHECK_COND(cpu.cpl<desc.DPL(), "JMP:C:CPL < DPL", EXCEPTION_GP, selector & 0xfffc)
CODE_jmp:
		if (!desc.saved.seg.p)
			{
			// win
			CPU_Exception(EXCEPTION_NP, selector & 0xfffc);
			return;
			}
		// Normal jump to another selector:offset
		Segs.phys[cs] = desc.GetBase();
		cpu.code.big = desc.Big() > 0;
		Segs.val[cs] = (selector & 0xfffc) | cpu.cpl;
		reg_eip = offset;
		return;
	default:
		E_Exit("JMP: Illegal descriptor type %X", desc.Type());
		}
	}

void CPU_CALL(bool use32, Bitu selector, Bitu offset, Bitu oldeip)
	{
	if (!cpu.pmode)
		{
		if (!use32)
			{
			CPU_Push16(SegValue(cs));
			CPU_Push16(oldeip);
			reg_eip = offset&0xffff;
			}
		else
			{
			CPU_Push32(SegValue(cs));
			CPU_Push32(oldeip);
			reg_eip = offset;
			}
		cpu.code.big = false;
		SegSet16(cs, selector);
		return;
		}
	CPU_CHECK_COND((selector & 0xfffc) == 0, "CALL:CS selector zero", EXCEPTION_GP, 0)
	Bitu rpl = selector & 3;
	Descriptor call;
	CPU_CHECK_COND(!cpu.gdt.GetDescriptor(selector, call), "CALL:CS beyond limits", EXCEPTION_GP, selector & 0xfffc)
	// Check for type of far call
	switch (call.Type())
		{
	case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
	case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
		CPU_CHECK_COND(rpl > cpu.cpl, "CALL:CODE:NC:RPL>CPL", EXCEPTION_GP, selector & 0xfffc)
		CPU_CHECK_COND(call.DPL() != cpu.cpl, "CALL:CODE:NC:DPL!=CPL", EXCEPTION_GP, selector & 0xfffc)
		goto call_code;	
	case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
		CPU_CHECK_COND(call.DPL() > cpu.cpl, "CALL:CODE:C:DPL>CPL", EXCEPTION_GP, selector & 0xfffc)
call_code:
		if (!call.saved.seg.p)
			{
			// borland extender (RTM)
			CPU_Exception(EXCEPTION_NP, selector & 0xfffc);
			return;
			}
		// commit point
		if (!use32)
			{
			CPU_Push16(SegValue(cs));
			CPU_Push16(oldeip);
			reg_eip = offset & 0xffff;
			}
		else
			{
			CPU_Push32(SegValue(cs));
			CPU_Push32(oldeip);
			reg_eip = offset;
			}
		Segs.phys[cs] = call.GetBase();
		cpu.code.big = call.Big() > 0;
		Segs.val[cs] = (selector & 0xfffc) | cpu.cpl;
		return;
	case DESC_386_CALL_GATE: 
	case DESC_286_CALL_GATE:
		{
		CPU_CHECK_COND(call.DPL() < cpu.cpl, "CALL:Gate:Gate DPL<CPL", EXCEPTION_GP, selector & 0xfffc)
		CPU_CHECK_COND(call.DPL() < rpl, "CALL:Gate:Gate DPL<RPL", EXCEPTION_GP, selector & 0xfffc)
		CPU_CHECK_COND(!call.saved.seg.p, "CALL:Gate:Segment not present", EXCEPTION_NP, selector & 0xfffc)
		Descriptor n_cs_desc;
		Bitu n_cs_sel = call.GetSelector();

		CPU_CHECK_COND((n_cs_sel & 0xfffc) == 0, "CALL:Gate:CS selector zero", EXCEPTION_GP, 0)
		CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_cs_sel, n_cs_desc), "CALL:Gate:CS beyond limits", EXCEPTION_GP, n_cs_sel & 0xfffc)
		Bitu n_cs_dpl = n_cs_desc.DPL();
		CPU_CHECK_COND(n_cs_dpl > cpu.cpl, "CALL:Gate:CS DPL>CPL", EXCEPTION_GP, n_cs_sel & 0xfffc)
		CPU_CHECK_COND(!n_cs_desc.saved.seg.p, "CALL:Gate:CS not present", EXCEPTION_NP, n_cs_sel & 0xfffc)

		Bitu n_eip = call.GetOffset();
		switch (n_cs_desc.Type())
			{
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
		case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			// Check if we goto inner priviledge
			if (n_cs_dpl < cpu.cpl)
				{
				// Get new SS:ESP out of TSS
				Bitu n_ss_sel, n_esp;
				Descriptor n_ss_desc;
				cpu_tss.Get_SSx_ESPx(n_cs_dpl, n_ss_sel, n_esp);
				CPU_CHECK_COND((n_ss_sel & 0xfffc) == 0, "CALL:Gate:NC:SS selector zero", EXCEPTION_TS, 0)
				CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_ss_sel, n_ss_desc), "CALL:Gate:Invalid SS selector", EXCEPTION_TS, n_ss_sel & 0xfffc)
				CPU_CHECK_COND(((n_ss_sel & 3) != n_cs_desc.DPL()) || (n_ss_desc.DPL() != n_cs_desc.DPL()), "CALL:Gate:Invalid SS selector privileges", EXCEPTION_TS, n_ss_sel & 0xfffc)

				switch (n_ss_desc.Type())
					{
					case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
					case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
					// writable data segment
					break;
				default:
					E_Exit("Call-Gate: SS no writable data segment");	// or #TS(ss_sel)
					}
				CPU_CHECK_COND(!n_ss_desc.saved.seg.p, "CALL:Gate:Stack segment not present", EXCEPTION_SS, n_ss_sel & 0xfffc)

				// Load the new SS:ESP and save data on it
				Bitu o_esp = reg_esp;
				Bitu o_ss = SegValue(ss);
				PhysPt o_stack = SegPhys(ss)+(reg_esp & cpu.stack.mask);

				// catch pagefaults
				if (call.saved.gate.paramcount&31)
					{
					if (call.Type() == DESC_386_CALL_GATE)
						for (Bits i = (call.saved.gate.paramcount&31)-1; i >= 0; i--) 
							Mem_Lodsd(o_stack+i*4);
					else
						for (Bits i = (call.saved.gate.paramcount&31)-1; i >= 0; i--)
							Mem_Lodsw(o_stack+i*2);
					}

				// commit point
				Segs.val[ss] = n_ss_sel;
				Segs.phys[ss] = n_ss_desc.GetBase();
				if (n_ss_desc.Big())
					{
					cpu.stack.big = true;
					cpu.stack.mask = 0xffffffff;
					cpu.stack.notmask = 0;
					reg_esp = n_esp;
					}
				else
					{
					cpu.stack.big = false;
					cpu.stack.mask = 0xffff;
					cpu.stack.notmask = 0xffff0000;
					reg_sp = n_esp & 0xffff;
					}

				cpu.cpl = n_cs_desc.DPL();
				Bit16u oldcs = SegValue(cs);
				// Switch to new CS:EIP
				Segs.phys[cs] = n_cs_desc.GetBase();
				Segs.val[cs] = (n_cs_sel & 0xfffc) | cpu.cpl;
				cpu.code.big = n_cs_desc.Big() > 0;
				reg_eip = n_eip;
				if (!use32)
					reg_eip &= 0xffff;
				if (call.Type() == DESC_386_CALL_GATE)
					{
					CPU_Push32(o_ss);		//save old stack
					CPU_Push32(o_esp);
					if (call.saved.gate.paramcount&31)
						for (Bits i = (call.saved.gate.paramcount&31)-1; i >= 0; i--) 
							CPU_Push32(Mem_Lodsd(o_stack+i*4));
					CPU_Push32(oldcs);
					CPU_Push32(oldeip);
					}
				else
					{
					CPU_Push16(o_ss);		//save old stack
					CPU_Push16(o_esp);
					if (call.saved.gate.paramcount&31)
						for (Bits i = (call.saved.gate.paramcount&31)-1; i >= 0; i--)
							CPU_Push16(Mem_Lodsw(o_stack+i*2));
					CPU_Push16(oldcs);
					CPU_Push16(oldeip);
					}
				break;		
				}
			else if (n_cs_dpl > cpu.cpl)
				E_Exit("CALL-GATE: CS DPL>CPL");		// or #GP(sel)
		case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
		case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
			// zrdx extender
			if (call.Type() == DESC_386_CALL_GATE)
				{
				CPU_Push32(SegValue(cs));
				CPU_Push32(oldeip);
				}
			else
				{
				CPU_Push16(SegValue(cs));
				CPU_Push16(oldeip);
				}
			// Switch to new CS:EIP
			Segs.phys[cs] = n_cs_desc.GetBase();
			Segs.val[cs] = (n_cs_sel & 0xfffc) | cpu.cpl;
			cpu.code.big = n_cs_desc.Big()>0;
			reg_eip = n_eip;
			if (!use32)
				reg_eip &= 0xffff;
			break;
		default:
			E_Exit("CALL-GATE: CS no executable segment");
			}
		}			// Call Gates
		break;
	case DESC_DATA_EU_RW_NA:	// vbdos
	case DESC_INVALID:			// used by some installers
		CPU_Exception(EXCEPTION_GP, selector & 0xfffc);
		return;
	default:
		E_Exit("CALL: Descriptor type %x unsupported", call.Type());
		}
	}

void CPU_RET(bool use32, Bitu bytes)
	{
	if (!cpu.pmode)
		{
		Bitu new_ip, new_cs;
		if (!use32)
			{
			new_ip = CPU_Pop16();
			new_cs = CPU_Pop16();
			}
		else
			{
			new_ip = CPU_Pop32();
			new_cs = CPU_Pop32() & 0xffff;
			}
		reg_esp += bytes;
		SegSet16(cs, new_cs);
		reg_eip = new_ip;
		cpu.code.big = false;
		return;
		}
	Bitu offset,selector;
	if (!use32)
		selector = Mem_Lodsw(SegPhys(ss) + (reg_esp & cpu.stack.mask) + 2);
	else
		selector = Mem_Lodsd(SegPhys(ss) + (reg_esp & cpu.stack.mask) + 4) & 0xffff;
	Descriptor desc;
	Bitu rpl = selector & 3;
	if (rpl < cpu.cpl)
		{
		// win setup
		CPU_Exception(EXCEPTION_GP, selector & 0xfffc);
		return;
		}
	CPU_CHECK_COND((selector & 0xfffc) == 0, "RET:CS selector zero", EXCEPTION_GP, 0)
	CPU_CHECK_COND(!cpu.gdt.GetDescriptor(selector,desc), "RET:CS beyond limits", EXCEPTION_GP, selector & 0xfffc)
	if (cpu.cpl == rpl)
		{	
		// Return to same level
		switch (desc.Type())
			{
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
		case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			CPU_CHECK_COND(cpu.cpl != desc.DPL(), "RET to NC segment of other privilege", EXCEPTION_GP, selector & 0xfffc)
			goto RET_same_level;
		case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
		case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
			CPU_CHECK_COND(desc.DPL() > cpu.cpl, "RET to C segment of higher privilege", EXCEPTION_GP, selector & 0xfffc)
			break;
		default:
			E_Exit("RET from illegal descriptor type %X", desc.Type());
			}
RET_same_level:
		if (!desc.saved.seg.p)
			{
			// borland extender (RTM)
			CPU_Exception(EXCEPTION_NP, selector & 0xfffc);
			return;
			}
		// commit point
		if (!use32)
			{
			offset = CPU_Pop16();
			selector = CPU_Pop16();
			}
		else
			{
			offset = CPU_Pop32();
			selector = CPU_Pop32() & 0xffff;
			}
		Segs.phys[cs] = desc.GetBase();
		cpu.code.big = desc.Big() > 0;
		Segs.val[cs] = selector;
		reg_eip = offset;
		if (cpu.stack.big)
			reg_esp += bytes;
		else
			reg_sp += bytes;
		return;
		}
	else
		{
		// Return to outer level
		switch (desc.Type())
			{
		case DESC_CODE_N_NC_A:	case DESC_CODE_N_NC_NA:
		case DESC_CODE_R_NC_A:	case DESC_CODE_R_NC_NA:
			CPU_CHECK_COND(desc.DPL() != rpl, "RET to outer NC segment with DPL!=RPL", EXCEPTION_GP, selector & 0xfffc)
			break;
		case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
		case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
			CPU_CHECK_COND(desc.DPL() > rpl, "RET to outer C segment with DPL>RPL", EXCEPTION_GP, selector & 0xfffc)
			break;
		default:
			E_Exit("RET from illegal descriptor type %X", desc.Type());		// or #GP(selector)
			}
		CPU_CHECK_COND(!desc.saved.seg.p, "RET:Outer level:CS not present", EXCEPTION_NP, selector & 0xfffc)

		// commit point
		Bitu n_esp, n_ss;
		if (use32)
			{
			offset = CPU_Pop32();
			selector = CPU_Pop32() & 0xffff;
			reg_esp += bytes;
			n_esp = CPU_Pop32();
			n_ss = CPU_Pop32() & 0xffff;
			}
		else
			{
			offset = CPU_Pop16();
			selector = CPU_Pop16();
			reg_esp += bytes;
			n_esp = CPU_Pop16();
			n_ss = CPU_Pop16();
			}
		CPU_CHECK_COND((n_ss & 0xfffc) == 0, "RET to outer level with SS selector zero", EXCEPTION_GP, 0)

		Descriptor n_ss_desc;
		CPU_CHECK_COND(!cpu.gdt.GetDescriptor(n_ss, n_ss_desc), "RET:SS beyond limits", EXCEPTION_GP, n_ss & 0xfffc)
		CPU_CHECK_COND(((n_ss & 3) != rpl) || (n_ss_desc.DPL() != rpl), "RET to outer segment with invalid SS privileges", EXCEPTION_GP, n_ss & 0xfffc)
		switch (n_ss_desc.Type())
			{
		case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
			break;
		default:
			E_Exit("RET: SS selector type no writable data segment");	// or #GP(selector)
			}
		CPU_CHECK_COND(!n_ss_desc.saved.seg.p, "RET: Stack segment not present", EXCEPTION_SS, n_ss & 0xfffc)
		cpu.cpl = rpl;
		Segs.phys[cs] = desc.GetBase();
		cpu.code.big = desc.Big()>0;
		Segs.val[cs] = (selector&0xfffc) | cpu.cpl;
		reg_eip = offset;

		Segs.val[ss] = n_ss;
		Segs.phys[ss] = n_ss_desc.GetBase();
		if (n_ss_desc.Big())
			{
			cpu.stack.big = true;
			cpu.stack.mask = 0xffffffff;
			cpu.stack.notmask = 0;
			reg_esp = n_esp+bytes;
			}
		else
			{
			cpu.stack.big = false;
			cpu.stack.mask = 0xffff;
			cpu.stack.notmask = 0xffff0000;
			reg_sp = (n_esp & 0xffff)+bytes;
			}
		CPU_CheckSegments();
		return;
		}
	return;
	}

Bitu CPU_SLDT(void)
	{
	return cpu.gdt.SLDT();
	}

bool CPU_LLDT(Bitu selector)
	{
	if (!cpu.gdt.LLDT(selector))
		return true;
	return false;
	}

Bitu CPU_STR(void)
	{
	return cpu_tss.selector;
	}

bool CPU_LTR(Bitu selector)
	{
	if ((selector & 0xfffc) == 0)
		{
		cpu_tss.SetSelector(selector);
		return false;
		}
	TSS_Descriptor desc;
	if ((selector & 4) || (!cpu.gdt.GetDescriptor(selector,desc)))
		{
		LOG(LOG_CPU, LOG_ERROR)("LTR failed, selector=%X", selector);
		return CPU_PrepareException(EXCEPTION_GP, selector);
		}
	if ((desc.Type() == DESC_286_TSS_A) || (desc.Type() == DESC_386_TSS_A))
		{
		if (!desc.saved.seg.p)
			{
			LOG(LOG_CPU, LOG_ERROR)("LTR failed, selector=%X (not present)", selector);
			return CPU_PrepareException(EXCEPTION_NP, selector);
			}
		if (!cpu_tss.SetSelector(selector))
			E_Exit("LTR failed, selector=%X", selector);
		cpu_tss.desc.SetBusy(true);
		cpu_tss.SaveSelector();
		return false;
		}
	return CPU_PrepareException(EXCEPTION_GP, selector);		// Descriptor was no available TSS descriptor
	}

void CPU_LGDT(Bitu limit, Bitu base)
	{
	cpu.gdt.SetLimit(limit);
	cpu.gdt.SetBase(base);
	}

void CPU_LIDT(Bitu limit, Bitu base)
	{
	cpu.idt.SetLimit(limit);
	cpu.idt.SetBase(base);
	}

Bitu CPU_SGDT_base(void)
	{
	return cpu.gdt.GetBase();
	}

Bitu CPU_SGDT_limit(void)
	{
	return cpu.gdt.GetLimit();
	}

Bitu CPU_SIDT_base(void)
	{
	return cpu.idt.GetBase();
	}

Bitu CPU_SIDT_limit(void)
	{
	return cpu.idt.GetLimit();
	}

void CPU_SET_CRX(Bitu cr, Bitu value)
	{
	switch (cr)
		{
	case 0:
		{
		// CID
		if (cpu.cr0^value)
			{
			cpu.cr0 = value;
			if (value&CR0_PROTECTION)
				{
				cpu.pmode = true;
				PAGING_Enable((value&CR0_PAGING) ? true : false);
				}
			else
				{
				cpu.pmode = false;
				PAGING_Enable(false);
				if (value&CR0_PAGING)
					vLog("Paging requested without PE=1");
				}
			}
		break;
		}
	case 2:
		paging.cr2 = value;
		break;
	case 3:
		PAGING_SetDirBase(value);
		break;
	default:
		LOG(LOG_CPU,LOG_ERROR)("Unhandled MOV CR%d, %X", cr, value);
		break;
		}
	}

bool CPU_WRITE_CRX(Bitu cr, Bitu value)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	if ((cr == 1) || (cr >= 4))
		return CPU_PrepareException(EXCEPTION_UD, 0);
	CPU_SET_CRX(cr, value);
	return false;
	}

Bitu CPU_GET_CRX(Bitu cr)
	{
	switch (cr)
		{
	case 0:
		return (cpu.cr0 | 0x7ffffff0);		// CID
	case 2:
		return paging.cr2;
	case 3:
		return PAGING_GetDirBase() & 0xfffff000;
	default:
		LOG(LOG_CPU, LOG_ERROR)("Unhandled MOV XXX, CR%d", cr);
		break;
		}
	return 0;
	}

bool CPU_READ_CRX(Bitu cr, Bit32u & retvalue)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	if ((cr == 1) || (cr > 4))
		return CPU_PrepareException(EXCEPTION_UD, 0);
	retvalue = CPU_GET_CRX(cr);
	return false;
	}


bool CPU_WRITE_DRX(Bitu dr,Bitu value)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	switch (dr)
		{
	case 0:	case 1:	case 2:	case 3:
		cpu.drx[dr] = value;
		break;
	case 4:	case 6:
		cpu.drx[6] = (value|0xffff0ff0) & 0xffffefff;
		break;
	case 5:	case 7:
		cpu.drx[7] = (value|0x400) & 0xffff2fff;
		break;
	default:
		LOG(LOG_CPU, LOG_ERROR)("Unhandled MOV DR%d,%X", dr, value);
		break;
		}
	return false;
	}

bool CPU_READ_DRX(Bitu dr,Bit32u & retvalue)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	switch (dr)
		{
	case 0:	case 1:	case 2:	case 3:	case 6:	case 7:
		retvalue = cpu.drx[dr];
		break;
	case 4:
		retvalue = cpu.drx[6];
		break;
	case 5:
		retvalue = cpu.drx[7];
		break;
	default:
		LOG(LOG_CPU, LOG_ERROR)("Unhandled MOV XXX, DR%d", dr);
		retvalue = 0;
		break;
		}
	return false;
	}

bool CPU_WRITE_TRX(Bitu tr, Bitu value)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	switch (tr)
		{
//	case 3:
	case 6:	case 7:
		cpu.trx[tr] = value;
		return false;
	default:
		LOG(LOG_CPU, LOG_ERROR)("Unhandled MOV TR%d,%X", tr, value);
		break;
		}
	return CPU_PrepareException(EXCEPTION_UD, 0);
	}

bool CPU_READ_TRX(Bitu tr, Bit32u & retvalue)
	{
	// Check if privileged to access control registers
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	switch (tr)
		{
//	case 3:
	case 6:	case 7:
		retvalue = cpu.trx[tr];
		return false;
	default:
		LOG(LOG_CPU, LOG_ERROR)("Unhandled MOV XXX, TR%d", tr);
		break;
		}
	return CPU_PrepareException(EXCEPTION_UD, 0);
	}

Bitu CPU_SMSW(void)
	{
	return cpu.cr0;
	}

bool CPU_LMSW(Bitu word)
	{
	if (cpu.pmode && (cpu.cpl > 0))
		return CPU_PrepareException(EXCEPTION_GP, 0);
	word &= 0xf;
	if (cpu.cr0 & 1)
		word |= 1; 
	word |= (cpu.cr0&0xfffffff0);
	CPU_SET_CRX(0, word);
	return false;
	}

void CPU_ARPL(Bitu & dest_sel, Bitu src_sel)
	{
	FillFlags();
	if ((dest_sel & 3) < (src_sel & 3))
		{
		dest_sel = (dest_sel & 0xfffc) + (src_sel & 3);
//		dest_sel|=0xff3f0000;
		SETFLAGBIT(ZF, true);
		}
	else
		SETFLAGBIT(ZF, false);
	}
	
void CPU_LAR(Bitu selector, Bitu & ar)
	{
	FillFlags();
	if (selector == 0)
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	Descriptor desc;
	Bitu rpl = selector & 3;
	if (!cpu.gdt.GetDescriptor(selector,desc))
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	switch (desc.Type())
		{
	case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
		break;
	case DESC_286_INT_GATE:	case DESC_286_TRAP_GATE:
	case DESC_386_INT_GATE:	case DESC_386_TRAP_GATE:
		SETFLAGBIT(ZF, false);
		return;
	case DESC_LDT:
	case DESC_TASK_GATE:
	case DESC_286_TSS_A:		case DESC_286_TSS_B:
	case DESC_286_CALL_GATE:
	case DESC_386_TSS_A:		case DESC_386_TSS_B:
	case DESC_386_CALL_GATE:
	case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:
	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
	case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:
	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
	case DESC_CODE_N_NC_A:		case DESC_CODE_N_NC_NA:
	case DESC_CODE_R_NC_A:		case DESC_CODE_R_NC_NA:
		if (desc.DPL() < cpu.cpl || desc.DPL() < rpl)
			{
			SETFLAGBIT(ZF, false);
			return;
			}
		break;
	default:
		SETFLAGBIT(ZF, false);
		return;
		}
	// Valid descriptor
	ar = desc.saved.fill[1] & 0x00ffff00;
	SETFLAGBIT(ZF, true);
	}

void CPU_LSL(Bitu selector, Bitu & limit)
	{
	FillFlags();
	if (selector == 0)
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	Descriptor desc;
	if (!cpu.gdt.GetDescriptor(selector, desc))
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	switch (desc.Type())
		{
	case DESC_CODE_N_C_A:	case DESC_CODE_N_C_NA:
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:
		break;
	case DESC_LDT:
	case DESC_286_TSS_A:	case DESC_286_TSS_B:
	case DESC_386_TSS_A:	case DESC_386_TSS_B:
	case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:
	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
	case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:
	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
	case DESC_CODE_N_NC_A:		case DESC_CODE_N_NC_NA:
	case DESC_CODE_R_NC_A:		case DESC_CODE_R_NC_NA:
		if (desc.DPL() < cpu.cpl || desc.DPL() < (selector & 3))
			{
			SETFLAGBIT(ZF, false);
			return;
			}
		break;
	default:
		SETFLAGBIT(ZF, false);
		return;
	}
	limit = desc.GetLimit();
	SETFLAGBIT(ZF, true);
	}

void CPU_VERR(Bitu selector)
	{
	FillFlags();
	if (selector == 0)
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	Descriptor desc;
	if (!cpu.gdt.GetDescriptor(selector, desc))
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	switch (desc.Type())
		{
	case DESC_CODE_R_C_A:	case DESC_CODE_R_C_NA:	
		// Conforming readable code segments can be always read 
		break;
	case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:
	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
	case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:
	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
	case DESC_CODE_R_NC_A:		case DESC_CODE_R_NC_NA:
		if (desc.DPL() < cpu.cpl || desc.DPL() < (selector & 3))
			{
			SETFLAGBIT(ZF, false);
			return;
			}
		break;
	default:
		SETFLAGBIT(ZF, false);
		return;
		}
	SETFLAGBIT(ZF, true);
	}

void CPU_VERW(Bitu selector)
	{
	FillFlags();
	if (selector == 0)
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	Descriptor desc;
	if (!cpu.gdt.GetDescriptor(selector, desc))
		{
		SETFLAGBIT(ZF, false);
		return;
		}
	switch (desc.Type())
		{
	case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
	case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		if (desc.DPL() < cpu.cpl || desc.DPL() < (selector & 3))
			{
			SETFLAGBIT(ZF, false);
			return;
			}
		break;
	default:
		SETFLAGBIT(ZF, false);
		return;
		}
	SETFLAGBIT(ZF, true);
	}

bool CPU_SetSegGeneral(SegNames seg, Bitu value)
	{
	value &= 0xffff;
	if (!cpu.pmode)
		{
		Segs.val[seg] = value;
		Segs.phys[seg] = value << 4;
		if (seg == ss)
			{
			cpu.stack.big = false;
			cpu.stack.mask = 0xffff;
			cpu.stack.notmask = 0xffff0000;
			}
		return false;
		}
	if (seg == ss)
		{
		// Stack needs to be non-zero
		if ((value & 0xfffc) == 0)
			E_Exit("CPU_SetSegGeneral: Stack segment zero");
		Descriptor desc;
		if (!cpu.gdt.GetDescriptor(value, desc))
			E_Exit("CPU_SetSegGeneral: Stack segment beyond limits");
		if (((value & 3) != cpu.cpl) || (desc.DPL() != cpu.cpl))
			E_Exit("CPU_SetSegGeneral: Stack segment with invalid privileges");
		switch (desc.Type())
			{
		case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
			break;
		default:
			// Earth Siege 1
			return CPU_PrepareException(EXCEPTION_GP, value & 0xfffc);
			}
		if (!desc.saved.seg.p)
			return CPU_PrepareException(EXCEPTION_SS, value & 0xfffc);
		Segs.val[seg] = value;
		Segs.phys[seg] = desc.GetBase();
		if (desc.Big())
			{
			cpu.stack.big = true;
			cpu.stack.mask = 0xffffffff;
			cpu.stack.notmask = 0;
			}
		else
			{
			cpu.stack.big = false;
			cpu.stack.mask = 0xffff;
			cpu.stack.notmask = 0xffff0000;
			}
		}
	else
		{
		if ((value&0xfffc) == 0)
			{
			Segs.val[seg] = value;
			Segs.phys[seg] = 0;	// ??
			return false;
			}
		Descriptor desc;
		if (!cpu.gdt.GetDescriptor(value, desc))
			return CPU_PrepareException(EXCEPTION_GP,value & 0xfffc);
		switch (desc.Type())
			{
		case DESC_DATA_EU_RO_NA:	case DESC_DATA_EU_RO_A:
		case DESC_DATA_EU_RW_NA:	case DESC_DATA_EU_RW_A:
		case DESC_DATA_ED_RO_NA:	case DESC_DATA_ED_RO_A:
		case DESC_DATA_ED_RW_NA:	case DESC_DATA_ED_RW_A:
		case DESC_CODE_R_NC_A:		case DESC_CODE_R_NC_NA:
			if (((value&3) > desc.DPL()) || (cpu.cpl > desc.DPL()))		// extreme pinball
				return CPU_PrepareException(EXCEPTION_GP,value & 0xfffc);
			break;
		case DESC_CODE_R_C_A:		case DESC_CODE_R_C_NA:
			break;
		default:
			// gabriel knight
			return CPU_PrepareException(EXCEPTION_GP, value & 0xfffc);
			}
		if (!desc.saved.seg.p)			// win
			return CPU_PrepareException(EXCEPTION_NP,value & 0xfffc);
		Segs.val[seg] = value;
		Segs.phys[seg] = desc.GetBase();
		}
	return false;
	}

bool CPU_Pop16Seg(SegNames seg)
	{
	if (!CPU_SetSegGeneral(seg, Mem_Lodsw(SegPhys(ss)+(reg_esp&cpu.stack.mask))))
        {
		reg_esp = (reg_esp&cpu.stack.notmask)|((reg_esp+2)&cpu.stack.mask);
		return false;
        }
	return true;
	}

 bool CPU_Pop32Seg(SegNames seg)
	{
	if (!CPU_SetSegGeneral(seg, Mem_Lodsw(SegPhys(ss)+(reg_esp&cpu.stack.mask))))
        {
		reg_esp = (reg_esp&cpu.stack.notmask)|((reg_esp+4)&cpu.stack.mask);
		return false;
        }
	return true;
	}

static Bits HLT_Decode(void)
	{
	// Once an interrupt occurs, it should change cpu core
	if (reg_eip != cpu.hlt.eip || SegValue(cs) != cpu.hlt.cs)
		cpudecoder = cpu.hlt.old_decoder;
	else
		CPU_Cycles = 0;
	return 0;
	}

void CPU_HLT(Bitu oldeip)
	{
	reg_eip = oldeip;
	CPU_Cycles = 0;
	cpu.hlt.cs = SegValue(cs);
	cpu.hlt.eip = reg_eip;
	cpu.hlt.old_decoder = cpudecoder;
	cpudecoder = &HLT_Decode;
	}

void CPU_ENTER(bool use32, Bitu bytes, Bitu level)
	{
	level &= 0x1f;
	Bitu sp_index = reg_esp&cpu.stack.mask;
	Bitu bp_index = reg_ebp&cpu.stack.mask;
	if (!use32)
		{
		sp_index -= 2;
		Mem_Stosw(SegPhys(ss)+sp_index, reg_bp);
		reg_bp = (Bit16u)(reg_esp-2);
		if (level)
			{
			for (Bitu i = 1; i < level; i++)
				{	
				sp_index -= 2;
				bp_index -= 2;
				Mem_Stosw(SegPhys(ss)+sp_index, Mem_Lodsw(SegPhys(ss)+bp_index));
				}
			sp_index -= 2;
			Mem_Stosw(SegPhys(ss)+sp_index, reg_bp);
			}
		}
	else
		{
		sp_index -= 4;
        Mem_Stosd(SegPhys(ss)+sp_index, reg_ebp);
		reg_ebp = (reg_esp-4);
		if (level)
			{
			for (Bitu i = 1; i < level; i++)
				{	
				sp_index -= 4;
				bp_index -= 4;
				Mem_Stosd(SegPhys(ss)+sp_index, Mem_Lodsd(SegPhys(ss)+bp_index));
				}
			sp_index -= 4;
			Mem_Stosd(SegPhys(ss)+sp_index, reg_ebp);
			}
		}
	sp_index -= bytes;
	reg_esp = (reg_esp&cpu.stack.notmask)|((sp_index)&cpu.stack.mask);
	}

void CPU_Init()
	{
	reg_eax = 0;
	reg_ebx = 0;
	reg_ecx = 0;
	reg_edx = 0;
	reg_edi = 0;
	reg_esi = 0;
	reg_ebp = 0;
	reg_esp = 0;
	
	SegSet16(cs, 0);
	SegSet16(ds, 0);
	SegSet16(es, 0);
	SegSet16(fs, 0);
	SegSet16(gs, 0);
	SegSet16(ss, 0);
	
	CPU_SetFlags(FLAG_IF, FMASK_ALL);							// Enable interrupts
	cpu.cr0 = 0xffffffff;
	CPU_SET_CRX(0, 0);											// Initialize
	cpu.code.big = false;
	cpu.stack.mask = 0xffff;
	cpu.stack.notmask = 0xffff0000;
	cpu.stack.big = false;
	cpu.trap_skip = false;
	cpu.idt.SetBase(0);
	cpu.idt.SetLimit(1023);

	for (Bitu i = 0; i < 7; i++)
		{
		cpu.drx[i] = 0;
		cpu.trx[i] = 0;
		}
	cpu.drx[6] = 0xffff1ff0;
	cpu.drx[7] = 0x00000400;

	cpudecoder = &CPU_Core_Normal_Run;
	CPU_JMP(false, 0, 0);										// Setup the first cpu core
	}
