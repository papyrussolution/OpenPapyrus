// DL600R.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
// Run-time DL600 modules
//
#pragma hdrstop
#define DL600R
#include <pp.h>
#include <dl600.h>
//
//
//
PPFilt::PPFilt() : ID(0), Ptr(0)
{
}

PPFilt::PPFilt(long id) : ID(id), Ptr(0)
{
}

PPFilt::PPFilt(void * ptr) : ID(0), Ptr(ptr)
{
}

PView::PView(long id) : ID(id), Ptr(0)
{
}

PView::PView(void * ptr) : ID(0), Ptr(ptr)
{
}
//
// Блок доступа к распакованным формулам. При обращении к формуле вызывается функция //
// DlContext::GetFormula, которая пытается найти уже распакованную формулу. Если это у нее
// не выходит, то формула распаковывается, вставляется в таблицу и передается клиенту.
//
struct UnpFormItem {
	DLSYMBID ScopeID;
	uint   FldPos;
	CtmExpr * P_Expr;
};

DlContext::UnpFormArray::UnpFormArray() : SArray(sizeof(UnpFormItem), O_ARRAY|aryEachItem)
{
}

DlContext::UnpFormArray::~UnpFormArray()
{
	freeAll();
}

int DlContext::UnpFormArray::Add(DLSYMBID scopeID, uint fldPos, CtmExpr * pExpr)
{
	int    ok = 1;
	UnpFormItem item;
	item.ScopeID = scopeID;
	item.FldPos = fldPos;
	item.P_Expr = pExpr;
	uint   pos = 0;
	if(bsearch(&item, &pos, PTR_CMPFUNC(_2long))) {
		UnpFormItem * p_item = static_cast<UnpFormItem *>(at(pos));
		if(p_item->P_Expr) {
			p_item->P_Expr->Destroy();
			delete p_item->P_Expr;
		}
		p_item->P_Expr = pExpr;
	}
	else if(!ordInsert(&item, &(pos = 0), PTR_CMPFUNC(_2long)))
		ok = PPSetErrorSLib();
	return ok;
}

CtmExpr * DlContext::UnpFormArray::Get(DLSYMBID scopeID, uint fldPos)
{
	UnpFormItem item;
	item.ScopeID = scopeID;
	item.FldPos = fldPos;
	uint   pos = 0;
	return (bsearch(&item, &pos, PTR_CMPFUNC(_2long))) ? static_cast<UnpFormItem *>(at(pos))->P_Expr : 0;
}

/*virtual*/void FASTCALL DlContext::UnpFormArray::freeItem(void * pItem)
{
	UnpFormItem * p_item = static_cast<UnpFormItem *>(pItem);
	if(p_item && p_item->P_Expr) {
		p_item->P_Expr->Destroy();
		delete p_item->P_Expr;
	}
}

CtmExpr * DlContext::GetFormula(const DlScope * pScope, uint fldPos)
{
	CtmExpr * p_expr = UnpFormList.Get(pScope->ID, fldPos);
	if(!p_expr) {
		SdbField fld;
		THROW(pScope->GetFieldByPos(fldPos, &fld));
		THROW(fld.T.Flags & STypEx::fFormula);
		{
			SStrScan scan(fld.InnerFormula);
			THROW_MEM(p_expr = new CtmExpr);
			THROW(p_expr->Unpack(scan));
			THROW(UnpFormList.Add(pScope->ID, fldPos, p_expr));
		}
		THROW(p_expr = UnpFormList.Get(pScope->ID, fldPos));
	}
	CATCH
		ZDELETE(p_expr);
	ENDCATCH
	return p_expr;
}
//
// Блок создания run-time объектов SCoClass и DlRtm
//
int DlContext::CreateDlIClsInstance(const DlScope * pScope, SCoClass ** ppInstance) const
{
	int    ok = 1;
	SCoClass * p_inst = 0;
	SString ffn;
	Use001(); // чтобы линковались модули с коклассами
	ffn.Cat("DL6CF_").Cat(pScope->Name);
	FN_DL6CLS_FACTORY f = reinterpret_cast<FN_DL6CLS_FACTORY>(::GetProcAddress(SLS.GetHInst(), ffn));
	THROW_PP_S(f, PPERR_DL6_ICLSNOTIMPL, ffn);
	THROW(p_inst = f(this, pScope));
	CATCH
		ZDELETE(p_inst);
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(ppInstance, p_inst);
	return ok;
}

int DlContext::CreateDlIClsInstance(const S_GUID & rClsUuid, SCoClass ** ppInstance) const
{
	int    ok = 1;
	const  DlScope * p_scope = 0;
	DLSYMBID symb_id = SearchUuid(rClsUuid);
	ASSIGN_PTR(ppInstance, 0);
	THROW(symb_id);
	THROW(p_scope = GetScope_Const(symb_id, DlScope::kIClass));
	THROW(CreateDlIClsInstance(p_scope, ppInstance));
	CATCHZOK
	return ok;
}

int DlContext::CreateDlRtmInstance(DlScope * pScope, DlRtm ** ppInstance)
{
	int    ok = 1;
	SString ffn;
	DlRtm * p_inst = 0;
	DlScope * p_base = pScope;
	for(DLSYMBID base_id = p_base->GetBaseId(); base_id != 0;) {
		THROW(p_base = GetScope(base_id, DlScope::kExpData));
		base_id = p_base->GetBaseId();
	}
	ffn.Cat("DL6FF_").Cat(p_base->Name);
	FN_DL6RTM_FACTORY f = (FN_DL6RTM_FACTORY)GetProcAddress(SLS.GetHInst(), ffn);
	THROW_PP_S(f, PPERR_DL6_STRUCNOTIMPL, ffn);
	THROW(p_inst = f(this, pScope));
	CATCH
		ZDELETE(p_inst);
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(ppInstance, p_inst);
	return ok;
}

int DlContext::CreateDlRtmInstance(const char * pName, DlRtm ** ppInstance)
{
	int    ok = 1;
	DLSYMBID parent_id = 0;
	DlScope * p_scope = Sc.SearchByName(DlScope::kExpData, pName, &parent_id);
	if(p_scope)
		ok = CreateDlRtmInstance(p_scope, ppInstance);
	else {
		ASSIGN_PTR(ppInstance, 0);
		ok = PPSetError(PPERR_DL6_UNDEFDATASYMB, pName);
	}
	return ok;
}
//
//
//
DlRtm * FASTCALL DlContext::GetRtm(DLSYMBID dataId)
{
	DlRtm * p_rtm = 0;
	uint   i = RtmList.getCount();
	if(i) do {
		DlRtm * p_local_rtm = RtmList.at(--i);
		if(p_local_rtm && p_local_rtm->GetDataId() == dataId)
			p_rtm = p_local_rtm;
	} while(i && !p_rtm);
	if(!p_rtm) {
		SString msg_buf;
		DlScope * p_scope = Sc.SearchByID(dataId, 0);
		THROW_PP_S(p_scope, PPERR_DL6_DATASTRUCIDNFOUND, msg_buf.Cat(dataId));
		THROW(CreateDlRtmInstance(p_scope, &p_rtm));
		RtmList.insert(p_rtm);
	}
	CATCH
		ZDELETE(p_rtm);
	ENDCATCH
	return p_rtm;
}

DlRtm * FASTCALL DlContext::GetRtm(const char * pName)
{
	DLSYMBID parent_id = 0;
	DlScope * p_scope = Sc.SearchByName(DlScope::kExpData, pName, &parent_id);
	return p_scope ? GetRtm(p_scope->ID) : (PPSetError(PPERR_DL6_UNDEFDATASYMB, pName), 0);
}

uint DlContext::AllocStackType(DLSYMBID typeID, TypeEntry * pTe)
{
	uint   sp = 0;
	TypeEntry te;
	SearchTypeID(typeID, 0, &te);
	ASSIGN_PTR(pTe, te);
	if(te.T.IsZStr(0)) {
		SString * p_s = StP.Alloc(0);
		sp = S.Alloc(sizeof(p_s));
		void * ptr = S.GetPtr(sp);
		*static_cast<SString **>(ptr) = p_s;
		Pss.insert(&sp);
	}
	else {
		size_t sz = te.T.GetBinSize();
		sp = S.Alloc(sz);
	}
	return sp;
}

int FASTCALL DlContext::ReleaseStack(uint pos)
{
	uint   c = Pss.getCount();
	if(c) do {
		uint   sp = Pss.at(--c);
		if(sp >= pos) {
			StP.Free(*static_cast<SString **>(S.GetPtr(sp)));
			Pss.atFree(c);
		}
	} while(c);
	S.SetCurPos(pos);
	return 1;
}

void * FASTCALL DlContext::GetStackPtr(uint sp)
{
	return S.GetPtr(sp);
}

int DlContext::FormatVar(CtmVar v, SString & rBuf) const
{
	int    ok = 0;
	rBuf.Z();
	const DlScope * p_scope = GetScope_Const(v.ScopeID, 0);
	if(p_scope) {
		rBuf.Cat(p_scope->Name);
		SdbField fld;
		if(p_scope->GetFieldByPos(v.Pos, &fld) > 0) {
			rBuf.CatCharN(':', 2).Cat(fld.Name);
			ok = 1;
		}
		else
			rBuf.CatCharN(':', 2).Cat("(error position)");
	}
	else
		rBuf.Cat("(error scope)");
	return ok;
}

const DlScope * DlContext::GetEvaluatedVarScope(const DlScope * pScope, const CtmExpr * pExpr) const // @obsolete
{
	DLSYMBID target_id = pExpr->U.V.ScopeID;
	const DlScope * p_target = 0;
	if(pScope) {
		if(pScope->ID == target_id)
			p_target = pScope;
		else if(!pScope->GetOwner() || (p_target = pScope->GetOwner()->SearchByID_Const(target_id, 0)) == 0)
			p_target = pScope->GetBase() ? GetEvaluatedVarScope(pScope->GetBase(), pExpr) : 0; // @recursion
	}
	return p_target;
}

const DlScope * DlContext::GetEvaluatedVarScope(const DlScope * pScope, DLSYMBID targetScopeID) const
{
	const DlScope * p_target = 0;
	if(pScope) {
		if(pScope->ID == targetScopeID)
			p_target = pScope;
		else if(!pScope->GetOwner() || (p_target = pScope->GetOwner()->SearchByID_Const(targetScopeID, 0)) == 0)
			p_target = pScope->GetBase() ? GetEvaluatedVarScope(pScope->GetBase(), targetScopeID) : 0; // @recursion
	}
	return p_target;
}
//
// ARG(pRtm  IN) - Run-time - контекст исполнения операторов
// ARG(pExpr IN) - Вычисляемое выражение
// ARG(sp    IN) - Позиция на стеке, по которой должен быть записан результат выражения //
// Returns:
// Remark: Пространство на стеке для возвращаемого значения выделяет вызывающая функция.
//   Пространство для аргументов - эта функция.
//
int DlContext::EvaluateExpr(DlRtm * pRtm, const DlScope * pScope, DlRtm * pCallerRtm, const DlScope * pCallerScope, CtmExpr * pExpr, size_t sp)
{
	int    ok = 1;
	size_t slen = 0;
	const  uint   cur_pos = S.GetCurPos();
	const  DLSYMBID impl_type_id = pExpr->GetImplicitCast();
	uint   ret_pos = impl_type_id ? AllocStackType(impl_type_id, 0) : sp;
	TypeEntry te;
	if(pCallerRtm && pRtm != pCallerRtm)
		pRtm->P_Ep = pCallerRtm->P_Ep;
	switch(pExpr->GetKind()) {
		case CtmExpr::kEmpty:
			break;
		case CtmExpr::kConst:
			THROW(SearchTypeID(pExpr->U.C.TypeId, 0, &te));
			if(te.T.IsZStr(&slen)) {
				SString * p_str = *static_cast<SString **>(S.GetPtr(ret_pos));
				*p_str = static_cast<const char *>(ConstList.GetPtr(&pExpr->U.C, slen));
				PPExpandString(*p_str, CTRANSF_UTF8_TO_OUTER); // @v9.2.8
				p_str->Transf(CTRANSF_OUTER_TO_INNER);
			}
			else
				ConstList.Get(&pExpr->U.C, S.GetPtr(ret_pos), te.T.GetBinSize());
			break;
		case CtmExpr::kVar:
			{
				const DlScope * p_scope = GetEvaluatedVarScope(pScope, pExpr->U.V.ScopeID);
				assert(p_scope);
				const STypEx * p_t = p_scope->GetFieldExType(pExpr->U.V.Pos);
				assert(p_t != 0); // @debug
				if(p_t->IsZStr(&slen)) {
					SString * p_str = *static_cast<SString **>(S.GetPtr(ret_pos));
					*p_str = static_cast<const char *>(p_scope->GetDataC(pExpr->U.V.Pos));
				}
				else
					memcpy(S.GetPtr(ret_pos), p_scope->GetDataC(pExpr->U.V.Pos), p_t->GetBinSize());
			}
			break;
		case CtmExpr::kFunc:
			{
				//
				// Arguments evaluation
				//
				int    preproc_func = 0;
				uint   arg_no = 0;
				CtmExpr * p_arg = pExpr->P_Arg;
				SV_Uint32 arg_pos_list;
				arg_pos_list.Init();
				DlFunc func;
				DlScope * p_scope = GetScope(pExpr->U.F.ScopeID);
				THROW(p_scope);
				THROW(p_scope->GetFuncByPos(pExpr->U.F.Pos, &func));
				if(func.ImplID >= DL6FI_FIRST && func.ImplID <= DL6FI_LAST)
					preproc_func = func.ImplID;
				arg_pos_list.Add(ret_pos);
				while(p_arg) {
					arg_no++;
					const  size_t ret_sp = AllocStackType(p_arg->GetTypeID(), &te);
					arg_pos_list.Add(ret_sp);
					THROW(EvaluateExpr(pCallerRtm, pCallerScope, pCallerRtm, pCallerScope, p_arg, ret_sp)); // @recursion (use caller scope)
					if(preproc_func && arg_no == 1) {
						if(func.ImplID == DL6FI_DOT) {
							int    r;
							DlRtm * p_rtm = GetRtm(te.T.Link);
							THROW(p_rtm);
							{
								PPFilt pf(*static_cast<const long *>(S.GetPtr(ret_sp)));
								THROW(r = p_rtm->InitData(pf));
								//
								// Если порожденный класс не смог инициализировать данные, то
								// мы инициализируем их сами. Это - наследие прошлого: слишком
								// большой объем кода работает так, что DlRtm::InitData не вызывается //
								// в случае неудачи с поиском требуемой записи и т.д.
								if(r < 0) {
									PPFilt empty_filt;
									p_rtm->DlRtm::InitData(empty_filt, 0);
								}
								THROW(EvaluateExpr(p_rtm, p_rtm->GetHdrScope(), pRtm, pScope, p_arg->P_Next, ret_pos)); // @recursion (set caller scope)
							}
							break;
						}
						else if(func.ImplID == DL6FI_REF) {
							*static_cast<long *>(S.GetPtr(ret_pos)) = *static_cast<const long *>(S.GetPtr(ret_sp));
							break;
						}
						else {
							int    ret_bool = *static_cast<const int *>(S.GetPtr(ret_sp));
							if(func.ImplID == DL6FI_AND) {
								if(ret_bool) {
									p_arg = p_arg->P_Next;
									assert(p_arg); // @debug
									THROW(EvaluateExpr(pCallerRtm, pCallerScope, pCallerRtm, pCallerScope, p_arg, ret_pos)); // @recursion (use caller scope)
								}
								else
									*static_cast<int *>(S.GetPtr(ret_pos)) = ret_bool;
								break;
							}
							else if(func.ImplID == DL6FI_OR) {
								if(!ret_bool) {
									p_arg = p_arg->P_Next;
									assert(p_arg); // @debug
									THROW(EvaluateExpr(pCallerRtm, pCallerScope, pCallerRtm, pCallerScope, p_arg, ret_pos)); // @recursion (use caller scope)
								}
								else
									*static_cast<int *>(S.GetPtr(ret_pos)) = ret_bool;
								break;
							}
							else if(func.ImplID == DL6FI_QUEST) {
								if(ret_bool)
									p_arg = p_arg->P_Next;
								else {
									assert(p_arg->P_Next); // @debug
									p_arg = p_arg->P_Next->P_Next;
								}
								THROW(EvaluateExpr(pCallerRtm, pCallerScope, pCallerRtm, pCallerScope, p_arg, ret_pos)); // @recursion (use caller scope)
								break;
							}
							else if(func.ImplID == DL6FI_IF) {
								if(ret_bool) {
									p_arg = p_arg->P_Next;
									assert(p_arg); // @debug
									THROW(EvaluateExpr(pCallerRtm, pCallerScope, pCallerRtm, pCallerScope, p_arg, ret_pos)); // @recursion (use caller scope)
								}
								else {
									//
									// Половинчатый оператор IF. Если условие оператора ложное, то
									// обнуляем результат выражения (наверное, это не всегда будет правильно!).
									//
									THROW(SearchTypeID(pExpr->GetOrgTypeID(), 0, &te));
									if(te.T.IsZStr(&slen)) {
										SString * p_str = *static_cast<SString **>(S.GetPtr(ret_pos));
										p_str->Z();
									}
									else
										memzero(S.GetPtr(ret_pos), te.T.GetBinSize());
								}
								break;
							}
						}
					}
					p_arg = p_arg->P_Next;
				}
				//
				// Function calling
				//
				if(!preproc_func) {
					if(func.ImplID) {
						THROW(BuiltinOp(&func, &arg_pos_list));
					}
					else {
						pRtm->EvaluateFunc(&func, &arg_pos_list, S);
					}
				}
				arg_pos_list.Destroy();
			}
			break;
		default:
			CALLEXCEPT();
			break;
	}
	if(impl_type_id)
		THROW(TypeCast(pExpr->GetOrgTypeID(), impl_type_id, 1, S.GetPtr(ret_pos), S.GetPtr(sp), 0));
	CATCHZOK
	ReleaseStack(cur_pos);
	// @debug {
#ifdef _DEBUG
	SString * debug_p_s = *static_cast<SString **>(S.GetPtr(sp));
	double debug_r = *static_cast<const double *>(S.GetPtr(sp));
	int    debug_i = *static_cast<const int *>(S.GetPtr(sp));
#endif
	// } @debug
	if(pCallerRtm && pRtm != pCallerRtm)
		pRtm->P_Ep = 0;
	return ok;
}

void DlRtm::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
}
//
//
//
int DlRtm::InitScope(const DlScope * pScope, int topLevel)
{
	int    ok = 1;
	if(pScope) {
		//
		// Сначала инициализируем собственно pScope
		//
		DlScope * p_child = 0;
		for(uint i = 0; pScope->EnumChilds(&i, &p_child);) {
			THROW_SL(p_child->AllocDataBuf());
			if(topLevel)
				if(p_child->IsKind(DlScope::kExpDataHdr)) {
					if(p_child->Name == "hdr")
						P_HdrScope = p_child;
				}
				else if(p_child->IsKind(DlScope::kExpDataIter)) {
					if(p_child->Name == "iter@def" && IterList.GetCount()) {
						//
						// default-итератор должен быть самым первым в списке.
						// Этот блок гарантирует выполнение такого правила.
						//
						const uint32 first = IterList[0];
						IterList.P_Data[1+0] = p_child->ID;
						IterList.Add(first);
					}
					else
						IterList.Add(p_child->ID);
				}
		}
		//
		// Теперь инициализируем базовую область
		//
		THROW(InitScope(pScope->GetBase(), 0)); // @recursion
	}
	CATCHZOK
	return ok;
}

DlRtm::DlRtm(DlContext * pCtx, DlScope * pScope) : P_Ep(0), P_Ctx(pCtx), DataId(0), P_Data(0), P_HdrScope(0), Valid(1)
{
	THROW_MEM(Extra = (ExtData *)SAlloc::C((size_t)32, sizeof(ExtData)));
	IterList.Init();
	if(P_Ctx && pScope) {
		P_Data = pScope;
		DataId = pScope->ID;
		THROW(InitScope(pScope, 1));
	}
	CATCH
		Valid = 0;
	ENDCATCH
}

DlRtm::~DlRtm()
{
	IterList.Destroy();
	SAlloc::F(Extra);
}

long FASTCALL DlRtm::GetIterID(const char * pIterName) const
{
	long   id = 0;
	if(pIterName == 0)
		id = BIN(IterList.GetCount());
	else {
		DlScope * p_scope = P_Data->SearchByName(DlScope::kExpDataIter, pIterName, 0);
		if(p_scope) {
			uint   pos = 0;
			if(IterList.Search(p_scope->ID, &pos))
				id = (pos+1);
		}
	}
	return id;
}

void * DlRtm::GetFixFieldData(const DlScope * pScope, uint fldPos)
{
	void * ptr = 0;
	size_t offs = 0;
	uint   i = 0;
	if(fldPos < pScope->GetCount()) {
		if(!(pScope->GetC(fldPos)->T.Flags & STypEx::fFormula)) {
			for(; i < fldPos; i++) {
				const STypEx & r_t = pScope->GetC(i)->T;
				if(!(r_t.Flags & STypEx::fFormula))
					offs += r_t.GetBinSize();
			}
			ptr = pScope->GetFixDataPtr(offs);
		}
	}
	return ptr;
}

int FASTCALL DlRtm::FinishRecord(const DlScope * pScope)
{
	int    ok = 1;
	uint   i;
	//
	// Сначала формируем базовые области
	//
	if(pScope->GetBase())
		THROW(FinishRecord(pScope->GetBase())); // @recursion
	//
	// После того, как сформировали базовые области, формируем текущую
	//
	{
		size_t fix_offs = 0;
		uint   c = pScope->GetCount();
		//
		// Перенос фиксированных полей в буфер данных
		//
		for(i = 0; i < c; i++) {
			const STypEx & r_t = pScope->GetC(i)->T;
			if(!(r_t.Flags & STypEx::fFormula)) {
				size_t fld_sz = r_t.GetBinSize();
				memcpy(pScope->GetData(i), pScope->GetFixDataPtr(fix_offs), fld_sz);
				fix_offs += fld_sz;
			}
		}
		//
		// Расчет значений, задаваемых формулами
		//
		for(i = 0; i < c; i++) {
			const STypEx & r_t = pScope->GetC(i)->T;
			if(r_t.Flags & STypEx::fFormula) {
				size_t fld_sz = r_t.GetBinSize();
				CtmExpr * p_expr = P_Ctx->GetFormula(pScope, i);
				THROW(p_expr); // @todo error
				{
					DlContext::TypeEntry te;
					size_t slen;
					const  size_t ret_sp = P_Ctx->AllocStackType(p_expr->GetTypeID(), &te);
					THROW(P_Ctx->EvaluateExpr(this, pScope, this, pScope, p_expr, ret_sp));
					if(r_t.IsZStr(&slen)) {
						SString * p_ss = *(SString **)P_Ctx->GetStackPtr(ret_sp);
						p_ss->CopyTo((char *)pScope->GetData(i), slen);
					}
					else
						memcpy(pScope->GetData(i), P_Ctx->GetStackPtr(ret_sp), fld_sz);
					P_Ctx->ReleaseStack(ret_sp);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int DlRtm::InitData(PPFilt & rF, long rsrv)
{
	return BIN(P_HdrScope && FinishRecord(P_HdrScope));
}

int DlRtm::InitIteration(long iterId, int sortId, long rsrv)
{
	return -1;
}

int DlRtm::NextIteration(long iterId/*, long rsrv*/)
{
	if(iterId) {
		DLSYMBID scope_id = IterList.Get(iterId-1);
		DlScope * p_scope = P_Data->SearchByID(scope_id, 0);
		return BIN(p_scope && FinishRecord(p_scope));
	}
	else
		return 0;
}

int DlRtm::Set(long iterId, int commit)
{
	PPSetError(PPERR_DL6_SETUNSUPPORTED, P_Data ? P_Data->GetName() : static_cast<const char *>(0));
	return 0;
}

void DlRtm::Destroy()
{
}

int FASTCALL DlRtm::IterProlog(PPIterID & rID, int doInit)
{
	if(rID == DEFAULT_ITER)
		rID = GetIterID();
	int    prev_is_first = Extra[(int)rID-1].isFirst;
	Extra[(int)rID-1].isFirst = BIN(doInit);
	return (!doInit && prev_is_first) ? -1 : 1;
}

int FASTCALL DlRtm::AssignHeadData(void * pData, size_t dataSize) { return InitFixData(rscDefHdr, pData, dataSize); }
int FASTCALL DlRtm::AssignIterData(int one, void * pData, size_t dataSize) { return InitFixData(rscDefIter, pData, dataSize); }
int FASTCALL DlRtm::AssignDefIterData(void * pData, size_t dataSize) { return InitFixData(rscDefIter, pData, dataSize); }

int DlRtm::InitFixData(const char * pScopeName, void * pData, size_t dataSize)
{
	int    ok = 0;
	if(P_Data) {
		DLSYMBID parent_id = 0;
		const DlScope * p_base = P_Data;
		while(p_base->GetBase())
			p_base = p_base->GetBase();
		DlScope * p_scope = (DlScope *)p_base->SearchByName_Const(0, pScopeName, &parent_id); // @badcast (const-->non-const)
		if(p_scope) {
			p_scope->SetFixDataBuf(pData, dataSize, 1);
			ok = 1;
		}
	}
	return ok;
}

int DlRtm::InitFixData(int reservedScopeCode, void * pData, size_t dataSize)
{
	int    ok = 0;
	if(reservedScopeCode == rscDefHdr)
		ok = InitFixData("hdr", pData, dataSize);
	else if(reservedScopeCode == rscDefIter)
		ok = InitFixData("iter@def", pData, dataSize);
	return ok;
}

// @Muxa {
class SetScopeBlk {
public:
	SetScopeBlk(DlRtm * pRtm) : P_Rtm(pRtm), P_Root(0), P_Curr(0), ID(0), ParentID(0)
	{
	}
	~SetScopeBlk()
	{
	}
	int FASTCALL Init(DlScope * pRoot)
	{
		int    ok = 1;
		THROW(P_Root = pRoot);
		THROW(ToHeader());
		CATCHZOK
		return ok;
	}
	int FASTCALL ToChild(const char * pName)
	{
		int    ok = 1;
		THROW(P_Curr = P_Root->SearchByName(DlScope::kExpDataIter, pName, &ParentID));
		ID = P_Rtm->GetIterID(pName);
		CATCHZOK
		return ok;
	}
	int ToHeader()
	{
		int    ok = 1;
		THROW(P_Curr = P_Root->SearchByName(0, "hdr", &ParentID));
		ID = 0;
		CATCHZOK
		return ok;
	}
	int GetFieldByName(const char * pName, SdbField * pFld) const
	{
		int    ok = 1;
		THROW(P_Curr);
		THROW(P_Curr->GetFieldByName(pName, pFld));
		CATCHZOK
		return ok;
	}
	DLSYMBID GetScopeID() const { return ID; }
	void * GetBuffer() const { return P_Curr ? P_Curr->GetFixDataPtr(0) : 0; }
private:
	DlRtm    * P_Rtm;
	DlScope  * P_Root;
	DlScope  * P_Curr;
	DLSYMBID   ID;
	DLSYMBID   ParentID;
};

int DlRtm::SetByJSON_Helper(json_t * pNode, SetScopeBlk & rBlk)
{
	int    ok = 1;
	void * p_buf = NULL;
	const  char * fld_name = 0;
	SdbField fld;
	SFormatParam fp;
	SString temp_buf;
	for(json_t * p_cur = pNode; p_cur != NULL; p_cur = p_cur->P_Next) {
		switch(p_cur->Type) {
			case json_t::tARRAY:
				THROW(SetByJSON_Helper(p_cur->P_Child, rBlk));   // @recursion
				break;
			case json_t::tOBJECT:
                THROW(SetByJSON_Helper(p_cur->P_Child, rBlk));   // @recursion
				THROW(Set(rBlk.GetScopeID(), 0));
				break;
			case json_t::tSTRING:
				if(p_cur->P_Child) {
					fld_name = p_cur->Text;
					switch(p_cur->P_Child->Type) {
						case json_t::tNUMBER:
						case json_t::tSTRING:
							THROW(rBlk.GetFieldByName(fld_name, &fld));
							if(!(fld.T.Flags & STypEx::fFormula)) {
								//p_value = json_unescape(p_cur->P_Child->P_Text);
								(temp_buf = p_cur->P_Child->Text).Unescape();
								THROW(p_buf = rBlk.GetBuffer());
								fld.PutFieldDataToBuf(temp_buf, p_buf, fp);
								//ZFREE(p_value);
							}
							break;
						default:
							THROW(Set(rBlk.GetScopeID(), 0));
							THROW(rBlk.ToChild(temp_buf.Z().Cat("iter@").Cat(fld_name)));
							THROW(SetByJSON_Helper(p_cur->P_Child, rBlk));	// @recursion
							THROW(rBlk.ToHeader());
							break;
					}
				}
				break;
			default:
				break;
		}
	}
	CATCHZOK
	//SAlloc::F(p_value);
	return ok;
}

int DlRtm::SetByJSON(json_t * pJSONDoc, PPID & ObjId)
{
	int    ok = 1;
	SetScopeBlk s_blk(this);
	THROW(s_blk.Init(P_Data));
	THROW(SetByJSON_Helper(pJSONDoc, s_blk));
	THROW(Set(0, 1));
	ObjId = reinterpret_cast<PPID>(Extra[4].Ptr);
	CATCHZOK
	return ok;
}
// } @Muxa

static DBTable * __CreateDBTable(const DlScope * pScope, BDictionary * pDict,
	const char * pDataName, const char * pPath, const char * pSuffix, const char * pFileName, int idN)
{
	BNKey  bnkey;
	SdbField fld;
	SString fname;
	DBTable * p_tbl = new DBTable;
	THROW_MEM(p_tbl);
	{
		fname = "_ID_";
		if(idN)
			fname.CatLongZ(idN, 3);
		p_tbl->AddField(fname, MKSTYPE(S_AUTOINC, 4));
	}
	{
		const DlScope * p_scope = 0;
		for(uint j = 0; pScope->EnumInheritance(&j, &p_scope);)
			for(uint i = 0; p_scope->EnumFields(&i, &fld);)
				p_tbl->AddField(fld.Name, fld.T.GetDbFieldType());
	}
	bnkey.addSegment(0, XIF_EXT);
	p_tbl->AddKey(bnkey);
	{
		p_tbl->SetTableName((fname = pDataName).Cat(pSuffix));
		p_tbl->SetName((fname = pPath).SetLastSlash().Cat(pFileName));
		THROW(pDict->CreateTableAndFileBySpec(&p_tbl));
		p_tbl->SetFlag(XTF_TEMP);
	}
	CATCH
		ZDELETE(p_tbl);
	ENDCATCH
	return p_tbl;
}

static int FASTCALL __FillRecBuf(const DlScope * pScope, char * pRecBuf)
{
	int    ok = 1;
	size_t offset = sizeof(long);
	SdbField fld;
	SString temp_buf;
	const DlScope * p_scope = 0;
	for(uint j = 0; pScope->EnumInheritance(&j, &p_scope);) {
		for(uint i = 0; p_scope->EnumFields(&i, &fld);) {
			const size_t sz = fld.T.GetBinSize();
			memcpy(pRecBuf+offset, p_scope->GetDataC(i-1), sz);
			if(fld.T.IsZStr(0)) {
				// @v9.5.5 pRecBuf[offset+sz] = 0;
				// @v9.5.5 SOemToChar(pRecBuf+offset);
				// @v9.5.5 {
				(temp_buf = pRecBuf+offset).Transf(CTRANSF_INNER_TO_OUTER);
				strnzcpy(pRecBuf+offset, temp_buf, sz);
				// } @v9.5.5 
			}
			offset += sz;
		}
	}
	return ok;
}

int FASTCALL __CopyFileByPath(const char * pSrcPath, const char * pDestPath, const char * pFileName); // Prototype (pputil.cpp)

DlRtm::ExportParam::ExportParam() : P_F(0), Sort(0), Flags(0), P_ViewDef(0)
{
}

int DlRtm::Export(ExportParam & rParam)
{
	P_Ep = &rParam;
	int    ok = 1;
	const  int use_ddf = BIN(!DS.CheckExtFlag(ECF_DBDICTDL600) || rParam.Flags & ExportParam::fForceDDF);
	const  DlScope * p_data = GetData();
	StringSet out_file_set;
	DBTable * p_tbl = 0;
	SString path, data_name;
	BDictionary * p_dict = 0;
	PPGetPath(PPPATH_TEMP, path); 
	THROW_PP_S(::access(path.RmvLastSlash(), 0) == 0, PPERR_NEXISTPATH, path);
	path.SetLastSlash();
	path.CatLongZ(NZOR(DS.GetConstTLA().PrnDirId, LConfig.SessionID), 8);
	rParam.Path = path;
	if(::access(path, 0) != 0) {
		THROW_PP_S(createDir(path), PPERR_SLIB, path);
	}
	if(use_ddf) {
		SString packpath;
		PPGetPath(PPPATH_PACK, packpath);
		THROW_PP(packpath.NotEmptyS(), PPERR_UNDEFPACKPATH);
		THROW_PP_S(::access(packpath.RmvLastSlash(), 0) == 0, PPERR_NEXISTPATH, packpath);
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfTableFileName));
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfFieldFileName));
		THROW(__CopyFileByPath(packpath, path, BDictionary::DdfIndexFileName));
	}
	p_dict = use_ddf ? BDictionary::CreateBtrDictInstance(path) : new BDictionary(path);
	THROW_MEM(p_dict);
	THROW_DB(p_dict->IsValid());
	data_name = (rParam.Flags & ExportParam::fInheritedTblNames && p_data->GetBase()) ? p_data->GetBase()->Name : p_data->Name;
	if(rParam.DestPath.NotEmpty() && ::access(rParam.DestPath, 0) != 0) {
		THROW_SL(createDir(rParam.DestPath));
	}
	{
		SString suffix, fname, left;
		DlScope * p_child = 0;
		int    idn = 0;
		int    idn_iter_inc = 0;
		for(uint i = 0; p_data->EnumChilds(&i, &p_child);) {
			int    is_hdr = 0;
			if(p_child->Name.IsEqiAscii("hdr")) {
				suffix = "_Head";
				PPGetFileName(PPFILNAM_HEAD_BTR, fname);
				is_hdr = 1;
				if(rParam.Flags & ExportParam::fDiff_ID_ByScope)
					idn = 1;
			}
			else if(p_child->Name == "iter@def") {
				suffix = "_Iter";
				PPGetFileName(PPFILNAM_ITER_BTR, fname);
				if(rParam.Flags & ExportParam::fDiff_ID_ByScope)
					idn = 2;
			}
			else if(p_child->Name.Divide('@', left, suffix) > 0) {
				left.Z().CatChar('_').Cat(suffix);
				suffix = left;
				(fname = suffix).Dot().Cat("btr");
				if(rParam.Flags & ExportParam::fDiff_ID_ByScope) {
					++idn_iter_inc;
					idn = (10+idn_iter_inc);
				}
			}
			else {
				suffix = p_child->Name;
				(fname = suffix).Dot().Cat("btr");
				if(rParam.Flags & ExportParam::fDiff_ID_ByScope) {
					++idn_iter_inc;
					idn = (10+idn_iter_inc);
				}
			}
			THROW(p_tbl = __CreateDBTable(p_child, p_dict, data_name, path, suffix, fname, idn));
			if(rParam.P_F) {
				if(is_hdr) {
					THROW(InitData(*rParam.P_F, BIN(rParam.Flags & ExportParam::fIsView)));
					__FillRecBuf(p_child, static_cast<char *>(p_tbl->getDataBuf()));
					THROW_DB(p_tbl->insertRec());
				}
				else {
					long   iter_id = GetIterID(p_child->Name);
					char * p_rec_buf = static_cast<char *>(p_tbl->getDataBuf());
					int    r;
					BExtInsert bei(p_tbl, 16*1024U);
					THROW(InitIteration(iter_id, rParam.Sort));
					while((r = NextIteration(iter_id)) > 0) {
						p_tbl->clearDataBuf();
						__FillRecBuf(p_child, p_rec_buf);
						THROW_DB(bei.insert(p_rec_buf));
					}
					THROW_DB(bei.flash());
					THROW(r);
				}
			}
			out_file_set.add(p_tbl->GetName());
			ZDELETE(p_tbl);
			if(rParam.DestPath.NotEmpty())
				THROW(__CopyFileByPath(path, rParam.DestPath, fname));
		}
	}
	if(rParam.DestPath.NotEmpty()) {
		if(use_ddf) {
			ZDELETE(p_dict);
			THROW(__CopyFileByPath(path, rParam.DestPath, BDictionary::DdfTableFileName));
			THROW(__CopyFileByPath(path, rParam.DestPath, BDictionary::DdfFieldFileName));
			THROW(__CopyFileByPath(path, rParam.DestPath, BDictionary::DdfIndexFileName));
		}
	}
	else {
		//
		// Участок кода, введенный как попытка решить следующую проблему:
		//   в некоторых случаях при печати CrystalReports выдает ошибку "Файл не найден"
		//   или что-то в этом роде. Предположительно, ошибка возникает из-за того,
		//   что при каких-то особенностях операционной системы файловая система
		//   в течении короткого промежутка времени "не успевает" отобразить файлы данных.
		//
		const int _by_btr = BIN(p_dict && DS.CheckExtFlag(ECF_DETECTCRDBTEXISTBYOPEN));
		for(uint p = 0; out_file_set.get(&p, path);) {
			if(_by_btr) {
				while(!p_dict->IsFileExists_(path)) {
					SDelay(10);
				}
			}
			else {
				while(!fileExists(path)) {
					SDelay(10);
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_tbl);
	ZDELETE(p_dict);
	P_Ep = 0;
	return ok;
}

int DlRtm::FillXmlBuf(const DlScope * pScope, xmlTextWriter * pWriter, StringSet * pDtd, SCodepageIdent cp) const
{
	SFormatParam fp;
	fp.FReal  = MKSFMTD(0, 5, NMBF_NOTRAILZ|NMBF_EXPLFLOAT); // @v9.5.5 MKSFMTD(0, 4, 0)-->MKSFMTD(0, 5, NMBF_NOTRAILZ)
	fp.FDate  = DATF_DMY|DATF_CENTURY;
	fp.Flags |= SFormatParam::fFloatSize;
	SString buf;
	SdbField fld;
	char   xml_entity_spec[256];
	const  char * p_xml_entity_spec = 0;
	{
		buf = DS.GetConstTLA().DL600XMLEntityParam;
		if(buf.NotEmptyS()) {
			buf.CopyTo(xml_entity_spec, sizeof(xml_entity_spec));
			p_xml_entity_spec = xml_entity_spec;
		}
		buf.Z();
	}
	const DlScope * p_scope = 0;
	for(uint j = 0; pScope->EnumInheritance(&j, &p_scope);) {
		for(uint i = 0; p_scope->EnumFields(&i, &fld);) {
			if(pDtd) {
				pDtd->add(fld.Name);
			}
			else {
				fld.GetFieldDataFromBuf(buf, p_scope->GetDataC(0), fp);
				if(fld.T.IsZStr(0)) {
					if(oneof2(cp, cpANSI, cp1251))
						buf.Transf(CTRANSF_INNER_TO_OUTER);
					else if(oneof2(cp, cpOEM, cp866))
						;
					else if(cp == cpUTF8)
						buf.Transf(CTRANSF_INNER_TO_UTF8);
				}
				XMLReplaceSpecSymb(buf, p_xml_entity_spec);
				xmlTextWriterWriteElement(pWriter, fld.Name.ucptr(), buf.ucptr());
			}
		}
	}
	return 1;
}

void DlRtm::FillDTDBuf(const DlScope * pScope, xmlTextWriter * pWriter, const char * pElemName) const
{
	StringSet ss_dtd(',', 0);
	FillXmlBuf(pScope, pWriter, &ss_dtd, SCodepageIdent(cpANSI));
	SString buf;
	SString huge_buf(ss_dtd.getBuf());
	huge_buf.Quot('(', ')');
	xmlTextWriterWriteDTDElement(pWriter, (const xmlChar *)pElemName, huge_buf.ucptr());
	for(uint p = 0; ss_dtd.get(&p, buf) > 0;)
		xmlTextWriterWriteDTDElement(pWriter, buf.ucptr(), (const xmlChar *)"(#PCDATA)");
}

int DlRtm::PutToXmlBuffer(ExportParam & rParam, SString & rBuf)
{
	int    ok = 1;
	int    r;
	const  DlScope * p_data = GetData();
	SString data_name, head_name, suffix, left;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	rBuf.Z();
	data_name = (rParam.Flags & ExportParam::fInheritedTblNames && p_data->GetBase()) ? p_data->GetBase()->Name : p_data->Name;
	head_name = data_name;
	data_name.Dot().Cat("xml");
	THROW(p_xml_buf = xmlBufferCreate());
	THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
	if(rParam.P_F && p_writer) {
		uint   i;
		DlScope * p_child = 0;
		xmlTextWriterSetIndent(p_writer, 0);
		//xmlTextWriterSetIndentTab(writer);
		if(!(rParam.Flags & ExportParam::fDontWriteXmlDTD)) {
			{
				left = 0;
				SCodepageIdent temp_cp = rParam.Cp;
				if(temp_cp == cpANSI)
					temp_cp = cp1251;
				else if(temp_cp == cpOEM)
					temp_cp = cp866;
				else if(temp_cp == cpUndef)
					temp_cp = cpUTF8;
				temp_cp.ToStr(SCodepageIdent::fmtXML, left);
				xmlTextWriterStartDocument(p_writer, 0, left, 0);
			}
			xmlTextWriterStartDTD(p_writer, head_name.ucptr(), 0, 0);
			XMLWriteSpecSymbEntities(p_writer);
			{
				StringSet dtd(',', 0);
				for(i = 0; p_data->EnumChilds(&i, &p_child);) {
					int    is_hdr = 0;
					if(p_child->Name.IsEqiAscii("hdr")) {
						suffix = "Head";
						is_hdr = 1;
					}
					else if(p_child->Name == "iter@def")
						suffix = "Iter";
					else if(p_child->Name.Divide('@', left, suffix) > 0) {
						left.Z().CatChar('_').Cat(suffix);
						suffix = left;
					}
					else
						suffix = p_child->Name;
					if(!is_hdr)
						suffix.CatChar('+');
					dtd.add(suffix);
				}
				suffix.Z().CatParStr(dtd.getBuf());
				xmlTextWriterWriteDTDElement(p_writer, head_name.ucptr(), suffix.ucptr());
			}
			for(i = 0; p_data->EnumChilds(&i, &p_child);) {
				if(p_child->Name.IsEqiAscii("hdr"))
					suffix = "Head";
				else if(p_child->Name == "iter@def")
					suffix = "Iter";
				else if(p_child->Name.Divide('@', left, suffix) > 0) {
					left.Z().CatChar('_').Cat(suffix);
					suffix = left;
				}
				else
					suffix = p_child->Name;
				FillDTDBuf(p_child, p_writer, suffix);
			}
			xmlTextWriterEndDTD(p_writer);
		}
		{
			SXml::WNode hnode(p_writer, head_name);
			for(i = 0; p_data->EnumChilds(&i, &p_child);) {
				int    is_hdr = 0;
				if(p_child->Name.IsEqiAscii("hdr")) {
					suffix = "Head";
					is_hdr = 1;
					THROW(InitData(*rParam.P_F, BIN(rParam.Flags & ExportParam::fIsView)));
				}
				else if(p_child->Name == "iter@def") {
					suffix = "Iter";
				}
				else if(p_child->Name.Divide('@', left, suffix) > 0) {
					left.Z().CatChar('_').Cat(suffix);
					suffix = left;
				}
				else
					suffix = p_child->Name;
				if(is_hdr) {
					SXml::WNode xn(p_writer, suffix);
					FillXmlBuf(p_child, p_writer, 0, rParam.Cp);
				}
				else {
					long   iter_id = GetIterID(p_child->Name);
					THROW(InitIteration(iter_id, rParam.Sort));
					while((r = NextIteration(iter_id)) > 0) {
						SXml::WNode xn(p_writer, suffix);
						FillXmlBuf(p_child, p_writer, 0, rParam.Cp);
					}
				}
			}
		}
		xmlTextWriterEndDocument(p_writer);
		xmlTextWriterFlush(p_writer);
		rBuf.CopyFromN(reinterpret_cast<const char *>(p_xml_buf->content), p_xml_buf->use)/*.UTF8ToChar()*/;
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
	return ok;
}

int DlRtm::ExportXML(ExportParam & rParam, SString & rOutFileName)
{
	int    ok = 1;
	int    r;
	const  DlScope * p_data = GetData();
	SdbField fld;
	SString path, data_name, head_name, suffix;//, left;
	SString temp_buf;
	xmlTextWriter * p_writer = 0;
	char   xml_entity_spec[256];
	const  char * p_xml_entity_spec = 0;
	{
		temp_buf = DS.GetConstTLA().DL600XMLEntityParam;
		if(temp_buf.NotEmptyS()) {
			temp_buf.CopyTo(xml_entity_spec, sizeof(xml_entity_spec));
			p_xml_entity_spec = xml_entity_spec;
		}
		temp_buf.Z();
	}
	rOutFileName.Z();
	data_name = (rParam.Flags & ExportParam::fInheritedTblNames && p_data->GetBase()) ? p_data->GetBase()->Name : p_data->Name;
	head_name = data_name;
	data_name.Dot().Cat("xml");
	PPGetFilePath(PPPATH_OUT, data_name, path);
	p_writer = xmlNewTextWriterFilename(path, (rParam.Flags & ExportParam::fCompressXml) ? 9 : 0); // @v10.6.0 0-->((rParam.Flags & ExportParam::fCompressXml) ? 9 : 0)
	if(rParam.P_F && p_writer) {
		uint   i;
		DlScope * p_child = 0;
		xmlTextWriterSetIndent(p_writer, 1);
		xmlTextWriterSetIndentTab(p_writer);
		// @v9.4.6 xmlTextWriterStartDocument(writer, 0, "windows-1251", 0);
		if(rParam.Cp == cpUndef)
			rParam.Cp = cp1251;
		else if(rParam.Cp == cpANSI)
			rParam.Cp = cp1251;
		rParam.Cp.ToStr(SCodepageIdent::fmtXML, temp_buf); // @v9.4.6
		xmlTextWriterStartDocument(p_writer, 0, temp_buf, 0); // @v9.4.6
		if(!(rParam.Flags & ExportParam::fDontWriteXmlDTD)) {
			xmlTextWriterStartDTD(p_writer, head_name.ucptr(), 0, 0);
			XMLWriteSpecSymbEntities(p_writer);
			{
				StringSet dtd(',', 0);
				for(i = 0; p_data->EnumChilds(&i, &p_child);) {
					int    is_hdr = 0;
					if(p_child->Name.IsEqiAscii("hdr")) {
						suffix = "Head";
						is_hdr = 1;
					}
					else if(p_child->Name == "iter@def")
						suffix = "Iter";
					else if(p_child->Name.Divide('@', temp_buf, suffix) > 0) {
						temp_buf.Z().CatChar('_').Cat(suffix);
						suffix = temp_buf;
					}
					else
						suffix = p_child->Name;
					if(!is_hdr)
						suffix.CatChar('+');
					dtd.add(suffix);
				}
				suffix.Z().CatParStr(dtd.getBuf());
				xmlTextWriterWriteDTDElement(p_writer, head_name.ucptr(), suffix.ucptr());
			}
			for(i = 0; p_data->EnumChilds(&i, &p_child);) {
				if(p_child->Name.IsEqiAscii("hdr"))
					suffix = "Head";
				else if(p_child->Name == "iter@def")
					suffix = "Iter";
				else if(p_child->Name.Divide('@', temp_buf, suffix) > 0) {
					temp_buf.Z().CatChar('_').Cat(suffix);
					suffix = temp_buf;
				}
				else
					suffix = p_child->Name;
				FillDTDBuf(p_child, p_writer, suffix);
			}
			xmlTextWriterEndDTD(p_writer);
		}
		xmlTextWriterStartElement(p_writer, head_name.ucptr());
		if(!(rParam.Flags & ExportParam::fDontWriteXmlTypes)) {
		// @paul (pentaho export types) {
			int    h_i = 0; // счетчик, обработали ли мы уже И "Head" И "Iter"
			StringSet * p_dtd = 0;
			xmlTextWriterStartElement(p_writer, reinterpret_cast<const xmlChar *>("Types"));
			for(i = 0; p_data->EnumChilds(&i, &p_child);) {
				if(h_i >= 2)
					break;
				else if(p_child->Name.IsEqiAscii("hdr")) {
					h_i++;
					suffix = "Head";
				}
				else if(p_child->Name == "iter@def") {
					h_i++;
					suffix = "Iter";
				}
				xmlTextWriterStartElement(p_writer, suffix.ucptr());
				//
				const DlScope * p_scope = 0;
				for(uint j = 0; p_child->EnumInheritance(&j, &p_scope);) {
					for(uint k = 0; p_scope->EnumFields(&k, &fld);) {
						if(p_dtd)
							p_dtd->add(fld.Name);
						else {
							XMLReplaceSpecSymb(GetBinaryTypeString(fld.T.Typ, 0, temp_buf, 0, 0), p_xml_entity_spec);
							xmlTextWriterWriteElement(p_writer, fld.Name.ucptr(), temp_buf.ucptr());
						}
					}
				}
				xmlTextWriterEndElement(p_writer);
				if(h_i >= 2)
					break;
			}
			xmlTextWriterEndElement(p_writer);
		// } @paul (pentaho export types)
		}
		// @erik v10.5.2{
		if(rParam.P_ViewDef) { //надеюсь, список всех entry  отсортирован по Zone. При обратном ничего плохого конечно не случится, но XML будет некрасивый
			const PPNamedFilt::ViewDefinition * p_vd = static_cast<const PPNamedFilt::ViewDefinition *>(rParam.P_ViewDef);
			PPNamedFilt::ViewDefinition::Entry tmp_entry;
			suffix = "ViewDescription";
			xmlTextWriterStartElement(p_writer, suffix.ucptr());
			for(uint i = 0; i < p_vd->GetCount(); i++) {
				if(p_vd->GetEntry(i, tmp_entry)){
					if(oneof2(rParam.Cp, cpANSI, cp1251)){
						tmp_entry.Zone.Transf(CTRANSF_INNER_TO_OUTER);
						tmp_entry.FieldName.Transf(CTRANSF_INNER_TO_OUTER);
						tmp_entry.Text.Transf(CTRANSF_INNER_TO_OUTER);
					}
					else if(rParam.Cp == cpUTF8){
						tmp_entry.Zone.Transf(CTRANSF_INNER_TO_UTF8);
						tmp_entry.FieldName.Transf(CTRANSF_INNER_TO_UTF8);
						tmp_entry.Text.Transf(CTRANSF_INNER_TO_UTF8);
					}
					suffix = "Item";
					xmlTextWriterStartElement(p_writer, suffix.ucptr());
					suffix = "Zone";
					xmlTextWriterWriteElement(p_writer, suffix.ucptr() , tmp_entry.Zone.ucptr());
					suffix = "FieldName";
					xmlTextWriterWriteElement(p_writer, suffix.ucptr(), tmp_entry.FieldName.ucptr());
					suffix = "Text";
					xmlTextWriterWriteElement(p_writer, suffix.ucptr(), tmp_entry.Text.ucptr());
					suffix = "TotalFunc";
					xmlTextWriterWriteElement(p_writer, suffix.ucptr(), temp_buf.Z().Cat(tmp_entry.TotalFunc).ucptr());
					xmlTextWriterEndElement(p_writer);
				}
			}
			xmlTextWriterEndElement(p_writer);
		}
		// } @erik

		for(i = 0; p_data->EnumChilds(&i, &p_child);) {
			int    is_hdr = 0;
			if(p_child->Name.IsEqiAscii("hdr")) {
				suffix = "Head";
				is_hdr = 1;
				THROW(InitData(*rParam.P_F, BIN(rParam.Flags & ExportParam::fIsView)));
			}
			else if(p_child->Name == "iter@def") {
				suffix = "Iter";
			}
			else if(p_child->Name.Divide('@', temp_buf, suffix) > 0) {
				temp_buf.Z().CatChar('_').Cat(suffix);
				suffix = temp_buf;
			}
			else
				suffix = p_child->Name;
			if(is_hdr) {
				xmlTextWriterStartElement(p_writer, suffix.ucptr());
				FillXmlBuf(p_child, p_writer, 0, rParam.Cp);
				xmlTextWriterEndElement(p_writer);
			}
			else {
				long   iter_id = GetIterID(p_child->Name);
				THROW(InitIteration(iter_id, rParam.Sort));
				while((r = NextIteration(iter_id)) > 0) {
					xmlTextWriterStartElement(p_writer, suffix.ucptr());
					FillXmlBuf(p_child, p_writer, 0, rParam.Cp);
					xmlTextWriterEndElement(p_writer);
				}
			}
		}
		xmlTextWriterEndElement(p_writer);
		xmlTextWriterEndDocument(p_writer);
	}
	rOutFileName = path;
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	return ok;
}

int DlRtm::Helper_PutScopeToJson(const DlScope * pScope, json_t * pJsonObj) const
{
	int    ok = 1;
	const  DlScope * p_scope = 0;
	SString buf;
	SdbField fld;
	SFormatParam fp;
	fp.FReal  = MKSFMTD(0, 5, NMBF_NOTRAILZ); // @v9.5.5 MKSFMTD(0, 4, 0)-->MKSFMTD(0, 5, NMBF_NOTRAILZ)
	fp.FDate  = DATF_DMY|DATF_CENTURY;
	fp.Flags |= SFormatParam::fFloatSize;
	THROW(pJsonObj);
	for(uint j = 0; pScope->EnumInheritance(&j, &p_scope);) {
		for(uint i = 0; p_scope->EnumFields(&i, &fld);) {
			fld.GetFieldDataFromBuf(buf, p_scope->GetDataC(0), fp);
			if(fld.T.IsZStr(0))
				buf.Transf(CTRANSF_INNER_TO_OUTER);
			buf.Escape();
			pJsonObj->InsertString(fld.Name.cptr(), buf);
		}
	}
	CATCHZOK
	return ok;
}

int DlRtm::Helper_PutItemToJson(ExportParam & rParam, json_t * pRoot)
{
	int     ok = 1;
	SString left, suffix;
	const   DlScope * p_data = GetData();
	DlScope * p_child = 0;
	json_t  * p_hdr_obj = new json_t(json_t::tOBJECT);
	for(uint i = 0; p_data->EnumChilds(&i, &p_child);) {
		if(p_child->Name.IsEqiAscii("hdr")) {
			//THROW(InitData(*pFilt, 0));
			THROW(InitData(*rParam.P_F, BIN(rParam.Flags & ExportParam::fIsView)));
			Helper_PutScopeToJson(p_child, p_hdr_obj);
		}
		else {
			long iter_id = GetIterID(p_child->Name);
			if(p_child->Name == "iter@def") {
				suffix = "Iter";
			}
			else if(p_child->Name.Divide('@', left, suffix) > 0) {
			}
			else {
				suffix = p_child->Name;
			}
			THROW(InitIteration(iter_id, 0));
			json_t * p_iter_ary = new json_t(json_t::tARRAY);
			while(NextIteration(iter_id) > 0) {
				json_t * p_iter_obj = new json_t(json_t::tOBJECT);
				Helper_PutScopeToJson(p_child, p_iter_obj);
				THROW_SL(json_insert_child(p_iter_ary, p_iter_obj));
			}
			THROW_SL(p_hdr_obj->Insert(suffix.cptr(), p_iter_ary));
		}
	}
	THROW_SL(json_insert_child(pRoot, p_hdr_obj));
	CATCHZOK
	if(!ok)
		ZDELETE(p_hdr_obj);
	return ok;
}

int DlRtm::PutToJsonBuffer(StrAssocArray * pAry, SString & rBuf, int flags)
{
	int    ok = 1;
	json_t * p_root_ary = new json_t(json_t::tARRAY);
	THROW_MEM(p_root_ary);
	THROW(pAry);
	for(uint i = 0, n = pAry->getCount(); i < n; i++) {
		PPFilt filt(pAry->Get(i).Id);
		if(filt.ID > 0) {
			ExportParam ep;
			ep.P_F = &filt;
			THROW(Helper_PutItemToJson(/*&filt*/ep, p_root_ary));
		}
	}
	THROW_SL(json_tree_to_string(p_root_ary, rBuf));
	CATCHZOK
	delete p_root_ary;
	return ok;
}

int DlRtm::PutToJsonBuffer(void * ptr, SString & rBuf, int flags)
{
	int    ok = 1;
	json_t * p_root_ary = new json_t(json_t::tARRAY);
	THROW_MEM(p_root_ary);
	THROW(ptr);
	{
		PPFilt filt(ptr);
		ExportParam ep;
		ep.P_F = &filt;
		THROW(Helper_PutItemToJson(/*&filt*/ep, p_root_ary));
		THROW_SL(json_tree_to_string(p_root_ary, rBuf));
	}
	CATCHZOK
	delete p_root_ary;
	return ok;
}

int DlRtm::PutToJsonBuffer(PPView * pV, SString & rBuf, int flags)
{
	int    ok = 1;
	json_t * p_root_ary = new json_t(json_t::tARRAY);
	THROW_MEM(p_root_ary);
	THROW(pV);
	{
		PPFilt filt(pV);
		ExportParam ep;
		ep.P_F = &filt;
		ep.Flags |= ExportParam::fIsView;
		THROW(Helper_PutItemToJson(/*&filt*/ep, p_root_ary));
		THROW_SL(json_tree_to_string(p_root_ary, rBuf));
	}
	CATCHZOK
	delete p_root_ary;
	return ok;
}
//
// Test
//
#if 0 // {

#include "C:\PAPYRUS\Src\PPTEST\_dd.h"
//
// Implementation of PPALDD_TestRel
//
PPALDD_CONSTRUCTOR(TestRel)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(TestRel) { Destroy(); }

int PPALDD_TestRel::InitData(PPFilt & rFilt, long rsrv)
{
	MEMSZERO(H);
	H.ID = 10;
	SString name = "TestRel_Object_";
	name.Cat(rFilt.ID);
	STRNSCPY(H.Name, name);
	H.Val = 100.5;
	return DlRtm::InitData(rFilt, rsrv);
}
//
// Implementation of PPALDD_Test
//
PPALDD_CONSTRUCTOR(Test)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(Test) { Destroy(); }

int PPALDD_Test::InitData(PPFilt & rFilt, long rsrv)
{
	MEMSZERO(H);
	H.RelID = 11;
	H.I = 1;
	H.J = 5;
	H.K = -100;
	H.L = 1000;
	H.X = 20.;
	H.Y = 40.5;
	// H.Z Не инициализирован
	STRNSCPY(H.S1, "123456");
	STRNSCPY(H.S2, "789");
	return DlRtm::InitData(rFilt, rsrv);
}

int Test_DL6_Rtm()
{
	int    ok = 1;
	SString file_name = "c:\\papyrus\\src\\pptest\\_dd.bin";
	SString out_file_name;
	DlContext ctx;
	PPFilt pf;
	DlRtm * p_rtm = 0;
	THROW(ctx.Init(file_name));
	THROW(ctx.CreateDlRtmInstance("Test", &p_rtm));
	pf.ID = 1;
	THROW(p_rtm->InitData(&pf));
	{
		const DlScopeList & r_list = p_rtm->GetData()->GetChildList();
		SString ext;
		TextDbFile::Param p(TextDbFile::fFldNameRec, ";");
		for(uint i = 0; i < r_list.getCount(); i++) {
			TextDbFile out;
			const DlScope * p_scope = r_list.at(i);
			(ext = p_scope->Name).Cat(".txt");
			SPathStruc::ReplaceExt(out_file_name = file_name, ext, 1);
			THROW(out.Open(out_file_name, &p, 0));
			THROW(out.AppendRecord(*p_scope, p_scope->GetDataC()));
		}
	}
	CATCHZOK
	return ok;
}

#endif // }
//
// Тест вызова интерфейсов
//
#if 0 // {

#include <..\Rsrc\DL600\ppifc_h.h>

int Use001();

int Test_InterfaceCall()
{
	Use001(); // Насильственная линковка модуля, содержащего TestSession
	CoInitialize(0);
	IPapyrusSession * p_obj = 0;
	HRESULT ok = CoCreateInstance(CLSID_PPSession, 0, CLSCTX_INPROC_SERVER, IID_IPapyrusSession, (void **)&p_obj);
	if(ok == S_OK) {
		BSTR db_name = 0;//L"land_store1";
		BSTR user_name = 0;//L"master";
		BSTR passw = 0;//L"123";
		SString temp_buf;
		(temp_buf = "land_store1").CopyToOleStr(&db_name);
		(temp_buf = "master").CopyToOleStr(&user_name);
		(temp_buf = "123").CopyToOleStr(&passw);
		long ret;
		p_obj->Login(db_name, user_name, passw, &ret);
		p_obj->Logout(&ret);
		p_obj->Release();
		SysFreeString(db_name);
		SysFreeString(user_name);
		SysFreeString(passw);
		return 1;
	}
	return -1;
}

#endif // } 0
