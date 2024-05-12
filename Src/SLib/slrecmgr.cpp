// SLRECMGR.CPP
// Copyright (c) A.Sobolev 2024
// @experimental
//
#include <slib-internal.h>
#pragma hdrstop

/*static*/uint64 SRecPageManager::MakeRowId(uint pageSize, uint pageSeq, uint offset)
{
	uint64 result = 0;
	const uint bits_offs = (32 - SBits::Clz(pageSize)) + 1;
	const uint bits_page = RowIdBitWidth - bits_offs;
	assert((1ULL << bits_offs) >= pageSize);
	if(pageSeq == 0) {
		; // @error
	}
	else if(pageSeq >= (1ULL << bits_page)) {
		; // @error
	}
	else if(offset >= (1ULL << bits_offs)) {
		; // @error
	}
	else if(offset >= pageSize) {
		; // @error
	}
	else {
		result = (static_cast<uint64>(pageSeq) << bits_offs) | (static_cast<uint64>(offset));
	}
	return result;
}

/*static*/int SRecPageManager::SplitRowId(uint64 rowId, uint pageSize, uint * pPageSeq, uint * pOffset)
{
	int    ok = 1;
	const uint bits_offs = (32 - SBits::Clz(pageSize)) + 1;
	const uint bits_page = RowIdBitWidth - bits_offs;
	constexpr uint64 seq_mask = (~0ULL >> (64-RowIdBitWidth));
	const uint64 ofs_mask = (~0ULL >> (64-bits_offs));
	uint page_seq = static_cast<uint>((rowId & seq_mask) >> bits_offs);
	uint offset = static_cast<uint>(rowId & ofs_mask);
	ASSIGN_PTR(pPageSeq, page_seq);
	ASSIGN_PTR(pOffset, offset);
	if(page_seq == 0) {
		ok = 0; // @error
	}
	else if(page_seq >= (1ULL << bits_page)) {
		ok = 0; // @error
	}
	else if(offset >= (1ULL << bits_offs)) {
		ok = 0; // @error
	}
	else if(offset >= pageSize) {
		ok = 0; // @error
	}
	return ok;
}
SRecPageManager::SRecPageManager(uint32 pageSize) : PageSize(pageSize), LastSeq(0)
{
}
	
SRecPageManager::~SRecPageManager()
{
}

int SRecPageManager::Write(uint64 * pRowId, uint pageType, const void * pData, size_t dataLen)
{
	int    ok = 1;
	SDataPageHeader::RecPrefix pfx;
	pfx.Size = dataLen;
	pfx.Flags = 0;
	const uint pfx_size = SDataPageHeader::EvaluateRecPrefix(pfx, 0, 0);
	SDataPageHeader * p_page = QueryPageForWriting(pageType, dataLen + pfx_size);
	if(p_page) {
		uint64 rowid = p_page->Write(pData, dataLen);
		if(rowid) {
			Fl.Put(pageType, p_page->Seq, p_page->GetFreeSize());
			ASSIGN_PTR(pRowId, rowid);
		}
		else
			ok = 0;
	}
	return ok;
}

int SRecPageManager::Read(uint64 rowId, void * pBuf, size_t bufSize)
{
	int    ok = 1;
	uint   ofs = 0;
	SDataPageHeader * p_page = QueryPageForReading(rowId, 0/*pageType*/, &ofs);
	if(p_page) {
		
	}
	return ok;
}

SDataPageHeader * SRecPageManager::GetPage(uint32 seq)
{
	SDataPageHeader * p_result = 0;
	for(uint32 i = 0; !p_result && i < L.getCount(); i++) {
		SDataPageHeader * p_iter = L.at(i);
		if(p_iter && p_iter->Seq == seq) {
			p_result = p_iter;
		}
	}
	return p_result;
}

SDataPageHeader * SRecPageManager::AllocatePage(uint32 type)
{
	SDataPageHeader * p_new_page = SDataPageHeader::Allocate(type, ++LastSeq, PageSize);
	if(p_new_page) {
		L.insert(p_new_page);
		Fl.Put(type, p_new_page->Seq, p_new_page->GetFreeSize());
	}
	return p_new_page;
}

SDataPageHeader * SRecPageManager::QueryPageForWriting(uint32 pageType, uint32 reqSize)
{
	SDataPageHeader * p_result = 0;
	const SRecPageFreeList::FreeEntry * p_free_entry = Fl.Get(pageType, reqSize);
	if(!p_free_entry) {
		SDataPageHeader * p_new_page = AllocatePage(pageType);
		if(p_new_page) {
			p_free_entry = Fl.Get(pageType, reqSize);
			assert(p_free_entry->Seq == p_new_page->Seq);
		}
	}
	p_result = p_free_entry ? GetPage(p_free_entry->Seq) : 0;
	return p_result;
}

int SRecPageManager::ReleasePage(SDataPageHeader * pPage)
{
	int    ok = -1;
	if(pPage) {
		// @stub
	}
	return ok;
}

SDataPageHeader * SRecPageManager::QueryPageForReading(uint64 rowId, uint32 pageType, uint * pOffset)
{
	SDataPageHeader * p_result = 0;
	uint   ofs = 0;
	if(rowId) {
		uint   seq = 0;
		if(SplitRowId(rowId, PageSize, &seq, &ofs)) {
			p_result = GetPage(seq);
			if(p_result && pageType && p_result->Type != pageType) {
				ReleasePage(p_result);
				p_result = 0;
			}
		}
	}
	ASSIGN_PTR(pOffset, ofs);
	return p_result;
}
//
//
//
uint SDataPageHeader::GetFreeSizeFromPos(uint pos) const 
{ 
	return (pos <= (TotalSize - EndSentinelSize())) ? (TotalSize - EndSentinelSize() - pos) : 0;
}
	
uint SDataPageHeader::GetFreeSize() const 
{ 
	assert(FreePos <= (TotalSize - EndSentinelSize()));
	return GetFreeSizeFromPos(FreePos);
}

bool SDataPageHeader::IsValid() const
{
	return (Signature == SignatureValue && TotalSize > 0 && FreePos < TotalSize &&
		(!FixedChunkSize || Type == tFixedChunkPool));
}

uint32 SDataPageHeader::ReadRecPrefix(uint pos, RecPrefix & rPfx) const
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

/*static*/uint32 SDataPageHeader::EvaluateRecPrefix(const RecPrefix & rPfx, uint8 * pPfxBuf, uint8 * pFlags)
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

uint32 SDataPageHeader::WriteRecPrefix(uint offset, const RecPrefix & rPfx)
{
	uint8 pfx_buf[32];
	const uint  pfx_size = EvaluateRecPrefix(rPfx, pfx_buf, 0);
	if((rPfx.Size+pfx_size) <= GetFreeSize()) {
		memcpy(PTR8(this) + offset, pfx_buf, pfx_size);
		return pfx_size;
	}
	else
		return 0;
}

uint64 SDataPageHeader::Write(const void * pData, uint dataLen)
{
	uint64 rowid = 0;
	RecPrefix rp;
	rp.Size = dataLen;
	rp.Flags = 0;
	THROW(IsValid());
	{
		uint32 start_position = FreePos;
		uint32 pfx_size = WriteRecPrefix(start_position, rp);
		THROW(pfx_size);
		memcpy(PTR8(this)+start_position+pfx_size, pData, dataLen);
		FreePos += (pfx_size + dataLen);
		rowid = SRecPageManager::MakeRowId(TotalSize, Seq, start_position);
	}
	CATCH
		rowid = 0;
	ENDCATCH
	return rowid;
}

const void * SDataPageHeader::Enum(uint * pPos, uint * pSize) const
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

/*static*/SDataPageHeader * SDataPageHeader::Allocate(uint32 type, uint32 seq, uint totalSize)
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
//
//
//
int SRecPageFreeList::SingleTypeList::Put(uint32 seq, uint32 freeSize)
{
	int    ok = 0;
	uint   pos = 0;
	if(lsearch(&seq, &pos, CMPF_LONG)) {
		if(freeSize == 0) {
			atFree(pos);
			ok = 2;
		}
		else {
			FreeEntry & r_entry = at(pos);
			if(r_entry.FreeSize != freeSize) {
				r_entry.FreeSize = freeSize;
				ok = 1;
			}
			else
				ok = -1;
		}
	}
	else {
		if(freeSize == 0) {
			ok = -2;
		}
		else {
			FreeEntry new_entry;
			new_entry.Seq = seq;
			new_entry.FreeSize = freeSize;
			insert(&new_entry);
			ok = 3;
		}
	}
	return ok;
}

const  SRecPageFreeList::FreeEntry * SRecPageFreeList::SingleTypeList::Get(uint32 reqSize) const
{
	const  FreeEntry * p_result = 0;
	uint32 min_tail = UINT_MAX;
	uint   min_tail_idx = 0; // +1
	for(uint i = 0; i < getCount(); i++) {
		const FreeEntry & r_entry = at(i);
		if(r_entry.FreeSize >= reqSize) {
			const uint32 tail = (r_entry.FreeSize - reqSize);
			if(tail < min_tail) {
				min_tail = tail;
				min_tail_idx = i+1;
			}
		}
	}
	if(min_tail_idx) {
		p_result = &at(min_tail_idx-1);
	}
	return p_result;
}

int SRecPageFreeList::Put(uint32 type, uint32 seq, uint32 freeSize)
{
	int    ok = 0;
	SingleTypeList * p_list = 0;
	for(uint i = 0; !p_list && i < L.getCount(); i++) {
		SingleTypeList * p_iter = L.at(i);
		if(p_iter && p_iter->GetType() == type) {
			p_list = p_iter;
			ok = p_iter->Put(seq, freeSize);
		}
	}
	if(!p_list) {
		SingleTypeList * p_new_list = new SingleTypeList(type);
		L.insert(p_new_list);
		p_list = p_new_list;
	}
	if(p_list)
		ok = p_list->Put(seq, freeSize);
	return ok;
}

const SRecPageFreeList::FreeEntry * SRecPageFreeList::Get(uint32 type, uint32 reqSize) const
{
	const  FreeEntry * p_result = 0;
	for(uint i = 0; i < L.getCount(); i++) {
		const SingleTypeList * p_iter = L.at(i);
		if(p_iter && p_iter->GetType() == type) {
			p_result = p_iter->Get(reqSize);
			break;
		}
	}
	return p_result;
}

static void Test()
{
	struct DataEntry {
		static DataEntry * Generate(uint size)
		{
			DataEntry * p_result = 0;
			if(size) {
				p_result = static_cast<DataEntry *>(SAlloc::M(size+sizeof(DataEntry)));
				if(p_result) {
					p_result->Size = size;
					p_result->RowId = 0;
					SLS.GetTLA().Rg.ObfuscateBuffer(p_result+1, size);
				}
			}
			return p_result;
		}
		uint   Size;
		uint64 RowId;
	};
	SCollection data_list;
	const uint entry_count = 100;
	const uint max_rec_size = 500;
	SRecPageManager rm(4096);
	{
		for(uint i = 0; i < entry_count; i++) {
			uint rs = SLS.GetTLA().Rg.GetUniformIntPos(max_rec_size+1);
			if(rs > 0) {
				DataEntry * p_entry = DataEntry::Generate(rs);
				if(p_entry) {
					uint64 row_id = 0;
					if(rm.Write(&row_id, SDataPageHeader::tRecord, p_entry+1, p_entry->Size)) {
						p_entry->RowId = row_id;
						data_list.insert(p_entry);
					}
					else
						SAlloc::F(p_entry);
				}
			}
		}
	}
	{
		for(uint i = 0; i < data_list.getCount(); i++) {
			const DataEntry * p_entry = static_cast<const DataEntry *>(data_list.at(i));
			if(p_entry) {
				uint8 rec_buf[max_rec_size*2];
				rm.Read(p_entry->RowId, rec_buf, sizeof(rec_buf));
			}
		}
	}
}