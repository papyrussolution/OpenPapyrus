// OBJBSKT.CPP
// Copyright (c) A.Sobolev 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

IMPL_CMPFUNC(ILTI, i1, i2) { return cmp_long(static_cast<const ILTI *>(i1)->BillID, static_cast<const ILTI *>(i2)->BillID); }

IMPL_CMPFUNC(ILTIGGRP, i1, i2)
{
	const ILTI * item1 = static_cast<const ILTI *>(i1);
	const ILTI * item2 = static_cast<const ILTI *>(i2);
	int    cmp = 0;
	PPObjGoods goods_obj;
	Goods2Tbl::Rec rec, grp_rec;
	SString name1, name2;
	SString grp_name1, grp_name2;
	if(goods_obj.Fetch(item1->GoodsID, &rec) > 0) {
		name1 = rec.Name;
		goods_obj.Fetch(rec.ParentID, &grp_rec);
		grp_name1 = grp_rec.Name;
	}
	if(goods_obj.Fetch(item2->GoodsID, &rec) > 0) {
		name2 = rec.Name;
		goods_obj.Fetch(rec.ParentID, &grp_rec);
		grp_name2 = grp_rec.Name;
	}
	if((cmp = stricmp866(grp_name1, grp_name2)) > 0)
		return 1;
	else if(cmp < 0)
		return -1;
	if((cmp = stricmp866(name1, name2)) > 0)
		return 1;
	else if(cmp < 0)
		return -1;
	return 0;
}

IMPL_CMPFUNC(ILTIGOODS, i1, i2)
{
	PPObjGoods goods_obj;
	SString name1, name2;
	Goods2Tbl::Rec rec1, rec2;
	goods_obj.Fetch(static_cast<const ILTI *>(i1)->GoodsID, &rec1);
	goods_obj.Fetch(static_cast<const ILTI *>(i2)->GoodsID, &rec2);
	int    cmp = stricmp866(rec1.Name, rec2.Name);
	if(cmp > 0)
		return 1;
	else if(cmp < 0)
		return -1;
	return 0;
}
//
// Descr: внутреннее представление для элемента товарной корзины. Проецируется на
//   запись таблицы ObjAssocTbl
//
struct PPGoodsBasketItem { // @#{sizeof(PPGoodsBasketItem) == sizeof(ObjAssocTbl::Rec)}
	PPID   Id;             // Внутренний идентификатор строки (уникальный по всей таблице ObjAssoc)
	PPID   Tag;            // Const=PPASS_GOODSBASKET
	PPID   BasketID;       // PrimaryID =? ID структуры PPGoodsBasket
	PPID   ItemGoodsID;    // ->Goods.ID ID элемента структуры
	long   Num;            // Внутренний номер (не используется, но инициализируется)
	PPID   UserID;         // Идентификатор пользователя, создавшего строку
	char   Reserve1[20];   // @reserve
	long   Flags;          // Флаги
	double Quantity;	   // Количество [0..]
	double Price;	       // Цена за единицу (смысл интерпретируется пользователем корзины).
	double Reserve2;       // @reserve
	double UnitPerPack;    // Емкость упаковки товара
	LDATE  Expiry;         // Срок годности товара
};

#ifdef _DEBUG // {

static class LocalAssertion_PPObjGoodsBasket {
public:
	LocalAssertion_PPObjGoodsBasket()
	{
		STATIC_ASSERT(sizeof(PPGoodsBasketItem) == sizeof(ObjAssocTbl::Rec));
	}
} Inst_LocalAssertion_PPObjGoodsBasket;

#endif // } _DEBUG
//
// PPBasketPacket
//
PPBasketPacket::PPBasketPacket() : ILBillPacket()
{
	Init();
}

PPBasketPacket::~PPBasketPacket()
{
	destroy();
}

void PPBasketPacket::Init()
{
	Head.ID  = Head.Num = Head.Flags = 0;
	Head.Tag = PPOBJ_GOODSBASKET;
	Head.User = LConfig.UserID;
	PTR32(Head.Name)[0] = 0;
	Head.SupplID = 0;
	GoodsID = 0;
	Lots.freeAll();
	InsertedGoodsList.freeAll();
}

void PPBasketPacket::InitInsertion()
{
	InsertedGoodsList.freeAll();
}

PPBasketPacket & FASTCALL PPBasketPacket::operator = (const PPBasketPacket & src)
{
	if(&src != this) {
		Head.Tag     = src.Head.Tag;
		Head.ID      = src.Head.ID;
		Head.User    = src.Head.User;
		Head.Flags   = src.Head.Flags;
		Head.SupplID = src.Head.SupplID;
		STRNSCPY(Head.Name, src.Head.Name);
		GoodsID      = src.GoodsID;
		memzero(Head.Reserve1, sizeof(Head.Reserve1));
		Lots.copy(src.Lots);
		InsertedGoodsList.copy(src.InsertedGoodsList);
	}
	return *this;
}

int PPBasketPacket::AddItem(const ILTI * pItem, uint * pPos, int replaceOption)
{
	int    ok = 1;
	uint   pos = 0;
	ILTI   temp_item = *pItem;
	if(SearchGoodsID(labs(temp_item.GoodsID), &pos) > 0) {
		if(replaceOption == 3 || InsertedGoodsList.lsearch(labs(temp_item.GoodsID))) {
			temp_item.Quantity += Lots.at(pos).Quantity;
			DelItem(pos);
		}
		else if(replaceOption == 2) {
			DelItem(pos);
		}
		else {
			SString msg_buf;
			GetGoodsName(temp_item.GoodsID, msg_buf).Space().CatChar('[').Cat(Lots.at(pos).Quantity, MKSFMTD(0, 6, NMBF_NOTRAILZ)).
				Space().Cat("itm").CatChar(']');
			CALLEXCEPT_PP_S(PPERR_DUPBASKETITEM, msg_buf);
		}
	}
	if(Head.Flags & GBASKF_SORTITEMS) {
		THROW_SL(Lots.ordInsert(&temp_item, pPos, PTR_CMPFUNC(ILTIGOODS)));
	}
	else {
		THROW_SL(Lots.insert(&temp_item));
		ASSIGN_PTR(pPos, Lots.getCount()-1);
	}
	THROW(InsertedGoodsList.add(labs(temp_item.GoodsID)));
	CATCHZOK
	return ok;
}

int PPBasketPacket::DelItem(long pos)
{
	return Lots.atFree(static_cast<uint>(pos)) ? 1 : -1;
}

int PPBasketPacket::SearchGoodsID(PPID goodsID, uint * pPos) const
{
	const PPID goods_id = labs(goodsID);
	return Lots.lsearch(&goods_id, pPos, CMPF_LONG, offsetof(ILTI, GoodsID));
}

PPBasketCombine::PPBasketCombine() : BasketID(0)
{
}
//
// PPObjGoodsBasket
//
PPObjGoodsBasket::Locking::Locking(PPID id) : ID(0), L(0)
{
	if(id)
		Lock(id);
}

PPObjGoodsBasket::Locking::~Locking()
{
	Unlock();
}

int PPObjGoodsBasket::Locking::Lock(PPID id)
{
	int    ok = -1;
	PPID   mutex_id = 0;
	if(id) {
		if(L && id == ID)
			ok = 1;
		else {
			if(L && id != ID)
				Unlock();
			if(DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_GOODSBASKET, id, &mutex_id, 0) > 0) {
				ID = id;
				L = 1;
				ok = 1;
			}
			else {
				SetAddLockErrInfo(mutex_id);
				ok = PPSetError(PPERR_BASKETLOCKED);
			}
		}
	}
	return ok;
}

int PPObjGoodsBasket::Locking::Unlock()
{
	int    ok = -1;
	if(L && ID) {
		ok = DS.GetSync().ReleaseMutex(PPOBJ_GOODSBASKET, ID);
		L = 0;
		ID = 0;
		ok = 1;
	}
	return ok;
}

PPObjGoodsBasket::PPObjGoodsBasket(void * extraPtr) : PPObjReference(PPOBJ_GOODSBASKET, extraPtr)
{
}

/*static*/void PPObjGoodsBasket::SetAddLockErrInfo(PPID mutexID)
{
	SString buf;
	DS.GetSync().GetLockingText(mutexID, 1, buf);
	PPSetAddedMsgString(buf);
}

/*static*/int PPObjGoodsBasket::ForceUnlock(PPID id)
{
	int    ok = 1;
	PPSyncArray sync_ary;
	THROW(DS.GetSync().GetItemsList(PPSYNC_MUTEX, &sync_ary));
	for(uint i = 0; i < sync_ary.getCount(); i++) {
		PPSyncItem s_i = sync_ary.at(i);
		if(s_i.ObjType == PPOBJ_GOODSBASKET && id == s_i.ObjID) {
			THROW(DS.GetSync().ClearMutex(s_i.ID));
			break;
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObjGoodsBasket::IsLocked(PPID id)
{
	int    ok = 0;
	PPID   mutex_id = 0;
	if(DS.GetSync().CreateMutex_(LConfig.SessionID, PPOBJ_GOODSBASKET, id, &mutex_id, 0) > 0) {
		DS.GetSync().ReleaseMutex(PPOBJ_GOODSBASKET, id);
		ok = 0;
	}
	else {
		SetAddLockErrInfo(mutex_id);
		ok = (PPSetError(PPERR_BASKETLOCKED), 1);
	}
	return ok;
}

int PPObjGoodsBasket::GetPacket(PPID id, PPBasketPacket * pData, long options)
{
	int    ok = 1;
	if(options & gpoProcessPrivate && IsPrivate(id)) {
		*pData = *DS.GetPrivateBasket();
		ok = 1;
	}
	else {
		PPGoodsBasket gb;
		PPGoodsBasketItem gbi;
		THROW(P_Ref->GetItem(Obj, id, &gb) > 0);
		pData->destroy();
		pData->Head.Tag     = gb.Tag;
		pData->Head.ID      = gb.ID;
		pData->Head.Num     = gb.Num;
		pData->Head.Flags   = gb.Flags;
		pData->Head.User    = gb.User;
		pData->Head.SupplID = gb.SupplID;
		STRNSCPY(pData->Head.Name, gb.Name);
		memzero(pData->Head.Reserve1, sizeof(pData->Head.Reserve1));
		pData->Lots.freeAll();
		int    new_load_method = 1; // Проверено: этот метод быстрее
		if(new_load_method) {
			PROFILE_START
			for(SEnum en = P_Ref->Assc.Enum(PPASS_GOODSBASKET, gb.ID, 0); en.Next(&gbi) > 0;) {
				ILTI   item;
				item.GoodsID     = gbi.ItemGoodsID;
				item.Flags       = gbi.Flags;
				item.Quantity    = gbi.Quantity;
				item.Rest        = gbi.Quantity;
				item.Price       = gbi.Price;
				item.UnitPerPack = gbi.UnitPerPack;
				item.Expiry      = gbi.Expiry;
				item.BillID      = static_cast<PPID>(gbi.Num);
				THROW_SL(pData->Lots.insert(&item));
			}
			PROFILE_END
		}
		else {
			PROFILE_START
			PPID   scnd_id = -MAXLONG;
			while(P_Ref->Assc.EnumByPrmr(PPASS_GOODSBASKET, gb.ID, &scnd_id, reinterpret_cast<ObjAssocTbl::Rec *>(&gbi)) > 0) {
				ILTI   item;
				item.GoodsID     = gbi.ItemGoodsID;
				item.Flags       = gbi.Flags;
				item.Quantity    = gbi.Quantity;
				item.Rest        = gbi.Quantity;
				item.Price       = gbi.Price;
				item.UnitPerPack = gbi.UnitPerPack;
				item.Expiry      = gbi.Expiry;
				item.BillID      = static_cast<PPID>(gbi.Num);
				THROW_SL(pData->Lots.insert(&item));
			}
			PROFILE_END
		}
	}
	if(pData->Head.Flags & GBASKF_SORTITEMS) {
		PPObjGoods goods_obj;
		pData->Lots.sort(PTR_CMPFUNC(ILTIGOODS));
	}
	else
		pData->Lots.sort(PTR_CMPFUNC(ILTI));
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::PutPacket(PPID * pID, PPBasketPacket * pData, int use_ta)
{
	int    ok = -1, r;
	int    skip_items = 0;
	long   action = 0;
	uint   i;
	SVector items(sizeof(ObjAssocTbl::Rec)); // @v10.7.3 SArray-->SVector
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		if(pData) {
			if(pData->Head.Flags & GBASKF_PRIVATE) {
				THROW(DS.SetPrivateBasket(pData, 0));
				skip_items = 1;
			}
			else {
				PPBasketPacket * p_private = DS.GetPrivateBasket();
				if(p_private && *pID && p_private->Head.ID == *pID) {
					//
					// Сохраняемая корзина была приватной, но потеряла этот статус:
					// сначала сохраняем пакет как приватный, затем очищаем
					// приватную корзину. Таким образом, изменения будут сохранены.
					//
					THROW(DS.SetPrivateBasket(pData, 0));
					THROW(DS.SetPrivateBasket(0, 0));
					skip_items = 1;
				}
				else {
					ILTI * pi;
					for(i = 0; pData->Lots.enumItems(&i, (void **)&pi);) {
						PPGoodsBasketItem gbi;
						MEMSZERO(gbi);
						gbi.ItemGoodsID = pi->GoodsID;
						gbi.Flags       = pi->Flags;
						gbi.Quantity    = pi->Quantity;
						gbi.Price       = pi->Price;
						gbi.UnitPerPack = pi->UnitPerPack;
						gbi.Expiry      = pi->Expiry;
						gbi.Num         = i;
						THROW_SL(items.insert(&gbi));
					}
					PPGoodsBasket head;
					MEMSZERO(head);
					head.Tag = Obj;
					head.ID = *pID;
					STRNSCPY(head.Name, pData->Head.Name);
					head.Num = pData->Head.Num;
					head.Flags = (pData->Head.Flags & ~GBASKF_PRIVATE);
					head.User = pData->Head.User;
					head.SupplID = pData->Head.SupplID;
					if(*pID == 0){
						THROW(P_Ref->AddItem(Obj, pID, &head, 0));
						pData->Head.ID = *pID;
						ok = 1;
					}
					else {
						THROW(r = P_Ref->UpdateItem(Obj, *pID, &head, 0, 0));
						action = PPACN_OBJUPD;
						if(r > 0)
							ok = 1;
					}
				}
			}
		}
		else {
			THROW(P_Ref->RemoveItem(Obj, *pID, 0));
			ok = 1;
		}
		if(!skip_items) {
			if(items.getCount() == 0) {
				THROW(P_Ref->Assc.Remove(PPASS_GOODSBASKET, *pID, 0, 0));
				ok = 1;
			}
			else {
				long   last_num = 0;
				PPGoodsBasketItem gbi;
				SVector prev_items(sizeof(ObjAssocTbl::Rec)); // @v10.7.3 SArray-->SVector
				LongArray found_pos_list;
				for(SEnum en = P_Ref->Assc.Enum(PPASS_GOODSBASKET, *pID, 0); en.Next(&gbi) > 0;) {
					THROW_SL(prev_items.insert(&gbi));
					SETMAX(last_num, gbi.Num);
				}
				for(i = 0; i < prev_items.getCount(); i++) {
					uint pos = 0;
					gbi = *static_cast<const PPGoodsBasketItem *>(prev_items.at(i));
					if(items.lsearch(&gbi.ItemGoodsID, &pos, PTR_CMPFUNC(long), offsetof(PPGoodsBasketItem, ItemGoodsID))) {
						const PPGoodsBasketItem * p_list_item = static_cast<const PPGoodsBasketItem *>(items.at(pos));
						found_pos_list.add(static_cast<long>(pos));
						if(p_list_item->Flags != gbi.Flags || p_list_item->Quantity != gbi.Quantity ||
							p_list_item->Price != gbi.Price || p_list_item->UnitPerPack != gbi.UnitPerPack ||
							p_list_item->Expiry != gbi.Expiry) {
							gbi.Flags = p_list_item->Flags;
							gbi.Quantity = p_list_item->Quantity;
							gbi.Price = p_list_item->Price;
							gbi.UnitPerPack = p_list_item->UnitPerPack;
							gbi.Expiry = p_list_item->Expiry;
							THROW(P_Ref->Assc.Update(gbi.Id, reinterpret_cast<ObjAssocTbl::Rec *>(&gbi), 0));
							ok = 1;
						}
					}
					else {
						if(gbi.Num == last_num)
							last_num--;
						THROW(P_Ref->Assc.Remove(gbi.Id, 0));
						ok = 1;
					}
				}
				{
					BExtInsert bei(&P_Ref->Assc);
					for(i = 0; i < items.getCount(); i++) {
						if(!found_pos_list.lsearch((long)i)) {
							PPGoodsBasketItem * p_list_item = static_cast<PPGoodsBasketItem *>(items.at(i));
							p_list_item->Tag = PPASS_GOODSBASKET;
							p_list_item->BasketID = *pID;
							p_list_item->Num = ++last_num;
							THROW_DB(bei.insert(p_list_item));
							ok = 1;
						}
					}
					THROW_DB(bei.flash());
				}
			}
		}
		if(ok > 0 && action)
			DS.LogAction(action, Obj, *pID, 0, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

IMPL_DESTROY_OBJ_PACK(PPObjGoodsBasket, PPBasketPacket);

int PPObjGoodsBasket::SerializePacket(int dir, PPBasketPacket * pPack, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW_SL(P_Ref->SerializeRecord(dir, &pPack->Head, rBuf, pSCtx));
	THROW(pPack->SerializeLots(dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::Read(PPObjPack * pPack, PPID id, void * stream, ObjTransmContext * pCtx)
	{ return Implement_ObjReadPacket<PPObjGoodsBasket, PPBasketPacket>(this, pPack, id, stream, pCtx); }

int PPObjGoodsBasket::Write(PPObjPack * pPack, PPID * pID, void * stream, ObjTransmContext * pCtx) // @srlz
{
	int    ok = 1;
	PPBasketPacket * p_pack = static_cast<PPBasketPacket *>(pPack->Data);
	if(p_pack) {
		if(stream == 0) {
			if(*pID == 0) {
				if(SearchByName(pID, p_pack->Head.Name) > 0)
					ok = -1;
				else if(!PutPacket(pID, p_pack, 1)) {
					pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTBASKET, p_pack->Head.ID, p_pack->Head.Name);
					ok = -1;
				}
			}
			else if(!PutPacket(pID, p_pack, 1)) {
				pCtx->OutputAcceptErrMsg(PPTXT_ERRACCEPTBASKET, p_pack->Head.ID, p_pack->Head.Name);
				ok = -1;
			}
		}
		else {
			SBuffer buffer;
			THROW(SerializePacket(+1, p_pack, buffer, &pCtx->SCtx));
			THROW_SL(buffer.WriteToFile(static_cast<FILE *>(stream), 0, 0))
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::ProcessObjRefs(PPObjPack * p, PPObjIDArray * ary, int replace, ObjTransmContext * pCtx)
{
	if(p && p->Data) {
		PPBasketPacket * p_pack= static_cast<PPBasketPacket *>(p->Data);
		ILTI * p_item = 0;
		for(uint i = 0; p_pack->Lots.enumItems(&i, (void **)&p_item);)
			if(!ProcessObjRefInArray(PPOBJ_GOODS, &p_item->GoodsID, ary, replace))
				return 0;
		return 1;
	}
	return -1;
}

int PPObjGoodsBasket::Transfer(PPID id)
{
	int    ok = -1;
	PPObjectTransmit * p_ot = 0;
	if(id) {
		THROW(!PPObjGoodsBasket::IsLocked(id));
		{
			PPBasketPacket gb_packet;
			ObjTransmitParam param;
			PPObjGoodsBasket::Locking lck(id);
			THROW(GetPacket(id, &gb_packet));
			if(ObjTransmDialog(DLG_OBJTRANSM, &param) > 0) {
				const PPIDArray & rary = param.DestDBDivList.Get();
				//const int sync_cmp = BIN(param.Flags & ObjTransmitParam::fSyncCmp);
				//const int recover_transmission = BIN(param.Flags & param.fRecoverTransmission);
				uint ot_ctrf = 0;
				if(param.Flags & ObjTransmitParam::fSyncCmp) 
					ot_ctrf |= PPObjectTransmit::ctrfSyncCmp;
				if(param.Flags & param.fRecoverTransmission)
					ot_ctrf |= PPObjectTransmit::ctrfRecoverTransmission;
				for(uint i = 0; i < rary.getCount(); i++) {
					PPWaitStart();
					THROW_MEM(p_ot = new PPObjectTransmit(PPObjectTransmit::tmWriting, ot_ctrf/*sync_cmp, recover_transmission*/));
					THROW(p_ot->SetDestDbDivID(rary.at(i)));
					THROW(p_ot->PostObject(PPOBJ_GOODSBASKET, id, param.UpdProtocol, BIN(param.Flags & ObjTransmitParam::fSyncCmp)));
					THROW(p_ot->CreateTransmitPacket());
					ZDELETE(p_ot);
					THROW(PutTransmitFiles(rary.at(i), param.TrnsmFlags));
					PPWaitStop();
				}
				ok = 1;
			}
		}
	}
	CATCHZOKPPERR
	delete p_ot;
	return ok;
}

int PPObjGoodsBasket::ClearDefForBaskets(int use_ta)
{
	int    ok = -1;
	PPBasketPacket gb_packet;
	PPGoodsBasket gb_rec;
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		for(PPID id = 0; P_Ref->EnumItems(Obj, &id, &gb_rec) > 0;) {
			if(gb_rec.Flags & GBASKF_DEFAULT) {
				gb_rec.Flags &= ~GBASKF_DEFAULT;
				THROW(P_Ref->UpdateItem(Obj, id, &gb_rec, 1, 0));
			}
			ok = 1;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::SearchByName(PPID * pID, const char * pName, PPBasketPacket * pPacket)
{
	int    ok = -1;
	PPID   id = 0;
	PPGoodsBasket gb_rec;
	if(PPObjReference::SearchByName(pName, &id, &gb_rec) > 0) {
		if(pPacket)
			THROW(GetPacket(id, pPacket) > 0);
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::SearchDefaultBasket(PPID * pID, PPGoodsBasket * pRec)
{
	int    ok = -1;
	long   eh = -1;
	if(P_Ref->InitEnum(Obj, 0, &eh) > 0) {
		PPGoodsBasket rec;
		while(ok < 0 && P_Ref->NextEnum(eh, &rec) > 0)
			if(rec.Flags & GBASKF_DEFAULT) {
				ASSIGN_PTR(pID, rec.ID);
				ASSIGN_PTR(pRec, rec);
				ok = 1;
			}
		P_Ref->DestroyIter(eh);
	}
	return ok;
}

int PPObjGoodsBasket::GetPreferredBasket(PPBasketCombine & rC)
{
	int    ok = -1;
	PPBasketPacket * p_private_cart = DS.GetPrivateBasket();
	if(p_private_cart) {
		rC.Pack = *p_private_cart;
		rC.BasketID = p_private_cart->Head.ID;
		ok = 1;
	}
	else {
		PPID   id = DS.GetTLA().Lid.BasketID;
		PPGoodsBasket rec;
		SString msg_buf, fmt_buf;
		if(id && Search(id, &rec) > 0) {
			if(IsLocked(id)) {
				PPLoadText(PPTXT_OWNBASKETISLOCKED, fmt_buf);
				msg_buf.Printf(fmt_buf, rec.Name, DS.GetConstTLA().AddedMsgString.cptr());
				if(Select(&(id = 0), msg_buf) > 0)
					ok = 1;
			}
			else
				ok = 1;
		}
		else if(SearchDefaultBasket(&id, &rec) > 0) {
			if(IsLocked(id)) {
				PPLoadText(PPTXT_DEFBASKETISLOCKED, fmt_buf);
				msg_buf.Printf(fmt_buf, rec.Name, DS.GetConstTLA().AddedMsgString.cptr());
				if(Select(&(id = 0), msg_buf) > 0)
					ok = 1;
			}
			else
				ok = 1;
		}
		else if(Select(&(id = 0), 0) > 0)
			ok = 1;
		if(ok > 0) {
			THROW(rC.Lck.Lock(id));
			THROW(GetPacket(id, &rC.Pack));
			rC.BasketID = id;
		}
	}
	CATCHZOK
	return ok;
}

int PPObjGoodsBasket::SelectBasket(PPBasketCombine & rBasket)
{
	int    ok = -1;
	rBasket.BasketID = 0;
	if(GetPreferredBasket(rBasket) > 0) {
		int  r = GoodsBasketDialog(rBasket, 0);
		if(r > 0) {
			if(r == 2 && CONFIRM(PPCFM_BASKETISCHANGED))
				THROW(GetPacket(rBasket.BasketID, &rBasket.Pack, gpoProcessPrivate));
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

/*virtual*/int PPObjGoodsBasket::ProcessReservedItem(TVRez & rRez)
{
	int    ok = 1;
	int    r;
	SString name;
	SString symb;
	PPID   id = static_cast<PPID>(rRez.getUINT());
	rRez.getString(name, 2);
	PPExpandString(name, CTRANSF_UTF8_TO_INNER);
	rRez.getString(symb, 2);
	THROW(r = Search(id));
	if(r < 0) {
		ReferenceTbl::Rec rec;
		// @v10.6.8 @ctr MEMSZERO(rec);
		rec.ObjType = Obj;
		rec.ObjID   = id;
		STRNSCPY(rec.ObjName, name);
		STRNSCPY(rec.Symb, symb);
		THROW(StoreItem(Obj, 0, &rec, 1));
	}
	CATCHZOK
	return ok;
}

/*virtual*/void * PPObjGoodsBasket::CreateObjListWin(uint flags, void * extraPtr)
{
	class PPObjGoodsBasketListWindow : public PPObjListWindow {
	public:
		PPObjGoodsBasketListWindow(PPObject * pObj, uint flags, void * extraPtr) : PPObjListWindow(pObj, flags, extraPtr)
		{
			DefaultCmd = cmaEdit;
			SetToolbar(TOOLBAR_LIST_GOODSBASKET);
		}
	private:
		DECL_HANDLE_EVENT
		{
			int    update = 0;
			PPID   id = 0;
			PPObjListWindow::handleEvent(event);
			if(P_Obj) {
				getResult(&id);
				if(TVCOMMAND) {
					switch(TVCMD) {
						case cmaInsert:
							id = 0;
							if(Flags & OLW_CANINSERT && P_Obj->Edit(&id, ExtraPtr) == cmOK)
								update = 2;
							else
								::SetFocus(H());
							break;
						case cmaMore:
							if(id) {
								PPBasketCombine bc;
								bc.BasketID = id;
								if(PPObjGoodsBasket::IsPrivate(id)) {
									bc.Pack = *DS.GetPrivateBasket();
									GoodsBasketDialog(bc, 1);
									update = 2;
								}
								else if(!PPObjGoodsBasket::IsLocked(id)) {
									bc.Lck.Lock(id);
									if(static_cast<PPObjGoodsBasket *>(P_Obj)->GetPacket(id, &bc.Pack)) {
										GoodsBasketDialog(bc, 1);
										update = 2;
									}
									else
										PPError();
								}
								else
									PPError();
							}
							break;
						case cmForceUnlock:
							if(id)
								PPObjGoodsBasket::ForceUnlock(id);
							break;
					}
				}
				PostProcessHandleEvent(update, id);
			}
		}
		virtual int Transmit(PPID id)
		{
			return id ? static_cast<PPObjGoodsBasket *>(P_Obj)->Transfer(id) : -1;
		}
	};
	return /*0; */ new PPObjGoodsBasketListWindow(this, flags, extraPtr);
}
//
//
//
class GoodsBasketView : public ObjViewDialog {
public:
	GoodsBasketView(PPObjGoodsBasket * _ppobj, const char * pMsg, int asSelector) :
		ObjViewDialog(asSelector ? DLG_SELBASKET : DLG_GOODSBASKETVIEW, _ppobj, 0), AsSelector(asSelector)
	{
		setStaticText(CTL_OBJVIEW_ST_MSG, pMsg);
	}
private:
	virtual void extraProc(long id)
	{
		if(id) {
			PPBasketCombine bc;
			bc.BasketID = id;
			if(PPObjGoodsBasket::IsPrivate(id)) {
				bc.Pack = *DS.GetPrivateBasket();
				GoodsBasketDialog(bc, 1);
				updateList(-1);
			}
			else if(!PPObjGoodsBasket::IsLocked(id)) {
				bc.Lck.Lock(id);
				if(static_cast<PPObjGoodsBasket *>(P_Obj)->GetPacket(id, &bc.Pack)) {
					GoodsBasketDialog(bc, 1);
					updateList(-1);
				}
				else
					PPError();
			}
			else
				PPError();
		}
	}
	DECL_HANDLE_EVENT
	{
		ObjViewDialog::handleEvent(event);
		if(event.isCmd(cmaTransfer))
			static_cast<PPObjGoodsBasket *>(P_Obj)->Transfer(getCurrID());
		else if(event.isKeyDown(kbCtrlX))
			PPObjGoodsBasket::ForceUnlock(getCurrID());
		else
			return;
		clearEvent(event);
	}
	int    AsSelector;
};

int PPObjGoodsBasket::Browse(void * extraPtr)
{
	int    ok = 1;
	if(CheckRights(PPR_READ)) {
		TDialog * dlg = new GoodsBasketView(this, 0, 0);
		if(CheckDialogPtrErr(&dlg))
			ExecViewAndDestroy(dlg);
	}
	else
		ok = PPErrorZ();
	return ok;
}


/*static*/int PPObjGoodsBasket::IsPrivate(PPID id)
{
	PPBasketPacket * p_private_pack = DS.GetPrivateBasket();
	return BIN(p_private_pack && p_private_pack->Head.ID == id);
}

int PPObjGoodsBasket::Select(PPID * pID, const char * pMsg)
{
	int    ok = -1;
	PPID   sel_id = 0;
	TDialog * dlg = new GoodsBasketView(this, pMsg, 1);
	if(CheckDialogPtrErr(&dlg))
		while(!sel_id && ExecView(dlg) == cmOK) {
			dlg->getCtrlData(CTL_OBJVIEW_LIST, &sel_id);
			if(sel_id && Search(sel_id, 0) > 0)
				if(!IsPrivate(sel_id) && IsLocked(sel_id))
					sel_id = PPErrorZ();
				else
					ok = 1;
			else
				sel_id = 0;
		}
	ASSIGN_PTR(pID, sel_id);
	delete dlg;
	return ok;
}

int PPObjGoodsBasket::Edit(PPID * pID, void * extraPtr)
{
	int    ok = cmCancel;
	int    valid_data = 0;
	int    is_locked = 0;
	PPObjGoods goods_obj; // Загружаем объект для того, чтобы во вложенных вызовах он был открыт
	PPGoodsBasket  org_rec;
	PPID   org_id = 0;
	PPBasketCombine bc;
	THROW_INVARG(pID);
	org_id = bc.BasketID = *pID;
	if(PPObjGoodsBasket::IsPrivate(*pID)) {
		PPBasketPacket * p_private_cart = DS.GetPrivateBasket();
		bc.Pack = *p_private_cart;
		bc.BasketID = p_private_cart->Head.ID;
	}
	else if(bc.BasketID) {
		THROW(PPObjGoodsBasket::IsLocked(bc.BasketID) == 0);
		bc.Lck.Lock(bc.BasketID);
		THROW(GetPacket(bc.BasketID, &bc.Pack));
	}
	org_rec = bc.Pack.Head;
	while(!valid_data && GoodsBasketDialog(bc, 3) > 0) {
		PPID   def_id = 0;
		valid_data = 1;
		if(bc.Pack.Head.Flags & GBASKF_DEFAULT && !(org_rec.Flags & GBASKF_DEFAULT)) {
			int    r;
			THROW(r = SearchDefaultBasket(&def_id, 0));
			if(r > 0 && def_id != org_id) {
				if(IsLocked(def_id))
					if(CONFIRM(PPCFM_CANTWRITEATTRIB))
						bc.Pack.Head.Flags &= GBASKF_DEFAULT;
					else
						valid_data = 0;
			}
			else
				def_id = 0;
		}
		if(valid_data) {
			PPGoodsBasket def_rec;
			PPTransaction tra(1);
			THROW(tra);
			if(def_id && Search(def_id, &def_rec) > 0) {
				def_rec.Flags &= ~GBASKF_DEFAULT;
				THROW(UpdateItem(def_id, &def_rec, 0));
			}
			if(bc.Pack.Head.Flags & GBASKF_DEFAULT)
				THROW(ClearDefForBaskets(0));
			if(bc.Pack.Head.Flags & GBASKF_PRIVATE)
				bc.Lck.Unlock();
			THROW(PutPacket(pID, &bc.Pack, 0));
			THROW(tra.Commit());
			ok = cmOK;
		}
	}
	CATCHZOKPPERR
	return ok;
}
//
//
//
SelBasketParam::SelBasketParam() : PPBasketCombine(), SelPrice(0), SelReplace(1), Flags(0)
{
}

int SelBasketParam::StoreInReg(const char * pName) const
{
	int    ok = -1;
	if(!isempty(pName)) {
		SString temp_buf;
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::PrefBasketSelSettings, 0);
		StringSet ss(';', 0);
		ss.add(temp_buf.Z().Cat(SelPrice));
		ss.add(temp_buf.Z().Cat(SelReplace));
		ok = reg_key.PutString(pName, ss.getBuf()) ? 1 : PPSetErrorSLib();
	}
	return ok;
}

int SelBasketParam::RestoreFromReg(const char * pName)
{
	int    ok = -1;
	if(!isempty(pName)) {
		SString temp_buf;
		WinRegKey reg_key(HKEY_CURRENT_USER, PPRegKeys::PrefBasketSelSettings, 1);
		if(reg_key.GetString(pName, temp_buf)) {
			StringSet ss(';', temp_buf);
			uint i = 0;
			if(ss.get(&i, temp_buf))
				SelPrice = temp_buf.ToLong();
			if(ss.get(&i, temp_buf))
				SelReplace = temp_buf.ToLong();
		}
	}
	return ok;
}

int GetBasketByDialog(SelBasketParam * pParam, const char * pCallerSymb, uint dlgID)
{
	class GBDataDialog : public TDialog {
	public:
		GBDataDialog(uint dlgID, SelBasketParam & rData, const char * pCallerSymb) : TDialog(dlgID), R_Data(rData), P_CallerSymb(pCallerSymb)
		{
		}
		int setDTS()
		{
			int    ok = 1;
			ushort v = 0;
			SetupPPObjCombo(this, CTLSEL_GBDATA_NAMEDSTRUC, PPOBJ_GOODSBASKET, R_Data.BasketID, OLW_LOADDEFONOPEN|OLW_CANINSERT, 0);
			if(!BillObj->CheckRights(BILLRT_ACCSCOST)) {
				DisableClusterItem(CTL_GBDATA_SELPRICE, 0, 1);
				if(oneof2(R_Data.SelPrice, 0, 1))
					R_Data.SelPrice = 2;
			}
			switch(R_Data.SelPrice) {
				case 1: v = 0; break;
				case 2: v = 1; break;
				case 3: v = 2; break;
				case 4: v = 3; break;
				default: v = 0; break;
			}
			setCtrlUInt16(CTL_GBDATA_SELPRICE, v);
			disableCtrl(CTL_GBDATA_SELPRICE, BIN(R_Data.Flags & SelBasketParam::fNotSelPrice));
			AddClusterAssocDef(CTL_GBDATA_REPLACE, 0, 1);
			AddClusterAssoc(CTL_GBDATA_REPLACE, 1, 2);
			AddClusterAssoc(CTL_GBDATA_REPLACE, 2, 3);
			SetClusterData(CTL_GBDATA_REPLACE, R_Data.SelReplace);
			AddClusterAssoc(CTL_GBDATA_SELGOODSREST, 0, SelBasketParam::fUseGoodsRestAsQtty);
			AddClusterAssoc(CTL_GBDATA_SELGOODSREST, 1, SelBasketParam::fFillUpToMinStock);
			SetClusterData(CTL_GBDATA_SELGOODSREST, R_Data.Flags);
			DisableClusterItem(CTL_GBDATA_SELGOODSREST, 0, (R_Data.Flags & SelBasketParam::fEnableFillUpToMinStock) ? 1 : 0);
			DisableClusterItem(CTL_GBDATA_SELGOODSREST, 1, (R_Data.Flags & SelBasketParam::fEnableFillUpToMinStock) ? 0 : 1);
			return ok;
		}
		int getDTS()
		{
			int    ok = 1;
			R_Data.BasketID = getCtrlLong(CTLSEL_GBDATA_NAMEDSTRUC);
			ushort v = getCtrlUInt16(CTL_GBDATA_SELPRICE);
			switch(v) {
				case 0: R_Data.SelPrice = 1; break;
				case 1: R_Data.SelPrice = 2; break;
				case 2: R_Data.SelPrice = 3; break;
				case 3: R_Data.SelPrice = 4; break;
				default: R_Data.SelPrice = 1; break;
			}
			GetClusterData(CTL_GBDATA_REPLACE, &R_Data.SelReplace);
			GetClusterData(CTL_GBDATA_SELGOODSREST, &R_Data.Flags);
			if(PPObjGoodsBasket::IsPrivate(R_Data.BasketID) || R_Data.Lck.Lock(R_Data.BasketID)) {
				if(GbObj.GetPacket(R_Data.BasketID, &R_Data.Pack, PPObjGoodsBasket::gpoProcessPrivate) > 0) {
					if(R_Data.SelReplace == 1)
						R_Data.Pack.Lots.freeAll();
					R_Data.StoreInReg(P_CallerSymb);
				}
				else
					ok = PPErrorZ();
			}
			else
				ok = PPErrorZ();
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GBDATA_NAMEDSTRUC)) {
				PPID   prev_id = R_Data.Pack.Head.ID;
				PPID   id = getCtrlLong(CTLSEL_GBDATA_NAMEDSTRUC);
				if(id != prev_id) {
					int    err = 0;
					if(PPObjGoodsBasket::IsPrivate(id)) {
						R_Data.Lck.Unlock();
						if(GbObj.GetPacket(id, &R_Data.Pack, PPObjGoodsBasket::gpoProcessPrivate) > 0)
							R_Data.BasketID = id;
						else
							err = 1;
					}
					else if(!PPObjGoodsBasket::IsLocked(id)) {
						R_Data.Lck.Lock(id);
						if(GbObj.GetPacket(id, &R_Data.Pack, PPObjGoodsBasket::gpoProcessPrivate) > 0)
							R_Data.BasketID = id;
						else
							err = 1;
					}
					else
						err = 1;
					if(err) {
						PPError();
						setCtrlLong(CTLSEL_GBDATA_NAMEDSTRUC, prev_id);
					}
				}
			}
			else if(event.isCmd(cmShowBasket)) {
				if(!showBasket(getCtrlLong(CTLSEL_GBDATA_NAMEDSTRUC)))
					PPError();
			}
			else
				return;
			clearEvent(event);
		}
		int    showBasket(PPID gbID)
		{
			int    ok = -1;
			if(gbID) {
				PPBasketCombine bc;
				bc.BasketID = gbID;
				//
				// Здесь не следует блокировать корзину поскольку
				// она была заблокирована при выборе в комбо-боксе.
				//
				THROW(GbObj.GetPacket(gbID, &bc.Pack, PPObjGoodsBasket::gpoProcessPrivate));
				THROW(GoodsBasketDialog(bc, 1));
				ok = 1;
			}
			CATCH
				ok = 0;
			ENDCATCH
			return ok;
		}
		SelBasketParam & R_Data;
		const char * P_CallerSymb;
		PPObjGoodsBasket GbObj;
	};
	int    ok = -1, valid_data = 0;
	uint   dlg_id = NZOR(dlgID, DLG_GBDATA);
	PPObjGoodsBasket gb_obj;
	GBDataDialog * p_dlg = 0;
	if(gb_obj.GetPreferredBasket(*pParam) > 0) {
		p_dlg = new GBDataDialog(dlg_id, *pParam, pCallerSymb);
		THROW(CheckDialogPtr(&p_dlg) > 0);
		pParam->RestoreFromReg(pCallerSymb);
		p_dlg->setDTS();
		while(ok < 0 && ExecView(p_dlg) == cmOK)
			if(p_dlg->getDTS())
				ok = 1;
	}
	CATCHZOK
	pParam->Lck.Unlock();
	delete p_dlg;
	return ok;
}
//
// PPViewGoodsBasket
//
PPViewGoodsBasket::PPViewGoodsBasket(PPBasketPacket * pPacket) :
	IterCount(0), NumIters(0), P_OrdTbl(0), P_IterQuery(0), P_GBPacket(pPacket), Order(0), Flags(0)
{
}

PPViewGoodsBasket::~PPViewGoodsBasket()
{
	ZDELETE(P_OrdTbl);
	BExtQuery::ZDelete(&P_IterQuery);
	DBRemoveTempFiles();
}

const IterCounter & PPViewGoodsBasket::GetIterCounter() const { return Counter; }
const PPBasketPacket * PPViewGoodsBasket::GetPacket() const { return P_GBPacket; }

int PPViewGoodsBasket::Init(int ord)
{
	int    ok = 1;
	ZDELETE(P_OrdTbl);
	Order = ord;
	ok = CreateOrderTable();
	return ok;
}

int PPViewGoodsBasket::InitIteration()
{
	int    ok = 0;
	TempOrderTbl::Key1 k;
	PPObjGoods g_obj;
	MEMSZERO(k);
	Counter.Init(P_GBPacket->Lots.getCount());
	BExtQuery::ZDelete(&P_IterQuery);
	if(P_OrdTbl) {
		THROW_MEM(P_IterQuery = new BExtQuery(P_OrdTbl, 1, 64));
		P_IterQuery->selectAll();
		P_IterQuery->initIteration(0, &k, spGe);
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int FASTCALL PPViewGoodsBasket::NextIteration(ILTI * pItem)
{
	int    ok = -1;
	if(P_IterQuery)
		if(P_IterQuery->nextIteration() > 0) {
			TempOrderTbl::Rec item = P_OrdTbl->data;
			uint   pos = (uint)item.ID;
			if(pos < P_GBPacket->Lots.getCount()) {
				if(pItem) {
					*pItem = P_GBPacket->Lots.at(pos);
					if(Flags & PPViewGoodsBasket::fHideCost)
						pItem->Price = 0;
				}
			}
			Counter.Increment();
			ok = 1;
		}
	return ok;
}

int PPViewGoodsBasket::Print()
{
	int    ok = -1;
	uint   rpt_id = 0;
	ushort v = 0;
	TDialog * p_dlg = new TDialog(DLG_GBPRINT);
	if(CheckDialogPtrErr(&p_dlg)) {
		p_dlg->setCtrlData(CTL_GBPRINT_REPORT, &(v = 0));
		p_dlg->setCtrlData(CTL_GBPRINT_REPORT, &(v = 0));
		if(ExecView(p_dlg) == cmOK) {
			PPReportEnv env;
			p_dlg->getCtrlData(CTL_GBPRINT_REPORT, &v);
			if(v == 0) {
				rpt_id = REPORT_GOODSBASKET;
				env.Sort = PPObjGoodsBasket::ordByDefault;
			}
			else if(v == 1) {
				rpt_id = REPORT_GOODSBASKETGRPNG;
				env.Sort = PPObjGoodsBasket::ordByGroup;
			}
			p_dlg->getCtrlData(CTL_GBPRINT_FLAGS, &(v = 0));
			if(v & 0x01)
				Flags |= PPViewGoodsBasket::fHideCost;
			PPAlddPrint(rpt_id, PView(this), &env);
			ok = 1;
		}
	}
	delete p_dlg;
	return ok;
}

int PPViewGoodsBasket::CreateOrderTable()
{
	int    ok = 1;
	uint   i = 0;
	ILTI * p_item;
	BExtInsert * p_bei = 0;
	SString goods_name, grp_name, temp_buf;
	PPObjGoods goods_obj;

	THROW(P_GBPacket);
	THROW(P_OrdTbl = CreateTempOrderFile());
	THROW_MEM(p_bei = new BExtInsert(P_OrdTbl));
	for(i = 0; P_GBPacket->Lots.enumItems(&i, (void **)&p_item);) {
		TempOrderTbl::Rec rec;
		rec.ID = i - 1;
		temp_buf.Z();
		if(Order == PPObjGoodsBasket::ordByDefault)
			temp_buf.CatLongZ(i, 8);
		else {
			Goods2Tbl::Rec goods_rec, grp_rec;
			if(goods_obj.Fetch(p_item->GoodsID, &goods_rec) > 0)
				goods_name = goods_rec.Name;
			else {
				MEMSZERO(goods_rec);
				ideqvalstr(p_item->GoodsID, goods_name.Z());
			}
			if(Order == PPObjGoodsBasket::ordByGoods)
				temp_buf = goods_name;
			else if(Order == PPObjGoodsBasket::ordByGroup) {
				grp_name = (goods_obj.Fetch(goods_rec.ParentID, &grp_rec) > 0) ? grp_rec.Name : static_cast<const char *>(0);
				grp_name.Cat(goods_name);
			}
		}
		temp_buf.CopyTo(rec.Name, sizeof(rec.Name));
		THROW_DB(p_bei->insert(&rec));
	}
	THROW_DB(p_bei->flash());
	CATCH
		ok = 0;
		ZDELETE(p_bei);
		ZDELETE(P_OrdTbl);
	ENDCATCH
	delete p_bei;
	return ok;
}
//
//
//
int PPObjGoodsBasket::Print(PPBasketPacket * pData)
{
	PPViewGoodsBasket v_gb(pData);
	return v_gb.Print();
}
//
// GBItemDialog
//
class GBItemDialog : public TDialog {
	DECL_DIALOG_DATA(ILTI);
	enum {
		ctlgroupGoods = 1
	};
public:
	enum {
		fEnableChangeBasket = 0x0001,
		fFocusOnPckg        = 0x0002
	};
	//
	// Parameters:
	//   defLocID - склад, используемый для нахождения последнего лота
	//     товара с целью определения текущих параметров.
	//     Если defLocID == 0, то считается, что он равен LConfig.Location.
	//
	GBItemDialog(PPBasketCombine & rCart, PPID defLocID, long flags) :
		TDialog(DLG_GBITEM), R_Cart(rCart), DefLocID(NZOR(defLocID, LConfig.Location)), Flags(flags)
	{
		//EnableChangeBasket = enableChgBasket;
		// @v10.7.3 @ctr MEMSZERO(Item);
		SetupCalDate(CTLCAL_GBITEM_EXPIRY, CTL_GBITEM_EXPIRY);
	}
	DECL_DIALOG_SETDTS()
	{
		RVALUEPTR(Data, pData);
		int    ok = 1;
		LDATE  expiry;
		ReceiptTbl::Rec lot_rec;
		GoodsCtrlGroup::Rec rec(0, Data.GoodsID, 0, GoodsCtrlGroup::enableSelUpLevel | GoodsCtrlGroup::disableEmptyGoods);
		enableCommand(cmShowBasket, BIN(Flags & fEnableChangeBasket));
		disableCtrls(1, CTL_GBITEM_LINESCOUNT, CTL_GBITEM_AMOUNT, CTL_GBITEM_BRUTTO, 0);
		disableCtrls(!(Flags & fEnableChangeBasket), CTLSEL_GBITEM_BASKET, CTLSEL_GBITEM_SUPPL, CTL_GBITEM_PRIVATE, 0);
		addGroup(ctlgroupGoods, new GoodsCtrlGroup(CTLSEL_GBITEM_GGRP, CTLSEL_GBITEM_GOODS));
		getLotInfo(Data.GoodsID, &lot_rec);
		SetupPPObjCombo(this, CTLSEL_GBITEM_BASKET, PPOBJ_GOODSBASKET, R_Cart.Pack.Head.ID, OLW_LOADDEFONOPEN|OLW_CANINSERT, 0);
		SetupArCombo(this, CTLSEL_GBITEM_SUPPL, R_Cart.Pack.Head.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
		setCtrlUInt16(CTL_GBITEM_PRIVATE, BIN(R_Cart.Pack.Head.Flags & GBASKF_PRIVATE));
		setGroupData(ctlgroupGoods, &rec);
		setCtrlData(CTL_GBITEM_VALUE, &Data.Quantity);
		SETIFZ(Data.UnitPerPack, lot_rec.UnitPerPack);
		if(Data.UnitPerPack > 0)
			setCtrlReal(CTL_GBITEM_QTTYPACK, Data.Quantity / Data.UnitPerPack);
		setCtrlData(CTL_GBITEM_UPPACK, &Data.UnitPerPack);
		if(!checkdate(Data.Expiry))
			Data.Expiry = lot_rec.Expiry;
		expiry = Data.Expiry;
		setCtrlData(CTL_GBITEM_EXPIRY, &expiry);
		if(Data.Price == 0.0 && Data.GoodsID)
			Data.Price = R5(BillObj->CheckRights(BILLRT_ACCSCOST) ? lot_rec.Cost : lot_rec.Price);
		setCtrlReal(CTL_GBITEM_PRICE, Data.Price);
		setupAmount(0);
		setupPalette(Data.GoodsID);
		if(Data.GoodsID) {
			if(Data.UnitPerPack != 0.0 && Flags & fFocusOnPckg)
				selectCtrl(CTL_GBITEM_QTTYPACK);
			else
				selectCtrl(CTL_GBITEM_VALUE);
		}
		else
			selectCtrl(CTLSEL_GBITEM_GGRP);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = -1, sel = 0;
		double qtty_pack = 0.0;
		PPID   inner_basket_id = 0;
		PPID   inner_suppl_id = 0;
		GoodsCtrlGroup::Rec rec;
		if(Flags & fEnableChangeBasket) {
			getCtrlData(CTLSEL_GBITEM_BASKET, &inner_basket_id);
			getCtrlData(CTLSEL_GBITEM_SUPPL,  &inner_suppl_id);
		}
		ushort v = getCtrlUInt16(CTL_GBITEM_PRIVATE);
		SETFLAG(R_Cart.Pack.Head.Flags, GBASKF_PRIVATE, v);
		THROW(getGroupData(ctlgroupGoods, &rec));
		getCtrlData(CTL_GBITEM_UPPACK,   &Data.UnitPerPack);
		getCtrlData(CTL_GBITEM_QTTYPACK, &qtty_pack);
		getCtrlData((sel = CTL_GBITEM_VALUE), &Data.Quantity);
		if(Data.UnitPerPack > 0.0 && qtty_pack > 0.0 && Data.Quantity == 0.0)
			Data.Quantity = qtty_pack * Data.UnitPerPack;
		THROW_PP(Data.Quantity >= 0.0, PPERR_INVQTTY);
		Data.Rest = Data.Quantity;
		getCtrlData((sel = CTL_GBITEM_PRICE), &Data.Price);
		THROW_PP(Data.Price >= 0.0, PPERR_INVAMOUNT);
		getCtrlData(sel = CTL_GBITEM_EXPIRY, &Data.Expiry);
		THROW_SL(checkdate(Data.Expiry, 1));
		Data.GoodsID = rec.GoodsID;
		ok = 1;
		CATCHZOKPPERRBYDLG
		ASSIGN_PTR(pData, Data);
		return ok;
	}
	int    getLotInfo(PPID goodsID, ReceiptTbl::Rec * pRec)
	{
		memzero(pRec, sizeof(*pRec));
		return goodsID ? ::GetCurGoodsPrice(goodsID, DefLocID, GPRET_INDEF | GPRET_OTHERLOC, 0, pRec) : -1;
	}
private:
	DECL_HANDLE_EVENT;
	int    readQttyFld(uint master, uint ctl, double * pVal)
	{
		double tmp = R6(getCtrlReal(ctl));
		return (tmp == *pVal && master == ctl) ? 0 : ((*pVal = tmp), 1);
	}
	void   setupQuantity(uint master, int readFlds);
	int    setupAmount(double * pAmount, int recalcQttyPack = 1);
	void   setupPalette(PPID goodsID)
	{
		GoodsStockExt gse;
		setCtrlReal(CTL_GBITEM_PALETTE, (goodsID && GObj.GetStockExt(goodsID, &gse, 1) > 0) ? gse.Package : 0);
	}
	PPObjGoodsBasket BaskObj;
	PPObjGoods GObj;
	PPBasketCombine & R_Cart;
	long   Flags;
	PPID   DefLocID; // Склад, используемый для нахождения последнего лота
};

int GBItemDialog::setupAmount(double * pAmount, int recalcQttyPack /*=1*/)
{
	int    ok = 1;
	uint   lines_count = 0;
	PPID   goods_id = 0;
	double cur_sum = 0.0, sum_price = 0.0, sum_brutto = 0.0, sum_volume = 0.0;
	GoodsStockExt gse;
	ILTI * p_item = 0;
	for(uint i = 0; R_Cart.Pack.Lots.enumItems(&i, (void **)&p_item);) {
		cur_sum = p_item->Price * p_item->Quantity;
		sum_price += cur_sum;
		if(GObj.GetStockExt(p_item->GoodsID, &gse, 1) > 0) {
			sum_brutto += gse.CalcBrutto(p_item->Quantity);
			sum_volume += gse.CalcVolume(p_item->Quantity);
		}
	}
	lines_count = R_Cart.Pack.Lots.getCount();
	double qtty_pack = getCtrlReal(CTL_GBITEM_QTTYPACK);
	if(recalcQttyPack || qtty_pack == 0.0)
		setCtrlReal(CTL_GBITEM_QTTYPACK, fdivnz(getCtrlReal(CTL_GBITEM_VALUE), getCtrlReal(CTL_GBITEM_UPPACK)));
	setCtrlData(CTL_GBITEM_AMOUNT,   &sum_price);
	setCtrlData(CTL_GBITEM_LINESCOUNT, &lines_count);
	setCtrlData(CTL_GBITEM_BRUTTO, &sum_brutto);
	setCtrlData(CTL_GBITEM_VOLUME, &sum_volume);
	ASSIGN_PTR(pAmount, sum_price);
	return ok;
}

void GBItemDialog::setupQuantity(uint master, int readFlds)
{
	double numpacks = 0.0;
	if(readFlds && (
		!readQttyFld(master, CTL_GBITEM_UPPACK,   &Data.UnitPerPack) ||
		!readQttyFld(master, CTL_GBITEM_QTTYPACK, &numpacks) ||
		!readQttyFld(master, CTL_GBITEM_VALUE,    &Data.Quantity)))
		return;
	if(Data.UnitPerPack == 0.0)
		numpacks = 0.0;
	else if(oneof3(master, 0, CTL_GBITEM_QTTYPACK, CTL_GBITEM_UPPACK) && numpacks > 0.0)
		Data.Quantity = Data.UnitPerPack * numpacks;
	else if(oneof2(master, 0, CTL_GBITEM_VALUE))
		numpacks = Data.Quantity / Data.UnitPerPack;
	if(master != CTL_GBITEM_UPPACK)
		setCtrlData(CTL_GBITEM_UPPACK,   &Data.UnitPerPack);
	if(master != CTL_GBITEM_QTTYPACK)
		setCtrlData(CTL_GBITEM_QTTYPACK, &numpacks);
	if(master != CTL_GBITEM_VALUE)
		setCtrlData(CTL_GBITEM_VALUE,    &Data.Quantity);
}

IMPL_HANDLE_EVENT(GBItemDialog)
{
	TDialog::handleEvent(event);
	if(event.isKeyDown(kbF2)) {
		if(isCurrCtlID(CTL_GBITEM_VALUE)) {
			const PPID goods_id = getCtrlLong(CTLSEL_GBITEM_GOODS);
			GoodsStockExt gse;
			if(GObj.GetStockExt(goods_id, &gse, 1) > 0 && gse.Package) {
				double ipart;
				double qtty = R3(getCtrlReal(CTL_GBITEM_VALUE));
				double fract = R6(modf(qtty / gse.Package, &ipart));
				setCtrlReal(CTL_GBITEM_VALUE, ((fract == 0.0) ? ipart : (ipart+1.0)) * gse.Package);
				setupQuantity(CTL_GBITEM_VALUE, 1);
			}
		}
		else
			return;
	}
	else if(event.isCmd(cmShowBasket)) {
		getCtrlData(CTLSEL_GBITEM_SUPPL, &R_Cart.Pack.Head.SupplID);
		if(GoodsBasketDialog(R_Cart, 1) > 0)
			setupAmount(0);
	}
	else if(event.isCmd(cmQuot)) {
		const PPID goods_id = getCtrlLong(CTLSEL_GBITEM_GOODS);
		if(goods_id > 0)
			GObj.EditQuotations(goods_id, 0L, -1L/*@curID*/, 0, PPQuot::clsGeneral);
	}
	else if(event.isCmd(cmInputUpdatedByBtn)) {
		const uint ctl = event.getCtlID();
		if(oneof3(ctl, CTL_GBITEM_VALUE, CTL_GBITEM_UPPACK, CTL_GBITEM_QTTYPACK)) {
			setupQuantity(ctl, 1);
			setupAmount(0, 0);
		}
		else if(oneof2(ctl, CTL_GBITEM_GOODS, CTL_GBITEM_PRICE))
			setupAmount(0);
		else
			return;
	}
	else if(event.isCbSelected(CTLSEL_GBITEM_GOODS)) {
		ReceiptTbl::Rec lot_rec;
		const PPID goods_id = getCtrlLong(CTLSEL_GBITEM_GOODS);
		getLotInfo(goods_id, &lot_rec);
		setCtrlReal(CTL_GBITEM_VALUE,  0.0);
		setCtrlReal(CTL_GBITEM_PRICE,  R5(lot_rec.Cost));
		setCtrlReal(CTL_GBITEM_UPPACK, lot_rec.UnitPerPack);
		setupAmount(0);
		setCtrlData(CTL_GBITEM_EXPIRY, &lot_rec.Expiry);
		setupPalette(goods_id);
	}
	else if(event.isCbSelected(CTLSEL_GBITEM_BASKET)) {
		if(Flags & fEnableChangeBasket) {
			PPID   prev_id = R_Cart.Pack.Head.ID;
			PPID   id = getCtrlLong(CTLSEL_GBITEM_BASKET);
			if(id != prev_id) {
				int    err = 0;
				if(PPObjGoodsBasket::IsPrivate(id)) {
					R_Cart.BasketID = id;
					R_Cart.Pack = *DS.GetPrivateBasket();
				}
				else if(!PPObjGoodsBasket::IsLocked(id)) {
					R_Cart.Lck.Lock(id);
					if(BaskObj.GetPacket(id, &R_Cart.Pack) > 0)
						R_Cart.BasketID = id;
					else
						err = 1;
				}
				else
					err = 1;
				if(err) {
					PPError();
					setCtrlLong(CTLSEL_GBITEM_BASKET, prev_id);
				}
				else {
					setupAmount(0);
					setCtrlLong(CTLSEL_GBITEM_SUPPL, R_Cart.Pack.Head.SupplID);
					setCtrlUInt16(CTL_GBITEM_PRIVATE, BIN(R_Cart.Pack.Head.Flags & GBASKF_PRIVATE)); // @v6.4.0
				}
			}
		}
	}
	/*
	else if(event.isCmd(cmPrognosis)) {
		ViewPrognosis();
	}
	*/
	else
		return;
	clearEvent(event);
}

int GoodsBasketItemDialog(ILTI * pData, PPBasketCombine & rCart)
{
	int    ok = -1;
	GBItemDialog * dlg = 0;
	long   basket_dlg_flags = 0;
	UserInterfaceSettings uis;
	if(uis.Restore() > 0 && uis.Flags & UserInterfaceSettings::fBasketItemFocusPckg)
		basket_dlg_flags |= GBItemDialog::fFocusOnPckg;
	if(CheckDialogPtrErr(&(dlg = new GBItemDialog(rCart, 0, basket_dlg_flags))) && dlg->setDTS(pData))
		for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;)
			if(dlg->getDTS(pData) > 0)
				ok = valid_data = 1;
	delete dlg;
	return ok;
}
//
// GBDialog
//
class GBDialog : public PPListDialog {
public:
	GBDialog(PPID * pID, PPBasketCombine & rData, int action) : PPListDialog((action == 3) ? DLG_GBSTRUC_N : DLG_GBSTRUC, CTL_GBTRUC_LIST),
		R_Data(rData), P_EGSDlg(0), P_ID(pID), Flags(0), LastInnerNum(0), InitBasketFlags(0)
	{
		disableCtrl(CTL_GBTRUC_TOTAL, 1);
		disableCtrls(BIN(action == 1), CTLSEL_GBTRUC_BASKET/*, CTLSEL_GBTRUC_SUPPL*/, 0);
		enableCommand(cmAddFromBasket, BIN(action & 0x01));
		if(action == 2)
			ToCascade();
		else if(action == 3)
			Flags |= gbdfEditNameNFlags;
		updateList(-1);
	}
	~GBDialog()
	{
		delete P_EGSDlg;
	}
	int setDTS()
	{
		int    ok = 0;
		if(Flags & gbdfEditNameNFlags) {
			SString buf(R_Data.Pack.Head.Name);
			setCtrlString(CTL_GBTRUC_BASKET, buf);
			buf.Z();
			if(R_Data.Pack.Head.ID)
				buf.Cat(R_Data.Pack.Head.ID);
			setStaticText(CTL_GBTRUC_ID, buf);
			AddClusterAssoc(CTL_GBTRUC_FLAGS, 0, GBASKF_DEFAULT);
			AddClusterAssoc(CTL_GBTRUC_FLAGS, 1, GBASKF_SORTITEMS);
			SetClusterData(CTL_GBTRUC_FLAGS, R_Data.Pack.Head.Flags);
			setCtrlUInt16(CTL_GBTRUC_PRIVATE, BIN(R_Data.Pack.Head.Flags & GBASKF_PRIVATE));
			DisableClusterItem(CTL_GBTRUC_FLAGS, 0, oneof2(R_Data.Pack.Head.ID, PPGDSBSK_ACNUPD, PPGDSBSK_ACNRMV)); // @v10.6.8
			DisableClusterItem(CTL_GBTRUC_PRIVATE, 0, oneof2(R_Data.Pack.Head.ID, PPGDSBSK_ACNUPD, PPGDSBSK_ACNRMV)); // @v10.6.8
		}
		else
			SetupPPObjCombo(this, CTLSEL_GBTRUC_BASKET, PPOBJ_GOODSBASKET, R_Data.Pack.Head.ID, OLW_LOADDEFONOPEN, 0);
		SetupArCombo(this, CTLSEL_GBTRUC_SUPPL, R_Data.Pack.Head.SupplID, OLW_LOADDEFONOPEN, GetSupplAccSheet(), sacfDisableIfZeroSheet);
		updateList(-1);
		LastInnerNum = R_Data.Pack.Lots.getCount();
		InitBasketFlags = R_Data.Pack.Head.Flags;
		return ok;
	}
	int getDTS()
	{
		int    ok = 0;
		if(Flags & gbdfEditNameNFlags) {
			SString  name_buf;
			getCtrlString(CTL_GBTRUC_BASKET, name_buf);
			name_buf.CopyTo(R_Data.Pack.Head.Name, sizeof(R_Data.Pack.Head.Name));
			if(R_Data.Pack.Head.Name[0] != 0) {
				GetClusterData(CTL_GBTRUC_FLAGS, &R_Data.Pack.Head.Flags);
				ushort v = getCtrlUInt16(CTL_GBTRUC_PRIVATE);
				SETFLAG(R_Data.Pack.Head.Flags, GBASKF_PRIVATE, v);
				ok = 1;
			}
			else
				PPSetError(PPERR_NAMENEEDED);
		}
		else {
			getCtrlData(CTLSEL_GBTRUC_BASKET, &R_Data.Pack.Head.ID);
			if(R_Data.Pack.Head.ID != 0)
				ok = 1;
			else
				PPSetError(PPERR_BASKETNEEDED);
		}
		if(ok) {
			R_Data.Pack.Head.SupplID = getCtrlLong(CTLSEL_GBTRUC_SUPPL);
			// @v10.6.8 {
			if(oneof2(R_Data.Pack.Head.ID, PPGDSBSK_ACNUPD, PPGDSBSK_ACNRMV)) {
				R_Data.Pack.Head.Flags &= ~(GBASKF_DEFAULT|GBASKF_PRIVATE);
			}
			// } @v10.6.8 
		}
		return ok;
	}
	int    IsChanged();
private:
	DECL_HANDLE_EVENT
	{
		PPListDialog::handleEvent(event);
		if(event.isCmd(cmClearGB)) {
			if(CONFIRM(PPCFM_DELETE)) {
				R_Data.Pack.Lots.freeAll();
				Flags |= gbdfChanged;
			}
			updateList(-1);
		}
		else if(event.isCmd(cmPrint))
			GbObj.Print(&R_Data.Pack);
		else if(event.isCmd(cmAddFromBasket))
			addFromBasket();
		else if(event.isCbSelected(CTLSEL_GBTRUC_BASKET))
			setupBasket(getCtrlLong(CTLSEL_GBTRUC_BASKET));
		else if(event.isKeyDown(KB_CTRLENTER)) {
			if(IsInState(sfModal)) {
				endModal(cmOK);
				return; // После endModal не следует обращаться к this
			}
		}
		else if(event.isKeyDown(kbF2))
			DoDiscount();
		else if(event.isKeyDown(kbF7))
			GbObj.Print(&R_Data.Pack);
		else if(event.isClusterClk(CTL_GBTRUC_FLAGS) && CheckForSortingItems() > 0)
			updateList(-1);
		else
			return;
		clearEvent(event);
	}
	virtual int  setupList();
	virtual int  addItem(long * pPos, long * pID);
	virtual int  editItem(long pos, long id);
	virtual int  delItem(long pos, long id);
	int    setupBasket(PPID);
	int    addFromBasket();
	int    DoDiscount();
	int    CheckForSortingItems();

	PPObjGoodsBasket GbObj;
	PPBasketCombine & R_Data;
	PPID * P_ID;
	enum {
		gbdfEditNameNFlags = 0x01,
		gbdfChanged        = 0x02
	};
	int16  Flags;          // gbdfXXX
	int16  Reserve;        // @alignment
	uint   LastInnerNum;
	long   InitBasketFlags;
	ExtGoodsSelDialog * P_EGSDlg;
};

int GBDialog::setupBasket(PPID id)
{
	int    ok = 1;
	PPID   prev_id = R_Data.Pack.Head.ID;
	if(id != prev_id) {
		THROW(!PPObjGoodsBasket::IsLocked(id));
		R_Data.Lck.Lock(id);
		THROW(GbObj.GetPacket(id, &R_Data.Pack));
		setCtrlData(CTLSEL_GBTRUC_SUPPL, &R_Data.Pack.Head.SupplID);
		updateList(-1);
	}
	else
		ok = -1;
	CATCH
		R_Data.Lck.Lock(prev_id);
		setCtrlLong(CTLSEL_GBTRUC_BASKET, prev_id);
		ok = PPErrorZ();
	ENDCATCH
	return ok;
}

int GBDialog::setupList()
{
	int    ok = 1;
	ILTI * p_item = 0;
	double cur_sum, sum_price = 0.0, sum_brutto = 0.0, sum_volume = 0.0;
	long   items_count = (long)R_Data.Pack.Lots.getCount();
	SString sub;
	StringSet ss(SLBColumnDelim);
	PPObjGoods goods_obj;
	Goods2Tbl::Rec goods_rec;
	for(uint i = 0; R_Data.Pack.Lots.enumItems(&i, (void **)&p_item);) {
		GoodsStockExt gse;
		goods_obj.Fetch(p_item->GoodsID, &goods_rec);
		ss.clear();
		ss.add(goods_rec.Name);
		ss.add(sub.Z().Cat(p_item->Quantity, MKSFMTD(0, 5, NMBF_NOTRAILZ)));
		ss.add(sub.Z().Cat(p_item->Price, SFMT_MONEY));
		cur_sum = p_item->Price * p_item->Quantity;
		ss.add(sub.Z().Cat(cur_sum, SFMT_MONEY));
		sum_price += cur_sum;
		THROW(addStringToList(i, ss.getBuf()));
		if(goods_obj.GetStockExt(p_item->GoodsID, &gse, 1) > 0) {
			sum_brutto += gse.CalcBrutto(p_item->Quantity);
			sum_volume += gse.CalcVolume(p_item->Quantity);
		}
	}
	setCtrlReal(CTL_GBTRUC_TOTAL, sum_price);
	setCtrlLong(CTL_GBTRUC_LINES, items_count);
	setCtrlReal(CTL_GBTRUC_BRUTTO, sum_brutto);
	setCtrlReal(CTL_GBTRUC_VOLUME, sum_volume);
	CATCHZOK
	return ok;
}

int GBDialog::delItem(long pos, long)
{
	if(pos >= 0) {
		R_Data.Pack.DelItem(static_cast<uint>(pos));
		Flags |= gbdfChanged;
		return 1;
	}
	else
		return -1;
}

int GBDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1, r = 0;
	SString msg_buf;
	long   egsd_flags = ExtGoodsSelDialog::GetDefaultFlags(); // @v10.7.7
	SETIFZ(P_EGSDlg, new ExtGoodsSelDialog(0, 0, egsd_flags));
	if(CheckDialogPtrErr(&P_EGSDlg)) {
		while(ExecView(P_EGSDlg) == cmOK) {
			TIDlgInitData tidi;
			if(P_EGSDlg->getDTS(&tidi)) {
				int    local_ok = -1;
				ILTI   item;
				item.GoodsID = tidi.GoodsID;
				item.Quantity = tidi.Quantity ? tidi.Quantity : 1.0;
				while(local_ok < 0 && GoodsBasketItemDialog(&item, R_Data) > 0) {
					uint pos = 0;
					if(R_Data.Pack.SearchGoodsID(item.GoodsID, &pos)) {
						GetGoodsName(item.GoodsID, msg_buf).Space().CatChar('[').Cat(R_Data.Pack.Lots.at(pos).Quantity, MKSFMTD(0, 6, NMBF_NOTRAILZ)).
							Space().Cat("itm").CatChar(']');
						PPSetAddedMsgString(msg_buf);
						if(PPMessage(mfConf|mfYes|mfNo, PPCFM_SUMDUPGOODSINBASKET) == cmYes) {
							R_Data.Pack.Lots.at(pos).Quantity += item.Quantity;
							ASSIGN_PTR(pPos, static_cast<long>(pos));
							ASSIGN_PTR(pID, (long)(pos+1));
							Flags |= gbdfChanged;
							local_ok = ok = 1;
						}
					}
					else {
						item.BillID = ++LastInnerNum;
						if(R_Data.Pack.AddItem(&item, &pos)) {
							ASSIGN_PTR(pPos, static_cast<long>(pos));
							ASSIGN_PTR(pID, (long)(pos+1));
							Flags |= gbdfChanged;
							local_ok = ok = 1;
						}
						else
							PPError();
					}
				}
				r = 1;
			}
		}
	}
	return ok;
}

int GBDialog::editItem(long pos, long)
{
	if(pos >= 0 && pos < (long)R_Data.Pack.Lots.getCount()) {
		SString goods_name;
		ILTI * p_item = &R_Data.Pack.Lots.at(static_cast<uint>(pos));
		uint   p;
		getCtrlData(CTLSEL_GBTRUC_SUPPL, &R_Data.Pack.Head.SupplID);
		while(GoodsBasketItemDialog(p_item, R_Data) > 0)
			if(R_Data.Pack.SearchGoodsID(p_item->GoodsID, &(p = 0)) &&
				(p != (uint)pos || R_Data.Pack.SearchGoodsID(p_item->GoodsID, &(++p)))) {
				PPError(PPERR_DUPBASKETITEM, GetGoodsName(p_item->GoodsID, goods_name));
			}
			else {
				Flags |= gbdfChanged;
				return 1;
			}
	}
	return -1;
}

int GBDialog::addFromBasket()
{
	int    ok = 1, is_locked = 0;
	PPBasketCombine bc;
	if((ok = GoodsBasketDialog(bc, 2)) > 0 && bc.BasketID && bc.BasketID != *P_ID) {
		ILTI * p_item;
		PPObjGoods goods_obj;
		PPWaitStart();
		for(uint i = 0; bc.Pack.Lots.enumItems(&i, (void **)&p_item);) {
			uint   pos = 0;
			if(R_Data.Pack.SearchGoodsID(p_item->GoodsID, &pos))
				R_Data.Pack.Lots.at(pos).Quantity += p_item->Quantity;
			else {
				p_item->BillID = ++LastInnerNum;
				THROW(R_Data.Pack.AddItem(p_item, &pos));
			}
			PPWaitPercent(i, bc.Pack.Lots.getCount());
		}
		if(bc.Pack.Lots.getCount() > 0)
			Flags |= gbdfChanged;
		updateList(-1);
		PPWaitStop();
		PPSetAddedMsgString(bc.Pack.Head.Name);
		if(GbObj.CheckRights(PPR_DEL) && CONFIRM(PPCFM_REMOVEBASKET))
			THROW(GbObj.RemoveObjV(bc.BasketID, 0, PPObject::use_transaction, 0));
	}
	CATCHZOKPPERR
	return ok;
}

int GBDialog::DoDiscount()
{
	int    ok = 0, valid_data = 0, pctdis = 0;
	double discount = 0.0;
	TDialog * p_dlg = new TDialog(DLG_DISCOUNT);
	if(CheckDialogPtrErr(&p_dlg) > 0) {
		while(!valid_data && ExecView(p_dlg) == cmOK) {
			char   buf[64];
			buf[0] = 0;
			p_dlg->getCtrlData(CTL_DISCOUNT_VALUE, buf);
			strip(buf);
			if(strpbrk(buf, "%/") != 0)
				pctdis = 1;
			strtodoub(buf, &discount);
			if(discount == 0.0) {
				pctdis = 0;
				PPError(PPERR_USERINPUT);
			}
			else
				ok = valid_data = 1;
		}
		if(ok > 0) {
			PPWaitStart();
			ILTI * p_item = 0;
			for(uint i = 0; R_Data.Pack.Lots.enumItems(&i, (void **)&p_item) > 0;) {
				const double price = p_item->Price - (pctdis ? (p_item->Price * fdiv100r(discount)) : discount);
				if(price > 0.0)
					p_item->Price = price;
			}
			if(R_Data.Pack.Lots.getCount() > 0)
				Flags |= gbdfChanged;
			updateList(-1);
			PPWaitStop();
		}
	}
	delete p_dlg;
	return ok;
}

int GBDialog::CheckForSortingItems()
{
	int    ok = -1;
	long   new_flags = 0;
	GetClusterData(CTL_GBTRUC_FLAGS, &new_flags);
	if(!TESTFLAG(R_Data.Pack.Head.Flags, new_flags, GBASKF_SORTITEMS)) {
		if(new_flags & GBASKF_SORTITEMS)
			R_Data.Pack.Lots.sort(PTR_CMPFUNC(ILTIGOODS));
		else
			R_Data.Pack.Lots.sort(PTR_CMPFUNC(ILTI));
		INVERSEFLAG(R_Data.Pack.Head.Flags, GBASKF_SORTITEMS);
		ok = 1;
	}
	return ok;
}

int GBDialog::IsChanged()
{
	int  is_changed = 0;
	if(Flags & gbdfChanged)
		is_changed = 1;
	else if(Flags & gbdfEditNameNFlags) {
		long  new_flags = 0;
		GetClusterData(CTL_GBTRUC_FLAGS, &new_flags);
		const ushort v = getCtrlUInt16(CTL_GBTRUC_PRIVATE);
		SETFLAG(new_flags, GBASKF_PRIVATE, v);
		if(new_flags != InitBasketFlags)
			is_changed = 1;
	}
	return is_changed;
}

int GoodsBasketDialog(PPBasketCombine & rBasket, int action)
{
	int    ok = -1, valid_data = 0, r;
	GBDialog * dlg = 0;
	PPObjGoodsBasket gb_obj;
	THROW(CheckDialogPtr(&(dlg = new GBDialog(&rBasket.BasketID, rBasket, action))));
	dlg->setDTS();
	while(!valid_data && ((r = ExecView(dlg)) == cmOK || (dlg->IsChanged() && !CONFIRM(PPCFM_WARNCANCEL))))
		if(r != cmCancel)
			if(dlg->getDTS()) {
				rBasket.BasketID = rBasket.Pack.Head.ID;
				if(action != 3)
					THROW(gb_obj.PutPacket(&rBasket.BasketID, &rBasket.Pack, 1));
				DS.GetTLA().Lid.BasketID = rBasket.BasketID;
				valid_data = 1;
				ok = dlg->IsChanged() ? 2 : 1;
			}
			else
				PPError();
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int AddGoodsToBasket(PPID goodsID, PPID defLocID, double qtty, double price)
{
	int    ok = -1;
	int    is_locked = 0;
	PPID   gb_id = 0;
	GBItemDialog * dlg = 0;
	if(goodsID) {
		PPObjGoodsBasket gb_obj(reinterpret_cast<void *>(labs(goodsID)));
		PPID   suppl_id = 0;
		ILTI   item;
		long   basket_dlg_flags = GBItemDialog::fEnableChangeBasket;
		PPBasketCombine basket;
		item.GoodsID  = labs(goodsID);
		UserInterfaceSettings uis;
		const int uir_result = BIN(uis.Restore() > 0);
		if(uir_result && uis.Flags & UserInterfaceSettings::fBasketItemFocusPckg)
			basket_dlg_flags |= GBItemDialog::fFocusOnPckg;
		if(qtty > 0.0)
			item.Quantity = qtty;
		else if(uir_result && uis.Flags & UserInterfaceSettings::fAddToBasketItemCurBrwItemAsQtty) {
			double c = 0.0;
			if(TView::messageCommand(APPL->P_DeskTop, cmGetFocusedNumber, &c))
				item.Quantity = (c > 0.0) ? c : 1.0;
		}
		else
			item.Quantity = 1.0;
		if(price > 0.0)
			item.Price = R2(price);
		if(gb_obj.GetPreferredBasket(basket) > 0) {
			THROW(CheckDialogPtr(&(dlg = new GBItemDialog(basket, defLocID, basket_dlg_flags))));
			THROW(dlg->setDTS(&item));
			while(ok < 0 && ExecView(dlg) == cmOK) {
				if(dlg->getDTS(&item)) {
					if(suppl_id)
						basket.Pack.Head.SupplID = suppl_id;
					int    skip = 0;
					int    do_add = 1;
					uint   pos = 0;
					if(basket.Pack.SearchGoodsID(item.GoodsID, &pos)) {
						SString goods_name;
						if(PPMessage(mfConf|mfYes|mfNo, PPCFM_SUMDUPGOODSINBASKET, GetGoodsName(item.GoodsID, goods_name)) == cmYes) {
							basket.Pack.Lots.at(pos).Quantity += item.Quantity;
							do_add = 0;
						}
						else
							skip = 1;
					}
					if(!skip) {
						if(!do_add || basket.Pack.AddItem(&item, 0)) {
							basket.Lck.Unlock(); // @v6.4.0
							if(gb_obj.PutPacket(&basket.Pack.Head.ID, &basket.Pack, 1)) {
								DS.GetTLA().Lid.BasketID = basket.Pack.Head.ID;
								ok = 1;
							}
							else {
								basket.Lck.Lock(basket.Pack.Head.ID);
								PPError();
							}
						}
						else
							PPError();
					}
				}
			}
			if(ok <= 0)
				dlg->getCtrlData(CTLSEL_GBITEM_BASKET, &gb_id);
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}
//
//
//
struct Basket2BillParam {
	Basket2BillParam() : BasketID(0), QuotKindID(0), RuleCost(0), RulePrice(0), Flags(0)
	{
	}
	enum {
		costFromBasket  = 1,  // из корзины
		costFromLastLot = 2   // из последнего лота
	};
	enum {
		priceFromBasket  = 1, // из корзины
		priceFromLastLot = 2, // из последнего лота
		priceFromQuot    = 3  // из котировки вида QuotKindID
	};
	enum {
		fLinkBillExists  = 0x0001, // Информационный флаг от вызывающей функции,
			// показывающий существование связанного документа, из строк которого можно
			// извлекать цены и прочие параметры
		fUseLinkParams   = 0x0002, // Использовать параметры из связанного документа
		fSilentOnDeficit = 0x0004  // При недостатке товара для внесения в документ не выводить
			// об этом сообщение на экран
	};
	PPID   BasketID;
	PPID   QuotKindID;
	long   RuleCost;  // Правило формирования цены поступления в документе (Basket2BillParam::costFromXXX)
	long   RulePrice; // Правило формирования цены реализации в документе (Basket2BillParam::priceFromXXX)
	long   Flags;
};

static int EditBasket2BillParam(Basket2BillParam * pParam)
{
	class Basket2BillDialog : public TDialog {
		DECL_DIALOG_DATA(Basket2BillParam);
	public:
		Basket2BillDialog() : TDialog(DLG_BSKT2BILL)
		{
		}
		DECL_DIALOG_SETDTS()
		{
			RVALUEPTR(Data, pData);
			int    ok = 1;
			AddClusterAssocDef(CTL_BSKT2BILL_RULECOST,  0, Basket2BillParam::costFromBasket);
			AddClusterAssoc(CTL_BSKT2BILL_RULECOST,  1, Basket2BillParam::costFromLastLot);
			SetClusterData(CTL_BSKT2BILL_RULECOST, Data.RuleCost);
			AddClusterAssoc(CTL_BSKT2BILL_RULEPRICE,  0, Basket2BillParam::priceFromBasket);
			AddClusterAssocDef(CTL_BSKT2BILL_RULEPRICE, 1, Basket2BillParam::priceFromLastLot);
			AddClusterAssoc(CTL_BSKT2BILL_RULEPRICE,  2, Basket2BillParam::priceFromQuot);
			SetClusterData(CTL_BSKT2BILL_RULEPRICE, Data.RulePrice);
			SetupPPObjCombo(this, CTLSEL_BSKT2BILL_QK, PPOBJ_QUOTKIND, Data.QuotKindID, OLW_LOADDEFONOPEN, reinterpret_cast<void *>(1));
			disableCtrl(CTLSEL_BSKT2BILL_QK, Data.RulePrice != Basket2BillParam::priceFromQuot);
			AddClusterAssoc(CTL_BSKT2BILL_USELINK, 0, Basket2BillParam::fUseLinkParams);
			AddClusterAssoc(CTL_BSKT2BILL_USELINK, 1, Basket2BillParam::fSilentOnDeficit);
			SetClusterData(CTL_BSKT2BILL_USELINK, Data.Flags);
			DisableClusterItem(CTL_BSKT2BILL_USELINK, 0, (Data.Flags & Basket2BillParam::fLinkBillExists) ? 0 : 1);
			return ok;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			GetClusterData(CTL_BSKT2BILL_RULECOST,  &Data.RuleCost);
			getCtrlData(CTLSEL_BSKT2BILL_QK, &Data.QuotKindID);
			GetClusterData(CTL_BSKT2BILL_RULEPRICE, &Data.RulePrice);
			if(Data.RulePrice == Basket2BillParam::priceFromQuot) {
				sel = CTL_BSKT2BILL_QK;
				THROW_PP(Data.QuotKindID, PPERR_QUOTKINDNEEDED);
			}
			GetClusterData(CTL_BSKT2BILL_USELINK, &Data.Flags);
			ASSIGN_PTR(pData, Data);
			CATCHZOKPPERRBYDLG
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(TVCOMMAND && event.isClusterClk(CTL_BSKT2BILL_RULEPRICE)) {
				disableCtrl(CTLSEL_BSKT2BILL_QK, getCtrlUInt16(CTL_BSKT2BILL_RULEPRICE) != 2);
				clearEvent(event);
			}
		}
	};
	DIALOG_PROC_BODY(Basket2BillDialog, pParam);
}

int PPObjBill::ConvertBasket(const PPBasketPacket & rBasket, PPBillPacket * pPack)
{
	int    ok = 1;
	int    is_expend = -1;
	PPLogger logger;
	SString msg_buf;
	SString temp_buf;
	BillTbl::Rec link_rec;
	LongArray all_rows, one_goods_rows;
	Basket2BillParam param;
	// @v10.8.0 @ctr MEMSZERO(param);
	param.Flags |= Basket2BillParam::fSilentOnDeficit;
	if(pPack->Rec.LinkBillID && Search(pPack->Rec.LinkBillID, &link_rec) > 0 && IsDraftOp(link_rec.OpID)) {
		param.Flags |= Basket2BillParam::fLinkBillExists;
		param.Flags |= Basket2BillParam::fUseLinkParams;
	}
	PPID   op_type_id = GetOpType(pPack->Rec.OpID);
	if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
		param.RuleCost  = Basket2BillParam::costFromBasket;
		param.RulePrice = Basket2BillParam::priceFromLastLot;
		is_expend = 0;
	}
	else if(oneof3(op_type_id, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER)) {
		param.RuleCost  = Basket2BillParam::costFromLastLot;
		param.RulePrice = Basket2BillParam::priceFromBasket;
		is_expend = (oneof2(op_type_id, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER)) ? 2 : 1;
	}
	else if(op_type_id == PPOPT_GOODSACK) {
		param.RuleCost  = Basket2BillParam::costFromLastLot;
		param.RulePrice = Basket2BillParam::priceFromBasket;
		is_expend = 0;
	}
	else if(op_type_id == PPOPT_GOODSMODIF) {
		TDialog * p_sel_modif_dlg = 0;
		THROW(CheckDialogPtr(&(p_sel_modif_dlg = new TDialog(DLG_SELMODIF))));
		// @v11.2.2 {
		if(!isempty(rBasket.Head.Name)) {
			p_sel_modif_dlg->setStaticText(CTL_SELMODIF_ST_INFO, rBasket.Head.Name);
		}
		// } @v11.2.2 
		p_sel_modif_dlg->setCtrlUInt16(CTL_SELMODIF_WHAT, 0);
		p_sel_modif_dlg->DisableClusterItem(CTL_SELMODIF_WHAT, 2, 1);
		p_sel_modif_dlg->DisableClusterItem(CTL_SELMODIF_FLAGS, 0, 1); // @v11.2.4
		if(ExecView(p_sel_modif_dlg) == cmOK) {
			ushort v = p_sel_modif_dlg->getCtrlUInt16(CTL_SELMODIF_WHAT);
			if(v == 0) {
				param.RuleCost  = Basket2BillParam::costFromLastLot;
				param.RulePrice = Basket2BillParam::priceFromBasket;
				is_expend = 1;
			}
			else if(v == 1) {
				param.RuleCost  = Basket2BillParam::costFromBasket;
				param.RulePrice = Basket2BillParam::priceFromLastLot;
				is_expend = 0;
			}
			else if(v == 2)
				ok = 0;
		}
		ZDELETE(p_sel_modif_dlg);
	}
	else
		ok = -1;
	if(ok > 0 && EditBasket2BillParam(&param) > 0) {
		int    rollback = 0, use_link_bill = 0;
		ILTI * p_item;
		PPBillPacket link_pack;
		if(param.Flags & Basket2BillParam::fUseLinkParams) {
			if(ExtractPacket(pPack->Rec.LinkBillID, &link_pack) > 0)
				use_link_bill = 1;
		}
		for(uint i = 0; !rollback && rBasket.Lots.enumItems(&i, (void **)&p_item);) {
			int    r = 0;
			uint   cvt_ilti_flags = CILTIF_ABSQTTY;
			double last_price = 0.0;
			Goods2Tbl::Rec goods_rec;
			ReceiptTbl::Rec lot_rec;
			ILTI   ilti = *p_item;
			PPTransferItem * p_link_ti = 0;
			if(GObj.Fetch(p_item->GoodsID, &goods_rec) <= 0) {
				logger.LogLastError();
				continue;
			}
			if(!pPack->CheckGoodsForRestrictions((int)(i-1), p_item->GoodsID, TISIGN_UNDEF, ilti.Quantity, PPBillPacket::cgrfAll, 0)) {
				logger.LogLastError();
				continue;
			}
			if(use_link_bill) {
				uint   pos = 0;
				if(link_pack.SearchGoods(p_item->GoodsID, &pos))
					p_link_ti = &link_pack.TI(pos);
			}
			if(trfr->Rcpt.GetCurrentGoodsPrice(p_item->GoodsID, pPack->Rec.LocID, GPRET_MOSTRECENT, &last_price, &lot_rec) == GPRET_ERROR) {
				logger.LogLastError();
				continue;
			}
			ilti.QCert = 0;
			ilti.Cost  = 0.0;
			ilti.Price = 0.0;
			if(p_link_ti)
				ilti.Cost = R5(p_link_ti->Cost); // @v10.8.0 R2-->R5
			else if(param.RuleCost == Basket2BillParam::costFromBasket)
				ilti.Cost = R5(p_item->Price); // @v10.8.0 R2-->R5
			else if(param.RuleCost == Basket2BillParam::costFromLastLot)
				ilti.Cost = R5(lot_rec.Cost);

			if(p_link_ti)
				ilti.Price = R5(p_link_ti->Price);
			else if(param.RulePrice == Basket2BillParam::priceFromBasket)
				ilti.Price = R5(p_item->Price);
			else if(param.RulePrice == Basket2BillParam::priceFromLastLot)
				ilti.Price = R5(lot_rec.Price);
			else if(param.RulePrice == Basket2BillParam::priceFromQuot) {
				double price = 0.0;
				if(GObj.GetQuotExt(p_item->GoodsID, QuotIdent(pPack->Rec.LocID, param.QuotKindID, pPack->Rec.CurID, pPack->Rec.Object), ilti.Cost, last_price, &price, 1) > 0) {
					cvt_ilti_flags |= CILTIF_QUOT;
					if(pPack->Rec.CurID)
						ilti.CurPrice = price;
					else
						ilti.Price = price;
				}
				else
					ilti.Price = last_price;
			}
			one_goods_rows.freeAll();
			if(is_expend > 0) {
				ilti.Rest = -p_item->Rest;
				if(is_expend == 2)
					cvt_ilti_flags |= CILTIF_ZERODSCNT;
			}
			else if(is_expend == 0) {
				if(op_type_id == PPOPT_GOODSRECEIPT)
					ilti.Flags |= PPTFR_RECEIPT;
			}
			if(!ConvertILTI(&ilti, pPack, &one_goods_rows, cvt_ilti_flags, 0)) {
				logger.LogLastError();
				continue;
			}
			else if(ilti.HasDeficit()) {
				PPLoadString("deficit", msg_buf);
				msg_buf.CatDiv(':', 2);
				GObj.GetSingleBarcode(goods_rec.ID, temp_buf.Z());
				if(temp_buf.NotEmptyS())
					msg_buf.Cat(temp_buf).CatDiv('-', 1);
				msg_buf.Cat(goods_rec.Name).CatDiv('=', 1).Cat(ilti.Rest, MKSFMTD(0, 6, 0));
				logger.Log(msg_buf);
			}
			if(param.Flags & Basket2BillParam::fSilentOnDeficit)
				r = -1;
			else
				r = (fabs(ilti.Rest) > 0.0) ? ProcessUnsuffisientGoods(ilti.GoodsID, pugpNoBalance) : -1;
			if(oneof2(r, PCUG_ASGOODAS, -1)) {
				int * p_i;
				for(uint j = 0; one_goods_rows.enumItems(&j, (void **)&p_i);) {
					PPTransferItem & r_ti = pPack->TI(static_cast<uint>(*p_i));
					if(op_type_id == PPOPT_GOODSRECEIPT && r_ti.Flags & PPTFR_RECEIPT) {
						r_ti.QCert = lot_rec.QCertID;
						r_ti.Expiry = p_item->Expiry;
						if(lot_rec.ID) {
							//
							// Наследуем теги (имеющие признак OTF_INHERITABLE) от последнего лота.
							//
							ObjTagList inh_tag_list;
							GetTagListByLot(lot_rec.ID, 1, &inh_tag_list);
							const uint tc = inh_tag_list.GetCount();
							if(tc) {
								PPObjTag tag_obj;
								PPObjectTag tag_rec;
								ObjTagList new_lot_tag_list;
								for(uint i_ = 0; i_ < tc; i_++) {
									const ObjTagItem * p_tag = inh_tag_list.GetItemByPos(i_);
									if(p_tag && tag_obj.Fetch(p_tag->TagID, &tag_rec) > 0 && tag_rec.Flags & OTF_INHERITABLE)
										new_lot_tag_list.PutItem(p_tag->TagID, p_tag);
								}
								if(new_lot_tag_list.GetCount())
									pPack->LTagL.Set((uint)*p_i, &new_lot_tag_list);
							}
						}
					}
					THROW_SL(all_rows.insert(p_i));
				}
			}
			else if(r == PCUG_EXCLUDE)
				pPack->RemoveRows(&one_goods_rows);
			else {
				rollback = 1;
				pPack->RemoveRows(&one_goods_rows);
				pPack->RemoveRows(&all_rows);
			}
		}
		ok = (rollback == 0) ? 1 : -1;
	}
	CATCH
		ok = 0;
		pPack->RemoveRows(&one_goods_rows);
		pPack->RemoveRows(&all_rows);
		logger.LogLastError();
	ENDCATCH
	return ok;
}
