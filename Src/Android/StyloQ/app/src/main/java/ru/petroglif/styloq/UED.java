// UED.JAVA
// Copyright (c) A.Sobolev 2025
// Частичная реимплементация методов класса UED из проекта Papyrus (UED.H)
//
package ru.petroglif.styloq;

public class UED {
	static boolean IsMetaId(long ued) { return (SLib.HiDWord(ued) == 1); }
	//
	// Descr: Возвращает количество бит данных, доступных для значения UED-величины в
	//   зависимости от мета-идентификатора.
	//   Если аргумент meta не является мета-идентификатором, то возвращает 0.
	//
	static int GetMetaRawDataBits(long meta)
	{
		int    result = 0;
		if(IsMetaId(meta)) {
			final int meta_lodw = SLib.LoDWord(meta);
			if((meta_lodw & 0x80000000) != 0)
				result = 56;
			else if((meta_lodw & 0x40000000) != 0)
				result = 48;
			else
				result = 32;
		}
		return result;
	}
	//
	// Descr: Возвращает количество бит данных, доступных для значения UED.
	//   Если аргумент является meta-идентификатором или не является валидным UED-значением,
	//   то возвращает 0.
	//
	static int GetRawDataBits(long ued)
	{
		int    result = 0;
		if(!IsMetaId(ued)) {
			final int hi_dword = SLib.HiDWord(ued);
			if((hi_dword & 0x80000000) != 0)
				result = 56;
			else if((hi_dword & 0x40000000) != 0)
				result = 48;
			else if(hi_dword != 0)
				result = 32;
		}
		return result;
	}
	static long GetMeta(long ued)
	{
		long result = 0L;
		if(IsMetaId(ued))
			result = UED_ID.UED_META_META; // meta
		else {
			int dw_hi = SLib.HiDWord(ued);
			int b_hi = (dw_hi >> 24);
			if((b_hi & 0x80) != 0)
				result = (0x0000000100000000L | (dw_hi & 0xff000000));
			else if((b_hi & 0x40) != 0)
				result = (0x0000000100000000L | (dw_hi & 0xffff0000));
			else if(dw_hi != 0)
				result = (0x0000000100000000L | (dw_hi & 0x0fffffff));
		}
		return result;
	}
	static boolean BelongToMeta(long ued, long meta) { return (IsMetaId(meta) && GetMeta(ued) == meta); }
	static long GetRawValue(long ued)
	{
		long   result = 0L;
		int    bits = GetRawDataBits(ued);
		assert(bits == 56 || bits == 48 || bits == 32 || bits == 0);
		if(bits == 32)
			result = SLib.LoDWord(ued);
		else if(bits == 48)
			result = ued & 0x0000ffffffffffffL;
		else if(bits == 56)
			result = ued & 0x0000ffffffffffffL;
		return result;
	}
	static int GetRawValue32(long ued)
	{
		int bits = GetRawDataBits(ued);
		assert(bits == 56 || bits == 48 || bits == 32 || bits == 0);
		return (bits == 32) ? SLib.LoDWord(ued) : 0;
	}
	static long ApplyMetaToRawValue(long meta, long rawValue)
	{
		long result = 0L;
		int  bits = GetMetaRawDataBits(meta);
		assert(bits < 64);
		if(bits > 0) {
			if((64 - Long.numberOfLeadingZeros(rawValue)) <=bits) {
				long meta_lodw = SLib.LoDWord(meta);
				if((meta_lodw & 0x80000000) != 0)
					result = ((meta_lodw) << 32) | (rawValue & 0x00ffffffffffffffL);
				else if((meta_lodw & 0x40000000) != 0)
					result = ((meta_lodw) << 32) | (rawValue & 0x0000ffffffffffffL);
				else
					result = ((meta_lodw) << 32) | (rawValue & 0x00000000ffffffffL);
			}
		}
		return result;
	}
	static long SetRaw_Oid(SLib.PPObjID oid)
	{
		long result = 0;
		long meta = 0;
		switch(oid.Type) {
			case SLib.PPOBJ_GOODS: meta = UED_ID.UED_META_PRV_WARE; break;
			case SLib.PPOBJ_PERSON: meta = UED_ID.UED_META_PRV_PERSON; break;
			case SLib.PPOBJ_LOCATION: meta = UED_ID.UED_META_PRV_LOCATION; break;
			case SLib.PPOBJ_BILL: meta = UED_ID.UED_META_PRV_DOC; break;
			case SLib.PPOBJ_LOT: meta = UED_ID.UED_META_PRV_LOT; break;
		}
		if(meta != 0L) {
			result = ApplyMetaToRawValue(meta, oid.Id);
		}
		return result;
	}
	static SLib.PPObjID GetRaw_Oid(long ued)
	{
		SLib.PPObjID result = null;
		int    obj_type = 0;
		long   meta = GetMeta(ued);
		if(meta == UED_ID.UED_META_PRV_WARE)
			obj_type = SLib.PPOBJ_GOODS;
		else if(meta == UED_ID.UED_META_PRV_PERSON)
			obj_type = SLib.PPOBJ_PERSON;
		else if(meta == UED_ID.UED_META_PRV_LOCATION)
			obj_type = SLib.PPOBJ_LOCATION;
		else if(meta == UED_ID.UED_META_PRV_DOC)
			obj_type = SLib.PPOBJ_BILL;
		else if(meta == UED_ID.UED_META_PRV_LOT)
			obj_type = SLib.PPOBJ_LOT;
		if(obj_type != 0) {
			long raw_value = GetRawValue(ued);
			result = new SLib.PPObjID(obj_type, (int)raw_value);
		}
		return result;
	}
}
