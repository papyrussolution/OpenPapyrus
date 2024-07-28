// SLRECMGR.CPP
// Copyright (c) A.Sobolev 2024
// @experimental
//
#include <slib-internal.h>
#pragma hdrstop

/*static*/uint SRecPageManager::Helper_SplitRowId_GetOffsetBits(uint64 rowId, uint pageSize)
{
	uint  result = 0U;
	const uint pobw = static_cast<uint>((rowId >> (RowIdBitWidth-4)) & 0xFULL) + 9;
	THROW_S(pobw >= 9 && (pobw - 9) < 16, SLERR_RECMGR_INVROWID_POBW); // @todo @err
	if(pageSize > 0) {
		result = SBits::CeilLog2(pageSize); //(32 - SBits::Clz(pageSize)) - 1;
		THROW_S(result == pobw, SLERR_RECMGR_INVROWID_PDBWFIT); // @todo @err
	}
	else {
		result = pobw;
	}
	CATCH
		result = 0U;
	ENDCATCH
	return result;
}

/*static*/uint64 SRecPageManager::MakeRowId(uint pageSize, uint pageSeq, uint offset)
{
	assert(pageSize >= 512);
	assert((pageSize % 512) == 0);
	uint64 result = 0;
	const uint bits_offs = SBits::CeilLog2(pageSize); //(32 - SBits::Clz(pageSize)) - 1; // pobw
	const uint bits_page = GetPageBits(bits_offs);
	assert((1ULL << bits_offs) >= pageSize);
	assert(bits_offs >= 9);
	assert((bits_offs - 9) < 16);
	THROW(pageSeq > 0); // @todo @err
	THROW(pageSeq < (1ULL << bits_page)); // @todo @err
	THROW(offset < (1ULL << bits_offs)); // @todo @err
	THROW(offset < pageSize); // @todo @err
	result = (static_cast<uint64>(bits_offs - 9) << (RowIdBitWidth-4) | (static_cast<uint64>(pageSeq) << bits_offs) | (static_cast<uint64>(offset)));
	CATCH
		result = 0ULL;
	ENDCATCH
	return result;
}
	
/*static*/int SRecPageManager::SplitRowId_WithPageSizeCheck(uint64 rowId, uint pageSize, uint * pPageSeq, uint * pOffset)
{
	int    ok = 1;
	const uint bits_offs = Helper_SplitRowId_GetOffsetBits(rowId, pageSize);
	THROW(bits_offs);
	{
		const uint bits_page = GetPageBits(bits_offs);
		constexpr uint64 seq_mask = (~0ULL >> (64-(RowIdBitWidth-4)));
		const uint64 ofs_mask = (~0ULL >> (64-bits_offs));
		const uint page_seq = static_cast<uint>((rowId & seq_mask) >> bits_offs);
		const uint offset = static_cast<uint>(rowId & ofs_mask);
		ASSIGN_PTR(pPageSeq, page_seq);
		ASSIGN_PTR(pOffset, offset);
		THROW_S(page_seq > 0, SLERR_RECMGR_INVROWID_PAGESEQLO);
		THROW_S(page_seq < (1ULL << bits_page), SLERR_RECMGR_INVROWID_PAGESEQLO);
		THROW_S(offset < (1ULL << bits_offs), SLERR_RECMGR_INVROWID_OFFSUP);
		THROW_S(!pageSize || offset < pageSize, SLERR_RECMGR_INVROWID_OFFSOUTOFPG);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

/*static*/int SRecPageManager::SplitRowId(uint64 rowId, uint * pPageSeq, uint * pOffset)
{
	return SplitRowId_WithPageSizeCheck(rowId, 0, pPageSeq, pOffset);
}

SRecPageManager::SRecPageManager(uint32 pageSize) : PageSize(pageSize), LastSeq(0)
{
}
	
SRecPageManager::~SRecPageManager()
{
}

int SRecPageManager::GetFreeListForPage(const SDataPage_ * pPage, TSVector <SRecPageFreeList::Entry> & rList) const
{
	rList.clear();
	return pPage ? Fl.GetListForPage(pPage->GetSeq(), rList) : 0;
}

int SRecPageManager::VerifyFreeList()
{
	int    ok = 1;
	{
		// Перебираем все элементы списка свободных блоков и проверяем их актуальность
		const  uint slc = Fl.GetTypeCount();
		for(uint i = 0; i < slc; i++) {
			const SRecPageFreeList::SingleTypeList * p_stl = Fl.GetTypeListByIdx(i);
			if(p_stl) {
				for(uint bi = 0; bi < p_stl->getCount(); bi++) {
					const SRecPageFreeList::Entry & r_fle = p_stl->at(bi);
					uint   offset = 0;
					SDataPage_ * p_page = QueryPageForReading(r_fle.RowId, p_stl->GetType(), &offset);
					if(!p_page) {
						ok = 0; // @todo report error
					}
					else {
						SDataPageHeader::RecPrefix pfx;
						uint ps = p_page->ReadRecPrefix(offset, pfx);
						if(ps == 0) {
							ok = 0; // @todo report error
						}
						else if(!(pfx.Flags & SDataPageHeader::RecPrefix::fDeleted)) {
							ok = 0; // @todo report error
						}
						else if(pfx.PayloadSize != r_fle.FreeSize) {
							ok = 0; // @todo report error
						}
						ReleasePage(p_page);
					}
				}
			}
		}
	}
	if(ok) {
		// Теперь перебираем все страницы и если на какой то из них есть свободное пространство,
		// то ищем соответствие в списке свободных областей
		for(uint seq = 1; seq < LastSeq; seq++) {
			SDataPage_ * p_page = QueryPageBySeqForReading(seq, 0);
			if(p_page) {
				SDataPageHeader::Stat stat;
				TSVector <SRecPageFreeList::Entry> usable_block_list; 
				if(p_page->GetStat(stat, &usable_block_list)) {
					for(uint ubli = 0; ubli < usable_block_list.getCount(); ubli++) {
						const SRecPageFreeList::Entry & r_entry = usable_block_list.at(ubli);
						const SRecPageFreeList::SingleTypeList * p_stl = 0;
						uint idx_in_list = 0;
						if(Fl.SearchEntry(r_entry, &p_stl, &idx_in_list)) {
							assert(p_stl);
							assert(idx_in_list < p_stl->getCount());
							if(p_stl->at(idx_in_list).FreeSize != r_entry.FreeSize) {
								ok = 0; // @todo report error
							}
						}
						else {
							ok = 0; // @todo report error
						}
					}
				}
				else {
					ok = 0; // @todo @err
				}
			}
			else {
				ok = 0; // @todo @err
			}
		}
	}
	return ok;
}

int SRecPageManager::WriteToPage(SDataPage_ * pPage, uint64 rowId, const void * pData, size_t dataLen)
{
	int    ok = 1;
	uint   seq = 0;
	uint   offset = 0;
	uint64 post_write_rowid = 0;
	THROW(pPage);
	THROW(SRecPageManager::SplitRowId(rowId, &seq, &offset));
	assert(pPage->VerifySeq(seq)); // Вызывающая функция не должна была передать неверное сочетание параметров!
	{
		SRecPageFreeList::Entry new_free_entry;
		assert(pPage->VerifyOffset(offset));
		post_write_rowid = pPage->Write(offset, pData, dataLen, &new_free_entry);
		THROW(post_write_rowid);
		assert(post_write_rowid == rowId);
		THROW(Fl.Remove(pPage->GetType(), post_write_rowid));
		if(new_free_entry.RowId) {
			THROW(Fl.Put(pPage->GetType(), new_free_entry.RowId, new_free_entry.FreeSize));
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SRecPageManager::Write(uint64 * pRowId, uint pageType, const void * pData, size_t dataLen)
{
	int    ok = 1;
	SDataPage_ * p_page = 0;
	uint   offset = 0;
	const  SRecPageFreeList::Entry * p_free_entry = Fl.Get(pageType, dataLen);
	if(!p_free_entry) {
		SDataPage_ * p_new_page = AllocatePage(pageType);
		if(p_new_page) {
			p_free_entry = Fl.Get(pageType, dataLen);
		}
	}
	THROW(p_free_entry);
	THROW(p_page = QueryPageForWriting(p_free_entry->RowId, pageType, &offset));
	{
		const uint64 row_id = p_free_entry->RowId; // Функция WriteToPage изменит free_list и p_free_entry будет указывать
			// уже на другие данные!
		THROW(WriteToPage(p_page, row_id, pData, dataLen));
		ASSIGN_PTR(pRowId, row_id);
	}
	CATCHZOK
	ReleasePage(p_page);
	return ok;
}

int SRecPageManager::Update(uint64 rowId, uint64 * pNewRowId, uint pageType, const void * pData, size_t dataLen) // @todo
{
	//
	// Функция изменения записи. Тут может быть несколько вариантов:
	// -- новая запись равна по размеру старой - все очень просто: переписываем данные и все
	// -- новая запись больше по размеру чем старая - две возможные ветки:
	//   -- после текущей записи на странице есть свободный блок, которого хватает для измененной записи:
	//     размечаем увеличенный блок и копируем в него данные. Оставшееся место в ранее свободном блоке
	//     отправляем в список доступных блоков.
	//   -- после текущей записи нет свободного блока: удаляем текущую запись и вносим новую запись как обычно
	//  -- новая запись меньше по размеру чем старая: заново размечаем блок и копируем данные, освободившееся
	//    пространство кидаем в список свободных блоков
	//    Здесь есть небольшая дилемма: с точки зрения эффективности использования пространства страниц
	//    правильно было бы найти наиболее оптимальный с позиции остатка свободный участок среди
	//    всех страниц данных. Если же смотреть с позиции скорости испольнения, то будет выгоднее
	//    резместить новую запись там же и оставшийся "хвост" каким бы он ни был кинуть в список свободных.
	//    Мы воспользуемся вторым методом поскольку перенос записи на другую страницу будет сопровождаться //
	//    заменой ссылок, что сильно усугубит падение производительности.
	//
	int    ok = 1;
	uint   ofs = 0;
	uint64 new_row_id = 0;
	SDataPage_ * p_page = QueryPageForWriting(rowId, 0/*pageType*/, &ofs);
	SDataPageHeader::RecPrefix pfx;
	SRecPageFreeList::UpdGroup ug;
	THROW(p_page);
	const uint32 pfx_size = p_page->ReadRecPrefix(ofs, pfx);
	THROW(pfx_size);
	THROW(!(pfx.Flags & SDataPageHeader::RecPrefix::fDeleted)); // Мы изменяем запись - следовательно блок не может свободным
	if(pfx.PayloadSize == dataLen) {
		SRecPageFreeList::Entry new_free_entry;
		const uint64 post_write_rowid = p_page->Write(ofs, pData, dataLen, &new_free_entry);
		THROW(post_write_rowid);
		assert(post_write_rowid == rowId);
		THROW(Fl.Put(p_page->GetType(), new_free_entry.RowId, new_free_entry.FreeSize));
		new_row_id = rowId;
	}
	else if(pfx.PayloadSize > dataLen) {
		SRecPageFreeList::Entry new_free_entry;
		const uint64 post_write_rowid = p_page->Write(ofs, pData, dataLen, &new_free_entry);
		THROW(post_write_rowid);
		assert(post_write_rowid == rowId);
		THROW(Fl.Put(p_page->GetType(), new_free_entry.RowId, new_free_entry.FreeSize));
		new_row_id = rowId;
	}
	else { // (pfx.PayloadSize < dataLen)
		bool do_insert_and_delete = true;
		// Сначала проверим нет ли за изменяемым блоком свободного пространства...
		const uint ofs_next = ofs + pfx.TotalSize;
		if(ofs_next < p_page->GetSize()) {
			SDataPageHeader::RecPrefix pfx_next;	
			const uint32 pfx_next_size = p_page->ReadRecPrefix(ofs_next, pfx_next);
			THROW(pfx_next_size);
			if(pfx_next.IsDeleted()) {
				// Предполагаем, что страница нормализована, то есть на ней нет двух или более последовательных свободных блоков
				SDataPageHeader::RecPrefix hypot_pfx;
				hypot_pfx.SetTotalSize(pfx.TotalSize + pfx_next.TotalSize, 0);
				uint32 hypot_pfx_size = SDataPageHeader::EvaluateRecPrefix(hypot_pfx, 0, 0);
				THROW(hypot_pfx_size);
				if(hypot_pfx.PayloadSize >= dataLen) {
					SDataPageHeader::RecPrefix new_rp;
					SRecPageFreeList::Entry new_free_entry;
					THROW(p_page->MarkOutBlock(ofs, hypot_pfx.TotalSize, &new_rp));
					uint64 row_id_to_remove_from_fl = MakeRowId(p_page->GetSize(), p_page->GetSeq(), ofs_next);
					THROW(row_id_to_remove_from_fl);
					THROW(Fl.Remove(p_page->GetType(), row_id_to_remove_from_fl));
					const uint64 post_write_rowid = p_page->Write(ofs, pData, dataLen, &new_free_entry);
					THROW(post_write_rowid);
					assert(post_write_rowid == rowId);
					THROW(Fl.Put(p_page->GetType(), new_free_entry.RowId, new_free_entry.FreeSize));
					new_row_id = rowId;
					//
					do_insert_and_delete = false; // мы все сделали: блок ниже не будет проводить процедуру удаления-вставки
				}
			}
		}
		else {
			THROW(ofs_next == p_page->GetSize()); // @todo @err
		}
		if(do_insert_and_delete) {
			THROW(DeleteFromPage(p_page, rowId)); // Здесь нельзя использовать Delete() поскольку возникнет двойное блокирование страницы
			// Перед вызовом Write мы должны освободить страницу
			ReleasePage(p_page);
			p_page = 0; // обнуляем указатель чтобы ReleasePage ниже не пыталась второй раз освобождать его
			THROW(Write(&new_row_id, pageType, pData, dataLen));
		}
	}
	CATCHZOK
	ReleasePage(p_page);
	return ok;
}

int SRecPageManager::DeleteFromPage(SDataPage_ * pPage, uint64 rowId)
{
	int    ok = 1;
	uint   seq = 0;
	uint   offset = 0;
	THROW(pPage && pPage->IsConsistent());
	THROW(SRecPageManager::SplitRowId(rowId, &seq, &offset));
	assert(pPage->VerifySeq(seq)); // Вызывающая функция не должна была передать неверное сочетание параметров!
	THROW(pPage->VerifySeq(seq));
	{
		SRecPageFreeList::Entry free_entry;
		THROW(pPage->Delete(offset, &free_entry));
		{
			SRecPageFreeList::UpdGroup fbug;
			THROW(Fl.Put(pPage->GetType(), free_entry.RowId, free_entry.FreeSize));
			THROW(pPage->MergeFreeEntries(fbug));
			{
				for(uint i = 0; i < fbug.ListToRemove.getCount(); i++) {
					const SRecPageFreeList::Entry & r_fe = fbug.ListToRemove.at(i);
					THROW(Fl.Remove(pPage->GetType(), r_fe.RowId));
				}
			}
			{
				for(uint i = 0; i < fbug.ListToAdd.getCount(); i++) {
					const SRecPageFreeList::Entry & r_fe = fbug.ListToAdd.at(i);
					THROW(Fl.Put(pPage->GetType(), r_fe.RowId, r_fe.FreeSize));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SRecPageManager::Delete(uint64 rowId)
{
	int    ok = 1;
	uint   ofs = 0;
	SDataPage_ * p_page = QueryPageForWriting(rowId, 0/*pageType*/, &ofs);
	if(p_page) {
		THROW(DeleteFromPage(p_page, rowId));
	}
	CATCHZOK
	ReleasePage(p_page);
	return ok;
}

uint SRecPageManager::Read(uint64 rowId, void * pBuf, size_t bufSize)
{
	uint   ofs = 0;
	uint   actual_size = 0;
	SDataPage_ * p_page = QueryPageForReading(rowId, 0/*pageType*/, &ofs);
	if(p_page) {
		actual_size = p_page->Read(ofs, pBuf, bufSize);
	}
	return actual_size;
}

SDataPage_ * SRecPageManager::GetPage(uint32 seq)
{
	SDataPage_ * p_result = 0;
	for(uint32 i = 0; !p_result && i < L.getCount(); i++) {
		SDataPage_ * p_iter = L.at(i);
		if(p_iter && p_iter->VerifySeq(seq)) {
			p_result = p_iter;
		}
	}
	return p_result;
}

SDataPage_ * SRecPageManager::AllocatePage(uint32 type)
{
	SRecPageFreeList::Entry new_free_entry;
	SDataPage_ * p_new_page = new SDataPage_(type, ++LastSeq, PageSize, &new_free_entry);
	if(p_new_page && p_new_page->IsConsistent()) {
		L.insert(p_new_page);
		Fl.Put(type, new_free_entry.RowId, new_free_entry.FreeSize);
	}
	return p_new_page;
}

int SRecPageManager::ReleasePage(SDataPage_ * pPage)
{
	int    ok = -1;
	if(pPage) {
		// @stub
	}
	return ok;
}

SDataPage_ * SRecPageManager::Helper_QueryPage_BySeq(uint seq, uint32 pageType)
{
	SDataPage_ * p_result = GetPage(seq);
	if(p_result && !p_result->VerifyType(pageType)) {
		ReleasePage(p_result);
		p_result = 0;
	}
	return p_result;
}

SDataPage_ * SRecPageManager::QueryPageBySeqForReading(uint seq, uint32 pageType)
{
	return Helper_QueryPage_BySeq(seq, pageType);
}

SDataPage_ * SRecPageManager::QueryPageBySeqForWriting(uint seq, uint32 pageType)
{
	return Helper_QueryPage_BySeq(seq, pageType);
}

SDataPage_ * SRecPageManager::Helper_QueryPage(uint64 rowId, uint32 pageType, uint * pOffset)
{
	SDataPage_ * p_result = 0;
	uint   ofs = 0;
	if(rowId) {
		uint   seq = 0;
		if(SplitRowId(rowId, &seq, &ofs))
			p_result = Helper_QueryPage_BySeq(seq, 0);
	}
	ASSIGN_PTR(pOffset, ofs);
	return p_result;
}

SDataPage_ * SRecPageManager::QueryPageForReading(uint64 rowId, uint32 pageType, uint * pOffset)
{
	// @todo Эта функция должна содержать блокировку на чтение
	return Helper_QueryPage(rowId, pageType, pOffset);
}

SDataPage_ * SRecPageManager::QueryPageForWriting(uint64 rowId, uint32 pageType, uint * pOffset)
{
	// @todo Эта функция должна содержать блокировку на запись
	return Helper_QueryPage(rowId, pageType, pOffset);
}
//
//
//
SDataPageHeader::RecPrefix::RecPrefix() : PayloadSize(0), Flags(0), TotalSize(0)
{
}
		
void SDataPageHeader::RecPrefix::SetPayload(uint size, uint flags)
{
	PayloadSize = size;
	TotalSize = 0;
	Flags = flags;
}

void SDataPageHeader::RecPrefix::SetTotalSize(uint totalSize, uint flags)
{
	//assert(oneof3(padding, 1, 2, 3));
	PayloadSize = 0;
	Flags = flags;
	if(totalSize >= 1 && totalSize <= 3) {
		Flags |= fDeleted; // Ошметки 1..3 байта безусловно трактуем как неиспользуемые блоки
	}
	TotalSize = totalSize;
}

SDataPageHeader::Stat::Stat() : BlockCount(0), FreeBlockCount(0), UsableBlockCount(0), UsableBlockSize(0)
{
}
		
SDataPageHeader::Stat & SDataPageHeader::Stat::Z()
{
	BlockCount = 0;
	FreeBlockCount = 0;
	UsableBlockCount = 0;
	UsableBlockSize = 0;
	return *this;
}

bool SDataPageHeader::IsValid() const
{
	bool   ok = true;
	THROW_S(Signature == SignatureValue, SLERR_RECMGR_INVPAGE_SIGNATURE);
	THROW_S(Size > 0, SLERR_RECMGR_INVPAGE_ZEROSIZE);
	/*FreePos < TotalSize &&*/
	THROW_S(!FixedChunkSize || Type == tFixedChunkPool, SLERR_RECMGR_INVPAGE_FIXEDCHUNK);
	CATCHZOK
	return ok;
}

uint64 SDataPageHeader::MakeRowId(uint offset) const
{
	assert(Size > 0);
	assert(Seq > 0);
	return SRecPageManager::MakeRowId(Size, Seq, offset);
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
						uint64 row_id = MakeRowId(offs);
						THROW(row_id);
						{
							SRecPageFreeList::Entry ue(row_id, pfx.PayloadSize);
							pUsableBlockList->insert(&ue);
						}
					}
				}
			}
			offs += pfx.TotalSize;
		} while(offs < Size);
		THROW(offs == Size);
		THROW(total_block_size == (Size-sizeof(*this)));
	}
	CATCHZOK
	return ok;
}

uint SDataPageHeader::RecPrefix::GetPrefixSize() const
{
	uint   result = 0;
	if(TotalSize == 1) {
		result = 1;
	}
	else if(TotalSize == 2) {
		result = 2;
	}
	else if(TotalSize == 3) {
		result = 3;
	}
	else if(TotalSize) {
		if((Flags & 0x3) == 0) {
			result = 2 + sizeof(uint32);
		}
		else if((Flags & 0x3) == 3) {
			result = 2 + 3;
		}
		else if((Flags & 0x3) == 2) {
			result = 2 + 2;
		}
		else if((Flags & 0x3) == 1) {
			result = 2 + 1;
		}
		else {
			// @error
		}
	}
	else {
		// @error
	}
	return result;	
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
			rPfx.PayloadSize = *reinterpret_cast<const uint32 *>(ptr+2) & 0x00ffffffU;
			THROW_S(rPfx.PayloadSize < (1 << 24), SLERR_RECMGR_RECPFX_PAYLOADSIZE);
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
		else {
			CALLEXCEPT_S(SLERR_RECMGR_RECPFX_FLAGS);
		}
	}
	else {
		CALLEXCEPT_S(SLERR_RECMGR_RECPFX_SIGNATURE);
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
			flags |= RecPrefix::fDeleted;
			pfx_size = 1;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
			}
		}
		else if(rPfx.TotalSize == 2) {
			signature = RecPrefix::Signature_End2;
			flags |= RecPrefix::fDeleted;
			pfx_size = 2;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
				pPfxBuf[1] = RecPrefix::fDeleted;
			}
		}
		else if(rPfx.TotalSize == 3) {
			signature = RecPrefix::Signature_End3;
			flags |= RecPrefix::fDeleted;
			pfx_size = 3;
			if(pPfxBuf) {
				pPfxBuf[0] = signature;
				pPfxBuf[1] = RecPrefix::fDeleted;
				pPfxBuf[2] = 0;
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
			else if(rPfx.TotalSize < ((1U << 16) - 4)) {
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
	rPfx.Flags = flags;
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
	assert((offset+rPfx.PayloadSize+pfx_size) <= Size);
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
					new_free_entry.RowId = MakeRowId(next_position);
					new_free_entry.FreeSize = new_free_pfx.PayloadSize;
				}
				else {
					assert(ex_rp.TotalSize == rp.TotalSize);
				}
			}
			rowid = MakeRowId(start_position);
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

int SDataPageHeader::MergeFreeEntries(SRecPageFreeList::UpdGroup & rFbug)
{
	int    ok = -1;
	uint   total_block_size = 0;
	uint   offs = sizeof(*this);
	uint   sfbl[128]; // sequential free block list.
	uint   sfbl_count = 0; // sequential free block list count
	uint   sfbl_size = 0; // sequential free block list total size
	TSVector <SRecPageFreeList::Entry> free_entry_to_remove_candidate_list;
	THROW(IsValid());
	do {
		RecPrefix pfx;
		uint32 pfx_size = ReadRecPrefix(offs, pfx);
		THROW(pfx_size);
		total_block_size += pfx.TotalSize;
		if(pfx.Flags & RecPrefix::fDeleted) {
			if(sfbl_count >= SIZEOFARRAY(sfbl)) {
				// @todo		
				// Вероятность попадания сюда мизерная: количество последовательных свободных блоков
				// должно превысить SIZEOFARRAY(sfbl). Тем не менее обработать этот случай надо.
			}
			else {
				sfbl[sfbl_count++] = offs;
				sfbl_size += pfx.TotalSize;
				if(pfx.PayloadSize) {
					SRecPageFreeList::Entry free_entry_to_remove_candidate(MakeRowId(offs), pfx.PayloadSize);
					free_entry_to_remove_candidate_list.insert(&free_entry_to_remove_candidate);
				}
			}
		}
		else {
			if(sfbl_count > 1) {
				// merge sequential blocks
				RecPrefix ex_rp;
				RecPrefix new_rp;
				uint32 ex_pfx_size = ReadRecPrefix(sfbl[0], ex_rp); // offset validation
				THROW(ex_pfx_size);
				THROW(MarkOutBlock(sfbl[0], sfbl_size, &new_rp));
				{
					SRecPageFreeList::Entry new_free_entry;
					new_free_entry.RowId = MakeRowId(sfbl[0]);
					new_free_entry.FreeSize = new_rp.PayloadSize;
					rFbug.ListToAdd.insert(&new_free_entry);
				}
				assert(free_entry_to_remove_candidate_list.getCount() <= sfbl_count);
				for(uint i = 0; i < free_entry_to_remove_candidate_list.getCount(); i++) {
					rFbug.ListToRemove.insert(&free_entry_to_remove_candidate_list.at(i));
				}
			}
			//
			sfbl_count = 0;
			sfbl_size = 0;
			free_entry_to_remove_candidate_list.clear();
		}
		offs += pfx.TotalSize;
	} while(offs < Size);
	THROW(offs == Size);
	if(sfbl_count > 1) {
		// merge sequential blocks
		RecPrefix ex_rp;
		RecPrefix new_rp;
		uint32 ex_pfx_size = ReadRecPrefix(sfbl[0], ex_rp); // offset validation
		THROW(ex_pfx_size);
		THROW(MarkOutBlock(sfbl[0], sfbl_size, &new_rp));
		{
			SRecPageFreeList::Entry new_free_entry;
			new_free_entry.RowId = MakeRowId(sfbl[0]);
			new_free_entry.FreeSize = new_rp.PayloadSize;
			rFbug.ListToAdd.insert(&new_free_entry);
		}
		assert(free_entry_to_remove_candidate_list.getCount() <= sfbl_count);
		for(uint i = 0; i < free_entry_to_remove_candidate_list.getCount(); i++) {
			rFbug.ListToRemove.insert(&free_entry_to_remove_candidate_list.at(i));
		}
	}
	THROW(total_block_size == (Size-sizeof(*this)));
	CATCHZOK
	return ok;
}

uint64 SDataPageHeader::Delete(uint offset, SRecPageFreeList::Entry * pNewFreeEntry)
{
	uint64 rowid = 0;
	SRecPageFreeList::Entry new_free_entry;
	THROW(IsValid());
	{
		RecPrefix ex_rp;
		uint32 ex_pfx_size = ReadRecPrefix(offset, ex_rp); // offset validation
		THROW(ex_pfx_size);
		THROW(!(ex_rp.Flags & RecPrefix::fDeleted)); // @todo @err (блок уже помечен как свободный)
		THROW(ex_rp.TotalSize > 3); // @todo @err (не может такого быть, что блок размером 3 или менее байта был занят)
		{
			RecPrefix new_rp;
			THROW(MarkOutBlock(offset, ex_rp.TotalSize, &new_rp));
			new_free_entry.RowId = MakeRowId(offset);
			new_free_entry.FreeSize = new_rp.PayloadSize;
			rowid = new_free_entry.RowId;
		}
	}
	CATCH
		rowid = 0;
		new_free_entry.Z();
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
		if(end_offs > Size) {
			; // @error
		}
		else if(end_offs == Size) {
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
		p_result->Size = totalSize;
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
IMPL_CMPFUNC(SRecPageFreeList_Entry, p1, p2)
{
	const uint64 r1 = static_cast<const SRecPageFreeList::Entry *>(p1)->RowId;
	const uint64 r2 = static_cast<const SRecPageFreeList::Entry *>(p2)->RowId;
	return CMPSIGN(r1, r2);
}

SRecPageFreeList::Entry::Entry(uint64 rowId, uint32 freeSize) : RowId(rowId), FreeSize(freeSize)
{
}
		
SRecPageFreeList::Entry::Entry() : RowId(0ULL), FreeSize(0)
{
}
		
SRecPageFreeList::Entry & SRecPageFreeList::Entry::Z()
{
	RowId = 0ULL;
	FreeSize = 0;
	return *this;
}

int SRecPageFreeList::SingleTypeList::Put(uint64 rowId, uint32 freeSize)
{
	int    ok = 0;
	uint   pos = 0;
	if(lsearch(&rowId, &pos, CMPF_INT64)) {
		if(freeSize <= 3) {
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
		if(freeSize <= 3) {
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
	uint tail_size = 0;
	return SRecPageFreeList::FindOptimalFreeEntry(*this, reqSize, &tail_size);
}

/*static*/const SRecPageFreeList::Entry * SRecPageFreeList::FindOptimalFreeEntry(const TSVector <Entry> & rList, uint reqSize, uint * pTailSize)
{
	const Entry * p_result = 0;
	uint   final_tail = 0;
	uint   min_tail = UINT_MAX;
	uint   min_tail_idx = 0;
	uint   min_badtail = UINT_MAX;
	uint   min_badtail_idx = 0;
	for(uint i = 0; !p_result && i < rList.getCount(); i++) {
		const Entry & r_item = rList.at(i);
		if(r_item.FreeSize == reqSize) {
			p_result = &r_item;
			final_tail = 0;
		}
		else if(r_item.FreeSize > reqSize) {
			uint local_tail = (r_item.FreeSize - reqSize);
			if(oneof2(local_tail, 1, 2)) {
				if(local_tail < min_badtail) {
					min_badtail = local_tail;
					min_badtail_idx = i+1;
				}
			}
			else {
				if(local_tail < min_tail) {
					min_tail = local_tail;
					min_tail_idx = i+1;
				}
			}
		}
	}
	if(!p_result) {
		if(min_tail_idx) {
			p_result = &rList.at(min_tail_idx-1);
			final_tail = min_tail;
		}
		else if(min_badtail_idx) {
			p_result = &rList.at(min_badtail_idx-1);
			final_tail = min_badtail;
		}
	}
	ASSIGN_PTR(pTailSize, final_tail);
	return p_result;
}

int SRecPageFreeList::Put(uint32 type, uint64 rowId, uint32 freeSize)
{
	int    ok = 1;
	SingleTypeList * p_list = 0;
	THROW(rowId); // @todo @err
	for(uint i = 0; !p_list && i < L.getCount(); i++) {
		SingleTypeList * p_iter = L.at(i);
		if(p_iter && p_iter->GetType() == type)
			p_list = p_iter;
	}
	if(!p_list) {
		SingleTypeList * p_new_list = new SingleTypeList(type);
		THROW(p_new_list);
		THROW(L.insert(p_new_list));
		p_list = p_new_list;
	}
	THROW(p_list);
	THROW(p_list->Put(rowId, freeSize));
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

bool SRecPageFreeList::SearchEntry(const Entry & rKey, const SingleTypeList ** ppList, uint * pIdxInList) const
{
	bool   ok = false;
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const SingleTypeList * p_list = L.at(i);
		if(p_list) {
			uint idx = 0;
			if(p_list->lsearch(&rKey, &idx, PTR_CMPFUNC(SRecPageFreeList_Entry))) {
				assert(p_list->at(idx).RowId == rKey.RowId);
				ASSIGN_PTR(ppList, p_list);
				ASSIGN_PTR(pIdxInList, idx);
				ok = true;
			}
		}
	}
	return ok;
}

int SRecPageFreeList::GetListForPage(uint pageSeq, TSVector <SRecPageFreeList::Entry> & rList) const // @debug
{
	rList.clear();
	int    ok = -1;
	for(uint i = 0; i < L.getCount(); i++) {
		const SingleTypeList * p_stl = L.at(i);
		if(p_stl) {
			for(uint j = 0; j < p_stl->getCount(); j++) {
				const Entry & r_entry = p_stl->at(j);
				uint   page_seq = 0;
				uint   page_ofs = 0;
				THROW(SRecPageManager::SplitRowId(r_entry.RowId, &page_seq, &page_ofs));
				if(page_seq == pageSeq) {
					THROW(rList.insert(&r_entry));
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
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
