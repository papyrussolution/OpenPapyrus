// PPTEX2HTML.CPP
// Copyright (c) A.Sobolev 2014, 2015, 2016, 2018, 2019, 2020, 2021
//
#include <pp.h>
#pragma hdrstop

class PPTex2HtmlPrcssr {
public:
	struct Param : public PPBaseFilt {
		Param();
		Param & FASTCALL operator = (const Param & rS);
		int    GetExtStrData(int fldID, SString & rBuf) const;
		int    PutExtStrData(int fldID, const char * pBuf);

		enum {
			fDivideByChapter  = 0x0001,
			fAttachToWorkbook = 0x0002,
			fUtf8Codepage     = 0x0004
		};
		enum {
			exsInputFileName = 1,
			exsInputPictPath,
			exsOutputFileName,
			exsOutputPictPath,
			exsAnchorPrefix,
			exsWbLinkTemplate,
			exsWbPicLinkTemplate
		};
		uint8  ReserveStart[32];
		long   Flags;
		PPID   ParentWbID;     // Родительский элемент для привязки страниц к рабочим книгам
		PPID   ParentPicWbID;  // Родительский элемент для привязки изображений к рабочим книгам
		SString ExtString;     // @anchor
	};
	static int EditParam(Param * pParam);

	PPTex2HtmlPrcssr();
	~PPTex2HtmlPrcssr();
	int    SetParam(const Param * pParam);
	int    Run();
	int    Debug_Output(const char * pOutputFileName);
private:
	enum {
		rtmAll = 0,
		rtmParagraph,
		rtmCommand,
		rtmArgBrc,
		rtmArgBrk,
		rtmFormula,
		rtmFormula2,
		rtmFormulaBody
	};
	struct TextBlock {
		enum {
			tParagraph    = 1,
			tCommand      = 2,
			tContinuation = 3
		};
		TextBlock(int type, uint lineNo);
		~TextBlock();
		const char * GetInnerLabel() const;
		const char * GetInnerText() const;

		int    Type;
		long   Flags;
		uint   LineNo;        // @v11.0.10 Номер строки, с которой начинается блок
		SString Text;         // Если (Flags & fCommand) то - текст команды
		TextBlock * P_ArgBrc; // Список командных аргументов {}
		TextBlock * P_ArgBrk; // Список командных аргументов []
		TextBlock * P_Next;
	};
	struct StateBlock {
		StateBlock();
		int    Init(size_t sz);
		int    ResetPreprocess();
		int    FASTCALL GetChr(uint _pos) const;
		int    FASTCALL GetCurChr() const;
		uint   IsEol() const;

		struct OutPart {
			OutPart();

			uint   ChapterNo;
			SString Label;
			SString FileName;
			StringSet LinkSs;
            PPID   WbID;
		};
		struct PictItem {
			PictItem();

            SString OrgSymb;
            SString SrcFileName;
            SString DestFileName;
            PPID   WbID;
		};

		uint   LineNo;
		uint   InputSize;
		uint   ChapterNo; // Порядковый номер главы (\chapter)
		SEOLFormat Eolf;
		SBaseBuffer InputBuffer;
		SStrScan Scan;
		TSCollection <OutPart> OutPartList;
		TSCollection <PictItem> PictList;
	};
	enum _TexItemType {
		_texEnv = 1,
		_texCmd
	};
	enum {
		_thfSuppressPara = 0x0001, // Блокирует обрамление текста параграфом, если текст представлен одиночным блоком
		_thfDisablePara  = 0x0002, // Безусловно блокирует обрамление текста параграфом
		_thfEndPara      = 0x0004,
		_thfFormula      = 0x0008, // Режим формул
		_thfSingle       = 0x0010  // Обрабатывать только заданный блок, не обрабатывая последующие
	};
	struct _TexToHtmlEntry {
		int    Type;
		const  char * P_TexSymb;
		const  char * P_HtmlTag;
		long   Flags; // _thfXXX
	};
	struct _TexEnvItem {
		const  PPTex2HtmlPrcssr::TextBlock * P_StartBlk;
		const  _TexToHtmlEntry * P_ThEntry;
	};
	//
	// Descr: Флаги состояний вызова функции ReadText
	//
	enum {
		rtsVerbatim = 0x0001 // Текущий блок находится в verbatim-режиме
	};
	int    ReadText(long mode, long state, TextBlock * pBlk);
	int    WriteText(SFile & rF, const SString & rLineBuf);
	int    ReadCmd(TextBlock * pBlk);
	int    Helper_Debug_OutputTextBlock(const PPTex2HtmlPrcssr::TextBlock * pBlk, SFile & rF, int noPrefix);
	int    Helper_PreprocessOutput(const TextBlock * pBlk, long flags, TSStack <_TexEnvItem> & rEnvStack);
	int    Helper_Output(SFile & rOut, const TextBlock * pBlk, long flags, TSStack <_TexEnvItem> & rEnvStack);
	int    InitOutput(SFile * pOut, const char * pPartLabel, const char * pPartText, uint chapterNo);
	int    OutputStyles(SFile & rOut);
	const  _TexToHtmlEntry * SearchTexToHtmlEntry(int texType, const char * pTexSymb);
	int    Paragraph(SFile * pOut, int & rPara);
	const  char * OutputFormulaItem(const char * pText, SString & rOutText, int brace);
	int    ResolvePict(const char * pOrgSymb, const char * pName, uint * pPicListPos, SString & rRef);

	static _TexToHtmlEntry __TexToHtmlList[];

	StateBlock St;
	uint   LastSymbId;
	Param  P;
	TextBlock * P_Head;
	PPObjWorkbook WbObj;
};

PPTex2HtmlPrcssr::Param::Param() : PPBaseFilt(PPFILT_TEX2HTMLPARAM, 0, 0)
{
	SetFlatChunk(offsetof(Param, ReserveStart), offsetof(Param, ExtString)-offsetof(Param, ReserveStart));
	SetBranchSString(offsetof(Param, ExtString));
	Init(1, 0);
}

PPTex2HtmlPrcssr::Param & FASTCALL PPTex2HtmlPrcssr::Param::operator = (const PPTex2HtmlPrcssr::Param & rS)
{
	Copy(&rS, 1);
	return *this;
}

int PPTex2HtmlPrcssr::Param::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPTex2HtmlPrcssr::Param::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }
//
//
//
PPTex2HtmlPrcssr::TextBlock::TextBlock(int type, uint lineNo) : Type(type), Flags(0), P_ArgBrc(0), P_ArgBrk(0), P_Next(0), LineNo(lineNo)
{
}

PPTex2HtmlPrcssr::TextBlock::~TextBlock()
{
	ZDELETE(P_ArgBrc);
	ZDELETE(P_ArgBrk);
	{
		//
		// Чтобы не попасть на переполнение стека избегаем рекурсии переносом
		// всей цепочки элементов в линейный массив с последующим удалением.
		//
		SArray ptr_list(sizeof(void *), /*128,*/aryDataOwner|aryPtrContainer);
		{
			for(TextBlock * p = P_Next; p; p = p->P_Next) {
				ptr_list.insert(p);
			}
		}
		{
			uint c = ptr_list.getCount();
			if(c) do {
				TextBlock * p = static_cast<TextBlock *>(ptr_list.at(--c));
				p->P_Next = 0;
				delete p;
			} while(c);
		}
	}
}

const char * PPTex2HtmlPrcssr::TextBlock::GetInnerLabel() const
{
	const char * p_label = 0;
	if(P_ArgBrc) {
		const TextBlock * p_next = P_ArgBrc->P_Next;
		if(p_next && p_next->Type == TextBlock::tCommand && sstreq(p_next->Text, "label") && p_next->P_ArgBrc)
			p_label = p_next->P_ArgBrc->Text;
	}
	return p_label;
}

const char * PPTex2HtmlPrcssr::TextBlock::GetInnerText() const
{
	return P_ArgBrc ? P_ArgBrc->Text.cptr() : 0;
}
//
//
//
PPTex2HtmlPrcssr::StateBlock::OutPart::OutPart() : ChapterNo(0), WbID(0)
{
}

PPTex2HtmlPrcssr::StateBlock::PictItem::PictItem() : WbID(0)
{
}

PPTex2HtmlPrcssr::StateBlock::StateBlock() : ChapterNo(0), LineNo(0), InputSize(0)
{
	InputBuffer.Init();
}

int PPTex2HtmlPrcssr::StateBlock::Init(size_t sz)
{
	int    ok = 1;
	ChapterNo = 0;
	LineNo = 0;
	InputSize = 0;
	InputBuffer.Destroy();
	Eolf = eolUndef;
	OutPartList.freeAll();
	PictList.freeAll();
	if(!InputBuffer.Alloc(sz+128))
		ok = 0;
	return ok;
}

int PPTex2HtmlPrcssr::StateBlock::ResetPreprocess()
{
	int    ok = 1;
	ChapterNo = 0;
	LineNo = 0;
	InputSize = 0;
	InputBuffer.Destroy();
	Eolf = eolUndef;
	return ok;
}

int FASTCALL PPTex2HtmlPrcssr::StateBlock::GetChr(uint _pos) const
{
	return (_pos < InputSize) ? InputBuffer.P_Buf[_pos] : 0;
}

int FASTCALL PPTex2HtmlPrcssr::StateBlock::GetCurChr() const
{
	return Scan[0];
}

uint PPTex2HtmlPrcssr::StateBlock::IsEol() const
{
	return Scan.IsEol(Eolf);
}
//
//
//
PPTex2HtmlPrcssr::PPTex2HtmlPrcssr() : LastSymbId(0), P_Head(0)
{
}

PPTex2HtmlPrcssr::~PPTex2HtmlPrcssr()
{
	delete P_Head;
}

int PPTex2HtmlPrcssr::SetParam(const Param * pParam)
{
	return RVALUEPTR(P, pParam) ? 1 : -1;
}

int PPTex2HtmlPrcssr::WriteText(SFile & rF, const SString & rLineBuf)
{
    if(P.Flags & Param::fUtf8Codepage) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = rLineBuf).ToUtf8();
		return rF.WriteLine(r_temp_buf) ? 1 : PPSetErrorSLib();
    }
	else
		return rF.WriteLine(rLineBuf) ? 1 : PPSetErrorSLib();
}

int PPTex2HtmlPrcssr::ReadText(long mode, long state, TextBlock * pText)
{
	int    ok = -1;
	const  size_t prev_offs = St.Scan.Offs;
	TextBlock * p_current_blk = pText;
	long   inner_state = state;
	if(oneof2(mode, rtmFormula, rtmFormula2)) {
		if(mode == rtmFormula)
			p_current_blk->Text = "$";
		else // rtmFormula2
			p_current_blk->Text = "$$";
		TextBlock * p_arg = new TextBlock(TextBlock::tParagraph, St.LineNo);
		THROW_MEM(p_arg);
		THROW(ReadText(rtmFormulaBody, inner_state, p_arg)); // @recursion
		{
			if(p_current_blk->P_ArgBrc == 0)
				p_current_blk->P_ArgBrc = p_arg;
			else {
				// @error (в формуле только один аргумент)
				TextBlock * p_cur_arg = p_current_blk->P_ArgBrc;
				while(p_cur_arg->P_Next)
					p_cur_arg = p_cur_arg->P_Next;
				p_cur_arg->P_Next = p_arg;
			}
		}
	}
	else if(mode == rtmCommand) {
		if(St.Scan.GetIdent(p_current_blk->Text)) {
			// @debug {
			int    debug_break = 0;
			if(p_current_blk->Text == "sum")
				debug_break = 1;
			// } @debug
			for(int read_next_arg = 1; read_next_arg != 0;) {
				const uint preserve_scan_offs = St.Scan.Offs;
				St.Scan.Skip();
				uint _eol = St.IsEol();
				if(_eol) {
					St.LineNo++;
					St.Scan.Incr(_eol);
					St.Scan.Skip();
				}
				const char c = St.GetCurChr();
				if(c == '{') {
					St.Scan.Incr();
					TextBlock * p_arg = new TextBlock(TextBlock::tParagraph, St.LineNo);
					THROW_MEM(p_arg);
					THROW(ReadText(rtmArgBrc, inner_state, p_arg)); // @recursion
					// @debug {
					if(p_arg->Text == "Verbatim") {
						if(p_current_blk->Text == "begin")
							inner_state |= rtsVerbatim;
						else if(p_current_blk->Text == "end")
							inner_state &= ~rtsVerbatim;
						debug_break = 1;
					}
					// } @debug
					{
						if(p_current_blk->P_ArgBrc == 0)
							p_current_blk->P_ArgBrc = p_arg;
						else {
							TextBlock * p_cur_arg = p_current_blk->P_ArgBrc;
							while(p_cur_arg->P_Next)
								p_cur_arg = p_cur_arg->P_Next;
							p_cur_arg->P_Next = p_arg;
						}
					}
				}
				else if(c == '[') {
					St.Scan.Incr();
					TextBlock * p_arg = new TextBlock(TextBlock::tParagraph, St.LineNo);
					THROW_MEM(p_arg);
					THROW(ReadText(rtmArgBrk, inner_state, p_arg)); // @recursion
					{
						if(p_current_blk->P_ArgBrk == 0)
							p_current_blk->P_ArgBrk = p_arg;
						else {
							TextBlock * p_cur_arg = p_current_blk->P_ArgBrk;
							while(p_cur_arg->P_Next)
								p_cur_arg = p_cur_arg->P_Next;
							p_cur_arg->P_Next = p_arg;
						}
					}
				}
				else {
					St.Scan.Offs = preserve_scan_offs;
					read_next_arg = 0;
				}
			}
		}
		else {
			p_current_blk->Text = "unkncommand";
		}
	}
	else {
		while(ok < 0 && St.Scan.Offs < St.InputSize) {
			uint _eol_count = 0;
			uint _eol = 0;
			while((_eol = St.IsEol()) != 0) {
				St.LineNo++;
				St.Scan.Incr(_eol);
				_eol_count++;
			}
			if(_eol_count > 1) {
				if(mode == rtmAll) {
					TextBlock * p_para = new TextBlock(TextBlock::tParagraph, St.LineNo);
					THROW_MEM(p_para);
					THROW(ReadText(rtmParagraph, inner_state, p_para)); // @recursion
					{
						p_current_blk->P_Next = p_para;
						p_current_blk = p_para;
						while(p_current_blk->P_Next)
							p_current_blk = p_current_blk->P_Next;
					}
				}
				else if(mode == rtmParagraph) {
					ok = 1;
				}
				else if(mode == rtmCommand) {
					ok = 1;
				}
			}
			else if(_eol_count == 1) {
				p_current_blk->Text.CatChar('\n');
			}
			else {
				const char c = St.GetCurChr();
				if(c == '%') {
					do {
						St.Scan.Incr();
					} while(!St.IsEol());
				}
				else if(c == ']') {
					St.Scan.Incr();
					if(mode == rtmArgBrk) {
						ok = 1;
					}
					else {
						// @error Unexpected ']'
						p_current_blk->Text.CatChar(c);
					}
				}
				else if(c == '}') {
					St.Scan.Incr();
					if(mode == rtmArgBrc) {
						ok = 1;
					}
					else {
						// @error Unexpected '}'
						p_current_blk->Text.CatChar(c);
					}
				}
				else if(c == '$') {
					St.Scan.Incr();
					const char c_next = St.GetCurChr();
					if(c_next == '$')
						St.Scan.Incr();
					if(mode == rtmFormulaBody) {
						ok = 1;
					}
					else {
						TextBlock * p_cmd = new TextBlock(TextBlock::tCommand, St.LineNo);
						THROW_MEM(p_cmd);
						THROW(ReadText((c_next == '$') ? rtmFormula2 : rtmFormula, inner_state, p_cmd)); // @recursion
						p_current_blk->P_Next = p_cmd;
						p_current_blk = p_cmd;
						{
							TextBlock * p_para = new TextBlock(TextBlock::tContinuation, St.LineNo);
							THROW_MEM(p_para);
							p_current_blk->P_Next = p_para;
							p_current_blk = p_para;
						}
					}
				}
				else if(c == '~') {
					St.Scan.Incr();
					p_current_blk->Text.Cat("&nbsp;");
				}
				else if(c == '#') {
					St.Scan.Incr();
					p_current_blk->Text.Cat("&#035;");
				}
				else if(c == '\\') {
					St.Scan.Incr();
					const char c_next = St.GetCurChr();
					const char * p_literal = "%{}[]_$^&"; // @v11.0.10 ^&
					if(c_next == '\\') {
						St.Scan.Incr();
						p_current_blk->Text.CatTagBrace("br", 0);
					}
					if(sstrchr(p_literal, c_next)) {
						St.Scan.Incr();
						p_current_blk->Text.CatChar(c_next);
					}
					else {
						TextBlock * p_cmd = new TextBlock(TextBlock::tCommand, St.LineNo);
						THROW_MEM(p_cmd);
						THROW(ReadText(rtmCommand, inner_state, p_cmd)); // @recursion
						p_current_blk->P_Next = p_cmd;
						p_current_blk = p_cmd;
						{
							TextBlock * p_para = new TextBlock(TextBlock::tContinuation, St.LineNo);
							THROW_MEM(p_para);
							p_current_blk->P_Next = p_para;
							p_current_blk = p_para;
						}
					}
				}
				else {
					St.Scan.Incr();
					p_current_blk->Text.CatChar(c);
				}
			}
		}
	}
	if(St.Scan.Offs > prev_offs) {
		PPWaitPercent(St.Scan.Offs, St.InputSize, "Reading");
	}
	CATCHZOK
	return ok;
}

int PPTex2HtmlPrcssr::Helper_Debug_OutputTextBlock(const PPTex2HtmlPrcssr::TextBlock * pBlk, SFile & rF, int noPrefix)
{
	int    ok = 1;
	SString line_buf;
	if(pBlk->Type == TextBlock::tParagraph) {
		if(!noPrefix)
			(line_buf = ":paragraph").Tab();
		else
			line_buf.Z();
		line_buf.Cat(pBlk->Text).CR();
		WriteText(rF, line_buf);
	}
	else if(pBlk->Type == TextBlock::tContinuation) {
		if(!noPrefix)
			(line_buf = ":continuation").Tab();
		else
			line_buf.Z();
		line_buf.Cat(pBlk->Text).CR();
		WriteText(rF, line_buf);
	}
	else if(pBlk->Type == TextBlock::tCommand) {
		if(!noPrefix)
			(line_buf = ":command").Tab();
		else
			line_buf.Z();
		line_buf.Cat(pBlk->Text).CR();
		WriteText(rF, line_buf);
		if(pBlk->P_ArgBrk) {
			for(TextBlock * p_arg = pBlk->P_ArgBrk; p_arg; p_arg = p_arg->P_Next) {
				if(p_arg->Text.Len() <= 80) {
					WriteText(rF, line_buf.Z().CatBrackStr(p_arg->Text).CR());
				}
				else {
					WriteText(rF, line_buf.Z().CatChar('[').CR());
					THROW(Helper_Debug_OutputTextBlock(p_arg, rF, 1));
					WriteText(rF, line_buf.Z().CatChar(']').CR());
				}
			}
		}
		if(pBlk->P_ArgBrc) {
			for(TextBlock * p_arg = pBlk->P_ArgBrc; p_arg; p_arg = p_arg->P_Next) {
				if(p_arg->Text.Len() <= 80) {
					WriteText(rF, line_buf.Z().CatChar('{').Cat(p_arg->Text).CatChar('}').CR());
				}
				else {
					WriteText(rF, line_buf.Z().CatChar('{').CR());
					THROW(Helper_Debug_OutputTextBlock(p_arg, rF, 1));
					WriteText(rF, line_buf.Z().CatChar('}').CR());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPTex2HtmlPrcssr::Debug_Output(const char * pOutputFileName)
{
	int    ok = 1;
	if(P_Head) {
		long   blk_count = 0;
		long   blk_n = 0;
		SFile f_out(pOutputFileName, SFile::mWrite);
		THROW_SL(f_out.IsValid());
		{
			for(TextBlock * p_blk = P_Head; p_blk; p_blk = p_blk->P_Next) {
				blk_count++;
			}
		}
		{
			for(TextBlock * p_blk = P_Head; p_blk; p_blk = p_blk->P_Next) {
				THROW(Helper_Debug_OutputTextBlock(p_blk, f_out, 0));
				PPWaitPercent(++blk_n, blk_count, "Output Debug");
			}
		}
	}
	CATCHZOK
	return ok;
}

/*static*/PPTex2HtmlPrcssr::_TexToHtmlEntry PPTex2HtmlPrcssr::__TexToHtmlList[] = {
	{ PPTex2HtmlPrcssr::_texEnv, "__default",   "p", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "document",    "body", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "itemize",     "dl", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "description", "dl", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "enumerate",   "ol", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "Verbatim",    "pre", 0 },
	{ PPTex2HtmlPrcssr::_texEnv, "lstlisting",  "pre", 0 }, // @v10.8.2

	{ PPTex2HtmlPrcssr::_texCmd, "__default", "p", 0 },
	{ PPTex2HtmlPrcssr::_texCmd, "chapter", "h1", _thfSuppressPara|_thfEndPara },
	{ PPTex2HtmlPrcssr::_texCmd, "section", "h2", _thfSuppressPara|_thfEndPara },
	{ PPTex2HtmlPrcssr::_texCmd, "subsection", "h3", _thfSuppressPara|_thfEndPara },
	{ PPTex2HtmlPrcssr::_texCmd, "subsubsection", "h4", _thfSuppressPara|_thfEndPara },
	{ PPTex2HtmlPrcssr::_texCmd, "paragraph", "h5", _thfSuppressPara|_thfEndPara }
};

const PPTex2HtmlPrcssr::_TexToHtmlEntry * PPTex2HtmlPrcssr::SearchTexToHtmlEntry(int texType, const char * pTexSymb)
{
	const _TexToHtmlEntry * p_entry = 0;
	for(uint i = 0; !p_entry && i < SIZEOFARRAY(__TexToHtmlList); i++) {
		if(__TexToHtmlList[i].Type == texType && stricmp(__TexToHtmlList[i].P_TexSymb, pTexSymb) == 0) {
			p_entry = __TexToHtmlList+i;
		}
	}
	return p_entry;
}

int PPTex2HtmlPrcssr::Paragraph(SFile * pOut, int & rPara)
{
	int    ok = 1;
	if(rPara) {
		if(pOut) {
			SString & r_line_buf = SLS.AcquireRvlStr();
			WriteText(*pOut, r_line_buf.CatTagBrace("p", 1).CR());
		}
		rPara = 0;
	}
	else {
		if(pOut) {
			SString & r_line_buf = SLS.AcquireRvlStr();
			WriteText(*pOut, r_line_buf.CatTagBrace("p", 0).CR());
		}
		rPara = 1;
	}
	return ok;
}

const char * PPTex2HtmlPrcssr::OutputFormulaItem(const char * pText, SString & rOutText, int brace)
{
	const char * p = pText;
	if(p) {
		while(*p != 0) {
			if(*p == '_') {
				p++;
				rOutText.CatTagBrace("sub", 0).CatTagBrace("small", 0);
				if(*p == '{')
					p = OutputFormulaItem(++p, rOutText, 1); // @recursion
				else
					rOutText.CatChar(*p++);
				rOutText.CatTagBrace("small", 1).CatTagBrace("sub", 1);
			}
			else if(*p == '^') {
				p++;
				rOutText.CatTagBrace("sup", 0).CatTagBrace("small", 0);
				if(*p == '{')
					p = OutputFormulaItem(++p, rOutText, 1); // @recursion
				else
					rOutText.CatChar(*p++);
				rOutText.CatTagBrace("small", 1).CatTagBrace("sup", 1);
			}
			else if(*p == '}') {
				if(brace) {
					p++;
					break;
				}
				else
					rOutText.CatChar(*p++);
			}
			else
				rOutText.CatChar(*p++);
		}
	}
	return p;
}

int PPTex2HtmlPrcssr::Helper_PreprocessOutput(const TextBlock * pBlk, long flags, TSStack <_TexEnvItem> & rEnvStack)
{
	int    ok = 1;
	int    paragraph = 0;   // Признак того, что был начат параграф
	int    list_item = 0;   // Признак того, что был начат 'лемент списка. Если list_item == 2, то <dd>, если list_item == 1, то <li>
	//int    description = 0; // Признак того, что был начат <dd>
	SString temp_buf, line_buf, file_name_buf, temp_buf2;
	//TSStack <_TexEnvItem> env_stack;
	SString in_pic_path, out_pic_path, out_file_name;
	P.GetExtStrData(Param::exsInputPictPath, in_pic_path);
	P.GetExtStrData(Param::exsOutputFileName, out_file_name);
	P.GetExtStrData(Param::exsOutputPictPath, out_pic_path);
	uint   blk_count = 0; // Отладочный счетчик блоков
	for(const TextBlock * p_blk = pBlk; p_blk; p_blk = p_blk->P_Next) {
		blk_count++;
		if(p_blk->Type == TextBlock::tCommand) {
			int    env_tag = 0;
			if(p_blk->Text == "begin")
				env_tag = 1;
			else if(p_blk->Text == "end")
				env_tag = 2;
			const TextBlock * p_first_brc_arg = p_blk->P_ArgBrc;
			const TextBlock * p_arg = p_blk->P_ArgBrc;
			if(p_arg) {
				do {
					p_arg = p_arg->P_Next;
				} while(p_arg);
			}
			if(env_tag == 1) {
				const _TexToHtmlEntry * p = p_first_brc_arg ? SearchTexToHtmlEntry(_texEnv, p_first_brc_arg->Text) : 0;
				if(p) {
					if(paragraph)
						Paragraph(0, paragraph);
				}
				_TexEnvItem env_item;
				env_item.P_StartBlk = p_blk;
				env_item.P_ThEntry = p;
				rEnvStack.push(env_item);
			}
			else if(env_tag == 2) {
				_TexEnvItem env_item;
				THROW_PP(rEnvStack.getPointer(), PPERR_TEX2H_ENVSTACKFAULT);
				rEnvStack.pop(env_item);
				THROW_PP(p_first_brc_arg, PPERR_TEX2H_ENVWITHOUTBRCARG);
				if(list_item) {
					//
					// Закрываем <dt></dt><dd>...
					//
					if(p_first_brc_arg->Text.IsEqiAscii("description") || p_first_brc_arg->Text.IsEqiAscii("itemize") || p_first_brc_arg->Text.IsEqiAscii("enumerate")) {
						list_item = 0;
					}
				}
				if(env_item.P_StartBlk && env_item.P_StartBlk->P_ArgBrc && env_item.P_StartBlk->P_ArgBrc->Text.CmpNC(p_first_brc_arg->Text) == 0) {
				}
				else {
					; // @error
				}
			}
			else {
				if(oneof2(p_blk->Text, "ppypict", "ppypictsc")) {
					if(p_first_brc_arg) {
						if(IsDirectory(in_pic_path) && IsDirectory(out_pic_path)) {
							(temp_buf = in_pic_path).SetLastSlash().Cat(p_first_brc_arg->Text).DotCat("png");
							if(fileExists(temp_buf)) {
								(file_name_buf = out_pic_path).SetLastSlash().Cat(p_first_brc_arg->Text).DotCat("png");
								if(copyFileByName(temp_buf, file_name_buf)) {
									SPathStruc::GetRelativePath(out_file_name, 0, file_name_buf, 0, line_buf);
									temp_buf2.Z().Cat("pic").CatChar('-').Cat(p_first_brc_arg->Text);
									file_name_buf.Z().CatQStr(temp_buf2);
									temp_buf2.Z().CatChar('<').Cat("a").Space().CatEq("name", file_name_buf).CatChar('>');
									{
										temp_buf.Z().CatQStr(line_buf);
										line_buf.Z().Cat(temp_buf2).CatChar('<').Cat("img").Space().CatEq("src", temp_buf);
										// line_buf.Space().CatEq("align", temp_buf.Z().CatQStr("left"));
										if(p_first_brc_arg->P_Next) {
											if(p_first_brc_arg->P_Next->Text.NotEmpty()) {
												temp_buf.Z().CatQStr(p_first_brc_arg->P_Next->Text);
												line_buf.Space().CatEq("alt", temp_buf).Space().CatEq("title", temp_buf);
											}
											if(p_first_brc_arg->P_Next->P_Next && p_first_brc_arg->P_Next->P_Next->Text.NotEmpty()) {
												/*
												double sc = p_first_brc_arg->P_Next->P_Next->Text.ToReal();
												if(sc > 0.0) {
													temp_buf.Z().CatChar('\"').Cat(sc * 100.0, MKSFMTD(0, 0, 0)).CatChar('%').CatChar('\"');
													line_buf.Space().CatEq("width", temp_buf);
													line_buf.Space().CatEq("height", temp_buf);
												}
												*/
											}
										}
										line_buf.CatChar('>');
									}
									//line_buf.CatTagBrace("a", 1);
									//WriteText(rOut, line_buf);
								}
							}
						}
					}
				}
				else if(p_blk->Text == "$") { // formula
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, _thfDisablePara|_thfFormula, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "$$") { // formula
					if(p_first_brc_arg) {
						for(const TextBlock * p_blk_ = p_first_brc_arg; p_blk_; p_blk_ = p_blk_->P_Next) {
							THROW(Helper_PreprocessOutput(p_blk_, _thfFormula|_thfSingle, rEnvStack)); // @recursion
						}
					}
				}
				else if(p_blk->Text == "frac") {
					if(p_first_brc_arg) {
						long _thf = _thfDisablePara|_thfSingle;
						if(flags & _thfFormula)
							_thf |= _thfFormula;
						THROW(Helper_PreprocessOutput(p_first_brc_arg, _thf, rEnvStack)); // @recursion
						if(p_first_brc_arg->P_Next) {
							THROW(Helper_PreprocessOutput(p_first_brc_arg->P_Next, _thf, rEnvStack)); // @recursion
						}
					}
				}
				else if(p_blk->Text == "underline") {
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "trademark") {
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "ppymenu") {
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "ppynote") {
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, 0, rEnvStack)); // @recursion
					}
				}
				else if(oneof2(p_blk->Text, "ppyexample", "ppyexampletitle")) {
					if(p_first_brc_arg) {
						THROW(Helper_PreprocessOutput(p_first_brc_arg, 0, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "label") {
					if(p_first_brc_arg) {
						if(St.ChapterNo) {
							for(uint i = 0; i < St.OutPartList.getCount(); i++) {
								StateBlock::OutPart * p = St.OutPartList.at(i);
								if(p && p->ChapterNo == St.ChapterNo) {
									temp_buf = p_first_brc_arg->Text;
									p->LinkSs.add(temp_buf);
									break;
								}
							}
						}
					}
				}
				else if(oneof2(p_blk->Text, "ref", "pageref")) {
					if(p_first_brc_arg) {
						//temp_buf.Z().CatChar('\"').CatChar('#').Cat(p_first_brc_arg->Text).CatChar('\"');
						//line_buf.Z().CatChar('<').Cat("a").Space().CatEq("href", temp_buf).CatChar('>').Cat("link").CatTagBrace("a", 1);
						//WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "item") {
					if(list_item)
						list_item = 0;
					if(p_blk->P_ArgBrk) {
						THROW(Helper_PreprocessOutput(p_blk->P_ArgBrk, _thfSuppressPara, rEnvStack)); // @recursion
						list_item = 2;
					}
					else
						list_item = 1;
				}
				else {
					const _TexToHtmlEntry * p = SearchTexToHtmlEntry(_texCmd, p_blk->Text);
					if(p && sstreq(p->P_TexSymb, "chapter")) {
						const char * p_chapter_symb = p_blk->GetInnerLabel();
						const char * p_chapter_text = p_blk->GetInnerText();
						St.ChapterNo++;
						if(P.Flags & P.fDivideByChapter) {
							THROW(InitOutput(0, p_chapter_symb, p_chapter_text, St.ChapterNo));
						}
					}
					if(p_first_brc_arg) {
						const long thf = p ? p->Flags : 0;
						if(p && p->P_HtmlTag) {
							if(thf & _thfEndPara && paragraph)
								Paragraph(0, paragraph);
						}
						THROW(Helper_PreprocessOutput(p_first_brc_arg, thf, rEnvStack)); // @recursion
					}
				}
			}
		}
		else if(p_blk->Type == TextBlock::tParagraph) {
			if(paragraph)
				Paragraph(0, paragraph);
			if(!(flags & _thfDisablePara) && (!(flags & _thfSuppressPara) || p_blk->P_Next))
				Paragraph(0, paragraph);
		}
		if(flags & _thfSingle)
			break;
	}
	if(paragraph)
		Paragraph(0, paragraph);
	CATCHZOK
	return ok;
}

int PPTex2HtmlPrcssr::ResolvePict(const char * pOrgSymb, const char * pName, uint * pPicListPos, SString & rRef)
{
	rRef.Z();

	int    ok = 0;
	uint   pos = 0;
	const  StateBlock::PictItem * p_result = 0;
	WorkbookTbl::Rec wb_rec;
	SString in_pic_path, out_pic_path, out_file_name, temp_buf, anchor_prefix;
	SString org_symb(pOrgSymb);
	org_symb.Strip().ToLower();
	if(org_symb.NotEmpty()) {
		P.GetExtStrData(Param::exsInputPictPath, in_pic_path);
		P.GetExtStrData(Param::exsOutputFileName, out_file_name);
		P.GetExtStrData(Param::exsOutputPictPath, out_pic_path);
		P.GetExtStrData(Param::exsAnchorPrefix, anchor_prefix);
		for(uint i = 0; i < St.PictList.getCount(); i++) {
			if(St.PictList.at(i)->OrgSymb == org_symb) {
				pos = i;
				p_result = St.PictList.at(pos);
				ok = 1;
			}
		}
		if(!ok) {
			StateBlock::PictItem * p_new_item = new StateBlock::PictItem;
			THROW_MEM(p_new_item);
			p_new_item->OrgSymb = org_symb;

			if(IsDirectory(in_pic_path) && IsDirectory(out_pic_path)) {
				(temp_buf = in_pic_path).SetLastSlash().Cat(org_symb).DotCat("png");
				if(fileExists(temp_buf)) {
					SString file_name_buf;
					p_new_item->SrcFileName = temp_buf;
					(file_name_buf = out_pic_path).SetLastSlash().Cat(org_symb).DotCat("png");
					if(copyFileByName(temp_buf, file_name_buf)) {
						p_new_item->DestFileName = file_name_buf;
						if(P.Flags & Param::fAttachToWorkbook) {
							SString wb_symb;
							wb_symb.Cat(anchor_prefix).Cat(org_symb);
							PPID   wb_id = 0;
                            int r = WbObj.SearchByLongSymb(wb_symb, &wb_id, &wb_rec);
                            if(r > 0) {
								// @todo Если r == 2, то выдать сообщение, что есть более одной записи с заданным символом
                            	p_new_item->WbID = wb_id;
                            }
                            else {
                            	PPWorkbookPacket wb_pack;
                            	if(isempty(pName)) {
									wb_symb.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
                            	}
                            	else {
									STRNSCPY(wb_pack.Rec.Name, pName);
                            	}
                            	SCharToOem(wb_pack.Rec.Name);
								{
									PPID   dup_id = 0;
									if(WbObj.SearchByName(wb_pack.Rec.Name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID) {
										const  size_t max_nm_len = sizeof(wb_pack.Rec.Name)-1;
										PPID   temp_nm_id = 0;
										long   uc = 1;
										SString suffix, wb_name;
										do {
											(suffix = 0).Space().CatChar('#').Cat(++uc);
											wb_name = wb_pack.Rec.Name;
											size_t sum_len = wb_name.Len() + suffix.Len();
											if(sum_len > max_nm_len)
												wb_name.Trim(max_nm_len-suffix.Len());
											wb_name.Cat(suffix);
										} while(WbObj.SearchByName(wb_name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID);
										wb_name.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
									}
								}
                                wb_pack.Rec.Type = PPWBTYP_MEDIA;
                                wb_pack.Rec.ParentID = P.ParentPicWbID;
                                wb_pack.SetLongSymb(wb_symb);
								{
									THROW(WbObj.MakeUniqueCode(temp_buf.Z(), 0));
									temp_buf.CopyTo(wb_pack.Rec.Symb, sizeof(wb_pack.Rec.Symb));
								}
                                THROW(WbObj.PutPacket(&(wb_id = 0), &wb_pack, 0));
                            }
						}
						THROW_SL(St.PictList.insert(p_new_item));
						pos = St.PictList.getCount()-1;
						p_result = St.PictList.at(pos);
						ok = 1;
					}
				}
			}
		}
	}
	if(p_result) {
		SString rel_path;
		if(P.Flags & Param::fAttachToWorkbook) {
            if(p_result->WbID && WbObj.Search(p_result->WbID, &wb_rec) > 0) {
            	SString tmpl_buf;
                P.GetExtStrData(Param::exsWbPicLinkTemplate, tmpl_buf);
                // <img src="/dispatcher/workbook/content?code=FIG-PRCBUSY" height="265" width="500">
				if(tmpl_buf.NotEmptyS()) {
					for(uint j = 0; j < tmpl_buf.Len(); j++) {
						const char c = tmpl_buf.C(j);
						if(c != '@')
							rel_path.CatChar(c);
						else
							rel_path.Cat(wb_rec.Symb);
					}
				}
				else {
                    rel_path.CatChar('/').Cat("dispatcher").CatChar('/').Cat("workbook").CatChar('/').Cat("content").CatChar('?').CatEq("code", wb_rec.Symb);
				}
            }
		}
		else {
			SPathStruc::GetRelativePath(out_file_name, 0, p_result->DestFileName, 0, rel_path);
		}
		if(rel_path.NotEmpty()) {
			temp_buf.Z().Cat("pic").CatChar('-').Cat(org_symb);
			rRef.CatChar('<').Cat("a").Space().CatEqQ("name", temp_buf).CatChar('>').CatChar('<').Cat("img").Space().CatEqQ("src", rel_path);
			// line_buf.Space().CatEq("align", temp_buf.Z().CatQStr("left"));
			if(!isempty(pName))
				rRef.Space().CatEqQ("alt", pName).Space().CatEqQ("title", pName);
			rRef.CatChar('>').CatTagBrace("a", 1);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPicListPos, pos);
	return ok;
}

int PPTex2HtmlPrcssr::Helper_Output(SFile & rOut, const TextBlock * pBlk, long flags, TSStack <_TexEnvItem> & rEnvStack)
{
	int    ok = 1;
	int    paragraph = 0;   // Признак того, что был начат параграф
	int    list_item = 0;   // Признак того, что был начат 'лемент списка. Если list_item == 2, то <dd>, если list_item == 1, то <li>
	//int    description = 0; // Признак того, что был начат <dd>
	SString temp_buf;
	SString line_buf;
	SString file_name_buf;
	SString temp_buf2;
	//TSStack <_TexEnvItem> env_stack;
	SString in_pic_path, out_pic_path, out_file_name;
	P.GetExtStrData(Param::exsInputPictPath, in_pic_path);
	P.GetExtStrData(Param::exsOutputFileName, out_file_name);
	P.GetExtStrData(Param::exsOutputPictPath, out_pic_path);
	for(const TextBlock * p_blk = pBlk; p_blk; p_blk = p_blk->P_Next) {
		if(p_blk->Type == TextBlock::tCommand) {
			int    env_tag = 0;
			if(p_blk->Text == "begin")
				env_tag = 1;
			else if(p_blk->Text == "end")
				env_tag = 2;
			const TextBlock * p_first_brc_arg = p_blk->P_ArgBrc;
			const TextBlock * p_arg = p_blk->P_ArgBrc;
			if(p_arg) {
				do {
					p_arg = p_arg->P_Next;
				} while(p_arg);
			}
			if(env_tag == 1) {
				const _TexToHtmlEntry * p = p_first_brc_arg ? SearchTexToHtmlEntry(_texEnv, p_first_brc_arg->Text) : 0;
				if(p) {
					if(paragraph)
						Paragraph(&rOut, paragraph);
					line_buf.Z().CatTagBrace(p->P_HtmlTag, 0).CR();
					WriteText(rOut, line_buf);
				}
				_TexEnvItem env_item;
				env_item.P_StartBlk = p_blk;
				env_item.P_ThEntry = p;
				rEnvStack.push(env_item);
			}
			else if(env_tag == 2) {
				_TexEnvItem env_item;
				THROW_PP(rEnvStack.getPointer(), PPERR_TEX2H_ENVSTACKFAULT);
				rEnvStack.pop(env_item);
				THROW_PP(p_first_brc_arg, PPERR_TEX2H_ENVWITHOUTBRCARG);
				if(list_item) {
					//
					// Закрываем <dt></dt><dd>...
					//
					if(p_first_brc_arg->Text.IsEqiAscii("description") || p_first_brc_arg->Text.IsEqiAscii("itemize") || p_first_brc_arg->Text.IsEqiAscii("enumerate")) {
						if(list_item == 2)
							WriteText(rOut, line_buf.Z().CatTagBrace("dd", 1));
						else
							WriteText(rOut, line_buf.Z().CatTagBrace("li", 1));
						list_item = 0;
					}
				}
				if(env_item.P_StartBlk && env_item.P_StartBlk->P_ArgBrc && env_item.P_StartBlk->P_ArgBrc->Text.CmpNC(p_first_brc_arg->Text) == 0) {
					if(env_item.P_ThEntry) {
						line_buf.Z().CatTagBrace(env_item.P_ThEntry->P_HtmlTag, 1).CR();
						WriteText(rOut, line_buf);
					}
				}
				else {
					; // @error
				}
			}
			else {
				if(p_blk->Text == "ppypict" || p_blk->Text == "ppypictsc") {
					if(p_first_brc_arg) {
						uint pic_list_pos = 0;
                        SString ref_buf;
						THROW(ResolvePict(p_first_brc_arg->Text, p_first_brc_arg->P_Next ? p_first_brc_arg->P_Next->Text.cptr() : 0, &pic_list_pos, ref_buf));
						if(ref_buf.NotEmpty())
							WriteText(rOut, ref_buf);
					}
				}
				else if(p_blk->Text == "ref" || p_blk->Text == "pageref") {
					if(p_first_brc_arg) {
						temp_buf.Z().CatChar('\"');
						uint i;
						const StateBlock::OutPart * p_this_part = 0;
						for(i = 0; i < St.OutPartList.getCount(); i++) {
							const StateBlock::OutPart * p = St.OutPartList.at(i);
							if(p && p->ChapterNo == St.ChapterNo) {
								p_this_part = p;
								break;
							}
						}
						if(p_this_part) {
							for(i = 0; i < St.OutPartList.getCount(); i++) {
								const StateBlock::OutPart * p = St.OutPartList.at(i);
								if(p && p->LinkSs.search(p_first_brc_arg->Text, 0, 0)) {
									line_buf.Z();
									if(P.Flags & Param::fAttachToWorkbook) {
										WorkbookTbl::Rec wb_rec;
										assert(p->WbID);
										if(p->WbID && WbObj.Search(p->WbID, &wb_rec) > 0) {
											SString tmpl_buf;
											P.GetExtStrData(Param::exsWbLinkTemplate, tmpl_buf);
											// <a href="?page=00571">Автоматизация розничной торговли в системе Papyrus</a>
											if(tmpl_buf.NotEmptyS()) {
												for(uint j = 0; j < tmpl_buf.Len(); j++) {
													const char c = tmpl_buf.C(j);
													if(c != '@')
														line_buf.CatChar(c);
													else
														line_buf.Cat(wb_rec.Symb);
												}
											}
											else {
												line_buf.CatChar('?').CatEq("page", wb_rec.Symb);
											}
										}
									}
									else {
										SPathStruc::GetRelativePath(p_this_part->FileName, 0, p->FileName, 0, line_buf);
									}
									temp_buf.Cat(line_buf);
								}
							}
						}
						temp_buf.CatChar('#').Cat(p_first_brc_arg->Text).CatChar('\"');
						line_buf.Z().CatChar('<').Cat("a").Space().CatEq("href", temp_buf).CatChar('>').Cat("link").CatTagBrace("a", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "$") { // formula
					if(p_first_brc_arg) {
						WriteText(rOut, line_buf.Z().CatTagBrace("i", 0).CatTagBrace("strong", 0));
						THROW(Helper_Output(rOut, p_first_brc_arg, _thfDisablePara|_thfFormula, rEnvStack)); // @recursion
						WriteText(rOut, line_buf.Z().CatTagBrace("strong", 1).CatTagBrace("i", 1));
					}
				}
				else if(p_blk->Text == "$$") { // formula
					if(p_first_brc_arg) {
						WriteText(rOut, line_buf.Z().CatTagBrace("i", 0).CatTagBrace("strong", 0));
						for(const TextBlock * p_blk_ = p_first_brc_arg; p_blk_; p_blk_ = p_blk_->P_Next) {
							THROW(Helper_Output(rOut, p_blk_, _thfFormula|_thfSingle, rEnvStack)); // @recursion
						}
						WriteText(rOut, line_buf.Z().CatTagBrace("strong", 1).CatTagBrace("i", 1));
					}
				}
				else if(p_blk->Text == "ppybrand") {
					WriteText(rOut, (line_buf = "Papyrus").Space());
				}
				else if(p_blk->Text == "newline") {
					WriteText(rOut, line_buf.Z().CatTagBrace("br", 0));
				}
				else if(p_blk->Text == "indent") {
					WriteText(rOut, line_buf.Z().Tab());
				}
				else if(p_blk->Text == "sum") {
					WriteText(rOut, line_buf = "&Sigma;");
				}
				else if(p_blk->Text == "frac") {
					if(p_first_brc_arg) {
						long _thf = _thfDisablePara|_thfSingle;
						if(flags & _thfFormula)
							_thf |= _thfFormula;
						THROW(Helper_Output(rOut, p_first_brc_arg, _thf, rEnvStack)); // @recursion
						WriteText(rOut, line_buf.Z().Space().CatChar('/').Space());
						if(p_first_brc_arg->P_Next) {
							THROW(Helper_Output(rOut, p_first_brc_arg->P_Next, _thf, rEnvStack)); // @recursion
						}
					}
				}
				else if(p_blk->Text == "underline") {
					if(p_first_brc_arg) {
						WriteText(rOut, line_buf.Z().CatTagBrace("u", 0));
						THROW(Helper_Output(rOut, p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
						WriteText(rOut, line_buf.Z().CatTagBrace("u", 1));
					}
				}
				else if(p_blk->Text == "trademark") {
					if(p_first_brc_arg) {
						THROW(Helper_Output(rOut, p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
						WriteText(rOut, line_buf.Z().CatTagBrace("sup", 0).Cat("&trade;").CatTagBrace("sup", 1));
					}
				}
				else if(p_blk->Text == "rdir") {
					WriteText(rOut, line_buf.Z().Cat("&#8594;"));
				}
				else if(p_blk->Text == "symbol") {
					if(p_first_brc_arg) {
						long sc = p_first_brc_arg->Text.ToLong();
						line_buf.Z().CatChar('&').CatChar('#').Cat(sc).Semicol();
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "ppymenu") {
					if(p_first_brc_arg) {
						THROW(Helper_Output(rOut, p_first_brc_arg, _thfDisablePara, rEnvStack)); // @recursion
					}
				}
				else if(p_blk->Text == "ppynote") {
					if(p_first_brc_arg) {
						temp_buf.Z().CatQStr("ppynote");
						line_buf.Z().CatChar('<').Cat("div").Space().CatEq("class", temp_buf).CatChar('>');
						WriteText(rOut, line_buf);
						THROW(Helper_Output(rOut, p_first_brc_arg, 0, rEnvStack)); // @recursion
						line_buf.Z().CatTagBrace("div", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "ppyexample" || p_blk->Text == "ppyexampletitle") {
					if(p_first_brc_arg) {
						temp_buf.Z().CatQStr("ppyexample");
						line_buf.Z().CatChar('<').Cat("div").Space().CatEq("class", temp_buf).CatChar('>');
						WriteText(rOut, line_buf);
						THROW(Helper_Output(rOut, p_first_brc_arg, 0, rEnvStack)); // @recursion
						line_buf.Z().CatTagBrace("div", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "keyb") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("kbd", 0).Cat("&lt;").Cat(p_first_brc_arg->Text).Cat("&gt;").CatTagBrace("kbd", 1).Space();
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "dlgbutton") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("em", 0).CatChar('[').Cat(p_first_brc_arg->Text).CatChar(']').CatTagBrace("em", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "dlgcombo") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("em", 0).Cat("&#9660;").Cat(p_first_brc_arg->Text).CatTagBrace("em", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "dlgflag") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("em", 0).Cat("&#10003;").Cat(p_first_brc_arg->Text).CatTagBrace("em", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "dlgradioi") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("em", 0).Cat("&#9675;").Cat(p_first_brc_arg->Text).CatTagBrace("em", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "ppysyntax") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("em", 0).Cat(p_first_brc_arg->Text).CatTagBrace("em", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "ppyterm") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("dfn", 0).Cat(p_first_brc_arg->Text).CatTagBrace("dfn", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "ppyrsrv") {
					if(p_first_brc_arg) {
						line_buf.Z().CatTagBrace("dfn", 0).Cat(p_first_brc_arg->Text).CatTagBrace("dfn", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "qu") {
					if(p_first_brc_arg) {
						line_buf.Z().Cat("&ldquo;").Cat(p_first_brc_arg->Text).Cat("&rdquo;");
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text.IsEqiAscii("label")) {
					if(p_first_brc_arg) {
						temp_buf.Z().CatChar('\"').Cat(p_first_brc_arg->Text).CatChar('\"');
						line_buf.Z().CatChar('<').Cat("a").Space().CatEq("name", temp_buf).CatChar('>').CatTagBrace("a", 1);
						WriteText(rOut, line_buf);
					}
				}
				else if(p_blk->Text == "item") {
					if(list_item) {
						if(list_item == 2) {
							WriteText(rOut, line_buf.Z().CatTagBrace("dd", 1));
						}
						else {
							WriteText(rOut, line_buf.Z().CatTagBrace("li", 1));
						}
						list_item = 0;
					}
					if(p_blk->P_ArgBrk) {
						WriteText(rOut, line_buf.Z().CatTagBrace("dt", 0));
						THROW(Helper_Output(rOut, p_blk->P_ArgBrk, _thfSuppressPara, rEnvStack)); // @recursion
						WriteText(rOut, line_buf.Z().CatTagBrace("dt", 1));
						WriteText(rOut, line_buf.Z().CatTagBrace("dd", 0));
						list_item = 2;
					}
					else {
						WriteText(rOut, line_buf.Z().CatTagBrace("li", 0));
						list_item = 1;
					}
				}
				else {
					const _TexToHtmlEntry * p = SearchTexToHtmlEntry(_texCmd, p_blk->Text);
					if(p && sstreq(p->P_TexSymb, "chapter")) {
						const char * p_chapter_symb = p_blk->GetInnerLabel();
						const char * p_chapter_text = p_blk->GetInnerText();
						St.ChapterNo++;
						if(P.Flags & P.fDivideByChapter) {
							THROW(InitOutput(&rOut, p_chapter_symb, p_chapter_text, St.ChapterNo));
						}
					}
					if(p_first_brc_arg) {
						const long thf = p ? p->Flags : 0;
						if(p && p->P_HtmlTag) {
							if(thf & _thfEndPara && paragraph)
								Paragraph(&rOut, paragraph);
							WriteText(rOut, line_buf.Z().CatTagBrace(p->P_HtmlTag, 0));
						}
						THROW(Helper_Output(rOut, p_first_brc_arg, thf, rEnvStack)); // @recursion
						if(p && p->P_HtmlTag) {
							WriteText(rOut, line_buf.Z().CatTagBrace(p->P_HtmlTag, 1));
						}
					}
				}
			}
		}
		else if(p_blk->Type == TextBlock::tParagraph) {
			if(paragraph)
				Paragraph(&rOut, paragraph);
			if(!(flags & _thfDisablePara) && (!(flags & _thfSuppressPara) || p_blk->P_Next))
				Paragraph(&rOut, paragraph);
			if(flags & _thfFormula) {
				OutputFormulaItem(p_blk->Text, temp_buf.Z(), 0);
				WriteText(rOut, temp_buf);
			}
			else
				WriteText(rOut, p_blk->Text);
		}
		else if(p_blk->Type == TextBlock::tContinuation) {
			if(flags & _thfFormula) {
				OutputFormulaItem(p_blk->Text, temp_buf.Z(), 0);
				WriteText(rOut, temp_buf);
			}
			else
				WriteText(rOut, p_blk->Text);
		}
		if(flags & _thfSingle)
			break;
	}
	if(paragraph)
		Paragraph(&rOut, paragraph);
	CATCHZOK
	return ok;
}

int PPTex2HtmlPrcssr::OutputStyles(SFile & rOut)
{
#if 0 // {
	<style type="text/css">
		.block1 {
		width: 200px;
		background: #ccc;
		padding: 5px;
		padding-right: 20px;
		border: solid 1px black;
		float: left;
	}
	.block2 {
		width: 200px;
		background: #fc0;
		padding: 5px;
		border: solid 1px black;
		float: left;
		position: relative;
		top: 40px;
		left: -70px;
	}
	</style>
#endif // } 0
	SString line_buf, temp_buf;
	temp_buf.Z().CatQStr("text/css");
	line_buf.Z().CatChar('<').Cat("style").Space().CatEq("type", temp_buf).CatChar('>');

		line_buf.Space().DotCat("ppynote").Space().CatChar('{').Space();
		line_buf.Cat("width").CatDiv(':', 2).Cat("600px").Semicol();
		line_buf.Cat("background").CatDiv(':', 2).Cat("#ccc").Semicol();
		line_buf.Cat("border").CatDiv(':', 2).Cat("solid lpx black").Semicol();
		//line_buf.Cat("float").CatDiv(':', 2).Cat("left").Semicol();
		line_buf.CatChar('}');

		line_buf.Space().DotCat("ppyexample").Space().CatChar('{').Space();
		line_buf.Cat("width").CatDiv(':', 2).Cat("800px").Semicol();
		line_buf.Cat("background").CatDiv(':', 2).Cat("#ccc").Semicol();
		line_buf.Cat("border").CatDiv(':', 2).Cat("solid lpx black").Semicol();
		//line_buf.Cat("float").CatDiv(':', 2).Cat("left").Semicol();
		line_buf.CatChar('}');

	line_buf.CatTagBrace("style", 1).CR();
	WriteText(rOut, line_buf);
	return 1;
}

int PPTex2HtmlPrcssr::InitOutput(SFile * pOut, const char * pPartLabel, const char * pPartName, uint chapterNo)
{
	int    ok = 1;
	StateBlock::OutPart * p_part = 0;
	SString org_symb, part_suffix, wb_name;
	(org_symb = "chapter").CatChar('-').CatLongZ(chapterNo, 3);
	if(isempty(pPartLabel))
		(org_symb = "chapter").CatChar('-').CatLongZ(chapterNo, 3);
	else
		(org_symb = pPartLabel).Strip().ToLower();

	if(P.Flags & P.fDivideByChapter) {
		part_suffix.CatChar('-').Cat(org_symb);
	}
	for(uint i = 0; !p_part && i < St.OutPartList.getCount(); i++) {
		StateBlock::OutPart * p = St.OutPartList.at(i);
		if(p && p->ChapterNo == chapterNo) {
			assert(p->Label == pPartLabel);
			p_part = p;
		}
	}
	if(!p_part) {
		THROW_MEM(p_part = new StateBlock::OutPart);
		p_part->ChapterNo = chapterNo;
		p_part->Label = pPartLabel;
		if(P.Flags & Param::fAttachToWorkbook) {
			SString anchor_prefix, wb_symb;
			P.GetExtStrData(Param::exsAnchorPrefix, anchor_prefix);
			WorkbookTbl::Rec wb_rec;
			PPWorkbookPacket wb_pack;

			wb_symb.Cat(anchor_prefix).Cat(org_symb);
			(wb_name = pPartName ? pPartName : wb_symb).Strip();
			PPID   wb_id = 0;
			int r = WbObj.SearchByLongSymb(wb_symb, &wb_id, &wb_rec);
			if(r > 0) {
				// @todo Если r == 2, то выдать сообщение, что есть более одной записи с заданным символом

				//
				// Некоторые атрибуты, возможно, придется изменить
				//
				THROW(WbObj.GetPacket(wb_id, &wb_pack) > 0);
			}
			else {
				wb_id = 0;
				wb_pack.Rec.Type = PPWBTYP_PAGE;
				wb_pack.SetLongSymb(wb_symb);
			}
			wb_pack.Rec.ParentID = P.ParentWbID;
			wb_pack.Rec.Rank = chapterNo;
			wb_name.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
			SCharToOem(wb_pack.Rec.Name);
			{
				PPID   dup_id = 0;
				if(WbObj.SearchByName(wb_pack.Rec.Name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID) {
					const  size_t max_nm_len = sizeof(wb_pack.Rec.Name)-1;
					PPID   temp_nm_id = 0;
					long   uc = 1;
					SString suffix;
					do {
						(suffix = 0).Space().CatChar('#').Cat(++uc);
						wb_name = wb_pack.Rec.Name;
						size_t sum_len = wb_name.Len() + suffix.Len();
						if(sum_len > max_nm_len)
							wb_name.Trim(max_nm_len-suffix.Len());
						wb_name.Cat(suffix);
					} while(WbObj.SearchByName(wb_name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID);
					wb_name.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
				}
			}
			if(!wb_pack.Rec.Symb[0]) {
				THROW(WbObj.MakeUniqueCode(wb_symb = 0, 0));
				wb_symb.CopyTo(wb_pack.Rec.Symb, sizeof(wb_pack.Rec.Symb));
			}
			THROW(WbObj.PutPacket(&wb_id, &wb_pack, 0));
			p_part->WbID = wb_id;
		}
		else {
			(wb_name = pPartName ? pPartName : org_symb).Strip();
		}
		St.OutPartList.insert(p_part);
	}
	{
		SString output_file_name;
		P.GetExtStrData(Param::exsOutputFileName, output_file_name);
		if(output_file_name.IsEmpty()) {
			P.GetExtStrData(Param::exsInputFileName, output_file_name);
			SPathStruc::ReplaceExt(output_file_name, "html", 1);
			P.PutExtStrData(Param::exsOutputFileName, output_file_name);
		}
		if(part_suffix.NotEmpty()) {
			SPathStruc ps(output_file_name);
			ps.Nam.Cat(part_suffix);
			ps.Merge(output_file_name);
		}
		p_part->FileName = output_file_name;
		if(pOut) {
			THROW_SL(pOut->Open(output_file_name, SFile::mWrite));
			OutputStyles(*pOut);
		}
	}
	CATCHZOK
	return ok;
}

int PPTex2HtmlPrcssr::Run()
{
	ZDELETE(P_Head);

	int    ok = 1;
	SString line_buf, temp_buf;
	SString input_buffer;
	P.GetExtStrData(Param::exsInputFileName, temp_buf);
	SFile  f_in(temp_buf, SFile::mRead|SFile::mBinary);
	THROW_SL(f_in.IsValid());
	{
		P.GetExtStrData(Param::exsOutputPictPath, temp_buf);
		if(temp_buf.IsEmpty()) {
			SString output_file_name;
			P.GetExtStrData(Param::exsOutputFileName, output_file_name);
			if(output_file_name.IsEmpty()) {
				P.GetExtStrData(Param::exsInputFileName, output_file_name);
				SPathStruc::ReplaceExt(output_file_name, "html", 1);
			}
			SPathStruc ps(output_file_name);
			ps.Ext = "img";
			ps.Merge(output_file_name);
			createDir(output_file_name);
			P.PutExtStrData(Param::exsOutputPictPath, output_file_name);
		}
		PPWaitStart();
		{
			int64  _fsize = 0;
			STextEncodingStat tes;
			THROW_SL(f_in.CalcSize(&_fsize));
			{
				size_t actual_size = 0;
				THROW(St.Init((size_t)_fsize));
				THROW_SL(f_in.Read(St.InputBuffer.P_Buf, (size_t)_fsize, &actual_size));
				THROW(actual_size == _fsize);
				tes.Add(St.InputBuffer.P_Buf, actual_size);
				tes.Finish();
				THROW(!tes.CheckFlag(tes.fMiscEolf));
				St.Eolf = tes.GetEolFormat();
				St.InputSize = actual_size;
				St.InputBuffer.P_Buf[actual_size] = 0;
				St.Scan.Set(St.InputBuffer.P_Buf, 0);
				{
					THROW_MEM(P_Head = new TextBlock(TextBlock::tParagraph, St.LineNo));
					THROW(ReadText(0, 0, P_Head));
				}
			}
		}
		{
			PPTransaction tra(BIN(P.Flags & P.fAttachToWorkbook));
			THROW(tra);
			{
				TSStack <_TexEnvItem> env_stack;
				THROW(InitOutput(0, 0, 0, 0));
				THROW(Helper_PreprocessOutput(P_Head, 0, env_stack));
				St.ResetPreprocess();
			}
			{
				SFile f_out;
				TSStack <_TexEnvItem> env_stack;
				THROW(InitOutput(&f_out, 0, 0, 0));
				THROW(Helper_Output(f_out, P_Head, 0, env_stack));
			}
			if(P.Flags & Param::fAttachToWorkbook) {
				uint i;
				for(i = 0; i < St.OutPartList.getCount(); i++) {
                    const StateBlock::OutPart * p_item = St.OutPartList.at(i);
					if(p_item && p_item->WbID && fileExists(p_item->FileName)) {
						THROW(WbObj.AttachFile(p_item->WbID, p_item->FileName, 0));
					}
				}
				for(i = 0; i < St.PictList.getCount(); i++) {
					const StateBlock::PictItem * p_item = St.PictList.at(i);
					if(p_item && p_item->WbID && fileExists(p_item->DestFileName)) {
						THROW(WbObj.AttachFile(p_item->WbID, p_item->DestFileName, 0));
					}
				}
			}
			THROW(tra.Commit());
		}
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

class Tex2HtmlParamDialog : public TDialog {
	DECL_DIALOG_DATA(PPTex2HtmlPrcssr::Param);
public:
	enum {
		grpInput = 1,
		grpInputPic,
		grpOutput,
		grpOutputPic
	};
	Tex2HtmlParamDialog() : TDialog(DLG_TEX2HTM)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_TEX2HTM_INFILE, CTL_TEX2HTM_INFILE, grpInput, 0, PPTXT_FILPAT_LATEX, FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_TEX2HTM_INPICPATH, CTL_TEX2HTM_INPICPATH, grpInputPic, 0, 0, FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_TEX2HTM_OUTPATH, CTL_TEX2HTM_OUTPATH, grpOutput, 0, 0, FileBrowseCtrlGroup::fbcgfAllowNExists|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_TEX2HTM_PICPATH, CTL_TEX2HTM_PICPATH, grpOutputPic, 0, 0, FileBrowseCtrlGroup::fbcgfAllowNExists|FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		SString temp_buf;
		Data.GetExtStrData(Data.exsInputFileName,  temp_buf); setCtrlString(CTL_TEX2HTM_INFILE, temp_buf);
		Data.GetExtStrData(Data.exsInputPictPath,  temp_buf); setCtrlString(CTL_TEX2HTM_INPICPATH, temp_buf);
		Data.GetExtStrData(Data.exsOutputFileName, temp_buf); setCtrlString(CTL_TEX2HTM_OUTPATH, temp_buf);
		Data.GetExtStrData(Data.exsOutputPictPath, temp_buf); setCtrlString(CTL_TEX2HTM_PICPATH, temp_buf);
		AddClusterAssoc(CTL_TEX2HTM_FLAGS, 0, PPTex2HtmlPrcssr::Param::fDivideByChapter);
		AddClusterAssoc(CTL_TEX2HTM_FLAGS, 1, PPTex2HtmlPrcssr::Param::fAttachToWorkbook);
		AddClusterAssoc(CTL_TEX2HTM_FLAGS, 2, PPTex2HtmlPrcssr::Param::fUtf8Codepage);
		SetClusterData(CTL_TEX2HTM_FLAGS, Data.Flags);

		SetupPPObjCombo(this, CTLSEL_TEX2HTM_PARWB, PPOBJ_WORKBOOK, Data.ParentWbID, OLW_CANSELUPLEVEL, 0);
		SetupPPObjCombo(this, CTLSEL_TEX2HTM_PARPICWB, PPOBJ_WORKBOOK, Data.ParentPicWbID, OLW_CANSELUPLEVEL, 0);
		Data.GetExtStrData(Data.exsWbLinkTemplate, temp_buf); setCtrlString(CTL_TEX2HTM_WBREFTMPL, temp_buf);
		Data.GetExtStrData(Data.exsWbPicLinkTemplate, temp_buf); setCtrlString(CTL_TEX2HTM_WBPICREFTMPL, temp_buf);
		Data.GetExtStrData(Data.exsAnchorPrefix, temp_buf); setCtrlString(CTL_TEX2HTM_ANCHORPFX, temp_buf);

		disableCtrls(!(Data.Flags & PPTex2HtmlPrcssr::Param::fAttachToWorkbook),
			CTLSEL_TEX2HTM_PARWB, CTLSEL_TEX2HTM_PARPICWB, CTL_TEX2HTM_WBREFTMPL, CTL_TEX2HTM_WBPICREFTMPL, 0);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		getCtrlString(CTL_TEX2HTM_INFILE, temp_buf);    Data.PutExtStrData(Data.exsInputFileName, temp_buf);
		getCtrlString(CTL_TEX2HTM_INPICPATH, temp_buf); Data.PutExtStrData(Data.exsInputPictPath, temp_buf);
		getCtrlString(CTL_TEX2HTM_OUTPATH, temp_buf);   Data.PutExtStrData(Data.exsOutputFileName, temp_buf);
		getCtrlString(CTL_TEX2HTM_PICPATH, temp_buf);   Data.PutExtStrData(Data.exsOutputPictPath, temp_buf);
		getCtrlString(CTL_TEX2HTM_WBREFTMPL, temp_buf);    Data.PutExtStrData(Data.exsWbLinkTemplate, temp_buf);
		getCtrlString(CTL_TEX2HTM_WBPICREFTMPL, temp_buf); Data.PutExtStrData(Data.exsWbPicLinkTemplate, temp_buf);
		getCtrlString(CTL_TEX2HTM_ANCHORPFX, temp_buf);    Data.PutExtStrData(Data.exsAnchorPrefix, temp_buf);
		GetClusterData(CTL_TEX2HTM_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_TEX2HTM_PARWB, &Data.ParentWbID);
		getCtrlData(CTLSEL_TEX2HTM_PARPICWB, &Data.ParentPicWbID);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_TEX2HTM_FLAGS)) {
			GetClusterData(CTL_TEX2HTM_FLAGS, &Data.Flags);
			disableCtrls(!(Data.Flags & PPTex2HtmlPrcssr::Param::fAttachToWorkbook),
				CTLSEL_TEX2HTM_PARWB, CTLSEL_TEX2HTM_PARPICWB, CTL_TEX2HTM_WBREFTMPL, CTL_TEX2HTM_WBPICREFTMPL, 0);
		}
		else
			return;
		clearEvent(event);
	}
};

/*static*/int PPTex2HtmlPrcssr::EditParam(PPTex2HtmlPrcssr::Param * pParam)
{
    return PPDialogProcBody <Tex2HtmlParamDialog, PPTex2HtmlPrcssr::Param> (pParam);
}

int PPTex2Html()
{
	int    ok = 1;
	PPTex2HtmlPrcssr::Param param;
	PPTex2HtmlPrcssr prc;
	SString file_name;
	// file_name = "D:\\PAPYRUS\\ManWork\\LaTex\\ppmanual.tex";
	//param.InputFileName = file_name;
	//param.InputPictPath = "D:\\PAPYRUS\\ManWork\\pict\\png";
	if(prc.EditParam(&param) > 0) {
		//param.Flags |= PPTex2HtmlPrcssr::Param::fDivideByChapter;
		param.GetExtStrData(param.exsInputFileName, file_name);
		prc.SetParam(&param);
		ok = prc.Run();
		SPathStruc::ReplaceExt(file_name, "debug", 1);
		prc.Debug_Output(file_name);
	}
	return ok;
}
//
//
//
class CMD_HDL_CLS(CONVERTLATEXTOHTML) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CONVERTLATEXTOHTML)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PPTex2HtmlPrcssr prc;
		PPTex2HtmlPrcssr::Param filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(prc.EditParam(&filt) > 0) {
			pParam->Z();
			THROW(filt.Write(*pParam, 0));
			ok = 1; // @v10.6.0
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PPTex2HtmlPrcssr::Param filt;
			if(filt.Read(*pParam, 0)) {
				PPTex2HtmlPrcssr prc;
				prc.SetParam(&filt);
				THROW(prc.Run());
				ok = 1;
			}
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CONVERTLATEXTOHTML);
//
//
//
class PPVer2HtmlPrcssr {
public:
	struct Param : public PPBaseFilt {
		Param();
		Param & FASTCALL operator = (const Param & rS);
		int    GetExtStrData(int fldID, SString & rBuf) const;
		int    PutExtStrData(int fldID, const char * pBuf);
		enum {
			fDivide   = 0x0001,
			fAttachToWorkbook = 0x0002,
			fUtf8Codepage     = 0x0004
		};
		enum {
			exsInputFileName = 1,
			exsPictPath,
			exsOutputFileName,
			exsWbCodePrefix
		};
		uint8  ReserveStart[32];
		long   Flags;
		PPID   ParentWbID;     // Родительский элемент для привязки страниц к рабочим книгам
		SString ExtString;     // @anchor
	};

	static int EditParam(Param * pParam);

	PPVer2HtmlPrcssr();
	~PPVer2HtmlPrcssr();
	void    Destroy();
	int     SetParam(const Param * pParam);
	int     Run();

	struct Paragraph {
		Paragraph(int type, long flags) : Type(type), Flags(flags), P_Next(0)
		{
		}
		~Paragraph()
		{
			ZDELETE(P_Next);
		}
        enum {
        	tRegular = 1,
        	tAttention,
        	tContinuation // Следующий параграф того же элемента описания
        };
        enum {
        	fExclam   = 0x0001,
        	fFix      = 0x0002,
        	fDev      = 0x0004,
        	fMan      = 0x0008,
        	fListItem = 0x0010
        };
        int    Type;
        long   Flags;
        SVerT BugVer; // Версия, в которой появился дефект (для типа tFix)
        SString Topic;
        SString Text;
        Paragraph * P_Next;
	};
	struct VersionEntry {
		VersionEntry(LDATE dt, SVerT ver) : Dt(dt), Ver(ver), P_Body(0)
		{
		}
		~VersionEntry()
		{
			delete P_Body;
		}
		int    AddParagraph(Paragraph * pPara)
		{
			if(pPara) {
				Paragraph * p_last = 0;
				for(Paragraph * p_cur = P_Body; p_cur; p_cur = p_cur->P_Next) {
					p_last = p_cur;
				}
				if(p_last) {
					assert(p_last->P_Next == 0);
					p_last->P_Next = pPara;
				}
				else
					P_Body = pPara;
			}
			return 1;
		}
		SVerT Ver;
        LDATE  Dt;
        PPVer2HtmlPrcssr::Paragraph * P_Body;
	};
private:
	int     AddParagraph(VersionEntry * pEntry, Paragraph * pPara)
	{
		int    ok = 1;
		if(pEntry && pPara) {
            pEntry->AddParagraph(pPara);
			if(pPara->Topic.NotEmpty()) {
				uint p = 0;
				if(TopicCountList.SearchByText(pPara->Topic, 0, &p)) {
					long c = TopicCountList.Get(p).Id;
					TopicCountList.UpdateByPos(p, c+1);
				}
				else {
					TopicCountList.AddFast(1, pPara->Topic);
				}
			}
		}
		return ok;
	}
	int     Parse(const char * pSrcFileName);
	int     Debug_Output(const char * pOutputFileName);
	int     Output(const char * pOutputFileName, const char * pImgPath);
	VersionEntry * ParseVerEntry(const SString & rLine);
	PPID    AttachEntryToWorkbook(const VersionEntry * pEntry, const char * pFileName, int rank, PPID parentWbID, PPObjWorkbook & rWbObj);
	int     WriteText(SFile & rF, const SString & rLineBuf)
	{
		SString temp_buf = rLineBuf;
		if(P.Flags & Param::fUtf8Codepage)
			temp_buf.Transf(CTRANSF_OUTER_TO_UTF8);
		return rF.WriteLine(temp_buf) ? 1 : PPSetErrorSLib();
	}
	Param  P;
	TSCollection <VersionEntry> Entries;
	StrAssocArray TopicCountList;
	SStrScan Scan;
	long   ReH_Ver;
	long   ReH_Para;
	long   ReH_ParaFix;
	long   ReH_ParaMan;
	long   ReH_ParaDev;
	long   ReH_Topic;
	long   ReH_ListItem;
};

IMPL_CMPFUNC(PPVer2HtmlPrcssr_VersionEntry, i1, i2)
{
    const LDATE d1 = static_cast<const PPVer2HtmlPrcssr::VersionEntry *>(i1)->Dt;
    const LDATE d2 = static_cast<const PPVer2HtmlPrcssr::VersionEntry *>(i2)->Dt;
    return CMPSIGN(d2, d1); // Обратный порядок потому d2 идет перед d1
}
//
//
//
PPVer2HtmlPrcssr::Param::Param() : PPBaseFilt(PPFILT_VER2HTMLPARAM, 0, 0)
{
	SetFlatChunk(offsetof(Param, ReserveStart), offsetof(Param, ExtString)-offsetof(Param, ReserveStart));
	SetBranchSString(offsetof(Param, ExtString));
	Init(1, 0);
}

PPVer2HtmlPrcssr::Param & FASTCALL PPVer2HtmlPrcssr::Param::operator = (const PPVer2HtmlPrcssr::Param & rS)
{
	Copy(&rS, 1);
	return *this;
}

int PPVer2HtmlPrcssr::Param::GetExtStrData(int fldID, SString & rBuf) const { return PPGetExtStrData(fldID, ExtString, rBuf); }
int PPVer2HtmlPrcssr::Param::PutExtStrData(int fldID, const char * pBuf) { return PPPutExtStrData(fldID, ExtString, pBuf); }

class Ver2HtmlParamDialog : public TDialog {
	DECL_DIALOG_DATA(PPVer2HtmlPrcssr::Param);
public:
	enum {
		grpInput = 1,
		grpInputPic,
		grpOutput,
		grpOutputPic
	};
	Ver2HtmlParamDialog() : TDialog(DLG_VER2HTM)
	{
		FileBrowseCtrlGroup::Setup(this, CTLBRW_VER2HTM_INFILE, CTL_VER2HTM_INFILE, grpInput, 0, PPTXT_FILPAT_TEXT, FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_VER2HTM_PICPATH, CTL_VER2HTM_PICPATH, grpInputPic, 0, 0, FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_VER2HTM_OUTPATH, CTL_VER2HTM_OUTPATH, grpOutput, 0, 0, FileBrowseCtrlGroup::fbcgfAllowNExists|FileBrowseCtrlGroup::fbcgfSaveLastPath);
		FileBrowseCtrlGroup::Setup(this, CTLBRW_VER2HTM_PICPATH, CTL_VER2HTM_PICPATH, grpOutputPic, 0, 0, FileBrowseCtrlGroup::fbcgfAllowNExists|FileBrowseCtrlGroup::fbcgfPath|FileBrowseCtrlGroup::fbcgfSaveLastPath);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		SString temp_buf;
		Data.GetExtStrData(Data.exsInputFileName,  temp_buf); setCtrlString(CTL_VER2HTM_INFILE, temp_buf);
		Data.GetExtStrData(Data.exsPictPath,  temp_buf); setCtrlString(CTL_VER2HTM_PICPATH, temp_buf);
		Data.GetExtStrData(Data.exsOutputFileName, temp_buf); setCtrlString(CTL_VER2HTM_OUTPATH, temp_buf);
		AddClusterAssoc(CTL_VER2HTM_FLAGS, 0, PPVer2HtmlPrcssr::Param::fDivide);
		AddClusterAssoc(CTL_VER2HTM_FLAGS, 1, PPVer2HtmlPrcssr::Param::fAttachToWorkbook);
		AddClusterAssoc(CTL_VER2HTM_FLAGS, 2, PPVer2HtmlPrcssr::Param::fUtf8Codepage);
		SetClusterData(CTL_VER2HTM_FLAGS, Data.Flags);

		SetupPPObjCombo(this, CTLSEL_VER2HTM_PARWB, PPOBJ_WORKBOOK, Data.ParentWbID, OLW_CANSELUPLEVEL, 0);
		Data.GetExtStrData(Data.exsWbCodePrefix, temp_buf); setCtrlString(CTL_VER2HTM_ANCHORPFX, temp_buf);

		disableCtrls(!(Data.Flags & PPVer2HtmlPrcssr::Param::fAttachToWorkbook), CTLSEL_VER2HTM_PARWB, 0);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		getCtrlString(CTL_VER2HTM_INFILE, temp_buf);    Data.PutExtStrData(Data.exsInputFileName, temp_buf);
		getCtrlString(CTL_VER2HTM_PICPATH, temp_buf);   Data.PutExtStrData(Data.exsPictPath, temp_buf);
		getCtrlString(CTL_VER2HTM_OUTPATH, temp_buf);   Data.PutExtStrData(Data.exsOutputFileName, temp_buf);
		getCtrlString(CTL_VER2HTM_ANCHORPFX, temp_buf); Data.PutExtStrData(Data.exsWbCodePrefix, temp_buf);
		GetClusterData(CTL_VER2HTM_FLAGS, &Data.Flags);
		getCtrlData(CTLSEL_VER2HTM_PARWB, &Data.ParentWbID);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_VER2HTM_FLAGS)) {
			GetClusterData(CTL_VER2HTM_FLAGS, &Data.Flags);
			disableCtrls(!(Data.Flags & PPVer2HtmlPrcssr::Param::fAttachToWorkbook), CTLSEL_VER2HTM_PARWB, 0);
		}
		else
			return;
		clearEvent(event);
	}
};

/*static*/int PPVer2HtmlPrcssr::EditParam(PPVer2HtmlPrcssr::Param * pParam)
{
    return PPDialogProcBody <Ver2HtmlParamDialog, PPVer2HtmlPrcssr::Param> (pParam);
}

PPVer2HtmlPrcssr::PPVer2HtmlPrcssr() : ReH_Ver(0), ReH_Para(0), ReH_ParaFix(0), ReH_ParaDev(0), ReH_ParaMan(0), ReH_Topic(0), ReH_ListItem(0)
{
	Scan.RegisterRe("^[v]?[0-9][0-9]?\\.[0-9][0-9]?\\.[0-9][0-9]?", &ReH_Ver);
	Scan.RegisterRe("^[!]?-[^\\-]", &ReH_Para);
	Scan.RegisterRe("^--", &ReH_ListItem);
	Scan.RegisterRe("^[!]?-FIX[:]?", &ReH_ParaFix);
	Scan.RegisterRe("^[!]?-DEV[:]?", &ReH_ParaDev);
	Scan.RegisterRe("^[!]?-MAN[:]?", &ReH_ParaMan);
	Scan.RegisterRe("^\\{.*\\}", &ReH_Topic);
}

PPVer2HtmlPrcssr::~PPVer2HtmlPrcssr()
{
}

void PPVer2HtmlPrcssr::Destroy()
{
	Entries.freeAll();
}

int PPVer2HtmlPrcssr::SetParam(const PPVer2HtmlPrcssr::Param * pParam)
{
	return RVALUEPTR(P, pParam) ? 1 : -1;
}

PPVer2HtmlPrcssr::VersionEntry * PPVer2HtmlPrcssr::ParseVerEntry(const SString & rLine)
{
	VersionEntry * p_entry = 0;
	LDATE  dt = ZERODATE;
	SVerT ver;
    SString ver_buf;
    Scan.Set(rLine, 0);
    Scan.Skip();
    if(Scan.GetDate(DATF_DMY, dt)) {
    	Scan.Skip();
    	if(Scan.GetRe(ReH_Ver, ver_buf)) {
            ver_buf.ShiftLeftChr('v');
			if(ver.FromStr(ver_buf)) {
				 THROW_MEM(p_entry = new VersionEntry(dt, ver));
			}
    	}
	}
	CATCH
		ZDELETE(p_entry);
	ENDCATCH
	return p_entry;
}

int PPVer2HtmlPrcssr::Debug_Output(const char * pOutputFileName)
{
	int    ok = 1;
	SString line_buf, temp_buf;
	SFile  f_out(pOutputFileName, SFile::mWrite);
	THROW_SL(f_out.IsValid());
	for(uint i = 0; i < Entries.getCount(); i++) {
		const VersionEntry * p_entry = Entries.at(i);
		if(p_entry) {
			line_buf.Z().Cat(p_entry->Dt, DATF_DMY).Space().Cat(p_entry->Ver.ToStr(temp_buf.Z())).CR();
			f_out.WriteLine(line_buf);
			line_buf.Z();
			for(Paragraph * p_para = p_entry->P_Body; p_para; p_para = p_para->P_Next) {
				line_buf.Z();
                if(p_para->Type == p_para->tRegular) {
                	line_buf.Tab();
					if(p_para->Flags & p_para->fExclam)
						line_buf.CatChar('!');
					line_buf.CatChar('-');
                }
                else if(p_para->Type == p_para->tAttention) {
                	line_buf.Tab();
					line_buf.Cat("ATTENTION!");
                }
                else {
                	line_buf.Tab();
					line_buf.Cat("INVALID PARAGRAPH TYPE!");
                }
                line_buf.Tab().Cat(p_para->Text);
				f_out.WriteLine(line_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

static SString & MakeHtmlImg(const char * pPath, const char * pName, SString & rBuf)
{
	SString img_file_name;
	(img_file_name = pPath).SetLastSlash().RmvLastSlash().CatChar('/').Cat(pName);
	return rBuf.Z().CatChar('<').Cat("img").Space().CatEqQ("src", img_file_name).CatChar('>');
}

PPID PPVer2HtmlPrcssr::AttachEntryToWorkbook(const VersionEntry * pEntry, const char * pFileName, int rank, PPID parentWbID, PPObjWorkbook & rWbObj)
{
	PPID   wb_id = 0;
	int    mj = 0, mn = 0, rz = 0;
	SString wb_symb, code_prefix, wb_name, temp_buf;
	P.GetExtStrData(Param::exsWbCodePrefix, code_prefix);
	if(!code_prefix.NotEmptyS()) {
		code_prefix = "PPVER";
	}
	wb_symb.Cat(code_prefix.ToUpper());
	if(pEntry) {
        pEntry->Ver.Get(&mj, &mn, &rz);
        wb_symb.CatLongZ(mj, 2).CatLongZ(mn, 2).CatLongZ(rz, 2);
	}
	else {
		wb_symb.Cat("ALL");
	}
	const PPID parent_wb_id = NZOR(parentWbID, P.ParentWbID);
	WorkbookTbl::Rec wb_rec, parent_wb_rec;
	PPWorkbookPacket wb_pack;
	if(!isempty(pFileName)) {
		THROW_SL(fileExists(pFileName));
	}
	if(parent_wb_id) {
		THROW(rWbObj.Search(parent_wb_id, &parent_wb_rec) > 0);
	}
	// @v10.6.4 else { MEMSZERO(parent_wb_rec); }
	int r = rWbObj.SearchBySymb(wb_symb, &wb_id, &wb_rec);
	if(r > 0) {
		// wb_id известен
		// Если что-то существенное изменилось, то перепроводим пакет
		if((pEntry && wb_rec.Dt != pEntry->Dt) || wb_rec.ParentID != parent_wb_id || wb_rec.Rank != rank) {
			PPWorkbookPacket wb_pack;
			THROW(rWbObj.GetPacket(wb_id, &wb_pack) > 0);
			if(pEntry)
				wb_pack.Rec.Dt = pEntry->Dt;
			wb_pack.Rec.ParentID = parent_wb_id;
			wb_pack.Rec.Rank = rank;
			THROW(rWbObj.PutPacket(&wb_id, &wb_pack, 0));
		}
	}
	else {
		if(pEntry) {
			PPLoadText(PPTXT_HTMLPPVERTITLE, wb_name);
			pEntry->Ver.ToStr(temp_buf.Z());
			wb_name.Space().Cat(temp_buf);
			wb_pack.Rec.Dt = pEntry->Dt;
		}
		else {
			PPLoadText(PPTXT_HTMLPPVERALLTITLE, wb_name);
		}
		wb_symb.CopyTo(wb_pack.Rec.Symb, sizeof(wb_pack.Rec.Symb));
		wb_name.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
		{
			PPID   dup_id = 0;
			if(rWbObj.SearchByName(wb_pack.Rec.Name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID) {
				const  size_t max_nm_len = sizeof(wb_pack.Rec.Name)-1;
				PPID   temp_nm_id = 0;
				long   uc = 1;
				SString suffix;
				do {
					(suffix = 0).Space().CatChar('#').Cat(++uc);
					wb_name = wb_pack.Rec.Name;
					size_t sum_len = wb_name.Len() + suffix.Len();
					if(sum_len > max_nm_len)
						wb_name.Trim(max_nm_len-suffix.Len());
					wb_name.Cat(suffix);
				} while(rWbObj.SearchByName(wb_name, &dup_id, 0) > 0 && dup_id != wb_pack.Rec.ID);
				wb_name.CopyTo(wb_pack.Rec.Name, sizeof(wb_pack.Rec.Name));
			}
		}
		wb_pack.Rec.Type = PPWBTYP_PAGE;
		wb_pack.Rec.Rank = rank;
		wb_pack.Rec.ParentID = parent_wb_id;
		THROW(rWbObj.PutPacket(&(wb_id = 0), &wb_pack, 0));
	}
	if(!isempty(pFileName)) {
		THROW(rWbObj.AttachFile(wb_id, pFileName, 0));
	}
	CATCH
		wb_id = 0;
	ENDCATCH
	return wb_id;
}

int PPVer2HtmlPrcssr::Output(const char * pOutputFileName, const char * pImgPath)
{
	/*
		temp_buf.Z().CatQStr(line_buf);
		line_buf.Z().Cat(temp_buf2).CatChar('<').Cat("img").Space().CatEq("src", temp_buf);
	*/

	int    ok = 1;
	PPObjWorkbook wb_obj;
	SString line_buf, temp_buf, entry_buf;
	if(Entries.getCount()) {
		SString out_file_name;
		SFile  f_out(pOutputFileName, SFile::mWrite);
		THROW_SL(f_out.IsValid());
		{
			PPID   root_wb_id = 0;
			PPTransaction tra(BIN(P.Flags & Param::fAttachToWorkbook));
			THROW(tra);
			if(P.Flags & Param::fAttachToWorkbook) {
				//
				// Создаем (если не существует) запись для полного тела текста.
				// Затем к этой записи в виде потомков будут цепляться статьи по отдельным версиям
				// (если установлена опция разбивки по версиям).
				//
				THROW(root_wb_id = AttachEntryToWorkbook(0, 0, -10000, 0, wb_obj));
			}
			line_buf.Z().CatChar('<').Cat("table").Space().CatEqQ("rules", "groups").CatChar('>').CR();
			WriteText(f_out, line_buf);
			for(uint i = 0; i < Entries.getCount(); i++) {
				const VersionEntry * p_entry = Entries.at(i);
				if(p_entry) {
					entry_buf.Z();
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("thead", 0).CR());
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tr", 0).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
							entry_buf.Cat(line_buf.Z().Tab(3).Cat(p_entry->Ver.ToStr(temp_buf.Z())).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
							entry_buf.Cat(line_buf.Z().Tab(3).Cat(p_entry->Dt, DATF_DMY|DATF_CENTURY).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
							// empty
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tr", 1).CR());
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("thead", 1).CR());
					//
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tbody", 0).CR());
					for(Paragraph * p_para = p_entry->P_Body; p_para; p_para = p_para->P_Next) {
						entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tr", 0).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
							line_buf.Z();
							if(p_para->Type == p_para->tRegular) {
								if(p_para->Flags & p_para->fExclam) {
									MakeHtmlImg(pImgPath, "checker-red-32.png", temp_buf);
									line_buf.Cat(temp_buf);
									//line_buf.CatChar('!');
								}
								if(p_para->Flags & Paragraph::fFix) {
									MakeHtmlImg(pImgPath, "wrench-32.png", temp_buf);
									line_buf.Cat(temp_buf);
								}
								else if(p_para->Flags & Paragraph::fMan) {
									MakeHtmlImg(pImgPath, "manual-32.png", temp_buf);
									line_buf.Cat(temp_buf);
								}
								else if(p_para->Flags & Paragraph::fDev) {
									MakeHtmlImg(pImgPath, "development-32.png", temp_buf);
									line_buf.Cat(temp_buf);
								}
								else if(!(p_para->Flags & Paragraph::fExclam)) {
									MakeHtmlImg(pImgPath, "checker-green-32.png", temp_buf);
									line_buf.Cat(temp_buf);
								}
							}
							else if(p_para->Type == Paragraph::tAttention) {
								//line_buf.Cat("ATTENTION!");
								MakeHtmlImg(pImgPath, "attention-32.png", temp_buf);
								line_buf.Cat(temp_buf);
							}
							else if(p_para->Type == Paragraph::tContinuation) {
							}
							else {
								line_buf.Cat("$INVALID$");
							}
							entry_buf.Cat(line_buf.CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
							line_buf.Z().Tab(3).CatTagBrace("p", 0);
							if(p_para->Topic.NotEmpty()) {
								line_buf.Cat(p_para->Topic);
							}
							line_buf.CatTagBrace("p", 1);
							entry_buf.Cat(line_buf.CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 0).CR());
						{
							Paragraph * p_next = 0;
							do {
								if(p_next)
									p_para = p_next;
								p_next = p_para->P_Next;
								if(p_para->Flags & Paragraph::fListItem) {
									Paragraph * p_li_next = 0;
									entry_buf.Cat(line_buf.Z().Tab(3).CatTagBrace("ul", 0).CR());
									do {
										if(p_li_next)
											p_para = p_li_next;
										p_next = p_li_next = p_para->P_Next;
										entry_buf.Cat(line_buf.Z().Tab(4).CatTagBrace("li", 0).CR());
										line_buf.Z().Tab(5).CatTagBrace("p", 0);
										if(p_para->Text.NotEmpty()) {
											line_buf.Cat((temp_buf = p_para->Text).ReplaceSpecSymb(SFileFormat::Html));
										}
										line_buf.CatTagBrace("p", 1);
										entry_buf.Cat(line_buf.CR());
										entry_buf.Cat(line_buf.Z().Tab(4).CatTagBrace("li", 1).CR());
									} while(p_li_next && p_li_next->Type == Paragraph::tContinuation && p_li_next->Flags & Paragraph::fListItem);
									entry_buf.Cat(line_buf.Z().Tab(3).CatTagBrace("ul", 1).CR());
								}
								else {
									line_buf.Z().Tab(3).CatTagBrace("p", 0);
									if(p_para->Text.NotEmpty()) {
										line_buf.Cat((temp_buf = p_para->Text).ReplaceSpecSymb(SFileFormat::Html));
									}
									line_buf.CatTagBrace("p", 1);
									entry_buf.Cat(line_buf.CR());
								}
							} while(p_next && p_next->Type == Paragraph::tContinuation);
						}
						entry_buf.Cat(line_buf.Z().Tab(2).CatTagBrace("td", 1).CR());
						entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tr", 1).CR());
					}
					entry_buf.Cat(line_buf.Z().Tab().CatTagBrace("tbody", 1).CR());

					WriteText(f_out, entry_buf);
					if(P.Flags & Param::fDivide) {
						SString part_file_name;
						SPathStruc ps(pOutputFileName);
						int    mj, mn, rz;
						p_entry->Ver.Get(&mj, &mn, &rz);
						ps.Nam.CatChar('-').CatLongZ(mj, 2).CatLongZ(mn, 2).CatLongZ(rz, 2);
						ps.Merge(part_file_name);
						SFile f_part_out(part_file_name, SFile::mWrite);

						line_buf.Z().CatChar('<').Cat("table").Space().CatEqQ("rules", "groups").CatChar('>').CR();
						WriteText(f_part_out, line_buf);
						WriteText(f_part_out, entry_buf);
						WriteText(f_part_out, line_buf.Z().CatTagBrace("table", 1).CR());
						if(P.Flags & Param::fAttachToWorkbook) {
							f_part_out.Close();
							THROW(AttachEntryToWorkbook(p_entry, part_file_name, i+1, root_wb_id, wb_obj));
						}
					}
				}
			}
			WriteText(f_out, line_buf.Z().CatTagBrace("table", 1).CR());
			if(P.Flags & Param::fAttachToWorkbook) {
				f_out.Close();
				// Осталось подцепить общий файл к корневому узлу.
				THROW(wb_obj.AttachFile(root_wb_id, pOutputFileName, 0));
				//THROW(AttachEntryToWorkbook(0, pOutputFileName, 0, -10000, wb_obj));
			}
			THROW(tra.Commit());
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPVer2HtmlPrcssr::Parse(const char * pSrcFileName)
{
    int    ok = 1;
    int    start_process = 0;
    VersionEntry * p_cur_entry = 0;
    Paragraph * p_cur_para = 0;
    SString line_buf, temp_buf;
    SFile f_in(pSrcFileName, SFile::mRead);
    THROW_SL(f_in.IsValid());
    Destroy();
    while(f_in.ReadLine(line_buf)) {
		line_buf.Chomp().Strip();
		VersionEntry * p_temp_entry = ParseVerEntry(line_buf);
		if(p_temp_entry) {
			if(p_cur_entry) {
				if(p_cur_para) {
					AddParagraph(p_cur_entry, p_cur_para);
					p_cur_para = 0;
				}
				THROW_SL(Entries.insert(p_cur_entry));
			}
			p_cur_entry = p_temp_entry;
			p_temp_entry = 0;
		}
		else if(p_cur_entry) {
            Paragraph * p_temp_para = 0;
			if(line_buf.IsEmpty()) {
				if(p_cur_para) {
					THROW(AddParagraph(p_cur_entry, p_cur_para));
					p_cur_para = new Paragraph(Paragraph::tContinuation, 0);
				}
			}
			else {
				Scan.Set(line_buf, 0);
				Scan.Skip();
				long   para_flags = 0;
				if(Scan.Get("ВНИМАНИЕ!", temp_buf)) {
					p_temp_para = new Paragraph(Paragraph::tAttention, para_flags);
				}
				else if(Scan.GetRe(ReH_ParaFix, temp_buf)) {
					para_flags = Paragraph::fFix;
					if(temp_buf[0] == '!')
						para_flags |= Paragraph::fExclam;
					p_temp_para = new Paragraph(Paragraph::tRegular, para_flags);
				}
				else if(Scan.GetRe(ReH_ParaDev, temp_buf)) {
					para_flags = Paragraph::fDev;
					if(temp_buf[0] == '!')
						para_flags |= Paragraph::fExclam;
					p_temp_para = new Paragraph(Paragraph::tRegular, para_flags);
				}
				else if(Scan.GetRe(ReH_ParaMan, temp_buf)) {
					para_flags = Paragraph::fMan;
					if(temp_buf[0] == '!')
						para_flags |= Paragraph::fExclam;
					p_temp_para = new Paragraph(Paragraph::tRegular, para_flags);
				}
				else if(Scan.GetRe(ReH_Para, temp_buf)) {
					para_flags = 0;
					if(temp_buf[0] == '!')
						para_flags |= Paragraph::fExclam;
					p_temp_para = new Paragraph(Paragraph::tRegular, para_flags);
				}
				else if(Scan.GetRe(ReH_ListItem, temp_buf)) {
					p_temp_para = new Paragraph(p_cur_para ? Paragraph::tContinuation : Paragraph::tRegular, Paragraph::fListItem);
				}
				if(p_temp_para) {
					Scan.Skip();
					if(Scan.GetRe(ReH_Topic, temp_buf)) {
						Scan.Skip();
						assert(temp_buf[0] == '{' && temp_buf.Last() == '}');
						temp_buf.TrimRightChr('}').ShiftLeftChr('{').Strip();
						p_temp_para->Topic = temp_buf;
					}
					if(p_cur_para) {
						THROW(AddParagraph(p_cur_entry, p_cur_para));
					}
					p_cur_para = p_temp_para;
					p_temp_para = 0;
					//
					(p_cur_para->Text = (const char *)Scan).CR();
				}
				else if(p_cur_para) {
					p_cur_para->Text.Cat((const char *)Scan).CR();
				}
			}
		}
    }
    if(p_cur_entry) {
		if(p_cur_para) {
			THROW(AddParagraph(p_cur_entry, p_cur_para));
			p_cur_para = 0;
		}
    	Entries.insert(p_cur_entry);
		p_cur_entry = 0;
    }
    Entries.sort(PTR_CMPFUNC(PPVer2HtmlPrcssr_VersionEntry));
    CATCHZOK
    return ok;
}

int PPVer2HtmlPrcssr::Run()
{
	int    ok = 1;
	SString file_name, out_file_name;
	SString pict_path;
	P.GetExtStrData(Param::exsInputFileName, file_name);
	THROW(Parse(file_name));
	{
		P.GetExtStrData(Param::exsPictPath, pict_path);
		P.GetExtStrData(Param::exsOutputFileName, out_file_name);
		{
			if(!out_file_name.NotEmptyS()) {
				SPathStruc::ReplaceExt(out_file_name = file_name, "html", 1);
			}
			THROW(Output(out_file_name, pict_path));
		}
		{
			SPathStruc::ReplaceExt(out_file_name, "index", 1);
			SString line_buf;
			SFile  f_out(out_file_name, SFile::mWrite);
			THROW_SL(f_out.IsValid());
			for(uint i = 0; i < TopicCountList.getCount(); i++) {
				StrAssocArray::Item item = TopicCountList.at_WithoutParent(i);
				line_buf.Z().Cat(item.Txt).Tab().Cat(item.Id).CR();
				f_out.WriteLine(line_buf);
			}
		}
		{
			SPathStruc::ReplaceExt(out_file_name, "out", 1);
			THROW(Debug_Output(out_file_name));
		}
	}
	CATCHZOKPPERR
	return ok;
}

int PPVer2Html()
{
	int    ok = 1;
	PPVer2HtmlPrcssr::Param param;
	PPVer2HtmlPrcssr prc;
	//file_name = "d:\\papyrus\\src\\doc\\version.txt";
	//pict_path = "..\\rsrc\\bitmap";
	if(prc.EditParam(&param) > 0) {
		prc.SetParam(&param);
		THROW(prc.Run());
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
class CMD_HDL_CLS(CONVERTVERSIONTOHTML) : public PPCommandHandler {
public:
	CMD_HDL_CLS(CONVERTVERSIONTOHTML)(const PPCommandDescr * pDescr) : PPCommandHandler(pDescr)
	{
	}
	virtual int EditParam(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		size_t sav_offs = 0;
		PPVer2HtmlPrcssr prc;
		PPVer2HtmlPrcssr::Param filt;
		THROW_INVARG(pParam);
        sav_offs = pParam->GetRdOffs();
		filt.Read(*pParam, 0);
		if(prc.EditParam(&filt) > 0) {
			pParam->Z();
			THROW(filt.Write(*pParam, 0));
		}
		else
			pParam->SetRdOffs(sav_offs);
		CATCH
			CALLPTRMEMB(pParam, SetRdOffs(sav_offs));
			ok = 0;
		ENDCATCH
		return ok;
	}
	virtual int Run(SBuffer * pParam, long, void * extraPtr)
	{
		int    ok = -1;
		if(pParam) {
			PPVer2HtmlPrcssr::Param filt;
			if(filt.Read(*pParam, 0)) {
				PPVer2HtmlPrcssr prc;
				prc.SetParam(&filt);
				THROW(prc.Run());
				ok = 1;
			}
		}
		CATCHZOKPPERR
		return ok;
	}
};

IMPLEMENT_CMD_HDL_FACTORY(CONVERTVERSIONTOHTML);
