// SXML.H
// Copyright (c) A.Sobolev 2015, 2016, 2017
//
#ifndef __SXML_H
#define __SXML_H

#include <slib.h>
//#include <libxml.h>
#include <libxml\xmlwriter.h>
#include <libxml\xmlreader.h>
//
//
//
class SXml {
public:
	class WDoc {
	public:
		WDoc(xmlTextWriter * pWriter, SCodepage cp);
		~WDoc();
		operator xmlTextWriter * ()
		{
			return Lx;
		}
	private:
		enum {
			stStarted = 0x0001
		};
		long   State;
		xmlTextWriter * Lx;
	};
    class WNode {
	public:
		static SString & FASTCALL CDATA(SString & rBuf);

        WNode(xmlTextWriter * pWriter, const char * pName);
        WNode(xmlTextWriter * pWriter, const char * pName, const SString & rValue);
        WNode(xmlTextWriter * pWriter, const char * pName, const char * pValue);
        ~WNode();
        int    PutAttrib(const char * pName, const char * pValue);
        int    PutAttribSkipEmpty(const char * pName, const char * pValue);
        int    PutInner(const char * pInnerName, const char * pInnerValue);
        int    PutInnerSkipEmpty(const char * pInnerName, const char * pInnerValue);
        int    SetValue(const SString & rText);
	private:
		int    Construct(xmlTextWriter * pWriter, const char * pName);

		enum {
			stStarted = 0x0001
		};
		long   State;
		xmlTextWriter * Lx;
        SString Name;
    };

    static int FASTCALL IsName(const xmlNode * pNode, const char * pName);
    static int FASTCALL IsContent(const xmlNode * pNode, const char * pText);
    static int FASTCALL GetContent(const xmlNode * pNode, SString & rResult);
    //
    // Descr: Функция, совмещающая вызов
    // {
    //    if(SXml::IsName(pNode, pName))
    //       SXml::GetContent(pNode, rResult);
    // }
    // Returns:
    //    >0 - узел pNode имеет имя pName и не пустое содержание, которое помещено в rResult
    //    <0 - узел pNode имеет имя pName, но содержание пустое (rResult = 0)
    //    0  - узел pNode не имеет имя pName. В этом случае rResult не меняется.
    //
    static int SLAPI GetContentByName(const xmlNode * pNode, const char * pName, SString & rResult);
    static int SLAPI GetAttrib(const xmlNode * pNode, const char * pAttr, SString & rResult);
};

#endif // __SXML_H
