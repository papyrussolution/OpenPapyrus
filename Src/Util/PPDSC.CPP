// PPDSC.CPP
// Copyright (c) A.Sobolev 2001-2002, 2008, 2010, 2011, 2016, 2017
//
#pragma hdrstop
#define LOCAL_PPERRCODE
#include <pp.h>
#include <ppds.h>
//
//
//
int PPErrCode = 0;
static SString AddedMsgString;

int FASTCALL PPSetError(int err)
{
	PPErrCode = err;
	return 1;
}

int PPSetErrorNoMem() { return PPSetError(PPERR_NOMEM); }
int PPSetErrorSLib() { return PPSetError(PPERR_SLIB); }
int PPSetErrorInvParam() { return PPSetError(PPERR_INVPARAM); }

int FASTCALL PPSetError(int err, const char * pStr)
{
	PPErrCode = err;
	AddedMsgString = pStr;
	return 0;
}

void FASTCALL PPSetAddedMsgString(const char * pStr)
{
	AddedMsgString = pStr;
}

int SLAPI PPOutputMessage(char * pMsg, uint /*options*/)
{
	printf("%s\n", pMsg);
	return 1;
}

//static
PPDeclStruc * SLAPI PPDeclStruc::CreateInstance(long typeID, void * extraPtr, const PPDeclStruc * pOuter, PPLogger * pLogger)
{
	return 0;
}
//
//
//
int SLAPI PPDeclStrucProcessor::WriteDeclToFiles(FILE * pRcStream, FILE * pChStream, PPDeclStruc * pDecl)
{
	int    ok = 1;
	char   decl_symb[64], decl_define[64], fld_define[128];
	PPDeclStrucItem item;
	if(pDecl && pDecl->GetCount()) {
		pDecl->GetItem(0, &item);
		_strupr(STRNSCPY(decl_symb, item.Name));
		strcpy(stpcpy(decl_define, "PPDS_"), decl_symb);
		if(pChStream)
			fprintf(pChStream, "\n#define %s %ld\n", decl_define, item.ID);
		if(pRcStream) {
			fprintf(pRcStream, "%s PP_RCDECLSTRUC {\n\t\"%s\\0\", %u\n",
				decl_define, item.Name, pDecl->GetCount()-1);
			for(uint idx = 1; pDecl->EnumItems(&idx, &item) > 0;) {
				char type_name[48];
 				char * p = fld_define;
				p = stpcpy(p, "DSF_");
				p = stpcpy(p, decl_symb);
				*p++ = '_';
				_strupr(strcpy(p, item.Name));
				if(pChStream)
					fprintf(pChStream, "\t#define %s %ld\n", fld_define, item.ID);
				if(item.Type >= FIRST_DECLSTRUC_ID) {
					THROW(GetNameByType(item.Type, type_name, sizeof(type_name)) > 0);
					strcpy(stpcpy(decl_define, "PPDS_"), _strupr(type_name));
				}
				else {
					decl_define[0] = '0';
					decl_define[1] = 0;
				}
				fprintf(pRcStream, "\t%s, \"%s\\0\", 0x%lx, %s\n", fld_define, item.Name, item.Flags, decl_define);
			}
			fprintf(pRcStream, "};\n\n");
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

static int SLAPI PrintGenHeader(FILE * pStream, const char * pFileName, const char * pSrcFileName)
{
	SString dtm_buf;
	dtm_buf.Cat(getcurdatetime_(), DATF_DMY | DATF_CENTURY, TIMF_HMS);
	fprintf(pStream, "// %s\n// This file was automaticaly generated from %s [%s]\n// Don't modify\n\n",
		pFileName, pSrcFileName, (const char *)dtm_buf);
	return 1;
}

int SLAPI PPDeclStrucProcessor::ParseDefinition(const char * pFileName,
	const char * pOutRcFileName, const char * pOutChFileName)
{
	Line = 1;

	int    ok = 1, term = ppdsUnknown;
	FILE * out_rc_stream = 0;
	FILE * out_h_stream = 0;
	PPDeclStruc * p_decl = 0;

	if(InStream) {
		fclose(InStream);
		InStream = 0;
	}
	ZDELETE(P_FileName);
	PPSetAddedMsgString(pFileName);
	THROW_PP(InStream = fopen(pFileName, onecstr('r')), PPERR_CANTOPENFILE);
	P_FileName = newStr(pFileName);
	if(pOutRcFileName) {
		out_rc_stream = fopen(pOutRcFileName, onecstr('w'));
		PPSetAddedMsgString(pOutRcFileName);
		THROW_PP(out_rc_stream, PPERR_CANTOPENFILE);
		PrintGenHeader(out_rc_stream, pOutRcFileName, pFileName);
		if(pOutChFileName)
			fprintf(out_rc_stream, "#include \"%s\"\n\n", pOutChFileName);
	}
	if(pOutChFileName) {
		out_h_stream = fopen(pOutChFileName, onecstr('w'));
		PPSetAddedMsgString(pOutChFileName);
		THROW_PP(out_h_stream, PPERR_CANTOPENFILE);
		PrintGenHeader(out_h_stream, pOutChFileName, pFileName);
	}
	while(GetTerm(&term) == ppdsDeclare) {
		long fld_id = 1, decl_id = 0;
		PPDeclStruc decl;
		THROW_PP(GetWord(), PPERR_DS_UNEXPEOF);
		THROW(GetTypeByName(WordBuf, 0) < 0);
		THROW(GetNewTypeID(&decl_id));
		THROW(decl.SetHeader(decl_id, WordBuf, 0));
		THROW(AddTypeName(decl_id, WordBuf));
		THROW_PP(GetTerm() == ppdsLeftBrace, PPERR_DS_EXPLEFTBRACE);
		do {
			PPDeclStrucItem item;
			MEMSZERO(item);
			THROW_PP(GetWord(), PPERR_DS_UNEXPEOF);
			STRNSCPY(item.Name, WordBuf);
			item.ID = fld_id++;

			if(GetTerm(&term) == ppdsLeftBracket) {
				THROW_PP(GetTerm() == ppdsRightBracket, PPERR_DS_EXPRIGHTBRACKET);
				item.Flags |= DSIF_LIST;
				THROW(GetTerm(&term));
			}
			if(term == ppdsColon) {
				long type_id = 0;
				THROW_PP(GetWord(), PPERR_DS_UNEXPEOF);
				THROW(GetTypeByName(WordBuf, &type_id) > 0);
				item.Type = type_id;
				THROW(GetTerm(&term));
			}
			if(term == ppdsAsterisk) {
				item.Flags |= DSIF_REQUIRED;
				THROW(GetTerm(&term));
			}
			THROW_PP(term != 0, PPERR_DS_UNEXPEOF);
			THROW_PP(term == ppdsRightBrace || term == -1, PPERR_DS_EXPRBRACEORNAME);
			if(decl.SearchItem(item.Name, 0) > 0) {
				decl.SetItemAddedMsgString(item.Name);
				CALLEXCEPT();
			}
			THROW(decl.AddItem(&item));
		} while(term != ppdsRightBrace);
		THROW(WriteDeclToFiles(out_rc_stream, out_h_stream, &decl));
	}
	THROW_PP(term == 0, PPERR_DS_SYNTAX);
	CATCH
		ok = 0;
	ENDCATCH
	delete p_decl;
	if(InStream) {
		fclose(InStream);
		InStream = 0;
	}
	if(out_rc_stream)
		fclose(out_rc_stream);
	if(out_h_stream)
		fclose(out_h_stream);
	return ok;
}

int main(int argc, char * argv[])
{
	int    ok = 1;
	SString in_fname;
	SString h_fname;
	SString rc_fname;
	if(argc < 2) {
		printf("Papyrus DECLSTRUC compiler\nCopyright (c) A.Sobolev 2001\n");
		printf("Usage: ppdsc filename[.ds] [rc_filename[.rc]] [h_filename[.h]]\n");
		ok = 0;
	}
	else {
		in_fname = argv[1];
		SPathStruc::ReplaceExt(in_fname, "ds", 0);
		if(!fileExists(in_fname)) {
			printf("Error: input file %s not found\n", (const char *)in_fname);
			ok = 0;
		}
		else {
			if(argc >= 3) {
				rc_fname = argv[2];
				SPathStruc::ReplaceExt(rc_fname, "rc", 0);
			}
			else {
				rc_fname = in_fname;
				SPathStruc::ReplaceExt(rc_fname, "rc", 1);
			}
			if(argc >= 4) {
				h_fname = argv[3];
				SPathStruc::ReplaceExt(h_fname, "h", 0);
			}
			else {
				h_fname = in_fname;
				SPathStruc::ReplaceExt(h_fname, "h", 1);
			}
		}
	}
	if(ok) {
		PPDeclStrucProcessor dsp;
		if(!dsp.ParseDefinition(in_fname, rc_fname, h_fname)) {
			char   msg_buf[256];
			dsp.MakeErrMsgString(msg_buf, sizeof(msg_buf));
			PPOutputMessage(msg_buf, mfError | mfOK);
			SFile::Remove(rc_fname);
			SFile::Remove(h_fname);
			ok = 0;
		}
	}
	return ok ? 0 : -1;
}
