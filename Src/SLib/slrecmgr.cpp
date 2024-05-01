// SLRECMGR.CPP
// Copyright (c) A.Sobolev 2024
// @experimental
//
#include <slib-internal.h>
#pragma hdrstop

struct SDataPageHeader {
	static constexpr uint32 SignatureValue = 0x76AE0000U;
	static constexpr uint32 EndSentinelSize() { return 0U; }
	//
	// Descr: Типы страниц
	//
	enum {
		tUndef = 0,
		tStorageHeader   = 1, // Заголовочная страница хранилища. Определяет мета-данные 
		tAllocationTable = 2, // Таблица распределения записей
		tStructure       = 3, // Определение структуры записей
		tRecord          = 4, // Данные записей
		tIndex           = 5, // Индексные данные
		tStringPool      = 6, // Пул текстовых строк 
		tFixedChunkPool  = 7, // Пул отрезков данных фиксированной длины. Например GUID'ы (16байт) или MAC-адреса (6байт)
	};
	enum {
		fUndef = 0,
		fDirty = 0x0001,
		fEmpty = 0x0002
	};
	uint GetFreeSizeFromPos(uint pos) const 
	{ 
		return (pos <= (TotalSize - EndSentinelSize())) ? (TotalSize - EndSentinelSize() - pos) : 0;
	}
	uint GetFreeSize() const 
	{ 
		assert(FreePos <= (TotalSize - EndSentinelSize()));
		return GetFreeSizeFromPos(FreePos);
	}
	bool IsValid() const
	{
		return (Signature == SignatureValue && TotalSize > 0 && FreePos < TotalSize &&
			(!FixedChunkSize || Type == tFixedChunkPool));
	}
	struct RecPrefix {
		static constexpr uint8 Signature = 0x9A;
		enum {
			fDeleted = 0x04
		};
		uint32 Size;
		uint   Flags;
	};
	uint32 ReadRecPrefix(uint pos, RecPrefix & rPfx) const
	{
		uint32 pfx_size = 0;
		THROW(PTR8C(this+1)[pos] == RecPrefix::Signature);
		pfx_size = 2;
		rPfx.Flags = PTR8C(this+1)[pos+1];
		if((rPfx.Flags & 0x3) == 0) {
			pfx_size += sizeof(uint32);
			rPfx.Size = *reinterpret_cast<const uint32 *>(PTR8C(this+1)+2);
		}
		else if((rPfx.Flags & 0x3) == 3) {
			pfx_size += 3;
			rPfx.Size = *reinterpret_cast<const uint32 *>(PTR8C(this+1)+2);
			THROW(rPfx.Size < (1 << 24));
		}
		else if((rPfx.Flags & 0x3) == 2) {
			pfx_size += 2;
			rPfx.Size = *reinterpret_cast<const uint16 *>(PTR8C(this+1)+2);
		}
		else if((rPfx.Flags & 0x3) == 1) {
			pfx_size += 1;
			rPfx.Size = *reinterpret_cast<const uint8 *>(PTR8C(this+1)+2);
		}
		CATCH
			pfx_size = 0;
		ENDCATCH
		return pfx_size;
	}
	uint32 EvaluateRecPrefix(const RecPrefix & rPfx, uint8 * pPfxBuf, uint8 * pFlags) const
	{
		uint  pfx_size = 0;
		uint8 flags = 0;
		pfx_size = 2; // signature + flags
		if(rPfx.Size < (1U << 8)) {
			if(pPfxBuf)
				*reinterpret_cast<uint8 *>(pPfxBuf+2) = static_cast<uint8>(rPfx.Size);
			pfx_size += 1; 
			flags = 0x01;
		}
		else if(rPfx.Size < (1U << 16)) {
			if(pPfxBuf)
				*reinterpret_cast<uint16 *>(pPfxBuf+2) = static_cast<uint16>(rPfx.Size);
			pfx_size += 2;
			flags = 0x02;
		}
		else if(rPfx.Size < (1 << 24)) {
			if(pPfxBuf) {
				*reinterpret_cast<uint32 *>(pPfxBuf+2) = static_cast<uint32>(rPfx.Size);
				assert(pPfxBuf[5] == 0);
			}
			pfx_size += 3;
			flags = 0x03;
		}
		else {
			if(pPfxBuf)
				*reinterpret_cast<uint32 *>(pPfxBuf+2) = static_cast<uint32>(rPfx.Size);
			pfx_size += 4;
			flags = 0x00;
		}
		if(rPfx.Flags & RecPrefix::fDeleted)
			flags |= RecPrefix::fDeleted;
		if(pPfxBuf) {
			pPfxBuf[0] = RecPrefix::Signature;
			pPfxBuf[1] = flags;
		}
		ASSIGN_PTR(pFlags, flags);
		return pfx_size;		
	}
	uint32 WriteRecPrefix(const RecPrefix & rPfx)
	{
		uint8 pfx_buf[32];
		const uint  pfx_size = EvaluateRecPrefix(rPfx, pfx_buf, 0);
		if((rPfx.Size+pfx_size) <= GetFreeSize()) {
			memcpy(PTR8(this) + FreePos, pfx_buf, pfx_size);
			return pfx_size;
		}
		else
			return 0;
	}
	int  Write(const void * pData, uint dataLen)
	{
		int    ok = 1;
		RecPrefix rp;
		rp.Size = dataLen;
		rp.Flags = 0;
		THROW(IsValid());
		{
			uint32 pfx_size = WriteRecPrefix(rp);
			THROW(pfx_size);
			memcpy(PTR8(this)+FreePos+pfx_size, pData, dataLen);
			FreePos += (pfx_size + dataLen);
		}
		CATCHZOK
		return ok;
	}
	const void * Enum(uint * pPos, uint * pSize) const
	{
		const void * p_result = 0;
		if(pPos) {
			uint pos = *pPos;
			if(pos == 0) {
				pos = sizeof(*this);
			} 
			if(pos < FreePos) {
				RecPrefix rp;
				uint pfx_size = ReadRecPrefix(pos, rp);
				if(pfx_size) {
					if(GetFreeSizeFromPos(pos) >= rp.Size) {
						ASSIGN_PTR(pSize, rp.Size);
						pos += rp.Size + pfx_size;
						*pPos = pos;
						p_result = PTR8C(this) + pos + pfx_size;
					}
				}
			}
		}
		return p_result;
	}
	static SDataPageHeader * Allocate(uint32 type, uint32 seq, uint totalSize)
	{
		SDataPageHeader * p_result = 0;
		assert(totalSize > (sizeof(SDataPageHeader) + EndSentinelSize()));
		void * ptr = (totalSize > (sizeof(SDataPageHeader) + EndSentinelSize())) ? SAlloc::M(totalSize) : 0;
		if(ptr) {
			memzero(ptr, totalSize);
			p_result = static_cast<SDataPageHeader *>(ptr);
			p_result->Signature = SignatureValue;
			p_result->Type = type;
			p_result->Seq = seq;
			p_result->TotalSize = totalSize;
			p_result->FreePos = static_cast<uint32>(sizeof(SDataPageHeader));
		}
		return p_result;
	}
	uint32 Signature;
	uint32 Type;         // Тип страницы
	uint32 Seq;          // Порядковый номер страницы. Уникальный в рамках одного хранилища. Поскольку все страницы в хранилище
		// имеют одинаковый размер, то это поле определяет и адрес хранения страницы.
	uint32 NextSeq;      // Номер последующей страницы, хранящей данные того же типа. 0 - для терминальной страницы
	uint32 PrevSeq;      // Номер предыдущей страницы, хранящей данные того же типа. 0 - для первой страницы
	uint16 Flags;
	uint16 FixedChunkSize; // Если GetType() == tFixedChunkPool, то >0 иначе - 0.
	uint32 TotalSize;    // Общий размер страницы вместе с этим заголовком и sentinel'ами (если есть такие)
	uint32 FreePos;      // Смещение, с которого начинается свободное пространство. Свободный размер = (TotalSize-FreePos-EndSentinelSize)
};

class SRecPageManager {
public:
	SRecPageManager(uint32 pageSize) : PageSize(pageSize), LastSeq(0)
	{
	}
	~SRecPageManager()
	{
	}
	int    Write(uint64 * pRowId, uint pageType, const void * pData, size_t dataLen)
	{
		int    ok = 1;
		return ok;
	}
	int    Read(uint64 rowId, void * pBuf, size_t bufSize)
	{
		int    ok = 1;
		return ok;
	}
	SDataPageHeader * AllocatePage(uint32 type)
	{
		SDataPageHeader * p_new_page = SDataPageHeader::Allocate(type, ++LastSeq, PageSize);
		if(p_new_page) {
			L.insert(p_new_page);
		}
		return p_new_page;
	}
private:
	const uint32 PageSize;
	uint32 LastSeq;
	TSCollection <SDataPageHeader> L;
};
