// STRSTORE.CPP
// Copyright (c) A.Sobolev 1996-1999, 2000-2003, 2004, 2008, 2010, 2011, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#pragma hdrstop
#define SKIP_UED_ID_H
#include <pp.h>
#include <strstore.h>
//
// Testing command line params:
//   регулярная компиляция (sc2s): D:\Papyrus\Src\Rsrc\Str\ppstr2.txt
//   автоперевод (papyrus):        /autotranslate:D:\Papyrus\Src\Rsrc\Str\ppstr2.txt;en
//
static const char * P_StrSt_EmptyCodeSymb = ".N.";
//
//
//
StringStore2::LangStrCollItem::LangStrCollItem() : SLang(0), P_PositionHash(0)
{
}

StringStore2::LangStrCollItem::~LangStrCollItem()
{
	ZDELETE(P_PositionHash);
}

int StringStore2::LangStrCollItem::Get(long ident, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 0;
	if(P_PositionHash) {
		long   tpos = 0;
		if(P_PositionHash->Get(ident, &tpos))
			ok = List.GetTextByPos((uint)tpos, rBuf);
	}
	else if(List.GetText(ident, rBuf))
		ok = 1;
	return ok;
}
//
//
//
StringStore2::Hash::Hash(size_t sz) : HashTableBase(sz)
{
}

size_t FASTCALL StringStore2::Hash::GetHash(long id) const
{
	uint32 __h = SlHash::DJB(&id, sizeof(id));
	return static_cast<size_t>(__h % Size);
}

int StringStore2::Hash::Add(long id, long value)
{
	int    c = 0;
	if(InitTab()) {
		const size_t h = GetHash(id);
		c = P_Tab[h].SetVal(id, value);
	}
	return c;
}

int StringStore2::Hash::Get(long id, long * pValue) const
{
	int    ok = 0;
	if(P_Tab) {
		const  size_t h = GetHash(id);
		const  Entry & r_entry = P_Tab[h];
		if(r_entry.Count > 0) {
			if(r_entry.Val.Key == id) {
				ASSIGN_PTR(pValue, r_entry.Val.Val);
				ok = 1;
			}
			else {
				for(uint i = 1; !ok && i < r_entry.Count; i++) {
					if(r_entry.P_Ext[i-1].Key == id) {
						ASSIGN_PTR(pValue, r_entry.P_Ext[i-1].Val);
						ok = 1;
					}
				}
			}
		}
	}
	return ok;
}

StringStore2::ParseItem::ParseItem() : Kind(0), Flags(0), Group(0), Code(0), SLang(0), Scope(0)
{
}

StringStore2::ParseItem & StringStore2::ParseItem::Z()
{
	Kind = 0;
	Flags = 0;
	Group = 0;
	Code = 0;
	SLang = 0;
	Scope = 0;
	GroupSymb.Z();
	CodeSymb.Z();
	Signature.Z();
	Str.Z();
	return *this;
}

StringStore2::StringStore2() : P_Pb(0), Flags(0), ErrCode(0), ActiveLanguage(0), P_SignatureHash(0)
{
}

StringStore2::~StringStore2()
{
	delete P_Pb;
	delete P_SignatureHash;
}

void StringStore2::SetActiveLanguage(int slang)
{
	ActiveLanguage = slang;
}

#ifndef SC2C // {

bool StringStore2::Load(const char * pFileName, bool selfTest)
{
	bool   ok = true;
	LoadFileName.Z();
	SFile bin_file(pFileName, SFile::mRead|SFile::mBinary);
	THROW(bin_file.IsValid());
	THROW(Read(bin_file));
	{
		SString temp_buf;
		THROW(CreateHash(0));
        if(selfTest) {
			if(P_SignatureHash) {
            	for(uint i = 0; i < SignatureList.getCount(); i++) {
					StrAssocArray::Item item = SignatureList.at_WithoutParent(i);
					uint   hval = 0;
					uint   hpos = 0;
					assert(P_SignatureHash->Search(item.Txt, &hval, &hpos));
					assert(static_cast<long>(hval) == item.Id);
            	}
            }
            for(uint j = 0; j < StrList.getCount(); j++) {
				const LangStrCollItem * p_item = StrList.at(j);
				assert(p_item);
				if(p_item->P_PositionHash) {
					for(uint i = 0; i < p_item->List.getCount(); i++) {
						StrAssocArray::Item item = p_item->List.Get(i);
						long   tpos = 0;
						assert(p_item->P_PositionHash->Get(item.Id, &tpos));
						p_item->List.GetTextByPos((uint)tpos, temp_buf);
						assert(temp_buf == item.Txt);
					}
				}
            }
        }
		LoadFileName = pFileName;
	}
	CATCHZOK
	return ok;
}

int StringStore2::CreateHash(int doTest)
{
	int    ok = -1;
	/* @v12.3.5
	bool   debug_mark = false; // @debug @v12.3.5
	if(!P_SignatureHash) {
		const uint slc = SignatureList.getCount();
		if(slc > 16) { // Если количество сигнатур не больше 16 то нет смысла городить огород с хэшем
			// Размер хэша должен быть точно больше количества сигнатур и при этом должен быть простым числом
			size_t hash_size = GetPrimeGreaterThan(slc*2); // @v12.3.5
			THROW_MEM(P_SignatureHash = new SymbHashTable(hash_size, 0));
			for(uint i = 0; i < slc; i++) {
				StrAssocArray::Item item = SignatureList.at_WithoutParent(i);
				// @v12.3.5 @debug {
				if(sstreq(item.Txt, "bizscore_timeaggrfunc"))
					debug_mark = true;
				// } @v12.3.5 @debug 
				THROW_SL(P_SignatureHash->Add(item.Txt, (uint)item.Id));
			}
			ok = 1;
		}
	}
	*/
	// @v12.3.5 {
	{
		const uint slc = SignatureList.getCount();
		if(slc > 16) { // Если количество сигнатур не больше 16 то нет смысла городить огород с хэшем
			// Размер хэша должен быть точно больше количества сигнатур и при этом должен быть простым числом
			size_t hash_size = GetPrimeGreaterThan(slc*2); // @v12.3.5
			if(!P_SignatureHash) {
				THROW_MEM(P_SignatureHash = new SymbHashTable(hash_size, 0));
			}
			for(uint i = 0; i < slc; i++) {
				StrAssocArray::Item item = SignatureList.at_WithoutParent(i);
				uint   ex_val = 0;
				uint   ex_pos = 0;
				if(P_SignatureHash->Search(item.Txt, &ex_val, &ex_pos)) {
					assert(ex_val == static_cast<uint>(item.Id));
				}
				else {
					THROW_SL(P_SignatureHash->Add(item.Txt, static_cast<uint>(item.Id)));
				}
			}
			ok = 1;
		}
	}
	// } @v12.3.5
	{
		for(uint i = 0; i < StrList.getCount(); i++) {
			LangStrCollItem * p_item = StrList.at(i);
			if(p_item && !p_item->P_PositionHash) {
				const uint lc = p_item->List.getCount();
				if(lc > 16) {
					const size_t hash_size = GetPrimeGreaterThan(lc);
					THROW_MEM(p_item->P_PositionHash = new StringStore2::Hash(hash_size));
					for(uint i = 0; i < lc; i++) {
						StrAssocArray::Item li = p_item->List.at_WithoutParent(i);
						const uint tpos = p_item->List.GetTextPos(i);
						THROW(p_item->P_PositionHash->Add(li.Id, (long)tpos));
					}
					ok = 1;
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int StringStore2::Helper_GetDescription(long ident, SString & rBuf) // @noncost
{
	int    ok = -1;
	rBuf.Z();
	const  int lang = SLS.GetUiLanguageId();
	const  LangStrCollItem * p_item = GetList(lang);
	long   offs = 0;
	int    is_found = 0;
	if(p_item && p_item->DescrPosList.Search(ident, &offs, 0))
		is_found = 1;
	else if(lang != 0) {
		p_item = GetList(0);
		if(p_item && p_item->DescrPosList.Search(ident, &(offs = 0), 0))
			is_found = 1;
	}
	if(is_found && p_item->FileName.NotEmpty()) {
		THROW_SL(::fileExists(p_item->FileName));
		ENTER_CRITICAL_SECTION
		SFile f_in(p_item->FileName, SFile::mRead|SFile::mBinary);
		THROW_SL(f_in.IsValid());
		{
			uint32 tlen = 0;
			int32  rd_ident = 0;
			THROW_SL(f_in.Seek(offs));
			THROW_SL(f_in.Read(&rd_ident, sizeof(rd_ident)));
			THROW(rd_ident == ident); // @todo @err
			THROW_SL(f_in.Read(&tlen, sizeof(tlen)));
			if(tlen) {
				THROW_SL(rBuf.Read(f_in, tlen));
			}
			ok = 1;
		}
		LEAVE_CRITICAL_SECTION
	}
	CATCHZOK
    return ok;
}

SymbHashTable * StringStore2::LoadStringHash(int group) const
{
	SymbHashTable * p_tab = 0;
    if(HashGroupList.lsearch(group)) {
		const  LangStrCollItem * p_item = GetList(0);
		if(p_item) {
			LongArray str_id_list;
			p_item->HashAssocList.GetListByKey(group, str_id_list);
			if(str_id_list.getCount()) {
				SString temp_buf;
				uint  result_count = 0;
				const uint ht_size = GetPrimeGreaterThan((str_id_list.getCount() * 3) - 1);
				THROW_MEM(p_tab = new SymbHashTable(ht_size, 1)); // @v9.6.5 useAssoc 0-->1
				for(uint i = 0; i < str_id_list.getCount(); i++) {
					const long str_id = str_id_list.get(i);
					//const long combine_id = MakeLong(str_id, group);
					if(p_item->Get(str_id, temp_buf)) {
						THROW_SL(p_tab->Add(temp_buf, LoWord(str_id)));
						result_count++;
					}
				}
				if(!result_count) {
					ZDELETE(p_tab);
				}
			}
		}
    }
    CATCH
		ZDELETE(p_tab);
    ENDCATCH
    return p_tab;
}

const SymbHashTable * FASTCALL StringStore2::GetStringHash(int group) // @nonconst @cs
{
	//
	// Если не удалось загрузить хэш-таблицу, то мы все равно вставляем в список PhgL
	// нулевой указатель с идентификатором группы group, дабы при следующем
	// обращении не тратить время на попытку.
	//
	const SymbHashTable * p_result = 0;
	int    _found = 0;
	ENTER_CRITICAL_SECTION
	//
	// Увы, поиск в списке тоже под защитой критической секции. Это меньшее зло
	// чем Read-Write-блокировка.
	//
	for(uint i = 0; !_found && i < PhgL.getCount(); i++) {
		const PreloadedHashGroupEntry * p_entry = PhgL.at(i);
		if(p_entry && p_entry->Group == group) {
			p_result = p_entry->P_Tab;
			_found = 1;
		}
	}
    if(!_found) {
		SymbHashTable * p_new_tab = LoadStringHash(group);
		PreloadedHashGroupEntry * p_new_entry = PhgL.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Group = group;
			p_new_entry->P_Tab = p_new_tab;
			p_result = p_new_entry->P_Tab;
		}
		else
			PPSetErrorSLib();
    }
    LEAVE_CRITICAL_SECTION
    return p_result;
}

int StringStore2::GetDescription(int group, int code, SString & rBuf) // @nonconst
{
	return Helper_GetDescription(MakeLong(code, group), rBuf);
}

int StringStore2::GetDescription(const char * pSignature, SString & rBuf) // @nonconst
{
	rBuf.Z();
	int    ok = 0;
	uint   pos = 0;
	if(P_SignatureHash) {
		uint   hval = 0;
		uint   hpos = 0;
        if(P_SignatureHash->Search(pSignature, &hval, &hpos)) {
			ok = Helper_GetDescription((long)hval, rBuf);
        }
	}
	else if(SignatureList.SearchByText(pSignature, &pos) > 0) {
		long   code_str = SignatureList.Get(pos).Id;
		ok = Helper_GetDescription(code_str, rBuf);
	}
	return ok;
}

#endif // } SC2C

int StringStore2::ExpandString(SString & rBuf, int ctransf /*CTRANSF_XXX*/) const
{
	int    ok = -1;
	if(rBuf.Len() > 3 && rBuf.HasChr('@')) {
		size_t ips = 0;
		size_t ipe = 0;
		while(rBuf.Search("@{", ips, 0, &ips) && rBuf.Search("}", ips+2, 0, &ipe)) {
			SString & r_inner_signature = SLS.AcquireRvlStr();
            rBuf.Sub(ips+2, ipe-(ips+2), r_inner_signature);
			if(r_inner_signature.NotEmpty()) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				if(GetString(r_inner_signature, r_temp_buf)) {
					if(ctransf)
						r_temp_buf.Transf(ctransf);
					rBuf.Excise(ips, ipe-ips+1);
					rBuf.Insert(ips, r_temp_buf);
					ok = 1;
				}
				else
					ips = ips+2;
			}
			else
				ips = ips+2;
		}
	}
	return ok;
}

int StringStore2::Helper_GetString(long ident, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 0;
	const  int lang = SLS.GetUiLanguageId();
	const  LangStrCollItem * p_item = GetList(lang);
	if(p_item && p_item->Get(ident, rBuf))
		ok = 1;
	else if(lang != 0) {
		p_item = GetList(0);
		if(p_item && p_item->Get(ident, rBuf))
			ok = 2;
	}
	if(ok)
		ExpandString(rBuf, 0);
	return ok;
}

int StringStore2::GetString(int group, int code, SString & rBuf) const
{
	return Helper_GetString(MakeLong(code, group), rBuf);
}

int StringStore2::GetString(const char * pSignature, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 0;
	uint   pos = 0;
	if(P_SignatureHash) {
		uint   hval = 0;
		uint   hpos = 0;
        if(P_SignatureHash->Search(pSignature, &hval, &hpos)) {
			ok = Helper_GetString((long)hval, rBuf);
        }
	}
	else if(SignatureList.SearchByText(pSignature, &pos) > 0) {
		long   code_str = SignatureList.Get(pos).Id;
		ok = Helper_GetString(code_str, rBuf);
	}
	return ok;
}

StringStore2::LangStrCollItem * FASTCALL StringStore2::GetOrConstructList(int slang)
{
	LangStrCollItem * p_result = 0;
    for(uint i = 0; !p_result && i < StrList.getCount(); i++) {
		LangStrCollItem * p_item = StrList.at(i);
		if(p_item && p_item->SLang == slang)
			p_result = p_item;
    }
    if(!p_result) {
		p_result = StrList.CreateNewItem();
		if(p_result)
			p_result->SLang = slang;
    }
    return p_result;
}

const StringStore2::LangStrCollItem * FASTCALL StringStore2::GetList(int slang) const
{
	const LangStrCollItem * p_result = 0;
    for(uint i = 0; !p_result && i < StrList.getCount(); i++) {
		const LangStrCollItem * p_item = StrList.at(i);
		if(p_item && p_item->SLang == slang)
			p_result = p_item;
    }
    return p_result;
}

// #define SC2C // @construction

//#ifdef SC2C // {

enum {
	sc2eUsage            = PPERR_SC2_USAGE,             // Usage: SC2C inputFileName
	sc2eGrpCodeLimit     = PPERR_SC2_GRPCODELIMIT,      // Превышение максимального значения кода группы (0x7fff-1)
	sc2eStrCodeLimit     = PPERR_SC2_STRCODELIMIT,      // Превышение максимального значения кода строки (0x7ffff-1)
	sc2eDupSignature     = PPERR_SC2_DUPSIGNATURE,      // Дублирование сигнатуры строки
	sc2eDupStrCode       = PPERR_SC2_DUPSTRCODE,        // Дублирование кода строки
	sc2eDupStrSymb       = PPERR_SC2_DUPSTRSYMB,        // Дублирование символа строки
	sc2eGrpSuccNotFound  = PPERR_SC2_GRPSUCCNOTFOUND,   // Не найдена групповая последовательность
	sc2eSrcFileOpenFault = PPERR_SC2_SRCFILEOPENFAULT,  // Ошибка открытия исходного файла
	sc2eUnfinStrWrap     = PPERR_SC2_UNFINSTRWRAP,      // Перенос строки не завершен
	sc2eUnexpEndOfStr    = PPERR_SC2_UNEXPENDOFSTR,     // Строка должна оканчиваться либо символом '\' либо '"'
	sc2eStxGrp01         = PPERR_SC2_STXGRP01,          // Ошибка в определении группы строк (ожидается ']')
	sc2eStxGrp02         = PPERR_SC2_STXGRP02,          // Ошибка в определении группы строк [symb=number]
	sc2eStxEqAfterCode   = PPERR_SC2_STXEQAFTERCODE,    // Числовое значение не должно предшествовать символу '='
	sc2eStxStrSymbStart  = PPERR_SC2_STXSTRSYMBSTART,   // Символ строки должен начинаться с прописной латинской буквы или '_'
	sc2eStxStrSymbChr    = PPERR_SC2_STXSTRSYMBCHR,     // Символ строки должен содержать либо прописные латинские буквы либо цифры либо '_'
	sc2eStxStr01         = PPERR_SC2_STXSTR01,          // Ошибка в определении строки (symb=number)
	sc2eInvSlashX        = PPERR_SC2_INVSLASHX,         // Не оконченная конструкция '\x00'
	sc2eInvSlashDigit    = PPERR_SC2_INVSLASHDIGIT,     // Не оконченная конструкция '\000'
	sc2eStxLangGrp01     = PPERR_SC2_STXLANGGRP01,      // Ошибка в определении группы языка (ожидается '/')
	sc2eInvLangSymb      = PPERR_SC2_INVLANGSYMB,       // Недопустимый символ языка
	sc2eLangDescrDup     = PPERR_SC2_LANGDESCRDUP,      // Дублирование определения языка для одной строки
	sc2eStrNotIdent      = PPERR_SC2_STRNOTIDENT,       // Строка не идентифицирована (нет определителя и не задан язык)
	sc2eSrcFileNotFound  = PPERR_SC2_SRCFILENOTFOUND,   // Исходный файл не найден
	sc2eSrcFileHasHyphen = PPERR_SC2_SRCFILEHASHYPHEN,  // Имя исходного файла не должно содержать дефисов
	sc2eCodeSymbConflict = PPERR_SC2_CODESYMBCONFLICT,  // Конфликт между символьными идентификаторами строк
	sc2eDupStrById       = PPERR_SC2_DUPSTRBYID,        // Дублирование строки по значению идентификатора
	sc2eStrNotIdentified = PPERR_SC2_STRNOTIDENTIED,    // Строка не имеет идентификации
	sc2eSavedSymbNEqGrp  = PPERR_SC2_SAVEDSYMBNEQGRP,   // При восстановлении сохраненного идентификатора символа строки возник конфликт групп
};

int StringStore2::Error(int errCode, long flags)
{
	SString msg_buf;
	SString temp_buf(4);
	SString ess_msg_buf;
	char   stub[32];
	stub[0] = 0;
	const char * p_file_name = stub;
	int    line = 0;
	if(P_Pb) {
		p_file_name = P_Pb->InFileName;
		line = P_Pb->Line;
	}
#ifdef SC2C // {
	static const SIntToSymbTabEntry ErrMsgList[] = {
		{ PPERR_SC2_USAGE,                  "Usage: SC2C inputFileName" },
		{ PPERR_SC2_GRPCODELIMIT,           "Превышение максимального значения кода группы (0x7fff-1)" },
		{ PPERR_SC2_STRCODELIMIT,           "Превышение максимального значения кода строки (0x7ffff-1)" },
		{ PPERR_SC2_DUPSIGNATURE,           "Дублирование сигнатуры строки" },
		{ PPERR_SC2_DUPSTRCODE,             "Дублирование кода строки" },
		{ PPERR_SC2_DUPSTRSYMB,             "Дублирование символа строки" },
		{ PPERR_SC2_GRPSUCCNOTFOUND,        "Не найдена групповая последовательность" },
		{ PPERR_SC2_SRCFILEOPENFAULT,       "Ошибка открытия исходного файла '%s'" },
		{ PPERR_SC2_UNFINSTRWRAP,           "Перенос строки не завершен" },
		{ PPERR_SC2_UNEXPENDOFSTR,          "Строка должна оканчиваться либо символом '\\' либо '\"'" },
		{ PPERR_SC2_STXGRP01,               "Ошибка в определении группы строк (ожидается ']')" },
		{ PPERR_SC2_STXGRP02,               "Ошибка в определении группы строк [symb=number]" },
		{ PPERR_SC2_STXEQAFTERCODE,         "Числовое значение не должно предшествовать символу '='" },
		{ PPERR_SC2_STXSTRSYMBSTART,        "Символ строки должен начинаться с прописной латинской буквы или '_'" },
		{ PPERR_SC2_STXSTRSYMBCHR,          "Символ строки должен содержать либо прописные латинские буквы либо цифры либо '_'" },
		{ PPERR_SC2_STXSTR01,               "Ошибка в определении строки (symb=number)" },
		{ PPERR_SC2_INVSLASHX,              "Незаконченная конструкция '\x00'" },
		{ PPERR_SC2_INVSLASHDIGIT,          "Незаконченная конструкция '\000'" },
		{ PPERR_SC2_STXLANGGRP01,           "Ошибка в определении группы языка (ожидается '/')" },
		{ PPERR_SC2_INVLANGSYMB,            "Недопустимый символ языка" },
		{ PPERR_SC2_LANGDESCRDUP,           "Дублирование определения языка для одной строки" },
		{ PPERR_SC2_STRNOTIDENT,            "Строка не идентифицирована (нет определителя и не задан язык)" },
		{ PPERR_SC2_SRCFILENOTFOUND,        "Исходный файл не найден" },
		{ PPERR_SC2_SRCFILEHASHYPHEN,       "Имя исходного файла не должно содержать дефисов" },
		{ PPERR_SC2_CODESYMBCONFLICT,       "Конфликт между символьными идентификаторами строк" },
		{ PPERR_SC2_DUPSTRBYID,             "Дублирование строки по значению идентификатора" },
		{ PPERR_SC2_STRNOTIDENTIED,         "Строка не имеет идентификации" },
		{ PPERR_SC2_SAVEDSYMBNEQGRP,        "При восстановлении сохраненного идентификатора символа '%s' возник конфликт групп" }
	};
	SIntToSymbTab_GetSymb(ErrMsgList, SIZEOFARRAY(ErrMsgList), errCode, temp_buf);
	temp_buf.Transf(CTRANSF_UTF8_TO_OUTER);
#else
	PPLoadString(PPMSG_ERROR, errCode, temp_buf);
#endif
	temp_buf.SetIfEmpty("Unknown error");
	ess_msg_buf.Printf(temp_buf, ErrAddedMsg.cptr());
	msg_buf.Printf("%s(%d): error sc2c: %s", p_file_name, line, ess_msg_buf.cptr());
	printf((const char *)msg_buf.CR().ToOem());
	return 0;
}

int StringStore2::StoreState(const char * pFileName, const HashBlock & rHashBlk)
{
	int    ok = 1;
	uint   i;
	SString temp_buf;
    SIniFile ini_file(pFileName, 1, 1, 1);
    THROW(ini_file.IsValid());
	// @v12.3.4 {
	{
		SString key_buf;
		SString val_buf;
		for(uint i = 0; i < rHashBlk.getCount(); i++) {
			const HashEntry * p_he= rHashBlk.at(i);
			if(p_he) {
				if(p_he->LangId == 0)
					key_buf = "base";
				else {
					GetLinguaCode(p_he->LangId, temp_buf);
					if(temp_buf.NotEmpty()) {
						(key_buf = "lang").CatChar('-').Cat(temp_buf);
					}
					else {
						(key_buf = "lang").CatChar('-').Cat(p_he->LangId);
					}
				}
				val_buf.Z();
				val_buf.Cat(p_he->SrcFileName).Comma().Cat(p_he->SrcFileHash.Mime64(temp_buf)).Comma().
					Cat(p_he->BinFileName).Comma().Cat(p_he->BinFileHash.Mime64(temp_buf));
				ini_file.AppendParam("hash", key_buf, val_buf, 1);
			}
		}
		if(rHashBlk.CHdrFileName.NotEmpty() && rHashBlk.CHdrFileHash.Len()) {
			key_buf = "chdr";
			val_buf.Z().Cat(rHashBlk.CHdrFileName).Comma().Cat(rHashBlk.CHdrFileHash.Mime64(temp_buf));
			ini_file.AppendParam("hash", key_buf, val_buf, 1);
		}
		if(rHashBlk.JavaFileName.NotEmpty() && rHashBlk.JavaFileHash.Len()) {
			key_buf = "java";
			val_buf.Z().Cat(rHashBlk.JavaFileName).Comma().Cat(rHashBlk.JavaFileHash.Mime64(temp_buf));
			ini_file.AppendParam("hash", key_buf, val_buf, 1);
		}
	}
	// } @v12.3.4 
    ini_file.AppendParam("common", "LastAutoGrpCode",   temp_buf.Z().Cat(P_Pb->LastAutoGrpCode), 1);
    ini_file.AppendParam("common", "LastSignatureCode", temp_buf.Z().Cat(P_Pb->LastSignatureCode), 1);
    //
    temp_buf.Z();
    for(i = 0; i < P_Pb->LastAutoCodeList.getCount(); i++) {
		const LAssoc & r_item = P_Pb->LastAutoCodeList.at(i);
		if(i)
			temp_buf.Semicol();
		temp_buf.Cat((uint32)r_item.Key).CatChar(',').Cat((uint32)r_item.Val);
    }
    ini_file.AppendParam("common", "LastAutoCodeList", temp_buf, 1);
	//
	for(i = 0; i < SignatureList.getCount(); i++) {
		StrAssocArray::Item item = SignatureList.Get(i);
		ini_file.AppendParam("SignatureList", item.Txt, temp_buf.Z().Cat((uint32)item.Id), 1);
	}
	//
	for(i = 0; i < GrpSymbList.getCount(); i++) {
		StrAssocArray::Item item = GrpSymbList.Get(i);
		ini_file.AppendParam("GrpSymbList", item.Txt, temp_buf.Z().Cat((uint32)item.Id), 1);
	}
	//
	for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->StrSymbList.Get(i);
		ini_file.AppendParam("StrSymbList", item.Txt, temp_buf.Z().Cat((uint32)item.Id), 1);
	}
	//
	THROW(ini_file.FlashIniBuf());
	CATCHZOK
	return ok;
}

bool StringStore2::VerifyHashEntry(int langId, const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk)
{
	bool   ok = false;
	const HashEntry * p_entry = rHshBlk.SearchLangEntry(langId, 0);
	if(p_entry) {
		SString src_path;
		SString bin_path;
		SString src_fnm;
		SString bin_fnm;
		rOrgFileBlk.MakeSrcFilePath(langId, src_path, &src_fnm);
		rOrgFileBlk.MakeBinFilePath(langId, bin_path, &bin_fnm);
		if(fileExists(src_path) && fileExists(bin_path)) {
			if(p_entry->SrcFileName.IsEqiAscii(src_fnm) && p_entry->BinFileName.IsEqiAscii(bin_fnm)) {
				SBinaryChunk src_hash;
				SBinaryChunk bin_hash;
				if(rHshBlk.MakeFileHash(src_path, 0, src_hash) && src_hash == p_entry->SrcFileHash) {
					if(rHshBlk.MakeFileHash(bin_path, 0, bin_hash) && bin_hash == p_entry->BinFileHash) {
						ok = true;
					}
				}
			}
		}
	}
	return ok;
}

bool StringStore2::VerifyHashEntry_CHdr(const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk)
{
	bool   ok = false;
	SString path;
	SString fnm;
	SBinaryChunk hash;
	rOrgFileBlk.MakeCHdrFilePath(path, &fnm);
	if(fileExists(path)) {
		ok = (rHshBlk.CHdrFileName.IsEqiAscii(fnm) && rHshBlk.MakeFileHash(path, 0, hash) && hash == rHshBlk.CHdrFileHash);
	}
	return ok;
}

bool StringStore2::VerifyHashEntry_Java(const HashBlock & rHshBlk, const OrgSrcFileBlock & rOrgFileBlk)
{
	bool   ok = false;
	SString path;
	SString fnm;
	SBinaryChunk hash;
	rOrgFileBlk.MakeJavaFilePath(path, &fnm);
	if(fileExists(path)) {
		ok = (rHshBlk.JavaFileName.IsEqiAscii(fnm) && rHshBlk.MakeFileHash(path, 0, hash) && hash == rHshBlk.JavaFileHash);
	}
	return ok;
}

int StringStore2::RestoreState(const char * pFileName, const StrAssocArray & rFileLangList)
{
    int    ok = 1;
    if(fileExists(pFileName)) {
		SString temp_buf;
		SString item_buf;
		SString key_buf;
		SString val_buf;
        StringSet ss;
        SIniFile ini_file(pFileName, 0, 1, 1);
		THROW(ini_file.IsValid());
		// @v12.3.4 {
		{
			P_Pb->Saved_HashBlk.Z();
			StringSet ss_hash;
			StringSet ss_entry(",");
			SString fnm;
			SString hsh;
			SString fnm_src;
			SString fnm_bin;
			SBinaryChunk bc_src;
			SBinaryChunk bc_bin;
			ini_file.GetEntries2("hash", &ss_hash, 0/*flags*/);
			for(uint ssp = 0; ss_hash.get(&ssp, key_buf);) {
				const int gpr = ini_file.GetParam("hash", key_buf, val_buf);
				assert(gpr > 0); // Это - невозможно. Мы получили ключ только что методом GetEntries
				if(gpr > 0) {
					if(key_buf.IsEqiAscii("chdr")) {
						if(val_buf.Divide(',', fnm, hsh) > 0) {
							if(fnm.NotEmptyS() && P_Pb->Saved_HashBlk.CHdrFileHash.FromMime64(hsh)) {
								P_Pb->Saved_HashBlk.CHdrFileName = fnm;	
							}
						}
					}
					else if(key_buf.IsEqiAscii("java")) {
						if(val_buf.Divide(',', fnm, hsh) > 0) {
							if(fnm.NotEmptyS() && P_Pb->Saved_HashBlk.JavaFileHash.FromMime64(hsh)) {
								P_Pb->Saved_HashBlk.JavaFileName = fnm;	
							}
						}
					}
					else {
						static const char * p_lang_prefix = "lang-";
						const bool is_base = key_buf.IsEqiAscii("base");
						const bool is_lang = (!is_base && key_buf.HasPrefixIAscii(p_lang_prefix));
						if(is_base || is_lang) {
							ss_entry.setBuf(val_buf);
							if(ss_entry.getCount() == 4) {
								uint   ssep = 0;
								bool   local_ok = false;
								fnm_src.Z();
								fnm_bin.Z();
								if(ss_entry.get(&ssep, temp_buf) && temp_buf.NotEmpty()) {
									fnm_src = temp_buf;
									if(ss_entry.get(&ssep, temp_buf) && bc_src.FromMime64(temp_buf)) {
										if(ss_entry.get(&ssep, temp_buf) && temp_buf.NotEmpty()) {
											fnm_bin = temp_buf;
											if(ss_entry.get(&ssep, temp_buf) && bc_bin.FromMime64(temp_buf)) {
												HashEntry * p_new_entry = new HashEntry;
												p_new_entry->SrcFileName = fnm_src;
												p_new_entry->BinFileName = fnm_bin;
												p_new_entry->SrcFileHash = bc_src;
												p_new_entry->BinFileHash = bc_bin;
												if(is_base) {
													p_new_entry->LangId = 0;
												}
												else {
													assert(is_lang);
													key_buf.Sub(strlen(p_lang_prefix), key_buf.Len(), temp_buf);
													p_new_entry->LangId = RecognizeLinguaSymb(temp_buf, 0);
													if(!p_new_entry->LangId) {
														ZDELETE(p_new_entry);
													}
												}
												if(p_new_entry) {
													HashEntry * p_ex_entry = P_Pb->Saved_HashBlk.SearchLangEntry(p_new_entry->LangId, 0);
													if(p_ex_entry) {
														// Вообще-то такого быть не может, но паранойя заставляет рассматривать все варианты
														*p_ex_entry = *p_new_entry;
													}
													else {
														P_Pb->Saved_HashBlk.insert(p_new_entry);
													}
													local_ok = true;
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
		}
		// } @v12.3.4 
		ini_file.GetParam("common", "LastAutoGrpCode", temp_buf);
		P_Pb->LastAutoGrpCode = temp_buf.ToLong();
		ini_file.GetParam("common", "LastSignatureCode", temp_buf);
		P_Pb->LastSignatureCode = temp_buf.ToULong();
		//
		{
			P_Pb->LastAutoCodeList.clear();
			ini_file.GetParam("common", "LastAutoCodeList", temp_buf);
			StringSet ss(';', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
				if(item_buf.Divide(',', key_buf, val_buf) > 0) {
					P_Pb->LastAutoCodeList.Add((long)key_buf.ToULong(), (long)val_buf.ToULong(), 0, 0);
				}
			}
		}
		//
		{
			SignatureList.Z();
			P_Pb->Saved_SignatureList.Z();
			ss.Z();
			ini_file.GetEntries2("SignatureList", &ss, SIniFile::gefStoreAllString);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					P_Pb->Saved_SignatureList.Add((long)val_buf.Strip().ToULong(), key_buf.Strip(), 1);
                }
			}
		}
		//
		{
			GrpSymbList.Z();
			P_Pb->Saved_GrpSymbList.Z();
			ss.Z();
			ini_file.GetEntries2("GrpSymbList", &ss, SIniFile::gefStoreAllString);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					P_Pb->Saved_GrpSymbList.Add((long)val_buf.Strip().ToULong(), key_buf.Strip(), 1);
                }
			}
		}
		//
		{
			P_Pb->StrSymbList.Z();
			P_Pb->Saved_StrSymbList.Z();
			ss.Z();
			ini_file.GetEntries2("StrSymbList", &ss, SIniFile::gefStoreAllString);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					P_Pb->Saved_StrSymbList.Add((long)val_buf.Strip().ToULong(), key_buf.Strip(), 1);
                }
			}
		}
    }
    else
		ok = -1;
    CATCHZOK
    return ok;
}

int StringStore2::TranslationWanted(const uchar * pOrgText) const
{
	const  uchar * p_notrasl_chars = reinterpret_cast<const uchar *>(".,:; \t\n\r-_(){}[]<>!=$%^&\'\"*`@#\\/+~0123456789");
    int    yes = 0;
    const  size_t len = sstrlen(pOrgText);
    if(len) {
		for(const uchar * p = pOrgText; !yes && *p; p++) {
			const uchar c = p[0];
			if(c == '@' && p[1] == '{') {
				p += 2;
				while(p[0] && p[0] != '}')
					p++;
				if(p[0] == 0) {
					// Специальный случай - без этого проверка в головном цикле сбойнет и цикл уйдет за пределы '\0'
					// Такая ситуация - ошибочна - не закрытая справа конструкция @{...}
					break;
				}
			}
			else if(c == '\\' && oneof3(p[1], 'n', 't', 'r'))
				p++;
			else if(!sstrchr((const char *)p_notrasl_chars, c)) {
				if(isasciialpha(c)) {
					yes = 1;
				}
				else {
					uint16 extra = SUtfConst::TrailingBytesForUTF8[c];
					if(((size_t)(p - pOrgText) + extra) < len) {
						if(SUnicode::IsLegalUtf8Char(p, extra+1)) {
							yes = 1; // валидный utf8 символ - считаем что перевод нужен
						}
					}
				}
			}
		}
    }
    return yes;
}

int Helper_PPAutoTranslSvc_Microsoft_Auth(PPAutoTranslSvc_Microsoft & rAt);

static void PreserveStringForAutotraslation(const SString & rOrgBuf, SString & rResultBuf, StrAssocArray & rAssocList)
{
	rAssocList.Z();
	rResultBuf.Z();
	const size_t org_len = rOrgBuf.Len();
	SString preserve_buf;
	SString subst_buf;
	long   subst_n = 0;
	for(size_t p = 0; p < org_len;) {
		const char c = rOrgBuf.C(p);
		const char c2 = rOrgBuf.C(p+1);
		if(c == '@' && c2 == '{') {
			subst_buf.Z();
			preserve_buf.Z().CatChar(c).CatChar(c2);
			size_t p2 = p+2;
			do {
				const char c3 = rOrgBuf.C(p2++);
				if(isasciialnum(c3) || oneof2(c3, '_', '-')) {
					preserve_buf.CatChar(c3);
				}
				else if(c3 == '}') {
					preserve_buf.CatChar(c3);
					if(rOrgBuf.C(p2) == ':') {
						preserve_buf.CatChar(rOrgBuf.C(p2++));
						while(rOrgBuf.C(p2) == ' ') {
							preserve_buf.CatChar(rOrgBuf.C(p2++));
						}
					}
					break;
				}
				else {
					preserve_buf.Z();
					break;
				}
			} while(1);
			if(preserve_buf.NotEmpty()) {
				(subst_buf = "#").CatLongZ(++subst_n, 3);
				rAssocList.Add(subst_n, preserve_buf);
				rResultBuf.Cat(subst_buf);
				p = p2;
			}
			else {
				rResultBuf.CatChar(c);
				p++;
			}
		}
		else if(oneof4(c, '[', '(', '{', '\'')) {
			char end_c = 0;
			if(c == '[')
				end_c = ']';
			else if(c == '(')
				end_c = ')';
			else if(c == '\'')
				end_c = '\'';
			else if(c == '{')
				end_c = '}';
			assert(end_c != 0);
			subst_buf.Z();
			preserve_buf.Z().CatChar(c);
			size_t p2 = p+1;
			do {
				const char c3 = rOrgBuf.C(p2++);
				if(c3 == end_c) {
					preserve_buf.CatChar(c3);
					if(rOrgBuf.C(p2) == ':') {
						preserve_buf.CatChar(rOrgBuf.C(p2++));
						while(rOrgBuf.C(p2) == ' ') {
							preserve_buf.CatChar(rOrgBuf.C(p2++));
						}
					}
					break;
				}
				else if(isasciialnum(c3) || oneof12(c3, '_', '-', '+', '/', '%', '=', ' ', '.', ',', ':', ';', '@')) {
					preserve_buf.CatChar(c3);
				}
				else {
					preserve_buf.Z();
					break;
				}
			} while(1);
			if(preserve_buf.NotEmpty()) {
				(subst_buf = "#").CatLongZ(++subst_n, 3);
				rAssocList.Add(subst_n, preserve_buf);
				rResultBuf.Cat(subst_buf);
				p = p2;
			}
			else {
				rResultBuf.CatChar(c);
				p++;
			}
		}
		else {
			rResultBuf.CatChar(c);
			p++;
		}
	}
}

static void RecoverAutotranslatedString(SString & rBuf, StrAssocArray & rAssocList)
{
	SString subst_buf;
	rBuf.ReplaceStr("% s", "%s", 0);
	rBuf.ReplaceStr("% ld", "%ld", 0);
	rBuf.ReplaceStr("% lf", "%lf", 0);
	rBuf.ReplaceStr("% d", "%d", 0);
	rBuf.ReplaceStr("%. ", "%.", 0);
	rBuf.ReplaceStr(":%", "% :", 0);
	rBuf.ReplaceStr("\'%s \'", "\'%s\'", 0);
	rBuf.ReplaceStr("@ {", "@{", 0);
	for(uint i = 0; i < rAssocList.getCount(); i++) {
		StrAssocArray::Item item = rAssocList.at_WithoutParent(i);
		if(item.Id > 0 && item.Id < 1000) {
			(subst_buf = "#").CatLongZ(item.Id, 3);
			rBuf.ReplaceStr(subst_buf, item.Txt, 1);
		}
	}
	// @debug {
	{
		int  there_is_a_problem = 0;
		size_t debug_pos = 0;
		if(rBuf.Search("#0", 0, 0, &debug_pos)) {
			there_is_a_problem = 1;
		}
	}
	// } @debug
	rBuf.ElimDblSpaces();
}

StringStore2::PreloadedHashGroupEntry::PreloadedHashGroupEntry() : Group(0), P_Tab(0)
{
}

StringStore2::PreloadedHashGroupEntry::~PreloadedHashGroupEntry()
{
	ZDELETE(P_Tab);
}

StringStore2::HashEntry::HashEntry() : LangId(0)
{
}

StringStore2::HashEntry::~HashEntry()
{
}

StringStore2::HashEntry & StringStore2::HashEntry::Z()
{
	LangId = 0;
	SrcFileName.Z();
	BinFileName.Z();
	SrcFileHash.Z();
	BinFileHash.Z();
	return *this;
}

StringStore2::HashBlock::HashBlock() : TSCollection <HashEntry>(), HashFunc(SHASHF_SHA256)
{
}

StringStore2::HashBlock & StringStore2::HashBlock::Z()
{
	CHdrFileName.Z();
	JavaFileName.Z();
	CHdrFileHash.Z();
	JavaFileHash.Z();
	freeAll();
	return *this;
}

StringStore2::HashEntry * StringStore2::HashBlock::SearchLangEntry(int langId, uint * pPos) const
{
	HashEntry * p_entry = 0;
	uint   pos = 0;
	for(uint i = 0; !p_entry && i < getCount(); i++) {
		HashEntry * p_iter_entry = at(i);
		if(p_iter_entry && p_iter_entry->LangId == langId) {
			pos = i;
			p_entry = p_iter_entry;
		}
	}
	ASSIGN_PTR(pPos, pos);
	return p_entry;
}

void StringStore2::HashBlock::CopyCHdrHash(const HashBlock & rS)
{
	CHdrFileName = rS.CHdrFileName;
	CHdrFileHash = rS.CHdrFileHash; 
}

void StringStore2::HashBlock::CopyJavaHash(const HashBlock & rS)
{
	JavaFileName = rS.JavaFileName;
	JavaFileHash = rS.JavaFileHash; 
}

bool StringStore2::HashBlock::CopyEntry(int langId, const HashBlock & rS)
{
	bool   ok = false;
	const  HashEntry * p_src_entry = rS.SearchLangEntry(langId, 0);
	if(p_src_entry) {
		HashEntry * p_ex_entry = SearchLangEntry(langId, 0);
		if(p_ex_entry) {
			*p_ex_entry = *p_src_entry;
			ok = true;
		}
		else {
			HashEntry * p_new_entry = new HashEntry(*p_src_entry);
			ok = (p_new_entry && insert(p_new_entry));
		}	
	}
	return ok;
}

bool StringStore2::HashBlock::MakeFileHash(const char * pFilePath, SString * pResultFileName, SBinaryChunk & rHash) const
{
	CALLPTRMEMB(pResultFileName, Z());
	rHash.Z();
	bool   ok = true;
	SString file_path;
	{
		SFsPath::NormalizePath(pFilePath, SFsPath::npfCompensateDotDot, file_path);
		SFile f(file_path, SFile::mRead|SFile::mBinary|SFile::mNoStd);
		THROW(f.IsValid());
		THROW(f.CalcHash(0, HashFunc, rHash));
		if(pResultFileName) {
			SFsPath ps(file_path);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, *pResultFileName);
		}
	}
	CATCHZOK
	return ok;
}

bool StringStore2::HashBlock::MakeCHdrHash(const char * pFilePath) { return MakeFileHash(pFilePath, &CHdrFileName, CHdrFileHash); }
bool StringStore2::HashBlock::MakeJavaHash(const char * pFilePath) { return MakeFileHash(pFilePath, &JavaFileName, JavaFileHash); }

int StringStore2::HashBlock::CreateEntry(int langId, const char * pSrcFilePath, const char * pBinFilePath)
{
	int    ok = 1;
	HashEntry * p_entry = 0;
	SFsPath ps;
	SString file_path;
	assert(HashFunc);
	THROW(HashFunc);
	THROW(p_entry = new HashEntry);
	p_entry->LangId = langId;
	THROW(MakeFileHash(pSrcFilePath, &p_entry->SrcFileName, p_entry->SrcFileHash));
	THROW(MakeFileHash(pBinFilePath, &p_entry->BinFileName, p_entry->BinFileHash));
	{
		HashEntry * p_ex_entry = SearchLangEntry(langId, 0);
		if(p_ex_entry) {
			*p_ex_entry = *p_entry;
		}
		else {
			insert(p_entry);
		}
	}
	CATCH
		ok = 0;
		ZDELETE(p_entry);
	ENDCATCH
	return ok;
}

StringStore2::ParseBlock::ParseBlock() : Line(0), LastAutoGrpCode(1000), LastSignatureCode(0x80000000)
{
}

StringStore2::ParseBlock::~ParseBlock()
{
}

SString & StringStore2::OrgSrcFileBlock::MakeSrcFilePath(int langId, SString & rFilePath, SString * pFileName) const
{
	rFilePath.Z();
	CALLPTRMEMB(pFileName, Z());
	if(!(State & stError)) {
		SFsPath ps(Ps);
		ps.Nam = GetOrgName();
		if(langId) {
			SString temp_buf;
            GetLinguaCode(langId, temp_buf);
			ps.Nam.CatChar('-').Cat(temp_buf);
		}
		ps.Ext = GetOrgExt();
		ps.Merge(rFilePath);
		if(pFileName)
			ps.Merge(SFsPath::fNam|SFsPath::fExt, *pFileName);
	}
	return rFilePath;
}

SString & StringStore2::OrgSrcFileBlock::MakeBinFilePath(int langId, SString & rFilePath, SString * pFileName) const
{
	rFilePath.Z();
	CALLPTRMEMB(pFileName, Z());
	if(!(State & stError)) {
		SFsPath ps(Ps);
		ps.Nam = GetOrgName();
		if(langId) {
			SString temp_buf;
            GetLinguaCode(langId, temp_buf);
			ps.Nam.CatChar('-').Cat(temp_buf);
		}
		ps.Ext = "bin";
		ps.Merge(rFilePath);
		if(pFileName)
			ps.Merge(SFsPath::fNam|SFsPath::fExt, *pFileName);
	}
	return rFilePath;
}

SString & StringStore2::OrgSrcFileBlock::MakeFilePathByChangingExt(const char * pExt, SString & rFilePath, SString * pFileName) const
{
	assert(!isempty(pExt));
	rFilePath.Z();
	CALLPTRMEMB(pFileName, Z());
	if(!(State & stError)) {
		SFsPath ps(Ps);
		ps.Ext = pExt;
		ps.Merge(rFilePath);
		if(pFileName)
			ps.Merge(SFsPath::fNam|SFsPath::fExt, *pFileName);
	}
	return rFilePath;
}

SString & StringStore2::OrgSrcFileBlock::MakeCHdrFilePath(SString & rFilePath, SString * pFileName) const
{
	return MakeFilePathByChangingExt("h", rFilePath, pFileName);
}

SString & StringStore2::OrgSrcFileBlock::MakeJavaFilePath(SString & rFilePath, SString * pFileName) const
{
	return MakeFilePathByChangingExt("java", rFilePath, pFileName);
}

void StringStore2::OrgSrcFileBlock::MakeIdentFilePath(SString & rFilePath) const // *.symb
{
	MakeFilePathByChangingExt("symb", rFilePath, 0);
}

void StringStore2::OrgSrcFileBlock::MakeLangSrcFileSearchPattern(SString & rPattern) const
{
	rPattern.Z();
	if(!(State & stError)) {
		SFsPath ps(Ps);
		ps.Ext = GetOrgExt();
		(ps.Nam = GetOrgName()).CatChar('-').CatChar('*');
		ps.Merge(rPattern);
	}
}

void StringStore2::OrgSrcFileBlock::MakeAutotranslateFilePath(SString & rFilePath) const
{
	rFilePath.Z();
	if(!(State & stError) && AutoTranslLang) {
		SString temp_buf;
		GetLinguaCode(AutoTranslLang, temp_buf);
		SFsPath ps(Ps);
		ps.Nam.Z().Cat("auto").CatChar('-').Cat(GetOrgName()).CatChar('-').Cat(temp_buf);
		ps.Ext = GetOrgExt();
		ps.Merge(rFilePath);		
	}
}

StringStore2::OrgSrcFileBlock::OrgSrcFileBlock(const char * pFileName, const char * pTranslateToLang) : State(0), AutoTranslLang(0), Ps(pFileName)
{
	EXCEPTVAR(PrivateErrCode);
	
	SString temp_buf;
	THROW_V(fileExists(pFileName), sc2eSrcFileNotFound);
	{
		THROW_V(!Ps.Nam.HasChr('-'), sc2eSrcFileHasHyphen);
		FileLangList.Add(0, pFileName);
		//OrgName = Ps.Nam;
		//OrgExt = Ps.Ext;
		//
		MakeIdentFilePath(IdentFileName);
		MakeLangSrcFileSearchPattern(temp_buf);
		{
        	SString lang_symb;
			SString _iter_file_name;
			SDirEntry de;
			SFsPath ps_lang;
			SFsPath local_ps(Ps);
			for(SDirec direc(temp_buf, 0); direc.Next(&de) > 0;) {
				if(!de.IsFolder()) {
					de.GetNameA(_iter_file_name);
					size_t hyphen_pos = 0;
					if(_iter_file_name.SearchChar('-', &hyphen_pos)) {
						_iter_file_name.Sub(hyphen_pos+1, _iter_file_name.Len(), lang_symb.Z());
						const int slang = RecognizeLinguaSymb(lang_symb, 0);
						if(slang) {
							ps_lang.Split(_iter_file_name);
							local_ps.Nam = ps_lang.Nam;
							local_ps.Ext = ps_lang.Ext;
							local_ps.Merge(temp_buf);
							FileLangList.Add(slang, temp_buf);
						}
					}
				}
			}
		}
	}
	if(!isempty(pTranslateToLang)) {
		THROW_V(AutoTranslLang = RecognizeLinguaSymb(pTranslateToLang, 0), PPERR_SC2_INVLANGSYMB);
		MakeAutotranslateFilePath(AutoTranslFile);
		//
		// При необходимости автоперевода мы в первую очередь должны будем разобрать файл с тем
		// языком, на который будет осуществляться перевод для того, чтобы при переводе
		// можно было выяснять есть у нас перевод какой-то строки или нет.
		// Контекст проблемы: после автоперевода результат доводится в ручную и складывается
		// в регулярный файл строк на нужном языке.
		// Что бы, не дай бог, не перезаписать результаты ручной правки мы будет переводить
		// только строки, которые не были переведены ранее.
		//
		for(uint fli = 0; fli < FileLangList.getCount(); fli++) {
			StrAssocArray::Item fl_item = FileLangList.Get(fli);
			if(fl_item.Id == AutoTranslLang) {
				FileLangList.Swap(0, fli);
                break;
			}
		}
	}
	CATCH
		State |= stError;
	ENDCATCH
}

int StringStore2::Compile(const char * pFileName, long flags, const char * pTranslateToLang)
{
	EXCEPTVAR(ErrCode);
	//SProfile prfl; // @v11.1.11
	//const  uint64 _ptstart = prfl.GetAbsTimeMicroseconds(); // @v11.1.11
	const  int64 _clk_start = clock(); // @v11.1.11
	int    ok = 1;
	SString temp_buf;
	SString final_line_buf;
	SString intr_at_text;
	HashBlock actual_hash_block; // @v12.3.4
	SFsPath ps;
	PPAutoTranslSvc_Microsoft * p_at = 0;
	int    at_auth_error = 0;
	LAssocArray is_unmodif_list; // Key - lang_id; Val - is-unmodified tag
	bool   is_unmodif_base = false;
	bool   is_unmodif_chdr = false;
	bool   is_unmodif_java = false;
	bool   is_unmodif_all = false;
	const  OrgSrcFileBlock org_file_blk(pFileName, pTranslateToLang);
	THROW_V(org_file_blk, org_file_blk.PrivateErrCode);
	THROW(InitParsing());
	{
		SStrScan scan;
		SString line_buf;
		SString auto_transl_string;
		SString org_text;
		SString string_to_translate;
		StrAssocArray subst_list;
		//
		THROW(RestoreState(org_file_blk.IdentFileName, org_file_blk.FileLangList));
		{
			is_unmodif_all = true; 
			is_unmodif_chdr = VerifyHashEntry_CHdr(P_Pb->Saved_HashBlk, org_file_blk);
			if(!is_unmodif_chdr)
				is_unmodif_all = false;
			is_unmodif_java = VerifyHashEntry_Java(P_Pb->Saved_HashBlk, org_file_blk);
			if(!is_unmodif_java)
				is_unmodif_all = false;
			for(uint i = 0; i < org_file_blk.FileLangList.getCount(); i++) {
				StrAssocArray::Item fl_item = org_file_blk.FileLangList.Get(i);
				const bool is_unmodif = VerifyHashEntry(fl_item.Id, P_Pb->Saved_HashBlk, org_file_blk);
				if(!is_unmodif)
					is_unmodif_all = false;
				is_unmodif_list.Add(fl_item.Id, is_unmodif);
				if(fl_item.Id == 0) {
					is_unmodif_base = is_unmodif;
				}
			}
		}
		if(is_unmodif_all && !(flags & cfTranslateOnly)) {
			temp_buf.Z().Cat("Nothing to do").CatDiv(':', 2).Cat("nothing changed");
			puts(temp_buf);
		}
		else {
			for(uint fli = 0; fli < org_file_blk.FileLangList.getCount(); fli++) {
				StrAssocArray::Item fl_item = org_file_blk.FileLangList.Get(fli);
				StringStore2::ParseItem item;
				StringStore2::ParseItem last_group;
				StringStore2::ParseItem last_lang;
				SetActiveLanguage(fl_item.Id);
				const  int local_auto_transl_lang = (fl_item.Id == 0) ? org_file_blk.AutoTranslLang : 0;
				long   last_string_id = 0;
				long   last_string_lang = 0;
				bool   skip_iteration = false;
				/*if(!local_auto_transl_lang) {
					long is_unmodif_local = 0;
					uint local_pos = 0;
					if(is_unmodif_list.Search(fl_item.Id, &local_pos)) {
						is_unmodif_local = is_unmodif_list.at(local_pos).Val;
					}
					if(is_unmodif_local) {
						skip_iteration = true;
						temp_buf.Z().Cat("Compilation of").Space().Cat(fl_item.Txt).Space().Cat("skipped due to lack of changes");
						puts(temp_buf);
					}
				}*/
				if(!skip_iteration) {
					{
						temp_buf.Z().Cat("Compilation of").Space().Cat(fl_item.Txt);
						puts(temp_buf);
					}
					SFile  f_at;
					P_Pb->Line = 0;
					P_Pb->InFileName = fl_item.Txt;
					SFile f_in(P_Pb->InFileName, SFile::mRead);
					ErrAddedMsg = P_Pb->InFileName;
					THROW_V(f_in.IsValid(), sc2eSrcFileOpenFault); // @str Ошибка открытия исходного файла
					if(local_auto_transl_lang) {
						THROW(f_at.Open(org_file_blk.AutoTranslFile, SFile::mWrite));
					}
					while(f_in.ReadLine(line_buf)) {
						++P_Pb->Line;
						const int r = ParseString(line_buf, scan, item);
						THROW(r);
						if(r > 0) {
							if(item.Kind == StringStore2::ParseItem::kGroup) {
								last_string_id = 0;
								last_string_lang = 0;
								THROW(AddGroup(item));
								last_group = item;
								if(local_auto_transl_lang) {
									f_at.WriteLine(line_buf);
								}
							}
							else if(item.Kind == StringStore2::ParseItem::kScopeStart) {
								if(item.Scope == item.scopeNoTranslate)
									Flags |= fNoTranslateScope;
								else if(item.Scope == item.scopeDescr)
									Flags |= fDescrScope;
							}
							else if(item.Kind == StringStore2::ParseItem::kScopeEnd) {
								if(item.Scope == item.scopeNoTranslate)
									Flags &= ~fNoTranslateScope;
								else if(item.Scope == item.scopeDescr) {
									Flags &= ~fDescrScope;
									if(item.Str.NotEmpty() && last_string_id) {
										item.Kind = StringStore2::ParseItem::kDescription;
										{
											uint   _pos = 0;
											LangStrCollItem * p_coll_item = GetOrConstructList(last_string_lang);
											THROW(p_coll_item);
											THROW(p_coll_item->List.Search(last_string_id, &_pos)); // @todo @error
											THROW(p_coll_item->DescrList.Add(last_string_id, item.Str));
										}
									}
								}
							}
							else if(item.Kind == StringStore2::ParseItem::kGroupLang) {
								SetActiveLanguage(item.SLang);
							}
							else if(item.Kind == StringStore2::ParseItem::kString && !(item.Flags & StringStore2::ParseItem::fNextLine)) {
								bool   dont_translate = (Flags & fNoTranslateScope || item.Flags & item.fNoTranslate || !TranslationWanted(item.Str.ucptr()));
								THROW(AddString(last_group.Group, item, &last_string_id));
								last_string_lang = item.SLang;
								if(!dont_translate && local_auto_transl_lang && ActiveLanguage == 0) {
									//
									// Непосредственно до вызова функции автоперевода проверим нет ли уже готового перевода нашей строки
									//
									const LangStrCollItem * p_coll_item = GetList(org_file_blk.AutoTranslLang);
									if(p_coll_item && p_coll_item->List.Search(last_string_id))
										dont_translate = true;
								}
								if(!dont_translate && local_auto_transl_lang && ActiveLanguage == 0) {
									org_text = item.Str;
									org_text.ReplaceStr("\\n\\003", " ", 0);
									org_text.ReplaceStr("\003", "", 0);
									org_text.ReplaceStr("\n", " ", 0);
									temp_buf.Z().Cat("\t//#org").Space().Cat(org_text).CR();
									f_at.WriteLine(temp_buf);
									//
									final_line_buf.Z();
									if(item.CodeSymb.NotEmpty() && item.CodeSymb != P_StrSt_EmptyCodeSymb) {
										final_line_buf.Cat(item.CodeSymb);
									}
									else if(item.Signature.NotEmpty()) {
										final_line_buf.CatChar('@').Cat(item.Signature);
									}
									else {
										THROW_V(item.Code != 0, sc2eStrNotIdentified);
										final_line_buf.Cat(item.Code);
									}
									final_line_buf.Space();
									{
#ifdef SC2C // {
										auto_transl_string = org_text;
#else
										if(!p_at && !at_auth_error) {
											p_at = new PPAutoTranslSvc_Microsoft;
											if(p_at) {
												if(!Helper_PPAutoTranslSvc_Microsoft_Auth(*p_at)) {
													ZDELETE(p_at);
													at_auth_error = 1;
												}
											}
										}
										if(p_at) {
											if(org_text.HasChr(';')) {
												auto_transl_string.Z();
												StringSet ss(';', org_text);
												for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
													if(auto_transl_string.NotEmpty())
														auto_transl_string.CatChar(';');
													{
														//
														// Специальный препроцессинг префиксов "number," (e.g. "2,...;3,...")
														//
														size_t _dec_count = 0;
														if(isdec(temp_buf.C(_dec_count))) {
															do {
																_dec_count++;
															} while(isdec(temp_buf.C(_dec_count)));
															if(temp_buf.C(_dec_count) == ',') {
																for(uint cidx = 0; cidx <= _dec_count; cidx++)
																	auto_transl_string.CatChar(temp_buf.C(cidx));
																temp_buf.ShiftLeft(_dec_count+1);
															}
														}
													}
													//
													// Специальный препроцессинг подстановок (@symb or @{symb})
													//
													int    is_subst = 1;
													if(temp_buf.C(0) == '@' && !temp_buf.HasChr(' ')) {
														size_t c_first_pos = 1;
														size_t c_last_pos = temp_buf.Len()-1;
														if(temp_buf.C(c_first_pos) == '{' && temp_buf.C(c_last_pos) == '}') {
															c_first_pos++;
															c_last_pos--;
														}
														for(uint cidx = c_first_pos; is_subst && cidx <= c_last_pos; cidx++) {
															const char c2 = temp_buf.C(cidx);
															if(!isasciialnum(c2) && !oneof2(c2, '_', '-'))
																is_subst = 0;
														}
													}
													else
														is_subst = 0;
													if(is_subst) {
														auto_transl_string.Cat(temp_buf);
													}
													else {
														PreserveStringForAutotraslation(temp_buf, string_to_translate, subst_list);
														if(p_at->Request(slangRU, org_file_blk.AutoTranslLang, /*temp_buf*/string_to_translate, intr_at_text) > 0) {
															RecoverAutotranslatedString(intr_at_text, subst_list);
															auto_transl_string.Cat(intr_at_text);
														}
														else
															auto_transl_string.Cat("error-autotranslation");
													}
												}
											}
											else {
												PreserveStringForAutotraslation(org_text, string_to_translate, subst_list);
												if(p_at->Request3(slangRU, org_file_blk.AutoTranslLang, /*org_text*/string_to_translate, auto_transl_string) > 0) { // @v12.4.2 Request-->Request3
													RecoverAutotranslatedString(auto_transl_string, subst_list);
												}
												else
													auto_transl_string = "error-autotranslation";
											}
										}
										else
											auto_transl_string = "error-autotranslation";
#endif
										final_line_buf.CatQStr(auto_transl_string);
									}
									f_at.WriteLine(final_line_buf.CR());
								}
							}
						}
						else if(local_auto_transl_lang) {
							f_at.WriteLine(line_buf);
						}
					}
					THROW_V(!(item.Flags & StringStore2::ParseItem::fNextLine), sc2eUnfinStrWrap); // @str Перенос строки не завершен
				}
			}
			if(!(flags & cfTranslateOnly)) {
				SString out_file_name;
				SString src_file_name;
				for(uint i = 0; i < StrList.getCount(); i++) {
					LangStrCollItem * p_item = StrList.at(i);
					if(p_item) {
						bool skip_iteration = false;
						{
							long is_unmodif_local = 0;
							uint local_pos = 0;
							if(is_unmodif_list.Search(p_item->SLang, &local_pos)) {
								is_unmodif_local = is_unmodif_list.at(local_pos).Val;
							}
							if(is_unmodif_local) {
								skip_iteration = true;
							}
						}
						if(skip_iteration) {
							actual_hash_block.CopyEntry(p_item->SLang, P_Pb->Saved_HashBlk);
						}
						else {
							org_file_blk.MakeBinFilePath(p_item->SLang, out_file_name, 0);
							org_file_blk.MakeSrcFilePath(p_item->SLang, src_file_name, 0);
							{
								// Область выделена для того, чтобы файл закрылся перед тем, как будет считаться хэш этого файла вызовом actual_hash_block.CreateEntry()
								SFile bin_file(out_file_name.ToLower(), SFile::mReadWriteTrunc|SFile::mBinary);
								THROW(bin_file.IsValid());
								THROW(Write(bin_file, p_item));
							}
							if(actual_hash_block.CreateEntry(p_item->SLang, src_file_name, out_file_name)) {
								;
							}
							else {
								;
							}
						}
					}
				}
				if(/*is_unmodif_chdr && is_unmodif_base*/false) {
					actual_hash_block.CopyCHdrHash(P_Pb->Saved_HashBlk);
				}
				else {
					THROW(GenerateHeaderFile(org_file_blk.MakeCHdrFilePath(out_file_name, 0).ToLower(), pFileName));
					actual_hash_block.MakeCHdrHash(out_file_name);
				}
				if(/*is_unmodif_java && is_unmodif_base*/false) {
					actual_hash_block.CopyJavaHash(P_Pb->Saved_HashBlk);
				}
				else {
					THROW(GenerateJavaFile(org_file_blk.MakeJavaFilePath(out_file_name, 0).ToLower(), pFileName)); // @v11.1.5
					actual_hash_block.MakeJavaHash(out_file_name);
				}
				//
				if(!is_unmodif_all) {
					THROW(StoreState(org_file_blk.IdentFileName, actual_hash_block));
				}
#if 0 // {
				//
				// TEST
				// Считываем созданный бинарный файл, затем записываем его в файл с тем же именем и
				// расширением .TEST и сравниваем оригинальный файл с новым: они должны совпасть байт-в-байт.
				//
				// Начиная с версии 9.0.8 такой простой трюк не работает из-за того, что подробные описания строк
				// теперь хранятся в файле и считываются только по запросу
				//
				{
					StringStore2 test_s;
					org_file_blk.MakeBinFilePath(0, file_name);
					SFile bin_file(file_name.ToLower(), SFile::mRead|SFile::mBinary);
					THROW(test_s.Read(bin_file));
					for(uint i = 0; i < StrList.getCount(); i++) {
						LangStrCollItem * p_item = StrList.at(i);
						if(p_item) {
							test_file_name = file_name;
							SFsPath::ReplaceExt(test_file_name, "test", 1);
							SFile test_bin_file(test_file_name.ToLower(), SFile::mReadWriteTrunc|SFile::mBinary);
							THROW(test_bin_file.IsValid());
							THROW(test_s.Write(test_bin_file, p_item));
						}
					}
				}
				THROW(SFile::Compare(file_name, test_file_name, 0));
				SFile::Remove(test_file_name);
#endif // } 0
			}
		}
	}
	// @v11.1.11 {
	{
		//const  uint64 _ptend = prfl.GetAbsTimeMicroseconds();
		const  int64 _clk_end = clock(); // @v11.1.11
		temp_buf.Z().Cat("Execution time").CatDiv(':', 2).Cat(_clk_end-_clk_start).Cat("ms").CR();
		puts(temp_buf);
	}
	// } @v11.1.11
	CATCH
		ok = Error(ErrCode, 0);
	ENDCATCH
#ifndef SC2C
	delete p_at;
#endif
	return ok;
}

int StringStore2::Write(SFile & rF, LangStrCollItem * pData)
{
	int    ok = 1;
	StrgHeader hdr;
	MEMSZERO(hdr);
	hdr.Signature[0] = 'S';
	hdr.Signature[1] = 'C';
	hdr.Signature[2] = '2';
	hdr.Signature[3] = 'B';
	hdr.SLang = pData->SLang;
	hdr.Ver = 3;
	THROW(rF.IsValid());
	rF.Seek(0);
	THROW(rF.Write(&hdr, sizeof(hdr)));
	hdr.ListOffs = (uint32)rF.Tell();
	THROW(pData->List.Write(rF, 0));
	hdr.SListOffs = (uint32)rF.Tell();
	THROW(SignatureList.Write(rF, 0));
	{
		HashGroupList.sortAndUndup();
		pData->HashAssocList.Sort();
		//
		THROW(GrpSymbList.Write(rF, 0));
		THROW(HashGroupList.Write(rF, 0));
		THROW(pData->HashAssocList.Write(rF, 0));
	}
	{
		pData->DescrPosList.freeAll();
		for(uint i = 0; i < pData->DescrList.getCount(); i++) {
            long   offs = rF.Tell();
            const  StrAssocArray::Item item = pData->DescrList.Get(i);
            const  size_t tlen = sstrlen(item.Txt);
            THROW(rF.Write(&item.Id, sizeof(item.Id)));
			THROW(rF.Write(&tlen, sizeof(tlen)));
			if(tlen)
				THROW(rF.Write(item.Txt, tlen));
			THROW(pData->DescrPosList.Add(item.Id, offs, 0));
		}
		hdr.DescrListOffs = (uint32)rF.Tell();
		THROW(pData->DescrPosList.Write(rF, 0));
	}
	THROW(rF.CalcCRC(sizeof(hdr), &hdr.Crc32));
	rF.Seek(0);
	THROW(rF.Write(&hdr, sizeof(hdr)));
	CATCHZOK
	return ok;
}

int StringStore2::Read(SFile & rF)
{
	int    ok = 1;
	uint32 crc = 0;
	StrgHeader hdr;
	MEMSZERO(hdr);
	THROW(rF.IsValid());
	rF.Seek(0);
	THROW(rF.Read(&hdr, sizeof(hdr)));
	THROW(rF.CalcCRC(sizeof(hdr), &crc));
	THROW(crc == hdr.Crc32);
	THROW(hdr.Signature[0] == 'S' && hdr.Signature[1] == 'C' && hdr.Signature[2] == '2' && hdr.Signature[3] == 'B');
	THROW(hdr.Ver >= 2);
	{
		LangStrCollItem * p_lsc_item = GetOrConstructList(hdr.SLang);
		THROW(p_lsc_item);
		{
			THROW(rF.Seek(hdr.ListOffs));
			THROW(p_lsc_item->List.Read(rF, 0));
			THROW(rF.Seek(hdr.SListOffs));
			THROW(SignatureList.Read(rF, 0));
			if(hdr.Ver >= 3) {
				THROW(GrpSymbList.Read(rF, 0));
				THROW(HashGroupList.Read(rF, 0));
				THROW(p_lsc_item->HashAssocList.Read(rF, 0));
			}
		}
		{
			THROW(rF.Seek(hdr.DescrListOffs));
			THROW(p_lsc_item->DescrPosList.Read(rF, 0));
		}
		p_lsc_item->FileName = rF.GetName();
	}
	CATCHZOK
	return ok;
}

int StringStore2::InitParsing()
{
	ZDELETE(P_Pb);
	P_Pb = new ParseBlock;
	// @v12.3.4 @ctr P_Pb->LastAutoGrpCode = 1000;
	// @v12.3.4 @ctr P_Pb->LastSignatureCode = 0x80000000;
	return 1;
}

int StringStore2::AddGroup(StringStore2::ParseItem & rItem)
{
	int    ok = -1;
	int    empty_symb = 0;
	uint   pos;
	if(rItem.Group == 0) {
		if(rItem.GroupSymb.NotEmpty()) {
			if(GrpSymbList.SearchByText(rItem.GroupSymb, &(pos = 0))) {
				rItem.Group = GrpSymbList.Get(pos).Id;
				ok = 2;
			}
			else if(P_Pb->Saved_GrpSymbList.SearchByText(rItem.GroupSymb, &(pos = 0))) {
				rItem.Group = P_Pb->Saved_GrpSymbList.Get(pos).Id;
				THROW(GrpSymbList.Add(rItem.Group, rItem.GroupSymb));
				if(!P_Pb->LastAutoCodeList.Search(rItem.Group, 0)) {
					THROW(P_Pb->LastAutoCodeList.Add(rItem.Group, 10000, 0, 0));
				}
				ok = 1;
			}
		}
		if(ok < 0)
			rItem.Group = ++P_Pb->LastAutoGrpCode;
	}
	if(ok < 0) {
		if(rItem.GroupSymb.IsEmpty()) {
			empty_symb = 1;
			if(GrpSymbList.Search(rItem.Group, &(pos = 0))) {
				rItem.GroupSymb = GrpSymbList.Get(pos).Txt;
				ok = 2;
			}
			else
				rItem.GroupSymb = P_StrSt_EmptyCodeSymb;
		}
	}
	if(ok < 0) {
		if(GrpSymbList.Search(rItem.Group, &(pos = 0))) {
			THROW(rItem.GroupSymb.Cmp(GrpSymbList.Get(pos).Txt, 0) == 0);
			ok = 2;
		}
		else if(!empty_symb && GrpSymbList.SearchByText(rItem.GroupSymb, &(pos = 0))) {
			THROW(rItem.Group == GrpSymbList.Get(pos).Id);
			ok = 2;
		}
		else {
			THROW(GrpSymbList.Add(rItem.Group, rItem.GroupSymb));
			if(!P_Pb->LastAutoCodeList.Search(rItem.Group, 0)) {
				THROW(P_Pb->LastAutoCodeList.Add(rItem.Group, 10000, 0, 0));
			}
			ok = 1;
		}
	}
	if(ok > 0 && rItem.Group && rItem.Flags & rItem.fHashGroup) {
		HashGroupList.addUnique(rItem.Group);
	}
	CATCHZOK
	return ok;
}

int StringStore2::AddString(long group, StringStore2::ParseItem & rItem, long * pCodeStr)
{
	EXCEPTVAR(ErrCode);
	int    ok = 1;
	uint   pos = 0;
	long   code_str = 0;
	SString temp_buf;
	THROW_V(group < 0x7fff, sc2eGrpCodeLimit); // @str Превышение максимального значения кода группы
	if(rItem.CodeSymb.NotEmpty()) {
		if(P_Pb->StrSymbList.SearchByText(rItem.CodeSymb, &(pos = 0))) {
			temp_buf = P_Pb->StrSymbList.Get(pos).Txt;
			THROW_V(temp_buf == rItem.CodeSymb, sc2eCodeSymbConflict);
			code_str = P_Pb->StrSymbList.Get(pos).Id;
			ErrAddedMsg = rItem.CodeSymb;
			THROW_V(HiWord(code_str) == group, sc2eSavedSymbNEqGrp);
		}
		else {
			if(rItem.Code == 0) {
                if(P_Pb->Saved_StrSymbList.SearchByText(rItem.CodeSymb, &(pos = 0))) {
					long temp_code_str = P_Pb->Saved_StrSymbList.Get(pos).Id;
					rItem.Code = LoWord(temp_code_str);
					ErrAddedMsg = rItem.CodeSymb;
                    THROW_V(HiWord(temp_code_str) == group, sc2eSavedSymbNEqGrp);
                }
                else {
					long   c = 0;
					uint   pos = 0;
					THROW_V(P_Pb->LastAutoCodeList.Search(group, &c, &pos), sc2eGrpSuccNotFound);
					rItem.Code = ++c;
					P_Pb->LastAutoCodeList.at(pos).Val = c;
                }
			}
			THROW_V(rItem.Code < 0x7ffff, sc2eStrCodeLimit); // @str Превышение максимального значения кода строки
			code_str = MakeLong(rItem.Code, group);
			THROW(P_Pb->StrSymbList.Add(code_str, rItem.CodeSymb));
		}
	}
	else {
		rItem.CodeSymb = P_StrSt_EmptyCodeSymb;
		if(rItem.Signature.NotEmpty()) {
			if(SignatureList.SearchByText(rItem.Signature, &(pos = 0))) {
				code_str = SignatureList.Get(pos).Id;
			}
			else if(P_Pb->Saved_SignatureList.SearchByText(rItem.Signature, &(pos = 0))) {
				code_str = P_Pb->Saved_SignatureList.Get(pos).Id;
				THROW(SignatureList.Add(code_str, rItem.Signature));
			}
			else {
				code_str = (long)(++P_Pb->LastSignatureCode);
				THROW(SignatureList.Add(code_str, rItem.Signature));
			}
		}
		else {
			THROW_V(rItem.Code != 0, sc2eStrNotIdentified);
			THROW_V(rItem.Code < 0x7ffff, sc2eStrCodeLimit); // @str Превышение максимального значения кода строки
			code_str = MakeLong(rItem.Code, group);
		}
	}
	{
		uint   dup_pos = 0;
		LangStrCollItem * p_coll_item = GetOrConstructList(rItem.SLang);
		THROW(p_coll_item);
		THROW_V(p_coll_item->List.Search(code_str, &dup_pos) == 0, sc2eDupStrById);
		THROW(p_coll_item->List.Add(code_str, rItem.Str));
		ASSIGN_PTR(pCodeStr, code_str);
		if(HashGroupList.lsearch(group)) {
            THROW(p_coll_item->HashAssocList.Add(group, code_str, 0, 0));
		}
	}
	CATCHZOK
	return ok;
}

int StringStore2::ParseString(const SString & rStr, SStrScan & rScan, StringStore2::ParseItem & rItem)
{
	/*
		[/lang/]
		/!/ - no translate line
		#!{ - start no translate scope
		#!} - end no translate scope

		[hgroup:hash] // Суффикс :hash определяет хэширующую группу с символом "hgroup".

		[PPSTR_SOMEGROUP]
		1 "Some String 01"
		PPSTG_SOMESTRING02 "Some String 02"
		PPSTG_SOMESTRING02 /nt/ "Some String 02"
		PPSTG_SOMESTRING03=3 "Some String 03"
		@symb01 "Some String By Signature 'symb01'"
		@symb01<ru> "Некоторая строка по сигнатуре 'symb01'"
		@symb02=1002 "Some String By Signature 'symb02'"
		#{
			Описание для последнего определения строки.
		#}
	*/
	EXCEPTVAR(ErrCode);
	int    ok = 1;
	size_t pos;
	SString temp_buf;
	SString left;
	SString right;
	SString src_buf;
	(src_buf = rStr).Chomp().Strip();
	if(src_buf.Search("//", 0, 0, &pos))
		src_buf.Trim(pos).Strip();
	if(src_buf.NotEmpty()) {
		if(rItem.Flags & StringStore2::ParseItem::fNextLine) {
			rItem.Flags &= ~StringStore2::ParseItem::fNextLine;
			while(oneof2(src_buf.C(0), ' ', '\t'))
				src_buf.ShiftLeft();
			left = src_buf;
			if(left.Last() == '\\') {
				rItem.Flags |= StringStore2::ParseItem::fNextLine;
			}
			else {
				THROW_V(left.Last() == '\"', sc2eUnexpEndOfStr);
					// @str Строка должна оканчиваться либо символом '\' либо '"'
			}
			left.TrimRight();
			rItem.Str.Cat(left);
		}
		else if(src_buf.HasPrefix("#}")) {
			rItem.Kind = rItem.kScopeEnd;
			rItem.Scope = rItem.scopeDescr;
		}
		else if(Flags & fDescrScope) {
			assert(rItem.Scope == rItem.scopeDescr);
			rItem.Kind = rItem.kDescription;
			(temp_buf = rStr).Chomp();
			int   is_empty_str = 1;
            for(size_t dp = 0; is_empty_str && dp < temp_buf.Len(); dp++) {
				const char dc = temp_buf.C(dp);
				if(!oneof2(dp, ' ', '\t'))
					is_empty_str = 0;
            }
            if(is_empty_str)
				rItem.Str.CatChar('\n');
			else {
				if(rItem.Str.NotEmpty())
					rItem.Str.Space();
				rItem.Str.Cat(temp_buf.Strip());
			}
		}
		else {
			StringStore2::ParseItem preserve_item = rItem;
			rItem.Z();
			if(src_buf.HasPrefix("#{")) {
				rItem.Kind = rItem.kScopeStart;
				rItem.Scope = rItem.scopeDescr;
			}
			else if(src_buf.HasPrefix("#!{")) {
				rItem.Kind = rItem.kScopeStart;
				rItem.Scope = rItem.scopeNoTranslate;
			}
			else if(src_buf.HasPrefix("#!}")) {
				rItem.Kind = rItem.kScopeEnd;
				rItem.Scope = rItem.scopeNoTranslate;
			}
			else if(src_buf.C(0) == '[') {
				THROW_V(src_buf.Last() == ']', sc2eStxGrp01); // @str Ошибка в определении группы строк (ожидается ']')
				src_buf.TrimRight().ShiftLeft(1).Strip();
				if(src_buf.C(0) == '/') {
					int    slang = 0;
					THROW_V(src_buf.Last() == '/', sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
					src_buf.TrimRight().ShiftLeft(1).Strip();
					if(src_buf == "0" || src_buf.IsEqiAscii("default"))
						slang = 0;
					else {
						slang = RecognizeLinguaSymb(src_buf, 0);
						THROW_V(slang != 0, sc2eInvLangSymb);
					}
					rItem.Kind = StringStore2::ParseItem::kGroupLang;
					rItem.SLang = slang;
				}
				else {
					rItem.Kind = StringStore2::ParseItem::kGroup;
					if(src_buf.Divide('=', left, right) > 0) {
						rItem.GroupSymb = left.Strip();
						rItem.Group = right.Strip().ToLong();
						THROW_V(rItem.Group > 0, sc2eStxGrp02); // @str Ошибка в определении группы строк [symb=number]
					}
					else if(src_buf.ToLong() != 0) {
						rItem.Group = src_buf.ToLong();
                        if(src_buf.Divide(':', left, right) > 0) {
							if(right == "hash")
                                rItem.Flags |= rItem.fHashGroup;
                        }
					}
					else {
						rItem.Group = 0;
						rItem.GroupSymb = src_buf;
					}
					if(rItem.GroupSymb.NotEmpty()) {
                        if(rItem.GroupSymb.Divide(':', left, right) > 0) {
							rItem.GroupSymb = left;
							if(right == "hash")
                                rItem.Flags |= rItem.fHashGroup;
                        }
					}
				}
			}
			else if(src_buf.SearchChar('\"', &pos)) {
				rItem.Kind = StringStore2::ParseItem::kString;
				src_buf.Sub(pos+1 /* кавычку пропускаем */, src_buf.Len(), rItem.Str);
				//
				// Разбор идентификации строки {
				//
				src_buf.Trim(pos).Strip();
				rScan.Set(src_buf, 0);
				int    slang = -1;
				int    is_ident = 0; // Признак того, что строка идентифицирована
				double symb_val = 0.0;
				if(rScan[0] == '/') {
					rScan.Incr();
					temp_buf.Z();
					while(!rScan.Is('/')) {
						temp_buf.CatChar(rScan[0]);
						rScan.Incr();
					}
					THROW_V(rScan.Is('/'), sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
					if(temp_buf == "!")
						rItem.Flags |= rItem.fNoTranslate;
					else if(temp_buf == "0" || temp_buf.IsEqiAscii("default"))
						slang = 0;
					else {
						slang = RecognizeLinguaSymb(temp_buf, 0);
						THROW_V(slang != 0, sc2eInvLangSymb);
					}
				}
				rScan.Skip();
				if(rScan[0] == '@') {
					rScan.Incr();
					rScan.GetWord(0, temp_buf);
					rItem.Signature = temp_buf;
					is_ident = 1;
				}
				else if(rScan.GetDigits(temp_buf)) {
					rItem.Code = temp_buf.ToLong();
					is_ident = 1;
				}
				else {
					temp_buf.Z();
					uint n = 0;
					int  c = rScan[0];
					if(isasciiupr(c) || c == '_') {
						// THROW_V((c >= 'A' && c <= 'Z') || c == '_', sc2eStxStrSymbStart);
							// @str Символ строки должен начинаться с прописной латинской буквы или '_'
						temp_buf.CatChar(c);
						rScan.Incr();
						while((c = rScan[0]) != 0) {
							if(!oneof5(c, ' ', '\t', '/', '=', '\"')) {
								THROW_V(isasciiupr(c) || isdec(c) || c == '_', sc2eStxStrSymbChr);
									// @str Символ строки должен содержать либо прописные латинские буквы либо цифры либо '_'
								temp_buf.CatChar(c);
								rScan.Incr();
							}
							else
								break;
						}
						rItem.CodeSymb = temp_buf;
						rScan.Skip();
						if(rScan[0] == '=') {
							rScan.Incr();
							rScan.Skip();
							THROW_V(rScan.GetDigits(temp_buf), sc2eStxStr01); // @str Ошибка в определении строки (symb=number)
							rItem.Code = temp_buf.Strip().ToLong();
						}
						is_ident = 1;
					}
				}
				if(rScan.Skip()[0] == '/') {
					rScan.Incr();
					temp_buf.Z();
					while(!rScan.Is('/')) {
						temp_buf.CatChar(rScan[0]);
						rScan.Incr();
					}
					THROW_V(rScan.Is('/'), sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
					THROW_V(slang == -1, sc2eLangDescrDup);
					if(temp_buf == "!")
						rItem.Flags |= rItem.fNoTranslate;
					else if(temp_buf == "0" || temp_buf.IsEqiAscii("default"))
						slang = 0;
					else {
						slang = RecognizeLinguaSymb(temp_buf, 0);
						THROW_V(slang != 0, sc2eInvLangSymb);
					}
				}
				THROW_V(is_ident || slang >= 0, sc2eStrNotIdent);
				rItem.SLang = (slang >= 0) ? slang : ActiveLanguage;
				//
				// }
				//
				if(rItem.Str.Last() == '\\') {
					rItem.Flags |= StringStore2::ParseItem::fNextLine;
				}
				else {
					THROW_V(rItem.Str.Last() == '\"', sc2eUnexpEndOfStr);
						// @str Строка должна оканчиваться либо символом '\' либо '"'
				}
				rItem.Str.TrimRight();
				{
					src_buf = rItem.Str;
					rItem.Str = 0;
					size_t len = src_buf.Len();
					for(uint i = 0; i < len; i++) {
						int    c = src_buf.C(i);
						if(c == '\\') {
							c = src_buf.C(++i);
							if(c == 'n')
								c = '\n';
							else if(c == 't')
								c = '\t';
							else if(oneof2(c, 'x', 'X')) {
								uchar hex[4];
								THROW_V(len > i+2, sc2eInvSlashX);
								hex[0] = src_buf.C(++i);
								hex[1] = src_buf.C(++i);
								hex[2] = 0;
								sscanf(_strupr(reinterpret_cast<char *>(hex)), "%X", &c);
							}
							else if(isdec(c)) {
								uchar oct[4];
								oct[0] = c;
								THROW_V(len > i+2, sc2eInvSlashDigit);
								oct[1] = src_buf.C(++i);
								oct[2] = src_buf.C(++i);
								oct[3] = 0;
								sscanf(reinterpret_cast<const char *>(oct), "%o", &c);
							}
						}
						rItem.Str.CatChar(c);
					}
				}
			}
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int StringStore2::GenerateJavaFile(const char * pFileName, const char * pSrcFileName) // @v11.1.5
{
	int    ok = 1;
	uint   i;
	long   last_grp = 0;
	SString temp_buf;
	SFsPath ps;
	Generator_CPP gen(pFileName);
	THROW(gen.IsValid());
	ps.Split(pFileName);
	ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, temp_buf);
	gen.Wr_Comment(temp_buf.ToUpper());
	temp_buf.Printf("This file was generated by SC2C.EXE from '%s'", pSrcFileName);
	gen.Wr_Comment(temp_buf);
	gen.Wr_Comment(0);
	GrpSymbList.SortByID();
	P_Pb->StrSymbList.SortByID();
	//
	// Следующий блок очень плохой - мне надо было быстро и потому я заложился на то, что пакет, в котором это используется называется ru.petroglif.styloq
	//
	temp_buf.Z().Cat("package").Space().Cat("ru.petroglif.styloq").Semicol();
	gen.Wr_Indent();
	gen.WriteLine(temp_buf.CR());
	gen.WriteBlancLine();
	//
	gen.Wr_Indent();
	gen.Wr_StartClassDecl(Generator_CPP::clsClass, ps.Nam, 0, Generator_CPP::acsPublic, 0);
	gen.IndentInc();
	{
		static const SIntToSymbTabEntry StrGroupList[] = {
			{ 1,   "PPSTR_ERROR" },
			{ 4,   "PPSTR_DBLIB" },
			{ 6,   "PPSTR_DBENGINE" },
			{ 7,   "PPSTR_SLIBERR" },
			{ 102, "PPSTR_CONFIRMATION" },
			{ 103, "PPSTR_INFORMATION" },
			{ 110, "PPSTR_OBJNAMES" },
			{ 111, "PPSTR_TEXT" },
			{ 114, "PPSTR_SYMB" },
			{ 122, "PPSTR_CURLERR" },
			{ 123, "PPSTR_HASHTOKEN" },
			{ 124, "PPSTR_HASHTOKEN_C" },
			{ 342, "PPSTR_CRYSTAL_REPORT" },
		};
		for(i = 0; i < SIZEOFARRAY(StrGroupList); i++) {
			const SIntToSymbTabEntry & r_item = StrGroupList[i];
			temp_buf.Z().Cat("public").Space().Cat("static").Space().Cat("final").Space().Cat("int").Space();
			temp_buf.Cat(r_item.P_Symb).Space().Eq().Space().Cat(r_item.Id).Semicol();
			gen.Wr_Indent();
			gen.WriteLine(temp_buf.CR());
		}
		gen.WriteBlancLine();
	}
	for(i = 0; i < GrpSymbList.getCount(); i++) {
		StrAssocArray::Item item = GrpSymbList.Get(i);
		if(!sstreq(item.Txt, P_StrSt_EmptyCodeSymb)) {
			//gen.Wr_Define(item.Txt, temp_buf.Z().Cat(item.Id));
			// public static final int DDD = val;
			temp_buf.Z().Cat("public").Space().Cat("static").Space().Cat("final").Space().Cat("int").Space();
			temp_buf.Cat(item.Txt).Space().Eq().Space().Cat(item.Id).Semicol();
			gen.Wr_Indent();
			gen.WriteLine(temp_buf.CR());
		}
	}
	gen.WriteBlancLine();
	for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->StrSymbList.Get(i);
		const long grp = static_cast<long>(HiWord(item.Id));
		if(!sstreq(item.Txt, P_StrSt_EmptyCodeSymb)) {
			long code = 0;
			if(grp != last_grp) {
				gen.Wr_Comment(0);
				if(GrpSymbList.GetText(grp, temp_buf) <= 0)
					temp_buf.Z().Cat(grp);
				gen.Wr_Comment(temp_buf);
				gen.Wr_Comment(0);
			}
			temp_buf.Z().Cat("public").Space().Cat("static").Space().Cat("final").Space().Cat("int").Space();
			temp_buf.Cat(item.Txt).Space().Eq().Space();
			if(item.Id & 0x80000000)
				temp_buf.CatChar('0').CatChar('x').CatHex(item.Id);
			else
				temp_buf.Cat((long)LoWord(item.Id));
			temp_buf.Semicol();
			//gen.Wr_Define(item.Txt, temp_buf);
			gen.Wr_Indent();
			gen.WriteLine(temp_buf.CR());
		}
		last_grp = grp;
	}
	gen.IndentDec();
	gen.Wr_Indent();
	gen.Wr_CloseBrace(0);
	CATCHZOK
	return ok;
}

int StringStore2::GenerateHeaderFile(const char * pFileName, const char * pSrcFileName)
{
	int    ok = 1;
	uint   i;
	long   last_grp = 0;
	SString temp_buf;
	SFsPath ps;
	Generator_CPP gen(pFileName);
	THROW(gen.IsValid());
	ps.Split(pFileName);
	ps.Merge(0, SFsPath::fDrv|SFsPath::fDir, temp_buf);
	gen.Wr_Comment(temp_buf.ToUpper());
	temp_buf.Printf("This file was generated by SC2C.EXE from '%s'", pSrcFileName);
	gen.Wr_Comment(temp_buf);
	gen.Wr_Comment(0);
	GrpSymbList.SortByID();
	P_Pb->StrSymbList.SortByID();
	for(i = 0; i < GrpSymbList.getCount(); i++) {
		StrAssocArray::Item item = GrpSymbList.Get(i);
		if(!sstreq(item.Txt, P_StrSt_EmptyCodeSymb))
			gen.Wr_Define(item.Txt, temp_buf.Z().Cat(item.Id));
	}
	for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->StrSymbList.Get(i);
		long grp = (long)HiWord(item.Id);
		if(!sstreq(item.Txt, P_StrSt_EmptyCodeSymb)) {
			long code = 0;
			if(grp != last_grp) {
				gen.Wr_Comment(0);
				if(GrpSymbList.GetText(grp, temp_buf) <= 0)
					temp_buf.Z().Cat(grp);
				gen.Wr_Comment(temp_buf);
				gen.Wr_Comment(0);
			}
			temp_buf.Z();
			if(item.Id & 0x80000000)
				temp_buf.CatChar('0').CatChar('x').CatHex(item.Id);
			else
				temp_buf.Cat((long)LoWord(item.Id));
			gen.Wr_Define(item.Txt, temp_buf);
		}
		last_grp = grp;
	}
	CATCHZOK
	return ok;
}

#ifdef SC2C // {
	int main(int argc, char * argv[])
	{
		int    r = 0;
		if(argc < 2) {
			printf("Usage: s2c filename\n");
			r = -1;
		}
		else {
			StringStore2 s;
			SString auto_transl_lang;
			long   flags = 0;
			if(argc > 2) {
				auto_transl_lang = argv[2];
				flags |= s.cfTranslateOnly;
			}
			r = s.Compile(argv[1], flags, auto_transl_lang) ? 0 : 1;
		}
		return r;
	}
#endif // } SC2C
