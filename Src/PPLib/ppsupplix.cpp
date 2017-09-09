// PPSUPPLIX.CPP
// Copyright (c) A.Sobolev 2016, 2017
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
	EncodeStr = 0;
	memzero(this, offsetof(SupplExpFilt, EncodeStr));
}

int SLAPI SupplExpFilt::Write(SBuffer & rBuf, long) const
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
	THROW_SL(rBuf.Write((long)LocList.GetCount()));
	{
		PPIDArray loc_list;
		LocList.CopyTo(&loc_list);
		for(uint32 i = 0; i < loc_list.getCount(); i++) {
			THROW_SL(rBuf.Write(loc_list.at(i)));
		}
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

int SLAPI SupplExpFilt::Read(SBuffer & rBuf, long)
{
	int    ok = 1;
	long   count = 0;
	LocList.FreeAll();
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
	ProtVer    = (uint16)pCfg->ProtVer;
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

#define HORECA_DLVRADDR_OFFS 100000000L

class PPSupplExchange_Baltika : public PrcssrSupplInterchange::ExecuteBlock {
public:
	enum BillExpParam {
		expEtc     = 0,
		expWeakAlc,
		expWoTareBeer
	};
	SLAPI  PPSupplExchange_Baltika(PrcssrSupplInterchange::ExecuteBlock & rEb) : PrcssrSupplInterchange::ExecuteBlock(rEb)
	{
		KegUnitID = -1;
	}
	int    SLAPI Init(/*const SupplExpFilt * pFilt*/);
	int    SLAPI Import(const char * pPath);
	int    SLAPI Export(PPLogger & rLogger);
private:
	int    SLAPI Send();
	int    SLAPI ExportPrice();
	int    SLAPI ExportRest();
	int    SLAPI ExportRestParties();
	int    SLAPI ExportSpoilageRest(PPID locID, uint filesIdx);
	int    SLAPI ExportBills(const BillExpParam &, const char * pClientCode, PPLogger & rLogger);
	int    SLAPI ExportSaldo2(const PPIDArray & rExclArList, const char * pClientCode, PPLogger * pLog);
	void   SLAPI DelFiles(const char * pFileName);
	PPID   SLAPI GetSaleChannelTagID();
	PPID   SLAPI GetConsigLocGroupID();
	long   SLAPI GetSaleChannelExtFld();
	int    SLAPI GetWeakAlcInfo(PPID * pLocID, PPID * pGGrpID, int getWoTareBeer);
	int    SLAPI GetSpoilageLocList(PPIDArray * pList);
	int    SLAPI GetBarcode(PPID goodsID, char * pBuf, size_t bufSize, int * pPackCode, int * pIsHoreca, double * pMult);
	int    SLAPI GetQtty(PPID goodsID, int calcByPhPerU, double * pQtty, double * pPrice);
	int    SLAPI GetDlvrAddrHorecaCode(PPID * pDlvrAddrID, SString & rCode);
	int    SLAPI GetConsigLocInfo(BillViewItem * pItem, PPID consigLocGrpID, LDATE * pParentDt, SString & rParentCode);
	int    SLAPI GetInfoFromMemo(const char * pMemo, LDATE * pParentDt, SString & rParentCode, int simple = 0);
	void   SLAPI GetInfoByLot(PPID lotID, PPTransferItem * pTi, LDATE * pBillDt, LDATE * pCreateDt, LDATE * pExpiry, SString * pSerial);
	int    SLAPI IsKegUnit(PPID goodsId);
	const char * SLAPI GetEaText() const;
	int    SLAPI GetSerial(PPID lotID, PPID goodsID, SString & rSerial);

	PPID   KegUnitID;
	PPID   DlvrAddrExtFldID; // Идентификатор дополнительного поля адреса доставки, хранящего код адреса у получателя отчета
	SStrCollection  Files;
	PPObjGoodsClass GCObj;
};

int SLAPI PPSupplExchange_Baltika::Init(/*const SupplExpFilt * pFilt*/)
{
	int    ok = 1;
	SString temp_buf;
	DlvrAddrExtFldID = 0;
	Files.freeAll();
	/* @v9.2.1
	if(!RVALUEPTR(Filt, pFilt))
		Filt.Init();
	THROW_PP(Filt.SupplID, PPERR_INVSUPPL);
	*/
	{
		PPIniFile ini_file;
		if(ini_file.IsValid()) {
			ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKADLVRADDREXTFLDID, temp_buf);
			if(temp_buf.NotEmptyS())
				DlvrAddrExtFldID = temp_buf.ToLong();
		}
	}
	// @v9.2.1 CATCHZOK
	return ok;
}

int SLAPI PPSupplExchange_Baltika::GetSerial(PPID lotID, PPID goodsID, SString & rSerial)
{
	// @construction
	int    ok = -1;
    rSerial = 0;
	ObjTagItem tag;
    if(PPRef->Ot.GetTag(PPOBJ_LOT, lotID, PPTAG_LOT_MANUFTIME, &tag) > 0) { // @v9.7.11
		SString temp_buf;
		int32  arcode_pack;
		GObj.P_Tbl->GetArCode(P.SupplID, goodsID, temp_buf, &arcode_pack);

		LDATETIME create_dtm;
		tag.GetTimestamp(&create_dtm);
		rSerial.Cat(create_dtm.d, DATF_DMY|DATF_NODIV).Cat(temp_buf);
		ok = 1;
    }
	/*@v9.7.11 else {
		P_BObj->GetSerialNumberByLot(lotID, rSerial, 1);
		ok = 2;
	}*/
	return ok;
}

class SoapExporter {
public:
	SLAPI SoapExporter(bool flatStruc = false)
	{
		HeaderRecCount = 0;
		LinesRecCount = 0;
		FilesCount = 0;
		MaxTransmitSize = 0;
		AddedRecType = 0;
		FlatStruc = flatStruc;
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
	int    SLAPI Init(const char * pFile, uint headRecType, uint lineRecType, const char * pHeadScheme, const char * pLineScheme,
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
	int SLAPI Init(const char * pFile, uint headRecType, uint lineRecType, uint addLineRecType, const char * pHeadScheme, const char * pLineScheme, const char * pAddLineScheme,
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
		if(addLineRecType) {
			AddLineRecType = addLineRecType;
			InitExportParam(p, addLineRecType);
			AddLineRec = p.InrRec;
		}
		HeadScheme = pHeadScheme;
		LineScheme = pLineScheme;
		AddLineScheme = pAddLineScheme;
		SchemeName = pSchemeName;
		F.Close();
		THROW_SL(F.Open(pFile, SFile::mWrite) > 0);
		THROW(WriteHeader());
		CATCHZOK
		HeaderRecCount = LinesRecCount = 0;
		return ok;
	}
	int    SLAPI AppendRecT(uint recTypeID, void * pRec, size_t recSize, int isFirstRec, int schemeNum, const char * pSchemeName /*=0*/)
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
	int    SLAPI AppendRecP(void * pHeadRec, size_t headRecSize, void * pLineRec, size_t lineRecSize, int headRecForNewFile /*=0*/)
	{
		int    ok = -1;
		if(pHeadRec && headRecSize || pLineRec && lineRecSize) {
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
				THROW(WriteRec(&LineRec, (LinesRecCount == 0) ? ((const char*)LineScheme) : 0, 1));
				LinesRecCount++;
			}
			ok = 1;
			{
				int64  file_size = 0;
				F.CalcSize(&file_size);
				//if(Filt.MaxFileSizeKB && (((size_t)file_size) / 1024) > Filt.MaxFileSizeKB) {
				if(MaxTransmitSize > 0 && (((size_t)file_size) / 1024) > MaxTransmitSize) {
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
	void   SLAPI EndDocument(int ok)
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
	void   SLAPI EndRecBlock()
	{
		if(F.IsValid()) {
			F.WriteLine("</d>");
			LinesRecCount = 0;
		}
	}
private:
	int    SLAPI InitExportParam(PPImpExpParam & rParam, uint recTyp)
	{
		int    ok = 1;
		rParam.Init();
		THROW(LoadSdRecord(recTyp, &rParam.InrRec));
		rParam.Direction  = 0; // export
		rParam.DataFormat = PPImpExpParam::dfSoap;
		rParam.TdfParam.Flags |= TextDbFile::fOemText/*|TextDbFile::fVerticalRec*/;
		rParam.TdfParam.FldDiv = ";";
		rParam.FileName = FileName;
		rParam.OtrRec = rParam.InrRec;
		CATCHZOK
		return ok;
	}
	int    SLAPI WriteHeader()
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
			//if(strlen(Filt.AddScheme)) {
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
	int    SLAPI WriteFooter()
	{
		int    ok = -1;
		if(F.IsValid()) {
			if(LinesRecCount)
				F.WriteLine("</d>");
			//if(!(Filt.Flags & SupplExpFilt::expFlatStruc)) {
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
	int    SLAPI WriteScheme(SdRecord * pRec, const char * pSchemeName)
	{
		int    ok = -1;
		if(F.IsValid() && pSchemeName) {
			SString buf;
			SString field_name, field_type;
			SdbField fld;
			// @v9.1.4 buf.Printf("<d name=\"%s\">", pSchemeName);
			(buf = 0).CatChar('<').Cat("d").Space().CatEqQ("name", pSchemeName).CatChar('>'); // @v9.1.4
			F.WriteLine(buf);
			for(uint i = 0; i < pRec->GetCount(); i++) {
				int    base_type = 0;
				THROW(pRec->GetFieldByPos(i, &fld));
				field_name = fld.Name;
				if(!field_name.NotEmptyS())
					field_name.Cat(fld.ID);
				base_type = stbase(fld.T.Typ);
				field_type = 0;
				if(base_type == BTS_DATE)
					field_type = "Date";
				else if(base_type == BTS_INT)
					field_type = "Integer";
				else if(base_type == BTS_REAL)
					field_type = "Currency";
				else if(base_type == BTS_STRING)
					field_type = "String";
				// @v9.1.4 buf.Printf("<f name=\"%s\" type=\"%s\"/>", (const char*)field_name, (const char*)field_type);
				(buf = 0).CatChar('<').Cat("f").Space().CatEqQ("name", field_name).Space().CatEqQ("type", field_type).Cat("/>"); // @v9.1.4
				F.WriteLine(buf);
			}
			ok = 1;
		}
		CATCHZOK
		return ok;
	}
	int    SLAPI WriteRec(SdRecord * pRec, const char * pSchemeName, int endRecord)
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
					F.WriteLine((buf = 0).CatTag("f", line));
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

	int    HeaderRecCount;
	int    LinesRecCount;
	int    FilesCount;
	uint   HeadRecType;
	uint   LineRecType;
	uint   AddLineRecType;
	//SupplExpFilt Filt;
	SString HeadScheme;
	SString LineScheme;
	SString AddLineScheme;
	SString SchemeName;
	SString FileName;
	SdRecord HeadRec;
	SdRecord LineRec;
	SdRecord AddLineRec;
	SFile  F;
};

void SLAPI PPSupplExchange_Baltika::DelFiles(const char * pFileName)
{
	SString wc_path, wild_card;
	SString full_path;
	SDirec sd;
	SDirEntry sde;
	SPathStruc sp;
	PPGetFilePath(PPPATH_OUT, pFileName, wc_path);
	MEMSZERO(sde);
	sp.Split(wc_path);
	sp.Nam.CatChar('*');
	sp.Merge(wild_card);
	sp.Merge(0, SPathStruc::fNam|SPathStruc::fExt, wc_path);
	for(sd.Init(wild_card); sd.Next(&sde) > 0;) {
		(full_path = wc_path).SetLastSlash().Cat(sde.FileName);
		SFile::Remove(full_path.cptr());
	}
}

int SLAPI PPSupplExchange_Baltika::Export(PPLogger & rLogger)
{
	int    ok = 1;
	SString buf;
	PPIniFile ini_file(0, 0, 0, 1);
	//const long filt_flags = Filt.Flags;
	SString client_code /*= Filt.ClientCode*/;
	Ep.GetExtStrData(Ep.extssClientCode, client_code);

	DelFiles("spprice.xml");
	DelFiles("sprest.xml");
	DelFiles("spbills.xml");
	DelFiles("spbills1.xml");
	DelFiles("spbills2.xml");
	DelFiles("spdlvadr.xml");
	// if(filt_flags & SupplExpFilt::expPrice) {
	if(P.Actions & P.opExportPrices) {
		THROW(ExportPrice());
	}
	//if(filt_flags & SupplExpFilt::expRest) {
	if(P.Actions & P.opExportStocks) {
		THROW(ExportRest());
		THROW(ExportRestParties());
	}
	//if(filt_flags & SupplExpFilt::expBills) {
	if(P.Actions & P.opExportBills) {
		THROW(ExportBills(expEtc, client_code, rLogger));
		if(ini_file.IsValid()) {
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWEAKALCCODE, buf) > 0) {
				//STRNSCPY(Filt.ClientCode, buf);
				THROW(ExportBills(expWeakAlc, buf, rLogger));
			}
			if(ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAWOTAREBEERCODE, buf) > 0) {
				//STRNSCPY(Filt.ClientCode, buf);
				THROW(ExportBills(expWoTareBeer, buf, rLogger));
			}
		}
		//STRNSCPY(Filt.ClientCode, client_code);
	}
	//if(filt_flags & SupplExpFilt::expSaldo) {
	if(P.Actions & P.opExportDebts) {
		PPIDArray excl_ar_list;
		SString temp_client_code = client_code;
		if(ini_file.IsValid() && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKASALDOCODE, (buf = 0)) > 0) {
			//STRNSCPY(Filt.ClientCode, buf);
			temp_client_code = buf;
		}
		if(ini_file.IsValid() && ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_BALTIKAEXCLSALDOCONTRAGENTS, (buf = 0)) > 0) {
			StringSet ss(',', buf);
			for(uint p = 0; ss.get(&p, (buf = 0)) > 0;)
				excl_ar_list.add(buf.ToLong());
			excl_ar_list.sort();
		}
		THROW(ExportSaldo2(excl_ar_list, temp_client_code, &rLogger)); // @v9.1.5 ExportSaldo-->ExportSaldo2
		//STRNSCPY(Filt.ClientCode, client_code);
	}
	THROW(Send());
	{
		PPSupplAgreement suppl_agr;
		THROW(ArObj.GetSupplAgreement(/*Filt.SupplID*/P.SupplID, &suppl_agr, 0) > 0);
		suppl_agr.Ep.LastDt = NZOR(/*Filt.Period.upp*/P.ExpPeriod.upp, getcurdate_()); // @v8.5.0 LConfig.OperDate-->getcurdate_()
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

//#define SOAP_SCHEMERESTEX         "CRMWhBalanceEx"
//#define SOAP_SCHEMEREST           "CRMWhBalance"
//#define SOAP_SCHEMERESTLINE       "CRMWhBalanceLine"
//#define SOAP_SCHEMERESTPARTEX     "CRMWhBalanceExParts"
//#define SOAP_SCHEMERESTPART       "CRMWhBalanceParam"
//#define SOAP_SCHEMERESTPARTLINE   "CRMWhBalanceParts"
//#define SOAP_SCHEMEBILLEX         "CRMDespatchEx"
//#define SOAP_SCHEMEBILL           "CRMDespatchParam"
//#define SOAP_SCHEMEBILLLINE       "CRMDespatch"
//#define SOAP_SCHEMEBILLLINELOT    "CRMDespatchParts"
//#define SOAP_SCHEMEDLVRADDR       "CRMClientAddress"
//#define SOAP_SCHEMEEXTDLVRADDR    "CRMExtClientAddressDef"
//#define SOAP_SCHEMESALDOEX        "CRMSaldoEx"
//#define SOAP_SCHEMESALDOAGGREGATE "CRMSaldoAggregate"
//#define SOAP_SCHEMESALDODOC       "CRMSaldoDoc"
//#define SOAP_SCHEMESALDOWARE      "CRMSaldoWare"

const char * SLAPI PPSupplExchange_Baltika::GetEaText() const
{
	//return (Filt.ProtVer == 0) ? "ea" : "кг";
	return (Ep.ProtVer == 0) ? "ea" : "кг";
}

int SLAPI PPSupplExchange_Baltika::ExportPrice()
{
	int    ok = 1;
	uint   count = 0;
	SString path;
	SString client_code;
	//SupplExpFilt se_filt;
	SoapExporter soap_e;
	//se_filt = Filt;
	//se_filt.MaxFileSizeKB = 0;
	{
		Goods2Tbl::Rec grec;
		GoodsIterator  giter(/*se_filt*/Ep.GoodsGrpID, 0);
		MEMSZERO(grec);
		PPGetFilePath(PPPATH_OUT, "spprice.xml", path);
		Ep.GetExtStrData(Ep.extssClientCode, client_code);
		soap_e.SetClientCode(client_code);
		THROW(soap_e.Init(path, PPREC_SUPPLPRICE, 0, "CRMWarePrice", 0/*, &se_filt*/, 0));
		for(;giter.Next(&grec) > 0;) {
			int    pack_code = 0;
			double bc_pack = 1.0;
			Sdr_SupplPrice rec;
			MEMSZERO(rec);
			if(GetBarcode(grec.ID, rec.WareId, sizeof(rec.WareId), &pack_code, 0, &bc_pack) > 0 && pack_code == 0) {
				THROW(BillObj->trfr->Rcpt.GetCurrentGoodsPrice(grec.ID, LConfig.Location, 0, &rec.Price));
				STRNSCPY(rec.PriceTypeId, "001");
				STRNSCPY(rec.UnitId, GetEaText());
				// @v8.9.8 {
				if(bc_pack > 1.0) {
					rec.Price = R5(rec.Price * bc_pack);
				}
				// } @v8.9.8
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
	const Sdr_Baltika_RestPartLine * p_i1 = (const Sdr_Baltika_RestPartLine*)i1;
	const Sdr_Baltika_RestPartLine * p_i2 = (const Sdr_Baltika_RestPartLine*)i2;
	int r = stricmp866(p_i1->WareId, p_i2->WareId);
	if(r == 0)
		r = stricmp866(p_i1->PartNumber, p_i2->PartNumber);
	return r;
}

void SLAPI PPSupplExchange_Baltika::GetInfoByLot(PPID lotID, PPTransferItem * pTi, LDATE * pBillDt, LDATE * pCreateDt, LDATE * pExpiry, SString * pSerial)
{
	//
	// @v8.6.10 Извлечение срока годности скорректировано так, чтобы приоритет был у даты, установленной у порожденного лота против оригинального
	//
	PPID   org_lot_id = 0;
	LDATE  crt_dt = ZERODATE;
	LDATE  expiry = pTi ? pTi->Expiry : ZERODATE;
	SString serial;
	ReceiptTbl::Rec lot, org_lot;
	MEMSZERO(lot);
	if(P_BObj->trfr->Rcpt.SearchOrigin(lotID, &org_lot_id, &lot, &org_lot) > 0) {
		ObjTagItem tag;
		if(PPRef->Ot.GetTag(PPOBJ_LOT, org_lot.ID, PPTAG_LOT_MANUFTIME, &tag) > 0) {
			LDATETIME create_dtm;
			tag.GetTimestamp(&create_dtm);
			crt_dt = create_dtm.d;
		}
		SETIFZ(expiry, org_lot.Expiry);
		// @v9.7.9 P_BObj->GetSerialNumberByLot(org_lot.ID, serial, 1);
		GetSerial(org_lot.ID, org_lot.GoodsID, serial); // @v9.7.9
	}
	else {
		// @v9.7.9 P_BObj->GetSerialNumberByLot(lotID, serial, 1);
		GetSerial(lot.ID, lot.GoodsID, serial); // @v9.7.9
	}
	// ASSIGN_PTR(pBillDt, lot.Dt);
	ASSIGN_PTR(pCreateDt, crt_dt);
	ASSIGN_PTR(pExpiry, expiry);
	ASSIGN_PTR(pSerial, serial);
}

int SLAPI PPSupplExchange_Baltika::ExportRestParties()
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
	TSArray <Sdr_Baltika_RestPartLine> wotarebeerrest_list;
	//SupplExpFilt  se_filt;
	GoodsRestFilt filt;
	SoapExporter soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest gr_view;
	PPIniFile ini_file(0, 0, 0, 1);

	//se_filt = Filt;
	//se_filt.MaxFileSizeKB = 0;
	PPGetFilePath(PPPATH_OUT, "sprestp.xml", path);
	MEMSZERO(item);
	filt.GoodsGrpID = Ep.GoodsGrpID;
	filt.Date       = P.ExpPeriod.upp ? P.ExpPeriod.upp : LConfig.OperDate;
	//
	filt.DiffParam = GoodsRestParam::_diffSerial;
	//filt.DiffParam = GoodsRestParam::_diffLotTag; // @v9.7.11
	//filt.DiffLotTagID = PPTAG_LOT_MANUFTIME; // @v9.7.11
	//
	if(P.LocList.IsEmpty()) {
		THROW(LocObj.GetWarehouseList(&loc_list));
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
		filt.LocList.FreeAll();
		filt.LocList.Add(loc_list.at(i));
		THROW(gr_view.Init_(&filt));
		file_name = path;
		if(i > 0) {
			SPathStruc sp;
			files_count++;
			sp.Split(file_name);
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
			THROW(soap_e.Init(file_name, PPREC_BALTIKA_RESTPART, PPREC_BALTIKA_RESTPARTLINE, "CRMWhBalanceParam",
				"CRMWhBalanceParts", /*&se_filt,*/ "CRMWhBalanceExParts"));
			// @v9.2.5 head_rec.SkipDelete = skip_delete; // (Filt.Flags & SupplExpFilt::expDelRecentBills) ? 0 : 1;
			head_rec.SkipDelete = (P.Flags & P.fDeleteRecentBills) ? 0 : 1; // @v9.2.5
			THROW(soap_e.AppendRecP(&head_rec, sizeof(head_rec), 0, 0, 0/*headRecForNewFile*/));
			// @v8.7.8 skip_delete = 1;
		}
		if(loc_list.at(i) != wotarebeer_locid) {
			TSArray <Sdr_Baltika_RestPartLine> items_list;
			for(gr_view.InitIteration(); gr_view.NextIteration(&item) > 0;) {
				double bc_pack = 1.0;
				Sdr_Baltika_RestPartLine line_rec;
				MEMSZERO(line_rec);
				if(sstrlen(item.Serial) && GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
					line_rec.Quantity = item.Rest;
					STRNSCPY(line_rec.UnitId, GetEaText());
					{
						//LDATETIME serial_dtm;
						//strtodatetime(item.Serial, &serial_dtm, DATF_DMY, TIMF_HMS);
						STRNSCPY(line_rec.PartNumber, item.Serial);
					}
					ltoa(loc_list.at(i), line_rec.WareHouseId, 10);
					GetInfoByLot(item.LotID, 0, &(line_rec.DocumentDate = filt.Date), &line_rec.ProductionDate, &line_rec.ExpirationDate, 0);
					// @v8.9.8 {
					if(bc_pack > 1.0) {
						line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
					}
					// } @v8.9.8
					if(wotarebeer_ggrpid && GObj.BelongToGroup(item.GoodsID, wotarebeer_ggrpid) > 0) {
						GetQtty(item.GoodsID, IsKegUnit(item.GoodsID), &line_rec.Quantity, 0);
						ltoa(wotarebeer_locid, line_rec.WareHouseId, 10);
						wotarebeerrest_list.insert(&line_rec);
					}
					else if(!tare_ggrpid || GObj.BelongToGroup(item.GoodsID, tare_ggrpid) <= 0) {
						uint idx = 0;
						if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)) > 0)
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
					skip_delete = 1; // @v8.7.8
				}
			}
		}
		else { // Экспортируем остатки разливного пива
			uint items_count = wotarebeerrest_list.getCount();
			TSArray <Sdr_Baltika_RestPartLine> items_list;
			for(uint j = 0; j < items_count; j++) {
				Sdr_Baltika_RestPartLine temp_item = wotarebeerrest_list.at(j);
				Sdr_Baltika_RestPartLine line_rec = temp_item;
				uint idx = 0;
				if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)) > 0)
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
						MEMSZERO(line_rec);
						if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
							uint   idx = 0;
							line_rec.Quantity = item.Rest;
							STRNSCPY(line_rec.UnitId, GetEaText());
							STRNSCPY(line_rec.PartNumber, item.Serial);
							ltoa(loc_list.at(i), line_rec.WareHouseId, 10);
							GetInfoByLot(item.LotID, 0, &(line_rec.DocumentDate = filt.Date), &line_rec.ProductionDate, &line_rec.ExpirationDate, 0);
							// @v8.9.8 {
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
							}
							// } @v8.9.8
							if(items_list.lsearch(&line_rec, &idx, PTR_CMPFUNC(Sdr_Baltika_RestPartLine)) > 0)
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
					skip_delete = 1; // @v8.7.8
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

int SLAPI PPSupplExchange_Baltika::ExportRest()
{
	int    ok = 1;
	uint   count = 0, files_count = 0, i;
	SString path;
	SString file_name;
	SString temp_buf;
	SString client_code;
	PPID   weakalc_locid = 0, weakalc_ggrpid = 0, wotarebeer_locid = 0, wotarebeer_ggrpid = 0, tare_ggrpid = 0;
	PPIDArray   loc_list, spoilage_loc_list;
	RAssocArray weakalcrest_list, wotarebeerrest_list;
	//SupplExpFilt  se_filt;
	GoodsRestFilt filt;
	SoapExporter  soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest   v;
	PPIniFile ini_file(0, 0, 0, 1);

	//se_filt = Filt;
	//se_filt.MaxFileSizeKB = 0;
	PPGetFilePath(PPPATH_OUT, "sprest.xml", path);
	MEMSZERO(item);
	filt.GoodsGrpID = /*se_filt.*/Ep.GoodsGrpID;
	filt.Date       = NZOR(/*se_filt.Period.*/P.ExpPeriod.upp, LConfig.OperDate);
	if(P.LocList.IsEmpty()) {
		THROW(LocObj.GetWarehouseList(&loc_list));
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
		filt.LocList.FreeAll();
		filt.LocList.Add(loc_list.at(i));
		THROW(v.Init_(&filt));
		file_name = path;
		if(i > 0) {
			SPathStruc sp;
			files_count++;
			sp.Split(file_name);
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
		MEMSZERO(head_rec);
		soap_e.SetClientCode(client_code);
		THROW(soap_e.Init(file_name, PPREC_SUPPLREST, PPREC_SUPPLRESTLINE, "CRMWhBalance", "CRMWhBalanceLine", /*&se_filt,*/ "CRMWhBalanceEx"));
		soap_e.SetMaxTransmitSize(P.MaxTransmitSize);
		ltoa(loc_list.at(i), head_rec.WareHouseId, 10);
		head_rec.DocumentDate = filt.Date;
		THROW(soap_e.AppendRecP(&head_rec, sizeof(head_rec), 0, 0, 0/*headRecForNewFile*/));
		if(!oneof2(loc_list.at(i), weakalc_locid, wotarebeer_locid)) {
			TSArray <Sdr_SupplRestLine> items_list;
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
					MEMSZERO(line_rec);
					if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
						uint   idx = 0;
						line_rec.Quantity = item.Rest;
						STRNSCPY(line_rec.UnitId, GetEaText());
						// @v8.9.8 {
						if(bc_pack > 1.0) {
							line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
						}
						// } @v8.9.8
						if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)) > 0)
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
			TSArray <Sdr_SupplRestLine> items_list;
			for(uint j = 0; j < items_count; j++) {
				PPID   goods_id = p_rest_list->at(j).Key;
				double rest     = p_rest_list->at(j).Val;
				double bc_pack  = 1.0;
				Sdr_SupplRestLine line_rec;
				MEMSZERO(line_rec);
				if(GetBarcode(goods_id, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
					uint idx = 0;
					line_rec.Quantity = rest;
					STRNSCPY(line_rec.UnitId, GetEaText());
					// @v8.9.8 {
					if(bc_pack > 1.0) {
						line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
					}
					// } @v8.9.8
					if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)) > 0)
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
						MEMSZERO(line_rec);
						if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
							uint idx = 0;
							line_rec.Quantity = item.Rest;
							STRNSCPY(line_rec.UnitId, GetEaText());
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
							}
							if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)) > 0)
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

int SLAPI PPSupplExchange_Baltika::ExportSpoilageRest(PPID locID, uint filesIdx)
{
	int    ok = 1;
	int    is_weak_alc = 0, is_wotare = 0;
	long   count = 0;
	PPID   wotarebeer_ggrpid = 0;
	Sdr_SupplRest     head_rec;
	GoodsRestViewItem rest_item;
	TSArray <Sdr_SupplRestLine> items_list;
	//SupplExpFilt      se_filt;
	GoodsRestFilt     filt;
	SoapExporter      soap_e;
	GoodsRestViewItem item;
	PPViewGoodsRest   v;

	//se_filt = Filt;
	//se_filt.MaxFileSizeKB = 0;
	MEMSZERO(item);
	filt.GoodsGrpID = /*se_filt.*/Ep.GoodsGrpID;
	filt.Date       = NZOR(/*se_filt.Period.*/P.ExpPeriod.upp, LConfig.OperDate);
	filt.Flags      = GoodsRestFilt::fNullRest;
	THROW(GetWeakAlcInfo(0, &wotarebeer_ggrpid, 1));
	THROW(v.Init_(&filt));
	for(v.InitIteration(); v.NextIteration(&rest_item) > 0;) {
		double bc_pack = 1.0;
		Sdr_SupplRestLine line_rec;
		MEMSZERO(line_rec);
		if(GetBarcode(item.GoodsID, line_rec.WareId, sizeof(line_rec.WareId), 0, 0, &bc_pack) > 0) {
			uint idx = 0;
			if(wotarebeer_ggrpid && GObj.BelongToGroup(item.GoodsID, wotarebeer_ggrpid) > 0) {
				GetQtty(item.GoodsID, IsKegUnit(item.GoodsID), &item.Rest, 0);
				is_wotare = 1;
			}
			line_rec.Quantity = item.Rest;
			// @v8.9.8 {
			if(!is_wotare && bc_pack > 1.0) {
				line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
			} // } @v8.9.8
			STRNSCPY(line_rec.UnitId, GetEaText());
			if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplRestLine, WareId)) > 0)
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
		SPathStruc sp;

		sp.Split(file_name);
		sp.Nam.Cat(filesIdx);
		sp.Merge(file_name = 0);

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

int SLAPI PPSupplExchange_Baltika::GetBarcode(PPID goodsID, char * pBuf, size_t bufSize, int * pPackCode, int * pIsHoreca, double * pMult)
{
	int    ok = -1, pack_code = 0, is_horeca = 0;
	double mult = 1.0;
	int32  arcode_pack = 0;
	SString barcode, temp_buf;
	GObj.P_Tbl->GetArCode(P.SupplID, goodsID, barcode, &arcode_pack);
	if(barcode.Empty()) {
		BarcodeArray barcode_list;
		GObj.ReadBarcodes(goodsID, barcode_list);
		for(uint i = 0; barcode.Empty() && i < barcode_list.getCount(); i++) {
			temp_buf = barcode_list.at(i).Code;
			if(temp_buf.C(0) == '=' || temp_buf.C(0) == '*') {
				barcode = temp_buf;
				mult = barcode_list.at(i).Qtty; // @v8.9.8
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
	Sdr_BaltikaBillItemAttrs * p_i1 = (Sdr_BaltikaBillItemAttrs*)i1;
	Sdr_BaltikaBillItemAttrs * p_i2 = (Sdr_BaltikaBillItemAttrs*)i2;
	int r = stricmp866(p_i1->DocumentNumber, p_i2->DocumentNumber);
	if(r == 0)
		r = stricmp866(p_i1->WareId, p_i2->WareId);
	if(r == 0)
		r = stricmp866(p_i1->PartNumber, p_i2->PartNumber);
	return r;
}

int SLAPI PPSupplExchange_Baltika::ExportBills(const BillExpParam & rExpParam, const char * pClientCode, PPLogger & rLogger)
{
	int    ok = 1;
	int    is_first = 1, from_consig_loc = 0;
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
	PPID   order_number_tag_id = 0; // @v9.3.2
	LDATE  consig_parent_dt = ZERODATE;
	SString path, log_msg;
	SString consig_parent_code;
	SString ord_num;
	SString bill_text;
	SString db_uuid_text;
	SString horeca_code;
	SString encode_str;
	SString temp_buf;
	Sdr_SupplBill head_rec;
	SoapExporter  soap_e(true /*flatStruc*/);
	LAssocArray dlvr_addr_list;
	BillFilt filt;
	PPIDArray loss_op_list, invrcpt_op_list, spoilage_loc_list;
	TSArray <Sdr_BaltikaBillItemAttrs> items_attrs_list;
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
	// @v9.3.2 {
	{
		PPObjectTag ord_no_tag_rec;
		PPID   temp_id = 0;
		if(obj_tag.SearchBySymb("BALTIKA-ORDER-NO", &temp_id, &ord_no_tag_rec) > 0) {
			if(ord_no_tag_rec.ObjTypeID == PPOBJ_BILL && ord_no_tag_rec.TagDataType == OTTYP_STRING)
				order_number_tag_id = ord_no_tag_rec.ID;
		}
	}
	// } @v9.3.2
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
	MEMSZERO(item);
	THROW(v.Init_(&filt));
	if(Ep.ProtVer == 0) {
		soap_e.SetAddedScheme(PPREC_SUPPLDLVRADDR, "CRMClientAddress");
	}
	else {
		soap_e.SetAddedScheme(PPREC_SUPPLDLVRADDR, "CRMExtClientAddressDef");
	}
	soap_e.SetClientCode(pClientCode);
	THROW(soap_e.Init(path, PPREC_SUPPLBILL, PPREC_SUPPLBILLLINE, PPREC_BALTIKABILLITEMATTRS,
		"CRMDespatchParam", "CRMDespatch", "CRMDespatchParts", "CRMDespatchEx"));
	soap_e.SetMaxTransmitSize(P.MaxTransmitSize);
	//
	MEMSZERO(head_rec);
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
		ord_num = 0;
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
		TSArray <Sdr_SupplBillLine> items_list;
		THROW(P_BObj->ExtractPacket(item.ID, &bpack));
		PPObjBill::MakeCodeString(&bpack.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, bill_text);
		if(bpack.GetOrderList(ord_list) > 0) {
			BillTbl::Rec bill_rec;
			MEMSZERO(bill_rec);
			if(P_BObj->Search(ord_list.at(0), &bill_rec) > 0) {
				ord_num = bill_rec.Code;
				ord_dt = bill_rec.Dt;
				if(spoilage_loc_list.bsearch(bill_rec.LocID, 0) > 0)
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
		// @v9.3.2 {
		else if(IsOpBelongTo(item.OpID, Ep.RetOp) && order_number_tag_id) {
            if(PPRef->Ot.GetTagStr(PPOBJ_BILL, item.ID, order_number_tag_id, temp_buf) > 0) {
                if(temp_buf.Divide(' ', ord_num, log_msg) > 0) {
                    ord_num.Strip();
                    if(ord_num.NotEmptyS()) {
						log_msg.Strip();
						LDATE   temp_dt = ZERODATE;
						if(strtodate(log_msg, DATF_DMY, &temp_dt) && checkdate(temp_dt, 0))
                            ord_dt = temp_dt;
						else
							ord_num = 0;
                    }
                }
                else
					ord_num = 0;
            }
		}
		// } @v9.3.2
		MEMSZERO(psn_rec);
		if(!oneof2(item.OpID, Ep.MovOutOp, Ep.MovInOp)) {
			client_id = ObjectToPerson(bpack.Rec.Object, 0);
			if(bpack.P_Freight && bpack.P_Freight->DlvrAddrID)
				dlvr_addr_id = bpack.P_Freight->DlvrAddrID;
			else if(PsnObj.Search(client_id, &psn_rec) > 0)
				dlvr_addr_id = (psn_rec.RLoc) ? psn_rec.RLoc : psn_rec.MainLoc;
			/* Сначала нужно выяснить, будем ли выгружать данную накладную. Поэтому блок перенесен
			if(client_id && dlvr_addr_id)
				if(dlvr_addr_list.lsearch(&dlvr_addr_id, 0, PTR_CMPFUNC(long), sizeof(long)) <= 0)
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
			else if(loss_op_list.lsearch(item.OpID) > 0)
				doc_type_idx = BALTIKA_DOCTYPES_LOSSES;
			else if(invrcpt_op_list.lsearch(item.OpID) > 0)
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
					else
						GetInfoFromMemo(item.Memo, &ord_dt, ord_num, 1);
				}
				// @v8.7.5 {
				else if(oneof2(doc_type_idx, BALTIKA_DOCTYPES_MOVINGTO, BALTIKA_DOCTYPES_MOVINGFROM)) {
					ord_num = bpack.Rec.Code;
					ord_dt = bpack.Rec.Dt;
				}
				// } @v8.7.5
			}
		}
		if(doc_type_str.NotEmpty()) {
			PPID   obj_id = oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp) ? 0L : bpack.Rec.Object;
			PPID   loc_id = bpack.Rec.LocID;
			PPID   loc2_id = PPObjLocation::ObjToWarehouse(bpack.Rec.Object);
			RetailPriceExtractor rtl_price_extr(bpack.Rec.LocID, 0, obj_id, ZERODATETIME, RTLPF_GETCURPRICE);
			RetailPriceExtractor::ExtQuotBlock eqb(Ep.PriceQuotID);
			RetailPriceExtractor price_by_quot_extr(bpack.Rec.LocID, &eqb, obj_id, ZERODATETIME, RTLPF_PRICEBYQUOT);
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
					MEMSZERO(line_rec);
					MEMSZERO(line_attrs_rec);
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
						STRNSCPY(line_rec.SrcCRMDbId, pClientCode); // @v8.7.4
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
								MEMSZERO(line_rec.WareHouseId);
								ltoa(loc_id, line_rec.WareHouseId, 10);
							}
							{
								SString buf, sale_channel;
								ObjTagItem tag;
								WorldTbl::Rec     wrec;
								LocationTbl::Rec  loc_rec;
								MEMSZERO(loc_rec);
								MEMSZERO(wrec);
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
									QuotIdent qi(bpack.Rec.Dt, bpack.Rec.LocID, Ep.PriceQuotID, 0, obj_id);
									if(GetCurGoodsPrice(trfr_item.GoodsID, bpack.Rec.LocID, 0, &lot_price, &lot_rec) > 0) {
										use_quot = BIN(GObj.GetQuotExt(trfr_item.GoodsID, qi, lot_rec.Cost, lot_price, &_price, 1) > 0);
									}
								}
								if(!use_quot) {
									SETIFZ(_price, trfr_item.NetPrice());
									const float pct_dis = (atol(line_rec.AddressRegionType) != 1) ? P.SpcDisPct2 : P.SpcDisPct1;
									_price -= (_price / 100.0) * pct_dis;
								}
								_price  = round(_price, 2);
							}
							GetQtty(trfr_item.GoodsID, IsKegUnit(trfr_item.GoodsID), &qtty_val, &_price);
							line_rec.Quantity = qtty_val;
							line_rec.Price    = _price;
							if(bc_pack > 1.0) {
								line_rec.Quantity = R6(line_rec.Quantity / bc_pack);
								line_rec.Price = R5(line_rec.Price * bc_pack);
							}
							doc_type_str.CopyTo(line_rec.DocumentTypeId, sizeof(line_rec.DocumentTypeId));
							if(items_list.lsearch(line_rec.WareId, &idx, PTR_CMPFUNC(PcharNoCase), offsetof(Sdr_SupplBillLine, WareId)) > 0)
								items_list.at(idx).Quantity += line_rec.Quantity;
							else
								items_list.insert(&line_rec);
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
								if(items_attrs_list.lsearch(&line_attrs_rec, &idx, PTR_CMPFUNC(Sdr_BaltikaBillItemAttrs)))
									items_attrs_list.at(idx).Quantity += line_rec.Quantity;
								else
									items_attrs_list.insert(&line_attrs_rec);
							}
							count++;
						}
					}
				}
			}
			if(items_list.getCount() > 0) {
				int add_link_op = 0;
				SString add_link_op_str;
				if(oneof2(item.OpID, Ep.MovInOp, Ep.MovOutOp)) {
					add_link_op = 1;
					if(item.OpID == Ep.MovInOp)
						PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGFROM, add_link_op_str);
					else {
						//
						// Межскладские перемещения документов с консигнационного склада будем разбивать, на мескладской расход и приход товара от поставщика
						//
						if(from_consig_loc)
							PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_RECEIPT, add_link_op_str);
						else
							PPGetSubStr(PPTXT_BALTIKA_DOCTYPES, BALTIKA_DOCTYPES_MOVINGTO, add_link_op_str);
					}
				}
				for(uint i = 0; i < items_list.getCount(); i++) {
					Sdr_SupplBillLine line_rec = items_list.at(i);
					THROW(soap_e.AppendRecT(PPREC_SUPPLBILLLINE, &line_rec, sizeof(line_rec), is_first, 1, 0/*pSchemeName*/));
					is_first = 0;
					(log_msg = 0).Cat("code=[").Cat(line_rec.DocumentNumber).Cat("]").Space().Space().Space().Cat("order=[").Cat(line_rec.CRMOrderNumber).Cat("]");
					rLogger.Log(log_msg);
					if(add_link_op) {
						add_link_op_str.CopyTo(line_rec.DocumentTypeId, sizeof(line_rec.DocumentTypeId));
						MEMSZERO(line_rec.WareHouseId);
						ltoa(loc2_id, line_rec.WareHouseId, 10);
						if(from_consig_loc) {
							consig_parent_code.CopyTo(line_rec.CRMOrderNumber, sizeof(line_rec.CRMOrderNumber));
							line_rec.CRMOrderDate = consig_parent_dt;
							ltoa(baltika_id, line_rec.CompanyId, 10);
						}
						THROW(soap_e.AppendRecT(PPREC_SUPPLBILLLINE, &line_rec, sizeof(line_rec), is_first, 1, 0/*pSchemeName*/));
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
			is_first = 1;
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
				MEMSZERO(loc_rec);
				if(LocObj.Search(dlvr_addr_id, &loc_rec) > 0) {
					PPID   client_id = dlvr_addr_list.at(i).Key;
					Sdr_SupplDlvrAddr addr_rec;
					MEMSZERO(addr_rec);
					GetObjectName(PPOBJ_PERSON, client_id, cli_name = 0);
					// specencodesym=%01,294520000;%02,294520003
					// %01478954         294520003780100
					(code = 0).EncodeString(loc_rec.Code, /*Filt.EncodeStr*/encode_str, 1).CopyTo(addr_rec.CRMClientId, sizeof(addr_rec.CRMClientId));
					ltoa(client_id, addr_rec.CompanyId, 10);
					cli_name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(addr_rec.CompanyName, sizeof(addr_rec.CompanyName));
					ltoa(dlvr_addr_id, addr_rec.AddressId, 10);
					(temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER);
					STRNSCPY(addr_rec.AddressName, temp_buf);
					LocObj.P_Tbl->GetAddress(loc_rec, 0, addr);
					addr.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(addr_rec.Location, sizeof(addr_rec.Location));
					THROW(soap_e.AppendRecT(PPREC_SUPPLDLVRADDR, &addr_rec, sizeof(addr_rec), BIN(j == 0), 0, 0/*pSchemeName*/));
					j++;
					if(GetDlvrAddrHorecaCode(&dlvr_addr_id, (code = 0)) > 0) {
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
	}
	CATCHZOK
	soap_e.EndDocument(ok > 0 && count);
	//Filt.Flags &= ~SupplExpFilt::expFlatStruc;
	return ok;
}

static int _WriteScheme(SXml::WDoc & rXmlDoc, SdRecord & rRd, const char * pSchemeName)
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
				field_type = 0;
				if(base_type == BTS_DATE)
					field_type = "Date";
				else if(base_type == BTS_INT)
					field_type = "Integer";
				else if(base_type == BTS_REAL)
					field_type = "Currency";
				else if(base_type == BTS_STRING)
					field_type = "String";
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

static int _WriteRec(SXml::WDoc & rXmlDoc, SdRecord & rRd, const void * pDataBuf)
{
	assert(pDataBuf);
	int    ok = 1;
	SString value_buf;
	{
		SXml::WNode w_r(rXmlDoc, "r");
		SFormatParam fp;
		fp.FDate = DATF_ISO8601|DATF_CENTURY;
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
	//CATCHZOK
	return ok;
}

int SLAPI PPSupplExchange_Baltika::ExportSaldo2(const PPIDArray & rExclArList, const char * pClientCode, PPLogger * pLog)
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
    Sdr_BaltikaSaldoAggregate sdr_saldo_aggr;
    Sdr_BaltikaSaldoDoc sdr_saldo_doc;
    Sdr_BaltikaSaldoWare sdr_saldo_ware;

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
									ArObj.P_Tbl->PersonToArticle(psn_rec.ID, acs_id, &ar_id);
									MEMSZERO(sdr_saldo_aggr);
									sdr_saldo_aggr.SaldoDate = _curdt; //           date                "Дата, за которую экспортируются данные"; // @v9.1.4
									temp_buf.Z().Cat(psn_rec.ID);
									STRNSCPY(sdr_saldo_aggr.CompanyId, temp_buf);
									temp_buf.Z().Cat(loc_id);
									STRNSCPY(sdr_saldo_aggr.AddressId, temp_buf); // zstring(24)         "Код клиента";
									// @optional sdr_saldo_aggr.ProductId            zstring(24)         "Код продукта";
									// @optional sdr_saldo_aggr.ProductName          zstring(128)        "Название продукта";
									if(ArObj.GetClientAgreement(ar_id, &cli_agt, 0) > 0) {
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
											MEMSZERO(sdr_saldo_doc);
											temp_buf.Z().Cat(psn_rec.ID);
											STRNSCPY(sdr_saldo_doc.CompanyId, temp_buf);
											temp_buf.Z().Cat(loc_id);
											STRNSCPY(sdr_saldo_doc.AddressId, temp_buf);
											BillCore::GetCode(temp_buf = bill_rec.Code);
											STRNSCPY(sdr_saldo_doc.DocumentNumber, temp_buf);
											sdr_saldo_doc.DocumentDate = bill_rec.Dt;
											sdr_saldo_doc.ActionDate = bill_rec.Dt;
											{
												LDATE  last_pay_date = ZERODATE;
												P_BObj->P_Tbl->GetLastPayDate(bill_id, &last_pay_date);
												sdr_saldo_doc.PaymentDate = last_pay_date;
												sdr_saldo_doc.OverduePeriod = checkdate(last_pay_date, 0) ? diffdate(_curdt, last_pay_date) : 0;
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
									const PPID ar_id = p_cli_list->at(i).Id;
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
											MEMSZERO(sdr_saldo_ware);
											if(GetBarcode(goods_id, sdr_saldo_ware.WareId, sizeof(sdr_saldo_ware.WareId), 0, 0, &bc_pack) > 0) {
												//sdr_saldo_ware.Date = (Filt.Period.upp && Filt.Period.upp < _curdt) ? Filt.Period.upp : _curdt;
												sdr_saldo_ware.Date = (P.ExpPeriod.upp && P.ExpPeriod.upp < _curdt) ? P.ExpPeriod.upp : _curdt;
												for(uint locidx = 0; locidx < dlvr_loc_list.getCount(); locidx++) {
													const PPID dlvr_loc_id = dlvr_loc_list.get(locidx);
													if(P_BObj->GetGoodsSaldo(goods_id, ar_id, dlvr_loc_id, sdr_saldo_ware.Date, MAXLONG, &sdr_saldo_ware.Quantity, &sdr_saldo_ware.Summ) > 0 && sdr_saldo_ware.Quantity != 0.0) {
														//sdr_saldo_ware.Quantity = 1.0; // @debug
														//sdr_saldo_ware.Summ = 1.0;      // @debug
														if(sdr_saldo_ware.Quantity < 0.0) { // @v8.8.0
															sdr_saldo_ware.Quantity = fabs(sdr_saldo_ware.Quantity);
															sdr_saldo_ware.Summ = fabs(sdr_saldo_ware.Summ);
														}
														// @v8.8.0 {
														else {
															sdr_saldo_ware.Quantity = 0.0;
															sdr_saldo_ware.Summ = 0.0;
														}
														// } @v8.8.0
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

int SLAPI PPSupplExchange_Baltika::GetInfoFromMemo(const char * pMemo, LDATE * pParentDt, SString & rParentCode, int simple)
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

int SLAPI PPSupplExchange_Baltika::GetConsigLocInfo(BillViewItem * pItem, PPID consigLocGrpID, LDATE * pParentDt, SString & rParentCode)
{
	int    ok = 0;
	rParentCode = 0;
	ASSIGN_PTR(pParentDt, ZERODATE);
	if(pItem && consigLocGrpID) {
		PPObjLocation obj_loc;
		if(obj_loc.IsMemberOfGroup(pItem->LocID, consigLocGrpID) > 0)
			ok = GetInfoFromMemo(pItem->Memo, pParentDt, rParentCode);
	}
	return ok;
}

PPID SLAPI PPSupplExchange_Baltika::GetConsigLocGroupID()
{
	PPID loc_grp_id = 0L;
	PPObjLocation obj_loc;
	LocationFilt loc_filt(LOCTYP_WAREHOUSEGROUP);
	StrAssocArray * p_list = obj_loc.MakeList_(&loc_filt, 1);
	for(uint i = 0; !loc_grp_id && i < p_list->getCount(); i++) {
		LocationTbl::Rec loc_rec;
		if(obj_loc.Search(p_list->at(i).Id, &loc_rec) > 0 && sstreqi_ascii(loc_rec.Code, "CONSIGNATION"))
			loc_grp_id = loc_rec.ID;
	}
	ZDELETE(p_list);
	return loc_grp_id;
}

PPID SLAPI PPSupplExchange_Baltika::GetSaleChannelTagID()
{
	PPID   sale_channel_tag = 0;
	SString sale_channel_tag_symb;
	SArray * p_tags_list = 0;
	PPObjTag   obj_tag;
	sale_channel_tag_symb = "SaleChannel";
	if(p_tags_list = obj_tag.CreateList(0, 0)) {
   		for(uint i = 0; !sale_channel_tag && i < p_tags_list->getCount(); i++) {
   			const PPID tag_id = *(PPID *)p_tags_list->at(i);
			PPObjectTag tag_kind;
			if(obj_tag.Search(tag_id, &tag_kind) > 0 && sale_channel_tag_symb.CmpPrefix(tag_kind.Symb, 1) == 0)
				sale_channel_tag = tag_id;
		}
	}
	ZDELETE(p_tags_list);
	return sale_channel_tag;
}

long SLAPI PPSupplExchange_Baltika::GetSaleChannelExtFld()
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

int SLAPI PPSupplExchange_Baltika::GetWeakAlcInfo(PPID * pLocID, PPID * pGGrpID, int getWoTareBeer)
{
	PPID   loc_id = 0, ggrp_id = 0;
	PPIniFile ini_file;
	if(ini_file.IsValid()) {
		uint inipar_locsymb  = (oneof2(getWoTareBeer, 1, 2)) ? PPINIPARAM_BALTIKAWOTAREBEERLOCSYMB : PPINIPARAM_BALTIKAWEAKALCLOCSYMB;
		uint inipar_ggrpcode = (getWoTareBeer == 1) ? PPINIPARAM_BALTIKAWOTAREBEERGGRPCODE : ((getWoTareBeer == 2) ? PPINIPARAM_BALTIKATAREGGRPCODE : PPINIPARAM_BALTIKAWEAKALCGGRPCODE);
		SString loc_symb, ggrp_code;
		BarcodeTbl::Rec bc_rec;
		PPObjGoodsGroup obj_ggrp;
		MEMSZERO(bc_rec);
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
int SLAPI PPSupplExchange_Baltika::IsKegUnit(PPID goodsID)
{
	int    yes = 0;
	if(KegUnitID < 0) {
		PPIniFile ini_file;
		int    unit_id = 0;
		if(ini_file.GetInt((uint)PPINISECT_CONFIG, PPINIPARAM_BALTIKAUNITKEGID, &unit_id) > 0) {
			PPObjUnit u_obj;
			PPUnit u_rec;
			if(u_obj.Fetch(unit_id, &u_rec) <= 0)
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

int SLAPI PPSupplExchange_Baltika::GetSpoilageLocList(PPIDArray * pList)
{
	int    ok = -1;
	PPIniFile ini_file;
	if(ini_file.IsValid()) {
		SString loc_symb_list;
		BarcodeTbl::Rec bc_rec;
		MEMSZERO(bc_rec);
		ini_file.Get((uint)PPINISECT_CONFIG, PPINIPARAM_BALTIKASPOILAGELOCSYMB, loc_symb_list);
		{
			SString loc_symb;
			StringSet ss(",");
			ss.setBuf(loc_symb_list, loc_symb_list.Len() + 1);
			for(uint p = 0; ss.get(&p, loc_symb) > 0;) {
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

int SLAPI PPSupplExchange_Baltika::GetQtty(PPID goodsID, int calcByPhPerU, double * pQtty, double * pPrice)
{
	double qtty = (pQtty) ? *pQtty : 0;
	double price = (pPrice) ? *pPrice : 0;
	if(calcByPhPerU) {
		double phuperu = 0.0;
		PPGoodsPacket g_pack;
		if(GObj.GetPacket(goodsID, &g_pack, PPObjGoods::gpoSkipQuot) > 0 && g_pack.Rec.GdsClsID && g_pack.ExtRec.W) { // @v8.3.7 PPObjGoods::gpoSkipQuot
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

int SLAPI PPSupplExchange_Baltika::GetDlvrAddrHorecaCode(PPID * pDlvrAddrID, SString & rCode)
{
	rCode = 0;
	int    ok = -1;
	PPID   dlvr_addr_id = (pDlvrAddrID && *pDlvrAddrID) ? *pDlvrAddrID : 0;
	if(dlvr_addr_id) {
		LocationTbl::Rec loc_rec;
		MEMSZERO(loc_rec);
		if(DlvrAddrExtFldID && LocObj.Search(dlvr_addr_id, &loc_rec) > 0 && LocationCore::GetExField(&loc_rec, DlvrAddrExtFldID, rCode) > 0 && rCode.Len()) {
			dlvr_addr_id += HORECA_DLVRADDR_OFFS;
			ok = 1;
		}
	}
	ASSIGN_PTR(pDlvrAddrID, dlvr_addr_id);
	return ok;
}

int SLAPI PPSupplExchange_Baltika::Send()
{
	/*
	int ok = 1;
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

int SLAPI EditSupplExpFilt(SupplExpFilt * pFilt, int selOnlySuppl)
{
	#define GRP_LOC 1

	class SupplExpFiltDialog : public TDialog {
	public:
		SupplExpFiltDialog(int selOnlySuppl) : TDialog(DLG_SUPLEXPFLT), SelOnlySuppl(selOnlySuppl)
		{
			SetupCalPeriod(CTLCAL_SUPLEXPFLT_PRD, CTL_SUPLEXPFLT_PRD);
			addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_SUPLEXPFLT_LOC, 0, 0, cmLocList, 0, 0, 0));
		}
		int    setDTS(const SupplExpFilt * pData)
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
			setGroupData(GRP_LOC, &loc_rec);
			disableCtrls(SelOnlySuppl, CTL_SUPLEXPFLT_FLAGS, CTLSEL_SUPLEXPFLT_OP, CTLSEL_SUPLEXPFLT_GGRP, CTL_SUPLEXPFLT_PRD, CTLSEL_SUPLEXPFLT_LOC, 0);
			enableCommand(cmLocList, !SelOnlySuppl);
			enableCommand(cmaMore,   !SelOnlySuppl);
			return ok;
		}
		int    getDTS(SupplExpFilt * pData)
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
				THROW(getGroupData(GRP_LOC, &loc_rec));
				Data.LocList = loc_rec.LocList;
				getCtrlData(sel = CTL_SUPLEXPFLT_DISCOUNT1, &Data.PctDis1);
				THROW_PP(Data.PctDis1 >= 0 && Data.PctDis1 <= 100, PPERR_PERCENTINPUT);
				getCtrlData(sel = CTL_SUPLEXPFLT_DISCOUNT2, &Data.PctDis2);
				THROW_PP(Data.PctDis2 >= 0 && Data.PctDis2 <= 100, PPERR_PERCENTINPUT);
			}
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
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
					Data.IP     = (ulong)suppl_agr.Ep.ConnAddr;
					// @v8.5.0 Data.TechID = suppl_agr.ExchCfg.TechID;
					Data.PriceQuotID = suppl_agr.Ep.PriceQuotID;
					// @v8.5.0 {
					suppl_agr.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssClientCode, temp_buf.Z());
					STRNSCPY(Data.ClientCode, temp_buf);
					// } @v8.5.0
					// @v8.5.0 STRNSCPY(Data.ClientCode, suppl_agr.ExchCfg.ClientCode);
				}
			}
			else if(event.isCmd(cmaMore)) {
			}
			else
				return;
			clearEvent(event);
		}
		const int SelOnlySuppl;
		SupplExpFilt Data;
		PPObjArticle ArObj;
	};
	DIALOG_PROC_BODY_P1ERR(SupplExpFiltDialog, selOnlySuppl, pFilt)
}
//
//
//
class SupplInterchangeFiltDialog : public TDialog {
public:
	SupplInterchangeFiltDialog() : TDialog(DLG_SUPPLIX)
	{
		SetupCalPeriod(CTLCAL_SUPPLIX_EXPPRD, CTL_SUPPLIX_EXPPRD);
		addGroup(GRP_LOC, new LocationCtrlGroup(CTLSEL_SUPPLIX_LOC, 0, 0, cmLocList, 0, 0, 0));
	}
	int    setDTS(const SupplInterchangeFilt * pData)
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
			setGroupData(GRP_LOC, &loc_rec);
		}
		Data.GetExtStrData(Data.extssParam, temp_buf); // @v9.5.0
		setCtrlString(CTL_SUPPLIX_ADDPARAM, temp_buf); // @v9.5.0
		//CATCHZOK
		return ok;
	}
	int    getDTS(SupplInterchangeFilt * pData)
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
			THROW(getGroupData(GRP_LOC, &loc_rec));
			Data.LocList = loc_rec.LocList;
		}
		getCtrlString(CTL_SUPPLIX_ADDPARAM, temp_buf); // @v9.5.0
		Data.PutExtStrData(Data.extssParam, temp_buf); // @v9.5.0
		ASSIGN_PTR(pData, Data);
		CATCH
			ok = PPErrorByDialog(this, sel);
		ENDCATCH
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
	SupplInterchangeFilt Data;
	PPObjArticle ArObj;
};
//
// Import suppl data
//
int SLAPI PPSupplExchange_Baltika::Import(const char * pPath)
{
#if 0 // @unused {
	class Exchanger : public PpyInetDataPrcssr {
	public:
		int SLAPI SendRequest(const char * pURL, const char * request)
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
				ShellExecute(0, "open", "c:\\temp\\1\\answer.html", NULL, NULL, SW_SHOWNORMAL);
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
		for(uint i = 0, j = 0; ss.get(&i, (buf = 0)) > 0; j++) {
			if(j != 0) {
				ResolveGoodsItem gitem;
				StringSet ss2("<f>");
				ss2.setBuf(buf, buf.Len() + 1);
				for(uint k = 0, m = 0; ss2.get(&k, buf) > 0; m++) {
					uint p = 0;
					StringSet ss3("</f>");
					ss3.setBuf(buf, buf.Len() + 1);
					ss3.get(&p, buf);
					if(buf.Len() && buf.CmpPrefix("</r>", 1) != 0) {
						buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(m == 1)
							buf.CopyTo(gitem.Barcode, sizeof(gitem.Barcode));
						else if(m == 2)
							buf.CopyTo(gitem.GoodsName, sizeof(gitem.GoodsName));
						else if(m == 5)
							gitem.Quantity = buf.ToReal();
					}
				}
				if(strlen(gitem.Barcode) > 0 && strlen(gitem.GoodsName) > 0 && GObj.SearchByBarcode(gitem.Barcode, 0) <= 0 && GObj.P_Tbl->SearchByArCode(P.SupplID, gitem.Barcode, 0) <= 0)
					THROW_SL(goods_list.insert(&gitem));
			}
		}
		if(goods_list.getCount() && ResolveGoodsDlg(&goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_SHOWQTTY|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG) > 0) {
			for(uint i = 0; i < goods_list.getCount(); i++) {
				ResolveGoodsItem gitem = goods_list.at(i);
				if(gitem.ResolvedGoodsID && strlen(gitem.Barcode) > 0) {
					int r = 0;
					Goods2Tbl::Rec grec;
					MEMSZERO(grec);
					THROW(r = GObj.P_Tbl->SearchByArCode(P.SupplID, gitem.Barcode, 0));
					if(r < 0) {
						THROW(GObj.P_Tbl->SetArCode(gitem.ResolvedGoodsID, P.SupplID, gitem.Barcode, (int32)gitem.Quantity, 1));
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
	SLAPI  iSalesPepsi(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	SLAPI ~iSalesPepsi();

	int    SLAPI Init(/*PPID arID*/);
	void   SLAPI GetLogFileName(SString & rFileName) const;
	int    SLAPI ParseResultString(const char * pText, TSCollection <iSalesPepsi::ResultItem> & rList, long * pErrItemCount) const;

	int    SLAPI ReceiveGoods(int forceSettings, int useStorage);
	int    SLAPI ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult);
	int    SLAPI ReceiveReceipts();
	int    SLAPI ReceiveOrders();
	int    SLAPI ReceiveUnclosedInvoices(TSCollection <iSalesBillDebt> & rResult);

	int    SLAPI SendPrices();
	int    SLAPI SendStocks();
	int    SLAPI SendInvoices();
	int    SLAPI SendDebts();

	int    SLAPI SendStatus(const TSCollection <iSalesTransferStatus> & rList);
private:
	int    SLAPI PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	int    SLAPI Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <iSalesBillPacket> & rList);
	int    SLAPI Helper_MakeBillEntry(PPID billID, int outerDocType, TSCollection <iSalesBillPacket> & rList);
	void   SLAPI Helper_Make_iSalesIdent(const BillTbl::Rec & rRec, int outerDocType, SString & rIdent) const;
	void   SLAPI Helper_Parse_iSalesIdent(const SString & rIdent, SString & rCode, LDATE * pDate) const;
	int    SLAPI GetGoodsStoreFileName(SString & rBuf) const;
	int    SLAPI StoreGoods(TSCollection <iSalesGoodsPacket> & rList);
	int    SLAPI RestoreGoods(TSCollection <iSalesGoodsPacket> & rList);
	int    SLAPI LogErrors(TSCollection <iSalesPepsi::ResultItem> & rResultList, const SString * pMsg);
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
		stInited             = 0x0001,
		stEpDefined          = 0x0002,
		stGoodsMappingInited = 0x0004
	};
	long   State;
	SDynLibrary * P_Lib;
	void * P_DestroyFunc;
	SString SvcUrl;
	SString UserName;
	SString Password;
	SString LastMsg;
	SString LogFileName;
	TSCollection <iSalesGoodsPacket> GoodsMapping;
	PPObjPerson PsnObj;
	PPLogger & R_Logger;
};


SLAPI iSalesPepsi::iSalesPepsi(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger)
{
	State = 0;
	P_DestroyFunc = 0;
	PPGetFilePath(PPPATH_LOG, "isalespepsi.log", LogFileName);
 	{
		SString lib_path;
		PPGetFilePath(PPPATH_BIN, "PPSoapPepsi.dll", lib_path);
		P_Lib = new SDynLibrary(lib_path);
		if(P_Lib && !P_Lib->IsValid()) {
			ZDELETE(P_Lib);
		}
		if(P_Lib)
			P_DestroyFunc = (void *)P_Lib->GetProcAddr("iSalesDestroyResult");
	}
}

SLAPI iSalesPepsi::~iSalesPepsi()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

void SLAPI iSalesPepsi::GetLogFileName(SString & rFileName) const
{
	rFileName = LogFileName;
}

int SLAPI iSalesPepsi::Init(/*PPID arID*/)
{
	State = 0;
	SvcUrl = 0;
	UserName = 0;
	Password = 0;
	int    ok = 1;
	{
        Ep.GetExtStrData(Ep.extssRemoveAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		State |= stEpDefined;
	}
	InitGoodsList(0);
	State |= stInited;
	return ok;
}

int SLAPI iSalesPepsi::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL iSalesPepsi::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		((UHTT_DESTROYRESULT)P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

int SLAPI iSalesPepsi::ParseResultString(const char * pText, TSCollection <iSalesPepsi::ResultItem> & rList, long * pErrItemCount) const
{
	int    ok = 1;
	long   err_item_count = 0;
	if(isempty(pText)) {
		ok = -1;
	}
	else {
		StringSet ss("\x0D\x0A");
		StringSet ss_sub("|");
		ss.setBuf(pText, strlen(pText)+1);
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
                	if(temp_buf.CmpNC("OK") == 0)
						p_new_item->Status = 1;
					else if(temp_buf.CmpNC("ERR") == 0) {
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

int SLAPI iSalesPepsi::LogErrors(TSCollection <iSalesPepsi::ResultItem> & rResultList, const SString * pMsg)
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
    char   Signature[4]; // "ISGS"
    uint32 CRC;
    uint8  Reserve[24];
};

int SLAPI iSalesPepsi::GetGoodsStoreFileName(SString & rBuf) const
{
	return PPGetFilePath(PPPATH_OUT, "isalesgoodsstorage", rBuf = 0);
}

int SLAPI iSalesPepsi::StoreGoods(TSCollection <iSalesGoodsPacket> & rList)
{
    int    ok = 1;
    SString file_name;
    SSerializeContext sctx;
    SBuffer buffer;
    THROW_SL(TSCollection_Serialize(rList, +1, buffer, &sctx));
    {
    	const  size_t bsize = buffer.GetAvailableSize();
    	iSalesGoodsStorageHeader hdr;
        MEMSZERO(hdr);
        memcpy(hdr.Signature, "ISGS", 4);
        CRC32 cc;
        hdr.CRC = cc.Calc(0, (const uint8 *)buffer.GetBuf(0), bsize);
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

int SLAPI iSalesPepsi::RestoreGoods(TSCollection <iSalesGoodsPacket> & rList)
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
			CRC32 cc;
			uint32 _crc = cc.Calc(0, (const uint8 *)buffer.GetBuf(0), buffer.GetAvailableSize());
			THROW(_crc == hdr.CRC);
		}
		THROW_SL(TSCollection_Serialize(rList, -1, buffer, &sctx));
    }
    CATCHZOK
    return ok;
}

int SLAPI iSalesPepsi::ReceiveGoods(int forceSettings, int useStorage)
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
	DateRange goods_query_period; // @v9.6.3
	goods_query_period.SetZero(); // @v9.6.3
	if(useStorage && fileExists(strg_file_name)) {
		storage_exists = 1;
		LDATETIME mt;
        if(SFile::GetTime(strg_file_name, 0, 0, &mt) && diffdate(getcurdate_(), mt.d) <= 2) {
			if(RestoreGoods(GoodsMapping)) {
				goods_query_period.low = mt.d; // @v9.6.3
				State |= stGoodsMappingInited;
				ok = 2;
			}
        }
	}
	if(ok < 0 || forceSettings) { // @v9.7.6 (|| forceSettings)
		SString tech_buf;
		Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
		{
			//PPTXT_LOG_SUPPLIX_IMPGOODS_S  "Импорт товаров поставщика @zstr '@article'"
			PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_S, &msg_buf, tech_buf.cptr(), P.SupplID);
			//R_Logger.Log(msg_buf);
			PPWaitMsg(msg_buf);
		}
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = (ISALESGETGOODSLIST_PROC)P_Lib->GetProcAddr("iSalesGetGoodsList"));
		sess.Setup(SvcUrl);
		// @v9.7.7 {
		if(forceSettings && P.ExpPeriod.low)
			goods_query_period = P.ExpPeriod;
		// } @v9.7.7
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
			// @v9.6.3 THROW_SL(TSCollection_Copy(GoodsMapping, *p_result));
			// @v9.6.3 {
			for(uint nidx = 0; nidx < p_result->getCount(); nidx++) {
				const iSalesGoodsPacket * p_np = p_result->at(nidx);
				int _found = 0;
				for(uint pidx = 0; pidx < GoodsMapping.getCount(); pidx++) {
					iSalesGoodsPacket * p_pp = GoodsMapping.at(pidx);
					if(p_pp && p_pp->OuterCode == p_np->OuterCode) {
                        *p_pp = *p_np;
                        _found = 1;
					}
				}
				if(!_found) {
                    iSalesGoodsPacket * p_new_item = GoodsMapping.CreateNewItem();
					THROW_SL(p_new_item);
					*p_new_item = *p_np;
				}
			}
			// } @v9.6.3
			State |= stGoodsMappingInited;
			ok = 1;
			if(useStorage) {
				StoreGoods(GoodsMapping);
			}
			{
				//PPTXT_LOG_SUPPLIX_IMPGOODS_E  "Импортировано @int товаров поставщика @zstr '@article'"
				PPFormatT(PPTXT_LOG_SUPPLIX_IMPGOODS_E, &msg_buf, (long)p_result->getCount(), tech_buf.cptr(), P.SupplID);
				//R_Logger.Log(msg_buf);
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
							long native_id = p_item->NativeCode.ToLong();
							Goods2Tbl::Rec goods_rec;
							if(native_id && GObj.Search(native_id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS && !(goods_rec.Flags & GF_GENERIC)) {
								int32  arcode_pack = 0;
								GObj.P_Tbl->GetArCode(P.SupplID, native_id, ar_code, &arcode_pack);
								(temp_buf = p_item->OuterCode).Transf(CTRANSF_UTF8_TO_INNER);
								if(ar_code != temp_buf) {
									THROW(GObj.P_Tbl->SetArCode(native_id, P.SupplID, temp_buf, 1));
									// PPTXT_LOG_SUPPLIX_SETARCODE   "Для товара '@goods' установлен код по статье '@article' =@zstr"
									R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_SETARCODE, &msg_buf, native_id, P.SupplID, temp_buf.cptr()));
								}
							}
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
		DestroyResult((void **)&p_result);
	}
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::ReceiveReceipts()
{
    int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString src_receipt_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;
	DateRange period = P.ExpPeriod;
	if(!checkdate(period.low, 0))
		period.low = encodedate(1, 1, 2016);
	if(!checkdate(period.upp, 0))
		period.upp = encodedate(31, 12, 2030);
	SString tech_buf;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	{
		//PPTXT_LOG_SUPPLIX_IMPRCPT_S   "Импорт DESADV от поставщика @zstr '@article'"
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPRCPT_S, &msg_buf, tech_buf.cptr(), P.SupplID);
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETRECEIPTLIST_PROC)P_Lib->GetProcAddr("iSalesGetReceiptList"));
	sess.Setup(SvcUrl);

	p_result = func(sess, UserName, Password, &period, /*1*/BIN(P.Flags & P.fRepeatProcessing) /*inclProcessedItems*/);
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		//PPTXT_LOG_SUPPLIX_IMPRCPT_E   "Импортировано @int DESADV поставщика @zstr '@article'"
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPRCPT_E, &msg_buf, (long)p_result->getCount(), tech_buf.cptr(), P.SupplID);
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	if(p_result->getCount()) {
		PPAlbatrosConfig acfg;
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
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d, 0) ? p_src_pack->Dtm.d : getcurdate_();
					STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &ex_bill_id, &ex_bill_rec) > 0) {
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
						{
							pack.Rec.EdiOp = PPEDIOP_DESADV;
							pack.BTagL.PutItemStr(PPTAG_BILL_EDICHANNEL, "ISALES-PEPSI");
							pack.BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, p_src_pack->iSalesId);
						}
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						// @v9.3.1 R_Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcReceipt;
							(p_new_status->Ident = 0).Cat(p_src_pack->iSalesId);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult((void **)&p_result);
    CATCHZOK
    return ok;
}

int SLAPI iSalesPepsi::ReceiveOrders()
{
    int    ok = -1;
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	SString src_order_code; // Необходимо сохранять дабы после акцепта сформировать подтверждение
	TSCollection <iSalesBillPacket> * p_result = 0;
	TSCollection <iSalesTransferStatus> status_list; // Список статусов приема заказов. Это список отправляется серверу в ответ на прием заказов
	ISALESGETORDERLIST_PROC func = 0;

	DateRange period;
	period.Set(encodedate(1, 1, 2016), encodedate(31, 12, 2030));

	SString tech_buf;
	Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
	{
		//PPTXT_LOG_SUPPLIX_IMPORD_S    "Импорт заказов @zstr"
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_S, &msg_buf, tech_buf.cptr());
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETORDERLIST_PROC)P_Lib->GetProcAddr("iSalesGetOrderList"));
	sess.Setup(SvcUrl);
	{
		const int do_incl_processed_items = BIN(P.Flags & P.fRepeatProcessing);
		p_result = func(sess, UserName, Password, &period, do_incl_processed_items);
	}
	THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
	{
		//PPTXT_LOG_SUPPLIX_IMPORD_E    "Импортировано @int заказов @zstr"
		PPFormatT(PPTXT_LOG_SUPPLIX_IMPORD_E, &msg_buf, (long)p_result->getCount(), tech_buf.cptr());
		//R_Logger.Log(msg_buf);
		PPWaitMsg(msg_buf);
	}
	if(p_result->getCount()) {
		SString srcloc_attr_pattern = "ORD_GROUPCODE1";

		PPAlbatrosConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			//PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const iSalesBillPacket * p_src_pack = p_result->at(i);
				if(p_src_pack && p_src_pack->Status == 0) { // @v9.3.2 (&& p_src_pack->Status == 0) отмененные заказы не проводить
					BillTbl::Rec ex_bill_rec;
					PPID   ex_bill_id = 0;
					PPBillPacket pack;
					Goods2Tbl::Rec goods_rec;
					PPBillPacket::SetupObjectBlock sob;

					src_order_code = p_src_pack->Code;
					const  PPID   _src_loc_id = p_src_pack->SrcLocCode.ToLong();
					//
					temp_buf = p_src_pack->PayerCode;
					temp_buf.ShiftLeftChr('W'); // @v9.2.8
					temp_buf.ShiftLeftChr('w'); // @v9.2.8
					const  PPID   _src_psn_id = temp_buf.ToLong();
					//
					const  PPID   _src_dlvrloc_id = p_src_pack->ShipTo.ToLong();
					const  PPID   _src_agent_id = p_src_pack->AgentCode.ToLong();
					PPID   ar_id = 0;
					PPID   loc_id = 0;
					// @v9.3.1 {
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
					// } @v9.3.1
					if(!loc_id && _src_loc_id && LocObj.Fetch(_src_loc_id, &loc_rec) > 0 && loc_rec.Type == LOCTYP_WAREHOUSE)
						loc_id = loc_rec.ID;
					THROW(pack.CreateBlank_WithoutCode(acfg.Hdr.OpID, 0, loc_id, 1));
					STRNSCPY(pack.Rec.Code, p_src_pack->Code);
					pack.Rec.Dt = checkdate(p_src_pack->Dtm.d, 0) ? p_src_pack->Dtm.d : getcurdate_();
					pack.Rec.DueDate = checkdate(p_src_pack->IncDtm.d, 0) ? p_src_pack->IncDtm.d : ZERODATE; // @v9.2.6
					STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
					if(_src_psn_id && ArObj.P_Tbl->PersonToArticle(_src_psn_id, op_rec.AccSheetID, &ar_id) > 0) {
						if(!pack.SetupObject(ar_id, sob)) {
							R_Logger.LogLastError();
						}
                        if(_src_dlvrloc_id && LocObj.Search(_src_dlvrloc_id, &loc_rec) > 0 &&
							loc_rec.Type == LOCTYP_ADDRESS && (!loc_rec.OwnerID || loc_rec.OwnerID == _src_psn_id)) {
                            PPFreight freight;
                            freight.DlvrAddrID = _src_dlvrloc_id;
                            pack.SetFreight(&freight);
						}
						else {
							//PPTXT_LOG_SUPPLIX_DLVRLOCNID   "Не удалость идентифицировать адрес доставки документа '@zstr' по идентификатору @int"
							R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_DLVRLOCNID, &msg_buf, (const char *)pack.Rec.Code, _src_dlvrloc_id));
						}
					}
					else {
						//PPTXT_LOG_SUPPLIX_CLINID    "Не удалость идентифицировать контрагента документа '@zstr' по идентификатору @int"
						R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_CLINID, &msg_buf, (const char *)pack.Rec.Code, _src_psn_id));
					}
					if(_src_agent_id && ArObj.P_Tbl->PersonToArticle(_src_agent_id, GetAgentAccSheet(), &(ar_id = 0)) > 0) {
						pack.Ext.AgentID = ar_id;
					}
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &ex_bill_id, &ex_bill_rec) > 0) {
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
									else {
										//PPTXT_LOG_SUPPLIX_GOODSNID     "Не удалость идентифицировать товар для документа '@zstr' по идентификатору @int"
										R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_GOODSNID, &msg_buf, (const char *)pack.Rec.Code, _src_goods_id));
									}
								}
							}
						}
						{
							pack.Rec.EdiOp = PPEDIOP_SALESORDER;
							pack.BTagL.PutItemStr(PPTAG_BILL_EDICHANNEL, "ISALES-PEPSI");
							pack.BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, p_src_pack->iSalesId);
						}
						pack.InitAmounts();
						THROW(P_BObj->TurnPacket(&pack, 1));
						// @v9.3.1 R_Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
						{
							iSalesTransferStatus * p_new_status = status_list.CreateNewItem();
							THROW_SL(p_new_status);
							p_new_status->Ifc = iSalesTransferStatus::ifcOrder;
							(p_new_status->Ident = 0).CatChar('O').Cat(src_order_code);
						}
					}
				}
			}
			if(status_list.getCount()) {
				THROW(SendStatus(status_list));
			}
		}
	}
	DestroyResult((void **)&p_result);
    CATCHZOK
    return ok;
}

int SLAPI iSalesPepsi::ReceiveUnclosedInvoices(TSCollection <iSalesBillDebt> & rResult)
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
	period.Set(encodedate(1, 1, 2016), encodedate(31, 12, 2030));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETUNCLOSEDBILLLIST_PROC)P_Lib->GetProcAddr("iSalesGetUnclosedBillList"));
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
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::ReceiveRouts(TSCollection <iSalesRoutePacket> & rResult)
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
	period.Set(encodedate(1, 1, 2016), encodedate(31, 12, 2030));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW_SL(func = (ISALESGETROUTELIST_PROC)P_Lib->GetProcAddr("iSalesGetRouteList"));
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
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendStatus(const TSCollection <iSalesTransferStatus> & rList)
{
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	if(rList.getCount()) {
		int    result = 0;
		ISALESPUTTRANSFERSTATUS_PROC func = 0;
		THROW_SL(func = (ISALESPUTTRANSFERSTATUS_PROC)P_Lib->GetProcAddr("iSalesPutTransferStatus"));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &rList);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
		DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
		DestroyResult((void **)&p_result);
    }
    CATCHZOK
	return ok;
}

IMPL_CMPFUNC(iSalesBillDebt, p1, p2)
{
	const iSalesBillDebt * b1 = (const iSalesBillDebt *)p1;
	const iSalesBillDebt * b2 = (const iSalesBillDebt *)p2;
	int    si = cmp(b1->Dtm, b2->Dtm);
	SETIFZ(si, stricmp866(b1->Code, b2->Code));
	return si;
}

int SLAPI iSalesPepsi::SendDebts()
{
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	TSCollection <iSalesBillDebt> outer_debt_list;
	{
		THROW(ReceiveUnclosedInvoices(outer_debt_list));
	}
	if(outer_debt_list.getCount()) {
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
                        if(GetOpType(bill_rec.OpID) != PPOPT_GOODSORDER) { // @v9.3.2
							BillCore::GetCode(temp_buf = bill_rec.Code);
							int    found = 0;
							for(uint j = first_idx_by_date; !found && j < i; j++) {
								const iSalesBillDebt * p_temp_item = outer_debt_list.at(j);
								if(p_temp_item->Code.CmpNC(temp_buf) == 0) {
									double payment = 0.0;
									P_BObj->P_Tbl->CalcPayment(bill_rec.ID, 1, 0, 0, &payment);
									iSalesBillDebt * p_new_item = current_debt_list.CreateNewItem();
									THROW_SL(p_new_item);
									*p_new_item = *p_temp_item;
									p_new_item->Code.Transf(CTRANSF_INNER_TO_UTF8);
									if(payment >= p_new_item->Amount)
										p_new_item->Debt = 0.0;
									else
										p_new_item->Debt = p_new_item->Amount - payment;
									found = 1;
								}
							}
                        }
					}
					//
					first_idx_by_date = i;
				}
				if(i < _c) {
					prev_date = p_outer_bill->Dtm.d;
				}
			}
		}
		if(current_debt_list.getCount()) {
			ISALESPUTDEBTSETTLEMENT_PROC func = 0;
			THROW_SL(func = (ISALESPUTDEBTSETTLEMENT_PROC)P_Lib->GetProcAddr("iSalesPutDebtSettlement"));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &current_debt_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			// @v9.3.1 {
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
			// } @v9.3.1
			// @v9.3.1 DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
			DestroyResult((void **)&p_result);
		}
    }
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendPrices()
{
	// CustomerPricesTransfer
	int    ok = -1;
	SString * p_result = 0;
	PPSoapClientSession sess;
	SString temp_buf;
	//TSCollection <iSalesStockCountingWhPacket> outp_packet;
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
						ss_outer_cli_code.add(p_visit->InnerClientCode);
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
			if(sp_obj.GetPacket(Ep.Fb.StyloPalmID, &sp_pack) > 0) {
                if(sp_pack.QkList.GetCount()) {
                    for(uint i = 0; i < sp_pack.QkList.GetCount(); i++) {
						const PPID qk_id = sp_pack.QkList.Get(i);
                        plq_list.addnz(qk_id);
                    }
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
						const QuotIdent qi(0, plq_id, 0, 0);
						GObj.GetQuotExt(goods_rec.ID, qi, lot_cost, lot_price, &price, 0);
					}
					else {
						if(Ep.PriceQuotID) {
							const QuotIdent qi(0, Ep.PriceQuotID, 0, 0);
							GObj.GetQuotExt(goods_rec.ID, qi, lot_cost, lot_price, &price, 0);
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
			THROW_SL(func = (ISALESPUTPRICES_PROC)P_Lib->GetProcAddr("iSalesPutPrices"));
			sess.Setup(SvcUrl);
			p_result = func(sess, UserName, Password, &pl_list);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			// @v9.3.1 {
			{
				long   err_item_count = 0;
				SString tech_buf;
				SString msg_buf;
				Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
				TSCollection <iSalesPepsi::ResultItem> result_list;
				ParseResultString(*p_result, result_list, &err_item_count); // @v9.5.1 &err_item_count
				/* @v9.5.1
				{
					for(uint i = 0; i < result_list.getCount(); i++) {
						const ResultItem * p_result_item = result_list.at(i);
						if(p_result_item && p_result_item->Status == 0)
							err_item_count++;
					}
				}
				*/
				PPFormatT(PPTXT_LOG_SUPPLIX_EXPPRICE_E, &msg_buf, tech_buf.cptr(), P.SupplID, err_item_count);
				PPWaitMsg(msg_buf);
				if(err_item_count)
					LogErrors(result_list, &msg_buf);
			}
			// } @v9.3.1
			// } @v9.3.1 DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
			DestroyResult((void **)&p_result);
		}
    }
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendStocks()
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
	gr_filt.LocList = P.LocList; // @v9.3.0
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
				p_new_item->UnitCode = 1004/*0*/;
			}
		}
    }
    {
		SString * p_result = 0;
		ISALESPUTSTOCKCOUNTING_PROC func = 0;
		THROW_SL(func = (ISALESPUTSTOCKCOUNTING_PROC)P_Lib->GetProcAddr("iSalesPutStockCounting"));
		sess.Setup(SvcUrl);
		p_result = func(sess, UserName, Password, &outp_packet);
		THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
        DS.Log(LogFileName, *p_result, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER|LOGMSGF_NODUPFORJOB);
		{
			long   err_item_count = 0;
			TSCollection <iSalesPepsi::ResultItem> result_list;
			ParseResultString(*p_result, result_list, &err_item_count); // @v9.5.1 &err_item_count
			{
				SString tech_buf;
				Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
				/* @v9.5.1
				{
					for(uint i = 0; i < result_list.getCount(); i++) {
						const ResultItem * p_result_item = result_list.at(i);
						if(p_result_item && p_result_item->Status == 0)
							err_item_count++;
					}
				}
				*/
				//PPTXT_LOG_SUPPLIX_EXPSTOCK_E   "Экспортированы остатки поставщику @zstr '@article'. Количество элементов с ошибками: @int"
				PPFormatT(PPTXT_LOG_SUPPLIX_EXPSTOCK_E, &msg_buf, tech_buf.cptr(), P.SupplID, err_item_count);
				PPWaitMsg(msg_buf);
				if(err_item_count)
					LogErrors(result_list, &msg_buf);
			}
		}
		DestroyResult((void **)&p_result);
    }
    CATCHZOK
	return ok;
}

void SLAPI iSalesPepsi::Helper_Make_iSalesIdent(const BillTbl::Rec & rRec, int outerDocType, SString & rIdent) const
{
	rIdent = 0;
	BillCore::GetCode(rIdent = rRec.Code);
	rIdent.Space().Cat(rRec.Dt, DATF_GERMAN|DATF_CENTURY).Space().Cat(labs(outerDocType));
}

void SLAPI iSalesPepsi::Helper_Parse_iSalesIdent(const SString & rIdent, SString & rCode, LDATE * pDate) const
{
	rCode = 0;
	LDATE   dt = ZERODATE;
	const char * p = rIdent;
	while(*p && *p != ' ') {
		rCode.CatChar(*p);
		p++;
	}
	if(*p == ' ') {
		SString temp_buf;
		dt = strtodate_(p, DATF_GERMAN);
		if(!checkdate(dt, 0))
			dt = ZERODATE;
	}
	ASSIGN_PTR(pDate, dt);
}
//
// Если outerDocType < 0, то это - отмена документа
//
int SLAPI iSalesPepsi::Helper_MakeBillEntry(PPID billID, int outerDocType, TSCollection <iSalesBillPacket> & rList)
{
	int    ok = 1;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	int    do_cancel = BIN(outerDocType < 0);
	outerDocType = labs(outerDocType);
	PPBillPacket pack;
	if(P_BObj->ExtractPacket(billID, &pack) > 0) {
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
			if(bill_ack_tag_id) // @v9.5.7
				pack.BTagL.GetItemStr(bill_ack_tag_id, cancel_code);
			if(cancel_code.Empty())
				do_cancel = 0;
		}
		else {
			for(TiIter tiiter(&pack, ETIEF_UNITEBYGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
				//const PPTransferItem & r_ti = pack.ConstTI(i);
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
			cli_face_code = 0;
			cli_addr_code = 0;
			const  PPID psn_id = ObjectToPerson(pack.Rec.Object, 0);
			// @v9.5.5 {
			PPID   special_qk_id = 0;
			{
				PPObjTag tag_obj;
				PPObjectTag tag_rec;
				for(uint tagidx = 0; !special_qk_id && tagidx < pack.BTagL.GetCount(); tagidx++) {
                    const ObjTagItem * p_tag_item = pack.BTagL.GetItemByPos(tagidx);
                    if(p_tag_item && p_tag_item->TagDataType == OTTYP_OBJLINK) {
						if(tag_obj.Fetch(p_tag_item->TagID, &tag_rec) > 0 && tag_rec.TagEnumID == PPOBJ_QUOTKIND)
							special_qk_id = p_tag_item->Val.IntVal;
                    }
				}
			}
			// } @v9.5.5
			// @v9.2.8 'W' {
			if(oneof2(outerDocType, 1, 5))
				cli_face_code.CatChar('W');
			// }
			cli_face_code.Cat(psn_id);
			if(pack.P_Freight && pack.P_Freight->DlvrAddrID)
				cli_addr_code.Cat(pack.P_Freight->DlvrAddrID);
			iSalesBillPacket * p_new_pack = rList.CreateNewItem();
			THROW_SL(p_new_pack);
			p_new_pack->NativeID = pack.Rec.ID;
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
			p_new_pack->ExtCode = 0;
			p_new_pack->ExtDtm.SetZero();
			//
			BillCore::GetCode(p_new_pack->Code = pack.Rec.Code);
			p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
			// @v9.6.2 Debug_TestUtfText(p_new_pack->Code, "makebillentry-1", R_Logger); // @v9.5.11
			p_new_pack->Dtm.Set(pack.Rec.Dt, ZEROTIME);
			// @v9.4.3 {
			if(outerDocType == 6 && pack.Rec.LinkBillID) {
				BillTbl::Rec link_bill_rec;
				if(P_BObj->Search(pack.Rec.LinkBillID, &link_bill_rec) > 0 && GetOpType(link_bill_rec.OpID) == PPOPT_DRAFTRECEIPT) {
					BillCore::GetCode(p_new_pack->Code = link_bill_rec.Code);
					p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
					// @v9.6.2 Debug_TestUtfText(p_new_pack->Code, "makebillentry-2", R_Logger); // @v9.5.11
					p_new_pack->Dtm.Set(link_bill_rec.Dt, ZEROTIME);
				}
			}
			// } @v9.4.3
			{
				if(cancel_code.NotEmpty()) {
					p_new_pack->iSalesId = cancel_code;
					p_new_pack->iSalesId.Transf(CTRANSF_INNER_TO_UTF8);
					// @v9.6.2 Debug_TestUtfText(p_new_pack->iSalesId, "makebillentry-7", R_Logger); // @v9.5.11
					// @v9.3.10 {
					LDATE   prev_date;
					Helper_Parse_iSalesIdent(cancel_code, temp_buf, &prev_date);
					if(temp_buf.NotEmptyS() && checkdate(prev_date, 0)) {
						p_new_pack->Code = temp_buf;
						p_new_pack->Code.Transf(CTRANSF_INNER_TO_UTF8);
						// @v9.6.2 Debug_TestUtfText(p_new_pack->Code, "makebillentry-3", R_Logger); // @v9.5.11
						p_new_pack->Dtm.Set(prev_date, ZEROTIME);
					}
					// } @v9.3.10
				}
				else {
					Helper_Make_iSalesIdent(pack.Rec, outerDocType, p_new_pack->iSalesId);
					p_new_pack->iSalesId.Transf(CTRANSF_INNER_TO_UTF8);
					// @v9.6.2 Debug_TestUtfText(p_new_pack->iSalesId, "makebillentry-7/2", R_Logger); // @v9.5.11
				}
			}
			p_new_pack->IncDtm.SetZero();
			pack.Pays.GetLast(&p_new_pack->DueDate, 0, 0);
			p_new_pack->Status = do_cancel ? 1 : 0;
			// @v9.4.9 (p_new_pack->Memo = pack.Rec.Memo).Transf(CTRANSF_INNER_TO_UTF8);
			p_new_pack->Memo = 0; // @v9.4.9
			if(outerDocType == 6) { // Приход (подтверждение)
				(p_new_pack->SellerCode = 0).Cat(psn_id);
				(p_new_pack->ShipFrom = 0).Cat(psn_id);
				p_new_pack->PayerCode = own_code;
				p_new_pack->ShipTo = own_code;
				p_new_pack->DestLocCode.Cat(pack.Rec.LocID);
				if(pack.BTagL.GetItemStr(PPTAG_BILL_EDIIDENT, temp_buf) > 0) {
					(p_new_pack->iSalesId = temp_buf).Transf(CTRANSF_INNER_TO_UTF8); // @v9.5.11 Transf(CTRANSF_INNER_TO_UTF8)
				}
				{
					iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
					THROW_SL(p_new_ref);
					p_new_ref->DocType = 13;
					BillCore::GetCode((p_new_ref->Code = 0).CatChar('O').Cat(p_new_pack->Code));
					p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
					// @v9.6.2 Debug_TestUtfText(p_new_ref->Code, "makebillentry-4", R_Logger); // @v9.5.11
					p_new_ref->Dtm.SetZero();
				}
			}
			else if(outerDocType == 5) { // Возврат
				p_new_pack->SellerCode = cli_face_code;
				p_new_pack->ShipFrom = cli_addr_code;
				p_new_pack->PayerCode = own_code;
				p_new_pack->ShipTo = own_code;
				p_new_pack->DestLocCode.Cat(pack.Rec.LocID);
				//
				const int transmit_linkret_as_unlink = 1;

				BillTbl::Rec sell_rec;
				if(!transmit_linkret_as_unlink && GetOpType(pack.Rec.OpID) == PPOPT_GOODSRETURN && pack.Rec.LinkBillID && P_BObj->Search(pack.Rec.LinkBillID, &sell_rec) > 0) {
					iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
					THROW_SL(p_new_ref);
					p_new_ref->DocType = 1;
					BillCore::GetCode(p_new_ref->Code = sell_rec.Code);
					p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
					// @v9.6.2 Debug_TestUtfText(p_new_ref->Code, "makebillentry-5", R_Logger); // @v9.5.11
					p_new_ref->Dtm.Set(sell_rec.Dt, ZEROTIME);
				}
				else {
					p_new_pack->DocType = 4; // Не привязанный возврат имеет отдельный тип
				}
			}
			else { // Продажа
				p_new_pack->SellerCode = own_code;
				p_new_pack->ShipFrom = own_code;
				p_new_pack->PayerCode = cli_face_code;
				p_new_pack->ShipTo = cli_addr_code;
				p_new_pack->SrcLocCode.Cat(pack.Rec.LocID);
				//
				PPIDArray order_id_list;
				pack.GetOrderList(order_id_list);
				for(uint ordidx = 0; ordidx < order_id_list.getCount(); ordidx++) {
					const PPID ord_id = order_id_list.get(ordidx);
					BillTbl::Rec ord_rec;
					if(ord_id && P_BObj->Search(ord_id, &ord_rec) > 0 && ord_rec.EdiOp == PPEDIOP_SALESORDER) {
						if(PPRef->Ot.GetTagStr(PPOBJ_BILL, ord_id, PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.CmpNC("ISALES-PEPSI") == 0) {
							THROW(P_BObj->ExtractPacket(ord_id, &order_pack) > 0);
							is_own_order = 1;
							{
								iSalesBillRef * p_new_ref = p_new_pack->Refs.CreateNewItem();
								THROW_SL(p_new_ref);
								p_new_ref->DocType = 13;
								BillCore::GetCode(p_new_ref->Code = ord_rec.Code);
								p_new_ref->Code.Transf(CTRANSF_INNER_TO_UTF8);
								// @v9.6.2 Debug_TestUtfText(p_new_ref->Code, "makebillentry-6", R_Logger); // @v9.5.11
								p_new_ref->Dtm.SetZero();
							}
						}
					}
				}
			}
			if(pack.Ext.AgentID) {
				const PPID agent_psn_id = ObjectToPerson(pack.Ext.AgentID, 0);
				if(agent_psn_id)
					p_new_pack->AgentCode.Cat(agent_psn_id);
			}
			if(p_sj && p_sj->GetObjCreationEvent(PPOBJ_BILL, pack.Rec.ID, &sj_rec) > 0)
				p_new_pack->CreationDtm.Set(sj_rec.Dt, sj_rec.Tm);
			else
				p_new_pack->CreationDtm.SetZero();
			{
				int    is_creation = 0;
				LDATETIME moment;
				if(p_sj && p_sj->GetLastObjModifEvent(PPOBJ_BILL, pack.Rec.ID, &moment, &is_creation, &sj_rec) > 0 && !is_creation)
					p_new_pack->LastUpdDtm = moment;
				else
					p_new_pack->LastUpdDtm.SetZero();
			}
			long   tiiterpos = 0;
			for(TiIter tiiter(&pack, ETIEF_UNITEBYGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
				tiiterpos++;
				uint   pos_list_item_pos = 0;
				if(ti_pos_list.Get(tiiterpos, temp_buf) > 0) {
					Goods2Tbl::Rec goods_rec;
					if(GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
						const iSalesGoodsPacket * p_goods_entry = SearchGoodsMappingEntry(temp_buf);
						double ord_part_dis = 0.0;
						double net_price = ti.NetPrice();
						double special_net_price = 0.0; // @v9.5.5
						if(feqeps(net_price, 0.0, 1E-2)) { // @v9.6.5 1E-3-->1E-2
							ord_part_dis = 1.0;
							net_price = ti.Price;
						}
						else {
							if(outerDocType != 6) {
								if(special_qk_id) {
									double quot = 0.0;
									const QuotIdent qi(ti.LocID, special_qk_id, 0, pack.Rec.Object); // @v9.4.12 0-->pack.Rec.Object
									if(GObj.GetQuotExt(ti.GoodsID, qi, &quot, 0) > 0 && quot > 0.0)
										special_net_price = quot;
								}
								if(Ep.PriceQuotID && special_net_price == 0.0) {
									double quot = 0.0;
									const QuotIdent qi(ti.LocID, Ep.PriceQuotID, 0, pack.Rec.Object); // @v9.4.12 0-->pack.Rec.Object
									if(GObj.GetQuotExt(ti.GoodsID, qi, &quot, 0) > 0 && quot > 0.0)
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
											if(pos_local < pack.GetTCount()) {
												const PPTransferItem & r_ti_local = pack.TI(pos_local);
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
								if(ord_dis != 0.0 && ord_price != 0.0) {
									ord_part_dis = R6(ord_dis / ord_price); // @v9.5.0 R4-->R6
								}
							}
						}
						iSalesBillItem * p_new_item = p_new_pack->Items.CreateNewItem();
						THROW_SL(p_new_item);
						const double qtty = fabs(ti.Quantity_);
						p_new_item->LineN = tiiterpos; // ti.RByBill;
						p_new_item->OuterGoodsCode = temp_buf; // ti_pos_item.Txt;
						(p_new_item->NativeGoodsCode = 0).Cat(ti.GoodsID);
						{
							p_new_item->Country = 0;
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
							if(p_new_item->Country.Empty())
								(p_new_item->Country = "RU").Transf(CTRANSF_INNER_TO_UTF8);
						}
						p_new_item->CLB = 0;
						p_new_item->UnitCode = 1004;
						p_new_item->Qtty = qtty;
						p_new_item->Memo = 0;
						{
							double vat_sum_in_nominal_price = 0.0;
							double vat_sum_in_full_price = 0.0;
							double vat_sum_in_discount = 0.0;
							// @v9.5.5 const double full_price = (outerDocType == 6) ? ti.Cost : ((ord_part_dis >= 1.0) ? 0.0 : net_price);
							// @v9.5.5 const double nominal_price = (outerDocType == 6) ? ti.Cost : ((ord_part_dis >= 1.0) ? net_price : (net_price / (1.0 - ord_part_dis)));
							// @v9.5.5 {
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
							// } @v9.5.5
							const double discount = nominal_price - full_price;
							// @v9.5.9 (Установлена точность округления 6) {
							// @v9.5.11 (Установлена точность округления 12) {
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, 1.0, full_price,    &vat_sum_in_full_price, 0, 0, 16); // @v9.7.6 12-->16
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, 1.0, nominal_price, &vat_sum_in_nominal_price, 0, 0, 16); // @v9.7.6 12-->16
							GObj.CalcCostVat(0, goods_rec.TaxGrpID, pack.Rec.Dt, 1.0, discount,      &vat_sum_in_discount, 0, 0, 16); // @v9.7.6 12-->16
							// } @v9.5.9
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
								// @v9.3.2 p_amt_entry->DiscNetSum = (discount - vat_sum_in_discount);
								p_amt_entry->DiscNetSum = (p_amt_entry->NetSum - p_amt_entry->DiscAmount); // @v9.3.2
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

int SLAPI iSalesPepsi::Helper_MakeBillList(PPID opID, int outerDocType, TSCollection <iSalesBillPacket> & rList)
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
		b_filt.LocList = P.LocList; // @v9.7.5
		if(outerDocType == 6) {
			b_filt.ObjectID = P.SupplID;
		}
		b_filt.Period = P.ExpPeriod; //BillExportPeriod;
		SETIFZ(b_filt.Period.low, encodedate(1, 1, 2016));
		{
			SString org_isales_code;
			PPIDArray acn_list;
			acn_list.add(PPACN_UPDBILL);
			LDATETIME since;
			since.Set(b_filt.Period.low, ZEROTIME);
			PPIDArray upd_bill_list;
			// @v9.7.5 p_sj->GetObjListByEventSince(PPOBJ_BILL, &acn_list, since, upd_bill_list);
			p_sj->GetObjListByEventPeriod(PPOBJ_BILL, 0, &acn_list, &b_filt.Period, upd_bill_list); // @v9.7.5
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
								Helper_MakeBillEntry(upd_bill_id, -outerDocType, rList);
								force_bill_list.add(upd_bill_id);
							}
							else {
								int    is_isales_goods = 0;
								long   tiiterpos = 0;
								for(uint tiidx = 0; !is_isales_goods && tiidx < pack.GetTCount(); tiidx++) {
									const PPID goods_id = labs(pack.ConstTI(tiidx).GoodsID);
									if(GObj.BelongToGroup(goods_id, Ep.GoodsGrpID) > 0 && GObj.P_Tbl->GetArCode(P.SupplID, goods_id, temp_buf.Z(), 0) > 0) {
										int   skip_goods = 0;
										if(State & stGoodsMappingInited) {
											const iSalesGoodsPacket * p_entry = SearchGoodsMappingEntry(temp_buf);
											if(p_entry && p_entry->NativeCode.ToLong() == 0)
												skip_goods = 1;
										}
										if(!skip_goods)
											is_isales_goods = 1;
									}
								}
								if(is_isales_goods) {
									if(isales_code != org_isales_code) {
										Helper_MakeBillEntry(upd_bill_id, -outerDocType, rList);
									}
									force_bill_list.add(upd_bill_id);
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
				if(outerDocType == 6 && !P_BObj->CheckStatusFlag(view_item.StatusID, BILSTF_READYFOREDIACK)) {
					// Статус не позволяет отправку
					dont_send = 1;
				}
				else if(p_ref->Ot.GetTagStr(PPOBJ_BILL, view_item.ID, bill_ack_tag_id, temp_buf) > 0 && !test_uuid.FromStr(temp_buf)) {
					// @v9.6.4 (не отправляем документы, которые уже были отправлены ранее)
					dont_send = 1;
				}
				if(!dont_send) {
					Helper_MakeBillEntry(view_item.ID, outerDocType, rList);
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
						Helper_MakeBillEntry(bill_rec.ID, outerDocType, rList);
					}
				}
			}
		}
	}
    CATCHZOK
	return ok;
}

int SLAPI iSalesPepsi::SendInvoices()
{
	/*
		Возврат обратной реализацией (4) – это документ, без привязки к расходной накладной, и тут также не нужно передавать теги REFS (<DOC_TP> = 4)
		Возврат по акту (5) – это документ, c привязкой к расходной накладной, в тегах REFS в этом случае должны быть данные с номером расходной накладной, датой накладной, и тип документа расх. накладная = 1 (<DOC_TP> = 5)
	*/
	int    ok = -1;
	Reference * p_ref = PPRef;
	const  PPID bill_ack_tag_id = NZOR(Ep.Fb.BillAckTagID, PPTAG_BILL_EDIACK);
	PPSoapClientSession sess;
	SString temp_buf;
	SString msg_buf;
	TSCollection <iSalesBillPacket> outp_packet;
	//BillExportPeriod.Set(encodedate(1, 6, 2016), encodedate(30, 6, 2016));
	THROW(State & stInited);
	THROW(State & stEpDefined);
	THROW(P_Lib);
	THROW(Helper_MakeBillList(Ep.ExpendOp, 1, outp_packet));
	THROW(Helper_MakeBillList(Ep.RetOp, 5, outp_packet));
    {
		PPAlbatrosConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.EdiDesadvOpID && GetOpData(acfg.Hdr.EdiDesadvOpID, &op_rec) > 0) {
			if(op_rec.OpTypeID == PPOPT_DRAFTRECEIPT) {
				PPObjOprKind op_obj;
				PPDraftOpEx doe;
				if(op_obj.GetDraftExData(op_rec.ID, &doe) > 0 && doe.WrOffOpID) {
					THROW(Helper_MakeBillList(doe.WrOffOpID, 6, outp_packet));
				}
			}
            else if(op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
				THROW(Helper_MakeBillList(acfg.Hdr.EdiDesadvOpID, 6, outp_packet));
            }
		}
    }
	if(LogFileName.NotEmpty()) {
		SString dump_file_name;
		SPathStruc ps;
		ps.Split(LogFileName);
		ps.Nam.CatChar('-').Cat("dump").CatChar('-').Cat("invoices");
		ps.Merge(dump_file_name);
		SFile f_out_log(dump_file_name, SFile::mWrite);
		if(f_out_log.IsValid()) {
			f_out_log.WriteLine(0);
			for(uint i = 0; i < outp_packet.getCount(); i++) {
				const iSalesBillPacket * p_pack = outp_packet.at(i);
				if(p_pack) {
					msg_buf.Z().
						CatEq("NativeID", p_pack->NativeID).CatDiv(';', 0).
						CatEq("iSalesId", p_pack->iSalesId).CatDiv(';', 0).
						CatEq("DocType", (long)p_pack->DocType).CatDiv(';', 0).
						CatEq("ExtDocType", (long)p_pack->ExtDocType).CatDiv(';', 0).
						CatEq("Status", (long)p_pack->Status).CatDiv(';', 0).
						CatEq("Code", p_pack->Code).CatDiv(';', 0).
						CatEq("ExtCode", p_pack->ExtCode).CatDiv(';', 0).
						CatEq("Dtm", p_pack->Dtm.d, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("IncDtm", p_pack->IncDtm.d, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("ExtDtm", p_pack->ExtDtm.d, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("CreationDtm", p_pack->CreationDtm.d, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("LastUpdDtm", p_pack->LastUpdDtm.d, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("DueDate", p_pack->DueDate, DATF_DMY|DATF_CENTURY).CatDiv(';', 0).
						CatEq("ShipFrom", p_pack->ShipFrom).CatDiv(';', 0).
						CatEq("ShipTo", p_pack->ShipTo).CatDiv(';', 0).
						CatEq("SellerCode", p_pack->SellerCode).CatDiv(';', 0).
						CatEq("PayerCode", p_pack->PayerCode).CatDiv(';', 0).
						CatEq("Memo", p_pack->Memo).CatDiv(';', 0).
						CatEq("SrcLocCode", p_pack->SrcLocCode).CatDiv(';', 0).
						CatEq("DestLocCode", p_pack->DestLocCode).CatDiv(';', 0).
						CatEq("AgentCode", p_pack->AgentCode).CatDiv(';', 0).
						CatEq("AuthId", p_pack->AuthId).CatDiv(';', 0).
						CatEq("EditId", p_pack->EditId).CatDiv(';', 0).
						CatEq("ErrMsg", p_pack->ErrMsg).CatDiv(';', 0);
					//PPLogMessage(LogFileName, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
					f_out_log.WriteLine(msg_buf.CR());
				}
			}
		}
	}
	if(P.Flags & P.fTestMode) {

	}
	else if(outp_packet.getCount()) {
		SString tech_buf;
		Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, tech_buf);
		{
			//PPTXT_LOG_SUPPLIX_EXPBILL_S   "Экспорт документов поставщику @zstr '@article'"
			PPFormatT(PPTXT_LOG_SUPPLIX_EXPBILL_S, &msg_buf, tech_buf.cptr(), P.SupplID);
			//R_Logger.Log(msg_buf);
			PPWaitMsg(msg_buf);
		}
		const  uint max_fraction_size = 20;
		long   total_error_count = 0;
		ISALESPUTBILLS_PROC func = 0;
		THROW_SL(func = (ISALESPUTBILLS_PROC)P_Lib->GetProcAddr("iSalesPutBills"));
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
										if(bill_ack_tag_id) { // @v9.5.7
											if(p_item->Status == 1) { // @v9.7.6
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
			DestroyResult((void **)&p_result);
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
	SLAPI  SapEfes(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger);
	SLAPI ~SapEfes();

	void   SLAPI Init();
	void   SLAPI GetLogFileName(SString & rFileName) const;

	int    SLAPI ReceiveOrders();

	int    SLAPI SendStocks();
	int    SLAPI SendInvoices();
	int    SLAPI SendDebts();
	int    SLAPI SendSales_ByGoods();
	int    SLAPI SendSales_ByDlvrLoc();

private:
	int    SLAPI PreprocessResult(const void * pResult, const PPSoapClientSession & rSess);
	void   FASTCALL DestroyResult(void ** ppResult);
	void   SLAPI InitCallHeader(SapEfesCallHeader & rHdr);
	int    SLAPI Helper_MakeBillList(PPID opID, TSCollection <SapEfesBillPacket> & rList, PPIDArray & rToCancelBillList);
	int    SLAPI LogResultMsgList(const TSCollection <SapEfesLogMsg> * pMsgList);
	int    SLAPI PrepareDebtsData(TSCollection <SapEfesDebtReportEntry> & rList, TSCollection <SapEfesDebtDetailReportEntry> & rDetailList);
	int    SLAPI Helper_SendDebts(TSCollection <SapEfesDebtReportEntry> & rList);
	int    SLAPI Helper_SendDebtsDetail(TSCollection <SapEfesDebtDetailReportEntry> & rList);
	int    SLAPI MakeOrderReply(TSCollection <SapEfesBillStatus> & rList, const SapEfesOrder * pSrcPack, PPID resultOrderID, const char * pRetCode);

	enum {
		stInited             = 0x0001,
		stEpDefined          = 0x0002
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
	SString LogFileName;

	//PPObjBill * P_BObj;
	//PPObjLocation LocObj;
	//PPObjGoods GObj;

	PPLogger & R_Logger;
};

SLAPI SapEfes::SapEfes(PrcssrSupplInterchange::ExecuteBlock & rEb, PPLogger & rLogger) : PrcssrSupplInterchange::ExecuteBlock(rEb), R_Logger(rLogger)
{
	//P_BObj = BillObj;
	State = 0;
	//ArID = 0;
	P_DestroyFunc = 0;
	//BillExportPeriod.SetZero();
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

SLAPI SapEfes::~SapEfes()
{
	P_DestroyFunc = 0;
	delete P_Lib;
}

void SLAPI SapEfes::Init()
{
	Reference * p_ref = PPRef;
	State = 0;
	SvcUrl = 0;
	UserName = 0;
	Password = 0;
	Wareh = 0;
	{
        Ep.GetExtStrData(Ep.extssRemoveAddr, SvcUrl);
        Ep.GetExtStrData(Ep.extssAccsName, UserName);
        Ep.GetExtStrData(Ep.extssAccsPassw, Password);
		Ep.GetExtStrData(Ep.extssClientCode, SalesOrg);
		State |= stEpDefined;
	}
	// @v9.5.5 {
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
	// } @v9.5.5
	if(Wareh.Empty())
		Wareh = "DDJ0";
	InitGoodsList(iglfWithArCodesOnly); // @v9.5.1
	State |= stInited;
}

void SLAPI SapEfes::GetLogFileName(SString & rFileName) const
{
	rFileName = LogFileName;
}

int SLAPI SapEfes::PreprocessResult(const void * pResult, const PPSoapClientSession & rSess)
{
	LastMsg = rSess.GetMsg();
    return BIN(pResult);
}

void FASTCALL SapEfes::DestroyResult(void ** ppResult)
{
	if(P_DestroyFunc) {
		((UHTT_DESTROYRESULT)P_DestroyFunc)(*ppResult);
		*ppResult = 0;
	}
}

void SLAPI SapEfes::InitCallHeader(SapEfesCallHeader & rHdr)
{
	GetSequence(&rHdr.SessionID, 1);
	rHdr.P_SalesOrg = SalesOrg;
	rHdr.P_Wareh = Wareh;
}

int SLAPI SapEfes::MakeOrderReply(TSCollection <SapEfesBillStatus> & rList, const SapEfesOrder * pSrcPack, PPID resultOrderID, const char * pRetCode)
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


int SLAPI SapEfes::ReceiveOrders()
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
	if(!checkdate(period.low, 0))
		period.low = encodedate(1, 12, 2016);
	if(!checkdate(period.upp, 0))
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
	THROW_SL(func = (EFESGETSALESORDERSYNCLIST_PROC)P_Lib->GetProcAddr("EfesGetSalesOrderSyncList"));
	THROW_SL(func_status = (EFESSETSALESORDERSTATUSSYNC_PROC)P_Lib->GetProcAddr("EfesSetSalesOrderStatusSync"))
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
		PPAlbatrosConfig acfg;
		PPAlbatrosCfgMngr::Get(&acfg);
		PPOprKind op_rec;
		if(acfg.Hdr.OpID && GetOpData(acfg.Hdr.OpID, &op_rec) > 0 && oneof2(op_rec.OpTypeID, PPOPT_GOODSORDER, PPOPT_DRAFTEXPEND)) {
			PPID   loc_id = 0;
			PPIDArray dlvr_loc_list;
			PPIDArray person_list;
			for(uint i = 0; i < p_result->getCount(); i++) {
				const SapEfesOrder * p_src_pack = p_result->at(i);
				if(!p_src_pack || !checkdate(p_src_pack->Date.d, 0)) {
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
					if(checkdate(p_src_pack->DueDate, 0)) {
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
						sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop; // @v9.5.10
						if(!pack.SetupObject(contractor_by_loc_ar_id, sob)) {
							R_Logger.LogLastError();
							skip = 1;
						}
					}
					else if(contractor_ar_id) {
						sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop; // @v9.5.10
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
						else if(pack.Rec.Object) {
							PPFreight freight;
							freight.DlvrAddrID = dlvr_loc_id;
							pack.SetFreight(&freight);
						}
						// @v9.5.11 {
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
						// } @v9.5.11
						STRNSCPY(pack.Rec.Memo, p_src_pack->Memo);
						if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, &ex_bill_id, &ex_bill_rec) > 0) {
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
											if(p_src_item->BaseUnitType == sapefesUnitItem && p_src_item->QtyN > 0.0) {
												_qtty_mult = (((double)p_src_item->QtyD) / ((double)p_src_item->QtyN));
											}
											else if(GObj.GetStockExt(goods_rec.ID, &gse, 0) > 0 && gse.Package > 0.0) {
												_qtty_mult = gse.Package;
											}
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
								{
									pack.Rec.EdiOp = PPEDIOP_SALESORDER;
									pack.BTagL.PutItemStr(PPTAG_BILL_EDICHANNEL, "SAP-EFES");
									pack.BTagL.PutItemStrNE(PPTAG_BILL_EDIIDENT, p_src_pack->Code);
								}
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
	DestroyResult((void **)&p_result);
    return ok;
}

int SLAPI SapEfes::SendSales_ByDlvrLoc()
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
	THROW_SL(func = (EFESSETMTDOUTLETSREPORTSYNC_PROC)P_Lib->GetProcAddr("EfesSetMTDOutletsReportSync"));
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
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI SapEfes::SendSales_ByGoods()
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
	THROW_SL(func = (EFESSETMTDPRODUCTREPORTSYNC_PROC)P_Lib->GetProcAddr("EfesSetMTDProductReportSync"));
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
	DestroyResult((void **)&p_result);
	CATCHZOK
	return ok;
}

int SLAPI SapEfes::SendStocks()
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
	THROW_SL(func = (EFESSETDAILYSTOCKREPORTSYNC_PROC)P_Lib->GetProcAddr("EfesSetDailyStockReportSync"));
	sess.Setup(SvcUrl, UserName, Password);
	InitCallHeader(sech);
	{
		PPViewGoodsRest gr_view;
		GoodsRestFilt gr_filt;
		GoodsRestViewItem gr_item;
		SString ar_code;
		//LocationTbl::Rec loc_rec;

		gr_filt.Date = NZOR(P.ExpPeriod.upp, _curdate);
		gr_filt.GoodsGrpID = Ep.GoodsGrpID;
		gr_filt.LocList = P.LocList; // @v9.3.0
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
	DestroyResult((void **)&p_result);
    CATCHZOK
    return ok;
}

int SLAPI SapEfes::Helper_MakeBillList(PPID opID, TSCollection <SapEfesBillPacket> & rList, PPIDArray & rToCancelBillList)
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
			p_sj->GetObjListByEventSince(PPOBJ_BILL, &acn_list, since, upd_bill_list);
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
						cli_code = 0;
						loc_code = 0;
                        p_ref->Ot.GetTagStr(PPOBJ_PERSON, cli_psn_id, Ep.Fb.CliCodeTagID, cli_code);
                        if(pack.P_Freight && pack.P_Freight->DlvrAddrID)
							p_ref->Ot.GetTagStr(PPOBJ_LOCATION, pack.P_Freight->DlvrAddrID, Ep.Fb.LocCodeTagID, loc_code);
						if(cli_code.Empty()) {
							if(loc_code.NotEmpty())
								cli_code = loc_code;
							else {
								//PPTXT_LOG_SUPPLIX_NOCLLCCODES
                                PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddObjName, temp_buf);
								R_Logger.Log(PPFormatT(PPTXT_LOG_SUPPLIX_NOCLLCCODES, &msg_buf, temp_buf.cptr()));
								skip = 1;
							}
						}
						else if(loc_code.Empty())
							loc_code = cli_code;
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
									if(p_ref->Ot.GetTagStr(PPOBJ_BILL, ord_id, PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.CmpNC("SAP-EFES") == 0) {
										THROW(P_BObj->ExtractPacket(ord_id, &order_pack) > 0);
										is_own_order = 1;
									}
								}
							}
							p_new_item->Date = pack.Rec.Dt;
							p_new_item->DocType = SapEfesBillPacket::tRetail;
							BillCore::GetCode(p_new_item->NativeCode = pack.Rec.Code);
							p_new_item->DueDate = (is_own_order && ord_rec.DueDate) ? ord_rec.DueDate : pack.Rec.Dt;
							p_new_item->BuyerCode = cli_code;
							p_new_item->DlvrLocCode = loc_code;
							if(is_own_order) {
								BillCore::GetCode(p_new_item->OrderCode = ord_rec.Code);
								p_new_item->Flags |= p_new_item->fHasOrderRef;
							}
							else
								p_new_item->Flags &= ~p_new_item->fHasOrderRef;
							p_new_item->Memo = pack.Rec.Memo;
							{
								long   tiiterpos = 0;
								for(TiIter tiiter(&pack, ETIEF_UNITEBYGOODS|ETIEF_FORCEUNITEGOODS, 0); pack.EnumTItemsExt(&tiiter, &ti, &tiext) > 0;) {
									tiiterpos++;
									uint   pos_list_item_pos = 0;
									if(ti_pos_list.Get(tiiterpos, temp_buf) > 0 && GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
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

int SLAPI SapEfes::SendInvoices()
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

		THROW_SL(func = (EFESSETDELIVERYNOTESYNC_PROC)P_Lib->GetProcAddr("EfesSetDeliveryNoteSync"));
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
		DestroyResult((void **)&p_result);
		p_result = 0;
	}
	if(to_cancel_bill_list.getCount()) {
		TSCollection <SString> to_cancel_code_list;
		EFESCANCELDELIVERYNOTESYNC_PROC func = 0;
		THROW_SL(func = (EFESCANCELDELIVERYNOTESYNC_PROC)P_Lib->GetProcAddr("EfesCancelDeliveryNoteSync"));
		{
			for(uint i = 0; i < to_cancel_bill_list.getCount(); i++) {
				const PPID to_cancel_bill_id = to_cancel_bill_list.get(i);
				if(P_BObj->Search(to_cancel_bill_id, &bill_rec) > 0) {
					SString * p_new_code = to_cancel_code_list.CreateNewItem();
					THROW_SL(p_new_code);
					*p_new_code = bill_rec.Code;
					BillCore::GetCode(*p_new_code);
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
			DestroyResult((void **)&p_result);
		}
	}
    CATCHZOK
    return ok;
}

int SLAPI SapEfes::LogResultMsgList(const TSCollection <SapEfesLogMsg> * pMsgList)
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

int SLAPI SapEfes::PrepareDebtsData(TSCollection <SapEfesDebtReportEntry> & rList, TSCollection <SapEfesDebtDetailReportEntry> & rDetailList)
{
    int    ok = 1;
	//SString temp_buf;
	//TSCollection <SapEfesDebtReportEntry> outp_list;
	//TSCollection <SapEfesDebtDetailReportEntry> outp_detail_list;
	//TSCollection <SapEfesLogMsg> * p_result = 0;
	//PPSoapClientSession sess;
	//SapEfesCallHeader sech;
	//EFESSETDEBTSYNC_PROC func = 0;
	//THROW(State & stInited);
	//THROW(State & stEpDefined);
	//THROW(P_Lib);
	//THROW_SL(func = (EFESSETDEBTSYNC_PROC)P_Lib->GetProcAddr("EfesSetDebtSync"));
	//sess.Setup(SvcUrl, UserName, Password);
	//InitCallHeader(sech);
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
									if(ArObj.GetClientAgreement(ar_id, &cli_agt, 1) > 0) {
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

int SLAPI SapEfes::Helper_SendDebts(TSCollection <SapEfesDebtReportEntry> & rList)
{
    int    ok = -1;
	if(rList.getCount()) {
		SString temp_buf;
		//TSCollection <SapEfesDebtReportEntry> outp_list;
		//TSCollection <SapEfesDebtDetailReportEntry> outp_detail_list;
		TSCollection <SapEfesLogMsg> * p_result = 0;
		PPSoapClientSession sess;
		SapEfesCallHeader sech;
		EFESSETDEBTSYNC_PROC func = 0;
		THROW(State & stInited);
		THROW(State & stEpDefined);
		THROW(P_Lib);
		THROW_SL(func = (EFESSETDEBTSYNC_PROC)P_Lib->GetProcAddr("EfesSetDebtSync"));
		sess.Setup(SvcUrl, UserName, Password);
		InitCallHeader(sech);
		{
			p_result = func(sess, sech, &rList);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			LogResultMsgList(p_result);
			DestroyResult((void **)&p_result);
		}
	}
    CATCHZOK
    return ok;
}

int SLAPI SapEfes::Helper_SendDebtsDetail(TSCollection <SapEfesDebtDetailReportEntry> & rList)
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
		THROW_SL(func = (EFESSETDEBTDETAILSYNC_PROC)P_Lib->GetProcAddr("EfesSetDebtDetailSync"));
		sess.Setup(SvcUrl, UserName, Password);
		InitCallHeader(sech);
		{
			p_result = func(sess, sech, &rList);
			THROW_PP_S(PreprocessResult(p_result, sess), PPERR_UHTTSVCFAULT, LastMsg);
			LogResultMsgList(p_result);
			DestroyResult((void **)&p_result);
		}
	}
    CATCHZOK
    return ok;
}

int SLAPI SapEfes::SendDebts()
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
IMPLEMENT_PPFILT_FACTORY(SupplInterchange); SLAPI SupplInterchangeFilt::SupplInterchangeFilt() : PPBaseFilt(PPFILT_SUPPLINTERCHANGE, 0, 0)
{
	SetFlatChunk(offsetof(SupplInterchangeFilt, ReserveStart),
		offsetof(SupplInterchangeFilt, Reserve) - offsetof(SupplInterchangeFilt, ReserveStart) + sizeof(Reserve));
	SetBranchSString(offsetof(SupplInterchangeFilt, ExtString));
	SetBranchObjIdListFilt(offsetof(SupplInterchangeFilt, LocList));
	Init(1, 0);
}

int SLAPI SupplInterchangeFilt::ReadPreviosVer(SBuffer & rBuf, int ver)
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
	Copy(&rS, 0);
	return *this;
}

SupplInterchangeFilt & FASTCALL SupplInterchangeFilt::operator = (const SupplExpFilt & rS)
{
	Init(1, 0);
	SupplID = rS.SupplID;
	ExpPeriod = rS.Period;
	MaxTransmitSize = rS.MaxFileSizeKB;

	SpcDisPct1 = (float)rS.PctDis1;
	SpcDisPct2 = (float)rS.PctDis2;

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
SLAPI PrcssrSupplInterchange::ExecuteBlock::ExecuteBlock()
{
	P_BObj = BillObj;
	SeqID = 0;
	BaseState = 0;
}

SLAPI PrcssrSupplInterchange::ExecuteBlock::ExecuteBlock(const ExecuteBlock & rS)
{
	P_BObj = BillObj;
	Ep = rS.Ep;
	P = rS.P;
	SeqID = Ep.Fb.SequenceID;
	BaseState = rS.BaseState;
	GoodsList = rS.GoodsList;
}

#if 0 // @v9.6.2 {
int SLAPI PrcssrSupplInterchange::ExecuteBlock::Debug_TestUtfText(const SString & rText, const char * pAddendum, PPLogger & rLogger)
{
	int    ok = 1;
	SString msg_buf, fmt_buf;
	if(!rText.IsLegalUtf8()) {
		PPLoadText(PPTXT_TXTHASILLUTF8, fmt_buf);
		msg_buf.Printf(fmt_buf, rText.cptr());
		if(pAddendum)
			msg_buf.CatDiv(':', 2).Cat(pAddendum);
        //rLogger.Log(msg_buf);
        PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		ok = 0;
	}
	else if(rText.StrChr('?', 0)) {
		PPLoadText(PPTXT_TXTHASSUSPCHR, fmt_buf);
		msg_buf.Printf(fmt_buf, rText.cptr());
		if(pAddendum)
			msg_buf.CatDiv(':', 2).Cat(pAddendum);
        //rLogger.Log(msg_buf);
        PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_DBINFO|LOGMSGF_TIME|LOGMSGF_USER);
		ok = 0;
	}
	return ok;
}
#endif // } 0 @v9.6.2

int SLAPI PrcssrSupplInterchange::ExecuteBlock::InitGoodsList(long flags)
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
{
	return (BaseState & bstGoodsListInited && !(BaseState & bstAnyGoods)) ? &GoodsList : 0;
}

int SLAPI PrcssrSupplInterchange::ExecuteBlock::GetSequence(long * pSeq, int use_ta)
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
SLAPI PrcssrSupplInterchange::PrcssrSupplInterchange()
{
	State = 0;
	P_Eb = 0;
}

SLAPI PrcssrSupplInterchange::~PrcssrSupplInterchange()
{
	delete P_Eb;
}

int SLAPI PrcssrSupplInterchange::InitParam(PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	SupplInterchangeFilt temp_filt;
	THROW(temp_filt.IsA(pBaseFilt));
	{
		// SupplInterchangeFilt * p_filt = (SupplInterchangeFilt *)pBaseFilt;
	}
	CATCHZOK
	return ok;
}

int SLAPI PrcssrSupplInterchange::EditParam(PPBaseFilt * pBaseFilt)
{
	SupplInterchangeFilt temp_filt;
	if(!temp_filt.IsA(pBaseFilt))
		return 0;
	SupplInterchangeFilt * p_filt = (SupplInterchangeFilt *)pBaseFilt;
	DIALOG_PROC_BODY(SupplInterchangeFiltDialog, p_filt);
}

int SLAPI PrcssrSupplInterchange::InitExecuteBlock(const SupplInterchangeFilt * pParam, PrcssrSupplInterchange::ExecuteBlock & rBlk)
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

int SLAPI PrcssrSupplInterchange::Init(const PPBaseFilt * pBaseFilt)
{
	int    ok = 1;
	ArticleTbl::Rec ar_rec;
	PPSupplAgreement suppl_agt;
	SupplInterchangeFilt temp_filt;
	State &= stInited;
	ZDELETE(P_Eb);
	THROW(temp_filt.IsA(pBaseFilt));
	temp_filt = *(SupplInterchangeFilt *)pBaseFilt;
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
		P_Eb->ArName = ar_rec.Name; // @v9.4.4
	}
	State |= stInited;
	CATCHZOK
	return ok;
}

int SLAPI SupplGoodsImport()
{
	int    ok = -1;
	SupplExpFilt filt;
	if(EditSupplExpFilt(&filt, 1) > 0) {
		SString path;
		if(PPOpenFile(PPTXT_FILPAT_GOODS_XML, path, 0, APPL->H_MainWnd) > 0) {
			//
			SupplInterchangeFilt six_filt;
			six_filt = filt;
			PrcssrSupplInterchange six_prc;
			PrcssrSupplInterchange::ExecuteBlock eb;
			THROW(six_prc.InitExecuteBlock(&six_filt, eb));
			{
				PPSupplExchange_Baltika s_e(eb);
				THROW(s_e.Init(/*&filt*/));
				THROW(s_e.Import(path));
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	return ok;
}

int SLAPI DoSupplInterchange(SupplInterchangeFilt * pFilt)
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

int SLAPI PrcssrSupplInterchange::Run()
{
	int    ok = -1;
	SString temp_buf;
	SString log_file_name;
	PPLogger logger;
	THROW_PP(State & stInited && P_Eb, PPERR_SUPPLIXNOTINITED);
	P_Eb->Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssTechSymbol, temp_buf);
	if(temp_buf.CmpNC("MONOLIT-BALTIKA") == 0) {
		int    max_size_kb = 0;
		PPIniFile ini_file;
		if(ini_file.IsValid()) {
			ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SUPPLEXP_BILLFILEMAXSIZE, &max_size_kb);
			ini_file.Get(PPINISECT_CONFIG, PPINIPARAM_SPECENCODESYMBS, temp_buf.Z());
			P_Eb->P.PutExtStrData(SupplInterchangeFilt::extssEncodeStr, temp_buf);
		}
		P_Eb->P.MaxTransmitSize = (size_t)max_size_kb;
		{
			PPSupplExchange_Baltika s_e(*P_Eb);
			PPWait(1);
			THROW(s_e.Init(/*&filt*/));
			if(P_Eb->P.Actions & SupplInterchangeFilt::opImportGoods) {
				PPGetFilePath(PPPATH_OUT, "monolit-baltica.xml", temp_buf.Z());
				//
				THROW(s_e.Import(temp_buf));
				ok = 1;
			}
			if(P_Eb->P.Actions & SupplInterchangeFilt::opExportBills|SupplInterchangeFilt::opExportClients|
				SupplInterchangeFilt::opExportDebts|SupplInterchangeFilt::opExportGoodsDebts|
				SupplInterchangeFilt::opExportPrices|SupplInterchangeFilt::opExportStocks) {
				THROW(s_e.Export(logger));
				ok = 1;
			}
		}
	}
	else if(temp_buf.CmpNC("ISALES-PEPSI") == 0) {
		const int rcv_goods_force_settings = BIN(P_Eb->P.Actions & SupplInterchangeFilt::opImportGoods);
		iSalesPepsi cli(*P_Eb, logger);
		TSCollection <iSalesRoutePacket> routs;
		PPWait(1);
		THROW(cli.Init(/*P_Eb->P.SupplID*/)); // ООО "ПепсиКо Холдингс"
		if(P_Eb->P.Actions & (SupplInterchangeFilt::opExportBills|SupplInterchangeFilt::opExportStocks|
			SupplInterchangeFilt::opExportPrices|SupplInterchangeFilt::opImportDesadv|SupplInterchangeFilt::opImportOrders))
			P_Eb->P.Actions |= SupplInterchangeFilt::opImportGoods;
		if(P_Eb->P.Actions & SupplInterchangeFilt::opImportGoods) {
			THROW(cli.ReceiveGoods(rcv_goods_force_settings, 1));
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opImportRouts) {
			if(!cli.ReceiveRouts(routs))
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opImportOrders) {
			if(!cli.ReceiveOrders())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opImportDesadv) {
			if(!cli.ReceiveReceipts())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportBills) {
			if(!cli.SendInvoices())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportStocks) {
			if(!cli.SendStocks())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportPrices) {
			if(!cli.SendPrices())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportDebts) {
			if(!cli.SendDebts())
				logger.LogLastError();
		}
		cli.GetLogFileName(log_file_name);
		PPWait(0);
	}
	else if(temp_buf.CmpNC("SAP-EFES") == 0) {
		SapEfes cli(*P_Eb, logger);
		PPWait(1);
		cli.Init();
		if(P_Eb->P.Actions & SupplInterchangeFilt::opImportOrders) {
			if(!cli.ReceiveOrders())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportBills) {
			if(!cli.SendInvoices())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportStocks) {
			if(!cli.SendStocks())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportDebts) {
			if(!cli.SendDebts())
				logger.LogLastError();
		}
		if(P_Eb->P.Actions & SupplInterchangeFilt::opExportSales) {
			if(!cli.SendSales_ByGoods())
				logger.LogLastError();
			if(!cli.SendSales_ByDlvrLoc())
				logger.LogLastError();
		}
		cli.GetLogFileName(log_file_name);
		PPWait(0);
	}
	else {
		; //
	}
	CATCH
		logger.LogLastError();
	ENDCATCH
	PPWait(0); // @v9.7.6
	if(log_file_name.NotEmpty())
		logger.Save(log_file_name, LOGMSGF_DIRECTOUTP|LOGMSGF_TIME|LOGMSGF_USER);
	return ok;
}
