// TEXTBRW.CPP
// Copyright (c) A.Starodub 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
// @codepage UTF-8
// STextBrowser
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <scintilla.h>
#include <scilexer.h>
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
        1. sync: (every N seconds) the backup file will be saved if the buffer is dirty and modifications are present. A
           boolean flag is set to true upon modification notification, and it's set to false after the backup file is
           saved.
        2. sync: each save file, or close file, the backup file will be deleted (if buffer is not dirty).
        3. before switch off to another tab (or close files on exit), check 1 & 2 (sync with backup).

        - Close
        In the current session, Notepad++
        1. track FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist (in session.xml).
        2. track last modified timestamp of FILENAME (c:\tmp\foo.h) if FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776)
           was tracked  (in session.xml).

   For untitled document (new 4)
        - Open
        In the next session, Notepad++
        1. open file UNTITLED_NAME@CREATION_TIMESTAMP (backup\new 4@198776)
        2. set label as UNTITLED_NAME (new 4) and disk icon as red.

        - Editing
        when a untitled document starts being modified, a backup file will be created with name:
           UNTITLED_NAME@CREATION_TIMESTAMP (backup\new 4@198776)
        the Buffer object will associate with this UNTITLED_NAME@CREATION_TIMESTAMP file (backup\new 4@198776).
        1. Sync: (every N seconds) the backup file will be saved if the buffer is dirty and modifications are present. A
           boolean flag is set to true upon modification notification, and it's set to false after the backup file is
           saved.
        2. sync: if untitled document is saved, or closed, the backup file will be deleted.
        3. before switch off to another tab (or close documents on exit), check 1 & 2 (sync with backup).

        - Close
        In the current session, Notepad++
        1. track UNTITLED_NAME@CREATION_TIMESTAMP (backup\new 4@198776) in session.xml.
*/
/*
	Структура состояния редактирования из notepad++

struct Position {
	intptr_t _firstVisibleLine = 0;
	intptr_t _startPos = 0;
	intptr_t _endPos = 0;
	intptr_t _xOffset = 0;
	intptr_t _selMode = 0;
	intptr_t _scrollWidth = 1;
	intptr_t _offset = 0;
	intptr_t _wrapCount = 0;
};

struct MapPosition {
private:
	intptr_t _maxPeekLenInKB = 512; // 512 KB
public:
	intptr_t _firstVisibleDisplayLine = -1;
	intptr_t _firstVisibleDocLine = -1; // map
	intptr_t _lastVisibleDocLine = -1;  // map
	intptr_t _nbLine = -1;              // map
	intptr_t _higherPos = -1;           // map
	intptr_t _width = -1;
	intptr_t _height = -1;
	intptr_t _wrapIndentMode = -1;
	intptr_t _KByteInDoc = _maxPeekLenInKB;

	bool _isWrap = false;
	bool isValid() const { return (_firstVisibleDisplayLine != -1); };
	bool canScroll() const { return (_KByteInDoc < _maxPeekLenInKB); }; // _nbCharInDoc < _maxPeekLen : Don't scroll the document for the performance issue
};

struct sessionFileInfo : public Position {
	sessionFileInfo(const wchar_t* fn, const wchar_t * ln, int encoding, bool userReadOnly,
	    bool isPinned, bool isUntitleTabRenamed, const Position & pos, const wchar_t * backupFilePath,
	    FILETIME originalFileLastModifTimestamp, const MapPosition & mapPos) :
		Position(pos), _encoding(encoding), _isUserReadOnly(userReadOnly), _isPinned(isPinned), _isUntitledTabRenamed(isUntitleTabRenamed), _originalFileLastModifTimestamp(
			originalFileLastModifTimestamp), _mapPos(mapPos)
	{
		if(fn)  
			_fileName = fn;
		if(ln)  
			_langName = ln;
		if(backupFilePath)  
			_backupFilePath = backupFilePath;
	}
	sessionFileInfo(const std::wstring& fn) : _fileName(fn) 
	{
	}
	std::wstring _fileName;
	std::wstring _langName;
	std::vector<size_t> _marks;
	std::vector<size_t> _foldStates;
	int    _encoding = -1;
	bool   _isUserReadOnly = false;
	bool   _isMonitoring = false;
	int    _individualTabColour = -1;
	bool   _isRTL = false;
	bool   _isPinned = false;
	bool   _isUntitledTabRenamed = false;
	std::wstring _backupFilePath;
	FILETIME _originalFileLastModifTimestamp {};
	MapPosition _mapPos;
};
*/
/*
	SCN_UPDATEUI SCN_MODIFIED события, которые надо отслеживать для засечки необходимости сохранить состояние //
*/ 
int SScEditorBase::MakeBackupPath(const Document & rDoc, int storagesubj, SString & rBuf) // @v12.5.11
{
	rBuf.Z();
	int    ok = 0;
	SString temp_buf;
	SString base_path;
	SString file_name;
	//if(GetKnownFolderPath(UED_FSKNOWNFOLDER_LOCAL_APP_DATA, local_path)) { // @v11.8.5
	if(Cfg.BackupPath.NotEmpty()) {
		if(SFile::IsDir(Cfg.BackupPath)) {
			base_path = Cfg.BackupPath;
		}
		else {
			if(SFile::CreateDir(Cfg.BackupPath)) {
				base_path = Cfg.BackupPath;
			}
		}
	}
	if(base_path.IsEmpty()) {
		// UED_FSKNOWNFOLDER_LOCAL_APP_DATA - C:\Users\USERNAME\AppData\Local 
		// UED_FSKNOWNFOLDER_LOCAL_APP_DATA_LOW - C:\Users\USERNAME\AppData\LocalLow
		// UED_FSKNOWNFOLDER_ROAMING_APP_DATA - C:\Users\USERNAME\AppData\Roaming
		GetKnownFolderPath(UED_FSKNOWNFOLDER_ROAMING_APP_DATA, base_path);
		if(SFile::IsDir(base_path)) {
			const char * p_app_name = SLS.GetAppName();
			if(!isempty(p_app_name)) {
				base_path.SetLastSlash().Cat(p_app_name);
			}
			base_path.SetLastSlash().Cat("TEdSt");
			{
				DbProvider * p_dict = CurDict;
				if(p_dict && p_dict->GetDbSymb(temp_buf)) {
					base_path.SetLastSlash().Cat(temp_buf);
				}
			}
			if(SFile::CreateDir(base_path)) {
				;
			}
			else
				base_path.Z();
		}
	}
	if(base_path.NotEmpty()) {
		SString name_symb;
		if(rDoc.MakeNameSymbol(storagesubj, false/*withoutExt*/, name_symb)) {
			rBuf.Cat(base_path).SetLastSlash().Cat(name_symb);
			ok = 1;
		}
	}
	return ok;
}

bool SScEditorBase::StoreText(const char * pDestFileName)
{
	bool   ok = true;
	if(!isempty(pDestFileName)) {
		const  uint8 * p_buf = reinterpret_cast<const uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER, 0, 0)); // to get characters directly from Scintilla buffer;
		const  size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
		SFile  file;
		THROW_SL(file.Open(pDestFileName, SFile::mWrite|SFile::mBinary));
		if(Doc.OrgCp == Doc.Cp) {
			THROW_SL(file.Write(p_buf, len));
		}
		else if(Doc.Cp == cpUTF8) {
			SString temp_buf;
			temp_buf.CatN(reinterpret_cast<const char *>(p_buf), len);
			temp_buf.Utf8ToCp(Doc.OrgCp);
			THROW_SL(file.Write(temp_buf, temp_buf.Len()));
		}
		else {
			THROW_SL(file.Write(p_buf, len));
		}
	}
	else
		ok = false;
	CATCHZOK
	return ok;
}

bool SScEditorBase::StoreState(const char * pDestFileName)
{
	bool   ok = true;
	SJson * p_js = 0;
	if(!isempty(pDestFileName)) {
		SString temp_buf;
		SScEditorTextInfo state;
		THROW(GetCurrentState(state));
		p_js = state.ToJsonObj();
		THROW_SL(p_js);
		THROW_SL(p_js->ToStr(temp_buf));
		{
			SFile  file;
			THROW_SL(file.Open(pDestFileName, SFile::mWrite|SFile::mBinary));
			THROW_SL(file.Write(temp_buf, temp_buf.Len()));
		}
	}
	else
		ok = false;
	CATCHZOK
	delete p_js;
	return ok;
}

int SScEditorBase::DoBackup(int storagesubj) // @v12.5.11 @construction
{
	int    ok = -1;
	SString backup_path;
	if(storagesubj == storagesubjText) {
		THROW(MakeBackupPath(Doc, storagesubjText, backup_path));
		THROW(StoreText(backup_path));
	}
	if(storagesubj == storagesubjState) {
		THROW(MakeBackupPath(Doc, storagesubjState, backup_path));
		THROW(StoreState(backup_path));
	}
	CATCHZOK
	return ok;
}

void SScEditorBase::RegisterEditEvent()
{
	const  LDATETIME now_dtm = getcurdatetime_();
	ASSt.LastEditEventMoment = now_dtm;
	Doc.SetState(Document::stDirty, 1);
}

void SScEditorBase::RegisterStateEvent()
{
	const  LDATETIME now_dtm = getcurdatetime_();
	ASSt.LastUiEventMoment = now_dtm;
}

bool SScEditorBase::IsFolded(int64 line) 
{ 
	return (CallFunc(SCI_GETFOLDEXPANDED, line) != 0); 
}

void SScEditorBase::Fold(int64 line, bool mode, bool shouldBeNotified)
{
	int    end_styled = CallFunc(SCI_GETENDSTYLED);
	int    len = CallFunc(SCI_GETTEXTLENGTH);
	if(end_styled < len)
		CallFunc(SCI_COLOURISE, 0, -1);
	int32  header_line;
	int    level = CallFunc(SCI_GETFOLDLEVEL, line);
	if(level & SC_FOLDLEVELHEADERFLAG)
		header_line = static_cast<int32>(line);
	else {
		header_line = static_cast<int32>(CallFunc(SCI_GETFOLDPARENT, line));
		if(header_line == -1)
			return;
	}
	if(IsFolded(header_line) != mode) {
		CallFunc(SCI_TOGGLEFOLD, header_line);
		if(shouldBeNotified) {
			SCNotification scnN;
			scnN.nmhdr.code = SCN_FOLDINGSTATECHANGED;
			scnN.nmhdr.hwndFrom = H_Window;
			scnN.nmhdr.idFrom = 0;
			scnN.line = header_line;
			scnN.foldLevelNow = IsFolded(header_line) ? 1 : 0; // folded:1, unfolded:0
			::SendMessageW(/*_hParent*/GetParent(H_Window), WM_NOTIFY, 0, reinterpret_cast<LPARAM>(&scnN));
		}
	}
}

int SScEditorBase::GetCurrentFoldStateList(Int64Array & rList)
{
	rList.clear();
	int    ok = -1;
	if(P_SciFn && P_SciPtr) {
		// xCodeOptimization1304: For active document get folding state from Scintilla.
		// The code using SCI_CONTRACTEDFOLDNEXT is usually 10%-50% faster than checking each line of the document!!
		int64  contracted_fold_header_line = 0;
		do {
			contracted_fold_header_line = CallFunc(SCI_CONTRACTEDFOLDNEXT, contracted_fold_header_line);
			if(contracted_fold_header_line != -1) {
				//-- Store contracted line
				rList.add(contracted_fold_header_line);
				//-- Start next search with next line
				++contracted_fold_header_line;
				ok = 1;
			}
		} while(contracted_fold_header_line != -1);
	}
	else
		ok = 0;
	return ok;
}

int SScEditorBase::ApplyFoldStateList(const Int64Array & rList)
{
	int    ok = -1;
	if(P_SciFn && P_SciPtr) {
		const  int64 nb_line_state = rList.getCount();
		if(nb_line_state > 0) {
			if(nb_line_state > MAX_FOLD_LINES_MORE_THAN) { // Block WM_SETREDRAW messages
				::SendMessageW(H_Window, WM_SETREDRAW, FALSE, 0);
			}
			for(size_t i = 0; i < nb_line_state; ++i) {
				const  int64 line = rList.at(i);
				Fold(line, false/*fold_collapse*/, false/*shouldBeNotified*/);
			}
			if(nb_line_state > MAX_FOLD_LINES_MORE_THAN) {
				::SendMessageW(H_Window, WM_SETREDRAW, TRUE, 0);
				CallFunc(SCI_SCROLLCARET);
				::InvalidateRect(H_Window, nullptr, TRUE);
			}
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int SScEditorBase::GetCurrentState(SScEditorTextInfo & rResult)
{
	rResult.Z();
	int    ok = 1;
	THROW(GetCurrentPosition(rResult));
	THROW(GetCurrentSnapshot(rResult.MapPos));
	{
		{
			const int64 max_line = static_cast<int64>(CallFunc(SCI_GETLINECOUNT));
			for(int64 j = 0; j < max_line; ++j) {
				const int64 iter_marker = CallFunc(SCI_MARKERGET, j);
				if(iter_marker & (1 << MARK_BOOKMARK)) {
					rResult.MarkList.add(j);
				}
			}
		}
		GetCurrentFoldStateList(rResult.FoldStateList);
		// @tod Остальные атрибут (искать аналогии в notepad++)
	}
	CATCHZOK
	return ok;
}

int SScEditorBase::ApplyPreservedState(SScEditorTextInfo & rSet)
{
	int    ok = -1;
	SETIFZQ(P_RestorePosBlk, new RestorePositionBlock);
	if(P_RestorePosBlk) {
		P_RestorePosBlk->RestorePositionRetryCount = 0;
		P_RestorePosBlk->Flags = 0;
		THROW(RestoreCurrentPosition_PreStep(rSet, *P_RestorePosBlk));
	}
	{
		{
			for(uint i = 0; i < rSet.MarkList.getCount(); i++) {
				const  int64 mark_state = rSet.MarkList.get(i);
				CallFunc(SCI_MARKERADD, mark_state, MARK_BOOKMARK);
			}
		}
		ApplyFoldStateList(rSet.FoldStateList);
	}
	CATCHZOK
	return ok;
}

int64 SScEditorBase::GetDocSize()
{
	int64  result = 0;
	if(P_SciFn && P_SciPtr) {
		result = CallFunc(SCI_GETLENGTH);
	}
	return result;
}

int SScEditorBase::SetWrapMode(bool willBeWrapped)
{
	int    ok = 1;
	if(P_SciFn && P_SciPtr) {
		CallFunc(SCI_SETWRAPMODE, willBeWrapped);
	}
	else
		ok = 0;
	return ok;
}

int SScEditorBase::GetWrapMode()
{ 
	int    result = 0;
	if(P_SciFn && P_SciPtr) {
		result = CallFunc(SCI_GETWRAPMODE); //(execute(SCI_GETWRAPMODE) == SC_WRAP_WORD); 
	}
	return result;
}

void SScEditorBase::Scroll(int64 column, int64 line)
{
	CallFunc(SCI_LINESCROLL, column, line);
}

int  SScEditorBase::GetTextZoneWidth()
{
	int    result = 0;
	TRect  editor_rect;
	if(GetRectCli(editor_rect)) {
		intptr_t margin_widths = 0;
		for(int m = 0; m < 4; ++m) {
			margin_widths += CallFunc(SCI_GETMARGINWIDTHN, m);
		}
		result = (editor_rect.width() - static_cast<int>(margin_widths));
	}
	return result;
}

bool SScEditorBase::GetRectCli(TRect & rR)
{
	bool   ok = true;
	RECT   r;
	if(::GetClientRect(H_Window, &r)) {
		rR = r;
	}
	else {
		rR.Z();
		ok = false;
	}
	return ok;
}

bool SScEditorBase::GetRectWin(TRect & rR)
{
	bool   ok = true;
	RECT   r;
	if(::GetWindowRect(H_Window, &r)) {
		rR = r;
	}
	else {
		rR.Z();
		ok = false;
	}
	return ok;
}

int SScEditorBase::GetCurrentSnapshot(SScEditorMapPosition & rResult) // @construction
{
	// by-example-of: notepad++ DocumentPeeker::saveCurrentSnapshot
	int    ok = 1;
	const  bool is_wrap = (GetWrapMode() == SC_WRAP_WORD);
	//Buffer * buffer = editView.getCurrentBuffer();
	//MapPosition mapPos = buffer->getMapPosition();
	// First visible document line for scrolling to this line
	rResult.FirstVisibleDisplayLine = CallFunc(SCI_GETFIRSTVISIBLELINE);
	rResult.FirstVisibleDocLine = CallFunc(SCI_DOCLINEFROMVISIBLE, rResult.FirstVisibleDisplayLine);
	rResult.NbLine = CallFunc(SCI_LINESONSCREEN, rResult.FirstVisibleDisplayLine);
	rResult.LastVisibleDocLine = CallFunc(SCI_DOCLINEFROMVISIBLE, rResult.FirstVisibleDisplayLine + rResult.NbLine);

	auto line_height = CallFunc(SCI_TEXTHEIGHT, rResult.FirstVisibleDocLine);
	rResult.Height = rResult.NbLine * line_height;
	//
	TRect editor_rect;
	GetRectCli(editor_rect);
	intptr_t margin_widths = 0;
	for(int m = 0; m < 4; ++m) {
		margin_widths += CallFunc(SCI_GETMARGINWIDTHN, m);
	}
	double edit_view_width  = editor_rect.width() - static_cast<LONG>(margin_widths);
	double edit_view_height = editor_rect.height();
	rResult.Width = static_cast<int64>((edit_view_width / edit_view_height) * static_cast<double>(rResult.Height));
	rResult.WrapIndentMode = CallFunc(SCI_GETWRAPINDENTMODE);
	SETFLAG(rResult.Flags, SScEditorMapPosition::fIsWrap, /*editView.isWrap()*/is_wrap);
	if(/*editView.isWrap()*/is_wrap) {
		rResult.HigherPos = CallFunc(SCI_POSITIONFROMPOINT, 0, 0);
	}
	rResult.KByteInDoc = GetDocSize() / 1024; // Length of document
	//buffer->setMapPosition(rResult); // set current map position in buffer
	return ok;
}

int SScEditorBase::ScrollSnapshotWith(const SScEditorMapPosition & rMapPos, int textZoneWidth) // @construction
{
	int    ok = 1;
	/*if(_pPeekerView)*/ {
		TRect  wrect; // судя по исходникам notepad++, _rc - полная область окна (не клиентская). Где я использую wrect было _rc
		bool   has_been_changed = false;
		GetRectWin(wrect);
		//
		// if window size has been changed, resize windows
		//
		if(rMapPos.Height != -1 && /*_rc.bottom*/wrect.b.y != (/*_rc.top*/wrect.a.y + rMapPos.Height)) {
			//_rc.bottom = _rc.top + static_cast<LONG>(rMapPos.Height);
			wrect.b.y = wrect.a.y + static_cast<int16>(rMapPos.Height);
			has_been_changed = true;
		}
		if(rMapPos.Width != -1 && /*_rc.right*/wrect.b.x != (/*_rc.left*/wrect.a.x + rMapPos.Width)) {
			//_rc.right = _rc.left + static_cast<LONG>(rMapPos.Width);
			wrect.b.x = wrect.a.x + static_cast<int16>(rMapPos.Width);
			has_been_changed = true;
		}
		if(has_been_changed)
			::MoveWindow(H_Window, 0, 0, /*_rc.right - _rc.left*/wrect.width(), /*_rc.bottom - _rc.top*/wrect.height(), TRUE);
		//
		// Wrapping
		//
		SetWrapMode(LOGIC(rMapPos.Flags & SScEditorMapPosition::fIsWrap));
		CallFunc(SCI_SETWRAPINDENTMODE, rMapPos.WrapIndentMode);
		//
		// Add padding
		//
		/*
		const ScintillaViewParams & svp = NppParameters::getInstance().getSVP();
		if(svp._paddingLeft || svp._paddingRight) {
			int docPeekerWidth = GetTextZoneWidth();
			int paddingMapLeft = static_cast<int>(svp._paddingLeft / (textZoneWidth / docPeekerWidth));
			int paddingMapRight = static_cast<int>(svp._paddingRight / (textZoneWidth / docPeekerWidth));
			CallFunc(SCI_SETMARGINLEFT, 0, paddingMapLeft);
			CallFunc(SCI_SETMARGINRIGHT, 0, paddingMapRight);
		}
		*/
		//
		// Reset to zero
		//
		CallFunc(SCI_HOMEDISPLAY);
		//
		// Visible line for the code view
		//
		// scroll to the first visible display line
		CallFunc(SCI_LINESCROLL, 0, rMapPos.FirstVisibleDisplayLine);
	}
	return ok;
}
/*
void DocumentPeeker::scrollSnapshotWith(const MapPosition & mapPos, int textZoneWidth)
{
	if(_pPeekerView) {
		bool hasBeenChanged = false;
		//
		// if window size has been changed, resize windows
		//
		if(mapPos._height != -1 && _rc.bottom != _rc.top + mapPos._height) {
			_rc.bottom = _rc.top + static_cast<LONG>(mapPos._height);
			hasBeenChanged = true;
		}
		if(mapPos._width != -1 && _rc.right != _rc.left + mapPos._width) {
			_rc.right = _rc.left + static_cast<LONG>(mapPos._width);
			hasBeenChanged = true;
		}

		if(hasBeenChanged)
			::MoveWindow(_pPeekerView->getHSelf(), 0, 0, _rc.right - _rc.left, _rc.bottom - _rc.top, TRUE);
		//
		// Wrapping
		//
		_pPeekerView->wrap(mapPos._isWrap);
		_pPeekerView->execute(SCI_SETWRAPINDENTMODE, mapPos._wrapIndentMode);
		//
		// Add padding
		//
		const ScintillaViewParams& svp = NppParameters::getInstance().getSVP();
		if(svp._paddingLeft || svp._paddingRight) {
			int docPeekerWidth = _pPeekerView->getTextZoneWidth();
			int paddingMapLeft = static_cast<int>(svp._paddingLeft / (textZoneWidth / docPeekerWidth));
			int paddingMapRight = static_cast<int>(svp._paddingRight / (textZoneWidth / docPeekerWidth));
			_pPeekerView->execute(SCI_SETMARGINLEFT, 0, paddingMapLeft);
			_pPeekerView->execute(SCI_SETMARGINRIGHT, 0, paddingMapRight);
		}
		//
		// Reset to zero
		//
		_pPeekerView->execute(SCI_HOMEDISPLAY);
		//
		// Visible line for the code view
		//
		// scroll to the first visible display line
		_pPeekerView->execute(SCI_LINESCROLL, 0, mapPos._firstVisibleDisplayLine);
	}
}

 void DocumentPeeker::saveCurrentSnapshot(ScintillaEditView & editView)
{
	if(_pPeekerView) {
		Buffer * buffer = editView.getCurrentBuffer();
		MapPosition mapPos = buffer->getMapPosition();
		// First visible document line for scrolling to this line
		mapPos._firstVisibleDisplayLine = editView.execute(SCI_GETFIRSTVISIBLELINE);
		mapPos._firstVisibleDocLine = editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine);
		mapPos._nbLine = editView.execute(SCI_LINESONSCREEN, mapPos._firstVisibleDisplayLine);
		mapPos._lastVisibleDocLine = editView.execute(SCI_DOCLINEFROMVISIBLE, mapPos._firstVisibleDisplayLine + mapPos._nbLine);

		auto lineHeight = _pPeekerView->execute(SCI_TEXTHEIGHT, mapPos._firstVisibleDocLine);
		mapPos._height = mapPos._nbLine * lineHeight;

		// Width
		RECT editorRect;
		editView.getClientRect(editorRect);
		intptr_t marginWidths = 0;
		for(int m = 0; m < 4; ++m) {
			marginWidths += editView.execute(SCI_GETMARGINWIDTHN, m);
		}
		double editViewWidth = editorRect.right - editorRect.left - static_cast<LONG>(marginWidths);
		double editViewHeight = editorRect.bottom - editorRect.top;
		mapPos._width = static_cast<intptr_t>((editViewWidth / editViewHeight) * static_cast<double>(mapPos._height));
		mapPos._wrapIndentMode = editView.execute(SCI_GETWRAPINDENTMODE);
		mapPos._isWrap = editView.isWrap();
		if(editView.isWrap()) {
			mapPos._higherPos = editView.execute(SCI_POSITIONFROMPOINT, 0, 0);
		}
		mapPos._KByteInDoc = editView.getCurrentDocLen() / 1024; // Length of document
		buffer->setMapPosition(mapPos); // set current map position in buffer
	}
}
*/
//
// Descr: restore current position is executed in two steps.
//   The detection wrap state done in the pre step function:
//   if wrap is enabled, then _positionRestoreNeeded is activated
//   so post step function will be called in the next SCN_PAINTED message
//
int SScEditorBase::RestoreCurrentPosition_PreStep(const SScEditorPosition & rPos, RestorePositionBlock & rBlk)
{
	int    ok = 1;
	if(P_SciFn && P_SciPtr) {
		//Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);
		//const Position & pos = buf->getPosition(this);
		CallFunc(SCI_SETSELECTIONMODE, rPos.SelMode);    //enable
		CallFunc(SCI_SETANCHOR, rPos.StartPos);
		CallFunc(SCI_SETCURRENTPOS, rPos.EndPos);
		CallFunc(SCI_CANCEL); //disable
		//if(!isWrap()) { //only offset if not wrapping, otherwise the offset isn't needed at all
		if(GetWrapMode() != SC_WRAP_WORD) {
			CallFunc(SCI_SETSCROLLWIDTH, rPos.ScrollWidth);
			CallFunc(SCI_SETXOFFSET, rPos.XOffset);
		}
		CallFunc(SCI_CHOOSECARETX); // choose current x position
		int64  line_to_show = CallFunc(SCI_VISIBLEFROMDOCLINE, rPos.FirstVisibleLine);
		CallFunc(SCI_SETFIRSTVISIBLELINE, line_to_show);
		//if(isWrap()) {
		if(GetWrapMode() == SC_WRAP_WORD) {
			// Enable flag 'positionRestoreNeeded' so that function restoreCurrentPosPostStep get called
			// once scintilla send SCN_PAINTED notification
			//_positionRestoreNeeded = true;
			rBlk.Flags |= RestorePositionBlock::fPosRestoreNeeded;
			ok = 100;
		}
		//_restorePositionRetryCount = 0;
		rBlk.RestorePositionRetryCount = 0;
	}
	else
		ok = 0;
	return ok;
}
//
// Descr: If wrap is enabled, the post step function will be called in the next SCN_PAINTED message
//   to scroll several lines to set the first visible line to the correct wrapped line.
//
#if 1 // @construction {
int SScEditorBase::RestoreCurrentPosition_PostStep(const SScEditorPosition & rPos, RestorePositionBlock & rBlk)
{
	int    ok = 1;
	if(P_SciFn && P_SciPtr) {
		//if(!_positionRestoreNeeded)
			//return;
		if(rBlk.Flags & RestorePositionBlock::fPosRestoreNeeded) {
			//Buffer * buf = MainFileManager.getBufferByID(_currentBufferID);
			//const Position & pos = buf->getPosition(this);
			//++_restorePositionRetryCount;
			rBlk.RestorePositionRetryCount++;
			// Scintilla can send several SCN_PAINTED notifications before the buffer is ready to be displayed.
			// this post step function is therefore iterated several times in a maximum of 8 iterations.
			// 8 is an arbitrary number. 2 is a minimum. Maximum value is unknown.
			if(rBlk.RestorePositionRetryCount > 8) {
				// Abort the position restoring  process. Buffer topology may have changed
				//_positionRestoreNeeded = false;
				rBlk.Flags &= ~RestorePositionBlock::fPosRestoreNeeded;
			}
			else {
				const  int64 displayed_line = CallFunc(SCI_GETFIRSTVISIBLELINE);
				const  int64 doc_line = CallFunc(SCI_DOCLINEFROMVISIBLE, displayed_line); //linenumber of the line displayed in the
				// check docLine must equals saved position
				if(doc_line != rPos.FirstVisibleLine) {
					// Scintilla has paint the buffer but the position is not correct.
					const  int64 line_to_show = CallFunc(SCI_VISIBLEFROMDOCLINE, rPos.FirstVisibleLine);
					CallFunc(SCI_SETFIRSTVISIBLELINE, line_to_show);
				}
				else if(rPos.Offset > 0) {
					// don't scroll anything if the wrap count is different than the saved one.
					// Buffer update may be in progress (in case wrap is enabled)
					const  int64 wrap_count = CallFunc(SCI_WRAPCOUNT, doc_line);
					if(wrap_count == rPos.WrapCount) {
						Scroll(0, rPos.Offset);
						//_positionRestoreNeeded = false;
						rBlk.Flags &= ~RestorePositionBlock::fPosRestoreNeeded;
					}
				}
				else {
					// Buffer position is correct, and there is no scroll to apply
					//_positionRestoreNeeded = false;
					rBlk.Flags &= ~RestorePositionBlock::fPosRestoreNeeded;
				}
			}
		}
		else
			ok = -1;
	}
	else
		ok = 0;
	return ok;
}
#endif // } 0 @construction

int SScEditorBase::GetCurrentPosition(SScEditorPosition & rResult)
{
	rResult.Z();
	int    ok = 1;
	if(P_SciFn && P_SciPtr) {
		int64  displayed_line = CallFunc(SCI_GETFIRSTVISIBLELINE);
		int64  doc_line = CallFunc(SCI_DOCLINEFROMVISIBLE, displayed_line); //linenumber of the line displayed in the top
		int64  offset = displayed_line - CallFunc(SCI_VISIBLEFROMDOCLINE, doc_line); //use this to calc offset of wrap. If no wrap this should be zero
		int64  wrap_count = CallFunc(SCI_WRAPCOUNT, doc_line);
		// the correct visible line number
		rResult.FirstVisibleLine = doc_line;
		rResult.StartPos = CallFunc(SCI_GETANCHOR);
		rResult.EndPos = CallFunc(SCI_GETCURRENTPOS);
		rResult.XOffset = CallFunc(SCI_GETXOFFSET);
		rResult.SelMode = CallFunc(SCI_GETSELECTIONMODE);
		rResult.ScrollWidth = CallFunc(SCI_GETSCROLLWIDTH);
		rResult.Offset = offset;
		rResult.WrapCount = wrap_count;
	}
	else
		ok = 0;
	return ok;
}

SScEditorPosition::SScEditorPosition() : FirstVisibleLine(0), StartPos(0), EndPos(0), XOffset(0), SelMode(0), ScrollWidth(1), Offset(0), WrapCount(0)
{
}

SScEditorPosition & SScEditorPosition::Z()
{
	FirstVisibleLine = 0;
	StartPos = 0;
	EndPos = 0;
	XOffset = 0;
	SelMode = 0;
	ScrollWidth = 1;
	Offset = 0;
	WrapCount = 0;
	return *this;
}

SJson * SScEditorPosition::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	if(SJson::IsObject(p_result)) {
		if(FirstVisibleLine)
			p_result->InsertInt64("FirstVisLn", FirstVisibleLine);
		if(StartPos)
			p_result->InsertInt64("Start", StartPos);
		if(EndPos)
			p_result->InsertInt64("End", EndPos);
		if(XOffset)
			p_result->InsertInt64("XOffs", XOffset);
		if(SelMode)
			p_result->InsertInt64("SelMode", SelMode);
		if(ScrollWidth != 1)
			p_result->InsertInt64("ScrollWidth", ScrollWidth);
		if(Offset)
			p_result->InsertInt64("Offs", Offset);
		if(WrapCount)
			p_result->InsertInt64("WrapCt", WrapCount);
	}
	return p_result;
}

bool SScEditorPosition::FromJsonObj(const SJson * pJs)
{
	bool   ok = true;
	Z();
	for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Text.IsEqiAscii("FirstVisLn")) {
			SJson::GetChildInt64(p_cur, FirstVisibleLine);
		}
		else if(p_cur->Text.IsEqiAscii("Start")) {
			SJson::GetChildInt64(p_cur, StartPos);
		}
		else if(p_cur->Text.IsEqiAscii("End")) {
			SJson::GetChildInt64(p_cur, EndPos);
		}
		else if(p_cur->Text.IsEqiAscii("XOffs")) {
			SJson::GetChildInt64(p_cur, XOffset);
		}
		else if(p_cur->Text.IsEqiAscii("SelMode")) {
			SJson::GetChildInt64(p_cur, SelMode);
		}
		else if(p_cur->Text.IsEqiAscii("ScrollWidth")) {
			SJson::GetChildInt64(p_cur, ScrollWidth);
		}
		else if(p_cur->Text.IsEqiAscii("Offs")) {
			SJson::GetChildInt64(p_cur, Offset);
		}
		else if(p_cur->Text.IsEqiAscii("WrapCt")) {
			SJson::GetChildInt64(p_cur, WrapCount);
		}
	}
	return ok;
}

SScEditorMapPosition::SScEditorMapPosition() : FirstVisibleDisplayLine(-1), FirstVisibleDocLine(-1), LastVisibleDocLine(-1), NbLine(-1),
	HigherPos(-1), Width(-1), Height(-1), WrapIndentMode(-1), KByteInDoc(MaxPeekLenInKB), Flags(0)
{
}

SScEditorMapPosition & SScEditorMapPosition::Z()
{
	FirstVisibleDisplayLine = -1;
	FirstVisibleDocLine = -1;
	LastVisibleDocLine = -1;
	NbLine = -1;
	HigherPos = -1;
	Width = -1;
	Height = -1;
	WrapIndentMode = -1;
	KByteInDoc = MaxPeekLenInKB;
	Flags = 0;
	return *this;
}
	
SJson * SScEditorMapPosition::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	if(SJson::IsObject(p_result)) {
		if(FirstVisibleDisplayLine != -1)
			p_result->InsertInt64("FirstVisDispLn", FirstVisibleDisplayLine);
		if(FirstVisibleDocLine != -1)
			p_result->InsertInt64("FirstVisDocLn", FirstVisibleDocLine);
		if(LastVisibleDocLine != -1)
			p_result->InsertInt64("LastVisDocLn", LastVisibleDocLine);
		if(NbLine != -1)
			p_result->InsertInt64("NbLn", NbLine);
		if(HigherPos != -1)
			p_result->InsertInt64("HigherPos", HigherPos);
		if(Width != -1)
			p_result->InsertInt64("Width", Width);
		if(Height != -1)
			p_result->InsertInt64("Height", Height);
		if(WrapIndentMode != -1)
			p_result->InsertInt64("WrapIndentMode", WrapIndentMode);
		if(KByteInDoc != MaxPeekLenInKB)
			p_result->InsertInt64("KByteInDoc", KByteInDoc);
		if(Flags != 0)
			p_result->InsertUInt("Flags", Flags);
	}
	return p_result;
}

bool SScEditorMapPosition::FromJsonObj(const SJson * pJs)
{
	bool   ok = true;
	Z();
	for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Text.IsEqiAscii("FirstVisDispLn")) {
			SJson::GetChildInt64(p_cur, FirstVisibleDisplayLine);
		}
		else if(p_cur->Text.IsEqiAscii("FirstVisDocLn")) {
			SJson::GetChildInt64(p_cur, FirstVisibleDocLine);
		}
		else if(p_cur->Text.IsEqiAscii("LastVisDocLn")) {
			SJson::GetChildInt64(p_cur, LastVisibleDocLine);
		}
		else if(p_cur->Text.IsEqiAscii("NbLn")) {
			SJson::GetChildInt64(p_cur, NbLine);
		}
		else if(p_cur->Text.IsEqiAscii("HigherPos")) {
			SJson::GetChildInt64(p_cur, HigherPos);
		}
		else if(p_cur->Text.IsEqiAscii("Width")) {
			SJson::GetChildInt64(p_cur, Width);
		}
		else if(p_cur->Text.IsEqiAscii("Height")) {
			SJson::GetChildInt64(p_cur, Height);
		}
		else if(p_cur->Text.IsEqiAscii("WrapIndentMode")) {
			SJson::GetChildInt64(p_cur, WrapIndentMode);
		}
		else if(p_cur->Text.IsEqiAscii("KByteInDoc")) {
			SJson::GetChildInt64(p_cur, KByteInDoc);
		}
		else if(p_cur->Text.IsEqiAscii("Flags")) {
			SJson::GetChildUInt(p_cur, Flags);
		}
	}
	return ok;
}

SScEditorTextInfo::SScEditorTextInfo() : SScEditorPosition(), Lingua(0), Cp(-1), IndividualTabColour(-1), Flags(0), UedOrgFileModTm(0)
{
}

SScEditorTextInfo & SScEditorTextInfo::Z()
{
	SScEditorPosition::Z();
	Lingua = 0;
	Cp = -1;
	IndividualTabColour = -1;
	Flags = 0;
	UedOrgFileModTm = 0;
	MapPos.Z();
	FileNameUtf8.Z();
	BackupPathUtf8.Z();
	MarkList.clear();
	FoldStateList.clear();
	return *this;
}
	
SJson * SScEditorTextInfo::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	if(SJson::IsObject(p_result)) {
		{
			SJson * p_js_inner = SScEditorPosition::ToJsonObj();
			if(p_js_inner) {
				p_result->Insert("Position", p_js_inner);
			}
		}
		if(Lingua != 0)
			p_result->InsertInt("Lingua", Lingua);
		if(Cp != -1)
			p_result->InsertInt("Cp", Cp);
		if(IndividualTabColour != -1)
			p_result->InsertInt("IndTabClr", IndividualTabColour);
		if(Flags != 0)
			p_result->InsertUInt("Flags", Flags);
		if(UedOrgFileModTm != 0)
			p_result->InsertUInt("UedOrgFileModTm", UedOrgFileModTm);
		{
			SJson * p_js_inner = MapPos.ToJsonObj();
			if(p_js_inner) {
				p_result->Insert("MapPos", p_js_inner);
			}
		}
		if(FileNameUtf8.NotEmpty()) {
			p_result->InsertString("FileName", FileNameUtf8);
		}
		if(BackupPathUtf8.NotEmpty()) {
			p_result->InsertString("BackupPath", BackupPathUtf8);
		}
		if(MarkList.getCount()) {
			SJson * p_js_inner = SJson::CreateArr();
			if(p_js_inner) {
				for(uint i = 0; i < MarkList.getCount(); i++) {
					p_js_inner->InsertChild(SJson::CreateInt64(MarkList.get(i)));
				}
				p_result->Insert("MarkList", p_js_inner);
			}
		}
		if(FoldStateList.getCount()) {
			SJson * p_js_inner = SJson::CreateArr();
			if(p_js_inner) {
				for(uint i = 0; i < FoldStateList.getCount(); i++) {
					p_js_inner->InsertChild(SJson::CreateInt64(FoldStateList.get(i)));
				}
				p_result->Insert("FoldStateList", p_js_inner);
			}
		}
	}
	return p_result;
}
	
bool SScEditorTextInfo::FromJsonObj(const SJson * pJs)
{
	bool   ok = true;
	SScEditorTextInfo::Z();
	for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Text.IsEqiAscii("Position")) {
			if(SJson::IsObject(p_cur->P_Child)) { 
				SScEditorPosition::FromJsonObj(p_cur->P_Child);
			}
		}
		else if(p_cur->Text.IsEqiAscii("Lingua")) {
			SJson::GetChildInt(p_cur, Lingua);
		}
		else if(p_cur->Text.IsEqiAscii("Cp")) {
			SJson::GetChildInt(p_cur, Cp);
		}
		else if(p_cur->Text.IsEqiAscii("IndTabClr")) {
			SJson::GetChildInt(p_cur, IndividualTabColour);
		}
		else if(p_cur->Text.IsEqiAscii("Flags")) {
			SJson::GetChildUInt(p_cur, Flags);
		}
		else if(p_cur->Text.IsEqiAscii("UedOrgFileModTm")) {
			SJson::GetChildUInt64(p_cur, UedOrgFileModTm);
		}
		else if(p_cur->Text.IsEqiAscii("MapPos")) {
			if(SJson::IsObject(p_cur->P_Child)) {
				MapPos.FromJsonObj(p_cur->P_Child);
			}
		}
		else if(p_cur->Text.IsEqiAscii("FileName")) {
			SJson::GetChildTextUnescaped(p_cur, FileNameUtf8);
		}
		else if(p_cur->Text.IsEqiAscii("BackupPath")) {
			SJson::GetChildTextUnescaped(p_cur, BackupPathUtf8);
		}
		else if(p_cur->Text.IsEqiAscii("MarkList")) {
			SJson::GetArrayAsInt64Vector(p_cur->P_Child, MarkList);
		}
		else if(p_cur->Text.IsEqiAscii("FoldStateList")) {
			SJson::GetArrayAsInt64Vector(p_cur->P_Child, FoldStateList);
		}
	}
	return ok;
}
//
//
//
const int MARK_BOOKMARK           = 24;
const int MARK_HIDELINESBEGIN     = 23;
const int MARK_HIDELINESEND       = 22;
const int MARK_HIDELINESUNDERLINE = 21;

static const SIntToSymbTabEntry SScLangEntryList[] = {
	{   SCLEX_NULL,         "normal"       },
	{   SCLEX_HTML,         "php"          },
	{   SCLEX_CPP,          "c"            },
	{   SCLEX_CPP,          "cpp"          },
	{   SCLEX_CPP,          "cs"           },
	{   SCLEX_CPP,          "objc"         },
	{   SCLEX_CPP,          "java"         },
	{   SCLEX_CPP,          "rc"           },
	{   SCLEX_HTML,         "html"         },
	{   SCLEX_XML,          "xml"          },
	{   SCLEX_MAKEFILE,     "makefile"     },
	{   SCLEX_PASCAL,       "pascal"       },
	{   SCLEX_BATCH,        "batch"        },
	{   SCLEX_PROPERTIES,   "ini"          },
	{   SCLEX_NULL,         "nfo"          },
	{   SCLEX_USER,         "udf"          },
	{   SCLEX_HTML,         "asp"          },
	{   SCLEX_SQL,          "sql"          },
	{   SCLEX_VB,           "vb"           },
	{   SCLEX_CPP,          "javascript"   },
	{   SCLEX_CSS,          "css"          },
	{   SCLEX_PERL,         "perl"         },
	{   SCLEX_PYTHON,       "python"       },
	{   SCLEX_LUA,          "lua"          },
	{   SCLEX_TEX,          "tex"          },
	{   SCLEX_FORTRAN,      "fortran"      },
	{   SCLEX_BASH,         "bash"         },
	{   SCLEX_CPP,          "actionscript" },
	{   SCLEX_NSIS,         "nsis"         },
	{   SCLEX_TCL,          "tcl"          },
	{   SCLEX_LISP,         "lisp"         },
	{   SCLEX_LISP,         "scheme"       },
	{   SCLEX_ASM,          "asm"          },
	{   SCLEX_DIFF,         "diff"         },
	{   SCLEX_PROPERTIES,   "props"        },
	{   SCLEX_PS,           "postscript"   },
	{   SCLEX_RUBY,         "ruby"         },
	{   SCLEX_SMALLTALK,    "smalltalk"    },
	{   SCLEX_VHDL,         "vhdl"         },
	{   SCLEX_KIX,          "kix"          },
	{   SCLEX_AU3,          "autoit"       },
	{   SCLEX_CAML,         "caml"         },
	{   SCLEX_ADA,          "ada"          },
	{   SCLEX_VERILOG,      "verilog"      },
	{   SCLEX_MATLAB,       "matlab"       },
	{   SCLEX_HASKELL,      "haskell"      },
	{   SCLEX_INNOSETUP,    "inno"         },
	{   SCLEX_SEARCHRESULT, "searchResult" },
	{   SCLEX_CMAKE,        "cmake"        },
	{   SCLEX_YAML,         "yaml"         },
	{   SCLEX_COBOL,        "cobol"        },
	{   SCLEX_GUI4CLI,      "gui4cli"      },
	{   SCLEX_D,            "d"            },
	{   SCLEX_POWERSHELL,   "powershell"   },
	{   SCLEX_R,            "r"            },
	{   SCLEX_HTML,         "jsp"          },
	{   SCLEX_COFFEESCRIPT, "coffeescript" },
	{   SCLEX_CPP,          "json"         },
	{   SCLEX_CPP,          "javascript.js"},
	{   SCLEX_F77,          "fortran77"    },
	{   SCLEX_BAAN,         "baanc"        },
	{   SCLEX_SREC,         "srec"         },
	{   SCLEX_IHEX,         "ihex"         },
	{   SCLEX_TEHEX,        "tehex"        },
	{   SCLEX_CPP,          "swift"        },
	{   SCLEX_ASN1,         "asn1"         },
	{   SCLEX_AVS,          "avs"          },
	{   SCLEX_BLITZBASIC,   "blitzbasic"   },
	{   SCLEX_PUREBASIC,    "purebasic"    },
	{   SCLEX_FREEBASIC,    "freebasic"    },
	{   SCLEX_CSOUND,       "csound"       },
	{   SCLEX_ERLANG,       "erlang"       },
	{   SCLEX_ESCRIPT,      "escript"      },
	{   SCLEX_FORTH,        "forth"        },
	{   SCLEX_LATEX,        "latex"        },
	{   SCLEX_MMIXAL,       "mmixal"       },
	{   SCLEX_NIMROD,       "nimrod"       },
	{   SCLEX_NNCRONTAB,    "nncrontab"    },
	{   SCLEX_OSCRIPT,      "oscript"      },
	{   SCLEX_REBOL,        "rebol"        },
	{   SCLEX_REGISTRY,     "registry"     },
	{   SCLEX_RUST,         "rust"         },
	{   SCLEX_SPICE,        "spice"        },
	{   SCLEX_TXT2TAGS,     "txt2tags"     },
	{   SCLEX_NULL,         "ext"          }
};

static int FASTCALL SScGetLexerIdByName(const char * pName)
{
    for(uint i = 0; i < SIZEOFARRAY(SScLangEntryList); i++) {
		const SIntToSymbTabEntry & r_entry = SScLangEntryList[i];
		if(sstreqi_ascii(r_entry.P_Symb, pName))
			return i+1;
    }
    return 0;
}

//static const char * FASTCALL SScGetLexerNameById(int id) { return (id > 0 && id <= SIZEOFARRAY(SScLangEntryList)) ? SScLangEntryList[id-1].P_Symb : 0; }
static int FASTCALL SScGetLexerModelById(int id) { return (id > 0 && id <= SIZEOFARRAY(SScLangEntryList)) ? SScLangEntryList[id-1].Id : 0; }

SScEditorStyleSet::SScEditorStyleSet()
{
}

SScEditorStyleSet::~SScEditorStyleSet()
{
}

void SScEditorStyleSet::Destroy()
{
	L.freeAll();
	ML.freeAll();
	KwL.freeAll();
	DestroyS();
}

void   FASTCALL SScEditorStyleSet::InnerToOuter(const InnerLangModel & rS, LangModel & rD) const
{
	rD.LexerId = rS.LexerId;
	GetS(rS.CommentLineP, rD.CommentLine);
	GetS(rS.CommentStartP, rD.CommentStart);
	GetS(rS.CommentEndP, rD.CommentEnd);
}

void   FASTCALL SScEditorStyleSet::InnerToOuter(const InnerLangModelKeywords & rS, LangModelKeywords & rD) const
{
	rD.LexerId = rS.LexerId;
	GetS(rS.KeywordClassP, rD.KeywordClass);
	GetS(rS.KeywordListP, rD.KeywordList);
}

void FASTCALL SScEditorStyleSet::InnerToOuter(const InnerStyle & rS, Style & rD) const
{
    rD.Group = rS.Group;
    rD.LexerId = rS.LexerId;
    rD.StyleId = rS.StyleId;
    rD.BgC = rS.BgC;
    rD.FgC = rS.FgC;
    rD.FontStyle = rS.FontStyle;
    rD.FontSize = rS.FontSize;
    GetS(rS.LexerDescrP, rD.LexerDescr);
    GetS(rS.StyleNameP, rD.StyleName);
    GetS(rS.FontFaceP, rD.FontFace);
    GetS(rS.KeywordClassP, rD.KeywordClass);
}

int SScEditorStyleSet::GetModel(int lexerId, LangModel * pModel) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < ML.getCount(); i++) {
		const InnerLangModel & r_item = ML.at(i);
		if(r_item.LexerId == lexerId) {
			if(pModel) {
				InnerToOuter(r_item, *pModel);
			}
			ok = 1;
		}
	}
	return ok;
}

int SScEditorStyleSet::GetModelKeywords(int lexerId, TSCollection <LangModelKeywords> * pList) const
{
	int    ok = 0;
	for(uint i = 0; i < KwL.getCount(); i++) {
		const InnerLangModelKeywords & r_item = KwL.at(i);
		if(r_item.LexerId == lexerId) {
			if(pList) {
				LangModelKeywords * p_new_entry = pList->CreateNewItem();
				THROW(p_new_entry);
				InnerToOuter(r_item, *p_new_entry);
			}
			ok++;
		}
	}
	CATCHZOK
	return ok;
}

int SScEditorStyleSet::GetStyles(int group, int lexerId, TSCollection <Style> * pList) const
{
	int    ok = 0;
	SString temp_buf;
    for(uint i = 0; i < L.getCount(); i++) {
        const InnerStyle & r_is = L.at(i);
        if(r_is.Group == group) {
			if(lexerId) {
				if(r_is.LexerId == lexerId) {
					if(pList) {
						Style * p_new_entry = pList->CreateNewItem();
						THROW_SL(p_new_entry);
						InnerToOuter(r_is, *p_new_entry);
					}
					ok++;
				}
			}
			else if(group == sgGlobal) {
				if(pList) {
					Style * p_new_entry = pList->CreateNewItem();
					THROW_SL(p_new_entry);
					InnerToOuter(r_is, *p_new_entry);
				}
				ok++;
			}
        }
    }
	CATCHZOK
    return ok;
}

int SScEditorStyleSet::GetStyle(int group, int lexerId, int styleId, Style & rS) const
{
	int    ok = 0;
	SString temp_buf;
    for(uint i = 0; !ok && i < L.getCount(); i++) {
        const InnerStyle & r_is = L.at(i);
        if(r_is.Group == group && r_is.StyleId == styleId) {
			if(lexerId) {
				if(r_is.LexerId == lexerId) {
					InnerToOuter(r_is, rS);
					ok = 1;
				}
			}
			else if(group == sgGlobal) {
				InnerToOuter(r_is, rS);
				ok = 1;
			}
        }
    }
    return ok;
}

int SScEditorStyleSet::ReadStyleAttributes(const xmlNode * pNode, InnerStyle & rS) 
{
	int    ok = 1;
	SString temp_buf;
	rS.FgC.Z();
	rS.BgC.Z();
	rS.FontStyle = -1;
	if(SXml::GetAttrib(pNode, "name", temp_buf))
		AddS(temp_buf, &rS.StyleNameP);
	if(SXml::GetAttrib(pNode, "styleID", temp_buf))
		rS.StyleId = (uint)temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "fgColor", temp_buf)) {
		if(temp_buf.Len() == 6) {
			temp_buf.Insert(0, "#");
			rS.FgC.FromStr(temp_buf);
		}
	}
	if(SXml::GetAttrib(pNode, "bgColor", temp_buf)) {
		if(temp_buf.Len() == 6) {
			temp_buf.Insert(0, "#");
			rS.BgC.FromStr(temp_buf);
		}
	}
	if(SXml::GetAttrib(pNode, "fontName", temp_buf))
		AddS(temp_buf, &rS.FontFaceP);
	if(SXml::GetAttrib(pNode, "fontStyle", temp_buf))
		rS.FontStyle = temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "fontSize", temp_buf))
		rS.FontSize = (uint)temp_buf.ToLong();
	if(SXml::GetAttrib(pNode, "keywordClass", temp_buf))
		AddS(temp_buf, &rS.KeywordClassP);
	return ok;
}

int SScEditorStyleSet::ParseStylesXml(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString lexer_name;
	SString lexer_descr;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	THROW_SL(fileExists(pFileName));
	THROW(p_ctx = xmlNewParserCtxt());
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "EditorStyles")) {
		for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "LexerStyles")) {
				for(const xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "LexerType")) {
						if(SXml::GetAttrib(p_s, "name", lexer_name)) {
							uint   lexer_id = SScGetLexerIdByName(lexer_name);
							if(lexer_id) {
								uint   lexer_descr_p = 0;
								if(SXml::GetAttrib(p_s, "desc", lexer_descr)) 
									AddS(lexer_descr, &lexer_descr_p);
								for(const xmlNode * p_e = p_s->children; p_e; p_e = p_e->next) {
									if(SXml::IsName(p_e, "WordsStyle")) {
										InnerStyle st;
										MEMSZERO(st);
										st.Group = sgLexer;
										st.LexerId = lexer_id;
										st.LexerDescrP = lexer_descr_p;
										ReadStyleAttributes(p_e, st);
										THROW_SL(L.insert(&st));
									}
								}
							}
						}
					}
				}
			}
			else if(SXml::IsName(p_n, "GlobalStyles")) {
				for(const xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "WidgetStyle")) {
						InnerStyle st;
						MEMSZERO(st);
						st.Group = sgGlobal;
						ReadStyleAttributes(p_s, st);
						THROW_SL(L.insert(&st));
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

int SScEditorStyleSet::ParseModelXml(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString lexer_name;
	SString lexer_ext;
	SString keyword_class_name;
	xmlParserCtxt * p_ctx = 0;
	xmlDoc * p_doc = 0;
	xmlNode * p_root = 0;
	THROW_SL(fileExists(pFileName));
	THROW(p_ctx = xmlNewParserCtxt());
	THROW_LXML((p_doc = xmlCtxtReadFile(p_ctx, pFileName, 0, XML_PARSE_NOENT)), p_ctx);
	THROW(p_root = xmlDocGetRootElement(p_doc));
	if(SXml::IsName(p_root, "EditorLangModels")) {
		for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, "Languages")) {
				for(const xmlNode * p_s = p_n->children; p_s; p_s = p_s->next) {
					if(SXml::IsName(p_s, "Language")) {
						if(SXml::GetAttrib(p_s, "name", lexer_name)) {
							uint   lexer_id = SScGetLexerIdByName(lexer_name);
							if(lexer_id) {
								InnerLangModel model;
								MEMSZERO(model);
								model.LexerId = lexer_id;
								if(SXml::GetAttrib(p_s, "ext", temp_buf))
									AddS(temp_buf, &model.ExtListP);
								if(SXml::GetAttrib(p_s, "commentLine", temp_buf))
									AddS(temp_buf, &model.CommentLineP);
								if(SXml::GetAttrib(p_s, "commentStart", temp_buf))
									AddS(temp_buf, &model.CommentStartP);
								if(SXml::GetAttrib(p_s, "commentEnd", temp_buf))
									AddS(temp_buf, &model.CommentEndP);
								THROW_SL(ML.insert(&model));
								for(const xmlNode * p_e = p_s->children; p_e; p_e = p_e->next) {
									if(SXml::IsName(p_e, "Keywords")) {
										if(SXml::GetContent(p_e, temp_buf) && temp_buf.NotEmptyS()) {
											InnerLangModelKeywords entry;
											MEMSZERO(entry);
											entry.LexerId = lexer_id;
											AddS(temp_buf, &entry.KeywordListP);
											if(SXml::GetAttrib(p_e, "name", temp_buf))
												AddS(temp_buf, &entry.KeywordClassP);
											THROW_SL(KwL.insert(&entry));
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	xmlFreeDoc(p_doc);
	xmlFreeParserCtxt(p_ctx);
	return ok;
}

void SScEditorBase::SetSpecialStyle(const SScEditorStyleSet::Style & rStyle)
{
	int    styleID = rStyle.StyleId;
	if(!rStyle.FgC.IsEmpty())
		CallFunc(SCI_STYLESETFORE, styleID, rStyle.FgC);
	if(!rStyle.BgC.IsEmpty())
		CallFunc(SCI_STYLESETBACK, styleID, rStyle.BgC);
	if(rStyle.FontFace.NotEmpty()) {
		CallFunc(SCI_STYLESETFONT, styleID, reinterpret_cast<intptr_t>(rStyle.FontFace.cptr()));
	}
	int font_style = rStyle.FontStyle;
	if(font_style != -1/*STYLE_NOT_USED*/) {
		CallFunc(SCI_STYLESETBOLD, styleID, font_style & SScEditorStyleSet::FONTSTYLE_BOLD);
		CallFunc(SCI_STYLESETITALIC, styleID, font_style & SScEditorStyleSet::FONTSTYLE_ITALIC);
		CallFunc(SCI_STYLESETUNDERLINE, styleID, font_style & SScEditorStyleSet::FONTSTYLE_UNDERLINE);
	}
	if(rStyle.FontSize > 0)
		CallFunc(SCI_STYLESETSIZE, styleID, rStyle.FontSize);
}

constexpr int LANG_INDEX_INSTR  = 0;
constexpr int LANG_INDEX_INSTR2 = 1;
constexpr int LANG_INDEX_TYPE   = 2;
constexpr int LANG_INDEX_TYPE2  = 3;
constexpr int LANG_INDEX_TYPE3  = 4;
constexpr int LANG_INDEX_TYPE4  = 5;
constexpr int LANG_INDEX_TYPE5  = 6;

static int GetKwClassFromName(const char * pStr, const char * pLexerName)
{
	if(sstreq(pStr, "instre1")) 
		return LANG_INDEX_INSTR;
	else if(sstreq(pStr, "instre2")) 
		return LANG_INDEX_INSTR2;
	else if(sstreq(pStr, "type1")) {
		if(sstreq(pLexerName, "cpp"))
			return 1;
		else
			return LANG_INDEX_TYPE;
	}
	else if(sstreq(pStr, "type2")) 
		return LANG_INDEX_TYPE2;
	else if(sstreq(pStr, "type3")) 
		return LANG_INDEX_TYPE3;
	else if(sstreq(pStr, "type4")) 
		return LANG_INDEX_TYPE4;
	else if(sstreq(pStr, "type5")) 
		return LANG_INDEX_TYPE5;
	else if(pStr[1] == 0 && pStr[0] >= '0' && pStr[0] <= '8')
		return (pStr[0] - '0');
	else
		return -1;
}

static SScEditorStyleSet * _GetGlobalSScEditorStyleSetInstance()
{
	static const char * P_GlobalSymbol = "SScEditorStyleSet";
	SScEditorStyleSet * p_ss = 0;
	long   symbol_id = SLS.GetGlobalSymbol(P_GlobalSymbol, -1, 0);
	THROW_SL(symbol_id);
	if(symbol_id < 0) {
		TSClassWrapper <SScEditorStyleSet> cls;
		THROW_SL(symbol_id = SLS.CreateGlobalObject(cls));
		THROW_SL(p_ss = static_cast<SScEditorStyleSet *>(SLS.GetGlobalObject(symbol_id)));
		{
			long s = SLS.GetGlobalSymbol(P_GlobalSymbol, symbol_id, 0);
			assert(symbol_id == s);
		}
		{
			int    r = 0;
			SString file_name;
			PPGetFilePath(PPPATH_DD, "editorlangmodel.xml", file_name);
			r = p_ss->ParseModelXml(file_name);
			if(r) {
				PPGetFilePath(PPPATH_DD, "editorstyles.xml", file_name);
				r = p_ss->ParseStylesXml(file_name);
			}
			if(!r) {
				p_ss->Destroy(); // Указатель оставляем не нулевым дабы в течении сеанса каждый раз не пытаться создавать его заново.
			}
		}
	}
	else if(symbol_id > 0) {
		THROW_SL(p_ss = static_cast<SScEditorStyleSet *>(SLS.GetGlobalObject(symbol_id)));
	}
	CATCH
		p_ss = 0;
	ENDCATCH
	return p_ss;
}

int SScEditorBase::SetLexer(const char * pLexerName)
{
	int    ok = 0;
	SScEditorStyleSet * p_ss = _GetGlobalSScEditorStyleSetInstance();
	int    lexer_id = SScGetLexerIdByName(pLexerName);
	if(lexer_id && p_ss) {
		int lexer_model = SScGetLexerModelById(lexer_id);
		SScEditorStyleSet::LangModel model;
		if(p_ss->GetModel(lexer_model, &model)) {
			CallFunc(SCI_SETLEXER, lexer_model);
			TSCollection <SScEditorStyleSet::LangModelKeywords> kw_list;
			TSCollection <SScEditorStyleSet::Style> style_list;
			p_ss->GetStyles(SScEditorStyleSet::sgLexer, lexer_id, &style_list);
			int    kwc = p_ss->GetModelKeywords(lexer_model, &kw_list);
			for(uint i = 0; i < style_list.getCount(); i++) {
				const SScEditorStyleSet::Style * p_style = style_list.at(i);
				if(p_style) {
					if(p_style->KeywordClass.NotEmpty()) {
						for(uint j = 0; j < kw_list.getCount(); j++) {
							SScEditorStyleSet::LangModelKeywords * p_kw = kw_list.at(j);
							if(p_kw && p_kw->KeywordClass.CmpNC(p_style->KeywordClass) == 0) {
								int kw_n = GetKwClassFromName(p_kw->KeywordClass, pLexerName);
								if(kw_n >= 0 && kw_n <= 8) {
									CallFunc(SCI_SETKEYWORDS, kw_n, reinterpret_cast<intptr_t>(p_kw->KeywordList.cptr()));
									break;
								}
							}
						}
					}
					SetSpecialStyle(*p_style);
				}
			}
			if(sstreq(pLexerName, "cpp")) {
				//width = NppParameters::getInstance()->_dpiManager.scaleX(100) >= 150 ? 18 : 14;
				//CallFunc(SCI_SETMARGINWIDTHN, 2/*folding*/, 14); // @v10.2.0
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold"), reinterpret_cast<intptr_t>("1"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.compact"), reinterpret_cast<intptr_t>("0"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.comment"), reinterpret_cast<intptr_t>("1"));
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("fold.preprocessor"), reinterpret_cast<intptr_t>("1"));
				// Disable track preprocessor to avoid incorrect detection.
				// In the most of cases, the symbols are defined outside of file.
				CallFunc(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<intptr_t>("0"));
				// @v11.1.12 {
				CallFunc(SCI_SETMARGINWIDTHN, scmargeFolder, 0);
				CallFunc(SCI_SETMARGINTYPEN,  scmargeFolder, SC_MARGIN_SYMBOL);
				CallFunc(SCI_SETMARGINMASKN,  scmargeFolder, SC_MASK_FOLDERS);
				CallFunc(SCI_SETMARGINMASKN, scmargeFolder, SC_MASK_FOLDERS);
				CallFunc(SCI_SETMARGINWIDTHN, scmargeFolder, 20);
				//CallFunc(SCI_SETMARGINMASKN, scmargeSymbole, (1<<MARK_BOOKMARK) | (1<<MARK_HIDELINESBEGIN) | (1<<MARK_HIDELINESEND) | (1<<MARK_HIDELINESUNDERLINE));

				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_PLUS);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY);
				CallFunc(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY);
				CallFunc(SCI_SETFOLDFLAGS, 16, 0); // 16 Draw line below if not expande
				// } @v11.1.12 
			}
			ok = 1;
		}
	}
	//CATCHZOK
	return ok;
}
//
//
//
class SearchReplaceDialog : public TDialog {
	DECL_DIALOG_DATA(SSearchReplaceParam);
public:
	SearchReplaceDialog() : TDialog(DLG_SCISEARCH)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		setCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		AddClusterAssoc(CTL_SCISEARCH_RF, 0, SSearchReplaceParam::fReplace);
		SetClusterData(CTL_SCISEARCH_RF, Data.Flags);
		if(Data.Flags & SSearchReplaceParam::fReplace) {
			disableCtrl(CTL_SCISEARCH_REPLACE, false);
			setCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		}
		else {
			disableCtrl(CTL_SCISEARCH_REPLACE, true);
		}
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 0, SSearchReplaceParam::fNoCase);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 1, SSearchReplaceParam::fWholeWords);
		AddClusterAssoc(CTL_SCISEARCH_FLAGS, 2, SSearchReplaceParam::fReverse);
		SetClusterData(CTL_SCISEARCH_FLAGS, Data.Flags);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		getCtrlString(CTL_SCISEARCH_PATTERN, Data.Pattern);
		getCtrlString(CTL_SCISEARCH_REPLACE, Data.Replacer);
		GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
		GetClusterData(CTL_SCISEARCH_FLAGS, &Data.Flags);
		ASSIGN_PTR(pData, Data);
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_SCISEARCH_RF)) {
			GetClusterData(CTL_SCISEARCH_RF, &Data.Flags);
			disableCtrl(CTL_SCISEARCH_REPLACE, !BIN(Data.Flags & SSearchReplaceParam::fReplace));
		}
		else
			return;
		clearEvent(event);
	}
};

int EditSearchReplaceParam(SSearchReplaceParam * pData) { DIALOG_PROC_BODY(SearchReplaceDialog, pData); }
//
//
//
SScEditorBase::SScEditorBase() : P_SciFn(0), P_SciPtr(0), P_Tknzr(0), H_Window(0), P_RestorePosBlk(0)
{
	Init(0, 0);
}

SScEditorBase::~SScEditorBase()
{
	delete P_Tknzr;
	delete P_RestorePosBlk;
}

int SScEditorBase::GetConfig(Config & rCfg) const
{
	rCfg = Cfg;
	return 1;
}

int SScEditorBase::SetConfig(const Config & rCfg)
{
	Cfg = rCfg;
	return 1;
}

void SScEditorBase::ClearIndicator(int indicatorNumber)
{
	int    doc_start = 0;
	int    doc_end = CallFunc(SCI_GETLENGTH);
	CallFunc(SCI_SETINDICATORCURRENT, indicatorNumber);
	CallFunc(SCI_INDICATORCLEARRANGE, doc_start, doc_end-doc_start);
}

void SScEditorBase::Init(HWND hScW, int preserveFileName)
{
	P_SciFn = 0;
	P_SciPtr = 0;
	Doc.Reset(preserveFileName);
	if(hScW) {
		P_SciFn  = reinterpret_cast<intptr_t (__cdecl *)(void *, uint, uintptr_t, intptr_t)>(::SendMessage(hScW, SCI_GETDIRECTFUNCTION, 0, 0));
		P_SciPtr = reinterpret_cast<void *>(SendMessage(hScW, SCI_GETDIRECTPOINTER, 0, 0));
		CallFunc(SCI_INDICSETSTYLE, indicUnknWord, /*INDIC_SQUIGGLE*/INDIC_COMPOSITIONTHICK);
		CallFunc(SCI_INDICSETFORE,  indicUnknWord, GetColorRef(SClrRed));
		CallFunc(SCI_INDICSETSTYLE, indicStxRule, INDIC_PLAIN);
		CallFunc(SCI_INDICSETFORE,  indicStxRule, GetColorRef(SClrCyan));
		// @v12.5.7 {
		{
			// Отключаем встроенную обработку для Alt+стрелки
			CallFunc(SCI_ASSIGNCMDKEY, (SCMOD_ALT << 8)|SCK_LEFT,  SCI_NULL);
			CallFunc(SCI_ASSIGNCMDKEY, (SCMOD_ALT << 8)|SCK_RIGHT, SCI_NULL);
			CallFunc(SCI_ASSIGNCMDKEY, (SCMOD_ALT << 8)|SCK_UP,    SCI_NULL);
			CallFunc(SCI_ASSIGNCMDKEY, (SCMOD_ALT << 8)|SCK_DOWN,  SCI_NULL);
		}
		// } @v12.5.7 
	}
	H_Window = hScW;
}

int SScEditorBase::Release()
{
	int    ok = -1;
	if(Doc.SciDoc) {
		CallFunc(SCI_CLEARALL);
		CallFunc(SCI_RELEASEDOCUMENT, 0, reinterpret_cast<intptr_t>(Doc.SciDoc));
		Doc.Reset(0);
		ok = 1;
	}
	return ok;
}

intptr_t SScEditorBase::CallFunc(uint msg) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, 0, 0) : 0; }
intptr_t SScEditorBase::CallFunc(uint msg, uintptr_t param1) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, param1, 0) : 0; }
intptr_t SScEditorBase::CallFunc(uint msg, uintptr_t param1, intptr_t param2) { return (P_SciFn && P_SciPtr) ? P_SciFn(P_SciPtr, msg, param1, param2) : 0; }

int SScEditorBase::SetKeybAccelerator(KeyDownCommand & rK, int cmd)
{
	int    ok = OuterKeyAccel.Set(rK, cmd);
	KeyAccel.Set(rK, cmd);
	return ok;
}

int32 SScEditorBase::GetCurrentPos()
{
	return CallFunc(SCI_GETCURRENTPOS);
}

int32 FASTCALL SScEditorBase::SetCurrentPos(int32 pos)
{
	int32 prev = CallFunc(SCI_GETCURRENTPOS);
	CallFunc(SCI_SETCURRENTPOS, pos);
	return prev;
}

int FASTCALL SScEditorBase::GetSelection(IntRange & rR)
{
	rR.Set(CallFunc(SCI_GETSELECTIONSTART), CallFunc(SCI_GETSELECTIONEND));
	return 1;
}

int FASTCALL SScEditorBase::SetSelection(const IntRange * pR)
{
	int    ok = -1;
	if(!pR || pR->IsZero()) {
		CallFunc(SCI_SETEMPTYSELECTION);
		ok = -1;
	}
	else {
		CallFunc(SCI_SETSELECTIONSTART, pR->low);
		CallFunc(SCI_SETSELECTIONEND, pR->upp);
		ok = 1;
	}
	return ok;
}

int FASTCALL SScEditorBase::GetSelectionText(SString & rBuf)
{
	rBuf.Z();
	int sz = CallFunc(SCI_GETSELTEXT);
	if(sz > 0) {
		STempBuffer temp_b(sz);
		if(temp_b.IsValid()) {
			sz = CallFunc(SCI_GETSELTEXT, 0, reinterpret_cast<intptr_t>((char *)temp_b));
			rBuf = temp_b;
		}
		else
			sz = 0;
	}
	return sz;
}

int SScEditorBase::SearchAndReplace(long flags)
{
	int    ok = -1;
	SSearchReplaceParam param = LastSrParam;
	int   ssz = 0;
	if(param.Pattern.IsEmpty() || flags & srfUseDialog) {
		ssz = GetSelectionText(param.Pattern);
		if(ssz > 0)
			param.Pattern.Transf(CTRANSF_UTF8_TO_INNER);
	}
	if(!(flags & srfUseDialog) || EditSearchReplaceParam(&param) > 0) {
		LastSrParam = param;
		SString pattern(param.Pattern);
		pattern.Transf(CTRANSF_INNER_TO_UTF8);
		if(pattern.NotEmpty()) {
			IntRange sel;
			int    sci_srch_flags = 0;
			int    _func = 0;
			if(!(param.Flags & param.fNoCase))
				sci_srch_flags |= SCFIND_MATCHCASE;
			if(param.Flags & param.fWholeWords)
				sci_srch_flags |= SCFIND_WHOLEWORD;
			GetSelection(sel);
			const IntRange preserve_sel = sel;
			if(param.Flags & param.fReverse) {
				_func = SCI_SEARCHPREV;
			}
			else {
				_func = SCI_SEARCHNEXT;
				sel.low++;
				SetSelection(&sel);
			}
			CallFunc(SCI_SEARCHANCHOR);
			int    result = CallFunc(_func, sci_srch_flags, reinterpret_cast<intptr_t>(pattern.cptr()));
			if(result >= 0) {
				ok = 1;
				const int selend = CallFunc(SCI_GETSELECTIONEND);
				SetCurrentPos(selend);
				CallFunc(SCI_SCROLLCARET);
				SetSelection(&sel.Set(result, selend));
				CallFunc(SCI_SEARCHANCHOR);
			}
			else {
				SetSelection(&preserve_sel);
			}
		}
	}
	return ok;
}
//
//
//
#if 0 // unused {
static int FASTCALL VkToScTranslate(int keyIn)
{
	switch(keyIn) {
		case VK_DOWN:		return SCK_DOWN;
		case VK_UP:			return SCK_UP;
		case VK_LEFT:		return SCK_LEFT;
		case VK_RIGHT:		return SCK_RIGHT;
		case VK_HOME:		return SCK_HOME;
		case VK_END:		return SCK_END;
		case VK_PRIOR:		return SCK_PRIOR;
		case VK_NEXT:		return SCK_NEXT;
		case VK_DELETE:		return SCK_DELETE;
		case VK_INSERT:		return SCK_INSERT;
		case VK_ESCAPE:		return SCK_ESCAPE;
		case VK_BACK:		return SCK_BACK;
		case VK_TAB:		return SCK_TAB;
		case VK_RETURN:		return SCK_RETURN;
		case VK_ADD:		return SCK_ADD;
		case VK_SUBTRACT:	return SCK_SUBTRACT;
		case VK_DIVIDE:		return SCK_DIVIDE;
		case VK_OEM_2:		return '/';
		case VK_OEM_3:		return '`';
		case VK_OEM_4:		return '[';
		case VK_OEM_5:		return '\\';
		case VK_OEM_6:		return ']';
		default:			return keyIn;
	}
};

static int FASTCALL ScToVkTranslate(int keyIn)
{
	switch(keyIn) {
		case SCK_DOWN: return VK_DOWN;
		case SCK_UP: return VK_UP;
		case SCK_LEFT: return VK_LEFT;
		case SCK_RIGHT: return VK_RIGHT;
		case SCK_HOME: return VK_HOME;
		case SCK_END: return VK_END;
		case SCK_PRIOR: return VK_PRIOR;
		case SCK_NEXT: return VK_NEXT;
		case SCK_DELETE: return VK_DELETE;
		case SCK_INSERT: return VK_INSERT;
		case SCK_ESCAPE: return VK_ESCAPE;
		case SCK_BACK: return VK_BACK;
		case SCK_TAB: return VK_TAB;
		case SCK_RETURN: return VK_RETURN;
		case SCK_ADD: return VK_ADD;
		case SCK_SUBTRACT: return VK_SUBTRACT;
		case SCK_DIVIDE: return VK_DIVIDE;
		case '/': return VK_OEM_2;
		case '`': return VK_OEM_3;
		case '[': return VK_OEM_4;
		case '\\': return VK_OEM_5;
		case ']': return VK_OEM_6;
		default: return keyIn;
	}
}
#endif // } 0 unused

STextBrowser::Document::Document() : Cp(/*cpANSI*/cpUTF8), Eolf(eolUndef), State(0), SciDoc(0) // @v9.9.9 cpANSI-->cpUTF8
{
}

STextBrowser::Document & FASTCALL STextBrowser::Document::Reset(bool preserveSourceInfo)
{
	OrgCp = cpUndef;
	Cp = cpUndef;
	Eolf = eolUndef;
	State = 0;
	SciDoc = 0;
	if(!preserveSourceInfo) {
		FileName.Z();
		OtrIdent.Z(); // @v12.5.6
	}
	return *this;
}

long STextBrowser::Document::SetState(long st, int set)
{
	SETFLAG(State, st, set);
	return State;
}

bool STextBrowser::Document::MakeNameSymbol(int storagesubj, bool withoutExt, SString & rBuf) const
{
	rBuf.Z();
	bool   ok = true;
	if(OtrIdent.O.IsFullyDefined() && OtrIdent.P) {
		OtrIdent.ToStr(rBuf);
		if(oneof2(storagesubj, storagesubjUndef, storagesubjText)) {
			rBuf.DotCat("txt");
		}
		else if(storagesubj == storagesubjState) {
			rBuf.DotCat("json");
		}
	}
	else if(FileName.NotEmpty()) {
		SString temp_buf;
		SFsPath::NormalizePath(FileName, SFsPath::npfCompensateDotDot|SFsPath::npfSlash, temp_buf); // Это - важно (иначе хэши не совпадут)!
		SFsPath ps(temp_buf);
		ps.Merge(SFsPath::fNam, rBuf);
		const  uint32 hash = SlHash::XX32(temp_buf.ucptr(), temp_buf.Len(), /*HashSeed*/SlConst::Seed32_FileName);
		rBuf.CatChar('#').CatHex(hash);
		if(!withoutExt) {
			if(oneof2(storagesubj, storagesubjUndef, storagesubjText)) {
				if(ps.Ext.NotEmpty())
					rBuf.DotCat(ps.Ext);
			}
			else if(storagesubj == storagesubjState) {
				rBuf.DotCat("json");
			}
		}
	}
	else
		ok = false;
	return ok;
}

STextBrowser::STextBrowser() : TBaseBrowserWindow(WndClsName), SScEditorBase(), SpcMode(spcmNo), IdleTimer(0)
{
	Init(0, 0, 0);
}

STextBrowser::STextBrowser(const char * pFileName, const char * pLexerSymb, int toolbarId) : 
	TBaseBrowserWindow(WndClsName), SScEditorBase(), SpcMode(spcmNo), IdleTimer(0)
{
	Init(pFileName, pLexerSymb, toolbarId);
}

STextBrowser::STextBrowser(const SObjTextRefIdent & rIdent, const char * pLexerSymb, int toolbarId/*=-1*/) : 
	TBaseBrowserWindow(WndClsName), SScEditorBase(), SpcMode(spcmNo), IdleTimer(0) // @v12.5.6
{
	Init(rIdent, pLexerSymb, toolbarId);
}

STextBrowser::~STextBrowser()
{
	if(::IsWindow(HwndSci)) {
		FileClose();
		if(OrgScintillaWndProc) {
			TView::SetWindowProp(HwndSci, GWLP_WNDPROC, OrgScintillaWndProc);
			TView::SetWindowProp(HwndSci, GWLP_USERDATA, nullptr);
		}
		::DestroyWindow(HwndSci);
	}
}

int STextBrowser::SetSpecialMode(int spcm)
{
	int    ok = 1;
	if(spcm == spcmSartrTest) {
		SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();
		if(p_srdb) {
			SpcMode = spcm;
		}
	}
	else {
		SpcMode = spcmNo;
	}
	return ok;
}

/*virtual*/TBaseBrowserWindow::IdentBlock & STextBrowser::GetIdentBlock(TBaseBrowserWindow::IdentBlock & rBlk)
{
	rBlk.IdBias = IdBiasTextBrowser;
	rBlk.ClsName = SUcSwitchW(STextBrowser::WndClsName);
	(rBlk.InstanceIdent = Doc.FileName).Strip().ToLower();
	return rBlk;
}

int STextBrowser::Init(const SObjTextRefIdent & rIdent, const char * pLexerSymb, int toolbarId/*= -1*/) // @v12.5.6
{
	SScEditorBase::Init(0, 0);
	LexerSymb = pLexerSymb;
	OrgScintillaWndProc = 0;
	SysState = 0;
	Doc.OtrIdent = rIdent;
	BbState |= bbsWoScrollbars;
	P_Toolbar = 0;
	ToolBarWidth = 0;
	if(toolbarId < 0)
		ToolbarID = TOOLBAR_TEXTBROWSER;
	else if(toolbarId > 0)
		ToolbarID = toolbarId;
	{
		KeyDownCommand k;
		k.SetTvKeyCode(kbF3);
		SetKeybAccelerator(k, PPVCMD_SEARCHNEXT);
	}
	//SetIdlePeriod(3);
	return 1;
}

int STextBrowser::Init(const char * pFileName, const char * pLexerSymb, int toolbarId)
{
	SScEditorBase::Init(0, 0);
	LexerSymb = pLexerSymb;
	OrgScintillaWndProc = 0;
	SysState = 0;
	Doc.FileName = pFileName;
	BbState |= bbsWoScrollbars;
	P_Toolbar = 0;
	ToolBarWidth = 0;
	if(toolbarId < 0)
		ToolbarID = TOOLBAR_TEXTBROWSER;
	else if(toolbarId > 0)
		ToolbarID = toolbarId;
	{
		KeyDownCommand k;
		k.SetTvKeyCode(kbF3);
		SetKeybAccelerator(k, PPVCMD_SEARCHNEXT);
	}
	return 1;
}

IMPL_HANDLE_EVENT(STextBrowser) // @v12.5.11
{
	TBaseBrowserWindow::handleEvent(event);
	if(TVBROADCAST) {
		if(TVCMD == cmIdle) {
			if(IdleTimer.Check(0)) {
				DoAutosaveByIdle(0);
				//P_View->ProcessCommand(PPVCMD_IDLE, 0, this);
			}
			return; // в конце функции стоит clearEvent(event) который препятствует дальнейшей обработке cmIdle
		}
	}
}

/*static*/const wchar_t * STextBrowser::WndClsName = L"STextBrowser"; // @global

/*static*/int STextBrowser::RegWindowClass(HINSTANCE hInst)
{
	WNDCLASSEXW wc;
	TBaseBrowserWindow::MakeDefaultWindowClassBlock(&wc, hInst);
	wc.lpfnWndProc   = STextBrowser::WndProc;
	wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCE(/*ICON_TIMEGRID*/172));
	wc.lpszClassName = STextBrowser::WndClsName;
#if !defined(_PPDLL) && !defined(_PPSERVER)
	Scintilla_RegisterClasses(hInst);
#endif
	return RegisterClassExW(&wc);
}
//
// Run through full document. When switching in or opening folding
// hide is false only when user click on margin
//
void STextBrowser::RunMarkers(bool doHide, size_t searchStart, bool endOfDoc, bool doDelete)
{
	//Removes markers if opening
	/*
	   AllLines = (start,ENDOFDOCUMENT)
	   Hide:
	        Run through all lines.
	                Find open hiding marker:
	                        set hiding start
	                Find closing:
	                        if (hiding):
	                                Hide lines between now and start
	                                if (endOfDoc = false)
	                                        return
	                                else
	                                        search for other hidden sections

	   Show:
	        Run through all lines
	                Find open hiding marker
	                        set last start
	                Find closing:
	                        Show from last start. Stop.
	                Find closed folding header:
	                        Show from last start to folding header
	                        Skip to LASTCHILD
	                        Set last start to lastchild
	 */
	size_t max_lines = CallFunc(SCI_GETLINECOUNT);
	if(doHide) {
		size_t start_hiding = searchStart;
		bool   is_in_section = false;
		for(size_t i = searchStart; i < max_lines; ++i) {
			uint state = CallFunc(SCI_MARKERGET, i);
			if(((state & (1 << MARK_HIDELINESEND)) != 0) ) {
				if(is_in_section) {
					CallFunc(SCI_HIDELINES, start_hiding, i-1);
					if(!endOfDoc) {
						return; // done, only single section requested
					}       //otherwise keep going
				}
				is_in_section = false;
			}
			if(((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0)) {
				is_in_section = true;
				start_hiding = i+1;
			}
		}
	}
	else {
		size_t startShowing = searchStart;
		bool   is_in_section = false;
		for(size_t i = searchStart; i < max_lines; ++i) {
			uint state = CallFunc(SCI_MARKERGET, i);
			if(((state & (1 << MARK_HIDELINESEND)) != 0)) {
				if(doDelete) {
					CallFunc(SCI_MARKERDELETE, i, MARK_HIDELINESEND);
					if(!endOfDoc) {
						return; // done, only single section requested
					} // otherwise keep going
				}
				else if(is_in_section) {
					if(startShowing >= i) { //because of fold skipping, we passed the close tag. In that case we cant do anything
						if(!endOfDoc) {
							return;
						}
						else {
							continue;
						}
					}
					CallFunc(SCI_SHOWLINES, startShowing, i-1);
					if(!endOfDoc) {
						return; //done, only single section requested
					}       //otherwise keep going
					is_in_section = false;
				}
			}
			if(((state & (1 << MARK_HIDELINESBEGIN | 1 << MARK_HIDELINESUNDERLINE)) != 0)) {
				if(doDelete) {
					CallFunc(SCI_MARKERDELETE, i, MARK_HIDELINESBEGIN);
					CallFunc(SCI_MARKERDELETE, i, MARK_HIDELINESUNDERLINE);
				}
				else {
					is_in_section = true;
					startShowing = i+1;
				}
			}
			int level_line = CallFunc(SCI_GETFOLDLEVEL, i, 0);
			if(level_line & SC_FOLDLEVELHEADERFLAG) { //fold section. Dont show lines if fold is closed
				if(is_in_section && !IsFolded(i)) {
					CallFunc(SCI_SHOWLINES, startShowing, i);
					//startShowing = execute(SCI_GETLASTCHILD, i, (levelLine &
					// SC_FOLDLEVELNUMBERMASK));
				}
			}
		}
	}
}

void STextBrowser::Expand(size_t & rLine, bool doExpand, bool force, int visLevels, int level)
{
	//void expand(size_t& line, bool doExpand, bool force = false, int visLevels = 0, int level = -1);
	size_t line_max_subord = CallFunc(SCI_GETLASTCHILD, rLine, level & SC_FOLDLEVELNUMBERMASK);
	++rLine;
	while(rLine <= line_max_subord) {
		if(force) {
			CallFunc(((visLevels > 0) ? SCI_SHOWLINES : SCI_HIDELINES), rLine, rLine);
		}
		else {
			if(doExpand)
				CallFunc(SCI_SHOWLINES, rLine, rLine);
		}
		int level_line = level;
		if(level_line == -1)
			level_line = int(CallFunc(SCI_GETFOLDLEVEL, rLine, 0));
		if(level_line & SC_FOLDLEVELHEADERFLAG) {
			if(force) {
				if(visLevels > 1)
					CallFunc(SCI_SETFOLDEXPANDED, rLine, 1);
				else
					CallFunc(SCI_SETFOLDEXPANDED, rLine, 0);
				Expand(rLine, doExpand, force, visLevels-1, -1); // @recursion
			}
			else {
				if(doExpand) {
					if(!IsFolded(rLine))
						CallFunc(SCI_SETFOLDEXPANDED, rLine, 1);
					Expand(rLine, true, force, visLevels-1, -1); // @recursion
				}
				else
					Expand(rLine, false, force, visLevels-1, -1); // @recursion
			}
		}
		else
			++rLine;
	}
	RunMarkers(true, 0, true, false);
}

void STextBrowser::MarginClick(/*Sci_Position*/int position, int modifiers)
{
	size_t line_click = CallFunc(SCI_LINEFROMPOSITION, position, 0);
	int level_click = int(CallFunc(SCI_GETFOLDLEVEL, line_click, 0));
	if(level_click & SC_FOLDLEVELHEADERFLAG) {
		if(modifiers & SCMOD_SHIFT) {
			// Ensure all children visible
			CallFunc(SCI_SETFOLDEXPANDED, line_click, 1);
			Expand(line_click, true, true, 100, level_click);
		}
		else if(modifiers & SCMOD_CTRL) {
			if(IsFolded(line_click)) {
				// Contract this line and all children
				CallFunc(SCI_SETFOLDEXPANDED, line_click, 0);
				Expand(line_click, false, true, 0, level_click);
			}
			else {
				// Expand this line and all children
				CallFunc(SCI_SETFOLDEXPANDED, line_click, 1);
				Expand(line_click, true, true, 100, level_click);
			}
		}
		else {
			// Toggle this line
			bool mode = IsFolded(line_click);
			Fold(line_click, !mode, true/*shouldBeNotified*/);
			RunMarkers(true, line_click, true, false);
		}
	}
}

/*static*/LRESULT CALLBACK STextBrowser::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CREATESTRUCT * p_init_data;
	STextBrowser * p_view = 0;
	switch(message) {
		case WM_CREATE:
			p_view = static_cast<STextBrowser *>(Helper_InitCreation(lParam, (void **)&p_init_data));
			if(p_view) {
				p_view->HW = hWnd;
				TView::SetWindowProp(hWnd, GWLP_USERDATA, p_view);
				::SetFocus(hWnd);
				::SendMessageW(hWnd, WM_NCACTIVATE, TRUE, 0);
				p_view->WMHCreate();
				::PostMessageW(hWnd, WM_PAINT, 0, 0);
				if(!(p_view->BbState & bbsInner)) { // @v12.5.4
					SString temp_buf;
					TView::SGetWindowText(hWnd, temp_buf);
					APPL->AddItemToMenu(temp_buf, p_view);
				}
				::SetFocus(p_view->HwndSci);
				return 0;
			}
			else
				return -1;
		case WM_COMMAND:
			{
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					if(HIWORD(wParam) == 0) {
						if(p_view->KeyAccel.getCount()) {
							long   cmd = 0;
							KeyDownCommand k;
							k.SetTvKeyCode(LOWORD(wParam));
							if(p_view->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
								p_view->ProcessCommand(cmd, 0, p_view);
							}
						}
					}
					/*
					if(LOWORD(wParam))
						p_view->ProcessCommand(LOWORD(wParam), 0, p_view);
					*/
				}
			}
			break;
		case WM_DESTROY:
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				p_view->SaveChanges();
				TWindowBase::Helper_Finalize(hWnd, p_view);
			}
			return 0;
		case WM_SETFOCUS:
			{
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && !(p_view->Sf & (sfOnDestroy|sfOnParentDestruction))) { // @v12.5.4
					if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION)) {
						SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
						APPL->NotifyFrame(0);
					}
					{
						::SetFocus(p_view->HwndSci);
						APPL->SelectTabItem(p_view);
						TView::messageBroadcast(p_view, cmReceivedFocus);
						p_view->select();
					}
				}
			}
			break;
		case WM_KILLFOCUS:
			if(!(TView::SGetWindowStyle(hWnd) & WS_CAPTION))
				APPL->NotifyFrame(0);
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(p_view) {
				TView::messageBroadcast(p_view, cmReleasedFocus);
				p_view->ResetOwnerCurrent();
			}
			break;
		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view) {
					p_view->endModal(cmCancel);
					return 0;
				}
			}
			else if(wParam == VK_TAB) {
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && GetKeyState(VK_CONTROL) & 0x8000 && !p_view->IsInState(sfModal)) {
					SetFocus(GetNextBrowser(hWnd, (GetKeyState(VK_SHIFT) & 0x8000) ? 0 : 1));
					return 0;
				}
			}
			else if(oneof4(wParam, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN) && (GetKeyState(VK_MENU) & 0x8000)) { // @v12.5.7
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && p_view->P_Owner) {
					p_view->P_Owner->TView::HandleKeyboardEvent(LOWORD(wParam));
				}
			}
			return 0;
		case WM_SIZE:
			p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
			if(lParam && p_view) {
				HWND   hw = p_view->P_Toolbar ? p_view->P_Toolbar->H() : 0;
				if(IsWindowVisible(hw)) {
					MoveWindow(hw, 0, 0, LOWORD(lParam), p_view->ToolBarWidth, 0);
					TView::messageCommand(p_view, cmResize); // must be cmSize
				}
				p_view->Resize();
			}
			break;
		case WM_NOTIFY:
			{
				//LPNMHDR lpnmhdr = (LPNMHDR)lParam;
				const SCNotification * p_scn = reinterpret_cast<const SCNotification *>(lParam);
				p_view = static_cast<STextBrowser *>(TView::GetWindowUserData(hWnd));
				if(p_view && p_scn->nmhdr.hwndFrom == p_view->GetSciWnd()) {
					int    test_value = 0; // @debug
					switch(p_scn->nmhdr.code) {
						case SCN_KEY: // @v12.5.7 GTK only (doesn't work in Windows)
							{
								const  int modifiers = (p_scn->ch >> 16);    // Получаем модификаторы (Alt, Ctrl, Shift)
								const  int key_code  = (p_scn->ch & 0xFFFF); // Получаем код клавиши
								if((modifiers & SCMOD_ALT) && oneof4(key_code, SCK_LEFT, SCK_RIGHT, SCK_UP, SCK_DOWN)) {
									// --- Здесь ваша собственная обработка ---
									// Например, вызвать функцию переключения между вкладками
									//MyHandleAltArrow(keyCode); 
									// ---------------------------------------
									// Возвращаем TRUE, чтобы Scintilla НЕ обрабатывала это нажатие дальше 
									::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, TRUE);
									return TRUE;  
								}
							}
							break;
						case SCN_UPDATEUI:
							p_view->RegisterStateEvent(); // @v12.5.11
							StatusWinChange(0, -1);
							break;
						case SCN_CHARADDED:
						case SCN_MODIFIED:
							if(p_scn->modificationType & (SC_MOD_DELETETEXT|SC_MOD_INSERTTEXT|SC_PERFORMED_UNDO|SC_PERFORMED_REDO)) {
								if(!(p_scn->modificationType & SC_STARTACTION)) {
									// @v12.5.11 p_view->Doc.SetState(Document::stDirty, 1);
									p_view->RegisterEditEvent(); // @v12.5.11
								}
							}
							break;
						case SCN_MARGINCLICK:
							{
								/*
								if(p_scn->nmhdr.hwndFrom == _mainEditView.getHSelf())
									switchEditViewTo(MAIN_VIEW);
								else if(p_scn->nmhdr.hwndFrom == _subEditView.getHSelf())
									switchEditViewTo(SUB_VIEW);
								*/
								int line_click = static_cast<int>(p_view->CallFunc(SCI_LINEFROMPOSITION, p_scn->position));
								if(p_scn->margin == scmargeFolder) {
									p_view->MarginClick(p_scn->position, p_scn->modifiers);
									/*
									if(_pDocMap)
										_pDocMap->fold(line_click, p_view->IsFolded(line_click));
									ScintillaEditView * unfocusView = isFromPrimary ? &_subEditView : &_mainEditView;
									_smartHighlighter.highlightView(p_view, unfocusView);
									*/
								}
								else if(p_scn->margin == scmargeSymbole && !p_scn->modifiers) {
									// toggle bookmark
								}
							}
							break;
						case SCN_DWELLSTART:
							{
								test_value = 1;
								if(p_view->SpcMode == spcmSartrTest) {
									SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();
									if(p_srdb) {
										const char * p_wb = " \t.,;:()[]{}/\\!@#$%^&*+=<>\n\r\"\'?";
										const Sci_Position _start_pos = p_scn->position;
										LongArray left, right;
										Sci_Position _pos = _start_pos;
										int    c;
										while((c = p_view->CallFunc(SCI_GETCHARAT, _pos++)) != 0) {
											if(!sstrchr(p_wb, (uchar)c)) {
												right.add(c);
											}
											else
												break;
										}
										if(_start_pos > 0) {
											_pos = _start_pos;
											while((c = p_view->CallFunc(SCI_GETCHARAT, --_pos)) != 0) {
												if(!sstrchr(p_wb, (uchar)c)) {
													left.add(c);
												}
												else
													break;
											}
											left.reverse(0, left.getCount());
										}
										left.add(&right);
										//SCI_CALLTIPSHOW(int posStart, const char *definition)
										if(left.getCount()) {
                                    		SString src_text, text_to_show;
                                    		TSVector <SrWordInfo> info_list;
											for(uint ti = 0; ti < left.getCount(); ti++)
												src_text.CatChar((char)left.at(ti));
											if(p_srdb->GetWordInfo(src_text, 0, info_list) > 0) {
												SString temp_buf;
												for(uint j = 0; j < info_list.getCount(); j++) {
													p_srdb->WordInfoToStr(info_list.at(j), temp_buf);
													if(j)
														text_to_show.CR();
													text_to_show.Cat(temp_buf);
												}
											}
											if(text_to_show.Len())
												p_view->CallFunc(SCI_CALLTIPSHOW, _start_pos, reinterpret_cast<intptr_t>(text_to_show.cptr()));
										}
									}
								}
							}
							break;
						case SCN_DWELLEND:
							{
								p_view->CallFunc(SCI_CALLTIPCANCEL);
							}
							break;
					}
				}
			}
			break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int STextBrowser::UpdateIndicators()
{
	int    ok = -1;
	const  size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
	if(len && SpcMode == spcmSartrTest) {
		SrDatabase * p_srdb = DS.GetTLA().GetSrDatabase();									
		if(p_srdb) {
			SETIFZ(P_Tknzr, new SrSyntaxRuleTokenizer);
			if(P_Tknzr) {
				STokenizer::Param tp;
				P_Tknzr->GetParam(&tp);
				tp.Flags |= STokenizer::fRawOrgOffs;
				P_Tknzr->SetParam(&tp);
				ClearIndicator(indicUnknWord);
				const  uint8 * p_buf = reinterpret_cast<const uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER)); // to get characters directly from Scintilla buffer;
				//int    start_pos = MIN(0, p_scn->position-64);
                SString src_text;
				SString text_to_show;
				SString temp_buf;
				TSVector <SrWordInfo> info_list;
				P_Tknzr->Reset(0);
				P_Tknzr->Write("#00", 0, p_buf, len);
				uint idx_first = 0;
				uint idx_count = 0;
				P_Tknzr->Run(&idx_first, &idx_count);
				STokenizer::Item ti;
				for(uint i = 0; i < idx_count; i++) {
					P_Tknzr->Get(idx_first+i, ti);
					if(ti.Token == STokenizer::tokWord && !ti.Text.IsDec()) {
						if(p_srdb->GetWordInfo(ti.Text, 0, info_list) > 0) {
							/*for(uint j = 0; j < info_list.getCount(); j++) {
								p_srdb->WordInfoToStr(info_list.at(j), temp_buf);
								if(j)
									text_to_show.CR();
								text_to_show.Cat(temp_buf);
							}*/
						}
						else {
							int    start_pos = (int)ti.OrgOffs;
							int    end_pos = 0;
							P_Tknzr->Get(idx_first+i+1, ti);
							end_pos = (int)ti.OrgOffs;
							CallFunc(SCI_SETINDICATORCURRENT, indicUnknWord);
							CallFunc(SCI_INDICATORFILLRANGE, start_pos, end_pos-start_pos);
						}
					}
				}
				{
					const SrSyntaxRuleSet * p_sr = DS.GetSrSyntaxRuleSet();
					TSCollection <SrSyntaxRuleSet::Result> result_list;
					if(p_sr && p_sr->ProcessText(*p_srdb, *(SrSyntaxRuleTokenizer *)P_Tknzr, idx_first, idx_count, result_list) > 0) {
						for(uint residx = 0; residx < result_list.getCount(); residx++) {
							const SrSyntaxRuleSet::Result * p_result = result_list.at(residx);
							if(p_result) {
								int    start_pos = 0;
								int    end_pos = 0;
								P_Tknzr->Get(p_result->TIdxFirst, ti);
								start_pos = (int)ti.OrgOffs;
								P_Tknzr->Get(p_result->TIdxNext, ti);
								end_pos = (int)ti.OrgOffs;
								CallFunc(SCI_SETINDICATORCURRENT, indicStxRule);
								CallFunc(SCI_INDICATORFILLRANGE, start_pos, end_pos-start_pos);
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int STextBrowser::Run()
{
	int    ok = -1;
	//
	return ok;
}

int STextBrowser::GetStatus(StatusBlock * pSb)
{
	int    ok = 1;
	if(pSb) {
		pSb->TextSize = CallFunc(SCI_GETTEXTLENGTH);
		pSb->LineCount = CallFunc(SCI_GETLINECOUNT);
		const int32 pos = GetCurrentPos();
		pSb->LineNo = CallFunc(SCI_LINEFROMPOSITION, pos);
		pSb->ColumnNo = CallFunc(SCI_GETCOLUMN, pos);
		pSb->Cp = Doc.OrgCp;
	}
	return ok;
}

void STextBrowser::Resize()
{
	if(HwndSci != 0) {
		RECT rc;
		::GetWindowRect(H(), &rc);
		if(BbState & bbsInner) {
			;
		}
		else if(::IsWindowVisible(APPL->H_ShortcutsWnd)) {
			RECT sh_rect;
			::GetWindowRect(APPL->H_ShortcutsWnd, &sh_rect);
			rc.bottom -= sh_rect.bottom - sh_rect.top;
		}
		::MoveWindow(HwndSci, 0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, 1);
	}
}

SKeyAccelerator::SKeyAccelerator() : LAssocArray()
{
}

int SKeyAccelerator::Set(const KeyDownCommand & rK, int cmd)
{
	int    ok = 0;
	long   key = rK;
	long   val = 0;
	uint   pos = 0;
	if(Search(key, &val, &pos)) {
		if(cmd > 0) {
			if(cmd != val) {
				at(pos).Val = cmd;
				ok = 2;
			}
			else
				ok = -1;
		}
		else {
			atFree(pos);
			ok = 4;
		}
	}
	else {
		Add(key, cmd, 0);
		ok = 1;
	}
	return ok;
}

/*static*/LRESULT CALLBACK STextBrowser::ScintillaWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	STextBrowser * p_this = reinterpret_cast<STextBrowser *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
	if(p_this) {
		switch(msg) {
			case WM_DESTROY:
				TView::SetWindowProp(p_this->HwndSci, GWLP_WNDPROC, p_this->OrgScintillaWndProc);
				TView::SetWindowProp(p_this->HwndSci, GWLP_USERDATA, nullptr);
				return ::CallWindowProcW(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_CHAR:
				if(p_this->SysState & p_this->sstLastKeyDownConsumed)
					return ::DefWindowProcW(hwnd, msg, wParam, lParam);
				else
					return ::CallWindowProcW(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
				{
					p_this->SysState &= ~p_this->sstLastKeyDownConsumed;
					bool   processed = false;
					KeyDownCommand k;
					k.SetWinMsgCode(wParam);
					if(k.Code == VK_TAB && k.State & KeyDownCommand::stateCtrl) {
						::SendMessageW(p_this->H(), WM_KEYDOWN, wParam, lParam);
						p_this->SysState |= p_this->sstLastKeyDownConsumed;
						processed = true;
					}
					else if((k.State & KeyDownCommand::stateAlt) == KeyDownCommand::stateAlt && oneof4(k.Code, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN)) { // @v12.5.7
						::SendMessageW(p_this->H(), WM_KEYDOWN, wParam, lParam);
						p_this->SysState |= p_this->sstLastKeyDownConsumed;
						processed = true;						
					}
					else if(p_this->KeyAccel.getCount()) {
						long   cmd = 0;
						if(p_this->KeyAccel.BSearch(*reinterpret_cast<const long *>(&k), &cmd, 0)) {
							p_this->SysState |= p_this->sstLastKeyDownConsumed;
							p_this->ProcessCommand(cmd, 0, p_this);
							processed = true;
						}
					}
					return processed ? ::DefWindowProcW(hwnd, msg, wParam, lParam) : ::CallWindowProc(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
				}
				break;
			default:
				return ::CallWindowProcW(p_this->OrgScintillaWndProc, hwnd, msg, wParam, lParam);
		}
	}
	else
		return ::DefWindowProcW(hwnd, msg, wParam, lParam);
};

int STextBrowser::WMHCreate()
{
	const  UiDescription * p_uid = SLS.GetUiDescription(); // @v12.5.3
	RECT   rc;
	GetWindowRect(H(), &rc);
	P_Toolbar = new TToolbar(H(), TBS_NOMOVE);
	if(P_Toolbar && LoadToolbarResource(ToolbarID) > 0) {
		P_Toolbar->Init(ToolbarID, &ToolbarL);
		if(P_Toolbar->IsValid()) {
			RECT tbr;
			::GetWindowRect(P_Toolbar->H(), &tbr);
			ToolBarWidth = tbr.bottom - tbr.top;
		}
	}
	HwndSci = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"Scintilla", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPCHILDREN,
		0, ToolBarWidth, rc.right - rc.left, rc.bottom - rc.top, H(), 0/*(HMENU)GuiID*/, APPL->GetInst(), 0);
	SScEditorBase::Init(HwndSci, 1/*preserveFileName*/);
	TView::SetWindowProp(HwndSci, GWLP_USERDATA, this);
	OrgScintillaWndProc = static_cast<WNDPROC>(TView::SetWindowProp(HwndSci, GWLP_WNDPROC, ScintillaWindowProc));
	// @v8.6.2 (SCI_SETKEYSUNICODE deprecated in sci 3.5.5) CallFunc(SCI_SETKEYSUNICODE, 1, 0);
	CallFunc(SCI_SETCARETLINEVISIBLE, 1);
	CallFunc(SCI_SETCARETLINEBACK, RGB(232,232,255));
	CallFunc(SCI_SETSELBACK, 1, RGB(117,217,117));
	// @v12.5.3 {
	{
		SFontDescr fd_default("Courier New", 10, 0);
		const SFontDescr * p_fd = p_uid ? p_uid->GetFontDescrC("EditorFont") : &fd_default;
		CallFunc(SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)p_fd->Face.cptr());
		CallFunc(SCI_STYLESETSIZE, STYLE_DEFAULT, (LPARAM)p_fd->Size);
	}
	// } @v12.5.3 
	CallFunc(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED); // @v9.8.2 SC_EFF_QUALITY_ANTIALIASED-->SC_EFF_QUALITY_LCD_OPTIMIZED
	// CallFunc(SCI_SETTECHNOLOGY, /*SC_TECHNOLOGY_DIRECTWRITERETAIN*/SC_TECHNOLOGY_DIRECTWRITEDC, 0); // @v9.8.2
	//
	CallFunc(SCI_SETMOUSEDWELLTIME, 500);
	CallFunc(SCI_SETMARGINSENSITIVEN, scmargeFolder, true);
	//
	{
		KeyAccel.clear();
		{
			for(uint i = 0; i < OuterKeyAccel.getCount(); i++) {
				const LAssoc & r_accel_item = OuterKeyAccel.at(i);
				const KeyDownCommand & r_k = *reinterpret_cast<const KeyDownCommand *>(&r_accel_item.Key);
				KeyAccel.Set(r_k, r_accel_item.Val);
			}
		}
		if(P_Toolbar) {
			const uint tbc = P_Toolbar->getItemsCount();
			for(uint i = 0; i < tbc; i++) {
				const ToolbarItem & r_tbi = P_Toolbar->getItem(i);
				if(!(r_tbi.Flags & r_tbi.fHidden) && r_tbi.KeyCode && r_tbi.KeyCode != TV_MENUSEPARATOR && r_tbi.Cmd) {
					KeyDownCommand k;
					if(k.SetTvKeyCode(r_tbi.KeyCode))
						KeyAccel.Set(k, r_tbi.Cmd);
				}
			}
		}
		KeyAccel.Sort();
	}
	if(Doc.OtrIdent.O.IsFullyDefined() && Doc.OtrIdent.P) { // @v12.5.6
		ObjectLoad(Doc.OtrIdent, cpUTF8, 0);
	}
	else {
		FileLoad(Doc.FileName, cpUTF8, 0);
	}
	// @v12.5.11 @construction {
	{
		bool   state_restoration_done = false;
		SJson * p_js = 0;
		{
			LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
			if(p_lstb) {
				LocalStateBinderyCore::StateIdent sident;
				sident.Kind = LocalStateBinderyCore::kEditorPosition;
				Doc.MakeNameSymbol(storagesubjState, true/*withoutExt*/, sident.Symb);
				PPID   _ret_id = 0;
				SString temp_buf;
				SBuffer sbuf;
				SScEditorTextInfo state;
				if(p_lstb->FetchState(sident, &_ret_id, &sbuf)) {
					if(LocalStateBinderyCore::GetStringFromStateBuf(LocalStateBinderyCore::treatbStringUtf8, sbuf, temp_buf)) {
						p_js = SJson::Parse(temp_buf);
						if(p_js) {
							SScEditorTextInfo state;
							if(state.FromJsonObj(p_js)) {
								ApplyPreservedState(state);
								state_restoration_done = true;
							}
						}
					}
				}
			}
		}
		if(!state_restoration_done) {
			SString path;
			MakeBackupPath(Doc, storagesubjState, path);
			if(fileExists(path)) {
				p_js = SJson::ParseFile(path);
				if(p_js) {
					SScEditorTextInfo state;
					if(state.FromJsonObj(p_js)) {
						ApplyPreservedState(state);
						state_restoration_done = true;
					}
				}
			}
		}
		delete p_js;
	}
	// }
	return BIN(P_SciFn && P_SciPtr);
}

SCodepage STextBrowser::SelectEncoding(SCodepage initCp) const
{
	SCodepage result_cp = initCp;
	ListWindow * p_lw = CreateListWindow_Simple(lbtDblClkNotify);
	if(p_lw) {
		ListWindowSmartListBox * p_lb = p_lw->GetListBox();
		if(p_lb) {
			SCodepage cp;
			SString cp_name;
			for(uint i = 0; i < SCodepageIdent::GetRegisteredCodepageCount(); i++) {
				if(SCodepageIdent::GetRegisteredCodepage(i, cp, cp_name.Z()) && cp_name.NotEmpty()) {
					p_lb->addItem(cp, cp_name);
				}
			}
			p_lb->TransmitData(+1, (long *)&result_cp);
			if(ExecView(p_lw) == cmOK) {
				p_lb->TransmitData(-1, (long *)&result_cp);
			}
		}
	}
	else {
		result_cp = cpUndef;
		PPError();
	}
	delete p_lw;
	return result_cp;
}

int STextBrowser::SetEncoding(SCodepage cp)
{
	int    ok = -1;
	if(cp == cpUndef) {
		cp = SelectEncoding(cp);
	}
	if(cp != cpUndef) {
		if(SaveChanges() > 0) {
			if(FileLoad(Doc.FileName, cp, 0))
				ok = 1;
		}
	}
	return ok;
}

int STextBrowser::InsertWorkbookLink()
{
	int    ok = -1;
	{
		PPObjWorkbook wb_obj;
		PPObjWorkbook::SelectLinkBlock link;
		link.Type = PPWBTYP_MEDIA;
		if(wb_obj.SelectLink(&link) > 0 && link.ID) {
			WorkbookTbl::Rec rec, addendum_rec;
			if(wb_obj.Fetch(link.ID, &rec) > 0) {
				SString text;
				if(link.Type == link.ltImage) {
					(text = "#IMAGE").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltRef) {
					(text = "#REF").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltLink) {
					(text = "#LINK").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
				else if(link.Type == link.ltAnnot) {
					if(link.AddendumID && wb_obj.Fetch(link.AddendumID, &addendum_rec) > 0) {
						text = "#ANNOTIMG";
						(text = "#ANNOTIMG").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatDiv(',', 2).
							CatChar('\'').Cat(addendum_rec.Symb).CatChar('\'').CatChar(')');
					}
					else {
						(text = "#ANNOT").CatChar('(').CatChar('\'').Cat(rec.Symb).CatChar('\'').CatChar(')');
					}
					text.Transf(CTRANSF_INNER_TO_UTF8);
					CallFunc(SCI_INSERTTEXT, -1, reinterpret_cast<intptr_t>(text.cptr()));
				}
			}
		}
	}
	return ok;
}
//
//
//
int STextBrowser::SetIdlePeriod(long periodMs) // @v12.5.11
{
	if(periodMs >= 0 && periodMs < 86400L) {
		IdleTimer.Restart(static_cast<uint32>(periodMs) * 1000);
		return 1;
	}
	else
		return 0;
}

int STextBrowser::ProcessCommand(uint ppvCmd, const void * pHdr, void * pBrw)
{
	int    ok = -2;
	switch(ppvCmd) {
		case PPVCMD_OPEN:
			{
				SString file_name(Doc.FileName);
				if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, file_name, 0, H()) > 0)
					ok = FileLoad(file_name, cpUTF8, 0);
			}
			break;
		case PPVCMD_SAVE: 
			if(Doc.OtrIdent.O.IsFullyDefined() && Doc.OtrIdent.P) {
				ok = ObjectSave(Doc.OtrIdent, 0); 
			}
			else {
				ok = FileSave(0, 0); 
			}
			break;
		case PPVCMD_SAVEAS: 
			ok = FileSave(0, ofInteractiveSaveAs); 
			break;
		case PPVCMD_SELCODEPAGE: ok = SetEncoding(cpUndef); break;
		case PPVCMD_PROCESSTEXT:
			/* @v10.9.1
			{
				uint8 * p_buf = reinterpret_cast<uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER, 0, 0));
				const size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
				TidyProcessBlock blk;
				blk.InputBuffer.Set(p_buf, len);
				blk.TidyOptions.Add(TidyInCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyOutCharEncoding, "utf8");
				blk.TidyOptions.Add(TidyWrapLen, "200");
				blk.TidyOptions.Add(TidyIndentContent, "yes");
				blk.TidyOptions.Add(TidyIndentSpaces, "4");
				blk.TidyOptions.Add(TidyBodyOnly, "yes");
				if(TidyProcessText(blk) > 0) {
					CallFunc(SCI_CLEARALL, 0, 0);
					CallFunc(SCI_APPENDTEXT, (int)blk.Output.Len(), reinterpret_cast<intptr_t>(blk.Output.cptr()));
					ok = 1;
				}
			}*/
			break;
		case PPVCMD_SEARCH: SearchAndReplace(srfUseDialog); break;
		case PPVCMD_SEARCHNEXT: SearchAndReplace(0); break;
		case PPVCMD_INSERTLINK: InsertWorkbookLink(); break;
		case PPVCMD_BRACEHTMLTAG: BraceHtmlTag(); break;
		case PPVCMD_SETUPSARTREINDICATORS: UpdateIndicators(); break;
		case PPVCMD_RUN: Run(); break;
	}
	return ok;
}

int STextBrowser::SaveChanges()
{
	int    ok = 1;
	if(Doc.State & Document::stDirty) {
		if(CONFIRM(PPCFM_DATACHANGED))
			ok = FileSave(0, 0);
		else
			ok = -1;
	}
	return ok;
}

int STextBrowser::FileClose()
{
	return SScEditorBase::Release();
}

int STextBrowser::ObjectLoad(const SObjTextRefIdent & rIdent, SCodepage cp, long flags) // @v12.5.6
{
	int    ok = -1;
	if(rIdent.O.IsFullyDefined() && rIdent.P) {
		SlExtraProcBlock epb;
		SLS.GetExtraProcBlock(&epb);
		if(epb.F_LoadObjText) {
			SString text_buf;
			int    r = epb.F_LoadObjText(rIdent, text_buf, 0);
			if(r != 0) {
				Doc.SciDoc = reinterpret_cast<SScEditorBase::SciDocument>(CallFunc(SCI_CREATEDOCUMENT, 0, 0));
				//Setup scratchtilla for new filedata
				CallFunc(SCI_SETSTATUS, SC_STATUS_OK, 0); // reset error status
				CallFunc(SCI_SETDOCPOINTER, 0, (int)Doc.SciDoc);
				const int ro = CallFunc(SCI_GETREADONLY, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 0, 0);
				}
				CallFunc(SCI_CLEARALL, 0, 0);
				//
				// Здесь следует установить LEXER
				//
				const char * p_lexer_symb = 0;
				if(LexerSymb.NotEmpty()) {
					p_lexer_symb = LexerSymb.cptr();
				}
				if(!isempty(p_lexer_symb)) {
					SetLexer(p_lexer_symb);
				}
				CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
				CallFunc(SCI_ALLOCATE, static_cast<WPARAM>(text_buf.Len()), 0);
				THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
				{
					{
						{
							Doc.OrgCp = cpUTF8;
							Doc.Cp = cpUTF8;
						}
						CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
						Doc.Eolf = eolWindows;
						{
							int    sci_eol = SC_EOL_CRLF;
							if(Doc.Eolf == eolWindows)
								sci_eol = SC_EOL_CRLF;
							else if(Doc.Eolf == eolUnix)
								sci_eol = SC_EOL_LF;
							else if(Doc.Eolf == eolMac)
								sci_eol = SC_EOL_CR;
							CallFunc(SCI_SETEOLMODE, sci_eol, 0);
						}
					}
					if(Doc.OrgCp == cpUTF8) {
						// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
						CallFunc(SCI_APPENDTEXT, text_buf.Len(), reinterpret_cast<intptr_t>(text_buf.cptr()));
					}
					else {
						SStringU ubuf;
						SString utfbuf;
						ubuf.CopyFromMb(Doc.OrgCp, text_buf, text_buf.Len());
						ubuf.CopyToUtf8(utfbuf, 0);
						CallFunc(SCI_APPENDTEXT, utfbuf.Len(), reinterpret_cast<intptr_t>(utfbuf.cptr()));
					}
					THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
				}
				CallFunc(SCI_EMPTYUNDOBUFFER, 0, 0);
				CallFunc(SCI_SETSAVEPOINT, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 1, 0);
				}
				Doc.SetState(Document::stDirty, 0);
				ok = 1;
			}
		}
	}
	CATCH
		Doc.Reset(0);
		ok = 0;
	ENDCATCH
	return ok;
}

int STextBrowser::FileLoad(const char * pFileName, SCodepage orgCp, long flags)
{
	int    ok = 1;
	SString file_name;
	(file_name = pFileName).Strip();
	THROW_SL(fileExists(file_name));
	{
		SFileFormat ff;
		const int fir = ff.Identify(file_name);
		size_t block_size = SMEGABYTE(8);
		int64  _fsize = 0;
		SFile _f(file_name, SFile::mRead|SFile::mBinary);
		THROW_SL(_f.IsValid());
		THROW_SL(_f.CalcSize(&_fsize));
		{
			const uint64 bufsize_req = _fsize + MIN(1<<20, _fsize/6);
			THROW(bufsize_req <= SMEGABYTELL(1025));
			{
				Doc.SciDoc = reinterpret_cast<SScEditorBase::SciDocument>(CallFunc(SCI_CREATEDOCUMENT, 0, 0));
				//Setup scratchtilla for new filedata
				CallFunc(SCI_SETSTATUS, SC_STATUS_OK, 0); // reset error status
				CallFunc(SCI_SETDOCPOINTER, 0, (int)Doc.SciDoc);
				const int ro = CallFunc(SCI_GETREADONLY, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 0, 0);
				}
				CallFunc(SCI_CLEARALL, 0, 0);
				//
				// Здесь следует установить LEXER
				//
				const char * p_lexer_symb = 0;
				if(LexerSymb.NotEmpty()) {
					p_lexer_symb = LexerSymb.cptr();
				}
				else if(oneof3(fir, 1, 2, 3)) {
					if(ff == ff.Ini) {
						p_lexer_symb = "ini";
					}
					else if(ff == ff.Xml) {
						p_lexer_symb = "xml";
					}
					else if(oneof3(ff, ff.C, ff.CPP, ff.H)) {
						p_lexer_symb = "cpp";
					}
					else if(ff == ff.Gravity) {
					}
				}
				if(!isempty(p_lexer_symb)) {
					SetLexer(p_lexer_symb);
				}
				CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
				CallFunc(SCI_ALLOCATE, static_cast<WPARAM>(bufsize_req), 0);
				THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
				{
					int    first_block = 1;
					STextEncodingStat tes(STextEncodingStat::fUseUCharDet);
					SStringU ubuf;
					SString utfbuf;
					size_t incomplete_multibyte_char = 0;
					size_t actual_size = 0;
					int64  _fsize_rest = _fsize;
					STempBuffer buffer(block_size+8);
					THROW_SL(buffer.IsValid());
					while(_fsize_rest > 0) {
						actual_size = 0;
						THROW_SL(_f.Read(buffer+incomplete_multibyte_char, block_size-incomplete_multibyte_char, &actual_size));
						_fsize_rest -= actual_size;
						actual_size += incomplete_multibyte_char;
						if(first_block) {
							tes.Add(buffer, actual_size);
							tes.Finish();
							if(tes.CheckFlag(tes.fLegalUtf8Only)) {
								if(_fsize_rest > 0) {
									//
									// Если все символы первого блока utf8, но проанализирован не весь
									// файл, то исходной кодовой страницей должна быть заданная из-вне (если определена).
									// Ибо, попытавшись установить utf8 как исходную страницу мы рискуем
									// исказить символы в следующих блоках файла.
									//
									Doc.OrgCp = (orgCp == cpUndef) ? cpUTF8 : orgCp;
								}
								else {
									//
									// Если мы проанализировали весь файл и все символы - utf8, то
									// можно смело устанавливать исходную страницу как utf8
									//
									Doc.OrgCp = cpUTF8;
								}
								Doc.Cp = cpUTF8;
							}
							else {
								//
								// Экспериментальный блок с автоматической идентификацией кодировки.
								// Требуются уточнения.
								//
								SCodepageIdent local_cp = tes.GetAutodetectedCp();
								Doc.OrgCp = (local_cp == cpUndef) ? ((orgCp == cpUndef) ? cpUTF8 : orgCp) : local_cp;
								Doc.Cp = cpUTF8;
							}
							Doc.Eolf = tes.GetEolFormat();
							CallFunc(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
							{
								int    sci_eol = SC_EOL_CRLF;
								if(Doc.Eolf == eolWindows)
									sci_eol = SC_EOL_CRLF;
								else if(Doc.Eolf == eolUnix)
									sci_eol = SC_EOL_LF;
								else if(Doc.Eolf == eolMac)
									sci_eol = SC_EOL_CR;
								CallFunc(SCI_SETEOLMODE, sci_eol, 0);
							}
							first_block = 0;
						}
						if(Doc.OrgCp == cpUTF8) {
							// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
							CallFunc(SCI_APPENDTEXT, actual_size, reinterpret_cast<intptr_t>(buffer.cptr()));
						}
						else {
							ubuf.CopyFromMb(Doc.OrgCp, buffer, actual_size);
							ubuf.CopyToUtf8(utfbuf, 0);
							CallFunc(SCI_APPENDTEXT, utfbuf.Len(), reinterpret_cast<intptr_t>(utfbuf.cptr()));
						}
						THROW(CallFunc(SCI_GETSTATUS, 0, 0) == SC_STATUS_OK);
					}
				}
				CallFunc(SCI_EMPTYUNDOBUFFER, 0, 0);
				CallFunc(SCI_SETSAVEPOINT, 0, 0);
				if(ro) {
					CallFunc(SCI_SETREADONLY, 1, 0);
				}
				Doc.SetState(Document::stDirty, 0);
				//CallFunc(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			}
		}
	}
	CATCH
		Doc.Reset(0);
		ok = 0;
	ENDCATCH
	return ok;
}

int STextBrowser::ObjectSave(const SObjTextRefIdent & rIdent, long flags) // @v12.5.6
{
	int    ok = -1;
	if(rIdent.O.IsFullyDefined() && rIdent.P) {
		SlExtraProcBlock epb;
		SLS.GetExtraProcBlock(&epb);
		if(epb.F_StoreObjText) {
			const  uint8 * p_buf = reinterpret_cast<const uint8 *>(CallFunc(SCI_GETCHARACTERPOINTER, 0, 0)); // to get characters directly from Scintilla buffer;
			const  size_t len = static_cast<size_t>(CallFunc(SCI_GETLENGTH));
			const int r = epb.F_StoreObjText(rIdent, p_buf, len, 0);
			if(r != 0) {
				Doc.SetState(Document::stDirty, 0);
				ok = r;
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int STextBrowser::FileSave(const char * pFileName, long flags)
{
	int    ok = -1;
	bool   skip = false;
	SString path(isempty(pFileName) ? Doc.FileName.cptr() : pFileName);
	if((flags & ofInteractiveSaveAs) || !path.NotEmptyS()) {
		if(PPOpenFile(PPTXT_TEXTBROWSER_FILETYPES, path, ofilfNExist, H()) > 0) {
			;
		}
		else
			skip = true;
	}
	if(!skip) {
		THROW(SScEditorBase::StoreText(path));
		Doc.FileName = path;
		Doc.SetState(Document::stDirty, 0);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int STextBrowser::DoAutosaveByIdle(int kind)
{
	const  long   edit_event_idle_timeout = 3; // seconds
	const  long   state_event_idle_timeout = 3; // seconds
	int    ok = -1;
	const  LDATETIME now_dtm = getcurdatetime_();
	if(!!ASSt.LastEditEventMoment) {
		if(diffdatetimesec(now_dtm, ASSt.LastEditEventMoment) > edit_event_idle_timeout) {
			if(Cfg.Flags & Config::fAutoBackupText) {
				if(!ASSt.LastBackupTextMoment || diffdatetimesec(ASSt.LastEditEventMoment, ASSt.LastBackupTextMoment) > 0) {
					if(DoBackup(storagesubjText) > 0) {
						ASSt.LastBackupTextMoment = getcurdatetime_();
						ok = 1;
					}
				}
			}
			if(Cfg.Flags & Config::fAutoSaveText) {
				if(!ASSt.LastAutoSaveTextMoment || diffdatetimesec(ASSt.LastEditEventMoment, ASSt.LastAutoSaveTextMoment) > 0) {
					if(Doc.OtrIdent.O.IsFullyDefined() && Doc.OtrIdent.P) {
						if(ObjectSave(Doc.OtrIdent, 0)) {
							ASSt.LastAutoSaveTextMoment = getcurdatetime_();
							ok = 1;
						}
						else {
							ok = 0;
						}
					}
					else {
						if(FileSave(0, 0)) {
							ASSt.LastAutoSaveTextMoment = getcurdatetime_();
							ok = 1;
						}
						else {
							ok = 0;
						}
					}
				}
			}
		}
	}
	if(!!ASSt.LastUiEventMoment) {
		if(diffdatetimesec(now_dtm, ASSt.LastUiEventMoment) > state_event_idle_timeout) {
			if(Cfg.Flags & Config::fAutoBackupState) {
				if(!ASSt.LastBackupStateMoment || diffdatetimesec(ASSt.LastUiEventMoment, ASSt.LastBackupStateMoment) > 0) {
					if(DoBackup(storagesubjState) > 0) {
						ASSt.LastBackupStateMoment = getcurdatetime_();
						if(ok == 1)
							ok = 3;
						else
							ok = 2;
					}
				}
			}
			if(Cfg.Flags & Config::fAutoSaveState) {
				if(!ASSt.LastAutoSaveStateMoment || diffdatetimesec(ASSt.LastUiEventMoment, ASSt.LastAutoSaveStateMoment) > 0) {
					LocalStateBinderyCore * p_lstb = DS.GetTLA().GetLocalStateBindery();
					if(p_lstb) {
						LocalStateBinderyCore::StateIdent sident;
						sident.Kind = LocalStateBinderyCore::kEditorPosition;
						Doc.MakeNameSymbol(storagesubjState, true/*withoutExt*/, sident.Symb);
						PPID   _ret_id = 0;
						SString temp_buf;
						SScEditorTextInfo state;
						if(GetCurrentState(state)) {
							SJson * p_js = state.ToJsonObj();
							if(p_js && p_js->ToStr(temp_buf)) {
								SBuffer sbuf;
								if(LocalStateBinderyCore::PutStringToStateBuf(LocalStateBinderyCore::treatbStringUtf8, sbuf, temp_buf)) {
									if(p_lstb->RegisterState(&_ret_id, sident, sbuf, 1)) {
										ASSt.LastAutoSaveStateMoment = getcurdatetime_();
										ok = 1;
									}
								}
							}
							ZDELETE(p_js);
						}
					}
				}
			}
		}
	}
	return ok;
}

int STextBrowser::CmpFileName(const char * pFileName)
{
	if(Doc.FileName.NotEmpty())
		return Doc.FileName.Cmp(pFileName, 1);
	// @v12.5.6 {
	else {
		SString temp_buf;
		return Doc.OtrIdent.ToStr(temp_buf).Cmp(pFileName, 1);
	}
	// } @v12.5.6 
}

int STextBrowser::BraceHtmlTag()
{
	int    ok = -1;
	SString tag, text;
	TDialog * dlg = new TDialog(DLG_SELHTMLTAG);
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
		if(ExecView(dlg) == cmOK) {
			dlg->getCtrlString(CTL_SELHTMLTAG_TAGTEXT, tag);
			if(tag.NotEmptyS()) {
				IntRange sel_range;
				GetSelection(sel_range);
				if(sel_range.low >= 0 && sel_range.upp >= 0) {
					if(tag == "*") { // comment
						text.Z().Cat("-->");
						CallFunc(SCI_INSERTTEXT, sel_range.upp, reinterpret_cast<intptr_t>(text.cptr()));
						text.Z().Cat("<!--");
						CallFunc(SCI_INSERTTEXT, sel_range.low, reinterpret_cast<intptr_t>(text.cptr()));
					}
					else {
						text.Z().CatChar('<').Slash().Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.upp, reinterpret_cast<intptr_t>(text.cptr()));
						text.Z().CatChar('<').Cat(tag).CatChar('>');
						CallFunc(SCI_INSERTTEXT, sel_range.low, reinterpret_cast<intptr_t>(text.cptr()));
					}
					ok = 1;
				}
			}
		}
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}
