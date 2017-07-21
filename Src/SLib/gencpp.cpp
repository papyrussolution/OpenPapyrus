// GENCPP.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2016
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

Generator_CPP::Generator_CPP(const char * pFileName) : SFile()
{
	Open(pFileName);
	Indent = 0;
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
	TempBuf = 0;
	TempBuf.CatChar('#').Cat("include").Space();
	TempBuf.CatChar(quot ? '\"' : '<').Cat(pFileName).CatChar(quot ? '\"' : '>').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Define(const char * pMacro, const char * pVal)
{
	(TempBuf = 0).CatChar('#').Cat("define").Space().Cat(pMacro).Space().Cat(pVal).CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_IfDef(const char * pSymb, int _ifndef /*=0*/)
{
	(TempBuf = 0).CatChar('#').Cat(_ifndef ? "ifndef" : "ifdef").Space().Cat(pSymb).CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_EndIf(const char * pSymb)
{
	(TempBuf = 0).CatChar('#').Cat("endif");
	if(pSymb && pSymb[0])
		TempBuf.Space().CatCharN('/', 2).Space().Cat(pSymb);
	TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Indent()
{
	return WriteLine(CatIndent(TempBuf = 0));
}

SString & Generator_CPP::CatIndent(SString & rBuf)
{
	return rBuf.Tab(Indent);
}

SString & Generator_CPP::MakeClsfName(const char * pClsName, const char * pMembName, SString & rBuf) const
{
	return (rBuf = 0).Cat(pClsName).CatCharN(':', 2).Cat(pMembName);
}

int Generator_CPP::Wr_Comment(const char * pBuf)
{
	TempBuf = 0;
	CatIndent(TempBuf).CatCharN('/', 2);
	if(pBuf)
		TempBuf.Space().Cat(pBuf);
	TempBuf.CR();
	return WriteLine(TempBuf);
}

SString & Generator_CPP::CatCls(int cls, SString & rBuf)
{
	if(cls == clsClass)
		rBuf.Cat("class");
	else if(cls == clsStruct)
		rBuf.Cat("struct");
	else if(cls == clsUnion)
		rBuf.Cat("union");
	else if(cls == clsEnum)
		rBuf.Cat("enum");
	else if(cls == clsInterface)
		rBuf.Cat("interface");
	return rBuf;
}

SString & Generator_CPP::CatAcs(int acs, SString & rBuf)
{
	if(acs == acsPublic)
		rBuf.Cat("public");
	else if(acs == acsProtected)
		rBuf.Cat("protected");
	else if(acs == acsPrivate)
		rBuf.Cat("private");
	return rBuf;
}

int Generator_CPP::Wr_OpenBrace()
{
	TempBuf = 0;
	return WriteLine(CatIndent(TempBuf).CatChar('{').CR());
}

int Generator_CPP::Wr_CloseBrace(int addSemicolon, const char * pInstanceSymb)
{
	TempBuf = 0;
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
	TempBuf = 0;
	CatIndent(TempBuf);
	if(declAlignment) {
		TempBuf.Cat("__declspec(align(").Cat(declAlignment).CatCharN(')', 2).Space();
	}
	CatCls(cls, TempBuf).Space().Cat(pName).Space();
	if(pBase && pBase[0]) {
		TempBuf.CatChar(':').Space();
		CatAcs(acs, TempBuf).Space().Cat(pBase).Space();
	}
	TempBuf.CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartIdlInterfaceDecl(const char * pName, int dispIface)
{
	TempBuf = 0;
	TempBuf.Cat("interface").Space().Cat(pName).Space().CatChar(':').Space();
	TempBuf.Cat(dispIface ? "IDispatch" : "IUnknown");
	TempBuf.Space().CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartIdlCoClassDecl(const char * pName)
{
	TempBuf = 0;
	TempBuf.Cat("coclass").Space().Cat(pName).Space().CatChar('{').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_ClassAcsZone(int acs)
{
	TempBuf = 0;
	CatAcs(acs, TempBuf).CatChar(':').CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_ClassPrototype(int cls, const char * pName)
{
	TempBuf = 0;
	CatIndent(TempBuf);
	CatCls(cls, TempBuf).Space().Cat(pName).Semicol().CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_StartDeclFunc(int funcKind, int funcMod, const char * pRetType, const char * pName, int funcCallMod)
{
	TempBuf = 0;
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
	TempBuf = 0;
	TempBuf.Cat(pType);
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
	TempBuf = 0;
	TempBuf.CatChar(')');
	if(semicol)
		TempBuf.Semicol();
	if(newLine)
		TempBuf.CR();
	return WriteLine(TempBuf);
}

int Generator_CPP::Wr_Return(const char * pVal)
{
	TempBuf = 0;
	CatIndent(TempBuf).Cat("return").Space().Cat(pVal).Semicol().CR();
	return WriteLine(TempBuf);
}

