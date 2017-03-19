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
			SString temp_buf;
			LineBuf = 0;
			ps.Split(pSrcFileName);
			ps.Nam.CatChar('-').Cat("temp").CatLongZ(ChunkNo, 8);
			ps.Merge(FileName);
			SFile f_temp(FileName, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
			THROW(f_temp.IsValid());
			for(uint i = 0; i < _c; i++) {
				temp_buf = rChunk.at_WithoutParent(i).Txt;
				if(i == 0)
					First = temp_buf;
				LineBuf.Cat(temp_buf.CRB());
				if(LineBuf.Len() >= accum_limit) {
					f_temp.WriteLine(LineBuf);
					LineBuf = 0;
				}
			}
			if(LineBuf.Len()) {
				f_temp.WriteLine(LineBuf);
				LineBuf = 0;
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
		THROW(P_RdStream == 0);
		THROW(fileExists(FileName));
		THROW_S(P_RdStream = new SFile(FileName, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd), SLERR_NOMEM);
		THROW(P_RdStream->IsValid());
		P_RdStream->ReadLine(LineBuf);
		LineBuf.Chomp();
		THROW(LineBuf == First);
		CurrentFlushIdx = 1;
		CATCHZOK
		return ok;
	}
	int    ShiftNext()
	{
		int    ok = 1;
		if(CurrentFlushIdx < LineCount) {
			assert(P_RdStream && P_RdStream->IsValid());
			P_RdStream->ReadLine(LineBuf);
			LineBuf.Chomp();
			First = LineBuf;
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
	SString LineBuf; // temporary
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

// static
int SLAPI SFile::Sort(const char * pSrcFileName, const char * pOutFileName, CompFunc fcmp, size_t maxChunkSize, uint maxChunkCount)
{
	int    ok = 1;
	const  uint max_chunk_size = (maxChunkSize >= (512*1024) && maxChunkSize <= 64*1024*1024) ? maxChunkSize : (8*1024*1024);
	const  uint max_chunk_count = (maxChunkCount >= 2 && maxChunkCount <= 64) ? maxChunkCount : 8;
	uint64 line_count = 0;
	StrAssocArray chunk;
	SString line_buf;
	SString temp_buf;
	SPathStruc ps;
	SFSortChunkInfoList chunk_info_list(pSrcFileName, max_chunk_count, fcmp);
	THROW(!isempty(pSrcFileName) && !isempty(pOutFileName));
	THROW(fileExists(pSrcFileName));
	{
		uint   chunk_line_no = 0;
		SFile f_src(pSrcFileName, SFile::mRead|SFile::mBinary|SFile::mNoStd|SFile::mBuffRd);
		THROW(f_src.IsValid());
		{
			if(fcmp)
				chunk.SetTextCmpProc(fcmp);
			while(f_src.ReadLine(line_buf)) {
				line_buf.Chomp();
				if(chunk.GetPoolDataLen() >= max_chunk_size) {
					SFSortChunkInfo * p_new_ci = chunk_info_list.CreateItem(0);
					THROW(p_new_ci);
					THROW(p_new_ci->Finish(pSrcFileName, chunk));
					chunk_line_no = 0;
				}
				chunk.AddFast(++chunk_line_no, line_buf);
			}
			if(chunk.getCount()) {
				SFSortChunkInfo * p_new_ci = chunk_info_list.CreateItem(0);
				THROW(p_new_ci);
				THROW(p_new_ci->Finish(pSrcFileName, chunk));
				chunk_line_no = 0;
			}
		}
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
	CATCHZOK
	return ok;
}

#if SLTEST_RUNNING // {

IMPL_CMPCFUNC(STRINT64_test, p1, p2)
{
	int64 v1 = atoll((const char *)p1);
	int64 v2 = atoll((const char *)p2);
	return CMPSIGN(v1, v2);
}

SLTEST_R(FileSort)
{
	int    ok = 1;
	const uint64 max_src_file_size = 32ULL*1024ULL*1024ULL/* *1024ULL */;
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
		THROW(SFile::Sort(test_file_name, dest_file_name, PTR_CMPCFUNC(STRINT64_test), 1024*1024, 4));
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

#endif // } SLTEST_RUNNING

