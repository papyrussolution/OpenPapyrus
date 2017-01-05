// V_GRAPH.CPP
// Copyright (c) A.Starodub 2003
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
#include <graph.h>

SLAPI PPGraphParam::PPGraphParam()
{
	memset(Title, 0, sizeof(Title));
	Kind      = 0;
	Flags     = 0;
	NumRows   = 0;
	P_Rows    = 0;
	P_Abcissa = 0;
}

SLAPI PPGraphParam::~PPGraphParam()
{
	ZDELETE(P_Rows);
	ZDELETE(P_Abcissa);
}

void SLAPI PPGraphParam::AllocMem()
{
	ZDELETE(P_Rows);
	ZDELETE(P_Abcissa);
	P_Rows    = new SArray(sizeof(double) * NumRows, 1);
	P_Abcissa = new SArray(GETSSIZE(AbcissaType), 1);
}

int SLAPI PPGraphParam::copy(const PPGraphParam & aParam)
{
	char *p_buf = 0;
	STRNSCPY(Title, aParam.Title);
	Kind    = aParam.Kind;
	Flags   = aParam.Flags;
	NumRows = aParam.NumRows;
	for(uint i = 0; aParam.Names.enumItems(&i, (void**)&p_buf) > 0;)
		Names.insert(newStr(p_buf));
	AbcissaType = aParam.AbcissaType;
	AllocMem();
	P_Rows->copy(*aParam.P_Rows);
	P_Abcissa->copy(*aParam.P_Abcissa);
	return 1;
}
//
//
//
SLAPI PPGraph::PPGraph()
{
	P_Window = new PPGraphWindow();
}

SLAPI PPGraph::~PPGraph()
{
	/*ZDELETE(P_Window);*/
}

int SLAPI PPGraph::Init(const PPGraphParam *
#ifdef __WIN32__
	pParam
#endif
)
{
	int ok = -1;
#ifdef __WIN32__
	ok = P_Window->Init(pParam);
#endif
	return ok;
}

int SLAPI PPGraph::Browse()
{
	int ok = -1;
#ifdef __WIN32__
	P_Window->Browse();
	ok = 1;
#endif
	return ok;
}

int SLAPI PPGraph::Print()
{
	int ok = -1;
	return ok;
}
