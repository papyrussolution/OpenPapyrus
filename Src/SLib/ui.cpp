// UI.CPP
// Copyright (c) A.Sobolev 2011, 2016, 2018, 2020, 2023
//
#include <slib-internal.h>
#pragma hdrstop

/*static*/int UiItemKind::GetTextList(StrAssocArray & rList)
{
	int    ok = 1;
	rList.Z();
	UiItemKind item;
	for(int i = kDialog; i < kCount; i++) {
		if(item.Init(i)) {
			rList.Add(i, item.Text);
		}
	}
	return ok;
}

/*static*/int UiItemKind::GetIdBySymb(const char * pSymb)
{
	int    id = 0;
	UiItemKind item;
	for(int i = kDialog; !id && i < kCount; i++) {
		if(item.Init(i) && item.Symb.Cmp(pSymb, 0) == 0)
			id = i;
	}
	return id;
}

UiItemKind::UiItemKind(int kind)
{
	Init(kind);
}

int UiItemKind::Init(int kind)
{
	int    ok = 1;
	Id = 0;
	P_Cls = 0;
	Symb.Z();
	Text.Z();
	const char * p_text_sign = 0;
	switch(kind) {
		case kUnkn:
			break;
		case kDialog:
			p_text_sign = "ui_dialog";
			Symb = "dialog";
			break;
		case kInput:
			p_text_sign = "ui_input";
			Symb = "input";
			break;
		case kStatic:
			p_text_sign = "ui_static";
			Symb = "statictext";
			break;
		case kPushbutton:
			p_text_sign = "ui_pushbutton";
			Symb = "button";
			break;
		case kCheckbox:
			p_text_sign = "ui_checkbox";
			Symb = "checkbox";
			break;
		case kRadioCluster:
			p_text_sign = "ui_radiocluster";
			Symb = "radiocluster";
			break;
		case kCheckCluster:
			p_text_sign = "ui_checkcluster";
			Symb = "checkcluster";
			break;
		case kCombobox:
			p_text_sign = "ui_combobox";
			Symb = "combobox";
			break;
		case kListbox:
			p_text_sign = "ui_listbox";
			Symb = "listbox";
			break;
		case kTreeListbox:
			p_text_sign = "ui_treelistbox";
			Symb = "treelistbox";
			break;
		case kFrame:
			p_text_sign = "ui_frame";
			Symb = "framebox";
			break;
		default:
			ok = 0;
			break;
	}
	if(p_text_sign) {
		Id = kind;
		SLS.LoadString_(p_text_sign, Text);
	}
	return ok;
}
//
// @v11.7.10 @construction {
//

SColorSet::InnerEntry::InnerEntry() : C(ZEROCOLOR), CcbP(0)
{
}

const void * SColorSet::InnerEntry::GetHashKey(const void * pCtx, uint * pKeyLen) const // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования.
{
	const  void * p_result = 0;
	uint   key_len = 0;
	assert(pCtx);
	if(pCtx) {
		//
		// Здесь я применяю довольно рискованную тактику получения ключа: формирую его в револьверном буфере SString.
		// Однако, учитывая то, что результат работы GetHashKey очень короткоживущий, вероятность перезаписи этого
		// буфера в течении времени, когда он реально нужен, мизерная.
		//
		SString & r_key_buf = SLS.AcquireRvlStr();
		static_cast<const SColorSet *>(pCtx)->GetS(SymbP, r_key_buf);
		r_key_buf.Utf8ToLower();
		p_result = r_key_buf.cptr();
		key_len = r_key_buf.Len();
	}
	ASSIGN_PTR(pKeyLen, key_len);
	return p_result;
}

SColorSet::SColorSet(const char * pSymb) : L(256, this), Symb(pSymb)
{
}

SColorSet & SColorSet::Z()
{
	Symb.Z();
	L.Z();
	return *this;
}

int SColorSet::SetSymb(const char * pSymb)
{
	Symb = pSymb;
	return 1;
}
	
const void * SColorSet::GetHashKey(const void * pCtx, uint * pKeyLen) const // Descr: Каноническая функция возвращающая ключ экземпляра для хэширования
{
	ASSIGN_PTR(pKeyLen, Symb.Len());
	return Symb.cptr();
}

SColorSet::ColorArg::ColorArg() : C(ZEROCOLOR)
{
}

SString & SColorSet::ColorArg::ToStr(SString & rBuf) const
{
	rBuf.Z();
	if(RefSymb.NotEmpty()) {
		rBuf.CatChar('$').Cat(RefSymb);
	}
	else if(!C.IsEmpty()) {
		C.ToStr(rBuf, SColor::fmtHEX|SColor::fmtName);
		if(C.Alpha < 255) {
			rBuf.CatChar('|').Cat(C.Alpha);
		}
	}
	else if(F > 0.0f)
		rBuf.Cat(F, MKSFMTD(0, 2, NMBF_NOTRAILZ|NMBF_OMITEPS));
	return rBuf;
}

SColorSet::ComplexColorBlock::ComplexColorBlock() : C(ZEROCOLOR), Func(funcNone)
{
}
		
SColorSet::ComplexColorBlock & SColorSet::ComplexColorBlock::Copy(const ComplexColorBlock & rS)
{
	C = rS.C;
	RefSymb = rS.RefSymb;
	Func = rS.Func;
	TSCollection_Copy(ArgList, rS.ArgList);
	return *this;
}

SColorSet::ComplexColorBlock & SColorSet::ComplexColorBlock::Z()
{
	C = ZEROCOLOR;
	RefSymb.Z();
	Func = funcNone;
	ArgList.clear();
	return *this;
}
		
SString & SColorSet::ComplexColorBlock::ToStr(SString & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	if(Func) {
		switch(Func) {
			case funcLerp:
				{
					rBuf.Cat("lerp");
					for(uint i = 0; i < ArgList.getCount(); i++) {
						const ColorArg * p_arg = ArgList.at(i);
						if(p_arg) {
							p_arg->ToStr(temp_buf);
							rBuf.Space().Cat(temp_buf);
						}
					}
				}
				break;
			case funcLighten:
				break;
			case funcDarken:
				break;
			case funcGrey:
				break;
			default:
				break;
		}
	}
	else if(RefSymb.NotEmpty()) {
		rBuf.Cat(RefSymb);
	}
	else if(!C.IsEmpty()) {
		C.ToStr(rBuf, SColor::fmtHEX|SColor::fmtName);
		if(C.Alpha < 255) {
			rBuf.CatChar('|').Cat(C.Alpha);
		}
	}
	else {
		;
	}
	return rBuf;
}

int SColorSet::Helper_ParsePrimitive(SStrScan & rScan, ColorArg & rItem) const
{
	int    ok = 0;
	bool   syntax_err = false;
	SString temp_buf;
	rScan.Skip();
	if(rScan[0] == '#') {
		rScan.Incr();
		if(rScan.GetXDigits(temp_buf)) {
			// hex-цвет
			temp_buf.Insert(0, "#");
			if(rItem.C.FromStr(temp_buf))
				ok = 1;
		}
		else if(rScan.GetIdent(temp_buf)) {
			// именованный цвет	
			if(rItem.C.FromStr(temp_buf))
				ok = 1;			
		}
		else 
			syntax_err = true;
		if(!syntax_err) {
			if(rScan[0] == '|') {
				// alpha
				rScan.Incr();
				if(rScan.GetNumber(temp_buf)) {
					double alpha = temp_buf.ToReal();
					if(alpha > 1.0 && alpha < 256.0)
						rItem.C.SetAlpha(static_cast<uint8>(alpha));
					else if(alpha >= 0.0 && alpha <= 1.0)
						rItem.C.SetAlphaF(static_cast<float>(alpha));
					else {
						; // @todo @err
					}
				}
			}
		}
	}
	else if(rScan[0] == '$') {
		rScan.Incr();
		if(rScan.GetIdent(temp_buf)) {
			// ссылка на другой цвет набора
			rItem.RefSymb = temp_buf;
			ok = 3;
		}
		else {
			syntax_err = true;
			// @todo @err
		}
	}
	else if(rScan.GetNumber(temp_buf)) {
		rItem.F = static_cast<float>(temp_buf.ToReal_Plain());
		ok = 2;
	}
	return ok;
}

int SColorSet::ParseComplexColorBlock(const char * pText, ComplexColorBlock & rBlk) const
{
	// #xxxxxx
	// #xxxxxx|xx - with alpha
	// $primary - referece
	// lighten $secondary 0.2
	// darken $primary|xx 0.35
	// lerp $primary #xxxxxx
	// lerp $secondary 0.7
	// 
	int    ok = 1;
	SString src_buf(pText);
	if(!src_buf.NotEmptyS()) {
		ok = -1;
	}
	else {
		SString temp_buf;
		SStrScan scan(src_buf);
		if(scan.GetIdent(temp_buf)) {
			//funcLerp,      // (color, factor) || (color, color)
			//funcLighten,   // (color, factor)
			//funcDarken,    // (color, factor)
			//funcGrey,      // (whitePart)
			int    func = funcNone;
			if(temp_buf.IsEqiAscii("lerp")) {
				func = funcLerp;
			}
			else if(temp_buf.IsEqiAscii("lighten")) {
				func = funcLighten;
			}
			else if(temp_buf.IsEqiAscii("darken")) {
				func = funcDarken;
			}
			else if(temp_buf.IsEqiAscii("grey")) {
				func = funcGrey;
			}
			else {
				CALLEXCEPT(); // @todo @err
			}
			rBlk.Func = func;
			scan.Skip();
			while(ok && !scan.IsEnd()) {
				ColorArg primitive;
				int gpr = Helper_ParsePrimitive(scan, primitive);
				if(gpr > 0) {
					ColorArg * p_new_arg = rBlk.ArgList.CreateNewItem();
					*p_new_arg = primitive;
				}
				else {
					; // @todo @err
					ok = 0;
				}
				scan.Skip();
			}
			if(ok) {
				switch(func) {
					case funcLerp:
						THROW(rBlk.ArgList.getCount() == 2); // @todo @err
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							THROW(!p_arg1->C.IsEmpty() || p_arg1->RefSymb.NotEmpty());
						}
						{
							const ColorArg * p_arg2 = rBlk.ArgList.at(1);
							THROW(!p_arg2->C.IsEmpty() || p_arg2->RefSymb.NotEmpty() || p_arg2->F > 0.0f);
						}
						break;
					case funcLighten:
					case funcDarken:
						THROW(rBlk.ArgList.getCount() == 2); // @todo @err
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							THROW(!p_arg1->C.IsEmpty() || p_arg1->RefSymb.NotEmpty());
						}
						{
							const ColorArg * p_arg2 = rBlk.ArgList.at(1);
							THROW(p_arg2->F > 0.0f);
						}
						break;
					case funcGrey:
						THROW(rBlk.ArgList.getCount() == 1); // @todo @err
						{
							const ColorArg * p_arg1 = rBlk.ArgList.at(0);
							THROW(p_arg1->F > 0.0f);
						}
						break;
					default:
						assert(0); // unknown function
						break;
				}
			}
		}
		else {
			ColorArg primitive;
			int gpr = Helper_ParsePrimitive(scan, primitive);
			switch(gpr) {
				case 1: // color
					
					break;
				case 2: // number
					break;
				case 3: // reference
					break;
				default:
					CALLEXCEPT();
					break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SColorSet::ResolveComplexColorBlock(const ComplexColorBlock & rBlk, SColor & rC) const
{
	int    ok = 1;
	if(rBlk.Func) {
		switch(rBlk.Func) {
			case funcLerp:
				THROW(rBlk.ArgList.getCount() == 2);
				break;
			case funcLighten:
				THROW(rBlk.ArgList.getCount() == 2);
				break;
			case funcDarken:
				THROW(rBlk.ArgList.getCount() == 2);
				break;
			case funcGrey:
				THROW(rBlk.ArgList.getCount() == 1);
				break;
			default:
				CALLEXCEPT();
				break;
		}
	}
	else if(rBlk.RefSymb.NotEmpty()) {
		Get(rBlk.RefSymb, 0);
	}
	else if(!rBlk.C.IsEmpty()) {
		rC = rBlk.C;
	}
	CATCHZOK
	return ok;
}

SJson * SColorSet::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	InnerEntry * p_entry = 0;
	SString symb_buf;
	SString val_buf;
	p_result->InsertStringNe("symb", GetSymb());
	for(uint idx = 0; L.Enum(&idx, &p_entry);) {
		GetS(p_entry->SymbP, symb_buf);
		if(symb_buf.NotEmptyS()) {
			p_entry->C.ToStr(val_buf, SColor::fmtName|SColor::fmtHEX);
			p_result->InsertString(symb_buf.Escape(), val_buf);
		}
	}
	return p_result;
}
	
int SColorSet::FromJsonObj(const SJson * pJs)
{
	int    ok = 1;
	SString symb_buf;
	SString val_buf;
	THROW(SJson::IsObject(pJs)); // @todo @err
	Z();
	for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
		if(p_jsn->P_Child) {
			(symb_buf = p_jsn->Text).Unescape();
			val_buf = p_jsn->P_Child->Text;
			if(symb_buf.IsEqiAscii("symb")) {
				SetSymb(val_buf.Unescape());
			}
			else {
				SColor c;
				THROW(c.FromStr(val_buf)); // @todo @err
				THROW(Put(symb_buf, c)); // @todo @err
			}
		}
	}
	CATCHZOK
	return ok;
}
	
int SColorSet::Put(const char * pSymb, SColor c)
{
	int    ok = 1;
	if(!isempty(pSymb)) {
		InnerEntry new_entry;
		new_entry.C = c;
		AddS(pSymb, &new_entry.SymbP);
		ok = L.Put(&new_entry, true/*forceUpdate*/);
	}
	else
		ok = 0;
	return ok;
}
	
int SColorSet::Get(const char * pSymb, SColor * pC) const
{
	int    ok = 0;
	SColor c(ZEROCOLOR);
	if(!isempty(pSymb)) {
		SString & r_key_buf = SLS.AcquireRvlStr();
		r_key_buf.Cat(pSymb).Utf8ToLower();
		const InnerEntry * p_entry = L.Get(r_key_buf.cptr(), r_key_buf.Len());
		if(p_entry) {
			c = p_entry->C;
			ok = 1;
		}
	}
	ASSIGN_PTR(pC, c);
	return ok;
}

UiDescription::UiDescription()
{
}

UiDescription::~UiDescription()
{
}

UiDescription & UiDescription::Z()
{
	FontList.clear();
	ClrList.clear();
	LoList.clear();
	return *this;
}

UiDescription & UiDescription::Copy(const UiDescription & rS)
{
	TSCollection_Copy(FontList, rS.FontList);
	TSCollection_Copy(ClrList, rS.ClrList);
	TSCollection_Copy(LoList, rS.LoList);
	return *this;
}

SColorSet * UiDescription::GetColorSet(const char * pCsSymb)
{
	return const_cast<SColorSet *>(GetColorSetC(pCsSymb));
}

const SColorSet * UiDescription::GetColorSetC(const char * pCsSymb) const
{
	const SColorSet * p_result = 0;
	for(uint i = 0; !p_result && i < ClrList.getCount(); i++) {
		const SColorSet * p_cset = ClrList.at(i);
		if(p_cset && p_cset->GetSymb().IsEqiAscii(pCsSymb))
			p_result = p_cset;
	}
	return p_result;
}


SJson * UiDescription::ToJsonObj() const
{
	SJson * p_result = SJson::CreateObj();
	SString temp_buf;
	p_result->InsertString("symb", "ui");
	if(FontList.getCount()) {
		SJson * p_js_fontsrc_list = SJson::CreateArr();
		for(uint i = 0; i < FontList.getCount(); i++) {
			const SFontSource * p_item = FontList.at(i);
			if(p_item) {
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_fontsrc_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("fontsrc_list", p_js_fontsrc_list);
		p_js_fontsrc_list = 0;
	}
	if(ClrList.getCount()) {
		bool is_there_anonym = false;
		SJson * p_js_colorset_list = SJson::CreateArr();
		for(uint i = 0; i < ClrList.getCount(); i++) {
			const SColorSet * p_item = ClrList.at(i);
			if(p_item) {
				const SString & r_symb = p_item->GetSymb();
				if(r_symb.IsEmpty()) {
					THROW(!is_there_anonym); // @todo @err
					temp_buf = "default";
					is_there_anonym = true;
				}
				else
					temp_buf = r_symb;
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_colorset_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("colorset_list", p_js_colorset_list);
		p_js_colorset_list = 0;
	}
	if(LoList.getCount()) {
		SJson * p_js_lo_list = SJson::CreateArr();
		for(uint i = 0; i < LoList.getCount(); i++) {
			const SUiLayout * p_item = LoList.at(i);
			if(p_item) {
				SJson * p_js_item = p_item->ToJsonObj();
				THROW(p_js_item);
				p_js_lo_list->InsertChild(p_js_item);
				p_js_item = 0;
			}
		}
		p_result->Insert("layout_list", p_js_lo_list);
		p_js_lo_list = 0;
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

int UiDescription::FromJsonObj(const SJson * pJsObj)
{
	int    ok = 1;
	THROW(SJson::IsObject(pJsObj)); // @todo @err
	Z();
	for(const SJson * p_jsn = pJsObj->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
		if(p_jsn->P_Child) {
			if(p_jsn->Text.IsEqiAscii("symb")) {
			}
			else if(p_jsn->Text.IsEqiAscii("fontsrc_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SFontSource * p_item = FontList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
				}
			}
			else if(p_jsn->Text.IsEqiAscii("colorset_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SColorSet * p_item = ClrList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
				}
			}
			else if(p_jsn->Text.IsEqiAscii("layout_list")) {
				if(SJson::IsArray(p_jsn->P_Child)) {
					for(const SJson * p_js_inner = p_jsn->P_Child->P_Child; p_js_inner; p_js_inner = p_js_inner->P_Next) {
						SUiLayout * p_item = LoList.CreateNewItem();
						THROW(p_item);								
						THROW(p_item->FromJsonObj(p_js_inner));
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

// } } @v11.7.10 @construction