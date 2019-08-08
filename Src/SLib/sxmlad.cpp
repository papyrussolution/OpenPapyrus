// SXMLAD.CPP
// Copyright (c) A.Sobolev, 2019
// Вспомогательные механизмы для работы с XML (функции с дополнительными зависимостями)
// 
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <snet.h>
#include <sxml.h>
#include <libxml/xmlschemastypes.h>

int SXml::WNode::PutAttrib_Ns(const char * pNs, const char * pDomain, const char * pPath)
{
	assert(!isempty(pNs) && !isempty(pDomain) && !isempty(pPath));
	return PutAttrib(SXml::nst("xmlns", pNs), InetUrl::MkHttp(pDomain, pPath));
}
