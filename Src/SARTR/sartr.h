// SARTR.H
// Copyright (c) A.Sobolev 2011, 2012, 2016, 2017
//

/*
	<...> - металексемы

	//
	//
	//
	Формы слов:
	<NOUN>
		Род (Gender): Мужской (Masculine) | Женский (Feminine) | Средний (Neuter) | Общий [сирота; недотрога]
		Специальная форма: Литературная | Разговорная | Архаическая //
		Число (): Единственное (Singular) | Множественное (Plural)
		Падеж (Case):
			Именительный (Nominative) |
			Родительный (Genitive) |
			Дательный (Dative) |
			Винительный (Accusative) |
			Творительный (Ablative)  |
			Предложный (Prepositional) |
			(Другие падежи - всего не более 32)

	<ADJV>
	<VERB>
	//
	// Другие термины
	//
	(pluralia tantum) - существительные, употребляемые только в мн. числе [трусы; брюки; вилы]
*/
//
// Грамматические признаки слов
// Пометка @bin стоит у признаков, которым могут иметь значение 1 или отсутствовать
//
#define SRWG_LANGUAGE                 1 // Язык
/*
	#define SRLANG_META                   1
	#define SRLANG_LAT                    2
	#define SRLANG_ENG                    3
	#define SRLANG_RUS                    4
	#define SRLANG_GER                    5
*/
#define SRWG_CLASS                    2 // SRWC_XXX       Класс лексемы
	#define SRWC_ALPHA                    1 // ALPHA        Буква алфавита
	#define SRWC_DIGIT                    2 // DIGIT        Арабская цифра
	#define SRWC_NOUN                     3 // NOUN         Существительное (Noun)
	#define SRWC_NUMERAL                  4 // NUM          Числительное (Numeral) [One]
	#define SRWC_NUMERALORD               5 // ORD          Порядковое числительное (Order numeral) [First]
	#define SRWC_ADJECTIVE                6 // ADJ          Прилагательное (Adjective)
	#define SRWC_VERB                     7 // VERB         Глагол (Verb)
	// @v9.2.0 #define SRWC_VERBMODAL                8 // VERBMODAL    Модальный глагол (Modal verb) [Should]
	#define SRWC_PRONOUN                  9 // PRONOUN      Местоимение (Personal pronoun)
	#define SRWC_PRONOUNPOSS             10 // PRONOUNPOSS  Местоимение-прилагательное (Possessive pronoun) [His, Her]
	#define SRWC_PRAEDICPRO              11 // PRAEDICPRO   Местоимение-предикатив @ex "нечего"
	#define SRWC_PRAEDIC                 12 // PRAEDIC      Предикатив @ex "интересно"
	#define SRWC_ADVERB                  13 // ADVERB       Наречие (Adverb) [freely; свободно]
	#define SRWC_PREPOSITION             14 // PREP         Предлог (Preposition) [beside]
	#define SRWC_POSTPOSITION            15 // POSTP        Послелог @ex "бога ради"
	#define SRWC_CONJUNCTION             16 // CONJ         Союз (Conjunction) [and]
	#define SRWC_INTERJECTION            17 // INTERJ       Междометие (Interjection) [oh]
	#define SRWC_PARENTH                 18 // PARENTH      Вводное слово @ex "конечно"
	#define SRWC_ARTICLE                 19 // ARTICLE      Артикль (Article) [the]
	#define SRWC_PARTICLE                20 // PARTICLE     Частица (Particle) [to]
	#define SRWC_PARTICIPLE              21 // PARTCP       Причастие (Participle) [reading]
	#define SRWC_GERUND                  22 // GERUND       Герундий (Gerund) [reading books]
	#define SRWC_GPARTICIPLE             23 // GPARTC       Деепричастие @ex "Рассказывая, он громко смеялся"
	#define SRWC_PHRAS                   24 // PHRAS        Фразеологический токен (мне не совсем ясен детальный смысл
		// этого классификатора - в русском словаре встречается только одно слово такого типа - "БОСУ").
	#define SRWC_PREFIX                  25 // PREFIX       Приставка
	#define SRWC_AFFIX                   26 // AFFIX        Окончание
	#define SRWC_SKELETON                27 // SKEL         Неизменяемая основа слова
	//#define SRWC_PRONOUNREFL             28 // PRONOUNREFL  Возвратное местоимение (reflexive pronoun)
	#define SRWC_PRONOUNNOUN             29 // PRONOUNNOUN  Местоимение-существительное [all, anybody]
	#define SRWC_POSSESSIVEGROUP         30 // POSSGRP possessive (english)
		// выделена условная "часть речи", поскольку показатель этой грамматической категории относится не к ОДНОМУ СЛОВУ,
		// но к целой именной группе и присоединяется просто к последнему его члену.
		// [the King of England's daughter] [the girl I was dancing with's name]
#define SRWG_ABBR                     3 // SRABBR_XXX     Виды аббревиатур
	#define SRABBR_ABBR                   1 // ABBR Общее обозначение аббревиатуры без детализации
	#define SRABBR_NARROW                 2 // NARR Сильно сокращенный вариант слова (дни недели, месяцы и т.д.)
#define SRWG_PROPERNAME               4 // SRPROPN_XXX    Вид имени собственного
	#define SRPROPN_PERSONNAME            1 // PERSN Имя //
	#define SRPROPN_FAMILYNAME            2 // FAMN  Фамилия //
	#define SRPROPN_PATRONYMIC            3 // PATRN Отчество
	#define SRPROPN_ZOONAME               4 // ZOON  Кличка животного
	#define SRPROPN_ORG                   5 // ORG   Наименование организации
	#define SRPROPN_GEO                   6 // GEO   Наименование географического объекта (населенный пункт, гора, река и т.д.)
#define SRWG_ANIMATE                  5 // SRANIM_XXX     Одушевленность
	#define SRANIM_ANIMATE                1 // ANIM   Одушевленное
	#define SRANIM_INANIMATE              2 // INANIM Недушевленное
#define SRWG_USAGE                    6 // SRWU_XXX       Форма употребления слова
	#define SRWU_LITERARY                 1 // LIT Литературная //
	#define SRWU_PRO                      2 // PRO Профессионализм
	#define SRWU_ARCHAIC                  3 // ARC Архаическая (устаревшая) //
	#define SRWU_SPOKEN                   4 // SPK Разговорная  //
	#define SRWU_VULGAR                   5 // VUL Вульгарная   //
#define SRWG_GENDER                   7 // SRGENDER_XXX   Род
	#define SRGENDER_MASCULINE            1 // MASC Мужской (Masculine)
	#define SRGENDER_FEMININE             2 // FEM  Женский (Feminine)
	#define SRGENDER_NEUTER               3 // NEU  Средний (Neuter)
	#define SRGENDER_COMMON               4 // GCOM Общий [сирота; недотрога]
#define SRWG_TANTUM                   8 // Классификатор, указывающий на возможность использования слова только в одном числе
	#define SRTANTUM_SINGULAR             1 // SINGT Существительное всегда во единственном числе @ex "дичь" "мороженое"
	#define SRTANTUM_PLURAL               2 // PLURT Существительное всегда во множественном числе @ex "весы" "азы"
#define SRWG_COUNT                    9 // SRCNT_XXX      Число
	#define SRCNT_SINGULAR                1 // SING Единственное число
	#define SRCNT_PLURAL                  2 // PLUR Множественное число
#define SRWG_CASE                    10 // SRCASE_XXX     Падеж
	#define SRCASE_NOMINATIVE             1 // NOM    Именительный
	#define SRCASE_GENITIVE               2 // GEN    Родительный
	#define SRCASE_GENITIVE2              3 // GEN2   Второй родительный @ex "чашка чаю"
	#define SRCASE_DATIVE                 4 // DAT    Дательный
	#define SRCASE_DATIVE2                5 // DAT2   Дистрибутивный дательный @ex "[по] многу, нескольку, стольку"
	#define SRCASE_ACCUSATIVE             6 // ACC    Винительный
	#define SRCASE_ACCUSATIVE2            7 // ACC2   Второй винительный @ex "постричься в монахи" "по два человека"
	#define SRCASE_INSTRUMENT             8 // INS    Творительный @ex "головой" "сыном" "степью" "санями" "которым"
	#define SRCASE_PREPOSITIONAL          9 // PREP   Предложный
	#define SRCASE_PREPOSITIONAL2        10 // PREP2  Второй предложный @ex "в лесу" "на оси"
	#define SRCASE_VOCATIVE              11 // VOC    Звательный @ex "Господи" "Серёж" "ребят"
	#define SRCASE_ADNUM                 12 // ADNUM  Счетная форма @ex "два часа" "три шара"
	#define SRCASE_OBJECTIVE             13 // OBJCTV Объектный падеж (в английском)
#define SRWG_TENSE                   11 // SRTENSE_XXX    Время //
	#define SRTENSE_PRESENT               1 // PRES Настоящее
	#define SRTENSE_PAST                  2 // PAST Прощедшее
	#define SRTENSE_FUTURE                3 // FUTU Будущее
	#define SRTENSE_PASTPARTICIPLE        4 // PASTPART Прощедшее время дополнительная форма (English: Past Participle)
#define SRWG_PERSON                  12 // SRPERSON_XXX   Лицо
	#define SRPERSON_FIRST                1 // P1   Первое лицо
	#define SRPERSON_SECOND               2 // P2   Второе лицо
	#define SRPERSON_THIRD                3 // P3   Третье лицо
#define SRWG_VALENCY                 13 // SRVALENCY_XXX  Валентность глагола
	#define SRVALENCY_AVALENT             1 // AVALENT   Авалентный (безличный) глагол. @ex "светает"
	#define SRVALENCY_INTRANSITIVE        2 // INTRANS   Непереходный глагол. @ex "спать"
	#define SRVALENCY_TRANSITIVE          3 // TRANS     Переходный глагол.   @ex "покупать"
	#define SRVALENCY_DITRANSITIVE        4 // DITRANS   Переходный глагол с валентностью 3. @ex "He gives her a flower."
#define SRWG_ASPECT                  14 // SRASPECT_XXX   Аспект глагола
	#define SRASPECT_INFINITIVE           1 // INF       Неопределенная форма глагола
	#define SRASPECT_PERFECTIVE           2 // PERFV     Совершенный глагол
	#define SRASPECT_IMPERFECTIVE         3 // IMPERFV   Несовершенный глагол
	#define SRASPECT_HABITUAL             4 // HABIT     Повторяющееся действие  @ex "I used to go there every day"
	#define SRASPECT_CONTINUOUS           5 // CONTS     Продолжающееся действие
	#define SRASPECT_STATIVE              6 // CSTATV    Подмножество SRASPECT_CONTINUOUS @ex "I know French"
	#define SRASPECT_PROGRESSIVE          7 // CPROGV    Подмножество SRASPECT_CONTINUOUS @ex "I am running"
	#define SRASPECT_PERFECT              8 // PERF      Комбинированный со временем аспект @ex "I have studied well"
#define SRWG_MOOD                    15 // SRMOOD_XXX     Наклонение (модальность) глагола
	#define SRMOOD_INDICATIVE             1 // INDCTV    Indicative   @ex "Paul is eating an apple" "John eats apples"
	#define SRMOOD_SUBJUNCTIVE            2 // SUBJUNCTV Subjunctive  @ex "John would eat if he 'were' hungry"
	#define SRMOOD_CONJUNCTIVE            3 // CONJUNCTV Conjunctive
	#define SRMOOD_OPTATIVE               4 // OPTV      Optative
	#define SRMOOD_JUSSIVE                5 // JUSSIV    Jussive
	#define SRMOOD_POTENTIAL              6 // POTENT    Potential
	#define SRMOOD_PROHIBITIVE            7 // PROHIBV   Prohibitive
	#define SRMOOD_IMPERATIVE             8 // IMPERV    Повелительное наклонение
	#define SRMOOD_INTERROGATIVE          9 // INTERRV   Интер
#define SRWG_VOICE                   16 // SRVOICE_XXX    Залог глагола
	#define SRVOICE_ACTIVE                1 // ACT       Действительный залог  @ex "разрушил" "разрушивший"
	#define SRVOICE_PASSIVE               2 // PASS      Страдательный залог   @ex "разрушаемый" "разрушенный"
	#define SRVOICE_MEDIAL                3 // MED       Медиальный залог      @ex "разрушился"
	// .. Существует множество иных залогов в разных языках
#define SRWG_ADJCAT                  17 // Категория прилагательного
	#define SRADJCAT_QUALIT               1 // ADJQUAL   Качественное прилагательное
	#define SRADJCAT_RELATIVE             2 // ADJREL    Относительное прилагательное
	#define SRADJCAT_POSSESSIVE           3 // ADJPOSS   Притяжательное прилагательное
	#define SRADJCAT_NATION               4 // ADJNATION Прилагательное, означающее принадлежность к национальности
		// @? я не уверен, что эта категория не может пересекаться с другими.
#define SRWG_ADJCMP                  18 // SRADJCMP_XXX   Сравнительная форма прилагательного
	#define SRADJCMP_COMPARATIVE          1 // COMP    Сравнительное прилагательное @ex "веселей" "веселее"
	#define SRADJCMP_SUPERLATIVE          2 // SUPR    Превосходная степень прилагательного
	#define SRADJCMP_COMPARATIVE2         3 // COMP2   Вторая сравнительная степень прилагательного @ex "повеселей" "повеселее"
#define SRWG_SHORT                   19 // SRSHORT_XXX    Краткая форма (обычно прилагательных, но возможно и других частей речи)
	#define SRSHORT_BREV                  1 // BREV  Краткая форма прилагательного
	#define SRSHORT_PLEN                  2 // PLEN  Полная форма прилагательного
#define SRWG_INVARIABLE              20 //                Неизменяемое слово (либо 0, либо 1)
#define SRWG_ADVERBCAT               21 // SRADVCAT_XXX   Категория наречия //
	#define SRADVCAT_INTERROGATIVE        1 // ADVINTR Вопросительное наречие
	#define SRADVCAT_RELATIVE             2 // ADVREL  Относительное наречие
	#define SRADVCAT_POINTING             3 // ADVPNT  Указательное наречие
#define SRWG_LOCAT                   22 // @bin LOC Локативное прилагательное @ex "Верхнеуральский"
#define SRWG_ERROR                   23 // @bin ERR Ошибочное написание слова, частая опечатки и т.д.
#define SRWG_TOBE                    24 // @bin Специальный случай - глагол to be в английском. Все остальные
	// теги словоформ соответствуют глагольным (или герундий) формам.
#define SRWG_QUEST                   25 // @bin QUEST (Не знаю в какую категорию занести) Вопросительная форма слова (глагола)
#define SRWG_MODAL                   26 // @bin MODAL (Не знаю в какую категорию занести) Модальный глагол.
#define SRWG_POSSESSIVE              27 // @bin POSSTAG (Запутался с possessive в английском: надо будет еще раз перетряхнуть классификацию
	// словоформ с таким признаком). На данный момент указанный тег может быть присвоен местоимению и существительному в english
#define SRWG_PREDICATIVE             28 // @bin PREDICAT Признак предикативного местоимения (english) [yours]
#define SRWG_PRONOUN                 29 // SRPRON_XXX Виды местоимений. У местоимения может не быт такого тега (generic)
	#define SRPRON_REFL               1 // PRONREFL Возвратное местоимение (reflexive pronoun) [self]
	#define SRPRON_DEMONSTR           2 // PRONDEM  Указательное местоимение (pronomina demonstrativa) [one]
	#define SRPRON_PERSONAL           3 // PRONPERS Личное местоимение
#define SRWG_COUNTAB                 30 // SRCTB_XXX Исчисляемость существительных
	#define SRCTB_UNCOUNTABLE         1 // NUNCNT Неисчисляемое существительное (english)
	#define SRCTB_MASS                2 // NMASS  mass-существительные (english)
//
// --------------------------------------------------------
//
typedef uint32 LEXID;
typedef int64  CONCEPTID;
typedef int64  NGID;
//
// Типы грамматических правил преобразования форм слова
//
#define SRGRULETYP_AFFIXLIST          1
#define SRGRULETYP_ACCENTLIST         2
//
// Descr: Типы грамматических конструкций
//
#define SRGRAMTYP_WORDFORM            1 // Дескриптор словоформы => SrWordForm
#define SRGRAMTYP_FLEXIAMODEL         2 // FlexiaModel           => SrFlexiaModel
//
//
//
class SrDatabase;
//
//
//
class SrSList : public SBaseBuffer {
public:
	SrSList(int type);
	virtual ~SrSList();
	int    GetType() const;
	size_t GetLength() const;
	int FASTCALL Copy(const SrSList & rS);
	virtual void Clear();
	virtual int IsEqual(const SrSList & rS) const;
	virtual int Validate() const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
protected:
	int32  Type;
	uint32 Len;
};
//
// Descr: Обобщенное представление атрибутов словоформы.
//
class SrWordForm : public SrSList { // @transient
public:
	SrWordForm();
	SrWordForm & FASTCALL operator = (const SrWordForm & rS);
	SrWordForm & FASTCALL Copy(const SrWordForm & rS);
	//
	// Descr: Формирует новый дескриптор слоформы слиянием базового дескриптора и дескриптора частной формы.
	// Note: Функция не коммутативна: Merge(base_form, var_form) != Merge(var_form, base_form)
	//   После объединения результирующая словоформа нормализуется вызовом Normalize()
	// ARG(rBase  IN): ссылка на базовую слоформу
	// ARG(rVar   IN): ссылка на словоформу, тэги которой вливаются в базовую.
	// ARG(mode   IN): режим установки тэгов:
	//   0 - если не существует, то создается новый, если уже существует - заменяется.
	//   1 - если не существует, то ничего не делать, если уже существует - заменяется.
	//   2 - если не существует, то создается новый, если уже существует, то ничего не делать.
	//
	SrWordForm & Merge(const SrWordForm & rBase, const SrWordForm & rVar, int mode = 0);
	int    Normalize();
	int    FASTCALL IsEqual(const SrWordForm & rS) const;
	//
	// Descr: Сопоставляет словоформу this со словоформой rS.
	//   Возвращает значение эквивалентности как Real-число. Чем больше число, тем
	//   больше сохожесть сопоставляемых словоформ.
	// Note: Перечисление ведется по элементам this, каждый из которых сопоставляется с
	//   аналогичным тэгом в rS. Таким образом, если необходимо сопоставить определитель словоформы A,
	//   содержащий неполное описание, с полным описанием B, то следует вызывать функцию в виде A.MatchScore(B).
	//
	double FASTCALL MatchScore(const SrWordForm & rS) const;
	//
	// Descr: Устанавливает тэг tag в значение val.
	// ARG(tag  IN): идентификатор тэга словоформы.
	// ARG(val  IN): значение тэга tag словоформы.
	// ARG(mode IN): режим установки тэга:
	//   0 - если не существует, то создается новый, если уже существует - заменяется.
	//   1 - если не существует, то ничего не делать, если уже существует - заменяется.
	//   2 - если не существует, то создается новый, если уже существует, то ничего не делать.
	// Returns:
	//   1 - значение тэга было заменено
	//   2 - был добавлен новый тэг с указанным значением
	//  -1 - ничего не изменилось
	//   0 - ошибка
	//
	int    SetTag(int tag, int val, int mode = 0);
	int    FASTCALL RemoveTag(int tag);
	int    FASTCALL GetTag(int tag) const;
	size_t CalcLength() const;
	int    FASTCALL ToStr(SString & rBuf) const;
	int    FASTCALL FromStr(const char * pStr);
private:
	void   CatTok(SString & rBuf, const char * pTok) const;
	size_t Set(size_t pos, int val);
	int    FASTCALL Tag(size_t pos) const;
	int    FASTCALL Get(size_t pos) const;
	size_t FASTCALL Step(size_t pos) const;
};
//
// Descr: Представление вариантов словоформ в зависимости от окончания и, возможно, приставки.
//
class SrFlexiaModel : public SrSList { // @transient
public:
	struct Item {
		LEXID  AffixID;
		LEXID  PrefixID;
		int32  WordFormID;
	};
	SrFlexiaModel();
	int    Normalize();
	int    FASTCALL IsEqual(const SrFlexiaModel & rS) const;
	int    FASTCALL Add(const SrFlexiaModel::Item & rItem);
	int    Search(LEXID afxID, LEXID pfxID, LongArray & rWordFormList) const;
	int    GetNext(size_t * pPos, SrFlexiaModel::Item & rItem) const;
private:
	enum {
		fmiZeroAffix  = 0x01,
		fmiZeroPrefix = 0x02,
		fmiWf16       = 0x04
	};
};
//
//
//
struct SrWordAssoc {
	SLAPI  SrWordAssoc();
	SrWordAssoc & SLAPI Normalize();
	SString & FASTCALL ToStr(SString & rBuf) const;

	enum {
		fHasFlexiaModel = 0x0001,
		fHasAccentModel = 0x0002,
		fHasPrefix      = 0x0004,
		fHasAffixModel  = 0x0008
	};

	int32  ID;             // Уникальный идент ассоциации
	LEXID  WordID;         // Идентификатор слова
	long   Flags;          // @flags
	int32  BaseDescrID;
	int32  FlexiaModelID;
	int32  AccentModelID;
	int32  PrefixID;
	int32  AffixModelID;
};
//
// {id;word}
//
class SrWordTbl : public BDbTable {
public:
	enum {
		spcEmpty = 1,
		spcPrefix,
		spcAffix,
		spcConcept,
		spcCPropSymb
	};
	SrWordTbl(BDbDatabase * pDb);
	~SrWordTbl();
	int    Add(const char * pWordUtf8, LEXID * pID);
	int    AddSpecial(int spcTag, const char * pWordUtf8, LEXID * pID);
	int    Search(const char * pWordUtf8, LEXID * pID);
	int    SearchSpecial(int spcTag, const char * pWordUtf8, LEXID * pID);
	int    Search(LEXID id, SString & rBuf);
private:
	int    MakeSpecial(int spcTag, const char * pWordUtf8, SString & rBuf);

	long   SeqID;
};
//
// Descr:
//   Таблица, хранящая сложные грамматические конструкции. Как-то:
//     -- дескрипторы словоформ
//     -- FlexiaModel (структура окончаний, ассоциированных со словоформами)
//     -- Варианты суффиксов (для проверки орфографии)
//     -- и т.д.
// Структура записи:
// {
//     int32 ID;      // Идентификатор записи
//     int16 Type;    // Тип данных, хранящихся в записи
//     uint8 Tail[];  // Собственно грамматическая конструкция. Формат хранения целиком зависит от Type.
// }
//
// Дополнительный индекс таблицы построен по паре {Type; Tail}.
//
class SrGrammarTbl : public BDbTable {
public:
	SrGrammarTbl(BDbDatabase * pDb);
	~SrGrammarTbl();
	int    Add(SrWordForm * pWf, long * pID);
	int    Add(SrFlexiaModel * pFm, long * pID);
	int    Search(long id, SrWordForm * pWf);
	int    Search(long id, SrFlexiaModel * pFm);
	int    Search(SrWordForm * pWf, long * pID);
	int    Search(SrFlexiaModel * pFm, long * pID);
private:
	int    Helper_Add(SrSList * pL, long * pID);
	int    Helper_Search(SrSList * pL, long * pID);
	int    Helper_Search(long id, SrSList * pL);

	long   SeqID;
};
//
//
//
class SrWordAssocTbl : public BDbTable {
public:
	SrWordAssocTbl(BDbDatabase * pDb);
	~SrWordAssocTbl();
	int    Add(SrWordAssoc * pWa, int32 * pID);
	int    Update(int32 id, SrWordAssoc * pWa);
	int    Search(int32 id, SrWordAssoc * pWa);
	int    Search(LEXID wordID, TSArray <SrWordAssoc> & rList);
	int    SerializeRecBuf(int dir, SrWordAssoc * pWa, SBuffer & rBuf);
private:
	long   SeqID;
};
//
// Descr:
//
class SrNGram {
public:
	SrNGram();
	NGID   ID;
	int32  Ver;
	LongArray WordIdList;
};
//
// Descr: Таблица N-грамм. Содержит идентифицированные списки ссылок на слова, составлющие выражения.
//   Например: "Мария Стюарт" хранится как пара ссылок на слова "мария" и "стюарт" в таблице SrWordTbl.
//
class SrNGramTbl : public BDbTable {
public:
	SrNGramTbl(BDbDatabase * pDb);
	~SrNGramTbl();
	int    Add(SrNGram & rNGram);
	int    Search(NGID id, SrNGram * pNGram);
	int    Search(const SrNGram & rNGram, NGID * pID);
	int    SearchByPrefix(const SrNGram & rNGram, TSArray <NGID> & rList);

	int    SerializeRecBuf(int dir, SrNGram * pRec, SBuffer & rBuf);
private:
	long   SeqID;
};

#define SRPROPT_UNKN      0
#define SRPROPT_INT       1
#define SRPROPT_STRING    2
#define SRPROPT_REAL      3
#define SRPROPT_HDATE     4
#define SRPROPT_HPERIOD   5

class SrCPropDecl {
public:
	SrCPropDecl();
	~SrCPropDecl();
	int    FASTCALL IsEqual(const SrCPropDecl & rS) const;
//private:
	CONCEPTID PropID; // ИД концепции-свойства
	LEXID  SymbID;    // Опциональный идентификатор символа свойства
	SBaseBuffer Tail; // Вариабельная часть спецификации свойства, которая может содержать ограничения значений параметров
};

class SrCPropDeclList {
public:
	SrCPropDeclList();
	~SrCPropDeclList();
	SrCPropDeclList(const SrCPropDeclList & rS);
	SrCPropDeclList & FASTCALL operator = (const SrCPropDeclList & rS);
	SrCPropDeclList & FASTCALL Copy(const SrCPropDeclList & rS);
	SrCPropDeclList & Clear();
	uint   GetCount() const
	{
		return D.getCount();
	}
	int    FASTCALL Add(const SrCPropDecl & rP);
	int    Replace(uint pos, const SrCPropDecl & rP);
	int    Get(uint idx, SrCPropDecl & rP) const;
	int    GetBySymbID(LEXID id, SrCPropDecl & rP) const;
	int    Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx);
	int    FASTCALL IsEqual(const SrCPropDeclList & rS) const;
	int    FASTCALL Merge(const SrCPropDeclList & rS);
private:
	struct Item {
		CONCEPTID PropID;
		LEXID  SymbID;
		uint32 TailS;  // Размер хвостовой части
		uint32 TailP;  // Позиция хвостовой части дескриптора свойства в буфере Pool
	};
	TSArray <Item> D;
	uint32 PoolP; // Позиция, с которой следует добавлять следующий элемент в Pool
	SBaseBuffer Pool;
};

class SrConcept {
public:
	SrConcept();
	int    FASTCALL IsEqual(const SrConcept & rS) const;
	SrConcept & Clear();

	CONCEPTID ID;
	LEXID  SymbID;
	int32  Ver;    // Версия формата записи (0..)
	SrCPropDeclList Pdl;
};
//
// Descr: Таблица концепций. Концепция определяется только идентификатором.
//   В дополнение к идентификатору запись концепции может содержать ссылку на
//   метасимвол (из таблицы SrWordTbl) и список типов свойств.
//   Концепция может быть создана без ссылки на символ. В таком случае одним из
//   свойств концепции должно быть обозначение на реальном (не мета-) языке.
// {id;symbol-ref;prop-decl-list}
//
class SrConceptTbl : public BDbTable {
public:
	SrConceptTbl(BDbDatabase * pDb);
	~SrConceptTbl();
	int    Add(SrConcept & rRec);
	int    Update(SrConcept & rRec);
	int    Remove(CONCEPTID id);
	int    SetPropDeclList(CONCEPTID id, SrCPropDeclList * pPdl);
	int    SearchByID(CONCEPTID id, SrConcept * pRec);
	int    SearchBySymb(LEXID symbId, SrConcept * pRec);

	int    SerializeRecBuf(int dir, SrConcept * pRec, SBuffer & rBuf);
private:
	long   SeqID;
};
//
// Descr: Значение свойства концепции
//
class SrCProp {
public:
	SrCProp();
	SrCProp(CONCEPTID cID, CONCEPTID propID);
	int    FASTCALL IsEqual(const SrCProp & rS) const;
	SrCProp & Clear();
	SrCProp & FASTCALL operator = (int);
	SrCProp & FASTCALL operator = (int64);
	SrCProp & FASTCALL operator = (double);
	SrCProp & FASTCALL operator = (const char *);

	int    FASTCALL Get(int64 & rIntVal) const;
	int    FASTCALL Get(double & rRealVal) const;

	CONCEPTID CID;
	CONCEPTID PropID;
	SBuffer Value;
};
//
// Descr: Список значений свойств концепции
//
class SrCPropList {
public:
	SrCPropList();
	int    Set(CONCEPTID cID, CONCEPTID propID, const void * pData, size_t dataLen);
	SrCPropList & Clear();
	int    Search(CONCEPTID cID, CONCEPTID propID, uint * pPos) const;
	size_t FASTCALL GetDataLen(uint pos) const;
	const  void * GetDataPtr(uint pos, size_t * pDataLen) const;
	size_t GetData(uint pos, void * pData, size_t bufLen) const;
	uint   GetCount() const;
	int    GetProp(uint pos, SrCProp & rProp) const;
private:
	struct Item {
		CONCEPTID CID;
		CONCEPTID PropID;
		uint32 P;         // Позиция данных в буфере SrCPropList::D
		uint32 S;         // Длина данных в буфере SrCPropList::D
	};
	int    SetData(uint pos, const void * pData, size_t dataLen);
	TSArray <Item> L;
	SBuffer D;
};
//
// Descr: Таблица хранения свойств концепций
// {concept-id; prop-id; prop-value}
//
class SrConceptPropTbl : public BDbTable {
public:
	SrConceptPropTbl(SrDatabase & rSr);
	int    Set(SrCProp & rProp);
	int    Search(SrCProp & rRec);
	int    GetPropIdList(CONCEPTID cID, Int64Array & rPropIdList);
	int    GetPropList(CONCEPTID cID, SrCPropList & rPropList);
	int    Remove(CONCEPTID cID, CONCEPTID propID);

	int    SerializeRecBuf(int dir, SrCProp * pRec, SBuffer & rBuf);

	static int FASTCALL EncodePrimeKey(BDbTable::Buffer & rKeyBuf, const SrCProp & rRec);
	static int FASTCALL DecodePrimeKey(const BDbTable::Buffer & rKeyBuf, SrCProp & rRec);
private:
	SrDatabase & R_Sr; // @notowned
};
//
// Descr: Таблица соответствий между N-граммами и концепциями. Фактически, это -
//   таблица, обеспечивающая связку между словосочетаниями (словами) на естественных языках
//   и формализованными концепциями.
// {concept-id, ngram-id}
//
class SrConceptNgTbl : public BDbTable {
public:
	SrConceptNgTbl(BDbDatabase * pDb);
	int    Set(CONCEPTID cID, NGID ngID);
	int    GetNgList(CONCEPTID cID, Int64Array & rNgList);
	int    GetConceptList(NGID ngID, Int64Array & rConceptList);
};
//
//
//
class SrGeoNodeTbl : public BDbTable {
public:
	SLAPI  SrGeoNodeTbl(BDbDatabase * pDb);
	SLAPI ~SrGeoNodeTbl();
	int    SLAPI Add(PPOsm::NodeCluster & rNc, uint64 outerID);
	int    SLAPI Update(PPOsm::NodeCluster & rNc, uint64 outerID);
	int    SLAPI Search(uint64 id, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID);
	int    SLAPI Search(uint64 id, PPOsm::NodeCluster * pCluster, uint64 * pLogicalID);
private:
	int    SLAPI Helper_Set(PPOsm::NodeCluster & rNc, uint64 outerID, int update);
	int    SLAPI Helper_Search(uint64 id, PPOsm::NodeCluster * pCluster, PPOsm::Node * pNode, PPOsm::NodeRefs * pNrList, uint64 * pLogicalID);
	//
	// Буферы для временного использования. Определены как члены класса
	// дабы избежать частых распределений памяти.
	//
	BDbTable::Buffer KeyBuf;
	BDbTable::Buffer DataBuf;
};

class SrGeoWayTbl : public BDbTable {
public:
	SLAPI  SrGeoWayTbl(BDbDatabase * pDb);
	SLAPI ~SrGeoWayTbl();
	int    SLAPI Add(PPOsm::Way & rW, PPOsm::WayBuffer * pBuffer);
	int    SLAPI Search(uint64 id, PPOsm::Way * pW);
private:
	//
	// Буферы для временного использования. Определены как члены класса
	// дабы избежать частых распределений памяти.
	//
	BDbTable::Buffer KeyBuf;
	BDbTable::Buffer DataBuf;
};
//
//
//
class SrImportParam {
public:
	enum {
		inpFlexiaModel = 1,
		inpMySpell
	};
	enum {
		fldAncodeFileName = 1,
		fldFlexiaModelFileName
	};
	enum {
		fTest = 0x0001
	};
	SrImportParam();
	int    SetField(int fld, const char * pVal);
	int    GetField(int fld, SString & rVal) const;

	int    InputKind;
	int    LangID;    // Идентификатор языка
	int    CpID;      // Идентификатор кодовой страницы входных файлов
	long   Flags;     // @flags
	StrAssocArray StrItems; // Список текстовых параметров
};

struct SrWordInfo {
	SrWordInfo();
	SrWordInfo & Clear();

	LEXID  BaseID;     // Идентификатор неизменяемой базы слова
	LEXID  PrefixID;   // Идентификатор приставки слова
	LEXID  AffixID;    // Идентификатор окончания слова
	//
	int32  BaseFormID; // Базовый дескриптор словоформы, ассоциированный с BaseID
	int32  FormID;     // Идентификатор словоформы найденного слова
	// Для того, чтобы получить итоговое описание словоформы необходимо сцепить словоформы BaseFormID и FormID
	int32  WaID;       // Идентификатор дескриптивной ассоциации слова
	double Score;      // Величина сопоставления данной грамматической формы с требуемой (при преобразовании слова)
};

class SrDatabase {
public:
	enum {
		rcInstance = 1,
		rcSubclass,
		rcType
	};
	SrDatabase();
	~SrDatabase();
	int    Open(const char * pDbPath);
	int    Close();
	operator BDbDatabase *()
	{
		return P_Db;
	}

	CONCEPTID GetReservedConcept(int rc);

	int    GetWordInfo(const char * pWordUtf8, long flags, TSArray <SrWordInfo> & rInfo);
	int    WordInfoToStr(const SrWordInfo & rWi, SString & rBuf);
	int    Transform(const char * pWordUtf8, const SrWordForm * pDestForm, TSArray <SrWordInfo> & rResult);

	int    SearchWord(int special, const char * pWordUtf8, LEXID * pID);
	int    SearchNGram(const LongArray & rNg, NGID * pID);

	enum {
		ngclAnonymOnly = 0x0001
	};
	//
	// Descr: Возвращает список идентификаторов концепций, ассоциированных с N-граммой ngID.
	// ARG(ngID  IN): Идентификатор N-граммы, для которой ищутся концепции
	// ARG(flags IN): BDbDatabase::ngclXXX Опции поиска
	// ARG(rConceptList OUT): Список найденных идентификаторов концепций
	// Returns:
	//   >0 - найдена по крайней мере одна концепция, ассоциированная с ngID
	//   <0 - не найдено ни одной концепции, удовлетворяющей условиям
	//   0  - ошибка
	//
	int    GetNgConceptList(NGID ngID, long flags, Int64Array & rConceptList);
	int    GetConceptHier(CONCEPTID cID, Int64Array & rConceptHier);
	int    GetConceptSymb(CONCEPTID cID, SString & rSymbUht8);
	int    GetPropDeclList(CONCEPTID cID, SrCPropDeclList & rPdl);
	int    GetConceptPropList(CONCEPTID cID, SrCPropList & rPl);

	int    GetPropType(CONCEPTID propID);

	int    ResolveConcept(const char * pSymbUtf8, CONCEPTID * pID);
	int    CreateAnonymConcept(CONCEPTID * pID);
	int    ResolveWord(const char * pWordUtf8, LEXID * pID);
	int    ResolveNGram(const LongArray & rList, NGID * pID);
	int    ResolveCPropSymb(const char * pSymbUtf8, LEXID * pID);

	int    MakeConceptPropC(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const char * pConceptSymb);
	int    MakeConceptPropNg(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, const LongArray & rNg);
	int    MakeConceptPropN(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, double value);
	int    FormatProp(const SrCProp & rCp, long flags, SString & rBuf);

	int    ImportFlexiaModel(const SrImportParam & rParam);
	int    StoreGeoNodeList(const TSArray <PPOsm::Node> & rList, TSArray <PPOsm::NodeClusterStatEntry> * pStat);
	int    StoreGeoWayList(const TSCollection <PPOsm::Way> & rList, TSArray <PPOsm::WayStatEntry> * pStat);
	int    StoreGeoNodeWayRefList(const LLAssocArray & rList);
//private:
public:
	BDbDatabase      * P_Db;
	SrGrammarTbl     * P_GrT;
	SrWordTbl        * P_WdT;
	SrWordAssocTbl   * P_WaT;
	SrConceptTbl     * P_CT;
	SrConceptPropTbl * P_CpT;
	SrNGramTbl       * P_NgT;
	SrConceptNgTbl   * P_CNgT;
	SrGeoNodeTbl     * P_GnT;
	SrGeoWayTbl      * P_GwT;
	LEXID  ZeroWordID;
private:
	int    Helper_GetConceptHier(CONCEPTID cID, Int64Array & rConceptHier);
	int    Helper_MakeConceptProp(const SrCPropDeclList & rPdl, const char * pPropSymb, SrCProp & rProp, CONCEPTID cID);

	CONCEPTID PropInstance; // :crp_instance
	CONCEPTID PropSubclass; // :crp_subclass
	CONCEPTID PropType;     // :crp_type
};
