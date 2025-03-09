// GENCPP.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2016, 2018, 2020, 2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop

Generator_CPP::Generator_CPP(const char * pFileName) : SFile(), Indent(0)
{
	Open(pFileName);
}

int Generator_CPP::Open(const char * pFileName)
{
	Indent = 0;
	return pFileName ? SFile::Open(pFileName, SFile::mWrite) : -1;
}

void Generator_CPP::IndentInc()
{
	++Indent;
}

void Generator_CPP::IndentDec()
{
	if(Indent)
		--Indent;
}

int Generator_CPP::Wr_Include(const char * pFileName, int quot)
{
	TempBuf.Z();
	TempBuf.CatChar('#').Cat("include").Space();
	TempBuf.CatChar(quot ? '\"' : '<').Cat(pFileName).CatChar(quot ? '\"' : '>').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Define(const char * pMacro, const char * pVal)
{
	TempBuf.Z().CatChar('#').Cat("define").Space().Cat(pMacro);
	if(!isempty(pVal))
		TempBuf.Space().Cat(pVal);
	TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_IfDef(const char * pSymb, int _ifndef /*=0*/)
{
	TempBuf.Z().CatChar('#').Cat(_ifndef ? "ifndef" : "ifdef").Space().Cat(pSymb).CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_EndIf(const char * pSymb)
{
	TempBuf.Z().CatChar('#').Cat("endif");
	if(!isempty(pSymb))
		TempBuf.Space().CatCharN('/', 2).Space().Cat(pSymb);
	TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Indent()
{
	return WriteLine(CatIndent(TempBuf.Z()));
}

SString & Generator_CPP::CatIndent(SString & rBuf)
{
	if(Indent)
		rBuf.Tab_(Indent);
	return rBuf;
}

SString & Generator_CPP::MakeClsfName(const char * pClsName, const char * pMembName, SString & rBuf) const
{
	return rBuf.Z().Cat(pClsName).CatCharN(':', 2).Cat(pMembName);
}

int Generator_CPP::Wr_Comment(const char * pBuf)
{
	TempBuf.Z();
	CatIndent(TempBuf).CatCharN('/', 2);
	if(!isempty(pBuf))
		TempBuf.Space().Cat(pBuf);
	TempBuf.CR();
	return WriteLine(TempBuf);
}

SString & Generator_CPP::CatCls(int cls, SString & rBuf)
{
	switch(cls) {
		case clsClass: rBuf.Cat("class"); break;
		case clsStruct: rBuf.Cat("struct"); break;
		case clsUnion: rBuf.Cat("union"); break;
		case clsEnum: rBuf.Cat("enum"); break;
		case clsInterface: rBuf.Cat("interface"); break;
	}
	return rBuf;
}

SString & Generator_CPP::CatAcs(int acs, SString & rBuf)
{
	switch(acs) {
		case acsPublic: rBuf.Cat("public"); break;
		case acsProtected: rBuf.Cat("protected"); break;
		case acsPrivate: rBuf.Cat("private"); break;
	}
	return rBuf;
}

int Generator_CPP::Wr_OpenBrace()
{
	TempBuf.Z();
	return WriteLine(CatIndent(TempBuf).CatChar('{').CR());
}

int Generator_CPP::Wr_CloseBrace(int addSemicolon, const char * pInstanceSymb)
{
	TempBuf.Z();
	CatIndent(TempBuf).CatChar('}');
	if(addSemicolon) {
		if(pInstanceSymb)
			TempBuf.Space().Cat(pInstanceSymb);
		TempBuf.Semicol();
	}
	TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartClassDecl(int cls, const char * pName, const char * pBase, int acs, uint declAlignment)
{
	TempBuf.Z();
	CatIndent(TempBuf);
	CatCls(cls, TempBuf);
	if(declAlignment) {
		if(oneof5(declAlignment, 1, 2, 4, 8, 16)) {
			//TempBuf.Space().Cat("__declspec(align(").Cat(declAlignment).CatCharN(')', 2);
			TempBuf.Space().Cat("alignas(").Cat(declAlignment).CatChar(')');
		}
		else {
			; // @todo warning message
		}
	}
	TempBuf.Space().Cat(pName).Space();
	if(!isempty(pBase)) {
		TempBuf.CatDiv(':', 2);
		CatAcs(acs, TempBuf).Space().Cat(pBase).Space();
	}
	TempBuf.CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartIdlInterfaceDecl(const char * pName, int dispIface)
{
	TempBuf.Z().Cat("interface").Space().Cat(pName).CatDiv(':', 1);
	TempBuf.Cat(dispIface ? "IDispatch" : "IUnknown");
	TempBuf.Space().CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartIdlCoClassDecl(const char * pName)
{
	TempBuf.Z().Cat("coclass").Space().Cat(pName).Space().CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_ClassAcsZone(int acs)
{
	TempBuf.Z();
	CatAcs(acs, TempBuf).Colon().CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_ClassPrototype(int cls, const char * pName)
{
	TempBuf.Z();
	CatIndent(TempBuf);
	CatCls(cls, TempBuf).Space().Cat(pName).Semicol().CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartDeclFunc(int funcKind, int funcMod, const char * pRetType, const char * pName, int funcCallMod)
{
	TempBuf.Z();
	CatIndent(TempBuf);
	if(funcMod & fmVirtual)
		TempBuf.Cat("virtual").Space();
	else if(funcMod & fmStatic)
		TempBuf.Cat("static").Space();
	if(funcKind == fkConstr)
		TempBuf.Cat(pName).CatChar('(');
	else if(funcKind == fkDestr)
		TempBuf.CatChar('~').Cat(pName).CatChar('(');
	else {
		TempBuf.Cat(pRetType).Space();
		if(funcCallMod == fcmCDecl)
			TempBuf.Cat("__cdecl").Space();
		else if(funcCallMod == fcmStdCall)
			TempBuf.Cat("__stdcall").Space();
		else if(funcCallMod == fcmFastCall)
			TempBuf.Cat("__fastcall").Space();
		TempBuf.Cat(pName).CatChar('(');
	}
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_VarDecl(const char * pType, const char * pName, const char * pDef, int term)
{
	TempBuf.Z().Cat(pType);
	if(pName)
		TempBuf.Space().Cat(pName);
	if(pDef)
		TempBuf.CatDiv('=', 1).Cat(pDef);
	if(term == ',')
		TempBuf.Comma().Space();
	else if(term == ';')
		TempBuf.Semicol();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_EndDeclFunc(int semicol, int newLine)
{
	TempBuf.Z().CatChar(')');
	if(semicol)
		TempBuf.Semicol();
	if(newLine)
		TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Return(const char * pVal)
{
	TempBuf.Z();
	CatIndent(TempBuf).Cat("return").Space().Cat(pVal).Semicol().CR();
	return WriteLine(TempBuf);
}
