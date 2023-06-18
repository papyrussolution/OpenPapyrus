// TEST-ETC.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Модуль тестирования разных функций. В основном в процессе разработки.
//
#include <pp.h>
#pragma hdrstop
//
//
//
class PoBlock : SStrGroup {
public:
	PoBlock() : MsgIdHash(SMEGABYTE(20), 0/*don't use assoc at startup*/), LastMsgId(0)
	{
	}
	int    Add(uint lang, const char * pMsgId, const char * pText)
	{
		int    ok = 1;
		if(!isempty(pMsgId) && !isempty(pText)) {
			Entry new_entry(lang);
			if(!MsgIdHash.Search(pMsgId, &new_entry.MsgId, 0)) {
				new_entry.MsgId = ++LastMsgId;
				THROW_SL(MsgIdHash.Add(pMsgId, new_entry.MsgId));
			}
			THROW_SL(SStrGroup::AddS(pText, &new_entry.TextP));
			THROW_SL(L.insert(&new_entry));
		}
		CATCHZOK
		return ok;
	}
	SJson * ExportToJson()
	{
		SJson * p_result = SJson::CreateObj();
		if(Ident.NotEmpty()) {
			p_result->InsertString("ident", Ident);
		}
		{
			SJson * p_js_list = SJson::CreateArr();
			SString temp_buf;
			for(uint i = 0; i < L.getCount(); i++) {
				const Entry & r_entry = L.at(i);
				SJson * p_js_entry = SJson::CreateObj();
				MsgIdHash.GetByAssoc(r_entry.MsgId, temp_buf);
				p_js_entry->InsertString("id", temp_buf.Escape());
				GetLinguaCode(r_entry.Lang, temp_buf);
				p_js_entry->InsertString("lng", temp_buf.Escape());
				GetS(r_entry.TextP, temp_buf);
				p_js_entry->InsertString("str", temp_buf.Escape());
				p_js_list->InsertChild(p_js_entry);
				p_js_entry = 0;
			}
			p_result->Insert("list", p_js_list);
			p_js_list = 0;
		}
		return p_result;
	}
	//
	// Descr: Следует вызвать после завершения вставки данных. Функция выполяет 
	//   всякие индексирующие операции. В целом, можно и без этого, но все будет медленно.
	//
	void   Finish();
	void   Sort();
private:
	struct Entry {
		explicit Entry(uint lang = 0) : MsgId(0), Lang(lang), TextP(0)
		{
		}
		uint   MsgId;
		uint   Lang;
		uint   TextP;
	};
	static DECL_CMPFUNC(PoBlock_Entry);
	uint   LastMsgId;
	SString Ident; // 
	TSVector <Entry> L;
	SymbHashTable MsgIdHash;
};

/*static*/IMPL_CMPMEMBFUNC(PoBlock, PoBlock_Entry, i1, i2)
{
	int    s = 0;
	const PoBlock * p_this = static_cast<const PoBlock *>(pExtraData);
	if(p_this) {
		const Entry * p_e1 = static_cast<const Entry *>(i1);
		const Entry * p_e2 = static_cast<const Entry *>(i2);
		if(p_e1->MsgId == p_e2->MsgId) {
			if(p_e1->Lang == p_e2->Lang) {
				if(p_e1->TextP == p_e2->TextP) {
					s = 0;
				}
				else {
					SString & r_buf1 = SLS.AcquireRvlStr();
					SString & r_buf2 = SLS.AcquireRvlStr();
					p_this->GetS(p_e1->TextP, r_buf1);
					p_this->GetS(p_e2->TextP, r_buf2);
					s = r_buf1.Cmp(r_buf2, 0);
				}
			}
			else {
				s = CMPSIGN(p_e1->Lang, p_e2->Lang);
			}
		}
		else {
			SString & r_buf1 = SLS.AcquireRvlStr();
			SString & r_buf2 = SLS.AcquireRvlStr();
			p_this->MsgIdHash.GetByAssoc(p_e1->MsgId, r_buf1);
			p_this->MsgIdHash.GetByAssoc(p_e2->MsgId, r_buf2);
			s = r_buf1.Cmp(r_buf2, 0);
		}
	}
	return s;
}

void PoBlock::Sort()
{
	L.sort2(PTR_CMPFUNC(PoBlock_Entry), this);
}

void PoBlock::Finish()
{
	MsgIdHash.BuildAssoc();
}

int ImportPo_construction(const char * pFileName, PoBlock & rBlk)
{
	int    ok = 1;
	enum {
		stateNothing = 0,
		stateEmptyLine,
		stateMsgId,
		stateMsgStr
	};
	int    state = stateNothing;
	uint   lang = 0;
	constexpr const char * p_pfx_msgid = "msgid";
	constexpr const char * p_pfx_msgstr = "msgstr";
	const size_t pfxlen_msgid = strlen(p_pfx_msgid);
	const size_t pfxlen_msgstr = strlen(p_pfx_msgstr);
	SString temp_buf;
	SString line_buf;
	SString last_msgid_buf;
	SString last_msgstr_buf;
	SPathStruc ps(pFileName);
	{
		int _fn_lang = RecognizeLinguaSymb(ps.Nam, 1);
		if(_fn_lang)
			lang = _fn_lang;
	}
	SFile  f_in(pFileName, SFile::mRead);
	THROW_SL(f_in.IsValid());
	while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
		if(line_buf.IsEmpty()) {
			if(state == stateMsgStr) {
				if(last_msgid_buf.IsEmpty() && last_msgstr_buf.NotEmpty()) {
					// metadata
					uint _p = 0;
					const char * p_pattern = "Language:";
					if(last_msgstr_buf.Search(p_pattern, 0, 1, &_p)) {
						const size_t pat_len = strlen(p_pattern);
						const char * p = last_msgstr_buf.cptr() + _p + pat_len;
						while(*p == ' ' || *p == '\t')
							p++;
						temp_buf.Z();
						while(isasciialpha(*p)) {
							temp_buf.CatChar(*p);
							p++;
						}
						int _meta_lang = RecognizeLinguaSymb(temp_buf, 1);
						if(lang == 0)
							lang = _meta_lang;
						else if(lang != _meta_lang) {
							; // плохо - язык в мета-данных не совпадает с языком в наименовании файла
						}
						//
						THROW(lang);
					}
				}
				else if(last_msgid_buf.NotEmpty() && last_msgstr_buf.NotEmpty()) {
					THROW(rBlk.Add(lang, last_msgid_buf, last_msgstr_buf));
				}
			}
			state = stateEmptyLine;
		}
		else if(line_buf.C(0) == '#') {
			; // skip comment
		}
		else if(line_buf.HasPrefixIAscii(p_pfx_msgid)) {
			state = stateMsgId;
			last_msgid_buf.Z();
			last_msgstr_buf.Z();
			ReadQuotedString(line_buf.cptr()+pfxlen_msgid, line_buf.Len()-pfxlen_msgid, QSF_ESCAPE|QSF_SKIPUNTILQ, 0, last_msgid_buf);
		}
		else if(line_buf.HasPrefixIAscii(p_pfx_msgstr)) {
			if(state == stateMsgId) {
				state = stateMsgStr;
				ReadQuotedString(line_buf.cptr()+pfxlen_msgstr, line_buf.Len()-pfxlen_msgstr, QSF_ESCAPE|QSF_SKIPUNTILQ, 0, last_msgid_buf);
			}
			else {
				// @error
			}
		}
		else if(line_buf.C(0) == '\"') {
			SString * p_dest_buf = (state == stateMsgId) ? &last_msgid_buf : ((state == stateMsgStr) ? &last_msgstr_buf : 0);
			if(p_dest_buf) {
				const int gqsr = ReadQuotedString(line_buf.cptr(), line_buf.Len(), QSF_ESCAPE|QSF_SKIPUNTILQ, 0, temp_buf);
				if(gqsr > 0)
					p_dest_buf->Cat(temp_buf);
			}
		}
	}
	CATCHZOK
	return ok;
}

SLTEST_R(ImportPo)
{
	// C:\Papyrus\Src\Rsrc\Data\iso-codes
	// C:\Papyrus\Src\Rsrc\Data\iso-codes\iso_15924
	SString temp_buf;
	SString path_buf;
	PoBlock blk;
	SDirEntry de;
	SString base_dir("/Papyrus/Src/Rsrc/Data/iso-codes/iso_15924/");
	(temp_buf = base_dir).Cat("*.po");
	for(SDirec sd(temp_buf); sd.Next(&de) > 0;) {
		if(!de.IsSelf() && !de.IsUpFolder() && (de.IsFolder() || de.IsFile())) {
			de.GetNameA(base_dir, temp_buf);
			SPathStruc::NormalizePath(temp_buf, SPathStruc::npfCompensateDotDot, path_buf);
			ImportPo_construction(path_buf, blk);
		}
	}
	blk.Finish();
	blk.Sort();
	{
		SJson * p_js = blk.ExportToJson();
		if(p_js) {
			p_js->ToStr(temp_buf);
			(path_buf = base_dir).SetLastDSlash().Cat("importpo-out-file.json");
			SFile f_out(path_buf, SFile::mWrite);
			f_out.Write(temp_buf.cptr(), temp_buf.Len());
		}
	}
	return CurrentStatus;
}