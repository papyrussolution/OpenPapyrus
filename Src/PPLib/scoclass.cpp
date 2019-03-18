// SCOCLASS.CPP
// Copyright (c) A.Sobolev 2007, 2015, 2016, 2018, 2019
//
#pragma hdrstop
#define DL600R
#include <pp.h>
#include <dl600.h>
//
// @ModuleDef(SCoClass)
//
//static
void * FASTCALL SCoClass::GetExtraPtrByInterface(const void * pIfc)
{
	return pIfc ? ((const TabEntry *)pIfc)->ThisPtr->ExtraPtr : 0;
}

// static
int SLAPI SCoClass::SetExtraPtrByInterface(const void * pIfc, void * extraPtr)
{
	int    ok = -1;
	if(pIfc && ((TabEntry *)pIfc)->ThisPtr->ExtraPtr == 0) {
		((TabEntry *)pIfc)->ThisPtr->ExtraPtr = extraPtr;
		ok = 1;
	}
	return ok;
}

SCoClass::SCoClass(const DlContext * pCtx, const DlScope * pScope, void * pVt) : P_Ctx(pCtx), P_Scope(pScope), TabCount(0), P_Tab(0),
	Flags(0), AppFlags(0), AppError(0), ExtraPtr(0)
{
	Ref.Assign(1);
	InitVTable(pVt);
}

SCoClass::SCoClass(SCoClassConstructor ccc, void * pVt) : P_Ctx(DS.GetInterfaceContext(PPSession::ctxtInterface)),
	P_Scope(0), TabCount(0), P_Tab(0), Flags(fFactory), AppFlags(0), AppError(0), ExtraPtr(0)
{
	Ref.Assign(1);
	InitVTable(pVt);
}

SCoClass::~SCoClass()
{
	TRACE_FUNC();
	ReleaseVTable();
}

int SLAPI SCoClass::CreateInnerInstance(const char * pClsName, const char * pIfcName, void ** ppIfc)
{
	int    ok = 1;
	DLSYMBID clsid = 0, ifcid = 0;
	S_GUID ifc_uuid;
	SCoClass * p_cls = 0;
	const DlScope * p_scope = 0;
	THROW_INVARG(pClsName);
	THROW_PP(P_Ctx, PPERR_DL6_CTXNINITED);
	THROW(P_Ctx->SearchSymb(pClsName, '@', &clsid));
	if(pIfcName) {
		THROW(P_Ctx->SearchSymb(pIfcName, '@', &ifcid));
		THROW_PP_S(P_Ctx->GetUuidByScopeID(ifcid, &ifc_uuid), PPERR_DL6_IFCUUIDNFOUND, pIfcName);
	}
	else
		ifc_uuid.Init(IID_IUnknown);
	THROW_PP_S(p_scope = P_Ctx->GetScope_Const(clsid, DlScope::kIClass), PPERR_DL6_CLSNFOUND, pClsName);
	THROW(P_Ctx->CreateDlIClsInstance(p_scope, &p_cls));
	THROW(p_cls->ImpQueryInterface(ifc_uuid, ppIfc) == S_OK);
	p_cls->ImpRelease();
	CATCHZOK
	return ok;
}

int SLAPI SCoClass::RaiseAppError()
{
	AppError = 1;
	return 0; // @important!
}

void * SLAPI SCoClass::RaiseAppErrorPtr()
{
	AppError = 1;
	return 0; // @important!
}

int FASTCALL SCoClass::SetAppError(int assertion)
{
	if(!assertion) {
		AppError = 1;
		return 0;
	}
	else
		return 1;
}

int SLAPI SCoClass::GetInnerUUID(const char * pScopeName, S_GUID & rIID) const
{
	int    ok = 1;
	DLSYMBID symb_id = 0;
	THROW_PP(P_Ctx, PPERR_DL6_CTXNINITED);
	THROW(P_Ctx->SearchSymb(pScopeName, '@', &symb_id));
	THROW_PP_S(P_Ctx->GetUuidByScopeID(symb_id, &rIID), PPERR_DL6_IFCUUIDNFOUND, pScopeName);
	CATCHZOK
	return ok;
}

int SLAPI SCoClass::SetupError()
{
	int    ok = 0;
	ICreateErrorInfo * p_cer = 0;
	IErrorInfo * p_ei = 0;
	if(SUCCEEDED(CreateErrorInfo(&p_cer))) {
		BSTR w_msg_buf = 0;
		SString msg_buf;
		PPGetLastErrorMessage(1, msg_buf);
		msg_buf.CopyToOleStr(&w_msg_buf);
		p_cer->SetDescription(w_msg_buf);
		if(SUCCEEDED(p_cer->QueryInterface(IID_IErrorInfo, (void **)&p_ei))) {
			SetErrorInfo(0, p_ei);
			ok = 1;
			p_ei->Release();
		}
		p_cer->Release();
		SysFreeString(w_msg_buf);
	}
	return ok;
}

int SLAPI SCoClass::FuncNotSupported()
{
	AppError = 1;
	return PPSetError(PPERR_PPIFCFUNCNSUPPORTED);
}

HRESULT SLAPI SCoClass::Epilog()
{
	if(AppError) {
		SetupError();
		return MAKE_HRESULT(1, FACILITY_ITF, 0);
	}
	else
		return S_OK;
}

int FASTCALL SCoClass::InitVTable(void * pVt)
{
	int    ok = 1;
	P_Vt = pVt;
	TRACE_FUNC();
	THROW_PP(P_Ctx, PPERR_DL6_CTXNINITED);
	if(P_Scope) {
		size_t offs = 0;
		uint   c = P_Scope->GetIfaceBaseCount();
		TabCount = c;
		TabCount++; // ISupportErrorInfo
		P_Tab = (TabCount < MAX_COCLASS_STTAB) ? StTab : new TabEntry[TabCount];
		uint   i = 0;
		for(; i < c; i++) {
			DlScope::IfaceBase iface;
			THROW(P_Scope->GetIfaceBase(i, &iface));
			const DlScope * p_scope = P_Ctx->GetScope_Const(iface.ID);
			THROW(p_scope && p_scope->IsKind(DlScope::kInterface));
			P_Tab[i].P_Tbl = ((uint32 *)P_Vt) + offs;
			P_Tab[i].ThisPtr = this;
			offs += (p_scope->GetFuncCount() + 3); // 3 = count of IUnknown methods
		}
		{
			//
			// ISupportErrorInfo
			//
			P_Tab[i].P_Tbl = ((uint32 *)P_Vt) + offs;
			P_Tab[i].ThisPtr = this;
			offs += 1+3; // 1 = count of ISupportErrorInfo methods, 3 = count of IUnknown methods
			++i;
		}
	}
	else if(Flags & fFactory) {
		P_Tab = StTab;
		TabCount = 1;
		P_Tab[0].P_Tbl = (uint32 *)P_Vt;
		P_Tab[0].ThisPtr = this;
	}
	CATCH
		ReleaseVTable();
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SCoClass::ReleaseVTable()
{
	if(TabCount >= MAX_COCLASS_STTAB)
		delete [] P_Tab;
	P_Tab = 0;
	TabCount = 0;
#ifndef COVT_STATIC
	if(!(Flags & fFactory))
		ZDELETE(P_Vt);
#endif
	return 1;
}

HRESULT SCoClass::ImpQueryInterface(REFIID rIID, void ** ppObject)
{
	HRESULT ok = E_NOINTERFACE;
	*ppObject = 0;
	if(P_Ctx && P_Tab) {
		if(Flags & fFactory) {
			if(IsEqualIID(rIID, IID_IClassFactory)) {
				ImpAddRef();
				*ppObject = (void *)&P_Tab[0];
				ok = S_OK;
			}
		}
		else {
			if(P_Ctx && P_Scope && P_Tab) {
				S_GUID uuid;
				DLSYMBID scope_id = 0;
				if(IsEqualIID(rIID, IID_IUnknown)) {
					if(P_Scope->GetIfaceBaseCount() > 0) {
						ImpAddRef();
						*ppObject = (void *)&P_Tab[0];
						ok = S_OK;
					}
				}
				else if(IsEqualIID(rIID, IID_ISupportErrorInfo)) {
					uint   c = P_Scope->GetIfaceBaseCount();
					ImpAddRef();
					*ppObject = (void *)&P_Tab[c];
					ok = S_OK;
				}
				else if(P_Ctx->GetInterface(uuid.Init(rIID), &scope_id, 0)) {
					uint   c = P_Scope->GetIfaceBaseCount();
					assert(c <= TabCount);
					for(uint i = 0; i < c; i++) {
						DlScope::IfaceBase iface;
						if(P_Scope->GetIfaceBase(i, &iface) && iface.ID == scope_id) {
							ImpAddRef();
							*ppObject = (void *)&P_Tab[i];
							ok = S_OK;
							break;
						}
					}
				}
			}
		}
	}
	return ok;
}

uint32 SCoClass::ImpAddRef()
{
	return (uint32)Ref.Incr();
}

uint32 SCoClass::ImpRelease()
{
	assert(Ref > 0);
#ifdef _DEBUG
	SString b;
	(b = (P_Scope ? P_Scope->GetName() : "zero scope")).Space().CatEq("Ref", Ref);
	TRACE_FUNC_S(b);
#endif
	uint32 c = (uint32)Ref.Decr();
	if(c == 0)
		delete this;
	return c;
}

HRESULT SCoClass::ImpInterfaceSupportsErrorInfo(REFIID rIID) const
{
	HRESULT ok = E_FAIL;
	if(P_Ctx && !IsEqualIID(rIID, IID_IUnknown) && !IsEqualIID(rIID, IID_ISupportErrorInfo)) {
		S_GUID uuid;
		DLSYMBID scope_id = 0;
		if(P_Ctx->GetInterface(uuid.Init(rIID), &scope_id, 0))
			ok = S_OK;
	}
	return ok;
}

HRESULT __stdcall SCoClass::QueryInterface(REFIID rIID, void ** ppObject)
{
#ifdef _DEBUG
	S_GUID guid;
	guid.Init(rIID);
	SString b;
	TRACE_FUNC_S(guid.ToStr(S_GUID::fmtIDL, b));
#endif
	return (reinterpret_cast<TabEntry *>(this)->ThisPtr)->ImpQueryInterface(rIID, ppObject);
}

uint32 __stdcall SCoClass::AddRef() { return (reinterpret_cast<TabEntry *>(this)->ThisPtr)->ImpAddRef(); }
uint32 __stdcall SCoClass::Release() { return (reinterpret_cast<TabEntry *>(this)->ThisPtr)->ImpRelease(); }
HRESULT __stdcall SCoClass::InterfaceSupportsErrorInfo(REFIID rIID) { return (reinterpret_cast<TabEntry *>(this)->ThisPtr)->ImpInterfaceSupportsErrorInfo(rIID); }
//
//
//
class SCoFactory : public SCoClass {
public:
	explicit SLAPI  SCoFactory(REFCLSID);
	HRESULT __stdcall CreateInstance(IUnknown * pUnknownOuter, const IID & iID, void ** ppV);
	HRESULT __stdcall LockServer(BOOL bLock);
private:
	S_GUID ClsUuid;
};

struct SCoFactory_VTab {
#define MFP(f) (__stdcall SCoFactory::*f)
	IUNKN_METHOD_PTRS(00);
	HRESULT MFP(f00000)(IUnknown * pOuter, const IID & iID, void ** ppV);
	HRESULT MFP(f00100)(BOOL bLock);
#undef MFP
	SCoFactory_VTab()
	{
		IUNKN_METHOD_PTRS_ASSIGN(00);
		f00000 = &SCoFactory::CreateInstance;
		f00100 = &SCoFactory::LockServer;
	}
};

static SCoFactory_VTab VT_SCoFactory;

SLAPI SCoFactory::SCoFactory(REFCLSID rClsID) : SCoClass(scccFactory, &VT_SCoFactory/*new SCoFactory_VTab*/)
{
	ClsUuid.Init(rClsID);
}

HRESULT __stdcall SCoFactory::CreateInstance(IUnknown * pOuter, const IID & rIID, void ** ppV)
{
	TRACE_FUNC();
	SCoFactory * __tp = static_cast<SCoFactory *>(reinterpret_cast<TabEntry *>(this)->ThisPtr);
	IUnknown * p_com = 0;
	HRESULT ok = CLASS_E_CLASSNOTAVAILABLE;
	if(pOuter)
		ok = CLASS_E_NOAGGREGATION;
	else if(__tp->P_Ctx) {
		SCoClass * p_cls = 0;
		if(__tp->P_Ctx->CreateDlIClsInstance(__tp->ClsUuid, &p_cls)) {
			ok = p_cls->ImpQueryInterface(rIID, ppV);
			p_cls->ImpRelease();
		}
	}
	return ok;
}

HRESULT __stdcall SCoFactory::LockServer(BOOL bLock)
{
	return DS.LockingDllServer(bLock ? PPSession::ldsLock : PPSession::ldsUnlock) ? S_OK : E_FAIL;
}
//
// DLL helpers
//
//static
HRESULT SLAPI SCoClass::Helper_DllGetClassObject(REFCLSID rClsID, REFIID rIID, void ** ppV)
{
	SCoFactory * p_cf = new SCoFactory(rClsID);
	HRESULT ok = p_cf ? p_cf->ImpQueryInterface(rIID, ppV) : E_OUTOFMEMORY;
#ifdef _DEBUG
	SString s, s2;
	s.Cat(SUCCEEDED(ok) ? 1 : 0).Space();
	S_GUID uuid;
	uuid.Init(rClsID);
	s.Cat(uuid.ToStr(S_GUID::fmtIDL, s2)).Space();
	uuid.Init(rIID);
	s.Cat(uuid.ToStr(S_GUID::fmtIDL, s2));
	TRACE_FUNC_S(s);
#endif
	//p_cf->Release();
	return ok;
}

//static
HRESULT SLAPI SCoClass::Helper_DllCanUnloadNow()
{
	return DS.LockingDllServer(PPSession::ldsCanUnload) ? S_OK : S_FALSE;
}

//static
int SLAPI SCoClass::Helper_DllRegisterServer(int unreg)
{
	int    ok = 0;
	DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxtInterface);
	if(p_ctx)
		ok = p_ctx->RegisterICls(0, unreg);
	return ok;
}
