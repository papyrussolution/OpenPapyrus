// ACS_1.CPP
// Copyright (c) A.Sobolev 1997-2001, 2007, 2008, 2009, 2010, 2016, 2017, 2020, 2025
// @codepage UTF-8
// Поддержка кассовых аппаратов Електроника-92-Аквариус и ЭКР-4110
//
#include <pp.h>
#pragma hdrstop
//
//
//
CS_1::CS_1(PPID n) : PPAsyncCashSession(n), Valid(1), P_Entries(0), NumEntries(0), FilesPerSet(0)
{
}

CS_1::~CS_1()
{
	SAlloc::F(P_Entries);
}

int CS_1::GetFileSet(char *, uint filesPerSet)
{
	int    ok = 1;
	SString temp_buf;
	SString temp_path;
	SString in_path;
	PPObjCashNode cnobj;
	PPAsyncCashNode acn;
	FilesPerSet = filesPerSet;
	NumEntries = 0;
	ZFREE(P_Entries);
	PPGetPath(PPPATH_IN, in_path);
	THROW(cnobj.GetAsync(NodeID, &acn) > 0);
	THROW_PP(acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
	{
		SFsPath ps;
		StringSet ss(';', acn.ImpFiles);
		for(uint i = 0; ss.get(&i, temp_buf);) {
			if(temp_buf.NotEmptyS()) {
				uint   j = 0;
				StringSet ss2(',', temp_buf);
				++NumEntries;
				THROW_MEM(P_Entries = static_cast<InFiles *>(SAlloc::R(P_Entries, NumEntries * sizeof(InFiles))));
				for(uint k = 0; ss2.get(&k, temp_buf);) {
					THROW_PP(temp_buf.NotEmptyS(), PPERR_INVFILESET);
					ps.Split(temp_buf);
					if(ps.Flags & (SFsPath::fDrv | SFsPath::fUNC))
						temp_path = temp_buf;
					else
						(temp_path = in_path).SetLastSlash().Cat(temp_buf);
					SFsPath::ReplaceExt(temp_path, "DBF", 0);
					temp_path.CopyTo(P_Entries[i].fn[j], sizeof(P_Entries[i].fn[j]));
					++j;
				}
				THROW_PP(j <= FilesPerSet, PPERR_INVFILESET);
			}
		}
	}
	CATCH
		NumEntries = 0;
		ZFREE(P_Entries);
		ok = 0;
	ENDCATCH
	return ok;
}

DbfTable * CS_1::OpenDBFTable(uint num, uint fn)
{
	DbfTable * t = 0;
	THROW_INVARG(P_Entries && num >= 0 && num < NumEntries && fn >= 0 && fn < FilesPerSet);
	THROW_MEM(t = new DbfTable(P_Entries[num].fn[fn]));
	THROW_PP(t->isOpened(), PPERR_DBFOPFAULT);
	CATCH
		ZDELETE(t);
	ENDCATCH
	return t;
}

PP_CREATE_TEMP_FILE_PROC(TempBarcodeTbl, Barcode);

BarcodeTbl * CS_1::CreateTmpBarToID(int num, int fn, int fldGoodsID, int fldBarCode, int fldUnitPerPack)
{
	BarcodeTbl * p_tbl = 0;
	char   code[24];
	char * c = 0;
	DbfTable * dbft = 0;
	THROW(dbft = OpenDBFTable(num, fn));
	THROW(p_tbl = TempBarcodeTbl());
	if(dbft->top()) {
		BExtInsert bei(p_tbl);
		do {
			DbfRecord dbfr(dbft);
			THROW(dbft->getRec(&dbfr));
			p_tbl->clearDataBuf();
			dbfr.get(fldBarCode, code, sizeof(code));
			c = strip(code);
			if(*c && *c != '0') {
				if((c = sstrchr(code, '.')) != 0)
					*c = 0;
				STRNSCPY(p_tbl->data.Code, code);
				dbfr.get(fldGoodsID, p_tbl->data.GoodsID);
				dbfr.get(fldUnitPerPack, p_tbl->data.Qtty);
				// @problem : Dup value
				THROW_DB(bei.insert(&p_tbl->data));
			}
		} while(dbft->next());
		THROW_DB(bei.flash());
	}
	CATCH
		PPError();
		ZDELETE(p_tbl);
	ENDCATCH
	delete dbft;
	return p_tbl;
}
