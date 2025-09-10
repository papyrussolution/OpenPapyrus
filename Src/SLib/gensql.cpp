// GENSQL.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2010, 2013, 2015, 2017, 2018, 2019, 2020, 2022, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

Generator_SQL::Generator_SQL(SqlServerType sqlst, long flags) : 
	Sqlst(oneof6(sqlst, sqlstGeneric, sqlstORA, sqlstMSS, sqlstFB, sqlstMySQL, sqlstSQLite) ? sqlst : sqlstGeneric), Flags(flags), Typ(typUndef)
{
}

Generator_SQL & FASTCALL Generator_SQL::Tok(int tok)
{
	if(Buf.IsEmpty()) {
		switch(tok) {
			case tokSelect: Typ = typSelect; break;
			case tokInsert: Typ = typInsert; break;
			case tokUpdate: Typ = typUpdate; break;
			case tokDelete: Typ = typDelete; break;
			default: Typ = typUndef; break;
		}
	}
	else {
		switch(tok) {
			case tokTransaction:
				{
					SString & r_temp_buf = SLS.AcquireRvlStr();
					(r_temp_buf = Buf).Strip();
					if(r_temp_buf.IsEqiAscii("begin"))
						Typ = typBeginTransaction;
					else if(r_temp_buf.IsEqiAscii("commit"))
						Typ = typCommitTransaction;
					else if(r_temp_buf.IsEqiAscii("rollback"))
						Typ = typRollbackTransaction;
				}
				break;
		}
	}
	Buf.Cat(GetToken(tok));
	return *this;
}

Generator_SQL & Generator_SQL::Z()
{
	Buf.Z();
	Typ = typUndef;
	return *this;
}

Generator_SQL & FASTCALL Generator_SQL::Text(const char * pName)
{
	Buf.Cat(pName);
	return *this;
}

Generator_SQL & FASTCALL Generator_SQL::QText(const char * pName)
{
	Buf.CatChar('\'').Cat(pName).CatChar('\'');
	return *this;
}

Generator_SQL & FASTCALL Generator_SQL::Param(const char * pParam)
{
	if(Sqlst == sqlstORA)
		Buf.Colon();
	else if(Sqlst == sqlstMySQL)
		Buf.CatChar('@');
	else
		Buf.Colon(); // @?
	Buf.Cat(pParam);
	return *this;
}

Generator_SQL & Generator_SQL::Cr()
{
	if(Flags & fIndent)
		Buf.CR();
	else
		Buf.Space();
	return *this;
}

Generator_SQL & Generator_SQL::Tab()
{
	if(Flags & fIndent)
		Buf.Tab();
	return *this;
}

Generator_SQL & Generator_SQL::LPar()
{
	Buf.CatChar('(');
	return *this;
}

Generator_SQL & Generator_SQL::RPar()
{
	Buf.CatChar(')');
	return *this;
}

Generator_SQL & Generator_SQL::Sp()
{
	Buf.CatChar(' ');
	return *this;
}

Generator_SQL & Generator_SQL::Aster()
{
	Buf.CatChar('*');
	return *this;
}

Generator_SQL & Generator_SQL::Com()
{
	Buf.Comma();
	return *this;
}

Generator_SQL & Generator_SQL::Dot()
{
	Buf.Dot();
	return *this;
}

Generator_SQL & FASTCALL Generator_SQL::Select(const BNFieldList * pFldList)
{
	Tok(Generator_SQL::tokSelect).Sp();
	if(pFldList) {
		for(uint i = 0; i < pFldList->getCount(); i++) {
			if(i)
				Buf.Comma();
			Text((*pFldList)[i].Name);
		}
	}
	else
		Aster();
	return Sp();
}

Generator_SQL & Generator_SQL::From(const char * pTable, const char * pAlias)
{
	Tok(Generator_SQL::tokFrom).Sp().Text(pTable);
	if(pAlias)
		Sp().Text(pAlias);
	return *this;
}

Generator_SQL & Generator_SQL::Eq(const char * pFldName, const char * pVal)
{
	Text(pFldName)._Symb(_EQ_);
	Buf.CatChar('\'').Cat(pVal).CatChar('\'');
	return *this;
}

Generator_SQL & Generator_SQL::Eq(const char * pFldName, long val)
{
	Text(pFldName)._Symb(_EQ_);
	Buf.Cat(val);
	return *this;
}

Generator_SQL & FASTCALL Generator_SQL::_Symb(int s)
{
	switch(s) {
		case _GT_: Buf.Cat(">"); break;
		case _GE_: Buf.Cat(">="); break;
		case _LT_: Buf.Cat("<"); break;
		case _LE_: Buf.Cat("<="); break;
		case _EQ_: Buf.Cat("="); break;
		case _NE_: Buf.Cat("<>"); break;
	}
	return *this;
}

SString & Generator_SQL::GetType(TYPEID typ, SString & rBuf)
{
	rBuf.Z();
	int _t = GETSTYPE(typ);
	int _s = GETSSIZE(typ);
	switch(Sqlst) {
		case sqlstMySQL:
			switch(_t) {
				case S_CHAR:     rBuf.Cat("VARCHAR").CatParStr(_s); break;
				case S_LSTRING:  rBuf.Cat("VARCHAR").CatParStr(_s); break;
				case S_ZSTRING:  rBuf.Cat("VARCHAR").CatParStr(_s); break;
				case S_INT:
				case S_UINT:     rBuf.Cat("INT"); break; 
				case S_AUTOINC:  rBuf.Cat("INT AUTO_INCREMENT UNIQUE"); break;
				case S_INT64:
				case S_UINT64:   rBuf.Cat("BIGINT"); break; 
				case S_UUID_:    rBuf.Cat("BINARY").CatParStr(16); break; 
				case S_FLOAT:    
					if(_s == 4)
						rBuf.Cat("FLOAT"); 
					else
						rBuf.Cat("DOUBLE"); 
					break;
				case S_DATE:     rBuf.Cat("DATE"); break;
				case S_TIME:     rBuf.Cat("TIME"); break; 
				case S_DATETIME: rBuf.Cat("DATETIME"); break;
				case S_DEC:
				case S_MONEY:
					_s = GETSSIZED(typ);
					rBuf.Cat("NUMERIC").CatChar('(').Cat(_s * 2 - 2).Comma().Cat(GETSPRECD(typ)).CatChar(')'); break;
				case S_BOOL:
				case S_BIT:   rBuf.Cat("NUMERIC").CatParStr(1); break;
				case S_NOTE:  rBuf.Cat("VARCHAR").CatParStr(_s); break;
				case S_WCHAR: rBuf.Cat("NCHAR").CatParStr(_s/2); break;
				case S_WZSTRING: rBuf.Cat("CHAR").CatParStr(_s/2).Space().Cat("CHARACTER SET utf16"); break;
				case S_RAW:   rBuf.Cat("BINARY").CatParStr(_s); break;
				case S_BLOB:  rBuf.Cat("MEDIUMBLOB"); break;
				case S_CLOB:  rBuf.Cat("MEDIUMTEXT"); break;
				default:      rBuf.Cat("UNSUPPORTED_TYPE"); break;
			}
			break;
		case sqlstORA:
			switch(_t) {
				case S_CHAR:     rBuf.Cat("VARCHAR2").CatParStr(_s); break;
				case S_LSTRING:  rBuf.Cat("VARCHAR2").CatParStr(_s); break;
				case S_ZSTRING:  rBuf.Cat("VARCHAR2").CatParStr(_s); break;
				case S_INT:
				case S_UINT:
				case S_AUTOINC:  rBuf.Cat("NUMERIC").CatParStr(12); break;
				case S_INT64:
				case S_UINT64:   rBuf.Cat("NUMERIC").CatParStr(19); break;
				case S_UUID_:    rBuf.Cat("RAW").CatParStr(16); break;
				case S_FLOAT:    rBuf.Cat("NUMERIC").CatChar('(').Cat(38).Comma().Cat(12).CatChar(')'); break;
				case S_DATE:     rBuf.Cat("DATE"); break;
				case S_TIME:
				case S_DATETIME: rBuf.Cat("TIMESTAMP").CatParStr(2); break;
				case S_DEC:
				case S_MONEY:
					_s = GETSSIZED(typ);
					rBuf.Cat("NUMERIC").CatChar('(').Cat(_s * 2 - 2).Comma().Cat(GETSPRECD(typ)).CatChar(')'); break;
				case S_BOOL:
				case S_BIT:   rBuf.Cat("NUMERIC").CatParStr(1); break;
				case S_NOTE:  rBuf.Cat("VARCHAR2").CatParStr(_s); break;
				case S_WCHAR: rBuf.Cat("NCHAR").CatParStr(_s/2); break;
				case S_RAW:   rBuf.Cat("RAW").CatParStr(_s); break;
				case S_BLOB:  rBuf.Cat("BLOB"); break;
				case S_CLOB:  rBuf.Cat("CLOB"); break;
				default:      rBuf.Cat("UNSUPPORTED_TYPE"); break;
			}
			break;
		case sqlstSQLite: // @v11.1.2
			switch(_t) {
				case S_CHAR:     rBuf.Cat("TEXT").CatParStr(_s); break;
				case S_LSTRING:  rBuf.Cat("TEXT").CatParStr(_s); break;
				case S_ZSTRING:  rBuf.Cat("TEXT").CatParStr(_s); break;
				case S_INT:
				case S_UINT:     rBuf.Cat("INTEGER").CatParStr(4L); break; 
				case S_AUTOINC:  rBuf.Cat("INTEGER PRIMARY KEY AUTOINCREMENT"); break; // @v12.3.12 (AUTOINCREMENT)
				case S_INT64:
				case S_UINT64:   rBuf.Cat("INTEGER").CatParStr(8L); break; 
				case S_UUID_:    rBuf.Cat("BLOB").CatParStr(16); break; 
				case S_FLOAT:    rBuf.Cat("REAL"); break;
				case S_DATE:     rBuf.Cat("INTEGER").CatParStr(4L); break;
				case S_TIME:     rBuf.Cat("INTEGER").CatParStr(4L); break; 
				case S_DATETIME: rBuf.Cat("INTEGER").CatParStr(8L); break;
				case S_DEC:
				case S_MONEY:    rBuf.Cat("REAL"); break;
				case S_BOOL:
				case S_BIT:      rBuf.Cat("INTEGER").CatParStr(1); break;
				case S_NOTE:     rBuf.Cat("TEXT").CatParStr(_s); break;
				case S_WCHAR:    rBuf.Cat("TEXT").CatParStr(_s/2); break;
				case S_WZSTRING: rBuf.Cat("TEXT").CatParStr(_s/2); break;
				case S_RAW:      rBuf.Cat("BLOB").CatParStr(_s); break;
				case S_BLOB:     rBuf.Cat("BLOB"); break;
				case S_CLOB:     rBuf.Cat("BLOB"); break;
				default:         rBuf.Cat("UNSUPPORTED_TYPE"); break;
			}
			break;
		default:
			switch(_t) {
				case S_CHAR:
				case S_LSTRING:
				case S_ZSTRING: rBuf.Cat("CHAR").CatParStr(_s); break;
				case S_INT:
				case S_UINT:
				case S_AUTOINC: rBuf.Cat("NUMERIC").CatParStr(12); break;
				case S_INT64:
				case S_UINT64:  rBuf.Cat("NUMERIC").CatParStr(19); break; 
				case S_FLOAT:   rBuf.Cat("NUMERIC").CatChar('(').Cat(38).Comma().Cat(12).CatChar(')'); break;
				case S_DATE:    rBuf.Cat("DATE"); break;
				case S_TIME:
				case S_DATETIME: rBuf.Cat("TIMESTAMP"); break;
				case S_DEC:
				case S_MONEY:
					_s = GETSSIZED(typ);
					rBuf.Cat("NUMERIC").CatChar('(').Cat(_s * 2 - 2).Comma().Cat(GETSPRECD(typ)).CatChar(')'); break;
				case S_BOOL:
				case S_BIT:   rBuf.Cat("NUMERIC").CatParStr(1); break;
				case S_NOTE:  rBuf.Cat("VARCHAR").CatParStr(_s); break;
				case S_WCHAR: rBuf.Cat("NCHAR").CatParStr(_s/2); break;
				case S_BLOB:  rBuf.Cat("BLOB"); break;
				case S_CLOB:  rBuf.Cat("CLOB"); break;
				default:      rBuf.Cat("UNSUPPORTED_TYPE"); break;
			}
			break;
	}
	return rBuf;
}

int Generator_SQL::CreateTable(const DBTable & rTbl, const char * pFileName, bool ifNotExists, int indent)
{
	const char * p_name = NZOR(pFileName, rTbl.GetName());
	SString type_name;
	Tok(tokCreate).Sp();
	Tok(tokTable).Sp();
	if(ifNotExists) {
		Tok(tokIfNotExists).Sp();		
	}
	Buf.Cat(p_name);
	Sp().LPar();
	if(indent)
		Cr();
	const  uint c = rTbl.GetFields().getCount();
	for(uint i = 0; i < c; i++) {
		const BNField & r_fld = rTbl.GetFields()[i];
		if(indent)
			Tab();
		Buf.Cat(r_fld.Name).Space().Cat(GetType(r_fld.T, type_name));
		if(i < (c-1))
			Com();
		if(indent)
			Cr();
	}
	RPar();
	Typ = typCreateTable; // @v12.3.12
	return 1;
}

Generator_SQL & Generator_SQL::Eos()
{
	Buf.Semicol().CR();
	return *this;
}

int Generator_SQL::CreateIndex(const DBTable & rTbl, const char * pFileName, uint n)
{
	int    ok = 1;
	if(n < rTbl.GetIndices().getNumKeys()) {
		SString temp_buf;
		const  BNKey  key = rTbl.GetIndices().getKey(n);
		int    fl = key.getFlags();
		const char * p_name = NZOR(pFileName, rTbl.GetName());
		Tok(tokCreate).Sp();
		if(!(fl & XIF_DUP))
			Tok(tokUnique).Sp();
		Tok(tokIndex).Sp();
		{
			SString & r_prefix = SLS.AcquireRvlStr();
			PrefixName(temp_buf.Z().Cat(p_name).Cat("Key").Cat(n), pfxIndex, r_prefix, 0);
			Buf.Cat(r_prefix).Space();
		}
		Tok(tokOn).Sp();
		Buf.Cat(p_name);
		Sp().LPar();
		int    ns = key.getNumSeg();
		for(int i = 0; i < ns; i++) {
			if(Sqlst == sqlstORA && key.getFlags(i) & XIF_ACS) {
				//
				// Для ORACLE нечувствительность к регистру символов
				// реализуется функциональным сегментом индекса nls_lower(fld)
				//
				Buf.Cat("nls_lower(").Cat(rTbl.GetIndices().field(n, i).Name).CatChar(')');
			}
			else
				Buf.Cat(rTbl.GetIndices().field(n, i).Name);
			if(key.getFlags(i) & XIF_DESC) {
				Buf.Space();
				Tok(tokDesc);
			}
			if(i < (ns-1))
				Buf.Comma().Space();
		}
		RPar();
		//Buf.Semicol();
		Typ = typCreateIndex;
	}
	else
		ok = 0;
	return ok;
}

int Generator_SQL::GetIndexName(const DBTable & rTbl, uint n, SString & rBuf)
{
	int    ok = 1;
	if(n < rTbl.GetIndices().getNumKeys()) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf.Cat(rTbl.GetName()).Cat("Key").Cat(n);
		PrefixName(r_temp_buf, pfxIndex, rBuf, 0);
	}
	else
		ok = 0;
	return ok;
}

int Generator_SQL::CreateSequenceOnField(const DBTable & rTbl, const char * pFileName, uint fldN, long newVal)
{
	int    ok = 1;
	if(fldN < rTbl.GetFields().getCount()) {
		SString temp_buf;
		const BNField & r_fld = rTbl.GetFields()[fldN];
		const char * p_name = NZOR(pFileName, rTbl.GetName());
		Tok(tokCreate).Sp().Tok(tokSequence).Sp();
		{
			SString & r_prefix = SLS.AcquireRvlStr();
			PrefixName(temp_buf.Z().Cat(p_name).CatChar('_').Cat(r_fld.Name), pfxSequence, r_prefix, 0);
			Buf.Cat(r_prefix);
		}
		if(newVal) {
			temp_buf.Z().Cat(newVal);
			Sp().Text("START").Sp().Text("WITH").Sp().Text(temp_buf);
		}
		//Buf.Semicol();
	}
	else
		ok = 0;
	return ok;
}

int Generator_SQL::GetSequenceNameOnField(const DBTable & rTbl, uint fldN, SString & rBuf)
{
	int    ok = 1;
	rBuf.Z();
	if(fldN < rTbl.GetFields().getCount()) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		r_temp_buf.Cat(rTbl.GetName()).CatChar('_').Cat(rTbl.GetFields()[fldN].Name);
		PrefixName(r_temp_buf, pfxSequence, rBuf, 0);
	}
	else
		ok = 0;
	return ok;
}

/*static*/const char * FASTCALL Generator_SQL::GetToken(uint tok)
{
	return (tok < tokCountOfTokens) ? P_Tokens[tok] : 0;
}

SString & FASTCALL Generator_SQL::PrefixName(const char * pName, int prefix, SString & rBuf, int cat)
{
	const char * p_pfx = 0;
	if(prefix == pfxIndex)
		p_pfx = "idx";
	else if(prefix == pfxSequence)
		p_pfx = "seq";
	else if(prefix == pfxConstr)
		p_pfx = "csr";
	else
		p_pfx = "";
	if(cat)
		rBuf.Cat(p_pfx).Cat(pName);
	else
		(rBuf = p_pfx).Cat(pName);
	return rBuf;
}

const char * Generator_SQL::P_Tokens[] = {
	"CREATE",
	"ALTER",
	"DROP",
	"DATABASE",
	"TABLE",
	"INDEX",
	"CONSTRAINT",
	"SEQUENCE",
	"INSERT",
	"UPDATE",
	"DELETE",
	"SELECT",
	"INTO",
	"WHERE",
	"FROM",
	"UNIQUE",
	"ON",
	"AND",
	"OR",
	"DESC",
	"VALUES",
	"INDEX_ASC",
	"INDEX_DESC",
	"FOR",
	"ROWID",
	"SET",
	"RETURNING",
	"MAX",
	"NLS_LOWER",
	"LOWER",
	"IF NOT EXISTS", // @v11.9.12
	"BEGIN",         // @v12.3.12
	"COMMIT",        // @v12.3.12 
	"ROLLBACK",      // @v12.3.12
	"TRANSACTION",   // @v12.3.12
	"INDEXED BY",    // @v12.4.0 SQLITE  
	"ORDER BY",      // @v12.4.0
};

Generator_SQL & Generator_SQL::HintBegin()
{
	Buf.Cat("/*+");
	return *this;
}

Generator_SQL & Generator_SQL::HintEnd()
{
	Buf.Cat("*/");
	return *this;
}

Generator_SQL & Generator_SQL::HintIndex(const DBTable & rTbl, const char * pAlias, uint idxN, int desc)
{
	SString idx_name;
	if(GetIndexName(rTbl, idxN, idx_name))
		Tok(desc ? tokHintIndexDesc : tokHintIndexAsc).LPar().Text(pAlias ? pAlias : rTbl.GetName()).Sp().Text(idx_name).RPar();
	return *this;

}

Generator_SQL & Generator_SQL::Func(int tokFn, const char * pArg)
{
	Tok(tokFn).LPar();
	if(pArg)
		Text(pArg);
	RPar();
	return *this;
}
