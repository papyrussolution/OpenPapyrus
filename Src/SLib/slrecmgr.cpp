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
	pfx.SetPayload(dataLen, 0);
	SDataPageHeader * p_page = 0;//QueryPageForWriting(pageType, dataLen);
	const SRecPageFreeList::Entry * p_free_entry = 0;
	uint   offset = 0;
	//SDataPageHeader * SRecPageManager::QueryPageForWriting(uint32 pageType, uint32 reqSize)
	{
		//SDataPageHeader * p_result = 0;
		p_free_entry = Fl.Get(pageType, dataLen);
		if(!p_free_entry) {
			SDataPageHeader * p_new_page = AllocatePage(pageType);
			if(p_new_page) {
				p_free_entry = Fl.Get(pageType, dataLen);
				//assert(p_free_entry->Seq == p_new_page->Seq);
			}
		}
		uint   seq = 0;
		if(p_free_entry && SRecPageManager::SplitRowId(p_free_entry->RowId, PageSize, &seq, &offset)) {
			p_page = GetPage(seq);
		}
		//return p_result;
	}
	if(p_page) {
		SRecPageFreeList::Entry new_free_entry;
		assert(offset >= sizeof(*p_page) && offset < p_page->TotalSize);
		uint64 rowid = p_page->Write(offset, pData, dataLen, &new_free_entry);
		if(rowid) {
			assert(rowid == p_free_entry->RowId);
			Fl.Remove(pageType, rowid);
			if(new_free_entry.RowId && new_free_entry.FreeSize > 3) {
				Fl.Put(pageType, p_page->Seq, new_free_entry.FreeSize);
			}
			ASSIGN_PTR(pRowId, rowid);
		}
		else
			ok = 0;
	}
	return ok;
}

uint SRecPageManager::Read(uint64 rowId, void * pBuf, size_t bufSize)
{
	uint   ofs = 0;
	uint   actual_size = 0;
	SDataPageHeader * p_page = QueryPageForReading(rowId, 0/*pageType*/, &ofs);
	if(p_page) {
		actual_size = p_page->Read(ofs, pBuf, bufSize);
	}
	return actual_size;
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
	SRecPageFreeList::Entry new_free_entry;
	SDataPageHeader * p_new_page = SDataPageHeader::Allocate(type, ++LastSeq, PageSize, &new_free_entry);
	if(p_new_page) {
		L.insert(p_new_page);
		Fl.Put(type, new_free_entry.RowId, new_free_entry.FreeSize);
	}
	return p_new_page;
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
bool SDataPageHeader::IsValid() const
{
	return (Signature == SignatureValue && TotalSize > 0 && /*FreePos < TotalSize &&*/(!FixedChunkSize || Type == tFixedChunkPool));
}

bool SDataPageHeader::GetStat(Stat & rStat, TSVector <SRecPageFreeList::Entry> * pUsableBlockList) const
{
	bool ok = true;
	rStat.Z();
	THROW(IsValid());
	{
		uint   total_block_size = 0;
		uint   offs = sizeof(*this);
		do {
			RecPrefix pfx;
			uint32 pfx_size = ReadRecPrefix(offs, pfx);
			THROW(pfx_size);
			rStat.BlockCount++;
			total_block_size += pfx.TotalSize;
			if(pfx.Flags & RecPrefix::fDeleted) {
				rStat.FreeBlockCount++;
				if(pfx.PayloadSize >= 8) {
					rStat.UsableBlockCount++;
					rStat.UsableBlockSize += pfx.PayloadSize;
					if(pUsableBlockList) {
						uint64 row_id = SRecPageManager::MakeRowId(TotalSize, Seq, offs);
						THROW(row_id);
						{
							SRecPageFreeList::Entry ue(row_id, pfx.PayloadSize);
							pUsableBlockList->insert(&ue);
						}
					}
				}
			}
			offs += pfx.TotalSize;
		} while(offs < TotalSize);
		THROW(offs == TotalSize);
		THROW(total_block_size == (TotalSize-sizeof(*this)));
	}
	CATCHZOK
	return ok;
}

uint32 SDataPageHeader::ReadRecPrefix(uint pos, RecPrefix & rPfx) const
{
	uint32 pfx_size = 0;
	const uint8 * ptr = PTR8C(this)+pos;
	if(ptr[0] == RecPrefix::Signature_End1) {
		pfx_size = 1;
		rPfx.Flags |= RecPrefix::fDeleted;
		rPfx.TotalSize = 1;
		rPfx.PayloadSize = 0;
	}
	else if(ptr[0] == RecPrefix::Signature_End2) {
		pfx_size = 2;
		rPfx.Flags |= RecPrefix::fDeleted;
		rPfx.TotalSize = 2;
		rPfx.PayloadSize = 0;
	}
	else if(ptr[0] == RecPrefix::Signature_End3) {
		pfx_size = 3;
		rPfx.Flags |= RecPrefix::fDeleted;
		rPfx.TotalSize = 3;
		rPfx.PayloadSize = 0;
	}
	else if(oneof2(ptr[0], RecPrefix::Signature_Used, RecPrefix::Signature_Free)) {
		pfx_size = 2;
		rPfx.Flags = ptr[1];
		if((rPfx.Flags & 0x3) == 0) {
			pfx_size += sizeof(uint32);
			rPfx.PayloadSize = *reinterpret_cast<const uint32 *>(ptr+2);
			rPfx.TotalSize = rPfx.PayloadSize + pfx_size;
		}
		else if((rPfx.Flags & 0x3) == 3) {
			pfx_size += 3;
			rPfx.PayloadSize = *reinterpret_cast<const uint32 *>(ptr+2);
			THROW(rPfx.PayloadSize < (1 << 24));
			rPfx.TotalSize = rPfx.PayloadSize + pfx_size;
		}
		else if((rPfx.Flags & 0x3) == 2) {
			pfx_size += 2;
			rPfx.PayloadSize = *reinterpret_cast<const uint16 *>(ptr+2);
			rPfx.TotalSize = rPfx.PayloadSize + pfx_size;
		}
		else if((rPfx.Flags & 0x3) == 1) {
			pfx_size += 1;
			rPfx.PayloadSize = *reinterpret_cast<const uint8 *>(ptr+2);
			rPfx.TotalSize = rPfx.PayloadSize + pfx_size;
		}
	}
	CATCH
		pfx_size = 0;
	ENDCATCH
	return pfx_size;
}

/*static*/uint32 SDataPageHeader::EvaluateRecPrefix(RecPrefix & rPfx, uint8 * pPfxBuf, uint8 * pFlags)
{
	uint  pfx_size = 0;
	uint8 flags = 0;
	uint8 signature = 0;
	if(rPfx.PayloadSize == 0) {
		//
		// Этот блок обрабатывает случай, когда известен rPfx.TotalSize но не определен rPfx.PayloadSize
		//
		uint   payload_size = 0;
		if(rPfx.TotalSize == 1) {
			signature = RecPrefix::Signature_End1;
			pfx_size = 1;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
			}
		}
		else if(rPfx.TotalSize == 2) {
			signature = RecPrefix::Signature_End2;
			pfx_size = 2;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
				pPfxBuf[1] = RecPrefix::fDeleted;
			}
		}
		else if(rPfx.TotalSize == 3) {
			signature = RecPrefix::Signature_End3;
			pfx_size = 2;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
				pPfxBuf[1] = RecPrefix::fDeleted;
			}
		}
		else {
			signature = (rPfx.Flags & RecPrefix::fDeleted) ? RecPrefix::Signature_Free : RecPrefix::Signature_Used;
			if(rPfx.TotalSize < ((1U << 8) - 3)) {
				pfx_size = 3;
				flags = 0x01;
				if(rPfx.Flags & RecPrefix::fDeleted)
					flags |= RecPrefix::fDeleted;
				payload_size = rPfx.TotalSize - pfx_size;
				if(pPfxBuf) {
					pPfxBuf[0] = signature;
					pPfxBuf[1] = flags;
					pPfxBuf[2] = static_cast<uint8>(payload_size);
				}
			}
			else if(rPfx.PayloadSize < ((1U << 16) - 4)) {
				pfx_size = 4;
				flags = 0x02;
				payload_size = rPfx.TotalSize - pfx_size;
				if(rPfx.Flags & RecPrefix::fDeleted)
					flags |= RecPrefix::fDeleted;
				if(pPfxBuf) {
					pPfxBuf[0] = signature;
					pPfxBuf[1] = flags;
					*reinterpret_cast<uint16 *>(pPfxBuf+2) = static_cast<uint16>(payload_size);
				}
			}
			else if(rPfx.TotalSize < ((1 << 24) - 5)) {
				pfx_size = 5;
				flags = 0x03;
				payload_size = rPfx.TotalSize - pfx_size;
				if(rPfx.Flags & RecPrefix::fDeleted)
					flags |= RecPrefix::fDeleted;
				if(pPfxBuf) {
					pPfxBuf[0] = signature;
					pPfxBuf[1] = flags;
					*reinterpret_cast<uint32 *>(pPfxBuf+2) = payload_size;
					assert(pPfxBuf[5] == 0);
				}
			}
			else {
				pfx_size = 6;
				flags = 0x00;
				payload_size = rPfx.TotalSize - pfx_size;
				if(rPfx.Flags & RecPrefix::fDeleted)
					flags |= RecPrefix::fDeleted;
				if(pPfxBuf) {
					pPfxBuf[0] = signature;
					pPfxBuf[1] = flags;
					*reinterpret_cast<uint32 *>(pPfxBuf+2) = payload_size;
				}
			}
		}
		rPfx.PayloadSize = payload_size;
	}
	else {
		//
		// Этот блок обрабатывает случай, когда известен rPfx.PayloadSize но не определен rPfx.TotalSize
		//
		assert(rPfx.TotalSize == 0);
		uint   total_size = 0;
		pfx_size = 2; // signature + flags
		signature = (rPfx.Flags & RecPrefix::fDeleted) ? RecPrefix::Signature_Free : RecPrefix::Signature_Used;
		if(rPfx.PayloadSize < (1U << 8)) {
			pfx_size += 1; 
			flags = 0x01;
			total_size = rPfx.PayloadSize + pfx_size;
			if(pPfxBuf)
				*reinterpret_cast<uint8 *>(pPfxBuf+2) = static_cast<uint8>(rPfx.PayloadSize);
		}
		else if(rPfx.PayloadSize < (1U << 16)) {
			pfx_size += 2;
			flags = 0x02;
			total_size = rPfx.PayloadSize + pfx_size;
			if(pPfxBuf)
				*reinterpret_cast<uint16 *>(pPfxBuf+2) = static_cast<uint16>(rPfx.PayloadSize);
		}
		else if(rPfx.PayloadSize < (1 << 24)) {
			pfx_size += 3;
			flags = 0x03;
			total_size = rPfx.PayloadSize + pfx_size;
			if(pPfxBuf) {
				*reinterpret_cast<uint32 *>(pPfxBuf+2) = static_cast<uint32>(rPfx.PayloadSize);
				assert(pPfxBuf[5] == 0);
			}
		}
		else {
			pfx_size += 4;
			flags = 0x00;
			total_size = rPfx.PayloadSize + pfx_size;
			if(pPfxBuf)
				*reinterpret_cast<uint32 *>(pPfxBuf+2) = static_cast<uint32>(rPfx.PayloadSize);
		}
		rPfx.TotalSize = total_size;
		if(rPfx.Flags & RecPrefix::fDeleted)
			flags |= RecPrefix::fDeleted;
		if(pPfxBuf) {
			pPfxBuf[0] = signature;
			if(pfx_size > 1)
				pPfxBuf[1] = flags;
		}
	}
	ASSIGN_PTR(pFlags, flags);
	// Проверочные asserions для минимальной уверенности, что все сделано правильно
	assert(rPfx.TotalSize > 0);
	assert(rPfx.TotalSize > rPfx.PayloadSize);
	assert(rPfx.TotalSize <= 3 || rPfx.PayloadSize > 0);
	assert((rPfx.TotalSize - rPfx.PayloadSize) == pfx_size);
	assert(rPfx.TotalSize > 3 || flags & RecPrefix::fDeleted);
	return pfx_size;		
}

uint32 SDataPageHeader::WriteRecPrefix(uint offset, RecPrefix & rPfx)
{
	uint8 pfx_buf[32];
	const uint  pfx_size = EvaluateRecPrefix(rPfx, pfx_buf, 0);
	assert((offset+rPfx.PayloadSize+pfx_size) <= TotalSize);
	memcpy(PTR8(this) + offset, pfx_buf, pfx_size);
	return pfx_size;
}

uint64 SDataPageHeader::Write(uint offset, const void * pData, uint dataLen, SRecPageFreeList::Entry * pNewFreeEntry)
{
	assert(pNewFreeEntry != 0);
	uint64 rowid = 0;
	SRecPageFreeList::Entry new_free_entry;
	THROW(IsValid());
	{
		RecPrefix ex_rp;
		uint32 ex_pfx_size = ReadRecPrefix(offset, ex_rp); // offset validation
		THROW(ex_pfx_size);
		assert(ex_rp.TotalSize > dataLen);
		{
			uint32 start_position = offset;
			RecPrefix rp;
			rp.SetPayload(dataLen, 0);
			uint32 pfx_size = WriteRecPrefix(start_position, rp);
			THROW(pfx_size);
			assert(rp.PayloadSize == dataLen);
			assert((pfx_size + dataLen) == rp.TotalSize);
			assert(rp.TotalSize <= ex_rp.TotalSize);
			memcpy(PTR8(this)+start_position+pfx_size, pData, dataLen);
			//
			{
				const uint next_position = start_position+pfx_size+dataLen;
				if(ex_rp.TotalSize > rp.TotalSize) {
					//FreePos += (pfx_size + dataLen);
					SDataPageHeader::RecPrefix new_free_pfx;
					const uint mobr = MarkOutBlock(next_position, (ex_rp.TotalSize - rp.TotalSize), &new_free_pfx);
					THROW(mobr);
					if(mobr != _FFFF32) {
						new_free_entry.RowId = SRecPageManager::MakeRowId(TotalSize, Seq, next_position);
						new_free_entry.FreeSize = new_free_pfx.PayloadSize;
					}
				}
				else {
					assert(ex_rp.TotalSize == rp.TotalSize);
				}
			}
			rowid = SRecPageManager::MakeRowId(TotalSize, Seq, start_position);
		}
	}
	CATCH
		rowid = 0;
		new_free_entry.RowId = 0;
		new_free_entry.FreeSize = 0;
	ENDCATCH
	ASSIGN_PTR(pNewFreeEntry, new_free_entry);
	return rowid;
}

uint SDataPageHeader::Read(uint offset, void * pBuf, size_t bufSize)
{
	uint    result = 0;
	RecPrefix rp;
	uint pfx_size = ReadRecPrefix(offset, rp);
	if(pfx_size) {
		if(pBuf) {
			if(bufSize >= rp.PayloadSize) {
				memcpy(pBuf, PTR8C(this) + offset + pfx_size, rp.PayloadSize);
				result = rp.PayloadSize;
			}
		}
		else {
			result = rp.PayloadSize;
		}
	}
	return result;
}

uint SDataPageHeader::VerifyBlock(uint offset, uint * pSize) const
{
	uint    result = 0;
	if(offset >= sizeof(SDataPageHeader)) {
		RecPrefix pfx;
		uint32 pfx_size = ReadRecPrefix(offset, pfx);
		if(pfx_size) {
			
		}
	}
	return result;
}

uint SDataPageHeader::VerifyBlock(uint offset, uint size) const
{
	uint   result = 0;
	if(offset >= sizeof(SDataPageHeader)) {
		const uint end_offs = (offset + size);
		if(end_offs > TotalSize) {
			; // @error
		}
		else if(end_offs == TotalSize) {
			result = _FFFF32;
		}
		else {
			const uint8 _next_signature = PTR8C(this)[end_offs];
			if(oneof5(_next_signature, RecPrefix::Signature_Used, RecPrefix::Signature_Free, RecPrefix::Signature_End1,
				RecPrefix::Signature_End2, RecPrefix::Signature_End3)) {
				result = _next_signature;
			}
			else {
				; // @error
			}
		}
	}
	return result;
}

uint SDataPageHeader::MarkOutBlock(uint offset, uint size, RecPrefix * pPfx)
{
	uint    _next_signature = VerifyBlock(offset, size);
	RecPrefix pfx;
	if(_next_signature) {
		uint8   pfx_buf[32];
		uint8   pfx_flags = 0;
		pfx.SetTotalSize(size, RecPrefix::fDeleted);
		uint32 pfxsize = EvaluateRecPrefix(pfx, pfx_buf, &pfx_flags);
		if(pfxsize) {
			memcpy(PTR8(this)+offset, pfx_buf, pfxsize);
		}
		else 
			_next_signature = 0;
	}
	ASSIGN_PTR(pPfx, pfx);
	return _next_signature;
}

/*static*/SDataPageHeader * SDataPageHeader::Allocate(uint32 type, uint32 seq, uint totalSize, SRecPageFreeList::Entry * pNewFreeEntry)
{
	SDataPageHeader * p_result = 0;
	SRecPageFreeList::Entry new_free_entry;
	assert(totalSize > sizeof(SDataPageHeader));
	void * ptr = (totalSize > sizeof(SDataPageHeader)) ? SAlloc::M(totalSize) : 0;
	if(ptr) {
		RecPrefix pfx;
		memzero(ptr, totalSize);
		p_result = static_cast<SDataPageHeader *>(ptr);
		p_result->Signature = SignatureValue;
		p_result->Type = type;
		p_result->Seq = seq;
		p_result->TotalSize = totalSize;
		//p_result->FreePos = static_cast<uint32>(sizeof(SDataPageHeader));
		uint mobr = p_result->MarkOutBlock(sizeof(SDataPageHeader), totalSize - sizeof(SDataPageHeader), &pfx);
		assert(mobr);
		if(!mobr) {
			ZFREE(p_result);
		}
		else {
			new_free_entry.RowId = SRecPageManager::MakeRowId(totalSize, seq, sizeof(SDataPageHeader));
			new_free_entry.FreeSize = pfx.PayloadSize;
		}
	}
	ASSIGN_PTR(pNewFreeEntry, new_free_entry);
	return p_result;
}
//
//
//
int SRecPageFreeList::SingleTypeList::Put(uint64 rowId, uint32 freeSize)
{
	int    ok = 0;
	uint   pos = 0;
	if(lsearch(&rowId, &pos, CMPF_INT64)) {
		if(freeSize == 0) {
			atFree(pos);
			ok = 2;
		}
		else {
			Entry & r_entry = at(pos);
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
			Entry new_entry(rowId, freeSize);
			insert(&new_entry);
			ok = 3;
		}
	}
	return ok;
}

const  SRecPageFreeList::Entry * SRecPageFreeList::SingleTypeList::Get(uint32 reqSize) const
{
	const  Entry * p_result = 0;
	uint32 min_tail = UINT_MAX;
	uint   min_tail_idx = 0; // +1
	for(uint i = 0; i < getCount(); i++) {
		const Entry & r_entry = at(i);
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

int SRecPageFreeList::Put(uint32 type, uint64 rowId, uint32 freeSize)
{
	int    ok = 0;
	SingleTypeList * p_list = 0;
	for(uint i = 0; !p_list && i < L.getCount(); i++) {
		SingleTypeList * p_iter = L.at(i);
		if(p_iter && p_iter->GetType() == type) {
			p_list = p_iter;
			ok = p_iter->Put(rowId, freeSize);
		}
	}
	if(!p_list) {
		SingleTypeList * p_new_list = new SingleTypeList(type);
		L.insert(p_new_list);
		p_list = p_new_list;
	}
	if(p_list)
		ok = p_list->Put(rowId, freeSize);
	return ok;
}

int SRecPageFreeList::GetListForPage(uint pageSeq, TSVector <SRecPageFreeList::Entry> & rList) const // @debug
{
	// @unfinished
	int    ok = -1;
	for(uint i = 0; i < L.getCount(); i++) {
		const SingleTypeList * p_stl = L.at(i);
		if(p_stl) {
			for(uint j = 0; j < p_stl->getCount(); j++) {
				const Entry & r_entry = p_stl->at(j);
				//SRecPageManager::SplitRowId(r_entry.RowId, )
			}
		}
	}
	return ok;
}

const SRecPageFreeList::Entry * SRecPageFreeList::Get(uint32 type, uint32 reqSize) const
{
	const  Entry * p_result = 0;
	for(uint i = 0; i < L.getCount(); i++) {
		const SingleTypeList * p_iter = L.at(i);
		if(p_iter && p_iter->GetType() == type) {
			p_result = p_iter->Get(reqSize);
			break;
		}
	}
	return p_result;
}

/*static*/bool SRecPageManager::TestSinglePage(uint pageSize)
{
	bool    ok = true;
	SRecPageManager mgr(pageSize);
	SDataPageHeader * p_page = mgr.AllocatePage(SDataPageHeader::tRecord);
	THROW(p_page);
	{
		SDataPageHeader::Stat stat;
		TSVector <SRecPageFreeList::Entry> free_list;
		THROW(p_page->GetStat(stat, &free_list));
	}
	CATCHZOK
	return ok;
}