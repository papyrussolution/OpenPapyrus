// V_GGRP.CPP
// Copyright (c) A.Sobolev 2006, 2007
// @construction
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop

struct GoodsGroupFilt {
	enum {
		fNormal        = 0x0001, // Показывать обыкновенные группы
		fAltern        = 0x0002, // Показывать альтернативные группы
		fExtracFilters = 0x0004  // Извлекать вместе с альтернативными группами их фильтры
	};
	enum {
		fxAltExcl  = 0x0001, // Показывать только эксклюзивные альтернативные группы
		fxAltDyn   = 0x0002, // Показывать только динамические альтернативные группы
		fxAltTemp  = 0x0004, // Показывать только временные альтернативные группы
		fxFiltered = 0x0008, // Показывать только альтернативные группы, содержащие фильтр
	};
	enum {
		sName = 0x0001,
		sAbbr = 0x0002
	};
	PPID   ParentID;
	long   Flags;
};

class PPViewGoodsGroup : public PPView {
public:
	SLAPI  PPViewGoodsGroup();
	virtual SLAPI ~PPViewGoodsGroup();
	virtual int   SLAPI Browse(int modeless);
	virtual int   SLAPI ProcessCommand(uint ppvCmd, const void *, PPViewBrowser *);
	int    SLAPI Init(const GoodsGroupFilt * pFilt);
private:
	PPObjGoodsGroup GGObj;
	GoodsGroupFilt Filt;
	PPIDArray IdList;
	//StringSet
};

SLAPI PPViewGoodsGroup::PPViewGoodsGroup() : PPView(&GGObj)
{
}

SLAPI PPViewGoodsGroup::~PPViewGoodsGroup()
{
}

int SLAPI PPViewGoodsGroup::Init(const GoodsGroupFilt * pFilt)
{
	int    ok = 1;
	Filt = *pFilt;

	return ok;
}

