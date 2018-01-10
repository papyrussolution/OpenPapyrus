// PPEDI.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop

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
	return P_Node ? IsText((const char *)P_Node->name, pName) : 0;
}

int SEancomXmlSegment::IsContent(const char * pText) const
{
	return P_Node ? IsText((const char *)P_Node->content, pText) : 0;
}

SString & SEancomXmlSegment::GetContent(SString & rBuf) const
{
	rBuf = P_Node ? (const char *)P_Node->content : 0;
	return rBuf;
}

int SEancomXmlSegment::GetMsgTypeName() const
{
	return P_Node ? GetMsgType((const char *)P_Node->name) : 0;
}

int SEancomXmlSegment::GetMsgTypeContent() const
{
	return P_Node ? GetMsgType((const char *)P_Node->content) : 0;
}

int SEancomXmlSegment::GetMsgType(const char * pText) const
{
	int    msg_type = 0;
	if(pText) {
		if(IsText(pText, "DESADV"))
			msg_type = PPEDIOP_DESADV;
		else if(IsText(pText, "ORDRSP"))
			msg_type = PPEDIOP_ORDERRSP;
		else if(IsText(pText, "ALCDES"))
			msg_type = PPEDIOP_ALCODESADV;
		else if(IsText(pText, "ORDERS"))
			msg_type = PPEDIOP_ORDER;
		else if(IsText(pText, "RECADV"))
			msg_type = PPEDIOP_RECADV;
		else if(IsText(pText, "APERAK"))
			msg_type = PPEDIOP_APERAK;
	}
	return msg_type;
}

int SEancomXmlSegment::GetInt(int & rVal) const
{
	int    ok = 0;
	if(P_Node && P_Node->content) {
		rVal = atoi((const char *)P_Node->content);
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
	if(P_Node && P_Node->content) {
		rVal = atof((const char *)P_Node->content);
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
	if(P_Node && P_Node->content) {
		rText.Set(P_Node->content);
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
	rR.Clear();
	if(Is("RFF")) {
		SEancomXmlSegment seg;
		while(GetNext(seg)) {
			if(seg.Is("S506")) {
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
	rD.Clear();
	if(Is("DTM")) { // Date/time/period
		SEancomXmlSegment seg;
		SString value;
		//
		// 102   = CCYYMMDD
		// 203   = CCYYMMDDHHMM
		int    format = 0;
		//
		while(GetNext(seg)) {
			if(seg.Is("S507")) {
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
	rM.Clear();
	if(Is("MOA")) {
		SEancomXmlSegment seg;
		SString value;
		while(GetNext(seg)) {
			if(seg.Is("S516")) {
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
	rN.Clear();
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
