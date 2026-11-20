// ORTOOLS_MODULE_CLIENT.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
// Модуль, реализующий работу с or-tools через "прокладку" ortoos_support через IPC (named-pipe)
//
#include <pp.h>
#pragma hdrstop
//
// Note: Практически, дословно повторяет class Crr32SupportClient (ppreport.cpp)
//
class OrToolsSupportClient {
public:
	OrToolsSupportClient(const char * pPipeNameUtf8) : BusyPipeTimeoutMs(20000), WrBuf(SKILOBYTE(4)), RdBuf(SKILOBYTE(4))
	{
		PipeNameU.CopyFromUtf8Strict(pPipeNameUtf8, sstrlen(pPipeNameUtf8));
	}
	SIntHandle Connect()
	{
		SIntHandle result;
		if(PipeNameU.NotEmpty()) {
			bool do_exit = false;
			do {
				result = ::CreateFileW(PipeNameU, GENERIC_READ|GENERIC_WRITE, 0/*no sharing*/,
					NULL/*default security attributes*/, OPEN_EXISTING/*opens existing pipe*/, 0/*default attributes*/, NULL/*no template file*/);
				if(!result) {
					if(GetLastError() == ERROR_PIPE_BUSY) {
						boolint w_ok = ::WaitNamedPipeW(PipeNameU, BusyPipeTimeoutMs);
						if(!w_ok) {
							do_exit = true;
						}
					}
					else {
						do_exit = true;
					}
				}
			} while(!result && !do_exit);
			if(!!result) {
				DWORD pipe_mode = PIPE_READMODE_MESSAGE; 
				boolint _ok = ::SetNamedPipeHandleState(result, &pipe_mode/*new pipe mode*/, NULL/*don't set maximum bytes*/, NULL/*don't set maximum time*/);
				if(!_ok) {
					::CloseHandle(result);
					result.Z();
				}
					
			}
		}
		return result;
	}
	bool   SendQuitCommand(SIntHandle hPipe)
	{
		bool   ok = false;
		SJson * p_js_reply = SendCommand(hPipe, -1, "quit", 0);
		if(p_js_reply) {
			delete p_js_reply;
			ok = true;
		}
		return ok;
	}
	SJson * SendCommand(SIntHandle hPipe, int waitTimeout, const char * pCmdUtf8, const char * pParam)
	{
		SJson * p_js_reply = 0;
		SString temp_buf;
		SString reply_buf;
		THROW(hPipe); // @todo @err
		THROW(!isempty(pCmdUtf8)); // @todo @err
		{
			SJson js_query(SJson::tOBJECT);
			js_query.InsertString("cmd", pCmdUtf8);
			if(!isempty(pParam)) {
				js_query.InsertString("param", pParam);
			}
			if(sstreqi_ascii(pCmdUtf8, "run")) {
				js_query.InsertBool("quitafter", true);
			}
			//
			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = ::WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len32()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				reply_buf.Z();
				bool more_data = false;
				do {
					more_data = false;
					union {;
						DWORD rd_size;
						size_t actual_size;
					} rs;
					rs.actual_size = 0;
					RdBuf[0] = 0;
					int rd_result = SFile::AsyncReadFileWithTimeout(hPipe, RdBuf, RdBuf.GetSize(), waitTimeout, &rs.actual_size);
					if(rd_result > 0) {
						reply_buf.CatN(RdBuf, rs.actual_size);
					}
					else if(rd_result < 0) {
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertString("result", "timeout");
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
				if(!p_js_reply)
					p_js_reply = SJson::Parse(reply_buf);
			}
		}
		CATCH
			ZDELETE(p_js_reply);
		ENDCATCH
		return p_js_reply;
	}
private:
	SStringU PipeNameU;
	uint   BusyPipeTimeoutMs;
	STempBuffer WrBuf;
	STempBuffer RdBuf;
};

/*static*/PPOptiModule::BaseExecutionResult * PPOptiModule::Helper_ClientExecution_Task(SIntHandle hPipe, BaseExecutionBlock & rEB)
{
	const  int pipe_wait_timeout = 120000; // ms
	PPOptiModule::BaseExecutionResult * p_result = 0;
	SJson * p_js_reply = 0;
	SString temp_buf;
	SString reply_buf;
	if(!!hPipe) {
		bool   do_process = false;
		SBuffer eb_buf;
		SSerializeContext sctx;
		SJson js_query(SJson::tOBJECT);
		switch(rEB.GetTask()) {
			case taskCuttingStock2D:
				{
					if(rEB.SerializeV(+1, eb_buf, &sctx)) {
						js_query.InsertInt("cmd", taskCuttingStock2D);
						temp_buf.Z().EncodeMime64(eb_buf.constptr(), eb_buf.GetAvailableSize());
						js_query.InsertString("param", temp_buf);
						p_result = new CuttingStock2D::Result();
						do_process = true;
					}
				}
				break;
			default:
				break;
		}
		if(do_process) {
			//result = InternalBlock::SendPrintCommandToServer(cli, h_pipe, rBlk, rReply);

			DWORD wr_size = 0;
			js_query.ToStr(temp_buf);
			boolint wr_ok = ::WriteFile(hPipe, temp_buf.cptr(), temp_buf.Len()+1, &wr_size, NULL/*not overlapped*/);
			if(wr_ok) {
				
				bool more_data = false;
				STempBuffer rd_buf(SKILOBYTE(512));
				do {
					more_data = false;
					union {;
						DWORD rd_size;
						size_t actual_size;
					} rs;
					rs.actual_size = 0;
					rd_buf[0] = 0;
					int rd_result = SFile::AsyncReadFileWithTimeout(hPipe, rd_buf, rd_buf.GetSize(), pipe_wait_timeout, &rs.actual_size);
					if(rd_result > 0) {
						reply_buf.CatN(rd_buf, rs.actual_size);
					}
					else if(rd_result < 0) {
						p_js_reply = new SJson(SJson::tOBJECT);
						p_js_reply->InsertString("result", "timeout");
					}
					else if(GetLastError() == ERROR_MORE_DATA) {
						more_data = true;
					}
					else {
						; // real error
					}
				} while(more_data);
				if(!p_js_reply) {
					p_js_reply = SJson::Parse(reply_buf);
				}
				if(SJson::IsObject(p_js_reply)) {
					for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Text.IsEqiAscii("status")) {
						}
						else if(p_cur->Text.IsEqiAscii("info")) {
						}
						else if(p_cur->Text.IsEqiAscii("result")) {
							SBuffer sbuf;
							if(sbuf.AssignMime64(p_cur->P_Child->Text)) {
								if(!p_result) {
									p_result = new CuttingStock2D::Result();
								}
								p_result->SerializeV(-1, sbuf, &sctx);
							}
						}
					}
				}
			}
		}
	}
	delete p_js_reply;
	return p_result;
}

/*static*/PPOptiModule::BaseExecutionResult * PPOptiModule::ClientExecution(BaseExecutionBlock & rEB)
{
	PPOptiModule::BaseExecutionResult * p_result = 0;
	int    ok = -1;
	int    result = 0;
	const  char * p_ext_process_name = "ortools_support.exe";
	SJson * p_js_reply = 0;
	bool   is_new_process = false;
	SString temp_buf;
	SString pipe_name;
	SString module_path;
	SString reply_buf;
	PPGetPath(PPPATH_BIN, module_path);
	module_path.SetLastSlash().Cat("or-tools").SetLastSlash().Cat(p_ext_process_name);
	GetORToolsProxiPipeName(pipe_name);
	uint   prc_id = SlProcess::SearchProcessByName(p_ext_process_name);
	if(prc_id) {
		// Пытаемся отправить команду ping для того, чтоб удостовериться, что существует
		// общий процесс для обработки не-интерактивных команд печати
		OrToolsSupportClient cli(pipe_name);
		SIntHandle h_pipe(cli.Connect());
		if(!!h_pipe) {
			p_js_reply = cli.SendCommand(h_pipe, 1000, "ping", 0);
			if(p_js_reply) {
				// ...
				//result = 1; // ???
				p_js_reply->ToStr(temp_buf); // @debug
				ZDELETE(p_js_reply);
				p_result = Helper_ClientExecution_Task(h_pipe, rEB);
			}
			::CloseHandle(h_pipe);
		}
	}
	if(!p_result) {
		if(fileExists(module_path)) {
			SlProcess::Result prc_result;
			SlProcess process;
			process.SetPath(module_path);
			SFsPath ps(module_path);
			ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
			process.SetWorkingDir(temp_buf);
			if(process.Run(&prc_result)) {
				SDelay(200);
				is_new_process = true;
				//
				OrToolsSupportClient cli(pipe_name);
				SIntHandle h_pipe(cli.Connect());
				uint  try_to_connect_count = 1;
				if(!h_pipe) {
					for(uint i = 0; !h_pipe && i < 10; i++) {
						SDelay(50);
						h_pipe = cli.Connect();
						try_to_connect_count++;
					}
				}
				if(!h_pipe) {
					;
					// @todo @log
					//do_use_local_func = true;
				}
				else {
					p_js_reply = cli.SendCommand(h_pipe, 1000, "ping", 0);
					if(p_js_reply) {
						// ...
						//result = 1; // ???
						p_js_reply->ToStr(temp_buf); // @debug
						ZDELETE(p_js_reply);
						p_result = Helper_ClientExecution_Task(h_pipe, rEB);
					}
					::CloseHandle(h_pipe);
				}
			}
		}
		else {
			//assert(!do_use_local_func);
			//do_use_local_func = true;
		}
	}
	return p_result;
}
//
// Идея такова: зная набор изделий которые трубуются, получаем их BOM'ы и ищем путь производства до сырьевого товара RawI.GoodsID.
//   Таким образом, мы понимаем какие детали из какого сырья будем кроить. После этого уточняем остатки и цену сырья на скаде RawI.LocID,
//   формулирем задачу раскроя и отправляем ее ortools'у.
// 
class PrcssrOrtCuttingStock2D {
public:
	struct DemandItem {
		DemandItem() : GoodsID(0), Qtty(0.0), Price(0.0)
		{
		}
		PPID   GoodsID;
		double Qtty;
		double Price;
	};

	struct RawItem {
		RawItem() : GoodsID(0), LocID(0)
		{
		}
		PPID   GoodsID;
		PPID   LocID; // Склад, с которого мы возьмем сырье для раскроя //
	};
	
	class Param {
	public:
		TSVector <DemandItem> DL;
		RawItem RawI;
	};
	PrcssrOrtCuttingStock2D();
	~PrcssrOrtCuttingStock2D();
	int    InitParam(Param *);
	int    EditParam(Param *);
	int    Init(const Param *);
	int    Run();
private:
	PPObjGoods GObj;
	Param  P;
};

PrcssrOrtCuttingStock2D::PrcssrOrtCuttingStock2D()
{
}
	
PrcssrOrtCuttingStock2D::~PrcssrOrtCuttingStock2D()
{
}

int PrcssrOrtCuttingStock2D::InitParam(Param * pParam)
{
	if(pParam) {
		pParam->RawI.LocID = LConfig.Location;
	}
	return 1;
}

int PrcssrOrtCuttingStock2D::EditParam(Param * pParam)
{
	class CuttingStock2DTestParamDialog : public PPListDialog {
		enum {
			ctlgroupGoods = 1
		};
		DECL_DIALOG_DATA(PrcssrOrtCuttingStock2D::Param);
	public:
		int    EditItem(DemandItem & rData)
		{
			int    ok = -1;
			TDialog * dlg = new TDialog(DLG_ORTCS2DPARAM_ITEM);
			if(CheckDialogPtrErr(&dlg)) {
				dlg->addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_ORTCS2DPARAM_ITEM_GOODSGRP, CTLSEL_ORTCS2DPARAM_ITEM_GOODS));
				{
					GoodsCtrlGroup::Rec ggrp_rec(0, rData.GoodsID, 0, 0);
					dlg->setGroupData(ctlgroupGoods, &ggrp_rec);
				}
				dlg->setCtrlReal(CTL_ORTCS2DPARAM_ITEM_QTTY, rData.Qtty);
				dlg->setCtrlReal(CTL_ORTCS2DPARAM_ITEM_PRICE, rData.Price);
				while(ok < 0 && ExecView(dlg) == cmOK) {
					{
						GoodsCtrlGroup::Rec ggrp_rec;
						dlg->getGroupData(ctlgroupGoods, &ggrp_rec);
						rData.GoodsID = ggrp_rec.GoodsID;
					}
					rData.Qtty = dlg->getCtrlReal(CTL_ORTCS2DPARAM_ITEM_QTTY);
					rData.Price = dlg->getCtrlReal(CTL_ORTCS2DPARAM_ITEM_PRICE);
					ok = 1;
				}
			}
			else
				ok = 0;
			delete dlg;
			return ok;
		}

		CuttingStock2DTestParamDialog() : PPListDialog(DLG_ORTCS2DPARAM, CTL_ORTCS2DPARAM_DEMANDLIST)
		{
			addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_ORTCS2DPARAM_RAWGRP, CTLSEL_ORTCS2DPARAM_RAW));
			updateList(-1);
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			RVALUEPTR(Data, pData);
			{
				GoodsCtrlGroup::Rec ggrp_rec(0, Data.RawI.GoodsID, 0, 0);
				setGroupData(ctlgroupGoods, &ggrp_rec);
			}
			SetupLocationCombo(this, CTLSEL_ORTCS2DPARAM_LOC, Data.RawI.LocID, 0, LOCTYP_WAREHOUSE, 0);
			updateList(-1);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			{
				GoodsCtrlGroup::Rec ggrp_rec;
				getGroupData(ctlgroupGoods, &ggrp_rec);
				Data.RawI.GoodsID = ggrp_rec.GoodsID;
			}
			getCtrlData(CTLSEL_ORTCS2DPARAM_LOC, &Data.RawI.LocID);
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		virtual int setupList()
		{
			SString temp_buf;
			StringSet ss(SLBColumnDelim);
			for(uint i = 0; i < Data.DL.getCount(); i++) {
				const  DemandItem & r_item = Data.DL.at(i);
				ss.Z();
				GetGoodsName(r_item.GoodsID, temp_buf);
				ss.add(temp_buf);
				temp_buf.Z().Cat(r_item.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
				ss.add(temp_buf);
				temp_buf.Z().Cat(r_item.Price, MKSFMTD_020);
				addStringToList(i+1, ss.getBuf());
			}
			return 1;
		}
		virtual int addItem(long * pPos, long * pID)
		{
			int   ok = -1;
			long  pos = 0;
			long  id = 0;
			DemandItem new_item;
			new_item.Qtty = 1.0;
			if(EditItem(new_item) > 0) {
				Data.DL.insert(&new_item);
				pos = Data.DL.getCount()-1;
				id = Data.DL.getCount();
				ok = 1;
			}
			ASSIGN_PTR(pPos, pos);
			ASSIGN_PTR(pID, id);
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			if(pos >= 0 && pos < Data.DL.getCountI()) {
				DemandItem & r_item = Data.DL.at(pos);
				if(EditItem(r_item) > 0) {
					ok = 1;
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			if(pos >= 0 && pos < Data.DL.getCountI()) {
				Data.DL.atFree(pos);
				ok = 1;
			}
			return ok;
		}
	};
	DIALOG_PROC_BODY(CuttingStock2DTestParamDialog, pParam);
}
	
int PrcssrOrtCuttingStock2D::Init(const Param * pParam)
{
	int    ok = 0;
	if(pParam) {
		if(pParam->RawI.GoodsID && pParam->DL.getCount()) {
			ok = 1;
		}
	}
	return ok;
}

int PrcssrOrtCuttingStock2D::Run()
{
	int    ok = -1;
	Reference * p_ref(PPRef);
	PPObjBill * p_bobj(BillObj);
	PPOptiModule::CuttingStock2D::ExecutionBlock eblk;
	SString temp_buf;
	//
	// todo: Обязательно найти путь производства от каждого элемента P.DL к одному из элементов raw_id_list.
	//
	if(P.RawI.GoodsID && P.DL.getCount()) {
		// PPObjGoodsStruc::FindComplPath()
		/*{
			for(uint dli = 0; dli < P.DL.getCount(); dli++) {
				const  DemandItem & r_di = P.DL.at(dli);
				TSCollection <PPGoodsStruc> gs_list;
				PPGoodsStruc::Ident gsi(r_di.GoodsID, GSF_COMPL, GSF_PARTITIAL|GSF_SUBST);
				GObj.LoadGoodsStruc(gsi, gs_list);
				if(gs_list.getCount()) {
					for(uint si = 0; si < gs_list.getCount(); si++) {
					}
				}
			}
		}*/

		int    dimx = PPGdsCls2::eX;
		int    dimy = PPGdsCls2::eY;
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		Goods2Tbl::Rec goods_rec;
		GoodsExtTbl::Rec goodsext_rec;
		PPIDArray raw_id_list;
		RAssocArray raw_subst_list;
		if(GObj.IsGeneric(P.RawI.GoodsID)) {
			GObj.GetGenericList(P.RawI.GoodsID, &raw_id_list);
		}
		GObj.GetStrucSubstList(P.RawI.GoodsID, raw_subst_list);
		raw_subst_list.GetKeyList(raw_id_list, false); // Эта функция добавит (не заместит) идентификаторы в raw_id_list
		raw_id_list.sortAndUndup();
		for(uint rli = 0; rli < raw_id_list.getCount(); rli++) {
			const  PPID raw_id = raw_id_list.get(rli);
			bool   local_ok = false;
			if(GObj.Fetch(raw_id, &goods_rec) > 0) {
				if(gc_obj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0 && GObj.P_Tbl->GetExt(raw_id, &goodsext_rec) > 0) {
					SPoint2R raw_size;
					raw_size.Z();
					gc_pack.GetExtDim(&goodsext_rec, dimx, &raw_size.x);
					gc_pack.GetExtDim(&goodsext_rec, dimy, &raw_size.y);
					if(raw_size.GetArea() > 0.0) {
						PPOptiModule::CuttingStock2D::RawSheet sh;
						sh.UedID = UED::SetRaw_Oid(SObjID(PPOBJ_GOODS, raw_id));
						sh.Size.Set(R0i(raw_size.x), R0i(raw_size.y));
						{
							GoodsRestParam gr;
							gr.GoodsID = raw_id;
							gr.LocID = P.RawI.LocID;
							gr.CalcMethod = GoodsRestParam::pcmAvg;
							p_bobj->trfr->GetRest(gr);
							if(gr.Total.Rest > 0.0) {
								sh.Qtty = static_cast<uint>(gr.Total.Rest);
								sh.Cost = gr.Total.Cost;
								eblk.AddSheet(sh);
								local_ok = true;
							}
						}
					}
				}
				if(!local_ok) {
					LotArray lot_list;
					p_bobj->trfr->Rcpt.GetListOfOpenedLots(0, raw_id, P.RawI.LocID, ZERODATE, &lot_list);
					if(lot_list.getCount()) {
						for(uint loti = 0; loti < lot_list.getCount(); loti++) {
							const  ReceiptTbl::Rec & r_lot_rec = lot_list.at(loti);
							const  PPID lot_id = r_lot_rec.ID;
							ObjTagItem tag_item;
							if(p_ref->Ot.GetTag(PPOBJ_LOT, lot_id, PPTAG_LOT_DIMENSIONS, &tag_item) > 0) {
								ReceiptCore::LotDimensions ld;
								tag_item.GetStr(temp_buf);
								ld.FromString(temp_buf);
								if(ld.DimX > 0.0 && ld.DimY > 0.0) {
									PPOptiModule::CuttingStock2D::RawSheet sh;
									sh.UedID = UED::SetRaw_Oid(SObjID(PPOBJ_LOT, lot_id));
									sh.Size.Set(R0i(ld.DimX), R0i(ld.DimY));
									sh.Qtty = static_cast<uint>(r_lot_rec.Rest);
									sh.Cost = r_lot_rec.Cost;
									eblk.AddSheet(sh);
									local_ok = true;
								}
							}
						}
					}
				}
			}
		}
		for(uint dli = 0; dli < P.DL.getCount(); dli++) {
			const  DemandItem & r_di = P.DL.at(dli);
		}
	}
	return ok;
}

int DoOrtCuttingStock2D()
{
	int    ok = -1;
	PrcssrOrtCuttingStock2D::Param p;
	PrcssrOrtCuttingStock2D proc;
	proc.InitParam(&p);
	while(proc.EditParam(&p) > 0) {
		if(!proc.Init(&p)) {
			PPError();
		}
		else if(proc.Run()) {
			ok = 1;
			break;
		}
	}
	return ok;
}