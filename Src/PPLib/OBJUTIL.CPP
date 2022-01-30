// OBJUTIL.CPP
// Copyright (c) A.Sobolev 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop
#include <sys/types.h>
#include <sys/stat.h>

#define UNREGISTERED_OBJECT 0
//
// Учитывая, что все идентификаторы типов объектов умещаются в
// 2-х байтовое целое значение и ради экономии памяти этот массив
// определен как ushort [].
//
static const ushort _Objects[] = {
	PPOBJ_CONFIG,       // ?
	PPOBJ_USRGRP,       // ?
	PPOBJ_USR,          // ?
	PPOBJ_UNIT,         // !
	PPOBJ_CURRATE,      // !
	PPOBJ_CITYSTATUS,   // !
	PPOBJ_PERSONKIND,     // !
	PPOBJ_PRSNSTATUS,   // !
	PPOBJ_STAFFRANK,    // !
	PPOBJ_BNKACCTYPE,   // !
	PPOBJ_PRICETYPE,    // !
	PPOBJ_AMOUNTTYPE,   // !
	PPOBJ_ACCSHEET,     // !
	PPOBJ_PERSON,       // !
	PPOBJ_ARTICLE,      // !
	PPOBJ_ACCOUNT_PRE9004, // @v9.0.4
	PPOBJ_ACCOUNT2,     // ! // @v9.0.4 PPOBJ_ACCOUNT-->PPOBJ_ACCOUNT2
	PPOBJ_GOODSGROUP,   // !
	PPOBJ_GOODS,        // !
	PPOBJ_LOCATION,     // !
	PPOBJ_OPRTYPE,      // !
	PPOBJ_OPRKIND,      // !
	PPOBJ_ACCTURN,      // ?
	PPOBJ_CASHNODE,     // ?
	PPOBJ_ACTION,       // ?
	PPOBJ_GTACTION,     // ?
	PPOBJ_BILL,
	PPOBJ_QCERT,        // !
	// @v9.0.4 PPOBJ_BACCT,        // ?
	PPOBJ_BCODESTRUC,   // !
	PPOBJ_DBDIV,
	PPOBJ_VATBOOK,
	PPOBJ_GOODSTYPE,    // !
	PPOBJ_GOODSSTRUC,   // !
	PPOBJ_FORMULA,
	PPOBJ_SCALE,
	PPOBJ_REGISTERTYPE,
	PPOBJ_REGISTER,
	PPOBJ_ELINKKIND,
	PPOBJ_QUOTKIND,
	PPOBJ_PERSONOPKIND,
	PPOBJ_PERSONEVENT,
	PPOBJ_TAG,
	PPOBJ_STAFFLIST_PRE9003,
	PPOBJ_STAFFLIST2,
	PPOBJ_GOODSTAX,
	PPOBJ_BCODEPRINTER,
	PPOBJ_CURRENCY,
	PPOBJ_CURRATETYPE,
	PPOBJ_OPCOUNTER,
	PPOBJ_GOODSCLASS,
	PPOBJ_WOODBREED,
	PPOBJ_PCKGTYPE,
	PPOBJ_TRANSPORT,
	PPOBJ_GOODSBASKET,
	PPOBJ_BHT,
	PPOBJ_SCARDSERIES,
	PPOBJ_SCARD,
	PPOBJ_STYLOPALM,
	PPOBJ_DRAFTWROFF,
	PPOBJ_ASSTWROFFGRP,
	PPOBJ_ADVBILLKIND,
	PPOBJ_CSESSION,
	PPOBJ_TRANSPMODEL,
	PPOBJ_INTERNETACCOUNT,
	PPOBJ_BRAND,
	PPOBJ_PERSONRELTYPE,
	PPOBJ_PRSNCATEGORY,
	PPOBJ_PROJECT,
	PPOBJ_MRPTAB,
	PPOBJ_PRJTASK,
	PPOBJ_DFCREATERULE,
	PPOBJ_DYNAMICOBJS,
	PPOBJ_BILLSTATUS,
	PPOBJ_PROCESSOR,
	PPOBJ_TECH,
	PPOBJ_TSESSION,
	PPOBJ_TOUCHSCREEN,
	PPOBJ_DATETIMEREP,
	PPOBJ_DUTYSCHED,
	PPOBJ_WORLD,
	PPOBJ_GOODSINFO,
	PPOBJ_SALCHARGE,
	PPOBJ_STAFFCAL,
	PPOBJ_NAMEDOBJASSOC,
	PPOBJ_BIZSCORE,
	PPOBJ_GLOBALUSERACC,
	PPOBJ_GOODSVALRESTR,
	PPOBJ_BUDGET,
	PPOBJ_BIZSCTEMPL,
	PPOBJ_PALLET,
	PPOBJ_RFIDDEVICE,
	PPOBJ_DEBTDIM,
	PPOBJ_EVENTTOKEN,
	PPOBJ_SMSPRVACCOUNT,
	PPOBJ_PHONESERVICE,
	PPOBJ_GENERICDEVICE,
	PPOBJ_LOT,           // @nonobject
	PPOBJ_CAFETABLE,
	PPOBJ_EDIPROVIDER,
	PPOBJ_USREXCLRIGHTS,
	PPOBJ_WORKBOOK,           // @v9.3.8
	PPOBJ_FREIGHTPACKAGETYPE, // @v10.4.1
	PPOBJ_TAXSYSTEMKIND,      // @v10.6.0
	PPOBJ_TIMESERIES,         // @v10.7.4
	PPOBJ_TSSMODEL,           // @v10.7.4
	PPOBJ_EVENTSUBSCRIPTION,  // @v10.8.9
	PPOBJ_STYLOQBINDERY       // @v11.1.7
};

// @v10.9.4 #define _ObjectsCount (sizeof(_Objects) / sizeof(ushort/*PPID*/))

static PPObject * FASTCALL _GetPPObject(PPID obj, void * extraPtr)
{
	int    is_undef_obj = 0;
	PPObject * p = 0;
	switch(static_cast<uint>(obj)) {
		case PPOBJ_CONFIG      :
		case PPOBJ_USRGRP      :
		case PPOBJ_USREXCLRIGHTS:
		case PPOBJ_USR         : p = new PPObjSecur(obj, extraPtr); break;
		case PPOBJ_UNIT        : p = new PPObjUnit(extraPtr); break;
		case PPOBJ_PERSONKIND    : p = new PPObjPersonKind(extraPtr); break;
		case PPOBJ_PRSNCATEGORY: p = new PPObjPersonCat(extraPtr); break;
		case PPOBJ_CURRATE     :
		case PPOBJ_STAFFRANK   :
		case PPOBJ_BNKACCTYPE  :
		case PPOBJ_PRICETYPE   :
		case PPOBJ_WOODBREED   :
		case PPOBJ_EVENTTOKEN  :
		case PPOBJ_CAFETABLE   :
		case PPOBJ_TRANSPMODEL : p = new PPObjReference(obj, extraPtr); break;
		case PPOBJ_ACTION      : p = new PPObjSysAction(extraPtr); break;
		case PPOBJ_GTACTION    : p = new PPObjGtaAction(extraPtr); break;
		case PPOBJ_CITYSTATUS  : p = new PPObjWorldObjStatus(extraPtr); break;
		case PPOBJ_ADVBILLKIND : p = new PPObjAdvBillKind(extraPtr); break;
		case PPOBJ_DYNAMICOBJS : p = new PPObjDynamicObjs(extraPtr); break;
		case PPOBJ_PRSNSTATUS  : p = new PPObjPersonStatus(extraPtr); break;
		case PPOBJ_AMOUNTTYPE  : p = new PPObjAmountType(extraPtr); break;
		case PPOBJ_OPRTYPE     : p = new PPObjOprType(extraPtr); break;
		case PPOBJ_OPRKIND     : p = new PPObjOprKind(extraPtr); break;
		case PPOBJ_BILLSTATUS  : p = new PPObjBillStatus(extraPtr); break;
		case PPOBJ_ACCSHEET    : p = new PPObjAccSheet(extraPtr); break;
		case PPOBJ_CASHNODE    : p = new PPObjCashNode(extraPtr); break;
		case PPOBJ_PERSON      : p = new PPObjPerson(extraPtr); break;
		case PPOBJ_ARTICLE     : p = new PPObjArticle(extraPtr); break;
		case PPOBJ_ACCOUNT_PRE9004:
		case PPOBJ_ACCOUNT2    : p = new PPObjAccount(extraPtr); break;
		case PPOBJ_GOODSGROUP  : p = new PPObjGoodsGroup(extraPtr); break;
		case PPOBJ_GOODS       : p = new PPObjGoods(extraPtr); break;
		case PPOBJ_LOCATION    : p = new PPObjLocation(extraPtr); break;
		case PPOBJ_ACCTURN     : p = new PPObjAccTurn(extraPtr); break;
		case PPOBJ_BILL        : p = new PPObjBill(extraPtr); break;
		case PPOBJ_QCERT       : p = new PPObjQCert(extraPtr); break;
		// @v9.0.4 case PPOBJ_BACCT       : p = new PPObjBnkAcct(extraPtr); break;
		case PPOBJ_BCODESTRUC  : p = new PPObjBarCodeStruc(extraPtr); break;
		case PPOBJ_DBDIV       : p = new PPObjDBDiv(extraPtr); break;
		case PPOBJ_VATBOOK     : p = new PPObjVATBook(extraPtr); break;
		case PPOBJ_GOODSTYPE   : p = new PPObjGoodsType(extraPtr); break;
		case PPOBJ_GOODSSTRUC  : p = new PPObjGoodsStruc(extraPtr); break;
		case PPOBJ_FORMULA     : p = new PPObjFormula(extraPtr); break;
		case PPOBJ_SCALE       : p = new PPObjScale(extraPtr); break;
		case PPOBJ_PHONESERVICE: p = new PPObjPhoneService(extraPtr); break;
		case PPOBJ_REGISTERTYPE: p = new PPObjRegisterType(/*extraPtr*/); break;
		case PPOBJ_REGISTER    : p = new PPObjRegister(extraPtr); break;
		case PPOBJ_ELINKKIND   : p = new PPObjELinkKind(extraPtr); break;
		case PPOBJ_QUOTKIND    : p = new PPObjQuotKind(/*extraPtr*/); break;
		case PPOBJ_PERSONOPKIND: p = new PPObjPsnOpKind(extraPtr); break;
		case PPOBJ_PERSONEVENT : p = new PPObjPersonEvent(extraPtr); break;
		case PPOBJ_TAG         : p = new PPObjTag(extraPtr); break;
		case PPOBJ_STAFFLIST_PRE9003:
		case PPOBJ_STAFFLIST2  : p = new PPObjStaffList(extraPtr); break;
		case PPOBJ_GOODSTAX    : p = new PPObjGoodsTax(extraPtr); break;
		case PPOBJ_BCODEPRINTER: p = new PPObjBarcodePrinter(extraPtr); break;
		case PPOBJ_CURRENCY    : p = new PPObjCurrency(extraPtr); break;
		case PPOBJ_CURRATETYPE : p = new PPObjCurRateType(extraPtr); break;
		case PPOBJ_OPCOUNTER   : p = new PPObjOpCounter(extraPtr); break;
		case PPOBJ_GOODSCLASS  : p = new PPObjGoodsClass(extraPtr); break;
		case PPOBJ_PCKGTYPE    : p = new PPObjPckgType(extraPtr); break;
		case PPOBJ_PALLET      : p = new PPObjPallet(extraPtr); break;
		case PPOBJ_TRANSPORT   : p = new PPObjTransport(extraPtr); break;
		case PPOBJ_GOODSBASKET : p = new PPObjGoodsBasket(extraPtr); break;
		case PPOBJ_BHT         : p = new PPObjBHT(extraPtr); break;
		case PPOBJ_SCARDSERIES : p = new PPObjSCardSeries(extraPtr); break;
		case PPOBJ_SCARD       : p = new PPObjSCard(extraPtr); break;
		case PPOBJ_STYLOPALM   : p = new PPObjStyloPalm(extraPtr); break;
		case PPOBJ_DRAFTWROFF  : p = new PPObjDraftWrOff(extraPtr); break;
		case PPOBJ_ASSTWROFFGRP: p = new PPObjAssetWrOffGrp(extraPtr); break;
		case PPOBJ_CSESSION    : p = new PPObjCSession(extraPtr); break;
		case PPOBJ_INTERNETACCOUNT : p = new PPObjInternetAccount(extraPtr); break;
		case PPOBJ_BRAND       : p = new PPObjBrand(extraPtr); break;
		case PPOBJ_PERSONRELTYPE: p = new PPObjPersonRelType(extraPtr); break;
		case PPOBJ_PROJECT     : p = new PPObjProject(extraPtr); break;
		case PPOBJ_MRPTAB      : p = new PPObjMrpTab(extraPtr); break;
		case PPOBJ_PRJTASK     : p = new PPObjPrjTask(extraPtr); break;
		case PPOBJ_PROCESSOR   : p = new PPObjProcessor(extraPtr); break;
		case PPOBJ_TECH        : p = new PPObjTech(extraPtr); break;
		case PPOBJ_TSESSION    : p = new PPObjTSession(extraPtr); break;
		case PPOBJ_DFCREATERULE: p = new PPObjDraftCreateRule(extraPtr); break;
		case PPOBJ_TOUCHSCREEN : p = new PPObjTouchScreen(extraPtr); break;
		case PPOBJ_DATETIMEREP : p = new PPObjDateTimeRep(extraPtr); break;
		case PPOBJ_DUTYSCHED   : p = new PPObjDutySched(extraPtr);   break;
		case PPOBJ_LOCPRINTER  : p = new PPObjLocPrinter(extraPtr); break;
		case PPOBJ_WORLD       : p = new PPObjWorld(extraPtr); break;
		case PPOBJ_GOODSINFO   : p = new PPObjGoodsInfo(extraPtr); break;
		case PPOBJ_SALCHARGE   : p = new PPObjSalCharge(extraPtr); break;
		case PPOBJ_STAFFCAL:           p = new PPObjStaffCal(extraPtr); break;
		case PPOBJ_NAMEDOBJASSOC:      p = new PPObjNamedObjAssoc(extraPtr); break;
		case PPOBJ_BIZSCORE:           p = new PPObjBizScore(extraPtr); break;
		case PPOBJ_GLOBALUSERACC:      p = new PPObjGlobalUserAcc(extraPtr); break;
		case PPOBJ_GOODSVALRESTR:      p = new PPObjGoodsValRestr(extraPtr); break;
		case PPOBJ_BUDGET:             p = new PPObjBudget(extraPtr); break;
		case PPOBJ_BIZSCTEMPL:         p = new PPObjBizScTempl(extraPtr); break;
		case PPOBJ_RFIDDEVICE:         p = new PPObjRFIDDevice(extraPtr); break;
		case PPOBJ_DEBTDIM:            p = new PPObjDebtDim(); break;
		case PPOBJ_SMSPRVACCOUNT:      p = new PPObjSmsAccount(extraPtr); break; // @vmiller
		case PPOBJ_UHTTSTORE:          p = new PPObjUhttStore(extraPtr); break; // @v7.6.1 @Muxa
		case PPOBJ_GENERICDEVICE:      p = new PPObjGenericDevice(extraPtr); break; // @v7.8.0
		case PPOBJ_WORKBOOK:           p = new PPObjWorkbook(extraPtr); break; // @Muxa
		case PPOBJ_EDIPROVIDER:        p = new PPObjEdiProvider(extraPtr); break; // @v8.5.5
		case PPOBJ_TIMESERIES:         p = new PPObjTimeSeries(extraPtr); break; // @v10.2.3
		case PPOBJ_FREIGHTPACKAGETYPE: p = new PPObjFreightPackageType(extraPtr); break; // @v10.4.1
		case PPOBJ_TAXSYSTEMKIND:      p = new PPObjTaxSystemKind(extraPtr); break; // @v10.6.0
		case PPOBJ_TSSMODEL:           p = new PPObjTssModel(extraPtr); break; // @v10.7.4
		case PPOBJ_EVENTSUBSCRIPTION:  p = new PPObjEventSubscription(extraPtr); break; // @v10.8.9
		case PPOBJ_STYLOQBINDERY:      p = new PPObjStyloQBindery(extraPtr); break;     // @v11.1.7
		default:
			if(PPRef->GetItem(PPOBJ_DYNAMICOBJS, obj) > 0)
				p = new PPObjReference(obj, extraPtr);
			else {
				char  add_msg[64];
				PPSetError(PPERR_UNDEFPPOBJTYPE, ltoa(obj, add_msg, 10));
				is_undef_obj = 1;
			}
	}
	if(p == 0 && !is_undef_obj)
		PPSetErrorNoMem();
	return p;
}

PPObject::SerializeSignature::SerializeSignature() : S(0), V(0)
{
}

PPObject::SerializeSignature::SerializeSignature(PPID objType) : S(0xAAAA0000 | objType), V(DS.GetVersion())
{
	assert(objType < 0x0000ffff);
}

PPObject::SerializeSignature::SerializeSignature(PPID objType, int dir, SBuffer & rBuf) : S(0xAAAA0000 | objType), V(DS.GetVersion())
{
	if(dir < 0) {
		Read(rBuf);
	}
	else if(dir > 0) {
		Write(rBuf);
	}
}

bool PPObject::SerializeSignature::IsValid() const
{
	return (((S & 0xffff0000) == 0xAAAA0000) && ((S & 0x0000ffff) <= 2000) && (V.GetMajor() <= 30) && (V.GetMinor() <= 20) && (V.GetRevision() <= 20));
}

int PPObject::SerializeSignature::Read(SBuffer & rBuf)
{
	int    ok = 1;
	SerializeSignature temp_signature;
	if(rBuf.ReadV(&temp_signature, sizeof(temp_signature)) && temp_signature.IsValid()) {
		*this = temp_signature;
	}
	else {
		S = 0;
		V.Set(0);
		rBuf.Unread(sizeof(temp_signature));
		ok = 0;
	}
	return ok;
}

int PPObject::SerializeSignature::Write(SBuffer & rBuf) const
{
	int    ok = 1;
	assert(IsValid());
	THROW(IsValid());
	THROW_SL(rBuf.Write(this, sizeof(*this)));
	CATCHZOK
	return ok;
}

/*static*/void FASTCALL PPObject::SetLastErrObj(PPID objType, PPID objID)
	{ DS.GetTLA().LastErrObj.Set(objType, objID); }
/*static*/int FASTCALL PPObject::RetRefsExistsErr(PPID objType, PPID objID)
	{ return PPSetObjError(PPERR_REFSEXISTS, objType, objID); }

/*static*/SString & PPObject::GetAcceptMsg(PPID objType, PPID objID, int upd, SString & rBuf)
{
	SString fmt_buf, obj_type_name, obj_name;
	PPGetSubStr(PPTXT_OBJACCEPTED, upd ? PPOBJACCP_OBJUPDATED : PPOBJACCP_OBJCREATED, fmt_buf);
	GetObjectTitle(objType, obj_type_name);
	GetObjectName(objType, objID, obj_name);
	return rBuf.Printf(fmt_buf, obj_type_name.cptr(), objID, obj_name.cptr());
}

/*static*/int PPObject::Helper_PutConfig(PPID cfgPropID, PPID cfgObjType, int isNew, const void * pData, size_t sz, int use_ta)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	if(p_ref) {
		PPTransaction tra(use_ta);
		THROW(tra);
		THROW(p_ref->PutProp(PPOBJ_CONFIG, PPCFG_MAIN, cfgPropID, pData, sz, 0));
		if(cfgObjType)
			DS.LogAction(isNew ? PPACN_CONFIGCREATED : PPACN_CONFIGUPDATED, cfgObjType, 0, 0, 0);
		THROW(tra.Commit());
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObject::ReadBlk(void * pBlk, size_t blkSize, void * stream)
{
	int    ok = 0;
	if(stream)
		ok = (fread(pBlk, blkSize, 1, static_cast<FILE *>(stream)) == 1) ? 1 : PPSetError(PPERR_PPOSREADFAULT);
	return ok;
}

/*static*/int FASTCALL PPObject::WriteBlk(const void * pBlk, size_t blkSize, void * stream)
{
	int    ok = 0;
	if(stream)
		ok = (fwrite(pBlk, blkSize, 1, static_cast<FILE *>(stream)) == 1) ? 1 : PPSetError(PPERR_PPOSWRITEFAULT);
	return ok;
}

/*static*/int FASTCALL PPObject::ReadSBuffer(SBuffer & rBuf, void * stream)
{
	int    ok = 1;
	uint32 sz = 0;
	THROW(stream);
	THROW_PP(fread(&sz, sizeof(sz), 1, static_cast<FILE *>(stream)) == 1, PPERR_PPOSREADFAULT);
	if(sz) {
		STempBuffer temp_buf(sz);
		THROW_PP(fread(temp_buf, temp_buf.GetSize(), 1, static_cast<FILE *>(stream)) == 1, PPERR_PPOSREADFAULT);
		THROW_SL(rBuf.Write(temp_buf, temp_buf.GetSize()));
	}
	CATCHZOK
	return ok;
}

/*static*/int FASTCALL PPObject::WriteSBuffer(SBuffer & rBuf, void * stream)
{
	int    ok = 1;
	THROW(stream);
	{
		uint32 sz = (uint32)rBuf.GetAvailableSize();
		STempBuffer temp_buf(sz);
		THROW_PP(fwrite(&sz, sizeof(sz), 1, static_cast<FILE *>(stream)) == 1, PPERR_PPOSREADFAULT);
		rBuf.Read(temp_buf, temp_buf.GetSize());
		THROW_PP(fwrite(temp_buf, temp_buf.GetSize(), 1, static_cast<FILE *>(stream)) == 1, PPERR_PPOSREADFAULT);
	}
	CATCHZOK
	return ok;
}

/*static*/int PPObject::ReplaceObj(PPID objType, PPID dest, PPID src, uint options)
{
	int    ok = 1;
	PPCommSyncID dest_comm_id;
	PPObject * o = 0;
	ObjSyncCore obj_sync;
	PPWaitStart();
	THROW_PP(dest != src, PPERR_REPLSAMEOBJ);
	if(obj_sync.SearchPrivate(objType, dest, 0, 0) > 0)
		dest_comm_id = obj_sync.data;
	THROW(o = GetPPObject(objType, 0));
	{
		PPTransaction tra(BIN(options & use_transaction));
		THROW(tra);
		THROW(SendObjMessage(DBMSG_OBJREPLACE, objType, objType, dest, reinterpret_cast<void *>(src), 0) == DBRPL_OK);
		PPWaitStop();
		if(!(options & not_repl_remove))
			THROW(o->RemoveObjV(dest, 0, (options & ~use_transaction), 0));
		DS.LogAction(PPACN_OBJUNIFY, objType, src,  -dest, 0);
		DS.LogAction(PPACN_OBJUNIFY, objType, dest, src, 0);
		THROW(tra.Commit());
	}
	CATCHZOK
	delete o;
	PPWaitStop();
	return ok;
}

/*static*/int PPObject::ReplaceObjInteractive(PPID objTypeID, PPID srcID /*=0*/)
{
	class ReplObjDialog : public TDialog {
	public:
		ReplObjDialog(PPID objTypeID) : TDialog(DLG_REPLOBJ), ObjTypeID(objTypeID)
		{
			SString buf;
			setSubTitle(GetObjectTitle(ObjTypeID, buf));
		}
		int    setDTS(PPID destID, PPID srcID)
		{
			SetupPPObjCombo(this, CTLSEL_REPLOBJ_SRC, ObjTypeID,   srcID,  OLW_CANSELUPLEVEL, 0);
			SetupPPObjCombo(this, CTLSEL_REPLOBJ_DEST, ObjTypeID,  destID, OLW_CANSELUPLEVEL, 0);
			return 1;
		}
		int    getDTS(PPID * pDestID, PPID * pSrcID)
		{
			int    ok = 1;
			PPID   src_id  = getCtrlLong(CTLSEL_REPLOBJ_SRC);
			PPID   dest_id = getCtrlLong(CTLSEL_REPLOBJ_DEST);
			THROW_PP(dest_id != 0 && src_id != 0, PPERR_REPLZEROOBJ);
			THROW_PP(dest_id != src_id, PPERR_REPLSAMEOBJ);
			if(ObjTypeID == PPOBJ_WORLD) {
				WorldTbl::Rec wrec_src, wrec_dest;
				PPObjWorld objworld;
				// @v10.7.9 @ctr MEMSZERO(wrec_src);
				// @v10.7.9 @ctr MEMSZERO(wrec_dest);
				objworld.Search(src_id, &wrec_src);
				objworld.Search(dest_id, &wrec_dest);
				THROW_PP(wrec_src.Kind == wrec_dest.Kind, PPERR_OBJKINDNOTEQUAL);
			}
			ASSIGN_PTR(pSrcID, src_id);
			ASSIGN_PTR(pDestID, dest_id);
			CATCH
				selectCtrl(src_id == 0 ? CTL_REPLOBJ_DEST : CTL_REPLOBJ_SRC);
				ok = 0;
			ENDCATCH
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmExchange)) {
				PPID   src_id = 0, dest_id = 0;
				getDTS(&dest_id, &src_id);
				setDTS(src_id, dest_id);
				clearEvent(event);
			}
		}
		PPID   ObjTypeID;
	};
	int    ok = -1;
	PPID   dest_id = 0, src_id = srcID;
	ReplObjDialog * dlg = 0;
	THROW(CheckDialogPtr(&(dlg = new ReplObjDialog(objTypeID))));
	dlg->setDTS(dest_id, src_id);
	while(ExecView(dlg) == cmOK) {
		if(!dlg->getDTS(&dest_id, &src_id) || !PPObject::ReplaceObj(objTypeID, dest_id, src_id, PPObject::use_transaction|PPObject::user_request))
			PPError();
		else {
			dlg->setDTS((dest_id = 0), src_id);
			ok = 1;
		}
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

/*static*/int PPObject::IndexPhones()
{
	int    ok = 1;
	THROW_PP(CConfig.Flags2 & CCFLG2_INDEXEADDR, PPERR_CCFG_INDEXEADDRNEEDED);
	{
		PPLogger logger;
		PPObjPerson psn_obj;
		PPObjSCard sc_obj;
		PPTransaction tra(1);
		PPWaitStart();
		THROW(tra);
		THROW(psn_obj.IndexPhones(&logger, 0));
		THROW(psn_obj.LocObj.IndexPhones(&logger, 0));
		THROW(sc_obj.IndexPhones(&logger, 0));
		THROW(tra.Commit());
		psn_obj.LocObj.P_Tbl->DumpEaList(0);
		PPWaitStop();
	}
	CATCHZOKPPERR
	return ok;
}

PPObjPack::PPObjPack() : Data(0), Mod(ZERODATETIME), Priority(PPObjectTransmit::DefaultPriority), Flags(0)
{
}

int PPObject::CreateReservedObjects(long flags)
{
	// @todo Выдавать на экран список объектов, созданных в результате работы функции
	int    ok = 1;
	PPIDArray obj_type_list;
	PPWaitStart();
	PPGetObjTypeList(&obj_type_list, 0);
    {
    	uint   p1 = 0, p2 = 0;
    	if(obj_type_list.lsearch(PPOBJ_PERSONKIND, &p1) && obj_type_list.lsearch(PPOBJ_ACCSHEET, &p2)) {
			if(p1 > p2)
				obj_type_list.swap(p1, p2);
    	}
    	p1 = p2 = 0;
    	if(obj_type_list.lsearch(PPOBJ_ACCSHEET, &p1) && obj_type_list.lsearch(PPOBJ_OPRKIND, &p2)) {
			if(p1 > p2)
				obj_type_list.swap(p1, p2);
    	}
    }
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		PPObject * p_o = GetPPObject(obj_type_list.at(i), 0);
		if(p_o && !p_o->MakeReserved(flags))
			ok = PPErrorZ();
		delete p_o;
	}
	if(flags & mrfInitializeDb) {
		PPObjSecur sec_obj(PPOBJ_CONFIG, 0);
		PPSecur sec_rec;
		if(sec_obj.Search(PPCFG_MAIN, &sec_rec) > 0) {
			PPConfig cfg_rec;
			PPCommConfig comm_cfg;
			Reference * p_ref = PPRef;
			PPIDArray loc_acs_list;
			PPIDArray sell_acs_list;
			PPIDArray suppl_acs_list;
			PPIDArray main_org_list;
			PPIDArray loc_list;
			PPID   suppl_acs_id = 0;
			PPID   rcpt_op_id = 0;
			PPID   purchase_op_id = 0;
			PPID   retail_op_id = 0;
			PPID   intr_rcpt_op_id = 0;
			{
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				for(PPID acs_id = 0; acs_obj.EnumItems(&acs_id, &acs_rec) > 0;) {
					if(acs_rec.Assoc == PPOBJ_LOCATION)
						loc_acs_list.add(acs_rec.ID);
					else if(acs_rec.Assoc == PPOBJ_PERSON) {
						if(acs_rec.ObjGroup == PPPRK_CLIENT)
							sell_acs_list.add(acs_rec.ID);
						else if(acs_rec.ObjGroup == PPPRK_SUPPL)
							suppl_acs_list.add(acs_rec.ID);
					}
				}
				if(suppl_acs_list.getCount() == 1)
					suppl_acs_id = suppl_acs_list.get(0);
			}
			{
				PPObjPerson psn_obj;
				psn_obj.GetListByKind(PPPRK_MAIN, &main_org_list, 0);
				{
					LocationTbl::Rec loc_rec;
					for(SEnum en = psn_obj.LocObj.P_Tbl->Enum(LOCTYP_WAREHOUSE, 0, 0); en.Next(&loc_rec) > 0;) {
						loc_list.add(loc_rec.ID);
					}
				}
			}
			{
				PPObjOprKind op_obj;
				PPOprKind op_rec;
				if(suppl_acs_id) {
					if(op_obj.Search(PPOPK_RECEIPT, &op_rec) > 0 && op_rec.AccSheetID == suppl_acs_id) {
						rcpt_op_id = op_rec.ID;
					}
				}
				if(op_obj.Search(PPOPK_RETAIL, &op_rec) > 0) {
					retail_op_id = op_rec.ID;
				}
				if(op_obj.Search(PPOPK_INTRRECEIPT, &op_rec) > 0) {
					intr_rcpt_op_id = op_rec.ID;
				}
				if(suppl_acs_id) {
					PPIDArray temp_op_list;
					PPIDArray candid_list;
					GetOpList(PPOPT_DRAFTRECEIPT, &temp_op_list);
					for(uint i = 0; i < temp_op_list.getCount(); i++) {
						const PPID temp_op_id = temp_op_list.get(i);
						if(op_obj.Search(temp_op_id, &op_rec) > 0 && op_rec.AccSheetID == suppl_acs_id)
							candid_list.add(temp_op_id);
					}
					if(candid_list.getCount() == 1)
						purchase_op_id = candid_list.get(0);
				}
			}
			{
				int    do_update = 0;
				int    do_update_comm_cfg = 0;
				PPTransaction tra(1);
				THROW(tra);
				int    r = p_ref->GetConfig(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec));
				THROW(r);
				THROW(GetCommConfig(&comm_cfg));
				if(r < 0) {
					cfg_rec.Tag   = 0;
					cfg_rec.ObjID = PPCFG_MAIN;
					cfg_rec.PropID        = PPPRP_CFG;
					cfg_rec.AccessLevel   = DEFCFG_ACCESS;
					cfg_rec.BaseCurID     = DEFCFG_CURRENCY;
					cfg_rec.RealizeOrder  = DEFCFG_RLZORD;
					cfg_rec.Menu  = DEFCFG_MENU;
					cfg_rec.LocAccSheetID = 0;
					cfg_rec.Location      = 0;
					cfg_rec.Flags = CFGFLG_UNITEINTRTRFR;
					do_update = 1;
				}
				if(!cfg_rec.LocAccSheetID && loc_acs_list.getCount() == 1) {
					cfg_rec.LocAccSheetID = loc_acs_list.get(0);
					do_update = 1;
				}
				if(!cfg_rec.MainOrg && main_org_list.getCount() == 1) {
					cfg_rec.MainOrg = main_org_list.get(0);
					do_update = 1;
				}
				if(!cfg_rec.Location && loc_list.getCount() == 1) {
					cfg_rec.Location = loc_list.get(0);
					do_update = 1;
				}
				if(!cfg_rec.RealizeOrder) {
					cfg_rec.RealizeOrder = RLZORD_FIFO;
					do_update = 1;
				}
				//
				if(!comm_cfg.SellAccSheet && sell_acs_list.getCount() == 1) {
					comm_cfg.SellAccSheet = sell_acs_list.get(0);
					do_update_comm_cfg = 1;
				}
				if(!comm_cfg.SupplAccSheet && suppl_acs_id) {
					comm_cfg.SupplAccSheet = suppl_acs_id;
					do_update_comm_cfg = 1;
				}
				{
					comm_cfg.Flags |= (CCFLG_USEDRAFTBILL|CCFLG_USEGDSCLS|CCFLG_USEHISTBILL|CCFLG_USEHISTGOODS|
						CCFLG_THROUGHSCARDUNIQ|CCFLG_USECCHECKEXT|CCFLG_USECCHECKLINEEXT|CCFLG_GENLOTONUNLIMORDER|
						CCFLG_USEARGOODSCODE|CCFLG_LOGCCHECK|CCFLG_RSRVSJ);
					comm_cfg.Flags2 |= (CCFLG2_QUOT2|CCFLG2_INDEXEADDR|CCFLG2_SYNCLOT);
					comm_cfg.IncomeCalcMethod = INCM_BYSHIPMENT;
					if(rcpt_op_id)
						comm_cfg.ReceiptOp = rcpt_op_id;
					if(intr_rcpt_op_id)
						comm_cfg.IntrReceiptOp = intr_rcpt_op_id;
					if(retail_op_id)
						comm_cfg.RetailOp = retail_op_id;
					if(purchase_op_id)
						comm_cfg.DraftRcptOp = purchase_op_id;
					do_update_comm_cfg = 1;
				}
				//
				if(do_update) {
					THROW(p_ref->SetConfig(PPOBJ_CONFIG, PPCFG_MAIN, PPPRP_CFG, &cfg_rec, sizeof(cfg_rec)));
				}
				if(do_update_comm_cfg) {
					THROW(SetCommConfig(&comm_cfg, 0));
				}
				THROW(tra.Commit());
			}
		}
	}
	// @v11.0.10 {
	if(flags & mrfCreateMqbDefaults) {
		PPMqbClient * p_mqbc = 0;
		PPMqbClient::InitParam mqip;
		if(PPMqbClient::SetupInitParam(mqip, 0/*vhost*/, 0/*domain*/)) {
			p_mqbc = PPMqbClient::CreateInstance(mqip);
			if(p_mqbc) {
				p_mqbc->DeclarePredefinedExchanges();
				ZDELETE(p_mqbc);
			}
		}
	}
	// } @v11.0.10 
	CATCHZOK
	PPWaitStop();
	return ok;
}

void FASTCALL PPObject::Helper_GetRights(int onStartUp)
{
	const PPThreadLocalArea & r_tla = DS.GetConstTLA();
	if(onStartUp || r_tla.RightsUpdateCounter > RightsUpdateCounter) {
		ObjRights def_rt;
		const ObjRights * p_obj_rt = r_tla.Rights.GetConstObjRights(Obj, &def_rt);
		if(p_obj_rt && !(r_tla.Lc.State & CFGST_MASTER)) {
			Rt_  = p_obj_rt->Flags;
			ORt_ = p_obj_rt->OprFlags;
		}
		else {
			Rt_  = PPRights::GetDefaultFlags();
			ORt_ = PPRights::GetDefaultOprFlags();
		}
		RightsUpdateCounter = r_tla.RightsUpdateCounter;
		if(r_tla.Lc.State & CFGST_MASTER) // PPMaster
			Rt_ |= PPR_ADM;
	}
}

PPObject::PPObject(PPID aObj) : Obj(aObj), FiltProc(0), ImplementFlags(0), Rt_(0), ORt_(0)
{
	if(Obj != 0)
		Helper_GetRights(1);
}

PPObject::~PPObject()
{
}

int PPObject::Browse(void * extraPtr)
{
	return 0;
}

int PPObject::EditPrereq(PPID * pID, TDialog * pDlg, int * pIsNew)
{
	int    ok = 1, is_new = 0;
	assert(pID);
	if(!pID)
		ok = PPSetErrorInvParam();
	else if(!CheckRights(PPR_READ))
		ok = 0;
	else if(*pID == 0) {
		is_new = 1;
		if(!CheckRights(PPR_INS))
			ok = 0;
	}
	if(ok && pDlg && !is_new && !CheckRights(PPR_MOD))
		pDlg->enableCommand(cmOK, 0);
	ASSIGN_PTR(pIsNew, is_new);
	return ok;
}

int PPObject::Edit(PPID * pID, void * extraPtr) { return 0; }
int PPObject::Search(PPID id, void *) { return 0; }
int PPObject::DeleteObj(PPID id) { return 0; }
int PPObject::ProcessObjRefs(PPObjPack *, PPObjIDArray *, int replace, ObjTransmContext * pCtx) { return 1; }
const char * PPObject::GetNamePtr() { return 0; }
StrAssocArray * PPObject::MakeStrAssocList(void * extraPtr) { return 0; }
int PPObject::MakeReserved(long flags) { return 1; }
int PPObject::HandleMsg(int msg, PPID /*_obj*/, PPID /*_id*/, void * extraPtr) { return (msg == DBMSG_SECURITY) ? DBRPL_CANCEL : DBRPL_OK; }
int PPObject::Read(PPObjPack *, PPID, void * /*stream*/, ObjTransmContext * pCtx) { return 0; }
int PPObject::Write(PPObjPack * /*pPack*/, PPID * /*pID*/, void * /*stream*/, ObjTransmContext * pCtx) { return 0; }

int PPObject::GetLastModifEvent(PPID objID, LDATETIME * pMoment, int * pCr, SysJournalTbl::Rec * b)
{
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	return p_sj ? p_sj->GetLastObjModifEvent(Obj, objID, pMoment, pCr, b) : 0;
}

int PPObject::GetName(PPID id, char * pBuf, size_t bufLen)
{
	const char * ptr = 0;
	int    r = Search(id);
	if(pBuf)
		if(r > 0 && (ptr = GetNamePtr()) != 0)
			strnzcpy(pBuf, ptr, bufLen);
		else
			pBuf[0] = 0;
	return r;
}

int PPObject::GetName(PPID id, SString * pBuf)
{
	char   temp_buf[512];
	int    ok = GetName(id, temp_buf, sizeof(temp_buf));
	ASSIGN_PTR(pBuf, temp_buf);
	return ok;
}

void FASTCALL PPObject::Destroy(PPObjPack * pPack)
{
	if(pPack)
		ZDELETE(pPack->Data);
}

ListBoxDef * PPObject::Selector(ListBoxDef * pOrgDef, long flags, void * extraPtr)
{
	ListBoxDef * p_def = 0;
	if(ImplementFlags & implStrAssocMakeList) {
		StrAssocArray * p_list = MakeStrAssocList(extraPtr);
		if(p_list) {
			// @v11.2.12 {
			if(flags & OLW_INSCONTEXTEDITEMS) {
				p_list->AddFast(ROBJID_CONTEXT, "#BYCONTEXT");
				p_list->Move(p_list->getCount()-1, 0);
			}
			// } @v11.2.12
			if(pOrgDef) {
				if(ImplementFlags & implTreeSelector)
					static_cast<StdTreeListBoxDef *>(pOrgDef)->setArray(p_list);
				else
					static_cast<StrAssocListBoxDef *>(pOrgDef)->setArray(p_list);
				p_def = pOrgDef;
			}
			else {
				if(ImplementFlags & implTreeSelector)
					p_def = new StdTreeListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData, MKSTYPE(S_ZSTRING, 64));
				else
					p_def = new StrAssocListBoxDef(p_list, lbtDblClkNotify|lbtFocNotify|lbtDisposeData);
				if(!p_def)
					PPSetErrorNoMem();
			}
		}
		if(!p_def) {
			delete p_list;
			PPError();
		}
	}
	else if(pOrgDef) {
		pOrgDef->refresh();
		p_def = pOrgDef;
	}
	return p_def;
}

/* @v11.1.10 int PPObject::UpdateSelector_Obsolete(ListBoxDef * pDef, long flags, void * extraPtr)
{
	int    ok = 1;
	if(pDef)
		if(ImplementFlags & implStrAssocMakeList) {
			StrAssocArray * p_list = MakeStrAssocList(extraPtr);
			if(p_list) {
				if(ImplementFlags & implTreeSelector)
					static_cast<StdTreeListBoxDef *>(pDef)->setArray(p_list);
				else
					static_cast<StrAssocListBoxDef *>(pDef)->setArray(p_list);
			}
			else
				ok = PPErrorZ();
		}
		else
			pDef->refresh();
	else
		ok = -1;
	return ok;
}*/

int PPObject::ValidateSelection(PPID, uint olwFlags, void * extraPtr)
{
	return 1;
}

int FASTCALL PPObject::Dirty(PPID id)
{
	int    ok = -1;
	ObjCache * p_cache = DS.GetDbLocalObjCache(Obj);
	if(p_cache) {
		p_cache->Dirty(id);
		ok = 1;
	}
	return ok;
}

int FASTCALL PPObject::CheckRights(long rt, int oprRights)
{
	Helper_GetRights(0);
	return (((oprRights ? ORt_ : Rt_) & rt) == rt) ? 1 : PPSetError(PPERR_NORIGHTS);
}

int FASTCALL PPObject::CheckRights(long rt)
{
	Helper_GetRights(0);
	return ((Rt_ & rt) == rt) ? 1 : PPSetError(PPERR_NORIGHTS);
}

int FASTCALL PPObject::CheckRightsModByID(const PPID * pID)
{
	return pID ? CheckRights(*pID ? PPR_MOD : PPR_INS) : PPSetErrorInvParam();
}

int GenericObjRightsDialog(PPID obj, ObjRights * rt, EmbedDialog * pDlg)
{
	class GenericObjRightsDlg : public TDialog {
	public:
		GenericObjRightsDlg(PPID obj) : TDialog(DLG_RTOBJ)
		{
			SString  obj_title;
			setTitle(GetObjectTitle(obj, obj_title));
		}
		virtual int TransmitData(int dir, void * pData)
		{
			int    s = 1;
			if(dir > 0)
				setDTS(static_cast<ObjRights *>(pData));
			else if(dir < 0)
				getDTS(static_cast<ObjRights *>(pData));
			else
				s = TDialog::TransmitData(dir, pData);
			return s;
		}
		int setDTS(ObjRights * pData)
		{
			if(pData)
				setCtrlData(CTL_RTOBJ_FLAGS, &pData->Flags);
			return 1;
		}
		int getDTS(ObjRights * pData)
		{
			if(pData) {
				getCtrlData(CTL_RTOBJ_FLAGS, &pData->Flags);
  				pData->Size     = sizeof(ObjRights);
				pData->OprFlags = PPRights::GetDefaultOprFlags();
			}
			return 1;
		}
	};
	int    r = 0;
	GenericObjRightsDlg * dlg = new GenericObjRightsDlg(obj);
	if(pDlg) {
		pDlg->Embed(dlg);
		r = cmOK;
	}
	else {
		ushort    data = rt ? rt->Flags : 0;
		if(CheckDialogPtr(&dlg)) {
			SString obj_title;
			dlg->setTitle(GetObjectTitle(obj, obj_title));
			dlg->setCtrlData(CTL_RTOBJ_FLAGS, &rt->Flags);
			if((r = ExecView(dlg)) == cmOK) {
				dlg->getCtrlData(CTL_RTOBJ_FLAGS, &rt->Flags);
				if(data == rt->Flags) {
					r = cmCancel;
					rt->Flags = data;
				}
			}
			delete dlg;
		}
	}
	return r;
}

int PPObject::EditRights(uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	int    r = 0;
	if(IS_DYN_OBJTYPE(Obj) || Obj == PPOBJ_DYNAMICOBJS) {
		r = EditSpcRightFlags(DLG_RTDYNOBJ, CTL_RTDYNOBJ_FLAGS, CTL_RTDYNOBJ_SFLAGS, bufSize, rt, pDlg);
	}
	else
		r = GenericObjRightsDialog(Obj, rt, pDlg);
	if(r == cmOK)
		if(pDlg)
			return 1;
		else
			return (bufSize < sizeof(ObjRights)) ? PPSetError(PPERR_OBJRTBUFSIZ) : 1;
	return r ? -1 : 0;
}

int PPObject::EditSpcRightFlags(uint dlgID, uint flCtlID, uint sflCtlID, uint bufSize, ObjRights * rt, EmbedDialog * pDlg)
{
	class SpcRightFlagsDlg : public TDialog {
	public:
		SpcRightFlagsDlg(uint dlgID, uint flCtlID, uint sflCtlID) : TDialog(dlgID), FLCtlID(flCtlID), SFLCtlID(sflCtlID)
		{
		}
		virtual int TransmitData(int dir, void * pData)
		{
			int    s = 1;
			if(dir > 0)
				setDTS(static_cast<ObjRights *>(pData));
			else if(dir < 0)
				getDTS(static_cast<ObjRights *>(pData));
			else
				s = TDialog::TransmitData(dir, pData);
			return s;
		}
		int setDTS(const ObjRights * pData)
		{
			ushort comm_rt = 0, spec_rt = 0;
			if(pData) {
				comm_rt = spec_rt = pData->Flags;
				spec_rt >>= 8;
			}
			setCtrlData(FLCtlID, &comm_rt);
			setCtrlData(SFLCtlID, &spec_rt);
			return 1;
		}
		int getDTS(ObjRights * pData)
		{
			if(pData) {
				ushort comm_rt = 0, spec_rt = 0;
				getCtrlData(FLCtlID,  &comm_rt);
				getCtrlData(SFLCtlID, &spec_rt);
				pData->Flags = ((comm_rt & 0x00ff) | ((spec_rt << 8) & 0xff00));
				pData->OprFlags = PPRights::GetDefaultOprFlags();
				pData->Size = sizeof(ObjRights);
			}
			return 1;
		}
	private:
		uint   FLCtlID;
		uint   SFLCtlID;
	};
	int    r = 1;
	SpcRightFlagsDlg * dlg = 0;
	flCtlID  = NZOR(flCtlID,  CTL_RTOBJ_FLAGS);
	sflCtlID = NZOR(sflCtlID, CTL_RTOBJ_SFLAGS);
	THROW(CheckDialogPtr(&(dlg = new SpcRightFlagsDlg(dlgID, flCtlID, sflCtlID))));
	if(pDlg) {
		pDlg->Embed(dlg);
		r = 1;
	}
	else {
		THROW_PP(bufSize >= sizeof(ObjRights), PPERR_OBJRTBUFSIZ);
		dlg->setDTS(rt);
		if((r = ExecView(dlg)) == cmOK)
			dlg->getDTS(rt);
		else
			r = -1;
	}
	CATCH
		r = 0;
	ENDCATCH
	if(!pDlg)
		delete dlg;
	return r;
}

int PPObject::RemoveSync(PPID id)
{
	int    ok = 1;
	if(LConfig.DBDiv && Obj) {
		ObjSyncCore & r_sync = *DS.GetTLA().P_ObjSync;
		if(!r_sync.RemoveByPrivateID(Obj, id, 0))
			ok = 0;
	}
	return ok;
}

/*virtual*/int PPObject::RemoveObjV(PPID id, ObjCollection * pObjColl, uint options, void * pExtraParam)
{
	int    r;
	if(!(options & not_checkrights))
		THROW(CheckRights(PPR_DEL));
	r = (options & user_request) ? PPMessage(mfConf|mfYes|mfCancel, PPCFM_DELETE) : cmYes;
	if(r == cmYes) {
		if(!(options & no_wait_indicator))
			PPWaitStart();
		PPTransaction tra(BIN(options & use_transaction));
		THROW(tra);
		if(!(options & not_objnotify)) {
			THROW(SendObjMessage(DBMSG_OBJDELETE, 0, Obj, id, pExtraParam, pObjColl) == DBRPL_OK);
		}
		// @v10.5.3 @fix Следующие 2 оператора находились в блоке выше. По-этому при установленной опции not_objnotify объект фактически не удалялся!
		THROW(Search(id) > 0);
		THROW(DeleteObj(id));
		//
		THROW(PPRef->RemoveProperty(Obj, id, 0, 0));
		THROW(RemoveSync(id));
		if(!(options & not_addtolog))
			DS.LogAction(PPACN_OBJRMV, Obj, id, 0, 0);
		THROW(tra.Commit());
		r = 1;
	}
	else
		r = -1;
	CATCH
		if(options & PPObject::user_request)
			PPError();
		r = 0;
	ENDCATCH
	if(!(options & no_wait_indicator))
		PPWaitStop();
	return r;
}

IMPL_CMPFUNC(PPObjID, i1, i2) { return memcmp(i1, i2, sizeof(PPObjID)); }

/*static*/int FASTCALL PPObject::ProcessObjRefInArray(PPID objTypeID, PPID * pObjID, PPObjIDArray * pArray, int replace)
{
	int    ok = -1;
	if(objTypeID) {
		PPObjID * p_o;
		if(!replace)
			ok = pArray->Add(objTypeID, *pObjID);
		else {
			for(uint p = 0; ok < 0 && pArray->enumItems(&p, (void **)&p_o);) {
				if(p_o->Obj) {
					p_o->Obj = 0;
					*pObjID = p_o->Id;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int FASTCALL PPObject::ProcessObjRefInArray_NoPreprocess(PPID objTypeID, PPID * pObjID, PPObjIDArray * pArray, int replace)
{
	int    ok = -1;
	if(objTypeID) {
		PPObjID * p_o;
		if(!replace)
			ok = pArray->Add_NotPreprocess(objTypeID, *pObjID);
		else {
			for(uint p = 0; ok < 0 && pArray->enumItems(&p, (void **)&p_o);) {
				if(p_o->Obj) {
					p_o->Obj = 0;
					*pObjID = p_o->Id;
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int PPObject::ProcessObjListRefInArray(PPID objTypeID, PPIDArray & rList, PPObjIDArray * pArray, int replace)
{
	const  uint c = rList.getCount();
	for(uint i = 0; i < c; i++)
		if(!PPObject::ProcessObjRefInArray(objTypeID, &rList.at(i), pArray, replace))
			return 0;
	return 1;
}

/*virtual*/void * PPObject::CreateObjListWin(uint aFlags, void * extraPtr)
{
	return 0;
}

PPObject * FASTCALL GetPPObject(PPID obj, void * extraPtr)
{
	return _GetPPObject(obj, extraPtr);
}

int FASTCALL PPGetObjTypeList(PPIDArray * pList, long flags)
{
	int    ok = 1;
	if(pList) {
		pList->clear();
		for(uint i = 0; i < SIZEOFARRAY(_Objects); i++) {
			const  PPID obj_type = _Objects[i];
			int    skip = 0;
			if(flags & gotlfExcludeObsolete && oneof3(obj_type, PPOBJ_COUNTRY, PPOBJ_REGION, PPOBJ_CITY))
				skip = 1;
			else if(flags & gotlfExcludeObjBill && obj_type == PPOBJ_BILL)
				skip = 1;
			if(!skip)
				pList->add(obj_type);
		}
		if(!(flags & gotlfExcludeDyn))
			for(PPID dyn_obj_type = 0; PPRef->EnumItems(PPOBJ_DYNAMICOBJS, &dyn_obj_type) > 0;)
				pList->add(dyn_obj_type);
		pList->sortAndUndup();
	}
	else
		ok = PPSetErrorInvParam();
	return ok;
}
//
// ObjCollection
//
ObjCollectionEntry::ObjCollectionEntry() : ObjType(0), ObjPtr(0)
{
}

ObjCollection::ObjCollection() : TSArray <ObjCollectionEntry> (aryDataOwner | aryEachItem)
{
}

ObjCollection::~ObjCollection()
{
	ObjCollectionEntry * p_entry = 0;
	for(uint i = 0; enumItems(&i, (void **)&p_entry) > 0;)
		ZDELETE(p_entry->ObjPtr);
}

/*virtual*/void FASTCALL ObjCollection::freeItem(void * pItem)
{
	delete static_cast<ObjCollectionEntry *>(pItem)->ObjPtr;
	SArray::freeItem(pItem);
}

int FASTCALL ObjCollection::CreateFullList(long flags)
{
	int    ok = 1;
	PPIDArray obj_type_list;
	PPGetObjTypeList(&obj_type_list, flags);
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		GetObjectPtr(obj_type_list.get(i));
	}
	return ok;
}

PPObject * FASTCALL ObjCollection::GetObjectPtr(PPID objType)
{
	uint   p = 0;
	PPObject * p_obj = 0;
	if(bsearch(&objType, &p, CMPF_LONG))
		p_obj = at(p).ObjPtr;
	else {
		ObjCollectionEntry entry;
		entry.ObjType = objType;
		p_obj = entry.ObjPtr = GetPPObject(objType, 0);
		if(p_obj && !SArray::ordInsert(&entry, 0, CMPF_LONG)) {
			ZDELETE(p_obj);
			PPSetErrorSLib();
		}
	}
	return p_obj;
}

static int FASTCALL Helper_SendObjMessage(int msg, PPID destObj, PPID obj, PPID id, void * msgExtraPtr, ObjCollection * pDestObjColl)
{
	int    r = DBRPL_OK;
	PPIDArray obj_type_list;
	if(destObj)
		obj_type_list.add(destObj);
	else {
		PPGetObjTypeList(&obj_type_list, gotlfExcludeObsolete);
	}
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		const PPID obj_type = obj_type_list.get(i);
		if(!oneof5(obj_type, PPOBJ_ACCOUNT_PRE9004, PPOBJ_STAFFLIST_PRE9003, PPOBJ_COUNTRY, PPOBJ_CITY, PPOBJ_REGION)) {
			if(obj_type == PPOBJ_BILL && BillObj) {
				r = static_cast<PPObject *>(BillObj)->HandleMsg(msg, obj, id, msgExtraPtr);
			}
			else if(obj_type == PPOBJ_LOT)
				r = DBRPL_OK;
			else {
				PPObject * ppobj = 0;
				if(pDestObjColl) {
					ppobj = pDestObjColl->GetObjectPtr(obj_type);
					r = ppobj ? ppobj->HandleMsg(msg, obj, id, msgExtraPtr) : DBRPL_ERROR;
					ppobj = 0; // Указатель - собственность коллекции pDestObjColl и не может быть разрушен
				}
				else {
					ppobj = GetPPObject(obj_type, 0);
					r = ppobj ? ppobj->HandleMsg(msg, obj, id, msgExtraPtr) : DBRPL_ERROR;
				}
				delete ppobj;
			}
			if(r == DBRPL_CANCEL) {
				PPSetError(PPERR_USERBREAK);
				break;
			}
			else if(r == DBRPL_ERROR)
				break;
		}
	}
	return r;
}

int FASTCALL SendObjMessage(int msg, PPID destObj, PPID obj, PPID id, void * msgExtraPtr, ObjCollection * pDestObjColl)
	{ return Helper_SendObjMessage(msg, destObj, obj, id, msgExtraPtr, pDestObjColl); }
int FASTCALL SendObjMessage(int msg, PPID destObj, PPID obj, PPID id)
	{ return Helper_SendObjMessage(msg, destObj, obj, id, 0, 0); }

int FASTCALL BroadcastObjMessage(int msg, PPID srcObjType, PPID srcObjID, void * msgExtraPtr)
{
	PPIDArray obj_type_list;
	PPGetObjTypeList(&obj_type_list, gotlfExcludeObsolete);
	for(uint i = 0; i < obj_type_list.getCount(); i++) {
		const PPID obj_type = obj_type_list.at(i);
		if(obj_type != srcObjType)
			if(SendObjMessage(msg, obj_type, srcObjType, srcObjID, msgExtraPtr, 0) == DBRPL_ERROR)
				return 0;
	}
	return 1;
}

int FASTCALL GetObjectName(PPID obj, PPID id, char * pBuf, size_t bufLen)
{
	int    r = 0;
	ASSIGN_PTR(pBuf, 0);
	if(obj && id) {
		PPObject * p_o = GetPPObject(obj, 0);
		if(p_o) {
			r = p_o->GetName(id, pBuf, bufLen);
			delete p_o;
		}
	}
	return r;
}

int FASTCALL GetObjectName(PPID objType, PPID objID, SString & rBuf, int cat)
{
	int    ok = 0;
	char   temp_buf[256];
	SString _t;
	PTR32(temp_buf)[0] = 0;
	switch(objType) {
		case PPOBJ_PERSON: ok = GetPersonName(objID, _t); break;
		case PPOBJ_GOODS: ok = GetGoodsNameR(objID, _t); break;
		case PPOBJ_ARTICLE: ok = GetArticleName(objID, _t); break;
		case PPOBJ_LOT:
			{
				PPObjBill * p_bobj = BillObj;
				if(p_bobj) {
					ReceiptTbl::Rec lot_rec;
					if(p_bobj->trfr->Rcpt.Search(objID, &lot_rec) > 0) {
						ReceiptCore::MakeCodeString(&lot_rec, 0, _t);
						ok = 1;
					}
				}
			}
			break;
		case PPOBJ_AMOUNTTYPE: // @v10.0.0
			{
				PPObjAmountType amtt_obj;
				PPAmountType amtt_rec;
				if(amtt_obj.Fetch(objID, &amtt_rec) > 0) {
					_t = amtt_rec.Name;
					ok = 1;
				}
				else {
					ideqvalstr(objID, _t);
					ok = -1;
				}
			}
			break;
		default:
			ok = GetObjectName(objType, objID, temp_buf, sizeof(temp_buf));
			break;
	}
	if(cat) {
		if(_t.NotEmpty())
			rBuf.Cat(_t);
		else
			rBuf.Cat(temp_buf);
	}
	else {
		if(_t.NotEmpty())
			rBuf = _t;
		else
			rBuf.CopyFrom(temp_buf);
	}
	return ok;
}

SString & GetExtObjectName(const ObjIdListFilt & rObjList, PPID obj, size_t maxItems, SString & rBuf)
{
	if(rObjList.IsEmpty()) {
		PPLoadString("allobjects", rBuf);
	}
	else {
		for(uint i = 0; i < rObjList.GetCount(); i++) {
			if(i > 0)
				rBuf.CatDiv(';', 2);
			if(maxItems && i >= maxItems) {
				rBuf.Dot().Dot();
				break;
			}
			else
				GetObjectName(obj, rObjList.Get().at(i), rBuf, 1);
		}
	}
	return rBuf;
}

SString & FASTCALL GetObjectTitle(PPID objType, SString & rBuf)
{
	DS.GetObjectTitle(objType, rBuf);
	return rBuf;
}

PPID FASTCALL GetObjectTypeBySymb(const char * pSymb, long * pExtraParam)
{
	return DS.GetObjectTypeBySymb(pSymb, pExtraParam);
}

/*static*/int PPObject::Identify(const char * pObjType, const char * pObjIdent, PPObjID * pIdent)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPObjID ident;
	ident.Z();
	long   obj_type_ext = 0;
	long   int_type_id = atol(pObjType);
	if(!SETIFZ(ident.Obj, NZOR(int_type_id, GetObjectTypeBySymb(pObjType, &obj_type_ext)))) {
		PPSaveErrContext();
		if(p_ref->SearchSymb(PPOBJ_DYNAMICOBJS, &int_type_id, pObjType, offsetof(ReferenceTbl::Rec, Symb)) > 0)
			ident.Obj = int_type_id;
		PPRestoreErrContext();
	}
	THROW(ident.Obj);
	PPSetObjError(PPERR_OBJNFOUND, ident.Obj, ident.Id);
	THROW(pObjIdent);
	ident.Id = atol(pObjIdent);
	if(ident.Id) {
		ok = SearchObject(ident.Obj, ident.Id);
	}
	else {
		switch(ident.Obj) {
			case PPOBJ_UNIT:
			case PPOBJ_GOODSTYPE:
			case PPOBJ_GOODSCLASS:
			case PPOBJ_PERSONKIND:
			case PPOBJ_PRSNSTATUS:
			case PPOBJ_PRSNCATEGORY:
			case PPOBJ_GLOBALUSERACC:
			case PPOBJ_QUOTKIND:
			case PPOBJ_CURRENCY:
			case PPOBJ_CURRATETYPE:
				ok = p_ref->SearchSymb(ident.Obj, &ident.Id, pObjIdent, offsetof(ReferenceTbl::Rec, Symb));
				break;
			case PPOBJ_LOCATION:
				if(sstreqi_ascii(pObjIdent, "_undef")) {
					ident.Id = 0;
				}
				else {
					PPObjLocation loc_obj;
					ok = loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, pObjIdent, &ident.Id, 0);
				}
				break;
			default:
				if(IS_DYN_OBJTYPE(ident.Obj)) {
					ok = p_ref->SearchSymb(ident.Obj, &ident.Id, pObjIdent, offsetof(ReferenceTbl::Rec, Symb));
				}
				else {
					SString msg_buf;
					msg_buf.CatChar('{').Cat(pObjType).CatDiv(';', 2).Cat(pObjIdent).CatChar('}');
					CALLEXCEPT_PP_S(PPERR_CANTIDENTOBJBYSYMB, msg_buf);
				}
				break;
		}
	}
	CATCHZOK
	ASSIGN_PTR(pIdent, ident);
	return ok;
}

IMPL_CMPFUNC(Acct, k1, k2)
{
	const Acct * a1 = static_cast<const Acct *>(k1);
	Acct a2;
	a2.FromStr(0, static_cast<const char *>(k2));
	// @v10.3.1 {
	int    si = 0;
	CMPCASCADE2(si, a1, &a2, ac, sb);
	return si;
	// } @v10.3.1
	/* @v10.3.1
	if(a1->ac < a2.ac) return -1;
	if(a1->ac > a2.ac) return  1;
	if(a1->sb < a2.sb) return -1;
	if(a1->sb > a2.sb) return  1; // @v10.3.1 @fix (<)-->(>)
	return 0;
	*/
}

ListWindow * FASTCALL GetPPObjList(PPID obj, uint flags, void * extraPtr)
{
	PPObjListWindow * olw = 0;
	PPObject * o = GetPPObject(obj, extraPtr);
	if(o) {
		olw = new PPObjListWindow(o, flags, extraPtr);
		if(!olw)
			PPSetErrorNoMem();
	}
	return olw;
}

int FASTCALL SetupPPObjCombo(TDialog * dlg, uint ctl, PPID obj, PPID id, uint flags, void * extraPtr)
{
	int    ok = SetupPPObjCombo(static_cast<ComboBox *>(dlg->getCtrlView(ctl)), obj, id, flags, extraPtr);
	if(ok && flags & OLW_WORDSELECTOR)
		dlg->SetupWordSelector(ctl, 0, id, 2, WordSel_ExtraBlock::fAlwaysSearchBySubStr);
	return ok;
}

int FASTCALL SetupPPObjCombo(TDialog * dlg, uint ctl, PPID obj, PPID id, uint flags)
{
	int    ok = SetupPPObjCombo(static_cast<ComboBox *>(dlg->getCtrlView(ctl)), obj, id, flags, 0);
	if(ok && flags & OLW_WORDSELECTOR)
		dlg->SetupWordSelector(ctl, 0, id, 2, 0);
	return ok;
}

int FASTCALL SetupPPObjCombo(ComboBox * pCombo, PPID obj, PPID id, uint flags, void * extraPtr)
{
	int    ok = 1;
	if(pCombo) {
		ListWindow * p_lw = GetPPObjList(obj, flags, extraPtr);
		if(p_lw) {
			pCombo->setListWindow(p_lw);
			if(!p_lw->listBox()->def && flags & OLW_LOADDEFONOPEN) {
				SString name;
				GetObjectName(obj, id, name);
				pCombo->setInputLineText(name);
				pCombo->setUndefID(id);
			}
			else {
				if(id)
					pCombo->TransmitData(+1, &id);
				else {
					PPID   single_id = 0;
					if(flags & OLW_SETUPSINGLE && p_lw->getSingle(&single_id))
						pCombo->TransmitData(+1, &single_id);
					else {
						pCombo->setInputLineText(0);
						pCombo->setUndefTag(1);
					}
				}
			}
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL SetupObjListCombo(TDialog * dlg, uint ctl, PPID id, const PPIDArray * pInclList)
{
	ComboBox * p_cb = static_cast<ComboBox *>(dlg->getCtrlView(ctl));
	if(p_cb) {
		PPIDArray obj_type_list;
		SString obj_title;
		StrAssocArray * p_list = new StrAssocArray;
		if(pInclList)
			obj_type_list = *pInclList;
		else
			PPGetObjTypeList(&obj_type_list, 0);
		uint   i = obj_type_list.getCount();
		if(i) do {
			const  PPID obj_type = obj_type_list.at(--i);
			p_list->Add(obj_type, GetObjectTitle(obj_type, obj_title));
		} while(i);
		p_list->SortByText();
		p_cb->setListWindow(CreateListWindow(p_list, lbtDisposeData|lbtDblClkNotify), id);
		return 1;
	}
	else
		return -1;
}

int EditPPObj(PPID objType, PPID objID)
{
	int    ok = -1;
	if(objType && objID) {
		if(objType == PPOBJ_LOT) {
			PPObjBill * p_bobj = BillObj;
			if(p_bobj) {
				ok = p_bobj->EditLotSystemInfo(objID);
			}
		}
		else {
			PPObject * p_obj = GetPPObject(objType, 0);
			PPID   id = objID;
			if(p_obj) {
				if(p_obj->Edit(&id, 0) == cmOK)
					ok = 1;
				delete p_obj;
			}
			else
				ok = PPErrorZ();
		}
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL ShowObjects(PPID obj, void * extraPtr)
{
	int    ok = 0;
	PPObject * ppobj = GetPPObject(obj, extraPtr);
	PPObjListWindow * p_lw = ppobj ? static_cast<PPObjListWindow *>(ppobj->CreateObjListWin(OLW_CANEDIT|OLW_CANINSERT|OLW_CANDELETE, extraPtr)) : 0;
	SString obj_title;
	if(!p_lw || APPL->AddListToTree(static_cast<PPApp *>(APPL)->LastCmd, GetObjectTitle(obj, obj_title).Transf(CTRANSF_INNER_TO_OUTER).cptr(), p_lw) <= 0) {
		ok = 0;
		ZDELETE(p_lw);
	}
	else
		ok = 1;
	if(ok == 0) {
		ok = ppobj ? ppobj->Browse(extraPtr) : PPErrorZ();
		delete ppobj;
	}
	return ok;
}

int FASTCALL SearchObject(PPID obj, PPID id, void * b)
{
	int    r = -1;
	if(obj)
		if(id) {
			PPObject * ppobj = GetPPObject(obj, 0);
			r = ppobj ? ppobj->Search(id, b) : 0;
			delete ppobj;
		}
		else
			PPSetObjError(PPERR_OBJNFOUND, obj, id);
	return r;
}

int PPSelectObject(PPID objType, PPID * pObjID, uint titleID, void * extraPtr)
{
	int    ok = -1;
	PPID   id = DEREFPTRORZ(pObjID);
	SString temp_buf;
	ListWindow * p_lw = static_cast<ListWindow *>(ValidView(GetPPObjList(objType, OLW_CANINSERT, extraPtr)));
	if(p_lw) {
		if(id)
			p_lw->listBox()->TransmitData(+1, &id);
		if(titleID && PPLoadText(titleID, temp_buf))
			p_lw->setTitle(temp_buf);
		p_lw->changeBounds(TRect(0, 0, 216, 144)); // @v10.9.4 TRect(0, 0, 96, 36)-->TRect(0, 0, 160, 64) // @v11.1.9 (0, 0, 160, 64)-->(0, 0, 216, 144)
		p_lw->ViewOptions |= (ofCenterX | ofCenterY);
		if(ExecView(p_lw) == cmOK) {
			p_lw->getResult(&id);
			if(id > 0)
				ok = 1;
		}
		else
			ok = -1;
	}
	else
		ok = PPErrorZ();
	delete p_lw;
	ASSIGN_PTR(pObjID, id);
	return ok;
}

int FASTCALL CheckCfgRights(PPID id, ushort rt, int oprRights)
{
	int    ok = 1;
	if(!PPMaster) {
		if(oprRights)
			ok = 0;
		else {
			uint   pos = 0;
			ObjRestrictArray * p_list = DS.GetTLA().Rights.P_CfgList;
			if(p_list && p_list->getCount() && p_list->SearchItemByID(id, &pos) > 0)
				ok = BIN((p_list->at(pos).Flags & rt) == rt);
			else
				ok = 0;
		}
		if(ok == 0)
			PPSetError(PPERR_NORIGHTS);
	}
	return ok;
}

int PPGetConfigList(StrAssocArray * pAry)
{
	int    ok = -1;
	if(pAry) {
		SString buf, names;
		PPLoadText(PPTXT_CFGNAMES, names);
		StringSet ss(';', names);
		for(uint i = 0, j = 1; ss.get(&i, buf); j++)
			pAry->Add(PPOBJ_FIRST_CFG_OBJ + j, buf);
		ok = 1;
	}
	return ok;
}
//
//
//
#define MAX_LINKED_FILES 32

ObjLinkFiles::ObjLinkFiles()
{
	Clear();
}

ObjLinkFiles::ObjLinkFiles(PPID objType)
{
	if(objType)
		Init(objType);
	else
		Clear();
}

ObjLinkFiles::ObjLinkFiles(const ObjLinkFiles & rS)
{
	Copy(rS);
}

void ObjLinkFiles::SetMode_IgnoreCheckStorageDir(int set)
{
	SETFLAG(State, stIgnoreCheckStorageDir, set);
}

void ObjLinkFiles::Clear()
{
	Files.freeAll();
	State = 0;
	StoreDir.Z();
	ObjType = 0;
	ObjID = 0;
	FilesNums = 0;
	InitFilesNums = 0;
}

void FASTCALL ObjLinkFiles::Init(PPID objType)
{
	Clear();
	SString temp_buf;
	DBS.GetDbPath(DBS.GetDbPathID(), StoreDir);
	PPLoadText(PPTXT_LNKFILESDIR, temp_buf);
	StoreDir.SetLastSlash().Cat(temp_buf).SetLastSlash();
	ObjType = objType;
}

void ObjLinkFiles::Init(PPID objType, const char * pDir)
{
	Clear();
	(StoreDir = pDir).SetLastSlash();
	ObjType = objType;
}

int FASTCALL ObjLinkFiles::Copy(const ObjLinkFiles & rS)
{
	int    ok = 1;
	Clear();
	for(uint i = 0; i < rS.Files.getCount(); i++) {
		char * p_new_item = newStr(rS.Files.at(i));
		Files.insert(p_new_item);
	}
	FilesNums = rS.FilesNums;
	StoreDir  = rS.StoreDir;
	ObjType   = rS.ObjType;
	ObjID     = rS.ObjID;
	return ok;
}

ObjLinkFiles & FASTCALL ObjLinkFiles::operator = (const ObjLinkFiles & rSrc)
{
	Copy(rSrc);
	return *this;
}

long ObjLinkFiles::GetState() const { return State; }
uint ObjLinkFiles::GetCount() const { return Files.getCount(); }

int ObjLinkFiles::CheckFile(uint pos, const char * pPath)
{
	SString path(pPath);
	const char * p_exist_file = (pos < Files.getCount()) ? Files.at(pos) : 0;
	const int    is_store_path = BIN(!(State & stIgnoreCheckStorageDir) && path.HasPrefixNC(StoreDir));
	if(p_exist_file && is_store_path && path.CmpNC(p_exist_file) == 0)
		return -1;
	else
		return (path.NotEmptyS() && !is_store_path) ? 1 : PPSetError(PPERR_LNKFILESPATHINV);
}

int ObjLinkFiles::IsChanged(PPID id, PPID addedID /*=0*/) const
{
	ObjLinkFiles lnk_files = *this;
	lnk_files.Load(id, addedID);
	const  uint _c = lnk_files.GetCount();
	int    r = BIN(lnk_files.FilesNums == FilesNums && _c == GetCount());
	if(r) {
		for(uint i = 0; r && i < _c; i++)
			r = BIN(stricmp866(Files.at(i), lnk_files.Files.at(i)) == 0);
	}
	return !r;
}

int ObjLinkFiles::Replace(uint i, const char * pPath)
{
	int    ok = CheckFile(i, pPath);
	if(ok > 0) {
		Remove(i);
		Files.atInsert(i, newStr(pPath));
	}
	return ok;
}

int ObjLinkFiles::SetupZeroPositionFile(PPID objType, PPID objID, const char * pPath)
{
	int    ok = 1;
	Init(objType);
	THROW(Load(objID, 0L));
	THROW(Replace(0, pPath));
	THROW(Save(objID, 0L));
	CATCHZOK
	return ok;
}

int ObjLinkFiles::GetZeroPositionFile(PPID objType, PPID objID, SString & rPath)
{
	int    ok = 1;
	rPath.Z();
	Init(objType);
	THROW(Load(objID, 0L));
	THROW(At(0, rPath));
	CATCHZOK
	return ok;
}

/*
int ObjLinkFiles::Add(const char * pPath)
{
	int    ok = 1;
	SString buf;
	THROW(CheckFile(pPath));
	buf.Cat(MAX_LINKED_FILES);
	PPSetAddedMsgString(buf);
	THROW_PP(pPath && Files.getCount() < MAX_LINKED_FILES, PPERR_MAXLINKFILESREACHED);
	THROW(Files.insert(newStr(pPath)));
	CATCHZOK
	return ok;
}
*/

int ObjLinkFiles::Remove(uint i)
{
	int    ok = -1;
	if(i < GetCount()) {
		uint   num = 0;
		SString path;
		if(GetNum(Files.at(i), &num) > 0)
			FilesNums &= ~(1L << (num - 1));
		Files.atFree(i);
		ok = 1;
	}
	return ok;
}

int ObjLinkFiles::MakeFilePath(PPID objID, /*PPID addedID*/const char * pAddedStr, SString & rPath)
{
	SString added_buf(pAddedStr);
	if(added_buf.NotEmptyS()) {
		(rPath = StoreDir).Cat(ObjType).CatChar('@').Cat(objID).CatChar('@').Cat(added_buf).CatChar('@');
	}
	else
		(rPath = StoreDir).Cat(ObjType).CatChar('@').Cat(objID).CatChar('@');
	return 1;
}

int ObjLinkFiles::At(uint i, SString & rPath) const
{
	int    ok = 1;
	if(i < Files.getCount())
		rPath = Files.at(i);
	else {
		rPath.Z();
		ok = 0;
	}
	return ok;
}

int ObjLinkFiles::GetFreeNum(uint * pNum) const
{
	int    ok = 0;
	for(uint i = 1, k = 1; !ok && i > 0; (i <<= 1), k++)
		if(!(FilesNums & i)) {
			ASSIGN_PTR(pNum, k);
			ok = 1;
		}
	return ok;
}

ObjLinkFiles::Fns::Fns()
{
	Reset();
}

void ObjLinkFiles::Fns::Reset()
{
	ObjType = 0;
	ObjID = 0;
	AddedStr.Z();
	Cntr = 0;
	Ext[0] = 0;
}

/*static*/int ObjLinkFiles::SplitInnerFileName(const char * pFileName, Fns * pS)
{
	int    ok = 0;
	if(pFileName) {
		pS->Reset();
		SPathStruc ps(pFileName);
		STRNSCPY(pS->Ext, ps.Ext);
		SString temp_buf;
		StringSet ss('@', ps.Nam);
		uint i = 0;
		if(ss.get(&i, temp_buf)) {
			pS->ObjType = temp_buf.ToLong();
			if(ss.get(&i, temp_buf)) {
				pS->ObjID = temp_buf.ToLong();
				if(ss.getCount() > 3) { // Извлечем AddedID
					if(ss.get(&i, temp_buf)) {
						// @v9.3.9 pS->AddedID = temp_buf.ToLong();
						pS->AddedStr = temp_buf; // @v9.3.9
					}
				}
				if(ss.get(&i, temp_buf)) {
					pS->Cntr = temp_buf.ToLong();
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int ObjLinkFiles::GetNum(const char * pPath, uint * pNum)
{
	Fns s;
	if(SplitInnerFileName(pPath, &s)) {
		ASSIGN_PTR(pNum, (uint)s.Cntr);
		return 1;
	}
	else
		return -1;
}

int ObjLinkFiles::Load(PPID objID, const char * pAddedStr)
{
	int    ok = -1;
	Files.freeAll();
	if(StoreDir.NotEmptyS() && objID) {
		uint   i = 0;
		SString path;
		SDirEntry e;
		SPathStruc  struc;
		SDirec sdir;
		FilesNums = 0;
		MakeFilePath(objID, pAddedStr, path);
		path.CatChar('*').Dot().CatChar('*');
		for(sdir.Init(path); i < MAX_LINKED_FILES && sdir.Next(&e) > 0; i++) {
			uint   num = 0;
			(path = StoreDir).SetLastSlash().Cat(e.FileName);
			if(GetNum(path, &num) > 0) {
				FilesNums |= (1 << (num - 1));
				THROW_SL(Files.insert(newStr((const char *)path)));
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int ObjLinkFiles::Load(PPID objID, PPID addedID /*=0*/)
{
	if(addedID) {
		SString added_str;
		return Load(objID, added_str.Cat(addedID));
	}
	else
		return Load(objID, static_cast<const char *>(0));
}

int ObjLinkFiles::MakeExistsFilePath(uint i, PPID objID, /*PPID addedID*/const char * pAddedStr, SString & rPath)
{
	int    ok = 0;
	if(i < MAX_LINKED_FILES) {
		SString path;
		SDirEntry e;
		SDirec sdir;
		MakeFilePath(objID, pAddedStr, path);
		path.Cat(i).Dot().CatChar('*');
        sdir.Init(path);
		if(sdir.Next(&e) > 0) {
			(rPath = StoreDir).Cat(e.FileName);
			ok = 1;
		}
	}
	return ok;
}

int ObjLinkFiles::Helper_Save(PPID objID, const char * pAddedStr, uint idx, SString * pResultFileName)
{
	int    ok = 1;
	ASSIGN_PTR(pResultFileName, static_cast<const char *>(0));
	THROW(idx < Files.getCount());
	{
		uint   num = 0;
		const SString orig_path(Files.at(idx));
		THROW(CheckFile(idx, orig_path) > 0);
		THROW(GetFreeNum(&num) > 0);
		{
			SString path;
			SPathStruc ps(orig_path);
			(path = StoreDir).RmvLastSlash();
			if(createDir(path)) {
				MakeFilePath(objID, pAddedStr, path);
				path.Cat(num).Dot().Cat(ps.Ext);
				// @v9.3.9 ::CopyFile(orig_path, path, FALSE); // @unicodeproblem
				SCopyFile(orig_path, path, 0, FILE_SHARE_READ, 0); // @v9.3.9
				_chmod(path, _S_IREAD|_S_IWRITE);
				ASSIGN_PTR(pResultFileName, path);
			}
			else {
				PPSetErrorSLib();
				PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_USER);
				CALLEXCEPT_PP(PPERR_SLIB);
			}
		}
	}
	CATCHZOK
	return ok;
}

int ObjLinkFiles::Helper_ClearBeforeSaving(PPID objID, const char * pAddedStr)
{
	int    ok = -1;
	SDirEntry e;
	SString path;
	Fns    s;
	MakeFilePath(objID, pAddedStr, path);
	path.CatChar('*').Dot().CatChar('*');
	for(SDirec sdir(path); sdir.Next(&e) > 0;) {
		if(SplitInnerFileName(e.FileName, &s)) {
			if(s.Cntr <= MAX_LINKED_FILES && !(FilesNums & (1 << (s.Cntr-1)))) {
				(path = StoreDir).Cat(e.FileName);
				ok = SFile::Remove(path);
			}
		}
	}
	return ok;
}

int ObjLinkFiles::SaveSingle(PPID objID, const char * pAddedStr, uint idx, SString * pResultName)
{
	int    ok = -1;
	if(objID && idx < Files.getCount()) {
		Helper_ClearBeforeSaving(objID, pAddedStr);
		if(Helper_Save(objID, pAddedStr, idx, pResultName))
			ok = 1;
	}
	return ok;
}

int ObjLinkFiles::Save(PPID objID, const char * pAddedStr)
{
	int    ok = -1;
	if(objID) {
		Helper_ClearBeforeSaving(objID, pAddedStr);
		for(uint i = 0; i < Files.getCount(); i++) {
			if(Helper_Save(objID, pAddedStr, i, 0))
				ok = 1;
		}
	}
	return ok;
}

int ObjLinkFiles::Save(PPID objID, PPID addedID)
{
	if(addedID) {
		SString added_str;
		return Save(objID, added_str.Cat(addedID));
	}
	else
		return Save(objID, static_cast<const char *>(0));
}

int ObjLinkFiles::Serialize(int dir, int transmissionNotSupported, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	const size_t file_chunk_size = 8 * 1024 * 1024;

	int    ok = 1;
	uint32 _c = 0; // Количество файлов
	int32  transmission_not_supported = BIN(dir > 0 && transmissionNotSupported);
	SString file_path, org_file_name;
	State &= ~stTransmissionNotSupported;

	THROW_SL(pSCtx->Serialize(dir, transmission_not_supported, rBuf));
	if(transmission_not_supported) {
		if(dir < 0) {
			Clear();
			State |= stTransmissionNotSupported;
			ok = -1;
		}
	}
	else {
		THROW_SL(pSCtx->Serialize(dir, ObjType, rBuf));
		THROW_SL(pSCtx->Serialize(dir, ObjID, rBuf));
		THROW_SL(pSCtx->Serialize(dir, FilesNums, rBuf));
		THROW_SL(pSCtx->Serialize(dir, InitFilesNums, rBuf));
		THROW_SL(pSCtx->Serialize(dir, StoreDir, rBuf)); // @?
		if(dir > 0) {
			_c = Files.getCount();
			THROW_SL(pSCtx->Serialize(dir, _c, rBuf));
			for(uint i = 0; i < _c; i++) {
				int64  fs = 0;
				org_file_name = Files.at(i);
				THROW_SL(pSCtx->Serialize(dir, org_file_name, rBuf));
				if(fileExists(org_file_name)) {
					SFile f(org_file_name, SFile::mRead|SFile::mBinary);
					THROW_SL(f.IsValid());
					THROW_SL(f.CalcSize(&fs));
					THROW_SL(pSCtx->Serialize(dir, fs, rBuf));
					{
						int64  fp = 0;
						STempBuffer _buf(file_chunk_size);
						THROW_SL(_buf.IsValid());
						while(fp < fs) {
							size_t actual_size = 0;
							THROW_SL(f.Read(_buf, _buf.GetSize(), &actual_size));
							THROW_SL(rBuf.Write(_buf, actual_size));
							fp += actual_size;
						}
						assert(fp == fs);
					}
				}
				else {
					THROW_SL(pSCtx->Serialize(dir, fs, rBuf));
				}
			}
		}
		else if(dir < 0) {
			Files.freeAll();
			SString temp_path;
			{
				DBS.GetDbPath(DBS.GetDbPathID(), StoreDir);
				PPLoadText(PPTXT_LNKFILESDIR, temp_path.Z());
				StoreDir.SetLastSlash().Cat(temp_path).SetLastSlash();
			}
			PPGetPath(PPPATH_TEMP, temp_path.Z());
			THROW_SL(pSCtx->Serialize(dir, _c, rBuf));
			for(uint i = 0; i < _c; i++) {
				int64  fs = 0;
				THROW_SL(pSCtx->Serialize(dir, org_file_name, rBuf));
				THROW_SL(pSCtx->Serialize(dir, fs, rBuf));
				{
					SPathStruc ps(org_file_name);
					MakeTempFileName(temp_path, "olf", ps.Ext, 0, file_path);
					SFile f(file_path, SFile::mWrite|SFile::mBinary);
					THROW_SL(f.IsValid());
					if(fs) {
						int64  fp = 0;
						STempBuffer _buf(file_chunk_size);
						THROW_SL(_buf.IsValid());
						while(fp < fs) {
							const size_t actual_size = (size_t)MIN((int64)_buf.GetSize(), (fs - fp));
							THROW_SL(rBuf.Read(_buf, actual_size));
							THROW_SL(f.Write(_buf, actual_size));
							fp += actual_size;
						}
					}
				}
				THROW(Replace(i, file_path));
			}
		}
	}
	CATCHZOK
	return ok;
}
//
// Implementation of PPALDD_DataObject
//
PPALDD_CONSTRUCTOR(DataObject)
{
	if(Valid)
		AssignHeadData(&H, sizeof(H));
}

PPALDD_DESTRUCTOR(DataObject) { Destroy(); }

int PPALDD_DataObject::InitData(PPFilt & rFilt, long rsrv)
{
	int    ok = -1;
	if(rFilt.ID == H.ID)
		ok = DlRtm::InitData(rFilt, rsrv);
	else {
		MEMSZERO(H);
		H.ID = rFilt.ID;
		if(H.ID) {
			SString obj_title;
			GetObjectTitle(rFilt.ID, obj_title).CopyTo(H.Name, sizeof(H.Name));
			ok = DlRtm::InitData(rFilt, rsrv);
		}
	}
	return ok;
}
//
// Implementation of PPGlobalAccRights
//
PPGlobalAccRights::Rec::Rec() : Flags(0)
{
	PTR32(Scope)[0] = 0;
}

PPGlobalAccRights::PPGlobalAccRights(const int tagID)
{
	const PPID glob_acc_id = DS.GetConstTLA().GlobAccID;
	if(glob_acc_id > 0) {
		ObjTagItem tag;
		if(glob_acc_id == PPGUAID_UHTT_CORE) {
			PPGlobalAccRights::Rec perm_rec;
			STRNSCPY(perm_rec.Scope, "@def");
			perm_rec.Flags = fAccess | fCreate | fEdit | fDelete | fOperation | fAccessByID | fAccessBySymb;
			Recs.insert(&perm_rec);
		}
		else if(PPRef->Ot.GetTag(PPOBJ_GLOBALUSERACC, glob_acc_id, tagID, &tag) > 0)
			Parse(SString(tag.Val.PStr));
	}
}

PPGlobalAccRights::PPGlobalAccRights(const SString & rStr)
{
	Parse(rStr);
}

PPGlobalAccRights::~PPGlobalAccRights()
{
	Recs.freeAll();
}

/*static*/int PPGlobalAccRights::EditDialog(PPID tagID, SString & rLine)
{
	int    ok = -1;
	TDialog * dlg = new TDialog(DLG_GUART);
	if(CheckDialogPtrErr(&dlg)) {
		long   flags = 0;
		uint   i;
		for(i = 0; i < rLine.Len(); i++) {
			switch(toupper(rLine.C(i))) {
				case 'A': flags |= PPGlobalAccRights::fAccess; break;
				case 'C': flags |= PPGlobalAccRights::fCreate; break;
				case 'E': flags |= PPGlobalAccRights::fEdit; break;
				case 'D': flags |= PPGlobalAccRights::fDelete; break;
				case 'O': flags |= PPGlobalAccRights::fOperation; break;
				case 'I': flags |= PPGlobalAccRights::fAccessByID; break;
				case 'S': flags |= PPGlobalAccRights::fAccessBySymb; break;
			}
		}
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 0, PPGlobalAccRights::fAccess);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 1, PPGlobalAccRights::fCreate);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 2, PPGlobalAccRights::fEdit);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 3, PPGlobalAccRights::fDelete);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 4, PPGlobalAccRights::fOperation);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 5, PPGlobalAccRights::fAccessByID);
		dlg->AddClusterAssoc(CTL_GUART_FLAGS, 6, PPGlobalAccRights::fAccessBySymb);
		if(tagID != PPTAG_GUA_SALOCRIGHTS) {
			dlg->DisableClusterItem(CTL_GUART_FLAGS, 5, 1);
			dlg->DisableClusterItem(CTL_GUART_FLAGS, 6, 1);
			flags |= (PPGlobalAccRights::fAccessByID|PPGlobalAccRights::fAccessBySymb);
		}
		dlg->SetClusterData(CTL_GUART_FLAGS, flags);
		if(ExecView(dlg) == cmOK) {
			flags = dlg->GetClusterData(CTL_GUART_FLAGS);
			if(tagID != PPTAG_GUA_SALOCRIGHTS) {
				flags &= ~(PPGlobalAccRights::fAccessByID|PPGlobalAccRights::fAccessBySymb);
			}
			rLine.Z();
			if(flags & PPGlobalAccRights::fAccess) rLine.CatChar('A');
			if(flags & PPGlobalAccRights::fCreate) rLine.CatChar('C');
			if(flags & PPGlobalAccRights::fEdit)   rLine.CatChar('E');
			if(flags & PPGlobalAccRights::fDelete) rLine.CatChar('D');
			if(flags & PPGlobalAccRights::fOperation) rLine.CatChar('O');
			if(flags & PPGlobalAccRights::fAccessByID) rLine.CatChar('I');
			if(flags & PPGlobalAccRights::fAccessBySymb) rLine.CatChar('S');
			ok = 1;
		}
	}
	else
		ok = 0;
	return ok;
}

int PPGlobalAccRights::Parse(const SString & rStr)
{
	int    ok = 1;
	SString rec, s1, s2, temp_buf;
	PPGlobalAccRights::Rec perm_rec;
	for(SString tail = rStr; tail.NotEmptyS();) {
		temp_buf = tail;
		temp_buf.Divide(';', rec.Z(), tail.Z());
		if(rec.Divide(':', s1, s2) > 0) {
			if(s1.IsEmpty())
				STRNSCPY(perm_rec.Scope, "@def");
			else
				STRNSCPY(perm_rec.Scope, s1);
			temp_buf = s2;
		}
		else {
			STRNSCPY(perm_rec.Scope, "@def");
			temp_buf = s1;
		}
		if(temp_buf.HasChr('A'))
			perm_rec.Flags |= fAccess;
		if(temp_buf.HasChr('C'))
			perm_rec.Flags |= fCreate;
		if(temp_buf.HasChr('E'))
			perm_rec.Flags |= fEdit;
		if(temp_buf.HasChr('D'))
			perm_rec.Flags |= fDelete;
		if(temp_buf.HasChr('O'))
			perm_rec.Flags |= fOperation;
		if(temp_buf.HasChr('I'))
			perm_rec.Flags |= fAccessByID;
		if(temp_buf.HasChr('S'))
			perm_rec.Flags |= fAccessBySymb;
		{
			int    found = 0;
			for(uint i = 0; !found && i < Recs.getCount(); i++) {
				Rec & r_item = Recs.at(i);
				if(stricmp(perm_rec.Scope, r_item.Scope) == 0) {
					r_item.Flags |= perm_rec.Flags;
					found = 1;
				}
			}
			if(!found)
				Recs.insert(&perm_rec);
		}
	}
	return ok;
}

int PPGlobalAccRights::IsAllow(int flags, const char * pScope) const
{
	int    r = 0;
	if(pScope) {
		for(uint i = 0; i < Recs.getCount(); i++) {
			const Rec & r_item = Recs.at(i);
			if(stricmp(pScope, r_item.Scope) == 0) {
				r = ((r_item.Flags & flags) == flags);
				break;
			}
		}
	}
	return r;
}
//
// Implementation of PPALDD_StrAssocArray
//
PPALDD_CONSTRUCTOR(StrAssocArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(StrAssocArray) { Destroy(); }

int PPALDD_StrAssocArray::InitData(PPFilt & rFilt, long rsrv)
{
	StrAssocArray * p_list = static_cast<StrAssocArray *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_StrAssocArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	StrAssocArray * p_list = static_cast<StrAssocArray *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_StrAssocArray::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	StrAssocArray * p_list = static_cast<StrAssocArray *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		StrAssocArray::Item item = p_list->Get(p_list->getPointer());
		I.Id = item.Id;
		I.ParentId = item.ParentId;
		STRNSCPY(I.Txt, item.Txt);
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
//
// Implementation of PPALDD_IntArray
//
PPALDD_CONSTRUCTOR(IntArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(IntArray) { Destroy(); }

int PPALDD_IntArray::InitData(PPFilt & rFilt, long rsrv)
{
	LongArray * p_list = static_cast<LongArray *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_IntArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	LongArray * p_list = static_cast<LongArray *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_IntArray::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	LongArray * p_list = static_cast<LongArray *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		I.Id = p_list->get(p_list->getPointer());
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
//
// Implementation of PPALDD_QuotArray
//
PPALDD_CONSTRUCTOR(QuotArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(QuotArray) { Destroy(); }

int PPALDD_QuotArray::InitData(PPFilt & rFilt, long rsrv)
{
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_QuotArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_QuotArray::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		PPQuotItem_ & r_item = p_list->at(p_list->getPointer());
		I.GoodsID = r_item.GoodsID;
		I.KindID = r_item.KindID;
		I.LocID = r_item.LocID;
		I.ArID = r_item.ArID;
		I.CurID = r_item.CurID;
		I.Dt = r_item.Dtm.d;
		I.Tm = r_item.Dtm.t;
		I.Flags = r_item.Flags;
		I.Val = r_item.Val;
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}

/*
void PPALDD_QuotArray::EvaluateFunc(const DlFunc * pF, SV_Uint32 * pApl, RtmStack & rS)
{
	#define _ARG_LONG(n) (*static_cast<const long *>(rS.GetPtr(pApl->Get(n))))
	#define _RET_STR     (**static_cast<SString **>(rS.GetPtr(pApl->Get(0))))
	if(pF->Name == "?GetRest") {
		PPID goods_id = _ARG_LONG(1);
		PPID loc_id = _ARG_LONG(2);
		long flags = _ARG_LONG(3);
		SString temp_buf;
		if(goods_id && loc_id) {
			double rest = 0.0;
			PPID   rest_op_id = 0;
			PPID   order_op_id = 0;
			PPOprKind op_rec;
			if(GetOpBySymb("GOODSREST", &op_rec) > 0)
				rest_op_id = op_rec.ID;
			if(GetOpBySymb("DRAFTORDER", &op_rec) > 0)
				order_op_id = op_rec.ID;
			if(rest_op_id)
				BillObj->CalcDraftTransitRest(rest_op_id, order_op_id, goods_id, loc_id, 0, &rest, 0);
			temp_buf.Z().Cat(rest, MKSFMTD(0, 6, flags));
		}
		_RET_STR = temp_buf;
	}
}
*/
//
// Implementation of PPALDD_UhttStoreItemArray
//
PPALDD_CONSTRUCTOR(UhttStoreItemArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(UhttStoreItemArray) { Destroy(); }

int PPALDD_UhttStoreItemArray::InitData(PPFilt & rFilt, long rsrv)
{
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttStoreItemArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_UhttStoreItemArray::NextIteration(long iterId)
{
	int    ok = -1;
	IterProlog(iterId, 0);
	PPQuotItemArray * p_list = static_cast<PPQuotItemArray *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		PPQuotItem_ & r_item = p_list->at(p_list->getPointer());
		I.GoodsID = r_item.GoodsID;
		I.CurID = r_item.CurID;
		I.Dt = r_item.Dtm.d;
		I.Tm = r_item.Dtm.t;
		I.Value = r_item.Val;
		if((r_item.GoodsID > 0) && (r_item.LocID > 0)) {
			PPOprKind op_rec;
			const PPID rest_op_id = (GetOpBySymb("GOODSREST", &op_rec) > 0) ? op_rec.ID : 0;
			const PPID order_op_id = (GetOpBySymb("DRAFTORDER", &op_rec) > 0) ? op_rec.ID : 0;
			if(rest_op_id)
				BillObj->CalcDraftTransitRest(rest_op_id, order_op_id, r_item.GoodsID, r_item.LocID, 0, &I.Rest, 0);
		}
		I.Flags = r_item.Flags;
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
//
// Implementation of PPALDD_UhttCurRateArray
//
PPALDD_CONSTRUCTOR(UhttCurRateArray)
{
	InitFixData(rscDefHdr, &H, sizeof(H));
	InitFixData(rscDefIter, &I, sizeof(I));
}

PPALDD_DESTRUCTOR(UhttCurRateArray) { Destroy(); }

int PPALDD_UhttCurRateArray::InitData(PPFilt & rFilt, long rsrv)
{
	TSArray <UhttCurRateIdent> * p_list = static_cast<TSArray <UhttCurRateIdent> *>(rFilt.Ptr);
	Extra[0].Ptr = p_list;
	return DlRtm::InitData(rFilt, rsrv);
}

int PPALDD_UhttCurRateArray::InitIteration(long iterId, int sortId, long rsrv)
{
	IterProlog(iterId, 1);
	TSArray <UhttCurRateIdent> * p_list = static_cast<TSArray <UhttCurRateIdent> *>(Extra[0].Ptr);
	CALLPTRMEMB(p_list, setPointer(0));
	return -1;
}

int PPALDD_UhttCurRateArray::NextIteration(long iterId)
{
	int     ok = -1;
	SString temp_buf;
	IterProlog(iterId, 0);
	TSArray <UhttCurRateIdent> * p_list = static_cast<TSArray <UhttCurRateIdent> *>(Extra[0].Ptr);
	if(p_list && p_list->getPointer() < p_list->getCount()) {
		UhttCurRateIdent & r_item = p_list->at(p_list->getPointer());
		I.CurID = r_item.Ident.CurID;
		I.BaseCurID = r_item.Ident.BaseCurID;
		I.RateTypeID = r_item.Ident.RateTypeID;
		I.Dt = r_item.Ident.Dt;
		{
			struct tm _tm;
			MEMSZERO(_tm);
			_tm.tm_year = r_item.Ident.Dt.year() - 1900;
			_tm.tm_mon = r_item.Ident.Dt.month() - 1;
			_tm.tm_mday = r_item.Ident.Dt.day();
			__int64 ts = mktime(&_tm);
			ts *= 1000;
			temp_buf.Z().Cat(ts);
			STRNSCPY(I.Ts, temp_buf);
		}
		I.Rate = r_item.Rate;
		p_list->incPointer();
		ok = DlRtm::NextIteration(iterId);
	}
	return ok;
}
//
// @construction {
//
struct UniFindObjParam {
	UniFindObjParam() : WFlags(0), DFlags(0)
	{
		Period.Z();
	}
	enum {
		wfBill        = 0x0001,
		wfGoods       = 0x0002,
		wfPerson      = 0x0004,
		wfSCard       = 0x0008,
		wfCCheck      = 0x0010,
		wfPersonEvent = 0x0020,
		wfProject     = 0x0040,
		wfToDo        = 0x0080
	};
	enum {
		dfInteractive = 0x0001
	};
	long   WFlags;
	long   DFlags;
	SString MainPattern;
	DateRange Period;
};

class UniFindObjParamDialog : public TDialog {
	DECL_DIALOG_DATA(UniFindObjParam);
public:
	UniFindObjParamDialog() : TDialog(DLG_UNIFINDOBJ)
	{
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		ASSIGN_PTR(pData, Data);
		return ok;
	}
};

int UniFindObj()
{
	int    ok = -1;
	UniFindObjParamDialog * dlg = new UniFindObjParamDialog();
	UniFindObjParam param;
	if(CheckDialogPtrErr(&dlg)) {
		dlg->setDTS(&param);
		if(ExecView(dlg) == cmOK) {
			if(dlg->getDTS(&param)) {
				//
			}
		}
	}
	delete dlg;
	return ok;
}
// } @construction
