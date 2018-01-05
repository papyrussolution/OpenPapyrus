// V_BBOARD.CPP
// Copyright (c) A.Starodub 2009, 2010, 2013, 2014, 2015, 2016, 2017, 2018
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop
#include <charry.h>
//
//
//
IMPLEMENT_PPFILT_FACTORY(ServerStat); SLAPI ServerStatFilt::ServerStatFilt() : PPBaseFilt(PPFILT_SERVERSTAT, 0, 1)
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

SLAPI PPViewServerStat::PPViewServerStat() : PPView(0, &Filt, PPVIEW_SERVERSTAT), Data(sizeof(ServerStatViewItem))
{
	ImplementFlags |= implBrowseArray;
}

SLAPI PPViewServerStat::~PPViewServerStat()
{
}

int SLAPI PPViewServerStat::EditBaseFilt(PPBaseFilt * pFilt)
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

int SLAPI PPViewServerStat::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	THROW(Helper_InitBaseFilt(pFilt));
	THROW(FetchStat());
	CATCHZOK
	return ok;
}

//static
int PPViewServerStat::GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	PPViewServerStat * p_v = (PPViewServerStat *)pBlk->ExtraPtr;
	return p_v ? p_v->_GetDataForBrowser(pBlk) : 0;
}

int SLAPI PPViewServerStat::_GetDataForBrowser(SBrowserDataProcBlock * pBlk)
{
	int    ok = 0;
	if(pBlk->P_SrcData && pBlk->P_DestData) {
		ok = 1;
		SString temp_buf;
		ServerStatViewItem * p_item = (ServerStatViewItem *)pBlk->P_SrcData;
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
					LDATETIME cur = getcurdatetime_();
					LTIME ctm;
					ctm.settotalsec(diffdatetimesec(cur, p_item->StartMoment));
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
	PPViewBrowser * p_brw = (PPViewBrowser *)extraPtr;
	if(p_brw && pData && pStyle) {
		ServerStatViewItem * p_item = (ServerStatViewItem *)pData;
		if(p_item->State & PPThread::Info::stLocalStop) {
			pStyle->Color = GetGrayColorRef(0.7f);
			pStyle->Flags = 0; // BrowserWindow::CellStyle::fCorner;
			ok = 1;
		}
	}
	return ok;
}

void SLAPI PPViewServerStat::PreprocessBrowser(PPViewBrowser * pBrw)
{
	if(pBrw) {
		pBrw->SetDefUserProc(PPViewServerStat::GetDataForBrowser, this);
		pBrw->SetRefreshPeriod(5);
		pBrw->SetCellStyleFunc(CellStyleFunc, pBrw);
	}
}

SArray * SLAPI PPViewServerStat::CreateBrowserArray(uint * pBrwId, SString * pSubTitle)
{
	SArray * p_array = new SArray(Data);
	uint   brw_id = BROWSER_SERVERSTAT;
	ASSIGN_PTR(pSubTitle, Filt.ServerAddr);
	ASSIGN_PTR(pBrwId, brw_id);
	return p_array;
}

int SLAPI PPViewServerStat::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	if(ok == -2 && oneof2(ppvCmd, PPVCMD_REFRESHBYPERIOD, PPVCMD_REFRESH)) {
		FetchStat();
		AryBrowserDef * p_def = (AryBrowserDef *)pBrw->getDef();
		if(p_def) {
			// @v8.4.11 {
			const long cp = p_def->_curItem();
			const void * p_cr = p_def->getRow(cp);
			const long tid = p_cr ? *(long *)p_cr : 0;
			// } @v8.4.11
			p_def->setArray(new SArray(Data), 0, 1);
			// @v8.4.11 {
			if(!p_def->search2(&tid, CMPF_LONG, srchFirst, 0))
				p_def->top();
			// } @v8.4.11
		}
		ok = 1;
	}
	else {
		if(ppvCmd == PPVCMD_DELETEITEM) {
			if(pHdr)
				ok = StopThread(*(long *)pHdr);
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

int SLAPI PPViewServerStat::ResetCache()
{
	int    ok = -1;
	if(PPMaster) {
		PPJobSrvClient * p_cli = DS.GetClientSession(0);
		if(p_cli) {
			PPJobSrvCmd cmd;
			PPJobSrvReply reply;
			SString q;
			q.Cat("RESETCACHE").Space().Cat("GOODS").Space().Cat("NAMEPOOL");
			if(p_cli->Exec(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewServerStat::LogLockStack()
{
	int    ok = -1;
	if(PPMaster) {
		PPJobSrvClient * p_cli = DS.GetClientSession(0);
		if(p_cli) {
			PPJobSrvCmd cmd;
			PPJobSrvReply reply;
			SString q;
			q.Cat("LOGLOCKSTACK");
			if(p_cli->Exec(q, reply)) {
				THROW(reply.StartReading(0));
				THROW(reply.CheckRepError());
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewServerStat::StopThread(long tid)
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
		THROW(cli.Exec(cmd, reply));
		reply.StartReading(0);
		THROW(reply.CheckRepError());
	}
	else
		ok = -1;
	CATCHZOKPPERR
	return ok;
}

int SLAPI PPViewServerStat::FetchStat()
{
	int    ok = 1;
	PPJobSrvClient cli;
	PPJobSrvReply reply;
	SString reply_buf;
	Data.clear();
	THROW(cli.Connect(Filt.ServerAddr, 0));
	THROW(cli.Exec("GetServerStat", reply));
	reply.StartReading(&reply_buf);
	THROW(reply.CheckRepError());
	{
		int32 c = 0;
		SSerializeContext ctx;
		PPThread::Info info_rec;
		THROW_SL(ctx.SerializeState(-1, reply));
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
	THROW(cli.Exec("QUIT", reply));
	CATCHZOK
	return ok;
}

