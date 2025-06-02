// DL600.H
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2011, 2015, 2016б 2017, 2018, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#ifndef __DL600_H
#define __DL600_H
#include <slib.h>
#include <db.h>
#include <slui.h>
//
//
//
enum DlOperator {
	dlopConstructor = '0', // null
	dlopDestructor  = '1',
	dlopNew = '2',
	dlopDelete      = '3',
	dlopAssignment  = '4',
	dlopEq  = '8',
	dlopNeq = '9',
	dlopConvert     = 'B',
	dlopAdd = 'H', // +
	dlopSub = 'G', // -
	dlopMul = 'D',
	dlopDiv = 'K',
	dlopMod = 'L', // %
	dlopInc = 'E',
	dlopDec = 'F',
	dlopLt  = 'M',
	dlopLe  = 'N',
	dlopGt  = 'O', // ou
	dlopGe  = 'P',
	dlopAnd = 'V',
	dlopOr  = 'W',
	dlopNot = '7',
	dlopBwAnd       = 'I',
	dlopBwOr        = 'U',
	dlopUnaryPlus   = '+',  // not msvs
	dlopUnaryMinus  = '-',  // not msvs
	dlopDot = '.',  // not msvs
	dlopQuest       = '?',  // not msvs
	dlopObjRef      = '>'   // not msvs
};

#define MANGLE_OP_CON  '0' // 'zero' constructor
#define MANGLE_OP_DES  '1' // desctructor
#define MANGLE_OP_NEW  '2'
#define MANGLE_OP_DEL  '3'
#define MANGLE_OP_ASSN '4'
#define MANGLE_OP_EQ   '8'
#define MANGLE_OP_NEQ  '9'
#define MANGLE_OP_CVT  'B' // operator typ ()
#define MANGLE_OP_ADD  'H'
#define MANGLE_OP_SUB  'G'
#define MANGLE_OP_MUL  'D'
#define MANGLE_OP_DIV  'K'
#define MANGLE_OP_LT   'M'
#define MANGLE_OP_LE   'N'
#define MANGLE_OP_GT   'O' // 'ou'
#define MANGLE_OP_GE   'P' // >=
#define MANGLE_OP_INC  'E' // ++
#define MANGLE_OP_DEC  'F' // --
#define MANGLE_OP_AND  'V' // &&
#define MANGLE_OP_OR   'W' // ||
#define MANGLE_OP_NOT  '7' // !
#define MANGLE_OP_BAND 'I' // &
#define MANGLE_OP_BOR  'U' // |

#define MANGLE_BT_SCHAR 'C'
#define MANGLE_BT_CHAR  'D'
#define MANGLE_BT_UCHAR 'E'
#define MANGLE_BT_SHRT  'F'
#define MANGLE_BT_USHRT 'G'
#define MANGLE_BT_INT   'H'
#define MANGLE_BT_UINT  'I'
#define MANGLE_BT_LONG  'J'
#define MANGLE_BT_ULONG 'K'
#define MANGLE_BT_FLOAT 'M'
#define MANGLE_BT_DBL   'N' // double
#define MANGLE_BT_LDBL  'O' // long double
#define MANGLE_BT_PTR   'P' // pointer   (*)
#define MANGLE_BT_REF   'A' // reference (&)
#define MANGLE_BT_ARRAY 'Q' // array     ([])
#define MANGLE_BT_CLASS 'V'
#define MANGLE_BT_VOID  'X' // (terminates argument list)
#define MANGLE_BT_ELIPS 'Z' // ... (terminates argument list)
#define MANGLE_BT_ZSTRING  'U' // not msvs
#define MANGLE_BT_WZSTRING 'W' // not msvs @v8.6.2

#define MANGLE_ENDCLASS 0x4040 // @@
#define MANGLE_ENDSIGN  'Z' // End of signature. Любое сжатое имя заканчивается этим символом

#define MANGLE_MEMB     'Q'
#define MANGLE_STATIC   'S'
#define MANGLE_SA       'Y'
//
// Зарезервированные номера реализации встроенных функций (операторов), обрабатываемые
// до входа в процедуру DlContext::BuiltinOp
//
#define DL6FI_FIRST     200
#define DL6FI_AND       200
#define DL6FI_OR        201
#define DL6FI_QUEST     202
#define DL6FI_IF        203
#define DL6FI_DOT       204
#define DL6FI_REF       205
#define DL6FI_LAST      999

class  SetScopeBlk; // @Muxa
class  DlScope;
class  DlContext;
class  DlRtm;
class  SCoClass;
class  PPView;
struct CtmToken;
struct CtmPropertySheet;

typedef uint   DLSYMBID;
typedef int    (*DlFuncImpl)(SdRecord * pArgList);

struct DlFunc {
	friend class DlFuncPool;
	enum {
		fArgIn  = 0x0001,
		fArgOut = 0x0002
	};
	struct Arg {
		DLSYMBID TypID;
		uint   NamePos;
		uint   Flags;   // @v11.7.8 uint16-->uint
	};
	static bool FASTCALL GetOpName(uint opID, SString & rName); // Compile-time
	DlFunc();
	bool   GetName(uint options, SString & rName) const; // Compile-time
	uint   GetArgCount() const;
	int    GetArg(uint argN, Arg *) const;
	DLSYMBID GetArgType(uint argN) const;
	int    GetArgName(uint argN, SString & rArgName) const;
	bool   IsEq(const DlFunc & rPat) const;
	void   AddArg(uint typeId, const char * pName, uint argFlags = 0);

	enum {
		fImplByID  = 0x0001, // Функция реализована через идентификатор реализации
		fLossCvt   = 0x0002, // Преобразование типа с потерей точности
		fPrototype = 0x0004, // Прототип функции (реализация не известна). ImplID = 0
		fInlined   = 0x0008, // Для интерфейсной функции - отсутствует функция реализации. Реализация делается непосредственно в теле интерфейсной функции
		fPropGet   = 0x0010, //
		fPropPut   = 0x0020  // Если установлены оба флага, то в IDL генерируется две записи: propget и propput
	};
	SString Name;
	DLSYMBID TypID;
	uint   Flags;          // DlFunc::fXXX // @v11.7.8 uint16-->uint
	union {
		uint   ImplID;
		DlFuncImpl Impl;
	};
private:
	SVector ArgList;
	StringSet ArgNamList;
};

class DlFuncPool {
public:
	DlFuncPool();
	int    FASTCALL Write(SBuffer & rBuf) const;
	int    FASTCALL Read(SBuffer & rBuf);
	bool   FASTCALL IsEq(const DlFuncPool & rPat) const;
	void   FASTCALL Add(const DlFunc * pF);
	uint   GetCount() const { return Items.getCount(); }
	int    GetByPos(uint pos, DlFunc * pF) const;
	int    EnumByName(const char * pName, uint * pPos, DlFunc * pFunc) const;
private:
	struct F {             // @persistent
		uint   NamePos;
		DLSYMBID TypID;
		uint   ArgNamPos;
		uint   ArgPos;
		uint16 ArgCount;
		uint16 Flags;
		union {
			uint   ImplID;
			DlFuncImpl Impl;
		};
	};
	struct Arg {
		DLSYMBID TypID;
		uint16 NamePos;
		uint16 Flags;
	};
	int    SearchNamePos(uint namePos, uint * pPos) const;
	SVector Items;
	SVector ArgList;
	StringSet NamePool;
};
//
// Descr: Константа.
//   Используется в формулах и при хранении сецифических параметров областей видимости (DlScope)
//
struct CtmExprConst { // @flat
	CtmExprConst & Init();
	bool   operator !() const;

	DLSYMBID TypeId;   // Тип константы
	union {
		uint32   Pos;  // Если Size > sizeof(uint32), то это - позиция в CtmConstList
		uint32   Data; // Если Size <= sizeof(uint32), то это - сама константа
	};
};
//
// Descr: Пул констант. хранится в экземпляре класса DlContext.
//
class CtmConstList : private SBaseBuffer { // @persistent
public:
	CtmConstList();
	~CtmConstList();
	int    Add(CtmExprConst * pC, const void * pBuf, size_t len);
	const  void * GetPtr(const CtmExprConst * pC, size_t len) const;
	int    Get(const CtmExprConst * pC, void * pBuf, size_t len) const;
	int    FASTCALL Write(SBuffer * pBuf) const;
	int    FASTCALL Read(SBuffer * pBuf);
	int    Test_Cmp(const CtmConstList &) const;
private:
	int    SearchAnalog(const void * pBuf, size_t len, size_t * pPos) const;
	uint32 DataLen;
};

typedef TSCollection <DlScope> DlScopeList;
//
// Descr: Области файла определения данных.
//
class DlScope : public SdRecord {
public:
	//
	// Descr: Виды областей
	//
	enum {
		kGlobal = 1,  // G
		kNamespace,   // N
		kFile,        // F
		kClass,       // C
		kFunc,        // U
		kLocal,       // L
		kExpData,     // E '!' Структура данных для экспорта
		kExpDataHdr,  // H     Заголовок структуры данных для экспорта
		kExpDataIter, // I     Итератор структуры данных для экспорта
		kArgList,     // A     Список аргументов функции
		kInterface,   //   '@' Интерфейс
		kEnum,        //   '@' Перечисление
		kStruct,      //   '@' Структура
		kIClass,      //   '@' Интерфейсный класс (реализация интерфейсов)
		kLibrary,     //   '%' Библиотека типов
		kTypedefPool, //       Пул алиасов типов (typedef)
			//
			// Если в некоторой области определен один или несколько typedef, то эти
			// определения хранятся в дочерней области с Kind=kTypedefPool и именем "typedef"
		kDbTable,     //   '$' Таблица базы данных
		kDbIndex,     // X     Индекс таблицы базы данных
		kUiDialog,
		kUiZone,
		kUiCtrl,
		kUiView,      // Обобщенный элемент view, содержащий, помимо прочего, параметры layout'а
		kHandler      // 
	};
	//
	// Descr: Флаги деклараций. храняться в DvFlags
	//
	enum {
		declfDestroy   = 0x0001,
		declfDosStub   = 0x0002,
		declfNoALDD    = 0x0004, // "noaldd" Отсутствие режима совместимости с ALDD
		declfWSDL      = 0x0008, // "WSDL" Необходимо генерировать WSLD-описание структуры
		declfSet       = 0x0010  // "set" Класс экспортной структуры декларирует реализацию метода DlRtm::Set()
	};
	//
	// Descr: Флаги областей видимости, соответствующих интерфейсам, интерфейсным классам и
	//   библиотекам интерфейсов.
	//
	enum {
		sfPrototype    = 0x0001, // Прототип имени области (общий флаг)
		sfHidden       = 0x0002, // COM-атрибут
		sfRestricted   = 0x0004, // COM-атрибут
		sfVersion      = 0x0008, // Для области определена версия //
		sfUUID         = 0x0010, // Для атрибута UUID
		sfNoIDL        = 0x0020  // Не заносить описание объекта в IDL-файл
	};
	//
	// Descr: Флаги областей таблиц базы данных
	//
	enum {
		// 0x0001, 0x0008, 0x0010 использовать нельзя, поскольку зарезервированы за общими признаками
		sfDbtVLR        = 0x0002,
		sfDbtVAT        = 0x0004,
		sfDbtReplace    = 0x0020,
		sfDbtTruncate   = 0x0040,
		sfDbtCompress   = 0x0080,
		sfDbtBalanced   = 0x0100,
		sfDbtTemporary  = 0x0200,
		sfDbtSystem     = 0x0400
	};
	//
	// Descr: Флаги областей индексов таблиц базы данных
	//
	enum {
		// 0x0001, 0x0008, 0x0010 использовать нельзя, поскольку зарезервированы за общими признаками
		sfDbiDup        = 0x0002,
		sfDbiUnique     = 0x0004, // @#{sfDbiDup ^ sfDbiUnique}
		sfDbiMod        = 0x0020,
		sfDbiAllSegNull = 0x0040,
		sfDbiAnySegNull = 0x0080
	};
	//
	// Descr: Типы областей-манипуляторов (kHandler)
	//
	enum {
		hdltOnStart = 1,
		hdltOnFinish,
		hdltOnChange
	};
	//
	// Descr: Типы layout'ов пользовательского интерфейса
	// @construction
	//
	enum {
		layoutHorizontal = 1,
		layoutVertical,
		layoutTable,
		layoutRalative,
		layoutAbsolute
	};
	//
	// Descr: Идентификаторы констант, используемых для описания опций областей
	//
	enum COption { // @persistent
		//
		// Опции таблиц базы данных
		//
		cdbtFileName            =  1, // string Имя файла таблицы базы данных
		cdbtAcsName             =  2, // string AlternateCollatingSequence file name
		cdbtPageSize            =  3, // uint32 Размер страницы таблицы базы данных
		cdbtPrealloc            =  4, // uint32 Количество страниц распределяемых при создании таблицы базы данных
		cdbtPctThreshold        =  5, // uint32 BTRIEVE Процентный порог свободного места, оставляемого в странице VLR
		cdbtAccess              =  6, // uint32 Уровень доступа к таблице базы данных
		//
		// Опции диалогов и других элементов UI
		//
		cuiRect                 =  7, // raw    Координаты области пользовательского интерфейса
		cuifViewKind            =  8, // uint32 Вид управляющего элемента
		cuifCtrlText            =  9, // string Текст управляющего элемента
		cuifCtrlScope           = 10, // uint32 Ассоциированная область управляющего элемента
		cuifCtrlCmd             = 11, // uint32 ИД команды кнопки
		cuifCtrlCmdSymb         = 12, // string Символ команды кнопки
		cuifCtrlRect            = 13, // raw    Координаты управляющего элемента
		cuifCtrlListboxColDescr = 14, // string Строка описания колонок списка
		cuifSymbSeries          = 15, // string Серия символов управляющих элементов диалога
		cuifCtrlLblRect         = 16, // raw    Координаты текстового ярлыка, ассоциированного с управляющим элементом
		cuifCtrlLblSymb         = 17, // string Символ текстового ярлыка, ассоциированного с управляющим элементом
		cuifLabelRect           = 18, // raw(UiRelRect) Положение текстового ярлыка, ассоциированного с управляющим элементом
		cuifReadOnly            = 19, // int8 // deprecated in favor of cuifFlags
		cuifDisabled            = 20, // int8 // deprecated in favor of cuifFlags
		cuifAlignment           = 21, // int8
		cuifHidden              = 22, // int8 // deprecated in favor of cuifFlags
		cuifFont                = 23, // string
		cuifStaticEdge          = 24, // int8 // deprecated in favor of cuifFlags
		cuifLayoutType          = 25, // int8 @construction
		cuifViewTabStop         = 26, // @v11.0.4 bool // deprecated in favor of cuifFlags
		cuifLayoutBlock         = 27, // @v11.0.5 SUiLayoutParam
		cuifFlags               = 28, // @v11.0.4 UiItemKind::fXXX 
		cuifViewDataType        = 29, // @v11.0.5 Тип данных, ассоциированный с элементом View
		cuifCtrlLblRelation     = 30, // @v11.0.5 uint8 Расположение этикетки относительно основного управляющего элемента
		cuifViewDataIdent       = 31, // @v11.0.5 string Символ данных, ассоциированных с областью view
		cuifViewVariableIdent   = 32, // @v11.0.5 string Символ переменной, ассоциированной с областью view
		cuifFontSize            = 33, // @v11.0.5 double Размер шрифта  
		cuifLblLayoutBlock      = 34, // @v12.3.2 SUiLayoutParam Разметка размещения текстовой метки относительно основного управляющего элемента
		cuifCbLineSymb          = 35, // @v12.3.3 string Символ строки ввода, связанной с комбо-боксом 
		//
		cucmSymbolIdent         = 36, // @v12.3.3 uint32 Идентификатор, ассоциированный с символом элемента. 
			// Префикс cucm подразумевает, что константа может быть использована для областей разных видов.
			// Инициирующая ситуация, потребовавшая ввод этой константы - небоходимость сопоставить с элементами пользовательского 
			// интерфейса числовые идентификаторы, полученные из заголовчного файла по символу. Использовать для этого 
			// основной иденти DlScope не удалось из-за требования сквозной уникальности DlScope::SdRecord::ID.
		cuifCbLineSymbIdent     = 37, // @v12.3.3 uint32 Идентификатор символа строки ввода, связанной с комбо-боксом 
		cuifImageSymb           = 38, // @v12.3.3 string Символ изображения //
		cuifListBoxColumns      = 39, // @v12.3.3 string Определение колонок для ListView //
	};
	struct IfaceBase {
		bool   FASTCALL IsEq(const IfaceBase & rS) const { return (ID == rS.ID && Flags == rS.Flags); }
		bool   FASTCALL operator == (const IfaceBase & rS) const { return (ID == rS.ID && Flags == rS.Flags); }
		enum {
			fDefault    = 0x0001,
			fSource     = 0x0002,
			fRestricted = 0x0004
		};
		DLSYMBID ID;
		uint16 Flags;
	};
	struct Attr {
		uint   A;
		union {
			uint32 Ver;
			uint   UuidIdx; // Индекса GUID'а, хранящегося в DlContext::TempUuidList
		};
	};
	enum {
		ckInput        = UiItemKind::kInput,
		ckStatic       = UiItemKind::kStatic,
		ckPushbutton   = UiItemKind::kPushbutton,
		ckCheckbox     = UiItemKind::kCheckbox,
		ckRadioCluster = UiItemKind::kRadioCluster,
		ckCheckCluster = UiItemKind::kCheckCluster,
		ckCombobox     = UiItemKind::kCombobox,
		ckListbox      = UiItemKind::kListbox,
		ckTreeListbox  = UiItemKind::kTreeListbox,
		ckFrame        = UiItemKind::kFrame,
		ckRadiobutton  = UiItemKind::kRadiobutton,
		ckLabel        = UiItemKind::kLabel
	};

	static int FASTCALL ResolvePropName(const char * pName);
	static int FASTCALL GetPropSymb(int propId, SString & rSymb);
	static int FASTCALL GetKindSymb(int kind, SString & rSymb); // @v12.2.10
	static SString & PropListToLine(const StrAssocArray & rPropList, uint tabCount, SString & rBuf);

	DlScope(DLSYMBID id, uint kind, const char * pName, int prototype);
	DlScope(const DlScope &);
	~DlScope();
	DlScope & FASTCALL operator = (const DlScope &);
	int    Copy(const DlScope &, int withoutChilds = 0);
	int    FASTCALL Add(DlScope * pChild);
	//
	// Descr: Удаляет из списка дочерних областей ту, что имеет идентификатор scopeID.
	// Returns:
	//   Количество удаленный дочерних областей.
	//   0 - не найдено ни одной дочерней области с ИД = scopeID
	//   >1 - найдено и удалено более одной дочерней области с ИД = scopeID
	//
	int    Remove(DLSYMBID scopeID);
	//
	// Descr: Возвращает true если структура имеет в своем составе только тривиальные поля, и false, если
	//   в составе структуры есть сложные поля (SString etc).
	//   Фактически, функция вводится из-за того, что я собираюсь ввести в состав структур записей таблиц
	//   баз данных "хвостатые" поля переменной длинны. Для начала SString а дальше посмотрим.
	//
	bool   IsFlatDataStruct() const;
	DLSYMBID GetId() const;
	DLSYMBID GetBaseId() const;
	const  SString & GetName() const;
	int    CheckDvFlag(long) const;
	int    FASTCALL IsKind(const uint) const;
	uint   GetKind() const;
	//
	// Descr: Возвращает номер версии области видимости. Используется для областей вида
	//   DlScope::kIClass и DlScope::kLibrary.
	//   Структура значения следующая: LoWord - major, HiWord - minor
	//
	uint32 GetVersion() const;
	const  DlScope * GetOwner() const;
	const  DlScopeList & GetChildList() const; // GetRecList
	//
	// Descr: Опции функций перебора (GetChildList)
	//
	enum {
		srchfRecursive = 0x0001, // Просматривать рекурсивно все области
		srchfTopLevel  = 0x0002  // Просматривать рекурсивно области, но имеющие вида kind до тех пор, пока не встретится область
			// заданного вида. Внутри этой области уже не искать!
	};
	const  DlScope * GetFirstChildByKind(int kind, int recursive) const;
	int    GetChildList(int kind, int recursive, LongArray * pList) const;
	bool   FASTCALL IsChildOf(const DlScope * pOwner) const;
	int    EnumChilds(uint * pIdx, DlScope ** ppScope) const;
	int    EnumInheritance(uint * pIdx, const DlScope ** ppScope) const;
	void   SetupTitle(uint kind, const char * pName);
	int    IsPrototype() const;
	void   ResetPrototypeFlag();
	int    GetQualif(DLSYMBID id, const char * pDiv, int inverse, SString & rBuf) const;
	int    SetDeclList(const StringSet * pSet);
	int    SetInheritance(const DlScope * pBase, DlContext * pCtx);
	//
	// Descr: Инициализирует указатели P_Base этой и всех дочерних областей
	//   в соответствии со значением поля BaseId. Функция доолжна быть вызвана
	//   после загрузки списка областей из хранилища, поскольку в хранилище держится только
	//   идентификатор базовой области.
	//
	int    FASTCALL InitInheritance(const DlScope * pTopScope); // @recursion
	DlScope * SearchByName(uint kind, const char * pName, DLSYMBID * pParentID);
	const  DlScope * SearchByName_Const(uint kind, const char * pName, DLSYMBID * pParentID) const;
	DlScope * SearchByID(DLSYMBID id, DLSYMBID * pParentID);
	const  DlScope * SearchByID_Const(DLSYMBID id, DLSYMBID * pParentID) const;
	const  DlScope * GetBase() const;
	DLSYMBID EnterScope(DLSYMBID parentId, DLSYMBID newScopeID, uint kind, const char * pName);
	int    LeaveScope(DLSYMBID scopeId, DLSYMBID * pParentID);
	int    FASTCALL SetRecord(const DlScope * pRec);
	int    FASTCALL SetRecList(const DlScopeList * pList);
	void   FASTCALL AddFunc(const DlFunc *);
	int    GetFuncListByName(const char * pName, LongArray * pList) const;
	uint   GetFuncCount() const;
	int    GetFuncByPos(uint pos, DlFunc * pFunc) const;
	int    EnumFunctions(uint * pI, DlFunc * pFunc) const;
	int    FASTCALL Write(SBuffer & rBuf) const;
	int    FASTCALL Read(SBuffer & rBuf);
	bool   FASTCALL IsEq(const DlScope & rPat) const;
	void   SetFixDataBuf(void * pBuf, size_t size, int clear = 0);
	void * FASTCALL GetFixDataPtr(size_t offs) const;
	//
	// Descr: Устанавливает атрибут Attr. Атрибут представлен флагом и, возможно, дополнительным
	//   значением. Функция является аддитивной и, в общем случае, не коммутативной.
	//
	void   FASTCALL SetAttrib(const Attr &);
	int    GetAttrib(uint attrFlag /* DlScope::sfXXX */, Attr * pAttr) const;
	//
	// Descr: Добавляет константу, соответствующую идентификатору id в структуру ConstList.
	// ARG(id      IN): Идентификатор константы. Все допустимые иджентификаторы должны быть
	//   перечислены в enum DlScope::COption.
	// ARG(rConst  IN): Константа. Параметр должен быть предварительно сформирован вызовом
	//   DlContext::AddConst().
	// ARG(replace IN): Если этот параметр !0 и в списке уже присутсвует константа с идентификатором id,
	//   то ее значение будет заменено на rConst. Если replace == 0 и в списке уже есть константа
	//   с заданным идентификатором, то функция вернет ошибку -1.
	// Returns:
	//   >0 - функция успешно добавила указанную константу
	//   <0 - константа id уже находится в списке
	//    0 - error
	//
	int    AddConst(COption id, const CtmExprConst & rConst, int replace);
	int    GetConst(COption id, CtmExprConst * pConst) const;
	CtmExprConst FASTCALL GetConst(COption id) const;
	int    AddFldConst(uint fldID, COption id, const CtmExprConst & rConst, int replace);
	int    GetFldConst(uint fldID, COption id, CtmExprConst * pConst) const;
	CtmExprConst GetFldConst(uint fldID, COption id) const;
	//
	// Функции управления базовыми интерфейсами класса (kIClass)
	//
	int    AddIfaceBase(const IfaceBase *);
	uint   GetIfaceBaseCount() const;
	int    GetIfaceBase(uint, IfaceBase *) const;
	int    AddDbIndexSegment(const char * pFieldName, long options);
	long   GetDbIndexSegOptions(uint pos) const;
#ifdef DL600C // {
	int    AddTempFldConst(COption id, const CtmExprConst & rConst);
	int    AcceptTempFldConstList(uint fldID);
	int    AcceptBrakPropList(const CtmPropertySheet & rS); // @v11.0.4
	void   InitLocalIdCounter(DLSYMBID initVal) { LastLocalId = initVal; }
	DLSYMBID GetLocalId() { return ++LastLocalId; }
#endif
private:
	uint   Kind;
	uint   ScFlags;        // DlScope::sfXXX
	uint   DvFlags;        // Поле флагов, используемое порожденными классами
	DLSYMBID BaseId;       // Идентификатор базовой области (от которой эта область унаследована)
	DLSYMBID ParentId;     // Идентификатор родительской области (в которую эта область вложена)
	uint32 Version;        // Версия области (kIClass, kLibrary)
	const  DlScope * P_Parent; // @transient Область, в которую вложена данная область
	const  DlScope * P_Base;   // @transient Область, наследуемая данной областью
	DlScopeList ChildList;
	DlFuncPool FuncPool;
	TSVector <IfaceBase> * P_IfaceBaseList;    // Список интерфейсов, поддерживаемых объектом (только для kIClass)
	//
	// Descr: Константа, хранящая опцию
	//
	struct CItem { // @flat
		int32  I;
		CtmExprConst C;
	};
	//
	// Descr: Константа, хранящая значение поля
	//
	struct CfItem {
		uint32 FldID;
		int32  I;
		CtmExprConst C;
	};
	TSVector <CItem> CList;      // Список констант, используемый для хранения специфических опций
	TSVector <CfItem> CfList;    // Список констант, ассоциированных с полями
	LongArray * P_DbIdxSegFlags; // Список флагов сегментов индекса таблицы базы данных (только для kDbIndex)
	SBaseBuffer FixDataBuf;      // @transient Буфер записи для фиксированных полей (не формул)
		// Экземпляр класса DlScope не владеет указателем FixDataBuf.P_Buf, по-этому деструктор не вызывает FixDataBuf.Destroy()
	TSVector <CfItem> TempCfList;
	DLSYMBID LastLocalId;
};
//
//
//
struct CtmToken {
	//
	// Descr: Перечисление определяет дополнительные коды значений, не входящие в список токенов,
	//   определяемых в dl600c.l.
	//   Эти коды начинаются с 10001.
	//
	enum {
		acFirstAddendumCodeStub = 10000,
		acUiCoord,
		acViewAlignment,
		acViewGravity,
		acBoundingBox,
		acLayoutItemSizeEntry,
		acLayoutItemSize,
		acBoundingBoxPair, // @v12.2.9
	};
	void   Init();
	void   Destroy();
	int    Create(uint code);
	int    Create(uint code, const char * pStr);
	int    Create(uint code, DLSYMBID id);
	int    AddStringToSet(const char * pStr);
	CtmToken & Copy(const CtmToken & rS);
	bool   IsEmpty() const;
	//
	// Descr: Возвращает true если элемент является идентификатором.
	//
	bool   IsIdent() const;
	bool   IsString() const;
	bool   IsIdentSet() const; // @v12.3.5
	double GetDouble(uint * pCastFlags) const;
	float  GetFloat(uint * pCastFlags) const;
	int    GetInt(uint * pCastFlags) const;

	uint32 Code; // @v11.7.8 uint16-->uint32
	union {
		char   C;
		uchar  CU;
		int    I;
		uint   IU;
		long   IL;
		ulong  ILU;
		float  F;
		double FD;
		LDBL   FDL;
		LDATE  D;
		LTIME  T;
		DLSYMBID ID;
		char * S;         // oneof4(Code, T_IDENT, T_AT_IDENT, T_CONST_STR, T_FMT)
		StringSet * P_Ss; // @v12.3.5 Для списка идентификаторов или строк
		S_GUID_Base Uuid;
		SColorBase Color; // @v11.0.4
		int    I2[2];
		int    I4[4];
		UiCoord UIC; // @v11.0.4
		UiRelPoint PT; // @v11.0.4
		UiRelRect Rect; // @v11.0.4
	} U;
};

struct CtmVar {
	DLSYMBID ScopeID;
	uint   Pos; // @#[0..] Позиция поля в области видимости
};

struct CtmFunc { // @noctr
	CtmFunc & Z();
	DLSYMBID ScopeID;
	uint   Pos; // @#[0..] Позиция функции в области видимости
};

class CtmExpr {
public:
	enum {
		kEmpty = 0,
		kConst = 1,
		kList,
		kVar,      // Разрешенная переменная //
		kFunc,     // Разрешенная функция //
		kOp,       // Неразрешенный оператор
		kFuncName, // Неразрешенное имя функции
		kVarName   // Неразрешенное имя переменной
	};
	struct OpTypCvt {
		uint   Op;
		DLSYMBID ToTyp;
	};
	struct OpRef {
		uint   Op;
		DLSYMBID Typ;
	};
	void   FASTCALL Init(int kind = 0);
	void   FASTCALL Init(const CtmExprConst & c);
	void   FASTCALL Init(const CtmVar & v);
	void   InitUnaryOp(uint op, const CtmExpr & a);
	void   InitBinaryOp(uint op, const CtmExpr & a1, const CtmExpr & a2);
	void   InitTypeConversion(const CtmExpr & a, DLSYMBID toType);
	void   InitRefOp(DLSYMBID type, const CtmExpr & a1);
	void   FASTCALL InitVar(const char * pName);
	void   FASTCALL InitVar(const CtmVar &);
	void   InitFuncCall(const char * pFuncName, const CtmExpr & a);
	void   Destroy();
	int    FASTCALL IsResolved(int inDepth) const;
	int    FASTCALL Append(const CtmExpr & a);
	int    FASTCALL AddArg(const CtmExpr & a);
	uint   GetListCount() const;
	uint   GetArgCount()  const;
	CtmExpr * FASTCALL GetArg(uint pos) const;
	int    Pack(SBuffer *, size_t * pOffs) const;
	int    Unpack(SBuffer *);
	int    Pack(SString & rBuf) const;
	int    Unpack(SStrScan &);
	int    GetKind() const { return Kind; }
	void   FASTCALL SetType(DLSYMBID typeID);
	DLSYMBID GetOrgTypeID() const { return TypID; }
	DLSYMBID GetTypeID() const;
	int    FASTCALL SetImplicitCast(DLSYMBID toTypID);
	DLSYMBID GetImplicitCast() const { return ToTypStdCvt; }
	int    SetResolvedVar(const CtmVar & rVar, DLSYMBID typeID);
	int    FASTCALL SetResolvedFunc(const CtmFunc & rFunc);
private:
	uint16 Kind;
	uint16 Flags;
	DLSYMBID TypID;
	DLSYMBID ToTypStdCvt;  // Если !0, то к терму применяется преобразование типов TypID-->ToTypStdCvt
public:
	union {
		CtmExprConst C;
		CtmFunc F;
		CtmVar V;
		uint   Op;
		OpTypCvt Cvt;
		OpRef  Ref;
		char * S;
	} U;
	CtmExpr * P_Next;      // Следующий элемент (по горизонтали). Например, в списке
	CtmExpr * P_Arg;       // Список аргументов
};

struct CtmDclr {
	void   Init();
	void   Destroy();
	int    Copy(const CtmDclr & rDclr);
	void   AddDim(uint dim);
	void   AddPtrMod(uint ptrMod, uint modifier = 0);
	void   AddDecimalDim(uint dec, uint precision);

	CtmToken Tok;
	CtmToken Alias;        // Алиас для экспортных полей
	CtmToken Format;       // Формат для экспортных полей
	SV_Uint32 DimList;     // Список размерностей массива
	SV_Uint32 PtrList;     // Elem: LowWord - STypEx::modXXX, HiWord - modifier

	DLSYMBID TypeID;
	uint16 IfaceArgDirMod; // Модификатор направления аргумента функции интерфейса: fArgIn, fArgOut
	uint16 Reserve;
};

struct CtmDclrList {
	void   Init();
	void   Destroy();
	int    Add(const CtmDclr & rDclr);

	TSCollection <CtmDclr> * P_List;
};

struct CtmFuncDclr {
	void   Init();
	void   Destroy();

	DlFunc * P_Fn;
};
//
//
//
struct CtmProperty {
	void   Init();
	void   Destroy();
	CtmProperty & Copy(const CtmProperty & rS);

	CtmToken Key;
	CtmToken Value;
};

struct CtmPropertySheet {
	void   Init();
	void   Destroy();
	int    Add(const CtmProperty & rItem);
	int    Add(const CtmPropertySheet & rList);

	TSCollection <CtmProperty> * P_List;
};

class DlMacro {
public:
	DlMacro();
	void   Add(const char * pSymb, const char * pResult);
	int    Subst(const char * pSymb, SString & rResult) const;
private:
	StringSet S;
};
//
// Descr: Заголовок скомпилированного бинарного файла DL600
//
struct DlCtxHdr {
	DlCtxHdr();
	int    Check() const;
	char   Signature[4]; // Сигнатура файла "DL6B"
	uint32 Crc32;        // CRC всего файла
	uint8  Reserve[56];  // Зарезервировано
};

struct RtmStack {
	RtmStack();
	~RtmStack();
	int    FASTCALL Init(size_t sz);
	uint   GetCurPos() const;
	void   FASTCALL SetCurPos(size_t p);
	void * FASTCALL GetPtr(size_t p);
	uint   FASTCALL Alloc(size_t sz);
private:
	SBaseBuffer B;
	size_t P;
};

class DlContext {
public:
	struct TypeEntry {
		DLSYMBID SymbID;
		STypEx T;
		int8   Pad1[2]; // @alignment
		char   MangleC; // Символ для идентификации типа параметра в функциях
		int8   Pad2;    // @alignment
	};
	explicit DlContext(int toCompile = 0);
	~DlContext();
	int    FASTCALL Init(const char * pInFileName);
	enum {
		ispcExpData = 1,
		ispcInterface
	};
	int    FASTCALL InitSpecial(int); // Run-time
	const  char * GetInputFileName() const;
	int    GetSymb(DLSYMBID id, SString & rBuf, int prefix) const;
	int    SearchSymb(const char * pSymb, int prefix, DLSYMBID * pID) const;
	//
	// Descr: Возвращает область видимости, соответствующую идентификатору scopeID.
	//   Если checkKind != 0, то функция проверяет, чтобы возвращаемая область имела
	//   указанный вид. Если это не так, то возвращается 0 и устанавливается код ошибки
	//   PPERR_DL6_INVSCOPEKIND
	//
	DlScope * GetScope(DLSYMBID scopeID, int checkKind = 0);
	DlScope * GetCurScope();
	const  DlScope * GetScope_Const(DLSYMBID scopeID, int checkKind = 0) const;
	const  DlScope * GetScopeByName_Const(uint kind, const char * pName) const;
	//
	// Descr: Специальный вариант поиска области, относящейся к виду DlScope::kUiView
	//   со значением константы DlScope::cuifViewKind == UiItemKind::kDialog и
	//   константы DlScope::cucmSymbolIdent == symbolIdent
	//
	const  DlScope * GetDialogScopeBySymbolIdent_Const(uint symbolIdent) const;

	enum {
		crsymfCatCurScope = 0x0001,
		crsymfErrorOnDup  = 0x0002
	};
	int    ResolveVar(DLSYMBID scopeID, int exactScope, const char * pSymb, CtmVar * pVar);
	int    ResolveVar(const DlScope * pScope, int exactScope, const char * pSymb, CtmVar * pVar);
	//
	// Descr: Специальный вариант реализации вызова функции в TDDO-файлах.
	//
	int    ResolveFunc(DlRtm * pRtm, const DlScope * pScope, int exactScope, const char * pFuncName, StrAssocArray & rArgList, SString & rResult, STypEx & rT);
	int    GetFunc(const CtmFunc & rF, DlFunc * pFunc);
	int    GetField(const CtmVar & rV, SdbField * pFld);
	int    GetConstData(const CtmExprConst & rC, void * pBuf, size_t bufLen) const;
	//
	// Descr: Значения, возвращаемые функцией TypeCast
	//
	enum {
		tcrUnable    = -1, // Преобразование невозможно
		tcrError     = 0,  // Ошибка
		tcrEqual     = 1,  // Типы эквивалентны (преобразование не нужно)
		tcrCast      = 2,  // Преобразование возможно без потери точности
		tcrCastSuper = 3,  // Преобразование возможно с увеличением точности
		tcrCastLoss  = 4   // Преобразование возможно с потерей точности
	};
	//
	// Descr: Преобразует тип srcTyp к типу destTyp. Если cvt != 0, то осуществляет
	//   преобразование данных из буфера pSrcData в буфер pDestData. В противном случае
	//   только определяет возможность преобразования.
	// Returns:
	//   -1 - преобразование невозможно
	//    1 - типы srcTyp и destTyp идентичны (преобразование не требуется)
	//    2 - типы не идентичны, но преобразование осуществляется без потери точности
	//    3 - преобразование возможно, но с потерей точности
	//    0 - error
	//
	int    TypeCast(DLSYMBID srcTyp, DLSYMBID destTyp, int cvt, const void * pSrcData, void * pDestData, int * pLoss = 0);
	int    AddStructType(DLSYMBID symbId);
	int    SearchTypeID(DLSYMBID id, uint * pPos, TypeEntry * pEntry) const;
	size_t FASTCALL GetTypeBinSize(DLSYMBID typID) const;
	DLSYMBID SearchSTypEx(const STypEx & rTyp, TypeEntry * pEntry) const;
	DLSYMBID SetDeclType(DLSYMBID typeID);
	TYPEID TypeToSType(DLSYMBID) const;
	int    DemangleType(const char * pTypeStr, STypEx * pTyp, DLSYMBID * pID);
	long   ParseFormat(const char * pStr, TYPEID typ) const; // *
	//
	// Descr: Устанавливает код ошибки и, возможно, дополнительную строку, связанную с ошибкой.
	//   Функция константная, так как для compile-time изменяется DlContext::LastError и
	//   DlContext::AddedMsgString, а для run-time вызывается PPSetError().
	//
	int    SetError(int errCode, const char * pAddedMsg = 0) const;
	enum {
		erfExit = 0x0001, // Завершить работу
		erfLog  = 0x0002  // Записать сообщение об ошибке в журнал
	};
	int    Error(int errCode = 0, const char * pAddedInfo = 0, long flags = erfExit);
	//
	// Compile-time {
	//
	enum {
		cfDebug         = 0x0001, // Отладочный режим компиляции
		cfBinOnly       = 0x0002, // Создавать только бинарные файлы (не генерировать .H, .CPP и прочие файлы для разработки)
		cfSQL           = 0x0004, // Генерировать SQL-скрипт для создания таблиц базы данных
		cfOracle        = 0x0008, // Генерировать SQL-скрипт специфичный для ORACLE для создания таблиц базы данных
		cfGravity       = 0x0010, // Генерировать gravity интерфейсы
		cfMySQL         = 0x0020, // Генерировать SQL-скрипт специфичный для MySQL для создания таблиц базы данных
		cfSqLite        = 0x0040, // Генерировать SQL-скрипт специфичный для SqLite для создания таблиц базы данных
		cfStyloQAndroid = 0x0080, // @v11.1.2 Генерировать модули для проекта StyloQ Android (если другие опции это предполагают)
		cfSkipBtrDict   = 0x0100, // @v11.9.4 Не создавать словарь btrieve
	};
	int    Compile(const char * pInFileName, const char * pDictPath, const char * pDataPath, long cflags);
	//
	// Descr: Вызывается в ответ на директиву cinclude "file_name" в исходном файле.
	//
	int    AddCInclude(const char * pCIncFileName); // @v12.3.3
	int    FindImportFile(const char * pFileName, SString & rPath);
	int    SetInheritance(DLSYMBID scopeID, DLSYMBID baseID);
	int    MangleType(DLSYMBID id, const STypEx &, SString & rBuf) const;
	int    AddType(const char * pName, TYPEID stypId, char mangleC = 0);
	DLSYMBID MakeSizedString(DLSYMBID typeID, size_t s);
	int    AddDeclaration(DLSYMBID typeId, const CtmDclr & rDclr, CtmExpr * pExpr);
	int    ResolveVar(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr);
	int    ResolveFunc(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr);
	int    ResolveExpr(DLSYMBID scopeID, DLSYMBID callerScopeID, int exactScope, CtmExpr * pExpr, int dontResolveNext = 0);
	DLSYMBID CreateSymb(const char * pSymb, int prefix, long flags);
	DLSYMBID CreateSymbWithId(const char * pSymb, DLSYMBID id, int prefix, long flags);
	//
	// Descr: Возвращает уникальный внутри этого экземпляра идентификатор.
	//
	DLSYMBID GetNewSymbID();
	long     GetUniqCntr();
	DLSYMBID SetDeclTypeMod(DLSYMBID ofTyp, int mod /* STypEx::modXXX */, uint arrayDim = 0);
	DLSYMBID EnterScope(uint scopeKind, const char * pName, DLSYMBID scopeId, const SV_Uint32 * pAttrList);
	int    LeaveScope();
	//
	// Descr: Реализует старт декларации области view.
	//   Специальная функция (вместо обычной EnterScope) понадобилась из-за необходимости
	//   тонко верифицировать уникальность символов: области верхнего уровня должны иметь
	//   уникальные символы в то время как внутренние области должны иметь символы, уникальные
	//   лишь внутри собственной области верхнего уровня (пропуская родительскую, если она так
	//   же является вложенной).
	//
	DLSYMBID EnterViewScope(const char * pSymb);
	DLSYMBID EnterDialogScope(const char * pSymb);
	int    PushScope();
	int    PopScope();
	int    CompleteExportDataStruc();
	void   AddMacro(const char * pMacro, const char * pResult);
	int    GetMacro(const char * pMacro, SString & rResult) const;
	int    FASTCALL GetDotFunc(CtmFunc * pF);
	int    FASTCALL GetRefFunc(CtmFunc * pF);
	int    AddConst(const char * pTypeSymb, const void * pData, size_t dataSize, CtmExprConst * pResult);
	int    AddConst(const void * pData, size_t dataSize, CtmExprConst * pResult);
	int    AddConst(const char * pData, CtmExprConst * pResult);
	int    AddConst(const SString & rData, CtmExprConst * pResult);
	int    AddConst(uint32 data, CtmExprConst * pResult);
	int    AddConst(int32 data, CtmExprConst * pResult);
	int    AddConst(int8 data, CtmExprConst * pResult);
	int    AddConst(int64 data, CtmExprConst * pResult);
	int    AddConst(int16 data, CtmExprConst * pResult);
	int    AddConst(float data, CtmExprConst * pResult);
	int    AddConst(double data, CtmExprConst * pResult);
	int    AddConst(LDATE data, CtmExprConst * pResult);
	int    AddConst(LTIME data, CtmExprConst * pResult);
	void   AddStrucDeclare(const char * pDecl);
	int    AddFuncDeclare(const CtmDclr & rSymb, const CtmDclrList & rArgList, int propDirParam = 0);
	int    AddPropDeclare(CtmDclr & rSymb, int propDirParam);
	int    AddEnumItem(const CtmToken & rSymb, int useExplVal, uint val);
	int    AddTypedef(const CtmToken & rSymb, DLSYMBID typeID, uint tdFlags);
	//
	// Descr: Инициализирует имя нового индекса таблицы базы данных.
	//   Если имя уже определено в параметре pIdxName, то проверяет его уникальность внутри таблицы.
	//   Если pIdxName == 0 || pIdxName[0] == 0, то присваивает индексу имя Key#, где # - номер [0..]
	//
	int    InitDbIndexName(const char * pIdxName, SString & rBuf);
	int    AddDbIndexSegmentDeclaration(const char * pFieldName, long options);
	int    ResolveDbIndexSegFlag(long flags, const char * pSymb, long * pResultFlags);
	int    ResolveDbIndexFlag(const char * pSymb);
	int    ResolveDbFileDefinition(const CtmToken & rSymb, const char * pConstStr, int constInt);
	int    Write_Code();
	int    CreateDbDictionary(const char * pDictPath, const char * pDataPath, bool skipBtrDictCreation, const LongArray * pSqlServerTypeList/*SqlServerType sqlst*/);
	//
	// Descr: Создает описание диалогового управляющего элемента в текущей области видимости.
	// Returns:
	//   ID созданного элемента описания.
	//   0  - ошибка.
	//
	uint   AddUiCtrl(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect);
	uint   AddUiButton(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rCmdSymb);
	uint   AddUiListbox(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rColumns);
	uint   AddUiCluster(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect);
	int    AddUiClusterItem(const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rDescr);
	int    AddTempFldProp(const CtmToken & rSymb, long val);
	int    AddTempFldProp(const CtmToken & rSymb, double val);
	int    AddTempFldProp(const CtmToken & rSymb, const char * pStr);
	int    AddTempFldProp(const CtmToken & rSymb, const void * pData, size_t sz);
	int    ApplyBrakPropList(DLSYMBID scopeID, const CtmToken * pViewKind, DLSYMBID typeID, const CtmPropertySheet & rS);
	//
	// } Compile-time
	// Run-time {
	//
	DlRtm * FASTCALL GetRtm(DLSYMBID); // run-time
	DlRtm * FASTCALL GetRtm(const char * pName); // run-time
	//
	// COM-инфраструктура
	//
	DLSYMBID FASTCALL SearchUuid(const S_GUID_Base & rUuid) const;
	int    GetUuidByScopeID(DLSYMBID scopeID, S_GUID * pUuid) const;
	int    GetInterface(const S_GUID_Base & rIID, DLSYMBID * pID, const DlScope ** ppScope) const;
	//
	// Descr: Перебирает интефейсы класса pCls.
	// Returns:
	//   >0 - интерфейс с индексом *pI успешно извлечен
	//   <0 - по индексу *pI и более интерфейсов нет
	//    0 - error извлечения интерфейса (*pI меньше, чем количество интерфейсов)
	//
	int    EnumInterfacesByICls(const DlScope * pCls, uint * pI, DlScope::IfaceBase * pIfb, const DlScope ** ppIfaceScope) const;
	//
	// Descr: Регистрирует (дерегистрирует) в реестре класс pCls
	// ARG(pCls  IN): @#{pCls->IsKind(DlScope::kIClass)!=0}
	//   указатель на область видимости, соответствующей классу
	// ARG(unreg IN): если !0, то класс pCls дерегистрируется, в противном случае - регистрируется //
	//
	int    RegisterICls(const DlScope * pCls, int unreg); // @recusion
	int    RegisterTypeLib(const DlScope * pCls, int unreg); // @v5.4.5 AHTOXA
	int    CreateDlIClsInstance(const DlScope *, SCoClass ** ppInstance) const;
	int    CreateDlIClsInstance(const S_GUID & rClsUuid, SCoClass ** ppInstance) const;
	int    CreateDlRtmInstance(DlScope *, DlRtm ** ppInstance);
	int    CreateDlRtmInstance(const char * pName, DlRtm ** ppInstance);
	//
	// } Run-time
	//
	int    UnpackFormula(DlScope *);
	CtmExpr * GetFormula(const DlScope *, uint fldPos);
	uint   AllocStackType(DLSYMBID typeID, TypeEntry * pTe);
	int    FASTCALL ReleaseStack(uint pos);
	int    FASTCALL FreeStack(uint sp);
	void * FASTCALL GetStackPtr(uint sp);
	int    EvaluateExpr(DlRtm * pRtm, const DlScope *, DlRtm * pCallerRtm, const DlScope * pCallerScope, CtmExpr * pExpr, size_t sp); // run-time
	const  DlScope * GetEvaluatedVarScope(const DlScope * pScope, const CtmExpr * pExpr) const; // run-time @obsolete
	const  DlScope * GetEvaluatedVarScope(const DlScope * pScope, DLSYMBID targetScopeID) const; // run-time
	//
	// Descr: Создает спецификацию таблицы базы данных, пригодную для формирования по ней
	//   записи в словаре базы данных и создания файла данных.
	// Returns:
	//   !0 - спецификация таблицы успешно создана
	//   0 - error
	//
	int    LoadDbTableSpec(DLSYMBID scopeID, DBTable * pTbl, int format) const;
	int    LoadDbTableSpec(const char * pName, DBTable * pTbl, int format) const;
	int    CreateNewDbTableSpec(const DBTable * pTbl);
	int    DropDbTableSpec(const char * pName);
	int    GetDbTableSpecList(StrAssocArray * pList) const;
	int    Test_ReWr_Code(const DlContext & rPattern);
	int    GetDialogList(StrAssocArray * pList) const;
	//
	// Descr: Возвращает указатель на блок лейаута области pScope, определенного константой DlScope::cuifLayoutBlock.
	//
	bool   GetLayoutBlock(const DlScope * pScope, DlScope::COption propId, SUiLayoutParam * pLp) const;
	bool   GetConst_String(const DlScope * pScope, DlScope::COption propId, SString & rBuf) const;
	bool   GetConst_Int(const DlScope * pScope, DlScope::COption propId, int & rValue) const;
	bool   GetConst_Uint32(const DlScope * pScope, DlScope::COption propId, uint32 & rValue) const;

	UUIDAssocArray TempUuidList; // @transient Временный список GUID'ов используемый при компиляции
	int    Test();
private:
	DlScope * Helper_GetScope(DLSYMBID id, const DlScope * pScope, int kind) const;
	DLSYMBID  Helper_CreateSymb(const char * pSymb, DLSYMBID newId, int prefix, long flags);
	//
	// Descr: Опции функции Helper_GetScopeList
	//
	enum {
		gslfRecursive = 0x0001, // Просматривать рекурсивно все области
		gslfTopLevel  = 0x0002  // Просматривать рекурсивно области, но имеющие вида kind до тех пор, пока не встретится область
			// заданного вида. Внутри этой области уже не искать!
	};
	int    Helper_GetScopeList(int kind, int recursive, StrAssocArray * pList) const;
	DlScope * GetCurDialogScope();
	int    GetUiSymbSeries(const char * pSymb, SString & rSerBuf, DLSYMBID * pId);
	bool   Helper_AddBFunc(const char * pFuncName, uint implID, const char * pRetType, va_list pArgList);
	bool   CDECL AddBOp(int op, uint implID, const char * pRetType, ...);
	bool   CDECL AddBFunc(const char * pFuncName, uint implID, const char * pRetType, ...);
	int    AddBCmpOps(uint implID, const char * pType);
	int    AddBCvt(uint implID, int loss, const char * pRetType, const char * pSrcType);
	int    FASTCALL BuiltinOp(const DlFunc * pF, SV_Uint32 *);
	int    GetFuncName(int, const CtmExpr * pExpr, SString & rBuf);
	int    SearchVarInChildList(const DlScope * pScope, uint childKind, const char * pSymb, CtmVar * pVar);
	int    ProcessQuestArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList);
	void   InitFileNames(const char * pInFileName);
	int    Helper_LoadDbTableSpec(const DlScope *, DBTable * pTbl, int format) const;
	DLSYMBID Helper_EnterViewScope(uint scopeKind, const char * pSymb);
	//
	// Descr: Форматы вывода наименований классов
	//
	enum {
		clsnfCPP = 0,           // Для CPP-файлов
		clsnfRegister,          // Для регистрации в системном реестре
		clsnfRegisterNoVersion, // Для регистрации в системном реестре без номера версии
		clsnfFriendly           // Дружественное имя для регистрации в системном реестре
	};
	//
	// Descr: Выводит имя области видимости как имя класса в соответствии с форматом clsnf
	// ARG(pStruc IN): Структура, определяющая класс
	// ARG(clsnf  IN): Формат вывода наименования класса.
	// ARG(cflags IN): Флаги компиляции. Могут влиять на имя класса.
	// ARG(rBuf  OUT): Буфер, в котором возвращается сформированное наименование класса.
	//
	void   MakeClassName(const DlScope * pStruc, int clsnf, long cflags, SString & rBuf) const;
	//
	// Compile-time {
	//
	int    AddCvtFuncToArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList) const;
		// @<<DlContext::ResolveFunc
	int    IsFuncSuited(const DlFunc & rFunc, CtmExpr * pExpr, LongArray * pCvtArgList);
	int    MakeDlRecName(const DlScope * pRec, int instanceName, SString & rBuf) const;
	void   Write_C_FileHeader(Generator_CPP & gen, const char * pFileName);
	int    Write_C_DeclFile(Generator_CPP & gen, const DlScope & rScope, long cflags);  // @recursion
	int    Write_C_ImplFile(Generator_CPP & gen, const DlScope & rScope, long cflags);  // @recursion
	int    Write_C_AutoImplFile(Generator_CPP & gen, const DlScope & rScope, StringSet & rSs, long cflags); // @recursion
	int    Write_WSDL_File(const char * pFileName, const DlScope & rScope);
	int    Write_DialogReverse();
	int    Write_UiView(const DlScope * pScope, uint indentTabCount, SString & rOutBuf);
	//
	// } Compile-time
	//
	enum {
		ffIDL = 1,     // Для IDL-файла
		ffH_Iface,     // Прототип интерфейса в H-файле
		ffH_Imp,       // Прототип реализации интерфейса в H-файле
		ffCPP_Iface,   // Реализация интерфейса в CPP-файле
		ffCPP_Imp,     // Реализация в CPP-файле
		ffCPP_VTbl,    // Элемент таблицы виртуальных функций в CPP-файле
		ffCPP_CallImp, // Вызов реализации из интерфейсной функции
		ffH_GravityIface,   // Прототип интерфейса Gravity в H-файле
		ffH_GravityImp,     // Прототип реализации интерфейса Gravity в H-файле
		ffCPP_GravityIface, // Реализация интефейса Gravity в CPP-файле
		ffCPP_GravityImp,   // Реализация Gravity в CPP-файле
	};
	int    Write_Func(Generator_CPP & gen, const DlFunc & rFunc, int format, const char * pForward = 0);
	int    Write_C_ImplInterfaceFunc(Generator_CPP & gen, const SString & rClsName, DlFunc & rFunc, long cflags);
	int    Write_IDL_Attr(Generator_CPP & gen, const DlScope & rScope);
	int    Write_IDL_File(Generator_CPP & gen, const DlScope & rScope); // @recursion
	void   Write_DebugListing();
	int    Write_Scope(int indent, SFile & rOutFile, const DlScope & rScope); // @recursion
	int    Format_TypeEntry(const TypeEntry & rEntry, SString & rBuf); // @recursion
	int    Format_Func(const DlFunc & rFunc, long options, SString & rBuf);
	int    FormatVar(CtmVar v, SString & rBuf) const;
	enum {
		fctfSourceOutput  = 0x0001, // Форматировать тип для вывода в C++
		fctfIDL           = 0x0002, // Форматировать тип для вывода в IDL
		fctfIfaceImpl     = 0x0004, // Форматировать тип для функций реализации интерфейсов
		fctfInstance      = 0x0008, // Форматирование для экземпляров типов. Основное отличие -
			// строки представлены не ссылками, а собственно экземплярами (SString вместо SString&)
		fctfResolveTypeID = 0x0010  // Функция должна вызвать SearchTypeID для идентификации типа typeID
	};
	int    Format_C_Type(DLSYMBID typeID, STypEx & rTyp, const char * pFldName, long flags, SString & rBuf);
	int    IsInterfaceTypeConversionNeeded(const STypEx & rTyp);
	int    Read_Code();
	//
	struct TypeDetail { // Compile-time
		TypeDetail();
		~TypeDetail();
		int    IsInterfaceTypeConversionNeeded() const;
		SV_Uint32 DimList;
		SV_Uint32 PtrList; // Elem: LowWord - STypEx::modXXX, HiWord - modifier
		DLSYMBID TerminalTypeID;
		STypEx T;
	};
	int    UnrollType(DLSYMBID typeID, const STypEx & rTyp, TypeDetail * pTd); // Compile-time
	enum {
		fCompile = 0x0001
	};
	long   Flags;
	LAssocArray AdjRetTypeProcList; // Compile-time
	DLSYMBID LastSymbId;
	long   UniqCntr;                // Сквозной (в пределах компилируемого файла) счетчик, используемый для формирования уникальный значений
	SymbHashTable Ht/*Tab*/;
	StringSet CurDeclList;
	CtmConstList ConstList;         // @persistent
	TSVector <TypeEntry> TypeList;  // @persistent
	UUIDAssocArray UuidList;        // @persistent
	SStack ScopeStack;              //
	DlScope Sc;          // @persistent Глобальная область видимости (все остальные области являются членами этой области)
	DLSYMBID CurScopeID; // Compile-time Область видимости, которая является текущий во время компиляции
	DlMacro * P_M; // Compile-time
	CtmFunc F_Dot; // Compile-time Функция '.' (что бы не тратить каждый раз время на ее определение)
	CtmFunc F_Ref; // Compile-time Функция '@()' (что бы не тратить каждый раз время на ее определение)
	SString InFileName;
	SString CDeclFileName;
	SString CImplFileName;
	SString BinFileName;
	SString LogFileName;
	RtmStack S;      // Run-time
#ifdef DL600C // {
	static int LastError;
	static SString AddedMsgString;
	//
	// Структура, используемая для хранения сгенерированных GUID'ов с целью использования //
	// при следующем разборе исходного файла.
	//
	struct SyncUuidAssoc {
		uint   NamePos;
		S_GUID Uuid;
	};
	int    StoreUuidList();
	int    RestoreUuidList();
	int    SetupScopeUUID(DLSYMBID scopeID, const char * pName, const S_GUID * pForceUUID);
	//
	//
	//
	class CPreprocBlock { // @v12.3.3 
	public:
		CPreprocBlock();
		~CPreprocBlock();
		bool   ResolveSymbol(const char * pSymb, SString * pResult, int * pResultInt);
		int    AddCInclude(const char * pThisFilePath, const char * pCIncFileName);
	private:
		enum {
			stInited = 0x0001,
			stError  = 0x0002
		};
		void * P_Opaq;
		StringSet CIncludeFileList; // Список h-файлов, включенных в компиляцию для получения идентификаторов символов.
		uint   State;
	};

	TSVector <SyncUuidAssoc> SyncUuidList; // @transient Compile-time 
	StringSet SyncUuidNameList;
	LAssocArray UiSymbAssoc; // Ассоциации символов элементов UI с символами сериий.
	//StringSet CIncludeFileList; // @v12.3.3 Список h-файлов, включенных в компиляцию для получения идентификаторов символов.
	CPreprocBlock CppBlk; // @v12.3.3
#else
	TSVector <uint> Pss; // Run-time "Pushed String Stack" Стэк позиций строк, распределенных на стеке StP.
		// Нужен для того, чтобы при освобождении стекового пространства освобождать соответствующие строки
	SStringPool StP;                     // Run-time
	TSCollection <DlRtm> RtmList;        // Run-time
	class UnpFormArray : private SArray {
	public:
		UnpFormArray();
		~UnpFormArray();
		int    Add(DLSYMBID scopeID, uint fldPos, CtmExpr * pExpr);
		CtmExpr * Get(DLSYMBID scopeID, uint fldPos);
	private:
		virtual void FASTCALL freeItem(void *);
	};
	UnpFormArray UnpFormList;
#endif // }
};

extern DlContext DCtx; // Compile-time
typedef DLSYMBID (*AdjRetTypeProc)(DlContext * pCtx, const CtmExpr * pExpr);

//#ifdef DL600C
typedef long PPIterID;
//#endif
#define DEFAULT_ITER    -1
#define DEFAULT_HEAD     1
#define GETARYSTYPE(typ) ((typ)&0x007f)
#define GETISARY(typ)    ((typ)&0x0080)

struct PPFilt {
	PPFilt();
	explicit PPFilt(long id);
	explicit PPFilt(void * ptr);

	long   ID;
	void * Ptr;
};

struct PView {
	explicit PView(long id);
	explicit PView(void * ptr);

	long   ID;
	void * Ptr;
};

struct FormatSpec {
	short  len;
	short  prec;
	long   flags;
};

class DlRtm {
	friend class PPView; // export functions
public:
	enum {
		rscDefHdr  = 1,
		rscDefIter = 2
	};
	DlRtm(DlContext * pCtx, DlScope * pScope);
	virtual ~DlRtm();
	virtual int InitData(PPFilt &, long rsrv = 0);
	virtual int InitIteration(long iterId, int sortId, long rsrv = 0);
	virtual int NextIteration(long iterId/*, long rsrv = 0*/);
	//
	// Descr: Реализует функцию изменения данных, соответствующих структуре,
	//   в базе данных.
	// ARG(iterId IN): Идентификатор итератора, для которого должны быть
	//   приняты данные.
	//   Если iterId == 0, то принимаются данные заголовочной структуры,
	//   если iterId == -1 (DEFAULT_ITER), то принимаются данные итератора по умолчанию (обычно, единственного).
	// ARG(commit IN): Если commit == 1, то все принятые до этого данные (при значении commit == 0) должны
	//   быть внесены в базу данных.
	// Returns:
	//   >0 - функция отработала успешно.
	//    0 - error.
	//
	virtual int  Set(long iterId, int commit);
	virtual void Destroy(); // @v9.6.4 int-->void
	virtual void EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS);
	DlContext * GetContext() const { return P_Ctx; }
	const  DlScope * GetData() const { return P_Data; }
	const  DlScope * GetHdrScope() const { return P_HdrScope; }
	void * GetFixFieldData(const DlScope * pScope, uint fldPos);
	long   FASTCALL GetIterID(const char * pIterName = 0) const;
	DLSYMBID GetDataId() const { return DataId; }
	int    SetByJSON(const SJson * pJSONDoc, long & ObjId);           // @Muxa
	int    SetByJSON_Helper(const SJson * pNode, SetScopeBlk & rBlk); // @Muxa

	struct ExportParam {
		ExportParam();
		ExportParam(PPFilt & rF, long flags);
		enum {
			fIsView    = 0x0001, // P_F->Ptr указывает на класс, порожденный от PPView
			fInheritedTblNames = 0x0002, // При экспорте в качестве наименования таблиц будет
				// применяться имя родительской структуры
			fDiff_ID_ByScope   = 0x0004, // Если этот флаг установлен, то наименования полей идентификаторов
				// записей в таблицах, соответствующих разным областям будут отличаться.
				// Эта опция необходима из-за того, что единовременно перевести все структуры на
				// различающиеся наименования таких полей невозможно по причине необходимости верификации
				// соответствующих отчетов
			fForceDDF  = 0x0008, // Если в файлах словаря нет необходимости, то все равно создавать такие файлы.
				// Возникновение флага обусловлено тем, что CrystalReports не требует для печати
				// файлов словарей. Однако, они необходимы для формирования и редактирования отчетов в дизайнере.
			fDontWriteXmlDTD   = 0x0010, // Не формировать DTD в XML-файле
			fDontWriteXmlTypes = 0x0020, // Не формировать зону типов в XML-файле
			fCompressXml       = 0x0040, // Сжимать создаваемый XML-файл
			fJsonStQStyle      = 0x0080  // @v11.2.0 При выводе в JSON-формате применять новую структуру (в рамках работы над проектом StyloQ)
		};
		PPFilt * P_F;
		int    Sort;
		long   Flags;
		SCodepageIdent Cp;
		SFileFormat OutputFormat;
		SString DestPath;
		SString Path;
		const void * P_ViewDef; // v10.5.1
	};
	int    Export(ExportParam & rParam);
	int    ExportXML(ExportParam & rParam, SString & rOutFileName);
	int    ExportJson(ExportParam & rParam, SString & rOutFileName);
	SJson * ExportJson(ExportParam & rParam);
	int    PutToXmlBuffer(ExportParam & rParam, SString & rBuf);
	//
	// ARG(cp IN): Кодовая страница вывода. Если cp == cpUndef, то ANSI
	//   Обрабатываются следующие варианты:
	//   cpUTF8 - строки преобразуются в UTF8
	//   cpANSI, cp1251 - строки преобразуются в cpANSI
	//   Все остальные варианты - строки преобразуются в cpANSI
	// ARG(adoptTypes IN): Если true то в json вставляются значения, типизированные в соответствии
	//   с типами, определенными описанием структуры. Если false, то все значения вставляются как строки.
	//
	int    Helper_PutScopeToJson(const DlScope * pScope, SJson * pJsonObj, int cp, bool adoptTypes) const;
	int    Helper_PutItemToJson(ExportParam & rParam, SJson * pRoot);
	int    PutToJsonBuffer(StrAssocArray * pAry, SString & rBuf, int flags);
	int    PutToJsonBuffer(void * ptr, SString & rBuf, int flags);
	int    PutToJsonBuffer(PPView * pV, SString & rBuf, int flags);

	const ExportParam * P_Ep;
protected:
	struct ExtData2 {
		void * Ptr; // @firstmember
		long   IsFirst;
	};
	int    InitFixData(const char * pScopeName, void * pData, size_t dataSize);
	int    InitFixData(int reservedScopeCode, void * pData, size_t dataSize);
	int    FASTCALL FinishRecord(const DlScope * pScope); // @recursion
	int    FASTCALL AssignHeadData(void * pData, size_t dataSize);          // PPALDD compatibility
		// @>>InitFixData
	int    FASTCALL AssignIterData(int one, void * pData, size_t dataSize); // PPALDD compatibility
		// @>>InitFixData
	int    FASTCALL AssignDefIterData(void * pData, size_t dataSize); // @>>InitFixData
	int    FillXmlBuf(const DlScope * pScope, xmlTextWriter * pWriter, StringSet * pDtd, SCodepageIdent cp) const;
	void   FillDTDBuf(const DlScope * pScope, xmlTextWriter * pWriter, const char * pElemName) const;
	//
	// Descr: Стандартный пролог функций InitIteration и NextIteration.
	// Returns:
	//   >0 - функция успешно выполнена.
	//   <0 - функция успешно выполнена. При этом Extra[rID-].IsFirst изменено с 1 на 0.
	//   0  - ошибка
	//
	int    FASTCALL IterProlog(/*PPIterID*/long & rID, int doInit);
	int    Helper_WriteXML(ExportParam & rParam, void * /*xmlWriter*/);
	DlContext * P_Ctx; // @notowned
	DlScope * P_Data;     // @*DlRtm::DlRtm
	DlScope * P_HdrScope; // @*DlRtm::DlRtm
	DLSYMBID DataId;
	int    Valid;   // PPALDD compatibility
	int    SortIdx; // PPALDD compatibility
	SV_Uint32 IterList; // Массив идентификаторов (DLSYMBID) областей видимости, соответствующих
		// итераторам. Функция GetIterID возвращает индекс+1 [1..] в этом массиве.
	ExtData2 * Extra;
private:
	int    InitScope(const DlScope * pScope, int topLevel); // @recursion @<<DlRtm::DlRtm
};

extern "C" typedef DlRtm    * (*FN_DL6RTM_FACTORY)(DlContext *, DlScope *);
extern "C" typedef SCoClass * (*FN_DL6CLS_FACTORY)(const DlContext *, const DlScope *);
//
// Если этот макрос определен, то система генерирует статические таблицы виртуальных
// функций для коклассов. В противном случае эти таблицы генерируются динамически
// при создании объектов и динамически же разрушаются при разрушении объектов
//
//#define COVT_STATIC

#define DL6_HDL_CLS(s)     PPALDD_##s
#define DL6_HDL_FACTORY(s) DL6FF_##s
#define IMPLEMENT_DL6_HDL_FACTORY(s) \
	extern "C" __declspec(dllexport) DlRtm * DL6_HDL_FACTORY(s)(DlContext * pCtx, DlScope * pScope) \
	{ return new DL6_HDL_CLS(s)(pCtx, pScope); }
#define PPALDD_CONSTRUCTOR(s) \
	IMPLEMENT_DL6_HDL_FACTORY(s) DL6_HDL_CLS(s)::DL6_HDL_CLS(s)(DlContext * pCtx, DlScope * pScope) : DlRtm(pCtx, pScope)
#define PPALDD_DESTRUCTOR(s)  DL6_HDL_CLS(s)::~DL6_HDL_CLS(s)()

#define DL6_IC_CLS(s)     DL6ICLS_##s
#define DL6_IC_FACTORY(s) DL6CF_##s
#define IMPLEMENT_DL6_IC_FACTORY(cls) \
	extern "C" __declspec(dllexport) SCoClass * DL6_IC_FACTORY(cls)(const DlContext * pCtx, const DlScope * pScope) \
	{ return new DL6_IC_CLS(cls)(pCtx, pScope); }
#define DL6_IC_CONSTRUCTION_DECL(s) DL6_IC_CLS(s)(const DlContext * pCtx, const DlScope * pScope); ~DL6_IC_CLS(s)()
#ifdef COVT_STATIC
	#define DL6_IC_CONSTRUCTOR(s, vtc) \
		static vtc inst_##vtc; \
		IMPLEMENT_DL6_IC_FACTORY(s) \
		DL6_IC_CLS(s)::DL6_IC_CLS(s)(const DlContext * pCtx, const DlScope * pScope) : SCoClass(pCtx, pScope, &inst_##vtc)
#else
	#define DL6_IC_CONSTRUCTOR(s, vtc) \
		IMPLEMENT_DL6_IC_FACTORY(s) \
		DL6_IC_CLS(s)::DL6_IC_CLS(s)(const DlContext * pCtx, const DlScope * pScope) : SCoClass(pCtx, pScope, new vtc)
#endif
#define DL6_IC_DESTRUCTOR(s) DL6_IC_CLS(s)::~DL6_IC_CLS(s)()
#define DL6_IC_CONSTRUCTION_EXTRA(s, vtc, cls_ext) \
	DL6_IC_CONSTRUCTOR(s, vtc) { ExtraPtr = new cls_ext; } \
	DL6_IC_DESTRUCTOR(s) { delete static_cast<cls_ext *>(ExtraPtr); }
//
// Макро-определения, используемые при реализации интерфейсов
//
#define IMPL_PPIFC_EXTPTR(cls_extra)                   (static_cast<cls_extra *>(ExtraPtr))
#define IMPL_PPIFC_EXTPTRVAR(cls_extra)                cls_extra * p_ext = static_cast<cls_extra *>(ExtraPtr)
#define IMPL_PPIFC_GETPROP(cls_extra, fld)             return (static_cast<cls_extra *>(ExtraPtr))->fld
#define IMPL_PPIFC_GETPROP_CAST(cls_extra, fld, totyp) return (totyp)(static_cast<cls_extra *>(ExtraPtr))->fld
#define IMPL_PPIFC_PUTPROP(cls_extra, fld)             (static_cast<cls_extra *>(ExtraPtr))->fld = value
#define IMPL_PPIFC_PUTPROP_CAST(cls_extra, fld, totyp) (static_cast<cls_extra *>(ExtraPtr))->fld = static_cast<totyp>(value)
//
// Реализация метода PPView::Init()
// ARG(view) наименование класса PPView без приставки PPView.
// Sample:
//	int32 DL6ICLS_PPViewCCheck::Init(IUnknown* pFilt)
//	{
//		IMPL_PPIFC_PPVIEWINIT(CCheck);
//	}
//
#define IMPL_PPIFC_PPVIEWINIT(view)    \
	IPpyFilt_##view * p_ifc_filt = 0;  \
	S_GUID uuid;                       \
	THROW_PP(pFilt, PPERR_INVPARAM);   \
	THROW(GetInnerUUID("IPpyFilt_" #view, uuid));                        \
	THROW(SUCCEEDED(pFilt->QueryInterface(uuid, (void **)&p_ifc_filt))); \
	THROW(((PPView##view *)ExtraPtr)->Init_(static_cast<const view##Filt *>(GetExtraPtrByInterface(p_ifc_filt)))); \
	CATCH AppError = 1; ENDCATCH                        \
	if(p_ifc_filt) reinterpret_cast<IUnknown *>(p_ifc_filt)->Release(); \
	return !AppError;

#define INIT_PPVIEW_ALDD_DATA(ViewTypNam, extr)         \
	PPView##ViewTypNam * p_v = 0;                       \
	if(extr && rFilt.Ptr) {                                          \
		Extra[1].Ptr = p_v = static_cast<PPView##ViewTypNam *>(rFilt.Ptr); } \
	else {                                              \
		Extra[0].Ptr = p_v = new PPView##ViewTypNam;    \
		p_v->Init(static_cast<ViewTypNam##Filt *>(rFilt.Ptr)); }         \
	const ViewTypNam##Filt * p_filt = p_v->GetFilt();

#define INIT_PPVIEW_ALDD_DATA_U(ViewTypNam, extr)         \
	PPView##ViewTypNam * p_v = 0;                       \
	if(extr && rFilt.Ptr) {                                  \
		Extra[1].Ptr = p_v = static_cast<PPView##ViewTypNam *>(rFilt.Ptr); } \
	else {                                              \
		Extra[0].Ptr = p_v = new PPView##ViewTypNam;    \
		p_v->Init_(static_cast<ViewTypNam##Filt *>(rFilt.Ptr)); }    \
	const ViewTypNam##Filt * p_filt = static_cast<const ViewTypNam##Filt *>(p_v->GetBaseFilt());

#define DESTROY_ALDD(Typ)      \
	if(Extra[0].Ptr) {                       \
		delete static_cast<Typ *>(Extra[0].Ptr);  \
		Extra[0].Ptr = 0;                    \
	}                                        \
	Extra[1].Ptr = 0;

#define DESTROY_PPVIEW_ALDD(ViewTypNam)      \
	if(Extra[0].Ptr) {                       \
		delete static_cast<PPView##ViewTypNam *>(Extra[0].Ptr);  \
		Extra[0].Ptr = 0;                    \
	}                                        \
	Extra[1].Ptr = 0;

#define INIT_PPVIEW_ALDD_ITER(ViewTypNam) \
	PPView##ViewTypNam * p_v = static_cast<PPView##ViewTypNam *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr); \
	IterProlog(iterId, 1);                                                                         \
	if(sortId >= 0)                                                                                \
		SortIdx = sortId;                                                                          \
	return p_v->InitIteration() ? 1 : 0;                                                           \

#define INIT_PPVIEW_ALDD_ITER_ORD(ViewTypNam, order) \
	PPView##ViewTypNam * p_v = static_cast<PPView##ViewTypNam *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr); \
	IterProlog(iterId, 1);                                                                         \
	if((order) >= 0)                                                                               \
		SortIdx = (order);                                                                         \
	p_v->InitIteration(order);                                                                     \
	return 1;

#define START_PPVIEW_ALDD_ITER(ViewTypNam)                                                          \
	IterProlog(iterId, 0);                                                                          \
	PPView##ViewTypNam * p_v = static_cast<PPView##ViewTypNam *>(Extra[1].Ptr ? Extra[1].Ptr : Extra[0].Ptr);  \
	ViewTypNam##ViewItem item;                                                                      \
	if(p_v->NextIteration(&item) <= 0) return -1;

#define FINISH_PPVIEW_ALDD_ITER()                                                                   \
	return DlRtm::NextIteration(iterId);

struct PPReportEnv {
	PPReportEnv();

	long   Sort;
	long   PrnFlags;
	SString DefPrnForm;
	SString EmailAddr;
	SString ContextSymb; // Символ контекста, позволяющий дифференцировать выбор формы отчета.
		// В файле report.ini этот символ приписывается в определении зоны после наименования формы отчета через ':'
		// Например:
		// [Ccheckdetailview:CN001]
	SString PrnPort; // @v11.8.8 Явное указание имени принтера, на которые следует отправлять печать
};

int  FASTCALL PPAlddPrint(int RptId, PPFilt & rF, const PPReportEnv * pEnv);
int  FASTCALL PPAlddPrint(int RptId, PView & rV, const PPReportEnv * pEnv);
int  FASTCALL PPExportDL600DataToBuffer(const char * pDataName, long id, SCodepageIdent cp, SString & rBuf);
int  FASTCALL PPExportDL600DataToBuffer(const char * pDataName, void * ptr, SCodepageIdent cp, SString & rBuf);
int  FASTCALL PPExportDL600DataToBuffer(const char * pDataName, PPView * pView, SCodepageIdent cp, SString & rBuf);
int  FASTCALL PPExportDL600DataToJson(const char * pDataName, StrAssocArray * pAry, void * ptr, SString & rBuf);
int  FASTCALL PPExportDL600DataToJson(const char * pDataName, PPView * pV, SString & rBuf);
//
//
//
#define MAX_COCLASS_STTAB 8
//
// Специальный тип для идентификации конструктора SCoClass, создающего SCoFactory
//
enum SCoClassConstructor { scccFactory };
//
// Descr: Базовый класс, реализующий общие механизмы управления COM-интерфейсами системы
//
class SCoClass {
public:
	static HRESULT Helper_DllGetClassObject(REFCLSID rClsID, REFIID rIID, void ** ppV);
	static HRESULT Helper_DllCanUnloadNow();
	static int Helper_DllRegisterServer(int unreg);
	static void * FASTCALL GetExtraPtrByInterface(const void * pIfc);
	static int SetExtraPtrByInterface(const void * pIfc, void * extraPtr);
	SCoClass(const DlContext * pCtx, const DlScope * pScope, void * pVt);
    virtual ~SCoClass();
	HRESULT __stdcall QueryInterface(REFIID, void **);
	uint32  __stdcall AddRef();
	uint32  __stdcall Release();
	HRESULT __stdcall InterfaceSupportsErrorInfo(REFIID rIID); // ISupportErrorInfo method
	HRESULT ImpQueryInterface(REFIID rIID, void ** ppObject);
	uint32  ImpAddRef();
	uint32  ImpRelease();
	HRESULT ImpInterfaceSupportsErrorInfo(REFIID rIID) const;
	int    CreateInnerInstance(const char * pClsName, const char * pIfcName, void ** ppIfc);
	int    RaiseAppError();
	void * RaiseAppErrorPtr();
	int    FASTCALL SetAppError(int assertion);

	long   AppFlags; // Может использоваться реализациями для хранения бинарных флагов
	int    AppError; // Должен использоваться реализациями для сигнализации о прикладной ошибке
protected:
	enum {
		fFactory = 0x0001 // Интерфейс IClassFactory
	};
	struct TabEntry {
		uint32 * P_Tbl;
		SCoClass * ThisPtr;
	};
	SCoClass(SCoClassConstructor, void * pVt);
	int    GetInnerUUID(const char * pScopeName, S_GUID & rIID) const;
	int    SetupError();
	int    FuncNotSupported();
	HRESULT Epilog();
	const  DlContext * P_Ctx;
	const  DlScope * P_Scope;
	void * ExtraPtr;
	SString RetStrBuf; // Временный буфер, используемый только для возврата строковых значений. // @!Больше ни для каких целей использовать нельзя.
private:
	int    FASTCALL InitVTable(void * pVt);
	int    ReleaseVTable();

	ACount Ref;
	long   Flags;
	void * P_Vt;       // Указатель на таблицу виртуальных функций, содержащую методы всех интерфейсов класса
	uint   TabCount;   // Равно количеству интерфейсов, поддерживаемых классом
	TabEntry * P_Tab;  // TabCount-размерная таблица, содержащая указатели на соответствующие интерфейсы, поддерживаемые классом
	TabEntry StTab[MAX_COCLASS_STTAB]; // Если класс поддерживает менее MAX_COCLASS_STTAB интерфейсов, то P_Tab = StTab, в противном случае P_Tab распределяется динамически
};
//
// Макро-определение, используемое в автоматическом описании таблиц виртуальных функций
// при формировании интерфейсов
//
#define IUNKN_METHOD_PTRS(ifn) \
	HRESULT (__stdcall SCoClass::*QI_##ifn)(REFIID, void **); \
	uint32 (__stdcall SCoClass::*AR_##ifn)(); \
	uint32 (__stdcall SCoClass::*R_##ifn)()
#define IUNKN_METHOD_PTRS_ASSIGN(ifn) \
	QI_##ifn=&SCoClass::QueryInterface; \
	AR_##ifn=&SCoClass::AddRef; \
	R_##ifn=&SCoClass::Release
#define ISUPERRINFO_METHOD_PTRS(ifn)        HRESULT (__stdcall SCoClass::*ISEI_##ifn)(REFIID)
#define ISUPERRINFO_METHOD_PTRS_ASSIGN(ifn) ISEI_##ifn=&SCoClass::InterfaceSupportsErrorInfo
#define IFACE_METHOD_PROLOG(cls) /*TRACE_FUNC();*/ cls * __tp = static_cast<cls *>(reinterpret_cast<TabEntry *>(this)->ThisPtr); __tp->AppError=0;
//#define IFACE_METHOD_EPILOG return __tp->AppError ? MAKE_HRESULT(1, FACILITY_ITF, 0) : S_OK
#define IFACE_METHOD_EPILOG /*TRACE_FUNC();*/ return __tp->Epilog()
#define IMCI(func) __tp->func

int Use001(); // чтобы линковались модули с коклассами
//
// Макроопределения, используемые для описания таблиц баз данных //
//
#define DBTABLE_CONSTRUCTOR(tbl, firstField) \
	tbl##Tbl::tbl##Tbl(const char * pFileName/*, int openMode*/) : DBTable(#tbl, pFileName, &firstField, &data, /*openMode*/omNormal) {}\
	tbl##Tbl::Rec::Rec() { Clear(); } // @v12.3.3 THISZERO()-->Clear()

#ifdef DL600C // {
//
// Lex and Yacc support
//
struct yy_buffer_state;

typedef struct {
	struct yy_buffer_state * yyin_buf;
	char   fname[MAX_PATH];
	long   yyin_line;
} YYIN_STR;

extern long yyline;

int    yylex();
void   yyerror(const char * str);
#endif // } DL600C
//
//
//
#endif // __DL600_H
