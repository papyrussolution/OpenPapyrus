// 7Z-CHM.CPP
//
#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

using namespace NWindows;
using namespace NTime;

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

// ChmHandler.cpp ChmIn.cpp
namespace NArchive {
	namespace NChm {
		// #define _CHM_DETAILS
		#ifdef _CHM_DETAILS
			enum {
				kpidSection = kpidUserDefined
			};
		#endif

		static const Byte kProps[] = {
			kpidPath,
			kpidSize,
			kpidMethod,
			kpidBlock
		  #ifdef _CHM_DETAILS
			,
			L"Section", kpidSection,
			kpidOffset
		  #endif
		};

		/*
		   static const Byte kArcProps[] =
		   {
		   // kpidNumBlocks,
		   };
		 */

		IMP_IInArchive_Props
		IMP_IInArchive_ArcProps_NO_Table

		STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT * value)
		{
			// COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			switch(propID) {
				/*
				   case kpidNumBlocks:
				   {
				   uint64 numBlocks = 0;
				   FOR_VECTOR(i, m_Database.Sections)
				   {
					const CSectionInfo &s = m_Database.Sections[i];
					FOR_VECTOR(j, s.Methods)
					{
					  const CMethodInfo &m = s.Methods[j];
					  if(m.IsLzx())
						numBlocks += m.LzxInfo.ResetTable.GetNumBlocks();
					}
				   }
				   prop = numBlocks;
				   break;
				   }
				 */
				case kpidOffset: prop = m_Database.StartPosition; break;
				case kpidPhySize: prop = m_Database.PhySize; break;
				case kpidErrorFlags: prop = m_ErrorFlags; break;
			}
			prop.Detach(value);
			return S_OK;
			// COM_TRY_END
		}

		STDMETHODIMP CHandler::GetProperty(uint32 index, PROPID propID, PROPVARIANT * value)
		{
			COM_TRY_BEGIN
			NCOM::CPropVariant prop;
			if(m_Database.NewFormat) {
				switch(propID) {
					case kpidSize:
						prop = (uint64)m_Database.NewFormatString.Len();
						break;
				}
				prop.Detach(value);
				return S_OK;
			}
			unsigned entryIndex = m_Database.LowLevel ? index : m_Database.Indices[index];
			const CItem &item = m_Database.Items[entryIndex];
			switch(propID) {
				case kpidPath:
				{
					UString us;
					// if(
					ConvertUTF8ToUnicode(item.Name, us);
					{
						if(!m_Database.LowLevel) {
							if(us.Len() > 1 && us[0] == L'/')
								us.Delete(0);
						}
						NItemName::ReplaceToOsSlashes_Remove_TailSlash(us);
						prop = us;
					}
					break;
				}
				case kpidIsDir:  prop = item.IsDir(); break;
				case kpidSize:  prop = item.Size; break;
				case kpidMethod:
				{
					if(!item.IsDir())
						if(item.Section == 0)
							prop = "Copy";
						else if(item.Section < m_Database.Sections.Size())
							prop = m_Database.Sections[(uint)item.Section].GetMethodName();
					break;
				}
				case kpidBlock:
					if(m_Database.LowLevel)
						prop = item.Section;
					else if(item.Section != 0 && item.Section < m_Database.Sections.Size())
						prop = m_Database.GetFolder(index);
					break;

			#ifdef _CHM_DETAILS

				case kpidSection:  prop = (uint32)item.Section; break;
				case kpidOffset:  prop = (uint32)item.Offset; break;

			#endif
			}

			prop.Detach(value);
			return S_OK;
			COM_TRY_END
		}

		/*
		   class CProgressImp: public CProgressVirt
		   {
		   CMyComPtr<IArchiveOpenCallback> _callback;
		   public:
		   STDMETHOD(SetTotal)(const uint64 *numFiles);
		   STDMETHOD(SetCompleted)(const uint64 *numFiles);
		   CProgressImp(IArchiveOpenCallback *callback): _callback(callback) {};
		   };

		   STDMETHODIMP CProgressImp::SetTotal(const uint64 *numFiles)
		   {
		   if(_callback)
			return _callback->SetCompleted(numFiles, NULL);
		   return S_OK;
		   }

		   STDMETHODIMP CProgressImp::SetCompleted(const uint64 *numFiles)
		   {
		   if(_callback)
			return _callback->SetCompleted(numFiles, NULL);
		   return S_OK;
		   }
		 */

		STDMETHODIMP CHandler::Open(IInStream * inStream, const uint64 * maxCheckStartPosition, IArchiveOpenCallback * /* openArchiveCallback */)
		{
			COM_TRY_BEGIN
			Close();
			try {
				CInArchive archive(_help2);
				// CProgressImp progressImp(openArchiveCallback);
				HRESULT res = archive.Open(inStream, maxCheckStartPosition, m_Database);
				if(!archive.IsArc) m_ErrorFlags |= kpv_ErrorFlags_IsNotArc;
				if(archive.HeadersError) m_ErrorFlags |= kpv_ErrorFlags_HeadersError;
				if(archive.UnexpectedEnd) m_ErrorFlags |= kpv_ErrorFlags_UnexpectedEnd;
				if(archive.UnsupportedFeature) m_ErrorFlags |= kpv_ErrorFlags_UnsupportedFeature;
				RINOK(res);
				/*
				   if(m_Database.LowLevel)
				   return S_FALSE;
				 */
				m_Stream = inStream;
			}
			catch(...) {
				return S_FALSE;
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::Close()
		{
			m_ErrorFlags = 0;
			m_Database.Clear();
			m_Stream.Release();
			return S_OK;
		}

		class CChmFolderOutStream : public ISequentialOutStream, public CMyUnknownImp {
		public:
			MY_UNKNOWN_IMP

			HRESULT Write2(const void * data, uint32 size, uint32 * processedSize, bool isOK);
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);

			uint64 m_FolderSize;
			uint64 m_PosInFolder;
			uint64 m_PosInSection;
			const CRecordVector<bool> * m_ExtractStatuses;
			unsigned m_StartIndex;
			unsigned m_CurrentIndex;
			unsigned m_NumFiles;
		private:
			const CFilesDatabase * m_Database;
			CMyComPtr<IArchiveExtractCallback> m_ExtractCallback;
			bool m_TestMode;
			bool m_IsOk;
			bool m_FileIsOpen;
			uint64 m_RemainFileSize;
			CMyComPtr<ISequentialOutStream> m_RealOutStream;

			HRESULT OpenFile();
			HRESULT WriteEmptyFiles();
		public:
			void Init(const CFilesDatabase * database, IArchiveExtractCallback * extractCallback, bool testMode);
			HRESULT FlushCorrupted(uint64 maxSize);
		};

		void CChmFolderOutStream::Init(const CFilesDatabase * database, IArchiveExtractCallback * extractCallback, bool testMode)
		{
			m_Database = database;
			m_ExtractCallback = extractCallback;
			m_TestMode = testMode;
			m_CurrentIndex = 0;
			m_FileIsOpen = false;
		}

		HRESULT CChmFolderOutStream::OpenFile()
		{
			int32 askMode = (*m_ExtractStatuses)[m_CurrentIndex] ? (m_TestMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract) : NExtractArc::NAskMode::kSkip;
			m_RealOutStream.Release();
			RINOK(m_ExtractCallback->GetStream(m_StartIndex + m_CurrentIndex, &m_RealOutStream, askMode));
			if(!m_RealOutStream && !m_TestMode)
				askMode = NExtractArc::NAskMode::kSkip;
			return m_ExtractCallback->PrepareOperation(askMode);
		}

		HRESULT CChmFolderOutStream::WriteEmptyFiles()
		{
			if(!m_FileIsOpen) {
				for(; m_CurrentIndex < m_NumFiles; m_CurrentIndex++) {
					uint64 fileSize = m_Database->GetFileSize(m_StartIndex + m_CurrentIndex);
					if(fileSize != 0)
						return S_OK;
					HRESULT result = OpenFile();
					m_RealOutStream.Release();
					RINOK(result);
					RINOK(m_ExtractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
				}
			}
			return S_OK;
		}

		// This is WritePart function
		HRESULT CChmFolderOutStream::Write2(const void * data, uint32 size, uint32 * processedSize, bool isOK)
		{
			uint32 realProcessed = 0;
			ASSIGN_PTR(processedSize, 0);
			while(size != 0) {
				if(m_FileIsOpen) {
					uint32 numBytesToWrite = (uint32)MyMin(m_RemainFileSize, (uint64)(size));
					HRESULT res = S_OK;
					if(numBytesToWrite > 0) {
						if(!isOK)
							m_IsOk = false;
						if(m_RealOutStream) {
							uint32 processedSizeLocal = 0;
							res = m_RealOutStream->Write((const Byte *)data, numBytesToWrite, &processedSizeLocal);
							numBytesToWrite = processedSizeLocal;
						}
					}
					realProcessed += numBytesToWrite;
					ASSIGN_PTR(processedSize, realProcessed);
					data = (const void *)((const Byte *)data + numBytesToWrite);
					size -= numBytesToWrite;
					m_RemainFileSize -= numBytesToWrite;
					m_PosInSection += numBytesToWrite;
					m_PosInFolder += numBytesToWrite;
					if(res != S_OK)
						return res;
					if(m_RemainFileSize == 0) {
						m_RealOutStream.Release();
						RINOK(m_ExtractCallback->SetOperationResult(m_IsOk ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kDataError));
						m_FileIsOpen = false;
					}
					if(realProcessed > 0)
						break;  // with this break this function works as write part
				}
				else {
					if(m_CurrentIndex >= m_NumFiles) {
						realProcessed += size;
						ASSIGN_PTR(processedSize, realProcessed);
						return S_OK;
						// return E_FAIL;
					}
					unsigned fullIndex = m_StartIndex + m_CurrentIndex;
					m_RemainFileSize = m_Database->GetFileSize(fullIndex);
					uint64 fileOffset = m_Database->GetFileOffset(fullIndex);
					if(fileOffset < m_PosInSection)
						return E_FAIL;
					if(fileOffset > m_PosInSection) {
						uint32 numBytesToWrite = (uint32)MyMin(fileOffset - m_PosInSection, uint64(size));
						realProcessed += numBytesToWrite;
						ASSIGN_PTR(processedSize, realProcessed);
						data = (const void *)((const Byte *)data + numBytesToWrite);
						size -= numBytesToWrite;
						m_PosInSection += numBytesToWrite;
						m_PosInFolder += numBytesToWrite;
					}
					if(fileOffset == m_PosInSection) {
						RINOK(OpenFile());
						m_FileIsOpen = true;
						m_CurrentIndex++;
						m_IsOk = true;
					}
				}
			}
			return WriteEmptyFiles();
		}

		STDMETHODIMP CChmFolderOutStream::Write(const void * data, uint32 size, uint32 * processedSize)
		{
			return Write2(data, size, processedSize, true);
		}

		HRESULT CChmFolderOutStream::FlushCorrupted(uint64 maxSize)
		{
			const uint32 kBufferSize = (1 << 10);
			Byte buffer[kBufferSize];
			for(uint i = 0; i < kBufferSize; i++)
				buffer[i] = 0;
			if(maxSize > m_FolderSize)
				maxSize = m_FolderSize;
			while(m_PosInFolder < maxSize) {
				uint32 size = (uint32)MyMin(maxSize - m_PosInFolder, (uint64)kBufferSize);
				uint32 processedSizeLocal = 0;
				RINOK(Write2(buffer, size, &processedSizeLocal, false));
				if(processedSizeLocal == 0)
					return S_OK;
			}
			return S_OK;
		}

		uint   CLzxInfo::GetNumDictBits() const { return (Version == 2 || Version == 3) ? (15 + WindowSizeBits) : 0; }
		uint64 CLzxInfo::GetFolderSize() const { return kBlockSize << ResetIntervalBits; }
		uint64 CLzxInfo::GetFolder(uint64 offset) const { return offset / GetFolderSize(); }
		uint64 CLzxInfo::GetFolderPos(uint64 folderIndex) const { return folderIndex * GetFolderSize(); }
		uint64 CLzxInfo::GetBlockIndexFromFolderIndex(uint64 folderIndex) const { return folderIndex << ResetIntervalBits; }

		bool CLzxInfo::GetOffsetOfFolder(uint64 folderIndex, uint64 &offset) const
		{
			uint64 blockIndex = GetBlockIndexFromFolderIndex(folderIndex);
			if(blockIndex >= ResetTable.ResetOffsets.Size())
				return false;
			offset = ResetTable.ResetOffsets[(uint)blockIndex];
			return true;
		}
		bool CLzxInfo::GetCompressedSizeOfFolder(uint64 folderIndex, uint64 &size) const
		{
			uint64 blockIndex = GetBlockIndexFromFolderIndex(folderIndex);
			return ResetTable.GetCompressedSizeOfBlocks(blockIndex, (uint32)1 << ResetIntervalBits, size);
		}
		STDMETHODIMP CHandler::Extract(const uint32 * indices, uint32 numItems, int32 testModeSpec, IArchiveExtractCallback * extractCallback)
		{
			COM_TRY_BEGIN
			bool allFilesMode = (numItems == (uint32)(int32)-1);
			if(allFilesMode)
				numItems = m_Database.NewFormat ? 1 : (m_Database.LowLevel ? m_Database.Items.Size() : m_Database.Indices.Size());
			if(numItems == 0)
				return S_OK;
			bool testMode = (testModeSpec != 0);
			uint64 currentTotalSize = 0;
			NCompress::CCopyCoder * copyCoderSpec = new NCompress::CCopyCoder;
			CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;
			uint32 i;
			CLocalProgress * lps = new CLocalProgress;
			CMyComPtr<ICompressProgressInfo> progress = lps;
			lps->Init(extractCallback, false);
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> inStream(streamSpec);
			streamSpec->SetStream(m_Stream);
			if(m_Database.LowLevel) {
				uint64 currentItemSize = 0;
				uint64 totalSize = 0;
				if(m_Database.NewFormat)
					totalSize = m_Database.NewFormatString.Len();
				else
					for(i = 0; i < numItems; i++)
						totalSize += m_Database.Items[allFilesMode ? i : indices[i]].Size;
				extractCallback->SetTotal(totalSize);
				for(i = 0; i < numItems; i++, currentTotalSize += currentItemSize) {
					currentItemSize = 0;
					lps->InSize = currentTotalSize; // Change it
					lps->OutSize = currentTotalSize;
					RINOK(lps->SetCur());
					CMyComPtr<ISequentialOutStream> realOutStream;
					int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
					int32 index = allFilesMode ? i : indices[i];
					RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
					if(m_Database.NewFormat) {
						if(index != 0)
							return E_FAIL;
						if(testMode || realOutStream) {
							if(!testMode) {
								uint32 size = m_Database.NewFormatString.Len();
								RINOK(WriteStream(realOutStream, (const char *)m_Database.NewFormatString, size));
							}
							RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
						}
						continue;
					}
					else {
						const CItem & item = m_Database.Items[index];
						currentItemSize = item.Size;
						if(testMode || realOutStream) {
							RINOK(extractCallback->PrepareOperation(askMode));
							if(item.Section != 0) {
								RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
								continue;
							}
							else if(testMode) {
								RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
								continue;
							}
							else {
								RINOK(m_Stream->Seek(m_Database.ContentOffset + item.Offset, STREAM_SEEK_SET, NULL));
								streamSpec->Init(item.Size);
								RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
								realOutStream.Release();
								RINOK(extractCallback->SetOperationResult((copyCoderSpec->TotalSize == item.Size) ? NExtractArc::NOperationResult::kOK : NExtractArc::NOperationResult::kDataError));
							}
						}
					}
				}
				return S_OK;
			}
			uint64 lastFolderIndex = ((uint64)0 - 1);
			for(i = 0; i < numItems; i++) {
				uint32 index = allFilesMode ? i : indices[i];
				const CItem &item = m_Database.Items[m_Database.Indices[index]];
				const uint64 sectionIndex = item.Section;
				if(!item.IsDir() && item.Size) {
					if(sectionIndex == 0) {
						currentTotalSize += item.Size;
					}
					else if(sectionIndex < m_Database.Sections.Size()) {
						const CSectionInfo &section = m_Database.Sections[(uint)sectionIndex];
						if(section.IsLzx()) {
							const CLzxInfo &lzxInfo = section.Methods[0].LzxInfo;
							uint64 folderIndex = m_Database.GetFolder(index);
							if(lastFolderIndex == folderIndex)
								folderIndex++;
							lastFolderIndex = m_Database.GetLastFolder(index);
							for(; folderIndex <= lastFolderIndex; folderIndex++)
								currentTotalSize += lzxInfo.GetFolderSize();
						}
					}
				}
			}
			RINOK(extractCallback->SetTotal(currentTotalSize));
			NCompress::NLzx::CDecoder * lzxDecoderSpec = NULL;
			CMyComPtr<IUnknown> lzxDecoder;
			CChmFolderOutStream * chmFolderOutStream = 0;
			CMyComPtr<ISequentialOutStream> outStream;
			currentTotalSize = 0;
			CRecordVector <bool> extractStatuses;
			CByteBuffer packBuf;
			for(i = 0;;) {
				RINOK(extractCallback->SetCompleted(&currentTotalSize));
				if(i >= numItems)
					break;
				uint32 index = allFilesMode ? i : indices[i];
				i++;
				const CItem &item = m_Database.Items[m_Database.Indices[index]];
				const uint64 sectionIndex = item.Section;
				int32 askMode = testMode ? NExtractArc::NAskMode::kTest : NExtractArc::NAskMode::kExtract;
				if(item.IsDir()) {
					CMyComPtr<ISequentialOutStream> realOutStream;
					RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
					RINOK(extractCallback->PrepareOperation(askMode));
					realOutStream.Release();
					RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kOK));
					continue;
				}
				else {
					lps->InSize = currentTotalSize; // Change it
					lps->OutSize = currentTotalSize;
					if(item.Size == 0 || sectionIndex == 0) {
						CMyComPtr <ISequentialOutStream> realOutStream;
						RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
						if(testMode || realOutStream) {
							RINOK(extractCallback->PrepareOperation(askMode));
							int32 opRes = NExtractArc::NOperationResult::kOK;
							if(!testMode && item.Size != 0) {
								RINOK(m_Stream->Seek(m_Database.ContentOffset + item.Offset, STREAM_SEEK_SET, NULL));
								streamSpec->Init(item.Size);
								RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
								if(copyCoderSpec->TotalSize != item.Size)
									opRes = NExtractArc::NOperationResult::kDataError;
							}
							realOutStream.Release();
							RINOK(extractCallback->SetOperationResult(opRes));
							currentTotalSize += item.Size;
						}
						continue;
					}
					else if(sectionIndex >= m_Database.Sections.Size()) {
						// we must report error here;
						CMyComPtr<ISequentialOutStream> realOutStream;
						RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
						if(testMode || realOutStream) {
							RINOK(extractCallback->PrepareOperation(askMode));
							RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kHeadersError));
						}
						continue;
					}
					else {
						const CSectionInfo & section = m_Database.Sections[(uint)sectionIndex];
						if(!section.IsLzx()) {
							CMyComPtr<ISequentialOutStream> realOutStream;
							RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
							if(testMode || realOutStream) {
								RINOK(extractCallback->PrepareOperation(askMode));
								RINOK(extractCallback->SetOperationResult(NExtractArc::NOperationResult::kUnsupportedMethod));
							}
							continue;
						}
						else {
							const CLzxInfo &lzxInfo = section.Methods[0].LzxInfo;
							if(!chmFolderOutStream) {
								chmFolderOutStream = new CChmFolderOutStream;
								outStream = chmFolderOutStream;
							}
							chmFolderOutStream->Init(&m_Database, extractCallback, testMode);
							if(!lzxDecoderSpec) {
								lzxDecoderSpec = new NCompress::NLzx::CDecoder;
								lzxDecoder = lzxDecoderSpec;
							}
							uint64 folderIndex = m_Database.GetFolder(index);
							const uint64 compressedPos = m_Database.ContentOffset + section.Offset;
							RINOK(lzxDecoderSpec->SetParams_and_Alloc(lzxInfo.GetNumDictBits()));
							const CItem * lastItem = &item;
							extractStatuses.Clear();
							extractStatuses.Add(true);
							for(;; folderIndex++) {
								RINOK(extractCallback->SetCompleted(&currentTotalSize));
								uint64 startPos = lzxInfo.GetFolderPos(folderIndex);
								uint64 finishPos = lastItem->Offset + lastItem->Size;
								uint64 limitFolderIndex = lzxInfo.GetFolder(finishPos);
								lastFolderIndex = m_Database.GetLastFolder(index);
								uint64 folderSize = lzxInfo.GetFolderSize();
								uint64 unPackSize = folderSize;
								chmFolderOutStream->m_StartIndex = extractStatuses.IsEmpty() ? (index + 1) : index;
								if(limitFolderIndex == folderIndex) {
									for(; i < numItems; i++) {
										const uint32 nextIndex = allFilesMode ? i : indices[i];
										const CItem &nextItem = m_Database.Items[m_Database.Indices[nextIndex]];
										if(nextItem.Section != sectionIndex)
											break;
										uint64 nextFolderIndex = m_Database.GetFolder(nextIndex);
										if(nextFolderIndex != folderIndex)
											break;
										for(index++; index < nextIndex; index++)
											extractStatuses.Add(false);
										extractStatuses.Add(true);
										index = nextIndex;
										lastItem = &nextItem;
										if(nextItem.Size != 0)
											finishPos = nextItem.Offset + nextItem.Size;
										lastFolderIndex = m_Database.GetLastFolder(index);
									}
								}
								unPackSize = MyMin(finishPos - startPos, unPackSize);
								chmFolderOutStream->m_FolderSize = folderSize;
								chmFolderOutStream->m_PosInFolder = 0;
								chmFolderOutStream->m_PosInSection = startPos;
								chmFolderOutStream->m_ExtractStatuses = &extractStatuses;
								chmFolderOutStream->m_NumFiles = extractStatuses.Size();
								chmFolderOutStream->m_CurrentIndex = 0;
								try {
									uint64 startBlock = lzxInfo.GetBlockIndexFromFolderIndex(folderIndex);
									const CResetTable &rt = lzxInfo.ResetTable;
									uint32 numBlocks = (uint32)rt.GetNumBlocks(unPackSize);
									for(uint32 b = 0; b < numBlocks; b++) {
										uint64 completedSize = currentTotalSize + chmFolderOutStream->m_PosInSection - startPos;
										RINOK(extractCallback->SetCompleted(&completedSize));
										uint64 bCur = startBlock + b;
										if(bCur >= rt.ResetOffsets.Size())
											return E_FAIL;
										uint64 offset = rt.ResetOffsets[(uint)bCur];
										uint64 compressedSize;
										rt.GetCompressedSizeOfBlock(bCur, compressedSize);
										// chm writes full blocks. So we don't need to use reduced size for last block
										RINOK(m_Stream->Seek(compressedPos + offset, STREAM_SEEK_SET, NULL));
										streamSpec->SetStream(m_Stream);
										streamSpec->Init(compressedSize);
										lzxDecoderSpec->SetKeepHistory(b > 0);
										size_t compressedSizeT = (size_t)compressedSize;
										if(compressedSizeT != compressedSize)
											throw 2;
										packBuf.AllocAtLeast(compressedSizeT);
										HRESULT res = ReadStream_FALSE(inStream, packBuf, compressedSizeT);
										if(res == S_OK) {
											lzxDecoderSpec->KeepHistoryForNext = true;
											res = lzxDecoderSpec->Code(packBuf, compressedSizeT, kBlockSize); // rt.BlockSize;
											if(res == S_OK)
												res = WriteStream(chmFolderOutStream, lzxDecoderSpec->GetUnpackData(), lzxDecoderSpec->GetUnpackSize());
										}
										if(res != S_OK) {
											if(res != S_FALSE)
												return res;
											throw 1;
										}
									}
								}
								catch(...) {
									RINOK(chmFolderOutStream->FlushCorrupted(unPackSize));
								}
								currentTotalSize += folderSize;
								if(folderIndex == lastFolderIndex)
									break;
								extractStatuses.Clear();
							}
						}
					}
				}
			}
			return S_OK;
			COM_TRY_END
		}

		STDMETHODIMP CHandler::GetNumberOfItems(uint32 * numItems)
		{
			*numItems = m_Database.NewFormat ? 1 : (m_Database.LowLevel ? m_Database.Items.Size() : m_Database.Indices.Size());
			return S_OK;
		}
		namespace NChm {
			static const Byte k_Signature[] = { 'I', 'T', 'S', 'F', 3, 0, 0, 0, 0x60, 0,  0, 0 };
			REGISTER_ARC_I_CLS(CHandler(false), "Chm", "chm chi chq chw", 0, 0xE9, k_Signature, 0, 0, NULL)
		}
		namespace NHxs {
			static const Byte k_Signature[] = { 'I', 'T', 'O', 'L', 'I', 'T', 'L', 'S', 1, 0, 0, 0, 0x28, 0, 0, 0 };
			REGISTER_ARC_I_CLS(CHandler(true), "Hxs", "hxs hxi hxr hxq hxw lit", 0, 0xCE, k_Signature, 0, NArcInfoFlags::kFindSignature, NULL)
		}
		//
		// ChmIn
		//
		static const uint32 kSignature_ITSP = 0x50535449;
		static const uint32 kSignature_PMGL = 0x4C474D50;
		static const uint32 kSignature_LZXC = 0x43585A4C;
		static const uint32 kSignature_IFCM = 0x4D434649;
		static const uint32 kSignature_AOLL = 0x4C4C4F41;
		static const uint32 kSignature_CAOL = 0x4C4F4143;
		static const uint32 kSignature_ITSF = 0x46535449;
		static const uint32 kSignature_ITOL = 0x4C4F5449;
		static const uint32 kSignature_ITLS = 0x534C5449;

		struct CEnexpectedEndException {};
		struct CHeaderErrorException {};

		// define CHM_LOW, if you want to see low level items
		// #define CHM_LOW

		static const Byte kChmLzxGuid[16]   = { 0x40, 0x89, 0xC2, 0x7F, 0x31, 0x9D, 0xD0, 0x11, 0x9B, 0x27, 0x00, 0xA0, 0xC9, 0x1E, 0x9C, 0x7C };
		static const Byte kHelp2LzxGuid[16] = { 0xC6, 0x07, 0x90, 0x0A, 0x76, 0x40, 0xD3, 0x11, 0x87, 0x89, 0x00, 0x00, 0xF8, 0x10, 0x57, 0x54 };
		static const Byte kDesGuid[16]      = { 0xA2, 0xE4, 0xF6, 0x67, 0xBF, 0x60, 0xD3, 0x11, 0x85, 0x40, 0x00, 0xC0, 0x4F, 0x58, 0xC3, 0xCF };

		static bool inline AreGuidsEqual(const Byte * g1, const Byte * g2)
		{
			return memcmp(g1, g2, 16) == 0;
		}

		static char GetHex(unsigned v)
		{
			return (char)((v < 10) ? ('0' + v) : ('A' + (v - 10)));
		}

		static void PrintByte(Byte b, AString &s)
		{
			s += GetHex(b >> 4);
			s += GetHex(b & 0xF);
		}

		AString CMethodInfo::GetGuidString() const
		{
			char s[48];
			RawLeGuidToString_Braced(Guid, s);
			// MyStringUpper_Ascii(s);
			return (AString)s;
		}

		bool CMethodInfo::IsLzx() const
		{
			return AreGuidsEqual(Guid, kChmLzxGuid) ? true : AreGuidsEqual(Guid, kHelp2LzxGuid);
		}

		bool CMethodInfo::IsDes() const
		{
			return AreGuidsEqual(Guid, kDesGuid);
		}

		AString CMethodInfo::GetName() const
		{
			AString s;
			if(IsLzx()) {
				s = "LZX:";
				s.Add_UInt32(LzxInfo.GetNumDictBits());
			}
			else {
				if(IsDes())
					s = "DES";
				else {
					s = GetGuidString();
					if(ControlData.Size() > 0) {
						s += ':';
						for(size_t i = 0; i < ControlData.Size(); i++)
							PrintByte(ControlData[i], s);
					}
				}
			}
			return s;
		}

		bool CSectionInfo::IsLzx() const
		{
			return (Methods.Size() == 1) ? Methods[0].IsLzx() : false;
		}

		UString CSectionInfo::GetMethodName() const
		{
			UString s;
			if(!IsLzx()) {
				UString temp;
				if(ConvertUTF8ToUnicode(Name, temp))
					s += temp;
				s += ": ";
			}
			FOR_VECTOR(i, Methods) {
				if(i != 0)
					s.Add_Space();
				s += Methods[i].GetName();
			}
			return s;
		}

		Byte CInArchive::ReadByte()
		{
			Byte b;
			if(!_inBuffer.ReadByte(b))
				throw CEnexpectedEndException();
			return b;
		}

		void CInArchive::Skip(size_t size)
		{
			if(_inBuffer.Skip(size) != size)
				throw CEnexpectedEndException();
		}

		void CInArchive::ReadBytes(Byte * data, uint32 size)
		{
			if(_inBuffer.ReadBytes(data, size) != size)
				throw CEnexpectedEndException();
		}

		uint16 CInArchive::ReadUInt16()
		{
			Byte b0, b1;
			if(!_inBuffer.ReadByte(b0)) throw CEnexpectedEndException();
			if(!_inBuffer.ReadByte(b1)) throw CEnexpectedEndException();
			return static_cast<uint16>(((uint16)b1 << 8) | b0);
		}

		uint32 CInArchive::ReadUInt32()
		{
			Byte p[4];
			ReadBytes(p, 4);
			return Get32(p);
		}

		uint64 CInArchive::ReadUInt64()
		{
			Byte p[8];
			ReadBytes(p, 8);
			return Get64(p);
		}

		uint64 CInArchive::ReadEncInt()
		{
			uint64 val = 0;
			for(int i = 0; i < 9; i++) {
				Byte b = ReadByte();
				val |= (b & 0x7F);
				if(b < 0x80)
					return val;
				val <<= 7;
			}
			throw CHeaderErrorException();
		}

		void CInArchive::ReadGUID(Byte * g)
		{
			ReadBytes(g, 16);
		}

		void CInArchive::ReadString(uint size, AString &s)
		{
			s.Empty();
			if(size != 0) {
				ReadBytes((Byte *)s.GetBuf(size), size);
				s.ReleaseBuf_CalcLen(size);
			}
		}

		void CInArchive::ReadUString(uint size, UString &s)
		{
			s.Empty();
			while(size-- != 0) {
				wchar_t c = ReadUInt16();
				if(c == 0) {
					Skip(2 * size);
					return;
				}
				s += c;
			}
		}

		HRESULT CInArchive::ReadChunk(IInStream * inStream, uint64 pos, uint64 size)
		{
			RINOK(inStream->Seek(pos, STREAM_SEEK_SET, NULL));
			CLimitedSequentialInStream * streamSpec = new CLimitedSequentialInStream;
			CMyComPtr<ISequentialInStream> limitedStream(streamSpec);
			streamSpec->SetStream(inStream);
			streamSpec->Init(size);
			m_InStreamRef = limitedStream;
			_inBuffer.SetStream(limitedStream);
			_inBuffer.Init();
			return S_OK;
		}

		HRESULT CInArchive::ReadDirEntry(CDatabase &database)
		{
			CItem item;
			uint64 nameLen = ReadEncInt();
			if(nameLen == 0 || nameLen > (1 << 13))
				return S_FALSE;
			ReadString((uint)nameLen, item.Name);
			item.Section = ReadEncInt();
			item.Offset = ReadEncInt();
			item.Size = ReadEncInt();
			database.Items.Add(item);
			return S_OK;
		}

		HRESULT CInArchive::OpenChm(IInStream * inStream, CDatabase &database)
		{
			uint32 headerSize = ReadUInt32();
			if(headerSize != 0x60)
				return S_FALSE;
			database.PhySize = headerSize;
			uint32 unknown1 = ReadUInt32();
			if(unknown1 != 0 && unknown1 != 1) // it's 0 in one .sll file
				return S_FALSE;
			IsArc = true;
			/* uint32 timeStamp = */ ReadUInt32();
			// Considered as a big-endian DWORD, it appears to contain seconds (MSB) and
			// fractional seconds (second byte).
			// The third and fourth bytes may contain even more fractional bits.
			// The 4 least significant bits in the last byte are constant.
			/* uint32 lang = */ ReadUInt32();
			Byte g[16];
			ReadGUID(g); // {7C01FD10-7BAA-11D0-9E0C-00A0-C922-E6EC}
			ReadGUID(g); // {7C01FD11-7BAA-11D0-9E0C-00A0-C922-E6EC}
			const uint kNumSections = 2;
			uint64 sectionOffsets[kNumSections];
			uint64 sectionSizes[kNumSections];
			uint i;
			for(i = 0; i < kNumSections; i++) {
				sectionOffsets[i] = ReadUInt64();
				sectionSizes[i] = ReadUInt64();
				uint64 end = sectionOffsets[i] + sectionSizes[i];
				database.UpdatePhySize(end);
			}
			// if(chmVersion == 3)
			database.ContentOffset = ReadUInt64();
			/*
			   else
			   database.ContentOffset = database.StartPosition + 0x58
			 */

			// Section 0
			ReadChunk(inStream, sectionOffsets[0], sectionSizes[0]);
			if(sectionSizes[0] < 0x18)
				return S_FALSE;
			if(ReadUInt32() != 0x01FE)
				return S_FALSE;
			ReadUInt32(); // unknown:  0
			uint64 fileSize = ReadUInt64();
			database.UpdatePhySize(fileSize);
			ReadUInt32(); // unknown:  0
			ReadUInt32(); // unknown:  0

			// Section 1: The Directory Listing
			ReadChunk(inStream, sectionOffsets[1], sectionSizes[1]);
			if(ReadUInt32() != kSignature_ITSP)
				return S_FALSE;
			if(ReadUInt32() != 1) // version
				return S_FALSE;
			/* uint32 dirHeaderSize = */ ReadUInt32();
			ReadUInt32(); // 0x0A (unknown)
			uint32 dirChunkSize = ReadUInt32(); // $1000
			if(dirChunkSize < 32)
				return S_FALSE;
			/* uint32 density = */ ReadUInt32(); //  "Density" of quickref section, usually 2.
			/* uint32 depth = */ ReadUInt32(); //  Depth of the index tree: 1 there is no index,
			// 2 if there is one level of PMGI chunks.

			/* uint32 chunkNumber = */ ReadUInt32(); //  Chunk number of root index chunk, -1 if there is none
			// (though at least one file has 0 despite there being no
			// index chunk, probably a bug.)
			/* uint32 firstPmglChunkNumber = */ ReadUInt32(); // Chunk number of first PMGL (listing) chunk
			/* uint32 lastPmglChunkNumber = */ ReadUInt32(); // Chunk number of last PMGL (listing) chunk
			ReadUInt32(); // -1 (unknown)
			uint32 numDirChunks = ReadUInt32(); // Number of directory chunks (total)
			/* uint32 windowsLangId = */ ReadUInt32();
			ReadGUID(g); // {5D02926A-212E-11D0-9DF9-00A0C922E6EC}
			ReadUInt32(); // 0x54 (This is the length again)
			ReadUInt32(); // -1 (unknown)
			ReadUInt32(); // -1 (unknown)
			ReadUInt32(); // -1 (unknown)

			for(uint32 ci = 0; ci < numDirChunks; ci++) {
				uint64 chunkPos = _inBuffer.GetProcessedSize();
				if(ReadUInt32() == kSignature_PMGL) {
					// The quickref area is written backwards from the end of the chunk.
					// One quickref entry exists for every n entries in the file, where n
					// is calculated as 1 + (1 << quickref density). So for density = 2, n = 5.

					uint32 quickrefLength = ReadUInt32(); // Len of free space and/or quickref area at end of
												  // directory chunk
					if(quickrefLength > dirChunkSize || quickrefLength < 2)
						return S_FALSE;
					ReadUInt32(); // Always 0
					ReadUInt32(); // Chunk number of previous listing chunk when reading
								  // directory in sequence (-1 if this is the first listing chunk)
					ReadUInt32(); // Chunk number of next  listing chunk when reading
								  // directory in sequence (-1 if this is the last listing chunk)
					unsigned numItems = 0;

					for(;;) {
						uint64 offset = _inBuffer.GetProcessedSize() - chunkPos;
						uint32 offsetLimit = dirChunkSize - quickrefLength;
						if(offset > offsetLimit)
							return S_FALSE;
						if(offset == offsetLimit)
							break;
						RINOK(ReadDirEntry(database));
						numItems++;
					}

					Skip(quickrefLength - 2);

					unsigned rrr = ReadUInt16();
					if(rrr != numItems) {
						// Lazarus 9-26-2 chm contains 0 here.
						if(rrr != 0)
							return S_FALSE;
					}
				}
				else
					Skip(dirChunkSize - 4);
			}
			return S_OK;
		}

		HRESULT CInArchive::OpenHelp2(IInStream * inStream, CDatabase &database)
		{
			if(ReadUInt32() != 1) // version
				return S_FALSE;
			if(ReadUInt32() != 0x28) // Location of header section table
				return S_FALSE;
			uint32 numHeaderSections = ReadUInt32();
			const uint kNumHeaderSectionsMax = 5;
			if(numHeaderSections != kNumHeaderSectionsMax)
				return S_FALSE;

			IsArc = true;

			ReadUInt32(); // Len of post-header table
			Byte g[16];
			ReadGUID(g); // {0A9007C1-4076-11D3-8789-0000F8105754}

			// header section table
			uint64 sectionOffsets[kNumHeaderSectionsMax];
			uint64 sectionSizes[kNumHeaderSectionsMax];
			uint32 i;
			for(i = 0; i < numHeaderSections; i++) {
				sectionOffsets[i] = ReadUInt64();
				sectionSizes[i] = ReadUInt64();
				uint64 end = sectionOffsets[i] + sectionSizes[i];
				database.UpdatePhySize(end);
			}

			// Post-Header
			ReadUInt32(); // 2
			ReadUInt32(); // 0x98: offset to CAOL from beginning of post-header)
			// ----- Directory information
			ReadUInt64(); // Chunk number of top-level AOLI chunk in directory, or -1
			ReadUInt64(); // Chunk number of first AOLL chunk in directory
			ReadUInt64(); // Chunk number of last AOLL chunk in directory
			ReadUInt64(); // 0 (unknown)
			ReadUInt32(); // $2000 (Directory chunk size of directory)
			ReadUInt32(); // Quickref density for main directory, usually 2
			ReadUInt32(); // 0 (unknown)
			ReadUInt32(); // Depth of main directory index tree
						  // 1 there is no index, 2 if there is one level of AOLI chunks.
			ReadUInt64(); // 0 (unknown)
			uint64 numDirEntries = ReadUInt64(); // Number of directory entries
			// ----- Directory Index Information
			ReadUInt64(); // -1 (unknown, probably chunk number of top-level AOLI in directory index)
			ReadUInt64(); // Chunk number of first AOLL chunk in directory index
			ReadUInt64(); // Chunk number of last AOLL chunk in directory index
			ReadUInt64(); // 0 (unknown)
			ReadUInt32(); // $200 (Directory chunk size of directory index)
			ReadUInt32(); // Quickref density for directory index, usually 2
			ReadUInt32(); // 0 (unknown)
			ReadUInt32(); // Depth of directory index index tree.
			ReadUInt64(); // Possibly flags -- sometimes 1, sometimes 0.
			ReadUInt64(); // Number of directory index entries (same as number of AOLL
						  // chunks in main directory)

			// (The obvious guess for the following two fields, which recur in a number
			// of places, is they are maximum sizes for the directory and directory index.
			// However, I have seen no direct evidence that this is the case.)

			ReadUInt32(); // $100000 (Same as field following chunk size in directory)
			ReadUInt32(); // $20000 (Same as field following chunk size in directory index)

			ReadUInt64(); // 0 (unknown)
			if(ReadUInt32() != kSignature_CAOL)
				return S_FALSE;
			if(ReadUInt32() != 2) // (Most likely a version number)
				return S_FALSE;
			uint32 caolLength = ReadUInt32(); // $50 (Len of the CAOL section, which includes the ITSF section)
			if(caolLength >= 0x2C) {
				/* uint32 c7 = */ ReadUInt16(); // Unknown.  Remains the same when identical files are built.
				// Does not appear to be a checksum.  Many files have
				// 'HH' (HTML Help?) here, indicating this may be a compiler ID
				//  field.  But at least one ITOL/ITLS compiler does not set this
				// field to a constant value.
				ReadUInt16(); // 0 (Unknown.  Possibly part of 00A4 field)
				ReadUInt32(); // Unknown.  Two values have been seen -- $43ED, and 0.
				ReadUInt32(); // $2000 (Directory chunk size of directory)
				ReadUInt32(); // $200 (Directory chunk size of directory index)
				ReadUInt32(); // $100000 (Same as field following chunk size in directory)
				ReadUInt32(); // $20000 (Same as field following chunk size in directory index)
				ReadUInt32(); // 0 (unknown)
				ReadUInt32(); // 0 (Unknown)
				if(caolLength == 0x2C) {
					// fprintf(stdout, "\n !!!NewFormat\n");
					// fflush(stdout);
					database.ContentOffset = 0; // maybe we must add database.StartPosition here?
					database.NewFormat = true;
				}
				else if(caolLength == 0x50) {
					ReadUInt32(); // 0 (Unknown)
					if(ReadUInt32() != kSignature_ITSF)
						return S_FALSE;
					if(ReadUInt32() != 4) // $4 (Version number -- CHM uses 3)
						return S_FALSE;
					if(ReadUInt32() != 0x20) // $20 (length of ITSF)
						return S_FALSE;
					uint32 unknown = ReadUInt32();
					if(unknown != 0 && unknown != 1) // = 0 for some HxW files, 1 in other cases;
						return S_FALSE;
					database.ContentOffset = database.StartPosition + ReadUInt64();
					/* uint32 timeStamp = */ ReadUInt32();
					// A timestamp of some sort.
					// Considered as a big-endian DWORD, it appears to contain
					// seconds (MSB) and fractional seconds (second byte).
					// The third and fourth bytes may contain even more fractional
					// bits.  The 4 least significant bits in the last byte are constant.
					/* uint32 lang = */ ReadUInt32(); // BE?
				}
				else
					return S_FALSE;
			}

			// Section 0
			ReadChunk(inStream, database.StartPosition + sectionOffsets[0], sectionSizes[0]);
			if(sectionSizes[0] < 0x18)
				return S_FALSE;
			if(ReadUInt32() != 0x01FE)
				return S_FALSE;
			ReadUInt32(); // unknown:  0
			uint64 fileSize = ReadUInt64();
			database.UpdatePhySize(fileSize);
			ReadUInt32(); // unknown:  0
			ReadUInt32(); // unknown:  0

			// Section 1: The Directory Listing
			ReadChunk(inStream, database.StartPosition + sectionOffsets[1], sectionSizes[1]);
			if(ReadUInt32() != kSignature_IFCM)
				return S_FALSE;
			if(ReadUInt32() != 1) // (probably a version number)
				return S_FALSE;
			uint32 dirChunkSize = ReadUInt32(); // $2000
			if(dirChunkSize < 64)
				return S_FALSE;
			ReadUInt32(); // $100000  (unknown)
			ReadUInt32(); // -1 (unknown)
			ReadUInt32(); // -1 (unknown)
			uint32 numDirChunks = ReadUInt32();
			ReadUInt32(); // 0 (unknown, probably high word of above)

			for(uint32 ci = 0; ci < numDirChunks; ci++) {
				uint64 chunkPos = _inBuffer.GetProcessedSize();
				if(ReadUInt32() == kSignature_AOLL) {
					uint32 quickrefLength = ReadUInt32(); // Len of quickref area at end of directory chunk
					if(quickrefLength > dirChunkSize || quickrefLength < 2)
						return S_FALSE;
					ReadUInt64(); // Directory chunk number
					// This must match physical position in file, that is
					// the chunk size times the chunk number must be the
					// offset from the end of the directory header.
					ReadUInt64(); // Chunk number of previous listing chunk when reading
								  // directory in sequence (-1 if first listing chunk)
					ReadUInt64(); // Chunk number of next listing chunk when reading
								  // directory in sequence (-1 if last listing chunk)
					ReadUInt64(); // Number of first listing entry in this chunk
					ReadUInt32(); // 1 (unknown -- other values have also been seen here)
					ReadUInt32(); // 0 (unknown)

					unsigned numItems = 0;
					for(;;) {
						uint64 offset = _inBuffer.GetProcessedSize() - chunkPos;
						uint32 offsetLimit = dirChunkSize - quickrefLength;
						if(offset > offsetLimit)
							return S_FALSE;
						if(offset == offsetLimit)
							break;
						if(database.NewFormat) {
							uint16 nameLen = ReadUInt16();
							if(nameLen == 0)
								return S_FALSE;
							UString name;
							ReadUString((uint)nameLen, name);
							AString s;
							ConvertUnicodeToUTF8(name, s);
							Byte b = ReadByte();
							s.Add_Space();
							PrintByte(b, s);
							s.Add_Space();
							uint64 len = ReadEncInt();
							// then number of items ?
							// then length ?
							// then some data (binary encoding?)
							while(len-- != 0) {
								b = ReadByte();
								PrintByte(b, s);
							}
							database.NewFormatString += s;
							database.NewFormatString += "\r\n";
						}
						else {
							RINOK(ReadDirEntry(database));
						}
						numItems++;
					}
					Skip(quickrefLength - 2);
					if(ReadUInt16() != numItems)
						return S_FALSE;
					if(numItems > numDirEntries)
						return S_FALSE;
					numDirEntries -= numItems;
				}
				else
					Skip(dirChunkSize - 4);
			}
			return numDirEntries == 0 ? S_OK : S_FALSE;
		}

		HRESULT CInArchive::DecompressStream(IInStream * inStream, const CDatabase &database, const AString &name)
		{
			int index = database.FindItem(name);
			if(index < 0)
				return S_FALSE;
			const CItem &item = database.Items[index];
			_chunkSize = item.Size;
			return ReadChunk(inStream, database.ContentOffset + item.Offset, item.Size);
		}

		#define DATA_SPACE "::DataSpace/"
		#define kNameList DATA_SPACE "NameList"
		#define kStorage DATA_SPACE "Storage/"
		#define kContent "Content"
		#define kControlData "ControlData"
		#define kSpanInfo "SpanInfo"
		#define kTransform "Transform/"
		#define kResetTable "/InstanceData/ResetTable"
		#define kTransformList "List"

		static AString GetSectionPrefix(const AString &name)
		{
			AString s(kStorage);
			s += name;
			s += '/';
			return s;
		}

		#define RINOZ(x) { int __tt = (x); if(__tt != 0) return __tt; }

		static int CompareFiles(const uint * p1, const uint * p2, void * param)
		{
			const CObjectVector<CItem> &items = *(const CObjectVector<CItem> *)param;
			const CItem &item1 = items[*p1];
			const CItem &item2 = items[*p2];
			bool isDir1 = item1.IsDir();
			bool isDir2 = item2.IsDir();
			if(isDir1 && !isDir2)
				return -1;
			if(isDir2) {
				if(!isDir1)
					return 1;
			}
			else {
				RINOZ(MyCompare(item1.Section, item2.Section));
				RINOZ(MyCompare(item1.Offset, item2.Offset));
				RINOZ(MyCompare(item1.Size, item2.Size));
			}
			return MyCompare(*p1, *p2);
		}

		uint64 FASTCALL CFilesDatabase::GetFileSize(uint fileIndex) const { return Items[Indices[fileIndex]].Size; }
		uint64 FASTCALL CFilesDatabase::GetFileOffset(uint fileIndex) const { return Items[Indices[fileIndex]].Offset; }

		uint64 FASTCALL CFilesDatabase::GetFolder(uint fileIndex) const
		{
			const CItem &item = Items[Indices[fileIndex]];
			if(item.Section < Sections.Size()) {
				const CSectionInfo &section = Sections[(uint)item.Section];
				if(section.IsLzx())
					return section.Methods[0].LzxInfo.GetFolder(item.Offset);
			}
			return 0;
		}

		uint64 FASTCALL CFilesDatabase::GetLastFolder(uint fileIndex) const
		{
			const CItem &item = Items[Indices[fileIndex]];
			if(item.Section < Sections.Size()) {
				const CSectionInfo &section = Sections[(uint)item.Section];
				if(section.IsLzx())
					return section.Methods[0].LzxInfo.GetFolder(item.Offset + item.Size - 1);
			}
			return 0;
		}

		void CFilesDatabase::HighLevelClear()
		{
			LowLevel = true;
			Indices.Clear();
			Sections.Clear();
		}

		void CFilesDatabase::Clear()
		{
			CDatabase::Clear();
			HighLevelClear();
		}

		void CFilesDatabase::SetIndices()
		{
			FOR_VECTOR(i, Items) {
				const CItem &item = Items[i];
				if(item.IsUserItem() && item.Name.Len() != 1)
					Indices.Add(i);
			}
		}

		void CFilesDatabase::Sort()
		{
			Indices.Sort(CompareFiles, (void *)&Items);
		}

		bool CFilesDatabase::Check()
		{
			uint64 maxPos = 0;
			uint64 prevSection = 0;
			FOR_VECTOR(i, Indices) {
				const CItem &item = Items[Indices[i]];
				if(item.Section == 0 || item.IsDir())
					continue;
				if(item.Section != prevSection) {
					prevSection = item.Section;
					maxPos = 0;
					continue;
				}
				if(item.Offset < maxPos)
					return false;
				maxPos = item.Offset + item.Size;
				if(maxPos < item.Offset)
					return false;
			}
			return true;
		}

		bool CFilesDatabase::CheckSectionRefs()
		{
			FOR_VECTOR(i, Indices) {
				const CItem &item = Items[Indices[i]];
				if(item.Section == 0 || item.IsDir())
					continue;
				if(item.Section >= Sections.Size())
					return false;
			}
			return true;
		}

		static int inline GetLog(uint32 num)
		{
			for(int i = 0; i < 32; i++)
				if((1U << i) == num)
					return i;
			return -1;
		}

		HRESULT CInArchive::OpenHighLevel(IInStream * inStream, CFilesDatabase &database)
		{
			{
				// The NameList file
				RINOK(DecompressStream(inStream, database, (AString)kNameList));
				/* uint16 length = */ ReadUInt16();
				uint16 numSections = ReadUInt16();
				for(uint i = 0; i < numSections; i++) {
					CSectionInfo section;
					uint16 nameLen = ReadUInt16();
					UString name;
					ReadUString(nameLen, name);
					if(ReadUInt16() != 0)
						return S_FALSE;
					ConvertUnicodeToUTF8(name, section.Name);
					// if(!ConvertUnicodeToUTF8(name, section.Name)) return S_FALSE;
					database.Sections.Add(section);
				}
			}
			uint si;
			for(si = 1; si < database.Sections.Size(); si++) {
				CSectionInfo &section = database.Sections[si];
				AString sectionPrefix(GetSectionPrefix(section.Name));
				{
					// Content
					int index = database.FindItem(sectionPrefix + kContent);
					if(index < 0)
						return S_FALSE;
					const CItem &item = database.Items[index];
					section.Offset = item.Offset;
					section.CompressedSize = item.Size;
				}
				AString transformPrefix(sectionPrefix + kTransform);
				if(database.Help2Format) {
					// Transform List
					RINOK(DecompressStream(inStream, database, transformPrefix + kTransformList));
					if((_chunkSize & 0xF) != 0)
						return S_FALSE;
					uint numGuids = (uint)(_chunkSize / 0x10);
					if(numGuids < 1)
						return S_FALSE;
					for(uint i = 0; i < numGuids; i++) {
						CMethodInfo method;
						ReadGUID(method.Guid);
						section.Methods.Add(method);
					}
				}
				else {
					CMethodInfo method;
					memcpy(method.Guid, kChmLzxGuid, 16);
					section.Methods.Add(method);
				}
				{
					// Control Data
					RINOK(DecompressStream(inStream, database, sectionPrefix + kControlData));
					FOR_VECTOR(mi, section.Methods) {
						CMethodInfo &method = section.Methods[mi];
						uint32 numDWORDS = ReadUInt32();
						if(method.IsLzx()) {
							if(numDWORDS < 5)
								return S_FALSE;
							if(ReadUInt32() != kSignature_LZXC)
								return S_FALSE;
							CLzxInfo &li = method.LzxInfo;
							li.Version = ReadUInt32();
							if(li.Version != 2 && li.Version != 3)
								return S_FALSE;

							{
								// There is bug in VC6, if we use function call as parameter for inline
								// function
								uint32 val32 = ReadUInt32();
								int n = GetLog(val32);
								if(n < 0 || n > 16)
									return S_FALSE;
								li.ResetIntervalBits = n;
							}

							{
								uint32 val32 = ReadUInt32();
								int n = GetLog(val32);
								if(n < 0 || n > 16)
									return S_FALSE;
								li.WindowSizeBits = n;
							}

							li.CacheSize = ReadUInt32();
							numDWORDS -= 5;
							while(numDWORDS-- != 0)
								ReadUInt32();
						}
						else {
							uint32 numBytes = numDWORDS * 4;
							method.ControlData.Alloc(numBytes);
							ReadBytes(method.ControlData, numBytes);
						}
					}
				}

				{
					// SpanInfo
					RINOK(DecompressStream(inStream, database, sectionPrefix + kSpanInfo));
					section.UncompressedSize = ReadUInt64();
				}
				// read ResetTable for LZX
				FOR_VECTOR(mi, section.Methods) {
					CMethodInfo &method = section.Methods[mi];
					if(method.IsLzx()) {
						// ResetTable;
						RINOK(DecompressStream(inStream, database, transformPrefix + method.GetGuidString() + kResetTable));
						CResetTable &rt = method.LzxInfo.ResetTable;
						if(_chunkSize < 4) {
							if(_chunkSize != 0)
								return S_FALSE;
							// ResetTable is empty in .chw files
							if(section.UncompressedSize != 0)
								return S_FALSE;
							rt.UncompressedSize = 0;
							rt.CompressedSize = 0;
							// rt.BlockSize = 0;
						}
						else {
							uint32 ver = ReadUInt32(); // 2  unknown (possibly a version number)
							if(ver != 2 && ver != 3)
								return S_FALSE;
							uint32 numEntries = ReadUInt32();
							const uint kEntrySize = 8;
							if(ReadUInt32() != kEntrySize)
								return S_FALSE;
							const uint kRtHeaderSize = 4 * 4 + 8 * 3;
							if(ReadUInt32() != kRtHeaderSize)
								return S_FALSE;
							if(kRtHeaderSize + kEntrySize * (uint64)numEntries != _chunkSize)
								return S_FALSE;
							rt.UncompressedSize = ReadUInt64();
							rt.CompressedSize = ReadUInt64();
							uint64 blockSize = ReadUInt64();
							if(blockSize != kBlockSize)
								return S_FALSE;
							uint64 numBlocks = (rt.UncompressedSize + kBlockSize + 1) / kBlockSize;
							if(numEntries != numBlocks &&
										numEntries != numBlocks + 1)
								return S_FALSE;

							rt.ResetOffsets.ClearAndReserve(numEntries);

							for(uint32 i = 0; i < numEntries; i++) {
								uint64 v = ReadUInt64();
								if(i != 0 && v < rt.ResetOffsets[i - 1])
									return S_FALSE;
								rt.ResetOffsets.AddInReserved(v);
							}

							if(numEntries != 0)
								if(rt.ResetOffsets[0] != 0)
									return S_FALSE;

							if(numEntries == numBlocks + 1) {
								// Lazarus 9-26-2 chm contains additional entty
								if(rt.ResetOffsets.Back() != rt.CompressedSize)
									return S_FALSE;
							}
						}
					}
				}
			}

			database.SetIndices();
			database.Sort();
			return database.Check() ? S_OK : S_FALSE;
		}

		HRESULT CInArchive::Open2(IInStream * inStream, const uint64 * searchHeaderSizeLimit, CFilesDatabase &database)
		{
			IsArc = false;
			HeadersError = false;
			UnexpectedEnd = false;
			UnsupportedFeature = false;
			database.Clear();
			database.Help2Format = _help2;
			const uint32 chmVersion = 3;
			RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &database.StartPosition));
			if(!_inBuffer.Create(1 << 14))
				return E_OUTOFMEMORY;
			_inBuffer.SetStream(inStream);
			_inBuffer.Init();

			if(_help2) {
				const uint kSignatureSize = 8;
				const uint64 signature = ((uint64)kSignature_ITLS << 32) | kSignature_ITOL;
				uint64 limit = 1 << 18;

				if(searchHeaderSizeLimit)
					if(limit > *searchHeaderSizeLimit)
						limit = *searchHeaderSizeLimit;
				uint64 val = 0;
				for(;;) {
					Byte b;
					if(!_inBuffer.ReadByte(b))
						return S_FALSE;
					val >>= 8;
					val |= ((uint64)b) << ((kSignatureSize - 1) * 8);
					if(_inBuffer.GetProcessedSize() >= kSignatureSize) {
						if(val == signature)
							break;
						if(_inBuffer.GetProcessedSize() > limit)
							return S_FALSE;
					}
				}

				database.StartPosition += _inBuffer.GetProcessedSize() - kSignatureSize;
				RINOK(OpenHelp2(inStream, database));
				if(database.NewFormat)
					return S_OK;
			}
			else {
				if(ReadUInt32() != kSignature_ITSF)
					return S_FALSE;
				if(ReadUInt32() != chmVersion)
					return S_FALSE;
				RINOK(OpenChm(inStream, database));
			}
		  #ifndef CHM_LOW
			try {
				try {
					HRESULT res = OpenHighLevel(inStream, database);
					if(res == S_FALSE) {
						UnsupportedFeature = true;
						database.HighLevelClear();
						return S_OK;
					}
					RINOK(res);
					if(!database.CheckSectionRefs())
						HeadersError = true;
					database.LowLevel = false;
				}
				catch(...) {
					database.HighLevelClear();
					throw;
				}
			}
			// catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(CEnexpectedEndException &) { UnexpectedEnd = true; }
			catch(CHeaderErrorException &) { HeadersError = true; }
			catch(...) { throw; }

		  #endif

			return S_OK;
		}

		HRESULT CInArchive::Open(IInStream * inStream, const uint64 * searchHeaderSizeLimit, CFilesDatabase &database)
		{
			try {
				try {
					HRESULT res = Open2(inStream, searchHeaderSizeLimit, database);
					m_InStreamRef.Release();
					return res;
				}
				catch(...) {
					m_InStreamRef.Release();
					throw;
				}
			}
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(CEnexpectedEndException &) { UnexpectedEnd = true; }
			catch(CHeaderErrorException &) { HeadersError = true; }
			return S_FALSE;
		}
	}
}
