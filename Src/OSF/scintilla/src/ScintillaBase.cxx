// Scintilla source code edit control
/** @file ScintillaBase.cxx
** An enhanced subclass of Editor with calltips, autocomplete and context menu.
**/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#include <scintilla-internal.h>
#pragma hdrstop

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif
//
//
//
//static 
PRectangle PRectangle::FromInts(int left_, int top_, int right_, int bottom_)
{
	return PRectangle(static_cast<XYPOSITION>(left_), static_cast<XYPOSITION>(top_), static_cast<XYPOSITION>(right_), static_cast<XYPOSITION>(bottom_));
}

PRectangle::PRectangle(XYPOSITION left_, XYPOSITION top_, XYPOSITION right_, XYPOSITION bottom_) :
	left(left_), top(top_), right(right_), bottom(bottom_)
{
}

PRectangle::PRectangle() : left(0), top(0), right(0), bottom(0)
{
}

// Other automatically defined methods (assignment, copy constructor, destructor) are fine
bool FASTCALL PRectangle::operator == (const PRectangle & rc) const
{
	return (rc.left == left) && (rc.right == right) && (rc.top == top) && (rc.bottom == bottom);
}
	
bool FASTCALL PRectangle::Contains(const Point & pt) const
{
	return (pt.x >= left) && (pt.x <= right) && (pt.y >= top) && (pt.y <= bottom);
}
	
bool FASTCALL PRectangle::ContainsWholePixel(const Point & pt) const
{
	// Does the rectangle contain all of the pixel to left/below the point
	return (pt.x >= left) && ((pt.x+1) <= right) && (pt.y >= top) && ((pt.y+1) <= bottom);
}
	
bool FASTCALL PRectangle::Contains(const PRectangle & rc) const
{
	return (rc.left >= left) && (rc.right <= right) && (rc.top >= top) && (rc.bottom <= bottom);
}
	
bool FASTCALL PRectangle::Intersects(const PRectangle & other) const
{
	return (right > other.left) && (left < other.right) && (bottom > other.top) && (top < other.bottom);
}
	
void PRectangle::Move(XYPOSITION xDelta, XYPOSITION yDelta)
{
	left += xDelta;
	top += yDelta;
	right += xDelta;
	bottom += yDelta;
}
	
bool PRectangle::Empty() const
{
	return (Height() <= 0) || (Width() <= 0);
}
//
//
//
ScintillaBase::ScintillaBase() : displayPopupMenu(SC_POPUP_ALL), listType(0), maxListWidth(0), multiAutoCMode(SC_MULTIAUTOC_ONCE)
{
}

ScintillaBase::~ScintillaBase()
{
}

void ScintillaBase::Finalise()
{
	Editor::Finalise();
	popup.Destroy();
}

void ScintillaBase::AddCharUTF(const char * s, uint len, bool treatAsDBCS)
{
	bool isFillUp = ac.Active() && ac.IsFillUpChar(*s);
	if(!isFillUp) {
		Editor::AddCharUTF(s, len, treatAsDBCS);
	}
	if(ac.Active()) {
		AutoCompleteCharacterAdded(s[0]);
		// For fill ups add the character after the autocompletion has
		// triggered so containers see the key so can display a calltip.
		if(isFillUp) {
			Editor::AddCharUTF(s, len, treatAsDBCS);
		}
	}
}

void FASTCALL ScintillaBase::Command(int cmdId)
{
	switch(cmdId) {
		case idAutoComplete: break; // Nothing to do
		case idCallTip: break; // Nothing to do
		case idcmdUndo: WndProc(SCI_UNDO, 0, 0); break;
		case idcmdRedo: WndProc(SCI_REDO, 0, 0); break;
		case idcmdCut:  WndProc(SCI_CUT, 0, 0); break;
		case idcmdCopy: WndProc(SCI_COPY, 0, 0); break;
		case idcmdPaste: WndProc(SCI_PASTE, 0, 0); break;
		case idcmdDelete: WndProc(SCI_CLEAR, 0, 0); break;
		case idcmdSelectAll: WndProc(SCI_SELECTALL, 0, 0); break;
	}
}

int ScintillaBase::KeyCommand(uint iMessage)
{
	// Most key commands cancel autocompletion mode
	if(ac.Active()) {
		switch(iMessage) {
			// Except for these
			case SCI_DELETEBACK: 
				DelCharBack(true);
			    AutoCompleteCharacterDeleted();
			    EnsureCaretVisible();
			    return 0;
			case SCI_DELETEBACKNOTLINE:
			    DelCharBack(false);
			    AutoCompleteCharacterDeleted();
			    EnsureCaretVisible();
			    return 0;
			case SCI_LINEDOWN: AutoCompleteMove(1); return 0;
			case SCI_LINEUP: AutoCompleteMove(-1); return 0;
			case SCI_PAGEDOWN: AutoCompleteMove(ac.lb->GetVisibleRows()); return 0;
			case SCI_PAGEUP: AutoCompleteMove(-ac.lb->GetVisibleRows()); return 0;
			case SCI_VCHOME: AutoCompleteMove(-5000); return 0;
			case SCI_LINEEND: AutoCompleteMove(5000); return 0;
			case SCI_TAB: AutoCompleteCompleted(0, SC_AC_TAB); return 0;
			case SCI_NEWLINE: AutoCompleteCompleted(0, SC_AC_NEWLINE); return 0;
			default: AutoCompleteCancel();
		}
	}
	if(ct.IsInCollTipMode()) {
		if(!oneof7(iMessage, SCI_CHARLEFT, SCI_CHARLEFTEXTEND, SCI_CHARRIGHT, SCI_CHARRIGHTEXTEND, SCI_EDITTOGGLEOVERTYPE, SCI_DELETEBACK, SCI_DELETEBACKNOTLINE)) {
			ct.CallTipCancel();
		}
		if(oneof2(iMessage, SCI_DELETEBACK, SCI_DELETEBACKNOTLINE)) {
			if(Sel.MainCaret() <= ct.posStartCallTip) {
				ct.CallTipCancel();
			}
		}
	}
	return Editor::KeyCommand(iMessage);
}

void ScintillaBase::AutoCompleteDoubleClick(void * p)
{
	ScintillaBase * sci = static_cast<ScintillaBase *>(p);
	sci->AutoCompleteCompleted(0, SC_AC_DOUBLECLICK);
}

void ScintillaBase::AutoCompleteInsert(Position startPos, int removeLen, const char * text, int textLen)
{
	UndoGroup ug(pdoc);
	if(multiAutoCMode == SC_MULTIAUTOC_ONCE) {
		pdoc->DeleteChars(startPos, removeLen);
		const int lengthInserted = pdoc->InsertString(startPos, text, textLen);
		SetEmptySelection(startPos + lengthInserted);
	}
	else {
		// SC_MULTIAUTOC_EACH
		for(size_t r = 0; r < Sel.Count(); r++) {
			if(!RangeContainsProtected(Sel.Range(r).Start().Position(), Sel.Range(r).End().Position())) {
				int positionInsert = Sel.Range(r).Start().Position();
				positionInsert = RealizeVirtualSpace(positionInsert, Sel.Range(r).caret.VirtualSpace());
				if(positionInsert - removeLen >= 0) {
					positionInsert -= removeLen;
					pdoc->DeleteChars(positionInsert, removeLen);
				}
				const int lengthInserted = pdoc->InsertString(positionInsert, text, textLen);
				if(lengthInserted > 0) {
					Sel.Range(r).caret.SetPosition(positionInsert + lengthInserted);
					Sel.Range(r).anchor.SetPosition(positionInsert + lengthInserted);
				}
				Sel.Range(r).ClearVirtualSpace();
			}
		}
	}
}

void ScintillaBase::AutoCompleteStart(int lenEntered, const char * list)
{
	//Platform::DebugPrintf("AutoComplete %s\n", list);
	ct.CallTipCancel();
	if(ac.GetFlags() & ac.fChooseSingle && (listType == 0)) {
		if(list && !sstrchr(list, ac.GetSeparator())) {
			const char * typeSep = sstrchr(list, ac.GetTypesep());
			int lenInsert = typeSep ? static_cast<int>(typeSep-list) : static_cast<int>(sstrlen(list));
			if(ac.GetFlags() & ac.fIgnoreCase) {
				// May need to convert the case before invocation, so remove lenEntered characters
				AutoCompleteInsert(Sel.MainCaret() - lenEntered, lenEntered, list, lenInsert);
			}
			else {
				AutoCompleteInsert(Sel.MainCaret(), 0, list + lenEntered, lenInsert - lenEntered);
			}
			ac.Cancel();
			return;
		}
	}
	ac.Start(wMain, idAutoComplete, Sel.MainCaret(), PointMainCaret(), lenEntered, vs.lineHeight, IsUnicodeMode(), technology);
	PRectangle rcClient = GetClientRectangle();
	Point pt = LocationFromPosition(Sel.MainCaret() - lenEntered);
	PRectangle rcPopupBounds = wMain.GetMonitorRect(pt);
	if(rcPopupBounds.Height() == 0)
		rcPopupBounds = rcClient;
	int heightLB = ac.heightLBDefault;
	int widthLB = ac.widthLBDefault;
	if(pt.x >= rcClient.right - widthLB) {
		HorizontalScrollTo(static_cast<int>(xOffset + pt.x - rcClient.right + widthLB));
		Redraw();
		pt = PointMainCaret();
	}
	if(wMargin.GetID()) {
		Point ptOrigin = GetVisibleOriginInMain();
		pt.x += ptOrigin.x;
		pt.y += ptOrigin.y;
	}
	PRectangle rcac;
	rcac.left = pt.x - ac.lb->CaretFromEdge();
	if(pt.y >= rcPopupBounds.bottom - heightLB &&   // Won't fit below.
	    pt.y >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2) {     // and there is more room above.
		rcac.top = pt.y - heightLB;
		if(rcac.top < rcPopupBounds.top) {
			heightLB -= static_cast<int>(rcPopupBounds.top - rcac.top);
			rcac.top = rcPopupBounds.top;
		}
	}
	else {
		rcac.top = pt.y + vs.lineHeight;
	}
	rcac.right = rcac.left + widthLB;
	rcac.bottom = static_cast<XYPOSITION>(smin(static_cast<int>(rcac.top) + heightLB, static_cast<int>(rcPopupBounds.bottom)));
	ac.lb->SetPositionRelative(rcac, wMain);
	ac.lb->SetFont(vs.styles[STYLE_DEFAULT].font);
	uint aveCharWidth = static_cast<uint>(vs.styles[STYLE_DEFAULT].aveCharWidth);
	ac.lb->SetAverageCharWidth(aveCharWidth);
	ac.lb->SetDoubleClickAction(AutoCompleteDoubleClick, this);

	ac.SetList(list ? list : "");

	// Fiddle the position of the list so it is right next to the target and wide enough for all its strings
	PRectangle rcList = ac.lb->GetDesiredRect();
	int heightAlloced = static_cast<int>(rcList.bottom - rcList.top);
	widthLB = smax(widthLB, static_cast<int>(rcList.right - rcList.left));
	if(maxListWidth != 0)
		widthLB = smin(widthLB, static_cast<int>(aveCharWidth)*maxListWidth);
	// Make an allowance for large strings in list
	rcList.left = pt.x - ac.lb->CaretFromEdge();
	rcList.right = rcList.left + widthLB;
	if(((pt.y + vs.lineHeight) >= (rcPopupBounds.bottom - heightAlloced)) &&   // Won't fit below.
	    ((pt.y + vs.lineHeight / 2) >= (rcPopupBounds.bottom + rcPopupBounds.top) / 2)) {     // and there is more room above.
		rcList.top = pt.y - heightAlloced;
	}
	else {
		rcList.top = pt.y + vs.lineHeight;
	}
	rcList.bottom = rcList.top + heightAlloced;
	ac.lb->SetPositionRelative(rcList, wMain);
	ac.Show(true);
	if(lenEntered != 0) {
		AutoCompleteMoveToCurrentWord();
	}
}

void ScintillaBase::AutoCompleteCancel()
{
	if(ac.Active()) {
		SCNotification scn; // = {};
		scn.nmhdr.code = SCN_AUTOCCANCELLED;
		//scn.wParam = 0;
		//scn.listType = 0;
		NotifyParent(scn);
	}
	ac.Cancel();
}

void ScintillaBase::AutoCompleteMove(int delta)
{
	ac.Move(delta);
}

void ScintillaBase::AutoCompleteMoveToCurrentWord()
{
	std::string wordCurrent = RangeText(ac.posStart - ac.startLen, Sel.MainCaret());
	ac.Select(wordCurrent.c_str());
}

void ScintillaBase::AutoCompleteCharacterAdded(char ch)
{
	if(ac.IsFillUpChar(ch))
		AutoCompleteCompleted(ch, SC_AC_FILLUP);
	else if(ac.IsStopChar(ch))
		AutoCompleteCancel();
	else
		AutoCompleteMoveToCurrentWord();
}

void ScintillaBase::AutoCompleteCharacterDeleted()
{
	if(Sel.MainCaret() < (ac.posStart - ac.startLen))
		AutoCompleteCancel();
	else if(ac.GetFlags() & ac.fCancelAtStartPos && (Sel.MainCaret() <= ac.posStart))
		AutoCompleteCancel();
	else
		AutoCompleteMoveToCurrentWord();
	SCNotification scn; // = {};
	scn.nmhdr.code = SCN_AUTOCCHARDELETED;
	//scn.wParam = 0;
	//scn.listType = 0;
	NotifyParent(scn);
}

void ScintillaBase::AutoCompleteCompleted(char ch, uint completionMethod)
{
	int item = ac.GetSelection();
	if(item == -1)
		AutoCompleteCancel();
	else {
		const std::string selected = ac.GetValue(item);
		ac.Show(false);
		SCNotification scn; // = {};
		scn.nmhdr.code = listType > 0 ? SCN_USERLISTSELECTION : SCN_AUTOCSELECTION;
		//scn.message = 0;
		scn.ch = ch;
		scn.listCompletionMethod = completionMethod;
		scn.wParam = listType;
		scn.listType = listType;
		Position firstPos = ac.posStart - ac.startLen;
		scn.position = firstPos;
		scn.lParam = firstPos;
		scn.text = selected.c_str();
		NotifyParent(scn);
		if(ac.Active()) {
			ac.Cancel();
			if(listType > 0)
				return;
			Position endPos = Sel.MainCaret();
			if(ac.GetFlags() & ac.fDropRestOfWord)
				endPos = pdoc->ExtendWordSelect(endPos, 1, true);
			if(endPos < firstPos)
				return;
			AutoCompleteInsert(firstPos, endPos - firstPos, selected.c_str(), static_cast<int>(selected.length()));
			SetLastXChosen();
			scn.nmhdr.code = SCN_AUTOCCOMPLETED;
			NotifyParent(scn);
		}
	}
}

int ScintillaBase::AutoCompleteGetCurrent() const
{
	if(!ac.Active())
		return -1;
	return ac.GetSelection();
}

int ScintillaBase::AutoCompleteGetCurrentText(char * buffer) const
{
	if(ac.Active()) {
		int item = ac.GetSelection();
		if(item != -1) {
			const std::string selected = ac.GetValue(item);
			if(buffer != NULL)
				memcpy(buffer, selected.c_str(), selected.length()+1);
			return static_cast<int>(selected.length());
		}
	}
	if(buffer != NULL)
		*buffer = '\0';
	return 0;
}

void ScintillaBase::CallTipShow(Point pt, const char * defn)
{
	ac.Cancel();
	// If container knows about STYLE_CALLTIP then use it in place of the
	// STYLE_DEFAULT for the face name, size and character set. Also use it
	// for the foreground and background colour.
	int ctStyle = ct.UseStyleCallTip() ? STYLE_CALLTIP : STYLE_DEFAULT;
	if(ct.UseStyleCallTip()) {
		ct.SetForeBack(vs.styles[STYLE_CALLTIP].fore, vs.styles[STYLE_CALLTIP].back);
	}
	if(wMargin.GetID()) {
		Point ptOrigin = GetVisibleOriginInMain();
		pt.x += ptOrigin.x;
		pt.y += ptOrigin.y;
	}
	PRectangle rc = ct.CallTipStart(Sel.MainCaret(), pt, vs.lineHeight, defn, vs.styles[ctStyle].fontName, 
		vs.styles[ctStyle].sizeZoomed, CodePage(), vs.styles[ctStyle].characterSet, vs.technology, wMain);
	// If the call-tip window would be out of the client
	// space
	PRectangle rcClient = GetClientRectangle();
	int offset = vs.lineHeight + static_cast<int>(rc.Height());
	// adjust so it displays above the text.
	if(rc.bottom > rcClient.bottom && rc.Height() < rcClient.Height()) {
		rc.top -= offset;
		rc.bottom -= offset;
	}
	// adjust so it displays below the text.
	if(rc.top < rcClient.top && rc.Height() < rcClient.Height()) {
		rc.top += offset;
		rc.bottom += offset;
	}
	// Now display the window.
	CreateCallTipWindow(rc);
	ct.wCallTip.SetPositionRelative(rc, wMain);
	ct.wCallTip.Show();
}

void ScintillaBase::CallTipClick()
{
	SCNotification scn; // = {};
	scn.nmhdr.code = SCN_CALLTIPCLICK;
	scn.position = ct.clickPlace;
	NotifyParent(scn);
}

bool ScintillaBase::ShouldDisplayPopup(Point ptInWindowCoordinates) const
{
	return (displayPopupMenu == SC_POPUP_ALL || (displayPopupMenu == SC_POPUP_TEXT && !PointInSelMargin(ptInWindowCoordinates)));
}

void ScintillaBase::ContextMenu(Point pt)
{
	if(displayPopupMenu) {
		bool writable = !WndProc(SCI_GETREADONLY, 0, 0);
		popup.CreatePopUp();
		AddToPopUp("Undo", idcmdUndo, writable && pdoc->CanUndo());
		AddToPopUp("Redo", idcmdRedo, writable && pdoc->CanRedo());
		AddToPopUp("");
		AddToPopUp("Cut", idcmdCut, writable && !Sel.Empty());
		AddToPopUp("Copy", idcmdCopy, !Sel.Empty());
		AddToPopUp("Paste", idcmdPaste, writable && WndProc(SCI_CANPASTE, 0, 0));
		AddToPopUp("Delete", idcmdDelete, writable && !Sel.Empty());
		AddToPopUp("");
		AddToPopUp("Select All", idcmdSelectAll);
		popup.Show(pt, wMain);
	}
}

void ScintillaBase::CancelModes()
{
	AutoCompleteCancel();
	ct.CallTipCancel();
	Editor::CancelModes();
}

void ScintillaBase::ButtonDownWithModifiers(Point pt, uint curTime, int modifiers)
{
	CancelModes();
	Editor::ButtonDownWithModifiers(pt, curTime, modifiers);
}

void ScintillaBase::ButtonDown(Point pt, uint curTime, bool shift, bool ctrl, bool alt)
{
	ButtonDownWithModifiers(pt, curTime, ModifierFlags(shift, ctrl, alt));
}

void ScintillaBase::RightButtonDownWithModifiers(Point pt, uint curTime, int modifiers)
{
	CancelModes();
	Editor::RightButtonDownWithModifiers(pt, curTime, modifiers);
}

#ifdef SCI_LEXER

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class LexState : public LexInterface {
public:
	explicit LexState(Document * pdoc_);
	virtual ~LexState();
	void SetLexer(uptr_t wParam);
	void SetLexerLanguage(const char * languageName);
	const char * DescribeWordListSets();
	void SetWordList(int n, const char * wl);
	const char * GetName() const;
	void * PrivateCall(int operation, void * pointer);
	const char * PropertyNames();
	int PropertyType(const char * name);
	const char * DescribeProperty(const char * name);
	void PropSet(const char * key, const char * val);
	const char * PropGet(const char * key) const;
	int PropGetInt(const char * key, int defaultValue = 0) const;
	int PropGetExpanded(const char * key, char * result) const;

	int LineEndTypesSupported();
	int AllocateSubStyles(int styleBase, int numberStyles);
	int SubStylesStart(int styleBase);
	int SubStylesLength(int styleBase);
	int StyleFromSubStyle(int subStyle);
	int PrimaryStyleFromStyle(int style);
	void FreeSubStyles();
	void SetIdentifiers(int style, const char * identifiers);
	int DistanceToSecondaryStyles();
	const char * GetSubStyleBases();

	int lexLanguage;
private:
	void SetLexerModule(const LexerModule * lex);

	const LexerModule * lexCurrent;
	PropSetSimple props;
	int interfaceVersion;
};

#ifdef SCI_NAMESPACE
}
#endif

LexState::LexState(Document * pdoc_) : LexInterface(pdoc_), lexCurrent(0), interfaceVersion(lvOriginal), lexLanguage(SCLEX_CONTAINER)
{
	performingStyle = false;
}

LexState::~LexState()
{
	if(instance) {
		instance->Release();
		instance = 0;
	}
}

LexState * ScintillaBase::DocumentLexState()
{
	SETIFZ(pdoc->pli, new LexState(pdoc));
	return static_cast<LexState *>(pdoc->pli);
}

void LexState::SetLexerModule(const LexerModule * lex)
{
	if(lex != lexCurrent) {
		if(instance) {
			instance->Release();
			instance = 0;
		}
		interfaceVersion = lvOriginal;
		lexCurrent = lex;
		if(lexCurrent) {
			instance = lexCurrent->Create();
			interfaceVersion = instance->Version();
		}
		pdoc->LexerChanged();
	}
}

void LexState::SetLexer(uptr_t wParam)
{
	lexLanguage = static_cast<int>(wParam);
	if(lexLanguage == SCLEX_CONTAINER) {
		SetLexerModule(0);
	}
	else {
		const LexerModule * lex = Catalogue::Find(lexLanguage);
		SETIFZ(lex, Catalogue::Find(SCLEX_NULL));
		SetLexerModule(lex);
	}
}

void LexState::SetLexerLanguage(const char * languageName)
{
	const LexerModule * lex = Catalogue::Find(languageName);
	if(SETIFZ(lex, Catalogue::Find(SCLEX_NULL)))
		lexLanguage = lex->GetLanguage();
	SetLexerModule(lex);
}

void LexState::SetWordList(int n, const char * wl)
{
	if(instance) {
		int firstModification = instance->WordListSet(n, wl);
		if(firstModification >= 0) {
			pdoc->ModifiedAt(firstModification);
		}
	}
}

const char * LexState::DescribeWordListSets() { return instance ? instance->DescribeWordListSets() : 0; }
const char * LexState::GetName() const { return lexCurrent ? lexCurrent->languageName : ""; }
void * LexState::PrivateCall(int operation, void * pointer) { return (pdoc && instance) ? instance->PrivateCall(operation, pointer) : 0; }
const char * LexState::PropertyNames() { return instance ? instance->PropertyNames() : 0; }
int LexState::PropertyType(const char * name) { return instance ? instance->PropertyType(name) : SC_TYPE_BOOLEAN; }
const char * LexState::DescribeProperty(const char * name) { return instance ? instance->DescribeProperty(name) : 0; }

void LexState::PropSet(const char * key, const char * val)
{
	props.Set(key, val);
	if(instance) {
		int firstModification = instance->PropertySet(key, val);
		if(firstModification >= 0)
			pdoc->ModifiedAt(firstModification);
	}
}

const char * LexState::PropGet(const char * key) const { return props.Get(key); }
int LexState::PropGetInt(const char * key, int defaultValue) const { return props.GetInt(key, defaultValue); }
int LexState::PropGetExpanded(const char * key, char * result) const { return props.GetExpanded(key, result); }

int LexState::LineEndTypesSupported()
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->LineEndTypesSupported();
	}
	return 0;
}

int LexState::AllocateSubStyles(int styleBase, int numberStyles)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->AllocateSubStyles(styleBase, numberStyles);
	}
	return -1;
}

int LexState::SubStylesStart(int styleBase)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->SubStylesStart(styleBase);
	}
	return -1;
}

int LexState::SubStylesLength(int styleBase)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->SubStylesLength(styleBase);
	}
	return 0;
}

int LexState::StyleFromSubStyle(int subStyle)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->StyleFromSubStyle(subStyle);
	}
	return 0;
}

int LexState::PrimaryStyleFromStyle(int style)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->PrimaryStyleFromStyle(style);
	}
	return 0;
}

void LexState::FreeSubStyles()
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		static_cast<ILexerWithSubStyles *>(instance)->FreeSubStyles();
	}
}

void LexState::SetIdentifiers(int style, const char * identifiers)
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		static_cast<ILexerWithSubStyles *>(instance)->SetIdentifiers(style, identifiers);
		pdoc->ModifiedAt(0);
	}
}

int LexState::DistanceToSecondaryStyles()
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->DistanceToSecondaryStyles();
	}
	return 0;
}

const char * LexState::GetSubStyleBases()
{
	if(instance && (interfaceVersion >= lvSubStyles)) {
		return static_cast<ILexerWithSubStyles *>(instance)->GetSubStyleBases();
	}
	return "";
}

#endif

void ScintillaBase::NotifyStyleToNeeded(int endStyleNeeded)
{
#ifdef SCI_LEXER
	if(DocumentLexState()->lexLanguage != SCLEX_CONTAINER) {
		int lineEndStyled = pdoc->LineFromPosition(pdoc->GetEndStyled());
		int endStyled = pdoc->LineStart(lineEndStyled);
		DocumentLexState()->Colourise(endStyled, endStyleNeeded);
		return;
	}
#endif
	Editor::NotifyStyleToNeeded(endStyleNeeded);
}

void ScintillaBase::NotifyLexerChanged(Document *, void *)
{
#ifdef SCI_LEXER
	vs.EnsureStyle(0xff);
#endif
}

sptr_t ScintillaBase::WndProc(uint iMessage, uptr_t wParam, sptr_t lParam)
{
	switch(iMessage) {
		case SCI_AUTOCSHOW:
		    listType = 0;
		    AutoCompleteStart(static_cast<int>(wParam), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_AUTOCCANCEL: ac.Cancel(); break;
		case SCI_AUTOCCOMPLETE: AutoCompleteCompleted(0, SC_AC_COMMAND); break;
		case SCI_AUTOCSETSEPARATOR: ac.SetSeparator(static_cast<char>(wParam)); break;
		case SCI_AUTOCSTOPS: ac.SetStopChars(reinterpret_cast<char *>(lParam)); break;
		case SCI_AUTOCSELECT: ac.Select(reinterpret_cast<char *>(lParam)); break;
		case SCI_AUTOCSETCANCELATSTART: ac.SetFlag(ac.fCancelAtStartPos, wParam); break;
		case SCI_AUTOCSETFILLUPS: ac.SetFillUpChars(reinterpret_cast<char *>(lParam)); break;
		case SCI_AUTOCSETCHOOSESINGLE: ac.SetFlag(ac.fChooseSingle, wParam); break;
		case SCI_AUTOCSETIGNORECASE: ac.SetFlag(ac.fIgnoreCase, wParam); break;
		case SCI_AUTOCSETCASEINSENSITIVEBEHAVIOUR: ac.ignoreCaseBehaviour = static_cast<uint>(wParam); break;
		case SCI_AUTOCSETMULTI: multiAutoCMode = static_cast<int>(wParam); break;
		case SCI_AUTOCSETORDER: ac.autoSort = static_cast<int>(wParam); break;
		case SCI_USERLISTSHOW:
		    listType = static_cast<int>(wParam);
		    AutoCompleteStart(0, reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_AUTOCSETAUTOHIDE: ac.SetFlag(ac.fAutoHide, wParam); break;
		case SCI_AUTOCSETDROPRESTOFWORD: ac.SetFlag(ac.fDropRestOfWord, wParam); break;
		case SCI_AUTOCSETMAXHEIGHT: ac.lb->SetVisibleRows(static_cast<int>(wParam)); break;
		case SCI_AUTOCSETMAXWIDTH:
		    maxListWidth = static_cast<int>(wParam);
		    break;
		case SCI_REGISTERIMAGE:
		    ac.lb->RegisterImage(static_cast<int>(wParam), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_REGISTERRGBAIMAGE:
		    ac.lb->RegisterRGBAImage(static_cast<int>(wParam), static_cast<int>(sizeRGBAImage.x), static_cast<int>(sizeRGBAImage.y),
		    reinterpret_cast<uchar *>(lParam));
		    break;
		case SCI_CLEARREGISTEREDIMAGES:
		    ac.lb->ClearRegisteredImages();
		    break;
		case SCI_AUTOCSETTYPESEPARATOR:
		    ac.SetTypesep(static_cast<char>(wParam));
		    break;
		case SCI_CALLTIPSHOW:
		    CallTipShow(LocationFromPosition(static_cast<int>(wParam)), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_CALLTIPCANCEL:
		    ct.CallTipCancel();
		    break;
		case SCI_CALLTIPSETPOSSTART:
		    ct.posStartCallTip = static_cast<int>(wParam);
		    break;
		case SCI_CALLTIPSETHLT:
		    ct.SetHighlight(static_cast<int>(wParam), static_cast<int>(lParam));
		    break;
		case SCI_CALLTIPSETBACK:
		    ct.colourBG = ColourDesired(static_cast<long>(wParam));
		    vs.styles[STYLE_CALLTIP].back = ct.colourBG;
		    InvalidateStyleRedraw();
		    break;
		case SCI_CALLTIPSETFORE:
		    ct.colourUnSel = ColourDesired(static_cast<long>(wParam));
		    vs.styles[STYLE_CALLTIP].fore = ct.colourUnSel;
		    InvalidateStyleRedraw();
		    break;
		case SCI_CALLTIPSETFOREHLT:
		    ct.colourSel = ColourDesired(static_cast<long>(wParam));
		    InvalidateStyleRedraw();
		    break;
		case SCI_CALLTIPUSESTYLE:
		    ct.SetTabSize(static_cast<int>(wParam));
		    InvalidateStyleRedraw();
		    break;
		case SCI_CALLTIPSETPOSITION:
		    ct.SetPosition(wParam != 0);
		    InvalidateStyleRedraw();
		    break;
		case SCI_USEPOPUP:
		    displayPopupMenu = static_cast<int>(wParam);
		    break;
#ifdef SCI_LEXER
		case SCI_SETLEXER:
		    DocumentLexState()->SetLexer(static_cast<int>(wParam));
		    break;
		case SCI_COLOURISE:
		    if(DocumentLexState()->lexLanguage == SCLEX_CONTAINER) {
			    pdoc->ModifiedAt(static_cast<int>(wParam));
			    NotifyStyleToNeeded((lParam == -1) ? pdoc->Length() : static_cast<int>(lParam));
		    }
		    else {
			    DocumentLexState()->Colourise(static_cast<int>(wParam), static_cast<int>(lParam));
		    }
		    Redraw();
		    break;
		case SCI_SETPROPERTY:
		    DocumentLexState()->PropSet(reinterpret_cast<const char *>(wParam), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_SETKEYWORDS:
		    DocumentLexState()->SetWordList(static_cast<int>(wParam), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_SETLEXERLANGUAGE:
		    DocumentLexState()->SetLexerLanguage(reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_FREESUBSTYLES:
		    DocumentLexState()->FreeSubStyles();
		    break;
		case SCI_SETIDENTIFIERS:
		    DocumentLexState()->SetIdentifiers(static_cast<int>(wParam), reinterpret_cast<const char *>(lParam));
		    break;
		case SCI_AUTOCGETCHOOSESINGLE: return BIN(ac.GetFlags() & ac.fChooseSingle);
		case SCI_AUTOCACTIVE: return ac.Active();
		case SCI_AUTOCPOSSTART: return ac.posStart;
		case SCI_AUTOCGETSEPARATOR: return ac.GetSeparator();
		case SCI_AUTOCGETCURRENT: return AutoCompleteGetCurrent();
		case SCI_AUTOCGETCURRENTTEXT: return AutoCompleteGetCurrentText(reinterpret_cast<char *>(lParam));
		case SCI_AUTOCGETCANCELATSTART: return BIN(ac.GetFlags() & ac.fCancelAtStartPos);
		case SCI_AUTOCGETIGNORECASE: return BIN(ac.GetFlags() & ac.fIgnoreCase);
		case SCI_AUTOCGETCASEINSENSITIVEBEHAVIOUR: return ac.ignoreCaseBehaviour;
		case SCI_AUTOCGETMULTI: return multiAutoCMode;
		case SCI_AUTOCGETORDER: return ac.autoSort;
		case SCI_AUTOCGETAUTOHIDE: return BIN(ac.GetFlags() & ac.fAutoHide);
		case SCI_AUTOCGETDROPRESTOFWORD: return BIN(ac.GetFlags() & ac.fDropRestOfWord);
		case SCI_AUTOCGETMAXHEIGHT: return ac.lb->GetVisibleRows();
		case SCI_AUTOCGETMAXWIDTH: return maxListWidth;
		case SCI_AUTOCGETTYPESEPARATOR: return ac.GetTypesep();
		case SCI_CALLTIPACTIVE: return ct.IsInCollTipMode();
		case SCI_CALLTIPPOSSTART: return ct.posStartCallTip;
		case SCI_GETLEXER: return DocumentLexState()->lexLanguage;
		case SCI_GETPROPERTY: return StringResult(lParam, DocumentLexState()->PropGet(reinterpret_cast<const char *>(wParam)));
		case SCI_GETPROPERTYEXPANDED: return DocumentLexState()->PropGetExpanded(reinterpret_cast<const char *>(wParam), reinterpret_cast<char *>(lParam));
		case SCI_GETPROPERTYINT: return DocumentLexState()->PropGetInt(reinterpret_cast<const char *>(wParam), static_cast<int>(lParam));
		case SCI_GETLEXERLANGUAGE: return StringResult(lParam, DocumentLexState()->GetName());
		case SCI_PRIVATELEXERCALL: return reinterpret_cast<sptr_t> (DocumentLexState()->PrivateCall(static_cast<int>(wParam), reinterpret_cast<void *>(lParam)));
		case SCI_GETSTYLEBITSNEEDED: return 8;
		case SCI_PROPERTYNAMES: return StringResult(lParam, DocumentLexState()->PropertyNames());
		case SCI_PROPERTYTYPE: return DocumentLexState()->PropertyType(reinterpret_cast<const char *>(wParam));
		case SCI_DESCRIBEPROPERTY: return StringResult(lParam, DocumentLexState()->DescribeProperty(reinterpret_cast<const char *>(wParam)));
		case SCI_DESCRIBEKEYWORDSETS: return StringResult(lParam, DocumentLexState()->DescribeWordListSets());
		case SCI_GETLINEENDTYPESSUPPORTED: return DocumentLexState()->LineEndTypesSupported();
		case SCI_ALLOCATESUBSTYLES: return DocumentLexState()->AllocateSubStyles(static_cast<int>(wParam), static_cast<int>(lParam));
		case SCI_GETSUBSTYLESSTART: return DocumentLexState()->SubStylesStart(static_cast<int>(wParam));
		case SCI_GETSUBSTYLESLENGTH: return DocumentLexState()->SubStylesLength(static_cast<int>(wParam));
		case SCI_GETSTYLEFROMSUBSTYLE: return DocumentLexState()->StyleFromSubStyle(static_cast<int>(wParam));
		case SCI_GETPRIMARYSTYLEFROMSTYLE: return DocumentLexState()->PrimaryStyleFromStyle(static_cast<int>(wParam));
		case SCI_DISTANCETOSECONDARYSTYLES: return DocumentLexState()->DistanceToSecondaryStyles();
		case SCI_GETSUBSTYLEBASES: return StringResult(lParam, DocumentLexState()->GetSubStyleBases());
#endif
		default: return Editor::WndProc(iMessage, wParam, lParam);
	}
	return 0l;
}
