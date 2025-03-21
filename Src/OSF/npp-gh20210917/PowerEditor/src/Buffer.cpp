// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
#include <npp-internal.h>
#pragma hdrstop
#include "uchardet.h"

static const int blockSize = 128 * 1024 + 4;
static const int CR = 0x0D;
static const int LF = 0x0A;

long Buffer::_recentTagCtr = 0;

namespace // anonymous
{
static EolType getEOLFormatForm(const char * const data, size_t length, EolType defvalue = EolType::osdefault)
{
	assert(length == 0 or data != nullptr && "invalid buffer for getEOLFormatForm()");
	for(size_t i = 0; i != length; ++i) {
		if(data[i] == CR) {
			if(i + 1 < length && data[i + 1] == LF)
				return EolType::windows;
			return EolType::macos;
		}
		if(data[i] == LF)
			return EolType::unix;
	}
	return defvalue;         // fallback unknown
}
} // anonymous namespace

Buffer::Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const TCHAR * fileName)
// type must be either DOC_REGULAR or DOC_UNNAMED
	: _pManager(pManager), _id(id), _doc(doc), _lang(L_TEXT)
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	const NewDocDefaultSettings& ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();
	_eolFormat = ndds._format;
	_unicodeMode = ndds._unicodeMode;
	_encoding = ndds._codepage;
	if(_encoding != -1)
		_unicodeMode = uniCookie;
	_currentStatus = type;
	setFileName(fileName, ndds._lang);
	updateTimeStamp();
	checkFileState();
	// reset after initialization
	_isDirty   = false;
	_canNotify = true;
	_needLexer = false; // new buffers do not need lexing, Scintilla takes care of that
}

void Buffer::doNotify(int mask)
{
	if(_canNotify) {
		assert(_pManager != nullptr);
		_pManager->beNotifiedOfBufferChange(this, mask);
	}
}

void Buffer::setDirty(bool dirty)
{
	_isDirty = dirty;
	doNotify(BufferChangeDirty);
}

void Buffer::setEncoding(int encoding)
{
	_encoding = encoding;
	doNotify(BufferChangeUnicode | BufferChangeDirty);
}

void Buffer::setUnicodeMode(UniMode mode)
{
	_unicodeMode = mode;
	doNotify(BufferChangeUnicode | BufferChangeDirty);
}

void Buffer::setLangType(LangType lang, const TCHAR* userLangName)
{
	if(lang == _lang && lang != L_USER)
		return;
	_lang = lang;
	if(_lang == L_USER)
		_userLangExt = userLangName;
	_needLexer = true;      //change of lang means lexern needs updating
	doNotify(BufferChangeLanguage|BufferChangeLexing);
}

void Buffer::updateTimeStamp()
{
	FILETIME timeStamp = {};
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
		timeStamp = attributes.ftLastWriteTime;
	}
	if(CompareFileTime(&_timeStamp, &timeStamp) != 0) {
		_timeStamp = timeStamp;
		doNotify(BufferChangeTimestamp);
	}
}

// Set full path file name in buffer object,
// and determinate its language by its extension.
// If the ext is not in the list, the defaultLang passed as argument will be set.
void Buffer::setFileName(const TCHAR * fn, LangType defaultLang)
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	if(_fullPathName == fn) {
		updateTimeStamp();
		doNotify(BufferChangeTimestamp);
		return;
	}
	_fullPathName = fn;
	_fileName = PathFindFileName(_fullPathName.c_str());
	// for _lang
	LangType newLang = defaultLang;
	TCHAR * ext = PathFindExtension(_fullPathName.c_str());
	if(*ext == '.') { // extension found
		ext += 1;
		// Define User Lang firstly
		const TCHAR* langName = nppParamInst.getUserDefinedLangNameFromExt(ext, _fileName);
		if(langName) {
			newLang = L_USER;
			_userLangExt = langName;
		}
		else { // if it's not user lang, then check if it's supported lang
			_userLangExt.clear();
			newLang = nppParamInst.getLangFromExt(ext);
		}
	}
	if(newLang == defaultLang || newLang == L_TEXT) {       //language can probably be refined
		if((OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("makefile")) == 0) || (OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("GNUmakefile")) == 0))
			newLang = L_MAKEFILE;
		else if(OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("CmakeLists.txt")) == 0)
			newLang = L_CMAKE;
		else if((OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("SConstruct")) == 0) || 
			(OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("SConscript")) == 0) || (OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("wscript")) == 0))
			newLang = L_PYTHON;
		else if((OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("Rakefile")) == 0) || (OrdinalIgnoreCaseCompareStrings(_fileName, TEXT("Vagrantfile")) == 0))
			newLang = L_RUBY;
	}
	updateTimeStamp();
	if(!_hasLangBeenSetFromMenu && (newLang != _lang || _lang == L_USER)) {
		_lang = newLang;
		doNotify(BufferChangeFilename | BufferChangeLanguage | BufferChangeTimestamp);
		return;
	}
	doNotify(BufferChangeFilename | BufferChangeTimestamp);
}

bool Buffer::checkFileState() // returns true if the status has been changed (it can change into DOC_REGULAR too). false
                              // otherwise
{
	// 1. Unsaved document cannot change by environment
	// 2. Monitoring is sent by NPPM_INTERNAL_RELOADSCROLLTOEND
	// So no need to check file status for both the above cases
	if(_currentStatus == DOC_UNNAMED || isMonitoringOn())
		return false;

	WIN32_FILE_ATTRIBUTE_DATA attributes;
	bool isWow64Off = false;
	NppParameters& nppParam = NppParameters::getInstance();

	if(!PathFileExists(_fullPathName.c_str())) {
		nppParam.safeWow64EnableWow64FsRedirection(FALSE);
		isWow64Off = true;
	}

	bool isOK = false;
	if(_currentStatus != DOC_DELETED && !PathFileExists(_fullPathName.c_str())) {   //document has been deleted
		_currentStatus = DOC_DELETED;
		_isFileReadOnly = false;
		_isDirty = true;        //dirty sicne no match with filesystem
		_timeStamp = {};
		doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
		isOK = true;
	}
	else if(_currentStatus == DOC_DELETED && PathFileExists(_fullPathName.c_str())) { //document has returned from its grave
		if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
			_isFileReadOnly = attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
			_currentStatus = DOC_MODIFIED;
			_timeStamp = attributes.ftLastWriteTime;
			if(_reloadFromDiskRequestGuard.try_lock()) {
				doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
				_reloadFromDiskRequestGuard.unlock();
			}
			isOK = true;
		}
	}
	else if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
		int mask = 0;   //status always 'changes', even if from modified to modified
		bool isFileReadOnly = attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
		if(isFileReadOnly != _isFileReadOnly) {
			_isFileReadOnly = isFileReadOnly;
			mask |= BufferChangeReadonly;
		}
		if(CompareFileTime(&_timeStamp, &attributes.ftLastWriteTime) != 0) {
			_timeStamp = attributes.ftLastWriteTime;
			mask |= BufferChangeTimestamp;
			_currentStatus = DOC_MODIFIED;
			mask |= BufferChangeStatus;     //status always 'changes', even if from modified to modified
		}
		if(mask != 0) {
			if(_reloadFromDiskRequestGuard.try_lock()) {
				doNotify(mask);
				_reloadFromDiskRequestGuard.unlock();
				return true;
			}
		}
		return false;
	}

	if(isWow64Off) {
		nppParam.safeWow64EnableWow64FsRedirection(TRUE);
	}
	return isOK;
}

void Buffer::reload()
{
	WIN32_FILE_ATTRIBUTE_DATA attributes;
	if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
		_timeStamp = attributes.ftLastWriteTime;
		_currentStatus = DOC_NEEDRELOAD;
		doNotify(BufferChangeTimestamp | BufferChangeStatus);
	}
}

int64_t Buffer::getFileLength() const
{
	if(_currentStatus != DOC_UNNAMED) {
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
			LARGE_INTEGER size;
			size.LowPart = attributes.nFileSizeLow;
			size.HighPart = attributes.nFileSizeHigh;
			return size.QuadPart;
		}
	}
	return -1;
}

generic_string Buffer::getFileTime(fileTimeType ftt) const
{
	generic_string result;
	if(_currentStatus != DOC_UNNAMED) {
		WIN32_FILE_ATTRIBUTE_DATA attributes;
		if(GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0) {
			FILETIME rawtime;
			switch(ftt) {
				case ft_created: rawtime = attributes.ftCreationTime; break;
				case ft_modified: rawtime = attributes.ftLastWriteTime; break;
				default: rawtime = attributes.ftLastAccessTime; break;
			}
			SYSTEMTIME utcSystemTime, localSystemTime;
			FileTimeToSystemTime(&rawtime, &utcSystemTime);
			SystemTimeToTzSpecificLocalTime(nullptr, &utcSystemTime, &localSystemTime);

			const size_t dateTimeStrLen = 256;
			TCHAR bufDate[dateTimeStrLen] = {'\0'};
			GetDateFormat(LOCALE_USER_DEFAULT, 0, &localSystemTime, nullptr, bufDate, dateTimeStrLen);
			result += bufDate;
			result += ' ';

			TCHAR bufTime[dateTimeStrLen] = {'\0'};
			GetTimeFormat(LOCALE_USER_DEFAULT, 0, &localSystemTime, nullptr, bufTime, dateTimeStrLen);
			result += bufTime;
		}
	}
	return result;
}

void Buffer::setPosition(const Position & pos, ScintillaEditView * identifier)
{
	int index = indexOfReference(identifier);
	if(index >= 0)
		_positions[index] = pos;
}

Position& Buffer::getPosition(ScintillaEditView* identifier)
{
	int index = indexOfReference(identifier);
	return _positions.at(index);
}

void Buffer::setHeaderLineState(const std::vector <size_t> & folds, ScintillaEditView * identifier)
{
	int index = indexOfReference(identifier);
	if(index >= 0) {
		//deep copy
		std::vector <size_t> & local = _foldStates[index];
		local.clear();
		size_t size = folds.size();
		for(size_t i = 0; i < size; ++i)
			local.push_back(folds[i]);
	}
}

const std::vector <size_t> & Buffer::getHeaderLineState(const ScintillaEditView * identifier) const
{
	int index = indexOfReference(identifier);
	return _foldStates.at(index);
}

Lang * Buffer::getCurrentLang() const
{
	NppParameters& nppParam = NppParameters::getInstance();
	int i = 0;
	Lang * l = nppParam.getLangFromIndex(i);
	++i;
	while(l) {
		if(l->_langID == _lang)
			return l;
		l = nppParam.getLangFromIndex(i);
		++i;
	}
	return nullptr;
}

int Buffer::indexOfReference(const ScintillaEditView * identifier) const
{
	size_t size = _referees.size();
	for(size_t i = 0; i < size; ++i) {
		if(_referees[i] == identifier)
			return static_cast<int>(i);
	}
	return -1;      //not found
}

int Buffer::addReference(ScintillaEditView * identifier)
{
	if(indexOfReference(identifier) != -1)
		return _references;

	_referees.push_back(identifier);
	_positions.push_back(Position());
	_foldStates.push_back(std::vector <size_t>());
	++_references;
	return _references;
}

int Buffer::removeReference(ScintillaEditView * identifier)
{
	int indexToPop = indexOfReference(identifier);
	if(indexToPop == -1)
		return _references;
	_referees.erase(_referees.begin() + indexToPop);
	_positions.erase(_positions.begin() + indexToPop);
	_foldStates.erase(_foldStates.begin() + indexToPop);
	_references--;
	return _references;
}

void Buffer::setHideLineChanged(bool isHide, int location)
{
	//First run through all docs without removing markers
	for(int i = 0; i < _references; ++i)
		_referees.at(i)->notifyMarkers(this, isHide, location, false); // (i == _references-1));
	if(!isHide) { // no deleting if hiding lines
		//Then all docs to remove markers.
		for(int i = 0; i < _references; ++i)
			_referees.at(i)->notifyMarkers(this, isHide, location, true);
	}
}

void Buffer::setDeferredReload() // triggers a reload on the next Document access
{
	_isDirty = false;       //when reloading, just set to false, since it should be marked as clean
	_needReloading = true;
	doNotify(BufferChangeDirty);
}

//filemanager

FileManager::~FileManager()
{
	for(std::vector <Buffer *>::iterator it = _buffers.begin(), end = _buffers.end(); it != end; ++it) {
		delete *it;
	}
}

void FileManager::init(Notepad_plus * pNotepadPlus, ScintillaEditView * pscratchTilla)
{
	_pNotepadPlus = pNotepadPlus;
	_pscratchTilla = pscratchTilla;
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, false);  //dont store any undo information
	_scratchDocDefault = (Document)_pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, _scratchDocDefault);
}

void FileManager::checkFilesystemChanges(bool bCheckOnlyCurrentBuffer)
{
	if(bCheckOnlyCurrentBuffer) {
		Buffer* buffer = _pNotepadPlus->getCurrentBuffer();
		buffer->checkFileState();
	}
	else {
		for(int i = int(_nbBufs) - 1; i >= 0; i--) {
			if(i >= int(_nbBufs)) {
				if(_nbBufs == 0)
					return;

				i = int(_nbBufs) - 1;
			}
			_buffers[i]->checkFileState();  //something has changed. Triggers update automatically
		}
	}
}

size_t FileManager::getNbDirtyBuffers() const
{
	size_t nb_dirtyBufs = 0;
	for(size_t i = 0; i < _nbBufs; ++i) {
		if(_buffers[i]->_isDirty)
			++nb_dirtyBufs;
	}
	return nb_dirtyBufs;
}

int FileManager::getBufferIndexByID(BufferID id)
{
	for(size_t i = 0; i < _nbBufs; ++i) {
		if(_buffers[i]->_id == id)
			return static_cast<int>(i);
	}
	return -1;
}

Buffer* FileManager::getBufferByIndex(size_t index)
{
	if(index >= _buffers.size())
		return nullptr;
	return _buffers.at(index);
}

void FileManager::beNotifiedOfBufferChange(Buffer* theBuf, int mask)
{
	_pNotepadPlus->notifyBufferChanged(theBuf, mask);
}

void FileManager::addBufferReference(BufferID buffer, ScintillaEditView * identifier)
{
	Buffer* buf = getBufferByID(buffer);
	buf->addReference(identifier);
}

void FileManager::closeBuffer(BufferID id, ScintillaEditView * identifier)
{
	int index = getBufferIndexByID(id);
	Buffer* buf = getBufferByIndex(index);

	int refs = buf->removeReference(identifier);

	if(!refs) { // buffer can be deallocated
		_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, buf->_doc);     //release for FileManager, Document is
		                                                                // now gone
		_buffers.erase(_buffers.begin() + index);
		delete buf;
		_nbBufs--;
	}
}

// backupFileName is sentinel of backup mode: if it's not NULL, then we use it (load it). Otherwise we use filename
BufferID FileManager::loadFile(const TCHAR * filename, Document doc, int encoding, const TCHAR * backupFileName, FILETIME fileNameTimestamp)
{
	bool ownDoc = false;
	if(!doc) {
		doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);
		ownDoc = true;
	}

	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
	if(_tcschr(fullpath, '~')) {
		::GetLongPathName(fullpath, fullpath, MAX_PATH);
	}
	bool isSnapshotMode = backupFileName != NULL && PathFileExists(backupFileName);
	if(isSnapshotMode && !PathFileExists(fullpath)) { // if backup mode and fullpath doesn't exist, we guess is UNTITLED
		wcscpy_s(fullpath, MAX_PATH, filename); // we restore fullpath with filename, in our case is "new  #"
	}
	Utf8_16_Read UnicodeConvertor;  //declare here so we can get information after loading is done
	char data[blockSize + 8]; // +8 for incomplete multibyte char
	LoadedFileFormat loadedFileFormat;
	loadedFileFormat._encoding = encoding;
	loadedFileFormat._eolFormat = EolType::unknown;
	loadedFileFormat._language = L_TEXT;

	bool res = loadFileData(doc, backupFileName ? backupFileName : fullpath, data, &UnicodeConvertor, loadedFileFormat);
	if(res) {
		Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_REGULAR, fullpath);
		BufferID id = static_cast<BufferID>(newBuf);
		newBuf->_id = id;
		if(backupFileName != NULL) {
			newBuf->_backupFileName = backupFileName;
			if(!PathFileExists(fullpath))
				newBuf->_currentStatus = DOC_UNNAMED;
		}
		const FILETIME zeroTime = {};
		if(CompareFileTime(&fileNameTimestamp, &zeroTime) != 0)
			newBuf->_timeStamp = fileNameTimestamp;

		_buffers.push_back(newBuf);
		++_nbBufs;
		Buffer* buf = _buffers.at(_nbBufs - 1);

		// restore the encoding (ANSI based) while opening the existing file
		buf->setEncoding(-1);

		// if no file extension, and the language has been detected,  we use the detected value
		if((buf->getLangType() == L_TEXT) && (loadedFileFormat._language != L_TEXT))
			buf->setLangType(loadedFileFormat._language);

		setLoadedBufferEncodingAndEol(buf, UnicodeConvertor, loadedFileFormat._encoding, loadedFileFormat._eolFormat);

		//determine buffer properties
		++_nextBufferID;
		return id;
	}
	else { //failed loading, release document
		if(ownDoc)
			_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, doc);   //Failure, so release document
		return BUFFER_INVALID;
	}
}

bool FileManager::reloadBuffer(BufferID id)
{
	Buffer* buf = getBufferByID(id);
	Document doc = buf->getDocument();
	Utf8_16_Read UnicodeConvertor;
	buf->_canNotify = false;        //disable notify during file load, we dont want dirty to be triggered
	char data[blockSize + 8]; // +8 for incomplete multibyte char
	LoadedFileFormat loadedFileFormat;
	loadedFileFormat._encoding = buf->getEncoding();
	loadedFileFormat._eolFormat = EolType::unknown;
	loadedFileFormat._language = buf->getLangType();
	buf->setLoadedDirty(false);     // Since the buffer will be reloaded from the disk, and it will be clean (not
	                                // dirty), we can set _isLoadedDirty false safetly.
	// Set _isLoadedDirty false before calling "_pscratchTilla->execute(SCI_CLEARALL);" in loadFileData() to avoid
	// setDirty in SCN_SAVEPOINTREACHED / SCN_SAVEPOINTLEFT
	bool res = loadFileData(doc, buf->getFullPathName(), data, &UnicodeConvertor, loadedFileFormat);
	buf->_canNotify = true;
	if(res) {
		setLoadedBufferEncodingAndEol(buf, UnicodeConvertor, loadedFileFormat._encoding, loadedFileFormat._eolFormat);
	}
	return res;
}

void FileManager::setLoadedBufferEncodingAndEol(Buffer* buf, const Utf8_16_Read& UnicodeConvertor, int encoding, EolType bkformat)
{
	if(encoding == -1) {
		NppParameters& nppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();
		UniMode um = UnicodeConvertor.getEncoding();
		if(um == uni7Bit)
			um = (ndds._openAnsiAsUtf8) ? uniCookie : uni8Bit;
		buf->setUnicodeMode(um);
	}
	else {
		// Test if encoding is set to UTF8 w/o BOM (usually for utf8 indicator of xml or html)
		buf->setEncoding((encoding == SC_CP_UTF8) ? -1 : encoding);
		buf->setUnicodeMode(uniCookie);
	}
	// Since the buffer will be reloaded from the disk, EOL might have been changed
	if(bkformat != EolType::unknown)
		buf->setEolFormat(bkformat);
}

bool FileManager::reloadBufferDeferred(BufferID id)
{
	Buffer* buf = getBufferByID(id);
	buf->setDeferredReload();
	return true;
}

bool FileManager::deleteFile(BufferID id)
{
	Buffer* buf = getBufferByID(id);
	generic_string fileNamePath = buf->getFullPathName();
	// Make sure to form a string with double '\0' terminator.
	fileNamePath.append(1, '\0');
	if(!PathFileExists(fileNamePath.c_str()))
		return false;
	SHFILEOPSTRUCT fileOpStruct = {0};
	fileOpStruct.hwnd = NULL;
	fileOpStruct.pFrom = fileNamePath.c_str();
	fileOpStruct.pTo = NULL;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.fFlags = FOF_ALLOWUNDO;
	fileOpStruct.fAnyOperationsAborted = false;
	fileOpStruct.hNameMappings = NULL;
	fileOpStruct.lpszProgressTitle     = NULL;
	return SHFileOperation(&fileOpStruct) == 0;
}

bool FileManager::moveFile(BufferID id, const TCHAR * newFileName)
{
	Buffer* buf = getBufferByID(id);
	const TCHAR * fileNamePath = buf->getFullPathName();
	if(::MoveFileEx(fileNamePath, newFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) == 0)
		return false;
	buf->setFileName(newFileName);
	return true;
}
/*
   Specs and Algorithm of session snapshot & periodic backup system:
   Notepad++ quits without asking for saving unsaved file.
   It restores all the unsaved files and document as the states they left.

   For existing file (c:\tmp\foo.h)
        - Open
        In the next session, Notepad++
        1. load backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist, otherwise load FILENAME
           (c:\tmp\foo.h).
        2. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, set it dirty (red).
        3. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, last modif timestamp of FILENAME
           (c:\tmp\foo.h), compare with tracked timestamp (in session.xml).
        4. in the case of unequal result, tell user the FILENAME (c:\tmp\foo.h) was modified. ask user if he want to
           reload FILENAME(c:\tmp\foo.h)

        - Editing
        when a file starts being modified, a file will be created with name: FILENAME@CREATION_TIMESTAMP
           (backup\foo.h@198776)
        the Buffer object will associate with this FILENAME@CREATION_TIMESTAMP file (backup\foo.h@198776).
        1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on
           modified notificatin).
        2. sync: each save file, or close file, the backup file will be deleted (if buffer is not dirty).
        3. before switch off to another tab (or close files on exit), check 1 & 2 (sync with backup).

        - Close
        In the current session, Notepad++
        1. track FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist (in session.xml).
        2. track last modified timestamp of FILENAME (c:\tmp\foo.h) if FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776)
           was tracked  (in session.xml).

   For untitled document (new  4)
        - Open
        In the next session, Notepad++
        1. open file UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
        2. set label as UNTITLED_NAME (new  4) and disk icon as red.

        - Editing
        when a untitled document starts being modified, a backup file will be created with name:
           UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
        the Buffer object will associate with this UNTITLED_NAME@CREATION_TIMESTAMP file (backup\new  4@198776).
        1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on
           modified notificatin).
        2. sync: if untitled document is saved, or closed, the backup file will be deleted.
        3. before switch off to another tab (or close documents on exit), check 1 & 2 (sync with backup).

        - CLOSE
        In the current session, Notepad++
        1. track UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776) in session.xml.
 */
bool FileManager::backupCurrentBuffer()
{
	static std::mutex backup_mutex;
	std::lock_guard<std::mutex> lock(backup_mutex);
	Buffer* buffer = _pNotepadPlus->getCurrentBuffer();
	bool result = false;
	bool hasModifForSession = false;
	if(buffer->isDirty()) {
		if(buffer->isModified()) { // buffer dirty and modified, write the backup file
			UniMode mode = buffer->getUnicodeMode();
			if(mode == uniCookie)
				mode = uni8Bit; //set the mode to ANSI to prevent converter from adding BOM and
			                        // performing conversions, Scintilla's data can be copied directly

			Utf8_16_Write UnicodeConvertor;
			UnicodeConvertor.setEncoding(mode);
			int encoding = buffer->getEncoding();

			generic_string backupFilePath = buffer->getBackupFileName();
			if(backupFilePath.empty()) {
				// Create file
				backupFilePath = NppParameters::getInstance().getUserPath();
				backupFilePath += TEXT("\\backup\\");

				// if "backup" folder doesn't exist, create it.
				if(!PathFileExists(backupFilePath.c_str())) {
					::CreateDirectory(backupFilePath.c_str(), NULL);
				}

				backupFilePath += buffer->getFileName();

				const int temBufLen = 32;
				TCHAR tmpbuf[temBufLen];
				time_t ltime = time(0);
				struct tm* today = localtime(&ltime);
				if(!today)
					return false;

				generic_strftime(tmpbuf, temBufLen, TEXT("%Y-%m-%d_%H%M%S"), today);

				backupFilePath += TEXT("@");
				backupFilePath += tmpbuf;

				// Set created file name in buffer
				buffer->setBackupFileName(backupFilePath);

				// Session changes, save it
				hasModifForSession = true;
			}

			TCHAR fullpath[MAX_PATH];
			::GetFullPathName(backupFilePath.c_str(), MAX_PATH, fullpath, NULL);
			if(_tcschr(fullpath, '~')) {
				::GetLongPathName(fullpath, fullpath, MAX_PATH);
			}

			// Make sure the backup file is not read only
			DWORD dwFileAttribs = ::GetFileAttributes(fullpath);
			if(dwFileAttribs & FILE_ATTRIBUTE_READONLY) { // if file is read only, remove read only attribute
				dwFileAttribs ^= FILE_ATTRIBUTE_READONLY;
				::SetFileAttributes(fullpath, dwFileAttribs);
			}
			FILE * fp = UnicodeConvertor.fopen(fullpath, TEXT("wbc"));
			if(fp) {
				int lengthDoc = _pNotepadPlus->_pEditView->getCurrentDocLen();
				char * buf = (char *)_pNotepadPlus->_pEditView->execute(SCI_GETCHARACTERPOINTER); //to get characters directly from Scintilla buffer
				size_t items_written = 0;
				if(encoding == -1) { //no special encoding; can be handled directly by Utf8_16_Write
					items_written = UnicodeConvertor.fwrite(buf, lengthDoc);
					if(lengthDoc == 0)
						items_written = 1;
				}
				else {
					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
					int grabSize;
					for(int i = 0; i < lengthDoc; i += grabSize) {
						grabSize = lengthDoc - i;
						if(grabSize > blockSize)
							grabSize = blockSize;
						int newDataLen = 0;
						int incompleteMultibyteChar = 0;
						const char * newData = wmc.encode(SC_CP_UTF8, encoding, buf+i, grabSize, &newDataLen, &incompleteMultibyteChar);
						grabSize -= incompleteMultibyteChar;
						items_written = UnicodeConvertor.fwrite(newData, newDataLen);
					}
					if(lengthDoc == 0)
						items_written = 1;
				}
				UnicodeConvertor.fclose();

				// Note that fwrite() doesn't return the number of bytes written, but rather the number
				// of ITEMS.
				if(items_written == 1) { // backup file has been saved
					buffer->setModifiedStatus(false);
					result = true;  //all done
				}
			}
		}
		else { // buffer dirty but unmodified
			result = true;
		}
	}
	else { // buffer not dirty, sync: delete the backup file
		generic_string backupFilePath = buffer->getBackupFileName();
		if(!backupFilePath.empty()) {
			// delete backup file
			generic_string file2Delete = buffer->getBackupFileName();
			buffer->setBackupFileName(generic_string());
			result = (::DeleteFile(file2Delete.c_str()) != 0);
			// Session changes, save it
			hasModifForSession = true;
		}
		//printStr(TEXT("backup deleted in backupCurrentBuffer"));
		result = true; // no backup file to delete
	}
	//printStr(TEXT("backup sync"));
	if(result && hasModifForSession) {
		//printStr(buffer->getBackupFileName().c_str());
		_pNotepadPlus->saveCurrentSession();
	}
	return result;
}

bool FileManager::deleteBufferBackup(BufferID id)
{
	Buffer* buffer = getBufferByID(id);
	bool result = true;
	generic_string backupFilePath = buffer->getBackupFileName();
	if(!backupFilePath.empty()) {
		// delete backup file
		buffer->setBackupFileName(generic_string());
		result = (::DeleteFile(backupFilePath.c_str()) != 0);
	}
	return result;
}

SavingStatus FileManager::saveBuffer(BufferID id, const TCHAR * filename, bool isCopy)
{
	static std::mutex save_mutex;
	std::lock_guard<std::mutex> lock(save_mutex);
	Buffer* buffer = getBufferByID(id);
	bool isHiddenOrSys = false;
	DWORD attrib = 0;
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
	if(_tcschr(fullpath, '~')) {
		::GetLongPathName(fullpath, fullpath, MAX_PATH);
	}
	if(PathFileExists(fullpath)) {
		attrib = ::GetFileAttributes(fullpath);
		if(attrib != INVALID_FILE_ATTRIBUTES) {
			isHiddenOrSys = (attrib & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
			if(isHiddenOrSys)
				::SetFileAttributes(filename, attrib & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM));
		}
	}
	UniMode mode = buffer->getUnicodeMode();
	if(mode == uniCookie)
		mode = uni8Bit; //set the mode to ANSI to prevent converter from adding BOM and performing conversions,
			// Scintilla's data can be copied directly
	Utf8_16_Write UnicodeConvertor;
	UnicodeConvertor.setEncoding(mode);
	int encoding = buffer->getEncoding();
	FILE * fp = UnicodeConvertor.fopen(fullpath, TEXT("wbc"));
	if(!fp) {
		return SavingStatus::SaveOpenFailed;
	}
	else {
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);    //generate new document
		int lengthDoc = _pscratchTilla->getCurrentDocLen();
		char * buf = (char *)_pscratchTilla->execute(SCI_GETCHARACTERPOINTER); //to get characters directly from Scintilla buffer
		size_t items_written = 0;
		if(encoding == -1) { //no special encoding; can be handled directly by Utf8_16_Write
			items_written = UnicodeConvertor.fwrite(buf, lengthDoc);
			if(lengthDoc == 0)
				items_written = 1;
		}
		else {
			WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
			int grabSize;
			for(int i = 0; i < lengthDoc; i += grabSize) {
				grabSize = lengthDoc - i;
				if(grabSize > blockSize)
					grabSize = blockSize;
				int newDataLen = 0;
				int incompleteMultibyteChar = 0;
				const char * newData = wmc.encode(SC_CP_UTF8, encoding, buf+i, grabSize, &newDataLen, &incompleteMultibyteChar);
				grabSize -= incompleteMultibyteChar;
				items_written = UnicodeConvertor.fwrite(newData, newDataLen);
			}
			if(lengthDoc == 0)
				items_written = 1;
		}
		// check the language du fichier
		LangType language = detectLanguageFromTextBegining((uchar *)buf, lengthDoc);
		UnicodeConvertor.fclose();
		// Error, we didn't write the entire document to disk.
		// Note that fwrite() doesn't return the number of bytes written, but rather the number of ITEMS.
		if(items_written != 1) {
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			return SavingStatus::SaveWrittingFailed;
		}
		if(isHiddenOrSys)
			::SetFileAttributes(fullpath, attrib);
		if(isCopy) { // Save As command
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			return SavingStatus::SaveOK;    //all done
		}
		buffer->setFileName(fullpath, language);
		buffer->setDirty(false);
		buffer->setStatus(DOC_REGULAR);
		buffer->checkFileState();
		_pscratchTilla->execute(SCI_SETSAVEPOINT);
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
		generic_string backupFilePath = buffer->getBackupFileName();
		if(!backupFilePath.empty()) {
			// delete backup file
			buffer->setBackupFileName(generic_string());
			::DeleteFile(backupFilePath.c_str());
		}
		return SavingStatus::SaveOK;
	}
}

size_t FileManager::nextUntitledNewNumber() const
{
	std::vector <size_t> usedNumbers;
	for(size_t i = 0; i < _buffers.size(); i++) {
		Buffer * buf = _buffers.at(i);
		if(buf->isUntitled()) {
			// if untitled document is invisible, then don't put its number into array (so its number is
			// available to be used)
			if((buf->_referees[0])->isVisible()) {
				generic_string newTitle = ((NppParameters::getInstance()).getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string", UNTITLED_STR);
				TCHAR * numberStr = buf->_fileName + newTitle.length();
				int usedNumber = satoi(numberStr);
				usedNumbers.push_back(usedNumber);
			}
		}
	}
	size_t newNumber = 1;
	bool numberAvailable = true;
	bool found = false;
	do {
		for(size_t j = 0; j < usedNumbers.size(); j++) {
			numberAvailable = true;
			found = false;
			if(usedNumbers[j] == newNumber) {
				numberAvailable = false;
				found = true;
				break;
			}
		}
		if(!numberAvailable)
			newNumber++;
		if(!found)
			break;
	} while(!numberAvailable);
	return newNumber;
}

BufferID FileManager::newEmptyDocument()
{
	generic_string newTitle = ((NppParameters::getInstance()).getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string", UNTITLED_STR);
	TCHAR nb[10];
	wsprintf(nb, TEXT("%d"), static_cast<int>(nextUntitledNewNumber()));
	newTitle += nb;
	Document doc = (Document)_pscratchTilla->execute(SCI_CREATEDOCUMENT);   //this already sets a reference for filemanager
	Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = static_cast<BufferID>(newBuf);
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	++_nbBufs;
	++_nextBufferID;
	return id;
}

BufferID FileManager::bufferFromDocument(Document doc, bool dontIncrease, bool dontRef)
{
	generic_string newTitle = ((NppParameters::getInstance()).getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string",
		UNTITLED_STR);
	TCHAR nb[10];
	wsprintf(nb, TEXT("%d"), static_cast<int>(nextUntitledNewNumber()));
	newTitle += nb;

	if(!dontRef)
		_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, doc);    //set reference for FileManager
	Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str());
	BufferID id = static_cast<BufferID>(newBuf);
	newBuf->_id = id;
	_buffers.push_back(newBuf);
	++_nbBufs;

	if(!dontIncrease)
		++_nextBufferID;
	return id;
}

int FileManager::detectCodepage(char * buf, size_t len)
{
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, buf, len);
	uchardet_data_end(ud);
	const char * cs = uchardet_get_charset(ud);
	int codepage = EncodingMapper::getInstance().getEncodingFromString(cs);
	uchardet_delete(ud);
	return codepage;
}

LangType FileManager::detectLanguageFromTextBegining(const uchar * data, size_t dataLen)
{
	struct FirstLineLanguages {
		std::string pattern;
		LangType lang;
	};

	// Is the buffer at least the size of a BOM?
	if(dataLen <= 3)
		return L_TEXT;

	// Eliminate BOM if present
	size_t i = 0;
	if((data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) ||  // UTF8 BOM
	    (data[0] == 0xFE && data[1] == 0xFF && data[2] == 0x00) ||     // UTF16 BE BOM
	    (data[0] == 0xFF && data[1] == 0xFE && data[2] == 0x00))       // UTF16 LE BOM
		i += 3;

	// Skip any space-like char
	for(; i < dataLen; ++i) {
		if(data[i] != ' ' && data[i] != '\t' && data[i] != '\n' && data[i] != '\r')
			break;
	}

	// Create the buffer to need to test
	const size_t longestLength = 40; // shebangs can be large
	std::string buf2Test = std::string((const char *)data + i, longestLength);

	// Is there a \r or \n in the buffer? If so, truncate it
	auto cr = buf2Test.find("\r");
	auto nl = buf2Test.find("\n");
	auto crnl = MIN(cr, nl);
	if(crnl != std::string::npos && crnl < longestLength)
		buf2Test = std::string((const char *)data + i, crnl);

	// First test for a Unix-like Shebang
	// See https://en.wikipedia.org/wiki/Shebang_%28Unix%29 for more details about Shebang
	std::string shebang = "#!";

	size_t foundPos = buf2Test.find(shebang);
	if(foundPos == 0) {
		// Make a list of the most commonly used languages
		const size_t NB_SHEBANG_LANGUAGES = 6;
		FirstLineLanguages ShebangLangs[NB_SHEBANG_LANGUAGES] = {
			{ "sh",         L_BASH },
			{ "python", L_PYTHON },
			{ "perl",       L_PERL },
			{ "php",        L_PHP },
			{ "ruby",       L_RUBY },
			{ "node",       L_JAVASCRIPT }
		};

		// Go through the list of languages
		for(i = 0; i < NB_SHEBANG_LANGUAGES; ++i) {
			if(buf2Test.find(ShebangLangs[i].pattern) != std::string::npos) {
				return ShebangLangs[i].lang;
			}
		}
		// Unrecognized shebang (there is always room for improvement ;-)
		return L_TEXT;
	}
	// Are there any other patterns we know off?
	const size_t NB_FIRST_LINE_LANGUAGES = 5;
	FirstLineLanguages languages[NB_FIRST_LINE_LANGUAGES] = {
		{ "<?xml",                      L_XML },
		{ "<?php",                      L_PHP },
		{ "<html",                      L_HTML },
		{ "<!DOCTYPE html",     L_HTML },
		{ "<?",                         L_PHP } // MUST be after "<?php" and "<?xml" to get the result as
		                                        // accurate as possible
	};
	for(i = 0; i < NB_FIRST_LINE_LANGUAGES; ++i) {
		foundPos = buf2Test.find(languages[i].pattern);
		if(foundPos == 0) {
			return languages[i].lang;
		}
	}
	// Unrecognized first line, we assume it is a text file for now
	return L_TEXT;
}

bool FileManager::loadFileData(Document doc, const TCHAR * filename, char * data, Utf8_16_Read * unicodeConvertor, LoadedFileFormat& fileFormat)
{
	FILE * fp = generic_fopen(filename, TEXT("rb"));
	if(!fp)
		return false;
	//Get file size
	_fseeki64(fp, 0, SEEK_END);
	unsigned __int64 fileSize = _ftelli64(fp);
	rewind(fp);
	// size/6 is the normal room Scintilla keeps for editing, but here we limit it to 1MiB when loading (maybe we
	// want to load big files without editing them too much)
	unsigned __int64 bufferSizeRequested = fileSize + MIN(1<<20, fileSize/6);
	// As a 32bit application, we cannot allocate 2 buffer of more than INT_MAX size (it takes the whole address
	// space)
	if(bufferSizeRequested > INT_MAX) {
		NativeLangSpeaker * pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		pNativeSpeaker->messageBox("FileTooBigToOpen", NULL, TEXT("File is too big to be opened by Notepad++"),
		    TEXT("File size problem"), MB_OK|MB_APPLMODAL);
		fclose(fp);
		return false;
	}

	//Setup scratchtilla for new filedata
	_pscratchTilla->execute(SCI_SETSTATUS, SC_STATUS_OK); // reset error status
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, doc);
	bool ro = _pscratchTilla->execute(SCI_GETREADONLY) != 0;
	if(ro) {
		_pscratchTilla->execute(SCI_SETREADONLY, false);
	}
	_pscratchTilla->execute(SCI_CLEARALL);

	if(fileFormat._language < L_EXTERNAL) {
		_pscratchTilla->execute(SCI_SETLEXER, ScintillaEditView::langNames[fileFormat._language].lexerID);
	}
	else {
		int id = fileFormat._language - L_EXTERNAL;
		TCHAR * name = NppParameters::getInstance().getELCFromIndex(id)._name;
		WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
		const char * pName = wmc.wchar2char(name, CP_ACP);
		_pscratchTilla->execute(SCI_SETLEXERLANGUAGE, 0, reinterpret_cast<LPARAM>(pName));
	}

	if(fileFormat._encoding != -1)
		_pscratchTilla->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

	bool success = true;
	EolType format = EolType::unknown;
	__try {
		// First allocate enough memory for the whole file (this will reduce memory copy during loading)
		_pscratchTilla->execute(SCI_ALLOCATE, WPARAM(bufferSizeRequested));
		if(_pscratchTilla->execute(SCI_GETSTATUS) != SC_STATUS_OK)
			throw;
		size_t lenFile = 0;
		size_t lenConvert = 0;  //just in case conversion results in 0, but file not empty
		bool isFirstTime = true;
		int incompleteMultibyteChar = 0;

		do {
			lenFile = fread(data+incompleteMultibyteChar, 1, blockSize-incompleteMultibyteChar, fp) + incompleteMultibyteChar;
			if(ferror(fp) != 0) {
				success = false;
				break;
			}
			if(lenFile == 0) 
				break;
			if(isFirstTime) {
				// check if file contain any BOM
				if(Utf8_16_Read::determineEncoding((uchar *)data, lenFile) != uni8Bit) {
					// if file contains any BOM, then encoding will be erased,
					// and the document will be interpreted as UTF
					fileFormat._encoding = -1;
				}
				else if(fileFormat._encoding == -1) {
					if(NppParameters::getInstance().getNppGUI()._detectEncoding)
						fileFormat._encoding = detectCodepage(data, lenFile);
				}
				if(fileFormat._language == L_TEXT) {
					// check the language du fichier
					fileFormat._language = detectLanguageFromTextBegining((uchar *)data, lenFile);
				}
				isFirstTime = false;
			}
			if(fileFormat._encoding != -1) {
				if(fileFormat._encoding == SC_CP_UTF8) {
					// Pass through UTF-8 (this does not check validity of characters, thus
					// inserting a multi-byte character in two halfs is working)
					_pscratchTilla->execute(SCI_APPENDTEXT, lenFile, reinterpret_cast<LPARAM>(data));
				}
				else {
					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
					int newDataLen = 0;
					const char * newData = wmc.encode(fileFormat._encoding, SC_CP_UTF8, data,
						static_cast<int32_t>(lenFile), &newDataLen, &incompleteMultibyteChar);
					_pscratchTilla->execute(SCI_APPENDTEXT, newDataLen, reinterpret_cast<LPARAM>(newData));
				}
				if(format == EolType::unknown)
					format = getEOLFormatForm(data, lenFile, EolType::unknown);
			}
			else {
				lenConvert = unicodeConvertor->convert(data, lenFile);
				_pscratchTilla->execute(SCI_APPENDTEXT, lenConvert, reinterpret_cast<LPARAM>(unicodeConvertor->getNewBuf()));
				if(format == EolType::unknown)
					format = getEOLFormatForm(unicodeConvertor->getNewBuf(), unicodeConvertor->getNewSize(), EolType::unknown);
			}

			if(_pscratchTilla->execute(SCI_GETSTATUS) != SC_STATUS_OK)
				throw;
			if(incompleteMultibyteChar != 0) {
				// copy bytes to next buffer
				memcpy(data, data + blockSize - incompleteMultibyteChar, incompleteMultibyteChar);
			}
		}
		while(lenFile > 0);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) //TODO: should filter correctly for other exceptions; the old
	                                    // filter(GetExceptionCode(), GetExceptionInformation()) was only catching
	                                    // access violations
	{
		NativeLangSpeaker * pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		pNativeSpeaker->messageBox("FileTooBigToOpen", NULL, TEXT("File is too big to be opened by Notepad++"),
		    TEXT("File size problem"), MB_OK | MB_APPLMODAL);
		success = false;
	}

	fclose(fp);

	// broadcast the format
	if(format == EolType::unknown) {
		NppParameters& nppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings(); // for ndds._format
		fileFormat._eolFormat = ndds._format;
		//for empty files, if the default for new files is UTF8, and "Apply to opened ANSI files" is set, apply it
		if(fileSize == 0) {
			if(ndds._unicodeMode == uniCookie && ndds._openAnsiAsUtf8)
				fileFormat._encoding = SC_CP_UTF8;
		}
	}
	else {
		fileFormat._eolFormat = format;
	}
	_pscratchTilla->execute(SCI_EMPTYUNDOBUFFER);
	_pscratchTilla->execute(SCI_SETSAVEPOINT);
	if(ro)
		_pscratchTilla->execute(SCI_SETREADONLY, true);
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
	return success;
}

BufferID FileManager::getBufferFromName(const TCHAR* name)
{
	TCHAR fullpath[MAX_PATH];
	::GetFullPathName(name, MAX_PATH, fullpath, NULL);
	if(_tcschr(fullpath, '~')) {
		::GetLongPathName(fullpath, fullpath, MAX_PATH);
	}
	for(size_t i = 0; i < _buffers.size(); i++) {
		if(OrdinalIgnoreCaseCompareStrings(name, _buffers.at(i)->getFullPathName()) == 0) {
			if(_buffers.at(i)->_referees[0]->isVisible()) {
				return _buffers.at(i)->getID();
			}
		}
	}
	return BUFFER_INVALID;
}

BufferID FileManager::getBufferFromDocument(Document doc)
{
	for(size_t i = 0; i < _nbBufs; ++i) {
		if(_buffers[i]->_doc == doc)
			return _buffers[i]->_id;
	}
	return BUFFER_INVALID;
}

bool FileManager::createEmptyFile(const TCHAR * path)
{
	FILE * file = generic_fopen(path, TEXT("wbc"));
	if(!file)
		return false;
	fflush(file);
	fclose(file);
	return true;
}

int FileManager::getFileNameFromBuffer(BufferID id, TCHAR * fn2copy)
{
	if(getBufferIndexByID(id) == -1)
		return -1;
	Buffer* buf = getBufferByID(id);
	if(fn2copy)
		lstrcpy(fn2copy, buf->getFullPathName());
	return lstrlen(buf->getFullPathName());
}

int FileManager::docLength(Buffer* buffer) const
{
	Document curDoc = _pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);
	int docLen = _pscratchTilla->getCurrentDocLen();
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, curDoc);
	return docLen;
}
