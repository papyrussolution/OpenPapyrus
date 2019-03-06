// XTAGPARS.CPP
// Copyright (c) A.Starodub 2002, 2015, 2016
//
#include <slib.h>

SLAPI XTagParser::XTagParser()
{
	FileName[0] = 0;
	P_Stream = 0;
}

SLAPI XTagParser::~XTagParser()
{
	SFile::ZClose(&P_Stream);
}

int SLAPI XTagParser::GetToken(const char * pCurTag, char * pTagBuf, size_t bufLen)
{
	int tok = tokErr;
	if(P_Stream) {
		char c;
		long i = 0;
		c = fgetc(P_Stream);
		if(c == '<') {
			while((c = fgetc(P_Stream)) != EOF && c != '>') {
				if(!bufLen || i < (long)bufLen) {
					pTagBuf[i] = c;
				}
				i++;
			}
			if(!bufLen || i < (long)bufLen) {
				pTagBuf[i] = '\0';
			}
			if(c == '>') {
				if(pTagBuf[0] == '/') {
					char *p_c = pTagBuf;
					p_c++;
					if(!pCurTag || strcmp(pCurTag, p_c)) {
						i = 0;
						tok = tokErr;
					}
					else
						tok = tokEndTag;
				}
				else if(pCurTag && !strcmp(pCurTag, pTagBuf))
					tok = tokErr;
				else
					tok = tokTag;
			}
			else {
				i = 0;
				tok = tokErr;
			}
		}
		else if(c != EOF) {
			pTagBuf[0] = c;
			i = 1;
			tok = tokChar;
		}
		else
			tok = tokEOF;
		if(!bufLen || i < (long) bufLen) {
			pTagBuf[i] = '\0';
		}
	}
	return (tok == tokErr) ? (tok, SLibError = SLERR_INVFORMAT) : tok;
}

int SLAPI XTagParser::Run(const char * pFileName)
{
	int tok = tokErr;
	char tag_buf[64];
	if(pFileName && (P_Stream = fopen(pFileName, "r")) != NULL) {
		STRNSCPY(FileName, pFileName);
		while((tok = GetToken(0, tag_buf, sizeof(tag_buf))) != tokEOF && tok != tokErr) {
			if(tok == tokTag) {
				if(ProcessTag(tag_buf, 0) == tokErr) {
					tok = tokErr;
					break;
				}
			}
		}
	}
	else
		SLibError = SLERR_OPENFAULT;
	if(P_Stream) {
		fclose(P_Stream);
		P_Stream = 0;
	}
	FileName[0] = 0;
	return (tok == tokErr) ? 0 : 1;
}
