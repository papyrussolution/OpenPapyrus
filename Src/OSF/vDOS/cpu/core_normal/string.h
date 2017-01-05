enum STRING_OP {
	R_OUTSB, R_OUTSW, R_OUTSD,
	R_INSB, R_INSW, R_INSD,
	R_MOVSB, R_MOVSW, R_MOVSD,
	R_LODSB, R_LODSW, R_LODSD,
	R_STOSB, R_STOSW, R_STOSD,
	R_SCASB, R_SCASW, R_SCASD,
	R_CMPSB, R_CMPSW, R_CMPSD
};

#define LoadD(_BLAH) _BLAH
#define R_OPTAT 6
#define BaseDI SegBase(es)
#define BaseSI BaseDS

static void DoString(STRING_OP type)
	{
	Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
	Bitu count = reg_ecx&add_mask;
	if (count == 0)																	// Seems to occur sometimes (calculated (E)CX)
		return;																		// Also required for do...while handling single operations
	Bitu si_index = reg_esi&add_mask;
	Bitu di_index = reg_edi&add_mask;
	reg_ecx &= ~add_mask;
	CPU_Cycles -= 2+count/64;
	Bits add_index = cpu.direction;

	switch (type)
		{
	case R_OUTSB:
		do
			{
			IO_WriteB(reg_dx, Mem_Lodsb(BaseSI+si_index));
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_OUTSW:
		add_index <<= 1;
		do
			{
			IO_WriteW(reg_dx, Mem_Lodsw(BaseSI+si_index));
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_OUTSD:
		add_index <<= 2;
		do
			{
			IO_WriteD(reg_dx, Mem_Lodsd(BaseSI+si_index));
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_INSB:
		do
			{
			Mem_Stosb(BaseDI+di_index, IO_ReadB(reg_dx));
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_INSW:
		add_index <<= 1;
		do
			{
			Mem_Stosw(BaseDI+di_index, IO_ReadW(reg_dx));
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_INSD:
		add_index <<= 2;
		do
			{
			Mem_Stosd(BaseDI+di_index, IO_ReadD(reg_dx));
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_MOVSB:
		if (count >= R_OPTAT && add_index > 0)										// Try to optimize if direction is up (down is used rarely)
			{
			Bitu di_newindex = di_index+count-1;
			Bitu si_newindex = si_index+count-1;
			if (((di_newindex|si_newindex)&add_mask) == (di_newindex|si_newindex))	// If no wraparounds, use Mem_rMovsb()
				{
				Mem_rMovsb(BaseDI+di_index, BaseSI+si_index, count);
				reg_esi = (reg_esi&~add_mask)|((si_newindex+1)&add_mask);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+1)&add_mask);
				break;
				}
			}
		do																			// Count too low or SI/DI wraps around
			{
			Mem_Stosb(BaseDI+di_index, Mem_Lodsb(BaseSI+si_index));
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_MOVSW:
		if (count >= R_OPTAT && add_index > 0)										// Try to optimize if direction is up
			{
			Bitu di_newindex = di_index+(count*2)-2;
			Bitu si_newindex = si_index+(count*2)-2;
			if (((di_newindex|si_newindex)&add_mask) == (di_newindex|si_newindex))	// If no wraparounds, use Mem_rMovsb()
				{
				Mem_rMovsb(BaseDI+di_index, BaseSI+si_index, count*2);
				reg_esi = (reg_esi&~add_mask)|((si_newindex+2)&add_mask);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+2)&add_mask);
				break;
				}
			}
		add_index <<= 1;
		do																			// Count too low or SI/DI wraps around
			{
			Mem_Stosw(BaseDI+di_index, Mem_Lodsw(BaseSI+si_index));
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_MOVSD:
		if (count >= R_OPTAT && add_index > 0)										// Try to optimize if direction is up
			{
			Bitu di_newindex = di_index+(count*4)-4;
			Bitu si_newindex = si_index+(count*4)-4;
			if (((di_newindex|si_newindex)&add_mask) == (di_newindex|si_newindex))	// If no wraparounds, use Mem_rMovsb()
				{
				Mem_rMovsb(BaseDI+di_index, BaseSI+si_index, count*4);
				reg_esi = (reg_esi&~add_mask)|((si_newindex+4)&add_mask);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+4)&add_mask);
				break;
				}
			}
		add_index <<= 2;
		do
			{
			Mem_Stosd(BaseDI+di_index, Mem_Lodsd(BaseSI+si_index));
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_LODSB:
		do
			{
			reg_al = Mem_Lodsb(BaseSI+si_index);
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_LODSW:
		add_index <<= 1;
		do
			{
			reg_ax = Mem_Lodsw(BaseSI+si_index);
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_LODSD:
		add_index <<= 2;
		do
			{
			reg_eax = Mem_Lodsd(BaseSI+si_index);
			si_index = (si_index+add_index)&add_mask;
			}
		while (--count);
		reg_esi = (reg_esi&~add_mask)|si_index;
		break;
	case R_STOSB:
		{
		if (count >= R_OPTAT)														// Try to optimize
			{
			Bitu di_newindex = di_index+(add_index*count)-add_index;
			if ((di_newindex&add_mask) == di_newindex)								// If no wraparound, use Mem_rStosb()
				{
				Mem_rStosb(BaseDI+(add_index > 0 ? di_index : di_newindex), reg_al, count);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+add_index)&add_mask);
				break;
				}
			}
		do																			// Count too low or DI wraps around
			{
			Mem_Stosb(BaseDI+di_index, reg_al);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
		}
	case R_STOSW:
		add_index <<= 1;
		if (count >= R_OPTAT)														// Try to optimize
			{
			Bitu di_newindex = di_index+(add_index*count)-add_index;
			if ((di_newindex&add_mask) == di_newindex)								// If no wraparound, use Mem_rStosw()
				{
				if (reg_al == reg_ah)												// NB, memset runtime is 32 bits optimized
					Mem_rStosb(BaseDI+(add_index > 0 ? di_index : di_newindex), reg_al, count*2);
				else
					Mem_rStosw(BaseDI+(add_index > 0 ? di_index : di_newindex), reg_ax, count);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+add_index)&add_mask);
				break;
				}
			}
		do																			// Count too low or DI wraps around
			{
			Mem_Stosw(BaseDI+di_index, reg_ax);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_STOSD:
		add_index <<= 2;
		if (count >= R_OPTAT)														// Try to optimize
			{
			Bitu di_newindex = di_index+(add_index*count)-add_index;
			if ((di_newindex&add_mask) == di_newindex)								// If no wraparound, use Mem_Stosd()
				{
				if ((reg_eax>>16) == reg_ax && reg_ah == reg_al)					// NB, memset runtime is 32 bits optimized
					Mem_rStosb(BaseDI+(add_index > 0 ? di_index : di_newindex), reg_al, count*4);
				else
					Mem_rStosd(BaseDI+(add_index > 0 ? di_index : di_newindex), reg_eax, count);
				reg_edi = (reg_edi&~add_mask)|((di_newindex+add_index)&add_mask);
				break;
				}
			}
		do
			{
			Mem_Stosd(BaseDI+di_index, reg_eax);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count);
		reg_edi = (reg_edi&~add_mask)|di_index;
		break;
	case R_SCASB:
		{
//if (add_index < 0)
//vLog("SCASB %05x, addr: %05x, byte: %02x, repeat: %5d, add_index: %x", reg_eip, BaseDI+di_index, reg_al, count, add_index);
		Bit8u val2;
		do
			{
			val2 = Mem_Lodsb(BaseDI+di_index);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (reg_al == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_edi = (reg_edi&~add_mask)|di_index;
//if (add_index < 0)
//vLog("SCASB %05x, addr: %05x, byte: %02x, repeat: %5d, reg_ecx: %x", reg_eip, BaseDI+di_index, val2, count, reg_ecx);
		CMPB(reg_al, val2, LoadD, 0);
		}
		break;
	case R_SCASW:
		{
		add_index <<= 1;
		Bit16u val2;
		do
			{
			val2 = Mem_Lodsw(BaseDI+di_index);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (reg_ax == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_edi = (reg_edi&~add_mask)|di_index;
		CMPW(reg_ax, val2, LoadD, 0);
		}
		break;
	case R_SCASD:
		{
		add_index <<= 2;
		Bit32u val2;
		do
			{
			val2 = Mem_Lodsd(BaseDI+di_index);
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (reg_eax == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_edi = (reg_edi&~add_mask)|di_index;
		CMPD(reg_eax, val2, LoadD, 0);
		}
		break;
	case R_CMPSB:
		{
		Bit8u val1, val2;
		do
			{
			val1 = Mem_Lodsb(BaseSI+si_index);
			val2 = Mem_Lodsb(BaseDI+di_index);
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (val1 == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		CMPB(val1, val2, LoadD, 0);
		}
		break;
	case R_CMPSW:
		{
		add_index <<= 1;
		Bit16u val1, val2;
		do
			{
			val1 = Mem_Lodsw(BaseSI+si_index);
			val2 = Mem_Lodsw(BaseDI+di_index);
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (val1 == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		CMPW(val1, val2, LoadD, 0);
		}
		break;
	case R_CMPSD:
		{
		add_index <<= 2;
		Bit32u val1, val2;
		do
			{
			val1 = Mem_Lodsd(BaseSI+si_index);
			val2 = Mem_Lodsd(BaseDI+di_index);
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			}
		while (--count && (val1 == val2) == core.rep_zero);
		reg_ecx |= count;
		CPU_Cycles += count/64;
		reg_esi = (reg_esi&~add_mask)|si_index;
		reg_edi = (reg_edi&~add_mask)|di_index;
		CMPD(val1, val2, LoadD, 0);
		}
		break;
		}
	}
