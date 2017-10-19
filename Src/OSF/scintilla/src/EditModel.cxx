// Scintilla source code edit control
/** @file EditModel.cxx
** Defines the editor state that must be visible to EditorView.
**/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <Platform.h>
#include <Scintilla.h>
#pragma hdrstop
#include <scintilla-internal.h>

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

Caret::Caret()
{
	//active = false;
	//on = false;
	Flags = 0;
	period = 500;
}

EditModel::EditModel()
{
	EditModelFlags = fPrimarySelection;
	//inOverstrike = false;
	//trackLineWidth = false;
	//primarySelection = true;
	xOffset = 0;
	posDrag = SelectionPosition(invalidPosition);
	braces[0] = invalidPosition;
	braces[1] = invalidPosition;
	bracesMatchStyle = STYLE_BRACEBAD;
	highlightGuideColumn = 0;
	imeInteraction = imeWindowed;
	foldFlags = 0;
	foldDisplayTextStyle = SC_FOLDDISPLAYTEXT_HIDDEN;
	hotspot = Range(invalidPosition);
	hoverIndicatorPos = invalidPosition;
	wrapWidth = LineLayout::wrapWidthInfinite;
	pdoc = new Document();
	pdoc->AddRef();
}

EditModel::~EditModel()
{
	pdoc->Release();
	pdoc = 0;
}

ColourDesired EditModel::SelectionBackground(const ViewStyle & vsDraw, bool main) const
{
	return main ? ((EditModelFlags & fPrimarySelection) ? vsDraw.selColours.back : vsDraw.selBackground2) : vsDraw.selAdditionalBackground;
}

