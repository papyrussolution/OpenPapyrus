// PF.H
// Copyright (c) A.Sobolev 2000-2002
//
// Definitions for remix of PlanFlex Project
//
#ifndef __PF_H
#define __PF_H

#ifndef __SLIB_H
	#include <slib.h>
#endif

#define FALSE       0
#define TRUE        1
#define RIGHT       0
#define LEFT        1
#define FULL        2
#define NOTNEEDED   0
#define CURR_TYPE   1
#define NEXT_TYPE   2
#define MAXMULT    20
#define PRECISION   5 // Точность представления расчетных значений, м

typedef long PFID;
typedef long HEQUIP;
//
//
//
class  CDFile;
class  CuttDg;
struct CuttDgRec;
class  EquipVect;
class  LogSpcVect;
class  EquipRowVect;
class  BoardSpcVect;
class  TechLimitsVect;
class  CuttingDiagram;
class  HardCDCollection;
class  ProtoDiagramQueue;
class  CuttingDiagramCollection;
class  LogItem;
class  ProtoDiagram;
class  SzCluster;
class  CuttDgGenerator;
class  HardCDEntry;
class  ProtoDiagram;
class  BoardSpcVect;
struct GenExtra;
class  VisiLumber;
//
//
//
double SQ(double a) { return (a*a); }
double defVolumeFunc(PFID breedID, double, double);
char * cd2str(ProtoDiagram*, char*, int, int = 0);

typedef TSArray <double> RealArray;

DECL_CMPFUNC(double);
//
// Tokens
//
enum {                  // Знак сравнения
	rvEqual = 0x0001,
	rvLess  = 0x0002,
	rvGreat = 0x0004,
};
//
// Структура обработки синтаксических выражений
//
struct TokenVect {
	int    tCount;      // Количество лексем
	int    nCount;      // Количество чисел
	int    tVect[64];   // Список лексем
	double nVect[32];   // Список чисел
};
//
// Структура проверки корректности выражения
//
struct ValidTokenTemplate {
	int  * vtokens;     // Массив допустимых лексем
	int    numvtokens;  // Количество допустимых лексем
	RealRange bounds;   // Границы представления чисел
};

struct ErrLoc {         // Положение ошибки в строке ввода
	int    begChar;
	int    endChar;
};
//
// Лексемы
//
enum {
	lexError = -1,       // Ошибочная лексема
	lexDelim =  0,       // Разделитель
	lexAsterisk,         // Звездочка ( "*"  )
	lexDoubleDot,        // Две точки ( ".." )
	lexLE,               // <=
	lexGE,               // >=
	lexEQ,               // =
	lexLT,               // <
	lexGT,               // >
	lexNum,              // Число
	lexEOL,              // Конец строки ( '\0' или '\n' )
	firstSign    = 0,    // Первый знак
	lastSign     = 7,    // Последний знак
	firstCmpSign = 2,    // Первый знак сравнения
	lastCmpSign  = 7     // Последний знак сравнения
};
//
// Типы элемента постава
//
const
	stMain     = 0x0100, // Основная доска
	stSide     = 0x0200, // Неосновная доска
	stCenter   = 0x0500, // Центральное место постава  (включает stMain)
	stPith     = 0x0800,
	stCntr     = 0x1000,
	stSawLog   = 0,      // Бревно
	stDETimber = 1,      // Двухкантный брус
	stSQBoard  = 2,      // Обрезная доска
	stUEBoard  = 3,      // Необрезная доска
	stSlab     = 4,      // Горбыль
	stEdging   = 5,      // Рейка
	stChip     = 6,      // Щепа
	stNumber   = 7,      // Количество типов элементов постава
	bpNumber   = 3,      // Количество типов мест в поставе
	MaxLevels  = 4;      // Максимальное число уровней распиловки
//
//
//
class Sawlog {
public:
	SLAPI  Sawlog(PFID breedID, double aDiam, double aLen);
	SLAPI  Sawlog(const Sawlog &);
	void   SLAPI Init(PFID breedID, double aDiam, double aLen);
	void   SLAPI Init(double aDiam, double aLen);
	void   SLAPI SetVolumeFunc(double (*)(PFID,double,double));

	PFID   SLAPI GetBreedID() { return BreedID;  }
	double SLAPI GetDiameter() { return D0; }
	double SLAPI GetLength() { return L; }
	double SLAPI GetVolume() { return V; }

	double SLAPI Diameter(double len);
	double SLAPI Taper();
	double SLAPI MinPith();
	double SLAPI MinCntr();
protected:
	void   SLAPI Evaluate();
	double SLAPI ParaWidth(double A);
	double SLAPI ParaLen(double A);
	double SLAPI SqLen(double A, double width);
	double SLAPI SqWidth(double A, double len);

	PFID   BreedID;
	double D0;
	double L;
	double V;
	double P;

	static double (*volumeFunc)(PFID, double, double);
};
//
//
//
class AbstractLumber {
public:
	SLAPI  AbstractLumber();
	SLAPI  AbstractLumber(const AbstractLumber &);
	int    SLAPI IsBoard() const;
	int    SLAPI IsSawLog() const;
	int    SLAPI IsBadLumber() const;
	int    SLAPI IsGoodLumber() const;
	int    SLAPI IsCenter() const;

	ushort LumberKind; // Вид пиломатериала
};
//
// Элемент протопостава
//
struct ProtoDiagramItem : public AbstractLumber {
	SLAPI  ProtoDiagramItem();
	SLAPI ~ProtoDiagramItem();
	int    Item;      // Номер соответствующего элемента в диаграмме
	double OuterCutt; // Внешний срез
	double InnerCutt; // Внутренний срез
	double NmnlSize;  // Номинальный размер
	uint16 SType;     // Тип размера
	ProtoDiagramQueue * P_Cluster; // Следующий проход
};
//
// Очередь протопоставов (можно использовать как обычную коллекцию)
//
class ProtoDiagramQueue : public SQueue {
public:
	SLAPI  ProtoDiagramQueue();
	int    SLAPI put(const ProtoDiagram *);
	ProtoDiagram * SLAPI get() const;
	ProtoDiagram * SLAPI peek() const;
private:
	virtual void SLAPI freeItem(void*);
};
//
// Исходный элемент для распиловки
//
class SourceLumber : public AbstractLumber, public Sawlog {
public:
	SLAPI  SourceLumber(const Sawlog &);
	SLAPI  SourceLumber(const SourceLumber&, const ProtoDiagramItem&);
	double SLAPI Scope(double minLen, double minWidth);
	double SLAPI Width2length(double, RealRange&, double, double);
	void   SLAPI InscribedLengthes(RealRange&, double&, double&);
	int    SLAPI InscribedSizes(SArray&, SzCluster*, double);

	double Voc;  // Вертикальный внешний срез
	double Vic;  // Вертикальный внутренний срез
	double Hrc;  // Правый срез
	double Hlc;  // Левый срез
	double Size; // Номинальный размер
};
//
// Размерные параметры структуры GenTemplateItem
//
struct SizeParams {
	double Sz;    // Номинальный размер, м
	double RawSz; // Размер с припуском на усушку, м
	double Scope; // Область вписания от центра бревна, м
};

DECL_CMPFUNC(SizeParams); // По убыванию SizeParams::Sz
//
// VisiLumber
//
const size_t
	maxSizeStr = 42,
	maxMultStr =  6;

class VisiLumber : public AbstractLumber {
public:
	SLAPI  VisiLumber();
	SLAPI  VisiLumber(const VisiLumber &);
	int    SLAPI Merge(VisiLumber *);
	int    SLAPI SizType(); // Возвращает формируемый размер ( szXXX )

	char   MultStr[maxMultStr]; // Множитель
	char   SizeStr[maxSizeStr]; // Список номинальных размеров
	CuttDg * P_Owner; // Владелец
	CuttDg * P_Diagr; // Схема раскроя
};
//
// CuttDg
//
class CuttDg : public SCollection {
public:
	CuttDg(VisiLumber *);
	CuttDg(CuttDg &);
	VisiLumber * at(uint);
	int    FindCenter(uint &);
	int    IsSymmetric() const;

	HEQUIP EquipID;   // Лесопильный станок
	uint   Type;      // Симметричный, Фиксированный
	short  MaxBoards; // Макс. число досок
	VisiLumber * P_Source; // Исходный сортимент
	// Объект CuttDg не является владельцем объекта
	// на который указывает P_Source
private:
	virtual	void SLAPI freeItem(void *);
};
//
// Элемент шаблона GenTemplate
//
struct GenTemplateItem : public AbstractLumber {
	SLAPI  GenTemplateItem();

	TSArray <SizeParams> Sizes;  // Размерные параметры
	IntRange Mults;  // Диапазон допустимых множителей
	int    SType;    // Вид размера (ширина/толщина)
	int    DItem;    // Номер объекта VisiLumber в коллекции CuttDg
	int    Locked;   // Флаг блокировки (для внутреннего пользования)
};
//
// Шаблон для генерирования полупоставов
// Используется классом Tracker
//
typedef TSCollection <GenTemplateItem> GenTemplate;
//
// Флаги поля state структуры TrackerItem
//
enum {
	tiTerminal = 0x0001,  // Терминальный (больше вставлять нечего)
	tiCenter   = 0x0002   // Центральный  (в центре бревна или бруса)
};
//
// Элемент трекера
//
struct TrackerItem {
	int    Item;        // Индекс шаблона в объекте GenTemplate
	int    SizePtr;     // Индекс текущего размера в шаблоне
	double Offset;      // Расстояние от точки отсчета до наружной пласти
	uint   State;       // Поле состояния
};
//
// Трекер - генератор полупоставов
//
class Tracker : public SArray {
public:
	SLAPI  Tracker(CuttDgGenerator*, SourceLumber*, CuttDg*, int);
	SLAPI ~Tracker();
	int    SLAPI GenNext();
	int    SLAPI Direction() const { return Dir; }
	int    SLAPI IsEven() const { return Even; }
	void   SLAPI Restart(int aDir);
	double SLAPI Size(int) const;
	double SLAPI Ssize(int) const;
	double SLAPI Scope(int) const;
	TrackerItem & SLAPI at(uint i) const
	{
		return *(TrackerItem *)SArray::at(i);
	}
	GenTemplateItem & SLAPI Lumber(uint i) const
	{
		return *GenTmpl.at(at(i).Item);
	}
protected:
	int    SLAPI Init();
	void   SLAPI InitSizes(VisiLumber&, GenTemplateItem&);
	void   SLAPI InitMults(VisiLumber&, GenTemplateItem&);
	void   SLAPI InitScope(VisiLumber&, GenTemplateItem&);
	void   SLAPI Recalc();
	void   SLAPI Terminal(int);
	int    SLAPI IsTerminal() const;
	int    SLAPI InsertItem(int);
	int    SLAPI InsertNext();
	int    SLAPI EnumSizes();
	int    SLAPI InsertingNeeded() const;
	int    SLAPI CheckLimits();
	int    SLAPI NumIdentItems() const;
	int    SLAPI IsTherePlace(const GenTemplateItem &) const;
	double SLAPI MinSize(const GenTemplateItem &);
	double SLAPI MaxHight(const GenTemplateItem &) const;
	double SLAPI MinHight(const GenTemplateItem &) const;
	GenTemplateItem * SLAPI NextLumber(int currentLumber) const;
	GenTemplateItem & SLAPI Lumber() const { return Lumber(Current); }
private:
	int    Dir;                  // Направление на шаблоне (RIGHT | LEFT)
	int    Even;                 // != 0 если постав четный
	int    NoEven;               // != 0 если запрещены четные поставы
	int    Current;              // Текущий элемент трекера
	double SawWidth;             // Ширина пропила, м
	CuttDg          * P_Diagram;   // Шаблон раскроя
	SourceLumber    * P_Source;    // Порождающий элемент верхнего уровня
	CuttDgGenerator * P_Generator; // Порождающий генератор
	GenTemplate       GenTmpl;
};
//
// Протопостав
//
class ProtoDiagram : public SArray {
public:
	SLAPI  ProtoDiagram();
	SLAPI ~ProtoDiagram();
	ProtoDiagramItem & SLAPI at(uint);
	virtual void   SLAPI atFree(int);
	void   SLAPI freeAll();
	int    SLAPI Make(Tracker&);
	int    SLAPI Make(HardCDEntry*, double);
	int    SLAPI Make(ProtoDiagram &left, ProtoDiagram &right);
	void   SLAPI SetSymm(int b) { x_symm = b ? TRUE : FALSE; }
	int    SLAPI IsSymm() const { return x_symm; }
	int    SLAPI IsEven() const { return x_even; }
	int    SLAPI IsFull() const { return x_full; }
	friend int SLAPI operator == (ProtoDiagram &a, ProtoDiagram&b);
	int    x_hard;     // Признак жесткого постава
	char * P_HardMark; // Обозначение жесткого постава
private:
	int    x_symm;   // Признак симметричности
	int    x_even;   // Признак четности
	int    x_full;   // Признак полноты (не полупостав)
};
//
// Элемент размерной группы бревен
//
struct LogItem {
	int  D;    // Диаметр, мм
	int  L;    // Длина, мм
	int  p;    // Доля в группе, промилле (0.1 %)
	int  base; // Признак базового диаметра для расчета
};
//
// Элемент кластера специфицированных размеров
//
struct SzEntry {
	double Sz;    // Номинальный размер, м
	double RawSz; // Размер с припуском на усушку, м
};

DECL_CMPFUNC(SzEntry);
//
// Кластер специфицированных размеров
// Содержит номинальный размер и множество соответствующих
// ему размеров другого измерения как объекты типа SzEntry
// (например: содержит толщину 0.075 м и сооветствующие ширины
// 0.100, 0.150, 0.175, 0.200).
// Сам кластер является элементом объекта SpcSizes
//
class SzCluster : public SArray { // Ordered CMPFUNC(SzEntry)
public:
	SLAPI  SzCluster();
	SLAPI  SzCluster(SzCluster&);
	SzEntry & SLAPI at(uint idx);

	double Sz;    // Номинальный размер, м
	double RawSz; // Размер с припуском на усушку, м
};

DECL_CMPFUNC(SzCluster);
//
// Спецификация размеров досок
// Создается при генерации, а по завершению разрушается
// Исходной для создания объекта SpcSizes является коллекция
// BoardSpcCollection (см. функцию CuttDgGenerator::initSizes)
//
typedef TSCollection <SzCluster> SpcSizes; // Ordered CMPFNC(SzCluster)
//
// Сообщения, передаваемые функции по адресу
// CuttDgGenerator::indicator
//
const
	cdgHalfDiagReady  = 1,  // Сгенерирован полупостав
	cdgProtoDiagReady = 2,  // Сгенерирован полный протопостав
	cdgDiagReady      = 3,  // Сгенерирован готовый постав
	cdgNextLevel      = 4,  // Рекурсивный переход на следующий проход
	cdgPrevLevel      = 5,  // Возврат из рекурсии к предыдущему уровню
	cdgNewEquipRow    = 6,  // Началась обработка новой линии
	cdgNewLogGroup    = 7,  // Началась обработка новой группы бревен
	cdgMkProtoDiag    = 8,  // Начало генерации протопоставов
	cdgMkDiag         = 9,  // Начало обсчета готовых поставов
	cdgFinish         = 10, // Завершение работы генератора
	cdgAvlMemory      = 11, // Наличие доступной памяти
	cdgUserBreak      = 12; // Прерывание пользователем
//
// ГЕНЕРАТОР В СБОРЕ
//
class CuttDgGenerator {
public:
	static void (*indicator)(int event, void *ptr = NULL);
	static void defIndicator(int, void*) {}

	SLAPI  CuttDgGenerator();
	SLAPI ~CuttDgGenerator();
	void   SLAPI ReleaseData();
	int    SLAPI LoadData();
	int    SLAPI Init();
	int    SLAPI Run();
	int    SLAPI Run(ProtoDiagramQueue*, SourceLumber*, CuttDg*);
	int    SLAPI Make(CuttingDiagram&, CDFile*, SourceLumber&,
		ProtoDiagramQueue&, int);
	int    SLAPI GlobalCheck(ProtoDiagram&, CuttDg&);
	int    SLAPI MakeLogItemList(RealRange &range);
	long   SLAPI DealTime(PFID equipRowID);
	void   SLAPI DeleteCDFile();

	int    NumLg;
	LogItem   * P_LogItem;
	SpcSizes  * P_Thicks;
	SpcSizes  * P_Widths;
	CuttDgRec * P_Scheme;
protected:
	int    SLAPI InitSizes(SArray*, SpcSizes*, SpcSizes*);
	int    SLAPI GlobalCheck(CuttingDiagram*);
	void   SLAPI SaveSpcSizes(SpcSizes*&, SpcSizes*&);
	void   SLAPI RestoreSpcSizes(SpcSizes*&, SpcSizes*&);
	void   SLAPI ProcessHards(HardCDCollection*, ProtoDiagramQueue&, RealRange&);
private:
	int    DataInited; // Если 0, то вектора не освобождать
};
//
// Сечение обрезной доски, которое может получиться из необрезной доски
//
struct PossibleSQBoard {
	double Thick;        // Толщина
	double Width;        // Ширина
	double Length;       // Длина
	double Probability;  // Вероятность
};

class ProtoDiagramItem;
//
// Параметры доски, получаемой из постава
//
struct CuttingDiagramItem {
	unsigned LType;  // Тип пиломатериала
	int      Thick;  // Толщина, мм
	int      Width;  // Ширина,  мм
	int      Length; // Длина,   мм
	float    Output; // Выход,   куб. м
};
//
// CuttingDiagram
//
const 
	cdfHard  = 0x0001,
	cdfInval = 0x8000;
//
// Структура описания постава
//
class CuttingDiagram {
public:
	SLAPI  CuttingDiagram();
	SLAPI  CuttingDiagram(CuttingDiagram&);
	SLAPI ~CuttingDiagram();
	CuttingDiagramItem & SLAPI  at(uint i);
	int    SLAPI search(void *, int &);
	int    SLAPI insert(void *);
	int    SLAPI getCount();
	void   SLAPI atFree(int);
	void   SLAPI slVolume();
	void   SLAPI totals();
	void   SLAPI normalizeSections();
	void   SLAPI makeLogDiamRange(RealRange &);
	void   SLAPI makeLogDiamRange(IntRange &);
	char * SLAPI getString(char *, int maxLen, int level);
	// ***
	uint32 Flags;         // Флаги
	PFID   CD_ID;         // Идентификационный номер постава
	PFID   Breed;
	PFID   EquipRow;
	long   DealTime;    // Время прогона одного бревна, с
	double LogVolume;   // Объем сырья, куб. м
	double TotalOutput; // Общий выход пиломатериалов, куб. м
	int16  TotalSects;  // Общее количество различных сечений
	uint16 NumLg;       // Количество размерных групп бревен
	uint16 Count;       // Количество элементов
	// ***
	// Данные между отметками '// ***' пишутся в- и читаются из
	// потоков как непрерывная область памяти размером 24 байта
	// ***
	CuttingDiagramItem * P_Items;
	uchar              * ExtData;
	static int           CdStyle;

	size_t SLAPI ExtSize();
	LogItem & SLAPI GetLogItem(int);
	int    SLAPI PutLogItems(LogItem*, int);
	int    SLAPI PutString(ProtoDiagram*, int level);
};
//
// CuttingDiagramCollection
//
class CuttingDiagram;

const
	sortByNumber = 0,
	sortByOutput = 1;
//
// Набор поставов
//
// CuttingDiagramCollection
//
#endif /* __PF_H */
