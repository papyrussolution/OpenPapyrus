// D_CHARRY.CPP
// Copyright (c) A.Sobolev, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
#include <ppds.h>
#include <charry.h>

SLAPI PPDS_CrrAddress::PPDS_CrrAddress() : PPDeclStruc()
{
	MEMSZERO(Data);
}

int SLAPI PPDS_CrrAddress::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		CityName = 0;
		CountryName = 0;
		Address = 0;
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			WorldTbl::Rec w_rec;
			Data = *(LocationTbl::Rec *)dataPtr;
			if(WObj.Fetch(Data.CityID, &w_rec) > 0) {
				CityName = w_rec.Name;
				if(WObj.GetCountryByChild(w_rec.ID, &w_rec))
					CountryName = w_rec.Name;
			}
			LocationCore::GetExField(&Data, LOCEXSTR_SHORTADDR, Address);
		}
	}
	else if(op == idoAccept) {
		if(CityName.Empty() || WObj.AddSimple(&Data.CityID, WORLDOBJ_CITY, CityName, CountryName, 1)) {
			LocationCore::SetExField(&Data, LOCEXSTR_SHORTADDR, Address.Strip());
			ok = (Address.NotEmpty() || Data.CityID) ? 1 : -1;
		}
		else
			ok = 0;
	}
	else
		ok = PPSetError(PPERR_INVPARAM);
	return ok;
}

int SLAPI PPDS_CrrAddress::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRADDRESS_CODE:    ok = TransferData(Data.Code, sizeof(Data.Code), dir, rBuf); break;
		case DSF_CRRADDRESS_COUNTRY: ok = TransferData(CountryName, dir, rBuf); break;
		case DSF_CRRADDRESS_CITY:    ok = TransferData(CityName, dir, rBuf); break;
		case DSF_CRRADDRESS_ADDRESS: ok = TransferData(Address, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrBnkAcct::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(IntrData);
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			PPObjPerson psn_obj;
			// @v9.0.4 Data = *(BankAccountTbl::Rec *)dataPtr;
			Data = *(PPBankAccount *)dataPtr; // @v9.0.4
			IntrData.InitFlags = BADIF_INITALLBR;
			psn_obj.GetBnkAcctData(0, &Data, &IntrData);
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPObjPerson psn_obj;
		PPID   bnk_id = 0;
		MEMSZERO(Data);
		if(psn_obj.AddBankSimple(&bnk_id, &IntrData.Bnk, 1) > 0 && IntrData.Acct[0]) {
			Data.BankID = IntrData.Bnk.ID = bnk_id;
			Data.AccType = PPBAC_CURRENT;
			STRNSCPY(Data.Acct, IntrData.Acct);
			ok = 1;
		}
		else
			ok = -1;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrBnkAcct::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRBNKACCT_ACC:
			ok = TransferData(IntrData.Acct, sizeof(IntrData.Acct), dir, rBuf);
			break;
		case DSF_CRRBNKACCT_BNKNAME:
			ok = TransferData(IntrData.Bnk.Name, sizeof(IntrData.Bnk.Name), dir, rBuf);
			break;
		case DSF_CRRBNKACCT_BNKFULLNAME:
			ok = TransferData(IntrData.Bnk.ExtName, sizeof(IntrData.Bnk.ExtName), dir, rBuf);
			break;
		case DSF_CRRBNKACCT_BNKCODE:
			ok = TransferData(IntrData.Bnk.BIC, sizeof(IntrData.Bnk.BIC), dir, rBuf);
			break;
		case DSF_CRRBNKACCT_BNKCORRACC:
			ok = TransferData(IntrData.Bnk.CorrAcc, sizeof(IntrData.Bnk.CorrAcc), dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_ELinkAddr::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int ok = 1;
	if(op == idoAlloc) {
		KindID = 0;
		KindName[0] = 0;
		Addr[0] = 0;
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPELink *)dataPtr;
			KindID = Data.KindID;
			GetObjectName(PPOBJ_ELINKKIND, Data.KindID, KindName, sizeof(KindName));
			STRNSCPY(Addr, Data.Addr);
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPObjELinkKind elk_obj;
		if(elk_obj.AddSimple(&KindID, KindName, 1)) {
			Data.KindID = KindID;
			STRNSCPY(Data.Addr, Addr);
		}
		else
			ok = 0;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_ELinkAddr::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int  ok = -1;
	switch(fldID) {
		case DSF_ELINKADDR_KINDID:   ok = TransferData(&KindID, dir, rBuf); break;
		case DSF_ELINKADDR_KINDNAME: ok = TransferData(KindName, sizeof(KindName), dir, rBuf); break;
		case DSF_ELINKADDR_ADDR:     ok = TransferData(Addr, sizeof(Addr), dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrPerson::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc) {
		Data.destroy();
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPPersonPacket *)dataPtr;
		}
		else if(addedParam) {
			PPObjPerson psn_obj;
			if(psn_obj.GetPacket(addedParam, &Data, 0) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		/*
		PPObjPerson psn_obj;
		PPID id = 0;
		if(psn_obj.SearchMaxLike(&Data, &id, 0, PPREGT_TPID) < 0 &&
			psn_obj.SearchMaxLike(&Data, &id, 0, PPREGT_ALBATROSCLID) < 0) {
			if(!psn_obj.PutPacket(&id, &Data, 1))
				ok = 0;
		}
		else if(psn_obj.GetPacket(id, &Data, 0) <= 0)
			ok = 0;
		*/
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrPerson::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	SString temp_buf;
	switch(fldID) {
		case DSF_CRRPERSON_ID:
			{
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
				if(dir == tfdBufToData)
					OrigID = id;
			}
			break;
		case DSF_CRRPERSON_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_CRRPERSON_FULLNAME:
			if(dir == tfdDataToBuf)
				Data.GetExtName(temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				Data.SetExtName(temp_buf.Strip());
			break;
		case DSF_CRRPERSON_INN:
			if(dir == tfdDataToBuf)
				Data.GetRegNumber(PPREGT_TPID, temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				Data.AddRegister(PPREGT_TPID, temp_buf, 0);
			break;
		case DSF_CRRPERSON_SYSCODE:
			if(dir == tfdDataToBuf)
				Data.GetRegNumber(PPREGT_ALBATROSCLID, temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				Data.AddRegister(PPREGT_ALBATROSCLID, temp_buf, 0);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrPerson::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRPERSON_ADDR) {
			Data.Loc = ((PPDS_CrrAddress *)pData)->Data;
			ok = 1;
		}
		else if(fldID == DSF_CRRPERSON_RADDR) {
			Data.RLoc = ((PPDS_CrrAddress *)pData)->Data;
			ok = 1;
		}
		else if(fldID == DSF_CRRPERSON_ELINK) {
			PPELink el_rec = ((PPDS_ELinkAddr *)pData)->Data;
			Data.ELA.AddItem(el_rec.KindID, el_rec.Addr);
			ok = 1;
		}
		else if(fldID == DSF_CRRPERSON_BNKACC) {
			PPDS_CrrBnkAcct * p_data = (PPDS_CrrBnkAcct *)pData;
			// @v9.0.4 ok = Data.BAA.insert(&p_data->Data) ? 1 : PPSetErrorSLib();
			ok = Data.Regs.SetBankAccount(&p_data->Data, (uint)-1); // @v9.0.4
		}
	}
	return ok;
}

int SLAPI PPDS_CrrPerson::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRPERSON_ADDR) {
		if(*pIter == 0) {
			if(!Data.Loc.IsEmptyAddress()) {
				pData->InitData(idoExtract, &Data.Loc, 0);
				ok = 1;
			}
		}
	}
	else if(fldID == DSF_CRRPERSON_RADDR) {
		if(*pIter == 0) {
			if(!Data.RLoc.IsEmptyAddress()) {
				pData->InitData(idoExtract, &Data.RLoc, 0);
				ok = 1;
			}
		}
	}
	else if(fldID == DSF_CRRPERSON_ELINK) {
		if(*pIter < Data.ELA.getCount()) {
			pData->InitData(idoExtract, &Data.ELA.at(*pIter), 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRPERSON_BNKACC) {
		if(*pIter < Data.Regs.getCount()) {
			const RegisterTbl::Rec & r_reg_rec = Data.Regs.at(*pIter);
			if(r_reg_rec.RegTypeID == PPREGT_BANKACCOUNT) {
				PPBankAccount ba(r_reg_rec);
				pData->InitData(idoExtract, &ba, 0);
			}
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_Barcode::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(BarcodeTbl::Rec *)dataPtr;
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_Barcode::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_BARCODE_CODE: ok = TransferData(Data.Code, sizeof(Data.Code), dir, rBuf); break;
		case DSF_BARCODE_QTTY: ok = TransferData(&Data.Qtty, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrGoods::ExtractOuterData()
{
	GetObjectName(PPOBJ_UNIT, Data.Rec.UnitID, UnitName, sizeof(UnitName));
	GetObjectName(PPOBJ_UNIT, Data.Rec.PhUnitID, PhUnitName, sizeof(PhUnitName)); // @v6.9.3
	PhUPerU = Data.Rec.PhUPerU;                                                   // @v6.9.3
	GetObjectName(PPOBJ_GOODSGROUP, Data.Rec.ParentID, GroupName, sizeof(GroupName));
	GetObjectName(PPOBJ_PERSON, Data.Rec.ManufID, ManufName, sizeof(ManufName));
	PPCountryBlock country_blk;
	GObj.GetManufCountry(Data.Rec.ID, &Data.Rec, 0, &country_blk);
	country_blk.Name.CopyTo(ManufCountry, sizeof(ManufCountry));
	return 1;
}

int SLAPI PPDS_CrrGoods::AcceptOuterData(int use_ta)
{
	int    ok = 1;
	PPObjUnit u_obj;
	PPUnit u_rec;
	PPObjGoodsGroup gg_obj;
	PPID   id = 0;
	Goods2Tbl::Rec goods_rec;
	PPGoodsConfig goods_cfg;

	Data.Rec.Kind = PPGDSK_GOODS;
	STRNSCPY(Data.Rec.Abbr, Data.Rec.Name);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		GObj.ReadConfig(&goods_cfg);
		if(u_obj.SearchByName(UnitName, &id) > 0)
			Data.Rec.UnitID = id;
		else if(goods_cfg.DefUnitID)
			Data.Rec.UnitID = goods_cfg.DefUnitID;
		else {
			MEMSZERO(u_rec);
			STRNSCPY(u_rec.Name, UnitName);
			u_rec.Flags |= PPUnit::Trade;
			THROW(u_obj.ref->AddItem(PPOBJ_UNIT, &(id = 0), &u_rec, 0));
			Data.Rec.UnitID = id;
		}
		if(PhUnitName[0]) {
			if(PhUPerU > 0.0) {
				id = 0;
				if(u_obj.SearchByName(PhUnitName, &id) > 0)
					Data.Rec.PhUnitID = id;
				else {
					MEMSZERO(u_rec);
					STRNSCPY(u_rec.Name, PhUnitName);
					u_rec.Flags |= PPUnit::Phisical;
					THROW(u_obj.ref->AddItem(PPOBJ_UNIT, &(id = 0), &u_rec, 0));
					Data.Rec.PhUnitID = id;
				}
				Data.Rec.PhUPerU = PhUPerU;
			}
		}
		id = 0;
		if(gg_obj.SearchByName(GroupName, &id, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GROUP)
			Data.Rec.ParentID = id;
		else {
			THROW(gg_obj.AddSimple(&id, gpkndOrdinaryGroup, 0, GroupName, 0, 0, 0));
			Data.Rec.ParentID = id;
		}
		id = 0;
		if(ManufName[0] || ManufCountry[0]) {
			PPObjPerson psn_obj;
			if(stricmp866(ManufName, ManufCountry) == 0) {
				THROW(psn_obj.AddSimple(&id, ManufCountry, PPPRK_MANUF, PPPRS_COUNTRY, 0));
			}
			else if(ManufName[0]) {
				THROW(psn_obj.AddSimple(&id, ManufCountry, PPPRK_MANUF, PPPRS_LEGAL, 0));
			}
			Data.Rec.ManufID = id;
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrGoods::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc) {
		Data.destroy();
		UnitName[0] = 0;
		PhUnitName[0] = 0;
		PhUPerU = 0.0;
		GroupName[0] = 0;
		ManufName[0] = 0;
		ManufCountry[0] = 0;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPGoodsPacket *)dataPtr;
			ExtractOuterData();
		}
		else if(addedParam) {
			if(GObj.GetPacket(addedParam, &Data, 0) > 0) {
				ExtractOuterData();
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		AcceptOuterData(1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrGoods::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRGOODS_ID:
			{
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
				if(dir == tfdBufToData)
					OrigID = id;
			}
			break;
		case DSF_CRRGOODS_NAME: ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf); break;
		case DSF_CRRGOODS_UNITNAME: ok = TransferData(UnitName, sizeof(UnitName), dir, rBuf); break;
		case DSF_CRRGOODS_PHUNITNAME: ok = TransferData(PhUnitName, sizeof(PhUnitName), dir, rBuf); break;
		case DSF_CRRGOODS_PHUPERU: ok = TransferData(&PhUPerU, dir, rBuf); break;
		case DSF_CRRGOODS_GROUPNAME: ok = TransferData(GroupName, sizeof(GroupName), dir, rBuf); break;
		case DSF_CRRGOODS_MANUFNAME: ok = TransferData(ManufName, sizeof(ManufName), dir, rBuf); break;
		case DSF_CRRGOODS_MANUFCOUNTRY: ok = TransferData(ManufCountry, sizeof(ManufCountry), dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrGoods::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRGOODS_CODE) {
			BarcodeTbl::Rec bc_rec = ((PPDS_Barcode *)pData)->Data;
			Data.AddCode(bc_rec.Code, 0, bc_rec.Qtty);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrGoods::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRGOODS_CODE) {
		if(*pIter < Data.Codes.getCount()) {
			pData->InitData(idoExtract, &Data.Codes.at(*pIter), 0);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrQCert::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
		RegOrgName[0] = 0;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(QualityCertTbl::Rec *)dataPtr;
			GetObjectName(PPOBJ_PERSON, Data.RegOrgan, RegOrgName, sizeof(RegOrgName));
		}
		if(addedParam) {
			if(QcObj.Search(addedParam, &Data) > 0) {
				GetObjectName(PPOBJ_PERSON, Data.RegOrgan, RegOrgName, sizeof(RegOrgName));
			}
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(*strip(RegOrgName)) {
			PPID id = 0;
			PPObjPerson psn_obj;
			if(psn_obj.AddSimple(&id, RegOrgName, PPPRK_BUSADMIN, PPPRS_LEGAL, 1))
				Data.RegOrgan = id;
			else
				ok = 0;
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrQCert::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRQCERT_CODE: ok = TransferData(Data.Code, sizeof(Data.Code), dir, rBuf); break;
		case DSF_CRRQCERT_BLANKCODE: ok = TransferData(Data.BlankCode, sizeof(Data.BlankCode), dir, rBuf); break;
		case DSF_CRRQCERT_INITDATE: ok = TransferData(&Data.InitDate, dir, rBuf); break;
		case DSF_CRRQCERT_EXPIRY: ok = TransferData(&Data.Expiry, dir, rBuf); break;
		case DSF_CRRQCERT_ORGNAME: ok = TransferData(RegOrgName, sizeof(RegOrgName), dir, rBuf); break;
		case DSF_CRRQCERT_ETC: ok = TransferData(Data.Etc, sizeof(Data.Etc), dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
SLAPI PPDS_CrrBillItem::PPDS_CrrBillItem() : PPDeclStruc()
{
}

int SLAPI PPDS_CrrBillItem::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
		Data.Init(0);
		CLB[0] = 0;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPTransferItem *)dataPtr;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrBillItem::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRBILLITEM_EXPIRY: ok = TransferData(&Data.Expiry, dir, rBuf); break;
		case DSF_CRRBILLITEM_CLB: ok = TransferData(CLB, sizeof(CLB), dir, rBuf); break;
		case DSF_CRRBILLITEM_UNITSPERPACK: ok = TransferData(&Data.UnitPerPack, dir, rBuf); break;
		case DSF_CRRBILLITEM_QTTY: ok = TransferData(&Data.Quantity_, dir, rBuf); break;
		case DSF_CRRBILLITEM_PRICE:
			{
				double price = 0.0;
				if(dir == tfdDataToBuf)
					price = Data.Price-Data.Discount;
				ok = TransferData(&price, dir, rBuf);
				if(dir == tfdBufToData)
					Data.Price = price;
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrBillItem::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	PPID   id = 0;
	if(pData) {
		if(fldID == DSF_CRRBILLITEM_GOODS) {
			PPObjGoods goods_obj;
			SString ar_code;
			PPID   ar_id = 0;
			PPGoodsPacket * p_data = &((PPDS_CrrGoods *)pData)->Data;
			const PPID orig_goods_id = ((PPDS_CrrGoods *)pData)->OrigID;
			if(P_Outer && P_Outer->GetId() == PPDS_CRRBILL) {
				const PPDS_CrrBill * p_outer = (const PPDS_CrrBill *)P_Outer;
				if(p_outer->DbxCfg.Flags & DBDXF_CHARRY_GIDASARCODE && p_outer->SellerArID && orig_goods_id) {
					ar_id = p_outer->SellerArID;
					ar_code.Cat(orig_goods_id);
					Goods2Tbl::Rec goods_rec;
					if(goods_obj.P_Tbl->SearchByArCode(ar_id, ar_code, 0, &goods_rec) > 0) {
						Data.GoodsID = goods_rec.ID;
						ok = 1;
					}
				}
			}
			if(ok < 0) {
				PPTransaction tra(1);
				THROW(tra);
				if(goods_obj.SearchMaxLike(p_data, &id) > 0) {
					if(ar_code.NotEmpty()) {
						THROW(goods_obj.P_Tbl->SetArCode(id, ar_id, ar_code, 0));
					}
					Data.GoodsID = id;
				}
				else {
					goods_obj.RemoveDupBarcodes(p_data, pCtx);
					if(ar_code.NotEmpty()) {
						ArGoodsCodeTbl::Rec ar_code_rec;
						MEMSZERO(ar_code_rec);
						ar_code_rec.ArID = ar_id;
						ar_code.CopyTo(ar_code_rec.Code, sizeof(ar_code_rec.Code));
						p_data->ArCodes.insert(&ar_code_rec);
					}
					THROW(goods_obj.PutPacket(&(id = 0), p_data, 0));
					Data.GoodsID = id;
				}
				THROW(tra.Commit());
				ok = 1;
			}
		}
		else if(fldID == DSF_CRRBILLITEM_QCERT) {
			PPObjQCert qc_obj;
			QualityCertTbl::Rec qc_rec = ((PPDS_CrrQCert *)pData)->Data;
			if(qc_obj.SearchByCode(qc_rec.Code, &id, 0) > 0) {
				Data.QCert = id;
			}
			else {
				qc_rec.ID = 0;
				THROW(qc_obj.PutPacket(&(id = 0), &qc_rec, 1));
				Data.QCert = id;
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBillItem::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRBILLITEM_GOODS) {
		if(*pIter == 0) {
			pData->InitData(idoExtract, 0, Data.GoodsID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRBILLITEM_QCERT) {
		if(*pIter == 0) {
			pData->InitData(idoExtract, 0, Data.QCert);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
SLAPI PPDS_CrrBill::PPDS_CrrBill() : PPDeclStruc()
{
}

int SLAPI PPDS_CrrBill::IdentifySuppl(PPID * pArID, int use_ta)
{
	int    ok = 1, r;
	PPID   op_id = CConfig.ReceiptOp;
	PPID   suppl_id = 0;
	PPID   ar_id = 0;
	PPOprKind op_rec;
	ArticleTbl::Rec ar_rec;
	THROW_PP(op_id, PPERR_UNDEFRECEIPTOP);
	THROW(GetOpData(op_id, &op_rec) > 0);
	{
		PPTransaction tra(use_ta);
		THROW(tra);
		Seller.Rec.Status = PPPRS_LEGAL;
		Seller.Kinds.addUnique(PPPRK_SUPPL);
		if(PsnObj.SearchMaxLike(&Seller, &suppl_id, 0, PPREGT_TPID) < 0 &&
			PsnObj.SearchMaxLike(&Seller, &suppl_id, PPObjPerson::smlRegisterOnly, PPREGT_ALBATROSCLID) < 0) {
			THROW(PsnObj.PutPacket(&suppl_id, &Seller, 0));
		}
		THROW(r = ArObj.P_Tbl->SearchObjRef(op_rec.AccSheetID, suppl_id, &ar_rec));
		if(r < 0)
			THROW(ArObj.CreateObjRef(&ar_rec.ID, op_rec.AccSheetID, suppl_id, 0, 0));
		THROW(tra.Commit());
		ar_id = ar_rec.ID;
	}
	CATCH
		ar_id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int SLAPI PPDS_CrrBill::InitData(Ido op, void * /*dataPtr*/, long addedParam)
{
	int    ok = 1;
	PPObjBill * p_bobj = BillObj;
	if(op == idoAlloc) {
		Data.destroy();
		SellerArID = 0;
		MEMSZERO(DlvrAddr);
		THROW(PPObjectTransmit::ReadConfig(&DbxCfg));
	}
	else if(op == idoExtract) {
		if(addedParam) {
			if(p_bobj->ExtractPacket(addedParam, &Data) > 0) {
				MEMSZERO(DlvrAddr);
				if(Data.P_Freight && Data.P_Freight->DlvrAddrID) {
					PPObjLocation loc_obj;
					if(loc_obj.Search(Data.P_Freight->DlvrAddrID, &DlvrAddr) > 0) {
					}
					else
						MEMSZERO(DlvrAddr);
				}
				p_bobj->LoadClbList(&Data, 1);
				Data.InitExtTIter(ETIEF_UNITEBYGOODS|ETIEF_DIFFBYQCERT|ETIEF_DIFFBYPACK|ETIEF_DIFFBYNETPRICE, 0, TiIter::ordByGoods);
				PPTransferItem * p_ti;
				for(uint i = 0; Data.EnumTItems(&i, &p_ti) > 0;)
					if(p_ti->Flags & PPTFR_PRICEWOTAXES) {
						GTaxVect vect;
						vect.CalcTI(p_ti, Data.Rec.OpID, TIAMT_PRICE);
						p_ti->Price = R2(vect.GetValue(GTAXVF_BEFORETAXES) / fabs(p_ti->Quantity_));
						p_ti->Discount = 0.0;
					}
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID   op_id = CConfig.ReceiptOp;
		SString clb;
		PPBillPacket pack;
		PPOprKind op_rec;
		PPTransferItem * p_ti;
		PPObjLocation loc_obj;
		THROW_PP(op_id, PPERR_UNDEFRECEIPTOP);
		THROW(GetOpData(op_id, &op_rec) > 0);
		{
			PPTransaction tra(1);
			THROW(tra);
			THROW(pack.CreateBlank(op_id, 0, 0, 0));
			{
				//
				// Проверяем соответствие между главной организацией и покупателем,
				// установленным в документе по номеру ИНН.
				//
				PPID   main_org_id = 0;
				SString main_org_inn, input_inn;
				GetMainOrgID(&main_org_id);
				PsnObj.GetRegNumber(main_org_id, PPREGT_TPID, main_org_inn);
				Buyer.GetRegNumber(PPREGT_TPID, input_inn);
				THROW_PP_S(input_inn.Len() && input_inn.CmpNC(main_org_inn) == 0, PPERR_UNEQBUYERTPID, input_inn);
				THROW(IdentifySuppl(&pack.Rec.Object, 0));
			}
			//
			// Если адрес доставки в документе соответствует коду одного из складов,
			// то считаем, что документ должен попасть на этот склад.
			//
			if(DlvrAddr.Code[0]) {
				PPID   loc_id = 0;
				LocationTbl::Rec loc_rec;
				if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, DlvrAddr.Code, &loc_id, &loc_rec) > 0) {
					pack.Rec.LocID = loc_id;
				}
			}
			pack.Rec.Dt = Data.Rec.Dt;
			STRNSCPY(pack.Rec.Code, Data.Rec.Code);
			STRNSCPY(pack.Rec.Memo, Data.Rec.Memo);
			pack.Pays.copy(Data.Pays);
			if(p_bobj->P_Tbl->SearchAnalog(&pack.Rec, 0, 0) <= 0) {
				int    skip = 0;
				for(uint i = 0; Data.EnumTItems(&i, &p_ti) > 0;) {
					if(pack.CheckGoodsForRestrictions((int)(i-1), p_ti->GoodsID, p_ti->GetSign(pack.Rec.OpID), p_ti->Qtty(), PPBillPacket::cgrfAll, 0)) {
						IntArray poslist;
						ReceiptTbl::Rec lot_rec;
						PPTransferItem ti = *p_ti;
						ti.Init(&pack.Rec);
						ti.Quantity_ = fabs(ti.Quantity_);
						ti.Cost = ti.Price;
						if(!(DbxCfg.Flags & DBDXF_CHARRY_PRICEQCOST))
							if(p_bobj->trfr->Rcpt.GetLastLot(ti.GoodsID, -labs(pack.Rec.LocID), pack.Rec.Dt, &lot_rec) > 0) // @v7.9.11 0-->-pack.Rec.LocID
								ti.Price = R5(lot_rec.Price);
							else {
								double price = 0.0;
								if(pack.GetQuotExt(ti, &price) > 0)
									ti.Price = price;
							}
						THROW(pack.InsertRow(&ti, &poslist));
						if(Data.ClbL.GetNumber(i-1, &clb) > 0)
							pack.ClbL.AddNumber(&poslist, clb);
					}
					else {
						CALLPTRMEMB(P_Logger, LogLastError());
						skip = 1;
					}
				}
				if(!skip) {
					THROW(p_bobj->__TurnPacket(&pack, 0, 0, 0));
					CALLPTRMEMB(P_Logger, LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0));
				}
			}
			THROW(tra.Commit());
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBill::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRBILL_ID:   ok = TransferData(&Data.Rec.ID, dir, rBuf); break;
		case DSF_CRRBILL_DATE: ok = TransferData(&Data.Rec.Dt, dir, rBuf); break;
		case DSF_CRRBILL_CODE: ok = TransferData(Data.Rec.Code, sizeof(Data.Rec.Code), dir, rBuf); break;
		case DSF_CRRBILL_MEMO: ok = TransferData(Data.Rec.Memo, sizeof(Data.Rec.Memo), dir, rBuf); break;
		case DSF_CRRBILL_PAYMENTDATE:
			{
				LDATE dt;
				if(dir == tfdDataToBuf)
					Data.GetLastPayDate(&dt);
				ok = TransferData(&dt, dir, rBuf);
				if(dir == tfdBufToData)
					Data.AddPayDate(dt, 0);
			}
			break;
		case DSF_CRRBILL_AMOUNT:
			{
				double amount = BR2(Data.Rec.Amount);
				ok = TransferData(&amount, dir, rBuf);
				if(dir == tfdBufToData)
					Data.Rec.Amount = BR2(amount);
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrBill::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRBILL_SELLER) {
			Seller = ((PPDS_CrrPerson *)pData)->Data;
			THROW(IdentifySuppl(&SellerArID, 1));
			ok = 1;
		}
		else if(fldID == DSF_CRRBILL_BUYER) {
			Buyer = ((PPDS_CrrPerson *)pData)->Data;
			ok = 1;
		}
		if(fldID == DSF_CRRBILL_DLVRADDR) {
			DlvrAddr = ((PPDS_CrrAddress *)pData)->Data;
			ok = 1;
		}
		else if(fldID == DSF_CRRBILL_ITEMS) {
			char   clb[32];
			IntArray poslist;
			PPTransferItem item = ((PPDS_CrrBillItem *)pData)->Data;
			STRNSCPY(clb, ((PPDS_CrrBillItem *)pData)->CLB);
			THROW(Data.InsertRow(&item, &poslist));
			for(uint i = 0; i < poslist.getCount(); i++)
				Data.ClbL.AddNumber(poslist.at(i), clb);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBill::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	PPID   id = 0;
	if(fldID == DSF_CRRBILL_SELLER) {
		if(*pIter == 0) {
			if(GetMainOrgID(&id) > 0) {
				pData->InitData(idoExtract, 0, id);
   				ok = 1;
			}
		}
	}
	else if(fldID == DSF_CRRBILL_BUYER) {
		if(*pIter == 0) {
			id = ObjectToPerson(Data.Rec.Object);
			if(id) {
				pData->InitData(idoExtract, 0, id);
				ok = 1;
			}
		}
	}
	if(fldID == DSF_CRRBILL_DLVRADDR) {
		if(*pIter == 0) {
			if(!LocationCore::IsEmptyAddressRec(DlvrAddr)) {
				pData->InitData(idoExtract, &DlvrAddr, 0);
				ok = 1;
			}
		}
	}
	else if(fldID == DSF_CRRBILL_ITEMS) {
		if(*pIter < Data.GetTCount()) {
			PPTransferItem ti;
			PPBillPacket::TiItemExt tiie;
			if(Data.EnumTItemsExt(0, &ti, &tiie) > 0) {
				pData->InitData(idoExtract, &ti, 0);
				STRNSCPY(((PPDS_CrrBillItem *)pData)->CLB, tiie.Clb);
				ok = 1;
			}
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrAmountType::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = -1;
	if(op == idoAlloc) {
		MEMSZERO(Pack.Rec);
		Pack.Formula = 0;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Pack = *(PPAmountTypePacket *)dataPtr;
			ok = 1;
		}
		else if(addedParam) {
			if(AmtObj.GetPacket(addedParam, &Pack) > 0)
				ok = 1;
		}
	}
	else if(op == idoAccept) {
		PPID   id = 0;
		if(*strip(Pack.Rec.Symb) != 0 && AmtObj.SearchSymb(&id, Pack.Rec.Symb) > 0) {
			Pack.Rec.ID = id;
			if(UpdateProtocol == updForce) {
				PPAmountTypePacket ex_pack;
				THROW(AmtObj.GetPacket(id, &ex_pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRAMOUNTTYPE_ID:
							break;
						case DSF_CRRAMOUNTTYPE_NAME:
							STRNSCPY(ex_pack.Rec.Name, Pack.Rec.Name);
							break;
						case DSF_CRRAMOUNTTYPE_SYMB:
							STRNSCPY(ex_pack.Rec.Symb, Pack.Rec.Symb);
							break;
						case DSF_CRRAMOUNTTYPE_TAX:            ex_pack.Rec.Tax = Pack.Rec.Tax; break;
						case DSF_CRRAMOUNTTYPE_TAXRATE:        ex_pack.Rec.TaxRate = Pack.Rec.TaxRate; break;
						case DSF_CRRAMOUNTTYPE_REFAMTTYPESYMB: ex_pack.Rec.RefAmtTypeID = Pack.Rec.RefAmtTypeID; break;
						case DSF_CRRAMOUNTTYPE_FERRONDEFAULT:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fErrOnDefault, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FMANUAL:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fManual, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FTAX:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fTax, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FREPLACECOST:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fReplaceCost, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FREPLACEPRICE:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fReplacePrice, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FREPLACEDISCOUNT:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fReplaceDiscount, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FINAMOUNT:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fInAmount, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FOUTAMOUNT:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fOutAmount, Pack.Rec.Flags); break;
						case DSF_CRRAMOUNTTYPE_FSTAFFAMOUNT:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, PPAmountType::fStaffAmount, Pack.Rec.Flags); break;
					}
				}
				THROW(AmtObj.PutPacket(&id, &ex_pack, 1));
				ok = 1;
			}
		}
		else {
			Pack.Rec.ID = id = 0;
			THROW(AmtObj.PutPacket(&id, &Pack, 1));
			Pack.Rec.ID = id;
			ok = 1;
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrAmountType::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRAMOUNTTYPE_ID:   ok = TransferData(&Pack.Rec.ID, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_NAME: ok = TransferData(Pack.Rec.Name, sizeof(Pack.Rec.Name), dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_SYMB: ok = TransferData(Pack.Rec.Symb, sizeof(Pack.Rec.Symb), dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_TAX:
			{
				const char * p_list = "VAT;EXCISE;STAX";
				if(dir == tfdDataToBuf) {
					if(Pack.Rec.Flags & PPAmountType::fTax)
						TempBuf.GetSubFrom(p_list, ';', Pack.Rec.Tax-1);
					else
						TempBuf = 0;
				}
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Pack.Rec.Tax = idx+1;
					else
						Pack.Rec.Tax = 0;
				}
			}
			break;
		case DSF_CRRAMOUNTTYPE_TAXRATE:
			{
				double rate = 0.0;
				if(dir == tfdDataToBuf)
					if(Pack.Rec.Flags & PPAmountType::fTax)
						rate = fdiv100i(Pack.Rec.TaxRate);
				ok = TransferData(&rate, dir, rBuf);
				if(dir == tfdBufToData) {
					if(Pack.Rec.Flags & PPAmountType::fTax)
						Pack.Rec.TaxRate = R0i(rate * 100.0);
				}
			}
			break;
		case DSF_CRRAMOUNTTYPE_REFAMTTYPESYMB:
			{
				PPAmountType ref_rec;
				TempBuf = 0;
				if(dir == tfdDataToBuf) {
					if(Pack.Rec.Flags & (PPAmountType::fInAmount|PPAmountType::fOutAmount))
						if(AmtObj.Fetch(Pack.Rec.RefAmtTypeID, &ref_rec) > 0)
							TempBuf = ref_rec.Symb;
				}
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					if(Pack.Rec.Flags & (PPAmountType::fInAmount|PPAmountType::fOutAmount)) {
						PPID ref_id = 0;
						if(AmtObj.SearchSymb(&ref_id, TempBuf) > 0)
							Pack.Rec.RefAmtTypeID = ref_id;
					}
				}
			}
			break;
		case DSF_CRRAMOUNTTYPE_FERRONDEFAULT:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fErrOnDefault, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FMANUAL:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fManual, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FTAX:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fTax, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FREPLACECOST:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fReplaceCost, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FREPLACEPRICE:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fReplacePrice, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FREPLACEDISCOUNT:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fReplaceDiscount, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FINAMOUNT:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fInAmount, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FOUTAMOUNT:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fOutAmount, dir, rBuf); break;
		case DSF_CRRAMOUNTTYPE_FSTAFFAMOUNT:
			ok = TransferDataFlag(&Pack.Rec.Flags, PPAmountType::fStaffAmount, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// PPDS_CrrSalCharge
//
int SLAPI PPDS_CrrSalCharge::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = -1;
	if(op == idoAlloc) {
		Data.Init();
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPSalChargePacket *)dataPtr;
			ok = 1;
		}
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Symb) != 0 && Obj.SearchBySymb(Data.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				PPSalChargePacket ex_pack;
				THROW(Obj.GetPacket(id, &ex_pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRSALCHARGE_FORMULA:
							ex_pack.Formula = Data.Formula;
							break;
					}
				}
				THROW(Obj.PutPacket(&id, &ex_pack, 1));
				ok = 1;
			}
		}
		else {
			Data.Rec.ID = id = 0;
			THROW(Obj.PutPacket(&id, &Data, 1));
			ok = 1;
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrSalCharge::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRSALCHARGE_ID:   ok = TransferData(&Data.Rec.ID, dir, rBuf); break;
		case DSF_CRRSALCHARGE_NAME: ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf); break;
		case DSF_CRRSALCHARGE_SYMB: ok = TransferData(Data.Rec.Symb, sizeof(Data.Rec.Symb), dir, rBuf); break;
		case DSF_CRRSALCHARGE_FORMULA: ok = TransferData(Data.Formula, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// PPDS_CrrSalChargeGroup
//
int SLAPI PPDS_CrrSalChargeGroup::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = -1;
	if(op == idoAlloc) {
		Data.Init();
		Data.Rec.Flags |= PPSalCharge::fGroup;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPSalChargePacket *)dataPtr;
			ok = 1;
		}
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Symb) != 0 && Obj.SearchBySymb(Data.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				PPSalChargePacket ex_pack;
				THROW(Obj.GetPacket(id, &ex_pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRSALCHARGEGROUP_ITEMSYMBLIST:
							ex_pack.GrpList = Data.GrpList;
							break;
					}
				}
				THROW(Obj.PutPacket(&id, &ex_pack, 1));
				ok = 1;
			}
		}
		else {
			Data.Rec.ID = id = 0;
			Data.Rec.Flags |= PPSalCharge::fGroup;
			THROW(Obj.PutPacket(&id, &Data, 1));
			ok = 1;
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrSalChargeGroup::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRSALCHARGEGROUP_ID:   ok = TransferData(&Data.Rec.ID, dir, rBuf); break;
		case DSF_CRRSALCHARGEGROUP_NAME: ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf); break;
		case DSF_CRRSALCHARGEGROUP_SYMB: ok = TransferData(Data.Rec.Symb, sizeof(Data.Rec.Symb), dir, rBuf); break;
		case DSF_CRRSALCHARGEGROUP_ITEMSYMBLIST:
			{
				TempBuf = 0;
				if(dir == tfdDataToBuf) {
					if((*pIter) < Data.GrpList.getCount()) {
						PPSalCharge item_rec;
						if(Obj.Search(Data.GrpList.get(*pIter), &item_rec) > 0)
							TempBuf = item_rec.Symb;
						else
							TempBuf = 0;
						ok = TransferData(TempBuf, dir, rBuf);
					}
					else
						ok = -1;
				}
				else if(dir == tfdBufToData) {
					ok = TransferData(TempBuf, dir, rBuf);
					if(TempBuf.NotEmptyS()) {
						PPID id = 0;
						if(Obj.SearchBySymb(TempBuf, &id) > 0)
							Data.GrpList.addUnique(id);
					}
				}
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// PPDS_CrrStaffCalEntry
//
int SLAPI PPDS_CrrStaffCalEntry::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = -1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
		Duration = ZEROTIME;
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(StaffCalendarTbl::Rec *)dataPtr;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(!Data.TmEnd) {
			if(Duration) {
				Data.TmEnd.settotalsec(Data.TmStart.totalsec() + Duration.totalsec());
			}
		}
		else
			Duration.settotalsec(Data.TmEnd.totalsec() - Data.TmStart.totalsec());
		Data.TmVal = Duration.totalsec();
		ok = 1;
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrStaffCalEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	CALDATE cdt;
	int    kind = 0;
	switch(fldID) {
		case DSF_CRRSTAFFCALENTRY_DATE:
			cdt.v = Data.DtVal;
			kind = cdt.GetKind();
			if((dir == tfdDataToBuf && kind == CALDATE::kDate) || dir == tfdBufToData) {
				ok = TransferData((LDATE *)&Data.DtVal, dir, rBuf);
			}
			break;
		case DSF_CRRSTAFFCALENTRY_CALDATE:
			cdt.v = Data.DtVal;
			kind = cdt.GetKind();
			if((dir == tfdDataToBuf && kind == CALDATE::kCalDate) || dir == tfdBufToData) {
				LDATE dt;
				if(dir == tfdDataToBuf) {
					dt.v = Data.DtVal;
					(TempBuf = 0).Cat(dt.day()).CatChar('/').Cat(dt.month());
				}
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					strtodate(TempBuf, DATF_DMY, &dt);
					cdt.SetCalDate(dt.day(), dt.month());
					Data.DtVal = cdt.v;
				}
			}
			break;
		case DSF_CRRSTAFFCALENTRY_DAYOFWEEK:
			cdt.v = Data.DtVal;
			kind = cdt.GetKind();
			if((dir == tfdDataToBuf && kind == CALDATE::kDayOfWeek) || dir == tfdBufToData) {
				if(dir == tfdDataToBuf)
					GetDayOfWeekText(dowtEnFull, Data.DtVal, TempBuf);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					cdt.SetDayOfWeek(GetDayOfWeekByText(TempBuf));
					Data.DtVal = cdt.v;
				}
			}
			break;
		case DSF_CRRSTAFFCALENTRY_STARTTIME: ok = TransferData(&Data.TmStart, dir, rBuf); break;
		case DSF_CRRSTAFFCALENTRY_ENDTIME:   ok = TransferData(&Data.TmEnd,   dir, rBuf); break;
		case DSF_CRRSTAFFCALENTRY_DURATION:  ok = TransferData(&Duration,     dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// PPDS_CrrStaffCal
//
int SLAPI PPDS_CrrStaffCal::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = -1;
	if(op == idoAlloc) {
		Data.Init(0);
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPStaffCalPacket *)dataPtr;
			ok = 1;
		}
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Symb) != 0 && Obj.SearchBySymb(Data.Rec.Symb, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				PPStaffCalPacket ex_pack;
				THROW(Obj.GetPacket(id, &ex_pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRSTAFFCAL_ENTRIES:
							ex_pack.Items = Data.Items;
							break;
					}
				}
				THROW(Obj.PutPacket(&id, &ex_pack, 1));
				ok = 1;
			}
		}
		else {
			Data.Rec.ID = id = 0;
			THROW(Obj.PutPacket(&id, &Data, 1));
			ok = 1;
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrStaffCal::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRSTAFFCAL_ID:   ok = TransferData(&Data.Rec.ID, dir, rBuf); break;
		case DSF_CRRSTAFFCAL_NAME: ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf); break;
		case DSF_CRRSTAFFCAL_SYMB: ok = TransferData(Data.Rec.Symb, sizeof(Data.Rec.Symb), dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrStaffCal::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRSTAFFCAL_ENTRIES) {
			PPDS_CrrStaffCalEntry * p_data = (PPDS_CrrStaffCalEntry *)pData;
			ok = Data.Items.insert(&p_data->Data) ? 1 : PPSetErrorSLib();
		}
	}
	return ok;
}

int SLAPI PPDS_CrrStaffCal::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRSTAFFCAL_ENTRIES) {
		if(*pIter < Data.Items.getCount()) {
			pData->InitData(idoExtract, &Data.Items.at(*pIter), 0);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrDbDiv::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = -1;
	if(op == idoAlloc) {
		Data.Init();
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(DBDivPack *)dataPtr;
			ok = 1;
		}
		else if(addedParam) {
			if(Obj.Get(addedParam, &Data) > 0)
				ok = 1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Name) != 0 && Obj.SearchByName(Data.Rec.Name, &id, 0) > 0 && id == Data.Rec.ID) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				DBDivPack ex_pack;
				THROW(Obj.Get(id, &ex_pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRDBDIV_ADDR:
							STRNSCPY(ex_pack.Rec.Addr, Data.Rec.Addr);
							break;
						case DSF_CRRDBDIV_FDISPATCH:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, DBDIVF_DISPATCH, Data.Rec.Flags);
							break;
						case DSF_CRRDBDIV_FSCARDONLY:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, DBDIVF_SCARDSONLY, Data.Rec.Flags);
							break;
						case DSF_CRRDBDIV_FRCVCSESSANDWROFFBILLS:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, DBDIVF_RCVCSESSANDWROFFBILLS, Data.Rec.Flags);
							break;
						case DSF_CRRDBDIV_FCONSOLID:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, DBDIVF_CONSOLID, Data.Rec.Flags);
							break;
						case DSF_CRRDBDIV_FPASSIVE:
							SETFLAGBYSAMPLE(ex_pack.Rec.Flags, DBDIVF_PASSIVE, Data.Rec.Flags);
							break;
						case DSF_CRRDBDIV_INTRRCPTOPSYMB:
							if(Data.Rec.IntrRcptOpr)
								ex_pack.Rec.IntrRcptOpr = Data.Rec.IntrRcptOpr;
							break;
						case DSF_CRRDBDIV_LOCSYMBLIST:
							ex_pack.LocList = Data.LocList;
							break;
					}
				}
				THROW(Obj.Put(&id, &ex_pack, 1));
				ok = 1;
			}
		}
		else {
			// Принимаем пакет с тем же идентификатором, с которым он пришел
			// Это отличает объект PPObjDBDiv от большинства остальных типов объектов данных.
			THROW(Obj.Put(&id, &Data, 1));
			ok = 1;
		}
	}
	else
		CALLEXCEPT_PP(PPERR_INVPARAM);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrDbDiv::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRDBDIV_ID: ok = TransferData(&Data.Rec.ID, dir, rBuf); break;
		case DSF_CRRDBDIV_NAME: ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf); break;
		case DSF_CRRDBDIV_ADDR: ok = TransferData(Data.Rec.Addr, sizeof(Data.Rec.Addr), dir, rBuf); break;
		case DSF_CRRDBDIV_FDISPATCH:
			ok = TransferDataFlag(&Data.Rec.Flags, DBDIVF_DISPATCH, dir, rBuf);
			break;
		case DSF_CRRDBDIV_FSCARDONLY:
			ok = TransferDataFlag(&Data.Rec.Flags, DBDIVF_SCARDSONLY, dir, rBuf);
			break;
		case DSF_CRRDBDIV_FRCVCSESSANDWROFFBILLS:
			ok = TransferDataFlag(&Data.Rec.Flags, DBDIVF_RCVCSESSANDWROFFBILLS, dir, rBuf);
			break;
		case DSF_CRRDBDIV_FCONSOLID:
			ok = TransferDataFlag(&Data.Rec.Flags, DBDIVF_CONSOLID, dir, rBuf);
			break;
		case DSF_CRRDBDIV_FPASSIVE:
			ok = TransferDataFlag(&Data.Rec.Flags, DBDIVF_PASSIVE, dir, rBuf);
			break;
		case DSF_CRRDBDIV_INTRRCPTOPSYMB:
			{
				TempBuf = 0;
				if(dir == tfdDataToBuf) {
					if(Data.Rec.IntrRcptOpr)
						GetOpName(Data.Rec.IntrRcptOpr, TempBuf);
				}
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					if(TempBuf.NotEmptyS()) {
						PPObjOprKind op_obj;
						PPOprKind op_rec;
						PPID op_id = 0;
						if(op_obj.SearchByName(TempBuf, &op_id, &op_rec) > 0)
							Data.Rec.IntrRcptOpr = op_id;
					}
				}
			}
			break;
		case DSF_CRRDBDIV_LOCSYMBLIST:
			{
				TempBuf = 0;
				if(dir == tfdDataToBuf) {
					if((*pIter) < Data.LocList.getCount()) {
						LocationTbl::Rec loc_rec;
						if(Obj.Search(Data.LocList.get(*pIter), &loc_rec) > 0) {
							TempBuf = loc_rec.Code;
							if(TempBuf.Strip().Empty())
								TempBuf = loc_rec.Name;
						}
						else
							TempBuf = 0;
						ok = TransferData(TempBuf, dir, rBuf);
					}
					else
						ok = -1;
				}
				else if(dir == tfdBufToData) {
					ok = TransferData(TempBuf, dir, rBuf);
					if(TempBuf.NotEmptyS()) {
						PPID id = 0;
						LocationTbl::Rec loc_rec;
						if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, TempBuf, &id, &loc_rec) > 0)
							Data.LocList.addUnique(id);
					}
				}
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrBarcodeStruc::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPBarcodeStruc*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_BCODESTRUC;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPBarcodeStruc rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRBARCODESTRUC_ID:
							break;
						case DSF_CRRBARCODESTRUC_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRBARCODESTRUC_TEMPL:
							STRNSCPY(rec.Templ, Data.Templ);
							break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBarcodeStruc::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRBARCODESTRUC_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRBARCODESTRUC_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRBARCODESTRUC_TEMPL:
			ok = TransferData(Data.Templ, sizeof(Data.Templ), dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrGoodsType::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPGoodsType*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_GOODSTYPE;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPGoodsType rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRGOODSTYPE_ID:
							break;
						case DSF_CRRGOODSTYPE_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRGOODSTYPE_WROFFGRPID:
							rec.WrOffGrpID = Data.WrOffGrpID;
							break;
						case DSF_CRRGOODSTYPE_AMTCVAT:
							rec.AmtCVat = Data.AmtCVat;
							break;
						case DSF_CRRGOODSTYPE_AMTCOST:
							rec.AmtCost = Data.AmtCost;
							break;
						case DSF_CRRGOODSTYPE_AMTPRICE:
							rec.AmtPrice = Data.AmtPrice;
							break;
						case DSF_CRRGOODSTYPE_AMTDSCNT:
							rec.AmtDscnt = Data.AmtDscnt;
							break;
						case DSF_CRRGOODSTYPE_FUNLIMITED:
							SETFLAGBYSAMPLE(rec.Flags, GTF_UNLIMITED, Data.Flags);
						case DSF_CRRGOODSTYPE_FRPLCCOST:
							SETFLAGBYSAMPLE(rec.Flags, GTF_RPLC_COST, Data.Flags);
						case DSF_CRRGOODSTYPE_FRPLCPRICE:
							SETFLAGBYSAMPLE(rec.Flags, GTF_RPLC_PRICE, Data.Flags);
						case DSF_CRRGOODSTYPE_FRPLCDSCNT:
							SETFLAGBYSAMPLE(rec.Flags, GTF_RPLC_DSCNT, Data.Flags);
						case DSF_CRRGOODSTYPE_FPRICEINCLDIS:
							SETFLAGBYSAMPLE(rec.Flags, GTF_PRICEINCLDIS, Data.Flags);
						case DSF_CRRGOODSTYPE_FEXCLAMOUNT:
							SETFLAGBYSAMPLE(rec.Flags, GTF_EXCLAMOUNT, Data.Flags);
						case DSF_CRRGOODSTYPE_FAUTOCOMPL:
							SETFLAGBYSAMPLE(rec.Flags, GTF_AUTOCOMPL, Data.Flags);
						case DSF_CRRGOODSTYPE_FALLOWZEROPRICE:
							SETFLAGBYSAMPLE(rec.Flags, GTF_ALLOWZEROPRICE, Data.Flags);
						case DSF_CRRGOODSTYPE_FASSETS:
							SETFLAGBYSAMPLE(rec.Flags, GTF_ASSETS, Data.Flags);

					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrGoodsType::TransferAmtType(PPID * pAmttID, Tfd dir, SString & rBuf)
{
	int ok = -1;
	SString buf;
	PPAmountType amtt_rec;
	MEMSZERO(amtt_rec);
	if(dir == tfdDataToBuf) {
		if(AmtTypeObj.Search(*pAmttID, &amtt_rec) > 0)
			buf.CopyFrom(amtt_rec.Symb);
	}
	ok = TransferData(buf, dir, rBuf);
	if(dir == tfdBufToData)
		ok = AmtTypeObj.SearchSymb(pAmttID, buf);
	return ok;
}

int SLAPI PPDS_CrrGoodsType::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRGOODSTYPE_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRGOODSTYPE_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRGOODSTYPE_WROFFGRPID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPAssetWrOffGrp wroffgrp_rec;
					MEMSZERO(wroffgrp_rec);
					if(AsstWrOffObj.Search(Data.WrOffGrpID, &wroffgrp_rec) > 0)
						buf.CopyFrom(wroffgrp_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = AsstWrOffObj.SearchByName(buf, &Data.WrOffGrpID);
			}
			break;
		case DSF_CRRGOODSTYPE_AMTCVAT:
			ok = TransferAmtType(&Data.AmtCVat, dir, rBuf);
			break;
		case DSF_CRRGOODSTYPE_AMTCOST:
			ok = TransferAmtType(&Data.AmtCost, dir, rBuf);
			break;
		case DSF_CRRGOODSTYPE_AMTPRICE:
			ok = TransferAmtType(&Data.AmtPrice, dir, rBuf);
			break;
		case DSF_CRRGOODSTYPE_AMTDSCNT:
			ok = TransferAmtType(&Data.AmtDscnt, dir, rBuf);
			break;
		case DSF_CRRGOODSTYPE_FUNLIMITED:
			ok = TransferDataFlag(&Data.Flags, GTF_UNLIMITED, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FRPLCCOST:
			ok = TransferDataFlag(&Data.Flags, GTF_RPLC_COST, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FRPLCPRICE:
			ok = TransferDataFlag(&Data.Flags, GTF_RPLC_PRICE, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FRPLCDSCNT:
			ok = TransferDataFlag(&Data.Flags, GTF_RPLC_DSCNT, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FPRICEINCLDIS:
			ok = TransferDataFlag(&Data.Flags, GTF_PRICEINCLDIS, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FEXCLAMOUNT:
			ok = TransferDataFlag(&Data.Flags, GTF_EXCLAMOUNT, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FAUTOCOMPL:
			ok = TransferDataFlag(&Data.Flags, GTF_AUTOCOMPL, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FALLOWZEROPRICE:
			ok = TransferDataFlag(&Data.Flags, GTF_ALLOWZEROPRICE, dir, rBuf); break;
		case DSF_CRRGOODSTYPE_FASSETS:
			ok = TransferDataFlag(&Data.Flags, GTF_ASSETS, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrFormula::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		Data.ID   = 0;
		Data.Name = 0;
		Data.Expr = 0;
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(Formula*)dataPtr;
		else if(addedParam) {
			if(Obj.Get(addedParam, Data.Name, Data.Expr) > 0) {
				Data.ID = addedParam;
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(Data.Name.Strip() != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				Formula rec;
				MEMSZERO(rec);
				THROW(Obj.Get(id, rec.Name, rec.Expr) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRFORMULA_ID:
							break;
						case DSF_CRRFORMULA_NAME:
							rec.Name = Data.Name;
							break;
						case DSF_CRRFORMULA_EXPR:
							rec.Expr = Data.Expr;
							break;
					}
				}
				ok = Obj.Put(&id, rec.Name, rec.Expr, 0);
			}
		}
		else
			ok = Obj.Put(&id, Data.Name, Data.Expr, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrFormula::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRFORMULA_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRFORMULA_NAME:
			ok = TransferData(Data.Name, dir, rBuf);
			break;
		case DSF_CRRFORMULA_EXPR:
			ok = TransferData(Data.Expr, dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrPersonKind::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPPersonKind*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Name)) {
			PPID id = 0;
			Data.Tag = PPOBJ_PRSNKIND;
			if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
				Data.ID = id;
				if(UpdateProtocol == updForce) {
					PPPersonKind rec;
					MEMSZERO(rec);
					THROW(Obj.Search(id, &rec) > 0);
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRPERSONKIND_ID:
								break;
							case DSF_CRRPERSONKIND_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRPERSONKIND_CODEREGTYPEID:
								rec.CodeRegTypeID = Data.CodeRegTypeID;
								break;
							case DSF_CRRPERSONKIND_FOLDERREGTYPEID:
								rec.FolderRegTypeID = Data.FolderRegTypeID;
								break;
						}
					}
					ok = Obj.UpdateItem(id, &rec, 1);
				}
			}
			else {
				ok = Obj.AddItem(&id, &Data, 1);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrPersonKind::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRPERSONKIND_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRPERSONKIND_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRPERSONKIND_CODEREGTYPEID:
		case DSF_CRRPERSONKIND_FOLDERREGTYPEID:
			{
				int is_coderegt = (fldID == DSF_CRRPERSONKIND_CODEREGTYPEID) ? 1 : 0;
				SString buf;
				if(dir == tfdDataToBuf) {
					PPRegisterType regt_rec;
					if(ObjRegT.Search((is_coderegt) ? Data.CodeRegTypeID : Data.FolderRegTypeID, &regt_rec) > 0)
						buf.CopyFrom(regt_rec.Symb);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = ObjRegT.SearchSymb((is_coderegt) ? &Data.CodeRegTypeID : &Data.FolderRegTypeID, buf);
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrCurrency::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPCurrency*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Symb) || strlen(Data.Name)) {
			PPID id = 0;
			Data.Tag = PPOBJ_CURRENCY;
			if(*strip(Data.Symb) != 0 && Obj.SearchSymb(&id, Data.Symb) > 0) {
				Data.ID = id;
				if(UpdateProtocol == updForce) {
					PPCurrency rec;
					MEMSZERO(rec);
					THROW(Obj.Search(id, &rec) > 0);
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRCURRENCY_ID:
								break;
							case DSF_CRRCURRENCY_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRCURRENCY_SYMB:
								STRNSCPY(rec.Symb, Data.Symb);
								break;
							case DSF_CRRCURRENCY_CODE:
								rec.Code = Data.Code;
								break;
						}
					}
					ok = Obj.UpdateItem(id, &rec, 1);
				}
			}
			else {
				ok = Obj.AddItem(&id, &Data, 0);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrCurrency::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRCURRENCY_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRCURRENCY_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRCURRENCY_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRRCURRENCY_CODE:
			ok = TransferData(&Data.Code, sizeof(Data.Code), dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrCurRateType::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPCurRateType*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_CURRATETYPE;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			Data.ID = id;
			if(UpdateProtocol == updForce) {
				PPCurRateType rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRCURRATETYPE_ID:
							break;
						case DSF_CRRCURRATETYPE_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrCurRateType::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRCURRATETYPE_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRCURRENCY_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrScale::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPScale*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_SCALE;
		if(Data.Flags & SCALF_TCPIP)
			PPObjScale::EncodeIP(StrPort, Data.Port, strlen(StrPort) + 1);
		else
			STRNSCPY(Data.Port, StrPort);
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPScale rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRSCALE_ID:
							break;
						case DSF_CRRSCALE_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRSCALE_GET_NUMTRIES:
							rec.Get_NumTries = Data.Get_NumTries;
							break;
						case DSF_CRRSCALE_GET_DELAY:
							rec.Get_Delay = Data.Get_Delay;
							break;
						case DSF_CRRSCALE_PUT_NUMTRIES:
							rec.Put_NumTries = Data.Put_NumTries;
							break;
						case DSF_CRRSCALE_PUT_DELAY:
							rec.Put_Delay = Data.Put_Delay;
							break;
						case DSF_CRRSCALE_QUOTKINDID:
							rec.QuotKindID = Data.QuotKindID;
							break;
						case DSF_CRRSCALE_PORT:
							STRNSCPY(rec.Port, Data.Port);
							break;
						case DSF_CRRSCALE_SCALETYPEID:
							rec.ScaleTypeID = Data.ScaleTypeID;
							break;
						case DSF_CRRSCALE_PROTOCOLVER:
							rec.ProtocolVer = Data.ProtocolVer;
							break;
						case DSF_CRRSCALE_LOGNUM:
							rec.LogNum = Data.LogNum;
							break;
						case DSF_CRRSCALE_LOCATION:
							rec.Location = Data.Location;
							break;
						case DSF_CRRSCALE_ALTGOODSGRP:
							rec.AltGoodsGrp = Data.AltGoodsGrp;
						case DSF_CRRSCALE_FSTRIPWP:
							SETFLAGBYSAMPLE(rec.Flags, SCALF_STRIPWP, Data.Flags); break;
						case DSF_CRRSCALE_FEXSGOODS:
							SETFLAGBYSAMPLE(rec.Flags, SCALF_EXSGOODS, Data.Flags); break;
						case DSF_CRRSCALE_FSYSPINITED:
							SETFLAGBYSAMPLE(rec.Flags, SCALF_SYSPINITED, Data.Flags); break;
						case DSF_CRRSCALE_FTCPIP:
							SETFLAGBYSAMPLE(rec.Flags, SCALF_TCPIP, Data.Flags); break;
						case DSF_CRRSCALE_FCHKINVPAR:
							SETFLAGBYSAMPLE(rec.Flags, SCALF_CHKINVPAR, Data.Flags); break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrScale::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRSCALE_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRSCALE_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRSCALE_GET_NUMTRIES:
			ok = TransferData(&Data.Get_NumTries, dir, rBuf);
			break;
		case DSF_CRRSCALE_GET_DELAY:
			ok = TransferData(&Data.Get_Delay, dir, rBuf);
			break;
		case DSF_CRRSCALE_PUT_NUMTRIES:
			ok = TransferData(&Data.Put_NumTries, dir, rBuf);
			break;
		case DSF_CRRSCALE_PUT_DELAY:
			ok = TransferData(&Data.Put_Delay, dir, rBuf);
			break;
		case DSF_CRRSCALE_QUOTKINDID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPQuotKind qk_rec;
					MEMSZERO(qk_rec);
					if(QKObj.Search(Data.QuotKindID, &qk_rec) > 0)
						buf.CopyFrom(qk_rec.Symb);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = QKObj.SearchSymb(&Data.QuotKindID, buf);
			}
			break;
		case DSF_CRRSCALE_PORT:
			{
				char buf[16];
				memzero(buf, sizeof(buf));
				if(dir == tfdDataToBuf) {
					if(Data.Flags & SCALF_TCPIP)
						PPObjScale::DecodeIP(Data.Port, buf);
					else
						STRNSCPY(buf, Data.Port);
					ok = TransferData(buf, sizeof(buf), dir, rBuf);
				}
				else
					ok = TransferData(StrPort, sizeof(StrPort), dir, rBuf);
			}
			break;
		case DSF_CRRSCALE_SCALETYPEID:
			{
				const char * p_list = "CAS;MASSA-K;METTLER-TOLEDO;CRYSTAL-CASH-SERVER;WEIGHT-TERMINAL;DIGI;BIZEBRA;SHTRIHPRINT";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.ScaleTypeID - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.ScaleTypeID = idx + 1;
					else
						Data.ScaleTypeID = 0;
				}
			}
			break;
		case DSF_CRRSCALE_PROTOCOLVER:
			ok = TransferData(&Data.ProtocolVer, dir, rBuf);
			break;
		case DSF_CRRSCALE_LOGNUM:
			ok = TransferData(&Data.LogNum, dir, rBuf);
			break;
		case DSF_CRRSCALE_LOCATION:
			if(dir == tfdDataToBuf) {
				LocationTbl::Rec lrec;
				MEMSZERO(lrec);
				if(LocObj.Search(Data.Location, &lrec) > 0)
					ok = TransferData(lrec.Code, sizeof(lrec.Code), dir, rBuf);
			}
			else
				ok = LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, rBuf, &Data.Location);
			break;
		case DSF_CRRSCALE_ALTGOODSGRP:
			{
				SString buf;
				Goods2Tbl::Rec ggrec;
				MEMSZERO(ggrec);
				if(dir == tfdDataToBuf) {
					if(GGObj.Search(Data.AltGoodsGrp, &ggrec) > 0)
						buf.CopyFrom(ggrec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData) {
					PPID ggid = 0;
					if(GGObj.SearchByName(buf, &ggid, &ggrec) > 0 && (ggrec.Flags & GF_ALTGROUP))
						Data.AltGoodsGrp = ggid;
				}
			}
			break;
		case DSF_CRRSCALE_FSTRIPWP:
			ok = TransferDataFlag(&Data.Flags, SCALF_STRIPWP, dir, rBuf); break;
		case DSF_CRRSCALE_FEXSGOODS:
			ok = TransferDataFlag(&Data.Flags, SCALF_EXSGOODS, dir, rBuf); break;
		case DSF_CRRSCALE_FSYSPINITED:
			ok = TransferDataFlag(&Data.Flags, SCALF_SYSPINITED, dir, rBuf); break;
		case DSF_CRRSCALE_FTCPIP:
			ok = TransferDataFlag(&Data.Flags, SCALF_TCPIP, dir, rBuf); break;
		case DSF_CRRSCALE_FCHKINVPAR:
			ok = TransferDataFlag(&Data.Flags, SCALF_CHKINVPAR, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrRegisterType::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPRegisterType*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Symb) || strlen(Data.Name)) {
			PPID id = 0;
			Data.Tag = PPOBJ_REGISTERTYPE;
			if(*strip(Data.Symb) != 0 && Obj.SearchSymb(&id, Data.Symb) > 0) {
				Data.ID = id;
				if(UpdateProtocol == updForce) {
					PPRegisterType rec;
					MEMSZERO(rec);
					THROW(Obj.Search(id, &rec) > 0);
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRREGISTERTYPE_ID:
								break;
							case DSF_CRRREGISTERTYPE_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRREGISTERTYPE_SYMB:
								STRNSCPY(rec.Symb, Data.Symb);
								break;
							case DSF_CRRREGISTERTYPE_PERSONKINDID:
								rec.PersonKindID = Data.PersonKindID;
								break;
							case DSF_CRRREGISTERTYPE_REGORGKIND:
								rec.RegOrgKind = Data.RegOrgKind;
								break;
							case DSF_CRRREGISTERTYPE_COUNTERID:
								rec.CounterID = Data.CounterID;
								break;
							case DSF_CRRREGISTERTYPE_FUNIQUE:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_UNIQUE, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FPRIVATE:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_PRIVATE, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FLEGAL:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_LEGAL, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FWARNEXPIRY:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_WARNEXPIRY, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FINSERT:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_INSERT, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FWARNABSENCE:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_WARNABSENCE, Data.Flags); break;
							case DSF_CRRREGISTERTYPE_FDUPNUMBER:
								SETFLAGBYSAMPLE(rec.Flags, REGTF_DUPNUMBER, Data.Flags); break;
						}
					}
					ok = Obj.UpdateItem(id, &rec, 1);
				}
			}
			else {
				ok = Obj.AddItem(&id, &Data, 1);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrRegisterType::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRREGISTERTYPE_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRREGISTERTYPE_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRREGISTERTYPE_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRRREGISTERTYPE_PERSONKINDID:
		case DSF_CRRREGISTERTYPE_REGORGKIND:
			{
				int is_psnkid = (fldID == DSF_CRRREGISTERTYPE_PERSONKINDID) ? 1 : 0;
				SString buf;
				if(dir == tfdDataToBuf) {
					PPPersonKind psnk_rec;
					MEMSZERO(psnk_rec);
					if(PsnKObj.Search((is_psnkid) ? Data.PersonKindID : Data.RegOrgKind, &psnk_rec) > 0)
						buf.CopyFrom(psnk_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					PsnKObj.SearchByName(buf, (is_psnkid) ? &Data.PersonKindID : &Data.RegOrgKind);
			}
			break;
		case DSF_CRRREGISTERTYPE_COUNTERID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOpCounter opc_rec;
					MEMSZERO(opc_rec);
					if(OpCounterObj.Search(Data.CounterID, &opc_rec) > 0)
						buf.CopyFrom(opc_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					OpCounterObj.SearchByName(buf, &Data.CounterID);
			}
			break;
		case DSF_CRRREGISTERTYPE_FUNIQUE:
			ok = TransferDataFlag(&Data.Flags, REGTF_UNIQUE, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FPRIVATE:
			ok = TransferDataFlag(&Data.Flags, REGTF_PRIVATE, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FLEGAL:
			ok = TransferDataFlag(&Data.Flags, REGTF_LEGAL, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FWARNEXPIRY:
			ok = TransferDataFlag(&Data.Flags, REGTF_WARNEXPIRY, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FINSERT:
			ok = TransferDataFlag(&Data.Flags, REGTF_INSERT, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FWARNABSENCE:
			ok = TransferDataFlag(&Data.Flags, REGTF_WARNABSENCE, dir, rBuf); break;
		case DSF_CRRREGISTERTYPE_FDUPNUMBER:
			ok = TransferDataFlag(&Data.Flags, REGTF_DUPNUMBER, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrQuotKind::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPQuotKind*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_QUOTKIND;
		if(*strip(Data.Symb) != 0 && Obj.SearchSymb(&id, Data.Symb) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPQuotKind rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRQUOTKIND_ID:
							break;
						case DSF_CRRQUOTKIND_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRQUOTKIND_SYMB:
							STRNSCPY(rec.Symb, Data.Symb);
							break;
						case DSF_CRRQUOTKIND_DISCOUNT:
							rec.Discount = Data.Discount;
							break;
						case DSF_CRRQUOTKIND_PERIOD:
							rec.Period = Data.Period;
							break;
						case DSF_CRRQUOTKIND_BEGINTM:
							rec.BeginTm = Data.BeginTm;
							break;
						case DSF_CRRQUOTKIND_ENDTM:
							rec.EndTm = Data.EndTm;
							break;
						case DSF_CRRQUOTKIND_RANK:
							rec.Rank = Data.Rank;
							break;
						case DSF_CRRQUOTKIND_OPID:
							rec.OpID = Data.OpID;
							break;
						case DSF_CRRQUOTKIND_ACCSHEETID:
							rec.AccSheetID = Data.AccSheetID;
							break;
						case DSF_CRRQUOTKIND_DAYSOFWEEK:
							rec.DaysOfWeek = Data.DaysOfWeek;
							break;
						case DSF_CRRQUOTKIND_USINGWSCARD:
							rec.UsingWSCard = Data.UsingWSCard;
							break;
						case DSF_CRRQUOTKIND_FABSDIS:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_ABSDIS, Data.Flags); break;
						case DSF_CRRQUOTKIND_FNOTFORBILL:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_NOTFORBILL, Data.Flags); break;
						case DSF_CRRQUOTKIND_FPCTDISONCOST:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_PCTDISONCOST, Data.Flags); break;
						case DSF_CRRQUOTKIND_FDSCNTONGROUPS:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_DSCNTONGROUPS, Data.Flags); break;
						case DSF_CRRQUOTKIND_FEXTPRICEBYBASE:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_EXTPRICEBYBASE, Data.Flags); break;
						case DSF_CRRQUOTKIND_FRETAILED:
							SETFLAGBYSAMPLE(rec.Flags, QUOTKF_RETAILED, Data.Flags); break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrQuotKind::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRQUOTKIND_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRQUOTKIND_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_DISCOUNT:
			ok = TransferData(&Data.Discount, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_PERIOD:
			if((ok = TransferData(&Data.Period.low, dir, rBuf)) > 0)
				ok = TransferData(&Data.Period.upp, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_BEGINTM:
			ok = TransferData(&Data.BeginTm, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_ENDTM:
			ok = TransferData(&Data.EndTm, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_RANK:
			ok = TransferData(&Data.Rank, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_OPID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOprKind opk_rec;
					if(OpKObj.Search(Data.OpID, &opk_rec) > 0)
						buf = opk_rec.Name;
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = OpKObj.SearchByName(buf, &Data.OpID);
			}
			break;
		case DSF_CRRQUOTKIND_ACCSHEETID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPAccSheet acs_rec;
					if(ACCSObj.Search(Data.AccSheetID, &acs_rec) > 0)
						buf = acs_rec.Name;
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = ACCSObj.SearchByName(buf, &Data.AccSheetID);
			}
			break;
		case DSF_CRRQUOTKIND_DAYSOFWEEK:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					for(uint i = 1; i < 7; i++) {
						if(Data.DaysOfWeek & (1 << i)) {
							GetDayOfWeekText(0, i, buf);
							rBuf.Cat(buf).Semicol();
						}
					}
				}
				else {
					StringSet ss(";");
					Data.DaysOfWeek = 0;
					ss.setBuf(rBuf, rBuf.Len() + 1);
					for(uint i = 0; ss.get(&i, buf) > 0;) {
						int day = GetDayOfWeekByText(buf);
						Data.DaysOfWeek |= (1 << day);
					}
				}
			}
			break;
		case DSF_CRRQUOTKIND_USINGWSCARD:
			ok = TransferData((uint8*)&Data.UsingWSCard, dir, rBuf);
			break;
		case DSF_CRRQUOTKIND_FABSDIS:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_ABSDIS, dir, rBuf); break;
		case DSF_CRRQUOTKIND_FNOTFORBILL:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_NOTFORBILL, dir, rBuf); break;
		case DSF_CRRQUOTKIND_FPCTDISONCOST:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_PCTDISONCOST, dir, rBuf); break;
		case DSF_CRRQUOTKIND_FDSCNTONGROUPS:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_DSCNTONGROUPS, dir, rBuf); break;
		case DSF_CRRQUOTKIND_FEXTPRICEBYBASE:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_EXTPRICEBYBASE, dir, rBuf); break;
		case DSF_CRRQUOTKIND_FRETAILED:
			ok = TransferDataFlag(&Data.Flags, QUOTKF_RETAILED, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrAssetWrOffGrp::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPAssetWrOffGrp*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_ASSTWROFFGRP;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPAssetWrOffGrp rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRASSETWROFFGRP_ID:
							break;
						case DSF_CRRASSETWROFFGRP_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRASSETWROFFGRP_CODE:
							STRNSCPY(rec.Code, Data.Code);
							break;
						case DSF_CRRASSETWROFFGRP_WROFFTYPE:
							rec.WrOffType = Data.WrOffType;
							break;
						case DSF_CRRASSETWROFFGRP_WROFFTERM:
							rec.WrOffTerm = Data.WrOffTerm;
							break;
						case DSF_CRRASSETWROFFGRP_LIMIT:
							rec.Limit = Data.Limit;
							break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrAssetWrOffGrp::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRASSETWROFFGRP_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRQUOTKIND_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRASSETWROFFGRP_CODE:
			ok = TransferData(Data.Code, sizeof(Data.Code), dir, rBuf);
			break;
		case DSF_CRRASSETWROFFGRP_WROFFTYPE:
			{
				const char * p_list = "AMORTIZATION-LINE;AMORTIZATION-ACCELERATE;AMORTIZATION-INDIRECT-COST";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.WrOffType - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.WrOffType = idx + 1;
					else
						Data.WrOffType = 0;
				}
			}
			break;
		case DSF_CRRASSETWROFFGRP_WROFFTERM:
			ok = TransferData(&Data.WrOffTerm, dir, rBuf);
			break;
		case DSF_CRRASSETWROFFGRP_LIMIT:
			ok = TransferData(&Data.Limit, dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrMailAccount::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPInternetAccount*)dataPtr;
		else if(addedParam) {
			if(Obj.Get(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID   id = 0;
		Data.Tag = PPOBJ_INTERNETACCOUNT;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				SString buf;
				Data.ID = id;
				PPInternetAccount rec;
				THROW(Obj.Get(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRMAILACCOUNT_ID:
							break;
						case DSF_CRRMAILACCOUNT_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRMAILACCOUNT_SMTPAUTHTYPE:
							rec.SmtpAuthType = Data.SmtpAuthType;
							break;
						case DSF_CRRMAILACCOUNT_TIMEOUT:
							rec.Timeout = Data.Timeout;
							break;
						case DSF_CRRMAILACCOUNT_PERSONID:
							rec.PersonID = Data.PersonID;
							break;
						case DSF_CRRMAILACCOUNT_SENDSERVER:
							Data.GetExtField(MAEXSTR_SENDSERVER, buf);
							rec.SetExtField(MAEXSTR_SENDSERVER,  buf);
							break;
						case DSF_CRRMAILACCOUNT_SENDPORT:
							Data.GetExtField(MAEXSTR_SENDPORT, buf);
							rec.SetExtField(MAEXSTR_SENDPORT,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RSRVSENDSERVER:
							Data.GetExtField(MAEXSTR_RSRVSENDSERVER, buf);
							rec.SetExtField(MAEXSTR_RSRVSENDSERVER,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RSRVSENDPORT:
							Data.GetExtField(MAEXSTR_RSRVSENDPORT, buf);
							rec.SetExtField(MAEXSTR_RSRVSENDPORT,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RCVSERVER:
							Data.GetExtField(MAEXSTR_RCVSERVER, buf);
							rec.SetExtField(MAEXSTR_RCVSERVER,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RCVPORT:
							Data.GetExtField(MAEXSTR_RCVPORT, buf);
							rec.SetExtField(MAEXSTR_RCVPORT,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RCVNAME:
							Data.GetExtField(MAEXSTR_RCVNAME, buf);
							rec.SetExtField(MAEXSTR_RCVNAME,  buf);
							break;
						case DSF_CRRMAILACCOUNT_RCVPASSWORD:
							{
								char cbuf[512];
								Data.GetMimedPassword(cbuf, sizeof(cbuf));
								rec.SetMimedPassword(cbuf);
							}
							break;
						case DSF_CRRMAILACCOUNT_FROMADDRESS:
							Data.GetExtField(MAEXSTR_FROMADDRESS, buf);
							rec.SetExtField(MAEXSTR_FROMADDRESS,  buf);
							break;
					}
				}
				ok = Obj.Put(&id, &rec, 0);
			}
		}
		else
			ok = Obj.Put(&id, &Data, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrMailAccount::TransferMailField(int mailFldID, Tfd dir, SString & rBuf)
{
	int ok = -1;
	SString buf;
	if(dir == tfdDataToBuf) {
		Data.GetExtField(mailFldID, buf);
		ok = TransferData(buf, dir, rBuf);
	}
	else
		Data.SetExtField(mailFldID,  rBuf);
	return ok;
}

int SLAPI PPDS_CrrMailAccount::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRMAILACCOUNT_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRMAILACCOUNT_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_SMTPAUTHTYPE:
			{
				const char * p_list = "PLAIN;LOGIN;MD5;POP3";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.SmtpAuthType - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.SmtpAuthType = idx + 1;
					else
						Data.SmtpAuthType = 0;
				}
			}
			break;
		case DSF_CRRMAILACCOUNT_TIMEOUT:
			ok = TransferData(&Data.Timeout, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_PERSONID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PersonTbl::Rec psn_rec;
					MEMSZERO(psn_rec);
					if(PsnObj.Search(Data.PersonID, &psn_rec) > 0)
						buf.CopyFrom(psn_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = PsnObj.P_Tbl->SearchByName(buf, &Data.PersonID);
			}
			break;
		case DSF_CRRMAILACCOUNT_SENDSERVER:
			ok = TransferMailField(MAEXSTR_SENDSERVER, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_SENDPORT:
			ok = TransferMailField(MAEXSTR_SENDPORT, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RSRVSENDSERVER:
			ok = TransferMailField(MAEXSTR_RSRVSENDSERVER, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RSRVSENDPORT:
			ok = TransferMailField(MAEXSTR_RSRVSENDPORT, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RCVSERVER:
			ok = TransferMailField(MAEXSTR_RCVSERVER, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RCVPORT:
			ok = TransferMailField(MAEXSTR_RCVPORT, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RCVNAME:
			ok = TransferMailField(MAEXSTR_RCVNAME, dir, rBuf);
			break;
		case DSF_CRRMAILACCOUNT_RCVPASSWORD:
			{
				if(dir == tfdDataToBuf) {
					char buf[512];
					memzero(buf, sizeof(buf));
					Data.GetMimedPassword(buf, sizeof(buf));
					ok = TransferData(buf, sizeof(buf), dir, rBuf);
				}
				else
					ok = Data.SetMimedPassword(rBuf);
			}
			break;
		case DSF_CRRMAILACCOUNT_FROMADDRESS:
			ok = TransferMailField(MAEXSTR_FROMADDRESS, dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrPersonRelType::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPPersonRelTypePacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Symb) != 0 && Obj.SearchSymb(&id, Data.Rec.Symb) > 0) {
			Data.Rec.ID = id;
			if(UpdateProtocol == updForce) {
				SString buf;
				PPPersonRelTypePacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRPERSONRELTYPE_ID:
							break;
						case DSF_CRRPERSONRELTYPE_NAME:
							STRNSCPY(pack.Rec.Name, Data.Rec.Name);
							break;
						case DSF_CRRPERSONRELTYPE_SYMB:
							STRNSCPY(pack.Rec.Symb, Data.Rec.Symb);
							break;
						case DSF_CRRPERSONRELTYPE_STATUSRESTRICTION:
							pack.Rec.StatusRestriction = Data.Rec.StatusRestriction;
							break;
						case DSF_CRRPERSONRELTYPE_CARDINALITY:
							pack.Rec.Cardinality = Data.Rec.Cardinality;
							break;
						case DSF_CRRPERSONRELTYPE_FINHADDR:
							SETFLAGBYSAMPLE(pack.Rec.Flags, PPPersonRelType::fInhAddr, Data.Rec.Flags); break;
						case DSF_CRRPERSONRELTYPE_FINHRADDR:
							SETFLAGBYSAMPLE(pack.Rec.Flags, PPPersonRelType::fInhRAddr, Data.Rec.Flags); break;
						case DSF_CRRPERSONRELTYPE_INHREGTYPELIST:
							pack.InhRegTypeList = Data.InhRegTypeList;
							break;
					}
				}
				ok = Obj.PutPacket(&id, &pack, 0);
			}
		}
		else {
			ok = Obj.PutPacket(&id, &Data, 0);
			Data.Rec.ID = id;
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrPersonRelType::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRPERSONRELTYPE_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRPERSONRELTYPE_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_CRRPERSONRELTYPE_SYMB:
			ok = TransferData(Data.Rec.Symb, sizeof(Data.Rec.Symb), dir, rBuf);
			break;
		case DSF_CRRPERSONRELTYPE_STATUSRESTRICTION:
			{
				const char * p_list = "UNDEF;PRIVATE-TO-PRIVATE;PRIVATE-TO-LEGAL;LEGAL-TO-LEGAL";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Rec.StatusRestriction);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Rec.StatusRestriction = idx;
					else
						Data.Rec.StatusRestriction = 0;
				}
			}
			break;
		case DSF_CRRPERSONRELTYPE_CARDINALITY:
			{
				const char * p_list = "UNDEF;ONE-TO-ONE;ONE-TO-MANY;MANY-TO-ONE;MANY-TO-MANY";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Rec.StatusRestriction);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Rec.StatusRestriction = idx;
					else
						Data.Rec.StatusRestriction = 0;
				}
			}
			break;
		case DSF_CRRPERSONRELTYPE_FINHADDR:
			ok = TransferDataFlag(&Data.Rec.Flags, PPPersonRelType::fInhAddr, dir, rBuf); break;
			break;
		case DSF_CRRPERSONRELTYPE_FINHRADDR:
			ok = TransferDataFlag(&Data.Rec.Flags, PPPersonRelType::fInhRAddr, dir, rBuf); break;
			break;
		case DSF_CRRPERSONRELTYPE_INHREGTYPELIST:
			{
				PPRegisterType regt_rec;
				TempBuf = 0;
				MEMSZERO(regt_rec);
				if(dir == tfdDataToBuf) {
					if((*pIter) < Data.InhRegTypeList.getCount()) {
						if(ObjRegT.Search(Data.InhRegTypeList.at(*pIter), &regt_rec) > 0) {
							TempBuf = regt_rec.Symb;
							/*
							if(TempBuf.Strip().Empty())
								;// TempBuf = regt_rec.Name;
							*/
						}
						else
							TempBuf = 0;
						ok = TransferData(TempBuf, dir, rBuf);
					}
					else
						ok = -1;
				}
				else if(dir == tfdBufToData) {
					ok = TransferData(TempBuf, dir, rBuf);
					if(TempBuf.NotEmptyS()) {
						PPID id = 0;
						if(ObjRegT.SearchSymb(&id, TempBuf) > 0)
							Data.InhRegTypeList.addUnique(id);
					}
				}
			}
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrObjTag::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPObjTagPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if((Data.Rec.TagDataType == OTTYP_GROUP &&
			*strip(Data.Rec.Name) != 0 && Obj.SearchByName(Data.Rec.Name, &id) > 0) ||
			*strip(Data.Rec.Symb) != 0 && Obj.SearchBySymb(Data.Rec.Symb, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				PPObjTagPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRROBJTAG_ID:
							break;
						case DSF_CRROBJTAG_NAME:
							STRNSCPY(pack.Rec.Name, Data.Rec.Name);
							break;
						case DSF_CRROBJTAG_SYMB:
							STRNSCPY(pack.Rec.Symb, Data.Rec.Symb);
							break;
						case DSF_CRROBJTAG_LINKOBJGRP:
							pack.Rec.LinkObjGrp = Data.Rec.LinkObjGrp;
							break;
						case DSF_CRROBJTAG_TAGENUMID:
							pack.Rec.TagEnumID = Data.Rec.TagEnumID;
							break;
						case DSF_CRROBJTAG_TAGDATATYPE:
							pack.Rec.TagDataType = Data.Rec.TagDataType;
							break;
						case DSF_CRROBJTAG_OBJTYPEID:
							pack.Rec.ObjTypeID = Data.Rec.ObjTypeID;
							break;
						case DSF_CRROBJTAG_TAGGROUPID:
							pack.Rec.TagGroupID = Data.Rec.TagGroupID;
							break;
						case DSF_CRROBJTAG_FDUP:
							SETFLAGBYSAMPLE(pack.Rec.Flags, /*OTF_DUP*/0x01, Data.Rec.Flags); // @v8.0.1 Флаг OTF_DUP упразднен. Строка оставлена для совместимости.
							break;
						case DSF_CRROBJTAG_FNOZERO:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OTF_NOZERO, Data.Rec.Flags);
							break;
						case DSF_CRROBJTAG_FNMBRULE:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OTF_NMBRULE, Data.Rec.Flags);
							break;
					}
				}
				ok = Obj.PutPacket(&id, &pack, 0);
			}
		}
		else
			ok = Obj.PutPacket(&id, &Data, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrObjTag::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRROBJTAG_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRROBJTAG_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_CRROBJTAG_SYMB:
			ok = TransferData(Data.Rec.Symb, sizeof(Data.Rec.Symb), dir, rBuf);
			break;
		case DSF_CRROBJTAG_LINKOBJGRP:
			break;
		case DSF_CRROBJTAG_TAGENUMID:
			break;
		case DSF_CRROBJTAG_TAGDATATYPE:
			{
				const char * p_list = "GROUP;BOOL;STRING;NUMBER;ENUM;INT;OBJLINK;DATE";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Rec.TagDataType);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Rec.TagDataType = idx;
					else
						Data.Rec.TagDataType = -1;
				}
			}
			break;
		case DSF_CRROBJTAG_OBJTYPEID:
			break;
		case DSF_CRROBJTAG_TAGGROUPID:
			{
				SString buf;
				if(dir ==  tfdDataToBuf) {
					PPObjectTag tag;
					MEMSZERO(tag);
					if(Data.Rec.TagGroupID && Obj.Search(Data.Rec.TagGroupID, &tag) > 0)
						buf.CopyFrom(tag.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = Obj.SearchByName(buf, &Data.Rec.TagGroupID);
			}
			break;
		case DSF_CRROBJTAG_FDUP:
			ok = TransferDataFlag(&Data.Rec.Flags, /*OTF_DUP*/0x01, dir, rBuf); break; // @v8.0.1 Флаг OTF_DUP упразднен. Строка оставлена для совместимости.
			break;
		case DSF_CRROBJTAG_FNOZERO:
			ok = TransferDataFlag(&Data.Rec.Flags, OTF_NOZERO, dir, rBuf); break;
			break;
		case DSF_CRROBJTAG_FNMBRULE:
			ok = TransferDataFlag(&Data.Rec.Flags, OTF_NMBRULE, dir, rBuf); break;
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrDraftWrOffEntry::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr) {
			Data = *(PPDraftWrOffEntry *)dataPtr;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrDraftWrOffEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRDRAFTWROFFENTRY_OPID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOprKind opk_rec;
					MEMSZERO(opk_rec);
					if(OpKObj.Search(Data.OpID, &opk_rec) > 0)
						buf.CopyFrom(opk_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = OpKObj.SearchByName(buf, &Data.OpID);
			}
			break;
		case DSF_CRRDRAFTWROFFENTRY_LOCID:
			if(dir == tfdDataToBuf) {
				LocationTbl::Rec lrec;
				MEMSZERO(lrec);
				if(LocObj.Search(Data.LocID, &lrec) > 0)
					ok = TransferData(lrec.Code, sizeof(lrec.Code), dir, rBuf);
			}
			else
				ok = LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, rBuf, &Data.LocID);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrDraftWrOff::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPDraftWrOffPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		if(*strip(Data.Rec.Name) != 0 && Obj.SearchByName(Data.Rec.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.Rec.ID = id;
				PPDraftWrOffPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRDRAFTWROFF_ID:
							break;
						case DSF_CRRDRAFTWROFF_NAME:
							STRNSCPY(pack.Rec.Name, Data.Rec.Name);
							break;
						case DSF_CRRDRAFTWROFF_POOLOPID:
							pack.Rec.PoolOpID = Data.Rec.PoolOpID;
							break;
						case DSF_CRRDRAFTWROFF_DFCTCOMPENSOPID:
							pack.Rec.DfctCompensOpID = Data.Rec.DfctCompensOpID;
							break;
						case DSF_CRRDRAFTWROFF_DFCTCOMPENSARID:
							// pack.Rec.DfctCompensArID = Data.Rec.DfctCompensArID;
							break;
						case DSF_CRRDRAFTWROFF_FDFCTARISLOC:
							SETFLAGBYSAMPLE(pack.Rec.Flags, DWOF_DFCTARISLOC, Data.Rec.Flags); break;
						case DSF_CRRDRAFTWROFF_FUSEMRPTAB:
							SETFLAGBYSAMPLE(pack.Rec.Flags, DWOF_USEMRPTAB, Data.Rec.Flags); break;
						case DSF_CRRDRAFTWROFF_ITEMS:
							if(Data.P_List) {
								ZDELETE(pack.P_List);
								THROW_MEM(pack.P_List = new ::SArray(sizeof(PPDraftWrOffEntry)));
								pack.P_List->copy(*Data.P_List);
							}
							break;
					}
				}
				ok = Obj.PutPacket(&id, &pack, 0);
			}
		}
		else
			ok = Obj.PutPacket(&id, &Data, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrDraftWrOff::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRDRAFTWROFF_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRDRAFTWROFF_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_CRRDRAFTWROFF_POOLOPID:
		case DSF_CRRDRAFTWROFF_DFCTCOMPENSOPID:
			{
				int is_poolopid = (fldID == DSF_CRRDRAFTWROFF_POOLOPID) ? 1 : 0;
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOprKind opk_rec;
					MEMSZERO(opk_rec);
					if(OpKObj.Search((is_poolopid) ? Data.Rec.PoolOpID : Data.Rec.DfctCompensOpID, &opk_rec) > 0)
						buf.CopyFrom(opk_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = OpKObj.SearchByName(buf, (is_poolopid) ? &Data.Rec.PoolOpID : &Data.Rec.DfctCompensOpID);
			}
			break;
		case DSF_CRRDRAFTWROFF_DFCTCOMPENSARID:
			if(dir == tfdDataToBuf) {
			   	ArticleTbl::Rec ar_rec;
				MEMSZERO(ar_rec);
				if(ArObj.Search(Data.Rec.DfctCompensArID, &ar_rec) > 0)
					ok = TransferData(ar_rec.Name, sizeof(ar_rec.Name), dir, rBuf);
			}
			/*
			else
				ok = ArObj.P_Tbl->SearchName(acc_sheet_id, rBuf, &Data.Rec.DfctCompensArID);
			*/
			break;
		case DSF_CRRDRAFTWROFF_FDFCTARISLOC:
			ok = TransferDataFlag(&Data.Rec.Flags, DWOF_DFCTARISLOC, dir, rBuf); break;
		case DSF_CRRDRAFTWROFF_FUSEMRPTAB:
			ok = TransferDataFlag(&Data.Rec.Flags, DWOF_USEMRPTAB, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrDraftWrOff::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{

	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRDRAFTWROFF_ITEMS) {
			PPDraftWrOffEntry item = ((PPDS_CrrDraftWrOffEntry *)pData)->Data;
			THROW_MEM(SETIFZ(Data.P_List, new ::SArray(sizeof(PPDraftWrOffEntry))));
			ok = Data.P_List->insert(&item);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrDraftWrOff::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	PPID   id = 0;
	if(fldID == DSF_CRRDRAFTWROFF_ITEMS) {
		if(Data.P_List && (*pIter < Data.P_List->getCount())) {
			pData->InitData(idoExtract, Data.P_List->at(*pIter), 0);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrLocation
//
int SLAPI PPDS_CrrLocation::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc)
		MEMSZERO(Data);
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(LocationTbl::Rec*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		strip(Data.Code);
		strip(Data.Name);
		if(strlen(Data.Code) || strlen(Data.Name)) {
			PPID   id = 0;
			SString temp_buf;
			LocationTbl::Rec rec;
			if(Obj.P_Tbl->SearchMaxLike(&Data, 0, &id, &rec) > 0) {
				Data.ID = rec.ID;
				if(UpdateProtocol == updForce) {
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRLOCATION_ID:
								break;
							case DSF_CRRLOCATION_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRLOCATION_TYPE:
								rec.Type = Data.Type;
								break;
							case DSF_CRRLOCATION_OWNER:
								rec.OwnerID = Data.OwnerID;
								break;
							case DSF_CRRLOCATION_RSPNSPERSON:
								rec.RspnsPersonID = Data.RspnsPersonID;
								break;
							case DSF_CRRLOCATION_CITYID:
								// rec.CityID = Data.CityID;
								break;
							case DSF_CRRLOCATION_CODE:
								STRNSCPY(rec.Code, Data.Code);
								break;
							case DSF_CRRLOCATION_ZIP:
								LocationCore::GetExField(&Data, LOCEXSTR_ZIP, temp_buf);
								LocationCore::SetExField(&rec, LOCEXSTR_ZIP, temp_buf);
								break;
							case DSF_CRRLOCATION_ADDRESS:
								LocationCore::GetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
								LocationCore::SetExField(&rec, LOCEXSTR_SHORTADDR, temp_buf);
								break;
							case DSF_CRRLOCATION_MASSCAPACITY:
								rec.MassCapacity = Data.MassCapacity;
								break;
							case DSF_CRRLOCATION_X: rec.X = Data.X; break;
							case DSF_CRRLOCATION_Y: rec.Y = Data.Y; break;
							case DSF_CRRLOCATION_Z: rec.Z = Data.Z; break;
							case DSF_CRRLOCATION_LATITUDE:  rec.Latitude  = Data.Latitude; break;
							case DSF_CRRLOCATION_LONGITUDE: rec.Longitude = Data.Longitude; break;
							case DSF_CRRLOCATION_NUMROWS:   rec.NumRows   = Data.NumRows; break;
							case DSF_CRRLOCATION_NUMLAYERS: rec.NumLayers = Data.NumLayers; break;
							case DSF_CRRLOCATION_DEPTH:     rec.Depth     = Data.Depth; break;
							case DSF_CRRLOCATION_FULLADDR:
								LocationCore::GetExField(&Data, LOCEXSTR_FULLADDR, temp_buf);
								LocationCore::SetExField(&rec, LOCEXSTR_FULLADDR, temp_buf);
								break;
							case DSF_CRRLOCATION_FVATFREE:
								SETFLAGBYSAMPLE(rec.Flags, LOCF_VATFREE, Data.Flags);
								break;
							case DSF_CRRLOCATION_FMANUALADDR:
								SETFLAGBYSAMPLE(rec.Flags, LOCF_MANUALADDR, Data.Flags);
								break;
							case DSF_CRRLOCATION_FVOLUMEVAL:
								SETFLAGBYSAMPLE(rec.Flags, LOCF_VOLUMEVAL, Data.Flags);
								break;
							case DSF_CRRLOCATION_FCOMPARABLE:
								SETFLAGBYSAMPLE(rec.Flags, LOCF_COMPARABLE, Data.Flags);
								break;
						}
					}
					ok = Obj.PutRecord(&id, &rec, 0);
				}
			}
			else {
				ok = Obj.PutRecord(&id, &Data, 0);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrLocation::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	SString temp_buf;
	switch(fldID) {
		case DSF_CRRLOCATION_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRLOCATION_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRLOCATION_TYPE:
			{
				const char * p_list = "WAREHOUSE;WAREPLACE;WHZONE;ADDRESS;DIVISION;WAREHOUSEGROUP;WHCOLUMN;WHCELL";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Type - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Type = idx + 1;
					else
						Data.Type = 0;
				}
			}
			ok = TransferData(&Data.Type, dir, rBuf);
			break;
		case DSF_CRRLOCATION_CITYID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.CityID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRLOCATION_CODE:
			ok = TransferData(Data.Code, sizeof(Data.Code), dir, rBuf);
			break;
		case DSF_CRRLOCATION_ZIP:
			if(dir == tfdDataToBuf)
				LocationCore::GetExField(&Data, LOCEXSTR_ZIP, temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				LocationCore::SetExField(&Data, LOCEXSTR_ZIP, temp_buf);
			break;
		case DSF_CRRLOCATION_ADDRESS:
			if(dir == tfdDataToBuf)
				LocationCore::GetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				LocationCore::SetExField(&Data, LOCEXSTR_SHORTADDR, temp_buf);
			break;
		case DSF_CRRLOCATION_MASSCAPACITY:
			ok = TransferData(&Data.MassCapacity, dir, rBuf);
			break;
		case DSF_CRRLOCATION_X:         ok = TransferData(&Data.X, dir, rBuf); break;
		case DSF_CRRLOCATION_Y:         ok = TransferData(&Data.Y, dir, rBuf); break;
		case DSF_CRRLOCATION_Z:         ok = TransferData(&Data.Z, dir, rBuf); break;
		case DSF_CRRLOCATION_LATITUDE:  ok = TransferData(&Data.Latitude, dir, rBuf);  break;
		case DSF_CRRLOCATION_LONGITUDE: ok = TransferData(&Data.Longitude, dir, rBuf); break;
		case DSF_CRRLOCATION_NUMROWS:   ok = TransferData(&Data.NumRows, dir, rBuf);   break;
		case DSF_CRRLOCATION_NUMLAYERS: ok = TransferData(&Data.NumLayers, dir, rBuf); break;
		case DSF_CRRLOCATION_DEPTH:     ok = TransferData(&Data.Depth, dir, rBuf);     break;
		case DSF_CRRLOCATION_FULLADDR:
			if(dir == tfdDataToBuf)
				LocationCore::GetExField(&Data, LOCEXSTR_FULLADDR, temp_buf);
			ok = TransferData(temp_buf, dir, rBuf);
			if(dir == tfdBufToData)
				LocationCore::SetExField(&Data, LOCEXSTR_FULLADDR, temp_buf);
			break;
		case DSF_CRRLOCATION_FVATFREE:
			ok = TransferDataFlag(&Data.Flags, LOCF_VATFREE, dir, rBuf); break;
		case DSF_CRRLOCATION_FMANUALADDR:
			ok = TransferDataFlag(&Data.Flags, LOCF_MANUALADDR, dir, rBuf); break;
		case DSF_CRRLOCATION_FVOLUMEVAL:
			ok = TransferDataFlag(&Data.Flags, LOCF_VOLUMEVAL, dir, rBuf); break;
		case DSF_CRRLOCATION_FCOMPARABLE:
			ok = TransferDataFlag(&Data.Flags, LOCF_COMPARABLE, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrLocation::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(oneof2(fldID, DSF_CRRLOCATION_OWNER, DSF_CRRLOCATION_RSPNSPERSON)) {
			PPPersonPacket item = ((PPDS_CrrPerson *)pData)->Data;
			if(fldID == DSF_CRRLOCATION_OWNER)
				Data.OwnerID = item.Rec.ID;
			else
				Data.RspnsPersonID = item.Rec.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRLOCATION_PARENT) {
			LocationTbl::Rec rec = ((PPDS_CrrLocation *)pData)->Data;
			Data.ParentID = rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrLocation::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRLOCATION_OWNER) {
		if(*pIter == 0 && Data.OwnerID) {
			pData->InitData(idoExtract, 0, Data.OwnerID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRLOCATION_RSPNSPERSON) {
		if(*pIter == 0 && Data.RspnsPersonID) {
			pData->InitData(idoExtract, 0, Data.RspnsPersonID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRLOCATION_PARENT) {
		if(*pIter == 0 && Data.ParentID) {
			pData->InitData(idoExtract, 0, Data.ParentID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrArticle
//
int SLAPI PPDS_CrrArticle::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		MEMSZERO(Data);
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(ArticleTbl::Rec*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Name) && Data.AccSheetID) {
			PPID id = 0;
			ArticleTbl::Rec rec;
			if(*strip(Data.Name) != 0 && Obj.P_Tbl->SearchName(Data.AccSheetID, Data.Name, &rec) > 0) {
				Data.ID = id = rec.ID;
				if(UpdateProtocol == updForce) {
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRARTICLE_ID:
								break;
							case DSF_CRRARTICLE_ARTICLE:
								rec.Article = Data.Article;
								break;
							case DSF_CRRARTICLE_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRARTICLE_CLOSED:
								rec.Closed = Data.Closed;
								break;
							case DSF_CRRARTICLE_ACCSHEET:
								rec.AccSheetID = Data.AccSheetID;
								break;
							case DSF_CRRARTICLE_ASSOCPERSON:
								rec.ObjID = Data.ObjID;
								break;
							case DSF_CRRARTICLE_ASSOCLOC:
								rec.ObjID = Data.ObjID;
								break;
							case DSF_CRRARTICLE_ASSOCACCOUNT:
								rec.ObjID = Data.ObjID;
							case DSF_CRRARTICLE_FGROUP:
								SETFLAGBYSAMPLE(rec.Flags, ARTRF_GROUP, Data.Flags);
								break;
							case DSF_CRRARTICLE_FSTOPBILL:
								SETFLAGBYSAMPLE(rec.Flags, ARTRF_STOPBILL, Data.Flags);
								break;
							case DSF_CRRARTICLE_FBUDGINC:
								SETFLAGBYSAMPLE(rec.Flags, ARTRF_BUDG_INC, Data.Flags);
								break;
						}
					}
					rec.ObjID = (rec.ObjID) ? rec.ObjID : rec.Article;
					ok = Obj.P_Tbl->Update(id, &rec, 0);
				}
			}
			else {
				Data.ObjID = (Data.ObjID) ? Data.ObjID : Data.Article;
				ok = Obj.P_Tbl->Add(&id, &Data, 0);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrArticle::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRARTICLE_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRARTICLE_ARTICLE:
			ok = TransferData(&Data.Article, dir, rBuf);
			break;
		case DSF_CRRARTICLE_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRARTICLE_CLOSED:
			ok = TransferData(&Data.Closed, dir, rBuf);
			break;
		case DSF_CRRARTICLE_FGROUP:
			ok = TransferDataFlag(&Data.Flags, ARTRF_GROUP, dir, rBuf); break;
			break;
		case DSF_CRRARTICLE_FSTOPBILL:
			ok = TransferDataFlag(&Data.Flags, ARTRF_STOPBILL, dir, rBuf); break;
			break;
		case DSF_CRRARTICLE_FBUDGINC:
			ok = TransferDataFlag(&Data.Flags, ARTRF_BUDG_INC, dir, rBuf); break;
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrArticle::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRARTICLE_ACCSHEET) {
			PPAccSheet item = ((PPDS_CrrAccSheet *)pData)->Data;
			Data.AccSheetID = item.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRARTICLE_ASSOCPERSON) {
			PPPersonPacket pack = ((PPDS_CrrPerson *)pData)->Data;
			Data.ObjID = (Data.ObjID) ? Data.ObjID : pack.Rec.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRARTICLE_ASSOCLOC) {
			LocationTbl::Rec rec = ((PPDS_CrrLocation *)pData)->Data;
			Data.ObjID = (Data.ObjID) ? Data.ObjID : rec.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRARTICLE_ASSOCACCOUNT) {
			PPAccountPacket pack = ((PPDS_CrrAccount *)pData)->Data;
			Data.ObjID = (Data.ObjID) ? Data.ObjID : pack.Rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrArticle::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	long assoc = 0;
	if(Data.AccSheetID) {
		PPAccSheet acs_rec;
		AccSheetObj.Fetch(Data.AccSheetID, &acs_rec);
		assoc = acs_rec.Assoc;
	}
	if(fldID == DSF_CRRARTICLE_ACCSHEET) {
		if(*pIter == 0 && Data.AccSheetID) {
			pData->InitData(idoExtract, 0, Data.AccSheetID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRARTICLE_ASSOCPERSON) {
		if(*pIter == 0 && assoc == PPOBJ_PERSON && Data.ObjID) {
			pData->InitData(idoExtract, 0, Data.ObjID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRARTICLE_ASSOCLOC) {
		if(*pIter == 0 && assoc == PPOBJ_LOCATION && Data.ObjID) {
			pData->InitData(idoExtract, 0, Data.ObjID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRARTICLE_ASSOCACCOUNT) {
		if(*pIter == 0 && assoc == PPOBJ_ACCOUNT2 && Data.ObjID) {
			pData->InitData(idoExtract, 0, Data.ObjID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrAccSheet
//
int SLAPI PPDS_CrrAccSheet::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		MEMSZERO(Data);
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPAccSheet*)dataPtr;
		else if(addedParam) {
			if(Obj.Fetch(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Name) || strlen(Data.Symb)) {
			PPID   id = 0;
			PPAccSheet rec;
			if((*strip(Data.Symb) != 0 && Obj.SearchBySymb(Data.Symb, &id, &rec) > 0) ||
				(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, &rec) > 0)
			) {
				Data.ID = id;
				if(UpdateProtocol == updForce) {
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRACCSHEET_ID:
								break;
							case DSF_CRRACCSHEET_NAME:
								STRNSCPY(rec.Name, Data.Name);
								break;
							case DSF_CRRACCSHEET_SYMB:
								STRNSCPY(rec.Symb, Data.Symb);
								break;
							case DSF_CRRACCSHEET_ASSOC:
								rec.Assoc = Data.Assoc;
								break;
							case DSF_CRRACCSHEET_CODEREGTYPE:
								rec.CodeRegTypeID = Data.CodeRegTypeID;
								break;
							case DSF_CRRACCSHEET_PERSONKIND:
								rec.ObjGroup = Data.ObjGroup;
								break;
							case DSF_CRRACCSHEET_FAUTOCREATART:
								SETFLAGBYSAMPLE(rec.Flags, ACSHF_AUTOCREATART, Data.Flags);
								break;
							case DSF_CRRACCSHEET_FUSECLIAGT:
								SETFLAGBYSAMPLE(rec.Flags, ACSHF_USECLIAGT, Data.Flags);
								break;
							case DSF_CRRACCSHEET_FUSEALIASSUBST:
								SETFLAGBYSAMPLE(rec.Flags, ACSHF_USEALIASSUBST, Data.Flags);
								break;
							case DSF_CRRACCSHEET_FUSESUPPLAGT:
								SETFLAGBYSAMPLE(rec.Flags, ACSHF_USESUPPLAGT, Data.Flags);
								break;
						}
					}
					ok = Obj.UpdateItem(id, &rec, 1);
				}
			}
			else {
				ok = Obj.AddItem(&id, &Data, 1);
				Data.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrAccSheet::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRACCSHEET_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRACCSHEET_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRACCSHEET_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRRACCSHEET_ASSOC:
			{
				const char * p_list = "NONE;PERSON;LOCATION;ACCOUNT";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Assoc);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Assoc = idx;
					else
						Data.Assoc = -1;
				}
			}
			break;
		case DSF_CRRACCSHEET_FAUTOCREATART:
			ok = TransferDataFlag(&Data.Flags, ACSHF_AUTOCREATART, dir, rBuf); break;
			break;
		case DSF_CRRACCSHEET_FUSECLIAGT:
			ok = TransferDataFlag(&Data.Flags, ACSHF_USECLIAGT, dir, rBuf); break;
			break;
		case DSF_CRRACCSHEET_FUSEALIASSUBST:
			ok = TransferDataFlag(&Data.Flags, ACSHF_USEALIASSUBST, dir, rBuf); break;
			break;
		case DSF_CRRACCSHEET_FUSESUPPLAGT:
			ok = TransferDataFlag(&Data.Flags, ACSHF_USESUPPLAGT, dir, rBuf); break;
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrAccSheet::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRACCSHEET_CODEREGTYPE) {
			PPRegisterType item = ((PPDS_CrrRegisterType *)pData)->Data;
			Data.CodeRegTypeID = item.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRACCSHEET_PERSONKIND) {
			PPPersonKind item = ((PPDS_CrrPersonKind*)pData)->Data;
			Data.ObjGroup = item.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrAccSheet::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRACCSHEET_CODEREGTYPE) {
		if(*pIter == 0 && Data.CodeRegTypeID) {
			pData->InitData(idoExtract, 0, Data.CodeRegTypeID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRACCSHEET_PERSONKIND) {
		if(*pIter == 0 && Data.ObjGroup) {
			pData->InitData(idoExtract, 0, Data.ObjGroup);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrAccount
//
int SLAPI PPDS_CrrAccount::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPAccountPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Rec.Name) && (Data.Rec.A.Ac || Data.Rec.A.Sb)) {
			PPID id = 0;
			PPAccount acct_rec;
			if((Data.Rec.A.Ac && Data.Rec.A.Sb && Obj.SearchNum(Data.Rec.A.Ac, Data.Rec.A.Sb, Data.Rec.CurID, &acct_rec) > 0) ||
				(*strip(Data.Rec.Code) != 0 && Obj.SearchCode(Data.Rec.Code, Data.Rec.CurID, &acct_rec) > 0)) {
				Data.Rec.ID = id = acct_rec.ID;
				if(UpdateProtocol == updForce) {
					PPAccountPacket pack;
					THROW(Obj.GetPacket(id, &pack) > 0);
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRRACCOUNT_ID:
								break;
							case DSF_CRRACCOUNT_AC:
								pack.Rec.A.Ac = Data.Rec.A.Ac;
								break;
							case DSF_CRRACCOUNT_SB:
								pack.Rec.A.Sb = Data.Rec.A.Sb;
								break;
							case DSF_CRRACCOUNT_CODE:
								STRNSCPY(pack.Rec.Code, Data.Rec.Code);
								break;
							case DSF_CRRACCOUNT_CURR:
								pack.Rec.CurID = Data.Rec.CurID;
								break;
							case DSF_CRRACCOUNT_NAME:
								STRNSCPY(pack.Rec.Name, Data.Rec.Name);
								break;
							case DSF_CRRACCOUNT_OPENDATE:
								pack.Rec.OpenDate = Data.Rec.OpenDate;
								break;
							case DSF_CRRACCOUNT_TYPE:
								pack.Rec.Type = Data.Rec.Type;
								break;
							case DSF_CRRACCOUNT_KIND:
								pack.Rec.Kind = Data.Rec.Kind;
								break;
							case DSF_CRRACCOUNT_LIMIT:
								pack.Rec.Limit = Data.Rec.Limit;
								break;
							case DSF_CRRACCOUNT_OVERDRAFT:
								pack.Rec.Overdraft = Data.Rec.Overdraft;
								break;
							case DSF_CRRACCOUNT_CURLIST:
								pack.CurList = Data.CurList;
								break;
							case DSF_CRRACCOUNT_ACCSHEET:
								pack.Rec.AccSheetID = Data.Rec.AccSheetID;
								break;
							case DSF_CRRACCOUNT_FRRL_DATE:
								pack.Rec.Frrl_Date = Data.Rec.Frrl_Date;
								break;
							case DSF_CRRACCOUNT_FFREEREST:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_FREEREST, Data.Rec.Flags);
								break;
							case DSF_CRRACCOUNT_FHASBRANCH:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_HASBRANCH, Data.Rec.Flags);
								break;
							case DSF_CRRACCOUNT_FCURRENCY:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_CURRENCY, Data.Rec.Flags);
								break;
							case DSF_CRRACCOUNT_FFRRL:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_FRRL, Data.Rec.Flags);
								break;
							case DSF_CRRACCOUNT_FEXCLINNERTRNOVR:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_EXCLINNERTRNOVR, Data.Rec.Flags);
								break;
							case DSF_CRRACCOUNT_FSYSNUMBER:
								SETFLAGBYSAMPLE(pack.Rec.Flags, ACF_SYSNUMBER, Data.Rec.Flags);
								break;
						}
					}
					ok = Obj.PutPacket(&id, &pack, 0);
				}
			}
			else {
				ok = Obj.PutPacket(&id, &Data, 0);
				Data.Rec.ID = id;
			}
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrAccount::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRACCOUNT_CURR) {
			PPCurrency item = ((PPDS_CrrCurrency *)pData)->Data;
			Data.Rec.CurID = item.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRACCOUNT_CURLIST) {
			PPCurrency item = ((PPDS_CrrCurrency *)pData)->Data;
			if(Data.CurList.insert(&item))
				ok = 1;
			else
				ok = 0;
		}
		else if(fldID == DSF_CRRACCOUNT_ACCSHEET) {
			PPAccSheet item = ((PPDS_CrrAccSheet *)pData)->Data;
			Data.Rec.AccSheetID = item.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrAccount::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRACCOUNT_CURR) {
		if(*pIter == 0 && Data.Rec.CurID) {
			pData->InitData(idoExtract, 0, Data.Rec.CurID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRACCOUNT_CURLIST) {
		if(*pIter < Data.CurList.getCount()) {
			pData->InitData(idoExtract, 0, Data.CurList.at(*pIter));
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRACCOUNT_ACCSHEET) {
		if(*pIter == 0 && Data.Rec.AccSheetID) {
			pData->InitData(idoExtract, 0, Data.Rec.AccSheetID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrAccount::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRACCOUNT_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.Rec.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRACCOUNT_AC:
			ok = TransferData(&Data.Rec.A.Ac, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_SB:
			ok = TransferData(&Data.Rec.A.Sb, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_CODE:
			ok = TransferData(Data.Rec.Code, sizeof(Data.Rec.Code), dir, rBuf);
			break;
		case DSF_CRRACCOUNT_NAME:
			ok = TransferData(Data.Rec.Name, sizeof(Data.Rec.Name), dir, rBuf);
			break;
		case DSF_CRRACCOUNT_OPENDATE:
			ok = TransferData(&Data.Rec.OpenDate, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_TYPE:
			{
				const char * p_list = "ACY_BAL;ACY_OBAL;ACY_AGGR;ACY_REGISTER;ACY_PERSONAL;ACY_ALIAS";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Rec.Type - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Rec.Type = idx + 1;
					else
						Data.Rec.Type = 0;
				}
			}
			break;
		case DSF_CRRACCOUNT_KIND:
			{
				const char * p_list = "ACT_ACTIVE;ACT_PASSIVE;ACT_AP";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.Rec.Kind - 1);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.Rec.Kind = idx + 1;
					else
						Data.Rec.Kind = 0;
				}
			}
			break;
		case DSF_CRRACCOUNT_LIMIT:
			ok = TransferData(&Data.Rec.Limit, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_OVERDRAFT:
			ok = TransferData(&Data.Rec.Overdraft, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_FRRL_DATE:
			ok = TransferData(&Data.Rec.Frrl_Date, dir, rBuf);
			break;
		case DSF_CRRACCOUNT_FFREEREST:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_FREEREST, dir, rBuf); break;
			break;
		case DSF_CRRACCOUNT_FHASBRANCH:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_HASBRANCH, dir, rBuf); break;
			break;
		case DSF_CRRACCOUNT_FCURRENCY:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_CURRENCY, dir, rBuf); break;
			break;
		case DSF_CRRACCOUNT_FFRRL:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_FRRL, dir, rBuf); break;
			break;
		case DSF_CRRACCOUNT_FEXCLINNERTRNOVR:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_EXCLINNERTRNOVR, dir, rBuf); break;
			break;
		case DSF_CRRACCOUNT_FSYSNUMBER:
			ok = TransferDataFlag(&Data.Rec.Flags, ACF_SYSNUMBER, dir, rBuf); break;
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrAcctEntry
//
int SLAPI PPDS_CrrAcctEntry::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(AcctID*)dataPtr;
	}
	else if(op == idoAccept) {
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrAcctEntry::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRACCTENTRY_AR) {
			ArticleTbl::Rec rec = ((PPDS_CrrArticle *)pData)->Data;
			Data.ar = rec.ID;
			ok = 1;
		}
		else if(fldID == DSF_CRRACCTENTRY_ACC) {
			PPAccountPacket pack = ((PPDS_CrrAccount *)pData)->Data;
			Data.ac = pack.Rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrAcctEntry::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRACCTENTRY_AR) {
			if(*pIter == 0 && Data.ar) {
				pData->InitData(idoExtract, 0, Data.ar);
				ok = 1;
			}
		}
		else if(fldID == DSF_CRRACCTENTRY_ACC) {
			if(*pIter == 0 && Data.ac) {
				pData->InitData(idoExtract, 0, Data.ac);
				ok = 1;
			}
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrAccturnTempl
//
int SLAPI PPDS_CrrAccturnTempl::InitData(Ido op, void * dataPtr, long /*addedParam*/)
{
	int    ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPAccTurnTempl*)dataPtr;
	}
	else if(op == idoAccept) {
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	return ok;
}

int SLAPI PPDS_CrrAccturnTempl::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRRACCTURNTEMPL_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRACCTURNTEMPL_EXPR:
			ok = TransferData(Data.Expr, sizeof(Data.Expr), dir, rBuf);
			break;
		case DSF_CRRACCTURNTEMPL_SUBST:
			{
				TempBuf = 0;
				if(dir == tfdDataToBuf) {
					if((*pIter) < 8)
						ok = -1;
					else
						ok = -1;
				}
				else if(dir == tfdBufToData)
					ok = -1;
			}
			break;
		case DSF_CRRACCTURNTEMPL_PERIOD:
			ok = TransferData(&Data.Period, dir, rBuf);
			break;
		case DSF_CRRACCTURNTEMPL_FDACCFIX:
			ok = TransferDataFlag(&Data.Flags, ATTF_DACCFIX, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FDARTFIX:
			ok = TransferDataFlag(&Data.Flags, ATTF_DARTFIX, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FCACCFIX:
			ok = TransferDataFlag(&Data.Flags, ATTF_CACCFIX, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FCARFIX:
			ok = TransferDataFlag(&Data.Flags, ATTF_CARTFIX, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FPRIMONCREDIT:
			ok = TransferDataFlag(&Data.Flags, ATTF_PRIMONCREDIT, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FEXPRESSION:
			ok = TransferDataFlag(&Data.Flags, ATTF_EXPRESSION, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FPSKIPONZOBJ:
			ok = TransferDataFlag(&Data.Flags, ATTF_PSKIPONZOBJ, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FFSKIPONZOBJ:
			ok = TransferDataFlag(&Data.Flags, ATTF_FSKIPONZOBJ, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FPSUBSTRULE:
			ok = TransferDataFlag(&Data.Flags, ATTF_PSUBSTRULE, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FFSUBSTRULE:
			ok = TransferDataFlag(&Data.Flags, ATTF_FSUBSTRULE, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FSKIPNEG:
			ok = TransferDataFlag(&Data.Flags, ATTF_SKIPNEG, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FINVERTNEG:
			ok = TransferDataFlag(&Data.Flags, ATTF_INVERTNEG, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FBASEPROJECTION:
			ok = TransferDataFlag(&Data.Flags, ATTF_BASEPROJECTION, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FINTROUNDING:
			ok = TransferDataFlag(&Data.Flags, ATTF_INTROUNDING, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FPASSIVE:
			ok = TransferDataFlag(&Data.Flags, ATTF_PASSIVE, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FSUBSTDACC:
			ok = TransferDataFlag(&Data.Flags, ATTF_SUBSTDACC, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FSUBSTCACC:
			ok = TransferDataFlag(&Data.Flags, ATTF_SUBSTCACC, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FBYADVLINES:
			ok = TransferDataFlag(&Data.Flags, ATTF_BYADVLINES, dir, rBuf); break;
		case DSF_CRRACCTURNTEMPL_FSKIPEMPTYALIAS:
			ok = TransferDataFlag(&Data.Flags, ATTF_SKIPEMPTYALIAS, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrAccturnTempl::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(oneof2(fldID, DSF_CRRACCTURNTEMPL_DBT, DSF_CRRACCTURNTEMPL_CRD)) {
			AcctID item = ((PPDS_CrrAcctEntry *)pData)->Data;
			if(fldID == DSF_CRRACCTURNTEMPL_DBT)
				Data.DbtID = item;
			else
				Data.CrdID = item;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrAccturnTempl::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRACCTURNTEMPL_DBT) {
		if(*pIter == 0 && (Data.DbtID.ar || Data.DbtID.ac)) {
			pData->InitData(idoExtract, &Data.DbtID, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRACCTURNTEMPL_CRD) {
		if(*pIter == 0 && (Data.CrdID.ar || Data.CrdID.ac)) {
			pData->InitData(idoExtract, &Data.CrdID, 0);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrOprKindEntry
//
int SLAPI PPDS_CrrOprKindEntry::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
		Pack.Init();
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Pack = *(PPOprKindPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Pack) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
		if(ok > 0)
			Data = Pack.Rec;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		PPOprKindPacket pack;
		Pack.Rec = Data;
		if(*strip(Data.Symb) && Obj.SearchBySymb(Data.Symb, &id) > 0) {
			Data.ID = id;
			if(UpdateProtocol == updForce) {
				PPOprKindPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRROPRKINDENTRY_ID:
							break;
						case DSF_CRROPRKINDENTRY_NAME:
							STRNSCPY(pack.Rec.Name, Data.Name);
							break;
						case DSF_CRROPRKINDENTRY_SYMB:
							STRNSCPY(pack.Rec.Symb, Data.Symb);
							break;
						case DSF_CRROPRKINDENTRY_RANK:
							pack.Rec.Rank = Data.Rank;
							break;
						case DSF_CRROPRKINDENTRY_LINKOPSYMB:
							pack.Rec.LinkOpID = Data.LinkOpID;
							break;
						case DSF_CRROPRKINDENTRY_PRNORDER:
							pack.Rec.PrnOrder = Data.PrnOrder;
							break;
						case DSF_CRROPRKINDENTRY_SUBTYPE:
							pack.Rec.SubType = Data.SubType;
							break;
						case DSF_CRROPRKINDENTRY_OPTYPEID:
							pack.Rec.OpTypeID = Data.OpTypeID;
							break;
						case DSF_CRROPRKINDENTRY_ACCSHEET:
							pack.Rec.AccSheetID = Data.AccSheetID;
							break;
						case DSF_CRROPRKINDENTRY_ACCSHEET2:
							pack.Rec.AccSheet2ID = Data.AccSheet2ID;
							break;
						case DSF_CRROPRKINDENTRY_OPKFPRTINCINVC:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_INCINVC, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTNEGINVC:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_NEGINVC, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTCHECK:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_CHECK, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTCHECKTI:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_CHECKTI, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTSRVACT:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_SRVACT, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTBUYING:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_BUYING, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTSELLING:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_SELLING, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTEXTOBJ2OBJ:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_EXTOBJ2OBJ, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTTARESALDO:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_TARESALDO, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTQCERT:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_QCERT, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTNBILLN:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_NBILLN, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTVATAX:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_VATAX, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTINVOICE:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_INVOICE, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTQCG:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_QCG, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTSHRTORG:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_SHRTORG, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTCASHORD:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_CASHORD, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTSELPRICE:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_SELPRICE, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTNDISCNT:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_NDISCNT, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTPAYPLAN:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_PAYPLAN, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTLADING:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_LADING, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTMERGETI:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_MERGETI, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTPLABEL:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_PLABEL, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTBCODELIST:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_BCODELIST, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFPRTQCERTLIST:
							SETFLAGBYSAMPLE(pack.Rec.PrnFlags, OPKF_PRT_QCERTLIST, Data.PrnFlags); break;
						case DSF_CRROPRKINDENTRY_OPKFNEEDPAYMENT:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_NEEDPAYMENT, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFGRECEIPT:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_GRECEIPT, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFGEXPEND:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_GEXPEND, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFBUYING:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_BUYING, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFSELLING:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_SELLING, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFNOUPDLOTREST:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_NOUPDLOTREST, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFADVACC:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ADVACC, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFPROFITABLE:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_PROFITABLE, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFONORDER:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ONORDER, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFFREIGHT:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_FREIGHT, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFPCKGMOUNTING:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_PCKGMOUNTING, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFORDEXSTONLY:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ORDEXSTONLY, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFORDRESERVE:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ORDRESERVE, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFCALCSTAXES:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_CALCSTAXES, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFCHARGENEGPAYM:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_CHARGENEGPAYM, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFAUTOWL:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_AUTOWL, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFATTACHFILES:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ATTACHFILES, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFUSEPAYER:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_USEPAYER, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFORDERBYLOC:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_ORDERBYLOC, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFNEEDVALUATION:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_NEEDVALUATION, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFOUTBALACCTURN:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_OUTBALACCTURN, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFEXTACCTURN:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_EXTACCTURN, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFEXTAMTLIST:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_EXTAMTLIST, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFDENYREVALCOST:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_DENYREVALCOST, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFRENT:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_RENT, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFNEEDACK:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_NEEDACK, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFNOCALCTIORD:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_NOCALCTIORD, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFRECKON:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_RECKON, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFBANKING:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_BANKING, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFPASSIVE:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_PASSIVE, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFCURTRANSIT:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_CURTRANSIT, Data.Flags); break;
						case DSF_CRROPRKINDENTRY_OPKFRESTRICTBYMTX:
							SETFLAGBYSAMPLE(pack.Rec.Flags, OPKF_RESTRICTBYMTX, Data.Flags); break;
					}
				}
				// ok = Obj.PutPacket(&id, &pack, 0);
				Data = pack.Rec;
			}
		}
		else
			;// ok = Obj.PutPacket(&id, &Pack, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrOprKindEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int    ok = -1;
	switch(fldID) {
		case DSF_CRROPRKINDENTRY_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRROPRKINDENTRY_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRROPRKINDENTRY_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRROPRKINDENTRY_RANK:
			ok = TransferData(&Data.Rank, dir, rBuf);
			break;
		case DSF_CRROPRKINDENTRY_LINKOPSYMB:
			{
				PPOprKind linkop_rec;
				MEMSZERO(linkop_rec);
				if(dir == tfdDataToBuf) {
					if(Obj.Search(Data.LinkOpID, &linkop_rec) > 0)
						ok = TransferData(linkop_rec.Symb, sizeof(linkop_rec.Symb), dir, rBuf);
				}
				else
					ok = (rBuf.Len()) ? Obj.SearchBySymb(rBuf, &Data.LinkOpID) : -1;
			}
			break;
		case DSF_CRROPRKINDENTRY_PRNORDER:
			ok = TransferData(&Data.PrnOrder, dir, rBuf);
			break;
		case DSF_CRROPRKINDENTRY_SUBTYPE:
			{
				const char * p_list = "COMMON;ADVANCEREP;REGISTER;ASSETRCV;ASSETEXPL;WARRANT;ASSETMODIF;DEBTINVENT;TRADEPLAN;ACCWROFF";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.SubType);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.SubType = idx;
					else
						Data.SubType = -1;
				}
			}
			break;
		case DSF_CRROPRKINDENTRY_OPTYPEID:
			ok = TransferData(&Data.OpTypeID, dir, rBuf);
			break;
		case DSF_CRROPRKINDENTRY_OPKFPRTINCINVC:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_INCINVC, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTNEGINVC:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_NEGINVC, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTCHECK:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_CHECK, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTCHECKTI:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_CHECKTI, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTSRVACT:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_SRVACT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTBUYING:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_BUYING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTSELLING:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_SELLING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTEXTOBJ2OBJ:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_EXTOBJ2OBJ, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTTARESALDO:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_TARESALDO, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTQCERT:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_QCERT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTNBILLN:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_NBILLN, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTVATAX:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_VATAX, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTINVOICE:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_INVOICE, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTQCG:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_QCG, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTSHRTORG:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_SHRTORG, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTCASHORD:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_CASHORD, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTSELPRICE:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_SELPRICE, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTNDISCNT:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_NDISCNT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTPAYPLAN:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_PAYPLAN, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTLADING:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_LADING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTMERGETI:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_MERGETI, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTPLABEL:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_PLABEL, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTBCODELIST:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_BCODELIST, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPRTQCERTLIST:
			ok = TransferDataFlag(&Data.PrnFlags, OPKF_PRT_QCERTLIST, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFNEEDPAYMENT:
			ok = TransferDataFlag(&Data.Flags, OPKF_NEEDPAYMENT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFGRECEIPT:
			ok = TransferDataFlag(&Data.Flags, OPKF_GRECEIPT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFGEXPEND:
			ok = TransferDataFlag(&Data.Flags, OPKF_GEXPEND, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFBUYING:
			ok = TransferDataFlag(&Data.Flags, OPKF_BUYING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFSELLING:
			ok = TransferDataFlag(&Data.Flags, OPKF_SELLING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFNOUPDLOTREST:
			ok = TransferDataFlag(&Data.Flags, OPKF_NOUPDLOTREST, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFADVACC:
			ok = TransferDataFlag(&Data.Flags, OPKF_ADVACC, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPROFITABLE:
			ok = TransferDataFlag(&Data.Flags, OPKF_PROFITABLE, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFONORDER:
			ok = TransferDataFlag(&Data.Flags, OPKF_ONORDER, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFFREIGHT:
			ok = TransferDataFlag(&Data.Flags, OPKF_FREIGHT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPCKGMOUNTING:
			ok = TransferDataFlag(&Data.Flags, OPKF_PCKGMOUNTING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFORDEXSTONLY:
			ok = TransferDataFlag(&Data.Flags, OPKF_ORDEXSTONLY, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFORDRESERVE:
			ok = TransferDataFlag(&Data.Flags, OPKF_ORDRESERVE, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFCALCSTAXES:
			ok = TransferDataFlag(&Data.Flags, OPKF_CALCSTAXES, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFCHARGENEGPAYM:
			ok = TransferDataFlag(&Data.Flags, OPKF_CHARGENEGPAYM, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFAUTOWL:
			ok = TransferDataFlag(&Data.Flags, OPKF_AUTOWL, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFATTACHFILES:
			ok = TransferDataFlag(&Data.Flags, OPKF_ATTACHFILES, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFUSEPAYER:
			ok = TransferDataFlag(&Data.Flags, OPKF_USEPAYER, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFORDERBYLOC:
			ok = TransferDataFlag(&Data.Flags, OPKF_ORDERBYLOC, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFNEEDVALUATION:
			ok = TransferDataFlag(&Data.Flags, OPKF_NEEDVALUATION, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFOUTBALACCTURN:
			ok = TransferDataFlag(&Data.Flags, OPKF_OUTBALACCTURN, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFEXTACCTURN:
			ok = TransferDataFlag(&Data.Flags, OPKF_EXTACCTURN, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFEXTAMTLIST:
			ok = TransferDataFlag(&Data.Flags, OPKF_EXTAMTLIST, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFDENYREVALCOST:
			ok = TransferDataFlag(&Data.Flags, OPKF_DENYREVALCOST, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFRENT:
			ok = TransferDataFlag(&Data.Flags, OPKF_RENT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFNEEDACK:
			ok = TransferDataFlag(&Data.Flags, OPKF_NEEDACK, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFNOCALCTIORD:
			ok = TransferDataFlag(&Data.Flags, OPKF_NOCALCTIORD, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFRECKON:
			ok = TransferDataFlag(&Data.Flags, OPKF_RECKON, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFBANKING:
			ok = TransferDataFlag(&Data.Flags, OPKF_BANKING, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFPASSIVE:
			ok = TransferDataFlag(&Data.Flags, OPKF_PASSIVE, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFCURTRANSIT:
			ok = TransferDataFlag(&Data.Flags, OPKF_CURTRANSIT, dir, rBuf); break;
		case DSF_CRROPRKINDENTRY_OPKFRESTRICTBYMTX:
			ok = TransferDataFlag(&Data.Flags, OPKF_RESTRICTBYMTX, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrOprKindEntry::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(oneof2(fldID, DSF_CRROPRKINDENTRY_ACCSHEET, DSF_CRROPRKINDENTRY_ACCSHEET2)) {
			PPAccSheet item = ((PPDS_CrrAccSheet *)pData)->Data;
			if(fldID == DSF_CRROPRKINDENTRY_ACCSHEET)
				Data.AccSheetID = item.ID;
			else
				Data.AccSheet2ID = item.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrOprKindEntry::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRROPRKINDENTRY_ACCSHEET) {
		if(*pIter == 0 && Data.AccSheetID) {
			pData->InitData(idoExtract, 0, Data.AccSheetID);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKINDENTRY_ACCSHEET2) {
		if(*pIter == 0 && Data.AccSheet2ID) {
			pData->InitData(idoExtract, 0, Data.AccSheet2ID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrInvOpExEntry
//
int SLAPI PPDS_CrrInvOpExEntry::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
		Pack.Init();
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Pack = *(PPOprKindPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Pack) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
		if(ok > 0) {
			if(Pack.P_IOE)
				Data = *Pack.P_IOE;
			else
				ok = -1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		PPOprKindPacket pack;
		if(!Pack.P_IOE)
			THROW_MEM(Pack.P_IOE = new PPInventoryOpEx);
		ASSIGN_PTR(Pack.P_IOE, Data);
		if(*strip(Pack.Rec.Symb) && Obj.SearchBySymb(Pack.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				PPOprKindPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				if(!pack.P_IOE)
					THROW_MEM(pack.P_IOE = new PPInventoryOpEx);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRINVOPEXENTRY_ID:
							break;
						case DSF_CRRINVOPEXENTRY_WRDNOPSYMB:
							pack.P_IOE->WrDnOp = Data.WrDnOp;
							break;
						case DSF_CRRINVOPEXENTRY_WRUPOPSYMB:
							pack.P_IOE->WrUpOp = Data.WrUpOp;
							break;
						case DSF_CRRINVOPEXENTRY_AMOUNTCALCMETHOD:
					  		pack.P_IOE->AmountCalcMethod = Data.AmountCalcMethod;
							break;
						case DSF_CRRINVOPEXENTRY_AUTOFILLMETHOD:
							pack.P_IOE->AutoFillMethod = Data.AutoFillMethod;
							break;
						case DSF_CRRINVOPEXENTRY_WRDNOBJ:
							pack.P_IOE->WrDnObj = Data.WrDnObj;
							break;
						case DSF_CRRINVOPEXENTRY_WRUPOBJ:
							pack.P_IOE->WrUpObj = Data.WrUpObj;
							break;
						case DSF_CRRINVOPEXENTRY_FCOSTNOMINAL:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_COSTNOMINAL, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FZERODEFAULT:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_ZERODEFAULT, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FWROFFWODSCNT:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_WROFFWODSCNT, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FUSEPACKS:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_USEPACKS, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FSELGOODSBYNAME:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_SELGOODSBYNAME, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FUSEANOTERLOCLOTS:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_USEANOTERLOCLOTS, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FINVBYCLIENT:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_INVBYCLIENT, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FACCELADDITEMS:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_ACCELADDITEMS, Data.Flags); break;
						case DSF_CRRINVOPEXENTRY_FASSET:
							SETFLAGBYSAMPLE(pack.P_IOE->Flags, INVOPF_ASSET, Data.Flags); break;
					}
				}
				// ok = Obj.PutPacket(&id, &pack, 0);
				Data = *pack.P_IOE;
			}
		}
		else
			; // ok = Obj.PutPacket(&id, &Pack, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrInvOpExEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRINVOPEXENTRY_ID:
			if(dir == tfdDataToBuf) {
				PPID id = Data.ID;
				ok = TransferData(&id, dir, rBuf);
			}
			break;
		case DSF_CRRINVOPEXENTRY_WRDNOPSYMB:
			{
				PPOprKind op_rec;
				MEMSZERO(op_rec);
				if(dir == tfdDataToBuf) {
					if(Obj.Search(Data.WrDnOp, &op_rec) > 0)
						ok = TransferData(op_rec.Symb, sizeof(op_rec.Symb), dir, rBuf);
				}
				else
					ok = (rBuf.Len()) ? Obj.SearchBySymb(rBuf, &Data.WrDnOp) : -1;
			}
			break;
		case DSF_CRRINVOPEXENTRY_WRUPOPSYMB:
			{
				PPOprKind op_rec;
				MEMSZERO(op_rec);
				if(dir == tfdDataToBuf) {
					if(Obj.Search(Data.WrUpOp, &op_rec) > 0)
						ok = TransferData(op_rec.Symb, sizeof(op_rec.Symb), dir, rBuf);
				}
				else
					ok = (rBuf.Len()) ? Obj.SearchBySymb(rBuf, &Data.WrUpOp) : -1;
			}
			break;
		case DSF_CRRINVOPEXENTRY_AMOUNTCALCMETHOD:
			{
				const char * p_list = "LIFO;FIFO;AVG";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.AmountCalcMethod);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.AmountCalcMethod = idx;
					else
						Data.AmountCalcMethod = -1;
				}
			}
			break;
		case DSF_CRRINVOPEXENTRY_AUTOFILLMETHOD:
			{
				const char * p_list = "PRESENTS;ALL;PREVLIFO;BYCURLOTREST";
				if(dir == tfdDataToBuf)
					TempBuf.GetSubFrom(p_list, ';', Data.AutoFillMethod);
				ok = TransferData(TempBuf, dir, rBuf);
				if(dir == tfdBufToData) {
					int idx = 0;
					if(PPSearchSubStr(p_list, &idx, TempBuf, 1))
						Data.AutoFillMethod = idx;
					else
						Data.AutoFillMethod = -1;
				}
			}
			break;
		case DSF_CRRINVOPEXENTRY_FCOSTNOMINAL:
			ok = TransferDataFlag(&Data.Flags, INVOPF_COSTNOMINAL, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FZERODEFAULT:
			ok = TransferDataFlag(&Data.Flags, INVOPF_ZERODEFAULT, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FWROFFWODSCNT:
			ok = TransferDataFlag(&Data.Flags, INVOPF_WROFFWODSCNT, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FUSEPACKS:
			ok = TransferDataFlag(&Data.Flags, INVOPF_USEPACKS, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FSELGOODSBYNAME:
			ok = TransferDataFlag(&Data.Flags, INVOPF_SELGOODSBYNAME, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FUSEANOTERLOCLOTS:
			ok = TransferDataFlag(&Data.Flags, INVOPF_USEANOTERLOCLOTS, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FINVBYCLIENT:
			ok = TransferDataFlag(&Data.Flags, INVOPF_INVBYCLIENT, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FACCELADDITEMS:
			ok = TransferDataFlag(&Data.Flags, INVOPF_ACCELADDITEMS, dir, rBuf); break;
		case DSF_CRRINVOPEXENTRY_FASSET:
			ok = TransferDataFlag(&Data.Flags, INVOPF_ASSET, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrInvOpExEntry::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(oneof2(fldID, DSF_CRRINVOPEXENTRY_WRDNOBJ, DSF_CRRINVOPEXENTRY_WRUPOBJ)) {
			ArticleTbl::Rec rec = ((PPDS_CrrArticle*)pData)->Data;
			if(fldID == DSF_CRRINVOPEXENTRY_WRDNOBJ)
				Data.WrDnObj = rec.ID;
			else
				Data.WrUpObj = rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrInvOpExEntry::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRINVOPEXENTRY_WRDNOBJ) {
		if(*pIter == 0 && Data.WrDnObj) {
			pData->InitData(idoExtract, 0, Data.WrDnObj);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRRINVOPEXENTRY_WRUPOBJ) {
		if(*pIter == 0 && Data.WrUpObj) {
			pData->InitData(idoExtract, 0, Data.WrUpObj);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrReckonOpExEntry
//
int SLAPI PPDS_CrrReckonOpExEntry::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Pack = *(PPOprKindPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Pack) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
		if(ok > 0) {
			if(Pack.P_ReckonData)
				Data = *Pack.P_ReckonData;
			else
				ok = -1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		PPOprKindPacket pack;
		if(!Pack.P_ReckonData)
			THROW_MEM(Pack.P_ReckonData = new PPReckonOpEx);
		ASSIGN_PTR(Pack.P_ReckonData, Data);
		if(*strip(Pack.Rec.Symb) && Obj.SearchBySymb(Pack.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				PPOprKindPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				if(!pack.P_ReckonData)
					THROW_MEM(pack.P_ReckonData = new PPReckonOpEx);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRRECKONOPEXENTRY_BEG:
							pack.P_ReckonData->Beg = Data.Beg;
							break;
						case DSF_CRRRECKONOPEXENTRY_END:
							pack.P_ReckonData->End = Data.End;
							break;
						case DSF_CRRRECKONOPEXENTRY_OPSYMBLIST:
							break;
						case DSF_CRRRECKONOPEXENTRY_PERSONRELTYPE:
							pack.P_ReckonData->PersonRelTypeID = Data.PersonRelTypeID;
							break;
						case DSF_CRRRECKONOPEXENTRY_FBEGISBILLDT:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_BEGISBILLDT, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FENDISBILLDT:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_ENDISBILLDT, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FAUTOPAYM:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_AUTOPAYM, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FCFMPAYM:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_CFM_PAYM, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FAUTODEBT:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_AUTODEBT, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FCFMDEBT:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_CFM_DEBT, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FTHISLOCONLY:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_THISLOCONLY, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FBYEXTOBJ:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_BYEXTOBJ, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FREQALTOBJ:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_REQALTOBJ, Data.Flags); break;
						case DSF_CRRRECKONOPEXENTRY_FTHISALTOBJONLY:
							SETFLAGBYSAMPLE(pack.P_ReckonData->Flags, ROXF_THISALTOBJONLY, Data.Flags); break;
					}
				}
				// ok = Obj.PutPacket(&id, &pack, 0);
				Data = *pack.P_ReckonData;
			}
		}
		else
			; // ok = Obj.PutPacket(&id, &Pack, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrReckonOpExEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRRECKONOPEXENTRY_BEG:
			ok = TransferData(&Data.Beg, dir, rBuf); break;
			break;
		case DSF_CRRRECKONOPEXENTRY_END:
			ok = TransferData(&Data.End, dir, rBuf); break;
			break;
		case DSF_CRRRECKONOPEXENTRY_OPSYMBLIST:
			break;
		case DSF_CRRRECKONOPEXENTRY_FBEGISBILLDT:
			ok = TransferDataFlag(&Data.Flags, ROXF_BEGISBILLDT, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FENDISBILLDT:
			ok = TransferDataFlag(&Data.Flags, ROXF_ENDISBILLDT, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FAUTOPAYM:
			ok = TransferDataFlag(&Data.Flags, ROXF_AUTOPAYM, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FCFMPAYM:
			ok = TransferDataFlag(&Data.Flags, ROXF_CFM_PAYM, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FAUTODEBT:
			ok = TransferDataFlag(&Data.Flags, ROXF_AUTODEBT, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FCFMDEBT:
			ok = TransferDataFlag(&Data.Flags, ROXF_CFM_DEBT, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FTHISLOCONLY:
			ok = TransferDataFlag(&Data.Flags, ROXF_THISLOCONLY, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FBYEXTOBJ:
			ok = TransferDataFlag(&Data.Flags, ROXF_BYEXTOBJ, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FREQALTOBJ:
			ok = TransferDataFlag(&Data.Flags, ROXF_REQALTOBJ, dir, rBuf); break;
		case DSF_CRRRECKONOPEXENTRY_FTHISALTOBJONLY:
			ok = TransferDataFlag(&Data.Flags, ROXF_THISALTOBJONLY, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrReckonOpExEntry::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRRECKONOPEXENTRY_PERSONRELTYPE) {
			PPPersonRelTypePacket pack = ((PPDS_CrrPersonRelType*)pData)->Data;
			Data.PersonRelTypeID = pack.Rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrReckonOpExEntry::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRRECKONOPEXENTRY_PERSONRELTYPE) {
		if(*pIter == 0 && Data.PersonRelTypeID) {
			pData->InitData(idoExtract, 0, Data.PersonRelTypeID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrDraftOpExEntry
//
int SLAPI PPDS_CrrDraftOpExEntry::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Pack = *(PPOprKindPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Pack) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
		if(ok > 0) {
			if(Pack.P_DraftData)
				Data = *Pack.P_DraftData;
			else
				ok = -1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		PPOprKindPacket pack;
		if(!Pack.P_DraftData)
			THROW_MEM(Pack.P_DraftData = new PPDraftOpEx);
		ASSIGN_PTR(Pack.P_DraftData, Data);
		if(*strip(Pack.Rec.Symb) && Obj.SearchBySymb(Pack.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				PPOprKindPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				if(!pack.P_DraftData)
					THROW_MEM(pack.P_DraftData = new PPDraftOpEx);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRDRAFTOPEXENTRY_WROFFOP:
							pack.P_DraftData->WrOffOpID = Data.WrOffOpID;
							break;
						case DSF_CRRDRAFTOPEXENTRY_WROFFCOMPLOP:
							pack.P_DraftData->WrOffComplOpID = Data.WrOffComplOpID;
							break;
						case DSF_CRRDRAFTOPEXENTRY_WROFFOBJ:
							pack.P_DraftData->WrOffObjID = Data.WrOffObjID;
							break;
						case DSF_CRRDRAFTOPEXENTRY_FCREMPTYBILL:
							SETFLAGBYSAMPLE(pack.P_DraftData->Flags, DROXF_CREMPTYBILL, Data.Flags); break;
						case DSF_CRRDRAFTOPEXENTRY_FUSEPARTSTRUC:
							SETFLAGBYSAMPLE(pack.P_DraftData->Flags, DROXF_USEPARTSTRUC, Data.Flags); break;
						case DSF_CRRDRAFTOPEXENTRY_FWROFFCURDATE:
							SETFLAGBYSAMPLE(pack.P_DraftData->Flags, DROXF_WROFFCURDATE, Data.Flags); break;
						case DSF_CRRDRAFTOPEXENTRY_FDONTINHEXPIRY:
							SETFLAGBYSAMPLE(pack.P_DraftData->Flags, DROXF_DONTINHEXPIRY, Data.Flags); break;
					}
				}
				// ok = Obj.PutPacket(&id, &pack, 0);
				Data = *pack.P_DraftData;
			}
		}
		else
			; // ok = Obj.PutPacket(&id, &Pack, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrDraftOpExEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRDRAFTOPEXENTRY_WROFFOP:
			{
				PPOprKind op_rec;
				MEMSZERO(op_rec);
				if(dir == tfdDataToBuf) {
					if(Obj.Search(Data.WrOffOpID, &op_rec) > 0)
						ok = TransferData(op_rec.Symb, sizeof(op_rec.Symb), dir, rBuf);
				}
				else
					ok = (rBuf.Len()) ? Obj.SearchBySymb(rBuf, &Data.WrOffOpID) : -1;
			}
			break;
		case DSF_CRRDRAFTOPEXENTRY_WROFFCOMPLOP:
			{
				PPOprKind op_rec;
				MEMSZERO(op_rec);
				if(dir == tfdDataToBuf) {
					if(Obj.Search(Data.WrOffComplOpID, &op_rec) > 0)
						ok = TransferData(op_rec.Symb, sizeof(op_rec.Symb), dir, rBuf);
				}
				else
					ok = (rBuf.Len()) ? Obj.SearchBySymb(rBuf, &Data.WrOffComplOpID) : -1;
			}
			break;
		case DSF_CRRDRAFTOPEXENTRY_FCREMPTYBILL:
			ok = TransferDataFlag(&Data.Flags, DROXF_CREMPTYBILL, dir, rBuf); break;
		case DSF_CRRDRAFTOPEXENTRY_FUSEPARTSTRUC:
			ok = TransferDataFlag(&Data.Flags, DROXF_USEPARTSTRUC, dir, rBuf); break;
		case DSF_CRRDRAFTOPEXENTRY_FWROFFCURDATE:
			ok = TransferDataFlag(&Data.Flags, DROXF_WROFFCURDATE, dir, rBuf); break;
		case DSF_CRRDRAFTOPEXENTRY_FDONTINHEXPIRY:
			ok = TransferDataFlag(&Data.Flags, DROXF_DONTINHEXPIRY, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrDraftOpExEntry::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int    ok = -1;
	if(pData) {
		if(fldID == DSF_CRRDRAFTOPEXENTRY_WROFFOBJ) {
			ArticleTbl::Rec rec = ((PPDS_CrrArticle*)pData)->Data;
			Data.WrOffObjID = rec.ID;
			ok = 1;
		}
	}
	return ok;
}

int SLAPI PPDS_CrrDraftOpExEntry::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRRDRAFTOPEXENTRY_WROFFOBJ) {
		if(*pIter == 0 && Data.WrOffObjID) {
			pData->InitData(idoExtract, 0, Data.WrOffObjID);
			ok = 1;
		}
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrBillPoolOpExEntry
//
int SLAPI PPDS_CrrBillPoolOpExEntry::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr) {
			Pack = *(PPOprKindPacket*)dataPtr;
		}
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Pack) > 0) {
				ok = 1;
			}
			else
				ok = -1;
		}
		else
			ok = -1;
		if(ok > 0) {
			if(Pack.P_PoolData)
				Data = *Pack.P_PoolData;
			else
				ok = -1;
		}
	}
	else if(op == idoAccept) {
		PPID id = 0;
		PPOprKindPacket pack;
		if(!Pack.P_PoolData)
			THROW_MEM(Pack.P_PoolData = new PPBillPoolOpEx);
		ASSIGN_PTR(Pack.P_PoolData, Data);
		if(*strip(Pack.Rec.Symb) && Obj.SearchBySymb(Pack.Rec.Symb, &id) > 0) {
			if(UpdateProtocol == updForce) {
				PPOprKindPacket pack;
				THROW(Obj.GetPacket(id, &pack) > 0);
				if(!pack.P_PoolData)
					THROW_MEM(pack.P_PoolData = new PPBillPoolOpEx);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRBILLPOOLOPEXENTRY_FONEOP:
							SETFLAGBYSAMPLE(pack.P_PoolData->Flags, BPOXF_ONEOP, Data.Flags); break;
						case DSF_CRRBILLPOOLOPEXENTRY_FONEDATE:
							SETFLAGBYSAMPLE(pack.P_PoolData->Flags, BPOXF_ONEDATE, Data.Flags); break;
						case DSF_CRRBILLPOOLOPEXENTRY_FONEOBJECT:
							SETFLAGBYSAMPLE(pack.P_PoolData->Flags, BPOXF_ONEOBJECT, Data.Flags); break;
						case DSF_CRRBILLPOOLOPEXENTRY_FUNITEACCTURNS:
							SETFLAGBYSAMPLE(pack.P_PoolData->Flags, BPOXF_UNITEACCTURNS, Data.Flags); break;
						case DSF_CRRBILLPOOLOPEXENTRY_FUNITEPAYMENTS:
							SETFLAGBYSAMPLE(pack.P_PoolData->Flags, BPOXF_UNITEPAYMENTS, Data.Flags); break;
					}
				}
				// ok = Obj.PutPacket(&id, &pack, 0);
				Data = *pack.P_PoolData;
			}
		}
		else
			; // ok = Obj.PutPacket(&id, &Pack, 0);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBillPoolOpExEntry::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRBILLPOOLOPEXENTRY_OPLIST:
			break;
		case DSF_CRRBILLPOOLOPEXENTRY_FONEOP:
			ok = TransferDataFlag(&Data.Flags, BPOXF_ONEOP, dir, rBuf); break;
		case DSF_CRRBILLPOOLOPEXENTRY_FONEDATE:
			ok = TransferDataFlag(&Data.Flags, BPOXF_ONEDATE, dir, rBuf); break;
		case DSF_CRRBILLPOOLOPEXENTRY_FONEOBJECT:
			ok = TransferDataFlag(&Data.Flags, BPOXF_ONEOBJECT, dir, rBuf); break;
		case DSF_CRRBILLPOOLOPEXENTRY_FUNITEACCTURNS:
			ok = TransferDataFlag(&Data.Flags, BPOXF_UNITEACCTURNS, dir, rBuf); break;
		case DSF_CRRBILLPOOLOPEXENTRY_FUNITEPAYMENTS:
			ok = TransferDataFlag(&Data.Flags, BPOXF_UNITEPAYMENTS, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
// CrrOprKind
//
int SLAPI PPDS_CrrOprKind::InitData(Ido op, void * dataPtr, long addedParam)
{
	int    ok = 1;
	if(op == idoAlloc)
		Data.Init();
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPOprKindPacket*)dataPtr;
		else if(addedParam) {
			if(Obj.GetPacket(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		if(strlen(Data.Rec.Symb) || strlen(Data.Rec.Name)) {
			PPID id = 0;
			PPOprKindPacket pack;
			if(*strip(Data.Rec.Symb) && Obj.SearchBySymb(Data.Rec.Symb, &id) > 0 ||
				*strip(Data.Rec.Name) && Obj.SearchByName(Data.Rec.Name, &id) > 0
			) {
				Data.Rec.ID = id;
				if(UpdateProtocol == updForce) {
					PPOprKindPacket pack;
					THROW(Obj.GetPacket(id, &pack) > 0);
					for(uint i = 0; i < AcceptedFields.getCount(); i++) {
						switch(AcceptedFields.get(i)) {
							case DSF_CRROPRKIND_EXTSTRING:
								pack.ExtString = Data.ExtString;
								break;
							case DSF_CRROPRKIND_REC:
								pack.Rec = Data.Rec;
								break;
							case DSF_CRROPRKIND_INVOPEX:
								if(Data.P_IOE) {
									ZDELETE(pack.P_IOE);
									if(Data.Rec.OpTypeID == PPOPT_INVENTORY) {
										THROW_MEM(pack.P_IOE = new PPInventoryOpEx);
										*pack.P_IOE = *Data.P_IOE;
									}
								}
								break;
							case DSF_CRROPRKIND_RECKONOPEX:
								if(Data.P_ReckonData) {
									ZDELETE(pack.P_ReckonData);
									if(Data.Rec.Flags & OPKF_RECKON) {
										THROW_MEM(pack.P_ReckonData = new PPReckonOpEx);
										*pack.P_ReckonData = *Data.P_ReckonData;
									}
								}
								break;
							case DSF_CRROPRKIND_DRAFTOPEX:
								if(Data.P_DraftData) {
									ZDELETE(pack.P_DraftData);
									if(oneof3(Data.Rec.OpTypeID, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_DRAFTTRANSIT)) {
										THROW_MEM(pack.P_DraftData = new PPDraftOpEx);
										*pack.P_DraftData = *Data.P_DraftData;
									}
								}
								break;
							case DSF_CRROPRKIND_BILLPOOLOPEX:
								if(Data.P_PoolData) {
									ZDELETE(pack.P_PoolData);
									if(Data.Rec.OpTypeID == PPOPT_POOL) {
										THROW_MEM(pack.P_PoolData = new PPBillPoolOpEx);
										*pack.P_PoolData = *Data.P_PoolData;
									}
								}
								break;
							case DSF_CRROPRKIND_GENOPSYMBLIST:
								break;
							case DSF_CRROPRKIND_AMOUNTS:
								pack.Amounts = Data.Amounts;
								break;
							case DSF_CRROPRKIND_ACCTURNTEMPLATES:
								pack.ATTmpls.copy(Data.ATTmpls);
								break;
						}
					}
					ok = Obj.PutPacket(&id, &pack, 0);
				}
			}
			else
				ok = Obj.PutPacket(&id, &Data, 0);
		}
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrOprKind::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRROPRKIND_EXTSTRING:
			ok = TransferData(Data.ExtString, dir, rBuf);
			break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}

int SLAPI PPDS_CrrOprKind::AcceptListItem(long fldID, PPDeclStruc * pData, ObjTransmContext * pCtx)
{
	int ok = -1;
	if(pData) {
		if(fldID == DSF_CRROPRKIND_REC) {
			Data.Rec = ((PPDS_CrrOprKindEntry*)pData)->Data;
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_AMOUNTS) {
			PPAmountTypePacket item = ((PPDS_CrrAmountType*)pData)->Pack;
			Data.Amounts.add(item.Rec.ID);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_ACCTURNTEMPLATES) {
			PPAccTurnTempl item = ((PPDS_CrrAccturnTempl*)pData)->Data;
			Data.ATTmpls.insert(&item);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_INVOPEX) {
			PPInventoryOpEx item = ((PPDS_CrrInvOpExEntry*)pData)->Data;
			ZDELETE(Data.P_IOE);
			THROW_MEM(Data.P_IOE = new PPInventoryOpEx);
			ASSIGN_PTR(Data.P_IOE, item);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_RECKONOPEX) {
			PPReckonOpEx item = ((PPDS_CrrReckonOpExEntry*)pData)->Data;
			ZDELETE(Data.P_ReckonData);
			THROW_MEM(Data.P_ReckonData = new PPReckonOpEx);
			ASSIGN_PTR(Data.P_ReckonData, item);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_DRAFTOPEX) {
			PPDraftOpEx item = ((PPDS_CrrDraftOpExEntry*)pData)->Data;
			ZDELETE(Data.P_DraftData);
			THROW_MEM(Data.P_DraftData = new PPDraftOpEx);
			ASSIGN_PTR(Data.P_DraftData, item);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_BILLPOOLOPEX) {
			PPBillPoolOpEx item = ((PPDS_CrrBillPoolOpExEntry*)pData)->Data;
			ZDELETE(Data.P_PoolData);
			THROW_MEM(Data.P_PoolData = new PPBillPoolOpEx);
			ASSIGN_PTR(Data.P_PoolData, item);
			ok = 1;
		}
		else if(fldID == DSF_CRROPRKIND_GENOPSYMBLIST) {
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrOprKind::CreateListItem(long fldID, uint * pIter, PPDeclStruc * pData)
{
	int    ok = -1;
	if(fldID == DSF_CRROPRKIND_REC) {
		if(*pIter == 0) {
			pData->InitData(idoExtract, &Data.Rec, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_AMOUNTS) {
		if(*pIter < Data.Amounts.getCount()) {
			pData->InitData(idoExtract, 0, Data.Amounts.at(*pIter));
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_ACCTURNTEMPLATES) {
		if(*pIter < Data.ATTmpls.getCount()) {
			pData->InitData(idoExtract, &Data.ATTmpls.at(*pIter), 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_INVOPEX) {
		if(*pIter == 0 && Data.P_IOE) {
			pData->InitData(idoExtract, &Data, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_RECKONOPEX) {
		if(*pIter == 0 && Data.P_ReckonData) {
			pData->InitData(idoExtract, &Data, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_DRAFTOPEX) {
		if(*pIter == 0 && Data.P_DraftData) {
			pData->InitData(idoExtract, &Data, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_BILLPOOLOPEX) {
		if(*pIter == 0 && Data.P_PoolData) {
			pData->InitData(idoExtract, &Data, 0);
			ok = 1;
		}
	}
	else if(fldID == DSF_CRROPRKIND_GENOPSYMBLIST) {
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
//
//
//
int SLAPI PPDS_CrrBillStatus::InitData(Ido op, void * dataPtr, long addedParam)
{
	int ok = 1;
	if(op == idoAlloc) {
		MEMSZERO(Data);
	}
	else if(op == idoExtract) {
		if(dataPtr)
			Data = *(PPBillStatus*)dataPtr;
		else if(addedParam) {
			if(Obj.Search(addedParam, &Data) > 0)
				ok = 1;
			else
				ok = -1;
		}
		else
			ok = -1;
	}
	else if(op == idoAccept) {
		PPID id = 0;
		Data.Tag = PPOBJ_BILLSTATUS;
		if(*strip(Data.Name) != 0 && Obj.SearchByName(Data.Name, &id, 0) > 0) {
			if(UpdateProtocol == updForce) {
				Data.ID = id;
				PPBillStatus rec;
				MEMSZERO(rec);
				THROW(Obj.Search(id, &rec) > 0);
				for(uint i = 0; i < AcceptedFields.getCount(); i++) {
					switch(AcceptedFields.get(i)) {
						case DSF_CRRBILLSTATUS_ID:
							break;
						case DSF_CRRBILLSTATUS_NAME:
							STRNSCPY(rec.Name, Data.Name);
							break;
						case DSF_CRRBILLSTATUS_SYMB:
							STRNSCPY(rec.Symb, Data.Symb);
							break;
						case DSF_CRRBILLSTATUS_COUNTERID:
							rec.CounterID = Data.CounterID;
							break;
						case DSF_CRRBILLSTATUS_RESTRICTOPID:
							rec.RestrictOpID = Data.RestrictOpID;
							break;
						case DSF_CRRBILLSTATUS_RANK:
							rec.Rank = Data.Rank;
							break;
						case DSF_CRRBILLSTATUS_FBILSTDENYMOD:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_DENY_MOD, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTDENYDEL:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_DENY_DEL, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTDENYTRANSM:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_DENY_TRANSM, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTDENYCHANGELINK:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_DENY_CHANGELINK, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTDENYRANKDOWN:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_DENY_RANKDOWN, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTLOCKACCTURN:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_LOCK_ACCTURN, Data.Flags);
							break;
						case DSF_CRRBILLSTATUS_FBILSTLOCKPAYMENT:
							SETFLAGBYSAMPLE(rec.Flags, BILSTF_LOCK_PAYMENT, Data.Flags);
							break;

						case DSF_CRRBILLSTATUS_FBILCHECKAGENT:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_AGENT, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKPAYER:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_PAYER, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKDLVRADDR:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_DLVRADDR, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKPORTOFLOADING:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_PORTOFLOADING, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKPORTOFDISCHARGE:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_PORTOFDISCHARGE, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKISSUEDT:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_ISSUEDT, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKARRIVALDT:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_ARRIVALDT, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKSHIP:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_SHIP, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKFREIGHTCOST:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_FREIGHTCOST, Data.CheckFields);
							break;
						case DSF_CRRBILLSTATUS_FBILCHECKFREIGHT:
							SETFLAGBYSAMPLE(rec.CheckFields, BILCHECKF_FREIGHT, Data.CheckFields);
							break;
					}
				}
				ok = Obj.UpdateItem(id, &rec, 1);
			}
		}
		else
			ok = Obj.AddItem(&id, &Data, 1);
	}
	else
		ok = (PPErrCode = PPERR_INVPARAM, 0);
	CATCHZOK
	return ok;
}

int SLAPI PPDS_CrrBillStatus::TransferField(long fldID, Tfd dir, uint * pIter, SString & rBuf)
{
	int ok = -1;
	switch(fldID) {
		case DSF_CRRBILLSTATUS_ID:
			{
				if(dir == tfdDataToBuf) {
					PPID id = Data.ID;
					ok = TransferData(&id, dir, rBuf);
				}
			}
			break;
		case DSF_CRRBILLSTATUS_NAME:
			ok = TransferData(Data.Name, sizeof(Data.Name), dir, rBuf);
			break;
		case DSF_CRRBILLSTATUS_SYMB:
			ok = TransferData(Data.Symb, sizeof(Data.Symb), dir, rBuf);
			break;
		case DSF_CRRBILLSTATUS_COUNTERID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOpCounter opc_rec;
					MEMSZERO(opc_rec);
					if(Data.CounterID && ObjCntr.Search(Data.CounterID, &opc_rec) > 0)
						buf.CopyFrom(opc_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ObjCntr.SearchByName(buf, &Data.CounterID);
			}
			break;
		case DSF_CRRBILLSTATUS_RESTRICTOPID:
			{
				SString buf;
				if(dir == tfdDataToBuf) {
					PPOprKind opk_rec;
					MEMSZERO(opk_rec);
					if(OpKObj.Search(Data.RestrictOpID, &opk_rec) > 0)
						buf.CopyFrom(opk_rec.Name);
				}
				ok = TransferData(buf, dir, rBuf);
				if(dir == tfdBufToData)
					ok = OpKObj.SearchByName(buf, &Data.RestrictOpID);
			}
			break;
		case DSF_CRRBILLSTATUS_RANK:
			ok = TransferData(&Data.Rank, dir, rBuf);
			break;

		case DSF_CRRBILLSTATUS_FBILSTDENYMOD:
			ok = TransferDataFlag(&Data.Flags, BILSTF_DENY_MOD, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTDENYDEL:
			ok = TransferDataFlag(&Data.Flags, BILSTF_DENY_DEL, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTDENYTRANSM:
			ok = TransferDataFlag(&Data.Flags, BILSTF_DENY_TRANSM, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTDENYCHANGELINK:
			ok = TransferDataFlag(&Data.Flags, BILSTF_DENY_CHANGELINK, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTDENYRANKDOWN:
			ok = TransferDataFlag(&Data.Flags, BILSTF_DENY_RANKDOWN, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTLOCKACCTURN:
			ok = TransferDataFlag(&Data.Flags, BILSTF_LOCK_ACCTURN, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILSTLOCKPAYMENT:
			ok = TransferDataFlag(&Data.Flags, BILSTF_LOCK_PAYMENT, dir, rBuf); break;

    	case DSF_CRRBILLSTATUS_FBILCHECKAGENT:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_AGENT, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKPAYER:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_PAYER, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKDLVRADDR:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_DLVRADDR, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKPORTOFLOADING:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_PORTOFLOADING, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKPORTOFDISCHARGE:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_PORTOFDISCHARGE, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKISSUEDT:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_ISSUEDT, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKARRIVALDT:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_ARRIVALDT, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKSHIP:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_SHIP, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKFREIGHTCOST:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_FREIGHTCOST, dir, rBuf); break;
		case DSF_CRRBILLSTATUS_FBILCHECKFREIGHT:
			ok = TransferDataFlag(&Data.CheckFields, BILCHECKF_FREIGHT, dir, rBuf); break;
	}
	if(ok > 0)
		(*pIter)++;
	return ok;
}
