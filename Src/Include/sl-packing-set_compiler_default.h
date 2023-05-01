// SL-PACKING-SET_COMPILER_DEFAULT.H
//
// @attention: don't set there #pragma once or other trick for preventing several instances during one compilation module!
#pragma warning(disable:4103) // warning C4103: alignment changed after including header, may be due to missing #pragma pack(pop) 
#if _M_X64
	#pragma pack(push, 16)
#else
	#pragma pack(push, 8)
#endif
