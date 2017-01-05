// CPT720.CPP
//
#include <slib.h>
#include <cpt720\cptprot.h>
#include <cpt720\registry.h>
#include <cpt720\cpt720.h>
//#include <winreg.h>

HRESULT CPT720ErrCode = 0;

static HMODULE g_hModule = NULL;  // @global описатель модуля DLL
static long g_cComponents = 0;    // @global счетчик активных компонентов
static long g_cServerLocks = 0;   // @global счетчик блокировок

// Дружественное имя компонента
const char g_szFriendlyName[] = "Transmit module for Cipher720";
// Не зависящий от версии ProgID
const char g_szVerIndProgID[] = "TModuleForCipher720";
// ProgID
const char g_szProgID[] = "TModuleForCipher720.1";

//
// Интерфейс для обмена данными с Cipher 720 терминалом
//
class CPT720 : public CPT720Interface {
public:
	CPT720()
	{
		CRef = 1;
		g_cComponents++;
		SetComPortParams(cbr19200, 8, 0, 0);
		SetComPortTimeouts(2000, 1, 400, 5, 300);
		SetTransmitParams(3000, 10);
	}
	~CPT720()
	{
		g_cComponents--;
	}
	virtual HRESULT __stdcall QueryInterface(const IID &iID, void ** ppV);
	virtual ULONG   __stdcall AddRef();
	virtual ULONG   __stdcall Release();

	virtual HRESULT __stdcall SetComPortParams(__int16 cbr, __int8 byteSize, __int8 parity, __int8 stopBits);
	virtual HRESULT __stdcall SetComPortTimeouts(unsigned __int16 getNumTries, unsigned __int16 getDelay,
					unsigned __int16 putNumTries, unsigned __int16 putDelay, unsigned __int16 wGetDelay);
	virtual HRESULT __stdcall SetTransmitParams(unsigned __int16 timeout, unsigned __int16 maxTries);

	virtual HRESULT __stdcall SendGoodsData(int comPort, const char * pPath, CallbackPrctFunc);
	virtual HRESULT __stdcall SendSupplData(int comPort, const char * pPath, CallbackPrctFunc);
	virtual HRESULT __stdcall ReceiveFiles(int comPort, const char * pBillPath, const char * pBLinePath,
					const char * pInvPath, const char * pILinePath);
private:
	int  InitComPort(int comPort);
	int  AcceptBills(const char * pBillInFile, const char * pBLineInFile, const char * pBillOutFile, const char * pBLineOutFile);
	int  AcceptInvent(const char * pInvInFile, const char * pILineInFile, const char * pInvOutFile, const char * pILineOutFile);
	int  CreateBillTbl(DbfTable * p_tbl);
	int  CreateBLineTbl(DbfTable * p_tbl);
	int  CreateInventTbl(DbfTable * p_tbl);
	int  CreateILineTbl(DbfTable * p_tbl);
	int  PrepareFileToSend(const char * pPath, CallbackPrctFunc, int supplFile);
	int  FromDbfToBht(DbfRecord * pDbfr, BhtRecord * pBhtRec, int supplFile);
	int  InitGoodsBhtRec(BhtRecord * pRec);
	int  InitSupplBhtRec(BhtRecord * pRec);
	int  GetFilePath(const char * pInPath, char * pBuf, size_t bufSize, int supplFile);

	CipherProtocol CP;
	long CRef;
};

//
// Фабрика класса
//
class CFactory : public IClassFactory {
public:
	CFactory() {CRef = 1;}
	~CFactory() {}
	virtual HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
	virtual ULONG   __stdcall AddRef();
	virtual ULONG   __stdcall Release();

	virtual HRESULT __stdcall CreateInstance(IUnknown *pUnknownOuter, const IID & iID, void ** ppV);
	virtual HRESULT __stdcall LockServer(BOOL bLock);
private:
	long CRef;
};

//
// CPT720
//
// virtual
HRESULT __stdcall CPT720::QueryInterface(const IID &iID, void **ppV)
{
	HRESULT ok = S_OK;
	if(iID == IID_IUnknown)
		*ppV = (CPT720Interface *)(this);
	else if(iID == IID_CPT720Interface)
		*ppV = (CPT720Interface *)(this);
	else
		ok = E_NOINTERFACE;
	if(ok == S_OK)
		((IUnknown *)(*ppV))->AddRef();
	return ok;
}

// virtual
ULONG __stdcall CPT720::AddRef()
{
	return ++CRef;
}

// virtual
ULONG __stdcall CPT720::Release()
{
	ULONG r = CRef;
	if(--CRef == 0) {
		r = 0;
		delete this;
	}
	return r;
}

//virtual
HRESULT __stdcall CPT720::SetComPortParams(__int16 cbr, __int8 byteSize, __int8 parity, __int8 stopBits)
{
	HRESULT ok = S_OK;
	CommPortParams cpp;

	cpp.Cbr = cbr;
	cpp.ByteSize = byteSize;
	cpp.Parity = parity;
	cpp.StopBits = stopBits;
	CP.SetParams(&cpp);
	return ok;
}

//virtual
HRESULT __stdcall CPT720::SetComPortTimeouts(unsigned __int16 getNumTries, unsigned __int16 getDelay,
					unsigned __int16 putNumTries, unsigned __int16 putDelay, unsigned __int16 wGetDelay)
{
	HRESULT ok = S_OK;
	CommPortTimeouts cpt;
	cpt.Get_NumTries = getNumTries;
	cpt.Get_Delay = getDelay;
	cpt.Put_NumTries = putNumTries;
	cpt.Put_Delay = putDelay;
	cpt.W_Get_Delay = wGetDelay;
	CP.SetTimeouts(&cpt);
	return ok;
}

//virtual
HRESULT __stdcall CPT720::SetTransmitParams(unsigned __int16 timeout, unsigned __int16 maxTries)
{
	HRESULT ok = S_OK;
	CP.SetProtParams(timeout, maxTries, 0);
	return ok;
}

// virtual
HRESULT __stdcall CPT720::SendGoodsData(int comPort, const char * pPath, CallbackPrctFunc pf)
{
	char path[MAXPATH];
	HRESULT ok = S_OK;
	BhtRecord * p_bht_rec = 0;

	THROW_CPT(InitComPort(comPort), CPT720_ERR_INITCOMPORT);
	THROW_CMEM(p_bht_rec = new BhtRecord);
	THROW(PrepareFileToSend(pPath, pf, 0));
	InitGoodsBhtRec(p_bht_rec);
	THROW(GetFilePath(pPath, path, sizeof(path), 0));
	THROW(CP.SendFile(path, p_bht_rec, pf));
	CATCH
		ok = CPT720ErrCode;
	ENDCATCH
	delete p_bht_rec;
	return ok;
}

// virtual
HRESULT __stdcall CPT720::SendSupplData(int comPort, const char * pPath, CallbackPrctFunc pf)
{
	char path[MAXPATH];
	HRESULT ok = S_OK;
	BhtRecord * p_bht_rec = 0;

	THROW_CPT(InitComPort(comPort), CPT720_ERR_INITCOMPORT);
	THROW_CMEM(p_bht_rec = new BhtRecord);
	THROW(PrepareFileToSend(pPath, pf, 1));
	InitSupplBhtRec(p_bht_rec);
	THROW(GetFilePath(pPath, path, sizeof(path), 1));
	THROW(CP.SendFile(path, p_bht_rec, pf));
	CATCH
		ok = CPT720ErrCode;
	ENDCATCH
	delete p_bht_rec;
	return ok;
}

// virtual
HRESULT __stdcall CPT720::ReceiveFiles(int comPort, const char * pBillPath, const char * pBLinePath,
			const char * pInvPath, const char * pILinePath)
{
	HRESULT ok = S_OK;
	int r = 0;
	int fi_bill = -1, fi_line  = -1;
	int fi_inv  = -1, fi_iline = -1;
	uint i = 0;
	SStrCollection files;

	if(pBillPath && pBLinePath && pInvPath && pILinePath) {
		char   dir[MAXPATH], drv[10];
		SString path;
		long   s = 1, timeout = 30000L;
		THROW_CPT(InitComPort(comPort), CPT720_ERR_INITCOMPORT);
		fnsplit(pBillPath, drv, dir, 0, 0);
		(path = drv).Cat(dir).SetLastSlash().CopyTo(dir, sizeof(dir));
		do {
			MakeTempFileName(dir, "BHT", "DAT", &s, path);
			THROW((r = CP.ReceiveFile(path, timeout)));
			if(r > 0) {
				files.insert(newStr(path));
				timeout = 2000L;
			}
			else
				SFile::Remove(path);
		} while(r > 0);
		for(i = 0; i < files.getCount(); i++) {
			BhtFile bf(files.at(i));
			if(stricmp(bf.Name, CPT720_FILNAM_BILL) == 0)
				fi_bill = i;
			else if(stricmp(bf.Name, CPT720_FILNAM_BLINE) == 0)
				fi_line = i;
			else if(stricmp(bf.Name, CPT720_FILNAM_INVENT) == 0)
				fi_inv = i;
			else if(stricmp(bf.Name, CPT720_FILNAM_ILINE) == 0)
				fi_iline = i;
		}
		if(fi_bill >= 0 && fi_line >= 0) {
			THROW(AcceptBills(files.at(fi_bill), files.at(fi_line), pBillPath, pBLinePath));
		}
		if(fi_inv >= 0 && fi_iline >= 0) {
			THROW(AcceptInvent(files.at(fi_inv), files.at(fi_iline), pInvPath, pILinePath));
		}
	}
	else
		ok = CPT720_ERR_CANTOPENFILE;
	CATCH
		ok = CPT720ErrCode;
		if(pBillPath)
			::remove(pBillPath);
		if(pBLinePath)
			::remove(pBLinePath);
		if(pInvPath)
			::remove(pInvPath);
		if(pILinePath)
			::remove(pILinePath);
	ENDCATCH
	for(i = 0; i < files.getCount(); i++)
		::remove(files.at(i));
	return ok;
}

int CPT720::InitComPort(int comPort)
{
	return CP.InitPort(comPort);
}

int CPT720::AcceptBills(const char * pBillInFile, const char * pBLineInFile, const char * pBillOutFile, const char * pBLineOutFile)
{
	int    ok = 1;
	char   path[MAXPATH];
	uint   bi, li;
	DbfTable * p_bill_out = 0, * p_bline_out = 0;
	BhtFile bf_bill(pBillInFile);
	BhtFile bf_line(pBLineInFile);
	BhtRecord * p_br_bill = 0;
	BhtRecord * p_br_line = 0;

	STRNSCPY(path, pBillOutFile);
	THROW_CMEM(p_bill_out = new DbfTable(path));
	STRNSCPY(path, pBLineOutFile);
	THROW_CMEM(p_bline_out = new DbfTable(path));
	THROW(CreateBillTbl(p_bill_out));
	THROW(CreateBLineTbl(p_bline_out));

	THROW_CMEM(p_br_bill = new BhtRecord);
	THROW_CMEM(p_br_line = new BhtRecord);

	THROW_CPT(bf_bill.InitRecord(p_br_bill), E_FAIL);
	THROW_CPT(bf_line.InitRecord(p_br_line), E_FAIL);
	THROW_CPT(p_bill_out->open(), CPT720_ERR_DBFOPENFLT);
	THROW_CPT(p_bline_out->open(), CPT720_ERR_DBFOPENFLT);
	for(bi = 0; bf_bill.EnumRecords(&bi, p_br_bill) > 0;) {
		DbfRecord dbfr_b(p_bill_out);
		long _bid = 0;
		char bid[12], bdate[16];
		long suppl_id = 0;
		LDATE dt = ZERODATE;

		p_br_bill->GetStr(0, bid, sizeof(bid));
		p_br_bill->GetStr(1, bdate, sizeof(bdate));
		p_br_bill->GetInt(2, &suppl_id);

		_bid = atol(bid);
		dbfr_b.empty();
		dbfr_b.put(1, _bid);
		strtodate(bdate, DATF_DMY, &dt);
		dbfr_b.put(2, dt);
		dbfr_b.put(3, suppl_id);
		THROW_CPT(p_bill_out->appendRec(&dbfr_b), CPT720_ERR_DBFWRFAULT);

		for(li = 0; bf_line.EnumRecords(&li, p_br_line) > 0;) {
			char   lbid[12], gid[16], str_price[8], expiry[10];
			long   goods_id;
			double qtty, price;
			DbfRecord dbfr_bl(p_bline_out);

			p_br_line->GetStr(0, lbid, sizeof(lbid));
			p_br_line->GetStr(1, gid, sizeof(gid));
			p_br_line->GetDbl(2, &qtty);
			p_br_line->GetStr(3, str_price, sizeof(str_price));
			p_br_line->GetStr(4, expiry, sizeof(expiry));

			goods_id = atol(gid);
			price = atof(str_price);
			if(strcmp(lbid, bid) == 0) {
				dbfr_bl.empty();
				dbfr_bl.put(1, _bid);
				dbfr_bl.put(2, goods_id);
				dbfr_bl.put(3, qtty);
				dbfr_bl.put(4, price);
				strtodate(expiry, DATF_DMY, &dt);
				dbfr_bl.put(5, dt);
				THROW_CPT(p_bline_out->appendRec(&dbfr_bl), CPT720_ERR_DBFWRFAULT);
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	if(p_bill_out && p_bill_out->isOpened())
		p_bill_out->close();
	if(p_bline_out && p_bline_out->isOpened())
		p_bline_out->close();
	delete p_bill_out;
	delete p_bline_out;
	return ok;
}

int CPT720::AcceptInvent(const char * pInvInFile, const char * pILineInFile, const char * pInvOutFile, const char * pILineOutFile)
{
	int ok = 1;
	char path[MAXPATH];
	uint   bi, li;
	DbfTable * p_inv_out = 0, * p_iline_out = 0;
	BhtFile bf_inv(pInvInFile);
	BhtFile bf_line(pILineInFile);
	BhtRecord * p_br_inv = 0;
	BhtRecord * p_br_line = 0;

	STRNSCPY(path, pInvOutFile);
	THROW_CMEM(p_inv_out = new DbfTable(path));
	STRNSCPY(path, pILineOutFile);
	THROW_CMEM(p_iline_out = new DbfTable(path));
	THROW(CreateInventTbl(p_inv_out));
	THROW(CreateILineTbl(p_iline_out));

	THROW_CMEM(p_br_inv = new BhtRecord);
	THROW_CMEM(p_br_line = new BhtRecord);

	THROW_CPT(bf_inv.InitRecord(p_br_inv), E_FAIL);
	THROW_CPT(bf_line.InitRecord(p_br_line), E_FAIL);
	THROW_CPT(p_inv_out->open(), CPT720_ERR_DBFOPENFLT);
	THROW_CPT(p_iline_out->open(), CPT720_ERR_DBFOPENFLT);
	for(bi = 0; bf_inv.EnumRecords(&bi, p_br_inv) > 0;) {
		DbfRecord dbfr_i(p_inv_out);
		long _bid = 0;
		char bid[12], bdate[16];
		LDATE dt = ZERODATE;

		p_br_inv->GetStr(0, bid, sizeof(bid));
		p_br_inv->GetStr(1, bdate, sizeof(bdate));

		_bid = atol(bid);
		dbfr_i.empty();
		dbfr_i.put(1, _bid);
		strtodate(bdate, DATF_DMY, &dt);
		dbfr_i.put(2, dt);
		THROW_CPT(p_inv_out->appendRec(&dbfr_i), CPT720_ERR_DBFWRFAULT);

		for(li = 0; bf_line.EnumRecords(&li, p_br_line) > 0;) {
			char   lbid[12], gid[16], str_price[8];
			long   goods_id;
			double qtty, price;
			DbfRecord dbfr_il(p_iline_out);

			p_br_line->GetStr(0, lbid, sizeof(lbid));
			p_br_line->GetStr(1, gid, sizeof(gid));
			p_br_line->GetDbl(2, &qtty);
			p_br_line->GetStr(3, str_price, sizeof(str_price));

			goods_id = atol(gid);
			price = atof(str_price);
			if(strcmp(lbid, bid) == 0) {
				dbfr_il.empty();
				dbfr_il.put(1, _bid);
				dbfr_il.put(2, goods_id);
				dbfr_il.put(3, qtty);
				dbfr_il.put(4, price);
				THROW_CPT(p_iline_out->appendRec(&dbfr_il), CPT720_ERR_DBFWRFAULT);
			}
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	if(p_inv_out && p_inv_out->isOpened())
		p_inv_out->close();
	if(p_iline_out && p_iline_out->isOpened())
		p_iline_out->close();
	delete p_inv_out;
	delete p_iline_out;
	return ok;
}

int CPT720::CreateBillTbl(DbfTable * p_tbl)
{
	int    ok = 0;
	DBFCreateFld * p_dbf_flds = 0;
	THROW_CPT(p_tbl, CPT720_ERR_DBFOPENFLT);
	THROW_CMEM(p_dbf_flds = new DBFCreateFld[DBF_BILL_NUMFLDS]);
	p_dbf_flds[0].Init("BILLID",   'N', 10, 0);
	p_dbf_flds[1].Init("BILLDATE", 'D',  8, 0);
	p_dbf_flds[2].Init("SUPPLID",  'N', 10, 0);
	THROW_CPT(p_tbl->create(DBF_BILL_NUMFLDS, p_dbf_flds), CPT720_ERR_ERRCRTTBL);
	ok = 1;
	CATCH
		ok = 0;
	ENDCATCH
	ZDELETE(p_dbf_flds);
	return ok;
}

int CPT720::CreateBLineTbl(DbfTable * p_tbl)
{
	int    ok = 0;
	DBFCreateFld * p_dbf_flds = 0;
	THROW_CPT(p_tbl, CPT720_ERR_DBFOPENFLT);
	THROW_CMEM(p_dbf_flds = new DBFCreateFld[DBF_BLINE_NUMFLDS]);
	p_dbf_flds[0].Init("BILLID",  'N', 10, 0);
	p_dbf_flds[1].Init("GOODSID", 'N', 10, 0);
	p_dbf_flds[2].Init("QTTY",    'N', 10, 2);
	p_dbf_flds[3].Init("PRICE",   'N', 10, 2);
	p_dbf_flds[4].Init("EXPIRY",  'D',  8, 0);
	THROW_CPT(p_tbl->create(DBF_BLINE_NUMFLDS, p_dbf_flds), CPT720_ERR_ERRCRTTBL);
	ok = 1;
	CATCH
		ok = 0;
	ENDCATCH
	ZDELETE(p_dbf_flds);
	return ok;
}

int CPT720::CreateInventTbl(DbfTable * p_tbl)
{
	int    ok = 0;
	DBFCreateFld * p_dbf_flds = 0;
	THROW_CPT(p_tbl, CPT720_ERR_DBFOPENFLT);
	THROW_CMEM(p_dbf_flds = new DBFCreateFld[DBF_INVENT_NUMFLDS]);
	p_dbf_flds[0].Init("BILLID",   'N', 10, 0);
	p_dbf_flds[1].Init("BILLDATE", 'D',  8, 0);
	THROW_CPT(p_tbl->create(DBF_INVENT_NUMFLDS, p_dbf_flds), CPT720_ERR_ERRCRTTBL);
	ok = 1;
	CATCH
		ok = 0;
	ENDCATCH
	ZDELETE(p_dbf_flds);
	return ok;
}

int CPT720::CreateILineTbl(DbfTable * p_tbl)
{
	int    ok = 0;
	DBFCreateFld * p_dbf_flds = 0;
	THROW_CPT(p_tbl, CPT720_ERR_DBFOPENFLT);
	THROW_CMEM(p_dbf_flds = new DBFCreateFld[DBF_ILINE_NUMFLDS]);
	p_dbf_flds[0].Init("BILLID",  'N', 10, 0);
	p_dbf_flds[1].Init("GOODSID", 'N', 10, 0);
	p_dbf_flds[2].Init("QTTY",    'N', 10, 2);
	p_dbf_flds[3].Init("PRICE",   'N', 10, 2);
	THROW_CPT(p_tbl->create(DBF_ILINE_NUMFLDS, p_dbf_flds), CPT720_ERR_ERRCRTTBL);
	ok = 1;
	CATCH
		ok = 0;
	ENDCATCH
	ZDELETE(p_dbf_flds);
	return ok;
}

int CPT720::PrepareFileToSend(const char * pPath, CallbackPrctFunc pf, int supplFile)
{
	int    ok = 1;
	char   path[MAXPATH];
	char   preparing_info[256];
	char   name[MAXFILE], ext[MAXEXT];
	ulong  i = 0, num_recs = 0;
	BhtRecord * p_bht_rec = 0;
	DbfTable * p_in_tbl = 0;
	FILE * stream = 0;

	THROW_CPT(pPath, CPT720_ERR_CANTOPENFILE);
	STRNSCPY(path, pPath);
	fnsplit(path, 0, 0, name, ext);
	sprintf(preparing_info, "Preparing %s%s", name, ext);
	THROW_CMEM(p_in_tbl = new DbfTable(path));
	THROW_CMEM(p_bht_rec = new BhtRecord);

	THROW(GetFilePath(pPath, path, sizeof(path), supplFile));
	THROW_CPT(stream = fopen(path, "w"), CPT720_ERR_CANTOPENFILE);
	if(supplFile)
		InitSupplBhtRec(p_bht_rec);
	else
		InitGoodsBhtRec(p_bht_rec);
	THROW_CPT(p_in_tbl->open(), CPT720_ERR_DBFOPENFLT);
	p_in_tbl->top();
	num_recs = p_in_tbl->getNumRecs();
	for(i = 0; i < num_recs; i++) {
		DbfRecord dbfr(p_in_tbl);
		dbfr.empty();
		p_in_tbl->getRec(&dbfr);
		FromDbfToBht(&dbfr, p_bht_rec, supplFile);
		PutBhtRecToFile(p_bht_rec, stream);
		p_in_tbl->next();
		if(pf)
			pf(i+1, num_recs, preparing_info, sizeof(preparing_info));
	}
	CATCH
		ok = 0;
	ENDCATCH
	if(p_in_tbl && p_in_tbl->isOpened())
		p_in_tbl->close();
	delete p_bht_rec;
	delete p_in_tbl;
	if(stream)
		fclose(stream);
	return ok;
}

int CPT720::FromDbfToBht(DbfRecord * pDbfr, BhtRecord * pBhtRec, int supplFile)
{
	int ok = 0;
	if(pDbfr && pBhtRec) {
		if(supplFile) {
			long suppl_id = 0;
			char suppl_name[58];
			pDbfr->get(1, suppl_id);
			pDbfr->get(2, suppl_name);
			pBhtRec->PutInt(0, suppl_id);
			pBhtRec->PutStr(1, suppl_name);
		}
		else {
			long goods_id = 0;
			char barcode[32];
			pDbfr->get(1, goods_id);
			pDbfr->get(2, barcode);
			pBhtRec->PutInt(0, goods_id);
			pBhtRec->PutStr(1, barcode);
		}
		ok = 1;
	}
	return ok;
}

int CPT720::InitGoodsBhtRec(BhtRecord * pRec)
{
	pRec->Reset();
	pRec->AddFld(8);  // GoodsID
	pRec->AddFld(14); // Barcode
	return 1;
}

int CPT720::InitSupplBhtRec(BhtRecord * pRec)
{
	pRec->Reset();
	pRec->AddFld(8);  // SupplID
	pRec->AddFld(30); // Name
	return 1;
}

int CPT720::GetFilePath(const char * pInPath, char * pBuf, size_t bufSize, int supplFile)
{
	int ok = 1;
	char drv[10], dir[MAXPATH], path[MAXPATH];

	THROW_CPT(pInPath, CPT720_ERR_CANTOPENFILE);
	fnsplit(pInPath, drv, dir, 0, 0);
	sprintf(path, "%s%s", drv, dir);
	setLastSlash(path);
	if(supplFile)
		strcat(path, CPT720_FILNAM_SUPPL);
	else
		strcat(path, CPT720_FILNAM_GOODS);
	strnzcpy(pBuf, path, bufSize);
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

//
// CFactory
//
// virtual
HRESULT __stdcall CFactory::QueryInterface(const IID & iID, void ** ppV)
{
	HRESULT ok = S_OK;
	if((iID == IID_IUnknown) || (iID == IID_IClassFactory))
		*ppV = (IClassFactory*)(this);
	else {
		*ppV = NULL;
		ok = E_NOINTERFACE;
	}
	if(ok == S_OK)
		((IUnknown*)(*ppV))->AddRef();
	return ok;

}

// virtual
ULONG __stdcall CFactory::AddRef()
{
	return ++CRef;
}

// virtual
ULONG __stdcall CFactory::Release()
{
	ULONG r = CRef;
	if(--CRef == 0) {
		r = 0;
		delete this;
	}
	return r;
}

// virtual
HRESULT __stdcall CFactory::CreateInstance(IUnknown * pUnknownOuter, const IID &iID, void ** ppV)
{
	CPT720 * p_com = 0;
	HRESULT ok = S_OK;
	THROW_CPT(pUnknownOuter == NULL, CLASS_E_NOAGGREGATION);
	if(ok == S_OK) {
		THROW_CMEM(p_com = new CPT720);
		ok = p_com->QueryInterface(iID, ppV);
		p_com->Release();
	}
	CATCH
		ok = CPT720ErrCode;
	ENDCATCH
	return ok;
}

// virtual
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
	g_cServerLocks = bLock ? g_cServerLocks + 1 : g_cServerLocks - 1;
	return S_OK;
}

//
// Экспортируемые функции
//
STDAPI DllCanUnloadNow()
{
	return ((g_cComponents == 0) && (g_cServerLocks == 0)) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(const CLSID &clsid, const IID & iID, void ** ppV)
{
	HRESULT ok = S_OK;
	CFactory *p_factory = 0;
	THROW_CPT(clsid == CLSID_CPT720, CLASS_E_CLASSNOTAVAILABLE);
	THROW_CMEM(p_factory = new CFactory);
	ok = p_factory->QueryInterface(iID, ppV);
	p_factory->Release();
	CATCH
		ok = CPT720ErrCode;
	ENDCATCH
	return ok;
}

STDAPI DllRegisterServer()
{
	return RegisterServer(g_hModule, CLSID_CPT720, g_szFriendlyName, g_szVerIndProgID, g_szProgID);
}

STDAPI DllUnregisterServer()
{
	return UnregisterServer(CLSID_CPT720, g_szVerIndProgID, g_szProgID);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason) {
		case DLL_PROCESS_ATTACH:
				g_hModule = (HMODULE)hModule;
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
