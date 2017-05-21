// SFSORT.CPP
// Copyright (c) A.Sobolev 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

class SfSortStringPool : public SStrGroup {
public:
	static int SLAPI Helper_CmpFunc_ByText(const uint * p1, const uint * p2, SfSortStringPool * pArray);
	SLAPI  SfSortStringPool(CompFunc fcmp) : SStrGroup()
	{
		TextCmpProc = fcmp;
	}
	SLAPI  SfSortStringPool(const StrAssocArray & rS)
	{
		TextCmpProc = 0;
		Copy(rS);
	}
	SfSortStringPool & FASTCALL operator = (const SfSortStringPool & rS)
	{
		Copy(rS);
		return *this;
	}
	void SLAPI Clear()
	{
		SPosList.clear();
		ClearS();
	}
	int FASTCALL Copy(const SfSortStringPool & rS)
	{
		SStrGroup::CopyS(rS);
		TextCmpProc = rS.TextCmpProc;
		SPosList = rS.SPosList;
		return 1;
	}
	uint   SLAPI getCount() const
	{
		return SPosList.getCount();
	}
	const  char * FASTCALL Get(uint pos) const
	{
		if(pos < SPosList.getCount()) {
			const uint spos = SPosList.at(pos);
			return (spos && spos < Pool.getDataLen()) ? (Pool.getBuf() + spos) : 0;
		}
		else
			return 0;
	}
	int    FASTCALL Add(const SString & rBuf)
	{
		int    ok = -1;
		if(rBuf.NotEmpty()) {
			uint   str_pos = 0;
			ok = BIN(AddS(rBuf, &str_pos) && SPosList.insert(&str_pos));
		}
		return ok;
	}
	void   SLAPI Sort();
private:
	CompFunc TextCmpProc;
	TSArray <uint> SPosList;
};

class SFSortChunkInfo {
public:
	SFSortChunkInfo()
	{
		ChunkNo = 0;
		LineCount_ = 0;
		CurrentFlushIdx_ = 0;
		P_RdStream = 0;
	}
	~SFSortChunkInfo()
	{
		CleanUp();
	}
	int    CleanUp()
	{
		ZDELETE(P_RdStream);
		if(FileName.NotEmpty()) {
			SFile::Remove(FileName);
			FileName = 0;
		}
		return 1;
	}
	int Finish(const char * pSrcFileName, SfSortStringPool & rChunk)
	{
		const  size_t accum_limit = 512*1024;
		int    ok = 1;
		const uint _c = rChunk.getCount();
		if(_c) {
			rChunk.Sort();
			//
			SPathStruc ps;
			SString line_buf;
			SString temp_buf;
			ps.Split(pSrcFileName);
			ps.Nam.CatChar('-').Cat("temp").CatLongZ(ChunkNo, 8);
			ps.Merge(FileName);
			SFile f_temp(FileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
			THROW(f_temp.IsValid());
			for(uint i = 0; i < _c; i++) {
				temp_buf = rChunk.Get(i);
				if(i == 0)
					First = temp_buf;
				line_buf.Cat(temp_buf.CRB());
				if(line_buf.Len() >= accum_limit) {
					f_temp.WriteLine(line_buf);
					line_buf = 0;
				}
			}
			if(line_buf.Len()) {
				f_temp.WriteLine(line_buf);
				line_buf = 0;
			}
			LineCount_ = _c;
		}
		else
			ok = -1;
		CATCHZOK
		rChunk.Clear();
		return ok;
	}
	int ChargeForMerging()
	{
		int    ok = 1;
		SString temp_buf;
		THROW(P_RdStream == 0);
		THROW(fileExists(FileName));
		THROW_S(P_RdStream = new SFile(FileName, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd), SLERR_NOMEM);
		THROW(P_RdStream->IsValid());
		P_RdStream->ReadLine(temp_buf);
		temp_buf.Chomp();
		THROW(temp_buf == First);
		CurrentFlushIdx_ = 1;
		CATCHZOK
		return ok;
	}
	int    ShiftNext()
	{
		int    ok = 1;
		if(CurrentFlushIdx_ < LineCount_) {
			assert(P_RdStream && P_RdStream->IsValid());
			P_RdStream->ReadLine(First);
			First.Chomp();
		}
		else {
			THROW(CurrentFlushIdx_ <= LineCount_);
			ok = -1;
		}
		CurrentFlushIdx_++;
		CATCHZOK
		return ok;
	}
	uint   ChunkNo;
	//
	// »з-за того, что в файле отрезка может быть более 4G элементов
	// следующие 2 пол€ 64-битные
	// (элементы хран€тс€ только в файле, потому переполнени€ пам€ти не будет)
	//
	uint64 LineCount_;
	uint64 CurrentFlushIdx_;
	//
	SString First;
	SString FileName;
	//
	SFile * P_RdStream;
};

IMPL_CMPCFUNC(SFSortChunkInfo, i1, i2)
{
	const SFSortChunkInfo * ptr1 = (const SFSortChunkInfo *)i1;
	const SFSortChunkInfo * ptr2 = (const SFSortChunkInfo *)i2;
	if(pExtraData) {
		return ((CompFunc)pExtraData)(ptr1->First, ptr2->First, 0);
	}
	else {
		return ptr1->First.CmpNC(ptr2->First);
	}
}

class SFSortChunkInfoList : public TSCollection <SFSortChunkInfo> {
public:
	SFSortChunkInfoList(const char * pSrcFileName, uint maxChunkCount, CompFunc fcmp, SFile::SortParam * pSp) :
		TSCollection <SFSortChunkInfo>(), MaxFlashAccumBufLen(1024*1024),
		SrcFileName(pSrcFileName), MaxChunkCount(maxChunkCount), FCmp(fcmp)
	{
		assert(MaxChunkCount > 1);
		P_Parent = 0;
		RVALUEPTR(Sp, pSp);
	}
	~SFSortChunkInfoList()
	{
		freeAll();
		delete P_Parent; // @recursion
	}
	void Destroy()
	{
		LineBuf.Destroy();
		FlashAccumBuf.Destroy();
	}
	SFSortChunkInfoList * CreateChild()
	{
		SFSortChunkInfoList * p_child = new SFSortChunkInfoList(SrcFileName, MaxChunkCount, FCmp, &Sp);
		THROW_S(p_child, SLERR_NOMEM);
		p_child->P_Parent = this;
		CATCH
			ZDELETE(p_child);
		ENDCATCH
		return p_child;
	}
	SFSortChunkInfo * CreateItem()
	{
		SFSortChunkInfo * p_new_item = CreateNewItem();
		if(p_new_item) {
			long   n = 0;
			SFSortChunkInfoList * p_head = this;
			while(p_head->P_Parent) {
				p_head = p_head->P_Parent;
			}
			assert(p_head);
			n = p_head->LastChunkN.Incr();
            p_new_item->ChunkNo = n;
		}
		return p_new_item;
	}
	void    Sort()
	{
		sort(PTR_CMPCFUNC(SFSortChunkInfo), FCmp);
	}
	int     ChargeForMerging(uint firstIdx, uint lastIdx)
	{
		int    ok = 1;
		const  uint _c = getCount();
		assert(firstIdx < _c);
		assert(lastIdx < _c);
		for(uint i = firstIdx; i <= lastIdx; i++) {
			THROW(at(i)->ChargeForMerging());
		}
		CATCHZOK
		return ok;
	}
	int     FlashCurrent(uint itemIdx, uint lastIdx, SFile & rFOut, SFSortChunkInfo * pResultChunk)
	{
		int    ok = 1;
		SFSortChunkInfo * p_head_item = at(itemIdx);
		if(p_head_item->CurrentFlushIdx_ <= p_head_item->LineCount_) {
			for(uint j = itemIdx+1; j <= lastIdx; j++) {
				SFSortChunkInfo * p_item = at(j);
				while(p_item->CurrentFlushIdx_ <= p_item->LineCount_) {
					int cmpr = 0;
					if(FCmp) {
						cmpr = FCmp(p_head_item->First, p_item->First, 0);
					}
					else {
						cmpr = p_head_item->First.CmpNC(p_item->First);
					}
					if(cmpr > 0) {
						THROW(FlashCurrent(j, lastIdx, rFOut, pResultChunk)); // @recursion
					}
					else
						break;
				}
			}
			(LineBuf = p_head_item->First).CRB();
			FlashAccumBuf.Cat(LineBuf);
			if(pResultChunk) {
				if(pResultChunk->LineCount_ == 0)
					pResultChunk->First = p_head_item->First;
				pResultChunk->LineCount_++;
			}
			if(FlashAccumBuf.Len() >= MaxFlashAccumBufLen) {
				THROW(rFOut.WriteLine(FlashAccumBuf));
				FlashAccumBuf = 0;
			}
			THROW(p_head_item->ShiftNext());
		}
		else
			ok = -1;
		CATCHZOK
		return ok;
	}
	int    FlashFinal(SFile & rFOut)
	{
		int    ok = 1;
		if(FlashAccumBuf.Len()) {
			THROW(rFOut.WriteLine(FlashAccumBuf));
			FlashAccumBuf = 0;
		}
		CATCHZOK
		return ok;
	}
	int    Merge(uint firstIdx, uint lastIdx, SFSortChunkInfoList & rDest, SFileSortProgressData * pCbInfo)
	{
		int    ok = 1;
		assert(firstIdx <= lastIdx);
		assert(firstIdx < getCount());
		assert(lastIdx < getCount());
		THROW(lastIdx >= firstIdx);
		if(lastIdx == firstIdx) {
			SFSortChunkInfo * p_item = at(firstIdx);
			atPut(firstIdx, 0);
			THROW(rDest.insert(p_item));
			atFree(firstIdx);
			if(pCbInfo) {
                pCbInfo->MergeChunkLeft = getCount();
                if(Sp.ProgressCbProc)
					Sp.ProgressCbProc(pCbInfo);
			}
		}
		else if((lastIdx-firstIdx) < MaxChunkCount) {
			THROW(ChargeForMerging(firstIdx, lastIdx));
			{
				SFSortChunkInfo * p_result_chunk = rDest.CreateItem();
				THROW(p_result_chunk);
				{
					SPathStruc ps;
					SString temp_buf;
					ps.Split(SrcFileName);
					ps.Nam.CatChar('-').Cat("temp").CatLongZ(p_result_chunk->ChunkNo, 8);
					ps.Merge(p_result_chunk->FileName);
					{
						SFile f_out(p_result_chunk->FileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
						THROW(f_out.IsValid());
						{
							for(uint i = firstIdx; i <= lastIdx; i++) {
								int  fcr = 0;
								do {
									THROW(fcr = FlashCurrent(i, lastIdx, f_out, p_result_chunk));
								} while(fcr > 0);
							}
							THROW(FlashFinal(f_out));
						}
					}
					{
						for(uint i = 0; i <= (lastIdx-firstIdx); i++) {
							atFree(lastIdx-i);
						}
					}
					if(pCbInfo) {
						pCbInfo->MergeChunkLeft = getCount();
						if(Sp.ProgressCbProc)
							Sp.ProgressCbProc(pCbInfo);
					}
				}
			}
		}
		else {
			for(uint rest = (lastIdx - firstIdx) + 1; rest > 0;) {
				const uint local_count = MIN(MaxChunkCount, rest);
				THROW(Merge(lastIdx-local_count+1, lastIdx, rDest, pCbInfo)); // @recursion
				lastIdx -= local_count;
				rest -= local_count;
			}
		}
		CATCHZOK
		return ok;
	}
private:
	const size_t MaxFlashAccumBufLen;
	const SString SrcFileName;
	const uint   MaxChunkCount;
	CompFunc FCmp;
	SFile::SortParam Sp;

	ACount LastChunkN;
	SString LineBuf; // temporary
	SString FlashAccumBuf;
	SFSortChunkInfoList * P_Parent;
};

static SString & FASTCALL _SfSortMakeFinishEvntName(const char * pSrcFileName, SString & rBuf)
{
    size_t len = sstrlen(pSrcFileName);
    uint32 hash = BobJencHash(pSrcFileName, len);
	(rBuf = "SFSORTFINISHEVNT").CatChar('-').Cat(hash);
	return rBuf;
}

IMPL_CMPFUNC(SfSortStringPool, i1, i2)
{
	return SfSortStringPool::Helper_CmpFunc_ByText((const uint *)i1, (const uint *)i2, (SfSortStringPool *)pExtraData);
}

//static
int SLAPI SfSortStringPool::Helper_CmpFunc_ByText(const uint * p1, const uint * p2, SfSortStringPool * pArray)
{
	uint   pos1 = *p1;
	uint   pos2 = *p2;
	char   stub[8];
	stub[0] = 0;
	size_t ss_data_len = pArray->Pool.getDataLen();
	const char * p_buf = pArray->Pool.getBuf();
	const char * p_str1 = (pos1 == 0 || pos1 >= ss_data_len) ? stub : (p_buf + pos1);
	const char * p_str2 = (pos2 == 0 || pos2 >= ss_data_len) ? stub : (p_buf + pos2);
	return pArray->TextCmpProc ? pArray->TextCmpProc(p_str1, p_str2, 0) : stricmp866(p_str1, p_str2);
}

void SLAPI SfSortStringPool::Sort()
{
	SPosList.sort(PTR_CMPFUNC(SfSortStringPool), this);
}

// static
int SLAPI SFile::Sort(const char * pSrcFileName_, const char * pOutFileName, CompFunc fcmp, SFile::SortParam * pExtraParam)
{
	int    ok = 1;
	const  uint max_chunk_size = (pExtraParam && pExtraParam->MaxChunkSize >= (512*1024) && pExtraParam->MaxChunkSize <= 64*1024*1024) ? pExtraParam->MaxChunkSize : (8*1024*1024);
	const  uint max_chunk_count = (pExtraParam && pExtraParam->MaxChunkCount >= 2 && pExtraParam->MaxChunkCount <= 64) ? pExtraParam->MaxChunkCount : 8;
	const  uint max_thread = (pExtraParam && pExtraParam->MaxThread >= 1 && pExtraParam->MaxThread <= 128) ? pExtraParam->MaxThread : 4;
	//const  int use_mt = BIN(max_thread > 1);
	Evnt * p_ev_finish = 0;
	uint64 line_count = 0;
	const  SString src_file_name = pSrcFileName_;
	SString line_buf;
	SString temp_buf;
	SPathStruc ps;
	SFSortChunkInfoList * p_header_list = new SFSortChunkInfoList(src_file_name, max_chunk_count, fcmp, pExtraParam);
	SFSortChunkInfoList * p_current_list = 0;
	THROW(p_header_list);
	THROW(!isempty(src_file_name) && !isempty(pOutFileName));
	THROW(fileExists(src_file_name));
	{
		SFileSortProgressData cb_info; // ƒанные возвращаемые callback-процедуре (если така€ задана)
		cb_info.P_SrcFileName = src_file_name;
		cb_info.ExtraPtr = pExtraParam ? pExtraParam->ProgressCbExtraPtr : 0;
		{
			//
			// Splitting
			//
			SfSortStringPool chunk_(fcmp);
			SfSortStringPool * p_chunk_ = 0;
			uint   chunk_line_no = 0;
			SFile f_src(src_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
			THROW(f_src.IsValid());
			{
				class SfSortSplitThread : public SlThread {
				public:
					struct InitBlock {
						InitBlock(const char * pSrcFileName, SfSortStringPool * pPool, SFSortChunkInfo * pInfo, volatile int * pResult, ACount * pCntr)
						{
							P_Pool = pPool;
							P_ChunkInfo = pInfo;
							SrcFileName = pSrcFileName;
							P_Result = pResult;
							P_Counter = pCntr;
						}
						InitBlock(InitBlock & rS)
						{
							P_Pool = rS.P_Pool;
							P_ChunkInfo = rS.P_ChunkInfo;
							P_Result = rS.P_Result;
							P_Counter = rS.P_Counter;
							SrcFileName = rS.SrcFileName;
						}
						SfSortStringPool * P_Pool;
						SFSortChunkInfo * P_ChunkInfo;
						volatile int  * P_Result;
						ACount * P_Counter;
						SString SrcFileName;
					};
					SfSortSplitThread(InitBlock * pBlk) : SlThread(pBlk), B(*pBlk)
					{
						B = *pBlk;
					}
					virtual void Run()
					{
						B.P_Counter->Incr();
						if(!B.P_ChunkInfo->Finish(B.SrcFileName, *B.P_Pool)) {
                            *B.P_Result = 0;
						}
                        delete B.P_Pool;
                        long c = B.P_Counter->Decr();
                        if(c <= 0) {
							SString temp_buf;
							Evnt evnt_finish(_SfSortMakeFinishEvntName(B.SrcFileName, temp_buf), Evnt::modeOpen);
							evnt_finish.Signal();
                        }
					}
				private:
					InitBlock B;
				};
				if(max_thread > 1) {
					THROW_S(p_chunk_ = new SfSortStringPool(fcmp), SLERR_NOMEM);
					THROW_S(p_ev_finish = new Evnt(_SfSortMakeFinishEvntName(src_file_name, temp_buf), Evnt::modeCreateAutoReset), SLERR_NOMEM);
				}
				else {
					p_chunk_ = &chunk_;
				}
				cb_info.Phase = 1; // splitting
				ACount thread_counter;
				uint64 total_line_count = 0;
				f_src.CalcSize(&cb_info.TotalFileSize);
				volatile int thread_result = 1;
				thread_counter.Incr();
				while(f_src.ReadLine(line_buf)) {
					line_buf.Chomp();
					total_line_count++;
					if(chunk_.GetPoolDataLen() >= max_chunk_size) {
						SFSortChunkInfo * p_new_ci = p_header_list->CreateItem();
						THROW(p_new_ci);
						if(max_thread > 1) {
							//
							// ≈сли количество потоков превысило предел, то придетс€ ждать завершени€ всех (у нас сейчас нет
							// механизма межпоточного ожидани€ по значению счетчика).
							// Ќе забываем, что мы искусственно увеличиваем на 1 количество потоков.
							//
							if(thread_counter > (long)max_thread) {
								thread_counter.Decr();
								p_ev_finish->Wait(-1);
								thread_counter.Incr();
							}
							*p_chunk_ = chunk_;
							SfSortSplitThread::InitBlock ib(src_file_name, p_chunk_, p_new_ci, &thread_result, &thread_counter);
							SfSortSplitThread * p_thread = new SfSortSplitThread(&ib);
							THROW_S(p_thread, SLERR_NOMEM);
							p_thread->Start(0);
							//
							THROW_S(p_chunk_ = new SfSortStringPool(fcmp), SLERR_NOMEM);
						}
						else {
							THROW(p_new_ci->Finish(src_file_name, *p_chunk_));
						}
						chunk_line_no = 0;
						chunk_.Clear();
					}
					chunk_.Add(line_buf);
                    if((total_line_count % 1000000) == 0) {
                        if(pExtraParam && pExtraParam->ProgressCbProc) {
                            cb_info.SplitBytesRead = f_src.Tell64();
                            cb_info.SplitThreadCount = (thread_counter - 1);
							pExtraParam->ProgressCbProc(&cb_info);
                        }
                    }
				}
				if(chunk_.getCount()) {
					SFSortChunkInfo * p_new_ci = p_header_list->CreateItem();
					THROW(p_new_ci);
					if(max_thread > 1) {
						*p_chunk_ = chunk_;
						SfSortSplitThread::InitBlock ib(src_file_name, p_chunk_, p_new_ci, &thread_result, &thread_counter);
						SfSortSplitThread * p_thread = new SfSortSplitThread(&ib);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(0);
					}
					else {
						THROW(p_new_ci->Finish(src_file_name, *p_chunk_));
					}
					chunk_line_no = 0;
				}
				if(max_thread > 1) {
					if(thread_counter.Decr() > 0)
						p_ev_finish->Wait(-1);
				}
				THROW(thread_result);
				ZDELETE(p_ev_finish);
			}
		}
		{
			//
			// Merging
			//
			cb_info.Phase = 2; // merging
			cb_info.MergeChunkCount = p_header_list->getCount();
			p_current_list = p_header_list;
			while(p_current_list->getCount() > 1) {
				SFSortChunkInfoList * p_child_list = p_current_list->CreateChild();
				THROW(p_child_list);
				p_current_list->Sort();
				if(p_current_list->Merge(0, p_current_list->getCount()-1, *p_child_list, &cb_info)) {
					assert(p_current_list->getCount() == 0);
					p_current_list->Destroy();
					p_current_list = p_child_list;
					p_child_list = 0;
				}
				else {
					ZDELETE(p_child_list);
					CALLEXCEPT();
				}
			}
			assert(oneof2(p_current_list->getCount(), 0, 1));
			if(p_current_list->getCount() == 1) {
				SFSortChunkInfo * p_item = p_current_list->at(0);
				SFile::Remove(pOutFileName);
				THROW(SFile::Rename(p_item->FileName, pOutFileName));
			}
		}
	}
	CATCHZOK
	if(p_current_list) {
		ZDELETE(p_current_list);
	}
	else {
		ZDELETE(p_header_list);
	}
	ZDELETE(p_ev_finish);
	return ok;
}

#if SLTEST_RUNNING // {

IMPL_CMPCFUNC(STRINT64_test, p1, p2)
{
	int64 v1 = _atoi64((const char *)p1);
	int64 v2 = _atoi64((const char *)p2);
	return CMPSIGN(v1, v2);
}

SLTEST_R(FileSort)
{
	int    ok = 1;
	const uint64 max_src_file_size = 57ULL*1024ULL*1024ULL/* *1024ULL */;
	SString test_file_name = MakeOutputFilePath("sfilesort_test.txt");
	SString temp_buf;
	SString line_buf;
	uint64 src_file_size = 0;
	LongArray test_list;
	{
		LongArray temp_test_list;
		SFile f_test(test_file_name, SFile::mWrite|SFile::mNoStd);
		line_buf = 0;
		while(src_file_size < max_src_file_size) {
			const uint rn = SLS.GetTLA().Rg.GetUniformInt(2000000000UL);
			temp_test_list.add((long)rn);
			line_buf.Cat(rn).CR();
			if(line_buf.Len() > (1024*1024)) {
				THROW(SLTEST_CHECK_NZ(f_test.WriteLine(line_buf)));
				src_file_size += line_buf.Len();
				line_buf = 0;
			}
			if(temp_test_list.getCount() >= 1024) {
                THROW(SLTEST_CHECK_NZ(test_list.add(&temp_test_list)));
                temp_test_list.clear();
			}
		}
		if(line_buf.NotEmpty()) {
			THROW(SLTEST_CHECK_NZ(f_test.WriteLine(line_buf)));
			src_file_size += line_buf.Len();
			line_buf = 0;
		}
		THROW(SLTEST_CHECK_NZ(test_list.add(&temp_test_list)));
		test_list.sort();
	}
	{
		SString dest_file_name;
		SFile::SortParam sp;
		sp.MaxChunkCount = 7;
		sp.MaxChunkSize = 1024 * 1024;
		sp.MaxThread = 2;
		{
			SPathStruc ps;
			ps.Split(test_file_name);
			ps.Nam.CatChar('-').Cat("sorted");
			ps.Merge(dest_file_name);
		}
		THROW(SLTEST_CHECK_NZ(SFile::Sort(test_file_name, dest_file_name, PTR_CMPCFUNC(STRINT64_test), &sp)));
		{
            SFile f_result(dest_file_name, SFile::mRead|SFile::mNoStd|SFile::mBinary|SFile::mBuffRd);
            uint   test_count = 0;
            while(f_result.ReadLine(line_buf)) {
				line_buf.Chomp();
				const long value = line_buf.ToLong();
				THROW(SLTEST_CHECK_LT((long)test_count, (long)test_list.getCount()));
				THROW(SLTEST_CHECK_EQ(value, test_list.get(test_count)));
				test_count++;
            }
			THROW(SLTEST_CHECK_EQ(test_count, test_list.getCount()));
		}
	}
	CATCHZOK
	return ok;
}

#endif // } SLTEST_RUNNING

