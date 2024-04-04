/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* This file is part of xlslib -- A multiplatform, C/C++ library
* for dynamic generation of Excel(TM) files.
*
* Copyright 2008-2011 David Hoerl All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are
* permitted provided that the following conditions are met:
*
*    1. Redistributions of source code must retain the above copyright notice, this list of
*       conditions and the following disclaimer.
*
*    2. Redistributions in binary form must reproduce the above copyright notice, this list
*       of conditions and the following disclaimer in the documentation and/or other materials
*       provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
* FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* File description:
*
*
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "xlslib-internal.h"
#pragma hdrstop

#define CPP_BRIDGE_XLS // @sobolev Обратить внимание (было до include'ов)

// since xlslib.h does not include these for C files
//#include "xlsys.h"
#ifdef HAVE_SYS_TYPES_H
//#include <sys/types.h>
#endif
//#include <string>
//#include "systype.h"
//#include "common.h"
//#include "record.h"
//#include "xlslib.h"

using namespace xlslib_core;
using namespace xlslib_strings;

extern "C" {
// Workbook
workbook *xlsNewWorkbook(void) { return new workbook; }
void xlsDeleteWorkbook(workbook * w) { delete w; }

worksheet *xlsWorkbookSheet(workbook * w, const char * sheetname) {
	std::string str = sheetname;
	return w->sheet(str);
}

worksheet *xlsWorkbookSheetW(workbook * w, const unichar_t * sheetname)
{
	ustring str = sheetname;
	return w->sheet(str);
}

worksheet *xlsWorkbookGetSheet(workbook * w, uint16 sheetnum)
{ return w->GetSheet(sheetnum); }

font_t *xlsWorkbookFont(workbook * w, const char * name)          {
	std::string str = name;
	return w->font(str);
}

format_t *xlsWorkbookFormat(workbook * w, const char * name)      {
	std::string str = name;
	return w->format(str);
}

format_t *xlsWorkbookFormatW(workbook * w, const unichar_t * name) {
	ustring str = name;
	return w->format(str);
}

xf_t *xlsWorkbookxFormat(workbook * w)                                           { return w->xformat(); }
xf_t *xlsWorkbookxFormatFont(workbook * w, font_t * font)         { return w->xformat(font); }

#if defined(HAVE_WORKING_ICONV)
int xlsWorkbookIconvInType(workbook * w, const char * inType)     { return w->iconvInType(inType); }
#endif
uint8 xlsWorkbookProperty(workbook * w, property_t prop, const char * s)
{
	std::string str = s;
	return w->property(prop, str) ? 1 : 0;
}

void xlsWorkBookWindPosition(workbook * w, uint16 horz, uint16 vert)
{ w->windPosition(horz, vert); }
void xlsWorkBookWindSize(workbook * w, uint16 horz, uint16 vert)
{ w->windSize(horz, vert); }
void xlsWorkBookFirstTab(workbook * w, uint16 firstTab)
{ w->firstTab(firstTab); }
void xlsWorkBookTabBarWidth(workbook * w, uint16 width)
{ w->tabBarWidth(width); }
int xlsWorkbookDump(workbook * w, const char * filename)                  {
	std::string str = filename;
	return w->Dump(str);
}

// Worksheet
void xlsWorksheetMakeActive(worksheet * w)                                       { return w->MakeActive(); }
cell_t *xlsWorksheetFindCell(worksheet * w, uint32 row, uint32 col)
{ return w->FindCell(row, col); }
// Cell operations
void xlsWorksheetMerge(worksheet * w, uint32 first_row, uint32 first_col, uint32 last_row, uint32 last_col)
{ return w->merge(first_row, first_col, last_row, last_col); }
void xlsWorksheetColwidth(worksheet * w, uint32 col, uint16 width, xf_t* pxformat)
{ return w->colwidth(col, width, pxformat); }
void xlsWorksheetRowheight(worksheet * w, uint32 row, uint16 height, xf_t* pxformat)
{ return w->rowheight(row, height, pxformat); }
// Ranges
range *xlsWorksheetRangegroup(worksheet * w, uint32 row1, uint32 col1, uint32 row2, uint32 col2)
{ return w->rangegroup(row1, col1, row2, col2); }

// Cells
cell_t *xlsWorksheetLabel(worksheet * w, uint32 row, uint32 col, const char * strlabel, xf_t * pxformat)
{
	std::string str = strlabel;
	return w->label(row, col, strlabel, pxformat);
}

cell_t *xlsWorksheetLabelW(worksheet * w, uint32 row, uint32 col, const unichar_t * strlabel, xf_t * pxformat)
{
	ustring str = strlabel;
	return w->label(row, col, strlabel, pxformat);
}

cell_t *xlsWorksheetBlank(worksheet * w, uint32 row, uint32 col, xf_t * pxformat)
{ return w->blank(row, col, pxformat); }

cell_t *xlsWorksheetNumberDbl(worksheet * w, uint32 row, uint32 col, double numval, xf_t * pxformat)
{ return w->number(row, col, numval, pxformat); }
// 536870911 >= numval >= -536870912
cell_t *xlsWorksheetNumberInt(worksheet * w, uint32 row, uint32 col, int32 numval, xf_t * pxformat)
{ return w->number(row, col, numval, pxformat); }

cell_t *xlsWorksheetBoolean(worksheet * w, uint32 row, uint32 col, int boolval, xf_t * pxformat)
{ return w->boolean(row, col, !!boolval, pxformat); }

cell_t *xlsWorksheetError(worksheet * w, uint32 row, uint32 col, errcode_t errval, xf_t * pxformat)
{ return w->error(row, col, errval, pxformat); }

cell_t *xlsWorksheetNote(worksheet * w, uint32 row, uint32 col, const char * remark, const char * author, xf_t * pxformat)
{
	std::string cmt = remark;
	std::string auth = author;

	return w->note(row, col, cmt, auth, pxformat);
}

cell_t *xlsWorksheetNoteW(worksheet * w, uint32 row, uint32 col, const unichar_t * remark, const unichar_t * author, xf_t * pxformat)
{
	ustring cmt = remark;
	ustring auth = author;

	return w->note(row, col, cmt, auth, pxformat);
}

formula_t *xlsWorksheetFormula(worksheet * w) { return w->formula_data(); }
void xlsFormulaPushBoolean(formula_t * formula, bool value) { formula->PushBoolean(value); }
void xlsFormulaPushMissingArgument(formula_t * formula) { formula->PushMissingArgument(); }
void xlsFormulaPushError(formula_t * formula, uint8 value) { formula->PushError(value); }
void xlsFormulaPushNumberInt(formula_t * formula, int32 value) { formula->PushInteger(value); }
void xlsFormulaPushNumberDbl(formula_t * formula, double value) { formula->PushFloatingPoint(value); }
void xlsFormulaPushNumberArray(formula_t * formula, double * values, size_t count)
{
	std::vector<double> vec;
	for(size_t i = 0; i<count; i++)
		vec.push_back(values[i]);
	formula->PushFloatingPointArray(vec);
}

void xlsFormulaPushOperator(formula_t * formula, expr_operator_code_t op) { formula->PushOperator(op); }
void xlsFormulaPushCellReference(formula_t * formula, cell_t * cell, cell_addr_mode_t opt) { formula->PushCellReference(*cell, opt); }
void xlsFormulaPushCellAreaReference(formula_t * formula, cell_t * upper_left_cell,
    cell_t * lower_right_cell, cell_addr_mode_t opt) { formula->PushCellAreaReference(*upper_left_cell, *lower_right_cell, opt); }
void xlsFormulaPushFunction(formula_t * formula, expr_function_code_t func) { formula->PushFunction(func); }
void xlsFormulaPushFunctionV(formula_t * formula, expr_function_code_t func, size_t arg_count) { formula->PushFunction(func, arg_count); }
void xlsFormulaPushCharacterArray(formula_t * formula, const char * text, size_t count)
{
	std::string str = "";
	for(size_t i = 0; i<count; i++) {
		str += text[i];
	}
	formula->PushText(str);
}

void xlsFormulaPushCharacterArrayW(formula_t * formula, const unichar_t * text, size_t count)
{
	ustring str = L"";
	for(size_t i = 0; i<count; i++) {
		str += text[i];
	}
	formula->PushText(str);
}

void xlsFormulaPushText(formula_t * formula, const char * text)
{
	std::string str = text;
	formula->PushText(str);
}

void xlsFormulaPushTextW(formula_t * formula, const unichar_t * text)
{
	ustring str = text;
	formula->PushText(str);
}

void xlsFormulaPushTextArray(formula_t * formula, const char ** text, size_t count)
{
	std::vector<std::string> vec;
	for(size_t i = 0; i<count; i++) {
		std::string str = text[i];
		vec.push_back(str);
	}
	formula->PushTextArray(vec);
}

void xlsFormulaPushTextArrayW(formula_t * formula, const unichar_t ** text, size_t count)
{
	std::vector<ustring> vec;
	for(size_t i = 0; i<count; i++) {
		ustring str = text[i];
		vec.push_back(str);
	}
	formula->PushTextArray(vec);
}

cell_t *xlsWorksheetFormulaCell(worksheet * w, uint32 row, uint32 col, formula_t * formula, xf_t * pxformat)
{
	return w->formula(row, col, formula, pxformat);
}

void xlsWorksheetValidateCell(worksheet * w, cell_t * cell, uint32 options,
    const formula_t * cond1, const formula_t * cond2,
    const char * prompt_title, const char * prompt_text,
    const char * error_title, const char * error_text)
{
	range_t * rg = new range_t;
	rg->first_row = cell->GetRow();
	rg->last_row = cell->GetRow();
	rg->first_col = cell->GetCol();
	rg->last_col = cell->GetCol();
	std::string sPromptTitle = prompt_title ? prompt_title : "";
	std::string sPromptText = prompt_text ? prompt_text : "";
	std::string sErrorTitle = error_title ? error_title : "";
	std::string sErrorText = error_text ? error_text : "";
	w->validate(rg, options, cond1, cond2,
	    sPromptTitle, sPromptText, sErrorTitle, sErrorText);
	delete rg;
}

void xlsWorksheetValidateCellW(worksheet * w, cell_t * cell, uint32 options,
    const formula_t * cond1, const formula_t * cond2,
    const unichar_t * prompt_title, const unichar_t * prompt_text,
    const unichar_t * error_title, const unichar_t * error_text)
{
	range_t * rg = new range_t;
	rg->first_row = cell->GetRow();
	rg->last_row = cell->GetRow();
	rg->first_col = cell->GetCol();
	rg->last_col = cell->GetCol();
	ustring sPromptTitle = prompt_title ? prompt_title : L"";
	ustring sPromptText = prompt_text ? prompt_text : L"";
	ustring sErrorTitle = error_title ? error_title : L"";
	ustring sErrorText = error_text ? error_text : L"";
	w->validate(rg, options, cond1, cond2,
	    sPromptTitle, sPromptText, sErrorTitle, sErrorText);
	delete rg;
}

void xlsWorksheetValidateCellArea(worksheet * w, cell_t * upper_left_cell,
    cell_t * lower_right_cell, uint32 options,
    const formula_t * cond1, const formula_t * cond2,
    const char * prompt_title, const char * prompt_text,
    const char * error_title, const char * error_text)
{
	range_t * rg = new range_t;
	rg->first_row = upper_left_cell->GetRow();
	rg->last_row = lower_right_cell->GetRow();
	rg->first_col = upper_left_cell->GetCol();
	rg->last_col = lower_right_cell->GetCol();
	std::string sPromptTitle = prompt_title ? prompt_title : "";
	std::string sPromptText = prompt_text ? prompt_text : "";
	std::string sErrorTitle = error_title ? error_title : "";
	std::string sErrorText = error_text ? error_text : "";
	w->validate(rg, options, cond1, cond2,
	    sPromptTitle, sPromptText, sErrorTitle, sErrorText);
	delete rg;
}

void xlsWorksheetValidateCellAreaW(worksheet * w, cell_t * upper_left_cell,
    cell_t * lower_right_cell, uint32 options,
    const formula_t * cond1, const formula_t * cond2,
    const unichar_t * prompt_title, const unichar_t * prompt_text,
    const unichar_t * error_title, const unichar_t * error_text)
{
	range_t * rg = new range_t;
	rg->first_row = upper_left_cell->GetRow();
	rg->last_row = lower_right_cell->GetRow();
	rg->first_col = upper_left_cell->GetCol();
	rg->last_col = lower_right_cell->GetCol();
	ustring sPromptTitle = prompt_title ? prompt_title : L"";
	ustring sPromptText = prompt_text ? prompt_text : L"";
	ustring sErrorTitle = error_title ? error_title : L"";
	ustring sErrorText = error_text ? error_text : L"";
	w->validate(rg, options, cond1, cond2,
	    sPromptTitle, sPromptText, sErrorTitle, sErrorText);
	delete rg;
}

void xlsWorksheetHyperLink(worksheet * w, cell_t * cell, const char * url, const char * mark)
{
	std::string sUrl = url;
	std::string sMark = mark ? mark : "";

	w->hyperLink(cell, sUrl, sMark);
}

void xlsWorksheetHyperLinkW(worksheet * w, cell_t * cell, const unichar_t * url, const unichar_t * mark)
{
	ustring sUrl = url;
	ustring sMark = mark ? mark : L"";

	w->hyperLink(cell, sUrl, sMark);
}

// Cells
// xf_i interface
void xlsCellFont(cell_t * c, font_t * fontidx)                            { return c->font(fontidx); }
void xlsCellFormat(cell_t * c, format_number_t format)           { return c->format(format); }
void xlsCellFormatP(cell_t * c, format_t * format)                        { return c->format(format); }
void xlsCellHalign(cell_t * c, halign_option_t ha_option)        { return c->halign(ha_option); }
void xlsCellValign(cell_t * c, valign_option_t va_option)        { return c->valign(va_option); }
void xlsCellIndent(cell_t * c, indent_option_t in_option)        { return c->indent(in_option); }
void xlsCellOrientation(cell_t * c, txtori_option_t ori_option)
{ return c->orientation(ori_option); }
void xlsCellFillfgcolor(cell_t * c, color_name_t color)          { return c->fillfgcolor(color); }
void xlsCellFillbgcolor(cell_t * c, color_name_t color)          { return c->fillbgcolor(color); }
void xlsCellFillstyle(cell_t * c, fill_option_t fill)            { return c->fillstyle(fill); }
void xlsCellLocked(cell_t * c, bool locked_opt)                          { return c->locked(locked_opt); }
void xlsCellHidden(cell_t * c, bool hidden_opt)                          { return c->hidden(hidden_opt); }
void xlsCellWrap(cell_t * c, bool wrap_opt)                                      { return c->wrap(wrap_opt); }
void xlsCellBorderstyle(cell_t * c, border_side_t side, border_style_t style)
{ return c->borderstyle(side, style); }
void xlsCellBordercolor(cell_t * c, border_side_t side, color_name_t color)
{ return c->bordercolor(side, color); }
void xlsCellBordercolorIdx(cell_t * c, border_side_t side, uint8 color)
{ return c->bordercolor(side, color); }
//font_i interface
void xlsCellFontname(cell_t * c, const char * fntname)            {
	std::string str = fntname;
	c->fontname(str);
}

void xlsCellFontheight(cell_t * c, uint16 fntheight)     { return c->fontheight(fntheight); }
void xlsCellFontbold(cell_t * c, boldness_option_t fntboldness)
{ return c->fontbold(fntboldness); }
void xlsCellFontunderline(cell_t * c, underline_option_t fntunderline)
{ return c->fontunderline(fntunderline); }
void xlsCellFontscript(cell_t * c, script_option_t fntscript){ return c->fontscript(fntscript); }
void xlsCellFontcolor(cell_t * c, color_name_t fntcolor)         { return c->fontcolor(fntcolor); }
void xlsCellFontitalic(cell_t * c, bool italic)                          { return c->fontitalic(italic); }
void xlsCellFontstrikeout(cell_t * c, bool so)                           { return c->fontstrikeout(so); }
void xlsCellFontoutline(cell_t * c, bool ol)                                     { return c->fontoutline(ol); }
void xlsCellFontshadow(cell_t * c, bool sh)                                      { return c->fontshadow(sh); }

uint32 xlsCellGetRow(cell_t * c)                                         { return c->GetRow(); }
uint32 xlsCellGetCol(cell_t * c)                                         { return c->GetCol(); }
// range
void xlsRangeCellcolor(range * r, color_name_t color)            { return r->cellcolor(color); }
// xformat
void xlsXformatSetFont(xf_t * x, font_t* fontidx)                        { return x->SetFont(fontidx); }
uint16 xlsXformatGetFontIndex(xf_t * x)                          { return x->GetFontIndex(); }
font_t* xlsXformatGetFont(xf_t * x)                                                      { return x->GetFont(); }
/* Format Index wrappers*/
void xlsXformatSetFormat(xf_t * x, format_number_t formatidx){ return x->SetFormat(formatidx); }
void xlsXformatSetFormatP(xf_t * x, format_t * fmt)                       { return x->SetFormat(fmt); }
/* Horizontal Align option wrappers*/
void xlsXformatSetHAlign(xf_t * x, halign_option_t ha_option){ return x->SetHAlign(ha_option); }
uint8 xlsXformatGetHAlign(xf_t * x)                                      { return x->GetHAlign(); }
/* Vertical Align option wrappers*/
void xlsXformatSetVAlign(xf_t * x, valign_option_t va_option){ return x->SetVAlign(va_option); }
uint8 xlsXformatGetVAlign(xf_t * x)                                      { return x->GetVAlign(); }
/* Indent option wrappers*/
void xlsXformatSetIndent(xf_t * x, indent_option_t in_option){ return x->SetIndent(in_option); }
uint8 xlsXformatGetIndent(xf_t * x)                                      { return x->GetIndent(); }
/* Text orientation option wrappers*/
void xlsXformatSetTxtOrientation(xf_t * x, txtori_option_t ori_option)
{ return x->SetTxtOrientation(ori_option); }
uint8 xlsXformatGetTxtOrientation(xf_t * x)                      { return x->GetTxtOrientation(); }
/* Fill Foreground color option wrappers*/
void xlsXformatSetFillFGColor(xf_t * x, color_name_t color)      { return x->SetFillFGColor(color); }
uint16 xlsXformatGetFillFGColorIdx(xf_t * x)                     { return x->GetFillFGColorIdx(); }
/* Fill Background color option wrappers*/
void xlsXformatSetFillBGColor(xf_t * x, color_name_t color)      { return x->SetFillBGColor(color); }
uint16 xlsXformatGetFillBGColorIdx(xf_t * x)                     { return x->GetFillBGColorIdx(); }
/* Fill Style option wrappers*/
void xlsXformatSetFillStyle(xf_t * x, fill_option_t fill)        { return x->SetFillStyle(fill); }
uint8 xlsXformatGetFillStyle(xf_t * x)                                   { return x->GetFillStyle(); }
/* Locked option wrappers*/
void xlsXformatSetLocked(xf_t * x, bool locked_opt)                      { return x->SetLocked(locked_opt); }
bool xlsXformatIsLocked(xf_t * x)                                                        { return x->IsLocked(); }
/* Hidden option wrappers*/
void xlsXformatSetHidden(xf_t * x, bool hidden_opt)                      { return x->SetHidden(hidden_opt); }
bool xlsXformatIsHidden(xf_t * x)                                                        { return x->IsHidden(); }
/* Wrap option wrappers*/
void xlsXformatSetWrap(xf_t * x, bool wrap_opt)                          { return x->SetWrap(wrap_opt); }
bool xlsXformatIsWrap(xf_t * x)                                                          { return x->IsWrap(); }
/* Cell option wrappers*/
void xlsXformatSetBorderStyle(xf_t * x, border_side_t side, border_style_t style)
{ return x->SetBorderStyle(side, style); }
void xlsXformatSetBorderColor(xf_t * x, border_side_t side, color_name_t color)
{ return x->SetBorderColor(side, color); }
void xlsXformatSetBorderColorIdx(xf_t * x, border_side_t side, uint8 color)
{ return x->SetBorderColor(side, color); }
uint8 xlsXformatGetBorderStyle(xf_t * x, border_side_t side)
{ return x->GetBorderStyle(side); }
uint16 xlsXformatGetBorderColorIdx(xf_t * x, border_side_t side)
{ return x->GetBorderColorIdx(side); }

// Font
void xlsFontSetName(font_t * f, const char * fntname)                     {
	std::string str = fntname;
	f->SetName(str);
}

char *xlsFontGetName(font_t * f, char * dst, size_t dstsize)      {
	const char * ptr = f->GetName().c_str();
	size_t len = strlen(ptr) + 1;
	if(len > dstsize)
		len = dstsize;
	memcpy(dst, ptr, len);
	dst[dstsize - 1] = 0;
	return dst;
}

/* FONT height wrappers*/
void xlsFontSetHeight(font_t * f, uint16 fntheight)      { return f->SetHeight(fntheight); }
uint16 xlsFontGetHeight(font_t * f)                                      { return f->GetHeight(); }
/* FONT boldstyle wrappers*/
void xlsFontSetBoldStyle(font_t * f, boldness_option_t fntboldness)
{ return f->SetBoldStyle(fntboldness); }
uint16 xlsFontGetBoldStyle(font_t * f)                                   { return f->GetBoldStyle(); }
/* FONT underline wrappers*/
void xlsFontSetUnderlineStyle(font_t * f, underline_option_t fntunderline)
{ return f->SetUnderlineStyle(fntunderline); }
uint8 xlsFontGetUnderlineStyle(font_t * f)                               { return f->GetUnderlineStyle(); }
/* FONT script wrappers*/
void xlsFontSetScriptStyle(font_t * f, script_option_t fntscript)
{ return f->SetScriptStyle(fntscript); }
uint16 xlsFontGetScriptStyle(font_t * f)                         { return f->GetScriptStyle(); }
/* FONT script wrappers*/
void xlsFontSetColor(font_t * f, color_name_t fntcolor)          { return f->SetColor(fntcolor); }
uint16 xlsFontGetColorIdx(font_t * f)                                    { return f->GetColorIdx(); }
void xlsFontSetItalic(font_t * f, bool italic)                           { return f->SetItalic(italic); }
void xlsFontSetStrikeout(font_t * f, bool so)                            { return f->SetStrikeout(so); }
/* FONT  attributes wrappers */
// Macintosh only
void xlsFontSetOutline(font_t * f, bool ol)                                      { return f->SetOutline(ol); }
void xlsFontSetShadow(font_t * f, bool sh)                                       { return f->SetShadow(sh); }

// these are accessing private members. Is this intended?
uint16 xlsFontGetAttributes(font_t * f)                          { return f->GetAttributes(); }
#if defined(DEPRECATED)
void xlsFontSetAttributes(font_t * f, uint16 attr)               { f->SetAttributes(attr); }
#endif
//uint32 xlsXformatGetSignature(xf_t *x)				{ return x->GetSignature(); }
bool xlsXformatIsCell(xf_t * x)                                                          { return x->IsCell(); }
void xlsXformatSetCellMode(xf_t * x, bool cellmode)                      { x->SetCellMode(cellmode); }
uint16 xlsXformatGetFormatIndex(xf_t * x)                                { return x->GetFormatIndex(); }
format_number_t xlsXformatGetFormat(xf_t * x)                            { return x->GetFormat(); }
void xlsCellSetXF(cell_t * c, xf_t * pxfval)                                      { c->SetXF(pxfval); }
uint16 xlsCellGetXFIndex(cell_t * c)                                     { return c->GetXFIndex(); }
//void xlsCellFontattr(cell_t *c, uint16 attr)		{ c->Fontattr(attr); }
}
