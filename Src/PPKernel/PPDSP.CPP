// PPDSP.CPP
// Copyright (c) A.Sobolev 2001, 2002, 2005, 2007, 2009, 2010, 2011, 2015, 2016
//
#pragma hdrstop
#ifdef PPDSC
	#define LOCAL_PPERRCODE
#endif
#include <pp.h>
#include <ppds.h>

SLAPI PPDeclStrucProcessor::PPDeclStrucProcessor()
{
	P_NameTab = 0;
	InStream = 0;
	P_FileName = 0;
}

SLAPI PPDeclStrucProcessor::~PPDeclStrucProcessor()
{
	delete P_FileName;
	delete P_NameTab;
	SFile::ZClose(&InStream);
}

#ifndef PPDSC //

int SLAPI PPDeclStrucProcessor::MakeErrMsgString(char * pBuf, size_t bufLen)
{
	SString file_msg, msg;
	PPGetMessage(mfError|mfOK, PPErrCode, 0, 1, msg);
	if(P_FileName) {
		file_msg.Cat("File").Space().Cat(P_FileName).CatDiv(',', 2).
			Cat("Line").Space().Cat(Line).CatDiv(':', 2).Cat(msg);
		msg = file_msg;
	}
	msg.CopyTo(pBuf, bufLen);
	return 1;
}

#else // PPDSC

int SLAPI PPDeclStrucProcessor::MakeErrMsgString(char * pBuf, size_t bufLen)
{
	const size_t msg_buf_len = 1024;
	char * p_msg = new char[msg_buf_len];
	char * p = p_msg;
	if(P_FileName) {
		sprintf(p, "File %s, Line %ld: ", P_FileName, Line);
		p += strlen(p);
	}
	sprintf(p, "error %d", PPErrCode);
	strnzcpy(pBuf, p_msg, bufLen);
	delete p_msg;
	return 1;
}

#endif

int SLAPI PPDeclStrucProcessor::SavePos(P * p)
{
	p->Pos = ftell(InStream);
	p->Lin = Line;
	return 1;
}

int SLAPI PPDeclStrucProcessor::RestorePos(const P * p)
{
	fseek(InStream, p->Pos, SEEK_SET);
	Line = p->Lin;
	return 1;
}

int SLAPI PPDeclStrucProcessor::GetChar(int skipWS)
{
	int    retry = 0;
	SavePos(&ChrP);
	int    c = fgetc(InStream);
	do {
		retry = 0;
		if(c == '\n')
			Line++;
		if(skipWS) {
			while(c == ' ' || c == '\t' || c == '\n') {
				c = fgetc(InStream);
				if(c == '\n')
					Line++;
			}
			if(c == '/')
				if((c = fgetc(InStream)) == '/') {
					do {
						c = fgetc(InStream);
					} while(c != '\n' && c != EOF);
					retry = 1;
				}
				else {
					ungetc(c, InStream);
					c = '/';
				}
		}
	} while(retry);
	return (c == EOF) ? 0 : c;
}

int SLAPI PPDeclStrucProcessor::UngetChar()
{
	return RestorePos(&ChrP);
}

int SLAPI PPDeclStrucProcessor::GetTerm(int * pTerm)
{
	int    term = ppdsUnknown;
	int    c = GetChar(1);
	switch(c) {
		case '=': term = ppdsEqual;        break;
		case '{': term = ppdsLeftBrace;    break;
		case '}': term = ppdsRightBrace;   break;
		case '[': term = ppdsLeftBracket;  break;
		case ']': term = ppdsRightBracket; break;
		case '*': term = ppdsAsterisk;     break;
		case ':': term = ppdsColon;        break;
		case 0:   term = 0;                break;
		default:
			{
				P p;
				UngetChar();
				SavePos(&p);
				if(GetWord())
					if(WordBuf.CmpNC("data") == 0)
						term = ppdsData;
					else if(WordBuf.CmpNC("declare") == 0)
						term = ppdsDeclare;
					else
						RestorePos(&p);
				else
					term = 0;
			}
			break;
	}
	ASSIGN_PTR(pTerm, term);
	return term;
}

int SLAPI PPDeclStrucProcessor::GetWord()
{
	int    c = GetChar(1);
	WordBuf = 0;
	if(isalpha(c) || c == '_')
		do {
			WordBuf.CatChar(c);
			c = GetChar();
		} while(isalnum(c) || c == '_');
	UngetChar();
	return c ? 1 : 0;
}

int SLAPI PPDeclStrucProcessor::GetValue(SString & rBuf)
{
	rBuf = 0;
	for(int c = GetChar(1); c != '\n' && c != ';';) {
		if(c == '\\') {
			c = GetChar(0);
			if(!oneof2(c, ';', '\\')) {
				UngetChar();
				c = '\\';
			}
		}
		rBuf.CatChar(c);
		c = GetChar();
		if(c == '/') {
			if((c = GetChar()) == '/')
				do {
					c = GetChar();
				} while(c != '\n' && c != 0);
			else {
				UngetChar();
				c = '/';
			}
		}
	}
	return 1;
}

#ifndef PPDSC // {

PPDeclStruc * SLAPI PPDeclStrucProcessor::GetData(long typeID, void * extraPtr, PPDeclStruc * pOuter, int dontAccept)
{
	uint   j;
	PPDeclStruc * p_decl = 0, * p_inner_decl = 0;
	PPDeclStrucItem item;
	SString value;
	int    term;

	THROW(p_decl = PPDeclStruc::CreateInstance(typeID, extraPtr, pOuter, &Logger));
	THROW(p_decl->InitData(PPDeclStruc::idoAlloc, 0, 0));
	while(GetTerm(&term) != ppdsRightBrace && term != 0) {
		int    private_field = 0;
		THROW_PP(GetWord(), PPERR_DS_UNEXPEOF);
		if(WordBuf.CmpNC("UpdateProtocol") == 0) {
			private_field = 1;
		}
		else if(p_decl->SearchItem(WordBuf, &item) <= 0) {
			p_decl->SetItemAddedMsgString(WordBuf);
			CALLEXCEPT();
		}
		THROW_PP(GetTerm() == ppdsEqual, PPERR_DS_EXPEQ);
		if(private_field) {
			THROW(GetValue(value));
			if(private_field == 1) {
				if(value.CmpNC("none") == 0)
					p_decl->UpdateProtocol = PPDeclStruc::updNone;
				else if(value.CmpNC("force") == 0)
					p_decl->UpdateProtocol = PPDeclStruc::updForce;
				else {
					long u = value.ToLong();
					if(u == 1)
						p_decl->UpdateProtocol = PPDeclStruc::updForce;
				}
			}
		}
		else if(item.Flags & DSIF_LIST) {
			THROW_PP(GetTerm() == ppdsLeftBrace, PPERR_DS_EXPLEFTBRACE);
			if(item.Type >= FIRST_DECLSTRUC_ID) {
				while(GetTerm() == ppdsLeftBrace) {
					THROW(p_inner_decl = GetData(item.Type, extraPtr, p_decl, dontAccept)); // @recursion
					THROW(p_decl->AcceptListItem(item.ID, p_inner_decl, &Ctx));
					ZDELETE(p_inner_decl);
				}
			}
			else {
				for(j = 0; GetTerm(&term) != ppdsRightBrace;) {
					THROW(GetValue(value));
					THROW(p_decl->TransferField(item.ID, PPDeclStruc::tfdBufToData, &j, value));
				}
			}
			p_decl->AcceptedFields.addUnique(item.ID);
		}
		else {
			if(item.Type >= FIRST_DECLSTRUC_ID) {
				THROW_PP(GetTerm() == ppdsLeftBrace, PPERR_DS_EXPLEFTBRACE);
				THROW(p_inner_decl = GetData(item.Type, extraPtr, p_decl, dontAccept)); // @recursion
				THROW(p_decl->AcceptListItem(item.ID, p_inner_decl, &Ctx));
				ZDELETE(p_inner_decl);
			}
			else {
				THROW(GetValue(value));
				THROW(p_decl->TransferField(item.ID, PPDeclStruc::tfdBufToData, &(j = 0), value));
			}
			p_decl->AcceptedFields.addUnique(item.ID);
		}
	}
	THROW_PP(term, PPERR_DS_UNEXPEOF);
	if(!dontAccept)
		THROW(p_decl->InitData(PPDeclStruc::idoAccept, 0, 0));
	CATCH
		ZDELETE(p_decl);
	ENDCATCH
	delete p_inner_decl;
	return p_decl;
}

int SLAPI PPDeclStrucProcessor::InitDataParsing(const char * pFileName)
{
	Line = 1;
	SFile::ZClose(&InStream);
	ZDELETE(P_FileName);
	int    ok = 1;
	InStream = fopen(pFileName, onecstr('r'));
	P_FileName = newStr(pFileName);
	PPSetAddedMsgString(pFileName);
	THROW_PP(InStream, PPERR_CANTOPENFILE);
	THROW(LoadNameTabFromResource());
	CATCHZOK
	return ok;
}

int SLAPI PPDeclStrucProcessor::ReleaseDataParsing()
{
	SFile::ZClose(&InStream);
	ZDELETE(P_NameTab);
	return 1;
}

int SLAPI PPDeclStrucProcessor::NextDecl(PPDeclStruc ** ppDecl, int dontAccept)
{
	int    ok = -1, term;
	SString fmt_buf, msg_buf;
	PPDeclStruc * p_decl = 0;
	PPLoadText(PPTXT_ACCEPTINGCHYOBJ, fmt_buf);
	while(GetTerm(&term) != ppdsData && term != 0)
		;
	if(term == ppdsData) {
		long   type_id = 0;
		THROW_PP(GetWord(), PPERR_DS_UNEXPEOF);
		THROW(GetTypeByName(WordBuf, &type_id) > 0);
		PPWaitMsg(msg_buf.Printf(fmt_buf, (const char *)WordBuf));
		THROW_PP(GetTerm() == ppdsLeftBrace, PPERR_DS_EXPLEFTBRACE);
		THROW(p_decl = GetData(type_id, 0/*extraData*/, 0, dontAccept));
		ok = 1;
	}
	CATCH
		ok = 0;
		ZDELETE(p_decl);
	ENDCATCH
	*ppDecl = p_decl;
	return ok;
}

int SLAPI PPDeclStruc::LoadFromResource(long declID)
{
	int    ok = 1;
	TVRez * p_rez = P_SlRez;
	if(p_rez) {
		char   name_buf[64];
		uint16 i, items_count = 0;
		long   p = 0L;
		p_rez->findResource((uint)declID, PP_RCDECLSTRUC, &p, 0);
		THROW_PP(p, PPERR_RESFAULT);
		freeAll();
		p_rez->getString(name_buf);
		THROW(SetHeader(declID, name_buf, 0));
		items_count = p_rez->getUINT();
		for(i = 0; i < items_count; i++) {
			PPDeclStrucItem item;
			memset(&item, 0, sizeof(item));
			item.ID    = p_rez->getUINT();
			p_rez->getString(name_buf);
			STRNSCPY(item.Name, name_buf);
			item.Flags = p_rez->getUINT();
			item.Type  = p_rez->getUINT();
			THROW(AddItem(&item));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDeclStrucProcessor::LoadNameTabFromResource()
{
	uint   res_id = 0;
	ulong  res_pos = 0;
	TVRez * p_rez = P_SlRez;
	if(p_rez)
		while(p_rez->enumResources(PP_RCDECLSTRUC, &res_id, &res_pos) > 0) {
			char name_buf[64];
			if(p_rez->findResource(res_id, PP_RCDECLSTRUC)) {
				p_rez->getString(name_buf);
				if(!AddTypeName(res_id, name_buf))
					return 0;
			}
		}
	return 1;
}

#endif // } PPDSC

int SLAPI PPDeclStrucProcessor::AddTypeName(long typeID, const char * pName)
{
	int    ok = 1;
	if(P_NameTab == 0)
		THROW_MEM(P_NameTab = new SArray(sizeof(NameTabItem)));
	if(GetTypeByName(pName, 0) > 0)
		ok = -1;
	else {
		NameTabItem tab_item;
		tab_item.ID = typeID;
		STRNSCPY(tab_item.Name, pName);
		THROW_SL(P_NameTab->insert(&tab_item));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDeclStrucProcessor::GetNewTypeID(long * pTypeID)
{
	long   max_id = FIRST_DECLSTRUC_ID-1;
	NameTabItem * p_item;
	if(P_NameTab)
		for(uint i = 0; P_NameTab->enumItems(&i, (void**)&p_item);)
			if(p_item->ID > max_id)
				max_id = p_item->ID;
	ASSIGN_PTR(pTypeID, max_id+1);
	return 1;
}

int SLAPI PPDeclStrucProcessor::GetTypeByName(const char * pName, long * pTypeID)
{
	NameTabItem * p_item;
	PPSetAddedMsgString(pName);
	if(P_NameTab)
		for(uint i = 0; P_NameTab->enumItems(&i, (void**)&p_item);)
			if(stricmp(p_item->Name, pName) == 0) {
				ASSIGN_PTR(pTypeID, p_item->ID);
				PPErrCode = PPERR_DS_TYPENAMEEXISTS;
				return 1;
			}
	ASSIGN_PTR(pTypeID, 0);
	return (PPErrCode = PPERR_DS_TYPENAMENFOUND, -1);
}

int SLAPI PPDeclStrucProcessor::GetNameByType(long typeID, char * pBuf, size_t bufLen)
{
	NameTabItem * p_item;
	if(P_NameTab)
		for(uint i = 0; P_NameTab->enumItems(&i, (void**)&p_item);)
			if(p_item->ID == typeID) {
				strnzcpy(pBuf, p_item->Name, bufLen);
				return 1;
			}
	ASSIGN_PTR(pBuf, 0);
	char   msg[48];
	PPSetAddedMsgString(ltoa(typeID, msg, 10));
	return (PPErrCode = PPERR_DS_STRUCTYPENFOUND, -1);
}

