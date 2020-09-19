// TEXTBRW.CPP
// Copyright (c) A.Starodub 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
// STextBrowser
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <scintilla.h>
#include <scilexer.h>
//
//
//
static const SIntToSymbTabEntry SScLangEntryList[] = {
	{   SCLEX_NULL,         "normal"       },
	{   SCLEX_HTML,         "php"          },
	{   SCLEX_CPP,          "c"            },
	{   SCLEX_CPP,          "cpp"          },
	{   SCLEX_CPP,          "cs"           },
	{   SCLEX_CPP,          "objc"         },
	{   SCLEX_CPP,          "java"         },
	{   SCLEX_CPP,          "rc"           },
	{   SCLEX_HTML,         "html"         },
	{   SCLEX_XML,          "xml"          },
	{   SCLEX_MAKEFILE,     "makefile"     },
	{   SCLEX_PASCAL,       "pascal"       },
	{   SCLEX_BATCH,        "batch"        },
	{   SCLEX_PROPERTIES,   "ini"          },
	{   SCLEX_NULL,         "nfo"          },
	{   SCLEX_USER,         "udf"          },
	{   SCLEX_HTML,         "asp"          },
	{   SCLEX_SQL,          "sql"          },
	{   SCLEX_VB,           "vb"           },
	{   SCLEX_CPP,          "javascript"   },
	{   SCLEX_CSS,          "css"          },
	{   SCLEX_PERL,         "perl"         },
	{   SCLEX_PYTHON,       "python"       },
	{   SCLEX_LUA,          "lua"          },
	{   SCLEX_TEX,          "tex"          },
	{   SCLEX_FORTRAN,      "fortran"      },
	{   SCLEX_BASH,         "bash"         },
	{   SCLEX_CPP,          "actionscript" },
	{   SCLEX_NSIS,         "nsis"         },
	{   SCLEX_TCL,          "tcl"          },
	{   SCLEX_LISP,         "lisp"         },
	{   SCLEX_LISP,         "scheme"       },
	{   SCLEX_ASM,          "asm"          },
	{   SCLEX_DIFF,         "diff"         },
	{   SCLEX_PROPERTIES,   "props"        },
	{   SCLEX_PS,           "postscript"   },
	{   SCLEX_RUBY,         "ruby"         },
	{   SCLEX_SMALLTALK,    "smalltalk"    },
	{   SCLEX_VHDL,         "vhdl"         },
	{   SCLEX_KIX,          "kix"          },
	{   SCLEX_AU3,          "autoit"       },
	{   SCLEX_CAML,         "caml"         },
	{   SCLEX_ADA,          "ada"          },
	{   SCLEX_VERILOG,      "verilog"      },
	{   SCLEX_MATLAB,       "matlab"       },
	{   SCLEX_HASKELL,      "haskell"      },
	{   SCLEX_INNOSETUP,    "inno"         },
	{   SCLEX_SEARCHRESULT, "searchResult" },
	{   SCLEX_CMAKE,        "cmake"        },
	{   SCLEX_YAML,         "yaml"         },
	{   SCLEX_COBOL,        "cobol"        },
	{   SCLEX_GUI4CLI,      "gui4cli"      },
	{   SCLEX_D,            "d"            },
	{   SCLEX_POWERSHELL,   "powershell"   },
	{   SCLEX_R,            "r"            },
	{   SCLEX_HTML,         "jsp"          },
	{   SCLEX_COFFEESCRIPT, "coffeescript" },
	{   SCLEX_CPP,          "json"         },
	{   SCLEX_CPP,          "javascript.js"},
	{   SCLEX_F77,          "fortran77"    },
	{   SCLEX_BAAN,         "baanc"        },
	{   SCLEX_SREC,         "srec"         },
	{   SCLEX_IHEX,         "ihex"         },
	{   SCLEX_TEHEX,        "tehex"        },
	{   SCLEX_CPP,          "swift"        },
	{   SCLEX_ASN1,         "asn1"         },
	{   SCLEX_AVS,          "avs"          },
	{   SCLEX_BLITZBASIC,   "blitzbasic"   },
	{   SCLEX_PUREBASIC,    "purebasic"    },
	{   SCLEX_FREEBASIC,    "freebasic"    },
	{   SCLEX_CSOUND,       "csound"       },
	{   SCLEX_ERLANG,       "erlang"       },
	{   SCLEX_ESCRIPT,      "escript"      },
	{   SCLEX_FORTH,        "forth"        },
	{   SCLEX_LATEX,        "latex"        },
	{   SCLEX_MMIXAL,       "mmixal"       },
	{   SCLEX_NIMROD,       "nimrod"       },
	{   SCLEX_NNCRONTAB,    "nncrontab"    },
	{   SCLEX_OSCRIPT,      "oscript"      },
	{   SCLEX_REBOL,        "rebol"        },
	{   SCLEX_REGISTRY,     "registry"     },
	{   SCLEX_RUST,         "rust"         },
	{   SCLEX_SPICE,        "spice"        },
	{   SCLEX_TXT2TAGS,     "txt2tags"     },
	{   SCLEX_NULL,         "ext"          }
};

static int FASTCALL SScGetLexerIdByName(const char * pName)
{
    for(uint i = 0; i < SIZEOFARRAY(SScLangEntryList); i++) {
		const SIntToSymbTabEntry & r_entry = SScLangEntryList[i];
		if(sstreqi_ascii(r_entry.P_Symb, pName))
			return i+1;
    }
    return 0;
}

//static const char * FASTCALL SScGetLexerNameById(int id) { return (id > 0 && id <= SIZEOFARRAY(SScLangEntryList)) ? SScLangEntryList[id-1].P_Symb : 0; }
static int FASTCALL SScGetLexerModelById(int id) { return (id > 0 && id <= SIZEOFARRAY(SScLangEntryList)) ? SScLangEntryList[id-1].Id : 0; }

SLAPI SScEditorStyleSet::SScEditorStyleSet()
{
}

SLAPI SScEditorStyleSet::~SScEditorStyleSet()
{
}

void SScEditorStyleSet::Destroy()
{
	L.freeAll();
	ML.freeAll();
	KwL.freeAll();
	DestroyS();
}

void   FASTCALL SScEditorStyleSet::InnerToOuter(const InnerLangModel & rS, LangModel & rD) const
{
	rD.LexerId = rS.LexerId;
	GetS(rS.CommentLineP, rD.CommentLine);
	GetS(rS.CommentStartP, rD.CommentStart);
	GetS(rS.CommentEndP, rD.CommentEnd);
}

void   FASTCALL SScEditorStyleSet::InnerToOuter(const InnerLangModelKeywords & rS, LangModelKeywords & rD) const
{
	rD.LexerId = rS.LexerId;
	GetS(rS.KeywordClassP, rD.KeywordClass);
	GetS(rS.KeywordListP, rD.KeywordList);
}

void FASTCALL SScEditorStyleSet::InnerToOuter(const InnerStyle & rS, Style & rD) const
{
    rD.Group = rS.Group;
    rD.LexerId = rS.LexerId;
    rD.StyleId = rS.StyleId;
    rD.BgC = rS.BgC;
    rD.FgC = rS.FgC;
    rD.FontStyle = rS.FontStyle;
    rD.FontSize = rS.FontSize;
    GetS(rS.LexerDescrP, rD.LexerDescr);
    GetS(rS.StyleNameP, rD.StyleName);
    GetS(rS.FontFaceP, rD.FontFace);
    GetS(rS.KeywordClassP, rD.KeywordClass);
}

int SLAPI SScEditorStyleSet::GetModel(int lexerId, LangModel * pModel) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < ML.getCount(); i++) {
		const InnerLangModel & r_item = ML.at(i);
		if(r_item.LexerId == lexerId) {
			if(pModel) {
				InnerToOuter(r_item, *pModel);
			}
			ok = 1;
		}
	}
	return ok;
}

int SLAPI SScEditorStyleSet::GetModelKeywords(int lexerId, TSCollection <LangModelKeywords> * pList) const
{
	int    ok = 0;
	for(uint i = 0; i < KwL.getCount(); i++) {
		const InnerLangModelKeywords & r_item = KwL.at(i);
		if(r_item.LexerId == lexerId) {
			if(pList) {
				LangModelKeywords * p_new_entry = pList->CreateNewItem();
				THROW(p_new_entry);
				InnerToOuter(r_item, *p_new_entry);
			}
			ok++;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SScEditorStyleSet::GetStyles(int group, int lexerId, TSCollection <Style> * pList) const
{
	int    ok = 0;
	SString temp_buf;
    for(uint i = 0; i < L.getCount(); i++) {
        const InnerStyle & r_is = L.at(i);
        if(r_is.Group == group) {
			if(lexerId) {
				if(r_is.LexerId == lexerId) {
					if(pList) {
						Style * p_new_entry = pList->CreateNewItem();
						THROW_SL(p_new_entry);
						InnerToOuter(r_is, *p_new_entry);
					}
					ok++;
				}
			}
			else if(group == sgGlobal) {
				if(pList) {
					Style * p_new_entry = pList->CreateNewItem();
					THROW_SL(p_new_entry);
					InnerToOuter(r_is, *p_new_entry);
				}
				ok++;
			}
        }
    }
	CATCHZOK
    return ok;
}

int SLAPI SScEditorStyleSet::GetStyle(int group, int lexerId, int styleId, Style & rS) const
{
	int    ok = 0;
	SString temp_buf;
    for(uint i = 0; !ok && i < L.getCount(); i++) {
        const InnerStyle & r_is = L.at(i);
        if(r_is.Group == group && r_is.StyleId == styleId) {
			if(lexerId) {
				if(r_is.LexerId == lexerId) {
					InnerToOuter(r_is, rS);
					ok = 1;
				}
			}
			else if(group == sgGlobal) {
				InnerToOuter(r_is, rS);
				ok = 1;
			}
        }
    }
    return ok;
}

int SLAPI SScEditorStyleSet::ReadStyleAttributes(const xmlNode * pNode, InnerStyle & rS) 
{
	int    ok = 1;
	SString temp_buf;
	rS.FgC.Z();
	rS.BgC.Z();
	rS.FontStyle = -1;
	if(SXml::GetAttrib(pNode, "name", temp_buf) > 0)
		AddS(temp_buf, &rS.StyleNameP);
	if(SXml::GetAttrib(pNode, "styleID", temp_buf) > 0)
		rS.StyleId = (uint)temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "fgColor", temp_buf) > 0) {
		if(temp_buf.Len() == 6) {
			temp_buf.Insert(0, "#");
			rS.FgC.FromStr(temp_buf);
		}
	}
	if(SXml::GetAttrib(pNode, "bgColor", temp_buf) > 0) {
		if(temp_buf.Len() == 6) {
			temp_buf.Insert(0, "#");
			rS.BgC.FromStr(temp_buf);
		}
	}
	if(SXml::GetAttrib(pNode, "fontName", temp_buf) > 0)
		AddS(temp_buf, &rS.FontFaceP);
	if(SXml::GetAttrib(pNode, "fontStyle", temp_buf) > 0)
		rS.FontStyle = temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "fontSize", temp_buf) > 0)
		rS.FontSize = (uint)temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "keywordClass", temp_buf) > 0)
		AddS(temp_buf, &rS.KeywordClassP);
	return ok;
}

int SLAPI SScEditorStyleSet::ParseStylesXml(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString lexer_name;
	SString lexer_descr;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	THROW_SL(fileExists(pFileName));
	THROW(p_ctx = xmlNewParserCtxt());
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "EditorStyles")) {
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "LexerStyles")) {
				for(xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "LexerType")) {
						if(SXml::GetAttrib(p_s, "name", lexer_name) > 0) {
							uint   lexer_id = SScGetLexerIdByName(lexer_name);
							if(lexer_id) {
								uint   lexer_descr_p = 0;
								if(SXml::GetAttrib(p_s, "desc", lexer_descr) > 0) 
									AddS(lexer_descr, &lexer_descr_p);
								for(xmlNode * p_e = p_s->children; p_e; p_e = p_e->next) {
									if(SXml::IsName(p_e, "WordsStyle")) {
										InnerStyle st;
										MEMSZERO(st);
										st.Group = sgLexer;
										st.LexerId = lexer_id;
										st.LexerDescrP = lexer_descr_p;
										ReadStyleAttributes(p_e, st); // @v10.8.0
										/* @v10.8.0 st.FgC.SetEmpty();
										st.BgC.SetEmpty();
										st.FontStyle = -1;
										if(SXml::GetAttrib(p_e, "name", temp_buf) > 0)
											AddS(temp_buf, &st.StyleNameP);
										if(SXml::GetAttrib(p_e, "styleID", temp_buf) > 0)
											st.StyleId = (uint)temp_buf.ToLong();
										if(SXml::GetAttrib(p_e, "fgColor", temp_buf) > 0) {
											if(temp_buf.Len() == 6) {
												temp_buf.Insert(0, "#");
												st.FgC.FromStr(temp_buf);
											}
										}
										if(SXml::GetAttrib(p_e, "bgColor", temp_buf) > 0) {
											if(temp_buf.Len() == 6) {
												temp_buf.Insert(0, "#");
												st.BgC.FromStr(temp_buf);
											}
										}
										if(SXml::GetAttrib(p_e, "fontName", temp_buf) > 0)
											AddS(temp_buf, &st.FontFaceP);
										if(SXml::GetAttrib(p_e, "fontStyle", temp_buf) > 0)
											st.FontStyle = temp_buf.ToLong();
										if(SXml::GetAttrib(p_e, "fontSize", temp_buf) > 0)
											st.FontSize = (uint)temp_buf.ToLong();
										if(SXml::GetAttrib(p_e, "keywordClass", temp_buf) > 0)
											AddS(temp_buf, &st.KeywordClassP);*/
										THROW_SL(L.insert(&st));
									}
								}
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "GlobalStyles")) {
				for(xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "WidgetStyle")) {
						InnerStyle st;
						MEMSZERO(st);
						st.Group = sgGlobal;
						ReadStyleAttributes(p_s, st); // @v10.8.0
						/* @v10.8.0 st.FgC.SetEmpty();
						st.BgC.SetEmpty();
						if(SXml::GetAttrib(p_s, "name", temp_buf) > 0)
							AddS(temp_buf, &st.StyleNameP);
						if(SXml::GetAttrib(p_s, "styleID", temp_buf) > 0)
							st.StyleId = (uint)temp_buf.ToLong();
						if(SXml::GetAttrib(p_s, "fgColor", temp_buf) > 0) {
							if(temp_buf.Len() == 6) {
								temp_buf.Insert(0, "#");
								st.FgC.FromStr(temp_buf);
							}
						}
						if(SXml::GetAttrib(p_s, "bgColor", temp_buf) > 0) {
							if(temp_buf.Len() == 6) {
								temp_buf.Insert(0, "#");
								st.BgC.FromStr(temp_buf);
							}
						}
						if(SXml::GetAttrib(p_s, "fontName", temp_buf) > 0)
							AddS(temp_buf, &st.FontFaceP);
						if(SXml::GetAttrib(p_s, "fontStyle", temp_buf) > 0)
							st.FontStyle = (uint)temp_buf.ToLong();
						if(SXml::GetAttrib(p_s, "fontSize", temp_buf) > 0)
							st.FontSize = (uint)temp_buf.ToLong();
						if(SXml::GetAttrib(p_s, "keywordClass", temp_buf) > 0)
							AddS(temp_buf, &st.KeywordClassP);*/
						THROW_SL(L.insert(&st));
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int SLAPI SScEditorStyleSet::ParseModelXml(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString lexer_name;
	SString lexer_ext;
	SString keyword_class_name;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	THROW_SL(fileExists(pFileName));
	THROW(p_ctx = xmlNewParserCtxt());
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "EditorLangModels")) {
		for(xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "Languages")) {
				for(xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "Language")) {
						if(SXml::GetAttrib(p_s, "name", lexer_name) > 0) {
							uint   lexer_id = SScGetLexerIdByName(lexer_name);
							if(lexer_id) {
								InnerLangModel model;
								MEMSZERO(model);
								model.LexerId = lexer_id;
								if(SXml::GetAttrib(p_s, "ext", temp_buf) > 0)
									AddS(temp_buf, &model.ExtListP);
								if(SXml::GetAttrib(p_s, "commentLine", temp_buf) > 0)
									AddS(temp_buf, &model.CommentLineP);
								if(SXml::GetAttrib(p_s, "commentStart", temp_buf) > 0)
									AddS(temp_buf, &model.CommentStartP);
								if(SXml::GetAttrib(p_s, "commentEnd", temp_buf) > 0)
									AddS(temp_buf, &model.CommentEndP);
								THROW_SL(ML.insert(&model));
								for(xmlNode * p_e = p_s->children; p_e; p_e = p_e->next) {
									if(SXml::IsName(p_e, "Keywords")) {
										if(SXml::GetContent(p_e, temp_buf) > 0 && temp_buf.NotEmptyS()) {
											InnerLangModelKeywords entry;
											MEMSZERO(entry);
											entry.LexerId = lexer_id;
											AddS(temp_buf, &entry.KeywordListP);
											if(SXml::GetAttrib(p_e, "name", temp_buf) > 0)
												AddS(temp_buf, &entry.KeywordClassP);
											THROW_SL(KwL.insert(&entry));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

void SScEditorBase::SetSpecialStyle(const SScEditorStyleSet::Style & rStyle)
{
	int styleID = rStyle.StyleId;
	if(!rStyle.FgC.IsEmpty())
		CallFunc(SCI_STYLESETFORE, styleID, rStyle.FgC);
	if(!rStyle.BgC.IsEmpty())
		CallFunc(SCI_STYLESETBACK, styleID, rStyle.BgC);
	if(rStyle.FontFace.NotEmpty()) {
		CallFunc(SCI_STYLESETFONT, styleID, reinterpret_cast<intptr_t>(rStyle.FontFace.cptr()));
	}
	int font_style = rStyle.FontStyle;
	if(font_style != -1/*STYLE_NOT_USED*/) {
		CallFunc(SCI_STYLESETBOLD, styleID, font_style & SScEditorStyleSet::FONTSTYLE_BOLD);
		CallFunc(SCI_STYLESETITALIC, styleID, font_style & SScEditorStyleSet::FONTSTYLE_ITALIC);
		CallFunc(SCI_STYLESETUNDERLINE, styleID, font_style & SScEditorStyleSet::FONTSTYLE_UNDERLINE);
	}
	if(rStyle.FontSize > 0)
		CallFunc(SCI_STYLESETSIZE, styleID, rStyle.FontSize);
}

const int LANG_INDEX_INSTR  = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE   = 2;
const int LANG_INDEX_TYPE2  = 3;
const int LANG_INDEX_TYPE3  = 4;
const int LANG_INDEX_TYPE4  = 5;
const int LANG_INDEX_TYPE5  = 6;

static int GetKwClassFromName(const char * pStr, const char * pLexerName)
{
	if(sstreq(pStr, "instre1")) 
		return LANG_INDEX_INSTR;
	else if(sstreq(pStr, "instre2")) 
		return LANG_INDEX_INSTR2;
	else if(sstreq(pStr, "type1")) {
		if(sstreq(pLexerName, "cpp"))
			return 1;
		else
			return LANG_INDEX_TYPE;
	}
	else if(sstreq(pStr, "type2")) 
		return LANG_INDEX_TYPE2;
	else if(sstreq(pStr, "type3")) 
		return LANG_INDEX_TYPE3;
	else if(sstreq(pStr, "type4")) 
		return LANG_INDEX_TYPE4;
	else if(sstreq(pStr, "type5")) 
		return LANG_INDEX_TYPE5;
	else if(pStr[1] == 0 && pStr[0] >= '0' && pStr[0] <= '8')
		return (pStr[0] - '0');
	else
		return -1;
}

static SScEditorStyleSet * _GetGlobalSScEditorStyleSetInstance()
{
	static const char * P_GlobalSymbol = "SScEditorStyleSet";
	SScEditorStyleSet * p_ss = 0;
	long   symbol_id = SLS.GetGlobalSymbol(P_GlobalSymbol, -1, 0);
	THROW_SL(symbol_id);
	if(symbol_id < 0) {
		TSClassWrapper <SScEditorStyleSet> cls;
		THROW_SL(symbol_id = SLS.CreateGlobalObject(cls));
		THROW_SL(p_ss = static_cast<SScEditorStyleSet *>(SLS.GetGlobalObject(symbol_id)));
		{
			long s = SLS.GetGlobalSymbol(P_GlobalSymbol, symbol_id, 0);
			assert(symbol_id == s);
		}
		{
			int    r = 0;
			SString file_name;
			PPGetFilePath(PPPATH_DD, "editorlangmodel.xml", file_name);
			r = p_ss->ParseModelXml(file_name);
			if(r) {
				PPGetFilePath(PPPATH_DD, "editorstyles.xml", file_name);
				r = p_ss->ParseStylesXml(file_name);
			}
			if(!r) {
				p_ss->Destroy(); // Указатель оставляем не нулевым дабы в течении сеанса каждый раз не пытаться создавать его заново.
			}
		}
	}
	else if(symbol_id > 0) {
		THROW_SL(p_ss = static_cast<SScEditorStyleSet *>(SLS.GetGlobalObject(symbol_id)));
	}
	CATCH
		p_ss = 0;
	ENDCATCH
	return p_ss;
}

int SScEditorBase::SetLexer(const char * pLexerName)
{
	int    ok = 0;
	SScEditorStyleSet * p_ss = _GetGlobalSScEditorStyleSetInstance();
	int    lexer_id = SScGetLexerIdByName(pLexerName);
	if(lexer_id && p_ss) {
		int lexer_model = SScGetLexerModelById(lexer_id);
		SScEditorStyleSet::LangModel model;
		if(p_ss->GetModel(lexer_model, &model)) {

			CallFunc(SCI_SETLEXER, lexer_model);

			TSCollection <SScEditorStyleSet::LangModelKeywords> kw_list;
			TSCollection <SScEditorStyleSet::Style> style_list;
			p_ss->GetStyles(SScEditorStyleSet::sgLexer, lexer_id, &style_list);
			int    kwc = p_ss->GetModelKeywords(lexer_model, &kw_list);
			for(uint i = 0; i < style_list.getCount(); i++) {
				const SScEditorStyleSet::Style * p_style = style_list.at(i);
				if(p_style) {
					if(p_style->KeywordClass.NotEmpty()) {
						for(uint j = 0; j < kw_list.getCount(); j++) {
							SScEditorStyleSet::LangModelKeywords * p_kw = kw_list.at(j);
							if(p_kw && p_kw->KeywordClass.CmpNC(p_style->KeywordClass) == 0) {
								int kw_n = GetKwClassFromName(p_kw->KeywordClass, pLexerName);
								if(kw_n >= 0 && kw_n <= 8) {
									CallFunc(SCI_SETKEYWORDS, kw_n, reinterpret_cast<intptr_t>(p_kw->KeywordList.cptr()));
									break;
								}
							}
						}
					}
					SetSpecialStyle(*p_style);
				}
			}
			if(sstreq(pLexerName, "cpp")) {
				//width = NppParameters::getInstance()->_dpiManager.scaleX(100) >= 150 ? 18 : 14;
				//CallFunc(SCI_SETMARGINWIDTHN, 2/*folding*/, 14); // @v10.2.0
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<intptr_t>("1"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<intptr_t>("0"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<intptr_t>("1"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<intptr_t>("1"));
				// Disable track preprocessor to avoid incorrect detection.
				// In the most of cases, the symbols are defined outside of file.
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<intptr_t>("0"));
			}
			ok = 1;
		}
	}
	//CATCHZOK
	return ok;
}
//
//
//
class SearchReplaceDialog : public TDialog {
	DECL_DIALOG_DATA(SSearchReplaceParam);
public:
	SearchReplaceDialog() : TDialog(DLG_SCISEARCH)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		setCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		AddClusterAssoc(CTL_SCISEARCH_RF, 0, SSearchReplaceParam::fReplace);
		SetClusterData(CTL_SCISEARCH_RF, Data.Flags);
		if(Data.Flags & SSearchReplaceParam::fReplace) {
			disableCtrl(CTL_SCISEARCH_REPLACE, 0);
			setCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		}
		else {
			disableCtrl(CTL_SCISEARCH_REPLACE, 1);
		}
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 0, SSearchReplaceParam::fNoCase);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 1, SSearchReplaceParam::fWholeWords);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 2, SSearchReplaceParam::fReverse);
		SetClusterData(CTL_SCISEARCH_FLAGS, Data.Flags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		getCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
		GetClusterData(CTL_SCISEARCH_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_SCISEARCH_RF)) {
			GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
			disableCtrl(CTL_SCISEARCH_REPLACE, !BIN(Data.Flags & SSearchReplaceParam::fReplace));
		}
		else
			return;
		clearEvent(event);
	}
};

int SLAPI EditSearchReplaceParam(SSearchReplaceParam * pData) { DIALOG_PROC_BODY(SearchReplaceDialog, pData); }
//
//
//
SScEditorBase::SScEditorBase() : P_SciFn(0), P_SciPtr(0), P_Tknzr(0)
{
	Init(0, 0);
}

SScEditorBase::~SScEditorBase()
{
	delete P_Tknzr;
}

void SScEditorBase::ClearIndicator(int indicatorNumber)
{
	int doc_start = 0;
	int doc_end = CallFunc(SCI_GETLENGTH);
	CallFunc(SCI_SETINDICATORCURRENT, indicatorNumber);
	CallFunc(SCI_INDICATORCLEARRANGE, doc_start, doc_end-doc_start);
}

void SScEditorBase::Init(HWND hScW, int preserveFileName)
{
	P_SciFn = 0;
	P_SciPtr = 0;
	Doc.Reset(preserveFileName);
	if(hScW) {
		P_SciFn  = reinterpret_cast<intptr_t (__cdecl *)(void *, uint, uintptr_t, intptr_t)>(::SendMessage(hScW, SCI_GETDIRECTFUNCTION, 0, 0));
		P_SciPtr = reinterpret_cast<void *>(SendMessage(hScW, SCI_GETDIRECTPOINTER, 0, 0));
		CallFunc(SCI_INDICSETSTYLE, indicUnknWord, /*INDIC_SQUIGGLE*/INDIC_COMPOSITIONTHICK);
		CallFunc(SCI_INDICSETFORE, indicUnknWord, GetColorRef(SClrRed));
		CallFunc(SCI_INDICSETSTYLE, indicStxRule, INDIC_PLAIN);
		CallFunc(SCI_INDICSETFORE, indicStxRule, GetColorRef(SClrCyan));
	}
}

int SScEditorBase::Release()
{
	int    ok = -1;
	if(Doc.SciDoc) {
		CallFunc(SCI_CLEARALL);
		CallFunc(SCI_RELEASEDOCUMENT, 0, reinterpret_cast<intptr_t>(Doc.SciDoc));
		Doc.Reset(0);
		ok = 1;
	}
	return ok;
}

intptr_t SScEditorBase::CallFunc(uint msg) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, 0, 0) : 0; }
intptr_t SScEditorBase::CallFunc(uint msg, uintptr_t param1) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, param1, 0) : 0; }
intptr_t SScEditorBase::CallFunc(uint msg, uintptr_t param1, intptr_t param2) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, param1, param2) : 0; }

int SScEditorBase::SetKeybAccelerator(KeyDownCommand & rK, int cmd)
{
	int    ok = OuterKeyAccel.Set(rK, cmd);
	KeyAccel.Set(rK, cmd);
	return ok;
}

int32 SScEditorBase::GetCurrentPos()
{
	return CallFunc(SCI_GETCURRENTPOS);
}

int32 FASTCALL SScEditorBase::SetCurrentPos(int32 pos)
{
	int32 prev = CallFunc(SCI_GETCURRENTPOS);
	CallFunc(SCI_SETCURRENTPOS, pos);
	return prev;
}

int FASTCALL SScEditorBase::GetSelection(IntRange & rR)
{
	rR.Set(CallFunc(SCI_GETSELECTIONSTART), CallFunc(SCI_GETSELECTIONEND));
	return 1;
}

int FASTCALL SScEditorBase::SetSelection(const IntRange * pR)
{
	int    ok = -1;
	if(!pR || pR->IsZero()) {
		CallFunc(SCI_SETEMPTYSELECTION);
		ok = -1;
	}
	else {
		CallFunc(SCI_SETSELECTIONSTART, pR->low);
		CallFunc(SCI_SETSELECTIONEND, pR->upp);
		ok = 1;
	}
	return ok;
}

int FASTCALL SScEditorBase::GetSelectionText(SString & rBuf)
{
	rBuf.Z();
	int sz = CallFunc(SCI_GETSELTEXT);
	if(sz > 0) {
		STempBuffer temp_b(sz);
		if(temp_b.IsValid()) {
			sz = CallFunc(SCI_GETSELTEXT, 0, reinterpret_cast<intptr_t>((char *)temp_b));
			rBuf = temp_b;
		}
		else
			sz = 0;
	}
	return sz;
}

int SScEditorBase::SearchAndReplace(long flags)
{
	int    ok = -1;
	SSearchReplaceParam param = LastSrParam;
	IntRange sel;
	int   ssz = 0;
	if(param.Pattern.Empty() || flags & srfUseDialog) {
		ssz = GetSelectionText(param.Pattern);
		if(ssz > 0)
			param.Pattern.Transf(CTRANSF_UTF8_TO_INNER);
	}
	if(!(flags & srfUseDialog) || EditSearchReplaceParam(&param) > 0) {
		LastSrParam = param;
		SString pattern = param.Pattern;
		pattern.Transf(CTRANSF_INNER_TO_UTF8);
		if(pattern.NotEmpty()) {
			int    sci_srch_flags = 0;
			int    _func = 0;
			if(!(param.Flags & param.fNoCase))
				sci_srch_flags |= SCFIND_MATCHCASE;
			if(param.Flags & param.fWholeWords)
				sci_srch_flags |= SCFIND_WHOLEWORD;
			GetSelection(sel);
			const IntRange preserve_sel = sel;
			if(param.Flags & param.fReverse) {
				_func = SCI_SEARCHPREV;
			}
			else {
				_func = SCI_SEARCHNEXT;
				sel.low++;
				SetSelection(&sel);
			}
			CallFunc(SCI_SEARCHANCHOR);
			int    result = CallFunc(_func, sci_srch_flags, reinterpret_cast<intptr_t>(pattern.cptr()));
			if(result >= 0) {
				ok = 1;
				int selend = CallFunc(SCI_GETSELECTIONEND);
				SetCurrentPos(selend);
				CallFunc(SCI_SCROLLCARET);
				IntRange sel;
				SetSelection(&sel.Set(result, selend));
				CallFunc(SCI_SEARCHANCHOR);
			}
			else {
				SetSelection(&preserve_sel);
			}
		}
	}
	return ok;
}
//
//
//
#if 0 // unused {
static int FASTCALL VkToScTranslate(int keyIn)
{
	switch(keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:			return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:		return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:		return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:		return SCK_RETURN;
		case VK_ADD:		return SCK_ADD;
		case VK_SUBTRACT:	return SCK_SUBTRACT;
		case VK_DIVIDE:		return SCK_DIVIDE;
		case VK_OEM_2:		return '/';
		case VK_OEM_3:		return '`';
		case VK_OEM_4:		return '[';
		case VK_OEM_5:		return '\\';
		case VK_OEM_6:		return ']';
		default:			return keyIn;
	}
};

static int FASTCALL ScToVkTranslate(int keyIn)
{
	switch(keyIn) {
		case SCK_DOWN: return VK_DOWN;
		case SCK_UP: return VK_UP;
		case SCK_LEFT: return VK_LEFT;
		case SCK_RIGHT: return VK_RIGHT;
		case SCK_HOME: return VK_HOME;
		case SCK_END: return VK_END;
		case SCK_PRIOR: return VK_PRIOR;
		case SCK_NEXT: return VK_NEXT;
		case SCK_DELETE: return VK_DELETE;
		case SCK_INSERT: return VK_INSERT;
		case SCK_ESCAPE: return VK_ESCAPE;
		case SCK_BACK: return VK_BACK;
		case SCK_TAB: return VK_TAB;
		case SCK_RETURN: return VK_RETURN;
		case SCK_ADD: return VK_ADD;
		case SCK_SUBTRACT: return VK_SUBTRACT;
		case SCK_DIVIDE: return VK_DIVIDE;
		case '/': return VK_OEM_2;
		case '`': return VK_OEM_3;
		case '[': return VK_OEM_4;
		case '\\': return VK_OEM_5;
		case ']': return VK_OEM_6;
		default: return keyIn;
	}
}
#endif // } 0 unused

STextBrowser::Document::Document() : Cp(/*cpANSI*/cpUTF8), Eolf(eolUndef), State(0), SciDoc(0) // @v9.9.9 cpANSI-->cpUTF8
{
}

STextBrowser::Document & FASTCALL STextBrowser::Document::Reset(int preserveFileName)
{
	OrgCp = cpUndef;
	Cp = cpUndef;
	Eolf = eolUndef;
	State = 0;
	SciDoc = 0;
	if(!preserveFileName)
		FileName.Z();
	return *this;
}

long STextBrowser::Document::SetState(long st, int set)
{
	SETFLAG(State, st, set);
	return State;
}

STextBrowser::STextBrowser() : TBaseBrowserWindow(WndClsName), SScEditorBase(), SpcMode(spcmNo)
{
	Init(0, 0, 0);
}

STextBrowser::STextBrowser(const char * pFileName, const char * pLexerSymb, int toolbarId) : TBaseBrowserWindow(WndClsName), SScEditorBase(), SpcMode(spcmNo)
{
	Init(pFileName, pLexerSymb, toolbarId);
}

STextBrowser::~STextBrowser()
{
	if(::IsWindow(HwndSci)) {
		FileClose();
		if(OrgScintillaWndProc) {
			TView::SetWindowProp(HwndSci, GWLP_WNDPROC, OrgScintillaWndProc);
			TView::SetWindowProp(HwndSci, GWLP_USERDATA, (void *)0);
		}
		DestroyWindow(HwndSci);
	}
	P_Toolbar->DestroyHWND();
	ZDELETE(P_Toolbar);
	// @v9.7.11 ZDELETE(P_SrDb); // @v9.2.0
}

int STextBrowser::SetSpecialMode(int spcm)
{
	int    ok = 1;
	if(spcm == spcmSartrTest) {
		SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();
		if(p_srdb) {
			/*if(!P_SrDb) {
				//SString db_path;
				//getExecPath(db_path);
				//db_path.SetLastSlash().Cat("SARTRDB");
				THROW_S(P_SrDb = new SrDatabase(), SLERR_NOMEM);
				THROW(P_SrDb->Open(0, 0)); // @todo Режим открытия
			}*/
			SpcMode = spcm;
		}
	}
	else {
		// @v9.7.11 ZDELETE(P_SrDb);
		SpcMode = spcmNo;
	}
	return ok;
}

/*virtual*/TBaseBrowserWindow::IdentBlock & STextBrowser::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasTextBrowser;
	rBlk.ClsName = SUcSwitch(STextBrowser::WndClsName); // @unicodeproblem
	(rBlk.InstanceIdent = Doc.FileName).Strip().ToLower();
	return rBlk;
}

int STextBrowser::Init(const char * pFileName, const char * pLexerSymb, int toolbarId)
{
	SScEditorBase::Init(0, 0);
	LexerSymb = pLexerSymb; // @v10.2.6
	OrgScintillaWndProc = 0;
	SysState = 0;
	Doc.FileName = pFileName;
	BbState |= bbsWoScrollbars;
	P_Toolbar    = 0;
	ToolBarWidth = 0;
	if(toolbarId < 0)
		ToolbarId = TOOLBAR_TEXTBROWSER;
	else if(toolbarId > 0)
		ToolbarId = toolbarId;
	{
		KeyDownCommand k;
		k.SetTvKeyCode(kbF3);
		SetKeybAccelerator(k, PPVCMD_SEARCHNEXT);
	}
	return 1;
}

// static
LPCTSTR STextBrowser::WndClsName = _T("STextBrowser"); // @global

// static
int STextBrowser::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	MEMSZERO(wc);
	wc.cbSize        = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wc.lpfnWndProc   = STextBrowser::WndProc;
	wc.cbClsExtra    = BRWCLASS_CEXTRA;
	wc.cbWndExtra    = BRWCLASS_WEXTRA;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.hCursor       = NULL; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = ::CreateSolidBrush(RGB(0xEE, 0xEE, 0xEE));
	wc.lpszClassName = STextBrowser::WndClsName;
#if !defined(_PPDLL) && !defined(_PPSERVER)
	Scintilla_RegisterClasses(hInst);
#endif
	return RegisterClassEx(&wc);
}

// static
LRESULT CALLBACK STextBrowser::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	STextBrowser * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_init_data = reinterpret_cast<CREATESTRUCT *>(lParam);
			if(TWindow::IsMDIClientWindow(p_init_data->hwndParent)) {
				p_view = reinterpret_cast<STextBrowser *>(static_cast<LPMDICREATESTRUCT>(p_init_data->lpCreateParams)->lParam);
				p_view->BbState |= bbsIsMDI;
			}
			else {
				p_view = static_cast<STextBrowser *>(p_init_data->lpCreateParams);
				p_view->BbState &= ~bbsIsMDI;
			}
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessage(hWnd, WM_NCACTIVATE, TRUE, 0L);
				p_view->WMHCreate();
				PostMessage(hWnd, WM_PAINT, 0, 0);
				{
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf);
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				::SetFocus(p_view->HwndSci);
				return 0;
			}
			else
				return -1;
		case WM_COMMAND:
			{
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					if(HIWORD(wParam) == 0) {
						if(p_view->KeyAccel.getCount()) {
							long   cmd = 0;
							KeyDownCommand k;
							k.SetTvKeyCode(LOWORD(wParam));
							if(p_view->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
								p_view->ProcessCommand(cmd, 0, p_view);
							}
						}
					}
					/*
					if(LOWORD(wParam))
						p_view->ProcessCommand(LOWORD(wParam), 0, p_view);
					*/
				}
			}
			break;
		case WM_DESTROY:
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				p_view->SaveChanges();
				SETIFZ(p_view->EndModalCmd, cmCancel);
				APPL->DelItemFromMenu(p_view);
				p_view->ResetOwnerCurrent();
				if(!p_view->IsInState(sfModal)) {
					APPL->P_DeskTop->remove(p_view);
					delete p_view;
					TView::SetWindowProp(hWnd, GWLP_USERDATA, (void *)0);
				}
			}
			return 0;
		case WM_SETFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION)) {
				SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
				APPL->NotifyFrame(0);
			}
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				::SetFocus(p_view->HwndSci);
				APPL->SelectTabItem(p_view);
				TView::messageBroadcast(p_view, cmReceivedFocus);
				p_view->select();
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::GetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0;
				}
			}
			else if(wParam == VK_TAB) {
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(lParam && p_view) {
				HWND hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					TView::messageCommand(p_view, cmResize);
				}
				p_view->Resize();
			}
			break;
		case WM_NOTIFY:
			{
				//LPNMHDR lpnmhdr = (LPNMHDR)lParam;
				const SCNotification * p_scn = (const SCNotification *)lParam;
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && p_scn->nmhdr.hwndFrom == p_view->GetSciWnd()) {
					int    test_value = 0; // @debug
					switch(p_scn->nmhdr.code) {
						case SCN_UPDATEUI:
							StatusWinChange(0, -1);
							break;
						case SCN_CHARADDED:
						case SCN_MODIFIED:
							if(p_scn->modificationType & (SC_MOD_DELETETEXT|SC_MOD_INSERTTEXT|SC_PERFORMED_UNDO|SC_PERFORMED_REDO)) {
								p_view->Doc.SetState(Document::stDirty, 1);
							}
							break;
						case SCN_DWELLSTART:
							{
								test_value = 1;
								if(p_view->SpcMode == spcmSartrTest) {
									SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();
									if(p_srdb) {
										const char * p_wb = " \t.,;:()[]{}/\\!@#$%^&*+=<>\n\r\"\'?";
										const Sci_Position _start_pos = p_scn->position;
										LongArray left, right;
										Sci_Position _pos = _start_pos;
										int    c;
										while((c = p_view->CallFunc(SCI_GETCHARAT, _pos++)) != 0) {
											if(!sstrchr(p_wb, (uchar)c)) {
												right.add(c);
											}
											else
												break;
										}
										if(_start_pos > 0) {
											_pos = _start_pos;
											while((c = p_view->CallFunc(SCI_GETCHARAT, --_pos)) != 0) {
												if(!sstrchr(p_wb, (uchar)c)) {
													left.add(c);
												}
												else
													break;
											}
											left.reverse(0, left.getCount());
										}
										left.add(&right);
										//SCI_CALLTIPSHOW(int posStart, const char *definition)
										if(left.getCount()) {
                                    		SString src_text, text_to_show;
                                    		TSVector <SrWordInfo> info_list;
											for(uint ti = 0; ti < left.getCount(); ti++)
												src_text.CatChar((char)left.at(ti));
											if(p_srdb->GetWordInfo(src_text, 0, info_list) > 0) {
												SString temp_buf;
												for(uint j = 0; j < info_list.getCount(); j++) {
													p_srdb->WordInfoToStr(info_list.at(j), temp_buf);
													if(j)
														text_to_show.CR();
													text_to_show.Cat(temp_buf);
												}
											}
											if(text_to_show.Len())
												p_view->CallFunc(SCI_CALLTIPSHOW, _start_pos, reinterpret_cast<intptr_t>(text_to_show.cptr()));
										}
									}
								}
							}
							break;
						case SCN_DWELLEND:
							{
								p_view->CallFunc(SCI_CALLTIPCANCEL);
							}
							break;
					}
				}
			}
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int STextBrowser::UpdateIndicators()
{
	int    ok = -1;
	const  size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
	if(len && SpcMode == spcmSartrTest) {
		SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();									
		if(p_srdb) {
			SETIFZ(P_Tknzr, new SrSyntaxRuleTokenizer);
			if(P_Tknzr) {
				STokenizer::Param tp;
				P_Tknzr->GetParam(&tp);
				tp.Flags |= STokenizer::fRawOrgOffs;
				P_Tknzr->SetParam(&tp);
				ClearIndicator(indicUnknWord);
				const  uint8 * p_buf = reinterpret_cast<const uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER)); // to get characters directly from Scintilla buffer;
				//int    start_pos = MIN(0, p_scn->position-64);
                SString src_text, text_to_show;
				SString temp_buf;
				TSVector <SrWordInfo> info_list;
				P_Tknzr->Reset(0);
				P_Tknzr->Write("#00", 0, p_buf, len);
				uint idx_first = 0;
				uint idx_count = 0;
				P_Tknzr->Run(&idx_first, &idx_count);
				STokenizer::Item ti;
				for(uint i = 0; i < idx_count; i++) {
					P_Tknzr->Get(idx_first+i, ti);
					if(ti.Token == STokenizer::tokWord && !ti.Text.IsDigit()) {
						if(p_srdb->GetWordInfo(ti.Text, 0, info_list) > 0) {
							/*for(uint j = 0; j < info_list.getCount(); j++) {
								p_srdb->WordInfoToStr(info_list.at(j), temp_buf);
								if(j)
									text_to_show.CR();
								text_to_show.Cat(temp_buf);
							}*/
						}
						else {
							int    start_pos = (int)ti.OrgOffs;
							int    end_pos = 0;
							P_Tknzr->Get(idx_first+i+1, ti);
							end_pos = (int)ti.OrgOffs;
							CallFunc(SCI_SETINDICATORCURRENT, indicUnknWord);
							CallFunc(SCI_INDICATORFILLRANGE, start_pos, end_pos-start_pos);
						}
					}
				}
				{
					const SrSyntaxRuleSet * p_sr = DS.GetSrSyntaxRuleSet();
					TSCollection <SrSyntaxRuleSet::Result> result_list;
					if(p_sr && p_sr->ProcessText(*p_srdb, *(SrSyntaxRuleTokenizer *)P_Tknzr, idx_first, idx_count, result_list) > 0) {
						for(uint residx = 0; residx < result_list.getCount(); residx++) {
							const SrSyntaxRuleSet::Result * p_result = result_list.at(residx);
							if(p_result) {
								int    start_pos = 0;
								int    end_pos = 0;
								P_Tknzr->Get(p_result->TIdxFirst, ti);
								start_pos = (int)ti.OrgOffs;
								P_Tknzr->Get(p_result->TIdxNext, ti);
								end_pos = (int)ti.OrgOffs;
								CallFunc(SCI_SETINDICATORCURRENT, indicStxRule);
								CallFunc(SCI_INDICATORFILLRANGE, start_pos, end_pos-start_pos);
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int STextBrowser::Run()
{
	int    ok = -1;

	return ok;
}

int STextBrowser::GetStatus(StatusBlock * pSb)
{
	int    ok = 1;
	if(pSb) {
		pSb->TextSize = CallFunc(SCI_GETTEXTLENGTH);
		pSb->LineCount = CallFunc(SCI_GETLINECOUNT);
		const int32 pos = GetCurrentPos();
		pSb->LineNo = CallFunc(SCI_LINEFROMPOSITION, pos);
		pSb->ColumnNo = CallFunc(SCI_GETCOLUMN, pos);
		pSb->Cp = Doc.OrgCp;
	}
	return ok;
}

int STextBrowser::Resize()
{
	if(HwndSci != 0) {
		RECT rc;
		GetWindowRect(H(), &rc);
		if(IsWindowVisible(APPL->H_ShortcutsWnd)) {
			RECT sh_rect;
			GetWindowRect(APPL->H_ShortcutsWnd, &sh_rect);
			rc.bottom -= sh_rect.bottom - sh_rect.top;
		}
		MoveWindow(HwndSci, 0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, 1);
	}
	return 1;
}

SKeyAccelerator::SKeyAccelerator() : LAssocArray()
{
}

int SKeyAccelerator::Set(const KeyDownCommand & rK, int cmd)
{
	int    ok = 0;
	long   key = rK;
	long   val = 0;
	uint   pos = 0;
	if(Search(key, &val, &pos)) {
		if(cmd > 0) {
			if(cmd != val) {
				at(pos).Val = cmd;
				ok = 2;
			}
			else
				ok = -1;
		}
		else {
			atFree(pos);
			ok = 4;
		}
	}
	else {
		Add(key, cmd, 0);
		ok = 1;
	}
	return ok;
}

// @v9.7.5 moved(tv.h) int ImpLoadToolbar(TVRez & rez, ToolbarList * pList); // @prototype(wbrowse.cpp)

#if 0 // {
static int ImpLoadToolbar(TVRez & rez, ToolbarList * pList)
{
	pList->setBitmap(rez.getUINT());
	SString temp_buf;
	while(rez.getUINT() != TV_END) {
		fseek(rez.getStream(), -((long)sizeof(uint16)), SEEK_CUR);
		ToolbarItem item;
		// @v10.7.8 @ctr MEMSZERO(item);
		item.Cmd = rez.getUINT();
		if(item.Cmd != TV_MENUSEPARATOR) {
			item.KeyCode = rez.getUINT();
			item.Flags = rez.getUINT();
			item.BitmapIndex = rez.getUINT();
			// @v9.0.11 rez.getString(item.ToolTipText);
			// @v9.0.11 {
			rez.getString(temp_buf, 0);
            PPExpandString(temp_buf);
            STRNSCPY(item.ToolTipText, temp_buf);
            // } @v9.0.11
		}
		else
			item.KeyCode = (ushort)item.Cmd;
		pList->addItem(&item);
	}
	return 1;
}
#endif // } 0

int SLAPI STextBrowser::LoadToolbar(uint tbId)
{
	int    r = 0;
	TVRez & rez = *P_SlRez;
	ToolbarList tb_list;
	r = rez.findResource(tbId, TV_EXPTOOLBAR, 0, 0) ? ImpLoadToolbar(rez, &tb_list) : 0;
	if(r > 0)
		setupToolbar(&tb_list);
	return r;
}

//static
LRESULT CALLBACK STextBrowser::ScintillaWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	STextBrowser * p_this = reinterpret_cast<STextBrowser *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(p_this) {
		switch(msg) {
			case WM_DESTROY:
				TView::SetWindowProp(p_this->HwndSci, GWLP_WNDPROC, p_this->OrgScintillaWndProc);
				TView::SetWindowProp(p_this->HwndSci, GWLP_USERDATA, (void *)0);
				return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_CHAR:
				if(p_this->SysState & p_this->sstLastKeyDownConsumed)
					return ::DefWindowProc(hwnd, msg, wParam, lParam);
				else
					return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				{
					p_this->SysState &= ~p_this->sstLastKeyDownConsumed;
					int    processed = 0;
					KeyDownCommand k;
					k.SetWinMsgCode(wParam);
					if(k.Code == VK_TAB && k.State & k.stateCtrl) {
						::SendMessage(p_this->H(), WM_KEYDOWN, wParam, lParam);
						p_this->SysState |= p_this->sstLastKeyDownConsumed;
						processed = 1;
					}
					else if(p_this->KeyAccel.getCount()) {
						long   cmd = 0;
						if(p_this->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
							p_this->SysState |= p_this->sstLastKeyDownConsumed;
							p_this->ProcessCommand(cmd, 0, p_this);
							processed = 1;
						}
					}
					return processed ? ::DefWindowProc(hwnd, msg, wParam, lParam) : ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
				}
				break;
			default:
				return ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
		}
	}
	else
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
};

int STextBrowser::WMHCreate()
{
	RECT rc;
	GetWindowRect(H(), &rc);
	P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
	if(P_Toolbar && LoadToolbar(ToolbarId) > 0) {
		P_Toolbar->Init(ToolbarID, &Toolbar);
		if(P_Toolbar->IsValid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	HwndSci = ::CreateWindowEx(WS_EX_CLIENTEDGE, _T("Scintilla"), _T(""), WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
		0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, H(), 0/*(HMENU)GuiID*/, APPL->GetInst(), 0);
	SScEditorBase::Init(HwndSci, 1/*preserveFileName*/);
	TView::SetWindowProp(HwndSci, GWLP_USERDATA, this);
	OrgScintillaWndProc = static_cast<WNDPROC>(TView::SetWindowProp(HwndSci, GWLP_WNDPROC, ScintillaWindowProc));
	// @v8.6.2 (SCI_SETKEYSUNICODE deprecated in sci 3.5.5) CallFunc(SCI_SETKEYSUNICODE, 1, 0);
	CallFunc(SCI_SETCARETLINEVISIBLE, 1);
	CallFunc(SCI_SETCARETLINEBACK, RGB(232,232,255));
	CallFunc(SCI_SETSELBACK, 1, RGB(117,217,117));
	CallFunc(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED); // @v9.8.2 SC_EFF_QUALITY_ANTIALIASED-->SC_EFF_QUALITY_LCD_OPTIMIZED
	// CallFunc(SCI_SETTECHNOLOGY, /*SC_TECHNOLOGY_DIRECTWRITERETAIN*/SC_TECHNOLOGY_DIRECTWRITEDC, 0); // @v9.8.2
	//
	CallFunc(SCI_SETMOUSEDWELLTIME, 500);
	//
	{
		KeyAccel.clear();
		{
			for(uint i = 0; i < OuterKeyAccel.getCount(); i++) {
				const LAssoc & r_accel_item = OuterKeyAccel.at(i);
				KeyDownCommand & r_k = *(KeyDownCommand *)&r_accel_item.Key;
				KeyAccel.Set(r_k, r_accel_item.Val);
			}
		}
		if(P_Toolbar) {
			const uint tbc = P_Toolbar->getItemsCount();
			for(uint i = 0; i < tbc; i++) {
				const ToolbarItem & r_tbi = P_Toolbar->getItem(i);
				if(!(r_tbi.Flags & r_tbi.fHidden) && r_tbi.KeyCode && r_tbi.KeyCode != TV_MENUSEPARATOR && r_tbi.Cmd) {
					KeyDownCommand k;
					if(k.SetTvKeyCode(r_tbi.KeyCode))
						KeyAccel.Set(k, r_tbi.Cmd);
				}
			}
		}
		KeyAccel.Sort();
	}
	FileLoad(Doc.FileName, /*cpANSI*/cpUTF8, 0); // @v9.9.9 cpANSI-->cpUTF8
	return BIN(P_SciFn && P_SciPtr);
}

SCodepage STextBrowser::SelectEncoding(SCodepage initCp) const
{
	SCodepage result_cp = initCp;
	ListWindow * p_lw = CreateListWindow(128, lbtDisposeData | lbtDblClkNotify);
	if(p_lw) {
		SCodepage cp;
		SString cp_name;
		for(uint i = 0; i < SCodepageIdent::GetRegisteredCodepageCount(); i++) {
			if(SCodepageIdent::GetRegisteredCodepage(i, cp, cp_name.Z()) && cp_name.NotEmpty()) {
				p_lw->listBox()->addItem(cp, cp_name);
			}
		}
		p_lw->listBox()->TransmitData(+1, (long *)&result_cp);
		if(ExecView(p_lw) == cmOK) {
			p_lw->listBox()->TransmitData(-1, (long *)&result_cp);
		}
	}
	else {
		result_cp = cpUndef;
		PPError();
	}
	delete p_lw;
	return result_cp;
}

int STextBrowser::SetEncoding(SCodepage cp)
{
	int    ok = -1;
	if(cp == cpUndef) {
		cp = SelectEncoding(cp);
	}
	if(cp != cpUndef) {
		if(SaveChanges() > 0) {
			if(FileLoad(Doc.FileName, cp, 0))
				ok = 1;
		}
	}
	return ok;
}

int STextBrowser::InsertWorkbookLink()
{
	int    ok = -1;
	{
		MemLeakTracer mlt;
		PPObjWorkbook wb_obj;
		PPObjWorkbook::SelectLinkBlock link;
		link.Type = PPWBTYP_MEDIA;
		if(wb_obj.SelectLink(&link) > 0 && link.ID) {
			WorkbookTbl::Rec rec, addendum_rec;
			if(wb_obj.Fetch(link.ID, &rec) > 0) {
				SString text;
				if(link.Type == link.ltImage) {
					(text = "#IMAGE").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltRef) {
					(text = "#REF").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltLink) {
					(text = "#LINK").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltAnnot) {
					if(link.AddendumID && wb_obj.Fetch(link.AddendumID, &addendum_rec) > 0) {
						text = "#ANNOTIMG";
						(text = "#ANNOTIMG").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatDiv(',', 2).
							CatChar('\'').Cat(addendum_rec.Symb).CatChar('\'').CatChar(')');
					}
					else {
						(text = "#ANNOT").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					}
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
			}
		}
	}
	return ok;
}
//
//
//
struct TidyProcessBlock {
	TidyProcessBlock();

	long   Flags;
	SBaseBuffer InputBuffer;
	SString Output;
	StrAssocArray TidyOptions;
};

int TidyProcessText(TidyProcessBlock & rBlk);

#include <..\osf\tidy\include\tidy.h>

TidyProcessBlock::TidyProcessBlock() : Flags(0)
{
	InputBuffer.Init();
}

int TidyProcessText(TidyProcessBlock & rBlk)
{
	int    ok = 1, r;
	TidyDoc tdoc = tidyCreate();
	TidyBuffer input;
	TidyBuffer output;
	TidyBuffer errbuf;

	tidyBufInit(&input);
	tidyBufInit(&output);
	tidyBufInit(&errbuf);
	for(uint i = 0; i < rBlk.TidyOptions.getCount(); i++) {
		StrAssocArray::Item item = rBlk.TidyOptions.at_WithoutParent(i);
		tidyOptSetValue(tdoc, (TidyOptionId)item.Id, item.Txt);
	}
	r = tidySetErrorBuffer(tdoc, &errbuf);
	tidyBufAppend(&input, rBlk.InputBuffer.P_Buf, rBlk.InputBuffer.Size);

	r = tidyParseBuffer(tdoc, &input);
	r = tidyCleanAndRepair(tdoc);
	r = tidyRunDiagnostics(tdoc);
	r = tidyOptSetBool(tdoc, TidyForceOutput, true);
	r = tidySaveBuffer(tdoc, &output);
	rBlk.Output.Z();
	while(!tidyBufEndOfInput(&output)) {
		rBlk.Output.CatChar(tidyBufGetByte(&output));
	}
	tidyBufFree(&output);
	tidyBufFree(&errbuf);
	tidyRelease(tdoc);
	return ok;
}

int STextBrowser::ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw)
{
	int    ok = -2;
	switch(ppvCmd) {
		case PPVCMD_OPEN:
			{
				SString file_name = Doc.FileName;
				if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, file_name, 0, H()) > 0)
					ok = FileLoad(file_name, cpUTF8, 0);
			}
			break;
		case PPVCMD_SAVE: ok = FileSave(0, 0); break;
		case PPVCMD_SAVEAS: ok = FileSave(0, ofInteractiveSaveAs); break;
		case PPVCMD_SELCODEPAGE: ok = SetEncoding(cpUndef); break;
		case PPVCMD_PROCESSTEXT:
			{
				uint8 * p_buf = reinterpret_cast<uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER, 0, 0));
				const size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
				TidyProcessBlock blk;
				blk.InputBuffer.Set(p_buf, len);
				blk.TidyOptions.Add(TidyInCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyOutCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyWrapLen, "200");
				blk.TidyOptions.Add(TidyIndentContent, "yes");
				blk.TidyOptions.Add(TidyIndentSpaces, "4");
				blk.TidyOptions.Add(TidyBodyOnly, "yes");
				if(TidyProcessText(blk) > 0) {
					CallFunc(SCI_CLEARALL, 0, 0);
					CallFunc(SCI_APPENDTEXT, (int)blk.Output.Len(), reinterpret_cast<intptr_t>(blk.Output.cptr()));
					ok = 1;
				}
			}
			break;
		case PPVCMD_SEARCH: SearchAndReplace(srfUseDialog); break;
		case PPVCMD_SEARCHNEXT: SearchAndReplace(0); break;
		case PPVCMD_INSERTLINK: InsertWorkbookLink(); break;
		case PPVCMD_BRACEHTMLTAG: BraceHtmlTag(); break;
		case PPVCMD_SETUPSARTREINDICATORS: UpdateIndicators(); break;
		case PPVCMD_RUN: Run(); break;
	}
	return ok;
}

int STextBrowser::SaveChanges()
{
	int    ok = 1;
	if(Doc.State & Document::stDirty) {
		if(CONFIRM(PPCFM_DATACHANGED))
			ok = FileSave(0, 0);
		else
			ok = -1;
	}
	return ok;
}

int STextBrowser::FileClose()
{
	return SScEditorBase::Release();
}

int STextBrowser::FileLoad(const char * pFileName, SCodepage orgCp, long flags)
{
	int    ok = 1;
	SString file_name;
	(file_name = pFileName).Strip();
	THROW_SL(fileExists(file_name));
	{
		SFileFormat ff;
		const int fir = ff.Identify(file_name);
		size_t block_size = 8 * 1024 * 1024;
		int64  _fsize = 0;
		SFile _f(file_name, SFile::mRead|SFile::mBinary);
		THROW_SL(_f.IsValid());
		THROW_SL(_f.CalcSize(&_fsize));
		{
			const uint64 bufsize_req = _fsize + MIN(1<<20, _fsize/6);
			THROW(bufsize_req <= 1024*1024*1025);
			{
				Doc.SciDoc = reinterpret_cast<SScEditorBase::SciDocument>(CallFunc(SCI_CREATEDOCUMENT, 0, 0));
				//Setup scratchtilla for new filedata
				CallFunc(SCI_SETSTATUS, SC_STATUS_OK, 0); // reset error status
				CallFunc(SCI_SETDOCPOINTER, 0, (int)Doc.SciDoc);
				const int ro = CallFunc(SCI_GETREADONLY, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 0, 0);
				}
				CallFunc(SCI_CLEARALL, 0, 0);
				//
				// Здесь следует установить LEXER
				//
				if(LexerSymb.NotEmpty()) {
					SetLexer(LexerSymb);
				}
				else if(oneof3(fir, 1, 2, 3)) {
					if(ff == ff.Ini) {
						SetLexer("ini");
					}
					else if(ff == ff.Xml) {
						SetLexer("xml");
					}
					else if(oneof3(ff, ff.C, ff.CPP, ff.H)) {
						SetLexer("cpp");
					}
					else if(ff == ff.Gravity) {

					}
				}
				CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
				CallFunc(SCI_ALLOCATE, static_cast<WPARAM>(bufsize_req), 0);
				THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
				{
					int    first_block = 1;
					STextEncodingStat tes(STextEncodingStat::fUseUCharDet);
					SStringU ubuf;
					SString utfbuf;
					size_t incomplete_multibyte_char = 0;
					size_t actual_size = 0;
					int64  _fsize_rest = _fsize;
					STempBuffer buffer(block_size+8);
					THROW_SL(buffer.IsValid());
					while(_fsize_rest > 0) {
						actual_size = 0;
						THROW_SL(_f.Read(buffer+incomplete_multibyte_char, block_size-incomplete_multibyte_char, &actual_size));
						_fsize_rest -= actual_size;
						actual_size += incomplete_multibyte_char;
						if(first_block) {
							tes.Add(buffer, actual_size);
							tes.Finish();
							if(tes.CheckFlag(tes.fLegalUtf8Only)) {
								if(_fsize_rest > 0) {
									//
									// Если все символы первого блока utf8, но проанализирован не весь
									// файл, то исходной кодовой страницей должна быть заданная из-вне (если определена).
									// Ибо, попытавшись установить utf8 как исходную страницу мы рискуем
									// исказить символы в следующих блоках файла.
									//
									Doc.OrgCp = (orgCp == cpUndef) ? cpUTF8 : orgCp;
								}
								else {
									//
									// Если мы проанализировали весь файл и все символы - utf8, то
									// можно смело устанавливать исходную страницу как utf8
									//
									Doc.OrgCp = cpUTF8;
								}
								Doc.Cp = cpUTF8;
							}
							else {
								//
								// Экспериментальный блок с автоматической идентификацией кодировки.
								// Требуются уточнения.
								//
								SCodepageIdent local_cp = tes.GetAutodetectedCp();
								Doc.OrgCp = (local_cp == cpUndef) ? ((orgCp == cpUndef) ? cpUTF8 : orgCp) : local_cp;
								Doc.Cp = cpUTF8;
							}
							Doc.Eolf = tes.GetEolFormat();
							CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
							{
								int    sci_eol = SC_EOL_CRLF;
								if(Doc.Eolf == eolWindows)
									sci_eol = SC_EOL_CRLF;
								else if(Doc.Eolf == eolUnix)
									sci_eol = SC_EOL_LF;
								else if(Doc.Eolf == eolMac)
									sci_eol = SC_EOL_CR;
								CallFunc(SCI_SETEOLMODE, sci_eol, 0);
							}
							first_block = 0;
						}
						if(Doc.OrgCp == cpUTF8) {
							// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
							CallFunc(SCI_APPENDTEXT, actual_size, reinterpret_cast<intptr_t>(buffer.cptr()));
						}
						else {
							ubuf.CopyFromMb(Doc.OrgCp, buffer, actual_size);
							ubuf.CopyToUtf8(utfbuf, 0);
							CallFunc(SCI_APPENDTEXT, utfbuf.Len(), reinterpret_cast<intptr_t>(utfbuf.cptr()));
						}
						THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
					}
				}
				CallFunc(SCI_EMPTYUNDOBUFFER, 0, 0);
				CallFunc(SCI_SETSAVEPOINT, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 1, 0);
				}
				Doc.SetState(Document::stDirty, 0);
				//CallFunc(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			}
		}
	}
	CATCH
		Doc.Reset(0);
		ok = 0;
	ENDCATCH
	return ok;
}

int STextBrowser::FileSave(const char * pFileName, long flags)
{
	int    ok = -1, skip = 0;
	SString path(isempty(pFileName) ? Doc.FileName.cptr() : pFileName);
	if((flags & ofInteractiveSaveAs) || !path.NotEmptyS()) {
		if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, path, ofilfNExist, H()) > 0) {
			;
		}
		else
			skip = 1;
	}
	if(!skip) {
		const  uint8 * p_buf = reinterpret_cast<const uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER, 0, 0)); // to get characters directly from Scintilla buffer;
		const  size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
		SFile file;
		THROW_SL(file.Open(path, SFile::mWrite|SFile::mBinary));
		if(Doc.OrgCp == Doc.Cp) {
			THROW_SL(file.Write(p_buf, len));
		}
		else if(Doc.Cp == cpUTF8) {
			SString temp_buf;
			temp_buf.CatN(reinterpret_cast<const char *>(p_buf), len);
			temp_buf.Utf8ToCp(Doc.OrgCp);
			THROW_SL(file.Write(temp_buf, temp_buf.Len()));
		}
		else {
			THROW_SL(file.Write(p_buf, len));
		}
		Doc.FileName = path;
		Doc.SetState(Document::stDirty, 0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int STextBrowser::CmpFileName(const char * pFileName)
{
	return Doc.FileName.Cmp(pFileName, 1);
}

int STextBrowser::BraceHtmlTag()
{
	int    ok = -1;
	SString tag, text;
	TDialog * dlg = new TDialog(DLG_SELHTMLTAG);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
			if(tag.NotEmptyS()) {
				IntRange sel_range;
				GetSelection(sel_range);
				if(sel_range.low >= 0 && sel_range.upp >= 0) {
					if(tag == "*") { // comment
						text.Z().Cat("-->");
						CallFunc(SCI_INSERTTEXT, sel_range.upp, reinterpret_cast<intptr_t>(text.cptr()));
						text.Z().Cat("<!--");
						CallFunc(SCI_INSERTTEXT, sel_range.low, reinterpret_cast<intptr_t>(text.cptr()));
					}
					else {
						text.Z().CatChar('<').CatChar('/').Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.upp, reinterpret_cast<intptr_t>(text.cptr()));
						text.Z().CatChar('<').Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.low, reinterpret_cast<intptr_t>(text.cptr()));
					}
					ok = 1;
				}
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
