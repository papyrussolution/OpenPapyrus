// SFSORT.CPP
// Copyright (c) A.Sobolev 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

class SFSortChunkInfo {
public:
	SFSortChunkInfo()
	{
		ChunkNo = 0;
		LineCount = 0;
		CurrentFlushIdx = 0;
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
	int Finish(const char * pSrcFileName, StrAssocArray & rChunk)
	{
		const  size_t accum_limit = 512*1024;
		int    ok = 1;
		const uint _c = rChunk.getCount();
		if(_c) {
			rChunk.SortByText();
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
				temp_buf = rChunk.at_WithoutParent(i).Txt;
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
			LineCount = _c;
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
		CurrentFlushIdx = 1;
		CATCHZOK
		return ok;
	}
	int    ShiftNext()
	{
		int    ok = 1;
		if(CurrentFlushIdx < LineCount) {
			assert(P_RdStream && P_RdStream->IsValid());
			P_RdStream->ReadLine(First);
			First.Chomp();
		}
		else {
			THROW(CurrentFlushIdx <= LineCount);
			ok = -1;
		}
		CurrentFlushIdx++;
		CATCHZOK
		return ok;
	}
	uint   ChunkNo;
	uint   LineCount;
	uint   CurrentFlushIdx;
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
	SFSortChunkInfoList(const char * pSrcFileName, uint maxChunkCount, CompFunc fcmp) :
		TSCollection <SFSortChunkInfo>(), MaxFlashAccumBufLen(1024*1024),
		SrcFileName(pSrcFileName), MaxChunkCount(maxChunkCount), FCmp(fcmp)
	{
		assert(MaxChunkCount > 1);
		LastChunkN = 0;
	}
	SFSortChunkInfo * CreateItem(int dontInsert)
	{
		SFSortChunkInfo * p_new_item = dontInsert ? (new SFSortChunkInfo) : CreateNewItem(0);
		if(p_new_item) {
            p_new_item->ChunkNo = ++LastChunkN;
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
		if(p_head_item->CurrentFlushIdx <= p_head_item->LineCount) {
			for(uint j = itemIdx+1; j <= lastIdx; j++) {
				SFSortChunkInfo * p_item = at(j);
				while(p_item->CurrentFlushIdx <= p_item->LineCount) {
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
				if(pResultChunk->LineCount == 0)
					pResultChunk->First = p_head_item->First;
				pResultChunk->LineCount++;
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
	int    Merge(uint firstIdx, uint lastIdx)
	{
		int    ok = 1;
		assert(firstIdx <= lastIdx);
		assert(firstIdx < getCount());
		assert(lastIdx < getCount());
		if(lastIdx > firstIdx) {
			if((lastIdx-firstIdx) < MaxChunkCount) {
				THROW(ChargeForMerging(firstIdx, lastIdx));
				{
					SFSortChunkInfo * p_result_chunk = CreateItem(1);
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
							THROW(atInsert(firstIdx, p_result_chunk));
						}
					}
				}
			}
			else {
				const uint middle_idx = (lastIdx + firstIdx) / 2;
				//
				// Важно: начинаем с верхних индексов, поскольку они будут замещены единственным блоком
				//   и позиции блоков по нижним индексам при этом не изменяться.
				//
				THROW(Merge(middle_idx+1, lastIdx)); // @recursion
				THROW(Merge(firstIdx, middle_idx)); // @recursion
				{
					THROW(Merge(firstIdx, firstIdx+1)); // @recursion
				}
			}
		}
		else {
			// Nothing to do
		}
		CATCHZOK
		return ok;
	}
private:
	const size_t MaxFlashAccumBufLen;
	const SString SrcFileName;
	const uint   MaxChunkCount;
	CompFunc FCmp;

	uint   LastChunkN;
	SString LineBuf; // temporary
	SString FlashAccumBuf;
};

static SString & FASTCALL _SfSortMakeFinishEvntName(const char * pSrcFileName, SString & rBuf)
{
    size_t len = sstrlen(pSrcFileName);
    uint32 hash = BobJencHash(pSrcFileName, len);
	(rBuf = "SFSORTFINISHEVNT").CatChar('-').Cat(hash);
	return rBuf;
}

// static
int SLAPI SFile::Sort(const char * pSrcFileName_, const char * pOutFileName, CompFunc fcmp, size_t maxChunkSize, uint maxChunkCount)
{
	int    ok = 1;
	const  int use_mt = 1;
	const  uint max_chunk_size = (maxChunkSize >= (512*1024) && maxChunkSize <= 64*1024*1024) ? maxChunkSize : (8*1024*1024);
	const  uint max_chunk_count = (maxChunkCount >= 2 && maxChunkCount <= 64) ? maxChunkCount : 8;
	Evnt * p_ev_finish = 0;
	uint64 line_count = 0;
	const  SString src_file_name = pSrcFileName_;
	SString line_buf;
	SString temp_buf;
	SPathStruc ps;
	SFSortChunkInfoList chunk_info_list(src_file_name, max_chunk_count, fcmp);
	THROW(!isempty(src_file_name) && !isempty(pOutFileName));
	THROW(fileExists(src_file_name));
	{
		{
			//
			// Splitting
			//
			StrAssocArray chunk;
			StrAssocArray * p_chunk = 0;
			uint   chunk_line_no = 0;
			SFile f_src(src_file_name, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
			THROW(f_src.IsValid());
			{
				class SfSortSplitThread : public SlThread {
				public:
					struct InitBlock {
						InitBlock(const char * pSrcFileName, StrAssocArray * pPool, SFSortChunkInfo * pInfo, volatile int * pResult, ACount * pCntr)
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
						StrAssocArray * P_Pool;
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
				if(!use_mt) {
					p_chunk = &chunk;
				}
				else {
					THROW_S(p_chunk = new StrAssocArray, SLERR_NOMEM);
					THROW_S(p_ev_finish = new Evnt(_SfSortMakeFinishEvntName(src_file_name, temp_buf), Evnt::modeCreate), SLERR_NOMEM);
				}
				if(fcmp)
					p_chunk->SetTextCmpProc(fcmp);
				ACount thread_counter;
				volatile int thread_result = 1;
				while(f_src.ReadLine(line_buf)) {
					line_buf.Chomp();
					if(p_chunk->GetPoolDataLen() >= max_chunk_size) {
						SFSortChunkInfo * p_new_ci = chunk_info_list.CreateItem(0);
						THROW(p_new_ci);
						if(use_mt) {
							SfSortSplitThread::InitBlock ib(src_file_name, p_chunk, p_new_ci, &thread_result, &thread_counter);
							SfSortSplitThread * p_thread = new SfSortSplitThread(&ib);
							THROW_S(p_thread, SLERR_NOMEM);
							p_thread->Start(0);
							//
							THROW_S(p_chunk = new StrAssocArray, SLERR_NOMEM);
							if(fcmp)
								p_chunk->SetTextCmpProc(fcmp);
						}
						else {
							THROW(p_new_ci->Finish(src_file_name, *p_chunk));
						}
						chunk_line_no = 0;
					}
					p_chunk->AddFast(++chunk_line_no, line_buf);
				}
				if(p_chunk->getCount()) {
					SFSortChunkInfo * p_new_ci = chunk_info_list.CreateItem(0);
					THROW(p_new_ci);
					if(use_mt) {
						SfSortSplitThread::InitBlock ib(src_file_name, p_chunk, p_new_ci, &thread_result, &thread_counter);
						SfSortSplitThread * p_thread = new SfSortSplitThread(&ib);
						THROW_S(p_thread, SLERR_NOMEM);
						p_thread->Start(0);
					}
					else {
						THROW(p_new_ci->Finish(src_file_name, *p_chunk));
					}
					chunk_line_no = 0;
				}
				if(use_mt) {
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
			if(chunk_info_list.getCount() > 1) {
				chunk_info_list.Sort();
				THROW(chunk_info_list.Merge(0, chunk_info_list.getCount()-1));
				assert(chunk_info_list.getCount() == 1);
			}
			assert(oneof2(chunk_info_list.getCount(), 0, 1));
			if(chunk_info_list.getCount() == 1) {
				SFSortChunkInfo * p_item = chunk_info_list.at(0);
				SFile::Remove(pOutFileName);
				THROW(SFile::Rename(p_item->FileName, pOutFileName));
			}
		}
	}
	CATCHZOK
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
	const uint64 max_src_file_size = 48ULL*1024ULL*1024ULL/* *1024ULL */;
	SString test_file_name = MakeOutputFilePath("sfilesort_test.txt");
	SString temp_buf;
	SString line_buf;
	uint64 src_file_size = 0;
	{
		SFile f_test(test_file_name, SFile::mWrite|SFile::mNoStd);
		line_buf = 0;
		while(src_file_size < max_src_file_size) {
			const uint rn = SLS.GetTLA().Rg.GetUniformInt(3000000000UL);
			line_buf.Cat(rn).CR();
			if(line_buf.Len() > (1024*1024)) {
				THROW(f_test.WriteLine(line_buf));
				src_file_size += line_buf.Len();
				line_buf = 0;
			}
		}
		if(line_buf.NotEmpty()) {
			THROW(f_test.WriteLine(line_buf));
			src_file_size += line_buf.Len();
			line_buf = 0;
		}
	}
	{
		SString dest_file_name;
		{
			SPathStruc ps;
			ps.Split(test_file_name);
			ps.Nam.CatChar('-').Cat("sorted");
			ps.Merge(dest_file_name);
		}
		THROW(SFile::Sort(test_file_name, dest_file_name, /*PTR_CMPCFUNC(STRINT64_test)*/0, 1024*1024, 4));
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

#endif // } SLTEST_RUNNING

