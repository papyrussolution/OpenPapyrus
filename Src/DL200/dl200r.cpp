// DL200R.CPP
// Copyright (c) A.Sobolev 2008, 2009, 2012, 2015, 2016, 2018
//
#include <pp.h>
#pragma hdrstop
//
//
//
int SLAPI DL2_ObjList::Get(int32 id, PPID * pObjType, ObjIdListFilt & rList)
{
	int    ok = -1;
	PPObjLocation * p_loc_obj = 0;
	PPObjGoodsGroup * p_gg_obj = 0;
	rList.Set(0);
	ASSIGN_PTR(pObjType, 0);
	for(uint i = 0; ok < 0 && i < getCount(); i++) {
		const Item * p_item = (Item *)at(i);
		if(p_item->Id == id) {
			PPIDArray list, rlist;
			LocationTbl::Rec loc_rec;
			SString code;
			if(p_item->ObjType == PPOBJ_LOCATION) {
				SETIFZ(p_loc_obj, new PPObjLocation);
				THROW_MEM(p_loc_obj);
			}
			else if(p_item->ObjType == PPOBJ_GOODSGROUP) {
				SETIFZ(p_gg_obj, new PPObjGoodsGroup);
				THROW_MEM(p_gg_obj);
			}
			for(uint p = 0; p_item->Ss.get(&p, code);) {
				if(p_item->ObjType == PPOBJ_LOCATION) {
					PPID   loc_id = 0;
					if(p_loc_obj->P_Tbl->SearchCode(LOCTYP_WAREHOUSE, code, &loc_id, &loc_rec) > 0) {
						list.addUnique(loc_id);
					}
					else if(p_loc_obj->P_Tbl->SearchCode(LOCTYP_WAREHOUSEGROUP, code, &loc_id, &loc_rec) > 0) {
						list.addUnique(loc_id);
					}
					else {
						; // @error
					}
				}
				else if(p_item->ObjType == PPOBJ_GOODSGROUP) {
					PPID  grp_id = 0;
					BarcodeTbl::Rec bc_rec;
					if(p_gg_obj->SearchCode(code, &bc_rec) > 0) {
						list.addUnique(bc_rec.GoodsID);
					}
					else {
						; // @error
					}
				}
			}
			if(p_item->ObjType == PPOBJ_LOCATION) {
				THROW(p_loc_obj->ResolveWarehouseList(&list, rlist));
				rList.Set(&rlist);
			}
			else
				rList.Set(&list);
			ASSIGN_PTR(pObjType, p_item->ObjType);
			ok = 1;
		}
	}
	CATCHZOK
	delete p_loc_obj;
	delete p_gg_obj;
	return ok;
}
