// SC_GOODS.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2010, 2011, 2015, 2016, 2019, 2020, 2022
// Part of StyloConduit project
// Ёкспорт/»мпорт товаров
//
#include <slib-internal.h>
#pragma hdrstop
#include "StyloConduit.h"
//#include <stddef.h>

IMPL_CMPFUNC(GOODSID, i1, i2)
{
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if(g1->ID < g2->ID)
		return -1;
	else if(g1->ID > g2->ID)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(GOODSNAM, i1, i2)
{
	int r = 0;
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if((r = stricmp866(g1->Name, g2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(GOODSGRPIDNAM, i1, i2)
{
	int r = 0;
	int cmp = 0;
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if(g1->GrpID < g2->GrpID)
		cmp = -1;
	else if(g1->GrpID > g2->GrpID)
		cmp = 1;
	else
		cmp = 0;
	if(cmp == 0) {
		if((r = stricmp866(g1->Name, g2->Name)) < 0)
			cmp = -1;
		else if(r > 0)
			cmp = 1;
		else
			cmp = 0;
	}
	return cmp;
}

IMPL_CMPFUNC(GOODSGRPIDID, i1, i2)
{
	int cmp = 0;
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if(g1->GrpID < g2->GrpID)
		cmp = -1;
	else if(g1->GrpID > g2->GrpID)
		cmp = 1;
	else
		cmp = 0;
	if(cmp == 0) {
		if(g1->ID < g2->ID)
			cmp = -1;
		else if(g1->ID > g2->ID)
			cmp = 1;
		else
			cmp = 0;
	}
	return cmp;
}

IMPL_CMPFUNC(GOODSCODE, i1, i2)
{
	int r = 0;
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if((r = stricmp866(g1->Code, g2->Code)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

IMPL_CMPFUNC(GOODSGRPIDCODE, i1, i2)
{
	int r = 0;
	int cmp = 0;
	const SCDBObjGoods::IdxRec * g1 = static_cast<const SCDBObjGoods::IdxRec *>(i1);
	const SCDBObjGoods::IdxRec * g2 = static_cast<const SCDBObjGoods::IdxRec *>(i2);
	if(g1->GrpID < g2->GrpID)
		cmp = -1;
	else if(g1->GrpID > g2->GrpID)
		cmp = 1;
	else
		cmp = 0;
	if(cmp == 0) {
		if((r = stricmp866(g1->Code, g2->Code)) < 0)
			cmp = -1;
		else if(r > 0)
			cmp = 1;
		else
			cmp = 0;
	}
	return cmp;
}

SCDBObjGoods::SCDBObjGoods(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_GoodsTbl(0), P_QkTbl(0)
{
}

SCDBObjGoods::~SCDBObjGoods()
{
	delete P_GoodsTbl;
	delete P_QkTbl;
}

const SCDBTblEntry * SCDBObjGoods::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{"Goods.tbl",   5, offsetof(PalmConfig, TmGoods), 0, 0},
		{"GdsID.idx",   0, 0, 0, PTR_CMPFUNC(GOODSID)},
		{"GdsName.idx", 0, 0, 0, PTR_CMPFUNC(GOODSNAM)},
		{"GdsGNam.idx", 0, 0, 0, PTR_CMPFUNC(GOODSGRPIDNAM)},
		{"GdsGID.idx",  0, 0, 0, PTR_CMPFUNC(GOODSGRPIDID)},
		{"GdsGIDC.idx", 0, 0, 0, PTR_CMPFUNC(GOODSGRPIDCODE)}
	};
	ASSIGN_PTR(pEntryCount, 6);
	return def;
}

int SCDBObjGoods::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_GoodsTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_goods.dbf"));
	P_QkTbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_quotk.dbf"));
	LoadQuotKindList();
	return ok;
}

IMPL_CMPFUNC(QuotKindName, i1, i2)
{
	const SCDBObjGoods::QuotKind * p1 = static_cast<const SCDBObjGoods::QuotKind *>(i1);
	const SCDBObjGoods::QuotKind * p2 = static_cast<const SCDBObjGoods::QuotKind *>(i2);
	int    r;
	if((r = stricmp866(p1->Name, p2->Name)) < 0)
		return -1;
	else if(r > 0)
		return 1;
	else
		return 0;
}

int SCDBObjGoods::LoadQuotKindList()
{
	int    ok = 1;
	QkList.freeAll();
	if(P_QkTbl && P_QkTbl->top()) {
		int fldn_id = 0;
		int fldn_n = 0;
		int fldn_name = 0;
		P_QkTbl->getFieldNumber("ID",   &fldn_id);
		P_QkTbl->getFieldNumber("N",    &fldn_n);
		P_QkTbl->getFieldNumber("NAME", &fldn_name);
		do {
			QuotKind qk_rec;
			MEMSZERO(qk_rec);
			DbfRecord rec(P_QkTbl);
			P_QkTbl->getRec(&rec);
			rec.get(fldn_id, qk_rec.ID);
			rec.get(fldn_n, qk_rec.N);
			get_str_from_dbfrec(&rec, fldn_name, 1, qk_rec.Name, sizeof(qk_rec.Name));
			QkList.insert(&qk_rec); // @checkerr
		} while(P_QkTbl->next());
		QkList.sort(PTR_CMPFUNC(QuotKindName));
	}
	return ok;
}

SCDBObjGoods::PalmRec * SCDBObjGoods::AllocPalmRec(const TSVector <Quot> * pQuotList, size_t * pBufLen)
{
	const size_t buf_len = sizeof(PalmRec) + pQuotList->getCount() * sizeof(Quot);
	PalmRec * p_buf = static_cast<PalmRec *>(SAlloc::C(1, buf_len));
	if(p_buf) {
		p_buf->QuotCount = SyncHostToHHWord(pQuotList->getCount());
		for(uint i = 0; i < pQuotList->getCount(); i++) {
			reinterpret_cast<Quot *>(p_buf+1)[i] = pQuotList->at(i);
		}
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjGoods::PalmRec156 * SCDBObjGoods::AllocPalmRec156(const TSVector <Quot> * pQuotList, size_t * pBufLen)
{
	const size_t buf_len = sizeof(PalmRec156) + pQuotList->getCount() * sizeof(Quot);
	PalmRec156 * p_buf = static_cast<PalmRec156 *>(SAlloc::C(1, buf_len));
	if(p_buf) {
		p_buf->QuotCount = SyncHostToHHWord(pQuotList->getCount());
		for(uint i = 0; i < pQuotList->getCount(); i++) {
			reinterpret_cast<Quot *>(p_buf+1)[i] = pQuotList->at(i);
		}
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjGoods::PalmRec500 * SCDBObjGoods::AllocPalmRec500(const TSVector <Quot> * pQuotList, size_t * pBufLen)
{
	const size_t buf_len = sizeof(PalmRec500) + pQuotList->getCount() * sizeof(Quot);
	PalmRec500 * p_buf = (PalmRec500 *)SAlloc::C(1, buf_len);
	if(p_buf) {
		p_buf->QuotCount = SyncHostToHHWord(pQuotList->getCount());
		for(uint i = 0; i < pQuotList->getCount(); i++) {
			reinterpret_cast<Quot *>(p_buf+1)[i] = pQuotList->at(i);
		}
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjGoods::PalmRec800 * SCDBObjGoods::AllocPalmRec800(const TSVector <Quot> * pQuotList, size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec800) + pQuotList->getCount() * sizeof(Quot);
	PalmRec800 * p_buf = (PalmRec800 *)SAlloc::C(1, buf_len);
	p_buf->QuotCount = SyncHostToHHWord(pQuotList->getCount());
	for(uint i = 0; i < pQuotList->getCount(); i++) {
		reinterpret_cast<Quot *>(p_buf+1)[i] = pQuotList->at(i);
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

SCDBObjGoods::PalmRec900 * SCDBObjGoods::AllocPalmRec900(const TSVector <Quot> * pQuotList, size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec900) + pQuotList->getCount() * sizeof(Quot);
	PalmRec900 * p_buf = (PalmRec900 *)SAlloc::C(1, buf_len);
	p_buf->QuotCount = SyncHostToHHWord(pQuotList->getCount());
	for(uint i = 0; i < pQuotList->getCount(); i++) {
		reinterpret_cast<Quot *>(p_buf+1)[i] = pQuotList->at(i);
	}
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

#define MAXQUOT 20

int SCDBObjGoods::GetQuotList(DbfRecord * pRec, int * pFldnList, TSVector <Quot> & rList)
{
	int    ok = 1;
	rList.freeAll();
	for(size_t i = 0; i < MAXQUOT; i++)
		if(pFldnList[i]) {
			double price = 0;
			pRec->get(pFldnList[i], price);
			price = round(price, 2);
			if(price > 0)
				for(uint j = 0; j < QkList.getCount(); j++)
					if(QkList.at(j).N == (i+1)) {
						Quot q;
						q.QuotKindID = SyncHostToHHDWord(QkList.at(j).ID);
						q.Price = dbltopalmintmoney(price);
						rList.insert(&q); // @checkerr
						break;
					}
		}
	return ok;
}

int SCDBObjGoods::SendQuotKindList()
{
	int    ok = 1;
	SyncTable stbl_qk(0, 0, P_Ctx);
	if(QkList.getCount()) {
		uint i;
		THROW(stbl_qk.Open("QuotKind.tbl", SyncTable::oCreate));
		THROW(stbl_qk.PurgeAll());
		for(i = 0; i < QkList.getCount(); i++) {
			struct PalmQuotKind {
				int32  ID;
				char   Name[30];
			} rec;
			MEMSZERO(rec);
			rec.ID = SyncHostToHHDWord(QkList.at(i).ID);
			STRNSCPY(rec.Name, QkList.at(i).Name);
			DWORD rec_id = 0;
			THROW(stbl_qk.AddRec(&rec_id, &rec, sizeof(rec)));
		}
	}
	CATCHZOK
	return ok;
}

int SCDBObjGoods::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	const  uint32 ver = P_Ctx->PalmCfg.Ver;
	long   numrecs = 0, recno = 0;
	union {
		PalmRec * p_out_buf;
		PalmRec156 * p_out_buf156;
		PalmRec500 * p_out_buf500;
		PalmRec800 * p_out_buf800;
		PalmRec900 * p_out_buf900;
	} b;
	if(ver < 156)
		b.p_out_buf = 0;
	else if(ver < 500)
		b.p_out_buf156 = 0;
	else if(ver < 800)
		b.p_out_buf500 = 0;
	else if(ver < 900)
		b.p_out_buf800 = 0;
	else
		b.p_out_buf900 = 0;
	if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmGoods, P_Ctx->HostCfg.TmGoods) >= 0) {
		SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: GOODS base on palm is recently that host");
		ok = -1;
	}
	else {
		IdxList.freeAll();
		SendQuotKindList(); // @checkerr
		if(P_GoodsTbl && P_GoodsTbl->getNumRecs() && P_GoodsTbl->top()) {
			size_t i;
			numrecs = P_GoodsTbl->getNumRecs();
			SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
			THROW(InitTable(&stbl));
			int fldn_id    = 0;
			int fldn_name  = 0;
			int fldn_bcode = 0;
			int fldn_pack  = 0;
			int fldn_price = 0;
			int fldn_rest  = 0;
			int fldn_ggid  = 0;
			int fldn_brand = 0;
			int fldn_brandown      = 0;
			int fldn_minord        = 0;
			int fldn_multord       = 0;
			int fldn_quot[MAXQUOT];
			memzero(&fldn_quot, sizeof(fldn_quot));
			P_GoodsTbl->getFieldNumber("ID",       &fldn_id);
			P_GoodsTbl->getFieldNumber("NAME",     &fldn_name);
			P_GoodsTbl->getFieldNumber("BARCODE",  &fldn_bcode);
			P_GoodsTbl->getFieldNumber("PACK",     &fldn_pack);
			P_GoodsTbl->getFieldNumber("PRICE",    &fldn_price);
			P_GoodsTbl->getFieldNumber("REST",     &fldn_rest);
			P_GoodsTbl->getFieldNumber("GROUPID",  &fldn_ggid);
			P_GoodsTbl->getFieldNumber("BRANDID",  &fldn_brand);
			P_GoodsTbl->getFieldNumber("BRDOWNID", &fldn_brandown);
			P_GoodsTbl->getFieldNumber("MINORD",   &fldn_minord);
			P_GoodsTbl->getFieldNumber("MULTORD",  &fldn_multord);
			for(i = 0; i < MAXQUOT; i++) {
				char quot_fld_name[32];
				sprintf(quot_fld_name, "%s%d", "QUOT", i + 1);
				P_GoodsTbl->getFieldNumber(quot_fld_name, &fldn_quot[i]);
			}
			do {
				int32  id_p = 0;
				char name[64];
				char code[16];
				int32  ggrp_id     = 0;
				int32  pack        = 0;
				int32  price       = 0;
				int32  rest        = 0;
				int32  brand_id    = 0;
				int32  brandown_id = 0;
				int32  min_ord     = 0;
				int16  mult_ord    = 0;
				size_t buf_len = 0;
				long   id = 0, temp_long;
				double temp_dbl;
				TSVector <Quot> quot_list;
				IdxRec idx_rec;
				MEMSZERO(idx_rec);
				DbfRecord rec(P_GoodsTbl);
				P_GoodsTbl->getRec(&rec);
				GetQuotList(&rec, fldn_quot, quot_list); // @checkerr
				// ID
				rec.get(fldn_id, id);
				id_p = SyncHostToHHDWord(id);
				idx_rec.ID = id;
				// Name
				get_str_from_dbfrec(&rec, fldn_name, 1, idx_rec.Name, sizeof(idx_rec.Name));
				get_str_from_dbfrec(&rec, fldn_name, 1, name, sizeof(name));
				// Code
				get_str_from_dbfrec(&rec, fldn_bcode, 0, idx_rec.Code, sizeof(idx_rec.Code));
				get_str_from_dbfrec(&rec, fldn_bcode, 0, code, sizeof(code));
				// GoodsGrpID
				rec.get(fldn_ggid, temp_long);
				ggrp_id = SyncHostToHHDWord(temp_long);
				idx_rec.GrpID = temp_long;
				// Pack
				rec.get(fldn_pack, temp_dbl);
				pack = dbltopalmintmoney(temp_dbl);
				// Price
				rec.get(fldn_price, temp_dbl);
				price = dbltopalmintmoney(temp_dbl);
				// Rest
				rec.get(fldn_rest, temp_dbl);
				rest = dbltopalmintmoney(temp_dbl);
				// Brand
				rec.get(fldn_brand, temp_long);
				brand_id = SyncHostToHHDWord(temp_long);
				// Brand owner
				rec.get(fldn_brandown, temp_long);
				brandown_id = SyncHostToHHDWord(temp_long);
				// Min order
				rec.get(fldn_minord, temp_dbl);
				min_ord = dbltopalmintmoney(temp_dbl);
				// Multiply min order
				rec.get(fldn_multord, temp_long);
				mult_ord = (int16)temp_long;
				{
					if(ver < 156) {
						b.p_out_buf = AllocPalmRec(&quot_list, &buf_len); // @checkerr
						b.p_out_buf->ID = id_p;
						b.p_out_buf->GoodsGrpID = ggrp_id;
						b.p_out_buf->Pack = pack;
						b.p_out_buf->Price = price;
						b.p_out_buf->Rest  = rest;
						STRNSCPY(b.p_out_buf->Name, name);
						STRNSCPY(b.p_out_buf->Code, code);
					}
					else if(ver < 500) {
						b.p_out_buf156 = AllocPalmRec156(&quot_list, &buf_len); // @checkerr
						b.p_out_buf156->ID = id_p;
						b.p_out_buf156->GoodsGrpID = ggrp_id;
						b.p_out_buf156->Pack = pack;
						b.p_out_buf156->Price = price;
						b.p_out_buf156->Rest  = rest;
						STRNSCPY(b.p_out_buf156->Name, name);
						STRNSCPY(b.p_out_buf156->Code, code);
					}
					else if(ver < 800) {
						b.p_out_buf500 = AllocPalmRec500(&quot_list, &buf_len); // @checkerr
						b.p_out_buf500->ID = id_p;
						b.p_out_buf500->GoodsGrpID = ggrp_id;
						b.p_out_buf500->Pack = pack;
						b.p_out_buf500->Price = price;
						b.p_out_buf500->Rest  = rest;
						STRNSCPY(b.p_out_buf500->Name, name);
						STRNSCPY(b.p_out_buf500->Code, code);
						b.p_out_buf500->BrandID      = brand_id;
						b.p_out_buf500->BrandOwnerID = brandown_id;
					}
					else if(ver < 900) {
						b.p_out_buf800 = AllocPalmRec800(&quot_list, &buf_len); // @checkerr
						b.p_out_buf800->ID    = id_p;
						b.p_out_buf800->GoodsGrpID    = ggrp_id;
						b.p_out_buf800->Pack  = pack;
						b.p_out_buf800->Price = price;
						b.p_out_buf800->Rest  = rest;
						STRNSCPY(b.p_out_buf800->Name, name);
						STRNSCPY(b.p_out_buf800->Code, code);
						b.p_out_buf800->BrandID       = brand_id;
						b.p_out_buf800->BrandOwnerID  = brandown_id;
						b.p_out_buf800->MinOrd        = min_ord;
					}
					else {
						b.p_out_buf900 = AllocPalmRec900(&quot_list, &buf_len); // @checkerr
						b.p_out_buf900->ID    = id_p;
						b.p_out_buf900->GoodsGrpID    = ggrp_id;
						b.p_out_buf900->Pack  = pack;
						b.p_out_buf900->Price = price;
						b.p_out_buf900->Rest  = rest;
						STRNSCPY(b.p_out_buf900->Name, name);
						STRNSCPY(b.p_out_buf900->Code, code);
						b.p_out_buf900->BrandID       = brand_id;
						b.p_out_buf900->BrandOwnerID  = brandown_id;
						b.p_out_buf900->MinOrd        = min_ord;
						b.p_out_buf900->MultMinOrd    = mult_ord;
					}
				}
				if(ver < 156) {
					THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf, buf_len));
				}
				else if(ver < 500) {
					THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf156, buf_len));
				}
				else if(ver < 800) {
					THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf500, buf_len));
				}
				else if(ver < 900) {
					THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf800, buf_len));
				}
				else {
					THROW(stbl.AddRec(&idx_rec.RecID, b.p_out_buf900, buf_len));
				}
				recno++;
				THROW(stbl.Reopen(-1, recno));
				IdxList.insert(&idx_rec); // @checkerr
				WaitPercent(pFn, recno, numrecs, "Ёкспорт справочника товаров");
				if(ver < 156)
					ZFREE(b.p_out_buf);
				else if(ver < 500)
					ZFREE(b.p_out_buf156);
				else if(ver < 800)
					ZFREE(b.p_out_buf500);
				else if(ver < 900)
					ZFREE(b.p_out_buf800);
				else
					ZFREE(b.p_out_buf900);
			} while(P_GoodsTbl->next());
			stbl.Close();
			if(!P_Ctx->PalmCfg.CompressData()) {
				THROW(ExportIndexes(pFn, &IdxList));
			}
			else
				P_Ctx->TransmitComprFile = 1;
			{
				char log_msg[128];
				sprintf(log_msg, "SPII OK: %ld GOODS records exported", numrecs);
				SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
			}
		}
		else
			ok = -1;
	}
	CATCHZOK
	if(ver < 156)
		delete b.p_out_buf;
	else if(ver < 500)
		delete  b.p_out_buf156;
	else if(ver < 800)
		delete b.p_out_buf500;
	else if(ver < 900)
		delete b.p_out_buf800;
	else
		delete b.p_out_buf900;
	return ok;
}
//
// Export brands and brands owners
//
SCDBObjBrand::SCDBObjBrand(SpiiExchgContext * pCtx) : SCDBObject(pCtx), P_Tbl(0)
{
}

SCDBObjBrand::~SCDBObjBrand()
{
	delete P_Tbl;
}

const SCDBTblEntry * SCDBObjBrand::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{"Brand.tbl", 0, offsetof(PalmConfig, TmGoods), 0, 0},
	};
	ASSIGN_PTR(pEntryCount, 1);
	return def;
}

int SCDBObjBrand::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_Tbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_brand.dbf"));
	return ok;
}

SCDBObjBrand::PalmRec * SCDBObjBrand::AllocPalmRec(size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec);
	PalmRec * p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjBrand::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	long   numrecs = 0, recno = 0;
	PalmRec * p_out_buf = 0;
	if(ver >= 500) {
		if(!(P_Ctx->PalmCfg.Flags & CFGF_LOADBRANDS)) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: BRANDS base declined by palm");
			ok = -1;
		}
		else if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmGoods, P_Ctx->HostCfg.TmGoods) >= 0) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: BRANDS base on palm is recently that host");
			ok = -1;
		}
		else {
			IdxList.freeAll();
			if(P_Tbl && P_Tbl->getNumRecs() && P_Tbl->top()) {
				numrecs = P_Tbl->getNumRecs();
				SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
				THROW(InitTable(&stbl));

				int fldn_id      = 0;
				int fldn_ownid   = 0;
				int fldn_name    = 0;
				int fldn_ownname = 0;
				P_Tbl->getFieldNumber("ID",      &fldn_id);
				P_Tbl->getFieldNumber("OWNID",   &fldn_ownid);
				P_Tbl->getFieldNumber("NAME",    &fldn_name);
				P_Tbl->getFieldNumber("OWNNAME", &fldn_ownname);
				do {
					int32  id = 0, own_id = 0;
					char   name[64], own_name[64];
					size_t buf_len = 0;
					long   temp_long;
					DbfRecord rec(P_Tbl);

					P_Tbl->getRec(&rec);
					// ID
					rec.get(fldn_id, temp_long);
					id = SyncHostToHHDWord(temp_long);
					// Owner ID
					rec.get(fldn_ownid, temp_long);
					own_id = SyncHostToHHDWord(temp_long);
					// Name
					get_str_from_dbfrec(&rec, fldn_name, 1, name, sizeof(name));
					// Owner name
					get_str_from_dbfrec(&rec, fldn_ownname, 1, own_name, sizeof(own_name));
					{
						p_out_buf = AllocPalmRec(&buf_len); // @checkerr
						p_out_buf->ID      = id;
						p_out_buf->OwnerID = own_id;
						STRNSCPY(p_out_buf->Name,      name);
						STRNSCPY(p_out_buf->OwnerName, own_name);
					}
					THROW(stbl.AddRec(0, p_out_buf, buf_len));
					recno++;
					THROW(stbl.Reopen(-1, recno));
					WaitPercent(pFn, recno, numrecs, "Ёкспорт справочника брендов");
					ZFREE(p_out_buf);
				} while(P_Tbl->next());
				stbl.Close();
				if(P_Ctx->PalmCfg.CompressData())
					P_Ctx->TransmitComprFile = 1;
				{
					char log_msg[128];
					sprintf(log_msg, "SPII OK: %ld BRANDS records exported", numrecs);
					SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
				}
			}
			else
				ok = -1;
		}
	}
	CATCHZOK
	delete p_out_buf;
	return ok;
}
//
// Export locations
//
SCDBObjLoc::SCDBObjLoc(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
	P_Tbl = 0;
}

SCDBObjLoc::~SCDBObjLoc()
{
	delete P_Tbl;
}

const SCDBTblEntry * SCDBObjLoc::GetDefinition(uint * pEntryCount) const
{
	static const SCDBTblEntry def[] = {
		{"Location.tbl", 0, offsetof(PalmConfig, TmGoods), 0, 0},
	};
	ASSIGN_PTR(pEntryCount, 1);
	return def;
}

int SCDBObjLoc::Init(const char * pExpPath, const char * pImpPath)
{
	int    ok = 1;
	char   path[MAXPATH], fname[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	P_Tbl = new DbfTable(strcat(STRNSCPY(fname, path), "sp_loc.dbf"));
	return ok;
}

SCDBObjLoc::PalmRec * SCDBObjLoc::AllocPalmRec(size_t * pBufLen)
{
	size_t  buf_len = sizeof(PalmRec);
	PalmRec * p_buf = (PalmRec *)SAlloc::C(1, buf_len);
	ASSIGN_PTR(pBufLen, buf_len);
	return p_buf;
}

int SCDBObjLoc::Export(PROGRESSFN pFn, CSyncProperties * pProps)
{
	int    ok = 1;
	uint32 ver = P_Ctx->PalmCfg.Ver;
	long   numrecs = 0, recno = 0;
	PalmRec * p_out_buf = 0;
	if(ver >= 700) {
		if(!(P_Ctx->PalmCfg.Flags & CFGF_LOADLOCS)) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: LOCATIONS base declined by palm");
			ok = -1;
		}
		else if(!ForceExportObsoleteData && cmp(P_Ctx->PalmCfg.TmGoods, P_Ctx->HostCfg.TmGoods) >= 0) {
			SyncTable::LogMessage(P_Ctx->LogFile, "SPII OK: LOCATIONS base on palm is recently that host");
			ok = -1;
		}
		else {
			IdxList.freeAll();
			if(P_Tbl && P_Tbl->getNumRecs() && P_Tbl->top()) {
				numrecs = P_Tbl->getNumRecs();
				SyncTable stbl(P_Ctx->PalmCfg.CompressData(), 0, P_Ctx);
				THROW(InitTable(&stbl));

				int fldn_id      = 0;
				int fldn_name    = 0;
				P_Tbl->getFieldNumber("ID",      &fldn_id);
				P_Tbl->getFieldNumber("NAME",    &fldn_name);
				do {
					int32  id = 0;
					char   name[48];
					size_t buf_len = 0;
					long   temp_long;
					DbfRecord rec(P_Tbl);

					P_Tbl->getRec(&rec);
					// ID
					rec.get(fldn_id, temp_long);
					id = SyncHostToHHDWord(temp_long);
					// Name
					get_str_from_dbfrec(&rec, fldn_name, 1, name, sizeof(name));
					{
						p_out_buf = AllocPalmRec(&buf_len); // @checkerr
						p_out_buf->ID = id;
						STRNSCPY(p_out_buf->Name, name);
					}
					THROW(stbl.AddRec(0, p_out_buf, buf_len));
					recno++;
					THROW(stbl.Reopen(-1, recno));
					WaitPercent(pFn, recno, numrecs, "Ёкспорт справочника складов");
					ZFREE(p_out_buf);
				} while(P_Tbl->next());
				stbl.Close();
				if(P_Ctx->PalmCfg.CompressData())
					P_Ctx->TransmitComprFile = 1;
				{
					char log_msg[128];
					sprintf(log_msg, "SPII OK: %ld LOCATION records exported", numrecs);
					SyncTable::LogMessage(P_Ctx->LogFile, log_msg);
				}
			}
			else
				ok = -1;
		}
	}
	CATCHZOK
	delete p_out_buf;
	return ok;
}
