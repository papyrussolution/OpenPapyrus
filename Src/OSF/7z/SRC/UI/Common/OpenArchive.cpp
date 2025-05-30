// OpenArchive.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

#ifdef SHOW_DEBUG_INFO
	#define PRF(x) x
#else
	#define PRF(x)
#endif

COpenSpecFlags::COpenSpecFlags() : /* CanReturnFull(true),*/ CanReturnFrontal(false), CanReturnTail(false), CanReturnMid(false)
{
}

bool COpenSpecFlags::CanReturn_NonStart() const { return CanReturnTail || CanReturnMid; }

COpenType::COpenType() : FormatIndex(-1), Recursive(true), EachPos(false), CanReturnArc(true), CanReturnParser(false),
	/* SkipSfxStub(true), ExeAsUnknown(true),*/
	ZerosTailIsAllowed(false), MaxStartOffset_Defined(false), MaxStartOffset(0)
{
	SpecForcedType.CanReturnFrontal = true;
	SpecForcedType.CanReturnTail = true;
	SpecForcedType.CanReturnMid = true;
	SpecMainType.CanReturnFrontal = true;
	SpecUnknownExt.CanReturnTail = true; // for sfx
	SpecUnknownExt.CanReturnMid = true;
	SpecUnknownExt.CanReturnFrontal = true; // for alt streams of sfx with pad
	// ZerosTailIsAllowed = true;
}

const COpenSpecFlags & COpenType::GetSpec(bool isForced, bool isMain, bool isUnknown) const
{
	return isForced ? SpecForcedType : (isMain ? SpecMainType : (isUnknown ? SpecUnknownExt : SpecWrongExt));
}

COpenOptions::COpenOptions() : codecs(NULL), types(NULL), excludedFormats(NULL), stream(NULL), seqStream(NULL),
	callback(NULL), callbackSpec(NULL), stdInMode(false)
{
}

// increase it, if you need to support larger SFX stubs
static const uint64 kMaxCheckStartPosition = 1 << 23;
/*
   Open:
   - formatIndex >= 0 (exact Format)
       1) Open with main type. Archive handler is allowed to use archive start finder.
          Warning, if there is tail.

   - formatIndex = -1 (Parser:0) (default)
    - same as #1 but doesn't return Parser

   - formatIndex = -2 (#1)
    - file has supported extension (like a.7z)
      Open with that main type (only starting from start of file).
        - open OK:
            - if there is no tail - return OK
            - if there is tail:
              - archive is not "Self Exe" - return OK with Warning, that there is tail
              - archive is "Self Exe"
                ignore "Self Exe" stub, and tries to open tail
                  - tail can be open as archive - shows that archive and stub size property.
                  - tail can't be open as archive - shows Parser ???
        - open FAIL:
           Try to open with all other types from offset 0 only.
           If some open type is OK and physical archive size is uequal or larger
           than file size, then return that archive with warning that can not be open as [extension type].
           If extension was EXE, it will try to open as unknown_extension case
    - file has unknown extension (like a.hhh)
       It tries to open via parser code.
         - if there is full archive or tail archive and unknown block or "Self Exe"
           at front, it shows tail archive and stub size property.
         - in another cases, if there is some archive inside file, it returns parser/
         - in another cases, it retuens S_FALSE


   - formatIndex = -3 (#2)
    - same as #1, but
    - stub (EXE) + archive is open in Parser

   - formatIndex = -4 (#3)
    - returns only Parser. skip full file archive. And show other sub-archives

   - formatIndex = -5 (#4)
    - returns only Parser. skip full file archive. And show other sub-archives for each byte pos

 */
using namespace NWindows;

/*
   #ifdef _SFX
   #define OPEN_PROPS_PARAM
   #else
   #define OPEN_PROPS_PARAM  , props
   #endif
 */

/*
   CArc::~CArc()
   {
   GetRawProps.Release();
   Archive.Release();
   printf("\nCArc::~CArc()\n");
   }
 */

#ifndef _SFX

namespace NArchive {
	namespace NParser {
		struct CParseItem {
			uint64 Offset;
			uint64 Size;
			// uint64 OkSize;
			UString Name;
			UString Extension;
			FILETIME FileTime;
			UString Comment;
			UString ArcType;

			bool FileTime_Defined;
			bool UnpackSize_Defined;
			bool NumSubDirs_Defined;
			bool NumSubFiles_Defined;

			bool IsSelfExe;
			bool IsNotArcType;

			uint64 UnpackSize;
			uint64 NumSubDirs;
			uint64 NumSubFiles;
			int FormatIndex;
			bool LenIsUnknown;
			CParseItem() : LenIsUnknown(false), FileTime_Defined(false), UnpackSize_Defined(false), NumSubFiles_Defined(false),
				NumSubDirs_Defined(false), IsSelfExe(false), IsNotArcType(false)
				// OkSize(0)
			{
			}
			/*
			   bool IsEqualTo(const CParseItem &item) const
			   {
			   return Offset == item.Offset && Size == item.Size;
			   }
			 */
			void NormalizeOffset()
			{
				if((int64)Offset < 0) {
					Size += Offset;
					// OkSize += Offset;
					Offset = 0;
				}
			}
		};

		class CHandler : public IInArchive, public IInArchiveGetStream, public CMyUnknownImp {
		public:
			CObjectVector<CParseItem> _items;
			uint64 _maxEndOffset;
			CMyComPtr<IInStream> _stream;

			MY_UNKNOWN_IMP2(IInArchive, IInArchiveGetStream)
			INTERFACE_IInArchive(; )
			STDMETHOD(GetStream) (uint32 index, ISequentialInStream **stream);
			uint64 GetLastEnd() const
			{
				if(_items.IsEmpty())
					return 0;
				const CParseItem &back = _items.Back();
				return back.Offset + back.Size;
			}
			void AddUnknownItem(uint64 next);
			int FindInsertPos(const CParseItem &item) const;
			void AddItem(const CParseItem &item);
			CHandler() : _maxEndOffset(0) 
			{
			}
		};

		int CHandler::FindInsertPos(const CParseItem &item) const
		{
			unsigned left = 0, right = _items.Size();
			while(left != right) {
				unsigned mid = (left + right) / 2;
				const CParseItem & midItem = _items[mid];
				if(item.Offset < midItem.Offset)
					right = mid;
				else if(item.Offset > midItem.Offset)
					left = mid + 1;
				else if(item.Size < midItem.Size)
					right = mid;
				else if(item.Size > midItem.Size)
					left = mid + 1;
				else {
					left = mid + 1;
					// return -1;
				}
			}
			return left;
		}

		void CHandler::AddUnknownItem(uint64 next)
		{
			/*
			   uint64 prevEnd = 0;
			   if(!_items.IsEmpty())
			   {
			   const CParseItem &back = _items.Back();
			   prevEnd = back.Offset + back.Size;
			   }
			 */
			if(_maxEndOffset < next) {
				CParseItem item2;
				item2.Offset = _maxEndOffset;
				item2.Size = next - _maxEndOffset;
				_maxEndOffset = next;
				_items.Add(item2);
			}
			else if(_maxEndOffset > next && !_items.IsEmpty()) {
				CParseItem &back = _items.Back();
				if(back.LenIsUnknown) {
					back.Size = next - back.Offset;
					_maxEndOffset = next;
				}
			}
		}

		void CHandler::AddItem(const CParseItem &item)
		{
			AddUnknownItem(item.Offset);
			int pos = FindInsertPos(item);
			if(pos >= 0) {
				_items.Insert(pos, item);
				uint64 next = item.Offset + item.Size;
				if(_maxEndOffset < next)
					_maxEndOffset = next;
			}
		}

		/*
		   static const CStatProp kProps[] =
		   {
		   { NULL, kpidPath, VT_BSTR},
		   { NULL, kpidSize, VT_UI8},
		   { NULL, kpidMTime, VT_FILETIME},
		   { NULL, kpidType, VT_BSTR},
		   { NULL, kpidComment, VT_BSTR},
		   { NULL, kpidOffset, VT_UI8},
		   { NULL, kpidUnpackSize, VT_UI8},
		   //   { NULL, kpidNumSubDirs, VT_UI8},
		   };
		 */

		static const Byte kProps[] = { kpidPath, kpidSize, kpidMTime, kpidType, kpidComment, kpidOffset, kpidUnpackSize };

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps_NO

		STDMETHODIMP CHandler::Open(IInStream * stream, const uint64 *, IArchiveOpenCallback * /* openArchiveCallback */)
		{
			COM_TRY_BEGIN
			{
				Close();
				_stream = stream;
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			_items.Clear();
			_stream.Release();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = _items.Size();
			return S_OK;
		}

		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			const CParseItem &item = _items[index];
			switch(propID) {
				case kpidPath:
				{
					char sz[32];
					ConvertUInt32ToString(index + 1, sz);
					UString s(sz);
					if(!item.Name.IsEmpty()) {
						s += '.';
						s += item.Name;
					}
					if(!item.Extension.IsEmpty()) {
						s += '.';
						s += item.Extension;
					}
					prop = s; break;
				}
				case kpidSize:
				case kpidPackSize: prop = item.Size; break;
				case kpidOffset: prop = item.Offset; break;
				case kpidUnpackSize: if(item.UnpackSize_Defined) prop = item.UnpackSize; break;
				case kpidNumSubFiles: if(item.NumSubFiles_Defined) prop = item.NumSubFiles; break;
				case kpidNumSubDirs: if(item.NumSubDirs_Defined) prop = item.NumSubDirs; break;
				case kpidMTime: if(item.FileTime_Defined) prop = item.FileTime; break;
				case kpidComment: if(!item.Comment.IsEmpty()) prop = item.Comment; break;
				case kpidType: if(!item.ArcType.IsEmpty()) prop = item.ArcType; break;
			}
			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		HRESULT CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testMode, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = _items.Size();
			if(_stream && numItems == 0)
				return S_OK;
			uint64 totalSize = 0;
			uint32 i;
			for(i = 0; i < numItems; i++)
				totalSize += _items[allFilesMode ? i : indices[i]].Size;
			extractCallback->SetTotal(totalSize);
			totalSize = 0;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> inStream(streamSpec);
			streamSpec->SetStream(_stream);
			CLimitedSequentialOutStream * outStreamSpec = new CLimitedSequentialOutStream;
			CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder();
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			for(i = 0; i < numItems; i++) {
				lps->InSize = totalSize;
				lps->OutSize = totalSize;
				RINOK(lps->SetCur());
				CMyComPtr<ISequentialOutStream> realOutStream;
				int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				int32 index = allFilesMode ? i : indices[i];
				const CParseItem &item = _items[index];
				RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
				uint64 unpackSize = item.Size;
				totalSize += unpackSize;
				bool skipMode = false;
				if(!testMode && !realOutStream)
					continue;
				RINOK(extractCallback->PrepareOperation(askMode));
				outStreamSpec->SetStream(realOutStream);
				realOutStream.Release();
				outStreamSpec->Init(skipMode ? 0 : unpackSize, true);
				int32 opRes = NExtractArc::NOperationResult::kOK;
				RINOK(_stream->Seek(item.Offset, STREAM_SEEK_SET, NULL));
				streamSpec->Init(unpackSize);
				RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
				if(outStreamSpec->GetRem() != 0)
					opRes = NExtractArc::NOperationResult::kDataError;
				outStreamSpec->ReleaseStream();
				RINOK(extractCallback->SetOperationResult(opRes));
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetStream(uint32 index, ISequentialInStream ** stream)
		{
			COM_TRY_BEGIN
			const CParseItem &item = _items[index];
			return CreateLimitedInStream(_stream, item.Offset, item.Size, stream);
			COM_TRY_END
		}
	}
}

#endif

HRESULT FASTCALL Archive_GetItemBoolProp(IInArchive * arc, uint32 index, PROPID propID, bool &result) throw()
{
	NCOM::CPropVariant prop;
	result = false;
	RINOK(arc->GetProperty(index, propID, &prop));
	if(prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

HRESULT FASTCALL Archive_IsItem_Dir(IInArchive * arc, uint32 index, bool &result) throw()
	{ return Archive_GetItemBoolProp(arc, index, kpidIsDir, result); }
HRESULT FASTCALL Archive_IsItem_Aux(IInArchive * arc, uint32 index, bool &result) throw()
	{ return Archive_GetItemBoolProp(arc, index, kpidIsAux, result); }
HRESULT FASTCALL Archive_IsItem_AltStream(IInArchive * arc, uint32 index, bool &result) throw()
	{ return Archive_GetItemBoolProp(arc, index, kpidIsAltStream, result); }
HRESULT FASTCALL Archive_IsItem_Deleted(IInArchive * arc, uint32 index, bool &result) throw()
	{ return Archive_GetItemBoolProp(arc, index, kpidIsDeleted, result); }

static HRESULT FASTCALL Archive_GetArcBoolProp(IInArchive * arc, PROPID propid, bool &result) throw()
{
	NCOM::CPropVariant prop;
	result = false;
	RINOK(arc->GetArchiveProperty(propid, &prop));
	if(prop.vt == VT_BOOL)
		result = VARIANT_BOOLToBool(prop.boolVal);
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	return S_OK;
}

static HRESULT Archive_GetArcProp_UInt(IInArchive * arc, PROPID propid, uint64 &result, bool &defined)
{
	defined = false;
	NCOM::CPropVariant prop;
	RINOK(arc->GetArchiveProperty(propid, &prop));
	switch(prop.vt) {
		case VT_UI4: result = prop.ulVal; defined = true; break;
		case VT_I4: result = (int64)prop.lVal; defined = true; break;
		case VT_UI8: result = (uint64)prop.uhVal.QuadPart; defined = true; break;
		case VT_I8: result = (uint64)prop.hVal.QuadPart; defined = true; break;
		case VT_EMPTY: break;
		default: return E_FAIL;
	}
	return S_OK;
}

static HRESULT Archive_GetArcProp_Int(IInArchive * arc, PROPID propid, int64 &result, bool &defined)
{
	defined = false;
	NCOM::CPropVariant prop;
	RINOK(arc->GetArchiveProperty(propid, &prop));
	switch(prop.vt) {
		case VT_UI4: result = prop.ulVal; defined = true; break;
		case VT_I4: result = prop.lVal; defined = true; break;
		case VT_UI8: result = (int64)prop.uhVal.QuadPart; defined = true; break;
		case VT_I8: result = (int64)prop.hVal.QuadPart; defined = true; break;
		case VT_EMPTY: break;
		default: return E_FAIL;
	}
	return S_OK;
}

CArc::CArc() : MTimeDefined(false), IsTree(false), IsReadOnly(false), Ask_Deleted(false), Ask_AltStream(false),
	Ask_Aux(false), Ask_INode(false), IgnoreSplit(false)
{
}

HRESULT CArc::Close()
{
	InStream.Release();
	return Archive->Close();
}

HRESULT CArc::IsItemAnti(uint32 index, bool &result) const
{
	return Archive_GetItemBoolProp(Archive, index, kpidIsAnti, result);
}

#ifndef _SFX

HRESULT CArc::GetItemPathToParent(uint32 index, uint32 parent, UStringVector &parts) const
{
	if(!GetRawProps)
		return E_FAIL;
	if(index == parent)
		return S_OK;
	uint32 curIndex = index;
	UString s;
	bool prevWasAltStream = false;
	for(;;) {
    #if defined(SL_LITTLEENDIAN)
		const void * p;
		uint32 size;
		uint32 propType;
		RINOK(GetRawProps->GetRawProp(curIndex, kpidName, &p, &size, &propType));
		if(p && propType == PROP_DATA_TYPE_wchar_t_PTR_Z_LE)
			s = (const wchar_t *)p;
		else
    #endif
		{
			NCOM::CPropVariant prop;
			RINOK(Archive->GetProperty(curIndex, kpidName, &prop));
			if(prop.vt == VT_BSTR && prop.bstrVal)
				s.SetFromBstr(prop.bstrVal);
			else if(prop.vt == VT_EMPTY)
				s.Empty();
			else
				return E_FAIL;
		}
		uint32 curParent = (uint32)(int32)-1;
		uint32 parentType = 0;
		RINOK(GetRawProps->GetParent(curIndex, &curParent, &parentType));
		if(parentType != NParentType::kAltStream) {
			for(;;) {
				int pos = s.ReverseFind_PathSepar();
				if(pos < 0) {
					break;
				}
				parts.Insert(0, s.Ptr(pos + 1));
				s.DeleteFrom(pos);
			}
		}
		parts.Insert(0, s);
		if(prevWasAltStream) {
			{
				UString &s2 = parts[parts.Size() - 2];
				s2 += ':';
				s2 += parts.Back();
			}
			parts.DeleteBack();
		}
		if(parent == curParent)
			return S_OK;
		prevWasAltStream = false;
		if(parentType == NParentType::kAltStream)
			prevWasAltStream = true;
		if(curParent == (uint32)(int32)-1)
			return E_FAIL;
		curIndex = curParent;
	}
}

#endif

HRESULT CArc::GetItemPath(uint32 index, UString &result) const
{
  #if defined(SL_LITTLEENDIAN)
	if(GetRawProps) {
		const void * p;
		uint32 size;
		uint32 propType;
		if(!IsTree) {
			if(GetRawProps->GetRawProp(index, kpidPath, &p, &size, &propType) == S_OK &&
			    propType == NPropDataType::kUtf16z) {
				uint len = size / 2 - 1;
				wchar_t * s = result.GetBuf(len);
				for(uint i = 0; i < len; i++) {
					wchar_t c = GetUi16(p);
					p = (const void *)((const Byte *)p + 2);
	  #if WCHAR_PATH_SEPARATOR != L'/'
					if(c == L'/')
						c = WCHAR_PATH_SEPARATOR;
	  #endif
					*s++ = c;
				}
				*s = 0;
				result.ReleaseBuf_SetLen(len);
				if(len != 0)
					return S_OK;
			}
		}
		/*
		   else if(GetRawProps->GetRawProp(index, kpidName, &p, &size, &propType) == S_OK &&
		    p && propType == NPropDataType::kUtf16z)
		   {
		   size -= 2;
		   uint32 totalSize = size;
		   bool isOK = false;
		   {
		    uint32 index2 = index;
		    for(;;)
		    {
		      uint32 parent = (uint32)(int32)-1;
		      uint32 parentType = 0;
		      if(GetRawProps->GetParent(index2, &parent, &parentType) != S_OK)
		        break;
		      if(parent == (uint32)(int32)-1)
		      {
		        if(parentType != 0)
		          totalSize += 2;
		        isOK = true;
		        break;
		      }
		      index2 = parent;
		      uint32 size2;
		      const void *p2;
		      if(GetRawProps->GetRawProp(index2, kpidName, &p2, &size2, &propType) != S_OK &&
		          p2 && propType == NPropDataType::kUtf16z)
		        break;
		      totalSize += size2;
		    }
		   }

		   if(isOK)
		   {
		    wchar_t *sz = result.GetBuf_SetEnd(totalSize / 2);
		    uint32 pos = totalSize - size;
		    memcpy((Byte *)sz + pos, p, size);
		    uint32 index2 = index;
		    for(;;)
		    {
		      uint32 parent = (uint32)(int32)-1;
		      uint32 parentType = 0;
		      if(GetRawProps->GetParent(index2, &parent, &parentType) != S_OK)
		        break;
		      if(parent == (uint32)(int32)-1)
		      {
		        if(parentType != 0)
		          sz[pos / 2 - 1] = L':';
		        break;
		      }
		      index2 = parent;
		      uint32 size2;
		      const void *p2;
		      if(GetRawProps->GetRawProp(index2, kpidName, &p2, &size2, &propType) != S_OK)
		        break;
		      pos -= size2;
		      memcpy((Byte *)sz + pos, p2, size2);
		      sz[(pos + size2 - 2) / 2] = (parentType == 0) ? WCHAR_PATH_SEPARATOR : L':';
		    }
		   #ifdef _WIN32
		    // result.Replace(L'/', WCHAR_PATH_SEPARATOR);
		   #endif
		    return S_OK;
		   }
		   }
		 */
	}
  #endif
	{
		NCOM::CPropVariant prop;
		RINOK(Archive->GetProperty(index, kpidPath, &prop));
		if(prop.vt == VT_BSTR && prop.bstrVal)
			result.SetFromBstr(prop.bstrVal);
		else if(prop.vt == VT_EMPTY)
			result.Empty();
		else
			return E_FAIL;
	}
	return result.IsEmpty() ? GetDefaultItemPath(index, result) : S_OK;
}

HRESULT CArc::GetDefaultItemPath(uint32 index, UString &result) const
{
	result.Empty();
	bool isDir;
	RINOK(Archive_IsItem_Dir(Archive, index, isDir));
	if(!isDir) {
		result = DefaultName;
		NCOM::CPropVariant prop;
		RINOK(Archive->GetProperty(index, kpidExtension, &prop));
		if(prop.vt == VT_BSTR) {
			result += '.';
			result += prop.bstrVal;
		}
		else if(prop.vt != VT_EMPTY)
			return E_FAIL;
	}
	return S_OK;
}

HRESULT CArc::GetItemPath2(uint32 index, UString &result) const
{
	RINOK(GetItemPath(index, result));
	if(Ask_Deleted) {
		bool isDeleted = false;
		RINOK(Archive_IsItem_Deleted(Archive, index, isDeleted));
		if(isDeleted)
			result.Insert(0, L"[DELETED]" WSTRING_PATH_SEPARATOR);
	}
	return S_OK;
}

#ifdef SUPPORT_ALT_STREAMS

int FindAltStreamColon_in_Path(const wchar_t * path)
{
	int colonPos = -1;
	for(uint i = 0; ; i++) {
		wchar_t c = path[i];
		if(c == 0)
			return colonPos;
		if(c == ':') {
			if(colonPos < 0)
				colonPos = i;
			continue;
		}
		if(c == WCHAR_PATH_SEPARATOR)
			colonPos = -1;
	}
}

#endif

HRESULT CArc::GetItem(uint32 index, CReadArcItem &item) const
{
  #ifdef SUPPORT_ALT_STREAMS
	item.IsAltStream = false;
	item.AltStreamName.Empty();
	item.MainPath.Empty();
  #endif

	item.IsDir = false;
	item.Path.Empty();
	item.ParentIndex = (uint32)(int32)-1;

	item.PathParts.Clear();

	RINOK(Archive_IsItem_Dir(Archive, index, item.IsDir));
	item.MainIsDir = item.IsDir;

	RINOK(GetItemPath2(index, item.Path));

  #ifndef _SFX
	uint32 mainIndex = index;
  #endif

  #ifdef SUPPORT_ALT_STREAMS

	item.MainPath = item.Path;
	if(Ask_AltStream) {
		RINOK(Archive_IsItem_AltStream(Archive, index, item.IsAltStream));
	}
	bool needFindAltStream = false;
	if(item.IsAltStream) {
		needFindAltStream = true;
		if(GetRawProps) {
			uint32 parentType = 0;
			uint32 parentIndex;
			RINOK(GetRawProps->GetParent(index, &parentIndex, &parentType));
			if(parentType == NParentType::kAltStream) {
				NCOM::CPropVariant prop;
				RINOK(Archive->GetProperty(index, kpidName, &prop));
				if(prop.vt == VT_BSTR && prop.bstrVal)
					item.AltStreamName.SetFromBstr(prop.bstrVal);
				else if(prop.vt != VT_EMPTY)
					return E_FAIL;
				else {
					// item.IsAltStream = false;
				}
				/*
				   if(item.AltStreamName.IsEmpty())
				   item.IsAltStream = false;
				 */

				needFindAltStream = false;
				item.ParentIndex = parentIndex;
				mainIndex = parentIndex;

				if(parentIndex == (uint32)(int32)-1) {
					item.MainPath.Empty();
					item.MainIsDir = true;
				}
				else {
					RINOK(GetItemPath2(parentIndex, item.MainPath));
					RINOK(Archive_IsItem_Dir(Archive, parentIndex, item.MainIsDir));
				}
			}
		}
	}

	if(item.WriteToAltStreamIfColon || needFindAltStream) {
		/* Good handler must support GetRawProps::GetParent for alt streams./
		   So the following code currently is not used */
		int colon = FindAltStreamColon_in_Path(item.Path);
		if(colon >= 0) {
			item.MainPath.DeleteFrom(colon);
			item.AltStreamName = item.Path.Ptr(colon + 1);
			item.MainIsDir = (colon == 0 || IsPathSepar(item.Path[(uint)colon - 1]));
			item.IsAltStream = true;
		}
	}

  #endif

  #ifndef _SFX
	if(item._use_baseParentFolder_mode) {
		RINOK(GetItemPathToParent(mainIndex, item._baseParentFolder, item.PathParts));

    #ifdef SUPPORT_ALT_STREAMS
		if((item.WriteToAltStreamIfColon || needFindAltStream) && !item.PathParts.IsEmpty()) {
			int colon;
			{
				UString &s = item.PathParts.Back();
				colon = FindAltStreamColon_in_Path(s);
				if(colon >= 0) {
					item.AltStreamName = s.Ptr(colon + 1);
					item.MainIsDir = (colon == 0 || IsPathSepar(s[(uint)colon - 1]));
					item.IsAltStream = true;
					s.DeleteFrom(colon);
				}
			}
			if(colon == 0)
				item.PathParts.DeleteBack();
		}
    #endif
	}
	else
  #endif
	  #ifdef SUPPORT_ALT_STREAMS
	    SplitPathToParts(item.MainPath, item.PathParts);
	  #else
	    SplitPathToParts(item.Path, item.PathParts);
	  #endif
	return S_OK;
}

#ifndef _SFX

static HRESULT Archive_GetItem_Size(IInArchive * archive, uint32 index, uint64 &size, bool &defined)
{
	NCOM::CPropVariant prop;
	defined = false;
	size = 0;
	RINOK(archive->GetProperty(index, kpidSize, &prop));
	switch(prop.vt) {
		case VT_UI1: size = prop.bVal; break;
		case VT_UI2: size = prop.uiVal; break;
		case VT_UI4: size = prop.ulVal; break;
		case VT_UI8: size = (uint64)prop.uhVal.QuadPart; break;
		case VT_EMPTY: return S_OK;
		default: return E_FAIL;
	}
	defined = true;
	return S_OK;
}

#endif

HRESULT CArc::GetItemSize(uint32 index, uint64 &size, bool &defined) const
{
	NCOM::CPropVariant prop;
	defined = false;
	size = 0;
	RINOK(Archive->GetProperty(index, kpidSize, &prop));
	switch(prop.vt) {
		case VT_UI1: size = prop.bVal; break;
		case VT_UI2: size = prop.uiVal; break;
		case VT_UI4: size = prop.ulVal; break;
		case VT_UI8: size = (uint64)prop.uhVal.QuadPart; break;
		case VT_EMPTY: return S_OK;
		default: return E_FAIL;
	}
	defined = true;
	return S_OK;
}

HRESULT CArc::GetItemMTime(uint32 index, FILETIME &ft, bool &defined) const
{
	NCOM::CPropVariant prop;
	defined = false;
	ft.dwHighDateTime = ft.dwLowDateTime = 0;
	RINOK(Archive->GetProperty(index, kpidMTime, &prop));
	if(prop.vt == VT_FILETIME) {
		ft = prop.filetime;
		defined = true;
	}
	else if(prop.vt != VT_EMPTY)
		return E_FAIL;
	else if(MTimeDefined) {
		ft = MTime;
		defined = true;
	}
	return S_OK;
}

#ifndef _SFX

static inline bool TestSignature(const Byte * p1, const Byte * p2, size_t size)
{
	for(size_t i = 0; i < size; i++)
		if(p1[i] != p2[i])
			return false;
	return true;
}

static void MakeCheckOrder(CCodecs * codecs, CIntVector &orderIndices, unsigned numTypes, CIntVector &orderIndices2, const Byte * data, size_t dataSize)
{
	for(uint i = 0; i < numTypes; i++) {
		const int index = orderIndices[i];
		if(index >= 0) {
			const CArcInfoEx & ai = codecs->Formats[index];
			if(ai.SignatureOffset != 0) {
				orderIndices2.Add(index);
				orderIndices[i] = -1;
			}
			else {
				const CObjectVector <CByteBuffer> &sigs = ai.Signatures;
				FOR_VECTOR(k, sigs) {
					const CByteBuffer &sig = sigs[k];
					if(sig.Size() == 0 && dataSize == 0 || sig.Size() != 0 && sig.Size() <= dataSize && TestSignature(data, sig, sig.Size())) {
						orderIndices2.Add(index);
						orderIndices[i] = -1;
						break;
					}
				}
			}
		}
	}
}

#endif

#ifdef UNDER_CE
static const uint kNumHashBytes = 1;
  #define HASH_VAL(buf) ((buf)[0])
#else
static const uint kNumHashBytes = 2;
// #define HASH_VAL(buf) ((buf)[0] | ((uint32)(buf)[1] << 8))
  #define HASH_VAL(buf) GetUi16(buf)
#endif

#ifndef _SFX

static bool IsExeExt(const UString &ext)
{
	return ext.IsEqualTo_Ascii_NoCase("exe");
}

static const char * const k_PreArcFormats[] = { "pe", "elf", "macho", "mub", "te" };

static bool IsNameFromList(const UString &s, const char * const names[], size_t num)
{
	for(uint i = 0; i < num; i++)
		if(sstreqi_ascii(s, names[i]))
			return true;
	return false;
}

static bool IsPreArcFormat(const CArcInfoEx &ai)
{
	return ai.Flags_PreArc() ? true : IsNameFromList(ai.Name, k_PreArcFormats, SIZEOFARRAY(k_PreArcFormats));
}

static const char * const k_Formats_with_simple_signuature[] = { "7z", "xz", "rar", "bzip2", "gzip", "cab", "wim", "rpm", "vhd", "xar" };

static bool IsNewStyleSignature(const CArcInfoEx &ai)
{
	// if(ai.Version >= 0x91F)
	return ai.NewInterface ? true : IsNameFromList(ai.Name, k_Formats_with_simple_signuature, SIZEOFARRAY(k_Formats_with_simple_signuature));
}

class CArchiveOpenCallback_Offset : public IArchiveOpenCallback, public IArchiveOpenVolumeCallback,
  #ifndef _NO_CRYPTO
	public ICryptoGetTextPassword,
  #endif
	public CMyUnknownImp {
public:
	CMyComPtr<IArchiveOpenCallback> Callback;
	CMyComPtr<IArchiveOpenVolumeCallback> OpenVolumeCallback;
	uint64 Files;
	uint64 Offset;
  #ifndef _NO_CRYPTO
	CMyComPtr<ICryptoGetTextPassword> GetTextPassword;
  #endif
	MY_QUERYINTERFACE_BEGIN2(IArchiveOpenCallback)
	MY_QUERYINTERFACE_ENTRY(IArchiveOpenVolumeCallback)
  #ifndef _NO_CRYPTO
	MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
  #endif
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	INTERFACE_IArchiveOpenCallback(; )
	INTERFACE_IArchiveOpenVolumeCallback(; )
  #ifndef _NO_CRYPTO
	STDMETHOD(CryptoGetTextPassword) (BSTR *password);
  #endif
};

#ifndef _NO_CRYPTO
STDMETHODIMP CArchiveOpenCallback_Offset::CryptoGetTextPassword(BSTR * password)
{
	COM_TRY_BEGIN
	return GetTextPassword ? GetTextPassword->CryptoGetTextPassword(password) : E_NOTIMPL;
	COM_TRY_END
}

#endif

STDMETHODIMP CArchiveOpenCallback_Offset::SetTotal(const uint64 *, const uint64 *)
{
	return S_OK;
}

STDMETHODIMP CArchiveOpenCallback_Offset::SetCompleted(const uint64 *, const uint64 * bytes)
{
	if(!Callback)
		return S_OK;
	uint64 value = Offset;
	if(bytes)
		value += *bytes;
	return Callback->SetCompleted(&Files, &value);
}

STDMETHODIMP CArchiveOpenCallback_Offset::GetProperty(PROPID propID, PROPVARIANT * value)
{
	if(OpenVolumeCallback)
		return OpenVolumeCallback->GetProperty(propID, value);
	NCOM::PropVariant_Clear(value);
	return S_OK;
	// return E_NOTIMPL;
}

STDMETHODIMP CArchiveOpenCallback_Offset::GetStream(const wchar_t * name, IInStream ** inStream)
{
	return OpenVolumeCallback ? OpenVolumeCallback->GetStream(name, inStream) : S_FALSE;
}

#endif

uint32 GetOpenArcErrorFlags(const NCOM::CPropVariant &prop, bool * isDefinedProp)
{
	ASSIGN_PTR(isDefinedProp, false);
	switch(prop.vt) {
		case VT_UI8: 
			ASSIGN_PTR(isDefinedProp, true); 
			return (uint32)prop.uhVal.QuadPart;
		case VT_UI4: 
			ASSIGN_PTR(isDefinedProp, true); 
			return prop.ulVal;
		case VT_EMPTY: return 0;
		default: throw 151199;
	}
}

CArcErrorInfo::CArcErrorInfo() : ThereIsTail(false), UnexpecedEnd(false), IgnoreTail(false), /* NonZerosTail(false),*/
	ErrorFlags_Defined(false), ErrorFlags(0), WarningFlags(0), ErrorFormatIndex(-1), TailSize(0)
{
}
// call IsArc_After_NonOpen only if Open returns S_FALSE

bool CArcErrorInfo::IsArc_After_NonOpen() const
{
	return (ErrorFlags_Defined && (ErrorFlags & kpv_ErrorFlags_IsNotArc) == 0);
}

void CArcErrorInfo::ClearErrors_Full()
{
	ErrorFormatIndex = -1;
	ClearErrors();
}

bool CArcErrorInfo::IsThereErrorOrWarning() const
{
	return ErrorFlags != 0 || WarningFlags != 0 || NeedTailWarning() || UnexpecedEnd || !ErrorMessage.IsEmpty() || !WarningMessage.IsEmpty();
}

bool CArcErrorInfo::AreThereErrors() const { return ErrorFlags != 0 || UnexpecedEnd; }
bool CArcErrorInfo::AreThereWarnings() const { return WarningFlags != 0 || NeedTailWarning(); }
bool CArcErrorInfo::NeedTailWarning() const { return !IgnoreTail && ThereIsTail; }

uint32 CArcErrorInfo::GetWarningFlags() const
{
	uint32 a = WarningFlags;
	if(NeedTailWarning() && (ErrorFlags & kpv_ErrorFlags_DataAfterEnd) == 0)
		a |= kpv_ErrorFlags_DataAfterEnd;
	return a;
}

uint32 CArcErrorInfo::GetErrorFlags() const
{
	uint32 a = ErrorFlags;
	if(UnexpecedEnd)
		a |= kpv_ErrorFlags_UnexpectedEnd;
	return a;
}

void CArcErrorInfo::ClearErrors()
{
	// ErrorFormatIndex = -1; // we don't need to clear ErrorFormatIndex here !!!

	ThereIsTail = false;
	UnexpecedEnd = false;
	IgnoreTail = false;
	// NonZerosTail = false;
	ErrorFlags_Defined = false;
	ErrorFlags = 0;
	WarningFlags = 0;
	TailSize = 0;

	ErrorMessage.Empty();
	WarningMessage.Empty();
}

HRESULT CArc::ReadBasicProps(IInArchive * archive, uint64 startPos, HRESULT openRes)
{
	// OkPhySize_Defined = false;
	PhySizeDefined = false;
	PhySize = 0;
	Offset = 0;
	AvailPhySize = FileSize - startPos;

	ErrorInfo.ClearErrors();
	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidErrorFlags, &prop));
		ErrorInfo.ErrorFlags = GetOpenArcErrorFlags(prop, &ErrorInfo.ErrorFlags_Defined);
	}
	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidWarningFlags, &prop));
		ErrorInfo.WarningFlags = GetOpenArcErrorFlags(prop);
	}

	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidError, &prop));
		if(prop.vt != VT_EMPTY)
			ErrorInfo.ErrorMessage = (prop.vt == VT_BSTR ? prop.bstrVal : L"Unknown error");
	}

	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidWarning, &prop));
		if(prop.vt != VT_EMPTY)
			ErrorInfo.WarningMessage = (prop.vt == VT_BSTR ? prop.bstrVal : L"Unknown warning");
	}

	if(openRes == S_OK || ErrorInfo.IsArc_After_NonOpen()) {
		RINOK(Archive_GetArcProp_UInt(archive, kpidPhySize, PhySize, PhySizeDefined));
		/*
		   RINOK(Archive_GetArcProp_UInt(archive, kpidOkPhySize, OkPhySize, OkPhySize_Defined));
		   if(!OkPhySize_Defined)
		   {
		   OkPhySize_Defined = PhySizeDefined;
		   OkPhySize = PhySize;
		   }
		 */

		bool offsetDefined;
		RINOK(Archive_GetArcProp_Int(archive, kpidOffset, Offset, offsetDefined));

		int64 globalOffset = startPos + Offset;
		AvailPhySize = FileSize - globalOffset;
		if(PhySizeDefined) {
			uint64 endPos = globalOffset + PhySize;
			if(endPos < FileSize) {
				AvailPhySize = PhySize;
				ErrorInfo.ThereIsTail = true;
				ErrorInfo.TailSize = FileSize - endPos;
			}
			else if(endPos > FileSize)
				ErrorInfo.UnexpecedEnd = true;
		}
	}
	return S_OK;
}
/*
   static PrintNumber(const char *s, int n)
   {
   char temp[100];
   sprintf(temp, "%s %d", s, n);
   OutputDebugStringA(temp);
   }
 */

HRESULT CArc::PrepareToOpen(const COpenOptions &op, unsigned formatIndex, CMyComPtr<IInArchive> &archive)
{
	// OutputDebugStringA("a1");
	// PrintNumber("formatIndex", formatIndex);
	RINOK(op.codecs->CreateInArchive(formatIndex, archive));
	// OutputDebugStringA("a2");
	if(!archive)
		return S_OK;
  #ifdef EXTERNAL_CODECS
	if(op.codecs->NeedSetLibCodecs) {
		const CArcInfoEx &ai = op.codecs->Formats[formatIndex];
		if(ai.LibIndex >= 0 ?
		    !op.codecs->Libs[ai.LibIndex].SetCodecs :
		    !op.codecs->Libs.IsEmpty()) {
			CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
			archive.QueryInterface(IID_ISetCompressCodecsInfo, (void **)&setCompressCodecsInfo);
			if(setCompressCodecsInfo) {
				RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(op.codecs));
			}
		}
	}
  #endif

  #ifndef _SFX
	const CArcInfoEx &ai = op.codecs->Formats[formatIndex];
	// OutputDebugStringW(ai.Name);
	// OutputDebugStringA("a3");
	if(ai.Flags_PreArc()) {
		/* we notify parsers that extract executables, that they don't need
		   to open archive, if there is tail after executable (for SFX cases) */
		CMyComPtr <IArchiveAllowTail> allowTail;
		archive.QueryInterface(IID_IArchiveAllowTail, (void **)&allowTail);
		CALLPTRMEMB(allowTail, AllowTail(BoolToInt(true)));
	}
	if(op.props) {
		/*
		   FOR_VECTOR (y, op.props) {
		   const COptionalOpenProperties &optProps = (*op.props)[y];
		   if(optProps.FormatName.IsEmpty() || optProps.FormatName.CompareNoCase(ai.Name) == 0)
		   {
		    RINOK(SetProperties(archive, optProps.Props));
		    break;
		   }
		   }
		 */
		RINOK(SetProperties(archive, *op.props));
	}
  #endif
	return S_OK;
}

#ifndef _SFX

static HRESULT ReadParseItemProps(IInArchive * archive, const CArcInfoEx &ai, NArchive::NParser::CParseItem &pi)
{
	pi.Extension = ai.GetMainExt();
	pi.FileTime_Defined = false;
	pi.ArcType = ai.Name;
	RINOK(Archive_GetArcBoolProp(archive, kpidIsNotArcType, pi.IsNotArcType));
	// RINOK(Archive_GetArcBoolProp(archive, kpidIsSelfExe, pi.IsSelfExe));
	pi.IsSelfExe = ai.Flags_PreArc();
	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidMTime, &prop));
		if(prop.vt == VT_FILETIME) {
			pi.FileTime_Defined = true;
			pi.FileTime = prop.filetime;
		}
	}
	if(!pi.FileTime_Defined) {
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidCTime, &prop));
		if(prop.vt == VT_FILETIME) {
			pi.FileTime_Defined = true;
			pi.FileTime = prop.filetime;
		}
	}
	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidName, &prop));
		if(prop.vt == VT_BSTR) {
			pi.Name.SetFromBstr(prop.bstrVal);
			pi.Extension.Empty();
		}
		else {
			RINOK(archive->GetArchiveProperty(kpidExtension, &prop));
			if(prop.vt == VT_BSTR)
				pi.Extension.SetFromBstr(prop.bstrVal);
		}
	}
	{
		NCOM::CPropVariant prop;
		RINOK(archive->GetArchiveProperty(kpidShortComment, &prop));
		if(prop.vt == VT_BSTR)
			pi.Comment.SetFromBstr(prop.bstrVal);
	}
	uint32 numItems;
	RINOK(archive->GetNumberOfItems(&numItems));
	// pi.NumSubFiles = numItems;
	// RINOK(Archive_GetArcProp_UInt(archive, kpidUnpackSize, pi.UnpackSize, pi.UnpackSize_Defined));
	// if(!pi.UnpackSize_Defined)
	{
		pi.NumSubFiles = 0;
		pi.NumSubDirs = 0;
		pi.UnpackSize = 0;
		for(uint32 i = 0; i < numItems; i++) {
			uint64 size = 0;
			bool defined = false;
			Archive_GetItem_Size(archive, i, size, defined);
			if(defined) {
				pi.UnpackSize_Defined = true;
				pi.UnpackSize += size;
			}
			bool isDir = false;
			Archive_IsItem_Dir(archive, i, isDir);
			if(isDir)
				pi.NumSubDirs++;
			else
				pi.NumSubFiles++;
		}
		if(pi.NumSubDirs != 0)
			pi.NumSubDirs_Defined = true;
		pi.NumSubFiles_Defined = true;
	}
	return S_OK;
}

#endif

HRESULT CArc::CheckZerosTail(const COpenOptions &op, uint64 offset)
{
	if(!op.stream)
		return S_OK;
	RINOK(op.stream->Seek(offset, STREAM_SEEK_SET, NULL));
	const uint32 kBufSize = 1 << 11;
	Byte buf[kBufSize];
	for(;;) {
		uint32 processed = 0;
		RINOK(op.stream->Read(buf, kBufSize, &processed));
		if(processed == 0) {
			// ErrorInfo.NonZerosTail = false;
			ErrorInfo.IgnoreTail = true;
			return S_OK;
		}
		for(size_t i = 0; i < processed; i++) {
			if(buf[i] != 0) {
				// ErrorInfo.IgnoreTail = false;
				// ErrorInfo.NonZerosTail = true;
				return S_OK;
			}
		}
	}
}

#ifndef _SFX

class CExtractCallback_To_OpenCallback : public IArchiveExtractCallback, public ICompressProgressInfo, public CMyUnknownImp {
public:
	CMyComPtr<IArchiveOpenCallback> Callback;
	uint64 Files;
	uint64 Offset;

	MY_UNKNOWN_IMP2(IArchiveExtractCallback, ICompressProgressInfo)
	INTERFACE_IArchiveExtractCallback(; )
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
	void Init(IArchiveOpenCallback * callback)
	{
		Callback = callback;
		Files = 0;
		Offset = 0;
	}
};

STDMETHODIMP CExtractCallback_To_OpenCallback::SetTotal(uint64 /* size */)
{
	return S_OK;
}

STDMETHODIMP CExtractCallback_To_OpenCallback::SetCompleted(const uint64 * /* completeValue */)
{
	return S_OK;
}

STDMETHODIMP CExtractCallback_To_OpenCallback::SetRatioInfo(const uint64 * inSize, const uint64 * /* outSize */)
{
	if(Callback) {
		uint64 value = Offset;
		if(inSize)
			value += *inSize;
		return Callback->SetCompleted(&Files, &value);
	}
	return S_OK;
}

STDMETHODIMP CExtractCallback_To_OpenCallback::GetStream(uint32 /* index */, ISequentialOutStream ** outStream, int32 /*askExtractMode*/)
{
	*outStream = 0;
	return S_OK;
}

STDMETHODIMP CExtractCallback_To_OpenCallback::PrepareOperation(int32 /* askExtractMode */)
{
	return S_OK;
}

STDMETHODIMP CExtractCallback_To_OpenCallback::SetOperationResult(int32 /* operationResult */)
{
	return S_OK;
}

static HRESULT OpenArchiveSpec(IInArchive * archive, bool needPhySize, IInStream * stream, const uint64 * maxCheckStartPosition,
    IArchiveOpenCallback * openCallback, IArchiveExtractCallback * extractCallback)
{
	/*
	   if(needPhySize)
	   {
	   CMyComPtr<IArchiveOpen2> open2;
	   archive->QueryInterface(IID_IArchiveOpen2, (void **)&open2);
	   if(open2)
	    return open2->ArcOpen2(stream, kOpenFlags_RealPhySize, openCallback);
	   }
	 */
	RINOK(archive->Open(stream, maxCheckStartPosition, openCallback));
	if(needPhySize) {
		bool phySize_Defined = false;
		uint64 phySize = 0;
		RINOK(Archive_GetArcProp_UInt(archive, kpidPhySize, phySize, phySize_Defined));
		if(phySize_Defined)
			return S_OK;

		bool phySizeCantBeDetected = false;;
		RINOK(Archive_GetArcBoolProp(archive, kpidPhySizeCantBeDetected, phySizeCantBeDetected));

		if(!phySizeCantBeDetected) {
			RINOK(archive->Extract(0, (uint32)(int32)-1, BoolToInt(true), extractCallback));
		}
	}
	return S_OK;
}

static int FindFormatForArchiveType(CCodecs * codecs, CIntVector orderIndices, const char * name)
{
	FOR_VECTOR(i, orderIndices) {
		if(sstreqi_ascii(codecs->Formats[orderIndices[i]].Name, name))
			return i;
	}
	return -1;
}

#endif

HRESULT CArc::OpenStream2(const COpenOptions &op)
{
	// fprintf(stdout, "\nOpen: %S", Path); fflush(stdout);
	Archive.Release();
	GetRawProps.Release();
	GetRootProps.Release();
	ErrorInfo.ClearErrors();
	ErrorInfo.ErrorFormatIndex = -1;
	IsParseArc = false;
	ArcStreamOffset = 0;
	// OutputDebugStringA("1");
	// OutputDebugStringW(Path);
	const UString fileName = ExtractFileNameFromPath(Path);
	UString extension;
	{
		int dotPos = fileName.ReverseFind_Dot();
		if(dotPos >= 0)
			extension = fileName.Ptr(dotPos + 1);
	}
	CIntVector orderIndices;
	bool searchMarkerInHandler = false;
  #ifdef _SFX
	searchMarkerInHandler = true;
  #endif
	CBoolArr isMainFormatArr(op.codecs->Formats.Size());
	{
		FOR_VECTOR(i, op.codecs->Formats) {
			isMainFormatArr[i] = false;
		}
	}
	uint64 maxStartOffset = op.openType.MaxStartOffset_Defined ? op.openType.MaxStartOffset : kMaxCheckStartPosition;
  #ifndef _SFX
	bool isUnknownExt = false;
  #endif
	bool isForced = false;
	unsigned numMainTypes = 0;
	int formatIndex = op.openType.FormatIndex;
	if(formatIndex >= 0) {
		isForced = true;
		orderIndices.Add(formatIndex);
		numMainTypes = 1;
		isMainFormatArr[(uint)formatIndex] = true;
		searchMarkerInHandler = true;
	}
	else {
		unsigned numFinded = 0;
    #ifndef _SFX
		bool isPrearcExt = false;
    #endif
		{
      #ifndef _SFX
			bool isZip = false;
			bool isRar = false;
			const wchar_t c = extension[0];
			if(c == 'z' || c == 'Z' || c == 'r' || c == 'R') {
				bool isNumber = false;
				for(uint k = 1;; k++) {
					const wchar_t d = extension[k];
					if(d == 0)
						break;
					if(!isdec(d)) {
						isNumber = false;
						break;
					}
					isNumber = true;
				}
				if(isNumber)
					if(c == 'z' || c == 'Z')
						isZip = true;
					else
						isRar = true;
			}
      #endif
			FOR_VECTOR(i, op.codecs->Formats) {
				const CArcInfoEx &ai = op.codecs->Formats[i];
				if(IgnoreSplit || !op.openType.CanReturnArc)
					if(ai.IsSplit())
						continue;
				if(op.excludedFormats->FindInSorted(i) >= 0)
					continue;

	#ifndef _SFX
				if(IsPreArcFormat(ai))
					isPrearcExt = true;
	#endif

				if(ai.FindExtension(extension) >= 0
	    #ifndef _SFX
				   || isZip && sstreqi_ascii(ai.Name, "zip") || isRar && sstreqi_ascii(ai.Name, "rar")
	    #endif
				    ) {
					// PrintNumber("orderIndices.Insert", i);
					orderIndices.Insert(numFinded++, i);
					isMainFormatArr[i] = true;
				}
				else
					orderIndices.Add(i);
			}
		}
		if(!op.stream) {
			if(numFinded != 1)
				return E_NOTIMPL;
			orderIndices.DeleteFrom(1);
		}
		// PrintNumber("numFinded", numFinded );

		/*
		   if(op.openOnlySpecifiedByExtension)
		   {
		   if(numFinded != 0 && !IsExeExt(extension))
		    orderIndices.DeleteFrom(numFinded);
		   }
		 */
    #ifndef _SFX
		if(op.stream && orderIndices.Size() >= 2) {
			RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
			CByteBuffer byteBuffer;
			CIntVector orderIndices2;
			if(numFinded == 0 || IsExeExt(extension)) {
				// signature search was here
			}
			else if(extension.IsEqualTo("000") || extension.IsEqualTo("001")) {
				int i = FindFormatForArchiveType(op.codecs, orderIndices, "rar");
				if(i >= 0) {
					const size_t kBufSize = (1 << 10);
					byteBuffer.Alloc(kBufSize);
					size_t processedSize = kBufSize;
					RINOK(ReadStream(op.stream, byteBuffer, &processedSize));
					if(processedSize >= 16) {
						const Byte * buf = byteBuffer;
						const Byte kRarHeader[] = { 0x52, 0x61, 0x72, 0x21, 0x1a, 0x07, 0x00 };
						if(TestSignature(buf, kRarHeader, 7) && buf[9] == 0x73 && (buf[10] & 1) != 0) {
							orderIndices2.Add(orderIndices[i]);
							orderIndices[i] = -1;
							if(i >= (int)numFinded)
								numFinded++;
						}
					}
				}
			}
			else {
				const size_t kBufSize = (1 << 10);
				byteBuffer.Alloc(kBufSize);
				size_t processedSize = kBufSize;
				RINOK(ReadStream(op.stream, byteBuffer, &processedSize));
				if(processedSize == 0)
					return S_FALSE;
				/*
				   check type order:
				   1) matched extension, no signuature
				   2) matched extension, matched signuature
				   // 3) no signuature
				   // 4) matched signuature
				 */
				MakeCheckOrder(op.codecs, orderIndices, numFinded, orderIndices2, NULL, 0);
				MakeCheckOrder(op.codecs, orderIndices, numFinded, orderIndices2, byteBuffer, processedSize);
				// MakeCheckOrder(op.codecs, orderIndices, orderIndices.Size(), orderIndices2, NULL, 0);
				// MakeCheckOrder(op.codecs, orderIndices, orderIndices.Size(), orderIndices2,
				// byteBuffer, processedSize);
			}
			FOR_VECTOR(i, orderIndices) {
				int val = orderIndices[i];
				if(val != -1)
					orderIndices2.Add(val);
			}
			orderIndices = orderIndices2;
		}
		if(orderIndices.Size() >= 2) {
			int iIso = FindFormatForArchiveType(op.codecs, orderIndices, "iso");
			int iUdf = FindFormatForArchiveType(op.codecs, orderIndices, "udf");
			if(iUdf > iIso && iIso >= 0) {
				int isoIndex = orderIndices[iIso];
				int udfIndex = orderIndices[iUdf];
				orderIndices[iUdf] = isoIndex;
				orderIndices[iIso] = udfIndex;
			}
		}
		numMainTypes = numFinded;
		isUnknownExt = (numMainTypes == 0) || isPrearcExt;
    #else // _SFX
		numMainTypes = orderIndices.Size();
		// we need correct numMainTypes for mutlivolume SFX (if some volume is missing)
		if(numFinded != 0)
			numMainTypes = numFinded;
    #endif
	}
	uint64 fileSize = 0;
	if(op.stream) {
		RINOK(op.stream->Seek(0, STREAM_SEEK_END, &fileSize));
		RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
	}
	FileSize = fileSize;
  #ifndef _SFX
	CBoolArr skipFrontalFormat(op.codecs->Formats.Size());
	{
		FOR_VECTOR(i, op.codecs->Formats) {
			skipFrontalFormat[i] = false;
		}
	}
  #endif
	const COpenType &mode = op.openType;
	if(mode.CanReturnArc) {
		// ---------- OPEN main type by extenssion ----------

		unsigned numCheckTypes = orderIndices.Size();
		if(formatIndex >= 0)
			numCheckTypes = numMainTypes;

		for(uint i = 0; i < numCheckTypes; i++) {
			FormatIndex = orderIndices[i];
			bool exactOnly = false;
      #ifndef _SFX
			const CArcInfoEx &ai = op.codecs->Formats[FormatIndex];
			// OutputDebugStringW(ai.Name);
			if(i >= numMainTypes) {
				if(!ai.Flags_BackwardOpen()
				    // && !ai.Flags_PureStartOpen()
				    )
					continue;
				exactOnly = true;
			}
      #endif
			// Some handlers do not set total bytes. So we set it here
			if(op.callback)
				RINOK(op.callback->SetTotal(NULL, &fileSize));
			if(op.stream) {
				RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
			}
			CMyComPtr<IInArchive> archive;
			RINOK(PrepareToOpen(op, FormatIndex, archive));
			if(!archive)
				continue;
			HRESULT result;
			if(op.stream) {
				uint64 searchLimit = (!exactOnly && searchMarkerInHandler) ? maxStartOffset : 0;
				result = archive->Open(op.stream, &searchLimit, op.callback);
			}
			else {
				CMyComPtr<IArchiveOpenSeq> openSeq;
				archive.QueryInterface(IID_IArchiveOpenSeq, (void **)&openSeq);
				if(!openSeq)
					return E_NOTIMPL;
				result = openSeq->OpenSeq(op.seqStream);
			}
			RINOK(ReadBasicProps(archive, 0, result));
			if(result == S_FALSE) {
				bool isArc = ErrorInfo.IsArc_After_NonOpen();
	#ifndef _SFX
				// if it's archive, we allow another open attempt for parser
				if(!mode.CanReturnParser || !isArc)
					skipFrontalFormat[(uint)FormatIndex] = true;
	#endif
				if(exactOnly)
					continue;
				if(i == 0 && numMainTypes == 1) {
					// we set NonOpenErrorInfo, only if there is only one main format (defined by
					// extension).
					ErrorInfo.ErrorFormatIndex = FormatIndex;
					NonOpen_ErrorInfo = ErrorInfo;
					if(!mode.CanReturnParser && isArc) {
						// if(formatIndex < 0 && !searchMarkerInHandler)
						{
							// if bad archive was detected, we don't need additional open
							// attempts
	      #ifndef _SFX
							if(!IsPreArcFormat(ai) /* || !mode.SkipSfxStub */)
	      #endif
							return S_FALSE;
						}
					}
				}

				/*
				   #ifndef _SFX
				   if(IsExeExt(extension) || ai.Flags_PreArc())
				   {
				   // openOnlyFullArc = false;
				   // canReturnTailArc = true;
				   // limitSignatureSearch = true;
				   }
				   #endif
				 */

				continue;
			}
			RINOK(result);
      #ifndef _SFX
			bool isMainFormat = isMainFormatArr[(uint)FormatIndex];
			const COpenSpecFlags &specFlags = mode.GetSpec(isForced, isMainFormat, isUnknownExt);
			bool thereIsTail = ErrorInfo.ThereIsTail;
			if(thereIsTail && mode.ZerosTailIsAllowed) {
				RINOK(CheckZerosTail(op, Offset + PhySize));
				if(ErrorInfo.IgnoreTail)
					thereIsTail = false;
			}
			if(Offset > 0) {
				if(exactOnly || !searchMarkerInHandler || !specFlags.CanReturn_NonStart() || (mode.MaxStartOffset_Defined && (uint64)Offset > mode.MaxStartOffset))
					continue;
			}
			if(thereIsTail) {
				if(Offset > 0) {
					if(!specFlags.CanReturnMid)
						continue;
				}
				else if(!specFlags.CanReturnFrontal)
					continue;
			}
			if(Offset > 0 || thereIsTail) {
				if(formatIndex < 0) {
					if(IsPreArcFormat(ai)) {
						// openOnlyFullArc = false;
						// canReturnTailArc = true;
						/*
						   if(mode.SkipSfxStub)
						   limitSignatureSearch = true;
						 */
						// if(mode.SkipSfxStub)
						{
							// skipFrontalFormat[FormatIndex] = true;
							continue;
						}
					}
				}
			}

      #endif

			Archive = archive;
			return S_OK;
		}
	}

  #ifndef _SFX
	if(!op.stream)
		return S_FALSE;
	if(formatIndex >= 0 && !mode.CanReturnParser) {
		if(mode.MaxStartOffset_Defined) {
			if(mode.MaxStartOffset == 0)
				return S_FALSE;
		}
		else {
			const CArcInfoEx &ai = op.codecs->Formats[formatIndex];
			if(ai.FindExtension(extension) >= 0) {
				if(ai.Flags_FindSignature() && searchMarkerInHandler)
					return S_FALSE;
			}
		}
	}
	NArchive::NParser::CHandler * handlerSpec = new NArchive::NParser::CHandler;
	CMyComPtr<IInArchive> handler = handlerSpec;
	CExtractCallback_To_OpenCallback * extractCallback_To_OpenCallback_Spec = new CExtractCallback_To_OpenCallback;
	CMyComPtr<IArchiveExtractCallback> extractCallback_To_OpenCallback = extractCallback_To_OpenCallback_Spec;
	extractCallback_To_OpenCallback_Spec->Init(op.callback);
	{
		// ---------- Check all possible START archives ----------
		// this code is better for full file archives than Parser's code.

		CByteBuffer byteBuffer;
		bool endOfFile = false;
		size_t processedSize;
		{
			size_t bufSize = 1 << 20; // it must be larger than max signature offset or IsArcFunc offset ((1 << 19) + x for UDF)
			if(bufSize > fileSize) {
				bufSize = (size_t)fileSize;
				endOfFile = true;
			}
			byteBuffer.Alloc(bufSize);
			RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
			processedSize = bufSize;
			RINOK(ReadStream(op.stream, byteBuffer, &processedSize));
			if(processedSize == 0)
				return S_FALSE;
			if(processedSize < bufSize)
				endOfFile = true;
		}
		CUIntVector sortedFormats;
		uint i;
		int splitIndex = -1;
		for(i = 0; i < orderIndices.Size(); i++) {
			unsigned form = orderIndices[i];
			if(!skipFrontalFormat[form]) {
				const CArcInfoEx &ai = op.codecs->Formats[form];
				if(ai.IsSplit()) {
					splitIndex = form;
				}
				else {
					if(ai.IsArcFunc) {
						uint32 isArcRes = ai.IsArcFunc(byteBuffer, processedSize);
						if(isArcRes == k_IsArc_Res_NO)
							continue;
						if(isArcRes == k_IsArc_Res_NEED_MORE && endOfFile)
							continue;
						// if(isArcRes == k_IsArc_Res_YES_LOW_PROB) continue;
						sortedFormats.Insert(0, form);
					}
					else {
						bool isNewStyleSignature = IsNewStyleSignature(ai);
						bool needCheck = !isNewStyleSignature || ai.Signatures.IsEmpty() || ai.Flags_PureStartOpen() || ai.Flags_StartOpen() || ai.Flags_BackwardOpen();
						if(isNewStyleSignature && !ai.Signatures.IsEmpty()) {
							uint k;
							for(k = 0; k < ai.Signatures.Size(); k++) {
								const CByteBuffer &sig = ai.Signatures[k];
								uint32 signatureEnd = ai.SignatureOffset + (uint32)sig.Size();
								if(processedSize < signatureEnd) {
									if(!endOfFile)
										needCheck = true;
								}
								else if(memcmp(sig, byteBuffer + ai.SignatureOffset, sig.Size()) == 0)
									break;
							}
							if(k != ai.Signatures.Size()) {
								sortedFormats.Insert(0, form);
								continue;
							}
						}
						if(needCheck)
							sortedFormats.Add(form);
					}
				}
			}
		}
		if(splitIndex >= 0)
			sortedFormats.Insert(0, splitIndex);
		for(i = 0; i < sortedFormats.Size(); i++) {
			FormatIndex = sortedFormats[i];
			const CArcInfoEx &ai = op.codecs->Formats[FormatIndex];
			if(op.callback)
				RINOK(op.callback->SetTotal(NULL, &fileSize));
			RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
			CMyComPtr<IInArchive> archive;
			RINOK(PrepareToOpen(op, FormatIndex, archive));
			if(!archive)
				continue;
			PRF(printf("\nSorted Open %S", (const wchar_t *)ai.Name));
			HRESULT result;
			{
				uint64 searchLimit = 0;
				/*
				   if(mode.CanReturnArc)
				   result = archive->Open(op.stream, &searchLimit, op.callback);
				   else
				 */
				result = OpenArchiveSpec(archive, !mode.CanReturnArc, op.stream, &searchLimit, op.callback, extractCallback_To_OpenCallback);
			}
			if(result == S_FALSE) {
				skipFrontalFormat[(uint)FormatIndex] = true;
				// FIXME: maybe we must use LenIsUnknown.
				// printf("  OpenForSize Error");
				continue;
			}
			RINOK(result);
			RINOK(ReadBasicProps(archive, 0, result));
			if(Offset > 0) {
				continue; // good handler doesn't return such Offset > 0
				// but there are some cases like false prefixed PK00 archive, when
				// we can support it?
			}
			NArchive::NParser::CParseItem pi;
			pi.Offset = Offset;
			pi.Size = AvailPhySize;
			// bool needScan = false;
			if(!PhySizeDefined) {
				// it's for Z format
				pi.LenIsUnknown = true;
				// needScan = true;
				// phySize = arcRem;
				// nextNeedCheckStartOpen = false;
			}
			/*
			   if(OkPhySize_Defined)
			   pi.OkSize = pi.OkPhySize;
			   else
			   pi.OkSize = pi.Size;
			 */
			pi.NormalizeOffset();
			// printf("  phySize = %8d", (uint)phySize);
			if(mode.CanReturnArc) {
				bool isMainFormat = isMainFormatArr[(uint)FormatIndex];
				const COpenSpecFlags &specFlags = mode.GetSpec(isForced, isMainFormat, isUnknownExt);
				bool openCur = false;
				if(!ErrorInfo.ThereIsTail)
					openCur = true;
				else {
					if(mode.ZerosTailIsAllowed) {
						RINOK(CheckZerosTail(op, Offset + PhySize));
						if(ErrorInfo.IgnoreTail)
							openCur = true;
					}
					if(!openCur) {
						openCur = specFlags.CanReturnFrontal;
						if(formatIndex < 0) { // format is not forced
							if(IsPreArcFormat(ai)) {
								// if(mode.SkipSfxStub)
								{
									openCur = false;
								}
							}
						}
					}
				}
				if(openCur) {
					InStream = op.stream;
					Archive = archive;
					return S_OK;
				}
			}
			skipFrontalFormat[(uint)FormatIndex] = true;
			// if(!mode.CanReturnArc)
			/*
			   if(!ErrorInfo.ThereIsTail)
			    continue;
			 */
			if(pi.Offset == 0 && !pi.LenIsUnknown && pi.Size >= FileSize)
				continue;
			// printf("\nAdd offset = %d", (int)pi.Offset);
			RINOK(ReadParseItemProps(archive, ai, pi));
			handlerSpec->AddItem(pi);
		}
	}
	// ---------- PARSER ----------
	CUIntVector arc2sig; // formatIndex to signatureIndex
	CUIntVector sig2arc; // signatureIndex to formatIndex;
	{
		unsigned sum = 0;
		FOR_VECTOR(i, op.codecs->Formats) {
			arc2sig.Add(sum);
			const CObjectVector<CByteBuffer> &sigs = op.codecs->Formats[i].Signatures;
			sum += sigs.Size();
			FOR_VECTOR(k, sigs) {
				sig2arc.Add(i);
			}
		}
	}
	{
		const size_t kBeforeSize = 1 << 16;
		const size_t kAfterSize  = 1 << 20;
		const size_t kBufSize = 1 << 22; // it must be more than kBeforeSize + kAfterSize
		const uint32 kNumVals = (uint32)1 << (kNumHashBytes * 8);
		CByteArr hashBuffer(kNumVals);
		Byte * hash = hashBuffer;
		memset(hash, 0xff, kNumVals);
		Byte prevs[256];
		memset(prevs, 0xff, sizeof(prevs));
		if(sig2arc.Size() >= 0xFF)
			return S_FALSE;
		CUIntVector difficultFormats;
		CBoolArr difficultBools(256);
		{
			for(uint i = 0; i < 256; i++)
				difficultBools[i] = false;
		}
		bool thereAreHandlersForSearch = false;
		// uint32 maxSignatureEnd = 0;
		FOR_VECTOR(i, orderIndices) {
			int index = orderIndices[i];
			if(index < 0)
				continue;
			const CArcInfoEx &ai = op.codecs->Formats[index];
			bool isDifficult = false;
			// if(ai.Version < 0x91F) // we don't use parser with old DLL (before 9.31)
			if(!ai.NewInterface)
				isDifficult = true;
			else {
				if(ai.Flags_StartOpen())
					isDifficult = true;
				FOR_VECTOR(k, ai.Signatures) {
					const CByteBuffer &sig = ai.Signatures[k];
					/*
					   uint32 signatureEnd = ai.SignatureOffset + (uint32)sig.Size();
					   if(maxSignatureEnd < signatureEnd)
					   maxSignatureEnd = signatureEnd;
					 */
					if(sig.Size() < kNumHashBytes) {
						isDifficult = true;
						continue;
					}
					thereAreHandlersForSearch = true;
					uint32 v = HASH_VAL(sig);
					uint   sigIndex = arc2sig[(uint)index] + k;
					prevs[sigIndex] = hash[v];
					hash[v] = (Byte)sigIndex;
				}
			}
			if(isDifficult) {
				difficultFormats.Add(index);
				difficultBools[index] = true;
			}
		}
		if(!thereAreHandlersForSearch) {
			// openOnlyFullArc = true;
			// canReturnTailArc = true;
		}
		RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
		CLimitedCachedInStream * limitedStreamSpec = new CLimitedCachedInStream;
		CMyComPtr <IInStream> limitedStream = limitedStreamSpec;
		limitedStreamSpec->SetStream(op.stream);
		CArchiveOpenCallback_Offset * openCallback_Offset_Spec = NULL;
		CMyComPtr <IArchiveOpenCallback> openCallback_Offset;
		if(op.callback) {
			openCallback_Offset_Spec = new CArchiveOpenCallback_Offset;
			openCallback_Offset = openCallback_Offset_Spec;
			openCallback_Offset_Spec->Callback = op.callback;
			openCallback_Offset_Spec->Callback.QueryInterface(IID_IArchiveOpenVolumeCallback, &openCallback_Offset_Spec->OpenVolumeCallback);
      #ifndef _NO_CRYPTO
			openCallback_Offset_Spec->Callback.QueryInterface(IID_ICryptoGetTextPassword, &openCallback_Offset_Spec->GetTextPassword);
      #endif
		}
		if(op.callback)
			RINOK(op.callback->SetTotal(NULL, &fileSize));
		CByteBuffer &byteBuffer = limitedStreamSpec->Buffer;
		byteBuffer.Alloc(kBufSize);
		uint64 callbackPrev = 0;
		bool needCheckStartOpen = true; // = true, if we need to test all archives types for current pos.
		bool endOfFile = false;
		uint64 bufPhyPos = 0;
		size_t bytesInBuf = 0;
		// uint64 prevPos = 0;
		// ---------- Main Scan Loop ----------
		uint64 pos = 0;
		if(!mode.EachPos && handlerSpec->_items.Size() == 1) {
			NArchive::NParser::CParseItem &pi = handlerSpec->_items[0];
			if(!pi.LenIsUnknown && pi.Offset == 0)
				pos = pi.Size;
		}
		for(;;) {
			// printf("\nPos = %d", (int)pos);
			uint64 posInBuf = pos - bufPhyPos;
			// if(pos > ((uint64)1 << 35)) break;
			if(!endOfFile) {
				if(bytesInBuf < kBufSize) {
					size_t processedSize = kBufSize - bytesInBuf;
					// printf("\nRead ask = %d", (uint)processedSize);
					uint64 seekPos = bufPhyPos + bytesInBuf;
					RINOK(op.stream->Seek(bufPhyPos + bytesInBuf, STREAM_SEEK_SET, NULL));
					RINOK(ReadStream(op.stream, byteBuffer + bytesInBuf, &processedSize));
					// printf("   processed = %d", (uint)processedSize);
					if(processedSize == 0) {
						fileSize = seekPos;
						endOfFile = true;
					}
					else {
						bytesInBuf += processedSize;
						limitedStreamSpec->SetCache(processedSize, (size_t)bufPhyPos);
					}
					continue;
				}
				if(bytesInBuf < posInBuf) {
					uint64 skipSize = posInBuf - bytesInBuf;
					if(skipSize <= kBeforeSize) {
						size_t keepSize = (size_t)(kBeforeSize - skipSize);
						// printf("\nmemmove skip = %d", (int)keepSize);
						memmove(byteBuffer, byteBuffer + bytesInBuf - keepSize, keepSize);
						bytesInBuf = keepSize;
						bufPhyPos = pos - keepSize;
						continue;
					}
					// printf("\nSkip %d", (int)(skipSize - kBeforeSize));
					// RINOK(op.stream->Seek(skipSize - kBeforeSize, STREAM_SEEK_CUR, NULL));
					bytesInBuf = 0;
					bufPhyPos = pos - kBeforeSize;
					continue;
				}
				if(bytesInBuf - posInBuf < kAfterSize) {
					size_t beg = (size_t)posInBuf - kBeforeSize;
					// printf("\nmemmove for after beg = %d", (int)beg);
					memmove(byteBuffer, byteBuffer + beg, bytesInBuf - beg);
					bufPhyPos += beg;
					bytesInBuf -= beg;
					continue;
				}
			}
			if(bytesInBuf <= (size_t)posInBuf)
				break;
			bool useOffsetCallback = false;
			if(openCallback_Offset) {
				openCallback_Offset_Spec->Files = handlerSpec->_items.Size();
				openCallback_Offset_Spec->Offset = pos;
				useOffsetCallback = (!op.openType.CanReturnArc || handlerSpec->_items.Size() > 1);
				if(pos >= callbackPrev + (1 << 23)) {
					RINOK(openCallback_Offset_Spec->SetCompleted(NULL, NULL));
					callbackPrev = pos;
				}
			}
			{
				uint64 endPos = bufPhyPos + bytesInBuf;
				if(fileSize < endPos) {
					FileSize = fileSize; // why ????
					fileSize = endPos;
				}
			}
			size_t availSize = bytesInBuf - (size_t)posInBuf;
			if(availSize < kNumHashBytes)
				break;
			size_t scanSize = availSize - ((availSize >= kAfterSize) ? kAfterSize : kNumHashBytes);
			{
				/*
				   uint64 scanLimit = openOnlyFullArc ?
				    maxSignatureEnd :
				    op.openType.ScanSize + maxSignatureEnd;
				 */
				if(!mode.CanReturnParser) {
					if(pos > maxStartOffset)
						break;
					uint64 remScan = maxStartOffset - pos;
					SETMIN(scanSize, (size_t)remScan);
				}
			}
			scanSize++;
			const Byte * buf = byteBuffer + (size_t)posInBuf;
			const Byte * bufLimit = buf + scanSize;
			size_t ppp = 0;
			if(!needCheckStartOpen) {
				for(; buf < bufLimit && hash[HASH_VAL(buf)] == 0xFF; buf++) ;
				ppp = buf - (byteBuffer + (size_t)posInBuf);
				pos += ppp;
				if(buf == bufLimit)
					continue;
			}
			uint32 v = HASH_VAL(buf);
			bool   nextNeedCheckStartOpen = true;
			uint   i = hash[v];
			uint   indexOfDifficult = 0;
			// ---------- Open Loop for Current Pos ----------
			bool wasOpen = false;
			for(;;) {
				uint index;
				bool isDifficult;
				if(needCheckStartOpen && indexOfDifficult < difficultFormats.Size()) {
					index = difficultFormats[indexOfDifficult++];
					isDifficult = true;
				}
				else {
					if(i == 0xFF)
						break;
					index = sig2arc[i];
					unsigned sigIndex = i - arc2sig[index];
					i = prevs[i];
					if(needCheckStartOpen && difficultBools[index])
						continue;
					const CArcInfoEx &ai = op.codecs->Formats[index];
					if(pos < ai.SignatureOffset)
						continue;
					/*
					   if(openOnlyFullArc)
					   if(pos != ai.SignatureOffset)
					    continue;
					 */
					const CByteBuffer &sig = ai.Signatures[sigIndex];
					if(ppp + sig.Size() > availSize || !TestSignature(buf, sig, sig.Size()))
						continue;
					// printf("\nSignature OK: %10S %8x %5d", (const wchar_t *)ai.Name, (int)pos,
					// (int)(pos - prevPos));
					// prevPos = pos;
					isDifficult = false;
				}
				const CArcInfoEx &ai = op.codecs->Formats[index];
				if((isDifficult && pos == 0) || ai.SignatureOffset == pos) {
					// we don't check same archive second time */
					if(skipFrontalFormat[index])
						continue;
				}
				uint64 startArcPos = pos;
				if(!isDifficult) {
					if(pos < ai.SignatureOffset)
						continue;
					startArcPos = pos - ai.SignatureOffset;
					/*
					   // we don't need the check for Z files
					   if(startArcPos < handlerSpec->GetLastEnd())
					   continue;
					 */
				}
				if(ai.IsArcFunc && startArcPos >= bufPhyPos) {
					size_t offsetInBuf = (size_t)(startArcPos - bufPhyPos);
					if(offsetInBuf < bytesInBuf) {
						uint32 isArcRes = ai.IsArcFunc(byteBuffer + offsetInBuf, bytesInBuf - offsetInBuf);
						if(isArcRes == k_IsArc_Res_NO)
							continue;
						if(isArcRes == k_IsArc_Res_NEED_MORE && endOfFile)
							continue;
						/*
						   if(isArcRes == k_IsArc_Res_YES_LOW_PROB) {
						   // if(pos != ai.SignatureOffset)
						   continue;
						   }
						 */
					}
					// printf("\nIsArc OK: %S", (const wchar_t *)ai.Name);
				}
				/*
				   if(pos == 67109888)
				   pos = pos;
				 */
				PRF(printf("\npos = %9I64d : %S", pos, (const wchar_t *)ai.Name));
				bool isMainFormat = isMainFormatArr[index];
				const COpenSpecFlags &specFlags = mode.GetSpec(isForced, isMainFormat, isUnknownExt);
				CMyComPtr <IInArchive> archive;
				RINOK(PrepareToOpen(op, index, archive));
				if(!archive)
					return E_FAIL;
				// OutputDebugStringW(ai.Name);
				uint64 rem = fileSize - startArcPos;
				uint64 arcStreamOffset = 0;
				if(ai.Flags_UseGlobalOffset()) {
					limitedStreamSpec->InitAndSeek(0, fileSize);
					limitedStream->Seek(startArcPos, STREAM_SEEK_SET, NULL);
				}
				else {
					limitedStreamSpec->InitAndSeek(startArcPos, rem);
					arcStreamOffset = startArcPos;
				}
				uint64 maxCheckStartPosition = 0;
				if(openCallback_Offset) {
					openCallback_Offset_Spec->Files = handlerSpec->_items.Size();
					openCallback_Offset_Spec->Offset = startArcPos;
				}
				// HRESULT result = archive->Open(limitedStream, &maxCheckStartPosition,
				// openCallback_Offset);
				extractCallback_To_OpenCallback_Spec->Files = 0;
				extractCallback_To_OpenCallback_Spec->Offset = startArcPos;
				HRESULT result = OpenArchiveSpec(archive, true, limitedStream, &maxCheckStartPosition,
				    useOffsetCallback ? (IArchiveOpenCallback*)openCallback_Offset : (IArchiveOpenCallback*)op.callback,
				    extractCallback_To_OpenCallback);
				RINOK(ReadBasicProps(archive, ai.Flags_UseGlobalOffset() ? 0 : startArcPos, result));
				bool isOpen = false;
				if(result == S_FALSE) {
					if(!mode.CanReturnParser) {
						if(formatIndex < 0 && ErrorInfo.IsArc_After_NonOpen()) {
							ErrorInfo.ErrorFormatIndex = index;
							NonOpen_ErrorInfo = ErrorInfo;
							// if archive was detected, we don't need additional open attempts
							return S_FALSE;
						}
						continue;
					}
					if(!ErrorInfo.IsArc_After_NonOpen() || !PhySizeDefined || PhySize == 0)
						continue;
				}
				else {
					isOpen = true;
					RINOK(result);
					PRF(printf("  OK "));
				}
				// slfprintf_stderr("\n %8X  %S", startArcPos, Path);
				// printf("\nOpen OK: %S", ai.Name);
				NArchive::NParser::CParseItem pi;
				pi.Offset = startArcPos;
				if(ai.Flags_UseGlobalOffset())
					pi.Offset = Offset;
				else if(Offset != 0)
					return E_FAIL;
				uint64 arcRem = FileSize - pi.Offset;
				uint64 phySize = arcRem;
				bool phySizeDefined = PhySizeDefined;
				if(phySizeDefined) {
					if(pi.Offset + PhySize > FileSize) {
						// ErrorInfo.ThereIsTail = true;
						PhySize = FileSize - pi.Offset;
					}
					phySize = PhySize;
				}
				if(phySize == 0 || (uint64)phySize > ((uint64)1 << 63))
					return E_FAIL;
				/*
				   if(!ai.UseGlobalOffset) {
				   if(phySize > arcRem) {
				    ThereIsTail = true;
				    phySize = arcRem;
				   }
				   }
				 */
				bool needScan = false;
				if(isOpen && !phySizeDefined) {
					// it's for Z format
					pi.LenIsUnknown = true;
					needScan = true;
					phySize = arcRem;
					nextNeedCheckStartOpen = false;
				}
				pi.Size = phySize;
				/*
				   if(OkPhySize_Defined)
				   pi.OkSize = OkPhySize;
				 */
				pi.NormalizeOffset();
				// printf("  phySize = %8d", (uint)phySize);
				/*
				   if(needSkipFullArc)
				   if(pi.Offset == 0 && phySizeDefined && pi.Size >= fileSize)
				    continue;
				 */
				if(pi.Offset == 0 && !pi.LenIsUnknown && pi.Size >= FileSize) {
					// it's possible for dmg archives
					if(!mode.CanReturnArc)
						continue;
				}
				if(mode.EachPos)
					pos++;
				else if(needScan) {
					pos++;
					/*
					   if(!OkPhySize_Defined)
					   pos++;
					   else
					   pos = pi.Offset + pi.OkSize;
					 */
				}
				else
					pos = pi.Offset + pi.Size;
				RINOK(ReadParseItemProps(archive, ai, pi));
				if(pi.Offset < startArcPos && !mode.EachPos /* && phySizeDefined */) {
					/* It's for DMG format.
					   This code deletes all previous items that are included to current item */
					while(!handlerSpec->_items.IsEmpty()) {
						{
							const NArchive::NParser::CParseItem &back = handlerSpec->_items.Back();
							if(back.Offset < pi.Offset)
								break;
							if(back.Offset + back.Size > pi.Offset + pi.Size)
								break;
						}
						handlerSpec->_items.DeleteBack();
					}
				}
				if(isOpen && mode.CanReturnArc && phySizeDefined) {
					// if(pi.Offset + pi.Size >= fileSize)
					bool openCur = false;
					bool thereIsTail = ErrorInfo.ThereIsTail;
					if(thereIsTail && mode.ZerosTailIsAllowed) {
						RINOK(CheckZerosTail(op, arcStreamOffset + Offset + PhySize));
						if(ErrorInfo.IgnoreTail)
							thereIsTail = false;
					}
					if(pi.Offset != 0) {
						if(!pi.IsNotArcType)
							openCur = thereIsTail ? specFlags.CanReturnMid : specFlags.CanReturnTail;
					}
					else {
						openCur = thereIsTail ? specFlags.CanReturnFrontal : true;
						if(formatIndex >= -2)
							openCur = true;
					}
					if(formatIndex < 0 && pi.IsSelfExe /* && mode.SkipSfxStub */)
						openCur = false;
					// We open file as SFX, if there is front archive or first archive is "Self
					// Executable"
					if(!openCur && !pi.IsSelfExe && !thereIsTail && (!pi.IsNotArcType || pi.Offset == 0)) {
						if(handlerSpec->_items.IsEmpty()) {
							if(specFlags.CanReturnTail)
								openCur = true;
						}
						else if(handlerSpec->_items.Size() == 1) {
							if(handlerSpec->_items[0].IsSelfExe) {
								if(mode.SpecUnknownExt.CanReturnTail)
									openCur = true;
							}
						}
					}

					if(openCur) {
						InStream = op.stream;
						Archive = archive;
						FormatIndex = index;
						ArcStreamOffset = arcStreamOffset;
						return S_OK;
					}
				}
				/*
				   if(openOnlyFullArc) {
				   ErrorInfo.ClearErrors();
				   return S_FALSE;
				   }
				 */
				pi.FormatIndex = index;
				// printf("\nAdd offset = %d", (int)pi.Offset);
				handlerSpec->AddItem(pi);
				wasOpen = true;
				break;
			}
			// ---------- End of Open Loop for Current Pos ----------

			if(!wasOpen)
				pos++;
			needCheckStartOpen = (nextNeedCheckStartOpen && wasOpen);
		}
		// ---------- End of Main Scan Loop ----------
		/*
		   if(handlerSpec->_items.Size() == 1) {
		   const NArchive::NParser::CParseItem &pi = handlerSpec->_items[0];
		   if(pi.Size == fileSize && pi.Offset == 0) {
		    Archive = archive;
		    FormatIndex2 = pi.FormatIndex;
		    return S_OK;
		   }
		   }
		 */
		if(mode.CanReturnParser) {
			bool returnParser = (handlerSpec->_items.Size() == 1); // it's possible if fileSize was not correct at start of parsing
			handlerSpec->AddUnknownItem(fileSize);
			if(handlerSpec->_items.Size() == 0)
				return S_FALSE;
			if(returnParser || handlerSpec->_items.Size() != 1) {
				// return S_FALSE;
				handlerSpec->_stream = op.stream;
				Archive = handler;
				ErrorInfo.ClearErrors();
				IsParseArc = true;
				FormatIndex = -1; // It's parser
				Offset = 0;
				return S_OK;
			}
		}
	}
  #endif
	return Archive ? S_OK : S_FALSE;
}

HRESULT CArc::OpenStream(const COpenOptions &op)
{
	RINOK(OpenStream2(op));
	// PrintNumber("op.formatIndex 3", op.formatIndex);
	if(Archive) {
		GetRawProps.Release();
		GetRootProps.Release();
		Archive->QueryInterface(IID_IArchiveGetRawProps, (void **)&GetRawProps);
		Archive->QueryInterface(IID_IArchiveGetRootProps, (void **)&GetRootProps);
		RINOK(Archive_GetArcBoolProp(Archive, kpidIsTree, IsTree));
		RINOK(Archive_GetArcBoolProp(Archive, kpidIsDeleted, Ask_Deleted));
		RINOK(Archive_GetArcBoolProp(Archive, kpidIsAltStream, Ask_AltStream));
		RINOK(Archive_GetArcBoolProp(Archive, kpidIsAux, Ask_Aux));
		RINOK(Archive_GetArcBoolProp(Archive, kpidINode, Ask_INode));
		RINOK(Archive_GetArcBoolProp(Archive, kpidReadOnly, IsReadOnly));
		const UString fileName = ExtractFileNameFromPath(Path);
		UString extension;
		{
			int dotPos = fileName.ReverseFind_Dot();
			if(dotPos >= 0)
				extension = fileName.Ptr(dotPos + 1);
		}
		DefaultName.Empty();
		if(FormatIndex >= 0) {
			const CArcInfoEx &ai = op.codecs->Formats[FormatIndex];
			if(ai.Exts.Size() == 0)
				DefaultName = GetDefaultName2(fileName, UString(), UString());
			else {
				int subExtIndex = ai.FindExtension(extension);
				if(subExtIndex < 0)
					subExtIndex = 0;
				const CArcExtInfo &extInfo = ai.Exts[subExtIndex];
				DefaultName = GetDefaultName2(fileName, extInfo.Ext, extInfo.AddExt);
			}
		}
	}

	return S_OK;
}

#ifdef _SFX

#ifdef _WIN32
  #define k_ExeExt ".exe"
static const uint k_ExeExt_Len = 4;
#else
  #define k_ExeExt ""
static const uint k_ExeExt_Len = 0;
#endif

#endif

HRESULT CArc::OpenStreamOrFile(COpenOptions &op)
{
	CMyComPtr<IInStream> fileStream;
	CMyComPtr<ISequentialInStream> seqStream;
	CInFileStream * fileStreamSpec = NULL;
	if(op.stdInMode) {
		seqStream = new CStdInFileStream;
		op.seqStream = seqStream;
	}
	else if(!op.stream) {
		fileStreamSpec = new CInFileStream;
		fileStream = fileStreamSpec;
		Path = filePath;
		if(!fileStreamSpec->Open(us2fs(Path))) {
			return GetLastError();
		}
		op.stream = fileStream;
    #ifdef _SFX
		IgnoreSplit = true;
    #endif
	}

	/*
	   if(callback) {
	   uint64 fileSize;
	   RINOK(op.stream->Seek(0, STREAM_SEEK_END, &fileSize));
	   RINOK(op.callback->SetTotal(NULL, &fileSize))
	   }
	 */

	HRESULT res = OpenStream(op);
	IgnoreSplit = false;

  #ifdef _SFX
	if(res != S_FALSE || !fileStreamSpec || !op.callbackSpec || NonOpen_ErrorInfo.IsArc_After_NonOpen())
		return res;
	{
		if(filePath.Len() > k_ExeExt_Len && sstreqi_ascii(filePath.RightPtr(k_ExeExt_Len), k_ExeExt)) {
			const UString path2 = filePath.Left(filePath.Len() - k_ExeExt_Len);
			FOR_VECTOR(i, op.codecs->Formats) {
				const CArcInfoEx &ai = op.codecs->Formats[i];
				if(ai.IsSplit())
					continue;
				UString path3 = path2;
				path3 += '.';
				path3 += ai.GetMainExt(); // "7z"  for SFX.
				Path = path3;
				Path += ".001";
				bool isOk = op.callbackSpec->SetSecondFileInfo(us2fs(Path));
				if(!isOk) {
					Path = path3;
					isOk = op.callbackSpec->SetSecondFileInfo(us2fs(Path));
				}
				if(isOk) {
					if(fileStreamSpec->Open(us2fs(Path))) {
						op.stream = fileStream;
						NonOpen_ErrorInfo.ClearErrors_Full();
						if(OpenStream(op) == S_OK)
							return S_OK;
					}
				}
			}
		}
	}

  #endif

	return res;
}

CArchiveLink::CArchiveLink() : VolumesSize(0), IsOpen(false), PasswordWasAsked(false)
{
}

CArchiveLink::~CArchiveLink() 
{
	Release();
}

HRESULT CArchiveLink::Open_Strict(COpenOptions &options, IOpenCallbackUI * callbackUI)
{
	const HRESULT result = Open3(options, callbackUI);
	return (result == S_OK && NonOpen_ErrorInfo.ErrorFormatIndex >= 0) ? S_FALSE : result;
}

void CArchiveLink::KeepModeForNextOpen()
{
	for(uint i = Arcs.Size(); i != 0;) {
		i--;
		CMyComPtr<IArchiveKeepModeForNextOpen> keep;
		Arcs[i].Archive->QueryInterface(IID_IArchiveKeepModeForNextOpen, (void **)&keep);
		CALLPTRMEMB(keep, KeepModeForNextOpen());
	}
}

HRESULT CArchiveLink::Close()
{
	for(uint i = Arcs.Size(); i != 0;) {
		i--;
		RINOK(Arcs[i].Close());
	}
	IsOpen = false;
	// ErrorsText.Empty();
	return S_OK;
}

void CArchiveLink::Release()
{
	// NonOpenErrorFormatIndex = -1;
	NonOpen_ErrorInfo.ClearErrors();
	NonOpen_ArcPath.Empty();
	while(!Arcs.IsEmpty())
		Arcs.DeleteBack();
}
/*
	void CArchiveLink::Set_ErrorsText()
	{
		FOR_VECTOR(i, Arcs) {
			const CArc &arc = Arcs[i];
			if(!arc.ErrorFlagsText.IsEmpty()) {
				if(!ErrorsText.IsEmpty())
					ErrorsText.Add_LF();
				ErrorsText += GetUnicodeString(arc.ErrorFlagsText);
			}
			if(!arc.ErrorMessage.IsEmpty()) {
				if(!ErrorsText.IsEmpty())
					ErrorsText.Add_LF();
				ErrorsText += arc.ErrorMessage;
			}
			if(!arc.WarningMessage.IsEmpty()) {
				if(!ErrorsText.IsEmpty())
					ErrorsText.Add_LF();
				ErrorsText += arc.WarningMessage;
			}
		}
	}
 */
HRESULT CArchiveLink::Open(COpenOptions &op)
{
	Release();
	if(op.types->Size() >= 32)
		return E_NOTIMPL;
	HRESULT resSpec;
	for(;;) {
		resSpec = S_OK;
		op.openType = COpenType();
		if(op.types->Size() >= 1) {
			COpenType latest;
			if(Arcs.Size() < op.types->Size())
				latest = (*op.types)[op.types->Size() - Arcs.Size() - 1];
			else {
				latest = (*op.types)[0];
				if(!latest.Recursive)
					break;
			}
			op.openType = latest;
		}
		else if(Arcs.Size() >= 32)
			break;
		/*
		   op.formatIndex = -1;
		   if(op.types->Size() >= 1) {
		   int latest;
		   if(Arcs.Size() < op.types->Size())
		    latest = (*op.types)[op.types->Size() - Arcs.Size() - 1];
		   else {
		    latest = (*op.types)[0];
		    if(latest != -2 && latest != -3)
		      break;
		   }
		   if(latest >= 0)
		    op.formatIndex = latest;
		   else if(latest == -1 || latest == -2) {
		    // default
		   }
		   else if(latest == -3)
		    op.formatIndex = -2;
		   else
		    op.formatIndex = latest + 2;
		   }
		   else if(Arcs.Size() >= 32)
		   break;
		 */
		if(Arcs.IsEmpty()) {
			CArc arc;
			arc.filePath = op.filePath;
			arc.Path = op.filePath;
			arc.SubfileIndex = (uint32)(int32)-1;
			HRESULT result = arc.OpenStreamOrFile(op);
			if(result != S_OK) {
				if(result == S_FALSE) {
					NonOpen_ErrorInfo = arc.NonOpen_ErrorInfo;
					// NonOpenErrorFormatIndex = arc.ErrorFormatIndex;
					NonOpen_ArcPath = arc.Path;
				}
				return result;
			}
			Arcs.Add(arc);
			continue;
		}
		// PrintNumber("op.formatIndex 11", op.formatIndex);
		const CArc &arc = Arcs.Back();
		if(op.types->Size() > Arcs.Size())
			resSpec = E_NOTIMPL;
		uint32 mainSubfile;
		{
			NCOM::CPropVariant prop;
			RINOK(arc.Archive->GetArchiveProperty(kpidMainSubfile, &prop));
			if(prop.vt == VT_UI4)
				mainSubfile = prop.ulVal;
			else
				break;
			uint32 numItems;
			RINOK(arc.Archive->GetNumberOfItems(&numItems));
			if(mainSubfile >= numItems)
				break;
		}
		CMyComPtr<IInArchiveGetStream> getStream;
		if(arc.Archive->QueryInterface(IID_IInArchiveGetStream, (void **)&getStream) != S_OK || !getStream)
			break;
		CMyComPtr<ISequentialInStream> subSeqStream;
		if(getStream->GetStream(mainSubfile, &subSeqStream) != S_OK || !subSeqStream)
			break;
		CMyComPtr<IInStream> subStream;
		if(subSeqStream.QueryInterface(IID_IInStream, &subStream) != S_OK || !subStream)
			break;
		CArc arc2;
		RINOK(arc.GetItemPath(mainSubfile, arc2.Path));
		bool zerosTailIsAllowed;
		RINOK(Archive_GetItemBoolProp(arc.Archive, mainSubfile, kpidZerosTailIsAllowed, zerosTailIsAllowed));
		if(op.callback) {
			CMyComPtr<IArchiveOpenSetSubArchiveName> setSubArchiveName;
			op.callback->QueryInterface(IID_IArchiveOpenSetSubArchiveName, (void **)&setSubArchiveName);
			CALLPTRMEMB(setSubArchiveName, SetSubArchiveName(arc2.Path));
		}
		arc2.SubfileIndex = mainSubfile;
		// CIntVector incl;
		CIntVector excl;

		COpenOptions op2;
    #ifndef _SFX
		op2.props = op.props;
    #endif
		op2.codecs = op.codecs;
		// op2.types = &incl;
		op2.openType = op.openType;
		op2.openType.ZerosTailIsAllowed = zerosTailIsAllowed;
		op2.excludedFormats = &excl;
		op2.stdInMode = false;
		op2.stream = subStream;
		op2.filePath = arc2.Path;
		op2.callback = op.callback;
		op2.callbackSpec = op.callbackSpec;

		HRESULT result = arc2.OpenStream(op2);
		resSpec = (op.types->Size() == 0 ? S_OK : S_FALSE);
		if(result == S_FALSE) {
			NonOpen_ErrorInfo = arc2.ErrorInfo;
			NonOpen_ArcPath = arc2.Path;
			break;
		}
		RINOK(result);
		RINOK(arc.GetItemMTime(mainSubfile, arc2.MTime, arc2.MTimeDefined));
		Arcs.Add(arc2);
	}
	IsOpen = !Arcs.IsEmpty();
	return resSpec;
}

HRESULT CArchiveLink::Open2(COpenOptions &op, IOpenCallbackUI * callbackUI)
{
	VolumesSize = 0;
	COpenCallbackImp * openCallbackSpec = new COpenCallbackImp;
	CMyComPtr <IArchiveOpenCallback> callback = openCallbackSpec;
	openCallbackSpec->Callback = callbackUI;
	FString prefix, name;
	if(!op.stream && !op.stdInMode) {
		NFile::NDir::GetFullPathAndSplit(us2fs(op.filePath), prefix, name);
		openCallbackSpec->Init(prefix, name);
	}
	else {
		openCallbackSpec->SetSubArchiveName(op.filePath);
	}
	op.callback = callback;
	op.callbackSpec = openCallbackSpec;
	HRESULT res = Open(op);
	PasswordWasAsked = openCallbackSpec->PasswordWasAsked;
	// Password = openCallbackSpec->Password;
	RINOK(res);
	// VolumePaths.Add(fs2us(prefix + name));
	FOR_VECTOR(i, openCallbackSpec->FileNames_WasUsed) {
		if(openCallbackSpec->FileNames_WasUsed[i]) {
			VolumePaths.Add(fs2us(prefix) + openCallbackSpec->FileNames[i]);
			VolumesSize += openCallbackSpec->FileSizes[i];
		}
	}
	// VolumesSize = openCallbackSpec->TotalSize;
	return S_OK;
}

HRESULT CArc::ReOpen(const COpenOptions &op)
{
	ErrorInfo.ClearErrors();
	ErrorInfo.ErrorFormatIndex = -1;
	uint64 fileSize = 0;
	if(op.stream) {
		RINOK(op.stream->Seek(0, STREAM_SEEK_END, &fileSize));
		RINOK(op.stream->Seek(0, STREAM_SEEK_SET, NULL));
	}
	FileSize = fileSize;

	CMyComPtr <IInStream> stream2;
	int64 globalOffset = GetGlobalOffset();
	if(globalOffset <= 0)
		stream2 = op.stream;
	else {
		class CTailInStream : public IInStream, public CMyUnknownImp {
			uint64 _virtPos;
		public:
			CMyComPtr<IInStream> Stream;
			uint64 Offset;
			void Init()
			{
				_virtPos = 0;
			}
			MY_UNKNOWN_IMP2(ISequentialInStream, IInStream)
			STDMETHOD(Read)(void * data, uint32 size, uint32 *processedSize)
			{
				uint32 cur;
				HRESULT res = Stream->Read(data, size, &cur);
				ASSIGN_PTR(processedSize, cur);
				_virtPos += cur;
				return res;
			}
			STDMETHOD(Seek)(int64 offset, uint32 seekOrigin, uint64 *newPosition)
			{
				switch(seekOrigin) {
					case STREAM_SEEK_SET: break;
					case STREAM_SEEK_CUR: offset += _virtPos; break;
					case STREAM_SEEK_END:
					{
						uint64 pos = 0;
						RINOK(Stream->Seek(offset, STREAM_SEEK_END, &pos));
						if(pos < Offset)
							return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
						_virtPos = pos - Offset;
						ASSIGN_PTR(newPosition, _virtPos);
						return S_OK;
					}
					default: return STG_E_INVALIDFUNCTION;
				}
				if(offset < 0)
					return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
				_virtPos = offset;
				ASSIGN_PTR(newPosition, _virtPos);
				return Stream->Seek(Offset + _virtPos, STREAM_SEEK_SET, NULL);
			}
			HRESULT SeekToStart() { return Stream->Seek(Offset, STREAM_SEEK_SET, NULL); }
		};
		CTailInStream * tailStreamSpec = new CTailInStream;
		stream2 = tailStreamSpec;
		tailStreamSpec->Stream = op.stream;
		tailStreamSpec->Offset = globalOffset;
		tailStreamSpec->Init();
		RINOK(tailStreamSpec->SeekToStart());
	}
	// There are archives with embedded STUBs (like ZIP), so we must support signature scanning
	// But for another archives we can use 0 here. So the code can be fixed !!!
	uint64 maxStartPosition = kMaxCheckStartPosition;
	HRESULT res = Archive->Open(stream2, &maxStartPosition, op.callback);
	if(res == S_OK) {
		RINOK(ReadBasicProps(Archive, globalOffset, res));
		ArcStreamOffset = globalOffset;
		if(ArcStreamOffset != 0)
			InStream = op.stream;
	}
	return res;
}

HRESULT CArchiveLink::Open3(COpenOptions &op, IOpenCallbackUI * callbackUI)
{
	HRESULT res = Open2(op, callbackUI);
	if(callbackUI) {
		RINOK(callbackUI->Open_Finished());
	}
	return res;
}

HRESULT CArchiveLink::ReOpen(COpenOptions &op)
{
	if(Arcs.Size() > 1)
		return E_NOTIMPL;
	CObjectVector<COpenType> inc;
	CIntVector excl;
	op.types = &inc;
	op.excludedFormats = &excl;
	op.stdInMode = false;
	op.stream = NULL;
	if(Arcs.Size() == 0) // ???
		return Open2(op, NULL);
	COpenCallbackImp * openCallbackSpec = new COpenCallbackImp;
	CMyComPtr<IArchiveOpenCallback> openCallbackNew = openCallbackSpec;
	openCallbackSpec->Callback = NULL;
	openCallbackSpec->ReOpenCallback = op.callback;
	{
		FString dirPrefix, fileName;
		NFile::NDir::GetFullPathAndSplit(us2fs(op.filePath), dirPrefix, fileName);
		openCallbackSpec->Init(dirPrefix, fileName);
	}
	CInFileStream * fileStreamSpec = new CInFileStream;
	CMyComPtr<IInStream> stream(fileStreamSpec);
	if(!fileStreamSpec->Open(us2fs(op.filePath)))
		return GetLastError();
	op.stream = stream;
	CArc &arc = Arcs[0];
	HRESULT res = arc.ReOpen(op);
	PasswordWasAsked = openCallbackSpec->PasswordWasAsked;
	// Password = openCallbackSpec->Password;
	IsOpen = (res == S_OK);
	return res;
}

#ifndef _SFX
	bool ParseComplexSize(const wchar_t * s, uint64 &result)
	{
		result = 0;
		const wchar_t * end;
		uint64 number = ConvertStringToUInt64(s, &end);
		if(end == s)
			return false;
		else if(*end == 0) {
			result = number;
			return true;
		}
		else if(end[1] != 0)
			return false;
		else {
			uint   numBits;
			switch(MyCharLower_Ascii(*end)) {
				case 'b': result = number; return true;
				case 'k': numBits = 10; break;
				case 'm': numBits = 20; break;
				case 'g': numBits = 30; break;
				case 't': numBits = 40; break;
				default: return false;
			}
			if(number >= ((uint64)1 << (64 - numBits)))
				return false;
			else {
				result = number << numBits;
				return true;
			}
		}
	}
	static bool ParseTypeParams(const UString &s, COpenType &type)
	{
		if(s[0] == 0)
			return true;
		if(s[1] == 0) {
			switch((uint)(Byte)s[0]) {
				case 'e': type.EachPos = true; return true;
				case 'a': type.CanReturnArc = true; return true;
				case 'r': type.Recursive = true; return true;
			}
			return false;
		}
		if(s[0] == 's') {
			uint64 result;
			if(!ParseComplexSize(s.Ptr(1), result))
				return false;
			type.MaxStartOffset = result;
			type.MaxStartOffset_Defined = true;
			return true;
		}
		return false;
	}
	static bool ParseType(CCodecs & codecs, const UString & s, COpenType & type)
	{
		int pos2 = s.Find(L':');
		{
			UString name;
			if(pos2 < 0) {
				name = s;
				pos2 = s.Len();
			}
			else {
				name = s.Left(pos2);
				pos2++;
			}
			int index = codecs.FindFormatForArchiveType(name);
			type.Recursive = false;
			if(index < 0) {
				if(name[0] == '*') {
					if(name[1] != 0)
						return false;
				}
				else if(name[0] == '#') {
					if(name[1] != 0)
						return false;
					type.CanReturnArc = false;
					type.CanReturnParser = true;
				}
				else
					return false;
			}
			type.FormatIndex = index;
		}
		for(uint i = pos2; i < s.Len();) {
			int next = s.Find(L':', i);
			if(next < 0)
				next = s.Len();
			const UString name = s.Mid(i, next - i);
			if(name.IsEmpty())
				return false;
			if(!ParseTypeParams(name, type))
				return false;
			i = next + 1;
		}
		return true;
	}

	bool ParseOpenTypes(CCodecs & codecs, const UString & s, CObjectVector <COpenType> & types)
	{
		types.Clear();
		for(uint pos = 0; pos < s.Len();) {
			int pos2 = s.Find(L'.', pos);
			if(pos2 < 0)
				pos2 = s.Len();
			UString name = s.Mid(pos, pos2 - pos);
			if(name.IsEmpty())
				return false;
			COpenType type;
			if(!ParseType(codecs, name, type))
				return false;
			types.Add(type);
			pos = pos2 + 1;
		}
		return true;
	}
#endif
