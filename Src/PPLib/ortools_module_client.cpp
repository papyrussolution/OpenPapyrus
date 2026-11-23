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
						SDelay(500);
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
	typedef PPBTE_QP DemandItem;

	struct RawItem {
		RawItem() : GoodsID(0), LocID(0)
		{
		}
		PPID   GoodsID;
		PPID   LocID; // Склад, с которого мы возьмем сырье для раскроя //
	};
	
	class Param {
	public:
		//
		// Returns:
		//   >0 - индекс [1..DL.getCount()] добавленного или измененного элемента
		//    0 - error
		//
		int    AddDemandItem(PPID goodsID, double qtty, double price);
		//
		// Returns:
		//   >0 - индекс [1..DL.getCount()] добавленного или измененного элемента
		//    0 - error
		//
		int    UpdateDemandItem(uint idx/*0..*/, PPID goodsID, double qtty, double price);
		int    RemoveDemandItem(uint idx/*0..*/);
		const  TSVector <DemandItem> & GetDemandList() const { return DL; }
		bool   GetDemandQtty(PPID goodsID, double * pQtty) const;

		RawItem RawI;
	private:
		int    Helper_UpdateDemandItemWithSameGoods(uint idx/*0..*/, PPID goodsID, double qtty, double price);
		TSVector <DemandItem> DL;
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

bool PrcssrOrtCuttingStock2D::Param::GetDemandQtty(PPID goodsID, double * pQtty) const
{
	bool   ok = false;
	double qtty = 0.0;
	for(uint i = 0; i < DL.getCount(); i++) {
		const  DemandItem & r_di = DL.at(i);
		if(r_di.GoodsID == goodsID) {
			qtty += r_di.Qtty;
			ok = true;
		}
	}
	ASSIGN_PTR(pQtty, qtty);
	return ok;
}

int PrcssrOrtCuttingStock2D::Param::Helper_UpdateDemandItemWithSameGoods(uint idx/*0..*/, PPID goodsID, double qtty, double price)
{
	int    result = 0;
	if(idx < DL.getCount()) {
		DemandItem & r_di = DL.at(idx);
		assert(r_di.GoodsID == goodsID);
		if(r_di.GoodsID == goodsID) {
			const  double sum_qtty = (r_di.Qtty + qtty);
			if(price <= 0.0) {
				;
			}
			else if(r_di.Price <= 0.0) {
				r_di.Price = price;
			}
			else {
				assert(price > 0.0 && r_di.Price > 0.0);
				if(sum_qtty > 0.0) {
					r_di.Price = ((r_di.Price * r_di.Qtty) + (price * qtty)) / sum_qtty;
				}
				r_di.Qtty = sum_qtty;
			}
			result = static_cast<int>(idx+1);
		}
	}
	return result;
}

int PrcssrOrtCuttingStock2D::Param::AddDemandItem(PPID goodsID, double qtty, double price)
{
	int    result = 0;
	if(goodsID > 0 && qtty > 0.0 && price >= 0.0) {
		uint   ex_idx = 0;
		if(DL.lsearch(&goodsID, &ex_idx, CMPF_LONG)) {
			result = Helper_UpdateDemandItemWithSameGoods(ex_idx, goodsID, qtty, price);
		}
		else {
			DemandItem di;
			di.GoodsID = goodsID;
			di.Qtty = qtty;
			di.Price = price;
			if(DL.insert(&di)) {
				result = DL.getCountI();
			}
		}
	}
	return result;
}

int PrcssrOrtCuttingStock2D::Param::UpdateDemandItem(uint idx/*0..*/, PPID goodsID, double qtty, double price)
{
	int    result = 0;
	if(idx < DL.getCount()) {
		if(goodsID > 0 && qtty > 0.0 && price >= 0.0) {
			DemandItem & r_di = DL.at(idx);
			if(goodsID == r_di.GoodsID) {
				r_di.Qtty = qtty;
				r_di.Price = price;
				result = static_cast<int>(idx+1);
			}
			else {
				uint   ex_idx = 0;
				if(DL.lsearch(&goodsID, &ex_idx, CMPF_LONG)) {
					result = Helper_UpdateDemandItemWithSameGoods(ex_idx, goodsID, qtty, price);
				}
				else {
					r_di.GoodsID = goodsID;
					r_di.Qtty = qtty;
					r_di.Price = price;
					result = static_cast<int>(idx+1);
				}
			}
		}
	}
	return result;
}

int PrcssrOrtCuttingStock2D::Param::RemoveDemandItem(uint idx/*0..*/)
{
	int    ok = 0;
	if(idx < DL.getCount()) {
		DL.atFree(idx);
		ok = 1;
	}
	return ok;
}

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
			for(uint i = 0; i < Data.GetDemandList().getCount(); i++) {
				const  DemandItem & r_item = Data.GetDemandList().at(i);
				ss.Z();
				GetGoodsName(r_item.GoodsID, temp_buf);
				ss.add(temp_buf);
				temp_buf.Z().Cat(r_item.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ));
				ss.add(temp_buf);
				temp_buf.Z().Cat(r_item.Price, MKSFMTD_020);
				ss.add(temp_buf);
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
				int new_idx = Data.AddDemandItem(new_item.GoodsID, new_item.Qtty, new_item.Price);
				if(new_idx > 0) {
					pos = new_idx-1;
					id = new_idx;
					ok = 1;
				}
			}
			ASSIGN_PTR(pPos, pos);
			ASSIGN_PTR(pID, id);
			return ok;
		}
		virtual int editItem(long pos, long id)
		{
			int    ok = -1;
			if(pos >= 0 && pos < Data.GetDemandList().getCountI()) {
				DemandItem _item = Data.GetDemandList().at(pos);
				if(EditItem(_item) > 0) {
					if(Data.UpdateDemandItem(pos, _item.GoodsID, _item.Qtty, _item.Price) > 0) 
						ok = 1;
				}
			}
			return ok;
		}
		virtual int delItem(long pos, long id)
		{
			int    ok = -1;
			if(pos >= 0 && pos < Data.GetDemandList().getCountI()) {
				Data.RemoveDemandItem(pos);
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
		if(pParam->RawI.GoodsID && pParam->GetDemandList().getCount()) {
			P = *pParam;
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
	PPOptiModule::CuttingStock2D::Config model_cfg;
	PPOptiModule::Config opti_cfg;
	{
		opti_cfg.TimeLimitMSec = 300000;
		opti_cfg.ThreadCount   = 8;
		eblk.SetModuleConfig(opti_cfg);
	}
	{
		model_cfg.Flags |= PPOptiModule::CuttingStock2D::Config::fGuillotineCut/*|PPOptiModule::CuttingStock2D::Config::fTexturedMaterial*/;
		model_cfg.GuillotineLevels = 3;
		model_cfg.KerfWidth = 4; // mm
		model_cfg.Objv = PPOptiModule::CuttingStock2D::objvMinWaste;
		eblk.SetConfig(model_cfg);
	}
	SString out_buf;
	SString temp_buf;
	SString debug_output_file_path;
	PPGetFilePath(PPPATH_OUT, "PrcssrOrtCuttingStock2D_Debug_Output", debug_output_file_path);
	SFile f_debug_out(debug_output_file_path, SFile::mWrite);
	if(P.RawI.GoodsID && P.GetDemandList().getCount()) {
		PPObjGoodsStruc gs_obj;
		LongArray compl_path_result_list; // @debug
		TSCollection <PPObjGoodsStruc::ComplPath> compl_path_list;
		{
			for(uint dli = 0; dli < P.GetDemandList().getCount(); dli++) {
				const  DemandItem & r_di = P.GetDemandList().at(dli);
				if(gs_obj.FindComplPath(r_di.GoodsID, P.RawI.GoodsID, &compl_path_list) > 0) {
					compl_path_result_list.add(r_di.GoodsID);
				}
			}
			if(f_debug_out.IsValid()) {
				out_buf.Z();
				for(uint i = 0; i < compl_path_list.getCount(); i++) {
					const  PPObjGoodsStruc::ComplPath * p_path = compl_path_list.at(i);
					if(p_path) {
						out_buf.Cat("Path").Space().CatChar('#').Cat(i+1).CatDiv(':', 0).CR();
						for(uint j = 0; j < p_path->getCount(); j++) {
							const PPObjGoodsStruc::ComplPathEntry & r_cpe = p_path->at(j);
							GetGoodsName(r_cpe.GoodsID, temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
							out_buf.Tab().CatEq("StrucID", r_cpe.GsID).Space().CatParStr(temp_buf).Space().
								CatEq("Qtty", r_cpe.Qtty, MKSFMTD(0, 3, 0)).CR();
						}
					}
				}
				f_debug_out.WriteLine(out_buf);
			}
		}

		int    dimx = PPGdsCls2::eX;
		int    dimy = PPGdsCls2::eY;
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		Goods2Tbl::Rec goods_rec;
		GoodsExtTbl::Rec goodsext_rec;
		PPIDArray raw_id_list;
		{
			RAssocArray raw_subst_list;
			if(GObj.IsGeneric(P.RawI.GoodsID)) {
				GObj.GetGenericList(P.RawI.GoodsID, &raw_id_list);
			}
			else {
				raw_id_list.add(P.RawI.GoodsID);
			}
			GObj.GetStrucSubstList(P.RawI.GoodsID, raw_subst_list);
			raw_subst_list.GetKeyList(raw_id_list, false); // Эта функция добавит (не заместит) идентификаторы в raw_id_list
			raw_id_list.sortAndUndup();
		}
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
								if(f_debug_out.IsValid()) {
									out_buf.Z();
									out_buf.Cat("Raw").Space().CatChar('#').Cat(eblk.SheetList.getCount()).CatDiv(':', 2);
									GetGoodsName(raw_id, temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
									out_buf.CatParStr(temp_buf).Space().Cat("Size").Eq().Cat(sh.Size.x).CatDiv('x', 1).Cat(sh.Size.y).Space();
									out_buf.CatEq("Qtty", sh.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).Space().CatEq("Cost", sh.Cost, MKSFMTD_020).CR();
									f_debug_out.WriteLine(out_buf);
								}
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
									if(f_debug_out.IsValid()) {
										out_buf.Z();
										out_buf.Cat("Raw").Space().CatChar('#').Cat(eblk.SheetList.getCount()).CatDiv(':', 2);
										GetGoodsName(raw_id, temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
										temp_buf.Space().Cat("Lot").Space().CatChar('#').Cat(lot_id);
										out_buf.CatParStr(temp_buf).Space().Cat("Size").Eq().Cat(sh.Size.x).CatDiv('x', 1).Cat(sh.Size.y).Space();
										out_buf.CatEq("Qtty", sh.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).Space().CatEq("Cost", sh.Cost, MKSFMTD_020).CR();
										f_debug_out.WriteLine(out_buf);
									}
									local_ok = true;
								}
							}
						}
					}
				}
			}
		}
		{
			for(uint i = 0; i < compl_path_list.getCount(); i++) {
				const  PPObjGoodsStruc::ComplPath * p_path = compl_path_list.at(i);
				uint   pidx = p_path ? p_path->getCount() : 0;
				bool   local_ok = false;
				if(pidx >= 2) {
					// Первый элемент пути это заданный непосредственно продукт для производства (в P.GetDemandList() должен содержаться соответсвующий товар).
					// Здесь он нам нужен для получения первичного количества (see below)
					const  PPObjGoodsStruc::ComplPathEntry & r_first_cpe = p_path->at(0);
					double demand_qtty = 0.0;
					const  bool gdqr = P.GetDemandQtty(r_first_cpe.GoodsID, &demand_qtty);
					assert(gdqr);
					if(gdqr && demand_qtty > 0.0) {
						pidx--; // последний элемент - сырьевая поверхность
						do {
							//
							// Деталь, которую необходимо выкроить мы будем искать начиная с конца пути. Соображения вот какие:
							//   - в начале пути - конкретная потребность (P.GetDemandList())
							//   - последний элемент пути - сырьевая поверхность, которую будем резать
							//   - скорее всего, наша деталь - предпоследний элемент, но может быть и нечто чуть ближе к конечному продукту.
							//     В любом случае, мы детектируем деталь как только попадем на компонент, у которого есть координаты x и y.
							//
							const  PPObjGoodsStruc::ComplPathEntry & r_cpe = p_path->at(--pidx);
							bool   local_ok = false;
							if(GObj.Fetch(r_cpe.GoodsID, &goods_rec) > 0) {
								if(gc_obj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0 && GObj.P_Tbl->GetExt(r_cpe.GoodsID, &goodsext_rec) > 0) {
									SPoint2R raw_size;
									raw_size.Z();
									gc_pack.GetExtDim(&goodsext_rec, dimx, &raw_size.x);
									gc_pack.GetExtDim(&goodsext_rec, dimy, &raw_size.y);
									if(raw_size.GetArea() > 0.0) {
										PPOptiModule::CuttingStock2D::BomItem bomitem;
										bomitem.UedID = UED::SetRaw_Oid(SObjID(PPOBJ_GOODS, r_cpe.GoodsID));
										bomitem.Size.Set(R0i(raw_size.x), R0i(raw_size.y));
										{
											bomitem.Qtty = R0i(r_cpe.Qtty * demand_qtty);
											bomitem.Cost = 0.0;
											eblk.AddBomItem(bomitem);
											if(f_debug_out.IsValid()) {
												out_buf.Z();
												out_buf.Cat("Item").Space().CatChar('#').Cat(eblk.BomList.getCount()).CatDiv(':', 2);
												GetGoodsName(r_cpe.GoodsID, temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
												out_buf.CatParStr(temp_buf).Space().Cat("Size").Eq().Cat(bomitem.Size.x).CatDiv('x', 1).Cat(bomitem.Size.y).Space();
												out_buf.CatEq("Qtty", bomitem.Qtty, MKSFMTD(0, 3, NMBF_NOTRAILZ)).Space().CatEq("Cost", bomitem.Cost, MKSFMTD_020).CR();
												f_debug_out.WriteLine(out_buf);
											}
											local_ok = true;
										}
									}
								}
							}
						} while(!local_ok && pidx);
					}
				}
			}
		}
		{
			PPOptiModule::BaseExecutionResult * p_result = PPOptiModule::ClientExecution(eblk);
			if(p_result && p_result->IsSuccess()) {
				SString svg_out;
				PPOptiModule::CuttingStock2D::GenerateSvg(eblk, *static_cast<PPOptiModule::CuttingStock2D::Result *>(p_result), 1.0, svg_out);
				if(svg_out.NotEmpty()) {
					SString svg_file_name;
					PPGetFilePath(PPPATH_OUT, "PPOptiModule_CuttingStock2D_Test.svg", svg_file_name);
					SFile f_out(svg_file_name, SFile::mWrite);
					if(f_out.IsValid()) {
						f_out.WriteLine(svg_out);
					}
				}
			}
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