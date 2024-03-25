// TEST-SLUI.CPP
// Copyright (c) A.Sobolev 2023, 2024
// @codepage UTF-8
// Тестирование низкоуровневых компонентов пользовательского интерфейса
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(LayoutFlex)
{
	SString root_path;
	SString temp_buf;
	SLS.QueryPath("testroot", root_path);
	{
		enum {
			loidMain = 1,
			loidUpr,
			loidLwr,
			loidF,
			loidB
		};
		const float f_width = 64.0f;
		SUiLayout lo_main;
		lo_main.SetID(loidMain);
		{
			SUiLayoutParam alb(DIREC_VERT, 0, SUiLayoutParam::alignStretch);
			alb.Flags &= ~SUiLayoutParam::fContainerWrap;
			lo_main.SetLayoutBlock(alb);
		}
		SUiLayout * p_lo_upr = lo_main.InsertItem();
		p_lo_upr->SetID(loidUpr);
		{
			SUiLayoutParam alb;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb.GrowFactor = 1.0f;
			p_lo_upr->SetLayoutBlock(alb);
		}
		SUiLayout * p_lo_lwr = lo_main.InsertItem();
		p_lo_lwr->SetID(loidLwr);
		{
			SUiLayoutParam alb(DIREC_HORZ, 0, SUiLayoutParam::alignStretch);
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb.GrowFactor = 1.0f;
			alb.Flags &= ~SUiLayoutParam::fContainerWrap;
			p_lo_lwr->SetLayoutBlock(alb);
		}
		SUiLayout * p_lo_f = p_lo_lwr->InsertItem();
		p_lo_f->SetID(loidF);
		{
			SUiLayoutParam alb;
			alb.SetVariableSizeX(SUiLayoutParam::szByContainer, 1.0f);
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			//alb.ShrinkFactor = 0.0f;
			p_lo_f->SetLayoutBlock(alb);
		}
		SUiLayout * p_lo_b = p_lo_lwr->InsertItem();
		p_lo_b->SetID(loidB);
		{
			SUiLayoutParam alb;
			alb.SetFixedSizeX(f_width);
			alb.SetVariableSizeY(SUiLayoutParam::szByContainer, 1.0f);
			alb.ShrinkFactor = 0.0f;
			p_lo_b->SetLayoutBlock(alb);
		}
		{
			SUiLayout::Param p;
			p.ForceSize.Set(360.0f, 240.0f);
			lo_main.Evaluate(&p);
			//
			{
				const FRect rc = lo_main.GetFrameAdjustedToParent();
				//SLCHECK_EQ(rc.Width(), p.ForceWidth);
				//SLCHECK_EQ(rc.Height(), p.ForceHeight);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByIdC(loidUpr);
				SLCHECK_EQ(p_lo_found, p_lo_upr);
				const FRect rc = p_lo_upr->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceSize.x);
				SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByIdC(loidLwr);
				SLCHECK_EQ(p_lo_found, p_lo_lwr);
				const FRect rc = p_lo_lwr->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceSize.x);
				SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByIdC(loidF);
				SLCHECK_EQ(p_lo_found, p_lo_f);
				const FRect rc = p_lo_f->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceSize.x-f_width);
				SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByIdC(loidB);
				SLCHECK_EQ(p_lo_found, p_lo_b);
				const FRect rc = p_lo_b->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), f_width);
				SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
			}
		}
		{
			SString js_buf;
			(temp_buf = root_path).SetLastSlash().Cat("out").SetLastSlash().Cat("lo-test.json");
			{
				SFile f_out(temp_buf, SFile::mWrite);
				SLCHECK_NZ(f_out.IsValid());
				SJson * p_js_obj = lo_main.ToJsonObj();
				SLCHECK_NZ(p_js_obj);
				p_js_obj->ToStr(temp_buf);
				SJson::FormatText(temp_buf, js_buf);
				f_out.Write(js_buf, js_buf.Len());
				delete p_js_obj;
			}
			//
			{
				(temp_buf = root_path).SetLastSlash().Cat("out").SetLastSlash().Cat("lo-test.json");
				SFile f_in(temp_buf, SFile::mRead);
				size_t actual_size = 0;
				SLCHECK_NZ(f_in.IsValid());
				SUiLayout lo_main2;
				STempBuffer in_buf(SMEGABYTE(1));
				SLCHECK_NZ(in_buf.IsValid());
				SLCHECK_NZ(f_in.ReadAll(in_buf, 0, &actual_size));
				js_buf.Z().CatN(in_buf.cptr(), actual_size);
				SJson * p_js_obj = SJson::Parse(js_buf);
				SLCHECK_NZ(p_js_obj);
				if(p_js_obj) {
					SLCHECK_NZ(lo_main2.FromJsonObj(p_js_obj));
					{
						SUiLayout::Param p;
						p.ForceSize.Set(360.0f, 240.0f);
						lo_main2.Evaluate(&p);
						//
						{
							const FRect rc = lo_main2.GetFrameAdjustedToParent();
							//SLCHECK_EQ(rc.Width(), p.ForceWidth);
							//SLCHECK_EQ(rc.Height(), p.ForceHeight);
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByIdC(loidUpr);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_upr->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceSize.x);
								SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByIdC(loidLwr);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_lwr->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceSize.x);
								SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByIdC(loidF);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_f->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceSize.x-f_width);
								SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByIdC(loidB);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_b->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), f_width);
								SLCHECK_EQ(rc.Height(), p.ForceSize.y/2.0f);
							}
						}
					}
				}
			}
		}
		{
			// Проверяем функции копирования и сравнения //
			SUiLayout lo2(lo_main);
			SLCHECK_NZ(lo_main == lo2);
		}
	}
	return CurrentStatus;
}

int SColorSet::Test()
{
	int    ok = 1;
	SString temp_buf;
	Z();
	const SColor primary(0, 0xff, 0xff);
	const SColor secondary(0, 0x80, 0x00);
	{
		{
			ComplexColorBlock ccb;
			THROW(ParseComplexColorBlock("#00ffff", ccb));
			THROW(ccb.RefSymb.IsEmpty());
			THROW(ccb.Func == funcNone);
			THROW(ccb.C == SColor(SClrCyan));
			THROW(ccb.C == primary);
			THROW(Put("primary", new ComplexColorBlock(ccb)));
		}
		{
			ComplexColorBlock ccb;
			THROW(ParseComplexColorBlock("#008000", ccb));
			THROW(ccb.RefSymb.IsEmpty());
			THROW(ccb.Func == funcNone);
			THROW(ccb.C == SColor(SClrGreen));
			THROW(ccb.C == secondary);
			THROW(Put("secondary", new ComplexColorBlock(ccb)));
		}
	}
	{
		const char * p_symb = "color001";
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("#ff0000", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcNone);
		THROW(ccb.C == SColor(SClrRed));
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
		}
	}
	{
		const char * p_symb = "color002";
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("#ff0000|0.5", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcNone);
		THROW(ccb.C == SColor(0xff, 0x00, 0x00).SetAlphaF(0.5f));
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
		}
	}
	{
		const char * p_symb = "color003";
		const SString ref_symb("primary");
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock(temp_buf.Z().CatChar('$').Cat(ref_symb), ccb));
		THROW(ccb.RefSymb.IsEqiAscii(ref_symb));
		THROW(ccb.Func == funcNone);
		THROW(ccb.C == ZEROCOLOR);
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb_ref;
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(Get(ref_symb, &ccb_ref));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == ccb_ref.C);
		}
		//
		//ResolveComplexColorBlock(ccb, )
	}
	{
		const char * p_symb = "color004";
		const SString ref_symb("primary");
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("$Primary|0.7", ccb));
		THROW(ccb.RefSymb.IsEqiAscii(ref_symb));
		THROW(ccb.Func == funcNone);
		THROW(ccb.C == SColor(ZEROCOLOR).SetAlphaF(0.7f));
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb_ref;
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(Get(ref_symb, &ccb_ref));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == SColor(ccb_ref.C).SetAlphaF(0.7f));
		}
	}
	{
		const char * p_symb = "color005";
		const SString ref_symb1("primary");
		const SString ref_symb2("secondary");
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("lerp $Primary $Secondary 0.4", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcLerp);
		THROW(ccb.C == ZEROCOLOR);
		THROW(ccb.ArgList.getCount() == 3);
		THROW(ccb.ArgList.at(0)->GetType() == argtRefColor);
		THROW(ccb.ArgList.at(0)->RefSymb.IsEqiAscii("primary"));
		THROW(ccb.ArgList.at(0)->C == ZEROCOLOR);
		THROW(ccb.ArgList.at(1)->GetType() == argtRefColor);
		THROW(ccb.ArgList.at(1)->RefSymb.IsEqiAscii("secondary"));
		THROW(ccb.ArgList.at(1)->C == ZEROCOLOR);
		THROW(ccb.ArgList.at(2)->GetType() == argtNumber);
		THROW(ccb.ArgList.at(2)->F == 0.4f);
		THROW(ccb.ArgList.at(2)->C == ZEROCOLOR);
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb_ref1;
			ComplexColorBlock ccb_ref2;
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(Get(ref_symb1, &ccb_ref1));
			THROW(Get(ref_symb2, &ccb_ref2));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == SColor::Lerp(primary, secondary, 0.4f));
		}
	}
	{
		const char * p_symb = "color006";
		const SString ref_symb("primary");
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("lighten $Primary 0.1", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcLighten);
		THROW(ccb.C == ZEROCOLOR);
		THROW(ccb.ArgList.getCount() == 2);
		THROW(ccb.ArgList.at(0)->GetType() == argtRefColor);
		THROW(ccb.ArgList.at(0)->RefSymb.IsEqiAscii("primary"));
		THROW(ccb.ArgList.at(0)->C == ZEROCOLOR);
		THROW(ccb.ArgList.at(1)->GetType() == argtNumber);
		THROW(ccb.ArgList.at(1)->F == 0.1f);
		THROW(ccb.ArgList.at(1)->C == ZEROCOLOR);
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb_ref;
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(Get(ref_symb, &ccb_ref));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == SColor(ccb_ref.C).Lighten(0.1f));
		}
	}
	{
		const char * p_symb = "color007";
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("darken #green|180 0.3", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcDarken);
		THROW(ccb.C == ZEROCOLOR);
		THROW(ccb.ArgList.getCount() == 2);
		THROW(ccb.ArgList.at(0)->GetType() == argtAbsoluteColor);
		THROW(ccb.ArgList.at(0)->RefSymb.IsEmpty());
		THROW(ccb.ArgList.at(0)->C == SColor(SClrGreen).SetAlpha(180));
		THROW(ccb.ArgList.at(1)->GetType() == argtNumber);
		THROW(ccb.ArgList.at(1)->F == 0.3f);
		THROW(ccb.ArgList.at(1)->C == ZEROCOLOR);
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == SColor(SClrGreen).Darken(0.3f).SetAlpha(180));
		}
	}
	{
		const char * p_symb = "color008";
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("grey 0.14", ccb));
		THROW(ccb.RefSymb.IsEmpty());
		THROW(ccb.Func == funcGrey);
		THROW(ccb.C == ZEROCOLOR);
		THROW(ccb.ArgList.getCount() == 1);
		THROW(ccb.ArgList.at(0)->GetType() == argtNumber);
		THROW(ccb.ArgList.at(0)->RefSymb.IsEmpty());
		THROW(ccb.ArgList.at(0)->C == ZEROCOLOR);
		THROW(ccb.ArgList.at(0)->F == 0.14f);
		THROW(Put(p_symb, new ComplexColorBlock(ccb)));
		{
			ComplexColorBlock ccb2;
			THROW(Get(p_symb, &ccb2));
			THROW(ccb2.RefSymb == ccb.RefSymb);
			THROW(ccb2.Func == ccb.Func);
			THROW(ccb2.C == ccb.C);
			//
			SColor color_resolved;
			THROW(Get(p_symb, 0, color_resolved) > 0);
			THROW(color_resolved == SColor(0.14f));
		}
	}
	{
		ComplexColorBlock ccb;
		THROW(ParseComplexColorBlock("nofunction $secondary 0.14", ccb) == 0);
	}
	CATCHZOK
	return ok;
}

SLTEST_R(SColorSet)
{
	SColorSet cs;
	SLCHECK_NZ(cs.Test());
	return CurrentStatus;
}

SLTEST_R(SFontDescr)
{
	SJson * p_js = 0;
	TSCollection <SFontDescr> font_list1;
	TSCollection <SFontDescr> font_list2;
	{
		// Считываем json-описание шрифтов
		p_js = SJson::ParseFile(MakeInputFilePath("fontdescr_list.json"));
		THROW(SLCHECK_NZ(p_js));
		if(p_js->IsObject() && p_js->P_Child) {
			SFontDescr::ListFromJsonArray(p_js->P_Child->P_Child, font_list1);
		}
		ZDELETE(p_js);
	}
	{
		// Обратно преобразуем список дескрипторов в json-формат

		SJson js(SJson::tOBJECT);
		SJson * p_js_list = SFontDescr::ListToJsonArray(font_list1);
		THROW(SLCHECK_NZ(p_js_list));
		js.Insert("font_list", p_js_list);
		//
		THROW(SLCHECK_NZ(js.P_Child));
		THROW(SLCHECK_NZ(SFontDescr::ListFromJsonArray(js.P_Child->P_Child, font_list2)));
		THROW(SLCHECK_NZ(TSCollection_IsEq(&font_list1, &font_list2)));
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	delete p_js;
	return CurrentStatus;
}

SLTEST_R(UiDescription)
{
	SJson * p_js = 0;
	SJson * p_js2 = 0;
	SJson * p_js_reverse = 0;
	UiDescription uid;
	UiDescription uid2; // Результат, полученный из реверсивного json'а
	SString temp_buf;
	{
		p_js = SJson::ParseFile(MakeInputFilePath("colorset-imgui-test.json"));
		THROW(SLCHECK_NZ(p_js));
		SLCHECK_NZ(uid.FromJsonObj(p_js));
		{
			p_js_reverse = uid.ToJsonObj();
			THROW(SLCHECK_NZ(p_js_reverse));
			THROW(SLCHECK_NZ(p_js_reverse->ToStr(temp_buf)));
			{
				SFile f_out(MakeOutputFilePath("colorset-imgui-test-reverse.json"), SFile::mWrite);
				THROW(SLCHECK_NZ(f_out.IsValid()));
				f_out.Write(temp_buf.cptr(), temp_buf.Len());
			}
		}
		{
			/*
				{
					"symb": "testset01",
					"a": "#white",
					"b": "#green"
				},
				{
					"symb": "testset02",
					"a": "#black",
					"b": "#blue"
				},
				{
					"symb": "testset03",
					"a1": "$testset01.a",
					"b1": "$testset01.b",
					"a2": "$testset02.a",
					"b2": "$testset02.b",
					"x": "$testset03.a1",
					"y": "a2"
				},
			*/
			SColor color_result1;
			SColor color_result2;
			SLCHECK_NZ(uid.GetColor("testset03", "a1", color_result1));
			SLCHECK_NZ(uid.GetColor("testset01", "a", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrWhite));
			SLCHECK_EQ(color_result1, color_result2);
			//
			SLCHECK_NZ(uid.GetColor("testset03", "b1", color_result1));
			SLCHECK_NZ(uid.GetColor("testset01", "b", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrGreen));
			SLCHECK_EQ(color_result1, color_result2);
			//
			SLCHECK_NZ(uid.GetColor("testset03", "a2", color_result1));
			SLCHECK_NZ(uid.GetColor("testset02", "a", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrBlack));
			SLCHECK_EQ(color_result1, color_result2);
			//
			SLCHECK_NZ(uid.GetColor("testset03", "b2", color_result1));
			SLCHECK_NZ(uid.GetColor("testset02", "b", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrBlue));
			SLCHECK_EQ(color_result1, color_result2);
			//
			SLCHECK_NZ(uid.GetColor("testset03", "x", color_result1));
			SLCHECK_NZ(uid.GetColor("testset03", "a1", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrWhite));
			SLCHECK_EQ(color_result1, color_result2);
			//
			SLCHECK_NZ(uid.GetColor("testset03", "y", color_result1));
			SLCHECK_NZ(uid.GetColor("testset03", "a2", color_result2));
			SLCHECK_EQ(color_result1, SColor(SClrBlack));
			SLCHECK_EQ(color_result1, color_result2);
		}
	}
	{
		p_js2 = SJson::ParseFile(MakeOutputFilePath("colorset-imgui-test-reverse.json"));
		THROW(SLCHECK_NZ(p_js2));
		SLCHECK_NZ(uid2.FromJsonObj(p_js2));
		SLCHECK_NZ(uid2.IsEq(uid));
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	delete p_js;
	delete p_js2;
	delete p_js_reverse;
	return CurrentStatus;
}
