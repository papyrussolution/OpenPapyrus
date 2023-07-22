// SC_GGRP.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2015, 2016, 2019
// Part of StyloConduit project
// Ёкспорт/»мпорт товарных групп
//
#include <slib-internal.h>
#pragma hdrstop
#include "StyloConduit.h"
//#include <stddef.h>

IMPL_CMPFUNC(GGROUPID, i1, i2)
{
	const SCDBObjGoodsGrp::IdxRec * gg1 = static_cast<const SCDBObjGoodsGrp::IdxRec *>(i1);
	const SCDBObjGoodsGrp::IdxRec * gg2 = static_cast<const SCDBObjGoodsGrp::IdxRec *>(i2);
	if(gg1->ID < gg2->ID)
		return -1;
	else if(gg1->ID > gg2->ID)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(GGROUPNAM, i1, i2)
{
	int r = 0;
	const SCDBObjGoodsGrp::IdxRec * gg1 = static_cast<const SCDBObjGoodsGrp::IdxRec *>(i1);
	const SCDBObjGoodsGrp::IdxRec * gg2 = static_cast<const SCDBObjGoodsGrp::IdxRec *>(i2);
	if((r = stricmp866(gg1->Name, gg2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

SCDBObjGoodsGrp::SCDBObjGoodsGrp(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_GGrpTbl(0)
{
}

SCDBObjGoodsGrp::~SCDBObjGoodsGrp()
{
	delete P_GGrpTbl;
}

const SCDBTblEntry * SCDBObjGoodsGrp::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{"GoodsGrp.tbl", 2, offsetof(PalmConfig, TmGoods), 0, 0},
		{"GroupID.idx",  0, 0, 0, PTR_CMPFUNC(GGROUPID)},
		{"GroupNam.idx", 0, 0, 0, PTR_CMPFUNC(GGROUPNAM)}
	};
	ASSIGN_PTR(pEntryCount, 3);
	return def;
}

int SCDBObjGoodsGrp::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAX_PATH], fname[MAX_PATH];
	setLastSlash(STRNSCPY(path, pExpPath));

	P_GGrpTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_ggrp.dbf"));
	return ok;
}

SCDBObjGoodsGrp::PalmRec * SCDBObjGoodsGrp::AllocPalmRec(size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec);
	PalmRec * p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjGoodsGrp::PalmRec156 * SCDBObjGoodsGrp::AllocPalmRec156(size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec156);
	PalmRec156 * p_buf = (PalmRec156 *)SAlloc::C(1, buf_len);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjGoodsGrp::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	union {
		PalmRec * p_out_buf;
		PalmRec156 * p_out_buf156;
	} b;
	if(ver < 156)
		b.p_out_buf = 0;
	else
		b.p_out_buf156 = 0;
	if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmGoods, P_Ctx->HostCfg.TmGoods) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: GOODSGROUP base on palm is recently that host");
		ok = -1;
	}
	else if(P_GGrpTbl && P_GGrpTbl->getNumRecs() && P_GGrpTbl->top()) {
		long   recno = 0, numrecs = P_GGrpTbl->getNumRecs();
		SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
		THROW(InitTable(&stbl));
		do {
			size_t buf_len = 0;
			long   id = 0;
			IdxRec idx_rec;
			MEMSZERO(idx_rec);
			int    fldn_id = 0;
			int    fldn_name = 0;
			int    fldn_pid = 0;
			P_GGrpTbl->getFieldNumber("ID",      &fldn_id);
			P_GGrpTbl->getFieldNumber("NAME",    &fldn_name);
			P_GGrpTbl->getFieldNumber("PARENTID", &fldn_pid);

			DbfRecord rec(P_GGrpTbl);
			P_GGrpTbl->getRec(&rec);
			rec.get(fldn_id, id);
			if(ver < 156) {
				b.p_out_buf = AllocPalmRec(&buf_len); // @checkerr
				b.p_out_buf->ID = SyncHostToHHDWord(id);
			}
			else {
				b.p_out_buf156 = AllocPalmRec156(&buf_len); // @checkerr
				b.p_out_buf156->ID = SyncHostToHHDWord(id);
			}
			idx_rec.ID = id;
			get_str_from_dbfrec(&rec, fldn_name, 1, idx_rec.Name, sizeof(idx_rec.Name));
			if(ver < 156)
				get_str_from_dbfrec(&rec, fldn_name, 1, b.p_out_buf->Name, sizeof(b.p_out_buf->Name));
			else
				get_str_from_dbfrec(&rec, fldn_name, 1, b.p_out_buf156->Name, sizeof(b.p_out_buf156->Name));

			if(ver < 156) {
				THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf, buf_len));
			}
			else {
				THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf156, buf_len));
			}
			IdxList.insert(&idx_rec); // @checkerr
			WaitPercent(pFn, ++recno, numrecs, "Ёкспорт справочника товарных групп");
			if(ver < 156) {
				ZDELETE(b.p_out_buf);
			}
			else {
				ZDELETE(b.p_out_buf156);
			}
		} while(P_GGrpTbl->next());
		stbl.Close();
		if(!P_Ctx->PalmCfg.CompressData()) {
			THROW(ExportIndexes(pFn, &IdxList));
		}
		else
			P_Ctx->TransmitComprFile = 1;
		{
			char   log_msg[128];
			sprintf(log_msg, "SPII OK: %ld GOODSGROUP records exported", numrecs);
			SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
		}
	}
	else
		ok = -1;
	CATCH
		ok = 0;
	ENDCATCH
	if(ver < 156)
		delete b.p_out_buf;
	else
		delete  b.p_out_buf156;
	return ok;
}
