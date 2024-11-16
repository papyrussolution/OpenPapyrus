// OBJGS.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
//
// Descr: Внутренний блок хранения строки товарной структуры
//
struct _GSItem {           // @persistent @store(ObjAssocTbl)
	PPID   ID;             //
	PPID   Tag;            // Const=PPASS_GOODSSTRUC
	PPID   GSID;           // ->Ref(PPOBJ_GOODSSTRUC) ID структуры
	PPID   ItemGoodsID;    // ->Goods.ID              ID элемента структуры
	long   Num;            // Внутренний номер (не используется, но инициализируется)
	double Netto;          // Нетто количество компонента
	char   Symb[20];       // Символ элемента структуры (для ссылки из формул)
	// @v12.0.6 char   Reserve1[4];    // @v8.6.5 [8]-->[4]
	PPID   AccSheetID;     // @v12.0.6 Таблица аналитический статей для указания статьи в поле ItemGoodsID (для PPGoodsStruc::kPricePlanning)
	long   PrefInnefGsID;  // @v8.6.5
	long   Flags;          // Флаги
	double Median;         // Среднее значение
	double Width;          // Ширина оценочного интервала
	double Denom;          // Знаменатель дроби qtty = Median / Denom
};
//
//
//
PPGoodsStruc::Ident::Ident(PPID goodsID, long andF, long notF, LDATE dt) : GoodsID(goodsID), AndFlags(andF), NotFlags(notF), Dt(dt), Options(0)
{
}

PPGoodsStruc::PPGoodsStruc() : GoodsID(0), P_Cb(0)
{
}

PPGoodsStruc::PPGoodsStruc(const PPGoodsStruc & rS) : GoodsID(0), P_Cb(0)
{
	Copy(rS);
}

void PPGoodsStruc::Init()
{
	GoodsID = 0;
	P_Cb = 0;
	MEMSZERO(Rec);
	Items.clear();
	Children.freeAll();
}

PPGoodsStruc & FASTCALL PPGoodsStruc::operator = (const PPGoodsStruc & rS) { return Copy(rS); }
bool   PPGoodsStruc::IsEmpty() const { return (!Items.getCount() && !Children.getCount()); }
bool   PPGoodsStruc::IsNamed() const { return LOGIC(Rec.Flags & GSF_NAMED); }
bool   PPGoodsStruc::CanExpand() const { return (Rec.Flags & (GSF_CHILD|GSF_FOLDER)) ? false : true; }
bool   PPGoodsStruc::CanReduce() const { return (Rec.Flags & GSF_FOLDER && Children.getCount() <= 1); }
double PPGoodsStruc::GetDenom() const { return (Rec.CommDenom != 0.0 && Rec.CommDenom != 1.0) ? Rec.CommDenom : 1.0; }
int    PPGoodsStruc::MoveItem(uint pos, int dir  /* 0 - down, 1 - up */, uint * pNewPos) { return Items.moveItem(pos, dir, pNewPos); }
SString & PPGoodsStruc::MakeChildDefaultName(SString & rBuf) const
	{ return rBuf.Z().Cat("BOM").Space().CatChar('#').Cat(Children.getCount()+1); }
int    PPGoodsStruc::GetKind() const
	{ return PPGoodsStruc::GetStrucKind(Rec.Flags); }
SString & FASTCALL PPGoodsStruc::GetTypeString(SString & rBuf) const
	{ return PPGoodsStruc::MakeTypeString(Rec.ID, Rec.Flags, Rec.ParentID, rBuf); }

bool FASTCALL PPGoodsStruc::IsEq(const PPGoodsStruc & rS) const
{
	int   eq = true;
#define CMPRECF(f) if(Rec.f != rS.Rec.f) return false;
	CMPRECF(GiftLimit);
	CMPRECF(GiftQuotKindID);
	CMPRECF(GiftAmtRestrict);
	CMPRECF(VariedPropObjType);
	CMPRECF(Period);
	CMPRECF(CommDenom);
	CMPRECF(Flags);
	CMPRECF(ParentID);
#undef CMPRECF
	if(stricmp866(Rec.Name, rS.Rec.Name) != 0)
		eq = false;
	else if(stricmp866(Rec.Symb, rS.Rec.Symb) != 0)
		eq = false;
	else {
		{
			const uint c = Items.getCount();
			if(c != rS.Items.getCount())
				eq = false;
			else {
				for(uint i = 0; eq && i < c; i++) {
					if(!Items.at(i).IsEq(rS.Items.at(i)))
						eq = false;
				}
			}
		}
		if(eq) {
			const uint c = Children.getCount();
			if(c != rS.Children.getCount())
				eq = false;
			else {
				for(uint i = 0; eq && i < c; i++) {
					const PPGoodsStruc * p_child = Children.at(i);
					const PPGoodsStruc * p_s_child = rS.Children.at(i);
					if(p_child && p_s_child) {
						if(!p_child->IsEq(*p_s_child))
							eq = false;
					}
					else if(p_child && !p_s_child) // @paranoic
						eq = false;
					else if(!p_child && p_s_child) // @paranoic
						eq = false;
				}
			}
		}
	}
	return eq;
}

int PPGoodsStruc::SetKind(int kind)
{
	int    ok = 1;
	switch(kind) {
		case kUndef:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT|GSF_COMPLEX|GSF_PRICEPLANNING);
			break;
		case kBOM:
			Rec.Flags &= ~(GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT|GSF_COMPLEX|GSF_PRICEPLANNING);
			if(!(Rec.Flags & (GSF_COMPL|GSF_DECOMPL)))
				Rec.Flags |= GSF_COMPL;
			break;
		case kPart:
			Rec.Flags &= ~(GSF_SUBST|GSF_PRESENT|GSF_COMPLEX|GSF_PRICEPLANNING);
			Rec.Flags |= GSF_PARTITIAL;
			break;
		case kSubst:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_PRESENT|GSF_COMPLEX|GSF_PRICEPLANNING);
			Rec.Flags |= GSF_SUBST;
			break;
		case kGift:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_COMPLEX|GSF_PRICEPLANNING);
			Rec.Flags |= GSF_PRESENT;
			break;
		case kComplex:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT|GSF_PRICEPLANNING);
			Rec.Flags |= GSF_COMPLEX;
			break;
		case kPricePlanning: // @v12.0.6 @construction
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT);
			Rec.Flags |= GSF_PRICEPLANNING;
		default:
			ok = 0;
			break;
	}
	if(kind != PPGoodsStruc::kGift)
		Rec.Flags &= ~GSF_GIFTPOTENTIAL;
	if(kind != PPGoodsStruc::kBOM)
		Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL);
	return ok;
}

/*static*/int FASTCALL PPGoodsStruc::GetStrucKind(long flags)
{
	if(flags & GSF_COMPLEX)
		return kComplex;
	else if(flags & GSF_PRESENT)
		return kGift;
	else if(flags & GSF_SUBST)
		return kSubst;
	else if(flags & GSF_PARTITIAL)
		return kPart;
	else if(flags & GSF_PRICEPLANNING) // @v12.0.6 @construction
		return kPricePlanning;
	else if(flags & (GSF_COMPL|GSF_DECOMPL))
		return kBOM;
	else
		return kUndef;
}

/*static*/SString & PPGoodsStruc::MakeTypeString(PPID strucID, long flags, PPID parentStrucID, SString & rBuf)
{
	rBuf.Z();
	if(strucID)
		rBuf.CatChar('E');
	if(flags & GSF_COMPL)
		rBuf.CatChar('C');
	if(flags & GSF_DECOMPL)
		rBuf.CatChar('D');
	if(flags & GSF_PARTITIAL)
		rBuf.CatChar('P');
	if(flags & GSF_SUBST)
		rBuf.CatChar('S');
	if(flags & GSF_PRESENT)
		rBuf.CatChar('G');
	if(parentStrucID)
		rBuf.CatChar('F');
	return rBuf;
}

int PPGoodsStruc::Select(const Ident & rIdent, PPGoodsStruc * pGs) const
{
	if(Rec.Flags & GSF_FOLDER) {
		for(uint i = 0; i < Children.getCount(); i++) {
			const PPGoodsStruc * p_child = Children.at(i);
			if(p_child && p_child->Select(rIdent, pGs)) // @recursion
				return 1;
		}
	}
	else {
		DateRange period = Rec.Period;
		if(!rIdent.Dt || period.Actualize(ZERODATE).CheckDate(rIdent.Dt)) {
			if(!rIdent.AndFlags || (Rec.Flags & rIdent.AndFlags) == rIdent.AndFlags) {
				if(!(Rec.Flags & rIdent.NotFlags)) {
					// ((Rec.Flags & rIdent.NotFlags) != rIdent.NotFlags)-->!(Rec.Flags & rIdent.NotFlags)
					if(pGs) {
						*pGs = *this;
						pGs->GoodsID = rIdent.GoodsID;
					}
					return 1;
				}
			}
		}
	}
	return 0;
}

int PPGoodsStruc::Helper_Select(const Ident & rIdent, TSCollection <PPGoodsStruc> & rList) const
{
	int    ok = -1;
	if(Rec.Flags & GSF_FOLDER) {
		for(uint i = 0; i < Children.getCount(); i++) {
			const PPGoodsStruc * p_child = Children.at(i);
			if(p_child) {
				int    r = p_child->Helper_Select(rIdent, rList); // @recursion
				THROW(r);
				if(r > 0)
					ok = 1;
			}
		}
	}
	else {
		DateRange period = Rec.Period;
		if(!rIdent.Dt || period.Actualize(ZERODATE).CheckDate(rIdent.Dt)) {
			int    suit_by_and_flags = 0;
			if(!rIdent.AndFlags)
				suit_by_and_flags = 1;
			else if(rIdent.Options & rIdent.oAnyOfAndFlags && (Rec.Flags & rIdent.AndFlags))
				suit_by_and_flags = 1;
			else if(!(rIdent.Options & rIdent.oAnyOfAndFlags) && (Rec.Flags & rIdent.AndFlags) == rIdent.AndFlags)
				suit_by_and_flags = 1;
			if(suit_by_and_flags && !(Rec.Flags & rIdent.NotFlags)) {
				// ((Rec.Flags & rIdent.NotFlags) != rIdent.NotFlags)-->!(Rec.Flags & rIdent.NotFlags)

				//
				// Защита от рекурсивных структур: если в списке уже есть структура с таким ID, то мы - в рекурсии.
				//
				bool   found = false;
				for(uint i = 0; !found && i < rList.getCount(); i++) {
					const PPGoodsStruc * p_item = rList.at(i);
					if(p_item && p_item->Rec.ID == Rec.ID)
						found = true;
				}
				if(!found) {
					PPGoodsStruc * p_new_item = new PPGoodsStruc;
					THROW_MEM(p_new_item);
					*p_new_item = *this;
					p_new_item->GoodsID = rIdent.GoodsID;
					THROW_SL(rList.insert(p_new_item));
					ok = 1;
				}
			}
		}
	}
	CATCH
		rList.freeAll();
		ok = 0;
	ENDCATCH
	return ok;
}

int PPGoodsStruc::Select(const Ident & rIdent, TSCollection <PPGoodsStruc> & rList) const
{
	rList.freeAll();
	return Helper_Select(rIdent, rList);
}

PPGoodsStruc & FASTCALL PPGoodsStruc::Copy(const PPGoodsStruc & rS)
{
	Init();
	GoodsID = rS.GoodsID;
	Rec = rS.Rec;
	Items.copy(rS.Items);
	for(uint i = 0; i < rS.Children.getCount(); i++) {
		PPGoodsStruc * p_child = new PPGoodsStruc;
		*p_child = *rS.Children.at(i);
		Children.insert(p_child);
	}
	return *this;
}

const PPGoodsStrucItem * PPGoodsStruc::GetMainItem(uint * pPos) const
{
	for(uint i = 0; i < Items.getCount(); i++) {
		if(Items.at(i).Flags & GSIF_MAINITEM) {
			ASSIGN_PTR(pPos, i);
			return &Items.at(i);
		}
	}
	return (PPSetError(PPERR_GSTRUCHASNTMAINC), nullptr);
}

int PPGoodsStruc::RecalcQttyByMainItemPh(double * pQtty) const
{
	int    ok = -1;
	if(pQtty) {
		const PPGoodsStrucItem * p_main_item = GetMainItem(0);
		if(p_main_item && *pQtty) {
			PPObjGoods goods_obj;
			double phuperu;
			if(goods_obj.GetPhUPerU(p_main_item->GoodsID, 0, &phuperu) > 0) {
				(*pQtty) *= phuperu;
				ok = 1;
			}
		}
	}
	return ok;
}

int PPGoodsStruc::GetEstimationPrice(uint itemIdx, PPID locID, double * pPrice, double * pTotalPrice, ReceiptTbl::Rec * pRec) const
{
	int    ok = -1;
	double p = 0.0;
	double t = 0.0;
	ReceiptTbl::Rec rec;
	if(itemIdx < Items.getCount()) {
		const PPGoodsStrucItem & r_item = Items.at(itemIdx);
		const  PPID loc_id = LConfig.Location;
		if(r_item.GoodsID) {
			int    r = 0;
			PPObjGoods goods_obj;
			const PPGoodsConfig & r_cfg = goods_obj.GetConfig();
			Goods2Tbl::Rec goods_rec;
			goods_obj.Fetch(r_item.GoodsID, &goods_rec);
			if(goods_rec.Flags & GF_UNLIM) {
				r = goods_obj.GetQuot(r_item.GoodsID, QuotIdent(loc_id, PPQUOTK_BASE), 0, 0, &p, 1);
			}
			else if(::GetCurGoodsPrice(r_item.GoodsID, loc_id, GPRET_MOSTRECENT, &p, &rec) > 0) {
				p = (r_cfg.Flags & GCF_SHOWGSTRUCPRICE) ? rec.Price : rec.Cost;
				r = 1;
			}
			else {
				RAssocArray subst_list;
				if(goods_obj.GetSubstList(r_item.GoodsID, 0, subst_list) > 0) {
					for(uint i = 0; !r && i < subst_list.getCount(); i++) {
						const  PPID alt_goods_id = subst_list.at(i).Key;
						if(::GetCurGoodsPrice(alt_goods_id, loc_id, GPRET_MOSTRECENT, &p, &rec) > 0) {
							const double ratio = subst_list.at(i).Val;
							const double temp_p = (r_cfg.Flags & GCF_SHOWGSTRUCPRICE) ? rec.Price : rec.Cost;
							p = (ratio > 0.0) ? (temp_p * ratio) : temp_p;
							r = 1;
						}
					}
				}
			}
			if(r > 0) {
				double phuperu;
				r_item.GetQtty(p / GetDenom(), &t);
				if(r_item.Flags & GSIF_PHUVAL && goods_obj.GetPhUPerU(r_item.GoodsID, 0, &phuperu) > 0)
					p = p / phuperu;
				ok = 1;
			}
		}
	}
	else
		ok = PPSetErrorInvParam();
	ASSIGN_PTR(pRec, rec);
	ASSIGN_PTR(pPrice, p);
	ASSIGN_PTR(pTotalPrice, t);
	return ok;
}

void PPGoodsStruc::CalcEstimationPrice(PPID locID, double * pPrice, int * pUncertainty, int calcInner) const
{
	bool   uncertainty = false;
	double price = 0.0;
	for(uint i = 0; i < Items.getCount(); i++) {
		PPGoodsStrucItem * p_item = & Items.at(i);
		double iprice = 0.0;
		double tprice = 0.0;
		bool   is_inner_struc = false;
		if(calcInner) {
			PPGoodsStruc inner_struc;
			if(LoadGoodsStruc(PPGoodsStruc::Ident(p_item->GoodsID, GSF_COMPL, GSF_PARTITIAL|GSF_SUBST), &inner_struc) > 0) {
				int    uncert = 0;
				inner_struc.CalcEstimationPrice(locID, &iprice, &uncert, 1); // @recursion
				p_item->GetQtty(iprice / GetDenom(), &iprice);
				price += iprice;
				if(uncert)
					uncertainty = true;
				SETFLAG(p_item->Flags, GSIF_UNCERTPRICE, uncert);
				is_inner_struc = true;
			}
		}
		if(!is_inner_struc) {
			if(GetEstimationPrice(i, locID, &iprice, &tprice, 0) > 0 && tprice > 0.0) {
				price += R2(tprice);
				p_item->Flags &= ~GSIF_UNCERTPRICE;
			}
			else {
				p_item->Flags |= GSIF_UNCERTPRICE;
				uncertainty = true;
			}
		}
	}
	ASSIGN_PTR(pPrice, price);
	ASSIGN_PTR(pUncertainty, uncertainty);
}

int FASTCALL PPGoodsStruc::HasGoods(PPID goodsID) const
{
	int    ok = 0;
	PPGoodsStrucItem * p_item;
	if(goodsID) {
		for(uint i = 0; !ok && Items.enumItems(&i, (void **)&p_item);) {
			if(p_item->GoodsID == goodsID) {
				ok = static_cast<int>(i);
			}
		}
	}
	return ok;
}

int PPGoodsStruc::SearchSymb(const char * pSymb, uint * pPos) const
{
	int    ok = 0;
	PPGoodsStrucItem * p_item;
	if(pSymb)
		for(uint i = 0; !ok && Items.enumItems(&i, (void **)&p_item);)
			if(stricmp(p_item->Symb, pSymb) == 0) {
				ASSIGN_PTR(pPos, i-1);
				ok = 1;
			}
	return ok;
}

int PPGoodsStruc::CopyItemsFrom(const PPGoodsStruc * pS)
{
	int    ok = 1;
	if(pS) {
		PPGoodsStrucItem * p_item;
		for(uint i = 0; ok && pS->Items.enumItems(&i, (void **)&p_item);)
			if(!Items.insert(p_item))
				ok = PPSetErrorSLib();
	}
	else
		ok = -1;
	return ok;
}

int PPGoodsStruc::SubstVariedProp(PPID parentGoodsID, PPGoodsStrucItem * pItem) const
{
	int    ok = -1;
	if(Rec.VariedPropObjType && pItem->GoodsID && parentGoodsID) {
		PPObjGoods goods_obj;
		Goods2Tbl::Rec goods_rec;
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		GoodsExtTbl::Rec gext_rec;
		PPID   real_prop_id = 0;
		if(goods_obj.Fetch(parentGoodsID, &goods_rec) > 0 && goods_rec.GdsClsID) {
			if(gc_obj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0 && goods_obj.P_Tbl->GetExt(parentGoodsID, &gext_rec) > 0) {
				if(gc_pack.PropKind.ItemsListID == Rec.VariedPropObjType)
					real_prop_id = gext_rec.KindID;
				else if(gc_pack.PropGrade.ItemsListID == Rec.VariedPropObjType)
					real_prop_id = gext_rec.GradeID;
				else if(gc_pack.PropAdd.ItemsListID == Rec.VariedPropObjType)
					real_prop_id = gext_rec.AddObjID;
				else if(gc_pack.PropAdd2.ItemsListID == Rec.VariedPropObjType)
					real_prop_id = gext_rec.AddObj2ID;
			}
		}
		if(real_prop_id && goods_obj.Fetch(pItem->GoodsID, &goods_rec) > 0 && goods_rec.GdsClsID) {
			int do_replace_kind  = 0;
			int do_replace_grade = 0;
			int do_replace_add   = 0;
			int do_replace_add2  = 0;
			if(goods_obj.P_Tbl->GetExt(pItem->GoodsID, &gext_rec) > 0 && gc_obj.Fetch(goods_rec.GdsClsID, &gc_pack) > 0) {
				if(!(gc_pack.Rec.Flags & PPGdsCls::fDupCombine)) {
					if(gc_pack.PropKind.ItemsListID == Rec.VariedPropObjType)
						do_replace_kind = 1;
					if(gc_pack.PropGrade.ItemsListID == Rec.VariedPropObjType)
						do_replace_grade = 1;
					if(gc_pack.PropAdd.ItemsListID == Rec.VariedPropObjType)
						do_replace_add = 1;
					if(gc_pack.PropAdd2.ItemsListID == Rec.VariedPropObjType)
						do_replace_add2 = 1;
				}
			}
			if(do_replace_kind || do_replace_grade || do_replace_add || do_replace_add2) {
				if(goods_obj.P_Tbl->GetExt(pItem->GoodsID, &gext_rec) > 0) {
					int    r = 0;
					PPID   new_goods_id = 0;
					GoodsExtTbl::Rec gext_rec2 = gext_rec;
					gext_rec2.GoodsID = 0;
					if(do_replace_kind)
						gext_rec2.KindID = real_prop_id;
					if(do_replace_grade)
						gext_rec2.GradeID = real_prop_id;
					if(do_replace_add)
						gext_rec2.AddObjID = real_prop_id;
					if(do_replace_add2)
						gext_rec2.AddObj2ID = real_prop_id;
					THROW(r = goods_obj.GetGoodsByExt(&gext_rec2, &new_goods_id, 1, 1));
					if(r > 0) {
						pItem->GoodsID = new_goods_id;
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPGoodsStruc::GetItemExt(uint pos, PPGoodsStrucItem * pItem, PPID parentGoodsID, double srcQtty, double * pQtty) const
{
	int    ok = -1;
	if(pos < Items.getCount()) {
		double qtty = 0.0;
		PPGoodsStrucItem item = Items.at(pos);
		item.GetQtty(srcQtty / GetDenom(), &qtty);
		THROW(SubstVariedProp(parentGoodsID, &item));
		ASSIGN_PTR(pItem, item);
		ASSIGN_PTR(pQtty, qtty);
		ok = item.Formula__[0] ? 2 : 1;
	}
	CATCHZOK
	return ok;
}

int PPGoodsStruc::EnumItemsExt(uint * pPos, PPGoodsStrucItem * pItem, PPID parentGoodsID, double srcQtty, double * pQtty) const
{
	uint   p = DEREFPTRORZ(pPos);
	int    ok = GetItemExt(p, pItem, parentGoodsID, srcQtty, pQtty);
	if(ok > 0) {
		p++;
		ASSIGN_PTR(pPos, p);
	}
	return ok;
}

int PPGoodsStruc::Expand()
{
	int    ok = 1;
	int    restore_from_backup = 0;
	PPGoodsStruc backup;
	THROW_PP(!(Rec.Flags & GSF_CHILD), PPERR_EXPANDCHILDGSTRUC);
	if(!(Rec.Flags & GSF_FOLDER)) {
		backup = *this;
		restore_from_backup = 1;
		PPGoodsStruc * p_child = new PPGoodsStruc;
		THROW_MEM(p_child);
		p_child->Rec.Flags |= GSF_CHILD;
		p_child->Rec.Flags |= (Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_OUTPWOVAT));
		p_child->Rec.Period = Rec.Period;
		p_child->Rec.CommDenom = Rec.CommDenom;
		p_child->Rec.ParentID = Rec.ID; // may be ZERO
		p_child->Rec.VariedPropObjType = Rec.VariedPropObjType;

		Rec.Flags |= GSF_FOLDER;
		Rec.Period.Z();
		Rec.CommDenom = 0;
		Rec.ParentID = 0;
		THROW(p_child->CopyItemsFrom(this));
		if(p_child->Rec.Name[0] == 0) {
			SString name_buf;
			MakeChildDefaultName(name_buf).CopyTo(p_child->Rec.Name, sizeof(p_child->Rec.Name));
		}
		Items.freeAll();
		Children.insert(p_child);
	}
	else
		ok = -1;
	CATCH
		ok = 0;
		if(restore_from_backup)
			*this = backup;
	ENDCATCH
	return ok;
}

int PPGoodsStruc::Reduce()
{
	int    ok = 1;
	if(Rec.Flags & GSF_FOLDER && !(Rec.Flags & GSF_CHILD)) {
		PPGoodsStruc * p_child = Children.getCount() ? Children.at(0) : 0;
		THROW_PP(Children.getCount() <= 1, PPERR_REDUCEMULTGSTRUCFOLDER);
		if(p_child) {
			Rec.Flags |= (p_child->Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_OUTPWOVAT));
			Rec.Period = p_child->Rec.Period;
			Rec.CommDenom = p_child->Rec.CommDenom;
			Rec.VariedPropObjType = p_child->Rec.VariedPropObjType;
			THROW(CopyItemsFrom(p_child));
		}
		Children.freeAll();
		Rec.Flags &= ~GSF_FOLDER;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPGoodsStruc::GetItemQtty(PPID goodsID, double complQtty, double * pQtty) const
{
	for(uint i = 0; i < Items.getCount(); i++) {
		const PPGoodsStrucItem & r_item = Items.at(i);
		if(r_item.GoodsID == goodsID)
			return r_item.GetQtty(complQtty, pQtty);
	}
	PPSetAddedMsgObjName(PPOBJ_GOODS, goodsID);
	return PPSetError(PPERR_GSTRUCHASNTGOODS);
}

static int IsNumber(const char * pStr, size_t * pPos)
{
	int    was_sign = 0;
	int    was_dot = 0;
	size_t pos = DEREFPTRORZ(pPos);
	SString temp_buf;
	while(oneof2(pStr[pos], ' ', '\t'))
		pos++;
	if(oneof2(pStr[pos], '-', '+')) {
		if(was_sign) {
			ASSIGN_PTR(pPos, pos);
			return BIN(temp_buf.ToReal() != 0.0);
		}
		was_sign = 1;
		temp_buf.CatChar(pStr[pos++]);
	}
	while(oneof2(pStr[pos], ' ', '\t'))
		pos++;
	while(isdec(pStr[pos]) || oneof2(pStr[pos], '.', ',')) {
		if(oneof2(pStr[pos], '.', ',')) {
			if(was_dot) {
				ASSIGN_PTR(pPos, pos);
				return BIN(temp_buf.ToReal() != 0.0);
			}
			was_dot = 1;
		}
		temp_buf.CatChar(pStr[pos++]);
	}
	ASSIGN_PTR(pPos, pos);
	return BIN(temp_buf.ToReal() != 0.0);
}

/*static*/int FASTCALL PPGoodsStruc::IsSimpleQttyString(const char * pStr)
{
	size_t pos = 0;
	if(IsNumber(pStr, &pos)) {
		while(oneof2(pStr[pos], ' ', '\t'))
			pos++;
		if(pStr[pos] == 0)
			return 1;
		else if(pStr[pos] == '/') {
			pos++;
			if(IsNumber(pStr, &pos)) {
				while(oneof2(pStr[pos], ' ', '\t'))
					pos++;
				if(pStr[pos] == 0)
					return 1;
			}
		}
	}
	return 0;
}
//
//
//
PPGoodsStrucHeader2::PPGoodsStrucHeader2()
{
	THISZERO();
}

PPGoodsStrucItem::PPGoodsStrucItem()
{
	THISZERO();
}

int FASTCALL PPGoodsStrucItem::IsEq(const PPGoodsStrucItem & rS) const
{
#define CMPF(f) if(f != rS.f) return 0;
	CMPF(GoodsID);
	CMPF(Flags);
	CMPF(Median);
	CMPF(Width);
	CMPF(Denom);
	CMPF(Netto);
	CMPF(ObjType); // @v12.0.7
	CMPF(AccSheetID); // @v12.0.7
#undef CMPF
	if(!sstreq(Symb, rS.Symb))
		return 0;
	if(!sstreq(Formula__, rS.Formula__))
		return 0;
	return 1;
}

int FASTCALL PPGoodsStrucItem::operator == (const PPGoodsStrucItem & rS) const { return IsEq(rS); }
int FASTCALL PPGoodsStrucItem::operator != (const PPGoodsStrucItem & rS) const { return !IsEq(rS); }

int PPGoodsStrucItem::SetFormula(const char * pStr, const PPGoodsStruc * pStruc)
{
	int    ok = 1;
	double v = 0.0;
	memzero(Formula__, sizeof(Formula__));
	SString temp_buf(pStr);
	if(temp_buf.NotEmptyS()) {
		GdsClsCalcExprContext ctx(pStruc);
		if(PPCalcExpression(temp_buf, &v, &ctx))
			STRNSCPY(Formula__, temp_buf);
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int PPGoodsStrucItem::SetEstimationString(const char * pStr)
{
	int    ok = 1;
	Median = 0.0;
	Width  = 0.0;
	Denom  = 1.0;
	if(PPGoodsStruc::IsSimpleQttyString(pStr)) {
		double v;
		SString temp_buf(pStr);
		SString s1, s2;
		temp_buf.Divide('/', s1, s2);
		strtodoub(s1, &v);
		Median = R6(v);
		if(s2.NotEmpty()) {
			strtodoub(s2, &v);
			Denom = R6(v);
			if(Denom == 0.0)
				Denom = 1.0;
		}
	}
	return ok;
}

SString & PPGoodsStrucItem::GetEstimationString(SString & rBuf, long format) const
{
	return PPGoodsStrucItem::MakeEstimationString(Median, Denom, rBuf, format);
}

int PPGoodsStrucItem::GetQttyAsPrice(double complPriceSum, double * pItemPrice) const
{
	int    ok = 1;
	double price = 0.0;
	if(Flags & GSIF_QTTYASPRICE) {
		price = complPriceSum * Median;
		if(Denom != 0.0 && Denom != 1.0)
			price /= Denom;
		price = R2(fdiv100r(price));
		ok = 1;
	}
	else
		ok = -1;
	ASSIGN_PTR(pItemPrice, price);
	return ok;
}

/*static*/int FASTCALL PPGoodsStrucItem::GetEffectiveQuantity(double complQtty, PPID goodsID, double median, double denom, long flags, double * pItemQtty)
{
	int    ok = 1;
	double qtty = complQtty * median;
	if(denom != 0.0 && denom != 1.0)
		qtty /= denom;
	if(flags & GSIF_PCTVAL)
		qtty = fdiv100r(qtty);
	else if(flags & GSIF_PHUVAL) {
		PPObjGoods goods_obj;
		double phuperu;
		if(goods_obj.GetPhUPerU(goodsID, 0, &phuperu) > 0)
			qtty /= phuperu;
	}
	else if(flags & GSIF_QTTYASPRICE) {
		qtty = 1.0;
		ok = 2;
	}
	if(flags & GSIF_ROUNDDOWN)
		qtty = floor(qtty);
	ASSIGN_PTR(pItemQtty, R6(qtty));
	return ok;
}

/*static*/SString & FASTCALL PPGoodsStrucItem::MakeEstimationString(double median, double denom, SString & rBuf, long format)
{
	long   fmt = NZOR(format, MKSFMTD(0, 6, NMBF_NOTRAILZ));
	rBuf.Z().Cat(median, fmt);
	if(denom != 0.0 && denom != 1.0)
		rBuf.Slash().Cat(denom, fmt);
	return rBuf;
}

int PPGoodsStrucItem::GetQtty(double complQtty, double * pItemQtty) const
{
	return GetEffectiveQuantity(complQtty, GoodsID, Median, Denom, Flags, pItemQtty);
}
//
//
//
class GoodsStrucSelectorDialog : public TDialog {
public:
	struct DataBlock {
		DataBlock(PPID namedGsID) : Mode(mNamedStruc), Flags(0), GsID(namedGsID), P_List(0)
		{
		}
		DataBlock(const TSCollection <PPGoodsStruc> * pList, PPID selectionID) : Mode(mList), Flags(0), GsID(selectionID), P_List(pList)
		{
		}
		enum {
            mNamedStruc = 1,
            mList       = 2
		};
		int    Mode;
		long   Flags;
		PPID   GsID;
		SString DlgTitle;
		SString SelTitle;
		const  TSCollection <PPGoodsStruc> * P_List;
	};

	GoodsStrucSelectorDialog(PPObjGoodsStruc & rObj) : TDialog(DLG_GSDATA), R_GsObj(rObj), Data(0)
	{
	}
	int    setDTS(const DataBlock * pData)
	{
		Data = *pData;
		int    ok = 1;
		setTitle(Data.DlgTitle);
		if(Data.SelTitle.NotEmptyS())
			setLabelText(CTL_GSDATA_NAMEDSTRUC, Data.SelTitle);
        if(Data.Mode == DataBlock::mNamedStruc) {
			SetupPPObjCombo(this, CTLSEL_GSDATA_NAMEDSTRUC, PPOBJ_GOODSSTRUC, Data.GsID, 0, 0);
        }
        else if(Data.Mode == DataBlock::mList) {
        	if(Data.P_List) {
				ComboBox * p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_GSDATA_NAMEDSTRUC));
				if(p_cb) {
					StrAssocArray * p_list = new StrAssocArray;
					for(uint i = 0; i < Data.P_List->getCount(); i++) {
						const PPGoodsStruc * p_gs = Data.P_List->at(i);
						if(p_gs) {
							p_list->Add(p_gs->Rec.ID, p_gs->Rec.Name);
						}
					}
					p_cb->setListWindow(CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify), Data.GsID);
				}
        	}
        	else
				ok = 0;
        }
        else
			ok = 0;
        return ok;
	}
	int    getDTS(DataBlock * pData)
	{
		int    ok = 1;
		if(pData)
			pData->GsID = getCtrlLong(CTLSEL_GSDATA_NAMEDSTRUC);
        return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmShowGoodsStruc)) {
			PPID   gs_id = getCtrlLong(CTLSEL_GSDATA_NAMEDSTRUC);
			if(gs_id) {
				if(Data.Mode == DataBlock::mNamedStruc) {
					PPGoodsStruc gs;
					if(R_GsObj.Get(gs_id, &gs) > 0) {
						PPObjGoodsStruc::EditDialog(&gs, 0);
					}
				}
				else if(Data.Mode == DataBlock::mList && Data.P_List) {
					for(uint i = 0; i < Data.P_List->getCount(); i++) {
						const PPGoodsStruc * p_gs = Data.P_List->at(i);
						if(p_gs && p_gs->Rec.ID == gs_id) {
							PPGoodsStruc temp_gs(*p_gs);
							PPObjGoodsStruc::EditDialog(&temp_gs, 0);
							break;
						}
					}
				}
			}
			clearEvent(event);
		}
	}
	DataBlock Data;
	PPObjGoodsStruc & R_GsObj;
};

int PPObjGoodsStruc::Browse(void * extraPtr)
{
	class NamedGoodsStrucView : public ObjViewDialog {
	public:
		NamedGoodsStrucView(PPObjGoodsStruc * pObj) : ObjViewDialog(DLG_NGSVIEW, pObj, 0)
		{
		}
	private:
		virtual void extraProc(long id)
		{
			if(id) {
				PPObjGoodsStruc * p_gsobj = static_cast<PPObjGoodsStruc *>(P_Obj);
				PPGoodsStrucHeader gsh;
				if(p_gsobj->Search(id, &gsh) > 0) {
					PPObjGoods goods_obj;
					PPIDArray goods_list;
					goods_obj.SearchGListByStruc(id, false/*expandGenerics*/, goods_list);
					if(goods_list.getCount()) {
						GoodsFilt flt;
						flt.GoodsStrucID = id;
						PPView::Execute(PPVIEW_GOODS, &flt, 1, 0);
					}
				}
			}
		}
	};
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		NamedGoodsStrucView * dlg = new NamedGoodsStrucView(this);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
		else
			ok = 0;
	}
	else
		ok = PPErrorZ();
	return ok;
}

int PPObjGoodsStruc::SelectorDialog(const TSCollection <PPGoodsStruc> & rList, uint * pSelectionPos)
{
	int    ok = -1;
	uint   pos = 0;
	GoodsStrucSelectorDialog * dlg = new GoodsStrucSelectorDialog(*this);
	if(CheckDialogPtrErr(&dlg)) {
		PPID   init_id = 0;
		if(pSelectionPos && *pSelectionPos < rList.getCount())
			init_id = rList.at(*pSelectionPos)->Rec.ID;
		GoodsStrucSelectorDialog::DataBlock blk(&rList, init_id);
		PPLoadText(PPTXT_LAB_SELECTGSTRUC, blk.SelTitle);
		dlg->setDTS(&blk);
		if(ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&blk) && blk.GsID) {
                for(uint i = 0; ok < 0 && i < rList.getCount(); i++) {
                	const PPGoodsStruc * p_gs = rList.at(i);
					if(p_gs && p_gs->Rec.ID == blk.GsID) {
						pos = i;
						ok = 1;
					}
                }
			}
		}
		delete dlg;
	}
	ASSIGN_PTR(pSelectionPos, pos);
	return ok;
}

int PPObjGoodsStruc::SelectorDialog(PPID * pNamedGsID)
{
	int    ok = -1;
	GoodsStrucSelectorDialog * dlg = new GoodsStrucSelectorDialog(*this);
	if(CheckDialogPtrErr(&dlg)) {
		GoodsStrucSelectorDialog::DataBlock blk(*pNamedGsID);
		dlg->setDTS(&blk);
		if(ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&blk)) {
				ASSIGN_PTR(pNamedGsID, blk.GsID);
				ok = 1;
			}
		}
		delete dlg;
	}
	return ok;
}
//
// GSDialog
//
struct GoodsStrucCopyParam {
	GoodsStrucCopyParam() : GoodsGrpID(0), GoodsID(0), GStrucID(0)
	{
	}
	PPID   GoodsGrpID;
	PPID   GoodsID;
	PPID   GStrucID;
};

class GSDialog : public PPListDialog {
	DECL_DIALOG_DATA(PPGoodsStruc);
	PPObjGoodsStruc GsObj;
	PPObjGoods      GObj;
	PPID   NewGoodsGrpID;
	GoodsStrucCopyParam GscParam;
	PPGoodsStruc RecurData;
	int    Changed;
public:
	GSDialog() : PPListDialog(DLG_GSTRUC, CTL_GSTRUC_LIST), NewGoodsGrpID(0), Changed(0)
	{
		SetupCalPeriod(CTLCAL_GSTRUC_PERIOD, CTL_GSTRUC_PERIOD);
		if(SmartListBox::IsValidS(P_Box))
			P_Box->P_Def->SetOption(lbtFocNotify, 1);
		updateList(-1);
	}
	DECL_DIALOG_SETDTS()
	{
		SString temp_buf;
		if(pData->GoodsID)
			GetGoodsName(pData->GoodsID, temp_buf);
		else if(pData->Rec.ID) {
			PPID   goods_id = 0;
			if(GObj.P_Tbl->SearchAnyRef(PPOBJ_GOODSSTRUC, pData->Rec.ID, &goods_id) > 0)
				GetGoodsName(goods_id, temp_buf);
		}
		setCtrlString(CTL_GSTRUC_GNAME, temp_buf);
		Data = *pData;
		temp_buf.Z();
		if(Data.Rec.Flags & GSF_DYNGEN)
			PPLoadText(PPTXT_DYNGENGSTRUC, temp_buf);
		setStaticText(CTL_GSTRUC_INFO, temp_buf);
		setCtrlData(CTL_GSTRUC_NAME, Data.Rec.Name);
		setCtrlLong(CTL_GSTRUC_ID, Data.Rec.ID);
		SetupObjListCombo(this, CTLSEL_GSTRUC_VAROBJ, Data.Rec.VariedPropObjType);
		setCtrlData(CTL_GSTRUC_COMMDENOM, &Data.Rec.CommDenom);
		SetPeriodInput(this, CTL_GSTRUC_PERIOD, &Data.Rec.Period);

		const long kind = Data.GetKind();
		AddClusterAssocDef(CTL_GSTRUC_KIND, 0, PPGoodsStruc::kUndef);
		AddClusterAssoc(CTL_GSTRUC_KIND, 1, PPGoodsStruc::kBOM);
		AddClusterAssoc(CTL_GSTRUC_KIND, 2, PPGoodsStruc::kPart);
		AddClusterAssoc(CTL_GSTRUC_KIND, 3, PPGoodsStruc::kSubst);
		AddClusterAssoc(CTL_GSTRUC_KIND, 4, PPGoodsStruc::kGift);
		AddClusterAssoc(CTL_GSTRUC_KIND, 5, PPGoodsStruc::kComplex);
		AddClusterAssoc(CTL_GSTRUC_KIND, 6, PPGoodsStruc::kPricePlanning); // @v12.0.6
		SetClusterData(CTL_GSTRUC_KIND, kind);

		AddClusterAssoc(CTL_GSTRUC_FLAGS, 0, GSF_COMPL);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 1, GSF_DECOMPL);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 2, GSF_OUTPWOVAT);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 3, GSF_GIFTPOTENTIAL);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 4, GSF_OVRLAPGIFT);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 5, GSF_POSMODIFIER);
		AddClusterAssoc(CTL_GSTRUC_FLAGS, 6, GSF_AUTODECOMPL); // @v11.6.2
		SetClusterData(CTL_GSTRUC_FLAGS, Data.Rec.Flags);
		SetupCtrls();
		setCtrlReal(CTL_GSTRUC_GIFTAMTRESTR, Data.Rec.GiftAmtRestrict);
		{
			ComboBox * p_cb = static_cast<ComboBox *>(getCtrlView(CTLSEL_GSTRUC_GIFTQK));
			if(p_cb) {
				StrAssocArray * p_qk_list = new StrAssocArray;
				if(p_qk_list) {
					PPObjQuotKind qk_obj;
					QuotKindFilt qk_filt;
					qk_filt.Flags = QuotKindFilt::fAll;
					qk_obj.MakeList(&qk_filt, p_qk_list);
					PPLoadText(PPTXT_CHEAPESTITEMFREE, temp_buf);
					p_qk_list->Add(GSGIFTQ_CHEAPESTITEMFREE, temp_buf);
					PPLoadText(PPTXT_CHEAPESTITEMBYGIFTQ, temp_buf);
					p_qk_list->Add(GSGIFTQ_CHEAPESTITEMBYGIFTQ, temp_buf);
					PPLoadText(PPTXT_LASTITEMBYGIFTQ, temp_buf);
					p_qk_list->Add(GSGIFTQ_LASTITEMBYGIFTQ, temp_buf);
					p_cb->setListWindow(CreateListWindow(p_qk_list, lbtDisposeData|lbtDblClkNotify), Data.Rec.GiftQuotKindID);
				}
			}
		}
		setCtrlData(CTL_GSTRUC_GIFTLIMIT, &Data.Rec.GiftLimit);
		updateList(-1);
		enableCommand(cmGStrucExpandReduce, Data.CanExpand());
		enableEditRecurStruc();
		return 1;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0, i;
		SString temp_buf;
		getCtrlString(sel = CTL_GSTRUC_NAME, temp_buf);
		if(!temp_buf.NotEmptyS()) {
			THROW_PP(!(Data.Rec.Flags & GSF_NAMED), PPERR_NAMENEEDED);
		}
		else if(!(Data.Rec.Flags & GSF_CHILD))
			Data.Rec.Flags |= GSF_NAMED;
		STRNSCPY(Data.Rec.Name, temp_buf);
		getCtrlData(CTLSEL_GSTRUC_VAROBJ, &Data.Rec.VariedPropObjType);
		//
		// Если структура переведена в расширенный режим, то забирать данные надо не из всех контролов
		//
		if(!(Data.Rec.Flags & GSF_FOLDER)) {
			long   kind = PPGoodsStruc::kUndef;
			getCtrlData(CTL_GSTRUC_COMMDENOM, &Data.Rec.CommDenom);
			THROW(GetPeriodInput(this, sel = CTL_GSTRUC_PERIOD, &Data.Rec.Period));
			GetClusterData(CTL_GSTRUC_KIND, &kind);
			GetClusterData(CTL_GSTRUC_FLAGS, &Data.Rec.Flags);
			Data.SetKind(kind);
			if(GObj.GetConfig().Flags & GCF_BANSTRUCCDONDECOMPL) {
				if((Data.Rec.Flags & GSF_DECOMPL) && !(Data.Rec.Flags & GSF_COMPL))
					Data.Rec.CommDenom = 1.0;
			}
			for(i = 0; i < Data.Items.getCount(); i++)
				Data.Items.at(i).Flags &= ~0x8000L;
			if(Data.Rec.Flags & GSF_PRESENT) {
				Data.Rec.GiftAmtRestrict = getCtrlReal(CTL_GSTRUC_GIFTAMTRESTR);
				Data.Rec.GiftQuotKindID = getCtrlLong(CTLSEL_GSTRUC_GIFTQK);
				getCtrlData(CTL_GSTRUC_GIFTLIMIT, &Data.Rec.GiftLimit);
			}
			else {
				Data.Rec.GiftAmtRestrict = 0.0;
				Data.Rec.GiftQuotKindID = 0;
				Data.Rec.GiftLimit = 0.0f;
			}
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
	int    IsChanged();
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	virtual int  moveItem(long pos, long id, int up);
	void   SetupCtrls();
	int    addItemExt(long * pPos, long * pID);
	int    addItemBySample();
	int    editItemDialog(int pos, PPGoodsStrucItem *);
	int    checkDupGoods(int pos, const PPGoodsStrucItem *);
	void   selNamedGS();
	int    enableEditRecurStruc();
	void   ViewHierarchy();
};

void GSDialog::ViewHierarchy() // @v11.2.11
{
	class GoodsStrucHierarchyDialog : public TDialog {
	public:
		GoodsStrucHierarchyDialog(PPID goodsID, PPID strucID) : TDialog(DLG_GSTRUCTREE), P_Box(0)
		{
			P_Box = static_cast<SmartListBox *>(getCtrlView(CTL_GSTRUCTREE_LIST));
			if(P_Box) {
				Cb.AddItem(goodsID, strucID, 0, 0, GoodsStrucProcessingBlock::addifRecursive);
				GoodsStrucTreeListViewBlock vb(Cb);
				StrAssocTree * p_tree = vb.MakeTree();
				if(p_tree) {
					//PPListDialog a;
					StdTreeListBoxDef2_ * p_def = new StdTreeListBoxDef2_(p_tree, 0, 0);
					p_def->AddTopLevelRestrictionId(strucID); // @v11.4.0
					P_Box->setDef(p_def);
					P_Box->Draw_();
				}
			}
		}
	private:
		GoodsStrucProcessingBlock Cb;
		SmartListBox * P_Box;
	};
	GoodsStrucHierarchyDialog * dlg = new GoodsStrucHierarchyDialog(Data.GoodsID, Data.Rec.ID);
	ExecViewAndDestroy(dlg);
}

int GSDialog::IsChanged()
{
	PPGoodsStruc temp = Data;
	PPGoodsStruc gs;
	return NZOR(Changed, (getDTS(&gs), Data = temp, !temp.IsEq(gs)));
}

int GSDialog::enableEditRecurStruc()
{
	int    ok = 1, enable = 0;
	if(SmartListBox::IsValidS(P_Box)) {
		long   pos = 0;
		P_Box->P_Def->getCurID(&pos);
		if(pos > 0 && pos-1 < (long)Data.Items.getCount()) {
			int r = 0;
			RecurData.Init();
			THROW(r = GObj.LoadGoodsStruc(PPGoodsStruc::Ident(Data.Items.at(pos - 1).GoodsID, GSF_COMPL, GSF_PARTITIAL), &RecurData));
			enable = BIN(r > 0 && RecurData.Rec.ID);
		}
	}
	CATCHZOK
	enableCommand(cmEditStruct, enable);
	return ok;
}

void GSDialog::SetupCtrls()
{
	bool   lock_changes = false;
	const  long preserve_kind = Data.GetKind();
	const  long preserve_flags = Data.Rec.Flags;
	long   kind = PPGoodsStruc::kUndef;
	GetClusterData(CTL_GSTRUC_KIND, &kind);
	GetClusterData(CTL_GSTRUC_FLAGS, &Data.Rec.Flags);
	if(kind != preserve_kind && (kind == PPGoodsStruc::kPricePlanning || preserve_kind == PPGoodsStruc::kPricePlanning)) {
		if(Data.Items.getCount()) {
			SetClusterData(CTL_GSTRUC_KIND, preserve_kind);
			SetClusterData(CTL_GSTRUC_FLAGS, preserve_flags);
			lock_changes = true;
		}	 
	}
	if(!lock_changes) {
		Data.SetKind(kind);
		DisableClusterItem(CTL_GSTRUC_FLAGS, 0, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 1, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 2, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 3, !(Data.Rec.Flags & GSF_PRESENT));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 4, !(Data.Rec.Flags & GSF_PRESENT));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 5, !(Data.Rec.Flags & GSF_PARTITIAL));
		DisableClusterItem(CTL_GSTRUC_FLAGS, 6, !(Data.Rec.Flags & GSF_DECOMPL)); // @v11.6.2
		showCtrl(CTL_GSTRUC_GIFTAMTRESTR, (Data.Rec.Flags & GSF_PRESENT));
		showCtrl(CTLSEL_GSTRUC_GIFTQK,    (Data.Rec.Flags & GSF_PRESENT));
		showCtrl(CTL_GSTRUC_GIFTLIMIT,    (Data.Rec.Flags & GSF_PRESENT));
		SetClusterData(CTL_GSTRUC_FLAGS, Data.Rec.Flags);
		if(GObj.GetConfig().Flags & GCF_BANSTRUCCDONDECOMPL) {
			disableCtrl(CTL_GSTRUC_COMMDENOM, (Data.Rec.Flags & GSF_DECOMPL) && !(Data.Rec.Flags & GSF_COMPL));
		}
		if(P_Box) {
			if(kind == PPGoodsStruc::kPricePlanning) {
				P_Box->SetupColumns("@lbt_goodsstruc_pp");
			}
			else {
				P_Box->SetupColumns("@lbt_goodsstruc");
			}
		}
	}
}

IMPL_HANDLE_EVENT(GSDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isClusterClk(CTL_GSTRUC_KIND) || event.isClusterClk(CTL_GSTRUC_FLAGS)) {
		SetupCtrls();
		clearEvent(event);
	}
	else if(event.isCmd(cmSelNamedGS))
		selNamedGS();
	else if(event.isCmd(cmGStrucExpandReduce)) {
		if(Data.CanExpand() && getDTS(0)) {
			Changed = 1;
			if(Data.Expand()) {
				if(IsInState(sfModal)) {
					endModal(cmUtil);
					return; // После endModal не следует обращаться к this
				}
			}
			else
				PPError();
		}
	}
	else if(event.isCmd(cmAddBySample))
		Changed = (addItemBySample() > 0) ? 1 : Changed;
	else if(event.isCmd(cmEditStruct)) {
		if(RecurData.Rec.ID && PPObjGoodsStruc::EditDialog(&RecurData, 1) > 0) {
			int r = 0;
			if((r = GsObj.Put(&RecurData.Rec.ID, &RecurData, 1)) > 0)
				updateList(-1);
			else if(!r)
				PPError();
			Changed = 1;
		}
	}
	else if(event.isCmd(cmPrint))
		GsObj.Print(&Data);
	else if(event.isCmd(cmLBItemFocused)) {
		if(event.isCtlEvent(CtlList) && !enableEditRecurStruc())
			PPError();
	}
	else if(event.isCmd(cmTree)) { // @v11.2.11
		ViewHierarchy();
	}
	else if(event.isKeyDown(KB_CTRLENTER)) {
		if(IsInState(sfModal)) {
			endModal(cmOK);
			return; // После endModal не следует обращаться к this
		}
	}
	else if(event.isKeyDown(kbF7))
		GsObj.Print(&Data);
	else if(event.isKeyDown(kbF2))
		addItemExt(0, 0);
	else
		return;
	clearEvent(event);
}

void GSDialog::selNamedGS()
{
	// Дочерняя структура не может быть именованной
	if(!(Data.Rec.Flags & GSF_CHILD) && (Data.IsNamed() || Data.IsEmpty() || CONFIRM(PPCFM_SELNAMEDSTRUC))) {
		PPID   id = (Data.IsNamed() || Data.IsEmpty()) ? Data.Rec.ID : 0;
		if(GsObj.SelectorDialog(&id) > 0) {
			PPGoodsStruc temp;
			if(id == 0) {
				temp.GoodsID = Data.GoodsID;
				setDTS(&temp);
				Changed = 1;
			}
			else if(id != Data.Rec.ID) {
				if(GsObj.Get(id, &temp) > 0) {
					temp.GoodsID = Data.GoodsID;
					setDTS(&temp);
					Changed = 1;
				}
			}
		}
	}
}

int GSDialog::setupList()
{
	int    ok = 1;
	bool   show_total = false;
	double t_qtty = 0.0;
	double t_netto = 0.0;
	double t_sum = 0.0;
	PPGoodsStrucItem * p_item = 0;
	SString sub;
	long   qtty_fmt = MKSFMTD(0, 3, NMBF_NOZERO);
	long   money_fmt = MKSFMTD(0, 2, NMBF_NOZERO);
	int    uncert = 0; // @v11.8.2 Признак того, что расчетная оценка стоимости комплекта неполная
	uint   i = 0;
	const  PPID loc_id = LConfig.Location; // @v11.7.11
	PPObjArticle ar_obj;
	while(Data.Items.enumItems(&i, (void **)&p_item)) {
		double price = 0.0;
		double sum = 0.0;
		int    local_uncert = 0;
		Goods2Tbl::Rec goods_rec;
		StringSet ss(SLBColumnDelim);
		PPGoodsStruc inner_struc;
		THROW(GObj.LoadGoodsStruc(PPGoodsStruc::Ident(p_item->GoodsID, GSF_COMPL, GSF_PARTITIAL), &inner_struc));
		if(p_item->Flags & GSIF_ARTICLE) {
			ArticleTbl::Rec ar_rec;
			if(ar_obj.Fetch(p_item->GoodsID, &ar_rec) > 0) {
				sub = ar_rec.Name;
			}
			else {
				ideqvalstr(p_item->GoodsID, sub);
			}
		}
		else {
			if(GObj.Fetch(p_item->GoodsID, &goods_rec) > 0)
				sub = goods_rec.Name;
			else {
				MEMSZERO(goods_rec);
				ideqvalstr(p_item->GoodsID, sub);
			}
		}
		ss.add(sub);
		if(Data.GetKind() == PPGoodsStruc::kPricePlanning) {
			sub.Z().Cat(p_item->Formula__);
			ss.add(sub);
			sub.Z().Cat("?price?"); // @todo
			ss.add(sub);
		}
		else {
			p_item->GetEstimationString(sub, qtty_fmt);
			if(p_item->Flags & GSIF_PCTVAL)
				sub.CatChar('%');
			else if(p_item->Flags & GSIF_PHUVAL) {
				PPUnit u_rec;
				sub.Space();
				if(GObj.FetchUnit(goods_rec.PhUnitID, &u_rec) > 0)
					sub.Cat(u_rec.Name);
				else
					sub.CatChar('?');
			}
			ss.add(sub);
			t_qtty  += sub.ToReal();
			sub.Z();
			if(inner_struc.Rec.ID) {
				if(GsObj.CheckStruc(inner_struc.Rec.ID, 0) != 2) {
					inner_struc.CalcEstimationPrice(loc_id, &price, &local_uncert, 1); // @bottleneck
					p_item->GetQtty(price / Data.GetDenom(), &sum);
					if(local_uncert)
						uncert = 1;
				}
				else {
					price = 0.0;
					PPError(PPERR_CYCLEGOODSSTRUC);
				}
			}
			else {
				if(Data.GetEstimationPrice(i-1, loc_id, &price, &sum, 0) > 0 && sum > 0.0) {
					;
				}
				else {
					uncert = 1;
				}
			}
			t_netto += p_item->Netto;
			t_sum   += sum;
			ss.add(sub.Z().Cat(p_item->Netto, qtty_fmt));
			ss.add(sub.Z().Cat(price, money_fmt));
			{
				sub.Z();
				if(sum != 0.0 && local_uncert)
					sub.CatChar('?');
				ss.add(sub.Cat(sum,   money_fmt));
			}
		}
		sub.Z();
		if(inner_struc.Rec.ID)
			sub.CatChar('R');
		ss.add(sub);
		THROW(addStringToList(i, ss.getBuf()));
		show_total = true;
	}
	if(show_total) {
		StringSet ss(SLBColumnDelim);
		PPGetWord(PPWORD_TOTAL, 0, sub);
		ss.add(sub.ToLower());
		ss.add(sub.Z().Cat(t_qtty,  qtty_fmt));
		ss.add(sub.Z().Cat(t_netto, qtty_fmt));
		ss.add(0);
		{
			sub.Z();
			if(t_sum != 0.0 && uncert)
				sub.CatChar('?');
			ss.add(sub.Cat(t_sum, money_fmt));
		}
		THROW(addStringToList(i+1, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int GSDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		Data.Items.atFree(static_cast<uint>(pos));
		Changed = 1;
		return 1;
	}
	else
		return -1;
}

int GSDialog::moveItem(long pos, long id, int up)
{
	uint   new_pos = static_cast<uint>(pos);
	return Data.MoveItem(pos, up, &new_pos);
}
//
// Descr: Класс диалога редактирования структуры планирования цены товара
//
class GSPPItemDialog : public TDialog {
	enum {
		ctlgroupGoods = 1,
		ctlgroupArticle = 2
	};
	DECL_DIALOG_DATA(PPGoodsStrucItem);
	const PPGoodsStruc * P_Struc;
	PPID  PreserveObjType;
public:
	GSPPItemDialog(const PPGoodsStruc * pStruc) : TDialog(DLG_GSPPITEM), P_Struc(pStruc), PreserveObjType(0)
	{
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_GSITEM_GGRP, CTLSEL_GSITEM_GOODS));
		addGroup(ctlgroupArticle, new ArticleCtrlGroup(CTLSEL_GSITEM_GGRP, 0, CTLSEL_GSITEM_GOODS, 0, 0));
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		PreserveObjType = NZOR(Data.ObjType, PPOBJ_ARTICLE);
		AddClusterAssocDef(CTL_GSITEM_SELOBJTYPE, 0, PPOBJ_ARTICLE);
		AddClusterAssoc(CTL_GSITEM_SELOBJTYPE, 1, PPOBJ_GOODS);
		SetClusterData(CTL_GSITEM_SELOBJTYPE, Data.ObjType);
		SetupItemObject(true);
		temp_buf = Data.Formula__;
		setCtrlString(CTL_GSITEM_FORMULA, temp_buf);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		PPID   obj_type = GetClusterData(sel = CTL_GSITEM_SELOBJTYPE);
		THROW(oneof2(obj_type, PPOBJ_GOODS, PPOBJ_ARTICLE)); // @todo @err
		if(obj_type == PPOBJ_GOODS) {
			GoodsCtrlGroup::Rec rec;
			getGroupData(ctlgroupGoods, &rec);
			Data.ObjType = obj_type;
			Data.GoodsID = rec.GoodsID;
			Data.Flags &= ~GSIF_ARTICLE;
			THROW(Data.GoodsID); // @todo @err
		}
		else if(obj_type == PPOBJ_ARTICLE) {
			ArticleCtrlGroup::Rec rec;
			getGroupData(ctlgroupArticle, &rec);
			Data.ObjType = obj_type;
			Data.AccSheetID = rec.AcsID;
			Data.GoodsID = rec.ArList.GetSingle();
			Data.Flags |= GSIF_ARTICLE;
			THROW(Data.GoodsID); // @todo @err
		}
		{
			getCtrlString(sel = CTL_GSITEM_FORMULA, temp_buf);
			THROW(temp_buf.NotEmptyS()); // @todo @err
			STRNSCPY(Data.Formula__, temp_buf);
		}
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isClusterClk(CTL_GSITEM_SELOBJTYPE)) {
			SetupItemObject(false);
		}
		else
			return;
		clearEvent(event);
	}
	void SetupItemObject(bool onInit)
	{
		PPID obj_type = GetClusterData(CTL_GSITEM_SELOBJTYPE);
		SETIFZ(obj_type, PPOBJ_ARTICLE);
		if(onInit || obj_type != PreserveObjType) {
			if(obj_type == PPOBJ_GOODS) {
				//SetupPPObjCombo(this, CTLSEL_GSITEM_GGRP, )
				CtrlGroup * p_ctlgrp_goods = getGroup(ctlgroupGoods);
				CtrlGroup * p_ctlgrp_ar = getGroup(ctlgroupArticle);
				CALLPTRMEMB(p_ctlgrp_goods, SetActive());
				CALLPTRMEMB(p_ctlgrp_ar, SetPassive());
				{
					GoodsCtrlGroup::Rec rec;
					if(onInit) {
						rec.GoodsID = Data.GoodsID;
					}
					setGroupData(ctlgroupGoods, &rec);
				}
				PreserveObjType = obj_type;
			}
			else if(obj_type = PPOBJ_ARTICLE) {
				CtrlGroup * p_ctlgrp_goods = getGroup(ctlgroupGoods);
				CtrlGroup * p_ctlgrp_ar = getGroup(ctlgroupArticle);
				CALLPTRMEMB(p_ctlgrp_goods, SetPassive());
				CALLPTRMEMB(p_ctlgrp_ar, SetActive());
				{
					ArticleCtrlGroup::Rec rec;
					if(onInit) {
						rec.AcsID = Data.AccSheetID;
						rec.ArList.Add(Data.GoodsID);
					}
					setGroupData(ctlgroupArticle, &rec);
				}
				PreserveObjType = obj_type;
			}
			else {
				obj_type = PreserveObjType;
				SetClusterData(CTL_GSITEM_SELOBJTYPE, obj_type);
			}
		}
	}
};

class GSItemDialog : public TDialog {
	DECL_DIALOG_DATA(PPGoodsStrucItem);
	enum {
		ctlgroupGoods = 1
	};
	PPObjGoods GObj;
	const PPGoodsStruc * P_Struc;
	double Price;
	double NettBruttCoeff;
public:
	GSItemDialog(const PPGoodsStruc * pStruc) : TDialog(DLG_GSITEM), P_Struc(pStruc), Price(0.0), NettBruttCoeff(0.0)
	{
		disableCtrl(CTL_GSITEM_PRICE, 1);
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_GSITEM_GGRP, CTLSEL_GSITEM_GOODS));
		DisableClusterItem(CTL_GSITEM_UNITS, 3, (P_Struc->Rec.Flags & GSF_PARTITIAL) ? 0 : 1);
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		ushort v = 0;
		SString temp_buf;
		RVALUEPTR(Data, pData);
		long   goods_sel_flags = GoodsCtrlGroup::enableInsertGoods|GoodsCtrlGroup::disableEmptyGoods;
		setCtrlReal(CTL_GSITEM_NETTO, Data.Netto);
		setCtrlData(CTL_GSITEM_SYMB, Data.Symb);
		if(P_Struc) {
			const PPGoodsStrucItem * p_main_item = P_Struc->GetMainItem(0);
			if(p_main_item && p_main_item->GoodsID != Data.GoodsID && !(Data.Flags & GSIF_MAINITEM))
				DisableClusterItem(CTL_GSITEM_FLAGS, 2, 1);
			if(P_Struc->GetKind() != PPGoodsStruc::kGift) {
				disableCtrl(CTL_GSITEM_GROUPONLY, 1);
				Data.Flags &= ~GSIF_GOODSGROUP;
			}
		}
		else
			disableCtrl(CTL_GSITEM_GROUPONLY, 1);
		setCtrlUInt16(CTL_GSITEM_GROUPONLY, BIN(Data.Flags & GSIF_GOODSGROUP));
		if(Data.Flags & GSIF_GOODSGROUP) {
			goods_sel_flags &= ~GoodsCtrlGroup::disableEmptyGoods;
			goods_sel_flags |= GoodsCtrlGroup::enableSelUpLevel;
		}
		DisableClusterItem(CTL_GSITEM_FLAGS, 3, !P_Struc || !(P_Struc->Rec.Flags & GSF_PARTITIAL));
		GoodsCtrlGroup::Rec rec(0, Data.GoodsID, 0, goods_sel_flags);
		setGroupData(ctlgroupGoods, &rec);
		setCtrlString(CTL_GSITEM_VALUE, Data.GetEstimationString(temp_buf));
		temp_buf = Data.Formula__;
		setCtrlString(CTL_GSITEM_FORMULA, temp_buf);
		if(Data.Flags & GSIF_QTTYASPRICE)
			v = 3;
		else if(Data.Flags & GSIF_PCTVAL)
			v = 2;
		else if(Data.Flags & GSIF_PHUVAL)
			v = 1;
		else
			v = 0;
		setCtrlData(CTL_GSITEM_UNITS, &v);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 0, GSIF_ROUNDDOWN);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 1, GSIF_AUTOTSWROFF);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 2, GSIF_MAINITEM);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 3, GSIF_SUBPARTSTR);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 4, GSIF_IDENTICAL);
		AddClusterAssoc(CTL_GSITEM_FLAGS, 5, GSIF_QUERYEXPLOT);
		SetClusterData(CTL_GSITEM_FLAGS, Data.Flags);
		setupPrice();
		if(Data.GoodsID) {
			selectCtrl(CTL_GSITEM_VALUE);
			GoodsStockExt gse;
			if(GObj.P_Tbl->GetStockExt(Data.GoodsID, &gse, 1) > 0 && gse.NettBruttCoeff > 0.0f && gse.NettBruttCoeff <= 1.0f) {
				NettBruttCoeff = gse.NettBruttCoeff;
				temp_buf.Z().CatEq("netto/brutto", NettBruttCoeff, MKSFMTD(0, 6, NMBF_NOTRAILZ));
				setStaticText(CTL_GSITEM_NTBRTCOEF, temp_buf);
			}
		}
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;
		SString temp_buf;
		ushort v;
		GoodsCtrlGroup::Rec gc_rec;
		Goods2Tbl::Rec goods_rec;
		getGroupData(GSItemDialog::ctlgroupGoods, &gc_rec);
		sel = CTLSEL_GSITEM_GOODS;
		THROW_PP(Data.GoodsID && GObj.Fetch(Data.GoodsID, &goods_rec) > 0, (Data.Flags & GSIF_GOODSGROUP) ? PPERR_GOODSGROUPNEEDED : PPERR_GOODSNEEDED);
		getCtrlData(sel = CTL_GSITEM_UNITS, &(v = 0));
		Data.Flags &= ~(GSIF_PCTVAL | GSIF_PHUVAL | GSIF_QTTYASPRICE);
		if(v == 1)
			Data.Flags |= GSIF_PHUVAL;
		else if(v == 2)
			Data.Flags |= GSIF_PCTVAL;
		else if(v == 3)
			Data.Flags |= GSIF_QTTYASPRICE;
		THROW_PP(!(Data.Flags & GSIF_PHUVAL) || goods_rec.PhUPerU > 0.0, PPERR_PHUFORGOODSWOPHU); //TODO: V614 https://www.viva64.com/en/w/v614/ Uninitialized variable 'goods_rec.PhUPerU' used.
		GetClusterData(CTL_GSITEM_FLAGS, &Data.Flags);
		getCtrlString(sel = CTL_GSITEM_VALUE, temp_buf.Z());
		THROW(Data.SetEstimationString(temp_buf));
		getCtrlString(sel = CTL_GSITEM_FORMULA, temp_buf.Z());
		THROW(Data.SetFormula(temp_buf, P_Struc));
		getCtrlData(CTL_GSITEM_NETTO, &Data.Netto);
		getCtrlData(CTL_GSITEM_SYMB, Data.Symb);
		SETFLAG(Data.Flags, GSIF_GOODSGROUP, getCtrlUInt16(CTL_GSITEM_GROUPONLY));
		Data.GoodsID = (Data.Flags & GSIF_GOODSGROUP) ? gc_rec.GoodsGrpID : gc_rec.GoodsID;
		ASSIGN_PTR(pData, Data);
		CATCHZOKPPERRBYDLG
		return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCbSelected(CTLSEL_GSITEM_GOODS))
			setupPrice();
		else if(event.isClusterClk(CTL_GSITEM_GROUPONLY)) {
			SETFLAG(Data.Flags, GSIF_GOODSGROUP, getCtrlUInt16(CTL_GSITEM_GROUPONLY));
			GoodsCtrlGroup * p_grp = static_cast<GoodsCtrlGroup *>(getGroup(ctlgroupGoods));
			CALLPTRMEMB(p_grp, setFlag(this, GoodsCtrlGroup::disableEmptyGoods, BIN(!(Data.Flags & GSIF_GOODSGROUP))));
		}
		else if(event.isCmd(cmGSItemLots)) {
			GoodsCtrlGroup::Rec rec;
			getGroupData(ctlgroupGoods, &rec);
			if(rec.GoodsID)
				ViewLots(rec.GoodsID, 0, 0, 0, 0);
		}
		else if(event.isKeyDown(kbF2)) {
			if(isCurrCtlID(CTL_GSITEM_VALUE) || isCurrCtlID(CTL_GSITEM_NETTO)) {
				if(NettBruttCoeff > 0.0 && NettBruttCoeff <= 1.0) {
					SString temp_buf;
					double qtty = 0.0;
					double netto = 0.0;
					getCtrlData(CTL_GSITEM_NETTO, &netto);
					PPGoodsStrucItem item;
					getCtrlString(CTL_GSITEM_VALUE, temp_buf);
					if(netto > 0.0 && !temp_buf.NotEmptyS()) {
						item.Median = netto / NettBruttCoeff;
						item.Denom = 1.0;
						item.GetEstimationString(temp_buf);
						setCtrlString(CTL_GSITEM_VALUE, temp_buf);
					}
					else if(item.SetEstimationString(temp_buf) && item.GetQtty(1, &qtty) > 0) {
						if(qtty > 0.0 && netto <= 0.0) {
							netto = qtty * NettBruttCoeff;
							setCtrlData(CTL_GSITEM_NETTO, &netto);
						}
					}
				}
			}
		}
		else if(event.isKeyDown(kbF6)) {
			if(P_Struc && P_Struc->GoodsID && isCurrCtlID(CTL_GSITEM_VALUE)) {
				SString temp_buf;
				double qtty = 0.0, phuperu;
				PPGoodsStrucItem item;
				getCtrlString(CTL_GSITEM_VALUE, temp_buf);
				if(item.SetEstimationString(temp_buf) && item.GetQtty(1, &qtty) > 0) {
					PPObjGoods goods_obj;
					if(goods_obj.GetPhUPerU(P_Struc->GoodsID, 0, &phuperu) > 0) {
						item.Median = qtty * phuperu;
						item.Denom = 1.0;
						setCtrlString(CTL_GSITEM_VALUE, item.GetEstimationString(temp_buf));
					}
				}
			}
			else
				return;
		}
		else if(event.isKeyDown(kbF4)) {
			if(P_Struc && P_Struc->GoodsID && isCurrCtlID(CTL_GSITEM_VALUE)) {
				SString temp_buf;
				double qtty = 0.0;
				PPGoodsStrucItem item;
				getCtrlString(CTL_GSITEM_VALUE, temp_buf);
				if(item.SetEstimationString(temp_buf) && item.GetQtty(1, &qtty) > 0 && Price > 0.0) {
					item.Median = qtty / Price;
					item.Denom = 1.0;
					setCtrlString(CTL_GSITEM_VALUE, item.GetEstimationString(temp_buf));
				}
			}
			else
				return;
		}
		else
			return;
		clearEvent(event);
	}
	void   setupPrice()
	{
		Price = 0.0;
		GoodsCtrlGroup::Rec rec;
		getGroupData(ctlgroupGoods, &rec);
		if(rec.GoodsID)
			::GetCurGoodsPrice(rec.GoodsID, LConfig.Location, GPRET_MOSTRECENT, &Price);
		setCtrlReal(CTL_GSITEM_PRICE, Price);
	}
};

static int Helper_EditGoodsStrucItem_Ordinary(const PPGoodsStruc * pStruc, PPGoodsStrucItem * pItem)
{
	int    ok = -1;
	int    valid_data = 0;
	GSItemDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new GSItemDialog(pStruc))));
	dlg->setDTS(pItem);
	while(!valid_data && ExecView(dlg) == cmOK) {
		ok = -1;
		if(dlg->getDTS(pItem)) {
			ok = valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

static int Helper_EditGoodsStrucItem_PricePlanning(const PPGoodsStruc * pStruc, PPGoodsStrucItem * pItem)
{
	int    ok = -1;
	int    valid_data = 0;
	GSPPItemDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new GSPPItemDialog(pStruc))));
	dlg->setDTS(pItem);
	while(!valid_data && ExecView(dlg) == cmOK) {
		ok = -1;
		if(dlg->getDTS(pItem)) {
			ok = valid_data = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

static int EditGoodsStrucItem(const PPGoodsStruc * pStruc, PPGoodsStrucItem * pItem)
{
	int    ok = -1;
	if(pStruc && pItem) {
		if(pStruc->GetKind() == PPGoodsStruc::kPricePlanning)
			ok = Helper_EditGoodsStrucItem_PricePlanning(pStruc, pItem);
		else
			ok = Helper_EditGoodsStrucItem_Ordinary(pStruc, pItem);
	}
	return ok;
}

int GSDialog::checkDupGoods(int pos, const PPGoodsStrucItem * pItem)
{
	int    ok = 1;
	for(int i = 0; ok && i < static_cast<int>(Data.Items.getCount()); i++)
		if(i != pos && pItem->GoodsID == Data.Items.at(i).GoodsID)
			ok = PPSetError(PPERR_DUPGSTRUCITEM);
	return ok;
}

int GSDialog::editItemDialog(int pos, PPGoodsStrucItem * pData)
{
	PPGoodsStrucItem item = *pData;
	if(pos < 0) {
		ushort v;
		getCtrlData(CTL_GSTRUC_FLAGS, &v);
		if(v & 4)
			item.Flags |= GSIF_PCTVAL;
	}
	while(EditGoodsStrucItem(&Data, &item) > 0)
		if(!checkDupGoods(pos, &item))
			PPError();
		else {
			*pData = item;
			return 1;
		}
	return -1;
}

int GSDialog::addItem(long * pPos, long * pID) { return addItemExt(pPos, pID); }

int GSDialog::addItemExt(long * pPos, long * pID)
{
	int    ok = -1;
	if(Data.GetKind() == PPGoodsStruc::kPricePlanning) {
		PPGoodsStrucItem item;
		item.Flags |= GSIF_ARTICLE;
		item.ObjType = PPOBJ_ARTICLE;
		if(editItemDialog(-1, &item) > 0) {
			if(Data.Items.insert(&item)) {
				ASSIGN_PTR(pPos, Data.Items.getCount()-1);
				ASSIGN_PTR(pID, Data.Items.getCount());
				updateList(-1);
				Changed = ok = 1;
			}
			else
				PPError(PPERR_SLIB);
		}
	}
	else {
		long   egsd_flags = (ExtGoodsSelDialog::GetDefaultFlags() | ExtGoodsSelDialog::fForcePassive); // @v10.7.7
		ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, NewGoodsGrpID, egsd_flags);
		if(CheckDialogPtrErr(&dlg)) {
			TIDlgInitData tidi;
			tidi.GoodsGrpID = NewGoodsGrpID;
			dlg->setDTS(&tidi);
			while(ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&tidi) > 0) {
					PPGoodsStrucItem item;
					item.GoodsID = tidi.GoodsID;
					NewGoodsGrpID = tidi.GoodsGrpID;
					if(editItemDialog(-1, &item) > 0)
						if(Data.Items.insert(&item)) {
							ASSIGN_PTR(pPos, Data.Items.getCount()-1);
							ASSIGN_PTR(pID, Data.Items.getCount());
							updateList(-1);
							Changed = ok = 1;
						}
						else
							PPError(PPERR_SLIB);
					else
						break;

				}
			}
			delete dlg;
		}
	}
	return ok;
}
//
//
//
int GSDialog::addItemBySample()
{
	class GoodsStrucCopyDialog : public TDialog {
		enum {
			ctlgroupGoods = 1
		};
	public:
		GoodsStrucCopyDialog() : TDialog(DLG_GSCOPY)
		{
			addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_GSCOPY_GGRP, CTLSEL_GSCOPY_GOODS));
		}
		int    setDTS(const GoodsStrucCopyParam * pData)
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			GoodsCtrlGroup::Rec rec(Data.GoodsGrpID, Data.GoodsID);
			setGroupData(ctlgroupGoods, &rec);
			SetupPPObjCombo(this, CTLSEL_GSCOPY_GSTRUC, PPOBJ_GOODSSTRUC, Data.GStrucID, 0, reinterpret_cast<void *>(Data.GoodsID));
			return ok;
		}
		int    getDTS(GoodsStrucCopyParam * pData)
		{
			int    ok = 1;
			uint   sel =0;
			GoodsCtrlGroup::Rec rec;
			getGroupData(ctlgroupGoods, &rec);
			Data.GoodsID = rec.GoodsID;
			getCtrlData(sel = CTLSEL_GSCOPY_GSTRUC, &Data.GStrucID);
			THROW_PP(Data.GStrucID, PPERR_GSTRUCNEEDED);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GSCOPY_GOODS)) {
				const  PPID prev_goods_id = Data.GoodsID;
				getCtrlData(CTLSEL_GSCOPY_GOODS, &Data.GoodsID);
				if(Data.GoodsID != prev_goods_id) {
					Data.GStrucID = 0;
					SetupPPObjCombo(this, CTLSEL_GSCOPY_GSTRUC, PPOBJ_GOODSSTRUC, Data.GStrucID, 0, reinterpret_cast<void *>(Data.GoodsID));
				}
				clearEvent(event);
			}
		}
		GoodsStrucCopyParam Data;
	};
	int    ok = -1;
	GoodsStrucCopyDialog * dlg = new GoodsStrucCopyDialog;
	if(CheckDialogPtrErr(&dlg)) {
		if(!GscParam.GoodsGrpID) {
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(Data.GoodsID, &goods_rec) > 0) {
				GscParam.GoodsGrpID = goods_rec.ParentID;
				SETIFZ(GscParam.GoodsID, Data.GoodsID);
			}
		}
		GscParam.GStrucID = 0;
		dlg->setDTS(&GscParam);
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(&GscParam)) {
				PPGoodsStruc temp;
				if(GsObj.Get(GscParam.GStrucID, &temp) > 0)
					if(Data.IsEmpty() || CONFIRM(PPCFM_SELNAMEDSTRUC)) {
						Data.Items.freeAll();
						Data.CopyItemsFrom(&temp);
						updateList(-1);
						ok = valid_data = 1;
					}
			}
	}
	delete dlg;
	return ok;
}

int GSDialog::editItem(long pos, long)
{
	if(pos >= 0 && pos < (long)Data.Items.getCount() && editItemDialog((int)pos, &Data.Items.at(static_cast<uint>(pos))) > 0) {
		Changed = 1;
		return 1;
	}
	else
		return -1;
}
//
//
//
class GSExtDialog : public PPListDialog {
public:
	GSExtDialog() : PPListDialog(DLG_GSTRUCFOLDER, CTL_GSTRUC_LIST)
	{
		updateList(-1);
	}
	int setDTS(const PPGoodsStruc *);
	int getDTS(PPGoodsStruc *);
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	void   selNamedGS();
	void   reduce();

	PPGoodsStruc Data;
};

int GSExtDialog::setDTS(const PPGoodsStruc * pData)
{
	if(pData->GoodsID) {
		SString goods_name;
		setCtrlString(CTL_GSTRUC_GNAME, GetGoodsName(pData->GoodsID, goods_name));
	}
	Data = *pData;
	setCtrlData(CTL_GSTRUC_NAME, Data.Rec.Name);
	setCtrlLong(CTL_GSTRUC_ID, Data.Rec.ID);
	updateList(-1);
	enableCommand(cmGStrucExpandReduce, Data.CanReduce());
	return 1;
}

int GSExtDialog::getDTS(PPGoodsStruc * pData)
{
	char   buf[64];
	buf[0] = 0;
	getCtrlData(CTL_GSTRUC_NAME, buf);
	if(*strip(buf) == 0) {
		if(Data.Rec.Flags & GSF_NAMED)
			return PPSetError(PPERR_NAMENEEDED);
	}
	else
		Data.Rec.Flags |= GSF_NAMED;
	STRNSCPY(Data.Rec.Name, buf);
	ASSIGN_PTR(pData, Data);
	return 1;
}

IMPL_HANDLE_EVENT(GSExtDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isCmd(cmSelNamedGS))
		selNamedGS();
	else if(event.isCmd(cmGStrucExpandReduce))
		reduce();
	else if(event.isCmd(cmPrint)) {
		PPObjGoodsStruc gs_obj;
		gs_obj.Print(&Data);
	}
	else if(event.isKeyDown(KB_CTRLENTER)) {
		if(IsInState(sfModal)) {
			endModal(cmOK);
			return; // После endModal не следует обращаться к this
		}
	}
	else if(event.isKeyDown(kbF7)) {
		PPObjGoodsStruc gs_obj;
		gs_obj.Print(&Data);
	}
	else
		return;
	clearEvent(event);
}

void GSExtDialog::reduce()
{
	if(Data.CanReduce() && getDTS(0))
		if(Data.Reduce()) {
			if(IsInState(sfModal)) {
				endModal(cmUtil);
				return; // После endModal не следует обращаться к this
			}
		}
		else
			PPError();
}

void GSExtDialog::selNamedGS()
{
	TDialog * dlg = new TDialog(DLG_GSDATA);
	if(CheckDialogPtrErr(&dlg)) {
		PPObjGoodsStruc gs_obj;
		PPID   id = Data.Rec.ID;
		if(!Data.IsNamed() && !Data.IsEmpty()) {
			PPMessage(mfConf | mfOK, PPCFM_SELNAMEDSTRUC);
			id = 0;
		}
		SetupPPObjCombo(dlg, CTLSEL_GSDATA_NAMEDSTRUC, PPOBJ_GOODSSTRUC, id, 0, 0);
		if(ExecView(dlg) == cmOK) {
			PPGoodsStruc temp;
			dlg->getCtrlData(CTLSEL_GSDATA_NAMEDSTRUC, &id);
			if(id == 0) {
				temp.GoodsID = Data.GoodsID;
				setDTS(&temp);
			}
			else if(id != Data.Rec.ID) {
				if(gs_obj.Get(id, &temp) > 0) {
					temp.GoodsID = Data.GoodsID;
					setDTS(&temp);
				}
			}
		}
		delete dlg;
	}
}

int GSExtDialog::setupList()
{
	PPGoodsStruc * p_item = 0;
	for(uint i = 0; Data.Children.enumItems(&i, (void **)&p_item);) {
		char   sub[128];
		StringSet ss(SLBColumnDelim);
		ss.add(STRNSCPY(sub, p_item->Rec.Name));
		ss.add(periodfmt(&p_item->Rec.Period, sub));
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int GSExtDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		if(CONFIRM(PPCFM_DELETE)) {
			Data.Children.atFree(static_cast<uint>(pos));
			enableCommand(cmGStrucExpandReduce, Data.CanReduce());
			return 1;
		}
	}
	return -1;
}

int GSExtDialog::addItem(long * pPos, long * pID)
{
	SString name_buf;
	PPGoodsStruc item;
	item.Rec.Flags |= GSF_CHILD;
	item.Rec.ParentID = Data.Rec.ID; // may be ZERO (unsaved structure)
	item.Rec.VariedPropObjType = Data.Rec.VariedPropObjType;
	Data.MakeChildDefaultName(name_buf).CopyTo(item.Rec.Name, sizeof(item.Rec.Name));
	if(PPObjGoodsStruc::EditDialog(&item) > 0) {
		PPGoodsStruc * p_child = new PPGoodsStruc(item);
		if(Data.Children.insert(p_child)) {
			ASSIGN_PTR(pPos, Data.Children.getCount()-1);
			ASSIGN_PTR(pID, Data.Children.getCount());
			enableCommand(cmGStrucExpandReduce, Data.CanReduce());
			return 1;
		}
		else
			return PPSetErrorSLib();
	}
	else
		return -1;
}

int GSExtDialog::editItem(long pos, long)
{
	if(pos >= 0 && pos < Data.Children.getCountI()) {
		PPGoodsStruc * p_child = Data.Children.at(pos);
		SETIFZ(p_child->GoodsID, Data.GoodsID);
		if(PPObjGoodsStruc::EditDialog(p_child) > 0)
			return 1;
	}
	return -1;
}
//
// PPObjGoodsStruc
//
static int GSListFilt(void * pRec, void * extraPtr)
{
	const PPGoodsStrucHeader * p_rec = static_cast<const PPGoodsStrucHeader *>(pRec);
	return BIN(p_rec->Flags & GSF_NAMED);
}

PPObjGoodsStruc::PPObjGoodsStruc(void * extraPtr) : PPObjReference(PPOBJ_GOODSSTRUC, extraPtr)
{
	FiltProc = GSListFilt;
	ImplementFlags |= implStrAssocMakeList;
}

StrAssocArray * PPObjGoodsStruc::MakeStrAssocList(void * extraPtr /*goodsID*/)
{
	const   PPID goods_id = reinterpret_cast<PPID>(extraPtr);
	StrAssocArray * p_list = new StrAssocArray;
	PPIDArray id_list;
	if(goods_id) {
		//
		// Выбираем только структуры, относящиеся к указанному товару
		//
		if(goods_id > 0) {
			SString temp_buf;
			PPObjGoods goods_obj;
			Goods2Tbl::Rec goods_rec;
			if(goods_obj.Fetch(goods_id, &goods_rec) > 0) {
				PPID   struc_by_goods_id = goods_rec.StrucID;
				if(!struc_by_goods_id)
					goods_obj.GetAltGoodsStrucID(goods_id, 0, &struc_by_goods_id);
				PPGoodsStrucHeader gsh_rec;
				if(Search(struc_by_goods_id, &gsh_rec) > 0) {
					if(gsh_rec.Flags & GSF_FOLDER)
						GetChildIDList(struc_by_goods_id, &id_list);
					else
						id_list.add(struc_by_goods_id);
				}
				for(uint i = 0; i < id_list.getCount(); i++) {
					if(Search(id_list.at(i), &gsh_rec) > 0) {
						temp_buf = gsh_rec.Name;
						if(!temp_buf.NotEmptyS())
							(temp_buf = "BOM").Space().CatChar('#').CatLongZ(static_cast<long>(i+1), 2);
						THROW_SL(p_list->Add(gsh_rec.ID, temp_buf));
					}
				}
			}
		}
	}
	else {
		//
		// Выбираем только именованные структуры
		//
		PPGoodsStrucHeader gs_rec;
		for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&gs_rec) > 0;)
			if(gs_rec.Flags & GSF_NAMED) {
				THROW_SL(p_list->Add(gs_rec.ID, gs_rec.Name));
			}
	}
	p_list->SortByText();
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int PPObjGoodsStruc::GetChildIDList(PPID strucID, PPIDArray * pList)
{
	int    ok = -1;
	BExtQuery q(P_Ref, 0, 128);
	q.select(P_Ref->ObjID, 0L).where(P_Ref->ObjType == Obj && P_Ref->Val1 == strucID);
	ReferenceTbl::Key0 k0;
	k0.ObjType = Obj;
	k0.ObjID = 0;
	for(q.initIteration(false, &k0, spGe); q.nextIteration() > 0;) {
		CALLPTRMEMB(pList, add(P_Ref->data.ObjID));
		ok = 1;
	}
	return ok;
}

static IMPL_CMPFUNC(_GSItem, i1, i2) { return cmp_long(static_cast<const _GSItem *>(i1)->Num, static_cast<const _GSItem *>(i2)->Num); }

int PPObjGoodsStruc::Helper_LoadItems(PPID id, PPGoodsStruc * pData)
{
	int    ok = 1;
	TSVector <ObjAssocTbl::Rec> raw_items_list;
	_GSItem * p_raw_item;
	SString temp_buf, formula;
	THROW(P_Ref->GetPropVlrString(Obj, id, GSPRP_EXTITEMSTR, temp_buf));
	THROW(P_Ref->Assc.GetItemsListByPrmr(PPASS_GOODSSTRUC, id, &raw_items_list));
	raw_items_list.sort(PTR_CMPFUNC(_GSItem));
	//
	// !Мы получили вектор записей типа <ObjAssocTbl::Rec> но обрабатываем его так, как будто
	// там элементы типа _GSItem. Здесь мы закладываемся на то, что структура _GSItem подогнана под ObjAssocTbl::Rec
	// (see remarks on _GSItem)
	//
	for(uint i = 0; raw_items_list.enumItems(&i, (void **)&p_raw_item);) {
		PPGoodsStrucItem item;
		item.GoodsID = p_raw_item->ItemGoodsID;
		item.Flags   = p_raw_item->Flags;
		// @v12.0.7 {
		if(item.Flags & GSIF_ARTICLE) {
			item.ObjType = PPOBJ_ARTICLE;
			item.AccSheetID = p_raw_item->AccSheetID;
		}
		else {
			item.ObjType = PPOBJ_GOODS;
			item.AccSheetID = 0;
		}
		// } @v12.0.7 
		item.Median  = p_raw_item->Median;
		item.Width   = p_raw_item->Width;
		item.Denom   = p_raw_item->Denom;
		item.Netto   = p_raw_item->Netto;
		STRNSCPY(item.Symb, p_raw_item->Symb);
		PPGetExtStrData(i, temp_buf, formula);
		formula.CopyTo(item.Formula__, sizeof(item.Formula__));
		THROW_SL(pData->Items.insert(&item));
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::Get(PPID id, PPGoodsStruc * pData)
{
	int    ok = 1, r = 0;
	THROW(r = P_Ref->GetItem(Obj, id, &pData->Rec));
	if(r > 0) {
		pData->Items.clear(); // @v11.1.12 freeAll()-->clear()
		pData->Children.freeAll();
		if(pData->Rec.Flags & GSF_FOLDER) {
			PPIDArray child_idlist;
			THROW(GetChildIDList(id, &child_idlist));
			for(uint i = 0; i < child_idlist.getCount(); i++) {
				PPGoodsStruc * p_child = new PPGoodsStruc;
				THROW_MEM(p_child);
				THROW(r = Get(child_idlist.at(i), p_child)); // @recursion
				if(r > 0)
					THROW_SL(pData->Children.insert(p_child));
			}
		}
		else {
			THROW(Helper_LoadItems(id, pData));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::Put(PPID * pID, PPGoodsStruc * pData, int use_ta)
{
	int    ok = 1;
	int    r;
	int    unchg = 0;
	int    items_unchg = 0;
	int    items_components_updated = 0; // !0 если были добавлены, удалены или изменены товары в строках структуры (изменения остальных атрибутов здесь не важны)
	int    cleared = 0;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!pData || (pData->IsEmpty() && !pData->IsNamed() && !(pData->Rec.Flags & GSF_FOLDER))) {
			if(*pID) {
				THROW(P_Ref->RemoveItem(Obj, *pID, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
				cleared = 1;
			}
		}
		else {
			pData->Rec.Flags &= ~GSF_DYNGEN;
			if(*strip(pData->Rec.Name) != 0 && !(pData->Rec.Flags & GSF_CHILD))
				pData->Rec.Flags |= GSF_NAMED;
			if(*pID) {
				THROW(r = P_Ref->UpdateItem(Obj, *pID, &pData->Rec, 1, 0));
				if(r < 0)
					unchg = 1;
			}
			else {
				THROW(P_Ref->AddItem(Obj, pID, &pData->Rec, 0));
				pData->Rec.ID = *pID;
			}
			if(pData->Rec.Flags & GSF_FOLDER) {
				for(uint i = 0; i < pData->Children.getCount(); i++) {
					PPGoodsStruc * p_child = pData->Children.at(i);
					if(p_child) {
						p_child->Rec.ParentID = *pID;
						p_child->Rec.Flags |= GSF_CHILD;
						THROW(r = Put(&p_child->Rec.ID, p_child, 0)); // @recursion
						if(unchg && r > 0)
							unchg = 0;
					}
				}
			}
		}
		if(*pID) {
			if(pData) {
				uint i;
				PPGoodsStruc temp_struc;
				LongArray pos_list;
				THROW(Helper_LoadItems(*pID, &temp_struc));
				if(temp_struc.Items.getCount() == pData->Items.getCount()) {
					items_unchg = 1;
					for(i = 0; items_unchg && i < temp_struc.Items.getCount(); i++) {
						if(!(pData->Items.at(i) == temp_struc.Items.at(i)))
							items_unchg = 0;
					}
				}
				if(!items_unchg) {
					for(i = 0; !items_components_updated && i < temp_struc.Items.getCount(); i++) {
						const PPGoodsStrucItem & r_org_item = temp_struc.Items.at(i);
						if(!pData->HasGoods(r_org_item.GoodsID))
							items_components_updated = 1;
					}
					for(i = 0; !items_components_updated && i < pData->Items.getCount(); i++) {
						const PPGoodsStrucItem & r_item = pData->Items.at(i);
						if(!temp_struc.HasGoods(r_item.GoodsID))
							items_components_updated = 1;
					}
				}
			}
			if(!items_unchg)
				THROW(P_Ref->Assc.Remove(PPASS_GOODSSTRUC, *pID, 0, 0));
		}
		if(pData && !items_unchg) {
			_GSItem gsi;
			PPGoodsStrucItem * pi;
			SString ext_buf;
			for(uint i = 0; pData->Items.enumItems(&i, (void **)&pi);) {
				PPID   assc_id = 0;
				MEMSZERO(gsi);
				gsi.Tag  = PPASS_GOODSSTRUC;
				gsi.GSID = *pID;
				gsi.ItemGoodsID = pi->GoodsID;
				if(pi->ObjType == PPOBJ_ARTICLE) {
					gsi.AccSheetID = pi->AccSheetID;
					gsi.Flags |= GSIF_ARTICLE;
				}
				else {
					gsi.AccSheetID = 0;
					gsi.Flags &= ~GSIF_ARTICLE;
				}
				gsi.Flags  = pi->Flags;
				gsi.Median = pi->Median;
				gsi.Width  = pi->Width;
				gsi.Denom  = pi->Denom;
				gsi.Netto  = pi->Netto;
				STRNSCPY(gsi.Symb, pi->Symb);
				gsi.Num    = i;
				THROW(P_Ref->Assc.SearchFreeNum(gsi.Tag, *pID, &gsi.Num));
				THROW(P_Ref->Assc.Add(&assc_id, (ObjAssocTbl::Rec *)&gsi, 0));
				THROW(PPPutExtStrData(i, ext_buf, strip(pi->Formula__)));
			}
			THROW(P_Ref->PutPropVlrString(Obj, *pID, GSPRP_EXTITEMSTR, ext_buf));
		}
		if(cleared)
			*pID = 0;
		if(unchg) {
			if(items_unchg)
				ok = -1;
			else {
				DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
				// @v10.2.1 {
				if(items_components_updated)
					DS.LogAction(PPACN_GSTRUCCUPD, Obj, *pID, 0, 0);
				// } @v10.2.1
			}
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsStruc::EditExtDialog(PPGoodsStruc * pData)
{
	int    ok = -1;
	GSExtDialog * dlg = new GSExtDialog;
	if(CheckDialogPtrErr(&dlg)) {
		int r;
		dlg->setDTS(pData);
		while(ok <= 0 && ((r = ExecView(dlg)) == cmOK || r == cmUtil))
			if(dlg->getDTS(pData)) {
				ZDELETE(dlg);
				ok = (r == cmUtil) ? PPObjGoodsStruc::EditDialog(pData) : 1;
				break;
			}
			else
				PPError();
	}
	else
		ok = 0;
	delete dlg;
	return ok;
}

/*static*/int PPObjGoodsStruc::EditDialog(PPGoodsStruc * pData, int toCascade)
{
	int    ok = -1;
	PPObjGoodsStruc gs_obj;
	if(gs_obj.CheckRights(PPR_READ)) {
		if(pData->Rec.Flags & GSF_FOLDER)
			ok = EditExtDialog(pData);
		else {
			GSDialog * dlg = new GSDialog;
			if(CheckDialogPtrErr(&dlg)) {
				int    r;
				if(toCascade)
					dlg->ToCascade();
				dlg->setDTS(pData);
				while(ok <= 0 && ((r = ExecView(dlg)) == cmOK || r == cmUtil || dlg->IsChanged() && !CONFIRM(PPCFM_WARNCANCEL)))
					if(r != cmCancel && dlg->getDTS(pData)) {
						ZDELETE(dlg);
						ok = (r == cmUtil) ? PPObjGoodsStruc::EditExtDialog(pData) : 1;
						break;
					}
			}
			else
				ok = 0;
			delete dlg;
		}
	}
	else
		PPError();
	return ok;
}

int PPObjGoodsStruc::Edit(PPID * pID, void * extraPtr /*goodsID*/)
{
	int    ok = cmCancel;
	PPGoodsStruc data;
	if(*pID) {
		THROW(Get(*pID, &data));
	}
	data.GoodsID = reinterpret_cast<PPID>(extraPtr);
	if(!data.GoodsID && data.Rec.ID) {
		PPObjGoods goods_obj;
		PPIDArray owner_list;
		goods_obj.SearchGListByStruc(data.Rec.ID, false/*expandGenerics*/, owner_list);
		if(owner_list.getCount() == 1)
			data.GoodsID = owner_list.get(0);
	}
	if((ok = EditDialog(&data)) > 0) {
		THROW(Put(pID, &data, 1));
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsStruc, PPGoodsStruc);

int PPObjGoodsStruc::SerializePacket(int dir, PPGoodsStruc * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	int32  c = (int32)pPack->Items.getCount(); // @persistent
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
	THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(c)), &c, 0, rBuf));
	for(int i = 0; i < c; i++) {
		PPGoodsStrucItem item;
		if(dir > 0)
			item = pPack->Items.at(i);
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(PPID)), &item.GoodsID, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_INT, sizeof(long)), &item.Flags, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_FLOAT, sizeof(item.Median)), &item.Median, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_FLOAT, sizeof(item.Width)), &item.Width, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_FLOAT, sizeof(item.Denom)), &item.Denom, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_FLOAT, sizeof(item.Netto)), &item.Netto, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_ZSTRING, sizeof(item.Symb)), item.Symb, 0, rBuf));
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_ZSTRING, sizeof(item.Formula__)), item.Formula__, 0, rBuf));
		if(dir < 0) {
			THROW_SL(pPack->Items.insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::Read(PPObjPack * pPack, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPGoodsStruc * p_gs = new PPGoodsStruc;
	THROW_MEM(p_gs);
	if(stream == 0) {
		THROW(Get(id, p_gs));
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile(static_cast<FILE *>(stream), 0))
		THROW(SerializePacket(-1, p_gs, buffer, &pCtx->SCtx));
	}
	CATCH
		ok = 0;
		ZDELETE(p_gs);
	ENDCATCH
	pPack->Data = p_gs;
	return ok;
}

int PPObjGoodsStruc::Write(PPObjPack * pPack, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPGoodsStruc * p_gs = static_cast<PPGoodsStruc *>(pPack->Data);
	if(p_gs) {
		if(stream == 0) {
			if(*pID == 0) {
				if(!Put(pID, p_gs, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSSTRUC, p_gs->Rec.ID, p_gs->Rec.Name);
					ok = -1;
				}
			}
			else {
				PPObjGoods goods_obj;
				const long gc_flags = goods_obj.GetConfig().Flags;
				if(gc_flags & GCF_XCHG_RCVSTRUCUPD) {
					if(!(gc_flags & GCF_XCHG_RCVSTRUCFROMDDONLY) || (pCtx->P_SrcDbDivPack && pCtx->P_SrcDbDivPack->Rec.Flags & DBDIVF_DISPATCH)) { // @v10.2.1
						if(!Put(pID, p_gs, 1)) {
							pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSSTRUC, p_gs->Rec.ID, p_gs->Rec.Name);
							ok = -1;
						}
					}
				}
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_gs, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		uint   i;
		PPGoodsStruc * p_gs = static_cast<PPGoodsStruc *>(p->Data);
		PPGoodsStrucItem * p_gsi;
		for(i = 0; p_gs->Items.enumItems(&i, (void **)&p_gsi);)
			THROW(ProcessObjRefInArray(PPOBJ_GOODS, &p_gsi->GoodsID, ary, replace));
		for(i = 0; i < p_gs->Children.getCount(); i++)
			THROW(ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &p_gs->Children.at(i)->Rec.ID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &p_gs->Rec.ParentID, ary, replace));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE)
		if(_obj == PPOBJ_GOODS) {
			ObjAssocTbl::Rec assc_rec;
			SEnum en = P_Ref->Assc.Enum(PPASS_GOODSSTRUC, _id, 1);
			if(en.Next(&assc_rec) > 0) {
				ok = RetRefsExistsErr(Obj, assc_rec.PrmrObjID);
			}
		}
	if(msg == DBMSG_OBJREPLACE)
		if(_obj == PPOBJ_GOODS) {
			ObjAssocTbl * t = &P_Ref->Assc;
			if(!updateFor(t, 0, (t->AsscType == PPASS_GOODSSTRUC && t->ScndObjID == _id),
				set(t->ScndObjID, dbconst(reinterpret_cast<long>(extraPtr))))) {
				ok = PPSetErrorDB();
			}
		}
	return ok;
}

int PPObjGoodsStruc::Print(PPGoodsStruc * pGoodsStruc)
{
	int    ok = -1;
	int    is_hier = 0; // !0 - иерархическая структура
	uint   what = 0x00;
	PPObjGoods goods_obj;
	{
		PPIDArray goods_ids;
		PPIDArray struct_ids;
		PPLogger log;
		goods_obj.SearchGListByStruc(pGoodsStruc->Rec.ID, false/*expandGenerics*/, goods_ids);
		if(CheckStruct(&goods_ids, &struct_ids, pGoodsStruc, 0, &log) > 0)
			is_hier = 1;
	}
	if(!is_hier || ::SelectorDialog(DLG_GSTRUCPRN, CTL_GSTRUCPRN_WHAT, &what) > 0) {
		uint   rpt_id = 0;
		if(what == 0x01)
			rpt_id = REPORT_GOODSSTRUCRECUR;
		else {
			is_hier = 0;
			rpt_id = REPORT_GOODSSTRUC;
		}
		GStrucIterator gs_iter;
		gs_iter.Init(pGoodsStruc, is_hier);
		PPAlddPrint(rpt_id, PView(&gs_iter), 0);
		ok = 1;
	}
	return ok;
}

GStrucIterator::GStrucIterator() : Idx(0), LoadRecurItems(0)
{
}

const PPGoodsStruc * GStrucIterator::GetStruc() const { return &GStruc; }

int GStrucIterator::LoadItems(const PPGoodsStruc * pStruc, PPID parentGoodsID, double srcQtty, int level)
{
	int    ok = 1;
	if(pStruc) {
		long   fmt = MKSFMTD(0, 5, NMBF_NOTRAILZ);
		double dest_qtty = 0.0;
		SString s_qtty;
		PPObjGoods g_obj;
		PPObjGoodsStruc gs_obj;
		GStrucRecurItem gsr_item;
		MEMSZERO(gsr_item);
		gsr_item.Level = level;
		for(uint i = 0; pStruc->EnumItemsExt(&i, &gsr_item.Item, parentGoodsID, srcQtty, &dest_qtty) > 0;) {
			int    do_load_recur = 0;
			double sum = 0.0;
			PPGoodsStruc inner_struc;
			ReceiptTbl::Rec lot_rec;
			s_qtty.Z().Cat(dest_qtty, fmt);
			gsr_item.Qtty = dest_qtty;
			gsr_item.Item.SetEstimationString(s_qtty);
			if(pStruc->GetEstimationPrice(i-1, 0, &gsr_item.Price, &gsr_item.Sum, &lot_rec) > 0)
				gsr_item.LastLotID = lot_rec.ID;
			gsr_item.HasInner = 0;
			{
				const int r = g_obj.LoadGoodsStruc(PPGoodsStruc::Ident(gsr_item.Item.GoodsID), &inner_struc);
				THROW(r);
				if(r > 0 && gs_obj.CheckStruc(inner_struc.Rec.ID, 0) != 2) {
					int    uncert = 0;
					inner_struc.CalcEstimationPrice(0, &gsr_item.Price, &uncert, 1);
					gsr_item.Item.GetQtty(gsr_item.Price / GStruc.GetDenom(), &sum);
					gsr_item.Item.SetEstimationString(s_qtty);
					gsr_item.HasInner = 1;
					if(LoadRecurItems)
						do_load_recur = 1;
				}
			}
			THROW_SL(Items.insert(&gsr_item));
			if(do_load_recur)
				THROW(LoadItems(&inner_struc, gsr_item.Item.GoodsID, dest_qtty, level + 1)); // @recursion
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

void GStrucIterator::Init(const PPGoodsStruc * pStruc, int loadRecurItems)
{
	RVALUEPTR(GStruc, pStruc);
	LoadRecurItems = loadRecurItems;
	Items.freeAll();
	LoadItems(&GStruc, GStruc.GoodsID, 1, 0);
}

void GStrucIterator::InitIteration()
{
	Idx = 0;
}

int FASTCALL GStrucIterator::NextIteration(GStrucRecurItem * pItem)
{
	int    ok = -1;
	GStrucRecurItem * p_item = 0;
	if(Items.enumItems(&Idx, (void **)&p_item) > 0) {
		ASSIGN_PTR(pItem, *p_item);
		ok = 1;
	}
	return ok;
}

int PPObjGoodsStruc::CheckStruct(PPIDArray * pGoodsIDs, PPIDArray * pStructIDs, const PPGoodsStruc * pStruc, TSCollection <CheckGsProblem> * pProblemList, PPLogger * pLogger)
{
	int    ok = 1;
	int    recur = 0;
	if(pStruc) {
		const int gs_kind = PPGoodsStruc::GetStrucKind(pStruc->Rec.Flags);
		SString fmt_buf;
		SString buf;
		SString g_name, cg_name, cstruc_name;
		SString struc_name;
		PPObjGoods goods_obj;
		ideqvalstr(pStruc->Rec.ID, struc_name);
		if(pStruc->Rec.Name[0])
			struc_name.Space().CatParStr(pStruc->Rec.Name);
		/* @construction
		if(gs_kind == PPGoodsStruc::kUndef && pStruct->Rec.ID && (!pStructIDs || !pStructIDs->lsearch(pStruct->Rec.ID))) {
			PPLoadText(PPTXT_GSTRUCNDEFKIND, fmt_buf);
			buf.Printf(fmt_buf.cptr(), struc_name.cptr());
			CALLPTRMEMB(pLog, Log(buf));
		}
		*/
		if(pStruc->Rec.ID) {
			PPIDArray owner_list;
			goods_obj.SearchGListByStruc(pStruc->Rec.ID, false/*expandGenerics*/, owner_list);
			if(!owner_list.getCount() && !(pStruc->Rec.Flags & GSF_CHILD)) {
				// На товарную структуру %s не ссылается ни один товар
				PPLoadText(PPTXT_GSTRUCUNREF, fmt_buf);
				buf.Printf(fmt_buf.cptr(), struc_name.cptr());
				if(pProblemList) {
					CheckGsProblem * p_problem = pProblemList->CreateNewItem();
					THROW_SL(p_problem);
					p_problem->Code = CheckGsProblem::errUnRef;
					p_problem->Descr = buf;
					p_problem->LocIdent = pStruc->Rec.ID;
					p_problem->ItemI = -1;
				}
				CALLPTRMEMB(pLogger, Log(buf));
			}
			else if(owner_list.getCount() > 1 && !(pStruc->Rec.Flags & GSF_NAMED)) {
				// Товарная структура %s не является именованной, но на нее ссылается более одного товара
				PPLoadText(PPTXT_GSTRUCNONAMEAMBIGREF, fmt_buf);
				buf.Printf(fmt_buf.cptr(), struc_name.cptr());
				if(pProblemList) {
					CheckGsProblem * p_problem = pProblemList->CreateNewItem();
					THROW_SL(p_problem);
					p_problem->Code = CheckGsProblem::errNoNameAmbig;
					p_problem->Descr = buf;
					p_problem->LocIdent = pStruc->Rec.ID;
					p_problem->ItemI = -1;
				}
				CALLPTRMEMB(pLogger, Log(buf));
			}
			if(pStruc->Rec.Flags & GSF_NAMED && pStruc->Rec.Name[0] == 0) {
				// Именованная товарная стуктура имеет пустое наименование
				PPLoadText(PPTXT_GSTRUCNAMEDWONAME, fmt_buf);
				buf.Printf(fmt_buf.cptr(), struc_name.cptr());
				if(pProblemList) {
					CheckGsProblem * p_problem = pProblemList->CreateNewItem();
					THROW_SL(p_problem);
					p_problem->Code = CheckGsProblem::errNamedEmptyName;
					p_problem->Descr = buf;
					p_problem->LocIdent = pStruc->Rec.ID;
					p_problem->ItemI = -1;
				}
				CALLPTRMEMB(pLogger, Log(buf));
			}
		}
		if(pStruc->Rec.Flags & GSF_COMPL && pGoodsIDs && pStructIDs) {
			PPGoodsStrucItem * p_item = 0;
			PPGoodsStruc gstruc;
			THROW_SL(pStructIDs->add(pStruc->Rec.ID));
			for(uint i = 0; !recur && pStruc->Items.enumItems(&i, (void **)&p_item) > 0;) {
				int    s = 0;
				int    g = 0;
				double price = 0.0;
				gstruc.Init();
				THROW(goods_obj.LoadGoodsStruc(PPGoodsStruc::Ident(p_item->GoodsID), &gstruc));
				if((g = pGoodsIDs->lsearch(p_item->GoodsID)) > 0 || (s = pStructIDs->lsearch(gstruc.Rec.ID)) > 0) {
					recur = 1;
					PPID   goods_id = (pGoodsIDs->getCount() > 0) ? pGoodsIDs->at(pGoodsIDs->getCount() - 1) : 0;
					GetGoodsName(goods_id, g_name);
					GetGoodsName(p_item->GoodsID, cg_name);
					(cstruc_name = gstruc.Rec.Name).Strip();
					if(cstruc_name.IsEmpty())
						ideqvalstr(gstruc.Rec.ID, cstruc_name);
					PPLoadText(PPTXT_CYCLESSTRUCT, fmt_buf);
					buf.Printf(fmt_buf.cptr(), g_name.cptr(), struc_name.cptr(), cg_name.cptr(), cstruc_name.cptr());
					if(pProblemList) {
						CheckGsProblem * p_problem = pProblemList->CreateNewItem();
						THROW_SL(p_problem);
						p_problem->Code = CheckGsProblem::errRecur;
						p_problem->Descr = buf;
						p_problem->LocIdent = pStruc->Rec.ID;
						p_problem->ItemI = i;
					}
					CALLPTRMEMB(pLogger, Log(buf));
				}
				else {
					int    r = 0;
					THROW_SL(pGoodsIDs->add(p_item->GoodsID));
					THROW(r = CheckStruct(pGoodsIDs, pStructIDs, &gstruc, pProblemList, pLogger)); // @recursion
					THROW_SL(pGoodsIDs->removeByID(p_item->GoodsID));
					if(r == 2)
						recur = 1;
					if(ok < 0 && r > 0)
						ok = r;
				}
			}
			THROW_SL(pStructIDs->removeByID(pStruc->Rec.ID));
		}
	}
	else
		ok = -1;
	CATCHZOK
	if(recur)
		ok = 2;
	return ok;
}

int PPObjGoodsStruc::CheckStruc(PPID strucID, PPLogger * pLogger)
{
	int    ok = -1;
	PPGoodsStruc gs;
	PPObjGoods goods_obj;
	PPIDArray struct_ids;
	PPIDArray goods_ids;
	goods_obj.SearchGListByStruc(strucID, false/*expandGenerics*/, goods_ids);
	THROW(Get(strucID, &gs));
	THROW(ok = CheckStruct(&goods_ids, &struct_ids, &gs, 0, pLogger));
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::CheckStructs()
{
	int    ok = 1;
	long   p = 0, t = 0;
	PPID   id = 0;
	PPLogger logger;
	PPObjGoods goods_obj;
	PPWaitStart();
	for(id = 0; EnumItems(&id) > 0; t++)
		;
	for(id = 0, p = 0; EnumItems(&id) > 0; p++) {
		THROW(CheckStruc(id, &logger));
		PPWaitPercent(p, t);
	}
	CATCHZOKPPERR
	PPWaitStop();
	return ok;
}

/*static*/int PPObjGoodsStruc::CheckRecursion()
{
	PPObjGoodsStruc gs_obj;
	return gs_obj.CheckStructs();
}
//
//
//
SaGiftItem::SaGiftItem() : StrucID(0), OrgStrucID(0), QuotKindID(0), Flags(0), Limit(0.0f), AmtRestrict(0.0)
{
}

SaGiftItem::SaGiftItem(const SaGiftItem & rS)
{
	Copy(&rS);
}

SaGiftItem & FASTCALL SaGiftItem::operator = (const SaGiftItem & rS)
{
	Copy(&rS);
	return *this;
}

int FASTCALL SaGiftItem::Copy(const SaGiftItem * pS)
{
	int    ok = 1;
	if(pS) {
		StrucID = pS->StrucID;
		OrgStrucID = pS->OrgStrucID;
		QuotKindID = pS->QuotKindID;
		Flags = pS->Flags;
		Limit = pS->Limit;
		GiftList = pS->GiftList;
		AmtRestrict = pS->AmtRestrict;
		List.freeAll();
		for(uint i = 0; i < pS->List.getCount(); i++) {
			Entry * p_entry = new Entry(*pS->List.at(i));
			THROW_MEM(p_entry);
			THROW_SL(List.insert(p_entry));
		}
	}
	else {
		StrucID = 0;
		OrgStrucID = 0;
		Limit = 0.0f;
		AmtRestrict = 0.0;
		GiftList.freeAll();
		List.freeAll();
	}
	CATCHZOK
	return ok;
}

int SaGiftItem::IsSaleListSuitable(const TSVector <SaSaleItem> & rSaleList, RAssocArray * pCheckList, LongArray * pMainPosList, double * pQtty) const
{
	int    ok = 1;
	LongArray main_pos_list;
	RAssocArray check_list;
	RAssocArray goods_list;
	double min_mult = SMathConst::Max;
	double total_amt = 0.0;
	for(uint i = 0; ok && i < List.getCount(); ++i) {
		const  Entry * p_entry = List.at(i);
		double entry_mult = 0.0;
		goods_list.clear();
		if(p_entry->MinQtty > 0.0) {
			double total_iq = 0.0;
			if(p_entry->GsiFlags & GSIF_IDENTICAL) {
				//
				// В случае GSIF_IDENTICAL нам нужно найти проданный товар, который входит в подарочную группу.
				// В то же время, количество учитывается только по одному наименованию. Таким образом,
				// мы ищем максимум по количеству среди проданных товаров, входящих в группу.
				//
				// Не забываем проверить, чтобы товар дважды не учелся в разных подарках (check_list)
				//
				int    max_iq_idx = -1;
				for(uint j = 0; j < rSaleList.getCount(); j++) {
					const SaSaleItem & r_sa_item = rSaleList.at(j);
					if(r_sa_item.Qtty > total_iq && !check_list.Has(r_sa_item.GoodsID) && p_entry->GoodsList.lsearch(r_sa_item.GoodsID)) {
						total_iq = r_sa_item.Qtty;
						total_amt = (r_sa_item.Qtty * r_sa_item.Price);
						max_iq_idx = (int)j;
					}
				}
				if(max_iq_idx >= 0) {
					goods_list.Add(rSaleList.at(max_iq_idx).GoodsID, p_entry->MinQtty);
					entry_mult = total_iq / p_entry->MinQtty;
				}
			}
			else {
				//
				// Нам подойдет любой из товаров, представленных в p_entry->GoodsList и находящийся в rSaleList.
				//
				// Не забываем проверить, чтобы товар дважды не учелся в разных подарках (check_list)
				//
				const uint egc = p_entry->GoodsList.getCount();
				for(uint j = 0; j < egc; j++) {
					uint   p = 0;
					double iq = 0.0;
					const  PPID sale_goods_id = p_entry->GoodsList.get(j);
					if(!check_list.Search(sale_goods_id, 0, 0) && rSaleList.lsearch(&sale_goods_id, &p, CMPF_LONG)) {
						const SaSaleItem & r_sa_item = rSaleList.at(p);
						total_iq += r_sa_item.Qtty;
						total_amt += (r_sa_item.Qtty * r_sa_item.Price);
						goods_list.Add(sale_goods_id, p_entry->MinQtty);
					}
				}
				entry_mult = total_iq / p_entry->MinQtty;
			}
		}
		const uint glc = goods_list.getCount();
		if(glc && entry_mult > 0.0) {
			for(uint j = 0; j < glc; j++) {
				const RAssoc & r_goods_entry = goods_list.at(j);
				if(!check_list.Search(r_goods_entry.Key, 0, 0)) {
					check_list.Add(r_goods_entry.Key, r_goods_entry.Val);
					if(p_entry->GsiFlags & GSIF_MAINITEM)
						main_pos_list.addUnique(check_list.getCount()-1);
				}
			}
			SETMIN(min_mult, entry_mult);
		}
		else {
			//
			// Этот элемент списка не продавался, значит и весь подарок не проходит.
			//
			ok = 0;
		}
	}
	if(ok && AmtRestrict > 0.0) {
		if(total_amt < AmtRestrict)
			ok = 0;
		else
			min_mult = MIN(min_mult, fint(total_amt / AmtRestrict));
	}
	if(!ok) {
		min_mult = 0.0;
		check_list.freeAll();
	}
	ASSIGN_PTR(pQtty, min_mult);
	ASSIGN_PTR(pCheckList, check_list);
	ASSIGN_PTR(pMainPosList, main_pos_list);
	return ok;
}

int SaGiftItem::CalcPotential(const TSVector <SaSaleItem> & rSaleList, PPID * pPotGoodsID, double * pPotAmount, double * pPotDeficit, SString & rPotName) const
{
	int    ok = -1;
	if(Flags & fCalcPotential) {
		int    potential = 0;
		uint   lack_count = 0;
		PPID   lack_goods_id = 0;
		PPIDArray check_list;
		PPIDArray goods_list;
		double min_mult = SMathConst::Max;
		double total_amt = 0.0;
		double dfct_amt = 0.0;
		for(uint i = 0; i < List.getCount(); ++i) {
			const  Entry * p_entry = List.at(i);
			int    found = 0;
			double entry_mult = 0.0;
			goods_list.clear();
			if(p_entry->MinQtty > 0.0) {
				double total_iq = 0.0;
				for(uint j = 0; j < p_entry->GoodsList.getCount(); j++) {
					uint   p = 0;
					double iq = 0.0;
					const  PPID sale_goods_id = p_entry->GoodsList.get(j);
					if(!check_list.lsearch(sale_goods_id) && rSaleList.lsearch(&sale_goods_id, &p, CMPF_LONG)) {
						const SaSaleItem & r_sa_item = rSaleList.at(p);
						total_iq += r_sa_item.Qtty;
						total_amt += (r_sa_item.Qtty * r_sa_item.Price);
						goods_list.addUnique(sale_goods_id);
						found = 1;
					}
				}
				entry_mult = total_iq / p_entry->MinQtty;
			}
			if(entry_mult > 0.0 && goods_list.getCount()) {
				check_list.addUnique(&goods_list);
				SETMIN(min_mult, entry_mult);
				SETIFZ(lack_goods_id, p_entry->OrgGoodsID);
			}
			else if(!found) {
				lack_count++;
				lack_goods_id = p_entry->OrgGoodsID;
			}
		}
		if(!lack_count || (List.getCount() > 1 && lack_count == 1)) {
			potential = 1;
		}
		if(potential && AmtRestrict > 0.0) {
			if(total_amt < AmtRestrict)
				dfct_amt = (AmtRestrict - total_amt);
			else
				min_mult = fint(total_amt / AmtRestrict);
		}
		if(potential) {
			PPObjGoodsStruc gs_obj;
			PPGoodsStrucHeader gs_rec;
			if(gs_obj.Fetch(StrucID, &gs_rec) > 0 && gs_rec.Name[0])
				rPotName = gs_rec.Name;
			else if(OrgStrucID != StrucID && gs_obj.Fetch(OrgStrucID, &gs_rec) > 0 && gs_rec.Name[0])
				rPotName = gs_rec.Name;
			else {
				for(uint j = 0; j < GiftList.getCount() && rPotName.IsEmpty(); ++j)
					GetGoodsName(GiftList.get(j), rPotName);
			}
			ASSIGN_PTR(pPotGoodsID, lack_goods_id);
			ASSIGN_PTR(pPotAmount, AmtRestrict);
			ASSIGN_PTR(pPotDeficit, dfct_amt);
			ok = 1;
		}
	}
	return ok;
}

SaGiftArray::Gift::Gift()
{
	Z();
}

SaGiftArray::Gift & SaGiftArray::Gift::Z()
{
	ID = 0;
	Qtty = 0.0;
	QuotKindID = 0;
	List.clear();
	CheckList.clear();
	MainPosList.clear();
	Pot.GoodsID = 0;
	Pot.Amount = 0.0;
	Pot.Deficit = 0.0;
	Pot.Name = 0;
	return *this;
}

int FASTCALL SaGiftArray::Gift::IsEqualForResult(const Gift & rS) const
{
	int    eq = 1;
	if(ID != rS.ID)
		eq = 0;
	else if(Qtty != rS.Qtty)
		eq = 0;
	else if(QuotKindID != rS.QuotKindID)
		eq = 0;
	else if(List != rS.List)
		eq = 0;
	else if(!CheckList.IsEq(rS.CheckList))
		eq = 0;
	return eq;
}

void FASTCALL SaGiftArray::Gift::PreservePotential(SaGiftArray::Potential & rS) const { rS = Pot; }
void FASTCALL SaGiftArray::Gift::RestorePotential(const SaGiftArray::Potential & rS) { Pot = rS; }

SaGiftArray::SaGiftArray() : TSCollection <SaGiftItem>()
{
}

SaGiftArray::SaGiftArray(const SaGiftArray & rS)
{
	Copy(&rS);
}

SaGiftArray & FASTCALL SaGiftArray::operator = (const SaGiftArray & rS)
{
	Copy(&rS);
	return *this;
}

int FASTCALL SaGiftArray::Copy(const SaGiftArray * pS)
{
	int    ok = 1;
	freeAll();
	if(pS) {
		for(uint i = 0; i < pS->getCount(); i++) {
			SaGiftItem * p_item = new SaGiftItem(*pS->at(i));
			THROW_MEM(p_item);
			THROW_SL(insert(p_item));
		}
		Index = pS->Index;
	}
	else {
		Index.freeAll();
	}
	CATCHZOK
	return ok;
}

int SaGiftArray::CreateIndex()
{
	int    ok = 1;
	Index.clear();
	uint   i = getCount();
	if(i) do {
		const SaGiftItem * p_item = at(--i);
		for(uint j = 0; j < p_item->List.getCount(); j++) {
			const PPIDArray & r_goods_list = p_item->List.at(j)->GoodsList;
			for(uint k = 0; k < r_goods_list.getCount(); k++)
				THROW_SL(Index.Add(p_item->StrucID, r_goods_list.get(k), 0));
		}
	} while(i);
	CATCHZOK
	return ok;
}

int SaGiftArray::SelectGift(const TSVector <SaSaleItem> & rSaleList, const RAssocArray & rExGiftList, int overlap, SaGiftArray::Gift & rGift) const
{
	rGift.Z();
	int    ok = -1;
	const  PPCommConfig & r_ccfg = CConfig;
	uint   i;
	PPID   goods_id = 0;
	SString msg_buf, fmt_buf;
	double max_amt_restr = 0.0; // Суммовое ограничение последнего найденного подарка
	// @v10.3.0 (never used) double deficit = SMathConst::Max; // Минимальный дефицит (остаток, на который необходимо добрать товара чтобы получить подарок).
	PPIDArray struc_list;
	PPIDArray temp_list;
	RAssocArray check_list;
	for(i = 0; i < rSaleList.getCount(); i++) {
		temp_list.clear();
		const  PPID sale_goods_id = rSaleList.at(i).GoodsID;
		if(Index.GetListByVal(sale_goods_id, temp_list) > 0) {
			THROW_SL(struc_list.addUnique(&temp_list));
			if(r_ccfg.Flags & CCFLG_DEBUG) {
				//PPTXT_LOG_GOODSBELONGSGIFT        "Идентифицирована подарочная ассоциация @int --> @int"
				PPLoadText(PPTXT_LOG_GOODSBELONGSGIFT, fmt_buf);
				for(uint n = 0; n < temp_list.getCount(); n++) {
					PPFormat(fmt_buf, &msg_buf, sale_goods_id, temp_list.get(n));
					PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
				}
			}
		}
	}
	for(i = 0; i < struc_list.getCount(); i++) {
		uint   p = 0;
		const  PPID struc_id = struc_list.get(i);
		if(lsearch(&struc_id, &p, CMPF_LONG)) {
			const  SaGiftItem * p_item = at(p);
			if((overlap && p_item->Flags & SaGiftItem::fOverlap) || (!overlap && !(p_item->Flags & SaGiftItem::fOverlap))) {
				double qtty = 0.0;
				double limit = (p_item->Limit > 0.0f) ? (p_item->Limit - rExGiftList.Get(p_item->StrucID)) : SMathConst::Max;
				if(limit > 0.0) {
					LongArray main_pos_list;
					if(p_item->IsSaleListSuitable(rSaleList, &check_list, &main_pos_list, &qtty)) {
						int    use = 0;
						SETMIN(qtty, limit);
						if(max_amt_restr > 0.0 && p_item->AmtRestrict > 0.0) {
							if(p_item->AmtRestrict > max_amt_restr)
								use = 1;
						}
						else if(qtty > rGift.Qtty)
							use = 1;
						if(use) {
							rGift.ID = struc_id;
							rGift.Qtty = qtty;
							rGift.CheckList = check_list;
							rGift.MainPosList = main_pos_list;
							rGift.QuotKindID = p_item->QuotKindID;
							rGift.List = at(p)->GiftList;
							max_amt_restr = p_item->AmtRestrict;
							ok = 1;
						}
					}
					else if(rGift.Pot.Name.IsEmpty()) {
						PPID   pot_goods_id = 0;
						double pot_amount = 0.0;
						double pot_deficit = 0.0;
						SString pot_name;
						if(p_item->CalcPotential(rSaleList, &pot_goods_id, &pot_amount, &pot_deficit, pot_name) > 0) {
							rGift.Pot.GoodsID = pot_goods_id;
							rGift.Pot.Amount = pot_amount;
							rGift.Pot.Deficit = pot_deficit;
							rGift.Pot.Name = pot_name;
						}
					}
				}
			}
		}
		else {
			if(r_ccfg.Flags & CCFLG_DEBUG) {
				//PPTXT_LOG_GIFTSTRUCNFINSAGIFTARRAY "Подарочная структура @int не обнаружена в списке SaGiftArray"
				PPFormatT(PPTXT_LOG_GIFTSTRUCNFINSAGIFTARRAY, &msg_buf, struc_id);
				PPLogMessage(PPFILNAM_DEBUG_LOG, msg_buf, LOGMSGF_USER|LOGMSGF_TIME);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::LoadGiftList(SaGiftArray * pList)
{
	int    ok = -1;
	uint   i;
	PPIDArray owner_goods_list, temp_goods_list;
	PPIDArray antirecur_trace; // След извлечения родительских структур, хранимый во избежании зацикливания //
	SaGiftItem * p_item = 0;
	SaGiftItem::Entry * p_entry = 0;
	PPGoodsStrucHeader rec;
	PPGoodsStrucHeader org_rec;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	pList->freeAll();
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		rec.Period.Actualize(ZERODATE);
		if(!(rec.Flags & GSF_FOLDER) && rec.Flags & GSF_PRESENT && rec.Period.CheckDate(getcurdate_())) {
			double qtty = 0.0;
			PPGoodsStruc gs;
			PPGoodsStrucItem gs_item;
			THROW(Helper_LoadItems(rec.ID, &gs));
			THROW_MEM(p_item = new SaGiftItem);
			p_item->StrucID = rec.ID;
			p_item->OrgStrucID = rec.ID;
			p_item->QuotKindID = rec.GiftQuotKindID;
			p_item->Limit      = rec.GiftLimit;
			p_item->AmtRestrict = rec.GiftAmtRestrict;
			SETFLAG(p_item->Flags, SaGiftItem::fCalcPotential, rec.Flags & GSF_GIFTPOTENTIAL);
			SETFLAG(p_item->Flags, SaGiftItem::fOverlap, rec.Flags & GSF_OVRLAPGIFT);

			PPID   parent_id = rec.ParentID;
			antirecur_trace.clear();
			antirecur_trace.add(parent_id);
			while(parent_id) {
				if(Search(parent_id, &org_rec) > 0) {
					if(antirecur_trace.addUnique(parent_id = org_rec.ParentID) < 0) {
						//
						// Цепочка структур зациклилась
						//
						p_item->OrgStrucID = 0;
						break;
					}
					else
						p_item->OrgStrucID = org_rec.ID;
				}
				else {
					p_item->OrgStrucID = 0;
					parent_id = 0;
				}
			}
			if(p_item->OrgStrucID) {
				for(i = 0; gs.EnumItemsExt(&i, &gs_item, 0, 1.0, &qtty) > 0;)
					if(goods_obj.Fetch(gs_item.GoodsID, &goods_rec) > 0) {
						THROW_MEM(p_entry = new SaGiftItem::Entry);
						p_entry->OrgGoodsID = gs_item.GoodsID;
						p_entry->MinQtty = qtty;
						p_entry->GsiFlags = gs_item.Flags;
						if(gs_item.Flags & GSIF_GOODSGROUP) {
							GoodsIterator::GetListByGroup(gs_item.GoodsID, &p_entry->GoodsList);
						}
						else if(goods_obj.IsGeneric(gs_item.GoodsID)) {
							THROW(goods_obj.GetGenericList(gs_item.GoodsID, &p_entry->GoodsList));
						}
						else {
							THROW_SL(p_entry->GoodsList.add(gs_item.GoodsID));
						}
						if(p_entry->GoodsList.getCount()) {
							THROW_SL(p_item->List.insert(p_entry));
						}
						else
							ZDELETE(p_entry);
						p_entry = 0;
					}
				if(p_item->List.getCount()) {
					THROW_SL(pList->insert(p_item));
				}
				else
					delete p_item;
			}
			else // Мы попали в "тупик" при попытке идентификации оригинальной структуры
				delete p_item;
			p_item = 0;
		}
	}
	i = pList->getCount();
	if(i) do {
		SaGiftItem * p_item = pList->at(--i);
		if(p_item->StrucID) {
			owner_goods_list.clear();
			THROW(goods_obj.SearchGListByStruc(p_item->OrgStrucID, false/*expandGenerics*/, temp_goods_list));
			for(uint j = 0; j < temp_goods_list.getCount(); j++) {
				owner_goods_list.addUnique(temp_goods_list.get(j));
			}
			if(owner_goods_list.getCount())
				p_item->GiftList = owner_goods_list;
			else
				pList->atFree(i);
		}
	} while(i);
	if(pList->getCount()) {
		pList->CreateIndex();
		ok = 1;
	}
	CATCHZOK
	return ok;
}

SaSubstBlock::Item::Item(PPID goodsID, uint rank, double rate) : GoodsID(goodsID), Rank(rank), Rate(rate)
{
	assert(GoodsID > 0);
	assert(Rate > 0.0);
}

SaSubstBlock::Entry::Entry() : GsID(0)
{
}

SaSubstBlock::SaSubstBlock()
{
}
	
SaSubstBlock & SaSubstBlock::Z()
{
	L.freeAll();
	Index.clear();
	return *this;
}

int SaSubstBlock::Get(PPID goodsID, RAssocArray & rSubstList) const
{
	rSubstList.clear();
	int    ok = -1;
	TSVector <Item> temp_list;
	for(uint i = 0; i < L.getCount(); i++) {
		const Entry * p_entry = L.at(i);
		if(p_entry) {
			uint   idx = 0;
			if(p_entry->V.lsearch(&goodsID, &idx, CMPF_LONG)) {
				const Item & r_item = p_entry->V.at(idx);
				assert(r_item.Rate > 0.0); // Не должно такого быть, что в список попало инвалидное количество!
				if(r_item.Rate > 0.0) {
					for(uint j = 0; j < p_entry->V.getCount(); j++) {
						if(j != idx) {
							const Item & r_subst_item = p_entry->V.at(j);
							assert(r_subst_item.Rate > 0.0); // Не должно такого быть, что в список попало инвалидное количество!
							if(r_subst_item.GoodsID != goodsID && r_subst_item.Rate > 0.0) { // @paranoic (if j != idx above)
								//rSubstList.Add(r_subst_item.GoodsID, r_subst_item.Rate / r_item.Rate);
								Item new_item(r_subst_item.GoodsID, r_subst_item.Rank, r_subst_item.Rate / r_item.Rate);
								THROW_SL(temp_list.insert(&new_item));
							}
						}
					}
				}
			}
		}
	}
	if(temp_list.getCount()) {
		temp_list.sort(PTR_CMPFUNC(_2long));
		for(uint i = 0; i < temp_list.getCount(); i++) {
			const Item & r_item = temp_list.at(i);
			// Элементы результирующего списка расставлены в порядке, соответствующем рангу. Таким образом,
			// порядок применения подставновочных компонентов - управляемый.
			rSubstList.Add(r_item.GoodsID, r_item.Rate); // Rate мы уже (надеюсь) правильно посчитали выше.
		}
	}
	CATCHZOK
	return ok;
}

SaSubstBlock::Entry * SaSubstBlock::GetEntry(PPID gsID)
{
	Entry * p_result = 0;
	if(gsID > 0) {
		for(uint i = 0; !p_result && i < L.getCount(); i++) {
			Entry * p_iter = L.at(i);
			if(p_iter && p_iter->GsID == gsID) {
				p_result = p_iter;
			}
		}
		if(!p_result) {
			p_result = L.CreateNewItem();
			p_result->GsID = gsID;
		}
	}
	return p_result;
}
	
int  SaSubstBlock::AddItem(SaSubstBlock::Entry * pEntry, PPID goodsID, uint rank, double rate)
{
	assert(pEntry);
	assert(goodsID > 0);
	assert(rate > 0.0);
	int    ok = 1;
	uint   idx = 0;
	THROW(pEntry); // @todo @err
	if(pEntry->V.lsearch(&goodsID, &idx, CMPF_LONG)) {
		assert(pEntry->V.at(idx).GoodsID == goodsID);
		THROW(pEntry->V.at(idx).GoodsID == goodsID && pEntry->V.at(idx).Rate == rate); // @todo @err Попытка повторно вставить тот же товар.
		ok = -1;
	}
	else {
		Item new_item(goodsID, rank, rate);
		THROW_SL(pEntry->V.insert(&new_item));
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsStruc::LoadSubstBlock(SaSubstBlock & rBlk) // @v11.6.6
{
	rBlk.Z();
	int    ok = 1;
	PPGoodsStrucHeader2 rec;
	PPIDArray struc_id_list;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		if(!(rec.Flags & GSF_FOLDER) && PPGoodsStruc::GetStrucKind(rec.Flags) == PPGoodsStruc::kSubst) {
			rec.Period.Actualize(ZERODATE);
			if(rec.Period.CheckDate(getcurdate_()))
				struc_id_list.add(rec.ID);
		}
	}
	if(struc_id_list.getCount()) {
		PPObjGoods goods_obj;
		PPGoodsStruc gs;
		PPIDArray owner_list; // Список идентификаторов товаров-владельцев структуры
		for(uint i = 0; i < struc_id_list.getCount(); i++) {
			const  PPID struc_id = struc_id_list.get(i);
			if(Get(struc_id, &gs) > 0) {
				goods_obj.SearchGListByStruc(struc_id, true/*expandGenerics*/, owner_list);
				if(owner_list.getCount()) {
					SaSubstBlock::Entry * p_subst_entry = rBlk.GetEntry(struc_id);
					PPID   single_owner_id = 0;
					{
						for(uint owneridx = 0; owneridx < owner_list.getCount(); owneridx++) {
							const  PPID owner_id = owner_list.get(owneridx);
							// @attention Следующий вызов устанавливает ранг позиции в 0. То есть,
							// компоненты обобщения будут иметь больший приоритет использования нежели любой из элементов структуры!
							rBlk.AddItem(p_subst_entry, owner_id, 0, 1.0); 
							if(!single_owner_id)
								single_owner_id = owner_id;
						}
					}
					assert(single_owner_id); // Мы выше проверили, что списко владельцев не пустой. Значит как минимум один владелец структуры должен быть!
					if(single_owner_id) {
						for(uint itemidx = 0; itemidx < gs.Items.getCount(); itemidx++) {
							PPGoodsStrucItem gsi;
							double item_qtty = 0.0;
							if(gs.GetItemExt(itemidx, &gsi, single_owner_id, 1.0, &item_qtty) > 0) {
								if(item_qtty > 0.0) {
									rBlk.AddItem(p_subst_entry, gsi.GoodsID, itemidx+1, item_qtty);
								}
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

int PPObjGoodsStruc::LoadAutoDecomplList(TSVector <SaAutoDecomplItem> & rList) // @v11.6.2
{
	rList.clear();
	int    ok = -1;
	PPGoodsStrucHeader2 rec;
	PPIDArray struc_id_list;
	for(SEnum en = P_Ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
		const int gs_kind = PPGoodsStruc::GetStrucKind(rec.Flags);
		if(!(rec.Flags & GSF_FOLDER) && gs_kind == PPGoodsStruc::kBOM && (rec.Flags & GSF_DECOMPL) && (rec.Flags & GSF_AUTODECOMPL)) {
			rec.Period.Actualize(ZERODATE);
			if(rec.Period.CheckDate(getcurdate_()))
				struc_id_list.add(rec.ID);
		}
	}
	if(struc_id_list.getCount()) {
		PPObjGoods goods_obj;
		PPGoodsStruc gs;
		PPIDArray owner_list; // Список идентификаторов товаров-владельцев структуры
		for(uint i = 0; i < struc_id_list.getCount(); i++) {
			const  PPID struc_id = struc_id_list.get(i);
			if(Get(struc_id, &gs) > 0) {
				goods_obj.SearchGListByStruc(struc_id, true/*expandGenerics*/, owner_list);
				if(owner_list.getCount()) {
					for(uint j = 0; j < owner_list.getCount(); j++) {
						const  PPID owner_id = owner_list.get(j);
						PPGoodsStrucItem gs_item;
						double gsi_qtty = 0.0;
						for(uint cidx = 0; gs.EnumItemsExt(&cidx, &gs_item, owner_id, 1.0, &gsi_qtty) > 0;) {
							if(gsi_qtty > 0.0) {
								SaAutoDecomplItem new_entry;
								new_entry.GoodsID = gs_item.GoodsID;
								new_entry.StrucID = gs.Rec.ID;
								new_entry.Qtty = fabs(gsi_qtty);
								rList.insert(&new_entry);
							}
						}
					}
				}
			}
		}
	}
	if(rList.getCount()) {
		rList.sort(PTR_CMPFUNC(_2long));
		ok = 1;
	}
	return ok;
}
//
//
//
class GoodsStrucCache : public ObjCache {
public:
	GoodsStrucCache() : ObjCache(PPOBJ_GOODSSTRUC, sizeof(D)), P_GiftList(0), P_SubstBlock(0)
	{
	}
	~GoodsStrucCache()
	{
		delete P_GiftList;
		delete P_SubstBlock; // @v11.6.6
	}
	int    GetSaGiftList(SaGiftArray * pList, int clear);
	int    GetSaSubst(PPID goodsID, RAssocArray & rList);
private:
	virtual void FASTCALL Dirty(PPID id);
	virtual int  FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct D : public ObjCacheEntry {
		PPID   VariedPropObjType;
		DateRange Period;
		double CommDenom;
		float  GiftLimit;
		long   Flags;
		PPID   ParentID;
	};
	SaGiftArray * P_GiftList;
	SaSubstBlock * P_SubstBlock; // @v11.6.6
};

int GoodsStrucCache::GetSaSubst(PPID goodsID, RAssocArray & rList)
{
	int    ok = 1;
	ENTER_CRITICAL_SECTION
	if(!P_SubstBlock) {
		PPObjGoodsStruc gs_obj;
		P_SubstBlock = new SaSubstBlock;
		gs_obj.LoadSubstBlock(*P_SubstBlock);
	}
	{
		assert(P_SubstBlock);
		if(P_SubstBlock)
			ok = P_SubstBlock->Get(goodsID, rList);
	}
	LEAVE_CRITICAL_SECTION
	return ok;
}

int GoodsStrucCache::GetSaGiftList(SaGiftArray * pList, int clear)
{
	int    ok = -1;
	ENTER_CRITICAL_SECTION
	if(clear) {
		ZDELETE(P_GiftList);
	}
	else {
		if(P_GiftList) {
			ASSIGN_PTR(pList, *P_GiftList);
			ok = 1;
		}
		else {
			int    r = 0;
			PPObjGoodsStruc gs_obj;
			THROW_MEM(P_GiftList = new SaGiftArray);
			THROW(r = gs_obj.LoadGiftList(P_GiftList));
			if(r > 0) {
				ASSIGN_PTR(pList, *P_GiftList);
				ok = 1;
			}
			else {
				ZDELETE(P_GiftList);
			}
		}
	}
	CATCH
		ZDELETE(P_GiftList);
		ok = 0;
	ENDCATCH
	LEAVE_CRITICAL_SECTION
	return ok;
}

int GoodsStrucCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	D * p_cache_rec = static_cast<D *>(pEntry);
	PPObjGoodsStruc gs_obj;
	PPGoodsStrucHeader rec;
	if(gs_obj.Search(id, &rec) > 0) {
#define CPY_FLD(Fld) p_cache_rec->Fld=rec.Fld
		CPY_FLD(VariedPropObjType);
		CPY_FLD(Period);
		CPY_FLD(CommDenom);
		CPY_FLD(GiftLimit);
		CPY_FLD(Flags);
		CPY_FLD(ParentID);
#undef CPY_FLD
		ok = PutName(rec.Name, p_cache_rec);
	}
	else
		ok = -1;
	return ok;
}

void GoodsStrucCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPGoodsStrucHeader * p_data_rec = static_cast<PPGoodsStrucHeader *>(pDataRec);
	const D * p_cache_rec = static_cast<const D *>(pEntry);
	memzero(p_data_rec, sizeof(*p_data_rec));
#define CPY_FLD(Fld) p_data_rec->Fld=p_cache_rec->Fld
	p_data_rec->Tag = PPOBJ_GOODSSTRUC;
	CPY_FLD(ID);
	CPY_FLD(VariedPropObjType);
	CPY_FLD(Period);
	CPY_FLD(CommDenom);
	CPY_FLD(GiftLimit);
	CPY_FLD(Flags);
	CPY_FLD(ParentID);
#undef CPY_FLD
	GetName(pEntry, p_data_rec->Name, sizeof(p_data_rec->Name));
}

void FASTCALL GoodsStrucCache::Dirty(PPID id)
{
	{
		SRWLOCKER(RwL, SReadWriteLocker::Write);
		Helper_Dirty(id);
	}
	if(P_GiftList) {
		PPGoodsStrucHeader temp_rec;
		int    r = Get(id, &temp_rec);
		if((r > 0 && temp_rec.Flags & GSF_PRESENT) || r < 0) {
			GetSaGiftList(0, 1); // Функция защищена внутренней критической секцией
		}
	}
}

IMPL_OBJ_FETCH(PPObjGoodsStruc, PPGoodsStrucHeader, GoodsStrucCache);

int PPObjGoodsStruc::FetchGiftList(SaGiftArray * pList)
{
	GoodsStrucCache * p_cache = GetDbLocalCachePtr <GoodsStrucCache> (Obj);
	return p_cache ? p_cache->GetSaGiftList(pList, 0) : LoadGiftList(pList);
}

int PPObjGoodsStruc::FetchSubstList(PPID goodsID, RAssocArray & rSubstList)
{
	GoodsStrucCache * p_cache = GetDbLocalCachePtr <GoodsStrucCache> (Obj);
	return p_cache ? p_cache->GetSaSubst(goodsID, rSubstList) : (rSubstList.clear(), -1);
}
