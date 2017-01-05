// SSYSINFO.CPP
// Copyright (c) A.Sobolev 2007
//
#include <slib.h>

class SSysInfo {
public:
	enum {
		cpuI086 = 1,
		cpuI186,
		cpuI286,
		cpuI386,
		cpuI486,
		cpuIPENT,
		cpuIP6,
		cpuPENT2,
		cpuPENT3,
		cpuPENT4
	};
	enum {
		cpufMMX     = 0x0001,
		cpufSSE     = 0x0002,
		cpufSSE2    = 0x0004,
		cpufSSE3    = 0x0008,
		cpuf3DNOW   = 0x0010
	};
	enum {
		cpumanufIntel = 1,
		cpumanufAMD
	};
	SSysInfo();
private:
	int    RetrieveCpuInfo();
	int    RetrieveCacheInfo();

	uint16 CpuCount;
	uint16 Reserve;
	uint16 CpuType;
	uint16 CpuManuf;
	uint32 CpuFeatures;
	uint32 CpuFreq;
	uint32 C1DSize;
	uint32 C1CSize;
	uint32 C2DSize;
	uint32 C2CSize;
	uint16 C1DLine;
	uint16 C2DLine;
};

SSysInfo::SSysInfo()
{
}

int GetCpuId(uint32 input, uint32 * pOutput)
{
	int    ok = 1;
    __try {
		__asm {
			mov eax, input
			cpuid
			mov edi, pOutput
			mov [edi], eax
			mov [edi+4], ebx
			mov [edi+8], ecx
			mov [edi+12], edx
		}
	}
    __except(1) {
		ok = 0;
    }
	return ok;
}

enum CpuIdCmd_Intel {
	cpuidcmdiInit    = 0,
	cpuidcmdiFeature = 1,
	cpuidcmdiCache   = 2,
};

long GetCpuFeatures()
{
	long   result = 0;
    __try {
		struct Reg {
			uint32 f_ax;
			uint32 f_bx;
			uint32 f_cx;	
			uint32 f_dx;
		};
		union {
			Reg    R;
			char   S[16];
		} cpu_ret;
		__asm {
			xor eax, eax
			cpuid
			mov cpu_ret.R.f_ax, eax
			mov cpu_ret.R.f_bx, ebx
			mov cpu_ret.R.f_cx, ecx
			mov cpu_ret.R.f_dx, edx			
		}
		if(memcmp(cpu_ret.str+4, "GenuineIntel", 12) == 0) {
			__asm {
				mov eax, 1
				cpuid
				mov cpu_ret.R.f_ax, eax
				mov cpu_ret.R.f_bx, ebx
				mov cpu_ret.R.f_cx, ecx
				mov cpu_ret.R.f_dx, edx
			}			
			if(cpu_ret.R.f_dx & (1 << 23)) result |= SSysInfo::cpufMMX;
			if(cpu_ret.R.f_dx & (1 << 25)) result |= SSysInfo::cpufSSE;
			if(cpu_ret.R.f_dx & (1 << 26)) result |= SSysInfo::cpufSSE2;
			if(cpu_ret.R.f_cx & (1 <<  0)) result |= SSysInfo::cpufSSE3;
			
		}
		else if(memcmp(cpu_ret.str+4, "AuthenticAMD", 12) == 0) {
			__asm {
				mov eax, 1
				cpuid
				mov cpu_ret.R.f_ax, eax
				mov cpu_ret.R.f_bx, ebx
				mov cpu_ret.R.f_cx, ecx
				mov cpu_ret.R.f_dx, edx				
			}
		}
	}
    __except(1) {
		ok = 0;
    }	
}

void SSysInfo::DetectX86Features()
{
	word32 cpuid[4], cpuid1[4];
	if(!GetCpuId(0, cpuid))
		return;
	if(!GetCpuId(1, cpuid1))
		return;
	SETFLAG(CpuFeatures, cpufMMX, cpuid1[3] & (1 << 23));
	if(cpuid1[3] & (1 << 26))
		SETFLAG(CpuFeatures, cpufSSE2, TrySSE2());
	SETFLAG(CpuFeatures, cpufSSE3, CpuFeatures & cpufSSE2 && (cpuid1[2] & (1<<9)));
	Exchange(&cpuid[2], &cpuid[3]);
	if(memcmp(cpuid+1, "GenuineIntel", 12) == 0) {
		CpuManuf = cpumanufIntel;
		if(((cpuid1[0] >> 8) & 0xf) == 0xf)
			CpuType = cpuPENT4;			
		C1DLine = 8 * GETBYTE(cpuid1[1], 1);
	}
	else if(memcmp(cpuid+1, "AuthenticAMD", 12) == 0) {
		CpuManuf = cpumanufAMD;
		CpuId(0x80000005, cpuid);
		C1DLine = GETBYTE(cpuid[2], 0);
	}
	if(!C1DLine)
		C1DLine = CRYPTOPP_L1_CACHE_LINE_SIZE;
}

int SSysInfo::RetrieveCacheInfo()
{
}
