// TEST-ETC.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Модуль тестирования разных функций. В основном в процессе разработки.
//
#include <pp.h>
#pragma hdrstop
#include <combaseapi.h>
#include "..\slib\bzip3\include\libbz3.h"
#include "libsais.h"
#include "..\OSF\zstd\lib\include\divsufsort.h"
//
//
//
SLTEST_R(ImportPo)
{
	// C:\Papyrus\Src\Rsrc\Data\iso-codes
	// C:\Papyrus\Src\Rsrc\Data\iso-codes\iso_15924
	SString temp_buf;
	SString path_buf;
	PoBlock blk(0);
	SDirEntry de;
	SString base_dir("/Papyrus/Src/Rsrc/Data/iso-codes/iso_15924/");
	(temp_buf = base_dir).Cat("*.po");
	for(SDirec sd(temp_buf); sd.Next(&de) > 0;) {
		if(!de.IsSelf() && !de.IsUpFolder() && (de.IsFolder() || de.IsFile())) {
			de.GetNameA(base_dir, temp_buf);
			SPathStruc::NormalizePath(temp_buf, SPathStruc::npfCompensateDotDot, path_buf);
			blk.Import(path_buf, 0, 0);
		}
	}
	blk.Finish();
	blk.Sort();
	{
		SJson * p_js = blk.ExportToJson();
		if(p_js) {
			p_js->ToStr(temp_buf);
			(path_buf = base_dir).SetLastDSlash().Cat("importpo-out-file.json");
			SFile f_out(path_buf, SFile::mWrite);
			f_out.Write(temp_buf.cptr(), temp_buf.Len());
		}
	}
	return CurrentStatus;
}

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
			p.ForceWidth = 360.0f;
			p.ForceHeight = 240.0f;
			lo_main.Evaluate(&p);
			//
			{
				const FRect rc = lo_main.GetFrameAdjustedToParent();
				//SLCHECK_EQ(rc.Width(), p.ForceWidth);
				//SLCHECK_EQ(rc.Height(), p.ForceHeight);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByID(loidUpr);
				SLCHECK_EQ(p_lo_found, p_lo_upr);
				const FRect rc = p_lo_upr->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceWidth);
				SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByID(loidLwr);
				SLCHECK_EQ(p_lo_found, p_lo_lwr);
				const FRect rc = p_lo_lwr->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceWidth);
				SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByID(loidF);
				SLCHECK_EQ(p_lo_found, p_lo_f);
				const FRect rc = p_lo_f->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), p.ForceWidth-f_width);
				SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
			}
			{
				const SUiLayout * p_lo_found = lo_main.FindByID(loidB);
				SLCHECK_EQ(p_lo_found, p_lo_b);
				const FRect rc = p_lo_b->GetFrameAdjustedToParent();
				SLCHECK_EQ(rc.Width(), f_width);
				SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
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
						p.ForceWidth = 360.0f;
						p.ForceHeight = 240.0f;
						lo_main2.Evaluate(&p);
						//
						{
							const FRect rc = lo_main2.GetFrameAdjustedToParent();
							//SLCHECK_EQ(rc.Width(), p.ForceWidth);
							//SLCHECK_EQ(rc.Height(), p.ForceHeight);
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByID(loidUpr);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_upr->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceWidth);
								SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByID(loidLwr);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_lwr->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceWidth);
								SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByID(loidF);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_f->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), p.ForceWidth-f_width);
								SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
							}
						}
						{
							const SUiLayout * p_lo_found = lo_main2.FindByID(loidB);
							SLCHECK_NZ(p_lo_found);
							if(p_lo_found) {
								SLCHECK_NZ(p_lo_found->GetLayoutBlockC().IsEq(p_lo_b->GetLayoutBlockC()));
								const FRect rc = p_lo_found->GetFrameAdjustedToParent();
								SLCHECK_EQ(rc.Width(), f_width);
								SLCHECK_EQ(rc.Height(), p.ForceHeight/2.0f);
							}
						}
					}
				}
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(HASHTAB)
{
	SString in_buf;
	SString line_buf;
	{
		const uint test_iter_count = 1000000;
		const size_t ht_size_tab[] = { 10, 100, 1000, 100000 };
		for(uint hts_idx = 0; hts_idx < SIZEOFARRAY(ht_size_tab); hts_idx++) {
			size_t ht_size = ht_size_tab[hts_idx];
			uint   _count = 0;
			SStrCollection ptr_collection;
			PtrHashTable ht(ht_size);

			(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
			SFile inf(in_buf, SFile::mRead);
			THROW(SLCHECK_NZ(inf.IsValid()));
			while(inf.ReadLine(line_buf, SFile::rlfChomp)) {
				char * p_str = newStr(line_buf);
				THROW(SLCHECK_NZ(ptr_collection.insert(p_str)));
				//
				// Нечетные позиции вставляем в кэш, четные - нет
				//
				if(_count % 2) {
					THROW(SLCHECK_NZ(ht.Add(p_str, _count+1, 0)));
				}
				else {
					//
				}
				_count++;
			}
			THROW(SLCHECK_EQ(ptr_collection.getCount(), _count));
			for(uint i = 0; i < test_iter_count; i++) {
				uint idx = SLS.GetTLA().Rg.GetUniformInt(_count);
				THROW(SLCHECK_LT((long)idx, (long)_count));
				char * p_str = ptr_collection.at(idx);
				{
					uint val = 0;
					uint pos = 0;
					if(idx % 2) {
						SLCHECK_NZ(ht.Search(p_str, &val, &pos));
						void * ptr = ht.Get(pos);
						SLCHECK_NZ(ptr);
						SLCHECK_EQ(ptr, (const void *)p_str);
						SLCHECK_EQ(val, idx+1);
					}
					else {
						SLCHECK_Z(ht.Search(p_str, &val, &pos));
					}
				}
			}
		}
	}
	{
		//
		// 
		//
		const size_t ht_size_tab[] = { 10, 100, 1000, 100000 };
		for(uint hts_idx = 0; hts_idx < SIZEOFARRAY(ht_size_tab); hts_idx++) {
			size_t ht_size = ht_size_tab[hts_idx];
			uint   _count = 0;
			SStrCollection ptr_collection;
			TokenSymbHashTable tsht(ht_size);

			(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
			SFile inf(in_buf, SFile::mRead);
			THROW(SLCHECK_NZ(inf.IsValid()));
			while(inf.ReadLine(line_buf, SFile::rlfChomp)) {
				char * p_str = newStr(line_buf);
				THROW(SLCHECK_NZ(ptr_collection.insert(p_str)));
				_count++;
			}
			THROW(SLCHECK_EQ(ptr_collection.getCount(), _count));
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_Z(tsht.Get(key, 0));
					SLCHECK_NZ(tsht.Put(key, ptr_collection.at(key-1)));
				}
			}
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_EQ(tsht.Put(key, ptr_collection.at(key-1)), 1);
				}
			}
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_NZ(tsht.Get(key, &line_buf));
					SLCHECK_EQ(line_buf, ptr_collection.at(key-1));
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
//
// @sandbox {
// Отработка итераторов
//
SLTEST_R(iterator)
{
	class _Container {
	public:
		class Iterator {
			const _Container & R_Obj;
		public:
			Iterator(const _Container & rObj) : R_Obj(rObj), Idx(0)
			{
			}
			Iterator(const Iterator & rS) : R_Obj(rS.R_Obj), Idx(rS.Idx)
			{
			}
			bool operator == (const Iterator & rS) const
			{
				return (&rS.R_Obj == &R_Obj && rS.Idx == Idx);
			}
			bool operator != (const Iterator & rS) const
			{
				return (&rS.R_Obj != &R_Obj || rS.Idx != Idx);
			}
			const int * operator * () const { return &R_Obj[Idx]; }
			const int & operator & () const { return R_Obj[Idx]; }
			Iterator & operator++ ()
			{
				if(Idx < R_Obj.C)
					Idx++;
				return *this;
			}
			Iterator operator++ (int)
			{
				Iterator preserve(*this);
				if(Idx < R_Obj.C)
					Idx++;
				return preserve;
			}
			uint   Idx;
		};
		_Container() : C(0)
		{
			MEMSZERO(Items);
		}
		const int & operator [](size_t idx) const
		{ 
			static int dummy = 0;
			return (idx < C) ? Items[idx] : dummy;
		}
		Iterator begin() const
		{
			Iterator it(*this);
			return it;
		}
		Iterator end() const
		{
			Iterator it(*this);
			it.Idx = C;
			return it;
		}
		int    Items[1024];
		uint   C;
	};
	{
		_Container c;
		int pattern_sum = 0;
		{
			for(uint i = 0; i < 103; i++) {
				c.Items[i] = (int)(i+1);
				c.C++;
				pattern_sum += (i+1);
			}
		}
		{
			int s = 0;
			for(auto item : c) {
				s += *item;
			}
			assert(s == pattern_sum);
		}
	}
	return CurrentStatus;
}

// } @sandbox

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
			THROW(Get(p_symb, color_resolved) > 0);
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
			THROW(Get(p_symb, color_resolved) > 0);
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
			THROW(Get(p_symb, color_resolved) > 0);
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
			THROW(Get(p_symb, color_resolved) > 0);
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
			THROW(Get(p_symb, color_resolved) > 0);
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
			THROW(Get(p_symb, color_resolved) > 0);
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

SLTEST_R(GUID) // @v11.7.11
{
	{
		S_GUID u;
		SLCHECK_NZ(u.IsZero());
		u.Generate();
		SLCHECK_Z(u.IsZero());
		S_GUID u2(u);
		SLCHECK_EQ(u2, u);
		u2.Z();
		SLCHECK_Z(u2 == u);
		SLCHECK_Z(u2);
	}
	{
		const char * p_guid_text1 = "{076A7660-6891-4E8B-A9D7-E7A8B074267B}";
		const char * p_guid_text2 = "076A7660-6891-4E8B-A9D7-E7A8B074267B";
		const char * p_guid_text3 = "076a7660-6891-4e8b-a9d7-e7a8b074267b";
		SString temp_buf;
		GUID win_guid;
		S_GUID u;
		S_GUID u2;
		u.FromStr(p_guid_text1);
		u2.FromStr(p_guid_text2);
		SLCHECK_Z(CLSIDFromString(SUcSwitchW(p_guid_text1), &win_guid));
		SLCHECK_Z(SMem::Cmp(&win_guid, &u, sizeof(S_GUID)));
		SLCHECK_NZ(u2 == u);
		u.ToStr(S_GUID::fmtIDL, temp_buf);
		SLCHECK_EQ(temp_buf, p_guid_text2);
		u.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		SLCHECK_EQ(temp_buf, p_guid_text3);
	}
	return CurrentStatus;
}

SLTEST_R(bzip3)
{
	{
		//SetInfo("Compressing shakespeare.txt back and forth in memory");
		// Read the entire "shakespeare.txt" file to memory:
		SFile f_inp(MakeInputFilePath("shakespeare.txt"), SFile::mRead|SFile::mBinary);
		//FILE * fp = fopen("shakespeare.txt", "rb");
		//fseek(fp, 0, SEEK_END);
		//int64 fsize = 0LL;
		STempBuffer in_buf(SKILOBYTE(16));
		size_t in_buf_size = 0;
		//f_inp.CalcSize(&fsize);
		//size_t size = ftell(fp);
		//fseek(fp, 0, SEEK_SET);
		THROW(SLCHECK_NZ(f_inp.IsValid()));
		THROW(SLCHECK_NZ(in_buf.IsValid()));
		THROW(SLCHECK_NZ(f_inp.ReadAll(in_buf, 0, &in_buf_size)));
		assert(in_buf.GetSize() >= in_buf_size);
		{
			//uint8 * buffer = (uint8 *)SAlloc::M(size);
			//fread(buffer, 1, size, fp);
			//fclose(fp);
			// Compress the file:
			size_t out_size = bz3_bound(in_buf_size);
			STempBuffer out_buf(out_size);
			//uint8 * outbuf = (uint8 *)SAlloc::M(out_size);
			THROW(SLCHECK_NZ(out_buf.IsValid()));
			THROW(SLCHECK_EQ(bz3_compress(SMEGABYTE(1), in_buf.ucptr(), static_cast<uint8 *>(out_buf.vptr()), in_buf_size, &out_size), BZ3_OK));
			//printf("%d => %d\n", size, out_size);
			{
				// Decompress the file.
				size_t inflated_size = in_buf_size * 2;
				STempBuffer inflate_buf(inflated_size);
				THROW(SLCHECK_NZ(inflate_buf.IsValid()));
				THROW(SLCHECK_EQ(bz3_decompress(out_buf.ucptr(), static_cast<uint8 *>(inflate_buf.vptr()), out_size, &inflated_size), BZ3_OK));
				THROW(SLCHECK_EQ(inflated_size, in_buf_size));
				{
					bool debug_mark = false;
					for(uint i = 0; i < inflated_size; i++) {
						if(inflate_buf[i] != in_buf[i]) {
							debug_mark = true;
						}
					}
				}
				THROW(SLCHECK_Z(memcmp(inflate_buf.vcptr(), in_buf.vcptr(), in_buf_size)));
				//printf("%d => %d\n", out_size, size);
				//SAlloc::F(buffer);
				//SAlloc::F(outbuf);
				//return 0;
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

struct TestFixtureSuffixArray {
	TestFixtureSuffixArray() : SfxArray_Sais(0), SfxArray_DivSufSort(0), InBuf(SKILOBYTE(16)), InBufSize(0)
	{
	}
	int Init(const char * pInFileName)
	{
		int    ok = 1;
		SFile f_inp(pInFileName, SFile::mRead|SFile::mBinary);
		THROW(f_inp.IsValid());
		THROW(InBuf.IsValid());
		THROW(f_inp.ReadAll(InBuf, 0, &InBufSize));
		assert(InBuf.GetSize() >= InBufSize);
		THROW(SfxArray_Sais = new int32[InBufSize]);
		THROW(SfxArray_DivSufSort = new int32[InBufSize]);
		CATCHZOK
		return ok;
	}
	~TestFixtureSuffixArray()
	{
		delete [] SfxArray_Sais;
		delete [] SfxArray_DivSufSort;
	}
	STempBuffer InBuf;
	size_t InBufSize;
	int32 * SfxArray_Sais;
	int32 * SfxArray_DivSufSort;
};

SLTEST_FIXTURE(SuffixArray, TestFixtureSuffixArray)
{
	// int32_t libsais(const uint8_t * T, int32_t * SA, int32_t n, int32_t fs, int32_t * freq);
	// int divsufsort(const uchar * T, int * SA, int n, int openMP);
	// benchmark=sais;divsufsort
	int    bm = -1;
	if(pBenchmark == 0) {
		THROW(SLCHECK_NZ(F.Init(MakeInputFilePath("shakespeare.txt"))));
		bm = 0;
	}
	else if(sstreqi_ascii(pBenchmark, "sais"))
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "divsufsort"))
		bm = 2;
	//SFile f_inp(MakeInputFilePath("shakespeare.txt"), SFile::mRead|SFile::mBinary);
	//STempBuffer in_buf(SKILOBYTE(16));
	//int32 * sfxarray_sais = 0;
	//int32 * sfxarray_divsufsort = 0;
	//size_t in_buf_size = 0;
	
	//f_inp.CalcSize(&fsize);
	//size_t size = ftell(fp);
	//fseek(fp, 0, SEEK_SET);
	
	//THROW(SLCHECK_NZ(f_inp.IsValid()));
	//THROW(SLCHECK_NZ(in_buf.IsValid()));
	//THROW(SLCHECK_NZ(f_inp.ReadAll(in_buf, 0, &in_buf_size)));
	//assert(in_buf.GetSize() >= in_buf_size);
	if(bm == 0) {
		//sfxarray_sais = new int32[in_buf_size];
		THROW(SLCHECK_Z(libsais(F.InBuf.ucptr(), (int32_t *)F.SfxArray_Sais, F.InBufSize, 0, 0/*freq*/)));
		//sfxarray_divsufsort = new int32[in_buf_size];
		THROW(SLCHECK_Z(divsufsort(F.InBuf.ucptr(), (int *)F.SfxArray_DivSufSort, F.InBufSize, 0)));
		THROW(SLCHECK_Z(memcmp(F.SfxArray_Sais, F.SfxArray_DivSufSort, F.InBufSize * sizeof(int32))));
		{
			const char * p_pattern = "trophies";
			LongArray pos_list_fallback;
			LongArray pos_list;
			SaIndex saidx(F.InBuf, F.InBufSize);
			saidx.Text.Utf8ToLower();
			THROW(SLCHECK_NZ(saidx.Build()));
			uint cf = saidx.Search_fallback(p_pattern, &pos_list_fallback);
			uint c = saidx.Search(p_pattern, &pos_list);
			pos_list_fallback.sort();
			pos_list.sort();
			SLCHECK_LE(0U, cf);
			SLCHECK_EQ(cf, c);
			SLCHECK_NZ(pos_list.IsEq(&pos_list_fallback));
		}
	}
	else if(bm == 1) {
		memzero(F.SfxArray_Sais, F.InBufSize * sizeof(int));
		THROW(SLCHECK_Z(libsais(F.InBuf.ucptr(), (int32_t *)F.SfxArray_Sais, F.InBufSize, 0, 0/*freq*/)));
	}
	else if(bm == 2) {
		memzero(F.SfxArray_DivSufSort, F.InBufSize);
		THROW(SLCHECK_Z(divsufsort(F.InBuf.ucptr(), (int *)F.SfxArray_DivSufSort, F.InBufSize, 0)));
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
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
		SFile f_inp(MakeInputFilePath("colorset-imgui-test.json"), SFile::mRead|SFile::mBinary);
		THROW(SLCHECK_NZ(f_inp.IsValid()));
		{
			{
				STempBuffer in_buf(4096);
				size_t actual_size = 0;
				THROW(SLCHECK_NZ(in_buf.IsValid()));
				THROW(SLCHECK_NZ(f_inp.ReadAll(in_buf, 0, &actual_size)));
				temp_buf.Z().CatN(in_buf, actual_size);
			}
			{
				p_js = SJson::Parse(temp_buf);
				THROW(SLCHECK_NZ(p_js));
				SLCHECK_NZ(uid.FromJsonObj(p_js));
			}
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
		}
	}
	{
		SFile f_inp2(MakeOutputFilePath("colorset-imgui-test-reverse.json"), SFile::mRead|SFile::mBinary);
		THROW(SLCHECK_NZ(f_inp2.IsValid()));
		{
			STempBuffer in_buf(4096);
			size_t actual_size = 0;
			THROW(SLCHECK_NZ(in_buf.IsValid()));
			THROW(SLCHECK_NZ(f_inp2.ReadAll(in_buf, 0, &actual_size)));
			temp_buf.Z().CatN(in_buf, actual_size);
		}
		{
			p_js2 = SJson::Parse(temp_buf);
			THROW(SLCHECK_NZ(p_js2));
			SLCHECK_NZ(uid2.FromJsonObj(p_js2));
			SLCHECK_NZ(uid2.IsEq(uid));
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	delete p_js;
	delete p_js2;
	delete p_js_reverse;
	return CurrentStatus;
}

SLTEST_R(WinToken)
{
	SetInfo("@construction");
	/*
	HANDLE h = SSystem::GetLocalSystemProcessToken();
	SSystem::WinUserBlock wub;
	uint   guhf = 0;
	BOOL   loaded_profile = false;
	PROFILEINFO profile_info;
	HANDLE h_cmd_pipe = 0;
	wub.UserName = _T("petroglif\\sobolev");
	wub.Password = _T("AntonSobolev1969");
	bool guhr = SSystem::GetUserHandle(wub, guhf, loaded_profile, profile_info, h_cmd_pipe);
	*/
	return CurrentStatus;
}
