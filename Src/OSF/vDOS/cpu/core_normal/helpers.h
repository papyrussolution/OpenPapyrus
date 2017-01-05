#define GetEAa	PhysPt eaa = EALookupTable[rm]();

#define GetRMEAa															\
	GetRM;																	\
	GetEAa;											


#define RMEbGb(inst)														\
	{																		\
	GetRMrb;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArb;															\
		inst(*earb, *rmrb, LoadRb, SaveRb);									\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, *rmrb, Mem_Lodsb, Mem_Stosb);								\
		}																	\
	}

#define RMGbEb(inst)														\
	{																		\
	GetRMrb;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArb;															\
		inst(*rmrb, *earb, LoadRb, SaveRb);									\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(*rmrb, Mem_Lodsb(eaa), LoadRb, SaveRb);						\
		}																	\
	}

#define RMEb(inst)															\
	{																		\
	if (rm >= 0xc0)															\
		{																	\
		GetEArb;															\
		inst(*earb, LoadRb, SaveRb);										\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, Mem_Lodsb, Mem_Stosb);									\
		}																	\
	}

#define RMEwGw(inst)														\
	{																		\
	GetRMrw;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArw;															\
		inst(*earw, *rmrw, LoadRw, SaveRw);									\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, *rmrw, Mem_Lodsw, Mem_Stosw);								\
		}																	\
	}

#define RMEwGwOp3(inst, op3)												\
	{																		\
	GetRMrw;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArw;															\
		inst(*earw, *rmrw, op3, LoadRw, SaveRw);							\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, *rmrw, op3, Mem_Lodsw, Mem_Stosw);						\
		}																	\
	}

#define RMGwEw(inst)														\
	{																		\
	GetRMrw;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArw;															\
		inst(*rmrw, *earw, LoadRw, SaveRw);									\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(*rmrw, Mem_Lodsw(eaa), LoadRw, SaveRw);						\
		}																	\
	}																

#define RMGwEwOp3(inst, op3)												\
	{																		\
	GetRMrw;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArw;															\
		inst(*rmrw, *earw, op3, LoadRw, SaveRw);							\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(*rmrw, Mem_Lodsw(eaa), op3, LoadRw, SaveRw);					\
		}																	\
	}																

#define RMEdGd(inst)														\
	{																		\
	GetRMrd;																\
	if (rm >= 0xc0) {GetEArd; inst(*eard, *rmrd, LoadRd, SaveRd);}			\
	else {GetEAa; inst(eaa, *rmrd, Mem_Lodsd, Mem_Stosd);}					\
	}

#define RMEdGdOp3(inst,op3)													\
	{																		\
	GetRMrd;																\
	if (rm >= 0xc0) {GetEArd; inst(*eard, *rmrd, op3, LoadRd, SaveRd);}	\
	else {GetEAa; inst(eaa, *rmrd, op3, Mem_Lodsd, Mem_Stosd);}				\
	}

#define RMGdEd(inst)														\
	{																		\
	GetRMrd;																\
	if (rm >= 0xc0)															\
		{																	\
		GetEArd;															\
		inst(*rmrd, *eard, LoadRd, SaveRd);									\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(*rmrd, Mem_Lodsd(eaa), LoadRd, SaveRd);						\
		}																	\
	}																

#define RMGdEdOp3(inst, op3)												\
	{																		\
	GetRMrd;																\
	if (rm >= 0xc0) {GetEArd; inst(*rmrd, *eard, op3, LoadRd, SaveRd);}	\
	else {GetEAa; inst(*rmrd, Mem_Lodsd(eaa), op3, LoadRd, SaveRd);}		\
	}																

#define RMEw(inst)															\
	{																		\
	if (rm >= 0xc0)															\
		{																	\
		GetEArw;															\
		inst(*earw, LoadRw, SaveRw);										\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, Mem_Lodsw, Mem_Stosw);									\
		}																	\
	}

#define RMEd(inst)															\
	{																		\
	if (rm >= 0xc0)															\
		{																	\
		GetEArd;															\
		inst(*eard, LoadRd, SaveRd);										\
		}																	\
	else																	\
		{																	\
		GetEAa;																\
		inst(eaa, Mem_Lodsd, Mem_Stosd);									\
		}																	\
	}

#define ALIb(inst)															\
	{																		\
	inst(reg_al, Fetchb(), LoadRb, SaveRb)									\
	}

#define AXIw(inst)															\
	{																		\
	inst(reg_ax, Fetchw(), LoadRw, SaveRw);									\
	}

#define EAXId(inst)															\
	{																		\
	inst(reg_eax, Fetchd(), LoadRd, SaveRd);								\
	}

#define CASE_W(_WHICH)														\
	case (OPCODE_NONE+_WHICH):

#define CASE_D(_WHICH)														\
	case (OPCODE_SIZE+_WHICH):

#define CASE_B(_WHICH)														\
	CASE_W(_WHICH)															\
	CASE_D(_WHICH)

#define CASE_0F_W(_WHICH)													\
	case ((OPCODE_0F|OPCODE_NONE)+_WHICH):

#define CASE_0F_D(_WHICH)													\
	case ((OPCODE_0F|OPCODE_SIZE)+_WHICH):

#define CASE_0F_B(_WHICH)													\
	CASE_0F_W(_WHICH)														\
	CASE_0F_D(_WHICH)
