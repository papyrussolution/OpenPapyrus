	CASE_D(0x01)												/* ADD Ed,Gd */
		RMEdGd(ADDD);
		break;	
	CASE_D(0x03)												/* ADD Gd,Ed */
		RMGdEd(ADDD);
		break;
	CASE_D(0x05)												/* ADD EAX,Id */
		EAXId(ADDD);
		break;
	CASE_D(0x06)												/* PUSH ES */		
		CPU_Push32(SegValue(es));
		break;
	CASE_D(0x07)												/* POP ES */
		if (CPU_Pop32Seg(es))
			RUNEXCEPTION();
		break;
	CASE_D(0x09)												/* OR Ed,Gd */
		RMEdGd(ORD);break;
	CASE_D(0x0b)												/* OR Gd,Ed */
		RMGdEd(ORD);break;
	CASE_D(0x0d)												/* OR EAX,Id */
		EAXId(ORD);break;
	CASE_D(0x0e)												/* PUSH CS */		
		CPU_Push32(SegValue(cs));break;
	CASE_D(0x11)												/* ADC Ed,Gd */
		RMEdGd(ADCD);break;	
	CASE_D(0x13)												/* ADC Gd,Ed */
		RMGdEd(ADCD);break;
	CASE_D(0x15)												/* ADC EAX,Id */
		EAXId(ADCD);break;
	CASE_D(0x16)												/* PUSH SS */
		CPU_Push32(SegValue(ss));break;
	CASE_D(0x17)												/* POP SS */
		if (CPU_Pop32Seg(ss))
			RUNEXCEPTION();
		CPU_Cycles++;
		break;
	CASE_D(0x19)												/* SBB Ed,Gd */
		RMEdGd(SBBD);break;
	CASE_D(0x1b)												/* SBB Gd,Ed */
		RMGdEd(SBBD);break;
	CASE_D(0x1d)												/* SBB EAX,Id */
		EAXId(SBBD);break;
	CASE_D(0x1e)												/* PUSH DS */		
		CPU_Push32(SegValue(ds));
		break;
	CASE_D(0x1f)												/* POP DS */
		if (CPU_Pop32Seg(ds))
			RUNEXCEPTION();
		break;
	CASE_D(0x21)												/* AND Ed,Gd */
		RMEdGd(ANDD);break;	
	CASE_D(0x23)												/* AND Gd,Ed */
		RMGdEd(ANDD);break;
	CASE_D(0x25)												/* AND EAX,Id */
		EAXId(ANDD);break;
	CASE_D(0x29)												/* SUB Ed,Gd */
		RMEdGd(SUBD);break;
	CASE_D(0x2b)												/* SUB Gd,Ed */
		RMGdEd(SUBD);break;
	CASE_D(0x2d)												/* SUB EAX,Id */
		EAXId(SUBD);break;
	CASE_D(0x31)												/* XOR Ed,Gd */
		RMEdGd(XORD);break;	
	CASE_D(0x33)												/* XOR Gd,Ed */
		RMGdEd(XORD);break;
	CASE_D(0x35)												/* XOR EAX,Id */
		EAXId(XORD);break;
	CASE_D(0x39)												/* CMP Ed,Gd */
		RMEdGd(CMPD);break;
	CASE_D(0x3b)												/* CMP Gd,Ed */
		RMGdEd(CMPD);break;
	CASE_D(0x3d)												/* CMP EAX,Id */
		EAXId(CMPD);break;
	CASE_D(0x40)												/* INC EAX */
		INCD(reg_eax,LoadRd,SaveRd);break;
	CASE_D(0x41)												/* INC ECX */
		INCD(reg_ecx,LoadRd,SaveRd);break;
	CASE_D(0x42)												/* INC EDX */
		INCD(reg_edx,LoadRd,SaveRd);break;
	CASE_D(0x43)												/* INC EBX */
		INCD(reg_ebx,LoadRd,SaveRd);break;
	CASE_D(0x44)												/* INC ESP */
		INCD(reg_esp,LoadRd,SaveRd);break;
	CASE_D(0x45)												/* INC EBP */
		INCD(reg_ebp,LoadRd,SaveRd);break;
	CASE_D(0x46)												/* INC ESI */
		INCD(reg_esi,LoadRd,SaveRd);break;
	CASE_D(0x47)												/* INC EDI */
		INCD(reg_edi,LoadRd,SaveRd);break;
	CASE_D(0x48)												/* DEC EAX */
		DECD(reg_eax,LoadRd,SaveRd);break;
	CASE_D(0x49)												/* DEC ECX */
		DECD(reg_ecx,LoadRd,SaveRd);break;
	CASE_D(0x4a)												/* DEC EDX */
		DECD(reg_edx,LoadRd,SaveRd);break;
	CASE_D(0x4b)												/* DEC EBX */
		DECD(reg_ebx,LoadRd,SaveRd);break;
	CASE_D(0x4c)												/* DEC ESP */
		DECD(reg_esp,LoadRd,SaveRd);break;
	CASE_D(0x4d)												/* DEC EBP */
		DECD(reg_ebp,LoadRd,SaveRd);break;
	CASE_D(0x4e)												/* DEC ESI */
		DECD(reg_esi,LoadRd,SaveRd);break;
	CASE_D(0x4f)												/* DEC EDI */
		DECD(reg_edi,LoadRd,SaveRd);break;
	CASE_D(0x50)												/* PUSH EAX */
		CPU_Push32(reg_eax);
		break;
	CASE_D(0x51)												/* PUSH ECX */
		CPU_Push32(reg_ecx);break;
	CASE_D(0x52)												/* PUSH EDX */
		CPU_Push32(reg_edx);break;
	CASE_D(0x53)												/* PUSH EBX */
		CPU_Push32(reg_ebx);break;
	CASE_D(0x54)												/* PUSH ESP */
		CPU_Push32(reg_esp);break;
	CASE_D(0x55)												/* PUSH EBP */
		CPU_Push32(reg_ebp);break;
	CASE_D(0x56)												/* PUSH ESI */
		CPU_Push32(reg_esi);break;
	CASE_D(0x57)												/* PUSH EDI */
		CPU_Push32(reg_edi);break;
	CASE_D(0x58)												/* POP EAX */
		reg_eax = CPU_Pop32();
		break;
	CASE_D(0x59)												/* POP ECX */
		reg_ecx=CPU_Pop32();break;
	CASE_D(0x5a)												/* POP EDX */
		reg_edx=CPU_Pop32();break;
	CASE_D(0x5b)												/* POP EBX */
		reg_ebx=CPU_Pop32();break;
	CASE_D(0x5c)												/* POP ESP */
		reg_esp=CPU_Pop32();break;
	CASE_D(0x5d)												/* POP EBP */
		reg_ebp=CPU_Pop32();break;
	CASE_D(0x5e)												/* POP ESI */
		reg_esi=CPU_Pop32();break;
	CASE_D(0x5f)												/* POP EDI */
		reg_edi=CPU_Pop32();break;
	CASE_D(0x60)												/* PUSHAD */
	{
		Bitu tmpesp = reg_esp;
		CPU_Push32(reg_eax);CPU_Push32(reg_ecx);CPU_Push32(reg_edx);CPU_Push32(reg_ebx);
		CPU_Push32(tmpesp);CPU_Push32(reg_ebp);CPU_Push32(reg_esi);CPU_Push32(reg_edi);
	}; break;
	CASE_D(0x61)																	// POPAD
		reg_edi = CPU_Pop32();
		reg_esi = CPU_Pop32();
		reg_ebp = CPU_Pop32();
		CPU_Pop32();																// Don't save ESP
		reg_ebx = CPU_Pop32();
		reg_edx = CPU_Pop32();
		reg_ecx = CPU_Pop32();
		reg_eax = CPU_Pop32();
		if (reg_eax == 0x12345678 && reg_eip == 0x4662)								// 4DOS checking for 80386(SX) bug
			reg_eax = 0;
		break;
	CASE_D(0x62)												/* BOUND Ed */
		{
			Bit32s bound_min, bound_max;
			GetRMrd;GetEAa;
			bound_min=Mem_Lodsd(eaa);
			bound_max=Mem_Lodsd(eaa+4);
			if ((((Bit32s)*rmrd) < bound_min) || (((Bit32s)*rmrd) > bound_max)) {
				EXCEPTION(5);
			}
		}
		break;
	CASE_D(0x63)												/* ARPL Ed,Rd */
		{
			if (!cpu.pmode)
				goto illegal_opcode;
			GetRMrw;
			if (rm >= 0xc0) {
				GetEArd;Bitu new_sel=(Bit16u)*eard;
				CPU_ARPL(new_sel,*rmrw);
				*eard=(Bit32u)new_sel;
			} else {
				GetEAa;Bitu new_sel=Mem_Lodsw(eaa);
				CPU_ARPL(new_sel,*rmrw);
				Mem_Stosd(eaa,(Bit32u)new_sel);
			}
		}
		break;
	CASE_D(0x68)												/* PUSH Id */
		CPU_Push32(Fetchd());break;
	CASE_D(0x69)												/* IMUL Gd,Ed,Id */
		RMGdEdOp3(DIMULD,Fetchds());
		break;
	CASE_D(0x6a)												/* PUSH Ib */
		CPU_Push32(Fetchbs());break;
	CASE_D(0x6b)												/* IMUL Gd,Ed,Ib */
		RMGdEdOp3(DIMULD,Fetchbs());
		break;
	CASE_D(0x6d)												/* INSD */
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 4))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_INSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosd(BaseDI+di_index, IO_ReadD(reg_dx));
			Bits add_index = cpu.direction<<2;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_D(0x6f)												/* OUTSD */
		if (cpu.pmode)
			if (CPU_IO_Exception(reg_dx, 4))
				RUNEXCEPTION();
		if (TEST_PREFIX_REP)
			DoString(R_OUTSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			IO_WriteD(reg_dx, Mem_Lodsd(BaseSI+si_index));
			Bits add_index = cpu.direction<<2;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		break;
	CASE_D(0x70)												/* JO */
		JumpCond32_b(TFLG_O);break;
	CASE_D(0x71)												/* JNO */
		JumpCond32_b(TFLG_NO);break;
	CASE_D(0x72)												/* JB */
		JumpCond32_b(TFLG_B);break;
	CASE_D(0x73)												/* JNB */
		JumpCond32_b(TFLG_NB);break;
	CASE_D(0x74)												/* JZ */
  		JumpCond32_b(TFLG_Z);break;
	CASE_D(0x75)												/* JNZ */
		JumpCond32_b(TFLG_NZ);break;
	CASE_D(0x76)												/* JBE */
		JumpCond32_b(TFLG_BE);break;
	CASE_D(0x77)												/* JNBE */
		JumpCond32_b(TFLG_NBE);break;
	CASE_D(0x78)												/* JS */
		JumpCond32_b(TFLG_S);break;
	CASE_D(0x79)												/* JNS */
		JumpCond32_b(TFLG_NS);break;
	CASE_D(0x7a)												/* JP */
		JumpCond32_b(TFLG_P);break;
	CASE_D(0x7b)												/* JNP */
		JumpCond32_b(TFLG_NP);break;
	CASE_D(0x7c)												/* JL */
		JumpCond32_b(TFLG_L);break;
	CASE_D(0x7d)												/* JNL */
		JumpCond32_b(TFLG_NL);break;
	CASE_D(0x7e)												/* JLE */
		JumpCond32_b(TFLG_LE);break;
	CASE_D(0x7f)												/* JNLE */
		JumpCond32_b(TFLG_NLE);break;
	CASE_D(0x81)												/* Grpl Ed,Id */
		{
			GetRM;Bitu which=(rm>>3)&7;
			if (rm >= 0xc0) {
				GetEArd;Bit32u id=Fetchd();
				switch (which) {
				case 0x00:ADDD(*eard,id,LoadRd,SaveRd);break;
				case 0x01: ORD(*eard,id,LoadRd,SaveRd);break;
				case 0x02:ADCD(*eard,id,LoadRd,SaveRd);break;
				case 0x03:SBBD(*eard,id,LoadRd,SaveRd);break;
				case 0x04:ANDD(*eard,id,LoadRd,SaveRd);break;
				case 0x05:SUBD(*eard,id,LoadRd,SaveRd);break;
				case 0x06:XORD(*eard,id,LoadRd,SaveRd);break;
				case 0x07:CMPD(*eard,id,LoadRd,SaveRd);break;
				}
			} else {
				GetEAa;Bit32u id=Fetchd();
				switch (which) {
				case 0x00:ADDD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x01: ORD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x02:ADCD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x03:SBBD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x04:ANDD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x05:SUBD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x06:XORD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x07:CMPD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				}
			}
		}
		break;
	CASE_D(0x83)												/* Grpl Ed,Ix */
		{
			GetRM;Bitu which=(rm>>3)&7;
			if (rm >= 0xc0) {
				GetEArd;Bit32u id=(Bit32s)Fetchbs();
				switch (which) {
				case 0x00:ADDD(*eard,id,LoadRd,SaveRd);break;
				case 0x01: ORD(*eard,id,LoadRd,SaveRd);break;
				case 0x02:ADCD(*eard,id,LoadRd,SaveRd);break;
				case 0x03:SBBD(*eard,id,LoadRd,SaveRd);break;
				case 0x04:ANDD(*eard,id,LoadRd,SaveRd);break;
				case 0x05:SUBD(*eard,id,LoadRd,SaveRd);break;
				case 0x06:XORD(*eard,id,LoadRd,SaveRd);break;
				case 0x07:CMPD(*eard,id,LoadRd,SaveRd);break;
				}
			} else {
				GetEAa;Bit32u id=(Bit32s)Fetchbs();
				switch (which) {
				case 0x00:ADDD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x01: ORD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x02:ADCD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x03:SBBD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x04:ANDD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x05:SUBD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x06:XORD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				case 0x07:CMPD(eaa,id,Mem_Lodsd,Mem_Stosd);break;
				}
			}
		}
		break;
	CASE_D(0x85)												/* TEST Ed,Gd */
		RMEdGd(TESTD);break;
	CASE_D(0x87)												/* XCHG Ed,Gd */
		{
		GetRMrd;
		Bit32u oldrmrd = *rmrd;
		if (rm < 0xc0)
			{
			GetEAa;
			*rmrd = Mem_Lodsd(eaa);
			Mem_Stosd(eaa, oldrmrd);
			}
		else
			{
			GetEArd;
			*rmrd = *eard;
			*eard = oldrmrd;
			}
		break;
		}
	CASE_D(0x89)												/* MOV Ed,Gd */
		{	
		GetRMrd;
		if (rm >= 0xc0)
			{
			GetEArd;
			*eard = *rmrd;
			}
		else
			{
			GetEAa;
			Mem_Stosd(eaa, *rmrd);
			}
		break;
		}
	CASE_D(0x8b)												/* MOV Gd,Ed */
		{	
		GetRMrd;
		if (rm < 0xc0)
			{
			GetEAa;
			*rmrd = Mem_Lodsd(eaa);
			}
		else
			{
			GetEArd;
			*rmrd = *eard;
			}
		break;
		}
	CASE_D(0x8c)												/* Mov Ew,Sw */
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		if (which > 5)
			goto illegal_opcode;
		Bit16u val = Segs.val[which];
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosw(eaa, val);
			}
		else
			{
			GetEArd;
			*eard = val;
			}
		break;
		}	
	CASE_D(0x8d)												/* LEA Gd */
		{
		//Little hack to always use segprefixed version
		GetRMrd;
		BaseDS = BaseSS = 0;
		*rmrd = (Bit32u)(*EATable[TEST_PREFIX_ADDR*256+rm])();
		break;
		}
	CASE_D(0x8f)												/* POP Ed */
		{
		Bit32u val = CPU_Pop32();
		GetRM;
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosd(eaa, val);
			}
		else
			{
			GetEArd;\
			*eard = val;
			}
		break;
		}
	CASE_D(0x91)												/* XCHG ECX,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_ecx;reg_ecx=temp;break;}
	CASE_D(0x92)												/* XCHG EDX,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_edx;reg_edx=temp;break;}
		break;
	CASE_D(0x93)												/* XCHG EBX,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_ebx;reg_ebx=temp;break;}
		break;
	CASE_D(0x94)												/* XCHG ESP,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_esp;reg_esp=temp;break;}
		break;
	CASE_D(0x95)												/* XCHG EBP,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_ebp;reg_ebp=temp;break;}
		break;
	CASE_D(0x96)												/* XCHG ESI,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_esi;reg_esi=temp;break;}
		break;
	CASE_D(0x97)												/* XCHG EDI,EAX */
		{ Bit32u temp=reg_eax;reg_eax=reg_edi;reg_edi=temp;break;}
		break;
	CASE_D(0x98)												/* CWDE */
		reg_eax=(Bit16s)reg_ax;break;
	CASE_D(0x99)												/* CDQ */
		if (!(reg_eax & 0x80000000))
			reg_edx= 0;
		else
			reg_edx= 0xffffffff;
		break;
	CASE_D(0x9a)																	// CALL FAR Ad
		{ 
		Bit32u newip = Fetchd();
		Bit16u newcs = Fetchw();
		FillFlags();
		CPU_CALL(true, newcs, newip, GETIP);
		if (GETFLAG(TF))
			{	
			cpudecoder = CPU_Core_Normal_Trap_Run;
			return CBRET_NONE;
			}
		continue;
		}
	CASE_D(0x9c)												/* PUSHFD */
		FillFlags();
		CPU_Push32(reg_flags&0xf8ffff);
		break;
	CASE_D(0x9d)												/* POPFD */
		CPU_POPF(true);
		if (GETFLAG(TF))
			{	
			cpudecoder = CPU_Core_Normal_Trap_Run;
			goto decode_end;
			}
		break;
	CASE_D(0xa1)												/* MOV EAX,Od */
		{
			GetEADirect;
			reg_eax=Mem_Lodsd(eaa);
		}
		break;
	CASE_D(0xa3)												/* MOV Od,EAX */
		{
			GetEADirect;
			Mem_Stosd(eaa,reg_eax);
		}
		break;
	CASE_D(0xa5)												/* MOVSD */
		if (TEST_PREFIX_REP)
			DoString(R_MOVSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosd(BaseDI+di_index, Mem_Lodsd(BaseSI+si_index));
			Bits add_index = cpu.direction<<2;
			di_index = (di_index+add_index)&add_mask;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_D(0xa7)												/* CMPSD */
		if (TEST_PREFIX_REP)
			DoString(R_CMPSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			Bitu di_index = reg_edi&add_mask;
			Bit32u val1 = Mem_Lodsd(BaseSI+si_index);
			Bit32u val2 = Mem_Lodsd(BaseDI+di_index);
			Bits add_index = cpu.direction<<2;
			si_index = (si_index+add_index)&add_mask;
			di_index = (di_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPD(val1, val2, LoadD, 0);
			}
		break;
	CASE_D(0xa9)												/* TEST EAX,Id */
		EAXId(TESTD);break;
	CASE_D(0xab)												/* STOSD */
		if (TEST_PREFIX_REP)
			DoString(R_STOSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Mem_Stosd(BaseDI+di_index, reg_eax);
			Bits add_index = cpu.direction<<2;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			}
		break;
	CASE_D(0xad)												/* LODSD */
		if (TEST_PREFIX_REP)
			DoString(R_LODSD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu si_index = reg_esi&add_mask;
			reg_eax = Mem_Lodsd(BaseSI+si_index);
			Bits add_index = cpu.direction<<2;
			si_index = (si_index+add_index)&add_mask;
			reg_esi = (reg_esi&~add_mask)|si_index;
			}
		break;
	CASE_D(0xaf)												/* SCASD */
		if (TEST_PREFIX_REP)
			DoString(R_SCASD);
		else
			{
			Bitu add_mask = AddrMaskTable[core.prefixes&PREFIX_ADDR];
			Bitu di_index = reg_edi&add_mask;
			Bit32u val2 = Mem_Lodsd(BaseDI+di_index);
			Bits add_index = cpu.direction<<2;
			di_index = (di_index+add_index)&add_mask;
			reg_edi = (reg_edi&~add_mask)|di_index;
			CMPD(reg_eax, val2, LoadD, 0);
			}
		break;
	CASE_D(0xb8)												/* MOV EAX,Id */
		reg_eax=Fetchd();break;
	CASE_D(0xb9)												/* MOV ECX,Id */
		reg_ecx=Fetchd();break;
	CASE_D(0xba)												/* MOV EDX,Iw */
		reg_edx=Fetchd();break;
	CASE_D(0xbb)												/* MOV EBX,Id */
		reg_ebx=Fetchd();break;
	CASE_D(0xbc)												/* MOV ESP,Id */
		reg_esp=Fetchd();break;
	CASE_D(0xbd)												/* MOV EBP.Id */
		reg_ebp=Fetchd();break;
	CASE_D(0xbe)												/* MOV ESI,Id */
		reg_esi=Fetchd();break;
	CASE_D(0xbf)												/* MOV EDI,Id */
		reg_edi=Fetchd();break;
	CASE_D(0xc1)												/* GRP2 Ed,Ib */
		GRP2D(Fetchb());break;
	CASE_D(0xc2)												/* RETN Iw */
		reg_eip=CPU_Pop32();
		reg_esp+=Fetchw();
		continue;
	CASE_D(0xc3)												/* RETN */
		reg_eip=CPU_Pop32();
		continue;
	CASE_D(0xc4)												/* LES */
		{	
			GetRMrd;
			if (rm >= 0xc0) goto illegal_opcode;
			GetEAa;
			if (CPU_SetSegGeneral(es,Mem_Lodsw(eaa+4))) RUNEXCEPTION();
			*rmrd=Mem_Lodsd(eaa);
			break;
		}
	CASE_D(0xc5)												/* LDS */
		{	
			GetRMrd;
			if (rm >= 0xc0) goto illegal_opcode;
			GetEAa;
			if (CPU_SetSegGeneral(ds,Mem_Lodsw(eaa+4))) RUNEXCEPTION();
			*rmrd=Mem_Lodsd(eaa);
			break;
		}
	CASE_D(0xc7)												/* MOV Ed,Id */
		{
		GetRM;
		if (rm < 0xc0)
			{
			GetEAa;
			Mem_Stosd(eaa, Fetchd());
			}
		else
			{
			GetEArd;
			*eard = Fetchd();
			}
		break;
		}
	CASE_D(0xc8)												/* ENTER Iw,Ib */
		{
			Bitu bytes=Fetchw();
			Bitu level=Fetchb();
			CPU_ENTER(true,bytes,level);
		}
		break;
	CASE_D(0xc9)												/* LEAVE */
		reg_esp&=cpu.stack.notmask;
		reg_esp|=(reg_ebp&cpu.stack.mask);
		reg_ebp=CPU_Pop32();
		break;
	CASE_D(0xca)												/* RETF Iw */
		{
			Bitu words=Fetchw();
			FillFlags();
			CPU_RET(true, words);
			continue;
		}
	CASE_D(0xcb)												/* RETF */			
		{ 
			FillFlags();
            CPU_RET(true, 0);
			continue;
		}
	CASE_D(0xcf)												/* IRET */
		{
		CPU_IRET(true);
		if (GETFLAG(TF))
			{	
			cpudecoder=CPU_Core_Normal_Trap_Run;
			return CBRET_NONE;
			}
		continue;
		}
	CASE_D(0xd1)												/* GRP2 Ed,1 */
		GRP2D(1);
		break;
	CASE_D(0xd3)												/* GRP2 Ed,CL */
		GRP2D(reg_cl);
		break;
	CASE_D(0xe0)												/* LOOPNZ */
		if (TEST_PREFIX_ADDR)
			JumpCond32_b(--reg_ecx && !get_ZF());
		JumpCond32_b(--reg_cx && !get_ZF());
		break;
	CASE_D(0xe1)												/* LOOPZ */
		if (TEST_PREFIX_ADDR)
			JumpCond32_b(--reg_ecx && get_ZF());
		JumpCond32_b(--reg_cx && get_ZF());
		break;
	CASE_D(0xe2)												/* LOOP */
		if (TEST_PREFIX_ADDR)
			JumpCond32_b(--reg_ecx);
		JumpCond32_b(--reg_cx);
		break;
	CASE_D(0xe3)												/* JCXZ */
		JumpCond32_b(!(reg_ecx&AddrMaskTable[core.prefixes&PREFIX_ADDR]));
		break;
	CASE_D(0xe5)												/* IN EAX,Ib */
		{
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 4))
			RUNEXCEPTION();
		reg_eax = IO_ReadD(port);
		break;
		}
	CASE_D(0xe7)												/* OUT Ib,EAX */
		{
		Bitu port = Fetchb();
		if (cpu.pmode && CPU_IO_Exception(port, 4))
			RUNEXCEPTION();
		IO_WriteD(port,reg_eax);
		break;
		}
	CASE_D(0xe8)												/* CALL Jd */
		{ 
		Bit32s addip = Fetchds();
		SAVEIP;
		CPU_Push32(reg_eip);
		reg_eip += addip;
		continue;
		}
	CASE_D(0xe9)												/* JMP Jd */
		{ 
		Bit32s addip = Fetchds();
		SAVEIP;
		reg_eip += addip;
		continue;
		}
	CASE_D(0xea)												/* JMP Ad */
		{
		Bit32u newip = Fetchd();
		Bit16u newcs = Fetchw();
		CPU_JMP(true, newcs, newip);
		if (!GETFLAG(TF))
			continue;
		FillFlags();
		cpudecoder=CPU_Core_Normal_Trap_Run;
		return CBRET_NONE;
		}
	CASE_D(0xeb)												/* JMP Jb */
		{ 
			Bit32s addip=Fetchbs();
			SAVEIP;
			reg_eip+=addip;
			continue;
		}
	CASE_D(0xed)												/* IN EAX,DX */
		reg_eax=IO_ReadD(reg_dx);
		break;
	CASE_D(0xef)												/* OUT DX,EAX */
		IO_WriteD(reg_dx,reg_eax);
		break;
	CASE_D(0xf7)												/* GRP3 Ed(,Id) */
		{ 
			GetRM;Bitu which=(rm>>3)&7;
			switch (which) {
			case 0x00:											/* TEST Ed,Id */
			case 0x01:											/* TEST Ed,Id Undocumented*/
				{
				if (rm < 0xc0)
					{
					GetEAa;
					TESTD(eaa, Fetchd(), Mem_Lodsd, Mem_Stosd);
					}
				else
					{
					GetEArd;
					TESTD(*eard, Fetchd(), LoadRd, SaveRd);
					}
				break;
				}
			case 0x02:											/* NOT Ed */
				{
				if (rm >= 0xc0)
					{
					GetEArd;
					*eard = ~*eard;
					}
				else
					{
					GetEAa;
					Mem_Stosd(eaa, ~Mem_Lodsd(eaa));
					}
				break;
				}
			case 0x03:											/* NEG Ed */
				{
				lflags.type = t_NEGd;
				if (rm >= 0xc0)
					{
					GetEArd;
					lf_var1d = *eard;
					lf_resd = 0-lf_var1d;
					*eard = lf_resd;
					}
				else
					{
					GetEAa;
					lf_var1d = Mem_Lodsd(eaa);
					lf_resd = 0-lf_var1d;
					Mem_Stosd(eaa, lf_resd);
					}
				break;
				}
			case 0x04:											/* MUL EAX,Ed */
				RMEd(MULD);
				break;
			case 0x05:											/* IMUL EAX,Ed */
				RMEd(IMULD);
				break;
			case 0x06:											/* DIV Ed */
				RMEd(DIVD);
				break;
			case 0x07:											/* IDIV Ed */
				RMEd(IDIVD);
				break;
			}
			break;
		}
	CASE_D(0xff)												/* GRP 5 Ed */
		{
		GetRM;
		Bitu which = (rm>>3)&7;
		switch (which)
			{
		case 0x00:												/* INC Ed */
			RMEd(INCD);
			break;		
		case 0x01:												/* DEC Ed */
			RMEd(DECD);
			break;
		case 0x02:												/* CALL NEAR Ed */
			if (rm < 0xc0)
				{
				GetEAa;
				reg_eip = Mem_Lodsd(eaa);
				}
			else
				{
				GetEArd;
				reg_eip= *eard;
				}
			CPU_Push32(GETIP);
			continue;
		case 0x03:												/* CALL FAR Ed */
			{
			if (rm >= 0xc0)
				goto illegal_opcode;
			GetEAa;
			Bit32u newip = Mem_Lodsd(eaa);
			Bit16u newcs = Mem_Lodsw(eaa+4);
			FillFlags();
			CPU_CALL(true, newcs, newip, GETIP);
			if (GETFLAG(TF))
				{	
				cpudecoder = CPU_Core_Normal_Trap_Run;
				return CBRET_NONE;
				}
			continue;
			}
		case 0x04:												/* JMP NEAR Ed */	
			if (rm < 0xc0)
				{
				GetEAa;
				reg_eip = Mem_Lodsd(eaa);
				}
			else
				{
				GetEArd;
				reg_eip = *eard;
				}
			continue;
		case 0x05:												/* JMP FAR Ed */	
			{
			if (rm >= 0xc0)
				goto illegal_opcode;
			GetEAa;
			Bit32u newip = Mem_Lodsd(eaa);
			Bit16u newcs=Mem_Lodsw(eaa+4);
			FillFlags();
			CPU_JMP(true,newcs,newip);
			if (GETFLAG(TF))
				{	
				cpudecoder = CPU_Core_Normal_Trap_Run;
				return CBRET_NONE;
				}
			}
			continue;
		case 0x06:												/* Push Ed */
			if (rm < 0xc0)
				{
				GetEAa;
				CPU_Push32(Mem_Lodsd(eaa));
				}
			else
				{
				GetEArd;
				CPU_Push32(*eard);
				}
			break;
		default:
			goto illegal_opcode;
			}
			break;
		}


