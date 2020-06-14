// SSVG.CPP
// Copyright (c) A.Sobolev 2010, 2012, 2016, 2017, 2018, 2019, 2020
// @codepage UTF-8
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

class SSvg {
public:
	SSvg();
	~SSvg();
	int    ParseFile(const char * pFileName, SDraw & rResult);
private:
	enum {
		tUnkn = 0,
		tNone,
		tId,
		tUrl,
		tSvg,
		tGroup,
		tStyle,
		tFill,
		tFillRule,
		tFillOpacity,
		tStroke,
		tStrokeDashArray,
		tStrokeDashOffset,
		tStrokeLineCap,
		tStrokeLineJoin,
		tStrokeMiterLimit,
		tStrokeOpacity,
		tStrokeWidth,
		tPath,
		tLine,
		tRect,
		tCircle,
		tEllipse,
		tDefs,
		tUse,
		tSymbol,
		tPolygon,
		tPolyline,
		tText,
		tImage,
		tLinearGradient,
		tRadialGradient,
		tStop,
		tStopColor,
		tStopOpacity,
		tOffset,
		tPattern,
		tTransform,
		tCurrentColor,
		tButt,
		tSquare,
		tRound,
		tMiter,
		tBevel,
		tX,
		tY,
		tX1,
		tY1,
		tX2,
		tY2,
		tRx,
		tRy,
		tCx,
		tCy,
		tFx,
		tFy,
		tR,
		tWidth,
		tHeight,
		tD,      // "d" - path description
		tPoints,
		tGradientUnits,
		tGradientTransform,
		tSpreadMethod,
		tHRef,
		tViewBox,
		tPreserveAspectRatio,
		tOpacity
	};

	struct StyleBlock {
		enum {
			fHasStroke = 0x0001,
			fHasFill   = 0x0002
		};
		StyleBlock();
		void   Init();
		SPaintObj::Pen Pen;
		SPaintObj::Brush Brush;
		SColor GradientColor;
		float  Opacity; // Общая (не)прозрачность элемента.
		long   Flags;
		int    IdPen;
		int    IdBrush;
	};

	class CommonFigAttr : public StyleBlock {
	public:
		CommonFigAttr();
		void   Init();

		SString Sid;
		LMatrix2D * P_Mtx;
		LMatrix2D Mtx;
	};

	int    InitTokens();
	int    FASTCALL GetToken(const char * pSymb) const;
	SString & FASTCALL MakePaintObjSymb(const char * pOrgSymb);
	int    _GetUSize(const char * pTxt, int dir, float & rR);
	int    _GetGradientCoord(const char * pStr, float & rF, int & rPct);
	int    _GetPoints(SStrScan & rScan, const char * pTxt, FloatArray & rList);
	int    _GetCommonFigAttrAndInsert(const StrAssocArray & rAttrList, CommonFigAttr & rA, SDrawFigure * pFig, SDrawGroup * pParent);
	int    GetAttr(const xmlNode * pNode, const char * pAttr, SString & rVal);
	int    GetAttrList(xmlNode * pNode, StrAssocArray & rList);
	int    GetColor(const SString & rProp, SColor & rC) const;

	enum {
		psirOK          = 0x00000001,
		psirUnknOccured = 0x10000000,
		psirFillOccured = 0x00010000
	};

	int    ProcessStyleItem(int token, const SString & rVal, StyleBlock & rBlk);
	int    ParseStyle(xmlNode * pNode, SDraw & rDraw, StyleBlock & rBlk);
	int    ParsePrimitivs(xmlNode * pParentNode, SDrawGroup & rGroup, SStrScan & rTempScan);
	int    GetViewPortAttr(const StrAssocArray & rAttrList, SViewPort & rVp);

	SColor CurColor;
	SymbHashTable TTab;
	SStrScan Scan;
	long  LastOwnSymbNo;
	SString SymbPrefix;
	SString TempBuf;  // @allocreuse
	SDraw * P_Result; // @notowned
	long   ReScpNumber; // Идентификатор регулярного выражения, представляющего число без обязательной ведущей цифры (допустимы точка, + или -)
};

SSvg::StyleBlock::StyleBlock()
{
	Init();
}

void SSvg::StyleBlock::Init()
{
	Pen.SetSimple(SColor(SClrBlack));
	Pen.S = SPaintObj::psNull;
	Brush.SetSimple(SColor(SClrWhite));
	Brush.S = SPaintObj::bsNull;
	GradientColor = SColor();
	IdPen = 0;
	IdBrush = 0;
	Flags = 0;
	Opacity = 1.0f;
}

SSvg::CommonFigAttr::CommonFigAttr() : SSvg::StyleBlock(), P_Mtx(0)
{
}

void SSvg::CommonFigAttr::Init()
{
	StyleBlock::Init();
	Sid = 0;
	P_Mtx = 0;
	Mtx.InitUnit();
}

SSvg::SSvg() : TTab(1024, 0), P_Result(0), LastOwnSymbNo(0), ReScpNumber(0)
{
	InitTokens();
}

SSvg::~SSvg()
{
}

int SSvg::GetAttr(const xmlNode * pNode, const char * pAttr, SString & rVal)
{
	int    ok = 0;
	xmlChar * p_val = xmlGetProp(pNode, (const xmlChar *)pAttr);
	if(p_val) {
		rVal.Set(p_val);
		SAlloc::F(p_val);
		ok = 1;
	}
	else
		rVal.Z();
	return ok;
}

int SSvg::GetAttrList(xmlNode * pNode, StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
	if(pNode && pNode->type == XML_ELEMENT_NODE) {
		xmlAttr * p_prop = pNode->properties;
		for(xmlAttr * p_prop = pNode->properties; p_prop; p_prop = p_prop->next) {
			int    token = GetToken(reinterpret_cast<const char *>(p_prop->name));
			if(token) {
				if(p_prop->type == XML_ATTRIBUTE_NODE) {
					if(p_prop->children) {
						if(!p_prop->children->next && oneof2(p_prop->children->type, XML_TEXT_NODE, XML_CDATA_SECTION_NODE)) {
							rList.Add(token, reinterpret_cast<const char *>(p_prop->children->content));
						}
						else {
							xmlChar * p_ret = xmlNodeListGetString(p_prop->doc, p_prop->children, 1);
							if(p_ret) {
								rList.Add(token, reinterpret_cast<const char *>(p_ret));
								SAlloc::F(p_ret);
							}
						}
					}
				}
				else if(p_prop->type == XML_ATTRIBUTE_DECL) {
					rList.Add(token, reinterpret_cast<const char *>(reinterpret_cast<xmlAttribute *>(p_prop)->defaultValue));
				}
			}
		}
	}
	return ok;
}

int SSvg::GetColor(const SString & rProp, SColor & rC) const
{
	int    ok = 1;
	switch(GetToken(rProp)) {
		case tNone: ok = -1; break;
		case tCurrentColor: ok = 2; break;
		case tUrl: ok = -100; break;
		case tUnkn:
			if(!rC.FromStr(rProp))
				ok = 0;
			break;
		default: ok = 0; break;
	}
	return ok;
}

int SSvg::InitTokens()
{
	int    ok = 1;
	TTab.Add("none",  tNone);
	TTab.Add("id",    tId);
	TTab.Add("url",   tUrl);
	TTab.Add("svg",   tSvg);
	TTab.Add("g",     tGroup);
	TTab.Add("style", tStyle);
	TTab.Add("fill",  tFill);
	TTab.Add("fill-rule",    tFillRule);
	TTab.Add("fill-opacity", tFillOpacity);
	TTab.Add("stroke", tStroke);
	TTab.Add("stroke-dasharray",  tStrokeDashArray);
	TTab.Add("stroke-dashoffset", tStrokeDashOffset);
	TTab.Add("stroke-linecap",    tStrokeLineCap);
	TTab.Add("stroke-linejoin",   tStrokeLineJoin);
	TTab.Add("stroke-miterlimit", tStrokeMiterLimit);
	TTab.Add("stroke-opacity",    tStrokeOpacity);
	TTab.Add("stroke-width",      tStrokeWidth);
	TTab.Add("path",     tPath);
	TTab.Add("line",     tLine);
	TTab.Add("rect",     tRect);
	TTab.Add("circle",   tCircle);
	TTab.Add("ellipse",  tEllipse);
	TTab.Add("defs",     tDefs);
	TTab.Add("use",      tUse);
	TTab.Add("symbol",   tSymbol);
	TTab.Add("polygon",   tPolygon);
	TTab.Add("polyline", tPolyline);
	TTab.Add("text",     tText);
	TTab.Add("image",    tImage);
	TTab.Add("linearGradient", tLinearGradient);
	TTab.Add("radialGradient", tRadialGradient);
	TTab.Add("stop",      tStop);
	TTab.Add("stop-color",   tStopColor);
	TTab.Add("stop-opacity", tStopOpacity);
	TTab.Add("offset",       tOffset);
	TTab.Add("pattern",   tPattern);
	TTab.Add("transform", tTransform);
	TTab.Add("currentColor", tCurrentColor);
	TTab.Add("butt",   tButt);
	TTab.Add("round",  tRound);
	TTab.Add("square", tSquare);
	TTab.Add("miter",  tMiter);
	TTab.Add("bevel",  tBevel);
	TTab.Add("x",  tX);
	TTab.Add("y",  tY);
	TTab.Add("x1", tX1);
	TTab.Add("y1", tY1);
	TTab.Add("x2", tX2);
	TTab.Add("y2", tY2);
	TTab.Add("rx", tRx);
	TTab.Add("ry", tRy);
	TTab.Add("cx", tCx);
	TTab.Add("cy", tCy);
	TTab.Add("fx", tFx);
	TTab.Add("fy", tFy);
	TTab.Add("r",  tR);
	TTab.Add("width",  tWidth);
	TTab.Add("height", tHeight);
	TTab.Add("d",      tD);
	TTab.Add("points", tPoints);
	TTab.Add("gradientUnits",     tGradientUnits);
	TTab.Add("gradientTransform", tGradientTransform);
	TTab.Add("spreadMethod",      tSpreadMethod);
	TTab.Add("href",              tHRef);
	TTab.Add("viewBox", tViewBox);
	TTab.Add("preserveAspectRatio", tPreserveAspectRatio);
	TTab.Add("opacity", tOpacity);
	return ok;
}

int FASTCALL SSvg::GetToken(const char * pSymb) const
{
	uint   val = 0;
	return TTab.Search(pSymb, &val, 0) ? (int)val : 0;
}

SString & FASTCALL SSvg::MakePaintObjSymb(const char * pOrgSymb)
{
	(TempBuf = SymbPrefix);
	if(isempty(pOrgSymb))
		TempBuf.CatLongZ(++LastOwnSymbNo, 6);
	else
		TempBuf.Cat(pOrgSymb);
	return TempBuf;
}

int SSvg::GetViewPortAttr(const StrAssocArray & rAttrList, SViewPort & rVp)
{
	int    ok = 1;
	SString temp_buf;
	if(rAttrList.GetText(tViewBox, temp_buf)) {
		THROW(rVp.FromStr(temp_buf, SViewPort::fmtSVG));
	}
	if(rAttrList.GetText(tPreserveAspectRatio, temp_buf)) {
		THROW(rVp.FromStr(temp_buf, SViewPort::fmtSVG));
	}
	CATCHZOK
	return ok;
}

int SSvg::ProcessStyleItem(int token, const SString & rVal, StyleBlock & rBlk)
{
	int    ok = 1;
	int    r;
	SString temp_buf;
	switch(token) {
		case tStyle:
			{
				StringSet ss(';', rVal);
				SString key, local_val;
				for(uint i = 0; ss.get(&i, temp_buf);) {
					if(temp_buf.Strip().Divide(':', key, local_val) > 0) {
						int r = ProcessStyleItem(GetToken(key), local_val.Strip(), rBlk); // @recursion
						ok |= r;
					}
				}
			}
			break;
		case tOpacity:   rBlk.Opacity = rVal.ToFloat(); break;
		case tStopColor: GetColor(rVal, rBlk.GradientColor); break;
		case tStopOpacity: rBlk.GradientColor.Alpha = static_cast<uint8>(rVal.ToFloat() * rBlk.Opacity * 255.0f); break;
		case tFill:
			if(rVal != "none") {
				if(rVal.HasPrefix("url")) {
					temp_buf = rVal;
					temp_buf.ShiftLeft(3);
					temp_buf.ShiftLeftChr('(');
					temp_buf.ShiftLeftChr('\"');
					temp_buf.ShiftLeftChr('#');
					size_t par_pos = 0;
					if(temp_buf.SearchChar(')', &par_pos))
						temp_buf.Trim(par_pos);
					temp_buf.TrimRightChr('\"');
					if(P_Result) {
						SPaintToolBox * p_tb = P_Result->GetToolBox();
						if(p_tb) {
							SPaintObj * p_obj = p_tb->GetObjBySymb(MakePaintObjSymb(temp_buf), 0);
							if(p_obj && p_obj->GetType() == SPaintObj::tGradient) {
								rBlk.Brush.S &= ~SPaintObj::bsNull;
								rBlk.Brush.IdPattern = p_obj->GetId();
							}
						}
					}
				}
				else {
					rBlk.Brush.S &= ~SPaintObj::bsNull;
					r = GetColor(rVal, rBlk.Brush.C);
					if(r == 2)
						rBlk.Brush.C = CurColor;
					else if(r < -1)
						rBlk.Brush.S |= SPaintObj::bsNull;
				}
			}
			else {
				rBlk.Brush.S |= SPaintObj::bsNull;
			}
			rBlk.Flags |= StyleBlock::fHasFill;
			ok |= psirFillOccured;
			break;
		case tFillRule:
			if(rVal == "nonzero")
				rBlk.Brush.Rule = SPaintObj::frNonZero;
			else if(rVal == "evenodd")
				rBlk.Brush.Rule = SPaintObj::frEvenOdd;
			break;
		case tFillOpacity:
			rBlk.Brush.C.Alpha = static_cast<uint8>(rVal.ToFloat() * rBlk.Opacity * 255.0f);
			// @todo inherit
			break;
		case tStroke:
			if(rVal != "none") {
				r = GetColor(rVal, rBlk.Pen.C);
				if(r == 2)
					rBlk.Pen.C = CurColor;
				else if(r < -1)
					rBlk.Pen.S |= SPaintObj::psNull;
				rBlk.Pen.S = SPaintObj::psSolid;
			}
			else
				rBlk.Pen.S = SPaintObj::psNull;
			rBlk.Flags |= StyleBlock::fHasStroke;
			break;
		case tStrokeDashArray:
			if(GetToken(rVal) == tNone) {
				ZDELETE(rBlk.Pen.P_DashRule);
				rBlk.Pen.S = SPaintObj::psSolid;
			}
			else {
				Scan.Set(rVal, 0);
				Scan.Skip();
				while(Scan.Skip().GetDotPrefixedNumber(temp_buf)) {
					rBlk.Pen.AddDashItem(temp_buf.ToFloat());
					Scan.Skip().IncrChr(',');
				}
				rBlk.Pen.S = SPaintObj::psDash;
			}
			break;
		case tStrokeDashOffset:
			rBlk.Pen.DashOffs = rVal.ToFloat();
			break;
		case tStrokeLineCap:
			switch(GetToken(rVal)) {
				case tButt:   rBlk.Pen.LineCap = SPaintObj::lcButt; break;
				case tRound:  rBlk.Pen.LineCap = SPaintObj::lcRound; break;
				case tSquare: rBlk.Pen.LineCap = SPaintObj::lcSquare; break;
			}
			break;
		case tStrokeLineJoin:
			switch(GetToken(rVal)) {
				case tMiter: rBlk.Pen.LineCap = SPaintObj::ljMiter; break;
				case tRound: rBlk.Pen.LineCap = SPaintObj::ljRound; break;
				case tBevel: rBlk.Pen.LineCap = SPaintObj::ljBevel; break;
			}
			break;
		case tStrokeMiterLimit:
			rBlk.Pen.MiterLimit = rVal.ToFloat();
			break;
		case tStrokeOpacity:
			rBlk.Pen.C.Alpha = (uint8)(rVal.ToFloat() * rBlk.Opacity * 255.0f);
			break;
		case tStrokeWidth:
			_GetUSize(rVal, DIREC_UNKN, rBlk.Pen.W__);
			break;
		default:
			ok |= psirUnknOccured;
			break;
	}
	return ok;
}

int SSvg::ParseStyle(xmlNode * pNode, SDraw & rDraw, StyleBlock & rBlk)
{
	int    ok = 1;
	SString val;
	StrAssocArray attr_list;
	GetAttrList(pNode, attr_list);
	for(uint i = 0; i < attr_list.getCount(); i++) {
		StrAssocArray::Item item = attr_list.Get(i);
		ProcessStyleItem(item.Id, val = item.Txt, rBlk);
	}
	return ok;
}

int SSvg::_GetUSize(const char * pTxt, int dir, float & rR)
{
	USize usz;
	usz.Dir = dir;
	if(usz.FromStr(pTxt, USize::fmtSVG)) {
		double sz;
		P_Result->ConvertCoord(usz, &sz);
		rR = static_cast<float>(sz);
		return 1;
	}
	else {
		rR = 0.0f;
		return 0;
	}
}

int SSvg::_GetPoints(SStrScan & rScan, const char * pTxt, FloatArray & rList)
{
	int    ok = 1;
	rList.clear();
	rScan.Set(pTxt, 0);
	SString temp_buf;
	rScan.Skip();
	while(rScan.GetDotPrefixedNumber(temp_buf)) {
		// @v10.4.10 float val = temp_buf.ToFloat();
		float val = static_cast<float>(satof(temp_buf)); // @v10.4.10 // @v10.7.9 atof-->satof
		float temp_val = temp_buf.ToFloat();
		assert(val == temp_val);
		rList.add(val);
		rScan.Skip().IncrChr(',');
		rScan.Skip();
	}
	if(rScan[0] != 0)
		ok = 0; // @error
	else if(rList.getCount() % 2)
		ok = 0; // @error
	return ok;
}

int SSvg::_GetCommonFigAttrAndInsert(const StrAssocArray & rAttrList, CommonFigAttr & rA, SDrawFigure * pFig, SDrawGroup * pParent)
{
	int    ok = 1;
	int    fill_occured = 0;
	int    style_ret_flags = 0;
	SString temp_buf;
	rA.Init();
	for(uint i = 0; i < rAttrList.getCount(); i++) {
		StrAssocArray::Item item = rAttrList.Get(i);
		if(item.Id == tId) {
			rA.Sid = item.Txt;
		}
		else if(item.Id == tTransform) {
			if(rA.Mtx.FromStr(item.Txt, LMatrix2D::fmtSVG))
				rA.P_Mtx = &rA.Mtx;
		}
		else {
			int   r = ProcessStyleItem(item.Id, (temp_buf = item.Txt), rA);
			style_ret_flags |= r;
		}
	}
	if(pFig) {
		pFig->SetSid(rA.Sid);
		pFig->SetTransform(rA.P_Mtx);
		{
			long   style_flags = 0;
			SPaintToolBox * p_tb = 0;
			THROW(P_Result);
			THROW(p_tb = P_Result->GetToolBox());
			int color_id = 0;
			if(rA.Flags & StyleBlock::fHasStroke) {
				if(rA.Pen.S == SPaintObj::psNull)
					rA.IdPen = 0;
				else if(rA.Pen.IsSimple() && (color_id = p_tb->SearchColor(rA.Pen.C)) != 0) {
					rA.IdPen = color_id;
				}
				else {
					SPaintObj * p_new_obj = 0;
					THROW(rA.IdPen = p_tb->CreateDynIdent(MakePaintObjSymb(0)));
					THROW(p_new_obj = p_tb->CreateObj(rA.IdPen));
					THROW(p_new_obj->CreatePen(&rA.Pen));
				}
			}
			else if(pParent) {
				rA.IdPen = pParent->GetPen();
				//SETIFZ(rA.IdPen, -1);
			}
			else {
				//rA.IdPen = -1;
			}
			//
			//
			//
			if(rA.Flags & StyleBlock::fHasFill) {
				if(rA.Brush.S == SPaintObj::bsNull) {
					rA.IdBrush = 0;
					style_flags |= SDrawFigure::fNullBrush; // @v9.7.10
				}
				else if(rA.Brush.IsSimple() && (color_id = p_tb->SearchColor(rA.Brush.C)) != 0) {
					rA.IdBrush = color_id;
				}
				else {
					SPaintObj * p_new_obj = 0;
					THROW(rA.IdBrush = p_tb->CreateDynIdent(MakePaintObjSymb(0)));
					THROW(p_new_obj = p_tb->CreateObj(rA.IdBrush));
					THROW(p_new_obj->CreateBrush(&rA.Brush));
				}
			}
			else if(pParent) {
				rA.IdBrush = pParent->GetBrush();
				// @v9.7.10 {
				if(pParent->GetFlags() & SDrawFigure::fNullBrush) {
					rA.IdBrush = 0;
					style_flags |= SDrawFigure::fNullBrush; 
				}
				else { // } @v9.7.10 
					if(!rA.IdBrush) {
						color_id = p_tb->CreateColor(0, SColor(SClrBlack)); // @v9.1.9 SClrBlack-->SColor(SClrBlack)
						if(color_id != 0)
							rA.IdBrush = color_id;
					}
				}
			}
			CALLPTRMEMB(pFig, SetStyle(rA.IdPen, rA.IdBrush, style_flags));
		}
	}
	CALLPTRMEMB(pParent, Add(pFig));
	CATCHZOK
	return ok;
}

int SSvg::ParsePrimitivs(xmlNode * pParentNode, SDrawGroup & rGroup, SStrScan & rTempScan)
{
	int    ok = 1;
	StrAssocArray attr_list;
	SString temp_buf;
	FloatArray points;
	CommonFigAttr cfa;
	StringSet ss_style;
	SPaintToolBox * p_tb = P_Result ? P_Result->GetToolBox() : 0;
	for(xmlNode * p_node = pParentNode->children; p_node != 0; p_node = p_node->next) {
		int    token = (p_node->type == XML_ELEMENT_NODE) ? GetToken(reinterpret_cast<const char *>(p_node->name)) : 0;
		uint32 coord_ready = 0;
		switch(token) {
			case tSymbol: // @v10.4.5
				{
					SDrawGroup * p_group = new SDrawGroup(0, SDrawGroup::dgtSymbol);
					THROW_S(p_group, SLERR_NOMEM);
					GetAttrList(p_node, attr_list);
					THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_group, &rGroup));
					ParsePrimitivs(p_node, *p_group, rTempScan); // @recursion
				}
				break;
			case tGroup:
				{
					SDrawGroup * p_group = new SDrawGroup;
					THROW_S(p_group, SLERR_NOMEM);
					GetAttrList(p_node, attr_list);
					THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_group, &rGroup));
					ParsePrimitivs(p_node, *p_group, rTempScan); // @recursion
				}
				break;
			case tPath:
				{
					SDrawPath * p_path = new SDrawPath;
					THROW_S(p_path, SLERR_NOMEM);
					GetAttrList(p_node, attr_list);
					if(attr_list.GetText(tD, temp_buf) > 0)
						p_path->FromStr(temp_buf, SDrawPath::fmtSVG);
					THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_path, &rGroup));
				}
				break;
			case tLine:
				{
					FShape::Line line;
					GetAttrList(p_node, attr_list);
					for(uint i = 0; i < attr_list.getCount(); i++) {
						StrAssocArray::Item item = attr_list.Get(i);
						if(item.Id == tX1) {
							if(_GetUSize(item.Txt, DIREC_HORZ, line.A.X))
								coord_ready |= 0x0001;
						}
						else if(item.Id == tY1) {
							if(_GetUSize(item.Txt, DIREC_VERT, line.A.Y))
								coord_ready |= 0x0002;
						}
						else if(item.Id == tX2) {
							if(_GetUSize(item.Txt, DIREC_HORZ, line.B.X))
								coord_ready |= 0x0004;
						}
						else if(item.Id == tY2) {
							if(_GetUSize(item.Txt, DIREC_VERT, line.B.Y))
								coord_ready |= 0x0008;
						}
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = line;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tRect:
				{
					int    rounded = 0;
					FShape::Rect rect;
					FShape::RoundedRect rrect;
					float height = 0.0f;
					float width = 0.0f;
					FPoint vr;
					GetAttrList(p_node, attr_list);
					for(uint i = 0; i < attr_list.getCount(); i++) {
						StrAssocArray::Item item = attr_list.Get(i);
						switch(item.Id) {
							case tX:
								if(_GetUSize(item.Txt, DIREC_HORZ, rect.a.X))
									coord_ready |= 0x0001;
								break;
							case tY:
								if(_GetUSize(item.Txt, DIREC_VERT, rect.a.Y))
									coord_ready |= 0x0002;
								break;
							case tWidth:
								if(_GetUSize(item.Txt, DIREC_HORZ, width))
									coord_ready |= 0x0004;
								break;
							case tHeight:
								if(_GetUSize(item.Txt, DIREC_VERT, height))
									coord_ready |= 0x0008;
								break;
							case tRx:
								if(_GetUSize(item.Txt, DIREC_HORZ, vr.X))
									coord_ready |= 0x0010;
								break;
							case tRy:
								if(_GetUSize(item.Txt, DIREC_VERT, vr.Y))
									coord_ready |= 0x0020;
								break;
						}
					}
					rect.b.Set(rect.a.X + width, rect.a.Y + height);
					if(coord_ready & (0x0010|0x0020) && (vr.X || vr.Y)) {
						rounded = 1;
						rrect.a = rect.a;
						rrect.b = rect.b;
						if(!(coord_ready & 0x0020))
							vr.Y = vr.X;
						if(!(coord_ready & 0x0010))
							vr.X = vr.Y;
						rrect.R = vr;
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = rounded ? rrect : rect;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tCircle:
				{
					FShape::Circle circle;
					GetAttrList(p_node, attr_list);
					for(uint i = 0; i < attr_list.getCount(); i++) {
						StrAssocArray::Item item = attr_list.Get(i);
						if(item.Id == tCx) {
							if(_GetUSize(item.Txt, DIREC_HORZ, circle.C.X))
								coord_ready |= 0x0001;
						}
						else if(item.Id == tCy) {
							if(_GetUSize(item.Txt, DIREC_VERT, circle.C.Y))
								coord_ready |= 0x0002;
						}
						else if(item.Id == tR) {
							if(_GetUSize(item.Txt, DIREC_UNKN, circle.R))
								coord_ready |= 0x0004;
						}
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = circle;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tEllipse:
				{
					FShape::Ellipse ellipse;
					GetAttrList(p_node, attr_list);
					for(uint i = 0; i < attr_list.getCount(); i++) {
						StrAssocArray::Item item = attr_list.Get(i);
						if(item.Id == tCx) {
							if(_GetUSize(item.Txt, DIREC_HORZ, ellipse.C.X))
								coord_ready |= 0x0001;
						}
						else if(item.Id == tCy) {
							if(_GetUSize(item.Txt, DIREC_VERT, ellipse.C.Y))
								coord_ready |= 0x0002;
						}
						else if(item.Id == tRx) {
							if(_GetUSize(item.Txt, DIREC_HORZ, ellipse.R.X))
								coord_ready |= 0x0004;
						}
						else if(item.Id == tRy) {
							if(_GetUSize(item.Txt, DIREC_VERT, ellipse.R.Y))
								coord_ready |= 0x0008;
						}
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = ellipse;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tPolygon:
				{
					FShape::Polygon polygon;
					GetAttrList(p_node, attr_list);
					if(attr_list.GetText(tPoints, temp_buf) > 0) {
						if(_GetPoints(rTempScan, temp_buf, polygon))
							coord_ready |= 0x0001;
						else {
							; // @error
						}
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = polygon;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tPolyline:
				{
					FShape::Polyline polyline;
					GetAttrList(p_node, attr_list);
					if(attr_list.GetText(tPoints, temp_buf) > 0) {
						if(_GetPoints(rTempScan, temp_buf, polyline))
							coord_ready |= 0x0001;
						else {
							; // @error
						}
					}
					{
						SDrawShape * p_shape = new SDrawShape;
						p_shape->S = polyline;
						THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_shape, &rGroup));
					}
				}
				break;
			case tDefs:
				ParsePrimitivs(p_node, rGroup, rTempScan); // @recursion
				break;
			case tUse: // @v10.4.5
				{
					SDrawRef * p_ref = new SDrawRef;
					FPoint sz;
					GetAttrList(p_node, attr_list);
					if(attr_list.GetText(tHRef, temp_buf)) {
						p_ref->Ref = temp_buf.Strip();
					}
					else if(attr_list.GetText(tX, temp_buf)) {
						p_ref->Origin.X = temp_buf.ToFloat();
					}
					else if(attr_list.GetText(tY, temp_buf)) {
						p_ref->Origin.Y = temp_buf.ToFloat();
					}
					/*else if(attr_list.GetText(tWidth, temp_buf)) {
						sz.X = temp_buf.ToFloat();
					}
					else if(attr_list.GetText(tHeight, temp_buf)) {
						sz.Y = temp_buf.ToFloat();
					}*/
					p_ref->SetSize(sz);
					THROW(_GetCommonFigAttrAndInsert(attr_list, cfa, p_ref, &rGroup));
				}
				break;
			case tText:
				break;
			case tImage:
				break;
			case tLinearGradient:
			case tRadialGradient:
				{
					SString sid;
					int    units = 0;
					const  SPaintObj::Gradient * p_prototype = 0;
					GetAttrList(p_node, attr_list);
					if(attr_list.GetText(tGradientUnits, temp_buf)) {
						if(temp_buf == "userSpaceOnUse")
							units = SPaintObj::Gradient::uUserSpace;
						else if(temp_buf == "objectBoundingBox")
							units = SPaintObj::Gradient::uBB;
						else {
							; // @error
						}
					}
					SPaintObj::Gradient gradient(((token == tLinearGradient) ?
						SPaintObj::Gradient::kLinear : SPaintObj::Gradient::kRadial), units);
					if(attr_list.GetText(tId, temp_buf))
						sid = temp_buf;
					if(attr_list.GetText(tHRef, temp_buf)) {
						if(p_tb) {
							temp_buf.ShiftLeftChr('#');
							SPaintObj * p_obj = p_tb->GetObjBySymb(MakePaintObjSymb(temp_buf), SPaintObj::tGradient);
							if(p_obj) {
								p_prototype = p_obj->GetGradient();
								if(p_prototype) {
									gradient.SetPrototype(*p_prototype);
								}
								else {
									; // @error
								}
							}
							else {
								; // @error
							}
						}
					}
					if(attr_list.GetText(tSpreadMethod, temp_buf)) {
						if(temp_buf == "pad")
							gradient.Spread = gradient.sPad;
						else if(temp_buf == "reflect")
							gradient.Spread = gradient.sReflect;
						else if(temp_buf == "repeat")
							gradient.Spread = gradient.sRepeat;
						else {
							; // @error
						}
					}
					if(attr_list.GetText(tGradientTransform, temp_buf)) {
						if(!gradient.Tf.FromStr(temp_buf, LMatrix2D::fmtSVG)) {
							gradient.Tf.InitUnit();
							// @error
						}
					}
					{
						float f = 0.0f;
						int   pct = 0;
						if(token == tLinearGradient) {
							if(attr_list.GetText(tX1, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetLinearCoord(SPaintObj::Gradient::lcX1, f, pct);
							}
							else if(!p_prototype)
								gradient.SetLinearCoord(SPaintObj::Gradient::lcX1, 0.0f, 1);

							if(attr_list.GetText(tY1, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetLinearCoord(SPaintObj::Gradient::lcY1, f, pct);
							}
							else if(!p_prototype)
								gradient.SetLinearCoord(SPaintObj::Gradient::lcY1, 0.0f, 1);
							if(attr_list.GetText(tX2, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetLinearCoord(SPaintObj::Gradient::lcX2, f, pct);
							}
							else if(!p_prototype)
								gradient.SetLinearCoord(SPaintObj::Gradient::lcX2, 100.0f, 1);
							if(attr_list.GetText(tY2, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetLinearCoord(SPaintObj::Gradient::lcY2, f, pct);
							}
							else if(!p_prototype)
								gradient.SetLinearCoord(SPaintObj::Gradient::lcY2, 0.0f, 1);
						}
						else if(token == tRadialGradient) {
							if(attr_list.GetText(tCx, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcCX, f, pct);
							}
							else if(!p_prototype)
								gradient.SetRadialCoord(SPaintObj::Gradient::rcCX, 50.0f, 1);
							if(attr_list.GetText(tCy, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcCY, f, pct);
							}
							else if(!p_prototype)
								gradient.SetRadialCoord(SPaintObj::Gradient::rcCY, 50.0f, 1);
							if(attr_list.GetText(tR, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcR, f, pct);
							}
							else if(!p_prototype)
								gradient.SetRadialCoord(SPaintObj::Gradient::rcR, 50.0f, 1);
							if(attr_list.GetText(tFx, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcFX, f, pct);
							}
							else if(!p_prototype) {
								f = gradient.GetRadialCoord(SPaintObj::Gradient::rcCX, &pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcFX, f, pct);
							}
							if(attr_list.GetText(tFy, temp_buf)) {
								_GetGradientCoord(temp_buf, f, pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcFY, f, pct);
							}
							else if(!p_prototype) {
								f = gradient.GetRadialCoord(SPaintObj::Gradient::rcCY, &pct);
								gradient.SetRadialCoord(SPaintObj::Gradient::rcFY, f, pct);
							}
						}
					}
					//
					// Stops
					//
					for(xmlNode * p_gr_node = p_node->children; p_gr_node != 0; p_gr_node = p_gr_node->next) {
						int    gr_token = GetToken(reinterpret_cast<const char *>(p_gr_node->name));
						if(gr_token == tStop) {
							GetAttrList(p_gr_node, attr_list);
							float offs = 0.0f;
							StyleBlock sb;
							for(uint i = 0; i < attr_list.getCount(); i++) {
								StrAssocArray::Item item = attr_list.Get(i);
								ProcessStyleItem(item.Id, temp_buf = item.Txt, sb);
							}
							if(attr_list.GetText(tOffset, temp_buf)) {
								USize usz;
								usz.FromStr(temp_buf);
								if(usz.Unit == UNIT_PERCENT)
									offs = static_cast<float>(usz / 100.0);
								else
									offs = static_cast<float>(usz);
							}
							gradient.AddStop(offs, sb.GradientColor);
						}
					}
					//
					// Сохраняем градиент
					//
					if(p_tb) {
						int ident = p_tb->CreateDynIdent(MakePaintObjSymb(sid.Strip()));
						if(ident) {
							SPaintObj * p_obj = p_tb->CreateObj(ident);
							CALLPTRMEMB(p_obj, CreateGradient(&gradient));
						}
					}
				}
				break;
			case tPattern:
				break;
		}
	}
	CATCHZOK
	return ok;
}

int SSvg::_GetGradientCoord(const char * pStr, float & rF, int & rPct)
{
	rF = 0.0f;
	rPct = 0;
	USize usz;
	usz.FromStr(pStr, USize::fmtSVG);
	if(usz.Unit == UNIT_PERCENT) {
		rPct = 1;
		rF = static_cast<float>(usz);
	}
	else {
		rPct = 0;
		double df = 0.0;
		P_Result->ConvertCoord(usz, &df);
		rF = static_cast<float>(df);
	}
	return 1;
}

int SSvg::ParseFile(const char * pFileName, SDraw & rResult)
{
	int    ok = -1;
	SString temp_buf;
	xmlDoc * p_doc = 0;
	THROW(fileExists(pFileName));
	{
		uint32 crc = 0;
		SFile f(pFileName, SFile::mRead);
		THROW(f.IsValid());
		f.CalcCRC(0, &crc);
		SymbPrefix.Z().Cat(crc).CatChar('-');
	}
	{
		int    options = XML_PARSE_NOENT;
		SDrawGroup * p_cur_group = &rResult;
		StrAssocArray attr_list;
		SStrScan temp_scan;
		THROW(p_doc = xmlReadFile(pFileName, NULL, options));
		xmlNode * p_node = 0;
		xmlNode * p_root = xmlDocGetRootElement(p_doc);
		THROW(p_root);
		P_Result = &rResult;
		for(p_node = p_root; p_node; p_node = p_node->next) {
			if(sstreqi_ascii(reinterpret_cast<const char *>(p_node->name), "svg")) {
				GetAttrList(p_node, attr_list);
				{
					FPoint sz;
					if(attr_list.GetText(tWidth, temp_buf)) {
						_GetUSize(temp_buf, DIREC_HORZ, sz.X);
					}
					if(attr_list.GetText(tHeight, temp_buf)) {
						_GetUSize(temp_buf, DIREC_VERT, sz.Y);
					}
					rResult.SetSize(sz);
				}
				{
					SViewPort vp;
					GetViewPortAttr(attr_list, vp);
					rResult.SetViewPort(&vp);
				}
				ParsePrimitivs(p_node, rResult, temp_scan);
			}
		}
	}
	CATCHZOK
	if(p_doc)
		xmlFreeDoc(p_doc);
	P_Result = 0;
	return ok;
}

int SLAPI _ParseSvgFile(const char * pFileName, SDraw & rResult)
{
	SSvg svg;
	return svg.ParseFile(pFileName, rResult);
}
