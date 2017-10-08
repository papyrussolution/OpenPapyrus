// TDDO2.CPP
// Copyright (c) A.Sobolev 2010, 2011, 2012, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

#ifndef USE_TDDO_2 // {
int SLAPI TestTddo2() // @stub
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

// static
int SLAPI Tddo::GetFileName(const char * pFileName, int fileType, const char * pInputFileName, SString & rResult)
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
			SPathStruc ps;
			ps.Split(pInputFileName);
			ps.Nam.Z();
			ps.Ext.Z();
			ps.Merge(path);
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

// static
int SLAPI Tddo::LoadFile(const char * pFileName, SString & rBuf)
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

SLAPI Tddo::Tddo()
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

SLAPI Tddo::~Tddo()
{
}

void SLAPI Tddo::SetInputFileName(const char * pFileName)
{
	InputFileName = pFileName;
}

Tddo::Meta::Meta()
{
	Tok = tNone;
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

Tddo::ProcessBlock::ProcessBlock()
{
	P_Rtm = 0;
	MEMSZERO(F);
}

Tddo::Result::Result()
{
	RefType = 0;
	RefID = 0;
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

int SLAPI Tddo::ResolveExpr(DlRtm * pRtm, const DlScope * pScope, DlRtm * pCallerRtm, SStrScan & rScan, Result & rR)
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
			PPFilt pf;
			MEMSZERO(pf);
			pf.ID = rR.RefID;
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
				empty_filt.Ptr = 0;
				empty_filt.ID = 0;
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
			PPFilt pf;
			MEMSZERO(pf);
			pf.ID = rR.RefID;
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
				empty_filt.Ptr = 0;
				empty_filt.ID = 0;
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
			cop.Set(_GE_, 2);
		else if(rScan.Is("<="))
			cop.Set(_LE_, 2);
		else if(rScan.Is("=="))
			cop.Set(_EQ_, 2);
		else if(rScan.Is("!="))
			cop.Set(_NE_, 2);
		else if(rScan.Is("<>"))
			cop.Set(_NE_, 2);
		else if(rScan.Is('>'))
			cop.Set(_GT_, 1);
		else if(rScan.Is('<'))
			cop.Set(_LT_, 1);
		else if(rScan.Is('='))
			cop.Set(_EQ_, 1);
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
						case _GT_: _r = BIN(a1 > a2); break;
						case _LT_: _r = BIN(a1 < a2); break;
						case _GE_: _r = BIN(a1 >= a2); break;
						case _LE_: _r = BIN(a1 <= a2); break;
						case _EQ_: _r = BIN(a1 == a2); break;
						case _NE_: _r = BIN(a1 != a2); break;
					}
					done = 1;
				}
			}
			if(!done) {
				const int sc = rR.S.CmpNC(temp_buf);
				switch(cop.Op) {
					case _GT_: _r = BIN(sc > 0); break;
					case _LT_: _r = BIN(sc < 0); break;
					case _GE_: _r = BIN(sc >= 0); break;
					case _LE_: _r = BIN(sc <= 0); break;
					case _EQ_: _r = BIN(sc == 0); break;
					case _NE_: _r = BIN(sc != 0); break;
				}
			}
			rR.Clear().S.Cat(_r);
		}
	}
	CATCHZOK
	// @v8.7.8 {
	if(pCallerRtm && pCallerRtm != pRtm)
		pRtm->P_Ep = pCallerRtm->P_Ep;
	// } @v8.7.8
	return ok;
}

int SLAPI Tddo::GetVar(const SString & rInput, SString & rBuf) const
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

void SLAPI Tddo::Skip()
{
	Scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine, &LineNo);
}

int SLAPI Tddo::Process(const char * pDataName, const char * pBuf, DlRtm::ExportParam & rEp, const StringSet * pExtParamList, SBuffer & rOut)
{
	int    ok = 1;
	LineNo = 1;
	if(!RVALUEPTR(ExtParamList, pExtParamList))
		ExtParamList.clear(1);
	ProcessBlock pblk;
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

int SLAPI Tddo::ResolveArgN(const SString & rText, Result & rR)
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

int SLAPI Tddo::ResolveVar(const SString & rText, const DlScope * pScope, Result & rR)
{
	rR.Clear();
	int    ok = 1;
	SString temp_buf;
	if(rText == "__FILE__") {
		rR.S = InputFileName;
	}
	else if(rText == "__FILEDIR__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps;
			ps.Split(InputFileName);
			ps.Nam.Z();
			ps.Ext.Z();
			ps.Merge(rR.S);
			rR.S.SetLastSlash();
		}
	}
	else if(rText == "__FILENAME__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps;
			ps.Split(InputFileName);
			rR.S = ps.Nam;
		}
	}
	else if(rText == "__FILEEXT__") {
		if(InputFileName.NotEmpty()) {
			SPathStruc ps;
			ps.Split(InputFileName);
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
		IntArray fpl[3]; // func pos list
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
			IntArray func_pos_list;
			IntArray cvt_arg_list, cvt_al[3];
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
					for(j = 0; j < 3; j++)
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
									SString * p_str = *(SString **)S.GetPtr(arg_sp);
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
								SString * p_str = *(SString **)S.GetPtr(ret_sp);
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

int SLAPI Tddo::IsTextSection(const SString & rLineBuf, const char * pPattern, SString * pRet)
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

int SLAPI Tddo::ExtractText(const char * pFileName, const char * pTextIdent, int langId, SBuffer & rOut)
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

/*int SLAPI Tddo::Helper_RecognizeExprToken(long flags, SString & rText)
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
			t = tOperator+_EQ_;
		}
		else if(Scan.Is("!=") || Scan.Is("<>")) {
			Scan.Incr(2);
			rText.Z().Cat("!=");
			t = tOperator+_NE_;
		}
		else if(Scan.Is(">=")) {
			Scan.Incr(2);
			rText.Z().Cat(">=");
			t = tOperator+_GE_;
		}
		else if(Scan.Is("<=")) {
			Scan.Incr(2);
			rText.Z().Cat("<=");
			t = tOperator+_LE_;
		}
		else if(oneof9(c, '+', '-', '*', '/', '%', '<', '>', '=', '.')) {
			Scan.Incr();
			rText.Z().CatChar(c);
			switch(c) {
				case '+': t = tOperator + _PLUS_; break;
				case '-': t = tOperator + _MINUS_; break;
				case '*': t = tOperator + _MULT_; break;
				case '/': t = tOperator + _DIVIDE_; break;
				case '%': t = tOperator + _MODULO_; break;
				case '<': t = tOperator + _LT_; break;
				case '>': t = tOperator + _GT_; break;
				case '=': t = tOperator + _ASSIGN_; break;
				case '.': t = tOperator + _DOT_; break;
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

int SLAPI Tddo::Helper_RecognizeMetaKeyword()
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

int SLAPI Tddo::Helper_Process(ProcessBlock & rBlk, SBuffer & rOut, Meta & rMeta, const DlScope * pScope, int skipOutput)
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
		for(int exit_loop = 0; !exit_loop && !isempty((const char*)Scan);) {
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
							ProcessBlock pblk;
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

class TddoContentGraph : public SStrGroup {
public:
	enum {
		kStart = 1, // Стартовый фиктивный блок
		kFinish,    // Финишный фиктивный блок 
		kText,
		kExpr,
		kIf,        // branch
		kElse,      // branch
		kElseIf,    // branch
		kIter,      // branch 
		kMacro,     // branch 
		kMacroCall,
		kSet
	};
	struct Chunk {
		int    Kind;
		uint   NextP;
		uint   BranchP;
		uint   TextP;
	};
	enum {
		untiltokEot    = 0x0001,
		untiltokRBrace = 0x0002,
		untiltokRPar   = 0x0004,
		untiltokComma  = 0x0008,
		untiltokSpace  = 0x0010  // space || tab || newline
	};
	class ExprSet {
	public:
		enum {
			kOp = 1,
			kFunc,
			kString,
			kNumber,
			kVar
		};
		struct Item {
			SLAPI  Item()
			{
				K = 0;
				ArgCount = 0;
				Op = 0;
			}
			uint16 K;
			uint16 ArgCount; // Количество аргументов (для oneof(K, kOp, kFunc))
			union {
				uint   SymbP; // Позиция символа в R_Set.Pool (для oneof(K, kString, kVar, kFunc))
				uint32 Op;    // Ид операции (для K == kOp)
				double R;     // Значение числа (K == kNumber)
			};
		};
		class Stack : public TSStack <TddoContentGraph::ExprSet::Item> {
		public:
			SLAPI  Stack()
			{
			}
			int FASTCALL Push(const Stack & rS)
			{
				int    ok = -1;
				for(uint i = 0; i < rS.getPointer(); i++) {
					push(*(TddoContentGraph::ExprSet::Item *)rS.at(i));
					ok = 1;
				}
				return ok;
			}
		};
		class Expression {
		public:
			SLAPI  Expression()
			{
				NameP = 0;
				Flags = 0;
			}
			uint   NameP;
			long   Flags;
			TddoContentGraph::ExprSet::Stack ES; // Стэк выражения //
		};

		SLAPI  ExprSet(TddoContentGraph & rG) : R_G(rG)
		{
		}
		SLAPI ~ExprSet()
		{
		}
		// untilToken: } | ) | , | EOF
		int    Parse(uint untilToken, uint * pPos)
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
		int    Helper_Parse(uint untilToken, TddoContentGraph::ExprSet::Stack & rStack)
		{
			int    ok = 1;
			Tddo::Meta meta;
			SString temp_buf;
			SStrScan & r_scan = R_G.Scan;
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
					R_G.Skip();
					if(untilToken & untiltokRBrace && r_scan[0] == '}')
						done = 1;
					else if(untilToken & untiltokRPar && r_scan[0] == ')')
						done = 1;
					else if(untilToken & untiltokComma && r_scan[0] == ',')
						done = 1;
					else {
						int r = R_G.Helper_RecognizeExprToken(0, temp_buf);
						if(r == Tddo::tLiteral) {
							Item ei;
							ei.K = kString;
							R_G.AddS(temp_buf, &ei.SymbP);
							rStack.push(ei);
						}
						else if(r == Tddo::tNumber) {
							Item ei;
							ei.K = kNumber;
							ei.R = temp_buf.ToReal();
							rStack.push(ei);
						}
						else if(r == Tddo::tVar) {
							Item ei;
							ei.K = kVar;
							R_G.AddS(temp_buf, &ei.SymbP);
							rStack.push(ei);
						}
						else if(r == Tddo::tVarArgN) {
							Item ei;
							ei.K = kVar;
							R_G.AddS(temp_buf, &ei.SymbP);
							rStack.push(ei);
						}
						else if(r == Tddo::tDollarBrace) {
							Helper_Parse(untiltokRBrace, rStack); // @recursion
						}
						else if(r == Tddo::tFunc) {
							int    local_ok = 1;
							Item ei;
							ei.K = kFunc;
							R_G.AddS(temp_buf, &ei.SymbP);
							TSCollection <Stack> local_arg_list;
							do {
								uint   local_stk_pos = 0;
								Stack * p_local_stk = local_arg_list.CreateNewItem(&local_stk_pos);
								THROW_SL(p_local_stk);
								local_ok = Helper_Parse(untiltokComma|untiltokRPar, *p_local_stk); // @recursion
								if(p_local_stk->getPointer() == 0) {
									local_arg_list.atFree(local_stk_pos);
									if(r_scan[0] != ')' && local_ok)
										local_ok = 0; // @error Empty expression
								}
							} while(local_ok && r_scan[0] != ')');
							if(r_scan[0] == ')') {
								r_scan.Incr();
							}
							if(local_ok) {
								ei.ArgCount = local_arg_list.getCount();
								rStack.push(ei);
								for(uint i = 0; i < local_arg_list.getCount(); i++) {
									rStack.Push(*local_arg_list.at(i));
								}
							}
						}
						else if(r > Tddo::tOperator) {
							Item ei;
							ei.K = kOp;
							ei.Op = (r - Tddo::tOperator);
							ei.ArgCount = 2;
							rStack.push(ei);
						}
					}
				}
			}
			CATCHZOK
			return ok;
		}
	private:
		TddoContentGraph & R_G;
		TSCollection <Expression> EL;
	};
	SLAPI  TddoContentGraph(Tddo & rT) : R_T(rT), ES(*this)
	{
	}
	SLAPI ~TddoContentGraph()
	{
	}
	enum {
		stopEot    = 0x0001,
		stopEnd    = 0x0002,
		stopEndIf  = 0x0004,
		stopElse   = 0x0008,
		stopElseIf = 0x0010
	};
	int    SLAPI Parse(const char * pSrc)
	{
		int    ok = 1;
		SString temp_buf;
		L.clear();
		EndChunkP = 0;
		uint start_p = AddBlock(kStart, temp_buf.Z());
		assert(start_p == 0);
		EndChunkP = AddBlock(kFinish, temp_buf.Z());
		Scan.Set(pSrc, 0);
		LineNo = 0;
		Helper_Parse(0, 0, stopEot);
		return ok;
	}
	int    SLAPI Helper_Parse(uint parentChunkP, int isBranch, int stopEvent)
	{
		int    ok = 1;
		uint   prev_scan_stack_pos = 0;
		SString temp_buf, msg_buf;
		Tddo::Meta   meta;

		struct CurrentBlock {
			CurrentBlock(uint parentChunkP, int isBrach)
			{
				Kind = 0;
				P = parentChunkP;
				IsBranch = isBrach;
			}
			int    AppendToList(TddoContentGraph * pG)
			{
				int    ok = 1;
				uint   _p = pG->AddBlock(Kind, Text);
				if(IsBranch) {
					pG->L.at(P).BranchP = _p;
				}
				else {
					pG->L.at(P).NextP = _p;
				}
				P = _p;
				IsBranch = 0;
				Kind = 0;
				Text.Z();
				return ok;
			}
			int    Kind;
			int    IsBranch;
			uint   P;
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
												uint   arg_p = 0;
												AddS(temp_buf, &arg_p);
												macro_arg_list.add((long)arg_p);
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
								GetS(macro_name_p, temp_buf);
								_cb.Text = temp_buf;
								_cb.Kind = kMacro;
								_cb.AppendToList(this);
								Helper_Parse(_cb.P, 1, stopEot|stopEnd); // @recursion
							}
							else {
								local_ok = 0; // @error
							}
						}
						break;
					case Tddo::tMacroCall:
						{
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
										macro_arg_list.add((long)expr_p);
									}
									Skip();
									if(Scan[0] == ',') { // optional ','
										Scan.Incr();
										Scan.Skip();
									}
								} while(!Scan.IsEnd() && Scan[0] != ')');
								if(Scan[0] == ')') {
									Scan.Incr();
								}
								else {
									; // @error
								}
							}
							else {
								; // @error
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
								_cb.Text.Z();
								_cb.Kind = kSet;
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
						break;
					case Tddo::tStop:
						break;
					case Tddo::tIf:
						Scan.Skip();
						if(Scan[0] == '(') {
							Scan.Incr();
							uint   expr_p = 0;
							if(ES.Parse(untiltokRPar, &expr_p)) {
								_cb.Text.Z();
								_cb.Kind = kIf;
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
					case Tddo::tElse:
						if(stopEvent & stopElse) {
							exit_loop = 1;
						}
						break;
					case Tddo::tElif:
						if(stopEvent & stopElseIf) {
							exit_loop = 1;
						}
						break;
					case Tddo::tForEach:
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
					LineNo++;
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
	int    SLAPI Helper_RecognizeExprToken(long flags, SString & rText)
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
			else if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
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
			if(c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				do {
					text[tp++] = c;
					c = Scan[++sp];
				} while(c == '_' || isdec(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
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
			if(Scan.Is("&&")) {
				Scan.Incr(2);
				rText = "&&";
				t = Tddo::tOperator+_AND_;
			}
			else if(Scan.Is("and")) {
				Scan.Incr(3);
				rText = "&&";
				t = Tddo::tOperator+_AND_;
			}
			else if(Scan.Is("||")) {
				Scan.Incr(2);
				rText = "||";
				t = Tddo::tOperator+_OR_;
			}
			else if(Scan.Is("or")) {
				Scan.Incr(2);
				rText = "||";
				t = Tddo::tOperator+_OR_;
			}
			else if(Scan.Is("==")) {
				Scan.Incr(2);
				rText = "==";
				t = Tddo::tOperator+_EQ_;
			}
			else if(Scan.Is("!=") || Scan.Is("<>")) {
				Scan.Incr(2);
				rText = "!=";
				t = Tddo::tOperator+_NE_;
			}
			else if(Scan.Is(">=")) {
				Scan.Incr(2);
				rText = ">=";
				t = Tddo::tOperator+_GE_;
			}
			else if(Scan.Is("<=")) {
				Scan.Incr(2);
				rText = "<=";
				t = Tddo::tOperator+_LE_;
			}
			else if(oneof9(c, '+', '-', '*', '/', '%', '<', '>', '=', '.')) {
				Scan.Incr();
				rText.Z().CatChar(c);
				switch(c) {
					case '+': t = Tddo::tOperator + _PLUS_; break;
					case '-': t = Tddo::tOperator + _MINUS_; break;
					case '*': t = Tddo::tOperator + _MULT_; break;
					case '/': t = Tddo::tOperator + _DIVIDE_; break;
					case '%': t = Tddo::tOperator + _MODULO_; break;
					case '<': t = Tddo::tOperator + _LT_; break;
					case '>': t = Tddo::tOperator + _GT_; break;
					case '=': t = Tddo::tOperator + _ASSIGN_; break;
					case '.': t = Tddo::tOperator + _DOT_; break;
				}
				assert(t > Tddo::tOperator);
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
					Scan.Incr(sp+1); // + '\"'
					rText = text;
					t = Tddo::tLiteral;
				}
			}
			else if(Scan.IsNumber()) {
				Scan.GetNumber(rText);
				t = Tddo::tNumber;
			}
		}
		return t;
	}
	int    SLAPI Helper_RecognizeMetaKeyword(SString & rAddendum)
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
					LineNo++;
				m = Helper_RecognizeMetaKeyword(rAddendum); // @recusion
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
				m = Helper_RecognizeMetaKeyword(rAddendum); // @recusion
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
						m = Tddo::tMacro;
					else if(sstreq(text, "set"))
						m = Tddo::tSet;
					else if(sstreq(text, "break"))
						m = Tddo::tBreak;
					else if(sstreq(text, "stop"))
						m = Tddo::tStop;
					else if(sstreq(text, "if"))
						m = Tddo::tIf;
					else if(sstreq(text, "else"))
						m = Tddo::tElse;
					else if(sstreq(text, "elseif") || sstreq(text, "elif"))
						m = Tddo::tElif;
					else if(sstreq(text, "foreach"))
						m = Tddo::tForEach;
					else if(sstreq(text, "endif"))
						m = Tddo::tEndif;
					else if(sstreq(text, "end"))
						m = Tddo::tEnd;
					else if(sstreq(text, "codepage"))
						m = Tddo::tCodepage;
					else if(sstreq(text, "pragma"))
						m = Tddo::tPragma;
					else if(sstreq(text, "start"))
						m = Tddo::tStart;
					else if(sstreq(text, "rem"))
						m = Tddo::tRem;
					else if(sstreq(text, "iter"))
						m = Tddo::tIter;
					else if(sstreq(text, "itercount"))
						m = Tddo::tIterCount;
					else if(sstreq(text, "text"))
						m = Tddo::tText;
					else if(sstreq(text, "include"))
						m = Tddo::tInclude;
					else if(text[0]) {
						m = Tddo::tMacroCall;
						rAddendum = text;
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
	void   SLAPI Skip()
	{
		Scan.Skip(SStrScan::wsSpace|SStrScan::wsTab|SStrScan::wsNewLine, &LineNo);
	}

	int    SLAPI Output(SString & rBuf)
	{
		rBuf.Z();
		return ChunkToStr(0, 0, rBuf);
	}
private:
	struct ChunkInner {
		int    Kind;    // Вид блока (TddoContentGraph::kXXX)
		uint   NextP;   // Следующий блок
		uint   BranchP; // Ответвление (if | else | iter)
		uint   HeadP;   // Головной блок (для ответвления - узел ветвления)
		uint   TextP;   // Текст
		uint   ExprP;   // Выражение  
	};
	int    SLAPI ChunkToStr(uint cp, uint tabLevel, SString & rBuf) const
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
		if(r_chunk.ExprP) {
			rBuf.Space().CatEq("ExprP", r_chunk.ExprP);
		}
		if(r_chunk.TextP) {
			GetS(r_chunk.TextP, temp_buf);
			rBuf.Space().CatEq("Text", temp_buf);
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
	uint   AddBlock(int kind, const SString & rText)
	{
		uint   result_pos = 0;
		ChunkInner chunk;
		MEMSZERO(chunk);
		chunk.Kind = kind;
		AddS(rText, &chunk.TextP);
		result_pos = L.getCount();
		L.insert(&chunk);
		return result_pos;
	}

	TSArray <ChunkInner> L;
	// Блок по индексу 0 всегда стартовый (фиктивный блок)
	uint   EndChunkP;   // Позиция завершающего (фиктивного) блока 
	ExprSet ES; // Коллекция выражений. На них ссылаются ChunkInner::ExprP
	Tddo & R_T;
	SStrScan Scan;
	uint   LineNo;
};
//
// Implementation of PPALDD_HttpPreprocessBase
//
PPALDD_CONSTRUCTOR(HttpPreprocessBase)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
}

PPALDD_DESTRUCTOR(HttpPreprocessBase)
{
	Destroy();
}

int PPALDD_HttpPreprocessBase::InitData(PPFilt & rFilt, long rsrv)
{
	return DlRtm::InitData(rFilt, rsrv);
}

int SLAPI TestTddo2()
{
	int    ok = 1;
	SString inp_file_name;
	SString temp_buf, in_buf;
	SBuffer out_buf;
	LongArray id_list;
	StringSet ext_param_list;
	id_list.addUnique(55);
	id_list.addUnique(3);
	PPGetPath(PPPATH_TESTROOT, inp_file_name);
	inp_file_name.SetLastSlash().Cat("data").SetLastSlash().Cat("tddo2-test-01.tddo");
	SFile in_file(inp_file_name, SFile::mRead);
	PPGetPath(PPPATH_TESTROOT, temp_buf);
	temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("tddo2-test-01-out.tddo");
	SFile out_file(temp_buf, SFile::mWrite);
	THROW_SL(in_file.IsValid());
	THROW_SL(out_file.IsValid());
	{
		Tddo tddo;
		TddoContentGraph tcg(tddo);
		tddo.SetInputFileName(inp_file_name);
		while(in_file.ReadLine(temp_buf))
			in_buf.Cat(temp_buf);
		tcg.Parse(in_buf);
		tcg.Output(temp_buf);
		out_file.WriteLine(temp_buf);
	}
#if 0 // {
	for(uint i = 0; i < id_list.getCount(); i++) {
		DlRtm::ExportParam ep;
		PPFilt _pf;
		_pf.ID = id_list.get(i);
		_pf.Ptr = 0;
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
int SLAPI TestTddo()
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
		PPFilt _pf;
		_pf.ID = id_list.get(i);
		_pf.Ptr = 0;
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
		ext_param_list.clear(1);
		ext_param_list.add(temp_buf.Z().Cat(id_list.get(i)), 0);
		ext_param_list.add("Param 02", 0);
		ext_param_list.add("Param 03", 0);
		ext_param_list.add("Param 04", 0);
		{
			DlRtm::ExportParam ep;
			PPFilt _f;
			_f.ID = id_list.get(i);
			_f.Ptr = 0;
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
