// MethodProps.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;

bool FASTCALL StringToBool(const wchar_t * s, bool &res)
{
	if(s[0] == 0 || (s[0] == '+' && s[1] == 0) || sstreqi_ascii(s, "ON")) {
		res = true;
		return true;
	}
	else if((s[0] == '-' && s[1] == 0) || sstreqi_ascii(s, "OFF")) {
		res = false;
		return true;
	}
	else
		return false;
}

HRESULT FASTCALL PROPVARIANT_to_bool(const PROPVARIANT &prop, bool &dest)
{
	switch(prop.vt) {
		case VT_EMPTY: dest = true; return S_OK;
		case VT_BOOL: dest = (prop.boolVal != VARIANT_FALSE); return S_OK;
		case VT_BSTR: return StringToBool(prop.bstrVal, dest) ? S_OK : E_INVALIDARG;
	}
	return E_INVALIDARG;
}

uint FASTCALL ParseStringToUInt32(const UString &srcString, uint32 &number)
{
	const wchar_t * start = srcString;
	const wchar_t * end;
	number = ConvertStringToUInt32(start, &end);
	return (uint)(end - start);
}

HRESULT FASTCALL ParsePropToUInt32(const UString &name, const PROPVARIANT & prop, uint32 & resValue)
{
	// =VT_UI4
	// =VT_EMPTY
	// {stringUInt32}=VT_EMPTY
	if(prop.vt == VT_UI4) {
		if(!name.IsEmpty())
			return E_INVALIDARG;
		resValue = prop.ulVal;
		return S_OK;
	}
	if(prop.vt != VT_EMPTY)
		return E_INVALIDARG;
	if(name.IsEmpty())
		return S_OK;
	uint32 v;
	if(ParseStringToUInt32(name, v) != name.Len())
		return E_INVALIDARG;
	resValue = v;
	return S_OK;
}

HRESULT ParseMtProp(const UString &name, const PROPVARIANT &prop, uint32 defaultNumThreads, uint32 &numThreads)
{
	if(name.IsEmpty()) {
		switch(prop.vt) {
			case VT_UI4:
			    numThreads = prop.ulVal;
			    break;
			default:
		    {
			    bool val;
			    RINOK(PROPVARIANT_to_bool(prop, val));
			    numThreads = (val ? defaultNumThreads : 1);
			    break;
		    }
		}
		return S_OK;
	}
	if(prop.vt != VT_EMPTY)
		return E_INVALIDARG;
	return ParsePropToUInt32(name, prop, numThreads);
}

static HRESULT StringToDictSize(const UString &s, NCOM::CPropVariant &destProp)
{
	const wchar_t * end;
	uint32 number = ConvertStringToUInt32(s, &end);
	unsigned numDigits = (uint)(end - s.Ptr());
	if(numDigits == 0 || s.Len() > numDigits + 1)
		return E_INVALIDARG;
	if(s.Len() == numDigits) {
		if(number >= 64)
			return E_INVALIDARG;
		if(number < 32)
			destProp = (uint32)((uint32)1 << (uint)number);
		else
			destProp = (uint64)((uint64)1 << (uint)number);
		return S_OK;
	}
	unsigned numBits;
	switch(MyCharLower_Ascii(s[numDigits])) {
		case 'b': destProp = number; return S_OK;
		case 'k': numBits = 10; break;
		case 'm': numBits = 20; break;
		case 'g': numBits = 30; break;
		default: return E_INVALIDARG;
	}
	if(number < ((uint32)1 << (32 - numBits)))
		destProp = (uint32)(number << numBits);
	else
		destProp = (uint64)((uint64)number << numBits);
	return S_OK;
}

static HRESULT PROPVARIANT_to_DictSize(const PROPVARIANT &prop, NCOM::CPropVariant &destProp)
{
	if(prop.vt == VT_UI4) {
		uint32 v = prop.ulVal;
		if(v >= 64)
			return E_INVALIDARG;
		if(v < 32)
			destProp = (uint32)((uint32)1 << (uint)v);
		else
			destProp = (uint64)((uint64)1 << (uint)v);
		return S_OK;
	}
	if(prop.vt == VT_BSTR) {
		UString s;
		s = prop.bstrVal;
		return StringToDictSize(s, destProp);
	}
	return E_INVALIDARG;
}

void CProps::Clear() 
{
	Props.Clear();
}

bool CProps::AreThereNonOptionalProps() const
{
	FOR_VECTOR(i, Props) {
		if(!Props[i].IsOptional)
			return true;
	}
	return false;
}

void CProps::AddProp32(PROPID propid, uint32 val)
{
	CProp &prop = Props.AddNew();
	prop.IsOptional = true;
	prop.Id = propid;
	prop.Value = (uint32)val;
}

void CProps::AddPropBool(PROPID propid, bool val)
{
	CProp & prop = Props.AddNew();
	prop.IsOptional = true;
	prop.Id = propid;
	prop.Value = val;
}

void CProps::AddProp_Ascii(PROPID propid, const char * s)
{
	CProp & prop = Props.AddNew();
	prop.IsOptional = true;
	prop.Id = propid;
	prop.Value = s;
}

class CCoderProps {
public:
	CCoderProps(unsigned numPropsMax) : _numPropsMax(numPropsMax), _numProps(0), _propIDs(new PROPID[numPropsMax]), _props(new NCOM::CPropVariant[numPropsMax])
	{
	}
	~CCoderProps()
	{
		delete []_propIDs;
		delete []_props;
	}
	void AddProp(const CProp &prop);
	HRESULT SetProps(ICompressSetCoderProperties * setCoderProperties)
	{
		return setCoderProperties->SetCoderProperties(_propIDs, _props, _numProps);
	}
private:
	PROPID * _propIDs;
	NCOM::CPropVariant * _props;
	unsigned _numProps;
	unsigned _numPropsMax;
};

void CCoderProps::AddProp(const CProp &prop)
{
	if(_numProps >= _numPropsMax)
		throw 1;
	_propIDs[_numProps] = prop.Id;
	_props[_numProps] = prop.Value;
	_numProps++;
}

HRESULT CProps::SetCoderProps(ICompressSetCoderProperties * scp, const uint64 * dataSizeReduce) const
{
	CCoderProps coderProps(Props.Size() + (dataSizeReduce ? 1 : 0));
	FOR_VECTOR(i, Props) {
		coderProps.AddProp(Props[i]);
	}
	if(dataSizeReduce) {
		CProp prop;
		prop.Id = NCoderPropID::kReduceSize;
		prop.Value = *dataSizeReduce;
		coderProps.AddProp(prop);
	}
	return coderProps.SetProps(scp);
}

int CMethodProps::Get_NumThreads() const
{
	int i = FindProp(NCoderPropID::kNumThreads);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4)
			return (int)Props[i].Value.ulVal;
	return -1;
}

bool CMethodProps::Get_DicSize(uint32 &res) const
{
	res = 0;
	int i = FindProp(NCoderPropID::kDictionarySize);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4) {
			res = Props[i].Value.ulVal;
			return true;
		}
	return false;
}

uint32 CMethodProps::Get_Lzma_Algo() const
{
	int i = FindProp(NCoderPropID::kAlgorithm);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4)
			return Props[i].Value.ulVal;
	return GetLevel() >= 5 ? 1 : 0;
}

uint32 CMethodProps::Get_Lzma_DicSize() const
{
	int i = FindProp(NCoderPropID::kDictionarySize);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4)
			return Props[i].Value.ulVal;
	int level = GetLevel();
	return level <= 5 ? (1 << (level * 2 + 14)) : (level == 6 ? (1 << 25) : (1 << 26));
}

bool CMethodProps::Get_Lzma_Eos() const
{
	int i = FindProp(NCoderPropID::kEndMarker);
	if(i >= 0) {
		const NWindows::NCOM::CPropVariant &val = Props[i].Value;
		if(val.vt == VT_BOOL)
			return VARIANT_BOOLToBool(val.boolVal);
	}
	return false;
}

bool CMethodProps::Are_Lzma_Model_Props_Defined() const
{
	if(FindProp(NCoderPropID::kPosStateBits) >= 0) return true;
	if(FindProp(NCoderPropID::kLitContextBits) >= 0) return true;
	if(FindProp(NCoderPropID::kLitPosBits) >= 0) return true;
	return false;
}

uint32 CMethodProps::Get_Lzma_NumThreads() const
{
	if(Get_Lzma_Algo() == 0)
		return 1;
	else {
		int numThreads = Get_NumThreads();
		return (numThreads >= 0) ? ((numThreads < 2) ? 1 : 2) : 2;
	}
}

uint32 CMethodProps::Get_Lzma2_NumThreads(bool &fixedNumber) const
{
	fixedNumber = false;
	int numThreads = Get_NumThreads();
	if(numThreads >= 0) {
		fixedNumber = true;
		if(numThreads < 1) 
			return 1;
		else {
			const uint kNumLzma2ThreadsMax = 32;
			return (numThreads > kNumLzma2ThreadsMax) ? kNumLzma2ThreadsMax : numThreads;
		}
	}
	else
		return 1;
}

uint64 CMethodProps::Get_Lzma2_BlockSize() const
{
	int i = FindProp(NCoderPropID::kBlockSize);
	if(i >= 0) {
		const NWindows::NCOM::CPropVariant &val = Props[i].Value;
		if(val.vt == VT_UI4) return val.ulVal;
		if(val.vt == VT_UI8) return val.uhVal.QuadPart;
	}
	uint32 dictSize = Get_Lzma_DicSize();
	uint64 blockSize = (uint64)dictSize << 2;
	const uint32 kMinSize = (uint32)1 << 20;
	const uint32 kMaxSize = (uint32)1 << 28;
	if(blockSize < kMinSize) blockSize = kMinSize;
	if(blockSize > kMaxSize) blockSize = kMaxSize;
	if(blockSize < dictSize) blockSize = dictSize;
	return blockSize;
}

uint32 CMethodProps::Get_BZip2_NumThreads(bool &fixedNumber) const
{
	fixedNumber = false;
	int numThreads = Get_NumThreads();
	if(numThreads >= 0) {
		fixedNumber = true;
		if(numThreads < 1) return 1;
		const uint kNumBZip2ThreadsMax = 64;
		if(numThreads > kNumBZip2ThreadsMax) return kNumBZip2ThreadsMax;
		return numThreads;
	}
	return 1;
}

uint32 CMethodProps::Get_BZip2_BlockSize() const
{
	int i = FindProp(NCoderPropID::kDictionarySize);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4) {
			uint32 blockSize = Props[i].Value.ulVal;
			const uint32 kDicSizeMin = 100000;
			const uint32 kDicSizeMax = 900000;
			if(blockSize < kDicSizeMin) blockSize = kDicSizeMin;
			if(blockSize > kDicSizeMax) blockSize = kDicSizeMax;
			return blockSize;
		}
	int level = GetLevel();
	return 100000 * (level >= 5 ? 9 : (level >= 1 ? level * 2 - 1 : 1));
}

uint32 CMethodProps::Get_Ppmd_MemSize() const
{
	int i = FindProp(NCoderPropID::kUsedMemorySize);
	if(i >= 0)
		if(Props[i].Value.vt == VT_UI4)
			return Props[i].Value.ulVal;
	int level = GetLevel();
	return level >= 9 ? (192 << 20) : ((uint32)1 << (level + 19));
}

void CMethodProps::AddProp_Level(uint32 level) { AddProp32(NCoderPropID::kLevel, level); }
void CMethodProps::AddProp_NumThreads(uint32 numThreads) { AddProp32(NCoderPropID::kNumThreads, numThreads); }
	
void CMethodProps::AddProp_EndMarker_if_NotFound(bool eos)
{
	if(FindProp(NCoderPropID::kEndMarker) < 0)
		AddPropBool(NCoderPropID::kEndMarker, eos);
}

int CMethodProps::FindProp(PROPID id) const
{
	for(int i = Props.Size() - 1; i >= 0; i--)
		if(Props[i].Id == id)
			return i;
	return -1;
}

int CMethodProps::GetLevel() const
{
	int i = FindProp(NCoderPropID::kLevel);
	if(i < 0)
		return 5;
	if(Props[i].Value.vt != VT_UI4)
		return 9;
	uint32 level = Props[i].Value.ulVal;
	return level > 9 ? 9 : (int)level;
}

struct CNameToPropID {
	VARTYPE VarType;
	const char * Name;
};

static const CNameToPropID g_NameToPropID[] = {
	{ VT_UI4, "" }, { VT_UI4, "d" }, { VT_UI4, "mem" }, { VT_UI4, "o" }, { VT_UI4, "c" }, { VT_UI4, "pb" },
	{ VT_UI4, "lc" }, { VT_UI4, "lp" }, { VT_UI4, "fb" }, { VT_BSTR, "mf" }, { VT_UI4, "mc" }, { VT_UI4, "pass" },
	{ VT_UI4, "a" }, { VT_UI4, "mt" }, { VT_BOOL, "eos" }, { VT_UI4, "x" }, { VT_UI4, "reduceSize" }
};

static int FindPropIdExact(const UString &name)
{
	for(uint i = 0; i < ARRAY_SIZE(g_NameToPropID); i++)
		if(sstreqi_ascii(name, g_NameToPropID[i].Name))
			return i;
	return -1;
}

static bool ConvertProperty(const PROPVARIANT &srcProp, VARTYPE varType, NCOM::CPropVariant &destProp)
{
	if(varType == srcProp.vt) {
		destProp = srcProp;
		return true;
	}
	if(varType == VT_BOOL) {
		bool res;
		if(PROPVARIANT_to_bool(srcProp, res) != S_OK)
			return false;
		destProp = res;
		return true;
	}
	if(srcProp.vt == VT_EMPTY) {
		destProp = srcProp;
		return true;
	}
	return false;
}

static void SplitParams(const UString &srcString, UStringVector &subStrings)
{
	subStrings.Clear();
	uint len = srcString.Len();
	if(len) {
		UString s;
		for(uint i = 0; i < len; i++) {
			wchar_t c = srcString[i];
			if(c == L':') {
				subStrings.Add(s);
				s.Empty();
			}
			else
				s += c;
		}
		subStrings.Add(s);
	}
}

static void SplitParam(const UString &param, UString &name, UString &value)
{
	int eqPos = param.Find(L'=');
	if(eqPos >= 0) {
		name.SetFrom(param, eqPos);
		value = param.Ptr(eqPos + 1);
	}
	else {
		uint i;
		for(i = 0; i < param.Len(); i++) {
			wchar_t c = param[i];
			if(c >= L'0' && c <= L'9')
				break;
		}
		name.SetFrom(param, i);
		value = param.Ptr(i);
	}
}

static bool IsLogSizeProp(PROPID propid)
{
	switch(propid) {
		case NCoderPropID::kDictionarySize:
		case NCoderPropID::kUsedMemorySize:
		case NCoderPropID::kBlockSize:
		case NCoderPropID::kReduceSize:
		    return true;
	}
	return false;
}

HRESULT CMethodProps::SetParam(const UString &name, const UString &value)
{
	int index = FindPropIdExact(name);
	if(index < 0)
		return E_INVALIDARG;
	const CNameToPropID &nameToPropID = g_NameToPropID[(uint)index];
	CProp prop;
	prop.Id = index;
	if(IsLogSizeProp(prop.Id)) {
		RINOK(StringToDictSize(value, prop.Value));
	}
	else {
		NCOM::CPropVariant propValue;
		if(nameToPropID.VarType == VT_BSTR)
			propValue = value;
		else if(nameToPropID.VarType == VT_BOOL) {
			bool res;
			if(!StringToBool(value, res))
				return E_INVALIDARG;
			propValue = res;
		}
		else if(!value.IsEmpty()) {
			uint32 number;
			if(ParseStringToUInt32(value, number) == value.Len())
				propValue = number;
			else
				propValue = value;
		}
		if(!ConvertProperty(propValue, nameToPropID.VarType, prop.Value))
			return E_INVALIDARG;
	}
	Props.Add(prop);
	return S_OK;
}

HRESULT CMethodProps::ParseParamsFromString(const UString &srcString)
{
	UStringVector params;
	SplitParams(srcString, params);
	FOR_VECTOR(i, params) {
		const UString &param = params[i];
		UString name, value;
		SplitParam(param, name, value);
		RINOK(SetParam(name, value));
	}
	return S_OK;
}

HRESULT CMethodProps::ParseParamsFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
	if(realName.Len() == 0) {
		// [empty]=method
		return E_INVALIDARG;
	}
	if(value.vt == VT_EMPTY) {
		// {realName}=[empty]
		UString name, valueStr;
		SplitParam(realName, name, valueStr);
		return SetParam(name, valueStr);
	}

	// {realName}=value
	int index = FindPropIdExact(realName);
	if(index < 0)
		return E_INVALIDARG;
	const CNameToPropID &nameToPropID = g_NameToPropID[(uint)index];
	CProp prop;
	prop.Id = index;

	if(IsLogSizeProp(prop.Id)) {
		RINOK(PROPVARIANT_to_DictSize(value, prop.Value));
	}
	else {
		if(!ConvertProperty(value, nameToPropID.VarType, prop.Value))
			return E_INVALIDARG;
	}
	Props.Add(prop);
	return S_OK;
}

void COneMethodInfo::Clear()
{
	CMethodProps::Clear();
	MethodName.Empty();
	PropsString.Empty();
}

HRESULT COneMethodInfo::ParseMethodFromString(const UString &s)
{
	MethodName.Empty();
	int splitPos = s.Find(L':');
	{
		UString temp = s;
		if(splitPos >= 0)
			temp.DeleteFrom(splitPos);
		if(!temp.IsAscii())
			return E_INVALIDARG;
		MethodName.SetFromWStr_if_Ascii(temp);
	}
	if(splitPos < 0)
		return S_OK;
	PropsString = s.Ptr(splitPos + 1);
	return ParseParamsFromString(PropsString);
}

HRESULT COneMethodInfo::ParseMethodFromPROPVARIANT(const UString &realName, const PROPVARIANT &value)
{
	if(!realName.IsEmpty() && !sstreqi_ascii(realName, "m"))
		return ParseParamsFromPROPVARIANT(realName, value);
	// -m{N}=method
	if(value.vt != VT_BSTR)
		return E_INVALIDARG;
	UString s;
	s = value.bstrVal;
	return ParseMethodFromString(s);
}
