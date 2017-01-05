#ifndef __PPYDLL_H
#define __PPYDLL_H

#include <objbase.h>
#include <initguid.h>
#include <pp.h>

//
// GUID defines
//
// {23D81E14-B7F0-46b1-825B-4140218320E7}
DEFINE_GUID(IID_PPYSESSION, 0x23d81e14, 0xb7f0, 0x46b1, 0x82, 0x5b, 0x41, 0x40, 0x21, 0x83, 0x20, 0xe7);

// {A4C780FE-9EB9-4d2c-B632-407454A340BF}
DEFINE_GUID(IID_PPYVIEW, 0xa4c780fe, 0x9eb9, 0x4d2c, 0xb6, 0x32, 0x40, 0x74, 0x54, 0xa3, 0x40, 0xbf);

// {F4A74C97-816E-46e3-8AC4-FDBDF7A75AA7}
DEFINE_GUID(IID_PPYBHT, 
0xf4a74c97, 0x816e, 0x46e3, 0x8a, 0xc4, 0xfd, 0xbd, 0xf7, 0xa7, 0x5a, 0xa7);

// {1137ABAB-3D66-4d32-AA79-D98A8C20084C}
DEFINE_GUID(CLSID_PPYSESSION, 
0x1137abab, 0x3d66, 0x4d32, 0xaa, 0x79, 0xd9, 0x8a, 0x8c, 0x20, 0x8, 0x4c);

// {BDB09559-FAFD-462f-B42C-3F29C426D7E9}
DEFINE_GUID(CLSID_PPYVIEWOPKIND, 
0xbdb09559, 0xfafd, 0x462f, 0xb4, 0x2c, 0x3f, 0x29, 0xc4, 0x26, 0xd7, 0xe9);

// {86B50553-5ACA-49cb-ABDF-A4ABEFDA8AFE}
DEFINE_GUID(CLSID_PPYBHT, 
0x86b50553, 0x5aca, 0x49cb, 0xab, 0xdf, 0xa4, 0xab, 0xef, 0xda, 0x8a, 0xfe);

// {52D5E7CA-F613-4333-A04E-125DE29D715F}
DEFINE_GUID(CLSID_PPYDLLTLIB,
0x52d5e7ca, 0xf613, 0x4333, 0xa0, 0x4e, 0x12, 0x5d, 0xe2, 0x9d, 0x71, 0x5f);
//
// Фильтры
//
struct PpyOpKindFilt {
	long   OpTypeID;
	long   LinkOpID;
	long   AccSheetID;
	long   Flags;
	long   SortOrd;
};
//
// Записи
//
struct PpyBhtGoodsRec {
	long   ID;
	BSTR   Barcode;
	BSTR   Name;
	double UnitPerPack;
	double Price;
};

struct PpyBhtSupplRec {
	long ID;
	BSTR Name;
};

struct PpyBhtBillRec {
	long ID;
	long SupplID;
	int  Day;
	int  Month;
	int  Year;
};

struct PpyBhtBillLineRec {
	long   BillID;
	long   GoodsID;
	double Price;
	double Quantity;
	int    ExpiryDay;
	int    ExpiryMonth;
	int    ExpiryYear;
};

struct PpyBhtInventRec {
	long ID;
	int  Day;
	int  Month;
	int  Year;
};

struct PpyBhtInventLineRec {
	long   InventID;
	long   GoodsID;
	double Price;
	double Quantity;
};

struct PpyBhtTerminal {
	long   ID;
	BSTR   Name;
	int    ReceiptPlace;    // Куда качать накладные  @todo > Flags
	int    ComGet_NumTries; // @#{0..32000} not used
	int    ComGet_Delay;    // @#{0..1000}  for Win32 only
	int    ComPut_NumTries; // @#{0..32000} not used
	int    ComPut_Delay;    // @#{0..1000}  not used
	long   IntrExpndOpID;   // @v4.7.10 Операция внутреннего перемещения (PPOPT_GOODSEXPEND || PPOPT_DRAFTEXPEND)
	long   InventOpID;      // Inventory Operation Kind ID
	BSTR   Port;         // Output port name (default "COM1")
	int    Cbr;             // ComBaudRate (default 19200)
	int    BhtpTimeout;     // Bht protocol timeout, mc (default 3000)
	int    BhtpMaxTries;    // Bht protocol max attempts sending data (default 10)
	long   Flags;
	long   BhtTypeID;       // Reserved (Denso only)
	long   ExpendOpID;      // Expend Operation Kind ID (PPOPT_GOODSEXPEND || PPOPT_DRAFTEXPEND)
};
//
// Интерфейсы
//
//
// IPpyBase
//
interface IPpyBase : public IUnknown {
public:
	IPpyBase();
	virtual ~IPpyBase();
	virtual  ULONG __stdcall AddRef();
	virtual  ULONG __stdcall Release();
	virtual  HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
	int SLAPI Destroy();
private:
	long CRef;
};
//
//
// IPpySession
//
interface IPpySession : public IUnknown {
public:
	virtual HRESULT __stdcall Login(BSTR pDBSymb, BSTR pUserName, BSTR pPwd, long * pThrID, int * pRet) = 0;
	virtual HRESULT __stdcall Logout(int * pRet) = 0;
	virtual HRESULT __stdcall GetLastErr(long * pErrCode) = 0;
	virtual HRESULT __stdcall GetLastErrMsg(BSTR * pMsg, int * pRet) = 0;
	virtual HRESULT __stdcall StartTransaction(int * pTa, int useTa, int * pRet) = 0;
	virtual HRESULT __stdcall CommitWork(int * pTa, int * pRet) = 0;
	virtual HRESULT __stdcall RollbackWork(int * pTa) = 0;
};
//
// IPpyView
//
interface IPpyView : public IUnknown {
public:
	virtual HRESULT __stdcall Init(VARIANT *, int * pRet) = 0;
	virtual HRESULT __stdcall InitIteration(int * pRet) = 0;
	virtual HRESULT __stdcall NextIteration(BSTR, int * pRet) = 0;
};
//
// IPpyBht
//
interface IPpyBht : public IUnknown {
public:
	virtual HRESULT __stdcall Init(long bhtID, int * pRet) = 0;
	
	virtual HRESULT __stdcall InitIteration(int * pRet) = 0;
	virtual HRESULT __stdcall NextIteration(PpyBhtTerminal * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall Get(long bhtID, PpyBhtTerminal * pRec, int * pRet) = 0;

	virtual HRESULT __stdcall InitSuppl(int * pRet) = 0;
	virtual HRESULT __stdcall InitGoods(int * pRet) = 0;
	virtual HRESULT __stdcall NextSuppl(PpyBhtSupplRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall NextGoods(PpyBhtGoodsRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall AcceptBill(const PpyBhtBillRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall AcceptBillLine(const PpyBhtBillLineRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall FinishBill(int * pRet) = 0;
	virtual HRESULT __stdcall AcceptInvent(const PpyBhtInventRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall AcceptInventLine(const PpyBhtInventLineRec * pRec, int * pRet) = 0;
	virtual HRESULT __stdcall TestDialog() = 0;
};
//
// Фабрика классов
//
class CFactory : public IClassFactory {
public:
	CFactory() {CRef = 1; memset(&ClassID, 0, sizeof(CLSID));}
	~CFactory() {}
	HRESULT __stdcall Init(const CLSID classID);
	virtual HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
	virtual ULONG   __stdcall AddRef();
	virtual ULONG   __stdcall Release();

	// Возвращает интерфейс с ид iID для класса с идентификатором ClassID
	//
	//
	virtual HRESULT __stdcall CreateInstance(IUnknown *pUnknownOuter, const IID & iID, void ** ppV);
	virtual HRESULT __stdcall LockServer(BOOL bLock);
private:
	long CRef;
	CLSID ClassID;
};
//
// Объекты
//
class PpySession : public IPpySession {
public:
	PpySession();
	virtual  ~PpySession();
	virtual  HRESULT __stdcall Login(BSTR pDBSymb, BSTR pUserName, BSTR pPwd, long * pThrID, int * pRet);
	virtual  HRESULT __stdcall Logout(int * pRet);
	virtual  HRESULT __stdcall GetLastErr(long * pErrCode);
	virtual  HRESULT __stdcall GetLastErrMsg(BSTR * pMsg, int * pRet);
	virtual HRESULT __stdcall StartTransaction(int * pTa, int useTa, int * pRet)
	{
		ASSIGN_PTR(pRet, PPStartTransaction(pTa, useTa));
		return S_OK;
	}
	virtual HRESULT __stdcall CommitWork(int * pTa, int * pRet)
	{
		ASSIGN_PTR(pRet, PPCommitWork(pTa));
		return S_OK;
	}
	virtual HRESULT __stdcall RollbackWork(int * pTa)
	{
		PPRollbackWork(pTa);
		return S_OK;
	}
	virtual  ULONG __stdcall  AddRef();
	virtual  ULONG __stdcall  Release();
	virtual HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
private:
	int SLAPI GetDBEntry(PPDbEntrySet2 * dbes, PPID * dbentry, char * dict, char * data);
	long CRef;
};
//
// PpyViewOpKind
//
class PpyViewOpKind : public IPpyView {
public:
	PpyViewOpKind();
	~PpyViewOpKind();
	virtual HRESULT __stdcall Init(VARIANT *, int * pRet);
	virtual HRESULT __stdcall InitIteration(int * pRet);
	virtual HRESULT __stdcall NextIteration(BSTR, int * pRet);
	virtual  ULONG __stdcall  AddRef();
	virtual  ULONG __stdcall  Release();
	virtual HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
private:
	long CRef;
	PPViewOprKind View;
};
//
// PpyBht
//
interface PpyBht : public IPpyBht {
public:
	PpyBht();
	virtual ~PpyBht();
	
	virtual HRESULT __stdcall Init(long bhtID, int * pRet);
	
	virtual HRESULT __stdcall InitIteration(int * pRet);
	virtual HRESULT __stdcall NextIteration(PpyBhtTerminal * pRec, int * pRet);
	virtual HRESULT __stdcall Get(long bhtID, PpyBhtTerminal * pRec, int * pRet);

	virtual HRESULT __stdcall InitSuppl(int * pRet);
	virtual HRESULT __stdcall InitGoods(int * pRet);
	virtual HRESULT __stdcall NextSuppl(PpyBhtSupplRec * pRec, int * pRet);
	virtual HRESULT __stdcall NextGoods(PpyBhtGoodsRec * pRec, int * pRet);
	virtual HRESULT __stdcall AcceptBill(const PpyBhtBillRec * pRec, int * pRet);
	virtual HRESULT __stdcall AcceptBillLine(const PpyBhtBillLineRec * pRec, int * pRet);
	virtual HRESULT __stdcall FinishBill(int * pRet);
	virtual HRESULT __stdcall AcceptInvent(const PpyBhtInventRec * pRec, int * pRet);
	virtual HRESULT __stdcall AcceptInventLine(const PpyBhtInventLineRec * pRec, int * pRet);
	virtual HRESULT __stdcall TestDialog();
	virtual  ULONG __stdcall  AddRef();
	virtual  ULONG __stdcall  Release();
	virtual HRESULT __stdcall QueryInterface(const IID & iID, void ** ppV);
private:
	long CRef;
	long SupplIterIdx;
	PPID BhtIterIdx;
	PPID SupplAccSheet;
	PPID * P_AltGrpID;
	PPID BillID;
	BillTbl::Rec InvRec;
	PPObjBHT::InventRec IRec;
	PPBhtTerminal BhtRec;
	PPBasketPacket * P_GBPack;
	TempOrderTbl::Key1 GoodsKey;
	TempOrderTbl * P_GoodsTbl;
	PPObjBHT     * P_BhtObj;
	PPObjGoods   * P_GObj;
	PPObjArticle * P_ArObj;
};

HRESULT CanUnloadNow();
//
// ExeServer
//
int SLAPI RegisterCOMs();
int SLAPI UnregisterCOMs();
HRESULT SLAPI RegisterServer(HMODULE hModule, int isExeServer);
HRESULT SLAPI UnregisterServer(int isExeServer);
int SLAPI RunCOMServer();

#endif // __PPYDLL_H