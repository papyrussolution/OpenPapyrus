// Update.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>
#if defined(_WIN32) && !defined(UNDER_CE)
	#include <mapi.h>
#endif

using namespace NWindows;
using namespace NCOM;
using namespace NFile;
using namespace NDir;
using namespace NName;
using namespace NUpdateArchive;
using namespace NTime;

extern bool g_CaseSensitive;
//
// UpdateAction.cpp
namespace NUpdateArchive {
	const CActionSet k_ActionSet_Add = {
		{ NPairAction::kCopy, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress, NPairAction::kCompress }
	};
	const CActionSet k_ActionSet_Update = {
		{ NPairAction::kCopy, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress }
	};
	const CActionSet k_ActionSet_Fresh = {{NPairAction::kCopy, NPairAction::kCopy, NPairAction::kIgnore, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress }};
	const CActionSet k_ActionSet_Sync = {{NPairAction::kCopy, NPairAction::kIgnore, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress, NPairAction::kCopy, NPairAction::kCompress }};
	const CActionSet k_ActionSet_Delete = {{ NPairAction::kCopy, NPairAction::kIgnore, NPairAction::kIgnore, NPairAction::kIgnore, NPairAction::kIgnore, NPairAction::kIgnore, NPairAction::kIgnore }};

	bool FASTCALL CActionSet::IsEqualTo(const CActionSet &a) const
	{
		for(uint i = 0; i < NPairState::kNumValues; i++)
			if(StateActions[i] != a.StateActions[i])
				return false;
		return true;
	}
	bool CActionSet::NeedScanning() const
	{
		uint i;
		for(i = 0; i < NPairState::kNumValues; i++)
			if(StateActions[i] == NPairAction::kCompress)
				return true;
		for(i = 1; i < NPairState::kNumValues; i++)
			if(StateActions[i] != NPairAction::kIgnore)
				return true;
		return false;
	}
}
//
//#include <UpdatePair.h>
struct CUpdatePair {
	CUpdatePair() : ArcIndex(-1), DirIndex(-1), HostIndex(-1) 
	{
	}
	NUpdateArchive::NPairState::EEnum State;
	int ArcIndex;
	int DirIndex;
	int HostIndex; // >= 0 for alt streams only, contains index of host pair
};
//
// UpdatePair.cpp
static int MyCompareTime(NFileTimeType::EEnum fileTimeType, const FILETIME &time1, const FILETIME &time2)
{
	switch(fileTimeType) {
		case NFileTimeType::kWindows:
		    return ::CompareFileTime(&time1, &time2);
		case NFileTimeType::kUnix:
	    {
		    uint32 unixTime1, unixTime2;
		    FileTimeToUnixTime(time1, unixTime1);
		    FileTimeToUnixTime(time2, unixTime2);
		    return MyCompare(unixTime1, unixTime2);
	    }
		case NFileTimeType::kDOS:
	    {
		    uint32 dosTime1, dosTime2;
		    FileTimeToDosTime(time1, dosTime1);
		    FileTimeToDosTime(time2, dosTime2);
		    return MyCompare(dosTime1, dosTime2);
	    }
	}
	throw 4191618;
}

static const char * const k_Duplicate_inArc_Message = "Duplicate filename in archive:";
static const char * const k_Duplicate_inDir_Message = "Duplicate filename on disk:";
static const char * const k_NotCensoredCollision_Message = "Internal file name collision (file on disk, file in archive):";

static void ThrowError(const char * message, const UString &s1, const UString &s2)
{
	UString m(message);
	m.Add_LF(); m += s1;
	m.Add_LF(); m += s2;
	throw m;
}

static int FASTCALL CompareArcItemsBase(const CArcItem &ai1, const CArcItem &ai2)
{
	int res = CompareFileNames(ai1.Name, ai2.Name);
	if(res != 0)
		return res;
	else if(ai1.IsDir != ai2.IsDir)
		return ai1.IsDir ? -1 : 1;
	else
		return 0;
}

static int CompareArcItems(const uint * p1, const uint * p2, void * param)
{
	uint   i1 = *p1;
	uint   i2 = *p2;
	const  CObjectVector<CArcItem> &arcItems = *(const CObjectVector<CArcItem> *)param;
	int    res = CompareArcItemsBase(arcItems[i1], arcItems[i2]);
	return res ? res : MyCompare(i1, i2);
}

void GetUpdatePairInfoList(const CDirItems &dirItems, const CObjectVector<CArcItem> &arcItems, NFileTimeType::EEnum fileTimeType, CRecordVector<CUpdatePair> &updatePairs)
{
	CUIntVector dirIndices, arcIndices;
	uint   numDirItems = dirItems.Items.Size();
	uint   numArcItems = arcItems.Size();
	CIntArr duplicatedArcItem(numArcItems);
	{
		int * vals = &duplicatedArcItem[0];
		for(uint i = 0; i < numArcItems; i++)
			vals[i] = 0;
	}
	{
		arcIndices.ClearAndSetSize(numArcItems);
		if(numArcItems != 0) {
			uint   * vals = &arcIndices[0];
			for(uint i = 0; i < numArcItems; i++)
				vals[i] = i;
		}
		arcIndices.Sort(CompareArcItems, (void *)&arcItems);
		for(uint i = 0; i + 1 < numArcItems; i++)
			if(CompareArcItemsBase(arcItems[arcIndices[i]], arcItems[arcIndices[i + 1]]) == 0) {
				duplicatedArcItem[i] = 1;
				duplicatedArcItem[i + 1] = -1;
			}
	}
	UStringVector dirNames;
	{
		dirNames.ClearAndReserve(numDirItems);
		uint i;
		for(i = 0; i < numDirItems; i++)
			dirNames.AddInReserved(dirItems.GetLogPath(i));
		SortFileNames(dirNames, dirIndices);
		for(i = 0; i + 1 < numDirItems; i++) {
			const UString &s1 = dirNames[dirIndices[i]];
			const UString &s2 = dirNames[dirIndices[i + 1]];
			if(CompareFileNames(s1, s2) == 0)
				ThrowError(k_Duplicate_inDir_Message, s1, s2);
		}
	}
	uint   dirIndex = 0;
	uint   arcIndex = 0;
	int    prevHostFile = -1;
	const  UString * prevHostName = NULL;
	while(dirIndex < numDirItems || arcIndex < numArcItems) {
		CUpdatePair pair;
		int    dirIndex2 = -1;
		int    arcIndex2 = -1;
		const  CDirItem * di = NULL;
		const  CArcItem * ai = NULL;
		int    compareResult = -1;
		const  UString * name = NULL;
		if(dirIndex < numDirItems) {
			dirIndex2 = dirIndices[dirIndex];
			di = &dirItems.Items[dirIndex2];
		}
		if(arcIndex < numArcItems) {
			arcIndex2 = arcIndices[arcIndex];
			ai = &arcItems[arcIndex2];
			compareResult = 1;
			if(dirIndex < numDirItems) {
				compareResult = CompareFileNames(dirNames[dirIndex2], ai->Name);
				if(compareResult == 0) {
					if(di->IsDir() != ai->IsDir)
						compareResult = (ai->IsDir ? 1 : -1);
				}
			}
		}
		if(compareResult < 0) {
			name = &dirNames[dirIndex2];
			pair.State = NUpdateArchive::NPairState::kOnlyOnDisk;
			pair.DirIndex = dirIndex2;
			dirIndex++;
		}
		else if(compareResult > 0) {
			name = &ai->Name;
			pair.State = ai->Censored ? NUpdateArchive::NPairState::kOnlyInArchive : NUpdateArchive::NPairState::kNotMasked;
			pair.ArcIndex = arcIndex2;
			arcIndex++;
		}
		else {
			int dupl = duplicatedArcItem[arcIndex];
			if(dupl != 0)
				ThrowError(k_Duplicate_inArc_Message, ai->Name, arcItems[arcIndices[arcIndex + dupl]].Name);
			name = &dirNames[dirIndex2];
			if(!ai->Censored)
				ThrowError(k_NotCensoredCollision_Message, *name, ai->Name);
			pair.DirIndex = dirIndex2;
			pair.ArcIndex = arcIndex2;
			switch(ai->MTimeDefined ? MyCompareTime(ai->TimeType != -1 ? (NFileTimeType::EEnum)ai->TimeType : fileTimeType, di->MTime, ai->MTime) : 0) {
				case -1: pair.State = NUpdateArchive::NPairState::kNewInArchive; break;
				case  1: pair.State = NUpdateArchive::NPairState::kOldInArchive; break;
				default: pair.State = (ai->SizeDefined && di->Size == ai->Size) ? NUpdateArchive::NPairState::kSameFiles : NUpdateArchive::NPairState::kUnknowNewerFiles;
			}
			dirIndex++;
			arcIndex++;
		}
		if((di && di->IsAltStream) || (ai && ai->IsAltStream)) {
			if(prevHostName) {
				uint   hostLen = prevHostName->Len();
				if(name->Len() > hostLen)
					if((*name)[hostLen] == ':' && CompareFileNames(*prevHostName, name->Left(hostLen)) == 0)
						pair.HostIndex = prevHostFile;
			}
		}
		else {
			prevHostFile = updatePairs.Size();
			prevHostName = name;
		}
		updatePairs.Add(pair);
	}
	updatePairs.ReserveDown();
}
//
//#include <UpdateProduce.h>
struct CUpdatePair2 {
	CUpdatePair2();
	void   FASTCALL SetAs_NoChangeArcItem(int arcIndex);
	bool   ExistOnDisk() const;
	bool   ExistInArchive() const;

	bool   NewData;
	bool   NewProps;
	bool   UseArcProps; // if(UseArcProps && NewProps), we want to change only some properties.
	bool   IsAnti; // if(!IsAnti) we use other ways to detect Anti status
	int    DirIndex;
	int    ArcIndex;
	int    NewNameIndex;
	bool   IsMainRenameItem;
};
//
// UpdateProduce.cpp
static const char * const kUpdateActionSetCollision = "Internal collision in update action set";

CUpdatePair2::CUpdatePair2() : NewData(false), NewProps(false), UseArcProps(false), IsAnti(false), DirIndex(-1), ArcIndex(-1),
	NewNameIndex(-1), IsMainRenameItem(false)
{
}

void FASTCALL CUpdatePair2::SetAs_NoChangeArcItem(int arcIndex)
{
	NewData = NewProps = false;
	UseArcProps = true;
	IsAnti = false;
	ArcIndex = arcIndex;
}

bool CUpdatePair2::ExistOnDisk() const { return DirIndex != -1; }
bool CUpdatePair2::ExistInArchive() const { return ArcIndex != -1; }

struct IUpdateProduceCallback {
	virtual HRESULT ShowDeleteFile(unsigned arcIndex) = 0;
};

void UpdateProduce(const CRecordVector <CUpdatePair> & updatePairs, const CActionSet & actionSet, CRecordVector <CUpdatePair2> & operationChain, IUpdateProduceCallback * callback)
{
	FOR_VECTOR(i, updatePairs) {
		const CUpdatePair &pair = updatePairs[i];
		CUpdatePair2 up2;
		up2.DirIndex = pair.DirIndex;
		up2.ArcIndex = pair.ArcIndex;
		up2.NewData = up2.NewProps = true;
		up2.UseArcProps = false;
		switch(actionSet.StateActions[(uint)pair.State]) {
			case NPairAction::kIgnore:
			    if(pair.ArcIndex >= 0 && callback)
				    callback->ShowDeleteFile(pair.ArcIndex);
			    continue;
			case NPairAction::kCopy:
			    if(pair.State == NPairState::kOnlyOnDisk)
				    throw kUpdateActionSetCollision;
			    if(pair.State == NPairState::kOnlyInArchive) {
				    if(pair.HostIndex >= 0) {
					    /*
					       ignore alt stream if
					        1) no such alt stream in Disk
					        2) there is Host file in disk
					     */
					    if(updatePairs[pair.HostIndex].DirIndex >= 0)
						    continue;
				    }
			    }
			    up2.NewData = up2.NewProps = false;
			    up2.UseArcProps = true;
			    break;

			case NPairAction::kCompress:
			    if(pair.State == NPairState::kOnlyInArchive || pair.State == NPairState::kNotMasked)
				    throw kUpdateActionSetCollision;
			    break;
			case NPairAction::kCompressAsAnti:
			    up2.IsAnti = true;
			    up2.UseArcProps = (pair.ArcIndex >= 0);
			    break;
		}
		operationChain.Add(up2);
	}
	operationChain.ReserveDown();
}
//
// UpdateCallback.cpp
#ifndef _7ZIP_ST
	static NSynchronization::CCriticalSection g_CriticalSection;
	#define MT_LOCK NSynchronization::CCriticalSectionLock lock(g_CriticalSection);
#else
	#define MT_LOCK
#endif
#ifdef _USE_SECURITY_CODE
	bool InitLocalPrivileges();
#endif

class CArchiveUpdateCallback : public IArchiveUpdateCallback2, public IArchiveUpdateCallbackFile, public IArchiveExtractCallbackMessage,
	public IArchiveGetRawProps, public IArchiveGetRootProps, public ICryptoGetTextPassword2, public ICryptoGetTextPassword,
	public ICompressProgressInfo, public IInFileStream_Callback, public CMyUnknownImp
{
	struct CKeyKeyValPair {
		int    Compare(const CKeyKeyValPair & a) const;
		uint64 Key1;
		uint64 Key2;
		uint   Value;
	};
  #if defined(_WIN32) && !defined(UNDER_CE)
	bool _saclEnabled;
  #endif
	CRecordVector <CKeyKeyValPair> _map;
	uint32 _hardIndex_From;
	uint32 _hardIndex_To;
public:
	MY_QUERYINTERFACE_BEGIN2(IArchiveUpdateCallback2)
	MY_QUERYINTERFACE_ENTRY(IArchiveUpdateCallbackFile)
	MY_QUERYINTERFACE_ENTRY(IArchiveExtractCallbackMessage)
	MY_QUERYINTERFACE_ENTRY(IArchiveGetRawProps)
	MY_QUERYINTERFACE_ENTRY(IArchiveGetRootProps)
	MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword2)
	MY_QUERYINTERFACE_ENTRY(ICryptoGetTextPassword)
	MY_QUERYINTERFACE_ENTRY(ICompressProgressInfo)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE
	STDMETHOD(SetRatioInfo) (const uint64 *inSize, const uint64 *outSize);
	INTERFACE_IArchiveUpdateCallback2(; )
	INTERFACE_IArchiveUpdateCallbackFile(; )
	INTERFACE_IArchiveExtractCallbackMessage(; )
	INTERFACE_IArchiveGetRawProps(; )
	INTERFACE_IArchiveGetRootProps(; )
	STDMETHOD(CryptoGetTextPassword2) (int32 *passwordIsDefined, BSTR *password);
	STDMETHOD(CryptoGetTextPassword) (BSTR *password);

	bool AreAllFilesClosed() const { return _openFiles_Indexes.IsEmpty(); }
	virtual HRESULT InFileStream_On_Error(UINT_PTR val, DWORD error);
	virtual void InFileStream_On_Destroy(UINT_PTR val);
	CArchiveUpdateCallback();
	bool FASTCALL IsDir(const CUpdatePair2 &up) const;

	CRecordVector <uint32> _openFiles_Indexes;
	FStringVector _openFiles_Paths;
	CRecordVector<uint64> VolumesSizes;
	FString VolName;
	FString VolExt;
	IUpdateCallbackUI * Callback;
	const  CDirItems * DirItems;
	const  CDirItem * ParentDirItem;
	const  CArc * Arc;
	CMyComPtr <IInArchive> Archive;
	const  CObjectVector<CArcItem> * ArcItems;
	const  CRecordVector<CUpdatePair2> * UpdatePairs;
	const  UStringVector * NewNames;
	int    CommentIndex;
	const  UString * Comment;
	bool   ShareForWrite;
	bool   StdInMode;
	bool   KeepOriginalItemNames;
	bool   StoreNtSecurity;
	bool   StoreHardLinks;
	bool   StoreSymLinks;
	Byte * ProcessedItemsStatuses;
};

CArchiveUpdateCallback::CArchiveUpdateCallback() : _hardIndex_From((uint32)(int32)-1),
	Callback(NULL), DirItems(NULL), ParentDirItem(NULL), Arc(NULL), ArcItems(NULL),
	UpdatePairs(NULL), NewNames(NULL), CommentIndex(-1), Comment(NULL),
	ShareForWrite(false), StdInMode(false), KeepOriginalItemNames(false), StoreNtSecurity(false),
	StoreHardLinks(false), StoreSymLinks(false), ProcessedItemsStatuses(NULL)
{
  #ifdef _USE_SECURITY_CODE
	_saclEnabled = InitLocalPrivileges();
  #endif
}

bool FASTCALL CArchiveUpdateCallback::IsDir(const CUpdatePair2 &up) const
{
	if(up.DirIndex >= 0)
		return DirItems->Items[up.DirIndex].IsDir();
	else if(up.ArcIndex >= 0)
		return (*ArcItems)[up.ArcIndex].IsDir;
	else
		return false;
}

STDMETHODIMP CArchiveUpdateCallback::SetTotal(uint64 size)
{
	COM_TRY_BEGIN
	return Callback->SetTotal(size);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetCompleted(const uint64 * completeValue)
{
	COM_TRY_BEGIN
	return Callback->SetCompleted(completeValue);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetRatioInfo(const uint64 * inSize, const uint64 * outSize)
{
	COM_TRY_BEGIN
	return Callback->SetRatioInfo(inSize, outSize);
	COM_TRY_END
}
/*
   static const CStatProp kProps[] = {
	{ NULL, kpidPath, VT_BSTR}, { NULL, kpidIsDir, VT_BOOL}, { NULL, kpidSize, VT_UI8}, { NULL, kpidCTime, VT_FILETIME},
	{ NULL, kpidATime, VT_FILETIME}, { NULL, kpidMTime, VT_FILETIME}, { NULL, kpidAttrib, VT_UI4}, { NULL, kpidIsAnti, VT_BOOL}
   };
   STDMETHODIMP CArchiveUpdateCallback::EnumProperties(IEnumSTATPROPSTG **)
   {
	return CStatPropEnumerator::CreateEnumerator(kProps, ARRAY_SIZE(kProps), enumerator);
   }
 */
STDMETHODIMP CArchiveUpdateCallback::GetUpdateItemInfo(uint32 index, int32 * newData, int32 * newProps, uint32 * indexInArchive)
{
	COM_TRY_BEGIN
	RINOK(Callback->CheckBreak());
	const CUpdatePair2 &up = (*UpdatePairs)[index];
	if(newData) *newData = BoolToInt(up.NewData);
	if(newProps) *newProps = BoolToInt(up.NewProps);
	if(indexInArchive) {
		*indexInArchive = (uint32)(int32)-1;
		if(up.ExistInArchive())
			*indexInArchive = (ArcItems == 0) ? up.ArcIndex : (*ArcItems)[up.ArcIndex].IndexInServer;
	}
	return S_OK;
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetRootProp(PROPID propID, PROPVARIANT * value)
{
	NCOM::CPropVariant prop;
	switch(propID) {
		case kpidIsDir:  prop = true; break;
		case kpidAttrib: if(ParentDirItem) prop = ParentDirItem->Attrib; break;
		case kpidCTime:  if(ParentDirItem) prop = ParentDirItem->CTime; break;
		case kpidATime:  if(ParentDirItem) prop = ParentDirItem->ATime; break;
		case kpidMTime:  if(ParentDirItem) prop = ParentDirItem->MTime; break;
	}
	prop.Detach(value);
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetParent(uint32 /* index */, uint32 * parent, uint32 * parentType)
{
	*parentType = NParentType::kDir;
	*parent = (uint32)(int32)-1;
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetNumRawProps(uint32 * numProps)
{
	*numProps = StoreNtSecurity ? 1 : 0;
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetRawPropInfo(uint32 /* index */, BSTR * name, PROPID * propID)
{
	*name = NULL;
	*propID = kpidNtSecure;
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetRootRawProp(PROPID
    #ifdef _USE_SECURITY_CODE
    propID
    #endif
    , const void ** data, uint32 * dataSize, uint32 * propType)
{
	*data = 0;
	*dataSize = 0;
	*propType = 0;
	if(!StoreNtSecurity)
		return S_OK;
  #ifdef _USE_SECURITY_CODE
	if(propID == kpidNtSecure) {
		if(StdInMode)
			return S_OK;

		if(ParentDirItem) {
			if(ParentDirItem->SecureIndex < 0)
				return S_OK;
			const CByteBuffer &buf = DirItems->SecureBlocks.Bufs[ParentDirItem->SecureIndex];
			*data = buf;
			*dataSize = (uint32)buf.Size();
			*propType = NPropDataType::kRaw;
			return S_OK;
		}
		if(Arc && Arc->GetRootProps)
			return Arc->GetRootProps->GetRootRawProp(propID, data, dataSize, propType);
	}
  #endif
	return S_OK;
}

//    #ifdef _USE_SECURITY_CODE
//    #endif

STDMETHODIMP CArchiveUpdateCallback::GetRawProp(uint32 index, PROPID propID, const void ** data, uint32 * dataSize, uint32 * propType)
{
	*data = 0;
	*dataSize = 0;
	*propType = 0;
	if(oneof2(propID, kpidNtSecure, kpidNtReparse)) {
		if(StdInMode)
			return S_OK;
		const CUpdatePair2 &up = (*UpdatePairs)[index];
		if(up.UseArcProps && up.ExistInArchive() && Arc->GetRawProps)
			return Arc->GetRawProps->GetRawProp(ArcItems ? (*ArcItems)[up.ArcIndex].IndexInServer : up.ArcIndex, propID, data, dataSize, propType);
		{
			/*
			   if(!up.NewData)
			   return E_FAIL;
			 */
			if(up.IsAnti)
				return S_OK;
      #ifndef UNDER_CE
			const CDirItem &di = DirItems->Items[up.DirIndex];
      #endif
      #ifdef _USE_SECURITY_CODE
			if(propID == kpidNtSecure) {
				if(!StoreNtSecurity)
					return S_OK;
				if(di.SecureIndex < 0)
					return S_OK;
				const CByteBuffer &buf = DirItems->SecureBlocks.Bufs[di.SecureIndex];
				*data = buf;
				*dataSize = (uint32)buf.Size();
				*propType = NPropDataType::kRaw;
			}
			else
      #endif
			{
				// propID == kpidNtReparse
				if(!StoreSymLinks)
					return S_OK;
	#ifndef UNDER_CE
				const CByteBuffer * buf = &di.ReparseData2;
				if(buf->Size() == 0)
					buf = &di.ReparseData;
				if(buf->Size() != 0) {
					*data = *buf;
					*dataSize = (uint32)buf->Size();
					*propType = NPropDataType::kRaw;
				}
	#endif
			}

			return S_OK;
		}
	}
	return S_OK;
}

#ifndef UNDER_CE
	static UString GetRelativePath(const UString &to, const UString &from)
	{
		UStringVector partsTo, partsFrom;
		SplitPathToParts(to, partsTo);
		SplitPathToParts(from, partsFrom);
		uint i;
		for(i = 0;; i++) {
			if((i + 1) >= partsFrom.Size() || (i + 1) >= partsTo.Size())
				break;
			if(CompareFileNames(partsFrom[i], partsTo[i]) != 0)
				break;
		}
		if(i == 0) {
	#ifdef _WIN32
			if(NName::IsDrivePath(to) || NName::IsDrivePath(from))
				return to;
	#endif
		}
		UString s;
		uint   k;
		for(k = i + 1; k < partsFrom.Size(); k++)
			s += ".." STRING_PATH_SEPARATOR;
		for(k = i; k < partsTo.Size(); k++) {
			if(k != i)
				s.Add_PathSepar();
			s += partsTo[k];
		}
		return s;
	}
#endif

int CArchiveUpdateCallback::CKeyKeyValPair::Compare(const CKeyKeyValPair & a) const
{
	if(Key1 < a.Key1) return -1;
	if(Key1 > a.Key1) return 1;
	return MyCompare(Key2, a.Key2);
}

STDMETHODIMP CArchiveUpdateCallback::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
{
	COM_TRY_BEGIN
	const CUpdatePair2 &up = (*UpdatePairs)[index];
	NCOM::CPropVariant prop;

	if(up.NewData) {
		/*
		   if(propID == kpidIsHardLink)
		   {
		   prop = _isHardLink;
		   prop.Detach(value);
		   return S_OK;
		   }
		 */
		if(propID == kpidSymLink) {
			if(index == _hardIndex_From) {
				prop.Detach(value);
				return S_OK;
			}
			if(up.DirIndex >= 0) {
	#ifndef UNDER_CE
				const CDirItem &di = DirItems->Items[up.DirIndex];
				// if(di.IsDir())
				{
					CReparseAttr attr;
					if(attr.Parse(di.ReparseData, di.ReparseData.Size())) {
						UString simpleName = attr.GetPath();
						if(attr.IsRelative())
							prop = simpleName;
						else {
							const FString phyPath = DirItems->GetPhyPath(up.DirIndex);
							FString fullPath;
							if(NDir::MyGetFullPathName(phyPath, fullPath)) {
								prop = GetRelativePath(simpleName, fs2us(fullPath));
							}
						}
						prop.Detach(value);
						return S_OK;
					}
				}
	#endif
			}
		}
		else if(propID == kpidHardLink) {
			if(index == _hardIndex_From) {
				const CKeyKeyValPair &pair = _map[_hardIndex_To];
				const CUpdatePair2 &up2 = (*UpdatePairs)[pair.Value];
				prop = DirItems->GetLogPath(up2.DirIndex);
				prop.Detach(value);
				return S_OK;
			}
			if(up.DirIndex >= 0) {
				prop.Detach(value);
				return S_OK;
			}
		}
	}
	if(up.IsAnti && propID != kpidIsDir && propID != kpidPath && propID != kpidIsAltStream) {
		switch(propID) {
			case kpidSize:  prop = (uint64)0; break;
			case kpidIsAnti:  prop = true; break;
		}
	}
	else if(propID == kpidPath && up.NewNameIndex >= 0)
		prop = (*NewNames)[up.NewNameIndex];
	else if(propID == kpidComment && CommentIndex >= 0 && (uint)CommentIndex == index && Comment)
		prop = *Comment;
	else if(propID == kpidShortName && up.NewNameIndex >= 0 && up.IsMainRenameItem) {
		// we can generate new ShortName here;
	}
	else if((up.UseArcProps || (KeepOriginalItemNames && (propID == kpidPath || propID == kpidIsAltStream)))
	    && up.ExistInArchive() && Archive)
		return Archive->GetProperty(ArcItems ? (*ArcItems)[up.ArcIndex].IndexInServer : up.ArcIndex, propID, value);
	else if(up.ExistOnDisk()) {
		const CDirItem &di = DirItems->Items[up.DirIndex];
		switch(propID) {
			case kpidPath:  prop = DirItems->GetLogPath(up.DirIndex); break;
			case kpidIsDir:  prop = di.IsDir(); break;
			case kpidSize:  prop = di.IsDir() ? (uint64)0 : di.Size; break;
			case kpidAttrib:  prop = di.Attrib; break;
			case kpidCTime:  prop = di.CTime; break;
			case kpidATime:  prop = di.ATime; break;
			case kpidMTime:  prop = di.MTime; break;
			case kpidIsAltStream:  prop = di.IsAltStream; break;
      #if defined(_WIN32) && !defined(UNDER_CE)
			    // case kpidShortName:  prop = di.ShortName; break;
      #endif
		}
	}
	prop.Detach(value);
	return S_OK;
	COM_TRY_END
}

#ifndef _7ZIP_ST
	static NSynchronization::CCriticalSection CS;
#endif

STDMETHODIMP CArchiveUpdateCallback::GetStream2(uint32 index, ISequentialInStream ** inStream, uint32 mode)
{
	COM_TRY_BEGIN
	* inStream = NULL;
	const CUpdatePair2 &up = (*UpdatePairs)[index];
	if(!up.NewData)
		return E_FAIL;
	RINOK(Callback->CheckBreak());
	// RINOK(Callback->Finalize());
	bool isDir = IsDir(up);
	if(up.IsAnti) {
		UString name;
		if(up.ArcIndex >= 0)
			name = (*ArcItems)[up.ArcIndex].Name;
		else if(up.DirIndex >= 0)
			name = DirItems->GetLogPath(up.DirIndex);
		RINOK(Callback->GetStream(name, isDir, true, mode));
		// 9.33: fixed. Handlers expect real stream object for files, even for anti-file. so we return empty stream 
		if(!isDir) {
			CBufInStream * inStreamSpec = new CBufInStream();
			CMyComPtr<ISequentialInStream> inStreamLoc = inStreamSpec;
			inStreamSpec->Init(NULL, 0);
			*inStream = inStreamLoc.Detach();
		}
		return S_OK;
	}
	RINOK(Callback->GetStream(DirItems->GetLogPath(up.DirIndex), isDir, false, mode));
	if(isDir)
		return S_OK;
	if(StdInMode) {
		if(mode != NUpdateNotifyOp::kAdd && mode != NUpdateNotifyOp::kUpdate)
			return S_OK;
		CStdInFileStream * inStreamSpec = new CStdInFileStream;
		CMyComPtr<ISequentialInStream> inStreamLoc(inStreamSpec);
		*inStream = inStreamLoc.Detach();
	}
	else {
		CInFileStream * inStreamSpec = new CInFileStream;
		CMyComPtr <ISequentialInStream> inStreamLoc(inStreamSpec);
		inStreamSpec->SupportHardLinks = StoreHardLinks;
		inStreamSpec->Callback = this;
		inStreamSpec->CallbackRef = index;
		const FString path = DirItems->GetPhyPath(up.DirIndex);
		_openFiles_Indexes.Add(index);
		_openFiles_Paths.Add(path);
    #if defined(_WIN32) && !defined(UNDER_CE)
		if(DirItems->Items[up.DirIndex].AreReparseData()) {
			if(!inStreamSpec->File.OpenReparse(path)) {
				return Callback->OpenFileError(path, ::GetLastError());
			}
		}
		else
    #endif
		if(!inStreamSpec->OpenShared(path, ShareForWrite)) {
			return Callback->OpenFileError(path, ::GetLastError());
		}
		if(StoreHardLinks) {
			CStreamFileProps props;
			if(inStreamSpec->GetProps2(&props) == S_OK) {
				if(props.NumLinks > 1) {
					CKeyKeyValPair pair;
					pair.Key1 = props.VolID;
					pair.Key2 = props.FileID_Low;
					pair.Value = index;
					uint   numItems = _map.Size();
					uint   pairIndex = _map.AddToUniqueSorted2(pair);
					if(numItems == _map.Size()) {
						// const CKeyKeyValPair &pair2 = _map.Pairs[pairIndex];
						_hardIndex_From = index;
						_hardIndex_To = pairIndex;
						// we could return NULL as stream, but it's better to return real stream
						// return S_OK;
					}
				}
			}
		}

		if(ProcessedItemsStatuses) {
      #ifndef _7ZIP_ST
			NSynchronization::CCriticalSectionLock lock(CS);
      #endif
			ProcessedItemsStatuses[(uint)up.DirIndex] = 1;
		}
		*inStream = inStreamLoc.Detach();
	}

	return S_OK;
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::SetOperationResult(int32 opRes)
{
	COM_TRY_BEGIN
	return Callback->SetOperationResult(opRes);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetStream(uint32 index, ISequentialInStream ** inStream)
{
	COM_TRY_BEGIN
	return GetStream2(index, inStream, (*UpdatePairs)[index].ArcIndex < 0 ? NUpdateNotifyOp::kAdd : NUpdateNotifyOp::kUpdate);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::ReportOperation(uint32 indexType, uint32 index, uint32 op)
{
	COM_TRY_BEGIN
	bool isDir = false;
	if(indexType == NArchive::NEventIndexType::kOutArcIndex) {
		UString name;
		if(index != (uint32)(int32)-1) {
			const CUpdatePair2 &up = (*UpdatePairs)[index];
			if(up.ExistOnDisk()) {
				name = DirItems->GetLogPath(up.DirIndex);
				isDir = DirItems->Items[up.DirIndex].IsDir();
			}
		}
		return Callback->ReportUpdateOpeartion(op, name.IsEmpty() ? NULL : name.Ptr(), isDir);
	}
	wchar_t temp[16];
	UString s2;
	const wchar_t * s = NULL;
	if(indexType == NArchive::NEventIndexType::kInArcIndex) {
		if(index != (uint32)(int32)-1) {
			if(ArcItems) {
				const CArcItem &ai = (*ArcItems)[index];
				s = ai.Name;
				isDir = ai.IsDir;
			}
			else if(Arc) {
				RINOK(Arc->GetItemPath(index, s2));
				s = s2;
				RINOK(Archive_IsItem_Dir(Arc->Archive, index, isDir));
			}
		}
	}
	else if(indexType == NArchive::NEventIndexType::kBlockIndex) {
		temp[0] = '#';
		ConvertUInt32ToString(index, temp + 1);
		s = temp;
	}
	SETIFZ(s, L"");
	return Callback->ReportUpdateOpeartion(op, s, isDir);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::ReportExtractResult(uint32 indexType, uint32 index, int32 opRes)
{
	COM_TRY_BEGIN
	bool isEncrypted = false;
	wchar_t temp[16];
	UString s2;
	const wchar_t * s = NULL;
	if(indexType == NArchive::NEventIndexType::kOutArcIndex) {
		/*
		   UString name;
		   if(index != (uint32)(int32)-1) {
		   const CUpdatePair2 &up = (*UpdatePairs)[index];
		   if(up.ExistOnDisk()) {
		    s2 = DirItems->GetLogPath(up.DirIndex);
		    s = s2;
		   }
		   }
		 */
		return E_FAIL;
	}
	if(indexType == NArchive::NEventIndexType::kInArcIndex) {
		if(index != (uint32)(int32)-1) {
			if(ArcItems)
				s = (*ArcItems)[index].Name;
			else if(Arc) {
				RINOK(Arc->GetItemPath(index, s2));
				s = s2;
			}
			if(Archive) {
				RINOK(Archive_GetItemBoolProp(Archive, index, kpidEncrypted, isEncrypted));
			}
		}
	}
	else if(indexType == NArchive::NEventIndexType::kBlockIndex) {
		temp[0] = '#';
		ConvertUInt32ToString(index, temp + 1);
		s = temp;
	}
	return Callback->ReportExtractResult(opRes, BoolToInt(isEncrypted), s);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeSize(uint32 index, uint64 * size)
{
	if(VolumesSizes.Size() == 0)
		return S_FALSE;
	if(index >= (uint32)VolumesSizes.Size())
		index = VolumesSizes.Size() - 1;
	*size = VolumesSizes[index];
	return S_OK;
}

STDMETHODIMP CArchiveUpdateCallback::GetVolumeStream(uint32 index, ISequentialOutStream ** volumeStream)
{
	COM_TRY_BEGIN
	char temp[16];
	ConvertUInt32ToString(index + 1, temp);
	FString res(temp);
	while(res.Len() < 2)
		res.InsertAtFront(FTEXT('0'));
	FString fileName = VolName;
	fileName += '.';
	fileName += res;
	fileName += VolExt;
	COutFileStream * streamSpec = new COutFileStream;
	CMyComPtr<ISequentialOutStream> streamLoc(streamSpec);
	if(!streamSpec->Create(fileName, false))
		return ::GetLastError();
	*volumeStream = streamLoc.Detach();
	return S_OK;
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword2(int32 * passwordIsDefined, BSTR * password)
{
	COM_TRY_BEGIN
	return Callback->CryptoGetTextPassword2(passwordIsDefined, password);
	COM_TRY_END
}

STDMETHODIMP CArchiveUpdateCallback::CryptoGetTextPassword(BSTR * password)
{
	COM_TRY_BEGIN
	return Callback->CryptoGetTextPassword(password);
	COM_TRY_END
}

HRESULT CArchiveUpdateCallback::InFileStream_On_Error(UINT_PTR val, DWORD error)
{
	if(error == ERROR_LOCK_VIOLATION) {
		MT_LOCK
		uint32 index = (uint32)val;
		FOR_VECTOR(i, _openFiles_Indexes) {
			if(_openFiles_Indexes[i] == index) {
				RINOK(Callback->ReadingFileError(_openFiles_Paths[i], error));
				break;
			}
		}
	}
	return HRESULT_FROM_WIN32(error);
}

void CArchiveUpdateCallback::InFileStream_On_Destroy(UINT_PTR val)
{
	MT_LOCK
	uint32 index = (uint32)val;
	FOR_VECTOR(i, _openFiles_Indexes) {
		if(_openFiles_Indexes[i] == index) {
			_openFiles_Indexes.Delete(i);
			_openFiles_Paths.Delete(i);
			return;
		}
	}
	throw 20141125;
}
//
static const char * const kUpdateIsNotSupoorted = "update operations are not supported for this archive";
static const char * const kUpdateIsNotSupoorted_MultiVol = "Updating for multivolume archives is not implemented";
static CFSTR const kTempFolderPrefix = FTEXT("7zE");

CUpdateErrorInfo::CUpdateErrorInfo() : SystemError(0) 
{
}

bool CUpdateErrorInfo::ThereIsError() const { return SystemError != 0 || !Message.IsEmpty() || !FileNames.IsEmpty(); }
HRESULT CUpdateErrorInfo::Get_HRESULT_Error() const { return SystemError == 0 ? E_FAIL : HRESULT_FROM_WIN32(SystemError); }

void CUpdateErrorInfo::SetFromLastError(const char * message)
{
	SystemError = ::GetLastError();
	Message = message;
}

HRESULT CUpdateErrorInfo::SetFromLastError(const char * message, const FString &fileName)
{
	SetFromLastError(message);
	FileNames.Add(fileName);
	return Get_HRESULT_Error();
}

static bool DeleteEmptyFolderAndEmptySubFolders(const FString &path)
{
	NFind::CFileInfo fileInfo;
	FString pathPrefix = path + FCHAR_PATH_SEPARATOR;
	{
		NFind::CEnumerator enumerator;
		enumerator.SetDirPrefix(pathPrefix);
		while(enumerator.Next(fileInfo)) {
			if(fileInfo.IsDir())
				if(!DeleteEmptyFolderAndEmptySubFolders(pathPrefix + fileInfo.Name))
					return false;
		}
	}
	/*
	   // we don't need clear read-only for folders
	   if(!MySetFileAttributes(path, 0))
	   return false;
	 */
	return RemoveDir(path);
}
//
//#include <TempFiles.h>
class CTempFiles {
public:
	~CTempFiles() 
	{
		Clear();
	}
	FStringVector Paths;
private:
	void Clear()
	{
		while(!Paths.IsEmpty()) {
			NDir::DeleteFileAlways(Paths.Back());
			Paths.DeleteBack();
		}
	}
};
//
class COutMultiVolStream : public IOutStream, public CMyUnknownImp {
	uint   _streamIndex; // required stream
	uint64 _offsetPos; // offset from start of _streamIndex index
	uint64 _absPos;
	uint64 _length;
	struct CAltStreamInfo {
		COutFileStream * StreamSpec;
		CMyComPtr<IOutStream> Stream;
		FString Name;
		uint64 Pos;
		uint64 RealSize;
	};
	CObjectVector<CAltStreamInfo> Streams;
public:
	// CMyComPtr<IArchiveUpdateCallback2> VolumeCallback;
	CRecordVector<uint64> Sizes;
	FString Prefix;
	CTempFiles * TempFiles;
	void Init()
	{
		_streamIndex = 0;
		_offsetPos = 0;
		_absPos = 0;
		_length = 0;
	}
	bool SetMTime(const FILETIME * mTime);
	HRESULT Close();
	uint64 GetSize() const { return _length; }
	MY_UNKNOWN_IMP1(IOutStream)

	STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
	STDMETHOD(Seek) (int64 offset, uint32 seekOrigin, uint64 *newPosition);
	STDMETHOD(SetSize) (uint64 newSize);
};

// static NSynchronization::CCriticalSection g_TempPathsCS;

HRESULT COutMultiVolStream::Close()
{
	HRESULT res = S_OK;
	FOR_VECTOR(i, Streams) {
		COutFileStream * s = Streams[i].StreamSpec;
		if(s) {
			HRESULT res2 = s->Close();
			if(res2 != S_OK)
				res = res2;
		}
	}
	return res;
}

bool COutMultiVolStream::SetMTime(const FILETIME * mTime)
{
	bool res = true;
	FOR_VECTOR(i, Streams) {
		COutFileStream * s = Streams[i].StreamSpec;
		if(s)
			if(!s->SetMTime(mTime))
				res = false;
	}
	return res;
}

STDMETHODIMP COutMultiVolStream::Write(const void * data, uint32 size, uint32 * processedSize)
{
	ASSIGN_PTR(processedSize, 0);
	while(size > 0) {
		if(_streamIndex >= Streams.Size()) {
			CAltStreamInfo altStream;
			FString name;
			name.Add_UInt32(_streamIndex + 1);
			while(name.Len() < 3)
				name.InsertAtFront(FTEXT('0'));
			name.Insert(0, Prefix);
			altStream.StreamSpec = new COutFileStream;
			altStream.Stream = altStream.StreamSpec;
			if(!altStream.StreamSpec->Create(name, false))
				return ::GetLastError();
			{
				// NSynchronization::CCriticalSectionLock lock(g_TempPathsCS);
				TempFiles->Paths.Add(name);
			}
			altStream.Pos = 0;
			altStream.RealSize = 0;
			altStream.Name = name;
			Streams.Add(altStream);
			continue;
		}
		CAltStreamInfo &altStream = Streams[_streamIndex];
		uint   index = _streamIndex;
		if(index >= Sizes.Size())
			index = Sizes.Size() - 1;
		uint64 volSize = Sizes[index];
		if(_offsetPos >= volSize) {
			_offsetPos -= volSize;
			_streamIndex++;
			continue;
		}
		if(_offsetPos != altStream.Pos) {
			// CMyComPtr<IOutStream> outStream;
			// RINOK(altStream.Stream.QueryInterface(IID_IOutStream, &outStream));
			RINOK(altStream.Stream->Seek(_offsetPos, STREAM_SEEK_SET, NULL));
			altStream.Pos = _offsetPos;
		}
		uint32 curSize = (uint32)MyMin((uint64)size, volSize - altStream.Pos);
		uint32 realProcessed;
		RINOK(altStream.Stream->Write(data, curSize, &realProcessed));
		data = (void *)((Byte*)data + realProcessed);
		size -= realProcessed;
		altStream.Pos += realProcessed;
		_offsetPos += realProcessed;
		_absPos += realProcessed;
		if(_absPos > _length)
			_length = _absPos;
		if(_offsetPos > altStream.RealSize)
			altStream.RealSize = _offsetPos;
		if(processedSize)
			*processedSize += realProcessed;
		if(altStream.Pos == volSize) {
			_streamIndex++;
			_offsetPos = 0;
		}
		if(realProcessed == 0 && curSize != 0)
			return E_FAIL;
		break;
	}
	return S_OK;
}

STDMETHODIMP COutMultiVolStream::Seek(int64 offset, uint32 seekOrigin, uint64 * newPosition)
{
	if(seekOrigin >= 3)
		return STG_E_INVALIDFUNCTION;
	switch(seekOrigin) {
		case STREAM_SEEK_SET: _absPos = offset; break;
		case STREAM_SEEK_CUR: _absPos += offset; break;
		case STREAM_SEEK_END: _absPos = _length + offset; break;
	}
	_offsetPos = _absPos;
	ASSIGN_PTR(newPosition, _absPos);
	_streamIndex = 0;
	return S_OK;
}

STDMETHODIMP COutMultiVolStream::SetSize(uint64 newSize)
{
	uint   i = 0;
	while(i < Streams.Size()) {
		CAltStreamInfo &altStream = Streams[i++];
		if((uint64)newSize < altStream.RealSize) {
			RINOK(altStream.Stream->SetSize(newSize));
			altStream.RealSize = newSize;
			break;
		}
		newSize -= altStream.RealSize;
	}
	while(i < Streams.Size()) {
		{
			CAltStreamInfo &altStream = Streams.Back();
			altStream.Stream.Release();
			DeleteFileAlways(altStream.Name);
		}
		Streams.DeleteBack();
	}
	_offsetPos = _absPos;
	_streamIndex = 0;
	_length = newSize;
	return S_OK;
}

CArchivePath::CArchivePath() : Temp(false) 
{
}

void CArchivePath::ParseFromPath(const UString &path, EArcNameMode mode)
{
	OriginalPath = path;
	SplitPathToParts_2(path, Prefix, Name);
	if(mode == k_ArcNameMode_Add)
		return;
	if(mode == k_ArcNameMode_Exact) {
		BaseExtension.Empty();
		return;
	}
	int dotPos = Name.ReverseFind_Dot();
	if(dotPos < 0)
		return;
	if((uint)dotPos == Name.Len() - 1) {
		Name.DeleteBack();
		BaseExtension.Empty();
		return;
	}
	const UString ext = Name.Ptr(dotPos + 1);
	if(BaseExtension.IsEqualTo_NoCase(ext)) {
		BaseExtension = ext;
		Name.DeleteFrom(dotPos);
	}
	else
		BaseExtension.Empty();
}

UString CArchivePath::GetFinalPath() const
{
	UString path = GetPathWithoutExt();
	if(!BaseExtension.IsEmpty()) {
		path += '.';
		path += BaseExtension;
	}
	return path;
}

UString CArchivePath::GetFinalVolPath() const
{
	UString path = GetPathWithoutExt();
	if(!BaseExtension.IsEmpty()) {
		path += '.';
		path += VolExtension;
	}
	return path;
}

FString CArchivePath::GetTempPath() const
{
	FString path = TempPrefix;
	path += us2fs(Name);
	if(!BaseExtension.IsEmpty()) {
		path += '.';
		path += us2fs(BaseExtension);
	}
	path += ".tmp";
	path += TempPostfix;
	return path;
}

static const char * const kDefaultArcType = "7z";
static const char * const kDefaultArcExt = "7z";
#ifdef _WIN32
    static const char * const kSFXExtension = "exe";
#else
    static const char * const kSFXExtension = "";
#endif

CUpdateOptions::CUpdateOptions() : UpdateArchiveItself(true), SfxMode(false), StdInMode(false),
	StdOutMode(false), EMailMode(false), EMailRemoveAfter(false), OpenShareForWrite(false),
	ArcNameMode(k_ArcNameMode_Smart), PathMode(NWildcard::k_RelatPath), DeleteAfterCompressing(false),
	SetArcMTime(false) 
{
}

void CUpdateOptions::SetActionCommand_Add()
{
	Commands.Clear();
	CUpdateArchiveCommand c;
	c.ActionSet = NUpdateArchive::k_ActionSet_Add;
	Commands.Add(c);
}

bool CUpdateOptions::InitFormatIndex(const CCodecs * codecs, const CObjectVector<COpenType> &types, const UString &arcPath)
{
	if(types.Size() > 1)
		return false;
	// int arcTypeIndex = -1;
	if(types.Size() != 0) {
		MethodMode.Type = types[0];
		MethodMode.Type_Defined = true;
	}
	if(MethodMode.Type.FormatIndex < 0) {
		// MethodMode.Type = -1;
		MethodMode.Type = COpenType();
		if(ArcNameMode != k_ArcNameMode_Add) {
			MethodMode.Type.FormatIndex = codecs->FindFormatForArchiveName(arcPath);
			if(MethodMode.Type.FormatIndex >= 0)
				MethodMode.Type_Defined = true;
		}
	}
	return true;
}

bool CUpdateOptions::SetArcPath(const CCodecs * codecs, const UString &arcPath)
{
	UString typeExt;
	int formatIndex = MethodMode.Type.FormatIndex;
	if(formatIndex < 0) {
		typeExt = kDefaultArcExt;
	}
	else {
		const CArcInfoEx &arcInfo = codecs->Formats[formatIndex];
		if(!arcInfo.UpdateEnabled)
			return false;
		typeExt = arcInfo.GetMainExt();
	}
	UString ext = typeExt;
	if(SfxMode)
		ext = kSFXExtension;
	ArchivePath.BaseExtension = ext;
	ArchivePath.VolExtension = typeExt;
	ArchivePath.ParseFromPath(arcPath, ArcNameMode);
	FOR_VECTOR(i, Commands) {
		CUpdateArchiveCommand &uc = Commands[i];
		uc.ArchivePath.BaseExtension = ext;
		uc.ArchivePath.VolExtension = typeExt;
		uc.ArchivePath.ParseFromPath(uc.UserArchivePath, ArcNameMode);
	}
	return true;
}

struct CUpdateProduceCallbackImp : public IUpdateProduceCallback {
	const CObjectVector <CArcItem> * _arcItems;
	IUpdateCallbackUI * _callback;
	CDirItemsStat * P_Stat;

	CUpdateProduceCallbackImp(const CObjectVector<CArcItem> * a, CDirItemsStat * stat, IUpdateCallbackUI * callback) : 
		_arcItems(a), P_Stat(stat), _callback(callback) 
	{
	}
	virtual HRESULT ShowDeleteFile(unsigned arcIndex);
};

HRESULT CUpdateProduceCallbackImp::ShowDeleteFile(unsigned arcIndex)
{
	const CArcItem &ai = (*_arcItems)[arcIndex];
	{
		CDirItemsStat &stat = *P_Stat;
		if(ai.IsDir)
			stat.NumDirs++;
		else if(ai.IsAltStream) {
			stat.NumAltStreams++;
			stat.AltStreamsSize += ai.Size;
		}
		else {
			stat.NumFiles++;
			stat.FilesSize += ai.Size;
		}
	}
	return _callback->ShowDeleteFile(ai.Name, ai.IsDir);
}

CRenamePair::CRenamePair() : WildcardParsing(true), RecursedType(NRecursedType::kNonRecursed) 
{
}

bool CRenamePair::Prepare()
{
	if(RecursedType != NRecursedType::kNonRecursed)
		return false;
	else if(!WildcardParsing)
		return true;
	else
		return !DoesNameContainWildcard(OldName);
}

static uint CompareTwoNames(const wchar_t * s1, const wchar_t * s2)
{
	for(uint i = 0;; i++) {
		wchar_t c1 = s1[i];
		wchar_t c2 = s2[i];
		if(c1 == 0 || c2 == 0)
			return i;
		else if(c1 != c2) {
			if(!g_CaseSensitive && (MyCharUpper(c1) == MyCharUpper(c2)))
				continue;
			if(IsPathSepar(c1) && IsPathSepar(c2))
				continue;
			return i;
		}
	}
}

bool CRenamePair::GetNewPath(bool isFolder, const UString &src, UString &dest) const
{
	uint   num = CompareTwoNames(OldName, src);
	if(OldName[num] == 0) {
		if(src[num] != 0 && !IsPathSepar(src[num]) && num != 0 && !IsPathSepar(src[num - 1]))
			return false;
	}
	else {
		// OldName[num] != 0
		// OldName = "1\1a.txt"
		// src = "1"
		if(!isFolder || src[num] != 0 || !IsPathSepar(OldName[num]) || OldName[num + 1] != 0)
			return false;
	}
	dest = NewName + src.Ptr(num);
	return true;
}

#ifdef SUPPORT_ALT_STREAMS
	int FindAltStreamColon_in_Path(const wchar_t * path);
#endif

static HRESULT Compress(const CUpdateOptions &options, bool isUpdatingItself, CCodecs * codecs,
    const CActionSet &actionSet, const CArc * arc, CArchivePath &archivePath, const CObjectVector<CArcItem> &arcItems,
    Byte * processedItemsStatuses, const CDirItems &dirItems, const CDirItem * parentDirItem, CTempFiles &tempFiles,
    CUpdateErrorInfo &errorInfo, IUpdateCallbackUI * callback, CFinishArchiveStat &st)
{
	CMyComPtr <IOutArchive> outArchive;
	int formatIndex = options.MethodMode.Type.FormatIndex;
	if(arc) {
		formatIndex = arc->FormatIndex;
		if(formatIndex < 0)
			return E_NOTIMPL;
		CMyComPtr<IInArchive> archive2 = arc->Archive;
		HRESULT result = archive2.QueryInterface(IID_IOutArchive, &outArchive);
		if(result != S_OK)
			throw kUpdateIsNotSupoorted;
	}
	else {
		RINOK(codecs->CreateOutArchive(formatIndex, outArchive));
    #ifdef EXTERNAL_CODECS
		{
			CMyComPtr<ISetCompressCodecsInfo> setCompressCodecsInfo;
			outArchive.QueryInterface(IID_ISetCompressCodecsInfo, (void**)&setCompressCodecsInfo);
			if(setCompressCodecsInfo) {
				RINOK(setCompressCodecsInfo->SetCompressCodecsInfo(codecs));
			}
		}
    #endif
	}
	if(outArchive == 0)
		throw kUpdateIsNotSupoorted;
	NFileTimeType::EEnum fileTimeType;
	{
		uint32 value;
		RINOK(outArchive->GetFileTimeType(&value));
		switch(value) {
			case NFileTimeType::kWindows:
			case NFileTimeType::kUnix:
			case NFileTimeType::kDOS:
			    fileTimeType = (NFileTimeType::EEnum)value;
			    break;
			default:
			    return E_FAIL;
		}
	}
	{
		const CArcInfoEx &arcInfo = codecs->Formats[formatIndex];
		if(options.AltStreams.Val && !arcInfo.Flags_AltStreams())
			return E_NOTIMPL;
		if(options.NtSecurity.Val && !arcInfo.Flags_NtSecure())
			return E_NOTIMPL;
	}
	CRecordVector<CUpdatePair2> updatePairs2;
	UStringVector newNames;
	CArcToDoStat stat2;
	if(options.RenamePairs.Size() != 0) {
		FOR_VECTOR(i, arcItems) {
			const CArcItem &ai = arcItems[i];
			bool needRename = false;
			UString dest;
			if(ai.Censored) {
				FOR_VECTOR(j, options.RenamePairs) {
					const CRenamePair &rp = options.RenamePairs[j];
					if(rp.GetNewPath(ai.IsDir, ai.Name, dest)) {
						needRename = true;
						break;
					}

	  #ifdef SUPPORT_ALT_STREAMS
					if(ai.IsAltStream) {
						int colonPos = FindAltStreamColon_in_Path(ai.Name);
						if(colonPos >= 0) {
							UString mainName = ai.Name.Left(colonPos);
							/*
							   actually we must improve that code to support cases
							   with folder renaming like: rn arc dir1\ dir2\
							 */
							if(rp.GetNewPath(false, mainName, dest)) {
								needRename = true;
								dest += ':';
								dest += ai.Name.Ptr(colonPos + 1);
								break;
							}
						}
					}
	  #endif
				}
			}
			CUpdatePair2 up2;
			up2.SetAs_NoChangeArcItem(ai.IndexInServer);
			if(needRename) {
				up2.NewProps = true;
				RINOK(arc->IsItemAnti(i, up2.IsAnti));
				up2.NewNameIndex = newNames.Add(dest);
			}
			updatePairs2.Add(up2);
		}
	}
	else {
		CRecordVector <CUpdatePair> updatePairs;
		GetUpdatePairInfoList(dirItems, arcItems, fileTimeType, updatePairs); // must be done only once!!!
		CUpdateProduceCallbackImp upCallback(&arcItems, &stat2.DeleteData, callback);
		UpdateProduce(updatePairs, actionSet, updatePairs2, isUpdatingItself ? &upCallback : NULL);
	}
	{
		FOR_VECTOR(i, updatePairs2) {
			const CUpdatePair2 &up = updatePairs2[i];
			if(up.NewData) {
				CDirItemsStat &stat = stat2.NewData;
				const CDirItem &di = dirItems.Items[up.DirIndex];
				if(di.IsDir())
					stat.NumDirs++;
				else if(di.IsAltStream) {
					stat.NumAltStreams++;
					stat.AltStreamsSize += di.Size;
				}
				else {
					stat.NumFiles++;
					stat.FilesSize += di.Size;
				}
			}
			else if(up.ArcIndex >= 0) {
				CDirItemsStat &stat = stat2.OldData;
				const CArcItem &ai = arcItems[up.ArcIndex];
				if(ai.IsDir)
					stat.NumDirs++;
				else if(ai.IsAltStream) {
					stat.NumAltStreams++;
					stat.AltStreamsSize += ai.Size;
				}
				else {
					stat.NumFiles++;
					stat.FilesSize += ai.Size;
				}
			}
		}
		RINOK(callback->SetNumItems(stat2));
	}

	CArchiveUpdateCallback * updateCallbackSpec = new CArchiveUpdateCallback;
	CMyComPtr <IArchiveUpdateCallback> updateCallback(updateCallbackSpec);

	updateCallbackSpec->ShareForWrite = options.OpenShareForWrite;
	updateCallbackSpec->StdInMode = options.StdInMode;
	updateCallbackSpec->Callback = callback;

	if(arc) {
		// we set Archive to allow to transfer GetProperty requests back to DLL.
		updateCallbackSpec->Archive = arc->Archive;
	}
	updateCallbackSpec->DirItems = &dirItems;
	updateCallbackSpec->ParentDirItem = parentDirItem;
	updateCallbackSpec->StoreNtSecurity = options.NtSecurity.Val;
	updateCallbackSpec->StoreHardLinks = options.HardLinks.Val;
	updateCallbackSpec->StoreSymLinks = options.SymLinks.Val;
	updateCallbackSpec->Arc = arc;
	updateCallbackSpec->ArcItems = &arcItems;
	updateCallbackSpec->UpdatePairs = &updatePairs2;
	updateCallbackSpec->ProcessedItemsStatuses = processedItemsStatuses;
	if(options.RenamePairs.Size() != 0)
		updateCallbackSpec->NewNames = &newNames;
	CMyComPtr<IOutStream> outSeekStream;
	CMyComPtr<ISequentialOutStream> outStream;
	if(!options.StdOutMode) {
		FString dirPrefix;
		if(!GetOnlyDirPrefix(us2fs(archivePath.GetFinalPath()), dirPrefix))
			throw 1417161;
		CreateComplexDir(dirPrefix);
	}
	COutFileStream * outStreamSpec = NULL;
	CStdOutFileStream * stdOutFileStreamSpec = NULL;
	COutMultiVolStream * volStreamSpec = NULL;
	if(options.VolumesSizes.Size() == 0) {
		if(options.StdOutMode) {
			stdOutFileStreamSpec = new CStdOutFileStream;
			outStream = stdOutFileStreamSpec;
		}
		else {
			outStreamSpec = new COutFileStream;
			outSeekStream = outStreamSpec;
			outStream = outSeekStream;
			bool isOK = false;
			FString realPath;
			for(uint i = 0; i < (1 << 16); i++) {
				if(archivePath.Temp) {
					if(i > 0) {
						archivePath.TempPostfix.Empty();
						archivePath.TempPostfix.Add_UInt32(i);
					}
					realPath = archivePath.GetTempPath();
				}
				else
					realPath = us2fs(archivePath.GetFinalPath());
				if(outStreamSpec->Create(realPath, false)) {
					tempFiles.Paths.Add(realPath);
					isOK = true;
					break;
				}
				if(::GetLastError() != ERROR_FILE_EXISTS)
					break;
				if(!archivePath.Temp)
					break;
			}
			if(!isOK)
				return errorInfo.SetFromLastError("cannot open file", realPath);
		}
	}
	else {
		if(options.StdOutMode)
			return E_FAIL;
		if(arc && arc->GetGlobalOffset() > 0)
			return E_NOTIMPL;
		volStreamSpec = new COutMultiVolStream;
		outSeekStream = volStreamSpec;
		outStream = outSeekStream;
		volStreamSpec->Sizes = options.VolumesSizes;
		volStreamSpec->Prefix = us2fs(archivePath.GetFinalVolPath());
		volStreamSpec->Prefix += '.';
		volStreamSpec->TempFiles = &tempFiles;
		volStreamSpec->Init();
		/*
		   updateCallbackSpec->VolumesSizes = volumesSizes;
		   updateCallbackSpec->VolName = archivePath.Prefix + archivePath.Name;
		   if(!archivePath.VolExtension.IsEmpty())
		   updateCallbackSpec->VolExt = UString('.') + archivePath.VolExtension;
		 */
	}
	RINOK(SetProperties(outArchive, options.MethodMode.Properties));
	if(options.SfxMode) {
		CInFileStream * sfxStreamSpec = new CInFileStream;
		CMyComPtr<IInStream> sfxStream(sfxStreamSpec);
		if(!sfxStreamSpec->Open(options.SfxModule))
			return errorInfo.SetFromLastError("cannot open SFX module", options.SfxModule);
		CMyComPtr<ISequentialOutStream> sfxOutStream;
		COutFileStream * outStreamSpec2 = NULL;
		if(options.VolumesSizes.Size() == 0)
			sfxOutStream = outStream;
		else {
			outStreamSpec2 = new COutFileStream;
			sfxOutStream = outStreamSpec2;
			FString realPath = us2fs(archivePath.GetFinalPath());
			if(!outStreamSpec2->Create(realPath, false))
				return errorInfo.SetFromLastError("cannot open file", realPath);
		}
		{
			uint64 sfxSize;
			RINOK(sfxStreamSpec->GetSize(&sfxSize));
			RINOK(callback->WriteSfx(fs2us(options.SfxModule), sfxSize));
		}
		RINOK(NCompress::CopyStream(sfxStream, sfxOutStream, NULL));
		if(outStreamSpec2) {
			RINOK(outStreamSpec2->Close());
		}
	}
	CMyComPtr<ISequentialOutStream> tailStream;
	if(options.SfxMode || !arc || arc->ArcStreamOffset == 0)
		tailStream = outStream;
	else {
		// int64 globalOffset = arc->GetGlobalOffset();
		RINOK(arc->InStream->Seek(0, STREAM_SEEK_SET, NULL));
		RINOK(NCompress::CopyStream_ExactSize(arc->InStream, outStream, arc->ArcStreamOffset, NULL));
		if(options.StdOutMode)
			tailStream = outStream;
		else {
			CTailOutStream * tailStreamSpec = new CTailOutStream;
			tailStream = tailStreamSpec;
			tailStreamSpec->Stream = outSeekStream;
			tailStreamSpec->Offset = arc->ArcStreamOffset;
			tailStreamSpec->Init();
		}
	}
	HRESULT result = outArchive->UpdateItems(tailStream, updatePairs2.Size(), updateCallback);
	// callback->Finalize();
	RINOK(result);
	if(!updateCallbackSpec->AreAllFilesClosed()) {
		errorInfo.Message = "There are unclosed input file:";
		errorInfo.FileNames = updateCallbackSpec->_openFiles_Paths;
		return E_FAIL;
	}
	if(options.SetArcMTime) {
		FILETIME ft;
		ft.dwLowDateTime = 0;
		ft.dwHighDateTime = 0;
		FOR_VECTOR(i, updatePairs2) {
			CUpdatePair2 &pair2 = updatePairs2[i];
			const FILETIME * ft2 = NULL;
			if(pair2.NewProps && pair2.DirIndex >= 0)
				ft2 = &dirItems.Items[pair2.DirIndex].MTime;
			else if(pair2.UseArcProps && pair2.ArcIndex >= 0)
				ft2 = &arcItems[pair2.ArcIndex].MTime;
			if(ft2) {
				if(::CompareFileTime(&ft, ft2) < 0)
					ft = *ft2;
			}
		}
		if(ft.dwLowDateTime != 0 || ft.dwHighDateTime != 0) {
			if(outStreamSpec)
				outStreamSpec->SetMTime(&ft);
			else if(volStreamSpec)
				volStreamSpec->SetMTime(&ft); ;
		}
	}
	if(callback) {
		uint64 size = 0;
		if(outStreamSpec)
			outStreamSpec->GetSize(&size);
		else if(stdOutFileStreamSpec)
			size = stdOutFileStreamSpec->GetSize();
		else
			size = volStreamSpec->GetSize();
		st.OutArcFileSize = size;
	}
	if(outStreamSpec)
		result = outStreamSpec->Close();
	else if(volStreamSpec)
		result = volStreamSpec->Close();
	return result;
}

bool CensorNode_CheckPath2(const NWildcard::CCensorNode &node, const CReadArcItem &item, bool &include);

static bool Censor_CheckPath(const NWildcard::CCensor &censor, const CReadArcItem &item)
{
	bool finded = false;
	FOR_VECTOR(i, censor.Pairs) {
		bool include;
		if(CensorNode_CheckPath2(censor.Pairs[i].Head, item, include)) {
			if(!include)
				return false;
			finded = true;
		}
	}
	return finded;
}

static HRESULT EnumerateInArchiveItems(/*bool storeStreamsMode,*/ const NWildcard::CCensor &censor, const CArc &arc, CObjectVector<CArcItem> &arcItems)
{
	arcItems.Clear();
	uint32 numItems;
	IInArchive * archive = arc.Archive;
	RINOK(archive->GetNumberOfItems(&numItems));
	arcItems.ClearAndReserve(numItems);
	CReadArcItem item;
	for(uint32 i = 0; i < numItems; i++) {
		CArcItem ai;
		RINOK(arc.GetItem(i, item));
		ai.Name = item.Path;
		ai.IsDir = item.IsDir;
	#ifdef SUPPORT_ALT_STREAMS
		    ai.IsAltStream = item.IsAltStream;
	#else
		    ai.IsAltStream = false;
	#endif
		/*
		   if(!storeStreamsMode && ai.IsAltStream)
		   continue;
		 */
		ai.Censored = Censor_CheckPath(censor, item);
		RINOK(arc.GetItemMTime(i, ai.MTime, ai.MTimeDefined));
		RINOK(arc.GetItemSize(i, ai.Size, ai.SizeDefined));
		{
			CPropVariant prop;
			RINOK(archive->GetProperty(i, kpidTimeType, &prop));
			if(prop.vt == VT_UI4) {
				ai.TimeType = (int)(NFileTimeType::EEnum)prop.ulVal;
				switch(ai.TimeType) {
					case NFileTimeType::kWindows:
					case NFileTimeType::kUnix:
					case NFileTimeType::kDOS:
					    break;
					default:
					    return E_FAIL;
				}
			}
		}
		ai.IndexInServer = i;
		arcItems.AddInReserved(ai);
	}
	return S_OK;
}

struct CRefSortPair {
	uint   Len;
	uint   Index;
};

#define RINOZ(x) { int __tt = (x); if(__tt != 0) return __tt; }

static int CompareRefSortPair(const CRefSortPair * a1, const CRefSortPair * a2, void *)
{
	RINOZ(-MyCompare(a1->Len, a2->Len));
	return MyCompare(a1->Index, a2->Index);
}

#ifdef _WIN32
	void ConvertToLongNames(NWildcard::CCensor &censor);
#endif

HRESULT UpdateArchive(CCodecs * codecs, const CObjectVector<COpenType> &types, const UString &cmdArcPath2,
    NWildcard::CCensor &censor, CUpdateOptions &options, CUpdateErrorInfo &errorInfo, IOpenCallbackUI * openCallback,
    IUpdateCallbackUI2 * callback, bool needSetPath)
{
	if(options.StdOutMode && options.EMailMode)
		return E_FAIL;
	if(types.Size() > 1)
		return E_NOTIMPL;
	bool renameMode = !options.RenamePairs.IsEmpty();
	if(renameMode) {
		if(options.Commands.Size() != 1)
			return E_FAIL;
	}
	if(options.DeleteAfterCompressing) {
		if(options.Commands.Size() != 1)
			return E_NOTIMPL;
		const CActionSet &as = options.Commands[0].ActionSet;
		for(int i = 2; i < NPairState::kNumValues; i++)
			if(as.StateActions[i] != NPairAction::kCompress)
				return E_NOTIMPL;
	}
	censor.AddPathsToCensor(options.PathMode);
  #ifdef _WIN32
	ConvertToLongNames(censor);
  #endif
	censor.ExtendExclude();
	if(options.VolumesSizes.Size() > 0 && (options.EMailMode /* || options.SfxMode */))
		return E_NOTIMPL;
	if(options.SfxMode) {
		CProperty property;
		property.Name = "rsfx";
		options.MethodMode.Properties.Add(property);
		if(options.SfxModule.IsEmpty()) {
			errorInfo.Message = "SFX file is not specified";
			return E_FAIL;
		}
		bool found = false;
		if(options.SfxModule.Find(FCHAR_PATH_SEPARATOR) < 0) {
			const FString fullName = NDLL::GetModuleDirPrefix() + options.SfxModule;
			if(NFind::DoesFileExist(fullName)) {
				options.SfxModule = fullName;
				found = true;
			}
		}
		if(!found) {
			if(!NFind::DoesFileExist(options.SfxModule))
				return errorInfo.SetFromLastError("cannot find specified SFX module", options.SfxModule);
		}
	}
	CArchiveLink arcLink;
	if(needSetPath) {
		if(!options.InitFormatIndex(codecs, types, cmdArcPath2) || !options.SetArcPath(codecs, cmdArcPath2))
			return E_NOTIMPL;
	}
	UString arcPath = options.ArchivePath.GetFinalPath();
	if(!options.VolumesSizes.IsEmpty()) {
		arcPath = options.ArchivePath.GetFinalVolPath();
		arcPath += '.';
		arcPath += "001";
	}
	if(cmdArcPath2.IsEmpty()) {
		if(options.MethodMode.Type.FormatIndex < 0)
			throw "type of archive is not specified";
	}
	else {
		NFind::CFileInfo fi;
		if(!fi.Find(us2fs(arcPath))) {
			if(renameMode)
				throw "can't find archive"; ;
			if(options.MethodMode.Type.FormatIndex < 0) {
				if(!options.SetArcPath(codecs, cmdArcPath2))
					return E_NOTIMPL;
			}
		}
		else {
			if(fi.IsDir())
				throw "there is no such archive";
			if(fi.IsDevice)
				return E_NOTIMPL;
			if(options.VolumesSizes.Size() > 0) {
				errorInfo.FileNames.Add(us2fs(arcPath));
				errorInfo.SystemError = (DWORD)E_NOTIMPL;
				errorInfo.Message = kUpdateIsNotSupoorted_MultiVol;
				return E_NOTIMPL;
			}
			CObjectVector<COpenType> types2;
			// change it.
			if(options.MethodMode.Type_Defined)
				types2.Add(options.MethodMode.Type);
			// We need to set Properties to open archive only in some cases (WIM archives).

			CIntVector excl;
			COpenOptions op;
      #ifndef _SFX
			op.props = &options.MethodMode.Properties;
      #endif
			op.codecs = codecs;
			op.types = &types2;
			op.excludedFormats = &excl;
			op.stdInMode = false;
			op.stream = NULL;
			op.filePath = arcPath;
			RINOK(callback->StartOpenArchive(arcPath));
			HRESULT result = arcLink.Open_Strict(op, openCallback);
			if(result == E_ABORT)
				return result;
			HRESULT res2 = callback->OpenResult(codecs, arcLink, arcPath, result);
			/*
			   if(result == S_FALSE)
			   return E_FAIL;
			 */
			RINOK(res2);
			RINOK(result);
			if(arcLink.VolumePaths.Size() > 1) {
				errorInfo.SystemError = (DWORD)E_NOTIMPL;
				errorInfo.Message = kUpdateIsNotSupoorted_MultiVol;
				return E_NOTIMPL;
			}
			CArc &arc = arcLink.Arcs.Back();
			arc.MTimeDefined = !fi.IsDevice;
			arc.MTime = fi.MTime;
			if(arc.ErrorInfo.ThereIsTail) {
				errorInfo.SystemError = (DWORD)E_NOTIMPL;
				errorInfo.Message = "There is some data block after the end of the archive";
				return E_NOTIMPL;
			}
			if(options.MethodMode.Type.FormatIndex < 0) {
				options.MethodMode.Type.FormatIndex = arcLink.GetArc()->FormatIndex;
				if(!options.SetArcPath(codecs, cmdArcPath2))
					return E_NOTIMPL;
			}
		}
	}
	if(options.MethodMode.Type.FormatIndex < 0) {
		options.MethodMode.Type.FormatIndex = codecs->FindFormatForArchiveType((UString)kDefaultArcType);
		if(options.MethodMode.Type.FormatIndex < 0)
			return E_NOTIMPL;
	}
	bool thereIsInArchive = arcLink.IsOpen;
	if(!thereIsInArchive && renameMode)
		return E_FAIL;
	CDirItems dirItems;
	dirItems.Callback = callback;
	CDirItem parentDirItem;
	CDirItem * parentDirItem_Ptr = NULL;
	/*
	   FStringVector requestedPaths;
	   FStringVector *requestedPaths_Ptr = NULL;
	   if(options.DeleteAfterCompressing)
	   requestedPaths_Ptr = &requestedPaths;
	 */
	if(options.StdInMode) {
		CDirItem di;
		di.Name = options.StdInFileName;
		di.Size = (uint64)(int64)-1;
		di.Attrib = 0;
		NTime::GetCurUtcFileTime(di.MTime);
		di.CTime = di.ATime = di.MTime;
		dirItems.Items.Add(di);
	}
	else {
		bool needScanning = false;
		if(!renameMode) {
			FOR_VECTOR(i, options.Commands) {
				if(options.Commands[i].ActionSet.NeedScanning())
					needScanning = true;
			}
		}
		if(needScanning) {
			RINOK(callback->StartScanning());
			dirItems.SymLinks = options.SymLinks.Val;
      #if defined(_WIN32) && !defined(UNDER_CE)
			dirItems.ReadSecure = options.NtSecurity.Val;
      #endif
			dirItems.ScanAltStreams = options.AltStreams.Val;
			HRESULT res = EnumerateItems(censor, options.PathMode, options.AddPathPrefix, dirItems);
			if(res != S_OK) {
				if(res != E_ABORT)
					errorInfo.Message = "Scanning error";
				return res;
			}
			RINOK(callback->FinishScanning(dirItems.Stat));
			if(censor.Pairs.Size() == 1) {
				NFind::CFileInfo fi;
				FString prefix = us2fs(censor.Pairs[0].Prefix);
				prefix += '.';
				// UString prefix = censor.Pairs[0].Prefix;
				/*
				   if(prefix.Back() == WCHAR_PATH_SEPARATOR) {
				   prefix.DeleteBack();
				   }
				 */
				if(fi.Find(prefix))
					if(fi.IsDir()) {
						parentDirItem.Size = fi.Size;
						parentDirItem.CTime = fi.CTime;
						parentDirItem.ATime = fi.ATime;
						parentDirItem.MTime = fi.MTime;
						parentDirItem.Attrib = fi.Attrib;
						parentDirItem_Ptr = &parentDirItem;
						int secureIndex = -1;
	    #if defined(_WIN32) && !defined(UNDER_CE)
						if(options.NtSecurity.Val)
							dirItems.AddSecurityItem(prefix, secureIndex);
	    #endif
						parentDirItem.SecureIndex = secureIndex;
						parentDirItem_Ptr = &parentDirItem;
					}
			}
		}
	}
	FString tempDirPrefix;
	bool usesTempDir = false;
  #ifdef _WIN32
	CTempDir tempDirectory;
	if(options.EMailMode && options.EMailRemoveAfter) {
		tempDirectory.Create(kTempFolderPrefix);
		tempDirPrefix = tempDirectory.GetPath();
		NormalizeDirPathPrefix(tempDirPrefix);
		usesTempDir = true;
	}
  #endif
	CTempFiles tempFiles;
	bool createTempFile = false;
	if(!options.StdOutMode && options.UpdateArchiveItself) {
		CArchivePath &ap = options.Commands[0].ArchivePath;
		ap = options.ArchivePath;
		// if((archive != 0 && !usesTempDir) || !options.WorkingDir.IsEmpty())
		if((thereIsInArchive || !options.WorkingDir.IsEmpty()) && !usesTempDir && options.VolumesSizes.Size() == 0) {
			createTempFile = true;
			ap.Temp = true;
			if(!options.WorkingDir.IsEmpty())
				ap.TempPrefix = options.WorkingDir;
			else
				ap.TempPrefix = us2fs(ap.Prefix);
			NormalizeDirPathPrefix(ap.TempPrefix);
		}
	}
	uint   ci;
	for(ci = 0; ci < options.Commands.Size(); ci++) {
		CArchivePath &ap = options.Commands[ci].ArchivePath;
		if(usesTempDir) {
			// Check it
			ap.Prefix = fs2us(tempDirPrefix);
			// ap.Temp = true;
			// ap.TempPrefix = tempDirPrefix;
		}
		if(!options.StdOutMode && (ci > 0 || !createTempFile)) {
			const FString path = us2fs(ap.GetFinalPath());
			if(NFind::DoesFileOrDirExist(path)) {
				errorInfo.SystemError = ERROR_FILE_EXISTS;
				errorInfo.Message = "The file already exists";
				errorInfo.FileNames.Add(path);
				return errorInfo.Get_HRESULT_Error();
			}
		}
	}
	CObjectVector<CArcItem> arcItems;
	if(thereIsInArchive) {
		RINOK(EnumerateInArchiveItems(/*options.StoreAltStreams,*/censor, arcLink.Arcs.Back(), arcItems));
	}
	/*
	   FStringVector processedFilePaths;
	   FStringVector *processedFilePaths_Ptr = NULL;
	   if(options.DeleteAfterCompressing)
	   processedFilePaths_Ptr = &processedFilePaths;
	 */
	CByteBuffer processedItems;
	if(options.DeleteAfterCompressing) {
		uint   num = dirItems.Items.Size();
		processedItems.Alloc(num);
		for(uint i = 0; i < num; i++)
			processedItems[i] = 0;
	}
	/*
	   #ifndef _NO_CRYPTO
	   if(arcLink.PasswordWasAsked) {
	   // We set password, if open have requested password
	   RINOK(callback->SetPassword(arcLink.Password));
	   }
	   #endif
	 */
	for(ci = 0; ci < options.Commands.Size(); ci++) {
		const CArc * arc = thereIsInArchive ? arcLink.GetArc() : NULL;
		CUpdateArchiveCommand &command = options.Commands[ci];
		UString name;
		bool isUpdating;
		if(options.StdOutMode) {
			name = "stdout";
			isUpdating = thereIsInArchive;
		}
		else {
			name = command.ArchivePath.GetFinalPath();
			isUpdating = (ci == 0 && options.UpdateArchiveItself && thereIsInArchive);
		}
		RINOK(callback->StartArchive(name, isUpdating))
		CFinishArchiveStat st;
		RINOK(Compress(options, isUpdating, codecs, command.ActionSet, arc, command.ArchivePath, arcItems,
			options.DeleteAfterCompressing ? (Byte*)processedItems : NULL, dirItems, parentDirItem_Ptr, tempFiles, errorInfo, callback, st));
		RINOK(callback->FinishArchive(st));
	}
	if(thereIsInArchive) {
		RINOK(arcLink.Close());
		arcLink.Release();
	}
	tempFiles.Paths.Clear();
	if(createTempFile) {
		try {
			CArchivePath &ap = options.Commands[0].ArchivePath;
			const FString &tempPath = ap.GetTempPath();
			if(thereIsInArchive)
				if(!DeleteFileAlways(us2fs(arcPath)))
					return errorInfo.SetFromLastError("cannot delete the file", us2fs(arcPath));
			if(!MyMoveFile(tempPath, us2fs(arcPath))) {
				errorInfo.SetFromLastError("cannot move the file", tempPath);
				errorInfo.FileNames.Add(us2fs(arcPath));
				return errorInfo.Get_HRESULT_Error();
			}
		}
		catch(...)
		{
			throw;
		}
	}
  #if defined(_WIN32) && !defined(UNDER_CE)
	if(options.EMailMode) {
		NDLL::CLibrary mapiLib;
		if(!mapiLib.Load(FTEXT("Mapi32.dll"))) {
			errorInfo.SetFromLastError("cannot load Mapi32.dll");
			return errorInfo.Get_HRESULT_Error();
		}
		/*
		   LPMAPISENDDOCUMENTS fnSend = (LPMAPISENDDOCUMENTS)mapiLib.GetProc("MAPISendDocuments");
		   if(fnSend == 0)
		   {
		   errorInfo.SetFromLastError)("7-Zip cannot find MAPISendDocuments function");
		   return errorInfo.Get_HRESULT_Error();
		   }
		 */
		LPMAPISENDMAIL sendMail = (LPMAPISENDMAIL)mapiLib.GetProc("MAPISendMail");
		if(sendMail == 0) {
			errorInfo.SetFromLastError("7-Zip cannot find MAPISendMail function");
			return errorInfo.Get_HRESULT_Error();;
		}
		FStringVector fullPaths;
		uint i;
		for(i = 0; i < options.Commands.Size(); i++) {
			CArchivePath &ap = options.Commands[i].ArchivePath;
			FString finalPath = us2fs(ap.GetFinalPath());
			FString arcPath2;
			if(!MyGetFullPathName(finalPath, arcPath2))
				return errorInfo.SetFromLastError("GetFullPathName error", finalPath);
			fullPaths.Add(arcPath2);
		}
		CCurrentDirRestorer curDirRestorer;
		for(i = 0; i < fullPaths.Size(); i++) {
			const UString arcPath2 = fs2us(fullPaths[i]);
			const UString fileName = ExtractFileNameFromPath(arcPath2);
			const AString path(GetAnsiString(arcPath2));
			const AString name(GetAnsiString(fileName));
			// Warning!!! MAPISendDocuments function changes Current directory
			// fnSend(0, ";", (LPSTR)(LPCSTR)path, (LPSTR)(LPCSTR)name, 0);
			MapiFileDesc f;
			memzero(&f, sizeof(f));
			f.nPosition = 0xFFFFFFFF;
			f.lpszPathName = (char *)(const char *)path;
			f.lpszFileName = (char *)(const char *)name;
			MapiMessage m;
			memzero(&m, sizeof(m));
			m.nFileCount = 1;
			m.lpFiles = &f;
			const AString addr(GetAnsiString(options.EMailAddress));
			MapiRecipDesc rec;
			if(!addr.IsEmpty()) {
				memzero(&rec, sizeof(rec));
				rec.ulRecipClass = MAPI_TO;
				rec.lpszAddress = (char *)(const char *)addr;
				m.nRecipCount = 1;
				m.lpRecips = &rec;
			}
			sendMail((LHANDLE)0, 0, &m, MAPI_DIALOG, 0);
		}
	}
  #endif
	if(options.DeleteAfterCompressing) {
		CRecordVector<CRefSortPair> pairs;
		FStringVector foldersNames;
		uint i;
		for(i = 0; i < dirItems.Items.Size(); i++) {
			const CDirItem &dirItem = dirItems.Items[i];
			FString phyPath = dirItems.GetPhyPath(i);
			if(dirItem.IsDir()) {
				CRefSortPair pair;
				pair.Index = i;
				pair.Len = GetNumSlashes(phyPath);
				pairs.Add(pair);
			}
			else {
				if(processedItems[i] != 0 || dirItem.Size == 0) {
					RINOK(callback->DeletingAfterArchiving(phyPath, false));
					DeleteFileAlways(phyPath);
				}
				else {
					// file was skipped
					/*
					   errorInfo.SystemError = 0;
					   errorInfo.Message = "file was not processed";
					   errorInfo.FileName = phyPath;
					   return E_FAIL;
					 */
				}
			}
		}
		pairs.Sort(CompareRefSortPair, NULL);
		for(i = 0; i < pairs.Size(); i++) {
			FString phyPath = dirItems.GetPhyPath(pairs[i].Index);
			if(NFind::DoesDirExist(phyPath)) {
				RINOK(callback->DeletingAfterArchiving(phyPath, true));
				RemoveDir(phyPath);
			}
		}
		RINOK(callback->FinishDeletingAfterArchiving());
	}
	return S_OK;
}
