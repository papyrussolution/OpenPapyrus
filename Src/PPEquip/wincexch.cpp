// WINCEXCH.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2011, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//# @v11.9.12 include <sbht.h>
#include <stylobhtII.h>
//
// @v11.8.12 inlined sbht.h {
//
struct _LDATETIME {
	signed long t;
	signed long d;
};

struct SBHTCmdBuf { // @persistent
	enum {
		cmCheckConnection = 1,
		cmGetGoods        = 2,
		cmPutTSessLine    = 3,
		cmLogout          = 4
	};
	signed short  Cmd;
	signed short  Reserve;
	signed long   RetCode;
	unsigned long BufSize;
};

struct SBHTTSessLineRec {
	long   BillID;
	char   PrcCode[9];
	char   ArCode[9];
	signed short  Reserve;
	char   Serial[32];
	_LDATETIME Dtm;
	double Qtty;
};

struct SBHTGoodsRec {
	long ID;
	char Name[32];
};
//
// } @v11.8.12 inlined sbht.h
//
int RecvBuf(TcpSocket * pSo, void * pBuf, size_t bufSize)
{
	int    ok = -1;
	if(pSo && bufSize) {
		size_t rcv_bytes = 0, total_rcv_bytes = 0;
		while(total_rcv_bytes < bufSize) {
			THROW_SL(pSo->Recv(PTR8(pBuf) + total_rcv_bytes, bufSize - total_rcv_bytes, &rcv_bytes));
			total_rcv_bytes += rcv_bytes;
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SendCmd(TcpSocket * pSo, SBHTCmdBuf * pBuf, void * pParam)
{
	int    ok = -1;
	if(pSo) {
		THROW_SL(pSo->Send(pBuf, sizeof(SBHTCmdBuf), 0));
		if(pParam && pBuf->BufSize)
			THROW_SL(pSo->Send(pParam, pBuf->BufSize, 0));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

static int MakeParam(const void * pInBuf, char ** ppOutBuf, size_t bufSize)
{
	int    ok = 1;
	delete [] *ppOutBuf;
	*ppOutBuf = 0;
	if(bufSize) {
		THROW_MEM(*ppOutBuf = new char[bufSize]);
		memcpy(*ppOutBuf, pInBuf, bufSize);
	}
	CATCHZOK
	return ok;
}

int StyloBHTExch(TcpSocket * pSo)
{
	int    ok = 1, stop = 0;
	int    r = 1;
	char * p_param_buf = 0;
	SString temp_buf;
	while(!stop) {
		size_t in_buf_size = 0, out_buf_size = 0;
		SBHTCmdBuf cmd_buf;
		MEMSZERO(cmd_buf);
		THROW(RecvBuf(pSo, &cmd_buf, sizeof(cmd_buf)));
		in_buf_size = cmd_buf.BufSize;
		r = -1;
		switch(cmd_buf.Cmd) {
			case SBHTCmdBuf::cmCheckConnection:
				r = 1;
				break;
			case SBHTCmdBuf::cmGetGoods:
				{
					char code[256];
					SBHTGoodsRec rec;
					BarcodeTbl::Rec bc_rec;
					PPObjGoods gobj;
					memzero(code, sizeof(code));
					THROW(RecvBuf(pSo, code, in_buf_size));
        			if(gobj.SearchByBarcode(code, &bc_rec, 0, 1) > 0) {
						rec.ID = bc_rec.GoodsID;
						r = 1;
					}
					else {
						PPObjTSession tsobj;
						PPObjTSession::SelectBySerialParam ssp(0, code);
						int    r2 = tsobj.SelectBySerial(&ssp);
						if(r2 == 1) {
							rec.ID = ssp.GoodsID;
							r = 1;
						}
						else if(r2 == 2) {
							PPSetError(PPERR_SERIALUSED, code);
							r = 0;
						}
						else if(r2 == -2) {
							//
							// Лот с серийным номером найден, но находится на другом складе либо
							// закончился. Следовательно, даем сообщение в журнал, и инициализируем
							// ИД товара, количество. Серийный номер не используем.
							//
							rec.ID = ssp.GoodsID;
							r = 1;
						}
					}
					if(r > 0) {
						GetGoodsName(rec.ID, temp_buf);
						temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(rec.Name, sizeof(rec.Name));
						out_buf_size = sizeof(rec);
					}
					THROW(MakeParam(&rec, &p_param_buf, out_buf_size));
				}
				break;
			case SBHTCmdBuf::cmPutTSessLine:
				{
					SBHTTSessLineRec rec;
					THROW_PP(in_buf_size == sizeof(rec), PPERR_INVRECSIZE);
					THROW(RecvBuf(pSo, &rec, in_buf_size));
				}
				break;
			case SBHTCmdBuf::cmLogout:
				stop = 1;
				break;
		}
		cmd_buf.RetCode = r;
		cmd_buf.BufSize = out_buf_size;
		THROW(SendCmd(pSo, &cmd_buf, p_param_buf));
	}
	CATCHZOK
	delete [] p_param_buf;
	return ok;
}
//
// StyloBhtIIExchange
//

#if 0 // {
class StyloBhtIIExchanger {
public:
	StyloBhtIIExchanger(TcpSocket * pSo);
	~StyloBhtIIExchanger();
	int    Run();
private:
	int    GetTable(int16 Cmd, uint fileNameCode, const char * pTblInfo, SBIIRec * pRec, long nextRecNo = -1);
	int    SetTable(int16 cmd, uint fileNameCode, const char * pTblInfo, SBIIRec * pRec, long count);
	int    GetGoods();
	int    GetArticles();
	int    PrepareBills(int uniteGoods);
	int    GetBills();
	int    GetBillRows();
	int    GetBillRowsWithCells(long billID);
	int    GetOpRestrictions();
	int    GetConfig(StyloBhtIIConfig * pCfg);
	int    GetGoodsList(long cellID, int getGoods);
	int    SetBills(long count);
	int    SetBillRows(long count);
	int    AcceptLocOp(SBIILocOp * pRec);
	int    FindGoods(PPID goodsID, const char * pBarcode, SBIIGoodsRec * pRec);
	int    FindLocCell(PPID locID, const char * pName, SBIILocCellRec * pRec);
	int    PrintBarcode(const char * pBarcode);
	int    SendCmd(int16 cmd, int32 retcode, const void * pBuf, size_t bufSize);
	int    RecvCommand(void * pBuf, size_t bufSize, size_t * pRecvBytes = 0);
	int    GetReply();
	int    Log(uint errCode, uint msgCode, const char * pAddInfo);
	int    Log(uint errCode, uint msgCode, const char * pAddInfo, long count, long total);

	SString DeviceDir;
	TcpSocket * P_So;
	StyloBhtIIConfig Cfg;
	PPBhtTerminalPacket BhtPack;
	PPObjGoods GObj;
	PPObjLocation LocObj;
	LocTransfCore LocTransf;
	PPGoodsConfig CfgGoods;
};
#endif // } 0

StyloBhtIIExchanger::StyloBhtIIExchanger(/*TcpSocket * pSo*/)
{
	//P_So = pSo;
	P_LocTransf = 0;
}

StyloBhtIIExchanger::~StyloBhtIIExchanger()
{
	delete P_LocTransf;
}

int StyloBhtIIExchanger::SendCmd(TcpSocket & rSo, int16 cmd, int32 retcode, const void * pBuf, size_t bufSize)
{
	int    ok = 1;
	size_t snd_size = 0;
	SBhtIICmdBuf cmd_buf;
	cmd_buf.Cmd     = cmd;
	cmd_buf.RetCode = retcode;
	cmd_buf.BufSize = bufSize;
	THROW_SL(rSo.Send(&cmd_buf, sizeof(cmd_buf), &snd_size));
	if(bufSize && pBuf) {
		THROW_SL(rSo.Send(pBuf, bufSize, &snd_size));
	}
	CATCHZOK
	return ok;
}

int StyloBhtIIExchanger::RecvCommand(TcpSocket & rSo, void * pBuf, size_t bufSize, size_t * pRecvBytes /*=0*/)
{
	int    ok = 1;
	size_t rcvd_sz = 0;
	SBhtIICmdBuf cmd_buf;
	THROW_SL(rSo.RecvBlock(&cmd_buf, sizeof(cmd_buf), &rcvd_sz));
	THROW_PP_S(cmd_buf.RetCode == 1, PPERR_SBII_INVRETCODE, cmd_buf.RetCode);
	if(pBuf && cmd_buf.BufSize > 0) {
		THROW_PP(cmd_buf.BufSize <= bufSize, PPERR_INVRECSIZE);
		THROW_SL(rSo.RecvBlock(pBuf, cmd_buf.BufSize, &rcvd_sz));
	}
	ASSIGN_PTR(pRecvBytes, cmd_buf.BufSize);
	CATCHZOK
	return ok;
}

int StyloBhtIIExchanger::GetConfig(StyloBhtIIConfig * pCfg)
{
	int    ok = 1;
	SString fname, path;
	SFile file;
	StyloBhtIIConfig cfg;
	MEMSZERO(cfg);
	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	PPGetFileName(PPFILNAM_BHT_CONFIG, fname);
 	SFsPath::ReplaceExt(fname, "dat", 1);
	(path = DeviceDir).Cat(fname);
	/* Не будем трогать файл конфигурации из папки обмена
	if(fileExists(path)) {
		THROW_SL(file.Open(path, SFile::mRead|SFile::mBinary));
		THROW_SL(file.Read(&cfg, sizeof(StyloBhtIIConfig)));
		cfg.ToHost();
		ASSIGN_PTR(pCfg, cfg);
	}
	else*/
	//
	// Будем каждый раз готовить файл конфигурации
	//
	if(BhtPack.Rec.BhtTypeID == PPObjBHT::btStyloBhtII) {
		PPObjBHT bht_obj;
		BhtPack.ConvertToConfig(-1, &cfg);
		THROW(bht_obj.PrepareConfigData(&BhtPack, &cfg));
		ASSIGN_PTR(pCfg, cfg);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int StyloBhtIIExchanger::GetTable(TcpSocket & rSo, int16 cmd, uint fileNameCode, const char * pTblInfo, SBIIRec * pRec, long nextRecNo)
{
	int    ok = 1;
	long   i = 0, recs_count = 0;
	char * p_pack_buf = 0;
	SString fname, path;
	DbfTable * p_tbl = 0;
	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	THROW_INVARG(pRec);
	PPGetFileName(fileNameCode, fname);
	SFsPath::ReplaceExt(fname, "dbf", 1);
	(path = DeviceDir).Cat(fname);
	THROW_MEM(p_tbl = new DbfTable(path));
	{
		recs_count = p_tbl->getNumRecs();
		if(nextRecNo < 0) {
			THROW(SendCmd(rSo, cmd, 1, &recs_count, sizeof(recs_count)));
		}
		else if(recs_count > 0 && p_tbl->top()) {
			int    stop = 0;
			long   recs_sended = 0;
			long   rec_no = 0;
			if(nextRecNo >= 0) {
				recs_sended = nextRecNo;
				rec_no = nextRecNo;
				if(rec_no > 0)
					p_tbl->goToRec(rec_no);
			}
			const size_t pack_buf_size = pRec->GetSize() * MAXRECS_IN_PACKBUF;
			THROW_MEM(p_pack_buf = new char[pack_buf_size]);
			{
				size_t offs = 0;
				// long   idx = 0;
				// @v8.4.5 THROW(RecvCommand(rSo, &idx, sizeof(idx)));
				for(uint j = 0; j < MAXRECS_IN_PACKBUF && rec_no < recs_count; j++) {
					size_t buf_size = pack_buf_size - offs;
					pRec->FromDbfTbl(p_tbl);
					pRec->ToBuf((p_pack_buf + offs), &buf_size);
					offs += pRec->GetSize();
					rec_no++;
					if(!p_tbl->next()) {
						assert(rec_no == recs_count);
						break;
					}
				}
				THROW(SendCmd(rSo, cmd, 1, p_pack_buf, offs));
				recs_sended += rec_no;
			}
		}
	}
	CATCH
		PPError();
		Log_(PPErrCode, 0, DS.GetTLA().AddedMsgString);
		ok = 0;
	ENDCATCH
	ZDELETE(p_tbl);
	Log_(0, PPTXT_SBIIEXPORTOK, pTblInfo, i, recs_count);
	ZDELETE(p_pack_buf);
	return ok;
}

int StyloBhtIIExchanger::GetGoodsList(TcpSocket & rSo, long cellID, int getGoods)
{
	int16  cmd = getGoods ? SBhtIICmdBuf::cmGoodsListByCell : SBhtIICmdBuf::cmCellListByGoods;
	int    ok = 1;
	long recs_count = 0, i = 0;
	char * p_pack_buf = 0;
	SBIIRec * p_rec = 0;

	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	if(getGoods) {
		THROW_MEM(p_rec = new SBIIGoodsRec);
	}
	else {
		THROW_MEM(p_rec = new SBIILocCellRec);
	}
	{
		size_t pack_buf_size = p_rec->GetSize() * MAXRECS_IN_PACKBUF;
		RAssocArray list;
		SETIFZ(P_LocTransf, new LocTransfCore);
		THROW_MEM(P_LocTransf);
		//LocTransfCore loct_tbl;

		THROW_MEM(p_pack_buf = new char[pack_buf_size]);
		if(getGoods)
			P_LocTransf->GetGoodsList(cellID, &list);
		else
			P_LocTransf->GetLocCellList(cellID, LConfig.Location, &list);
		recs_count = list.getCount();
		THROW(SendCmd(rSo, cmd, 1, &recs_count, sizeof(recs_count)));
		for(i = 0; i < recs_count; i++) {
			int    stop = 0;
			long   idx = 0;
			size_t offs = 0;
			THROW(RecvCommand(rSo, &idx, sizeof(idx)));
			for(uint j = 0; !stop && j < MAXRECS_IN_PACKBUF; j++) {
				size_t buf_size = pack_buf_size - offs;
				RAssoc & r_item = list.at(i);
				if(getGoods) {
					THROW(FindGoods(r_item.Key, 0, (SBIIGoodsRec*)p_rec) > 0);
					((SBIIGoodsRec*)p_rec)->Rest = r_item.Val;
				}
				else {
					THROW(FindLocCell(r_item.Key, 0, (SBIILocCellRec*)p_rec) > 0);
					((SBIILocCellRec*)p_rec)->Qtty = r_item.Val;
				}
				p_rec->ToBuf((p_pack_buf + offs), &buf_size);
				offs += p_rec->GetSize();
				i++;
				stop = BIN(i >= recs_count);
			}
			THROW(SendCmd(rSo, cmd, 1, p_pack_buf, offs));
		}
	}
	CATCHZOK
	Log_(0, PPTXT_SBIIEXPORTOK, "GetGoodsList", i, recs_count);
	ZDELETE(p_rec);
	ZDELETE(p_pack_buf);
	return ok;
}

int StyloBhtIIExchanger::PrepareBills(int uniteGoods)
{
	int    ok = 1;
	SString fname, path;
	SFile file;
	PPObjBHT bht_obj;
	StyloBhtIIConfig cfg;
	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	THROW(bht_obj.PrepareBillData2(&BhtPack, 0, uniteGoods));
	if(GetConfig(&cfg) > 0) {
		PPGetFileName(PPFILNAM_BHT_CONFIG, fname);
 		SFsPath::ReplaceExt(fname, "dat", 1);
		(path = DeviceDir).Cat(fname);
		getcurdatetime(&cfg.BillLastExch);
		THROW_SL(file.Open(path, SFile::mWrite|SFile::mBinary));
		THROW_SL(file.Write(&cfg, sizeof(StyloBhtIIConfig)));
	}
	CATCHZOK
	return ok;
}

int StyloBhtIIExchanger::SetTable(TcpSocket & rSo, int16 cmd, uint fileNameCode, const char * pTblInfo, SBIIRec * pRec, long count)
{
	int    ok = 1;
	long   i = 0;
	char * p_pack_buf = 0;
	SString fname, path;
	DbfTable * p_tbl = 0;
	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	THROW_INVARG(pRec);
	THROW(SendCmd(rSo, cmd, 1, 0, 0));
	if(count > 0) {
		size_t pack_buf_size = pRec->GetSize() * MAXRECS_IN_PACKBUF;
		PPGetFileName(fileNameCode, fname);
		SFsPath::ReplaceExt(fname, "dbf", 1);
		(path = DeviceDir).Cat(fname);
		THROW_MEM(p_tbl = pRec->CreateDbfTbl(path));
		THROW_MEM(p_pack_buf = new char[pack_buf_size]);
		for(i = 0; i < count;) {
			size_t recs_in_packbuf = 0, recv_bytes = 0, offs = 0;
			THROW(RecvCommand(rSo, p_pack_buf, pack_buf_size, &recv_bytes));
			THROW_PP(recs_in_packbuf = (recv_bytes / pRec->GetSize()), PPERR_INVRECSIZE);
			for(size_t j = 0; j < recs_in_packbuf; j++) {
				pRec->FromBuf(p_pack_buf+offs);
				offs += pRec->GetSize();
				pRec->ToDbfTbl(p_tbl);
				i++;
			}
			THROW(SendCmd(rSo, cmd, 1, 0, 0));
		}
	}
	CATCHZOK
	ZDELETE(p_pack_buf);
	ZDELETE(p_tbl);
	Log_(0, PPTXT_SBIIIMPORTOK, pTblInfo, i, count);
	return ok;
}

int StyloBhtIIExchanger::FindGoods(PPID goodsID, const char * pBarcode, SBIIGoodsRec * pRec)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	int    is_serial = 0;
	double rest = 0;
	PPID   goods_id = 0;
	PPID   loc_id = 0;
	SString temp_buf;
	PPIDArray lot_list;
	Goods2Tbl::Rec goods_rec;
	GoodsCodeSrchBlock srch_blk;
	ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
	if(pBarcode) {
		STRNSCPY(srch_blk.Code, pBarcode);
		srch_blk.Flags = GoodsCodeSrchBlock::fGoodsId;
		if(GObj.SearchByCodeExt(&srch_blk) > 0)
			goods_id = srch_blk.Rec.ID;
		else if(p_bobj->SearchLotsBySerial(pBarcode, &lot_list) > 0 && lot_list.getCount()) {
			ReceiptTbl::Rec lot_rec;
			THROW(r_rcpt.Search(lot_list.at(0), &lot_rec) > 0);
			goods_id = lot_rec.GoodsID;
			rest = lot_rec.Rest;
			is_serial = 1;
		}
	}
	else
		goods_id = goodsID;
	if(goods_id && GObj.Fetch(goods_id, &goods_rec) > 0) {
		GoodsStockExt gse;
		ReceiptTbl::Rec lot_rec;
		SBIIGoodsRec sbii_grec;
		sbii_grec.ID = goods_id;
		if(is_serial)
			STRNSCPY(sbii_grec.Serial,  pBarcode);
		else
			STRNSCPY(sbii_grec.Barcode, pBarcode);
		if(isempty(sbii_grec.Barcode)) {
			GObj.GetSingleBarcode(goods_id, 0, temp_buf);
			STRNSCPY(sbii_grec.Barcode, temp_buf);
		}
		if(goods_rec.Flags & GF_UNLIM) {
			GObj.GetQuot(goods_id, QuotIdent(loc_id, PPQUOTK_BASE, 0L/*@curID*/), 0L, 0L, &sbii_grec.Cost);
		}
		else
			::GetCurGoodsPrice(goods_id, loc_id, GPRET_INDEF, &sbii_grec.Cost, &lot_rec);
		if(GObj.GetStockExt(goods_id, &gse) > 0 && gse.Package > 0)
			sbii_grec.Pack = gse.Package;
		else
			sbii_grec.Pack = lot_rec.UnitPerPack;
		sbii_grec.Rest = rest ? rest : lot_rec.Rest;
		STRNSCPY(sbii_grec.Name, goods_rec.Name);
		SOemToChar(sbii_grec.Name);
		if(sstrlen(sbii_grec.Barcode) < 7) {
			(temp_buf = sbii_grec.Barcode).PadLeft(12-temp_buf.Len(), '0');
			if(GObj.GetConfig().Flags & GCF_BCCHKDIG)
				AddBarcodeCheckDigit(temp_buf);
			temp_buf.CopyTo(sbii_grec.Barcode, sizeof(sbii_grec.Barcode));
		}
		ASSIGN_PTR(pRec, sbii_grec);
		ok = 1;
	}
	CATCHZOK
	if(ok <= 0)
		Log_(0, PPTXT_SBIIGOODSNOTFOUND, pBarcode);
	return ok;
}

int StyloBhtIIExchanger::FindLocCell(PPID locID, const char * pName, SBIILocCellRec * pRec)
{
	int    ok = -1;
	uint   i = 0, count = 0;
	LocationFilt filt(LOCTYP_WHZONE, 0, LConfig.Location);
	StrAssocArray * p_list = 0;
	if(locID) {
		p_list = new StrAssocArray;
		CALLPTRMEMB(p_list, Add(locID, 0, 0));
	}
	else
		p_list = LocObj.MakeList_(&filt);
	if(p_list && (count = p_list->getCount())) {
		LocationTbl::Rec loc_rec;
		for(uint i = 0; ok < 0 && i < count; i++ ) {
			if(LocObj.Fetch(p_list->Get(i).Id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WHCELL && (!pName || stricmp866(pName, loc_rec.Name) == 0)) {
				SBIILocCellRec sbii_lrec;
				sbii_lrec.ID = loc_rec.ID;
				STRNSCPY(sbii_lrec.Code, loc_rec.Code);
				STRNSCPY(sbii_lrec.Name, loc_rec.Name);
				ASSIGN_PTR(pRec, sbii_lrec);
				ok = 1;
			}
		}
	}
	if(ok <= 0)
		Log_(0, PPTXT_SBIILOCCELLNOTFOUND, pName);
	return ok;
}

int StyloBhtIIExchanger::AcceptLocOp(SBIILocOp * pRec)
{
	int    ok = -1;
	THROW_INVARG(pRec);
	SETIFZ(P_LocTransf, new LocTransfCore);
	THROW_MEM(P_LocTransf);
	{
		LocTransfOpBlock op_block((int)pRec->Op, pRec->LocCellID);
		op_block.GoodsID = pRec->GoodsID;
		op_block.Qtty    = pRec->Qtty;
		op_block.BillID  = pRec->BillID;
		op_block.RByBill = pRec->RByBill;
		THROW(P_LocTransf->ValidateOpBlock(op_block));
		THROW(P_LocTransf->PutOp(op_block, 0, 1));
		ok = 1;
	}
	CATCHZOK
	if(ok == 0)
		Log_(DS.GetTLA().LastErr, 0, DS.GetTLA().AddedMsgString);
	return ok;
}

int StyloBhtIIExchanger::PrintBarcode(const char * pBarcode)
{
	int    ok = -1;
	PPObjBill * p_bobj = BillObj;
	bool   is_serial = false;
	PPID   goods_id = 0;
	SString temp_buf;
	PPIDArray lot_list;
	ReceiptTbl::Rec lot_rec;
	RetailGoodsInfo rgi;
	GoodsCodeSrchBlock srch_blk;
	ReceiptCore & r_rcpt = p_bobj->trfr->Rcpt;
	THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
	STRNSCPY(srch_blk.Code, pBarcode);
	srch_blk.Flags = GoodsCodeSrchBlock::fGoodsId;
	if(GObj.SearchByCodeExt(&srch_blk) > 0)
		goods_id = srch_blk.Rec.ID;
	else if(p_bobj->SearchLotsBySerial(pBarcode, &lot_list) > 0 && lot_list.getCount()) {
		THROW(r_rcpt.Search(lot_list.at(0), &lot_rec) > 0);
		goods_id = lot_rec.GoodsID;
		is_serial = true;
	}
	if(GObj.GetRetailGoodsInfo(goods_id, LConfig.Location, &rgi) > 0) {
		if(is_serial) {
			STRNSCPY(rgi.Serial, pBarcode);
			GObj.GetSingleBarcode(goods_id, 0, temp_buf);
			STRNSCPY(rgi.BarCode, temp_buf);
		}
		else
			STRNSCPY(rgi.BarCode, pBarcode);
		if(sstrlen(rgi.Serial) > 0) {
			rgi.LotID = lot_rec.ID;
			rgi.Qtty = lot_rec.Quantity;
			rgi.UnitPerPack = lot_rec.UnitPerPack;
		}
		rgi.LabelCount = 1;
		ok = BarcodeLabelPrinter::PrintGoodsLabel2(&rgi, BhtPack.P_SBIICfg->BcdPrinterID, 1);
	}
	CATCHZOK
	return ok;
}

int StyloBhtIIExchanger::Log_(uint errCode, uint msgCode, const char * pAddInfo)
{
	LDATETIME dtm;
	SString temp_buf, buf, str_dtm;
	getcurdatetime(&dtm);
	str_dtm.Cat(dtm);
	if(errCode) {
		PPGetMessage(mfError, errCode, pAddInfo, DS.CheckExtFlag(ECF_SYSSERVICE), buf);
		buf.ShiftLeft();
	}
	else {
		PPLoadText(msgCode, temp_buf);
		if(temp_buf.Len()) {
			if(pAddInfo)
				buf.Printf(temp_buf, pAddInfo, str_dtm.cptr());
			else
				buf.Printf(temp_buf, str_dtm.cptr());
		}
	}
	if(DeviceDir.Len()) {
		SFile file;
		SString path, fname;
		PPGetFileName(PPFILNAM_INFO_LOG, fname);
		(path = DeviceDir).Cat(fname);
		file.Open(path, SFile::mAppend);
		if(file.IsValid()) {
			buf.CR();
			if(msgCode == PPTXT_SBIIENDEXCHANGE)
				buf.CR();
			file.WriteLine(buf);
		}
	}
	else {
		PPLogMessage(PPFILNAM_INFO_LOG, buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
	}
	return 1;
}

int StyloBhtIIExchanger::Log_(uint errCode, uint msgCode, const char * pAddInfo, long count, long total)
{
	SString add_info;
	if(total)
		add_info.Cat(count).Slash().Cat(total);
	else
		add_info.Cat(count);
	if(pAddInfo)
		add_info.Space().Cat(pAddInfo);
	return Log_(errCode, msgCode, add_info);
}

int FASTCALL StyloBhtIIExchanger::ProcessSocketInput(TcpSocket & rSo)
{
	int    ok = 1;
	int    r = 0;
	//char * p_param_buf = 0;
	SString temp_buf;
	SBhtIICmdBuf cmd_buf;
	int    reply_sended = 0;
	size_t rcvd_size = 0;
	size_t in_buf_size = 0/*, out_buf_size = 0*/;
	long   recs_count = 0;
	SBuffer in_buf;
	SBuffer ret_buf;
	THROW_SL(rSo.RecvBlock(&cmd_buf, sizeof(cmd_buf), &rcvd_size));
	// @v9.4.11 @debug {
	{
		(temp_buf = BhtPack.Rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
		temp_buf.Space().Cat("SBhtIICmdBuf").CatDiv(':', 2).Cat(cmd_buf.Cmd).Space().Cat(cmd_buf.BufSize).Space();
		PPLogMessage(PPFILNAM_DEBUG_LOG, temp_buf, LOGMSGF_TIME|LOGMSGF_USER);
	}
	// } @v9.4.11 @debug
	{
		const size_t in_buf_size = cmd_buf.BufSize;
		size_t actual_in_buf_size = 0;
		if(in_buf_size) {
			THROW_SL(rSo.RecvBuf(in_buf, in_buf_size, &actual_in_buf_size));
		}
		r = -1;
		switch(cmd_buf.Cmd) {
			case SBhtIICmdBuf::cmCheckConnection:
				r = 1;
				break;
			case SBhtIICmdBuf::cmTestConnection:
				{
					THROW_PP(actual_in_buf_size == in_buf_size, PPERR_SBII_PROT_INVARGSIZE);
					THROW_PP(actual_in_buf_size >= sizeof(uint32), PPERR_SBII_PROT_INVARGSIZE);
					SCRC32 c;
					const uint32 bht_crc = *(uint32 *)in_buf.constptr();
					uint32 this_crc = c.Calc(0, PTR8C(in_buf.constptr())+sizeof(this_crc), in_buf_size-sizeof(this_crc));
					THROW_PP(this_crc == bht_crc, PPERR_SBII_TEST_INVCRC);
					{

						this_crc = 0;
						//THROW_SL(ret_buf.Write(this_crc));
						const size_t out_data_len = SLS.GetTLA().Rg.GetUniformInt((1024*1024)-sizeof(this_crc));
						const uint out_data_dwlen = out_data_len/sizeof(uint32);
						STempBuffer buffer(out_data_len+sizeof(this_crc));
						THROW_SL(buffer.IsValid());
						for(uint i = 0; i < out_data_dwlen; i++) {
							const uint32 out_dword = SLS.GetTLA().Rg.Get();
							PTR32((char *)buffer)[i+1] = out_dword;
							//THROW_SL(ret_buf.Write(out_dword));
						}
						this_crc = c.Calc(0, ((const char *)buffer)+sizeof(this_crc), out_data_len);
						PTR32((char *)buffer)[0] = this_crc;
                        THROW_SL(ret_buf.Write(buffer, buffer.GetSize()));
					}
					r = 1;
				}
				break;
			case SBhtIICmdBuf::cmGetConfig:
				{
					StyloBhtIIConfig cfg;
					r = GetConfig(&cfg);
					if(r > 0) {
                        THROW_SL(ret_buf.Write(&cfg, sizeof(cfg)));
						//THROW(MakeParam(&cfg, &p_param_buf, out_buf_size = sizeof(cfg)));
					}
				}
				break;
			case SBhtIICmdBuf::cmGetGoods:
				{
					SBIIGoodsRec sbii_goods_rec;
					r = GetTable(rSo, SBhtIICmdBuf::cmGetGoods, PPFILNAM_BHT_GOODS, "Goods", &sbii_goods_rec, -1);
					reply_sended = BIN(r > 0);
				}
				break;
			case SBhtIICmdBuf::cmGetArticles:
				{
					SBIIArticleRec sbii_ar_rec;
					r = GetTable(rSo, SBhtIICmdBuf::cmGetArticles, PPFILNAM_BHT_SUPPL, "Articles", &sbii_ar_rec, -1);
					reply_sended = BIN(r > 0);
				}
				break;
			case SBhtIICmdBuf::cmPrepareBills:
				{
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					{
						const long unite_goods = *(const long *)in_buf.constptr();
						r = (PrepareBills(unite_goods) > 0) ? 1 : 0;
					}
				}
				break;
			case SBhtIICmdBuf::cmGetBills:
				{
					SBIISampleBillRec sbii_rec;
					r = GetTable(rSo, SBhtIICmdBuf::cmGetBills, PPFILNAM_BHT_SAMPLEBILLS, "SampleBills", &sbii_rec, -1);
					reply_sended = BIN(r > 0);
				}
				break;
			case SBhtIICmdBuf::cmGetBillRows:
				{
					SBIISampleBillRowRec sbii_rec;
					r = GetTable(rSo, SBhtIICmdBuf::cmGetBillRows, PPFILNAM_BHT_SAMPLEBROWS, "SampleBillRows", &sbii_rec, -1);
					reply_sended = BIN(r > 0);
				}
				break;
			case SBhtIICmdBuf::cmGetBillRowsWithCells:
				{
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					{
						const long bill_id = *static_cast<const long *>(in_buf.constptr());
						SBIIBillRowWithCellsRec sbii_rec;
						PPObjBHT bht_obj;
						StyloBhtIIConfig cfg;
						THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
						THROW(bht_obj.PrepareBillRowCellData(&BhtPack, bill_id));
						r = GetTable(rSo, SBhtIICmdBuf::cmGetBillRowsWithCells, PPFILNAM_BHT_BROWSWCELLS, "BillRowsWithCells", &sbii_rec, -1);
						reply_sended = BIN(r > 0);
					}
				}
				break;
			case SBhtIICmdBuf::cmGetOpRestrictions:
				{
					SBIIOpRestrRec sbii_rec;
					r = GetTable(rSo, SBhtIICmdBuf::cmGetOpRestrictions, PPFILNAM_BHT_OPLIST, "OpRestrictions", &sbii_rec, -1);
					reply_sended = BIN(r > 0);
				}
				break;
			case SBhtIICmdBuf::cmFindGoods:
				{
					char   code[128];
					SBIIGoodsRec sbii_grec;
					strnzcpy(code, (const char *)in_buf.constptr(), MIN(sizeof(code), in_buf.GetAvailableSize()));
					if(FindGoods(0, code, &sbii_grec) > 0) {
						THROW_SL(ret_buf.Write(&sbii_grec, sizeof(sbii_grec)));
						//THROW(MakeParam(&sbii_grec, &p_param_buf, out_buf_size = sizeof(sbii_grec)));
						r = 1;
					}
				}
				break;
			case SBhtIICmdBuf::cmSearchGoodsByCode:
				{
					char   code[128];
					strnzcpy(code, (const char *)in_buf.constptr(), MIN(sizeof(code), in_buf.GetAvailableSize()));
					{
						GoodsCodeSrchBlock srch_blk;
						Goods2Tbl::Rec goods_rec;
						STRNSCPY(srch_blk.Code, code);
						srch_blk.Flags |= GoodsCodeSrchBlock::fAdoptSearch;
						if(GObj.SearchByCodeExt(&srch_blk) > 0 && GObj.Fetch(srch_blk.Rec.ID, &goods_rec) > 0) {
							SOemToChar(goods_rec.Name);
							THROW_SL(ret_buf.Write(goods_rec.ID));
							THROW_SL(ret_buf.Write(goods_rec.Name, sstrlen(goods_rec.Name)));
							r = 1;
						}
					}
				}
				break;
			case SBhtIICmdBuf::cmGetGoodsInfo:
				{
					SBIIGoodsStateInfo ret_blk;
					RetailGoodsInfo rgi;
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					{
						const long goods_id = *static_cast<const long *>(in_buf.constptr());
						const  PPID loc_id = BhtPack.Rec.LocID;
						//SBIIGoodsStateInfo sbii_gsi;
						if(GObj.GetRetailGoodsInfo(goods_id, loc_id, &rgi) > 0) {
							ret_blk.ID = rgi.ID;
							STRNSCPY(ret_blk.Barcode, rgi.BarCode);
							STRNSCPY(ret_blk.Serial, rgi.Serial);
							STRNSCPY(ret_blk.Name, SOemToChar(rgi.Name));
							ret_blk.Cost = rgi.Cost;
							ret_blk.Price = rgi.Price;
							{
								GoodsRestParam gp;
								gp.GoodsID     = goods_id;
								gp.CalcMethod  = GoodsRestParam::pcmSum;
								gp.Date        = ZERODATE;
								gp.LocID       = loc_id;
								BillObj->trfr->GetRest(gp);
								ret_blk.Rest = gp.Total.Rest;
							}
							THROW_SL(ret_buf.Write(&ret_blk, sizeof(ret_blk)));
							r = 1;
						}
					}
				}
				break;
			case SBhtIICmdBuf::cmGetGoodsRecord:
				{
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					{
						const  PPID goods_id = *static_cast<const long *>(in_buf.constptr());
						SBIIGoodsRec sbii_grec;
						if(FindGoods(goods_id, 0, &sbii_grec) > 0) {
							THROW_SL(ret_buf.Write(&sbii_grec, sizeof(sbii_grec)));
							//THROW(MakeParam(&sbii_grec, &p_param_buf, out_buf_size = sizeof(sbii_grec)));
							r = 1;
						}
					}
				}
				break;
			case SBhtIICmdBuf::cmFindLocCell:
				{
					char name[128];
					SBIILocCellRec sbii_lrec;
					strnzcpy(name, (const char *)in_buf.constptr(), MIN(sizeof(name), in_buf.GetAvailableSize()));
					if(FindLocCell(0, name, &sbii_lrec) > 0) {
						THROW_SL(ret_buf.Write(&sbii_lrec, sizeof(sbii_lrec)));
						r = 1;
					}
				}
				break;
			case SBhtIICmdBuf::cmFindArticle:
				{
					r = -1;
				}
				break;
			case SBhtIICmdBuf::cmCellListByGoods:
				{
					long   goods_id = 0;
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					goods_id = *static_cast<const long *>(in_buf.constptr());
					reply_sended = ((r = GetGoodsList(rSo, goods_id, 0)) > 0) ? 1 : 0;
				}
				break;
			case SBhtIICmdBuf::cmGoodsListByCell:
				{
					long   cell_id = 0;
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					cell_id = *static_cast<const long *>(in_buf.constptr());
					reply_sended = ((r = GetGoodsList(rSo, cell_id, 1)) > 0) ? 1 : 0;
				}
				break;
			case SBhtIICmdBuf::cmSetConfig:
				{
					PPObjBHT bht_obj;
					THROW_PP(actual_in_buf_size == sizeof(Cfg), PPERR_SBII_PROT_INVARGSIZE);
					Cfg = *static_cast<const StyloBhtIIConfig *>(in_buf.constptr());
					Log_(0, PPTXT_SBIIIMPORTOK, "Config", 1, 0);
					if(Cfg.DeviceID)
						THROW(bht_obj.GetPacket(Cfg.DeviceID, &BhtPack) > 0);
					THROW_PP(BhtPack.P_SBIICfg && BhtPack.P_SBIICfg->IsValid(), PPERR_SBII_UNDEFDEVICE);
					(DeviceDir = BhtPack.ImpExpPath_).SetLastSlash().Transf(CTRANSF_INNER_TO_OUTER);
					r = 1;
				}
				break;
			case SBhtIICmdBuf::cmSetBills:
				{
					long   count = 0;
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					count = *static_cast<const long *>(in_buf.constptr());
					{
						SBIIBillRec sbii_rec;
						r = SetTable(rSo, SBhtIICmdBuf::cmSetBills, PPFILNAM_BHT_BILL, "Bills", &sbii_rec, count);
						reply_sended = BIN(r > 0);
					}
				}
				break;
			case SBhtIICmdBuf::cmSetBillRows:
				{
					long   count = 0;
					THROW_PP(actual_in_buf_size == sizeof(long), PPERR_SBII_PROT_INVARGSIZE);
					count = *static_cast<const long *>(in_buf.constptr());
					{
						SBIIBillRowRec sbii_rec;
						r = SetTable(rSo, SBhtIICmdBuf::cmSetBillRows, PPFILNAM_BHT_BLINE, "BillRows", &sbii_rec, count);
						reply_sended = BIN(r > 0);
					}
				}
				break;
			case SBhtIICmdBuf::cmAcceptLocOp:
				{
					SBIILocOp loc_op;
					THROW_PP(actual_in_buf_size == loc_op.GetSize(), PPERR_SBII_PROT_INVARGSIZE);
					loc_op.FromBuf(in_buf.constptr());
					r = AcceptLocOp(&loc_op);
				}
				break;
			case SBhtIICmdBuf::cmPrintBarcode:
				{
					char code[128];
					strnzcpy(code, static_cast<const char *>(in_buf.constptr()), MIN(sizeof(code), in_buf.GetAvailableSize()));
					r = PrintBarcode(code);
				}
				break;
			case SBhtIICmdBuf::cmLogout:
				reply_sended = 1;
				ok = -1;
				break;
			default:
				if(cmd_buf.Cmd > SBhtIICmdBuf::cmNextTableChunkBias) {
					long   next_rec_no = 0L;
					THROW_PP(actual_in_buf_size == sizeof(next_rec_no), PPERR_SBII_PROT_INVARGSIZE);
					next_rec_no = *static_cast<const long *>(in_buf.constptr());
					switch(cmd_buf.Cmd - SBhtIICmdBuf::cmNextTableChunkBias) {
						case SBhtIICmdBuf::cmGetGoods:
							{
								SBIIGoodsRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetGoods, PPFILNAM_BHT_GOODS, "Goods", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						case SBhtIICmdBuf::cmGetArticles:
							{
								SBIIArticleRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetArticles, PPFILNAM_BHT_SUPPL, "Articles", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						case SBhtIICmdBuf::cmGetBills:
							{
								SBIISampleBillRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetBills, PPFILNAM_BHT_SAMPLEBILLS, "SampleBills", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						case SBhtIICmdBuf::cmGetBillRows:
							{
								SBIISampleBillRowRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetBillRows, PPFILNAM_BHT_SAMPLEBROWS, "SampleBillRows", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						case SBhtIICmdBuf::cmGetBillRowsWithCells:
							{
								SBIIBillRowWithCellsRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetBillRowsWithCells, PPFILNAM_BHT_BROWSWCELLS, "BillRowsWithCells", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						case SBhtIICmdBuf::cmGetOpRestrictions:
							{
								SBIIOpRestrRec sbii_rec;
								r = GetTable(rSo, SBhtIICmdBuf::cmGetOpRestrictions, PPFILNAM_BHT_OPLIST, "OpRestrictions", &sbii_rec, next_rec_no);
								reply_sended = BIN(r > 0);
							}
							break;
						default:
							CALLEXCEPT_PP_S(PPERR_SBII_INVCMD, cmd_buf.Cmd);
					}
				}
				else {
					CALLEXCEPT_PP_S(PPERR_SBII_INVCMD, cmd_buf.Cmd);
				}
                break;
		}
		if(!r) {
			PPError();
			Log_(PPErrCode, 0, DS.GetTLA().AddedMsgString);
		}
		if(!reply_sended) {
			size_t actual_sended_size = 0;
			cmd_buf.RetCode = r;
			cmd_buf.BufSize = ret_buf.GetAvailableSize();
			THROW_SL(rSo.Send(&cmd_buf, sizeof(cmd_buf), &actual_sended_size));
			if(ret_buf.GetAvailableSize()) {
				THROW_SL(rSo.SendBuf(ret_buf, &actual_sended_size));
			}
		}
	}
    CATCH
		PPError();
		Log_(PPErrCode, 0, DS.GetTLA().AddedMsgString);
		{
			size_t snd_size = 0;
			cmd_buf.RetCode = 0;
			cmd_buf.BufSize = 0;
			rSo.Send(&cmd_buf, sizeof(cmd_buf), &snd_size);
			//SendCmd(rSo, &cmd_buf, p_param_buf);
		}
		ok = 0;
	ENDCATCH
	Log_(0, PPTXT_SBIIENDEXCHANGE, 0);
	return ok;
}

#if 0 // {
int StyloBhtIIExchange(const char * pDbSymb, const char * pName, const char * pPassword, TcpSocket * pSo)
{
	int    ok = 1;
	SString buf, msg_buf, pwd;
	Reference::Decrypt(Reference::crymRef2, pPassword, sstrlen(pPassword), pwd);
	if(DS.Login(pDbSymb, pName, pwd) > 0) {
		StyloBhtIIExchanger exch(pSo);
		pwd = 0;
		buf.Z().Cat(1L);
		THROW_SL(pSo->Send(buf.cptr(), buf.Len(), 0));
		THROW(exch.Run());
	}
	else {
		pwd = 0;
		buf.Z().Cat(0L);
		pSo->Send(buf.cptr(), buf.Len(), 0);
		THROW(0);
	}
	CATCH
		ok = (PPError(), 0);
	ENDCATCH
	DS.Logout();
	return ok;
}
#endif // } 0
