// PALMEXCH.CPP
// ..2009, 2010, 2012, 2015, 2016, 2018, 2019, 2020, 2021, 2022, 2023
//
#pragma hdrstop
#ifndef __GENERIC_MAIN_CONDUIT__
	#define SYNC_API //__declspec(dllimport)
	#include <pp.h>
#endif
#include <StyloConduit.h>
#include <commctrl.H>
// @v11.2.1 #include <condres.h>
// @v11.7.8 #include <ucl/ucl.h>

int ForceExportObsoleteData = 0; // @declared(StyloConduit.h)

#ifndef __GENERIC_MAIN_CONDUIT__
#pragma warn -stv

struct SyncFindDbByNameParams;
struct SyncDatabaseInfoType;

long  SyncFindDbByName(SyncFindDbByNameParams & rParam, SyncDatabaseInfoType & rInfo) { return -1; }
long  SyncOpenDB(const char * pName, int nCardNum, BYTE & rHandle, BYTE openMode) { return -1; }
long  SyncCloseDB(BYTE) { return -1; }
long  SyncDeleteDB(const char* pName, int nCardNum) { return -1; }
long  SyncCreateDB(CDbCreateDB& rDbStats) { return -1; }
long  SyncPurgeAllRecs(BYTE) { return -1; }
long  SyncReadRecordById(CRawRecordInfo& rInfo) { return -1; }
long  SyncReadRecordByIndex(CRawRecordInfo& rInfo) { return -1; }
long  SyncWriteRec(CRawRecordInfo & rInfo) { return -1; }
DWORD SyncHostToHHDWord(DWORD v) { return htonl(v); }
DWORD SyncHHToHostDWord(DWORD v) { return ntohl(v); }
WORD  SyncHostToHHWord(WORD v) { return htons(v); }
WORD  SyncHHToHostWord(WORD v) { return ntohs(v); }
long  SyncYieldCycles(WORD wMaxMiliSecs) { return -1; }
long  SyncReadUserID(CUserIDInfo &) { return -1; }
long  WINAPI UmGetUserDirectory(DWORD dwUserID, TCHAR * pUserDirBuffer, short * psUserDirBufSize) { return -1; }
#pragma warn .stv
#endif

char * convert_str(const char * pSrcBuf, int shrink, char * pDestBuf, size_t destBufLen)
{
	strnzcpy(pDestBuf, pSrcBuf, destBufLen);
	strip(pDestBuf);
	if(shrink) {
		strlwr866(pDestBuf);
		uchar * p = (uchar *)pDestBuf;
		for(; *p; p++) {
			if(*p == '\"')
				*p = '\'';
			if(*p == 241)
				*p = 165;
		}
		for(p = (uchar *)pDestBuf; *p;)
			if((p[0] == ' ' && p[1] == ' ') || p[0] == '\n')
				memmove(p, p+1, sstrlen((char *)(p+1)));
			else
				p++;
	}
	SOemToChar(pDestBuf);
	return pDestBuf;
}

char * get_str_from_dbfrec(DbfRecord * pRec, int fldN, int shrink, char * pDestBuf, size_t destBufLen)
{
	char temp_buf[256];
	temp_buf[0] = 0;
	pRec->get(fldN, temp_buf, sizeof(temp_buf));
	return convert_str(temp_buf, shrink, pDestBuf, destBufLen);
}

int32 dbltopalmintmoney(double v)
{
	return SyncHostToHHDWord(R0i(v * 100L));
}

double palmintmoneytodbl(int32 r)
{
	double sign = (r > 0) ? 1.00 : -1.00;
	//r = labs(r);
	double v = R2( ((double)(int32)SyncHHToHostDWord(r)) / 100. );
	//v *= sign;
	return v;
}

void WaitPercent(PROGRESSFN pFn, long iterN, long itersCount, const char * pMsg)
{
	if(pFn) {
		char text[256], temp[16];
		if(pMsg == 0) {
			temp[0] = 0;
			pMsg = temp;
		}
		sprintf(text, "%s %ld%%", pMsg, (100 * iterN) / itersCount);
		(*pFn)(text);
		SyncYieldCycles(1);
	}
}

int ScCheckDbfOpening(DbfTable * pTbl, const char * pLogFile)
{
	if(!pTbl->isOpened()) {
		char log_buf[512];
		sprintf(log_buf, "SPII FAIL: Error opening dbf file %s", pTbl->getName());
		SyncTable::LogMessage(pLogFile, log_buf);
		return 0;
	}
	else
		return 1;
}

int ScCreateDbfTable(DbfTable * pTbl, int numFlds, const DBFCreateFld * pFlds, const char * pLogFile)
{
	if(!pTbl->create(numFlds, pFlds)) {
		char log_buf[128];
		sprintf(log_buf, "SPII FAIL: Error creating dbf file %s", pTbl->getName());
		SyncTable::LogMessage(pLogFile, log_buf);
		return 0;
	}
	else
		return 1;
}

int ScAddDbfRec(DbfTable * pTbl, DbfRecord * pRec, const char * pLogFile)
{
	if(!pTbl->appendRec(pRec)) {
		char log_buf[256];
		sprintf(log_buf, "SPII FAIL: Error adding dbf rec %s", pTbl->getName());
		SyncTable::LogMessage(pLogFile, log_buf);
		return 0;
	}
	else
		return 1;
}
//
//
//
int SyncTable::Find(const char * pName, Stat * pStat, int fromPalmCompressedFile)
{
	int    ok = -1;
	if(fromPalmCompressedFile) {
		PalmArcHdr hdr;
		size_t rec_size = sizeof(hdr);
		P_Ctx->LastErr = SYNCERR_NOT_FOUND;
		MEMSZERO(hdr);
		if(ReadRecFromTempFile(&hdr, &rec_size) > 0) {
			if(!stricmp(pName, hdr.Name)) {
				hdr.NumRecs = SyncHHToHostDWord(hdr.NumRecs);
				hdr.Ver = SyncHHToHostWord(hdr.Ver);
				if(!hdr.Check()) {
					P_Ctx->PackedFileHdrOffs -= sizeof(uint16) + sizeof(hdr);
					LogMessage(P_Ctx->LogFile, "SPII FAIL: UnpackData(): Error arc header");
					P_Ctx->LastErr = 0;
					ok = 0;
				}
				else {
					pStat->NumRecs = hdr.NumRecs;
					ok = 1;
				}
			}
			if(ok <= 0)
				P_Ctx->PackedFileHdrOffs -= sizeof(uint16) + sizeof(hdr);
		}
	}
	else {
		if(P_Ctx->P_Pte) {
			SpiiDbHandler handle;
			SpiiTblStatParams params;
			THROW(ok = P_Ctx->P_Pte->TblFind(pName, handle));
			if(ok > 0) {
				if(pStat) {
					THROW(P_Ctx->P_Pte->GetTblStat(handle, &params));
					pStat->NumRecs = params.NumRecs;
				}
			}
			else {
				P_Ctx->LastErr = SYNCERR_NOT_FOUND;
				THROW(0);
			}
		}
		else {
			char   db_name[64];
			SyncFindDbByNameParams in_param;
			SyncDatabaseInfoType out_param;
			MEMSZERO(in_param);
			if(pStat)
				in_param.bOptFlags |= SYNC_DB_INFO_OPT_GET_SIZE;
			in_param.bOptFlags |= SYNC_DB_INFO_OPT_GET_ATTRIBUTES;
			in_param.dwCardNum = 0;
			in_param.pcDatabaseName = STRNSCPY(db_name, pName);
			MEMSZERO(out_param);
			THROW(!(P_Ctx->LastErr = SyncFindDbByName(in_param, out_param)));
			//LastErr = (!LastErr && out_param.baseInfo.m_Creator != 'SPII') ? SYNCERR_NOT_FOUND : LastErr;
			if(pStat) {
				pStat->NumRecs = out_param.dwNumRecords;
				pStat->TotalSize = out_param.dwTotalBytes;
				pStat->DataSize = out_param.dwDataBytes;
			}
			ok = 1;
		}
	}
	CATCH
		ok = (P_Ctx->LastErr == SYNCERR_NOT_FOUND) ? -1 : 0;
	ENDCATCH
	if(ok <= 0 && pStat)
		memzero(pStat, sizeof(*pStat));
	return ok;
}

int SyncTable::DeleteTable(const char * pName)
{
	int    ok = -1;
	if(pName) {
		if(P_Ctx->P_Pte) {
			P_Ctx->LastErr = 0;
			THROW(ok = P_Ctx->P_Pte->TblDelete(pName));
		}
		else {
			THROW(!(P_Ctx->LastErr = SyncDeleteDB(pName, 0)) || P_Ctx->LastErr == SYNCERR_NOT_FOUND);
			ok = 1;
		}
	}
	CATCH
		ok = 0;
		char log_buf[128];
		sprintf(log_buf, "SPII FAIL: SyncTable::%s(%s) error=0x%lx", "DeleteTable", pName, P_Ctx->LastErr);
		LogMessage(P_Ctx->LogFile, log_buf);
	ENDCATCH
	return ok;
}

SyncTable::SyncTable(int compress, int palmCompressed, SpiiExchgContext * pCtx) : P_Ctx(pCtx), 
	Compress(compress), PalmCompressed(palmCompressed), DefReopenDelta(2000)
{
	Handle[0] = 0;
	// @v10.2.12 @fix Handle[0] = 0;
	MEMSZERO(RecInfo);
	TblName[0] = 0;
}

SyncTable::~SyncTable()
{
	Close();
}

void SyncTable::LogError(const char * pFuncName)
{
	char log_buf[128];
	sprintf(log_buf, "SPII FAIL: SyncTable::%s(%s) error=0x%lx", pFuncName, TblName, P_Ctx->LastErr);
	LogMessage(P_Ctx->LogFile, log_buf);
}

/*static*/void SyncTable::LogMessage(const char * pFileName, const char * pMsg)
{
	int    r = 1;
	FILE * f = 0;
	SString file_name(pFileName);
	if(file_name.Len()) {
		f = fopen(file_name, "r");
		if(!f) {
			if(!createEmptyFile(file_name))
				r = 0;
		}
		else
			SFile::ZClose(&f);
	}
	else
		r = 0;
	if(r) {
		f = fopen(file_name, "a+");
		if(f) {
			SString msg(pMsg);
			SString s_dtm;
			SYSTEMTIME sys_dtm;
			GetLocalTime(&sys_dtm);
			s_dtm.Printf(" on %0.2d/%0.2d/%0.2d %0.2d:%0.2d:%0.2d \n", sys_dtm.wDay, sys_dtm.wMonth, sys_dtm.wYear, sys_dtm.wHour, sys_dtm.wMinute, sys_dtm.wSecond);
			msg.Cat((const char *)s_dtm);
			fputs(msg, f);
			fclose(f);
			DS.SetThreadNotification(PPSession::stntMessage, msg);
		}
	}
}

int SyncTable::Create(const char * pName)
{
	int    ok = 1;
	if(P_Ctx->P_Pte) {
		STRNSCPY(TblName, pName);
		THROW(P_Ctx->P_Pte->TblOpen(pName, SpiiTblOpenParams::omCreate, Handle));
	}
	else {
		STRNSCPY(TblName, pName);
		CDbCreateDB cr_blk;
		MEMSZERO(cr_blk);
		cr_blk.m_Creator = 'SPII';
		cr_blk.m_Flags = eRecord; // eRecord == 0
		STRNSCPY(cr_blk.m_Name, pName);
		cr_blk.m_Type = 'DATA';
		cr_blk.m_Version = 0;
		P_Ctx->LastErr = SyncCreateDB(cr_blk);
		Handle[0] = (long)cr_blk.m_FileHandle;
		RecInfo.m_FileHandle = (uint8)Handle[0];
		THROW(!P_Ctx->LastErr);
	}
	CATCH
		ok = 0;
		LogError("Create");
	ENDCATCH
	return ok;
}

int SyncTable::Open(const char * pName, long flags)
{
	int    ok = 1;
	NumRecs = 0;
	Close();
	STRNSCPY(TblName, pName);
	if(flags & oCreate) {
		int r = Find(pName, 0);
		if(r < 0) {
			ok = Create(pName);
			if(Compress)
				StartTableInTempFile();
		}
		else if(r > 0)
			ok = Open(pName, (flags & ~oCreate));
		else
			ok = 0;
	}
	else {
		if(P_Ctx->P_Pte) {
			long mode = flags & oReadOnly ? SpiiTblOpenParams::omRead : SpiiTblOpenParams::omReadWrite;
			ok = P_Ctx->P_Pte->TblOpen(pName, mode, Handle);
		}
		else {
			uint8  open_mode = 0, handle = 0;
			if(flags & oReadOnly)
				open_mode = (eDbRead|eDbShowSecret);
			else
				open_mode = (eDbRead|eDbWrite|eDbShowSecret);
			MEMSZERO(RecInfo);
			RecInfo.m_pBytes = 0;
			RecInfo.m_TotalBytes = 65530;
			RecInfo.m_dwReserved = 0;
			P_Ctx->LastErr = SyncOpenDB(pName, 0, handle, open_mode);
			RecInfo.m_FileHandle = handle;
			Handle[0] = handle;
			ok = P_Ctx->LastErr ? 0 : 1;
		}
		if(ok) {
			if(Compress)
				StartTableInTempFile();
		}
		else
			LogError("Open");
	}
	return ok;
}

int SyncTable::Close()
{
	int    ok = -1;
	if(Handle[0]) {
		if(Compress) {
			FinishTableInTempFile();
		}
		if(P_Ctx->P_Pte) {
			THROW(P_Ctx->P_Pte->TblClose(Handle));
		}
		else {
			P_Ctx->LastErr = SyncCloseDB((uint8)Handle[0]);
			THROW(!P_Ctx->LastErr);
		}
		Handle[0] = 0;
		Handle[1] = 0;
		ok = 1;
	}
	CATCH
		LogError("Close");
		ok = 0;
	ENDCATCH
	return ok;
}

// if delta < 0, then delta = SyncTable::DefReopenDelta
int SyncTable::Reopen(long delta, long recsCount)
{
	int    ok = -1;
	if(!Compress) {
		if(!P_Ctx->P_Pte) {
			if(delta < 0)
				delta = DefReopenDelta;
			if(delta > 0 && recsCount && (recsCount % delta) == 0) {
				char log_msg[256];
				sprintf(log_msg, "SPII INFO: table %s reopening on %ld recs", TblName, recsCount);
				LogMessage(P_Ctx->LogFile, log_msg);
				THROW(Close());
				THROW(Open(TblName, 0));
				ok = 1;
			}
		}
		else
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int SyncTable::ReadRecByIdx(uint16 idx, uint32 * pRecId, void * pBuf, size_t * pBufLen /* IN/OUT */)
{
	int    ok = 0;
	if(!PalmCompressed) {
		if(Handle[0]) {
			if(P_Ctx->P_Pte) {
				uint32 buf_len = *pBufLen;
				SpiiTblRecParams params;
				params.Pos = idx;
				THROW(P_Ctx->P_Pte->TblGetRec(Handle, &params, pBuf, &buf_len));
                ASSIGN_PTR(pRecId, params.RecID);
				*pBufLen = buf_len;
				ok = 1;
			}
			else {
				MEMSZERO(RecInfo);
				RecInfo.m_FileHandle = (uint8)Handle[0];
				RecInfo.m_pBytes = (BYTE *)pBuf;
				RecInfo.m_RecIndex = idx;
				RecInfo.m_TotalBytes = (WORD)*pBufLen;
				P_Ctx->LastErr = SyncReadRecordByIndex(RecInfo);
				THROW(!P_Ctx->LastErr);
				ASSIGN_PTR(pBufLen, static_cast<size_t>(RecInfo.m_RecSize));
				ASSIGN_PTR(pRecId, SyncHHToHostDWord(RecInfo.m_RecId));
				ok = 1;
			}
		}
	}
	else
		THROW(ok = ReadRecFromTempFile(pBuf, pBufLen));
	CATCH
		ok = 0;
		LogError("ReadRecByIdx");
	ENDCATCH
	return ok;
}

int SyncTable::ReadRecByID(uint32 recId, void * pBuf, size_t * pBufLen  /* IN/OUT */)
{
	int    ok = 0;
	if(Handle[0]) {
		if(P_Ctx->P_Pte) {
			uint32 buf_len = *pBufLen;
			SpiiTblRecParams params;
			params.RecID = recId;
			THROW(P_Ctx->P_Pte->TblGetRec(Handle, &params, pBuf, &buf_len));
			*pBufLen = buf_len;
			ok = 1;
		}
		else {
			MEMSZERO(RecInfo);
			RecInfo.m_FileHandle = static_cast<uint8>(Handle[0]);
			RecInfo.m_pBytes = static_cast<BYTE *>(pBuf);
			RecInfo.m_RecId = SyncHostToHHDWord(recId);
			RecInfo.m_TotalBytes = static_cast<WORD>(*pBufLen);
			P_Ctx->LastErr = SyncReadRecordById(RecInfo);
			THROW(!P_Ctx->LastErr);
			ASSIGN_PTR(pBufLen, static_cast<size_t>(RecInfo.m_RecSize));
			ok = 1;
		}
	}
	CATCH
		ok = 0;
		LogError("ReadRecByID");
	ENDCATCH
	return ok;
}

int SyncTable::StartTableInTempFile()
{
	FILE * f = fopen(P_Ctx->CompressFile, "r+b");
	if(f) {
		PalmArcHdr hdr;
		MEMSZERO(hdr);
		hdr.Ver = SyncHostToHHWord(1);
		STRNSCPY(hdr.Name, TblName);
		fseek(f, 0L, SEEK_END);
		TempFileHdrOffs = ftell(f);
		fwrite(&hdr, sizeof(hdr), 1, f);
		fclose(f);
		return 1;
	}
	else
		return SLS.SetError(SLERR_OPENFAULT, P_Ctx->CompressFile);
}

int SyncTable::FinishTableInTempFile()
{
	FILE * f = fopen(P_Ctx->CompressFile, "r+b");
	if(f) {
		PalmArcHdr hdr;
		MEMSZERO(hdr);
		fseek(f, TempFileHdrOffs, SEEK_SET);
		fread(&hdr, sizeof(hdr), 1, f);
		hdr.NumRecs = SyncHostToHHDWord(NumRecs);
		fseek(f, TempFileHdrOffs, SEEK_SET);
		fwrite(&hdr, sizeof(hdr), 1, f);
		fclose(f);
		return 1;
	}
	else
		return SLS.SetError(SLERR_OPENFAULT, P_Ctx->CompressFile);
}

int SyncTable::WriteRecToTempFile(void * pBuf, size_t bufLen)
{
	FILE * f = fopen(P_Ctx->CompressFile, "r+b");
	if(f) {
		uint16 palm_buf_len = SyncHostToHHWord((WORD)bufLen);
		fseek(f, 0L, SEEK_END);
		fwrite(&palm_buf_len, sizeof(palm_buf_len), 1, f);
		fwrite(pBuf, bufLen, 1, f);
		fclose(f);
		return 1;
	}
	else
		return SLS.SetError(SLERR_OPENFAULT, P_Ctx->CompressFile);
}

int SyncTable::ReadRecFromTempFile(void * pBuf, size_t * pBufLen)
{
	int    ok = 0;
	FILE * f = fopen(P_Ctx->PalmCompressedFile, "rb");
	if(f) {
		if(pBuf) {
			uint16 rec_size = 0;
			fseek(f, P_Ctx->PackedFileHdrOffs, SEEK_SET);
			fread(&rec_size, sizeof(rec_size), 1, f);
			rec_size = SyncHHToHostWord(rec_size);
			if(!pBufLen || rec_size && rec_size <= *pBufLen) {
				fread(pBuf, rec_size, 1, f);
				P_Ctx->PackedFileHdrOffs += sizeof(rec_size) + rec_size;
				ok = 1;
			}
		}
		fclose(f);
	}
	return ok;
}

int SyncTable::AddRec(uint32 * pRecId, void * pBuf, size_t bufLen)
{
	int    ok = 0;
	if(Handle[0]) {
		if(Compress) {
			if(WriteRecToTempFile(pBuf, bufLen)) {
				ASSIGN_PTR(pRecId, 0);
				ok = 1;
			}
		}
		else {
			if(P_Ctx->P_Pte) {
				SpiiTblRecParams params;
				THROW(P_Ctx->P_Pte->TblAddRec(Handle, &params, pBuf, bufLen));
				ASSIGN_PTR(pRecId, params.RecID);
			}
			else {
				MEMSZERO(RecInfo);
				RecInfo.m_FileHandle = (uint8)Handle[0];
				RecInfo.m_RecSize = (DWORD)bufLen;
				RecInfo.m_RecId = 0; // add new
				RecInfo.m_pBytes = (BYTE *)pBuf;
				RecInfo.m_TotalBytes = 65530;
				P_Ctx->LastErr = SyncWriteRec(RecInfo); // @checkerr
				THROW(!P_Ctx->LastErr);
				ASSIGN_PTR(pRecId, SyncHHToHostDWord(RecInfo.m_RecId));
			}
			ok = 1;
		}
	}
	CATCH
		ok = 0;
		LogError("AddRec");
	ENDCATCH
	if(ok)
		NumRecs++;
	return ok;
}

int SyncTable::UpdateRec(uint32 recId, void * pBuf, size_t bufLen)
{
	int    ok = 0;
	if(Handle[0]) {
		if(P_Ctx->P_Pte) {
			SpiiTblRecParams params;
			params.RecID = recId;
			THROW(P_Ctx->P_Pte->TblUpdRec(Handle, &params, pBuf, bufLen));
		}
		else {
			MEMSZERO(RecInfo);
			RecInfo.m_FileHandle = (uint8)Handle[0];
			RecInfo.m_RecSize = (DWORD)bufLen;
			RecInfo.m_RecId = SyncHostToHHDWord(recId);
			RecInfo.m_pBytes = (BYTE *)pBuf;
			RecInfo.m_TotalBytes = 65530;
			P_Ctx->LastErr = SyncWriteRec(RecInfo); // @checkerr
			THROW(!P_Ctx->LastErr);
		}
		ok = 1;
	}
	CATCH
		ok = 0;
		LogError("UpdateRec");
	ENDCATCH
	return ok;
}

int SyncTable::PurgeAll()
{
	int    ok = 0;
	if(Handle[0]) {
		if(P_Ctx->P_Pte) {
			THROW(P_Ctx->P_Pte->TblClose(Handle));
			THROW(P_Ctx->P_Pte->TblDelete(TblName));
			THROW(P_Ctx->P_Pte->TblOpen(TblName, SpiiTblOpenParams::omCreate, Handle));
		}
		else {
			P_Ctx->LastErr = SyncPurgeAllRecs((uint8)Handle[0]);
			THROW(!P_Ctx->LastErr);
		}
		ok = 1;
	}
	CATCH
		ok = 0;
		LogError("PurgeAll");
	ENDCATCH
	return ok;
}

#if 0 // @v11.7.8 {
static int AddCompressedRec(SyncTable * pTbl, uint32 ver, size_t bufSize, uint8 * pInBuf, size_t dataLen, uint8 * pOutBuf, FILE * fOut, FILE * fOut2)
{
	uint   compressed_size = bufSize;
	ucl_nrv2b_99_compress(pInBuf, dataLen, pOutBuf, &compressed_size, 0, 10, 0, 0);
	if(fOut) {
		if(ver > 500)
			fwrite(&compressed_size, sizeof(compressed_size), 1, fOut);
		fwrite(pOutBuf, compressed_size, 1, fOut);
	}
	if(ver <= 500)
		if(!pTbl->AddRec(0, pOutBuf, compressed_size))
			return 0;
	// @debug {
	if(fOut2) {
		uint _data_len = (uint)dataLen;
		memzero(pInBuf, _data_len);
		ucl_nrv2b_decompress_8(pOutBuf, compressed_size, pInBuf, &_data_len, 0);
		fwrite(pInBuf, _data_len, 1, fOut2);
	}
	// } @debug
	return 1;
}
#endif // } 0 @v11.7.8

/*static*/int SyncTable::ReceivePalmCompressedFile(PROGRESSFN pFn, SpiiExchgContext * pCtx)
{
	int    ok = -1;
	const  size_t blk_size = PALMPACKRECLEN;
	char * p_rec = 0;
	char   log_msg[128];
	SyncTable stbl(0, 0, pCtx);
	SyncTable::Stat stat;
	FILE * p_f = 0;

	if(stbl.Find(P_PalmPackedDataTblName, &stat) > 0 && stat.NumRecs > 0) {
		uint16 crecno = 0;
		uint16 recno = 0;
		SLibError = SLERR_OPENFAULT;
		THROW(p_f = fopen(pCtx->PalmCompressedFile, "wb"));
		THROW(stbl.Open(P_PalmPackedDataTblName, 0));
		THROW(p_rec = new char[blk_size]);
		for(crecno = 0; crecno < stat.NumRecs; crecno++) {
			size_t rec_size = blk_size;
			memzero(p_rec, blk_size);
			THROW(stbl.ReadRecByIdx(crecno, 0, p_rec, &rec_size) > 0);
			fwrite(p_rec, rec_size, 1, p_f);
			WaitPercent(pFn, crecno+1, stat.NumRecs, "Импорт сжатых данных");
		}
		{
			sprintf(log_msg, "SPII OK: %ld COMPRESSED DATA records imported", stat.NumRecs);
			LogMessage(pCtx->LogFile, log_msg);
		}
		ok = 1;
	}
	CATCH
		ok = 0;
		{
			sprintf(log_msg, "SPII ERR: COMPRESSED DATA import failed");
			LogMessage(pCtx->LogFile, log_msg);
		}
	ENDCATCH
	SFile::ZClose(&p_f);
	delete []p_rec;
	return ok;
}

#if 0 // @v11.7.8 {
/*static*/int SyncTable::TransmitCompressedFile(PROGRESSFN pFn, SpiiExchgContext * pCtx)
{
	const size_t InitBlockSize = 24*1024;
	int    ok = 1;
	//char   log_msg[128];
	SString wait_msg_buf;
	SString msg_buf;
	//char   out_file_name[MAX_PATH]; // @debug
	SString out_file_name;
	const  size_t blk_size = InitBlockSize; //PALMARCBUFSIZE;
	uint8 * p_blk = 0, * p_compressed_blk = 0;
	FILE * f = 0;
	FILE * f_out = 0;  // @debug
	FILE * f_out2 = 0; // @debug

	ucl_init();

	THROW_S_S(f = fopen(pCtx->CompressFile, "rb"), SLERR_OPENFAULT, pCtx->CompressFile);
	// @v10.5.6 replaceExt(STRNSCPY(out_file_name, pCtx->CompressFile), "out", 1);
	SPathStruc::ReplaceExt(out_file_name = pCtx->CompressFile, "out", 1); // @v10.5.6
	THROW_S_S(f_out = fopen(out_file_name, "wb"), SLERR_OPENFAULT, out_file_name);
	// @v10.5.6 replaceExt(STRNSCPY(out_file_name, pCtx->CompressFile), "chk", 1);
	SPathStruc::ReplaceExt(out_file_name = pCtx->CompressFile, "chk", 1); // @v10.5.6
	THROW_S_S(f_out2 = fopen(out_file_name, "wb"), SLERR_OPENFAULT, out_file_name);
	if(f) {
		TSArray <PalmArcHdr> hdr_list; // Список заголовков для хранения реального количества записей для каждой таблицы
		size_t p = 0;
		PalmArcHdr hdr;
		SyncTable stbl(0, 0, pCtx);
		THROW(stbl.DeleteTable(P_PalmArcTblName));
		THROW(stbl.Open(P_PalmArcTblName, SyncTable::oCreate));
		THROW_S(p_blk = new uint8[blk_size], SLERR_NOMEM);
		THROW_S(p_compressed_blk = new uint8[blk_size], SLERR_NOMEM);
		memzero(p_blk, blk_size);
		long   numrecs = 0, recno = 0;
		uint   current_hdr_pos = 0; // Текущая запись в hdr_list (+1)
		{
			current_hdr_pos = 0; // Текущая запись в hdr_list (+1)
			// Считаем количество записей, которые придется передать
			while(fread(&hdr, sizeof(hdr), 1, f) == 1) {
				{
					PalmArcHdr stored_hdr = hdr;
					stored_hdr.NumRecs = 0;
					hdr_list.insert(&stored_hdr);
					current_hdr_pos++;
				}
				if((p + sizeof(hdr)) > blk_size) {
					numrecs++;
					p = 0;
				}
				memcpy(p_blk+p, &hdr, sizeof(hdr));
				p += sizeof(hdr);
				const long num_recs = SyncHHToHostDWord(hdr.NumRecs);
				for(long i = 0; i < num_recs; i++) {
					uint16 palm_rec_size;
					THROW_S_S(fread(&palm_rec_size, sizeof(palm_rec_size), 1, f) == 1, SLERR_READFAULT, pCtx->CompressFile);
					uint16 rec_size = SyncHHToHostWord(palm_rec_size);
					if((rec_size+sizeof(palm_rec_size)) > blk_size) {
						//
						// Слишком большая запись - пропускаем ее (это - меньшее зло, чем переполнение буфера и краш)
						//
						fseek(f, (long)rec_size, SEEK_CUR);
						PPLoadText(PPTXT_SKIPTOOBIGRECORD, msg_buf);
                        msg_buf.Transf(CTRANSF_INNER_TO_OUTER).Space().Cat(hdr.Name).Space().Cat(i+1);
                        LogMessage(pCtx->LogFile, msg_buf); // @v10.3.0 @fix log_msg-->msg_buf
					}
					else {
						hdr_list.at(current_hdr_pos-1).NumRecs++;
						if((rec_size + sizeof(palm_rec_size) + p) > blk_size) {
							numrecs++;
							p = 0;
						}
						memcpy(p_blk+p, &palm_rec_size, sizeof(palm_rec_size));
						p += sizeof(palm_rec_size);
						THROW_S_S(fread(p_blk+p, rec_size, 1, f) == 1, SLERR_READFAULT, pCtx->CompressFile);
						p += rec_size;
					}
				}
			}
			if(p)
				numrecs++;
		}
		p = 0;
		memzero(p_blk, blk_size);
		rewind(f);
		PPLoadText(PPTXT_DATACOMPRESSION, wait_msg_buf);
		wait_msg_buf.Transf(CTRANSF_INNER_TO_OUTER);
		current_hdr_pos = 0; // Текущая запись в hdr_list (+1)
		while(fread(&hdr, sizeof(hdr), 1, f) == 1) {
			current_hdr_pos++;
			// @v9.1.1 Заносим в заголовок реальное количество записей (за минусом слишком длинных)
			hdr.NumRecs = SyncHostToHHDWord(hdr_list.at(current_hdr_pos-1).NumRecs);
			//
			if((p + sizeof(hdr)) > blk_size) {
				THROW(AddCompressedRec(&stbl, pCtx->PalmCfg.Ver, blk_size, p_blk, p, p_compressed_blk, f_out, f_out2));
				recno++;
				WaitPercent(pFn, recno, numrecs, wait_msg_buf);
				p = 0;
				memzero(p_blk, blk_size);
			}
			memcpy(p_blk+p, &hdr, sizeof(hdr));
			p += sizeof(hdr);
			const long num_recs = SyncHHToHostDWord(hdr.NumRecs);
			for(long i = 0; i < num_recs; i++) {
				uint16 palm_rec_size = 0;
				THROW_S_S(fread(&palm_rec_size, sizeof(palm_rec_size), 1, f) == 1, SLERR_READFAULT, pCtx->CompressFile);
				uint16 rec_size = SyncHHToHostWord(palm_rec_size);
				if((rec_size+sizeof(palm_rec_size)) > blk_size) {
					//
					// Слишком большая запись - пропускаем ее (это - меньшее зло, чем переполнение буфера и краш)
					//
					fseek(f, (long)rec_size, SEEK_CUR);
					// В журнал информация уже внесена выше при подсчете записей
				}
				else {
					if((rec_size + sizeof(palm_rec_size) + p) > blk_size) {
						THROW(AddCompressedRec(&stbl, pCtx->PalmCfg.Ver, blk_size, p_blk, p, p_compressed_blk, f_out, f_out2));
						recno++;
						WaitPercent(pFn, recno, numrecs, wait_msg_buf);
						p = 0;
						memzero(p_blk, blk_size);
					}
					memcpy(p_blk+p, &palm_rec_size, sizeof(palm_rec_size));
					p += sizeof(palm_rec_size);
					THROW_S_S(fread(p_blk+p, rec_size, 1, f) == 1, SLERR_READFAULT, pCtx->CompressFile);
					p += rec_size;
				}
			}
		}
		if(p) {
			THROW(AddCompressedRec(&stbl, pCtx->PalmCfg.Ver, blk_size, p_blk, p, p_compressed_blk, f_out, f_out2));
			recno++;
			WaitPercent(pFn, recno, numrecs, wait_msg_buf);
		}
		if(pCtx->PalmCfg.Ver > 500) {
			SFile::ZClose(&f_out);
			// @v10.5.6 replaceExt(STRNSCPY(out_file_name, pCtx->CompressFile), "out", 1);
			SPathStruc::ReplaceExt(out_file_name = pCtx->CompressFile, "out", 1); // @v10.5.6
			{
				long   recv_size = (pCtx->PalmCfg.RecvBufSize) ? pCtx->PalmCfg.RecvBufSize : InitBlockSize/*PALMARCBUFSIZE*/;
				long   tail_len = 0;
				SFileUtil::Stat fs;
				SFileUtil::GetStat(out_file_name, &fs);
				THROW_S_S(f_out = fopen(out_file_name, "rb"), SLERR_OPENFAULT, out_file_name);
				numrecs = (long)(fs.Size / (int64)recv_size);
				tail_len = (long)(fs.Size % (int64)recv_size);
				if(tail_len)
					numrecs++;
				PPLoadText(PPTXT_DATATRANSFTOPDA, wait_msg_buf);
				wait_msg_buf.Transf(CTRANSF_INNER_TO_OUTER);
				for(long i = 0; i < numrecs; i++) {
					long read_bytes = 0;
					if(i == numrecs - 1) {
						THROW_S_S(fread(p_blk, 1, tail_len, f_out) == tail_len, SLERR_READFAULT, pCtx->CompressFile);
						read_bytes = tail_len;
					}
					else {
						THROW_S_S(fread(p_blk, recv_size, 1, f_out) == 1, SLERR_READFAULT, pCtx->CompressFile);
						read_bytes = recv_size;
					}
					THROW(stbl.AddRec(0, p_blk, read_bytes));
					WaitPercent(pFn, i, numrecs, wait_msg_buf);
				}
			}
		}
		stbl.Close();
		{
			msg_buf.Printf("SPII OK: %ld COMPRESSED DATA records exported", numrecs);
			LogMessage(pCtx->LogFile, msg_buf);
		}
	}
	CATCH
		ok = 0;
		{
			msg_buf.Printf("SPII ERR: COMPRESSED DATA export failed");
			LogMessage(pCtx->LogFile, msg_buf);
		}
	ENDCATCH
	delete p_blk;
	delete p_compressed_blk;
	SFile::ZClose(&f_out);
	SFile::ZClose(&f_out2);
	SFile::ZClose(&f);
	return ok;
}
#endif // } 0 @v11.7.8
//
//
//
int SCDBObject::ExportIndex(PROGRESSFN pFn, const char * pDbName, SVector * pAry, CompFunc cf) // @v9.8.4 TSArray-->TSVector
{
	int    ok = 1;
	if(pDbName && pAry) {
		SString wait_fmt_buf, wait_msg_buf;
		uint   i;
		DWORD * p_buf = 0;
		PPLoadText(PPTXT_PDAINDEXEXPORT, wait_fmt_buf);
		wait_fmt_buf.Transf(CTRANSF_INNER_TO_OUTER);
		pAry->sort(cf);
		SyncTable stbl_idx(0, 0, P_Ctx);
		THROW(stbl_idx.Open(pDbName, SyncTable::oCreate));
		THROW(stbl_idx.PurgeAll());
		if(!P_Ctx->PalmCfg.CompressData()) {
			for(i = 0; pAry->enumItems(&i, (void **)&p_buf) > 0;) {
				DWORD uniq_id = *p_buf;//SyncHostToHHDWord(*(uint32 *)p_buf);
				THROW(stbl_idx.AddRec(0, &uniq_id, sizeof(uniq_id)));
				THROW(stbl_idx.Reopen(-1, i));
				{
					(wait_msg_buf = wait_fmt_buf).Space().Cat(pDbName);
					WaitPercent(pFn, i, pAry->getCount(), wait_msg_buf);
				}
			}
		}
		stbl_idx.Close();
	}
	CATCHZOK
	return ok;
}

int SCDBObject::ExportIndexes(PROGRESSFN pFn, SVector * pAry) // @v9.8.4 TSArray-->TSVector
{
	int    ok = -1;
	uint   count = 0;
	const  SCDBTblEntry * p_def = GetDefinition(&count);
	if(p_def && count > 1 && p_def[0].NumIndexes) {
		for(uint i = 0; ok && i < p_def[0].NumIndexes; i++) {
			const SCDBTblEntry & idx_def = p_def[i+1];
			if(!ExportIndex(pFn, idx_def.P_Name, pAry, idx_def.Cf))
				ok = 0;
		}
		ok = 1;
	}
	return ok;
}

int SCDBObject::InitTable(SyncTable * pTbl)
{
	int    ok = -1;
	uint   count = 0;
	const  SCDBTblEntry * p_def = GetDefinition(&count);
	if(p_def && count) {
		for(uint i = 0; i < count; i++)
			THROW(pTbl->DeleteTable(p_def[i].P_Name));
		THROW(pTbl->Open(p_def[0].P_Name, SyncTable::oCreate));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int AddToArray(SArray * pAry, uchar * pBuf, uint32 uniqID)
{
	if(pAry && pBuf) {
		char * p_buf = new char[pAry->getItemSize() + sizeof(uint32)];
		memcpy(p_buf, &uniqID, sizeof(uint32));
		memcpy(p_buf + sizeof(long), pBuf, pAry->getItemSize());
		pAry->insert(p_buf);
		delete p_buf;
	}
	return 1;
}
//
// PalmExchange
//
PalmTcpExchange::PalmTcpExchange(TcpSocket * pSo, const char * pSpiiPath, const char * pLogPath, const char * pTcpDevListPath, const char * pInstallPath)
{
	P_So = pSo;
	SpiiPath.CopyFrom(pSpiiPath);
	LogPath.CopyFrom(pLogPath);
	TcpDevListPath.CopyFrom(pTcpDevListPath);
	InstallPath.CopyFrom(pInstallPath);
}

PalmTcpExchange::~PalmTcpExchange()
{
}

int PalmTcpExchange::SpiiCmd(TcpSocket * pSo, const SpiiCmdBuf * pInBuf, const void * pInParam,
	SpiiCmdBuf * pOutBuf, void * pOutParam, const char * pCallingFuncName)
{
	int    ok = -1;
	if(pSo && pInBuf) {
		SString log_buf;
		SpiiCmdBuf in_buf = *pInBuf;
		SBuffer sbuf;
		in_buf.ToPalmRec();
		sbuf.Write(&in_buf, sizeof(SpiiCmdBuf));
		(log_buf = "Function").CatDiv(':', 2).Cat(pCallingFuncName);
		THROW(pSo->SendBuf(sbuf, 0));
		if(pInBuf->BufSize && pInParam) {
			size_t sended = 0;
			sbuf.Z();
			sbuf.Write(pInParam, pInBuf->BufSize);
			THROW(pSo->SendBuf(sbuf, &sended));
		}
		if(pOutBuf) {
			size_t rcv_bytes = 0;
			THROW(pSo->RecvBlock(pOutBuf, sizeof(SpiiCmdBuf), &rcv_bytes));
			pOutBuf->ToHostRec();
			if(pOutBuf->BufSize && pOutParam) {
				THROW(pSo->RecvBlock(pOutParam, pOutBuf->BufSize, &rcv_bytes));
			}
			ok = pOutBuf->RetCode;
		}
		else
			ok = 1;
	}
	CATCHZOK
	return ok;
}

int PalmTcpExchange::TblOpen(const char * pTblName, ulong mode, SpiiDbHandler pH)
{
	int    ok = 1;
	SpiiCmdBuf in_buf, out_buf;
	SpiiTblOpenParams params;
	in_buf.Cmd     = SpiiCmdBuf::cmOpen;
	in_buf.BufSize = sizeof(params);
	STRNSCPY(params.TblName, pTblName);
	params.Mode = mode;
	params.ToPalmRec();
	THROW(SpiiCmd(P_So, &in_buf, &params, &out_buf, 0, "TblOpen") > 0);
	pH[0] = out_buf.Hdl[0];
	pH[1] = out_buf.Hdl[1];
	CATCHZOK
	return ok;
}

int PalmTcpExchange::TblFind(const char * pTblName, SpiiDbHandler pH)
{
	int    ok = 1;
	SpiiCmdBuf in_buf, out_buf;
	SpiiTblOpenParams params;
	in_buf.Cmd     = SpiiCmdBuf::cmFindTbl;
	in_buf.BufSize = sizeof(params);
	STRNSCPY(params.TblName, pTblName);
	THROW(ok = SpiiCmd(P_So, &in_buf, &params, &out_buf, 0, "TblFind"));
	if(ok > 0) {
		pH[0] = out_buf.Hdl[0];
		pH[1] = out_buf.Hdl[1];
	}
	CATCHZOK
	return ok;
}

int PalmTcpExchange::TblClose(SpiiDbHandler h)
{
	SpiiCmdBuf in_buf, out_buf;
	in_buf.Cmd = SpiiCmdBuf::cmCloseTbl;
	in_buf.Hdl[0] = h[0];
	in_buf.Hdl[1] = h[1];
	return SpiiCmd(P_So, &in_buf, 0, &out_buf, 0, "TblClose");
}

int PalmTcpExchange::GetTblStat(SpiiDbHandler h, SpiiTblStatParams * pParams)
{
	int    ok = 1;
	SpiiTblStatParams stat = *pParams;
	SpiiCmdBuf in_buf, out_buf;
	in_buf.Cmd = SpiiCmdBuf::cmGetStat;
	in_buf.Hdl[0] = h[0];
	in_buf.Hdl[1] = h[1];
	THROW(SpiiCmd(P_So, &in_buf, 0, &out_buf, &stat, "GetTblStat"));
	stat.ToHostRec();
	ASSIGN_PTR(pParams, stat);
	CATCHZOK
	return ok;
}

int PalmTcpExchange::TblGetPos(SpiiDbHandler h, ulong * pPos) { return -1; }
int PalmTcpExchange::TblSetPos(SpiiDbHandler h, ulong pos) { return -1; }

int PalmTcpExchange::TblAddRec(SpiiDbHandler h, SpiiTblRecParams * pParams, const void * pRec, uint32 bufSize)
{
	SpiiCmdBuf in_buf, out_buf;
	in_buf.Cmd     = SpiiCmdBuf::cmAddRec;
	in_buf.Hdl[0]  = h[0];
	in_buf.Hdl[1]  = h[1];
	in_buf.BufSize = bufSize;
	return SpiiCmd(P_So, &in_buf, pRec, &out_buf, pParams, "TblAddRec") ? (pParams->ToHostRec(), 1) : 0;
}

int PalmTcpExchange::TblUpdRec(SpiiDbHandler h, SpiiTblRecParams * pParams, const void * pBuf, uint32 bufSize)
{
	int    ok = -1;
	if(pBuf && bufSize) {
		size_t params_size = sizeof(SpiiTblRecParams);
		SpiiCmdBuf in_buf, out_buf;
		SpiiTblRecParams params = *pParams;
		STempBuffer temp_buf(params_size + bufSize);
		in_buf.Cmd     = SpiiCmdBuf::cmUpdRec;
		in_buf.Hdl[0]  = h[0];
		in_buf.Hdl[1]  = h[1];
		in_buf.BufSize = params_size + bufSize;
		params.ToPalmRec();
		memcpy(temp_buf, &params, params_size);
		memcpy(temp_buf + params_size, pBuf, bufSize);
		THROW(SpiiCmd(P_So, &in_buf, temp_buf, &out_buf, 0, "TblUpdRec"));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PalmTcpExchange::TblDelRec(SpiiDbHandler h, SpiiTblRecParams * pParams)
{
	SpiiCmdBuf in_buf, out_buf;
	SpiiTblRecParams params = *pParams;
	in_buf.Cmd     = SpiiCmdBuf::cmDelRec;
	in_buf.Hdl[0]  = h[0];
	in_buf.Hdl[1]  = h[1];
	params.ToPalmRec();
	return SpiiCmd(P_So, &in_buf, &params, &out_buf, 0, "TblDelRec");
}

int PalmTcpExchange::TblGetRec(SpiiDbHandler h, SpiiTblRecParams * pParams, void * pRec, uint32 * pBufSize)
{
	int    ok = 1;
	size_t params_size = sizeof(SpiiTblRecParams);
	char * p_rec = new char[PALMPACKRECLEN + params_size];
	SpiiCmdBuf in_buf, out_buf;
	SpiiTblRecParams params = *pParams;
	THROW_S(p_rec, SLERR_NOMEM);
	in_buf.Cmd     = SpiiCmdBuf::cmGetRec;
	in_buf.Hdl[0]  = h[0];
	in_buf.Hdl[1]  = h[1];
	in_buf.BufSize = params_size;
	params.ToPalmRec();
	THROW(SpiiCmd(P_So, &in_buf, &params, &out_buf, p_rec, "TblGetRec"));
	memcpy(&params, p_rec, params_size);
	params.ToHostRec();
	THROW_S(!pBufSize || *pBufSize >= (out_buf.BufSize - params_size), SLERR_BUFTOOSMALL);
	memcpy(pRec, p_rec + params_size, out_buf.BufSize - params_size);
	ASSIGN_PTR(pParams, params);
	ASSIGN_PTR(pBufSize, out_buf.BufSize - params_size);
	CATCHZOK
	delete [] p_rec;
	return ok;
}

int PalmTcpExchange::TblGetTbl(SpiiDbHandler h, void *, uint32 * pBufSize) { return -1; }
int PalmTcpExchange::TblSetTbl(SpiiDbHandler h, const void *, uint32 bufSize) { return -1; }
int PalmTcpExchange::TblPurgeAllRecs(SpiiDbHandler h) { return -1; }

int PalmTcpExchange::TblDelete(const char * pTblName)
{
	SpiiCmdBuf in_buf, out_buf;
	SpiiTblDelParams params;
	in_buf.Cmd     = SpiiCmdBuf::cmDelTbl;
	in_buf.BufSize = sizeof(SpiiTblDelParams);
	STRNSCPY(params.TblName, pTblName);
	return SpiiCmd(P_So, &in_buf, &params, &out_buf, 0, "TblDelete");
}

int PalmTcpExchange::GetDeviceInfo(SpiiDeviceInfoParams * pParams)
{
	int    ok = 1;
	SpiiCmdBuf in_buf, out_buf;
	SpiiDeviceInfoParams dev_info;
	in_buf.Cmd = SpiiCmdBuf::cmGetDevInfo;
	THROW(SpiiCmd(P_So, &in_buf, 0, &out_buf, &dev_info, "GetDeviceInfo"));
	dev_info.ToHostRec();
	ASSIGN_PTR(pParams, dev_info);
	CATCHZOK
	return ok;
}

int PalmTcpExchange::GetProgramVer(uint32 * pVer)
{
	int    ok = 1;
	SpiiCmdBuf in_buf, out_buf;
	SpiiProgramVerParams pgm_ver;
	in_buf.Cmd = SpiiCmdBuf::cmGetProgramVer;
	THROW(SpiiCmd(P_So, &in_buf, 0, &out_buf, &pgm_ver, "GetProgramVer"));
	pgm_ver.ToHostRec();
	ASSIGN_PTR(pVer, pgm_ver.Ver);
	CATCHZOK
	return ok;
}

int PalmTcpExchange::QuitSess()
{
	SpiiCmdBuf in_buf;
	in_buf.Cmd = SpiiCmdBuf::cmQuitSess;
	SpiiCmd(P_So, &in_buf, 0, 0, 0, "QuitSess");
	return 1;
}

int PalmTcpExchange::GetSpiiPath(SString & aPath)
{
	aPath = SpiiPath;
	return 1;
}

int PalmTcpExchange::GetLogPath(SString & aPath)
{
	aPath = LogPath;
	return 1;
}

int PalmTcpExchange::GetInstallPath(SString & rPath)
{
	rPath = InstallPath;
	return 1;
}

SString & PalmTcpExchange::GetTcpDevListPath(SString & rPath)
{
	return (rPath = TcpDevListPath);
}

int PalmTcpExchange::LogTrafficInfo(const char * pDevice)
{
	int    ok = 1, r = 1, is_new = 0;
	FILE * f = 0;
	SString path, msg, s_dtm;
	THROW(pDevice && P_So);
	GetLogPath(path);
	path.SetLastSlash().Cat("palmexch.log");
	f = fopen(path, "r");
	if(!f) {
		is_new = 1;
		THROW_S_S(f = fopen(path, "w"), SLERR_OPENFAULT, path);
	}
	SFile::ZClose(&f);
	THROW_S_S(f = fopen(path, "a+"), SLERR_OPENFAULT, path);
	{
		long rdbytes = 0, wrbytes = 0;
		long total_bytes = 0;
		SYSTEMTIME sys_dtm;
		P_So->GetStat(&rdbytes, &wrbytes);
		total_bytes = rdbytes + wrbytes;
		GetLocalTime(&sys_dtm);
		if(is_new)
			fputs("Device;Date;Time;Received bytes;Sended bytes;Total bytes\n", f);
		s_dtm.Printf("%0.2d/%0.2d/%0.2d;%0.2d:%0.2d:%0.2d", sys_dtm.wDay, sys_dtm.wMonth, sys_dtm.wYear, sys_dtm.wHour, sys_dtm.wMinute, sys_dtm.wSecond);
		msg.Printf("%s;%s;%ld;%ld;%ld\n", pDevice, s_dtm.cptr(), rdbytes, wrbytes, total_bytes);
		fputs(msg, f);
		fclose(f);
	}
	CATCHZOK
	return ok;
}

/*static*/int PalmTcpExchange::LogTrafficInfo(const char * pPath, const char * pLogPath, const char * pDevice, int isReadBytes)
{
	int    ok = 1, r = 1, is_new = 0;
	FILE * f = 0;
	SString path, msg, s_dtm;
	THROW(pDevice && pPath && pLogPath);
	path = pLogPath;
	path.SetLastSlash().Cat("palmexch.log");
	f = fopen(path, "r");
	if(!f) {
		is_new = 1;
		THROW_S_S(f = fopen(path, "w"), SLERR_OPENFAULT, path);
	}
	SFile::ZClose(&f);
	THROW_S_S(f = fopen(path, "a+"), SLERR_OPENFAULT, path);
	{
		long rdbytes = 0, wrbytes = 0;
		long total_bytes = 0;
		int64 file_size = 0;
		SYSTEMTIME sys_dtm;
		SFile in_file(pPath, SFile::mRead);

		in_file.CalcSize(&file_size);
		in_file.Close();
		total_bytes = (long)file_size;
		GetLocalTime(&sys_dtm);
		if(isReadBytes)
			rdbytes = total_bytes;
		else
			wrbytes = total_bytes;
		if(is_new)
			fputs("Device;Date;Time;Received bytes;Sended bytes;Total bytes\n", f);
		s_dtm.Printf("%0.2d/%0.2d/%0.2d;%0.2d:%0.2d:%0.2d", sys_dtm.wDay, sys_dtm.wMonth, sys_dtm.wYear, sys_dtm.wHour, sys_dtm.wMinute, sys_dtm.wSecond);
		msg.Printf("%s;%s;%ld;%ld;%ld\n", pDevice, s_dtm.cptr(), rdbytes, wrbytes, total_bytes);
		fputs(msg, f);
		fclose(f);
	}
	CATCHZOK
	return ok;
}

static int GetRootPath(SString & aPath, PalmTcpExchange * pTcpExch, CSyncProperties * pProps)
{
	int    ok = 1;
	if(pTcpExch) {
		SString root_path;
		SpiiDeviceInfoParams dev_info;
		pTcpExch->GetSpiiPath(root_path);
		SLibError = SLERR_NOFOUND;
		THROW(root_path.Len());
		THROW(pTcpExch->GetDeviceInfo(&dev_info));
		root_path.SetLastSlash().Cat(dev_info.Name).SetLastSlash().Cat("SPII");
		aPath = root_path;
	}
	else
		aPath.CopyFrom(pProps->m_PathName);
	CATCHZOK
	return ok;
}

static int __CopyFile(const char * pSrcPath, const char * pDestPath, SpiiExchgContext & rCtx)
{
	int    ok = 1;
	SString msg_buf;
	if(fileExists(pSrcPath)) {
		int    r = 0;
		if(fileExists(pDestPath)) {
			uint j = 0;
			while(!r && j < 30) {
				if(SFile::Remove(pDestPath))
					r = 1;
				else {
					++j;
					SDelay(1000);
				}
			}
			if(j) {
				if(!r) {
					msg_buf.Z().Cat("ERR").CatDiv(':', 2).Cat("timeout for coping was expired").Space().
						CatQStr(pSrcPath).CatDiv('>', 1).CatQStr(pDestPath);
					SyncTable::LogMessage(rCtx.LogFile, msg_buf);
				}
				else {
					msg_buf.Z().Cat("WARN").CatDiv(':', 2).Cat("was delay").Space().Cat(j).Cat("sec").Space().Cat("on coping").
						Space().CatQStr(pSrcPath).CatDiv('>', 1).CatQStr(pDestPath);
					SyncTable::LogMessage(rCtx.LogFile, msg_buf);
				}
			}
		}
		else
			r = 1;
		if(r) {
			if(!SCopyFile(pSrcPath, pDestPath, 0, FILE_SHARE_READ, 0)) {
				msg_buf.Z().Cat("ERR").CatDiv(':', 2).Cat("coping").Space().CatQStr(pSrcPath).CatDiv('>', 1).CatQStr(pDestPath);
				SyncTable::LogMessage(rCtx.LogFile, msg_buf);
				ok = 0;
			}
		}
	}
	else
		ok = -1;
	return ok;
}

static int CopyFiles(const char * pSrcDir, const char * pDestDir, int inFiles, SpiiExchgContext & rCtx)
{
	int    ok = 1;
	if(pSrcDir && pDestDir) {
		const char * p_in_file_names[] = {
			"sp_goods.dbf",
			"sp_brand.dbf",
			"sp_loc.dbf",
			"sp_ggrp.dbf",
			"sp_quotk.dbf",
			"sp_cli.dbf",
			"sp_cliad.dbf",
			"sp_clidb.dbf",
			"sp_sell.dbf",
			"sp_todo.dbf",
			"palmcfg.bin"
		};
		const char * p_out_file_names[] = {
			"sp_bill.dbf",
			"sp_bitem.dbf",
			"sp_invh.dbf",
			"sp_invl.dbf",
			"sp_todo.dbf",
			"sp_ready"
		};
		SString src_path, dest_path;
		uint i;
		if(inFiles) {
			for(i = 0; i < SIZEOFARRAY(p_in_file_names); i++) {
				const char * p_sub_dir = "IN";
				const char * p_fname = p_in_file_names[i];
				(src_path = pSrcDir).SetLastSlash().Cat(p_sub_dir).SetLastSlash().Cat(p_fname);
				(dest_path = pDestDir).SetLastSlash().Cat(p_sub_dir).SetLastSlash().Cat(p_fname);
				__CopyFile(src_path, dest_path, rCtx);
			}
		}
		for(i = 0; i < SIZEOFARRAY(p_out_file_names); i++) {
			const char * p_sub_dir = "OUT";
			const char * p_fname = p_out_file_names[i];
			(src_path = pSrcDir).SetLastSlash().Cat(p_sub_dir).SetLastSlash().Cat(p_fname);
			(dest_path = pDestDir).SetLastSlash().Cat(p_sub_dir).SetLastSlash().Cat(p_fname);
			__CopyFile(src_path, dest_path, rCtx);
		}
	}
	else
		ok = -1;
	return ok;
}

#define TCP_ALLOW_ALL "__ALL__"

int SpiiExchange(PalmTcpExchange * pTcpExch, PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = -1;
	int    sess_quited = 0;
	long   job_info_id = 0;
	SString root_path, temp_path, temp_fname, msg_buf;
	SpiiExchgContext ctx(pTcpExch);
	if(pTcpExch || pFn) {
		SString exp_path, imp_path, device;
		SpiiDeviceInfoParams dev_info;
		if(pTcpExch) {
			THROW(pTcpExch->GetDeviceInfo(&dev_info));
			device = dev_info.Name;
		}
		(msg_buf = "StyloPalm Exchange").CatDiv(':', 2).Cat(dev_info.Name);
		DS.SetThreadNotification(PPSession::stntText, msg_buf);
		THROW(GetRootPath(root_path, pTcpExch, pProps));
		ctx.LogFile = root_path;
		ctx.LogFile.SetLastSlash().Cat("stylo.Log");
		SyncTable::LogMessage(ctx.LogFile, "\n\nSPII Start:");
		if(pTcpExch) {
			SyncTable::LogMessage(ctx.LogFile, "Exchange with ppy server");
			//
			// Проверяем, содержится-ли имя данного устройства в списке устройств, которым разрешен обмен с сервером
			//
			int found = 0;
			SFile devl_file;
			SString devl_path, allow_device;
			devl_file.Open(devl_path = pTcpExch->GetTcpDevListPath(devl_path), SFile::mRead);
			if(devl_file.IsValid()) {
				while(!found && devl_file.ReadLine(allow_device, SFile::rlfChomp|SFile::rlfStrip) > 0) {
					if(allow_device.CmpNC(TCP_ALLOW_ALL) == 0 || allow_device.CmpNC(dev_info.Name) == 0)
						found = 1;
				}
			}
			else
				found = 1;
			if(found == 0)
				SyncTable::LogMessage(ctx.LogFile, "ERR: Access denied");
			THROW(found);
		}
		root_path.SetLastSlash();
		::createDir((exp_path = root_path).Cat("IN"));
		::createDir((imp_path = root_path).Cat("OUT"));
		temp_path = root_path;
		if(pTcpExch) {
			temp_path.Cat("temp").SetLastSlash();
			MakeTempFileName(temp_path, 0, 0, 0, temp_fname);
			(temp_path = temp_fname).TrimRightChr('.').SetLastSlash();
			::createDir((exp_path = temp_path).Cat("IN"));
			::createDir((imp_path = temp_path).Cat("OUT"));
		}
#if 0 // @v11.2.1 {
		if(!pTcpExch || CopyFiles(root_path, temp_path, 1, ctx) > 0) {
			SCDBObjConfig obj_cfg(&ctx);
			THROW(obj_cfg.Init(exp_path, imp_path));
			if(obj_cfg.GetPalmConfig()->CompressData()) {
				SFile::Remove((temp_fname = exp_path).SetLastSlash().Cat("temp.dat"));
				FILE * f = fopen(temp_fname, "w");
				if(f) {
					fclose(f);
					ctx.CompressFile = temp_fname;
				}
			}
			ctx.TransmitComprFile = 0;
			if(obj_cfg.GetPalmConfig()->PalmCompressedData()) {
				(temp_fname = imp_path).SetLastSlash().Cat("temp.dat");
				SFile::Remove(temp_fname);
				ctx.PalmCompressedFile = temp_fname;
				SyncTable::ReceivePalmCompressedFile(pFn, &ctx);
			}
			{
				SCDBObjToDo obj_todo(&ctx);
				if(obj_todo.Init(exp_path, imp_path)) {
					THROW(obj_todo.Import(pFn, pProps));
					THROW(obj_todo.Export(pFn, pProps));
				}
			}
			{
				CSyncProperties prop;
				temp_path.CopyTo(prop.m_PathName, BIG_PATH);
				SCDBObjOrder obj_order(&ctx);
				if(obj_order.Init(exp_path, imp_path))
					THROW(obj_order.Import(pFn, &prop));
			}
			{
				SCDBObjCliInv obj_inv(&ctx);
				if(obj_inv.Init(exp_path, imp_path))
					THROW(obj_inv.Import(pFn, pProps));
			}
			{
				SCDBObjClient obj_client(&ctx);
				if(obj_client.Init(exp_path, imp_path))
					THROW(obj_client.Export(pFn, pProps));
			}
			{
				SCDBObjClientDebt obj_clidebt(&ctx);
				if(obj_clidebt.Init(exp_path, imp_path))
					THROW(obj_clidebt.Export(pFn, pProps));
			}
			{
				SCDBObjGoodsGrp obj_ggrp(&ctx);
				if(obj_ggrp.Init(exp_path, imp_path))
					THROW(obj_ggrp.Export(pFn, pProps));
			}
			{
				SCDBObjGoods obj_goods(&ctx);
				if(obj_goods.Init(exp_path, imp_path))
					THROW(obj_goods.Export(pFn, pProps));
			}
			{
				SCDBObjBrand obj_brand(&ctx);
				if(obj_brand.Init(exp_path, imp_path))
					THROW(obj_brand.Export(pFn, pProps));
			}
			{
				SCDBObjLoc obj_loc(&ctx);
				if(obj_loc.Init(exp_path, imp_path))
					THROW(obj_loc.Export(pFn, pProps));
			}
			{
				SCDBObjSell obj_sell(&ctx);
				if(obj_sell.Init(exp_path, imp_path))
					THROW(obj_sell.Export(pFn, pProps));
			}
			//
			// Обновление программы StyloWce на кпк. Только для устройств на базе ОС Windows Mobile
			//
			{
				SCDBObjProgram obj_pgm(&ctx);
				if(obj_pgm.Init())
					THROW(obj_pgm.Export(pFn, pProps));
			}
			if(ctx.TransmitComprFile)
				THROW(SyncTable::TransmitCompressedFile(pFn, &ctx));
			if(obj_cfg.GetPalmConfig()->PalmCompressedData()) {
				SyncTable stbl(0, 0, &ctx);
				THROW(stbl.DeleteTable(P_PalmPackedDataTblName));
			}
			THROW(obj_cfg.Export(pFn, pProps));
		}
#endif // } 0 @v11.2.1
		if(pTcpExch) {
			pTcpExch->QuitSess();
			sess_quited = 1;
			CopyFiles(temp_path, root_path, 0, ctx);
			pTcpExch->LogTrafficInfo(device);
		}
		ok = 1;
	}
	CATCH
		if(!sess_quited)
			SyncTable::LogMessage(ctx.LogFile, "SPII ERR: Orders not saved");
		ok = 0;
	ENDCATCH
	if(pTcpExch) {
#if 0 // { Если сессия обмена не завершена, значит где-то произошла ошибка. Команду о завершении сессии не будем передавать на КПК, а просто оборвем соединение
      // чтобы КПК выдал сообщение об ошибке
		if(!sess_quited)
			pTcpExch->QuitSess();
#endif // } 0
		RemoveDir(temp_path);
		(msg_buf = "Directory deleted").CatDiv(':', 2).Cat(temp_path).Space().Colon();
		SyncTable::LogMessage(ctx.LogFile, msg_buf);
	}
	SyncTable::LogMessage(ctx.LogFile, "SPII Finish:");
	return ok;
}
