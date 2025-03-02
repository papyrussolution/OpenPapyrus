// List.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
using namespace NCOM;

extern CStdOutStream * g_StdStream;
extern CStdOutStream * g_ErrStream;

static const char * const kPropIdToName[] = {
	"0", "1", "2", "Path", "Name", "Extension", "Folder", "Size", "Packed Size", "Attributes", "Created", "Accessed", "Modified", 
	"Solid", "Commented", "Encrypted", "Split Before", "Split After", "Dictionary Size", "CRC", "Type", "Anti", "Method", "Host OS", 
	"File System", "User", "Group", "Block", "Comment", "Position", "Path Prefix", "Folders", "Files", "Version", "Volume", 
	"Multivolume", "Offset", "Links", "Blocks", "Volumes", "Time Type", "64-bit", "Big-endian", "CPU", "Physical Size", "Headers Size", 
	"Checksum", "Characteristics", "Virtual Address", "ID", "Short Name", "Creator Application", "Sector Size", "Mode", "Symbolic Link", 
	"Error", "Total Size", "Free Space", "Cluster Size", "Label", "Local Name", "Provider", "NT Security", "Alternate Stream", "Aux", 
	"Deleted", "Tree", "SHA-1", "SHA-256", "Error Type", "Errors", "Errors", "Warnings", "Warning", "Streams", "Alternate Streams", 
	"Alternate Streams Size", "Virtual Size", "Unpack Size", "Total Physical Size", "Volume Index", "SubType", "Short Comment", 
	"Code Page", "Is not archive type", "Physical Size can't be detected", "Zeros Tail Is Allowed", "Tail Size", "Embedded Stub Size", 
	"Link", "Hard Link", "iNode", "Stream ID", "Read-only", "Out Name", "Copy Link"
};

static const char kEmptyAttribChar = '.';
static const char * const kListing = "Listing archive: ";
static const char * const kString_Files = "files";
static const char * const kString_Dirs = "folders";
static const char * const kString_AltStreams = "alternate streams";
static const char * const kString_Streams = "streams";
static const char * const kError = "ERROR: ";

static void GetAttribString(uint32 wa, bool isDir, bool allAttribs, char * s)
{
	if(isDir)
		wa |= FILE_ATTRIBUTE_DIRECTORY;
	if(allAttribs)
		ConvertWinAttribToString(s, wa);
	else {
		s[0] = ((wa & FILE_ATTRIBUTE_DIRECTORY) != 0) ? 'D' : kEmptyAttribChar;
		s[1] = ((wa & FILE_ATTRIBUTE_READONLY)  != 0) ? 'R' : kEmptyAttribChar;
		s[2] = ((wa & FILE_ATTRIBUTE_HIDDEN)    != 0) ? 'H' : kEmptyAttribChar;
		s[3] = ((wa & FILE_ATTRIBUTE_SYSTEM)    != 0) ? 'S' : kEmptyAttribChar;
		s[4] = ((wa & FILE_ATTRIBUTE_ARCHIVE)   != 0) ? 'A' : kEmptyAttribChar;
		s[5] = 0;
	}
}

enum EAdjustment {
	kLeft,
	kCenter,
	kRight
};

struct CFieldInfoInit {
	PROPID PropID;
	const  char * Name;
	EAdjustment TitleAdjustment;
	EAdjustment TextAdjustment;
	uint   PrefixSpacesWidth;
	uint   Width;
};

static const CFieldInfoInit kStandardFieldTable[] = {
	{ kpidMTime, "   Date      Time", kLeft, kLeft, 0, 19 },
	{ kpidAttrib, "Attr", kRight, kCenter, 1, 5 },
	{ kpidSize, "Size", kRight, kRight, 1, 12 },
	{ kpidPackSize, "Compressed", kRight, kRight, 1, 12 },
	{ kpidPath, "Name", kLeft, kLeft, 2, 24 }
};

const uint kNumSpacesMax = 32; // it must be larger than max CFieldInfoInit.Width
static const char * g_Spaces = "                                ";

static void FASTCALL PrintSpaces(unsigned numSpaces)
{
	if(numSpaces > 0 && numSpaces <= kNumSpacesMax)
		g_StdOut << g_Spaces + (kNumSpacesMax - numSpaces);
}

static void FASTCALL PrintSpacesToString(char * dest, unsigned numSpaces)
{
	uint i;
	for(i = 0; i < numSpaces; i++)
		dest[i] = ' ';
	dest[i] = 0;
}

// extern int g_CodePage;

static void PrintUString(EAdjustment adj, unsigned width, const UString &s, AString &temp)
{
	/*
	   // we don't need multibyte align.
	   int codePage = g_CodePage;
	   if(codePage == -1)
	   codePage = CP_OEMCP;
	   if(codePage == CP_UTF8)
	   ConvertUnicodeToUTF8(s, temp);
	   else
	   UnicodeStringToMultiByte2(temp, s, (UINT)codePage);
	 */
	unsigned numSpaces = 0;
	if(width > s.Len()) {
		numSpaces = width - s.Len();
		unsigned numLeftSpaces = 0;
		switch(adj) {
			case kLeft:   numLeftSpaces = 0; break;
			case kCenter: numLeftSpaces = numSpaces / 2; break;
			case kRight:  numLeftSpaces = numSpaces; break;
		}
		PrintSpaces(numLeftSpaces);
		numSpaces -= numLeftSpaces;
	}
	g_StdOut.PrintUString(s, temp);
	PrintSpaces(numSpaces);
}

static void PrintString(EAdjustment adj, unsigned width, const char * s)
{
	uint   numSpaces = 0;
	uint   len = (uint)sstrlen(s);
	if(width > len) {
		numSpaces = width - len;
		unsigned numLeftSpaces = 0;
		switch(adj) {
			case kLeft:   numLeftSpaces = 0; break;
			case kCenter: numLeftSpaces = numSpaces / 2; break;
			case kRight:  numLeftSpaces = numSpaces; break;
		}
		PrintSpaces(numLeftSpaces);
		numSpaces -= numLeftSpaces;
	}
	g_StdOut << s;
	PrintSpaces(numSpaces);
}

static void PrintStringToString(char * dest, EAdjustment adj, unsigned width, const char * textString)
{
	uint   numSpaces = 0;
	uint   len = (uint)sstrlen(textString);
	if(width > len) {
		numSpaces = width - len;
		unsigned numLeftSpaces = 0;
		switch(adj) {
			case kLeft:   numLeftSpaces = 0; break;
			case kCenter: numLeftSpaces = numSpaces / 2; break;
			case kRight:  numLeftSpaces = numSpaces; break;
		}
		PrintSpacesToString(dest, numLeftSpaces);
		dest += numLeftSpaces;
		numSpaces -= numLeftSpaces;
	}
	memcpy(dest, textString, len);
	dest += len;
	PrintSpacesToString(dest, numSpaces);
}

struct CListUInt64Def {
	CListUInt64Def() : Val(0), Def(false) 
	{
	}
	void Add(uint64 v) 
	{
		Val += v; 
		Def = true;
	}
	void Add(const CListUInt64Def &v) 
	{
		if(v.Def) 
			Add(v.Val);
	}
	uint64 Val;
	bool Def;
};

struct CListFileTimeDef {
	CListFileTimeDef() : Def(false) 
	{
		Val.dwLowDateTime = 0; Val.dwHighDateTime = 0;
	}
	void Update(const CListFileTimeDef &t)
	{
		if(t.Def && (!Def || CompareFileTime(&Val, &t.Val) < 0)) {
			Val = t.Val;
			Def = true;
		}
	}
	FILETIME Val;
	bool Def;
};

struct CListStat {
	CListStat() : NumFiles(0) 
	{
	}
	void Update(const CListStat &st)
	{
		Size.Add(st.Size);
		PackSize.Add(st.PackSize);
		MTime.Update(st.MTime);
		NumFiles += st.NumFiles;
	}
	void SetSizeDefIfNoFiles() 
	{
		if(NumFiles == 0) 
			Size.Def = true;
	}
	CListUInt64Def Size;
	CListUInt64Def PackSize;
	CListFileTimeDef MTime;
	uint64 NumFiles;
};

struct CListStat2 {
	CListStat2() : NumDirs(0) 
	{
	}
	void Update(const CListStat2 &st)
	{
		MainFiles.Update(st.MainFiles);
		AltStreams.Update(st.AltStreams);
		NumDirs += st.NumDirs;
	}
	const uint64 GetNumStreams() const { return MainFiles.NumFiles + AltStreams.NumFiles; }
	CListStat &GetStat(bool altStreamsMode) { return altStreamsMode ? AltStreams : MainFiles; }

	CListStat MainFiles;
	CListStat AltStreams;
	uint64 NumDirs;
};

class CFieldPrinter {
	struct CFieldInfo {
		PROPID PropID;
		bool   IsRawProp;
		UString NameU;
		AString NameA;
		EAdjustment TitleAdjustment;
		EAdjustment TextAdjustment;
		uint   PrefixSpacesWidth;
		uint   Width;
	};
	CObjectVector <CFieldInfo> _fields;
	void AddProp(const wchar_t * name, PROPID propID, bool isRawProp);
public:
	const CArc * Arc;
	bool TechMode;
	UString FilePath;
	AString TempAString;
	UString TempWString;
	bool IsDir;
	AString LinesString;

	void Clear() { _fields.Clear(); LinesString.Empty(); }
	void Init(const CFieldInfoInit * standardFieldTable, uint numItems);
	HRESULT AddMainProps(IInArchive * archive);
	HRESULT AddRawProps(IArchiveGetRawProps * getRawProps);
	void PrintTitle();
	void PrintTitleLines();
	HRESULT PrintItemInfo(uint32 index, const CListStat &st);
	void PrintSum(const CListStat &st, uint64 numDirs, const char * str);
	void PrintSum(const CListStat2 &stat2);
};

void CFieldPrinter::Init(const CFieldInfoInit * standardFieldTable, uint numItems)
{
	Clear();
	for(uint i = 0; i < numItems; i++) {
		CFieldInfo &f = _fields.AddNew();
		const CFieldInfoInit &fii = standardFieldTable[i];
		f.PropID = fii.PropID;
		f.IsRawProp = false;
		f.NameA = fii.Name;
		f.TitleAdjustment = fii.TitleAdjustment;
		f.TextAdjustment = fii.TextAdjustment;
		f.PrefixSpacesWidth = fii.PrefixSpacesWidth;
		f.Width = fii.Width;

		unsigned k;
		for(k = 0; k < fii.PrefixSpacesWidth; k++)
			LinesString.Add_Space();
		for(k = 0; k < fii.Width; k++)
			LinesString += '-';
	}
}

static void GetPropName(PROPID propID, const wchar_t * name, AString &nameA, UString &nameU)
{
	if(propID < SIZEOFARRAY(kPropIdToName)) {
		nameA = kPropIdToName[propID];
		return;
	}
	if(name)
		nameU = name;
	else {
		nameA.Empty();
		nameA.Add_UInt32(propID);
	}
}

void CFieldPrinter::AddProp(const wchar_t * name, PROPID propID, bool isRawProp)
{
	CFieldInfo f;
	f.PropID = propID;
	f.IsRawProp = isRawProp;
	GetPropName(propID, name, f.NameA, f.NameU);
	f.NameU += " = ";
	if(!f.NameA.IsEmpty())
		f.NameA += " = ";
	else {
		const UString &s = f.NameU;
		AString sA;
		uint i;
		for(i = 0; i < s.Len(); i++) {
			wchar_t c = s[i];
			if(c >= 0x80)
				break;
			sA += (char)c;
		}
		if(i == s.Len())
			f.NameA = sA;
	}
	_fields.Add(f);
}

HRESULT CFieldPrinter::AddMainProps(IInArchive * archive)
{
	uint32 numProps;
	RINOK(archive->GetNumberOfProperties(&numProps));
	for(uint32 i = 0; i < numProps; i++) {
		CMyComBSTR name;
		PROPID propID;
		VARTYPE vt;
		RINOK(archive->GetPropertyInfo(i, &name, &propID, &vt));
		AddProp(name, propID, false);
	}
	return S_OK;
}

HRESULT CFieldPrinter::AddRawProps(IArchiveGetRawProps * getRawProps)
{
	uint32 numProps;
	RINOK(getRawProps->GetNumRawProps(&numProps));
	for(uint32 i = 0; i < numProps; i++) {
		CMyComBSTR name;
		PROPID propID;
		RINOK(getRawProps->GetRawPropInfo(i, &name, &propID));
		AddProp(name, propID, true);
	}
	return S_OK;
}

void CFieldPrinter::PrintTitle()
{
	FOR_VECTOR(i, _fields) {
		const CFieldInfo &f = _fields[i];
		PrintSpaces(f.PrefixSpacesWidth);
		PrintString(f.TitleAdjustment, ((f.PropID == kpidPath) ? 0 : f.Width), f.NameA);
	}
}

void CFieldPrinter::PrintTitleLines()
{
	g_StdOut << LinesString;
}

static void PrintTime(char * dest, const FILETIME * ft)
{
	*dest = 0;
	if(ft->dwLowDateTime == 0 && ft->dwHighDateTime == 0)
		return;
	ConvertUtcFileTimeToString(*ft, dest, kTimestampPrintLevel_SEC);
}

#ifndef _SFX

static char FASTCALL GetHex(Byte value) { return (char)((value < 10) ? ('0' + value) : ('A' + (value - 10))); }

static void HexToString(char * dest, const Byte * data, uint32 size)
{
	for(uint32 i = 0; i < size; i++) {
		Byte b = data[i];
		dest[0] = GetHex((Byte)((b >> 4) & 0xF));
		dest[1] = GetHex((Byte)(b & 0xF));
		dest += 2;
	}
	*dest = 0;
}

#endif

#define MY_ENDL endl

HRESULT CFieldPrinter::PrintItemInfo(uint32 index, const CListStat &st)
{
	char temp[128];
	size_t tempPos = 0;

	bool techMode = this->TechMode;
	/*
	   if(techMode)
	   {
	   g_StdOut << "Index = ";
	   g_StdOut << (uint64)index;
	   g_StdOut << endl;
	   }
	 */
	FOR_VECTOR(i, _fields) {
		const CFieldInfo &f = _fields[i];
		if(!techMode) {
			PrintSpacesToString(temp + tempPos, f.PrefixSpacesWidth);
			tempPos += f.PrefixSpacesWidth;
		}
		if(techMode) {
			if(!f.NameA.IsEmpty())
				g_StdOut << f.NameA;
			else
				g_StdOut << f.NameU;
		}
		if(f.PropID == kpidPath) {
			if(!techMode)
				g_StdOut << temp;
			g_StdOut.PrintUString(FilePath, TempAString);
			if(techMode)
				g_StdOut << MY_ENDL;
			continue;
		}
		const uint width = f.Width;
		if(f.IsRawProp) {
      #ifndef _SFX
			const void * data;
			uint32 dataSize;
			uint32 propType;
			RINOK(Arc->GetRawProps->GetRawProp(index, f.PropID, &data, &dataSize, &propType));
			if(dataSize != 0) {
				bool needPrint = true;
				if(f.PropID == kpidNtSecure) {
					if(propType != NPropDataType::kRaw)
						return E_FAIL;
	  #ifndef _SFX
					ConvertNtSecureToString((const Byte *)data, dataSize, TempAString);
					g_StdOut << TempAString;
					needPrint = false;
	  #endif
				}
				else if(f.PropID == kpidNtReparse) {
					UString s;
					if(ConvertNtReparseToString((const Byte *)data, dataSize, s)) {
						needPrint = false;
						g_StdOut.PrintUString(s, TempAString);
					}
				}
				if(needPrint) {
					if(propType != NPropDataType::kRaw)
						return E_FAIL;
					const uint32 kMaxDataSize = 64;
					if(dataSize > kMaxDataSize) {
						g_StdOut << "data:";
						g_StdOut << dataSize;
					}
					else {
						char hexStr[kMaxDataSize * 2 + 4];
						HexToString(hexStr, (const Byte *)data, dataSize);
						g_StdOut << hexStr;
					}
				}
			}

      #endif
		}
		else {
			CPropVariant prop;
			switch(f.PropID) {
				case kpidSize: if(st.Size.Def) prop = st.Size.Val; break;
				case kpidPackSize: if(st.PackSize.Def) prop = st.PackSize.Val; break;
				case kpidMTime: if(st.MTime.Def) prop = st.MTime.Val; break;
				default:
				    RINOK(Arc->Archive->GetProperty(index, f.PropID, &prop));
			}
			if(f.PropID == kpidAttrib && (prop.vt == VT_EMPTY || prop.vt == VT_UI4)) {
				GetAttribString((prop.vt == VT_EMPTY) ? 0 : prop.ulVal, IsDir, techMode, temp + tempPos);
				if(techMode)
					g_StdOut << temp + tempPos;
				else
					tempPos += sstrlen(temp + tempPos);
			}
			else if(prop.vt == VT_EMPTY) {
				if(!techMode) {
					PrintSpacesToString(temp + tempPos, width);
					tempPos += width;
				}
			}
			else if(prop.vt == VT_FILETIME) {
				PrintTime(temp + tempPos, &prop.filetime);
				if(techMode)
					g_StdOut << temp + tempPos;
				else {
					size_t len = sstrlen(temp + tempPos);
					tempPos += len;
					if(len < (uint)f.Width) {
						len = f.Width - len;
						PrintSpacesToString(temp + tempPos, (uint)len);
						tempPos += len;
					}
				}
			}
			else if(prop.vt == VT_BSTR) {
				TempWString.SetFromBstr(prop.bstrVal);
				if(techMode) {
					// replace CR/LF here.
					g_StdOut.PrintUString(TempWString, TempAString);
				}
				else
					PrintUString(f.TextAdjustment, width, TempWString, TempAString);
			}
			else {
				char s[64];
				ConvertPropertyToShortString2(s, prop, f.PropID);
				if(techMode)
					g_StdOut << s;
				else {
					PrintStringToString(temp + tempPos, f.TextAdjustment, width, s);
					tempPos += sstrlen(temp + tempPos);
				}
			}
		}
		if(techMode)
			g_StdOut << MY_ENDL;
	}
	g_StdOut << MY_ENDL;
	return S_OK;
}

static void PrintNumber(EAdjustment adj, unsigned width, const CListUInt64Def &value)
{
	char s[32];
	s[0] = 0;
	if(value.Def)
		ConvertUInt64ToString(value.Val, s);
	PrintString(adj, width, s);
}

void Print_UInt64_and_String(AString &s, uint64 val, const char * name);

void CFieldPrinter::PrintSum(const CListStat &st, uint64 numDirs, const char * str)
{
	FOR_VECTOR(i, _fields) {
		const CFieldInfo &f = _fields[i];
		PrintSpaces(f.PrefixSpacesWidth);
		if(f.PropID == kpidSize)
			PrintNumber(f.TextAdjustment, f.Width, st.Size);
		else if(f.PropID == kpidPackSize)
			PrintNumber(f.TextAdjustment, f.Width, st.PackSize);
		else if(f.PropID == kpidMTime) {
			char s[64];
			s[0] = 0;
			if(st.MTime.Def)
				PrintTime(s, &st.MTime.Val);
			PrintString(f.TextAdjustment, f.Width, s);
		}
		else if(f.PropID == kpidPath) {
			AString s;
			Print_UInt64_and_String(s, st.NumFiles, str);
			if(numDirs != 0) {
				s += ", ";
				Print_UInt64_and_String(s, numDirs, kString_Dirs);
			}
			PrintString(f.TextAdjustment, 0, s);
		}
		else
			PrintString(f.TextAdjustment, f.Width, "");
	}
	g_StdOut << endl;
}

void CFieldPrinter::PrintSum(const CListStat2 &stat2)
{
	PrintSum(stat2.MainFiles, stat2.NumDirs, kString_Files);
	if(stat2.AltStreams.NumFiles != 0) {
		PrintSum(stat2.AltStreams, 0, kString_AltStreams);;
		CListStat st = stat2.MainFiles;
		st.Update(stat2.AltStreams);
		PrintSum(st, 0, kString_Streams);
	}
}

static HRESULT GetUInt64Value(IInArchive * archive, uint32 index, PROPID propID, CListUInt64Def &value)
{
	value.Val = 0;
	value.Def = false;
	CPropVariant prop;
	RINOK(archive->GetProperty(index, propID, &prop));
	value.Def = ConvertPropVariantToUInt64(prop, value.Val);
	return S_OK;
}

static HRESULT GetItemMTime(IInArchive * archive, uint32 index, CListFileTimeDef &t)
{
	t.Val.dwLowDateTime = 0;
	t.Val.dwHighDateTime = 0;
	t.Def = false;
	CPropVariant prop;
	RINOK(archive->GetProperty(index, kpidMTime, &prop));
	if(prop.vt == VT_FILETIME) {
		t.Val = prop.filetime;
		t.Def = true;
	}
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static void PrintPropNameAndNumber(CStdOutStream &so, const char * name, uint64 val)
{
	so << name << ": " << val << endl;
}

static void PrintPropName_and_Eq(CStdOutStream &so, PROPID propID)
{
	const char * s;
	char temp[16];
	if(propID < SIZEOFARRAY(kPropIdToName))
		s = kPropIdToName[propID];
	else {
		ConvertUInt32ToString(propID, temp);
		s = temp;
	}
	so << s << " = ";
}

static void PrintPropNameAndNumber(CStdOutStream &so, PROPID propID, uint64 val)
{
	PrintPropName_and_Eq(so, propID);
	so << val << endl;
}

static void PrintPropNameAndNumber_Signed(CStdOutStream &so, PROPID propID, int64 val)
{
	PrintPropName_and_Eq(so, propID);
	so << val << endl;
}

static void PrintPropPair(CStdOutStream &so, const char * name, const wchar_t * val)
{
	so << name << " = " << val << endl;
}

static void PrintPropertyPair2(CStdOutStream &so, PROPID propID, const wchar_t * name, const CPropVariant &prop)
{
	UString s;
	ConvertPropertyToString2(s, prop, propID);
	if(!s.IsEmpty()) {
		AString nameA;
		UString nameU;
		GetPropName(propID, name, nameA, nameU);
		if(!nameA.IsEmpty())
			PrintPropPair(so, nameA, s);
		else
			so << nameU << " = " << s << endl;
	}
}

static HRESULT PrintArcProp(CStdOutStream &so, IInArchive * archive, PROPID propID, const wchar_t * name)
{
	CPropVariant prop;
	RINOK(archive->GetArchiveProperty(propID, &prop));
	PrintPropertyPair2(so, propID, name, prop);
	return S_OK;
}

static void PrintArcTypeError(CStdOutStream &so, const UString &type, bool isWarning)
{
	so << "Open " << (isWarning ? "WARNING" : "ERROR")
	   << ": Can not open the file as ["
	   << type
	   << "] archive"
	   << endl;
}

int Find_FileName_InSortedVector(const UStringVector &fileName, const UString& name);

void PrintErrorFlags(CStdOutStream &so, const char * s, uint32 errorFlags);

static void ErrorInfo_Print(CStdOutStream &so, const CArcErrorInfo &er)
{
	PrintErrorFlags(so, "ERRORS:", er.GetErrorFlags());
	if(!er.ErrorMessage.IsEmpty())
		PrintPropPair(so, "ERROR", er.ErrorMessage);

	PrintErrorFlags(so, "WARNINGS:", er.GetWarningFlags());
	if(!er.WarningMessage.IsEmpty())
		PrintPropPair(so, "WARNING", er.WarningMessage);
}

HRESULT Print_OpenArchive_Props(CStdOutStream &so, const CCodecs * codecs, const CArchiveLink &arcLink)
{
	FOR_VECTOR(r, arcLink.Arcs)
	{
		const CArc &arc = arcLink.Arcs[r];
		const CArcErrorInfo &er = arc.ErrorInfo;

		so << "--\n";
		PrintPropPair(so, "Path", arc.Path);
		if(er.ErrorFormatIndex >= 0) {
			if(er.ErrorFormatIndex == arc.FormatIndex)
				so << "Warning: The archive is open with offset" << endl;
			else
				PrintArcTypeError(so, codecs->GetFormatNamePtr(er.ErrorFormatIndex), true);
		}
		PrintPropPair(so, "Type", codecs->GetFormatNamePtr(arc.FormatIndex));

		ErrorInfo_Print(so, er);

		int64 offset = arc.GetGlobalOffset();
		if(offset != 0)
			PrintPropNameAndNumber_Signed(so, kpidOffset, offset);
		IInArchive * archive = arc.Archive;
		RINOK(PrintArcProp(so, archive, kpidPhySize, NULL));
		if(er.TailSize != 0)
			PrintPropNameAndNumber(so, kpidTailSize, er.TailSize);
		{
			uint32 numProps;
			RINOK(archive->GetNumberOfArchiveProperties(&numProps));

			for(uint32 j = 0; j < numProps; j++) {
				CMyComBSTR name;
				PROPID propID;
				VARTYPE vt;
				RINOK(archive->GetArchivePropertyInfo(j, &name, &propID, &vt));
				RINOK(PrintArcProp(so, archive, propID, name));
			}
		}

		if(r != arcLink.Arcs.Size() - 1) {
			uint32 numProps;
			so << "----\n";
			if(archive->GetNumberOfProperties(&numProps) == S_OK) {
				uint32 mainIndex = arcLink.Arcs[r + 1].SubfileIndex;
				for(uint32 j = 0; j < numProps; j++) {
					CMyComBSTR name;
					PROPID propID;
					VARTYPE vt;
					RINOK(archive->GetPropertyInfo(j, &name, &propID, &vt));
					CPropVariant prop;
					RINOK(archive->GetProperty(mainIndex, propID, &prop));
					PrintPropertyPair2(so, propID, name, prop);
				}
			}
		}
	}
	return S_OK;
}

HRESULT Print_OpenArchive_Error(CStdOutStream &so, const CCodecs * codecs, const CArchiveLink &arcLink)
{
  #ifndef _NO_CRYPTO
	if(arcLink.PasswordWasAsked)
		so << "Can not open encrypted archive. Wrong password?";
	else
  #endif
	{
		if(arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0) {
			so << arcLink.NonOpen_ArcPath << endl;
			PrintArcTypeError(so, codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
		}
		else
			so << "Can not open the file as archive";
	}
	so << endl;
	so << endl;
	ErrorInfo_Print(so, arcLink.NonOpen_ErrorInfo);
	return S_OK;
}

bool CensorNode_CheckPath(const NWildcard::CCensorNode &node, const CReadArcItem &item);

HRESULT ListArchives(CCodecs * codecs, const CObjectVector<COpenType> &types, const CIntVector &excludedFormats,
    bool stdInMode, UStringVector &arcPaths, UStringVector &arcPathsFull, bool processAltStreams, bool showAltStreams,
    const NWildcard::CCensorNode &wildcardCensor, bool enableHeaders, bool techMode,
    #ifndef _NO_CRYPTO
    bool &passwordEnabled, UString &password,
    #endif
    #ifndef _SFX
    const CObjectVector<CProperty> * props,
    #endif
    uint64 &numErrors, uint64 &numWarnings)
{
	bool allFilesAreAllowed = wildcardCensor.AreAllAllowed();
	numErrors = 0;
	numWarnings = 0;
	CFieldPrinter fp;
	if(!techMode)
		fp.Init(kStandardFieldTable, SIZEOFARRAY(kStandardFieldTable));
	CListStat2 stat2total;
	CBoolArr skipArcs(arcPaths.Size());
	uint   arcIndex;
	for(arcIndex = 0; arcIndex < arcPaths.Size(); arcIndex++)
		skipArcs[arcIndex] = false;
	uint64 numVolumes = 0;
	uint64 numArcs = 0;
	uint64 totalArcSizes = 0;
	HRESULT lastError = 0;
	for(arcIndex = 0; arcIndex < arcPaths.Size(); arcIndex++) {
		if(!skipArcs[arcIndex]) {
			const UString & arcPath = arcPaths[arcIndex];
			uint64 arcPackSize = 0;
			if(!stdInMode) {
				NFile::NFind::CFileInfo fi;
				if(!fi.Find(us2fs(arcPath))) {
					DWORD errorCode = GetLastError();
					SETIFZ(errorCode, ERROR_FILE_NOT_FOUND);
					lastError = HRESULT_FROM_WIN32(lastError);;
					g_StdOut.Flush();
					*g_ErrStream << endl << kError << NError::MyFormatMessage(errorCode) <<
					endl << arcPath << endl << endl;
					numErrors++;
					continue;
				}
				if(fi.IsDir()) {
					g_StdOut.Flush();
					*g_ErrStream << endl << kError << arcPath << " is not a file" << endl << endl;
					numErrors++;
					continue;
				}
				arcPackSize = fi.Size;
				totalArcSizes += arcPackSize;
			}
			CArchiveLink arcLink;
			COpenCallbackConsole openCallback;
			openCallback.Init(&g_StdOut, g_ErrStream, NULL);
		#ifndef _NO_CRYPTO
			openCallback.PasswordIsDefined = passwordEnabled;
			openCallback.Password = password;
		#endif
			/*
			   CObjectVector<COptionalOpenProperties> optPropsVector;
			   COptionalOpenProperties &optProps = optPropsVector.AddNew();
			   optProps.Props = *props;
			 */
			COpenOptions options;
		#ifndef _SFX
			options.props = props;
		#endif
			options.codecs = codecs;
			options.types = &types;
			options.excludedFormats = &excludedFormats;
			options.stdInMode = stdInMode;
			options.stream = NULL;
			options.filePath = arcPath;
			if(enableHeaders) {
				g_StdOut << endl << kListing << arcPath << endl << endl;
			}
			HRESULT result = arcLink.Open_Strict(options, &openCallback);
			if(result != S_OK) {
				if(result == E_ABORT)
					return result;
				g_StdOut.Flush();
				*g_ErrStream << endl << kError << arcPath << " : ";
				if(result == S_FALSE) {
					Print_OpenArchive_Error(*g_ErrStream, codecs, arcLink);
				}
				else {
					lastError = result;
					*g_ErrStream << "opening : ";
					if(result == E_OUTOFMEMORY)
						*g_ErrStream << SlTxtOutOfMem;
					else
						*g_ErrStream << NError::MyFormatMessage(result);
				}
				*g_ErrStream << endl;
				numErrors++;
				continue;
			}
			{
				FOR_VECTOR(r, arcLink.Arcs) {
					const CArcErrorInfo & arc = arcLink.Arcs[r].ErrorInfo;
					if(!arc.WarningMessage.IsEmpty())
						numWarnings++;
					if(arc.AreThereWarnings())
						numWarnings++;
					if(arc.ErrorFormatIndex >= 0)
						numWarnings++;
					if(arc.AreThereErrors()) {
						numErrors++;
						// break;
					}
					if(!arc.ErrorMessage.IsEmpty())
						numErrors++;
				}
			}
			numArcs++;
			numVolumes++;
			if(!stdInMode) {
				numVolumes += arcLink.VolumePaths.Size();
				totalArcSizes += arcLink.VolumesSize;
				FOR_VECTOR(v, arcLink.VolumePaths) {
					int index = Find_FileName_InSortedVector(arcPathsFull, arcLink.VolumePaths[v]);
					if(index >= 0 && (uint)index > arcIndex)
						skipArcs[(uint)index] = true;
				}
			}
			if(enableHeaders) {
				RINOK(Print_OpenArchive_Props(g_StdOut, codecs, arcLink));
				g_StdOut << endl;
				if(techMode)
					g_StdOut << "----------\n";
			}
			if(enableHeaders && !techMode) {
				fp.PrintTitle();
				g_StdOut << endl;
				fp.PrintTitleLines();
				g_StdOut << endl;
			}
			const CArc &arc = arcLink.Arcs.Back();
			fp.Arc = &arc;
			fp.TechMode = techMode;
			IInArchive * archive = arc.Archive;
			if(techMode) {
				fp.Clear();
				RINOK(fp.AddMainProps(archive));
				if(arc.GetRawProps) {
					RINOK(fp.AddRawProps(arc.GetRawProps));
				}
			}
			CListStat2 stat2;
			uint32 numItems;
			RINOK(archive->GetNumberOfItems(&numItems));
			CReadArcItem item;
			UStringVector pathParts;
			for(uint32 i = 0; i < numItems; i++) {
				if(NConsoleClose::TestBreakSignal())
					return E_ABORT;
				HRESULT res = arc.GetItemPath2(i, fp.FilePath);
				if(stdInMode && res == E_INVALIDARG)
					break;
				RINOK(res);
				if(arc.Ask_Aux) {
					bool isAux;
					RINOK(Archive_IsItem_Aux(archive, i, isAux));
					if(isAux)
						continue;
				}
				bool isAltStream = false;
				if(arc.Ask_AltStream) {
					RINOK(Archive_IsItem_AltStream(archive, i, isAltStream));
					if(isAltStream && !processAltStreams)
						continue;
				}
				RINOK(Archive_IsItem_Dir(archive, i, fp.IsDir));
				if(!allFilesAreAllowed) {
					if(isAltStream) {
						RINOK(arc.GetItem(i, item));
						if(!CensorNode_CheckPath(wildcardCensor, item))
							continue;
					}
					else {
						SplitPathToParts(fp.FilePath, pathParts);;
						bool include;
						if(!wildcardCensor.CheckPathVect(pathParts, !fp.IsDir, include))
							continue;
						if(!include)
							continue;
					}
				}
				CListStat st;
				RINOK(GetUInt64Value(archive, i, kpidSize, st.Size));
				RINOK(GetUInt64Value(archive, i, kpidPackSize, st.PackSize));
				RINOK(GetItemMTime(archive, i, st.MTime));
				if(fp.IsDir)
					stat2.NumDirs++;
				else
					st.NumFiles = 1;
				stat2.GetStat(isAltStream).Update(st);
				if(isAltStream && !showAltStreams)
					continue;
				RINOK(fp.PrintItemInfo(i, st));
			}
			uint64 numStreams = stat2.GetNumStreams();
			if(!stdInMode && !stat2.MainFiles.PackSize.Def && !stat2.AltStreams.PackSize.Def) {
				if(arcLink.VolumePaths.Size() != 0)
					arcPackSize += arcLink.VolumesSize;
				stat2.MainFiles.PackSize.Add((numStreams == 0) ? 0 : arcPackSize);
			}
			stat2.MainFiles.SetSizeDefIfNoFiles();
			stat2.AltStreams.SetSizeDefIfNoFiles();
			if(enableHeaders && !techMode) {
				fp.PrintTitleLines();
				g_StdOut << endl;
				fp.PrintSum(stat2);
			}
			if(enableHeaders) {
				if(arcLink.NonOpen_ErrorInfo.ErrorFormatIndex >= 0) {
					g_StdOut << "----------\n";
					PrintPropPair(g_StdOut, "Path", arcLink.NonOpen_ArcPath);
					PrintArcTypeError(g_StdOut, codecs->Formats[arcLink.NonOpen_ErrorInfo.ErrorFormatIndex].Name, false);
				}
			}
			stat2total.Update(stat2);
			g_StdOut.Flush();
		}
	}
	if(enableHeaders && !techMode && (arcPaths.Size() > 1 || numVolumes > 1)) {
		g_StdOut << endl;
		fp.PrintTitleLines();
		g_StdOut << endl;
		fp.PrintSum(stat2total);
		g_StdOut << endl;
		PrintPropNameAndNumber(g_StdOut, "Archives", numArcs);
		PrintPropNameAndNumber(g_StdOut, "Volumes", numVolumes);
		PrintPropNameAndNumber(g_StdOut, "Total archives size", totalArcSizes);
	}
	return (numErrors == 1 && lastError != 0) ? lastError : S_OK;
}
