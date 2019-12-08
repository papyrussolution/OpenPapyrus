// DB.H
// Copyright (C) Sobolev A. 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019
// @codepage UTF-8
//
#ifndef __DB_H
#define __DB_H

//#include <stddef.h>
#include <slib.h>
#include <sxml.h>
#include <dboci.h>
#include <comdisp.h>
//
// Forward declarations
//
struct DBFCreateFld;
class  DbfTable;
class  DBTable;
struct DBField;
class  BNFieldList;
class  DBFieldList;
class  BExtInsert;
class  SOraDbProvider;
class  Generator_SQL;
struct DBConst;
struct DBTree;
struct DBQ;
class  KR;
class  DBQuery;
class  DbProvider;
class  BDbEnv; // Окружение для BerkeleyDB
class  BDbDatabase;

#define THROW_D(expr,val)      {if(!(expr)){DBS.SetError(val);goto __scatch;}}
#define THROW_D_S(expr,val,s)  {if(!(expr)){DBS.SetError(val, s);goto __scatch;}}
#define THROW_DS(expr)         {if(!(expr)){DBS.SetError(BE_SLIB, 0);goto __scatch;}}

#ifndef _WIN32_WCE // {
//
// В версии 5.9.2 все зависимости от следующего определения из исходных кодов удалены.
//
#define UNI_DBTABLE
//
//
//
enum SqlServerType {
	sqlstNone    = 0, // Не определенное значение
	sqlstGeneric = 1, // Общий
	sqlstORA,         // Oracle
	sqlstMSS,         // Ms SQL Server
	sqlstFB,          // FireBird
	sqlstPg           // PostgreSQL
};
//
// Descr: класс, представляющий поля типа BLOB или CLOB в таблицах баз данных.
//   Главная проблема, которую призвана решить этот класс - совместимость между
//   используемыми (на начало 2009года) записями переменной длины в таблицах Btrieve
//   и LOB-полями в SQL-серверах.
//   Для этого первые 16 байт структуры содержат либо случайные данные, непосредственно
//   извлеченные из таблицы данных, либо зарезервированную сигнатуру CLob::Signature, которая означает,
//   что данные представлены манипулятором Hdr::H.
//
class SLob { // @persistent @noctr @novtbl size=32
public:
	void   FASTCALL Init(void * descriptor);
	int    FASTCALL InitPtr(uint32 sz);
	int    IsPtr() const;
	//
	// Descr: Устанавливает структурированный объект с нулевым манипулятором.
	//
	void   Empty();
	int    DestroyPtr();
	void * GetRawDataPtr();
	size_t GetPtrSize() const;
	//
	// Descr: Сериализация данных.
	// ARG(flatSize IN): размер, отведенный под буфер this.
	//
	int    Serialize(int dir, size_t flatSize, uint8 * pInd, SBuffer & rBuf);
	//
	// Descr: Специальная функция, применяемая для гарантии того, что
	//   буфер записи, извлеченный из базы данных не содержит сигнатуры
	//   структурированного буфера.
	// Returns:
	//   <0 - буфер не структурированный. Функция ничего не делала.
	//   >0 - буфер структурированный. Функция обнулила сигнатуру.
	//
	int    EnsureUnstructured();
private:
	int    SetStructured();
	int    IsStructured() const;

	enum {
		hfPtr = 0x0001 // Поле Hdr.H является указателем на динамически распределенную
			// область памяти размером Hdr::PtrSize
	};
	struct Hdr {
		uint32 Signature[4];
		void * H; // @v10.4.5 uint32-->void *
		uint32 RdPos;
		int32  Flags;     // SLob::hfXXX
		uint32 PtrSize;   // Если Flags & hfPtr, то PtrSize - размер области памяти, распределнной под указатель (void *)H
	};
	union {
		Hdr    H;
		uint8  B[sizeof(Hdr)];
	} Buf;
};

template <size_t S> class TSLob : public SLob {
	uint8  ExtBuf[S-sizeof(SLob)];
};
//
// @ModuleDecl(SdRecord)
//
class SdbField { // @transient
public:
	SdbField();
	void   Init();
	int    FASTCALL IsEqual(const SdbField & rPat) const;
	//
	// Descr: транслирует строку rScan.P_Buf+Offs в базовый тип и, возможно, формат вывода
	//   поля. Допускаются следующие варианты синтаксиса:
	//   style 1. field ::= type name '[' size ']'
	//   style 2. field ::= type name '(' size ')'
	//   style 3. field ::= name type '[' size ']'
	//   style 4. field ::= name type '(' size ')'
	//   style 5. field ::= name, type, size
	//
	//   style 1,2,3,4 size ::= len [. prc]
	//   style 5       size ::= len, prc
	//
	// Returns:
	//   !0 - строка успешно распознана и данные распознавания занесены в поля Typ и OuterFormat.
	//        Значение *pPos указывет на следующий символ за последним просканированным.
	//   0  - не удалось распознать строку. Значение *pPos не меняется. Поля структуры также не меняются.
	//
	int    TranslateString(SStrScan & rScan);
	int    PutToString(int style, SString & rBuf) const;
	int    ConvertToDbfField(DBFCreateFld *) const;
	int    GetFieldDataFromBuf(SString & rTextData, const void * pRecBuf, const SFormatParam &) const;
	void   PutFieldDataToBuf(const SString & rTextData, void * pRecBuf, const SFormatParam &) const;
	uint   ID;
	STypEx T;
	union {
		long   OuterFormat; // Формат представления поля во внешнем источнике данных
		uint   EnumVal;     // Значение элемента перечисления //
	};
	uint32 InnerOffs;   // Смещение данных от начала буфера // @v9.8.6 size_t-->InnerOffs
	SString Name;       // Наименование поля       //
	SString Descr;      // Текстовое описание поля //
	SString Formula;    // Если поле представлено не значением, а формулой, то последняя хранится здесь.
private:
	int    Helper_TranslateString(SStrScan & rScan, void * pData);
};

class SdRecord { // @persistent
public:
	//
	// Descr: Внутреннее представление полей записи.
	//
	struct F { // @persistent
		uint   ID;    // @anchor ИД поля. Внутренний код полагается на то, что это поле - первое в структуре.
		STypEx T;
		union {
			long   OuterFormat;
			uint   EnumVal;
		};
		uint32 InnerOffs; // @v9.8.6 size_t-->uint32 (чтобы не нарушать @persistent)
		uint   NamePos;
		uint   DescrPos;
		uint   FormulaPos;
	};
	enum {
		fAllowDupID   = 0x0001, // При добавлении полей класс не следит за тем, чтобы их
			// идентификаторы были уникальными внутри записи.
		fAllowDupName = 0x0002, // При добавлении полей класс не следит за тем, чтобы их
			// наименования были уникальными внутри записи.
		fNamesToUpper = 0x0004, // При добавлении полей их наименования переводятся в верхний регистр.
			// Это гарантирует уникальность наименований полей не зависимо от регистра.
		fEnum         = 0x0008, // Структура-перечисление. Общий размер составляет sizeof(uint32)
			// Значение каждого варианта хранится как EnumVal поля.
		fNoData       = 0x0010  // Запись не является представлением сплошного пространства полей.
			// То есть, с записью невозможно сопоставить пространство данных. Речь идет о
			// декларативных стурктурах вроде typedef. Функция SetupOffsets в этом случае не инициализирует
			// поле F::InnerOffs и устанавливает RecSize в ноль.
	};
	explicit SdRecord(long flags = 0);
	SdRecord(const SdRecord &);
	~SdRecord();
	SdRecord & FASTCALL operator = (const SdRecord &);
	int    FASTCALL Copy(const SdRecord & rSrc);
	void   Clear();
	int    FASTCALL IsEqual(const SdRecord & rPat) const;
	void   FASTCALL SetDescription(const char *);
	int    GetDescription(SString &) const;
	//
	// Descr: Сериализация объекта.
	// Note: формат хранения в буфере не совместим с функциями Write_/Read_.
	//
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL Write_(SBuffer & rBuf) const;
	int    FASTCALL Read_(SBuffer & rBuf);
	//
	// Descr: Добавляет в конец записи новое поле с типом typ, именем pName и описанием pDescr.
	// ARG(pID     OUT): @#{vptr0} Указатель, по которому будет присвоен идентификатор нового поля //
	//   Если *pID != 0, то функция пытается добавить поле с таким идентификатором. Если, при этом,
	//   (Flags & fAllowDupID) == 0, и такой идентификатор в записи присутствует, то функция возвращает
	//   ошибку и поле не добавляется.
	//   Если *pID == 0 && pFld->Flags & SdbField::fZeroID, то функция пытается добавить поле с
	//   нулевым идентификатором. При этом правила поддержки уникальности идентификаторов остаются //
	//   в силе.
	// ARG(pFld     IN): @#{!0}  Указатель на структуру поля, значения которой используются для //
	//   вновь создаваемого поля.
	//   Если this->Flags & fNamesToUpper, то наименование поля переводится в верхний регистр.
	//   Метод следит за тем, чтобы наименование поля отличалось от наименований других
	//   полей записи, если не установлен флаг fAllowDupName.
	//   Если pFld->OuterFmt == 0, то при выводе поля в текстовый формат клиенты класса должны
	//   сами определять формат вывода.
	// ARG(pFld     IN): @#{!0}    Тип нового поля. (см STYPE.H)
	// Returns:
	//   >0 - поле успешно добавлено в структуру
	//   0  - ошибка (SLibError)
	//
	int    AddField(uint * pID, const SdbField * pFld);
	//
	// Descr: Модифирует поле в позиции pos так, что атрибуты этого поля будут соответствовать *pFld.
	//   Уникальность pFld->ID и pFld->Name проверяется аналогично тому, как это делается в методе AddField
	// Returns:
	//   >0 - поле успешно модифицировано
	//   0  - ошибка (SLibError)
	//
	int    UpdateField(uint pos, const SdbField * pFld);
	//
	// Descr: Удаляет поле, находящееся в позиции pos.
	//
	int    RemoveField(uint pos);
	//
	// Descr: Перемещает поле в позиции pos на одну позицию вниз или вверх (в зависимости от параметра up).
	// ARG(pos IN): @#{(up && pos>0 && pos<GetCount()) || (!up && pos>=0 && pos<GetCount()-1)}
	//   позиция, в которой находится поле, которое необходимо переместить.
	// ARG(up  IN): если up == 0, то поле pos меняется местами с полем pos+1, в противном случае
	//   поле pos меняется местами с полем pos-1.
	// ARG(pNewPos OUT): @#{vptr0} Указатель, по которому в случае успеха присваивается новая //
	//   позиция перемещенного поля.
	// Returns:
	//   !0 - поле успешно перемещено
	//   0  - ошибка (поле не перемещено).
	//
	int    MoveField(uint pos, int up, uint * pNewPos); // @>>Items.moveItem
	//
	// Descr: Возвращает количество полей в записи.
	//
	uint   GetCount() const;
	int    SearchName(const char * pName, uint * pPos, uint excludePos = UINT_MAX) const;
	int    ScanName(SStrScan & rScan, uint * pPos, uint excludePos = UINT_MAX) const;
	//
	// Descr: Возвращает размер бинарного представления записи.
	//
	size_t GetRecSize() const;
	int    FASTCALL GetFieldByPos(uint pos /*0..*/, SdbField * pFld) const;
	//
	// Descr: То же самое, что и GetFieldByPos() но не заполняет текстовые значения pFld (Name, Descr, Formula)
	// Note: Пользоваться функцией следует с большой осторожностью и уверенностью, что (Name, Descr, Formula)
	//   вам для использования не важны!
	//   Функция реализована для ускорения в экстремальных ситуациях, где обрабатывается значительное
	//   число записей.
	//
	int    FASTCALL GetFieldByPos_Fast(uint pos /*0..*/, SdbField * pFld) const;
	//
	// Descr: функция перечисления полей записи. Возвращает поле по индексу *pPos
	//   и увеличивает значение *pPos на единицу.
	// ARG(pPos IN/OUT) @#{vptr0} указатель на индекс записи. Если pPos == 0, то возвращается 0.
	// ARG(pFld    OUT) @#{vptr0} указатель на структуру SdbField. Если pFld == 0, то функция //
	//   отрабатывает корректно, но в холостую (не пытается что-либо присвоить по нулевому адресу).
	// Returns:
	//   >0 - поле по индексу *pPos найдено и успешно занесено по адресу pFld
	//   0  - индекс *pPos превышает количество полей в записи (больше ничего нет), либо pPos == 0.
	// Example:
	//   SdRecord * p_rec = ...;
	//   SdbField fld;
	//   for(uint i = 0; p_rec->EnumFields(&i, &fld);) {
	//       ...
	//   }
	//
	int    EnumFields(uint * pPos, SdbField * pFld) const;
	int    GetFieldByID(uint id, uint * pPos, SdbField * pFld) const;
	int    GetFieldByName(const char * pName, SdbField * pFld) const;
	//
	// Descr: То же самое, что и GetFieldByName(), но для извлечения
	//   SdbField применяется GetFieldByPos_Fast() что ускоряет вызов.
	// Note: see comments to GetFieldByPos_Fast.
	//
	int    GetFieldByName_Fast(const char * pName, SdbField * pFld) const;
	TYPEID GetFieldType(uint pos /*0..*/) const;
	const  STypEx * GetFieldExType(uint pos /*0..*/) const;
	long   GetFieldOuterFormat(uint pos /*0..*/) const;
	//
	// Descr: Устанавливает внешний буфер данных pBuf размером bufLen.
	//   Клиент класса самостоятельно управляет временем жизни буфера pBuf.
	//
	void   SetDataBuf(void * pBuf, size_t bufLen);
	//
	// Descr: Распределяет память под собственный буфер данных размером GetRecSize().
	//   Если до этого был установлен внешний буфер данных, то он сбрасывается.
	// Returns:
	//   !0 - собственный буфер данных размером GetRecSize успешно распределен.
	//   0  - GetRecSize() != 0 и при этом не удалось распределить память под буфер данных.
	//
	int    AllocDataBuf();
	//
	// Descr: Обнуляет внутренний буфер данных записи
	//
	int    ClearDataBuf();
	//
	// Descr: Возвращает CONST указатель на буфер данных со смещением,
	//   соответствующим полю, в позиции fldPos [0..].
	//   Если внутренний буфер данных не определен или fldPos >= GetCount(),
	//   то возвращает 0.
	//
	const  void * FASTCALL GetDataC(uint fldPos = 0) const;
	//
	// Descr: Возвращает указатель на буфер данных со смешением,
	//   соответствующим полю, в позиции fldPos [0..].
	//   Если внутренний буфер данных не определен или fldPos >= GetCount(),
	//   то возвращает 0.
	// @attension
	//   Обратите внимание, что в случае использования этой функции, указатель
	//   на приватный буфер отдается на откуп клиенту. Будьте крайне внимательны
	//   чтобы не разрушить целостность буфера.
	//
	void * FASTCALL GetData(uint fldPos = 0) const;
	void   CreateRecFromDbfTable(const DbfTable * pTbl);
	const F * FASTCALL GetC(uint) const;
	/* @v9.2.7 Заменено на контстанты CTRANSF_XXX
	enum {
		cvtCharToOem = 1,
		cvtOemToChar,
		cvtUtf8ToChar
	}; */
	//
	// Descr: Преобразует поля записи в соответсвии с параметром cvt.
	// ARG(cvt IN): Вид преобразования, применяемого к полям записи CTRANSF_XXX
	// ARG(pBuf IN/OUT): @#{vptr} Буфер, содержащий данные записи.
	//   Буфер должен иметь длину, равную или больше сумме всех длин полей записи.
	// Returns:
	//   >0 - по крайней мере к одному полю записи была применеа процедура конвертации
	//   <0 - ни одно из полей буфера не было изменено
	//   0  - ошибка
	//
	int    ConvertDataFields(int cvt, void * pBuf) const;
	//
	// Descr: Заносит текст pText в пул строк и возвращает позицию
	//   добавленного текста. Если текст пустой или содержит только пробелы, то
	//   ничего не делает и возвращает нулевую позицию.
	//
	int    SetText(uint * pPos, const char * pText);
	//
	// Descr: Возвращает текст, хранящийся в пуле строк в позиции pos.
	//
	int    GetText(uint pos, SString & rText) const;
	//
	// Два поля, используемые для идентификации записи в коллекциях.
	// Изменение значений этих полей не влияют на поведение класса, поэтому
	// они являются доступными как public.
	//
	long   ID;    // @persistent Значение, используемое для идентификации записи в коллекциях.
	SString Name; // @persistent Наименование, используемое для идентификации записи в коллекциях.
private:
	void   Init();
	//
	// Descr: Разрушает внутренний буфер данных, если он является собственным для //
	//   этого экземпляра класса.
	// Returns:
	//   >0 - буфер успешно разрушен
	//   <0 - буфер не разрушен, поскольку не является собственным.
	//
	int    DestroyDataBuf();
	int    SetupOffsets();
	F *  FASTCALL Get(uint) const;

	int32  Ver;              // @persistent (Serialize())
	int32  Flags;            // @persistent
	uint32 DescrPos;         // @persistent
	SVector Items;           // @persistent // @v9.8.4 SArray-->SVector
	StringSet StringPool;    // @persistent
	size_t RecSize;          // @transient
	mutable SString TempBuf; // @transient @allocreuse
	void * P_DataBuf;        // @transient
	size_t OuterDataBufSize; // @transient
	int    IsDataBufOwner;   // @transient Если !0, то экземпляр является владельцем буфера данных
};
//
// Descr: Буфер для хранения множества записей разной длины.
//   Внутренний формат совместим с буфером класса BExtInsert.
//
class SdRecordBuffer : public SBaseBuffer {
public:
	explicit SLAPI SdRecordBuffer(size_t maxSize);
	SLAPI ~SdRecordBuffer();
	int    SLAPI Reset();
	//
	// Descr: Добавляет в буфер очередную запись.
	// Returns:
	//   >0 - запись добавлена успешно
	//   <0 - запись не добавлена поскольку буфер полностью заполнен.
	//    0 - ошибка
	//
	int    SLAPI Add(const void * pRecData, size_t recSize);
	uint   SLAPI GetCount() const;
	//
	// Descr: Возвращает !0 если все записи в буфере имеют одинаковый размер.
	//
	int    SLAPI IsEqRec() const;
	SBaseBuffer FASTCALL Get(uint recNo) const;
	//
	// Descr: Возвращает буфер в котором значение Size равно this->Pos.
	//
	SBaseBuffer SLAPI GetBuf() const;
private:
	enum {
		fEqRec = 0x0001
	};
	size_t MaxSize;
	size_t Pos;
	size_t MaxRecSize;
	long   Flags;
};
//
// @ModuleDecl(TextDbFile)
//
class TextDbFile {
public:
	enum {
		relAbs = 0,
		relFirst,
		relLast,
		relNext,
		relPrev
	};
	enum {
		fVerticalRec = 0x0001, // Вертикальная ориентация записей
		fFldEqVal    = 0x0002, // Для вертикальных записей. Поля имеют формат: FieldName=FieldValue
		fFixedFields = 0x0004, // Для горизонтальных записей. Поля имеют фиксированные длины (без разделителей)
			// Если этот флаг установлен, то ассоциированный экземпляр SdRecord должен содержать
			// длины каждого поля в SdbField::OuterLen //
		fFldNameRec  = 0x0008, // Первая запись содержит имена полей
			// Если этот флаг установлен, то при сканировании первой записи считается, что
			// она содержит вместо значений имена полей. При извлечении данных из записей
			// сопоставление элементов SdRecord с элементами записи осуществляется по этим именам
			// (без учета регистра символов).
			// В противном случае сопоставление идет по порядку следования записей и элементов SdRecord.
		fOemText     = 0x0010, // Текст имеет кодировку OEM
		fQuotText    = 0x0020  // Текстовые поля обрамлены двойными кавычками
	};
	struct Param { // @persistent
		Param(long flags = 0, const char * pFldDiv = 0, const char * pVertRecTerm = 0);
		Param(long flags, int fldDivChr = 0, const char * pVertRecTerm = 0);
		void   Init();
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

		int32  Ver;              // @v7.4.7 Serialize version
		int32  HdrLinesCount;    // Количество строк в начале файла, которые следует пропустить как заголовок
		int32  DateFormat;       // Формат даты (DATF_XXX)
		int32  TimeFormat;       // Формат времени (TIMF_XXX)
		int32  RealFormat;       // Формат чисел с плавающей точкой
		int32  Flags;            // Флаги (TextDbFile::fXXX)
		SString VertRecTerm;     // Строка-терминатор вертикальной записи
		SString FldDiv;          // Строка-разделитель полей в горизонтальной записи
		SString FooterLine;      // @v7.4.1 Завершающая строка файла.
		SString DefFileName;     // Имя файла (параметр конструктора TextDbFile [если !0] переопределяет это имя)
	};
	TextDbFile();
	~TextDbFile();
	int    Open(const char * pFileName, const Param * pParam, int readOnly);
	int    Close();
	ulong  GetNumRecords() const;
	int    GoToRecord(ulong recNo, int rel = relAbs);
	int    GetRecord(const SdRecord & rRec, void * pDataBuf);
	int    AppendHeader(const SdRecord & rRec, const void * pDataBuf);
	int    AppendRecord(const SdRecord & rRec, const void * pDataBuf);
	const  char * GetFileName() const;
private:
	int    Scan();
	int    CheckParam(const SdRecord & rRec);
	int    ParseFieldNameRec(const SString & rLine);
	void   PutFieldDataToBuf(const SdbField & rFld, const SString & rTextData, void * pRecBuf);
	int    GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf);
	int    IsTerminalLine(const SString & rLine, uint fldNo) const;

	Param  P;
	SFile  F;
	enum {
		stReadOnly    = 0x0001, // Файл только для чтения. В него не могут добавляться записи.
		stHeaderAdded = 0x0002, // Признак того, что была вызвана функция AppendHeader.
			// Эта функция заности кроме заголовочной записи специфицированное количество
			// пустых записей и список полей. Таким образом, первый вызов AppendRecord не
			// должен будет этого делать.
		stSignUtf8    = 0x0004  // В начале файла содержится префикс кодировки UTF-8 (EF BB BF).
			// Флаг может быть установлен функцией Scan()
	};
	long   State;
	//int    ReadOnly;
	LongArray RecPosList; // Список позиций прочитанных записей
	StringSet FldNames;    // Если (P.Flags & fFldNameRec), то в это поле в порядке следования //
		// записываются наименования полей файла данных
	long   EndPos;
	long   CurRec;
};
//
// XmlDbFile
//
#define PPYXML_ROOTTAG "PpyImpExp"
#define PPYXML_RECTAG  "Record"
#define PPYXML_TAGCONT "text"

class XmlDbFile {
public:
	struct Param { // @persistent
		enum {
			fUseDTD         = 0x00000001L,
			fUtf8Codepage   = 0x00000002L,
			fHaveSubRec     = 0x00000004L,
			fSkipEntityList = 0x00000008L  // @v8.6.8 Не выводить в выходном файле список сущностей
		};
		Param(const char * pRootTag, const char * pHdrTag, const char * pRecTag, long flags);
		void   Init(const char * pRootTag, const char * pHdrTag, const char * pRecTag, long flags);
		int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

		int32  Ver;      // @v7.4.7 Serialize version
		long   Flags;
		SString RootTag;
		SString RecTag;
		SString HdrTag;  // @v7.2.6
	};

	static int CheckParam(const Param & rParam);

	XmlDbFile();
	~XmlDbFile();
	void   SetEntitySpec(const char * pSpec);
	int    Open(const char * pPath, const Param * pParam, const SdRecord * pRec, int readOnly);
	int    Close();
	int    AppendRecord(const SdRecord & rRec, const void * pDataBuf);
	int    AppendRecord(const char * pRecTag, const SdRecord & rRec, const void * pDataBuf);
	ulong  GetNumRecords() const;
	// int  GoToRecord(ulong recNo, int rel = relAbs);
	int    GetRecord(const SdRecord & rRec, void * pDataBuf);
	//
	// Следующие 2 функции позволяют прочитать из файла элементы, которые подчинены текущему. Для этого старое состояние сохраняется, а когда все элементы считаны восстанавливается.
	//
	//     Сохраняет текущее состояние.
	int    Push(const Param * pParam);
	//
	// Восстанавливает текущее состояние
	//
	int    Pop();
	const xmlNode * FindFirstRec_(const xmlNode * pRoot) const;
	const xmlNode * FindNextRec_(const xmlNode * pCurrent) const;
	const char * GetFileName() const;
	int    CountRecords(const xmlNode * pRootNode, uint * pCount);
	int    GetExportBuffer(SBuffer & rBuf);
private:
	int    IsUtf8() const;
	int    WriteField(const char * pFieldName, const char * pFieldValue, int isDtd);
	int    WriteDTDS(const SdRecord & rRec);
	int    GetValueByName(const char * pName, SString & rValue);
	const  xmlNode * Helper_FindRec_(const xmlNode * pCurrent) const;
	int    Helper_CloseWriter();

	struct State {
		State();
		~State();
		State(const State & rS);
		State & operator = (const State & rS);
		void   Reset();
		int    Copy(const State & rS);
		int    SetParam(const Param * pParam);
		int    FASTCALL IsRecTag(const char * pTag) const;
		int    FASTCALL IsRecNode(const xmlNode * pNode) const;
		const  xmlNode * FASTCALL GetHeadRecNode(const xmlNode * pNode) const;
		const  Param & GetParam() const { return P; }
		uint32 Pos;
		uint32 NumRecs;
		const xmlNode * P_CurRec;
		const xmlNode * P_LastRec; // Запись, обработанная последним вызовом GetRecord(). Указатель необходим
			// для постобработки записи так как P_CurRec этим вызовом переходит к следующей записи (или 0,
			// если записей не осталось).
	private:
		Param  P;
		StringSet * P_SplittedRecTag;
	};

	SString FileName;
	xmlTextWriter * P_Writer;
	xmlBuffer * P_Buffer;
	xmlDoc * P_Doc;
	SString EntitySpec;
	State  St;
	State  PreserveSt;
	TSCollection <State> StateColl; // @vmiller
	TSStack <int> StateStack; // @vmiller
	//
	// Для чтения, например строк документа
	//
	int    ReadOnly;
	int    UseSubChild;
};

class SoapDbFile {
public:
	struct Param {
	    explicit Param(const char * pRootTag = 0, const char * pHeadTag = 0, const char * pRecTag = 0);
		void   Init(const char * pRootTag, const char * pHeadTag, const char * pRecTag);

		SString RootTag;
		SString HeadTag;
		SString RecTag;
	};
	SoapDbFile();
	~SoapDbFile();
	int    Open(const char * pPath, const Param * pParam, int readOnly);
	int    Close();
	int    AppendRecord(const SdRecord & rRec, const void * pDataBuf);
	ulong  GetNumRecords() const;
	// int  GoToRecord(ulong recNo, int rel = relAbs);
	int    GetRecord(const SdRecord & rRec, void * pDataBuf);
private:
	int    WriteSchema();

	ulong  Pos;
	ulong  NumRecs;
	Param  P;
};
//
// @v6.4.4 AHTOXA {
// @ModuleDecl(ExcelDbFile)
//
class ExcelDbFile {
public:
	enum {
		relAbs = 0,
		relFirst,
		relLast,
		relNext,
		relPrev
	};
	enum {
		fFldNameRec    = 0x0001, // Первая запись содержит имена полей
			// Если этот флаг установлен, то при сканировании первой записи считается, что
			// она содержит вместо значений имена полей. При извлечении данных из записей
			// сопоставление элементов SdRecord с элементами записи осуществляется по этим именам
			// (без учета регистра символов).
			// В противном случае сопоставление идет по порядку следования записей и элементов SdRecord.
		fOemText       = 0x0002, // Текст имеет кодировку OEM
		fQuotText      = 0x0004, // Текстовые поля обрамлены двойными кавычками
		fVerticalRec   = 0x0008, // Вертикальная ориентация записей
		fOneRecPerFile = 0x0010  // Одна запись в файле
	};
	struct Param {
		explicit Param(long flags = 0);
		void  Init();
		int   Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);

		int32  Ver;              // @v7.4.7 Serialize version
		int32  HdrLinesCount;    // Количество строк в начале страницы, которые следует пропустить как заголовок
		int32  ColumnsCount;     // Количество колонок в начале страницы, которые следует пропустить
		int32  SheetNum;         // Номер страницы Excel в которую следует осуществлять экспорт/импорт данных
		int32  DateFormat;       // Формат даты (DATF_XXX)
		int32  TimeFormat;       // Формат времени (TIMF_XXX)
		int32  RealFormat;       // Формат чисел с плавающей точкой
		int32  Flags;            // Флаги (ExcelDbFile::fXXX)
		SString SheetName_;      // Наименование страницы Excel в которую следует осуществлять экспорт/импорт данных
			// Если при импорте данных листа с таким именем не найдено, то данные импортируются с листа с номером SheetNum
		SString EndStr_;         // Признак окончания данных
	};
	ExcelDbFile();
	~ExcelDbFile();
	int    Open(const char * pFileName, const Param * pParam, int readOnly);
	int    Close();
	ulong  GetNumRecords() const;
	int    GoToRecord(ulong recNo, int rel = relAbs);
	int    GetRecord(const SdRecord & rRec, void * pDataBuf);
	int    AppendRecord(const SdRecord & rRec, const void * pDataBuf);
	const char * GetFileName() const { return FileName.cptr(); }
private:
	int    Scan();
	int    CheckParam(const SdRecord & rRec);
	void   PutFieldDataToBuf(const SdbField & rFld, const SString & rTextData, void * pRecBuf);
	int    GetFieldDataFromBuf(const SdbField & rFld, SString & rTextData, const void * pRecBuf);
	int    GetFldNames();

	Param  P;
	int    ReadOnly;
	StringSet FldNames;    // Если (P.Flags & fFldNameRec), то в это поле в порядке следования //
		// записываются наименования полей файла данных
	long   CurRec;
	long   RecCount;
	SString FileName;
	LongArray WidthList;
	ComExcelWorkbook  * P_WkBook;
	ComExcelWorksheet * P_Sheet;
	ComExcelApp  * P_App;
	ComExcelWorksheets * P_Sheets;
};
// } @v6.4.4 AHTOXA
//
// Definitions
//
int btrokornfound();
int btrnfound__();

#define BTRIEVE_VER    0x0500 // 0x0600 || 0x0610
#define NUMPGSIZES          8 // Number of Btrieve page sizes
#define MAXKEYLEN         255
// @v8.6.10 #define IDXSEG(_S,_M,_F,_T)   *new DBIdxSpec(offsetof(_S,_M),sizeof((((_S SFAR*)0)->_M)),_F,_T)
// @v8.6.10 #define FILEDEF(_S,_F,_PS,_A) *new DBFileSpec(sizeof(_S),_F,_PS,_A)
#define BTRNFOUND         btrnfound__() //(BtrError==BE_EOF || BtrError==BE_KEYNFOUND)
//#define BTROKORNFOUND     oneof3(BtrError, 0, BE_EOF, BE_KEYNFOUND)
#define BTROKORNFOUND     btrokornfound()
#if defined(_Windows) || defined(__WIN32__) || defined(_WIN32)
	#define WBTRVTAIL MAXKEYLEN,index
	#define WBTRVTAIL_Z       0,index
	#define WBTRVTAIL_ZZ          0,0
#else
	#define WBTRVTAIL           index
	#define WBTRVTAIL_Z         index
	#define WBTRVTAIL_ZZ            0
#endif // _Windows
//
// Коды операций Btrieve
//
#define B_OPEN                     0
#define B_CLOSE                    1
#define B_INSERT                   2
#define B_UPDATE                   3
#define B_DELETE                   4
#define B_GETEQ                    5
#define B_GETNEXT                  6
#define B_GETPREV                  7
#define B_GETGT                    8
#define B_GETGE                    9
#define B_GETLT                   10
#ifndef __BTRV_H // If used Mebius's XPRESS
	#define B_GETLE                   11
#endif
#define B_GETFIRST                12
#define B_GETLAST                 13
#define B_CREATE                  14
#define B_STAT                    15
#define B_EXTEND                  16
#define B_SETDIR                  17
#define B_GETDIR                  18
#define B_BEGTRANSACTION          19
#define B_ENDTRANSACTION          20
#define B_ABORTTRANSACTION        21
#define B_GETPOS                  22
#define B_GETDIRECT               23
#define B_STEPDIRECT              24
#define B_STOP                    25
#define B_VERSION                 26
#define B_UNLOCK                  27
#define B_RESET                   28
#define B_SETOWNER                29
#define B_CLEAROWNER              30
#define B_CREATSUPIDX             31
#define B_DROPSUPIDX              32
#define B_STEPFIRST               33
#define B_STEPLAST                34
#define B_STEPPREV                35
#define B_GETNEXTEXT              36
#define B_GETPREVEXT              37
#define B_STEPNEXTEXT             38
#define B_STEPPREVEXT             39
#define B_INSERTEXT               40
#define B_CONTINUOUSOPR           42
#define B_GETBYPERCENT            44
#define B_FINDPERCENT             45
#define B_UPDATECHUNK             53
//
// Lock biases
//
#define BL_SNGL_WT                100 // Single wait lock
#define BL_SNGL_NWT               200 // Single nowait lock
#define BL_MULT_WT                300 // Multiply wait lock
#define BL_MULT_NWT               400 // Multiply nowait lock
#define BL_NW                     500 // Nowait concurrent transaction
#define BL_CONCUR_TA             1000 // Use concurrent transaction
//
// Chunk subfunctions
//
#define CHUNK_DIR_RAND    0x80000000L // Retrieve chunks directly into data buf
#define CHUNK_INDIR_RAND  0x80000001L // Retrieve chunks into buffer pointed by RandChunkItem::ptr
#define CHUNK_DIR_RECT    0x80000002L
#define CHUNK_INDIR_RECT  0x80000003L
#define CHUNK_NEXTINREC   0x40000000L // (Bias)
#define CHUNK_TRUNCATE    0x80000004L // UpdateChunk
#define CHUNK_APPEND      0x20000000L // UpdateChunk (Bias)
//
// Btrieve error codes
//
#define BE_SUCCESS                            0
#define BE_INVOP                              1
#define BE_IOFAIL                             2
#define BE_FILNOPEN                           3
#define BE_KEYNFOUND                          4
#define BE_DUP                                5
#define BE_INVKEY                             6
#define BE_KEYVIOL                            7
#define BE_INVPOS                             8
#define BE_EOF                                9
#define BE_MOD                               10
#define BE_INVFNAME                          11
#define BE_FNFOUND                           12
#define BE_EXTFAILNF                         13
#define BE_PREVOPEN                          14
#define BE_IMAGEFAIL                         15
#define BE_EXTFAIL                           16
#define BE_DISKFULL                          18
#define BE_UNRECFAIL                         19
#define BE_BTRNINIT                          20
#define BE_UKEYLEN                           21
#define BE_UBUFLEN                           22
#define BE_UPOSLEN                           23
#define BE_INVPAGE                           24
#define BE_CRIOFAIL                          25
#define BE_INVKEYNUM                         26
#define BE_INVKEYPOS                         27
#define BE_INVRECLEN                         28
#define BE_INVKEYLEN                         29
#define BE_NBTRFILE                          30
#define BE_FEXTDUP                           31
#define BE_EXTIOFAIL                         32
#define BE_INVEXTNAM                         34
#define BE_DIRFAIL                           35
#define BE_SESFAIL                           36
#define BE_SESNEST                           37
#define BE_SESCTRL                           38
#define BE_SESPAR                            39
#define BE_SESMAXF                           40
#define BE_SESOP                             41
#define BE_USACSS                            42
#define BE_INVADDR                           43
#define BE_NULLKEY                           44
#define BE_INVFLAG                           45
#define BE_ACSFAIL                           46
#define BE_OFMAX                             47
#define BE_INVALT                            48
#define BE_BADKEY                            49
#define BE_PWINST                            50
#define BE_INVPW                             51
#define BE_CACHFAIL                          52
#define BE_INVXFACE                          53
#define BE_VLRPAGE                           54
#define BE_BADINDEX                          56
#define BE_EMSFAIL                           57
#define BE_FEXISTS                           59 // Файл '%s' уже существует
#define BE_REJECTLIMIT                       60
#define BE_WORKSPACETOOSMALL                 61
#define BE_INCORRECTDESCRIPTOR               62
#define BE_FILTERLIMIT                       64
#define BE_INCORRECTFLDOFS                   65
#define BE_CANTOPENDICT                      67
#define BE_WAITERROR                         77
#define BE_DEADLOCK                          78
#define BE_CONFLICT                          80
#define BE_LOCKFAIL                          81
#define BE_LOSTPOS                           82
#define BE_RDOUTSES                          83
#define BE_RECLOCK                           84
#define BE_FILELOCK                          85
#define BE_FILEFULL                          86
#define BE_DRTFULL                           87
#define BE_MODEFAIL                          88
#define BE_NAMEFAIL                          89
#define BE_DEVFULL                           90
#define BE_SERVFAIL                          91
#define BE_TOOMSES                           92
#define BE_BADLOCK                           93
#define BE_INOPACK                           97
#define BE_DEMO                              99
#define BE_NOCACHE_BUFFERS_AVAIL            100
#define BE_NO_OS_MEMORY_AVAIL               101
#define BE_NO_STACK_AVAIL                   102
#define BE_CHUNK_OFFSET_TOO_LONG            103
#define BE_LOCALE_ERROR                     104
#define BE_CANNOT_CREATE_WITH_BAT           105
#define BE_CHUNK_CANNOT_GET_NEXT            106
#define BE_CHUNK_INCOMPATIBLE_FILE          107
#define BE_FILEREACHEDSIZELIMIT             132 // The file has reached its size limit
#define BE_MAXUSERCOUNTREACHED              161 // The maximum number of user count licenses has been reached
#define BE_DBLOGINFAILED                    171 // Database login failed (workstation not on a domain)
//
// Scalable SQL engine codes (Used for my DB library)
//
#define BE_TABLEISNOTDEFINED                204
//
// Btrieve requestor status codes
//
#define BE_INSUFFICIENT_MEM_ALLOC          2001
#define BE_INVALID_OPTION                  2002
#define BE_NO_LOCAL_ACCESS_ALLOWED         2003
#define BE_SPX_NOT_INSTALLED               2004
#define BE_INCORRECT_SPX_VERSION           2005
#define BE_NO_AVAIL_SPX_CONNECTION         2006
#define BE_INVALID_PTR_PARM                2007
//
// Windows Client Return codes
//
#define BE_LOCK_PARM_OUTOFRANGE            1001
//
// Error code BE_NOMEM assigned to BtrError global var
// if my DB library can not allocate memory for operation
//
#define BE_NOMEM                           1002
#define BE_MEM_PARM_TOO_SMALL              1003
#define BE_PAGE_SIZE_PARM_OUTOFRANGE       1004
#define BE_INVALID_PREIMAGE_PARM           1005
#define BE_PREIMAGE_BUF_PARM_OUTOFRANGE    1006
#define BE_FILES_PARM_OUTOFRANGE           1007
#define BE_INVALID_INIT_PARM               1008
#define BE_INVALID_TRANS_PARM              1009
#define BE_ERROR_ACC_TRANS_CONTROL_FILE    1010
#define BE_COMPRESSION_BUF_PARM_OUTOFRANGE 1011
#define BE_TASK_LIST_FULL                  1013
#define BE_STOP_WARNING                    1014
#define BE_ALREADY_INITIALIZED             1016
#define BE_MKDE_FAILEDTOINIT               1021 // @v9.9.11 The MicroKernel failed to initialize
//
//
//
#define BE_SLIB                            9000 // Трансляция ошибок SLIB
#define BE_ORA_TEXT                        9001 // Ошибка ORACLE. Текст сообщения заносится в DBS.AddedMsgString
#define BE_BDB_INVALID_TABLE               9002 // Внутренняя ошибка: объект таблицы BDB разрушен
#define BE_BDB_INVALID_CURSOR              9003 // Внутренняя ошибка: объект курсора BDB разрушен
#define BE_BDB_NOMEM                       9004 // Ошибка BerkeleyDB: недостаточно памяти.
//
// Коды ошибок, формируемые модулем интерфейса с BerkeleyDB
//
#define BE_BDB_UNKN                       10000 // Неизвестная ошибка BDB
#define BE_BDB_INVAL                      10001 // (EINVAL) Недопустимое значение параметра вызова функции
#define BE_BDB_LOCKDEADLOCK               10002 // (DB_LOCK_DEADLOCK) A transactional database environment operation
	// was selected to resolve a deadlock.
#define BE_BDB_LOCKNOTGRANTED             10003 // (DB_LOCK_NOTGRANTED) A Berkeley DB Concurrent Data Store database
	// environment configured for lock timeouts was unable to grant a lock in the allowed time.
#define BE_BDB_OLDVERSION                 10004 // (DB_OLD_VERSION) The database cannot be opened without being first upgraded.
#define BE_BDB_REPHANDLEDEAD              10005 // (DB_REP_HANDLE_DEAD) When a client synchronizes with the master,
	// it is possible for committed transactions to be rolled back. This invalidates all the database and cursor handles
	// opened in the Library Version 11.2.5.2 The DB Handle 6/10/2011 DB C API Page 72 replication environment.
	// Once this occurs, an attempt to use such a handle will return
	// or
	// The application will need to discard the handle and open a new one in order to continue processing.
#define BE_BDB_REPLOCKOUT                 10006 // (DB_REP_LOCKOUT) The operation was blocked by client/master synchronization.
#define BE_BDB_UNKNDBTYPE                 10007 // Неизвестный тип файла данных (%s)
#define BE_BDB_SEQIDNFOUND                10008 // Идентификатор последовательности %s не найден.
#define BE_BDB_IDX_ZEROMAINTBL            10009 // Внутренняя ошибка: для индексной таблицы '%s' не определена таблица данных
#define BE_BDB_IDX_MAINTBLNOPENED         10010 // Для индексной таблицы '%s' не открыта таблица данных
#define BE_BDB_INVALSP                    10011 // Недопустимое значение параметра поиска записи
#define BE_DBD_REPLEASEEXPIRED            10012 // (DB_REP_LEASE_EXPIRED) Master lease has expired
#define BE_DBD_INNERTXN                   10013 // Другая транзакция BDB еще активна
#define BE_BDB_UNDEFSERIALIZECTX          10014 // Не определен контекст синхронизации для экземпляра BDbDatabase
//
// Database library error codes (18/06/99)
//
#define SDBERR_SUCCESS                        0 // No error
#define SDBERR_SLIB                           1 // SLIB (see SLERR_XXX SLIB.H)
#define SDBERR_BTRIEVE                        2 // BTRV (see BE_XXX above)
#define SDBERR_NOMEM                         11 // Not enough memory
#define SDBERR_FCRFAULT                      12 // File creating fault
#define SDBERR_FOPENFAULT                    13 // File opening fault
#define SDBERR_FRNMFAULT                     14 // File renaming fault
#define SDBERR_FRMVFAULT                     15 // File removing fault
#define SDBERR_CONNECTFAULT                  16 // Cant Connect
#define SDBERR_ALREADYEXIST                  17 // DB name already exist
#define SDBERR_INCOMPATDBVER                 18 // Недопустимая версия менеджера базы данных. Требуется '%s'.
// Backup component error codes
#define SDBERR_BU_NOCOPYPATH                101 // Path of backup copy not found
#define SDBERR_BU_DICTNOPEN                 102 // Dictionary not open (must call DBBackup::SetDictionary)
#define SDBERR_BU_SRCFOPEN                  103 // Error opening file for backuping
#define SDBERR_BU_NOFREESPACE               104 // Unsufficient disk free space
#define SDBERR_BU_COPYINVALID               105 // Backup copy invalid
//
// Max lengthes
//
#define MAXTABLENAME                         20
#define MAXVIEWNAME                          20
#define MAXFIELDNAME                         20
#define MAXINDEXNAME                         20
#define MAXFKEYNAME                          20
#define MAXUSERNAME                          30
#define MAXPWORDNAME                          8
//
// @todo Увеличить это значение (предварительно проанализировав последствия)
//
#define MAXLOCATION                          64
//
// Table flags
//
#define XTF_VLR               0x00000001
#define XTF_TRUNCATE          0x00000002
#define XTF_PREALLOC          0x00000004
#define XTF_COMPRESS          0x00000008 // v6.x
#define XTF_DICT              0x00000010 // DDF flag
#define XTF_KEYONLY           0x00000010 // Btrieve file flag v6.x
#define XTF_BALANCED          0x00000020 // v6.x
#define XTF_THRESHOLD10       0x00000040 // v6.x
#define XTF_THRESHOLD20       0x00000080 // v6.x
#define XTF_THRESHOLD30       (XTF_THRESHOLD10 | XTF_THRESHOLD20)
#define XTF_THRESHOLD         (XTF_THRESHOLD10 | XTF_THRESHOLD20)
#define XTF_EXTRADUP          0x00000100 // v6.x
#define XTF_MANUALKEYNUMBER   0x00000400 // v6.x
#define XTF_VAT               0x00000800 // v6.x
#define XTF_TEMP              0x00010000 // temporary table
#define XTF_DISABLEOUTOFTAMSG 0x10000000 // @v8.9.10
//
// Index flags
// Note:
//     XIF_DUP, XIF_MOD, XIF_REPDUP, XIF_ALLSEGNULL, XIF_ANYSEGNULL
//     flags must be equal for all segments of the same key
//
#define XIF_DUP            0x0001 // Index allows duplicates
#define XIF_MOD            0x0002 // Index is modifiable
#define XIF_BINARY         0x0004 // Standard binary type
#define XIF_ALLSEGNULL     0x0008 // Null Key (All-Segment)
#define XIF_SEG            0x0010 // Another seg is concat to this one in the index
#define XIF_ACS            0x0020 // There is alternate collating sequence
#define XIF_DESC           0x0040 // Index is in descending order
#define XIF_NAMED          0x0080 // Supplemental index v5.x
#define XIF_REPDUP         0x0080 // Repeating Duplicatable index v6.1
#define XIF_EXT            0x0100 // Index is an extended data type
#define XIF_ANYSEGNULL     0x0200 // Null Key (Any-Segment)
#define XIF_NOCASE         0x0400 // Case insensitivity
#define XIF_ALLSEGFLAGS (XIF_DUP|XIF_REPDUP|XIF_MOD|XIF_ALLSEGNULL|XIF_ANYSEGNULL)
#define XIF_UNIQUE     0x08000000 // Индекс уникальный (этот флаг используется только для //
	// отрицания XIF_DUP. В словаре Btrieve не прописывается //
//
// Types
//
typedef int16  BTBLID;
typedef int16  BFLDID;
typedef int16  BIDXID;
typedef uint16 BRECSZ;
typedef char   BTABLENAME[MAXTABLENAME];
typedef char   BFILENAME[MAXLOCATION];
typedef char   BFIELDNAME[MAXFIELDNAME];
//
//
//
struct ChunkHeader {
	SLAPI  ChunkHeader(long pos, long func, long aNum = 0);

	long   recPos; // Pos of the requested rec (for GetChunk, not for UpdateChunk)
	long   subFunc;
	union {
		long numChunks;
		long numRows;
		long chunkOffset; // (4..) subFunc == CHUNK_TRUNCATE
	};
};
//
//
//
struct RandChunkItem {
	SLAPI  RandChunkItem(long ofs, long aLen, void * p = 0);

	long   offset;
	long   len;
	long   ptr;
};
//
//
//
struct RectChunk {
	SLAPI  RectChunk(long ofs, long rs, long pDist, long p, long aDist);

	long   offset;  //  Record
	long   rowSize; //  |------|****|------|****|---------------------------
	long   ptrDist; //  |      |      |       |
	long   ptr;     //  0      offset ptrDist rowSize
	long   appDist; //
};
//
// XFile (DDF struct)
//
struct XFile {
	SString & SLAPI GetTableName(SString & rBuf) const;
	// @v5.8.10 char   * SLAPI GetTableName(char * pBuf, size_t bufLen) const; // @obsolete

	int16  XfId;        // Internal ID
	//
	// @attention
	// Если тип XfNam или XfLoc поменяется, то необходимо очень внимательно
	// просмотреть исходные коды на предмет использования sizeof(XfName), sizeof(XfLoc).
	//
	BTABLENAME XfName;  // Table name
	BFILENAME  XfLoc;   // File location (pathname)
	int8   XfFlags;     // File flags. If (XfFlags & 0x0010) then dictionary file else user-defined
	int16  XfOwnrLvl;   // Owner access level (A. Sobolev)
	int16  XfBTFlags;   // Btrieve File flags (A. Sobolev)
	int8   reserv[6];   // Real file has record size 97 bytes
	// Indexes: { XfId } { XfName } 
};
//
// XField (DDF struct)
//
struct XField {
	int16  XeId;        // Field ID
	int16  XeFile;      // ID of table to which this field belongs
	BFIELDNAME XeName;  // Field name or named index
	int8   XeDataType;  // Field type (0xff for a named index)
	int16  XeOffset;    // Field offset in table; idx number in named idx
	int16  XeSize;      // Field size
	int8   XeDec;       // Field decimal places
	int16  XeFlags;     // Flags (Bit 0 - case flag for str data types)
	int16  reserv;      // Real file has record size 32 bytes
	// Indexes: { XeId } { XeFile DUP } { XeName DUP } { XeFile XeName } 
};
//
// XIndex (DDF struct)
//
struct XIndex {
	int16  XiFile;
	int16  XiField;
	int16  XiNumber;
	int16  XiPart;
	int16  XiFlags;
	// Indexes: { XiFile DUP } { XiField DUP } 
};
//
// PageSzInfo
//
struct PageSzInfo {
	int16  pgSz;
	int16  maxKeySegs;
};
//
// DBIdxSpec
//
struct DBIdxSpec {
	int16  position; // 1..
	int16  length;
	int16  flags;
	union {
		long numVals; // # of unique key values (Used in B_STAT function)
		long reserv1;
	};
	int8   extType;
	char   nullValue;
	int16  reserv2;
	int8   keyNumber;
	int8   acsNumber;
};
//
// DBFileSpec
//
struct DBFileSpec {
	int16  RecSize;
	int16  PageSize;
	int16  NumKeys;    // B_STAT returns numKeys in lo byte and file ver in hi byte
	union {
		long NumRecs;  // Used in B_STAT function
		long Reserv1;
	};
	int16  Flags;      // XTF_XXX
	int8   ExtraDup;   // B_STAT returns # of unused dup pointers
	union {
		int8 Reserv2;
		int8 NumSeg;   // Used by operator+(DBFileSpec &, DBIdxSpec &)
		// Should set this field to 0 before first call
		// of operator+(DBFileSpec &, DBIdxSpec &).
		// Function Btrieve::CreateTable resets this field to 0
	};
	int16  AllocPages; // B_STAT returns # of unused pages
};

DBFileSpec  & SLAPI operator + (DBFileSpec &, DBIdxSpec &);
ChunkHeader & SLAPI operator + (ChunkHeader &, RandChunkItem &);
ChunkHeader & SLAPI operator + (ChunkHeader &, RectChunk &);
//
// Extended operation buffer format
//
struct BExtHeader {
	uint16 bufLen;    // Total lenght of the data buffer
	uint16 signature; // "EG" begin with after current || "UC" begin with current ("UC" for GetExtended only)
	uint16 maxSkip;   // Maximum skipping records. If maxSkip == 0 then btrieve uses a system-determined value
	uint16 numTerms;  // Number of terms in the logic filter condition
};

struct BExtTerm {
	uchar  fldType;
	uint16 fldLen;
	uint16 fldOfs; // 0..
	uchar  cmp;    // 1 - Eq, 2 - Gt, 3 - Lt, 4 - NEq, 5 - Ge, 6 - Le
	//                Bias : +32 - acs, +64 - second operand is another
	//                field rather than a constant, +128 - no case
	uchar  link;   // 0 - last term, 1 - AND, 2 - OR
	//                2 bytes - offset of the second field || n bytes - comparing constant
};

struct BExtTail {
	uint16 numRecs; // Number of records to be retrieved
	uint16 numFlds; // Number of fields extracted from each record
};

struct BExtTailItem {
	uint16 fldLen;
	uint16 fldOfs; // 0..
};

struct BExtResultHeader {
	uint16 numRecs; // Number of records returned
};

struct BExtResultItem {
	uint16 recLen;         // Length of the record image
	RECORDNUMBER position; // Currency position of record
	//                        ... recLen bytes - record image
};
//
// Create file mode
//
enum {
	crmTTSReplace   =  0,
	crmTTSNoReplace = -1,
	crmReplace      = -2,
	crmNoReplace    = -3
};
//
// Search operation mode
//
enum {
	spFirst, // #00
	spLast,  // #01
	spEq,    // #02
	spLt,    // #03
	spLe,    // #04
	spGt,    // #05
	spGe,    // #06
	spNext,  // #07
	spPrev,  // #08
	spNextDup = 1000 + spNext, // BDbTable only
	spPrevDup = 1000 + spPrev  // BDbTable only
};
//
// Encrypt protection mode
//
enum {
	pmReadOnly  = 0x0001,
	pmEncrypt   = 0x0002
};
//
// Open modes
//
enum {
	omNormal    =  0,
	omAccel     = -1,
	omReadOnly  = -2,
	omReserved  = -3,
	omExclusive = -4,
	omSqlServer = -1000, // Таблицу следует открывать в контексте SqlServer'а
};
//
// Descr: класс, объединяющий статические методы вызова функций Btrieve
//
class Btrieve {
public:
	static int FASTCALL StartTransaction(int concurrent = 0, int lock = 0);
	static int SLAPI RollbackWork();
	static int SLAPI CommitWork();
	static int SLAPI AddContinuous(const char * fname /* "volume:\path[,volume:\path]*" */);
	static int SLAPI RemoveContinuous(const char * fname /* if fname == 0 then remove all files */);
	static int SLAPI GetVersion(int * pMajor, int * pMinor, int * pIsNet);
	static int SLAPI Reset(int station);
	static int SLAPI CreateTable(const char * pFileName, DBFileSpec & rTblDesc, int createMode, const char * pAltCode);

	static const PageSzInfo LimitPgInfo[NUMPGSIZES];
};
//
// Function's prototypes
//
int    FASTCALL Btr2SLibType(int);
int    FASTCALL SLib2BtrType(int);
void   SLAPI DBRemoveTempFiles();
//
// Класс BNKey ссылается на поля таблицы DBTable
// по индексу в списке DBTable::fields
//
class BNKey {
friend class BNKeyList;
public:
	SLAPI  BNKey();
	int    SLAPI operator !() const { return (data == 0); }
	//
	// Don't call BNKey::addSegment for
	// object returned by BNKeyList::getKey
	//
	int    SLAPI addSegment(int fldID, int flags);
	int    SLAPI setFieldID(int seg, int newID);
	int    SLAPI setKeyParams(int keyNumber, int acsNumber = 0);
	int    SLAPI getNumSeg() const;
	int    SLAPI getKeyNumber() const;
	int    SLAPI getACSNumber() const;
	int    SLAPI getFieldID(int seg) const;
	//
	// seg = 0.., if UNDEF, then for all
	//
	int    SLAPI getFlags(int seg = UNDEF) const;
	int    SLAPI setFlags(int flags, int seg = UNDEF);
	int    SLAPI containsField(int fldID, int * pSeg) const;
	int    SLAPI compareKey(const DBTable *, const void *, const void *) const;
private:
	void   SLAPI destroy();
	int    SLAPI reset();
	void * data;
};

class BNKeyList {
friend class DBTable;
public:
	SLAPI  BNKeyList();
	SLAPI  BNKeyList(const BNKeyList & rS);
	SLAPI ~BNKeyList();
	BNKeyList & FASTCALL operator = (const BNKeyList & s);
	void   SLAPI reset();
	int    FASTCALL copy(const BNKeyList *);
	int    SLAPI Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Function MNKeyList::addKey calls BNKey::destroy()
	//
	int    FASTCALL addKey(BNKey &);
	uint   SLAPI getNumKeys() const;
	//
	// Descr: Удаляет ключ с номером (позиции) key
	//
	int    SLAPI removeKey(int key);
	uint   FASTCALL getKeySize(int key) const;
	uint   SLAPI getSegOffset(int key, int seg) const;
	const BNField & SLAPI field(int key, int seg) const;
	//
	// makeKey returns:
	//   0 - segment can not be assigned (parameter seg > 0 || cmp == _NE_)
	//   1 - key assigned
	//
	int    SLAPI makeKey(int key, int seg, int cmp, const void * val, void * dest) const;
	int    FASTCALL compareKey(int key, const void *, const void *) const;
	//
	// Don't call BNKeyList::addKey and BNKeyList::removeKey
	// while using result of BNKeyList::getKey
	//
	BNKey  FASTCALL getKey(int position) const;
	BNKey  FASTCALL operator[](int i) const { return getKey(i); }

	uint   SLAPI setBound(int key, int seg, int min_max /*0 - min, !0 - max*/, void * dest) const;
	void   FASTCALL setTableRef(uint);
private:
	const  DBTable & SLAPI table() const;
	int    FASTCALL findKey(int key) const;
	int    FASTCALL findKeyByNumber(int keyNumber) const;
	uint   SLAPI GetNumCells() const;
	void * P_Data;
};
//
// Descr: Утилита, формирующая Alternate Collating Sequence (ACS),
//   нечувствительных к регистру русских (CP 866) и английских символов.
// ARG(pBuf OUT): @#{vptr} Указатель на буфер, длиной не менее 265 байт.
// Returns:
//   pBuf
//
char * SLAPI GetRusNCaseACS(char * pBuf);

extern int _db_open_count; // Счетчик открытых таблиц DBTable
extern int _db_open_peak;  // Пиковое количество открытых таблиц DBTable
//
// При создании Btrieve файла функцией DBTable::createFile можно
// использовать макрос SET_CRM_TEMP для того, чтобы при закрытии
// словаря данных этот файл был-бы автоматически удален.
// Параметром этого макроса должна являтся одна из констант crmXXX.
// Еще два макроса предназначены для работы с комбинированным
// флагом внутри функции DBTable::createFile.
// Столь сложный подход объясняется тем, что Btrieve использует
// для опций создания файла отрицательные значения с которыми
// не работают классические флаги.
//
#define SET_CRM_TEMP(crm)    ((crm) & ~0x0100)
#define RESET_CRM_TEMP(crm)  ((crm) | 0x0100)
#define IS_CRM_TEMP(crm)     (!((crm) & 0x0100))

#define DBOPEN_LOWLEVEL open_

union DBRowId { // @persistent size=32
public:
	DBRowId();
	int    IsLong() const;
	operator RECORDNUMBER() const;
	DBRowId & FASTCALL operator = (RECORDNUMBER n);
	void   SetZero();
	void   SetMaxVal();
	SString & FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL FromStr(const char *);
private:
	uint32 B;
	uint8  S[32];
};

typedef TSVector <DBRowId> DBRowIdArray; // @v9.8.4 TSArray-->TSVector

struct DBLobItem {
	uint   FldN;
	uint32 Size;
	uint32 Loc;
	uint8  StrgInd;    // Индикатор сохраняемого значения поля (see SLob::Serialize) //
	uint8  Reserve[3];
};

class DBLobBlock : public SVector { // @v9.8.4 SArray-->SVector
public:
	DBLobBlock();
	int    SetSize(uint fldIdx, size_t sz);
	int    GetSize(uint fldIdx, size_t * pSz) const;
	int    SetLocator(uint fldIdx, uint32 loc);
	int    GetLocator(uint fldIdx, uint32 * pLoc) const;
	int    FASTCALL SearchPos(uint fldIdx, uint * pPos) const;
	SBuffer Storage; // Временное хранилище данных из LOB-полей записи.
		// Используется функциями DBTable::StoreAndTrimLob() и DBTable::RestoreLob()
};
//
// Descr: SQL-оператор. Предполагается независимость от конкретного
//   провайдера SQL-сервера.
//
class SSqlStmt {
public:
	friend class SOraDbProvider;
	//
	//   @todo
	//   Конструктор сделать приватным так как создавать экземпляры
	//   этого класса могут только классы, непосредственно управляющие
	//   интерфейсом с конкретной СУБД.
	//
	SSqlStmt(DbProvider * pDb, const char * pText);
	~SSqlStmt();
	int    SetText(const char * pText);
	int    IsValid() const;
	//
	// Descr: Инициализирует подстановочные буферы для связывания переменных
	//   SQL-оператора.
	//   Функция должна быть вызвана до того, как возникнет...
	//
	int    SetupBindingSubstBuffer(int dir, uint count);
	int    BindItem(int pos, uint count, TYPEID typ, void * pDataBuf);
	//
	// Descr: Реализует привязку переменной, хранящей позицию извлеченной строки.
	//   Следует использовать только для извлечения данных (dir > 0).
	// Note: Должна вызываться перед BindData()
	//
	int    BindRowId(int pos, uint count, DBRowId * pDataBuf);
	int    BindData(int dir, uint count, const BNFieldList & rFldList, const void * pDataBuf, DBLobBlock *);
	int    BindData(int dir, uint count, const DBFieldList & rFldList, const void * pDataBuf, DBLobBlock *);
	int    BindKey(const BNKeyList & rKeyList, int idxN, const void * pDataBuf);
	//
	// Descr: Подставляет реальные данные по привязкам (binding), заданным до этого
	//   функцией SSqlStmt::BindData(-1,...)
	// ARG(recNo   IN): @#{0..BL.Dim-1} номер записи, данные которой должны быть подставлены в привязку.
	//
	int    SetData(uint recNo);
	//
	// Descr: Подставляет реальные данные по привязкам (binding), заданным до этого
	//   функцией SSqlStmt::BindData(-1,...)
	// Note: Отличается от SetData тем, что вызывает DbProvider::ProcessBinding
	//   с параметром action = -2 (привязка для изменения данных).
	//
	int    SetDataDML(uint recNo);
	int    GetOutData(uint recNo);
	int    GetData(uint recNo);
	int    Exec(uint count, int mode);
	int    Fetch(uint count, uint * pActualCount);
	int    Describe();
	uint   GetBindingCount(int dir) const;
	DBLobBlock * GetBindingLob() { return BL.P_Lob; }

	struct Bind {
		Bind();
		DECL_INVARIANT_C();
		enum {
			fSubst    = 0x0001, // Используется подстановка
			fCalcOnly = 0x0002  // Если этот флаг установлен, то функция ProcessBinding
				// при параметре action==0 только рассчитывает необходимый размер
				// в буфере подстановки и привсаивает результат полю Bind::NtvSize.
		};
		int16  Pos;        // Позиция привязки. <0 - входящая привязка, >0 - исходящая привязка
		uint16 NtvTyp;     // Тип данных, специфичный для SQL-сервера
		uint16 Flags;      //
		uint16 NtvSize;    // Размер данных, специфичный для SQL-сервера
		uint32 H;          // Манипулятор элемента привязки
		void * P_Data;     // Указатель на реальные данные поля, определенные приложением //
		//uint32 * P_LobSz;  // Указатель на актуальную длину данных в поле типа LOB (используется только для S_BLOB S_CLOB)
		uint32 SubstSize;  // @#{SubstSize >= Dim*ItemSize} Размер подстановочного участка буфера
		uint32 SubstOffs;  // @#{SubstOffs >= 4} Смещение до подстановочного участка буфера
		uint32 Dim;        // Размерность связываемого массива. Обычно Dim == BindArray::Dim
		uint32 ItemSize;   // Размер одного элемента массива (размерности Dim)
		uint32 IndPos;     // Позиция индикатора поля //
		uint32 FslPos;     //
		TYPEID Typ;        // Внутренний тип данных
	};
private:
	enum {
		fError      = 0x0001,
		fForUpdate  = 0x0002,
		fNoMoreData = 0x0004  // Последний вызов Fetch выбрал все доступные данные. Больше вызывать Fetch не следует.
	};
	class BindArray : public TSVector <SSqlStmt::Bind> {
	public:
		BindArray(uint dim = 1);
		uint   Dim; // Размерность связываемого массива элементов.
		DBLobBlock * P_Lob;
	};

	int    InitBinding();
	int    AllocBindSubst(uint32 itemCount, uint32 itemSize, Bind * pBind);
	size_t Helper_AllocBindSubst(uint32 itemCount, uint32 itemSize, int calcOnly);
	int    AllocIndSubst(uint32 itemCount, Bind * pBind);
	int    AllocFslSubst(uint32 itemCount, Bind * pBind);
	void * FASTCALL GetBindOuterPtr(const Bind * pBind, uint rowN) const;
	size_t FASTCALL GetBindOuterSize(const Bind * pBind) const;
	void * FASTCALL GetIndPtr(const Bind * pBind, uint rowN) const;

	DbProvider * P_Db;
	uint32 H;
	long   Flags;
	BindArray BL;
	SdRecord Descr;
	SBaseBuffer BS;       // Буфер обмена для связывания входящих и исходящих переменных
	size_t TopBindSubst;
	size_t IndSubstPlus;   // Смещение в буфере BS до массива индикаторов нулевого значения для //
		// исходящей привязки. Каждый элемент массива занимает 2 байта.
		// Размерность массива [BL.Dim * GetBindingCount(+1)]
	size_t IndSubstMinus;  // Смещение в буфере BS до массива индикаторов нулевого значения для //
		// входящей привязки. Каждый элемент массива занимает 2 байта.
		// Размерность массива [BL.Dim * GetBindingCount(-1)]
	size_t FslSubst;       // Смещение в буфере BS до массива длин результатов выборки.
		// Каждый элемент массива занимает 2 байта. Размерность массива [BL.Dim * GetBindingCount(+1)]
	/*
	uint16 * P_FSL;       // Fetch Size List. Массив, в который SQL-сервер заносить длины результатов выборки.
		// Размерность массива: [BL.Dim * GetBindingCount(+1)]
	*/
	//SBaseBuffer Ind;      // Буфер обмена для связывания значений индикатора
	//size_t TopIndSubst;
	//SBaseBuffer Fsl;      // Fetch Size List. Массив, в который SQL-сервер заносить длины результатов выборки.
		// Размерность массива: [BL.Dim * GetBindingCount(+1)]
};
//
//
//
struct DbTableStat {
	DbTableStat();
	void   Clear();
	//
	// Descr: Флаги элементов статистики, которые могут быть инициализированы
	//   в этой структуре.
	//
	enum {
		iID         = 0x0001,
		iOwnerLevel = 0x0002,
		iFlags      = 0x0004,
		iName       = 0x0008,
		iLocation   = 0x0010,
		iFixRecSize = 0x0020,
		iNumRecs    = 0x0040,
		iFldCount   = 0x0080,
		iIdxCount   = 0x0100,
		iFldList    = 0x0200,
		iIdxList    = 0x0400,
		iOwnerName  = 0x0800,
		iSpaceName  = 0x1000
	};
	long   ReqItems;       // Запрошенные элементы
	long   RetItems;       // Инициализированные элементы
	long   UnsupItem;      // Неподдерживаемые элементы
	long   ID;
	int    OwnerLevel;
	long   Flags;          // XTF_XXX
	uint32 FixRecSize;
	uint64 NumRecs;
	uint32 FldCount;       // Количество полей в таблице
	uint32 IdxCount;       // Количество индексов в таблице
	uint32 PageSize;       // Размер страницы
	SString TblName;       // Наименование спецификации таблицы
	SString Location;      // Местоположение таблицы
	SString OwnerName;     // @ora
	SString SpaceName;     // @ora
	BNFieldList FldList;   // Список полей
	BNKeyList   IdxList;   // Список индексов
};

class DBTable {
public:
	friend class DbProvider;
	friend class BDictionary;
	friend class DbDict_Btrieve;
	friend class SOraDbProvider;
	friend struct DBField;

	static void   FASTCALL InitErrFileName(const char * pFileName);
	static const  char * SLAPI GetLastErrorFileName();
	static int (*OpenExceptionProc)(const char * pFileName, int btrErr);
	static const char * CrTempFileNamePtr; // =0x0003 Специальное значение указателя. Если функция DBTable::open
		// получает такой указатель в качестве имени файла, то создает временный файл.

	SLAPI  DBTable();
	//
	// Descr: Создает экземпляр таблицы базы данных с именем спецификации pTblName.
	// ARG(pTblName  IN): Наименование спецификации таблицы в словаре данных.
	//   Если pTblName == 0, тогда создается экземпляр таблицы по имени файла pFileName.
	//   Пир этом используется низкоуровневая процедура открытия таблицы БД, не
	//   завязанная на спецификацию в словаре данных.
	// ARG(pFileName IN): Наименование файла данных, который следует открыть
	//   в этом конструкторе. Если pFileName == 0, то констурктор использует имя файла
	//   данных из спецификации pTblName (в этом случае pTblName должен содержать
	//   правильное имя спецификации и не быть равным нулю).
	//
	//   Обращаем внимание на то, что природа файла данных разная у разных провайдеров
	//   управления базами данных.
	//   Для btrieve это файл данных как он определен в файловой системе,
	//   для SQL-серверов это - экземпляр таблицы в области базы данных.
	//
	//   Если pFileName == DBTable::CrTempFileNamePtr, тогда конструктор самостоятельно
	//   создает временный файл данных, который и открывает.
	// ARG(openMode  IN): Опции открытия файла данных.
	// ARG(pDbP      IN): Провайдер БД, в области действия которого создается экземпляр
	//   объекта. Если pDbP == 0, то экземпляр создается в области действия CurDict.
	//
	SLAPI  DBTable(const char * pTblName, const char * pFileName = 0, int openMode = omNormal, DbProvider * pDbP = 0);
	SLAPI ~DBTable();
	DbProvider * GetDb() { return P_Db; }
	int    SLAPI open(const char * pTblName, const char * pFileName = 0, int openMode = omNormal);
	int    SLAPI close();
	int    SLAPI IsOpened() const;
	int    FASTCALL getField(uint fldN, DBField *) const;
	int    SLAPI getFieldByName(const char * pName, DBField *) const;
	int    SLAPI getFieldValue(uint fldN, void * pVal, size_t * pSize) const;
	int    SLAPI setFieldValue(uint fldN, const void * pVal);
	int    SLAPI getFieldValByName(const char * pName, void * pVal, size_t * pSize) const;
	int    SLAPI setFieldValByName(const char * pName, const void * pVal);
	int    SLAPI putRecToString(SString &, int withFieldNames);
	const  char * GetFileName() const { return OpenedFileName; }
	int    FASTCALL HasNote(DBField * pLastFld) const;
	int    FASTCALL HasLob(DBField * pLastFld) const;
	void   FASTCALL setDataBuf(void * aBuf, RECORDSIZE aBufLen);
	void * FASTCALL getDataBuf() { return static_cast<void *>(P_DBuf); } // @v10.3.5 (char *)-->(void *)
	const  void * FASTCALL getDataBufConst() const{ return static_cast<const void *>(P_DBuf); } // @v10.3.5 (char *)-->(void *)
	void   FASTCALL setBuffer(SBaseBuffer &);
	const  SBaseBuffer FASTCALL getBuffer() const;
	int    allocOwnBuffer(int size = -1);
	int    SLAPI InitLob();
	uint   SLAPI GetLobCount() const;
	int    SLAPI GetLobField(uint n, DBField * pFld) const;
	int    setLobSize(DBField fld, size_t sz);
	int    getLobSize(DBField fld, size_t * pSz) const;
	int    readLobData(DBField fld, SBuffer & rBuf) const;
	int    writeLobData(DBField fld, const void * pBuf, size_t dataSize, int forceCanonical = 0);
	void   FASTCALL destroyLobData(DBField fld);
	DBLobBlock * getLobBlock();
	int    SLAPI StoreAndTrimLob();
	int    SLAPI RestoreLob();
	DBRowId * getCurRowIdPtr(); // @realy private function
	void   SLAPI clearDataBuf();
	void   FASTCALL copyBufTo(void * pBuf) const;
	void   FASTCALL copyBufFrom(const void * pBuf);
	void   FASTCALL copyBufFrom(const void * pBuf, size_t srcBufSize);
	//
	// Descr: Копирует данные полей, соответствующих индексу idx в буфер
	//   ключа pKey.
	//
	int    SLAPI copyBufToKey(int idx, void * pKey) const;
	//
	// Descr: Специальная версия copyBufFrom учитывающая последнее LOB-поле
	//
	int    FASTCALL copyBufLobFrom(const void * pBuf, size_t srcBufSize);
	RECORDSIZE SLAPI getBufLen() const;
	RECORDSIZE SLAPI getRetBufLen() const { return retBufLen; }
	int    SLAPI getCurIndex() const { return index; }
	void   FASTCALL setIndex(int i) { index = static_cast<int16>(i); }
	//
	// Descr: Флаги поиска (DbProvider::Implement_Search)
	//
	enum {
		sfForUpdate = 0x0001, // SELECT FOR UPDATE
		sfKeyOnly   = 0x0002, // Извлечь только ключевые поля, по заданному индексу
		sfDirect    = 0x0004  // Извлечь запись по rowid
	};

	int    FASTCALL search(void * key, int srchMode);
	//
	// Descr: Функция, используемая для извлечения записи перед изменением или
	//   удалением. Для btrieve вызывает search(int idx, void * key, int srchMode).
	//   Для SQL-серверов должна реализовываться специальным образом.
	//
	int    FASTCALL searchForUpdate(void * key, int srchMode);
	int    FASTCALL search(int idx, void * key, int srchMode);
	//
	// Descr: Функция, используемая для извлечения записи перед изменением или
	//   удалением. Для btrieve вызывает search(int idx, void * key, int srchMode).
	//   Для SQL-серверов должна реализовываться специальным образом.
	//
	int    FASTCALL searchForUpdate(int idx, void * key, int srchMode);
	//
	// Descr: Считывает текущую запись с блокировкой для изменения.
	//
	int    SLAPI rereadForUpdate(int idx, void * pKey);
	int    SLAPI searchKey(int idx, void * pKey, int srchMode);
	int    SLAPI step(int srchMode);
	int    SLAPI getExtended(void * key, int srchMode, int lock = 0);
	int    SLAPI stepExtended(int srchMode, int lock = 0); // @unused
	//
	// В функциях getDirect допускается (key == 0).
	//
	int    SLAPI getDirect(int idx, void * pKey, const DBRowId &);
	int    SLAPI getDirectForUpdate(int idx, void * pKey, const DBRowId &);
	int    SLAPI getChunk(const ChunkHeader * pChunk, int lock = 0); // @unused
	int    FASTCALL getPosition(DBRowId * pPos);
	int    SLAPI findPercentage(void * pKey, int16 * pRelPos); // @unused
	int    SLAPI getByPercentage(int16 relPos, int keyIndex); // @unused
	int    FASTCALL insertRec();
	int    FASTCALL insertRec(int idx, void * pKey);
	int    FASTCALL insertRecBuf(const void * pDataBuf);
	int    SLAPI insertRecBuf(const void * pDataBuf, int idx, void * pKeyBuf);
	int    FASTCALL updateRec();
	int    FASTCALL updateRecBuf(const void * pDataBuf);
	//
	// Descr: То же, что и updateRec, но не изменяет текущую позицию курсора таблицы
	//
	int    FASTCALL updateRecNCC(); // @<<::updateForCb()
	int    SLAPI deleteRec();
	int    SLAPI deleteByQuery(int useTa, DBQ & rQ);
	int    SLAPI updateChunk();
	int    SLAPI unlock(int isAll);
	int    SLAPI getNumKeys(int16 * pNumKeys);
	//
	// Caller must destroy ptr returned by getIndexSpec
	//
	DBIdxSpec * SLAPI getIndexSpec(int idxNo, int * pNumSeg);
	int    FASTCALL getNumRecs(RECORDNUMBER * pNumRecs);
	int    SLAPI getTabFlags(int16 * pFlags);
	//
	// Descr: Возвращает размер фиксированной части записи (без "хвостов" переменной длины)
	//
	RECORDSIZE FASTCALL getRecSize() const;
	int    SLAPI GetFileStat(long reqItems, DbTableStat * pStat);
	int    SLAPI SerializeSpec(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    SLAPI SerializeRecord(int dir, void * pRec, SBuffer & rBuf, SSerializeContext * pCtx);
	int    SLAPI SerializeArrayOfRecords(int dir, SArray * pList, SBuffer & rBuf, SSerializeContext * pCtx);
	int    SLAPI SerializeArrayOfRecords(int dir, SVector * pList, SBuffer & rBuf, SSerializeContext * pCtx);
	//
	// Format of data buffer for Insert Extended operation:
	//     Fixed portion:
	//         2 bytes - Number of records inserted
	//     Repeating portion:
	//         2 bytes - Length of record image
	//         n bytes - Record image
	// ARG(NCC IN): No Change Currency (не изменять текущую позицию таблицы)
	//
	int    Btr_Implement_BExtInsert(BExtInsert * pBei); // really private
	void   FASTCALL SetPageSize(uint newPageSize);
	int    Debug_Output(SString & rBuf) const;
	int    SLAPI GetHandle() const { return handle; }
	BTBLID SLAPI GetTableID() const { return tableID; }
	void   FASTCALL SetTableID(BTBLID _id);
	const  BNKeyList & SLAPI GetIndices() const { return indexes; }
	void   SLAPI SetIndicesTblRef();
	const  BNFieldList & SLAPI GetFields() const { return fields; }
	BNFieldList & SLAPI GetFieldsNonConst() { return fields; }
	//
	// Descr: returns fileName
	//
	const SString & SLAPI GetName() const { return fileName; }
	//
	// Descr: set fileName
	//
	void   FASTCALL SetName(const char * pN);
	const  char * SLAPI GetTableName() const { return tableName; }
	void   FASTCALL SetTableName(const char * pN);
	int    SLAPI GetFlags() const { return flags; }
	void   FASTCALL SetFlag(int f);
	void   FASTCALL ResetFlag(int f);
	int    SLAPI AddField(const char * pName, TYPEID fldType, int fldId = UNDEF);
	int    FASTCALL AddField(const BNField & rF);
	int    FASTCALL AddKey(BNKey & rK);
public:
	//
	struct SelectStmt : public SSqlStmt {
		SelectStmt(DbProvider * pDb, const char * pText, int idx, int sp, int sf);
		int    Idx;
		int    Sp;
		long   Sf;
		int8   Key[512];
	};
	int    SLAPI SetStmt(SelectStmt * pStmt); // @private
	SelectStmt * SLAPI GetStmt();             // @private
	int    SLAPI ToggleStmt(int release);     // @private
protected:
	SLAPI  DBTable(const char *, const char *, void * pFlds, void * pData, int openMode, DbProvider * pDbP = 0);
	int    SLAPI getStat(void ** ppInfo, uint16 * pBufSize);
private:
	int    SLAPI Init(DbProvider * pDbP);
	void   SLAPI InitErrFileName();
	int    FASTCALL Ret(int ret);
	int    SLAPI Btr_Open(const char * pName, int openMode = omNormal, char * pPassword = 0);
	int    SLAPI Btr_Close();
	int    SLAPI Btr_GetStat(long reqItems, DbTableStat * pStat);
	int    SLAPI Btr_Encrypt(char * pPassword, int protectMode);
	int    SLAPI Btr_Decrypt();
	int    SLAPI Btr_ProcessLobOnReading();
	int    Btr_Implement_InsertRec(int idx, void * pKeyBuf, const void * pData);
	int    Btr_Implement_UpdateRec(const void * pDataBuf, int ncc);
	int    Btr_Implement_DeleteRec();
	int    Btr_Implement_Search(int idx, void * pKey, int srchMode, long sf);
	int    FASTCALL Btr_Implement_GetPosition(DBRowId * pPos);
	void   FASTCALL OutOfTransactionLogging(const char * pOp) const;
	int    SLAPI Helper_SerializeArrayOfRecords(int dir, SVectorBase * pList, SBuffer & rBuf, SSerializeContext * pCtx);

	enum {
		sOpened_    = 0x0001, // Таблица открыта на низком уровне (open_)
		sOwnDataBuf = 0x0002, // Экземпляр использует самостоятельно распределенный буфер записи
			// (деструкция либо установка нового буфера приведет к разрушению области памяти по адресу buf).
		sHasLob     = 0x0004, // Таблица имеет поля переменной длины типа BLOB или CLOB
		sHasNote    = 0x0008, // Последнее поле таблицы - строка переменной длины типа NOTE
		sHasAutoinc = 0x0010  // Таблица имеет поле (поля) типа AUTOINCREMENT
			// Вставка записей в такую таблицу, возможно, должна обрабатываться специальным образом.
	};

	void * P_DBuf;
	int    handle;
	int    flags;
	long   State;
	DbProvider * P_Db;     // @notowned Указатель на провайдера БД
	SelectStmt * P_Stmt;   // Последний SQL-оператор, использовавшийся для поиска
	SelectStmt * P_OppStmt; // SQL-оператор, сохраняемый при переключении направления выборки.
	DBRowId CurRowId;      // Текущая позиция записи (используется для SQL-серверов)
	DBRowId LastLockedRow; // @v8.1.4 Позиция последней заблокированной записи. Используется для разблокировки, если не было изменения.
	BTBLID tableID;        // X$FILE.XfId
	int16  ownrLvl;
	int16  index;
	RECORDSIZE bufLen;
	RECORDSIZE retBufLen;
	RECORDSIZE FixRecSize; // @*DBTable::open
	uint16 PageSize;       // Размер страницы, определенный в спецификации.
	uint8  Reserve[18];    // @alignment // @v9.0.0 [10]-->[14] Размер SArray снизился на 4 байта: а sizeof(DBTable) должен быть кратен 32
		// @v9.8.4 [14]-->[18] Уменьшился размер DBLobBlock из-за перехода SArray-->SVector
	BTABLENAME tableName;
	BNFieldList fields;
	BNKeyList   indexes;
	SString fileName;
	SString OpenedFileName;
	DBLobBlock LobB;
	char   FPB[256];
	//
	// Структура должна быть выравненена до размера, кратного 32.
	// В результате буфер данных, находящийся в начале порожденной структуры будет
	// находиться на границе кэш-линии, что увеличит скорость работы.
	// Общий размер регулируется полем Reserve[] (see above)
	//
};
//
// Directory structure of database
//
// \BACKUP             Backup directory
//       \XXXXXXXX ... One or more subdirectory of backup copies
// \SYS                System directory (common for all databases)
//       FILE.DDF      Dictionary of database tables
//       FIELD.DDF     Dictionary of database fields
//       INDEX.DDF     Dictionary of database indexies
// \DAT
//       DB.INF        Database info file (not implemented yet)
//       DBPA.         Database protected area
//       BACKUP.INF    Database backup info
//       XXX.BTR ...   Data files
//
//
// Event codes for callback function
// transmitted from BDictionary::recoverTable
//
#define BREV_START       1 // Начало процесса (открыт исходный файл, создан
	// и открыт файл-приемник).
	// Параметры (char *)lp1 - имя исходного файла
	//           (char *)lp2 - имя файла-приемника
#define BREV_FINISH      2 // Процесс завершен
	// Параметры lp1 - общее количество перенесенных записей
	//           lp2 - общее количество записей в исходном файле
#define BREV_PROGRESS    3 // Успешно перенесена одна запись
	// Параметры lp1 - общее количество перенесенных записей
	//           lp2 - общее количество записей в исходном файле
	//           (char *)vp  - имя исходного файла
#define BREV_ERRINS      4 // Ошибка переноса записи в файл-приемник
	// Параметры lp1 - физический адрес записи
	//           lp2 - длина записи
	//           vp  - бинарный образ записи
#define BREV_ERRSTEP     5 // Ошибка чтения записи из исходного файла
	// Параметры lp1 - физический адрес предыдущей записи
	//
#define BREV_ERRCREATE   6 // Ошибка создания файла-приемника.
	// Параметры (char *)lp1 - имя файла-приемника
#define BREV_ERRDELPREV  7 // Ошибка удаления предшествующего файла-приемника
	// Параметры (char *)lp1 - имя файла-приемника
#define BREV_ERRRENAME   8 // Ошибка переименования файла
	// Параметры (const char *)lp1 - имя файла-источника, (const char *)lp2 - имя файла-приемника
//
// Descr: Структура, передаваемая в качестве параметра функции BDictionary::recoverTable
//
class BRecoverParam {
public:
	SLAPI  BRecoverParam();
	//
	// Функция callbackProc должна вернуть значение (>0) если следует
	// продолжить процесс и 0 - если процесс следует прервать
	//
	virtual int SLAPI callbackProc(int event, const void * lp1 = 0, const void * lp2 = 0, const void * vp = 0);
	// IN {
	const  char * P_DestPath;
	const  char * P_BakPath;
	int    Format;
	uint   Flags;
	// }
	// OUT {
	RECORDNUMBER OrgNumRecs;
	RECORDNUMBER ActNumRecs;
	int    ErrCode;
	long   Tm;
	// }
};

class DbDictionary {
public:
	static void SetCreateInstanceProc(DbDictionary * (*proc)(const char * pPath, long options));
	static DbDictionary * CreateInstance(const char * pPath, long options);

	SLAPI  DbDictionary();
	virtual SLAPI ~DbDictionary();
	//
	// Note: Функции DbDictionary в структуре DbTableStat по умолчанию должны
	//   заполнить следующие поля: {ID, OwnerLevel, Flags, TblName, Location}
	//

	virtual int SLAPI LoadTableSpec(DBTable * pTbl, const char * pTblName) = 0;
	virtual int SLAPI CreateTableSpec(DBTable * pTbl) = 0;
	virtual int SLAPI DropTableSpec(const char * pTblName, DbTableStat * pStat) = 0;
	virtual int SLAPI GetTableID(const char * pTblName, long * pID, DbTableStat * pStat) = 0;
	virtual int SLAPI GetTableInfo(long tblID, DbTableStat * pStat) = 0;

	enum {
		gltSkipDict     = 0x0001,
		gltSkipEmptyLoc = 0x0002
	};
	virtual int SLAPI GetListOfTables(long options, StrAssocArray * pList) = 0;

	int    SLAPI IsValid() const;
protected:
	enum {
		stError  = 0x0001
	};
	long   State;
private:
	static DbDictionary * (*CreateInstanceProc)(const char * pPath, long options);
};

class DbDict_Btrieve : public DbDictionary {
public:
	SLAPI  DbDict_Btrieve(const char * pPath);
	virtual SLAPI ~DbDict_Btrieve();
	virtual int SLAPI LoadTableSpec(DBTable * pTbl, const char * pTblName);
	virtual int SLAPI CreateTableSpec(DBTable * pTbl);
	virtual int SLAPI DropTableSpec(const char * pTblName, DbTableStat * pStat);
	virtual int SLAPI GetTableID(const char * pTblName, long * pID, DbTableStat * pStat);
	virtual int SLAPI GetTableInfo(long tblID, DbTableStat * pStat);
	virtual int SLAPI GetListOfTables(long options, StrAssocArray * pList);
private:
	struct _FldListQuery {
		BExtHeader   h;
		BExtTerm     t1;
		int16        tblID;
		BExtTerm     t2;
		char         minus_one;
		BExtTail     th;
		BExtTailItem ti[2];
	} flq;
	struct _IdxListQuery {
		BExtHeader   h;
		BExtTerm     t;
		int16        tblID;
		BExtTail     th;
		BExtTailItem ti[1];
	} ilq;

	void   SLAPI makeFldListQuery(BTBLID tblID, int numRecs);
	void   SLAPI makeIdxListQuery(BTBLID tblID, int numRecs);
	int    SLAPI getFieldList(BTBLID tblID, BNFieldList * fields);
	int    SLAPI getIndexList(BTBLID tblID, BNKeyList * pKeyList);
	SString & SLAPI GetTempFileName(SString & rFileName, long * pCounter, int forceInDataPath = 0);
	int    SLAPI ExtractStat(const XFile & rRec, DbTableStat * pStat) const;

	DBTable xfile;
	DBTable xfield;
	DBTable xindex;
	XFile  fileBuf;
	XField fieldBuf;
	XIndex indexBuf;
};
//
// Descr: Блок, управляющий хранением атрибутов базы данных, необходимых для правильной
//   идентификации самой базы данных и авторизации в ней.
//   Так как разные серверы баз данных могут требовать совершенно различных атрибутов,
//   равно как и приложение, использующее сервер может требовать множества дополнительных
//   атрибутов, этот блок построен так, что позволяет с одной стороны легко расширять набор
//   атрибутов, а с другой стороны обеспечить правильное их хранение (например, пароль хранится в
//   зашифрованном виде).
//
class DbLoginBlock : private SBaseBuffer {
public:
	enum {
		attrID = 1,         // (long) Идентификатор блока. Имеет смысл лишь в контексте DbLoginBlockArray (передается как строка)
		attrServerType,     // Тип сервера базы данных ("DEFAULT", "BTRIEVE", "ORACLE")
		attrDbFriendlyName, // Имя блока, понятное пользователю
		attrDbSymb,         // Символьное имя блока
		attrDbName,         // Имя блока, понятное серверу базы данных
		attrDictPath,       // Путь к словарю базы данных
		attrDbPath,         // Путь к файлам базы данных
		attrTempPath,       // Путь к каталогу временных файлоы
		attrDbUuid,         // (S_GUID) UUID базы данных (передается как строка в формате S_GUID::fmtIDL)
		attrUserName,       // Имя пользователя для регистрации в базе данных
		attrPassword        // Пароль для регистрации в базе данных (хранится в зашифрованном виде)
	};
	DbLoginBlock();
	DbLoginBlock(const DbLoginBlock & rS);
	~DbLoginBlock();
	DbLoginBlock & FASTCALL operator = (const DbLoginBlock & rS);
	void   Clear();
	int    FASTCALL Copy(const DbLoginBlock & rS);
	int    SetAttr(int attr, const char * pVal);
	int    GetAttr(int attr, SString & rVal) const;
private:
	LAssocArray Items;
	size_t End;
};

class DbLoginBlockArray : private TSCollection <DbLoginBlock> {
public:
	DbLoginBlockArray();
	void   Clear();
	//
	// Descr: Вставляет в массив новый блок, содержимое которого определяется по указателю pBlk.
	// Returns:
	//   1 - новый блок успешно вставлен
	//   2 - в массиве существует дубликат блока по символу и repalceDup == 0.
	//   3 - в массиве существует дубликат блока по идентификатору и replaceDup == 0.
	//   0 - ошибка. Ошибка может возникнуть в связи с отсутствием в блоке pBlk символа (attrDbSymb)
	//       либо из за недостатка памяти.
	//
	int    Add(long id, const DbLoginBlock * pBlk, int replaceDup);
	int    GetByID(long id, DbLoginBlock * pBlk) const;
	int    GetBySymb(const char * pSymb, DbLoginBlock * pBlk) const;
	uint   GetCount() const;
	int    GetByPos(uint pos, DbLoginBlock * pBlk) const;
	//
	// Descr: Извлекает атрибут attr элемента с символом pDbSymb.
	//
	int    GetAttr(const char * pDbSymb, int attr, SString & rVal) const;
	//
	// Descr: Извлекает атрибут attr элемента с идентификатором id.
	//
	int    GetAttr(long id, int attr, SString & rVal) const;
	long   SetSelection(long id);
	long   GetSelection() const { return SelId; }

	enum {
		loSkipNExDbPath   = 0x0001, // Пропускать элементы, путь attrDbPath в которых не существует либо пустой
		//
		// Приоритет использования сроки при создании списка - в порядке следования следующих флагов.
		//
		loUseFriendlyName = 0x0002,
		loUseDbSymb       = 0x0004,
		loUseDbPath       = 0x0008
	};

	int    MakeList(StrAssocArray * pList, long options) const;
protected:
	long   SelId;
private:
	long   LastId;
};
//
// Descr: Базовый класс для реализации взаимодействия с сервером баз данных.
//
class DbProvider {
public:
	//
	// Descr: Опции возможностей провайдера (Capability)
	//
	enum {
		cSQL        = 0x0001, // Предоставляет SQL-доступ к данным
		cDbDependTa = 0x0002  // Применять транзакции для всех операций изменения данных (для Oracle да, для Btrieve - нет).
	};
	//
	// Descr: Флаги состояния объекта.
	//   Поле состояния возвращается функцией GetState()
	//
	enum {
		stError    = 0x0001,
		stLoggedIn = 0x0002
	};
	virtual SLAPI ~DbProvider();
	//
	// Descr: Опции состояния базы данных, возвращаемые функцией GetDatabaseState()
	//
	enum {
		dbstNormal     = 0x0000,
		dbstContinuous = 0x0001  // Одна или более таблиц базы данных находится в состоянии continuous-открытия (для резервного копирования)
	};
	//
	// Descr: Возвращает флаги состояния базы данных по указателю pStateFlags.
	//   Флаги могут иметь значения DbProvider::dbstXXX (see enum above).
	// Returns:
	//   >0 - состояние базы данных успешно идентифицировано и флаги присвоены по указателю pStateFlags
	//   <0 - функция не поддерживается
	//    0 - ошибка
	//
	virtual int SLAPI GetDatabaseState(uint * pStateFlags);
	virtual SString & SLAPI MakeFileName_(const char * pTblName, SString & rBuf) = 0;
	virtual int SLAPI IsFileExists_(const char * pFileName) = 0;
	virtual SString & SLAPI GetTempFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath) = 0;
	//
	// Descr: Создает файл данных с именем pFileName по спецификации pTbl.
	//
	virtual int SLAPI CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode) = 0;
	virtual int SLAPI DropFile(const char * pFileName) = 0;
	virtual int SLAPI Login(const DbLoginBlock * pBlk, long options);
	virtual int SLAPI Logout();
	virtual int SLAPI PostProcessAfterUndump(DBTable * pTbl);
	virtual int SLAPI StartTransaction() = 0;
	virtual int SLAPI CommitWork() = 0;
	virtual int SLAPI RollbackWork() = 0;
	virtual int SLAPI GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat) = 0;
	//
	// Descr: Реализует механизм открытия таблицы базы данных с именем pFileName.
	// ARG(pTbl      IN): Указатель на экземпляр открываемой таблицы
	// ARG(pFileName IN): Текстовое имя таблицы. Правила именования таблиц сильно варьируются в зависимости
	//   от провайдера базы данных.
	// ARG(openMode  IN): Параметр режима открытия таблицы. omXXX (see above). В общем случае затруднительно
	//   гарантировать, что все провайдеры DB реализуют заданный режим. Должно документироваться для каждого
	//   конкретного порожденного класса.
	// ARG(pPassword IN): Пароль на случай, если таблица зашифрована.
	// Returns:
	//   >0 - таблица открыта успешно
	//    0 - ошибка
	//
	virtual int SLAPI Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword) = 0;
	virtual int SLAPI Implement_Close(DBTable * pTbl) = 0;
	virtual int SLAPI Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf) = 0;
	virtual int SLAPI Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData) = 0;
	virtual int SLAPI Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc) = 0;
	virtual int SLAPI Implement_DeleteRec(DBTable * pTbl) = 0;
	virtual int SLAPI Implement_BExtInsert(BExtInsert * pBei) = 0;
	virtual int SLAPI Implement_GetPosition(DBTable * pTbl, DBRowId * pPos);
	virtual int SLAPI Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ);
	virtual int SLAPI ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection) = 0;
	virtual int SLAPI RecoverTable(BTBLID tblID, BRecoverParam * pParam);
	//
	// Виртуальные методы, реализующие работу с SQL-сервером
	//
	virtual int CreateStmt(SSqlStmt * pS, const char * pText, long flags);
	virtual int DestroyStmt(SSqlStmt * pS);
	virtual int Binding(SSqlStmt & rS, int dir);
	//
	// Descr: Реализует функционал связывания переменных, специфичный для данной СУБД.
	// ARG(action IN):
	//   0 - инициализация структуры SSqlStmt::Bind
	//   1 - извлечение данных из внешнего источника во внутренние буферы
	//   <0 - перенос данных из внутренних буферов во внешний источник
	//       Так как техника привзяки может отличаться для разных режимов, то предусмотрены
	//       следующие варианты:
	//       -1 - перенос для запроса извлечения данных
	//       -2 - перенос для запроса изменения данных
	//   1000 - разрушение специфичных для SQL-сервера элементов структуры SSqlStmt::Bind
	// ARG(count  IN): action(0, 1000) - количество записей, action(1, -1) - номер записи [0..NumRecs-1]
	// ARG(pStmt  IN): @#{vptr} указатель на экземпляр SQL-оператора
	// ARG(pBind  IN): @#{vptr} указатель на структуру SSqlStmt::Bind
	//
	virtual int ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind);
	virtual int Exec(SSqlStmt & rS, uint count, int mode);
	virtual int Describe(SSqlStmt & rS, SdRecord &);
	virtual int Fetch(SSqlStmt & rS, uint count, uint * pActualCount);

	int    SLAPI IsValid() const;
	long   SLAPI GetState() const { return State; }
	long   SLAPI GetCapability() const { return Capability; }
	int    SLAPI LoadTableSpec(DBTable * pTbl, const char * pTblName, const char * pFileName, int createIfNExists);
	int    SLAPI RenewFile(DBTable & rTbl, int createMode, const char * pAltCode);
	int    SLAPI DropTable(const char * pTblName, int inDictOnly);
	//
	// Descr: Создает таблицу и файл данных по спецификации **ppTblSpec.
	//   При успешном завершении функции разрушает экземпляр *ppTblSpec и создает новый
	//   присваивая его по указателю ppTblSpec.
	// ARG(ppTblSpec IN/OUT): Двойной указатель, на входе используемый как спецификация таблицы
	//   с предустановленными верными значениями DBTable::tableName и DBTable::fileName.
	//   Значения ppTblSpec и *ppTblSpec, подаваемые как аргумент не должны быть нулевыми.
	//
	int    SLAPI CreateTableAndFileBySpec(DBTable ** ppTblSpec);
	int    SLAPI CreateTempFile(const char * pTblName, SString & rFileNameBuf, int forceInDataPath);
	//
	// Descr: удаляет все временные файлы, которые не
	//   открыты в текущем объекте BDictionary. Для удаления файлов
	//   используется функция DOS remove(); Если файл прихвачен другим
	//   приложением или пользователем, то он просто не удаляется и все.
	//
	void   SLAPI RemoveTempFiles();
	//
	// Descr: Добавляет в коллекцию имен временных файлов имя файла pFileName.
	//   При разрушении экземпляра словаря все файлы, имена которых находятся в
	//   этой коллекции, удаляются.
	//
	int    SLAPI AddTempFileName(const char * pFileName);
	//
	// Descr: Если имя файла pFileName находится в списке на удаление, то убирает его из этого списка.
	//
	int    SLAPI DelTempFileName(const char * pFileName);
	//
	// Descr: Устанавливает имя и символ открытой базы данных.
	//   Смысл этих имен транслируется приложением, использующим класс DbProvider
	//
	int    SLAPI SetDbName(const char * pName, const char * pSymb);
	int    SLAPI GetDbName(SString & rBuf) const;
	int    SLAPI GetDbSymb(SString & rBuf) const;
	//
	// Descr: Возвращает идентификатор пути к базе данных. Этот идентификатор
	//   используется для дифференциации потоков выполнения (в многопоточном процессе) по
	//   признаку соединения с той или иной базой данных.
	//   Основное высокоуровневое назначение - дифференциация кэшей баз данных.
	//
	long   SLAPI GetDbPathID() const { return DbPathID; }
	int    SLAPI GetDataPath(SString & rBuf) const;
	int    SLAPI GetSysPath(SString & rBuf) const;
	int    SLAPI GetDbUUID(S_GUID * pUuid) const;
	int    SLAPI SetupProtectData(const char * pOldPw, const char * pNewPw);
	int    SLAPI CreateTableSpec(DBTable *);
	int    SLAPI GetTableID(const char * pTblName, long * pID, DbTableStat * pStat);
	int    SLAPI GetTableInfo(long tblID, DbTableStat * pStat);
	int    SLAPI GetListOfTables(long options, StrAssocArray * pList);
	int    SLAPI GetUniqueTableName(const char * pPrefix, DBTable *);
protected:
	//
	// ARG(pDict IN): Передается в собственность экземпляру DbProvider. Т.е. деструктор
	//   этого класса разрушит объект по адресу pDict.
	//
	SLAPI  DbProvider(DbDictionary * pDict, long capability);
	int    SLAPI GetProtectData();
	int    SLAPI GetProtectData(FILE * f, uint16 * buf);
	void   SLAPI Common_Login(const DbLoginBlock * pBlk);
	void   SLAPI Common_Logout();

	long   State;
	long   DbPathID; // Идентификатор каталога базы данных. Ссылается на таблицу DbSession::DbPathList
	long   Capability;
	DbLoginBlock Lb;
	S_GUID DbUUID;
	DbDictionary * P_Dict;
	SString Path;
	SString DataPath;
	SString DbName;
	SString DbSymb;
	SString TempPath;
	StringSet TempFileList;
};
//
// Descr: класс базы данных. хранить описание словаря базы данных и пути доступа к данным.
//   Реализует ряд общих методов для работы с таблицами данных.
//
class BDictionary : public DbProvider {
public:
	static const char * DdfTableFileName;
	static const char * DdfFieldFileName;
	static const char * DdfIndexFileName;

	static BDictionary * SLAPI CreateBtrDictInstance(const char * pPath);
	SLAPI  BDictionary(const char * pPath, const char * pDataPath = 0, const char * pTempPath = 0);
	SLAPI ~BDictionary();
	virtual int SLAPI GetDatabaseState(uint * pStateFlags);
	virtual SString & SLAPI MakeFileName_(const char * pTblName, SString & rBuf);
	virtual int SLAPI IsFileExists_(const char * pFileName);
	virtual SString & SLAPI GetTempFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath);
	virtual int SLAPI CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode);
	virtual int SLAPI DropFile(const char * pFileName);
	virtual int SLAPI GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat);
	virtual int SLAPI Login(const DbLoginBlock * pBlk, long options);
	virtual int SLAPI Logout();
	virtual int SLAPI StartTransaction();
	virtual int SLAPI CommitWork();
	virtual int SLAPI RollbackWork();
	virtual int SLAPI Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword);
	virtual int SLAPI Implement_Close(DBTable * pTbl);
	virtual int SLAPI Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf);
	virtual int SLAPI Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData);
	virtual int SLAPI Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc);
	virtual int SLAPI Implement_DeleteRec(DBTable * pTbl);
	virtual int SLAPI Implement_BExtInsert(BExtInsert * pBei);
	virtual int SLAPI Implement_GetPosition(DBTable * pTbl, DBRowId * pPos);
	virtual int SLAPI ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection);
	virtual int SLAPI RecoverTable(BTBLID, BRecoverParam *);
private:
	SLAPI  BDictionary(int btrDict, const char * pPath);
	int    SLAPI Init(const char * pDataPath, const char * pTempPath);
};

#define FILE_REDIRECT "redirect.rtc"
//
// Database backup classes
//
#define BCOPYDF_USECOMPRESS 0x00000001
#define BCOPYDF_USECOPYCONT 0x00000002
#define BCOPYDF_RELEASECONT 0x00000004 // Форсированное освобождение файлов из режима CopyContinuous

struct BCopyData {
	SLAPI  BCopyData();
	BCopyData & SLAPI Z();
	long   ID;         // IN/OUT   ID of copy
	long   BssFactor;  // IN       Backup safety space factor
	long   Flags;      // IN       0x01 - use compression
	LDATETIME Dtm;     // OUT      Time of backuping
	int64  SrcSize;    // OUT      Size of database (bytes)
	int64  DestSize;   // OUT      Size of backup copy (bytes)
	ulong  CheckSum;   // OUT      Check sum for all copy
	SString Set;       // IN       Name of backup set
	SString CopyPath;  // IN       Path to backup set
	SString TempPath;  // IN       Temp path for compressing
	SString SubDir;    // OUT      Subdir to copy
};

class BCopySet : public TSCollection <BCopyData> {
public:
	enum Order {
		ordNone = 0,
		ordByDate = 1,
		ordByDateDesc = 2
	};
	explicit SLAPI BCopySet(const char * pName);
	int    SLAPI Sort(Order);
	SString Name;
};
//
// Event codes for callback function (log)
//
#define BACKUPLOG_BEGIN              1
#define BACKUPLOG_SUC_COPY           2
#define BACKUPLOG_ERR_COPY           3
#define BACKUPLOG_SUC_RESTORE        4
#define BACKUPLOG_ERR_RESTORE        5
#define BACKUPLOG_END                6
#define BACKUPLOG_ERR_GETFILEPARAM   7 // Ошибка вызова функции DBBackup::GetFileParam
#define BACKUPLOG_ERR_COMPRESS       8 // Ошибка упаковки файла
#define BACKUPLOG_ERR_DECOMPRESS     9 // Ошибка распаковки файла
#define BACKUPLOG_SUC_REMOVE        10 // Удаление резервной копии
#define BACKUPLOG_ERROR             11 // Общая ошибка
#define BACKUPLOG_ERR_DECOMPRESSCRC 12 // Ошибка распаковки файла (неверный CRC)
//
// callback function ptr
//
typedef int (*BackupLogFunc)(int, const char *, long initParam);
//
//
//
class TablePartsEnum {
public:
	explicit SLAPI TablePartsEnum(const char * pPath);
	int    SLAPI Init(const char * pPath);
	int    SLAPI Next(SString & rPath, int * pFirst = 0);
	int    SLAPI ReplaceExt(int first, const SString & rIn, SString & rOut);
private:
	SString Dir;
	SString MainPart;
	StrAssocArray List; // @*TablePartsEnum::Init Список файлов (с путями), соответствующих шаблону поиска
};

class DBTablePartitionList {
public:
	enum {
		oBackupPath = 0x0001
	};
	enum {
		fMain = 0x0001,
		fExt  = 0x0002,
		fCon  = 0x0004,  // continuous operation partition
		fBu   = 0x0008,  // backup copy of partition
		fZip  = 0x0010   // compressed backup copy of partition
	};
	struct Entry {
		long   Id;
		long   Flags;
		SString Path;
	};
	DBTablePartitionList();
	DBTablePartitionList(const char * pPath, const char * pName, long options);
	int    Init(const char * pPath, const char * pName, long options);
	uint   GetCount() const;
	int    GetInitPath(SString & rBuf) const;
	int    Get(uint p, Entry & rEntry) const;
	int    GetMainEntry(Entry & rEntry) const;
	int    GetMainBuEntry(Entry & rEntry) const;
	int    GetConEntry(Entry & rEntry) const;
private:
	int    Helper_GetEntry(long andF, long notF, Entry & rEntry) const;

	struct _InnerEntry { // @flat
		long   Id;
		long   Flags;
		uint   P;
	};
	uint   InitPathP; // Позиция в Pool пути, заданного при инициализации.
	uint   InitNameP; // Позиция в Pool имени файла без расширения //
	TSVector <_InnerEntry> List; // @v9.8.4 TSArray-->TSVector
	StringSet Pool;
};
//
//
//
class DBBackup {
public:
	SLAPI  DBBackup();
	SLAPI ~DBBackup();
	int    SLAPI SetDictionary(DbProvider * pDb);
	int    SLAPI Backup(BCopyData *, BackupLogFunc, long initParam);
	int    SLAPI Restore(const BCopyData *, BackupLogFunc, long initParam);
	int    SLAPI RemoveCopy(const BCopyData *, BackupLogFunc, long initParam);
	int    SLAPI GetCopySet(BCopySet *);
	int    SLAPI GetCopyData(long copyID, BCopyData *);
	uint   SLAPI GetSpaceSafetyFactor();
	void   SLAPI SetSpaceSafetyFactor(uint);
protected:
	//
	// Function CBP_CopyProgress must return one of SPRGRS_XXX value
	// (see file SLIB.H for explain that constants)
	//
	virtual int SLAPI CBP_CopyProcess(const char * pSrcFile, const char * pDestFile,
		int64 totalSize, int64 fileSize, int64 totalBytesReady, int64 fileBytesReady);
	DbProvider * P_Db;
	uint   SpaceSafetyFactor; // space_needed = actual * SpaceSafetyFactor / 1000
	int    AbortProcessFlag;
private:
	class InfoFile {
	public:
		explicit SLAPI InfoFile(DbProvider *);
		SLAPI ~InfoFile();
		int    SLAPI ReadSet(BCopySet *);
		int    SLAPI ReadItem(long copyID, BCopyData *);
		int    SLAPI AddRecord(BCopyData *);
		int    SLAPI RemoveRecord(const char * pSet, long id);
	private:
		int    SLAPI OpenStream(int readOnly);
		void   SLAPI CloseStream();
		int    SLAPI MakeFileName(const char *, const char *, char *, size_t);
		int    SLAPI WriteRecord(FILE *, const BCopyData *);
		int    SLAPI ReadRecord(FILE *, BCopyData *);
		char   FileName[MAXPATH];
		FILE * Stream;
	};
	struct CopyParams {
		SString Path;
		SString TempPath;
		//SStrCollection FileList;
		StringSet SsFiles;
		int64  TotalSize;
		ulong  CheckSum;
	};
private:
	int    SLAPI WriteCopyData(FILE *, BCopyData *);
	int    SLAPI ReadCopyData(FILE *, BCopyData *);
	int    SLAPI MakeCopyPath(BCopyData * data, SString & rDestPath);
	int    SLAPI CheckAvailableDiskSpace(const char *, int64 sizeNeeded);
	int    SLAPI GetCopyParams(const BCopyData *, DBBackup::CopyParams *);
	int    SLAPI DoCopy(DBBackup::CopyParams *, long, BackupLogFunc, long initParam);
	int    SLAPI CopyByRedirect(const char * pDBPath, BackupLogFunc fnLog, long initParam);
	int    SLAPI RemoveDatabase(int safe);
	int    SLAPI RestoreRemovedDB(int restoreFiles);
	static int   CopyProgressProc(const SDataMoveProgressInfo *);
	int    SLAPI CheckCopy(const BCopyData * pData, const CopyParams & rCP, BackupLogFunc fnLog, long initParam);
	int    SLAPI CopyLinkFiles(const char * pSrcPath, const char * pDestPath, BackupLogFunc fnLog, long initParam);

	int64  TotalCopySize;
	int64  TotalCopyReady;
	DBBackup::InfoFile * InfoF;
};
//
// Descr: Генератор SQL-кода
//
class Generator_SQL {
public:
	enum {
		tokCreate = 0,
		tokAlter,
		tokDrop,
		tokDatabase,
		tokTable,
		tokIndex,
		tokConstr,    // CONSTRAINT
		tokSequence,
		tokInsert,
		tokUpdate,
		tokDelete,
		tokSelect,
		tokInto,
		tokWhere,
		tokFrom,
		tokUnique,
		tokOn,
		tokAnd,
		tokOr,
		tokDesc,
		tokValues,
		tokHintIndexAsc,
		tokHintIndexDesc,
		tokFor,
		tokRowId,
		tokSet,
		tokReturning,
		tokMax,      // function max
		tokNlsLower, // function nls_lower
		tokCountOfTokens
	};
	enum {
		pfxIndex = 1,
		pfxSequence,
		pfxConstr
	};
	enum {
		fIndent = 0x0001
	};
	static const char * FASTCALL GetToken(uint tok);
	static SString & FASTCALL PrefixName(const char * pName, int prefix, SString & rBuf, int cat = 0);
	Generator_SQL(SqlServerType sqlst, long flags);
	operator SString & () { return Buf; }
	int    CreateTable(const DBTable & rTbl, const char * pFileName, int indent = 1);
	int    CreateIndex(const DBTable & rTbl, const char * pFileName, uint n);
	int    GetIndexName(const DBTable & rTbl, uint n, SString & rBuf);
	int    CreateSequenceOnField(const DBTable & rTbl, const char * pFileName, uint fldN, long newVal);
	int    GetSequenceNameOnField(const DBTable & rTbl, uint fldN, SString & rBuf);
	Generator_SQL & Reset();
	Generator_SQL & Cr();
	Generator_SQL & FASTCALL Tok(int);
	Generator_SQL & Tab();
	Generator_SQL & LPar();
	Generator_SQL & RPar();
	Generator_SQL & Sp();    // ' '
	Generator_SQL & Com();   // ','
	Generator_SQL & Dot();   // '.'
	Generator_SQL & Aster(); // '*'
	Generator_SQL & Eos();   // ';' '\n'
	//
	// Descr: Заносит в конец буфера один из символов, определенных константами _XXX_.
	//   На текущий момент поддерживаются следующие символы:
	//   _GT_, _GE_, _LT_, _LE_, _EQ_, _NE_
	//
	Generator_SQL & FASTCALL _Symb(int s);
	Generator_SQL & FASTCALL Text(const char * pName);
	Generator_SQL & FASTCALL QText(const char * pName);
	Generator_SQL & FASTCALL Param(const char * pParam);
	Generator_SQL & FASTCALL Select(const BNFieldList * pFldList);
	Generator_SQL & From(const char * pTable, const char * pAlias = 0);
	Generator_SQL & Eq(const char * pFldName, const char * pVal);
	Generator_SQL & Eq(const char * pFldName, long val);
	Generator_SQL & Func(int tok, const char * pArg);
	Generator_SQL & HintBegin();
	Generator_SQL & HintEnd();
	Generator_SQL & HintIndex(const DBTable & rTbl, const char * pAlias, uint idxN, int desc);
	SString & GetType(TYPEID typ, SString & rBuf);
private:
	static const char * P_Tokens[];
	SqlServerType Sqlst;
	long   Flags;
	SString Buf;
	SString Temp;
};
//
// Descr: Провайдер доступа к СУБД Oracle
//
#define ORAERR_NFOUND 1403

class SOraDbProvider : public DbProvider, private Ocif {
public:
	//friend class SSqlStmt;
	SOraDbProvider(const char * pDataPath);
	virtual ~SOraDbProvider();
	virtual SString & SLAPI MakeFileName_(const char * pTblName, SString & rBuf);
	virtual int SLAPI IsFileExists_(const char * pFileName);
	virtual SString & SLAPI GetTempFileName(SString & rFileNameBuf, long * pStart, int forceInDataPath);
	virtual int SLAPI CreateDataFile(const DBTable * pTbl, const char * pFileName, int createMode, const char * pAltCode);
	virtual int SLAPI DropFile(const char * pFileName);
	virtual int SLAPI Login(const DbLoginBlock * pBlk, long options);
	virtual int SLAPI Logout();
	virtual int SLAPI PostProcessAfterUndump(const DBTable * pTbl);
	virtual int SLAPI StartTransaction();
	virtual int SLAPI CommitWork();
	virtual int SLAPI RollbackWork();
	virtual int SLAPI GetFileStat(DBTable * pTbl, long reqItems, DbTableStat * pStat);
	virtual int SLAPI Implement_Open(DBTable * pTbl, const char * pFileName, int openMode, char * pPassword);
	virtual int SLAPI Implement_Close(DBTable * pTbl);
	virtual int SLAPI Implement_Search(DBTable * pTbl, int idx, void * pKey, int srchMode, long sf);
	virtual int SLAPI Implement_InsertRec(DBTable * pTbl, int idx, void * pKeyBuf, const void * pData);
	virtual int SLAPI Implement_UpdateRec(DBTable * pTbl, const void * pDataBuf, int ncc);
	virtual int SLAPI Implement_DeleteRec(DBTable * pTbl);
	virtual int SLAPI Implement_BExtInsert(BExtInsert * pBei);
	virtual int SLAPI Implement_DeleteFrom(DBTable * pTbl, int useTa, DBQ & rQ);
	virtual int SLAPI ProtectTable(long dbTableID, char * pResetOwnrName, char * pSetOwnrName, int clearProtection);
	virtual int CreateStmt(SSqlStmt * pS, const char * pText, long flags);
	virtual int DestroyStmt(SSqlStmt * pS);
	virtual int Binding(SSqlStmt & rS, int dir);
	//
	// Descr: Реализует функционал связывания переменных, специфичный для данной СУБД.
	// ARG(action IN):
	//   0 - инициализация структуры SSqlStmt::Bind
	//   1 - извлечение данных из внешнего источника во внутренние буферы
	//   -1 - перенос данных из внутренних буферов во внешний источник
	//   1000 - разрушение специфичных для SQL-сервера элементов структуры SSqlStmt::Bind
	// ARG(count  IN): action(0, 1000) - количество записей, action(1, -1) - номер записи [0..NumRecs-1]
	// ARG(pStmt  IN): @#{vptr} указатель на экземпляр SQL-оператора
	// ARG(pBind  IN): @#{vptr} указатель на структуру SSqlStmt::Bind
	//
	virtual int ProcessBinding(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind);
	virtual int Exec(SSqlStmt & rS, uint count, int mode);
	virtual int Describe(SSqlStmt & rS, SdRecord &);
	virtual int Fetch(SSqlStmt & rS, uint count, uint * pActualCount);
	int    GetAutolongVal(const DBTable & rTbl, uint fldN, long * pVal); // v
	int    Insert(const BExtInsert *);
	int    SelectByKey(DBTable & rTbl, int idx, void * pKey, int sp, int forUpdate);
	int    GetDirect(DBTable & rTbl, const DBRowId & rPos, int forUpdate);
private:
	struct OH {
		OH() : H(0), T(0)
		{
		}
		int    Valid() const { return (T != 0); }
		int    operator !() const { return (T == 0); }
		void   Clear()
		{
			H = 0;
			T = 0;
		}
		operator uint32 () const;
		operator void * () const { return H; }
		operator OCIEnv * () const { return Env; }
		operator OCIError * () const { return Err; }
		operator OCIServer * () const { return Svr; }
		operator OCISvcCtx * () const { return Srvc; }
		operator OCISession * () const { return Sess; }
		operator OCIStmt * () const { return Stmt; }
		operator OCIBind * () const { return Bind; }
		operator OCITrans * () const { return Trans; }
		union {
			void * H;     // Сам манипулятор
			OCIEnv    * Env;   // OCI_HTYPE_ENV
			OCIError  * Err;   // OCI_HTYPE_ERROR
			OCIServer * Svr;   // OCI_HTYPE_SERVER
			OCISvcCtx * Srvc;  // OCI_HTYPE_SVCCTX
			OCISession * Sess; // OCI_HTYPE_SESSION
			OCIStmt   * Stmt;  // OCI_HTYPE_STMT
			OCIBind   * Bind;  // OCI_HTYPE_BIND
			OCITrans  * Trans; // OCI_HTYPE_TRANS
		};
		uint32 T;              // Тип манипулятора (OCI_HTYPE_XXX)
	};
	struct OD { // @#size=8
		OD() : H(0), T(0)
		{
		}
		int    operator !() const { return (T == 0); }
		operator OCIDateTime * () const { return static_cast<OCIDateTime *>(H); }    // OCI_DTYPE_TIMESTAMP
		operator OCIRowid * () const { return static_cast<OCIRowid*>(H); }           // OCI_DTYPE_ROWID
		operator OCILobLocator * () const { return static_cast<OCILobLocator*>(H); } // OCI_DTYPE_LOB
		void * H;
		uint32 T;
	};
	struct RAW {
		RAW() : P(0)
		{
		}
		OCIRaw * P;
	};
	struct XID {
		long   FormatID;
		long   GtrIdLen;
		long   BQualLen;
		char   Data[128];
	};

	int    LobRead(OD & rLob, TYPEID typ, SLob * pBuf, size_t * pActualSize); // v
	int    LobWrite(OD & rLob, TYPEID typ, SLob * pBuf, size_t dataLen);      // v
	SOraDbProvider::OH FASTCALL OhAlloc(int type);
	int    FASTCALL OhFree(OH & rO);
	SOraDbProvider::OD FASTCALL OdAlloc(int type);
	int    FASTCALL OdFree(OD & rO);
	int    OhAttrSet(OH o, uint attr, void * pData, size_t size);
	int    OdAttrSet(OD o, uint attr, void * pData, size_t size);
	int    OhAttrGet(OH o, uint attr, void * pData, size_t * pSize);
	int    OhAttrGet(OH o, uint attr, SString & rBuf);
	void   Helper_SetErr(int errCode, const char * pErrStr);
	int    FASTCALL ProcessError(int retCode);
	LDATE  FASTCALL GetDate(OCIDateTime *);
	LTIME  FASTCALL GetTime(OCIDateTime *);
	LDATETIME FASTCALL GetDateTime(OCIDateTime *);
	int    SetDate(OCIDateTime *, LDATE);
	int    SetTime(OCIDateTime *, LTIME);
	int    SetDateTime(OCIDateTime *, LDATETIME);
	int    RowidToStr(OD rowid, SString & rBuf);
	size_t FASTCALL RawGetSize(const RAW & r);
	int    RawResize(RAW & r, size_t newSize);
	int    RawCopyFrom(RAW & r, const void * pBuf, size_t dataSize);
	void * FASTCALL RawGetPtr(const RAW & r);
	int    FASTCALL DestroyBind(SSqlStmt::Bind * pBind); // V
	int    ProcessBinding_SimpleType(int action, uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType);
	int    ProcessBinding_AllocDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind, uint ntvType, int descrType);
	int    ProcessBinding_FreeDescr(uint count, SSqlStmt * pStmt, SSqlStmt::Bind * pBind);
	int    Helper_Fetch(DBTable * pTbl, DBTable::SelectStmt * pStmt, uint * pActual);
	static OH FASTCALL StmtHandle(const SSqlStmt & rS);
	int    SLAPI GetFileStat(const char * pFileName, long reqItems, DbTableStat * pStat);

	enum {
		fError   = 0x0001
	};
	long   Flags;
	OH     Env;
	OH     Err;
	OH     Svr;
	OH     Srvc;
	OH     Sess;
	int32  LastErrCode;
	SString LastErrMsg;
	StrAssocArray CrsfcNameList;
	Generator_SQL SqlGen;
};
//
//
//
DBTable * FASTCALL _GetTable(int handle); // Used DbSession

class DbThreadLocalArea {
public:
	//
	// Descr: Класс, регистрирующий открытые в данном потоке таблицы базы данных.
	//
	class DbRegList {
	public:
		DbRegList();
		void   Init();
		int    GetMaxEntries() const;
		int    FASTCALL AddEntry(void * pTbl, void * pSupplementPtr);
		int    FASTCALL FreeEntry(int handle, void * pSupplementPtr);
		void * FASTCALL GetPtr(int handle) const;
		void * FASTCALL GetBySupplementPtr(const void * pSupplementPtr) const;
	private:
		TSArray <void *> Tab;
		PtrHashTable Ht; // Индексная хэш-таблица
		int    OpenCount;
		int    OpenPeak;
	};

	enum {
		stTransaction = 0x0001
	};
	//
	SLAPI  DbThreadLocalArea();
	SLAPI ~DbThreadLocalArea();
	void   SLAPI Init();
	long   GetState() const { return State; }
	int    FASTCALL AddTableEntry(DBTable *);
	int    FASTCALL FreeTableEntry(int handle);
	uint   GetTabEntriesCount() const;
	DBTable * FASTCALL GetTableEntry(int handle) const { return static_cast<DBTable *>(DbTableReg.GetPtr(handle)); }
	DBTable * SLAPI GetCloneEntry(BTBLID) const;
	//
	// Descr: Временная функция (до завершения разработки интерфейса с BerkeleyDB).
	//   Возвращает ссылку на таблицу открытых BDB-файлов.
	//
	DbRegList & GetBDbRegList() { return BDbTableReg; }
	const DbRegList & GetBDbRegList_Const() const { return BDbTableReg; }
	void   FASTCALL InitErrFileName(const char * pFileName);
	const  char * SLAPI GetLastErrFileName() const;
	int    SLAPI StartTransaction();
	//
	// Descr: Запускает транзакцию в случае, если провайдер БД (P_CurDict)
	//   имеет установленный флаг Capability & cDbDependTa.
	//   Необходимость этой функции обусловлена следующими соображениями:
	//   существующий код проекта Papyrus имеет значительное число операций
	//   с временными таблицами, которые не требуют использования транзакций и
	//   не используют их.
	//   Однако, в случае, с Oracle такая практика приводит к ошибкам при запуске
	//   последующих транзакций. Дабы сохранить неизменной работу с Btrieve и
	//   избежать проблем с Oracle (возможно, и с другими СУБД) мы будем обрамлять
	//   такие участки кода вызовами пар:
	//   {
	//       StartTransaction_DbDepend();
	//       ...
	//       CommitWork();
	//   }
	// Returns:
	//   >0 - успешно осуществлен запуск транзакции
	//   <0 - транзакция не запущена по причине того, что провайдер БД не требует этого
	//   0  - ошибка
	//
	int    SLAPI StartTransaction_DbDepend();
	int    SLAPI CommitWork();
	int    SLAPI RollbackWork();
	//
	long   Id;
	int    LastBtrErr;
	int    LastDbErr;
	SString AddedMsgString;
	DbProvider * P_CurDict;
	char   ClientID[16];
private:
	DbRegList DbTableReg;
	DbRegList BDbTableReg;
	char * P_StFileName;   // Имя файла данных, с которым связана последняя ошибка
	long   State;
};

#define BTR_RECLOCKDISABLE -12345 // Значение DbSession::Config::NWLockTries означающее запрет на блокировку записей

class DbSession {
public:
	enum {
		fDetectExistByOpen = 0x0001 // Флаг, предписывающий идентифицировать факт
			// наличия файла посредством попытки открытия (вместо fileExists) - Btreive only
	};
	struct Config {
		long Flags;
		int  NWaitLockTries;       // Макс количество попыток заблокировать запись блокировкой без ожидания. Если <=0, то применяется блокировка с ожиданием
		int  NWaitLockTryTimeout;  // Таймаут (ms) между попытками заблокировать запись блокировкой без ожидания. Если <= 0, то применяется значение по умолчанию.
	};
	SLAPI  DbSession();
	SLAPI ~DbSession();
	int    IsConsistent() const;
	//void   SLAPI SetFlag(long f, int set);
	//long   SLAPI GetFlag(long f) const;
	void   SetConfig(const Config * pCfg);
	// @v10.0.0 void   FASTCALL GetConfig(Config & rCfg);
	const  Config & GetConfig() const { return Cfg; } // @v10.0.0
	int    SLAPI GetTaState();
	int    SLAPI InitThread();
	void   SLAPI ReleaseThread();
	DbThreadLocalArea & SLAPI GetTLA(); // { return *(DbThreadLocalArea *)TlsGetValue(TlsIdx); }
	const DbThreadLocalArea & SLAPI GetConstTLA() const; // { return *(PPThreadLocalArea *)TlsGetValue(TlsIdx); }
	//
	// Descr: Устанавливает код ошибки BtrError = errCode
	// Returns:
	//   0
	//
	int    FASTCALL SetError(int errCode);
	//
	// Descr: Устанавливает код ошибки BtrError = errCode и дополнительную строку сообщения.
	// Returns:
	//   0
	//
	int    FASTCALL SetError(int errCode, const char * pAddedMsg);
	int    SLAPI OpenDictionary2(DbProvider * pDb);
	int    SLAPI CloseDictionary();
	void   SLAPI GetProtectData(void * pBuf, int decr) const; // size of buffer must be at least 64 bytes
	void   SLAPI SetProtectData(const void * pBuf);
	void   FASTCALL SetAddedMsgString(const char *);
	const  SString & GetAddedMsgString() const;
	int    SLAPI GetDbPathID(const char * pPath, long * pID);
	int    SLAPI GetDbPath(long dbPathID, SString &) const;
	//
	// Descr: Возвращает ИД каталога базы данных текущего потока
	//
	long   SLAPI GetDbPathID() const;
private:
	void   SLAPI InitProtectData();

	//struct DbSessInit { DbSessInit() { SLS.SlSession::SlSession(); } } __Init;
	long   TlsIdx;       // Ид локальной области потока    //
	long   Id__;
	ACount LastThread;
	uint16 __dbdata__[32];
	uint32 _Oe;        // OCIEnv - идентификатор окружения для OCI (Oracle Call Interface)
	//
	Config Cfg; // @v10.0.0
	// @v10.0.0 long   Flags;
	// @v10.0.0 int    NWaitLockTries;       // Макс количество попыток заблокировать запись блокировкой без ожидания. Если <=0, то применяется блокировка с ожиданием
	// @v10.0.0 int    NWaitLockTryTimeout;  // Таймаут (ms) между попытками заблокировать запись блокировкой без ожидания. Если <= 0, то применяется значение по умолчанию.
	//
	SStrCollection DbPathList; // Список баз данных, на которые ссылаются открытые потоки.
};
//
// DSN Open Mode { PSQL\CATALOG.H
//
#define NORMAL_MODE         0
#define ACCELERATED_MODE    1
#define READONLY_MODE       2
#define EXCLUSIVE_MODE      3
// }
//
#define  P_MAX_NAME_SIZE         64    /* maximum size of common names */
#define  P_MAX_PATH_SIZE         1024  /* maximum size of a full path */
#define  P_MAX_COMM_PROTOCOLS    8     /* maximum communication protocols */

typedef int (*PT_PvConnectServer)(char * serverName, char * userName, char * password, long * phConnection);
typedef int (*PT_PvDisconnect)(long hConnection);
typedef int (*PT_PvStart)(long reserved);
typedef int (*PT_PvStop)(long * preserved);
typedef int (*PT_PvCreateDatabase)(long hConnection, char * dbName, char * dictPath, char * dataPath, ulong dbFlags);
typedef int (*PT_PvCreateDSN)(long hConnection, char * pdsnName, char * pdsnDesc, char * pdsnDBQ, long openMode);
typedef int (*PT_PvGetOpenFilesData)(long hConnection, ulong * pCount);
typedef int (*PT_PvGetFileHandlesData)(long hConnection, char * pFileName, ulong * pCount);
typedef int (*PT_PvFreeOpenFilesData)(long hConnection);
typedef int (*PT_PvGetOpenFileName)(long hConnection, ulong idx, ulong * pBufSize, char * pFileName);

struct PVDATETIME {
   ushort  year;           // Year (current year minus 1900)
   ushort  month;          // Month (0 – 11; January = 0)
   ushort  day;            // Day of month (1 – 31)
   ushort  hour;           // Hours since midnight (0 – 23)
   ushort  minute;         // Minutes after hour (0 – 59)
   ushort  second;         // Seconds after minute (0 – 59)
   ushort  millisecond;    // Milliseconds after minute (0 – 59000). Default to 0 if the smallest time unit is seconds.
};

struct PVFILEINFO {
	uchar  openMode;            /* open mode */
	uchar  locksFlag;           /* TRUE if locked */
	uchar  transFlag;           /* TRUE if in transaction mode */
	uchar  tTSFlag;             /*  */
	uchar  readOnly;            /* TRUE if opened for read-only access */
	uchar  continuousOpsFlag;   /*  */
	uchar  referentialIntgFlag; /*  */
	ulong  aFLIndex;            /*  */
	ulong  activeCursors;       /*  */
	ulong  pageSize;            /* page size in bytes */
	PVDATETIME openTimeStamp;  /* time when the file was open */
	uchar  Reserve;
};

struct PVFILEHDLINFO {
	ulong   clientIndex;         /*  */
	uchar   openMode;            /* open mode */
	uchar   locksFlag;           /* TRUE if the file is locked */
	uchar   waitFlag;            /* TRUE if in waiting mode */
	ushort  transState;          /* transaction state */
	char    userName[P_MAX_NAME_SIZE];    /* user who owns this handle */
};

typedef int (*PT_PvGetFileInfo)(long hConnection, char * pFileName, PVFILEINFO * pInfo);
typedef int (*PT_PvGetFileHandleInfo)(long hConnection, char * pFileName, PVFILEHDLINFO *);
typedef int (*PT_PvGetServerName)(long hConnection, ulong * pBufSize, char * pName);

struct DBFileInfo : PVFILEINFO {
	char FileName[MAXPATH];
	char UserName[64];
};

class PervasiveDBCatalog {
public:
	PervasiveDBCatalog();
	~PervasiveDBCatalog();
	int    IsValid() const;
	int    Connect(const char * pServerName, char * pUser_name, char * pPassword);
	int    Disconnect();
	int    CreateDB(const char * pEntryName, const char * pDict, const char * pData);
	int    ServernameFromFilename(const char * pFilename, SString & rServerName);
	int    GetOpenFilesData(TSArray <DBFileInfo> * pInfoList);
private:
	long   H_Connection;
	enum {
		sValid     = 0x0001,
		sConnected = 0x0002
	};
	long   State;
	static int PtLoad();
	static int PtRelease();
	static SDynLibrary * P_Lib;
	static PT_PvConnectServer      PvConnectServer;
	static PT_PvDisconnect         PvDisconnect;
	static PT_PvStart              PvStart;
	static PT_PvStop               PvStop;
	static PT_PvCreateDatabase     PvCreateDatabase;
	static PT_PvCreateDSN          PvCreateDSN;
	static PT_PvGetOpenFilesData   PvGetOpenFilesData;
	static PT_PvGetFileHandlesData PvGetFileHandlesData;
	static PT_PvFreeOpenFilesData  PvFreeOpenFilesData;
	static PT_PvGetOpenFileName    PvGetOpenFileName;
	static PT_PvGetFileInfo        PvGetFileInfo;
	static PT_PvGetFileHandleInfo  PvGetFileHandleInfo;
	static PT_PvGetServerName      PvGetServerName;
};

extern DbSession DBS;

#define BtrError  (DBS.GetTLA().LastBtrErr)
#define DBErrCode (DBS.GetTLA().LastDbErr)
#define CurDict   (DBS.GetTLA().P_CurDict)
//
// @v7.8.5 Спецификация параметра DATA_LEN везде заменена с int16 на uint16
//
#define BTRV BTRCALL
#if defined(__WIN32__) || defined(_WIN32)
	typedef int (__stdcall * BtrCallProc)(int OP, char * POS_BLK, char * DATA_BUF, uint16 * DATA_LEN, char * KEY_BUF, int KEY_LEN, int KEY_NUM);
	typedef int (__stdcall * BtrCallProcID)(int OP, char * POS_BLK, char * DATA_BUF, uint16 * DATA_LEN, char * KEY_BUF, int KEY_LEN, int KEY_NUM, void * pCliID);

	extern BtrCallProc   _BtrCall;
	extern BtrCallProcID _BtrCallID;

	//int SLAPI BTRCALL(int,char *,char *,int16 *,char *,int,int);
	FORCEINLINE int BTRCALL(int OP, char * POS_BLK, char * DATA_BUF, uint16 * DATA_LEN, char * KEY_BUF, int KEY_LEN, int KEY_NUM)
		{ return _BtrCallID(OP, POS_BLK, DATA_BUF, DATA_LEN, KEY_BUF, KEY_LEN, KEY_NUM, &DBS.GetTLA().ClientID); }
#elif defined(_Windows)
	int SLAPI BTRCALL(int,char *,char far*,uint16 far*,char far*,int,int);
#else
	int SLAPI BTRCALL(int, char *, char *, uint16 *, char *, int);
	int SLAPI BTRCALLID(int,char *,char *, uint16 *, char *, int, char *);
#endif
//
// DBQuery
//
#define MAX_DBQ_PARAMS 4

#define AGGR_BEGIN  1 // Перед первой записью
#define AGGR_NEXT   2 // Для каждой записи
#define AGGR_END    3 // Больше записей не будет
#define CALC_SIZE  10 // Вычислить длину результата
//
// params - массив параметров. Разрушается вызывающей функцией.
// Параметр option используется агрегатными функциями и принимает
// одно из значений {AGGR_BEGIN | AGGR_NEXT | AGGR_END}. При этом,
// если option == AGGR_BEGIN || option == AGGR_NEXT, параметр result
// используется по усмотрению функции (промежуточные результаты), при
// option == AGGR_END в параметр result должно быть занесено
// окончательное значение.
//
typedef void (SLAPI * DBQProc)(int option, DBConst * result, const DBConst * params);
#define IMPL_DBE_PROC(name) void SLAPI name(int option, DBConst * result, const DBConst * params)

enum DBFunc {
	dbq_error = 0,
	//
	// Conversion functions
	dbq_ltoa,
	dbq_enumtoa,
	dbq_flagtoa,
	//
	// Arithmetic functions
	dbq_add_ii,
	dbq_sub_ii,
	dbq_mul_ii,
	dbq_div_ii,
	dbq_rem_ii,
	dbq_add_rr,
	dbq_sub_rr,
	dbq_mul_rr,
	dbq_div_rr,
	//
	// Bitwise functions (long arguments)
	dbq_and,
	dbq_or,
	dbq_xor,
	dbq_not,
	//
	// String functions
	dbq_add_ss,
	dbq_ltrim,
	dbq_rtrim,
	dbq_left,
	dbq_right,
	dbq_strlen,
	dbq_lower,
	dbq_upper,
	dbq_contains, // Возвращает 1, если строка (arg0) содержит строку (arg1), в противном случае - 0 (case-sensitive)
	//
	// Date functions
	dbq_day,
	dbq_mon,
	dbq_year,
	dbq_weekday,
	dbq_add_di,
	dbq_sub_dd,
	dbq_add360a_di,
	dbq_add360e_di,
	dbq_sub360a_dd,
	dbq_sub360e_dd,
	//
	// Time functions
	dbq_hour,
	dbq_minute,
	dbq_millisecond,
	dbq_sub_tt,
	//
	// Aggregate functions
	dbq_count,
	dbq_sum_i,
	dbq_sum_r,
	dbq_avg,
	dbq_dev,
	dbq_min,
	dbq_max
};

struct DBFuncInfo {
	long   func;
	int    paramCount;
	int    paramTypes[MAX_DBQ_PARAMS];
	int    typ;
	int    aggr;           // Aggregate function
	DBQProc proc;
};

#define FIRST_DYN_DBQFUNC_ID 10001
#define LAST_DYN_DBQFUNC_ID  29999

class DbqFuncTab {
public:
	//
	// Descr: Динамически регистрирует новую неагрегатную функцию.
	//   Если *pFuncId == 0, то выполняет регистрацию и присваивает по этому указателю ИД новой функции.
	//   Если *pFuncId != 0, тогда считает, что функция уже зарегистрирована.
	//
	static int SLAPIV RegisterDyn(int * pFuncId, int retType, DBQProc pProc, int paramCount, ...); // @cs
	//
	// Descr: Тоже что и RegisterDyn(0, retType, pProc, paramCount,...) 
	//   Но возвращает идентификатор функции, а не сигнал успеха/неудачи.
	//
	static int SLAPIV RegisterDynR(int retType, DBQProc pProc, int paramCount, ...); // @cs
	//
	// Динамически регистрирует новую агрегатную функцию.
	// Если *pFuncId == 0, то выполняет регистрацию и присваивает по этому указателю ИД новой функции.
	// Если *pFuncId != 0, тогда считает, что функция уже зарегестрирована.
	//
	static int SLAPIV RegisterDynAggr(int * pFuncId, int retType, DBQProc pProc, int paramCount, ...); // @cs
	static DBFuncInfo * FASTCALL Get(int);
	SLAPI  DbqFuncTab();
	SLAPI ~DbqFuncTab();
private:
	int    SLAPI GetDynFuncId(int * pId);
	int    SLAPI _RegisterFunc(int funcId, int isAggr, int retType, DBQProc pProc, int paramCount, va_list);
	int    SLAPIV RegisterFunc(int funcId, int isAggr, int retType, DBQProc pProc, int paramCount, ...);
	int    SLAPI SearchFunc(int funcId, DBFuncInfo *);
	DBFuncInfo * FASTCALL GetFuncPtr(int funcId);
	int    SLAPI PreRegister();
	SArray * P_Tab;
};

#define Invalid_DBItem   0
#define DBQ_ID          -3
#define DBConst_ID      -2
#define DBE_ID          -1
#define DBField_ID       1

struct DBItem {
	//
	// id ==  DBQ_ID     DBQ
	// id ==  DBConst_ID DBConst
	// id ==  DBE_ID     DBE
	// id >   0          DBField (id == table handle)
	//
	void   SLAPI destroy();
	int    SLAPI baseType() const;
	TYPEID SLAPI stype() const;
	int    SLAPI typeOfItem() const;

	int    Id;
};

struct DBField : public DBItem {
	DBTable * SLAPI getTable() const;
	const BNField & SLAPI getField() const;
	int    FASTCALL getValue(void *, size_t * pSize) const;
	int    SLAPI putValue(const void *) const;
	void * SLAPI getValuePtr() const;
	// if !*k then getFirst index, else getNext index
	int    SLAPI getIndex(BNKey * pK, int * pKeyPos, int * pSeg);

	int    fld;
};

class DBFieldList {
public:
	explicit SLAPI DBFieldList(uint n = 0);
	SLAPI ~DBFieldList();
	DBFieldList & FASTCALL operator = (const DBFieldList &);
	void   SLAPI Destroy();
	int    SLAPI Search(const DBField & rFld, uint * pPos) const;
	int    FASTCALL Add(const DBField &);
	int    FASTCALL Add(const DBFieldList &);
	uint   SLAPI GetCount() const;
	const  DBField & FASTCALL Get(uint) const;
	const  BNField & FASTCALL GetField(uint) const;
	int    SLAPI GetValue(uint fldN, void * pBuf, size_t * pSize) const;
private:
	int    FASTCALL Alloc(uint n);
	uint   Count;
	DBField * P_Flds;
};

struct DBConst : public DBItem {
	void   FASTCALL init(long l);
	void   FASTCALL init(size_t l);
	void   FASTCALL init(double d);
	void   FASTCALL init(const char * s);
	void   FASTCALL init(LDATE d);
	void   FASTCALL init(LTIME t);
	void   FASTCALL init(LDATETIME t);
	void   FASTCALL init(const void * ptr);
	void   FASTCALL InitForeignStr(const char *); // @v9.9.3
	int    FASTCALL copy(const DBConst &);
	void   FASTCALL destroy();
	int    FASTCALL convert(TYPEID, void *) const;
	int    FASTCALL fromfld(DBField);
	char * FASTCALL tostring(long fmt, char * pBuf) const;

	enum {
		fNotOwner = 0x0001 // Экземпляр не владеет объектом, на который указывает sptr (или иной) в union
	};
	enum {
		lv   = BTS_INT,
		rv   = BTS_REAL,
		sp   = BTS_STRING,
		dv   = BTS_DATE,
		tv   = BTS_TIME,
		dtv  = BTS_DATETIME,
		ptrv = BTS_PTR,
		i64v = BTS_INT64_, // @v10.6.3
	};
	union {
		long   lval;
		int64  i64val; // @v10.6.3
		double rval;
		char * sptr;
		const  void * ptrval;  // Экземпляр ни при каких обстоятельствах не владеет объектом по этому указателю
		LDATE  dval;
		LTIME  tval;
		LDATETIME dtval;
	};
	int16  Flags;
	int16  Tag;
};

DBConst FASTCALL dbconst(long);
DBConst FASTCALL dbconst(int16);
DBConst FASTCALL dbconst(uint16);
DBConst FASTCALL dbconst(double);
DBConst FASTCALL dbconst(const char *);
DBConst FASTCALL dbconst(LDATE);
DBConst FASTCALL dbconst(LTIME);
DBConst FASTCALL dbconst(const void *);
//
// Эта функция может изменить объекты, на которые указывают передаваемые параметры
//
int FASTCALL compare(DBConst *, DBConst *);

struct DBE : public DBItem {
	int    Pointer;
	int    Count;
	int    DontDestroy;

	struct T {
		enum _tag {
			lv = BTS_INT,
			rv = BTS_REAL,
			sp = BTS_STRING,
			dv = BTS_DATE,
			tv = BTS_TIME,
			ptrv = BTS_PTR,
			i64v = BTS_INT64_, // @v10.6.3
			fld = 101,
			func = 102
		} tag;
		union {
			DBFunc  function;
			long    lval;
			int64   i64val; // @v10.6.3
			double  rval;
			char *  sptr;
			void *  ptrval;
			LDATE   dval;
			LTIME   tval;
			DBField field;
		};
	} * P_Terms;

	void   init();
	void   destroy();
	int    FASTCALL getTblHandle(int item);
	int    FASTCALL evaluate(int option, DBConst *);
	int    FASTCALL push(const DBE::T *);
	int    FASTCALL push(DBItem &);
	int    FASTCALL push(DBFunc);
	int    pop();
	int    FASTCALL call(int option, DBConst *);
};
//
// Битовые маски для поля DBQ::flags
//
#define DBQ_TRUE   0x0001
#define DBQ_FALSE  0x0002
#define DBQ_LOGIC  0x0003
#define DBQ_OUTER  0x0004

union DBDataCell {
	SLAPI  DBDataCell();
	SLAPI  DBDataCell(DBConst & r);
	SLAPI  DBDataCell(DBE & r);
	SLAPI  DBDataCell(DBField & r);
	int    SLAPI GetId() const { return I.Id; }
	int    FASTCALL containsTblRef(int tblID) const;
		// @<<DBQ::testForKey(int itm, int tblID, int * pIsDyn)
	int    FASTCALL getValue(TYPEID, void *);
	int    FASTCALL getValue(DBConst *);
	int    SLAPI toString(SString & rBuf, long options) const;
	int    SLAPI CreateSqlExpr(Generator_SQL * pGen) const;

	DBItem  I;
	DBConst C;
	DBE     E;
	DBField F;
};

struct DBQ {
	SLAPI  DBQ(DBItem &, int comp, DBItem &);
	SLAPI  DBQ(int logic, DBQ &, DBQ &);
	void   FASTCALL destroy(int withTree = 0);
	int    SLAPI testForKey(int itm, int tblID, int * pIsDyn);
		// @<<DBQ::getPotentialKey(int itm, int tblID, int segment, KR * kr)
	int    SLAPI getPotentialKey(int itm, int tblID, int segment, KR * pKr);
		// @<<DBTree::chooseKey(int n, int tblID, int seg, PKR dest, uint * pTrace)
	int    FASTCALL checkTerm(int);
	int    SLAPI CreateSqlExpr(Generator_SQL * pGen, int itm) const;

	DBTree * tree;
	uint   count;
	struct T {
		uchar  cmp;
		char   tblIdx; // Индекс таблицы в списке DBQuery::tbls для //
		// которой этот терм является связывающим. -1 означает то,
		// что терм не связывает ни одну из этих таблиц. Это поле
		// используется при разноске общего дерева ограничений по
		// таблицам и инициализируется функцией DBQuery::arrangeTerms().
		uint   flags;
		DBDataCell left, right;
	};
	T *  items;
};

#define NOKEY 0x0001

struct DBTree {
	explicit SLAPI DBTree(DBQ * pOwner = 0);
	void   FASTCALL init(DBQ * pOwner);
	int    SLAPI addNode(int link, int left, int right, int * pPos);
	int    SLAPI addLeaf(int term, int flags, int * pPos);
	int    SLAPI addTree(DBTree *, int p, int * pPos);
	int    SLAPI chooseKey(int node, int tblID, int seg, KR * dest, uint *);
	int    FASTCALL checkRestriction(int = -1);
	int    FASTCALL expand(int *);
	void   SLAPI destroy();
	int    SLAPI CreateSqlExpr(Generator_SQL * pGen, int node) const;

	DBQ  * P_Terms;
	int    Root;
	int    Count;
	struct T {
		uchar link;   // 0 - leaf, 1 - AND, 2 - OR
		uchar flags;
		union {
			int16 left;
			int16 term; // if link == 0 (leaf)
		};
		int16 right;
	} * P_Items;
};

struct __range {
	const void * low;
	const void * upp;
	int    ol; // Признак строгого сравнения (_LT_)
	int    ou; // Признак строгого сравнения (_GT_)
};
//
// Флаг SKIP_ONE_REC добавляется к параметру sp функцией KR::getKey() в случае, если одна запись
// должна быть пропущена. Такая ситуация возникает когда требуется взять последнюю запись
// с условием _EQ_ по неуникальному ключу.
//
#define SKIP_ONE_REC 0x8000
//
// Флаг ONLY_ONE_REC добавляется к параметру sp функцией KR::getKey() в случае,
// если задано условие _EQ_ по уникальному ключу
//
#define ONLY_ONE_REC 0x4000
//
// Флаг DYN_KEY идентифицирует динамический ключ, то есть тот, который необходимо пересматривать при каждой
// операции взятия первой записи, так как он зависит от одной из предыдущих таблиц соединения.
//
#define DYN_KEY      0x80

class KR { // Key Restriction
/*
	Fixed portion:
		KeyNumber         1
		PotentialSegment  1 ID of next potential segment
		KeyLen            2
		Length            2 Length of buffer in bytes
		NumItems          2
		Current           2 Offset from begining of buf
		TblHandle         2
	Repeating portion (NumItems times):
		LowBound          1   (_EQ_, _GT_, _GE_)
		UppBound          1   (_LT_, _LE_)
		Key value         KeyLen bytes
		Second Key value  KeyLen bytes (if LowBound != 0 && UppBound != 0)
*/
public:
	SLAPI  KR();
	SLAPI  KR(int, int);
	int    SLAPI init(int hdlTable, int keyPosition);
	int    SLAPI trim();
	int    SLAPI copy(KR, int = 0);
	void   SLAPI destroy();
	int    SLAPI operator !() const;
	int    FASTCALL add(const void *);
	int    SLAPI remove();
	uint   FASTCALL itemSize(const void *) const;
	int    FASTCALL walk(uint16 *) const;
	int    SLAPI first();
	int    SLAPI last();
	int    SLAPI operator++();
	int    SLAPI operator--();
	void   SLAPI makeRange(const void *, __range &) const;
	void   SLAPI rangeToBuf(const __range &, void *, int);
	int    SLAPI disjunction(void *);                         // OR
	int    SLAPI conjunction(__range & /*dest*/, const __range &, int *, int *);
	int    SLAPI conjunction(void *, int assign, int *, int *); // AND
	int    SLAPI link(int logic, KR);
	int    SLAPI rating();
	int    SLAPI getKey(void *, int * /* spXXX constant */);
	int    FASTCALL checkKey(const void *) const;
	struct H {
		uint8  keyNum;
		uint8  pseg; // Флаг DYN_KEY является признаком динамического ключа
		uint16 keyLen;
		int16  len;
		int16  count;
		uint16 current;
		uint16 hdlTable;
	};
	struct I {
		uint8 low;
		uint8 upp;
	};
	union {
		H    * P_h;
		char * P_b;
	};
};

int FASTCALL compare(KR, KR);

DBQuery & FASTCALL selectbycell(int count, const DBDataCell *);
DBQuery & FASTCALL select(const DBFieldList &);
DBQuery & SLAPIV select(DBField,...);
DBQuery & SLAPI  selectAll();

#define DBQTF_OUTER_JOIN         0x0004 // По таблице определен OUTER JOIN
#define DBQTF_OVRRD_DESTROY_TAG  0x0008 // Не разрушать таблицу при разрушении запроса
#define DBQTF_FIXED_INDEX        0x0010 //
#define DBQTF_LASTSRCHFAILED     0x0020 // @2003.11.09 Последняя попытка поиска
	// в таблице завершилась неудачно. Это означает, что не следует искать
	// запись по условию spNext или spPrev
#define DBQTF_SEARCHFORUPDATE    0x0040 // Для таблицы в запросе применять функцию
	// DBTable::searchForUpdate вместо DBTable::search
//
// Следующие два флага взаимоисключающие. Необходимы для переключения SQL-запросов
// при смене направления выборки.
//
#define DBQTF_PREVDOWN           0x0080 // Предыдущая операция выборки была "DOWN"
#define DBQTF_PREVUP             0x0100 // Предыдущая операция выборки была "UP"

class DBQuery {
	friend DBQuery & SLAPIV select(DBField, ...);
	friend DBQuery & SLAPI  selectAll();
public:
	enum {
		smart_frame    = 0x0001,
		save_positions = 0x0002,
		user_break     = 0x0004,
		fetch_reverse  = 0x0008,
		destroy_tables = 0x0010,
		correct_search_more_problem = 0x0020 // Опционально исправляет пероблему функции DBQuery::_search
			// (см. примечание @todo в теле функции).
	};
	SLAPI ~DBQuery();
	//
	// Descr: Определяет режим владения таблицами. Если set != 0,
	//   то при разрушении экземпляра будут разрушены и таблицы. В противном
	//   случае таблицы при разрушении DBQuery разрушаться не будут.
	// Returns:
	//   1
	//
	void   SLAPI setDestroyTablesMode(int set);
	//
	// Descr: Определяет режим извлечения записей запросом.
	//   Если set != 0, то записи будут извлекаться для изменения.
	//   В противном случае - в обычном режиме.
	// Note: Функция должна быть вызвана после того, как полностью сформирован
	//   набор таблиц, по которому осуществляется выборка
	// Returns:
	//   1
	//
	void   SLAPI setSearchForUpdateMode(int set);
	DBQuery & SLAPIV from(DBTable *,...);
	DBQuery & SLAPIV groupBy(DBField,...);
	DBQuery & SLAPIV orderBy(DBField,...);
	DBQuery & FASTCALL where(DBQ &);
	DBQuery & SLAPI  having(DBQ &);
	int    FASTCALL addField(DBConst &);
	int    FASTCALL addField(DBE &);
	int    FASTCALL addField(const DBField &);
	int    FASTCALL addTable(DBTable *);
	int    SLAPI addOrderField(const DBField & rFld);
	int    SLAPI getFieldPosByName(const char * pFldName, uint * pPos) const;
	int    SLAPI setFrame(uint viewHight, uint = static_cast<uint>(_defaultBufSize), uint = static_cast<uint>(_defaultBufDelta));
	void * SLAPI getBuffer();
	void * FASTCALL getRecord(uint);
	const void * FASTCALL getRecordC(uint r) const;
	void * SLAPI getCurrent();
	int    SLAPI fetch(long count, char * buf, int dir);
	int    SLAPI fetch(long, char *, RECORDNUMBER *, int);
	int    SLAPI single_fetch(char *, RECORDNUMBER *, int);
	int    SLAPI top();
	int    SLAPI bottom();
	int    FASTCALL step(long);
	int    SLAPI page();
	int    SLAPI refresh();
	char * SLAPI tostr(void * rec, int fld, long fmt, char * buf);
	int    SLAPI search(const void * pPattern, CompFunc fcmp, int fld, uint srchMode, void * pExtraData = 0);

	static long _defaultBufSize;
	static long _defaultBufDelta;
//private:
	SLAPI  DBQuery();
	int    SLAPI checkWhereRestriction();
	int    SLAPI _max_hdl(DBDataCell *, const int * pList, int);
	void   SLAPI arrangeTerms();
	int    SLAPI makeNode(int tblN, int * pNode, int option, int * pPos);
	int    FASTCALL analyzeOrder(int * pKeyArray);
	int    FASTCALL chooseKey(int tblN);
	// @v9.8.6 int    SLAPI optimizeTree(int tblN);
	int    SLAPI calcRecSize();
	int    SLAPI allocFrame(uint);
	void   FASTCALL fillRecord(char *, RECORDNUMBER *);
	int    FASTCALL _search(uint n, int dir);
	int    SLAPI _fetch_next(uint count, uint p, int dir);
	int    SLAPI _fetch_prev(uint count, uint p);
	int    SLAPI _fetch_last(uint count, uint p);
	int    SLAPI searchOnPage(const void *, uint *, CompFunc, uint ofs, int srchMode, int nxt, void * pExtraData = 0);
	int    SLAPI reverse();
	uint   FASTCALL addr(uint) const;
	void   SLAPI moveRec(uint rd, uint rs);
	void   SLAPI moveBuf(uint dest, uint src, uint recs);
	void   FASTCALL setCount(uint);
	void   FASTCALL frameOnBottom(int undefSDelta);
	int    FASTCALL normalizeFrame(int dir);
	enum {
		t_select   = 0x0001,
		t_from     = 0x0002,
		t_where    = 0x0004,
		t_group    = 0x0008,
		t_having   = 0x0010,
		t_order    = 0x0020,
		t_all      = 0x0040,
		t_distinct = 0x0080
	};
	enum {
		s_add_index_needed = 0x0001,
		s_tmp_table_needed = 0x0002,
		s_rec_cut          = 0x0004
	};
	struct Frame {
		SLAPI ~Frame();
		void FASTCALL topByCur(int dir);
		enum {
			Undef,
			Top,
			Bottom
		};
		char * buf;   // Data buffer
		RECORDNUMBER * posBuf; // Position buffer
		uint   zero;
		uint   size;  // Size of buffer (records)
		uint   state;
		uint   inc;
		uint   hight; // Size of frame of view
		uint   top;
		uint   cur;
		uint   sdelta;
		uint   srange;
		uint   spos;
		uint   last;
		uint   count; // Number of records in buffer
	};
	struct Tbl {
		Tbl();
		int    FASTCALL Srch(void * pKey, int sp);
		DBTable * tbl;
		DBTree tree;
		KR     key;
		char * keyBuf;
		DBRowId Pos;
		uint   flg;        // DBQTF_XXX
	};
	struct Fld {
		TYPEID     type;
		DBDataCell cell;
	};
	uint   syntax;
	uint   options;
	uint   status;
	int    error;
	DBQ  * w_restrict; // 'Where' restriction
	DBQ  * h_restrict; // 'Having' restriction
	uint   tblCount;
	uint   fldCount;
	uint   ordCount;
	DBField * order;
	RECORDSIZE recSize;  // Size of record (bytes)
	ulong  actCount;     // Number of records actualy fetched by fetch()
	Tbl  * tbls;
	Fld  * flds;
	Frame * P_Frame;
};

DBE & __stdcall ltoa(DBItem &);
DBE & __stdcall enumtoa(DBItem &, int, char **);
DBE & __stdcall flagtoa(DBItem & i, long mask, char ** str_array);
DBE & __stdcall operator + (DBItem &, DBItem &);
DBE & __stdcall operator - (DBItem &, DBItem &);
DBE & __stdcall operator * (DBItem &, DBItem &);
DBE & __stdcall operator / (DBItem &, DBItem &);
DBE & __stdcall operator % (DBItem &, DBItem &);
DBE & __stdcall operator + (DBItem &, double);
DBE & __stdcall operator - (DBItem &, double);
DBE & __stdcall operator * (DBItem &, double);
DBE & __stdcall operator / (DBItem &, double);
DBE & __stdcall operator % (DBItem &, long);
DBE & __stdcall operator + (double,  DBItem &);
DBE & __stdcall operator - (double,  DBItem &);
DBE & __stdcall operator * (double,  DBItem &);
DBE & __stdcall operator / (double,  DBItem &);
DBE & __stdcall operator % (long,    DBItem &);
DBE & __stdcall operator + (const char *,  DBItem &);
DBE & __stdcall operator + (DBItem &, const char * pS);
DBE & __stdcall operator + (LDATE,   DBItem &);
DBE & __stdcall operator - (LDATE,   DBItem &);
DBE & __stdcall operator - (DBItem &, LDATE);
DBE & __stdcall operator - (LTIME,   DBItem &);
DBE & __stdcall operator - (DBItem &, LTIME);
DBE & __stdcall operator & (DBItem &, long);
DBE & __stdcall operator & (DBItem &, DBItem &);
DBE & __stdcall ltrim(DBItem &);
DBE & __stdcall rtrim(DBItem &);
DBE & __stdcall left(DBItem &, int);
DBE & __stdcall right(DBItem &, int);
DBE & __stdcall sstrlen(DBItem &);
DBE & __stdcall lower(DBItem &);
DBE & __stdcall upper(DBItem &);
DBE & __stdcall contains(DBItem & i, const char * s);
DBE & __stdcall day(DBItem &);
DBE & __stdcall month(DBItem &);
DBE & __stdcall year(DBItem &);
DBE & __stdcall hour(DBItem &);
DBE & __stdcall minute(DBItem &);
DBE & __stdcall millisecond(DBItem &);
DBQ & __stdcall operator == (DBItem &, DBItem &);
DBQ & __stdcall operator += (DBItem &, DBItem &); // Outer join
DBQ & __stdcall operator != (DBItem &, DBItem &);
DBQ & __stdcall operator <  (DBItem &, DBItem &);
DBQ & __stdcall operator >  (DBItem &, DBItem &);
DBQ & __stdcall operator <= (DBItem &, DBItem &);
DBQ & __stdcall operator >= (DBItem &, DBItem &);
DBQ & __stdcall operator == (DBItem &, double);
DBQ & __stdcall operator != (DBItem &, double);
DBQ & __stdcall operator <  (DBItem &, double);
DBQ & __stdcall operator >  (DBItem &, double);
DBQ & __stdcall operator <= (DBItem &, double);
DBQ & __stdcall operator >= (DBItem &, double);
DBQ & __stdcall operator == (DBItem &, long);
DBQ & __stdcall operator <  (DBItem &, long);
DBQ & __stdcall operator >  (DBItem &, long);
DBQ & __stdcall operator <= (DBItem &, long);
DBQ & __stdcall operator >= (DBItem &, long);
DBQ & __stdcall operator == (DBItem &, const char *);
DBQ & __stdcall operator != (DBItem &, const char *);
DBQ & __stdcall operator <  (DBItem &, const char *);
DBQ & __stdcall operator >  (DBItem &, const char *);
DBQ & __stdcall operator <= (DBItem &, const char *);
DBQ & __stdcall operator >= (DBItem &, const char *);
DBQ & __stdcall operator == (DBItem &, LDATE);
DBQ & __stdcall operator != (DBItem &, LDATE);
DBQ & __stdcall operator <  (DBItem &, LDATE);
DBQ & __stdcall operator >  (DBItem &, LDATE);
DBQ & __stdcall operator <= (DBItem &, LDATE);
DBQ & __stdcall operator >= (DBItem &, LDATE);
DBQ & __stdcall operator == (DBItem &, LTIME);
DBQ & __stdcall operator != (DBItem &, LTIME);
DBQ & __stdcall operator <  (DBItem &, LTIME);
DBQ & __stdcall operator >  (DBItem &, LTIME);
DBQ & __stdcall operator <= (DBItem &, LTIME);
DBQ & __stdcall operator >= (DBItem &, LTIME);
DBQ & __stdcall operator == (DBItem &, LDATETIME);
DBQ & __stdcall operator != (DBItem &, LDATETIME);
DBQ & __stdcall operator <  (DBItem &, LDATETIME);
DBQ & __stdcall operator >  (DBItem &, LDATETIME);
DBQ & __stdcall operator <= (DBItem &, LDATETIME);
DBQ & __stdcall operator >= (DBItem &, LDATETIME);
DBQ & __stdcall operator && (DBQ &, DBQ &);
DBQ & __stdcall operator &= (DBQ &, DBQ &);
DBQ * __stdcall operator &= (DBQ *, DBQ &);
DBQ & __stdcall operator || (DBQ &, DBQ &);
DBQ & __stdcall daterange(DBItem & i, LDATE, LDATE);
DBQ & __stdcall daterange(DBItem & i, const DateRange *);
DBQ & __stdcall timerange(DBItem & i, LTIME, LTIME);
DBQ & __stdcall timerange(DBItem & i, const TimeRange *);
DBQ & __stdcall realrange(DBItem & i, double, double);
DBQ & __stdcall intrange(DBItem  & i, const IntRange & rR);
//
// Descr: Класс поддержки расширенных операций вставки записей
//
class BExtInsert : public SdRecordBuffer {
public:
	SLAPI  BExtInsert(DBTable * pTbl, size_t aBufSize = 0);
	SLAPI ~BExtInsert();
	//
	// Если sz == 0, то размер записи полагается равным fixRecSize.
	// Если sz >  0, то размер записи полагается равным sz
	// Если sz <  0, то размер записи равен
	//      FixRecSize + sstrlen(((char *)b) + FixRecSize) + 1
	//
	int    FASTCALL insert(const void * b);
	int    SLAPI flash();
	uint   SLAPI getActualCount() const;
	DBTable * SLAPI getTable();
private:
	enum {
		stValid   = 0x0001,
		stHasNote = 0x0002
	};
	DBTable * P_Tbl;
	uint16 ActualCount;
	RECORDSIZE FixRecSize;
	long   State;
	static const size_t DefBufSize;
};
//
// Descr: Интерфейс к extended-операция Btrieve
//
class BExtQuery {
public:
	enum {
		opEq = 1,
		opGt = 2,
		opLt = 3,
		opNe = 4,
		opGe = 5,
		opLe = 6
	};
	enum {
		linkEnd = 0,
		linkAnd = 1,
		linkOr  = 2
	};
	static int FASTCALL SlToBeqOp(int s);
	static int FASTCALL SlToBeqLink(int s);
	//
	// Descr: Удаляет экземпляр *ppQ с обнулением.
	// Note: Реализована для замещения частых вызовов ZDELETE(q)
	//
	static void FASTCALL ZDelete(BExtQuery ** ppQ);
	//
	// ARG(aBufSize IN): Количество записей, которое должно обрабатываться буфером.
	//
	SLAPI  BExtQuery(DBTable * pTbl, int idx, uint aBufSize /*= 32*/);
	SLAPI  BExtQuery(DBTable * pTbl, int idx);
	SLAPI ~BExtQuery();
	void   SLAPI setMaxReject(uint);
	//
	// Descr: Снимает внутренний признак окончания чтения. Необходим для случаев,
	//   когда требуется продолжить чтение с текущей позиции в предположении, что
	//   с момента последнего чтения появились новые записи.
	//
	void   SLAPI resetEof();
	BExtQuery & SLAPI  select(const DBFieldList &);
	BExtQuery & SLAPI  selectAll();
	BExtQuery & SLAPIV select(DBField,...);
	int    SLAPI addField(const DBField &);
	void   FASTCALL where(DBQ &);
	//
	// Вызов функции fetchFirst должен обязательно предшествовать вызовам fetchNext.
	// fetchFirst инициализирует итерации поиска. Особое внимание - параметрам initKey и initSpMode.
	// Специфика extended-операций Btrieve такова, что для эффективной работы этих операций необходимо
	// умно установить стартовую позицию в таблице. Два последних параметра как раз и служат для того,
	// чтобы вызывающая функция могла указать куда необходимо прыгнуть перед вызовом extended-операции.
	// Если initKey == 0 и initSpMode >= 0, то функция fetchFirst попытается оптимизировать запрос и выбрать
	// оптимальную стартовую позицию (пока не реализовано). Если же и initKey == 0 и initSpMode < 0,
	// то fetchFirst полагается на ту позицию, которая установлена перед ее вызовом.
	//
	int    SLAPI fetchFirst(void * initKey = 0, int initSpMode = spFirst);
	int    SLAPI initIteration(int reverse, const void * pInitKey = 0, int initSpMode = spFirst);
	int    SLAPI nextIteration();
	long   SLAPI countIterations(int reverse, const void * pInitKey = 0, int initSpMode = spFirst);
	int    FASTCALL getRecPosition(DBRowId * pPos);
	uint   SLAPI getActualCount() const { return ActCount; }
	int    SLAPI CreateSqlExpr(Generator_SQL * pSg, int reverse, const char * pInitKey, int initSpMode) const;
private:
	void   SLAPI Init(DBTable * pTbl, int idx, uint aBufSize);
	int    SLAPI search_first(const char * pInitKey, int initSpMode, int spMode);
	int    FASTCALL _search(int spMode, uint16 signature);
	int    FASTCALL add_term(int link, int n);
	int    FASTCALL add_tree(int link, int n);
	// @v9.8.6 (inlined) BExtTerm * SLAPI get_term(int);
	//
	// Descr: Перекидывает данные записи с текущим индексом cur
	//   из буфера BExtQuery::Buf в буфер записи таблицы данных DBTable::buf.
	//
	int    SLAPI fillTblBuf();
	//
	// Descr: Возвращает указатель на образ записи, полученной при
	//   вызове fetchFirst или fetchNext. n = (0..). Если n == UNDEF, то
	//   возвращается образ по индексу BExtQuery::cur. Первые
	//   sizeof(BExtResultItem) байт образа занимает заголовок (см. DB.H).
	//
	char * SLAPI getRecImage();

	DBTable * P_Tbl;
	DBQ * P_Restrict;
	DBTree  * P_Tree;
	int    Index_;
	DBFieldList Fields;
	union {
		char * P_QBuf;
		BExtHeader * P_QHead;
	};
	STempBuffer Buf;
	uint   RecSize;
	uint   MaxRecs;
	uint   ActCount;
	uint   TailOfs;
	uint   Cur;
	DBRowId Position;
	uint16 MaxReject; // Максимальное количество пропускаемых запросом записей. 0xffffU - не ограничено
	uint16 Reserve;   // @alignment
	enum {
		stFillBuf      = 0x0001,
		stUndefInitKey = 0x0002,
		stFirst        = 0x0004, // Следующее обращение к nextIteration будет первым
		stFirstFail    = 0x0008, // Первое обращение к nextIteration не принесло ни одной записи
		stEOF          = 0x0010,
		stRejectLimit  = 0x0020,
		stPosSaved     = 0x0040, // Признак того, что position содержит позицию предыдущего обращения к DBTable::getExtended
		stReverse      = 0x0080, // initIteration затребовал обратный по отношению к индексу порядок выборки записей
		stHasNote      = 0x0100, // Одно из полей имеет тип S_NOTE (специальная обработка поля переменной длины)
		stSqlProvider  = 0x0200  // Таблица tbl находится в подчинении провайдера SQL-сервера
	};
	long   State;
	SSqlStmt * P_Stmt;
	int    InitSpMode;
	char   InitKey[ALIGNSIZE(MAXKEYLEN, 2)];
	char   _Key[ALIGNSIZE(MAXKEYLEN, 2)];
};
//
//
//
struct DBUpdateSetItem {
	DBField Fld;
	DBDataCell Val;
};

class DBUpdateSet : SArray {
public:
	DBUpdateSet();
	~DBUpdateSet();
	DBUpdateSet & FASTCALL set(DBField f, DBItem & val);
	uint   GetCount() const;
	DBUpdateSetItem & Get(uint pos) const;
private:
	virtual void FASTCALL freeItem(void *);
};

DBUpdateSet & FASTCALL set(DBField f, DBItem & val);
//
// Descr: Спцификация callback-функции, используемой -Cb вариантами функций deleteFrom и upfateFor.
//
typedef int (*UpdateDbTable_CbProc)(DBTable * pTbl, const void * pRecBefore, const void * pRecAfter, void * extraPtr);

int FASTCALL deleteFrom(DBTable * pTbl, int useTA, DBQ & query);
int FASTCALL updateFor(DBTable * pTbl, int useTA, DBQ & query, DBUpdateSet & rSet);
int updateForCb(DBTable * pTbl, int useTA, DBQ & query, DBUpdateSet & rSet, UpdateDbTable_CbProc cbProc, void * extraPtr);
//
// Descr: Класс для работы с базами данных BerkeyDB
//
struct __db; typedef struct __db DB;
struct __db_dbt; typedef struct __db_dbt DBT;
struct __db_env; typedef struct __db_env DB_ENV;
struct __db_txn; typedef struct __db_txn DB_TXN;
struct __dbc; typedef struct __dbc DBC;
struct __env; typedef struct __env ENV;
struct __db_sequence; typedef struct __db_sequence DB_SEQUENCE;

class BDbTable {
public:
	friend class BDbDatabase;
	friend class BDbCursor;
	//
	// Descr: Типы индексов, используемые при создании таблиц
	//
	enum {
		idxtypDefault = 0,
		idxtypBTree,
		idxtypHash,
		idxtypHeap,
		idxtypQueue
	};
	enum {
		stError     = 0x0001,
		stOpened    = 0x0002,
		stIndex     = 0x0004, // Таблица является индексной. Поле P_MainT ссылается на основную таблицу.
		stOwnSCtx   = 0x0008, // Has own instance of SSerializeContext (P_SCtx)
		stReadOnly  = 0x0010, // Таблица была открыта в режиме ofReadOnly
		stExclusive = 0x0020  // Таблица была открыта в режиме ofExclusive
	};
	enum {
		cfEncrypt  = 0x0001,
		cfDup      = 0x0002,
		cfNeedSCtx = 0x0004  // Таблице требуется контекст сериализации. Если объект
			// создается с указателем на BDbDatabase, то контектс сериализации наследуется от
			// BDbDatabase, в противном случае создается собственный контекст.
	};
	//
	// Descr: Флаги открытия таблицы базы данных
	//
	enum {
		ofReadOnly       = 0x0001, // Только для чтения //
		ofExclusive      = 0x0002, // Эксклюзивный режим работы
		ofReadUncommited = 0x0004  // @v10.1.8  DB_READ_UNCOMMITTED
	};
	class Config {
	public:
		void   Clear();

		int    IdxType;      // BDbTable::idxtypXXX
		long   Flags;        // BDbTable::cfXXX
		uint32 DataChunk;    //
		//uint32 CacheSize;    // @v9.6.4 Размер кэша таблицы (kilobytes!!!). 0 - default
		uint32 PageSize;     // @v9.6.4 Размер страницы данных (bytes). 0 - default
		uint32 HashNElem;    // @v9.7.12 Для хэш-таблиц: ожидаемый размер хэш-таблицы
		uint32 HashFFactor;  // @v9.7.12 Для хэш-таблиц: fill-factor. Reasonable rule: (pagesize - 32) / (average_key_size + average_data_size + 8)
		uint   PartitionCount; // @v9.8.2 Количество partitions. Если параметр не нулевой, то
			// класс таблицы обязан переопределить виртуальную функцию Implement_PartitionFunc(DBT * pKey)
			// таким образом, чтобы она возвращала номер [0..PartitionCount-1] в зависимости от значения ключа pKey.
		SString Name;        // Имя файла
	protected:
		Config(const char * pName, int idxType, long flags, uint32 pageSize, uint32 cacheSizeKb, uint partitionCount);
	};
	//
	// Descr: Конфигурация таблицы с типом индекса HASH.
	//
	struct ConfigHash : public Config {
		//
		// Descr: Конструктор конфигурации таблицы с типом индекса HASH.
		// Note: Экспериментально выяснилось, что аргументы nElem и ffactor улучшения производительности
		//   не дают, и чаще сильно затормаживают работу (возможно, нужны дополнительные изыскания).
		//
		ConfigHash(const char * pName, long flags, uint32 pageSize, uint partitionCount, uint32 nElem = 0, uint32 ffactor = 0) :
			Config(pName, idxtypHash, flags, pageSize, 0, partitionCount)
		{
			HashNElem = nElem;
			HashFFactor = ffactor;
		}
	};
	//
	// Descr: Конфигурация таблицы с типом индекса BTREE.
	//
	struct ConfigBTree : public Config {
		ConfigBTree(const char * pName, long flags, uint32 pageSize, uint partitionCount) :
			Config(pName, idxtypBTree, flags, pageSize, 0, partitionCount)
		{
		}
	};
	//
	// Descr: Буфер данных для ключей и записей, вносимых и считываемых из таблиц.
	// Note: Бинарное представление заголовка этого класса обязательно должно быть совместимым
	//   со структурой DBT. Таким образом, ни в коем случае нельзя наследовать это класс от каких-либо
	//   иных классов или структур, а так же применять в нем виртуальные методы.
	//
	class Buffer { // @novtable
	public:
		enum {
			fMalloc  = 0x0010, // DB_DBT_MALLOC // When this flag is set, Berkeley DB will allocate memory
				// for the returned key or data item (using SAlloc::M(3), or the user-specified SAlloc::M function),
				// and return a pointer to it in the data field of the key or data DBT structure.
				// Because any allocated memory becomes the responsibility of the calling application, the caller
				// must determine whether memory was allocated using the returned value of the data field.
				// It is an error to specify more than one of DB_DBT_MALLOC, DB_DBT_REALLOC, and DB_DBT_USERMEM.
			fRealloc = 0x0080, // DB_DBT_REALLOC // When this flag is set Berkeley DB will allocate memory for the returned key or
				// data item (using SAlloc::R(3), or the user-specified realloc function), and return a pointer to it in the
				// data field of the key or data DBT structure. Because any allocated memory becomes the
				// responsibility of the calling application, the caller must determine whether memory was
				// allocated using the returned value of the data field. The difference between DB_DBT_MALLOC and DB_DBT_REALLOC
				// is that the latter will call SAlloc::R(3) instead of SAlloc::M(3), so the allocated memory will be grown as necessary
				// instead of the application doing repeated free/SAlloc::M calls. It is an error to specify more than
				// one of DB_DBT_MALLOC, DB_DBT_REALLOC, and DB_DBT_USERMEM.
			fUserMem = 0x0800, // DB_DBT_USERMEM // The data field of the key or data structure must refer to memory that is at least
				// ulen bytes in length. If the length of the requested item is less than or equal to that
				// number of bytes, the item is copied into the memory to which the data field refers.
				// Otherwise, the size field is set to the length needed for the requested item, and the error
				// DB_BUFFER_SMALL is returned. It is an error to specify more than one of DB_DBT_MALLOC, DB_DBT_REALLOC, and DB_DBT_USERMEM.
			fPartial = 0x0040, // DB_DBT_PARTIAL // Do partial retrieval or storage of an item. If the calling application is doing a get,
				// the dlen bytes starting doff bytes from the beginning of the retrieved data record are
				// returned as if they comprised the entire record. If any or all of the specified bytes do not
				// exist in the record, the get is successful, and any existing bytes are returned.
				// For example, if the data portion of a retrieved record was 100 bytes, and a partial
				// retrieval was done using a DBT having a dlen field of 20 and a doff field of 85, the get call
				// would succeed, the data field would refer to the last 15 bytes of the record, and the size
				// field would be set to 15.
				// If the calling application is doing a put, the dlen bytes starting doff bytes from the
				// beginning of the specified key's data record are replaced by the data specified by the
				// data and size structure elements. If dlen is smaller than size the record will grow; if dlen
				// is larger than size the record will shrink. If the specified bytes do not exist, the record
				// will be extended using nul bytes as necessary, and the put call will succeed.
				// It is an error to attempt a partial put using the DB->put() (page 73) method in a database
				// that supports duplicate records. Partial puts in databases supporting duplicate records
				// must be done using a DBcursor->put() (page 172) method.
				// It is an error to attempt a partial put with differing dlen and size values in Queue or
				// Recno databases with fixed-length records.
				// For example, if the data portion of a retrieved record was 100 bytes, and a partial put
				// was done using a DBT having a dlen field of 20, a doff field of 85, and a size field of 30,
				// the resulting record would be 115 bytes in length, where the last 30 bytes would be those
				// specified by the put call.
				// This flag is ignored when used with the pkey parameter on DB->pget() or DBcursor->pget().
			fAppMalloc = 0x0001, // DB_DBT_APPMALLOC // After an application-supplied callback routine passed to either DB->associate()
				// (page 6) or DB->set_append_recno() (page 83) is executed, the data field of a DBT may refer
				// to memory allocated with SAlloc::M(3) or SAlloc::R(3). In that case, the callback sets the
				// DB_DBT_APPMALLOC flag in the DBT so that Berkeley DB will call SAlloc::F(3) to deallocate the
				// memory when it is no longer required.
			fMultiple  = 0x0020, // DB_DBT_MULTIPLE  // Set in a secondary key creation callback routine passed to DB->associate()
				// (page 6) to indicate that multiple secondary keys should be associated with the given primary key/data pair.
				// If set, the size field indicates the number of secondary keys and the data field
				// refers to an array of that number of DBT structures.
				// The DB_DBT_APPMALLOC flag may be set on any of the DBT structures to indicate that
				// their data field needs to be freed.
			fReadOnly  = 0x0100  // DB_DBT_READONLY   // When this flag is set Berkeley DB will not write into the DBT.
				// This may be set on key values in cases where the key is a static string that cannot be written
				// and Berkeley DB might try to update it because the application has set a user defined comparison function.
		};

		DECL_INVARIANT_C();

		explicit Buffer(size_t initSize = 128);
		explicit Buffer(const DBT * pB);
		void   Reset();
		operator DBT * () { return reinterpret_cast<DBT *>(this); }
		int    FASTCALL Alloc(size_t sz);
		//
		// Descr: Если ULen < Size и Flags & fUserMem, то увеличивает распределенный
		//   размер буфера для акцепта полной заппси.
		//
		int    Realloc();
		Buffer & FASTCALL operator = (const BDbTable::Buffer & rS);
		Buffer & FASTCALL operator = (const SBuffer & rBuf);
		Buffer & FASTCALL operator = (const char * pStr);
		Buffer & FASTCALL operator = (const wchar_t * pUStr);
		Buffer & FASTCALL operator = (const int32 & rVal);
		Buffer & FASTCALL operator = (const uint32 & rVal);
		Buffer & FASTCALL operator = (const int64 & rVal);
		Buffer & FASTCALL operator = (const uint64 & rVal);
		Buffer & FASTCALL Set(const void * pData, size_t size);
		// @construction Buffer & FASTCALL SetShrinked(uint64 val);

		size_t GetSize() const { return Size; }
		const  void * FASTCALL GetPtr(size_t * pSize) const;
		DBT *  FASTCALL Get(DBT * pD) const;
		int    FASTCALL Get(SBuffer & rBuf) const;
		int    FASTCALL Get(SString & rBuf) const;
		int    FASTCALL Get(SStringU & rBuf) const;
		int    Get(void * pBuf, size_t bufSize) const;
		int    FASTCALL Get(int32 * pBuf) const;
		int    FASTCALL Get(uint32 * pBuf) const;
		int    FASTCALL Get(int64 * pBuf) const;
	private:
		void * P_Data;
		uint32 Size;
		uint32 ULen;
		uint32 DLen;
		uint32 DOff;
		void * P_AppData;
		uint32 Flags;
		STempBuffer B;
	};

	class SecondaryIndex {
	public:
		friend class BDbTable;

		SecondaryIndex();
		virtual ~SecondaryIndex();
	protected:
		virtual int Cb(const BDbTable::Buffer & rKey, const BDbTable::Buffer & rData, BDbTable::Buffer & rResult) = 0;
		virtual int Implement_Cmp(BDbTable * pMainTbl, const BDbTable::Buffer * pKey1, const BDbTable::Buffer * pKey2);

		BDbTable::Buffer ResultBuf;
		BDbTable * P_MainT; // @notowned
	};

	struct Statistics {
		Statistics();
		struct ISz {
			ISz();
			void   FASTCALL Put(const BDbTable::Buffer & rB);

			uint64 Count;
			uint64 Total;
			uint32 Min;
			uint32 Max;
		};
		struct ICt {
			ICt();
			void   FASTCALL Put(uint64 t);

			uint64 Count;
			uint64 TmTotal;
			uint64 TmMin;
			uint64 TmMax;
		};
		ICt    CtIns;
		ICt    CtUpd;
		ICt    CtRmv;
		ICt    CtGet;
		ISz    SzKey;
		ISz    SzRec;
	};

	BDbTable(const Config & rCfg, BDbDatabase * pDb);
	BDbTable(const Config & rCfg, BDbDatabase * pDb, SecondaryIndex * pIdxHandle, BDbTable * pMainTbl);
	virtual ~BDbTable();
	int    operator ! () const;
	operator DB * ();
	operator DB_TXN * ();
	TSCollection <BDbTable> & GetIdxList();
	int    IsConsistent() const;
	int    FASTCALL GetState(long stateFlag) const;
	int    GetConfig(int idx, Config & rCfg);
	int    Create(const char * pFileName, int createMode, BDbTable::Config * pCfg);
	//
	// Descr: Открывает таблицы базы данны с именем pFileName.
	// ARG(pFileName IN): Имя таблицы данных
	// ARG(flags     IN): Опции открытия таблицы (ofXXX see above).
	// Returns:
	//   >0 - таблица открыта успешно
	//   0  - ошибка
	//
	int    Open(const char * pFileName, int flags);
	int    Close();
	int    Search(Buffer & rKey, Buffer & rData);
	int    SearchPair(Buffer & rKey, Buffer & rData);
	int    Search(int idx, Buffer & rKey, Buffer & rData);
	int    InsertRec(Buffer & rKey, Buffer & rData);
	int    UpdateRec(Buffer & rKey, Buffer & rData);
	int    DeleteRec(Buffer & rKey);
protected:
	static int VerifyStatic();
	static int ScndIdxCallback(DB * pSecondary, const DBT * pKey, const DBT * pData, DBT * pResult);
	static int CmpCallback(DB * pDb, const DBT * pDbt1, const DBT * pDbt2);
	static int CmpCallback_6232(DB * pDb, const DBT * pDbt1, const DBT * pDbt2, size_t *);
	static uint32 PartitionCallback(DB * pDb, DBT * pDbt);
	//
	// Descr: Реализует сравнение ключей.
	//   Специальный случай: если pKey1 == 0 && pKey2 == 0, функция должна вернуть 1, если
	//   следует устанавливать специальное сравнение и 0 - если BDB должна применять сравнение по умолчанию.
	//
	virtual int Implement_Cmp(const BDbTable::Buffer * pKey1, const BDbTable::Buffer * pKey2);
	virtual uint FASTCALL Implement_PartitionFunc(DBT * pKey);
	//
	// Descr: Инициализирует экземпляр таблицы в контексте базы данных pDb.
	// ARG(pDb   IN): Указатель на экземпляр базы данных, в контексте которой открывается таблица.
	// ARG(flags IN): Флаги открытия таблицы (BDbTable::ofXXX).
	// Returns:
	//   >0 - таблица успешно инициализирована
	//    0 - ошибка
	//
	int    InitInstance(BDbDatabase * pDb, int flags);
	int    Helper_EndTransaction();
	SSerializeContext * GetSCtx() const;
	int    Helper_Search(Buffer & rKey, Buffer & rData, uint32 flags);
	int    Helper_Put(Buffer & rKey, Buffer & rData, uint32 flags);
	int    Helper_GetConfig(BDbTable * pH, Config & rCfg);

	uint32 Sign;   // Подпись экземпляра класса. Используется для идентификации инвалидных экземпляров.
	int    Handle; // Индекс открытой таблицы в списке DBThreadLocalArea
	long   State;  // BDbTable::stXXX Флаги состояния //
	DB   * H;      // Table handler
	Config Cfg;
	Statistics Stat;
	BDbDatabase * P_Db;
	BDbTable * P_MainT;              // @#{(State & stIndex) || !P_MainT} Для индексной таблицы - основная таблица.
	SecondaryIndex * P_IdxHandle;    // @#{(State & stIndex) || !P_IdxHandle}
	TSCollection <BDbTable> IdxList; // @#{!(State & stIndex) || !IdxList} Список индексов
private:
	SSerializeContext * P_SCtx;      //
};

class BDbCursor {
public:
	BDbCursor(BDbTable &, int idx);
	~BDbCursor();
	int    operator !() const;
	int    Search(BDbTable::Buffer & rKey, BDbTable::Buffer & rData, int sp);
	int    Insert(BDbTable::Buffer & rKey, BDbTable::Buffer & rData, int current);
	int    Delete();
private:
	DB   * FASTCALL GetIntTbl(int idx);

	BDbTable & R_Tbl;
	DBC  * C;
	int    Idx;
};

class BDbDatabase {
public:
	//
	// Descr: Флаги состояния объекта.
	//   Поле состояния возвращается функцией GetState()
	//
	enum {
		stError            = 0x0001,
		stLoggedIn         = 0x0002,
		stReadOnly         = 0x0004, // @v9.7.11 Экземпляр базы данных создан в режиме READ-ONLY
		stWriteStatOnClose = 0x0008, // @v9.7.11 При закрытии базы сохранять статистику по таблицам (проекция oWriteStatOnClose)
		stExclusive        = 0x0010  // @v10.0.12 База данных открыта в эксклюзивном режиме
	};
	enum {
		oRecover          = 0x00000001,
		oPrivate          = 0x00000002, // @v9.6.4 окружение (ENVIRONMENT) BerkeleyDB не может быть использовано разными процессами
		oReadOnly         = 0x00000004, // @v9.7.11 База данных открывается в режиме READ-ONLY
		oWriteStatOnClose = 0x00000008, // @v9.7.11 При закрытии базы сохранять статистику по таблицам
		oExclusive        = 0x00000010, // @v10.0.12 База данных открывается в эксклюзивном режиме
	};
	struct Config {
		Config();

		enum {
			fLogNoSync      = 0x00000004, // @v9.6.6 DB_LOG_NOSYNC
			fLogAutoRemove  = 0x00000008, // @v9.6.6 DB_LOG_AUTO_REMOVE
			fLogInMemory    = 0x00000010, // @v9.6.6 DB_LOG_IN_MEMORY
		};

		long   Flags;
		uint64 CacheSize;   // Максимальный размер кэш-буферов (bytes).
		uint   CacheCount;  // Максимальное количество кэш-буферов.
		uint   PageSize;    // @v9.6.4 Размер страницы данных

		uint   MaxLockers;  // Максимальное количество локеров.
		uint   MaxLocks;    // @v9.6.4 A thread uses this structure to lock a page (or record for the QUEUE access method) and hold it to the end of a transactions
		uint   MaxLockObjs; // @v9.6.4 For each page (or record) which is locked in the system, a lock object will be allocated.

		uint   LogBufSize;  // Размер буфера журнала транзакция (bytes).
		uint   LogFileSize; // Размер одного файла журнала транзакций (bytes). BerkeleyDB формирует журналы
			// транзакций файлами одинакового размера. После заполнения очередного файла создается //
			// новый такого же размера.
			// По умолчанию BerkeleyDB применяет размер 10Mb.
		uint   MutexCountInit;
		uint   MutexCountMax;
		uint   MutexCountIncr;
		SString LogSubDir;
	};

	static SString & MakeFileName(const char * pFile, const char * pTblName, SString & rFileName);
	static int SplitFileName(const char * pFileName, SString & rFile, SString & rTbl);
	static int FASTCALL ProcessError(int bdbErrCode, const DB * pDb, const char * pAddedMsg);
	static int FASTCALL ProcessError(int bdbErrCode);

	explicit BDbDatabase(const char * pHomeDir, Config * pCfg = 0, long options = 0);
	~BDbDatabase();
	int    operator ! () const;
	operator DB_ENV * ();
	operator DB_TXN * ();
	int    SetupErrLog(const char * pFileName);
	int    GetCurrentConfig(Config & rCfg);
	int    IsFileExists(const char * pFileName);
	int    CreateDataFile(const char * pFileName, int createMode, const BDbTable::Config * pCfg);
	int    Implement_Open(BDbTable * pTbl, const char * pFileName, int openMode, char * pPassword);
	int    Implement_Close(BDbTable * pTbl);
	int    WriteStat(const BDbTable * pTbl);
	int    RemoveUnusedLogs();
	int    FASTCALL CheckInTxnTable(BDbTable * pTbl);
	int    StartTransaction();
	int    RollbackWork();
	int    CommitWork();
	int    TransactionCheckPoint();
	int    MemPoolSync();
	int    LockDetect();
	int    CreateSequence(const char * pName, int64 initVal, long * pSeqID);
	int    CloseSequence(long seqId);
	int    GetSequence(long seqId, int64 * pVal);
	SSerializeContext * GetSCtx() const;
private:
	int    Helper_Create(const char * pFileName, int createMode, const BDbTable::Config * pCfg);
	int    Helper_SetConfig(const char * pHomeDir, const Config & rCfg);
	//
	// Descr: Вспомогательная функция, реализующая открытие таблицы базы данных.
	// ARG(pFileName IN): Имя таблицы
	// ARG(pTbl      IN): Предварительно созданный экземпляр таблицы
	// ARG(flags     IN): Флаги открытия таблицы (BDbTable::ofXXX)
	// Returns:
	//   !0 - внутренний манипулятор таблицы
	//   0  - ошибка
	//
	void * Helper_Open(const char * pFileName, BDbTable * pTbl, int flags);
	void   Helper_Close(void * pH);
	uint   FASTCALL SearchSequence(const char * pSeqName) const;
	int    FASTCALL Helper_CloseSequence(uint pos);

	class Txn {
	public:
		Txn();
		DB_TXN * T;
		SCollection TblList;
	};

	long   State; // BDbDatabase::stXXX
	DB_ENV * E;
	Txn    T;
	BDbTable * P_SeqT;
	uint   HomePathPos;
	struct Seq {
		long   Id;
		DB_SEQUENCE * H;
		uint   NamePos;
	};
	TSVector <Seq> SeqList; // @v9.8.4 TSArray-->TSVector
	StringSet StrPool;
	SSerializeContext * P_SCtx;
	SFile  ErrF;
};

class BDbTransaction {
public:
	BDbTransaction(BDbDatabase * pDb, int use_ta);
	~BDbTransaction();
	int    operator !() const;
	int    IsStarted() const { return Ta; }
	int    Start(int use_ta);
	int    Commit(int setCheckpoint);
private:
	int    Ta;
	int    Err;
	BDbDatabase * P_Db;
};

#endif // } _WIN32_WCE
#endif /* __DB_H */
