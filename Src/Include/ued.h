// UED.H
// Copyright (c) A.Sobolev 2023
//
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
};
//
//
//
class SrUedContainer : public SStrGroup {
public:
	SrUedContainer();
	~SrUedContainer();
	int    ReadSource(const char * pFileName, PPLogger * pLogger);
	int    WriteSource(const char * pFileName, const SBinaryChunk * pPrevHash, SBinaryChunk * pHash);
	//
	// Descr: Верифицирует UED-файл версии ver и находящийся в каталоге pPath 
	//   на предмет наличия и соответствия хэша, хранящегося в отдельном файле в том же каталоге.
	//
	int    Verify(const char * pPath, long ver, SBinaryChunk * pHash);
	//
	// Descr: Верифицирует this-контейнер на непротиворечивость и, если указан контейнер
	//   предыдущей версии (pPrevC != 0), то проверяет инварианты.
	//
	int    VerifyByPreviousVersion(const SrUedContainer * pPrevC, PPLogger * pLogger);
	uint64 SearchBaseSymb(const char * pSymb, uint64 meta) const;
	bool   SearchBaseId(uint64 id, SString & rSymb) const;
	bool   GenerateSourceDecl_C(const char * pFileName, uint versionN, const SBinaryChunk & rHash);
	bool   GenerateSourceDecl_Java(const char * pFileName, uint versionN, const SBinaryChunk & rHash);

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
	TSVector <BaseEntry> BL;
	TSVector <TextEntry> TL;

	static void MakeUedCanonicalName(SString & rResult, long ver);
	static long SearchLastCanonicalFile(const char * pPath, SString & rFileName);
private:
	uint64 SearchBaseIdBySymbId(uint symbId, uint64 meta) const;
	int    ReplaceSurrogateLocaleIds(const SymbHashTable & rT, PPLogger * pLogger);
	uint64 LinguaLocusMeta;
	SymbHashTable Ht; // Хэш-таблица символов из списка BL
	uint   LastSymbHashId;
};

