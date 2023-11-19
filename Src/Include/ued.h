// UED.H
// Copyright (c) A.Sobolev 2023
//
#ifndef __UED_H
#define __UED_H

class PPLogger;

class UED {
public:
	static bool IsMetaId(uint64 ued) { return (HiDWord(ued) == 1); }
	//
	// Descr: Возвращает количество бит данных, доступных для значения UED-величины в 
	//   зависимости от мета-идентификатора.
	//   Если аргумент meta не является мета-идентификатором, то возвращает 0.
	//
	static uint GetMetaRawDataBits(uint64 meta);
	//
	// Descr: Возвращает количество бит данных, доступных для значения UED.
	//   Если аргумент является meta-идентификатором или не является валидным UED-значением,
	//   то возвращает 0.
	//
	static uint GetRawDataBits(uint64 ued);
	static bool BelongToMeta(uint64 ued, uint64 meta)
	{
		//return (IsMetaId(meta) && ((ued >> 32) & 0x00000000ffffffffULL) == (meta & 0x00000000ffffffffULL));
		return (IsMetaId(meta) && GetMeta(ued) == meta);
	}
	static uint64 GetMeta(uint64 ued);
	static bool   GetRawValue(uint64 ued, uint64 * pRawValue);
	static bool   GetRawValue32(uint64 ued, uint32 * pRawValue);
	static uint64 ApplyMetaToRawValue(uint64 meta, uint64 rawValue);
	static uint64 ApplyMetaToRawValue32(uint64 meta, uint32 rawValue);
	
	static uint64 SetRaw_GeoLoc(const SGeoPosLL & rGeoPos);
	static bool   GetRaw_GeoLoc(uint64 ued, SGeoPosLL & rGeoPos);
	static uint64 SetRaw_PlanarAngleDeg(double deg);
	static bool   GetRaw_PlanarAngleDeg(uint64 ued, double & rDeg);
	static uint64 SetRaw_Color(const SColor & rC);
	static bool   GetRaw_Color(uint64 ued, SColor & rC);

	static uint64 SetRaw_Ru_INN(const char * pT);
	static bool   GetRaw_Ru_INN(uint64 ued, SString & rT);
	static uint64 SetRaw_Ru_KPP(const char * pT);
	static bool   GetRaw_Ru_KPP(uint64 ued, SString & rT);
	static uint64 SetRaw_Ar_DNI(const char * pT);
	static bool   GetRaw_Ar_DNI(uint64 ued, SString & rT);
	static uint64 SetRaw_Cl_RUT(const char * pT);
	static bool   GetRaw_Cl_RUT(uint64 ued, SString & rT);
	static uint64 SetRaw_EAN13(const char * pT);
	static bool   GetRaw_EAN13(uint64 ued, SString & rT);
	static uint64 SetRaw_EAN8(const char * pT);
	static bool   GetRaw_EAN8(uint64 ued, SString & rT);
	static uint64 SetRaw_UPCA(const char * pT);
	static bool   GetRaw_UPCA(uint64 ued, SString & rT);
	static uint64 SetRaw_GLN(const char * pT);
	static bool   GetRaw_GLN(uint64 ued, SString & rT);

	static bool   _GetRaw_Time(uint64 ued, SUniTime_Internal & rT);
	static uint64 _SetRaw_Time(uint64 meta, const SUniTime_Internal & rT);
private:
	static uint64 Helper_SetRaw_PlanarAngleDeg(uint64 meta, double deg);
	static bool   Helper_GetRaw_PlanarAngleDeg(uint64 meta, uint64 ued, double & rDeg);
};

class UedSetBase : private SBaseBuffer {
protected:
	DECL_INVARIANT_C();
	UedSetBase();
	UedSetBase(const UedSetBase & rS);
	~UedSetBase();
	UedSetBase & FASTCALL operator = (const UedSetBase & rS);
	bool   FASTCALL operator == (const UedSetBase & rS) const;
	UedSetBase & Z()
	{
		LimbCount = 0;
		return *this;
	}
	int    FASTCALL Copy(const UedSetBase & rS);
	bool   FASTCALL IsEq(const UedSetBase & rS) const;
	uint   GetLimbCount() const { return LimbCount; }
	bool   Add(const uint64 * pUed, uint count, uint * pIdx);
	bool   Add(uint64 ued, uint * pIdx);
private:
	uint   LimbCount; // Фактическое количество 64-битных элементов
};
//
// Descr: Базовый класс контейнера UED-объектов
//
class SrUedContainer_Base : public SStrGroup {
public:
	struct BaseEntry {
		uint64 Id;
		uint32 SymbHashId;
		uint32 LineNo; // @v11.7.8 Номер строки исходного файла, с которой начинается определение.
	};
	struct TextEntry {
		uint64 Id;
		uint32 Locale;
		uint32 TextP;
		uint32 LineNo; // @v11.7.8 Номер строки исходного файла, с которой начинается определение.
	};
	static void MakeUedCanonicalName(SString & rResult, long ver);
	static long SearchLastCanonicalFile(const char * pPath, SString & rFileName);
	//
	// Descr: Верифицирует UED-файл версии ver и находящийся в каталоге pPath 
	//   на предмет наличия и соответствия хэша, хранящегося в отдельном файле в том же каталоге.
	//
	int    Verify(const char * pPath, long ver, SBinaryChunk * pHash) const;
	uint64 SearchSymb(const char * pSymb, uint64 meta) const;
	//
	// Descr: Флаги функции Recoginaze
	//
	enum {
		rfDraft       = 0x0001, // Не искать соответствия символов
		rfPrefixSharp = 0x0002  // Символы сущностей должны предваряться знаком '#'
	};

	uint64 Recognize(SStrScan & rScan, uint64 implicitMeta, uint flags) const;
protected:
	SrUedContainer_Base();
	~SrUedContainer_Base();
	//
	// Descr: Флаги функции ReadSource
	//
	enum {
		rfDebug = 0x0001
	};

	int    ReadSource(const char * pFileName, uint flags, PPLogger * pLogger);
	int    WriteSource(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash);
	uint64 SearchBaseSymb(const char * pSymb, uint64 meta) const;
	bool   SearchBaseId(uint64 id, SString & rSymb) const;
	bool   SearchSymbHashId(uint32 symbHashId, SString & rSymb) const;
	//
	// Descr: Распознает простую конструкцию символа сущности в одном из двух
	//   вариантов:
	//   symb или meta.symb
	// Note: Функция не пытается определить достоверность символов. То есть
	//   не ищет их среди определений.
	// Returns:
	//   0 - не удалось распознать символ
	//   1 - распознан единичный символ
	//   2 - распознан вармант meta.symb
	//
	int    Helper_RecognizeSymb(SStrScan & rScan, uint flags, SString & rMeta, SString & rSymb) const;
	//
	// Descr: Определитель свойства сущности, сохраняемый в препроцессинговом виде
	//   так как финишная обработка требует знаний о всем массиве сущностей.
	//
	struct ProtoProp {
		ProtoProp() : Ued(0), LocaleId(0), LineNo(0)
		{
		}
		uint64 Ued;
		uint32 LocaleId; // Если свойство задано для языкового определения сущности, то здесь - 
			// идентификатор соответствующего локуса.
		uint32 LineNo; // Номер строки, на которой начинается определение свойства.
		StringSet Prop; // The first item - property symb, other - arguments
	};
	class ProtoPropList_SingleUed : public TSCollection <ProtoProp> {
	public:
		ProtoPropList_SingleUed(uint64 ued, int localeId);
		void Init(uint64 ued, int localeId);
		ProtoProp * Create();
		uint64 Ued;
		int    LocaleId;
	};
	class PropertyListParsingBlock {
	public:
		PropertyListParsingBlock();
		bool    IsStarted() const { return (Status > 0); }
		bool    Start(uint64 ued, int localeId);
		//
		// Returns:
		//   >0 - разбор завершен (встретился завершающий символ '}')
		//   <0 - разбор не завершен
		//    0 - error
		//
		int    Do(SrUedContainer_Base & rC, SStrScan & rScan);
		const  ProtoPropList_SingleUed & GetResult() const { return PL; }
	private:
		int    ScanProp(SrUedContainer_Base & rC, SStrScan & rScan);
		int    ScanArg(SrUedContainer_Base & rC, SStrScan & rScan, bool isFirst/*@debug*/);
		int    Status; // 0 - idle, 1 - started, 2 - in work (the first '{' was occured and processed)
		int    State;  // Внутренний идентификатор состояния разбора.
		ProtoPropList_SingleUed PL;
	};
	struct PropIdxEntry_Ued {
		uint64 Ued;
		LongArray RefList; // Список позиций в PropertySet
	};
	struct PropIdxEntry_UedLocale {
		uint64 Ued;
		uint   LocaleId;
		LongArray RefList; // Список позиций в PropertySet
	};
	//
	// Descr: Финишный пул свойств, на которые ссылаются элементы UED-реестра.
	//
	class PropertySet : private UedSetBase {
	public:
		PropertySet();
		int    Add(const uint64 * pPropChunk, uint count, uint * pPos);
	private:
		LAssocArray PosIdx; // Индекс позиций: Key - номер позиции, Val - количество элементов 
	};
	TSVector <BaseEntry> BL;
	TSVector <TextEntry> TL;
	TSCollection <ProtoProp> ProtoPropList; // compile-time
private:
	uint64 SearchBaseIdBySymbId(uint symbId, uint64 meta) const;
	int    ReplaceSurrogateLocaleId(const SymbHashTable & rT, uint32 surrLocaleId, uint32 * pRealLocaleId, uint lineNo, PPLogger * pLogger);
	int    ReplaceSurrogateLocaleIds(const SymbHashTable & rT, PPLogger * pLogger);
	int    RegisterProtoPropList(const ProtoPropList_SingleUed & rList);
	int    ProcessProperties();
	uint64 LinguaLocusMeta;
	SymbHashTable Ht; // Хэш-таблица символов из списка BL
	uint   LastSymbHashId;
	PropertySet PropS;
};
//
// Descr: Класс контейнера UED-объектов, реализующий функционал компиляции и сборки.
//
class SrUedContainer_Ct : public SrUedContainer_Base {
public:
	SrUedContainer_Ct();
	~SrUedContainer_Ct();
	int    Read(const char * pFileName, PPLogger * pLogger);
	int    Write(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash);
	bool   GenerateSourceDecl_C(const char * pFileName, uint versionN, const SBinaryChunk & rHash);
	bool   GenerateSourceDecl_Java(const char * pFileName, uint versionN, const SBinaryChunk & rHash);
	//
	// Descr: Верифицирует this-контейнер на непротиворечивость и, если указан контейнер
	//   предыдущей версии (pPrevC != 0), то проверяет инварианты.
	//
	int    VerifyByPreviousVersion(const SrUedContainer_Ct * pPrevC, bool tolerant, PPLogger * pLogger);
};
//
// Descr: Класс контейнера UED-объектов, реализующий функционал применения в run-time'е
//
class SrUedContainer_Rt : public SrUedContainer_Base {
public:
	SrUedContainer_Rt();
	~SrUedContainer_Rt();
	int    Read(const char * pFileName);
	bool   GetSymb(uint64 ued, SString & rSymb) const;
};

enum {
	prcssuedfForceUpdatePlDecl = 0x0001,
	prcssuedfTolerant          = 0x0002,
	prcssuedfDebug             = 0x0004, // @v11.8.10
};

int ProcessUed(const char * pSrcFileName, const char * pOutPath, const char * pRtOutPath, const char * pCPath, const char * pJavaPath, uint flags, PPLogger * pLogger); // @v11.7.10

#endif // __UED_H