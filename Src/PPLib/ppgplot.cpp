// PPGPLOT.CPP
// Copyright (c) A.Sobolev 2008, 2011, 2014, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <process.h>

PPGpStyle::PPGpStyle()
{
	THISZERO();
}

int PPGpStyle::SetLine(COLORREF c, float width)
{
	THISZERO();
	Kind = kLine;
	C = c;
	Sz = width;
	return 1;
}

int PPGpStyle::SetPoint(COLORREF c, int type, float size)
{
	THISZERO();
	Kind = kPoint;
	C = c;
	T = type;
	Sz = size;
	return 1;
}

int PPGpStyle::SetPointVarColor(int type, float size)
{
	THISZERO();
	Kind = kPoint;
	T = type;
	Sz = size;
	Flags |= fVariableColor;
	return 1;
}

int PPGpStyle::SetFill(int type, long ext, int noborder)
{
	THISZERO();
	Kind = kFill;
	T = type;
	if(noborder)
		Flags |= fNoBorder;
	return 1;
}

int PPGpStyle::ToStr(SString & rBuf) const
{
	int    ok = -1;
	if(Flags & fIndex) {
		rBuf.Space().Cat("ls").Space().Cat(Idx);
		ok = 1;
	}
	else if(Kind == kLine) {
		if(C) {
			rBuf.Space().Cat("lc").Space().Cat("rgb").Space();
			Generator_GnuPlot::ColorToStr(C, rBuf);
			ok = 1;
		}
		if(Sz != 0.0f) {
			rBuf.Space().Cat("lw").Space().Cat((long)Sz);
			ok = 1;
		}
	}
	else if(Kind == kPoint) {
		if(Flags & fVariableColor)
			rBuf.Space().Cat("lc").Space().Cat("rgb").Space().Cat("variable");
		else if(C) {
			rBuf.Space().Cat("lc").Space().Cat("rgb").Space();
			Generator_GnuPlot::ColorToStr(C, rBuf);
			ok = 1;
		}
		if(T > 0) {
			rBuf.Space().Cat("pt").Space().Cat(T);
			ok = 1;
		}
		if(Sz != 0.0f) {
			rBuf.Space().Cat("ps").Space().Cat((double)Sz);
			ok = 1;
		}
	}
	else if(Kind == kFill) {
		if(T == ftEmpty) {
			rBuf.Space().Cat("empty");
			ok = 1;
		}
		else if(T == ftSolid) {
			rBuf.Space().Cat("solid");
			if(Ext)
				rBuf.Space().Cat(fdiv100i(Ext));
			ok = 1;
		}
		else if(T == ftPattern) {
			rBuf.Space().Cat("pattern").Space().Cat(Ext);
			ok = 1;
		}
		if(Flags & fNoBorder) {
			rBuf.Space().Cat("noborder");
			ok = 1;
		}
	}
	return ok;
}
//
//
//
PPGpPlotItem::PPGpPlotItem(const char * pSrc, const char * pTitle, int plotStyle)
{
	Flags = 0;
	DataSrc = pSrc;
	Flags |= fDataFile;
	Title = pTitle;
	S = plotStyle;
}

int PPGpPlotItem::AddDataIndex(int idx)
{
	assert(idx > 0);
	return IdxList.add(idx);
}

int PPGpPlotItem::AddDataIndex(const char * pFunc)
{
	if(pFunc) {
		long _min = MAXLONG;
		for(uint i = 0; i < IdxList.getCount(); i++)
			SETMIN(_min, IdxList.get(i));
		if(_min > 0)
			_min = 0;
		_min--;
		StrIdxList.Add(_min, pFunc);
		IdxList.add(_min);
		return 1;
	}
	else
		return -1;
}

int PPGpPlotItem::ToStr(SString & rBuf) const
{
	SString temp_buf;
	if(Flags & fDataFile) {
		temp_buf = DataSrc;
		temp_buf.ReplaceChar('\\', '/');
		rBuf.CatQStr(temp_buf.NotEmpty() ? temp_buf : "");
	}
	else if(DataSrc.NotEmpty())
		rBuf.Cat(DataSrc);
	if(IdxList.getCount()) {
		rBuf.Space().Cat("using").Space();
		for(uint i = 0; i < IdxList.getCount(); i++) {
			long idx = IdxList.get(i);
			assert(idx != 0);
			if(i)
				rBuf.CatChar(':');
			if(idx > 0)
				rBuf.Cat(idx);
			else {
				int r = StrIdxList.Get(idx, temp_buf);
				assert(r);
				rBuf.Cat(temp_buf);
			}
		}
	}
	if(Title.NotEmpty())
		rBuf.Space().Cat("title").Space().CatQStr(Title);
	else
		rBuf.Space().Cat("notitle");
	{
		const char * p_s = 0;
		switch(S) {
			case sLines:          p_s = "lines"; break;
			case sPoints:         p_s = "points"; break;
			case sLinesPoints:    p_s = "linespoints"; break;
			case sImpulses:       p_s = "impulses"; break;
			case sDots:           p_s = "dots"; break;
			case sSteps:          p_s = "steps"; break;
			case sFSteps:         p_s = "fsteps"; break;
			case sHiSteps:        p_s = "histeps"; break;
			case sErrorBars:      p_s = "errorbars"; break;
			case sLabels:         p_s = "labels"; break;
			case sXErrorBars:     p_s = "xerrorbars"; break;
			case sYEerrorBars:    p_s = "yerrorbars"; break;
			case sXYErrorBars:    p_s = "xyerrorbars"; break;
			case sErrorLines:     p_s = "errorlines"; break;
			case sXErrorLines:    p_s = "xerrorlines"; break;
			case sYErrorLines:    p_s = "yerrorlines"; break;
			case sXYErrorLines:   p_s = "xyerrorlines"; break;
			case sBoxes:          p_s = "boxes"; break;
			case sHistograms:     p_s = "histograms"; break;
			case sFilledCurves:   p_s = "filledcurves"; break;
			case sBoxErrorBars:   p_s = "boxerrorbars"; break;
			case sBoxXYErrorBars: p_s = "boxxyerrorbars"; break;
			case sEnanceBars:     p_s = "enancebars"; break;
			case sCandleSticks:   p_s = "candlesticks"; break;
			case sVectors:        p_s = "vectors"; break;
			case sImage:          p_s = "image"; break;
			case sRgbImage:       p_s = "rgbimage"; break;
			case sPm3d:           p_s = "pm3d"; break;
		}
		if(p_s) {
			rBuf.Space().Cat("with").Space().Cat(p_s);
			SString style_buf;
			if(Style.ToStr(style_buf) > 0)
				rBuf.Space().Cat(style_buf);
		}
	}
	return 1;
}
//
//
//
PPGpTicsList::PPGpTicsList(int type)
{
	assert(oneof2(type, 0, 1));
	T = type;
}

int PPGpTicsList::Add(double val, const char * pText, int level)
{
	int    ok = 1;
	assert(T == 0);
	Item * p_item = new Item;
	THROW_MEM(p_item);
	p_item->Val.R = val;
	p_item->Level = level;
	p_item->Text = pText;
	THROW_SL(List.insert(p_item));
	CATCHZOK
	return ok;
}

int PPGpTicsList::Add(const LDATETIME & rVal, const char * pText, int level)
{
	int    ok = 1;
	assert(T == 1);
	Item * p_item = new Item;
	THROW_MEM(p_item);
	p_item->Val.D = rVal;
	p_item->Level = level;
	p_item->Text = pText;
	THROW_SL(List.insert(p_item));
	CATCHZOK
	return ok;
}

int PPGpTicsList::ToStr(SString & rBuf) const
{
	int    ok = -1;
	if(List.getCount()) {
		rBuf.CatChar('(');
		for(uint i = 0; i < List.getCount(); i++) {
			const Item * p_item = List.at(i);
			if(p_item) {
				if(i > 0)
					rBuf.CatDiv(',', 2);
				if(p_item->Text.NotEmpty())
					rBuf.CatQStr(p_item->Text).Space();
				if(T == 1)
					if(p_item->Val.D.t)
						rBuf.Cat(p_item->Val.D.t, TIMF_HMS);
					else
						rBuf.Cat(p_item->Val.D.d, DATF_DMY);
				else
					rBuf.Cat(p_item->Val.R, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				if(p_item->Level)
					rBuf.Space().Cat(p_item->Level);
			}
		}
		rBuf.CatChar(')');
		ok = 1;
	}
	return ok;
}
//
//
//
Generator_GnuPlot::StyleFont::StyleFont()
{
	Size = 0;
}

Generator_GnuPlot::StyleTics::StyleTics()
{
	Flags = 0;
	Rotate = 0;
}
//
//
//
Generator_GnuPlot::PlotParam::PlotParam()
{
	Flags = 0;
}

Generator_GnuPlot::Generator_GnuPlot(const char * pFileName) : SFile()
{
	SString dir, file_name;
	PPGetPath(PPPATH_TEMP, dir);
	if(pFileName == 0) {
		MakeTempFileName(dir, "GP", "PLT", 0, file_name);
		pFileName = file_name;
	}
	Open(pFileName, SFile::mWrite);
	{
		MakeTempFileName(dir, "GP", "DAT", 0, file_name);
		DataFileName = file_name;
	}
}

const char * SLAPI Generator_GnuPlot::GetDataFileName() const
{
	return DataFileName.cptr();
}

int Generator_GnuPlot::Preamble()
{
	// set terminal window "font,9"
	Set().Cat("terminal").Space().Cat("windows");
	StyleFont font;
	PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, font.Face);
	font.Size = 9;
	LineBuf.Space();
	Font(font).CR();
	return PutLine();
}

int Generator_GnuPlot::AddPlotItem(const PPGpPlotItem & rItem)
{
	int    ok = 1;
	PPGpPlotItem * p_new_item = new PPGpPlotItem(0, 0);
	THROW_MEM(p_new_item);
	*p_new_item = rItem;
	THROW_SL(Items.insert(p_new_item));
	CATCHZOK
	return ok;
}

int Generator_GnuPlot::SetDateTimeFormat(int axis, int usingTime)
{
	/*
		set xdata time
		set timefmt "%d/%m/%Y"
	*/
	int    ok = 1;
	Set();
	AxisLetter(axis).Cat("data").Space().Cat("time").CR();
	THROW(PutLine());
	Set().Cat("timefmt");
	if(usingTime)
		LineBuf.Space().CatQStr("%d/%m/%Y %H:%M:%S");
	else
		LineBuf.Space().CatQStr("%d/%m/%Y");
	LineBuf.CR();
	THROW(PutLine());
	CATCHZOK
	return ok;
}

int Generator_GnuPlot::PutLine()
{
	return WriteLine(LineBuf) ? 1 : PPSetErrorSLib();
}

int Generator_GnuPlot::PutDataLine()
{
	return DataFile.WriteLine(LineBuf) ? 1 : PPSetErrorSLib();
}

int Generator_GnuPlot::Cmd(const char * pLine)
{
	(LineBuf = pLine).CR();
	return PutLine();
}

SString & Generator_GnuPlot::Set()
{
	return LineBuf.Z().Cat("set").Space();
}

SString & Generator_GnuPlot::SetTics(int axis)
{
	Set();
	return AxisLetter(axis).Cat("tics").Space();
}

SString & Generator_GnuPlot::SetStyle()
{
	return LineBuf.Z().Cat("set").Space().Cat("style").Space();
}

int Generator_GnuPlot::SetStyleFill(const char * pFillStyle)
{
	SetStyle().Cat("fill").Space().Cat(pFillStyle).CR();
	return PutLine();
}

SString & Generator_GnuPlot::Font(const StyleFont & rFont)
{
	LineBuf.Cat("font").Space().CatChar('\"').Cat(rFont.Face);
	if(rFont.Size > 0)
		LineBuf.CatDiv(',', 0).Cat(rFont.Size);
	return LineBuf.CatChar('\"');
}

// static
SString & Generator_GnuPlot::ColorToStr(COLORREF c, SString & rBuf)
{
	char temp[32];
	sprintf(temp, "#%02x%02x%02x", GetRValue(c), GetGValue(c), GetBValue(c));
	return rBuf.CatQStr(temp);
}

SString & Generator_GnuPlot::Color(COLORREF c)
{
	return ColorToStr(c, LineBuf);
}

int Generator_GnuPlot::UnsetTics(int axis)
{
	LineBuf.Z().Cat("unset").Space();
	AxisLetter(axis).Cat("tics").CR();
	return PutLine();
}

int Generator_GnuPlot::SetTics(int axis, const StyleTics * pStyle)
{
	//set xtics rotate by 90 font "arial,8"
	int    ok = -1;
	if(pStyle) {
		SetTics(axis);
		if(pStyle->Rotate)
			LineBuf.Cat("rotate").Space().Cat("by").Space().Cat(pStyle->Rotate).Space();
		if(pStyle->Font.Face.NotEmpty())
			Font(pStyle->Font);
		LineBuf.CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::SetTicsInc(int axis, double inc)
{
	int    ok = -1;
	if(inc != 0.0) {
		SetTics(axis).Cat(inc, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::SetTicsInc(int axis, double inc, double low, double upp)
{
	int    ok = -1;
	if(inc != 0.0) {
		SetTics(axis).Cat(low, MKSFMTD(0, 6, NMBF_NOTRAILZ)).
			Cat(inc, MKSFMTD(0, 6, NMBF_NOTRAILZ)).Cat(upp, MKSFMTD(0, 6, NMBF_NOTRAILZ)).CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::SetTicsInc(int axis, long incSec, LDATE low, LDATE upp)
{
	int    ok = -1;
	if(incSec != 0) {
		SetTics(axis);
		if(low)
			LineBuf.CatChar('\"').Cat(low, DATF_DMY|DATF_CENTURY).CatChar('\"').CatDiv(',', 2);
		LineBuf.Cat(incSec);
		if(low && upp)
			LineBuf.CatDiv(',', 2).CatChar('\"').Cat(upp, DATF_DMY|DATF_CENTURY).CatChar('\"').CatDiv(',', 2);
		LineBuf.CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::SetTicsList(int axis, const PPGpTicsList & rList)
{
	int    ok = -1;
	SetTics(axis).Space();
	if(rList.ToStr(LineBuf) > 0) {
		LineBuf.CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::AddTicsExplicit(int axis, double val, const char * pText, int level)
{
	int    ok = -1;
	SetTics(axis).Cat("add").Space().CatChar('(');
	if(pText)
		LineBuf.CatQStr(pText).Space();
	LineBuf.Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ));
	if(level)
		LineBuf.Space().Cat(level);
	LineBuf.CatChar(')').CR();
	return PutLine();
}

int Generator_GnuPlot::AddTicsExplicit(int axis, const LDATETIME & rVal, const char * pText, int level)
{
	int    ok = -1;
	if(rVal.d) {
		SetTics(axis).Cat("add").Space().CatChar('(');
		if(pText)
			LineBuf.CatQStr(pText).Space();
		LineBuf.Cat(rVal.d, DATF_DMY|DATF_CENTURY);
		if(level)
			LineBuf.Space().Cat(level);
		LineBuf.CatChar(')').CR();
		ok = PutLine();
	}
	return ok;
}

int Generator_GnuPlot::SetGrid()
{
	Set().Cat("grid").CR();
	return PutLine();
}

int Generator_GnuPlot::SetTitle(const char * pTitle)
{
	Set().Cat("title").Space().CatQStr(pTitle).CR();
	return PutLine();
}

SString & Generator_GnuPlot::AxisLetter(int axis)
{
	if(axis == axX)
		LineBuf.CatChar('x');
	else if(axis == axY)
		LineBuf.CatChar('y');
	else if(axis == axZ)
		LineBuf.CatChar('z');
	else if(axis == axX2)
		LineBuf.Cat("x2");
	else if(axis == axY2)
		LineBuf.Cat("y2");
	else if(axis == axZ2)
		LineBuf.Cat("z2");
	else
		LineBuf.CatChar('?');
	return LineBuf;
}

int Generator_GnuPlot::SetAxisTitle(int axis, const char * pTitle)
{
	if(oneof3(axis, axX, axY, axZ)) {
		Set();
		AxisLetter(axis).Cat("label").Space().CatQStr(pTitle).CR();
		return PutLine();
	}
	else
		return 0;
}

int Generator_GnuPlot::SetAxisRange(int axis, double lo, double hi)
{
	//set xrange [1948.702:1961.215]
	if(oneof3(axis, axX, axY, axZ)) {
		Set();
		AxisLetter(axis).Cat("range").Space().CatChar('[').
			Cat(lo, MKSFMTD(0, 3, NMBF_NOTRAILZ)).CatChar(':').Cat(hi, MKSFMTD(0, 3, NMBF_NOTRAILZ)).
			CatChar(']').CR();
		return PutLine();
	}
	else
		return 0;
}

int Generator_GnuPlot::DeclareLineStyle(long idx, const PPGpStyle * pStyle)
{
	/*
	set style line <index> default
	set style line <index> {{linetype | lt} <line_type> | <colorspec>}
	{{linecolor | lc} <colorspec>}
	{{linewidth | lw} <line_width>}
	{{pointtype | pt} <point_type>}
	{{pointsize | ps} <point_size>}
	{palette}
	*/
	int    ok = -1;
	if(pStyle == 0) {
		SetStyle().Cat("line").Space().Cat(idx).Space().Cat("default").CR();
		ok = 1;
	}
	else {
		SString style_buf;
		if(pStyle->ToStr(style_buf) > 0) {
			SetStyle().Cat("line").Space().Cat(idx).Space().Cat(style_buf).CR();
			ok = 1;
		}
	}
	return (ok > 0) ? PutLine() : ok;
}

int Generator_GnuPlot::StartData(int putLegend)
{
	int    ok = 1;
	LineBuf = 0;
	THROW_SL(DataFile.Open(DataFileName, SFile::mWrite));
	/*
	if(Param.Legend.getCount() && !(Param.Flags & PlotParam::fNoTitle)) {
		long   max_id = 0;
		Param.Legend.GetMaxID(&max_id);
		SString text_buf;
		LineBuf = 0;
		for(long i = 1; i < max_id; i++) {
			LineBuf.CatDiv(' ', 0, 1);
			if(Param.Legend.Get(i, text_buf) > 0) {
				LineBuf.CatQStr(text_buf);
			}
			else {
				LineBuf.CatQStr(0);
			}
		}
		PutEOR();
	}
	*/
	CATCHZOK
	return ok;
}

void FASTCALL Generator_GnuPlot::PutData(LDATE dt)
{
	LineBuf.CatDivIfNotEmpty(' ', 0).Cat(dt, DATF_DMY|DATF_CENTURY);
}

void FASTCALL Generator_GnuPlot::PutData(LTIME tm)
{
	LineBuf.CatDivIfNotEmpty(' ', 0).Cat(tm, TIMF_HMS);
}

void Generator_GnuPlot::PutData(LDATETIME dtm)
{
	LineBuf.CatDivIfNotEmpty(' ', 0).Cat(dtm, DATF_DMY|DATF_CENTURY, TIMF_HMS);
}

void Generator_GnuPlot::PutData(double val)
{
	LineBuf.CatDivIfNotEmpty(' ', 0).Cat(val, MKSFMTD(0, 12, NMBF_NOTRAILZ));
}

void FASTCALL Generator_GnuPlot::PutData(long val)
{
	LineBuf.CatDivIfNotEmpty(' ', 0).Cat(val);
}

void Generator_GnuPlot::PutData(const char * pStr, int withoutQuot)
{
	LineBuf.CatDivIfNotEmpty(' ', 0);
	if(withoutQuot)
		LineBuf.Cat(pStr);
	else
		LineBuf.CatQStr(pStr);
}

int Generator_GnuPlot::PutEOR()
{
	LineBuf.CR();
	int    ok = PutDataLine();
	LineBuf = 0;
	return ok;
}

int Generator_GnuPlot::PutEndOfData()
{
	/*
	LineBuf.Z().CatChar('e').CR();
	return PutDataLine();
	*/
	DataFile.Close();
	return 1;
}

int Generator_GnuPlot::Plot(const PlotParam * pParam)
{
	// plot '-' using 1:2 title "Фигня всякая" w lines
	LineBuf = 0;
	SString data_file_name, temp_buf;
	if(Items.getCount()) {
		if(pParam)
			Param = *pParam;
		if(Param.Flags & PlotParam::fStereo)
			LineBuf.CatChar('s');
		LineBuf.Cat("plot").Space();
		for(uint i = 0; i < Items.getCount(); i++) {
			const PPGpPlotItem * p_item = Items.at(i);
			if(i > 0)
				LineBuf.CatDiv(',', 2);
			p_item->ToStr(LineBuf);
		}
	}
	else if(pParam) {
		Param = *pParam;
		Param.Legend.SortByID();
		if(Param.Flags & PlotParam::fHistogram) {
			SetStyle().Cat("data").Space().Cat("histograms").CR();
			PutLine();
		}
		else {
			if(Param.Flags & PlotParam::fPm3D) {
				Set().Cat("pm3d").CR();
				PutLine();
			}
			if(Param.Flags & PlotParam::fDGrid3D) {
				Set().Cat("dgrid3d").CR();
				PutLine();
			}
		}
		LineBuf = 0;
		if(Param.Flags & PlotParam::fStereo)
			LineBuf.CatChar('s');
		LineBuf.Cat("plot").Space();
		if(DataFileName.NotEmpty())
			(data_file_name = DataFileName).ReplaceChar('\\', '/');
		else
			data_file_name = "-";
		if(Param.Legend.getCount()) {
			if(Param.Flags & PlotParam::fHistogram) {
				for(uint i = 0; i < Param.Legend.getCount(); i++) {
					if(i)
						LineBuf.CatDiv(',', 2);
					LineBuf.CatQStr((i == 0) ? data_file_name : "").Space(); // data source
					LineBuf.Cat("using").Space().Cat(Param.Legend.at(i).Id).CatChar(':').Cat("xtic(1)");
					temp_buf.Z().Cat(Param.Legend.at(i).Id);
					LineBuf.Space().Cat("title").Space().CatQStr(temp_buf); // title
				}
			}
			else {
				for(uint i = 0; i < Param.Legend.getCount(); i++) {
					if(i)
						LineBuf.CatDiv(',', 2);
					LineBuf.CatQStr((i == 0) ? data_file_name : "").Space(); // data source
					LineBuf.Cat("using").Space().Cat(1).CatChar(':').Cat(Param.Legend.at(i).Id);
					LineBuf.Space().Cat("title").Space().CatQStr(Param.Legend.at(i).Txt); // title
					if(Param.Flags & PlotParam::fLines)
						LineBuf.Space().Cat("with").Space().Cat("lines");
					else if(Param.Flags & PlotParam::fPoints)
						LineBuf.Space().Cat("with").Space().Cat("points");
					else if(Param.Flags & PlotParam::fDots)
						LineBuf.Space().Cat("with").Space().Cat("dots");
				}
			}
		}
		else
			LineBuf.CatQStr(data_file_name);
	}
	else
		LineBuf.Cat("plot").Space().CatChar('\'').CatChar('-').CatChar('\'');
	LineBuf.CR();
	return PutLine();
}

int Generator_GnuPlot::Run()
{
	int    ok = 1;
	SString file_name, q_file_name, cmd_line;
	file_name = GetName();
	(q_file_name = file_name).Quot('\"', '\"');
	LineBuf.Z().Cat("pause").Space().Cat(100000).CR();
	PutLine();
	Close();

	PPGetFilePath(PPPATH_BIN, "ppgplot.exe", cmd_line);
	spawnl(_P_NOWAIT, (const char *)cmd_line, (const char *)cmd_line, (const char *)q_file_name, 0);
//#ifndef _DEBUG
	SLS.RegisterTempFileName(file_name);
	SLS.RegisterTempFileName(DataFileName);
//#endif
	return ok;
}

#if 0 // @construction {
//
// ARG(distrib IN):
//   0 - no distribution
//   1 - gaussian (sigma)
//   2 - binomial (p, n)
//   3 - poisson (mu)
//   4 - gamma (a, b)
//   5 - gamma int (a)
//
int PlotRandomGeneration(int distrib, uint argCount, double * pArgList)
{
	int    ok = 1;
	SRandGenerator gen;
	if(distrib == 1 && argCount < 1)
		ok = 0;
	else if(distrib == 2 && argCount < 2)
		ok = 0;
	else if(distrib == 3 && argCount < 1)
		ok = 0;
	else if(distrib == 4 && argCount < 2)
		ok = 0;
	else if(distrib == 5 && argCount < 1)
		ok = 0;
	if(!oneof6(distrig, 0, 1, 2, 3, 4, 5))
		ok = 0;
	if(ok) {
		Generator_GnuPlot plot(0);
		Generator_GnuPlot::PlotParam param;
		plot.Preamble();
		plot.SetGrid();
		{
			int axis = Generator_GnuPlot::axX;
			plot.UnsetTics(axis);

			Generator_GnuPlot::StyleTics tics;
			tics.Rotate = 0;
			tics.Font.Size = 8;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
			plot.SetTics(axis, &tics);

			PPGpTicsList tics_list(0);
			tics_list.Add(total.PaymPeriodMean, "Средний период платежей");
			plot.SetTicsList(axis, tics_list);
		}
		{
			int axis = Generator_GnuPlot::axY;
			plot.UnsetTics(axis);

			Generator_GnuPlot::StyleTics tics;
			tics.Rotate = 0;
			tics.Font.Size = 8;
			PPGetSubStr(PPTXT_FONTFACE, PPFONT_ARIALCYR, tics.Font.Face);
			plot.SetTics(axis, &tics);

			plot.SetAxisRange(axis, -0.1, 1.1);

			PPGpTicsList tics_list(0);
			tics_list.Add(-0.1, 0);
			tics_list.Add(0.0, 0);
			tics_list.Add(0.5, 0);
			tics_list.Add(1.0, 0);
			tics_list.Add(1.1, 0);
			plot.SetTicsList(axis, tics_list);
		}
		{
			PPGpPlotItem item(plot.GetDataFileName(), 0, PPGpPlotItem::sPoints);
			item.Style.SetPoint(GetColorRef(SClrBlueviolet), PPGpStyle::ptCircle, 0.25);
			item.AddDataIndex(1);
			item.AddDataIndex(2);
			plot.AddPlotItem(item);
		}
		plot.Plot(&param);
		plot.StartData(1);
		SHistogram
		for(i = 0; i < 10000; i++) {
			double v
			RPoint3 p = matrix.at(i);
			plot.PutData(p.x);
			plot.PutData(p.y);
			plot.PutEOR();
		}
		plot.PutEndOfData();
		ok = plot.Run();
	}
	return ok;
}

#endif // } 0 @construction
