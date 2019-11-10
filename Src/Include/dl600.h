// DL600.H
// Copyright (c) A.Sobolev 2006, 2007, 2008, 2010, 2011, 2015, 2016б 2017, 2018
// @codepage UTF-8
//
#ifndef __DL600_H
#define __DL600_H
#include <slib.h>
#include <db.h>
#include <tv.h>
#include <json.h> // @Muxa
//
//
//
enum DlOperator {
	dlopConstructor = '0', // null
	dlopDestructor  = '1',
	dlopNew         = '2',
	dlopDelete      = '3',
	dlopAssignment  = '4',
	dlopEq          = '8',
	dlopNeq         = '9',
	dlopConvert     = 'B',
	dlopAdd         = 'H', // +
	dlopSub         = 'G', // -
	dlopMul         = 'D',
	dlopDiv         = 'K',
	dlopMod         = 'L', // %
	dlopInc         = 'E',
	dlopDec         = 'F',
	dlopLt          = 'M',
	dlopLe          = 'N',
	dlopGt          = 'O', // ou
	dlopGe          = 'P',
	dlopAnd         = 'V',
	dlopOr          = 'W',
	dlopNot         = '7',
	dlopBwAnd       = 'I',
	dlopBwOr        = 'U',
	dlopUnaryPlus   = '+',  // not msvs
	dlopUnaryMinus  = '-',  // not msvs
	dlopDot         = '.',  // not msvs
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

class SetScopeBlk; // @Muxa
class DlScope;
class DlContext;
class DlRtm;
class SCoClass;
class PPView;

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
		uint16 Flags;
	};
	static int GetOpName(uint opID, SString & rName); // Compile-time
	DlFunc();
	int    GetName(uint options, SString & rName) const; // Compile-time
	uint   GetArgCount() const;
	int    GetArg(uint argN, Arg *) const;
	DLSYMBID GetArgType(uint argN) const;
	int    GetArgName(uint argN, SString & rArgName) const;
	int    IsEqual(const DlFunc & rPat) const;
	int    AddArg(uint typeId, const char * pName, uint argFlags = 0);

	enum {
		fImplByID  = 0x0001, // Функция реализована через идентификатор реализации
		fLossCvt   = 0x0002, // Преобразование типа с потерей точности
		fPrototype = 0x0004, // Прототип функции (реализация не известна). ImplID = 0
		fInlined   = 0x0008, // Для интерфейсной функции - отсутствует функция реализации.
			// Реализация делается непосредственно в теле интерфейсной функции
		fPropGet   = 0x0010, //
		fPropPut   = 0x0020  // Если установлены оба флага, то в IDL генерируется две
			// записи: propget и propput
	};
	SString Name;
	DLSYMBID TypID;
	uint16 Flags;          // DlFunc::fXXX
	uint16 Pad;            // @alignment
	union {
		uint   ImplID;
		DlFuncImpl Impl;
	};
private:
	SVector ArgList; // @v9.8.6 SArray-->SVector
	StringSet ArgNamList;
};

class DlFuncPool {
public:
	DlFuncPool();
	int    FASTCALL Write(SBuffer & rBuf) const;
	int    FASTCALL Read(SBuffer & rBuf);
	int    FASTCALL IsEqual(const DlFuncPool & rPat) const;
	int    FASTCALL Add(const DlFunc * pF);
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
	SVector Items;   // @v9.8.6 SArray-->SVector
	SVector ArgList; // @v9.8.6 SArray-->SVector
	StringSet NamePool;
};
//
// Descr: Константа.
//   Используется в формулах и при хранении сецифических параметров областей видимости (DlScope)
//
struct CtmExprConst {
	CtmExprConst & Init();
	int    operator !() const;

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
	uint32 DataLen; // @v9.8.6 size_t-->uint32
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
		kUiLayout,    // @v8.9.0
		kHandler      // @v8.9.0
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
	enum COption {
		//
		// Опции таблиц базы данных
		//
		cdbtFileName = 1, // string Имя файла таблицы базы данных
		cdbtAcsName,      // string AlternateCollatingSequence file name
		cdbtPageSize,     // uint32 Размер страницы таблицы базы данных
		cdbtPrealloc,     // uint32 Количество страниц распределяемых при создании таблицы базы данных
		cdbtPctThreshold, // uint32 BTRIEVE Процентный порог свободного места, оставляемого в странице VLR
		cdbtAccess,       // uint32 Уровень доступа к таблице базы данных
		//
		// Опции диалогов и других элементов UI
		//
		cuiRect,          // raw    Координаты области пользовательского интерфейса
		cuifCtrlKind,     // uint32 Вид управляющего элемента
		cuifCtrlText,     // string Текст управляющего элемента
		cuifCtrlScope,    // uint32 Ассоциированная область управляющего элемента
		cuifCtrlCmd,      // uint32 ИД команды кнопки
		cuifCtrlCmdSymb,  // string Символ команды кнопки
		cuifCtrlRect,     // raw    Координаты управляющего элемента
		cuifCtrlListboxColDescr, // string Строка описания колонок списка
		cuifSymbSeries,   // string Серия символов управляющих элементов диалога
		cuifCtrlLblRect,  // raw    Координаты текстового ярлыка, ассоциированного с управляющим элементом
		cuifCtrlLblSymb,  // string Символ текстового ярлыка, ассоциированного с управляющим элементом
		cuifLabelRect,    // raw(UiRelRect) Положение текстового ярлыка, ассоциированного с управляющим элементом
		cuifReadOnly,     // int8
		cuifDisabled,     // int8
		cuifAlignment,    // int8
		cuifHidden,       // int8
		cuifFont,         // string
		cuifStaticEdge,   // int8
		cuifLayoutType    // int8 @construction
	};
	struct IfaceBase {
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
		ckInput = UiItemKind::kInput,
		ckStatic = UiItemKind::kStatic,
		ckPushbutton = UiItemKind::kPushbutton,
		ckCheckbox = UiItemKind::kCheckbox,
		ckRadioCluster = UiItemKind::kRadioCluster,
		ckCheckCluster = UiItemKind::kCheckCluster,
		ckCombobox = UiItemKind::kCombobox,
		ckListbox = UiItemKind::kListbox,
		ckTreeListbox = UiItemKind::kTreeListbox,
		ckFrame = UiItemKind::kFrame,
		ckRadiobutton = UiItemKind::kRadiobutton,
		ckLabel = UiItemKind::kLabel
	};

	static int FASTCALL ResolvePropName(const char * pName);
	static int FASTCALL GetPropSymb(int propId, SString & rSymb);
	static SString & PropListToLine(const StrAssocArray & rPropList, uint tabCount, SString & rBuf);

	SLAPI  DlScope(DLSYMBID id, uint kind, const char * pName, int prototype);
	SLAPI  DlScope(const DlScope &);
	SLAPI ~DlScope();
	DlScope & FASTCALL operator = (const DlScope &);
	int    SLAPI Copy(const DlScope &, int withoutChilds = 0);
	int    FASTCALL Add(DlScope * pChild);
	//
	// Descr: Удаляет из списка дочерних областей ту, что имеет идентификатор scopeID.
	// Returns:
	//   Количество удаленный дочерних областей.
	//   0 - не найдено ни одной дочерней области с ИД = scopeID
	//   >1 - найдено и удалено более одной дочерней области с ИД = scopeID
	//
	int    SLAPI Remove(DLSYMBID scopeID);
	DLSYMBID SLAPI GetId() const;
	DLSYMBID SLAPI GetBaseId() const;
	const  SString & SLAPI GetName() const;
	int    SLAPI CheckDvFlag(long) const;
	int    FASTCALL IsKind(const uint) const;
	uint   SLAPI GetKind() const;
	//
	// Descr: Возвращает номер версии области видимости. Используется для областей вида
	//   DlScope::kIClass и DlScope::kLibrary.
	//   Структура значения следующая: LoWord - major, HiWord - minor
	//
	uint32 SLAPI GetVersion() const;
	const  DlScope * SLAPI GetOwner() const;
	const  DlScopeList & SLAPI GetChildList() const; // GetRecList
	const  DlScope * SLAPI GetFirstChildByKind(int kind, int recursive) const;
	int    SLAPI GetChildList(int kind, int recursive, LongArray * pList) const;
	int    FASTCALL IsChildOf(const DlScope * pOwner) const;
	int    SLAPI EnumChilds(uint * pIdx, DlScope ** ppScope) const;
	int    SLAPI EnumInheritance(uint * pIdx, const DlScope ** ppScope) const;
	void   SLAPI SetupTitle(uint kind, const char * pName);
	int    SLAPI IsPrototype() const;
	void   SLAPI ResetPrototypeFlag();
	int    SLAPI GetQualif(DLSYMBID id, const char * pDiv, int inverse, SString & rBuf) const;

	int    SLAPI SetDeclList(const StringSet * pSet);
	int    SLAPI SetInheritance(const DlScope * pBase, DlContext * pCtx);
	//
	// Descr: Инициализирует указатели P_Base этой и всех дочерних областей
	//   в соответствии со значением поля BaseId. Функция доолжна быть вызвана
	//   после загрузки списка областей из хранилища, поскольку в хранилище держится только
	//   идентификатор базовой области.
	//
	int    FASTCALL InitInheritance(const DlScope * pTopScope); // @recursion
	DlScope * SLAPI SearchByName(uint kind, const char * pName, DLSYMBID * pParentID);
	const  DlScope * SLAPI SearchByName_Const(uint kind, const char * pName, DLSYMBID * pParentID) const;
	DlScope * SLAPI SearchByID(DLSYMBID id, DLSYMBID * pParentID);
	const  DlScope * SLAPI SearchByID_Const(DLSYMBID id, DLSYMBID * pParentID) const;
	const  DlScope * SLAPI GetBase() const;
	DLSYMBID SLAPI EnterScope(DLSYMBID parentId, DLSYMBID newScopeID, uint kind, const char * pName);
	int    SLAPI LeaveScope(DLSYMBID scopeId, DLSYMBID * pParentID);

	int    FASTCALL SetRecord(const DlScope * pRec);
	int    FASTCALL SetRecList(const DlScopeList * pList);

	int    FASTCALL AddFunc(const DlFunc *);
	int    SLAPI GetFuncListByName(const char * pName, LongArray * pList) const;
	uint   SLAPI GetFuncCount() const;
	int    SLAPI GetFuncByPos(uint pos, DlFunc * pFunc) const;
	int    SLAPI EnumFunctions(uint * pI, DlFunc * pFunc) const;
	int    FASTCALL Write(SBuffer & rBuf) const;
	int    FASTCALL Read(SBuffer & rBuf);
	int    FASTCALL IsEqual(const DlScope & rPat) const;

	void   SLAPI SetFixDataBuf(void * pBuf, size_t size, int clear = 0);
	void * FASTCALL GetFixDataPtr(size_t offs) const;
	//
	// Descr: Устанавливает атрибут Attr. Атрибут представлен флагом и, возможно, дополнительным
	//   значением. Функция является аддитивной и, в общем случае, не коммутативной.
	//
	void   FASTCALL SetAttrib(const Attr &);
	int    SLAPI GetAttrib(uint attrFlag /* DlScope::sfXXX */, Attr * pAttr) const;
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
	//    0 - ошибка
	//
	int    SLAPI AddConst(COption id, const CtmExprConst & rConst, int replace);
	int    SLAPI GetConst(COption id, CtmExprConst * pConst) const;
	CtmExprConst FASTCALL GetConst(COption id) const;
	//
	//
	//
	int    SLAPI AddFldConst(uint fldID, COption id, const CtmExprConst & rConst, int replace);
	int    SLAPI GetFldConst(uint fldID, COption id, CtmExprConst * pConst) const;
	CtmExprConst SLAPI GetFldConst(uint fldID, COption id) const;
	//
	// Функции управления базовыми интерфейсами класса (kIClass)
	//
	int    SLAPI AddIfaceBase(const IfaceBase *);
	uint   SLAPI GetIfaceBaseCount() const;
	int    SLAPI GetIfaceBase(uint, IfaceBase *) const;
	//
	//
	//
	int    SLAPI AddDbIndexSegment(const char * pFieldName, long options);
	long   SLAPI GetDbIndexSegOptions(uint pos) const;
#ifdef DL600C // {
	int    SLAPI AddTempFldConst(COption id, const CtmExprConst & rConst);
	int    SLAPI AcceptTempFldConstList(uint fldID);
	int    SLAPI InitLocalIdCounter(DLSYMBID initVal)
	{
		LastLocalId = initVal;
		return 1;
	}
	DLSYMBID SLAPI GetLocalId()
	{
		return ++LastLocalId;
	}
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
	SVector * P_IfaceBaseList;    // Список интерфейсов, поддерживаемых объектом (только для kIClass) // @v9.8.4 SArray-->SVector
	//
	// Descr: Константа, хранящая опцию
	//
	struct CItem {
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
	TSVector <CItem> CList;      // Список констант, используемый для хранения специфических опций // @v9.8.4 TSArray-->TSVector
	TSVector <CfItem> CfList;    // Список констант, ассоциированных с полями // @v9.8.4 TSArray-->TSVector
	LongArray * P_DbIdxSegFlags; // Список флагов сегментов индекса таблицы базы данных (только для kDbIndex)
	SBaseBuffer FixDataBuf;      // @transient Буфер записи для фиксированных полей (не формул)
		// Экземпляр класса DlScope не владеет указателем FixDataBuf.P_Buf, по-этому деструктор
		// не вызывает FixDataBuf.Destroy()
#ifdef DL600C // {
	TSVector <CfItem> TempCfList; // @v9.8.4 TSArray-->TSVector
	DLSYMBID LastLocalId;
#endif
};
//
//
//
struct CtmToken {
	int    Init();
	void   Destroy();
	int    Create(uint code);
	int    Create(uint code, const char * pStr);
	int    Create(uint code, DLSYMBID id);

	uint16 Code;
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
		char * S;
		S_GUID_Base Uuid;
	} U;
};

struct CtmVar {
	DLSYMBID ScopeID;
	uint   Pos; // @#[0..] Позиция поля в области видимости
};

struct CtmFunc {
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
	int    FASTCALL InitVar(const char * pName);
	int    FASTCALL InitVar(const CtmVar &);
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
		OpRef  Ref; // @v7.1.10
		char * S;
	} U;
	CtmExpr * P_Next;      // Следующий элемент (по горизонтали). Например, в списке
	CtmExpr * P_Arg;       // Список аргументов
};

struct CtmDclr {
	void   Init();
	void   Destroy();
	int    Copy(const CtmDclr & rDclr);
	int    AddDim(uint dim);
	int    AddPtrMod(uint ptrMod, uint modifier = 0);
	int    AddDecimalDim(uint dec, uint precision);

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

class DlMacro {
public:
	SLAPI  DlMacro();
	int    SLAPI Add(const char * pSymb, const char * pResult);
	int    SLAPI Subst(const char * pSymb, SString & rResult) const;
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
	SLAPI  DlContext(int toCompile = 0);
	SLAPI ~DlContext();
	int    FASTCALL Init(const char * pInFileName);
	enum {
		ispcExpData = 1,
		ispcInterface
	};
	int    FASTCALL InitSpecial(int); // Run-time
	const  char * SLAPI GetInputFileName() const;
	int    SLAPI GetSymb(DLSYMBID id, SString & rBuf, int prefix) const;
	int    SLAPI SearchSymb(const char * pSymb, int prefix, DLSYMBID * pID) const;
	//
	// Descr: Возвращает область видимости, соответствующую идентификатору scopeID.
	//   Если checkKind != 0, то функция проверяет, чтобы возвращаемая область имела
	//   указанный вид. Если это не так, то возвращается 0 и устанавливается код ошибки
	//   PPERR_DL6_INVSCOPEKIND
	//
	DlScope * SLAPI GetScope(DLSYMBID scopeID, int checkKind = 0);
	DlScope * SLAPI GetCurScope();
	const  DlScope * SLAPI GetScope_Const(DLSYMBID scopeID, int checkKind = 0) const;
	const  DlScope * SLAPI GetScopeByName_Const(uint kind, const char * pName) const;
	enum {
		crsymfCatCurScope = 0x0001,
		crsymfErrorOnDup  = 0x0002
	};
	int    SLAPI ResolveVar(DLSYMBID scopeID, int exactScope, const char * pSymb, CtmVar * pVar);
	int    SLAPI ResolveVar(const DlScope * pScope, int exactScope, const char * pSymb, CtmVar * pVar);
	//
	// Descr: Специальный вариант реализации вызова функции в TDDO-файлах.
	//
	int    SLAPI ResolveFunc(DlRtm * pRtm, const DlScope * pScope, int exactScope, const char * pFuncName, StrAssocArray & rArgList, SString & rResult, STypEx & rT);
	int    SLAPI GetFunc(const CtmFunc & rF, DlFunc * pFunc);
	int    SLAPI GetField(const CtmVar & rV, SdbField * pFld);
	int    SLAPI GetConstData(const CtmExprConst & rC, void * pBuf, size_t bufLen) const;
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
	//
	int    SLAPI MakeClassName(const DlScope * pStruc, int clsnf, SString & rBuf) const;
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
	//    0 - ошибка
	//
	int    SLAPI TypeCast(DLSYMBID srcTyp, DLSYMBID destTyp, int cvt, const void * pSrcData, void * pDestData, int * pLoss = 0);
	int    SLAPI AddStructType(DLSYMBID symbId);
	int    SLAPI SearchTypeID(DLSYMBID id, uint * pPos, TypeEntry * pEntry) const;
	size_t FASTCALL GetTypeBinSize(DLSYMBID typID) const;
	DLSYMBID SLAPI SearchSTypEx(const STypEx & rTyp, TypeEntry * pEntry) const;
	DLSYMBID SLAPI SetDeclType(DLSYMBID typeID);
	TYPEID SLAPI TypeToSType(DLSYMBID) const;
	int    SLAPI DemangleType(const char * pTypeStr, STypEx * pTyp, DLSYMBID * pID);
	long   SLAPI ParseFormat(const char * pStr, TYPEID typ) const; // *
	//
	// Descr: Устанавливает код ошибки и, возможно, дополнительную строку, связанную с ошибкой.
	//   Функция константная, так как для compile-time изменяется DlContext::LastError и
	//   DlContext::AddedMsgString, а для run-time вызывается PPSetError().
	//
	int    SLAPI SetError(int errCode, const char * pAddedMsg = 0) const;
	enum {
		erfExit = 0x0001, // Завершить работу
		erfLog  = 0x0002  // Записать сообщение об ошибке в журнал
	};
	int    SLAPI Error(int errCode = 0, const char * pAddedInfo = 0, long flags = erfExit);
	//
	// Compile-time {
	//
	enum {
		cfDebug   = 0x0001, // Отладочный режим компиляции
		cfBinOnly = 0x0002, // Создавать только бинарные файлы (не генерировать .H, .CPP и прочие файлы для разработки)
		cfSQL     = 0x0004, // Генерировать SQL-скрипт для создания таблиц базы данных
		cfOracle  = 0x0008  // Генерировать SQL-скрипт специфичный для ORACLE для создания таблиц базы данных
	};
	int    SLAPI Compile(const char * pInFileName, const char * pDictPath, const char * pDataPath, long cflags);
	int    SLAPI FindImportFile(const char * pFileName, SString & rPath);
	int    SLAPI SetInheritance(DLSYMBID scopeID, DLSYMBID baseID);
	int    SLAPI MangleType(DLSYMBID id, const STypEx &, SString & rBuf) const;
	int    SLAPI AddType(const char * pName, TYPEID stypId, char mangleC = 0);
	DLSYMBID SLAPI MakeSizedString(DLSYMBID typeID, size_t s);
	int    SLAPI AddDeclaration(DLSYMBID typeId, const CtmDclr & rDclr, CtmExpr * pExpr);
	int    SLAPI ResolveVar(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr);
	int    SLAPI ResolveFunc(DLSYMBID scopeID, int exactScope, CtmExpr * pExpr);
	int    SLAPI ResolveExpr(DLSYMBID scopeID, DLSYMBID callerScopeID, int exactScope, CtmExpr * pExpr, int dontResolveNext = 0);
	DLSYMBID SLAPI CreateSymb(const char * pSymb, int prefix, long flags);
	DLSYMBID SLAPI CreateSymbWithId(const char * pSymb, DLSYMBID id, int prefix, long flags);
	//
	// Descr: Возвращает уникальный внутри этого экземпляра идентификатор.
	//
	DLSYMBID SLAPI GetNewSymbID();
	long     SLAPI GetUniqCntr();
	DLSYMBID SLAPI SetDeclTypeMod(DLSYMBID ofTyp, int mod /* STypEx::modXXX */, uint arrayDim = 0);
	DLSYMBID SLAPI EnterScope(uint scopeKind, const char * pName, DLSYMBID scopeId, const SV_Uint32 * pAttrList);
	int    SLAPI LeaveScope();
	int    SLAPI PushScope();
	int    SLAPI PopScope();
	int    SLAPI CompleteExportDataStruc();
	int    SLAPI AddMacro(const char * pMacro, const char * pResult);
	int    SLAPI GetMacro(const char * pMacro, SString & rResult) const;
	int    FASTCALL GetDotFunc(CtmFunc * pF);
	int    FASTCALL GetRefFunc(CtmFunc * pF);
	int    SLAPI AddConst(const char * pTypeSymb, const void * pData, size_t dataSize, CtmExprConst * pResult);
	int    SLAPI AddConst(const void * pData, size_t dataSize, CtmExprConst * pResult);
	int    SLAPI AddConst(const char * pData, CtmExprConst * pResult);
	int    SLAPI AddConst(const SString & rData, CtmExprConst * pResult);
	int    SLAPI AddConst(uint32 data, CtmExprConst * pResult);
	int    SLAPI AddConst(int32 data, CtmExprConst * pResult);
	int    SLAPI AddConst(int8 data, CtmExprConst * pResult);
	int    SLAPI AddConst(int64 data, CtmExprConst * pResult);
	int    SLAPI AddConst(int16 data, CtmExprConst * pResult);
	int    SLAPI AddConst(float data, CtmExprConst * pResult);
	int    SLAPI AddConst(double data, CtmExprConst * pResult);

	int    SLAPI AddConst(LDATE data, CtmExprConst * pResult);
	int    SLAPI AddConst(LTIME data, CtmExprConst * pResult);

	int    SLAPI AddStrucDeclare(const char * pDecl);
	int    SLAPI AddFuncDeclare(const CtmDclr & rSymb, const CtmDclrList & rArgList, int propDirParam = 0);
	int    SLAPI AddPropDeclare(CtmDclr & rSymb, int propDirParam);
	int    SLAPI AddEnumItem(const CtmToken & rSymb, int useExplVal, uint val);
	int    SLAPI AddTypedef(const CtmToken & rSymb, DLSYMBID typeID, uint tdFlags);
	//
	// Descr: Инициализирует имя нового индекса таблицы базы данных.
	//   Если имя уже определено в параметре pIdxName, то проверяет его уникальность внутри таблицы.
	//   Если pIdxName == 0 || pIdxName[0] == 0, то присваивает индексу имя Key#, где # - номер [0..]
	//
	int    SLAPI InitDbIndexName(const char * pIdxName, SString & rBuf);
	int    SLAPI AddDbIndexSegmentDeclaration(const char * pFieldName, long options);
	int    SLAPI ResolveDbIndexSegFlag(long flags, const char * pSymb, long * pResultFlags);
	int    SLAPI ResolveDbIndexFlag(const char * pSymb);
	int    SLAPI ResolveDbFileDefinition(const CtmToken & rSymb, const char * pConstStr, int constInt);
	int    SLAPI Write_Code();
	int    SLAPI CreateDbDictionary(const char * pDictPath, const char * pDataPath, SqlServerType sqlst);
	//
	// Descr: Создает описание диалогового управляющего элемента в текущей области видимости.
	// Returns:
	//   ID созданного элемента описания.
	//   0  - ошибка.
	//
	uint   SLAPI AddUiCtrl(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect);
	uint   SLAPI AddUiButton(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rCmdSymb);
	uint   SLAPI AddUiListbox(const CtmToken & rSymb, const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rColumns);
	uint   SLAPI AddUiCluster(int kind, const CtmToken & rSymb, const CtmToken & rText, DLSYMBID typeID, const UiRelRect & rRect);
	int    SLAPI AddUiClusterItem(const CtmToken & rText, const UiRelRect & rRect, const CtmToken & rDescr);
	int    SLAPI AddTempFldProp(const CtmToken & rSymb, long val);
	int    SLAPI AddTempFldProp(const CtmToken & rSymb, double val);
	int    SLAPI AddTempFldProp(const CtmToken & rSymb, const char * pStr);
	int    SLAPI AddTempFldProp(const CtmToken & rSymb, const void * pData, size_t sz);
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
	int    SLAPI GetUuidByScopeID(DLSYMBID scopeID, S_GUID * pUuid) const;
	int    SLAPI GetInterface(const S_GUID_Base & rIID, DLSYMBID * pID, const DlScope ** ppScope) const;
	//
	// Descr: Перебирает интефейсы класса pCls.
	// Returns:
	//   >0 - интерфейс с индексом *pI успешно извлечен
	//   <0 - по индексу *pI и более интерфейсов нет
	//    0 - ошибка извлечения интерфейса (*pI меньше, чем количество интерфейсов)
	//
	int    SLAPI EnumInterfacesByICls(const DlScope * pCls, uint * pI, DlScope::IfaceBase * pIfb, const DlScope ** ppIfaceScope) const;
	//
	// Descr: Регистрирует (дерегистрирует) в реестре класс pCls
	// ARG(pCls  IN): @#{pCls->IsKind(DlScope::kIClass)!=0}
	//   указатель на область видимости, соответствующей классу
	// ARG(unreg IN): если !0, то класс pCls дерегистрируется, в противном случае - регистрируется //
	//
	int    SLAPI RegisterICls(const DlScope * pCls, int unreg); // @recusion
	int    SLAPI RegisterTypeLib(const DlScope * pCls, int unreg); // @v5.4.5 AHTOXA
	int    SLAPI CreateDlIClsInstance(const DlScope *, SCoClass ** ppInstance) const;
	int    SLAPI CreateDlIClsInstance(const S_GUID & rClsUuid, SCoClass ** ppInstance) const;
	//
	int    SLAPI CreateDlRtmInstance(DlScope *, DlRtm ** ppInstance);
	int    SLAPI CreateDlRtmInstance(const char * pName, DlRtm ** ppInstance);
	//
	// } Run-time
	//
	int    SLAPI UnpackFormula(DlScope *);
	CtmExpr * SLAPI GetFormula(const DlScope *, uint fldPos);
	uint   SLAPI AllocStackType(DLSYMBID typeID, TypeEntry * pTe);
	int    FASTCALL ReleaseStack(uint pos);
	int    FASTCALL FreeStack(uint sp);
	void * FASTCALL GetStackPtr(uint sp);
	int    SLAPI EvaluateExpr(DlRtm * pRtm, const DlScope *, DlRtm * pCallerRtm, const DlScope * pCallerScope, CtmExpr * pExpr, size_t sp); // run-time
	const  DlScope * SLAPI GetEvaluatedVarScope(const DlScope * pScope, const CtmExpr * pExpr) const; // run-time @obsolete
	const  DlScope * SLAPI GetEvaluatedVarScope(const DlScope * pScope, DLSYMBID targetScopeID) const; // run-time
	//
	// Descr: Создает спецификацию таблицы базы данных, пригодную для формирования по ней
	//   записи в словаре базы данных и создания файла данных.
	// Returns:
	//   !0 - спецификация таблицы успешно создана
	//   0 - ошибка
	//
	int    SLAPI LoadDbTableSpec(DLSYMBID scopeID, DBTable * pTbl, int format) const;
	int    SLAPI LoadDbTableSpec(const char * pName, DBTable * pTbl, int format) const;
	int    SLAPI CreateNewDbTableSpec(const DBTable * pTbl);
	int    SLAPI DropDbTableSpec(const char * pName);
	int    SLAPI GetDbTableSpecList(StrAssocArray * pList) const;
	int    SLAPI Test_ReWr_Code(const DlContext & rPattern);
	//
	//
	//
	int    SLAPI GetDialogList(StrAssocArray * pList) const;
#ifdef DL600C // {
	UUIDAssocArray TempUuidList; // @transient Временный список GUID'ов используемый при компиляции
#endif
	int    SLAPI Test();
private:
	DlScope * SLAPI Helper_GetScope(DLSYMBID id, const DlScope * pScope, int kind) const;
	DLSYMBID  SLAPI Helper_CreateSymb(const char * pSymb, DLSYMBID newId, int prefix, long flags);
	int    SLAPI Helper_GetScopeList(int kind, int recursive, StrAssocArray * pList) const;
	DlScope * SLAPI GetCurDialogScope();
	int    SLAPI  GetUiSymbSeries(const char * pSymb, SString & rSerBuf, DLSYMBID * pId);
	int    SLAPI  Helper_AddBFunc(const char * pFuncName, uint implID, const char * pRetType, va_list pArgList);
	int    SLAPIV AddBOp(int op, uint implID, const char * pRetType, ...);
	int    SLAPIV AddBFunc(const char * pFuncName, uint implID, const char * pRetType, ...);
	int    SLAPI  AddBCmpOps(uint implID, const char * pType);
	int    SLAPI  AddBCvt(uint implID, int loss, const char * pRetType, const char * pSrcType);
	int    FASTCALL BuiltinOp(const DlFunc * pF, SV_Uint32 *);
	int    SLAPI GetFuncName(int, const CtmExpr * pExpr, SString & rBuf);
	int    SLAPI SearchVarInChildList(const DlScope * pScope, uint childKind, const char * pSymb, CtmVar * pVar);
	int    SLAPI ProcessQuestArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList);
	int    SLAPI InitFileNames(const char * pInFileName);
	int    SLAPI Helper_LoadDbTableSpec(const DlScope *, DBTable * pTbl, int format) const;
	//
	// Compile-time {
	//
	int    SLAPI AddCvtFuncToArgList(const DlFunc & rFunc, CtmExpr * pExpr, const LongArray & rCvtPosList) const;
		// @<<DlContext::ResolveFunc
	int    SLAPI IsFuncSuited(const DlFunc & rFunc, CtmExpr * pExpr, LongArray * pCvtArgList);
	int    SLAPI MakeDlRecName(const DlScope * pRec, int instanceName, SString & rBuf) const;
	int    SLAPI Write_C_FileHeader(Generator_CPP & gen, const char * pFileName);
	int    SLAPI Write_C_DeclFile(Generator_CPP & gen, const DlScope & rScope);  // @recursion
	int    SLAPI Write_C_ImplFile(Generator_CPP & gen, const DlScope & rScope);  // @recursion
	int    SLAPI Write_C_AutoImplFile(Generator_CPP & gen, const DlScope & rScope, StringSet & rSs); // @recursion
	int    SLAPI Write_WSDL_File(const char * pFileName, const DlScope & rScope);
	int    SLAPI Write_DialogReverse();
	//
	// } Compile-time
	//
	enum {
		ffIDL = 1,    // Для IDL-файла
		ffH_Iface,    // Прототип интерфейса в H-файле
		ffH_Imp,      // Прототип реализации интерфейса в H-файле
		ffCPP_Iface,  // Реализация интерфейса в CPP-файле
		ffCPP_Imp,    // Реализация в CPP-файле
		ffCPP_VTbl,   // Элемент таблицы виртуальных функций в CPP-файле
		ffCPP_CallImp // Вызов реализации из интерфейсной функции
	};
	int    SLAPI Write_Func(Generator_CPP & gen, const DlFunc & rFunc, int format, const char * pForward = 0);
	int    SLAPI Write_C_ImplInterfaceFunc(Generator_CPP & gen, const SString & rClsName, DlFunc & rFunc);
	int    SLAPI Write_IDL_Attr(Generator_CPP & gen, const DlScope & rScope);
	int    SLAPI Write_IDL_File(Generator_CPP & gen, const DlScope & rScope); // @recursion
	int    SLAPI Write_DebugListing();
	int    SLAPI Write_Scope(int indent, SFile & rOutFile, const DlScope & rScope); // @recursion
	int    SLAPI Format_TypeEntry(const TypeEntry & rEntry, SString & rBuf); // @recursion
	int    SLAPI Format_Func(const DlFunc & rFunc, long options, SString & rBuf);
	int    SLAPI FormatVar(CtmVar v, SString & rBuf) const;
	enum {
		fctfSourceOutput  = 0x0001, // Форматировать тип для вывода в C++
		fctfIDL           = 0x0002, // Форматировать тип для вывода в IDL
		fctfIfaceImpl     = 0x0004, // Форматировать тип для функций реализации интерфейсов
		fctfInstance      = 0x0008, // Форматирование для экземпляров типов. Основное отличие -
			// строки представлены не ссылками, а собственно экземплярами (SString вместо SString&)
		fctfResolveTypeID = 0x0010  // Функция должна вызвать SearchTypeID для идентификации типа typeID
	};
	int    SLAPI Format_C_Type(DLSYMBID typeID, STypEx & rTyp, const char * pFldName, long flags, SString & rBuf);
	int    SLAPI IsInterfaceTypeConversionNeeded(const STypEx & rTyp);
	int    SLAPI Read_Code();
	//
	struct TypeDetail { // Compile-time
		SLAPI  TypeDetail();
		SLAPI ~TypeDetail();
		int    SLAPI IsInterfaceTypeConversionNeeded() const;
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
	SymbHashTable Tab;              //
	StringSet CurDeclList;
	CtmConstList ConstList;         // @persistent
	TSVector <TypeEntry> TypeList;  // @persistent // @v9.8.4 TSArray-->TSVector
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

	TSVector <SyncUuidAssoc> SyncUuidList; // @transient Compile-time // @v9.8.4 TSArray-->TSVector
	StringSet SyncUuidNameList;
	LAssocArray UiSymbAssoc; // Ассоциации символов элементов UI с символами сериий.
#else
	TSVector <uint> Pss; // Run-time "Pushed String Stack" Стэк позиций строк, распределенных на стеке StP. // @v9.8.4 TSArray-->TSVector
		// Нужен для того, чтобы при освобождении стекового пространства освобождать соответствующие строки
	SStringPool StP;                     // Run-time
	TSCollection <DlRtm> RtmList;        // Run-time
	class UnpFormArray : private SArray {
	public:
		SLAPI  UnpFormArray();
		SLAPI ~UnpFormArray();
		int    SLAPI Add(DLSYMBID scopeID, uint fldPos, CtmExpr * pExpr);
		CtmExpr * SLAPI Get(DLSYMBID scopeID, uint fldPos);
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
	//    0 - ошибка.
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
	int    SetByJSON(json_t * pJSONDoc, long & ObjId);           // @Muxa
	int    SetByJSON_Helper(json_t * pNode, SetScopeBlk & rBlk); // @Muxa

	struct ExportParam {
		SLAPI  ExportParam();
		enum {
			fIsView            = 0x0001, // P_F->Ptr указывает на класс, порожденный от PPView
			fInheritedTblNames = 0x0002, // При экспорте в качестве наименования таблиц будет
				// применяться имя родительской структуры
			fDiff_ID_ByScope   = 0x0004, // Если этот флаг установлен, то наименования полей идентификаторов
				// записей в таблицах, соответствующих разным областям будут отличаться.
				// Эта опция необходима из-за того, что единовременно перевести все структуры на
				// различающиеся наименования таких полей невозможно по причине необходимости верификации
				// соответствующих отчетов
			fForceDDF          = 0x0008, // Если в файлах словаря нет необходимости, то все равно создавать такие файлы.
				// Возникновение флага обусловлено тем, что CrystalReports не требует для печати
				// файлов словарей. Однако, они необходимы для формирования и редактирования отчетов в дизайнере.
			fDontWriteXmlDTD   = 0x0010, // Не формировать DTD в XML-файле
			fDontWriteXmlTypes = 0x0020, // @v7.1.9 Не формировать зону типов в XML-файле
			fCompressXml       = 0x0040  // @v10.6.0 Сжимать создаваемый XML-файл
		};
		PPFilt * P_F;
		int    Sort;
		long   Flags;
		SCodepageIdent Cp;
		SFileFormat OutputFormat;
		SString DestPath;
		SString Path;
		const void * P_ViewDef; //v10.5.1
	};
	int    SLAPI Export(ExportParam & rParam);
	int    SLAPI ExportXML(ExportParam & rParam, SString & rOutFileName);
	int    SLAPI PutToXmlBuffer(ExportParam & rParam, SString & rBuf);
	int    SLAPI Helper_PutScopeToJson(const DlScope * pScope, json_t * pJsonObj) const;
	int    SLAPI Helper_PutItemToJson(ExportParam & rParam/*PPFilt * pFilt*/, json_t * pRoot);
	int    SLAPI PutToJsonBuffer(StrAssocArray * pAry, SString & rBuf, int flags);
	int    SLAPI PutToJsonBuffer(void * ptr, SString & rBuf, int flags);
	int    SLAPI PutToJsonBuffer(PPView * pV, SString & rBuf, int flags);

	const ExportParam * P_Ep;
protected:
	struct ExtData {
		long   isFirst;
		void * Ptr;
	};
	int    InitFixData(const char * pScopeName, void * pData, size_t dataSize);
	int    InitFixData(int reservedScopeCode, void * pData, size_t dataSize);
	int    FASTCALL FinishRecord(const DlScope * pScope); // @recursion
	int    FASTCALL AssignHeadData(void * pData, size_t dataSize);          // PPALDD compatibility
		// @>>InitFixData
	int    FASTCALL AssignIterData(int one, void * pData, size_t dataSize); // PPALDD compatibility
		// @>>InitFixData
	int    FASTCALL AssignDefIterData(void * pData, size_t dataSize); // @>>InitFixData
	int    SLAPI FillXmlBuf(const DlScope * pScope, xmlTextWriter * pWriter, StringSet * pDtd, SCodepageIdent cp) const;
	void   SLAPI FillDTDBuf(const DlScope * pScope, xmlTextWriter * pWriter, const char * pElemName) const;
	//
	// Descr: Стандартный пролог функций InitIteration и NextIteration.
	// Returns:
	//   >0 - функция успешно выполнена.
	//   <0 - функция успешно выполнена. При этом Extra[rID-].isFirst изменено с 1 на 0.
	//   0  - ошибка
	//
	int    FASTCALL IterProlog(/*PPIterID*/long & rID, int doInit);
	DlContext * P_Ctx; // @notowned
	DlScope * P_Data;     // @*DlRtm::DlRtm
	DlScope * P_HdrScope; // @*DlRtm::DlRtm
	DLSYMBID DataId;
	int    Valid;   // PPALDD compatibility
	int    SortIdx; // PPALDD compatibility
	SV_Uint32 IterList; // Массив идентификаторов (DLSYMBID) областей видимости, соответствующих
		// итераторам. Функция GetIterID возвращает индекс+1 [1..] в этом массиве.
	ExtData * Extra;
private:
	int    SLAPI InitScope(const DlScope * pScope, int topLevel); // @recursion @<<DlRtm::DlRtm
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

#define DESTROY_PPVIEW_ALDD(ViewTypNam)      \
	if(Extra[0].Ptr) {                       \
		delete static_cast<PPView##ViewTypNam *>(Extra[0].Ptr);  \
		Extra[0].Ptr = 0;                    \
	}                                        \
	Extra[1].Ptr = 0; /* @v9.6.4 return 1; */

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
};

int  FASTCALL PPAlddPrint(int RptId, PPFilt * pf, const PPReportEnv * pEnv = 0);
int  FASTCALL PPAlddPrint(int RptId, PView * pview, const PPReportEnv * pEnv = 0);

//int  SLAPI PPAlddPrint(int RptId, PPFilt * pf, int sort = 0, int prnflag = 0, const char * pDefPrnForm = 0);
//int  SLAPI PPAlddPrint(int RptId, PView * pview, int sort = 0, int prnflag = 0, const char * pDefPrnForm = 0);
// @v9.8.5 int  SLAPI PPAlddPrint(int RptId, DlRtm *);
// @v9.8.5 int  SLAPI PPAlddExport(DlRtm *, PPFilt *, int sort, int isView, char * pPath, const char * pDestPath = 0, int inheritedTblNames = 0);
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
	static HRESULT SLAPI Helper_DllGetClassObject(REFCLSID rClsID, REFIID rIID, void ** ppV);
	static HRESULT SLAPI Helper_DllCanUnloadNow();
	static int SLAPI Helper_DllRegisterServer(int unreg);
	static void * FASTCALL GetExtraPtrByInterface(const void * pIfc);
	static int SLAPI SetExtraPtrByInterface(const void * pIfc, void * extraPtr);

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

	int    SLAPI CreateInnerInstance(const char * pClsName, const char * pIfcName, void ** ppIfc);
	int    SLAPI RaiseAppError();
	void * SLAPI RaiseAppErrorPtr();
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
	int    SLAPI GetInnerUUID(const char * pScopeName, S_GUID & rIID) const;
	int    SLAPI SetupError();
	int    SLAPI FuncNotSupported();
	HRESULT SLAPI Epilog();
	const  DlContext * P_Ctx;
	const  DlScope * P_Scope;
	void * ExtraPtr;
	SString RetStrBuf; // Временный буфер, используемый только для возврата строковых значений.
		// @!Больше ни для каких целей использовать нельзя.
private:
	int    FASTCALL InitVTable(void * pVt);
	int    SLAPI ReleaseVTable();

	ACount Ref;
	long   Flags;
	void * P_Vt;       // Указатель на таблицу виртуальных функций, содержащую методы всех интерфейсов класса
	uint   TabCount;   // Равно количеству интерфейсов, поддерживаемых классом
	TabEntry * P_Tab;  // TabCount-размерная таблица, содержащая указатели на соответствующие интерфейсы,
		// поддерживаемые классом
	TabEntry StTab[MAX_COCLASS_STTAB]; // Если класс поддерживает менее MAX_COCLASS_STTAB интерфейсов,
		// то P_Tab = StTab, в противном случае P_Tab распределяется динамически
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

int Use001(); // @v5.4.5, чтобы линковались модули с коклассами
//
// Макроопределения, используемые для описания таблиц баз данных //
//
#define DBTABLE_CONSTRUCTOR(tbl, firstField) \
	tbl##Tbl::tbl##Tbl(const char * pFileName/*, int openMode*/) : DBTable(#tbl, pFileName, &firstField, &data, /*openMode*/omNormal) {}

#ifdef DL600C // {
//
// Lex and Yacc support
//
struct yy_buffer_state;

typedef struct {
	struct yy_buffer_state * yyin_buf;
	char   fname[MAXPATH];
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
