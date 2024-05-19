// CRCSHSRV.CPP
// Copyright (c) V.Nasonov, A.Sobolev 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
// Интерфейс (асинхронный) к драйверу кассового сервера (ООО Кристалл Сервис)
//
#include <pp.h>
#pragma hdrstop
//
// CashiersArray
//
struct CashierEntry { // @flat
	CashierEntry() : TabNum(0), Expiry(ZERODATE), PsnID(0)
	{
	}
	PPID   TabNum;
	LDATE  Expiry;
	PPID   PsnID;
};

IMPL_CMPFUNC(CashierEnKey, i1, i2)
{
	int    cmp = 0;
	const CashierEntry * k1 = static_cast<const CashierEntry *>(i1);
	const CashierEntry * k2 = static_cast<const CashierEntry *>(i2);
	if(k1->TabNum < k2->TabNum)
		cmp = -1;
	else if(k1->TabNum > k2->TabNum)
		cmp = 1;
	else if(k1->Expiry == ZERODATE || k2->Expiry == ZERODATE)
		cmp = -diffdate(k1->Expiry, k2->Expiry);
	else
		cmp = diffdate(k1->Expiry, k2->Expiry);
	return cmp;
}

class CashiersArray : public SVector {
public:
	CashiersArray() : SVector(sizeof(CashierEntry))
	{
	}
	CashierEntry & FASTCALL at(uint p) const { return *static_cast<CashierEntry *>(SVector::at(p)); }
	int    FASTCALL Add(const CashierEntry * pEntry) { return Search(pEntry->TabNum, pEntry->Expiry, 0) ? -1 : Insert(pEntry); }
	PPID   GetCshrID(long tabNum, LDATE dt) const
	{
		uint  pos = 0;
		return SearchNearest(tabNum, dt, &pos) ? at(pos).PsnID : 0;
	}
private:
	int    Search(long tabNum, LDATE dt, uint * pPos = 0) const;
	bool   SearchNearest(long tabNum, LDATE dt, uint * pPos = 0) const;
	int    Insert(const CashierEntry * pEntry, uint * pPos = 0) { return ordInsert(pEntry, pPos, PTR_CMPFUNC(CashierEnKey)) ? 1 : PPSetErrorSLib(); }
};

int CashiersArray::Search(long tabNum, LDATE dt, uint * pPos) const
{
	CashierEntry ce;
	ce.TabNum = tabNum;
	ce.Expiry = dt;
	return bsearch(&ce, pPos, PTR_CMPFUNC(CashierEnKey));
}

bool CashiersArray::SearchNearest(long tabNum, LDATE dt, uint * pPos) const
{
	bool   is_found = false;
	long   tab_num  = tabNum;
	while(!is_found && lsearch(&tab_num, pPos, CMPF_LONG)) {
		const CashierEntry & ce = at(*pPos);
		if(ce.Expiry == ZERODATE || diffdate(ce.Expiry, dt) >= 0)
			is_found = true;
		else
			(*pPos)++;
	}
	return is_found;
}
//
//
//
#define CRCSHSRV_DISCCARD_DEFTYPE  256
#define CRCSHSRV_CSHRRIGHTS_STRLEN  25

class ACS_CRCSHSRV : public PPAsyncCashSession {
public:
	struct AcceptedCheck_ {
		PPID  CashNum;
		LDATE Dt;
		LTIME Tm;
	};

	explicit ACS_CRCSHSRV(PPID n) : PPAsyncCashSession(n), Options(0), CurOperDate(ZERODATE), P_SCardPaymTbl(0), StatID(0)
	{
		int    ipar = 0;
		PPIniFile ini_file;
		ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_ACSCLOSE_USEALTIMPORT, &ipar); // &UseAltImport);
		if(ipar > 0)
			Options |= oUseAltImport;
		ChkRepPeriod.Z();
		for(size_t i = 0; i < SIZEOFARRAY(P_IEParam); i++)
			P_IEParam[i] = 0;
		{
			PPAsyncCashNode acn;
			GetNodeData(&acn);
			ModuleVer = acn.DrvVerMajor;
			ModuleSubVer = acn.DrvVerMinor;
		}
	}
	~ACS_CRCSHSRV()
	{
		for(size_t i = 0; i < SIZEOFARRAY(P_IEParam); i++)
			delete P_IEParam[i];
		delete P_SCardPaymTbl;
	}
	virtual int  ExportData(int updOnly);
	virtual int  GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int  ImportSession(int);
	virtual int  FinishImportSession(PPIDArray *);
	virtual void CleanUpSession();
	int ExportDataV10(int updOnly);
	int ExportData__(int updOnly);
	int Prev_ExportData(int updOnly);
private:
	enum {
		filTypZRep = 0,
		filTypChkHeads,
		filTypChkRows,
		filTypChkDscnt,
		filTypZRepXml,
		filTypChkXml
	};
	virtual int IsReadyForExport();
	int    ImportZRepList(SVector * pZRepList, bool useLocalFiles);
	int    ConvertWareList(const SVector * pZRepList, const char *);
	int    ConvertWareListV10(const SVector * pZRepList, const char * pPath, const char *);
	int    ConvertCheckHeads(const SVector * pZRepList, const char *);
	int    ConvertCheckRows(const char *);
	int    GetSeparatedFileSet(int filTyp);
	int    CreateSCardPaymTbl();
	int    PrepareImpFileName(int filTyp, int subStrId, const char * pPath, int sigNum);
	//int    PrepareImpFileName(int filTyp, const char * pName, const char * pPath, int sigNum);
	int    PrepareImpFileNameV10(int filTyp, const char * pName, const char * pPath);
	SString & MakeQueryBuf(LDATE dt, SString & rBuf) const;
	SString & MakeQueryBufV10(LDATE dt, SString & rBuf, int isZRep) const;
	int    QueryFile(int filTyp, const char * pQueryBuf, LDATE queryDate);
	int    IsFileExists(uint fileId, const char * pSubDir); // @<<ACS_CRCSHSRV::IsReadyForExport
	int    IsFileExists(const char * pFile, const char * pSubDir); // @<<ACS_CRCSHSRV::IsReadyForExport
	int    GetCashiersList();
	int    SearchCardCode(SCardCore * pSc, const char * pCode, SCardTbl::Rec * pRec);
	bool   GetFilesLocal();
	PPBillImpExpParam * CreateImpExpParam(uint sdRecID);
	void   Backup(const char * pPrefix, const char * pPath);
	int    Helper_ExportGoods_V10(int mode, bool goodsIdAsArticle, const SString & rPathGoods, const PPAsyncCashNode & rCnData, const SString & rStoreIndex,
		AsyncCashGoodsIterator * pGoodsIter, const SVector & rSalesGrpList, AsyncCashGoodsInfo & rGoodsInfo, SString & rResultFileName);

	class DeferredRemovingFileList : public SStrGroup {
	public:
		DeferredRemovingFileList()
		{
		}
		DeferredRemovingFileList & Z()
		{
			ClearS();
			L.clear();
			return *this;
		}
		void Add(const char * pBackupPrefix, const char * pPath)
		{
			Entry new_entry;
			AddS(pBackupPrefix, &new_entry.BackupPrefixP);
			AddS(pPath, &new_entry.FilePathP);
			L.insert(&new_entry);
		}
		uint GetCount() const { return L.getCount(); }
		int  Get(uint idx /*0..*/, SString & rBackupPrefix, SString & rPath)
		{
			int    ok = 1;
			rBackupPrefix.Z();
			rPath.Z();
			if(idx < L.getCount()) {
				const Entry & r_entry = L.at(idx);
				GetS(r_entry.BackupPrefixP, rBackupPrefix);
				GetS(r_entry.FilePathP, rPath);
			}
			else
				ok = 0;
			return ok;
		}
	private:
		struct Entry {
			Entry() : FilePathP(0), BackupPrefixP(0)
			{
			}
			uint    FilePathP;
			uint    BackupPrefixP;
		};
		TSVector <Entry> L;
	};

	DeferredRemovingFileList DrfL;
	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	PPIDArray SessAry;
	CashiersArray CshrList;
	SString PathQue[6];
	SString PathRpt[6];
	SString PathFlag;
	SString PathCshrs;
	TempOrderTbl * P_SCardPaymTbl;
	PPID   StatID;
	PPBillImpExpParam * P_IEParam[4];

	enum {
		oUseAltImport    = 0x0002,
		oSeparateReports = 0x0004
	};
	long   Options;
	int    ModuleVer;    // Версия модуля Set-Retail
	int    ModuleSubVer; // Субверсия модуля Set-Retail
	LDATE  CurOperDate;
	SString   PathSetRExpCfg;
	StringSet SeparatedFileSet;
	TSArray <AcceptedCheck_> AcceptedCheckList;
};

class CM_CRCSHSRV : public PPCashMachine {
public:
	CM_CRCSHSRV(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * AsyncInterface() { return new ACS_CRCSHSRV(NodeID); }
};

REGISTER_CMT(CRCSHSRV, false, true);

int ACS_CRCSHSRV::IsFileExists(uint fileId, const char * pSubDir)
{
	SString path;
	PPGetFilePath(PPPATH_OUT, fileId, path);
	return BIN(DistributeFile_(path, 0/*pEndFileName*/, dfactCheckExistence, pSubDir, 0) > 0);
}

int ACS_CRCSHSRV::IsFileExists(const char * pFile, const char * pSubDir)
{
	SString path;
	PPGetFilePath(PPPATH_OUT, pFile, path);
	return BIN(DistributeFile_(path, 0/*pEndFileName*/, dfactCheckExistence, pSubDir, 0) > 0);
}

bool ACS_CRCSHSRV::GetFilesLocal()
{
	bool   result = false;
	const  char * p_ready_fname = "ascready.";
	SString ready_fpath;
	PPAsyncCashNode acn;
	if(GetNodeData(&acn) > 0) {
		(ready_fpath = acn.ImpFiles).SetLastSlash().Cat(p_ready_fname);
		result = fileExists(ready_fpath);
	}
	return result;
}

#define GOODS_XML    "catalog-goods.xml"
#define CASHIERS_XML "cashiers.xml"
#define CARDS_XML    "Catalog-cards.xml"
#define SUBDIR_CARDS "\\cards\\source"
#define SUBDIR_CASHIERS "\\cashiers\\source"
#define SUBDIR_PRODUCTS "\\products\\source"


int ACS_CRCSHSRV::IsReadyForExport()
{
	int    ready = 1;
	if(ModuleVer == 10) {
		if(IsFileExists(GOODS_XML, SUBDIR_PRODUCTS))
			ready = 0;
		else if(IsFileExists(CASHIERS_XML, SUBDIR_CASHIERS))
			ready = 0;
	}
	else {
		if(IsFileExists(PPFILNAM_CS_GOODS_DBF, 0))
			ready = 0;
		else if(IsFileExists(PPFILNAM_CS_BAR_DBF, 0))
			ready = 0;
	}
	return ready;
}
/*
	Позиции прав доступа кассиров в SetRetail
	1  Оформление возвратов
	2  Печать товарных чеков
	3  Инкассация из кассы
	4  Инкассация из администратора
	5  Внесение денег
	6  Суточный отчет (Z)
	7  Печать отчетов
	8  Дневная сводка
	9  Удаление покупки из чека ***
	10 Копия чека
	11 Нарастающие итоги
	12 Работа в меню администратор
	13 Безналичный расчет
	14 Контрольная лента
	15 Аннулирование чеков
	16 По секциям
	17 По кассе (отчет Х)
	18 По кассирам
	19 По видам товаров
	20 По времени
	21 Почасовой
	22 Печать чеков
	23 Акт списания сумм
	24 Копия Z-отчета
	25 Безналичные расчеты
	26
	27 Режим кассира
	28 Печать контрольной ленты из ЭКЛЗ
	29 Дисконтная карта
*/
static void ConvertCashierRightsToCrystalRightsSet(const char * pRights, char * pCrystalRights, size_t szCrRts)
{
	static const char correspondance[32] = {
		12, 27,  1,  3,  4, 5, 15,  9,
		10, 13, 29,  7,  0, 0,  0,  0,
		22,  2, 17,  6, 24, 8, 16, 18,
		19, 20, 21, 23, 25, 11, 14, 0
	};
	const size_t count = MIN(SIZEOFARRAY(correspondance), szCrRts);
	memzero(pCrystalRights, szCrRts);
	for(size_t i = 0; i < count; i++) {
		const int idx = correspondance[i];
		if(idx)
			pCrystalRights[idx-1] = pRights[i];
	}
}

static int PrepareDscntCodeBiasList(LAssocArray * pAry)
{
	int    ok = 1;
	uint   i, k;
	LAssocArray  bias_ary;
	StrAssocArray qk_list;
	QuotKindFilt  qk_flt;
	PPObjQuotKind qk_obj;
	qk_flt.Flags = QuotKindFilt::fAddBase;
	THROW(qk_obj.MakeList(&qk_flt, &qk_list));
	k = MIN(qk_list.getCount(), 127); // Вынуждены ограничивать кол-во видов котировок (BIAS <128)
	for(i = 0; i < k; i++) {
		uint  pos = 0;
		long  key;
		PPID  qk_id = qk_list.Get(i).Id;
		PPID  dc_bias = qk_id % 127;
		while(bias_ary.SearchByVal(dc_bias, &key, &pos)) {
			dc_bias = (dc_bias + 1) % 127; // @v5.8.2 (qk_id+1)-->(dc_bias+1)
			//dc_bias = (qk_id+1) % 127; // @test
		}
		THROW_SL(bias_ary.Add(qk_id, dc_bias, 0));
	}
	CATCHZOK
	ASSIGN_PTR(pAry, bias_ary);
	return ok;
}
//
// В старом варианте код скидки формировался следующим образом:
//    dscnt_code_bias * 0x01000000 + ИД товара, где dscnt_code_bias генерируется как ИД вида котировки % 127
//    (см. функцию PrepareDscntCodeBiasList выше)
// Новый алгоритм формирования кодов скидок учитывает то, что скидки могут быть как по котировкам, так и по картам:
//    1. dscnt_code_bias = ИД объекта - 900, где ИД объекта - ИД серии карт или ИД вида котировки
//       Для ИД объекта < 1000 (например базовая котировка) dscnt_code_bias = ИД объекта
//    2. Код скидки представляет собой 32-битовое число следующего вида:
//       бит  31 = 0
//       бит  30 = 0 - для кодов по картам, 1 - для кодов по видам котировок
//       биты 19-29 - dscnt_code_bias (< 2048)
//       биты  0-18 - ИД товара (< 524288)
//
static long FASTCALL GetDscntCode(PPID goodsID, PPID objID, int isQuotKind)
{
	long   obj_bias = (objID > 900) ? objID - 900 : objID;
	long   dscnt_code = (obj_bias << 19) + goodsID + (isQuotKind ? 0x40000000 : 0);
	return dscnt_code;
}

static void FASTCALL AddTimeToFileName(SString & fName)
{
	const LTIME cur_time = getcurtime_();
	SFsPath ps(fName);
	// @v9.7.0 ps.Nam.CatLongZ(cur_time.hour(), 2).CatLongZ(cur_time.minut(), 2).CatLongZ(cur_time.sec(), 2);
	ps.Nam.Cat(cur_time, TIMF_HMS|TIMF_NODIV); // @v9.7.0
	ps.Merge(fName);
}

class XmlWriter {
public:
	XmlWriter(const char * pPath, int replaceSpecSymb);
	~XmlWriter();
	int StartElement(const char * pName, const char * pAttribName = 0, const char * pAttribValue = 0);
	void   EndElement();
	int AddAttrib(const char * pAttribName, const char * pAttribValue);
	int AddAttrib(const char * pAttribName, bool attribValue);
	int AddAttrib(const char * pAttribName, long attribValue);
	int AddAttrib(const char * pAttribName, int attribValue) { return AddAttrib(pAttribName, static_cast<long>(attribValue)); }
	int AddAttrib(const char * pAttribName, double attribValue);
	int AddAttrib(const char * pAttribName, LDATETIME);
	int AddAttrib(const char * pAttribName, LDATE);
	int PutElement(const char * pName, const char * pValue);
	int PutElement(const char * pName, long);
	int PutElement(const char * pName, double);
	int PutElement(const char * pName, bool);
	int PutElement(const char * pName, LDATETIME);
	int PutElement(const char * pName, LDATE);
	int PutElement(const char * pName, LTIME);
	void PutPlugin(const char * pKey, const char * pVal);
	void PutPlugin(const char * pKey, double val);
	void PutPlugin(const char * pKey, long val);
	void PutPlugin(const char * pKey, LDATE val);
private:
	int    ReplaceSpecSymb;
	xmlTextWriter * P_Writer;
	SString TempBuf;
};

XmlWriter::XmlWriter(const char * pPath, int replaceSpecSymb) : ReplaceSpecSymb(replaceSpecSymb), P_Writer(sstrlen(pPath) ? xmlNewTextWriterFilename(pPath, 0) : 0)
{
	if(P_Writer) {
		xmlTextWriterSetIndent(P_Writer, 1);
		xmlTextWriterSetIndentTab(P_Writer);
		xmlTextWriterStartDocument(P_Writer, 0, "utf8", 0);
	}
}

XmlWriter::~XmlWriter()
{
	if(P_Writer) {
		xmlTextWriterEndDocument(P_Writer);
		xmlFreeTextWriter(P_Writer);
		P_Writer = 0;
	}
}

void XmlWriter::PutPlugin(const char * pKey, const char * pVal)
{
	if(sstrlen(pKey) && sstrlen(pVal)) {
		SString key, val;
		(key = pKey).Transf(CTRANSF_INNER_TO_UTF8);
		(val = pVal).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ').ReplaceChar(30, ' ');
		if(ReplaceSpecSymb)
			XMLReplaceSpecSymb(val, "&<>\'");
		val.Transf(CTRANSF_INNER_TO_UTF8);
		StartElement("plugin-property");
		AddAttrib("key", pKey);
		AddAttrib("value", pVal);
		EndElement();
	}
}

void XmlWriter::PutPlugin(const char * pKey, double val)
{
	if(val != 0.0) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		PutPlugin(pKey, r_temp_buf.Cat(val));
	}
}

void XmlWriter::PutPlugin(const char * pKey, long val)
{
	if(val != 0) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		PutPlugin(pKey, r_temp_buf.Cat(val));
	}
}

void XmlWriter::PutPlugin(const char * pKey, LDATE val)
{
	if(val != ZERODATE) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		PutPlugin(pKey, r_temp_buf.Cat(val, DATF_ISO8601CENT));
	}
}

int XmlWriter::StartElement(const char * pName, const char * pAttribName /*=0*/, const char * pAttribValue /*=0*/)
{
	int    ok = 0;
	if(P_Writer && sstrlen(pName)) {
		(TempBuf = pName).Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterStartElement(P_Writer, TempBuf.ucptr());
		AddAttrib(pAttribName, pAttribValue);
		ok = 1;
	}
	return ok;
}

void XmlWriter::EndElement()
{
	xmlTextWriterEndElement(P_Writer);
}

int XmlWriter::AddAttrib(const char * pAttribName, bool attribValue) { return AddAttrib(pAttribName, TempBuf.Z().Cat(STextConst::GetBool(attribValue))); }
int XmlWriter::AddAttrib(const char * pAttribName, long attribValue) { return AddAttrib(pAttribName, TempBuf.Z().Cat(attribValue)); }
int XmlWriter::AddAttrib(const char * pAttribName, double attribValue) { return AddAttrib(pAttribName, TempBuf.Z().Cat(attribValue, MKSFMTD(0, 2, NMBF_NOTRAILZ))); }

int XmlWriter::AddAttrib(const char * pAttribName, const char * pAttribValue)
{
	int    ok = 0;
	if(P_Writer && sstrlen(pAttribName) && sstrlen(pAttribValue)) {
		SString attrib_name, attrib_value;
		(attrib_name = pAttribName).Transf(CTRANSF_INNER_TO_UTF8);
		(attrib_value = pAttribValue).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ').ReplaceChar(30, ' ');
		if(ReplaceSpecSymb)
			XMLReplaceSpecSymb(attrib_value, "&<>\'");
		attrib_value.Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterWriteAttribute(P_Writer, attrib_name.ucptr(), attrib_value.ucptr());
		ok = 1;
	}
	return ok;
}

int XmlWriter::AddAttrib(const char * pName, LDATETIME val) { return AddAttrib(pName, TempBuf.Z().Cat(val, DATF_ISO8601CENT, 0)); }
int XmlWriter::AddAttrib(const char * pName, LDATE val) { return AddAttrib(pName, TempBuf.Z().Cat(val, DATF_ISO8601CENT)); }
int XmlWriter::PutElement(const char * pName, long val) { return PutElement(pName, TempBuf.Z().Cat(val)); }
int XmlWriter::PutElement(const char * pName, double val) { return PutElement(pName, TempBuf.Z().Cat(val)); }
int XmlWriter::PutElement(const char * pName, bool val) { return PutElement(pName, TempBuf.Z().Cat(STextConst::GetBool(val))); }
int XmlWriter::PutElement(const char * pName, LDATETIME val) { return PutElement(pName, TempBuf.Z().Cat(val, DATF_ISO8601CENT, 0)); }
int XmlWriter::PutElement(const char * pName, LDATE val) { return PutElement(pName, TempBuf.Z().Cat(val, DATF_ISO8601CENT)); }
int XmlWriter::PutElement(const char * pName, LTIME val) { return PutElement(pName, TempBuf.Z().Cat(val, TIMF_HMS|TIMF_MSEC)); }

int XmlWriter::PutElement(const char * pName, const char * pValue)
{
	int    ok = 0;
	if(P_Writer && sstrlen(pName) && sstrlen(pValue)) {
		SString name_buf, val_buf;
		(name_buf = pName).Transf(CTRANSF_INNER_TO_UTF8);
		(val_buf = pValue).ReplaceChar('\n', ' ').ReplaceChar('\r', ' ').ReplaceChar(30, ' ');
		if(ReplaceSpecSymb)
			XMLReplaceSpecSymb(val_buf, "&<>\'");
		val_buf.Transf(CTRANSF_INNER_TO_UTF8);
		xmlTextWriterWriteElement(P_Writer, name_buf.ucptr(), val_buf.ucptr());
		ok = 1;
	}
	return ok;
}

struct _SalesGrpEntry { // @flat
	PPID   GrpID;
	char   GrpName[64];
	char   Code[24];
};

struct _MaxDisEntry { // @flat
	char   Barcode[24];
	int16  Deleted;
	int16  NoDis;
};

struct _MinPriceEntry { // @flat
	char   Barcode[24];
	double MinPrice;
	int16  Deleted;
	int16  Reserve; // @alignment
};

struct _ChZnTobaccoEntry { // @flat @v12.0.2
	char   Barcode[24];
	PPID   GoodsID;
};

int ACS_CRCSHSRV::Helper_ExportGoods_V10(const int mode, bool goodsIdAsArticle, const SString & rPathGoods_, const PPAsyncCashNode & rCnData, const SString & rStoreIndex,
	AsyncCashGoodsIterator * pGoodsIter, const SVector & rSalesGrpList, AsyncCashGoodsInfo & rGoodsInfo, SString & rResultFileName)
{
	assert(oneof3(mode, 0, 1, 2));
	rResultFileName.Z();
	int    ok = 1;
	Reference * p_ref = PPRef;
	XmlWriter * p_writer = 0;
	long   plu_num = 1;
	SString temp_buf;
	SString iter_msg;
	SString grp_code;
	AsyncCashGoodsInfo prev_gds_info;
	PPObjUnit unit_obj;
	PPGoodsConfig gds_cfg;
	PPObjGoods gobj;
	PPObjGoodsGroup ggobj;
	PPObjTag tag_obj;
	BarcodeArray barcodes;
	RetailPriceExtractor rpe;
	PrcssrAlcReport::GoodsItem agi;
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	PPID   alc_cls_id = 0;
	PPID   tobacco_cls_id = 0;
	PPID   giftcard_cls_id = 0;
	PPID   sr_prodtagb_tag = 0;
	PPID   prev_goods_id = 0;
	SVector max_dis_list(sizeof(_MaxDisEntry));
	SVector min_price_list(sizeof(_MinPriceEntry)); // @v10.6.4
	SVector chzntobacco_list(sizeof(_ChZnTobaccoEntry)); // @v12.0.2 Список кодов и идентификаторов табачных товаров (для специального plug-in'а для маркировки)
	LDATETIME beg_dtm;
	LDATETIME end_dtm;
	getcurdate(&beg_dtm.d);
	plusperiod(&(end_dtm.d = beg_dtm.d), PRD_ANNUAL, 50, 0);
	beg_dtm.t = encodetime(0,  0, 0, 0);
	end_dtm.t = MAXDAYTIMESEC;
	PPObjGoods::ReadConfig(&gds_cfg);
	rpe.Init(rCnData.LocID, 0, 0, ZERODATETIME, 0);
	{
		PPID   temp_tag_id = 0;
		if(tag_obj.FetchBySymb("setretail-prodtagb", &temp_tag_id) > 0)
			sr_prodtagb_tag = temp_tag_id;
	}
	{
		SString alc_proof;
		SString alc_vol;
		PPObjGoodsClass obj_gdscls;
		PPGdsClsPacket gc_pack;
		alc_cls_id = pGoodsIter->GetAlcoGoodsCls(&alc_proof, &alc_vol);
		if(!(alc_proof.Len() || alc_vol.Len()) || obj_gdscls.Fetch(alc_cls_id, &gc_pack) <= 0)
			alc_cls_id = 0;
		tobacco_cls_id = pGoodsIter->GetTobaccoGoodsCls();
		giftcard_cls_id = pGoodsIter->GetGiftCardGoodsCls();
	}
	{
		temp_buf.Z();
		if(mode == 0)
			temp_buf = rPathGoods_;
		else if(oneof2(mode, 1, 2)) {
			SFsPath ps(rPathGoods_);
			ps.Nam.CatChar('-').Cat((mode == 1) ? "attr" : "prices");
			ps.Merge(temp_buf);
		}
		THROW_MEM(p_writer = new XmlWriter(temp_buf, 1));
		rResultFileName = temp_buf;
	}
	p_writer->StartElement("goods-catalog");
	int    last_goods_export = 0, iter_end = 0;
	const int ignore_lookbackprices = CheckCnExtFlag(CASHFX_IGNLOOKBACKPRICES);
	while((iter_end = pGoodsIter->Next(&rGoodsInfo)) > 0 || !last_goods_export) {
		if(iter_end <= 0)
			last_goods_export = 1;
		if(last_goods_export || rGoodsInfo.ID != prev_goods_id && prev_goods_id) {
			const AsyncCashGoodsInfo & r_cur_entry = prev_gds_info;
			const  bool is_spirit    = (pGoodsIter->GetAlcoGoodsExtension(r_cur_entry.ID, 0, agi) > 0);
			const  bool is_tobacco   = (tobacco_cls_id && tobacco_cls_id == r_cur_entry.GdsClsID);
			const  bool is_gift_card = BIN(giftcard_cls_id && giftcard_cls_id == r_cur_entry.GdsClsID);
			int    tag1212 = 0; // @v11.2.0
			int    do_process_lookbackprices = 0;
			int    is_weight = 0;
			LDATE  expiry = ZERODATE;
			PPID   country_id = 0;
			PPQuotKind qk_rec;
			const long divn = (rCnData.Flags & CASHF_EXPDIVN) ? r_cur_entry.DivN : 1;
			const bool is_deleted = ((r_cur_entry.Flags_ & AsyncCashGoodsInfo::fDeleted) || (r_cur_entry.GoodsFlags & GF_PASSIV && 
				rCnData.ExtFlags & CASHFX_RMVPASSIVEGOODS && r_cur_entry.Rest <= 0.0));
			temp_buf.Z();
			if(CConfig.Flags & CCFLG_DEBUG)
				LogExportingGoodsItem(&r_cur_entry);
			{
				RetailExtrItem rtl_ext_item;
				rpe.GetPrice(r_cur_entry.ID, 0, 0.0, &rtl_ext_item);
				expiry = rtl_ext_item.Expiry;
			}
			char pref_barcode[32];
			STRNSCPY(pref_barcode, r_cur_entry.PrefBarCode);
			AddCheckDigToBarcode(pref_barcode);
			// @v12.0.2 {
			if(oneof2(rGoodsInfo.ChZnProdType, GTCHZNPT_TOBACCO, GTCHZNPT_ALTTOBACCO)) {
				_ChZnTobaccoEntry _entry;
				STRNSCPY(_entry.Barcode, pref_barcode);
				_entry.GoodsID = rGoodsInfo.ID;
				chzntobacco_list.insert(&_entry);
			}
			// } @v12.0.2 
			// @v10.6.8 {
			if(mode == 2) {
				/*
					<goods-catalog>
						<price-entry marking-of-the-good="3558270015196" price="7422.00" currency="RUB" deleted="false">
							<shop-indices>8</shop-indices>
							<department number="1">
								<name>1</name>
							</department>
						</price-entry>
					</goods-catalog>
				*/
				p_writer->StartElement("price-entry");
					p_writer->AddAttrib("marking-of-the-good", pref_barcode);
					p_writer->AddAttrib("price", temp_buf.Z().Cat(r_cur_entry.Price, SFMT_MONEY));
					p_writer->AddAttrib("currency", "RUB");
					p_writer->AddAttrib("deleted", is_deleted ? "true" : "false");
					if(rStoreIndex.NotEmpty())
						p_writer->PutElement("shop-indices", rStoreIndex);
					p_writer->StartElement("department", "number", temp_buf.Z().Cat(divn));
						p_writer->PutElement("name", temp_buf);
					p_writer->EndElement();
				p_writer->EndElement();
			}
			else {
			// } @v10.6.8
				// @v11.4.4 {
				if(goodsIdAsArticle) {
					p_writer->StartElement("good", "marking-of-the-good", temp_buf.Z().Cat(r_cur_entry.ID)); 
				}
				else /* } @v11.4.4 */ {
					p_writer->StartElement("good", "marking-of-the-good", pref_barcode);
				}
				if(oneof2(mode, 0, 2)) { // @v10.6.8 only 0 works
					if(rStoreIndex.NotEmpty())
						p_writer->PutElement("shop-indices", rStoreIndex);
				}
				if(oneof2(mode, 0, 1)) { // @v10.6.7
					p_writer->PutElement("name", r_cur_entry.Name);
					{
						// @v11.4.4 {
						/* @v11.4.5 if(goodsIdAsArticle) {
							// @v10.9.11 {
							if(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGMarkedType) {
								const char * p_mark_type = 0;
								switch(r_cur_entry.ChZnProdType) {
									case GTCHZNPT_SHOE: p_mark_type = "FOOTWEAR"; break;
									case GTCHZNPT_TEXTILE: p_mark_type = "LIGHT_INDUSTRY"; break;
									case GTCHZNPT_CARTIRE: p_mark_type = "TYRES"; break;
									case GTCHZNPT_PERFUMERY: p_mark_type = "PERFUMES"; break;
									case GTCHZNPT_MILK: p_mark_type = "MILK"; break;
								}
								if(p_mark_type)
									p_writer->PutElement("mark-type", p_mark_type);
							}
							// } @v10.9.11 
							p_writer->StartElement("bar-code", "code", temp_buf.Z().Cat(r_cur_entry.ID));
							p_writer->PutElement("count", temp_buf.Z().Cat("1"));
							p_writer->PutElement("default-code", true);
							p_writer->EndElement(); // </bar-code>
						}*/
						// } @v11.4.4 
						for(uint i = 0; i < barcodes.getCount(); i++) {
							BarcodeTbl::Rec bc = barcodes.at(i);
							if(sstrlen(bc.Code)) {
								if(!is_weight && !is_spirit && !is_tobacco && !is_gift_card)
									is_weight = gds_cfg.IsWghtPrefix(bc.Code);
								AddCheckDigToBarcode(bc.Code);
								// @v10.9.11 {
								if(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGMarkedType) {
									const char * p_mark_type = 0;
									switch(r_cur_entry.ChZnProdType) {
										case GTCHZNPT_SHOE: p_mark_type = "FOOTWEAR"; break;
										case GTCHZNPT_TEXTILE: p_mark_type = "LIGHT_INDUSTRY"; break;
										case GTCHZNPT_CARTIRE: p_mark_type = "TYRES"; break;
										case GTCHZNPT_PERFUMERY: p_mark_type = "PERFUMES"; break;
										case GTCHZNPT_MILK: p_mark_type = "MILK"; break;
										case GTCHZNPT_WATER: p_mark_type = "WATER"; break; // @v11.5.6
										case GTCHZNPT_DRAFTBEER: p_mark_type = "DRAFT_BEER"; break; // @v11.9.2
									}
									if(p_mark_type)
										p_writer->PutElement("mark-type", p_mark_type);
								}
								// } @v10.9.11 
								p_writer->StartElement("bar-code", "code", bc.Code);
								// @v10.4.11 {
								if((r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGMarkedType) || IsInnerBarcodeType(bc.BarcodeType, BARCODE_TYPE_MARKED)) {
									p_writer->AddAttrib("marked", "true");
								}
								// } @v10.4.11
								// p_writer->StartElement("price-entry", "price", temp_buf.Z().Cat(r_cur_entry.Price));
								// p_writer->PutElement("begin-date", beg_dtm);
								// p_writer->PutElement("end-date", end_dtm);
								// p_writer->EndElement();
								p_writer->PutElement("count", bc.Qtty);
								// @v11.4.5 if(!goodsIdAsArticle) // @v11.4.4 
									p_writer->PutElement("default-code", LOGIC(sstreq(bc.Code, pref_barcode)));
								p_writer->EndElement(); // </bar-code>
							}
						}
					}
					// тип товара:
					// ProductPieceEntity - штучный, ProductWeightEntity - весовой и т.д.
					if(is_spirit || r_cur_entry.ChZnProdType == GTCHZNPT_DRAFTBEER) { // @v11.9.2 (r_cur_entry.ChZnProdType == GTCHZNPT_DRAFTBEER)
						p_writer->PutElement("product-type", "ProductSpiritsEntity");
						tag1212 = 2; // @v11.2.0
					}
					else if(is_tobacco) {
						p_writer->PutElement("product-type", "ProductCiggyEntity");
						tag1212 = (r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGMarkedType) ? 31 : 30; // @v11.2.0
						if(!ignore_lookbackprices)
							do_process_lookbackprices = 1;
					}
					else if(is_gift_card) {
						p_writer->PutElement("product-type", "ProductGiftCardEntity");
						tag1212 = 10; // @v11.2.0
					}
					else {
						if(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGMarkedType)
							tag1212 = 33; // @v11.2.0
						else {
							if(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fGExciseProForma) // @v11.7.10
								tag1212 = 2;
							else
								tag1212 = 1;
						}
						if(is_weight == 1)
							p_writer->PutElement("product-type", "ProductWeightEntity");
						else if(is_weight == 2)
							p_writer->PutElement("product-type", "ProductPieceWeightEntity");
						else
							p_writer->PutElement("product-type", "ProductPieceEntity");
					}
					if(sr_prodtagb_tag && p_ref->Ot.GetTagStr(PPOBJ_GOODS, r_cur_entry.ID, sr_prodtagb_tag, temp_buf) > 0) {
						temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
						p_writer->PutElement(temp_buf, "true");
					}
				}
				if(mode == 0) {
					p_writer->StartElement("price-entry", "price", temp_buf.Z().Cat(r_cur_entry.Price, SFMT_MONEY));
					p_writer->PutElement("begin-date", beg_dtm);
					p_writer->PutElement("end-date", end_dtm);
					p_writer->PutElement("number", "1");
					p_writer->StartElement("department", "number", temp_buf.Z().Cat(divn));
					p_writer->PutElement("name", temp_buf);
					p_writer->EndElement();
					p_writer->EndElement();
				}
				if(oneof2(mode, 0, 1)) {
					p_writer->PutElement("vat", r_cur_entry.VatRate);
					//
					// Иерархия товарных групп
					//
					if(rCnData.Flags & CASHF_EXPGOODSGROUPS && r_cur_entry.ParentID) {
						Goods2Tbl::Rec ggrec;
						grp_code.Z();
						if(gobj.FetchSingleBarcode(r_cur_entry.ParentID, grp_code) > 0 && grp_code.Len())
							grp_code.ShiftLeftChr('@');
						else
							grp_code.Z().Cat(r_cur_entry.ParentID);
						p_writer->StartElement("group", "id", grp_code);
						GetObjectName(PPOBJ_GOODSGROUP, r_cur_entry.ParentID, temp_buf);
						p_writer->PutElement("name", temp_buf);
						{
							PPID   parent_id = r_cur_entry.ParentID;
							uint   parents_count = 0;
							while(gobj.GetParentID(parent_id, &parent_id) > 0 && parent_id != 0) {
								if(gobj.FetchSingleBarcode(parent_id, grp_code) > 0 && grp_code.Len())
									grp_code.ShiftLeftChr('@');
								else
									grp_code.Z().Cat(parent_id);
								p_writer->StartElement("parent-group");
								p_writer->AddAttrib("id", grp_code);
								GetObjectName(PPOBJ_GOODSGROUP, parent_id, temp_buf);
								p_writer->PutElement("name", temp_buf);
								parents_count++;
							}
							for(uint i = 0; i < parents_count; i++)
								p_writer->EndElement(); // </parent-group>
						}
						p_writer->EndElement(); // </group>
					}
					{
						PPUnit2 unit_rec;
						unit_obj.Fetch(r_cur_entry.UnitID, &unit_rec);
						p_writer->StartElement("measure-type");
						p_writer->AddAttrib("id", unit_rec.ID);
						p_writer->PutElement("name", unit_rec.Name);
						p_writer->EndElement();
					}
					{
						if(country_id) {
							p_writer->StartElement("country", "id", temp_buf.Z().Cat(country_id));
							GetObjectName(PPOBJ_WORLD, country_id, temp_buf.Z());
							p_writer->PutElement("name", temp_buf);
							p_writer->EndElement();
						}
						if(r_cur_entry.ManufID) {
							p_writer->StartElement("manufacturer", "id", temp_buf.Z().Cat(r_cur_entry.ManufID));
							GetPersonName(r_cur_entry.ManufID, temp_buf.Z());
							p_writer->PutElement("name", temp_buf);
							p_writer->EndElement();
						}
					}
					//
					// Загрузка принадлежности группам продаж
					//
					if(r_eq_cfg.SalesGoodsGrp != 0) {
						uint   sg_pos = 0;
						PPID   sub_grp_id = 0;
						if(ggobj.BelongToGroup(r_cur_entry.ID, r_eq_cfg.SalesGoodsGrp, &sub_grp_id) > 0 && sub_grp_id && rSalesGrpList.bsearch(&sub_grp_id, &sg_pos, CMPF_LONG)) {
							const _SalesGrpEntry * p_sentry = static_cast<const _SalesGrpEntry *>(rSalesGrpList.at(sg_pos));
							p_writer->StartElement("sale-group");
							p_writer->AddAttrib("id", sub_grp_id);
							p_writer->PutElement("name", p_sentry->GrpName);
							p_writer->EndElement();
						}
					}
					p_writer->PutPlugin("precision", r_cur_entry.Precision);
					if(is_spirit) {
						if(agi.Proof != 0.0) {
							temp_buf.Z().Cat(agi.Proof, MKSFMTD(0, 1, 0));
							p_writer->PutPlugin("alcoholic-content-percentage", temp_buf);
						}
						if(agi.StatusFlags & agi.stMarkWanted)
							p_writer->PutElement("excise", true);
					}
					else if(!is_weight) {
						if(checkdate(expiry))
							p_writer->PutPlugin("best-before", expiry);
					}
					else {
						SString ingred, storage, energy;
						// 1. composition (состав)
						// 2. storage-conditions (условия хранения)
						// 3. food-value (пищевая ценность)
						{
							uint   ss_pos = 0;
							r_cur_entry.AddedMsgList.get(&ss_pos, ingred) && r_cur_entry.AddedMsgList.get(&ss_pos, storage) && r_cur_entry.AddedMsgList.get(&ss_pos, energy);
						}
						if(checkdate(expiry)) {
							const long hours = diffdate(expiry, beg_dtm.d) * 24;
							p_writer->PutPlugin("good-for-hours", hours);
							// p_writer->PutPlugin("best-before", expiry);
						}
						p_writer->PutPlugin("composition", ingred);
						p_writer->PutPlugin("storage-conditions", storage);
						p_writer->PutPlugin("food-value", energy);
						p_writer->PutPlugin("plu-number", plu_num++);
						p_writer->PutPlugin("description-on-scale-screen", r_cur_entry.LabelName);
						p_writer->PutPlugin("name-on-scale-screen", r_cur_entry.Name);
					}
					// @v11.2.0 {
					{
						if(is_weight)
							p_writer->PutElement("ffd-tag-2108", "11"); // kg
						else
							p_writer->PutElement("ffd-tag-2108", "0"); // piece
						if(tag1212 > 0)
							p_writer->PutElement("ffd-tag-1212", temp_buf.Z().Cat(tag1212)); 
						//<ffd-tag-2108>5</ffd-tag-2108>
						//<ffd-tag-1212>4</ffd-tag-1212>
					}
					// } @v11.2.0 
				}
				if(oneof2(mode, 0, 2)) { // @v10.6.8 only 0 works
					if(do_process_lookbackprices) {
						RealArray price_list;
						if(pGoodsIter->GetDifferentPricesForLookBackPeriod(r_cur_entry.ID, r_cur_entry.Price, price_list) > 0 || (ModuleSubVer >= 2)) { // @v10.8.2 (ModuleSubVer >= 2)
							// @v12.0.2 assert(price_list.getCount());
							p_writer->StartElement("plugin-property", "key", "mrc");
							for(uint pi = 0; pi < price_list.getCount(); pi++) {
								p_writer->PutPlugin("price", price_list[pi]);
							}
							if(ModuleSubVer >= 2) {
								p_writer->PutPlugin("price", r_cur_entry.Price);
							}
							p_writer->EndElement(); // </plugin-property>
							/*
								<plugin-property key="mrc">
								<plugin-property key="price" value="15.00"/>
								<plugin-property key="price" value="18.00"/>
								<plugin-property key="price" value="25.00"/>
								</plugin-property>
							*/
						}
					}
				}
				if(oneof2(mode, 0, 1)) {
					if(is_deleted)
						p_writer->PutElement("delete-from-cash", true);
				}
				p_writer->EndElement(); // </good>
				if((r_cur_entry.Flags_ & AsyncCashGoodsInfo::fDeleted) || labs(r_cur_entry.NoDis) == 1) {
					_MaxDisEntry dis_entry;
					STRNSCPY(dis_entry.Barcode, pref_barcode);
					dis_entry.NoDis = r_cur_entry.NoDis;
					dis_entry.Deleted = BIN(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fDeleted);
					max_dis_list.insert(&dis_entry);
				}
				if(r_cur_entry.AllowedPriceR.low > 0.0) {
					_MinPriceEntry mp_entry;
					STRNSCPY(mp_entry.Barcode, pref_barcode);
					mp_entry.MinPrice = r_cur_entry.AllowedPriceR.low;
					mp_entry.Deleted = BIN(r_cur_entry.Flags_ & AsyncCashGoodsInfo::fDeleted);
					mp_entry.Reserve = 0;
					min_price_list.insert(&mp_entry);
				}
			}
			barcodes.clear();
		}
		if(rGoodsInfo.BarCode[0]) {
			BarcodeTbl::Rec bc;
			bc.GoodsID = rGoodsInfo.ID;
			STRNSCPY(bc.Code, rGoodsInfo.BarCode);
			bc.Qtty = rGoodsInfo.UnitPerPack;
			if(bc.Qtty <= 0.0)
				bc.Qtty = 1.0;
			if(rGoodsInfo.Flags_ & rGoodsInfo.fGMarkedCode)
				SetInnerBarcodeType(&bc.BarcodeType, BARCODE_TYPE_MARKED);
			IsInnerBarcodeType(bc.BarcodeType, BARCODE_TYPE_MARKED);
			if(rGoodsInfo.PrefBarCode[0] && sstreq(bc.Code, rGoodsInfo.PrefBarCode)) {
				//bc.BarcodeType = BARCODE_TYPE_PREFERRED;
				SetInnerBarcodeType(&bc.BarcodeType, BARCODE_TYPE_PREFERRED);
			}
			barcodes.insert(&bc);
		}
		prev_goods_id = rGoodsInfo.ID;
		prev_gds_info = rGoodsInfo;
		PPWaitPercent(pGoodsIter->GetIterCounter());
	}
	if(oneof2(mode, 0, 2)) {
		const char * p_subj_type = "GOOD";
		SString restr_id;
		//
		// Выгрузка максимальных скидок на товар
		//
		PPLoadText(PPTXT_MAXDISEXPORT, iter_msg);
		{
			for(uint i = 0; i < max_dis_list.getCount(); i++) {
				const char * p_type = "MAX_DISCOUNT_PERCENT";
				const _MaxDisEntry * p_entry = static_cast<const _MaxDisEntry *>(max_dis_list.at(i));
				(restr_id = p_subj_type).CatChar('-').Cat(p_entry->Barcode).CatChar('-').Cat(p_type);
				p_writer->StartElement("max-discount-restriction");
	 			p_writer->AddAttrib("id", restr_id.cptr());
				p_writer->AddAttrib("subject-type", p_subj_type);
				p_writer->AddAttrib("subject-code", p_entry->Barcode);
				p_writer->AddAttrib("type", p_type);
				p_writer->AddAttrib("value", (p_entry->NoDis > 0) ? 0.00 : 99.99);
				p_writer->PutElement("since-date", beg_dtm);
				p_writer->PutElement("till-date", end_dtm);
				p_writer->PutElement("since-time", beg_dtm.t);
				p_writer->PutElement("till-time", end_dtm.t);
				p_writer->PutElement("deleted", LOGIC(p_entry->Deleted));
				p_writer->PutElement("days-of-week", "MO TU WE TH FR SA SU");
				// @v10.4.6 {
				if(rStoreIndex.NotEmpty())
					p_writer->PutElement("shop-indices", rStoreIndex); //<shop-indices>2</shop-indices>
				// } @v10.4.6
				p_writer->EndElement(); // </max-discount-restriction>
				PPWaitPercent(i + 1, max_dis_list.getCount(), iter_msg);
			}
		}
		{
			const char * p_type = "MIN_PRICE";
			//
			// @v10.6.4 {
			// Выгрузка минимальных допустимых цен
			//
			if(min_price_list.getCount()) {
				for(uint i = 0; i < min_price_list.getCount(); i++) {
					const _MinPriceEntry * p_entry = static_cast<const _MinPriceEntry *>(min_price_list.at(i));
					(restr_id = p_subj_type).CatChar('-').Cat(p_entry->Barcode).CatChar('-').Cat(p_type);
					p_writer->StartElement("min-price-restriction");
	 				p_writer->AddAttrib("id", restr_id.cptr());
					p_writer->AddAttrib("subject-type", p_subj_type);
					p_writer->AddAttrib("subject-code", p_entry->Barcode);
					p_writer->AddAttrib("type", p_type);
					p_writer->AddAttrib("value", p_entry->MinPrice);
					p_writer->PutElement("since-date", beg_dtm);
					p_writer->PutElement("till-date", end_dtm);
					p_writer->PutElement("since-time", beg_dtm.t);
					p_writer->PutElement("till-time", end_dtm.t);
					p_writer->PutElement("deleted", LOGIC(p_entry->Deleted));
					p_writer->PutElement("days-of-week", "MO TU WE TH FR SA SU");
					if(rStoreIndex.NotEmpty())
						p_writer->PutElement("shop-indices", rStoreIndex); //<shop-indices>2</shop-indices>
					p_writer->EndElement(); // </max-discount-restriction>
					PPWaitPercent(i + 1, min_price_list.getCount(), iter_msg);
				}
			}
			if(chzntobacco_list.getCount()) {
				//chzntobacco_id_list.sortAndUndup();
				const double min_tobacco_price = 129.0;
				for(uint i = 0; i < chzntobacco_list.getCount(); i++) {
					const _ChZnTobaccoEntry * r_entry = static_cast<const _ChZnTobaccoEntry *>(chzntobacco_list.at(i));
					/*
					@20240409 Для табака тоже надо выгружать min-price-restriction
						<min-price-restriction id="GOOD-46245991-MIN_PRICE_PERCENT" subject-type="GOOD" subject-code="46245991" type="MIN_PRICE" value="50.00">
  							<since-date>2024-04-02T00:00:00.000</since-date>
  							<till-date>2074-04-02T23:59:59.000</till-date>
  							<since-time>00:00:00.000</since-time>
  							<till-time>23:59:59.000</till-time>
  							<deleted>false</deleted>
  							<days-of-week>MO TU WE TH FR SA SU</days-of-week>
						 </min-price-restriction>
					*/
					//const _MinPriceEntry * p_entry = static_cast<const _MinPriceEntry *>(min_price_list.at(i));
					(restr_id = p_subj_type).CatChar('-').Cat(r_entry->Barcode).CatChar('-').Cat(p_type);
					p_writer->StartElement("min-price-restriction");
	 				p_writer->AddAttrib("id", restr_id.cptr());
					p_writer->AddAttrib("subject-type", p_subj_type);
					p_writer->AddAttrib("subject-code", temp_buf.Z().Cat(r_entry->Barcode));
					p_writer->AddAttrib("type", p_type);
					p_writer->AddAttrib("value", temp_buf.Z().Cat(min_tobacco_price, MKSFMTD(0, 2, 0)));
					p_writer->PutElement("since-date", beg_dtm);
					p_writer->PutElement("till-date", end_dtm);
					p_writer->PutElement("since-time", beg_dtm.t);
					p_writer->PutElement("till-time", end_dtm.t);
					p_writer->PutElement("deleted", false);
					p_writer->PutElement("days-of-week", "MO TU WE TH FR SA SU");
					if(rStoreIndex.NotEmpty())
						p_writer->PutElement("shop-indices", rStoreIndex); //<shop-indices>2</shop-indices>
					p_writer->EndElement(); // </max-discount-restriction>
				}
			}
			// } @v10.6.4
		}
	}
	p_writer->EndElement(); // </goods-catalog>
	CATCHZOK
	ZDELETE(p_writer);
	return ok;
}

int ACS_CRCSHSRV::ExportDataV10(int updOnly)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	int    add_time_to_fname = 0;
	int    use_new_dscnt_code_alg = 0;
	int    diff_goods_export = 0;
	int    goodsid_as_article = 0; // @v11.4.4
	uint   i = 0;
	SString temp_buf;
	SString path;
	SString path_goods;
	SString path_cashiers;
	SString path_cards;
	SString iter_msg;
	SString store_index; // Индекс магазина (извлекается из тега склада по конфигурационному параметру PPLocationConfig::StoreIdxTag)
	StringSet ss_path_goods;
	PPObjTag  tag_obj;
	PPObjQuotKind  qk_obj;
	XmlWriter * p_writer = 0;
	PPAsyncCashNode cn_data;
	AsyncCashGoodsInfo gds_info;
	AsyncCashGoodsGroupInfo grp_info;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	SVector sales_grp_list(sizeof(_SalesGrpEntry));
	PPObjGoodsGroup ggobj;
	//
	// Список ассоциаций {Серия карты; Вид котировки} => Key - серия карты, Val - вид котировки
	//
	LAssocArray scard_quot_ary, dscnt_code_ary;
	PPQuotArray grp_dscnt_ary;
	PPIniFile ini_file;
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	PPWaitStart();
	THROW(GetNodeData(&cn_data) > 0);
	{
		PPLocationConfig loc_cfg;
		PPObjLocation::ReadConfig(&loc_cfg);
		if(loc_cfg.StoreIdxTagID)
			p_ref->Ot.GetTagStr(PPOBJ_LOCATION, cn_data.LocID, loc_cfg.StoreIdxTagID, store_index);
	}
	// @v10.4.12 {
	/* @v10.6.3 (moved to Helper_ExportGoods_V10()) {
		PPID   temp_tag_id = 0;
		if(tag_obj.FetchBySymb("setretail-prodtagb", &temp_tag_id) > 0)
			sr_prodtagb_tag = temp_tag_id;
	}*/
	// } @v10.4.12
	// @v10.6.3 (moved to Helper_ExportGoods_V10()) rpe.Init(cn_data.LocID, 0, 0, ZERODATETIME, 0);
	//const int check_dig  = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	THROW(DistributeFile_(0, 0/*pEndFileName*/, dfactCheckDestPaths, SUBDIR_PRODUCTS, 0));
	THROW(DistributeFile_(0, 0/*pEndFileName*/, dfactCheckDestPaths, SUBDIR_CARDS, 0));
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_ADDTIMETOFILENAMES, &add_time_to_fname);
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_USENEWDSCNTCODEALG, &use_new_dscnt_code_alg);
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_DIFFGOODSEXPORT, &diff_goods_export); // @v10.6.3
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_GOODSIDASARTICLE, &goodsid_as_article); // @v11.4.4
	THROW(PPGetFilePath(PPPATH_OUT, GOODS_XML,            path_goods));
	THROW(PPGetFilePath(PPPATH_OUT, CASHIERS_XML,         path_cashiers));
	THROW(PPGetFilePath(PPPATH_OUT, CARDS_XML,            path_cards));
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_CASH_UPD, path));
	if(add_time_to_fname) {
		AddTimeToFileName(path_goods);
	}
	createEmptyFile(path);
	THROW_MEM(SETIFZ(P_Dls, new DeviceLoadingStat));
	//
	// Подготовка списка групп продаж
	//
	if(r_eq_cfg.SalesGoodsGrp != 0) {
		SString code;
		PPIDArray _grp_list;
		ggobj.P_Tbl->GetGroupTerminalList(r_eq_cfg.SalesGoodsGrp, &_grp_list, 0);
		if(_grp_list.getCount()) {
			for(i = 0; i < _grp_list.getCount(); i++) {
				PPGoodsPacket gds_pack;
				if(ggobj.GetPacket(_grp_list.at(i), &gds_pack, PPObjGoods::gpoSkipQuot) > 0 && gds_pack.GetGroupCode(code) > 0) {
					_SalesGrpEntry sales_grp_item;
					sales_grp_item.GrpID = gds_pack.Rec.ID;
					STRNSCPY(sales_grp_item.GrpName, gds_pack.Rec.Name);
					STRNSCPY(sales_grp_item.Code, code);
					sales_grp_list.insert(&sales_grp_item);
				}
			}
		}
		sales_grp_list.sort(CMPF_LONG);
	}
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	PROFILE_START
	/* @v10.6.3 (moved to Helper_ExportGoods_V10())
	alc_cls_id = p_gds_iter->GetAlcoGoodsCls(&alc_proof, &alc_vol);
	if(!(alc_proof.Len() || alc_vol.Len()) || obj_gdscls.Fetch(alc_cls_id, &gc_pack) <= 0)
		alc_cls_id = 0;
	tobacco_cls_id = p_gds_iter->GetTobaccoGoodsCls();
	giftcard_cls_id = p_gds_iter->GetGiftCardGoodsCls();
	*/
	//
	// Инициализируем список видов котировок, которые нам понадобятся от RetailGoodsExtractor для экспорта
	//
	for(i = 0; i < scard_quot_ary.getCount(); i++)
		if(scard_quot_ary.at(i).Val)
			gds_info.QuotList.Add(scard_quot_ary.at(i).Val, 0, 1);
	//
	{
		long acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		//THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, acgif, SinceDlsID, P_Dls));
		if(diff_goods_export) {
			{
				AsyncCashGoodsIterator giter(NodeID, acgif, SinceDlsID, P_Dls);
				THROW(Helper_ExportGoods_V10(1/*mode*/, LOGIC(goodsid_as_article), path_goods, cn_data, store_index, &giter, sales_grp_list, gds_info, temp_buf));
				ss_path_goods.add(temp_buf);
			}
			{
				AsyncCashGoodsIterator giter(NodeID, acgif, SinceDlsID, P_Dls);
				THROW(Helper_ExportGoods_V10(2/*mode*/, LOGIC(goodsid_as_article), path_goods, cn_data, store_index, &giter, sales_grp_list, gds_info, temp_buf));
				ss_path_goods.add(temp_buf);
			}
		}
		else {
			AsyncCashGoodsIterator giter(NodeID, acgif, SinceDlsID, P_Dls);
			THROW(Helper_ExportGoods_V10(0/*mode*/, LOGIC(goodsid_as_article), path_goods, cn_data, store_index, &giter, sales_grp_list, gds_info, temp_buf));
			ss_path_goods.add(temp_buf);
		}
	}
	//
	PROFILE_END
	ZDELETE(p_grp_iter);
	//ZDELETE(p_gds_iter);
	/* кассиры заводятся в сете
	if(EqCfg.CshrsPsnKindID) {
		AsyncCashierInfo      cshr_info;
		AsyncCashiersIterator cshr_iter;

		THROW(p_writer = new XmlWriter(path_cashiers, 1));
		cshr_iter.Init(NodeID);
		while(cshr_iter.Next(&cshr_info) > 0) {
			p_writer->StartElement("cashUser");
			p_writer->PutElement("tabNum", cshr_info.TabNum);
			p_writer->PutElement("firstName", cshr_info.Name);
			// p_writer->PutElement("middleName", );
			// p_writer->PutElement("lastName", );
			// p_writer->PutElement("Birth", );
			// p_writer->PutElement("Login", );
			p_writer->PutElement("Blocked", !cshr_info.IsWorked);
			// p_writer->PutElement("barcode", );
			// p_writer->PutElement("magneticCard", );
			// p_writer->PutElement("magneticKey", );
			//p_writer->StartElement("cashUserSession", "seId", );
			//p_writer->PutElement("DateBegin", );
			//p_writer->PutElement("DateEnd", );
			//p_writer->EndElement(); // </cashUserSession>
			p_writer->EndElement(); // </cashUser>
		}
		ZDELETE(p_writer);
	}
	*/
	//
	// Дисконтные карты
	//
	{
		PPID   ser_id = 0;
		long   idx = 0;
		SString name, series_word;
		SString ser_name;
		SString fmt_buf;
		SString msg_buf;
		SString ser_ident;
		const LDATETIME now_dtm = getcurdatetime_();
		SFsPath sp;
		PPSCardSeries2 ser_rec;
		PPObjSCardSeries scs_obj;
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
		PPLoadText(PPTXT_EXPSCARD, iter_msg);
		// @v10.7.11 PPGetWord(PPWORD_SERIES, 0, series_word);
		PPLoadString("series", series_word); // @v10.7.11
		scard_quot_ary.clear();
		sp.Split(path_cards);
		(name = sp.Nam).CatChar('_').CatLongZ(now_dtm.d.day(), 2).CatChar('-').CatLongZ(now_dtm.d.month(), 2).CatChar('-').CatLongZ(now_dtm.d.year(), 4).
			CatChar('_').CatLongZ(now_dtm.t.hour(), 2).CatChar('-').CatLongZ(now_dtm.t.minut(), 2).CatChar('-').CatLongZ(now_dtm.t.sec(), 2);
		sp.Nam = name;
		sp.Merge(path_cards);
		THROW_MEM(p_writer = new XmlWriter(path_cards, 1));
		p_writer->StartElement("cards-catalog");
		PPLoadText(PPTXT_EXPSCARD, fmt_buf);
		if(ModuleSubVer >= 3) {
			int    zero_series_has_seen = 0;
			for(ser_id = 0, idx = 1; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
				if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
					int    skip = 0;
					ser_name = ser_rec.Name;
					ser_ident = ser_rec.Symb;
					if(!ser_ident.NotEmptyS() || !ser_ident.IsDec()) {
						ser_ident.Z().CatChar('0');
						if(zero_series_has_seen)
							skip = 1;
						else
							zero_series_has_seen = 1;
					}
					if(!skip) {
						p_writer->StartElement("internal-card-type");
							p_writer->AddAttrib("deleted", false);
							p_writer->AddAttrib("guid", ser_ident);
							p_writer->AddAttrib("name", ser_name);
							p_writer->AddAttrib("personalized", true); // @v11.3.7
							p_writer->AddAttrib("percentage-discount", temp_buf.Z().Cat(fdiv100i(ser_rec.PDis), MKSFMTD(0, 2, NMBF_EXPLFLOAT)));
						p_writer->EndElement();
					}
				}
			}
			for(ser_id = 0, idx = 1; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
				if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
					AsyncCashSCardInfo info;
					PPSCardSerPacket scs_pack;
					THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
					THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
					(msg_buf = fmt_buf).CatDiv(':', 2).Cat(ser_rec.Name);
					ser_name = ser_rec.Name;
					ser_ident = ser_rec.Symb;
					if(!ser_ident.NotEmptyS() || !ser_ident.IsDec())
						ser_ident.Z().CatChar('0');
					for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
						PPWaitPercent(iter.GetCounter(), msg_buf);
						LDATETIME expiry;
						expiry.d = info.Rec.Expiry;
						p_writer->StartElement("internal-card");
							p_writer->AddAttrib("amount", temp_buf.Z().Cat(info.Rec.Turnover, MKSFMTD(0, 2, NMBF_EXPLFLOAT)));
							p_writer->AddAttrib("deleted", false);
							//p_writer->AddAttrib("discountpercent", fdiv100i(info.Rec.PDis));
							// @v10.1.0 p_writer->AddAttrib("card-type-guid", ser_ident); // @v9.4.1
							p_writer->AddAttrib("percentage-discount", temp_buf.Z().Cat(fdiv100i(info.Rec.PDis), MKSFMTD(0, 2, NMBF_EXPLFLOAT)));
							p_writer->AddAttrib("number", info.Rec.Code);
							if(expiry.d != ZERODATE) {
								expiry.d.getactual(ZERODATE);
								p_writer->AddAttrib("expirationDate", expiry.d);
							}
							p_writer->AddAttrib("status", 0L);
							if(info.Rec.PersonID != 0) {
								p_writer->StartElement("client");
								{
									p_writer->AddAttrib("deleted", false);
									p_writer->AddAttrib("guid", info.Rec.PersonID);
									p_writer->AddAttrib("last-name", info.PsnName);
									bool is_thereis_email = false;
									bool is_thereis_phone = false;
									if(checkdate(info.PsnDOB)) {
										temp_buf.Z().Cat(info.PsnDOB, DATF_ISO8601CENT);
										p_writer->AddAttrib("birth-date", temp_buf);
									}
									if(info.Email.NotEmpty()) {
										p_writer->AddAttrib("email", info.Email);
										is_thereis_email = true;
									}
									if(info.Phone.NotEmpty()) {
										p_writer->AddAttrib("mobile-phone", info.Phone); // @v11.3.7 phone-->mobile-phone
										is_thereis_phone = true;
									}
									else if(info.PsnPhone.NotEmpty()) {
										p_writer->AddAttrib("mobile-phone", info.PsnPhone); // @v11.3.7 phone-->mobile-phone
										is_thereis_phone = true;
									}
									p_writer->AddAttrib("send-by-sms", (info.Flags & AsyncCashSCardInfo::fDisableSendPaperlassCCheck) ? false : true);
									p_writer->AddAttrib("send-by-email", (info.Flags & AsyncCashSCardInfo::fDisableSendPaperlassCCheck) ? false : true);
									// @v11.3.9 {
									if(is_thereis_email)
										p_writer->AddAttrib("receipt-feedback", "BY_EMAIL");
									else if(is_thereis_phone)
										p_writer->AddAttrib("receipt-feedback", "BY_PHONE");
									// } @v11.3.9 
								}
								p_writer->EndElement();
							}
						p_writer->EndElement();
						iter.SetStat();
						idx++;
					}
				}
			}
		}
		else {
			for(ser_id = 0, idx = 1; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
				if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
					AsyncCashSCardInfo info;
					PPSCardSerPacket scs_pack;
					THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
					THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
					(msg_buf = fmt_buf).CatDiv(':', 2).Cat(ser_rec.Name);
					ser_ident = ser_rec.Symb;
					if(!ser_ident.NotEmptyS() || !ser_ident.IsDec())
						ser_ident.Z().CatChar('0');
					for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
						PPWaitPercent(iter.GetCounter(), msg_buf);
						LDATETIME expiry;
						expiry.d = info.Rec.Expiry;
						p_writer->StartElement("client");
							if(info.Rec.PersonID != 0) {
								p_writer->AddAttrib("deleted", false);
								p_writer->AddAttrib("guid", info.Rec.PersonID);
								p_writer->AddAttrib("lastName", info.PsnName);
								// @v11.3.5 {
								if(checkdate(info.PsnDOB)) {
									temp_buf.Z().Cat(info.PsnDOB, DATF_ISO8601CENT);
									p_writer->AddAttrib("birth-date", temp_buf);
								}
								if(info.Email.NotEmpty()) {
									p_writer->AddAttrib("email", info.Email);
								}
								if(info.Phone.NotEmpty()) {
									p_writer->AddAttrib("mobile-phone", info.Phone); // @v11.3.7 phone-->mobile-phone
								}
								else if(info.PsnPhone.NotEmpty()) {
									p_writer->AddAttrib("mobile-phone", info.PsnPhone); // @v11.3.7 phone-->mobile-phone
								}
								p_writer->AddAttrib("send-by-sms", (info.Flags & AsyncCashSCardInfo::fDisableSendPaperlassCCheck) ? false : true);
								p_writer->AddAttrib("send-by-email", (info.Flags & AsyncCashSCardInfo::fDisableSendPaperlassCCheck) ? false : true);
								// } @v11.3.5 
							}
							ser_name = ser_rec.Name;
							p_writer->StartElement("internal-card-type");
								p_writer->AddAttrib("deleted", false);
								p_writer->AddAttrib("guid", ser_ident);
								p_writer->AddAttrib("name", ser_name);
								p_writer->AddAttrib("personalized", LOGIC(info.Rec.PersonID != 0));
								// p_writer->AddAttrib("workPeriodStart", info.Rec.UsageTmStart.d);
								// p_writer->AddAttrib("workPeriodEnd", info.Rec.UsageTmEnd.d);
								p_writer->StartElement("internal-card");
									p_writer->AddAttrib("amount", info.Rec.Turnover);
									p_writer->AddAttrib("deleted", false);
									p_writer->AddAttrib("discountpercent", fdiv100i(info.Rec.PDis));
									p_writer->AddAttrib("numberField", info.Rec.Code);
									if(expiry.d != ZERODATE) {
										expiry.d.getactual(ZERODATE);
										p_writer->AddAttrib("expirationDate", expiry.d);
									}
									p_writer->AddAttrib("status", 0L);
								p_writer->EndElement();
							p_writer->EndElement();
						p_writer->EndElement();
						iter.SetStat();
						idx++;
					}
				}
			}
		}
		p_writer->EndElement();
		ZDELETE(p_writer);
	}
	PPWaitStop();
	PPWaitStart();
	THROW(DistributeFile_(path, 0/*pEndFileName*/, dfactCopy, SUBDIR_PRODUCTS, 0));
	{
		//ps.Nam.CatChar('-').Cat((mode == 1) ? "attr" : "prices");
		for(uint ssp = 0; ss_path_goods.get(&ssp, temp_buf);) {
			if(fileExists(temp_buf)) {
				SDelay(2000);
				THROW(DistributeFile_(/*path_goods*/temp_buf, 0/*pEndFileName*/, dfactCopy, SUBDIR_PRODUCTS, 0));
			}
		}
	}
	SDelay(2000);
	THROW(DistributeFile_(path_cards, 0/*pEndFileName*/, dfactCopy, SUBDIR_CARDS, 0));
	// SDelay(2000);
	// THROW(DistributeFile(path_cashiers, 0, SUBDIR_CASHIERS));
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCH
		SFile::Remove(path_goods);
		SFile::Remove(path_cashiers);
		ok = 0;
	ENDCATCH
	ZDELETE(p_writer);
	PPWaitStop();
	//delete p_gds_iter;
	delete p_grp_iter;
	return ok;
}

class CrsFilePool {
public:
	enum {
		tGoods = 1,
		tDiscount,
		tBarcode,
		tGoodsGrp,
		tGoodsGrpQttyDis,
		tGoodsDis, // DBFS_CRCS_GOODSDIS_EXPORT
		tCards,
		tCashier,
		tGoodsQttyDis,
		tSalesGroup,
		tSalesGroupItems,
		tCashUpd,
		// @v9.2.5 tSignalAll // @v9.2.1
	};
	struct Info {
		Info() : T(0), FilNamId(0), DbfResId(0), P_Tbl(0)
		{
		}
		~Info()
		{
			ZDELETE(P_Tbl);
		}
		int Set(int t, uint filNamId, uint dbfResId, int addTimeToName, int deffered = 0)
		{
			int    ok = 1;
			T = t;
			FilNamId = filNamId;
			DbfResId = dbfResId;
			if(FilNamId) {
				THROW(PPGetFilePath(PPPATH_OUT, FilNamId, FileName));
				if(addTimeToName)
					AddTimeToFileName(FileName);
				if(DbfResId && !deffered) {
					THROW(P_Tbl = CreateDbfTable(DbfResId, FileName, 1));
				}
			}
			CATCHZOK
			return ok;
		}
		int    T;
		uint   FilNamId;
		uint   DbfResId;
		DbfTable * P_Tbl;
		SString FileName;
	};
	Info   List[32];
	SString ZeroString;

	CrsFilePool()
	{
	}
	int Init(int useDscntCode, int addTimeToName)
	{
		int    ok = 1;
		size_t pos = 0;
		THROW(List[pos++].Set(tGoods,           PPFILNAM_CS_GOODS_DBF,       DBFS_CRCS_GOODS_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tDiscount,        PPFILNAM_CS_DSCNT_DBF,       useDscntCode ? DBFS_CRCS_DSCNT_EXP49 : DBFS_CRCS_DSCNT_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tBarcode,         PPFILNAM_CS_BAR_DBF,         DBFS_CRCS_BAR_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tGoodsGrp,        PPFILNAM_CS_GROUP_DBF,       DBFS_CRCS_GROUP_EXPORT, addTimeToName, 1));
		THROW(List[pos++].Set(tGoodsGrpQttyDis, PPFILNAM_CS_GRPQD_DBF,       DBFS_CRCS_GRPQTTYDSC_EXPORT, addTimeToName, 1));
		THROW(List[pos++].Set(tGoodsDis,        PPFILNAM_CS_GOODSDIS_DBF,    DBFS_CRCS_GOODSDIS_EXPORT, addTimeToName, 1));
		THROW(List[pos++].Set(tCards,           PPFILNAM_CS_CARDS_DBF,       DBFS_CRCS_CARDS_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tCashier,         PPFILNAM_CS_CSHRS_DBF,       DBFS_CRCS_CSHRS_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tGoodsQttyDis,    PPFILNAM_CS_GDSQD_DBF,       DBFS_CRCS_GDSQTTYDSC_EXPORT, addTimeToName));
		THROW(List[pos++].Set(tSalesGroup,      PPFILNAM_CS_SALESGGRP_DBF,   DBFS_CRCS_SALESGGRP_EXPORT,  addTimeToName, 1));
		THROW(List[pos++].Set(tSalesGroupItems, PPFILNAM_CS_SALESGGRPI_DBF,  DBFS_CRCS_SALESGGRPI_EXPORT, addTimeToName, 1));
		THROW(List[pos++].Set(tCashUpd,         PPFILNAM_CS_CASH_UPD,        0, 0));
		// @v9.2.5 THROW(List[pos++].Set(tSignalAll,       PPFILNAM_CS_SIGNALALL_DBF,   DBFS_CRCS_SIGNAL_ALL_EXPORT, 0, 1)); // @v9.2.1
		CATCHZOK
		return ok;
	}
	int InitDeffered(int t)
	{
		int    ok = 1;
		for(uint i = 0; i < SIZEOFARRAY(List); i++) {
			Info & r_item = List[i];
			if(r_item.T == t) {
				if(r_item.P_Tbl == 0) {
					THROW(r_item.DbfResId);
					THROW(r_item.P_Tbl = CreateDbfTable(r_item.DbfResId, r_item.FileName, 1));
				}
				break;
			}
		}
		CATCHZOK
		return ok;
	}
	const SString & GetFileName(int t) const
	{
		for(uint i = 0; i < SIZEOFARRAY(List); i++)
			if(List[i].T == t)
				return List[i].FileName;
		return ZeroString;
	}
	DbfTable * GetTable(int t)
	{
		for(uint i = 0; i < SIZEOFARRAY(List); i++)
			if(List[i].T == t)
				return List[i].P_Tbl;
		return 0;
	}
	void CloseFiles()
	{
		for(uint i = 0; i < SIZEOFARRAY(List); i++)
			if(List[i].P_Tbl)
				ZDELETE(List[i].P_Tbl);
	}
	int DistributeFiles(PPAsyncCashSession * pSess)
	{
		int    ok = 1;
		uint   i;
		for(i = 0; i < SIZEOFARRAY(List); i++) {
			if(List[i].T == tCashUpd) {
				THROW(pSess->DistributeFile_(List[i].FileName, 0/*pEndFileName*/, PPAsyncCashSession::dfactCopy, 0, 0));
				SDelay(2000);
				break;
			}
		}
		for(i = 0; i < SIZEOFARRAY(List); i++) {
			if(List[i].T && List[i].T != tCashUpd && fileExists(List[i].FileName)) {
				THROW(pSess->DistributeFile_(List[i].FileName, 0/*pEndFileName*/, PPAsyncCashSession::dfactCopy, 0, 0));
				SDelay(2000);
			}
		}
		for(i = 0; i < SIZEOFARRAY(List); i++) {
			if(List[i].T == tCashUpd) {
				THROW(pSess->DistributeFile_(List[i].FileName, 0/*pEndFileName*/, PPAsyncCashSession::dfactDelete, 0, 0));
				break;
			}
		}
		CATCHZOK
		return ok;
	}
};

static SString & GetDatetimeStrBeg(LDATE dt, int16 tm, SString & rBuf)
{
	rBuf.Z().Cat(!dt ? encodedate(1, 1, 2000) : dt, DATF_GERMANCENT);
	rBuf.Space().Cat(encodetime(*(char *)&tm, *(((char *)&tm) + 1), 0, 0), TIMF_HM);
	return rBuf;
}

static SString & GetDatetimeStrEnd(LDATE dt, int16 tm, SString & rBuf)
{
	rBuf.Z().Cat(!dt ? encodedate(1, 1, 2050) : dt, DATF_GERMANCENT);
	rBuf.Space().Cat(encodetime(*(char *)&tm, *(((char *)&tm) + 1), 0, 0), TIMF_HM);
	return rBuf;
}

struct _CrSetV5_GroupEntry { // @flat
	_CrSetV5_GroupEntry()
	{
		THISZERO();
	}
	PPID   GrpID[5];
	char   GrpName[64];
	long   DivN;
	uint   Level;
};

int ACS_CRCSHSRV::ExportData__(int updOnly)
{
	const long alco_special_grp_id = 100000000;
	const char * p_alco_special_grp_name = "Alcohol-EGAIS";

	int    ok = 1;
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	CrsFilePool fp;
	AsyncCashGoodsIterator * p_gds_iter = 0;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	int    check_dig = 0;
	int    use_dscnt_code = 0;
	int    add_time_to_fname = 0;
	int    use_new_dscnt_code_alg = 0;
	uint   i, k;
	PPID   prev_goods_id = 0;
	long   old_dscnt_code_bias = 0L;
	PPID   loc_id = 0;
	SString dttm_str;
	SString msg_buf, fmt_buf;
	PPObjUnit unit_obj;
	PPUnit    unit_rec;
	PPObjQuotKind qk_obj;
	PPQuotKind    qk_rec;
	PPAsyncCashNode cn_data;
	AsyncCashGoodsInfo gi;
	AsyncCashGoodsGroupInfo grp_info;
	SVector grp_list(sizeof(_CrSetV5_GroupEntry));
	struct _SalesGrpEntry { // @flat
		PPID   GrpID;
		char   GrpName[64];
		char   Code[24];
	};
	SVector sales_grp_list(sizeof(_SalesGrpEntry));
	PPObjGoods goods_obj;
	PPObjGoodsGroup ggobj;
	PrcssrAlcReport::GoodsItem agi;
	//
	// Список ассоциаций {Серия карты; Вид котировки} => Key - серия карты, Val - вид котировки
	//
	LAssocArray  scard_quot_ary, dscnt_code_ary;
	//
	// Список видов котирок, по которым предоставляются свободные скидки (не привязанные к картам)
	//
	PPIDArray rtl_quot_list;
	PPQuotArray grp_dscnt_ary;
	PPIniFile ini_file;
	PPWaitStart();
	THROW(GetNodeData(&cn_data) > 0);
	loc_id = cn_data.LocID;
	check_dig  = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
	if(cn_data.DrvVerMajor > 4 || (cn_data.DrvVerMajor == 4 && cn_data.DrvVerMinor >= 9))
		use_dscnt_code = 1;
	THROW(DistributeFile_(0, 0/*pEndFileName*/, dfactCheckDestPaths, 0, 0));
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_ADDTIMETOFILENAMES, &add_time_to_fname);
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_USENEWDSCNTCODEALG, &use_new_dscnt_code_alg);
	THROW(fp.Init(use_dscnt_code, add_time_to_fname));
	createEmptyFile(fp.GetFileName(fp.tCashUpd));
	if(!P_Dls)
		THROW_MEM(P_Dls = new DeviceLoadingStat);
	//
	// Подготовка списка групп продаж
	//
	if(r_eq_cfg.SalesGoodsGrp) {
		SString code;
		PPIDArray _grp_list;
		ggobj.P_Tbl->GetGroupTerminalList(r_eq_cfg.SalesGoodsGrp, &_grp_list, 0);
		if(_grp_list.getCount()) {
			for(uint i = 0; i < _grp_list.getCount(); i++) {
				PPGoodsPacket gds_pack;
				if(ggobj.GetPacket(_grp_list.at(i), &gds_pack, PPObjGoods::gpoSkipQuot) > 0) {
					if(gds_pack.GetGroupCode(code) > 0) {
						_SalesGrpEntry sales_grp_item;
						sales_grp_item.GrpID = gds_pack.Rec.ID;
						STRNSCPY(sales_grp_item.GrpName, gds_pack.Rec.Name);
						STRNSCPY(sales_grp_item.Code, code);
						sales_grp_list.insert(&sales_grp_item);
					}
				}
			}
			THROW(fp.InitDeffered(fp.tSalesGroup));
			THROW(fp.InitDeffered(fp.tSalesGroupItems));
		}
		sales_grp_list.sort(CMPF_LONG);
	}
	P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
	{
		PPSCardSeries ser_rec;
		PPObjSCardSeries scs_obj;
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
		scard_quot_ary.freeAll();
		PPLoadText(PPTXT_EXPSCARD, fmt_buf);
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
				AsyncCashSCardInfo info;
				PPSCardSerPacket scs_pack;
				DbfTable * p_tbl = fp.GetTable(fp.tCards);
				THROW(p_tbl);
				THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				(msg_buf = fmt_buf).CatDiv(':', 2).Cat(ser_rec.Name);
				for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
					PPWaitPercent(iter.GetCounter(), msg_buf);
					const char * p_mode = (info.Flags & AsyncCashSCardInfo::fClosed) ? "-" : "+";
					DbfRecord dbfrC(p_tbl);
					dbfrC.put(1,  p_mode);                  // Тип действия //
					dbfrC.put(2,  info.Rec.Code);           // Код дисконтной карты
					dbfrC.put(3,  info.PsnName);            // Владелец карты
					dbfrC.put(4,  ser_rec.Name);            // Наименование карты
					dbfrC.put(5,  (int)0);                  // Тип карты (0 - дисконтная)
					dbfrC.put(6,  ser_rec.ID);              // Категория карты (ID серии карт)
					dbfrC.put(7,  fdiv100i(info.Rec.PDis)); // Процент скидки
					dbfrC.put(8,  info.Rec.MaxCredit);      // Максимальный кредит по карте
					dbfrC.put(9,  info.Rec.Dt);             // Дата выпуска карты
					dbfrC.put(10, info.Rec.Expiry);         // Срок действия карты
					THROW_PP(p_tbl->appendRec(&dbfrC), PPERR_DBFWRFAULT);
					iter.SetStat();
				}
			}
		}
	}
	if(!use_new_dscnt_code_alg)
		PrepareDscntCodeBiasList(&dscnt_code_ary);
	{
		long acgif = 0;
		if(updOnly) {
			acgif |= ACGIF_UPDATEDONLY;
			if(updOnly == 2)
				acgif |= ACGIF_REDOSINCEDLS;
		}
		THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, acgif, SinceDlsID, P_Dls));
	}
	PROFILE_START
	if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
		DbfTable * p_out_tbl_group = 0;
		DbfTable * p_out_tbl_grpqtty_dscnt = 0;
		THROW(fp.InitDeffered(fp.tGoodsGrp));
		THROW(fp.InitDeffered(fp.tGoodsGrpQttyDis));
		THROW(p_out_tbl_group = fp.GetTable(fp.tGoodsGrp));
		THROW(p_out_tbl_grpqtty_dscnt = fp.GetTable(fp.tGoodsGrpQttyDis));
		THROW_MEM(p_grp_iter  = new AsyncCashGoodsGroupIterator(NodeID, 0, P_Dls));
		while(p_grp_iter->Next(&grp_info) > 0) {
			uint   level  = MIN(4, grp_info.Level);
			uint   pos;
			PPID   parent = grp_info.ParentID;
			_CrSetV5_GroupEntry grpe;
			grpe.GrpID[0] = grp_info.ID;
			STRNSCPY(grpe.GrpName, grp_info.Name);
			grpe.DivN = grp_info.DivN;
			for(i = 1; i <= level; i++) {
				grpe.GrpID[i] = parent;
				if(parent && grp_list.lsearch(&parent, &(pos = 0), CMPF_LONG))
					parent = static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(pos))->GrpID[1];
				else {
					parent = 0;
					level  = i - 1;
					break;
				}
			}
			if(parent)
				for(i = 0; i < grp_list.getCount(); i++) {
					_CrSetV5_GroupEntry * p_grpe = static_cast<_CrSetV5_GroupEntry *>(grp_list.at(i));
					for(k = 1; k <= 4; k++)
						if(p_grpe->GrpID[k] == parent && p_grpe->GrpID[k - 1] == grpe.GrpID[4]) {
							p_grpe->Level = k - 1;
							break;
						}
					for(; k <= 4; k++)
						p_grpe->GrpID[k] = 0;
				}
			grpe.Level = level;
			THROW_SL(grp_list.insert(&grpe));
			if(grp_info.P_QuotByQttyList)
				for(uint c = 0; c < grp_info.P_QuotByQttyList->getCount(); c++)
					THROW_SL(grp_dscnt_ary.insert(&(grp_info.P_QuotByQttyList->at(c))));
		}
		{
			//
			// Вставляем специальную группу для маркированного алкоголя
			//
			_CrSetV5_GroupEntry grpe;
			grpe.GrpID[0] = alco_special_grp_id;
			grpe.DivN = 0;
			grpe.Level = 0;
			STRNSCPY(grpe.GrpName, p_alco_special_grp_name);
			THROW_SL(grp_list.insert(&grpe));
		}
		for(i = 0; i < grp_list.getCount(); i++) {
			uint  pos;
			_CrSetV5_GroupEntry grpe = *static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(i));
			DbfRecord dbfrGG(p_out_tbl_group);
			dbfrGG.put(1, grpe.GrpName);
			for(k = 0; k <= grpe.Level; k++)
				dbfrGG.put(k + 2, grpe.GrpID[grpe.Level - k]);
			for(; k <= grpe.Level; k++)
				dbfrGG.put(k + 2, 0L);
			dbfrGG.put(7, (cn_data.Flags & CASHF_EXPDIVN) ? grpe.DivN : 1); // Номер секции
			THROW_PP(p_out_tbl_group->appendRec(&dbfrGG), PPERR_DBFWRFAULT);
			for(pos = 0; grp_dscnt_ary.lsearch(&grpe.GrpID[0], &pos, CMPF_LONG, offsetof(PPQuot, GoodsID)); pos++) {
				uint  p = 0;
				PPQuot quot_by_qtty = grp_dscnt_ary.at(pos);
				if(use_new_dscnt_code_alg || dscnt_code_ary.Search(quot_by_qtty.Kind, &old_dscnt_code_bias, &p)) {
					DbfRecord  dbfrGGQD(p_out_tbl_grpqtty_dscnt);
					for(k = 0; k <= grpe.Level; k++)
						dbfrGGQD.put(k + 1, grpe.GrpID[grpe.Level - k]);
					for(; k <= grpe.Level; k++)
						dbfrGGQD.put(k + 1, 0L);
					if(use_new_dscnt_code_alg)
						dbfrGGQD.put(6, GetDscntCode(grpe.GrpID[0], quot_by_qtty.Kind, 1)); // Код скидки
					else
						dbfrGGQD.put(6, (long)(0x01000000 * old_dscnt_code_bias + grpe.GrpID[0])); // Код скидки
					dbfrGGQD.put(7, 2L);                             // Код обработки кол-ва (2 - на превышение)
					dbfrGGQD.put(8, quot_by_qtty.MinQtty - 1);       // Кол-во, больше которого применяется скидка
					dbfrGGQD.put(9, -quot_by_qtty.Quot);             // Процент скидки на кол-во товара
					if(qk_obj.Fetch(quot_by_qtty.Kind, &qk_rec) <= 0)
						MEMSZERO(qk_rec);
					dbfrGGQD.put(10, GetDatetimeStrBeg(qk_rec.Period.low, qk_rec.BeginTm, dttm_str));
					dbfrGGQD.put(11, GetDatetimeStrEnd(qk_rec.Period.upp, qk_rec.EndTm, dttm_str));
					THROW_PP(p_out_tbl_grpqtty_dscnt->appendRec(&dbfrGGQD), PPERR_DBFWRFAULT);
				}
			}
		}
	}
	{
		//
		// Инициализируем список видов котировок, которые нам понадобятся от RetailGoodsExtractor для экспорта
		//
		for(i = 0; i < scard_quot_ary.getCount(); i++)
			if(scard_quot_ary.at(i).Val)
				gi.QuotList.Add(scard_quot_ary.at(i).Val, 0, 1);
		{
			//
			// Формируем список розничных котировок, не входящих в список scard_quot_ary
			// и имеющих ограничение по дню недели или времени. Это список будет
			// использоваться для передачи в кассовый модуль соответствующих скидок.
			//
			PPIDArray temp_list;
			TimeRange tr;
			qk_obj.GetRetailQuotList(ZERODATETIME, &temp_list, 0);
			for(uint i = 0; i < temp_list.getCount(); i++) {
				const  PPID qk_id = temp_list.get(i);
				if(!gi.QuotList.Search(qk_id, 0, 0) && qk_obj.Fetch(qk_id, &qk_rec) > 0) {
					if(qk_rec.GetTimeRange(tr) || qk_rec.HasWeekDayRestriction()) {
						rtl_quot_list.addUnique(qk_id);
						// gi.QuotList.Add(qk_id, 0, 1);
					}
				}
			}
			if(rtl_quot_list.getCount()) {
				THROW(fp.InitDeffered(fp.tGoodsDis));
			}
		}
	}
	while(p_gds_iter->Next(&gi) > 0) {
		char   tempbuf[128];
	   	if(gi.ID != prev_goods_id) {
			const  int is_spirit    = BIN(p_gds_iter->GetAlcoGoodsExtension(gi.ID, 0, agi) > 0);
			DbfTable * p_out_tbl_goods = 0;
			THROW(p_out_tbl_goods = fp.GetTable(fp.tGoods));
			if(CConfig.Flags & CCFLG_DEBUG)
				LogExportingGoodsItem(&gi);
			DbfRecord dbfrG(p_out_tbl_goods);
			dbfrG.put(1,  ltoa(gi.ID, tempbuf, 10));
			dbfrG.put(2,  gi.Name);
		   	unit_obj.Fetch(gi.UnitID, &unit_rec);
			dbfrG.put(3,  unit_rec.Name);
			dbfrG.put(4,  (int)1);            // Разрешение к продаже
			if(is_spirit && agi.StatusFlags & agi.stMarkWanted) {
				dbfrG.put(5+0, alco_special_grp_id);
				for(k = 5+1; k < 10; k++) {
					dbfrG.put(k, (int)0);
				}
			}
			else {
				// Группа товаров 1-5 {
				if(cn_data.Flags & CASHF_EXPGOODSGROUPS && gi.ParentID && grp_list.lsearch(&gi.ParentID, &(i = 0), CMPF_LONG)) {
					_CrSetV5_GroupEntry grpe = *static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(i));
					for(k = 0; k <= grpe.Level; k++)
						dbfrG.put(5 + k, grpe.GrpID[grpe.Level - k]);
					for(; k <= grpe.Level; k++)
						dbfrG.put(5 + k, (int)0);
				}
				else {
					for(k = 5; k < 10; k++)
						dbfrG.put(k, (int)0);
				}
				// } Группа товаров 1-5
			}
			dbfrG.put(10, gi.Price);	                                  // Цена товара
			// @v6.7.8 dbfrG.put(11, fpow10i(-3));                        // Мерность товара
			dbfrG.put(11, gi.Precision);                                  // Мерность товара
			dbfrG.put(12, (cn_data.Flags & CASHF_EXPDIVN) ? gi.DivN : 1); // Номер секции
			dbfrG.put(13,  gi.ID);                                        // ID ограничения на скидку
			dbfrG.put(14, (double)((gi.NoDis > 0) ? 100 : 0));            // Min цена товара
				// (100% - скидки запрещены, 0% - любые скидки разрешены)
			dbfrG.put(15, gi.VatRate);
			dbfrG.put(16, gi.Rest);
			THROW_PP(p_out_tbl_goods->appendRec(&dbfrG), PPERR_DBFWRFAULT);
			//
			// Загрузка принадлежности группам продаж
			//
			if(r_eq_cfg.SalesGoodsGrp != 0) {
				DbfTable * p_tbl = 0;
				THROW(p_tbl = fp.GetTable(fp.tSalesGroupItems));
				{
					DbfRecord dbfrSGI(p_tbl);
					uint   sg_pos = 0;
					PPID   sub_grp_id = 0;
					if(ggobj.BelongToGroup(gi.ID, r_eq_cfg.SalesGoodsGrp, &sub_grp_id) > 0 && sub_grp_id && sales_grp_list.bsearch(&sub_grp_id, &sg_pos, CMPF_LONG)) {
						const _SalesGrpEntry * p_sentry = static_cast<const _SalesGrpEntry *>(sales_grp_list.at(sg_pos));
						dbfrSGI.put(1, ltoa(gi.ID, tempbuf, 10));
						dbfrSGI.put(2, p_sentry->Code);
						THROW_PP(p_tbl->appendRec(&dbfrSGI), PPERR_DBFWRFAULT);
					}
				}
			}
			{
				//
				// Загрузка скидок по котировкам, привязанным к дисконтным картам
				//
				DbfTable * p_tbl = fp.GetTable(fp.tDiscount);
				THROW(p_tbl);
				for(i = 0; i < scard_quot_ary.getCount(); i++) {
					int    is_there_quot = 0;
					uint   pos = 0;
					double dscnt_sum = 0.0;
					if(scard_quot_ary.at(i).Val) {
						double quot = gi.QuotList.Get(scard_quot_ary.at(i).Val);
						if(quot > 0.0) {
							dscnt_sum = gi.Price - quot;
							is_there_quot = 1;
						}
					}
					else if(gi.ExtQuot)
						dscnt_sum = gi.Price - gi.ExtQuot;
					if((is_there_quot || dscnt_sum != 0.0) && (!use_dscnt_code || use_new_dscnt_code_alg ||
						dscnt_code_ary.Search(scard_quot_ary.at(i).Val, &old_dscnt_code_bias, &pos))) {
						int   next_fld = 0;
						DbfRecord dbfrD(p_tbl);
						dbfrD.put(1, ltoa(gi.ID, tempbuf, 10));
						dbfrD.put(2, scard_quot_ary.at(i).Key); // Категория карты (ID серии карт)
						dbfrD.put(3, dscnt_sum);                // Сумма скидки
						next_fld = 4;
						if(use_dscnt_code)
							if(use_new_dscnt_code_alg)
								dbfrD.put(next_fld++, GetDscntCode(gi.ID, scard_quot_ary.at(i).Key, 0)); // Код скидки
							else
								dbfrD.put(next_fld++, (0x01000000L * old_dscnt_code_bias + gi.ID)); // Код скидки
						if(qk_obj.Fetch(scard_quot_ary.at(i).Val, &qk_rec) <= 0)
							MEMSZERO(qk_rec);
						dbfrD.put(next_fld++, GetDatetimeStrBeg(qk_rec.Period.low, qk_rec.BeginTm, dttm_str));
						dbfrD.put(next_fld++, GetDatetimeStrEnd(qk_rec.Period.upp, qk_rec.EndTm,   dttm_str));
						if(qk_rec.HasWeekDayRestriction()) {
							for(uint d = 0; d < 7; d++)
								tempbuf[d] = (qk_rec.DaysOfWeek & (1 << d)) ? '1' : '0';
						}
						else {
							for(uint d = 0; d < 7; d++)
								tempbuf[d] = '1';
						}
						tempbuf[7] = 0;
						dbfrD.put(next_fld++, tempbuf); // weekdays
						THROW_PP(p_tbl->appendRec(&dbfrD), PPERR_DBFWRFAULT);
					}
				}
			}
			if(gi.P_QuotByQttyList) {
				//
				// Загрузка скидок, призязанных к количеству
				//
				DbfTable * p_tbl = fp.GetTable(fp.tGoodsQttyDis);
				THROW(p_tbl);
				for(i = 0; i < gi.P_QuotByQttyList->getCount(); i++) {
					uint  pos = 0;
					PPQuot quot_by_qtty = gi.P_QuotByQttyList->at(i);
					if(use_new_dscnt_code_alg || dscnt_code_ary.Search(quot_by_qtty.Kind, &old_dscnt_code_bias, &pos)) {
						DbfRecord dbfrGQD(p_tbl);
						dbfrGQD.put(1, ltoa(gi.ID, tempbuf, 10)); // Ид товара
						if(use_new_dscnt_code_alg)
							dbfrGQD.put(2, GetDscntCode(gi.ID, quot_by_qtty.Kind, 1)); // Код скидки
						else
							dbfrGQD.put(2, (0x01000000L * old_dscnt_code_bias + gi.ID)); // Код скидки
						dbfrGQD.put(3, 0L);                             // Тип скидки (0 - на кол-во)
						dbfrGQD.put(4, quot_by_qtty.MinQtty);           // Кол-во, на которое применяется скидка
						dbfrGQD.put(5, -quot_by_qtty.Quot);             // Процент скидки на кол-во товара
						if(qk_obj.Fetch(quot_by_qtty.Kind, &qk_rec) <= 0)
							MEMSZERO(qk_rec);
						dbfrGQD.put(6, GetDatetimeStrBeg(qk_rec.Period.low, qk_rec.BeginTm, dttm_str));
						dbfrGQD.put(7, GetDatetimeStrEnd(qk_rec.Period.upp, qk_rec.EndTm,   dttm_str));
						THROW_PP(p_tbl->appendRec(&dbfrGQD), PPERR_DBFWRFAULT);
					}
				}
			}
			if(rtl_quot_list.getCount()) {
				//
				// Загрузка скидок, не привязанных к картам, но зависящих от дня недели либо времени
				//
				DbfTable * p_tbl = fp.GetTable(fp.tGoodsDis);
				THROW(p_tbl);
				for(i = 0; i < rtl_quot_list.getCount(); i++) {
					const  PPID qk_id = rtl_quot_list.get(i);
					QuotIdent qi(loc_id, qk_id, 0, 0);
					double quot = 0.0;
					if(goods_obj.GetQuotExt(gi.ID, qi, gi.Cost, gi.Price, &quot, 1) > 0) {
						if(quot > 0.0 && qk_obj.Fetch(qk_id, &qk_rec) > 0) {
							double dscnt_sum = gi.Price - quot;
							if(dscnt_sum != 0.0) {
								DbfRecord dbfr(p_tbl);
								dbfr.put(1, ltoa(gi.ID, tempbuf, 10));
								if(qk_rec.HasWeekDayRestriction()) {
									for(uint d = 0; d < 7; d++)
										tempbuf[d] = (qk_rec.DaysOfWeek & (1 << d)) ? '1' : '0';
								}
								else {
									for(uint d = 0; d < 7; d++)
										tempbuf[d] = '1';
								}
								tempbuf[7] = 0;
								dbfr.put(2, tempbuf); // weekdays
								dbfr.put(3, GetDscntCode(gi.ID, qk_id, 1)); // Код скидки
								dbfr.put(4, 0.0);       // Процент скидки
								dbfr.put(5, dscnt_sum); // Сумма скидки
								dbfr.put(6, GetDatetimeStrBeg(qk_rec.Period.low, qk_rec.BeginTm, dttm_str));
								dbfr.put(7, GetDatetimeStrEnd(qk_rec.Period.upp, qk_rec.EndTm,   dttm_str));
								dbfr.put(8, 4L);
								THROW_PP(p_tbl->appendRec(&dbfr), PPERR_DBFWRFAULT);
							}
						}
					}
				}
			}
		}
		if(sstrlen(gi.BarCode)) {
			DbfTable * p_tbl = fp.GetTable(fp.tBarcode);
			THROW(p_tbl);
			{
				DbfRecord dbfrB(p_tbl);
				gi.AdjustBarcode(check_dig);
				AddCheckDigToBarcode(gi.BarCode);
				dbfrB.put(1, ltoa(gi.ID, tempbuf, 10));
				dbfrB.put(2, gi.BarCode);
				dbfrB.put(3, gi.UnitPerPack);
				dbfrB.put(4, (cn_data.Flags & CASHF_EXPDIVN) ? gi.DivN : 1); // Номер секции
				THROW_PP(p_tbl->appendRec(&dbfrB), PPERR_DBFWRFAULT);
			}
		}
	   	prev_goods_id = gi.ID;
		PPWaitPercent(p_gds_iter->GetIterCounter());
	}
	PROFILE_END
	ZDELETE(p_grp_iter);
	ZDELETE(p_gds_iter);
	if(r_eq_cfg.CshrsPsnKindID) {
		DbfTable * p_tbl = fp.GetTable(fp.tCashier);
		THROW(p_tbl);
		{
			AsyncCashierInfo      cshr_info;
			AsyncCashiersIterator cshr_iter;
			char  rights[32], cr_rights[CRCSHSRV_CSHRRIGHTS_STRLEN * 2];
			memzero(rights, sizeof(rights));
			cshr_iter.Init(NodeID);
			while(cshr_iter.Next(&cshr_info) > 0) {
				DbfRecord dbfrC(p_tbl);
				dbfrC.put(1, cshr_info.IsWorked ? "+" : "-");
				dbfrC.put(2, cshr_info.TabNum);
				dbfrC.put(3, cshr_info.Name);
				dbfrC.put(4, cshr_info.Password);
				ConvertCashierRightsToCrystalRightsSet(cshr_info.Rights, cr_rights, sizeof(cr_rights));
				for(int pos = 0; pos < CRCSHSRV_CSHRRIGHTS_STRLEN; pos++) {
					if(cr_rights[pos])
						rights[pos] = (cr_rights[pos + CRCSHSRV_CSHRRIGHTS_STRLEN]) ? ':' : '.';
					else
						rights[pos] = (cr_rights[pos + CRCSHSRV_CSHRRIGHTS_STRLEN]) ? ((pos == 1) ? ':'
						/* по какой-то причине Кристалл понимает назначение этого права только так */ : '`') : 'x';
				}
				dbfrC.put(5, rights);
				THROW_PP(p_tbl->appendRec(&dbfrC), PPERR_DBFWRFAULT);
			}
		}
	}
	//
	// Выгрузка групп продаж
	//
	if(r_eq_cfg.SalesGoodsGrp != 0 && sales_grp_list.getCount()) {
		DbfTable * p_tbl = fp.GetTable(fp.tSalesGroup);
		THROW(p_tbl);
		{
			DbfRecord dbfrSGG(p_tbl);
			_SalesGrpEntry * p_sgitem = 0;
			for(uint i = 0; sales_grp_list.enumItems(&i, (void **)&p_sgitem) > 0;) {
				dbfrSGG.Z();
				dbfrSGG.put(1, p_sgitem->GrpName);
				dbfrSGG.put(2, p_sgitem->Code);
				THROW_PP(p_tbl->appendRec(&dbfrSGG), PPERR_DBFWRFAULT);
			}
		}
	}
	PPWaitStop();
	PPWaitStart();
	fp.CloseFiles();
	THROW(fp.DistributeFiles(this));
	if(StatID)
		P_Dls->FinishLoading(StatID, 1, 1);
	CATCHZOK
	PPWaitStop();
	return ok;
}

int ACS_CRCSHSRV::ExportData(int updOnly)
{
	return (ModuleVer == 10) ? ExportDataV10(updOnly) : ExportData__(updOnly);
}

int ACS_CRCSHSRV::Prev_ExportData(int updOnly)
{
	int    ok = 1;
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	DbfTable * p_out_tbl_goods = 0;
	DbfTable * p_out_tbl_group = 0;
	DbfTable * p_out_tbl_dscnt = 0;
	DbfTable * p_out_tbl_barcode = 0;
	DbfTable * p_out_tbl_cards = 0;
	DbfTable * p_out_tbl_cashiers = 0;
	DbfTable * p_out_tbl_gdsqtty_dscnt = 0;
	DbfTable * p_out_tbl_grpqtty_dscnt = 0;
	DbfTable * p_out_tbl_sggrp = 0;
	DbfTable * p_out_tbl_sggrpi = 0;
	AsyncCashGoodsIterator * p_gds_iter = 0;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	if(ModuleVer == 10)
		ok = ExportDataV10(updOnly);
	else {
		int    check_dig = 0;
		int    use_dscnt_code = 0;
		int    add_time_to_fname = 0;
		int    use_new_dscnt_code_alg = 0;
		uint   i;
		uint   k;
		PPID   prev_goods_id = 0;
		long   old_dscnt_code_bias = 0L;
		SString path_goods;
		SString path_group;
		SString path_dscnt;
		SString path_barcode;
		SString path_cards;
		SString path_cashiers;
		SString path_gdsqtty_dscnt;
		SString path_grpqtty_dscnt;
		SString path;
		SString path_salesggrp;
		SString path_salesggrpi;
		SString dttm_str;
		PPUnit    unit_rec;
		PPObjUnit unit_obj;
		PPObjQuotKind qk_obj;
		PPQuotKind    qk_rec;
		PPAsyncCashNode    cn_data;
		AsyncCashGoodsInfo gi;
		AsyncCashGoodsGroupInfo grp_info;
		SVector grp_list(sizeof(_CrSetV5_GroupEntry));
		struct _SalesGrpEntry { // @flat
			PPID   GrpID;
			char   GrpName[64];
			char   Code[24];
		};
		SVector sales_grp_list(sizeof(_SalesGrpEntry));
		PPObjGoodsGroup ggobj;
		//
		// Список ассоциаций {Серия карты; Вид котировки} => Key - серия карты, Val - вид котировки
		//
		LAssocArray  scard_quot_ary, dscnt_code_ary;
		//
		// Список видов котирок, по которым предоставляются свободные скидки (не привязанные к картам)
		//
		LAssocArray rtl_quot_ary;
		LAssocArray rtl_dscnt_code_ary;
		//
		PPQuotArray  grp_dscnt_ary;
		PPWaitStart();
		THROW(GetNodeData(&cn_data) > 0);
		check_dig  = BIN(GetGoodsCfg().Flags & GCF_BCCHKDIG);
		if(cn_data.DrvVerMajor > 4 || (cn_data.DrvVerMajor == 4 && cn_data.DrvVerMinor >= 9))
			use_dscnt_code = 1;
		THROW(DistributeFile_(0, 0/*pEndFileName*/, dfactCheckDestPaths, 0, 0));
		{
			PPIniFile ini_file;
			ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_ADDTIMETOFILENAMES, &add_time_to_fname);
			ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_CRYSTAL_USENEWDSCNTCODEALG, &use_new_dscnt_code_alg);
		}
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_GOODS_DBF,      path_goods));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_DSCNT_DBF,      path_dscnt));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_BAR_DBF,        path_barcode));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_CARDS_DBF,      path_cards));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_CSHRS_DBF,      path_cashiers));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_GDSQD_DBF,      path_gdsqtty_dscnt));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_SALESGGRP_DBF,  path_salesggrp));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_SALESGGRPI_DBF, path_salesggrpi));
		THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_CASH_UPD,       path));
		if(add_time_to_fname) {
			AddTimeToFileName(path_goods);
			AddTimeToFileName(path_dscnt);
			AddTimeToFileName(path_barcode);
			AddTimeToFileName(path_cards);
			AddTimeToFileName(path_cashiers);
			AddTimeToFileName(path_gdsqtty_dscnt);
		}
		createEmptyFile(path);
		THROW(p_out_tbl_goods    = CreateDbfTable(DBFS_CRCS_GOODS_EXPORT, path_goods, 1));
		THROW(p_out_tbl_dscnt    = CreateDbfTable(use_dscnt_code ? DBFS_CRCS_DSCNT_EXP49 : DBFS_CRCS_DSCNT_EXPORT, path_dscnt, 1));
		THROW(p_out_tbl_barcode  = CreateDbfTable(DBFS_CRCS_BAR_EXPORT,   path_barcode, 1));
		THROW(p_out_tbl_cards    = CreateDbfTable(DBFS_CRCS_CARDS_EXPORT, path_cards, 1));
		THROW(p_out_tbl_cashiers = CreateDbfTable(DBFS_CRCS_CSHRS_EXPORT, path_cashiers, 1));
		THROW(p_out_tbl_gdsqtty_dscnt = CreateDbfTable(DBFS_CRCS_GDSQTTYDSC_EXPORT, path_gdsqtty_dscnt, 1));
		if(!P_Dls)
			THROW_MEM(P_Dls = new DeviceLoadingStat);
		//
		// Подготовка списка групп продаж
		//
		if(r_eq_cfg.SalesGoodsGrp != 0) {
			SString code;
			PPIDArray _grp_list;
			ggobj.P_Tbl->GetGroupTerminalList(r_eq_cfg.SalesGoodsGrp, &_grp_list, 0);
			if(_grp_list.getCount()) {
				PPGoodsPacket gds_pack;
				for(uint i = 0; i < _grp_list.getCount(); i++) {
					if(ggobj.GetPacket(_grp_list.at(i), &gds_pack, PPObjGoods::gpoSkipQuot) > 0) {
						if(gds_pack.GetGroupCode(code) > 0) {
							_SalesGrpEntry sales_grp_item;
							sales_grp_item.GrpID = gds_pack.Rec.ID;
							STRNSCPY(sales_grp_item.GrpName, gds_pack.Rec.Name);
							STRNSCPY(sales_grp_item.Code, code);
							sales_grp_list.insert(&sales_grp_item);
						}
					}
				}
				THROW(p_out_tbl_sggrp  = CreateDbfTable(DBFS_CRCS_SALESGGRP_EXPORT,  path_salesggrp, 1));
				THROW(p_out_tbl_sggrpi = CreateDbfTable(DBFS_CRCS_SALESGGRPI_EXPORT, path_salesggrpi, 1));
			}
			sales_grp_list.sort(CMPF_LONG);
		}
		P_Dls->StartLoading(&StatID, dvctCashs, NodeID, 1);
		{
			PPSCardSeries ser_rec;
			PPObjSCardSeries scs_obj;
			AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, StatID);
			scard_quot_ary.freeAll();
			for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
				if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
					AsyncCashSCardInfo info;
					PPSCardSerPacket scs_pack;
					THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
					THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
					for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
						const char * p_mode = (info.Flags & AsyncCashSCardInfo::fClosed) ? "-" : "+";
						DbfRecord dbfrC(p_out_tbl_cards);
						dbfrC.put(1,  p_mode);                  // Тип действия //
						dbfrC.put(2,  info.Rec.Code);           // Код дисконтной карты
						dbfrC.put(3,  info.PsnName);            // Владелец карты
						dbfrC.put(4,  ser_rec.Name);            // Наименование карты
						dbfrC.put(5,  (int)0);                  // Тип карты (0 - дисконтная)
						dbfrC.put(6,  ser_rec.ID);              // Категория карты (ID серии карт)
						dbfrC.put(7,  fdiv100i(info.Rec.PDis)); // Процент скидки
						dbfrC.put(8,  info.Rec.MaxCredit);      // Максимальный кредит по карте
						dbfrC.put(9,  info.Rec.Dt);             // Дата выпуска карты
						dbfrC.put(10, info.Rec.Expiry);         // Срок действия карты
						THROW_PP(p_out_tbl_cards->appendRec(&dbfrC), PPERR_DBFWRFAULT);
						iter.SetStat();
					}
				}
			}
		}
		if(!use_new_dscnt_code_alg)
			PrepareDscntCodeBiasList(&dscnt_code_ary);
		THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, P_Dls));
		PROFILE_START
		if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
			THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_GROUP_DBF, path_group));
			THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_CS_GRPQD_DBF, path_grpqtty_dscnt));
			if(add_time_to_fname) {
				AddTimeToFileName(path_group);
				AddTimeToFileName(path_grpqtty_dscnt);
			}
			THROW(p_out_tbl_group = CreateDbfTable(DBFS_CRCS_GROUP_EXPORT, path_group, 1));
			THROW(p_out_tbl_grpqtty_dscnt = CreateDbfTable(DBFS_CRCS_GRPQTTYDSC_EXPORT, path_grpqtty_dscnt, 1));
			THROW_MEM(p_grp_iter  = new AsyncCashGoodsGroupIterator(NodeID, 0, P_Dls));
			while(p_grp_iter->Next(&grp_info) > 0) {
				uint   level  = MIN(4, grp_info.Level);
				uint   pos;
				PPID   parent = grp_info.ParentID;
				_CrSetV5_GroupEntry grpe;
				grpe.GrpID[0] = grp_info.ID;
				STRNSCPY(grpe.GrpName, grp_info.Name);
				grpe.DivN = grp_info.DivN;
				for(i = 1; i <= level; i++) {
					grpe.GrpID[i] = parent;
					if(parent && grp_list.lsearch(&parent, &(pos = 0), CMPF_LONG))
						parent = static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(pos))->GrpID[1];
					else {
						parent = 0;
						level  = i - 1;
						break;
					}
				}
				if(parent)
					for(i = 0; i < grp_list.getCount(); i++) {
						_CrSetV5_GroupEntry * p_grpe = static_cast<_CrSetV5_GroupEntry *>(grp_list.at(i));
						for(k = 1; k <= 4; k++)
							if(p_grpe->GrpID[k] == parent && p_grpe->GrpID[k - 1] == grpe.GrpID[4]) {
								p_grpe->Level = k - 1;
								break;
							}
						for(; k <= 4; k++)
							p_grpe->GrpID[k] = 0;
					}
				grpe.Level = level;
				THROW_SL(grp_list.insert(&grpe));
				if(grp_info.P_QuotByQttyList)
					for(uint c = 0; c < grp_info.P_QuotByQttyList->getCount(); c++)
						THROW_SL(grp_dscnt_ary.insert(&(grp_info.P_QuotByQttyList->at(c))));
			}
			for(i = 0; i < grp_list.getCount(); i++) {
				uint  pos;
				_CrSetV5_GroupEntry grpe = *static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(i));
				DbfRecord dbfrGG(p_out_tbl_group);
				dbfrGG.put(1, grpe.GrpName);
				for(k = 0; k <= grpe.Level; k++)
					dbfrGG.put(k + 2, grpe.GrpID[grpe.Level - k]);
				for(; k <= grpe.Level; k++)
					dbfrGG.put(k + 2, 0L);
				dbfrGG.put(7, (cn_data.Flags & CASHF_EXPDIVN) ? grpe.DivN : 1); // Номер секции
				THROW_PP(p_out_tbl_group->appendRec(&dbfrGG), PPERR_DBFWRFAULT);
				for(pos = 0; grp_dscnt_ary.lsearch(&grpe.GrpID[0], &pos, CMPF_LONG, offsetof(PPQuot, GoodsID)); pos++) {
					uint  p = 0;
					PPQuot quot_by_qtty = grp_dscnt_ary.at(pos);
					if(use_new_dscnt_code_alg || dscnt_code_ary.Search(quot_by_qtty.Kind, &old_dscnt_code_bias, &p)) {
						DbfRecord  dbfrGGQD(p_out_tbl_grpqtty_dscnt);
						for(k = 0; k <= grpe.Level; k++)
							dbfrGGQD.put(k + 1, grpe.GrpID[grpe.Level - k]);
						for(; k <= grpe.Level; k++)
							dbfrGGQD.put(k + 1, 0L);
						if(use_new_dscnt_code_alg)
							dbfrGGQD.put(6, GetDscntCode(grpe.GrpID[0], quot_by_qtty.Kind, 1)); // Код скидки
						else
							dbfrGGQD.put(6, (0x01000000L * old_dscnt_code_bias + grpe.GrpID[0])); // Код скидки
						dbfrGGQD.put(7, 2L);                             // Код обработки кол-ва (2 - на превышение)
						dbfrGGQD.put(8, quot_by_qtty.MinQtty - 1);       // Кол-во, больше которого применяется скидка
						dbfrGGQD.put(9, -quot_by_qtty.Quot);             // Процент скидки на кол-во товара
						if(qk_obj.Fetch(quot_by_qtty.Kind, &qk_rec) > 0) {
							dttm_str.Z().Cat((qk_rec.Period.low == ZERODATE) ? encodedate(1, 1, 2000) : qk_rec.Period.low, DATF_GERMANCENT);
							dttm_str.Space().Cat(encodetime(PTR8(&qk_rec.BeginTm)[0], PTR8(&qk_rec.BeginTm)[1], 0, 0), TIMF_HM);
							dbfrGGQD.put(10, dttm_str);
							dttm_str.Z().Cat((qk_rec.Period.upp == ZERODATE) ? encodedate(1, 1, 2050) : qk_rec.Period.upp, DATF_GERMANCENT);
							dttm_str.Space().Cat(encodetime(PTR8(&qk_rec.EndTm)[0], PTR8(&qk_rec.EndTm)[1], 0, 0), TIMF_HM);
							dbfrGGQD.put(11, dttm_str);
						}
						else {
							dttm_str.Z().Cat(encodedate(1, 1, 2000), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
							dbfrGGQD.put(10, dttm_str);
							dttm_str.Z().Cat(encodedate(1, 1, 2050), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
							dbfrGGQD.put(11, dttm_str);
						}
						THROW_PP(p_out_tbl_grpqtty_dscnt->appendRec(&dbfrGGQD), PPERR_DBFWRFAULT);
					}
				}
			}
		}
		//
		// Инициализируем список видов котировок, которые нам понадобятся от RetailGoodsExtractor для экспорта
		//
		for(i = 0; i < scard_quot_ary.getCount(); i++)
			if(scard_quot_ary.at(i).Val)
				gi.QuotList.Add(scard_quot_ary.at(i).Val, 0, 1);
		while(p_gds_iter->Next(&gi) > 0) {
			char   tempbuf[128];
	   		if(gi.ID != prev_goods_id) {
				if(CConfig.Flags & CCFLG_DEBUG)
					LogExportingGoodsItem(&gi);
				DbfRecord dbfrG(p_out_tbl_goods);
				dbfrG.put(1,  ltoa(gi.ID, tempbuf, 10));
				dbfrG.put(2,  gi.Name);
		   		unit_obj.Fetch(gi.UnitID, &unit_rec);
				dbfrG.put(3,  unit_rec.Name);
				dbfrG.put(4,  (int)1);            // Разрешение к продаже
				// Группа товаров 1-5 {
				if(cn_data.Flags & CASHF_EXPGOODSGROUPS && gi.ParentID && grp_list.lsearch(&gi.ParentID, &(i = 0), CMPF_LONG)) {
					_CrSetV5_GroupEntry grpe = *static_cast<const _CrSetV5_GroupEntry *>(grp_list.at(i));
					for(k = 0; k <= grpe.Level; k++)
						dbfrG.put(5 + k, grpe.GrpID[grpe.Level - k]);
					for(; k <= grpe.Level; k++)
						dbfrG.put(5 + k, static_cast<int>(0));
				}
				else {
					for(k = 5; k < 10; k++)
						dbfrG.put(k, static_cast<int>(0));
				}
				// } Группа товаров 1-5
				dbfrG.put(10, gi.Price);	  // Цена товара
				dbfrG.put(11, gi.Precision);  // Мерность товара
				dbfrG.put(12, (cn_data.Flags & CASHF_EXPDIVN) ? gi.DivN : 1); // Номер секции
				dbfrG.put(13,  gi.ID);       // ID ограничения на скидку
				dbfrG.put(14, (gi.NoDis > 0) ? 100.0 : 0.0); // Min цена товара
				// (100% - скидки запрещены, 0% - любые скидки разрешены)
				dbfrG.put(15, gi.VatRate);
				dbfrG.put(16, gi.Rest);
				THROW_PP(p_out_tbl_goods->appendRec(&dbfrG), PPERR_DBFWRFAULT);
				//
				// Загрузка принадлежности группам продаж
				//
				if(r_eq_cfg.SalesGoodsGrp != 0) {
					uint   sg_pos = 0;
					PPID   sub_grp_id = 0;
					if(ggobj.BelongToGroup(gi.ID, r_eq_cfg.SalesGoodsGrp, &sub_grp_id) > 0 && sub_grp_id && sales_grp_list.bsearch(&sub_grp_id, &sg_pos, CMPF_LONG)) {
						const _SalesGrpEntry * p_sentry = static_cast<const _SalesGrpEntry *>(sales_grp_list.at(sg_pos));
						DbfRecord dbfrSGI(p_out_tbl_sggrpi);
						dbfrSGI.put(1, ltoa(gi.ID, tempbuf, 10));
						dbfrSGI.put(2, p_sentry->Code);
						THROW_PP(p_out_tbl_sggrpi->appendRec(&dbfrSGI), PPERR_DBFWRFAULT);
					}
				}
				//
				// Загрузка скидок по котировкам
				//
				for(i = 0; i < scard_quot_ary.getCount(); i++) {
					int    is_there_quot = 0;
					uint   pos = 0;
					double dscnt_sum = 0.0;
					if(scard_quot_ary.at(i).Val) {
						double quot = gi.QuotList.Get(scard_quot_ary.at(i).Val);
						if(quot > 0.0) {
							dscnt_sum = gi.Price - quot;
							// @v5.3.12 Следующая строка комментируется из-за того, что она препятствует
							// загрузке цен по картам, которые равны базовой цене.
							// if(use_dscnt_code == 0)
								is_there_quot = 1;
						}
					}
					else if(gi.ExtQuot)
						dscnt_sum = gi.Price - gi.ExtQuot;
					if((is_there_quot || dscnt_sum != 0.0) && (!use_dscnt_code || use_new_dscnt_code_alg ||
						dscnt_code_ary.Search(scard_quot_ary.at(i).Val, &old_dscnt_code_bias, &pos))) {
						int   next_fld = 0;
						DbfRecord dbfrD(p_out_tbl_dscnt);
						dbfrD.put(1, ltoa(gi.ID, tempbuf, 10));
						dbfrD.put(2, scard_quot_ary.at(i).Key); // Категория карты (ID серии карт)
						dbfrD.put(3, dscnt_sum);        // Сумма скидки
						next_fld = 4;
						if(use_dscnt_code)
							if(use_new_dscnt_code_alg)
								dbfrD.put(next_fld++, GetDscntCode(gi.ID, scard_quot_ary.at(i).Key, 0)); // Код скидки
							else
								dbfrD.put(next_fld++, (0x01000000L * old_dscnt_code_bias + gi.ID)); // Код скидки
						if(qk_obj.Fetch(scard_quot_ary.at(i).Val, &qk_rec) > 0) {
							dttm_str.Z().Cat((qk_rec.Period.low == ZERODATE) ? encodedate(1, 1, 2000) : qk_rec.Period.low, DATF_GERMAN | DATF_CENTURY);
							dttm_str.Space().Cat(encodetime(*(char *)&qk_rec.BeginTm,*(((char *)&qk_rec.BeginTm) + 1), 0, 0), TIMF_HM);
							dbfrD.put(next_fld++, dttm_str);
							dttm_str.Z().Cat((qk_rec.Period.upp == ZERODATE) ? encodedate(1, 1, 2050) : qk_rec.Period.upp, DATF_GERMAN | DATF_CENTURY);
							dttm_str.Space().Cat(encodetime(*(char *)&qk_rec.EndTm,*(((char *)&qk_rec.EndTm) + 1), 0, 0), TIMF_HM);
							dbfrD.put(next_fld++, dttm_str);
						}
						else {
							dttm_str.Z().Cat(encodedate(1, 1, 2000), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
							dbfrD.put(next_fld++, encodedate(1, 1, 2000));
							dttm_str.Z().Cat(encodedate(1, 1, 2050), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
							dbfrD.put(next_fld, encodedate(1, 1, 2050));
						}
						THROW_PP(p_out_tbl_dscnt->appendRec(&dbfrD), PPERR_DBFWRFAULT);
					}
				}
				if(gi.P_QuotByQttyList)
					for(i = 0; i < gi.P_QuotByQttyList->getCount(); i++) {
						uint  pos = 0;
						PPQuot quot_by_qtty = gi.P_QuotByQttyList->at(i);
						if(use_new_dscnt_code_alg || dscnt_code_ary.Search(quot_by_qtty.Kind, &old_dscnt_code_bias, &pos)) {
							DbfRecord dbfrGQD(p_out_tbl_gdsqtty_dscnt);
							dbfrGQD.put(1, ltoa(gi.ID, tempbuf, 10)); // Ид товара
							if(use_new_dscnt_code_alg)
								dbfrGQD.put(2, GetDscntCode(gi.ID, quot_by_qtty.Kind, 1)); // Код скидки
							else
								dbfrGQD.put(2, (0x01000000L * old_dscnt_code_bias + gi.ID)); // Код скидки
							dbfrGQD.put(3, 0L);                             // Тип скидки (0 - на кол-во)
							dbfrGQD.put(4, quot_by_qtty.MinQtty);           // Кол-во, на которое применяется скидка
							dbfrGQD.put(5, -quot_by_qtty.Quot);             // Процент скидки на кол-во товара
							if(qk_obj.Fetch(quot_by_qtty.Kind, &qk_rec) > 0) {
								dttm_str.Z().Cat((qk_rec.Period.low == ZERODATE) ? encodedate(1, 1, 2000) : qk_rec.Period.low, DATF_GERMAN | DATF_CENTURY);
								dttm_str.Space().Cat(encodetime(*(char *)&qk_rec.BeginTm,*(((char *)&qk_rec.BeginTm) + 1), 0, 0), TIMF_HM);
								dbfrGQD.put(6, dttm_str);
								dttm_str.Z().Cat((qk_rec.Period.upp == ZERODATE) ? encodedate(1, 1, 2050) : qk_rec.Period.upp, DATF_GERMAN | DATF_CENTURY);
								dttm_str.Space().Cat(encodetime(*(char *)&qk_rec.EndTm,*(((char *)&qk_rec.EndTm) + 1), 0, 0), TIMF_HM);
								dbfrGQD.put(7, dttm_str);
							}
							else {
								dttm_str.Z().Cat(encodedate(1, 1, 2000), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
								dbfrGQD.put(6, dttm_str);
								dttm_str.Z().Cat(encodedate(1, 1, 2050), DATF_GERMANCENT).Space().Cat(ZEROTIME, TIMF_HM);
								dbfrGQD.put(7, dttm_str);
							}
							THROW_PP(p_out_tbl_gdsqtty_dscnt->appendRec(&dbfrGQD), PPERR_DBFWRFAULT);
						}
					}
			}
			if(sstrlen(gi.BarCode)) {
				DbfRecord dbfrB(p_out_tbl_barcode);
				gi.AdjustBarcode(check_dig);
				AddCheckDigToBarcode(gi.BarCode);
				dbfrB.put(1, ltoa(gi.ID, tempbuf, 10));
				dbfrB.put(2, gi.BarCode);
				dbfrB.put(3, gi.UnitPerPack);
				dbfrB.put(4, (cn_data.Flags & CASHF_EXPDIVN) ? gi.DivN : 1); // @v6.7.10 Номер секции
				THROW_PP(p_out_tbl_barcode->appendRec(&dbfrB), PPERR_DBFWRFAULT);
			}
	   		prev_goods_id = gi.ID;
			PPWaitPercent(p_gds_iter->GetIterCounter());
		}
		PROFILE_END
		ZDELETE(p_grp_iter);
		ZDELETE(p_gds_iter);
		if(r_eq_cfg.CshrsPsnKindID) {
			AsyncCashierInfo      cshr_info;
			AsyncCashiersIterator cshr_iter;
			char  rights[32], cr_rights[CRCSHSRV_CSHRRIGHTS_STRLEN * 2];
			memzero(rights, sizeof(rights));
			cshr_iter.Init(NodeID);
			while(cshr_iter.Next(&cshr_info) > 0) {
				DbfRecord dbfrC(p_out_tbl_cashiers);
				dbfrC.put(1, cshr_info.IsWorked ? "+" : "-");
				dbfrC.put(2, cshr_info.TabNum);
				dbfrC.put(3, cshr_info.Name);
				dbfrC.put(4, cshr_info.Password);
				ConvertCashierRightsToCrystalRightsSet(cshr_info.Rights, cr_rights, sizeof(cr_rights));
				for(int pos = 0; pos < CRCSHSRV_CSHRRIGHTS_STRLEN; pos++)
					if(cr_rights[pos])
						rights[pos] = (cr_rights[pos + CRCSHSRV_CSHRRIGHTS_STRLEN]) ? ':' : '.';
					else
						rights[pos] = (cr_rights[pos + CRCSHSRV_CSHRRIGHTS_STRLEN]) ? ((pos == 1) ? ':'
						/* по какой-то причине Кристалл понимает назначение этого права только так */ : '`') : 'x';
				dbfrC.put(5, rights);
				THROW_PP(p_out_tbl_cashiers->appendRec(&dbfrC), PPERR_DBFWRFAULT);
			}
		}
		//
		// Выгрузка групп продаж
		//
		if(r_eq_cfg.SalesGoodsGrp) {
			if(sales_grp_list.getCount()) {
				_SalesGrpEntry * p_sgitem = 0;
				DbfRecord dbfrSGG(p_out_tbl_sggrp);
				for(uint i = 0; sales_grp_list.enumItems(&i, (void **)&p_sgitem) > 0;) {
					dbfrSGG.Z();
					dbfrSGG.put(1, p_sgitem->GrpName);
					dbfrSGG.put(2, p_sgitem->Code);
					THROW_PP(p_out_tbl_sggrp->appendRec(&dbfrSGG), PPERR_DBFWRFAULT);
				}
			}
		}
		PPWaitStop();
		PPWaitStart();
		ZDELETE(p_out_tbl_barcode);
		ZDELETE(p_out_tbl_goods);
		ZDELETE(p_out_tbl_group);
		ZDELETE(p_out_tbl_dscnt);
		ZDELETE(p_out_tbl_cards);
		ZDELETE(p_out_tbl_cashiers);
		ZDELETE(p_out_tbl_gdsqtty_dscnt);
		ZDELETE(p_out_tbl_grpqtty_dscnt);
		ZDELETE(p_out_tbl_sggrp);
		ZDELETE(p_out_tbl_sggrpi);

		THROW(DistributeFile_(path, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		if(cn_data.Flags & CASHF_EXPGOODSGROUPS) {
			THROW(DistributeFile_(path_group, 0/*pEndFileName*/, dfactCopy, 0, 0));
			SDelay(2000);
			THROW(DistributeFile_(path_grpqtty_dscnt, 0/*pEndFileName*/, dfactCopy, 0, 0));
			SDelay(2000);
		}
		THROW(DistributeFile_(path_goods, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_dscnt, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_barcode, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_cards, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_cashiers, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_gdsqtty_dscnt, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_salesggrp, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path_salesggrpi, 0/*pEndFileName*/, dfactCopy, 0, 0));
		SDelay(2000);
		THROW(DistributeFile_(path, 0/*pEndFileName*/, dfactDelete, 0, 0));
		if(StatID)
			P_Dls->FinishLoading(StatID, 1, 1);
	}
	CATCHZOK
	PPWaitStop();
	delete p_gds_iter;
	delete p_grp_iter;
	delete p_out_tbl_barcode;
	delete p_out_tbl_goods;
	delete p_out_tbl_group;
	delete p_out_tbl_dscnt;
	delete p_out_tbl_cards;
	delete p_out_tbl_cashiers;
	delete p_out_tbl_gdsqtty_dscnt;
	delete p_out_tbl_grpqtty_dscnt;
	delete p_out_tbl_sggrp;
	delete p_out_tbl_sggrpi;
	return ok;
}

int ACS_CRCSHSRV::PrepareImpFileName(int filTyp, int subStrId, const char * pPath, int sigNum)
{
	int    ok = 1;
	SString sig_num_file;
	THROW(PPGetFileName(subStrId, PathRpt[filTyp]));
	SFsPath::ReplacePath(PathRpt[filTyp], pPath, 1);
	if(sigNum == 18 && ModuleVer == 5 && ModuleSubVer >= 9)
		sig_num_file.Cat("all").DotCat("dbf");
	else
		sig_num_file.Cat(sigNum).DotCat("txt");
	(PathQue[filTyp] = pPath).SetLastSlash().Cat(sig_num_file);
	CATCHZOK
	return ok;
}

/*int ACS_CRCSHSRV::PrepareImpFileName(int filTyp, const char * pName, const char * pPath, int sigNum)
{
	SString sig_num_file;
	PathRpt[filTyp] = pName;
	// } @v8.9.11 {
	SFsPath::ReplacePath(PathRpt[filTyp], pPath, 1);
	sig_num_file.Cat(sigNum).DotCat("txt");
	(PathQue[filTyp] = pPath).SetLastSlash().Cat(sig_num_file);
	// } @v8.9.11
	// @v8.9.11 strcat(setLastSlash(STRNSCPY(PathQue[filTyp], pPath)), replaceExt(itoa(sigNum, sig_num_file, 10), "txt", 1));
	return 1;
}*/

int ACS_CRCSHSRV::PrepareImpFileNameV10(int filTyp, const char * pName, const char * pPath)
{
	PathRpt[filTyp] = pName;
	SFsPath::ReplacePath(PathRpt[filTyp], pPath, 1);
	(PathQue[filTyp] = pPath).SetLastSlash().Cat("reports.request");
	return 1;
}

/* @v9.7.4 static void RemoveQuotations(char * pBuf)
{
	if(pBuf && pBuf[0] == '"' && pBuf[sstrlen(pBuf) - 1] == '"') {
		strcpy(pBuf, pBuf + 1);
		pBuf[sstrlen(pBuf) - 1] = 0;
		strip(pBuf);
	}
} */

static void ConvertCrystalRightsSetToCashierRights(long crystCshrRights, long * pCshrRights)
{
	int    i;
	const  char correspondance[] = {3,18,4,5,6,20,12,22,8,9,30,1,10,31,7,23,19,24,25,26,27,17,28,21,29,0,2,0,11,0,0,0}; // 32 items
	char   rights_buf[SIZEOFARRAY(correspondance)];
	SBitArray  cryst_righst_ary, rights_ary;
	cryst_righst_ary.Init(&crystCshrRights, 32);
	memzero(rights_buf, sizeof(rights_buf));
	for(i = 0; i < SIZEOFARRAY(correspondance); i++)
		if(cryst_righst_ary.get(i) && correspondance[i])
			rights_buf[correspondance[i]-1] = 1;
	for(i = 0; i < SIZEOFARRAY(rights_buf); i++)
		rights_ary.insert(rights_buf[i]);
	rights_ary.getBuf(pCshrRights, sizeof(long));
}

int ACS_CRCSHSRV::GetCashiersList()
{
	int    ok = -1;
	const PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	if(r_eq_cfg.CshrsPsnKindID) {
		uint   pos, p;
		SString buf;
		SString cshr_name_;
		SString cshr_tabnum_;
		SString cshr_password_;
		PPIDArray     psn_ary;
		PPObjPerson   psn_obj;
		PPObjRegister reg_obj;
		RegisterTbl::Rec  reg_rec;
		const  PPID tabnum_reg_id = r_eq_cfg.GetCashierTabNumberRegTypeID();
		if(tabnum_reg_id && ::fileExists(PathCshrs)) {
			PPIDArray by_name_ary;
			PPIDArray by_num_ary;
			RegisterArray reg_ary;
			StringSet ss(",");
			RegisterFilt reg_flt;
			LDATE  last_dt = plusdate(getcurdate_(), -1);
			reg_flt.Oid.Obj = PPOBJ_PERSON; // @v10.0.1
			reg_flt.RegTypeID = tabnum_reg_id;
			psn_obj.GetListByKind(r_eq_cfg.CshrsPsnKindID, &psn_ary, 0);
			SFile  cf(PathCshrs, SFile::mRead);
			THROW_SL(cf.IsValid());
			while(cf.ReadLine(buf, SFile::rlfChomp)) {
				int    is_kind = 0, is_reg = 0;
				uint   i = 0;
				long   rights = 0;
				char   cshr_rights[20];
				PPID   psn_id = 0;
				PPPersonPacket psn_pack;
				ss.Z();
				by_name_ary.clear();
				by_num_ary.clear();
				ss.add(buf);
				ss.get(&i, cshr_tabnum_); // Табельный номер
				cshr_tabnum_.StripQuotes();
				ss.get(&i, cshr_name_);                  // Имя кассира
				cshr_name_.Transf(CTRANSF_INNER_TO_OUTER).StripQuotes();
				ss.get(&i, cshr_password_);   // Пароль
				cshr_password_.StripQuotes();
				ss.get(&i, cshr_rights, sizeof(cshr_rights)); // Права
				strtolong(cshr_rights, &rights);
				PPObjPerson::SrchAnalogPattern sap(cshr_name_, PPObjPerson::sapfMatchWholeWord);
				psn_obj.GetListByPattern(&sap, &by_name_ary);
				if(by_name_ary.getCount())
					psn_id = by_name_ary.at(0);
				by_name_ary.intersect(&psn_ary, 0);
				if(by_name_ary.getCount()) {
					psn_id = by_name_ary.at(0);
					for(p = 0; p < by_name_ary.getCount(); p++)
						psn_ary.freeByKey(by_name_ary.at(p), 0);
					is_kind = 1;
				}
				reg_flt.NmbPattern = cshr_tabnum_;
				reg_obj.SearchByFilt(&reg_flt, 0, &by_num_ary);
				by_name_ary.intersect(&by_num_ary, 0);
				if(by_name_ary.getCount())
					psn_id = by_name_ary.at(0);
				for(pos = 0; pos < by_num_ary.getCount(); pos++) {
					PPID  psn_w_reg = by_num_ary.at(pos);
					if(psn_w_reg != psn_id) {
						reg_ary.clear();
						reg_obj.P_Tbl->GetByPerson(psn_w_reg, &reg_ary);
						for(p = 0; reg_ary.GetRegister(tabnum_reg_id, &p, &reg_rec) > 0;)
							if(reg_rec.Expiry == ZERODATE || diffdate(reg_rec.Expiry, last_dt) > 0) {
								reg_rec.Expiry = last_dt;
								THROW(reg_obj.P_Tbl->SetByPerson(psn_w_reg, 0, &reg_rec, 1));
							}
					}
				}
				if(psn_id) {
					THROW(psn_obj.GetPacket(psn_id, &psn_pack, 0) > 0);
					if(!is_kind)
						THROW_SL(psn_pack.Kinds.add(r_eq_cfg.CshrsPsnKindID));
					for(p = 0; psn_pack.Regs.GetRegister(tabnum_reg_id, &p, &reg_rec) > 0;)
						if(!sstreq(reg_rec.Num, cshr_tabnum_)) {
							if(reg_rec.Expiry == ZERODATE || diffdate(reg_rec.Expiry, last_dt) > 0)
								psn_pack.Regs.at(p-1).Expiry = last_dt;
						}
						else if(reg_rec.Expiry == ZERODATE || diffdate(reg_rec.Expiry, last_dt) > 0)
							is_reg = 1;
				}
				else {
					psn_pack.Rec.Status = PPPRS_PRIVATE;
					STRNSCPY(psn_pack.Rec.Name, cshr_name_);
					THROW_SL(psn_pack.Kinds.add(r_eq_cfg.CshrsPsnKindID));
				}
				if(!is_reg) {
					THROW(PPObjRegister::InitPacket(&reg_rec, tabnum_reg_id, PPObjID(PPOBJ_PERSON, psn_id), cshr_tabnum_));
					THROW_SL(psn_pack.Regs.insert(&reg_rec));
				}
				ConvertCrystalRightsSetToCashierRights(rights, &psn_pack.CshrInfo.Rights);
				STRNSCPY(psn_pack.CshrInfo.Password, cshr_password_);
				psn_pack.CshrInfo.Flags |= (CIF_CASHIER | CIF_MODIFIED);
				THROW(psn_obj.PutPacket(&psn_id, &psn_pack, 1));
			}
			SFile::Remove(PathCshrs);
			psn_ary.clear();
		}
		psn_obj.GetListByKind(r_eq_cfg.CshrsPsnKindID, &psn_ary, 0);
		for(pos = 0; pos < psn_ary.getCount(); pos++) {
			PPPersonPacket psn_pack;
			if(psn_obj.GetPacket(psn_ary.at(pos), &psn_pack, 0) > 0 && (psn_pack.CshrInfo.Flags & CIF_CASHIER)) {
				CashierEntry cshr_entry;
				cshr_entry.PsnID = psn_pack.Rec.ID;
				if(tabnum_reg_id) {
					for(p = 0; psn_pack.Regs.GetRegister(tabnum_reg_id, &p, &reg_rec) > 0;) {
						long   tab_num;
						strtolong(reg_rec.Num, &tab_num);
						cshr_entry.TabNum = tab_num;
						cshr_entry.Expiry = reg_rec.Expiry;
						THROW_SL(CshrList.Add(&cshr_entry));
					}
				}
				else {
					cshr_entry.TabNum = psn_pack.Rec.ID;
					THROW_SL(CshrList.Add(&cshr_entry));
				}
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

PPBillImpExpParam * ACS_CRCSHSRV::CreateImpExpParam(uint sdRecID)
{
	int   ok = -1;
	StringSet  sections;
	PPBillImpExpParam * p_param = 0;
	THROW_MEM(p_param = new PPBillImpExpParam);
	p_param->Init();
	THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, sdRecID, p_param, &sections, 0));
	if(sections.getCount()) {
		SString  ini_file_name, section;
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		{
			uint  p = 0;
			PPIniFile ini_file(ini_file_name, 0, 1, 1);
			sections.get(&p, section);
			p_param->OtrRec.Clear();
			if(p_param->ReadIni(&ini_file, section, 0)) {
				p_param->Direction = 1;
				SFsPath  sps(p_param->FileName);
				SFsPath  def_sps(PathRpt[sdRecID - PPREC_CS_ZREP]);
				if(!(sps.Flags & SFsPath::fDrv))
					sps.Drv = def_sps.Drv;
				if(!(sps.Flags & SFsPath::fDir)) {
					SString  path;
					if(!(def_sps.Flags & SFsPath::fDir))
						PPGetPath(PPPATH_OUT, path);
					else
						path = def_sps.Dir;
					sps.Dir = path;
				}
				if(sps.Flags & SFsPath::fNam)
					sps.Merge(p_param->FileName);
				else {
					sps.Nam = def_sps.Nam;
					sps.Merge(p_param->FileName);
					SFsPath::ReplaceExt(p_param->FileName, (p_param->DataFormat == PPImpExpParam::dfDbf) ? "dbf" :
						(p_param->DataFormat == PPImpExpParam::dfText) ? "txt" : "", 1);
				}
				ok = 1;
			}
		}
	}
	CATCHZOK
	if(ok <= 0)
		ZDELETE(p_param);
	return p_param;
}

int ACS_CRCSHSRV::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	if(GetFilesLocal())
		ok = 1;
	else if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtrErr(&dlg)) {
			SString dt_buf;
			const LDATE oper_date = getcurdate_(); // @v10.8.10 LConfig.OperDate-->getcurdate_()
			ChkRepPeriod.SetDate(oper_date);
			dlg->SetupCalPeriod(CTLCAL_DATERNG_PERIOD, CTL_DATERNG_PERIOD);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, &ChkRepPeriod);
			PPWaitStop();
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && strtoperiod(dt_buf, &ChkRepPeriod, 0) && !ChkRepPeriod.IsZero()) {
					SETIFZ(ChkRepPeriod.upp, plusdate(oper_date, 2));
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWaitStart();
		}
	}
	else {
		ChkRepPeriod = *pPrd;
		ok = 1;
	}
	if(ok > 0) {
		PPAsyncCashNode acn;
		THROW(GetNodeData(&acn) > 0);
		acn.GetLogNumList(LogNumList);
		THROW_PP(acn.ImpFiles.NotEmpty(), PPERR_INVFILESET);
		THROW(PrepareImpFileName(filTypZRep,     PPFILNAM_CS_ZREP_DBF,     acn.ImpFiles, 18));
		THROW(PrepareImpFileName(filTypChkHeads, PPFILNAM_CS_CHKHEADS_DBF, acn.ImpFiles, 6));
		THROW(PrepareImpFileName(filTypChkRows,  PPFILNAM_CS_CHKROWS_DBF,  acn.ImpFiles, 7));
		THROW(PrepareImpFileName(filTypChkDscnt, PPFILNAM_CS_CHKDSCNT_DBF, acn.ImpFiles, 9));
/*#ifndef NDEBUG
		THROW(PrepareImpFileNameV10(filTypChkXml,   "chks.xml", acn.ImpFiles));
		THROW(PrepareImpFileNameV10(filTypZRepXml,  "zrep.xml",  acn.ImpFiles));
#else*/
		THROW(PrepareImpFileNameV10(filTypChkXml,   "purchases.xml", acn.ImpFiles));
		THROW(PrepareImpFileNameV10(filTypZRepXml,  "zreports.xml",  acn.ImpFiles));
//#endif
		THROW(PPGetFileName(PPFILNAM_CS_WAIT, PathFlag));
		SFsPath::ReplacePath(PathFlag, acn.ImpFiles, 1);
		THROW(PPGetFileName(PPFILNAM_CASHIERS_TXT, PathCshrs));
		SFsPath::ReplacePath(PathCshrs, acn.ImpFiles, 1);
		THROW(GetCashiersList());
		if(Options & oUseAltImport) {
			THROW(PPGetFileName(PPFILNAM_CS_EXPORT_CFG, PathSetRExpCfg));
			SFsPath::ReplacePath(PathSetRExpCfg, acn.ImpFiles, 1);
			for(uint i = PPREC_CS_ZREP; i <= PPREC_CS_DSCNT; i++)
				THROW_PP(P_IEParam[i - PPREC_CS_ZREP] = CreateImpExpParam(i), PPERR_CASHSRV_IMPCHECKS);
		}
	}
	CATCHZOK
	*pSessCount = BIN(ok > 0);
	*pIsForwardSess = 0;
	delete dlg;
	return ok;
}

PP_CREATE_TEMP_FILE_PROC(CreateTempOrderTbl, TempOrder);

int ACS_CRCSHSRV::SearchCardCode(SCardCore * pSc, const char * pCode, SCardTbl::Rec * pRec)
{
	if(pSc->SearchCode(0, pCode, pRec) > 0)
		return 1;
	else {
		SString code(pCode);
		if(code.Len() == 13 && CheckCnFlag(CASHF_EXPCHECKD)) {
			code.TrimRight();
			if(pSc->SearchCode(0, code, pRec) > 0)
				return 1;
		}
	}
	return 0;
}

static void FASTCALL ReplaceFilePath(SString & destFileName, const SString & srcFileName)
{
	SString file_path;
	SString slash("\\/");
	(file_path = srcFileName).TrimToDiv(srcFileName.Len() - 1, slash);
	SFsPath::ReplacePath(destFileName, file_path, 1);
}

int ACS_CRCSHSRV::CreateSCardPaymTbl()
{
	int    ok = 1;
	SCardCore     sc_core;
	SCardTbl::Rec sc_rec;
	TempOrderTbl::Key0 k0;
	PPImpExp * p_ie_csd = 0;
	DbfTable * p_dbftd  = 0;
	THROW_MEM(P_SCardPaymTbl = CreateTempOrderTbl());
	if(Options & oUseAltImport) {
		SString  file_name, save_file_name, ser_name;
		PPObjSCardSeries scs_obj;
		for(uint pos = 0; SeparatedFileSet.get(&pos, file_name);) {
			long   c, count;
			PPID  ser_id = 0;
			PPBillImpExpParam * p_ie_param = P_IEParam[PPREC_CS_DSCNT - PPREC_CS_ZREP];
			Backup("dsct", file_name);
			p_ie_param->FileName = file_name;
			THROW_MEM(p_ie_csd = new PPImpExp(p_ie_param, 0));
			THROW(p_ie_csd->OpenFileForReading(0));
			p_ie_csd->GetNumRecs(&count);
			for(c = 0; c < count; c++) {
				Sdr_CS_Dscnt  cs_dscnt;
				THROW(p_ie_csd->ReadRecord(&cs_dscnt, sizeof(cs_dscnt)));
				if(cs_dscnt.DscntType == CRCSHSRV_DISCCARD_DEFTYPE) {
					MEMSZERO(k0);
					k0.ID = cs_dscnt.CheckLineID;
					if(cs_dscnt.CheckLineID && cs_dscnt.CardNumber[0] && !P_SCardPaymTbl->search(0, &k0, spEq)) {
						TempOrderTbl::Rec scard_rec;
						MEMSZERO(sc_rec);
						if(!SearchCardCode(&sc_core, cs_dscnt.CardNumber, &sc_rec)) {
							if(!ser_id) {
								ser_name = "IMPORT";
								PPSCardSerPacket  scs_pack;
								THROW(scs_obj.SearchByName(ser_name, &ser_id));
								if(!ser_id) {
									ser_name.CopyTo(scs_pack.Rec.Name, sizeof(scs_pack.Rec.Name));
									THROW(scs_obj.PutPacket(&ser_id, &scs_pack, 0));
								}
							}
							sc_rec.SeriesID = ser_id;
							STRNSCPY(sc_rec.Code, cs_dscnt.CardNumber);
							THROW(AddByID(&sc_core, &sc_rec.ID, &sc_rec, 0));
						}
						scard_rec.ID = cs_dscnt.CheckLineID;
						memcpy(scard_rec.Name, &sc_rec.ID, sizeof(sc_rec.ID));
						THROW_DB(P_SCardPaymTbl->insertRecBuf(&scard_rec));
					}
				}
			}
			ZDELETE(p_ie_csd);
			save_file_name = PathRpt[filTypChkDscnt];
			ReplaceFilePath(save_file_name, file_name);
			::rename(file_name, save_file_name);
		}
	}
	else {
		//
		// Коды полей входного файла скидок по чекам
		//
		int  fldn_d_chkln_id   = 0;
		int  fldn_d_dscnt_type = 0;
		int  fldn_d_cardno     = 0;

		Backup("dsct", PathRpt[filTypChkDscnt]);
		THROW_MEM(p_dbftd = new DbfTable(PathRpt[filTypChkDscnt]));
		THROW_PP(p_dbftd->isOpened(), PPERR_DBFOPFAULT);
		p_dbftd->getFieldNumber("chkln_id", &fldn_d_chkln_id);
		p_dbftd->getFieldNumber("type",     &fldn_d_dscnt_type);
		p_dbftd->getFieldNumber("cardno",   &fldn_d_cardno);
		if(p_dbftd->top()) {
			do {
				long   type, chkln_id;
				char   card_no[32];
				DbfRecord dbfrd(p_dbftd);
				if(p_dbftd->getRec(&dbfrd) <= 0)
					break;
				dbfrd.get(fldn_d_dscnt_type, type);
				if(type == CRCSHSRV_DISCCARD_DEFTYPE) {
					dbfrd.get(fldn_d_chkln_id, chkln_id);
					dbfrd.get(fldn_d_cardno,   card_no, sizeof(card_no));
					strip(card_no);
					MEMSZERO(k0);
					k0.ID = chkln_id;
					if(chkln_id && card_no[0] && !P_SCardPaymTbl->search(0, &k0, spEq)) {
						if(SearchCardCode(&sc_core, card_no, &sc_rec) > 0) {
							TempOrderTbl::Rec scard_rec;
							scard_rec.ID = chkln_id;
							memcpy(scard_rec.Name, &sc_rec.ID, sizeof(sc_rec.ID));
							THROW_DB(P_SCardPaymTbl->insertRecBuf(&scard_rec));
						}
					}
				}
			} while(p_dbftd->next());
		}
	}
	CATCH
		ZDELETE(P_SCardPaymTbl);
		ok = 0;
	ENDCATCH
	delete p_dbftd;
	delete p_ie_csd;
	return ok;
}

static int GetCrCshSrvDateTime(const char * pDttmBuf, long chk, LDATETIME * pDttm)
{
	size_t pos = 0;
	LDATE  dt;
	LTIME  tm;
	SString  dttm_buf(pDttmBuf);
	strtodate(dttm_buf.Strip(), DATF_DMY, &dt);
	//
	// Для времени 00:00:00 кассовый сервер возвращает пустую строку,
	// из-за этого такой случай приходится обрабатывать особо.
	//
	if(dttm_buf.SearchChar(' ', &pos))
		strtotime(dttm_buf.ShiftLeft(pos).Strip(), TIMF_HMS, &tm);
	else
		tm = ZEROTIME;
	if(chk > 0) {
		int h, m;
		decodetime(&h, &m, 0, 0, &tm);
		tm = encodetime(h, m, (int)(chk % 60), 0);
	}
	pDttm->d = dt;
	pDttm->t = tm;
	return 1;
}

struct ZRep { // @flat
	ZRep()
	{
		THISZERO();
	}
	long   CashCode;
	long   ZRepCode;
	LDATETIME Start;
	LDATETIME Stop;
	long   ChkFirst;
	long   ChkLast;
	long   Status;
};

static int FindFirstRec(xmlNode * pChild, xmlNode ** ppCurRec, const char * pTag)
{
	int    ok = -1;
	xmlNode * p_rec = pChild;
	if(pChild)
		p_rec = pChild;
	for(; p_rec && ok < 0; p_rec = p_rec->next) {
		if(sstreqi_ascii(reinterpret_cast<const char *>(p_rec->name), pTag)) {
			*ppCurRec = p_rec;
			ok = 1;
		}
		else
			ok = FindFirstRec(p_rec->children, ppCurRec, pTag); // @recursion
	}
	return ok;
}

//#define PAY_CASH       "CashPaymentEntity"
//#define PAY_CASH01     "ODCashPaymentEntity"

class XmlReader {
public:
	struct Header { // @falt
		Header()
		{
			THISZERO();
		}
		long   SmenaNum;
		long   CashNum;
		long   ChkNum;
		char   SCardNum[64];
		char   GiftCardNum[64];
		LDATETIME Dtm;
		int16  IsSale;
		int16  Banking;
		long   Div;
		double Amount;        // извлекается из заголовка чека, как показала практика эта сумма не всегда достоверна
		double CheckAmount;   // Сумма оплат внесенных за чек
		double AddedDiscount; // дополнительная скидка
		double BankingAmount;
		double GiftCardAmount;
		double Discount;      // извлекается из заголовка чека
		long   TabNum;
	};

	struct Item {
		long   Pos;
		char   GoodsCode[32];
		char   Barcode[32];
		char   Serial[32];
		char   Mark[256]; // @v11.3.8 Марка ЕГАИС или честный знак
		double Price;
		double PriceWithDiscount;
		double Qtty;
		double Discount;
		double Amount;
		double VatAmount;
		long   Div;
	};

	class Packet {
	public:
		Packet()
		{
		}
		Packet & FASTCALL operator = (const Packet & rPack)
		{
			Items.freeAll();
			Head = rPack.Head;
			Items.copy(rPack.Items);
			return *this;
		}
		int PutHead(const Header * pHead)
		{
			return (pHead) ? (Head = *pHead, 1) : (MEMSZERO(Head), 0);
		}
		uint   GetCount() const { return Items.getCount(); }
		Item & GetItemByIdx(uint idx) { return Items.at(idx); }
		int AddItem(const Item * pItem)
		{
			return (pItem) ? Items.insert(pItem) : 0;
		}
		int SetItemDiscount(long pos, double discount)
		{
			uint p = 0;
			return Items.lsearch(&pos, &p, CMPF_LONG) ? (Items.at(p).Discount = discount, 1) : -1;
		}
		int GetHead(Header * pHead)
		{
			ASSIGN_PTR(pHead, Head);
			return 1;
		}
		int EnumItems(long * pPos, Item * pItem)
		{
			if(pPos && *pPos < Items.getCountI()) {
				ASSIGN_PTR(pItem, Items.at((*pPos)++));
				return 1;
			}
			else
				return -1;
		}
	private:
		Header Head;
		TSArray <Item> Items;
	};
	XmlReader(const char * pPath, PPIDArray * pLogNumList, int subVer);
	~XmlReader();
	int    Next(Packet *);
private:
	int    GetGiftCard(const xmlNode * const * pPlugins, SString & rSerial, int isPaym);

	int    SubVer;
	long   ChecksCount;
	PPIDArray * P_LogNumList;
	xmlDoc  * P_Doc;
	xmlNode * P_CurRec;
	xmlTextReader * P_Reader;
};

XmlReader::XmlReader(const char * pPath, PPIDArray * pLogNumList, int subVer) : P_Reader(0), P_LogNumList(pLogNumList), ChecksCount(0), P_CurRec(0), P_Doc(0)
{
	const char * p_chr_tag = "purchase";
	if(pPath)
		P_Reader = xmlReaderForFile(pPath, NULL, XML_PARSE_NOENT);
	if(P_Reader) {
		int r = 0;
		xmlTextReaderPreservePattern(P_Reader, reinterpret_cast<const xmlChar *>(p_chr_tag), 0);
		r = xmlTextReaderRead(P_Reader);
		while(r == 1)
			r = xmlTextReaderRead(P_Reader);
		if(!r) {
			P_Doc = xmlTextReaderCurrentDoc(P_Reader);
			if(P_Doc) {
				xmlNode * p_root = xmlDocGetRootElement(P_Doc);
				if(FindFirstRec(p_root, &P_CurRec, p_chr_tag) > 0 && P_CurRec && sstreqi_ascii((const char *)P_CurRec->name, p_chr_tag)) {
					xmlNode * p_rec = P_CurRec;
					for(ChecksCount = 1; p_rec = p_rec->next;)
						if(sstreqi_ascii(reinterpret_cast<const char *>(p_rec->name), p_chr_tag))
							ChecksCount++;
				}
			}
		}
	}
	SubVer = subVer;
}

XmlReader::~XmlReader()
{
	if(P_Reader) {
		xmlFreeTextReader(P_Reader);
		P_Reader = 0;
	}
	xmlFreeDoc(P_Doc);
}

int XmlReader::GetGiftCard(const xmlNode * const * pPlugins, SString & rSerial, int isPaym)
{
	int    ok = -1;
	int    is_gift_card = 0;
	const char * p_gift_card      = (isPaym) ? "card.number" : "gift.card.number";
	const char * p_plug_card_attr = "value;key";
	SString val, serial;
	rSerial.Z();
	if(pPlugins) {
		for(const xmlNode * p_plugins = *pPlugins; !is_gift_card && p_plugins; p_plugins = p_plugins->next) {
			if(sstreqi_ascii((const char *)p_plugins->name, "plugin-property") && p_plugins->properties) {
				const xmlAttr * p_fld = p_plugins->properties;
				is_gift_card = 0;
				serial = 0;
				for(; p_fld; p_fld = p_fld->next) {
					if(p_fld->children && p_fld->children->content) {
						int idx = 0;
						val = (const char *)p_fld->children->content;
						if(PPSearchSubStr(p_plug_card_attr, &(idx = 0), (const char *)p_fld->name, 1) > 0) {
							switch(idx) {
								case 0: // Серийный номер подарочной карты
									serial = val;
									break;
								case 1:
									if(val.Cmp(p_gift_card, 1) == 0) // Подарочная карта
										is_gift_card = 1;
									break;
							}
						}
					}
				}
			}
		}
	}
	if(is_gift_card && serial.Len()) {
		rSerial = serial;
		ok = 1;
	}
	return ok;
}

int XmlReader::Next(Packet * pPack)
{
	// "plugin-property"
	//const char * p_attribs = "shop;operationType;operDay;cash;shift;saletime;number;amount;discountAmount;username;userTabNumber;tabNumber";
	int    ok = -1;
	Packet pack;
	SString tag_name;
	SString val;
	SString attr_name;
	do {
		if(P_CurRec) {
			Header hdr;
			// Read header
			tag_name.Set(P_CurRec->name).ToLower();
			if(P_CurRec->properties) {
				for(xmlAttr * p_fld = P_CurRec->properties; p_fld; p_fld = p_fld->next) {
					if(p_fld->children && p_fld->children->content) {
						attr_name.Set(p_fld->name).ToLower();
						val.Set(p_fld->children->content).ToLower();
						if(attr_name == "shop") {
						}
						else if(attr_name == "operationtype") { // Тип чека (продажа/возврат)
							if(val == "true")
								hdr.IsSale = 1;
						}
						else if(attr_name == "operday") { // Операционный день (дата)
						}
						else if(attr_name == "cash") { // Номер кассы
							hdr.CashNum = val.ToLong();
						}
						else if(attr_name == "shift") { // Номер смены
							hdr.SmenaNum = val.ToLong();
						}
						else if(attr_name == "saletime") { // Дата время чека
							strtodatetime(val, &hdr.Dtm, DATF_ISO8601, 0);
						}
						else if(attr_name == "number") { // Номер чека
							hdr.ChkNum = val.ToLong();
						}
						else if(attr_name == "amount") { // Сумма
							hdr.Amount = val.ToReal();
						}
						else if(attr_name == "discountamount") { // Сумма скидки
							hdr.Discount = val.ToReal();
						}
						else if(attr_name == "username") { // Имя кассира
						}
						else if(attr_name == "usertabnumber") { // Номер кассира (userTabNumber)
						}
						else if(attr_name == "tabnumber") { // Номер кассира для более новых версий Set Retail 10 (tabNumber)
							hdr.TabNum = val.ToLong();
						}
					}
				}
				if(!P_LogNumList || P_LogNumList->lsearch(hdr.CashNum, 0)) {
					THROW(pack.PutHead(&hdr));
					ok = 1;
				}
			}
			if(ok < 0)
				P_CurRec = P_CurRec->next;
		}
	} while(P_CurRec && ok < 0);
	//
	// Строки чека, дисконтые карты
	//
	if(ok > 0) {
		const char * p_items_attr = "order;goodsCode;barCode;cost;count;amount;nds;ndsSumm;discountValue;departNumber;costWithDiscount";
		const xmlNode * p_root  = 0;
		const xmlNode * p_items = 0;
		const xmlNode * p_fld_ = 0;
		SString attr_key;
		SString attr_val;
		for(p_fld_ = P_CurRec->children; /* @v11.4.0 (plugin-property считать еще надо!) !p_root &&*/ p_fld_; p_fld_ = p_fld_->next) {
			if(sstreqi_ascii(reinterpret_cast<const char *>(p_fld_->name), "positions")) {
				p_root = p_fld_;
			}
			else if(sstreqi_ascii(reinterpret_cast<const char *>(p_fld_->name), "plugin-property")) { // @v11.4.0
				//
				attr_key.Z();
				attr_val.Z();
				for(const xmlAttr * p_attr = p_fld_->properties; p_attr; p_attr = p_attr->next) {
					if(sstreqi_ascii(reinterpret_cast<const char *>(p_attr->name), "key")) {
						if(attr_key.IsEmpty())
							attr_key.Set(p_attr->children->content);
					}
					else if(sstreqi_ascii(reinterpret_cast<const char *>(p_attr->name), "value")) {
						if(attr_val.IsEmpty())
							attr_val.Set(p_attr->children->content);
					}
				}
				//if(attr_key.IsEqiAscii())
			}
		}
		if(p_root) {
			for(p_fld_ = p_root->children; !p_items && p_fld_; p_fld_ = p_fld_->next)
				if(sstreqi_ascii(reinterpret_cast<const char *>(p_fld_->name), "position"))
					p_items = p_fld_;
		}
		if(p_items) {
			for(; p_items; p_items = p_items->next) {
				if(p_items->type == XML_ELEMENT_NODE && p_items->properties) {
					Item item;
					MEMSZERO(item);
					for(xmlAttr * p_fld = p_items->properties; p_fld; p_fld = p_fld->next) {
						if(p_fld->children && p_fld->children->content) {
							attr_name.Set(p_fld->name).ToLower();
							val.Set(p_fld->children->content).ToLower();
							if(attr_name == "order") { // Номер позиции в чеке
								item.Pos = val.ToLong();
							}
							else if(attr_name == "goodscode") {
								val.CopyTo(item.GoodsCode, sizeof(item.GoodsCode));
							}
							else if(attr_name == "barcode") {
								val.CopyTo(item.Barcode, sizeof(item.Barcode));
							}
							else if(attr_name == "cost") {
								item.Price = val.ToReal();
							}
							else if(attr_name == "count") {
								item.Qtty = val.ToReal();
							}
							else if(attr_name == "amount") {
								item.Amount = val.ToReal();
							}
							else if(attr_name == "nds") {
								item.VatAmount = (item.Amount * val.ToReal()) / 100;
							}
							else if(attr_name == "ndssumm") {
								item.VatAmount = val.ToReal();
							}
							else if(attr_name == "discountvalue") {
								item.Discount = val.ToReal();
							}
							else if(attr_name == "departnumber") {
								item.Div = val.ToLong();
							}
							else if(attr_name == "costwithdiscount") {
								item.PriceWithDiscount = val.ToReal();
							}
						}
					}
					//
					// Извлекаем информацию о подарочной карте (продажа подарочной карты)
					//
					{
						/*const char * p_gift_card = "gift.card.number";
						const char * p_plug_card_attr = "value;key";
						int is_gift_card = 0;*/
						SString serial;
						if(GetGiftCard(&p_items->children, serial, 0) > 0)
							serial.CopyTo(item.Serial, sizeof(item.Serial));
						/*
						for(const xmlNode * p_plugins = p_items->children; !is_gift_card && p_plugins; p_plugins = p_plugins->next) {
							if(stricmp((const char *)p_plugins->name, "plugin-property") == 0 && p_plugins->properties) {
								xmlAttr * p_fld = p_plugins->properties;
								is_gift_card = 0;
								serial = 0;
								for(; p_fld; p_fld = p_fld->next) {
									if(p_fld->children && p_fld->children->content) {
										int idx = 0;
										val = (const char *)p_fld->children->content;
										if(PPSearchSubStr(p_plug_card_attr, &(idx = 0), (const char *)p_fld->name, 1) > 0) {
											switch(idx) {
												case 0: // Серийный номер подарочной карты
													serial = val;
													break;
												case 1:
													if(val.Cmp(p_gift_card, 1) == 0) // Подарочная карта
														is_gift_card = 1;
													break;
											}
										}
									}
								}
							}
						}
						if(is_gift_card)
							serial.CopyTo(item.Serial, sizeof(item.Serial));
						*/
					}
					THROW(pack.AddItem(&item));
				}
			}
		}
		{
			//
			// Извлекаем тип оплаты
			//
			/*
				<payments>
					<payment typeClass="CashPaymentEntity" amountPurchase="48613.85"/>
					<payment typeClass="com_mobimoney_plugin" amountPurchase="12.00" amountReturn="6.00"/>
					<payment typeClass="BankCardPaymentEntity" amountPurchase="110704.18"/>
				</payments>
			*/
			SString gift_card_code;
			for(const xmlNode * p_fld = P_CurRec->children; p_fld; p_fld = p_fld->next) {
				if(sstreqi_ascii((const char *)p_fld->name, "payments")) {
					//const char * p_items_attr = "amount;typeClass";
					CcAmountList ccpl;
					Header head;
					pack.GetHead(&head);
					for(const xmlNode * p_paym_fld = p_fld->children; p_paym_fld; p_paym_fld = p_paym_fld->next) {
						if(p_paym_fld->type == XML_ELEMENT_NODE) {
							int16  banking = -1;
							int    amount_type = CCAMTTYP_CASH;
							double amount  = 0.0;
							gift_card_code = 0;
							for(xmlAttr * p_props = p_paym_fld->properties; p_props; p_props = p_props->next) {
								if(p_props->children && p_props->children->content) {
									attr_name.Set(p_props->name).ToLower();
									val.Set(p_props->children->content).ToLower();
									if(attr_name == "amount")
										amount = val.ToReal();
									else if(attr_name == "typeclass") {
										if(val.IsEqiAscii("BankCardPaymentEntity") || val.IsEqiAscii("ExternalBankTerminalPaymentEntity") || val.IsEqiAscii("ODBankCardPaymentEntity"))
											amount_type = CCAMTTYP_BANK;
										else if(val.IsEqiAscii("GiftCardPaymentEntity"))
											amount_type = CCAMTTYP_CRDCARD;
										else if(val.IsEqiAscii("CashChangePaymentEntity")) // сдача
											amount_type = CCAMTTYP_DELIVERY;
										else if(val.IsEqiAscii("CashPaymentEntity")) // Сумма, полученная наличными (без учета сдачи)
											amount_type = CCAMTTYP_NOTE;
										else if(val.IsEqiAscii("com_mobimoney_plugin")) // @v10.1.0 Специальная сумма - заносится как банковская оплата
											amount_type = CCAMTTYP_BANK;
									}
									if(amount_type == CCAMTTYP_CRDCARD)
										GetGiftCard(&p_paym_fld->children, gift_card_code, 1);
								}
							}
							if(amount != 0.0) {
								if(amount_type == CCAMTTYP_CRDCARD) {
									if(gift_card_code.NotEmptyS())
										gift_card_code.CopyTo(head.GiftCardNum, sizeof(head.GiftCardNum));
								}
								ccpl.Add(amount_type, amount, 0);
							}
						}
					}
                    head.BankingAmount = ccpl.Get(CCAMTTYP_BANK);
                    head.GiftCardAmount = ccpl.Get(CCAMTTYP_CRDCARD);
                    head.CheckAmount = ccpl.Get(CCAMTTYP_CASH) + ccpl.Get(CCAMTTYP_NOTE) - ccpl.Get(CCAMTTYP_DELIVERY) + head.BankingAmount + head.GiftCardAmount;
					pack.PutHead(&head);
					break;
				}
			}
		}
		{
			//
			// Извлекаем скидки
			//
			xmlNode * p_fld = P_CurRec->children;
			for(; p_fld != 0; p_fld = p_fld->next) {
				if(sstreqi_ascii((const char *)p_fld->name, "discounts")) {
					const char * p_items_attr = "positionOrder;amount";
					Header head;
					pack.GetHead(&head);
					for(const xmlNode * p_dis_fld = p_fld->children; p_dis_fld; p_dis_fld = p_dis_fld->next) {
						int16  banking = -1;
						long   pos = -1;
						double discount = 0.0;
						for(xmlAttr * p_props = p_dis_fld->properties; p_props; p_props = p_props->next) {
							if(p_props->children && p_props->children->content) {
								int idx = 0;
								val = (const char *)p_props->children->content;
								if(PPSearchSubStr(p_items_attr, &idx, (const char *)p_props->name, 1) > 0) {
									switch(idx) {
										case 0:
											pos = val.ToLong();
											break;
										case 1:
											discount += val.ToReal();
											break;
									}
								}
							}
						}
						if(pos == 0) // Скидка с позицией 0 особенная, ее нет в строках чека
							head.AddedDiscount += discount;
					}
					pack.PutHead(&head);
					break;
				}
			}
			//
			// извлекаем номер дисконтной карты
			//
			xmlNode * p_cards_fld = 0;
			p_fld = P_CurRec->children;
			for(; !p_cards_fld && p_fld; p_fld = p_fld->next)
				if(sstreqi_ascii((const char *)p_fld->name, "discountCards"))
					p_cards_fld = p_fld;
			if(p_cards_fld && p_cards_fld->children) {
				xmlNode * p_dis_fld = 0;
				for(p_fld = p_cards_fld->children; !p_dis_fld && p_fld; p_fld = p_fld->next)
					if(sstreqi_ascii((const char *)p_fld->name, "discountCard"))
						p_dis_fld = p_fld;
				if(p_dis_fld && p_dis_fld->children && p_dis_fld->children->content) {
					Header head;
					pack.GetHead(&head);
					STRNSCPY(head.SCardNum, p_dis_fld->children->content);
					pack.PutHead(&head);
				}
			}
		}
		{
			// exciseBottles
			SString barcode_as_ident;
			SString egais_mark;
			for(const xmlNode * p_fld = P_CurRec->children; p_fld; p_fld = p_fld->next) {
				if(sstreqi_ascii((const char *)p_fld->name, "exciseBottles")) {
					for(const xmlNode * p_inr_fld = p_fld->children; p_inr_fld; p_inr_fld = p_inr_fld->next) {
						barcode_as_ident.Z();
						egais_mark.Z();
						if(SXml::IsName(p_inr_fld, "bottle")) {
							//<bottle barcode="9414416305528" exciseBarcode="236304799221631120001PBU2FHEAH5OPPWAANJGZJFJCNMS6GCMVE4QWLH2QITKZC7HLYJE3TAMNOLFAZ4U64MJYET4QHBVLELKUK64G22V67EJGSXRKUIH3BBZ2QJJF22VT6HNJ35KFSJMHB667Y" volume="0.0" price="2726.0"/>
							if(SXml::GetAttrib(p_inr_fld, "barcode", val)) {
								barcode_as_ident = val;
							}
							if(SXml::GetAttrib(p_inr_fld, "exciseBarcode", val)) {
								egais_mark = val;
							}
						}
						if(egais_mark.NotEmpty() && barcode_as_ident.NotEmpty()) {
							for(uint pidx = 0; pidx < pack.GetCount(); pidx++) {
								Item & r_item = pack.GetItemByIdx(pidx);
								if(barcode_as_ident.IsEqiAscii(r_item.Barcode) && isempty(r_item.Mark)) {
									STRNSCPY(r_item.Mark, egais_mark);
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if(ok > 0 && P_CurRec)
		P_CurRec = P_CurRec->next;
	ASSIGN_PTR(pPack, pack);
	CATCHZOK
	return ok;
}

IMPL_CMPFUNC(AcceptedCheck_, i1, i2)
{
	/* @v11.4.0
	const ACS_CRCSHSRV::AcceptedCheck_ * p_i1 = static_cast<const ACS_CRCSHSRV::AcceptedCheck_ *>(i1);
	const ACS_CRCSHSRV::AcceptedCheck_ * p_i2 = static_cast<const ACS_CRCSHSRV::AcceptedCheck_ *>(i2);
	if(p_i1->CashNum > p_i2->CashNum)
		return 1;
	else if(p_i1->CashNum < p_i2->CashNum)
		return -1;
	else if(p_i1->Dt > p_i2->Dt)
		return 1;
	else if(p_i1->Dt < p_i2->Dt)
		return -1;
	else if(p_i1->Tm > p_i2->Tm)
		return 1;
	else if(p_i1->Tm < p_i2->Tm)
		return -1;
	else
		return 0;
	*/
	RET_CMPCASCADE3(static_cast<const ACS_CRCSHSRV::AcceptedCheck_ *>(i1), static_cast<const ACS_CRCSHSRV::AcceptedCheck_ *>(i2), CashNum, Dt, Tm); // @v11.4.0 
}

int ACS_CRCSHSRV::ConvertWareListV10(const SVector * pZRepList, const char * pPath, const char * pWaitMsg)
{
	int    ok = 1;
	SString msg_buf;
	double max_qtty_diff = 0.0;
	double sum_diff = 0.0;
	PPObjGoods goods_obj;
	IterCounter cntr;
	XmlReader::Packet pack;
	XmlReader reader(pPath, &LogNumList, ModuleSubVer);
	{
		PPTransaction tra(1);
		THROW(tra);
		while(reader.Next(&pack) > 0) {
			int    r   = 0;
			long   cshr_id = 0;
			PPID   id = 0;
			PPID   scard_id = 0;
			PPID   gift_card_id = 0;
			XmlReader::Item   item;
			XmlReader::Header hdr;
			AcceptedCheck_ accept_chk;
			SCardTbl::Rec scard_rec;
			pack.GetHead(&hdr);
			accept_chk.CashNum = hdr.CashNum;
			accept_chk.Dt      = hdr.Dtm.d;
			accept_chk.Tm      = hdr.Dtm.t;
			if(!AcceptedCheckList.lsearch(&accept_chk, 0, PTR_CMPFUNC(AcceptedCheck_))) {
				AcceptedCheckList.insert(&accept_chk);
				// GetCrCshSrvDateTime(buf, hdr.ChkNum, &dttm);
				cshr_id = CshrList.GetCshrID(hdr.TabNum, hdr.Dtm.d);
				long   fl  = (hdr.IsSale == 0) ? CCHKF_RETURN : 0;
				if(hdr.SCardNum[0] && SearchCardCode(&CC.Cards, hdr.SCardNum, &scard_rec) > 0) {
					scard_id = scard_rec.ID;
				}
				if(hdr.GiftCardNum[0] && SearchCardCode(&CC.Cards, hdr.GiftCardNum, &scard_rec) > 0) {
					gift_card_id = scard_rec.ID;
				}
				if(pZRepList) {
					ZRep zrep_key;
					zrep_key.CashCode = hdr.CashNum;
					zrep_key.ZRepCode = hdr.SmenaNum;
					if(!pZRepList->lsearch(&zrep_key, 0, PTR_CMPFUNC(_2long)))
						fl |= CCHKF_TEMPSESS;
				}
				else if(Flags & PPACSF_TEMPSESS)
					fl |= CCHKF_TEMPSESS;
				double chk_dis = hdr.Discount + hdr.AddedDiscount;
				//
				// Если подарочная карта не найдена, будем считать что оплата без нее
				//
				if(!gift_card_id)
					hdr.GiftCardAmount = 0.0;
				hdr.CheckAmount = (fl & CCHKF_RETURN) ? -hdr.CheckAmount : hdr.CheckAmount;
				THROW(r = AddTempCheck(&id, hdr.SmenaNum, fl, hdr.CashNum, hdr.ChkNum, cshr_id, 0, hdr.Dtm, hdr.CheckAmount, 0/*, add_paym, hdr.GiftCardAmount*/));
				if(r < 0 && !(Flags & PPACSF_TEMPSESS) && !(fl & CCHKF_TEMPSESS)) {
					PPID   sess_id = 0;
					if(CS.SearchByNumber(&sess_id, NodeID, hdr.CashNum, hdr.SmenaNum, hdr.Dtm.d) > 0 && sess_id && CS.data.Temporary) {
						THROW(CS.ResetTempSessTag(sess_id, 0));
						SessAry.addUnique(sess_id);
					}
				}
				PPID   chk_id = 0;
				for(long idx = 0; pack.EnumItems(&idx, &item) > 0;) {
					PPID   goods_id = 0L;
					Goods2Tbl::Rec goods_rec;
					if(item.GoodsCode[0] && goods_obj.SearchByBarcode(item.GoodsCode, 0, &goods_rec, 1) > 0)
						goods_id = goods_rec.ID;
					if(goods_id == 0 && item.Barcode[0] && goods_obj.SearchByBarcode(item.Barcode, 0, &goods_rec, 1) > 0)
						goods_id = goods_rec.ID;
					if(!goods_id) {
						PPGetMessage(mfError, PPERR_GDSBYBARCODENFOUND, item.GoodsCode, DS.CheckExtFlag(ECF_SYSSERVICE), msg_buf.Z());
						msg_buf.Space().CatEq("cashno", hdr.CashNum).Comma();
						msg_buf.CatEq("barcode", item.Barcode).Comma();
						PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
					}
					THROW(r = SearchTempCheckByCode(hdr.CashNum, hdr.ChkNum, hdr.SmenaNum));
					if(r > 0) {
						chk_id = P_TmpCcTbl->data.ID;
						item.Qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(item.Qtty) : fabs(item.Qtty);
						item.Amount = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -item.Amount : item.Amount;
						item.Discount = (item.Qtty) ? (item.Price - item.Amount / item.Qtty) : 0;
						SetupTempCcLineRec(0, chk_id, hdr.ChkNum, P_TmpCcTbl->data.Dt, item.Div, goods_id);
						// @v10.7.3 SetTempCcLineValues(0, item.Qtty, item.Amount / item.Qtty + item.Discount, item.Discount/*, item.Serial*/, 0/*pLnExtStrings*/);
						// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
						{
							PPExtStrContainer ccl_ext_strings;
							ccl_ext_strings.PutExtStrData(CCheckPacket::lnextSerial, item.Serial);
							ccl_ext_strings.PutExtStrData(CCheckPacket::lnextEgaisMark, item.Mark); // @v11.4.0
							THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, item.Qtty, item.Amount / item.Qtty + item.Discount, item.Discount/*, item.Serial*/, &ccl_ext_strings)); // @v10.7.3
						}
						if(!P_TmpCcTbl->data.SCardID && scard_id)
							THROW(AddTempCheckSCardID(chk_id, scard_id));
						if(item.Discount) {
							const double _d = item.Discount * item.Qtty;
							THROW(AddTempCheckAmounts(chk_id, 0.0, _d));
						}
					}
				}
				if(id) {
					/*
					double Amount;        // извлекается из заголовка чека, как показала практика эта сумма не всегда достоверна
					double CheckAmount;   // Сумма оплат внесенных за чек
					double AddedDiscount; // дополнительная скидка
					double BankingAmount;
					double GiftCardAmount;
					*/
					const  double total_amount = hdr.CheckAmount;
					double bank_amount = (fl & CCHKF_RETURN) ? -hdr.BankingAmount : hdr.BankingAmount;
					double ccard_amount = (fl & CCHKF_RETURN) ? -hdr.GiftCardAmount : hdr.GiftCardAmount;
					double cash_amount = (total_amount - bank_amount - ccard_amount);
					assert(total_amount == (cash_amount + bank_amount + ccard_amount));
					long   added_cc_flags = 0;
					int    _list = 0; // Признак наличия списка оплат
					if(hdr.GiftCardAmount != 0.0) {
						if(gift_card_id) {
							THROW(AddTempCheckPaym(id, CCAMTTYP_CRDCARD, ccard_amount, gift_card_id));
							_list = 1;
						}
						else {
							ccard_amount = 0.0;
							if(cash_amount != 0.0)
								cash_amount = (total_amount - bank_amount);
							else
								bank_amount = total_amount;
						}
					}
					if(bank_amount == total_amount) {
						added_cc_flags |= CCHKF_BANKING;
					}
					else if(bank_amount != 0.0) {
						THROW(AddTempCheckPaym(id, CCAMTTYP_BANK, bank_amount, 0));
						_list = 1;
					}
					if(_list) {
						if(cash_amount != 0.0)
							THROW(AddTempCheckPaym(id, CCAMTTYP_CASH, cash_amount, 0));
					}
					if(added_cc_flags) {
						THROW(UpdateTempCheckFlags(id, added_cc_flags));
					}
				}
			}
			PPWaitPercent(cntr.Increment(), pWaitMsg);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ACS_CRCSHSRV::ConvertWareList(const SVector * pZRepList, const char * pWaitMsg)
{
	int    ok = 1;
	SString msg_buf;
	double max_qtty_diff = 0.0;
	double sum_diff = 0.0;
	PPObjGoods goods_obj; // @average
	IterCounter cntr;
	//
	// Коды полей входного файла чеков
	//
	int    fldn_h_op       = 0;
	int    fldn_h_date     = 0;
	int    fldn_h_check    = 0;
	int    fldn_h_cash     = 0;
	int    fldn_h_sess     = 0;
	int    fldn_h_cashier  = 0;
	//
	// Коды полей входного файла чековых строк
	//
	int    fldn_l_chkln_id = 0;
	int    fldn_l_check    = 0;
	int    fldn_l_cash     = 0;
	int    fldn_l_sess     = 0;
	int    fldn_l_div      = 0;
	int    fldn_l_goodsid  = 0;
	int    fldn_l_qtty     = 0;
	int    fldn_l_price    = 0;
	int    fldn_l_disc     = 0;
	int    fldn_l_barcode  = 0; // @average
	int    fldn_l_banking  = 0;
	int    fldn_l_sum      = 0; // Сумма по строке.

	DbfTable * p_dbfth = 0;
	DbfTable * p_dbftr = 0;

	Backup("dbfh", PathRpt[filTypChkHeads]);
	THROW_MEM(p_dbfth = new DbfTable(PathRpt[filTypChkHeads]));
	THROW_PP(p_dbfth->isOpened(), PPERR_DBFOPFAULT);
	p_dbfth->getFieldNumber("operation", &fldn_h_op);
	p_dbfth->getFieldNumber("dateoper",  &fldn_h_date);
	p_dbfth->getFieldNumber("numchk",    &fldn_h_check);
	p_dbfth->getFieldNumber("cashcode",  &fldn_h_cash);
	p_dbfth->getFieldNumber("smena",     &fldn_h_sess);
	p_dbfth->getFieldNumber("cashier",   &fldn_h_cashier);

	Backup("dbfr", PathRpt[filTypChkRows]);
	THROW_MEM(p_dbftr = new DbfTable(PathRpt[filTypChkRows]));
	THROW_PP(p_dbftr->isOpened(), PPERR_DBFOPFAULT);
	p_dbftr->getFieldNumber("chkln_id",  &fldn_l_chkln_id);
	p_dbftr->getFieldNumber("numchk",    &fldn_l_check);
	p_dbftr->getFieldNumber("smena",     &fldn_l_sess);
	p_dbftr->getFieldNumber("cashcode",  &fldn_l_cash);
	p_dbftr->getFieldNumber("div",       &fldn_l_div);
	p_dbftr->getFieldNumber("cod",       &fldn_l_goodsid);
	p_dbftr->getFieldNumber("quant",     &fldn_l_qtty);
	p_dbftr->getFieldNumber("price",     &fldn_l_price);
	p_dbftr->getFieldNumber("disc",      &fldn_l_disc);
	p_dbftr->getFieldNumber("barcode",   &fldn_l_barcode); // @average
	p_dbftr->getFieldNumber("bank",      &fldn_l_banking);
	p_dbftr->getFieldNumber("summa",     &fldn_l_sum);

	if(p_dbfth->getNumRecs()) {
		cntr.Init(p_dbfth->getNumRecs()+p_dbftr->getNumRecs());
		PPTransaction tra(1);
		THROW(tra);
		if(p_dbfth->top()) {
			do {
				char   op[32], buf[128];
				LDATETIME dttm;
				long   chk, csh, nsmena, tab_num, cshr_id;
				DbfRecord dbfrh(p_dbfth);
				if(p_dbfth->getRec(&dbfrh) <= 0)
					break;
				dbfrh.get(fldn_h_op,    op, sizeof(op));
				dbfrh.get(fldn_h_check, chk);
				dbfrh.get(fldn_h_cash,  csh);
				dbfrh.get(fldn_h_sess,  nsmena);
				dbfrh.get(fldn_h_date,  buf, sizeof(buf));
				GetCrCshSrvDateTime(buf, chk, &dttm);
				dbfrh.get(fldn_h_cashier, tab_num);
				cshr_id = CshrList.GetCshrID(tab_num, dttm.d);
				if(LogNumList.lsearch(csh)) {
					int    r   = 0;
					PPID   id = 0;
					long   fl  = (op[0] == 'R') ? CCHKF_RETURN : 0;
					double sum = 0.0, dscnt = 0.0;
					if(pZRepList) {
						ZRep zrep_key;
						zrep_key.CashCode = csh;
						zrep_key.ZRepCode = nsmena;
						if(!pZRepList->lsearch(&zrep_key, 0, PTR_CMPFUNC(_2long)))
							fl |= CCHKF_TEMPSESS;
					}
					else if(Flags & PPACSF_TEMPSESS)
						fl |= CCHKF_TEMPSESS;
					THROW(r = AddTempCheck(&id, nsmena, fl, csh, chk, cshr_id, 0, dttm, sum, dscnt));
					if(r < 0 && !(Flags & PPACSF_TEMPSESS) && !(fl & CCHKF_TEMPSESS)) {
						PPID   sess_id = 0;
						if(CS.SearchByNumber(&sess_id, NodeID, csh, nsmena, dttm.d) > 0 && sess_id && CS.data.Temporary) {
							THROW(CS.ResetTempSessTag(sess_id, 0));
							SessAry.addUnique(sess_id);
						}
					}
				}
				PPWaitPercent(cntr.Increment(), pWaitMsg);
			} while(p_dbfth->next());
		}
		if(p_dbftr->top()) {
			do {
				int    r;
				char   barcode[32];
				DbfRecord dbfrr(p_dbftr);
				PPID   goods_id = 0;
				int    banking = 0;
				long   chkln_id = 0, chk_no = 0, sess_no = 0, cash_no = 0, div = 0;
				double qtty, price, dscnt;
				double sum = 0.0;
				if(p_dbftr->getRec(&dbfrr) <= 0)
					break;
				dbfrr.get(fldn_l_chkln_id, chkln_id);
				dbfrr.get(fldn_l_check, chk_no);
				dbfrr.get(fldn_l_sess,  sess_no);
				dbfrr.get(fldn_l_cash,  cash_no);
				dbfrr.get(fldn_l_div,   div);
				dbfrr.get(fldn_l_goodsid, goods_id);
				dbfrr.get(fldn_l_qtty,  qtty);
				dbfrr.get(fldn_l_price, price);
				dbfrr.get(fldn_l_disc,  dscnt);
				dbfrr.get(fldn_l_sum,   sum);
				// @average {
				//
				// Обработка аварийного случая, когда на кассовом модуле
				// остались товары, с идентификаторами, отличными от тех, что
				// в нашей базе данных. Для такого случая сверяемся по штрихкоду.
				//
				barcode[0] = 0;                     // @average
				dbfrr.get(fldn_l_barcode, barcode, sizeof(barcode)); // @average
				dbfrr.get(fldn_l_banking, banking);
				if(fldn_l_barcode && barcode[0]) {
					Goods2Tbl::Rec goods_rec;
					if(goods_obj.SearchByBarcode(barcode, 0, &goods_rec, 1) > 0)
						if(goods_rec.ID != goods_id) {
							msg_buf.CatEq("cashno",    cash_no).Comma();
							msg_buf.CatEq("goodsid",   goods_rec.ID).Comma();
							msg_buf.CatEq("barcode",   barcode).Comma();
							msg_buf.CatEq("goodsname", goods_rec.Name);
							PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
							goods_id = goods_rec.ID;
						}
				}
				// } @average
				THROW(r = SearchTempCheckByCode(cash_no, chk_no, sess_no));
				if(r > 0) {
					PPID   chk_id = P_TmpCcTbl->data.ID;
					qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(qtty) : fabs(qtty);
					SetupTempCcLineRec(0, chk_id, chk_no, P_TmpCcTbl->data.Dt, div, goods_id);
					if(!fldn_l_sum)
						sum = price * qtty;
					// @v10.7.3 SetTempCcLineValues(0, qtty, (sum + dscnt)/qtty, dscnt/qtty, 0/*pLnExtStrings*/);
					// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
					THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, qtty, (sum + dscnt)/qtty, dscnt/qtty, 0/*pLnExtStrings*/)); // @v10.7.3
					if(banking) {
						THROW(UpdateTempCheckFlags(chk_id, CCHKF_BANKING));
					}
					THROW(AddTempCheckAmounts(chk_id, sum, dscnt));
					if(!P_TmpCcTbl->data.SCardID && P_SCardPaymTbl) {
						TempOrderTbl::Key0 k0;
						MEMSZERO(k0);
						k0.ID = chkln_id;
						if(P_SCardPaymTbl->search(0, &k0, spEq) && k0.ID == chkln_id) {
							long card_id = *(long *)P_SCardPaymTbl->data.Name;
							if(card_id)
								THROW(AddTempCheckSCardID(chk_id, card_id));
						}
					}
				}
				PPWaitPercent(cntr.Increment(), pWaitMsg);
			} while(p_dbftr->next());
		}
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	/*
	msg_buf.CatEq("MaxQttyDiff", max_qtty_diff, MKSFMTD(0, 6, 0)).Comma();
	msg_buf.CatEq("SumDiff",     sum_diff, MKSFMTD(0, 6, 0));
	PPLogMessage(PPFILNAM_ACS_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
	*/
	delete p_dbfth;
	delete p_dbftr;
	return ok;
}

int ACS_CRCSHSRV::ConvertCheckHeads(const SVector * pZRepList, const char * pWaitMsg)
{
	int     ok = 1;
	SString file_name, save_file_name;
	IterCounter cntr;
	PPImpExp * p_ie_csh = 0;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(uint pos = 0; SeparatedFileSet.get(&pos, file_name);) {
			long   c, count;
			PPBillImpExpParam * p_ie_param = P_IEParam[PPREC_CS_CHKHD - PPREC_CS_ZREP];
			p_ie_param->FileName = file_name;
			THROW_MEM(p_ie_csh = new PPImpExp(p_ie_param, 0));
			Backup("chkh", file_name);
			THROW(p_ie_csh->OpenFileForReading(0));
			p_ie_csh->GetNumRecs(&count);
			cntr.Init(count);
			for(c = 0; c < count; c++) {
				long   cshr_id;
				LDATETIME  dttm;
				Sdr_CS_Chkhd  cs_chkhd;
				THROW(p_ie_csh->ReadRecord(&cs_chkhd, sizeof(cs_chkhd)));
				GetCrCshSrvDateTime(cs_chkhd.OperDate, cs_chkhd.CheckNumber, &dttm);
				cshr_id = CshrList.GetCshrID(cs_chkhd.CashierNo, dttm.d);
				if(LogNumList.lsearch(cs_chkhd.CashNumber)) {
					int    r   = 0;
					PPID   id = 0;
					long   fl  = (cs_chkhd.Operation == 'R') ? CCHKF_RETURN : 0;
					if(pZRepList) {
						ZRep zrep_key;
						zrep_key.CashCode = cs_chkhd.CashNumber;
						zrep_key.ZRepCode = cs_chkhd.SessNumber;
						if(!pZRepList->lsearch(&zrep_key, 0, PTR_CMPFUNC(_2long)))
							fl |= CCHKF_TEMPSESS;
					}
					else if(Flags & PPACSF_TEMPSESS)
						fl |= CCHKF_TEMPSESS;
					THROW(r = AddTempCheck(&id, cs_chkhd.SessNumber, fl, cs_chkhd.CashNumber, cs_chkhd.CheckNumber, cshr_id, 0, dttm, 0.0, 0.0));
					if(r < 0 && !(Flags & PPACSF_TEMPSESS) && !(fl & CCHKF_TEMPSESS)) {
						PPID sess_id = 0;
						if(CS.SearchByNumber(&sess_id, NodeID, cs_chkhd.CashNumber, cs_chkhd.SessNumber, dttm.d) > 0 && sess_id && CS.data.Temporary) {
							THROW(CS.ResetTempSessTag(sess_id, 0));
							SessAry.addUnique(sess_id);
						}
					}
				}
				PPWaitPercent(cntr.Increment(), pWaitMsg);
			}
			ZDELETE(p_ie_csh);
			save_file_name = PathRpt[filTypChkHeads];
			ReplaceFilePath(save_file_name, file_name);
			::rename(file_name, save_file_name);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_ie_csh;
	return ok;
}

int ACS_CRCSHSRV::ConvertCheckRows(const char * pWaitMsg)
{
	int     ok = 1;
	SString file_name;
	SString save_file_name;
	SString barcode;
	SString goods_name;
	SString article;
	PPObjGoods  goods_obj;
	IterCounter cntr;
	PPImpExp * p_ie_csl = 0;
	{
		PPTransaction tra(1);
		THROW(tra);
		for(uint pos = 0; SeparatedFileSet.get(&pos, file_name);) {
			long   c, count;
			PPBillImpExpParam * p_ie_param = P_IEParam[PPREC_CS_CHKLN - PPREC_CS_ZREP];
			p_ie_param->FileName = file_name;
			THROW_MEM(p_ie_csl = new PPImpExp(p_ie_param, 0));
			Backup("chkr", file_name);
			THROW(p_ie_csl->OpenFileForReading(0));
			p_ie_csl->GetNumRecs(&count);
			cntr.Init(count);
			for(c = 0; c < count; c++) {
				int    r;
				Sdr_CS_ChkLn cs_chkln;
				THROW(p_ie_csl->ReadRecord(&cs_chkln, sizeof(cs_chkln)));
				THROW(r = SearchTempCheckByCode(cs_chkln.CashNumber, cs_chkln.CheckNumber, cs_chkln.SessNumber));
				if(r > 0) {
					PPID   goods_id = 0, grp_id = 0;
					double qtty = cs_chkln.Quantity, price = cs_chkln.Price, dscnt = cs_chkln.Discount, sum = cs_chkln.Sum;
					PPID   chk_id = P_TmpCcTbl->data.ID;
					PPGoodsPacket  gds_pack;
					BarcodeTbl::Rec  bc_rec;
					goods_name = "SetRetail";
					if(goods_obj.P_Tbl->SearchByName(PPGDSK_GROUP, goods_name, &grp_id, &gds_pack.Rec) <= 0) {
						gds_pack.Rec.Kind = PPGDSK_GROUP;
						goods_name.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						goods_obj.PutPacket(&grp_id, &gds_pack, 0);
					}
					if((barcode = cs_chkln.Barcode).NotEmptyS() && goods_obj.SearchByBarcode(barcode, &bc_rec) > 0)
						goods_id = bc_rec.GoodsID;
					else if(cs_chkln.Article && goods_obj.SearchByArticle(cs_chkln.Article, &bc_rec) > 0)
						goods_id = bc_rec.GoodsID;
					else if((goods_name = cs_chkln.GoodsName).Transf(CTRANSF_OUTER_TO_INNER).NotEmptyS()) {
						gds_pack.Z();
						THROW(goods_obj.P_Tbl->SearchByName(PPGDSK_GOODS, goods_name, &goods_id, &gds_pack.Rec));
					}
					if(!goods_id) {
						gds_pack.Z();
						gds_pack.Rec.Kind = PPGDSK_GOODS;
						if(cs_chkln.Article)
							article.Z().CatChar('$').Cat(cs_chkln.Article);
						if(goods_name.NotEmpty())
							goods_name.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						else if(barcode.NotEmpty())
							barcode.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						else if(cs_chkln.Article)
							article.CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						else
							(goods_name = "ID=").Cat(cs_chkln.Article).CopyTo(gds_pack.Rec.Name, sizeof(gds_pack.Rec.Name));
						STRNSCPY(gds_pack.Rec.Abbr, gds_pack.Rec.Name);
						gds_pack.Rec.ParentID = grp_id;
						if(barcode.NotEmpty())
							gds_pack.Codes.Add(barcode, 0, 1.0);
						if(cs_chkln.Article) {
							gds_pack.Codes.Add(article, -1, 1.0);
							THROW(goods_obj.PutPacket(&goods_id, &gds_pack, 0));
						}
					}
					qtty = (P_TmpCcTbl->data.Flags & CCHKF_RETURN) ? -fabs(qtty) : fabs(qtty);
					SetupTempCcLineRec(0, chk_id, cs_chkln.CheckNumber, P_TmpCcTbl->data.Dt, cs_chkln.Division, goods_id);
					SETIFZ(sum, price * qtty);
					// @v10.7.3 SetTempCcLineValues(0, qtty, (sum + dscnt)/qtty, dscnt/qtty, 0/*pLnExtStrings*/);
					// @v10.7.3 THROW_DB(P_TmpCclTbl->insertRec());
					THROW(SetTempCcLineValuesAndInsert(P_TmpCclTbl, qtty, (sum + dscnt)/qtty, dscnt/qtty, 0/*pLnExtStrings*/)); // @v10.7.3
					if(cs_chkln.IsBanking && !(P_TmpCcTbl->data.Flags & CCHKF_BANKING)) {
						THROW(UpdateTempCheckFlags(chk_id, CCHKF_BANKING));
					}
					THROW(AddTempCheckAmounts(chk_id, sum, dscnt));
					if(!P_TmpCcTbl->data.SCardID && P_SCardPaymTbl) {
						TempOrderTbl::Key0 k0;
						MEMSZERO(k0);
						k0.ID = cs_chkln.CheckLineID;
						if(P_SCardPaymTbl->search(0, &k0, spEq) && k0.ID == cs_chkln.CheckLineID) {
							long  card_id = *(long *)P_SCardPaymTbl->data.Name;
							if(card_id)
								THROW(AddTempCheckSCardID(chk_id, card_id));
						}
					}
				}
				PPWaitPercent(cntr.Increment(), pWaitMsg);
			}
			ZDELETE(p_ie_csl);
			save_file_name = PathRpt[filTypChkRows];
			ReplaceFilePath(save_file_name, file_name);
			::rename(file_name, save_file_name);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	delete p_ie_csl;
	return ok;
}

int ACS_CRCSHSRV::GetSeparatedFileSet(int filTyp)
{
	int  ok = -1;
	SString    buf, file_name, sect_name, param;
	SFsPath ps;
	PPGetSubStr(PPTXT_SETRETAIL_PARAM, SETR_PARAM_REPORTS, sect_name);
	PPGetSubStr(PPTXT_SETRETAIL_PARAM, SETR_PARAM_REPGANG + filTyp, param);
	PPIniFile ini_file(PathSetRExpCfg);
	if(ini_file.GetParam(sect_name, param, buf) > 0) {
		int  param_no = 0;
		StringSet param_set(';', buf);
		param.Z();
		for(uint pos = 0; param_no < 4 && param_set.get(&pos, buf); param_no++)
			if(param_no == 0)
				sect_name = buf;
			else if(param_no == 3)
				param = buf;
		ps.Split(PathRpt[filTyp]);
		if(param.NotEmptyS()) {
			size_t pos = 0;
			SString  num;
			int  d, m, y;
			decodedate(&d, &m, &y, &CurOperDate);
			if(param.Search("DD", 0, 1, &(pos = 0))) {
				num.CatLongZ(d, 2);
				param.Excise(pos, 2).Insert(pos, num);
			}
			if(param.Search("MM", 0, 1, &(pos = 0))) {
				num.Z().CatLongZ(m, 2);
				param.Excise(pos, 2).Insert(pos, num);
			}
			if(param.Search("YYYY", 0, 1, &(pos = 0))) {
				num.Z().CatLongZ(y, 4);
				param.Excise(pos, 4).Insert(pos, num);
			}
			else if(param.Search("YY", 0, 1, &(pos = 0))) {
				num.Z().CatLongZ(y % 100, 2);
				param.Excise(pos, 2).Insert(pos, num);
			}
			ps.Nam = param;
		}
		else
			ps.Nam = sect_name;
		ps.Merge(file_name);
		PPGetSubStr(PPTXT_SETRETAIL_PARAM, SETR_PARAM_SEPARATE, param);
		{
			Options &= ~oSeparateReports;
			if(sect_name.NotEmptyS()) {
				int    ipar = 0;
				ini_file.GetIntParam(sect_name, param, &ipar);
				if(ipar > 0)
					Options |= oSeparateReports;
			}
		}
		SeparatedFileSet.Z();
		if(Options & oSeparateReports) {
			PPGetSubStr(PPTXT_SETRETAIL_PARAM, SETR_PARAM_SEPARATEPATH, sect_name);
			for(uint i = 0; i < LogNumList.getCount(); i++) {
				param.Z().Cat(LogNumList.at(i));
				if(ini_file.GetParam(sect_name, param, buf) > 0) {
					SFsPath ps1(file_name);
					ps.Split(buf);
					ps.Drv = ps1.Drv;
				}
				else {
					ps.Split(file_name);
					ps.Dir.BSlash().Cat(param);
				}
				ps.Merge(buf);
				SeparatedFileSet.add(buf);
			}
		}
		else
			SeparatedFileSet.add(file_name);
		ok = 1;
	}
	return ok;
}

int ACS_CRCSHSRV::QueryFile(int filTyp, const char * pQueryBuf, LDATE queryDate)
{
	int    ok = 1;
	uint   pos;
	DbfTable * p_qtbl = 0;
	SString  query_buf(pQueryBuf), file_name, rmv_file_name;
	FILE * p_f = 0;
	SFile::Remove(PathQue[filTyp]);
	if(Options & oUseAltImport) {
		GetSeparatedFileSet(filTyp);
		for(pos = 0; SeparatedFileSet.get(&pos, file_name);) {
			rmv_file_name = PathRpt[filTyp];
			ReplaceFilePath(rmv_file_name, file_name);
			SFile::Remove(rmv_file_name);
		}
		if(Options & oSeparateReports) {
			query_buf.Semicol();
			for(uint i = 0; i < LogNumList.getCount(); i++) {
				query_buf.CatDivConditionally(',', 0, i > 0).Cat(LogNumList.at(i));
			}
		}
	}
	else
		SFile::Remove(PathRpt[filTyp]);
	{
		SFsPath ps(PathQue[filTyp]);
		if(ps.Nam.IsEqiAscii("all") && ps.Ext.IsEqiAscii("dbf")) {
			THROW(p_qtbl = CreateDbfTable(DBFS_CRCS_SIGNAL_ALL_EXPORT, PathQue[filTyp], 1));
			{
				DbfRecord dbfr_signal(p_qtbl);
				dbfr_signal.put(1, queryDate);
				dbfr_signal.put(2, "*");
				THROW_PP(p_qtbl->appendRec(&dbfr_signal), PPERR_DBFWRFAULT);
				ZDELETE(p_qtbl);
			}
		}
		else {
			THROW_PP_S(p_f = fopen(PathQue[filTyp], "wt"), PPERR_CANTOPENFILE, PathQue[filTyp]);
			fputs(query_buf, p_f);
			SFile::ZClose(&p_f);
		}
	}
	THROW(ok = WaitForExists(PathQue[filTyp], 1));
	if(ok > 0)
		THROW(ok = WaitForExists(PathFlag, 1));
	if(ok > 0) {
		if(Options & oUseAltImport) {
			for(pos = 0; SeparatedFileSet.get(&pos, file_name);) {
				PPWaitMsg(PPSTR_TEXT, PPTXT_WAITONFILE, file_name);
				THROW_PP(fileExists(file_name), PPERR_CASHSRV_IMPCHECKS);
			}
		}
		else {
			PPWaitMsg(PPSTR_TEXT, PPTXT_WAITONFILE, PathRpt[filTyp]);
			THROW_PP(fileExists(PathRpt[filTyp]), PPERR_CASHSRV_IMPCHECKS);
		}
	}
	CATCHZOK
	ZDELETE(p_qtbl);
	PPWaitMsg(0);
	return ok;
}

SString & ACS_CRCSHSRV::MakeQueryBuf(LDATE dt, SString & rBuf) const
{
	return rBuf.Z().CatLongZ(dt.year(), 4).CatLongZ(dt.month(), 2).CatLongZ(dt.day(), 2);
}

SString & ACS_CRCSHSRV::MakeQueryBufV10(LDATE dt, SString & rBuf, int isZRep) const
{
	return rBuf.Z().Cat("date:").Space().Cat(dt).CR().Cat("report:").Space().Cat((isZRep) ? "Zreports" : "purchases");
}

class XmlZRepReader {
public:
	XmlZRepReader(const char * pPath);
	~XmlZRepReader();
	int    Next(ZRep *);
private:
	long   ZRepsCount;
	xmlDoc  * P_Doc;
	xmlNode * P_CurRec;
	xmlTextReader * P_Reader;
};

XmlZRepReader::XmlZRepReader(const char * pPath) : ZRepsCount(0), P_CurRec(0), P_Doc(0)
{
	if(pPath)
		P_Reader = xmlReaderForFile(pPath, NULL, XML_PARSE_NOENT);
	if(P_Reader) {
		const char * p_chr_tag = "zreport";
		int    r = 0;
		xmlTextReaderPreservePattern(P_Reader, (const xmlChar *)p_chr_tag, 0);
		r = xmlTextReaderRead(P_Reader);
		while(r == 1)
			r = xmlTextReaderRead(P_Reader);
		if(!r) {
			P_Doc = xmlTextReaderCurrentDoc(P_Reader);
			if(P_Doc) {
				xmlNode * p_root = xmlDocGetRootElement(P_Doc);
				if(FindFirstRec(p_root, &P_CurRec, p_chr_tag) > 0 && P_CurRec && sstreqi_ascii((const char *)P_CurRec->name, p_chr_tag)) {
					xmlNode * p_rec = P_CurRec;
					for(ZRepsCount = 1; p_rec = p_rec->next;)
						if(sstreqi_ascii((const char *)p_rec->name, p_chr_tag))
							ZRepsCount++;
				}
			}
		}
	}
}

XmlZRepReader::~XmlZRepReader()
{
	if(P_Reader) {
		xmlFreeTextReader(P_Reader);
		P_Reader = 0;
	}
	xmlFreeDoc(P_Doc);
}

int XmlZRepReader::Next(ZRep * pItem)
{
	int    ok = -1;
	const char * p_tag_names = "shiftNumber;cashNumber;dateShiftClose";
	if(P_CurRec) {
		SString val;
		xmlNode * p_fld = P_CurRec->children;
		ZRep item;
		for(; p_fld; p_fld = p_fld->next) {
			if(p_fld->children && p_fld->children->content) {
				int idx = 0;
				val.Set(p_fld->children->content);
				if(PPSearchSubStr(p_tag_names, &(idx = 0), (const char *)p_fld->name, 1) > 0) {
					switch(idx) {
						case 0: item.ZRepCode = val.ToLong(); break; // Номер смены
						case 1: item.CashCode = val.ToLong(); break; // Номер кассы
						case 2:  // Дата время чека
							strtodatetime(val, &item.Start, DATF_ISO8601, TIMF_HMS);
							break;
					}
				}
			}
		}
		if(item.ZRepCode && item.CashCode)
			ASSIGN_PTR(pItem, item);
		P_CurRec = P_CurRec->next;
		ok = 1;
	}
	return ok;
}

int ACS_CRCSHSRV::ImportZRepList(SVector * pZRepList, bool useLocalFiles)
{
	int    ok = -1, r = 1;
	LDATE  oper_date, end = ChkRepPeriod.upp;
	SString query_buf;
	SETIFZ(end, plusdate(getcurdate_(), 2)); // @v10.8.10 LConfig.OperDate-->getcurdate_()
	DbfTable * p_dbftz  = 0;
	PPImpExp * p_ie_csz = 0;
	for(oper_date = ChkRepPeriod.low; useLocalFiles || (r > 0 && oper_date <= end); oper_date = plusdate(oper_date, 1)) {
		if(useLocalFiles)
			r = 1;
		else {
			if(oper_date > ChkRepPeriod.low)
				SDelay(500);
			CurOperDate = oper_date;
			if(ModuleVer == 10) {
				r = 1;
				// THROW(r = QueryFile(filTypZRepXml, MakeQueryBufV10(oper_date, query_buf, 1)));
			}
			else {
				THROW(r = QueryFile(filTypZRep, MakeQueryBuf(oper_date, query_buf), oper_date));
			}
		}
		if(r > 0) {
			if(ModuleVer == 10) {
				SString data_dir, data_path;
				SDirEntry sd_entry;
				SDirec sd;
				SFsPath sp(PathRpt[filTypZRepXml]);
				sp.Merge(SFsPath::fDrv|SFsPath::fDir, data_dir);
				sp.Nam.Cat("*");
				sp.Merge(data_path);
				for(sd.Init(data_path); sd.Next(&sd_entry) > 0;) {
					sd_entry.GetNameA(data_dir, data_path);
					// @v10.1.1 Backup("zrep", data_path);
					DrfL.Add("zrep", data_path); // @v10.1.1
					{
						ZRep   zrep;
						XmlZRepReader _rdr(data_path);
						while(_rdr.Next(&zrep) > 0) {
							// if(zrep.Start >= ChkRepPeriod.low && zrep.Stop <= ChkRepPeriod.upp) // @todo забирать только за определенную дату
							THROW_SL(pZRepList->insert(&zrep));
						}
					}
					// @v10.1.1 SFile::Remove(data_path);
				}
			}
			else if(Options & oUseAltImport) {
				long   count = 0;
				SString  file_name, save_file_name;
				PPBillImpExpParam * p_ie_param = P_IEParam[0];
				SeparatedFileSet.get(0U, file_name);
				p_ie_param->FileName = file_name;
				THROW_MEM(p_ie_csz = new PPImpExp(p_ie_param, 0));
				THROW(p_ie_csz->OpenFileForReading(0));
				p_ie_csz->GetNumRecs(&count);
				for(long c = 0; c < count; c++) {
					Sdr_CS_ZRep  cs_zrep;
					ZRep   zrep;
					THROW(p_ie_csz->ReadRecord(&cs_zrep, sizeof(cs_zrep)));
					zrep.CashCode = cs_zrep.CashNumber;
					zrep.ZRepCode = cs_zrep.SessNumber;
					GetCrCshSrvDateTime(cs_zrep.BeginDate, 0, &zrep.Start);
					GetCrCshSrvDateTime(cs_zrep.EndDate,   0, &zrep.Stop);
					zrep.ChkFirst = cs_zrep.FirstCheck;
					zrep.ChkLast  = cs_zrep.LastCheck;
					zrep.Status   = cs_zrep.CheckStatus;
					THROW_SL(pZRepList->insert(&zrep));
				}
				ZDELETE(p_ie_csz);
				save_file_name = PathRpt[filTypZRep];
				ReplaceFilePath(save_file_name, file_name);
				SFile::Rename(file_name, save_file_name);
			}
			else {
				//
				// Коды полей входного файла Z-отчетов
				//
				int    fldn_z_cash     = 0;
				int    fldn_z_sess     = 0;
				int    fldn_z_start    = 0;
				int    fldn_z_stop     = 0;
				int    fldn_z_chkfirst = 0;
				int    fldn_z_chklast  = 0;
				int    fldn_z_status   = 0;
				//
				Backup("zrep", PathRpt[filTypZRep]);
				THROW_MEM(p_dbftz = new DbfTable(PathRpt[filTypZRep]));
				THROW_PP_S(p_dbftz->isOpened(), PPERR_DBFOPFAULT, PathRpt[filTypZRep]);
				p_dbftz->getFieldNumber("cashnmb",   &fldn_z_cash);
				p_dbftz->getFieldNumber("smena",     &fldn_z_sess);
				p_dbftz->getFieldNumber("start",     &fldn_z_start);
				p_dbftz->getFieldNumber("stop",      &fldn_z_stop);
				p_dbftz->getFieldNumber("chk_first", &fldn_z_chkfirst);
				p_dbftz->getFieldNumber("chk_last",  &fldn_z_chklast);
				p_dbftz->getFieldNumber("status",    &fldn_z_status);
				if(p_dbftz->getNumRecs() && p_dbftz->top()) {
					do {
						char   buf[64];
						ZRep   zrep;
						DbfRecord dbfrz(p_dbftz);
						if(p_dbftz->getRec(&dbfrz) <= 0)
							break;
						dbfrz.get(fldn_z_cash,  zrep.CashCode);
						dbfrz.get(fldn_z_sess,  zrep.ZRepCode);
						dbfrz.get(fldn_z_start, buf, sizeof(buf));
						GetCrCshSrvDateTime(buf, 0, &zrep.Start);
						dbfrz.get(fldn_z_stop, buf, sizeof(buf));
						GetCrCshSrvDateTime(buf, 0, &zrep.Stop);
						dbfrz.get(fldn_z_chkfirst, zrep.ChkFirst);
						dbfrz.get(fldn_z_chklast,  zrep.ChkLast);
						dbfrz.get(fldn_z_status,   zrep.Status);
						THROW_SL(pZRepList->insert(&zrep));
					} while(p_dbftz->next());
				}
				ZDELETE(p_dbftz);
			}
		}
		if(useLocalFiles)
			break;
	}
	pZRepList->sort(PTR_CMPFUNC(_2long));
	if(r > 0)
		ok = 1;
	CATCHZOK
	delete p_dbftz;
	delete p_ie_csz;
	return ok;
}

void ACS_CRCSHSRV::Backup(const char * pPrefix, const char * pPath)
{
	const long _max_copies = 10L; //#define MAX_COPIES 10L
	long   start = 1L;
	SString backup_dir, dest_path;
	SString prefix(pPrefix);
	prefix.Strip().Trim(4);
	SFsPath sp(pPath);
	SString ext = sp.Ext;
	sp.Merge(SFsPath::fDrv|SFsPath::fDir, backup_dir);
	backup_dir.Cat("backup").SetLastSlash();
	SFile::CreateDir(backup_dir);
	dest_path = MakeTempFileName(backup_dir, prefix, ext, &start, dest_path);
	if(start > (_max_copies + 1)) {
		const size_t pfx_len = prefix.Len();
		SString prev_path, path;
		for(long i = 1; i < _max_copies; i++) {
			(path = backup_dir).Cat(prefix).CatLongZ(i, (int)(8 - pfx_len)).Dot().Cat(ext);
			(prev_path = backup_dir).Cat(prefix).CatLongZ(i + 1, int(8 - pfx_len)).Dot().Cat(ext);
			SFile::Remove(path);
			SCopyFile(prev_path, path, 0, FILE_SHARE_READ, 0);
		}
		(dest_path = backup_dir).Cat(prefix).CatLongZ(_max_copies, (int)(8 - pfx_len)).Dot().Cat(ext);
		SFile::Remove(dest_path);
		dest_path = MakeTempFileName(backup_dir, prefix, ext, &(start = 10), dest_path);
	}
	SCopyFile(pPath, dest_path, 0, FILE_SHARE_READ, 0);
}

int ACS_CRCSHSRV::ImportSession(int)
{
	int    ok = -1;
	int    r = 1;
	const  bool use_local_files = GetFilesLocal();
	SString wait_msg_tmpl;
	SString wait_msg;
	SString query_buf;
	LDATE  oper_date;
	LDATE  end = ChkRepPeriod.upp;
	SVector zrep_list(sizeof(ZRep));
	SETIFZ(end, plusdate(getcurdate_(), 2)); // @v10.8.10 LConfig.OperDate-->getcurdate_()
	PPLoadText(PPTXT_IMPORTCHECKS, wait_msg_tmpl);
	THROW(CreateTables());
	//
	// Все Z-отчеты мы должны получить до того, как начнем обрабатывать чеки,
	// поскольку в противном случае может получиться так, что мы увидим чек,
	// не увидев Z-отчет, и не сбросим чек в систему.
	//
	THROW(r = ImportZRepList(&zrep_list, use_local_files));
	if(r > 0) {
		SString data_dir;
		SString data_path;
		AcceptedCheckList.freeAll();
		for(oper_date = ChkRepPeriod.low; use_local_files || r > 0 && oper_date <= end; oper_date = plusdate(oper_date, 1)) {
			char   date_buf[32];
			wait_msg.Printf(wait_msg_tmpl, datefmt(&oper_date, DATF_DMY, date_buf));
			if(ModuleVer == 10) {
				SDirEntry sd_entry;
				SFsPath sp(PathRpt[filTypChkXml]);
				sp.Merge(SFsPath::fDrv|SFsPath::fDir, data_dir);
				sp.Nam.Cat("*");
				sp.Merge(data_path);
				for(SDirec sd(data_path); sd.Next(&sd_entry) > 0;) {
					sd_entry.GetNameA(data_dir, data_path);
					// @v10.1.1 Backup("chks", data_path);
					DrfL.Add("chks", data_path); // @v10.1.1
					THROW(ConvertWareListV10(&zrep_list, data_path, wait_msg));
					// @v10.1.1 SFile::Remove(data_path);
				}
				/*
				MakeQueryBufV10(oper_date, query_buf, 0);
				if(r > 0) {
					if(!files_local) {
						SDelay(2000);
						if(r > 0)
							THROW(r = QueryFile(filTypChkXml, query_buf));
						THROW(ConvertWareListV10(&zrep_list, PathRpt[filTypChkXml], wait_msg));
					}
				}
				*/
			}
			else {
				MakeQueryBuf(oper_date, query_buf);
				if(Options & oUseAltImport) {
					CurOperDate = oper_date;
					SDelay(2000);
					THROW(r = QueryFile(filTypChkDscnt, query_buf, oper_date));
					if(r > 0) {
						THROW(CreateSCardPaymTbl());
						THROW(r = QueryFile(filTypChkHeads, query_buf, oper_date));
					}
					if(r > 0) {
						THROW(ConvertCheckHeads(&zrep_list, wait_msg));
						THROW(r = QueryFile(filTypChkRows, query_buf, oper_date));
					}
					if(r > 0)
						THROW(ConvertCheckRows(wait_msg));
				}
				else {
					if(!use_local_files) {
						if(!(ModuleVer == 5 && ModuleSubVer >= 9)) { // @v9.2.7 В режиме 5.9 при посылке запроса на отчеты SetRetail возвращает все отчеты
							SDelay(2000);
							if(r > 0)
								THROW(r = QueryFile(filTypChkHeads, query_buf, oper_date));
							if(r > 0)
								THROW(r = QueryFile(filTypChkRows, query_buf, oper_date));
							if(r > 0)
								THROW(r = QueryFile(filTypChkDscnt, query_buf, oper_date));
						}
					}
					else
						oper_date = end;
					if(r > 0) {
						THROW(CreateSCardPaymTbl());
						THROW(ConvertWareList(&zrep_list, wait_msg));
						ZDELETE(P_SCardPaymTbl);
					}
					if(use_local_files)
						break;
				}
			}
		}
		if(r > 0) {
			if(ModuleVer == 10) {
				//
				// Сбрасываем признак Temporary с завершенных сессий
				//
				PPTransaction tra(1);
				THROW(tra);
				for(uint i = 0; i < zrep_list.getCount(); i++) {
					PPID   sess_id = 0;
					const ZRep * p_hdr = static_cast<const ZRep *>(zrep_list.at(i));
					if(p_hdr && CS.SearchByNumber(&sess_id, NodeID, p_hdr->CashCode, p_hdr->ZRepCode, p_hdr->Start.d) > 0 && sess_id && CS.data.Temporary) {
						THROW(CS.ResetTempSessTag(sess_id, 0));
						SessAry.addUnique(sess_id);
					}
				}
				THROW(tra.Commit());
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int ACS_CRCSHSRV::FinishImportSession(PPIDArray * pSessList)
{
	return pSessList->addUnique(&SessAry);
}

void ACS_CRCSHSRV::CleanUpSession()
{
	const uint c = DrfL.GetCount();
	SString backup_prefix, path;
	for(uint i = 0; i < c; i++) {
		if(DrfL.Get(i, backup_prefix, path)) {
			if(fileExists(path)) {
				Backup(backup_prefix, path);
				SFile::Remove(path);
			}
		}
	}
}
