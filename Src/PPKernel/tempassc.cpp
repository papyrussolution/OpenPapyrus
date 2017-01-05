// TEMPASSC.CPP
// Copyright (c) A.Sobolev 2004, 2009, 2015, 2016
// @codepage windows-1251
// @Kernel
//
#include <pp.h>
#pragma hdrstop

PP_CREATE_TEMP_FILE_PROC(CreateTempFile, TempAssoc);

//static
TempAssoc * SLAPI TempAssoc::CreateInstance()
{
	TempAssoc * p_assc = new TempAssoc;
	if(!p_assc)
		return (PPSetErrorNoMem(), (TempAssoc *)0);
	else
		return p_assc->IsValid() ? p_assc : 0;
}

SLAPI TempAssoc::TempAssoc()
{
	P_Tbl = CreateTempFile();
}

int SLAPI TempAssoc::IsValid() const
{
	return P_Tbl ? 1 : 0;
}

int SLAPI TempAssoc::Add(PPID prmrID, PPID scndID)
{
	if(P_Tbl) {
		P_Tbl->data.PrmrID = prmrID;
		P_Tbl->data.ScndID = scndID;
		return P_Tbl->insertRec() ? 1 : PPSetErrorDB();
	}
	return 0;
}

int SLAPI TempAssoc::EnumPrmr(PPID * pPrmrID)
{
	if(P_Tbl) {
		TempAssocTbl::Key0 k0;
		k0.PrmrID = *pPrmrID;
		k0.ScndID = MAXLONG;
		if(P_Tbl->search(0, &k0, spGt)) {
			*pPrmrID = P_Tbl->data.PrmrID;
			return 1;
		}
	}
	return -1;
}

int SLAPI TempAssoc::GetList(PPID prmrID, PPIDArray * pList)
{
	int    ok = 1;
	if(P_Tbl) {
		TempAssocTbl::Key0 k0;
		BExtQuery q(P_Tbl, 0, 128);
		q.select(P_Tbl->ScndID, 0L).where(P_Tbl->PrmrID == prmrID);
		k0.PrmrID = prmrID;
		k0.ScndID = -MAXLONG;
		for(q.initIteration(0, &k0, spGe); ok && q.nextIteration() > 0;)
			if(!pList->add(P_Tbl->data.ScndID))
				ok = 0;
	}
	else
		ok = 0;
	return ok;
}
