// SC_CFG.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2008, 2010, 2013, 2016, 2017
// Part of StyloConduit project
// Экспорт/Импорт конфигурации
//
#include <slib-internal.h>
#pragma hdrstop
#include "StyloConduit.h"

SCDBObjConfig::SCDBObjConfig(SpiiExchgContext * pCtx) : SCDBObject(pCtx)
{
}

SCDBObjConfig::~SCDBObjConfig()
{
}

int SCDBObjConfig::Init(const char * pExpPath, const char * pImpPath)
{
	MEMSZERO(PalmData);
	MEMSZERO(HostData);

	int    ok = 1;
	char   path[MAXPATH];
	setLastSlash(STRNSCPY(path, pExpPath));
	HostData.Read(path); // @checkerr
	HostData.TmLastXchg = getcurdatetime_();
	//
	// Чтение записи конфигурации
	//
	SyncTable stbl_cfg(0, 0, P_Ctx);
	uint32 rec_id = 0;
	size_t rec_size = sizeof(PalmData);
	SyncTable::Stat stat;
	THROW(stbl_cfg.Find("Config.tbl", &stat));
	THROW(stbl_cfg.Open("Config.tbl", SyncTable::oCreate));
	if(stat.NumRecs > 0) {
		THROW(stbl_cfg.ReadRecByIdx(0, &rec_id, &PalmData, &rec_size) || stbl_cfg.GetLastErr() == SYNCERR_NOT_FOUND);
	}
	PalmToHost(&PalmData);
	CATCHZOK
	P_Ctx->PalmCfg = PalmData;
	P_Ctx->HostCfg = HostData;
	return ok;
}

#define PALM2HOST_DW(s,m) s->m=SyncHHToHostDWord(s->m)
#define PALM2HOST_W(s,m)  s->m=SyncHHToHostWord(s->m)
#define HOST2PALM_DW(s,m) s->m=SyncHostToHHDWord(s->m)

void PalmToHost(LDATE & s) { s.v = SyncHHToHostDWord(s.v); }
void PalmToHost(LTIME & s) { /*s.v = SyncHHToHostDWord(s.v);*/ }
void PalmToHost(LDATETIME & s) { PalmToHost(s.d); PalmToHost(s.t); }

void HostToPalm(LDATE & s) { s.v = SyncHostToHHDWord(s.v); }
void HostToPalm(LTIME & s) { /*s.v = SyncHostToHHDWord(s.v);*/ }
void HostToPalm(LDATETIME & s) { HostToPalm(s.d); HostToPalm(s.t); }

void SCDBObjConfig::PalmToHost(PalmConfig * pCfg)
{
	PALM2HOST_DW(pCfg, Size);
	PALM2HOST_DW(pCfg, Ver);
	PALM2HOST_DW(pCfg, InvCode);
	PALM2HOST_DW(pCfg, OrdCode);
	PALM2HOST_DW(pCfg, Shift);
	PALM2HOST_DW(pCfg, Flags);
	PALM2HOST_DW(pCfg, RecvBufSize);
	PALM2HOST_DW(pCfg, SendBufSize);
	::PalmToHost(pCfg->TmLastXchg);
	::PalmToHost(pCfg->TmClient);
	::PalmToHost(pCfg->TmGoods);
	::PalmToHost(pCfg->TmCliDebt);
	::PalmToHost(pCfg->TmCliSell);
	::PalmToHost(pCfg->TmToDo);
	pCfg->CardNo = 0;
	pCfg->SetupBuffers(pCfg->RecvBufSize, pCfg->SendBufSize);
}

void SCDBObjConfig::HostToPalm(PalmConfig * pCfg)
{
	HOST2PALM_DW(pCfg, Size);
	HOST2PALM_DW(pCfg, Ver);
	HOST2PALM_DW(pCfg, InvCode);
	HOST2PALM_DW(pCfg, OrdCode);
	HOST2PALM_DW(pCfg, Shift);
	HOST2PALM_DW(pCfg, Flags);
	HOST2PALM_DW(pCfg, RecvBufSize);
	HOST2PALM_DW(pCfg, SendBufSize);
	::HostToPalm(pCfg->TmLastXchg);
	::HostToPalm(pCfg->TmClient);
	::HostToPalm(pCfg->TmGoods);
	::HostToPalm(pCfg->TmCliDebt);
	::HostToPalm(pCfg->TmCliSell);
	::HostToPalm(pCfg->TmToDo);
	PALM2HOST_W(pCfg, NumSellWeeks);
	PALM2HOST_W(pCfg, MaxNotSentOrd);
	pCfg->CardNo = 0;
	pCfg->Reserve2 = (uint16)SyncHostToHHDWord(pCfg->Reserve2);
}

static void LogDateTime(LDATETIME & tm, const char * pMsg, const char * pLogFile)
{
	char   log_buf[256];
	SString dtm_buf;
	dtm_buf.Cat(tm, DATF_DMY|DATF_CENTURY, TIMF_HMS);
	sprintf(log_buf, "SPII DEBUG: Config=%s: %s", pMsg, dtm_buf.cptr());
	SyncTable::LogMessage(pLogFile, log_buf);
}

int SCDBObjConfig::Export(PROGRESSFN pFn, CSyncProperties *)
{
	int    ok = 1;
	uint32 rec_id = 0;
	long last_err = 0;
	SyncTable::Stat stat;
	PalmConfig palm_buf;
	size_t rec_size = sizeof(palm_buf);
	SyncTable stbl_cfg(0, 0, P_Ctx);
	THROW(stbl_cfg.Find("Config.tbl", &stat));
	THROW(stbl_cfg.Open("Config.tbl", SyncTable::oCreate));
	if(stat.NumRecs > 0) {
		THROW(stbl_cfg.ReadRecByIdx(0, &rec_id, &palm_buf, &rec_size) || (last_err = stbl_cfg.GetLastErr()) == SYNCERR_NOT_FOUND);
	}
	palm_buf.TmLastXchg = HostData.TmLastXchg;
	if(HostData.TmClient.d && cmp(PalmData.TmClient, HostData.TmClient) < 0)
		palm_buf.TmClient  = HostData.TmClient;
	else
		::PalmToHost(palm_buf.TmClient);
	if(HostData.TmGoods.d && cmp(PalmData.TmGoods, HostData.TmGoods) < 0)
		palm_buf.TmGoods   = HostData.TmGoods;
	else
		::PalmToHost(palm_buf.TmGoods);
	if(HostData.TmCliDebt.d && cmp(PalmData.TmCliDebt, HostData.TmCliDebt) < 0)
		palm_buf.TmCliDebt = HostData.TmCliDebt;
	else
		::PalmToHost(palm_buf.TmCliDebt);
	PALM2HOST_DW((&palm_buf), Flags);
	SETFLAG(palm_buf.Flags, CFGF_PALMCOMPRESSEDDATA, 0);
	if(HostData.TmCliSell.d&& cmp(PalmData.TmCliSell, HostData.TmCliSell) < 0) {
		palm_buf.TmCliSell = HostData.TmCliSell;
		palm_buf.NumSellWeeks = HostData.NumSellWeeks;
	}
	else
		::PalmToHost(palm_buf.TmCliSell);
	if(HostData.TmToDo.d && cmp(PalmData.TmToDo, HostData.TmToDo) < 0)
		palm_buf.TmToDo = HostData.TmToDo;
	else
		::PalmToHost(palm_buf.TmToDo);
	::HostToPalm(palm_buf.TmLastXchg);
	::HostToPalm(palm_buf.TmClient);
	::HostToPalm(palm_buf.TmGoods);
	::HostToPalm(palm_buf.TmCliDebt);
	::HostToPalm(palm_buf.TmCliSell);
	::HostToPalm(palm_buf.TmToDo);
	HOST2PALM_DW((&palm_buf), Flags);
	palm_buf.MaxNotSentOrd = HostData.MaxNotSentOrd;
	if(stat.NumRecs == 0) {
		THROW(stbl_cfg.AddRec(&rec_id, &palm_buf, sizeof(palm_buf)));
	}
	else
		THROW(stbl_cfg.UpdateRec(rec_id, &palm_buf, sizeof(palm_buf)));
	CATCHZOK
	return ok;
}
