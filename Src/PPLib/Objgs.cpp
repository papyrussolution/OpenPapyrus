// OBJGS.CPP
// Copyright (c) A.Sobolev 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

#define GRP_GOODS 1

struct _GSItem {           // @persistent @store(ObjAssocTbl)
	PPID   ID;             //
	PPID   Tag;            // Const=PPASS_GOODSSTRUC
	PPID   GSID;           // ->Ref(PPOBJ_GOODSSTRUC) ID структуры
	PPID   ItemGoodsID;    // ->Goods.ID              ID элемента структуры
	long   Num;            // Внутренний номер (не используется, но инициализируется)
	double Netto;          // Нетто количество компонента
	char   Symb[20];       // Символ элемента структуры (для ссылки из формул)
	char   Reserve1[4];    // @v8.6.5 [8]-->[4]
	long   PrefInnefGsID;  // @v8.6.5
	long   Flags;          // Флаги
	double Median;         // Среднее значение
	double Width;          // Ширина оценочного интервала
	double Denom;          // Знаменатель дроби qtty = Median / Denom
};
//
//
//
SLAPI PPGoodsStruc::Ident::Ident(PPID goodsID, long andF, long notF, LDATE dt)
{
	THISZERO();
	GoodsID = goodsID;
	AndFlags = andF;
	NotFlags = notF;
	Dt = dt;
}

SLAPI PPGoodsStruc::PPGoodsStruc()
{
	Init();
}

void SLAPI PPGoodsStruc::Init()
{
	GoodsID = 0;
	P_Cb = 0;
	MEMSZERO(Rec);
	Items.freeAll();
	Childs.freeAll();
}

int SLAPI PPGoodsStruc::IsEmpty() const
{
	return (Items.getCount() || Childs.getCount()) ? 0 : 1;
}

int FASTCALL PPGoodsStruc::IsEqual(const PPGoodsStruc & rS) const
{
	int   eq = 1;
#define CMPRECF(f) if(Rec.f != rS.Rec.f) return 0;
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
		eq = 0;
	else if(stricmp866(Rec.Symb, rS.Rec.Symb) != 0)
		eq = 0;
	else {
		{
			const uint c = Items.getCount();
			if(c != rS.Items.getCount())
				eq = 0;
			else {
				for(uint i = 0; eq && i < c; i++) {
					if(!Items.at(i).IsEqual(rS.Items.at(i)))
						eq = 0;
				}
			}
		}
		if(eq) {
			const uint c = Childs.getCount();
			if(c != rS.Childs.getCount())
				eq = 0;
			else {
				for(uint i = 0; eq && i < c; i++) {
					const PPGoodsStruc * p_child = Childs.at(i);
					const PPGoodsStruc * p_s_child = rS.Childs.at(i);
					if(p_child && p_s_child) {
						if(!p_child->IsEqual(*p_s_child))
							eq = 0;
					}
					else if(p_child && !p_s_child) // @paranoic
						eq = 0;
					else if(!p_child && p_s_child) // @paranoic
						eq = 0;
				}
			}
		}
	}
	return eq;
}

int SLAPI PPGoodsStruc::IsNamed() const
{
	return (Rec.Flags & GSF_NAMED) ? 1 : 0;
}

int SLAPI PPGoodsStruc::SetKind(int kind)
{
	int    ok = 1;
	switch(kind) {
		case kUndef:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT|GSF_COMPLEX);
			break;
		case kBOM:
			Rec.Flags &= ~(GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT|GSF_COMPLEX);
			if(!(Rec.Flags & (GSF_COMPL|GSF_DECOMPL)))
				Rec.Flags |= GSF_COMPL;
			break;
		case kPart:
			Rec.Flags &= ~(GSF_SUBST|GSF_PRESENT|GSF_COMPLEX);
			Rec.Flags |= GSF_PARTITIAL;
			break;
		case kSubst:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_PRESENT|GSF_COMPLEX);
			Rec.Flags |= GSF_SUBST;
			break;
		case kGift:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_COMPLEX);
			Rec.Flags |= GSF_PRESENT;
			break;
		case kComplex:
			Rec.Flags &= ~(GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_SUBST|GSF_PRESENT);
			Rec.Flags |= GSF_COMPLEX;
			break;
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

int SLAPI PPGoodsStruc::GetKind() const
{
	if(Rec.Flags & GSF_COMPLEX)
		return kComplex;
	else if(Rec.Flags & GSF_PRESENT)
		return kGift;
	else if(Rec.Flags & GSF_SUBST)
		return kSubst;
	else if(Rec.Flags & GSF_PARTITIAL)
		return kPart;
	else if(Rec.Flags & (GSF_COMPL|GSF_DECOMPL))
		return kBOM;
	else
		return kUndef;
}

int SLAPI PPGoodsStruc::GetTypeString(SString & rBuf)
{
	rBuf.Z();
	if(Rec.ID)
		rBuf.CatChar('E');
	if(Rec.Flags & GSF_COMPL)
		rBuf.CatChar('C');
	if(Rec.Flags & GSF_DECOMPL)
		rBuf.CatChar('D');
	if(Rec.Flags & GSF_PARTITIAL)
		rBuf.CatChar('P');
	if(Rec.Flags & GSF_SUBST)
		rBuf.CatChar('S');
	if(Rec.Flags & GSF_PRESENT)
		rBuf.CatChar('G');
	if(Rec.ParentID)
		rBuf.CatChar('F');
	return 1;
}

int SLAPI PPGoodsStruc::CanExpand() const
{
	return (Rec.Flags & (GSF_CHILD|GSF_FOLDER)) ? 0 : 1;
}

int SLAPI PPGoodsStruc::CanReduce() const
{
	return (Rec.Flags & GSF_FOLDER && Childs.getCount() <= 1) ? 1 : 0;
}

int SLAPI PPGoodsStruc::Select(const Ident * pIdent, PPGoodsStruc * pGs) const
{
	if(Rec.Flags & GSF_FOLDER) {
		for(uint i = 0; i < Childs.getCount(); i++) {
			PPGoodsStruc * p_child = Childs.at(i);
			if(p_child && p_child->Select(pIdent, pGs)) // @recursion
				return 1;
		}
	}
	else {
		DateRange period = Rec.Period;
		if(!pIdent->Dt || period.Actualize(ZERODATE).CheckDate(pIdent->Dt)) {
			if(!pIdent->AndFlags || (Rec.Flags & pIdent->AndFlags) == pIdent->AndFlags) {
				if(!(Rec.Flags & pIdent->NotFlags)) {
					// ((Rec.Flags & pIdent->NotFlags) != pIdent->NotFlags)-->!(Rec.Flags & pIdent->NotFlags)
					if(pGs) {
						*pGs = *this;
						pGs->GoodsID = pIdent->GoodsID;
					}
					return 1;
				}
			}
		}
	}
	return 0;
}

int SLAPI PPGoodsStruc::Helper_Select(const Ident * pIdent, TSCollection <PPGoodsStruc> & rList) const
{
	int    ok = -1;
	if(Rec.Flags & GSF_FOLDER) {
		for(uint i = 0; i < Childs.getCount(); i++) {
			PPGoodsStruc * p_child = Childs.at(i);
			if(p_child) {
				int    r = p_child->Helper_Select(pIdent, rList); // @recursion
				THROW(r);
				if(r > 0)
					ok = 1;
			}
		}
	}
	else {
		DateRange period = Rec.Period;
		if(!pIdent->Dt || period.Actualize(ZERODATE).CheckDate(pIdent->Dt)) {
			if(!pIdent->AndFlags || (Rec.Flags & pIdent->AndFlags) == pIdent->AndFlags) {
				if(!(Rec.Flags & pIdent->NotFlags)) {
					// ((Rec.Flags & pIdent->NotFlags) != pIdent->NotFlags)-->!(Rec.Flags & pIdent->NotFlags)

					//
					// Защита от рекурсивных структур: если в списке уже есть структура с таким ID, то мы - в рекурсии.
					//
					int    found = 0;
					for(uint i = 0; !found && i < rList.getCount(); i++) {
						const PPGoodsStruc * p_item = rList.at(i);
						if(p_item && p_item->Rec.ID == Rec.ID)
							found = 1;
					}
					if(!found) {
						PPGoodsStruc * p_new_item = new PPGoodsStruc;
						THROW_MEM(p_new_item);
						*p_new_item = *this;
						p_new_item->GoodsID = pIdent->GoodsID;
						THROW_SL(rList.insert(p_new_item));
						ok = 1;
					}
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

int SLAPI PPGoodsStruc::Select(const Ident * pIdent, TSCollection <PPGoodsStruc> & rList) const
{
	rList.freeAll();
	return Helper_Select(pIdent, rList);
}

PPGoodsStruc & FASTCALL PPGoodsStruc::operator = (const PPGoodsStruc & src)
{
	Init();
	GoodsID = src.GoodsID;
	Rec = src.Rec;
	Items.copy(src.Items);
	for(uint i = 0; i < src.Childs.getCount(); i++) {
		PPGoodsStruc * p_child = new PPGoodsStruc;
		*p_child = *src.Childs.at(i);
		Childs.insert(p_child);
	}
	return *this;
}

const PPGoodsStrucItem * SLAPI PPGoodsStruc::GetMainItem(uint * pPos) const
{
	for(uint i = 0; i < Items.getCount(); i++)
		if(Items.at(i).Flags & GSIF_MAINITEM) {
			ASSIGN_PTR(pPos, i);
			return &Items.at(i);
		}
	return (PPSetError(PPERR_GSTRUCHASNTMAINC), 0);
}

int SLAPI PPGoodsStruc::RecalcQttyByMainItemPh(double * pQtty) const
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

int SLAPI PPGoodsStruc::GetEstimationPrice(uint itemIdx, double * pPrice, double * pTotalPrice, ReceiptTbl::Rec * pRec) const
{
	int    ok = -1;
	double p = 0.0, t = 0.0;
	ReceiptTbl::Rec rec;
	MEMSZERO(rec);
	if(itemIdx < Items.getCount()) {
		const PPGoodsStrucItem & r_item = Items.at(itemIdx);
		const PPID loc_id = LConfig.Location;
		if(r_item.GoodsID) {
			int    r = 0;
			PPObjGoods goods_obj;
			const PPGoodsConfig & r_cfg = goods_obj.GetConfig();
			Goods2Tbl::Rec grec;
			MEMSZERO(grec);
			goods_obj.Fetch(r_item.GoodsID, &grec);
			if(grec.Flags & GF_UNLIM) {
				QuotIdent q_i(loc_id, PPQUOTK_BASE);
				r = goods_obj.GetQuot(r_item.GoodsID, q_i, 0, 0, &p, 1);
			}
			else if(::GetCurGoodsPrice(r_item.GoodsID, loc_id, GPRET_MOSTRECENT, &p, &rec) > 0) {
				if(r_cfg.Flags & GCF_SHOWGSTRUCPRICE)
					p = rec.Price;
				else
					p = rec.Cost;
				r = 1;
			}
			else {
				RAssocArray subst_list;
				if(goods_obj.GetSubstList(r_item.GoodsID, 0, subst_list) > 0) {
					for(uint i = 0; !r && i < subst_list.getCount(); i++) {
						const PPID alt_goods_id = subst_list.at(i).Key;
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

int SLAPI PPGoodsStruc::CalcEstimationPrice(double * pPrice, int * pUncertainty, int calcInner) const
{
	int    uncertainty = 0;
	double price = 0.0;
	for(uint i = 0; i < Items.getCount(); i++) {
		PPGoodsStrucItem * p_item = & Items.at(i);
		double iprice = 0.0, tprice = 0.0;
		int    is_inner_struc = 0;
		if(calcInner) {
			PPGoodsStruc::Ident ident(p_item->GoodsID, GSF_COMPL, GSF_PARTITIAL|GSF_SUBST);
			PPGoodsStruc inner_struc;
			if(LoadGoodsStruc(&ident, &inner_struc) > 0) {
				int    uncert = 0;
				inner_struc.CalcEstimationPrice(&iprice, &uncert, 1); // @recursion
				p_item->GetQtty(iprice / GetDenom(), &iprice);
				price += iprice;
				if(uncert)
					uncertainty = 1;
				SETFLAG(p_item->Flags, GSIF_UNCERTPRICE, uncert);
				is_inner_struc = 1;
			}
		}
		if(!is_inner_struc) {
			if(GetEstimationPrice(i, &iprice, &tprice, 0) > 0 && tprice > 0) {
				price += R2(tprice);
				p_item->Flags &= ~GSIF_UNCERTPRICE;
			}
			else {
				p_item->Flags |= GSIF_UNCERTPRICE;
				uncertainty = 1;
			}
		}
	}
	ASSIGN_PTR(pPrice, price);
	ASSIGN_PTR(pUncertainty, uncertainty);
	return 1;
}

int FASTCALL PPGoodsStruc::HasGoods(PPID goodsID) const
{
	int    ok = 0;
	PPGoodsStrucItem * p_item;
	if(goodsID)
		for(uint i = 0; !ok && Items.enumItems(&i, (void **)&p_item);) {
			if(p_item->GoodsID == goodsID) {
				ok = (int)i;
			}
		}
	return ok;
}

int SLAPI PPGoodsStruc::SearchSymb(const char * pSymb, uint * pPos) const
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

int SLAPI PPGoodsStruc::CopyItemsFrom(const PPGoodsStruc * pS)
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

int SLAPI PPGoodsStruc::MoveItem(uint pos, int dir  /* 0 - down, 1 - up */, uint * pNewPos)
{
	return Items.moveItem(pos, dir, pNewPos);
}

int SLAPI PPGoodsStruc::SubstVariedProp(PPID parentGoodsID, PPGoodsStrucItem * pItem) const
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

double SLAPI PPGoodsStruc::GetDenom() const
{
	return (Rec.CommDenom != 0.0 && Rec.CommDenom != 1.0) ? Rec.CommDenom : 1.0;
}

int SLAPI PPGoodsStruc::EnumItemsExt(uint * pPos, PPGoodsStrucItem * pItem, PPID parentGoodsID, double srcQtty, double * pQtty) const
{
	int    ok = -1;
	uint   p = pPos ? *pPos : 0;
	if(p < Items.getCount()) {
		double qtty = 0.0;
		PPGoodsStrucItem item = Items.at(p);
		item.GetQtty(srcQtty / GetDenom(), &qtty);
		THROW(SubstVariedProp(parentGoodsID, &item));
		ASSIGN_PTR(pItem, item);
		ASSIGN_PTR(pQtty, qtty);
		p++;
		ASSIGN_PTR(pPos, p);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

SString & SLAPI PPGoodsStruc::MakeChildDefaultName(SString & rBuf) const
{
	rBuf.Z().Cat("BOM").Space().CatChar('#').Cat(Childs.getCount()+1);
	return rBuf;
}

int SLAPI PPGoodsStruc::Expand()
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
		Rec.Period.SetZero();
		Rec.CommDenom = 0;
		Rec.ParentID = 0;
		THROW(p_child->CopyItemsFrom(this));
		if(p_child->Rec.Name[0] == 0) {
			SString name_buf;
			MakeChildDefaultName(name_buf).CopyTo(p_child->Rec.Name, sizeof(p_child->Rec.Name));
		}
		Items.freeAll();
		Childs.insert(p_child);
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

int SLAPI PPGoodsStruc::Reduce()
{
	int    ok = 1;
	if(Rec.Flags & GSF_FOLDER && !(Rec.Flags & GSF_CHILD)) {
		PPGoodsStruc * p_child = Childs.getCount() ? Childs.at(0) : 0;
		THROW_PP(Childs.getCount() <= 1, PPERR_REDUCEMULTGSTRUCFOLDER);
		if(p_child) {
			Rec.Flags |= (p_child->Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL|GSF_OUTPWOVAT));
			Rec.Period = p_child->Rec.Period;
			Rec.CommDenom = p_child->Rec.CommDenom;
			Rec.VariedPropObjType = p_child->Rec.VariedPropObjType;
			THROW(CopyItemsFrom(p_child));
		}
		Childs.freeAll();
		Rec.Flags &= ~GSF_FOLDER;
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SLAPI PPGoodsStruc::GetItemQtty(PPID goodsID, double complQtty, double * pQtty) const
{
	for(uint i = 0; i < Items.getCount(); i++) {
		const PPGoodsStrucItem & r_item = Items.at(i);
		if(r_item.GoodsID == goodsID)
			return r_item.GetQtty(complQtty, pQtty);
	}
	PPSetAddedMsgObjName(PPOBJ_GOODS, goodsID);
	return PPSetError(PPERR_GSTRUCHASNTGOODS);
}

static int SLAPI IsNumber(const char * pStr, size_t * pPos)
{
	int    was_sign = 0;
	int    was_dot = 0;
	size_t pos = pPos ? *pPos : 0;
	SString temp_buf;
	while(pStr[pos] == ' ' || pStr[pos] == '\t')
		pos++;
	if(pStr[pos] == '-' || pStr[pos] == '+') {
		if(was_sign) {
			ASSIGN_PTR(pPos, pos);
			return BIN(temp_buf.ToReal() != 0.0);
		}
		was_sign = 1;
		temp_buf.CatChar(pStr[pos++]);
	}
	while(pStr[pos] == ' ' || pStr[pos] == '\t')
		pos++;
	while(isdec(pStr[pos]) || pStr[pos] == '.' || pStr[pos] == ',') {
		if(pStr[pos] == '.' || pStr[pos] == ',') {
			if(was_dot) {
				ASSIGN_PTR(pPos, pos);
				return (temp_buf.ToReal() != 0) ? 1 : 0;
			}
			was_dot = 1;
		}
		temp_buf.CatChar(pStr[pos++]);
	}
	ASSIGN_PTR(pPos, pos);
	return BIN(temp_buf.ToReal() != 0.0);
}

//static
int SLAPI PPGoodsStruc::IsSimpleQttyString(const char * pStr)
{
	size_t pos = 0;
	if(IsNumber(pStr, &pos)) {
		while(pStr[pos] == ' ' || pStr[pos] == '\t')
			pos++;
		if(pStr[pos] == 0)
			return 1;
		else if(pStr[pos] == '/') {
			pos++;
			if(IsNumber(pStr, &pos)) {
				while(pStr[pos] == ' ' || pStr[pos] == '\t')
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
SLAPI PPGoodsStrucItem::PPGoodsStrucItem()
{
	THISZERO();
}

int FASTCALL PPGoodsStrucItem::IsEqual(const PPGoodsStrucItem & rS) const
{
#define CMPF(f) if(f != rS.f) return 0;
	CMPF(GoodsID);
	CMPF(Flags);
	CMPF(Median);
	CMPF(Width);
	CMPF(Denom);
	CMPF(Netto);
#undef CMPF
	if(!sstreq(Symb, rS.Symb))
		return 0;
	if(!sstreq(Formula, rS.Formula))
		return 0;
	return 1;
}

int FASTCALL PPGoodsStrucItem::operator == (const PPGoodsStrucItem & rS) const
{
	return IsEqual(rS);
}

int FASTCALL PPGoodsStrucItem::operator != (const PPGoodsStrucItem & rS) const
{
	return !IsEqual(rS);
}

int SLAPI PPGoodsStrucItem::SetFormula(const char * pStr, const PPGoodsStruc * pStruc)
{
	int    ok = 1;
	double v = 0.0;
	memzero(Formula, sizeof(Formula));
	SString temp_buf = pStr;
	if(temp_buf.NotEmptyS()) {
		GdsClsCalcExprContext ctx(pStruc);
		if(PPCalcExpression(temp_buf, &v, &ctx))
			STRNSCPY(Formula, temp_buf);
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI PPGoodsStrucItem::SetEstimationString(const char * pStr)
{
	int    ok = 1;
	Median = 0.0;
	Width  = 0.0;
	Denom  = 1.0;
	if(PPGoodsStruc::IsSimpleQttyString(pStr)) {
		double v;
		SString temp_buf = pStr;
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

SString & SLAPI PPGoodsStrucItem::GetEstimationString(SString & rBuf, long format)
{
	rBuf.Z();
	long   fmt = NZOR(format, MKSFMTD(0, 6, NMBF_NOTRAILZ));
	rBuf.Cat(Median, fmt);
	if(Denom != 0 && Denom != 1)
		rBuf.CatChar('/').Cat(Denom, fmt);
	return rBuf;
}

int SLAPI PPGoodsStrucItem::GetQttyAsPrice(double complPriceSum, double * pItemPrice) const
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

int SLAPI PPGoodsStrucItem::GetQtty(double complQtty, double * pItemQtty) const
{
	int    ok = 1;
	double qtty = complQtty * Median;
	if(Denom != 0.0 && Denom != 1.0)
		qtty /= Denom;
	if(Flags & GSIF_PCTVAL)
		qtty = fdiv100r(qtty);
	else if(Flags & GSIF_PHUVAL) {
		PPObjGoods goods_obj;
		double phuperu;
		if(goods_obj.GetPhUPerU(GoodsID, 0, &phuperu) > 0)
			qtty /= phuperu;
	}
	else if(Flags & GSIF_QTTYASPRICE) {
		qtty = 1.0;
		ok = 2;
	}
	if(Flags & GSIF_ROUNDDOWN)
		qtty = floor(qtty);
	ASSIGN_PTR(pItemQtty, R6(qtty));
	return ok;
}
//
//
//
class GoodsStrucSelectorDialog : public TDialog {
public:
	struct DataBlock {
		DataBlock(PPID namedGsID)
		{
			Mode = mNamedStruc;
			Flags = 0;
			GsID = namedGsID;
			P_List = 0;
		}
		DataBlock(const TSCollection <PPGoodsStruc> * pList, PPID selectionID)
		{
			Mode = mList;
			Flags = 0;
			GsID = selectionID;
			P_List = pList;
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
	int    setDTS(DataBlock * pData)
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
				ComboBox * p_cb = (ComboBox *)getCtrlView(CTLSEL_GSDATA_NAMEDSTRUC);
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
							PPGoodsStruc temp_gs;
							temp_gs = *p_gs;
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

int SLAPI PPObjGoodsStruc::SelectorDialog(const TSCollection <PPGoodsStruc> & rList, uint * pSelectionPos)
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

int SLAPI PPObjGoodsStruc::SelectorDialog(PPID * pNamedGsID)
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
	PPID   GoodsGrpID;
	PPID   GoodsID;
	PPID   GStrucID;
};

class GSDialog : public PPListDialog {
public:
	GSDialog() : PPListDialog(DLG_GSTRUC, CTL_GSTRUC_LIST)
	{
		NewGoodsGrpID = 0;
		MEMSZERO(GscParam);
		SetupCalPeriod(CTLCAL_GSTRUC_PERIOD, CTL_GSTRUC_PERIOD);
		if(P_Box && P_Box->def)
			P_Box->def->SetOption(lbtFocNotify, 1);
		Changed = 0;
		updateList(-1);
	}
	int    setDTS(const PPGoodsStruc *);
	int    getDTS(PPGoodsStruc *);
	int    IsChanged();
private:
	DECL_HANDLE_EVENT;
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	virtual int  moveItem(long pos, long id, int up);
	void   setupCtrls();
	int    addItemExt(long * pPos, long * pID);
	int    addItemBySample();
	int    editItemDialog(int pos, PPGoodsStrucItem *);
	int    checkDupGoods(int pos, PPGoodsStrucItem *);
	void   selNamedGS();
	int    enableEditRecurStruc();

	PPObjGoodsStruc GsObj;
	PPObjGoods      GObj;
	PPID   NewGoodsGrpID;
	GoodsStrucCopyParam GscParam;
	PPGoodsStruc Data;
	PPGoodsStruc RecurData;
	int    Changed;
};

int GSDialog::IsChanged()
{
	PPGoodsStruc temp = Data, gs;
	return NZOR(Changed, (getDTS(&gs), Data = temp, !temp.IsEqual(gs)));
}

int GSDialog::setDTS(const PPGoodsStruc * pData)
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
	temp_buf = 0;
	if(Data.Rec.Flags & GSF_DYNGEN)
		PPLoadText(PPTXT_DYNGENGSTRUC, temp_buf);
	setStaticText(CTL_GSTRUC_INFO, temp_buf);
	setCtrlData(CTL_GSTRUC_NAME, Data.Rec.Name);
	setCtrlLong(CTL_GSTRUC_ID, Data.Rec.ID);
	SetupObjListCombo(this, CTLSEL_GSTRUC_VAROBJ, Data.Rec.VariedPropObjType);
	setCtrlData(CTL_GSTRUC_COMMDENOM, &Data.Rec.CommDenom);
	SetPeriodInput(this, CTL_GSTRUC_PERIOD, &Data.Rec.Period);

	long   kind = Data.GetKind();
	AddClusterAssocDef(CTL_GSTRUC_KIND, 0, PPGoodsStruc::kUndef);
	AddClusterAssoc(CTL_GSTRUC_KIND, 1, PPGoodsStruc::kBOM);
	AddClusterAssoc(CTL_GSTRUC_KIND, 2, PPGoodsStruc::kPart);
	AddClusterAssoc(CTL_GSTRUC_KIND, 3, PPGoodsStruc::kSubst);
	AddClusterAssoc(CTL_GSTRUC_KIND, 4, PPGoodsStruc::kGift);
	AddClusterAssoc(CTL_GSTRUC_KIND, 5, PPGoodsStruc::kComplex);
	SetClusterData(CTL_GSTRUC_KIND, kind);

	AddClusterAssoc(CTL_GSTRUC_FLAGS, 0, GSF_COMPL);
	AddClusterAssoc(CTL_GSTRUC_FLAGS, 1, GSF_DECOMPL);
	AddClusterAssoc(CTL_GSTRUC_FLAGS, 2, GSF_OUTPWOVAT);
	AddClusterAssoc(CTL_GSTRUC_FLAGS, 3, GSF_GIFTPOTENTIAL);
	AddClusterAssoc(CTL_GSTRUC_FLAGS, 4, GSF_OVRLAPGIFT);    // @v7.3.0
	AddClusterAssoc(CTL_GSTRUC_FLAGS, 5, GSF_POSMODIFIER);   // @v7.2.0
	SetClusterData(CTL_GSTRUC_FLAGS, Data.Rec.Flags);
	setupCtrls();
	setCtrlReal(CTL_GSTRUC_GIFTAMTRESTR, Data.Rec.GiftAmtRestrict);
	{
		ComboBox * p_cb = (ComboBox *)getCtrlView(CTLSEL_GSTRUC_GIFTQK);
		if(p_cb) {
			StrAssocArray * p_qk_list = new StrAssocArray;
			if(p_qk_list) {
				PPObjQuotKind qk_obj;
				QuotKindFilt qk_filt;
				qk_filt.Flags = QuotKindFilt::fAll;
				qk_obj.MakeList(&qk_filt, p_qk_list);
				PPLoadText(PPTXT_CHEAPESTITEMFREE, temp_buf);
				p_qk_list->Add(GSGIFTQ_CHEAPESTITEMFREE, temp_buf);
				// @v7.5.9 {
				PPLoadText(PPTXT_CHEAPESTITEMBYGIFTQ, temp_buf);
				p_qk_list->Add(GSGIFTQ_CHEAPESTITEMBYGIFTQ, temp_buf);
				// } @v7.5.9
				// @v7.4.10 {
				PPLoadText(PPTXT_LASTITEMBYGIFTQ, temp_buf);
				p_qk_list->Add(GSGIFTQ_LASTITEMBYGIFTQ, temp_buf);
				// } @v7.4.10
				p_cb->setListWindow(CreateListWindow(p_qk_list, lbtDisposeData|lbtDblClkNotify), Data.Rec.GiftQuotKindID);
			}
		}
	}
	setCtrlData(CTL_GSTRUC_GIFTLIMIT, &Data.Rec.GiftLimit); // @v7.0.0
	updateList(-1);
	enableCommand(cmGStrucExpandReduce, Data.CanExpand());
	enableEditRecurStruc();
	return 1;
}

int GSDialog::getDTS(PPGoodsStruc * pData)
{
	int    ok = 1;
	uint   sel = 0, i;
	char   buf[64];
	buf[0] = 0;
	getCtrlData(sel = CTL_GSTRUC_NAME, buf);
	if(*strip(buf) == 0) {
		THROW_PP(!(Data.Rec.Flags & GSF_NAMED), PPERR_NAMENEEDED);
	}
	else if(!(Data.Rec.Flags & GSF_CHILD))
		Data.Rec.Flags |= GSF_NAMED;
	STRNSCPY(Data.Rec.Name, buf);
	getCtrlData(CTLSEL_GSTRUC_VAROBJ, &Data.Rec.VariedPropObjType);
	//
	// Если структура переведена в расширенный режим, то забирать данные
	// надо не из всех контролов
	//
	if(!(Data.Rec.Flags & GSF_FOLDER)) {
		long   kind = PPGoodsStruc::kUndef;
		getCtrlData(CTL_GSTRUC_COMMDENOM, &Data.Rec.CommDenom);
		THROW(GetPeriodInput(this, sel = CTL_GSTRUC_PERIOD, &Data.Rec.Period));
		GetClusterData(CTL_GSTRUC_KIND, &kind);
		GetClusterData(CTL_GSTRUC_FLAGS, &Data.Rec.Flags);
		Data.SetKind(kind);
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
	CATCH
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

int GSDialog::enableEditRecurStruc()
{
	int    ok = 1, enable = 0;
	if(P_Box && P_Box->def) {
		long   pos = 0;
		P_Box->def->getCurID(&pos);
		if(pos > 0 && pos-1 < (long)Data.Items.getCount()) {
			int r = 0;
			PPGoodsStruc::Ident ident(Data.Items.at(pos - 1).GoodsID, GSF_COMPL, GSF_PARTITIAL);
			RecurData.Init();
			THROW(r = GObj.LoadGoodsStruc(&ident, &RecurData));
			enable = BIN(r > 0 && RecurData.Rec.ID);
		}
	}
	CATCHZOK
	enableCommand(cmEditStruct, enable);
	return ok;
}

void GSDialog::setupCtrls()
{
	long   kind = PPGoodsStruc::kUndef;
	GetClusterData(CTL_GSTRUC_KIND, &kind);
	GetClusterData(CTL_GSTRUC_FLAGS, &Data.Rec.Flags);
	Data.SetKind(kind);
	DisableClusterItem(CTL_GSTRUC_FLAGS, 0, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
	DisableClusterItem(CTL_GSTRUC_FLAGS, 1, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
	DisableClusterItem(CTL_GSTRUC_FLAGS, 2, !(Data.Rec.Flags & (GSF_COMPL|GSF_DECOMPL|GSF_PARTITIAL)));
	DisableClusterItem(CTL_GSTRUC_FLAGS, 3, !(Data.Rec.Flags & GSF_PRESENT));
	DisableClusterItem(CTL_GSTRUC_FLAGS, 4, !(Data.Rec.Flags & GSF_PRESENT));   // @v7.3.0
	DisableClusterItem(CTL_GSTRUC_FLAGS, 5, !(Data.Rec.Flags & GSF_PARTITIAL)); // @v7.2.0
	showCtrl(CTL_GSTRUC_GIFTAMTRESTR, (Data.Rec.Flags & GSF_PRESENT));
	showCtrl(CTLSEL_GSTRUC_GIFTQK,    (Data.Rec.Flags & GSF_PRESENT));
	showCtrl(CTL_GSTRUC_GIFTLIMIT,    (Data.Rec.Flags & GSF_PRESENT)); // @v7.0.0
	SetClusterData(CTL_GSTRUC_FLAGS, Data.Rec.Flags);
}

IMPL_HANDLE_EVENT(GSDialog)
{
	PPListDialog::handleEvent(event);
	if(event.isClusterClk(CTL_GSTRUC_KIND) || event.isClusterClk(CTL_GSTRUC_FLAGS)) {
		setupCtrls();
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
			else if(r == 0)
				PPError();
			Changed = 1;
		}
	}
	else if(event.isCmd(cmPrint))
		GsObj.Print(&Data);
	else if(event.isCmd(cmLBItemFocused)) {
		if(event.isCtlEvent(ctlList) && !enableEditRecurStruc())
			PPError();
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
				temp.Init();
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
	int    show_total = 0;
	double t_qtty = 0.0, t_netto = 0.0, t_sum = 0.0;
	PPGoodsStrucItem * p_item = 0;
	SString sub;
	long   qtty_fmt = MKSFMTD(0, 3, NMBF_NOZERO);
	long   money_fmt = MKSFMTD(0, 2, NMBF_NOZERO);
	uint   i = 0;
	while(Data.Items.enumItems(&i, (void**)&p_item)) {
		double price = 0.0, sum = 0.0;
		Goods2Tbl::Rec goods_rec;
		StringSet ss(SLBColumnDelim);
		PPGoodsStruc::Ident ident(p_item->GoodsID, GSF_COMPL, GSF_PARTITIAL);
		PPGoodsStruc inner_struc;

		THROW(GObj.LoadGoodsStruc(&ident, &inner_struc));
		if(GObj.Fetch(p_item->GoodsID, &goods_rec) > 0)
			sub = goods_rec.Name;
		else {
			MEMSZERO(goods_rec);
			ideqvalstr(p_item->GoodsID, sub);
		}
		ss.add(sub);
		p_item->GetEstimationString(sub, qtty_fmt);
		if(p_item->Flags & GSIF_PCTVAL)
			sub.CatChar('%');
		else if(p_item->Flags & GSIF_PHUVAL) {
			PPUnit u_rec;
			if(GObj.FetchUnit(goods_rec.PhUnitID, &u_rec) > 0)
				sub.Space().Cat(u_rec.Name);
			else
				sub.Space().CatChar('?');
		}
		ss.add(sub);
		t_qtty  += sub.ToReal();
		sub = 0;
		if(inner_struc.Rec.ID) {
			if(GsObj.CheckStruc(inner_struc.Rec.ID, 0) != 2) {
				int    uncert = 0;
				inner_struc.CalcEstimationPrice(&price, &uncert, 1); // @bottleneck
				p_item->GetQtty(price / Data.GetDenom(), &sum);
			}
			else {
				price = 0;
				PPError(PPERR_CYCLEGOODSSTRUC);
			}
		}
		else
			Data.GetEstimationPrice(i-1, &price, &sum, 0);
		t_netto += p_item->Netto;
		t_sum   += sum;
		ss.add(sub.Z().Cat(p_item->Netto, qtty_fmt));
		ss.add(sub.Z().Cat(price, money_fmt));
		ss.add(sub.Z().Cat(sum,   money_fmt));
		sub = 0;
		if(inner_struc.Rec.ID)
			sub.CatChar('R');
		ss.add(sub);
		THROW(addStringToList(i, ss.getBuf()));
		show_total = 1;
	}
	if(show_total) {
		StringSet ss(SLBColumnDelim);
		PPGetWord(PPWORD_TOTAL, 0, sub);
		ss.add(sub.ToLower());
		ss.add(sub.Z().Cat(t_qtty,  qtty_fmt));
		ss.add(sub.Z().Cat(t_netto, qtty_fmt));
		ss.add(0, 0);
		ss.add(sub.Z().Cat(t_sum, money_fmt));
		THROW(addStringToList(i+1, ss.getBuf()));
	}
	CATCHZOK
	return ok;
}

int GSDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		Data.Items.atFree((uint)pos);
		Changed = 1;
		return 1;
	}
	return -1;
}

int GSDialog::moveItem(long pos, long id, int up)
{
	uint   new_pos = (uint)pos;
	return Data.MoveItem(pos, up, &new_pos);
}

class GSItemDialog : public TDialog {
public:
	GSItemDialog(PPGoodsStrucItem *, const PPGoodsStruc * pStruc);
private:
	DECL_HANDLE_EVENT;
	void   setupPrice();
	const PPGoodsStruc * P_Struc;
	PPGoodsStrucItem * P_Data;
	double Price;
};

GSItemDialog::GSItemDialog(PPGoodsStrucItem * pData, const PPGoodsStruc * pStruc) : TDialog(DLG_GSITEM)
{
	P_Struc = pStruc;
	P_Data = pData;
	Price = 0.0;
	SString buf;
	ushort v;
	disableCtrl(CTL_GSITEM_PRICE, 1);
	addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_GSITEM_GGRP, CTLSEL_GSITEM_GOODS));
	long   goods_sel_flags = GoodsCtrlGroup::enableInsertGoods|GoodsCtrlGroup::disableEmptyGoods;
	if(P_Struc) {
		const PPGoodsStrucItem * p_main_item = P_Struc->GetMainItem(0);
		if(p_main_item && p_main_item != P_Data && !(P_Data->Flags & GSIF_MAINITEM))
			DisableClusterItem(CTL_GSITEM_FLAGS, 2, 1);
		if(P_Struc->GetKind() != PPGoodsStruc::kGift) {
			disableCtrl(CTL_GSITEM_GROUPONLY, 1);
			P_Data->Flags &= ~GSIF_GOODSGROUP;
		}
	}
	else
		disableCtrl(CTL_GSITEM_GROUPONLY, 1);
	setCtrlUInt16(CTL_GSITEM_GROUPONLY, BIN(P_Data->Flags & GSIF_GOODSGROUP));
	if(P_Data->Flags & GSIF_GOODSGROUP) {
		goods_sel_flags &= ~GoodsCtrlGroup::disableEmptyGoods;
		goods_sel_flags |= GoodsCtrlGroup::enableSelUpLevel;
	}
	DisableClusterItem(CTL_GSITEM_FLAGS, 3, !P_Struc || !(P_Struc->Rec.Flags & GSF_PARTITIAL));
	GoodsCtrlGroup::Rec rec(0, P_Data->GoodsID, 0, goods_sel_flags);
	setGroupData(GRP_GOODS, &rec);
	setCtrlString(CTL_GSITEM_VALUE, P_Data->GetEstimationString(buf));
	buf = P_Data->Formula;
	setCtrlString(CTL_GSITEM_FORMULA, buf);
	if(P_Data->Flags & GSIF_QTTYASPRICE)
		v = 3;
	else if(P_Data->Flags & GSIF_PCTVAL)
		v = 2;
	else if(P_Data->Flags & GSIF_PHUVAL)
		v = 1;
	else
		v = 0;
	setCtrlData(CTL_GSITEM_UNITS, &v);
	AddClusterAssoc(CTL_GSITEM_FLAGS, 0, GSIF_ROUNDDOWN);
	AddClusterAssoc(CTL_GSITEM_FLAGS, 1, GSIF_AUTOTSWROFF);
	AddClusterAssoc(CTL_GSITEM_FLAGS, 2, GSIF_MAINITEM);
	AddClusterAssoc(CTL_GSITEM_FLAGS, 3, GSIF_SUBPARTSTR);
	AddClusterAssoc(CTL_GSITEM_FLAGS, 4, GSIF_IDENTICAL);  // @v7.4.10
	AddClusterAssoc(CTL_GSITEM_FLAGS, 5, GSIF_QUERYEXPLOT);  // @v9.0.4
	SetClusterData(CTL_GSITEM_FLAGS, P_Data->Flags);
	setupPrice();
	if(P_Data->GoodsID)
		selectCtrl(CTL_GSITEM_VALUE);
}

void GSItemDialog::setupPrice()
{
	Price = 0.0;
	GoodsCtrlGroup::Rec rec;
	getGroupData(GRP_GOODS, &rec);
	if(rec.GoodsID)
		::GetCurGoodsPrice(rec.GoodsID, LConfig.Location, GPRET_MOSTRECENT, &Price);
	setCtrlReal(CTL_GSITEM_PRICE, Price);
}

IMPL_HANDLE_EVENT(GSItemDialog)
{
	TDialog::handleEvent(event);
	if(event.isCbSelected(CTLSEL_GSITEM_GOODS))
		setupPrice();
	else if(event.isClusterClk(CTL_GSITEM_GROUPONLY)) {
		SETFLAG(P_Data->Flags, GSIF_GOODSGROUP, getCtrlUInt16(CTL_GSITEM_GROUPONLY));
		GoodsCtrlGroup * p_grp = (GoodsCtrlGroup *)getGroup(GRP_GOODS);
		if(p_grp)
			p_grp->setFlag(this, GoodsCtrlGroup::disableEmptyGoods, BIN(!(P_Data->Flags & GSIF_GOODSGROUP)));
	}
	else if(event.isCmd(cmGSItemLots)) {
		GoodsCtrlGroup::Rec rec;
		getGroupData(GRP_GOODS, &rec);
		if(rec.GoodsID)
			ViewLots(rec.GoodsID, 0, 0, 0, 0);
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
	// @v8.0.0 {
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
	// } @v8.0.0
	else
		return;
	clearEvent(event);
}

static int SLAPI EditGoodsStrucItem(const PPGoodsStruc * pStruc, PPGoodsStrucItem * pItem)
{
	int    ok = -1, valid_data = 0;
	SString temp_buf, formula;
	ushort v;
	GSItemDialog * dlg = 0;
	GoodsCtrlGroup::Rec rec;
	PPGoodsStrucItem item = *pItem;
	PPObjGoods gobj;
	Goods2Tbl::Rec goods_rec;
	THROW(CheckDialogPtr(&(dlg = new GSItemDialog(&item, pStruc))));
	dlg->DisableClusterItem(CTL_GSITEM_UNITS, 3, (pStruc->Rec.Flags & GSF_PARTITIAL) ? 0 : 1);
	dlg->setCtrlReal(CTL_GSITEM_NETTO, item.Netto);
	dlg->setCtrlData(CTL_GSITEM_SYMB, item.Symb);
	while(!valid_data && ExecView(dlg) == cmOK) {
		ok = -1;
		dlg->getGroupData(GRP_GOODS, &rec);
		dlg->getCtrlData(CTL_GSITEM_UNITS, &(v = 0));
		item.Flags &= ~(GSIF_PCTVAL | GSIF_PHUVAL | GSIF_QTTYASPRICE);
		if(v == 1)
			item.Flags |= GSIF_PHUVAL;
		else if(v == 2)
			item.Flags |= GSIF_PCTVAL;
		else if(v == 3)
			item.Flags |= GSIF_QTTYASPRICE;
		dlg->GetClusterData(CTL_GSITEM_FLAGS, &item.Flags);
		dlg->getCtrlString(CTL_GSITEM_VALUE, temp_buf);
		dlg->getCtrlString(CTL_GSITEM_FORMULA, formula);
		dlg->getCtrlData(CTL_GSITEM_NETTO, &item.Netto);
		dlg->getCtrlData(CTL_GSITEM_SYMB, item.Symb);
		SETFLAG(item.Flags, GSIF_GOODSGROUP, dlg->getCtrlUInt16(CTL_GSITEM_GROUPONLY));
		if(!item.SetEstimationString(temp_buf))
			PPErrorByDialog(dlg, CTL_GSITEM_VALUE);
		else if(!item.SetFormula(formula, pStruc))
			PPErrorByDialog(dlg, CTL_GSITEM_FORMULA);
		else if(item.Flags & GSIF_PHUVAL && goods_rec.PhUPerU == 0)
			PPErrorByDialog(dlg, CTL_GSITEM_UNITS, PPERR_PHUFORGOODSWOPHU);
		else {
			item.GoodsID = (item.Flags & GSIF_GOODSGROUP) ? rec.GrpID : rec.GoodsID;
			if(!item.GoodsID || gobj.Fetch(item.GoodsID, &goods_rec) <= 0)
				PPErrorByDialog(dlg, CTLSEL_GSITEM_GOODS, (item.Flags & GSIF_GOODSGROUP) ? PPERR_GOODSGROUPNEEDED : PPERR_GOODSNEEDED);
			else {
				*pItem = item;
				ok = valid_data = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int GSDialog::checkDupGoods(int pos, PPGoodsStrucItem * pItem)
{
	int    ok = 1;
	for(int i = 0; ok && i < (int)Data.Items.getCount(); i++)
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

int GSDialog::addItem(long * pPos, long * pID)
{
	return addItemExt(pPos, pID);
}

int GSDialog::addItemExt(long * pPos, long * pID)
{
	int    ok = -1;
	ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, NewGoodsGrpID, ExtGoodsSelDialog::fForcePassive);
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
	return ok;
}
//
//
//
int GSDialog::addItemBySample()
{
	class GoodsStrucCopyDialog : public TDialog {
	public:
		GoodsStrucCopyDialog() : TDialog(DLG_GSCOPY)
		{
			addGroup(GRP_GOODS, new GoodsCtrlGroup(CTLSEL_GSCOPY_GGRP, CTLSEL_GSCOPY_GOODS));
		}
		int    setDTS(const GoodsStrucCopyParam * pData)
		{
			Data = *pData;
			int    ok = 1;
			GoodsCtrlGroup::Rec rec(Data.GoodsGrpID, Data.GoodsID);
			setGroupData(GRP_GOODS, &rec);
			SetupPPObjCombo(this, CTLSEL_GSCOPY_GSTRUC, PPOBJ_GOODSSTRUC, Data.GStrucID, 0, (void *)Data.GoodsID);
			return ok;
		}
		int    getDTS(GoodsStrucCopyParam * pData)
		{
			int    ok = 1;
			uint   sel =0;
			GoodsCtrlGroup::Rec rec;
			getGroupData(GRP_GOODS, &rec);
			Data.GoodsID = rec.GoodsID;
			getCtrlData(sel = CTLSEL_GSCOPY_GSTRUC, &Data.GStrucID);
			THROW_PP(Data.GStrucID, PPERR_GSTRUCNEEDED);
			ASSIGN_PTR(pData, Data);
			CATCH
				ok = PPErrorByDialog(this, sel);
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GSCOPY_GOODS)) {
				PPID   prev_goods_id = Data.GoodsID;
				getCtrlData(CTLSEL_GSCOPY_GOODS, &Data.GoodsID);
				if(Data.GoodsID != prev_goods_id) {
					Data.GStrucID = 0;
					SetupPPObjCombo(this, CTLSEL_GSCOPY_GSTRUC, PPOBJ_GOODSSTRUC, Data.GStrucID, 0, (void *)Data.GoodsID);
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
	if(pos >= 0 && pos < (long)Data.Items.getCount() &&
		editItemDialog((int)pos, &Data.Items.at((uint)pos)) > 0) {
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
	setCtrlLong(CTL_GSTRUC_ID, Data.Rec.ID); // @v7.2.4
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
				temp.Init();
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
	for(uint i = 0; Data.Childs.enumItems(&i, (void**)&p_item);) {
		char   sub[128];
		StringSet ss(SLBColumnDelim);
		ss.add(STRNSCPY(sub, p_item->Rec.Name), 0);
		ss.add(periodfmt(&p_item->Rec.Period, sub), 0);
		if(!addStringToList(i, ss.getBuf()))
			return 0;
	}
	return 1;
}

int GSExtDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		if(CONFIRM(PPCFM_DELETE)) {
			Data.Childs.atFree((uint)pos);
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
		PPGoodsStruc * p_child = new PPGoodsStruc;
		*p_child = item;
		if(Data.Childs.insert(p_child)) {
			ASSIGN_PTR(pPos, Data.Childs.getCount()-1);
			ASSIGN_PTR(pID, Data.Childs.getCount());
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
	if(pos >= 0 && pos < (long)Data.Childs.getCount()) {
		PPGoodsStruc * p_child = Data.Childs.at(pos);
		SETIFZ(p_child->GoodsID, Data.GoodsID);
		if(PPObjGoodsStruc::EditDialog(p_child) > 0)
			return 1;
	}
	return -1;
}
//
// PPObjGoodsStruc
//
static int SLAPI GSListFilt(void * pRec, void * extraPtr)
{
	PPGoodsStrucHeader * p_rec = (PPGoodsStrucHeader*)pRec;
	return BIN(p_rec->Flags & GSF_NAMED);
}

SLAPI PPObjGoodsStruc::PPObjGoodsStruc(void * extraPtr) : PPObjReference(PPOBJ_GOODSSTRUC, extraPtr)
{
	filt = GSListFilt;
	ImplementFlags |= implStrAssocMakeList;
}

StrAssocArray * SLAPI PPObjGoodsStruc::MakeStrAssocList(void * extraPtr /*goodsID*/)
{
	const   PPID goods_id = (PPID)extraPtr;
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
						if((temp_buf = gsh_rec.Name).Strip().Empty())
							(temp_buf = "BOM").Space().CatChar('#').CatLongZ((long)(i+1), 2);
						THROW_SL(p_list->Add(gsh_rec.ID, gsh_rec.Name));
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
		for(SEnum en = ref->Enum(Obj, 0); en.Next(&gs_rec) > 0;)
			if(gs_rec.Flags & GSF_NAMED) {
				THROW_SL(p_list->Add(gs_rec.ID, gs_rec.Name));
			}
	}
	CATCH
		ZDELETE(p_list);
	ENDCATCH
	return p_list;
}

int SLAPI PPObjGoodsStruc::GetChildIDList(PPID strucID, PPIDArray * pList)
{
	int    ok = -1;
	BExtQuery q(ref, 0, 128);
	q.select(ref->ObjID, 0L).where(ref->ObjType == Obj && ref->Val1 == strucID);
	ReferenceTbl::Key0 k0;
	k0.ObjType = Obj;
	k0.ObjID = 0;
	for(q.initIteration(0, &k0, spGe); q.nextIteration() > 0;) {
		CALLPTRMEMB(pList, add(ref->data.ObjID));
		ok = 1;
	}
	return ok;
}

static IMPL_CMPFUNC(_GSItem, i1, i2)
	{ return cmp_long(((_GSItem*)i1)->Num, ((_GSItem*)i2)->Num); }

int SLAPI PPObjGoodsStruc::Helper_LoadItems(PPID id, PPGoodsStruc * pData)
{
	int    ok = 1;
	SArray items_list(sizeof(_GSItem));
	_GSItem * p_raw_item;
	SString temp_buf, formula;
	THROW(ref->GetPropVlrString(Obj, id, GSPRP_EXTITEMSTR, temp_buf));
	THROW(ref->Assc.GetItemsListByPrmr(PPASS_GOODSSTRUC, id, &items_list));
	items_list.sort(PTR_CMPFUNC(_GSItem));
	for(uint i = 0; items_list.enumItems(&i, (void **)&p_raw_item);) {
		PPGoodsStrucItem item;
		item.GoodsID = p_raw_item->ItemGoodsID;
		item.Flags   = p_raw_item->Flags;
		item.Median  = p_raw_item->Median;
		item.Width   = p_raw_item->Width;
		item.Denom   = p_raw_item->Denom;
		item.Netto   = p_raw_item->Netto;
		STRNSCPY(item.Symb, p_raw_item->Symb);
		PPGetExtStrData(i, temp_buf, formula);
		formula.CopyTo(item.Formula, sizeof(item.Formula));
		THROW_SL(pData->Items.insert(&item));
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsStruc::Get(PPID id, PPGoodsStruc * pData)
{
	int    ok = 1, r = 0;
	THROW(r = ref->GetItem(Obj, id, &pData->Rec));
	if(r > 0) {
		pData->Items.freeAll();
		pData->Childs.freeAll();
		if(pData->Rec.Flags & GSF_FOLDER) {
			PPIDArray child_idlist;
			THROW(GetChildIDList(id, &child_idlist));
			for(uint i = 0; i < child_idlist.getCount(); i++) {
				PPGoodsStruc * p_child = new PPGoodsStruc;
				THROW_MEM(p_child);
				THROW(r = Get(child_idlist.at(i), p_child)); // @recursion
				if(r > 0)
					THROW_SL(pData->Childs.insert(p_child));
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

int SLAPI PPObjGoodsStruc::Put(PPID * pID, PPGoodsStruc * pData, int use_ta)
{
	int    ok = 1;
	int    r;
	int    unchg = 0, items_unchg = 0;
	int    action = 0;
	int    cleared = 0;
	PPGoodsStrucHeader gs;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(!pData || (pData->IsEmpty() && !pData->IsNamed())) {
			if(*pID) {
				THROW(ref->RemoveItem(Obj, *pID, 0));
				DS.LogAction(PPACN_OBJRMV, Obj, *pID, 0, 0);
				cleared = 1;
			}
		}
		else {
			gs.Flags &= ~GSF_DYNGEN;
			if(*strip(pData->Rec.Name) != 0 && !(pData->Rec.Flags & GSF_CHILD))
				gs.Flags |= GSF_NAMED;
			if(*pID) {
				THROW(r = ref->UpdateItem(Obj, *pID, &pData->Rec, 1, 0));
				if(r < 0)
					unchg = 1;
			}
			else {
				THROW(ref->AddItem(Obj, pID, &pData->Rec, 0));
				pData->Rec.ID = *pID;
			}
			if(pData->Rec.Flags & GSF_FOLDER) {
				for(uint i = 0; i < pData->Childs.getCount(); i++) {
					PPGoodsStruc * p_child = pData->Childs.at(i);
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
				PPGoodsStruc temp_struc;
				LongArray pos_list;
				THROW(Helper_LoadItems(*pID, &temp_struc));
				if(temp_struc.Items.getCount() == pData->Items.getCount()) {
					items_unchg = 1;
					for(uint i = 0; items_unchg && i < temp_struc.Items.getCount(); i++)
						if(!(pData->Items.at(i) == temp_struc.Items.at(i)))
							items_unchg = 0;
				}
			}
			if(!items_unchg)
				THROW(ref->Assc.Remove(PPASS_GOODSSTRUC, *pID, 0, 0));
		}
		if(pData && !items_unchg) {
			_GSItem gsi;
			PPGoodsStrucItem * pi;
			SString ext_buf;
			for(uint i = 0; pData->Items.enumItems(&i, (void**)&pi);) {
				PPID   assc_id = 0;
				MEMSZERO(gsi);
				gsi.Tag  = PPASS_GOODSSTRUC;
				gsi.GSID = *pID;
				gsi.ItemGoodsID = pi->GoodsID;
				gsi.Flags  = pi->Flags;
				gsi.Median = pi->Median;
				gsi.Width  = pi->Width;
				gsi.Denom  = pi->Denom;
				gsi.Netto  = pi->Netto;
				STRNSCPY(gsi.Symb, pi->Symb);
				gsi.Num    = i;
				THROW(ref->Assc.SearchFreeNum(gsi.Tag, *pID, &gsi.Num));
				THROW(ref->Assc.Add(&assc_id, (ObjAssocTbl::Rec *)&gsi, 0));
				THROW(PPPutExtStrData(i, ext_buf, strip(pi->Formula)));
			}
			THROW(ref->PutPropVlrString(Obj, *pID, GSPRP_EXTITEMSTR, ext_buf));
		}
		if(cleared)
			*pID = 0;
		if(unchg) {
			if(items_unchg)
				ok = -1;
			else
				DS.LogAction(PPACN_OBJUPD, Obj, *pID, 0, 0);
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

// static
int SLAPI PPObjGoodsStruc::EditExtDialog(PPGoodsStruc * pData)
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

// static
int SLAPI PPObjGoodsStruc::EditDialog(PPGoodsStruc * pData, int toCascade)
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
						delete dlg;
						dlg = 0;
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

int SLAPI PPObjGoodsStruc::Edit(PPID * pID, void * extraPtr /*goodsID*/)
{
	int    ok = cmCancel;
	PPGoodsStruc data;
	if(*pID) {
		THROW(Get(*pID, &data));
	}
	data.GoodsID = (PPID)extraPtr;
	if((ok = EditDialog(&data)) > 0) {
		THROW(Put(pID, &data, 1));
		ok = cmOK;
	}
	CATCHZOKPPERR
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsStruc, PPGoodsStruc);

int SLAPI PPObjGoodsStruc::SerializePacket(int dir, PPGoodsStruc * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	int32  c = (int32)pPack->Items.getCount(); // @persistent
	THROW_SL(ref->SerializeRecord(dir, &pPack->Rec, rBuf, pSCtx));
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
		THROW_SL(pSCtx->Serialize(dir, MKSTYPE(S_ZSTRING, sizeof(item.Formula)), item.Formula, 0, rBuf));
		if(dir < 0) {
			THROW_SL(pPack->Items.insert(&item));
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsStruc::Read(PPObjPack * pPack, PPID id, void * stream, ObjTransmContext * pCtx)
{
	int    ok = 1;
	PPGoodsStruc * p_gs = new PPGoodsStruc;
	THROW_MEM(p_gs);
	if(stream == 0) {
		THROW(Get(id, p_gs));
	}
	else {
		SBuffer buffer;
		THROW_SL(buffer.ReadFromFile((FILE*)stream, 0))
		THROW(SerializePacket(-1, p_gs, buffer, &pCtx->SCtx));
	}
	CATCH
		ok = 0;
		ZDELETE(p_gs);
	ENDCATCH
	pPack->Data = p_gs;
	return ok;
}

int SLAPI PPObjGoodsStruc::Write(PPObjPack * pPack, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPGoodsStruc * p_gs = (PPGoodsStruc*)pPack->Data;
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
				if(goods_obj.GetConfig().Flags & GCF_XCHG_RCVSTRUCUPD) {
					if(!Put(pID, p_gs, 1)) {
						pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTGOODSSTRUC, p_gs->Rec.ID, p_gs->Rec.Name);
						ok = -1;
					}
				}
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_gs, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile((FILE*)stream, 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsStruc::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(p && p->Data) {
		uint   i;
		PPGoodsStruc * p_gs = (PPGoodsStruc*)p->Data;
		PPGoodsStrucItem * p_gsi;
		for(i = 0; p_gs->Items.enumItems(&i, (void**)&p_gsi);)
			THROW(ProcessObjRefInArray(PPOBJ_GOODS, &p_gsi->GoodsID, ary, replace));
		for(i = 0; i < p_gs->Childs.getCount(); i++)
			THROW(ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &p_gs->Childs.at(i)->Rec.ID, ary, replace));
		THROW(ProcessObjRefInArray(PPOBJ_GOODSSTRUC, &p_gs->Rec.ParentID, ary, replace));
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsStruc::HandleMsg(int msg, PPID _obj, PPID _id, void * extraPtr)
{
	int    ok = DBRPL_OK;
	if(msg == DBMSG_OBJDELETE)
		if(_obj == PPOBJ_GOODS) {
			ObjAssocTbl::Rec assc_rec;
			for(SEnum en = ref->Assc.Enum(PPASS_GOODSSTRUC, _id, 1); en.Next(&assc_rec) > 0;) {
				ok = RetRefsExistsErr(Obj, assc_rec.PrmrObjID);
				break;
			}
		}
	if(msg == DBMSG_OBJREPLACE)
		if(_obj == PPOBJ_GOODS) {
			ObjAssocTbl * t = &ref->Assc;
			if(!updateFor(t, 0, (t->AsscType == PPASS_GOODSSTRUC && t->ScndObjID == _id),
				set(t->ScndObjID, dbconst((long)extraPtr)))) {
				ok = PPSetErrorDB();
			}
		}
	return ok;
}

int SLAPI PPObjGoodsStruc::Print(PPGoodsStruc * pGoodsStruc)
{
	int    ok = -1;
	int    is_hier = 0; // !0 - иерархическая структура
	uint   what = 0x00;
	PPObjGoods goods_obj;
	{
		PPIDArray goods_ids, struct_ids;
		PPLogger log;
		goods_obj.P_Tbl->SearchGListByStruc(pGoodsStruc->Rec.ID, &goods_ids);
		if(CheckStruct(&goods_ids, &struct_ids, pGoodsStruc, &log) > 0)
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
		PView  pv(&gs_iter);
		PPAlddPrint(rpt_id, &pv);
		ok = 1;
	}
	return ok;
}

SLAPI GStrucIterator::GStrucIterator()
{
	Idx = 0;
	LoadRecurItems = 0;
}

const PPGoodsStruc * SLAPI GStrucIterator::GetStruc() const
{
	return &GStruc;
}

int SLAPI GStrucIterator::LoadItems(PPGoodsStruc * pStruc, PPID parentGoodsID, double srcQtty, int level)
{
	int    ok = 1;
	if(pStruc) {
		long   fmt = MKSFMTD(0, 5, NMBF_NOTRAILZ);
		double dest_qtty = 0.0;
		SString s_qtty;
		PPObjGoods g_obj;
		GStrucRecurItem gsr_item;
		MEMSZERO(gsr_item);
		gsr_item.Level = level;
		for(uint i = 0; pStruc->EnumItemsExt(&i, &gsr_item.Item, parentGoodsID, srcQtty, &dest_qtty) > 0;) {
			int    do_load_recur = 0;
			double sum = 0.0;
			PPGoodsStruc inner_struc;
			ReceiptTbl::Rec lot_rec;
			(s_qtty = 0).Cat(dest_qtty, fmt);
			gsr_item.Qtty = dest_qtty;
			gsr_item.Item.SetEstimationString(s_qtty);
			if(pStruc->GetEstimationPrice(i-1, &gsr_item.Price, &gsr_item.Sum, &lot_rec) > 0)
				gsr_item.LastLotID = lot_rec.ID;
			gsr_item.HasInner = 0;
			{
				int    r = 0;
				PPGoodsStruc::Ident ident(gsr_item.Item.GoodsID);
				PPObjGoodsStruc gs_obj;
				THROW(r = g_obj.LoadGoodsStruc(&ident, &inner_struc));
				if(r > 0 && gs_obj.CheckStruc(inner_struc.Rec.ID, 0) != 2) {
					int    uncert = 0;
					inner_struc.CalcEstimationPrice(&gsr_item.Price, &uncert, 1);
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

int SLAPI GStrucIterator::Init(PPGoodsStruc * pStruc, int loadRecurItems)
{
	RVALUEPTR(GStruc, pStruc);
	LoadRecurItems = loadRecurItems;
	Items.freeAll();
	LoadItems(&GStruc, GStruc.GoodsID, 1, 0);
	return 1;
}

int SLAPI GStrucIterator::InitIteration()
{
	Idx = 0;
	return 1;
}

int FASTCALL GStrucIterator::NextIteration(GStrucRecurItem * pItem)
{
	int    ok = -1;
	GStrucRecurItem * p_item = 0;
	if(Items.enumItems(&Idx, (void**)&p_item) > 0) {
		ASSIGN_PTR(pItem, *p_item);
		ok = 1;
	}
	return ok;
}

int SLAPI PPObjGoodsStruc::CheckStruct(PPIDArray * pGoodsIDs, PPIDArray * pStructIDs, PPGoodsStruc * pStruct, PPLogger * pLog)
{
	int    ok = 1;
	int    recur = 0;
	if(pStruct && pStruct->Rec.Flags & GSF_COMPL && pGoodsIDs && pStructIDs) {
		PPObjGoods goods_obj;
		SString msg, buf, g_name, struc_name, cg_name, cstruc_name;
		PPGoodsStrucItem * p_item = 0;
		THROW_SL(pStructIDs->add(pStruct->Rec.ID));
		for(uint i = 0; pStruct->Items.enumItems(&i, (void**)&p_item) > 0;) {
			int    s = 0, g = 0;
			double price = 0.0, sum = 0.0;
			StringSet ss(SLBColumnDelim);
			PPGoodsStruc::Ident ident(p_item->GoodsID);
			PPGoodsStruc gstruc;
			THROW(goods_obj.LoadGoodsStruc(&ident, &gstruc));
			if((g = pGoodsIDs->lsearch(p_item->GoodsID)) > 0 || (s = pStructIDs->lsearch(gstruc.Rec.ID)) > 0) {
				recur = 1;
				PPID   goods_id;
				PPLoadText(PPTXT_CYCLESSTRUCT, msg);
				goods_id = (pGoodsIDs->getCount() > 0) ? pGoodsIDs->at(pGoodsIDs->getCount() - 1) : 0;
				GetGoodsName(goods_id, g_name);
				if(strip(pStruct->Rec.Name)[0] != '\0')
					struc_name = pStruct->Rec.Name;
				else
					ideqvalstr(pStruct->Rec.ID, struc_name = 0);
				GetGoodsName(p_item->GoodsID, cg_name);
				if(strip(gstruc.Rec.Name)[0] != '\0')
					cstruc_name = gstruc.Rec.Name;
				else
					ideqvalstr(gstruc.Rec.ID, cstruc_name = 0);
				buf.Printf(msg.cptr(), g_name.cptr(), struc_name.cptr(), cg_name.cptr(), cstruc_name.cptr());
				if(pLog)
					pLog->Log(buf);
			}
			else {
				int    r = 0;
				THROW_SL(pGoodsIDs->add(p_item->GoodsID));
				THROW(r = CheckStruct(pGoodsIDs, pStructIDs, &gstruc, pLog));
				THROW_SL(pGoodsIDs->removeByID(p_item->GoodsID));
				if(r == 2)
					recur = 1;
				if(ok < 0 && r > 0)
					ok = r;
				//ok = (ok <= 0) ? ok : r;
			}
		}
		THROW_SL(pStructIDs->removeByID(pStruct->Rec.ID));
	}
	else
		ok = -1;
	CATCHZOK
	if(recur)
		ok = 2;
	return ok;
}

int SLAPI PPObjGoodsStruc::CheckStruc(PPID strucID, PPLogger * pLogger)
{
	int    ok = -1;
	PPGoodsStruc gs;
	PPObjGoods goods_obj;
	PPIDArray struct_ids, goods_ids;
	goods_obj.P_Tbl->SearchGListByStruc(strucID, &goods_ids);
	THROW(Get(strucID, &gs));
	THROW(ok = CheckStruct(&goods_ids, &struct_ids, &gs, pLogger));
	CATCHZOK
	return ok;
}

int SLAPI PPObjGoodsStruc::CheckStructs()
{
	int    ok = 1;
	long   p = 0, t = 0;
	PPID   id = 0;
	PPLogger logger;
	PPObjGoods goods_obj;
	PPWait(1);
	for(id = 0; EnumItems(&id) > 0; t++)
		;
	for(id = 0, p = 0; EnumItems(&id) > 0; p++) {
		THROW(CheckStruc(id, &logger));
		PPWaitPercent(p, t);
	}
	CATCHZOKPPERR
	PPWait(0);
	return ok;
}

// static
int SLAPI PPObjGoodsStruc::CheckRecursion()
{
	PPObjGoodsStruc gs_obj;
	return gs_obj.CheckStructs();
}
//
//
//
SLAPI SaGiftItem::SaGiftItem()
{
	StrucID = 0;
	OrgStrucID = 0;
	QuotKindID = 0;
	Flags = 0;
	Limit = 0.0f; // @v7.0.0
	AmtRestrict = 0.0;
}

SLAPI SaGiftItem::SaGiftItem(const SaGiftItem & rS)
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
		Limit = pS->Limit; // @v7.0.0
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
		Limit = 0.0f; // @v7.0.0
		AmtRestrict = 0.0;
		GiftList.freeAll();
		List.freeAll();
	}
	CATCHZOK
	return ok;
}

int SLAPI SaGiftItem::IsSaleListSuitable(const TSArray <SaSaleItem> & rSaleList, RAssocArray * pCheckList, LongArray * pMainPosList, double * pQtty) const
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
					// @v7.7.11 goods_list.addUnique(rSaleList.at(max_iq_idx).GoodsID);
					goods_list.Add(rSaleList.at(max_iq_idx).GoodsID, p_entry->MinQtty); // @v7.7.11
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
					// @v7.7.11 if(!check_list.lsearch(sale_goods_id) && rSaleList.lsearch(&sale_goods_id, &p, CMPF_LONG)) {
					if(!check_list.Search(sale_goods_id, 0, 0) && rSaleList.lsearch(&sale_goods_id, &p, CMPF_LONG)) { // @v7.7.11
						const SaSaleItem & r_sa_item = rSaleList.at(p);
						total_iq += r_sa_item.Qtty;
						total_amt += (r_sa_item.Qtty * r_sa_item.Price);
						// @v7.7.11 goods_list.addUnique(sale_goods_id);
						goods_list.Add(sale_goods_id, p_entry->MinQtty); // @v7.7.11
					}
				}
				entry_mult = total_iq / p_entry->MinQtty;
			}
		}
		const uint glc = goods_list.getCount();
		if(glc && entry_mult > 0.0) {
			for(uint j = 0; j < glc; j++) {
				// @v7.7.11 {
				const RAssoc & r_goods_entry = goods_list.at(j);
				if(!check_list.Search(r_goods_entry.Key, 0, 0)) {
					check_list.Add(r_goods_entry.Key, r_goods_entry.Val);
					if(p_entry->GsiFlags & GSIF_MAINITEM)
						main_pos_list.addUnique(check_list.getCount()-1);
				}
				// } @v7.7.11
				/* @v7.7.11
				if(check_list.addUnique(goods_list.get(j)) > 0) {
					if(p_entry->GsiFlags & GSIF_MAINITEM)
						main_pos_list.addUnique(check_list.getCount()-1);
				} */
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

int SLAPI SaGiftItem::CalcPotential(const TSArray <SaSaleItem> & rSaleList,
	PPID * pPotGoodsID, double * pPotAmount, double * pPotDeficit, SString & rPotName) const
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
				for(uint j = 0; j < GiftList.getCount() && rPotName.Empty(); ++j)
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
	Init();
}

SaGiftArray::Gift & SaGiftArray::Gift::Init()
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
	else if(!CheckList.IsEqual(rS.CheckList))
		eq = 0;
	return eq;
}

int FASTCALL SaGiftArray::Gift::PreservePotential(SaGiftArray::Potential & rS) const
{
	rS = Pot;
	return 1;
}

int FASTCALL SaGiftArray::Gift::RestorePotential(const SaGiftArray::Potential & rS)
{
	Pot = rS;
	return 1;
}

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

int SaGiftArray::SelectGift(const TSArray <SaSaleItem> & rSaleList, const RAssocArray & rExGiftList, int overlap, SaGiftArray::Gift & rGift) const
{
	rGift.Init();

	int    ok = -1;
	const  PPCommConfig & r_ccfg = CConfig;
	uint   i;
	PPID   goods_id = 0;
	SString msg_buf, fmt_buf;
	double max_amt_restr = 0.0; // Суммовое ограничение последнего найденного подарка
	double deficit = SMathConst::Max; // Минимальный дефицит (остаток, на который необходимо добрать товара чтобы получить подарок).
	PPIDArray struc_list;
	PPIDArray temp_list;
	RAssocArray check_list;
	for(i = 0; i < rSaleList.getCount(); i++) {
		temp_list.clear();
		const PPID sale_goods_id = rSaleList.at(i).GoodsID;
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
				double limit = (p_item->Limit > 0.0f) ? (p_item->Limit - rExGiftList.Get(p_item->StrucID)) : SMathConst::Max; // @v7.0.0
				if(limit > 0.0) {
					LongArray main_pos_list; // @v7.0.6
					if(p_item->IsSaleListSuitable(rSaleList, &check_list, &main_pos_list, &qtty)) {
						int    use = 0;
						SETMIN(qtty, limit); // @v7.0.0
						if(max_amt_restr > 0.0 && p_item->AmtRestrict > 0.0) {
							if(p_item->AmtRestrict > max_amt_restr)
								use = 1;
						}
						else if(qtty > rGift.Qtty)
							use = 1;
						if(use) {
							rGift.ID = struc_id; // @v7.0.0
							rGift.Qtty = qtty;
							rGift.CheckList = check_list;
							rGift.MainPosList = main_pos_list; // @v7.0.6
							rGift.QuotKindID = p_item->QuotKindID;
							rGift.List = at(p)->GiftList;
							max_amt_restr = p_item->AmtRestrict;
							ok = 1;
						}
					}
					else if(rGift.Pot.Name.Empty()) {
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

int SLAPI PPObjGoodsStruc::LoadGiftList(SaGiftArray * pList)
{
	int    ok = -1;
	uint   i;
	PPIDArray owner_goods_list, temp_goods_list;
	PPIDArray antirecur_trace; // След извлечения родительских структур, хранимый во избежании зацикливания //
	SaGiftItem * p_item = 0;
	SaGiftItem::Entry * p_entry = 0;
	PPGoodsStrucHeader rec, org_rec;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	pList->freeAll();
	for(SEnum en = ref->Enum(Obj, 0); en.Next(&rec) > 0;) {
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
			p_item->Limit      = rec.GiftLimit;        // @v7.0.0
			p_item->AmtRestrict = rec.GiftAmtRestrict;
			SETFLAG(p_item->Flags, SaGiftItem::fCalcPotential, rec.Flags & GSF_GIFTPOTENTIAL);
			SETFLAG(p_item->Flags, SaGiftItem::fOverlap, rec.Flags & GSF_OVRLAPGIFT); // @v7.3.0

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
						p_entry->GsiFlags = gs_item.Flags; // @v7.0.6
						// @v7.3.0 {
						if(gs_item.Flags & GSIF_GOODSGROUP) {
							GoodsIterator::GetListByGroup(gs_item.GoodsID, &p_entry->GoodsList);
						}
						// } @v7.3.0
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
			temp_goods_list.clear();
			owner_goods_list.clear();
			THROW(goods_obj.P_Tbl->SearchGListByStruc(p_item->OrgStrucID, &temp_goods_list));
			for(uint j = 0; j < temp_goods_list.getCount(); j++) {
				owner_goods_list.addUnique(temp_goods_list.get(j));
			}
			if(owner_goods_list.getCount()) {
				p_item->GiftList = owner_goods_list;
			}
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
//
//
//
class GoodsStrucCache : public ObjCache {
public:
	SLAPI  GoodsStrucCache() : ObjCache(PPOBJ_GOODSSTRUC, sizeof(D))
	{
		P_GiftList = 0;
	}
	SLAPI ~GoodsStrucCache()
	{
		delete P_GiftList;
	}
	int    SLAPI GetSaGiftList(SaGiftArray * pList, int clear);
private:
	virtual int  SLAPI Dirty(PPID id);
	virtual int  SLAPI FetchEntry(PPID, ObjCacheEntry * pEntry, long);
	virtual void SLAPI EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const;
public:
	struct D : public ObjCacheEntry {
		PPID   VariedPropObjType;
		DateRange Period;
		double CommDenom;
		float  GiftLimit; // @v7.0.0
		long   Flags;
		PPID   ParentID;
	};
	SaGiftArray * P_GiftList;
};

int SLAPI GoodsStrucCache::GetSaGiftList(SaGiftArray * pList, int clear)
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

int SLAPI GoodsStrucCache::FetchEntry(PPID id, ObjCacheEntry * pEntry, long)
{
	int    ok = 1;
	D * p_cache_rec = (D *)pEntry;
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

void SLAPI GoodsStrucCache::EntryToData(const ObjCacheEntry * pEntry, void * pDataRec) const
{
	PPGoodsStrucHeader * p_data_rec = (PPGoodsStrucHeader *)pDataRec;
	const D * p_cache_rec = (const D *)pEntry;
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

int SLAPI GoodsStrucCache::Dirty(PPID id)
{
	int    ok = 1;
	RwL.WriteLock();
	ok = Helper_Dirty(id);
	RwL.Unlock();
	if(P_GiftList) {
		PPGoodsStrucHeader temp_rec;
		int    r = Get(id, &temp_rec);
		if((r > 0 && temp_rec.Flags & GSF_PRESENT) || r < 0) {
			GetSaGiftList(0, 1); // Функция защищена внутренней критической секцией
		}
	}
	return ok;
}

IMPL_OBJ_FETCH(PPObjGoodsStruc, PPGoodsStrucHeader, GoodsStrucCache);

int SLAPI PPObjGoodsStruc::FetchGiftList(SaGiftArray * pList)
{
	GoodsStrucCache * p_cache = GetDbLocalCachePtr <GoodsStrucCache> (Obj);
	return p_cache ? p_cache->GetSaGiftList(pList, 0) : LoadGiftList(pList);
}
