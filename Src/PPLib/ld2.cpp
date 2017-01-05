// LD2.CPP
// Copyright (c) A.Sobolev 1999-2001, 2002, 2003, 2004, 2005, 2006, 2007, 2016
//
#include <pp.h>
#pragma hdrstop
//
// Implementation of PPALDD_CashBook
//
PPALDD_CONSTRUCTOR(CashBook)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(CashBook)
{
	Destroy();
}

int PPALDD_CashBook::InitData(PPFilt & rFilt, long rsrv)
{
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_CashBook::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	return -1;
}

int PPALDD_CashBook::NextIteration(PPIterID iterId, long rsrv)
{
	IterProlog(iterId, 0);
	return DlRtm::NextIteration(iterId, rsrv);
}
//
// Implementation of PPALDD_TransferBase
//
PPALDD_CONSTRUCTOR(TransferBase)
{
	if(Valid) {
		AssignHeadData(&H, sizeof(H));
		AssignIterData(1, &I, sizeof(I));
	}
}

PPALDD_DESTRUCTOR(TransferBase)
{
	Destroy();
}

int PPALDD_TransferBase::InitData(PPFilt & rFilt, long rsrv)
{
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_TransferBase::InitIteration(PPIterID iterId, int /*sortId*/, long /*rsrv*/)
{
	IterProlog(iterId, 1);
	return -1;
}

int PPALDD_TransferBase::NextIteration(PPIterID iterId, long rsrv)
{
	IterProlog(iterId, 0);
	return DlRtm::NextIteration(iterId, rsrv);
}
