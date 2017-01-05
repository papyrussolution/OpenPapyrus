	CASE_0F_W(0x00)												/* GRP 6 Exxx */
		{
			if (!cpu.pmode)
				goto illegal_opcode;
			GetRM;Bitu which=(rm>>3)&7;
			switch (which) {
			case 0x00:	/* SLDT */
			case 0x01:	/* STR */
				{
					Bitu saveval;
					if (!which) saveval=CPU_SLDT();
					else saveval=CPU_STR();
					if (rm >= 0xc0) {GetEArw;*earw=saveval;}
					else {GetEAa;Mem_Stosw(eaa,saveval);}
				}
				break;
			case 0x02:case 0x03:case 0x04:case 0x05:
				{
					Bitu loadval;
					if (rm >= 0xc0) {GetEArw;loadval=*earw;}
					else {GetEAa;loadval=Mem_Lodsw(eaa);}
					switch (which) {
					case 0x02:
						if (cpu.cpl) EXCEPTION(EXCEPTION_GP);
						if (CPU_LLDT(loadval)) RUNEXCEPTION();
						break;
					case 0x03:
						if (cpu.cpl) EXCEPTION(EXCEPTION_GP);
						if (CPU_LTR(loadval)) RUNEXCEPTION();
						break;
					case 0x04:
						CPU_VERR(loadval);
						break;
					case 0x05:
						CPU_VERW(loadval);
						break;
					}
				}
				break;
			default:
				goto illegal_opcode;
			}
		}
		break;
	CASE_0F_W(0x01)												/* Group 7 Ew */
		{
			GetRM;Bitu which=(rm>>3)&7;
			if (rm < 0xc0)	{ //First ones all use EA
				GetEAa;Bitu limit;
				switch (which) {
				case 0x00:										/* SGDT */
					Mem_Stosw(eaa,CPU_SGDT_limit());
					Mem_Stosd(eaa+2,CPU_SGDT_base());
					break;
				case 0x01:										/* SIDT */
					Mem_Stosw(eaa,CPU_SIDT_limit());
					Mem_Stosd(eaa+2,CPU_SIDT_base());
					break;
				case 0x02:										/* LGDT */
					if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
					CPU_LGDT(Mem_Lodsw(eaa),Mem_Lodsd(eaa+2) & 0xFFFFFF);
					break;
				case 0x03:										/* LIDT */
					if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
					CPU_LIDT(Mem_Lodsw(eaa),Mem_Lodsd(eaa+2) & 0xFFFFFF);
					break;
				case 0x04:										/* SMSW */
					Mem_Stosw(eaa,CPU_SMSW());
					break;
				case 0x06:										/* LMSW */
					limit=Mem_Lodsw(eaa);
					if (CPU_LMSW(limit)) RUNEXCEPTION();
					break;
				case 0x07:										/* INVLPG */
					if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
//					PAGING_ClearTLB();
					break;
				}
			} else {
				GetEArw;
				switch (which) {
				case 0x02:										/* LGDT */
					if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
					goto illegal_opcode;
				case 0x03:										/* LIDT */
					if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
					goto illegal_opcode;
				case 0x04:										/* SMSW */
					*earw=CPU_SMSW();
					break;
				case 0x06:										/* LMSW */
					if (CPU_LMSW(*earw)) RUNEXCEPTION();
					break;
				default:
					goto illegal_opcode;
				}
			}
		}
		break;
	CASE_0F_W(0x02)												/* LAR Gw,Ew */
		{
			if (!cpu.pmode)
				goto illegal_opcode;
			GetRMrw;Bitu ar=*rmrw;
			if (rm >= 0xc0) {
				GetEArw;CPU_LAR(*earw, ar);
			} else {
				GetEAa;CPU_LAR(Mem_Lodsw(eaa), ar);
			}
			*rmrw=(Bit16u)ar;
		}
		break;
	CASE_0F_W(0x03)												/* LSL Gw,Ew */
		{
			if (!cpu.pmode)
				goto illegal_opcode;
			GetRMrw;Bitu limit=*rmrw;
			if (rm >= 0xc0) {
				GetEArw;CPU_LSL(*earw,limit);
			} else {
				GetEAa;CPU_LSL(Mem_Lodsw(eaa),limit);
			}
			*rmrw=(Bit16u)limit;
		}
		break;
	CASE_0F_B(0x06)												/* CLTS */
		if (cpu.pmode && cpu.cpl) EXCEPTION(EXCEPTION_GP);
		cpu.cr0&=(~CR0_TASKSWITCH);
		break;
	CASE_0F_B(0x08)												/* INVD */
	CASE_0F_B(0x09)												/* WBINVD */
		goto illegal_opcode;
	CASE_0F_B(0x12)																	// UMOV ADC Gb,Eb
		if (reg_ax == 0x2506 && reg_eip == 0x4624)									// 4DOS checking for UMOV support
			{
			RMGbEb(ADCB);
			break;
			}
		goto illegal_opcode;
	CASE_0F_B(0x20)																	// MOV Rd.CRx
		{
		GetRM;
		GetEArd;
		Bit32u crx_value;
		if (CPU_READ_CRX((rm>>3)&7, crx_value))
			RUNEXCEPTION();
		*eard = crx_value;
		}
		break;
	CASE_0F_B(0x21)																	// MOV Rd,DRx
		{
		GetRM;
		GetEArd;
		Bit32u drx_value;
		if (CPU_READ_DRX((rm>>3)&7, drx_value))
			RUNEXCEPTION();
		*eard = drx_value;
		}
		break;
	CASE_0F_B(0x22)																	// MOV CRx,Rd
		{
		GetRM;
		GetEArd;
		if (CPU_WRITE_CRX((rm>>3)&7, *eard))
			RUNEXCEPTION();
		}
		break;
	CASE_0F_B(0x23)																	// MOV DRx,Rd
		{
		GetRM;
		GetEArd;
		if (CPU_WRITE_DRX((rm>>3)&7, *eard))
			RUNEXCEPTION();
		}
		break;
	CASE_0F_B(0x24)																	// MOV Rd,TRx
		{
		GetRM;
		GetEArd;
		Bit32u trx_value;
		if (CPU_READ_TRX((rm>>3)&7, trx_value))
			RUNEXCEPTION();
		*eard = trx_value;
		}
		break;
	CASE_0F_B(0x26)																	// MOV TRx,Rd
		{
		GetRM;
		GetEArd;
		if (CPU_WRITE_TRX((rm>>3)&7, *eard)) RUNEXCEPTION();
		}
		break;
	CASE_0F_B(0x31)												/* RDTSC */
		goto illegal_opcode;
	CASE_0F_W(0x80)												/* JO */
		JumpCond16_w(TFLG_O);
	CASE_0F_W(0x81)												/* JNO */
		JumpCond16_w(TFLG_NO);
	CASE_0F_W(0x82)												/* JB */
		JumpCond16_w(TFLG_B);
	CASE_0F_W(0x83)												/* JNB */
		JumpCond16_w(TFLG_NB);
	CASE_0F_W(0x84)												/* JZ */
		JumpCond16_w(TFLG_Z);
	CASE_0F_W(0x85)												/* JNZ */
		JumpCond16_w(TFLG_NZ);
	CASE_0F_W(0x86)												/* JBE */
		JumpCond16_w(TFLG_BE);
	CASE_0F_W(0x87)												/* JNBE */
		JumpCond16_w(TFLG_NBE);
	CASE_0F_W(0x88)												/* JS */
		JumpCond16_w(TFLG_S);
	CASE_0F_W(0x89)												/* JNS */
		JumpCond16_w(TFLG_NS);
	CASE_0F_W(0x8a)												/* JP */
		JumpCond16_w(TFLG_P);
	CASE_0F_W(0x8b)												/* JNP */
		JumpCond16_w(TFLG_NP);
	CASE_0F_W(0x8c)												/* JL */
		JumpCond16_w(TFLG_L);
	CASE_0F_W(0x8d)												/* JNL */
		JumpCond16_w(TFLG_NL);
	CASE_0F_W(0x8e)												/* JLE */
		JumpCond16_w(TFLG_LE);
	CASE_0F_W(0x8f)												/* JNLE */
		JumpCond16_w(TFLG_NLE);
	CASE_0F_B(0x90)												/* SETO */
		SETcc(TFLG_O);
		break;
	CASE_0F_B(0x91)												/* SETNO */
		SETcc(TFLG_NO);
		break;
	CASE_0F_B(0x92)												/* SETB */
		SETcc(TFLG_B);
		break;
	CASE_0F_B(0x93)												/* SETNB */
		SETcc(TFLG_NB);
		break;
	CASE_0F_B(0x94)												/* SETZ */
		SETcc(TFLG_Z);
		break;
	CASE_0F_B(0x95)												/* SETNZ */
		SETcc(TFLG_NZ);
		break;
	CASE_0F_B(0x96)												/* SETBE */
		SETcc(TFLG_BE);
		break;
	CASE_0F_B(0x97)												/* SETNBE */
		SETcc(TFLG_NBE);
		break;
	CASE_0F_B(0x98)												/* SETS */
		SETcc(TFLG_S);
		break;
	CASE_0F_B(0x99)												/* SETNS */
		SETcc(TFLG_NS);
		break;
	CASE_0F_B(0x9a)												/* SETP */
		SETcc(TFLG_P);
		break;
	CASE_0F_B(0x9b)												/* SETNP */
		SETcc(TFLG_NP);
		break;
	CASE_0F_B(0x9c)												/* SETL */
		SETcc(TFLG_L);
		break;
	CASE_0F_B(0x9d)												/* SETNL */
		SETcc(TFLG_NL);
		break;
	CASE_0F_B(0x9e)												/* SETLE */
		SETcc(TFLG_LE);
		break;
	CASE_0F_B(0x9f)												/* SETNLE */
		SETcc(TFLG_NLE);
		break;
	CASE_0F_W(0xa0)												/* PUSH FS */		
		CPU_Push16(SegValue(fs));
		break;
	CASE_0F_W(0xa1)												/* POP FS */	
		if (CPU_Pop16Seg(fs))
			RUNEXCEPTION();
		break;
	CASE_0F_B(0xa2)												/* CPUID */
		goto illegal_opcode;
	CASE_0F_W(0xa3)												/* BT Ew,Gw */
		{
		FillFlags();
		GetRMrw;
		Bit16u mask = 1 << (*rmrw & 15);
		if (rm >= 0xc0)
			{
			GetEArw;
			SETFLAGBIT(CF, (*earw & mask));
			}
		else
			{
			GetEAa;
			eaa += (((Bit16s)*rmrw)>>4)*2;
			Bit16u old = Mem_Lodsw(eaa);
			SETFLAGBIT(CF, (old & mask));
			}
		break;
		}
	CASE_0F_W(0xa4)												/* SHLD Ew,Gw,Ib */
		RMEwGwOp3(DSHLW, Fetchb());
		break;
	CASE_0F_W(0xa5)												/* SHLD Ew,Gw,CL */
		RMEwGwOp3(DSHLW, reg_cl);
		break;
	CASE_0F_W(0xa8)												/* PUSH GS */		
		CPU_Push16(SegValue(gs));
		break;
	CASE_0F_W(0xa9)												/* POP GS */		
		if (CPU_Pop16Seg(gs))
			RUNEXCEPTION();
		break;
	CASE_0F_W(0xab)												/* BTS Ew,Gw */
		{
		FillFlags();
		GetRMrw;
		Bit16u mask = 1 << (*rmrw & 15);
		if (rm >= 0xc0)
			{
			GetEArw;
			SETFLAGBIT(CF, (*earw & mask));
			*earw |= mask;
			}
		else
			{
			GetEAa;
			eaa += (((Bit16s)*rmrw)>>4)*2;
			Bit16u old = Mem_Lodsw(eaa);
			SETFLAGBIT(CF, (old & mask));
			Mem_Stosw(eaa, old | mask);
			}
		break;
		}
	CASE_0F_W(0xac)												/* SHRD Ew,Gw,Ib */
		RMEwGwOp3(DSHRW, Fetchb());
		break;
	CASE_0F_W(0xad)												/* SHRD Ew,Gw,CL */
		RMEwGwOp3(DSHRW, reg_cl);
		break;
	CASE_0F_W(0xaf)												/* IMUL Gw,Ew */
		RMGwEwOp3(DIMULW, *rmrw);
		break;
	CASE_0F_B(0xb0) 											/* cmpxchg Eb,Gb */
	CASE_0F_W(0xb1) 											/* cmpxchg Ew,Gw */
		goto illegal_opcode;
	CASE_0F_W(0xb2)												/* LSS Ew */
		{	
		GetRMrw;
		if (rm >= 0xc0)
			goto illegal_opcode;
		GetEAa;
		if (CPU_SetSegGeneral(ss, Mem_Lodsw(eaa+2)))
			RUNEXCEPTION();
		*rmrw = Mem_Lodsw(eaa);
		break;
		}
	CASE_0F_W(0xb3)												/* BTR Ew,Gw */
		{
		FillFlags();
		GetRMrw;
		Bit16u mask = 1 << (*rmrw & 15);
		if (rm >= 0xc0)
			{
			GetEArw;
			SETFLAGBIT(CF, (*earw & mask));
			*earw &= ~mask;
			}
		else
			{
			GetEAa;
			eaa += (((Bit16s)*rmrw)>>4)*2;
			Bit16u old = Mem_Lodsw(eaa);
			SETFLAGBIT(CF, (old & mask));
			Mem_Stosw(eaa, old & ~mask);
			}
		break;
		}
	CASE_0F_W(0xb4)												/* LFS Ew */
		{	
		GetRMrw;
		if (rm >= 0xc0)
			goto illegal_opcode;
		GetEAa;
		if (CPU_SetSegGeneral(fs, Mem_Lodsw(eaa+2)))
			RUNEXCEPTION();
		*rmrw = Mem_Lodsw(eaa);
		break;
		}
	CASE_0F_W(0xb5)												/* LGS Ew */
		{	
		GetRMrw;
		if (rm >= 0xc0)
			goto illegal_opcode;
		GetEAa;
		if (CPU_SetSegGeneral(gs, Mem_Lodsw(eaa+2)))
			RUNEXCEPTION();
		*rmrw = Mem_Lodsw(eaa);
		break;
		}
	CASE_0F_W(0xb6)												/* MOVZX Gw,Eb */
		{
		GetRMrw;															
		if (rm >= 0xc0)
			{
			GetEArb;
			*rmrw = *earb;
			}
		else
			{
			GetEAa;
			*rmrw = Mem_Lodsb(eaa);
			}
		break;
		}
	CASE_0F_W(0xb7)												/* MOVZX Gw,Ew */
	CASE_0F_W(0xbf)												/* MOVSX Gw,Ew */
		{
		GetRMrw;															
		if (rm >= 0xc0)
			{
			GetEArw;
			*rmrw = *earw;
			}
		else
			{
			GetEAa;
			*rmrw = Mem_Lodsw(eaa);
			}
		break;
		}
	CASE_0F_W(0xba)												/* GRP8 Ew,Ib */
		{
		FillFlags();
		GetRM;
		if (rm >= 0xc0)
			{
			GetEArw;
			Bit16u mask = 1 << (Fetchb() & 15);
			SETFLAGBIT(CF, (*earw & mask));
			switch (rm & 0x38)
				{
			case 0x20:										/* BT */
				break;
			case 0x28:										/* BTS */
				*earw |= mask;
				break;
			case 0x30:										/* BTR */
				*earw &= ~mask;
				break;
			case 0x38:										/* BTC */
				*earw ^= mask;
				break;
			default:
				E_Exit("CPU:0F:BA: Illegal subfunction %X", rm & 0x38);
				}
			}
		else
			{
			GetEAa;
			Bit16u old = Mem_Lodsw(eaa);
			Bit16u mask = 1 << (Fetchb() & 15);
			SETFLAGBIT(CF, (old & mask));
			switch (rm & 0x38)
				{
			case 0x20:										/* BT */
				break;
			case 0x28:										/* BTS */
				Mem_Stosw(eaa, old|mask);
				break;
			case 0x30:										/* BTR */
				Mem_Stosw(eaa, old & ~mask);
				break;
			case 0x38:										/* BTC */
				Mem_Stosw(eaa, old ^ mask);
				break;
			default:
				E_Exit("CPU:0F:BA: Illegal subfunction %X", rm & 0x38);
				}
			}
		break;
		}
	CASE_0F_W(0xbb)												/* BTC Ew,Gw */
		{
		FillFlags();
		GetRMrw;
		Bit16u mask = 1 << (*rmrw & 15);
		if (rm >= 0xc0)
			{
			GetEArw;
			SETFLAGBIT(CF, (*earw & mask));
			*earw ^= mask;
			}
		else
			{
			GetEAa;
			eaa += (((Bit16s)*rmrw)>>4)*2;
			Bit16u old = Mem_Lodsw(eaa);
			SETFLAGBIT(CF, (old & mask));
			Mem_Stosw(eaa, old ^ mask);
			}
		break;
		}
	CASE_0F_W(0xbc)												/* BSF Gw,Ew */
		{
		GetRMrw;
		Bit16u result, value;
		if (rm >= 0xc0)
			{
			GetEArw;
			value = *earw;
			} 
		else
			{
			GetEAa;
			value = Mem_Lodsw(eaa);
			}
		if (value == 0)
			SETFLAGBIT(ZF,true);
		else
			{
			result = 0;
			while ((value & 0x01) == 0)
				{
				result++;
				value >>= 1;
				}
			SETFLAGBIT(ZF, false);
			*rmrw = result;
			}
		lflags.type = t_UNKNOWN;
		break;
		}
	CASE_0F_W(0xbd)												/* BSR Gw,Ew */
		{
		GetRMrw;
		Bit16u result, value;
		if (rm >= 0xc0)
			{
			GetEArw;
			value = *earw;
			} 
		else
			{
			GetEAa;
			value = Mem_Lodsw(eaa);
			}
		if (value == 0)
			SETFLAGBIT(ZF, true);
		else
			{
			result = 15;	// Operandsize-1
			while ((value & 0x8000) == 0)
				{
				result--;
				value <<= 1;
				}
			SETFLAGBIT(ZF, false);
			*rmrw = result;
			}
		lflags.type = t_UNKNOWN;
		break;
		}
	CASE_0F_W(0xbe)												/* MOVSX Gw,Eb */
		{
		GetRMrw;															
		if (rm >= 0xc0)
			{
			GetEArb;
			*rmrw = *(Bit8s *)earb;
			}
		else
			{
			GetEAa;
			*rmrw = LoadMbs(eaa);
			}
		break;
		}
	CASE_0F_B(0xc0)												/* XADD Gb,Eb */
	CASE_0F_W(0xc1)												/* XADD Gw,Ew */
	CASE_0F_W(0xc8)												/* BSWAP AX */
	CASE_0F_W(0xc9)												/* BSWAP CX */
	CASE_0F_W(0xca)												/* BSWAP DX */
	CASE_0F_W(0xcb)												/* BSWAP BX */
	CASE_0F_W(0xcc)												/* BSWAP SP */
	CASE_0F_W(0xcd)												/* BSWAP BP */
	CASE_0F_W(0xce)												/* BSWAP SI */
	CASE_0F_W(0xcf)												/* BSWAP DI */
		goto illegal_opcode;
		
