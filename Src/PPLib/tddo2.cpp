// TDDO2.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2012, 2013, 2015, 2016, 2017, 2018, 2020
//
#include <pp.h>
#pragma hdrstop

TddoProcessBlock::TddoProcessBlock() : P_Rtm(0)
{
}

#ifndef USE_TDDO_2 // {

int TestTddo2() // @stub
{
	return -1;
}
#else
/*

## #start{__StrAssocArray}

## Это - примечание
#start{UhttPerson}
	<table>

		#start(Person:${ID})
		#end

		<tr>
			<td>ИД:</td><td>${ID}</td>
		</tr>
		<tr>
			<td>Наименование:</td><td>${Name}</td>
		</tr>
		<tr>
			<td>Код:</td><td>${Code}</td>
		</tr>
	</table>
	<table>
		<caption>Относится к видам:</caption>
		#iter{KindList}
			<tr><td>${Name}</td><td>${Code}</td></tr>
		#end{}
	</table>

	#text(file:ident)
	#include(file)

#end{}
*/

/*static*/int Tddo::GetFileName(const char * pFileName, int fileType, const char * pInputFileName, SString & rResult)
{
	int    ok = 0;
	SString path, result;
	rResult = pFileName;
	if(fileType == ftTddo)
		SPathStruc::ReplaceExt(rResult, "tddo", 0);
	else if(fileType == ftTddt)
		SPathStruc::ReplaceExt(rResult, "tddt", 0);
	{
		SString preserve_result = rResult;
		if(pInputFileName) {
			SPathStruc ps(pInputFileName);
			ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, path);
		}
		if(path.NotEmptyS()) {
			SPathStruc::ReplacePath(rResult, path, 0);
			if(fileExists(rResult))
				ok = 1;
		}
		if(!ok) {
			PPIniFile ini_file;
			ini_file.Get(PPINISECT_PATH, PPINIPARAM_TDDOPATH, path);
			if(path.NotEmptyS()) {
				SPathStruc::ReplacePath(rResult, path, 0);
				if(fileExists(rResult))
					ok = 1;
			}
		}
		if(!ok) {
			PPGetPath(PPPATH_DD, path);
			if(path.NotEmptyS()) {
				SPathStruc::ReplacePath(rResult, path, 0);
				if(fileExists(rResult))
					ok = 1;
			}
		}
		if(!ok)
			rResult = preserve_result;
	}
	return (ok || fileExists(rResult)) ? 1 : PPSetErrorSLib();
}

/*static*/int Tddo::LoadFile(const char * pFileName, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	SString temp_buf;
	THROW(Tddo::GetFileName(pFileName, ftTddo, 0, temp_buf));
	{
		SFile in_file(temp_buf, SFile::mRead);
		THROW_SL(in_file.IsValid());
		while(in_file.ReadLine(temp_buf))
			THROW_SL(rBuf.Cat(temp_buf));
	}
	CATCHZOK
	return ok;
}

Tddo::Tddo()
{
	P_Ctx = DS.GetInterfaceContext(PPSession::ctxtExportData);
	Flags = 0;
	LineNo = 0;
	ReH_Meta = 0;
	ReH_Var = 0;
	ReH_VarShort = 0;
	ReH_VarArgN = 0;
	ReH_VarArgNShort = 0;
	ReH_RemLine = 0;
	ReH_If = 0;
	ReH_Elif = 0;
	ReH_String = 0;
	Scan.RegisterRe("^\\#[_a-zA-Z][_0-9a-zA-Z]*\\([^)]*\\)", &ReH_Meta);
	Scan.RegisterRe("^\\#if\\{[^}]*\\}", &ReH_If);
	Scan.RegisterRe("^\\#elif\\{[^}]*\\}", &ReH_Elif);
	Scan.RegisterRe("^\\$\\{[_a-zA-Z][_0-9a-zA-Z]*\\}", &ReH_Var);
	Scan.RegisterRe("^\\$[_a-zA-Z][_0-9a-zA-Z]*", &ReH_VarShort);
	Scan.RegisterRe("^\\$\\{[0-9]+\\}", &ReH_VarArgN);
	Scan.RegisterRe("^\\$[0-9]+", &ReH_VarArgNShort);
	Scan.RegisterRe("^\\#\\#[^\n]*\n", &ReH_RemLine);
	Scan.RegisterRe("^\\#\\@\\([^)]*\\)", &ReH_String);
	Cp = cp1251;
	/*
	Tokens.Add(tRem, "rem");
	Tokens.Add(tStart, "start");
	Tokens.Add(tEnd, "end");
	Tokens.Add(tIter, "iter");
	Tokens.Add(tIter, "codepage");
	*/
}

Tddo::~Tddo()
{
}

void Tddo::SetInputFileName(const char * pFileName)
{
	InputFileName = pFileName;
}

Tddo::Meta::Meta() : Tok(tNone)
{
}

Tddo::Meta & FASTCALL Tddo::Meta::operator = (const Meta & rS)
{
	Tok = rS.Tok;
	Text = rS.Text;
	Param = rS.Param;
	return *this;
}

void Tddo::Meta::Clear()
{
	Tok = tNone;
	Text.Z();
	Param.Z();
}

Tddo::Result::Result() : RefType(0), RefID(0)
{
}

Tddo::Result & Tddo::Result::Clear()
{
	S.Z();
	RefType = 0;
	RefID = 0;
	return *this;
}
// A.B(X().E, D, F.G)
// A.B(X().E, D, F.G) > 5

int Tddo::ResolveExpr(DlRtm * pRtm, const DlScope * pScope, DlRtm * pCallerRtm, SStrScan & rScan, Result & rR)
{
	rR.Clear();
	int    ok = 1, r;
	Result temp_result;
	SString item_name, temp_buf;
	StrAssocArray arg_list;
	rScan.Skip();
	// @v8.7.8 {
	if(pCallerRtm && pCallerRtm != pRtm)
		pRtm->P_Ep = pCallerRtm->P_Ep;
	// } @v8.7.8
	if(rScan.GetIdent(temp_buf)) {
		rScan.Skip();
		char c = rScan[0];
		if(c == '(') {
			STypEx ret_t;
			item_name = temp_buf;
			long   arg_count = 0;
			arg_list.Clear();
			do {
				rScan.Incr();
				if(rScan.Skip()[0] != ')') {
					THROW(ResolveExpr(pCallerRtm, pScope, pCallerRtm, rScan, temp_result)); // @recursion
					if(temp_result.RefID)
						temp_buf.Z().Cat(temp_result.RefID);
					else
						temp_buf = temp_result.S;
					arg_list.Add(++arg_count, temp_buf);
					rScan.Skip();
					c = rScan[0];
				}
				else
					break;
			} while(c == ',');
			rScan.Skip();
			c = rScan[0];
			THROW_PP_S(c == ')', PPERR_TDDO_SYMBEXPECTED, "',' OR ')'");
			rScan.Incr();
			c = rScan[0];
			THROW(P_Ctx->ResolveFunc(pRtm, pScope, 0, item_name, arg_list, rR.S, ret_t)); // @v8.7.10 exactScope 1-->0
			if(ret_t.Mod == STypEx::modLink && ret_t.Link) {
				const DlScope * p_scope = P_Ctx->GetScope_Const(ret_t.Link, 0);
				THROW(p_scope);
				rR.RefType = ret_t.Link;
				rR.RefID = rR.S.ToLong();
				(rR.S = p_scope->GetName()).CatChar(':').Cat(rR.RefID);
			}
		}
		else {
			item_name = temp_buf;
			THROW(ResolveVar(item_name, pScope, rR));
		}
		if(c == '.') {
			PPFilt pf(rR.RefID);
			rScan.Incr();
			THROW_PP_S(rR.RefType, PPERR_TDDO_DOTOPNOTLINK, item_name);
			DlRtm * p_rtm = P_Ctx->GetRtm(rR.RefType);
			THROW(p_rtm);
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
			THROW(ResolveExpr(p_rtm, p_rtm->GetHdrScope(), pCallerRtm, rScan, rR)); // @recursion (set caller scope)
		}
	}
	else if(rScan[0] == '@') {
		const DlScope * p_scope = 0;
		THROW(P_Ctx);
		rScan.Incr();
		char c = rScan.Skip()[0];
		THROW_PP_S(c == '(', PPERR_TDDO_SYMBEXPECTED, "'('");
		rScan.Incr();
		c = rScan.Skip()[0];
		THROW_PP(rScan.GetIdent(temp_buf), PPERR_TDDO_DL6SYMBEXPECTED);
		THROW_PP_S(p_scope = P_Ctx->GetScopeByName_Const(DlScope::kExpData, temp_buf), PPERR_TDDO_UNDEFDATANAME, temp_buf);
		c = rScan.Skip()[0];
		THROW_PP_S(c == ',', PPERR_TDDO_SYMBEXPECTED, "','");
		rScan.Incr();
		c = rScan.Skip()[0];
		THROW(ResolveExpr(pCallerRtm, pScope, pCallerRtm, rScan, temp_result)); // @recursion
		{
			rR.RefType = p_scope->GetId();
			rR.RefID = NZOR(temp_result.RefID, temp_result.S.ToLong());
			(rR.S = p_scope->GetName()).CatChar(':').Cat(rR.RefID);
		}
		c = rScan.Skip()[0];
		THROW_PP_S(c == ')', PPERR_TDDO_SYMBEXPECTED, "')'");
		rScan.Incr();
		c = rScan.Skip()[0];
		if(c == '.') {
			PPFilt pf(rR.RefID);
			rScan.Incr();
			THROW_PP_S(rR.RefType, PPERR_TDDO_DOTOPNOTLINK, item_name);
			DlRtm * p_rtm = P_Ctx->GetRtm(rR.RefType);
			THROW(p_rtm);
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
			THROW(ResolveExpr(p_rtm, p_rtm->GetHdrScope(), pCallerRtm, rScan, rR)); // @recursion (set caller scope)
		}
	}
	else if(rScan.GetQuotedString(temp_buf)) {
		rR.S = temp_buf;
	}
	else if(rScan.GetNumber(temp_buf)) {
		rR.S = temp_buf;
	}
	else if(rScan[0] == '$') {
		rScan.Incr();
		if(rScan.GetNumber(temp_buf)) {
			ResolveArgN(temp_buf, rR);
		}
	}
	else {
		CALLEXCEPT_PP_S(PPERR_TDDO_EXPR, (const char *)rScan);
	}
	//
	//
	//
	{
		struct _COp {
			_COp()
			{
				Op = _NONE_; //  _opNone;
				Inc = 0;
			}
			void FASTCALL Set(int op, int inc)
			{
				Op = op;
				Inc = inc;
			}
			int    Op;
			uint   Inc;
		};
		_COp cop;
		rScan.Skip();
		if(rScan.Is(">="))
			cop.Set(dlopGe, 2);
		else if(rScan.Is("<="))
			cop.Set(dlopLe, 2);
		else if(rScan.Is("=="))
			cop.Set(dlopEq, 2);
		else if(rScan.Is("!="))
			cop.Set(dlopNeq, 2);
		else if(rScan.Is("<>"))
			cop.Set(dlopNeq, 2);
		else if(rScan.Is('>'))
			cop.Set(dlopGt, 1);
		else if(rScan.Is('<'))
			cop.Set(dlopLt, 1);
		else if(rScan.Is('='))
			cop.Set(dlopEq, 1);
		if(cop.Op != _NONE_) {
			int    done = 0;
			int    _r = 0;
			SStrScan et_scan;
			rScan.Incr(cop.Inc);
			THROW(ResolveExpr(pCallerRtm, pScope, pCallerRtm, rScan, temp_result)); // @recursion
			et_scan.Set(rR.S, 0);
			if(et_scan.GetNumber(temp_buf)) {
				double a1 = temp_buf.ToReal();
				et_scan.Set(temp_result.S, 0);
				if(et_scan.GetNumber(temp_buf)) {
					double a2 = temp_buf.ToReal();
					switch(cop.Op) {
						case dlopGt: _r = (a1 > a2); break;
						case dlopLt: _r = (a1 < a2); break;
						case dlopGe: _r = (a1 >= a2); break;
						case dlopLe: _r = (a1 <= a2); break;
						case dlopEq: _r = (a1 == a2); break;
						case dlopNeq: _r = (a1 != a2); break;
					}
					done = 1;
				}
			}
			if(!done) {
				const int sc = rR.S.CmpNC(temp_buf);
				switch(cop.Op) {
					case dlopGt: _r = (sc > 0); break;
					case dlopLt: _r = (sc < 0); break;
					case dlopGe: _r = (sc >= 0); break;
					case dlopLe: _r = (sc <= 0); break;
					case dlopEq: _r = (sc == 0); break;
					case dlopNeq: _r = (sc != 0); break;
				}
			}
			rR.Clear().S.Cat(BIN(_r));
		}
	}
	CATCHZOK
	// @v8.7.8 {
	if(pCallerRtm && pCallerRtm != pRtm)
		pRtm->P_Ep = pCallerRtm->P_Ep;
	// } @v8.7.8
	return ok;
}

int Tddo::GetVar(const SString & rInput, SString & rBuf) const
{
	uint   i = 1;
	int    c;
	rBuf.Z();
	if(rInput.C(i) == '{')
		i++;
	for(; (c = rInput.C(i)) != '}' && c != 0; i++)
		rBuf.CatChar(c);
	return 1;
}

void Tddo::Skip()
{
	Scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine, &LineNo);
}

int Tddo::Process(const char * pDataName, const char * pBuf, DlRtm::ExportParam & rEp, const StringSet * pExtParamList, SBuffer & rOut)
{
	int    ok = 1;
	LineNo = 1;
	if(!RVALUEPTR(ExtParamList, pExtParamList))
		ExtParamList.clear();
	TddoProcessBlock pblk;
	pblk.SrcDataName = pDataName;
	pblk.P_Rtm = 0;
	pblk.F.ID = rEp.P_F ? rEp.P_F->ID : 0;
	pblk.F.Ptr = rEp.P_F ? rEp.P_F->Ptr : 0;
	pblk.Ep = rEp;
	Scan.Set(pBuf, 0);
	Meta meta;
	meta.Tok = tNone;
	THROW(Helper_Process(pblk, rOut, meta, 0, 0));
	CATCHZOK
	return ok;
}

int Tddo::ResolveArgN(const SString & rText, Result & rR)
{
	rR.Clear();
	int    ok = -1;
	long   argn = rText.ToLong();
	long   n = 0;
	SString temp_buf;
	for(uint p = 0; ok < 0 && ExtParamList.get(&p, temp_buf);) {
		n++;
		if(n == argn) {
			rR.S = temp_buf;
			ok = 1;
		}
	}
	return ok;
}

int Tddo::ResolveVar(const SString & rText, const DlScope * pScope, Result & rR)
{
	rR.Clear();
	int    ok = 1;
	SString temp_buf;
	if(rText == "__FILE__") {
		rR.S = InputFileName;
	}
	else if(rText == "__FILEDIR__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps(InputFileName);
			ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, rR.S);
			rR.S.SetLastSlash();
		}
	}
	else if(rText == "__FILENAME__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps(InputFileName);
			rR.S = ps.Nam;
		}
	}
	else if(rText == "__FILEEXT__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps(InputFileName);
			rR.S = ps.Ext;
		}
	}
	else if(rText == "__LINE__") {
		rR.S.Cat(LineNo);
	}
	else if(rText == "__DATE__") {
		rR.S.Cat(getcurdate_(), DATF_DMY|DATF_CENTURY);
	}
	else if(rText == "__TIME__") {
		rR.S.Cat(getcurtime_(), TIMF_HMS);
	}
	else {
		CtmVar cv;
		SdbField fld;
		SFormatParam fp;
		THROW_PP(pScope, PPERR_TDDO_ZEROVARSCOPE);
		THROW(P_Ctx->ResolveVar(pScope, 1, rText, &cv));
		const DlScope * p_var_scope = P_Ctx->GetEvaluatedVarScope(pScope, cv.ScopeID);
		THROW_PP_S(p_var_scope, PPERR_TDDO_INVFLDSCOPE, temp_buf.Z().Cat(pScope->GetName()).Space().Cat(cv.ScopeID));
		fp.FReal  = MKSFMTD(0, 4, 0);
		fp.FDate  = DATF_DMY|DATF_CENTURY;
		fp.Flags |= SFormatParam::fFloatSize;
		THROW_PP_S(p_var_scope->GetFieldByPos(cv.Pos, &fld), PPERR_TDDO_INVFLDPOS, temp_buf.Z().Cat(cv.Pos));
		fld.GetFieldDataFromBuf(rR.S, p_var_scope->GetDataC(0), fp);
		if(fld.T.Mod == STypEx::modLink && fld.T.Link) {
			const DlScope * p_scope = P_Ctx->GetScope_Const(fld.T.Link, 0);
			THROW(p_scope);
			rR.RefType = fld.T.Link;
			rR.RefID = rR.S.ToLong();
			(rR.S = p_scope->GetName()).CatChar(':').Cat(rR.RefID);
		}
		else {
			if(oneof3(Cp, cp1251, cpUndef, cpANSI))
				rR.S.Transf(CTRANSF_INNER_TO_OUTER);
			else if(Cp == cpUTF8)
				rR.S.Transf(CTRANSF_INNER_TO_UTF8);
		}
	}
	CATCH
		ok = 0;
		PPGetLastErrorMessage(1, rR.S);
	ENDCATCH
	return ok;
}

int DlContext::ResolveFunc(DlRtm * pRtm, const DlScope * pScope, int exactScope, const char * pFuncName, StrAssocArray & rArgList, SString & rResult, STypEx & rT)
{
	int    ok = -1;
	const  uint   cur_pos = S.GetCurPos();
	uint   i, j;
	CtmFunc rf;
	SString func_name, arg_buf;
	func_name.CatChar('?').Cat(pFuncName);
	{
		LongArray fpl[3]; // func pos list
		const DlScope * p_org_scope = pScope;
		const DlScope * p_scope = p_org_scope;
		while(ok < 0 && p_scope) {
			uint   pos = 0;
			uint   prev_scope_kind = p_scope->GetKind();
			DlFunc func;
			int    ambig[3]; // Признаки неоднозначности разрешения функции
			int    s_[3];
			memzero(ambig, sizeof(ambig));
			s_[0] = s_[1] = s_[2] = -1;
			LongArray func_pos_list;
			LongArray cvt_arg_list, cvt_al[3];
			p_scope->GetFuncListByName(func_name, &func_pos_list);
			for(i = 0; i < func_pos_list.getCount(); i++) {
				int    s = tcrUnable;
				THROW_PP(p_scope->GetFuncByPos(func_pos_list.at(i), &func), PPERR_DL6_NOFUNCBYPOS);
				{
					const  uint arg_count = func.GetArgCount();
					if(arg_count == rArgList.getCount()) {
						s = tcrCast;
					}
				}
				if(s > 0)
					for(j = 0; j < 3; j++) {
						if(s == (j+1)) {
							//
							// Так как неоднозначность может встретиться на менее
							// предпочтительном уровне преобразования аргументов,
							// сохраним информацию о неоднозначности для того, чтобы
							// иметь возможность подобрать более предпочтительную
							// однозначную функцию с точки зрения преобразования аргументов.
							//
							if(s_[j] >= 0)
								ambig[j] = 1;
							else {
								s_[j] = i;
								cvt_al[j] = cvt_arg_list;
								break;
							}
						}
					}
			}
			if(s_[0] >= 0 || s_[1] >= 0 || s_[2] >= 0) {
				rf.ScopeID = p_scope->ID;
				for(i = 0; ok < 0 && i < 3; i++)
					if(s_[i] >= 0) {
						THROW_PP(ambig[i] == 0, PPERR_DL6_FUNCAMBIG);
						rf.Pos = func_pos_list.at(s_[i]);
						THROW_PP(p_scope->GetFuncByPos(rf.Pos, &func), PPERR_DL6_NOFUNCBYPOS);
						{
							uint   arg_no = 0;
							size_t slen = 0;
							const  uint arg_count = func.GetArgCount();
							SV_Uint32 arg_pos_list;
							arg_pos_list.Init();
							TypeEntry ret_te;
							const  size_t ret_sp = AllocStackType(func.TypID, &ret_te);
							arg_pos_list.Add(ret_sp);
							for(j = 0; j < arg_count; j++) {
								arg_no++;
								TypeEntry te;
								DLSYMBID arg_type_id = func.GetArgType(j);
								THROW(SearchTypeID(arg_type_id, 0, &te));
								const  size_t arg_sp = AllocStackType(arg_type_id, &te);
								arg_pos_list.Add(arg_sp);
								rArgList.Get(arg_no, arg_buf);
								if(te.T.IsZStr(&(slen = 0))) {
									SString * p_str = *static_cast<SString **>(S.GetPtr(arg_sp));
									assert(p_str);
									*p_str = arg_buf;
								}
								else {
									stfromstr(te.T.Typ, S.GetPtr(arg_sp), 0, arg_buf.cptr());
								}
							}
							if(func.ImplID) {
								THROW(BuiltinOp(&func, &arg_pos_list));
							}
							else {
								pRtm->EvaluateFunc(&func, &arg_pos_list, S);
							}
							//
							rT = ret_te.T;
							if(ret_te.T.IsZStr(&(slen = 0))) {
								SString * p_str = *static_cast<SString **>(S.GetPtr(ret_sp));
								assert(p_str);
								rResult = *p_str;
							}
							else {
								const  TYPEID st = ret_te.T.GetDbFieldType();
								char   temp_buf[1024];
								temp_buf[0] = 0;
								sttostr(st, S.GetPtr(ret_sp), 0, temp_buf);
								rResult = temp_buf;
							}
						}
						ok = 1;
					}
			}
			else {
				if(p_scope->GetKind() == DlScope::kExpData) {
					//
					// Обращение к функции может быть связано с заголовочной областью kExpData
					// в то время как физически функция определена в одной из внутренних областей
					// вида kExpDataHdr.
					//
					uint   pos = 0;
					DlScope * p_child = 0;
					for(uint i = 0; ok < 0 && p_scope->EnumChilds(&i, &p_child);)
						if(p_child->GetKind() == DlScope::kExpDataHdr) {
							ok = ResolveFunc(pRtm, p_child, 1, pFuncName, rArgList, rResult, rT); // @recursion
							if(!ok && PPErrCode == PPERR_DL6_NOFUNCBYNAME)
								ok = -1;
						}
				}
				if(ok < 0) {
					THROW_PP_S(!exactScope, PPERR_DL6_NOFUNCBYNAME, func_name);
					p_scope = NZOR(p_scope->GetBase(), p_scope->GetOwner());
				}
			}
		}
	}
	THROW_PP(ok > 0, PPERR_DL6_NOFUNCBYNAME);
	CATCHZOK
	ReleaseStack(cur_pos);
	return ok;
}

int Tddo::IsTextSection(const SString & rLineBuf, const char * pPattern, SString * pRet)
{
	int    ok = -1;
	SString sect_name;
	SStrScan scan(rLineBuf, 0);
	scan.Skip();
	if(*scan == '[') {
		scan.Incr(1);
		if(scan.SearchChar(']')) {
			scan.Get(sect_name);
			if(sect_name.NotEmptyS()) {
				ok = (pPattern && sect_name.CmpNC(pPattern) == 0) ? 2 : 1;
				ASSIGN_PTR(pRet, sect_name);
			}
		}
	}
	return ok;
}

int Tddo::ExtractText(const char * pFileName, const char * pTextIdent, int langId, SBuffer & rOut)
{
	int    ok = -1;
	SString temp_buf, sect_buf, defsect_buf;
	if(isempty(pFileName)) {
		THROW(Tddo::GetFileName("common", ftTddt, InputFileName, temp_buf));
	}
	else {
		THROW(Tddo::GetFileName(pFileName, ftTddt, InputFileName, temp_buf));
	}
	if(ok) {
		int    this_sect = 0;
		SFile in_file(temp_buf, SFile::mRead);
		THROW_SL(in_file.IsValid());
		if(langId)
			GetLinguaCode(langId, temp_buf);
		else
			temp_buf.Z();
		(sect_buf = pTextIdent).Strip();
		if(temp_buf.NotEmpty()) {
			defsect_buf = sect_buf;
			sect_buf.CatChar(':').Cat(temp_buf);
		}
		while(in_file.ReadLine(temp_buf)) {
			int    s = IsTextSection(temp_buf, sect_buf, 0);
			if(s == 2) {
				if(this_sect)
					break;
				else {
					this_sect = 1;
					ok = 1;
				}
			}
			else if(s == 1) {
				if(this_sect)
					break;
				else
					this_sect = 0;
			}
			else if(this_sect) {
				rOut.Write(temp_buf.cptr(), temp_buf.Len());
			}
		}
	}
	CATCHZOK
	return ok;
}

/*int Tddo::Helper_RecognizeExprToken(long flags, SString & rText)
{
	int    t = 0;
	char   text[512];
	size_t tp = 0;
	size_t sp = 0;
	char   c = Scan[0];
	if(c == '$') {
		c = Scan[++sp];
		if(c == '{') {
			c = Scan[++sp];
			if(isdec(c)) {
				//
				// ? Номер внешнего аргумента вида ${1}
				//
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(isdec(c));
				text[tp] = 0;
				if(c == '}') {
					Scan.Incr(tp+3); // ${}
					rText = text;
					t = tVarArgN;
				}
			}
			else if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				//
				// ? Простая переменная вида ${var}
				//
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
				text[tp] = 0;
				if(c == '}') {
					Scan.Incr(tp+3); // ${}
					rText = text;
					t = tVar;
				}
			}
			if(!t) {
				//
				// Скорее всего (но не обязательно) начало сложного выражения.
				// Например: ${util.Goods.getSingleBarcode()}
				// Функция разбора выражения должны быть вызвана рекурсивно для разбора содержимого
				// с сигналом финаша '}'
				//
				t = tDollarBrace;
			}
		}
		else if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			do {
				text[tp++] = c;
				c = Scan[++sp];
			} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
			text[tp] = 0;
			Scan.Incr(tp+1); // $
			rText = text;
			t = tVar;
		}
	}
	else if(c == '@' && Scan[1] == '{') {
		c = Scan[++sp];
		if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
			do {
				text[tp++] = c;
				c = Scan[++sp];
			} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
			text[tp] = 0;
			if(c == '}') {
				Scan.Incr(tp+3); // # + { + }
				rText = text;
				t = tString;
			}
			else {
				; // @suspicious
			}
		}
	}
	else if(!(flags & rexptfMetaOnly)) {
		if(Scan.Is("&&")) {
			Scan.Incr(2);
			rText.Z().Cat("&&");
			t = tOperator+_AND_;
		}
		else if(Scan.Is("and")) {
			Scan.Incr(3);
			rText.Z().Cat("&&");
			t = tOperator+_AND_;
		}
		else if(Scan.Is("||")) {
			Scan.Incr(2);
			rText.Z().Cat("||");
			t = tOperator+_OR_;
		}
		else if(Scan.Is("or")) {
			Scan.Incr(2);
			rText.Z().Cat("||");
			t = tOperator+_OR_;
		}
		else if(Scan.Is("==")) {
			Scan.Incr(2);
			rText.Z().Cat("==");
			t = tOperator+dlopEq;
		}
		else if(Scan.Is("!=") || Scan.Is("<>")) {
			Scan.Incr(2);
			rText.Z().Cat("!=");
			t = tOperator+dlopNeq;
		}
		else if(Scan.Is(">=")) {
			Scan.Incr(2);
			rText.Z().Cat(">=");
			t = tOperator+dlopGe;
		}
		else if(Scan.Is("<=")) {
			Scan.Incr(2);
			rText.Z().Cat("<=");
			t = tOperator+dlopLe;
		}
		else if(oneof9(c, '+', '-', '*', '/', '%', '<', '>', '=', '.')) {
			Scan.Incr();
			rText.Z().CatChar(c);
			switch(c) {
				case '+': t = tOperator + dlopAdd; break;
				case '-': t = tOperator + dlopSub; break;
				case '*': t = tOperator + dlopMul; break;
				case '/': t = tOperator + dlopDiv; break;
				case '%': t = tOperator + dlopMod; break;
				case '<': t = tOperator + dlopLt; break;
				case '>': t = tOperator + dlopGt; break;
				case '=': t = tOperator + dlopAssignment; break;
				case '.': t = tOperator + dlopDot; break;
			}
			assert(t > tOperator);
		}
		else if(c == '\"') {
			c = Scan[++sp];
			while(c && c != '\"') {
				if(c == '\\' && Scan[sp] == '\"') {
					text[tp++] = '\"';
					sp++;
				}
				else
					text[tp++] = c;
				c = Scan[++sp];
			}
			text[tp] = 0;
			if(c == '\"') {
				Scan.Incr(sp);
				rText = text;
				t = tLiteral;
			}
		}
		else if(Scan.IsNumber()) {
			Scan.GetNumber(rText);
			t = tNumber;
		}
	}
	return t;
}*/

int Tddo::Helper_RecognizeMetaKeyword()
{
	int    m = 0;
	char   text[512];
	size_t tp = 0;
	size_t sp = 0;
	char   c;
	if(Scan[0] == '#') {
		c = Scan[++sp];
		if(c == '#') { // Пропускаем строчные комментарии
			Scan.Incr(2);
			while(Scan[0] && !Scan.IsEol(eolUndef))
				Scan.Incr();
			if(Scan.GetEol(eolUndef))
				LineNo++;
			m = Helper_RecognizeMetaKeyword(); // @recusion
		}
		else if(c == '*') { // Пропускаем многострочные комментарии
			Scan.Incr(2);
			while(Scan[0] && !Scan.Is("*#")) {
				if(Scan.GetEol(eolUndef))
					LineNo++;
				else
					Scan.Incr();
			}
			if(Scan.Is("*#"))
				Scan.Incr(2);
			m = Helper_RecognizeMetaKeyword(); // @recusion
		}
		else {
			int    recogn_result = 0;
			tp = 0;
			if(c == '{') {
				c = Scan[++sp];
				if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
					do {
						text[tp++] = c;
						c = Scan[++sp];
					} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
					text[tp] = 0;
					if(c == '}') {
						recogn_result = 200;
					}
					else {
						recogn_result = -1; // suspicious
					}
				}
			}
			else if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
				text[tp] = 0;
				recogn_result = 100;
			}
			if(recogn_result > 0) {
				if(sstreq(text, "macro"))
					m = tMacro;
				else if(sstreq(text, "set"))
					m = tSet;
				else if(sstreq(text, "if"))
					m = tIf;
				else if(sstreq(text, "else"))
					m = tElse;
				else if(sstreq(text, "elseif") || sstreq(text, "elif"))
					m = tElif;
				else if(sstreq(text, "foreach"))
					m = tForEach;
				else if(sstreq(text, "endif"))
					m = tEndif;
				else if(sstreq(text, "end"))
					m = tEnd;
				else if(sstreq(text, "codepage"))
					m = tCodepage;
				else if(sstreq(text, "pragma"))
					m = tPragma;
				else if(sstreq(text, "start"))
					m = tStart;
				else if(sstreq(text, "rem"))
					m = tRem;
				else if(sstreq(text, "iter"))
					m = tIter;
				else if(sstreq(text, "itercount"))
					m = tIterCount;
				else if(sstreq(text, "text"))
					m = tText;
				else if(sstreq(text, "include"))
					m = tInclude;
				if(m) {
					if(recogn_result == 200)
						Scan.Incr(tp+3); // # + { + }
					else
						Scan.Incr(tp+1); // #
				}
			}
		}
	}
	return m;
}

int FASTCALL Tddo::ScanMeta(Meta & rM)
{
	rM.Clear();
	int    ok = 1;
	const  char fc = Scan[0];
	if(fc != '#' && fc != '$') { // Так как все мета-символы начинаются с # или $ оптимизируем функцию предварительной проверкой.
		ok = -1;
	}
	else {
		const  char * s_ = 0;
		int    m = Helper_RecognizeMetaKeyword();
	}
	return ok;
}

int Tddo::Helper_Process(TddoProcessBlock & rBlk, SBuffer & rOut, Meta & rMeta, const DlScope * pScope, int skipOutput)
{
	int    ok = 1;
	uint   prev_scan_stack_pos = 0;
	SString temp_buf, msg_buf;
	Meta   meta;
	Result result;
	long   iter_id = 0;
	uint32 iter_count = 0;
	int    skip = BIN(skipOutput);
	TSStack <int> if_stack;
	do {
		if(!iter_id) {
			if(rMeta.Tok == tNone) {

			}
			else if(rMeta.Tok == tIter) {
				assert(rBlk.P_Rtm);
				temp_buf.Z().Cat("iter").CatChar('@').Cat(rMeta.Text.NotEmpty() ? rMeta.Text : "def");
				iter_id = rBlk.P_Rtm->GetIterID(temp_buf);
				THROW_PP_S(iter_id, PPERR_TDDO_UNDEFITERNAME, temp_buf);
				{
					const DlScope * p_loc_scope = pScope->SearchByName_Const(DlScope::kExpDataIter, temp_buf, 0);
					THROW_PP_S(p_loc_scope, PPERR_TDDO_UNDEFITERNAME, temp_buf);
					pScope = p_loc_scope;
				}
				THROW(rBlk.P_Rtm->InitIteration(iter_id, rBlk.Ep.Sort));
				if(rBlk.P_Rtm->NextIteration(iter_id) > 0)
					iter_count++;
				else
					skip++;
			}
		}
		if(iter_id) {
			Scan.Push(&prev_scan_stack_pos);
		}
		for(int exit_loop = 0; !exit_loop && !isempty((const char *)Scan);) {
			int    r = ScanMeta(meta);
			if(r > 0) {
				switch(meta.Tok) {
					// #pragma(htmlencode)
					case tPragma:
						if(meta.Text.CmpNC("htmlencode") == 0)
							Flags |= fHtmlEncode;
						else if(meta.Text.CmpNC("-htmlencode") == 0)
							Flags &= ~fHtmlEncode;
						break;
					case tCodepage:
						if(!Cp.FromStr(meta.Text)) {
							PPGetMessage(mfError, PPERR_INVCODEPAGE, meta.Text, 1, temp_buf);
							ErrMsgList.add(temp_buf, 0);
						}
						break;
					case tStart:
						{
							TddoProcessBlock pblk;
							THROW(P_Ctx);
							const DlScope * p_scope = P_Ctx->GetScopeByName_Const(DlScope::kExpData, meta.Text);
							THROW_PP_S(p_scope, PPERR_TDDO_UNDEFDATANAME, meta.Text);
							pblk.Ep.OutputFormat = rBlk.Ep.OutputFormat; // @v8.8.3
							if(meta.Param.NotEmptyS()) {
								const char * p_preserve_buf = Scan.GetBuf();
								Scan.Push();
								Scan.Set(meta.Param, 0);
								Meta m;
								ScanMeta(m);
								if(m.Tok == tVar) {
									ResolveVar(m.Text, pScope, result);
									pblk.F.ID = result.RefType ? result.RefID : result.S.ToLong();
								}
								else if(m.Tok == tVarArgN) {
									ResolveArgN(m.Text, result);
									pblk.F.ID = result.S.ToLong();
								}
								else
									pblk.F.ID = m.Text.ToLong();
								pblk.Ep.Sort = 0; // @v8.7.8
								pblk.Ep.Flags = 0; // @v8.7.8
								Scan.Set(p_preserve_buf, 0);
								Scan.Pop();
							}
							else if(rBlk.SrcDataName.NotEmpty()) {
								int    r = 0;
								pblk.F = rBlk.F;
								pblk.Ep.Sort = rBlk.Ep.Sort; // @v8.7.8
								pblk.Ep.Flags = rBlk.Ep.Flags; // @v8.7.8
								if(rBlk.SrcDataName.CmpNC(p_scope->GetName()) == 0)
									r = 1;
								else {
									while(!r && (p_scope = p_scope->GetBase()) != 0) {
										if(rBlk.SrcDataName.CmpNC(p_scope->GetName()) == 0)
											r = 1;
									}
								}
								THROW_PP_S(r, PPERR_TDDO_INVTEMPLDATA, meta.Text);
							}
							else {
								pblk.Ep.Sort = 0; // @v8.7.8
								pblk.Ep.Flags = 0; // @v8.7.8
							}
							{
								const DlScope * p_scope = 0;
								THROW_PP_S(p_scope = P_Ctx->GetScopeByName_Const(DlScope::kExpData, meta.Text), PPERR_TDDO_UNDEFDATANAME, meta.Text);
								pblk.P_Rtm = P_Ctx->GetRtm(p_scope->GetId());
								pblk.P_Rtm->P_Ep = &rBlk.Ep; // @v8.7.8
								THROW(pblk.P_Rtm->InitData(pblk.F, BIN(pblk.Ep.Flags & DlRtm::ExportParam::fIsView)));
								pblk.SrcDataName = meta.Text;
								pblk.Ep.P_F = &pblk.F;
								// @v8.7.8 pblk.Ep.Sort = 0;
								// @v8.7.8 pblk.Ep.Flags = 0;
								THROW(Helper_Process(pblk, rOut, meta, pblk.P_Rtm->GetData(), skip)); // @recursion
							}
						}
						break;
					case tIter:
						THROW(Helper_Process(rBlk, rOut, meta, pScope, skip)); // @recursion
						break;
					case tEnd:
						THROW_PP(oneof2(rMeta.Tok, tStart, tIter), PPERR_TDDO_UNEXPECTED_END);
						exit_loop = 1;
						break;
					case tVar:
						ResolveVar(meta.Text, pScope, result);
						if(!skip) {
							if(Flags & fHtmlEncode)
								result.S.ReplaceSpecSymb(SFileFormat::Html);
							else if(!!rBlk.Ep.OutputFormat)
								result.S.ReplaceSpecSymb(rBlk.Ep.OutputFormat);
							rOut.Write(result.S.cptr(), result.S.Len());
						}
						break;
					case tIterCount:
						if(!skip) {
							result.S.Z().Cat(iter_count);
							rOut.Write(result.S.cptr(), result.S.Len());
						}
						break;
					case tVarArgN:
						ResolveArgN(meta.Text, result);
						if(!skip) {
							if(Flags & fHtmlEncode)
								result.S.ReplaceSpecSymb(SFileFormat::Html);
							else if(!!rBlk.Ep.OutputFormat)
								result.S.ReplaceSpecSymb(rBlk.Ep.OutputFormat);
							rOut.Write(result.S.cptr(), result.S.Len());
						}
						break;
					case tExpr:
						{
							SStrScan inner_scan(meta.Text);
							THROW(ResolveExpr(rBlk.P_Rtm, pScope, rBlk.P_Rtm, inner_scan, result));
							if(!skip) {
								if(Flags & fHtmlEncode)
									result.S.ReplaceSpecSymb(SFileFormat::Html);
								else if(!!rBlk.Ep.OutputFormat)
									result.S.ReplaceSpecSymb(rBlk.Ep.OutputFormat);
								rOut.Write(result.S.cptr(), result.S.Len());
							}
						}
						break;
					case tIf:
						{
							SStrScan inner_scan(meta.Text);
							THROW(ResolveExpr(rBlk.P_Rtm, pScope, rBlk.P_Rtm, inner_scan, result));
							int    ir = (result.S.ToLong() != 0);
							if(!ir)
								skip++;
							if_stack.push(ir);
						}
						break;
					case tElse:
						{
							int    ir = 0;
							THROW_PP(if_stack.getPointer(), PPERR_TDDO_MISSPLACEDELSE);
							if_stack.pop(ir);
							if(!ir)
								skip--;
							ir = BIN(!ir);
							if(!ir)
								skip++;
							if_stack.push(ir);
						}
						break;
					case tElif:
						{
							int    ir = 0;
							THROW_PP(if_stack.peek(), PPERR_TDDO_MISSPLACEDELIF);
							if_stack.pop(ir);
							if(!ir)
								skip--;
							//
							SStrScan inner_scan(meta.Text);
							THROW(ResolveExpr(rBlk.P_Rtm, pScope, rBlk.P_Rtm, inner_scan, result));
							//
							ir = (result.S.ToLong() != 0);
							if(!ir)
								skip++;
							if_stack.push(ir);
						}
						break;
					case tEndif:
						{
							int    ir = 0;
							if_stack.pop(ir);
							if(!ir)
								skip--;
						}
						break;
					case tString:
						PPLoadString(meta.Text, result.S);
						result.S.Transf(CTRANSF_INNER_TO_OUTER);
						rOut.Write(result.S.cptr(), result.S.Len());
						break;
					case tText:
						ExtractText(meta.Param, meta.Text, 0 /* languageId */, rOut);
						break;
				}
			}
			else {
				char cc = Scan[0];
				if(!skip)
					rOut.WriteByte(cc);
				if(cc == '\xD' && Scan[1] == '\xA') {
					Scan.Incr();
					LineNo++;
				}
				else if(oneof2(cc, '\n', '\r'))
					LineNo++;
				/*if(cc == '\n')
					LineNo++;*/
				Scan.Incr();
			}
		}
		if(iter_id && rBlk.P_Rtm->NextIteration(iter_id) > 0) {
			int rp = Scan.Pop(prev_scan_stack_pos);
			assert(rp);
			iter_count++;
		}
		else
			break;
	} while(1);
	if(if_stack.getPointer() != 0) {
		temp_buf = "Error: Unfinished IF";
		rOut.Write(temp_buf.cptr(), temp_buf.Len());
	}
	CATCHZOK
	return ok;
}

struct Tddo2_OpInfo {
	uint8  Op;
	uint8  Prior;
	uint8  ArgCount;
	uint8  Reserve; // @alignment
	const char * P_Sym;
};

static const Tddo2_OpInfo Tddo2_OpInfoList[] = {
	// Список отсортирован по длине текста символа чтобы не допустить перекрытия
	// распознавания более длинных команд более короткими (например < вместо <=)
	{ dlopAnd,  7, 2, 0, "and" },
	{ dlopLe,     5, 2, 0, "<=" },
	{ dlopGe,     5, 2, 0, ">=" },
	{ dlopEq,     6, 2, 0, "==" },
	{ dlopNeq,     6, 2, 0, "!=" },
	{ dlopNeq,     6, 2, 0, "<>" },
	{ dlopAnd,  7, 2, 0, "&&" },
	{ dlopOr,   8, 2, 0, "||" },
	{ dlopOr,   8, 2, 0, "or" },
	{ dlopDot,    1, 2, 0, "."  },
	{ dlopNot,  2, 1, 0, "!"  },
	{ dlopMul,   3, 2, 0, "*"  },
	{ dlopDiv, 3, 2, 0, "/"  },
	{ dlopMod, 3, 2, 0, "%"  },
	{ dlopAdd,   4, 2, 0, "+"  },
	{ dlopSub,  4, 2, 0, "-"  },
	{ dlopGt,     5, 2, 0, ">"  },
	{ dlopLt,     5, 2, 0, "<"  },
	{ dlopAssignment, 9, 2, 0, "="  }
};

class TddoContentGraph : public SStrGroup {
public:
	//
	// Descr: Виды блоков разобранного исходного текста
	//
	enum {
		kStart = 1, // Стартовый фиктивный блок
		kFinish,    // Финишный фиктивный блок
		kText,      // Просто текст
		kExpr,      // Выражение
		kForEach,   // branch
		kIf,        // branch
		kElse,      // branch
		kElseIf,    // branch
		kIter,      // branch #iter
		kMacro,     // branch Определение макроса #macro(macro_name, $arg1, $arg2)
		kMacroCall, // Вызов макроса #macro_name($arg1, $arg2)
		kSet,       // Установка значения переменной #set($a = 10)
		kStop,      // stop execution
		kBreak,     // break loop
		kInclude    //
	};
	static int FASTCALL IsFirstCharOfIdent(char c);
	static int FASTCALL IsCharOfIdent(char c);

	TddoContentGraph(/*Tddo & rT*/);
	~TddoContentGraph();
	int    SetSourceName(const char * pSrcName);
	int    Parse(const char * pSrc);
	int    Execute(const char * pDataName, DlRtm::ExportParam & rEp, const StringSet * pExtParamList, SBuffer & rOut);

	int    Helper_Parse(uint parentChunkP, int isBranch, int stopEvent);
	int    Helper_RecognizeExprToken(long flags, SString & rText);
	int    FASTCALL Helper_RecognizeMetaKeyword(SString & rAddendum);
	void   Skip();
	int    Output(SString & rBuf) const;
private:
	enum {
		untiltokEot    = 0x0001,
		untiltokRBrace = 0x0002,
		untiltokRPar   = 0x0004,
		untiltokComma  = 0x0008,
		untiltokSpace  = 0x0010  // space || tab || newline
	};
	enum {
		stopEot    = 0x0001,
		stopEnd    = 0x0002,
		stopEndIf  = 0x0004,
		stopElse   = 0x0008,
		stopElseIf = 0x0010
	};
	class ExprSet {
	public:
		enum {
			kOp = 1,
			kFunc,
			kString,
			kNumber,
			kVar,
			kFormalArg
		};
		struct Item {
			explicit Item(uint16 k);
			TYPEID GetNumberType() const;

			uint16 K;
			uint16 ArgCount; // Количество аргументов (для oneof(K, kOp, kFunc))
			union {
				uint   SymbP; // Позиция символа в R_Set.Pool (для oneof(K, kString, kVar, kFunc))
				uint32 Op;    // Ид операции (для K == kOp)
				double R;     // Значение числа (K == kNumber)
				uint64 Stub;
			};
		};
		class Stack : public TSStack <TddoContentGraph::ExprSet::Item> {
		public:
			Stack();
			int    IsSingleOp() const;
			int    FASTCALL Push(const Stack & rS);
			const  TddoContentGraph::ExprSet::Item & Get(uint p) const
			{
				return *static_cast<const TddoContentGraph::ExprSet::Item *>(at(p));
			}
		};
		class Expression {
		public:
			Expression();
			Expression(const Expression & rS) : ES(rS.ES), Next(rS.Next), Flags(rS.Flags)
			{
			}

			uint   Next;  // Позиция следующего выражения списка (например, для цепочки аргументов функции или макроса)
			long   Flags;
			TddoContentGraph::ExprSet::Stack ES; // Стэк выражения //
		};

		explicit ExprSet(TddoContentGraph & rG);
		~ExprSet();
		const  Expression * FASTCALL Get(uint p) const
		{
			return (p > 0 && p < EL.getCount()) ? EL.at(p) : 0;
		}
		int    CreateFormalArgExpr(const char * pArg, uint * pPos);
		int    SetNextPos(uint itemToUpdatePos, uint nextPos);
		// untilToken: } | ) | , | EOF
		int    Parse(uint untilToken, uint * pPos);
		void   DebugOutput(uint exprPos, SString & rBuf) const;
	private:
		static int FASTCALL CmpOpPrior(int op1, int op2);
		int    Helper_Parse(uint untilToken, TddoContentGraph::ExprSet::Stack & rStack);
		//
		// Descr: Ранжирует список выражений rExprList в порядке приоритета операций.
		//   Результирующий стек rStack содержит готовое выражение, где все операции исполняются в правильном порядке.
		//
		int    ArrangeLocalExprList(const TSCollection <Stack> & rExprList, Stack & rStack); // @recursion

		TddoContentGraph & R_G;
		TSCollection <Expression> EL;
	};
	struct Var {
		uint   IdentP;
		STypEx T;
		uint   ValueP; // Позиция значения переменной в буфере VvBuf
	};
	struct ChunkInner {
		int    Kind;    // Вид блока (TddoContentGraph::kXXX)
		uint   NextP;   // Следующий блок
		uint   BranchP; // Ответвление (if | else | iter)
		uint   HeadP;   // Головной блок (для ответвления - узел ветвления)
		uint   TextP;   // Текст
		uint   ExprP;   // Выражение
	};
	struct Current {
		Current() : LineNo(0), SourceNameP(0), ScanOffs(0), P_Src(0)
		{
		}
		uint   LineNo;
		uint   SourceNameP; // @*(SetSourceName) Позиция имени текущего источника данных
		uint   ScanOffs;
		const char * P_Src;
	};
	struct Error {
		Error()
		{
			THISZERO();
		}
		uint   SourceNameP;
		uint   LineNo;
		uint   ColNo;
		int    ErrCode;
		uint   AddedMsgP;
	};
	struct ExprResult : public SBaseBuffer {
		ExprResult()
		{
			SBaseBuffer::Init();
		}
		~ExprResult()
		{
			SBaseBuffer::Destroy();
		}
		STypEx T;
	};
	class LocalScope : public DlScope {
	public:
		LocalScope();
		~LocalScope();
		uint   SetVar(const char * pName, const STypEx & rT, const void * pData);
		int    GetVar(const char * pName, ExprResult & rResult) const;
	private:
		class DataPool : public SArray {
		public:
			DataPool();
		private:
			virtual void FASTCALL freeItem(void * p);
		};
		DataPool DP;
	};
	int    ChunkToStr(uint cp, uint tabLevel, SString & rBuf) const;
	uint   AddBlock(int kind, uint exprP, const SString & rText);
	void   CurrentPush()
	{
		ScStk.push(C);
	}
	int    CurrentPop()
	{
		int    ok = 1;
		Current t;
		if(ScStk.pop(t)) {
			C = t;
			Scan.Set(C.P_Src, C.ScanOffs);
		}
		else
			ok = 0;
		assert(ok);
		return ok;
	}

	int    ConvertExpression(TddoProcessBlock & rBlk, const ExprSet::Expression & rExpr, uint & rExprPointer, CtmExpr & rResult);
	int    ResolveExpression(const ExprSet::Expression & rExpr, uint & rExprPointer, TddoProcessBlock & rBlk, ExprResult & rResult);
	int    Helper_Execute(uint chunkP, TddoProcessBlock & rBlk, SString & rBuf);

	TSVector <ChunkInner> L;
	// Блок по индексу 0 всегда стартовый (фиктивный блок)
	uint   EndChunkP;   // Позиция завершающего (фиктивного) блока
	ExprSet ES; // Коллекция выражений. На них ссылаются ChunkInner::ExprP
	//Tddo & R_T;
	SStrScan Scan;
	Current C;
	TSStack <Current> ScStk;
	TSVector <Error>  ErrL;
	TSVector <Var>    VarL;
	const  SymbHashTable * P_ShT; // Таблица символов, полученная вызовом PPGetStringHash(int)
	DlContext * P_Ctx;
};

int TddoContentGraph::Helper_Execute(uint chunkP, TddoProcessBlock & rBlk, SString & rBuf)
{
	int    ok = 1;
	int    done = 0;
	uint   _cc = 0; // Счетчик блоков
	SString temp_buf;
	for(uint cp = chunkP; !done;) {
		THROW(cp < L.getCount());
		THROW(!_cc || cp > 0);
		const ChunkInner & r_chunk = L.at(cp);
		_cc++;
		switch(r_chunk.Kind) {
			case kStart:
				cp = r_chunk.NextP;
				break;
			case kFinish:
				done = 1;
				break;
			case kText:
				GetS(r_chunk.TextP, temp_buf);
				rBuf.Cat(temp_buf);
				cp = r_chunk.NextP;
				break;
			case kExpr:
				break;
			case kForEach:
				break;
			case kIf:
				break;
			case kElse:
				break;
			case kElseIf:
				break;
			case kIter:
				break;
			case kMacro:
				break;
			case kMacroCall:
				break;
			case kSet:
				{
					const ExprSet::Expression * p_expr = ES.Get(r_chunk.ExprP);
					if(p_expr) {
						ExprResult result;
						uint ep = 0;
						ResolveExpression(*p_expr, ep, rBlk, result);
					}
					else {
						; // @error
					}
					cp = r_chunk.NextP;
				}
				break;
			case kStop:
				break;
			case kBreak:
				break;
			case kInclude:
				break;
			default:
				CALLEXCEPT(); // @error
				break;
		}
	}
	CATCHZOK
	return ok;
}

TddoContentGraph::LocalScope::DataPool::DataPool() : SArray(sizeof(void *), aryPtrContainer|aryEachItem)
{
}

/*virtual*/void FASTCALL TddoContentGraph::LocalScope::DataPool::freeItem(void * p)
{
	SAlloc::F(p);
}

TddoContentGraph::LocalScope::LocalScope() : DlScope(0, DlScope::kLocal, "", 0)
{
}

TddoContentGraph::LocalScope::~LocalScope()
{
}

int TddoContentGraph::LocalScope::GetVar(const char * pName, ExprResult & rResult) const
{
	int    ok = 0;
	uint   vp = 0;
	if(SearchName(pName, &vp)) {
		SdbField fld;
		if(GetFieldByPos(vp, &fld)) {
			rResult.T.Init();
			rResult.T = fld.T;
			const size_t s = rResult.T.GetBinSize();
			const void * p_data = DP.at(vp);
			if(p_data) {
				rResult.Alloc(s);
				memcpy(rResult.P_Buf, p_data, s);
				ok = 1;
			}
			else {
				rResult.Alloc(s);
				memzero(rResult.P_Buf, s);
				ok = 1;
			}
		}
	}
	return ok;
}

uint TddoContentGraph::LocalScope::SetVar(const char * pName, const STypEx & rT, const void * pData)
{
	int    ok = 1;
	uint   vp = 0;
	uint   vid = 0;
	int    is_new = 0;
	THROW(!isempty(pName));
	if(!SearchName(pName, &vp)) {
		SdbField new_fld;
		new_fld.Name = pName;
		new_fld.T = rT;
		THROW_SL(AddField(&vid, &new_fld));
		THROW_SL(SearchName(pName, &vp));
		{
			assert(DP.getCount() == SdRecord::GetCount()-1);
			void * p_new_data = 0;
			THROW_SL(DP.insert(p_new_data));
		}
		is_new = 1;
	}
	{
		SdbField fld;
		THROW_SL(GetFieldByPos(vp, &fld));
		if(pData) {
			size_t fts = fld.T.GetBinSize();
			void * p_new_data = SAlloc::M(fts);
			THROW_SL(p_new_data);
			memcpy(p_new_data, pData, fts);
			SAlloc::F(DP.at(vp));
			DP.atPut(vp, p_new_data);
		}
	}
	CATCHZOK
	return ok;
}

int TddoContentGraph::Execute(const char * pDataName, DlRtm::ExportParam & rEp, const StringSet * pExtParamList, SBuffer & rOut)
{
	int    ok = 1;
	LocalScope local_scope;
	SString temp_buf;

	const char * p_data_name = NZOR(pDataName, "HttpPreprocessBase");
	TddoProcessBlock pblk;

	const DlScope * p_scope = 0;
	THROW_PP_S(p_scope = P_Ctx->GetScopeByName_Const(DlScope::kExpData, p_data_name), PPERR_TDDO_UNDEFDATANAME, p_data_name);
	pblk.P_Rtm = P_Ctx->GetRtm(p_scope->GetId());
	pblk.P_Rtm->P_Ep = &rEp;
	THROW(pblk.P_Rtm->InitData(pblk.F, BIN(pblk.Ep.Flags & DlRtm::ExportParam::fIsView)));
	pblk.SrcDataName = p_data_name;
	pblk.Ep.P_F = &pblk.F;
	//THROW(Helper_Process(pblk, rOut, meta, pblk.P_Rtm->GetData(), skip)); // @recursion

	THROW(Helper_Execute(0, pblk/*local_scope*/, temp_buf))
	CATCHZOK
	return ok;
}

int TddoContentGraph::ConvertExpression(TddoProcessBlock & rBlk, const ExprSet::Expression & rExpr, uint & rExprPointer, CtmExpr & rResult)
{
	int    ok = 1;
	SString temp_buf;
	const  ExprSet::Item & r_item = rExpr.ES.Get(rExprPointer++);
	switch(r_item.K) {
		case ExprSet::kOp:
			{
				rResult.Init(CtmExpr::kOp);
				rResult.U.Op = r_item.Op;
				for(uint argi = 0; argi < r_item.ArgCount; argi++) {
					CtmExpr arg_expr;
					THROW(ConvertExpression(rBlk, rExpr, rExprPointer, arg_expr));
					rResult.AddArg(arg_expr);
				}
			}
			break;
		case ExprSet::kFunc:
			{
				GetS(r_item.SymbP, temp_buf);
				rResult.Init(CtmExpr::kFuncName);
				rResult.U.S = newStr(temp_buf);
				for(uint argi = 0; argi < r_item.ArgCount; argi++) {
					CtmExpr arg_expr;
					THROW(ConvertExpression(rBlk, rExpr, rExprPointer, arg_expr));
					rResult.AddArg(arg_expr);
				}
			}
			break;
		case ExprSet::kVar:
			GetS(r_item.SymbP, temp_buf);
			assert(temp_buf.NotEmpty());
			rResult.InitVar(temp_buf);
			break;
		case ExprSet::kString:
			{
				DlContext * p_ctx = rBlk.P_Rtm->GetContext();
				DlScope * p_scope = p_ctx->GetCurScope();
				CtmExprConst c;
				GetS(r_item.SymbP, temp_buf);
				THROW(p_ctx->AddConst(temp_buf, &c));
				rResult.Init(c);
			}
			break;
		case ExprSet::kNumber:
			{
				const TYPEID nt = r_item.GetNumberType();
				const size_t s = GETSSIZE(nt);
				int   invt = 0;
				DlContext * p_ctx = rBlk.P_Rtm->GetContext();
				DlScope * p_scope = p_ctx->GetCurScope();
				CtmExprConst c;
				if(GETSTYPE(nt) == S_INT) {
					if(s == 2) {
						THROW(p_ctx->AddConst((int16)r_item.R, &c));
					}
					else if(s == 4) {
						THROW(p_ctx->AddConst((int32)r_item.R, &c));
					}
					else if(s == 8) {
						THROW(p_ctx->AddConst((int64)r_item.R, &c));
					}
					else
						invt = 1;
				}
				else if(GETSTYPE(nt) == S_FLOAT) {
					if(s == 4) {
						THROW(p_ctx->AddConst((float)r_item.R, &c));
					}
					else if(s == 8) {
						THROW(p_ctx->AddConst(r_item.R, &c));
					}
					else
						invt = 1;
				}
				else
					invt = 1;
				assert(!invt);
				THROW(!invt);
				rResult.Init(c);
			}
			break;
	}
	CATCHZOK
	return ok;
}

int TddoContentGraph::ResolveExpression(const ExprSet::Expression & rExpr, uint & rExprPointer, TddoProcessBlock & rBlk, ExprResult & rResult)
{
	int    ok = 1;
	SString temp_buf;

	// test {
	uint cvt_expr_ptr = rExprPointer;
	CtmExpr ctm_expr;
	ok = ConvertExpression(rBlk, rExpr, cvt_expr_ptr, ctm_expr);
	// } test

	const ExprSet::Item & r_item = rExpr.ES.Get(rExprPointer++);
	switch(r_item.K) {
		case ExprSet::kOp:
			if(r_item.Op == dlopAssignment) {
				assert(r_item.ArgCount == 2);
				const ExprSet::Item & r_arg1 = rExpr.ES.Get(rExprPointer++);
				if(r_arg1.K == ExprSet::kVar) {
					GetS(r_arg1.SymbP, temp_buf);
					if(temp_buf.NotEmpty()) {
						ExprResult local_result;
						THROW(ResolveExpression(rExpr, rExprPointer, rBlk, local_result)); // @recursion
						//THROW(rLs.SetVar(temp_buf, local_result.T, local_result.P_Buf));
					}
					else {
						; // @error
					}
				}
				else {
					; // @error
				}
			}
			else {
				if(r_item.ArgCount == 2) {
					ExprResult local_result1, local_result2;
					THROW(ResolveExpression(rExpr, rExprPointer, rBlk, local_result1)); // @recursion
					THROW(ResolveExpression(rExpr, rExprPointer, rBlk, local_result2)); // @recursion
					switch(r_item.Op) {
						case dlopAnd:
							break;
						case dlopLe:
							break;
						case dlopGe:
							break;
						case dlopEq:
							break;
						case dlopNeq:
							break;
						case dlopOr:
							break;
						case dlopDot:
							break;
						case dlopNot:
							break;
						case dlopMul:
							break;
						case dlopDiv:
							break;
						case dlopMod:
							break;
						case dlopAdd:
							break;
						case dlopSub:
							break;
						case dlopGt:
							break;
						case dlopLt:
							break;
						default:
							// @error
							break;
					}
				}
				else if(r_item.ArgCount == 1) {
					if(r_item.Op == dlopNot) {
					}
					else {
						; // @error
					}
				}
			}
			break;
		case ExprSet::kFunc:
			break;
		case ExprSet::kNumber:
			{
				TYPEID nt = r_item.GetNumberType();
				const size_t s = GETSSIZE(nt);
				rResult.T.Init();
				rResult.T.Typ = nt;
				int   invt = 0;
				if(GETSTYPE(s) == S_INT) {
					if(s == 2) {
						int16 v = (int16)r_item.R;
						assert(rResult.T.GetBinSize() == sizeof(v));
						rResult.Alloc(sizeof(v));
						memcpy(rResult.P_Buf, &v, sizeof(v));
					}
					else if(s == 4) {
						int32 v = (int32)r_item.R;
						assert(rResult.T.GetBinSize() == sizeof(v));
						rResult.Alloc(sizeof(v));
						memcpy(rResult.P_Buf, &v, sizeof(v));
					}
					else if(s == 8) {
						int64 v = (int64)r_item.R;
						assert(rResult.T.GetBinSize() == sizeof(v));
						rResult.Alloc(sizeof(v));
						memcpy(rResult.P_Buf, &v, sizeof(v));
					}
					else
						invt = 1;
				}
				else if(GETSTYPE(s) == S_FLOAT) {
					if(s == 4) {
						float v = (float)r_item.R;
						assert(rResult.T.GetBinSize() == sizeof(v));
						rResult.Alloc(sizeof(v));
						memcpy(rResult.P_Buf, &v, sizeof(v));
					}
					else if(s == 8) {
						double v = r_item.R;
						assert(rResult.T.GetBinSize() == sizeof(v));
						rResult.Alloc(sizeof(v));
						memcpy(rResult.P_Buf, &v, sizeof(v));
					}
					else
						invt = 1;
				}
				else
					invt = 1;
				if(invt) {
					rResult.T.Init();
					// @error
				}
			}
			break;
		case ExprSet::kString:
			{
				GetS(r_item.SymbP, temp_buf);
				const size_t s = temp_buf.Len() + 1;
				rResult.T.Init();
				rResult.T.Typ = MKSTYPE(S_ZSTRING, s);
				assert(rResult.T.GetBinSize() == s);
				rResult.Alloc(s);
				memcpy(rResult.P_Buf, temp_buf.cptr(), s);
			}
			break;
		case ExprSet::kVar:
			{
				GetS(r_item.SymbP, temp_buf);
				/*
				if(rLs.GetVar(temp_buf, rResult)) {
				}
				else {
					; // Здесь надо попытаться взять переменную из контекста DL600
				}
				*/
			}
			break;
		case ExprSet::kFormalArg:
			break;
	}
	CATCHZOK
	return ok;
}

TddoContentGraph::ExprSet::Item::Item(uint16 k) : K(k), ArgCount(0), Stub(0)
{
}

TYPEID TddoContentGraph::ExprSet::Item::GetNumberType() const
{
	TYPEID typ = 0;
	if(K == ExprSet::kNumber) {
		if(((double)(int16)R) == R)
			typ = MKSTYPE(S_INT, sizeof(int16));
		else if(((double)(int32)R) == R)
			typ = MKSTYPE(S_INT, sizeof(int32));
		else if(((double)(int64)R) == R)
			typ = MKSTYPE(S_INT, sizeof(int64));
		else if(((double)(float)R) == R)
			typ = MKSTYPE(S_FLOAT, sizeof(float));
		else
			typ = MKSTYPE(S_FLOAT, sizeof(double));
	}
	return typ;
}

TddoContentGraph::ExprSet::Stack::Stack()
{
}

int TddoContentGraph::ExprSet::Stack::IsSingleOp() const
{
	const uint pt = getPointer();
	if(pt == 1 && static_cast<const Item *>(at(pt-1))->K == kOp)
		return static_cast<const Item *>(at(pt-1))->Op;
	else
		return 0;
}

int FASTCALL TddoContentGraph::ExprSet::Stack::Push(const Stack & rS)
{
	int    ok = -1;
	for(uint i = 0; i < rS.getPointer(); i++) {
		push(*static_cast<TddoContentGraph::ExprSet::Item *>(rS.at(i)));
		ok = 1;
	}
	return ok;
}

TddoContentGraph::ExprSet::Expression::Expression() : Next(0), Flags(0)
{
}

TddoContentGraph::ExprSet::ExprSet(TddoContentGraph & rG) : R_G(rG)
{
	//
	// Для того, чтобы 0-позиция считалась инвалидной, необходимо
	// вставить фиктивный элемент по 0-й позиции.
	//
	uint   zerop = 0;
	assert(EL.CreateNewItem(&zerop));
	assert(zerop == 0);
}

TddoContentGraph::ExprSet::~ExprSet()
{
}

int TddoContentGraph::ExprSet::SetNextPos(uint itemToUpdatePos, uint nextPos)
{
	int    ok = 0;
	if(itemToUpdatePos && itemToUpdatePos < EL.getCount()) {
		Expression * p_item = EL.at(itemToUpdatePos);
		if(p_item) {
			p_item->Next = nextPos;
			ok = 1;
		}
	}
	return ok;
}

int TddoContentGraph::ExprSet::CreateFormalArgExpr(const char * pArg, uint * pPos)
{
	int    ok = 0;
	uint   ep = 0;
	SString temp_buf = pArg;
	if(temp_buf.NotEmptyS()) {
		Expression * p_new_expr = EL.CreateNewItem(&ep);
		if(p_new_expr) {
			Item ei(kFormalArg);
			R_G.AddS(temp_buf, &ei.SymbP);
			p_new_expr->ES.push(ei);
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, ep);
	return ok;
}

// untilToken: } | ) | , | EOF
int TddoContentGraph::ExprSet::Parse(uint untilToken, uint * pPos)
{
	int    ok = 1;
	uint   ep = 0;
	Expression * p_new_expr = EL.CreateNewItem(&ep);
	if(p_new_expr) {
		if(Helper_Parse(untilToken, p_new_expr->ES)) {
			ASSIGN_PTR(pPos, ep);
		}
		else {
			EL.atFree(ep);
			ok = 0;
		}
	}
	else
		ok = PPSetErrorSLib();
	return ok;
}

int TddoContentGraph::ExprSet::Helper_Parse(uint untilToken, TddoContentGraph::ExprSet::Stack & rStack)
{
	int    ok = 1;
	Tddo::Meta meta;
	SString temp_buf;
	SStrScan & r_scan = R_G.Scan;
	//
	// Локальный список выражений.
	// Для учета приоритета операторов все операнды, разделенные линейными операторами (+ - . etc)
	// вносятся в local_expr_list вместе с соответствующими операторами.
	// Операторы вносятся в список как Stack содержащий единственный элемент - собственно оператор
	//
	TSCollection <Stack> local_expr_list;
	for(int done = 0; !done;) {
		if(r_scan.IsEnd()) {
			if(!(untilToken & untiltokEot)) {
				ok = 0;
			}
			done = 1;
		}
		else if(untilToken & untiltokSpace && r_scan.IsSpace(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine)) {
			done = 1;
		}
		else {
			Stack * p_local_stk = 0; // temporary pointer in order to save code lines. Don't destroy it!
			R_G.Skip();
			if(untilToken & untiltokRBrace && r_scan[0] == '}')
				done = 1;
			else if(untilToken & untiltokRPar && r_scan[0] == ')')
				done = 1;
			else if(untilToken & untiltokComma && r_scan[0] == ',')
				done = 1;
			else if(r_scan[0] == '(') {
				r_scan.Incr();
				THROW(p_local_stk = local_expr_list.CreateNewItem());
				int local_ok = Helper_Parse(untiltokRPar, *p_local_stk); // @recursion
				if(local_ok) {
					assert(r_scan[0] == ')');
					r_scan.Incr();
				}
			}
			else {
				int r = R_G.Helper_RecognizeExprToken(0, temp_buf);
				switch(r) {
					case Tddo::tLiteral:
						{
							Item ei(kString);
							R_G.AddS(temp_buf, &ei.SymbP);
							THROW(p_local_stk = local_expr_list.CreateNewItem());
							p_local_stk->push(ei);
						}
						break;
					case Tddo::tNumber:
						{
							Item ei(kNumber);
							ei.R = temp_buf.ToReal();
							THROW(p_local_stk = local_expr_list.CreateNewItem());
							p_local_stk->push(ei);
						}
						break;
					case Tddo::tVar:
						{
							Item ei(kVar);
							R_G.AddS(temp_buf, &ei.SymbP);
							THROW(p_local_stk = local_expr_list.CreateNewItem());
							p_local_stk->push(ei);
						}
						break;
					case Tddo::tVarArgN:
						{
							Item ei(kVar);
							R_G.AddS(temp_buf, &ei.SymbP);
							THROW(p_local_stk = local_expr_list.CreateNewItem());
							p_local_stk->push(ei);
						}
						break;
					case Tddo::tDollarBrace:
						{
							THROW(p_local_stk = local_expr_list.CreateNewItem());
							Helper_Parse(untiltokRBrace, *p_local_stk); // @recursion
						}
						break;
					case Tddo::tFunc:
						{
							int    local_ok = 1;
							Item ei(kFunc);
							R_G.AddS(temp_buf, &ei.SymbP);
							TSCollection <Stack> local_arg_list;
							do {
								uint   local_stk_pos = 0;
								THROW_SL(p_local_stk = local_arg_list.CreateNewItem(&local_stk_pos));
								local_ok = Helper_Parse(untiltokComma|untiltokRPar, *p_local_stk); // @recursion
								if(p_local_stk->getPointer() == 0) {
									local_arg_list.atFree(local_stk_pos);
									if(r_scan[0] != ')' && local_ok)
										local_ok = 0; // @error Empty expression
								}
								else if(local_ok && r_scan[0] == ',') {
									r_scan.Incr();
								}
							} while(local_ok && r_scan[0] != ')');
							if(r_scan[0] == ')') {
								r_scan.Incr();
							}
							if(local_ok) {
								THROW(p_local_stk = local_expr_list.CreateNewItem());
								ei.ArgCount = local_arg_list.getCount();
								p_local_stk->push(ei);
								for(uint i = 0; i < local_arg_list.getCount(); i++) {
									p_local_stk->Push(*local_arg_list.at(i));
								}
							}
						}
						break;
					default:
						if(r > Tddo::tOperator) {
							Item ei(kOp);
							ei.Op = (r - Tddo::tOperator);
							if(ei.Op == dlopNot)
								ei.ArgCount = 1;
							else
								ei.ArgCount = 2;
							Stack * p_local_stk = local_expr_list.CreateNewItem();
							THROW(p_local_stk);
							p_local_stk->push(ei);
						}
						else {
							assert(0); // @error
						}
						break;
				}
			}
		}
	}
	THROW(ArrangeLocalExprList(local_expr_list, rStack));
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL TddoContentGraph::ExprSet::CmpOpPrior(int op1, int op2)
{
	if(op1 == op2)
		return 0;
	else {
		int prior1 = 0;
		int prior2 = 0;
		for(uint i = 0; (!prior1 || !prior2) && i < SIZEOFARRAY(Tddo2_OpInfoList); i++) {
			const int _o = (int)Tddo2_OpInfoList[i].Op;
			if(_o == op1)
				prior1 = (int)Tddo2_OpInfoList[i].Prior;
			if(_o == op2)
				prior2 = (int)Tddo2_OpInfoList[i].Prior;
		}
		return CMPSIGN(prior1, prior2);
	}
}

int TddoContentGraph::ExprSet::ArrangeLocalExprList(const TSCollection <Stack> & rExprList, Stack & rStack)
{
	int    ok = 1;
	const  uint _c = rExprList.getCount();
	if(_c) {
		if(_c == 1) {
			rStack.Push(*rExprList.at(0));
		}
		else {
			uint   i;
			LAssocArray op_list; // key - idx, val - op
			for(i = 0; i < _c; i++) {
				int op = rExprList.at(i)->IsSingleOp();
				if(op)
					op_list.Add(i, op, 0);
			}
			const uint _oc = op_list.getCount();
			if(_oc == 1) {
				const int  op = op_list.at(0).Val;
				const uint op_pos = op_list.at(0).Key;
				if(op == dlopNot) {
					if(op_pos == 0 && _c == 2) {
						rStack.Push(*rExprList.at(op_pos));
						rStack.Push(*rExprList.at(op_pos+1));
					}
					else {
						ok = 0; // @error
					}
				}
				else {
					if(op_pos == 1 && _c == 3) {
						rStack.Push(*rExprList.at(op_pos));
						rStack.Push(*rExprList.at(op_pos-1));
						rStack.Push(*rExprList.at(op_pos+1));
					}
					else {
						ok = 0; // @error
					}
				}
			}
			else {
				for(i = 0; i < _oc; i++) {
					int local_ok = 1;
					const int  op = op_list.at(i).Val;
					const uint op_pos = op_list.at(i).Key;
					const int  cr = (i < (_oc-1)) ? CmpOpPrior(op, op_list.at(i+1).Val) : -1;
					if(cr <= 0) {
						TSCollection <Stack> inner_expr_list;
						uint   ii;
						if(op == dlopNot) {
							if(op_pos < (_c-1)) {
								for(ii = 0; ii < op_pos; ii++) {
									Stack * p_new_stk = inner_expr_list.CreateNewItem();
									*p_new_stk = *rExprList.at(ii);
								}
								{
									Stack * p_merge_stk = inner_expr_list.CreateNewItem();
									p_merge_stk->Push(*rExprList.at(op_pos));
									p_merge_stk->Push(*rExprList.at(op_pos+1));
								}
								for(ii = op_pos+2; ii < _c; ii++) {
									Stack * p_new_stk = inner_expr_list.CreateNewItem();
									*p_new_stk = *rExprList.at(ii);
								}
							}
							else {
								local_ok = 0; // @error
							}
						}
						else {
							if(op_pos > 0 && op_pos < (_c-1)) {
								for(ii = 0; ii < op_pos-1; ii++) {
									Stack * p_new_stk = inner_expr_list.CreateNewItem();
									*p_new_stk = *rExprList.at(ii);
								}
								{
									Stack * p_merge_stk = inner_expr_list.CreateNewItem();
									p_merge_stk->Push(*rExprList.at(op_pos));
									p_merge_stk->Push(*rExprList.at(op_pos-1));
									p_merge_stk->Push(*rExprList.at(op_pos+1));
								}
								for(ii = op_pos+2; ii < _c; ii++) {
									Stack * p_new_stk = inner_expr_list.CreateNewItem();
									*p_new_stk = *rExprList.at(ii);
								}
							}
							else {
								local_ok = 0; // @error
							}
						}
						ok = local_ok ? ArrangeLocalExprList(inner_expr_list, rStack) : 0; // @recursion
						break;
					}
				}
			}
		}
	}
	return ok;
}

void TddoContentGraph::ExprSet::DebugOutput(uint exprPos, SString & rBuf) const
{
	if(exprPos < EL.getCount()) {
		const Expression * p_expr = EL.at(exprPos);
		if(p_expr) {
			SString temp_buf;
			rBuf.Cat("expression").CatChar('[').Cat(exprPos).CatChar(']');
			if(p_expr->Next) {
				rBuf.Space().CatEq("Next", p_expr->Next);
			}
			if(p_expr->Flags) {
				rBuf.Space().CatEq("Flags", p_expr->Flags);
			}
			rBuf.CatChar(':');
			for(uint i = 0; i < p_expr->ES.getPointer(); i++) {
				const Item & r_item = p_expr->ES.Get(i);
				rBuf.Space();
				switch(r_item.K) {
					case kOp:
						rBuf.Cat("op");
						{
							int    _f = 0;
							for(uint j = 0; !_f && j < SIZEOFARRAY(Tddo2_OpInfoList); j++) {
								if(Tddo2_OpInfoList[j].Op == r_item.Op) {
									rBuf.CatChar('[').Cat(Tddo2_OpInfoList[j].P_Sym).CatChar(']');
									_f = 1;
								}
							}
							if(!_f)
								rBuf.CatChar('[').Cat("UNKN").CatChar(']');
						}
						rBuf.CatParStr(r_item.ArgCount);
						break;
					case kFunc:
						rBuf.Cat("func");
						R_G.GetS(r_item.SymbP, temp_buf);
						rBuf.CatChar('[').Cat(temp_buf).CatChar(']');
						rBuf.CatParStr(r_item.ArgCount);
						break;
					case kString:
						rBuf.Cat("string");
						R_G.GetS(r_item.SymbP, temp_buf);
						rBuf.CatChar('[').Cat(temp_buf).CatChar(']');
						break;
					case kNumber:
						rBuf.Cat("number");
						rBuf.CatChar('[').Cat(r_item.R, MKSFMTD(0, 9, NMBF_NOTRAILZ)).CatChar(']');
						break;
					case kVar:
						rBuf.Cat("var");
						R_G.GetS(r_item.SymbP, temp_buf);
						rBuf.CatChar('[').Cat(temp_buf).CatChar(']');
						break;
					case kFormalArg:
						rBuf.Cat("formalarg");
						R_G.GetS(r_item.SymbP, temp_buf);
						rBuf.CatChar('[').Cat(temp_buf).CatChar(']');
						break;
					default:
						rBuf.Cat("UNKN");
						break;
				}
			}
			if(p_expr->Next) {
				rBuf.CatDiv(',', 2);
				DebugOutput(p_expr->Next, rBuf); // @recursion
			}
		}
		else
			rBuf.Cat("zero-expression").CatChar('[').Cat(exprPos).CatChar(']');
	}
	else
		rBuf.Cat("invalid-expression-pos").CatChar('[').Cat(exprPos).CatChar(']');
}

TddoContentGraph::TddoContentGraph(/*Tddo & rT*/) : /*R_T(rT),*/ ES(*this),
	P_ShT(PPGetStringHash(PPSTR_HASHTOKEN)), P_Ctx(DS.GetInterfaceContext(PPSession::ctxtExportData))
{
}

TddoContentGraph::~TddoContentGraph()
{
}

int TddoContentGraph::SetSourceName(const char * pSrcName)
{
	if(isempty(pSrcName)) {
		C.SourceNameP = 0;
	}
	else {
		SString temp_buf = pSrcName;
		temp_buf.Strip();
		AddS(temp_buf, &C.SourceNameP);
	}
	return 1;
}

int TddoContentGraph::Parse(const char * pSrc)
{
	int    ok = 1;
	SString temp_buf;
	L.clear();
	EndChunkP = 0;
	uint start_p = AddBlock(kStart, 0, temp_buf.Z());
	assert(start_p == 0);
	EndChunkP = AddBlock(kFinish, 0, temp_buf.Z());
	Scan.Set(pSrc, 0);
	C.LineNo = 0;
	C.P_Src = pSrc;
	C.SourceNameP = 0;
	THROW(Helper_Parse(0, 0, stopEot));
	CATCHZOK
	return ok;
}

int TddoContentGraph::Helper_Parse(uint parentChunkP, int isBranch, int stopEvent)
{
	int    ok = 1;
	uint   prev_scan_stack_pos = 0;
	SString temp_buf, msg_buf;
	Tddo::Meta   meta;

	struct CurrentBlock {
		CurrentBlock(uint parentChunkP, int isBrach) : Kind(0), P(parentChunkP), ExprP(0), IsBranch(isBrach)
		{
		}
		void   FASTCALL AppendToList(TddoContentGraph * pG)
		{
			if(Kind) {
				uint   _p = pG->AddBlock(Kind, ExprP, Text);
				if(IsBranch)
					pG->L.at(P).BranchP = _p;
				else
					pG->L.at(P).NextP = _p;
				P = _p;
				IsBranch = 0;
				Kind = 0;
				ExprP = 0;
				Text.Z();
			}
		}
		void   Set(int kind, uint exprP, int isBranch)
		{
			Kind = kind;
			ExprP = exprP;
			IsBranch = isBranch;
			Text.Z();
		}
		int    Kind;
		int    IsBranch;
		uint   P;
		uint   ExprP;
		SString Text;
	};
	CurrentBlock _cb(parentChunkP, isBranch);

	for(int exit_loop = 0; !exit_loop && !Scan.IsEnd();) {
		int    regular_text = 1;
		int    m = Helper_RecognizeMetaKeyword(temp_buf);
		if(m > 0) {
			_cb.AppendToList(this); // Завершаем текущий блок
			regular_text = 0;
			switch(m) {
				case Tddo::tMacro:
					{
						// parse macro
						// macro_name ( ident [$ident]* )
						int    local_ok = 1;
						uint   macro_name_p = 0;
						LongArray macro_arg_list;
						Skip();
						if(Scan[0] == '(') {
							Scan.Incr();
							if(Scan[0] == ')') {
								; // end of declaration
							}
							else if(Scan.GetIdent(temp_buf)) {
								AddS(temp_buf, &macro_name_p);
								Scan.Skip();
								while(local_ok && Scan[0] != ')') {
									if(Scan[0] == '$') {
										Scan.Incr();
										if(Scan.GetIdent(temp_buf)) {
											uint   arg_expr_p = 0;
											THROW(ES.CreateFormalArgExpr(temp_buf, &arg_expr_p));
											macro_arg_list.add((long)arg_expr_p);
											Scan.Skip();
											if(Scan[0] == ',') { // optional ','
												Scan.Incr();
												Scan.Skip();
											}
										}
										else {
											local_ok = 0; // @error
										}
									}
									else {
										local_ok = 0; // @error
									}
								}
							}
							else {
								local_ok = 0; // @error
							}
							if(local_ok) {
								if(macro_arg_list.getCount()) {
									_cb.ExprP = macro_arg_list.at(0);
									assert(_cb.ExprP);
									// Устанавливаем связь в цепи выражений-аргументов
									for(uint i = 1; i < macro_arg_list.getCount(); i++) {
										uint   expr_p = (uint)macro_arg_list.get(i);
										uint   prev_expr_p = (uint)macro_arg_list.get(i-1);
										assert(expr_p);
										assert(i > 1 || prev_expr_p == _cb.ExprP);
										assert(prev_expr_p);
										ES.SetNextPos(prev_expr_p, expr_p);
									}
								}
							}
							GetS(macro_name_p, temp_buf);
							_cb.Text = temp_buf;
							_cb.Kind = kMacro;
							_cb.AppendToList(this);
							if(!Helper_Parse(_cb.P, 1, stopEot|stopEnd)) // @recursion
								local_ok = 0; // @error
						}
						else {
							local_ok = 0; // @error
						}
					}
					break;
				case Tddo::tMacroCall:
					{
						int    local_ok = 1;
						// temp_buf - symbol of macro
						LongArray macro_arg_list;
						_cb.Text = temp_buf;
						_cb.Kind = kMacroCall;
						Skip();
						if(Scan[0] == '(') {
							Scan.Incr();
							do {
								uint   expr_p = 0;
								if(ES.Parse(untiltokRPar|untiltokSpace|untiltokComma, &expr_p)) {
									assert(expr_p);
									macro_arg_list.add((long)expr_p);
								}
								else
									local_ok = 0;
								Skip();
								if(Scan[0] == ',') { // optional ','
									Scan.Incr();
									Scan.Skip();
								}
							} while(!Scan.IsEnd() && Scan[0] != ')');
							if(Scan[0] == ')') {
								Scan.Incr();
								if(local_ok) {
									if(macro_arg_list.getCount()) {
										_cb.ExprP = macro_arg_list.at(0);
										assert(_cb.ExprP);
										// Устанавливаем связь в цепи выражений-аргументов
										for(uint i = 1; i < macro_arg_list.getCount(); i++) {
											uint   expr_p = (uint)macro_arg_list.get(i);
											uint   prev_expr_p = (uint)macro_arg_list.get(i-1);
											assert(expr_p);
											assert(i > 1 || prev_expr_p == _cb.ExprP);
											assert(prev_expr_p);
											ES.SetNextPos(prev_expr_p, expr_p);
										}
									}
								}
							}
							else {
								local_ok = 0; // @error
							}
						}
						else {
							local_ok = 0; // @error
						}
						_cb.AppendToList(this);
					}
					break;
				case Tddo::tSet:
					Scan.Skip();
					if(Scan[0] == '(') {
						Scan.Incr();
						uint   expr_p = 0;
						if(ES.Parse(untiltokRPar, &expr_p)) {
							assert(Scan[0] == ')');
							Scan.Incr();
							_cb.Set(kSet, expr_p, 0);
							_cb.AppendToList(this);
						}
						else {
							; // @error
						}
					}
					else {
						; // @error
					}
					break;
				case Tddo::tBreak:
					_cb.Set(kBreak, 0, 0);
					_cb.AppendToList(this);
					break;
				case Tddo::tStop:
					_cb.Set(kStop, 0, 0);
					_cb.AppendToList(this);
					break;
				case Tddo::tIf:
					Scan.Skip();
					if(Scan[0] == '(') {
						Scan.Incr();
						uint   expr_p = 0;
						if(ES.Parse(untiltokRPar, &expr_p)) {
							assert(Scan[0] == ')');
							Scan.Incr();
							_cb.Set(kIf, expr_p, 0);
							_cb.AppendToList(this);
							if(!Helper_Parse(_cb.P, 1, stopEot|stopEnd|stopEndIf|stopElse|stopElseIf)) { // @recursion
								; // @error
							}
						}
						else {
							; // @error
						}
					}
					else {
						; // @error
					}
					break;
				case Tddo::tElse:
					{
						const int parent_kind = parentChunkP ? L.at(parentChunkP).Kind : 0;
						if(oneof2(parent_kind, kIf, kElseIf)) {
							; // @error
						}
						_cb.Set(kElse, 0, 1);
						_cb.AppendToList(this);
						if(!Helper_Parse(_cb.P, 1, stopEot|stopEnd|stopEndIf)) { // @recursion
							; // @error
						}
						if(stopEvent & stopElse) {
							exit_loop = 1;
						}
					}
					break;
				case Tddo::tElif:
					{
						const int parent_kind = parentChunkP ? L.at(parentChunkP).Kind : 0;
						if(oneof2(parent_kind, kIf, kElseIf)) {
							; // @error
						}
						Scan.Skip();
						if(Scan[0] == '(') {
							Scan.Incr();
							uint   expr_p = 0;
							if(ES.Parse(untiltokRPar, &expr_p)) {
								assert(Scan[0] == ')');
								Scan.Incr();
								_cb.Set(kElseIf, expr_p, 1);
								_cb.AppendToList(this);
								if(!Helper_Parse(_cb.P, 1, stopEot|stopEnd|stopEndIf|stopElse|stopElseIf)) { // @recursion
									; // @error
								}
							}
							else {
								; // @error
							}
						}
						else {
							; // @error
						}
						if(stopEvent & stopElseIf) {
							exit_loop = 1;
						}
					}
					break;
				case Tddo::tForEach:
					Scan.Skip();
					if(Scan[0] == '(') {
						Scan.Incr();
						uint   expr_p = 0;
						if(ES.Parse(untiltokRPar, &expr_p)) {
							assert(Scan[0] == ')');
							Scan.Incr();
							_cb.Set(kForEach, expr_p, 0);
							_cb.AppendToList(this);
							if(!Helper_Parse(_cb.P, 1, stopEot|stopEnd)) { // @recursion
								; // @error
							}
						}
						else {
							; // @error
						}
					}
					else {
						; // @error
					}
					break;
				case Tddo::tEndif:
					if(stopEvent & stopEndIf) {
						exit_loop = 1;
					}
					break;
				case Tddo::tEnd:
					if(stopEvent & stopEnd) {
						exit_loop = 1;
					}
					break;
				case Tddo::tCodepage:
					break;
				case Tddo::tPragma:
					break;
				case Tddo::tStart:
					break;
				case Tddo::tRem:
					break;
				case Tddo::tIter:
					break;
				case Tddo::tIterCount:
					break;
				case Tddo::tText:
					break;
				case Tddo::tInclude:
					break;
				default:
					regular_text = 1;
					break;
			}
		}
		else {
			int t = Helper_RecognizeExprToken(Tddo::rexptfMetaOnly, temp_buf);
			if(t > 0) {
				regular_text = 0;
			}
		}
		if(regular_text) {
			_cb.Kind = kText;
			if(Scan.GetEol(eolUndef)) {
				_cb.Text.CR();
				C.LineNo++;
			}
			else {
				_cb.Text.CatChar(Scan[0]);
				Scan.Incr();
			}
		}
	}
	_cb.AppendToList(this); // Завершаем последний блок
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL TddoContentGraph::IsFirstCharOfIdent(char c)
{
	return (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

/*static*/int FASTCALL TddoContentGraph::IsCharOfIdent(char c)
{
	return (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isdec(c));
}

int TddoContentGraph::Helper_RecognizeExprToken(long flags, SString & rText)
{
	int    t = 0;
	char   text[512];
	size_t tp = 0;
	size_t sp = 0;
	char   c = Scan[sp];
	if(c == '$') {
		c = Scan[++sp];
		if(c == '{') {
			c = Scan[++sp];
			if(isdec(c)) {
				//
				// ? Номер внешнего аргумента вида ${1}
				//
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(isdec(c));
				text[tp] = 0;
				if(c == '}') {
					Scan.Incr(tp+3); // ${}
					rText = text;
					t = Tddo::tVarArgN;
				}
			}
			else if(IsFirstCharOfIdent(c)) {
				//
				// ? Простая переменная вида ${var}
				//
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(IsCharOfIdent(c));
				text[tp] = 0;
				if(c == '}') {
					Scan.Incr(tp+3); // ${}
					rText = text;
					t = Tddo::tVar;
				}
			}
			if(!t) {
				//
				// Скорее всего (но не обязательно) начало сложного выражения.
				// Например: ${util.Goods.getSingleBarcode()}
				// Функция разбора выражения должны быть вызвана рекурсивно для разбора содержимого
				// с сигналом финиша '}'
				//
				t = Tddo::tDollarBrace;
			}
		}
		else if(IsFirstCharOfIdent(c)) {
			do {
				text[tp++] = c;
				c = Scan[++sp];
			} while(IsCharOfIdent(c));
			text[tp] = 0;
			Scan.Incr(tp+1); // $
			if(Scan[0] == '(') {
				Scan.Incr();
				t = Tddo::tFunc;
			}
			else
				t = Tddo::tVar;
			rText = text;
		}
	}
	else if(c == '@' && Scan[1] == '{') {
		c = Scan[++sp];
		if(IsFirstCharOfIdent(c)) {
			do {
				text[tp++] = c;
				c = Scan[++sp];
			} while(IsCharOfIdent(c));
			text[tp] = 0;
			if(c == '}') {
				Scan.Incr(tp+3); // # + { + }
				rText = text;
				t = Tddo::tString;
			}
			else {
				; // @suspicious
			}
		}
	}
	else if(!(flags & Tddo::rexptfMetaOnly)) {
		for(uint opi = 0; !t && opi < SIZEOFARRAY(Tddo2_OpInfoList); opi++) {
			const char * p_s = Tddo2_OpInfoList[opi].P_Sym;
			if(Scan.Is(p_s)) {
				Scan.Incr(sstrlen(p_s));
				rText = p_s;
				t = Tddo::tOperator+Tddo2_OpInfoList[opi].Op;
				assert(t > Tddo::tOperator);
				break;
			}
		}
		if(!t) {
			if(c == '\"') {
				c = Scan[++sp];
				while(c && c != '\"') {
					if(c == '\\' && Scan[sp] == '\"') {
						text[tp++] = '\"';
						sp++;
					}
					else
						text[tp++] = c;
					c = Scan[++sp];
				}
				text[tp] = 0;
				if(c == '\"') {
					Scan.Incr(sp+1); // + '\"'
					rText = text;
					t = Tddo::tLiteral;
				}
			}
			else if(Scan.IsNumber()) {
				Scan.GetNumber(rText);
				t = Tddo::tNumber;
			}
			else if(IsFirstCharOfIdent(c)) {
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(IsCharOfIdent(c));
				text[tp] = 0;
				Scan.Incr(tp);
				if(Scan[0] == '(') {
					Scan.Incr();
					t = Tddo::tFunc;
				}
				else
					t = Tddo::tVar;
				rText = text;
			}
		}
	}
	return t;
}

int FASTCALL TddoContentGraph::Helper_RecognizeMetaKeyword(SString & rAddendum)
{
	rAddendum.Z();
	int    m = 0;
	char   text[512];
	size_t tp = 0;
	size_t sp = 0;
	char   c;
	if(Scan[0] == '#') {
		c = Scan[++sp];
		if(c == '#') { // Пропускаем строчные комментарии
			Scan.Incr(2);
			while(Scan[0] && !Scan.IsEol(eolUndef))
				Scan.Incr();
			if(Scan.GetEol(eolUndef))
				C.LineNo++;
			m = Helper_RecognizeMetaKeyword(rAddendum); // @recusion
		}
		else if(c == '*') { // Пропускаем многострочные комментарии
			Scan.Incr(2);
			while(Scan[0] && !Scan.Is("*#")) {
				if(Scan.GetEol(eolUndef))
					C.LineNo++;
				else
					Scan.Incr();
			}
			if(Scan.Is("*#"))
				Scan.Incr(2);
			m = Helper_RecognizeMetaKeyword(rAddendum); // @recusion
		}
		else {
			int    recogn_result = 0;
			tp = 0;
			if(c == '{') {
				c = Scan[++sp];
				if(IsFirstCharOfIdent(c)) {
					do {
						text[tp++] = c;
						c = Scan[++sp];
					} while(IsCharOfIdent(c));
					text[tp] = 0;
					if(c == '}')
						recogn_result = 200;
					else
						recogn_result = -1; // suspicious
				}
			}
			else if(IsFirstCharOfIdent(c)) {
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(IsCharOfIdent(c));
				text[tp] = 0;
				recogn_result = 100;
			}
			if(recogn_result > 0) {
				uint _ut = 0;
				P_ShT->Search(text, &_ut, 0);
				switch(_ut) {
					case PPHS_MACRO:     m = Tddo::tMacro; break;
					case PPHS_SET:       m = Tddo::tSet; break;
					case PPHS_BREAK:     m = Tddo::tBreak; break;
					case PPHS_STOP:      m = Tddo::tStop; break;
					case PPHS_IF:        m = Tddo::tIf; break;
					case PPHS_ELSE:      m = Tddo::tElse; break;
					case PPHS_ELSEIF:
					case PPHS_ELIF:      m = Tddo::tElif; break;
					case PPHS_FOREACH:   m = Tddo::tForEach; break;
					case PPHS_ENDIF:     m = Tddo::tEndif; break;
					case PPHS_END:       m = Tddo::tEnd; break;
					case PPHS_CODEPAGE:  m = Tddo::tCodepage; break;
					case PPHS_PRAGMA:    m = Tddo::tPragma; break;
					case PPHS_START:     m = Tddo::tStart; break;
					case PPHS_REM:       m = Tddo::tRem; break;
					case PPHS_ITER:      m = Tddo::tIter; break;
					case PPHS_ITERCOUNT: m = Tddo::tIterCount; break;
					case PPHS_TEXT:      m = Tddo::tText; break;
					case PPHS_INCLUDE:   m = Tddo::tInclude; break;
					default:
						if(text[0]) {
							m = Tddo::tMacroCall;
							rAddendum = text;
						}
						break;
				}
				if(m) {
					if(recogn_result == 200)
						Scan.Incr(tp+3); // # + { + }
					else
						Scan.Incr(tp+1); // #
				}
			}
		}
	}
	return m;
}

void TddoContentGraph::Skip()
{
	Scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine, &C.LineNo);
}

int TddoContentGraph::Output(SString & rBuf) const
{
	rBuf.Z();
	return ChunkToStr(0, 0, rBuf);
}

int TddoContentGraph::ChunkToStr(uint cp, uint tabLevel, SString & rBuf) const
{
	int    ok = 1;
	SString temp_buf;
	const ChunkInner & r_chunk = L.at(cp);
	if(tabLevel)
		rBuf.Tab(tabLevel);
	rBuf.CatEq("Chunk", cp);
	switch(r_chunk.Kind) {
		case kStart: temp_buf = "kStart"; break;
		case kFinish: temp_buf = "kFinish"; break;
		case kText: temp_buf = "kText"; break;
		case kExpr: temp_buf = "kExpr"; break;
		case kIf: temp_buf = "kIf"; break;
		case kElse: temp_buf = "kElse"; break;
		case kElseIf: temp_buf = "kElseIf"; break;
		case kIter:temp_buf = "kIter"; break;
		case kMacro: temp_buf = "kMacro"; break;
		case kSet: temp_buf = "kSet"; break;
		default: temp_buf = "kUNKNOWN"; break;
	}
	rBuf.Space().CatEq("Kind", temp_buf);
	if(r_chunk.TextP) {
		GetS(r_chunk.TextP, temp_buf);
		rBuf.Space().CatEq("Text", temp_buf);
	}
	if(r_chunk.ExprP) {
		rBuf.Space().CatEq("ExprP", r_chunk.ExprP);
		ES.DebugOutput(r_chunk.ExprP, temp_buf.Z());
		rBuf.CR().Tab(tabLevel+1).Cat(temp_buf);
	}
	if(r_chunk.BranchP) {
		rBuf.CR();
		ChunkToStr(r_chunk.BranchP, tabLevel+1, rBuf); // @recursion
	}
	if(r_chunk.NextP) {
		rBuf.CR();
		ChunkToStr(r_chunk.NextP, tabLevel, rBuf); // @recursion
	}
	return ok;
}

uint TddoContentGraph::AddBlock(int kind, uint exprP, const SString & rText)
{
	uint   result_pos = 0;
	ChunkInner chunk;
	MEMSZERO(chunk);
	chunk.Kind = kind;
	chunk.ExprP = exprP;
	AddS(rText, &chunk.TextP);
	result_pos = L.getCount();
	if(EndChunkP)
		chunk.NextP = EndChunkP;
	L.insert(&chunk);
	return result_pos;
}
//
// Implementation of PPALDD_HttpPreprocessBase
//
PPALDD_CONSTRUCTOR(HttpPreprocessBase) { InitFixData(rscDefHdr, &H, sizeof(H)); }
PPALDD_DESTRUCTOR(HttpPreprocessBase) { Destroy(); }

int PPALDD_HttpPreprocessBase::InitData(PPFilt & rFilt, long rsrv)
{
	return DlRtm::InitData(rFilt, rsrv);
}

int TestTddo2()
{
	int    ok = 1;
	SString inp_file_name;
	SString out_file_name;
	SString debug_file_name;
	SString temp_buf, in_buf;
	SBuffer out_buf;
	LongArray id_list;
	StringSet ext_param_list;
	id_list.addUnique(55);
	id_list.addUnique(3);
	PPGetPath(PPPATH_TESTROOT, inp_file_name);
	const char * p_file_name = "tddo2-test-02";
	inp_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat((temp_buf = p_file_name).Dot().Cat("tddo"));
	SFile in_file(inp_file_name, SFile::mRead);
	//
	PPGetPath(PPPATH_TESTROOT, debug_file_name);
	debug_file_name.SetLastSlash().Cat("out").SetLastSlash().Cat((temp_buf = p_file_name).CatChar('-').Cat("debug").Dot().Cat("tddo"));
	SFile debug_file(debug_file_name, SFile::mWrite);
	//
	PPGetPath(PPPATH_TESTROOT, out_file_name);
	out_file_name.SetLastSlash().Cat("out").SetLastSlash().Cat((temp_buf = p_file_name).CatChar('-').Cat("out").Dot().Cat("tddo"));
	SFile out_file(out_file_name, SFile::mWrite);

	THROW_SL(in_file.IsValid());
	THROW_SL(debug_file.IsValid());
	THROW_SL(out_file.IsValid());
	{
		//Tddo tddo;
		//TddoContentGraph tcg(/*tddo*/);
		//tddo.SetInputFileName(inp_file_name);
		TddoContentGraph tcg;
		while(in_file.ReadLine(temp_buf))
			in_buf.Cat(temp_buf);
		tcg.Parse(in_buf);
		tcg.Output(temp_buf);
		debug_file.WriteLine(temp_buf);
		//
		{
			SBuffer result_buf;
			{
				DlRtm::ExportParam ep;
				tcg.Execute(0, ep, 0, result_buf);
			}
			out_file.WriteLine((const char *)result_buf.GetBuf(result_buf.GetRdOffs()));
		}
	}
#if 0 // {
	for(uint i = 0; i < id_list.getCount(); i++) {
		DlRtm::ExportParam ep;
		PPFilt _pf(id_list.get(i));
		ep.P_F = &_pf;
		tddo.Process(0, in_buf, /*id_list.get(i), 0*/ep, &ext_param_list, out_buf);
		out_buf.WriteByte('\n');
	}
	out_file.Write(out_buf, out_buf.GetAvailableSize());
#endif // } 0
	CATCHZOK
	return ok;
}
//
//
//
int TestTddo()
{
	int    ok = 1;
	SString temp_buf, in_buf;
	SBuffer out_buf;
	LongArray id_list;
	StringSet ext_param_list;
	Tddo tddo;
	id_list.addUnique(55);
	id_list.addUnique(3);
	PPGetPath(PPPATH_TESTROOT, temp_buf);
	temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("test.tddo");
	SFile in_file(temp_buf, SFile::mRead);
	tddo.SetInputFileName(temp_buf);
	PPGetPath(PPPATH_TESTROOT, temp_buf);
	temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("test-out.tddo");
	SFile out_file(temp_buf, SFile::mWrite);
	THROW_SL(in_file.IsValid());
	THROW_SL(out_file.IsValid());
	while(in_file.ReadLine(temp_buf))
		in_buf.Cat(temp_buf);
	for(uint i = 0; i < id_list.getCount(); i++) {
		DlRtm::ExportParam ep;
		PPFilt _pf(id_list.get(i));
		ep.P_F = &_pf;
		tddo.Process(0, in_buf, /*id_list.get(i), 0*/ep, &ext_param_list, out_buf);
		out_buf.WriteByte('\n');
	}
	out_file.Write(out_buf, out_buf.GetAvailableSize());
	CATCHZOK
	return ok;
}

#if SLTEST_RUNNING // {

SLTEST_R(Tddo)
{
	int    ok = 1;
	SString temp_buf, in_buf;
	SBuffer out_buf;
	LongArray id_list;
	Tddo tddo;
	StringSet ext_param_list;
	for(uint arg_no = 0; EnumArg(&arg_no, temp_buf);) {
		id_list.addUnique(temp_buf.ToLong());
	}
	(temp_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("test.tddo");
	SFile in_file(temp_buf, SFile::mRead);
	tddo.SetInputFileName(temp_buf); // @v7.5.10
	(temp_buf = GetSuiteEntry()->OutPath).SetLastSlash().Cat("test-out.tddo");
	SFile out_file(temp_buf, SFile::mWrite);
	THROW_SL(in_file.IsValid());
	THROW_SL(out_file.IsValid());
	while(in_file.ReadLine(temp_buf))
		in_buf.Cat(temp_buf);
	for(uint i = 0; i < id_list.getCount(); i++) {
		ext_param_list.clear();
		ext_param_list.add(temp_buf.Z().Cat(id_list.get(i)), 0);
		ext_param_list.add("Param 02", 0);
		ext_param_list.add("Param 03", 0);
		ext_param_list.add("Param 04", 0);
		{
			DlRtm::ExportParam ep;
			PPFilt _f(id_list.get(i));
			ep.P_F = &_f;
			THROW(tddo.Process(0, in_buf, ep, &ext_param_list, out_buf));
		}
		out_buf.WriteByte('\n');
	}
	out_file.Write(out_buf, out_buf.GetAvailableSize());
	CATCH
		ok = CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
#endif // } USE_TDDO_2
