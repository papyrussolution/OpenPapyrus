	CASE_B(0x0f)																	// 2 byte opcodes
		core.opcode_index |= OPCODE_0F;
		goto restart_opcode;
	CASE_B(0x00)																	// ADD Eb,Gb
		RMEbGb(ADDB);
		break;
	CASE_W(0x01)																	// ADD Ew,Gw
		RMEwGw(ADDW);
		break;	
	CASE_B(0x02)																	// ADD Gb,Eb
		RMGbEb(ADDB);
		break;
	CASE_W(0x03)																	// ADD Gw,Ew
		RMGwEw(ADDW);
		break;
	CASE_B(0x04)																	// ADD AL,Ib
		ALIb(ADDB);
		break;
	CASE_W(0x05)																	// ADD AX,Iw
		AXIw(ADDW);
		break;
	CASE_W(0x06)																	// PUSH ES
		CPU_Push16(SegValue(es));
		break;
	CASE_W(0x07)																	// POP ES
		if (CPU_Pop16Seg(es))
			RUNEXCEPTION();
		break;
	CASE_B(0x08)																	// OR Eb,Gb
		RMEbGb(ORB);
		break;
	CASE_W(0x09)																	// OR Ew,Gw
		RMEwGw(ORW);
		break;
	CASE_B(0x0a)																	// OR Gb,Eb
		RMGbEb(ORB);
		break;
	CASE_W(0x0b)																	// OR Gw,Ew
		RMGwEw(ORW);
		break;
	CASE_B(0x0c)																	// OR AL,Ib
		ALIb(ORB);
		break;
	CASE_W(0x0d)																	// OR AX,Iw
		AXIw(ORW);
		break;
	CASE_W(0x0e)																	// PUSH CS
		CPU_Push16(SegValue(cs));
		break;
	CASE_B(0x10)																	// ADC Eb,Gb
		RMEbGb(ADCB);
		break;
	CASE_W(0x11)																	// ADC Ew,Gw
		RMEwGw(ADCW);
		break;	
	CASE_B(0x12)																	// ADC Gb,Eb
		RMGbEb(ADCB);
		break;
	CASE_W(0x13)																	// ADC Gw,Ew
		RMGwEw(ADCW);
		break;
	CASE_B(0x14)																	// ADC AL,Ib
		ALIb(ADCB);
		break;
	CASE_W(0x15)																	// ADC AX,Iw
		AXIw(ADCW);
		break;
	CASE_W(0x16)																	// PUSH SS
		CPU_Push16(SegValue(ss));
		break;
	CASE_W(0x17)																	// POP SS
		if (CPU_Pop16Seg(ss))
			RUNEXCEPTION();
		CPU_Cycles++;																// Always do another instruction
		break;
	CASE_B(0x18)																	// SBB Eb,Gb
		RMEbGb(SBBB);
		break;
	CASE_W(0x19)																	// SBB Ew,Gw
		RMEwGw(SBBW);
		break;
	CASE_B(0x1a)																	// SBB Gb,Eb
		RMGbEb(SBBB);
		break;
	CASE_W(0x1b)																	// SBB Gw,Ew
		RMGwEw(SBBW);
		break;
	CASE_B(0x1c)																	// SBB AL,Ib
		ALIb(SBBB);
		break;
	CASE_W(0x1d)																	// SBB AX,Iw
		AXIw(SBBW);
		break;
	CASE_W(0x1e)																	// PUSH DS
		CPU_Push16(SegValue(ds));
		break;
	CASE_W(0x1f)																	//* POP DS
		if (CPU_Pop16Seg(ds))
			RUNEXCEPTION();
		break;
	CASE_B(0x20)																	// AND Eb,Gb
		RMEbGb(ANDB);
		break;
	CASE_W(0x21)																	// AND Ew,Gw
		RMEwGw(ANDW);
		break;	
	CASE_B(0x22)																	// AND Gb,Eb
		RMGbEb(ANDB);
		break;
	CASE_W(0x23)																	// AND Gw,Ew
		RMGwEw(ANDW);
		break;
	CASE_B(0x24)																	// AND AL,Ib 
		ALIb(ANDB);
		break;
	CASE_W(0x25)																	// AND AX,Iw
		AXIw(ANDW);
		break;
	CASE_B(0x26)																	// SEG ES:
		DO_PREFIX_SEG(es);
		break;
	CASE_B(0x27)																	// DAA
		DAA();
		break;
	CASE_B(0x28)																	// SUB Eb,Gb
		RMEbGb(SUBB);
		break;
	CASE_W(0x29)																	// SUB Ew,Gw
		RMEwGw(SUBW);
		break;
	CASE_B(0x2a)																	// SUB Gb,Eb
		RMGbEb(SUBB);
		break;
	CASE_W(0x2b)																	// SUB Gw,Ew
		RMGwEw(SUBW);
		break;
	CASE_B(0x2c)																	// SUB AL,Ib
		ALIb(SUBB);
		break;
	CASE_W(0x2d)																	// SUB AX,Iw
		AXIw(SUBW);
		break;
	CASE_B(0x2e)																	// SEG CS:
		DO_PREFIX_SEG(cs);
		break;
	CASE_B(0x2f)																	// DAS
		DAS();
		break;  
	CASE_B(0x30)																	// XOR Eb,Gb
		RMEbGb(XORB);
		break;
	CASE_W(0x31)																	// XOR Ew,Gw
		RMEwGw(XORW);
		break;	
	CASE_B(0x32)																	// XOR Gb,Eb
		RMGbEb(XORB);
		break;
	CASE_W(0x33)																	// XOR Gw,Ew
		RMGwEw(XORW);
		break;
	CASE_B(0x34)																	// XOR AL,Ib
		ALIb(XORB);
		break;
	CASE_W(0x35)																	// XOR AX,Iw
		AXIw(XORW);
		break;
	CASE_B(0x36)																	// SEG SS:
		DO_PREFIX_SEG(ss);
		break;
	CASE_B(0x37)																	// AAA
		AAA();
		break;  
	CASE_B(0x38)																	// CMP Eb,Gb
		RMEbGb(CMPB);
		break;
	CASE_W(0x39)																	// CMP Ew,Gw
		RMEwGw(CMPW);
		break;
	CASE_B(0x3a)																	// CMP Gb,Eb
		RMGbEb(CMPB);
		break;
	CASE_W(0x3b)																	// CMP Gw,Ew
		RMGwEw(CMPW);
		break;
	CASE_B(0x3c)																	// CMP AL,Ib
		ALIb(CMPB);
		break;
	CASE_W(0x3d)																	// CMP AX,Iw
		AXIw(CMPW);
		break;
	CASE_B(0x3e)																	// SEG DS:
		DO_PREFIX_SEG(ds);
		break;
	CASE_B(0x3f)																	// AAS
		AAS();
		break;
	CASE_W(0x40)																	// INC AX
		INCW(reg_ax, LoadRw, SaveRw);
		break;
	CASE_W(0x41)																	// INC CX
		INCW(reg_cx, LoadRw, SaveRw);
		break;
	CASE_W(0x42)																	// INC DX
		INCW(reg_dx, LoadRw, SaveRw);
		break;
	CASE_W(0x43)																	// INC BX
		INCW(reg_bx, LoadRw, SaveRw);
		break;
	CASE_W(0x44)																	// INC SP
		INCW(reg_sp, LoadRw, SaveRw);
		break;
	CASE_W(0x45)																	// INC BP
		INCW(reg_bp, LoadRw, SaveRw);
		break;
	CASE_W(0x46)																	// INC SI
		INCW(reg_si, LoadRw, SaveRw);
		break;
	CASE_W(0x47)																	// INC DI
		INCW(reg_di, LoadRw, SaveRw);
		break;
	CASE_W(0x48)																	// DEC AX
		DECW(reg_ax, LoadRw, SaveRw);
		break;
	CASE_W(0x49)																	// DEC CX
  		DECW(reg_cx, LoadRw, SaveRw);
		break;
	CASE_W(0x4a)																	// DEC DX
		DECW(reg_dx, LoadRw, SaveRw);
		break;
	CASE_W(0x4b)																	// DEC BX
		DECW(reg_bx, LoadRw, SaveRw);
		break;
	CASE_W(0x4c)																	// DEC SP
		DECW(reg_sp, LoadRw, SaveRw);
		break;
	CASE_W(0x4d)																	// DEC BP
		DECW(reg_bp, LoadRw, SaveRw);
		break;
	CASE_W(0x4e)																	// DEC SI
		DECW(reg_si, LoadRw, SaveRw);
		break;
	CASE_W(0x4f)																	// DEC DI
		DECW(reg_di, LoadRw, SaveRw);
		break;
	CASE_W(0x50)																	// PUSH AX
		CPU_Push16(reg_ax);
		break;
	CASE_W(0x51)																	// PUSH CX
		CPU_Push16(reg_cx);
		break;
	CASE_W(0x52)																	// PUSH DX
		CPU_Push16(reg_dx);
		break;
	CASE_W(0x53)																	// PUSH BX
		CPU_Push16(reg_bx);
		break;
	CASE_W(0x54)																	// PUSH SP
		CPU_Push16(reg_sp);
		break;
	CASE_W(0x55)																	// PUSH BP
		CPU_Push16(reg_bp);
		break;
	CASE_W(0x56)																	// PUSH SI
		CPU_Push16(reg_si);
		break;
	CASE_W(0x57)																	// PUSH DI
		CPU_Push16(reg_di);
		break;
	CASE_W(0x58)																	// POP AX
		reg_ax = CPU_Pop16();
		break;
	CASE_W(0x59)																	// POP CX
		reg_cx = CPU_Pop16();
		break;
	CASE_W(0x5a)																	// POP DX
		reg_dx = CPU_Pop16();
		break;
	CASE_W(0x5b)																	// POP BX
		reg_bx = CPU_Pop16();
		break;
	CASE_W(0x5c)																	// POP SP
		reg_sp = CPU_Pop16();
		break;
	CASE_W(0x5d)																	// POP BP
		reg_bp = CPU_Pop16();
		break;
	CASE_W(0x5e)																	// POP SI
		reg_si = CPU_Pop16();
		break;
	CASE_W(0x5f)																	// POP DI
		reg_di = CPU_Pop16();
		break;
	CASE_W(0x60)																	// PUSHA
		{
		Bit16u old_sp = reg_sp;
		CPU_Push16(reg_ax);
		CPU_Push16(reg_cx);
		CPU_Push16(reg_dx);
		CPU_Push16(reg_bx);
		CPU_Push16(old_sp);
		CPU_Push16(reg_bp);
		CPU_Push16(reg_si);
		CPU_Push16(reg_di);
		}
		break;
	CASE_W(0x61)																	// POPA
		reg_di = CPU_Pop16();
		reg_si = CPU_Pop16();
		reg_bp = CPU_Pop16();
		reg_esp = (reg_esp&cpu.stack.notmask)|((reg_esp+2)&cpu.stack.mask);			// Don't pop SP
		reg_bx = CPU_Pop16();
		reg_dx = CPU_Pop16();
		reg_cx = CPU_Pop16();
		reg_ax = CPU_Pop16();
		break;
	CASE_W(0x62)																	// BOUND
		{
		GetRMrw;
		GetEAa;
		Bit16s bound_min = Mem_Lodsw(eaa);
		Bit16s bound_max = Mem_Lodsw(eaa+2);
		if ((((Bit16s)*rmrw) < bound_min) || (((Bit16s)*rmrw) > bound_max))
			EXCEPTION(5);
		}
		break;
	CASE_W(0x63)																	// ARPL Ew,Rw
		{
		if (!cpu.pmode)
			goto illegal_opcode;
		GetRMrw;
		if (rm >= 0xc0)
			{
			GetEArw;
			Bitu new_sel = *earw;
			CPU_ARPL(new_sel, *rmrw);
			*earw = (Bit16u)new_sel;
			}
		else
			{
			GetEAa;
			Bitu new_sel = Mem_Lodsw(eaa);
			CPU_ARPL(new_sel, *rmrw);
			Mem_Stosw(eaa, (Bit16u)new_sel);
			}
		}
		break;
	CASE_B(0x64)																	// SEG FS:
		DO_PREFIX_SEG(fs);
		break;
	CASE_B(0x65)																	// SEG GS:
		DO_PREFIX_SEG(gs);
		break;
	CASE_B(0x66)																	// Operand Size Prefix
		core.opcode_index = (cpu.code.big^0x1)*0x200;
		goto restart_opcode;
	CASE_B(0x67)																	// Address Size Prefix
		DO_PREFIX_ADDR();
	CASE_W(0x68)																	// PUSH Iw
		CPU_Push16(Fetchw());
		break;
	CASE_W(0x69)																	// IMUL Gw,Ew,Iw
		RMGwEwOp3(DIMULW, Fetchws());
		break;
	CASE_W(0x6a)																	// PUSH Ib
		CPU_Push16(Fetchbs());
		break;
	CASE_W(0x6b)																	// IMUL Gw,Ew,Ib
		RMGwEwOp3(DIMULW, Fetchbs());
		break;
	CASE_B(0x6c)																	// INSB
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 1))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_INSB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosb(BaseDI+di_index, IO_ReadB(reg_dx));
			di_index = (di_index+cpu.direction)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_W(0x6d)																	// INSW
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 2))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_INSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosw(BaseDI+di_index, IO_ReadW(reg_dx));
			di_index = (di_index+(cpu.direction<<1))&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_B(0x6e)																	// OUTSB
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 1))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_OUTSB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			IO_WriteB(reg_dx, Mem_Lodsb(BaseSI+si_index));
			si_index = (si_index+cpu.direction)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		break;
	CASE_W(0x6f)																	// OUTSW
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 2))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_OUTSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			IO_WriteW(reg_dx, Mem_Lodsw(BaseSI+si_index));
			si_index = (si_index+(cpu.direction<<1))&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		break;
	CASE_W(0x70)																	// JO
		JumpCond16_b(TFLG_O);
	CASE_W(0x71)																	// JNO
		JumpCond16_b(TFLG_NO);
	CASE_W(0x72)																	// JB
		JumpCond16_b(TFLG_B);
	CASE_W(0x73)																	// JNB
		JumpCond16_b(TFLG_NB);
	CASE_W(0x74)																	// JZ
  		JumpCond16_b(TFLG_Z);
	CASE_W(0x75)																	// JNZ
		JumpCond16_b(TFLG_NZ);
	CASE_W(0x76)																	// JBE
		JumpCond16_b(TFLG_BE);
	CASE_W(0x77)																	// JNBE
		JumpCond16_b(TFLG_NBE);
	CASE_W(0x78)																	// JS
		JumpCond16_b(TFLG_S);
	CASE_W(0x79)																	// JNS
		JumpCond16_b(TFLG_NS);
	CASE_W(0x7a)																	// JP
		JumpCond16_b(TFLG_P);
	CASE_W(0x7b)																	// JNP
		JumpCond16_b(TFLG_NP);
	CASE_W(0x7c)																	// JL
		JumpCond16_b(TFLG_L);
	CASE_W(0x7d)																	// JNL
		JumpCond16_b(TFLG_NL);
	CASE_W(0x7e)																	// JLE
		JumpCond16_b(TFLG_LE);
	CASE_W(0x7f)																	// JNLE
		JumpCond16_b(TFLG_NLE);
	CASE_B(0x80)																	// Grpl Eb,Ib
	CASE_B(0x82)																	// Grpl Eb,Ib Mirror instruction
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		if (rm >= 0xc0)
			{
			GetEArb;
			Bit8u ib = Fetchb();
			switch (which)
				{
				case 0x00: ADDB(*earb, ib, LoadRb, SaveRb); break;
				case 0x01:  ORB(*earb, ib, LoadRb, SaveRb); break;
				case 0x02: ADCB(*earb, ib, LoadRb, SaveRb); break;
				case 0x03: SBBB(*earb, ib, LoadRb, SaveRb); break;
				case 0x04: ANDB(*earb, ib, LoadRb, SaveRb); break;
				case 0x05: SUBB(*earb, ib, LoadRb, SaveRb); break;
				case 0x06: XORB(*earb, ib, LoadRb, SaveRb); break;
				case 0x07: CMPB(*earb, ib, LoadRb, SaveRb); break;
				}
			}
		else
			{
			GetEAa;
			Bit8u ib = Fetchb();
			switch (which)
				{
				case 0x00: ADDB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x01:  ORB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x02: ADCB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x03: SBBB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x04: ANDB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x05: SUBB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x06: XORB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				case 0x07: CMPB(eaa, ib, Mem_Lodsb, Mem_Stosb); break;
				}
			}
		break;
		}
	CASE_W(0x81)																	// Grpl Ew,Iw
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		if (rm >= 0xc0)
			{
			GetEArw;
			Bit16u iw = Fetchw();
			switch (which)
				{
				case 0x00: ADDW(*earw, iw, LoadRw, SaveRw); break;
				case 0x01:  ORW(*earw, iw, LoadRw, SaveRw); break;
				case 0x02: ADCW(*earw, iw, LoadRw, SaveRw); break;
				case 0x03: SBBW(*earw, iw, LoadRw, SaveRw); break;
				case 0x04: ANDW(*earw, iw, LoadRw, SaveRw); break;
				case 0x05: SUBW(*earw, iw, LoadRw, SaveRw); break;
				case 0x06: XORW(*earw, iw, LoadRw, SaveRw); break;
				case 0x07: CMPW(*earw, iw, LoadRw, SaveRw); break;
				}
			}
		else
			{
			GetEAa;
			Bit16u iw = Fetchw();
			switch (which)
				{
				case 0x00: ADDW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x01:  ORW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x02: ADCW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x03: SBBW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x04: ANDW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x05: SUBW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x06: XORW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x07: CMPW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				}
			}
		break;
		}
	CASE_W(0x83)																	// Grpl Ew,Ix
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		if (rm >= 0xc0)
			{
			GetEArw;
			Bit16u iw = (Bit16s)Fetchbs();
			switch (which)
				{
				case 0x00: ADDW(*earw, iw, LoadRw, SaveRw); break;
				case 0x01:  ORW(*earw, iw, LoadRw, SaveRw); break;
				case 0x02: ADCW(*earw, iw, LoadRw, SaveRw); break;
				case 0x03: SBBW(*earw, iw, LoadRw, SaveRw); break;
				case 0x04: ANDW(*earw, iw, LoadRw, SaveRw); break;
				case 0x05: SUBW(*earw, iw, LoadRw, SaveRw); break;
				case 0x06: XORW(*earw, iw, LoadRw, SaveRw); break;
				case 0x07: CMPW(*earw, iw, LoadRw, SaveRw); break;
				}
			}
		else
			{
			GetEAa;
			Bit16u iw = (Bit16s)Fetchbs();
			switch (which)
				{
				case 0x00: ADDW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x01:  ORW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x02: ADCW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x03: SBBW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x04: ANDW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x05: SUBW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x06: XORW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				case 0x07: CMPW(eaa, iw, Mem_Lodsw, Mem_Stosw); break;
				}
			}
		break;
		}
	CASE_B(0x84)																	// TEST Eb,Gb
		RMEbGb(TESTB);
		break;
	CASE_W(0x85)																	// TEST Ew,Gw
		RMEwGw(TESTW);
		break;
	CASE_B(0x86)																	// XCHG Eb,Gb
		{	
		GetRMrb;
		Bit8u oldrmrb = *rmrb;
		if (rm >= 0xc0)
			{
			GetEArb;
			*rmrb = *earb;
			*earb = oldrmrb;
			}
		else
			{
			GetEAa;
			*rmrb = Mem_Lodsb(eaa);
			Mem_Stosb(eaa, oldrmrb);
			}
		break;
		}
	CASE_W(0x87)																	// XCHG Ew,Gw
		{	
		GetRMrw;
		Bit16u oldrmrw = *rmrw;
		if (rm >= 0xc0)
			{
			GetEArw;
			*rmrw = *earw;
			*earw = oldrmrw;
			}
		else
			{
			GetEAa;
			*rmrw = Mem_Lodsw(eaa);
			Mem_Stosw(eaa, oldrmrw);
			}
		break;
		}
	CASE_B(0x88)																	// MOV Eb,Gb
		{	
		GetRMrb;
		if (rm >= 0xc0)
			{
			GetEArb;
			*earb = *rmrb;
			}
		else
			{
			if (cpu.pmode)
				{
				if ((rm == 0x05) && (!cpu.code.big))
					{
					Descriptor desc;
					cpu.gdt.GetDescriptor(SegValue(core.base_val_ds), desc);
					if ((desc.Type() == DESC_CODE_R_NC_A) || (desc.Type() == DESC_CODE_R_NC_NA))
						{
						CPU_Exception(EXCEPTION_GP, SegValue(core.base_val_ds) & 0xfffc);
						continue;
						}
					}
				}
			GetEAa;
			Mem_Stosb(eaa, *rmrb);
			}
			break;
		}
	CASE_W(0x89)																	// MOV Ew,Gw
		{	
		GetRMrw;
		if (rm >= 0xc0)
			{
			GetEArw;
			*earw = *rmrw;
			}
		else
			{
			GetEAa;
			Mem_Stosw(eaa, *rmrw);
			}
		break;
		}
	CASE_B(0x8a)																	// MOV Gb,Eb
		{	
		GetRMrb;
		if (rm < 0xc0)
			{
			GetEAa;
			*rmrb = Mem_Lodsb(eaa);
			}
		else
			{
			GetEArb;
			*rmrb = *earb;
			}
		break;
		}
	CASE_W(0x8b)																	// MOV Gw,Ew
		{	
		GetRMrw;
		if (rm < 0xc0)
			{
			GetEAa;
			*rmrw = Mem_Lodsw(eaa);
			}
		else
			{
			GetEArw;
			*rmrw = *earw;
			}
		break;
		}
	CASE_W(0x8c)																	// Mov Ew,Sw
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		if (which > 5)
			goto illegal_opcode;
		Bit16u val = Segs.val[which];
		if (rm >= 0xc0)
			{
			GetEArw;
			*earw = val;
			}
		else
			{
			GetEAa;
			Mem_Stosw(eaa, val);
			}
		break;
		}
	CASE_W(0x8d)																	// LEA Gw
		{
		BaseDS = BaseSS = 0;														// Little hack to always use segprefixed version
		GetRMrw;
		*rmrw = (Bit16u)(*EATable[TEST_PREFIX_ADDR*256+rm])();
		break;
		}
	CASE_B(0x8e)																	// MOV Sw,Ew
		{
		GetRM;
		Bit16u val;
		Bitu which = (rm>>3)&7;
		if (rm >= 0xc0)
			{
			GetEArw;
			val = *earw;
			}
		else
			{
			GetEAa;
			val = Mem_Lodsw(eaa);
			}
		switch (which)
			{
		case 0x02:																	// MOV SS,Ew
			CPU_Cycles++;															// Always do another instruction
		case 0x00:																	// MOV ES,Ew
		case 0x03:																	// MOV DS,Ew
		case 0x05:																	// MOV GS,Ew
		case 0x04:																	// MOV FS,Ew
			if (CPU_SetSegGeneral((SegNames)which, val))
				RUNEXCEPTION();
			break;
		default:
			goto illegal_opcode;
			}
		break;
		}							
	CASE_W(0x8f)																	// POP Ew
		{
		Bit16u val = CPU_Pop16();
		GetRM;
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosw(eaa, val);
			}
		else
			{
			GetEArw;
			*earw = val;
			}
		break;
		}
	CASE_B(0x90)																	// NOP
		break;
	CASE_W(0x91)																	// XCHG CX,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_cx;
		reg_cx = temp;
		}
		break;
	CASE_W(0x92)																	// XCHG DX,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_dx;
		reg_dx = temp;
		}
		break;
	CASE_W(0x93)																	// XCHG BX,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_bx;
		reg_bx = temp;
		}
		break;
	CASE_W(0x94)																	// XCHG SP,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_sp;
		reg_sp = temp;
		}
		break;
	CASE_W(0x95)																	// XCHG BP,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_bp;
		reg_bp = temp;
		}
		break;
	CASE_W(0x96)																	// XCHG SI,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_si;
		reg_si = temp;
		}
		break;
	CASE_W(0x97)																	// XCHG DI,AX
		{
		Bit16u temp = reg_ax;
		reg_ax = reg_di;
		reg_di = temp;
		}
		break;
	CASE_W(0x98)																	// CBW
		reg_ax = (Bit8s)reg_al;
		break;
	CASE_W(0x99)																	// CWD
		if (!(reg_ax&0x8000))
			reg_dx = 0;
		else
			reg_dx = 0xffff;
		break;
	CASE_W(0x9a)																	// CALL Ap
		{
		Bit16u newip = Fetchw();
		Bit16u newcs = Fetchw();
		if (!cpu.pmode)
			{
			CPU_Push16(SegValue(cs));
			CPU_Push16(GETIP);
			reg_eip = newip&0xffff;
			SegSet16(cs, newcs);
			cpu.code.big = false;
			if (!GETFLAG(TF))
				continue;
			}
		else
			{
			CPU_CALL(false, newcs, newip, GETIP);
			if (!GETFLAG(TF))
				continue;
			}
		FillFlags();
		cpudecoder = CPU_Core_Normal_Trap_Run;
		return CBRET_NONE;
		}
	CASE_B(0x9b)																	// WAIT
		break;																		// No waiting here
	CASE_W(0x9c)																	// PUSHF
		FillFlags();
		CPU_Push16(reg_flags);
		break;
	CASE_W(0x9d)																	// POPF
		CPU_POPF(false);
		if (!GETFLAG(TF))
			break;
		cpudecoder = CPU_Core_Normal_Trap_Run;
		goto decode_end;
	CASE_B(0x9e)																	// SAHF
		SETFLAGSb(reg_ah);
		break;
	CASE_B(0x9f)																	// LAHF
		FillFlags();
		reg_ah = reg_flags&0xff;
		if (reg_ah == 2 && reg_eip == 0x47c8)										// 4DOS cheking for bit 1 change
			reg_ah = 0;
		break;
	CASE_B(0xa0)																	// MOV AL,Ob
		{
		GetEADirect;
		reg_al = Mem_Lodsb(eaa);
		}
		break;
	CASE_W(0xa1)																	// MOV AX,Ow
		{
		GetEADirect;
		reg_ax = Mem_Lodsw(eaa);
		}
		break;
	CASE_B(0xa2)																	// MOV Ob,AL
		{
		GetEADirect;
		Mem_Stosb(eaa, reg_al);
		}
		break;
	CASE_W(0xa3)																	// MOV Ow,AX
		{
		GetEADirect;
		Mem_Stosw(eaa, reg_ax);
		}
		break;
	CASE_B(0xa4)																	// MOVSB
		if (TEST_PREFIX_REP)
			DoString(R_MOVSB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosb(BaseDI+di_index, Mem_Lodsb(BaseSI+si_index));
			Bits add_index = cpu.direction;
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_W(0xa5)																	// MOVSW
		if (TEST_PREFIX_REP)
			DoString(R_MOVSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosw(BaseDI+di_index, Mem_Lodsw(BaseSI+si_index));
			Bits add_index = cpu.direction<<1;
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_B(0xa6)																	// CMPSB
		if (TEST_PREFIX_REP)
			DoString(R_CMPSB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Bit8u val1 = Mem_Lodsb(BaseSI+si_index);
			Bit8u val2 = Mem_Lodsb(BaseDI+di_index);
			Bits add_index = cpu.direction;
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPB(val1, val2, LoadD, 0);
			}
		break;
	CASE_W(0xa7)																	// CMPSW
		if (TEST_PREFIX_REP)
			DoString(R_CMPSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Bit16u val1 = Mem_Lodsw(BaseSI+si_index);
			Bit16u val2 = Mem_Lodsw(BaseDI+di_index);
			Bits add_index = cpu.direction<<1;
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPW(val1, val2, LoadD, 0);
			}
		break;
	CASE_B(0xa8)																	// TEST AL,Ib
		ALIb(TESTB);
		break;
	CASE_W(0xa9)																	// TEST AX,Iw
		AXIw(TESTW);
		break;
	CASE_B(0xaa)																	// STOSB
		if (TEST_PREFIX_REP)
			DoString(R_STOSB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosb(BaseDI+di_index, reg_al);
			Bits add_index = cpu.direction;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_W(0xab)																	// STOSW
		if (TEST_PREFIX_REP)
			DoString(R_STOSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosw(BaseDI+di_index, reg_ax);
			Bits add_index = cpu.direction<<1;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_B(0xac)																	// LODSB
		if (!TEST_PREFIX_REP)
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			reg_al = Mem_Lodsb(BaseSI+si_index);
			Bits add_index = cpu.direction;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		else
			DoString(R_LODSB);
		break;
	CASE_W(0xad)																	// LODSW
		if (TEST_PREFIX_REP)
			DoString(R_LODSW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			reg_ax = Mem_Lodsw(BaseSI+si_index);
			Bits add_index = cpu.direction<<1;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		break;
	CASE_B(0xae)																	// SCASB
		if (TEST_PREFIX_REP)
			DoString(R_SCASB);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Bit8u val2 = Mem_Lodsb(BaseDI+di_index);
			Bits add_index = cpu.direction;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPB(reg_al, val2, LoadD, 0);
			}
		break;
	CASE_W(0xaf)																	// SCASW
		if (TEST_PREFIX_REP)
			DoString(R_SCASW);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Bit16u val2 = Mem_Lodsw(BaseDI+di_index);
			Bits add_index = cpu.direction<<1;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPW(reg_ax, val2, LoadD, 0);
			}
		break;
	CASE_B(0xb0)																	// MOV AL,Ib
		reg_al = Fetchb();
		break;
	CASE_B(0xb1)																	// MOV CL,Ib
		reg_cl = Fetchb();
		break;
	CASE_B(0xb2)																	// MOV DL,Ib
		reg_dl = Fetchb();
		break;
	CASE_B(0xb3)																	// MOV BL,Ib
		reg_bl = Fetchb();
		break;
	CASE_B(0xb4)																	// MOV AH,Ib
		reg_ah = Fetchb();
		break;
	CASE_B(0xb5)																	// MOV CH,Ib
		reg_ch = Fetchb();
		break;
	CASE_B(0xb6)																	// MOV DH,Ib
		reg_dh = Fetchb();
		break;
	CASE_B(0xb7)																	// MOV BH,Ib
		reg_bh = Fetchb();
		break;
	CASE_W(0xb8)																	// MOV AX,Iw
		reg_ax = Fetchw();
		break;
	CASE_W(0xb9)																	// MOV CX,Iw
		reg_cx = Fetchw();
		break;
	CASE_W(0xba)																	// MOV DX,Iw
		reg_dx = Fetchw();
		break;
	CASE_W(0xbb)																	// MOV BX,Iw
		reg_bx = Fetchw();
		break;
	CASE_W(0xbc)																	// MOV SP,Iw
		reg_sp = Fetchw();
		break;
	CASE_W(0xbd)																	// MOV BP.Iw
		reg_bp = Fetchw();
		break;
	CASE_W(0xbe)																	// MOV SI,Iw
		reg_si = Fetchw();
		break;
	CASE_W(0xbf)																	// MOV DI,Iw
		reg_di = Fetchw();
		break;
	CASE_B(0xc0)																	// GRP2 Eb,Ib
		GRP2B(Fetchb());
		break;
	CASE_W(0xc1)																	// GRP2 Ew,Ib
		GRP2W(Fetchb());
		break;
	CASE_W(0xc2)																	// RETN Iw
		reg_eip = CPU_Pop16();
		reg_esp += Fetchw();
		continue;
	CASE_W(0xc3)																	// RETN
		reg_eip = CPU_Pop16();
		continue;
	CASE_W(0xc4)																	// LES
		{	
		GetRMrw;
		if (rm >= 0xc0)
			goto illegal_opcode;
		GetEAa;
		if (CPU_SetSegGeneral(es, Mem_Lodsw(eaa+2)))
			RUNEXCEPTION();
		*rmrw = Mem_Lodsw(eaa);
		break;
		}
	CASE_W(0xc5)																	// LDS
		{	
		GetRMrw;
		if (rm >= 0xc0)
			goto illegal_opcode;
		GetEAa;
		if (CPU_SetSegGeneral(ds, Mem_Lodsw(eaa+2)))
			RUNEXCEPTION();
		*rmrw = Mem_Lodsw(eaa);
		break;
		}
	CASE_B(0xc6)																	// MOV Eb,Ib
		{
		GetRM;
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosb(eaa, Fetchb());
			}
		else
			{
			GetEArb;
			*earb = Fetchb();
			}
		break;

		}
	CASE_W(0xc7)																	// MOV EW,Iw
		{
		GetRM;
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosw(eaa, Fetchw());
			}
		else
			{
			GetEArw;
			*earw = Fetchw();
			}
		break;
		}
	CASE_W(0xc8)																	// ENTER Iw,Ib
		{
		Bitu bytes = Fetchw();
		Bitu level = Fetchb();
		CPU_ENTER(false, bytes, level);
		}
		break;
	CASE_W(0xc9)																	// LEAVE
		reg_esp &= cpu.stack.notmask;
		reg_esp |= (reg_ebp&cpu.stack.mask);
		reg_bp = CPU_Pop16();
		break;
	CASE_W(0xca)																	// RETF Iw
		{
		Bitu bytes = Fetchw();
//		FillFlags();
	    if (!cpu.pmode)
			{
			Bitu new_ip, new_cs;
			new_ip = CPU_Pop16();
			new_cs = CPU_Pop16();
			reg_esp += bytes;
			SegSet16(cs, new_cs);
			reg_eip = new_ip;
			cpu.code.big = false;
			continue;
			}
		CPU_RET(false, bytes);
		continue;
		}
	CASE_W(0xcb)																	// RETF
//		FillFlags();
	    if (!cpu.pmode)
			{
			Bitu new_ip, new_cs;
			new_ip = CPU_Pop16();
			new_cs = CPU_Pop16();
			SegSet16(cs, new_cs);
			reg_eip = new_ip;
			cpu.code.big = false;
			continue;
			}
		CPU_RET(false, 0);
		continue;
	CASE_B(0xcc)																	// INT3
		CPU_SW_Interrupt_NoIOPLCheck(3, GETIP);
		cpu.trap_skip = true;
		continue;
	CASE_B(0xcd)																	// INT Ib
		{
		Bitu intNo = Fetchb();
		CPU_SW_Interrupt(intNo, GETIP);												// NB: DON"T use Fetchb() in this call, it messes up GETIP in some configurations
		cpu.trap_skip = true;
		continue;
		}
	CASE_B(0xce)																	// INTO
		if (get_OF())
			{
			CPU_SW_Interrupt(4, GETIP);
			cpu.trap_skip = true;
			continue;
			}
		break;
	CASE_W(0xcf)																	// IRET
		{
		CPU_IRET(false);
		if (!GETFLAG(TF))
			continue;
		cpudecoder = CPU_Core_Normal_Trap_Run;
		return CBRET_NONE;
		}
	CASE_B(0xd0)																	// GRP2 Eb,1
		GRP2B(1);
		break;
	CASE_W(0xd1)																	// GRP2 Ew,1
		GRP2W(1);
		break;
	CASE_B(0xd2)																	// GRP2 Eb,CL
		GRP2B(reg_cl);
		break;
	CASE_W(0xd3)																	// GRP2 Ew,CL
		GRP2W(reg_cl);
		break;
	CASE_B(0xd4)																	// AAM Ib
		AAM(Fetchb());
		break;
	CASE_B(0xd5)																	// AAD Ib
		AAD(Fetchb());
		break;
	CASE_B(0xd6)																	// SALC
		reg_al = get_CF() ? 0xFF : 0;
		break;
	CASE_B(0xd7)																	// XLAT
		if (TEST_PREFIX_ADDR)
			reg_al = Mem_Lodsb(BaseDS+(Bit32u)(reg_ebx+reg_al));
		else
			reg_al = Mem_Lodsb(BaseDS+(Bit16u)(reg_bx+reg_al));
		break;
	CASE_B(0xd8)																	// FPU ESC 0
	CASE_B(0xd9)																	// FPU ESC 1
	CASE_B(0xda)																	// FPU ESC 2
	CASE_B(0xdb)																	// FPU ESC 3
	CASE_B(0xdc)																	// FPU ESC 4
	CASE_B(0xdd)																	// FPU ESC 5
	CASE_B(0xde)																	// FPU ESC 6
	CASE_B(0xdf)																	// FPU ESC 7
		{
		Bit8u rm = Fetchb();
		if (rm < 0xc0)
			GetEAa;
		}
		break;
	CASE_W(0xe0)																	// LOOPNZ
		if (!TEST_PREFIX_ADDR)
			JumpCond16_b(--reg_cx && !get_ZF());
		JumpCond16_b(--reg_ecx && !get_ZF());
	CASE_W(0xe1)																	// LOOPZ
		if (!TEST_PREFIX_ADDR)
			JumpCond16_b(--reg_cx && get_ZF());
		JumpCond16_b(--reg_ecx && get_ZF());
	CASE_W(0xe2)																	// LOOP
		if (!TEST_PREFIX_ADDR)
			JumpCond16_b(--reg_cx);
		JumpCond16_b(--reg_ecx);
	CASE_W(0xe3)																	// JCXZ
		JumpCond16_b(!(reg_ecx&AddrMaskTable[core.prefixes&PREFIX_ADDR]));
	CASE_B(0xe4)																	// IN AL,Ib
		{	
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 1))
			RUNEXCEPTION();
		reg_al = IO_ReadB(port);
		break;
		}
	CASE_W(0xe5)																	// IN AX,Ib
		{	
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 2))
			RUNEXCEPTION();
		reg_ax = IO_ReadW(port);
		break;
		}
	CASE_B(0xe6)																	// OUT Ib,AL
		{
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 1))
			RUNEXCEPTION();
		IO_WriteB(port, reg_al);
		break;
		}		
	CASE_W(0xe7)																	// OUT Ib,AX
		{
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 2))
			RUNEXCEPTION();
		IO_WriteW(port, reg_ax);
		break;
		}
	CASE_W(0xe8)																	// CALL Jw
		{ 
		Bit16u addip = Fetchws();
		SAVEIP;
		CPU_Push16(reg_eip);
		reg_eip = (Bit16u)(reg_eip+addip);
		continue;
		}
	CASE_W(0xe9)																	// JMP Jw
		{ 
		Bit16u addip = Fetchws();
		SAVEIP;
		reg_eip = (Bit16u)(reg_eip+addip);
		continue;
		}
	CASE_W(0xea)																	// JMP Ap
		{
		Bit16u newip = Fetchw();
		Bit16u newcs = Fetchw();
		if (!cpu.pmode)
			{
			reg_eip = newip;
			SegSet16(cs, newcs);
			cpu.code.big = false;
			if (!GETFLAG(TF))
				continue;
			}
		else
			{
			CPU_JMP(false, newcs, newip);
			if (!GETFLAG(TF))
				continue;
			}
		FillFlags();
		cpudecoder = CPU_Core_Normal_Trap_Run;
		return CBRET_NONE;
		}
	CASE_W(0xeb)																	// JMP Jb
		{ 
		Bit16s addip = Fetchbs();
		SAVEIP;
		reg_eip = (Bit16u)(reg_eip+addip);
		continue;
		}
	CASE_B(0xec)																	// IN AL,DX
		if (cpu.pmode && CPU_IO_Exception(reg_dx, 1))
			RUNEXCEPTION();
		reg_al = IO_ReadB(reg_dx);
		break;
	CASE_W(0xed)																	// IN AX,DX
		if (cpu.pmode && CPU_IO_Exception(reg_dx, 2))
			RUNEXCEPTION();
		reg_ax = IO_ReadW(reg_dx);
		break;
	CASE_B(0xee)																	// OUT DX,AL
		if (cpu.pmode && CPU_IO_Exception(reg_dx, 1))
			RUNEXCEPTION();
		IO_WriteB(reg_dx, reg_al);
		break;
	CASE_W(0xef)																	// OUT DX,AX
		if (cpu.pmode && CPU_IO_Exception(reg_dx, 2))
			RUNEXCEPTION();
		IO_WriteW(reg_dx, reg_ax);
		break;
	CASE_B(0xf0)																	// LOCK
		break;
	CASE_B(0xf1)																	// ICEBP
		CPU_SW_Interrupt_NoIOPLCheck(1, GETIP);
		cpu.trap_skip = true;
		continue;
	CASE_B(0xf2)																	// REPNZ
		DO_PREFIX_REP(false);	
	CASE_B(0xf3)																	// REPZ
		DO_PREFIX_REP(true);	
	CASE_B(0xf4)																	// HLT
		if (cpu.pmode && cpu.cpl)
			EXCEPTION(EXCEPTION_GP);
		FillFlags();
		CPU_HLT(GETIP);
		return CBRET_NONE;															// Needs to return for hlt cpu core
	CASE_B(0xf5)																	// CMC
		FillFlags();
		SETFLAGBIT(CF, !(reg_flags&FLAG_CF));
		break;
	CASE_B(0xf6)																	// GRP3 Eb(,Ib)
		{	
		GetRM;
		Bitu which = (rm>>3)&7;
		switch (which)
			{
		case 0x00:																	// TEST Eb,Ib
		case 0x01:																	// TEST Eb,Ib Undocumented
			{
			if (rm < 0xc0)
				{
				GetEAa;
				TESTB(eaa, Fetchb(), Mem_Lodsb, 0);
				}
			else
				{
				GetEArb;
				TESTB(*earb, Fetchb(), LoadRb, 0)
				}
			break;
			}
		case 0x02:																	// NOT Eb
			{
			if (rm >= 0xc0)
				{
				GetEArb;
				*earb = ~*earb;
				}
			else
				{
				GetEAa;
				Mem_Stosb(eaa, ~Mem_Lodsb(eaa));
				}
			break;
			}
		case 0x03:																	// NEG Eb
			{
			lflags.type = t_NEGb;
			if (rm >= 0xc0)
				{
				GetEArb;
				lf_var1b = *earb;
				lf_resb = 0-lf_var1b;
				*earb = lf_resb;
				}
			else
				{
				GetEAa;
				lf_var1b = Mem_Lodsb(eaa);
				lf_resb = 0-lf_var1b;
 				Mem_Stosb(eaa, lf_resb);
				}
			break;
			}
		case 0x04:																	// MUL AL,Eb
			RMEb(MULB);
			break;
		case 0x05:																	// IMUL AL,Eb
			RMEb(IMULB);
			break;
		case 0x06:																	// DIV Eb
			RMEb(DIVB);
			break;
		case 0x07:																	// IDIV Eb
			RMEb(IDIVB);
			break;
			}
		break;
		}
	CASE_W(0xf7)																	// GRP3 Ew(,Iw)
		{ 
		GetRM;
		Bitu which = (rm>>3)&7;
		switch (which)
			{
		case 0x00:																	// TEST Ew,Iw
		case 0x01:																	// TEST Ew,Iw Undocumented
			{
			if (rm < 0xc0)
				{
				GetEAa;
				TESTW(eaa, Fetchw(), Mem_Lodsw, Mem_Stosw);
				}
			else
				{
				GetEArw;
				TESTW(*earw, Fetchw(), LoadRw, SaveRw);
				}
			break;
			}
		case 0x02:																	// NOT Ew
			{
			if (rm >= 0xc0)
				{
				GetEArw;
				*earw = ~*earw;
				}
			else
				{
				GetEAa;
				Mem_Stosw(eaa, ~Mem_Lodsw(eaa));
				}
			break;
			}
		case 0x03:																	// NEG Ew
			{
			lflags.type = t_NEGw;
			if (rm >= 0xc0)
				{
				GetEArw;
				lf_var1w = *earw;
				lf_resw = 0-lf_var1w;
				*earw = lf_resw;
				}
			else
				{
				GetEAa;
				lf_var1w = Mem_Lodsw(eaa);
				lf_resw = 0-lf_var1w;
 				Mem_Stosw(eaa, lf_resw);
				}
			break;
			}
		case 0x04:																	// MUL AX,Ew
			RMEw(MULW);
			break;
		case 0x05:																	// IMUL AX,Ew
			RMEw(IMULW)
			break;
		case 0x06:																	// DIV Ew
			RMEw(DIVW)
			break;
		case 0x07:																	// IDIV Ew
			RMEw(IDIVW)
			break;
			}
		break;
		}
	CASE_B(0xf8)																	// CLC
		FillFlags();
		SETFLAGBIT(CF, false);
		break;
	CASE_B(0xf9)																	// STC
		FillFlags();
		SETFLAGBIT(CF, true);
		break;
	CASE_B(0xfa)																	// CLI
		if (!cpu.pmode)
			{
			SETFLAGBIT(IF, false);
			break;
			}
		if (CPU_pCLI())
			RUNEXCEPTION();
		break;
	CASE_B(0xfb)																	// STI
		if (!cpu.pmode)
			{
			SETFLAGBIT(IF, true);
			break;
			}
		if (CPU_pSTI())
			RUNEXCEPTION();
		break;
	CASE_B(0xfc)																	// CLD
		SETFLAGBIT(DF, false);
		cpu.direction = 1;
		break;
	CASE_B(0xfd)																	// STD
		SETFLAGBIT(DF, true);
		cpu.direction = -1;
		break;
	CASE_B(0xfe)																	// GRP4 Eb
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		switch (which)
			{
		case 0x00:																	// INC Eb
			RMEb(INCB);
			break;		
		case 0x01:																	// DEC Eb
			RMEb(DECB);
			break;
		case 0x07:																	// CallBack
			{
			FillFlags();
			Bitu cb = Fetchw();
			SAVEIP;
			return cb;
			}
		default:
			E_Exit("Illegal GRP4 Call %d", which);
			break;
			}
		break;
		}
	CASE_W(0xff)																	// GRP5 Ew
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		switch (which)
			{
		case 0x00:																	// INC Ew
			RMEw(INCW);
			break;		
		case 0x01:																	// DEC Ew
			RMEw(DECW);
			break;		
		case 0x02:																	// CALL Ev
			if (rm >= 0xc0)
				{
				GetEArw;
				reg_eip = *earw;
				}
			else
				{
				GetEAa;
				reg_eip = Mem_Lodsw(eaa);
				}
			CPU_Push16(GETIP);
			continue;
		case 0x03:																	// CALL Ep
			{
			if (rm < 0xc0)
				{
				GetEAa;
				Bit16u newip = Mem_Lodsw(eaa);
				Bit16u newcs = Mem_Lodsw(eaa+2);
				if (!cpu.pmode)
					{
					CPU_Push16(SegValue(cs));
					CPU_Push16(GETIP);
					reg_eip = newip&0xffff;
					SegSet16(cs, newcs);
					cpu.code.big = false;
					if (!GETFLAG(TF))
						continue;
					}
				else
					{
					CPU_CALL(false, newcs, newip, GETIP);
					if (!GETFLAG(TF))
						continue;
					}
				FillFlags();
				cpudecoder = CPU_Core_Normal_Trap_Run;
				return CBRET_NONE;
				}
			goto illegal_opcode;
			}
		case 0x04:																	// JMP Ev
			if (rm < 0xc0)
				{
				GetEAa;
				reg_eip = Mem_Lodsw(eaa);
				}
			else
				{
				GetEArw;
				reg_eip = *earw;
				}
			continue;
		case 0x05:																	// JMP Ep
			{
			if (rm < 0xc0)
				{
				GetEAa;
				Bit16u newip = Mem_Lodsw(eaa);
				Bit16u newcs = Mem_Lodsw(eaa+2);
				if (!cpu.pmode)
					{
					reg_eip = newip&0xffff;
					SegSet16(cs, newcs);
					cpu.code.big = false;
					if (!GETFLAG(TF))
						continue;
					}
				else
					{
					CPU_JMP(false, newcs, newip);
					if (!GETFLAG(TF))
						continue;
					}
				FillFlags();
				cpudecoder = CPU_Core_Normal_Trap_Run;
				return CBRET_NONE;
				}
			goto illegal_opcode;
			}
		case 0x06:																	// PUSH Ev
			if (rm < 0xc0)
				{
				GetEAa;
				CPU_Push16(Mem_Lodsw(eaa));
				}
			else
				{
				GetEArw;
				CPU_Push16(*earw);
				}
			break;
		default:
//			LOG(LOG_CPU,LOG_ERROR)("CPU:GRP5:Illegal Call %2X", which);
//			goto illegal_opcode;
			CPU_Exception(6, 0);
			}
		break;
		}
			



