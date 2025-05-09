// SXMLAD.CPP
// Copyright (c) A.Sobolev, 2019, 2023, 2025
// @codepage UTF-8
// Вспомогательные механизмы для работы с XML (функции с дополнительными зависимостями)
// 
#include <slib-internal.h>
#pragma hdrstop
#include <sxml.h>
#include <libxml/xmlschemastypes.h>

int SXml::WNode::PutAttrib_Ns(const char * pNs, const char * pDomain, const char * pPath)
{
	assert(!isempty(pNs) && !isempty(pDomain) && !isempty(pPath));
	return PutAttrib(SXml::nst("xmlns", pNs), InetUrl::MkHttp(pDomain, pPath));
}

int SXml::WNode::PutAttrib(const char * pName, const char * pValue)
{
	int    ok = 1;
	if(Lx && State & stStarted) {
		xmlTextWriterStartAttribute(Lx, reinterpret_cast<const xmlChar *>(pName));
		xmlTextWriterWriteString(Lx, reinterpret_cast<const xmlChar *>(pValue));
		xmlTextWriterEndAttribute(Lx);
	}
	else
		ok = 0;
	return ok;
}
