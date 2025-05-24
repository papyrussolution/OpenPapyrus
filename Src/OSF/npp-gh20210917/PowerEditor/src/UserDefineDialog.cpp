// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <npp-internal.h>
#pragma hdrstop

using namespace std;

UserLangContainer * SharedParametersDialog::_pUserLang = NULL;
ScintillaEditView * SharedParametersDialog::_pScintilla = NULL;

GlobalMappers::GlobalMappers()
{
	// pre 2.0
	temp[TEXT("Operators")]             = SCE_USER_KWLIST_OPERATORS1;
	temp[TEXT("Folder+")]               = SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;
	temp[TEXT("Folder-")]               = SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;
	temp[TEXT("Words1")]                = SCE_USER_KWLIST_KEYWORDS1;
	temp[TEXT("Words2")]                = SCE_USER_KWLIST_KEYWORDS2;
	temp[TEXT("Words3")]                = SCE_USER_KWLIST_KEYWORDS3;
	temp[TEXT("Words4")]                = SCE_USER_KWLIST_KEYWORDS4;
	// in case of duplicate entries, newer string should overwrite old one !
	for(iter = temp.begin(); iter != temp.end(); ++iter)
		keywordNameMapper[iter->second] = iter->first;
	keywordIdMapper.insert(temp.begin(), temp.end());
	temp.clear();
	// 2.0
	temp[TEXT("Comments")]              = SCE_USER_KWLIST_COMMENTS;
	temp[TEXT("Numbers, additional")]   = SCE_USER_KWLIST_NUMBER_RANGE;
	temp[TEXT("Numbers, prefixes")]     = SCE_USER_KWLIST_NUMBER_PREFIX2;
	temp[TEXT("Numbers, extras with prefixes")] = SCE_USER_KWLIST_NUMBER_EXTRAS2;
	temp[TEXT("Numbers, suffixes")]     = SCE_USER_KWLIST_NUMBER_SUFFIX2;
	temp[TEXT("Operators1")]            = SCE_USER_KWLIST_OPERATORS1;
	temp[TEXT("Operators2")]            = SCE_USER_KWLIST_OPERATORS2;
	temp[TEXT("Folders in code1, open")]        = SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;
	temp[TEXT("Folders in code1, middle")]      = SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE;
	temp[TEXT("Folders in code1, close")]       = SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;
	temp[TEXT("Folders in code2, open")]        = SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN;
	temp[TEXT("Folders in code2, middle")]      = SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE;
	temp[TEXT("Folders in code2, close")]       = SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE;
	temp[TEXT("Folders in comment, open")]      = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN;
	temp[TEXT("Folders in comment, middle")]    = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE;
	temp[TEXT("Folders in comment, close")]     = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE;
	temp[TEXT("Keywords1")]             = SCE_USER_KWLIST_KEYWORDS1;
	temp[TEXT("Keywords2")]             = SCE_USER_KWLIST_KEYWORDS2;
	temp[TEXT("Keywords3")]             = SCE_USER_KWLIST_KEYWORDS3;
	temp[TEXT("Keywords4")]             = SCE_USER_KWLIST_KEYWORDS4;
	temp[TEXT("Keywords5")]             = SCE_USER_KWLIST_KEYWORDS5;
	temp[TEXT("Keywords6")]             = SCE_USER_KWLIST_KEYWORDS6;
	temp[TEXT("Keywords7")]             = SCE_USER_KWLIST_KEYWORDS7;
	temp[TEXT("Keywords8")]             = SCE_USER_KWLIST_KEYWORDS8;
	temp[TEXT("Delimiters")]            = SCE_USER_KWLIST_DELIMITERS;

	// in case of duplicate entries, newer string should overwrite old one !
	for(iter = temp.begin(); iter != temp.end(); ++iter)
		keywordNameMapper[iter->second] = iter->first;
	keywordIdMapper.insert(temp.begin(), temp.end());
	temp.clear();

	// 2.1
	temp[TEXT("Numbers, prefix1")]      = SCE_USER_KWLIST_NUMBER_PREFIX1;
	temp[TEXT("Numbers, prefix2")]      = SCE_USER_KWLIST_NUMBER_PREFIX2;
	temp[TEXT("Numbers, extras1")]      = SCE_USER_KWLIST_NUMBER_EXTRAS1;
	temp[TEXT("Numbers, extras2")]      = SCE_USER_KWLIST_NUMBER_EXTRAS2;
	temp[TEXT("Numbers, suffix1")]      = SCE_USER_KWLIST_NUMBER_SUFFIX1;
	temp[TEXT("Numbers, suffix2")]      = SCE_USER_KWLIST_NUMBER_SUFFIX2;
	temp[TEXT("Numbers, range")]        = SCE_USER_KWLIST_NUMBER_RANGE;

	// in case of duplicate entries, newer string should overwrite old one !
	for(iter = temp.begin(); iter != temp.end(); ++iter)
		keywordNameMapper[iter->second] = iter->first;
	keywordIdMapper.insert(temp.begin(), temp.end());
	temp.clear();

	// pre 2.0
	temp[TEXT("FOLDEROPEN")]   = SCE_USER_STYLE_FOLDER_IN_CODE1;
	temp[TEXT("FOLDERCLOSE")]  = SCE_USER_STYLE_FOLDER_IN_CODE1;
	temp[TEXT("KEYWORD1")]     = SCE_USER_STYLE_KEYWORD1;
	temp[TEXT("KEYWORD2")]     = SCE_USER_STYLE_KEYWORD2;
	temp[TEXT("KEYWORD3")]     = SCE_USER_STYLE_KEYWORD3;
	temp[TEXT("KEYWORD4")]     = SCE_USER_STYLE_KEYWORD4;
	temp[TEXT("COMMENT")]      = SCE_USER_STYLE_COMMENT;
	temp[TEXT("COMMENT LINE")] = SCE_USER_STYLE_COMMENTLINE;
	temp[TEXT("NUMBER")]       = SCE_USER_STYLE_NUMBER;
	temp[TEXT("OPERATOR")]     = SCE_USER_STYLE_OPERATOR;
	temp[TEXT("DELIMINER1")]   = SCE_USER_STYLE_DELIMITER1;
	temp[TEXT("DELIMINER2")]   = SCE_USER_STYLE_DELIMITER2;
	temp[TEXT("DELIMINER3")]   = SCE_USER_STYLE_DELIMITER3;

	// in case of duplicate entries, newer string should overwrite old one !
	for(iter = temp.begin(); iter != temp.end(); ++iter)
		styleNameMapper[iter->second] = iter->first;
	styleIdMapper.insert(temp.begin(), temp.end());
	temp.clear();

	// post 2.0
	temp[TEXT("DEFAULT")]      = SCE_USER_STYLE_DEFAULT;
	temp[TEXT("COMMENTS")]     = SCE_USER_STYLE_COMMENT;
	temp[TEXT("LINE COMMENTS")]        = SCE_USER_STYLE_COMMENTLINE;
	temp[TEXT("NUMBERS")]      = SCE_USER_STYLE_NUMBER;
	temp[TEXT("KEYWORDS1")]    = SCE_USER_STYLE_KEYWORD1;
	temp[TEXT("KEYWORDS2")]    = SCE_USER_STYLE_KEYWORD2;
	temp[TEXT("KEYWORDS3")]    = SCE_USER_STYLE_KEYWORD3;
	temp[TEXT("KEYWORDS4")]    = SCE_USER_STYLE_KEYWORD4;
	temp[TEXT("KEYWORDS5")]    = SCE_USER_STYLE_KEYWORD5;
	temp[TEXT("KEYWORDS6")]    = SCE_USER_STYLE_KEYWORD6;
	temp[TEXT("KEYWORDS7")]    = SCE_USER_STYLE_KEYWORD7;
	temp[TEXT("KEYWORDS8")]    = SCE_USER_STYLE_KEYWORD8;
	temp[TEXT("OPERATORS")]    = SCE_USER_STYLE_OPERATOR;
	temp[TEXT("FOLDER IN CODE1")]      = SCE_USER_STYLE_FOLDER_IN_CODE1;
	temp[TEXT("FOLDER IN CODE2")]      = SCE_USER_STYLE_FOLDER_IN_CODE2;
	temp[TEXT("FOLDER IN COMMENT")]    = SCE_USER_STYLE_FOLDER_IN_COMMENT;
	temp[TEXT("DELIMITERS1")]  = SCE_USER_STYLE_DELIMITER1;
	temp[TEXT("DELIMITERS2")]  = SCE_USER_STYLE_DELIMITER2;
	temp[TEXT("DELIMITERS3")]  = SCE_USER_STYLE_DELIMITER3;
	temp[TEXT("DELIMITERS4")]  = SCE_USER_STYLE_DELIMITER4;
	temp[TEXT("DELIMITERS5")]  = SCE_USER_STYLE_DELIMITER5;
	temp[TEXT("DELIMITERS6")]  = SCE_USER_STYLE_DELIMITER6;
	temp[TEXT("DELIMITERS7")]  = SCE_USER_STYLE_DELIMITER7;
	temp[TEXT("DELIMITERS8")]  = SCE_USER_STYLE_DELIMITER8;

	// in case of duplicate entries, newer string should overwrite old one !
	for(iter = temp.begin(); iter != temp.end(); ++iter)
		styleNameMapper[iter->second] = iter->first;
	styleIdMapper.insert(temp.begin(), temp.end());
	temp.clear();

	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER1]      = SCE_USER_MASK_NESTING_DELIMITER1;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER2]      = SCE_USER_MASK_NESTING_DELIMITER2;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER3]      = SCE_USER_MASK_NESTING_DELIMITER3;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER4]      = SCE_USER_MASK_NESTING_DELIMITER4;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER5]      = SCE_USER_MASK_NESTING_DELIMITER5;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER6]      = SCE_USER_MASK_NESTING_DELIMITER6;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER7]      = SCE_USER_MASK_NESTING_DELIMITER7;
	nestingMapper[IDC_STYLER_CHECK_NESTING_DELIMITER8]      = SCE_USER_MASK_NESTING_DELIMITER8;
	nestingMapper[IDC_STYLER_CHECK_NESTING_COMMENT] = SCE_USER_MASK_NESTING_COMMENT;
	nestingMapper[IDC_STYLER_CHECK_NESTING_COMMENT_LINE]    = SCE_USER_MASK_NESTING_COMMENT_LINE;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD1]        = SCE_USER_MASK_NESTING_KEYWORD1;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD2]        = SCE_USER_MASK_NESTING_KEYWORD2;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD3]        = SCE_USER_MASK_NESTING_KEYWORD3;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD4]        = SCE_USER_MASK_NESTING_KEYWORD4;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD5]        = SCE_USER_MASK_NESTING_KEYWORD5;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD6]        = SCE_USER_MASK_NESTING_KEYWORD6;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD7]        = SCE_USER_MASK_NESTING_KEYWORD7;
	nestingMapper[IDC_STYLER_CHECK_NESTING_KEYWORD8]        = SCE_USER_MASK_NESTING_KEYWORD8;
	nestingMapper[IDC_STYLER_CHECK_NESTING_OPERATORS1]      = SCE_USER_MASK_NESTING_OPERATORS1;
	nestingMapper[IDC_STYLER_CHECK_NESTING_OPERATORS2]      = SCE_USER_MASK_NESTING_OPERATORS2;
	nestingMapper[IDC_STYLER_CHECK_NESTING_NUMBERS] = SCE_USER_MASK_NESTING_NUMBERS;

	dialogMapper[IDC_NUMBER_PREFIX1_EDIT]   = SCE_USER_KWLIST_NUMBER_PREFIX1;
	dialogMapper[IDC_NUMBER_PREFIX2_EDIT]   = SCE_USER_KWLIST_NUMBER_PREFIX2;
	dialogMapper[IDC_NUMBER_EXTRAS1_EDIT]   = SCE_USER_KWLIST_NUMBER_EXTRAS1;
	dialogMapper[IDC_NUMBER_EXTRAS2_EDIT]   = SCE_USER_KWLIST_NUMBER_EXTRAS2;
	dialogMapper[IDC_NUMBER_SUFFIX1_EDIT]   = SCE_USER_KWLIST_NUMBER_SUFFIX1;
	dialogMapper[IDC_NUMBER_SUFFIX2_EDIT]   = SCE_USER_KWLIST_NUMBER_SUFFIX2;
	dialogMapper[IDC_NUMBER_RANGE_EDIT]     = SCE_USER_KWLIST_NUMBER_RANGE;

	dialogMapper[IDC_FOLDER_IN_CODE1_OPEN_EDIT] = SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN;
	dialogMapper[IDC_FOLDER_IN_CODE1_MIDDLE_EDIT]       = SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE;
	dialogMapper[IDC_FOLDER_IN_CODE1_CLOSE_EDIT]        = SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE;
	dialogMapper[IDC_FOLDER_IN_CODE2_OPEN_EDIT] = SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN;
	dialogMapper[IDC_FOLDER_IN_CODE2_MIDDLE_EDIT]       = SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE;
	dialogMapper[IDC_FOLDER_IN_CODE2_CLOSE_EDIT]        = SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE;
	dialogMapper[IDC_FOLDER_IN_COMMENT_OPEN_EDIT]       = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN;
	dialogMapper[IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT] = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE;
	dialogMapper[IDC_FOLDER_IN_COMMENT_CLOSE_EDIT]  = SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE;

	dialogMapper[IDC_KEYWORD1_EDIT]         = SCE_USER_KWLIST_KEYWORDS1;
	dialogMapper[IDC_KEYWORD2_EDIT]         = SCE_USER_KWLIST_KEYWORDS2;
	dialogMapper[IDC_KEYWORD3_EDIT]         = SCE_USER_KWLIST_KEYWORDS3;
	dialogMapper[IDC_KEYWORD4_EDIT]         = SCE_USER_KWLIST_KEYWORDS4;
	dialogMapper[IDC_KEYWORD5_EDIT]         = SCE_USER_KWLIST_KEYWORDS5;
	dialogMapper[IDC_KEYWORD6_EDIT]         = SCE_USER_KWLIST_KEYWORDS6;
	dialogMapper[IDC_KEYWORD7_EDIT]         = SCE_USER_KWLIST_KEYWORDS7;
	dialogMapper[IDC_KEYWORD8_EDIT]         = SCE_USER_KWLIST_KEYWORDS8;

	setLexerMapper[SCE_USER_KWLIST_COMMENTS]                    = "userDefine.comments";
	setLexerMapper[SCE_USER_KWLIST_DELIMITERS]                  = "userDefine.delimiters";
	setLexerMapper[SCE_USER_KWLIST_OPERATORS1]                  = "userDefine.operators1";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_PREFIX1]              = "userDefine.numberPrefix1";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_PREFIX2]              = "userDefine.numberPrefix2";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_EXTRAS1]              = "userDefine.numberExtras1";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_EXTRAS2]              = "userDefine.numberExtras2";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_SUFFIX1]              = "userDefine.numberSuffix1";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_SUFFIX2]              = "userDefine.numberSuffix2";
	setLexerMapper[SCE_USER_KWLIST_NUMBER_RANGE]                = "userDefine.numberRange";
	setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN]       = "userDefine.foldersInCode1Open";
	setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE] = "userDefine.foldersInCode1Middle";
	setLexerMapper[SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE]      = "userDefine.foldersInCode1Close";
};

GlobalMappers & globalMappper()
{
	// getting global object as reference to static ensures that only one object is created
	// even if called from different code units
	static GlobalMappers gm;
	return gm;
}

void convertTo(TCHAR * dest, int destLen, const TCHAR * toConvert, TCHAR * prefix)
{
	bool inGroup = false;
	int index = lstrlen(dest);
	if(index > 0)
		dest[index++] = ' ';
	dest[index++] = prefix[0];
	dest[index++] = prefix[1];

	for(size_t i = 0, len = lstrlen(toConvert); i < len && index < destLen - 7; ++i) {
		if(i == 0 && toConvert[i] == '(' && toConvert[i + 1] == '(') {
			inGroup = true;
		}
		else if(toConvert[i] == ' ' && toConvert[i + 1] == '(' && toConvert[i + 2] == '(') {
			inGroup = true;
			dest[index++] = ' ';
			dest[index++] = prefix[0];
			dest[index++] = prefix[1];
			++i;    // skip space
		}

		if(inGroup && toConvert[i - 1] == ')' && toConvert[i - 2] == ')') {
			inGroup = false;
		}

		if(toConvert[i] == ' ') {
			if(toConvert[i + 1] != ' ' && toConvert[i + 1] != '\0') {
				dest[index++] = ' ';
				if(!inGroup) {
					dest[index++] = prefix[0];
					dest[index++] = prefix[1];
				}
			}
		}
		else {
			dest[index++] = toConvert[i];
		}
	}
	dest[index] = '\0';
};

bool SharedParametersDialog::setPropertyByCheck(HWND hwnd, WPARAM id, bool & bool2set)
{
	bool2set = (BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, int(id)), BM_GETCHECK, 0, 0));

	if(_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
		_pScintilla->styleChange();

	return TRUE;
}

INT_PTR CALLBACK SharedParametersDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch(Message)
	{
		case WM_INITDIALOG:
	    {
		    // initControls();
		    return TRUE;
	    }

		case WM_CTLCOLOREDIT:
	    {
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		    }
		    break;
	    }

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
	    {
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
	    }

		case WM_PRINTCLIENT:
	    {
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
	    }

		case WM_COMMAND:
	    {
		    if(HIWORD(wParam) == EN_CHANGE) {
			    setKeywords2List(LOWORD(wParam));

			    if(_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
				    _pScintilla->styleChange();

			    return TRUE;
		    }
		    return FALSE;
	    }
	}
	return FALSE;
}

INT_PTR CALLBACK FolderStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message) {
		case WM_INITDIALOG:
	    {
		    _pageLink.init(_hInst, _hSelf);
		    _pageLink.create(::GetDlgItem(_hSelf, IDC_WEB_HELP_LINK), TEXT("https://ivan-radic.github.io/udl-documentation/"));
		    return TRUE;
	    }
		case WM_COMMAND:
	    {
		    switch(wParam) {
			    case IDC_FOLDER_FOLD_COMPACT:
			{
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_foldCompact);
			}
			    case IDC_FOLDER_IN_CODE1_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_CODE1, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_FOLDER_IN_CODE2_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_CODE2, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_FOLDER_IN_COMMENT_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_FOLDER_IN_COMMENT, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DEFAULT_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DEFAULT, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    default:
				return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
		    }
	    }
		case WM_DESTROY:
	    {
		    _pageLink.destroy();
		    return TRUE;
	    }
		default:
		    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void FolderStyleDialog::setKeywords2List(int id)
{
	switch(id) {
		case IDC_FOLDER_IN_CODE1_OPEN_EDIT:
		case IDC_FOLDER_IN_CODE1_MIDDLE_EDIT:
		case IDC_FOLDER_IN_CODE1_CLOSE_EDIT:
		case IDC_FOLDER_IN_CODE2_OPEN_EDIT:
		case IDC_FOLDER_IN_CODE2_MIDDLE_EDIT:
		case IDC_FOLDER_IN_CODE2_CLOSE_EDIT:
		case IDC_FOLDER_IN_COMMENT_OPEN_EDIT:
		case IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT:
		case IDC_FOLDER_IN_COMMENT_CLOSE_EDIT:
		    ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
		    break;
	}
}

void FolderStyleDialog::updateDlg()
{
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_FOLD_COMPACT,           BM_SETCHECK, _pUserLang->_foldCompact, 0);
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_OPEN_EDIT,     WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_OPEN]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_MIDDLE_EDIT,   WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_MIDDLE]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE1_CLOSE_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE1_CLOSE]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_OPEN_EDIT,     WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_OPEN]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_MIDDLE_EDIT,   WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_MIDDLE]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_CODE2_CLOSE_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_CODE2_CLOSE]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_OPEN_EDIT,   WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_OPEN]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_MIDDLE_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_MIDDLE]));
	::SendDlgItemMessage(_hSelf, IDC_FOLDER_IN_COMMENT_CLOSE_EDIT,  WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_FOLDERS_IN_COMMENT_CLOSE]));
}

void FolderStyleDialog::retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const
{
	int j = 0;
	bool begin2Copy = false;

	for(size_t i = 0, len = lstrlen(toRetrieve); i < len; ++i) {
		if((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1])) {
			if(j > 0)
				dest[j++] = ' ';

			begin2Copy = true;
			++i;
			continue;
		}
		else if(((toRetrieve[i] == ' ') && begin2Copy == true)) {
			begin2Copy = false;
		}

		if(begin2Copy)
			dest[j++] = toRetrieve[i];
	}
	dest[j++] = '\0';
}

INT_PTR CALLBACK KeyWordsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		case WM_INITDIALOG:
	    {
		    // extend Keyword edit boxes to hold 128k of TCHARs
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD1_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD2_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD3_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD4_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD5_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD6_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD7_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);
		    ::SendMessage(::GetDlgItem(_hSelf, IDC_KEYWORD8_EDIT), EM_LIMITTEXT, WPARAM(128*1024), 0);

		    return TRUE;
	    }

		case WM_COMMAND:
	    {
		    switch(wParam)
		    {
			    case IDC_KEYWORD1_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[0]);

			    case IDC_KEYWORD2_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[1]);

			    case IDC_KEYWORD3_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[2]);

			    case IDC_KEYWORD4_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[3]);

			    case IDC_KEYWORD5_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[4]);

			    case IDC_KEYWORD6_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[5]);

			    case IDC_KEYWORD7_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[6]);

			    case IDC_KEYWORD8_PREFIX_CHECK:
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isPrefix[7]);

			    case IDC_KEYWORD1_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD1, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD2_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD2, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD3_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD3, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD4_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD4, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD5_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD5, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD6_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD6, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD7_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD7, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_KEYWORD8_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_KEYWORD8, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    default:
				return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
		    }
	    }
		default:
		    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void KeyWordsStyleDialog::setKeywords2List(int id)
{
	switch(id) {
		case IDC_KEYWORD1_EDIT:
		case IDC_KEYWORD2_EDIT:
		case IDC_KEYWORD3_EDIT:
		case IDC_KEYWORD4_EDIT:
		case IDC_KEYWORD5_EDIT:
		case IDC_KEYWORD6_EDIT:
		case IDC_KEYWORD7_EDIT:
		case IDC_KEYWORD8_EDIT:
		    ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
	}
}

void KeyWordsStyleDialog::updateDlg()
{
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS1]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS2]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS3]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS4]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD5_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS5]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD6_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS6]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD7_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS7]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD8_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_KEYWORDS8]));
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD1_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[0], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD2_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[1], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD3_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[2], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD4_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[3], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD5_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[4], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD6_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[5], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD7_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[6], 0);
	::SendDlgItemMessage(_hSelf, IDC_KEYWORD8_PREFIX_CHECK, BM_SETCHECK, _pUserLang->_isPrefix[7], 0);
}

INT_PTR CALLBACK CommentStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message) {
		case WM_ACTIVATE:
		case WM_SHOWWINDOW:
	    {
		    ::SendDlgItemMessage(_hSelf, IDC_ALLOW_ANYWHERE,    BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_NONE, 0);
		    ::SendDlgItemMessage(_hSelf, IDC_FORCE_AT_BOL,      BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_BOL,  0);
		    ::SendDlgItemMessage(_hSelf, IDC_ALLOW_WHITESPACE,  BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_WSP,  0);
		    ::SendDlgItemMessage(_hSelf, IDC_DOT_RADIO,         BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_DOT,   0);
		    ::SendDlgItemMessage(_hSelf, IDC_COMMA_RADIO,       BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_COMMA, 0);
		    ::SendDlgItemMessage(_hSelf, IDC_BOTH_RADIO,        BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_BOTH,  0);
		    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	    }
		case WM_COMMAND:
	    {
		    switch(wParam) {
			    case IDC_FOLDING_OF_COMMENTS:
			{
				return setPropertyByCheck(_hSelf, wParam, _pUserLang->_allowFoldOfComments);
			}
			    case IDC_ALLOW_ANYWHERE:
			    case IDC_FORCE_AT_BOL:
			    case IDC_ALLOW_WHITESPACE:
			{
				if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_ALLOW_ANYWHERE), BM_GETCHECK, 0, 0))
					_pUserLang->_forcePureLC = PURE_LC_NONE;
				else if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_FORCE_AT_BOL), BM_GETCHECK, 0, 0))
					_pUserLang->_forcePureLC = PURE_LC_BOL;
				else if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_ALLOW_WHITESPACE), BM_GETCHECK, 0, 0))
					_pUserLang->_forcePureLC = PURE_LC_WSP;

				if(_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					_pScintilla->styleChange();

				return TRUE;
			}
			    case IDC_DOT_RADIO:
			    case IDC_COMMA_RADIO:
			    case IDC_BOTH_RADIO:
			{
				if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_DOT_RADIO), BM_GETCHECK, 0, 0))
					_pUserLang->_decimalSeparator = DECSEP_DOT;
				else if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_COMMA_RADIO), BM_GETCHECK, 0, 0))
					_pUserLang->_decimalSeparator = DECSEP_COMMA;
				else if(BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, IDC_BOTH_RADIO), BM_GETCHECK, 0, 0))
					_pUserLang->_decimalSeparator = DECSEP_BOTH;

				if(_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					_pScintilla->styleChange();

				return TRUE;
			}
			    case IDC_COMMENTLINE_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_COMMENTLINE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_COMMENT_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_COMMENT);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_NUMBER_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_NUMBER, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    default:
				return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
		    }
	    }
		default:
		    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void CommentStyleDialog::setKeywords2List(int id)
{
	int index = 0;
	switch(id)
	{
		case IDC_NUMBER_PREFIX1_EDIT:
		case IDC_NUMBER_PREFIX2_EDIT:
		case IDC_NUMBER_EXTRAS1_EDIT:
		case IDC_NUMBER_EXTRAS2_EDIT:
		case IDC_NUMBER_SUFFIX1_EDIT:
		case IDC_NUMBER_SUFFIX2_EDIT:
		case IDC_NUMBER_RANGE_EDIT:
	    {
		    ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[globalMappper().dialogMapper[id]], max_char);
		    break;
	    }
		case IDC_COMMENT_OPEN_EDIT:
		case IDC_COMMENT_CLOSE_EDIT:
		case IDC_COMMENTLINE_OPEN_EDIT:
		case IDC_COMMENTLINE_CONTINUE_EDIT:
		case IDC_COMMENTLINE_CLOSE_EDIT:
		    index = SCE_USER_KWLIST_COMMENTS;
		    break;
		default: index = -1;
	}
	if(index != -1) {
		TCHAR newList[max_char] = TEXT("");
		TCHAR buffer[max_char] = TEXT("");
		TCHAR intBuffer[10] = {'0', 0};
		const int list[] = {
			IDC_COMMENTLINE_OPEN_EDIT,
			IDC_COMMENTLINE_CONTINUE_EDIT,
			IDC_COMMENTLINE_CLOSE_EDIT,
			IDC_COMMENT_OPEN_EDIT,
			IDC_COMMENT_CLOSE_EDIT
		};
		for(auto i = 0; i < SIZEOFARRAY(list); ++i) {
			generic_itoa(i, intBuffer+1, 10);
			::GetDlgItemText(_hSelf, list[i], buffer, max_char);
			convertTo(newList, max_char, buffer, intBuffer);
		}
		wcscpy_s(_pUserLang->_keywordLists[index], newList);
	}
}

void CommentStyleDialog::retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const
{
	int j = 0;
	bool begin2Copy = false;
	bool inGroup = false;
	for(size_t i = 0, len = lstrlen(toRetrieve); i < len; ++i) {
		if((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1])) {
			if(j > 0)
				dest[j++] = ' ';

			begin2Copy = true;
			++i;
			continue;
		}
		if(toRetrieve[i] == '(' && toRetrieve[i+1] == '(' && inGroup == false && begin2Copy == true) {
			inGroup = true;
		}
		if(toRetrieve[i] != ')' && toRetrieve[i-1] == ')' && toRetrieve[i-2] == ')' && inGroup == true) {
			inGroup = false;
		}
		if(toRetrieve[i] == ' ' && begin2Copy == true) {
			begin2Copy = false;
		}

		if(begin2Copy || inGroup)
			dest[j++] = toRetrieve[i];
	}
	dest[j++] = '\0';
}

void CommentStyleDialog::updateDlg()
{
	TCHAR buffer[max_char] = TEXT("");
	TCHAR intBuffer[10] = {'0', 0};
	const int list[] = {
		IDC_COMMENTLINE_OPEN_EDIT,
		IDC_COMMENTLINE_CONTINUE_EDIT,
		IDC_COMMENTLINE_CLOSE_EDIT,
		IDC_COMMENT_OPEN_EDIT,
		IDC_COMMENT_CLOSE_EDIT
	};
	for(int i = 0; i < SIZEOFARRAY(list); ++i) {
		generic_itoa(i, intBuffer+1, 10);
		retrieve(buffer, _pUserLang->_keywordLists[SCE_USER_KWLIST_COMMENTS], intBuffer);
		::SendDlgItemMessage(_hSelf, list[i], WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
	}
	::SendDlgItemMessage(_hSelf, IDC_FOLDING_OF_COMMENTS,   BM_SETCHECK, _pUserLang->_allowFoldOfComments,    0);
	::SendDlgItemMessage(_hSelf, IDC_ALLOW_ANYWHERE,        BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_NONE, 0);
	::SendDlgItemMessage(_hSelf, IDC_FORCE_AT_BOL,          BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_BOL,  0);
	::SendDlgItemMessage(_hSelf, IDC_ALLOW_WHITESPACE,      BM_SETCHECK, _pUserLang->_forcePureLC == PURE_LC_WSP,  0);
	::SendDlgItemMessage(_hSelf, IDC_DOT_RADIO,             BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_DOT, 0);
	::SendDlgItemMessage(_hSelf, IDC_COMMA_RADIO,           BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_COMMA,  0);
	::SendDlgItemMessage(_hSelf, IDC_BOTH_RADIO,            BM_SETCHECK, _pUserLang->_decimalSeparator == DECSEP_BOTH,  0);
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_PREFIX1_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_PREFIX1]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_PREFIX2_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_PREFIX2]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_EXTRAS1_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_EXTRAS1]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_EXTRAS2_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_EXTRAS2]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_SUFFIX1_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_SUFFIX1]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_SUFFIX2_EDIT,    WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_SUFFIX2]));
	::SendDlgItemMessage(_hSelf, IDC_NUMBER_RANGE_EDIT,      WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_NUMBER_RANGE]));
}

void SymbolsStyleDialog::updateDlg()
{
	TCHAR buffer[max_char] = TEXT("");
	const int list[] = {
		IDC_DELIMITER1_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER1_ESCAPE_EDIT,
		IDC_DELIMITER1_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER2_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER2_ESCAPE_EDIT,
		IDC_DELIMITER2_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER3_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER3_ESCAPE_EDIT,
		IDC_DELIMITER3_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER4_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER4_ESCAPE_EDIT,
		IDC_DELIMITER4_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER5_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER5_ESCAPE_EDIT,
		IDC_DELIMITER5_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER6_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER6_ESCAPE_EDIT,
		IDC_DELIMITER6_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER7_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER7_ESCAPE_EDIT,
		IDC_DELIMITER7_BOUNDARYCLOSE_EDIT,
		IDC_DELIMITER8_BOUNDARYOPEN_EDIT,
		IDC_DELIMITER8_ESCAPE_EDIT,
		IDC_DELIMITER8_BOUNDARYCLOSE_EDIT
	};
	TCHAR intBuffer[10] = {'0', 0};
	for(int i = 0; i < SIZEOFARRAY(list); ++i) {
		if(i < 10)
			generic_itoa(i, intBuffer+1, 10);
		else
			generic_itoa(i, intBuffer, 10);
		retrieve(buffer, _pUserLang->_keywordLists[SCE_USER_KWLIST_DELIMITERS], intBuffer);
		::SendDlgItemMessage(_hSelf, list[i], WM_SETTEXT, 0, reinterpret_cast<LPARAM>(buffer));
	}
	::SendDlgItemMessage(_hSelf, IDC_OPERATOR1_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS1]));
	::SendDlgItemMessage(_hSelf, IDC_OPERATOR2_EDIT, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS2]));
}

INT_PTR CALLBACK SymbolsStyleDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message) {
		case WM_COMMAND:
	    {
		    switch(wParam) {
			    case IDC_OPERATOR_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_OPERATOR, SCE_USER_MASK_NESTING_NONE);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER1_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER1);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER2_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER2);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER3_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER3);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER4_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER4);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER5_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER5);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER6_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER6);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER7_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER7);
				stylerDlg.doDialog();
				return TRUE;
			}
			    case IDC_DELIMITER8_STYLER:
			{
				StylerDlg stylerDlg(_hInst, _hSelf, SCE_USER_STYLE_DELIMITER8);
				stylerDlg.doDialog();
				return TRUE;
			}
			    default:
				return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
		    }
	    }
		default:
		    return SharedParametersDialog::run_dlgProc(Message, wParam, lParam);
	}
}

void SymbolsStyleDialog::retrieve(TCHAR * dest, const TCHAR * toRetrieve, TCHAR * prefix) const
{
	int j = 0;
	bool begin2Copy = false;
	bool inGroup = false;
	for(size_t i = 0, len = lstrlen(toRetrieve); i < len; ++i) {
		if((i == 0 || (toRetrieve[i-1] == ' ')) && (toRetrieve[i] == prefix[0] && toRetrieve[i+1] == prefix[1])) {
			if(j > 0)
				dest[j++] = ' ';

			begin2Copy = true;
			++i;
			continue;
		}
		if(toRetrieve[i] == '(' && toRetrieve[i+1] == '(' && inGroup == false && begin2Copy == true) {
			inGroup = true;
		}
		if(toRetrieve[i] != ')' && toRetrieve[i-1] == ')' && toRetrieve[i-2] == ')' && inGroup == true) {
			inGroup = false;
		}
		if(toRetrieve[i] == ' ' && begin2Copy == true) {
			begin2Copy = false;
		}
		if(begin2Copy || inGroup)
			dest[j++] = toRetrieve[i];
	}
	dest[j++] = '\0';
}

void SymbolsStyleDialog::setKeywords2List(int id)
{
	switch(id) {
		case IDC_OPERATOR1_EDIT:
		    ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS1], max_char);
		    break;
		case IDC_OPERATOR2_EDIT:
		    ::GetDlgItemText(_hSelf, id, _pUserLang->_keywordLists[SCE_USER_KWLIST_OPERATORS2], max_char);
		    break;
		case IDC_DELIMITER1_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER1_ESCAPE_EDIT:
		case IDC_DELIMITER1_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER2_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER2_ESCAPE_EDIT:
		case IDC_DELIMITER2_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER3_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER3_ESCAPE_EDIT:
		case IDC_DELIMITER3_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER4_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER4_ESCAPE_EDIT:
		case IDC_DELIMITER4_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER5_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER5_ESCAPE_EDIT:
		case IDC_DELIMITER5_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER6_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER6_ESCAPE_EDIT:
		case IDC_DELIMITER6_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER7_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER7_ESCAPE_EDIT:
		case IDC_DELIMITER7_BOUNDARYCLOSE_EDIT:
		case IDC_DELIMITER8_BOUNDARYOPEN_EDIT:
		case IDC_DELIMITER8_ESCAPE_EDIT:
		case IDC_DELIMITER8_BOUNDARYCLOSE_EDIT:
	    {
		    TCHAR newList[max_char] = TEXT("");
		    TCHAR buffer[max_char] = TEXT("");
		    TCHAR intBuffer[10] = {'0', 0};
		    const int list[] = {
			    IDC_DELIMITER1_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER1_ESCAPE_EDIT,
			    IDC_DELIMITER1_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER2_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER2_ESCAPE_EDIT,
			    IDC_DELIMITER2_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER3_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER3_ESCAPE_EDIT,
			    IDC_DELIMITER3_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER4_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER4_ESCAPE_EDIT,
			    IDC_DELIMITER4_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER5_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER5_ESCAPE_EDIT,
			    IDC_DELIMITER5_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER6_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER6_ESCAPE_EDIT,
			    IDC_DELIMITER6_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER7_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER7_ESCAPE_EDIT,
			    IDC_DELIMITER7_BOUNDARYCLOSE_EDIT,
			    IDC_DELIMITER8_BOUNDARYOPEN_EDIT,
			    IDC_DELIMITER8_ESCAPE_EDIT,
			    IDC_DELIMITER8_BOUNDARYCLOSE_EDIT
		    };
		    for(int i = 0; i < SIZEOFARRAY(list); ++i) {
			    if(i < 10)
				    generic_itoa(i, intBuffer+1, 10);
			    else
				    generic_itoa(i, intBuffer, 10);
			    int dd = list[i];
			    ::GetDlgItemText(_hSelf, dd, buffer, max_char);
			    convertTo(newList, max_char, buffer, intBuffer);
		    }
		    wcscpy_s(_pUserLang->_keywordLists[SCE_USER_KWLIST_DELIMITERS], newList);
		    break;
	    }
		default:
		    break;
	}
}

UserDefineDialog::UserDefineDialog() : SharedParametersDialog()
{
	_pCurrentUserLang = new UserLangContainer();
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DEFAULT, globalMappper().styleNameMapper[SCE_USER_STYLE_DEFAULT].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_COMMENT, globalMappper().styleNameMapper[SCE_USER_STYLE_COMMENT].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_COMMENTLINE, globalMappper().styleNameMapper[SCE_USER_STYLE_COMMENTLINE].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_NUMBER, globalMappper().styleNameMapper[SCE_USER_STYLE_NUMBER].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD1, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD1].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD2, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD2].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD3, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD3].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD4, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD4].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD5, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD5].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD6, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD6].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD7, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD7].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_KEYWORD8, globalMappper().styleNameMapper[SCE_USER_STYLE_KEYWORD8].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_OPERATOR, globalMappper().styleNameMapper[SCE_USER_STYLE_OPERATOR].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_FOLDER_IN_CODE1, globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_CODE1].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_FOLDER_IN_CODE2, globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_CODE2].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_FOLDER_IN_COMMENT, globalMappper().styleNameMapper[SCE_USER_STYLE_FOLDER_IN_COMMENT].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER1, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER1].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER2, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER2].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER3, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER3].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER4, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER4].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER5, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER5].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER6, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER6].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER7, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER7].c_str());
	_pCurrentUserLang->_styles.addStyler(SCE_USER_STYLE_DELIMITER8, globalMappper().styleNameMapper[SCE_USER_STYLE_DELIMITER8].c_str());
}

UserDefineDialog::~UserDefineDialog()
{
	delete _pCurrentUserLang;
}

void UserDefineDialog::init(HINSTANCE hInst, HWND hPere, ScintillaEditView * pSev) 
{
	if(!_pScintilla) {
		Window::init(hInst, hPere);
		_pScintilla = pSev;
	}
}

void UserDefineDialog::doDialog(bool willBeShown, bool isRTL) 
{
	if(!isCreated())
		create(IDD_GLOBAL_USERDEFINE_DLG, isRTL);
	display(willBeShown);
}

/*virtual*/void UserDefineDialog::reSizeTo(RECT & rc) // should NEVER be const !!!
{
	Window::reSizeTo(rc);
	display(false);
	display();
}

void UserDefineDialog::reloadLangCombo()
{
	NppParameters& nppParam = NppParameters::getInstance();
	::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_RESETCONTENT, 0, 0);
	::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(TEXT("User Defined Language")));
	for(int i = 0, nb = nppParam.getNbUserLang(); i < nb; ++i) {
		UserLangContainer & userLangContainer = nppParam.getULCFromIndex(i);
		::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(userLangContainer.getName()));
	}
}

void UserDefineDialog::changeStyle()
{
	_status = !_status;
	::SetDlgItemText(_hSelf, IDC_DOCK_BUTTON, (_status == DOCK) ? TEXT("Undock") : TEXT("Dock"));
	auto style = ::GetWindowLongPtr(_hSelf, GWL_STYLE);
	if(!style)
		::MessageBox(NULL, TEXT("GetWindowLongPtr failed in UserDefineDialog::changeStyle()"), TEXT(""), MB_OK);
	style = (_status == DOCK) ? ((style & ~WS_POPUP) & ~DS_MODALFRAME & ~WS_CAPTION) | WS_CHILD : (style & ~WS_CHILD) | WS_POPUP | DS_MODALFRAME | WS_CAPTION;
	auto result = ::SetWindowLongPtr(_hSelf, GWL_STYLE, style);
	if(!result)
		::MessageBox(NULL, TEXT("SetWindowLongPtr failed in UserDefineDialog::changeStyle()"), TEXT(""), MB_OK);
	if(_status == DOCK)
		getActualPosSize();
	else
		restorePosSize();
	::SetParent(_hSelf, (_status == DOCK) ? _hParent : NULL);
}

void UserDefineDialog::getActualPosSize() 
{
	::GetWindowRect(_hSelf, &_dlgPos);
	_dlgPos.right -= _dlgPos.left;
	_dlgPos.bottom -= _dlgPos.top;
}

void UserDefineDialog::enableLangAndControlsBy(size_t index)
{
	_pUserLang = (index == 0) ? _pCurrentUserLang : &((NppParameters::getInstance()).getULCFromIndex(index - 1));
	if(index != 0)
		::SetWindowText(::GetDlgItem(_hSelf, IDC_EXT_EDIT), _pUserLang->_ext.c_str());

	::ShowWindow(::GetDlgItem(_hSelf, IDC_EXT_STATIC), (index == 0) ? SW_HIDE : SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EXT_EDIT), (index == 0) ? SW_HIDE : SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RENAME_BUTTON), (index == 0) ? SW_HIDE : SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_REMOVELANG_BUTTON), (index == 0) ? SW_HIDE : SW_SHOW);
}

void UserDefineDialog::updateDlg()
{
	int i = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0));
	if(i > 0) { // the first menu item is generic UDL
		NppParameters& nppParam = NppParameters::getInstance();
		nppParam.setUdlXmlDirtyFromIndex(i - 1);
	}
	::SendDlgItemMessage(_hSelf, IDC_LANGNAME_IGNORECASE_CHECK, BM_SETCHECK, _pUserLang->_isCaseIgnored, 0);
	_folderStyleDlg.updateDlg();
	_keyWordsStyleDlg.updateDlg();
	_commentStyleDlg.updateDlg();
	_symbolsStyleDlg.updateDlg();
}

INT_PTR CALLBACK UserDefineDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	NppParameters& nppParam = NppParameters::getInstance();
	NativeLangSpeaker * pNativeSpeaker = nppParam.getNativeLangSpeaker();
	switch(message) {
		case WM_INITDIALOG:
	    {
		    _pUserLang = _pCurrentUserLang;
		    _ctrlTab.init(_hInst, _hSelf, false);
		    NppDarkMode::subclassTabControl(_ctrlTab.getHSelf());
		    int tabDpiDynamicalHeight = nppParam._dpiManager.scaleY(13);
		    _ctrlTab.setFont(TEXT("Tahoma"), tabDpiDynamicalHeight);

		    _folderStyleDlg.init(_hInst, _hSelf);
		    _folderStyleDlg.create(IDD_FOLDER_STYLE_DLG);
		    _folderStyleDlg.display();

		    _keyWordsStyleDlg.init(_hInst, _hSelf);
		    _keyWordsStyleDlg.create(IDD_KEYWORD_STYLE_DLG);
		    _keyWordsStyleDlg.display(false);

		    _commentStyleDlg.init(_hInst, _hSelf);
		    _commentStyleDlg.create(IDD_COMMENT_STYLE_DLG);
		    _commentStyleDlg.display(false);

		    _symbolsStyleDlg.init(_hInst, _hSelf);
		    _symbolsStyleDlg.create(IDD_SYMBOL_STYLE_DLG);
		    _symbolsStyleDlg.display(false);

		    _wVector.push_back(DlgInfo(&_folderStyleDlg,    TEXT("Folder && Default")));
		    _wVector.push_back(DlgInfo(&_keyWordsStyleDlg,  TEXT("Keywords Lists")));
		    _wVector.push_back(DlgInfo(&_commentStyleDlg,   TEXT("Comment && Number")));
		    _wVector.push_back(DlgInfo(&_symbolsStyleDlg,   TEXT("Operators && Delimiters")));

		    _ctrlTab.createTabs(_wVector);
		    _ctrlTab.display();

		    RECT arc;
		    ::GetWindowRect(::GetDlgItem(_hSelf, IDC_IMPORT_BUTTON), &arc);

		    POINT p;
		    p.x = arc.left;
		    p.y = arc.bottom;
		    ::ScreenToClient(_hSelf, &p);

		    RECT rc;
		    getClientRect(rc);
		    rc.top = p.y + 10;
		    rc.bottom -= 20;
		    _ctrlTab.reSizeTo(rc);

		    _folderStyleDlg.reSizeTo(rc);
		    _keyWordsStyleDlg.reSizeTo(rc);
		    _commentStyleDlg.reSizeTo(rc);
		    _symbolsStyleDlg.reSizeTo(rc);

		    reloadLangCombo();
		    ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, 0, 0);

		    enableLangAndControlsBy(0);

		    if(nppParam.isTransparentAvailable()) {
			    ::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_SHOW);
			    ::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_SHOW);

			    ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_SETRANGE, FALSE, MAKELONG(20, 200));
			    ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_SETPOS, TRUE, 150);
			    if(!(BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, BM_GETCHECK, 0, 0)))
				    ::EnableWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), FALSE);
		    }
		    SCROLLINFO si;
		    INITWINAPISTRUCT(si);
		    si.fMask  = SIF_RANGE;//| SIF_PAGE;
		    si.nMin   = 0;
		    si.nMax   = 0;
		    ::SetScrollInfo(_hSelf, SB_VERT, &si, TRUE);

		    TCHAR temp[32];
		    generic_string udlVersion = TEXT("User Defined Language v.");
		    udlVersion += generic_itoa(SCE_UDL_VERSION_MAJOR, temp, 10);
		    udlVersion += TEXT(".");
		    udlVersion += generic_itoa(SCE_UDL_VERSION_MINOR, temp, 10);
		    udlVersion += TEXT(".");
		    udlVersion += generic_itoa(SCE_UDL_VERSION_BUILD, temp, 10);
		    udlVersion += TEXT(".");
		    udlVersion += generic_itoa(SCE_UDL_VERSION_REVISION, temp, 10);

		    ::SetWindowText(_hSelf, udlVersion.c_str());

		    NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
		    NppDarkMode::setDarkScrollBar(_hSelf);

		    return TRUE;
	    }

		case WM_CTLCOLOREDIT:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORLISTBOX:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_PRINTCLIENT:
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::autoThemeChildControls(_hSelf);
		    NppDarkMode::setDarkScrollBar(_hSelf);
		    return TRUE;
		case WM_NOTIFY:
	    {
		    NMHDR * nmhdr = (NMHDR*)lParam;
		    if(nmhdr->code == TCN_SELCHANGE) {
			    if(nmhdr->hwndFrom == _ctrlTab.getHSelf()) {
				    _ctrlTab.clickedUpdate();
				    return TRUE;
			    }
		    }
		    break;
	    }

		case WM_HSCROLL:
	    {
		    if(reinterpret_cast<HWND>(lParam) == ::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER)) {
			    int percent = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
			    nppParam.SetTransparent(_hSelf, percent);
		    }
		    return TRUE;
	    }

		case WM_COMMAND:
	    {
		    if(HIWORD(wParam) == EN_CHANGE) {
			    TCHAR ext[extsLenMax] = { '\0' };
			    ::SendDlgItemMessage(_hSelf, IDC_EXT_EDIT, WM_GETTEXT, extsLenMax, reinterpret_cast<LPARAM>(ext));
			    _pUserLang->_ext = ext;
			    return TRUE;
		    }
		    else if(HIWORD(wParam) == CBN_SELCHANGE) {
			    if(LOWORD(wParam) == IDC_LANGNAME_COMBO) {
				    auto i = ::SendDlgItemMessage(_hSelf, LOWORD(wParam), CB_GETCURSEL, 0, 0);
				    enableLangAndControlsBy(i);
				    updateDlg();
			    }
			    return TRUE;
		    }
		    else {
			    switch(wParam)
			    {
				    case IDC_DOCK_BUTTON:
				{
					int msg = WM_UNDOCK_USERDEFINE_DLG;

					if(_status == UNDOCK) {
						if(nppParam.isTransparentAvailable()) {
							nppParam.removeTransparent(_hSelf);
							::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_HIDE);
							::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_HIDE);
						}
						msg = WM_DOCK_USERDEFINE_DLG;
					}
					changeStyle();
					if(_status == UNDOCK) {
						if(nppParam.isTransparentAvailable()) {
							bool isChecked = (BST_CHECKED == ::SendDlgItemMessage(_hSelf, IDC_UD_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
							if(isChecked) {
								int percent = static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER, TBM_GETPOS, 0, 0));
								nppParam.SetTransparent(_hSelf, percent);
							}
							::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_TRANSPARENT_CHECK), SW_SHOW);
							::ShowWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), SW_SHOW);
						}
					}
					::SendMessage(_hParent, msg, 0, 0);
					return TRUE;
				}

				    case IDCANCEL:
				{
					::SendMessage(_hParent, WM_CLOSE_USERDEFINE_DLG, 0, 0);
					display(false);
					return TRUE;
				}

				    case IDC_REMOVELANG_BUTTON:
				{
					int result = nppParam.getNativeLangSpeaker()->messageBox("UDLRemoveCurrentLang", _hSelf,
						TEXT("Are you sure?"), TEXT("Remove the current language"), MB_YESNO);
					if(result == IDYES) {
						auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
						const size_t langNameLen = 256;
						TCHAR langName[langNameLen + 1];
						auto cbTextLen = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXTLEN, i, 0);
						if(cbTextLen > langNameLen)
							return TRUE;

						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXT, i,
						    reinterpret_cast<LPARAM>(langName));

						//remove current language from combobox
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_DELETESTRING, i, 0);
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i-1, 0);
						::SendMessage(_hSelf,
						    WM_COMMAND,
						    MAKELONG(IDC_LANGNAME_COMBO, CBN_SELCHANGE),
						    reinterpret_cast<LPARAM>(::GetDlgItem(_hSelf, IDC_LANGNAME_COMBO)));

						//remove current language from userLangArray
						nppParam.removeUserLang(i-1);

						//remove current language from langMenu
						HWND hNpp = ::GetParent(_hSelf);
						HMENU m = reinterpret_cast<HMENU>(::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0));
						HMENU subMenu = ::GetSubMenu(m, MENUINDEX_LANGUAGE);
						::RemoveMenu(subMenu, static_cast<UINT>(IDM_LANG_USER + i), MF_BYCOMMAND);
						::DrawMenuBar(hNpp);
						::SendMessage(_hParent, WM_REMOVE_USERLANG, 0, reinterpret_cast<LPARAM>(langName));
					}
					return TRUE;
				}
				    case IDC_RENAME_BUTTON:
				{
					auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
					const size_t langNameLen = 256;
					TCHAR langName[langNameLen + 1];
					auto cbTextLen = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXTLEN, i, 0);
					if(cbTextLen > langNameLen)
						return TRUE;
					::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(langName));
					generic_string strName = pNativeSpeaker->getLocalizedStrFromID("common-name", TEXT("Name: "));
					generic_string strTitle = pNativeSpeaker->getLocalizedStrFromID("userdefined-title-rename", TEXT("Rename Current Language Name"));
					StringDlg strDlg;
					strDlg.init(_hInst, _hSelf, strTitle.c_str(), strName.c_str(), langName, langNameLenMax - 1);
					TCHAR * newName = (TCHAR*)strDlg.doDialog();
					if(newName) {
						if(nppParam.isExistingUserLangName(newName)) {
							nppParam.getNativeLangSpeaker()->messageBox("UDLNewNameError", _hSelf,
							    TEXT("This name is used by another language,\rplease give another one."), TEXT("UDL Error"), MB_OK);
							::PostMessage(_hSelf, WM_COMMAND, IDC_RENAME_BUTTON, 0);
							return TRUE;
						}
						//rename current language name in combobox
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_DELETESTRING, i, 0);
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_INSERTSTRING, i, reinterpret_cast<LPARAM>(newName));
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i, 0);
						//rename current language name in userLangArray
						UserLangContainer & userLangContainer = nppParam.getULCFromIndex(i-1);
						userLangContainer._name = newName;
						//rename current language name in langMenu
						HWND hNpp = ::GetParent(_hSelf);
						HMENU hM = reinterpret_cast<HMENU>(::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0));
						HMENU hSubM = ::GetSubMenu(hM, MENUINDEX_LANGUAGE);
						::ModifyMenu(hSubM,
						    static_cast<UINT>(IDM_LANG_USER + i),
						    MF_BYCOMMAND,
						    IDM_LANG_USER + i,
						    newName);
						::DrawMenuBar(hNpp);
						::SendMessage(_hParent,
						    WM_RENAME_USERLANG,
						    reinterpret_cast<WPARAM>(newName),
						    reinterpret_cast<LPARAM>(langName));
					}

					return TRUE;
				}

				    case IDC_ADDNEW_BUTTON:
				    case IDC_SAVEAS_BUTTON:
				{
					auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
					if(i == 0)
						wParam = IDC_ADDNEW_BUTTON;

					generic_string strName = pNativeSpeaker->getLocalizedStrFromID("common-name", TEXT("Name: "));
					generic_string strTitle = (wParam == IDC_SAVEAS_BUTTON) ?
					    pNativeSpeaker->getLocalizedStrFromID("userdefined-title-save",
						TEXT("Save Current Language Name As...")) :
					    pNativeSpeaker->getLocalizedStrFromID("userdefined-title-new", TEXT("Create New Language..."));

					StringDlg strDlg;
					strDlg.init(_hInst, _hSelf, strTitle.c_str(), strName.c_str(), TEXT(""), langNameLenMax - 1);

					TCHAR * tmpName = reinterpret_cast<TCHAR *>(strDlg.doDialog());

					if(tmpName && tmpName[0]) {
						generic_string newNameString(tmpName);
						const TCHAR * newName = newNameString.c_str();

						if(nppParam.isExistingUserLangName(newName)) {
							pNativeSpeaker->messageBox("UDLNewNameError",
							    _hSelf,
							    TEXT("This name is used by another language,\rplease give another one."),
							    TEXT("UDL Error"),
							    MB_OK);
							::PostMessage(_hSelf, WM_COMMAND, IDC_RENAME_BUTTON, 0);
							return TRUE;
						}
						//add current language in userLangArray at the end as a new lang
						UserLangContainer & userLang =
						    (wParam ==
						    IDC_SAVEAS_BUTTON) ? nppParam.getULCFromIndex(i-1) : *_pCurrentUserLang;
						int newIndex = nppParam.addUserLangToEnd(userLang, newName);

						//add new language name in combobox
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_ADDSTRING, 0, LPARAM(newName));
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, newIndex + 1, 0);
						::SendMessage(_hSelf,
						    WM_COMMAND,
						    MAKELONG(IDC_LANGNAME_COMBO, CBN_SELCHANGE),
						    reinterpret_cast<LPARAM>(::GetDlgItem(_hSelf, IDC_LANGNAME_COMBO)));

						//add new language name in langMenu
						HWND hNpp = ::GetParent(_hSelf);
						HMENU m = reinterpret_cast<HMENU>(::SendMessage(hNpp, NPPM_INTERNAL_GETMENU, 0, 0));
						::InsertMenu(::GetSubMenu(m, MENUINDEX_LANGUAGE),
						    IDM_LANG_USER + newIndex,
						    MF_BYCOMMAND,
						    IDM_LANG_USER + newIndex + 1,
						    newName);
						::DrawMenuBar(hNpp);
					}

					return TRUE;
				}
				    case IDC_IMPORT_BUTTON:
				{
					CustomFileDialog fDlg(_hSelf);
					fDlg.setExtFilter(TEXT("UDL"), TEXT(".xml"));
					generic_string sourceFile = fDlg.doOpenSingleFileDlg();
					if(sourceFile.empty()) break;

					bool isSuccessful = nppParam.importUDLFromFile(sourceFile);
					if(isSuccessful) {
						auto i = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
						reloadLangCombo();
						::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_SETCURSEL, i, 0);
						printStr(TEXT("Import successful."));
					}
					else {
						printStr(TEXT("Fail to import."));
					}
					break;
				}

				    case IDC_EXPORT_BUTTON:
				{
					auto i2Export = ::SendDlgItemMessage(_hSelf, IDC_LANGNAME_COMBO, CB_GETCURSEL, 0, 0);
					if(i2Export == 0) {
						// maybe a better option would be to simply send IDC_SAVEAS_BUTTON
						// message, and display "Save As..." dialog?
						printStr(TEXT(
							    "Before exporting, save your language definition by clicking \"Save As...\" button"));
						break;
					}

					CustomFileDialog fDlg(_hSelf);
					fDlg.setExtFilter(TEXT("UDL"), TEXT(".xml"));
					fDlg.setExtIndex(0);                    // 0 Default index else file will be
						                                // saved without extension
					generic_string fileName2save = fDlg.doSaveDlg();
					if(fileName2save.empty()) break;

					if(i2Export > 0) {
						bool isSuccessful = nppParam.exportUDLToFile(i2Export - 1, fileName2save);
						if(isSuccessful) {
							printStr(TEXT("Export successful"));
						}
						else {
							printStr(TEXT("Fail to export."));
						}
					}
					break;
				}

				    case IDC_UD_TRANSPARENT_CHECK:
				{
					bool isChecked =
					    (BST_CHECKED ==
					    ::SendDlgItemMessage(_hSelf, IDC_UD_TRANSPARENT_CHECK, BM_GETCHECK, 0, 0));
					if(isChecked) {
						int percent =
						    static_cast<int32_t>(::SendDlgItemMessage(_hSelf, IDC_UD_PERCENTAGE_SLIDER,
						    TBM_GETPOS, 0, 0));
						nppParam.SetTransparent(_hSelf, percent);
					}
					else
						nppParam.removeTransparent(_hSelf);

					::EnableWindow(::GetDlgItem(_hSelf, IDC_UD_PERCENTAGE_SLIDER), isChecked);
					return TRUE;
				}

				    case IDC_LANGNAME_IGNORECASE_CHECK:
					return setPropertyByCheck(_hSelf, wParam, _pUserLang->_isCaseIgnored);

				    default:
					break;
			    }
		    }
		    return FALSE;
	    }

		case WM_DESTROY:
	    {
		    _folderStyleDlg.destroy();
		    _keyWordsStyleDlg.destroy();
		    _commentStyleDlg.destroy();
		    _symbolsStyleDlg.destroy();
		    _ctrlTab.destroy();
		    return TRUE;
	    }
		case WM_SIZE:
	    {
		    int originalHight = _dlgPos.bottom; //- ((_status == DOCK)?_dlgPos.top:0);
		    _currentHight = HIWORD(lParam);
		    int diff = _currentHight - _prevHightVal;
		    _prevHightVal = _currentHight;
		    int maxPos = originalHight - _currentHight;
		    // Set the vertical scrolling range and page size
		    SCROLLINFO si;
		    INITWINAPISTRUCT(si);
		    si.fMask  = SIF_RANGE | SIF_PAGE;
		    si.nMin   = 0;
		    si.nMax   = (_status == UNDOCK) ? 0 : originalHight;
		    si.nPage  = _currentHight;
		    //si.nPos = 0;
		    ::SetScrollInfo(_hSelf, SB_VERT, &si, TRUE);
		    if((_yScrollPos >= maxPos) && (_currentHight < originalHight)) {
			    //int nDelta = MIN(MAX(maxPos/10,5), maxPos - _yScrollPos);
			    if(_yScrollPos > 0) {
				    _yScrollPos -= diff;
				    ::SetScrollPos(_hSelf, SB_VERT, _yScrollPos, TRUE);
				    ::ScrollWindow(_hSelf, 0, diff, NULL, NULL);
			    }
		    }
		    return TRUE;
	    }
		case WM_VSCROLL:
	    {
		    int originalHight = _dlgPos.bottom;
		    int oldy = _yScrollPos;
		    int maxPos = originalHight - _currentHight;
		    switch(LOWORD(wParam)) {
			    // user clicked the top arrow
			    case SB_LINEUP:
				if(_yScrollPos <= 0)
					return FALSE;
				_yScrollPos = 0;
				break;

			    // user clicked the bottom arrow
			    case SB_LINEDOWN:
				if(_yScrollPos >= maxPos)
					return FALSE;
				_yScrollPos = maxPos;
				break;

			    case SB_PAGEDOWN:
				if(_yScrollPos >= maxPos)
					return FALSE;
				_yScrollPos = maxPos;
				break;

			    case SB_PAGEUP:
				if(_yScrollPos <= 0)
					return FALSE;
				_yScrollPos = 0;
				break;

			    case SB_THUMBTRACK:
			    case SB_THUMBPOSITION:
				_yScrollPos = HIWORD(wParam);
				break;

			    default:
				return FALSE;
		    }
		    ::SetScrollPos(_hSelf, SB_VERT, _yScrollPos, TRUE);
		    ::ScrollWindow(_hSelf, 0, oldy-_yScrollPos, NULL, NULL);
	    }
		case NPPM_MODELESSDIALOG:
		    return ::SendMessage(_hParent, NPPM_MODELESSDIALOG, wParam, lParam);
	}
	return FALSE;
}

void StringDlg::init(HINSTANCE hInst, HWND parent, const TCHAR * title, const TCHAR * staticName, const TCHAR * text2Set,
	int txtLen, const TCHAR* restrictedChars, bool bGotoCenter) 
{
	Window::init(hInst, parent);
	_title = title;
	_static = staticName;
	_textValue = text2Set;
	_txtLen = txtLen;
	_shouldGotoCenter = bGotoCenter;
	if(restrictedChars && _tcslen(restrictedChars))
		_restrictedChars = restrictedChars;
}

INT_PTR StringDlg::doDialog() 
{
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_STRING_DLG), _hParent,  dlgProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR CALLBACK StringDlg::run_dlgProc(UINT Message, WPARAM wParam, LPARAM)
{
	switch(Message) {
		case WM_INITDIALOG:
	    {
		    NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);
		    // Re-route to Subclassed the edit control's proc if needed
		    if(_restrictedChars.length()) {
			    ::SetWindowLongPtr(GetDlgItem(_hSelf, IDC_STRING_EDIT), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
			    _oldEditProc =
				reinterpret_cast<WNDPROC>(SetWindowLongPtr(GetDlgItem(_hSelf, IDC_STRING_EDIT), GWLP_WNDPROC,
				reinterpret_cast<LONG_PTR>(customEditProc)));
		    }
		    ::SetWindowText(_hSelf, _title.c_str());
		    ::SetDlgItemText(_hSelf, IDC_STRING_STATIC, _static.c_str());
		    ::SetDlgItemText(_hSelf, IDC_STRING_EDIT, _textValue.c_str());
		    if(_txtLen)
			    ::SendDlgItemMessage(_hSelf, IDC_STRING_EDIT, EM_SETLIMITTEXT, _txtLen, 0);
		    // localization for OK and Cancel
		    NativeLangSpeaker * pNativeSpeaker = (NppParameters::getInstance()).getNativeLangSpeaker();
		    if(pNativeSpeaker) {
			    generic_string ok = pNativeSpeaker->getLocalizedStrFromID("common-ok", TEXT("OK"));
			    generic_string cancel = pNativeSpeaker->getLocalizedStrFromID("common-cancel", TEXT("Cancel"));
			    ::SetDlgItemText(_hSelf, IDOK, ok.c_str());
			    ::SetDlgItemText(_hSelf, IDCANCEL, cancel.c_str());
		    }
		    if(_shouldGotoCenter)
			    goToCenter();
		    return TRUE;
	    }
		case WM_CTLCOLOREDIT:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_PRINTCLIENT:
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
		case WM_ERASEBKGND:
		    if(NppDarkMode::isEnabled()) {
			    RECT rc = { 0 };
			    getClientRect(rc);
			    ::FillRect(reinterpret_cast<HDC>(wParam), &rc, NppDarkMode::getDarkerBackgroundBrush());
			    return TRUE;
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::autoThemeChildControls(_hSelf);
		    return TRUE;
		case WM_COMMAND:
	    {
		    switch(wParam) {
			    case IDOK:
			{
				TCHAR tmpName[langNameLenMax];
				tmpName[0] = '\0';
				::GetDlgItemText(_hSelf, IDC_STRING_EDIT, tmpName, langNameLenMax);
				_textValue = tmpName;
				::EndDialog(_hSelf, reinterpret_cast<INT_PTR>(_textValue.c_str()));
				return TRUE;
			}

			    case IDCANCEL:
				::EndDialog(_hSelf, 0);
				return TRUE;

			    default:
				return FALSE;
		    }
	    }
		default:
		    return FALSE;
	}
	return FALSE;
}

LRESULT StringDlg::customEditProc(HWND hEdit, UINT msg, WPARAM wParam, LPARAM lParam)
{
	StringDlg * pSelf = reinterpret_cast<StringDlg *>(::GetWindowLongPtr(hEdit, GWLP_USERDATA));
	if(!pSelf) {
		return 0;
	}

	switch(msg)
	{
		case WM_CHAR:
		    if(0x80 & GetKeyState(VK_CONTROL)) {
			    switch(wParam)
			    {
				    case 0x16: // ctrl - V
					pSelf->HandlePaste(hEdit);
					return 0;

				    case 0x03: // ctrl - C
				    case 0x18: // ctrl - X
				    default:
					// Let them go to default
					break;
			    }
		    }
		    else {
			    // If Key pressed not permitted, then return 0
			    if(!pSelf->isAllowed(reinterpret_cast<TCHAR*>(&wParam)))
				    return 0;
		    }
		    break;

		case WM_DESTROY:
		    // Reset the message handler to the original one
		    SetWindowLongPtr(hEdit, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pSelf->_oldEditProc));
		    return 0;
	}

	// Process the message using the default handler
	return CallWindowProc(pSelf->_oldEditProc, hEdit, msg, wParam, lParam);
}

bool StringDlg::isAllowed(const generic_string & txt)
{
	for(auto ch : txt) {
	#ifndef __MINGW32__
		if(std::find(_restrictedChars.cbegin(), _restrictedChars.cend(), ch) != _restrictedChars.cend())
			return false;
	#endif
	}
	return true;
}

void StringDlg::HandlePaste(HWND hEdit)
{
	if(OpenClipboard(hEdit)) {
		HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
		if(NULL != hClipboardData) {
			LPTSTR pszText = reinterpret_cast<LPTSTR>(GlobalLock(hClipboardData));
			if(NULL != pszText && isAllowed(pszText)) {
				SendMessage(hEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(pszText));
			}

			GlobalUnlock(hClipboardData);
		}
		CloseClipboard();
	}
}

void StylerDlg::move2CtrlRight(HWND hwndDlg, int ctrlID, HWND handle2Move, int handle2MoveWidth, int handle2MoveHeight)
{
	POINT p;
	RECT rc;
	::GetWindowRect(::GetDlgItem(hwndDlg, ctrlID), &rc);
	p.x = rc.right + NppParameters::getInstance()._dpiManager.scaleX(5);
	p.y = rc.top + ((rc.bottom - rc.top) / 2) - handle2MoveHeight / 2;
	::ScreenToClient(hwndDlg, &p);
	::MoveWindow(handle2Move, p.x, p.y, handle2MoveWidth, handle2MoveHeight, TRUE);
}

StylerDlg::StylerDlg(HINSTANCE hInst, HWND parent, int stylerIndex, int enabledNesters) :
	_hInst(hInst), _parent(parent), _stylerIndex(stylerIndex), _enabledNesters(enabledNesters) 
{
	_pFgColour = new ColourPicker;
	_pBgColour = new ColourPicker;
	_initialStyle = SharedParametersDialog::_pUserLang->_styles.getStyler(stylerIndex);
}

StylerDlg::~StylerDlg() 
{
	_pFgColour->destroy();
	_pBgColour->destroy();
	delete _pFgColour;
	delete _pBgColour;
}

long StylerDlg::doDialog() 
{
	return long(::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_STYLER_POPUP_DLG), _parent, dlgProc, reinterpret_cast<LPARAM>(this)));
}

INT_PTR CALLBACK StylerDlg::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	StylerDlg * dlg = (StylerDlg*)::GetProp(hwnd, TEXT("Styler dialog prop"));
	NppParameters& nppParam = NppParameters::getInstance();
	switch(message) {
		case WM_INITDIALOG:
	    {
		    NppDarkMode::setDarkTitleBar(hwnd);
		    NppDarkMode::autoSubclassAndThemeChildControls(hwnd);

		    NativeLangSpeaker * pNativeLangSpeaker = nppParam.getNativeLangSpeaker();
		    pNativeLangSpeaker->changeUserDefineLangPopupDlg(hwnd);

		    ::SetProp(hwnd, TEXT("Styler dialog prop"), (HANDLE)lParam);
		    dlg = (StylerDlg*)::GetProp(hwnd, TEXT("Styler dialog prop"));
		    NppStyle & style = SharedParametersDialog::_pUserLang->_styles.getStyler(dlg->_stylerIndex);

		    // move dialog over UDL GUI (position 0,0 of UDL window) so it wouldn't cover the code
		    RECT wrc;
		    ::GetWindowRect(dlg->_parent, &wrc);
		    wrc.left = wrc.left < 0 ? 200 : wrc.left; // if outside of visible area
		    wrc.top = wrc.top < 0 ? 200 : wrc.top;
		    ::SetWindowPos(hwnd, HWND_TOP, wrc.left, wrc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		    ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_UNDERLINE, BM_SETCHECK, style._fontStyle & FONTSTYLE_UNDERLINE, 0);
		    ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_ITALIC,    BM_SETCHECK, style._fontStyle & FONTSTYLE_ITALIC, 0);
		    ::SendDlgItemMessage(hwnd, IDC_STYLER_CHECK_BOLD,      BM_SETCHECK, style._fontStyle & FONTSTYLE_BOLD, 0);

		    // for the font size combo
		    HWND hFontSizeCombo = ::GetDlgItem(hwnd, IDC_STYLER_COMBO_FONT_SIZE);
		    for(int j = 0; j < int(sizeof(fontSizeStrs))/(3*sizeof(TCHAR)); ++j)
			    ::SendMessage(hFontSizeCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontSizeStrs[j]));

		    TCHAR size[10];
		    if(style._fontSize == -1)
			    size[0] = '\0';
		    else
			    wsprintf(size, TEXT("%d"), style._fontSize);
		    auto i = ::SendMessage(hFontSizeCombo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(size));
		    if(i != CB_ERR)
			    ::SendMessage(hFontSizeCombo, CB_SETCURSEL, i, 0);

		    // for the font name combo
		    HWND hFontNameCombo = ::GetDlgItem(hwnd, IDC_STYLER_COMBO_FONT_NAME);
		    const std::vector <generic_string> & fontlist = nppParam.getFontList();
		    for(size_t j = 0, len = fontlist.size(); j < len; ++j) {
			    auto k = ::SendMessage(hFontNameCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fontlist[j].c_str()));
			    ::SendMessage(hFontNameCombo, CB_SETITEMDATA, k, reinterpret_cast<LPARAM>(fontlist[j].c_str()));
		    }
		    i = ::SendMessage(hFontNameCombo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(style._fontName.c_str()));
		    if(i == CB_ERR)
			    i = 0;
		    ::SendMessage(hFontNameCombo, CB_SETCURSEL, i, 0);
		    if(style._fgColor == COLORREF(-1))
			    style._fgColor = GetColorRef(SClrBlack); // black
		    if(style._bgColor == COLORREF(-1))
			    style._bgColor = GetColorRef(SClrWhite); // white
		    dlg->_pFgColour->init(dlg->_hInst, hwnd);
		    dlg->_pFgColour->setColour(style._fgColor);
		    dlg->_pBgColour->init(dlg->_hInst, hwnd);
		    dlg->_pBgColour->setColour(style._bgColor);

		    const int w = nppParam._dpiManager.scaleX(25);
		    const int h = nppParam._dpiManager.scaleY(25);
		    dlg->move2CtrlRight(hwnd, IDC_STYLER_FG_STATIC, dlg->_pFgColour->getHSelf(), w, h);
		    dlg->move2CtrlRight(hwnd, IDC_STYLER_BG_STATIC, dlg->_pBgColour->getHSelf(), w, h);
		    dlg->_pFgColour->display();
		    dlg->_pBgColour->display();
		    unordered_map<int, int>::iterator iter = globalMappper().nestingMapper.begin();
		    for(; iter != globalMappper().nestingMapper.end(); ++iter) {
			    ::SendDlgItemMessage(hwnd, iter->first, BM_SETCHECK, style._nesting & iter->second, 0);
			    ::EnableWindow(::GetDlgItem(hwnd, iter->first), dlg->_enabledNesters & iter->second);
		    }
		    return TRUE;
	    }

		case WM_CTLCOLOREDIT:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORLISTBOX:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColor(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		    if(NppDarkMode::isEnabled()) {
			    return NppDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		    }
		    break;
		case WM_PRINTCLIENT:
		    if(NppDarkMode::isEnabled()) {
			    return TRUE;
		    }
		    break;
		case NPPM_INTERNAL_REFRESHDARKMODE:
		    NppDarkMode::setDarkTitleBar(hwnd);
		    NppDarkMode::autoThemeChildControls(hwnd);
		    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		    return TRUE;
		case WM_COMMAND:
	    {
		    if(dlg == nullptr)
			    return FALSE;
		    NppStyle & style = SharedParametersDialog::_pUserLang->_styles.getStyler(dlg->_stylerIndex);
		    if(HIWORD(wParam) == CBN_SELCHANGE) {
			    auto i = ::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETCURSEL, 0, 0);
			    if(LOWORD(wParam) == IDC_STYLER_COMBO_FONT_SIZE) {
				    if(i != 0) {
					    const size_t intStrLen = 3;
					    TCHAR intStr[intStrLen];
					    auto lbTextLen = ::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETLBTEXTLEN, i, 0);
					    if(lbTextLen > intStrLen - 1)
						    return TRUE;
					    ::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(intStr));
					    if((!intStr) || (!intStr[0]))
						    style._fontSize = -1;
					    else {
						    TCHAR * finStr;
						    style._fontSize = generic_strtol(intStr, &finStr, 10);
						    if(*finStr != '\0')
							    style._fontSize = -1;
					    }
				    }
				    else {
					    style._fontSize = -1;
				    }
			    }
			    else if(LOWORD(wParam) == IDC_STYLER_COMBO_FONT_NAME) {
				    style._fontName = (TCHAR*)::SendDlgItemMessage(hwnd, LOWORD(wParam), CB_GETITEMDATA, i, 0);
			    }
			    // show changes to user, re-color document
			    if(SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
				    SharedParametersDialog::_pScintilla->styleChange();
			    return TRUE;
		    }
		    else if(HIWORD(wParam) == CPN_COLOURPICKED) { // #define CPN_COLOURPICKED (BN_CLICKED)
			    if(wParam == IDCANCEL) {
				    style = dlg->_initialStyle;
				    // show changes to user, re-color document
				    if(SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
					    SharedParametersDialog::_pScintilla->styleChange();
				    ::RemoveProp(hwnd, TEXT("Styler dialog prop"));
				    ::EndDialog(hwnd, IDCANCEL);
				    return TRUE;
			    }
			    if(wParam == IDOK) {
				    ::RemoveProp(hwnd, TEXT("Styler dialog prop"));
				    ::EndDialog(hwnd, IDOK);
				    return TRUE;
			    }
			    style._fgColor = dlg->_pFgColour->getColour();
			    style._bgColor = dlg->_pBgColour->getColour();
				SETFLAG(style._colorStyle, COLORSTYLE_FOREGROUND, dlg->_pFgColour->isEnabled());
				SETFLAG(style._colorStyle, COLORSTYLE_BACKGROUND, dlg->_pBgColour->isEnabled());
			    style._fontStyle = FONTSTYLE_NONE;
			    if(BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_BOLD), BM_GETCHECK, 0, 0))
				    style._fontStyle |= FONTSTYLE_BOLD;
			    if(BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_ITALIC), BM_GETCHECK, 0, 0))
				    style._fontStyle |= FONTSTYLE_ITALIC;
			    if(BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, IDC_STYLER_CHECK_UNDERLINE), BM_GETCHECK, 0, 0))
				    style._fontStyle |= FONTSTYLE_UNDERLINE;
			    style._nesting = SCE_USER_MASK_NESTING_NONE;
			    unordered_map<int, int>::iterator iter = globalMappper().nestingMapper.begin();
			    for(; iter != globalMappper().nestingMapper.end(); ++iter) {
				    if(BST_CHECKED == ::SendMessage(::GetDlgItem(hwnd, iter->first), BM_GETCHECK, 0, 0))
					    style._nesting |= iter->second;
			    }
			    // show changes to user, re-color document
			    if(SharedParametersDialog::_pScintilla->getCurrentBuffer()->getLangType() == L_USER)
				    SharedParametersDialog::_pScintilla->styleChange();
			    return TRUE;
		    }
		    return FALSE;
	    }
		case WM_CLOSE:
		    return TRUE;
		default:
		    return FALSE;
	}
	return FALSE;
}
