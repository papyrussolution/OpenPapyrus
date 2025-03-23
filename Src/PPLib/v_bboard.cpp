// V_BBOARD.CPP
// Copyright (c) A.Starodub 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2023, 2024, 2025
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
IMPLEMENT_PPFILT_FACTORY(ServerStat); ServerStatFilt::ServerStatFilt() : PPBaseFilt(PPFILT_SERVERSTAT, 0, 1)
{
	SetFlatChunk(offsetof(ServerStatFilt, ReserveStart), offsetof(ServerStatFilt, ServerAddr) - offsetof(ServerStatFilt, ReserveStart));
	SetBranchSString(offsetof(ServerStatFilt, ServerAddr));
	Init(1, 0);
}

ServerStatFilt & FASTCALL ServerStatFilt::operator = (const ServerStatFilt & s)
{
	Copy(&s, 1);
	return *this;
}

PPViewServerStat::PPViewServerStat() : PPView(0, &Filt, PPVIEW_SERVERSTAT, implBrowseArray, 0), Data(sizeof(ServerStatViewItem))
{
}

PPViewServerStat::~PPViewServerStat()
{
}

int PPViewServerStat::EditBaseFilt(PPBaseFilt * pFilt)
{
	int    ok = -1;
	ServerStatFilt filt;
	PPIniFile ini_file(0, 0, 0, 1);
	PPInputStringDialogParam isd_param;
	THROW(ini_file.IsValid());
	PPLoadText(PPTXT_JOBSERVERADDRPORT, isd_param.Title);
	isd_param.InputTitle = isd_param.Title;
	filt.Copy(pFilt, 0);
	if(filt.ServerAddr.Len() == 0)
		ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_JOBSERVERADDRPORT, filt.ServerAddr);
	if(InputStringDialog(&isd_param, filt.ServerAddr) > 0) {
		CALLPTRMEMB(pFilt, Copy(&filt, 0));
		ini_file.Append(PPINISECT_CONFIG, PPINIPARAM_JOBSERVERADDRPORT, filt.ServerAddr, 1);
		ini_file.FlashIniBuf();
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewServerStat::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(FetchStat());
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPViewServerStat::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewServerStat * p_v = static_cast<PPViewServerStat *>(pBlk->ExtraPtr);
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int PPViewServerStat::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		const ServerStatViewItem * p_item = static_cast<const ServerStatViewItem *>(pBlk->P_SrcData);
		void * p_dest = pBlk->P_DestData;
		switch(pBlk->ColumnN) {
			case 0: // ИД потока
				pBlk->Set((int32)p_item->TId);
				break;
			case 1: // Вид потока
				PPThread::GetKindText(p_item->Kind, temp_buf);
				pBlk->Set(temp_buf);
				break;
			case 2: // Время запуска
				pBlk->Set(temp_buf.Cat(p_item->StartMoment, DATF_DMY, TIMF_HMS));
				break;
			case 3: // Время работы
				{
					const LDATETIME now_dtm = getcurdatetime_();
					LTIME ctm;
					ctm.settotalsec(diffdatetimesec(now_dtm, p_item->StartMoment));
					pBlk->Set(ctm);
				}
				break;
			case 4: // Наименование
				pBlk->Set(p_item->Text);
				break;
			case 5: // Последнее сообщение
				pBlk->Set(p_item->LastMsg);
				break;
		}
	}
	return ok;
}

static int CellStyleFunc(const void * pData, long col, int paintAction, BrowserWindow::CellStyle * pStyle, void * extraPtr)
{
	int    ok = -1;
	PPViewBrowser * p_brw = static_cast<PPViewBrowser *>(extraPtr);
	if(p_brw && pData && pStyle) {
		const ServerStatViewItem * p_item = static_cast<const ServerStatViewItem *>(pData);
		if(p_item->State & PPThread::Info::stLocalStop) {
			pStyle->Color = GetGrayColorRef(0.7f);
			pStyle->Flags = 0; // BrowserWindow::CellStyle::fCorner;
			ok = 1;
		}
	}
	return ok;
}

void PPViewServerStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewServerStat::GetDataForBrowser, this);
		pBrw->SetRefreshPeriod(5);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

SArray * PPViewServerStat::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	ASSIGN_PTR(pSubTitle, Filt.ServerAddr);
	ASSIGN_PTR(pBrwId, BROWSER_SERVERSTAT);
	return new SArray(Data);
}

int PPViewServerStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2 && oneof2(ppvCmd, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		FetchStat();
		AryBrowserDef * p_def = static_cast<AryBrowserDef *>(pBrw->getDef());
		if(p_def) {
			const long cp = p_def->_curItem();
			const void * p_cr = p_def->getRow(cp);
			const long tid = p_cr ? *static_cast<const long *>(p_cr) : 0;
			p_def->setArray(new SArray(Data), 0, 1);
			if(!p_def->search2(&tid, CMPF_LONG, srchFirst, 0))
				p_def->top();
		}
		ok = 1;
	}
	else {
		if(ppvCmd == PPVCMD_DELETEITEM) {
			if(pHdr)
				ok = StopThread(*static_cast<const long *>(pHdr));
		}
		else if(ppvCmd == PPVCMD_RESETCACHE) {
			ResetCache();
		}
		else if(ppvCmd == PPVCMD_LOGLOCKSTACK) {
			LogLockStack();
		}
	}
	return ok;
}

int PPViewServerStat::ResetCache()
{
	int    ok = -1;
	if(PPMaster) {
		PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
		if(p_cli) {
			PPJobSrvCmd cmd;
			PPJobSrvReply reply;
			SString q;
			q.Cat("RESETCACHE").Space().Cat("GOODS").Space().Cat("NAMEPOOL");
			if(p_cli->ExecSrvCmd(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewServerStat::LogLockStack()
{
	int    ok = -1;
	if(PPMaster) {
		PPJobSrvClient * p_cli = DS.GetClientSession(false/*dontReconnect*/);
		if(p_cli) {
			PPJobSrvCmd cmd;
			PPJobSrvReply reply;
			SString q;
			q.Cat("LOGLOCKSTACK");
			if(p_cli->ExecSrvCmd(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPViewServerStat::StopThread(long tid)
{
	int    ok = 1;
	if(PPMaster) {
		PPJobSrvClient cli;
		PPJobSrvCmd cmd;
		PPJobSrvReply reply;
		SSerializeContext ctx;

		PPJobSrvProtocol::StopThreadBlock blk;
		blk.TId = tid;
		GetFirstMACAddr(&blk.MAddr);
		SGetComputerName(blk.HostName);
		GetCurUserName(blk.UserName);

		cmd.StartWriting(PPSCMD_STOPTHREAD);
		ctx.Init(0, getcurdate_());
		THROW(blk.Serialize(+1, cmd, &ctx));
		THROW(cmd.FinishWriting());
		THROW(cli.Connect(Filt.ServerAddr, 0));
		THROW(cli.ExecSrvCmd(cmd, reply));
		reply.StartReading(0);
		THROW(reply.CheckRepError());
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int PPViewServerStat::FetchStat()
{
	int    ok = 1;
	PPJobSrvClient cli;
	PPJobSrvReply reply;
	SString reply_buf;
	Data.clear();
	THROW(cli.Connect(Filt.ServerAddr, 0));
	THROW(cli.ExecSrvCmd("GetServerStat", reply));
	reply.StartReading(&reply_buf);
	THROW(reply.CheckRepError());
	{
		int32 c = 0;
		SSerializeContext ctx;
		PPThread::Info info_rec;
		THROW_SL(ctx.SerializeStateOfContext(-1, reply));
		THROW_SL(ctx.Serialize(-1, c, reply));
		for(int i = 0; i < c; i++) {
			ServerStatViewItem item;
			THROW(info_rec.Serialize(-1, reply, &ctx));
			MEMSZERO(item);
			item.TId = info_rec.Id;
			item.Kind = info_rec.Kind;
			item.State = info_rec.Status;
			item.StartMoment = info_rec.StartMoment;
			info_rec.Text.CopyTo(item.Text, sizeof(item.Text));
			info_rec.LastMsg.CopyTo(item.LastMsg, sizeof(item.LastMsg));
			THROW_SL(Data.insert(&item));
		}
	}
	THROW(cli.ExecSrvCmd("QUIT", reply));
	CATCHZOK
	return ok;
}

