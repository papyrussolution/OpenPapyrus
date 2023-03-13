// PPSUPPLIX.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
// Export suppl data
//
SupplExpFilt::SupplExpFilt()
{
	Init();
}

void SupplExpFilt::Init()
{
	LocList.Set(0);
	EncodeStr.Z();
	memzero(this, offsetof(SupplExpFilt, EncodeStr));
}

int SupplExpFilt::Write(SBuffer & rBuf, long) const
{
	int    ok = 1;
	THROW_SL(rBuf.Write(SupplID));
	THROW_SL(rBuf.Write(GoodsGrpID));
	THROW_SL(rBuf.Write(ExpendOp));
	THROW_SL(rBuf.Write(Period.low));
	THROW_SL(rBuf.Write(Period.upp));
	THROW_SL(rBuf.Write(&MaxFileSizeKB, sizeof(MaxFileSizeKB)));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(TechID));
	THROW_SL(rBuf.Write(ClientCode, sizeof(ClientCode)));
	THROW_SL(rBuf.Write(IP));
	THROW_SL(rBuf.Write(Port));
	THROW_SL(rBuf.Write(AddScheme, sizeof(AddScheme)));
	THROW_SL(rBuf.Write(&AddRecType, sizeof(AddRecType)));
	THROW_SL(rBuf.Write(EncodeStr));
	THROW_SL(rBuf.Write(PctDis1));
	THROW_SL(rBuf.Write(static_cast<long>(LocList.GetCount())));
	{
		PPIDArray loc_list;
		LocList.CopyTo(&loc_list);
		SForEachVectorItem(loc_list, i) { THROW_SL(rBuf.Write(loc_list.at(i))); }
	}
	THROW_SL(rBuf.Write(RcptOp));
	THROW_SL(rBuf.Write(SupplRetOp));
	THROW_SL(rBuf.Write(RetOp));
	THROW_SL(rBuf.Write(MovInOp));
	THROW_SL(rBuf.Write(MovOutOp));
	THROW_SL(rBuf.Write(PctDis2));
	CATCHZOK
	return ok;
}

int SupplExpFilt::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	long   count = 0;
	LocList.Z();
	THROW(rBuf.GetAvailableSize());
	THROW_SL(rBuf.Read(SupplID));
	THROW_SL(rBuf.Read(GoodsGrpID));
	THROW_SL(rBuf.Read(ExpendOp));
	THROW_SL(rBuf.Read(Period.low));
	THROW_SL(rBuf.Read(Period.upp));
	THROW_SL(rBuf.Read(&MaxFileSizeKB, sizeof(MaxFileSizeKB)));
	THROW_SL(rBuf.Read(Flags));
	THROW_SL(rBuf.Read(TechID));
	THROW_SL(rBuf.Read(ClientCode, sizeof(ClientCode)));
	THROW_SL(rBuf.Read(IP));
	THROW_SL(rBuf.Read(Port));
	THROW_SL(rBuf.Read(AddScheme, sizeof(AddScheme)));
	THROW_SL(rBuf.Read(&AddRecType, sizeof(AddRecType)));
	THROW_SL(rBuf.Read(EncodeStr));
	THROW_SL(rBuf.Read(PctDis1));
	THROW_SL(rBuf.Read(count));
	for(long i = 0; i < count; i++) {
		long id = 0;
		THROW_SL(rBuf.Read(id));
		LocList.Add(id);
	}
	THROW_SL(rBuf.Read(RcptOp));
	THROW_SL(rBuf.Read(SupplRetOp));
	THROW_SL(rBuf.Read(RetOp));
	THROW_SL(rBuf.Read(MovInOp));
	THROW_SL(rBuf.Read(MovOutOp));
	THROW_SL(rBuf.Read(PctDis2));
	CATCHZOK
	return ok;
}

int SupplExpFilt::OpListFromCfg(const /*PPSupplExchangeCfg*/PPSupplAgreement::ExchangeParam * pCfg)
{
	int    ok = 1;
	THROW_INVARG(pCfg);
	ExpendOp   = pCfg->ExpendOp;
	RcptOp     = pCfg->RcptOp;
	SupplRetOp = pCfg->SupplRetOp;
	RetOp      = pCfg->RetOp;
	MovInOp    = pCfg->MovInOp;
	MovOutOp   = pCfg->MovOutOp;
	ProtVer    = static_cast<uint16>(pCfg->ProtVer);
	CATCHZOK
	return ok;
}

int SupplExpFilt::OpListToCfg(/*PPSupplExchangeCfg*/PPSupplAgreement::ExchangeParam * pCfg)
{
	int    ok = 1;
	THROW_INVARG(pCfg);
	pCfg->ExpendOp   = ExpendOp;
	pCfg->RcptOp     = RcptOp;
	pCfg->SupplRetOp = SupplRetOp;
	pCfg->RetOp      = RetOp;
	pCfg->MovInOp    = MovInOp;
	pCfg->MovOutOp   = MovOutOp;
	CATCHZOK
	return ok;
}

class PPSupplExchange_Baltika : public PrcssrSupplInterchange::ExecuteBlock {
public:
	enum BillExpParam {
		expEtc     = 0,
		expWeakAlc,
		expWoTareBeer
	};
	explicit PPSupplExchange_Baltika(PrcssrSupplInterchange::ExecuteBlock & rEb) : PrcssrSupplInterchange::ExecuteBlock(rEb), KegUnitID(-1)
	{
	}
	int    Init(/*const SupplExpFilt * pFilt*/);
	int    Import(const char * pPath);
	int    Export(PPLogger & rLogger);
private:
	int    Send();
	int    ExportPrice();
	int    ExportRest();
	int    ExportRestParties();
	int    ExportSpoilageRest(PPID locID, uint filesIdx);
	int    ExportBills(const BillExpParam &, const char * pClientCode, PPLogger & rLogger);
	int    ExportSaldo2(const PPIDArray & rExclArList, const char * pClientCode, PPLogger * pLog);
	// @v10.3.1 (inlined) void   DelFiles(const char * pFileName);
	PPID   GetSaleChannelTagID();
	PPID   GetConsigLocGroupID();
	long   GetSaleChannelExtFld();
	int    GetWeakAlcInfo(PPID * pLocID, PPID * pGGrpID, int getWoTareBeer);
	int    GetSpoilageLocList(PPIDArray * pList);
	int    GetBarcode(PPID goodsID, char * pBuf, size_t bufSize, int * pPackCode, int * pIsHoreca, double * pMult);
	int    GetQtty(PPID goodsID, int calcByPhPerU, double * pQtty, double * pPrice);
	int    GetDlvrAddrHorecaCode(PPID * pDlvrAddrID, SString & rCode);
	int    GetConsigLocInfo(const BillViewItem * pItem, PPID consigLocGrpID, LDATE * pParentDt, SString & rParentCode);
	int    GetInfoFromMemo(const char * pMemo, LDATE * pParentDt, SString & rParentCode, int simple = 0);
	void   GetInfoByLot(PPID lotID, const PPTransferItem * pTi, LDATE * pBillDt, LDATE * pCreateDt, LDATE * pExpiry, SString * pSerial);
	int    IsKegUnit(PPID goodsId);
	const char * GetEaText() const;
	int    GetSerial(PPID lotID, PPID goodsID, SString & rSerial);

	const  long HorecaDlvrAddrOffs = 100000000L;
	PPID   KegUnitID;
	PPID   DlvrAddrExtFldID; // Идентификатор дополнительного поля адреса доставки, хранящего код адреса у получателя отчета
	SStrCollection  Files;
	PPObjGoodsClass GCObj;
};

int PPSupplExchange_Baltika::Init(/*const SupplExpFilt * pFilt*/)
{
	int    ok = 1;
	SString temp_buf;
	DlvrAddrExtFldID = 0;
	Files.freeAll();
	{
		PPIniFile ini_file;
		if(ini_file.IsValid()) {
			ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKADLVRADDREXTFLDID, temp_buf);
			if(temp_buf.NotEmptyS())
				DlvrAddrExtFldID = temp_buf.ToLong();
		}
	}
	return ok;
}

int PPSupplExchange_Baltika::GetSerial(PPID lotID, PPID goodsID, SString & rSerial)
{
	// @construction
	int    ok = -1;
    rSerial.Z();
	ObjTagItem tag;
    if(PPRef->Ot.GetTag(PPOBJ_LOT, lotID, PPTAG_LOT_MANUFTIME, &tag) > 0) {
		SString temp_buf;
		int32  arcode_pack;
		GObj.P_Tbl->GetArCode(P.SupplID, goodsID, temp_buf, &arcode_pack);

		LDATETIME create_dtm;
		tag.GetTimestamp(&create_dtm);
		rSerial.Cat(create_dtm.d, DATF_DMY|DATF_NODIV).Cat(temp_buf);
		ok = 1;
    }
	return ok;
}

class SoapExporter {
public:
	explicit SoapExporter(bool flatStruc = false) : HeaderRecCount(0), LinesRecCount(0), FilesCount(0), MaxTransmitSize(0), AddedRecType(0), FlatStruc(flatStruc)
	{
	}
	void   SetMaxTransmitSize(uint32 maxTransmitSize)
	{
		MaxTransmitSize = maxTransmitSize;
	}
	//
	// Note: Функция должна вызываться (при необходимости) до Init()
	//
	void   SetAddedScheme(uint addedRecType, const char * pScheme)
	{
		AddedRecType = addedRecType;
		AddedScheme = pScheme;
	}
	void   SetClientCode(const char * pClientCode)
	{
		ClientCode = pClientCode;
	}
	int    Init(const char * pFile, uint headRecType, uint lineRecType, const char * pHeadScheme, const char * pLineScheme,
		/*const SupplExpFilt * pFilt,*/ const char * pSchemeName /*=0*/)
	{
		int    ok = 1;
		PPImpExpParam p;
		/*
		if(!RVALUEPTR(Filt, pFilt))
			Filt.Init();
		*/
		FileName = pFile; // Инициализация FileName до вызова InitExportParam
		if(headRecType) {
			HeadRecType = headRecType;
			InitExportParam(p, headRecType);
			HeadRec = p.InrRec;
		}
		if(lineRecType) {
			LineRecType = lineRecType;
			InitExportParam(p, lineRecType);
			LineRec = p.InrRec;
		}
		HeadScheme = pHeadScheme;
		LineScheme = pLineScheme;
		SchemeName = pSchemeName;
		F.Close();
		THROW_SL(F.Open(pFile, SFile::mWrite) > 0);
		THROW(WriteHeader());
		CATCHZOK
		HeaderRecCount = LinesRecCount = 0;
		return ok;
	}
	int Init(const char * pFile, uint headRecType, uint lineRecType, uint addLineRecType, uint promoLineRecType,
		const char * pHeadScheme, const char * pLineScheme, const char * pAddLineScheme, const char * pPromoLineScheme,
		const char * pSchemeName)
	{
		int    ok = 1;
		PPImpExpParam p;
		FileName = pFile; // Инициализация FileName до вызова InitExportParam
		if(headRecType) {
			HeadRecType = headRecType;
			InitExportParam(p, headRecType);
			HeadRec = p.InrRec;
		}
		if(lineRecType) {
			LineRecType = lineRecType;
			InitExportParam(p, lineRecType);
			LineRec = p.InrRec;
		}
		if(addLineRecType) {
			AddLineRecType = addLineRecType;
			InitExportParam(p, addLineRecType);
			AddLineRec = p.InrRec;
		}
		// @v10.4.0 {
		if(promoLineRecType) {
			PromoLineRecType = promoLineRecType;
			InitExportParam(p, promoLineRecType);
			PromoLineRec = p.InrRec;
		}
		// } @v10.4.0 
		HeadScheme = pHeadScheme;
		LineScheme = pLineScheme;
		AddLineScheme = pAddLineScheme;
		PromoLineScheme = pPromoLineScheme; // @v10.4.0
		SchemeName = pSchemeName;
		F.Close();
		THROW_SL(F.Open(pFile, SFile::mWrite) > 0);
		THROW(WriteHeader());
		CATCHZOK
		HeaderRecCount = LinesRecCount = 0;
		return ok;
	}
	int    AppendRecT(uint recTypeID, void * pRec, size_t recSize, int isFirstRec, int schemeNum, const char * pSchemeName /*=0*/)
	{
		int    ok = -1;
		if(recTypeID) {
			const char * p_scheme = 0;
			if(isFirstRec) {
				if(!isempty(pSchemeName))
					p_scheme = pSchemeName;
				else if(schemeNum == 0)
					p_scheme = AddedScheme.cptr();
				else
					p_scheme = LineScheme.cptr();
			}
			if(pRec && recSize) {
				PPImpExpParam p;
				InitExportParam(p, recTypeID);
				p.InrRec.SetDataBuf(pRec, recSize);
				THROW(WriteRec(&p.InrRec, p_scheme, 1));
			}
			else {
				THROW(WriteRec(0, p_scheme, 1));
			}
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	int    AppendRecP(void * pHeadRec, size_t headRecSize, void * pLineRec, size_t lineRecSize, int headRecForNewFile /*=0*/)
	{
		int    ok = -1;
		if((pHeadRec && headRecSize) || (pLineRec && lineRecSize)) {
			const int hr_ex = BIN(pHeadRec && headRecSize);
			const int lr_ex = BIN(pLineRec && lineRecSize);
			if(!headRecForNewFile && hr_ex) {
				if(LinesRecCount)
					F.WriteLine("</d>");
				if(HeaderRecCount)
					F.WriteLine("</r>");
				HeadRec.SetDataBuf(pHeadRec, headRecSize);
				THROW(WriteRec(&HeadRec, 0, 0));
				HeaderRecCount++;
				LinesRecCount = 0;
			}
			if(lr_ex) {
				LineRec.SetDataBuf(pLineRec, lineRecSize);
				THROW(WriteRec(&LineRec, (LinesRecCount == 0) ? LineScheme.cptr() : 0, 1));
				LinesRecCount++;
			}
			ok = 1;
			{
				int64  file_size = 0;
				F.CalcSize(&file_size);
				if(MaxTransmitSize > 0 && (static_cast<size_t>(file_size) / 1024) > MaxTransmitSize) {
					SString file_name;
					SPathStruc sp;
					EndDocument(ok);
					FilesCount++;
					file_name = FileName;
					sp.Split(FileName);
					sp.Nam.Cat(FilesCount);
					sp.Merge(FileName);
					THROW(Init(FileName, HeadRecType, LineRecType, HeadScheme, LineScheme, /*&Filt,*/ SchemeName));
					if(headRecForNewFile)
						AppendRecP(pHeadRec, headRecSize, 0, 0, 0);
					FileName = file_name;
				}
			}
		}
		CATCHZOK
		return ok;
	}
	void   EndDocument(int ok)
	{
		if(ok > 0) {
			WriteFooter();
			F.Close();
		}
		else {
			SString fname = F.GetName();
			F.Close();
			SFile::Remove(fname);
		}
	}
	void   EndRecBlock()
	{
		if(F.IsValid()) {
			F.WriteLine("</d>");
			LinesRecCount = 0;
		}
	}
private:
	int    InitExportParam(PPImpExpParam & rParam, uint recTyp)
	{
		int    ok = 1;
		rParam.Init();
		THROW(LoadSdRecord(recTyp, &rParam.InrRec));
		rParam.Direction  = 0; // export
		rParam.DataFormat = PPImpExpParam::dfSoap;
		rParam.TdfParam.Flags |= TextDbFile::fCpOem/*|TextDbFile::fVerticalRec*/;
		rParam.TdfParam.FldDiv = ";";
		rParam.FileName = FileName;
		rParam.OtrRec = rParam.InrRec;
		CATCHZOK
		return ok;
	}
	int    WriteHeader()
	{
		int    ok = -1;
		if(F.IsValid() && HeadScheme.Len()) {
			SString buf;
			F.WriteLine("<?xml version=\"1.0\" encoding=\"windows-1251\"?>\n");
			buf.Printf("<extdata user=\"%s\">", ClientCode.cptr());
			F.WriteLine(buf);
			buf.Printf("<scheme name=\"%s\" request=\"set\">", (SchemeName.Len() ? SchemeName.cptr() : HeadScheme.cptr()));
			F.WriteLine(buf);
			F.WriteLine("<data>");
			F.WriteLine("<s>");
			THROW(WriteScheme(&HeadRec, HeadScheme));
			if(/*Filt.Flags & SupplExpFilt::expFlatStruc*/FlatStruc)
				F.WriteLine("</d>");
			if(LineScheme.Len()) {
				THROW(WriteScheme(&LineRec, LineScheme));
				F.WriteLine("</d>");
			}
			if(AddLineScheme.Len()) {
				THROW(WriteScheme(&AddLineRec, AddLineScheme));
				F.WriteLine("</d>");
			}
			// @v10.4.0 {
			if(PromoLineScheme.Len()) {
				THROW(WriteScheme(&PromoLineRec, PromoLineScheme));
				F.WriteLine("</d>");
			}
			// } @v10.4.0 
			if(AddedRecType && AddedScheme.NotEmpty()) {
				PPImpExpParam p;
				InitExportParam(p, /*Filt.AddRecType*/AddedRecType);
				THROW(WriteScheme(&p.InrRec, /*Filt.AddScheme*/AddedScheme));
				F.WriteLine("</d>");
			}
			//if(!(Filt.Flags & SupplExpFilt::expFlatStruc))
			if(!FlatStruc)
				F.WriteLine("</d>");
			F.WriteLine("</s>");
			F.WriteLine("<o>");
			buf.Printf("<d name=\"%s\">", HeadScheme.cptr());
			F.WriteLine(buf);
		}
		CATCHZOK
		return ok;
	}
	int    WriteFooter()
	{
		int    ok = -1;
		if(F.IsValid()) {
			if(LinesRecCount)
				F.WriteLine("</d>");
			if(!FlatStruc) {
				F.WriteLine("</r>");
				F.WriteLine("</d>");
			}
			F.WriteLine("</o>");
			F.WriteLine("</data>");
			F.WriteLine("</scheme>");
			F.WriteLine("</extdata>");
			ok = 1;
		}
		return ok;
	}
	int    WriteScheme(const SdRecord * pRec, const char * pSchemeName)
	{
		int    ok = -1;
		if(F.IsValid() && pSchemeName) {
			SString buf;
			SString field_name, field_type;
			SdbField fld;
			buf.Z().CatChar('<').Cat("d").Space().CatEqQ("name", pSchemeName).CatChar('>');
			F.WriteLine(buf);
			for(uint i = 0; i < pRec->GetCount(); i++) {
				int    base_type = 0;
				THROW(pRec->GetFieldByPos(i, &fld));
				field_name = fld.Name;
				if(!field_name.NotEmptyS())
					field_name.Cat(fld.ID);
				base_type = stbase(fld.T.Typ);
				switch(base_type) {
					case BTS_DATE: field_type = "Date"; break;
					case BTS_INT:  field_type = "Integer"; break;
					case BTS_REAL: field_type = "Currency"; break;
					case BTS_STRING: field_type = "String"; break;
					default: field_type.Z(); break;
				}
				buf.Z().CatChar('<').Cat("f").Space().CatEqQ("name", field_name).Space().CatEqQ("type", field_type).Cat("/>");
				F.WriteLine(buf);
			}
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	int    WriteRec(const SdRecord * pRec, const char * pSchemeName, int endRecord)
	{
		int    ok = -1;
		if(F.IsValid()) {
			SString buf, line;
			if(pSchemeName) {
				buf.Printf("<d name=\"%s\">", pSchemeName);
				F.WriteLine(buf);
			}
			if(pRec) {
				F.WriteLine("<r>");
				SFormatParam fp;
				fp.FDate = DATF_YMD|DATF_CENTURY;
				fp.FTime = TIMF_HMS;
				fp.FReal = MKSFMTD(0, 2, NMBF_NOTRAILZ);
				SdbField fld;
				for(uint i = 0; pRec->EnumFields(&i, &fld);) {
					fld.GetFieldDataFromBuf(line, pRec->GetDataC(), fp);
					if(stbase(fld.T.Typ) == BTS_DATE) {
						LTIME tm = ZEROTIME;
						if(line.Len()) {
							line.ReplaceChar('/', '-');
							line.Cat("T").Cat(tm, TIMF_HMS);
						}
					}
					F.WriteLine(buf.Z().CatTag("f", line));
				}
				if(endRecord)
					F.WriteLine("</r>");
			}
			ok = 1;
		}
		return ok;
	}

	uint32 MaxTransmitSize;
	uint   AddedRecType;
	SString AddedScheme;
	SString ClientCode;
	bool   FlatStruc;
	uint8  Reserve[3]; // @alignment
	int    HeaderRecCount;
	int    LinesRecCount;
	int    FilesCount;
	uint   HeadRecType;
	uint   LineRecType;
	uint   AddLineRecType;
	uint   PromoLineRecType; // @v10.4.0
	SString HeadScheme;
	SString LineScheme;
	SString AddLineScheme;
	SString PromoLineScheme; // @v10.4.0
	SString SchemeName;
	SString FileName;
	SdRecord HeadRec;
	SdRecord LineRec;
	SdRecord AddLineRec;
	SdRecord PromoLineRec; // @v10.4.0
	SFile  F;
};

int PPSupplExchange_Baltika::Export(PPLogger & rLogger)
{
	int    ok = 1;
	SString temp_buf;
	PPIniFile ini_file(0, 0, 0, 1);
	SString client_code /*= Filt.ClientCode*/;
	Ep.GetExtStrData(Ep.extssClientCode, client_code);
	{
		SString wc_path;
		SPathStruc sp;
		static const char * p_fn_to_remove[] = { "spprice", "sprest", "spbills", "spbills1", "spbills2", "spdlvadr" };
		for(uint fntridx = 0; fntridx < SIZEOFARRAY(p_fn_to_remove); fntridx++) {
			SDirEntry sde;
			PPGetFilePath(PPPATH_OUT, p_fn_to_remove[fntridx], wc_path);
			sp.Split(wc_path);
			sp.Nam.CatChar('*');
			sp.Ext = "xml";
			sp.Merge(temp_buf);
			sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, wc_path);
			for(SDirec sd(temp_buf); sd.Next(&sde) > 0;) {
				sde.GetNameA(wc_path, temp_buf);
				SFile::Remove(temp_buf.cptr());
			}
		}
	}
	if(P.Actions & P.opExportPrices) {
		THROW(ExportPrice());
	}
	if(P.Actions & P.opExportStocks) {
		THROW(ExportRest());
		THROW(ExportRestParties());
	}
	if(P.Actions & P.opExportBills) {
		THROW(ExportBills(expEtc, client_code, rLogger));
		if(ini_file.IsValid()) {
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWEAKALCCODE, temp_buf) > 0) {
				THROW(ExportBills(expWeakAlc, temp_buf, rLogger));
			}
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWOTAREBEERCODE, temp_buf) > 0) {
				THROW(ExportBills(expWoTareBeer, temp_buf, rLogger));
			}
		}
	}
	if(P.Actions & P.opExportDebts) {
		PPIDArray excl_ar_list;
		SString temp_client_code = client_code;
		if(ini_file.IsValid() && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKASALDOCODE, temp_buf.Z()) > 0) {
			temp_client_code = temp_buf;
		}
		if(ini_file.IsValid() && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAEXCLSALDOCONTRAGENTS, temp_buf.Z()) > 0) {
			StringSet ss(',', temp_buf);
			for(uint p = 0; ss.get(&p, temp_buf);)
				excl_ar_list.add(temp_buf.ToLong());
			excl_ar_list.sort();
		}
		THROW(ExportSaldo2(excl_ar_list, temp_client_code, &rLogger)); // @v9.1.5 ExportSaldo-->ExportSaldo2
		//STRNSCPY(Filt.ClientCode, client_code);
	}
	THROW(Send());
	{
		PPSupplAgreement suppl_agr;
		THROW(ArObj.GetSupplAgreement(/*Filt.SupplID*/P.SupplID, &suppl_agr, 0) > 0);
		suppl_agr.Ep.LastDt = NZOR(/*Filt.Period.upp*/P.ExpPeriod.upp, getcurdate_());
		THROW(ArObj.PutSupplAgreement(/*Filt.SupplID*/P.SupplID, &suppl_agr, 1) > 0);
	}
	CATCHZOK
	{
		/*
		PPGetFile
		log.Save();
		*/
	}
	return ok;
}

const char * PPSupplExchange_Baltika::GetEaText() const { return (Ep.ProtVer == 0) ? "ea" : "кг"; }

int PPSupplExchange_Baltika::ExportPrice()
{
	int    ok = 1;
	uint   count = 0;
	SString path;
	SString client_code;
	SoapExporter soap_e;
	{
		Goods2Tbl::Rec grec;
		GoodsIterator  giter(/*se_filt*/Ep.GoodsGrpID, 0);
		// @v10.6.4 MEMSZERO(grec);
		PPGetFilePath(PPPATH_OUT, "spprice.xml", path);
		Ep.GetExtStrData(Ep.extssClientCode, client_code);
		soap_e.SetClientCode(client_code);
		THROW(soap_e.Init(path, PPREC_SUPPLPRICE, 0, "CRMWarePrice", 0/*, &se_filt*/, 0));
		for(;giter.Next(&grec) > 0;) {
			int    pack_code = 0;
			double bc_pack = 1.0;
			Sdr_SupplPrice rec;
			// @v10.7.9 @ctr MEMSZERO(rec);
			if(GetBarcode(grec.ID, rec.WareId, sizeof(rec.WareId), &pack_code, 0, &bc_pack) > 0 && pack_code == 0) {
				THROW(BillObj->trfr->Rcpt.GetCurrentGoodsPrice(grec.ID, LConfig.Location, 0, &rec.Price));
				STRNSCPY(rec.PriceTypeId, "001");
				STRNSCPY(rec.UnitId, GetEaText());
				if(bc_pack > 1.0) {
					rec.Price = R5(rec.Price * bc_pack);
				}
				THROW(soap_e.AppendRecP(&rec, sizeof(rec), 0, 0, 0/*headRecForNewFile*/));
				count++;
			}
			PPWaitPercent(giter.GetIterCounter());
		}
	}
	if(count > 0)
		Files.insert(newStr(path));
	CATCHZOK
	soap_e.EndDocument(ok > 0 && count);
	return ok;
}

IMPL_CMPFUNC(Sdr_Baltika_RestPartLine, i1, i2)
{
	const Sdr_Baltika_RestPartLine * p_i1 = static_cast<const Sdr_Baltika_RestPartLine*>(i1);
	const Sdr_Baltika_RestPartLine * p_i2 = static_cast<const Sdr_Baltika_RestPartLine*>(i2);
	int r = stricmp866(p_i1->WareId, p_i2->WareId);
	if(!r)
		r = stricmp866(p_i1->PartNumber, p_i2->PartNumber);
	return r;
}

void PPSupplExchange_Baltika::GetInfoByLot(PPID lotID, const PPTransferItem * pTi, LDATE * pBillDt, LDATE * pCreateDt, LDATE * pExpiry, SString * pSerial)
{
	//
	// @v8.6.10 Извлечение срока годности скорректировано так, чтобы приоритет был у даты, установленной у порожденного лота против оригинального
	//
	PPID   org_lot_id = 0;
	LDATE  crt_dt = ZERODATE;
	LDATE  expiry = pTi ? pTi->Expiry : ZERODATE;
	SString serial;
	ReceiptTbl::Rec lot;
	ReceiptTbl::Rec org_lot;
	// @v10.6.4 MEMSZERO(lot);
	if(P_BObj->trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, &lot, &org_lot) > 0) {
		ObjTagItem tag;
		if(PPRef->Ot.GetTag(PPOBJ_LOT, org_lot.ID, PPTAG_LOT_MANUFTIME, &tag) > 0) {
			LDATETIME create_dtm;
			tag.GetTimestamp(&create_dtm);
			crt_dt = create_dtm.d;
		}
		SETIFZ(expiry, org_lot.Expiry);
		GetSerial(org_lot.ID, org_lot.GoodsID, serial);
	}
	else {
		GetSerial(lot.ID, lot.GoodsID, serial);
	}
	// ASSIGN_PTR(pBillDt, lot.Dt);
	ASSIGN_PTR(pCreateDt, crt_dt);
	ASSIGN_PTR(pExpiry, expiry);
	ASSIGN_PTR(pSerial, serial);
}

int PPSupplExchange_Baltika::ExportRestParties()
{
	int    ok = 1;
	int    skip_delete = 0;
	uint   count = 0;
	uint   files_count = 0;
	uint   i;
	SString path, file_name, temp_buf;
	SString client_code;
	PPID   weakalc_locid = 0, weakalc_ggrpid = 0, wotarebeer_locid = 0, wotarebeer_ggrpid = 0, tare_ggrpid = 0;
	PPIDArray loc_list, spoilage_loc_list;
	TSVector <Sdr_Baltika_RestPartLine> wotarebeerrest_list; // @v9.8.4 TSArray-->TSVector
	GoodsRestFilt filt;
	SoapExporter soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest gr_view;
	PPIniFile ini_file(0, 0, 0, 1);
	PPGetFilePath(PPPATH_OUT, "sprestp.xml", path);
	// @v10.8.2 @ctr MEMSZERO(item);
	filt.GoodsGrpID = Ep.GoodsGrpID;
	filt.Date       = P.ExpPeriod.upp ? P.ExpPeriod.upp : LConfig.OperDate;
	//
	//@v10.1.7 filt.DiffParam = GoodsRestParam::_diffSerial;
	//filt.DiffParam = GoodsRestParam::_diffLotTag; // @v9.7.11
	//filt.DiffLotTagID = PPTAG_LOT_MANUFTIME; // @v9.7.11
	filt.DiffParam = GoodsRestParam::_diffLotTag; // @v10.1.7
	filt.DiffLotTagID = PPTAG_LOT_MANUFTIME; // @v10.1.7
	//
	if(P.LocList.IsEmpty()) {
		THROW(LocObj.GetWarehouseList(&loc_list, 0));
	}
	else
		loc_list = P.LocList.Get();
	GetSpoilageLocList(&spoilage_loc_list);
	for(i = 0; i < spoilage_loc_list.getCount(); i++)
		loc_list.freeByKey(spoilage_loc_list.at(i), 0);
	THROW(GetWeakAlcInfo(&weakalc_locid, &weakalc_ggrpid, 0));
	loc_list.addnz(weakalc_locid);
	THROW(GetWeakAlcInfo(&wotarebeer_locid, &wotarebeer_ggrpid, 1));
	if(wotarebeer_locid) {
		loc_list.freeByKey(wotarebeer_locid, 0);
		loc_list.add(wotarebeer_locid);
	}
	THROW(GetWeakAlcInfo(0, &tare_ggrpid, 2));
	for(i = 0; i < loc_list.getCount(); i++) {
		count = 0;
		filt.LocList.Z().Add(loc_list.at(i));
		THROW(gr_view.Init_(&filt));
		file_name = path;
		if(i > 0) {
			files_count++;
			SPathStruc sp(file_name);
			sp.Nam.Cat(files_count);
			sp.Merge(file_name);
		}
		if(loc_list.at(i) == wotarebeer_locid && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWOTAREBEERCODE, temp_buf.Z()) > 0) {
			client_code = temp_buf;
		}
		else {
			Ep.GetExtStrData(Ep.extssClientCode, client_code);
		}
		{
			Sdr_Baltika_RestPart head_rec;
			soap_e.SetClientCode(client_code);
			THROW(soap_e.Init(file_name, PPREC_BALTIKA_RESTPART, PPREC_BALTIKA_RESTPARTLINE, "CRMWhBalanceParam", "CRMWhBalanceParts", /*&se_filt,*/ "CRMWhBalanceExParts"));
			head_rec.SkipDelete = (P.Flags & P.fDeleteRecentBills) ? 0 : 1;
			THROW(soap_e.AppendRecP(&head_rec, sizeof(head_rec), 0, 0, 0/*headRecForNewFile*/));
		}
		if(loc_list.at(i) != wotarebeer_locid) {
			TSVector <Sdr_Baltika_RestPartLine> items_list; // @v9.8.4 TSArray-->TSVector
			for(gr_view.InitIteration(); gr_view.NextIteration(&item) > 0;) {
				double bc_pack = 1.0;
				Sdr_Baltika_RestPartLine line_rec;
				// @v10.7.9 @ctr MEMSZERO(line_rec);
				if(sstrlen(item.Serial) && GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
					line_rec.Quantity = item.Rest;
					STRNSCPY(line_rec.UnitId, GetEaText());
					{
						//LDATETIME serial_dtm;
						//strtodatetime(item.Serial, &serial_dtm, DATF_DMY, TIMF_HMS);
						// @v10.1.7 STRNSCPY(line_rec.PartNumber, item.Serial);
					}
					ltoa(loc_list.at(i), line_rec.WareHouseId, 10);
					GetInfoByLot(item.LotID, 0, &(line_rec.DocumentDate = filt.Date), &line_rec.ProductionDate, &line_rec.ExpirationDate, &temp_buf);
					STRNSCPY(line_rec.PartNumber, temp_buf); // @v10.1.7
					if(bc_pack > 1.0) {
						line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
					}
					if(wotarebeer_ggrpid && GObj.BelongToGroup(item.GoodsID, wotarebeer_ggrpid) > 0) {
						GetQtty(item.GoodsID, IsKegUnit(item.GoodsID), &line_rec.Quantity, 0);
						ltoa(wotarebeer_locid, line_rec.WareHouseId, 10);
						wotarebeerrest_list.insert(&line_rec);
					}
					else if(!tare_ggrpid || GObj.BelongToGroup(item.GoodsID, tare_ggrpid) <= 0) {
						uint idx = 0;
						if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)))
							items_list.at(idx).Quantity += line_rec.Quantity;
						else
							items_list.insert(&line_rec);
						count++;
					}
				}
				PPWaitPercent(gr_view.GetCounter());
			}
			if(items_list.getCount()) {
				for(uint j = 0; j < items_list.getCount(); j++) {
					Sdr_Baltika_RestPartLine line_rec = items_list.at(j);
					THROW(soap_e.AppendRecP(0, 0, &line_rec, sizeof(line_rec), 0/*headRecForNewFile*/));
					skip_delete = 1;
				}
			}
		}
		else { // Экспортируем остатки разливного пива
			uint items_count = wotarebeerrest_list.getCount();
			TSVector <Sdr_Baltika_RestPartLine> items_list;
			for(uint j = 0; j < items_count; j++) {
				Sdr_Baltika_RestPartLine temp_item = wotarebeerrest_list.at(j);
				Sdr_Baltika_RestPartLine line_rec = temp_item;
				uint idx = 0;
				if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)))
					items_list.at(idx).Quantity += line_rec.Quantity;
				else
					items_list.insert(&line_rec);
				count++;
				PPWaitPercent(j+1, items_count);
			}
			if(tare_ggrpid && loc_list.at(i) == wotarebeer_locid) {
				for(gr_view.InitIteration(); gr_view.NextIteration(&item) > 0;) {
					if(item.Serial[0] && GObj.BelongToGroup(item.GoodsID, tare_ggrpid) > 0) {
						double bc_pack = 1.0;
						Sdr_Baltika_RestPartLine line_rec;
						// @v10.7.9 @ctr MEMSZERO(line_rec);
						if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
							uint   idx = 0;
							line_rec.Quantity = item.Rest;
							STRNSCPY(line_rec.UnitId, GetEaText());
							// @v10.1.7 STRNSCPY(line_rec.PartNumber, item.Serial);
							ltoa(loc_list.at(i), line_rec.WareHouseId, 10);
							GetInfoByLot(item.LotID, 0, &(line_rec.DocumentDate = filt.Date), &line_rec.ProductionDate, &line_rec.ExpirationDate, &temp_buf);
							STRNSCPY(line_rec.PartNumber, temp_buf); // @v10.1.7
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
							}
							if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)))
								items_list.at(idx).Quantity += line_rec.Quantity;
							else
								items_list.insert(&line_rec);
							count++;
						}
					}
					PPWaitPercent(gr_view.GetCounter());
				}
			}
			if(items_list.getCount()) {
				for(uint j = 0; j < items_list.getCount(); j++) {
					Sdr_Baltika_RestPartLine line_rec = items_list.at(j);
					THROW(soap_e.AppendRecP(0, 0, &line_rec, sizeof(line_rec), 0/*headRecForNewFile*/));
					skip_delete = 1;
				}
			}
			//STRNSCPY(se_filt.ClientCode, Filt.ClientCode);
		}
		soap_e.EndDocument(ok > 0 && count);
	}
	if(count)
		Files.insert(newStr(path));
	CATCHZOK
	soap_e.EndDocument(ok > 0 && count);
	return ok;
}

int PPSupplExchange_Baltika::ExportRest()
{
	int    ok = 1;
	uint   count = 0, files_count = 0, i;
	SString path;
	SString file_name;
	SString temp_buf;
	SString client_code;
	PPID   weakalc_locid = 0;
	PPID   weakalc_ggrpid = 0;
	PPID   wotarebeer_locid = 0;
	PPID   wotarebeer_ggrpid = 0;
	PPID   tare_ggrpid = 0;
	PPIDArray   loc_list, spoilage_loc_list;
	RAssocArray weakalcrest_list, wotarebeerrest_list;
	GoodsRestFilt filt;
	SoapExporter  soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest   v;
	PPIniFile ini_file(0, 0, 0, 1);
	PPGetFilePath(PPPATH_OUT, "sprest.xml", path);
	// @v10.8.2 @ctr MEMSZERO(item);
	filt.GoodsGrpID = /*se_filt.*/Ep.GoodsGrpID;
	filt.Date       = NZOR(/*se_filt.Period.*/P.ExpPeriod.upp, LConfig.OperDate);
	if(P.LocList.IsEmpty()) {
		THROW(LocObj.GetWarehouseList(&loc_list, 0));
	}
	else
		loc_list = P.LocList.Get();
	GetSpoilageLocList(&spoilage_loc_list);
	for(i = 0; i < spoilage_loc_list.getCount(); i++)
		loc_list.freeByKey(spoilage_loc_list.at(i), 0);
	THROW(GetWeakAlcInfo(&weakalc_locid, &weakalc_ggrpid, 0));
	if(weakalc_locid) {
		loc_list.freeByKey(weakalc_locid, 0);
		loc_list.add(weakalc_locid);
	}
	THROW(GetWeakAlcInfo(&wotarebeer_locid, &wotarebeer_ggrpid, 1));
	if(wotarebeer_locid) {
		loc_list.freeByKey(wotarebeer_locid, 0);
		loc_list.add(wotarebeer_locid);
	}
	THROW(GetWeakAlcInfo(0, &tare_ggrpid, 2));
	for(i = 0; i < loc_list.getCount(); i++) {
		count = 0;
		filt.LocList.Z().Add(loc_list.at(i));
		THROW(v.Init_(&filt));
		file_name = path;
		if(i > 0) {
			files_count++;
			SPathStruc sp(file_name);
			sp.Nam.Cat(files_count);
			sp.Merge(file_name);
		}
		if(loc_list.at(i) == weakalc_locid && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWEAKALCCODE, temp_buf.Z()) > 0)
			client_code = temp_buf;
		else if(loc_list.at(i) == wotarebeer_locid && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWOTAREBEERCODE, temp_buf.Z()) > 0)
			client_code = temp_buf;
		else
			Ep.GetExtStrData(Ep.extssClientCode, client_code);
		Sdr_SupplRest head_rec;
		// @v10.7.9 @ctr MEMSZERO(head_rec);
		soap_e.SetClientCode(client_code);
		THROW(soap_e.Init(file_name, PPREC_SUPPLREST, PPREC_SUPPLRESTLINE, "CRMWhBalance", "CRMWhBalanceLine", /*&se_filt,*/ "CRMWhBalanceEx"));
		soap_e.SetMaxTransmitSize(P.MaxTransmitSize);
		ltoa(loc_list.at(i), head_rec.WareHouseId, 10);
		head_rec.DocumentDate = filt.Date;
		THROW(soap_e.AppendRecP(&head_rec, sizeof(head_rec), 0, 0, 0/*headRecForNewFile*/));
		if(!oneof2(loc_list.at(i), weakalc_locid, wotarebeer_locid)) {
			TSVector <Sdr_SupplRestLine> items_list; // @v9.8.4 TSArray-->TSVector
			for(v.InitIteration(); v.NextIteration(&item) > 0;) {
				if(weakalc_ggrpid && GObj.BelongToGroup(item.GoodsID, weakalc_ggrpid) > 0)
					weakalcrest_list.Add(item.GoodsID, item.Rest, 1, 0);
				else if(wotarebeer_ggrpid && GObj.BelongToGroup(item.GoodsID, wotarebeer_ggrpid) > 0) {
					double ph_rest = item.Rest;
					GetQtty(item.GoodsID, IsKegUnit(item.GoodsID), &ph_rest, 0);
					wotarebeerrest_list.Add(item.GoodsID, ph_rest, 1, 0);
				}
				else if(!tare_ggrpid || GObj.BelongToGroup(item.GoodsID, tare_ggrpid) <= 0) {
					double bc_pack = 1.0;
					Sdr_SupplRestLine line_rec;
					// @v10.7.9 @ctr MEMSZERO(line_rec);
					if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
						uint   idx = 0;
						line_rec.Quantity = item.Rest;
						STRNSCPY(line_rec.UnitId, GetEaText());
						if(bc_pack > 1.0) {
							line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
						}
						if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)))
							items_list.at(idx).Quantity += line_rec.Quantity;
						else
							items_list.insert(&line_rec);
						count++;
					}
				}
				PPWaitPercent(v.GetCounter());
			}
			if(items_list.getCount()) {
				for(uint j = 0; j < items_list.getCount(); j++) {
					Sdr_SupplRestLine line_rec = items_list.at(j);
					THROW(soap_e.AppendRecP(0, 0, &line_rec, sizeof(line_rec), 0/*headRecForNewFile*/));
				}
			}
		}
		else { // Экспортируем остатки слабоалкогольной продукции и разливного пива
			RAssocArray * p_rest_list = (loc_list.at(i) == weakalc_locid) ? &weakalcrest_list : &wotarebeerrest_list;
			uint items_count = p_rest_list->getCount();
			TSVector <Sdr_SupplRestLine> items_list; // @v9.8.4 TSArray-->TSVector
			for(uint j = 0; j < items_count; j++) {
				PPID   goods_id = p_rest_list->at(j).Key;
				double rest     = p_rest_list->at(j).Val;
				double bc_pack  = 1.0;
				Sdr_SupplRestLine line_rec;
				// @v10.7.9 @ctr MEMSZERO(line_rec);
				if(GetBarcode(goods_id, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
					uint idx = 0;
					line_rec.Quantity = rest;
					STRNSCPY(line_rec.UnitId, GetEaText());
					if(bc_pack > 1.0) {
						line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
					}
					if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)))
						items_list.at(idx).Quantity += line_rec.Quantity;
					else
						items_list.insert(&line_rec);
					count++;
				}
				PPWaitPercent(j+1, items_count);
			}
			if(tare_ggrpid && loc_list.at(i) == wotarebeer_locid) {
				for(v.InitIteration(); v.NextIteration(&item) > 0;) {
					if(GObj.BelongToGroup(item.GoodsID, tare_ggrpid) > 0) {
						double bc_pack = 1.0;
						Sdr_SupplRestLine line_rec;
						// @v10.7.9 @ctr MEMSZERO(line_rec);
						if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
							uint idx = 0;
							line_rec.Quantity = item.Rest;
							STRNSCPY(line_rec.UnitId, GetEaText());
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
							}
							if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)))
								items_list.at(idx).Quantity += line_rec.Quantity;
							else
								items_list.insert(&line_rec);
							count++;
						}
					}
					PPWaitPercent(v.GetCounter());
				}
			}
			if(items_list.getCount()) {
				for(uint j = 0; j < items_list.getCount(); j++) {
					Sdr_SupplRestLine line_rec = items_list.at(j);
					THROW(soap_e.AppendRecP(0, 0, &line_rec, sizeof(line_rec), 0/*headRecForNewFile*/));
				}
			}
			//STRNSCPY(se_filt.ClientCode, Filt.ClientCode);
		}
		soap_e.EndDocument(ok > 0 && count);
	}
	for(i = 0; i < spoilage_loc_list.getCount(); i++)
		THROW(ExportSpoilageRest(spoilage_loc_list.at(i), ++files_count));
	if(count)
		Files.insert(newStr(path));
	CATCHZOK
	soap_e.EndDocument(ok > 0 && count);
	return ok;
}

int PPSupplExchange_Baltika::ExportSpoilageRest(PPID locID, uint filesIdx)
{
	int    ok = 1;
	int    is_weak_alc = 0, is_wotare = 0;
	long   count = 0;
	PPID   wotarebeer_ggrpid = 0;
	Sdr_SupplRest     head_rec;
	GoodsRestViewItem rest_item;
	TSVector <Sdr_SupplRestLine> items_list; // @v9.8.4 TSArray-->TSVector
	//SupplExpFilt      se_filt;
	GoodsRestFilt     filt;
	SoapExporter      soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest   v;
	//se_filt = Filt;
	//se_filt.MaxFileSizeKB = 0;
	// @v10.8.2 @ctr MEMSZERO(item);
	filt.GoodsGrpID = /*se_filt.*/Ep.GoodsGrpID;
	filt.Date       = NZOR(/*se_filt.Period.*/P.ExpPeriod.upp, LConfig.OperDate);
	filt.Flags      = GoodsRestFilt::fNullRest;
	THROW(GetWeakAlcInfo(0, &wotarebeer_ggrpid, 1));
	THROW(v.Init_(&filt));
	for(v.InitIteration(); v.NextIteration(&rest_item) > 0;) {
		double bc_pack = 1.0;
		Sdr_SupplRestLine line_rec;
		// @v10.7.9 @ctr MEMSZERO(line_rec);
		if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
			uint idx = 0;
			if(wotarebeer_ggrpid && GObj.BelongToGroup(item.GoodsID, wotarebeer_ggrpid) > 0) {
				GetQtty(item.GoodsID, IsKegUnit(item.GoodsID), &item.Rest, 0);
				is_wotare = 1;
			}
			line_rec.Quantity = item.Rest;
			if(!is_wotare && bc_pack > 1.0) {
				line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
			}
			STRNSCPY(line_rec.UnitId, GetEaText());
			if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)))
				items_list.at(idx).Quantity += line_rec.Quantity;
			else
				items_list.insert(&line_rec);
			count++;
		}
	}
	if(count > 0) {
		SString file_name;
		SString client_code;
		PPIniFile ini_file(0, 0, 0, 1);
		PPGetFilePath(PPPATH_OUT, "sprest.xml", file_name);
		SPathStruc sp(file_name);
		sp.Nam.Cat(filesIdx);
		sp.Merge(file_name);

		Ep.GetExtStrData(Ep.extssClientCode, client_code);
		if(ini_file.IsValid()) {
			SString buf;
			if(is_weak_alc && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWEAKALCCODE, buf) > 0)
				client_code = buf;
			else if(is_wotare && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWOTAREBEERCODE, buf) > 0)
				client_code = buf;
		}
		soap_e.SetClientCode(client_code);
	 	THROW(soap_e.Init(file_name, PPREC_SUPPLREST, PPREC_SUPPLRESTLINE, "CRMWhBalance", "CRMWhBalanceLine", /*&se_filt,*/ "CRMWhBalanceEx"));
	 	ltoa(locID, head_rec.WareHouseId, 10);
	 	MEMSZERO(head_rec);
		head_rec.DocumentDate = filt.Date;
	 	THROW(soap_e.AppendRecP(&head_rec, sizeof(head_rec), 0, 0, 0/*headRecForNewFile*/));
		for(uint i = 0; i < items_list.getCount(); i++) {
			Sdr_SupplRestLine line_rec = items_list.at(i);
			THROW(soap_e.AppendRecP(0, 0, &line_rec, sizeof(line_rec), 0/*headRecForNewFile*/));
		}
		soap_e.EndDocument(ok > 0);
	}
	CATCHZOK
	return ok;
}

int PPSupplExchange_Baltika::GetBarcode(PPID goodsID, char * pBuf, size_t bufSize, int * pPackCode, int * pIsHoreca, double * pMult)
{
	int    ok = -1, pack_code = 0, is_horeca = 0;
	double mult = 1.0;
	int32  arcode_pack = 0;
	SString barcode, temp_buf;
	GObj.P_Tbl->GetArCode(P.SupplID, goodsID, barcode, &arcode_pack);
	if(barcode.IsEmpty()) {
		BarcodeArray barcode_list;
		GObj.ReadBarcodes(goodsID, barcode_list);
		for(uint i = 0; barcode.IsEmpty() && i < barcode_list.getCount(); i++) {
			temp_buf = barcode_list.at(i).Code;
			if(temp_buf.C(0) == '=' || temp_buf.C(0) == '*') {
				barcode = temp_buf;
				mult = barcode_list.at(i).Qtty;
			}
			/*
			GObj.GetSingleBarcode(goodsID, barcode); // @todo сделать перебор всех штрихкодов
			if(barcode.Len() <= 0 || (barcode.C(0) != '=' && barcode.C(0) != '*'))
				barcode = 0;
			*/
		}
	}
	else {
		if(arcode_pack > 0)
			mult = fdiv1000i(arcode_pack);
	}
	if(barcode.Len()) {
		SString left, right;
		barcode.Divide('|', left, right);
		barcode = left;
		pack_code = (right.Len() > 0) ? 1 : 0;
		is_horeca = BIN(right.Cmp("horeCa", 1) == 0);
		ok = 1;
	}
	barcode.CopyTo(pBuf, bufSize);
	ASSIGN_PTR(pPackCode, pack_code);
	ASSIGN_PTR(pIsHoreca, is_horeca);
	if(mult <= 0.0)
		mult = 1.0;
	ASSIGN_PTR(pMult, mult);
	return ok;
}

IMPL_CMPFUNC(Sdr_BaltikaBillItemAttrs, i1, i2)
{
	const Sdr_BaltikaBillItemAttrs * p_i1 = static_cast<const Sdr_BaltikaBillItemAttrs *>(i1);
	const Sdr_BaltikaBillItemAttrs * p_i2 = static_cast<const Sdr_BaltikaBillItemAttrs *>(i2);
	int r = stricmp866(p_i1->DocumentNumber, p_i2->DocumentNumber);
	if(!r)
		r = stricmp866(p_i1->WareId, p_i2->WareId);
	if(!r)
		r = stricmp866(p_i1->PartNumber, p_i2->PartNumber);
	return r;
}

int PPSupplExchange_Baltika::ExportBills(const BillExpParam & rExpParam, const char * pClientCode, PPLogger & rLogger)
{
	int    ok = 1;
	int    is_first_item = 1;
	int    is_first_promo = 1;
	int    from_consig_loc = 0;
	uint   count = 0;
	PPID   org_id = 0;
	const  PPID baltika_id = ObjectToPerson(P.SupplID);
	long   sale_channel_ext_fld = 0L;
	PPID   sale_channel_tag = 0;
	PPID   weakalc_locid = 0;
	PPID   weakalc_ggrpid = 0;
	PPID   wotarebeer_locid = 0;
	PPID   wotarebeer_ggrpid = 0;
	PPID   tare_ggrpid = 0;
	PPID   consig_loc_grp = 0L;
	PPID   order_number_tag_id = 0;
	PPID   promo_tag_id = 0; // @v10.3.11
	LDATE  consig_parent_dt = ZERODATE;
	SString path, log_msg;
	SString consig_parent_code;
	SString ord_num;
	SString bill_text;
	SString db_uuid_text;
	SString horeca_code;
	SString encode_str;
	SString add_link_op_str;
	SString temp_buf;
	Sdr_SupplBill head_rec;
	SoapExporter  soap_e(true /*flatStruc*/);
	LAssocArray dlvr_addr_list;
	BillFilt filt;
	PPIDArray loss_op_list, invrcpt_op_list, spoilage_loc_list;
	TSVector <Sdr_BaltikaBillItemAttrs> items_attrs_list;
	TSVector <Sdr_BaltikaBillPricePromo> promo_item_list; // @v10.3.11
	BillViewItem item;
	PPObjTag   obj_tag;
	PPObjWorld obj_world;
	PPObjQCert obj_qcert;
	PPViewBill v;
	{
		S_GUID uuid;
		CurDict->GetDbUUID(&uuid);
		uuid.ToStr(S_GUID::fmtIDL, db_uuid_text);
	}
	{
		PPObjectTag tag_rec;
		PPID   temp_id = 0;
		if(obj_tag.SearchBySymb("BALTIKA-ORDER-NO", &temp_id, &tag_rec) > 0) { // @v9.3.2
			if(tag_rec.ObjTypeID == PPOBJ_BILL && tag_rec.TagDataType == OTTYP_STRING)
				order_number_tag_id = tag_rec.ID;
		}
		if(obj_tag.SearchBySymb("BALTIKA-PROMO-LABEL", &(temp_id = 0), &tag_rec) > 0) { // @v10.3.11
			if(tag_rec.ObjTypeID == PPOBJ_BILL && oneof2(tag_rec.TagDataType, OTTYP_STRING, OTTYP_ENUM)) // @v10.4.0 OTTYP_ENUM
				promo_tag_id = tag_rec.ID;
		}
	}
	GetMainOrgID(&org_id);
	P.GetExtStrData(P.extssEncodeStr, encode_str);
	{
		const char * p_fn_ = 0;
		if(rExpParam == expWeakAlc)
			p_fn_ = "spbills2.xml";
		else if(rExpParam == expWoTareBeer)
			p_fn_ = "spbills1.xml";
		else
			p_fn_ = "spbills.xml";
		assert(p_fn_);
		PPGetFilePath(PPPATH_OUT, p_fn_, path);
	}
	filt.Period  = P.ExpPeriod;
	filt.Bbt     = bbtGoodsBills;
	filt.LocList = P.LocList;
	// @v10.7.9 @ctr MEMSZERO(item);
	THROW(v.Init_(&filt));
	if(Ep.ProtVer == 0) {
		soap_e.SetAddedScheme(PPREC_SUPPLDLVRADDR, "CRMClientAddress");
	}
	else {
		soap_e.SetAddedScheme(PPREC_SUPPLDLVRADDR, "CRMExtClientAddressDef");
	}
	soap_e.SetClientCode(pClientCode);
	THROW(soap_e.Init(path, PPREC_SUPPLBILL, PPREC_SUPPLBILLLINE, PPREC_BALTIKABILLITEMATTRS, PPREC_BALTIKABILLPRICEPROMO, 
		"CRMDespatchParam", "CRMDespatch", "CRMDespatchParts", "CRMDiscount", "CRMDespatchEx"));
	soap_e.SetMaxTransmitSize(P.MaxTransmitSize);
	//
	// @v10.8.2 @ctr MEMSZERO(head_rec);
	head_rec.WorkDate = P.ExpPeriod.low;
	head_rec.SkipDelete = (P.Flags & P.fDeleteRecentBills) ? 0 : 1;
	head_rec.IsSupplyConvertClients = 1;
	THROW(soap_e.AppendRecT(PPREC_SUPPLBILL, &head_rec, sizeof(head_rec), 0, 0, 0/*pSchemeName*/));
	soap_e.EndRecBlock();
	THROW(GetWeakAlcInfo(&weakalc_locid, &weakalc_ggrpid, 0));
	THROW(GetWeakAlcInfo(&wotarebeer_locid, &wotarebeer_ggrpid, 1));
	THROW(GetWeakAlcInfo(0, &tare_ggrpid, 2));
	GetSpoilageLocList(&spoilage_loc_list);
	sale_channel_tag = GetSaleChannelTagID();
	sale_channel_ext_fld = GetSaleChannelExtFld();
	consig_loc_grp = GetConsigLocGroupID();
	{
		PPInventoryOpEx invop_rec;
		PPObjOprKind op_obj;
		for(PPID op_id = 0; EnumOperations(PPOPT_INVENTORY, &op_id) > 0;) {
			if(op_obj.FetchInventoryData(op_id, &invop_rec) > 0) {
				loss_op_list.addUnique(invop_rec.WrDnOp);
				invrcpt_op_list.addUnique(invop_rec.WrUpOp);
			}
		}
	}
	for(v.InitIteration(PPViewBill::OrdByDefault); v.NextIteration(&item) > 0;) {
		ord_num.Z();
		int    send = 0;
		uint   pos = 0;
		LDATE  ord_dt = ZERODATE;
		PPID   client_id = 0;
		PPID   dlvr_addr_id = 0;
		PPID   spoilage_loc = 0;
		PPIDArray ord_list;
		SString doc_type_str;
		PersonTbl::Rec psn_rec;
		PPTransferItem trfr_item;
		PPBillPacket bpack;
		TSVector <Sdr_SupplBillLine> items_list; // @v9.8.4 TSArray-->TSVector
		THROW(P_BObj->ExtractPacket(item.ID, &bpack));
		PPObjBill::MakeCodeString(&bpack.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, bill_text);
		if(bpack.GetOrderList(ord_list) > 0) {
			BillTbl::Rec bill_rec;
			// @v10.6.4 MEMSZERO(bill_rec);
			if(P_BObj->Search(ord_list.at(0), &bill_rec) > 0) {
				ord_num = bill_rec.Code;
				ord_dt = bill_rec.Dt;
				if(spoilage_loc_list.bsearch(bill_rec.LocID, 0))
					spoilage_loc = bill_rec.LocID;
			}
			else if(IsOpBelongTo(item.OpID, Ep.ExpendOp)) { // @debug {
				(log_msg = bill_text).CatDiv(':', 2).Cat("can't found order id=[").Cat(ord_list.at(0)).Cat("]");
				rLogger.Log(log_msg);
				PPLogMessage(PPFILNAM_BALTIKA_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_USER);
			} // } debug
		}
		else if(IsOpBelongTo(item.OpID, Ep.ExpendOp)) { // @debug {
			rLogger.Log((log_msg = bill_text).CatDiv(':', 2).Cat("can't get order list"));
			PPLogMessage(PPFILNAM_BALTIKA_LOG, log_msg, LOGMSGF_TIME|LOGMSGF_USER);
		} // } debug
		else if(IsOpBelongTo(item.OpID, Ep.RetOp) && order_number_tag_id) {
            if(PPRef->Ot.GetTagStr(PPOBJ_BILL, item.ID, order_number_tag_id, temp_buf) > 0) {
                if(temp_buf.Divide(' ', ord_num, log_msg) > 0) {
                    ord_num.Strip();
                    if(ord_num.NotEmptyS()) {
						log_msg.Strip();
						LDATE   temp_dt = ZERODATE;
						if(strtodate(log_msg, DATF_DMY, &temp_dt) && checkdate(temp_dt))
                            ord_dt = temp_dt;
						else
							ord_num.Z();
                    }
                }
                else
					ord_num.Z();
            }
		}
		// @v10.6.4 MEMSZERO(psn_rec);
		if(!oneof2(item.OpID, Ep.MovOutOp, Ep.MovInOp)) {
			client_id = ObjectToPerson(bpack.Rec.Object, 0);
			if(bpack.GetDlvrAddrID())
				dlvr_addr_id = bpack.GetDlvrAddrID();
			else if(PsnObj.Search(client_id, &psn_rec) > 0)
				dlvr_addr_id = NZOR(psn_rec.RLoc, psn_rec.MainLoc);
			/* Сначала нужно выяснить, будем ли выгружать данную накладную. Поэтому блок перенесен
			if(client_id && dlvr_addr_id)
				if(dlvr_addr_list.lsearch(&dlvr_addr_id, 0, CMPF_LONG, sizeof(long)) <= 0)
					dlvr_addr_list.Add(client_id, dlvr_addr_id, 0);
			*/
		}
		//
		// Определяем тип передаваемого документа
		//
		{
			int    doc_type_idx = -1;
			if(spoilage_loc)
				doc_type_idx = BALTIKA_DOCTYPES_RETURN;
			else if(item.OpID == Ep.MovOutOp)
				doc_type_idx = BALTIKA_DOCTYPES_MOVINGFROM;
			else if(item.OpID == Ep.MovInOp)
				doc_type_idx = BALTIKA_DOCTYPES_MOVINGTO;
			else if(IsOpBelongTo(item.OpID, Ep.RetOp))
				doc_type_idx = BALTIKA_DOCTYPES_RETURN;
			else if(IsOpBelongTo(item.OpID, Ep.ExpendOp))
				doc_type_idx = BALTIKA_DOCTYPES_EXPEND;
			else if(IsOpBelongTo(item.OpID, Ep.RcptOp))
				doc_type_idx = BALTIKA_DOCTYPES_RECEIPT;
			else if(IsOpBelongTo(item.OpID, Ep.SupplRetOp))
				doc_type_idx = BALTIKA_DOCTYPES_SUPPLRETURN;
			else if(loss_op_list.lsearch(item.OpID))
				doc_type_idx = BALTIKA_DOCTYPES_LOSSES;
			else if(invrcpt_op_list.lsearch(item.OpID))
				doc_type_idx = BALTIKA_DOCTYPES_INVENTORY;
			// не будем учитывать межскладское перемещение, если товар - слабый алкоголь или разливуха
			if(oneof2(doc_type_idx, BALTIKA_DOCTYPES_MOVINGTO, BALTIKA_DOCTYPES_MOVINGFROM) && oneof2(rExpParam, expWeakAlc, expWoTareBeer))
				doc_type_idx = -1;
			//
			// Если это перемещение с консигнационного склада, то пока не проставили правильное примечание, такой документ не трогаем
			//
			if(doc_type_idx == BALTIKA_DOCTYPES_MOVINGFROM) {
				from_consig_loc = GetConsigLocInfo(&item, consig_loc_grp, &consig_parent_dt, consig_parent_code);
				//
				// Документ перемещения с консигнационного склада. Но у него в примечании не проставлен номер родительского документа и дата, поэтому пропустим его
				//
				if(from_consig_loc == -1)
					doc_type_idx = -1;
			}
			if(doc_type_idx >= 0) {
				PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, doc_type_idx, doc_type_str);
				//
				// Заполняем информацию о приходном драфт документе
				//
				if(doc_type_idx == BALTIKA_DOCTYPES_RECEIPT) {
					if(item.LinkBillID != 0) {
						BillTbl::Rec bill_rec;
						if(P_BObj->Search(item.LinkBillID, &bill_rec) > 0) {
							ord_num = bill_rec.Code;
							ord_dt = bill_rec.Dt;
						}
					}
					else {
						GetInfoFromMemo(item.SMemo, &ord_dt, ord_num, 1); // @v11.1.12 item.Memo-->item.SMemo
					}
				}
				else if(oneof2(doc_type_idx, BALTIKA_DOCTYPES_MOVINGTO, BALTIKA_DOCTYPES_MOVINGFROM)) {
					ord_num = bpack.Rec.Code;
					ord_dt = bpack.Rec.Dt;
				}
			}
		}
		if(doc_type_str.NotEmpty()) {
			const PPID obj_id = oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp) ? 0L : bpack.Rec.Object;
			const PPID loc_id = bpack.Rec.LocID;
			const PPID loc2_id = PPObjLocation::ObjToWarehouse(bpack.Rec.Object);
			SString promo_label;
			RetailPriceExtractor rtl_price_extr(bpack.Rec.LocID, 0, obj_id, ZERODATETIME, RTLPF_GETCURPRICE);
			RetailPriceExtractor::ExtQuotBlock eqb(Ep.PriceQuotID);
			RetailPriceExtractor price_by_quot_extr(bpack.Rec.LocID, &eqb, obj_id, ZERODATETIME, RTLPF_PRICEBYQUOT);
			if(promo_tag_id && bpack.BTagL.GetItemStr(promo_tag_id, temp_buf) > 0) { // temp_buf содержит код скидки (ShareId)
				promo_label = temp_buf;
			}
			for(bpack.InitExtTIter(0/*ETIEF_UNITEBYGOODS*/); bpack.EnumTItemsExt(0, &trfr_item) > 0;) {
				// если операция имеет тип - модификация товара, то будем рассматривать только приходные строчки
				int check_modif_op = (GetOpType(item.OpID) == PPOPT_GOODSMODIF) ? BIN(trfr_item.Flags & PPTFR_PLUS) : 1;
				if(check_modif_op && (GObj.BelongToGroup(trfr_item.GoodsID, Ep.GoodsGrpID) > 0 ||
					(weakalc_ggrpid && GObj.BelongToGroup(trfr_item.GoodsID, weakalc_ggrpid) > 0) ||
					wotarebeer_ggrpid && (GObj.BelongToGroup(trfr_item.GoodsID, wotarebeer_ggrpid) > 0 || GObj.BelongToGroup(trfr_item.GoodsID, tare_ggrpid) > 0))) {
					int    is_horeca = 0;
					double bc_pack = 1.0;
					Sdr_SupplBillLine line_rec;
					Sdr_BaltikaBillItemAttrs line_attrs_rec;
					// @v10.7.9 @ctr MEMSZERO(line_rec);
					// @v10.7.9 @ctr MEMSZERO(line_attrs_rec);
					if(GetBarcode(trfr_item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, &is_horeca, &bc_pack) > 0) {
						BillExpParam exp_param;
						(temp_buf = item.Code).Transf(CTRANSF_INNER_TO_OUTER);
						STRNSCPY(line_rec.DocumentNumber, temp_buf);
						ord_num.CopyTo(line_rec.CRMOrderNumber, sizeof(line_rec.CRMOrderNumber));
						line_rec.CRMOrderDate = ord_dt;
						ltoa(client_id, line_rec.CompanyId, 10);
						if(is_horeca) {
							PPID   horeca_addr_id = dlvr_addr_id;
							GetDlvrAddrHorecaCode(&horeca_addr_id, horeca_code);
							ltoa(horeca_addr_id, line_rec.AddressId, 10);
						}
						else
							ltoa(dlvr_addr_id, line_rec.AddressId, 10);
						line_rec.DocumentDate    = trfr_item.Date;
						line_rec.ActionDate      = trfr_item.Date; // @v9.4.12
						STRNSCPY(line_rec.SrcCRMDbId, pClientCode);
						if(IsOpBelongTo(item.OpID, Ep.ExpendOp)) {
							LDATE pay_dt = ZERODATE;
							if(GetOpType(item.OpID) == PPOPT_GOODSMODIF)
								pay_dt = trfr_item.Date;
							else
								bpack.GetLastPayDate(&pay_dt);
							line_rec.PayDate = pay_dt;
						}
						if(weakalc_ggrpid && GObj.BelongToGroup(trfr_item.GoodsID, weakalc_ggrpid) > 0) {
							ltoa(weakalc_locid, line_rec.WareHouseId, 10);
							exp_param = expWeakAlc;
						}
						else if(wotarebeer_ggrpid && GObj.BelongToGroup(trfr_item.GoodsID, wotarebeer_ggrpid) > 0) {
							ltoa(wotarebeer_locid, line_rec.WareHouseId, 10);
							exp_param = expWoTareBeer;
						}
						else {
							ltoa(trfr_item.LocID, line_rec.WareHouseId, 10);
							exp_param = expEtc;
						}
						if(exp_param == rExpParam) {
							uint   idx = 0;
							double qtty_val = fabs(trfr_item.Qtty());
							double _price = 0.0;
							if(oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp)) {
								PTR32(line_rec.WareHouseId)[0] = 0;
								ltoa(loc_id, line_rec.WareHouseId, 10);
							}
							{
								SString buf, sale_channel;
								ObjTagItem tag;
								WorldTbl::Rec wrec;
								LocationTbl::Rec loc_rec;
								// @v10.6.4 MEMSZERO(loc_rec);
								// @v10.6.4 MEMSZERO(wrec);
								if(dlvr_addr_id && LocObj.Search(dlvr_addr_id, &loc_rec) > 0) {
									if(loc_rec.CityID && obj_world.Search(loc_rec.CityID, &wrec) > 0)
										STRNSCPY(line_rec.AddressRegionType, wrec.Phone);
									if(sale_channel_ext_fld)
										LocationCore::GetExField(&loc_rec, sale_channel_ext_fld, sale_channel);
								}
								if(sale_channel.Len() == 0 && sale_channel_tag && PPRef->Ot.GetTag(PPOBJ_PERSON, client_id, sale_channel_tag, &tag) > 0)
									obj_tag.GetCurrTagVal(&tag, sale_channel);
								sale_channel.CopyTo(line_rec.SaleChannel, sizeof(line_rec.SaleChannel));
							}
							{
								int    use_quot = 0;
								if(spoilage_loc) {
									_price = trfr_item.Cost;
									if(!_price) {
										RetailExtrItem rtl_price_item;
										rtl_price_extr.GetPrice(trfr_item.GoodsID, 0, 0.0, &rtl_price_item);
										_price = rtl_price_item.Cost;
									}
									ltoa(spoilage_loc, line_rec.WareHouseId, 10);
								}
								else if(Ep.PriceQuotID) {
									double lot_price = 0.0;
									ReceiptTbl::Rec lot_rec;
									const QuotIdent qi(bpack.Rec.Dt, bpack.Rec.LocID, Ep.PriceQuotID, 0, obj_id);
									if(GetCurGoodsPrice(trfr_item.GoodsID, bpack.Rec.LocID, 0, &lot_price, &lot_rec) > 0)
										use_quot = BIN(GObj.GetQuotExt(trfr_item.GoodsID, qi, lot_rec.Cost, lot_price, &_price, 1) > 0);
								}
								if(!use_quot) {
									SETIFZ(_price, trfr_item.NetPrice());
									const float pct_dis = (atol(line_rec.AddressRegionType) != 1) ? P.SpcDisPct2 : P.SpcDisPct1;
									_price -= (_price / 100.0) * pct_dis;
								}
								_price  = R2(_price);
							}
							GetQtty(trfr_item.GoodsID, IsKegUnit(trfr_item.GoodsID), &qtty_val, &_price);
							line_rec.Quantity = qtty_val;
							line_rec.Price    = _price;
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
								line_rec.Price = R5(line_rec.Price * bc_pack);
							}
							doc_type_str.CopyTo(line_rec.DocumentTypeId, sizeof(line_rec.DocumentTypeId));
							idx = 0;
							if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplBillLine, WareId)))
								items_list.at(idx).Quantity += line_rec.Quantity;
							else
								items_list.insert(&line_rec);
							// @v10.3.11 {
							if(promo_label.NotEmpty()) {
								double nominal_price = trfr_item.Price;
								double temp_qtty_val = fabs(trfr_item.Qtty());
								GetQtty(trfr_item.GoodsID, IsKegUnit(trfr_item.GoodsID), &temp_qtty_val, &nominal_price);
								if(nominal_price > line_rec.Price) {
									Sdr_BaltikaBillPricePromo promo_rec;
									// @v10.7.9 @ctr MEMSZERO(promo_rec);
									STRNSCPY(promo_rec.CompanyId, line_rec.CompanyId);
									STRNSCPY(promo_rec.AddressId, line_rec.AddressId);
									STRNSCPY(promo_rec.WareHouseId, line_rec.WareHouseId);
									STRNSCPY(promo_rec.DocumentNumber, line_rec.DocumentNumber);
									STRNSCPY(promo_rec.DocumentTypeId, line_rec.DocumentTypeId);
									promo_rec.DocumentDate = line_rec.DocumentDate;
									STRNSCPY(promo_rec.WareId, line_rec.WareId);
						
									STRNSCPY(promo_rec.ShareId, promo_label); // ShareId zstring(24) "Код скидки: [Cooler, Contract, Sellin, PricePromo]
									temp_buf.Z().Cat(bpack.Rec.Dt.year()).CatChar('-').CatLongZ(bpack.Rec.Dt.month(), 2);
									STRNSCPY(promo_rec.ShareDate, temp_buf); // Год/Месяц Скидки  в формате ГГГГ_ММ  (2018-01)
									promo_rec.DiscountSum = (nominal_price - line_rec.Price) * temp_qtty_val;
									promo_rec.DiscountPercent = 100.0 * promo_rec.DiscountSum / (nominal_price * temp_qtty_val);
									promo_item_list.insert(&promo_rec);
								}
							}
							// } @v10.3.11 
							if(!oneof2(item.OpID, Ep.MovOutOp, Ep.MovInOp) && client_id && dlvr_addr_id)
								if(!dlvr_addr_list.lsearch(&dlvr_addr_id, 0, CMPF_LONG, sizeof(long)))
									dlvr_addr_list.Add(client_id, dlvr_addr_id, 0);
							//
							// Дополнительная информация по строке документа
							//
							{
								SString serial;
								line_attrs_rec.DocumentDate = line_rec.DocumentDate;
								STRNSCPY(line_attrs_rec.DocumentNumber, line_rec.DocumentNumber);
								STRNSCPY(line_attrs_rec.DocumentTypeId, line_rec.DocumentTypeId);
								STRNSCPY(line_attrs_rec.WareId, line_rec.WareId);
								line_attrs_rec.Quantity = line_rec.Quantity;
								GetInfoByLot(trfr_item.LotID, &trfr_item, 0, &line_attrs_rec.ProductionDate, &line_attrs_rec.ExpirationDate, &serial);
								serial.CopyTo(line_attrs_rec.PartNumber, sizeof(line_attrs_rec.PartNumber));
								idx = 0;
								if(items_attrs_list.lsearch(&line_attrs_rec, &idx, PTR_CMPFUNC(Sdr_BaltikaBillItemAttrs)))
									items_attrs_list.at(idx).Quantity += line_rec.Quantity;
								else
									items_attrs_list.insert(&line_attrs_rec);
								// @v10.1.9 {
								if(oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp)) {
									Sdr_BaltikaBillItemAttrs line_attrs_mirror_rec;
									// @v10.7.9 @ctr MEMSZERO(line_attrs_mirror_rec);
									memcpy(&line_attrs_mirror_rec, &line_attrs_rec, sizeof(line_attrs_rec));
									add_link_op_str.Z();
									if(item.OpID == Ep.MovInOp)
										PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGFROM, add_link_op_str);
									else {
										// Межскладские перемещения документов с консигнационного склада будем разбивать, на мескладской расход и приход товара от поставщика
										if(from_consig_loc)
											PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_RECEIPT, add_link_op_str);
										else
											PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGTO, add_link_op_str);
									}
									STRNSCPY(line_attrs_mirror_rec.DocumentTypeId, add_link_op_str);
									/*idx = 0;
									if(items_attrs_list.lsearch(&line_attrs_mirror_rec, &idx, PTR_CMPFUNC(Sdr_BaltikaBillItemAttrs)))
										items_attrs_list.at(idx).Quantity += line_rec.Quantity;
									else*/
										items_attrs_list.insert(&line_attrs_mirror_rec);
								}
								// } @v10.1.9
							}
							count++;
						}
					}
				}
			}
			if(items_list.getCount()) {
				int    add_link_op = 0;
				add_link_op_str.Z();
				if(oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp)) {
					add_link_op = 1;
					if(item.OpID == Ep.MovInOp)
						PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGFROM, add_link_op_str);
					else {
						// Межскладские перемещения документов с консигнационного склада будем разбивать, на мескладской расход и приход товара от поставщика
						if(from_consig_loc)
							PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_RECEIPT, add_link_op_str);
						else
							PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGTO, add_link_op_str);
					}
				}
				for(uint i = 0; i < items_list.getCount(); i++) {
					Sdr_SupplBillLine line_rec = items_list.at(i);
					THROW(soap_e.AppendRecT(PPREC_SUPPLBILLLINE, &line_rec, sizeof(line_rec), is_first_item, 1, 0/*pSchemeName*/));
					is_first_item = 0;
					log_msg.Z().Cat("code=[").Cat((temp_buf = line_rec.DocumentNumber).Transf(CTRANSF_OUTER_TO_INNER)).Cat("]").
						Space().Space().Space().Cat("order=[").Cat(line_rec.CRMOrderNumber).Cat("]");
					rLogger.Log(log_msg);
					if(add_link_op) {
						STRNSCPY(line_rec.DocumentTypeId, add_link_op_str);
						PTR32(line_rec.WareHouseId)[0] = 0;
						ltoa(loc2_id, line_rec.WareHouseId, 10);
						if(from_consig_loc) {
							consig_parent_code.CopyTo(line_rec.CRMOrderNumber, sizeof(line_rec.CRMOrderNumber));
							line_rec.CRMOrderDate = consig_parent_dt;
							ltoa(baltika_id, line_rec.CompanyId, 10);
						}
						THROW(soap_e.AppendRecT(PPREC_SUPPLBILLLINE, &line_rec, sizeof(line_rec), is_first_item, 1, 0/*pSchemeName*/));
					}
				}
			}
		}
		PPWaitPercent(v.GetCounter());
	}
	if(count) {
		SString addr;
		if(count && items_attrs_list.getCount()) {
			soap_e.EndRecBlock();
			int is_first = 1;
			for(uint i = 0; i < items_attrs_list.getCount(); i++) {
				Sdr_BaltikaBillItemAttrs line_attrs_rec = items_attrs_list.at(i);
				THROW(soap_e.AppendRecT(PPREC_BALTIKABILLITEMATTRS, &line_attrs_rec, sizeof(line_attrs_rec), is_first, -1, "CRMDespatchParts"));
				is_first = 0;
			}
		}
		soap_e.EndRecBlock();
		Files.insert(newStr(path));
		if(dlvr_addr_list.getCount()) {
			uint   i = 0, j = 0;
			SString cli_name, code;
			for(; i < dlvr_addr_list.getCount(); i++) {
				PPID   dlvr_addr_id = dlvr_addr_list.at(i).Val;
				LocationTbl::Rec loc_rec;
				// @v10.6.4 MEMSZERO(loc_rec);
				if(LocObj.Search(dlvr_addr_id, &loc_rec) > 0) {
					PPID   client_id = dlvr_addr_list.at(i).Key;
					Sdr_SupplDlvrAddr addr_rec;
					// @v10.7.9 @ctr MEMSZERO(addr_rec);
					GetObjectName(PPOBJ_PERSON, client_id, cli_name = 0);
					// specencodesym=%01,294520000;%02,294520003
					// %01478954         294520003780100
					code.Z().EncodeString(loc_rec.Code, /*Filt.EncodeStr*/encode_str, 1).CopyTo(addr_rec.CRMClientId, sizeof(addr_rec.CRMClientId));
					ltoa(client_id, addr_rec.CompanyId, 10);
					cli_name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(addr_rec.CompanyName, sizeof(addr_rec.CompanyName));
					ltoa(dlvr_addr_id, addr_rec.AddressId, 10);
					(temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
					STRNSCPY(addr_rec.AddressName, temp_buf);
					LocObj.P_Tbl->GetAddress(loc_rec, 0, addr);
					addr.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(addr_rec.Location, sizeof(addr_rec.Location));
					THROW(soap_e.AppendRecT(PPREC_SUPPLDLVRADDR, &addr_rec, sizeof(addr_rec), BIN(j == 0), 0, 0/*pSchemeName*/));
					j++;
					if(GetDlvrAddrHorecaCode(&dlvr_addr_id, code.Z()) > 0) {
						code.CopyTo(addr_rec.CRMClientId, sizeof(addr_rec.CRMClientId));
						ltoa(dlvr_addr_id, addr_rec.AddressId, 10);
						THROW(soap_e.AppendRecT(PPREC_SUPPLDLVRADDR, &addr_rec, sizeof(addr_rec), BIN(j == 0), 0, 0/*pSchemeName*/));
						j++;
					}
				}
			}
			if(j > 0)
				soap_e.EndRecBlock();
		}
		// @v10.3.11 {
		if(promo_item_list.getCount()) {
			for(uint promoidx = 0; promoidx < promo_item_list.getCount(); promoidx++) {
				Sdr_BaltikaBillPricePromo promo_item_rec = promo_item_list.at(promoidx);
				THROW(soap_e.AppendRecT(PPREC_BALTIKABILLPRICEPROMO, &promo_item_rec, sizeof(promo_item_rec), is_first_promo, 1, "CRMDiscount"/*pSchemeName*/));
				is_first_promo = 0;
			}
			soap_e.EndRecBlock();
		}
		// } @v10.3.11 
	}
	CATCHZOK
	soap_e.EndDocument(ok > 0 && count);
	//Filt.Flags &= ~SupplExpFilt::expFlatStruc;
	return ok;
}

static int _WriteScheme(SXml::WDoc & rXmlDoc, const SdRecord & rRd, const char * pSchemeName)
{
	int    ok = 1;
	SString field_name, field_type;
	SdbField fld;
	{
		SXml::WNode n_d(rXmlDoc, "d");
		n_d.PutAttrib("name", NZOR(pSchemeName, rRd.Name));
		for(uint i = 0; i < rRd.GetCount(); i++) {
			THROW(rRd.GetFieldByPos(i, &fld));
			{
				const int base_type = stbase(fld.T.Typ);
				field_name = fld.Name;
				if(!field_name.NotEmptyS())
					field_name.Cat(fld.ID);
				switch(base_type) {
					case BTS_DATE:	field_type = "Date"; break;
					case BTS_INT:   field_type = "Integer"; break;
					case BTS_REAL:  field_type = "Currency"; break;
					case BTS_STRING: field_type = "String"; break;
					default: field_type.Z(); break;
				}
				{
					SXml::WNode n_f(rXmlDoc, "f");
					n_f.PutAttrib("name", field_name);
					n_f.PutAttrib("type", field_type);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static int FASTCALL _WriteRec(SXml::WDoc & rXmlDoc, const SdRecord & rRd, const void * pDataBuf)
{
	assert(pDataBuf);
	int    ok = 1;
	SString value_buf;
	{
		SXml::WNode w_r(rXmlDoc, "r");
		SFormatParam fp;
		fp.FDate = DATF_ISO8601CENT;
		fp.FTime = TIMF_HMS;
		fp.FReal = MKSFMTD(0, 2, NMBF_NOTRAILZ|NMBF_NOZERO);
		SdbField fld;
		for(uint i = 0; rRd.EnumFields(&i, &fld);) {
			fld.GetFieldDataFromBuf(value_buf = 0, pDataBuf, fp);
			if(stbase(fld.T.Typ) == BTS_DATE) {
				LTIME tm = ZEROTIME;
				if(value_buf.Len())
					value_buf.Cat("T").Cat(tm, TIMF_HMS);
			}
			SXml::WNode w_f(rXmlDoc, "f", value_buf.Strip());
		}
	}
	return ok;
}

int PPSupplExchange_Baltika::ExportSaldo2(const PPIDArray & rExclArList, const char * pClientCode, PPLogger * pLog)
{
	const   LDATE _curdt = getcurdate_();
	const   PPID acs_id = GetSellAccSheet();
    int    ok = 1;
    StrAssocArray * p_cli_list = 0;
	PPIDArray goods_list;
	SString out_file_name;
	SString temp_buf;
    SdRecord rd_saldo_aggr;
    SdRecord rd_saldo_doc;
    SdRecord rd_saldo_ware;
	PPGetFilePath(PPPATH_OUT, "spsaldo.xml", out_file_name);
    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
    THROW(p_x);
    THROW(LoadSdRecord(PPREC_BALTIKASALDOAGGREGATE, &rd_saldo_aggr));
    THROW(LoadSdRecord(PPREC_BALTIKASALDODOC, &rd_saldo_doc));
    THROW(LoadSdRecord(PPREC_BALTIKASALDOWARE, &rd_saldo_ware));
    {
		SXml::WDoc _doc(p_x, cp1251);
		{
			SXml::WNode n_h(p_x, "extdata");
			n_h.PutAttrib("user", /*Filt.ClientCode*/pClientCode);
			{
				SXml::WNode n_s(p_x, "scheme");
				n_s.PutAttrib("name", "CRMSaldoEx");
				n_s.PutAttrib("request", "set");
				{
					SXml::WNode n_data(p_x, "data");
					//
					LocationTbl::Rec loc_rec;
					PersonTbl::Rec psn_rec;
					PPViewDebtTrnovr debt_view;
					DebtTrnovrViewItem debt_item;
					PPIDArray processed_debt_id_list;
					//
					{
						SXml::WNode n_s(p_x, "s");
						THROW(_WriteScheme(_doc, rd_saldo_aggr, "CRMSaldoAggregate"));
						THROW(_WriteScheme(_doc, rd_saldo_doc, "CRMSaldoDoc"));
						THROW(_WriteScheme(_doc, rd_saldo_ware, "CRMSaldoWare"));
					}
					{
						SXml::WNode n_s(p_x, "o");
						{
							SXml::WNode n_d(p_x, "d");
							n_d.PutAttrib("name", "CRMSaldoAggregate");
                            //
							PPSupplAgreement agt;
							PPClientAgreement cli_agt;
							DebtTrnovrFilt debt_filt;
							debt_filt.Flags |= DebtTrnovrFilt::fDebtOnly;
							P.LocList.Get(debt_filt.LocIDList);
 							THROW(ArObj.GetSupplAgreement(P.SupplID, &agt, 0));
							debt_filt.AccSheetID = acs_id;
							debt_filt.DebtDimList = agt.Ep.DebtDimList;
							debt_filt.Sgb.S = SubstGrpBill::sgbDlvrLoc;
							THROW(debt_view.Init_(&debt_filt));
							for(debt_view.InitIteration(PPViewDebtTrnovr::OrdByDefault); debt_view.NextIteration(&debt_item) > 0;) {
								const PPID loc_id = debt_item.ID_;
								PPID   person_id = 0;
								if(LocObj.Search(loc_id, &loc_rec) > 0 && PsnObj.Search(loc_rec.OwnerID, &psn_rec) > 0) {
									PPID   ar_id = 0;
									Sdr_BaltikaSaldoAggregate sdr_saldo_aggr;
									ArObj.P_Tbl->PersonToArticle(psn_rec.ID, acs_id, &ar_id);
									// @v10.8.2 @ctr MEMSZERO(sdr_saldo_aggr);
									sdr_saldo_aggr.SaldoDate = _curdt; //           date                "Дата, за которую экспортируются данные"; // @v9.1.4
									temp_buf.Z().Cat(psn_rec.ID);
									STRNSCPY(sdr_saldo_aggr.CompanyId, temp_buf);
									temp_buf.Z().Cat(loc_id);
									STRNSCPY(sdr_saldo_aggr.AddressId, temp_buf); // zstring(24)         "Код клиента";
									// @optional sdr_saldo_aggr.ProductId            zstring(24)         "Код продукта";
									// @optional sdr_saldo_aggr.ProductName          zstring(128)        "Название продукта";
									if(ArObj.GetClientAgreement(ar_id, cli_agt, 0) > 0) {
										sdr_saldo_aggr.mCreditLimit = cli_agt.MaxCredit;
									}
									sdr_saldo_aggr.mCustInvoice = debt_item.Debt; //   double format(10.2) "mCustInvoice";
									// @optional sdr_saldo_aggr.mCustInvoiceAllow = 0.0; //  double format(10.2) "mCustInvoiceAllow";
									sdr_saldo_aggr.mCustInvoiceOverdue = debt_item.ExpiryDebt;
									// @optional sdr_saldo_aggr.mCustReturn          double format(10.2) "Возврат поставщику";
									// @optional sdr_saldo_aggr.mDebtDocBank         double format(10.2) "mDebtDocBank";
									// @optional sdr_saldo_aggr.mDebtDocCash         double format(10.2) "mDebtDocCash";
									// @optional sdr_saldo_aggr.mDebtDocCopy         double format(10.2) "mDebtDocCopy";
									// @optional sdr_saldo_aggr.mReturnableDebt      double format(10.2) "mReturnableDebt";
									// @optional sdr_saldo_aggr.mVendInvoice         double format(10.2) "mVendInvoice";
									// @optional sdr_saldo_aggr.mVendReturn          double format(10.2) "Возврат от покупателя";
									// @optional sdr_saldo_aggr.mCredDoc             double format(10.2) "mCredDoc";
									// @optional sdr_saldo_aggr.mTotalBalance        double format(10.2) "Итого баланс";
									THROW(_WriteRec(_doc, rd_saldo_aggr, &sdr_saldo_aggr));
									processed_debt_id_list.add(loc_id);
								}
							}
						}
						{
							SXml::WNode n_d(p_x, "d");
							n_d.PutAttrib("name", "CRMSaldoDoc");
                            processed_debt_id_list.sortAndUndup();
							for(debt_view.InitIteration(PPViewDebtTrnovr::OrdByDefault); debt_view.NextIteration(&debt_item) > 0;) {
								const PPID loc_id = debt_item.ID_;
								PPID   person_id = 0;
								if(processed_debt_id_list.bsearch(loc_id) && LocObj.Search(loc_id, &loc_rec) > 0 && PsnObj.Search(loc_rec.OwnerID, &psn_rec) > 0) {
									PPID   ar_id = 0;
									ArObj.P_Tbl->PersonToArticle(psn_rec.ID, acs_id, &ar_id);
									PPIDArray bill_list;
									debt_view.GetBillSubstBlock().AsscList.GetListByKey(loc_id, bill_list);
									for(uint i = 0; i < bill_list.getCount(); i++) {
										const PPID bill_id = bill_list.get(i);
										BillTbl::Rec bill_rec;
										if(P_BObj->Search(bill_id, &bill_rec) > 0) {
											Sdr_BaltikaSaldoDoc sdr_saldo_doc;
											// @v10.8.2 @ctr MEMSZERO(sdr_saldo_doc);
											temp_buf.Z().Cat(psn_rec.ID);
											STRNSCPY(sdr_saldo_doc.CompanyId, temp_buf);
											temp_buf.Z().Cat(loc_id);
											STRNSCPY(sdr_saldo_doc.AddressId, temp_buf);
											// @v11.1.12 BillCore::GetCode(temp_buf = bill_rec.Code);
											// @v11.1.12 STRNSCPY(sdr_saldo_doc.DocumentNumber, temp_buf);
											STRNSCPY(sdr_saldo_doc.DocumentNumber, bill_rec.Code); // @v11.1.12 
											sdr_saldo_doc.DocumentDate = bill_rec.Dt;
											sdr_saldo_doc.ActionDate = bill_rec.Dt;
											{
												LDATE  last_pay_date = ZERODATE;
												P_BObj->P_Tbl->GetLastPayDate(bill_id, &last_pay_date);
												sdr_saldo_doc.PaymentDate = last_pay_date;
												sdr_saldo_doc.OverduePeriod = checkdate(last_pay_date) ? diffdate(_curdt, last_pay_date) : 0;
											}
											{
												double payment = 0.0;
												P_BObj->P_Tbl->CalcPayment(bill_id, 0, 0, 0, &payment);
												double debt = bill_rec.Amount - payment;
												SETMAX(debt, 0.0);
												sdr_saldo_doc.CustInvoiceSumm = debt;
											}
											THROW(_WriteRec(_doc, rd_saldo_doc, &sdr_saldo_doc));
										}
									}
								}
							}
						}
						{
							SXml::WNode n_d(p_x, "d");
							n_d.PutAttrib("name", "CRMSaldoWare");
                            //
							ArticleFilt ar_filt;
							ar_filt.AccSheetID = acs_id;
							ar_filt.Ft_Closed = -1;
							p_cli_list = ArObj.MakeStrAssocList(&ar_filt);
							GoodsIterator::GetListByGroup(GObj.GetConfig().TareGrpID, &goods_list);
							if(p_cli_list && goods_list.getCount()) {
								const uint cli_count = p_cli_list->getCount();
								const uint goods_count = goods_list.getCount();
								for(uint i = 0; i < cli_count; i++) {
									const PPID ar_id = p_cli_list->Get(i).Id;
									const PPID person_id = ObjectToPerson(ar_id);
									ArticleTbl::Rec ar_rec;
									if(person_id && !rExclArList.bsearch(ar_id) && ArObj.Search(ar_id, &ar_rec) > 0 && !ar_rec.Closed) {
										PPIDArray dlvr_loc_list;
										PsnObj.GetDlvrLocList(person_id, &dlvr_loc_list);
                                        if(dlvr_loc_list.getCount() == 0)
											dlvr_loc_list.add(0L);
										else
											dlvr_loc_list.sortAndUndup();
										for(uint j = 0; j < goods_count; j++) {
											const PPID goods_id = goods_list.at(j);
											double bc_pack = 1.0;
											Sdr_BaltikaSaldoWare sdr_saldo_ware;
											// @v10.8.2 @ctr MEMSZERO(sdr_saldo_ware);
											if(GetBarcode(goods_id, sdr_saldo_ware.WareId, sizeof(sdr_saldo_ware.WareId), 0, 0, &bc_pack) > 0) {
												//sdr_saldo_ware.Date = (Filt.Period.upp && Filt.Period.upp < _curdt) ? Filt.Period.upp : _curdt;
												sdr_saldo_ware.Date = (P.ExpPeriod.upp && P.ExpPeriod.upp < _curdt) ? P.ExpPeriod.upp : _curdt;
												for(uint locidx = 0; locidx < dlvr_loc_list.getCount(); locidx++) {
													const PPID dlvr_loc_id = dlvr_loc_list.get(locidx);
													if(P_BObj->GetGoodsSaldo(goods_id, ar_id, dlvr_loc_id, sdr_saldo_ware.Date, MAXLONG, &sdr_saldo_ware.Quantity, &sdr_saldo_ware.Summ) > 0 && sdr_saldo_ware.Quantity != 0.0) {
														//sdr_saldo_ware.Quantity = 1.0; // @debug
														//sdr_saldo_ware.Summ = 1.0;      // @debug
														if(sdr_saldo_ware.Quantity < 0.0) {
															sdr_saldo_ware.Quantity = fabs(sdr_saldo_ware.Quantity);
															sdr_saldo_ware.Summ = fabs(sdr_saldo_ware.Summ);
														}
														else {
															sdr_saldo_ware.Quantity = 0.0;
															sdr_saldo_ware.Summ = 0.0;
														}
														ltoa(person_id, sdr_saldo_ware.CompanyId, 10);
														//memzero(sdr_saldo_ware.AddressId, sizeof(sdr_saldo_ware.AddressId)); // не указываем, так как передаем тару
														temp_buf.Z();
														if(dlvr_loc_id)
															temp_buf.Cat(dlvr_loc_id);
														STRNSCPY(sdr_saldo_ware.AddressId, temp_buf); // zstring(24)         "Код клиента";
														memzero(sdr_saldo_ware.FACode,    sizeof(sdr_saldo_ware.FACode));    // (код оборудования) не указываем, так как передаем тару
														THROW(_WriteRec(_doc, rd_saldo_ware, &sdr_saldo_ware));
														//THROW(soap_e.AppendRecT(PPREC_BALTIKASALDOWARE, &rec_saldo, sizeof(rec_saldo), is_first_rec, 0, "CRMSaldoWare"));
													}
												}
											}
										}
									}
									ok = 1;
									PPWaitPercent(i, cli_count - 1);
								}
							}
						}
					}
				}
			}
		}
    }
    CATCHZOK
    xmlFreeTextWriter(p_x);
    delete p_cli_list;
    return ok;
}

int PPSupplExchange_Baltika::GetInfoFromMemo(const char * pMemo, LDATE * pParentDt, SString & rParentCode, int simple)
{
	int    ok = -1;
	LDATE  dt = ZERODATE;
	SString str_dt, buf, str_code;
	SString memo(pMemo);
	if(!simple) {
		memo.ShiftLeft(1).Divide(' ', str_code, buf);
		(memo = buf).Divide(' ', buf, str_dt);
	}
	else
		memo.Divide(' ', str_code, str_dt);
	if(str_dt.Len())
		strtodate(str_dt, DATF_DMY, &dt);
	if(dt && str_code.Len())
		ok = 1;
	else {
		str_code = 0;
		dt = ZERODATE;
	}
	ASSIGN_PTR(pParentDt, dt);
	rParentCode = str_code;
	return ok;
}

int PPSupplExchange_Baltika::GetConsigLocInfo(const BillViewItem * pItem, PPID consigLocGrpID, LDATE * pParentDt, SString & rParentCode)
{
	int    ok = 0;
	rParentCode.Z();
	ASSIGN_PTR(pParentDt, ZERODATE);
	if(pItem && consigLocGrpID) {
		PPObjLocation obj_loc;
		if(obj_loc.IsMemberOfGroup(pItem->LocID, consigLocGrpID) > 0)
			ok = GetInfoFromMemo(pItem->SMemo, pParentDt, rParentCode); // @v11.1.12 item.Memo-->item.SMemo
	}
	return ok;
}

PPID PPSupplExchange_Baltika::GetConsigLocGroupID()
{
	PPID loc_grp_id = 0L;
	PPObjLocation obj_loc;
	LocationFilt loc_filt(LOCTYP_WAREHOUSEGROUP);
	StrAssocArray * p_list = obj_loc.MakeList_(&loc_filt, 1);
	for(uint i = 0; !loc_grp_id && i < p_list->getCount(); i++) {
		LocationTbl::Rec loc_rec;
		if(obj_loc.Search(p_list->Get(i).Id, &loc_rec) > 0 && sstreqi_ascii(loc_rec.Code, "CONSIGNATION"))
			loc_grp_id = loc_rec.ID;
	}
	ZDELETE(p_list);
	return loc_grp_id;
}

PPID PPSupplExchange_Baltika::GetSaleChannelTagID()
{
	PPID   sale_channel_tag = 0;
	SString sale_channel_tag_symb;
	PPObjTag   obj_tag;
	sale_channel_tag_symb = "SaleChannel";
	SArray * p_tags_list = obj_tag.CreateList(0, 0);
	if(p_tags_list) {
   		for(uint i = 0; !sale_channel_tag && i < p_tags_list->getCount(); i++) {
   			const PPID tag_id = *static_cast<const PPID *>(p_tags_list->at(i));
			PPObjectTag tag_kind;
			if(obj_tag.Search(tag_id, &tag_kind) > 0 && sale_channel_tag_symb.CmpPrefix(tag_kind.Symb, 1) == 0)
				sale_channel_tag = tag_id;
		}
		ZDELETE(p_tags_list);
	}
	return sale_channel_tag;
}

long PPSupplExchange_Baltika::GetSaleChannelExtFld()
{
	long   ext_fld = 0L;
	PPIniFile ini_file;
	if(ini_file.IsValid()) {
		SString str_ext_fld;
		ini_file.Get((uint)PPINISECT_CONFIG, PPINIPARAM_BALTIKASALECHANNELEXTFLD,  str_ext_fld);
		ext_fld = str_ext_fld.ToLong();
	}
	return ext_fld;
}

int PPSupplExchange_Baltika::GetWeakAlcInfo(PPID * pLocID, PPID * pGGrpID, int getWoTareBeer)
{
	PPID   loc_id = 0, ggrp_id = 0;
	PPIniFile ini_file;
	if(ini_file.IsValid()) {
		uint inipar_locsymb  = (oneof2(getWoTareBeer, 1, 2)) ? PPINIPARAM_BALTIKAWOTAREBEERLOCSYMB : PPINIPARAM_BALTIKAWEAKALCLOCSYMB;
		uint inipar_ggrpcode = (getWoTareBeer == 1) ? PPINIPARAM_BALTIKAWOTAREBEERGGRPCODE : ((getWoTareBeer == 2) ? PPINIPARAM_BALTIKATAREGGRPCODE : PPINIPARAM_BALTIKAWEAKALCGGRPCODE);
		SString loc_symb, ggrp_code;
		BarcodeTbl::Rec bc_rec;
		PPObjGoodsGroup obj_ggrp;
		// @v10.6.4 MEMSZERO(bc_rec);
		ini_file.Get((uint)PPINISECT_CONFIG, inipar_locsymb,  loc_symb);
		LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, loc_symb, &loc_id);
		ini_file.Get((uint)PPINISECT_CONFIG, inipar_ggrpcode, ggrp_code);
		if(obj_ggrp.SearchCode(ggrp_code, &bc_rec) > 0)
			ggrp_id = bc_rec.GoodsID;
	}
	ASSIGN_PTR(pLocID,  loc_id);
	ASSIGN_PTR(pGGrpID, ggrp_id);
	return 1;
}

// @vmiller
int PPSupplExchange_Baltika::IsKegUnit(PPID goodsID)
{
	int    yes = 0;
	if(KegUnitID < 0) {
		PPIniFile ini_file;
		int    unit_id = 0;
		if(ini_file.GetInt((uint)PPINISECT_CONFIG, PPINIPARAM_BALTIKAUNITKEGID, &unit_id) > 0) {
			PPUnit u_rec;
			if(UObj.Fetch(unit_id, &u_rec) <= 0)
				unit_id = 0;
		}
		KegUnitID = unit_id;
	}
	if(KegUnitID > 0) {
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(goodsID, &goods_rec) > 0 && goods_rec.UnitID == KegUnitID)
			yes = 1;
	}
	return yes;
}

int PPSupplExchange_Baltika::GetSpoilageLocList(PPIDArray * pList)
{
	int    ok = -1;
	PPIniFile ini_file;
	if(ini_file.IsValid()) {
		SString loc_symb_list;
		BarcodeTbl::Rec bc_rec;
		// @v10.6.4 MEMSZERO(bc_rec);
		ini_file.Get((uint)PPINISECT_CONFIG, PPINIPARAM_BALTIKASPOILAGELOCSYMB, loc_symb_list);
		{
			SString loc_symb;
			StringSet ss(",");
			ss.setBuf(loc_symb_list, loc_symb_list.Len() + 1);
			for(uint p = 0; ss.get(&p, loc_symb);) {
				PPID loc_id = 0;
				LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, loc_symb, &loc_id);
				if(pList && loc_id)
					pList->addUnique(loc_id);
			}
			CALLPTRMEMB(pList, sort());
			ok = 1;
		}
	}
	return ok;
}

int PPSupplExchange_Baltika::GetQtty(PPID goodsID, int calcByPhPerU, double * pQtty, double * pPrice)
{
	double qtty = DEREFPTRORZ(pQtty);
	double price = DEREFPTRORZ(pPrice);
	if(calcByPhPerU) {
		double phuperu = 0.0;
		PPGoodsPacket g_pack;
		if(GObj.GetPacket(goodsID, &g_pack, PPObjGoods::gpoSkipQuot) > 0 && g_pack.Rec.GdsClsID && g_pack.ExtRec.W) {
			phuperu = g_pack.ExtRec.W;
			/*
			PPGdsClsPacket  gc_pack;
			GCObj.GetPacket(g_pack.Rec.GdsClsID, &gc_pack);
			phuperu = gc_pack.DimW.ValList.getSingle();
			*/
		}
		else
 			GObj.GetPhUPerU(goodsID, 0, &phuperu);
 		qtty = phuperu ? (phuperu * qtty) : qtty;
 		price = phuperu ? (price / phuperu) : price;
 	}
	ASSIGN_PTR(pQtty,  qtty);
	ASSIGN_PTR(pPrice, price);
	return 1;
}

int PPSupplExchange_Baltika::GetDlvrAddrHorecaCode(PPID * pDlvrAddrID, SString & rCode)
{
	rCode.Z();
	int    ok = -1;
	PPID   dlvr_addr_id = DEREFPTRORZ(pDlvrAddrID);
	if(dlvr_addr_id) {
		LocationTbl::Rec loc_rec;
		// @v10.6.4 MEMSZERO(loc_rec);
		if(DlvrAddrExtFldID && LocObj.Search(dlvr_addr_id, &loc_rec) > 0 && LocationCore::GetExField(&loc_rec, DlvrAddrExtFldID, rCode) > 0 && rCode.Len()) {
			dlvr_addr_id += HorecaDlvrAddrOffs;
			ok = 1;
		}
	}
	ASSIGN_PTR(pDlvrAddrID, dlvr_addr_id);
	return ok;
}

int PPSupplExchange_Baltika::Send()
{
	/*
	int    ok = 1;
	int connected = 0;
	InetAddr addr;
	addr.Set(ExchCfg.IP, ExchCfg.Port);
	THROW((connected = Sock.Connect(addr)) > 0);
	for(uint i = 0; i < Files.getCount(); i++) {
		const char * p_file = Files.at(i);
		SFile f;
		SBuffer sbuf;
		THROW_SL(f.Open(p_file, SFile::mRead));
		THROW_SL(f.Read(sbuf));
		THROW_SL(Sock.Send(sbuf, 0));
	}
	CATCHZOK
	if(connected > 0)
		Sock.Disconnect();
	return ok;
	*/
	return -1;
}

int EditSupplExpFilt(SupplExpFilt * pFilt, int selOnlySuppl)
{
	class SupplExpFiltDialog : public TDialog {
		DECL_DIALOG_DATA(SupplExpFilt);
		enum {
			ctlgroupLoc = 1
		};
	public:
		explicit SupplExpFiltDialog(int selOnlySuppl) : TDialog(DLG_SUPLEXPFLT), SelOnlySuppl(selOnlySuppl)
		{
			SetupCalPeriod(CTLCAL_SUPLEXPFLT_PRD, CTL_SUPLEXPFLT_PRD);
			addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_SUPLEXPFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		}
		DECL_DIALOG_SETDTS()
		{
			int    ok = 1;
			if(!RVALUEPTR(Data, pData))
				Data.Init();
			LocationCtrlGroup::Rec loc_rec(&Data.LocList);
			SetupPPObjCombo(this, CTLSEL_SUPLEXPFLT_GGRP, PPOBJ_GOODSGROUP, Data.GoodsGrpID, OLW_CANSELUPLEVEL, 0);
			{
				const PPID acs_id = GetSupplAccSheet();
				SetupArCombo(this, CTLSEL_SUPLEXPFLT_SUPPL, Data.SupplID, 0, acs_id, sacfDisableIfZeroSheet|sacfNonEmptyExchageParam);
			}
			setCtrlData(CTL_SUPLEXPFLT_DISCOUNT1, &Data.PctDis1);
			setCtrlData(CTL_SUPLEXPFLT_DISCOUNT2, &Data.PctDis2);
			AddClusterAssoc(CTL_SUPLEXPFLT_FLAGS, 0, SupplExpFilt::expBills);
			AddClusterAssoc(CTL_SUPLEXPFLT_FLAGS, 1, SupplExpFilt::expRest);
			AddClusterAssoc(CTL_SUPLEXPFLT_FLAGS, 2, SupplExpFilt::expPrice);
			AddClusterAssoc(CTL_SUPLEXPFLT_FLAGS, 3, SupplExpFilt::expSaldo);
			AddClusterAssoc(CTL_SUPLEXPFLT_FLAGS, 4, SupplExpFilt::expDelRecentBills);
			Data.Flags = (Data.Flags) ? Data.Flags : (SupplExpFilt::expDelRecentBills|SupplExpFilt::expBills);
			SetClusterData(CTL_SUPLEXPFLT_FLAGS, Data.Flags);
			SetPeriodInput(this, CTL_SUPLEXPFLT_PRD, &Data.Period);
			setGroupData(ctlgroupLoc, &loc_rec);
			disableCtrls(SelOnlySuppl, CTL_SUPLEXPFLT_FLAGS, CTLSEL_SUPLEXPFLT_OP, CTLSEL_SUPLEXPFLT_GGRP, CTL_SUPLEXPFLT_PRD, CTLSEL_SUPLEXPFLT_LOC, 0);
			enableCommand(cmLocList, !SelOnlySuppl);
			enableCommand(cmaMore,   !SelOnlySuppl);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			getCtrlData(sel = CTLSEL_SUPLEXPFLT_SUPPL, &Data.SupplID);
			THROW_PP(Data.SupplID, PPERR_INVSUPPL);
			getCtrlData(CTLSEL_SUPLEXPFLT_GGRP,  &Data.GoodsGrpID);
			if(!SelOnlySuppl) {
				LocationCtrlGroup::Rec loc_rec;
				GetClusterData(sel = CTL_SUPLEXPFLT_FLAGS, &Data.Flags);
				THROW_PP(Data.Flags, PPERR_INVEXPCFG);
				THROW(GetPeriodInput(this, sel = CTL_SUPLEXPFLT_PRD, &Data.Period));
				// getCtrlData(CTLSEL_SUPLEXPFLT_OP,    &Data.OpID);
				THROW(getGroupData(ctlgroupLoc, &loc_rec));
				Data.LocList = loc_rec.LocList;
				getCtrlData(sel = CTL_SUPLEXPFLT_DISCOUNT1, &Data.PctDis1);
				THROW_PP(Data.PctDis1 >= 0 && Data.PctDis1 <= 100, PPERR_PERCENTINPUT);
				getCtrlData(sel = CTL_SUPLEXPFLT_DISCOUNT2, &Data.PctDis2);
				THROW_PP(Data.PctDis2 >= 0 && Data.PctDis2 <= 100, PPERR_PERCENTINPUT);
			}
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SUPLEXPFLT_SUPPL)) {
				PPSupplAgreement suppl_agr;
				const PPID suppl_id = getCtrlLong(CTLSEL_SUPLEXPFLT_SUPPL);
				if(ArObj.GetSupplAgreement(suppl_id, &suppl_agr) > 0) {
					SString temp_buf;
					setCtrlData(CTLSEL_SUPLEXPFLT_GGRP, &suppl_agr.Ep.GoodsGrpID);
					Data.OpListFromCfg(&suppl_agr.Ep);
					// setCtrlData(CTLSEL_SUPLEXPFLT_OP, &suppl_agr.ExchCfg.OpID);
					Data.Period.low = suppl_agr.Ep.LastDt;
					SetPeriodInput(this, CTL_SUPLEXPFLT_PRD, &Data.Period);
					//InetAddr
					Data.Port   = suppl_agr.Ep.ConnAddr.GetPort();
					Data.IP     = static_cast<ulong>(suppl_agr.Ep.ConnAddr);
					Data.PriceQuotID = suppl_agr.Ep.PriceQuotID;
					suppl_agr.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssClientCode, temp_buf.Z());
					STRNSCPY(Data.ClientCode, temp_buf);
				}
			}
			else if(event.isCmd(cmaMore)) {
			}
			else
				return;
			clearEvent(event);
		}
		const int SelOnlySuppl;
		PPObjArticle ArObj;
	};
	DIALOG_PROC_BODY_P1ERR(SupplExpFiltDialog, selOnlySuppl, pFilt)
}
//
//
//
class SupplInterchangeFiltDialog : public TDialog {
	DECL_DIALOG_DATA(SupplInterchangeFilt);
	enum {
		ctlgroupLoc = 1
	};
public:
	SupplInterchangeFiltDialog() : TDialog(DLG_SUPPLIX)
	{
		SetupCalPeriod(CTLCAL_SUPPLIX_EXPPRD, CTL_SUPPLIX_EXPPRD);
		addGroup(ctlgroupLoc, new LocationCtrlGroup(CTLSEL_SUPPLIX_LOC, 0, 0, cmLocList, 0, 0, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		{
			const PPID acs_id = GetSupplAccSheet();
			SetupArCombo(this, CTLSEL_SUPPLIX_SUPPL, Data.SupplID, 0, acs_id, sacfDisableIfZeroSheet|sacfNonEmptyExchageParam);
		}
		setCtrlData(CTL_SUPPLIX_SPCDIS1, &Data.SpcDisPct1);
		setCtrlData(CTL_SUPPLIX_SPCDIS2, &Data.SpcDisPct2);
		//
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  0, SupplInterchangeFilt::opExportStocks);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  1, SupplInterchangeFilt::opExportBills);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  2, SupplInterchangeFilt::opExportDebts);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  3, SupplInterchangeFilt::opExportGoodsDebts);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  4, SupplInterchangeFilt::opExportClients);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  5, SupplInterchangeFilt::opExportPrices);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  6, SupplInterchangeFilt::opExportSales); // @v9.5.3
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  7, SupplInterchangeFilt::opImportGoods);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  8, SupplInterchangeFilt::opImportRouts);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS,  9, SupplInterchangeFilt::opImportOrders);
		AddClusterAssoc(CTL_SUPPLIX_ACTIONS, 10, SupplInterchangeFilt::opImportDesadv);
		SetClusterData(CTL_SUPPLIX_ACTIONS, Data.Actions);

		AddClusterAssoc(CTL_SUPPLIX_FLAGS, 0, SupplInterchangeFilt::fDeleteRecentBills); // @v9.2.5
		AddClusterAssoc(CTL_SUPPLIX_FLAGS, 1, SupplInterchangeFilt::fRepeatProcessing); // @v9.5.7
		AddClusterAssoc(CTL_SUPPLIX_FLAGS, 2, SupplInterchangeFilt::fTestMode); // @v9.6.0
		SetClusterData(CTL_SUPPLIX_FLAGS, Data.Flags); // @v9.2.5

		SetPeriodInput(this, CTL_SUPPLIX_EXPPRD, &Data.ExpPeriod);
		{
			LocationCtrlGroup::Rec loc_rec(&Data.LocList);
			setGroupData(ctlgroupLoc, &loc_rec);
		}
		Data.GetExtStrData(Data.extssParam, temp_buf); // @v9.5.0
		setCtrlString(CTL_SUPPLIX_ADDPARAM, temp_buf); // @v9.5.0
		//CATCHZOK
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		getCtrlData(sel = CTLSEL_SUPPLIX_SUPPL, &Data.SupplID);
		THROW_PP(Data.SupplID, PPERR_INVSUPPL);
		getCtrlData(CTL_SUPPLIX_SPCDIS1, &Data.SpcDisPct1);
		getCtrlData(CTL_SUPPLIX_SPCDIS2, &Data.SpcDisPct2);
		GetClusterData(CTL_SUPPLIX_ACTIONS, &Data.Actions);
		GetClusterData(CTL_SUPPLIX_FLAGS, &Data.Flags); // @v9.2.5
		THROW(GetPeriodInput(this, sel = CTL_SUPPLIX_EXPPRD, &Data.ExpPeriod));
		{
			LocationCtrlGroup::Rec loc_rec;
			THROW(getGroupData(ctlgroupLoc, &loc_rec));
			Data.LocList = loc_rec.LocList;
		}
		getCtrlString(CTL_SUPPLIX_ADDPARAM, temp_buf); // @v9.5.0
		Data.PutExtStrData(Data.extssParam, temp_buf); // @v9.5.0
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_SUPPLIX_SUPPL)) {
			PPSupplAgreement suppl_agr;
			const PPID suppl_id = getCtrlLong(CTLSEL_SUPPLIX_SUPPL);
			if(ArObj.GetSupplAgreement(suppl_id, &suppl_agr) > 0) {
				Data.ExpPeriod.low = suppl_agr.Ep.LastDt;
				SetPeriodInput(this, CTL_SUPPLIX_EXPPRD, &Data.ExpPeriod);
			}
			clearEvent(event);
		}
	}
	PPObjArticle ArObj;
};
//
// Import suppl data
//
int PPSupplExchange_Baltika::Import(const char * pPath)
{
#if 0 // @unused {
	class Exchanger : public PpyInetDataPrcssr {
	public:
		int SendRequest(const char * pURL, const char * request)
		{
			int    ok = 0;
			char * p_types[1] = {"text/*"};
			HINTERNET Connection = InternetConnect(InetSession, pURL,
				INTERNET_DEFAULT_HTTP_PORT, "", "", INTERNET_SERVICE_HTTP, 0, 0);
			HINTERNET h = HttpOpenRequest(Connection, "POST", request, (LPCSTR)"HTTP/1.1", NULL, (LPCSTR*)p_types, INTERNET_FLAG_KEEP_CONNECTION/*|INTERNET_FLAG_SECURE*/, 1);
			if(HttpSendRequest(h, NULL, 0, NULL, 0)) {
				char   buf[512];
				DWORD  read_bytes = 0;
				SFile  f;
				f.Open("c:\\temp\\1\\answer.html", SFile::mWrite);
				do {
					InternetReadFile(h, buf, sizeof(buf) - 1, &read_bytes);
					if(read_bytes)
						f.Write(buf, read_bytes);
				} while(read_bytes > 0);
				ok = 1;
				InternetCloseHandle(h);
				::ShellExecute(0, "open", "c:\\temp\\1\\answer.html", NULL, NULL, SW_SHOWNORMAL);
			}
			return ok;
		}
	};
#endif // } 0 @unused
	int    ok = 1, connected = 0;
	size_t rcv_bytes = 0;
	SString dir;
	InetAddr addr;
	SFile f;
	ResolveGoodsItemList goods_list;
	THROW_SL(f.Open(pPath, SFile::mRead));
	/*
	addr.Set(Filt.IP, Filt.Port);
	{
		Exchanger conn;
		THROW(conn.Init());
		conn.SendRequest("monolit.com",
			"/xDataLink/xDataLink.asmx/Request?XMLData=<?xml version=\"1.0\" encoding=\"windows-1251\"?><extdata user=\"DS108\"><scheme name=\"CRMWare\" request=\"get\"/></extdata>");
	}
	*/
	{
		size_t read_bytes = 0;
		char   fbuf[515];
		SString buf;
		StringSet ss("<r>");
		MEMSZERO(fbuf);
		while(f.Read(fbuf, sizeof(fbuf), &read_bytes) > 0) {
			buf.Cat(fbuf);
		}
		THROW_PP(buf.Len() > 0, PPERR_UNEXPEOF);
		ss.setBuf(buf, buf.Len() + 1);
		for(uint i = 0, j = 0; ss.get(&i, buf); j++) {
			if(j != 0) {
				ResolveGoodsItem gitem;
				StringSet ss2("<f>");
				ss2.setBuf(buf, buf.Len() + 1);
				for(uint k = 0, m = 0; ss2.get(&k, buf); m++) {
					uint p = 0;
					StringSet ss3("</f>");
					ss3.setBuf(buf, buf.Len() + 1);
					ss3.get(&p, buf);
					if(buf.Len() && !buf.HasPrefixIAscii("</r>")) {
						buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(m == 1)
							buf.CopyTo(gitem.Barcode, sizeof(gitem.Barcode));
						else if(m == 2)
							buf.CopyTo(gitem.GoodsName, sizeof(gitem.GoodsName));
						else if(m == 5)
							gitem.Quantity = buf.ToReal();
					}
				}
				if(sstrlen(gitem.Barcode) > 0 && sstrlen(gitem.GoodsName) > 0 && GObj.SearchByBarcode(gitem.Barcode, 0) <= 0 && GObj.P_Tbl->SearchByArCode(P.SupplID, gitem.Barcode, 0) <= 0)
					THROW_SL(goods_list.insert(&gitem));
			}
		}
		if(goods_list.getCount() && ResolveGoodsDlg(&goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_SHOWQTTY|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG) > 0) {
			for(uint i = 0; i < goods_list.getCount(); i++) {
				ResolveGoodsItem gitem = goods_list.at(i);
				if(gitem.ResolvedGoodsID && sstrlen(gitem.Barcode) > 0) {
					int r = 0;
					Goods2Tbl::Rec grec;
					// @v10.6.4 MEMSZERO(grec);
					THROW(r = GObj.P_Tbl->SearchByArCode(P.SupplID, gitem.Barcode, 0));
					if(r < 0) {
						THROW(GObj.P_Tbl->SetArCode(gitem.ResolvedGoodsID, P.SupplID, gitem.Barcode, static_cast<int32>(gitem.Quantity), 1));
						// THROW(GObj.P_Tbl->AddBarcode(gitem.ResolvedGoodsID, gitem.Barcode, gitem.Quantity, 1));
					}
				}
			}
		}
	}
	CATCHZOK
	f.Close();
	return ok;
}
//
//
//
class iSalesPepsi : public PrcssrSupplInterchange::ExecuteBlock {
public:
	struct ResultItem {
		int   Status; // 0 - error, 1 - ок
		SString ItemDescr;
		SString ErrMsg;
	};
	iSalesPepsi(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	~iSalesPepsi();
	int    Init();
	int    ParseResultString(const char * pText, TSCollection <iSalesPepsi::ResultItem> & rList, long * pErrItemCount) const;
	int    ReceiveGoods(int forceSettings, int useStorage);
	int    ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult);
	int    ReceiveReceipts();
	int    ReceiveOrders();
	int    ReceiveOrder_Csv(const char * pInBuf, size_t inBufLen);
	int    ReceiveVDocs();
	int    ReceiveUnclosedInvoices(TSCollection <iSalesBillDebt> & rResult);
	int    SendPrices();
	int    SendStocks();
	int    SendInvoices();
	int    SendDebts();
	int    SendStatus(const TSCollection <iSalesTransferStatus> & rList);
	int    GetOrderFilesFromMailServer(PPID mailAccID, const char * pDestPath, int deleMsg);
private:
	int    PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	int    Helper_MakeBillList(PPID opID, int outerDocType, const PPIDArray * pRegisteredAgentList, TSCollection <iSalesBillPacket> & rList);
	int    Helper_MakeBillEntry(PPID billID, PPBillPacket * pBp, int outerDocType, const PPIDArray * pRegisteredAgentList, TSCollection <iSalesBillPacket> & rList);
	void   Helper_Make_iSalesIdent(const BillTbl::Rec & rRec, int outerDocType, SString & rIdent) const;
	void   Helper_Parse_iSalesIdent(const SString & rIdent, SString & rCode, LDATE * pDate) const;
	void   SetupLocalPeriod(DateRange & rPeriod) const;
	int    GetGoodsStoreFileName(SString & rBuf) const;
	int    StoreGoods(TSCollection <iSalesGoodsPacket> & rList);
	int    RestoreGoods(TSCollection <iSalesGoodsPacket> & rList);
	int    SetGoodsArCode(PPID goodsID, const char * pArCode, int use_ta);
	int    LogErrors(const TSCollection <iSalesPepsi::ResultItem> & rResultList, const SString * pMsg);
	const iSalesGoodsPacket * SearchGoodsMappingEntry(const char * pOuterCode) const
	{
		const iSalesGoodsPacket * p_result = 0;
		if(State & stGoodsMappingInited && !isempty(pOuterCode)) {
			for(uint j = 0; !p_result && j < GoodsMapping.getCount(); j++) {
				const iSalesGoodsPacket * p_entry = GoodsMapping.at(j);
				if(p_entry && p_entry->OuterCode == pOuterCode)
					p_result = p_entry;
			}
		}
		return p_result;
	}
	const iSalesGoodsPacket * SearchGoodsMappingEntry(long nativeID) const
	{
		const iSalesGoodsPacket * p_result = 0;
		if(State & stGoodsMappingInited && nativeID) {
			for(uint j = 0; !p_result && j < GoodsMapping.getCount(); j++) {
				const iSalesGoodsPacket * p_entry = GoodsMapping.at(j);
				if(p_entry && p_entry->NativeCode.ToLong() == nativeID)
					p_result = p_entry;
			}
		}
		return p_result;
	}

	enum {
		stInited     = 0x0001,
		stEpDefined  = 0x0002,
		stGoodsMappingInited = 0x0004
	};
	long   State;
	long   UnknAgentID; // @v10.8.11 Специальный идентификатор агента, не закрепленного за поставщиком 
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	SString SvcUrl;
	SString UserName;
	SString Password;
	SString LastMsg;
	TSCollection <iSalesGoodsPacket> GoodsMapping;
	PPObjPerson PsnObj;
	PPLogger & R_Logger;
};

iSalesPepsi::iSalesPepsi(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) :
	PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger), State(0), P_DestroyFunc(0), UnknAgentID(0)
{
	PPGetFilePath(PPPATH_LOG, "isalespepsi.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapPepsi.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib)
			P_DestroyFunc = static_cast<void *>(P_Lib->GetProcAddr("iSalesDestroyResult"));
	}
}

iSalesPepsi::~iSalesPepsi()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

int iSalesPepsi::Init()
{
	State = 0;
	UnknAgentID = 0; // @v10.8.11
	SvcUrl.Z();
	UserName.Z();
	Password.Z();
	int    ok = 1;
	{
        Ep.GetExtStrData(Ep.extssRemoteAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		State |= stEpDefined;
	}
	InitGoodsList(0);
	// @v10.8.11 {
	if(P.SupplID) {
		PPID suppl_psn_id = ObjectToPerson(P.SupplID, 0);
		if(suppl_psn_id) {
			Reference * p_ref = PPRef;
			if(p_ref) {
				PPObjTag tag_obj;
				PPID   tag_id = 0;
				PPObjectTag2 tag_rec;
				if(tag_obj.SearchBySymb("UNKNAGENTCODE", &tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_PERSON) {
					ObjTagItem tag_item;
					if(p_ref->Ot.GetTag(PPOBJ_PERSON, suppl_psn_id, tag_id, &tag_item) > 0) {
						long   v = 0;
						if(tag_item.GetInt(&v) && v > 0)
							UnknAgentID = v;
					}
				}
			}
		}
	}
	// } @v10.8.11
	State |= stInited;
	return ok;
}

int iSalesPepsi::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL iSalesPepsi::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		static_cast<UHTT_DESTROYRESULT>(P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

int iSalesPepsi::ParseResultString(const char * pText, TSCollection <iSalesPepsi::ResultItem> & rList, long * pErrItemCount) const
{
	int    ok = 1;
	long   err_item_count = 0;
	if(isempty(pText)) {
		ok = -1;
	}
	else {
		StringSet ss("\x0D\x0A");
		StringSet ss_sub("|");
		ss.setBuf(pText, sstrlen(pText)+1);
		SString temp_buf, sub_buf;
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			ss_sub.clear();
			ss_sub.setBuf(temp_buf, temp_buf.Len()+1);
			uint ssip = 0;
			if(ss_sub.get(&ssip, temp_buf)) {
				ResultItem * p_new_item = rList.CreateNewItem();
				THROW_SL(p_new_item);
                p_new_item->Status = -1;
                p_new_item->ItemDescr = temp_buf;
                if(ss_sub.get(&ssip, temp_buf)) {
                	if(temp_buf.IsEqiAscii("OK"))
						p_new_item->Status = 1;
					else if(temp_buf.IsEqiAscii("ERR")) {
						p_new_item->Status = 0;
						err_item_count++;
					}
					//
					if(ss_sub.get(&ssip, temp_buf)) {
						p_new_item->ErrMsg = temp_buf;
					}
                }
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pErrItemCount, err_item_count);
	return ok;
}

int iSalesPepsi::LogErrors(const TSCollection <iSalesPepsi::ResultItem> & rResultList, const SString * pMsg)
{
	int    ok = 1;
	SString msg_buf;
	if(pMsg) {
		msg_buf = *pMsg;
		R_Logger.Log(msg_buf);
	}
	for(uint i = 0; i < rResultList.getCount(); i++) {
		const ResultItem * p_result_item = rResultList.at(i);
		if(p_result_item && p_result_item->Status == 0) {
            msg_buf.Z().Cat("ERR").Tab().Cat(p_result_item->ItemDescr).Tab().Cat(p_result_item->ErrMsg);
			R_Logger.Log(msg_buf);
		}
	}
	return ok;
}

struct iSalesGoodsStorageHeader {
	iSalesGoodsStorageHeader() : CRC(0)
	{
		memcpy(Signature, "ISGS", 4);
		MEMSZERO(Reserve);
	}
    char   Signature[4]; // "ISGS"
    uint32 CRC;
    uint8  Reserve[24];
};

int iSalesPepsi::GetGoodsStoreFileName(SString & rBuf) const
{
	return PPGetFilePath(PPPATH_OUT, "isalesgoodsstorage", rBuf.Z());
}

int iSalesPepsi::StoreGoods(TSCollection <iSalesGoodsPacket> & rList)
{
    int    ok = 1;
    SString file_name;
    SSerializeContext sctx;
    SBuffer buffer;
    THROW_SL(TSCollection_Serialize(rList, +1, buffer, &sctx));
    {
    	const  size_t bsize = buffer.GetAvailableSize();
    	iSalesGoodsStorageHeader hdr;
        // @v10.8.2 @ctr MEMSZERO(hdr);
        // @v10.8.2 @ctr memcpy(hdr.Signature, "ISGS", 4);
        SCRC32 cc;
        hdr.CRC = cc.Calc(0, buffer.GetBuf(0), bsize);
        //
        GetGoodsStoreFileName(file_name);
        SFile f_out(file_name, SFile::mWrite|SFile::mBinary);
        THROW_SL(f_out.IsValid());
        THROW_SL(f_out.Write(&hdr, sizeof(hdr)));
        THROW_SL(f_out.Write(buffer));
    }
    CATCHZOK
    return ok;
}

int iSalesPepsi::RestoreGoods(TSCollection <iSalesGoodsPacket> & rList)
{
    int    ok = 1;
    SString file_name;
    SSerializeContext sctx;
    SBuffer buffer;
    {
    	iSalesGoodsStorageHeader hdr;
    	GetGoodsStoreFileName(file_name);
    	SFile f_in(file_name, SFile::mRead|SFile::mBinary);
    	THROW_SL(f_in.IsValid());
        THROW_SL(f_in.Read(&hdr, sizeof(hdr)));
        THROW(memcmp(hdr.Signature, "ISGS", 4) == 0);
		THROW_SL(f_in.Read(buffer));
		{
			SCRC32 cc;
			uint32 _crc = cc.Calc(0, buffer.GetBuf(0), buffer.GetAvailableSize());
			THROW(_crc == hdr.CRC);
		}
		THROW_SL(TSCollection_Serialize(rList, -1, buffer, &sctx));
    }
    CATCHZOK
    return ok;
}

int iSalesPepsi::SetGoodsArCode(PPID goodsID, const char * pArCode, int use_ta)
{
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(goodsID && GObj.Search(goodsID, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC)) {
		int32  arcode_pack = 0;
		SString new_ar_code;
		SString msg_buf;
		SString ex_ar_code;
		GObj.P_Tbl->GetArCode(P.SupplID, goodsID, ex_ar_code, &arcode_pack);
		(new_ar_code = pArCode).Transf(CTRANSF_UTF8_TO_INNER);
		if(ex_ar_code != new_ar_code) {
			ArGoodsCodeTbl::Rec ex_ac_rec;
			Goods2Tbl::Rec ex_goods_rec;
			PPTransaction tra(use_ta);
			THROW(tra);
			if(GObj.P_Tbl->SearchByArCode(P.SupplID, new_ar_code, &ex_ac_rec, &ex_goods_rec) > 0) {
				assert(ex_goods_rec.ID == ex_ac_rec.GoodsID);
				assert(ex_goods_rec.ID != goodsID);
				THROW(GObj.P_Tbl->SetArCode(ex_goods_rec.ID, P.SupplID, 0, 0, 0));
				//PPTXT_LOG_SUPPLIX_RESETARCODE "Для товара '@goods' снят код по статье '@article' =@zstr поскольку должен быть перенесен на другой товар '@goods'"
				R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_RESETARCODE, &msg_buf, ex_goods_rec.ID, P.SupplID, new_ar_code.cptr(), goodsID));
				ok = 2;
			}
			else
				ok = 1;
			THROW(GObj.P_Tbl->SetArCode(goodsID, P.SupplID, new_ar_code, 0));
			THROW(tra.Commit());
			R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_SETARCODE, &msg_buf, goodsID, P.SupplID, new_ar_code.cptr()));
		}
	}
	CATCH
		R_Logger.LogLastError();
		ok = 0;
	ENDCATCH
	return ok;
}

int iSalesPepsi::ReceiveGoods(int forceSettings, int useStorage)
{
	int    ok = -1;
	State &= ~stGoodsMappingInited;
	PPSoapClientSession sess;
	SString lib_path;
	SString temp_buf, url;
	SString msg_buf;
	SString out_file_name;
	TSCollection <iSalesGoodsPacket> * p_result = 0;
	ISALESGETGOODSLIST_PROC func = 0;
	SString strg_file_name;
	GetGoodsStoreFileName(strg_file_name);
	int     storage_exists = 0;
	DateRange goods_query_period;
	goods_query_period.Z();
	if(useStorage && fileExists(strg_file_name)) {
		storage_exists = 1;
		LDATETIME mt;
        if(SFile::GetTime(strg_file_name, 0, 0, &mt) && diffdate(getcurdate_(), mt.d) <= 2) {
			if(RestoreGoods(GoodsMapping)) {
				goods_query_period.low = mt.d;
				State |= stGoodsMappingInited;
				ok = 2;
			}
        }
	}
	if(ok < 0 || forceSettings) {
		SString tech_buf;
		Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
		{
			PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_S, &msg_buf, tech_buf.cptr(), P.SupplID);
			PPWaitMsg(msg_buf);
		}
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = reinterpret_cast<ISALESGETGOODSLIST_PROC>(P_Lib->GetProcAddr("iSalesGetGoodsList")));
		sess.Setup(SvcUrl);
		if(forceSettings && P.ExpPeriod.low)
			goods_query_period = P.ExpPeriod;
		{
			DateRange * p_qp = 0;
			if(goods_query_period.low) {
				SETIFZ(goods_query_period.upp, encodedate(31, 12, 2030));
				p_qp = &goods_query_period;
			}
			p_result = func(sess, UserName, Password, p_qp);
		}
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		{
			for(uint nidx = 0; nidx < p_result->getCount(); nidx++) {
				const iSalesGoodsPacket * p_np = p_result->at(nidx);
				bool _found = false;
				for(uint pidx = 0; pidx < GoodsMapping.getCount(); pidx++) {
					iSalesGoodsPacket * p_pp = GoodsMapping.at(pidx);
					if(p_pp && p_pp->OuterCode == p_np->OuterCode) {
                        *p_pp = *p_np;
                        _found = true;
					}
				}
				if(!_found) {
                    iSalesGoodsPacket * p_new_item = GoodsMapping.CreateNewItem();
					THROW_SL(p_new_item);
					*p_new_item = *p_np;
				}
			}
			State |= stGoodsMappingInited;
			ok = 1;
			if(useStorage) {
				StoreGoods(GoodsMapping);
			}
			{
				PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_E, &msg_buf, p_result->getCountI(), tech_buf.cptr(), P.SupplID);
				PPWaitMsg(msg_buf);
			}
		}
		{
			PPGetFilePath(PPPATH_OUT, "isales-pepsi-products.txt", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			if(f_out.IsValid() && p_result->getCount()) {
				SString line_buf;
				SString ar_code;
				line_buf.Z().Cat("OuterCode").Tab().Cat("NativeCode").Tab().
					Cat("TypeOfProduct").Tab().Cat("UnitCode").Tab().Cat("VatRate").Tab().Cat("Name").Tab().Cat("Abbr").CatChar('|').
					Cat("Uom.count").Tab().Cat("uom.Barcode").Tab().Cat("uom.Code").Tab().Cat("uom.Netto").Tab().
					Cat("uom.Brutto").Tab().Cat("uom.Width").Tab().Cat("uom.Height").Tab().Cat("uom.Length").CatChar('|').
					Cat("Cvt.count").Tab().Cat("cvt.From").Tab().Cat("cvt.To").Tab().Cat("cvt.Rate");
				line_buf.Transf(CTRANSF_INNER_TO_UTF8);
				f_out.WriteLine(line_buf.CR());
				for(uint i = 0; i < p_result->getCount(); i++) {
					const iSalesGoodsPacket * p_item = p_result->at(i);
					if(p_item) {
						if(forceSettings) {
							SetGoodsArCode(p_item->NativeCode.ToLong(), p_item->OuterCode, 1);
						}
						line_buf.Z().Cat(p_item->OuterCode).Tab().Cat(p_item->NativeCode).Tab().
							Cat(p_item->TypeOfProduct).Tab().Cat(p_item->UnitCode).Tab().
							Cat(p_item->VatRate).Tab().Cat(p_item->Name).Tab().Cat(p_item->Abbr).CatChar('|').Cat(p_item->UomList.getCount());
						for(uint j = 0; j < p_item->UomList.getCount(); j++) {
							const iSalesUOM * p_uom = p_item->UomList.at(j);
							if(p_uom) {
								line_buf.CatChar('|');
								line_buf.Cat(p_uom->Barcode).Tab().Cat(p_uom->Code).Tab().
									Cat(p_uom->Netto).Tab().Cat(p_uom->Brutto).Tab().
									Cat(p_uom->Width).Tab().Cat(p_uom->Height).Tab().Cat(p_uom->Length);
							}
						}
						for(uint k = 0; k < p_item->CvtList.getCount(); k++) {
							const iSalesUOMCvt * p_cvt = p_item->CvtList.at(k);
							if(p_cvt) {
								line_buf.CatChar('|');
								line_buf.Cat(p_cvt->UomFrom).Tab().Cat(p_cvt->UomTo).Tab().Cat(p_cvt->Rate);
							}
						}
						line_buf.Transf(CTRANSF_INNER_TO_UTF8);
						f_out.WriteLine(line_buf.CR());
					}
				}
			}
		}
		DestroyResult(reinterpret_cast<void **>(&p_result));
	}
	CATCHZOK
	return ok;
}

int iSalesPepsi::ReceiveReceipts()
{
    int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString src_receipt_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;
	DateRange period;
	SString tech_buf;
	SetupLocalPeriod(period);
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPRCPT_S, &msg_buf, tech_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<ISALESGETRECEIPTLIST_PROC>(P_Lib->GetProcAddr("iSalesGetReceiptList")));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password, &period, /*1*/BIN(P.Flags & P.fRepeatProcessing) /*inclProcessedItems*/);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPRCPT_E, &msg_buf, (long)p_result->getCount(), tech_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	if(p_result->getCount()) {
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.EdiDesadvOpID && GetOpData(acfg.Hdr.EdiDesadvOpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillPacket * p_src_pack = p_result->at(i);
				if(p_src_pack) {
					BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					PPBillPacket pack;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;
					src_receipt_code = p_src_pack->Code;
					PPID   ar_id = P.SupplID;
					PPID   loc_id = P.LocList.GetSingle();
					if(!loc_id && P.LocList.GetCount() > 1) {
						for(uint locidx = 0; !loc_id && locidx < P.LocList.GetCount(); locidx++) {
							const PPID local_loc_id = P.LocList.Get(locidx);
							if(LocObj.Search(local_loc_id, &loc_rec) > 0) {
								loc_id = local_loc_id;
							}
						}
					}
					THROW(pack.CreateBlank_WithoutCode(acfg.Hdr.EdiDesadvOpID, 0, loc_id, 1));
					pack.SetupObject(ar_id, sob);
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d) ? p_src_pack->Dtm.d : getcurdate_();
					// @v11.1.12 STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					pack.SMemo = p_src_pack->Memo; // @v11.1.12
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
						;
					}
					else {
						for(uint j = 0; j < p_src_pack->Items.getCount(); j++) {
							const iSalesBillItem * p_src_item = p_src_pack->Items.at(j);
							if(p_src_item) {
								PPTransferItem ti;
								THROW(ti.Init(&pack.Rec));
								{
									const PPID _src_goods_id = p_src_item->NativeGoodsCode.ToLong();
									if(_src_goods_id && GObj.Fetch(_src_goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
										ti.GoodsID = goods_rec.ID;
										const double src_qtty = fabs(p_src_item->Qtty);
										double my_qtty = src_qtty;
										if(Ep.Fb.DefUnitID && p_src_item->UnitCode != Ep.Fb.DefUnitID) {
											const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(goods_rec.ID);
											if(p_entry)
												my_qtty = p_entry->RecalcUnits(p_src_item->UnitCode, Ep.Fb.DefUnitID, my_qtty);
										}
										ti.Quantity_ = my_qtty;
										const uint _ac = p_src_item->Amounts.getCount();
										if(_ac > 1) {
											const iSalesBillAmountEntry * p_src_amt_entry = p_src_item->Amounts.at(1);
											if(p_src_amt_entry)
												ti.Cost = (p_src_amt_entry->GrossSum + p_src_amt_entry->DiscGrossSum) / ti.Quantity_;
										}
										THROW(pack.LoadTItem(&ti, 0, 0));
									}
								}
							}
						}
						pack.SetupEdiAttributes(PPEDIOP_DESADV, "ISALES-PEPSI", p_src_pack->iSalesId);
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcReceipt;
							p_new_status->Ident.Z().Cat(p_src_pack->iSalesId);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
    CATCHZOK
    return ok;
}

void iSalesPepsi::SetupLocalPeriod(DateRange & rPeriod) const
{
	rPeriod = P.ExpPeriod;
	if(!checkdate(rPeriod.low))
		rPeriod.low = encodedate(1, 1, 2020);
	if(!checkdate(rPeriod.upp))
		rPeriod.upp = encodedate(31, 12, 2030);
}

int iSalesPepsi::ReceiveVDocs()
{
    int    ok = -1;
	int    treat_duedate_as_maindate = 0; // @v10.8.11
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString src_order_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result = 0;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;
	DateRange period;
	SString tech_buf;
	SetupLocalPeriod(period);
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	// @v10.8.11 {
	if(Ep.Fb.StyloPalmID) {
		PPObjStyloPalm sp_obj;
		PPStyloPalmPacket sp_pack;
		if(sp_obj.GetPacket(Ep.Fb.StyloPalmID, &sp_pack) > 0 && sp_pack.Rec.Flags & PLMF_TREATDUEDATEASDATE)
			treat_duedate_as_maindate = 1;
	}
	// } @v10.8.11 
	PPWaitMsg(PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_S, &msg_buf, tech_buf.cptr()));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<ISALESGETVDOCLIST_PROC>(P_Lib->GetProcAddr("iSalesGetVDocList")));
	sess.Setup(SvcUrl);
	{
		const int do_incl_processed_items = BIN(P.Flags & P.fRepeatProcessing);
		p_result = func(sess, UserName, Password, &period, do_incl_processed_items);
	}
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	PPWaitMsg(PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_E, &msg_buf, p_result->getCountI(), tech_buf.cptr()));
	if(p_result->getCount()) {
		const SString srcloc_attr_pattern("ORD_GROUPCODE1");
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		LocationTbl::Rec loc_rec;
		PPID   op_id = 0;
		{
			//
			// Первая встреченная операция с типом PPOPT_DRAFTRECEIPT и подтипом OPSUBT_RETURNREQ считается искомой
			//
			for(PPID enop_id = 0; EnumOperations(PPOPT_DRAFTRECEIPT, &enop_id, &op_rec) > 0;) {
				if(op_rec.SubType == OPSUBT_RETURNREQ) {
					op_id = op_rec.ID;
					break;
				}
			}
		}
		if(op_id) {
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillPacket * p_src_pack = p_result->at(i);
				if(p_src_pack && p_src_pack->Status == 0) { // (p_src_pack->Status == 0) отмененные заказы не проводить
					BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					PPBillPacket pack;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;
					src_order_code = p_src_pack->Code;
					const  PPID   _src_loc_id = p_src_pack->SrcLocCode.ToLong();
					temp_buf = p_src_pack->PayerCode;
					temp_buf.ShiftLeftChr('W').ShiftLeftChr('w');
					const  PPID   _src_psn_id = temp_buf.ToLong();
					const  PPID   _src_dlvrloc_id = p_src_pack->ShipTo.ToLong();
					const  PPID   _src_agent_id = p_src_pack->AgentCode.ToLong();
					PPID   ar_id = 0;
					PPID   loc_id = 0;
					{
						for(uint aidx = 0; aidx < p_src_pack->Attrs.getCount(); aidx++) {
							const iSalesExtAttr * p_attr = p_src_pack->Attrs.at(aidx);
							if(p_attr && p_attr->Name.CmpNC(srcloc_attr_pattern) == 0) {
								if(p_attr->Value.NotEmpty()) {
									PPID   temp_wh_id = 0;
									if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, p_attr->Value, &temp_wh_id) > 0)
										loc_id = temp_wh_id;
								}
								break;
							}
						}
					}
					if(!loc_id && _src_loc_id && LocObj.Fetch(_src_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE)
						loc_id = loc_rec.ID;
					SETIFZ(loc_id, P.LocList.GetSingle());
					THROW(pack.CreateBlank_WithoutCode(op_id, 0, loc_id, 1));
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d) ? p_src_pack->Dtm.d : getcurdate_();
					pack.Rec.DueDate = checkdate(p_src_pack->IncDtm.d) ? p_src_pack->IncDtm.d : ZERODATE;
					// @v10.8.11 {
					if(treat_duedate_as_maindate && checkdate(pack.Rec.DueDate))
						pack.Rec.Dt = pack.Rec.DueDate;
					// } @v10.8.11 
					// @v11.1.12 STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					pack.SMemo = p_src_pack->Memo; // @v11.1.12
					{
						PPID   local_psn_id = _src_psn_id;
						if(_src_dlvrloc_id && LocObj.Search(_src_dlvrloc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS && 
							(!loc_rec.OwnerID || loc_rec.OwnerID == _src_psn_id || _src_psn_id == _src_dlvrloc_id)) {
							if(_src_psn_id == _src_dlvrloc_id && loc_rec.OwnerID) {
								local_psn_id = loc_rec.OwnerID;
							}
							pack.SetFreight_DlvrAddrOnly(_src_dlvrloc_id);
						}
						else
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_dlvrloc_id));
						if(local_psn_id && ArObj.P_Tbl->PersonToArticle(local_psn_id, op_rec.AccSheetID, &ar_id) > 0) {
							if(!pack.SetupObject(ar_id, sob))
								R_Logger.LogLastError();
						}
						else
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLINID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_psn_id));
					}
					if(_src_agent_id && ArObj.P_Tbl->PersonToArticle(_src_agent_id, GetAgentAccSheet(), &(ar_id = 0)) > 0) {
						pack.Ext.AgentID = ar_id;
					}
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
						;
					}
					else {
						for(uint j = 0; j < p_src_pack->Items.getCount(); j++) {
							const iSalesBillItem * p_src_item = p_src_pack->Items.at(j);
							if(p_src_item) {
								PPTransferItem ti;
								THROW(ti.Init(&pack.Rec));
								{
									const PPID _src_goods_id = p_src_item->NativeGoodsCode.ToLong();
									if(_src_goods_id && GObj.Fetch(_src_goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
										const double src_qtty = fabs(p_src_item->Qtty);
										double my_qtty = src_qtty;
										if(Ep.Fb.DefUnitID && p_src_item->UnitCode != Ep.Fb.DefUnitID) {
											const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(goods_rec.ID);
											if(p_entry)
												my_qtty = p_entry->RecalcUnits(p_src_item->UnitCode, Ep.Fb.DefUnitID, my_qtty);
										}
										ti.GoodsID = goods_rec.ID;
										ti.Quantity_ = my_qtty;
										const uint _ac = p_src_item->Amounts.getCount();
										if(_ac > 1) {
											const iSalesBillAmountEntry * p_src_amt_entry = p_src_item->Amounts.at(1);
											if(p_src_amt_entry) {
												ti.Price = (p_src_amt_entry->GrossSum + p_src_amt_entry->DiscGrossSum) / ti.Quantity_;
												ti.Discount = p_src_amt_entry->DiscGrossSum / ti.Quantity_;
											}
										}
										THROW(pack.LoadTItem(&ti, 0, 0));
									}
									else
										R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_goods_id));
								}
							}
						}
						pack.SetupEdiAttributes(PPEDIOP_RETURNREQ, "ISALES-PEPSI", p_src_pack->iSalesId);
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcReturnReq;
							p_new_status->Ident.Z().Cat(src_order_code);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
    CATCHZOK
	return ok;
}

int iSalesPepsi::GetOrderFilesFromMailServer(PPID mailAccID, const char * pDestPath, int deleMsg)
{
	int    ok = 1;
	SString temp_buf;
	SString eq_buf;
	SString enc_buf;
	PPID   mail_acc_id = mailAccID;
	PPObjInternetAccount mac_obj;
	PPInternetAccount mac_rec;
	if(mail_acc_id == 0) {
		PPAlbatrossConfig cfg;
		THROW(PPAlbatrosCfgMngr::Get(&cfg) > 0);
		mail_acc_id = cfg.Hdr.MailAccID;
	}
	THROW_PP(mail_acc_id, PPERR_UNDEFMAILACC);
	THROW_PP(mac_obj.Get(mail_acc_id, &mac_rec) > 0, PPERR_UNDEFMAILACC);
	PPWaitStart();
	{
		InetUrl url;
		SUniformFileTransmParam uftp;
		uftp.DestPath = pDestPath;
		{
			mac_rec.GetExtField(MAEXSTR_RCVSERVER, temp_buf);
			url.SetComponent(url.cHost, temp_buf);
			url.SetProtocol((mac_rec.Flags & mac_rec.fUseSSL) ? InetUrl::protPOP3S : InetUrl::protPOP3);
			int    port = mac_rec.GetRcvPort();
			if(port)
				url.SetPort_(port);
			mac_rec.GetExtField(MAEXSTR_RCVNAME, temp_buf);
			enc_buf.EncodeUrl(temp_buf, 0);
			url.SetComponent(url.cUserName, enc_buf);
			{
				char pw[128];
				mac_rec.GetPassword_(pw, sizeof(pw), MAEXSTR_RCVPASSWORD);
				enc_buf.EncodeUrl(temp_buf = pw, 0);
				url.SetComponent(url.cPassword, enc_buf);
				memzero(pw, sizeof(pw));
				enc_buf.Obfuscate();
				temp_buf.Obfuscate();
			}
		}
		url.SetQueryParam("wildcard", "*.csv");
		url.Composite(0, temp_buf);
		uftp.SrcPath = temp_buf;
		if(deleMsg)
			uftp.Flags |= uftp.fDeleteAfter;
		uftp.Pop3TopMaxLines = 200;
		THROW_SL(uftp.Run(/*GetFilesFromMailServerProgressProc*/0, 0));
		temp_buf = uftp.Reply;
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

// @v11.3.6 @construction {
/*
Пример файла в csv-формате для резервного приема заказов через email
--------------------
r01;Order No;83542-180322-134410-17948383;-180322-134410-20438;1C_PAYER_ID;20438
r02;;;0.07
r03;SalesOrg;RU20;iSales_Dealer_Id;491;isales_Doc_Type_ID;13;Order_date;18.03.2022
r04;Shipment Date;19.03.2022;ID customer;;20438
r05;GPIDCR;80917084;Route;RU6890
r06;1C CR Code;;1C_Warehouse_ID;101;Doc_Attributes;;;;;
r07;Comments;;
r08;Product;Product Name;Category;Product Quantity;Product unit code;Promo Applied;1C product code;UOM;Promo discount;Promo discount without VAT;Promo Desc;isales_Product_Code;Price for one Unit;Amount_without_VAT;Price for one Unit with VAT;Amount_with_VAT
r09;340033481;Чудо ЙогФр ЯгоднМорож 2.4% 270г БП 15Х;МОЛОЧНЫЕ ПРОДУКТЫ;1.00;EA;;407212;1004;0.0000000000;0.0000000000;;18252;58.9454550000;58.9454550000;64.8400000000;64.8400000000
*/
int iSalesPepsi::ReceiveOrder_Csv(const char * pInBuf, size_t inBufLen)
{
	class InnerBlock {
	public:
		static size_t ReadLine(const char * pInBuf, size_t inBufLen, SString & rBuf)
		{
			rBuf.Z();
			size_t result = 0;
			if(!isempty(pInBuf)) {
				for(size_t p = 0; p < inBufLen; p++) {
					if(pInBuf[p] == 0) {
						// Здесь result не инкрементируем дабы при следующем вызове функции не оказаться "за нулем".
						break;
					}
					else {
						uint el = iseol(pInBuf+p, eolAny);
						if(el) {
							result += el;
							break;
						}
						else {
							rBuf.CatChar(pInBuf[p]);
							result++;
						}
					}
				}
			}
			return result;
		}
	};
	int    ok = -1;
	SString msg_buf;
	if(!isempty(pInBuf) && inBufLen) {
		SString line_buf;
		StringSet ss(";");
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		LocationTbl::Rec loc_rec;
		PPID   loc_id = 0;
		const PPID  op_id = acfg.Hdr.OpID;
		if(op_id && GetOpData(op_id, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			PPBillPacket pack;
			SString isales_ident;
			SString token;
			SString prev_token;
			PPBillPacket::SetupObjectBlock sob;
			THROW(pack.CreateBlank_WithoutCode(op_id, 0, /*loc_id*/0, 1));
			for(size_t offs = 0; inBufLen > offs;) {
				size_t ll = InnerBlock::ReadLine(pInBuf+offs, inBufLen-offs, line_buf);
				offs += ll;
				if(line_buf.NotEmptyS()) {
					ss.setBuf(line_buf.Chomp());
					//line_buf.Tokenize(";", ss.Z());
					uint ssp = 0;
					if(ss.get(&ssp, token)) {
						if(token.IsEqiAscii("r01")) {
							// ID агента iSales-Дата-Время-Код клиента iSales
							// r01;Order No;83542-180322-134410-17948383;-180322-134410-20438;1C_PAYER_ID;20438
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("Order No")) {
									STRNSCPY(pack.Rec.Code, token);
								}
								else if(prev_token.IsEqiAscii("1C_PAYER_ID")) {
									;
								}
							}
						}
						else if(token.IsEqiAscii("r02")) {
							// r02;;;0.07
						}
						else if(token.IsEqiAscii("r03")) {
							// r03;SalesOrg;RU20;iSales_Dealer_Id;491;isales_Doc_Type_ID;13;Order_date;18.03.2022
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("SalesOrg")) {
									;
								}
								else if(prev_token.IsEqiAscii("iSales_Dealer_Id")) {
									;
								}
								else if(prev_token.IsEqiAscii("isales_Doc_Type_ID")) {
									;
								}
								else if(prev_token.IsEqiAscii("Order_date")) {
									LDATE dt = strtodate_(token, DATF_GERMAN);
									if(checkdate(dt))
										pack.Rec.Dt = dt;
								}
							}
						}
						else if(token.IsEqiAscii("r04")) {
							// r04;Shipment Date;19.03.2022;ID customer;;20438
							PPID   native_cli_id = 0;
							PPID   native_dlvrloc_id = 0;
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("Shipment Date")) {
									LDATE dt = strtodate_(token, DATF_GERMAN);
									if(checkdate(dt))
										pack.Rec.DueDate = dt;
								}
								else if(prev_token.IsEqiAscii("ID customer")) {
									native_cli_id = token.ToLong();
									//
									prev_token = token;
									if(ss.get(&ssp, token)) {
										native_dlvrloc_id = token.ToLong();
									}
								}
							}
							{
								PPID   local_psn_id = native_cli_id;
								PPID   ar_id = 0;
								if(native_dlvrloc_id && LocObj.Search(native_dlvrloc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS && 
									(!loc_rec.OwnerID || !native_cli_id || loc_rec.OwnerID == native_cli_id || native_cli_id == native_dlvrloc_id)) {
									if(!native_cli_id)
										local_psn_id = loc_rec.OwnerID;
									else if(native_cli_id == native_dlvrloc_id && loc_rec.OwnerID)
										local_psn_id = loc_rec.OwnerID;
									pack.SetFreight_DlvrAddrOnly(native_dlvrloc_id);
								}
								else
									R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), native_dlvrloc_id));
								if(local_psn_id && ArObj.P_Tbl->PersonToArticle(local_psn_id, op_rec.AccSheetID, &ar_id) > 0) {
									sob.Flags |= sob.fEnableStop;
									if(!pack.SetupObject(ar_id, sob))
										R_Logger.LogLastError();
								}
								else
									R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLINID, &msg_buf, static_cast<const char *>(pack.Rec.Code), native_cli_id));
							}
						}
						else if(token.IsEqiAscii("r05")) {
							// r05;GPIDCR;80917084;Route;RU6890
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("GPIDCR")) {
									;
								}
								else if(prev_token.IsEqiAscii("Route")) {
									;
								}
							}
						}
						else if(token.IsEqiAscii("r06")) {
							// r06;1C CR Code;;1C_Warehouse_ID;101;Doc_Attributes;;;;;
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("1C CR Code")) {
									;
								}
								else if(prev_token.IsEqiAscii("1C_Warehouse_ID")) {
									loc_id = token.ToLong();
									LocationTbl::Rec loc_rec;
									if(LocObj.Search(loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE) {
										pack.Rec.LocID = loc_rec.ID;
									}
								}
								else if(prev_token.IsEqiAscii("Doc_Attributes")) {
									;
								}
							}
						}
						else if(token.IsEqiAscii("r07")) {
							// r07;Comments;;
							for(prev_token = token; ss.get(&ssp, token); prev_token = token) {
								if(prev_token.IsEqiAscii("Comments")) {
									token.Transf(CTRANSF_UTF8_TO_INNER).Strip();
									pack.SMemo = token;
								}
							}
						}
						else if(token.IsEqiAscii("r08")) {
							// r08;Product;Product Name;Category;Product Quantity;Product unit code;Promo Applied;1C product code;UOM;Promo discount;Promo discount without VAT;Promo Desc;isales_Product_Code;Price for one Unit;Amount_without_VAT;Price for one Unit with VAT;Amount_with_VAT
						}
						else if(token.IsEqiAscii("r09")) {
							// r09;340033481;Чудо ЙогФр ЯгоднМорож 2.4% 270г БП 15Х;МОЛОЧНЫЕ ПРОДУКТЫ;1.00;EA;;407212;1004;0.0000000000;0.0000000000;;18252;58.9454550000;58.9454550000;64.8400000000;64.8400000000
							PPTransferItem ti;
							PPID   native_goods_id = 0;
							PPID   native_unit_id = 0;
							double qtty = 0.0;
							double price = 0.0;
							double discount = 0.0;
							ti.Init(&pack.Rec);
							for(uint tok_n = 1; ss.get(&ssp, token); tok_n++) {
								switch(tok_n) {
									case 1: // Product
										break;
									case 2: // Product Name 
										break;
									case 3: // Category
										break;
									case 4: // Product Quantity
										qtty = token.ToReal();
										break;
									case 5: // Product unit code
										break;
									case 6: // Promo Applied
										break;
									case 7: // 1C product code
										native_goods_id = token.ToLong();
										break;
									case 8: // UOM
										native_unit_id = token.ToLong();
										break;
									case 9: // Promo discount
										discount = token.ToReal();
										break;
									case 10: // Promo discount without VAT
										break;
									case 11: // Promo Desc
										break;
									case 12: // isales_Product_Code
										break;
									case 13: // Price for one Unit
										break;
									case 14: // Amount_without_VAT
										break;
									case 15: // Price for one Unit with VAT
										price = token.ToReal();
										break;
									case 16: // Amount_with_VAT
										break;
								}
							}
							{
								Goods2Tbl::Rec goods_rec;
								if(native_goods_id && GObj.Fetch(native_goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
									const double src_qtty = fabs(qtty);
									double my_qtty = src_qtty;
									if(Ep.Fb.DefUnitID && native_unit_id != Ep.Fb.DefUnitID) {
										const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(goods_rec.ID);
										if(p_entry)
											my_qtty = p_entry->RecalcUnits(native_unit_id, Ep.Fb.DefUnitID, my_qtty);
									}
									ti.GoodsID = goods_rec.ID;
									ti.Quantity_ = my_qtty;
									ti.Price = price;
									if(discount <= price)
										ti.Discount = discount;
									THROW(pack.LoadTItem(&ti, 0, 0));
								}
								else
									R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), native_goods_id));
							}
						}
					}
				}
			}
			if(pack.GetTCount()) {
				TSCollection <iSalesBillPacket> * p_result = 0;
				TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
				pack.SetupEdiAttributes(PPEDIOP_SALESORDER, "ISALES-PEPSI", /*p_src_pack->iSalesId*/isales_ident);
				pack.InitAmounts();
				THROW(P_BObj->TurnPacket(&pack, 1));
				{
					iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
					THROW_SL(p_new_status);
					p_new_status->Ifc = iSalesTransferStatus::ifcOrder;
					//p_new_status->Ident.Z().CatChar('O').Cat(src_order_code);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
// } @v11.3.6 @construction

int iSalesPepsi::ReceiveOrders()
{
    int    ok = -1;
	int    treat_duedate_as_maindate = 0; // @v10.8.11
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString src_order_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result = 0;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;
	DateRange period;
	SString tech_buf;
	SetupLocalPeriod(period);
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	// @v10.8.11 {
	if(Ep.Fb.StyloPalmID) {
		PPObjStyloPalm sp_obj;
		PPStyloPalmPacket sp_pack;
		if(sp_obj.GetPacket(Ep.Fb.StyloPalmID, &sp_pack) > 0 && sp_pack.Rec.Flags & PLMF_TREATDUEDATEASDATE)
			treat_duedate_as_maindate = 1;
	}
	// } @v10.8.11 
	PPWaitMsg(PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_S, &msg_buf, tech_buf.cptr()));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<ISALESGETORDERLIST_PROC>(P_Lib->GetProcAddr("iSalesGetOrderList")));
	sess.Setup(SvcUrl);
	{
		const int do_incl_processed_items = BIN(P.Flags & P.fRepeatProcessing);
		p_result = func(sess, UserName, Password, &period, do_incl_processed_items);
	}
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	PPWaitMsg(PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_E, &msg_buf, p_result->getCountI(), tech_buf.cptr()));
	if(p_result->getCount()) {
		const SString srcloc_attr_pattern("ORD_GROUPCODE1");
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		LocationTbl::Rec loc_rec;
		const PPID  op_id = acfg.Hdr.OpID;
		if(op_id && GetOpData(op_id, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillPacket * p_src_pack = p_result->at(i);
				if(p_src_pack && p_src_pack->Status == 0) { // (p_src_pack->Status == 0) отмененные заказы не проводить
					BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					PPBillPacket pack;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;
					src_order_code = p_src_pack->Code;
					const  PPID   _src_loc_id = p_src_pack->SrcLocCode.ToLong();
					temp_buf = p_src_pack->PayerCode;
					temp_buf.ShiftLeftChr('W').ShiftLeftChr('w');
					const  PPID   _src_psn_id = temp_buf.ToLong();
					const  PPID   _src_dlvrloc_id = p_src_pack->ShipTo.ToLong();
					const  PPID   _src_agent_id = p_src_pack->AgentCode.ToLong();
					PPID   ar_id = 0;
					PPID   loc_id = 0;
					{
						for(uint aidx = 0; aidx < p_src_pack->Attrs.getCount(); aidx++) {
							const iSalesExtAttr * p_attr = p_src_pack->Attrs.at(aidx);
							if(p_attr && p_attr->Name.CmpNC(srcloc_attr_pattern) == 0) {
								if(p_attr->Value.NotEmpty()) {
									PPID   temp_wh_id = 0;
									if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, p_attr->Value, &temp_wh_id) > 0)
										loc_id = temp_wh_id;
								}
								break;
							}
						}
					}
					if(!loc_id && _src_loc_id && LocObj.Fetch(_src_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE)
						loc_id = loc_rec.ID;
					SETIFZ(loc_id, P.LocList.GetSingle()); // @v10.8.12
					THROW(pack.CreateBlank_WithoutCode(op_id, 0, loc_id, 1));
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d) ? p_src_pack->Dtm.d : getcurdate_();
					pack.Rec.DueDate = checkdate(p_src_pack->IncDtm.d) ? p_src_pack->IncDtm.d : ZERODATE;
					// @v10.8.11 {
					if(treat_duedate_as_maindate && checkdate(pack.Rec.DueDate))
						pack.Rec.Dt = pack.Rec.DueDate;
					// } @v10.8.11 
					// @v11.1.12 STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					pack.SMemo = p_src_pack->Memo; // @v11.1.12
					{
						PPID   local_psn_id = _src_psn_id;
						if(_src_dlvrloc_id && LocObj.Search(_src_dlvrloc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS && 
							(!loc_rec.OwnerID || loc_rec.OwnerID == _src_psn_id || _src_psn_id == _src_dlvrloc_id)) {
							if(_src_psn_id == _src_dlvrloc_id && loc_rec.OwnerID) {
								local_psn_id = loc_rec.OwnerID;
							}
							pack.SetFreight_DlvrAddrOnly(_src_dlvrloc_id);
						}
						else
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_dlvrloc_id));
						if(local_psn_id && ArObj.P_Tbl->PersonToArticle(local_psn_id, op_rec.AccSheetID, &ar_id) > 0) {
							sob.Flags |= sob.fEnableStop; // @v10.9.0
							if(!pack.SetupObject(ar_id, sob))
								R_Logger.LogLastError();
						}
						else
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLINID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_psn_id));
					}
					if(_src_agent_id && ArObj.P_Tbl->PersonToArticle(_src_agent_id, GetAgentAccSheet(), &(ar_id = 0)) > 0) {
						pack.Ext.AgentID = ar_id;
					}
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
						;
					}
					else {
						for(uint j = 0; j < p_src_pack->Items.getCount(); j++) {
							const iSalesBillItem * p_src_item = p_src_pack->Items.at(j);
							if(p_src_item) {
								PPTransferItem ti;
								THROW(ti.Init(&pack.Rec));
								{
									const PPID _src_goods_id = p_src_item->NativeGoodsCode.ToLong();
									if(_src_goods_id && GObj.Fetch(_src_goods_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
										const double src_qtty = fabs(p_src_item->Qtty);
										double my_qtty = src_qtty;
										if(Ep.Fb.DefUnitID && p_src_item->UnitCode != Ep.Fb.DefUnitID) {
											const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(goods_rec.ID);
											if(p_entry)
												my_qtty = p_entry->RecalcUnits(p_src_item->UnitCode, Ep.Fb.DefUnitID, my_qtty);
										}
										ti.GoodsID = goods_rec.ID;
										ti.Quantity_ = my_qtty;
										const uint _ac = p_src_item->Amounts.getCount();
										if(_ac > 1) {
											const iSalesBillAmountEntry * p_src_amt_entry = p_src_item->Amounts.at(1);
											if(p_src_amt_entry) {
												ti.Price = (p_src_amt_entry->GrossSum + p_src_amt_entry->DiscGrossSum) / ti.Quantity_;
												ti.Discount = p_src_amt_entry->DiscGrossSum / ti.Quantity_;
											}
										}
										THROW(pack.LoadTItem(&ti, 0, 0));
									}
									else
										R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNID, &msg_buf, static_cast<const char *>(pack.Rec.Code), _src_goods_id));
								}
							}
						}
						pack.SetupEdiAttributes(PPEDIOP_SALESORDER, "ISALES-PEPSI", p_src_pack->iSalesId);
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcOrder;
							p_new_status->Ident.Z().CatChar('O').Cat(src_order_code);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
    CATCHZOK
    return ok;
}

int iSalesPepsi::ReceiveUnclosedInvoices(TSCollection <iSalesBillDebt> & rResult)
{
	rResult.freeAll();

	int    ok = -1;
	PPSoapClientSession sess;
	SString lib_path;
	SString temp_buf, url;
	SString out_file_name;
	TSCollection <iSalesBillDebt> * p_result = 0;
	ISALESGETUNCLOSEDBILLLIST_PROC func = 0;
	DateRange period;
	SetupLocalPeriod(period);
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<ISALESGETUNCLOSEDBILLLIST_PROC>(P_Lib->GetProcAddr("iSalesGetUnclosedBillList")));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password, &period);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		THROW_SL(TSCollection_Copy(rResult, *p_result));
	}
	{
		PPGetFilePath(PPPATH_OUT, "isales-pepsi-unclosedbills.txt", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillDebt * p_item = p_result->at(i);
				if(p_item) {
					line_buf.Z().Cat(p_item->iSalesId).Tab().Cat(p_item->DocType).Tab().Cat(p_item->Code).Tab().
						Cat(p_item->Dtm, DATF_DMY, TIMF_HMS).Tab().Cat(p_item->PayerCode).Tab().Cat(p_item->Amount, MKSFMTD(0, 2, 0)).Tab().
						Cat(p_item->Debt, MKSFMTD(0, 2, 0)).Tab().Cat(p_item->ErrMsg);
					line_buf.Transf(CTRANSF_INNER_TO_UTF8);
					f_out.WriteLine(line_buf.CR());
				}
			}
		}
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
	CATCHZOK
	return ok;
}

int iSalesPepsi::ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult)
{
	rResult.freeAll();
	int    ok = -1;
	PPSoapClientSession sess;
	SString lib_path;
	SString temp_buf, url;
	SString out_file_name;
	TSCollection <iSalesRoutePacket> * p_result = 0;
	ISALESGETROUTELIST_PROC func = 0;
	DateRange period;
	if(P.ExpPeriod.low && P.ExpPeriod.upp)
		period = P.ExpPeriod;
	else
		period.SetDate(getcurdate_());
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<ISALESGETROUTELIST_PROC>(P_Lib->GetProcAddr("iSalesGetRouteList")));
	sess.Setup(SvcUrl);
	p_result = func(sess, UserName, Password, &period);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		THROW_SL(TSCollection_Copy(rResult, *p_result));
	}
#if 0 // @debug {
	{
		PPGetFilePath(PPPATH_OUT, "isales-pepsi-routs.txt", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		if(f_out.IsValid()) {
			SString line_buf;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesRoutePacket * p_item = p_result->at(i);
				if(p_item) {
					line_buf.Z().Cat(p_item->Ident).Tab().Cat(p_item->TypeOfRoute).Tab().
						Cat(p_item->NativeAgentCode).Tab().Cat(p_item->Valid).Tab().Cat(p_item->ErrMsg).Tab().
						Cat(p_item->VisitList.getCount());
					for(uint j = 0; j < p_item->VisitList.getCount(); j++) {
                        const iSalesVisit * p_visit = p_item->VisitList.at(j);
                        if(p_visit) {
							line_buf.Tab();
							line_buf.Cat(p_visit->Ident).Tab().Cat(p_visit->OuterClientCode).Tab().Cat(p_visit->InnerClientCode).Tab().
								Cat(p_visit->InitDate, DATF_DMY|DATF_CENTURY).Tab().Cat(p_visit->Freq).Tab().
								Cat(p_visit->DayOfWeek).Tab().Cat(p_visit->Order).Tab().Cat(p_visit->Valid);
                        }
					}
					line_buf.Transf(CTRANSF_INNER_TO_UTF8);
					f_out.WriteLine(line_buf.CR());
				}
			}
		}
	}
#endif // } 0
	DestroyResult(reinterpret_cast<void **>(&p_result));
	CATCHZOK
	return ok;
}

int iSalesPepsi::SendStatus(const TSCollection <iSalesTransferStatus> & rList)
{
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	if(rList.getCount()) {
		int    result = 0;
		ISALESPUTTRANSFERSTATUS_PROC func = 0;
		THROW_SL(func = reinterpret_cast<ISALESPUTTRANSFERSTATUS_PROC>(P_Lib->GetProcAddr("iSalesPutTransferStatus")));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &rList);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
		DestroyResult(reinterpret_cast<void **>(&p_result));
    }
    CATCHZOK
	return ok;
}

IMPL_CMPFUNC(iSalesBillDebt, p1, p2)
{
	const iSalesBillDebt * b1 = static_cast<const iSalesBillDebt *>(p1);
	const iSalesBillDebt * b2 = static_cast<const iSalesBillDebt *>(p2);
	int    si = cmp(b1->Dtm, b2->Dtm);
	SETIFZ(si, stricmp866(b1->Code, b2->Code));
	return si;
}

int iSalesPepsi::SendDebts()
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	TSCollection <iSalesBillDebt> outer_debt_list;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK); // @v10.9.0
	THROW(ReceiveUnclosedInvoices(outer_debt_list));
	if(outer_debt_list.getCount()) {
		PPIDArray processed_id_list;
		StringSet processed_outer_list;
		int    result = 0;
		int    do_send = 0;
		uint   i;
		SString temp_buf;
		outer_debt_list.sort(PTR_CMPFUNC(iSalesBillDebt));
		TSCollection <iSalesBillDebt> current_debt_list;
		uint   first_idx_by_date = 0;
		LDATE  prev_date = ZERODATE;
		const  uint _c = outer_debt_list.getCount();
		for(i = 0; i <= _c; i++) { // (<=) - последняя итерация
			const iSalesBillDebt * p_outer_bill = (i < _c) ? outer_debt_list.at(i) : 0;
			if(p_outer_bill || i == _c) {
				if(i == _c || (prev_date && p_outer_bill->Dtm.d != prev_date)) {
					BillTbl::Rec bill_rec;
					for(DateIter di(prev_date, prev_date); P_BObj->P_Tbl->EnumByDate(&di, &bill_rec) > 0;) {
						if(bill_rec.Amount > 0.0 && GetOpType(bill_rec.OpID) != PPOPT_GOODSORDER && !processed_id_list.lsearch(bill_rec.ID)) {
							if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_rec.ID, bill_ack_tag_id, temp_buf) > 0) { // @v10.9.0
								// @v11.1.12 BillCore::GetCode(temp_buf = bill_rec.Code);
								temp_buf = bill_rec.Code; // @v11.1.12 
								int    found = 0;
								for(uint j = first_idx_by_date; !found && j < i; j++) {
									const iSalesBillDebt * p_temp_item = outer_debt_list.at(j);
									if(p_temp_item->Code.CmpNC(temp_buf) == 0 && !processed_outer_list.search(p_temp_item->iSalesId, 0, 1)) {
										double payment = 0.0;
										P_BObj->P_Tbl->CalcPayment(bill_rec.ID, 1, 0, 0, &payment);
										iSalesBillDebt * p_new_item = current_debt_list.CreateNewItem();
										THROW_SL(p_new_item);
										*p_new_item = *p_temp_item;
										p_new_item->Code.Transf(CTRANSF_INNER_TO_UTF8);
										p_new_item->Debt = (payment >= p_new_item->Amount) ? 0.0 : (p_new_item->Amount - payment);
										processed_id_list.add(bill_rec.ID); // @v10.9.0
										processed_outer_list.add(p_temp_item->iSalesId); // @v10.9.1
										found = 1;
									}
								}
							}
						}
					}
					//
					first_idx_by_date = i;
				}
				if(i < _c)
					prev_date = p_outer_bill->Dtm.d;
			}
		}
		if(current_debt_list.getCount()) {
			ISALESPUTDEBTSETTLEMENT_PROC func = 0;
			THROW_SL(func = reinterpret_cast<ISALESPUTDEBTSETTLEMENT_PROC>(P_Lib->GetProcAddr("iSalesPutDebtSettlement")));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &current_debt_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			{
				long   err_item_count = 0;
				SString tech_buf;
				SString msg_buf;
				Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
				TSCollection <iSalesPepsi::ResultItem> result_list;
				ParseResultString(*p_result, result_list, &err_item_count); // @v9.5.1 &err_item_count
				/* @v9.5.1
				{
					for(i = 0; i < result_list.getCount(); i++) {
						const ResultItem * p_result_item = result_list.at(i);
						if(p_result_item && p_result_item->Status == 0)
							err_item_count++;
					}
				}
				*/
				PPFormatT(PPTXT_LOG_SUPPLIX_EXPDEBT_E, &msg_buf, tech_buf.cptr(), P.SupplID, err_item_count);
				PPWaitMsg(msg_buf);
				if(err_item_count)
					LogErrors(result_list, &msg_buf);
			}
			// @v9.3.1 DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
    }
    CATCHZOK
	return ok;
}

int iSalesPepsi::SendPrices()
{
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	StringSet ss_outer_cli_code;
	{
		TSCollection <iSalesRoutePacket> routs;
		THROW(ReceiveRouts(routs));
		for(uint i = 0; i < routs.getCount(); i++) {
			const iSalesRoutePacket * p_item = routs.at(i);
			if(p_item) {
				for(uint j = 0; j < p_item->VisitList.getCount(); j++) {
					const iSalesVisit * p_visit = p_item->VisitList.at(j);
					if(p_visit && p_visit->OuterClientCode.NotEmpty()) {
						(temp_buf = p_visit->InnerClientCode).Transf(CTRANSF_INNER_TO_UTF8); // @v9.9.9 @fix
						ss_outer_cli_code.add(temp_buf);
					}
				}
			}
		}
		ss_outer_cli_code.sortAndUndup();
	}
	if(ss_outer_cli_code.getCount()) {
		int    result = 0;
		int    do_send = 0;
        PPIDArray plq_list;
        if(Ep.Fb.StyloPalmID) {
			PPObjStyloPalm sp_obj;
			PPStyloPalmPacket sp_pack;
			if(sp_obj.GetPacket(Ep.Fb.StyloPalmID, &sp_pack) > 0 && sp_pack.QkList__.GetCount()) {
                for(uint i = 0; i < sp_pack.QkList__.GetCount(); i++) {
                    plq_list.addnz(sp_pack.QkList__.Get(i));
                }
			}
        }
        plq_list.add(0L);
        plq_list.sortAndUndup();
		TSCollection <iSalesPriceListPacket> pl_list;
		for(uint plq_idx = 0; plq_idx < plq_list.getCount(); plq_idx++) {
			const PPID plq_id = plq_list.get(plq_idx);
			int   is_pl_empty = 1;
			uint  new_pl_pos = 0;
			iSalesPriceListPacket * p_new_pl = pl_list.CreateNewItem(&new_pl_pos);
			THROW_SL(p_new_pl);
			if(plq_id == 0)
				p_new_pl->PriceListID = 1;
			else if(plq_id == 1)
				p_new_pl->PriceListID = 100;
			else
				p_new_pl->PriceListID = plq_id;
			p_new_pl->OuterCliCodeList = ss_outer_cli_code;
			GoodsFilt gfilt;
			if(Ep.GoodsGrpID)
				gfilt.GrpIDList.Add(Ep.GoodsGrpID);
            gfilt.Flags |= GoodsFilt::fShowArCode;
            gfilt.CodeArID = P.SupplID;
            Goods2Tbl::Rec goods_rec;
			for(GoodsIterator gi(&gfilt, 0, 0); gi.Next(&goods_rec) > 0;) {
				double price = 0.0;
				int32  ar_code_pack = 0;
				int    skip_goods = 1;
				if(GObj.P_Tbl->GetArCode(P.SupplID, goods_rec.ID, temp_buf, &ar_code_pack) > 0 && temp_buf.NotEmptyS()) {
					skip_goods = 0;
					if(State & stGoodsMappingInited) {
						const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(temp_buf);
						if(p_entry && p_entry->NativeCode.ToLong() == 0)
							skip_goods = 1;
					}
				}
				if(!skip_goods) {
					ReceiptTbl::Rec lot_rec;
					double lot_cost = 0.0;
					double lot_price = 0.0;
					if(P_BObj->trfr->Rcpt.GetLastLot(goods_rec.ID, 0, MAXDATE, &lot_rec) > 0) {
						lot_cost = lot_rec.Cost;
						lot_price = lot_rec.Price;
					}
					if(plq_id) {
						GObj.GetQuotExt(goods_rec.ID, QuotIdent(0, plq_id, 0, 0), lot_cost, lot_price, &price, 0);
					}
					else {
						if(Ep.PriceQuotID) {
							GObj.GetQuotExt(goods_rec.ID, QuotIdent(0, Ep.PriceQuotID, 0, 0), lot_cost, lot_price, &price, 0);
						}
						if(price == 0.0) {
							//GetLastLot(PPID goodsID, PPID locID, LDATE date, ReceiptTbl::Rec * pLotRec);
							price = lot_price;
						}
					}
					if(price > 0.0) {
						iSalesPriceItem * p_new_pl_item = p_new_pl->Prices.CreateNewItem();
						THROW_SL(p_new_pl_item);
                        p_new_pl_item->OuterCode = temp_buf;
                        p_new_pl_item->Price = price;
                        is_pl_empty = 0;
                        do_send = 1;
					}
				}
			}
			if(is_pl_empty)
				pl_list.atFree(new_pl_pos);
			else
				do_send = 1;
		}
		if(do_send) {
			ISALESPUTPRICES_PROC func = 0;
			THROW_SL(func = reinterpret_cast<ISALESPUTPRICES_PROC>(P_Lib->GetProcAddr("iSalesPutPrices")));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &pl_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			{
				long   err_item_count = 0;
				SString tech_buf;
				SString msg_buf;
				Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
				TSCollection <iSalesPepsi::ResultItem> result_list;
				ParseResultString(*p_result, result_list, &err_item_count); // @v9.5.1 &err_item_count
				PPFormatT(PPTXT_LOG_SUPPLIX_EXPPRICE_E, &msg_buf, tech_buf.cptr(), P.SupplID, err_item_count);
				PPWaitMsg(msg_buf);
				if(err_item_count)
					LogErrors(result_list, &msg_buf);
			}
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
    }
    CATCHZOK
	return ok;
}

int iSalesPepsi::SendStocks()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	PPViewGoodsRest gr_view;
	GoodsRestFilt gr_filt;
	GoodsRestViewItem gr_item;
	TSCollection <iSalesStockCountingWhPacket> outp_packet;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
    gr_filt.Date = ZERODATE;
    gr_filt.GoodsGrpID = Ep.GoodsGrpID;
	gr_filt.LocList = P.LocList;
    gr_filt.Flags |= GoodsRestFilt::fEachLocation;
    THROW(gr_view.Init_(&gr_filt));
    for(gr_view.InitIteration(PPViewGoodsRest::OrdByDefault); gr_view.NextIteration(&gr_item) > 0;) {
        const PPID loc_id = gr_item.LocID;
        uint   _pos = 0;
        int    skip_goods = 1;
		iSalesStockCountingWhPacket * p_loc_item = 0;
		if(GObj.P_Tbl->GetArCode(P.SupplID, gr_item.GoodsID, temp_buf.Z(), 0) > 0 && temp_buf.NotEmptyS()) {
			skip_goods = 0;
			if(State & stGoodsMappingInited) {
				const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(temp_buf);
				if(p_entry && p_entry->NativeCode.ToLong() == 0)
					skip_goods = 1;
			}
		}
		if(!skip_goods) {
			if(outp_packet.lsearch(&loc_id, &_pos, CMPF_LONG)) {
				p_loc_item = outp_packet.at(_pos);
			}
			else {
				LocationTbl::Rec loc_rec;
				if(LocObj.Search(loc_id, &loc_rec) > 0) {
					THROW_SL(p_loc_item = outp_packet.CreateNewItem());
					p_loc_item->WhID = loc_id;
					p_loc_item->WhCode = loc_rec.Code;
				}
			}
			if(p_loc_item) {
				/*
					struct iSalesStockCountingItem {
						SString OuterCode;
						int   Type; // Тип остатков: 0 - годные, 1 - брак, 2 - резерв
						int   UnitCode; // 0 - штука, 1 - коробка, 2 - условная коробка
						double Qtty; // Количество на остатке
					};
				*/
				iSalesStockCountingItem * p_new_item = p_loc_item->Items.CreateNewItem(); // new iSalesStockCountingItem;
				THROW_SL(p_new_item);
				p_new_item->OuterCode = temp_buf;
				p_new_item->Qtty = gr_item.Rest;
				p_new_item->Type = 0;
				p_new_item->UnitCode = NZOR(Ep.Fb.DefUnitID, 1004); // @v10.8.9 1004-->NZOR(Ep.Fb.DefUnitID, 1004)
			}
		}
    }
	{
		{
			SString check_file_name;
			SString line_buf;
			temp_buf.Z().Cat("isales").CatChar('-').Cat(P.SupplID).CatChar('-').Cat("stock");
			if(P.Flags & P.fTestMode)
				temp_buf.CatChar('-').Cat('t'); // ! don't correct the Cat('t') error - users are accustomed to it
			temp_buf.DotCat("csv");
			PPGetFilePath(PPPATH_OUT, temp_buf, check_file_name);
			SFile f_check(check_file_name, SFile::mWrite);
			//№ п/п	Наименование поля	Комментарии
			//1	Код дистрибьютора iSales	константа = 118
			//2	Дата и время формирования выгрузки	значение на момент формирования выгрузки(выполнение запроса)
			//3	Код склада дистрибьютора 1С	
			//4	Код продукта 1С	
			//5	Код продукта iSales	
			//6	Наименование продукта 1С	
			//7	Тип остатка	брак/не брак
			//8	Кол-во продукта на остатке в шуках	значение >0
			SString own_code;
			Goods2Tbl::Rec goods_rec;
			Ep.GetExtStrData(Ep.extssClientCode, own_code);
			for(uint i = 0; i < outp_packet.getCount(); i++) {
				const iSalesStockCountingWhPacket * p_loc_item = outp_packet.at(i);
				if(p_loc_item) {
					for(uint j = 0; j < p_loc_item->Items.getCount(); j++) {
						const iSalesStockCountingItem * p_item = p_loc_item->Items.at(j);
						if(p_item) {
							const iSalesGoodsPacket * p_gp = SearchGoodsMappingEntry(p_item->OuterCode);
							if(p_gp) {
								(temp_buf = own_code).Transf(CTRANSF_INNER_TO_OUTER);
								line_buf.Z().Cat(temp_buf).Tab().CatCurDateTime(DATF_GERMANCENT, TIMF_HMS).Tab().
									Cat(p_loc_item->WhID).Tab().Cat(p_gp->NativeCode).Tab().Cat(p_item->OuterCode).Tab();
								if(GObj.Fetch(p_gp->NativeCode.ToLong(), &goods_rec) > 0)
									temp_buf.Z().Cat(goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
								else
									temp_buf.Z().CatChar('#').Cat(p_gp->NativeCode);
								line_buf.Cat(temp_buf).Tab().Cat(0L).Tab().Cat(p_item->Qtty, MKSFMTD(0, 3, NMBF_DECCOMMA));
								f_check.WriteLine(line_buf.CR());
							}
						}
					}
				}
			}
		}
		if(!(P.Flags & P.fTestMode) && outp_packet.getCount()) { // @v10.8.7
			SString * p_result = 0;
			ISALESPUTSTOCKCOUNTING_PROC func = 0;
			THROW_SL(func = reinterpret_cast<ISALESPUTSTOCKCOUNTING_PROC>(P_Lib->GetProcAddr("iSalesPutStockCounting")));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &outp_packet);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
			{
				long   err_item_count = 0;
				TSCollection <iSalesPepsi::ResultItem> result_list;
				ParseResultString(*p_result, result_list, &err_item_count);
				{
					SString tech_buf;
					Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
					//PPTXT_LOG_SUPPLIX_EXPSTOCK_E   "Экспортированы остатки поставщику @zstr '@article'. Количество элементов с ошибками: @int"
					PPFormatT(PPTXT_LOG_SUPPLIX_EXPSTOCK_E, &msg_buf, tech_buf.cptr(), P.SupplID, err_item_count);
					PPWaitMsg(msg_buf);
					if(err_item_count)
						LogErrors(result_list, &msg_buf);
				}
			}
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
	}
    CATCHZOK
	return ok;
}

void iSalesPepsi::Helper_Make_iSalesIdent(const BillTbl::Rec & rRec, int outerDocType, SString & rIdent) const
{
	// @v11.1.12 rIdent.Z();
	// @v11.1.12 BillCore::GetCode(rIdent = rRec.Code);
	rIdent = rRec.Code; // @v11.1.12 
	rIdent.Space().Cat(rRec.Dt, DATF_GERMANCENT).Space().Cat(labs(outerDocType));
}

void iSalesPepsi::Helper_Parse_iSalesIdent(const SString & rIdent, SString & rCode, LDATE * pDate) const
{
	rCode.Z();
	LDATE   dt = ZERODATE;
	const char * p = rIdent;
	while(*p && *p != ' ') {
		rCode.CatChar(*p);
		p++;
	}
	if(*p == ' ') {
		SString temp_buf;
		dt = strtodate_(p, DATF_GERMAN);
		if(!checkdate(dt))
			dt = ZERODATE;
	}
	ASSIGN_PTR(pDate, dt);
}
//
// Если outerDocType < 0, то это - отмена документа
//
int iSalesPepsi::Helper_MakeBillEntry(PPID billID, PPBillPacket * pBp, int outerDocType, const PPIDArray * pRegisteredAgentList, TSCollection <iSalesBillPacket> & rList)
{
	int    ok = 1;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	int    do_cancel = BIN(outerDocType < 0);
	outerDocType = labs(outerDocType);
	PPBillPacket pack__;
	if(!pBp && P_BObj->ExtractPacket(billID, &pack__) > 0) {
		pBp = &pack__;
	}
	if(pBp) {
		StrAssocArray ti_pos_list;
		PPTransferItem ti;
		PPBillPacket::TiItemExt tiext;
		long   tiiterpos = 0;
		SString temp_buf;
		SString cancel_code;
		SString cli_addr_code; // Буфер для кода клиента (торговой точки)
		SString cli_face_code; // Буфер для кода клиента
		SString own_code; // Наш код в системе поставщика
		SysJournalTbl::Rec sj_rec;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		Ep.GetExtStrData(Ep.extssClientCode, own_code);
		if(do_cancel) {
			if(bill_ack_tag_id)
				pBp->BTagL.GetItemStr(bill_ack_tag_id, cancel_code);
			if(cancel_code.IsEmpty())
				do_cancel = 0;
		}
		else {
			for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
				tiiterpos++;
				if(GObj.BelongToGroup(ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, ti.GoodsID, temp_buf.Z(), 0) > 0) {
					int   skip_goods = 0;
					if(State & stGoodsMappingInited) {
						const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(temp_buf);
						if(p_entry && p_entry->NativeCode.ToLong() == 0)
							skip_goods = 1;
					}
					if(!skip_goods)
						ti_pos_list.Add(tiiterpos, temp_buf, 0);
				}
			}
		}
		if(do_cancel || ti_pos_list.getCount()) {
			int    skip = 0;
			int    is_own_order = 0; // !0 если документ выписан по заказу, импортированному из iSales
			PPBillPacket order_pack;
			cli_face_code.Z();
			cli_addr_code.Z();
			const  PPID ar_id = pBp->Rec.Object;
			const  PPID psn_id = ObjectToPerson(ar_id, 0);
			PPObjQuotKind qk_obj;
			PPID   special_qk_id = 0;
			PPID    _temp_qk_id = 0;
			// @v11.0.10 Базовая цена реализации, определяемая поставщиком и применяемая для расчетов производных цен реализации дистрибьютора.
			// Здесь она нам нужна для идентификации размера промо-скидки, о котором необходимо отчитаться перед поставщиком.
			// Используется в том случае, когда отгрузка сформирована НЕ по заказу из iSales. В случае привязки отгрузки к заказу iSales эта котировка не используется.
			const   PPID isales_support_discount_qk =  (qk_obj.SearchBySymb("ISALES-SUPPORT", &_temp_qk_id, 0) > 0) ? _temp_qk_id : 0;
			//
			BillTbl::Rec link_bill_rec;
			if(pBp->Rec.LinkBillID && P_BObj->Search(pBp->Rec.LinkBillID, &link_bill_rec) > 0) {
				;
			}
			else
				link_bill_rec.ID = 0;
			{
				PPObjTag tag_obj;
				PPObjectTag tag_rec;
				for(uint tagidx = 0; !special_qk_id && tagidx < pBp->BTagL.GetCount(); tagidx++) {
                    const ObjTagItem * p_tag_item = pBp->BTagL.GetItemByPos(tagidx);
                    if(p_tag_item && p_tag_item->TagDataType == OTTYP_OBJLINK) {
						if(tag_obj.Fetch(p_tag_item->TagID, &tag_rec) > 0 && tag_rec.TagEnumID == PPOBJ_QUOTKIND)
							special_qk_id = p_tag_item->Val.IntVal;
                    }
				}
			}
			if(oneof2(outerDocType, 1, 5))
				cli_face_code.CatChar('W');
			cli_face_code.Cat(psn_id);
			if(pBp->GetDlvrAddrID())
				cli_addr_code.Cat(pBp->GetDlvrAddrID());
			else if(link_bill_rec.ID) {
				PPFreight link_freight;
				if(P_BObj->P_Tbl->GetFreight(link_bill_rec.ID, &link_freight) > 0) {
					cli_addr_code.Cat(link_freight.DlvrAddrID);
				}
			}
			iSalesBillPacket * p_new_pack = rList.CreateNewItem();
			THROW_SL(p_new_pack);
			p_new_pack->NativeID = pBp->Rec.ID;
			{
				iSalesBillAmountEntry * p_amt_entry = 0;
				THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem());
				p_amt_entry->SetType = 0;
				THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem());
				p_amt_entry->SetType = 1;
				THROW_SL(p_amt_entry = p_new_pack->Amounts.CreateNewItem());
				p_amt_entry->SetType = 2;
			}
			p_new_pack->DocType = outerDocType;
			p_new_pack->ExtDocType = 0;
			p_new_pack->ExtCode.Z();
			p_new_pack->ExtDtm.Z();
			//
			// @v11.1.12 BillCore::GetCode(p_new_pack->Code = pBp->Rec.Code);
			p_new_pack->Code = pBp->Rec.Code; // @v11.1.12 
			p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
			p_new_pack->Dtm.Set(pBp->Rec.Dt, ZEROTIME);
			if(outerDocType == 6 && pBp->Rec.LinkBillID) {
				if(link_bill_rec.ID && GetOpType(link_bill_rec.OpID) == PPOPT_DRAFTRECEIPT) {
					// @v11.1.12 BillCore::GetCode(p_new_pack->Code = link_bill_rec.Code);
					p_new_pack->Code = link_bill_rec.Code; // @v11.1.12 
					p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
					p_new_pack->Dtm.Set(link_bill_rec.Dt, ZEROTIME);
				}
			}
			{
				if(cancel_code.NotEmpty()) {
					p_new_pack->iSalesId = cancel_code;
					p_new_pack->iSalesId.Transf(CTRANSF_INNER_TO_UTF8);
					LDATE   prev_date;
					Helper_Parse_iSalesIdent(cancel_code, temp_buf, &prev_date);
					if(temp_buf.NotEmptyS() && checkdate(prev_date)) {
						p_new_pack->Code = temp_buf;
						p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
						p_new_pack->Dtm.Set(prev_date, ZEROTIME);
					}
				}
				else {
					Helper_Make_iSalesIdent(pBp->Rec, outerDocType, p_new_pack->iSalesId);
					p_new_pack->iSalesId.Transf(CTRANSF_INNER_TO_UTF8);
				}
			}
			p_new_pack->IncDtm.Z();
			pBp->Pays.GetLast(&p_new_pack->DueDate, 0, 0);
			p_new_pack->Status = BIN(do_cancel);
			p_new_pack->Memo.Z();
			if(outerDocType == 6) { // Приход (подтверждение)
				p_new_pack->SellerCode.Z().Cat(psn_id);
				p_new_pack->ShipFrom.Z().Cat(psn_id);
				p_new_pack->PayerCode = own_code;
				p_new_pack->ShipTo = own_code;
				p_new_pack->DestLocCode.Cat(pBp->Rec.LocID);
				if(pBp->BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
					(p_new_pack->iSalesId = temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
				}
				{
					iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
					THROW_SL(p_new_ref);
					p_new_ref->DocType = 13;
					// @v11.1.12 BillCore::GetCode(p_new_ref->Code.Z().CatChar('O').Cat(p_new_pack->Code));
					p_new_ref->Code.Z().CatChar('O').Cat(p_new_pack->Code); // @v11.1.12 
					p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
					p_new_ref->Dtm.Z();
				}
			}
			else if(outerDocType == 5) { // Возврат или корректировка
				p_new_pack->SellerCode = cli_addr_code /*cli_face_code*/;
				p_new_pack->ShipFrom = cli_addr_code;
				p_new_pack->PayerCode = own_code;
				p_new_pack->ShipTo = own_code;
				p_new_pack->DestLocCode.Cat(pBp->Rec.LocID);
				//
				const int transmit_linkret_as_unlink = 0; // @v10.8.9 1-->0
				if(!transmit_linkret_as_unlink && oneof2(pBp->OpTypeID, PPOPT_GOODSRETURN, PPOPT_CORRECTION) && link_bill_rec.ID) {
					iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
					THROW_SL(p_new_ref);
					p_new_ref->DocType = 1;
					// @v11.1.12 BillCore::GetCode(p_new_ref->Code = link_bill_rec.Code);
					p_new_ref->Code = link_bill_rec.Code; // @v11.1.12 
					p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
					p_new_ref->Dtm.Set(link_bill_rec.Dt, ZEROTIME);
				}
				else {
					p_new_pack->DocType = 4; // Не привязанный возврат имеет отдельный тип
				}
			}
			else { // Продажа
				p_new_pack->SellerCode = own_code;
				p_new_pack->ShipFrom = own_code;
				// В некоторых случаях PayerCode - код персоналии, в некоторых - адреса доставки. Тут нужна настройка в конфигурации. Пока просто фиксированное значение.
				p_new_pack->PayerCode = cli_addr_code /*cli_face_code*/;
				p_new_pack->ShipTo = cli_addr_code;
				p_new_pack->SrcLocCode.Cat(pBp->Rec.LocID);
				//
				PPIDArray order_id_list;
				pBp->GetOrderList(order_id_list);
				for(uint ordidx = 0; ordidx < order_id_list.getCount(); ordidx++) {
					const PPID ord_id = order_id_list.get(ordidx);
					BillTbl::Rec ord_rec;
					if(ord_id && P_BObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
						if(PPRef->Ot.GetTagStr(PPOBJ_BILL, ord_id, PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("ISALES-PEPSI")) {
							THROW(P_BObj->ExtractPacket(ord_id, &order_pack) > 0);
							is_own_order = 1;
							{
								iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
								THROW_SL(p_new_ref);
								p_new_ref->DocType = 13;
								// @v11.1.12 BillCore::GetCode(p_new_ref->Code = ord_rec.Code);
								p_new_ref->Code = ord_rec.Code; // @v11.1.12 
								p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
								p_new_ref->Dtm.Z();
							}
						}
					}
				}
			}
			if(pBp->Ext.AgentID) {
				PPID agent_psn_id = ObjectToPerson(pBp->Ext.AgentID, 0);
				// @v10.8.11 {
				if(UnknAgentID && pRegisteredAgentList && !pRegisteredAgentList->lsearch(agent_psn_id))
					agent_psn_id = UnknAgentID;
				// } @v10.8.11 
				if(agent_psn_id) {
					p_new_pack->AgentCode.Cat(agent_psn_id);
				}
			}
			if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, pBp->Rec.ID, &sj_rec) > 0)
				p_new_pack->CreationDtm.Set(sj_rec.Dt, sj_rec.Tm);
			else
				p_new_pack->CreationDtm.Z();
			{
				int    is_creation = 0;
				LDATETIME moment;
				if(p_sj && p_sj->GetLastObjModifEvent(PPOBJ_BILL, pBp->Rec.ID, &moment, &is_creation, &sj_rec) > 0 && !is_creation)
					p_new_pack->LastUpdDtm = moment;
				else
					p_new_pack->LastUpdDtm.Z();
			}
			long   tiiterpos = 0;
			for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
				tiiterpos++;
				uint   pos_list_item_pos = 0;
				if(ti_pos_list.GetText(tiiterpos, temp_buf) > 0) {
					Goods2Tbl::Rec goods_rec;
					if(GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
						const iSalesGoodsPacket * p_goods_entry = SearchGoodsMappingEntry(temp_buf);
						const  double org_net_price = ti.NetPrice();
						double ord_part_dis = 0.0;
						double net_price = org_net_price;
						double special_net_price = 0.0;
						if(feqeps(net_price, 0.0, 1E-2)) {
							ord_part_dis = 1.0;
							net_price = ti.Price;
						}
						else {
							if(outerDocType != 6) {
								if(special_qk_id) {
									double quot = 0.0;
									if(GObj.GetQuotExt(ti.GoodsID, QuotIdent(ti.LocID, special_qk_id, 0, ar_id), &quot, 0) > 0 && quot > 0.0)
										special_net_price = quot;
								}
								if(Ep.PriceQuotID && special_net_price == 0.0) {
									double quot = 0.0;
									if(GObj.GetQuotExt(ti.GoodsID, QuotIdent(ti.LocID, Ep.PriceQuotID, 0, ar_id), &quot, 0) > 0 && quot > 0.0)
										net_price = quot;
								}
							}
							if(is_own_order) {
								double ord_price = 0.0;
								double ord_dis = 0.0;
								for(uint ordidx = 0; order_pack.SearchGoods(ti.GoodsID, &ordidx); ordidx++) {
									const PPTransferItem & r_ord_ti = order_pack.ConstTI(ordidx);
									int   my = 0;
									if(ti.OrdLotID && ti.OrdLotID == r_ord_ti.LotID)
										my = 1;
									else if(tiext.MergePosList.getCount()) {
										for(uint k = 0; k < tiext.MergePosList.getCount(); k++) {
											const uint pos_local = tiext.MergePosList.get(k);
											if(pos_local < pBp->GetTCount()) {
												const PPTransferItem & r_ti_local = pBp->TI(pos_local);
												if(r_ti_local.OrdLotID && r_ti_local.OrdLotID == r_ord_ti.LotID)
													my = 1;
											}
										}
									}
									if(my) {
										const double ord_qtty = fabs(r_ord_ti.Quantity_);
										ord_price += fabs(r_ord_ti.Price) * ord_qtty;
										ord_dis += r_ord_ti.Discount * ord_qtty;
									}
								}
								if(ord_dis != 0.0 && ord_price != 0.0)
									ord_part_dis = R6(ord_dis / ord_price);
							}
							// @v11.0.10 {
							else if(isales_support_discount_qk) {
								// Специальный случай: отгрузка выписана не по заказу iSales но к ней применяется промо-скидка.
								// Размер скидки определяется разницей между опорной ценой реализации (isales_support_discount_qk) и
								// полной ценой продажи, если последняя меньше опорной цены.
								// Если мы встречаем описанные условия, то имитируем относительную скидку по заказу ord_part_dis
								double quot = 0.0;
								if(GObj.GetQuotExt(ti.GoodsID, QuotIdent(ti.LocID, isales_support_discount_qk, 0, ar_id), &quot, 0) > 0 && quot > 0.0 && org_net_price < quot)
									ord_part_dis = R6((quot - org_net_price) / quot);
							}
							// } @v11.0.10
						}
						iSalesBillItem * p_new_item = p_new_pack->Items.CreateNewItem();
						THROW_SL(p_new_item);
						// @v10.8.9 const double qtty = fabs(ti.Quantity_);
						const double qtty = fabs(ti.GetEffCorrectionExpQtty()); // @v10.8.9
						assert(!ti.IsCorrectionExp() || outerDocType == 5);
						p_new_item->LineN = tiiterpos; // ti.RByBill;
						p_new_item->OuterGoodsCode = temp_buf; // ti_pos_item.Txt;
						p_new_item->NativeGoodsCode.Z().Cat(ti.GoodsID);
						{
							p_new_item->Country.Z();
							/*if(p_goods_entry && p_goods_entry->CountryName.NotEmpty()) {
								(p_new_item->Country = p_goods_entry->CountryName).Transf(CTRANSF_INNER_TO_UTF8);
							}
							else*/ {
								PersonTbl::Rec psn_rec;
								if(goods_rec.ManufID && PsnObj.Search(goods_rec.ManufID, &psn_rec) > 0 && psn_rec.Status == PPPRS_COUNTRY) {
									if(PsnObj.GetRegNumber(goods_rec.ManufID, PPREGT_COUNTRYABBR, ZERODATE, temp_buf) > 0)
										(p_new_item->Country = temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
								}
							}
							if(p_new_item->Country.IsEmpty())
								(p_new_item->Country = "RU").Transf(CTRANSF_INNER_TO_UTF8);
						}
						p_new_item->CLB.Z();
						p_new_item->UnitCode = NZOR(Ep.Fb.DefUnitID, 1004); // @v10.8.9 1004-->NZOR(Ep.Fb.DefUnitID, 1004)
						p_new_item->Qtty = qtty;
						p_new_item->Memo.Z();
						{
							double vat_sum_in_nominal_price = 0.0;
							double vat_sum_in_full_price = 0.0;
							double vat_sum_in_discount = 0.0;
							double full_price = 0.0;
							double nominal_price = 0.0;
							if(outerDocType == 6) {
								full_price = ti.Cost;
								nominal_price = ti.Cost;
							}
							else if(special_net_price > 0.0) {
								full_price = (ord_part_dis >= 1.0) ? 0.0 : (special_net_price * (1.0 - ord_part_dis));
								nominal_price = special_net_price;
							}
							else {
								full_price = ((ord_part_dis >= 1.0) ? 0.0 : net_price);
								nominal_price = ((ord_part_dis >= 1.0) ? net_price : (net_price / (1.0 - ord_part_dis)));
							}
							const double discount = nominal_price - full_price;
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pBp->Rec.Dt, 1.0, full_price,    &vat_sum_in_full_price, 0, 0, 16);
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pBp->Rec.Dt, 1.0, nominal_price, &vat_sum_in_nominal_price, 0, 0, 16);
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pBp->Rec.Dt, 1.0, discount,      &vat_sum_in_discount, 0, 0, 16);
							{
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem();
								THROW_SL(p_amt_entry);
								p_amt_entry->SetType    = 0;
								p_amt_entry->NetPrice   = nominal_price - vat_sum_in_nominal_price;
								p_amt_entry->GrossPrice = nominal_price;
								p_amt_entry->NetSum     = p_amt_entry->NetPrice * qtty;
								p_amt_entry->DiscAmount = 0.0;
								p_amt_entry->DiscNetSum = (nominal_price - vat_sum_in_nominal_price) * qtty;
								p_amt_entry->VatSum     = vat_sum_in_nominal_price * qtty;
								p_amt_entry->GrossSum   = nominal_price * qtty;
								p_amt_entry->DiscGrossSum = 0.0;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(0);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
							{
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem();
								THROW_SL(p_amt_entry);
								*p_amt_entry = *p_new_item->Amounts.at(0);
								p_amt_entry->SetType = 1;
								p_amt_entry->NetPrice   = nominal_price - vat_sum_in_nominal_price;
								p_amt_entry->GrossPrice = nominal_price;
								p_amt_entry->NetSum     = p_amt_entry->NetPrice * qtty;
								p_amt_entry->DiscAmount = (discount - vat_sum_in_discount) * qtty;
								p_amt_entry->DiscNetSum = (p_amt_entry->NetSum - p_amt_entry->DiscAmount);
								p_amt_entry->VatSum     = vat_sum_in_full_price * qtty;
								p_amt_entry->GrossSum   = full_price * qtty;
								p_amt_entry->DiscGrossSum = discount * qtty;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(1);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
							{
								iSalesBillAmountEntry * p_amt_entry = p_new_item->Amounts.CreateNewItem();
								THROW_SL(p_amt_entry);
								*p_amt_entry = *p_new_item->Amounts.at(1);
								p_amt_entry->SetType = 2;
								//
								iSalesBillAmountEntry * p_billamt_entry = p_new_pack->Amounts.at(2);
								p_billamt_entry->NetSum += p_amt_entry->NetSum;
								p_billamt_entry->GrossSum += p_amt_entry->GrossSum;
								p_billamt_entry->VatSum += p_amt_entry->VatSum;
								p_billamt_entry->DiscAmount += p_amt_entry->DiscAmount;
								p_billamt_entry->DiscGrossSum += p_amt_entry->DiscGrossSum;
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int iSalesPepsi::Helper_MakeBillList(PPID opID, int outerDocType, const PPIDArray * pRegisteredAgentList, TSCollection <iSalesBillPacket> & rList)
{
	int    ok = -1;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	if(opID && outerDocType >= 0) {
		Reference * p_ref = PPRef;
		SString temp_buf;
		SString isales_code;
		S_GUID test_uuid;
		PPViewBill b_view;
		BillTbl::Rec bill_rec;
		PPIDArray force_bill_list; // Список документов которые надо послать снова
		BillFilt b_filt;
		BillViewItem view_item;
		PPBillPacket pack;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		b_filt.OpID = opID;
		b_filt.LocList = P.LocList;
		if(outerDocType == 6) {
			b_filt.ObjectID = P.SupplID;
		}
		b_filt.Period = P.ExpPeriod;
		SETIFZ(b_filt.Period.low, encodedate(1, 1, 2020)); // @v10.8.8 2016-->2020
		{
			PPIDArray upd_bill_list;
			PPIDArray rmvd_bill_list; // @v10.9.2 Список документов, которые были удалены
			if(!(P.Flags & P.fTestMode)) { // @v10.9.1
				SString org_isales_code;
				LDATETIME since;
				since.Set(b_filt.Period.low, ZEROTIME);
				{
					PPIDArray acn_list;
					acn_list.add(PPACN_UPDBILL);
					p_sj->GetObjListByEventPeriod(PPOBJ_BILL, 0, &acn_list, &b_filt.Period, upd_bill_list);
				}
				{
					//
					// Отправка информации об отмене документов-зомби (удаленных)
					//
					ObjVersioningCore * p_ovc = PPRef->P_OvT;
					if(p_ovc && p_ovc->InitSerializeContext(1)) {
						SSerializeContext & r_sctx = p_ovc->GetSCtx();
						SBuffer ov_buf;
						TSVector <SysJournalTbl::Rec> sj_list;
						p_sj->GetObjRemovingEventListByPeriod(PPOBJ_BILL, 0, &b_filt.Period, sj_list);
						for(uint rblidx = 0; rblidx < sj_list.getCount(); rblidx++) {
							SysJournalTbl::Rec & r_ev = sj_list.at(rblidx);						
							long   vv = 0;
							PPObjID oid;
							ov_buf.Z();
							if(p_ovc->Search(r_ev.Extra, &oid, &vv, &ov_buf) > 0 && oid.IsEq(r_ev.ObjType, r_ev.ObjID)) {
								if(P_BObj->SerializePacket__(-1, &pack, ov_buf, &r_sctx)) {
									pack.ProcessFlags |= (PPBillPacket::pfZombie|PPBillPacket::pfUpdateProhibited);
									if(pack.BTagL.GetItemStr(bill_ack_tag_id, org_isales_code) > 0 && !test_uuid.FromStr(org_isales_code)) {
										if(IsOpBelongTo(pack.Rec.OpID, b_filt.OpID) && b_filt.LocList.CheckID(pack.Rec.LocID))
											Helper_MakeBillEntry(0, &pack, -outerDocType, pRegisteredAgentList, rList);
									}
								}
							}
						}
					}
				}
				upd_bill_list.sortAndUndup();
				for(uint i = 0; i < upd_bill_list.getCount(); i++) {
					const PPID upd_bill_id = upd_bill_list.get(i);
					//
					// Теги PPTAG_BILL_EDIACK используются так же для обмена с ЕГАИС.
					// Дабы отличить документ isales от ЕГАИС попытаемся преобразовать значение
					// тега в GUID. Если не получилось - значит не ЕГАИС.
					// Метод очень плохой, но пока оставим так.
					// @v9.5.7 В конфигурацию обмена данными добавлен спец тег для этого.
					//
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, upd_bill_id, bill_ack_tag_id, org_isales_code) > 0 && !test_uuid.FromStr(org_isales_code)) {
						if(P_BObj->Search(upd_bill_id, &bill_rec) > 0 && IsOpBelongTo(bill_rec.OpID, b_filt.OpID) && b_filt.LocList.CheckID(bill_rec.LocID)) {
							if(P_BObj->ExtractPacket(upd_bill_id, &pack) > 0) {
								Helper_Make_iSalesIdent(bill_rec, outerDocType, isales_code);
								if(pack.GetTCount() == 0) {
									Helper_MakeBillEntry(upd_bill_id, &pack, -outerDocType, pRegisteredAgentList, rList);
									force_bill_list.add(upd_bill_id);
								}
								else if(pack.Rec.Flags2 & BILLF2_DECLINED) {
									Helper_MakeBillEntry(upd_bill_id, &pack, -outerDocType, pRegisteredAgentList, rList);
									force_bill_list.add(upd_bill_id);
								}
								else {
									int    is_my_goods = 0;
									long   tiiterpos = 0;
									for(uint tiidx = 0; !is_my_goods && tiidx < pack.GetTCount(); tiidx++) {
										const PPID goods_id = labs(pack.ConstTI(tiidx).GoodsID);
										if(GObj.BelongToGroup(goods_id, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, goods_id, temp_buf.Z(), 0) > 0) {
											int   skip_goods = 0;
											if(State & stGoodsMappingInited) {
												const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(temp_buf);
												if(p_entry && p_entry->NativeCode.ToLong() == 0)
													skip_goods = 1;
											}
											if(!skip_goods)
												is_my_goods = 1;
										}
									}
									if(is_my_goods) {
										if(isales_code != org_isales_code) { 
											Helper_MakeBillEntry(upd_bill_id, &pack, -outerDocType, pRegisteredAgentList, rList);
										}
										force_bill_list.add(upd_bill_id);
									}
								}
							}
						}
					}
				}
			}
		}
		force_bill_list.sortAndUndup();
		THROW(b_view.Init_(&b_filt));
		for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
			if(!force_bill_list.bsearch(view_item.ID)) { 
				int    dont_send = 0;
				if(outerDocType == 6 && !P_BObj->CheckStatusFlag(view_item.StatusID, BILSTF_READYFOREDIACK))
					dont_send = 1; // Статус не допускает отправку
				else {
					if(!(P.Flags & P.fTestMode)) { // @v10.9.0 При подготовке файла сверки нельзя пропускать документы, которые уже были отправлены
						if(p_ref->Ot.GetTagStr(PPOBJ_BILL, view_item.ID, bill_ack_tag_id, temp_buf) > 0 && !test_uuid.FromStr(temp_buf))
							dont_send = 1; // не отправляем документы, которые уже были отправлены ранее
					}
				}
				if(!dont_send)
					Helper_MakeBillEntry(view_item.ID, 0, outerDocType, pRegisteredAgentList, rList);
			}
		}
		{
			for(uint i = 0; i < force_bill_list.getCount(); i++) {
				const PPID force_bill_id = force_bill_list.get(i);
				if(P_BObj->Search(force_bill_id, &bill_rec) > 0) {
					if(outerDocType == 6 && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
						// Статус не позволяет отправку
					}
					else {
						Helper_MakeBillEntry(bill_rec.ID, 0, outerDocType, pRegisteredAgentList, rList);
					}
				}
			}
		}
	}
    CATCHZOK
	return ok;
}

int iSalesPepsi::SendInvoices()
{
	/*
		Возврат обратной реализацией (4) – это документ, без привязки к расходной накладной, и тут также не нужно передавать теги REFS (<DOC_TP> = 4)
		Возврат по акту (5) – это документ, c привязкой к расходной накладной, в тегах REFS в этом случае должны быть данные с номером расходной накладной, датой накладной, и тип документа расх. накладная = 1 (<DOC_TP> = 5)
	*/
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	PPSoapClientSession sess;
	PPObjOprKind op_obj; // @v10.8.9
	SString temp_buf;
	SString msg_buf;
	PPIDArray registered_agent_list; // @v10.8.11
	TSCollection <iSalesBillPacket> outp_packet;
	//BillExportPeriod.Set(encodedate(1, 6, 2016), encodedate(30, 6, 2016));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	// @v10.8.11 {
	{
		TSCollection <iSalesRoutePacket> routs; 
		THROW(ReceiveRouts(routs));
		for(uint i = 0; i < routs.getCount(); i++) {
			const iSalesRoutePacket * p_rout = routs.at(i);
			if(p_rout) {
				const PPID native_agent_id = p_rout->NativeAgentCode.ToLong();
				if(native_agent_id)
					registered_agent_list.add(native_agent_id);
			}
		}
		registered_agent_list.sortAndUndup();
	}
	// } @v10.8.11 
	THROW(Helper_MakeBillList(Ep.ExpendOp, 1, &registered_agent_list, outp_packet));
	// @v10.8.9 {
	{
		PPIDArray correction_op_list;
		{
			PPIDArray local_op_list;
			PPObjOprKind::ExpandOp(Ep.ExpendOp, local_op_list);
			for(uint i = 0; i < local_op_list.getCount(); i++) {
				op_obj.GetCorrectionOpList(local_op_list.get(i), &correction_op_list);
			}
		}
		if(correction_op_list.getCount()) {
			correction_op_list.sortAndUndup();
			for(uint i = 0; i < correction_op_list.getCount(); i++) {
				const PPID op_id = correction_op_list.get(i);
				THROW(Helper_MakeBillList(op_id, 5, &registered_agent_list, outp_packet));
			}
		}
	}
	// } @v10.8.9 
	THROW(Helper_MakeBillList(Ep.RetOp, 5, &registered_agent_list, outp_packet));
    {
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.EdiDesadvOpID && GetOpData(acfg.Hdr.EdiDesadvOpID, &op_rec) > 0) {
			if(op_rec.OpTypeID == PPOPT_DRAFTRECEIPT) {
				PPObjOprKind op_obj;
				PPDraftOpEx doe;
				if(op_obj.GetDraftExData(op_rec.ID, &doe) > 0 && doe.WrOffOpID) {
					THROW(Helper_MakeBillList(doe.WrOffOpID, 6, &registered_agent_list, outp_packet));
				}
			}
            else if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
				THROW(Helper_MakeBillList(acfg.Hdr.EdiDesadvOpID, 6, &registered_agent_list, outp_packet));
            }
		}
    }
	if(LogFileName.NotEmpty()) {
		SString dump_file_name;
		SPathStruc ps(LogFileName);
		ps.Nam.CatChar('-').Cat("dump").CatChar('-').Cat("invoices");
		ps.Merge(dump_file_name);
		SFile f_out_log(dump_file_name, SFile::mWrite);
		if(f_out_log.IsValid()) {
			f_out_log.WriteLine(0);
			for(uint i = 0; i < outp_packet.getCount(); i++) {
				const iSalesBillPacket * p_pack = outp_packet.at(i);
				if(p_pack) {
					msg_buf.Z().
						CatEq("NativeID", p_pack->NativeID).Semicol().
						CatEq("iSalesId", p_pack->iSalesId).Semicol().
						CatEq("DocType",  static_cast<long>(p_pack->DocType)).Semicol().
						CatEq("ExtDocType", static_cast<long>(p_pack->ExtDocType)).Semicol().
						CatEq("Status", static_cast<long>(p_pack->Status)).Semicol().
						CatEq("Code", p_pack->Code).Semicol().
						CatEq("ExtCode", p_pack->ExtCode).Semicol().
						CatEq("Dtm", p_pack->Dtm.d, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("IncDtm", p_pack->IncDtm.d, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("ExtDtm", p_pack->ExtDtm.d, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("CreationDtm", p_pack->CreationDtm.d, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("LastUpdDtm", p_pack->LastUpdDtm.d, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("DueDate", p_pack->DueDate, DATF_DMY|DATF_CENTURY).Semicol().
						CatEq("ShipFrom", p_pack->ShipFrom).Semicol().
						CatEq("ShipTo", p_pack->ShipTo).Semicol().
						CatEq("SellerCode", p_pack->SellerCode).Semicol().
						CatEq("PayerCode", p_pack->PayerCode).Semicol().
						CatEq("Memo", p_pack->Memo).Semicol().
						CatEq("SrcLocCode", p_pack->SrcLocCode).Semicol().
						CatEq("DestLocCode", p_pack->DestLocCode).Semicol().
						CatEq("AgentCode", p_pack->AgentCode).Semicol().
						CatEq("AuthId", p_pack->AuthId).Semicol().
						CatEq("EditId", p_pack->EditId).Semicol().
						CatEq("ErrMsg", p_pack->ErrMsg).Semicol();
					//PPLogMessage(LogFileName, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					f_out_log.WriteLine(msg_buf.CR());
				}
			}
		}
	}
	{
		/*
			№ п/п	Наименование поля	Комментарии
			1	Код дистрибьютора iSales	константа = id dealer
			2	Дата и время формирования выгрузки	значение на момент формирования выгрузки (выполнение запроса)
			3	Тип документа	возможные значения: 1- расходная накладная, 5 - возврат, 4 – приход
			4	Номер документа в 1С	
			5	Дата документа 1С	
			6	Номер заказа iSales	заполняется для расходных накладных
			7	Код контрагента в 1С	не заполняется для приходного документа
			8	Код торговой точки 1С	не заполняется для приходного документа
			9	Наименование клиента 1С	не заполняется для приходного документа
			10	Код агента 1С	не заполняется для приходного документа
			11	ФИО агента 1С	не заполняется для приходного документа
			12	Код продукта 1С	
			13	Код продукта iSales	может быть пустым, если продукт не замапплен
			14	Наименование продукта 1С	
			15	Кол-во продукта по документу в штуках	
			16	Сумма в руб с НДС по продукту в документе	
			17	Сумма в руб без НДС по продукту в документе	
			18	Процент скидки по продукту в документе	процент скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
			19	Сумма скидки в руб с НДС по продукту в документе	сумма скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
			20	Сумма долга в руб c НДС по документу	неоплаченная сумма по документу в целом, значение дублируется для каждой строки(продукта) документа; поле не заполняется для приходного документа
			21	Номер заказа SAP для приходного документа	номер заказа (SAP#) приходного документа, полученного из iSales, заполняется только для приходных документов
			22	Номер расходного документа SAP (iDoc) для приходного документа	номер SAP для приходного документа, полученного из iSales, заполняется только для приходных документов
			23	Код поставщика 1С	заполняется только для приходных документов
			24	Наименование поставщика 1С	заполняется только для приходных документов
		*/
		SString check_file_name;
		SString line_buf;
		temp_buf.Z().Cat("isales").CatChar('-').Cat(P.SupplID).CatChar('-').Cat("invoices");
		if(P.Flags & P.fTestMode)
			temp_buf.CatChar('-').Cat('t'); // ! don't correct the Cat('t') error - users are accustomed to it
		temp_buf.DotCat("csv");
		PPGetFilePath(PPPATH_OUT, temp_buf, check_file_name);
		SFile f_check(check_file_name, SFile::mWrite);
		SString own_code;
		Ep.GetExtStrData(Ep.extssClientCode, own_code);
		for(uint i = 0; i < outp_packet.getCount(); i++) {
			const iSalesBillPacket * p_bill_item = outp_packet.at(i);
			if(p_bill_item) {
				// @v10.8.12 {
				double debt_amount = 0.0;
				{
					BillTbl::Rec bill_rec;
					if(P_BObj->Search(p_bill_item->NativeID, &bill_rec) > 0) {
						double paym_amount = 0.0;
						P_BObj->P_Tbl->CalcPayment(p_bill_item->NativeID, 0, 0, 0/*curID*/, &paym_amount);
						debt_amount = bill_rec.Amount - paym_amount;
					}
				}
				// } @v10.8.12 
				for(uint j = 0; j < p_bill_item->Items.getCount(); j++) {
					const iSalesBillItem * p_item = p_bill_item->Items.at(j);
					if(p_item) {
						//const iSalesGoodsPacket * p_gp = SearchGoodsMappingEntry(p_item->OuterCode);
						line_buf.Z();
						line_buf.Cat((temp_buf = own_code).Transf(CTRANSF_INNER_TO_OUTER)).Tab();
						line_buf.CatCurDateTime(DATF_GERMANCENT, TIMF_HMS).Tab();
						line_buf.Cat(p_bill_item->DocType).Tab();
						line_buf.Cat((temp_buf = p_bill_item->Code).Transf(CTRANSF_UTF8_TO_OUTER)).Tab();
						line_buf.Cat(p_bill_item->Dtm.d, DATF_GERMANCENT).Tab();
						{
							for(uint ri = 0; ri < p_bill_item->Refs.getCount(); ri++) {
								const iSalesBillRef * p_ref = p_bill_item->Refs.at(ri);
								if(p_ref && p_ref->DocType == 13) {
									(temp_buf = p_ref->Code).Transf(CTRANSF_UTF8_TO_OUTER);
									line_buf.Cat(temp_buf);
									break;
								}
							}
							line_buf.Tab();
						}
						{
							LocationTbl::Rec loc_rec;
							if(p_bill_item->DocType == 1) {
								line_buf.Cat(p_bill_item->PayerCode).Tab(); 
								line_buf.Cat(p_bill_item->ShipTo).Tab();
								const PPID addr_id = p_bill_item->ShipTo.ToLong();
								if(LocObj.Search(addr_id, &loc_rec) > 0 && loc_rec.OwnerID)
									GetPersonName(loc_rec.OwnerID, temp_buf);
								else
									temp_buf.Z();
								line_buf.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).Tab();
							}
							else if(p_bill_item->DocType == 5) {
								line_buf.Cat(p_bill_item->SellerCode).Tab(); 
								line_buf.Cat(p_bill_item->ShipFrom).Tab();
								const PPID addr_id = p_bill_item->ShipFrom.ToLong();
								if(LocObj.Search(addr_id, &loc_rec) > 0 && loc_rec.OwnerID)
									GetPersonName(loc_rec.OwnerID, temp_buf);
								else
									temp_buf.Z();
								line_buf.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).Tab();
							}
							else {
								line_buf.Tab(3);
							}
						}
						line_buf.Cat(p_bill_item->AgentCode).Tab(); // 10	Код агента 1С	не заполняется для приходного документа
						{
							const PPID agent_id = p_bill_item->AgentCode.ToLong();
							if(agent_id)
								GetPersonName(agent_id, temp_buf);
							else
								temp_buf.Z();
							line_buf.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).Tab(); // 11	ФИО агента 1С	не заполняется для приходного документа
						}
						line_buf.Cat(p_item->NativeGoodsCode).Tab();
						line_buf.Cat(p_item->OuterGoodsCode).Tab();
						{
							Goods2Tbl::Rec goods_rec;
							if(GObj.Fetch(p_item->NativeGoodsCode.ToLong(), &goods_rec) > 0)
								temp_buf.Z().Cat(goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
							else
								temp_buf.Z().CatChar('#').Cat(p_item->NativeGoodsCode);
							line_buf.Cat(temp_buf).Tab();
						}
						line_buf.Cat(p_item->Qtty, MKSFMTD(0, 3, NMBF_DECCOMMA)).Tab();
						{
							iSalesBillAmountEntry * p_main_amt_entry = 0; // @v11.1.4
							iSalesBillAmountEntry * p_discount_amt_entry = 0; // @v11.1.4
							for(uint si = 0; si < p_item->Amounts.getCount(); si++) {
								iSalesBillAmountEntry * p_amt_entry = p_item->Amounts.at(si);
								if(p_amt_entry) {
									if(!p_main_amt_entry && p_amt_entry->SetType == 0) {
										p_main_amt_entry = p_amt_entry;
									}
									if(!p_discount_amt_entry && p_amt_entry->SetType == 1) {
										p_discount_amt_entry = p_amt_entry;
									}
								}

							}
							double gross_sum = 0.0;
							double net_sum = 0.0;
							double discount_pct = 0.0;
							double discount_amt = 0.0;
							if(p_discount_amt_entry) { // @v11.1.4
								gross_sum = p_discount_amt_entry->GrossSum;
								net_sum = p_discount_amt_entry->DiscNetSum;
								discount_pct = fdivnz(p_discount_amt_entry->DiscGrossSum, p_discount_amt_entry->GrossSum) * 100.0;
								discount_amt = p_discount_amt_entry->DiscGrossSum;
							}
							else if(p_main_amt_entry) {
								gross_sum = p_main_amt_entry->GrossSum;
								net_sum = p_main_amt_entry->NetSum;
							}
							line_buf.Cat(gross_sum, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab();    // 16 Сумма в руб с НДС по продукту в документе	
							line_buf.Cat(net_sum,   MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab();    // 17 Сумма в руб без НДС по продукту в документе	
							line_buf.Cat(discount_pct, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 18 Процент скидки по продукту в документе	процент скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
							line_buf.Cat(discount_amt, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 19 Сумма скидки в руб с НДС по продукту в документе	сумма скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
							/*if(p_main_amt_entry) {
								line_buf.Cat(p_main_amt_entry->GrossSum, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 16	Сумма в руб с НДС по продукту в документе	
								line_buf.Cat(p_main_amt_entry->NetSum, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab();   // 17	Сумма в руб без НДС по продукту в документе	
							}
							else {
								line_buf.Cat(0.0, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 16	Сумма в руб с НДС по продукту в документе	
								line_buf.Cat(0.0, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 17	Сумма в руб без НДС по продукту в документе	
							}
							if(p_discount_amt_entry) { // @v11.1.4
								const double _discount_pct = fdivnz(p_discount_amt_entry->DiscGrossSum, p_discount_amt_entry->GrossSum) * 100.0;
								line_buf.Cat(_discount_pct, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 18 Процент скидки по продукту в документе	процент скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
								line_buf.Cat(p_discount_amt_entry->DiscGrossSum, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 19 Сумма скидки в руб с НДС по продукту в документе	сумма скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
							}
							else {
								line_buf.Cat(0.0, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 18 Процент скидки по продукту в документе	процент скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
								line_buf.Cat(0.0, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 19 Сумма скидки в руб с НДС по продукту в документе	сумма скидки по промо-акциям по продукту до применения алгоритма размазывания скидки по документу, не заполняется для приходного документа
							}*/
							line_buf.Cat(debt_amount, MKSFMTD(0, 2, NMBF_DECCOMMA)).Tab(); // 20 Сумма долга в руб c НДС по документу	неоплаченная сумма по документу в целом, значение дублируется для каждой строки(продукта) документа; поле не заполняется для приходного документа
						}
						{
							line_buf.Tab(); // 21	Номер заказа SAP для приходного документа	номер заказа (SAP#) приходного документа, полученного из iSales, заполняется только для приходных документов
							line_buf.Tab(); // 22	Номер расходного документа SAP (iDoc) для приходного документа	номер SAP для приходного документа, полученного из iSales, заполняется только для приходных документов
							line_buf.Tab(); // 23	Код поставщика 1С	заполняется только для приходных документов
							line_buf.Tab(); // 24	Наименование поставщика 1С	заполняется только для приходных документов
						}
						f_check.WriteLine(line_buf.CR());
					}
				}
			}
		}
	}
	if(!(P.Flags & P.fTestMode) && outp_packet.getCount()) {
		SString tech_buf;
		Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
		{
			PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_S, &msg_buf, tech_buf.cptr(), P.SupplID);
			//R_Logger.Log(msg_buf);
			PPWaitMsg(msg_buf);
		}
		const  uint max_fraction_size = 10; // @v10.0.02 20-->10
		long   total_error_count = 0;
		ISALESPUTBILLS_PROC func = 0;
		THROW_SL(func = reinterpret_cast<ISALESPUTBILLS_PROC>(P_Lib->GetProcAddr("iSalesPutBills")));
		outp_packet.setPointer(0);
		for(uint sended_count = 0; sended_count < outp_packet.getCount(); sended_count += max_fraction_size) {
			sess.Setup(SvcUrl);
			//
			const uint _ptr = outp_packet.getPointer();
			SString * p_result = func(sess, UserName, Password, &outp_packet, max_fraction_size);
			outp_packet.incPointerSafe((int)max_fraction_size);
			//
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
			{
				long   err_item_count = 0;
				TSCollection <iSalesPepsi::ResultItem> result_list;
				ParseResultString(*p_result, result_list, &err_item_count); // @v9.5.1 &err_item_count
				{
					//PPTXT_LOG_SUPPLIX_EXPBILL_E   "Экспортировано @int документов поставщику @zstr '@article'. Количество документов с ошибками: @int"
					/* @v9.5.1
					uint   i;
					for(i = 0; i < result_list.getCount(); i++) {
						const ResultItem * p_result_item = result_list.at(i);
						if(p_result_item && p_result_item->Status == 0)
							err_item_count++;
					}
					*/
					const long __count = outp_packet.getPointer();
					total_error_count += err_item_count;
					PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_E, &msg_buf, __count, tech_buf.cptr(), P.SupplID, total_error_count);
					PPWaitMsg(msg_buf);
					if(err_item_count)
						LogErrors(result_list, &msg_buf);
				}
				for(uint i = 0; i < result_list.getCount(); i++) {
					const ResultItem * p_result_item = result_list.at(i);
					if(p_result_item && p_result_item->Status == 1) {
						BillTbl::Rec bill_rec;
						int    found = 0;
						for(uint j = 0; !found && j < outp_packet.getCount(); j++) {
							const iSalesBillPacket * p_item = outp_packet.at(j);
							if(p_item) {
								(temp_buf = p_item->Code).Transf(CTRANSF_UTF8_TO_INNER);
								if(temp_buf.CmpNC(p_result_item->ItemDescr) == 0) {
									if(p_item->NativeID && p_item->iSalesId.NotEmpty() && P_BObj->Search(p_item->NativeID, &bill_rec) > 0) {
										if(bill_ack_tag_id) {
											if(p_item->Status == 1) {
												THROW(p_ref->Ot.RemoveTag(PPOBJ_BILL, p_item->NativeID, bill_ack_tag_id, 1));
											}
											else {
												ObjTagItem tag_item;
												// @v9.6.2 Debug_TestUtfText(p_item->iSalesId, "sendinvoices-1", R_Logger); // @v9.5.11
												(temp_buf = p_item->iSalesId).Transf(CTRANSF_UTF8_TO_INNER);
												if(tag_item.SetStr(bill_ack_tag_id, temp_buf))
													THROW(p_ref->Ot.PutTag(PPOBJ_BILL, p_item->NativeID, &tag_item, 1));
											}
										}
									}
									found = 1;
								}
							}
						}
					}
				}
			}
			DestroyResult(reinterpret_cast<void **>(&p_result));
			if(max_fraction_size == 0)
				break;
		}
    }
    CATCHZOK
	return ok;
}
//
//
//
class SapEfes : public PrcssrSupplInterchange::ExecuteBlock {
public:
	SapEfes(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	~SapEfes();
	void   Init();
	int    ReceiveOrders();
	int    SendStocks();
	int    SendInvoices();
	int    SendDebts();
	int    SendSales_ByGoods();
	int    SendSales_ByDlvrLoc();
private:
	int    PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	void   InitCallHeader(SapEfesCallHeader & rHdr);
	int    Helper_MakeBillList(PPID opID, TSCollection <SapEfesBillPacket> & rList, PPIDArray & rToCancelBillList);
	int    LogResultMsgList(const TSCollection <SapEfesLogMsg> * pMsgList);
	int    PrepareDebtsData(TSCollection <SapEfesDebtReportEntry> & rList, TSCollection <SapEfesDebtDetailReportEntry> & rDetailList);
	int    Helper_SendDebts(TSCollection <SapEfesDebtReportEntry> & rList);
	int    Helper_SendDebtsDetail(TSCollection <SapEfesDebtDetailReportEntry> & rList);
	int    MakeOrderReply(TSCollection <SapEfesBillStatus> & rList, const SapEfesOrder * pSrcPack, PPID resultOrderID, const char * pRetCode);

	enum {
		stInited     = 0x0001,
		stEpDefined  = 0x0002
	};

	long   State;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	PPIDArray GoodsList;
	SString SvcUrl;
	SString UserName;
	SString Password;
	//
	// Следующие 2 строки используются для инициализации заголовка запросов
	//
	SString SalesOrg;
	SString Wareh;
	//
	SString LastMsg;
	PPLogger & R_Logger;
};

SapEfes::SapEfes(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger),
	State(0), P_DestroyFunc(0)
{
	PPGetFilePath(PPPATH_LOG, "sapefes.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapEfes.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib)
			P_DestroyFunc = (void *)P_Lib->GetProcAddr("EfesDestroyResult");
	}
}

SapEfes::~SapEfes()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

void SapEfes::Init()
{
	Reference * p_ref = PPRef;
	State = 0;
	SvcUrl.Z();
	UserName.Z();
	Password.Z();
	Wareh.Z();
	{
        Ep.GetExtStrData(Ep.extssRemoteAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		Ep.GetExtStrData(Ep.extssClientCode, SalesOrg);
		State |= stEpDefined;
	}
	if(Ep.Fb.LocCodeTagID) {
		SString loc_code;
		for(uint i = 0; i < P.LocList.GetCount(); i++) {
			const PPID loc_id = P.LocList.Get(i);
			if(loc_id && p_ref->Ot.GetTagStr(PPOBJ_LOCATION, loc_id, Ep.Fb.LocCodeTagID, loc_code) > 0) {
				Wareh = loc_code;
				break;
			}
		}
	}
	Wareh.SetIfEmpty("DDJ0");
	InitGoodsList(iglfWithArCodesOnly); // @v9.5.1
	State |= stInited;
}

int SapEfes::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL SapEfes::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		static_cast<UHTT_DESTROYRESULT>(P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

void SapEfes::InitCallHeader(SapEfesCallHeader & rHdr)
{
	GetSequence(&rHdr.SessionID, 1);
	rHdr.P_SalesOrg = SalesOrg;
	rHdr.P_Wareh = Wareh;
}

int SapEfes::MakeOrderReply(TSCollection <SapEfesBillStatus> & rList, const SapEfesOrder * pSrcPack, PPID resultOrderID, const char * pRetCode)
{
	int    ok = 1;
	SapEfesBillStatus * p_new_status = rList.CreateNewItem();
	THROW_SL(p_new_status);
	p_new_status->Code = pSrcPack->Code;
	p_new_status->NativeCode.Cat(resultOrderID);
	p_new_status->Status = pRetCode;
	CATCHZOK
	return ok;
}

int SapEfes::ReceiveOrders()
{
    int    ok = -1;
	Reference * p_ref = PPRef;
	const PPID agent_acs_id = GetAgentAccSheet();
	PPSoapClientSession sess;
	SString temp_buf;
	SString added_param;
	SString msg_buf;
	SString src_order_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <SapEfesOrder> * p_result = 0;
	TSCollection <SapEfesBillStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	SapEfesCallHeader sech;
	EFESGETSALESORDERSYNCLIST_PROC func = 0;
	EFESSETSALESORDERSTATUSSYNC_PROC func_status = 0;

	DateRange period;
	period = P.ExpPeriod;
	if(!checkdate(period.low))
		period.low = encodedate(1, 12, 2016);
	if(!checkdate(period.upp))
		period.upp = encodedate(31, 12, 2017);
	SString tech_buf;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	P.GetExtStrData(P.extssParam, added_param);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_S, &msg_buf, tech_buf.cptr());
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<EFESGETSALESORDERSYNCLIST_PROC>(P_Lib->GetProcAddr("EfesGetSalesOrderSyncList")));
	THROW_SL(func_status = reinterpret_cast<EFESSETSALESORDERSTATUSSYNC_PROC>(P_Lib->GetProcAddr("EfesSetSalesOrderStatusSync")));
	sess.Setup(SvcUrl, UserName, Password);
	InitCallHeader(sech);
	{
		const int do_incl_processed_items = BIN(P.Flags & P.fRepeatProcessing);
		p_result = func(sess, sech, &period, do_incl_processed_items, added_param);
	}
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		//PPTXT_LOG_SUPPLIX_IMPORD_E    "Импортировано @int заказов @zstr"
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_E, &msg_buf, (long)p_result->getCount(), tech_buf.cptr());
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	if(p_result->getCount()) {
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			PPID   loc_id = 0;
			PPIDArray dlvr_loc_list;
			PPIDArray person_list;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const SapEfesOrder * p_src_pack = p_result->at(i);
				if(!p_src_pack || !checkdate(p_src_pack->Date.d)) {
					if(p_src_pack)
						THROW(MakeOrderReply(status_list, p_src_pack, 0, "E0007")); // Ошибка
				}
				else {
					int    skip = 0;
					PPID   ex_bill_id = 0;
					PPID   dlvr_loc_id = 0;
					PPID   contractor_by_loc_ar_id = 0; // Контрагент идентифицированный по адресу доставки
					PPID   contractor_ar_id = 0; // Контрагент, идентифицированный собственно по коду контрагента
					Goods2Tbl::Rec goods_rec;
					LocationTbl::Rec loc_rec;
					PersonTbl::Rec psn_rec;
					BillTbl::Rec ex_bill_rec;
					PPBillPacket pack;
					PPBillPacket::SetupObjectBlock sob;
					THROW(pack.CreateBlank_WithoutCode(acfg.Hdr.OpID, 0, loc_id, 1));
					pack.Rec.Dt = p_src_pack->Date.d;
					if(checkdate(p_src_pack->DueDate)) {
						pack.Rec.Dt = p_src_pack->DueDate;
						pack.Rec.DueDate = p_src_pack->DueDate;
					}
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					if(Ep.Fb.LocCodeTagID && p_src_pack->DlvrLoc.Code.NotEmpty()) {
						p_ref->Ot.SearchObjectsByStr(PPOBJ_LOCATION, Ep.Fb.LocCodeTagID, p_src_pack->DlvrLoc.Code, &dlvr_loc_list);
						uint dli = dlvr_loc_list.getCount();
						if(dli) do {
							const PPID _loc_id = dlvr_loc_list.get(--dli);
							if(LocObj.Fetch(_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
								if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &psn_rec) > 0) {
									ArObj.P_Tbl->PersonToArticle(psn_rec.ID, op_rec.AccSheetID, &contractor_by_loc_ar_id);
									if(contractor_by_loc_ar_id)
										dlvr_loc_id = _loc_id;
								}
							}
							else
								dlvr_loc_list.atFree(dli);
						} while(!contractor_by_loc_ar_id && dli);
					}
					if(Ep.Fb.CliCodeTagID && p_src_pack->Buyer.Code.NotEmpty()) {
						p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, Ep.Fb.CliCodeTagID, p_src_pack->Buyer.Code, &person_list);
						uint pli = person_list.getCount();
						if(pli) do {
							const  PPID _psn_id = person_list.get(--pli);
							ArObj.P_Tbl->PersonToArticle(_psn_id, op_rec.AccSheetID, &contractor_ar_id);
						} while(!contractor_ar_id && pli);
					}
					if(contractor_by_loc_ar_id) {
						if(contractor_ar_id && contractor_ar_id != contractor_by_loc_ar_id) {
							// message
						}
						sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
						if(!pack.SetupObject(contractor_by_loc_ar_id, sob)) {
							R_Logger.LogLastError();
							skip = 1;
						}
					}
					else if(contractor_ar_id) {
						sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
						if(!pack.SetupObject(contractor_ar_id, sob)) {
							R_Logger.LogLastError();
							skip = 1;
						}
					}
					else {
						R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLINCODE, &msg_buf, (const char *)pack.Rec.Code, p_src_pack->Buyer.Code.cptr()));
						skip = 1;
					}
					if(skip) {
						THROW(MakeOrderReply(status_list, p_src_pack, 0, "E0007")); // Ошибка
					}
					else {
						if(!dlvr_loc_id) {
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNCODE, &msg_buf, (const char *)pack.Rec.Code, p_src_pack->DlvrLoc.Code.cptr()));
						}
						else if(pack.Rec.Object)
							pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
						if(Ep.Fb.CliCodeTagID && agent_acs_id && p_src_pack->TerrIdent.NotEmpty()) {
							p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, Ep.Fb.CliCodeTagID, p_src_pack->TerrIdent, &person_list);
							uint pli = person_list.getCount();
							PPID   agent_ar_id = 0;
							if(pli) do {
								const  PPID _psn_id = person_list.get(--pli);
								ArObj.P_Tbl->PersonToArticle(_psn_id, agent_acs_id, &agent_ar_id);
							} while(!agent_ar_id && pli);
							if(agent_ar_id) {
								pack.Ext.AgentID = agent_ar_id;
							}
							else {
								R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_AGENTNCODE, &msg_buf, (const char *)pack.Rec.Code, p_src_pack->TerrIdent.cptr()));
							}
						}
						// @v11.1.12 STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
						pack.SMemo = p_src_pack->Memo; // @v11.1.12
						if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
							PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName, temp_buf).Quot('(', ')');
							if(PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, temp_buf, 1, msg_buf))
								R_Logger.Log(msg_buf);
							THROW(MakeOrderReply(status_list, p_src_pack, ex_bill_rec.ID, "E0008"));
						}
						else {
							GoodsStockExt gse;
							for(uint i = 0; i < p_src_pack->Items.getCount(); i++) {
								const SapEfesBillItem * p_src_item = p_src_pack->Items.at(i);
								if(p_src_item) {
									if(GObj.P_Tbl->SearchByArCode(P.SupplID, p_src_item->GoodsCode, 0, &goods_rec) > 0) {
										PPTransferItem ti;
										ti.Init(&pack.Rec);
										double _qtty = fabs(p_src_item->Qtty);
										double _qtty_mult = 1.0;
										if(p_src_item->UnitType == sapefesUnitPack) {
											if(p_src_item->BaseUnitType == sapefesUnitItem && p_src_item->QtyN > 0.0)
												_qtty_mult = (p_src_item->QtyD / p_src_item->QtyN);
											else if(GObj.GetStockExt(goods_rec.ID, &gse, 0) > 0 && gse.Package > 0.0)
												_qtty_mult = gse.Package;
										}
										_qtty *= _qtty_mult;
										ti.GoodsID = goods_rec.ID;
										ti.Quantity_ = _qtty;
										if(ti.Quantity_ != 0.0) {
											double vat_amt = 0.0;
											GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, /*ti.Quantity_*/1.0, p_src_item->Amount, &vat_amt, 1, 0);
											ti.Price = (p_src_item->Amount + vat_amt) / _qtty_mult;
											THROW(pack.LoadTItem(&ti, 0, 0));
										}
									}
									else {
										R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNCODE, &msg_buf, (const char *)pack.Rec.Code, p_src_item->GoodsCode.cptr()));
										skip = 1;
									}
								}
							}
							if(!skip) {
								pack.SetupEdiAttributes(PPEDIOP_SALESORDER, "SAP-EFES", p_src_pack->Code);
								pack.InitAmounts();
								THROW(P_BObj->TurnPacket(&pack, 1));
								THROW(MakeOrderReply(status_list, p_src_pack, pack.Rec.ID, "E0008"));
							}
							else {
								THROW(MakeOrderReply(status_list, p_src_pack, 0, "E0007")); // Ошибка
							}
						}
					}
				}
			}
			if(status_list.getCount()) {
				sess.Setup(SvcUrl, UserName, Password);
				InitCallHeader(sech);
				const int status_result = func_status(sess, sech, &status_list);
				THROW_PP_S(PreprocessResult((const void *)status_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			}
		}
	}
    CATCHZOK
	DestroyResult(reinterpret_cast<void **>(&p_result));
    return ok;
}

int SapEfes::SendSales_ByDlvrLoc()
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  LDATE _curdate = getcurdate_();
	TSCollection <SapEfesLogMsg> * p_result = 0;
	TSCollection <SapEfesGoodsReportEntry> outp_packet;
	PPSoapClientSession sess;
	SapEfesCallHeader sech;
	EFESSETMTDOUTLETSREPORTSYNC_PROC func = 0;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<EFESSETMTDOUTLETSREPORTSYNC_PROC>(P_Lib->GetProcAddr("EfesSetMTDOutletsReportSync")));
	sess.Setup(SvcUrl, UserName, Password);
	InitCallHeader(sech);
	{
		SString cli_code, loc_code;
		PPViewTrfrAnlz ta_view;
		TrfrAnlzFilt ta_filt;
		TrfrAnlzViewItem ta_item;
		ta_filt.Period.upp = NZOR(P.ExpPeriod.upp, _curdate);
		ta_filt.Period.low = encodedate(1, ta_filt.Period.upp.month(), ta_filt.Period.upp.year());
		ta_filt.OpID = Ep.ExpendOp;
		ta_filt.GoodsGrpID = Ep.GoodsGrpID;
		ta_filt.LocList = P.LocList;
		ta_filt.Grp = TrfrAnlzFilt::gCntragent;
		ta_filt.Flags |= TrfrAnlzFilt::fDiffByDlvrAddr;
		if(ta_view.Init_(&ta_filt)) {
			for(ta_view.InitIteration(PPViewTrfrAnlz::OrdByDefault); ta_view.NextIteration(&ta_item) > 0;) {
				const PPID cli_psn_id = ObjectToPerson(ta_item.ArticleID, 0);
				const PPID dlvr_loc_id = ta_item.DlvrLocID;
				cli_code = 0;
				loc_code = 0;
                p_ref->Ot.GetTagStr(PPOBJ_PERSON, cli_psn_id, Ep.Fb.CliCodeTagID, cli_code);
                if(dlvr_loc_id)
					p_ref->Ot.GetTagStr(PPOBJ_LOCATION, dlvr_loc_id, Ep.Fb.LocCodeTagID, loc_code);
				if(cli_code.NotEmpty() || loc_code.NotEmpty()) {
					SapEfesGoodsReportEntry * p_new_item = outp_packet.CreateNewItem();
					THROW_SL(p_new_item);
					p_new_item->DlvrLocCode = loc_code;
					p_new_item->Dt = ta_filt.Period.upp;
					p_new_item->Qtty = fabs(ta_item.PhQtty);
					p_new_item->UnitType = spaefesUnitLiter;
				}
			}
		}
	}
	if(outp_packet.getCount()) {
		p_result = func(sess, sech, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		LogResultMsgList(p_result);
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
	CATCHZOK
	return ok;
}

int SapEfes::SendSales_ByGoods()
{
	int    ok = 1;
	const  LDATE _curdate = getcurdate_();
	TSCollection <SapEfesLogMsg> * p_result = 0;
	TSCollection <SapEfesGoodsReportEntry> outp_packet;
	PPSoapClientSession sess;
	SapEfesCallHeader sech;
	EFESSETMTDPRODUCTREPORTSYNC_PROC func = 0;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<EFESSETMTDPRODUCTREPORTSYNC_PROC>(P_Lib->GetProcAddr("EfesSetMTDProductReportSync")));
	sess.Setup(SvcUrl, UserName, Password);
	InitCallHeader(sech);
	{
		SString ar_code;
		PPViewTrfrAnlz ta_view;
		TrfrAnlzFilt ta_filt;
		TrfrAnlzViewItem ta_item;
		ta_filt.Period.upp = NZOR(P.ExpPeriod.upp, _curdate);
		ta_filt.Period.low = encodedate(1, ta_filt.Period.upp.month(), ta_filt.Period.upp.year());
		ta_filt.OpID = Ep.ExpendOp;
		ta_filt.GoodsGrpID = Ep.GoodsGrpID;
		ta_filt.LocList = P.LocList;
		ta_filt.Grp = TrfrAnlzFilt::gGoods;
		if(ta_view.Init_(&ta_filt)) {
			for(ta_view.InitIteration(PPViewTrfrAnlz::OrdByDefault); ta_view.NextIteration(&ta_item) > 0;) {
				int32  ar_code_pack = 0;
				if(GObj.P_Tbl->GetArCode(P.SupplID, ta_item.GoodsID, ar_code, &ar_code_pack) > 0) {
					SapEfesGoodsReportEntry * p_new_item = outp_packet.CreateNewItem();
					THROW_SL(p_new_item);
					p_new_item->GoodsCode = ar_code;
					p_new_item->Dt = ta_filt.Period.upp;
					p_new_item->Qtty = fabs(ta_item.PhQtty);
					p_new_item->UnitType = spaefesUnitLiter;
				}
			}
		}
	}
	if(outp_packet.getCount()) {
		p_result = func(sess, sech, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		LogResultMsgList(p_result);
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
	CATCHZOK
	return ok;
}

int SapEfes::SendStocks()
{
    int    ok = -1;
    const  LDATE _curdate = getcurdate_();
	SString temp_buf;
    TSCollection <SapEfesLogMsg> * p_result = 0;
	TSCollection <SapEfesGoodsReportEntry> outp_packet;
	PPSoapClientSession sess;
	SapEfesCallHeader sech;
	EFESSETDAILYSTOCKREPORTSYNC_PROC func = 0;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<EFESSETDAILYSTOCKREPORTSYNC_PROC>(P_Lib->GetProcAddr("EfesSetDailyStockReportSync")));
	sess.Setup(SvcUrl, UserName, Password);
	InitCallHeader(sech);
	{
		PPViewGoodsRest gr_view;
		GoodsRestFilt gr_filt;
		GoodsRestViewItem gr_item;
		SString ar_code;
		gr_filt.Date = NZOR(P.ExpPeriod.upp, _curdate);
		gr_filt.GoodsGrpID = Ep.GoodsGrpID;
		gr_filt.LocList = P.LocList;
		gr_filt.Flags |= GoodsRestFilt::fEachLocation;
		THROW(gr_view.Init_(&gr_filt));
		for(gr_view.InitIteration(PPViewGoodsRest::OrdByDefault); gr_view.NextIteration(&gr_item) > 0;) {
			const PPID loc_id = gr_item.LocID;
			uint   _pos = 0;
			int    skip_goods = 0;
			int32  ar_code_pack = 0;
			if(GObj.P_Tbl->GetArCode(P.SupplID, gr_item.GoodsID, ar_code, &ar_code_pack) > 0) {
				SapEfesGoodsReportEntry * p_new_item = outp_packet.CreateNewItem();
				THROW_SL(p_new_item);
				p_new_item->GoodsCode = ar_code;
				p_new_item->Dt = gr_filt.Date;
                p_new_item->Qtty = gr_item.PhRest;
				p_new_item->UnitType = spaefesUnitLiter;
			}
		}
	}
	if(outp_packet.getCount()) {
		p_result = func(sess, sech, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		LogResultMsgList(p_result);
	}
	DestroyResult(reinterpret_cast<void **>(&p_result));
    CATCHZOK
    return ok;
}

int SapEfes::Helper_MakeBillList(PPID opID, TSCollection <SapEfesBillPacket> & rList, PPIDArray & rToCancelBillList)
{
	rToCancelBillList.clear();
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID bill_ack_tag_id = Ep.Fb.BillAckTagID;
	THROW_PP_S(Ep.Fb.CliCodeTagID, PPERR_SUPPLIX_UNDEFCLICTAG, ArName);
	THROW_PP_S(Ep.Fb.LocCodeTagID, PPERR_SUPPLIX_UNDEFLOCCTAG, ArName);
	if(opID) {
		SString temp_buf;
		SString msg_buf;
		SString cli_code;
		SString loc_code;
		SString isales_code;
		PPViewBill b_view;
		//PPIDArray to_cancel_bill_list; // Список документов, по которым следует отослать отмену поставок
		PPIDArray order_id_list;
		BillFilt b_filt;
		BillViewItem view_item;
		PPBillPacket pack;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		b_filt.OpID = opID;
		b_filt.Period = P.ExpPeriod; //BillExportPeriod;
		SETIFZ(b_filt.Period.low, encodedate(1, 9, 2016));
		if(bill_ack_tag_id) {
			BillTbl::Rec bill_rec;
			PPIDArray acn_list;
			acn_list.add(PPACN_UPDBILL);
			LDATETIME since;
			since.Set(b_filt.Period.low, ZEROTIME);
			PPIDArray upd_bill_list;
			p_sj->GetObjListByEventSince(PPOBJ_BILL, &acn_list, since, upd_bill_list, 0);
			for(uint i = 0; i < upd_bill_list.getCount(); i++) {
				const PPID upd_bill_id = upd_bill_list.get(i);
				if(p_ref->Ot.GetTagStr(PPOBJ_BILL, upd_bill_id, bill_ack_tag_id, temp_buf) > 0) {
                    if(P_BObj->Search(upd_bill_id, &bill_rec) > 0) {
						if(IsOpBelongTo(bill_rec.OpID, b_filt.OpID)) {
							if(P_BObj->ExtractPacket(upd_bill_id, &pack) > 0) {
								long   tiiterpos = 0;
								int    is_t_goods = 0;
								for(uint tiidx = 0; !is_t_goods && tiidx < pack.GetTCount(); tiidx++) {
									const PPID goods_id = labs(pack.ConstTI(tiidx).GoodsID);
									if(GObj.BelongToGroup(goods_id, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, goods_id, temp_buf.Z(), 0) > 0)
										is_t_goods = 1;
								}
								if(!is_t_goods) {
									//
									// Если в документе не осталось ни одной строки нашего товара, то - в список на удаление
									//
									rToCancelBillList.add(upd_bill_id);
								}
							}
						}
                    }
				}
			}
		}
		rToCancelBillList.sortAndUndup();
		//
		THROW(b_view.Init_(&b_filt));
		for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
			const PPID bill_id = view_item.ID;
			if(!rToCancelBillList.bsearch(bill_id)) {
				PPBillPacket pack;
				PPFreight freight;
				if(P_BObj->ExtractPacket(bill_id, &pack) > 0) {
					int    is_own_order = 0; // !0 если документ выписан по заказу, импортированному от поставщика
					StrAssocArray ti_pos_list;
					PPTransferItem ti;
					PPBillPacket::TiItemExt tiext;
					{
						long   tiiterpos = 0;
						for(TiIter tiiter(&pack, ETIEF_FORCEUNITEGOODS|ETIEF_UNITEBYGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
							tiiterpos++;
							if(GObj.BelongToGroup(ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, ti.GoodsID, temp_buf.Z(), 0) > 0)
								ti_pos_list.Add(tiiterpos, temp_buf, 0);
						}
					}
					if(ti_pos_list.getCount()) {
						const PPID cli_psn_id = ObjectToPerson(pack.Rec.Object, 0);
						int   skip = 0;
						cli_code.Z();
						loc_code.Z();
                        p_ref->Ot.GetTagStr(PPOBJ_PERSON, cli_psn_id, Ep.Fb.CliCodeTagID, cli_code);
                        if(pack.GetDlvrAddrID())
							p_ref->Ot.GetTagStr(PPOBJ_LOCATION, pack.GetDlvrAddrID(), Ep.Fb.LocCodeTagID, loc_code);
						if(cli_code.IsEmpty()) {
							if(loc_code.NotEmpty())
								cli_code = loc_code;
							else {
								//PPTXT_LOG_SUPPLIX_NOCLLCCODES
                                PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
								R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_NOCLLCCODES, &msg_buf, temp_buf.cptr()));
								skip = 1;
							}
						}
						else
							loc_code.SetIfEmpty(cli_code);
						if(!skip) {
							Goods2Tbl::Rec goods_rec;
							BillTbl::Rec ord_rec;
							PPBillPacket order_pack;
							SapEfesBillPacket * p_new_item = rList.CreateNewItem();
							THROW_SL(p_new_item);
							p_new_item->NativeID = bill_id;
							pack.GetOrderList(order_id_list);
							for(uint ordidx = 0; ordidx < order_id_list.getCount(); ordidx++) {
								const PPID ord_id = order_id_list.get(ordidx);
								if(ord_id && P_BObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
									if(p_ref->Ot.GetTagStr(PPOBJ_BILL, ord_id, PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("SAP-EFES")) {
										THROW(P_BObj->ExtractPacket(ord_id, &order_pack) > 0);
										is_own_order = 1;
									}
								}
							}
							p_new_item->Date = pack.Rec.Dt;
							p_new_item->DocType = SapEfesBillPacket::tRetail;
							// @v11.1.12 BillCore::GetCode(p_new_item->NativeCode = pack.Rec.Code);
							p_new_item->NativeCode = pack.Rec.Code; // @v11.1.12 
							p_new_item->DueDate = (is_own_order && ord_rec.DueDate) ? ord_rec.DueDate : pack.Rec.Dt;
							p_new_item->BuyerCode = cli_code;
							p_new_item->DlvrLocCode = loc_code;
							if(is_own_order) {
								// @v11.1.12 BillCore::GetCode(p_new_item->OrderCode = ord_rec.Code);
								p_new_item->OrderCode = ord_rec.Code; // @v11.1.12 
								p_new_item->Flags |= p_new_item->fHasOrderRef;
							}
							else
								p_new_item->Flags &= ~p_new_item->fHasOrderRef;
							// @v11.1.12 p_new_item->Memo = pack.Rec.Memo;
							p_new_item->Memo = pack.SMemo; // @v11.1.12
							{
								long   tiiterpos = 0;
								for(TiIter tiiter(&pack, ETIEF_UNITEBYGOODS|ETIEF_FORCEUNITEGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
									tiiterpos++;
									uint   pos_list_item_pos = 0;
									if(ti_pos_list.GetText(tiiterpos, temp_buf) > 0 && GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
										SapEfesBillItem * p_new_row = p_new_item->Items.CreateNewItem();
										THROW_SL(p_new_row);
										p_new_row->PosN = tiiterpos;
										p_new_row->PosType = 0;
										p_new_row->GoodsCode = temp_buf;
										p_new_row->Qtty = fabs(ti.Quantity_);
										p_new_row->UnitType = sapefesUnitItem;
										{
											const double _amt = fabs(ti.Quantity_ * ti.NetPrice());
											double _vat_sum = 0.0;
											GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, 1.0, _amt, &_vat_sum, 0, 0);
											p_new_row->Amount = _amt - _vat_sum;
										}
										p_new_row->Currency = "RUB";
									}
								}
							}
						}
					}
				}
			}
		}
	}
    CATCHZOK
	return ok;
}

int SapEfes::SendInvoices()
{
    int    ok = -1;
	Reference * p_ref = PPRef;
    const  PPID bill_ack_tag_id = Ep.Fb.BillAckTagID;
	PPIDArray to_cancel_bill_list;
	SString temp_buf;
	BillTbl::Rec bill_rec;
	PPSoapClientSession sess;
	SapEfesCallHeader sech;
	TSCollection <SapEfesBillStatus> * p_result = 0;
	TSCollection <SapEfesBillPacket> outp_packet;
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW(Helper_MakeBillList(Ep.ExpendOp, outp_packet, to_cancel_bill_list));
	if(outp_packet.getCount()) {
		EFESSETDELIVERYNOTESYNC_PROC func = 0;
		//BillExportPeriod.Set(encodedate(1, 6, 2016), encodedate(30, 6, 2016));
		//THROW(Helper_MakeBillList(Ep.RetOp, 5, outp_packet));
		THROW_SL(func = reinterpret_cast<EFESSETDELIVERYNOTESYNC_PROC>(P_Lib->GetProcAddr("EfesSetDeliveryNoteSync")));
		sess.Setup(SvcUrl, UserName, Password);
		InitCallHeader(sech);
		p_result = func(sess, sech, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		for(uint i = 0; i < p_result->getCount(); i++) {
			const SapEfesBillStatus * p_status = p_result->at(i);
			if(p_status) {
				if(p_status->Code.NotEmpty()) {
					int    found = 0;
					for(uint j = 0; !found && j < outp_packet.getCount(); j++) {
						const SapEfesBillPacket * p_out_item = outp_packet.at(j);
						if(p_out_item) {
							if(p_out_item->NativeCode.CmpNC(p_status->NativeCode) == 0 && P_BObj->Search(p_out_item->NativeID, &bill_rec) > 0) {
								if(bill_ack_tag_id) {
									ObjTagItem tag_item;
									(temp_buf = p_status->Code).Transf(CTRANSF_UTF8_TO_INNER);
									if(tag_item.SetStr(bill_ack_tag_id, temp_buf)) {
										THROW(p_ref->Ot.PutTag(PPOBJ_BILL, bill_rec.ID, &tag_item, 1));
									}
								}
								found = 1;
							}
						}
					}
				}
				LogResultMsgList(&p_status->MsgList);
			}
		}
		DestroyResult(reinterpret_cast<void **>(&p_result));
		p_result = 0;
	}
	if(to_cancel_bill_list.getCount()) {
		TSCollection <SString> to_cancel_code_list;
		EFESCANCELDELIVERYNOTESYNC_PROC func = 0;
		THROW_SL(func = reinterpret_cast<EFESCANCELDELIVERYNOTESYNC_PROC>(P_Lib->GetProcAddr("EfesCancelDeliveryNoteSync")));
		{
			for(uint i = 0; i < to_cancel_bill_list.getCount(); i++) {
				const PPID to_cancel_bill_id = to_cancel_bill_list.get(i);
				if(P_BObj->Search(to_cancel_bill_id, &bill_rec) > 0) {
					SString * p_new_code = to_cancel_code_list.CreateNewItem();
					THROW_SL(p_new_code);
					*p_new_code = bill_rec.Code;
					// @v11.1.12 BillCore::GetCode(*p_new_code);
				}
			}
		}
		if(to_cancel_code_list.getCount()) {
			sess.Setup(SvcUrl, UserName, Password);
			InitCallHeader(sech);
			p_result = func(sess, sech, &to_cancel_code_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			for(uint i = 0; i < p_result->getCount(); i++) {
				const SapEfesBillStatus * p_status = p_result->at(i);
				if(p_status) {
					LogResultMsgList(&p_status->MsgList);
				}
			}
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
	}
    CATCHZOK
    return ok;
}

int SapEfes::LogResultMsgList(const TSCollection <SapEfesLogMsg> * pMsgList)
{
	int    ok = 1;
	if(pMsgList) {
		SString temp_buf;
		for(uint i = 0; i < pMsgList->getCount(); i++) {
			SapEfesLogMsg * p_msg = pMsgList->at(i);
			if(p_msg) {
				temp_buf.Z();
				if(p_msg->MsgType == SapEfesLogMsg::tE)
					temp_buf = "E";
				else if(p_msg->MsgType == SapEfesLogMsg::tW)
					temp_buf = "W";
				else if(p_msg->MsgType == SapEfesLogMsg::tS)
					temp_buf = "S";
				temp_buf.CatDivIfNotEmpty(':', 2);
				temp_buf.Cat(p_msg->Msg);
				R_Logger.Log(temp_buf);
			}
		}
	}
	return ok;
}

int SapEfes::PrepareDebtsData(TSCollection <SapEfesDebtReportEntry> & rList, TSCollection <SapEfesDebtDetailReportEntry> & rDetailList)
{
    int    ok = 1;
	{
		const int use_omt_paym_amt = BIN(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
		PPViewBill bill_view;
		BillFilt bill_filt;
		bill_filt.Period.Set(P_BObj->GetConfig().LowDebtCalcDate, ZERODATE);
        bill_filt.OpID = Ep.ExpendOp;
        bill_filt.Flags |= BillFilt::fDebtOnly;
        if(bill_view.Init_(&bill_filt)) {
			SString cli_code;
			BillViewItem bill_item;
            for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&bill_item) > 0;) {
				const PPID bill_id = bill_item.ID;
				PPBillPacket pack;
				const double org_amount = bill_item.Amount;
				if(org_amount > 0.0 && P_BObj->ExtractPacketWithRestriction(bill_id, &pack, 0, GetGoodsList()) > 0) {
					pack.InitAmounts(0);
					double paym = 0.0;
					if(pack.Rec.Amount > 0.0) {
						const PPID ar_id = pack.Rec.Object;
						if(use_omt_paym_amt)
							paym = bill_item.PaymAmount;
						else
							P_BObj->P_Tbl->CalcPayment(bill_id, 0, 0, 0/*bill_item.CurID*/, &paym);
						paym = paym * pack.Rec.Amount / org_amount;

						const  PPID cli_psn_id = ObjectToPerson(ar_id, 0);
						uint   ex_pos = 0;
						cli_code = 0;
						PPRef->Ot.GetTagStr(PPOBJ_PERSON, cli_psn_id, Ep.Fb.CliCodeTagID, cli_code);
						if(!cli_code.NotEmpty()) {
							; // @todo message
						}
						else if(rList.lsearch(&ar_id, &ex_pos, CMPF_LONG)) {
							SapEfesDebtReportEntry * p_ex_entry = rList.at(ex_pos);
							p_ex_entry->Debt += pack.Rec.Amount - paym;
						}
						else {
							{
								SapEfesDebtReportEntry * p_new_entry = rList.CreateNewItem();
								THROW_SL(p_new_entry);
								p_new_entry->NativeArID = ar_id;
								p_new_entry->BuyerCode = cli_code;
								p_new_entry->Debt = pack.Rec.Amount - paym;
								{
									PPClientAgreement cli_agt;
									if(ArObj.GetClientAgreement(ar_id, cli_agt, 1) > 0) {
										p_new_entry->CreditLimit = cli_agt.GetCreditLimit(0);
										p_new_entry->PayPeriod = cli_agt.DefPayPeriod;
									}
								}
							}
							{
								LDATE   paym_date = ZERODATE;
								SapEfesDebtDetailReportEntry * p_new_detail_entry = rDetailList.CreateNewItem();
								THROW_SL(p_new_detail_entry);
								pack.GetLastPayDate(&paym_date);
								p_new_detail_entry->NativeArID = ar_id;
								p_new_detail_entry->BuyerCode = cli_code;
								p_new_detail_entry->NativeBillCode = pack.Rec.Code;
								p_new_detail_entry->BillDate = pack.Rec.Dt;
								p_new_detail_entry->PaymDate = paym_date;
								p_new_detail_entry->Amount = pack.Rec.Amount;
								p_new_detail_entry->Debt = pack.Rec.Amount - paym;
							}
						}
					}
				}
            }
        }
	}
	rList.sort(CMPF_LONG);
	rDetailList.sort(CMPF_LONG);
    CATCHZOK
    return ok;
}

int SapEfes::Helper_SendDebts(TSCollection <SapEfesDebtReportEntry> & rList)
{
    int    ok = -1;
	if(rList.getCount()) {
		SString temp_buf;
		TSCollection <SapEfesLogMsg> * p_result = 0;
		PPSoapClientSession sess;
		SapEfesCallHeader sech;
		EFESSETDEBTSYNC_PROC func = 0;
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = reinterpret_cast<EFESSETDEBTSYNC_PROC>(P_Lib->GetProcAddr("EfesSetDebtSync")));
		sess.Setup(SvcUrl, UserName, Password);
		InitCallHeader(sech);
		{
			p_result = func(sess, sech, &rList);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			LogResultMsgList(p_result);
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
	}
    CATCHZOK
    return ok;
}

int SapEfes::Helper_SendDebtsDetail(TSCollection <SapEfesDebtDetailReportEntry> & rList)
{
    int    ok = -1;
	if(rList.getCount()) {
		SString temp_buf;
		TSCollection <SapEfesLogMsg> * p_result = 0;
		PPSoapClientSession sess;
		SapEfesCallHeader sech;
		EFESSETDEBTDETAILSYNC_PROC func = 0;
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = reinterpret_cast<EFESSETDEBTDETAILSYNC_PROC>(P_Lib->GetProcAddr("EfesSetDebtDetailSync")));
		sess.Setup(SvcUrl, UserName, Password);
		InitCallHeader(sech);
		{
			p_result = func(sess, sech, &rList);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			LogResultMsgList(p_result);
			DestroyResult(reinterpret_cast<void **>(&p_result));
		}
	}
    CATCHZOK
    return ok;
}

int SapEfes::SendDebts()
{
    int    ok = -1;
	TSCollection <SapEfesDebtReportEntry> outp_list;
	TSCollection <SapEfesDebtDetailReportEntry> outp_detail_list;
	TSCollection <SapEfesLogMsg> * p_result = 0;
	THROW(PrepareDebtsData(outp_list, outp_detail_list));
	THROW(Helper_SendDebts(outp_list));
	THROW(Helper_SendDebtsDetail(outp_detail_list));
    CATCHZOK
    return ok;
}
//
//
//
class SfaHeineken : public PrcssrSupplInterchange::ExecuteBlock {
public:
	SfaHeineken(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	~SfaHeineken();
	void   Init();
	int    ReceiveGoods();
	int    ReceiveOrders();
	int    SendStocks();
	int    SendSales();
	int    SendReceipts();
	int    SendStatus(const TSCollection <SfaHeinekenOrderStatusEntry> & rList);
	int    SendDebts();
private:
	int    Helper_MakeBillEntry(PPID billID, int outerDocType, TSCollection <SfaHeinekenInvoice> & rList, TSCollection <SfaHeinekenInvoice> & rToDeleteList);
	int    Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <SfaHeinekenInvoice> & rList,
		TSCollection <SfaHeinekenInvoice> & rToDeleteList);
	int    Helper_MakeDeliveryList(PPBillPacket & rPack, const StrAssocArray & rTiPosList, TSCollection <SfaHeinekenInvoice::DeliveryPosition> & rDeliveryList);

	struct ReplyInfo {
		ReplyInfo() : ErrCode(0), ErrInternalCode(0), ErrSeverity(0)
		{
		}
		int    ErrCode;
		int    ErrInternalCode;
		int    ErrSeverity;
		SString ErrMessage;
		SString ErrSource;
		SString ProcessingType;
	};
	struct GoodsEntry {
		int    ID;
		SString Name;
	};
	struct OrderEntry {
		OrderEntry() : Dtm(ZERODATETIME), DlvrDtm(ZERODATETIME), ForeignDlvrAddrID(0), PaidSum(0.0), UserID(0), WarehouseID(0)
		{
		}
		struct Item {
			Item() : SkuID(0), UnitID(0), Qtty(0)
			{
			}
			int    SkuID;
			int    UnitID;
			int    Qtty;
			SString SkuName;
		};
		S_GUID Uuid;
		LDATETIME Dtm;
		LDATETIME DlvrDtm;
		int    ForeignDlvrAddrID;
		double PaidSum;     // Сумма инкассации
		int    UserID;      // Идентификатор пользователя, создавшего заказ
		int    WarehouseID; // Идентификатор склада дистрибьютора
		SString ForeignDlvrAddrText;
		SString Memo;
		SString PaidNumber; // Номер кассового ордера
		SString UserName;   // Фамилия Имя Отчество торгового представителя
		TSCollection <Item> ItemList;
	};
	int    PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	int    ParseReplyInfo(const xmlNode * pNode, ReplyInfo * pInfo)
	{
		int    ok = -1;
		SString temp_buf;
		if(SXml::IsName(pNode, "Error")) {
			if(pNode->children) {
				if(pInfo) {
					for(const xmlNode * p_r = pNode->children; p_r; p_r = p_r->next) {
						if(SXml::GetContentByName(p_r, "ErrorID", temp_buf))
							pInfo->ErrCode = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_r, "InternalErrorID", temp_buf))
							pInfo->ErrInternalCode = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_r, "Severity", temp_buf))
							pInfo->ErrSeverity = temp_buf.ToLong();
						else if(SXml::GetContentByName(p_r, "ErrorMessage", temp_buf))
							pInfo->ErrMessage = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						else if(SXml::GetContentByName(p_r, "ErrorSource", temp_buf))
							pInfo->ErrSource = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					}
				}
			}
			ok = 1;
		}
		else if(SXml::GetContentByName(pNode, "ProcessingType", temp_buf)) {
			if(pInfo) {
				pInfo->ProcessingType = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			ok = 1;
		}
		return ok;
	}
	int    ParseOrdersPacket(const SString * pSrc, ReplyInfo * pInfo, TSCollection <OrderEntry> & rList)
	{
		int    ok = -1;
		xmlParserCtxt * p_ctx = 0;
		xmlDoc * p_doc = 0;
		if(pSrc && pSrc->NotEmpty()) {
			const size_t avl_size = pSrc->Len();
			SString temp_buf;
			const xmlNode * p_root = 0;
			THROW(p_ctx = xmlNewParserCtxt());
			THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, pSrc->cptr(), avl_size, 0, 0, XML_PARSE_NOENT), p_ctx);
			THROW(p_root = xmlDocGetRootElement(p_doc));
			if(SXml::IsName(p_root, "DRP_GetOrders") || SXml::IsName(p_root, "DRP_GetOrdersDemo") || SXml::IsName(p_root, "DRP_GetOrdersByDate")) {
				for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
					if(ParseReplyInfo(p_c, pInfo) > 0) {
						;
					}
					else if(SXml::IsName(p_c, "ResultObject")) {
						for(const xmlNode * p_r = p_c->children; p_r; p_r = p_r->next) {
							if(SXml::IsName(p_r, "Order")) {
								OrderEntry * p_new_entry = rList.CreateNewItem();
								THROW_SL(p_new_entry);
								for(const xmlNode * p_ord = p_r->children; p_ord; p_ord = p_ord->next) {
									if(SXml::GetContentByName(p_ord, "OrderID", temp_buf))
										p_new_entry->Uuid.FromStr(temp_buf);
									else if(SXml::GetContentByName(p_ord, "DateTime", temp_buf))
										strtodatetime(temp_buf, &p_new_entry->Dtm, DATF_ISO8601, TIMF_HMS);
									else if(SXml::GetContentByName(p_ord, "DateDelivery", temp_buf))
										strtodatetime(temp_buf, &p_new_entry->DlvrDtm, DATF_ISO8601, TIMF_HMS);
									else if(SXml::GetContentByName(p_ord, "SalePointID", temp_buf))
										p_new_entry->ForeignDlvrAddrID = temp_buf.ToLong();
									else if(SXml::GetContentByName(p_ord, "SalePointName", temp_buf))
										p_new_entry->ForeignDlvrAddrText = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									else if(SXml::GetContentByName(p_ord, "Comments", temp_buf))
										p_new_entry->Memo = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									else if(SXml::GetContentByName(p_ord, "PaidNumber", temp_buf))
										p_new_entry->PaidNumber = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									else if(SXml::GetContentByName(p_ord, "PaidSum", temp_buf))
										p_new_entry->PaidSum = temp_buf.ToReal();
									else if(SXml::GetContentByName(p_ord, "UserID", temp_buf))
										p_new_entry->UserID = temp_buf.ToLong();
									else if(SXml::GetContentByName(p_ord, "UserName", temp_buf))
										p_new_entry->UserName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									else if(SXml::GetContentByName(p_ord, "WareHouseID", temp_buf))
										p_new_entry->WarehouseID = temp_buf.ToLong();
									else if(SXml::IsName(p_ord, "OrderPositions")) {
										for(const xmlNode * p_items = p_ord->children; p_items; p_items = p_items->next) {
											if(SXml::IsName(p_items, "OrderPosition")) {
												OrderEntry::Item * p_new_item = p_new_entry->ItemList.CreateNewItem();
												THROW_SL(p_new_item);
												for(const xmlNode * p_it = p_items->children; p_it; p_it = p_it->next) {
													if(SXml::GetContentByName(p_it, "SkuID", temp_buf))
														p_new_item->SkuID = temp_buf.ToLong();
													else if(SXml::GetContentByName(p_it, "SkuName", temp_buf))
														p_new_item->SkuName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
													else if(SXml::GetContentByName(p_it, "UnitID", temp_buf))
														p_new_item->UnitID = temp_buf.ToLong();
													else if(SXml::GetContentByName(p_it, "Num", temp_buf))
														p_new_item->Qtty = temp_buf.ToLong();
												}
											}
										}
									}
								}
							}
						}
					}
				}
				ok = 1;
			}
		}
		CATCHZOK
		xmlFreeDoc(p_doc);
		xmlFreeParserCtxt(p_ctx);
		return ok;
	}
	int    ParseGoodsPacket(const SString * pSrc, ReplyInfo * pInfo, TSCollection <GoodsEntry> & rList)
	{
		int    ok = -1;
		xmlParserCtxt * p_ctx = 0;
		xmlDoc * p_doc = 0;
		if(pSrc && pSrc->NotEmpty()) {
			const size_t avl_size = pSrc->Len();
			SString temp_buf;
			const xmlNode * p_root = 0;
			THROW(p_ctx = xmlNewParserCtxt());
			THROW_LXML(p_doc = xmlCtxtReadMemory(p_ctx, pSrc->cptr(), avl_size, 0, 0, XML_PARSE_NOENT), p_ctx);
			THROW(p_root = xmlDocGetRootElement(p_doc));
			if(SXml::IsName(p_root, "DRP_GetSkuAssortment")) {
				for(const xmlNode * p_c = p_root->children; p_c; p_c = p_c->next) {
					if(ParseReplyInfo(p_c, pInfo) > 0) {
						;
					}
					else if(SXml::IsName(p_c, "ResultObject")) {
						for(const xmlNode * p_r = p_c->children; p_r; p_r = p_r->next) {
							if(SXml::IsName(p_r, "Sku")) {
								for(const xmlNode * p_sku = p_r->children; p_sku; p_sku = p_sku->next) {
									GoodsEntry * p_new_entry = rList.CreateNewItem();
									THROW_SL(p_new_entry);
									if(SXml::GetContentByName(p_sku, "ID", temp_buf)) {
										p_new_entry->ID = temp_buf.ToLong();
									}
									else if(SXml::GetContentByName(p_sku, "Name", temp_buf)) {
										p_new_entry->Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
									}
								}
							}
						}
					}
				}
				ok = 1;
			}
		}
		CATCHZOK
		xmlFreeDoc(p_doc);
		xmlFreeParserCtxt(p_ctx);
		return ok;
	}

	enum {
		stInited     = 0x0001,
		stEpDefined  = 0x0002
	};

	long   State;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	PPIDArray GoodsList;
	SString SvcUrl;
	SString UserName;
	SString Password;
	SString SalesOrg;
	SString LastMsg;
	PPLogger & R_Logger;
};

SfaHeineken::SfaHeineken(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger),
	State(0), P_DestroyFunc(0)
{
	PPGetFilePath(PPPATH_LOG, "sfaheineken.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapSfaHeineken.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib)
			P_DestroyFunc = static_cast<void *>(P_Lib->GetProcAddr("SfaHeinekenDestroyResult"));
	}
}

SfaHeineken::~SfaHeineken()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

void SfaHeineken::Init()
{
	Reference * p_ref = PPRef;
	State = 0;
	SvcUrl.Z();
	UserName.Z();
	Password.Z();
	{
        Ep.GetExtStrData(Ep.extssRemoteAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		Ep.GetExtStrData(Ep.extssClientCode, SalesOrg);
		State |= stEpDefined;
	}
	State |= stInited;
}

int SfaHeineken::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL SfaHeineken::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		static_cast<UHTT_DESTROYRESULT>(P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

int SfaHeineken::ReceiveOrders()
{
	int    ok = -1;
	LDATE  query_date = ZERODATE;
	Reference * p_ref = PPRef;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSCollection <OrderEntry> result_list;
	ReplyInfo reply_info;
	SFAHEINEKENGETORDERS_PROC func = 0;
	SString tech_buf;
	TSCollection <SfaHeinekenOrderStatusEntry> order_status_list;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_S, &msg_buf, tech_buf.cptr());
		PPWaitMsg(msg_buf);
	}
	THROW_PP_S(Ep.Fb.LocCodeTagID, PPERR_SUPPLIX_UNDEFLOCCTAG, ArName.cptr());
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENGETORDERS_PROC>(P_Lib->GetProcAddr("SfaHeineken_GetOrders")));
	sess.Setup(SvcUrl, UserName, Password);
	// @v10.0.1 {
	if(checkdate(P.ExpPeriod.low) && P.ExpPeriod.upp == P.ExpPeriod.low) {
		query_date = P.ExpPeriod.low;
	}
	// } @v10.0.1
	p_result = func(sess, query_date, 0/*demo*/);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	ParseOrdersPacket(p_result, &reply_info, result_list);
	DestroyResult(reinterpret_cast<void **>(&p_result));
	if(result_list.getCount()) {
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			PPIDArray loc_list;
			PPIDArray psn_list;
			SString bill_uuid_text;
			for(uint srcordidx = 0; srcordidx < result_list.getCount(); srcordidx++) {
				const OrderEntry * p_src_pack = result_list.at(srcordidx);
				LocationTbl::Rec loc_rec;
				PersonTbl::Rec psn_rec;
				PPID   wh_id = 0;
				PPID   dlvr_loc_id = 0; // Ид адреса доставки
				PPID   ar_id = 0; // Ид заказчика (статья)
				p_src_pack->Uuid.ToStr(S_GUID::fmtIDL, bill_uuid_text);
				if(p_src_pack->WarehouseID) {
					temp_buf.Z().Cat(p_src_pack->WarehouseID);
					if(p_ref->Ot.SearchObjectsByStr(PPOBJ_LOCATION, Ep.Fb.LocCodeTagID, temp_buf, &loc_list) > 0) {
						for(uint i = 0; !wh_id && i < loc_list.getCount(); i++) {
							if(LocObj.Fetch(loc_list.get(i), &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE)
								wh_id = loc_list.get(i);
						}
					}
				}
				if(!wh_id) {
					temp_buf.Z().Cat(p_src_pack->WarehouseID);
					R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_WAREHOUSENTAG, &msg_buf, bill_uuid_text.cptr(), temp_buf.cptr()));
				}
				if(p_src_pack->ForeignDlvrAddrID) {
					temp_buf.Z().Cat(p_src_pack->ForeignDlvrAddrID);
					if(p_ref->Ot.SearchObjectsByStr(PPOBJ_LOCATION, Ep.Fb.LocCodeTagID, temp_buf, &loc_list) > 0) {
						for(uint i = 0; !dlvr_loc_id && i < loc_list.getCount(); i++) {
							if(LocObj.Fetch(loc_list.get(i), &loc_rec) > 0 && loc_rec.Type == LOCTYP_ADDRESS) {
								dlvr_loc_id = loc_list.get(i);
								if(loc_rec.OwnerID && PsnObj.Fetch(loc_rec.OwnerID, &psn_rec) > 0) {
									ArObj.P_Tbl->PersonToArticle(psn_rec.ID, op_rec.AccSheetID, &ar_id);
								}
								if(!ar_id) {
									temp_buf.Space().CatParStr(p_src_pack->ForeignDlvrAddrText);
									R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLLTDLOCTOARBYTAG, &msg_buf, temp_buf.cptr()));
								}
							}
						}
					}
				}
				if(!dlvr_loc_id) {
					temp_buf.Z().Cat(p_src_pack->ForeignDlvrAddrID).Space().CatParStr(p_src_pack->ForeignDlvrAddrText);
					R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNTAG, &msg_buf, bill_uuid_text.cptr(), temp_buf.cptr()));
				}
				if(wh_id && dlvr_loc_id && ar_id) {
					int    skip = 0;
					PPBillPacket pack;
					//BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;
					THROW(pack.CreateBlank2(acfg.Hdr.OpID, checkdate(p_src_pack->Dtm.d) ? p_src_pack->Dtm.d : getcurdate_(), wh_id, 1));
					pack.Rec.DueDate = checkdate(p_src_pack->DlvrDtm.d) ? p_src_pack->DlvrDtm.d : ZERODATE;
					//
					sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
					if(!pack.SetupObject(ar_id, sob)) {
						R_Logger.LogLastError();
					}
					else {
						PPIDArray ex_bill_id_list;
						PPID   agent_id = 0;
						const  PPID agent_acs_id = GetAgentAccSheet();
						pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
						if(p_src_pack->UserID && Ep.Fb.CliCodeTagID && agent_acs_id) {
							temp_buf.Z().Cat(p_src_pack->UserID);
							if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, Ep.Fb.CliCodeTagID, temp_buf, &psn_list) > 0) {
								for(uint i = 0; !agent_id && i < psn_list.getCount(); i++) {
									if(PsnObj.Fetch(psn_list.get(i), &psn_rec) > 0) {
										ArObj.P_Tbl->PersonToArticle(psn_rec.ID, agent_acs_id, &agent_id);
									}
								}
							}
							if(agent_id) {
								pack.Ext.AgentID = agent_id;
							}
							else {
								// PPTXT_LOG_SUPPLIX_CLLTAGNTTOORD     "Не удалось сопоставить с заказом агента '@zstr'"
                                temp_buf.Space().CatParStr(p_src_pack->UserName);
								R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLLTAGNTTOORD, &msg_buf, temp_buf.cptr()));
							}
						}
						// @v11.1.12 STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
						pack.SMemo = p_src_pack->Memo; // @v11.1.12
						//if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &ex_bill_id, &ex_bill_rec) > 0) {
						if(p_ref->Ot.SearchObjectsByStrExactly(PPOBJ_BILL, PPTAG_BILL_EDIIDENT, bill_uuid_text, &ex_bill_id_list) > 0) {
							;
						}
						else {
							for(uint j = 0; !skip && j < p_src_pack->ItemList.getCount(); j++) {
								const OrderEntry::Item * p_src_item = p_src_pack->ItemList.at(j);
								if(p_src_item) {
									temp_buf.Z().Cat(p_src_item->SkuID);
									if(GObj.P_Tbl->SearchByArCode(P.SupplID, temp_buf, 0, &goods_rec) > 0) {
										PPTransferItem ti;
										THROW(ti.Init(&pack.Rec));
										double _qtty = labs(p_src_item->Qtty);
										ti.GoodsID = goods_rec.ID;
										ti.Quantity_ = _qtty;
										if(ti.Quantity_ != 0.0) {
											ti.Price = 0.0; // @todo
											THROW(pack.LoadTItem(&ti, 0, 0));
										}

									}
									else {
										temp_buf.Space().CatParStr(p_src_item->SkuName);
										R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNCODE, &msg_buf, bill_uuid_text.cptr(), temp_buf.cptr()));
										skip = 1;
									}
								}
							}
							if(!skip) {
								pack.SetupEdiAttributes(PPEDIOP_SALESORDER, "SFA-HEINEKEN", bill_uuid_text);
								pack.InitAmounts();
								THROW(P_BObj->TurnPacket(&pack, 1));
								{
									SfaHeinekenOrderStatusEntry * p_status_entry = order_status_list.CreateNewItem();
									THROW_SL(p_status_entry);
									p_status_entry->OrderUUID = p_src_pack->Uuid;
									p_status_entry->Status = 1; // accepted
								}
							}
						}
					}
				}
			}
			THROW(SendStatus(order_status_list));
		}
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::ReceiveGoods()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSCollection <GoodsEntry> result_list;
	ReplyInfo reply_info;
	SFAHEINEKENGETSKUASSORTIMENT_PROC func = 0;
	SString tech_buf;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_S, &msg_buf, tech_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENGETSKUASSORTIMENT_PROC>(P_Lib->GetProcAddr("SfaHeineken_GetSkuAssortiment")));
	sess.Setup(SvcUrl, UserName, Password);
	p_result = func(sess);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	ParseGoodsPacket(p_result, &reply_info, result_list);
	DestroyResult(reinterpret_cast<void **>(&p_result));
	CATCHZOK
	return ok;
}

int SfaHeineken::SendStatus(const TSCollection <SfaHeinekenOrderStatusEntry> & rList)
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString msg_buf;
	SString * p_result = 0;
	SFAHEINEKENSENDORDERSSTATUSES_PROC func = 0;
	SString tech_buf;
	if(rList.getCount()) {
		Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
		{
			PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_S, &msg_buf, tech_buf.cptr(), P.SupplID);
			PPWaitMsg(msg_buf);
		}
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = reinterpret_cast<SFAHEINEKENSENDORDERSSTATUSES_PROC>(P_Lib->GetProcAddr("SfaHeineken_SendOrdersStatuses")));
		sess.Setup(SvcUrl, UserName, Password);
		p_result = func(sess, rList);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DestroyResult(reinterpret_cast<void **>(&p_result));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::Helper_MakeDeliveryList(PPBillPacket & rPack, const StrAssocArray & rTiPosList, TSCollection <SfaHeinekenInvoice::DeliveryPosition> & rDeliveryList)
{
	int    ok = 1;
	long   tiiterpos = 0;
	SString temp_buf;
	PPTransferItem ti;
	PPBillPacket::TiItemExt tiext;
	for(TiIter tiiter(&rPack, ETIEF_UNITEBYGOODS, 0); rPack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
		tiiterpos++;
		uint   pos_list_item_pos = 0;
		if(rTiPosList.GetText(tiiterpos, temp_buf) > 0) {
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
				SfaHeinekenInvoice::DeliveryPosition * p_new_item = rDeliveryList.CreateNewItem();
				THROW_SL(p_new_item);
				p_new_item->SkuID = temp_buf.ToLong();
				p_new_item->Count = fabs(ti.Quantity_);
				double ratio = 0.0;
				if(GObj.TranslateGoodsUnitToBase(goods_rec, PPUNT_LITER, &ratio) > 0)
					p_new_item->Volume = fabs(ti.Quantity_) * ratio / 100.0; // гекталитры
				else
					p_new_item->Volume = 0.0;
				p_new_item->Amount = ti.CalcAmount();
			}
		}
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::Helper_MakeBillEntry(PPID billID, int outerDocType, TSCollection <SfaHeinekenInvoice> & rList, TSCollection <SfaHeinekenInvoice> & rToDeleteList)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	const  PPID bill_ack_tag_id = Ep.Fb.BillAckTagID; //NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	int    do_cancel = BIN(outerDocType < 0);
	SString temp_buf;
	SString msg_buf;
	outerDocType = labs(outerDocType);
	PPBillPacket pack;
	if(P_BObj->ExtractPacket(billID, &pack) > 0) {
		StrAssocArray ti_pos_list;
		PPTransferItem ti;
		PPBillPacket::TiItemExt tiext;
		long   tiiterpos = 0;
		for(TiIter tiiter(&pack, ETIEF_UNITEBYGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
			tiiterpos++;
			if(GObj.BelongToGroup(ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, ti.GoodsID, temp_buf.Z(), 0) > 0) {
				ti_pos_list.Add(tiiterpos, temp_buf, 0);
			}
		}
		SString bill_code;
		SString bill_ack_tag_value;
		pack.BTagL.GetItemStr(bill_ack_tag_id, bill_ack_tag_value);
		// @v11.1.12 BillCore::GetCode(bill_code = pack.Rec.Code);
		bill_code = pack.Rec.Code; // @v11.1.12 
		bill_code.Transf(CTRANSF_INNER_TO_UTF8);
		// @v11.1.12 if(strstr(pack.Rec.Memo, "#heineken-delete")) {
		if(pack.SMemo.Search("#heineken-delete", 0, 1, 0)) { // @v11.1.12
			SfaHeinekenInvoice * p_new_entry = rToDeleteList.CreateNewItem();
			THROW_SL(p_new_entry);
			p_new_entry->Code = bill_code;
			p_new_entry->Dt = pack.Rec.Dt;
		}
		else if(ti_pos_list.getCount() && bill_ack_tag_value.IsEmpty()) {
			PPIDArray order_id_list;
			PPBillPacket order_pack;
			int    is_own_order = 0;
			const  PPID   dlvr_addr_id = pack.GetDlvrAddrID();
			long   foreign_dlvr_addr_id = 0;
			S_GUID  order_uuid;
			SString inner_order_code;
			SString bill_text;
			P_BObj->MakeCodeString(&pack.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName|PPObjBill::mcsAddOpName, bill_text);
			pack.GetOrderList(order_id_list);
			for(uint ordidx = 0; !is_own_order && ordidx < order_id_list.getCount(); ordidx++) {
				const PPID ord_id = order_id_list.get(ordidx);
				BillTbl::Rec ord_rec;
				if(ord_id && P_BObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, ord_id, PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("SFA-HEINEKEN")) {
						THROW(P_BObj->ExtractPacket(ord_id, &order_pack) > 0);
						if(order_pack.BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, temp_buf) > 0)
							order_uuid.FromStr(temp_buf);
						if(!order_uuid.IsZero())
							is_own_order = 1;
					}
					if(!is_own_order) {
						// @v11.1.12 BillCore::GetCode(inner_order_code = ord_rec.Code);
						inner_order_code = ord_rec.Code; // @v11.1.12 
					}
				}
			}
			if(order_uuid.IsZero() && inner_order_code.IsEmpty()) {
				(inner_order_code = bill_code).CatChar('-').Cat("ORD"); // @v10.0.08
				// @v10.0.08 R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_EBILLHASNTORDER, &msg_buf, bill_text.cptr()));
			}
			/* @v10.0.08 else */if(!dlvr_addr_id) {
				R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_EBILLHASNTDLVRLOC, &msg_buf, bill_text.cptr()));
			}
			else {
				PPID foreign_warehouse_id = 0;
				if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, dlvr_addr_id, Ep.Fb.LocCodeTagID, temp_buf) > 0)
					foreign_dlvr_addr_id = temp_buf.ToLong();
				if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, pack.Rec.LocID, Ep.Fb.LocCodeTagID, temp_buf) > 0)
					foreign_warehouse_id = temp_buf.ToLong();
				SfaHeinekenInvoice * p_new_entry = rList.CreateNewItem();
				THROW_SL(p_new_entry);
				p_new_entry->Code = bill_code;
				p_new_entry->Dt = checkdate(pack.Ext.InvoiceDate) ? pack.Ext.InvoiceDate : pack.Rec.Dt;
				if(is_own_order) {
					// TSCollection <SfaHeinekenOrderDelivery> OrderList; // Если доставка по заказу из системы Jeans
					SfaHeinekenInvoice::OrderDelivery * p_o = p_new_entry->OrderList.CreateNewItem();
					THROW_SL(p_o);
					p_o->OrderUuid = order_uuid;
					THROW(Helper_MakeDeliveryList(pack, ti_pos_list, p_o->DeliveryList));
				}
				else if(foreign_dlvr_addr_id) {
					// TSCollection <SfaHeinekenDistributorDelivery> DistributorDeliveryList; // Если доставка вне заказа из системы Jeans, по ТТ из системы Jeans
					SfaHeinekenInvoice::DistributorDelivery * p_o = p_new_entry->DistributorDeliveryList.CreateNewItem();
					THROW_SL(p_o);
					(p_o->InnerOrderCode = inner_order_code).Transf(CTRANSF_INNER_TO_UTF8);
					p_o->SalePointID = foreign_dlvr_addr_id;
					THROW(Helper_MakeDeliveryList(pack, ti_pos_list, p_o->DeliveryList));
				}
				else {
					// TSCollection <SfaHeinekenSalePointDelivery> DistributorSalePointDeliveryList; // Если доставка вне заказа из системы Jeans, по торговой точке, остсутствующей в системе Jeans
					SfaHeinekenInvoice::SalePointDelivery * p_o = p_new_entry->DistributorSalePointDeliveryList.CreateNewItem();
					THROW_SL(p_o);
					if(outerDocType == 6) {
						PPObjTag tag_obj;
						PPID   rordn_tag_id = 0;
						if(tag_obj.FetchBySymb("HEINEKEN-RORDN", &rordn_tag_id) > 0 && pack.BTagL.GetItemStr(rordn_tag_id, temp_buf) > 0)
							(p_o->DistributorOrderID = temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
						else
							p_o->DistributorOrderID = "HRORD-000"; // @stub
					}
					else
						(p_o->InnerOrderCode = inner_order_code).Transf(CTRANSF_INNER_TO_UTF8);
					p_o->InnerDlvrLocID = dlvr_addr_id;
					p_o->ForeignLocID = foreign_warehouse_id; // @v10.0.08
					{
						PPLocationPacket loc_pack;
						THROW(LocObj.GetPacket(dlvr_addr_id, &loc_pack) > 0);
						temp_buf.Z();
						GetArticleName(pack.Rec.Object, temp_buf);
						if(loc_pack.Name[0]) {
							temp_buf.CatDiv('-', 1).Cat(loc_pack.Name);
						}
						p_o->DlvrLocName = temp_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8);
						LocObj.P_Tbl->GetAddress(loc_pack, 0, temp_buf);
						p_o->DlvrLocAddr = temp_buf.Strip().Transf(CTRANSF_INNER_TO_UTF8);
					}
					THROW(Helper_MakeDeliveryList(pack, ti_pos_list, p_o->DeliveryList));
				}
			}
		}
		else {
			if(bill_ack_tag_value.NotEmpty()) {
				SfaHeinekenInvoice * p_new_entry = rToDeleteList.CreateNewItem();
				THROW_SL(p_new_entry);
				p_new_entry->Code = bill_code;
				p_new_entry->Dt = pack.Rec.Dt;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <SfaHeinekenInvoice> & rList, TSCollection <SfaHeinekenInvoice> & rToDeleteList)
{
	int    ok = -1;
	const  PPID bill_ack_tag_id = Ep.Fb.BillAckTagID;
	if(opID && outerDocType >= 0) {
		Reference * p_ref = PPRef;
		SString temp_buf;
		SString isales_code;
		S_GUID test_uuid;
		PPViewBill b_view;
		BillTbl::Rec bill_rec;
		PPIDArray force_bill_list; // Список документов которые надо послать снова
		BillFilt b_filt;
		BillViewItem view_item;
		PPBillPacket pack;
		SysJournal * p_sj = DS.GetTLA().P_SysJ;
		b_filt.OpID = opID;
		b_filt.LocList = P.LocList;
		if(outerDocType == 6) {
			b_filt.ObjectID = P.SupplID;
		}
		b_filt.Period = P.ExpPeriod;
		SETIFZ(b_filt.Period.low, encodedate(1, 1, 2016));
		THROW(b_view.Init_(&b_filt));
		for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
			if(!force_bill_list.bsearch(view_item.ID)) {
				int    dont_send = 0;
				if(outerDocType == 6 && !P_BObj->CheckStatusFlag(view_item.StatusID, BILSTF_READYFOREDIACK)) {
					dont_send = 1; // Статус не позволяет отправку
				}
				/* @v10.3.3 (функция Helper_MakeBillEntry разберется) else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, view_item.ID, bill_ack_tag_id, temp_buf) > 0) {
					dont_send = 1; // не отправляем документы, которые уже были отправлены ранее
				}*/
				if(!dont_send) {
					if(!Helper_MakeBillEntry(view_item.ID, outerDocType, rList, rToDeleteList))
						R_Logger.LogLastError();
				}
			}
		}
		{
			for(uint i = 0; i < force_bill_list.getCount(); i++) {
				const PPID force_bill_id = force_bill_list.get(i);
				if(P_BObj->Search(force_bill_id, &bill_rec) > 0) {
					if(outerDocType == 6 && !P_BObj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK)) {
						// Статус не позволяет отправку
					}
					else {
						if(!Helper_MakeBillEntry(bill_rec.ID, outerDocType, rList, rToDeleteList))
							R_Logger.LogLastError();
					}
				}
			}
		}
	}
    CATCHZOK
	return ok;
}

int SfaHeineken::SendDebts()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSCollection <SfaHeinekenDebetEntry> list;
	SFAHEINEKENSENDALLCONTRAGENTDEBET_PROC func = 0;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_S, &msg_buf, temp_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENSENDALLCONTRAGENTDEBET_PROC>(P_Lib->GetProcAddr("SfaHeineken_SendAllContragentDebet")));
	{
		Reference * p_ref = PPRef;
		PPViewDebtTrnovr debt_view;
		DebtTrnovrViewItem debt_item;
		DebtTrnovrFilt debt_filt;
		debt_filt.AccSheetID = GetSellAccSheet();
		debt_filt.Flags |= (DebtTrnovrFilt::fSkipPassive|DebtTrnovrFilt::fDebtOnly);
		debt_filt.Sgb.S = SubstGrpBill::sgbDlvrLoc;
		THROW(debt_view.Init_(&debt_filt));
		for(debt_view.InitIteration(PPViewDebtTrnovr::OrdByDefault); debt_view.NextIteration(&debt_item) > 0;) {
			PPID   dlvr_addr_id = debt_item.ID_;
			if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, dlvr_addr_id, Ep.Fb.LocCodeTagID, temp_buf) > 0) {
				PPID foreign_dlvr_addr_id = temp_buf.ToLong();
				if(foreign_dlvr_addr_id > 0) {
					SfaHeinekenDebetEntry * p_new_entry = list.CreateNewItem();
					THROW_SL(p_new_entry);
					p_new_entry->ContragentID = foreign_dlvr_addr_id;
					p_new_entry->DebetSum = debt_item.Debt;
					p_new_entry->DebetLimit = 0.0;
				}
			}
		}
	}
	sess.Setup(SvcUrl, UserName, Password);
	if(list.getCount()) {
		p_result = func(sess, list);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DestroyResult(reinterpret_cast<void **>(&p_result));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::SendReceipts()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSCollection <SfaHeinekenInvoice> list;
	TSCollection <SfaHeinekenInvoice> to_delete_list;
	SFAHEINEKENSENDSELLIN_PROC func = 0; // SfaHeineken_SendSellin
	SFAHEINEKENDELETESELLIN_PROC func_delete = 0;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_S, &msg_buf, temp_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENSENDSELLIN_PROC>(P_Lib->GetProcAddr("SfaHeineken_SendSellin")));
    {
		PPAlbatrossConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.EdiDesadvOpID && GetOpData(acfg.Hdr.EdiDesadvOpID, &op_rec) > 0) {
			if(op_rec.OpTypeID == PPOPT_DRAFTRECEIPT) {
				PPObjOprKind op_obj;
				PPDraftOpEx doe;
				if(op_obj.GetDraftExData(op_rec.ID, &doe) > 0 && doe.WrOffOpID) {
					THROW(Helper_MakeBillList(doe.WrOffOpID, 6, list, to_delete_list));
				}
			}
            else if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
				THROW(Helper_MakeBillList(acfg.Hdr.EdiDesadvOpID, 6, list, to_delete_list));
            }
		}
    }
	if(list.getCount()) {
		sess.Setup(SvcUrl, UserName, Password);
		p_result = func(sess, list);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DestroyResult(reinterpret_cast<void **>(&p_result));
		ok = 1;
	}
	// @v10.4.9 {
	if(to_delete_list.getCount()) {
		sess.Setup(SvcUrl, UserName, Password);
		THROW_SL(func_delete = reinterpret_cast<SFAHEINEKENDELETESELLIN_PROC>(P_Lib->GetProcAddr("SfaHeineken_DeleteSellin")));
		for(uint i = 0; i < to_delete_list.getCount(); i++) {
			const SfaHeinekenInvoice * p_item = to_delete_list.at(i);
			if(p_item && p_item->Code.NotEmpty() && checkdate(p_item->Dt)) {
				p_result = func_delete(sess, p_item->Code, p_item->Dt);
				THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	// } @v10.4.9 
	CATCHZOK
	return ok;
}

int SfaHeineken::SendSales()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSCollection <SfaHeinekenInvoice> list;
	TSCollection <SfaHeinekenInvoice> to_delete_list;
	SFAHEINEKENSENDSELLOUT_PROC func = 0;
	SFAHEINEKENDELETESELLOUT_PROC func_delete = 0;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_S, &msg_buf, temp_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENSENDSELLOUT_PROC>(P_Lib->GetProcAddr("SfaHeineken_SendSellout")));
	sess.Setup(SvcUrl, UserName, Password);
	Helper_MakeBillList(Ep.ExpendOp, 1, list, to_delete_list);
	if(list.getCount()) {
		p_result = func(sess, list);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DestroyResult(reinterpret_cast<void **>(&p_result));
		ok = 1;
	}
	if(to_delete_list.getCount()) {
		THROW_SL(func_delete = reinterpret_cast<SFAHEINEKENDELETESELLOUT_PROC>(P_Lib->GetProcAddr("SfaHeineken_DeleteSellout")));
		for(uint i = 0; i < to_delete_list.getCount(); i++) {
			const SfaHeinekenInvoice * p_item = to_delete_list.at(i);
			if(p_item && p_item->Code.NotEmpty() && checkdate(p_item->Dt)) {
				p_result = func_delete(sess, p_item->Code, p_item->Dt);
				THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
				DestroyResult(reinterpret_cast<void **>(&p_result));
			}
		}
	}
	CATCHZOK
	return ok;
}

int SfaHeineken::SendStocks()
{
	int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString * p_result = 0;
	TSVector <SfaHeinekenWarehouseBalanceEntry> list;
	SFAHEINEKENSENDWAREHOUSEBALANCE_PROC func = 0;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
	{
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_S, &msg_buf, temp_buf.cptr(), P.SupplID);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = reinterpret_cast<SFAHEINEKENSENDWAREHOUSEBALANCE_PROC>(P_Lib->GetProcAddr("SfaHeineken_SendWarehousesBalance")));
	sess.Setup(SvcUrl, UserName, Password);
	if(Ep.Fb.LocCodeTagID) {
		Reference * p_ref = PPRef;
		PPViewGoodsRest gr_view;
		GoodsRestFilt gr_filt;
		GoodsRestViewItem gr_item;
		SString ar_code;
		gr_filt.Date = P.ExpPeriod.upp; // @v10.0.07 ZERODATE-->P.ExpPeriod.upp
		gr_filt.GoodsGrpID = Ep.GoodsGrpID;
		gr_filt.LocList = P.LocList;
		gr_filt.Flags |= GoodsRestFilt::fEachLocation;
		THROW(gr_view.Init_(&gr_filt));
		for(gr_view.InitIteration(PPViewGoodsRest::OrdByDefault); gr_view.NextIteration(&gr_item) > 0;) {
			const PPID loc_id = gr_item.LocID;
			uint   _pos = 0;
			int    skip_goods = 0;
			int32  ar_code_pack = 0;
			if(gr_item.Rest > 0.0 && GObj.P_Tbl->GetArCode(P.SupplID, gr_item.GoodsID, ar_code, &ar_code_pack) > 0) {
				SfaHeinekenWarehouseBalanceEntry new_entry;
				new_entry.ForeignGoodsID = ar_code.ToLong();
				if(new_entry.ForeignGoodsID > 0) {
					if(p_ref->Ot.GetTagStr(PPOBJ_LOCATION, gr_item.LocID, Ep.Fb.LocCodeTagID, temp_buf) > 0) {
						new_entry.ForeignLocID = temp_buf.ToLong();
						if(new_entry.ForeignLocID > 0) {
							new_entry.Rest = R0i(gr_item.Rest);
							// @v10.1.1 Специальная поправка для нештатного случая учета кегов (Unit = LITER, PhUnit = LITER, gse.Packege = keg's volume) {
							Goods2Tbl::Rec goods_rec;
							if(GObj.Fetch(gr_item.GoodsID, &goods_rec) > 0) {
								double rt1 = 0.0;
								double rt2 = 0.0;
								int tr1 = UObj.TranslateToBase(goods_rec.UnitID, PPUNT_LITER, &rt1);
								int tr2 = UObj.TranslateToBase(goods_rec.PhUnitID, PPUNT_LITER, &rt2);
								if(tr1 > 0 && tr2 > 0 && rt1 == rt2) {
									GoodsStockExt gse;
									if(GObj.GetStockExt(goods_rec.ID, &gse, 1) > 0 && gse.Package > 0.0) {
										new_entry.Rest = R0i(gr_item.Rest / gse.Package);
									}
								}
							}
							// } @v10.1.1
							THROW_SL(list.insert(&new_entry));
						}
					}
				}
			}
		}
	}
	if(list.getCount()) {
		p_result = func(sess, list);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DestroyResult(reinterpret_cast<void **>(&p_result));
		ok = 1;
	}
	CATCHZOK
	return ok;
}
//
// @v11.5.2 
// Интеграция с сервисом Газпром-нефть
//
struct GazpromNeftGoodsPacket {
	GazpromNeftGoodsPacket() : Ident(0), Weight(0.0), Capacity(0.0), IsActive(true), PackingTypeId(0)
	{
		Name[0] = 0;
		memzero(Reserve, sizeof(Reserve));
		PackingTypeName[0] = 0;
		PackagingTypeName[0] = 0;
	}
	GazpromNeftGoodsPacket(const GazpromNeftGoodsPacket & rS)
	{
		Copy(rS);
	}
	GazpromNeftGoodsPacket & FASTCALL Copy(const GazpromNeftGoodsPacket & rS)
	{
		Ident = rS.Ident;
		Weight = rS.Weight;
		Capacity = rS.Capacity;
		IsActive = rS.IsActive;
		STRNSCPY(Name, rS.Name);
		STRNSCPY(PackingTypeName, rS.PackingTypeName);
		PackingTypeId = rS.PackingTypeId;
		STRNSCPY(PackagingTypeName, rS.PackagingTypeName);
		PackagingTypeUuid = rS.PackagingTypeUuid;
		return *this;
	}
	GazpromNeftGoodsPacket & FASTCALL operator = (const GazpromNeftGoodsPacket & rS)
	{
		return Copy(rS);
	}
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
	{
		int    ok = 1;
		THROW(pSCtx->Serialize(dir, Ident, rBuf));
		THROW(pSCtx->Serialize(dir, Weight, rBuf));
		THROW(pSCtx->Serialize(dir, Capacity, rBuf));
		THROW(pSCtx->Serialize(dir, IsActive, rBuf));
		THROW(pSCtx->Serialize(dir, Name, sizeof(Name), rBuf));
		THROW(pSCtx->Serialize(dir, PackingTypeName, sizeof(PackingTypeName), rBuf));
		THROW(pSCtx->Serialize(dir, PackingTypeId, rBuf));
		THROW(pSCtx->Serialize(dir, PackagingTypeName, sizeof(PackagingTypeName), rBuf));
		THROW(pSCtx->Serialize(dir, PackagingTypeUuid, rBuf));
		CATCHZOK
		return ok;
	}

	int64  Ident;
	char   Name[256];
	double Weight;
	double Capacity;
	bool   IsActive;
	uint8  Reserve[3]; // @alignment
	//
	char   PackingTypeName[48];
	int    PackingTypeId;
	char   PackagingTypeName[48];
	S_GUID PackagingTypeUuid;
};

struct GazpromNeftClientPacket {
	GazpromNeftClientPacket() : PersonID(0), DlvrLocID(0), IsBlocked(false)
	{
		INN[0] = 0;
		KPP[0] = 0;
	}
	PPID   PersonID;
	PPID   DlvrLocID;
	S_GUID Uuid;
	bool   IsBlocked;  // @v11.5.9
	uint8  Reserve[3]; // @v11.5.9 @alignment
	SString Status;    // @v11.5.9
	SString Name;
	SString Address;
	SString ManagerName;
	char   INN[16];
	char   KPP[16];
};

struct GazpromNeftBillPacket {
	GazpromNeftBillPacket() : ID(0), Dtm(ZERODATETIME), GpnDtm(ZERODATETIME)
	{
		Code[0] = 0;
		GpnCode[0] = 0;
	}
	struct Position { // @flat
		Position() : GoodsID(0), ProductId(0), Qtty(0.0), Volume(0.0), Weight(0.0), Cost(0.0), CostWoVat(0.0)
		{
			UnitName[0] = 0;
		}
		PPID   GoodsID;
		S_GUID GoodsUuid;
		int64  ProductId;
		char   UnitName[48];
		double Qtty;
		double Volume;
		double Weight; // @v11.6.4
		double Cost;
		double CostWoVat;
	};
	PPID   ID;
	S_GUID Uuid;
	char   Code[32];
	char   GpnCode[32]; // Для приходных документов: номер входящей накладной
	LDATETIME Dtm;
	LDATETIME GpnDtm; // Для приходных документов: дата входящей накладной
	GazpromNeftClientPacket Client;
	TSVector <Position> ItemList;
};

class GazpromNeft : public PrcssrSupplInterchange::ExecuteBlock {
	enum {
		stInited     = 0x0001,
		stEpDefined  = 0x0002
	};
public:
	// https://gateways.suds.gazpromneft-sm.ru
	GazpromNeft(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	~GazpromNeft();
	int    Init();
	int    Test();
	int    Auth();
	int    GetWarehouses();
	int    GetProducts(bool useStorage);
	int    GetClients();
	int    SendSellout();
	int    SendSellin();
	int    SendRest();
private:
	enum {
		qUndef = 0,
		qAuthLogin,
		qAuthExtTok,
		qGetWarehouses,
		qGetProducts,
		qGetClients,
		qSendSellout,
		qSendSellin,
		qSendRest
	};
	struct GoodsStorageHeader {
		GoodsStorageHeader() : CRC(0)
		{
			memcpy(Signature, "GNGS", 4);
			MEMSZERO(Reserve);
		}
		char   Signature[4]; // "GNGS"
		uint32 CRC;
		uint8  Reserve[24];
	};
	SString & MakeTargetUrl_(int query, int * pReq/*SHttpProtocol::reqXXX*/, SString & rResult) const;
	SString & MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf);
	
	int    GetGoodsList(PPIDArray & rList);
	int    GetWarehouseUuid(PPID locID, S_GUID & rUuid);
	int    GetGoodsStoreFileName(SString & rBuf) const;
	int    StoreGoods(TSCollection <GazpromNeftGoodsPacket> & rList);
	int    RestoreGoods(TSCollection <GazpromNeftGoodsPacket> & rList);
	const  GazpromNeftGoodsPacket * SearchGoodsEntry(int64 ident) const;
	int    Helper_MakeBillList(PPID opID, int outerDocType, PPID locID, TSCollection <GazpromNeftBillPacket> & rList);
	int    Helper_MakeBillEntry(PPID billID, PPBillPacket * pBp, int outerDocType, TSCollection <GazpromNeftBillPacket> & rList);

	enum {
		stGoodsListInited = 0x0001
	};
	uint   State;
	SString SvcUrl;
	SString UserName;
	SString Password;
	SString AccessToken;
	long   AcsTokExpirySec;
	PPGlobalServiceLogTalkingHelper Lth;
	TSCollection <GazpromNeftGoodsPacket> GoodsList;
	PPLogger & R_Logger;
};

GazpromNeft::GazpromNeft(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), State(0), AcsTokExpirySec(0),
	Lth(PPFILNAM_MERCAPP_LOG), R_Logger(rLogger)
{
	PPGetFilePath(PPPATH_LOG, "gazpromneft.log", LogFileName);
}

GazpromNeft::~GazpromNeft()
{
}
	
int GazpromNeft::Init()
{
	int    ok = 1;
	State = 0;
	SvcUrl.Z();
	UserName.Z();
	Password.Z();
	{
		Ep.GetExtStrData(Ep.extssRemoteAddr, SvcUrl);
		Ep.GetExtStrData(Ep.extssAccsName, UserName);
		Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		State |= stEpDefined;
	}
	State |= stInited;
	return ok;
}

int GazpromNeft::Test()
{
	int    ok = -1;
	Auth();
	//GetClients();
	GetProducts(true);
	SendSellin();
	SendSellout();
	SendRest();
	GetWarehouses();
	return ok;
}

SString & GazpromNeft::MakeTargetUrl_(int query, int * pReq/*SHttpProtocol::reqXXX*/, SString & rResult) const
{
	(rResult = "https").Cat("://").Cat("gateways").DotCat("suds").DotCat("gazpromneft-sm").DotCat("ru");
	int    req = SHttpProtocol::reqUnkn;
	const char * p_path = 0;
	switch(query) {
		case qAuthLogin: 
			req = SHttpProtocol::reqPost;
			p_path = "login-api/api/v1/Auth/login";
			break;
		case qAuthExtTok: 
			req = SHttpProtocol::reqPost;
			p_path = "login-api/api/v1/Auth/extended-token";
			break;
		case qGetWarehouses: 
			req = SHttpProtocol::reqGet;
			p_path = "distribution-api/api/v1/Distributions/warehouses"; 
			break;
		case qGetProducts: 
			req = SHttpProtocol::reqGet;
			p_path = "product-api/api/v1/Products/integration";
			break;
		case qGetClients: 
			req = SHttpProtocol::reqPost;
			p_path = "client-api/api/v1/Clients/GetFilteredList";
			break;
		case qSendSellout:
			req = SHttpProtocol::reqPost;
			p_path = "sales-api/api/v2/Sales/sellout";
			break;
		case qSendSellin:
			req = SHttpProtocol::reqPost;
			p_path = "sales-api/api/v2/Sales/sellin";
			break;
		case qSendRest:
			req = SHttpProtocol::reqPost;
			p_path = "sales-api/api/v1/Sales/warehousebalances";
			break;
	}
	if(!isempty(p_path)) {
		rResult.SetLastDSlash().Cat(p_path);
	}
	ASSIGN_PTR(pReq, req);
	return rResult;
}

SString & GazpromNeft::MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf)
{
	StrStrAssocArray hdr_flds;
	SETIFZ(pHdrFlds, &hdr_flds);
	{
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrCacheControl, "no-cache");
		//SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAcceptLang, "ru");
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAccept, "application/json");
	}
	if(!isempty(pToken)) {
		SString temp_buf;
		(temp_buf = "Bearer").Space().Cat(pToken);
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAuthorization, temp_buf);
	}
	SHttpProtocol::PutHeaderFieldsIntoString(*pHdrFlds, rBuf);
	return rBuf;
}

int GazpromNeft::Auth()
{
	int    ok = -1;
	// 1.	Авторизация                    POST /login-api/api/v1/Auth/login
	// 2.	Получение расширенного токена  POST /login-api/api/v1/Auth/extended-token
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString hdr_buf;
	SBuffer ack_buf;
	SString token_type;
	int   req = SHttpProtocol::reqUnkn;
	bool   is_error = false;
	{
		InetUrl url(MakeTargetUrl_(qAuthLogin, &req, url_buf));
		ScURL c;
		StrStrAssocArray hdr_flds;
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, "application/json");
		MakeHeaderFields(0, &hdr_flds, hdr_buf);
		{
			/*
				{
					"userName": "string", -- логин
					"password": "string" -- пароль
				}
			*/
			// extssAccsName
			SJson json_req(SJson::tOBJECT);
			json_req.InsertString("userName", UserName);
			json_req.InsertString("password", Password);
			THROW_SL(json_req.ToStr(req_buf));
		}
		{
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, temp_buf);
					SJson * p_js_reply = SJson::Parse(temp_buf);
					if(p_js_reply) {
						if(p_js_reply->IsObject()) {
							for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
								if(p_itm->Text.IsEqiAscii("accessToken")) {
									if(p_itm->P_Child) {
										(AccessToken = p_itm->P_Child->Text).Unescape();
									}
								}
								else if(p_itm->Text.IsEqiAscii("tokenType")) {
									if(p_itm->P_Child) {
										(token_type = p_itm->P_Child->Text).Unescape();
									}
								}
								else if(p_itm->Text.IsEqiAscii("expiresIn")) {
									if(p_itm->P_Child) {
										AcsTokExpirySec = p_itm->P_Child->Text.ToLong();
									}
								}
								else if(p_itm->Text.IsEqiAscii("refeshToken")) {
									// ???
								}
							}							
						}
						ZDELETE(p_js_reply);
					}
						
					//Lth.Log("rep", 0, temp_buf);
					//if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
						//ok = 1;
				}
			}
		}
	}
	if(!is_error) {
		InetUrl url(MakeTargetUrl_(qAuthExtTok, &req, url_buf));
		ScURL c;
		StrStrAssocArray hdr_flds;
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
		//SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrAccept, "application/json");
		MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
		{
			/*
				{
					"accessToken":"eyJh…", -- токен для дальнейшей работы с API
					"tokenType": "Bearer",
					"expiresIn": "14400",
					"refeshToken": " eyJh…"
				}
			*/
			// extssAccsName
			SJson json_req(SJson::tOBJECT);
			json_req.InsertString("accessToken", (temp_buf = AccessToken).Escape());
			json_req.InsertString("tokenType", token_type);
			json_req.InsertInt("expiresIn", AcsTokExpirySec);
			json_req.InsertString("refeshToken", /*(temp_buf = AccessToken).Escape()*/"");
			THROW_SL(json_req.ToStr(req_buf));
		}
		{
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					SString access_token2;
					SString refresh_token;
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, temp_buf);
					SJson * p_js_reply = SJson::Parse(temp_buf);
					if(p_js_reply) {
						if(p_js_reply->IsObject()) {
							for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
								if(p_itm->Text.IsEqiAscii("accessToken")) {
									if(p_itm->P_Child) {
										(access_token2 = p_itm->P_Child->Text).Unescape();
									}
								}
								else if(p_itm->Text.IsEqiAscii("tokenType")) {
									if(p_itm->P_Child) {
										//(temp_buf = p_itm->P_Child->Text).Unescape();
										//opaque_data = temp_buf;
										//ib_.Tokenize(pTa, temp_buf, ss);
									}
								}
								else if(p_itm->Text.IsEqiAscii("expiresIn")) {
									if(p_itm->P_Child) {
										AcsTokExpirySec = p_itm->P_Child->Text.ToLong();
									}
								}
								else if(p_itm->Text.IsEqiAscii("refeshToken")) {
									if(p_itm->P_Child) {
										(refresh_token = p_itm->P_Child->Text).Unescape();
									}
								}
							}							
						}
						ZDELETE(p_js_reply);
					}
					if(access_token2.NotEmpty()) {
						AccessToken = access_token2;
						ok = 1;
					}
					//Lth.Log("rep", 0, temp_buf);
					//if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
						//ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::GetWarehouses()
{
	int    ok = -1;
	// GET /distribution-api/api/v1/Distributions/warehouses
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString hdr_buf;
	SBuffer ack_buf;
	int   req = SHttpProtocol::reqUnkn;
	THROW(AccessToken.NotEmpty()); // @err
	{
		InetUrl url(MakeTargetUrl_(qGetWarehouses, &req, url_buf));
		ScURL c;
		PPGetFilePath(PPPATH_OUT, "gazpromneft-warehouse.txt", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		bool   more = false;
		StrStrAssocArray hdr_flds;
		MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
		{
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, /*req_buf,*/&wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, temp_buf);
					SJson * p_js_reply = SJson::Parse(temp_buf);
					if(p_js_reply) {
						if(p_js_reply->IsObject()) {
							for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
								if(p_itm->Text.IsEqiAscii("hasNext")) {
									if(SJson::IsTrue(p_itm->P_Child))
										more = true;
								}
								else if(p_itm->Text.IsEqiAscii("items")) {
									if(p_itm->P_Child && SJson::IsArray(p_itm->P_Child)) {
										for(const SJson * p_js_ware = p_itm->P_Child->P_Child; p_js_ware; p_js_ware = p_js_ware->P_Next) {
											if(SJson::IsObject(p_js_ware)) {
												struct WarehouseEntry {
													S_GUID Uuid;
													char   Number[16];
													char   Name[256];
													S_GUID DistributorUuid;
												};
												WarehouseEntry entry;
												MEMSZERO(entry);
												for(const SJson * p_js_fld = p_js_ware->P_Child; p_js_fld; p_js_fld = p_js_fld->P_Next) {
													if(p_js_fld->P_Child) {
														if(p_js_fld->Text.IsEqiAscii("id")) {
															entry.Uuid.FromStr(p_js_fld->P_Child->Text);
														}
														else if(p_js_fld->Text.IsEqiAscii("name")) {
															STRNSCPY(entry.Name, p_js_fld->P_Child->Text);
														}
														else if(p_js_fld->Text.IsEqiAscii("distributorId")) {
															entry.DistributorUuid.FromStr(p_js_fld->P_Child->Text);
														}
														else if(p_js_fld->Text.IsEqiAscii("number")) {
															STRNSCPY(entry.Number, p_js_fld->P_Child->Text);
														}
													}
												}
												if(!entry.Uuid.IsZero() && entry.Name[0]) {
													temp_buf.Z();
													temp_buf.Cat(entry.Uuid, S_GUID::fmtIDL).Tab().Cat(entry.Name).Tab().Cat(entry.Number).Tab().
														Cat(entry.DistributorUuid, S_GUID::fmtIDL);
													f_out.WriteLine(temp_buf.CR());
												}
											}
										}
									}
								}
							}							
						}
						ZDELETE(p_js_reply);
					}
					//Lth.Log("rep", 0, temp_buf);
					//if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
						//ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::GetProducts(bool useStorage)
{
	GoodsList.freeAll();
	int    ok = -1;
	// GET /product-api/api/v1/Products/integration
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString hdr_buf;
	SBuffer ack_buf;
	int    req = SHttpProtocol::reqUnkn;
	bool   is_error = false;
	//
	SString strg_file_name;
	GetGoodsStoreFileName(strg_file_name);
	int     storage_exists = 0;
	DateRange goods_query_period;
	goods_query_period.Z();
	if(useStorage && fileExists(strg_file_name)) {
		storage_exists = 1;
		LDATETIME mt;
		if(SFile::GetTime(strg_file_name, 0, 0, &mt) && diffdate(getcurdate_(), mt.d) <= 2) {
			if(RestoreGoods(GoodsList)) {
				goods_query_period.low = mt.d;
				State |= stGoodsListInited;
				ok = 2;
			}
		}
	}
	if(ok < 0) {
		THROW(AccessToken.NotEmpty()); // @err
		{
			InetUrl url(MakeTargetUrl_(qGetProducts, &req, url_buf));
			ScURL c;
			PPGetFilePath(PPPATH_OUT, "gazpromneft-goods.txt", temp_buf);
			SFile f_out(temp_buf, SFile::mWrite);
			bool  more = false; // Если true, то надо сделать новую итерацию для извлечения очередной порции данных
			int   page_no = 1;
			do {
				more = false;
				StrStrAssocArray hdr_flds;
				//url.SetQueryParam("Enabled", "true");
				//url.SetQueryParam("IsActive", "true");
				//url.SetQueryParam("OFFSET", "21");
				url.SetQueryParam("PAGE", temp_buf.Z().Cat(page_no));
				page_no++;
				url.SetQueryParam("SIZE", "1000");
				//url.SetQueryParam("Size", "100");
				MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
				{
					/*
						Параметр	Тип данных	Описание
						--------------------------------
						Enabled	Boolean	Фильтр по признаку «Enabled»
						IsActive	Boolean	Фильтр по признаку «Активный»
						CapacityFrom	number	Емкость от
						CapacityTo	Number	Емкость до
						Search	String	Фильтр по названию
						SegmentIds	Array[integer]	Фильтр по сегменту
						TypeIds	array[string]	Фильтр по типу
						BrandIds	array[string]	Фильтр по бренду
						SegmentConsumerIds	array[string]	Фильтр по потребительскому сегменту
						BrandLineIds	array[string]	Фильтр по линейке
						Sort	String	Сортировка по названию поля
						Direction	integer	Сортировка по возрастанию/убыванию
						Page	Integer	Номер страницы
						Size	Integer	Количество элементов на странице

					*/
					/*
					SJson json_req(SJson::tOBJECT);
					json_req.InsertBool("Enabled", true);
					json_req.InsertBool("IsActive", true);
					json_req.InsertInt("Page", 0);
					json_req.InsertInt("Size", 10000);
					THROW_SL(json_req.ToStr(req_buf));
					*/
				}
				{
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, /*req_buf,*/&wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, temp_buf);
							SJson * p_js_reply = SJson::Parse(temp_buf);
							if(p_js_reply) {
								if(p_js_reply->IsObject()) {
									{
										temp_buf.Z();
										temp_buf.Cat("id").Tab().Cat("name").Tab().Cat("weight").Tab().
											Cat("capacity").Tab().Cat("isactive").Tab().
											Cat("PackingTypeName").Tab().Cat("PackingTypeId").Tab().
											Cat("PackagingTypeName").Tab().Cat("PackagingTypeUuid");
										f_out.WriteLine(temp_buf.CR());
									}
									for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
										if(p_itm->Text.IsEqiAscii("hasNext")) {
											if(SJson::IsTrue(p_itm->P_Child))
												more = true;
										}
										else if(p_itm->Text.IsEqiAscii("items")) {
											if(p_itm->P_Child && SJson::IsArray(p_itm->P_Child)) {
												for(const SJson * p_js_ware = p_itm->P_Child->P_Child; p_js_ware; p_js_ware = p_js_ware->P_Next) {
													if(SJson::IsObject(p_js_ware)) {
														uint entry_idx = 0;
														GazpromNeftGoodsPacket * p_entry = GoodsList.CreateNewItem(&entry_idx);
														THROW_SL(p_entry);
														for(const SJson * p_js_fld = p_js_ware->P_Child; p_js_fld; p_js_fld = p_js_fld->P_Next) {
															if(p_js_fld->P_Child) {
																if(p_js_fld->Text.IsEqiAscii("id")) {
																	p_entry->Ident = p_js_fld->P_Child->Text.ToInt64();
																}
																else if(p_js_fld->Text.IsEqiAscii("name")) {
																	STRNSCPY(p_entry->Name, p_js_fld->P_Child->Text);
																}
																else if(p_js_fld->Text.IsEqiAscii("segmentId")) {
																}
																else if(p_js_fld->Text.IsEqiAscii("weight")) {
																	p_entry->Weight = p_js_fld->P_Child->Text.ToReal();
																}
																else if(p_js_fld->Text.IsEqiAscii("capacity")) {
																	p_entry->Capacity = p_js_fld->P_Child->Text.ToReal();
																}
																else if(p_js_fld->Text.IsEqiAscii("isActive")) {
																	p_entry->IsActive = SJson::GetBoolean(p_js_fld);
																}
																else if(p_js_fld->Text.IsEqiAscii("unitMeasurement")) { // object
																}
																else if(p_js_fld->Text.IsEqiAscii("brand")) { // object
																}
																else if(p_js_fld->Text.IsEqiAscii("segmentConsumer")) { // object
																}
																else if(p_js_fld->Text.IsEqiAscii("packingType")) { // object
																	for(const SJson * p_js_pt = p_js_fld->P_Child->P_Child; p_js_pt; p_js_pt = p_js_pt->P_Next) {
																		if(p_js_pt->P_Child) {
																			if(p_js_pt->Text.IsEqiAscii("value")) {
																				STRNSCPY(p_entry->PackingTypeName, p_js_pt->P_Child->Text);
																			}
																			else if(p_js_pt->Text.IsEqiAscii("id")) {
																				p_entry->PackingTypeId = p_js_pt->P_Child->Text.ToLong();
																			}
																		}
																	}
																}
																else if(p_js_fld->Text.IsEqiAscii("packagingType")) { // object
																	for(const SJson * p_js_pt = p_js_fld->P_Child->P_Child; p_js_pt; p_js_pt = p_js_pt->P_Next) {
																		if(p_js_pt->P_Child) {
																			if(p_js_pt->Text.IsEqiAscii("value")) {
																				STRNSCPY(p_entry->PackagingTypeName, p_js_pt->P_Child->Text);
																			}
																			else if(p_js_pt->Text.IsEqiAscii("id")) {
																				p_entry->PackagingTypeUuid.FromStr(p_js_pt->P_Child->Text);
																			}
																		}
																	}
																}
																else if(p_js_fld->Text.IsEqiAscii("status")) { // object
																}
																else if(p_js_fld->Text.IsEqiAscii("viscosity")) { // object
																}
																else if(p_js_fld->Text.IsEqiAscii("viscositySAE")) { // object
																}
															}
														}
														if(p_entry->Ident != 0 && p_entry->Name[0]) {
															temp_buf.Z();
															temp_buf.Cat(p_entry->Ident).Tab().Cat(p_entry->Name).Tab().Cat(p_entry->Weight, MKSFMTD(0, 6, 0)).Tab().
																Cat(p_entry->Capacity, MKSFMTD(0, 6, 0)).Tab().Cat(p_entry->IsActive ? "true" : "false").Tab().
																Cat(p_entry->PackingTypeName).Tab().Cat(p_entry->PackingTypeId).Tab().
																Cat(p_entry->PackagingTypeName).Tab().Cat(p_entry->PackagingTypeUuid, S_GUID::fmtIDL);
															f_out.WriteLine(temp_buf.CR());
														}
														else {
															GoodsList.atFree(entry_idx);
														}
													}
												}
											}
										}
									}							
								}
								ZDELETE(p_js_reply);
							}
							//Lth.Log("rep", 0, temp_buf);
							//if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
								//ok = 1;
						}
					}
				}
			} while(more);
		}
		State |= stGoodsListInited;
		ok = 1;
		if(useStorage) {
			StoreGoods(GoodsList);
		}
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::GetClients()
{
	int    ok = -1;
	// POST /client-api/api/v1/Clients/GetFilteredList
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString hdr_buf;
	SBuffer ack_buf;
	int    req = SHttpProtocol::reqUnkn;
	bool   is_error = false;
	THROW(AccessToken.NotEmpty()); // @err
	{
		InetUrl url(MakeTargetUrl_(qGetClients, &req, url_buf));
		ScURL c;
		PPGetFilePath(PPPATH_OUT, "gazpromneft-clients.txt", temp_buf);
		SFile f_out(temp_buf, SFile::mWrite);
		bool  more = false; // Если true, то надо сделать новую итерацию для извлечения очередной порции данных
		int   page_no = 1;
		do {
			more = false;
			StrStrAssocArray hdr_flds;
			//url.SetQueryParam("Enabled", "true");
			//url.SetQueryParam("IsActive", "true");
			//url.SetQueryParam("OFFSET", "21");
			{
				SJson js_query(SJson::tOBJECT);
				js_query.InsertInt("page", page_no);
				js_query.InsertInt("size", 1000);
				js_query.ToStr(req_buf);
			}
			//url.SetQueryParam("PAGE", temp_buf.Z().Cat(page_no));
			page_no++;
			//url.SetQueryParam("SIZE", "1000");
			//url.SetQueryParam("Size", "100");
			MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
			{
				SFile wr_stream(ack_buf.Z(), SFile::mWrite);
				Lth.Log("req", url_buf, req_buf);
				THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
				{
					SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
					if(p_ack_buf) {
						temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
						Lth.Log("rep", 0, temp_buf);
						SJson * p_js_reply = SJson::Parse(temp_buf);
						if(p_js_reply) {
							if(p_js_reply->IsObject()) {
								{
									temp_buf.Z();
									temp_buf.Cat("id").Tab().Cat("name").Tab().Cat("weight").Tab().
										Cat("capacity").Tab().Cat("isactive").Tab().
										Cat("PackingTypeName").Tab().Cat("PackingTypeId").Tab().
										Cat("PackagingTypeName").Tab().Cat("PackagingTypeUuid");
									f_out.WriteLine(temp_buf.CR());
								}
								for(const SJson * p_itm = p_js_reply->P_Child; p_itm; p_itm = p_itm->P_Next) {
									if(p_itm->Text.IsEqiAscii("hasNext")) {
										if(SJson::IsTrue(p_itm->P_Child))
											more = true;
									}
									else if(p_itm->Text.IsEqiAscii("items")) {
										if(p_itm->P_Child && SJson::IsArray(p_itm->P_Child)) {
											for(const SJson * p_js_ware = p_itm->P_Child->P_Child; p_js_ware; p_js_ware = p_js_ware->P_Next) {
												if(SJson::IsObject(p_js_ware)) {
													uint entry_idx = 0;
													GazpromNeftGoodsPacket * p_entry = GoodsList.CreateNewItem(&entry_idx);
													THROW_SL(p_entry);
													for(const SJson * p_js_fld = p_js_ware->P_Child; p_js_fld; p_js_fld = p_js_fld->P_Next) {
														if(p_js_fld->P_Child) {
															if(p_js_fld->Text.IsEqiAscii("id")) {
																p_entry->Ident = p_js_fld->P_Child->Text.ToInt64();
															}
															else if(p_js_fld->Text.IsEqiAscii("name")) {
																STRNSCPY(p_entry->Name, p_js_fld->P_Child->Text);
															}
															else if(p_js_fld->Text.IsEqiAscii("segmentId")) {
															}
															else if(p_js_fld->Text.IsEqiAscii("weight")) {
																p_entry->Weight = p_js_fld->P_Child->Text.ToReal();
															}
															else if(p_js_fld->Text.IsEqiAscii("capacity")) {
																p_entry->Capacity = p_js_fld->P_Child->Text.ToReal();
															}
															else if(p_js_fld->Text.IsEqiAscii("isActive")) {
																p_entry->IsActive = SJson::GetBoolean(p_js_fld);
															}
															else if(p_js_fld->Text.IsEqiAscii("unitMeasurement")) { // object
															}
															else if(p_js_fld->Text.IsEqiAscii("brand")) { // object
															}
															else if(p_js_fld->Text.IsEqiAscii("segmentConsumer")) { // object
															}
															else if(p_js_fld->Text.IsEqiAscii("packingType")) { // object
																for(const SJson * p_js_pt = p_js_fld->P_Child->P_Child; p_js_pt; p_js_pt = p_js_pt->P_Next) {
																	if(p_js_pt->P_Child) {
																		if(p_js_pt->Text.IsEqiAscii("value")) {
																			STRNSCPY(p_entry->PackingTypeName, p_js_pt->P_Child->Text);
																		}
																		else if(p_js_pt->Text.IsEqiAscii("id")) {
																			p_entry->PackingTypeId = p_js_pt->P_Child->Text.ToLong();
																		}
																	}
																}
															}
															else if(p_js_fld->Text.IsEqiAscii("packagingType")) { // object
																for(const SJson * p_js_pt = p_js_fld->P_Child->P_Child; p_js_pt; p_js_pt = p_js_pt->P_Next) {
																	if(p_js_pt->P_Child) {
																		if(p_js_pt->Text.IsEqiAscii("value")) {
																			STRNSCPY(p_entry->PackagingTypeName, p_js_pt->P_Child->Text);
																		}
																		else if(p_js_pt->Text.IsEqiAscii("id")) {
																			p_entry->PackagingTypeUuid.FromStr(p_js_pt->P_Child->Text);
																		}
																	}
																}
															}
															else if(p_js_fld->Text.IsEqiAscii("status")) { // object
															}
															else if(p_js_fld->Text.IsEqiAscii("viscosity")) { // object
															}
															else if(p_js_fld->Text.IsEqiAscii("viscositySAE")) { // object
															}
														}
													}
													if(p_entry->Ident != 0 && p_entry->Name[0]) {
														temp_buf.Z();
														temp_buf.Cat(p_entry->Ident).Tab().Cat(p_entry->Name).Tab().Cat(p_entry->Weight, MKSFMTD(0, 6, 0)).Tab().
															Cat(p_entry->Capacity, MKSFMTD(0, 6, 0)).Tab().Cat(p_entry->IsActive ? "true" : "false").Tab().
															Cat(p_entry->PackingTypeName).Tab().Cat(p_entry->PackingTypeId).Tab().
															Cat(p_entry->PackagingTypeName).Tab().Cat(p_entry->PackagingTypeUuid, S_GUID::fmtIDL);
														f_out.WriteLine(temp_buf.CR());
													}
													else {
														GoodsList.atFree(entry_idx);
													}
												}
											}
										}
									}
								}							
							}
							ZDELETE(p_js_reply);
						}
						//Lth.Log("rep", 0, temp_buf);
						//if(ReadJsonReplyForSingleItem(temp_buf, "token", rIb.Token) > 0)
							//ok = 1;
					}
				}
			}
		} while(more);
	}
	ok = 1;
	CATCHZOK
	return ok;
}

int GazpromNeft::Helper_MakeBillList(PPID opID, int outerDocType/* 0 - sellout, 1 - sellin */, PPID locID, TSCollection <GazpromNeftBillPacket> & rList)
{
	int    ok = -1;
	BillFilt b_filt;
	PPViewBill b_view;
	BillViewItem view_item;
	PPBillPacket pack;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	b_filt.OpID = opID;
	//b_filt.LocList = P.LocList;
	b_filt.LocList.SetSingle(locID);
	if(outerDocType == 1) {
		b_filt.ObjectID = P.SupplID;
	}
	b_filt.Period = P.ExpPeriod;
	SETIFZ(b_filt.Period.low, encodedate(1, 1, 2022));
	THROW(b_view.Init_(&b_filt));
	for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
		if(!Helper_MakeBillEntry(view_item.ID, 0, outerDocType, rList))
			R_Logger.LogLastError();
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::Helper_MakeBillEntry(PPID billID, PPBillPacket * pBp, int outerDocType, TSCollection <GazpromNeftBillPacket> & rList)
{
	int    ok = -1;
	uint   new_pack_idx = 0;
	SString temp_buf;
	PPBillPacket pack__;
	if(!pBp && P_BObj->ExtractPacket(billID, &pack__) > 0) {
		pBp = &pack__;
	}
	if(pBp && pBp->Rec.Object) {
		PPID   acs_id = 0;
		const  PPID   psn_id = ObjectToPerson(pBp->Rec.Object, &acs_id);
		SString unit_name;
		SString added_msg_buf;
		PPPersonPacket psn_pack;
		if(psn_id && PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0) {
			long   tiiterpos = 0;
			StrAssocArray ti_pos_list;
			PPTransferItem ti;
			PPBillPacket::TiItemExt tiext;
			for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
				tiiterpos++;
				if(GObj.BelongToGroup(ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, ti.GoodsID, temp_buf.Z(), 0) > 0) {
					const  GazpromNeftGoodsPacket * p_entry = SearchGoodsEntry(temp_buf.ToInt64());
					if(p_entry)
						ti_pos_list.Add(tiiterpos, temp_buf, 0);
				}
			}
			if(ti_pos_list.getCount()) {
				GazpromNeftBillPacket * p_new_pack = rList.CreateNewItem(&new_pack_idx);
				THROW_SL(p_new_pack);
				p_new_pack->ID = pBp->Rec.ID;
				(temp_buf = pBp->Rec.Code).Transf(CTRANSF_INNER_TO_UTF8);
				STRNSCPY(p_new_pack->Code, temp_buf);
				p_new_pack->Dtm.Set(pBp->Rec.Dt, ZEROTIME);
				{
					S_GUID uuid;
					if(pBp->GetGuid(uuid) <= 0 || uuid.IsZero()) {
						uuid.Generate();
						THROW(P_BObj->PutGuid(pBp->Rec.ID, &uuid, 1));
					}
					p_new_pack->Uuid = uuid;
				}
				{
					PPLocationPacket loc_pack;
					RegisterTbl::Rec reg_rec;
					p_new_pack->Client.PersonID = psn_id;
					p_new_pack->Client.DlvrLocID = pBp->GetDlvrAddrID();
					{
						STokenRecognizer tr;
						SNaturalTokenArray nta;
						THROW_PP_S(PsnObj.GetRegister(psn_id, PPREGT_TPID, pBp->Rec.Dt, &reg_rec) > 0 && (temp_buf = reg_rec.Num).NotEmptyS(), PPERR_PERSONINNUNDEF, psn_pack.Rec.Name);
						tr.Run(temp_buf.ucptr(), temp_buf.Len(), nta, 0);
						added_msg_buf.Z().Cat(temp_buf).CatDiv('-', 1).Cat(psn_pack.Rec.Name);
						THROW_PP_S(nta.Has(SNTOK_RU_INN) > 0.0f, PPERR_PERSONINNINVALID, added_msg_buf);
					}
					STRNSCPY(p_new_pack->Client.INN, reg_rec.Num);
					if(PsnObj.GetRegister(psn_id, PPREGT_KPP, pBp->Rec.Dt, &reg_rec) > 0) {
						STRNSCPY(p_new_pack->Client.KPP, reg_rec.Num);
					}
					(p_new_pack->Client.Name = psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
					if(p_new_pack->Client.DlvrLocID && LocObj.GetPacket(p_new_pack->Client.DlvrLocID, &loc_pack) > 0) {
						S_GUID uuid;
						if(loc_pack.GetGuid(uuid) <= 0 || uuid.IsZero()) {
							uuid.Generate();
							THROW(LocObj.PutGuid(loc_pack.ID, &uuid, 1));
						}
						p_new_pack->Client.Uuid = uuid;
						LocationCore::GetAddress(loc_pack, 0, temp_buf);
						p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
					}
					else {
						p_new_pack->Client.DlvrLocID = 0;
						S_GUID uuid;
						if(psn_pack.GetGuid(uuid) <= 0 || uuid.IsZero()) {
							uuid.Generate();
							THROW(PsnObj.PutGuid(psn_id, &uuid, 1));
						}
						p_new_pack->Client.Uuid = uuid;
						if(psn_pack.Rec.RLoc && LocObj.GetPacket(psn_pack.Rec.RLoc, &loc_pack) > 0) {
							LocationCore::GetAddress(loc_pack, 0, temp_buf);
							p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
						}
						if(p_new_pack->Client.Address.IsEmpty()) {
							if(psn_pack.Rec.MainLoc && LocObj.GetPacket(psn_pack.Rec.MainLoc, &loc_pack) > 0) {
								LocationCore::GetAddress(loc_pack, 0, temp_buf);
								p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
							}
						}
					}
				}
				tiiterpos = 0;
				for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
					tiiterpos++;
					uint   pos_list_item_pos = 0;
					if(ti_pos_list.GetText(tiiterpos, temp_buf) > 0) {
						const int64 ware_ident = temp_buf.ToInt64();
						Goods2Tbl::Rec goods_rec;
						if(ware_ident > 0 && GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
							GazpromNeftBillPacket::Position item;
							double vat_sum_in_full_price = 0.0;
							item.GoodsID = goods_rec.ID;
							item.ProductId = temp_buf.ToInt64();
							item.Qtty = fabs(ti.Quantity_);
							item.Cost = fabs(ti.NetPrice());
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pBp->Rec.Dt, 1.0, item.Cost, &vat_sum_in_full_price, 0, 0, 16);
							item.CostWoVat = (item.Cost - vat_sum_in_full_price);
							const GazpromNeftGoodsPacket * p_goods_entry = SearchGoodsEntry(ware_ident);
							if(p_goods_entry) {
								//double volume = 0.0;
								//double weight = 0.0;
								item.Volume = item.Qtty * p_goods_entry->Capacity;
								item.Weight = item.Qtty * p_goods_entry->Weight; // @v11.6.4
								STRNSCPY(item.UnitName, p_goods_entry->PackagingTypeName);
								THROW_SL(p_new_pack->ItemList.insert(&item));
								ok = 1;
							}
						}
					}
				}
				if(ok <= 0 && p_new_pack) {
					rList.atFree(new_pack_idx);
					p_new_pack = 0;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

static IMPL_CMPFUNC(GazpromNeftBillPacket_ByClientUuid, i1, i2)
{
	int    result = 0;
	const GazpromNeftBillPacket * p1 = static_cast<const GazpromNeftBillPacket *>(i1);
	const GazpromNeftBillPacket * p2 = static_cast<const GazpromNeftBillPacket *>(i2);
	if(p1) {
		if(p2) {
			result = memcmp(&p1->Client.Uuid, &p2->Client.Uuid, sizeof(p1->Client.Uuid));
			if(result == 0) {
				CMPCASCADE2(result, &p1->Client, &p2->Client, PersonID, DlvrLocID);
			}
		}
		else
			result = +1;
	}
	else if(p2)
		result = -1;
	return result;
}

int GazpromNeft::SendSellout()
{
	int    ok = -1;
	const  LDATETIME dtm_now = getcurdatetime_();
	SString temp_buf;
	SString req_buf;
	SJson * p_js_list = 0;
	// POST /sales-api/api/v2/Sales/sellout
	PPIDArray loc_list;
	P.LocList.Get(loc_list);
	THROW(loc_list.getCount());
	{
		for(uint locidx = 0; locidx < loc_list.getCount(); locidx++) {
			const PPID loc_id = loc_list.get(locidx);
			S_GUID loc_uuid;
			if(GetWarehouseUuid(loc_id, loc_uuid) > 0) {
				SJson js_result(SJson::tOBJECT);
				TSCollection <GazpromNeftBillPacket> list;
				THROW(Helper_MakeBillList(Ep.ExpendOp, 0, loc_id, list));
				list.sort(PTR_CMPFUNC(GazpromNeftBillPacket_ByClientUuid));
				js_result.InsertString("warehouseId", temp_buf.Z().Cat(loc_uuid, S_GUID::fmtIDL));
				temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0).CatChar('Z');
				js_result.InsertString("uploadDate", temp_buf);
				{
					p_js_list = new SJson(SJson::tARRAY);
					//
					S_GUID prev_cli_uuid;
					PPID  prev_cli_psn_id = 0;
					PPID  prev_cli_dlvrloc_id = 0;
					SJson * p_js_clilist = 0; // new SJson(SJson::tOBJECT); // Список по одному клиенту
					SJson * p_js_doc_list = 0;
					for(uint idx = 0; idx < list.getCount(); idx++) {
						const GazpromNeftBillPacket * p_item = list.at(idx);
						if(p_item) {
							if(prev_cli_uuid != p_item->Client.Uuid || prev_cli_dlvrloc_id != p_item->Client.DlvrLocID || prev_cli_psn_id != p_item->Client.PersonID) {
								if(p_js_clilist) {
									assert(p_js_doc_list != 0);
									SETIFZ(p_js_doc_list, new SJson(SJson::tARRAY));
									p_js_clilist->Insert("documents", p_js_doc_list);
									p_js_doc_list = 0;
									p_js_list->InsertChild(p_js_clilist);
									p_js_clilist = 0;
								}
								p_js_clilist = new SJson(SJson::tOBJECT);
								p_js_doc_list = new SJson(SJson::tARRAY);
								SJson * p_js_client = new SJson(SJson::tOBJECT);
								//p_js_client->InsertString("internalId", "");
								p_js_client->InsertString("externalId", temp_buf.Z().Cat(p_item->Client.Uuid, S_GUID::fmtIDL));
								p_js_client->InsertString("name", (temp_buf = p_item->Client.Name).Escape());
								p_js_client->InsertString("address", (temp_buf = p_item->Client.Address).Escape());
								p_js_client->InsertString("inn", p_item->Client.INN);
								p_js_client->InsertStringNe("kpp", p_item->Client.KPP);
								p_js_client->InsertString("managerName", "none");
								// @v11.5.9 {
								p_js_client->InsertBool("isBlocked", p_item->Client.IsBlocked); 
								temp_buf = p_item->Client.Status;
								if(temp_buf.IsEmpty()) {
									PPLoadString("inaction_fem", temp_buf);
									temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
								}
								p_js_client->InsertString("status", temp_buf.Escape());
								// } @v11.5.9 
								p_js_clilist->Insert("client", p_js_client);
								p_js_client = 0;
							}
							{
								SJson * p_js_doc = new SJson(SJson::tOBJECT);
								p_js_doc->InsertString("externalId", temp_buf.Z().Cat(p_item->Uuid, S_GUID::fmtIDL));
								p_js_doc->InsertString("number", (temp_buf = p_item->Code).Escape());
								// @v11.6.2 temp_buf.Z().Cat(/*p_item->Dtm*/dtm_now, DATF_ISO8601CENT, 0).CatChar('Z'); // Это что-то парадоксальное! Газпромнефть требует чтоб дата документа совпадала с датой выгрузки.
								temp_buf.Z().Cat(p_item->Dtm, DATF_ISO8601CENT, 0).CatChar('Z'); // @v11.6.2 Они снова передумали! Теперь здесь будет дата документа.
								p_js_doc->InsertString("dateTime", temp_buf);
								//p_js_doc->InsertString("requestNumber", "");
								p_js_doc->InsertBool("isMainDocument", true);
								SJson * p_js_item_list = new SJson(SJson::tARRAY);
								for(uint itemidx = 0; itemidx < p_item->ItemList.getCount(); itemidx++) {
									const GazpromNeftBillPacket::Position & r_item = p_item->ItemList.at(itemidx);
									SJson * p_js_item = new SJson(SJson::tOBJECT);
									p_js_item->InsertInt64("productId", r_item.ProductId);
									p_js_item->InsertInt("quantity", R0i(r_item.Qtty));
									//p_js_item->InsertString("weight", ""); 
									p_js_item->InsertString("unit", r_item.UnitName);
									p_js_item->InsertString("volume", temp_buf.Z().Cat(r_item.Volume, MKSFMTD(0, 3, NMBF_NOTRAILZ)));
									p_js_item->InsertString("cost", temp_buf.Z().Cat(r_item.Cost, MKSFMTD(0, 2, 0)));
									p_js_item->InsertString("costWithoutVAT", temp_buf.Z().Cat(r_item.CostWoVat, MKSFMTD(0, 2, 0)));
									p_js_item_list->InsertChild(p_js_item);
									p_js_item = 0;
								}
								p_js_doc->Insert("positions", p_js_item_list);
								p_js_item_list = 0;
								p_js_doc_list->InsertChild(p_js_doc);
								p_js_doc = 0;
							}
							//
							prev_cli_uuid = p_item->Client.Uuid;
							prev_cli_psn_id = p_item->Client.PersonID;
							prev_cli_dlvrloc_id = p_item->Client.DlvrLocID;
						}
					}
					//
					if(p_js_clilist) {
						assert(p_js_doc_list != 0);
						SETIFZ(p_js_doc_list, new SJson(SJson::tARRAY));
						p_js_clilist->Insert("documents", p_js_doc_list);
						p_js_doc_list = 0;
						p_js_list->InsertChild(p_js_clilist);
						p_js_clilist = 0;
					}
					//
					js_result.Insert("sellouts", p_js_list);
					p_js_list = 0; // !
				}
				js_result.ToStr(req_buf);
				{
					SString url_buf;
					SString hdr_buf;
					SBuffer ack_buf;
					int   req = SHttpProtocol::reqUnkn;
					InetUrl url(MakeTargetUrl_(qSendSellout, &req, url_buf));
					ScURL c;
					StrStrAssocArray hdr_flds;
					MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
					{
						SFile wr_stream(ack_buf.Z(), SFile::mWrite);
						Lth.Log("req", url_buf, req_buf);
						THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
						{
							SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
							if(p_ack_buf) {
								temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
								Lth.Log("rep", 0, temp_buf);
								SJson * p_js_reply = SJson::Parse(temp_buf);
								if(p_js_reply) {
									//
									// ...
									//
									ZDELETE(p_js_reply);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_list;
	return ok;
}

int GazpromNeft::SendSellin()
{
	int    ok = -1;
	// POST /sales-api/api/v2/Sales/sellin
	const  LDATETIME dtm_now = getcurdatetime_();
	SString temp_buf;
	SString req_buf;
	SJson * p_js_doc_list = 0;
	// POST /sales-api/api/v2/Sales/sellout
	PPIDArray loc_list;
	P.LocList.Get(loc_list);
	TSCollection <GazpromNeftBillPacket> list;
	THROW(loc_list.getCount());
	list.sort(PTR_CMPFUNC(GazpromNeftBillPacket_ByClientUuid));
	{
		for(uint locidx = 0; locidx < loc_list.getCount(); locidx++) {
			const PPID loc_id = loc_list.get(locidx);
			S_GUID loc_uuid;
			if(GetWarehouseUuid(loc_id, loc_uuid) > 0) {
				SJson js_result(SJson::tARRAY);
				SJson * p_js_single_array_item = new SJson(SJson::tOBJECT);
				THROW(Helper_MakeBillList(Ep.RcptOp, 1, loc_id, list));
				//temp_buf.Z().Cat(dtm_now.d, DATF_ISO8601CENT)/*.CatChar('Z')*/;
				//temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0).Cat(".145Z");
				temp_buf.Z().Cat(dtm_now.d, DATF_ISO8601CENT);
				p_js_single_array_item->InsertString("uploadDate", temp_buf);
				p_js_single_array_item->InsertString("warehouseId", temp_buf.Z().Cat(loc_uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
				{
					p_js_doc_list = new SJson(SJson::tARRAY);
					for(uint idx = 0; idx < list.getCount(); idx++) {
						const GazpromNeftBillPacket * p_item = list.at(idx);
						if(p_item) {
							SJson * p_js_doc = new SJson(SJson::tOBJECT);
							p_js_doc->InsertString("invoiceId", temp_buf.Z().Cat(p_item->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
							SJson * p_js_item_list = new SJson(SJson::tARRAY);
							for(uint itemidx = 0; itemidx < p_item->ItemList.getCount(); itemidx++) {
								const GazpromNeftBillPacket::Position & r_item = p_item->ItemList.at(itemidx);
								SJson * p_js_item = new SJson(SJson::tOBJECT);
								// p_js_item->InsertNull("unit");
								//p_js_item->InsertString("Unit", /*r_item.UnitName*/"TNE");
								// p_js_item->InsertNull("volume");
								//if(r_item.Volume > 0.0)
									//p_js_item->InsertString("Volume", temp_buf.Z().Cat(r_item.Volume, MKSFMTD(0, 3, NMBF_NOTRAILZ)));
								if(r_item.Weight > 0.0) {
									// Масса должна быть в тоннах
									p_js_item->InsertString("weight", temp_buf.Z().Cat(r_item.Weight/1000.0, MKSFMTD(0, 3, NMBF_NOTRAILZ))); 
									//p_js_item->InsertDouble("weight", r_item.Weight/1000.0, MKSFMTD(0, 3, NMBF_NOTRAILZ)); 
								}
								p_js_item->InsertInt("quantity", R0i(r_item.Qtty));
								p_js_item->InsertInt64("productId", r_item.ProductId);
								p_js_item_list->InsertChild(p_js_item);
								p_js_item = 0;
							}
							p_js_doc->Insert("positions", p_js_item_list);
							p_js_item_list = 0;
							temp_buf.Z().Cat(/*p_item->Dtm*/dtm_now, DATF_ISO8601CENT, 0).Cat(".145Z"); // Это что-то парадоксальное! Газпромнефть требует чтоб дата документа совпадала с датой выгрузки.
							p_js_doc->InsertString("invoiceDate", temp_buf);
							p_js_doc->InsertString("invoiceNumber", (temp_buf = p_item->Code).Escape());
							temp_buf.Z().Cat(p_item->Dtm, DATF_ISO8601CENT, 0).Cat(".145Z");
							p_js_doc->InsertString("gpnInvoiceDate", temp_buf);
							p_js_doc->InsertString("gpnInvoiceNumber", (temp_buf = p_item->Code).Escape());
							//
							p_js_doc_list->InsertChild(p_js_doc);
							p_js_doc = 0;
						}
					}
					p_js_single_array_item->Insert("documents", p_js_doc_list);
					p_js_doc_list = 0;
				}
				js_result.InsertChild(p_js_single_array_item);
				p_js_single_array_item = 0;
				js_result.ToStr(req_buf);
				// @debug {
#if 0 // {
				if(false) {
					//"D:\Papyrus\Src\VBA\0-GasPromNeft-Rud\sellin-sample.json"
					SFile f_in("/Papyrus/Src/VBA/0-GasPromNeft-Rud/sellin-sample.json", SFile::mRead);
					if(f_in.IsValid()) {
						STempBuffer sample_buf(4096);
						size_t actual_size = 0;
						if(f_in.ReadAll(sample_buf, 0, &actual_size) > 0) {
							temp_buf.Z().CatN(sample_buf, actual_size);
							SJson * p_js_sample = SJson::Parse(temp_buf);
							if(p_js_sample) {
								p_js_sample->ToStr(req_buf);
								ZDELETE(p_js_sample);
							}
						}
					}
				}
				// } @debug 
#endif // } 0
				{
					SString url_buf;
					SString hdr_buf;
					SBuffer ack_buf;
					int   req = SHttpProtocol::reqUnkn;
					InetUrl url(MakeTargetUrl_(qSendSellin, &req, url_buf));
					ScURL c;
					StrStrAssocArray hdr_flds;
					MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
					{
						SFile wr_stream(ack_buf.Z(), SFile::mWrite);
						Lth.Log("req", url_buf, req_buf);
						THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
						{
							SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
							if(p_ack_buf) {
								temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
								Lth.Log("rep", 0, temp_buf);
								SJson * p_js_reply = SJson::Parse(temp_buf);
								if(p_js_reply) {
									//
									// ...
									//
									ZDELETE(p_js_reply);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc_list;
	return ok;
}

int GazpromNeft::GetGoodsList(PPIDArray & rList)
{
	rList.Z();
	int    ok = -1;
	GoodsFilt filt;
	filt.Flags |= GoodsFilt::fShowArCode;
	filt.CodeArID = P.SupplID;
	GoodsIterator iter(&filt, 0);
	Goods2Tbl::Rec goods_rec;
	while(iter.Next(&goods_rec) > 0) {
		rList.add(goods_rec.ID);
	}
	return ok;
}

int GazpromNeft::GetWarehouseUuid(PPID locID, S_GUID & rUuid)
{
	int    ok = 0;
	PPObjTag tag_obj;
	PPID   tag_id = 0;
	if(tag_obj.FetchBySymb("GAZPROMNEFT-WHUUID", &tag_id) > 0) {
		if(PPRef->Ot.GetTagGuid(PPOBJ_LOCATION, locID, tag_id, rUuid) > 0)
			ok = 1;
	}
	return ok;
}

int GazpromNeft::SendRest()
{
	int    ok = -1;
	// POST /sales-api/api/v1/Sales/warehousebalances
	SString temp_buf;
	SString unit_name;
	SString req_buf;
	uint   result_loc_count = 0; // Счетчик, по нулевому значению которого мы поймем, что результат пустой и стало быть надо сообщить об ошибке
	const LDATETIME dtm_now = getcurdatetime_();
	SJson js_result(SJson::tARRAY);
	PPIDArray goods_list;
	PPIDArray loc_list;
	P.LocList.Get(loc_list);
	THROW(loc_list.getCount());
	GetGoodsList(goods_list);
	THROW(goods_list.getCount());
	THROW(AccessToken.NotEmpty()); // @err // ***
	for(uint locidx = 0; locidx < loc_list.getCount(); locidx++) {
		const PPID loc_id = loc_list.get(locidx);
		S_GUID loc_uuid;
		if(GetWarehouseUuid(loc_id, loc_uuid) > 0) {
			PPViewGoodsRest view;
			GoodsRestFilt filt;
			GoodsRestViewItem view_item;
			
			filt.Date = plusdate(getcurdate_(), -1);
			filt.GoodsList.Set(&goods_list);
			THROW(view.Init_(&filt));
			{
				SJson * p_js_single_obj = new SJson(SJson::tOBJECT);
				SJson * p_js_positions = new SJson(SJson::tARRAY);
				temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0).CatChar('Z');
				p_js_single_obj->InsertString("dateTime", temp_buf);
				p_js_single_obj->InsertString("warehouseId", temp_buf.Z().Cat(loc_uuid, S_GUID::fmtIDL));
				for(view.InitIteration(); view.NextIteration(&view_item) > 0;) {
					if(view_item.Rest > 0.0) {
						GObj.P_Tbl->GetArCode(P.SupplID, view_item.GoodsID, temp_buf, 0);
						const int64 ware_ident = temp_buf.ToInt64();
						if(ware_ident) {
							SJson * p_js_item = new SJson(SJson::tOBJECT);
							const GazpromNeftGoodsPacket * p_goods_entry = SearchGoodsEntry(ware_ident);
							double weight = 0.0;
							double volume = 0.0;
							unit_name.Z();
							if(p_goods_entry) {
								weight = view_item.Rest * p_goods_entry->Weight;
								volume = view_item.Rest * p_goods_entry->Capacity;
								unit_name = p_goods_entry->PackagingTypeName;
							}
							p_js_item->InsertInt64("productId", ware_ident);
							p_js_item->InsertDouble("quantity", view_item.Rest, MKSFMTD(0, 6, NMBF_NOTRAILZ));
							p_js_item->InsertString("weight", temp_buf.Z().Cat(weight, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
							p_js_item->InsertString("volume", temp_buf.Z().Cat(volume, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
							p_js_item->InsertString("unit", unit_name);
							//
							p_js_positions->InsertChild(p_js_item);
							p_js_item = 0;
						}
					}
				}
				p_js_single_obj->Insert("positions", p_js_positions);
				p_js_positions = 0;
				js_result.InsertChild(p_js_single_obj);
				p_js_single_obj = 0;
				result_loc_count++;
			}
		}
	}
	js_result.ToStr(req_buf);
	{
		SString url_buf;
		SString hdr_buf;
		SBuffer ack_buf;
		int   req = SHttpProtocol::reqUnkn;
		InetUrl url(MakeTargetUrl_(qSendRest, &req, url_buf));
		ScURL c;
		StrStrAssocArray hdr_flds;
		MakeHeaderFields(AccessToken, &hdr_flds, hdr_buf);
		{
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, temp_buf);
					SJson * p_js_reply = SJson::Parse(temp_buf);
					if(p_js_reply) {
						//
						// ...
						//
						ZDELETE(p_js_reply);
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::GetGoodsStoreFileName(SString & rBuf) const
{
	return PPGetFilePath(PPPATH_OUT, "gazpromneftgoodsstorage", rBuf.Z());
}

int GazpromNeft::StoreGoods(TSCollection <GazpromNeftGoodsPacket> & rList)
{
	int    ok = 1;
	SString file_name;
	SSerializeContext sctx;
	SBuffer buffer;
	THROW_SL(TSCollection_Serialize(rList, +1, buffer, &sctx));
	{
    	const  size_t bsize = buffer.GetAvailableSize();
    	GoodsStorageHeader hdr;
		// @v10.8.2 @ctr MEMSZERO(hdr);
		// @v10.8.2 @ctr memcpy(hdr.Signature, "GNGS", 4);
		SCRC32 cc;
		hdr.CRC = cc.Calc(0, buffer.GetBuf(0), bsize);
		//
		GetGoodsStoreFileName(file_name);
		SFile f_out(file_name, SFile::mWrite|SFile::mBinary);
		THROW_SL(f_out.IsValid());
		THROW_SL(f_out.Write(&hdr, sizeof(hdr)));
		THROW_SL(f_out.Write(buffer));
	}
	CATCHZOK
	return ok;
}

int GazpromNeft::RestoreGoods(TSCollection <GazpromNeftGoodsPacket> & rList)
{
	int    ok = 1;
	SString file_name;
	SSerializeContext sctx;
	SBuffer buffer;
	{
    	GoodsStorageHeader hdr;
    	GetGoodsStoreFileName(file_name);
    	SFile f_in(file_name, SFile::mRead|SFile::mBinary);
    	THROW_SL(f_in.IsValid());
		THROW_SL(f_in.Read(&hdr, sizeof(hdr)));
		THROW(memcmp(hdr.Signature, "GNGS", 4) == 0);
		THROW_SL(f_in.Read(buffer));
		{
			SCRC32 cc;
			uint32 _crc = cc.Calc(0, buffer.GetBuf(0), buffer.GetAvailableSize());
			THROW(_crc == hdr.CRC);
		}
		THROW_SL(TSCollection_Serialize(rList, -1, buffer, &sctx));
	}
	CATCHZOK
	return ok;
}
	
const GazpromNeftGoodsPacket * GazpromNeft::SearchGoodsEntry(int64 ident) const
{
	const GazpromNeftGoodsPacket * p_result = 0;
	if(ident) {
		for(uint i = 0; !p_result && i < GoodsList.getCount(); i++) {
			const GazpromNeftGoodsPacket * p_entry = GoodsList.at(i);
			if(p_entry && p_entry->Ident == ident) {
				p_result = p_entry;
			}
		}
	}
	return p_result;
}
//
//
//
class AgentPlus : public PrcssrSupplInterchange::ExecuteBlock { // @v11.6.5 @construction
public:
	/*
		Наименование пакета	Каталог FTP	Название файла
		----------------------------------------------
		Контрагенты               customers      Customers_20190910120358.xml
			code          Строка Да  Код клиента -однозначный идентификатор контрагента, уникален;
			name          Строка Да  Наименование юр лица, сокращенное для отражения на планшете
			fullname      Строка Нет Полное наименование контрагента
			inn           Строка Да  Инн юридического лица
			kpp           Строка Нет КПП юридического лица
			address       Строка Нет Юридический адрес юридического лица
		Торговые точки            trade_points   Trade_points_20190910120358.xml
			code          Строка Да  Код торговой точки – уникальный идентификатор, позволяющий однозначно идентифицировать ТТ;
			customercode  Строка Да  Идентификатор контрагента, должен обязательно присутствовать в пакете Контрагенты;
			address       Строка Да  Адрес торговой точки
			price         Строка Да  Код прайс листа, для торговой точки; Должен быть в файле Prices;
		Товары                    products       Products_20190910120358.xml
			code          Строка Да  Код номенклатуры, в учетной системе дистрибьютера, позволяющий однозначно определить товар;
			name          Строка Да  Наименование товара
			suppliercode  Строка Нет Код производителя
			vat           Число  Да  Ставка НДС 
			price_group   Строка Нет Код ценовой группы, к которой принадлежит товар, должен быть в файле price_groups;
		Ценовые группы            price_groups   price_groups_20190910120358.xml
			code          Строка Да  Код ценовой группы;
			name          Строка Да  Название ценовой группы
		Склады                    warehouses     warehouses_20190910120358.xml
			code          Строка Да  Код склада, уникален, позволяет однозначное идентифицировать склад;
			name          Строка Да  Наименование склада
			discount      Число  Нет Скидки, распространяемая на склад, все остальные скидки при установке данной скидки – не учитываются;
		Торговые представители    salesreps      Salesreps_20190910120358.xml
			code          Строка Да  Код торгового представителя
			name          Строка Да  ФИО торгового представителя
		Остатки товаров           stocks         stocks_20190910120358.xml
			date          Дата   Да  Дата остатков
			warehousecode Строка Да  Код склада
			productcode   Строка Да  Код продукта
			quantity      Число  Да  Количество товара
		Цены номенклатуры         prices         prices_20190910120358.xml
			code          Строка Да  Уникальный код прайс листа
			name          Строка Да  Наименование прайс листа
			isactive      Булево Да  Признак активности
			vatIn         Булево Да  Признак, Тип цен включает НДС;
			products      Элемент_XML Да Список цен, по номенклатуре;
				productcode Строка Да   Код номенклатуры;
				price       Число  Да   Стоимость позиции, в данном виде цен;
		Скидки номенклатуры       discounts      discounts_20190910120358.xml
			customertype  Число  Да  Содержит описание назначения, на кого распространяется скидка
				Возможные значения:
				"1,000" – Скидка распространяется на контрагента;
				"2,000" – скидка распространяется на торговую точку
			customercode  Строка Да  Код назначения торговой скидки, на кого она распространяется; Код контрагента или торговой точки;
			producttype   Число  Да  Содержит описание назначения, на что распространяется скидка
				Возможные значения:
				"1,000" – Скидка распространяется на товар;
				"2,000" – скидка распространяется на ценовую группу;
			productcode   Число  Да  Код назначения, на что распространяется, скидка; Код номенклатуры, или ценовой группы;
			discount      Число  Да  Процент скидки, если число указано в отрицательном формате, это означает что это наценка;
		Дебиторская задолженность debts          debts_20190910120358.xml
			customercode   Строка Да  Код контрагента
			tradepointcode Строка Да  Код торговой точки
			documentnumber Строка Да  Номер документа, для идентификации клиентом;
			documentdate   Дата   Да  Дата документа дебиторской задолженности;
			paymentdate    Дата   Да  Дата оплаты;
			debtsum        Число  Да  Сумма задолженности
		Продажи                   salesrefunds   salesrefunds_20190910120358.xml

			tradepointcode Строка   Да Код торговой точки
			customercode   Строка   Да Код контрагента
			number         Строка   Да Номер документа продажи/возврата
			date           Дата     Да Дата документа продажи/возврата
			employeecode   Строка   Да Код торгового представителя
			warehousecode  Строка   Да Код склада
			type           Строка   Да Операция Продажа/возврат

			Items:
				productcode Строка Да Код товара
				counts      Число  Да Количество
				summ        Число  Да Сумма без НДС
				vatsumm     Число  Да Сумма НДС

		Движения товаров          movements      movements_20190910120358.xml
			date            Дата   Да  Дата операции движения
			warehousecode   Строка Да  склада
			transaction     Строка Да  Вид движения
			productcode     Строка Да  Код товара
			quantity        Число  Да  Количество
		Заказ покупателя          orders         Order_UD00000737_20190910120358.xml
			number          Строка Да  Номер заказа в учетной системе производителя
			date            Дата   Да  Дата создания заказа
			customercode    Строка Да  Код контрагента
			tradepointcode  Строка Да  Код торговой точки
			deliverydate    Дата   Да  Дата
			warehousecode   Строка Да  Код склада
			comment         Строка Нет Комментарий к заказу;

			Items:
				productcode Строка Да  Код товара
				counts      Число  Да  Количество в заказе;
				price       Число  Да  Цена клиента без НДС
				vat         Число  Да  Ставка НДС
				summ        Число  Да  Сумма по строке, с НДС 
	*/
	struct WhEntry { // @flat
		WhEntry() : ID(0)
		{
		}
		PPID   ID;
	};
	struct GoodsEntry { // @flat
		GoodsEntry() : ID(0), VatRate(0.0)
		{
			SupplCode[0] = 0;
			PriceGroupCode[0] = 0;
		}
		PPID   ID;
		char   SupplCode[24]; // Код производителя //
		double VatRate;
		char   PriceGroupCode[24];
	};
	struct StockEntry {
		StockEntry()
		{
			THISZERO();
		}
		PPID   LocID;
		LDATE  Dt;
		PPID   GoodsID;
		double InRest;
		double Mov_Tk01; // 1 - Поступления
		double Mov_Tk02; // 2 - Списание-расход - брак, прочие списания
		double Mov_Tk03; // 3 - Возвраты поставщику
		double Mov_Tk04; // 4 - Перемещение
		double Mov_Tk05; // 5 - Оприходование - прочие поступления
		double OutRest;
	};
	static IMPL_CMPFUNC(StockEntry, i1, i2)
	{
		const StockEntry * p1 = static_cast<const StockEntry *>(i1);
		const StockEntry * p2 = static_cast<const StockEntry *>(i2);
		RET_CMPCASCADE3(p1, p2, Dt, LocID, GoodsID);
	}
	struct SalesEntry {
		SalesEntry() : DocID(0), DocDate(ZERODATE), LocID(0), DlvrLocID(0), CliID(0), AgentID(0), IsRefund(false)
		{
			DocCode[0] = 0;
		}
		struct Item {
			PPID   GoodsID;
			double Qtty;
			double AmountWoVat;
			double Vat;
		};
		PPID   DocID;
		char   DocCode[48];
		LDATE  DocDate;
		PPID   LocID;
		PPID   DlvrLocID;
		PPID   CliID;      // person.id
		PPID   AgentID;
		bool   IsRefund; // false - sale, true - refund
		uint8  Reserve[3]; // @alignment
		TSVector <Item> ItemList;
	};
	AgentPlus(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger), TsHt(4096)
	{
	}
	~AgentPlus()
	{
	}
	int    Init()
	{
		int    ok = 1;
		WhList.clear();
		GoodsList.clear();
		MakeLocList(WhList);
		MakeGoodsList(GoodsList);
		return ok;
	}
	int      ExportAll()
	{
		int    ok = 1;
		xmlTextWriter * p_x = 0;
		SString temp_buf;
		ExportGoods(GoodsList);
		ExportWarehouses(WhList);
		{
			TSVector <StockEntry> stock_list;
			TSCollection <SalesEntry> sales_list;
			{
				PrepareMovementData(stock_list);
				ExportRest(stock_list);
				ExportMovement(stock_list);
			}
			{
				PrepareSalesData(sales_list);
				ExportSales(sales_list);
				{
					PPIDArray customer_list;
					LAssocArray dlvr_loc_list;
					PPIDArray agent_list;
					SString out_file_name;
					PPPersonPacket psn_pack;
					PersonTbl::Rec psn_rec;
					LocationTbl::Rec loc_rec;
					PrepareRefsBySalesList(sales_list, customer_list, dlvr_loc_list, agent_list);
					{
						PPGetFilePath(PPPATH_OUT, MakeFileName("customers", temp_buf), out_file_name);
						p_x = xmlNewTextWriterFilename(out_file_name, 0);
						THROW(p_x);
						{
							SXml::WDoc _doc(p_x, cpUTF8);
							{
								SXml::WNode n_h(p_x, "customers");
								for(uint i = 0; i < customer_list.getCount(); i++) {
									const PPID cli_id = customer_list.get(i);
									if(cli_id && PsnObj.GetPacket(cli_id, &psn_pack, 0) > 0) {
										SXml::WNode n_i(p_x, "customer");
										n_i.PutAttrib("code", temp_buf.Z().Cat(psn_pack.Rec.ID));
										n_i.PutAttrib("name", temp_buf.Z().Cat(psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
										n_i.PutAttrib("fullname", temp_buf.Z().Cat(psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
										n_i.PutAttrib("inn", "");
										n_i.PutAttrib("kpp", "");
										n_i.PutAttrib("address", "");
									}
								}
							}
						}
						xmlFreeTextWriter(p_x);
						p_x = 0; // @mandatory!
					}
					{
						PPGetFilePath(PPPATH_OUT, MakeFileName("trade_points", temp_buf), out_file_name);
						p_x = xmlNewTextWriterFilename(out_file_name, 0);
						THROW(p_x);
						{
							SXml::WDoc _doc(p_x, cpUTF8);
							{
								SXml::WNode n_h(p_x, "trade_points");
								for(uint i = 0; i < dlvr_loc_list.getCount(); i++) {
									const LAssoc & dlvr_loc_item = dlvr_loc_list.at(i);
									if(dlvr_loc_item.Key && LocObj.Fetch(dlvr_loc_item.Key, &loc_rec) > 0) {
										SXml::WNode n_i(p_x, "trade_point");
										n_i.PutAttrib("code", temp_buf.Z().Cat(loc_rec.ID));
										n_i.PutAttrib("customercode", temp_buf.Z().Cat(dlvr_loc_item.Val));
										n_i.PutAttrib("address", "");
										n_i.PutAttrib("price", "");
									}
								}
							}
						}
						xmlFreeTextWriter(p_x);
						p_x = 0; // @mandatory!
					}
					{
						PPGetFilePath(PPPATH_OUT, MakeFileName("salesreps", temp_buf), out_file_name);
						p_x = xmlNewTextWriterFilename(out_file_name, 0);
						THROW(p_x);
						{
							SXml::WDoc _doc(p_x, cpUTF8);
							{
								SXml::WNode n_h(p_x, "salesreps");
								for(uint i = 0; i < agent_list.getCount(); i++) {
									const PPID agent_id = agent_list.at(i);
									if(agent_id && PsnObj.Search(agent_id, &psn_rec) > 0) {
										SXml::WNode n_i(p_x, "salesrep");
										n_i.PutAttrib("code", temp_buf.Z().Cat(psn_rec.ID));
										n_i.PutAttrib("name", temp_buf.Z().Cat(psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
									}
								}
							}
						}
						xmlFreeTextWriter(p_x);
						p_x = 0; // @mandatory!
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int      PrepareRefsBySalesList(const TSCollection <SalesEntry> & rList, PPIDArray & rCustomerList, LAssocArray & rDlvrLocList, PPIDArray & rAgentList)
	{
		int    ok = 1;
		LocationTbl::Rec loc_rec;
		for(uint i = 0; i < rList.getCount(); i++) {
			const auto * p_se = rList.at(i);
			PPID   cli_id = 0;
			if(p_se->DlvrLocID && LocObj.Fetch(p_se->DlvrLocID, &loc_rec) > 0) {
				cli_id = NZOR(p_se->CliID, loc_rec.OwnerID);
				rDlvrLocList.AddUnique(p_se->DlvrLocID, cli_id, 0, 1);
			}
			else
				cli_id = p_se->CliID;
			rCustomerList.addnz(cli_id);
			rAgentList.addnz(p_se->AgentID);
		}
		rCustomerList.sortAndUndup();
		rAgentList.sortAndUndup();
		return ok;
	}
	int      PrepareSalesData(TSCollection <SalesEntry> & rList)
	{
		int    ok = 1;
		if(Ep.ExpendOp) {
			SString temp_buf;
			PPViewBill _view;
			BillFilt _filt;
			BillViewItem view_item;
			_filt.OpID = Ep.ExpendOp;
			//b_filt.LocList = P.LocList;
			for(uint locidx = 0; locidx < WhList.getCount(); locidx++) {
				_filt.LocList.Add(WhList.at(locidx).ID);
			}
			_filt.Period = P.ExpPeriod;
			SETIFZ(_filt.Period.low, encodedate(1, 1, 2022));
			THROW(_view.Init_(&_filt));
			for(_view.InitIteration(PPViewBill::OrdByDefault); _view.NextIteration(&view_item) > 0;) {
				const PPID bill_id = view_item.ID;
				if(view_item.Object) {
					uint   new_pack_idx = 0;
					PPBillPacket bpack;
					if(P_BObj->ExtractPacket(bill_id, &bpack) > 0) {
						PPID   buyer_acs_id = 0;
						PPID   agent_acs_id = 0;
						const  PPID   psn_id = ObjectToPerson(bpack.Rec.Object, &buyer_acs_id);
						SString unit_name;
						PPPersonPacket psn_pack;
						if(psn_id && PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0) {
							long   tiiterpos = 0;
							StrAssocArray ti_pos_list;
							PPTransferItem ti;
							PPBillPacket::TiItemExt tiext;
							for(TiIter tiiter(&bpack, ETIEF_UNITEBYGOODS, 0); bpack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
								tiiterpos++;
								if(GoodsList.lsearch(&ti.GoodsID, 0, CMPF_LONG)) {
									temp_buf.Z().Cat(ti.GoodsID);
									ti_pos_list.Add(tiiterpos, temp_buf, 0);
								}
							}
							if(ti_pos_list.getCount()) {
								SalesEntry * p_new_entry = rList.CreateNewItem(&new_pack_idx);
								THROW_SL(p_new_entry);
								{
									p_new_entry->LocID = bpack.Rec.LocID;
									p_new_entry->DocID = bpack.Rec.ID;
									STRNSCPY(p_new_entry->DocCode, bpack.Rec.Code);
									p_new_entry->DocDate = bpack.Rec.Dt;
									p_new_entry->AgentID = ObjectToPerson(bpack.Ext.AgentID, &agent_acs_id);
									p_new_entry->CliID = psn_pack.Rec.ID;
									{
										PPID dlvr_loc_id = bpack.GetDlvrAddrID();
										PPLocationPacket loc_pack;
										if(dlvr_loc_id && LocObj.GetPacket(dlvr_loc_id, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
											p_new_entry->DlvrLocID = dlvr_loc_id;
											//LocationCore::GetAddress(loc_pack, 0, p_new_entry->DlvrAddrText);
										}
										else {
											if(psn_pack.Rec.RLoc && LocObj.GetPacket(psn_pack.Rec.RLoc, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
												p_new_entry->DlvrLocID = psn_pack.Rec.RLoc;
												//LocationCore::GetAddress(loc_pack, 0, p_new_entry->DlvrAddrText);
											}
											else if(psn_pack.Rec.MainLoc && LocObj.GetPacket(psn_pack.Rec.MainLoc, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
												p_new_entry->DlvrLocID = psn_pack.Rec.MainLoc;
												//LocationCore::GetAddress(loc_pack, 0, p_new_entry->DlvrAddrText);
											}
										}
									}
									tiiterpos = 0;
									for(TiIter tiiter(&bpack, ETIEF_UNITEBYGOODS, 0); bpack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
										tiiterpos++;
										uint   pos_list_item_pos = 0;
										if(ti_pos_list.GetText(tiiterpos, temp_buf) > 0) {
											Goods2Tbl::Rec goods_rec;
											if(GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
												SalesEntry::Item item;
												double vat_sum_in_full_price = 0.0;
												item.GoodsID = goods_rec.ID;
												item.Qtty = fabs(ti.Quantity_);
												double item_amount = fabs(ti.NetPrice()) * item.Qtty;
												GObj.CalcCostVat(0, goods_rec.TaxGrpID, bpack.Rec.Dt, 1.0, item_amount, &vat_sum_in_full_price, 0, 0, 16);
												item.AmountWoVat = (item_amount - vat_sum_in_full_price);
												item.Vat = vat_sum_in_full_price;
												THROW_SL(p_new_entry->ItemList.insert(&item));
												ok = 1;
											}
										}
									}
									assert(p_new_entry);
									if(p_new_entry) {
										if(ok > 0) {
											//rList.SetupEntry(new_pack_idx, 0);
										}
										else {
											rList.atFree(new_pack_idx);
											p_new_entry = 0;
										}
									}
								}
							}
						}
					}
				}

			}
		}
		CATCHZOK
		return ok;
	}
	int      ExportSales(const TSCollection <SalesEntry> & rList)
	{
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		Goods2Tbl::Rec goods_rec;
		PPGetFilePath(PPPATH_OUT, MakeFileName("salesrefunds", temp_buf), out_file_name);
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "salesrefunds");
				for(uint i = 0; i < rList.getCount(); i++) {
					const auto * p_item = rList.at(i);
					if(p_item) {
						SXml::WNode n_i(p_x, "salesrefund");
						n_i.PutAttrib("tradepointcode", temp_buf.Z().Cat(p_item->DlvrLocID));
						n_i.PutAttrib("customercode", temp_buf.Z().Cat(p_item->CliID));
						n_i.PutAttrib("number", temp_buf.Z().Cat(p_item->DocCode).Transf(CTRANSF_INNER_TO_UTF8));
						n_i.PutAttrib("date", temp_buf.Z().Cat(p_item->DocDate, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat("000000"));
						n_i.PutAttrib("employeecode", temp_buf.Z().Cat(p_item->AgentID));
						n_i.PutAttrib("warehousecode", temp_buf.Z().Cat(p_item->LocID));
						{
							// Мы здесь используем предопределенные токены, применяемые для обмена с ЕГАИС, так как они по смыслу и
							// буквально совпадают с требованиями Agent Plust (за исключением прописной первой буквы - надеемся, что это проскочит).
							//PPHSC_RU_EGAIS_SALE            "Продажа" // @v11.0.11 Тип операции кассового чека, передаваемого в ЕГАИС
							//PPHSC_RU_EGAIS_RETOFSALE       "Возврат" // @v11.0.11 Тип операции кассового чека, передаваемого в ЕГАИС
							PPLoadStringS(PPSTR_HASHTOKEN_C, p_item->IsRefund ? PPHSC_RU_EGAIS_RETOFSALE : PPHSC_RU_EGAIS_SALE, temp_buf);
							n_i.PutAttrib("type", temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
						}
						for(uint tiidx = 0; tiidx < p_item->ItemList.getCount(); tiidx++) {
							const SalesEntry::Item & r_detail_item = p_item->ItemList.at(tiidx);
							SXml::WNode n_ti(p_x, "items ");
							n_ti.PutAttrib("productcode", temp_buf.Z().Cat(r_detail_item.GoodsID));
							n_ti.PutAttrib("counts", temp_buf.Z().Cat(r_detail_item.Qtty, MKSFMTD(0, 3, 0)));
							n_ti.PutAttrib("summ", temp_buf.Z().Cat(r_detail_item.AmountWoVat, MKSFMTD(0, 3, 0)));
							n_ti.PutAttrib("vatsumm", temp_buf.Z().Cat(r_detail_item.Vat, MKSFMTD(0, 3, 0)));
						}
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int      PrepareMovementData(TSVector <StockEntry> & rList)
	{
		int    ok = 1;
		const PPID suppl_acs_id = GetSupplAccSheet();
		const PPID sell_acs_id = GetSellAccSheet();
		GoodsGrpngArray gga;
		GCTFilt ggf;
		AdjGdsGrpng agg;
		Goods2Tbl::Rec goods_rec;
		PPUnit unit_rec;
		//TSVector <StockEntry> stock_list;
		TSVector <LDATE> date_list;
		{
			for(LDATE iter_dt = P.ExpPeriod.low; iter_dt <= P.ExpPeriod.upp; iter_dt = plusdate(iter_dt, 1)) {
				date_list.insert(&iter_dt);
			}
		}
		for(uint i = 0; i < WhList.getCount(); i++) {
			const WhEntry & r_loc_entry = WhList.at(i);
			PPLocationPacket loc_pack;
			for(uint date_idx = 0; date_idx < date_list.getCount(); date_idx++) {
				const LDATE iter_dt = date_list.at(date_idx);
				for(uint goodsidx = 0; goodsidx < GoodsList.getCount(); goodsidx++) {
					const GoodsEntry & r_goods_entry = GoodsList.at(goodsidx);
					StockEntry new_entry;
					uint   goods_idx = 0;
					new_entry.GoodsID = r_goods_entry.ID;
					GoodsGrpngEntry input;
					GoodsGrpngEntry output;
					gga.Reset();
					ggf.Period.SetDate(iter_dt);
					ggf.GoodsID = new_entry.GoodsID;
					ggf.Flags |= (OPG_CALCINREST|OPG_CALCOUTREST);
					THROW(agg.BeginGoodsGroupingProcess(ggf));
					THROW(gga.ProcessGoodsGrouping(ggf, &agg));
					agg.EndGoodsGroupingProcess();

					const GoodsGrpngEntry * p_in_rest = gga.GetInRest();
					const GoodsGrpngEntry * p_out_rest = gga.GetOutRest();
					gga.GetInput(input);
					gga.GetOutput(output);

					new_entry.LocID = r_loc_entry.ID;
					new_entry.Dt = iter_dt;
					new_entry.InRest = p_in_rest ? p_in_rest->Quantity : 0.0;
					new_entry.OutRest = p_out_rest ? p_out_rest->Quantity : 0.0;
					// 
					// double Mov_Tk01; // 1 - Поступления
					// double Mov_Tk02; // 2 - Списание-расход - брак, прочие списания
					// double Mov_Tk03; // 3 - Возвраты поставщику
					// double Mov_Tk04; // 4 - Перемещение
					// double Mov_Tk05; // 5 - Оприходование - прочие поступления
					//
					for(uint ggaidx = 0; ggaidx < gga.getCount(); ggaidx++) {
						auto & r_entry = gga.at(ggaidx);
						PPOprKind op_rec;
						if(IsIntrExpndOp(r_entry.OpID)) {
							new_entry.Mov_Tk04 += fabs(r_entry.Quantity);
						}
						else if(GetOpData(r_entry.OpID, &op_rec) > 0) {
							if(r_entry.OpTypeID == PPOPT_GOODSRECEIPT) {
								if(op_rec.AccSheetID && op_rec.AccSheetID == suppl_acs_id) {
									new_entry.Mov_Tk01 += fabs(r_entry.Quantity);
								}
								else {
									new_entry.Mov_Tk05 += fabs(r_entry.Quantity);
								}
							}
							else if(r_entry.OpTypeID == PPOPT_GOODSEXPEND) {
								if(op_rec.AccSheetID && op_rec.AccSheetID == sell_acs_id) {
									new_entry.Mov_Tk02 += fabs(r_entry.Quantity);
								}
								else if(op_rec.AccSheetID && op_rec.AccSheetID == suppl_acs_id) {
									new_entry.Mov_Tk03 += fabs(r_entry.Quantity); // Возвраты поставщику
								}
							}
						}
					}
					//new_entry.Input = input.Quantity;
					//new_entry.Output = output.Quantity;
					rList.insert(&new_entry);
				}
			}
		}
		CATCHZOK
		return ok;
	}
	int   ExportGoods(const TSVector <GoodsEntry> & rList)
	{
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		Goods2Tbl::Rec goods_rec;
		PPGetFilePath(PPPATH_OUT, MakeFileName("products", temp_buf), out_file_name);
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "products");
				for(uint i = 0; i < rList.getCount(); i++) {
					const auto & r_item = rList.at(i);
					if(r_item.ID && GObj.Search(r_item.ID, &goods_rec) > 0) {
						PPGoodsTaxEntry gte;
						double vat_rate = 0.0;
						if(GObj.FetchTax(goods_rec.ID, ZERODATE, 0, &gte) > 0)
							vat_rate = gte.GetVatRate();
						SXml::WNode n_i(p_x, "product");
						n_i.PutAttrib("code", temp_buf.Z().Cat(goods_rec.ID));
						n_i.PutAttrib("name", temp_buf.Z().Cat(goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
						n_i.PutAttrib("suppliercode", "");
						n_i.PutAttrib("vat", temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 3, 0)));
						n_i.PutAttrib("price_group", "");
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int   ExportWarehouses(const TSVector <WhEntry> & rList)
	{
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		LocationTbl::Rec loc_rec;
		PPGetFilePath(PPPATH_OUT, MakeFileName("warehouses", temp_buf), out_file_name);
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "warehouses");
				for(uint i = 0; i < rList.getCount(); i++) {
					const auto & r_item = rList.at(i);
					if(r_item.ID && LocObj.Search(r_item.ID, &loc_rec) > 0) {
						SXml::WNode n_i(p_x, "warehouse");
						n_i.PutAttrib("code", temp_buf.Z().Cat(loc_rec.ID));
						n_i.PutAttrib("name", temp_buf.Z().Cat(loc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
						n_i.PutAttrib("discount", temp_buf.Z().Cat(0.0, MKSFMTD(0, 3, 0)));
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int   ExportRest(const TSVector <StockEntry> & rList)
	{
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, MakeFileName("stocks", temp_buf), out_file_name);
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "stocks");
				for(uint i = 0; i < rList.getCount(); i++) {
					const auto & r_item = rList.at(i);
					SXml::WNode n_i(p_x, "stock");
					n_i.PutAttrib("date", temp_buf.Z().Cat(r_item.Dt, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat("000000"));
					n_i.PutAttrib("warehousecode", temp_buf.Z().Cat(r_item.LocID));
					n_i.PutAttrib("productcode", temp_buf.Z().Cat(r_item.GoodsID));
					n_i.PutAttrib("quantity", temp_buf.Z().Cat(r_item.OutRest, MKSFMTD(0, 3, NMBF_DECCOMMA)));
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int   ExportMovement(const TSVector <StockEntry> & rList)
	{
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, MakeFileName("movements", temp_buf), out_file_name);
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "movements");
				for(uint i = 0; i < rList.getCount(); i++) {
					const auto & r_item = rList.at(i);
					int   ta_id_list[8];
					double ta_val_list[8];
					uint  ta_count = 0;
					if(r_item.Mov_Tk01 != 0.0) {
						ta_id_list[ta_count] = 1;
						ta_val_list[ta_count] = r_item.Mov_Tk01;
						ta_count++;
					}
					if(r_item.Mov_Tk02 != 0.0) {
						ta_id_list[ta_count] = 2;
						ta_val_list[ta_count] = r_item.Mov_Tk02;
						ta_count++;
					}
					if(r_item.Mov_Tk03 != 0.0) {
						ta_id_list[ta_count] = 3;
						ta_val_list[ta_count] = r_item.Mov_Tk03;
						ta_count++;
					}
					if(r_item.Mov_Tk04 != 0.0) {
						ta_id_list[ta_count] = 4;
						ta_val_list[ta_count] = r_item.Mov_Tk04;
						ta_count++;
					}
					if(r_item.Mov_Tk05 != 0.0) {
						ta_id_list[ta_count] = 5;
						ta_val_list[ta_count] = r_item.Mov_Tk05;
						ta_count++;
					}
					for(uint taidx = 0; taidx < ta_count; taidx++) {
						SXml::WNode n_i(p_x, "movement");
						n_i.PutAttrib("date", temp_buf.Z().Cat(r_item.Dt, DATF_YMD|DATF_NODIV).Cat("000000"));
						n_i.PutAttrib("warehousecode", temp_buf.Z().Cat(r_item.LocID));
						n_i.PutAttrib("transaction", temp_buf.Z().Cat(ta_id_list[taidx]));
						n_i.PutAttrib("productcode", temp_buf.Z().Cat(r_item.GoodsID));
						n_i.PutAttrib("quantity", temp_buf.Z().Cat(ta_val_list[taidx], MKSFMTD(0, 3, NMBF_DECCOMMA)));
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
private:
	SString & MakeFileName(const char * pPacketName, SString & rBuf)
	{
		//ИмяПакета_ yyyyMMddHHmmss
		const LDATETIME now_dtm = getcurdatetime_();
		rBuf.Z().Cat(pPacketName).CatChar('_').Cat(now_dtm.d, DATF_YMD|DATF_CENTURY|DATF_NODIV).Cat(now_dtm.t, TIMF_HMS|TIMF_NODIV).Dot().Cat("xml");
		return rBuf;
	}
	const GoodsEntry * SearchGoodsEntry(const TSVector <GoodsEntry> & rGoodsList, PPID goodsID) const
	{
		uint   goods_idx = 0;
		return rGoodsList.lsearch(&goodsID, &goods_idx, CMPF_LONG) ? &rGoodsList.at(goods_idx) : 0;
	}
	int    MakeGoodsList(TSVector <GoodsEntry> & rGoodsList)
	{
		//GoodsList.Z();
		rGoodsList.clear();
		int    ok = -1;
		GoodsFilt filt;
		//filt.Flags |= GoodsFilt::fShowArCode;
		//filt.CodeArID = P.SupplID;
		if(Ep.GoodsGrpID)
			filt.GrpIDList.Add(Ep.GoodsGrpID);
		else
			filt.SupplID = P.SupplID;
		Goods2Tbl::Rec goods_rec;
		for(GoodsIterator iter(&filt, 0); iter.Next(&goods_rec) > 0;) {
			//GoodsList.add(goods_rec.ID);
			GoodsEntry new_entry;
			uint   goods_idx = 0;
			new_entry.ID = goods_rec.ID;
			const  GoodsEntry * p_goods_entry = 0;
			if(new_entry.ID && !SearchGoodsEntry(rGoodsList, new_entry.ID)) {
				if(GObj.Search(new_entry.ID, &goods_rec) > 0) {
					rGoodsList.insert(&new_entry);
				}
			}
		}
		return ok;
	}
	int    MakeLocList(TSVector <WhEntry> & rLocList)
	{
		int    ok = 1;
		rLocList.clear();
		for(uint i = 0; i < P.LocList.GetCount(); i++) {
			const PPID loc_id = P.LocList.Get(i);
			PPLocationPacket loc_pack;
			if(loc_id && LocObj.GetPacket(loc_id, &loc_pack) > 0) {
				uint   loc_idx = 0;
				const  WhEntry * p_loc_entry = 0;
				if(rLocList.lsearch(&loc_id, &loc_idx, CMPF_LONG)) {
					;//p_loc_entry = &rLocList.at(loc_id);
				}
				else {
					WhEntry oe;
					oe.ID = loc_id;
					rLocList.insert(&oe);
				}
			}
		}
		return ok;
	}
	PPLogger & R_Logger;
	SString TokBuf;
	TokenSymbHashTable TsHt;
	TSVector <WhEntry> WhList;
	TSVector <GoodsEntry> GoodsList;
};
//
//
//
class VladimirskiyStandard : public PrcssrSupplInterchange::ExecuteBlock { // @v11.5.10 @construction
public:
	VladimirskiyStandard(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger), TsHt(4096)
	{
	}
	~VladimirskiyStandard()
	{
	}
	int    Init()
	{
		int    ok = 1;
		//MakeGoodsList();
		return ok;
	}
	struct ObjEntry {
		explicit ObjEntry(PPID id = 0) : ID(id)
		{
		}
		PPID   ID;
		S_GUID Uuid;
	};
	struct BillPacket {
		BillPacket() : ID(0), Dtm(ZERODATETIME), ContractorID(0), DlvrLocID(0), AgentID(0)
		{
			Code[0] = 0;
		}
		struct Position { // @flat
			Position() : GoodsID(0), KgPerUnit(0.0), Qtty(0.0), Volume(0.0), Cost(0.0), CostWoVat(0.0)
			{
				UnitName[0] = 0;
			}
			PPID   GoodsID;
			S_GUID GoodsUuid;
			char   UnitName[48];
			double KgPerUnit;
			double Qtty;
			double Volume;
			double Cost;
			double CostWoVat;
		};
		PPID   ID;
		S_GUID Uuid;
		char   Code[48];
		LDATETIME Dtm;
		PPID   ContractorID;
		S_GUID ContractorUuid;
		PPID   DlvrLocID;
		S_GUID DlvrLocUuid;
		PPID   AgentID;
		S_GUID AgentUuid;
		SString DlvrAddrText;
		TSVector <Position> ItemList;
	};
	class BillPacketCollection : public TSCollection <BillPacket> {
	public:
		BillPacketCollection(PrcssrSupplInterchange::ExecuteBlock & rEb) : TSCollection <BillPacket>(), R_Eb(rEb)
		{
		}
		int    SetupEntry(uint idx, int use_ta)
		{
			int   ok = 1;
			if(idx < getCount()) {
				BillPacket * p_item = at(idx);
				if(p_item) {
					PPTransaction tra(use_ta);
					THROW(tra);
					if(p_item->ContractorID) {
						PPPersonPacket psn_pack;
						if(R_Eb.PsnObj.GetPacket(p_item->ContractorID, &psn_pack, 0) > 0) {
							uint   psn_idx = 0;
							if(ContractorList.lsearch(&p_item->ContractorID, &psn_idx, CMPF_LONG)) {
								if(!p_item->ContractorUuid) {
									p_item->ContractorUuid = ContractorList.at(psn_idx).Uuid;
									assert(p_item->ContractorUuid);
								}
								else {
									assert(p_item->ContractorUuid == ContractorList.at(psn_idx).Uuid);
								}
							}
							else {
								ObjEntry oe(p_item->ContractorID);
								if(!p_item->ContractorUuid) {
									if(psn_pack.GetGuid(oe.Uuid) <= 0 || oe.Uuid.IsZero()) {
										oe.Uuid.Generate();
										THROW(R_Eb.PsnObj.PutGuid(p_item->ContractorID, &oe.Uuid, 0));
									}
									p_item->ContractorUuid = oe.Uuid;
								}
								else
									oe.Uuid = p_item->ContractorUuid;
								ContractorList.insert(&oe);
							}
						}
					}
					if(p_item->DlvrLocID) {
						PPLocationPacket loc_pack;
						if(R_Eb.LocObj.GetPacket(p_item->DlvrLocID, &loc_pack) > 0) {
							uint   loc_idx = 0;
							if(DlvrLocList.lsearch(&p_item->DlvrLocID, &loc_idx, CMPF_LONG)) {
								if(!p_item->DlvrLocUuid) {
									p_item->DlvrLocUuid = DlvrLocList.at(loc_idx).Uuid;
									assert(p_item->DlvrLocUuid);
								}
								else {
									assert(p_item->DlvrLocUuid == DlvrLocList.at(loc_idx).Uuid);
								}
							}
							else {
								ObjEntry oe(p_item->DlvrLocID);
								if(!p_item->DlvrLocUuid) {
									if(loc_pack.GetGuid(oe.Uuid) <= 0 || oe.Uuid.IsZero()) {
										oe.Uuid.Generate();
										THROW(R_Eb.LocObj.PutGuid(loc_pack.ID, &oe.Uuid, 0));
									}
									p_item->DlvrLocUuid = oe.Uuid;
								}
								else
									oe.Uuid = p_item->DlvrLocUuid;
								DlvrLocList.insert(&oe);
							}
						}
					}
					if(p_item->AgentID) {
						PPPersonPacket psn_pack;
						if(R_Eb.PsnObj.GetPacket(p_item->AgentID, &psn_pack, 0) > 0) {
							uint   psn_idx = 0;
							if(AgentList.lsearch(&p_item->AgentID, &psn_idx, CMPF_LONG)) {
								if(!p_item->AgentUuid) {
									p_item->AgentUuid = AgentList.at(psn_idx).Uuid;
									assert(p_item->AgentUuid);
								}
								else {
									assert(p_item->AgentUuid == AgentList.at(psn_idx).Uuid);
								}
							}
							else {
								ObjEntry oe(p_item->AgentID);
								if(!p_item->AgentUuid) {
									if(psn_pack.GetGuid(oe.Uuid) <= 0 || oe.Uuid.IsZero()) {
										oe.Uuid.Generate();
										THROW(R_Eb.PsnObj.PutGuid(p_item->AgentID, &oe.Uuid, 0));
									}
									p_item->AgentUuid = oe.Uuid;
								}
								else
									oe.Uuid = p_item->AgentUuid;
								AgentList.insert(&oe);
							}
						}
					}
					THROW(tra.Commit());
				}
			}
			CATCHZOK
			return ok;
		}
		TSVector <ObjEntry> ContractorList;
		TSVector <ObjEntry> DlvrLocList;
		TSVector <ObjEntry> AgentList;
	private:
		PrcssrSupplInterchange::ExecuteBlock & R_Eb;
	};
	struct StockEntry {
		StockEntry()
		{
			THISZERO();
		}
		PPID   LocID;
		S_GUID LocUuid;
		LDATE  Dt;
		PPID   GoodsID;
		S_GUID GoodsUuid;
		double InRest;
		double Input;
		double Output;
		double OutRest;
	};
	static IMPL_CMPFUNC(StockEntry, i1, i2)
	{
		const StockEntry * p1 = static_cast<const StockEntry *>(i1);
		const StockEntry * p2 = static_cast<const StockEntry *>(i2);
		RET_CMPCASCADE3(p1, p2, Dt, LocID, GoodsID);
	}
	void   WriteWarehouses(xmlTextWriter * pX, const TSVector <ObjEntry> & rLocList)
	{
		SString temp_buf;
		(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_WAREHOUSE_PL));
		SXml::WNode n_h(pX, temp_buf);
		for(uint i = 0; i < rLocList.getCount(); i++) {
			const ObjEntry & r_loc_entry = rLocList.at(i);
			LocationTbl::Rec loc_rec;
			if(r_loc_entry.ID && LocObj.Search(r_loc_entry.ID, &loc_rec) > 0) {
				SXml::WNode n_o(pX, "Object");
				r_loc_entry.Uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UID), temp_buf);
				(temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NAME), temp_buf);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DELEMARK), "false");
			}
		}
	}
	void   WriteGoods(xmlTextWriter * pX, const TSVector <ObjEntry> & rGoodsList)
	{
		SString temp_buf;
		(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_GOODS));
		SXml::WNode n_h(pX, temp_buf);
		for(uint i = 0; i < rGoodsList.getCount(); i++) {
			const ObjEntry & r_goods_entry = rGoodsList.at(i);
			Goods2Tbl::Rec goods_rec;
			PPUnit unit_rec;
			if(r_goods_entry.ID && GObj.Search(r_goods_entry.ID, &goods_rec) > 0) {
				SXml::WNode n_o(pX, "Object");
				r_goods_entry.Uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UID), temp_buf);
				GObj.P_Tbl->GetArCode(P.SupplID, r_goods_entry.ID, temp_buf, 0);
				if(temp_buf.NotEmptyS()) {
					n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_CODE), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
				}
				else {
					GObj.GetSingleBarcode(r_goods_entry.ID, temp_buf);
					if(temp_buf.NotEmpty()) {
						n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_CODE), temp_buf.Transf(CTRANSF_INNER_TO_UTF8));
					}
				}
				(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NAME), temp_buf);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_AR), temp_buf.Z().Cat(r_goods_entry.ID));
				if(goods_rec.UnitID && UObj.Search(goods_rec.UnitID, &unit_rec) > 0) {
					(temp_buf = unit_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
					n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UOMBASE), temp_buf);
				}
				(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_FULLNAME), temp_buf);
				n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DELEMARK), "false");
			}
		}
	}
	int    SendRest()
	{
		const  LDATETIME now_dtm = getcurdatetime_();
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		SString cli_code;
		TSVector <StockEntry> stock_list;
		TSVector <ObjEntry> loc_list;
		TSVector <ObjEntry> goods_list;
		TSVector <LDATE> date_list;
		Ep.GetExtStrData(Ep.extssClientCode, cli_code);
		if(cli_code.IsEmpty())
			cli_code = "99999";
		{
			GoodsGrpngArray gga;
			GCTFilt ggf;
			AdjGdsGrpng agg;
			Goods2Tbl::Rec goods_rec;
			PPUnit unit_rec;
			{
				for(LDATE iter_dt = P.ExpPeriod.low; iter_dt <= P.ExpPeriod.upp; iter_dt = plusdate(iter_dt, 1)) {
					date_list.insert(&iter_dt);
				}
			}
			THROW(MakeLocList(loc_list));
			THROW(MakeGoodsList(goods_list));
			for(uint i = 0; i < loc_list.getCount(); i++) {
				const ObjEntry & r_loc_entry = loc_list.at(i);
				PPLocationPacket loc_pack;
				for(uint date_idx = 0; date_idx < date_list.getCount(); date_idx++) {
					const LDATE iter_dt = date_list.at(date_idx);
					for(uint goodsidx = 0; goodsidx < goods_list.getCount(); goodsidx++) {
						const ObjEntry & r_goods_entry = goods_list.at(goodsidx);
						StockEntry new_entry;
						uint   goods_idx = 0;
						new_entry.GoodsID = r_goods_entry.ID;
						GoodsGrpngEntry input;
						GoodsGrpngEntry output;
						gga.Reset();
						ggf.Period.SetDate(iter_dt);
						ggf.GoodsID = new_entry.GoodsID;
						ggf.Flags |= (OPG_CALCINREST|OPG_CALCOUTREST);
						THROW(agg.BeginGoodsGroupingProcess(ggf));
						THROW(gga.ProcessGoodsGrouping(ggf, &agg));
						agg.EndGoodsGroupingProcess();

						const GoodsGrpngEntry * p_in_rest = gga.GetInRest();
						const GoodsGrpngEntry * p_out_rest = gga.GetOutRest();
						gga.GetInput(input);
						gga.GetOutput(output);

						new_entry.LocID = r_loc_entry.ID;
						new_entry.LocUuid = r_loc_entry.Uuid;
						new_entry.GoodsUuid = r_goods_entry.Uuid;
						new_entry.Dt = iter_dt;
						new_entry.InRest = p_in_rest ? p_in_rest->Quantity : 0.0;
						new_entry.OutRest = p_out_rest ? p_out_rest->Quantity : 0.0;
						new_entry.Input = input.Quantity;
						new_entry.Output = output.Quantity;
						stock_list.insert(&new_entry);
					}
				}
			}
		}
		{
			//GoodsInStock_54644_23092021_230003.xml
			SString date_buf;
			SString time_buf;
			(temp_buf = "GoodsInStock").CatChar('_').Cat(cli_code).CatChar('_').Cat(date_buf.Z().Cat(now_dtm.d, DATF_DMY|DATF_CENTURY|DATF_NODIV)).
				CatChar('_').Cat(time_buf.Z().Cat(now_dtm.t, TIMF_HMS|TIMF_NODIV)).Dot().Cat("xml");
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
		}
	    xmlTextWriter * p_x = xmlNewTextWriterFilename(out_file_name, 0);
	    THROW(p_x);
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "Exchange.Data");
				WriteWarehouses(p_x, loc_list);
				WriteGoods(p_x, goods_list);
				{
					(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_STOCK_PL));
					SXml::WNode n_list(p_x, temp_buf);
					stock_list.sort(PTR_CMPFUNC(StockEntry));
					{
						for(uint date_idx = 0; date_idx < date_list.getCount(); date_idx++) {
							const LDATE iter_dt = date_list.at(date_idx);
							SXml::WNode n_o(p_x, "Object");
							temp_buf.Z().Cat(iter_dt, DATF_ISO8601CENT).CatChar('T').Cat("00:00:00"); // Здесь дата остатков в формате 2021-09-16T00:00:00
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_PERIOD), temp_buf);
							for(uint i = 0; i < stock_list.getCount(); i++) {
								const StockEntry & r_entry = stock_list.at(i);
								if(r_entry.Dt == iter_dt) {
									const ObjEntry * p_goods_item = 0;
									const ObjEntry * p_loc_item = 0;
									uint   goods_idx = 0;
									uint   loc_idx = 0;
									if(goods_list.lsearch(&r_entry.GoodsID, &goods_idx, CMPF_LONG) && loc_list.lsearch(&r_entry.LocID, &loc_idx, CMPF_LONG)) {
										p_goods_item = &goods_list.at(goods_idx);
										p_loc_item = &loc_list.at(loc_idx);
										SXml::WNode n_g(p_x, Helper_GetToken(PPHSC_AGPLUS_GOODSITEMS));
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_GOODS), temp_buf.Z().Cat(p_goods_item->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // Номенклатура GUID
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_WAREHOUSE), temp_buf.Z().Cat(p_loc_item->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // Склад GUID
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_OPNAME), ""); // ИмяВидаОперации (empty)
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_STOCK_BEG), temp_buf.Z().Cat(r_entry.InRest, MKSFMTD(0, 6, NMBF_NOTRAILZ))); // НачальныйОстаток
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_MOVIN), temp_buf.Z().Cat(r_entry.Input, MKSFMTD(0, 6, NMBF_NOTRAILZ))); // Приход
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_MOVOUT), temp_buf.Z().Cat(r_entry.Output, MKSFMTD(0, 6, NMBF_NOTRAILZ))); // Расход
										n_g.PutInner(Helper_GetToken(PPHSC_AGPLUS_STOCK_END), temp_buf.Z().Cat(r_entry.OutRest, MKSFMTD(0, 6, NMBF_NOTRAILZ))); // КонечныйОстаток
									}
								}
							}
						}
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
	int    SendSales()
	{
		const  LDATETIME now_dtm = getcurdatetime_();
		int    ok = 1;
		SString temp_buf;
		SString out_file_name;
		SString cli_code;
		TSVector <ObjEntry> loc_list;
		TSVector <ObjEntry> goods_list;
		BillPacketCollection bill_list(*this);
		xmlTextWriter * p_x = 0;
		Ep.GetExtStrData(Ep.extssClientCode, cli_code);
		if(cli_code.IsEmpty())
			cli_code = "99999";
		{
			//Sales_54644_23092021_230003.xml
			SString date_buf;
			SString time_buf;
			(temp_buf = "Sales").CatChar('_').Cat(cli_code).CatChar('_').Cat(date_buf.Z().Cat(now_dtm.d, DATF_DMY|DATF_CENTURY|DATF_NODIV)).
				CatChar('_').Cat(time_buf.Z().Cat(now_dtm.t, TIMF_HMS|TIMF_NODIV)).Dot().Cat("xml");
			PPGetFilePath(PPPATH_OUT, temp_buf, out_file_name);
		}
		THROW(MakeLocList(loc_list));
		THROW(MakeGoodsList(goods_list));
		{
			for(uint i = 0; i < loc_list.getCount(); i++) {
				const PPID loc_id = loc_list.at(i).ID;
				THROW(Helper_MakeBillList(Ep.ExpendOp, loc_id, goods_list, bill_list));
			}
		}
		//
	    THROW(p_x = xmlNewTextWriterFilename(out_file_name, 0));
		{
			SXml::WDoc _doc(p_x, cpUTF8);
			{
				SXml::WNode n_h(p_x, "Exchange.Data");
				{
					SString addr_buf;
					(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC_PL));
					SXml::WNode n_list(p_x, temp_buf);
					for(uint i = 0; i < bill_list.DlvrLocList.getCount(); i++) {
						const ObjEntry & r_loc_entry = bill_list.DlvrLocList.at(i);
						PPLocationPacket loc_pack;
						if(r_loc_entry.ID && LocObj.GetPacket(r_loc_entry.ID, &loc_pack) > 0) {
							SXml::WNode n_o(p_x, "Object");
							r_loc_entry.Uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UID), temp_buf);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_CODE), temp_buf.Z().Cat(r_loc_entry.ID));
							temp_buf.Z();
							if(isempty(loc_pack.Name)) {
								PersonTbl::Rec psn_rec;
								if(loc_pack.OwnerID && PsnObj.Search(loc_pack.OwnerID, &psn_rec) > 0) {
									(temp_buf = psn_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
								}
							}
							else {
								(temp_buf = loc_pack.Name).Transf(CTRANSF_INNER_TO_UTF8);
							}
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NAME), temp_buf);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_FULLNAME), temp_buf);
							LocationCore::GetAddress(loc_pack, 0, addr_buf);
							addr_buf.Transf(CTRANSF_INNER_TO_UTF8);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC_ADR), addr_buf); // АдресДоставкиТТ
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC_EMAIL), ""); // АдресЭлектроннойПочтыТТ
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC_PHONE), ""); // ТелефонТТ
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC_RADR), addr_buf); // ФактАдресТТ
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_LEVEL), "0"); // Уровень
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DELEMARK), "false");
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_MANAGER), ""); // Менеджер
						}						
					}
				}
				{
					(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_CONTRACTOR_PL));
					SXml::WNode n_list(p_x, temp_buf);
					for(uint i = 0; i < bill_list.ContractorList.getCount(); i++) {
						const ObjEntry & r_psn_entry = bill_list.ContractorList.at(i);
						PPPersonPacket psn_pack;
						if(PsnObj.GetPacket(r_psn_entry.ID, &psn_pack, 0) > 0) {
							SXml::WNode n_o(p_x, "Object");
							r_psn_entry.Uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UID), temp_buf);						
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_CODE), temp_buf.Z().Cat(r_psn_entry.ID));
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NAME), (temp_buf = psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_INN), "");
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_FULLNAME), (temp_buf = psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8));
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_LEVEL), "0");
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_KPP), "");
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_LEGAL_PRIV), ""); // Как вариант "Юр. лицо"
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DELEMARK), "false");
						}
					}
				}
				{
					(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_MANAGER_PL));
					SXml::WNode n_list(p_x, temp_buf);
					for(uint i = 0; i < bill_list.AgentList.getCount(); i++) {
						const ObjEntry & r_psn_entry = bill_list.AgentList.at(i);
						SXml::WNode n_o(p_x, "Object");
						r_psn_entry.Uuid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
						n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_UID), temp_buf);
						n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NAME), "");
						n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DELEMARK), "false");
					}
				}
				WriteWarehouses(p_x, loc_list);
				WriteGoods(p_x, goods_list);
				{
					(temp_buf = "List").Dot().Cat(Helper_GetToken(PPHSC_AGPLUS_SALE_PL));
					SXml::WNode n_list(p_x, temp_buf);
					for(uint docidx = 0; docidx < bill_list.getCount(); docidx++) {
						const BillPacket * p_pack = bill_list.at(docidx);
						if(p_pack) {
							SXml::WNode n_o(p_x, "Object");
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DOCREF), temp_buf.Z().Cat(p_pack->Uuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // СсылкаДокумента
							temp_buf = Helper_GetToken(PPHSC_AGPLUS_DOCTYP_SALE);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DOCTYPE), temp_buf); // ТипДокумента
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DATE), temp_buf.Z().Cat(p_pack->Dtm, DATF_ISO8601CENT, 0)); // 
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_NUMBER), temp_buf.Z().Cat(p_pack->Code).Transf(CTRANSF_INNER_TO_UTF8)); // 
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRLOC), temp_buf.Z().Cat(p_pack->DlvrLocUuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // ТорговаяТочка
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_CONTRACTOR), temp_buf.Z().Cat(p_pack->ContractorUuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // Контрагент
							(temp_buf = p_pack->DlvrAddrText).Transf(CTRANSF_INNER_TO_UTF8);
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_DLVRADDR), temp_buf); // АдресДоставки
							temp_buf.Z();
							if(!!p_pack->AgentUuid) {
								temp_buf.Cat(p_pack->AgentUuid, S_GUID::fmtIDL|S_GUID::fmtLower);
							}
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_AGENT), temp_buf); // Агент
							n_o.PutInner(Helper_GetToken(PPHSC_AGPLUS_AGTDOC), ""); // ДокументОснование
							for(uint itmidx = 0; itmidx < p_pack->ItemList.getCount(); itmidx++) {
								const BillPacket::Position & r_item = p_pack->ItemList.at(itmidx);
								SXml::WNode n_goods(p_x, Helper_GetToken(PPHSC_AGPLUS_GOODSITEMS));
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_GOODS), temp_buf.Z().Cat(r_item.GoodsUuid, S_GUID::fmtIDL|S_GUID::fmtLower)); // Номенклатура
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_QTTY), temp_buf.Z().Cat(r_item.Qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ))); // Количество
								(temp_buf = r_item.UnitName).Transf(CTRANSF_INNER_TO_UTF8);
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_UOM), temp_buf); // ЕдиницаИзмерения
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_COEFF), temp_buf.Z().Cat(r_item.KgPerUnit, MKSFMTD(0, 3, 0))); // Коэффициент
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_WEIGHT), temp_buf.Z().Cat(r_item.Qtty * r_item.KgPerUnit, MKSFMTD(0, 3, 0))); // Вес
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_AMOUNT), temp_buf.Z().Cat(r_item.Cost * r_item.Qtty, MKSFMTD(0, 2, 0))); // Сумма
								n_goods.PutInner(Helper_GetToken(PPHSC_AGPLUS_PRICE), temp_buf.Z().Cat(r_item.Cost, MKSFMTD(0, 2, 0))); // Цена
							}
							//
						}
					}
				}
			}
		}
		CATCHZOK
		xmlFreeTextWriter(p_x);
		return ok;
	}
private:
	SString & FASTCALL Helper_GetToken(long tokId)
	{
		TokBuf.Z();
		if(!TsHt.Get(tokId, &TokBuf)) {
			PPLoadStringUtf8(PPSTR_HASHTOKEN_AGENTPLUS, tokId, TokBuf);
			TsHt.Put(tokId, TokBuf);
		}
		return TokBuf;
	}
	int    MakeLocList(TSVector <ObjEntry> & rLocList)
	{
		int    ok = 1;
		rLocList.clear();
		for(uint i = 0; i < P.LocList.GetCount(); i++) {
			const PPID loc_id = P.LocList.Get(i);
			PPLocationPacket loc_pack;
			if(loc_id && LocObj.GetPacket(loc_id, &loc_pack) > 0) {
				uint   loc_idx = 0;
				const  ObjEntry * p_loc_entry = 0;
				if(rLocList.lsearch(&loc_id, &loc_idx, CMPF_LONG)) {
					;//p_loc_entry = &rLocList.at(loc_id);
				}
				else {
					ObjEntry oe;
					oe.ID = loc_id;
					if(loc_pack.GetGuid(oe.Uuid) <= 0 || oe.Uuid.IsZero()) {
						oe.Uuid.Generate();
						THROW(LocObj.PutGuid(loc_pack.ID, &oe.Uuid, 1));
					}
					rLocList.insert(&oe);
					//p_loc_entry = &loc_list.at(loc_list.getCount()-1);
				}
			}
		}
		CATCHZOK
		return ok;
	}
	const ObjEntry * SearchGoodsEntry(const TSVector <ObjEntry> & rGoodsList, PPID goodsID) const
	{
		uint   goods_idx = 0;
		return rGoodsList.lsearch(&goodsID, &goods_idx, CMPF_LONG) ? &rGoodsList.at(goods_idx) : 0;
	}
	int    MakeGoodsList(TSVector <ObjEntry> & rGoodsList)
	{
		//GoodsList.Z();
		rGoodsList.clear();
		int    ok = -1;
		GoodsFilt filt;
		//filt.Flags |= GoodsFilt::fShowArCode;
		//filt.CodeArID = P.SupplID;
		if(Ep.GoodsGrpID)
			filt.GrpIDList.Add(Ep.GoodsGrpID);
		else
			filt.SupplID = P.SupplID;
		GoodsIterator iter(&filt, 0);
		Goods2Tbl::Rec goods_rec;
		while(iter.Next(&goods_rec) > 0) {
			//GoodsList.add(goods_rec.ID);
			StockEntry new_entry;
			uint   goods_idx = 0;
			new_entry.GoodsID = goods_rec.ID;
			const  ObjEntry * p_goods_entry = 0;
			if(new_entry.GoodsID && !SearchGoodsEntry(rGoodsList, new_entry.GoodsID)) {
				if(GObj.Search(new_entry.GoodsID, &goods_rec) > 0) {
					ObjEntry oe;
					oe.ID = new_entry.GoodsID;
					THROW(GObj.GetUuid(new_entry.GoodsID, oe.Uuid, true, 1));
					rGoodsList.insert(&oe);
					//p_goods_entry = &goods_list.at(goods_list.getCount()-1);
				}
			}
		}
		CATCHZOK
		return ok;
	}
	int    Helper_MakeBillEntry(PPID billID, PPBillPacket * pBp, const TSVector <ObjEntry> & rGoodsList, BillPacketCollection & rList)
	{
		int    ok = -1;
		uint   new_pack_idx = 0;
		SString temp_buf;
		PPBillPacket pack__;
		if(!pBp && P_BObj->ExtractPacket(billID, &pack__) > 0) {
			pBp = &pack__;
		}
		if(pBp && pBp->Rec.Object) {
			PPID   acs_id = 0;
			const  PPID   psn_id = ObjectToPerson(pBp->Rec.Object, &acs_id);
			SString unit_name;
			PPPersonPacket psn_pack;
			if(psn_id && PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0) {
				long   tiiterpos = 0;
				StrAssocArray ti_pos_list;
				PPTransferItem ti;
				PPBillPacket::TiItemExt tiext;
				for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
					tiiterpos++;
					const ObjEntry * p_goods_entry = SearchGoodsEntry(rGoodsList, ti.GoodsID);
					//if(GObj.BelongToGroup(ti.GoodsID, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, ti.GoodsID, temp_buf.Z(), 0) > 0) {
					if(p_goods_entry) {
						//const  ObjEntry * p_entry = SearchGoodsEntry(rGoodsList, temp_buf.ToLong());
						//if(p_entry)
						temp_buf.Z().Cat(p_goods_entry->ID);
						ti_pos_list.Add(tiiterpos, temp_buf, 0);
					}
				}
				if(ti_pos_list.getCount()) {
					BillPacket * p_new_pack = rList.CreateNewItem(&new_pack_idx);
					THROW_SL(p_new_pack);
					{
						PPTransaction tra(1);
						THROW(tra);
						p_new_pack->ID = pBp->Rec.ID;
						STRNSCPY(p_new_pack->Code, pBp->Rec.Code);
						p_new_pack->Dtm.Set(pBp->Rec.Dt, ZEROTIME);
						{
							S_GUID uuid;
							if(pBp->GetGuid(uuid) <= 0 || uuid.IsZero()) {
								uuid.Generate();
								THROW(P_BObj->PutGuid(pBp->Rec.ID, &uuid, 0));
							}
							p_new_pack->Uuid = uuid;
						}
						p_new_pack->ContractorID = psn_pack.Rec.ID;
						{
							PPID dlvr_loc_id = pBp->GetDlvrAddrID();
							PPLocationPacket loc_pack;
							if(dlvr_loc_id && LocObj.GetPacket(dlvr_loc_id, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
								p_new_pack->DlvrLocID = dlvr_loc_id;
								LocationCore::GetAddress(loc_pack, 0, p_new_pack->DlvrAddrText);
							}
							else {
								if(psn_pack.Rec.RLoc && LocObj.GetPacket(psn_pack.Rec.RLoc, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
									p_new_pack->DlvrLocID = psn_pack.Rec.RLoc;
									LocationCore::GetAddress(loc_pack, 0, p_new_pack->DlvrAddrText);
								}
								else if(psn_pack.Rec.MainLoc && LocObj.GetPacket(psn_pack.Rec.MainLoc, &loc_pack) > 0 && loc_pack.Type == LOCTYP_ADDRESS) {
									p_new_pack->DlvrLocID = psn_pack.Rec.MainLoc;
									LocationCore::GetAddress(loc_pack, 0, p_new_pack->DlvrAddrText);
								}
							}
						}
						/*{
							PPLocationPacket loc_pack;
							RegisterTbl::Rec reg_rec;
							p_new_pack->Client.PersonID = psn_id;
							p_new_pack->Client.DlvrLocID = pBp->GetDlvrAddrID();
							if(PsnObj.GetRegister(psn_id, PPREGT_TPID, pBp->Rec.Dt, &reg_rec) > 0) {
								STRNSCPY(p_new_pack->Client.INN, reg_rec.Num);
							}
							if(PsnObj.GetRegister(psn_id, PPREGT_KPP, pBp->Rec.Dt, &reg_rec) > 0) {
								STRNSCPY(p_new_pack->Client.KPP, reg_rec.Num);
							}
							(p_new_pack->Client.Name = psn_pack.Rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
							if(p_new_pack->Client.DlvrLocID && LocObj.GetPacket(p_new_pack->Client.DlvrLocID, &loc_pack) > 0) {
								S_GUID uuid;
								if(loc_pack.GetGuid(uuid) <= 0 || uuid.IsZero()) {
									uuid.Generate();
									THROW(LocObj.PutGuid(loc_pack.ID, &uuid, 1));
								}
								p_new_pack->Client.Uuid = uuid;
								LocationCore::GetAddress(loc_pack, 0, temp_buf);
								p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
							}
							else {
								p_new_pack->Client.DlvrLocID = 0;
								S_GUID uuid;
								if(psn_pack.GetGuid(uuid) <= 0 || uuid.IsZero()) {
									uuid.Generate();
									THROW(PsnObj.PutGuid(psn_id, &uuid, 1));
								}
								p_new_pack->Client.Uuid = uuid;
								if(psn_pack.Rec.RLoc && LocObj.GetPacket(psn_pack.Rec.RLoc, &loc_pack) > 0) {
									LocationCore::GetAddress(loc_pack, 0, temp_buf);
									p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
								}
								if(p_new_pack->Client.Address.IsEmpty()) {
									if(psn_pack.Rec.MainLoc && LocObj.GetPacket(psn_pack.Rec.MainLoc, &loc_pack) > 0) {
										LocationCore::GetAddress(loc_pack, 0, temp_buf);
										p_new_pack->Client.Address = temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
									}
								}
							}
						}*/
						tiiterpos = 0;
						for(TiIter tiiter(pBp, ETIEF_UNITEBYGOODS, 0); pBp->EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
							tiiterpos++;
							uint   pos_list_item_pos = 0;
							if(ti_pos_list.GetText(tiiterpos, temp_buf) > 0) {
								const long ware_ident = temp_buf.ToLong();
								Goods2Tbl::Rec goods_rec;
								if(ware_ident > 0 && GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
									BillPacket::Position item;
									double vat_sum_in_full_price = 0.0;
									item.GoodsID = goods_rec.ID;
									item.Qtty = fabs(ti.Quantity_);
									item.Cost = fabs(ti.NetPrice());
									GObj.CalcCostVat(0, goods_rec.TaxGrpID, pBp->Rec.Dt, 1.0, item.Cost, &vat_sum_in_full_price, 0, 0, 16);
									item.CostWoVat = (item.Cost - vat_sum_in_full_price);
									const ObjEntry * p_goods_entry = SearchGoodsEntry(rGoodsList, ware_ident);
									if(p_goods_entry) {
										//double volume = 0.0;
										//item.Volume = item.Qtty * p_goods_entry->Capacity;
										//STRNSCPY(item.UnitName, p_goods_entry->PackagingTypeName);
										PPUnit u_rec;
										if(UObj.Fetch(goods_rec.UnitID, &u_rec) > 0) {
											STRNSCPY(item.UnitName, u_rec.Name);
											if(UObj.TranslateToBase(goods_rec.UnitID, PPUNT_KILOGRAM, &item.KgPerUnit) > 0) {
												;
											}
											else
												item.KgPerUnit = 1.0;
										}
										item.GoodsUuid = p_goods_entry->Uuid;
										THROW_SL(p_new_pack->ItemList.insert(&item));
										ok = 1;
									}
								}
							}
						}
						assert(p_new_pack);
						if(p_new_pack) {
							if(ok > 0) {
								rList.SetupEntry(new_pack_idx, 0);
							}
							else {
								rList.atFree(new_pack_idx);
								p_new_pack = 0;
							}
						}
						THROW(tra.Commit());
					}
				}
			}
		}
		CATCHZOK
		return ok;
	}
	int    Helper_MakeBillList(PPID opID, PPID locID, const TSVector <ObjEntry> & rGoodsList, BillPacketCollection & rList)
	{
		int    ok = -1;
		BillFilt b_filt;
		PPViewBill b_view;
		BillViewItem view_item;
		PPBillPacket pack;
		b_filt.OpID = opID;
		//b_filt.LocList = P.LocList;
		b_filt.LocList.SetSingle(locID);
		b_filt.Period = P.ExpPeriod;
		SETIFZ(b_filt.Period.low, encodedate(1, 1, 2022));
		THROW(b_view.Init_(&b_filt));
		for(b_view.InitIteration(PPViewBill::OrdByDefault); b_view.NextIteration(&view_item) > 0;) {
			if(!Helper_MakeBillEntry(view_item.ID, 0, rGoodsList, rList))
				R_Logger.LogLastError();
		}
		CATCHZOK
		return ok;		
	}
	PPLogger & R_Logger;
	SString TokBuf;
	TokenSymbHashTable TsHt;
	//PPIDArray GoodsList;
};
//
//
//
IMPLEMENT_PPFILT_FACTORY(SupplInterchange); SupplInterchangeFilt::SupplInterchangeFilt() : PPBaseFilt(PPFILT_SUPPLINTERCHANGE, 0, 0)
{
	SetFlatChunk(offsetof(SupplInterchangeFilt, ReserveStart),
		offsetof(SupplInterchangeFilt, Reserve) - offsetof(SupplInterchangeFilt, ReserveStart) + sizeof(Reserve));
	SetBranchSString(offsetof(SupplInterchangeFilt, ExtString));
	SetBranchObjIdListFilt(offsetof(SupplInterchangeFilt, LocList));
	Init(1, 0);
}

int SupplInterchangeFilt::ReadPreviousVer(SBuffer & rBuf, int ver)
{
    int    ok = -1;
    if(ver == RpvInvSignValue) {
		SupplExpFilt _prev_filt;
		THROW(_prev_filt.Read(rBuf, 0));
		*this = _prev_filt;
		ok = 1;
    }
    CATCHZOK
    return ok;
}

SupplInterchangeFilt & FASTCALL SupplInterchangeFilt::operator = (const SupplInterchangeFilt & rS)
{
	PPBaseFilt::Copy(&rS, 0);
	PPExtStrContainer::Copy(rS);
	return *this;
}

SupplInterchangeFilt & FASTCALL SupplInterchangeFilt::operator = (const SupplExpFilt & rS)
{
	Init(1, 0);
	SupplID = rS.SupplID;
	ExpPeriod = rS.Period;
	MaxTransmitSize = rS.MaxFileSizeKB;
	SpcDisPct1 = static_cast<float>(rS.PctDis1);
	SpcDisPct2 = static_cast<float>(rS.PctDis2);
	SETFLAG(Actions, opExportStocks, rS.Flags & rS.expRest);
	SETFLAG(Actions, opExportBills, rS.Flags & rS.expBills);
	SETFLAG(Actions, opExportPrices, rS.Flags & rS.expPrice);
	SETFLAG(Actions, opExportDebts, rS.Flags & rS.expSaldo);
	SETFLAG(Actions, opExportGoodsDebts, rS.Flags & rS.expSaldo);
	SETFLAG(Flags, fDeleteRecentBills, rS.Flags & rS.expDelRecentBills);
	SETFLAG(Flags, fFlatStruc, rS.Flags & rS.expFlatStruc);
	PutExtStrData(extssAddScheme, rS.AddScheme);
	PutExtStrData(extssEncodeStr, rS.EncodeStr);
	PutExtStrData(extssClientCode, rS.ClientCode);
	LocList = rS.LocList;
	return *this;
}
//
//
//
PrcssrSupplInterchange::ExecuteBlock::ExecuteBlock() : P_BObj(BillObj), SeqID(0), BaseState(0)
{
}

PrcssrSupplInterchange::ExecuteBlock::ExecuteBlock(const ExecuteBlock & rS) :
	P_BObj(BillObj), Ep(rS.Ep), P(rS.P), SeqID(Ep.Fb.SequenceID), BaseState(rS.BaseState), GoodsList(rS.GoodsList), ArName(rS.ArName)
{
}

int PrcssrSupplInterchange::ExecuteBlock::InitGoodsList(long flags)
{
	int    ok = 1;
	if(BaseState & bstGoodsListInited) {
		ok = -1;
	}
	else {
		if(Ep.GoodsGrpID) {
            GoodsIterator::GetListByGroup(Ep.GoodsGrpID, &GoodsList);
            GoodsList.sortAndUndup();
            if(flags & iglfWithArCodesOnly) {
				SString code_buf;
                uint c = GoodsList.getCount();
                if(c) do {
					const PPID goods_id = GoodsList.get(--c);
					if(GObj.P_Tbl->GetArCode(P.SupplID, goods_id, code_buf, 0) > 0 && code_buf.NotEmptyS()) {
                        ;
					}
					else
						GoodsList.atFree(c);
                } while(c);
            }
		}
		else {
			BaseState |= bstAnyGoods;
			ok = 2;
		}
		BaseState |= bstGoodsListInited;
	}
	return ok;
}

int FASTCALL PrcssrSupplInterchange::ExecuteBlock::IsGoodsUsed(PPID goodsID) const
{
	int    ok = 0;
	assert(BaseState & bstGoodsListInited);
    if(BaseState & bstGoodsListInited) {
		if(BaseState & bstAnyGoods)
			ok = 1;
		else if(GoodsList.bsearch(labs(goodsID)))
			ok = 1;
    }
    return ok;
}

const PPIDArray * PrcssrSupplInterchange::ExecuteBlock::GetGoodsList() const
	{ return (BaseState & bstGoodsListInited && !(BaseState & bstAnyGoods)) ? &GoodsList : 0; }
void PrcssrSupplInterchange::ExecuteBlock::GetLogFileName(SString & rFileName) const
	{ rFileName = LogFileName; }

int PrcssrSupplInterchange::ExecuteBlock::GetSequence(long * pSeq, int use_ta)
{
	int    ok = 1;
	long   seq = 0;
	PPObjOpCounter opc_obj;
	THROW(P.SupplID);
	{
		PPObjArticle ar_obj;
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!SeqID) {
			SString temp_buf;
			PPOpCounterPacket pack;
			(temp_buf = "SISQ").CatChar('-').Cat(P.SupplID);
			STRNSCPY(pack.Head.Name, temp_buf);
			STRNSCPY(pack.Head.Symb, temp_buf);
			THROW(opc_obj.PutPacket(&SeqID, &pack, 0));
			{
				PPSupplAgreement suppl_agt;
				ar_obj.GetSupplAgreement(P.SupplID, &suppl_agt, 0);
				suppl_agt.Ep.Fb.SequenceID = SeqID;
				THROW(ar_obj.PutSupplAgreement(P.SupplID, &suppl_agt, 0));
			}
		}
		THROW(SeqID);
		THROW(opc_obj.GetCounter(SeqID, 0, &seq, 0));
		THROW(tra.Commit());
	}
	CATCHZOK
	ASSIGN_PTR(pSeq, seq);
	return ok;
}
//
//
//
PrcssrSupplInterchange::PrcssrSupplInterchange() : State(0), P_Eb(0)
{
}

PrcssrSupplInterchange::~PrcssrSupplInterchange()
{
	delete P_Eb;
}

int PrcssrSupplInterchange::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SupplInterchangeFilt temp_filt;
	THROW(temp_filt.IsA(pBaseFilt));
	{
		// SupplInterchangeFilt * p_filt = static_cast<SupplInterchangeFilt *>(pBaseFilt);
	}
	CATCHZOK
	return ok;
}

int PrcssrSupplInterchange::EditParam(PPBaseFilt * pBaseFilt)
{
	SupplInterchangeFilt temp_filt;
	if(!temp_filt.IsA(pBaseFilt))
		return 0;
	SupplInterchangeFilt * p_filt = static_cast<SupplInterchangeFilt *>(pBaseFilt);
	DIALOG_PROC_BODY(SupplInterchangeFiltDialog, p_filt);
}

int PrcssrSupplInterchange::InitExecuteBlock(const SupplInterchangeFilt * pParam, PrcssrSupplInterchange::ExecuteBlock & rBlk)
{
	int    ok = 1;
	ArticleTbl::Rec ar_rec;
	PPSupplAgreement suppl_agt;
	THROW_INVARG(pParam);
	THROW_PP(pParam->SupplID, PPERR_INVSUPPL);
	THROW(ArObj.Search(pParam->SupplID, &ar_rec) > 0);
	THROW_PP_S(ArObj.GetSupplAgreement(pParam->SupplID, &suppl_agt, 0) > 0, PPERR_ARHASNTAGREEMENT, ar_rec.Name);
	THROW_PP_S(!suppl_agt.Ep.IsEmpty(), PPERR_ARHASNTEXCHGPARAMS, ar_rec.Name);
	{
		rBlk.Ep = suppl_agt.Ep;
		rBlk.P = *pParam;
		rBlk.P.ExpPeriod.Actualize(ZERODATE);
		rBlk.P.ImpPeriod.Actualize(ZERODATE);
	}
	CATCHZOK
	return ok;
}

int PrcssrSupplInterchange::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	ArticleTbl::Rec ar_rec;
	PPSupplAgreement suppl_agt;
	SupplInterchangeFilt temp_filt;
	State &= ~stInited;
	ZDELETE(P_Eb);
	THROW(temp_filt.IsA(pBaseFilt));
	temp_filt = *static_cast<const SupplInterchangeFilt *>(pBaseFilt);
	THROW_PP(temp_filt.SupplID, PPERR_INVSUPPL);
	THROW(ArObj.Search(temp_filt.SupplID, &ar_rec) > 0);
	THROW_PP_S(ArObj.GetSupplAgreement(temp_filt.SupplID, &suppl_agt, 0) > 0, PPERR_ARHASNTAGREEMENT, ar_rec.Name);
	THROW_PP_S(!suppl_agt.Ep.IsEmpty(), PPERR_ARHASNTEXCHGPARAMS, ar_rec.Name);
	{
		ExecuteBlock eb;
		THROW_MEM(P_Eb = new ExecuteBlock);
		P_Eb->Ep = suppl_agt.Ep;
		P_Eb->P = temp_filt;
		P_Eb->P.ExpPeriod.Actualize(ZERODATE);
		P_Eb->P.ImpPeriod.Actualize(ZERODATE);
		P_Eb->ArName = ar_rec.Name;
	}
	State |= stInited;
	CATCHZOK
	return ok;
}

int SupplGoodsImport()
{
	int    ok = -1;
	SupplExpFilt filt;
	if(EditSupplExpFilt(&filt, 1) > 0) {
		SString path;
		if(PPOpenFile(PPTXT_FILPAT_GOODS_XML, path, 0, APPL->H_MainWnd) > 0) {
			SupplInterchangeFilt six_filt;
			six_filt = filt;
			PrcssrSupplInterchange six_prc;
			PrcssrSupplInterchange::ExecuteBlock eb;
			THROW(six_prc.InitExecuteBlock(&six_filt, eb));
			{
				PPSupplExchange_Baltika s_e(eb);
				THROW(s_e.Init());
				THROW(s_e.Import(path));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int DoSupplInterchange(SupplInterchangeFilt * pFilt)
{
	int    ok = -1;
	PrcssrSupplInterchange prcssr;
	if(pFilt) {
		if(prcssr.Init(pFilt) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	else {
		SupplInterchangeFilt param;
		prcssr.InitParam(&param);
		if(prcssr.EditParam(&param) > 0)
			if(prcssr.Init(&param) && prcssr.Run())
				ok = 1;
			else
				ok = PPErrorZ();
	}
	return ok;
}

int PrcssrSupplInterchange::Run()
{
	int    ok = -1;
	SString temp_buf;
	SString log_file_name;
	PPLogger logger;
	THROW_PP(State & stInited && P_Eb, PPERR_SUPPLIXNOTINITED);
	{
		ExecuteBlock & r_eb = *P_Eb;
		r_eb.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
		if(temp_buf.IsEqiAscii("AGENT_PLUS")) { // @v11.6.5
			AgentPlus cli(r_eb, logger);
			const long actions = r_eb.P.Actions;
			PPWaitStart();
			THROW(cli.Init());
			if(actions & SupplInterchangeFilt::opExportBills) {
				cli.ExportAll();
			}
		}
		else if(temp_buf.IsEqiAscii("VLADIMIRSKIY_STANDARD")) { // @v11.5.10
			VladimirskiyStandard cli(r_eb, logger);
			const long actions = r_eb.P.Actions;
			PPWaitStart(); // @v11.6.4
			THROW(cli.Init());
			if(actions & SupplInterchangeFilt::opExportStocks) {
				cli.SendRest();
			}
			if(actions & SupplInterchangeFilt::opExportSales) {
				cli.SendSales();
			}
		}
		else if(temp_buf.IsEqiAscii("MERCAPP-GAZPROMNEFT")) { // @v11.5.2
			GazpromNeft cli(r_eb, logger);
			PPWaitStart(); // @v11.6.4
			THROW(cli.Init());
			if(r_eb.P.Actions & (SupplInterchangeFilt::opExportBills|SupplInterchangeFilt::opExportStocks|SupplInterchangeFilt::opExportSales))
				r_eb.P.Actions |= SupplInterchangeFilt::opImportGoods;
			const long actions = r_eb.P.Actions;
			if(cli.Auth()) {
				if(actions & SupplInterchangeFilt::opImportGoods) {
					cli.GetWarehouses();
					cli.GetProducts(true);
				}
				if(actions & SupplInterchangeFilt::opExportStocks) {
					cli.SendRest();
				}
				if(actions & SupplInterchangeFilt::opExportSales) {
					cli.SendSellout();
				}
				if(actions & SupplInterchangeFilt::opExportBills) {
					cli.SendSellin();
				}
			}
		}
		else if(temp_buf.IsEqiAscii("MONOLIT-BALTIKA")) {
			int    max_size_kb = 0;
			PPIniFile ini_file;
			if(ini_file.IsValid()) {
				ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SUPPLEXP_BILLFILEMAXSIZE, &max_size_kb);
				ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SPECENCODESYMBS, temp_buf.Z());
				r_eb.P.PutExtStrData(SupplInterchangeFilt::extssEncodeStr, temp_buf);
			}
			r_eb.P.MaxTransmitSize = static_cast<size_t>(max_size_kb);
			{
				PPSupplExchange_Baltika s_e(r_eb);
				PPWaitStart();
				THROW(s_e.Init());
				if(r_eb.P.Actions & SupplInterchangeFilt::opImportGoods) {
					PPGetFilePath(PPPATH_OUT, "monolit-baltica.xml", temp_buf.Z());
					//
					THROW(s_e.Import(temp_buf));
					ok = 1;
				}
				if(r_eb.P.Actions & SupplInterchangeFilt::opExportBills|SupplInterchangeFilt::opExportClients|
					SupplInterchangeFilt::opExportDebts|SupplInterchangeFilt::opExportGoodsDebts|
					SupplInterchangeFilt::opExportPrices|SupplInterchangeFilt::opExportStocks) {
					THROW(s_e.Export(logger));
					ok = 1;
				}
			}
		}
		else if(temp_buf.IsEqiAscii("ISALES-PEPSI")) {
			const int rcv_goods_force_settings = BIN(r_eb.P.Actions & SupplInterchangeFilt::opImportGoods);
			iSalesPepsi cli(r_eb, logger);
			TSCollection <iSalesRoutePacket> routs;
			PPWaitStart();
			THROW(cli.Init()); // ООО "ПепсиКо Холдингс"
			if(r_eb.P.Actions & (SupplInterchangeFilt::opExportBills|SupplInterchangeFilt::opExportStocks|
				SupplInterchangeFilt::opExportPrices|SupplInterchangeFilt::opImportDesadv|SupplInterchangeFilt::opImportOrders))
				r_eb.P.Actions |= SupplInterchangeFilt::opImportGoods;
			const long actions = r_eb.P.Actions;
			if(actions & SupplInterchangeFilt::opImportGoods) {
				// @debug THROW(cli.ReceiveGoods(rcv_goods_force_settings, 1));
				// @debug {
				if(!cli.ReceiveGoods(rcv_goods_force_settings, 1))
					logger.LogLastError();
				// } @debug
			}
			if(actions & SupplInterchangeFilt::opImportRouts) {
				if(!cli.ReceiveRouts(routs))
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opImportOrders) {
				// @v11.3.6 {
				if(r_eb.P.Flags & SupplInterchangeFilt::fTestMode) {
					/* @debug
					const char * p_test_filename = "D:/Papyrus/Src/VBA/iSales-WBD-Sann/TASK/491-0a2b2e00-a9a6-43e2-bed3-6ebc83ef0b9f.csv";
					SFile f_in(p_test_filename, SFile::mRead|SFile::mBinary);
					STempBuffer in_buf(SMEGABYTE(1));
					if(f_in.IsValid()) {
						size_t actual_size = 0;
						if(f_in.ReadAll(in_buf, 0, &actual_size)) {
							SUnicodeMode um = SDetermineUtfEncoding(in_buf, actual_size);
							size_t bom_size = SGetUnicodeModeBomSize(um);
							cli.ReceiveOrder_Csv(in_buf+bom_size, actual_size);
						}
					}
					*/
				}
				// } @v11.3.6 
				if(!(r_eb.P.Flags & SupplInterchangeFilt::fTestMode)) { // @v11.3.8
					if(!cli.ReceiveVDocs())
						logger.LogLastError();
					if(!cli.ReceiveOrders())
						logger.LogLastError();
				}
				// @v11.3.6 {
				{
					//
					// Аварийный канал передачи заказов: почтовые сообщения //
					//
					PPObjTag tag_obj;
					PPObjectTag tag_rec;
					const PPID suppl_psn_id = ObjectToPerson(r_eb.P.SupplID, 0);
					if(suppl_psn_id && tag_obj.SearchBySymb("WBD-ORD-EMAILACC", 0, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_OBJLINK && tag_rec.TagEnumID == PPOBJ_INTERNETACCOUNT) {
						ObjTagItem tag_item;
						if(PPRef->Ot.GetTag(PPOBJ_PERSON, suppl_psn_id, tag_rec.ID, &tag_item) > 0) {
							const PPID email_acc_id = tag_item.Val.IntVal;
							if(email_acc_id) {
								PPObjInternetAccount inetacc_obj;
								PPInternetAccount2 inetacc_pack;
								if(inetacc_obj.Get(email_acc_id, &inetacc_pack) > 0 && !(inetacc_pack.Flags & PPInternetAccount2::fFtpAccount)) {
									PPGetPath(PPPATH_IN, temp_buf);
									if(temp_buf.NotEmpty()) {
										temp_buf.SetLastSlash().Cat("wbd-ord-email");
										const SString src_path(temp_buf);
										if(createDir(src_path)) {
											cli.GetOrderFilesFromMailServer(inetacc_pack.ID, src_path, 0/*deleMsg*/);
											(temp_buf = src_path).SetLastSlash().Cat("*.csv");
											SDirEntry sde;
											for(SDirec sd(temp_buf); sd.Next(&sde) > 0;) {
												sde.GetNameA(src_path, temp_buf);
												SFile f_in(temp_buf, SFile::mRead|SFile::mBinary);
												STempBuffer in_buf(SMEGABYTE(1));
												if(f_in.IsValid()) {
													size_t actual_size = 0;
													if(f_in.ReadAll(in_buf, 0, &actual_size)) {
														const SUnicodeMode um = SDetermineUtfEncoding(in_buf, actual_size);
														const size_t bom_size = SGetUnicodeModeBomSize(um);
														cli.ReceiveOrder_Csv(in_buf+bom_size, actual_size);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				// } @v11.3.6 
			}
			if(actions & SupplInterchangeFilt::opImportDesadv) {
				if(!cli.ReceiveReceipts())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportBills) {
				if(!cli.SendInvoices())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportStocks) {
				if(!cli.SendStocks())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportPrices) {
				if(!cli.SendPrices())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportDebts) {
				if(!cli.SendDebts())
					logger.LogLastError();
			}
			cli.GetLogFileName(log_file_name);
			PPWaitStop();
		}
		else if(temp_buf.IsEqiAscii("SAP-EFES")) {
			SapEfes cli(r_eb, logger);
			PPWaitStart();
			cli.Init();
			const long actions = r_eb.P.Actions;
			if(actions & SupplInterchangeFilt::opImportOrders) {
				if(!cli.ReceiveOrders())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportBills) {
				if(!cli.SendInvoices())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportStocks) {
				if(!cli.SendStocks())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportDebts) {
				if(!cli.SendDebts())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportSales) {
				if(!cli.SendSales_ByGoods())
					logger.LogLastError();
				if(!cli.SendSales_ByDlvrLoc())
					logger.LogLastError();
			}
			cli.GetLogFileName(log_file_name);
			PPWaitStop();
		}
		else if(temp_buf.IsEqiAscii("SFA-HEINEKEN")) {
			SfaHeineken cli(r_eb, logger);
			PPWaitStart();
			cli.Init();
			const long actions = r_eb.P.Actions;
			if(actions & SupplInterchangeFilt::opImportGoods) {
				if(!cli.ReceiveGoods())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opImportOrders) {
				if(!cli.ReceiveOrders())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportStocks) {
				if(!cli.SendStocks())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportSales) {
				if(!cli.SendSales())
					logger.LogLastError();
			}
			if(actions & SupplInterchangeFilt::opExportDebts) {
				if(!cli.SendDebts())
					logger.LogLastError();
			}
			// @v10.4.4 {
			if(actions & SupplInterchangeFilt::opExportBills) {
				if(!cli.SendReceipts())
					logger.LogLastError();
			}
			// } @v10.4.4 
			cli.GetLogFileName(log_file_name);
			PPWaitStop();
		}
		else {
			; //
		}
	}
	CATCH
		logger.LogLastError();
	ENDCATCH
	PPWaitStop();
	if(log_file_name.NotEmpty())
		logger.Save(log_file_name, LOGMSGF_DIRECTOUTP|LOGMSGF_TIME|LOGMSGF_USER);
	return ok;
}
