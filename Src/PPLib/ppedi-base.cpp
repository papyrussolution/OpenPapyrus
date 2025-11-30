// PPEDI-BASE.CPP
// Copyright (c) A.Sobolev 2018, 2020, 2021, 2022, 2024, 2025
// @codepage UTF-8
// Attention! Этот исходный файл включается самостоятельно в специализированные модули обмена 
// с различными провайдерами. Поэтому он не должен зависеть от других функций и классов
// модуля pplib. Допускается только зависимость от библиотек slib и libxml.
//
#include <pp.h>
#pragma hdrstop

PPEdiMessageEntry::PPEdiMessageEntry()
{
	THISZERO();
}

PPEdiMessageList::PPEdiMessageList() : TSVector <PPEdiMessageEntry>(), LastId(0)
{
}
	
PPEdiMessageList & PPEdiMessageList::Z()
{
	SVector::clear();
	// LastId не обнуляем с целью обеспечения уникальности идентификаторов для разных сессий
	return *this;
}

int PPEdiMessageList::SearchId(int id, uint * pPos) const
{
	return lsearch(&id, pPos, CMPF_LONG);
}

int PPEdiMessageList::Add(const PPEdiMessageEntry & rEntry)
{
	PPEdiMessageEntry new_entry;
	new_entry = rEntry;
	LastId++;
	SETIFZ(new_entry.ID, LastId);
	return insert(&new_entry);
}

SEancomXmlSegment::REF::REF() : Type(0)
{
}

SEancomXmlSegment::REF & SEancomXmlSegment::REF::Z()
{
	Type = 0;
	Text.Z();
	return *this;
}

SEancomXmlSegment::DTM::DTM() : Type(0), Dtm(ZERODATETIME), Days(0)
{
}
		
SEancomXmlSegment::DTM & SEancomXmlSegment::DTM::Z()
{
	Type = 0;
	Dtm = ZERODATETIME;
	Days = 0;
	return *this;
}

SEancomXmlSegment::MOA::MOA() : Type(0), Value(0.0)
{
	CurrencySymb[0] = 0;
}
		
SEancomXmlSegment::MOA & SEancomXmlSegment::MOA::Z()
{
	Type = 0;
	CurrencySymb[0] = 0;
	Value = 0.0;
	return *this;
}

SEancomXmlSegment::NAD::NAD()
{
	Z();
}
		
SEancomXmlSegment::NAD & SEancomXmlSegment::NAD::Z()
{
	Type = 0;
	GLN[0] = 0;
	CountryCode[0] = 0;
	PostalCode[0] = 0;
	Name = 0;
	City = 0;
	Address = 0;
	return *this;
}

SEancomXmlSegment::SEancomXmlSegment(const xmlNode * pNode)
{
	Set(pNode);
}

int SEancomXmlSegment::operator !() const
{
	return (P_Node == 0);
}

int SEancomXmlSegment::GetNext(SEancomXmlSegment & rSeg)
{
	const xmlNode * p_cur = P_Cur;
	if(P_Cur)
		P_Cur = P_Cur->next;
	return rSeg.Set(p_cur);
}

int SEancomXmlSegment::Is(const char * pName) const
{
	return P_Node ? IsText(PTRCHRC_(P_Node->name), pName) : 0;
}

int SEancomXmlSegment::IsContent(const char * pText) const
{
	// @v11.2.8 return P_Node ? IsText(PTRCHRC_(P_Node->content), pText) : 0;
	return P_Cur ? IsText(PTRCHRC_(P_Cur->content), pText) : 0; // @v11.2.8
}

SString & SEancomXmlSegment::GetContent(SString & rBuf) const
{
	// @v11.2.8 rBuf = P_Node ? PTRCHRC_(P_Node->content) : 0;
	rBuf = P_Cur ? PTRCHRC_(P_Cur->content) : 0; // @v11.2.8
	return rBuf;
}

int SEancomXmlSegment::GetMsgTypeName() const
{
	return P_Node ? GetMsgType(PTRCHRC_(P_Node->name)) : 0;
}

int SEancomXmlSegment::GetMsgTypeContent() const
{
	return P_Node ? GetMsgType(PTRCHRC_(P_Node->content)) : 0;
}

int SEancomXmlSegment::GetMsgType(const char * pText) const
{
	int    msg_type = PPEanComDocument::GetMsgTypeBySymb(pText);
	if(!msg_type && IsText(pText, "ALCDES"))
		msg_type = PPEDIOP_ALCODESADV;
	return msg_type;
}

int SEancomXmlSegment::GetInt(int & rVal) const
{
	int    ok = 0;
	if(P_Cur && P_Cur->content) {
		rVal = satoi(PTRCHRC_(P_Cur->content));
		ok = 1;
	}
	else {
		rVal = 0;
		ok = 0;
	}
	return ok;
}

int SEancomXmlSegment::GetReal(double & rVal) const
{
	int    ok = 0;
	if(P_Cur && P_Cur->content) {
		rVal = satof(PTRCHRC_(P_Cur->content));
		ok = 1;
	}
	else {
		rVal = 0.0;
		ok = 0;
	}
	return ok;
}

int SEancomXmlSegment::GetText(SString & rText) const
{
	int    ok = 0;
	if(P_Cur && P_Cur->content) {
		rText.Set(P_Cur->content);
		ok = 1;
	}
	else {
		rText.Z();
		ok = 0;
	}
	return ok;
}

int SEancomXmlSegment::GetREF(REF & rR)
{
	int    ok = 0;
	rR.Z();
	if(Is("RFF")) {
		SEancomXmlSegment seg;
		while(GetNext(seg)) {
			if(seg.Is("C506")) {
				SEancomXmlSegment seg2;
				while(seg.GetNext(seg2)) {
					if(seg2.Is("E1153")) { // Reference code qualifier
						if(seg2.IsContent("ON")) {
							rR.Type = refON;
						}
						else if(seg2.IsContent("IT")) {
							rR.Type = refIT;
						}
						else if(seg2.IsContent("YC1")) {
							rR.Type = refYC1;
						}
						else if(seg2.IsContent("ABT")) {
							rR.Type = refABT;
						}
						else if(seg2.IsContent("IV")) {
							rR.Type = refIV;
						}
					}
					else if(seg2.Is("E1154")) { // Reference identifier
						seg2.GetText(rR.Text);
					}
				}
			}
		}
		ok = 1;
	}
	return ok;
}

int SEancomXmlSegment::GetDate(DTM & rD)
{
	int    ok = 0;
	rD.Z();
	if(Is("DTM")) { // Date/time/period
		SEancomXmlSegment seg;
		SString value;
		//
		// 102   = CCYYMMDD
		// 203   = CCYYMMDDHHMM
		int    format = 0;
		//
		while(GetNext(seg)) {
			if(seg.Is("C507")) {
				SEancomXmlSegment seg2;
				while(seg.GetNext(seg2)) {
					if(seg2.Is("E2005")) { // Date or time or period function code qualifier
						seg2.GetInt(rD.Type);
					}
					else if(seg2.Is("E2380")) { // Date or time or period value
						seg2.GetText(value);
					}
					else if(seg2.Is("E2379")) { // Date or time or period format code
						seg2.GetInt(format);
					}
				}
				if(format == 102) {
					strtodate(value.Trim(8), DATF_YMD|DATF_CENTURY, &rD.Dtm.d);
					rD.Dtm.t = ZEROTIME;
					ok = 1;
				}
				else if(format == 203) {
					SString temp_buf;
					(temp_buf = value).ShiftLeft(8);
					int  _t = temp_buf.ToLong();
					int  hh = _t / 100;
					int  mm = _t % 100;
					rD.Dtm.t = encodetime(hh, mm, 0, 0);
					strtodate(value.Trim(8), DATF_YMD|DATF_CENTURY, &rD.Dtm.d);
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SEancomXmlSegment::GetMOA(MOA & rM)
{
	int    ok = 0;
	rM.Z();
	if(Is("MOA")) {
		SEancomXmlSegment seg;
		SString value;
		while(GetNext(seg)) {
			if(seg.Is("C516")) {
				SEancomXmlSegment seg2;
				while(seg.GetNext(seg2)) {
					if(seg2.Is("E5025")) { // Monetary amount type code qualifier
						seg2.GetInt(rM.Type);
					}
					else if(seg2.Is("E5004")) { // Value
						seg2.GetReal(rM.Value);
					}
					else if(seg2.Is("E6345")) { // Currency ISO 4217 three alpha
						SString temp_buf;
						seg2.GetText(temp_buf);
						STRNSCPY(rM.CurrencySymb, temp_buf);
					}
				}
				ok = 1;
			}
		}
	}
	return ok;
}

int SEancomXmlSegment::GetNAD(NAD & rN)
{
	int    ok = 0;
	rN.Z();
	if(Is("NAD")) {
		SEancomXmlSegment seg;
		SString temp_buf;
		while(GetNext(seg)) {
			if(seg.Is("E3035")) { // Party function code qualifier
				if(seg.IsContent("SU")) {
					rN.Type = nadSU;
				}
				else if(seg.IsContent("BY")) {
					rN.Type = nadBY;
				}
				else if(seg.IsContent("DP")) {
					rN.Type = nadDP;
				}
				else if(seg.IsContent("UD")) {
					rN.Type = nadUD;
				}
				else if(seg.IsContent("IV")) {
					rN.Type = nadIV;
				}
			}
			else if(seg.Is("C082")) { // PARTY IDENTIFICATION DETAILS
				SEancomXmlSegment seg2;
				while(seg.GetNext(seg2)) { //
					if(seg2.Is("E3039")) { // Party identifier GLN -Format n13
						seg2.GetText(temp_buf);
						STRNSCPY(rN.GLN, temp_buf);
					}
					else if(seg2.Is("E3055")) { // Code list responsible agency code 9 = EAN
					}
				}
				ok = 1;
			}
			else if(seg.Is("C080")) { 
			}
			else if(seg.Is("C059")) { 
			}
			else if(seg.Is("E3164")) { 
			}
			else if(seg.Is("E3251")) { 
			}
			else if(seg.Is("E3207")) { 
			}
		}
	}
	return ok;
}

int SEancomXmlSegment::IsText(const char * pContent, const char * pText) const
{
	return BIN(pContent && pText && _stricmp(pContent, pText) == 0);
}

int SEancomXmlSegment::Set(const xmlNode * pNode)
{
	P_Node = pNode;
	P_Cur = P_Node ? P_Node->children : 0;
	return BIN(P_Node);
}
