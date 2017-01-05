// STRSTORE.CPP
// Copyright (c) A.Sobolev 1996-1999, 2000-2003, 2004, 2008, 2010, 2011, 2015, 2016
// @codepage windows-1251
//
#include <slib.h>
#pragma hdrstop
#include <strstore.h>

// #define TEST

SLAPI StringStore::StringStore(const char * pFileName, int for_writing) : SArray(sizeof(IdxItem))
{
	file = 0;
	open(pFileName, for_writing);
}

SLAPI StringStore::~StringStore()
{
	close();
}

int SLAPI StringStore::open(const char * pFileName, int for_writing)
{
	EXCEPTVAR(SLibError);
	last = 0;
	error = 0;
	writing = for_writing;
	char   om[3];
	om[0] = for_writing ? 'w' : 'r';
	om[1] = 'b';
	om[2] = 0;
	THROW_V(file = fopen(pFileName, om), SLERR_FILENOTFOUND);
	if(for_writing) {
		int zero = 0;
		fwrite(&zero, 1, 16, file);
		last = 1;
		flags |= aryUnique;
	}
	else {
		last  = 0;
		delta = 0;
		count = 0;
		Header h;
		fread(&h, sizeof(h), 1, file);
		THROW_V(h.offs, SLERR_INVFORMAT);
		fseek(file, ((long) h.offs) << 4, SEEK_SET);
		THROW_V(data = realloc(data, isize * h.count), SLERR_NOMEM);
		THROW_V(fread(data, isize * h.count, 1, file) == 1, SLERR_READFAULT);
		count = h.count;
		limit = h.count;
		flags |= arySorted;
	}
	CATCH
		freeAll();
		error = 1;
	ENDCATCH
	return !error;
}

int SLAPI StringStore::close()
{
	if(!error) {
		if(writing) {
			Header h;
			MEMSZERO(h);
			h.sign = 0;
			h.offs = last;
			h.count = count;
			if(fwrite(data, count * isize, 1, file) != 1) {
				SLibError = SLERR_WRITEFAULT;
				error = 1;
			}
			else {
				fseek(file, 0L, SEEK_SET);
				if(fwrite(&h, sizeof(h), 1, file) != 1) {
					SLibError = SLERR_WRITEFAULT;
					error = 1;
				}
			}
		}
	}
	if(file) {
		fclose(file);
		file = 0;
	}
	return !error;
}

int SLAPI StringStore::getString(int group, int code, SString & s)
{
	int    ok = 0;
	if(writing) {
		SLibError = SLERR_READFAULT;
		error = 1;
	}
	if(!error) {
		uint    pos;
		IdxItem item;
		item.group = group;
		item.code  = code;
		if(bsearch(&item, &pos, CMPF_LONG)) {
			uint16 tmp;
			fseek(file, ((IdxItem *)at(pos))->offs << 4, SEEK_SET);
			if(fread(&tmp, sizeof(tmp), 1, file) != 1) {
				SLibError = SLERR_READFAULT;
				error = 1;
			}
			else if(s.Read(file, tmp) > 0) {
				s.Utf8ToOem(); // @v8.9.10 ToOem-->Utf8ToOem
				ok = 1;
			}
			else
				error = 1;
		}
		else
			// Если требуемый элемент не найден, то флаг ошибки не устанавливаем.
			// Это не заблокирует дальнейшие обращения к функции.
			if(SLibError != SLERR_NOFOUND)
				error = 1;
	}
	if(!ok)
		s = 0;
	return ok;
}

int SLAPI StringStore::getString(int group, int code, char * pBuf, size_t bufLen)
{
	SString s;
	int    ok = getString(group, code, s);
	s.CopyTo(pBuf, bufLen);
	return ok;
}
//
//
//
SLAPI StringStore2::StringStore2()
{
	P_Pb = 0;
	Flags = 0;
	ErrCode = 0;
	ActiveLanguage = 0;
}

SLAPI StringStore2::~StringStore2()
{
	delete P_Pb;
}

int SLAPI StringStore2::SetActiveLanguage(int slang)
{
	ActiveLanguage = slang;
	return 1;
}

int SLAPI StringStore2::Load(const char * pFileName)
{
	int    ok = 1;
#ifndef _WIN32_WCE // {
	SFile bin_file(pFileName, SFile::mRead|SFile::mBinary);
	THROW(bin_file.IsValid());
	THROW(Read(bin_file));
#endif // } _WIN32_WCE
	CATCHZOK
	return ok;
}

#ifndef _WIN32_WCE // {

int SLAPI StringStore2::Read(SFile & rF)
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
	{
		LangStrCollItem * p_lsc_item = GetOrConstructList(hdr.SLang);
		THROW(p_lsc_item);
		THROW(p_lsc_item->List.Read(rF, 0));
	}
	THROW(SignatureList.Read(rF, 0));
	CATCHZOK
	return ok;
}

#endif // } _WIN32_WCE

int SLAPI StringStore2::Helper_GetString(long ident, SString & rBuf) const
{
	rBuf = 0;
	int    ok = 0;
	const  int lang = SLS.GetUiLanguageId();
	const  LangStrCollItem * p_item = GetList(lang);
	if(p_item && p_item->List.Get(ident, rBuf))
		ok = 1;
	else if(lang != 0) {
		p_item = GetList(0);
		if(p_item && p_item->List.Get(ident, rBuf))
			ok = 2;
	}
	return ok;
}

int SLAPI StringStore2::GetString(int group, int code, SString & rBuf) const
{
	return Helper_GetString(MakeLong(code, group), rBuf);
}

int SLAPI StringStore2::GetString(const char * pSignature, SString & rBuf) const
{
	rBuf = 0;
	uint   pos = 0;
	if(SignatureList.SearchByText(pSignature, 0, &pos) > 0) {
		long   code_str = SignatureList.at(pos).Id;
		return Helper_GetString(code_str, rBuf);
	}
	else
		return 0;
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
		p_result = new LangStrCollItem;
		p_result->SLang = slang;
		StrList.insert(p_result);
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

#ifdef SC2C // {

struct StringStoreItem {
	SLAPI  StringStoreItem();
	void   SLAPI Clear();
	enum {
		kGroup = 1, // Определитель группы строк [999]
		kString,    // Строка
		kGroupLang, // Определитель языка для всего последующего текста [/ge/] (до конца файла или до следующего
			// определителя языка.
		kLang // Определитель языка для следующей строки /en/
	};
	enum {
		fNextLine = 0x0001 // Признак того, что для предыдущей строки был задан перевод на новую строку (\)
	};
	long   Kind;
	long   Flags;
	long   Group;
	long   Code;
	int    SLang;  // @v8.9.10 Идентификатор языка slangXXX
	SString GroupSymb;
	SString CodeSymb;
	SString Signature;
	SString Str;
};

SLAPI StringStoreItem::StringStoreItem()
{
	Clear();
}

void SLAPI StringStoreItem::Clear()
{
	Kind = 0;
	Flags = 0;
	Group = 0;
	Code = 0;
	SLang = 0;
	GroupSymb = 0;
	CodeSymb = 0;
	Signature = 0;
	Str = 0;
}

enum {
	sc2eUsage = 1,         // Usage: SC2C inputFileName
	sc2eGrpCodeLimit,      // Превышение максимального значения кода группы
	sc2eStrCodeLimit,      // Превышение максимального значения кода строки
	sc2eDupSignature,      // Дублирование сигнатуры строки
	sc2eDupStrCode,        // Дублирование кода строки
	sc2eDupStrSymb,        // Дублирование символа строки
	sc2eGrpSuccNotFound,   // Не найдена групповая последовательность
	sc2eSrcFileOpenFault,  // Ошибка открытия исходного файла
	sc2eUnfinStrWrap,      // Перенос строки не завершен
	sc2eUnexpEndOfStr,     // Строка должна оканчиваться либо символом '\' либо '"'
	sc2eStxGrp01,          // Ошибка в определении группы строк (ожидается ']')
	sc2eStxGrp02,          // Ошибка в определении группы строк [symb=number]
	sc2eStxEqAfterCode,    // Числовое значение не должно предшествовать символу '='
	sc2eStxStrSymbStart,   // Символ строки должен начинаться с прописной латинской буквы или '_'
	sc2eStxStrSymbChr,     // Символ строки должен содержать либо прописные латинские буквы либо цифры либо '_'
	sc2eStxStr01,          // Ошибка в определении строки (symb=number)
	sc2eInvSlashX,         // Не оконченная конструкция '\x00'
	sc2eInvSlashDigit,     // Не оконченная конструкция '\000'
	sc2eStxLangGrp01,      // Ошибка в определении группы языка (ожидается '/')
	sc2eInvLangSymb,       // Не допустимый символ языка
	sc2eLangDescrDup,      // Дублирование определения языка для одной строки
	sc2eStrNotIdent,       // Строка не идентифицирована (нет определителя и не задан язык)
	sc2eSrcFileNotFound,   // Исходный файл не найден
	sc2eSrcFileHasHyphen,  // Имя исходного файла не должно содержать дефисов
	sc2eCodeSymbConflict,  // Конфликт между символьными идентификаторами строк
	sc2eDupStrById,        // Дублирование строки по значению идентификатора
	sc2eStrNotIdentified   // Строка не имеет идентификации
};

const char * Sc2Err[] = {
	"Success",
	"Usage: SC2C inputFileName",
	"Превышение максимального значения кода группы",
	"Превышение максимального значения кода строки",
	"Дублирование сигнатуры строки",
	"Дублирование кода строки",
	"Дублирование символа строки",
	"Не найдена групповая последовательность",
	"Ошибка открытия исходного файла",
	"Перенос строки не завершен",
	"Строка должна оканчиваться либо символом '\\' либо '\"'",
	"Ошибка в определении группы строк (ожидается ']')",
	"Ошибка в определении группы строк [symb=number]",
	"Числовое значение не должно предшествовать символу '='",
	"Символ строки должен начинаться с прописной латинской буквы или '_'",
	"Символ строки должен содержать либо прописные латинские буквы либо цифры либо '_'",
	"Ошибка в определении строки (symb=number)",
	"Не законченная конструкция '\x00'",
	"Не законченная конструкция '\000'",
	"Ошибка в определении группы языка (ожидается '/')",
	"Не допустимый символ языка",
	"Дублирование определения языка для одной строки",
	"Строка не идентифицирована (нет определителя и не задан язык)",
	"Исходный файл не найден",
	"Имя исходного файла не должно содержать дефисов",
	"Конфликт между символьными идентификаторами строк",
	"Дублирование строки по значению идентификатора",
	"Строка не имеет идентификации"
};

int SLAPI StringStore2::Error(int errCode, long flags)
{
	SString msg_buf, temp_buf(4);
	char   stub[32];
	stub[0] = 0;
	const char * p_file_name = stub;
	int    line = 0;
	if(P_Pb) {
		p_file_name = P_Pb->InFileName;
		line = P_Pb->Line;
	}
	if(errCode < sizeof(Sc2Err)/sizeof(Sc2Err[0]))
		temp_buf = Sc2Err[errCode];
	else
		temp_buf = "Unknown error";
	msg_buf.Printf("%s(%d): error sc2c: %s", p_file_name, line, (const char *)temp_buf);
	printf((const char *)msg_buf.CR().ToOem());
	return 0;
}

int SLAPI StringStore2::StoreState(const char * pFileName)
{
	int    ok = 1;
	uint   i;
	SString temp_buf;
    SIniFile ini_file(pFileName, 1, 1, 1);
    THROW(ini_file.Valid());
    ini_file.AppendParam("common", "LastAutoGrpCode",   (temp_buf = 0).Cat(P_Pb->LastAutoGrpCode), 1);
    ini_file.AppendParam("common", "LastSignatureCode", (temp_buf = 0).Cat(P_Pb->LastSignatureCode), 1);
    //
    temp_buf = 0;
    for(i = 0; i < P_Pb->LastAutoCodeList.getCount(); i++) {
		const LAssoc & r_item = P_Pb->LastAutoCodeList.at(i);
		if(i)
			temp_buf.CatChar(';');
		temp_buf.Cat(r_item.Key).CatChar(',').Cat(r_item.Val);
    }
    ini_file.AppendParam("common", "LastAutoCodeList", temp_buf, 1);
	//
	for(i = 0; i < SignatureList.getCount(); i++) {
		StrAssocArray::Item item = SignatureList.at(i);
		ini_file.AppendParam("SignatureList", item.Txt, (temp_buf = 0).Cat(item.Id), 1);
	}
	//
	for(i = 0; i < P_Pb->GrpSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->GrpSymbList.at(i);
		ini_file.AppendParam("GrpSymbList", item.Txt, (temp_buf = 0).Cat(item.Id), 1);
	}
	//
	for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->StrSymbList.at(i);
		ini_file.AppendParam("StrSymbList", item.Txt, (temp_buf = 0).Cat(item.Id), 1);
	}
	//
	THROW(ini_file.FlashIniBuf());
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI StringStore2::RestoreState(const char * pFileName)
{
    int    ok = 1;
    if(fileExists(pFileName)) {
		SString temp_buf, item_buf, key_buf, val_buf;
        SIniFile ini_file(pFileName, 0, 1, 1);
		THROW(ini_file.Valid());
		ini_file.GetParam("common", "LastAutoGrpCode", temp_buf = 0);
		P_Pb->LastAutoGrpCode = temp_buf.ToLong();
		ini_file.GetParam("common", "LastSignatureCode", temp_buf = 0);
		P_Pb->LastSignatureCode = temp_buf.ToULong();
		//
		{
			P_Pb->LastAutoCodeList.clear();
			ini_file.GetParam("common", "LastAutoCodeList", temp_buf = 0);
			StringSet ss(';', temp_buf);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
				if(item_buf.Divide(',', key_buf, val_buf) > 0) {
					P_Pb->LastAutoCodeList.Add(key_buf.ToLong(), val_buf.ToLong(), 0, 0);
				}
			}
		}
		//
		{
			SignatureList.Clear();
			StringSet ss;
			ini_file.GetEntries("SignatureList", &ss, 1);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					SignatureList.Add(val_buf.Strip().ToLong(), key_buf.Strip(), 1);
                }
			}
		}
		//
		{
			P_Pb->GrpSymbList.Clear();
			StringSet ss;
			ini_file.GetEntries("GrpSymbList", &ss, 1);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					P_Pb->GrpSymbList.Add(val_buf.Strip().ToLong(), key_buf.Strip(), 1);
                }
			}
		}
		//
		{
			P_Pb->StrSymbList.Clear();
			StringSet ss;
			ini_file.GetEntries("StrSymbList", &ss, 1);
			for(uint ssp = 0; ss.get(&ssp, item_buf);) {
                if(item_buf.Divide('=', key_buf, val_buf) > 0) {
					P_Pb->StrSymbList.Add(val_buf.Strip().ToLong(), key_buf.Strip(), 1);
                }
			}
		}
    }
    else
		ok = -1;
    CATCH
		ok = 0;
    ENDCATCH
    return ok;
}

int SLAPI StringStore2::Compile(const char * pFileName, long flags)
{
	EXCEPTVAR(ErrCode);
	int    ok = 1;
	StrAssocArray file_lang_list;
	SString ident_file_name; // Имя файла, хранящего соответствия символов и идентификаторов
	SString temp_buf;

	SPathStruc ps;
	SString org_nam, org_ext;

	THROW_V(fileExists(pFileName), sc2eSrcFileNotFound);
	THROW(InitParsing());
	//THROW(s.ParseFile(pFileName));
	{
        ps.Split(pFileName);
        THROW_V(!ps.Nam.HasChr('-'), sc2eSrcFileHasHyphen);
        file_lang_list.Add(0, pFileName);

		org_nam = ps.Nam;
		org_ext = ps.Ext;
		//
		ps.Ext = "symb";
		ps.Merge(ident_file_name);
		//
        ps.Ext = org_ext;
        (ps.Nam = org_nam).CatChar('-').CatChar('*');
        ps.Merge(temp_buf);
        {
        	SString lang_symb;
			SDirEntry de;
			SPathStruc ps_lang;
			for(SDirec direc(temp_buf, 0); direc.Next(&de) > 0;) {
				if(!(de.Attr & 0x10)) {
					temp_buf = de.FileName;
					uint   hyphen_pos = 0;
                    if(temp_buf.StrChr('-', &hyphen_pos)) {
                        temp_buf.Sub(hyphen_pos+1, temp_buf.Len(), lang_symb = 0);
						int slang = RecognizeLinguaSymb(lang_symb, 0);
						if(slang) {
							ps_lang.Split(de.FileName);
							ps.Nam = ps_lang.Nam;
							ps.Ext = ps_lang.Ext;
							ps.Merge(temp_buf);
                            file_lang_list.Add(slang, temp_buf);
						}
                    }
				}
			}
        }
	}
	{
		SStrScan scan;
		SString line_buf;
		THROW(RestoreState(ident_file_name));
		for(uint fli = 0; fli < file_lang_list.getCount(); fli++) {
			StrAssocArray::Item fl_item = file_lang_list.at(fli);
			StringStoreItem item;
			StringStoreItem last_group;
			StringStoreItem last_lang;

			SetActiveLanguage(fl_item.Id);
			P_Pb->Line = 0;
			P_Pb->InFileName = fl_item.Txt;
			SFile f_in(P_Pb->InFileName, SFile::mRead);
			THROW_V(f_in.IsValid(), sc2eSrcFileOpenFault); // @str Ошибка открытия исходного файла
			while(f_in.ReadLine(line_buf)) {
				int r;
				++P_Pb->Line;
				THROW(r = ParseString(line_buf, scan, item));
				if(r > 0) {
					if(item.Kind == StringStoreItem::kGroup) {
						THROW(AddGroup(item));
						last_group = item;
					}
					else if(item.Kind == StringStoreItem::kGroupLang) {
						SetActiveLanguage(item.SLang);
					}
					else if(item.Kind == StringStoreItem::kString && !(item.Flags & StringStoreItem::fNextLine)) {
						THROW(AddString(last_group.Group, item));
					}
				}
			}
			THROW_V(!(item.Flags & StringStoreItem::fNextLine), sc2eUnfinStrWrap); // @str Перенос строки не завершен
		}
		THROW(StoreState(ident_file_name));
	}
	{
		SString file_name = pFileName;
		SString test_file_name;
		for(uint i = 0; i < StrList.getCount(); i++) {
			const LangStrCollItem * p_item = StrList.at(i);
			if(p_item) {
				if(p_item->SLang == 0) {
					ps.Nam = org_nam;
				}
				else {
                    GetLinguaCode(p_item->SLang, temp_buf);
					(ps.Nam = org_nam).CatChar('-').Cat(temp_buf);
				}
				ps.Ext = "bin";
				ps.Merge(file_name);
				SFile bin_file(file_name.ToLower(), SFile::mReadWriteTrunc|SFile::mBinary);
				THROW(bin_file.IsValid());
				THROW(Write(bin_file, p_item));
			}
		}
		{
			ps.Nam = org_nam;
			ps.Ext = "h";
			ps.Merge(file_name);
			THROW(GenerateHeaderFile(file_name.ToLower(), pFileName));
		}
		/* @v8.9.10
		{
			SPathStruc::ReplaceExt(file_name, "rc", 1);
			THROW(s.GenerateRcFile(file_name.ToLower(), pFileName));
		}
		*/
#ifdef _DEBUG // {
		//
		// TEST
		// Считываем созданный бинарный файл, затем записываем его в файл с тем же именем и
		// расширением .TEST и сравниваем оригинальный файл с новым: они должны совпасть байт-в-байт.
		//
		{
			StringStore2 test_s;
			ps.Nam = org_nam;
			ps.Ext = "bin";
			ps.Merge(file_name);
			SFile bin_file(file_name.ToLower(), SFile::mRead|SFile::mBinary);
			THROW(test_s.Read(bin_file));
			for(uint i = 0; i < StrList.getCount(); i++) {
				const LangStrCollItem * p_item = StrList.at(i);
				if(p_item) {
					test_file_name = file_name;
					SPathStruc::ReplaceExt(test_file_name, "test", 1);
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
	CATCH
		ok = Error(ErrCode, 0);
	ENDCATCH
	return ok;
}

int SLAPI StringStore2::Write(SFile & rF, const LangStrCollItem * pData)
{
	int    ok = 1;
	StrgHeader hdr;
	MEMSZERO(hdr);
	hdr.Signature[0] = 'S';
	hdr.Signature[1] = 'C';
	hdr.Signature[2] = '2';
	hdr.Signature[3] = 'B';
	hdr.SLang = pData->SLang;
	THROW(rF.IsValid());
	rF.Seek(0);
	THROW(rF.Write(&hdr, sizeof(hdr)));
	hdr.ListOffs = (uint32)rF.Tell();
	THROW(pData->List.Write(rF, 0));
	hdr.SListOffs = (uint32)rF.Tell();
	THROW(SignatureList.Write(rF, 0));
	THROW(rF.CalcCRC(sizeof(hdr), &hdr.Crc32));
	rF.Seek(0);
	THROW(rF.Write(&hdr, sizeof(hdr)));
	CATCHZOK
	return ok;
}

int SLAPI StringStore2::InitParsing()
{
	ZDELETE(P_Pb);
	P_Pb = new ParseBlock;
	P_Pb->LastAutoGrpCode = 1000;
	P_Pb->LastSignatureCode = 0x80000000;
	return 1;
}

int SLAPI StringStore2::AddGroup(StringStoreItem & rItem)
{
	int    ok = -1;
	int    empty_symb = 0;
	uint   pos;
	if(rItem.Group == 0) {
		if(rItem.GroupSymb.NotEmpty() && P_Pb->GrpSymbList.SearchByText(rItem.GroupSymb, 0, &(pos = 0))) {
			rItem.Group = P_Pb->GrpSymbList.at(pos).Id;
			ok = 2;
		}
		else
			rItem.Group = ++P_Pb->LastAutoGrpCode;
	}
	if(ok < 0) {
		if(rItem.GroupSymb.Empty()) {
			empty_symb = 1;
			if(P_Pb->GrpSymbList.Search(rItem.Group, &(pos = 0))) {
				rItem.GroupSymb = P_Pb->GrpSymbList.at(pos).Txt;
				ok = 2;
			}
			else
				rItem.GroupSymb = ".N.";
		}
	}
	if(ok < 0) {
		if(P_Pb->GrpSymbList.Search(rItem.Group, &(pos = 0))) {
			THROW(rItem.GroupSymb.Cmp(P_Pb->GrpSymbList.at(pos).Txt, 0) == 0);
			ok = 2;
		}
		else if(!empty_symb && P_Pb->GrpSymbList.SearchByText(rItem.GroupSymb, 0, &(pos = 0))) {
			THROW(rItem.Group == P_Pb->GrpSymbList.at(pos).Id);
			ok = 2;
		}
		else {
			THROW(P_Pb->GrpSymbList.Add(rItem.Group, rItem.GroupSymb));
			if(!P_Pb->LastAutoCodeList.Search(rItem.Group, 0, 0)) {
				THROW(P_Pb->LastAutoCodeList.Add(rItem.Group, 10000, 0, 0));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI StringStore2::AddString(long group, StringStoreItem & rItem)
{
	EXCEPTVAR(ErrCode);
	int    ok = 1;
	uint   pos = 0;
	long   code_str = 0;
	SString temp_buf;
	THROW_V(group < 0x7fff, sc2eGrpCodeLimit); // @str Превышение максимального значения кода группы
	if(rItem.CodeSymb.NotEmpty()) {
		if(P_Pb->StrSymbList.SearchByText(rItem.CodeSymb, 0, &(pos = 0))) {
			temp_buf = P_Pb->StrSymbList.at(pos).Txt;
			THROW_V(temp_buf == rItem.CodeSymb, sc2eCodeSymbConflict);
			code_str = P_Pb->StrSymbList.at(pos).Id;
		}
		else {
			//
			if(rItem.Code == 0) {
				long   c = 0;
				uint   pos = 0;
				THROW_V(P_Pb->LastAutoCodeList.Search(group, &c, &pos), sc2eGrpSuccNotFound);
				rItem.Code = ++c;
				P_Pb->LastAutoCodeList.at(pos).Val = c;
			}
			THROW_V(rItem.Code < 0x7ffff, sc2eStrCodeLimit); // @str Превышение максимального значения кода строки
			code_str = MakeLong(rItem.Code, group);
			//
			THROW(P_Pb->StrSymbList.Add(code_str, rItem.CodeSymb));
		}
	}
	else {
		rItem.CodeSymb = ".N.";
		if(rItem.Signature.NotEmpty()) {
			if(SignatureList.SearchByText(rItem.Signature, 0, &(pos = 0))) {
				code_str = SignatureList.at(pos).Id;
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
	}
	CATCHZOK
	return ok;
}

int SLAPI StringStore2::ParseString(const SString & rStr, SStrScan & rScan, StringStoreItem & rItem)
{
	/*
		[/lang/]

		[PPSTR_SOMEGROUP]
		1 "Some String 01"
		PPSTG_SOMESTRING02 "Some String 02"
		PPSTG_SOMESTRING03=3 "Some String 03"
		@symb01 "Some String By Signature 'symb01'"
		@symb01<ru> "Некоторая строка по сигнатуре 'symb01'"
		@symb02=1002 "Some String By Signature 'symb02'"
	*/
	EXCEPTVAR(ErrCode);
	int    ok = 1;
	uint   pos;
	SString left, right;
	SString temp_buf;
	(TempBuf = rStr).Chomp().Strip();
	if(TempBuf.Search("//", 0, 0, &pos))
		TempBuf.Trim(pos).Strip();
	if(TempBuf.NotEmpty()) {
		if(rItem.Flags & StringStoreItem::fNextLine) {
			rItem.Flags &= ~StringStoreItem::fNextLine;
			left = TempBuf;
			if(left.Last() == '\\') {
				rItem.Flags |= StringStoreItem::fNextLine;
			}
			else {
				THROW_V(left.Last() == '\"', sc2eUnexpEndOfStr);
					// @str Строка должна оканчиваться либо символом '\' либо '"'
			}
			left.TrimRight();
			rItem.Str.Cat(left);
		}
		else {
			StringStoreItem preserve_item = rItem;
			rItem.Clear();
			if(TempBuf.C(0) == '[') {
				THROW_V(TempBuf.Last() == ']', sc2eStxGrp01); // @str Ошибка в определении группы строк (ожидается ']')
				TempBuf.TrimRight().ShiftLeft(1).Strip();
				if(TempBuf.C(0) == '/') {
					int    slang = 0;
					THROW_V(TempBuf.Last() == '/', sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
					TempBuf.TrimRight().ShiftLeft(1).Strip();
					if(TempBuf == "0" || TempBuf.CmpNC("default") == 0)
						slang = 0;
					else {
						slang = RecognizeLinguaSymb(TempBuf, 0);
						THROW_V(slang != 0, sc2eInvLangSymb);
					}
					rItem.Kind = StringStoreItem::kGroupLang;
					rItem.SLang = slang;
				}
				else {
					rItem.Kind = StringStoreItem::kGroup;
					if(TempBuf.Divide('=', left, right) > 0) {
						rItem.GroupSymb = left.Strip();
						rItem.Group = right.Strip().ToLong();
						THROW_V(rItem.Group > 0, sc2eStxGrp02); // @str Ошибка в определении группы строк [symb=number]
					}
					else if(TempBuf.ToLong() != 0) {
						rItem.Group = TempBuf.ToLong();
					}
					else {
						rItem.Group = 0;
						rItem.GroupSymb = TempBuf;
					}
				}
			}
			else if(TempBuf.StrChr('\"', &pos)) {
				rItem.Kind = StringStoreItem::kString;
				TempBuf.Sub(pos+1 /* кавычку пропускаем */, TempBuf.Len(), rItem.Str);
				//
				// Разбор идентификации строки {
				//
				TempBuf.Strip(pos).Strip();
				rScan.Set(TempBuf, 0);
				int    slang = -1;
				int    is_ident = 0; // Признак того, что строка идентифицирована
				double symb_val = 0.0;
				if(rScan[0] == '/') {
					rScan.Incr();
                    temp_buf = 0;
                    while(!rScan.Is('/')) {
						temp_buf.CatChar(rScan[0]);
						rScan.Incr();
                    }
                    THROW_V(rScan.Is('/'), sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
					if(temp_buf == "0" || temp_buf.CmpNC("default") == 0)
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
					temp_buf = 0;
					uint n = 0;
					int  c = rScan[0];
					if((c >= 'A' && c <= 'Z') || c == '_') {
						// THROW_V((c >= 'A' && c <= 'Z') || c == '_', sc2eStxStrSymbStart);
							// @str Символ строки должен начинаться с прописной латинской буквы или '_'
						temp_buf.CatChar(c);
						rScan.Incr();
						while((c = rScan[0]) != 0) {
							if(!oneof5(c, ' ', '\t', '/', '=', '\"')) {
								THROW_V((c >= 'A' && c <= 'Z') || isdigit(c) || c == '_', sc2eStxStrSymbChr);
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
				if(rScan[0] == '/') {
					rScan.Incr();
                    temp_buf = 0;
                    while(!rScan.Is('/')) {
						temp_buf.CatChar(rScan[0]);
						rScan.Incr();
                    }
                    THROW_V(rScan.Is('/'), sc2eStxLangGrp01); // @str Ошибка в определении группы языка (ожидается '/')
                    THROW_V(slang == -1, sc2eLangDescrDup);
					if(temp_buf == "0" || temp_buf.CmpNC("default") == 0)
						slang = 0;
					else {
						slang = RecognizeLinguaSymb(temp_buf, 0);
						THROW_V(slang != 0, sc2eInvLangSymb);
					}
				}
				THROW_V(is_ident || slang >= 0, sc2eStrNotIdent);
				if(slang >= 0) {
					rItem.SLang = slang;
				}
				else {
					rItem.SLang = ActiveLanguage;
				}
				//
				// }
				//
				if(rItem.Str.Last() == '\\') {
					rItem.Flags |= StringStoreItem::fNextLine;
				}
				else {
					THROW_V(rItem.Str.Last() == '\"', sc2eUnexpEndOfStr);
						// @str Строка должна оканчиваться либо символом '\' либо '"'
				}
				rItem.Str.TrimRight();
				{
					TempBuf = rItem.Str;
					rItem.Str = 0;
					size_t len = TempBuf.Len();
					for(uint i = 0; i < len; i++) {
						int    c = TempBuf.C(i);
						if(c == '\\') {
							c = TempBuf.C(++i);
							if(c == 'n')
								c = '\n';
							else if(c == 't')
								c = '\t';
							else if(c == 'x' || c == 'X') {
								uchar hex[4];
								THROW_V(len > i+2, sc2eInvSlashX);
								hex[0] = TempBuf.C(++i);
								hex[1] = TempBuf.C(++i);
								hex[2] = 0;
								sscanf(strupr((char *)hex), "%X", &c);
							}
							else if(isdigit((uchar)c)) {
								uchar oct[4];
								oct[0] = c;
								THROW_V(len > i+2, sc2eInvSlashDigit);
								oct[1] = TempBuf.C(++i);
								oct[2] = TempBuf.C(++i);
								oct[3] = 0;
								sscanf((char *)oct, "%o", &c);
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

#if 0 // @v8.9.10 {

int SLAPI StringStore2::GenerateRcFile(const char * pFileName, const char * pSrcFileName)
{
	int    ok = 1;
	uint   i;
	long   last_grp = 0;
	SString temp_buf, str_buf, symb_buf;
	P_Pb->GrpSymbList.SortByID();
	P_Pb->StrSymbList.SortByID();
	if(P_Pb->StrSymbList.getCount()) {
		/*
		STRINGTABLE
		BEGIN
    		IDS_STRING371           "Тестовая строка в ресурсе"
    		IDS_STRING372           "Yet another test string"
		END
		*/
		uint   max_len = 0;
		SPathStruc ps;
		Generator_CPP gen(pFileName);
		THROW(gen.IsValid());
		//
		//
		//
		ps.Split(pFileName);
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
		gen.Wr_Comment(temp_buf.ToUpper());
		temp_buf.Printf("This file was generated by SC2C.EXE from '%s'", (const char *)pSrcFileName);
		gen.Wr_Comment(temp_buf);
		gen.Wr_Comment(0);
		//
		//
		//
		ps.Split(pSrcFileName);
		ps.Ext = "h";
		ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
		gen.Wr_Include(temp_buf.ToLower(), 1);
		gen.WriteLine(0);
		//
		//
		//
		gen.WriteLine((temp_buf = "STRINGTABLE").CR());
		gen.WriteLine((temp_buf = "BEGIN").CR());
		gen.IndentInc();
		P_Pb->StrSymbList.GetMaxLength(&max_len);
		max_len = ALIGNSIZE(max_len+1, 2);
		for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
			StrAssocArray::Item item = P_Pb->StrSymbList.at(i);
			long grp = (long)HiWord(item.Id);
			if(strcmp(item.Txt, ".N.") != 0 && List.Get(item.Id, str_buf) > 0) {
				long code = 0;
				if(grp != last_grp) {
					gen.Wr_Comment(0);
					if(P_Pb->GrpSymbList.Get(grp, temp_buf) <= 0)
						(temp_buf = 0).Cat(grp);
					gen.Wr_Comment(temp_buf);
					gen.Wr_Comment(0);
				}
				temp_buf = 0;
				gen.CatIndent(temp_buf).Cat((symb_buf = item.Txt).Align(max_len, ADJ_LEFT)).Cat(str_buf.Quot('\"', '\"')).CR();
				gen.WriteLine(temp_buf);
			}
			last_grp = grp;
		}
		gen.IndentDec();
		gen.WriteLine((temp_buf = "END").CR());
	}
	CATCHZOK
	return ok;
}

#endif // } 0 @v8.9.10

int SLAPI StringStore2::GenerateHeaderFile(const char * pFileName, const char * pSrcFileName)
{
	int    ok = 1;
	uint   i;
	long   last_grp = 0;
	SString temp_buf;
	SPathStruc ps;
	Generator_CPP gen(pFileName);
	THROW(gen.IsValid());
	//
	//
	//
	ps.Split(pFileName);
	ps.Merge(0, SPathStruc::fDrv|SPathStruc::fDir, temp_buf);
	gen.Wr_Comment(temp_buf.ToUpper());
	temp_buf.Printf("This file was generated by SC2C.EXE from '%s'", (const char *)pSrcFileName);
	gen.Wr_Comment(temp_buf);
	gen.Wr_Comment(0);
	//
	//
	//
	P_Pb->GrpSymbList.SortByID();
	P_Pb->StrSymbList.SortByID();
	for(i = 0; i < P_Pb->GrpSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->GrpSymbList.at(i);
		if(strcmp(item.Txt, ".N.") != 0)
			gen.Wr_Define(item.Txt, (temp_buf = 0).Cat(item.Id));
	}
	for(i = 0; i < P_Pb->StrSymbList.getCount(); i++) {
		StrAssocArray::Item item = P_Pb->StrSymbList.at(i);
		long grp = (long)HiWord(item.Id);
		if(strcmp(item.Txt, ".N.") != 0) {
			long code = 0;
			if(grp != last_grp) {
				gen.Wr_Comment(0);
				if(P_Pb->GrpSymbList.Get(grp, temp_buf) <= 0)
					(temp_buf = 0).Cat(grp);
				gen.Wr_Comment(temp_buf);
				gen.Wr_Comment(0);
			}
			temp_buf = 0;
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

int main(int argc, char * argv[])
{
	int    r = 0;
	if(argc < 2) {
		printf("Usage: s2c filename\n");
		r = -1;
	}
	else {
		StringStore2 s;
		r = s.Compile(argv[1], 0) ? 0 : 1;
	}
	return r;
}

#endif // } SC2C

#ifdef TEST

#include <ctype.h>

FILE *f = NULL;
char buf[512];
char strbuf[512];

int readString(char * str, int * id)
{
	if(fgets(buf, 512, f) == NULL)
		return 0;
	else {
		char * p = buf;
		while(*p && (isdigit(*p) || *p == '-'))
			p++;
		*p++ = '\0';
		*id = atoi(buf);
		strcpy(str, p);
		return 1;
	}
}

void makeStore()
{
	int id;
	f = fopen("strtest.txt", "rt");
	if(f == 0) {
		printf("Unable open input file\n");
		exit(-1);
	}
	StringStore store("store.msg", 1);
	if(store.error) {
		printf("Error creating StringStore object\n");
		exit(-1);
	}
	while(readString(strbuf, &id)) {
		if(!store.addString(1, id, strbuf)) {
			printf("Error adding string to StringStore object\n");
			exit(-1);
		}
	}
}

void readStore(StringStore * store, int id, Pchar strbuf)
{
	if(!store->getString(1, id, strbuf, 512)) {
		printf("Error getting string from StringStore object\n");
		exit(-1);
	}
}

void main()
{
	int id = 1;
	makeStore();

	StringStore store("store.msg");
	while(id != 0) {
		int id;
		printf("\nInput ID, please... ");
		scanf("%d", &id);
		if(id == 0)
			return;
		readStore(&store, id, strbuf);
		printf(strbuf);
	}
}

#endif /* TEST */
