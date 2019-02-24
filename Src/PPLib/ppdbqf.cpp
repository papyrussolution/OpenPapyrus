// PPDBQF.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
//
#include <pp.h>
#pragma hdrstop

#if 0 // {
//static
PPID FASTCALL PPDbqFuncPool::helper_dbq_name(const DBConst * params, char * pNameBuf)
{
	PPID   id = 0;
	if(params[0].tag == DBConst::lv)
		id = params[0].lval;
	else if(params[0].tag == DBConst::rv)
		id = (long)params[0].rval;
	pNameBuf[0] = 0;
	return id;
}
#endif // } 0

template <class objcls, class objrec> inline void dbqf_objname_i(int option, DBConst * result, const DBConst * params)
{
	objrec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id) {
			objcls obj;
			obj.Fetch(id, &rec);
			if(rec.Name[0] == 0)
				ideqvalstr(id, rec.Name, sizeof(rec.Name));
		}
		// @v9.9.4
		//result->init(rec.Name);

		result->InitForeignStr(SLS.AcquireRvlStr() = rec.Name); // @v9.9.4
	}
}

static IMPL_DBE_PROC(dbqf_oidtext_ii)
{
    char   name_buf[128+48];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		name_buf[0] = 0;
		PPID   obj_type = params[0].lval;
		PPID   obj_id = params[1].lval;
		if(obj_type) {
			SString temp_buf, obj_buf;
            GetObjectTitle(obj_type, temp_buf);
			if(temp_buf.NotEmptyS()) {
				temp_buf.CatDiv(':', 2);
				GetObjectName(obj_type, obj_id, obj_buf);
				temp_buf.Cat(obj_buf);
				STRNSCPY(name_buf, temp_buf);
			}
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_scardextstring_ii)
{
    char   name_buf[256];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		name_buf[0] = 0;
		PPID   id = params[0].lval;
		PPID   fldid = params[1].lval;
        PPObjSCard sc_obj;
        SString & r_temp_buf = SLS.AcquireRvlStr();
        if(sc_obj.FetchExtText(id, fldid, r_temp_buf) > 0) {
			STRNSCPY(name_buf, r_temp_buf);
        }
		result->init(name_buf);
	}
}

template <class objcls, class objrec> inline void dbqf_objsymb_i(int option, DBConst * result, const DBConst * params)
{
	objrec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Symb));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Symb);
		if(id) {
			objcls obj;
			obj.Fetch(id, &rec);
		}
		result->init(rec.Symb);
	}
}

static IMPL_DBE_PROC(dbqf_empty)
{
	if(option == CALC_SIZE)
		result->init((long)4);
	else {
		union {
			char   buf[16];
			uint32 zero;
		};
		zero = 0;
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_tseslnflags_i)
{
	if(option == CALC_SIZE)
		result->init((long)8);
	else {
		long   flags = params[0].lval;
		char   buf[8];
		if(flags & TSESLF_TOOLING)
			buf[0] = 'T';
		else if(flags & TSESLF_AUTOCOMPL)
			buf[0] = 'A';
		else
			buf[0] = 0;
		buf[1] = 0;
		result->init(buf);
	}
}
//
// GoodsID, Flags, Qtty, WtQtty
//
static IMPL_DBE_PROC(dbqf_tseslnphqtty_iirr)
{
	PPID   goods_id = params[0].lval;
	long   f        = params[1].lval;
	double qtty     = params[2].rval;
	double wtqtty   = params[3].rval;
	double phqtty = 0;
	if(f & TSESLF_INDEPPHQTTY)
		phqtty = wtqtty;
	else if(f & TSESLF_PLAN_PHUNIT)
		phqtty = qtty;
	else {
		double phuperu;
		PPObjGoods goods_obj;
		goods_obj.GetPhUPerU(goods_id, 0, &phuperu);
		phqtty = qtty * phuperu;
	}
	result->init(phqtty);
}

static IMPL_DBE_PROC(dbqf_debt_rrii)
{
	long   f = params[2].lval;
	long   op_id = params[3].lval;
	double r = ((f & BILLF_NEEDPAYMENT) || CheckOpFlags(op_id, OPKF_RECKON)) ? (params[0].rval - params[1].rval) : 0;
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_billfrghtissuedt_i)
{
	PPFreight freight;
	result->init((BillObj->FetchFreight(params[0].lval, &freight) > 0) ? freight.IssueDate : ZERODATE);
}

static IMPL_DBE_PROC(dbqf_billdate_i)
{
	BillTbl::Rec bill_rec;
	const PPID bill_id = params[0].lval;
	if(bill_id && BillObj->Fetch(bill_id, &bill_rec) > 0)
		result->init(bill_rec.Dt);
	else
		result->init(ZERODATE);
}

static IMPL_DBE_PROC(dbqf_billfrghtarrvldt_i)
{
	PPFreight freight;
	result->init((BillObj->FetchFreight(params[0].lval, &freight) > 0) ? freight.ArrivalDate : ZERODATE);
}

static IMPL_DBE_PROC(dbqf_billfrghtstrgloc_i)
{
	char   ret_buf[48];
	if(option == CALC_SIZE)
		result->init((long)sizeof(ret_buf));
	else {
		PPFreight freight;
		SString temp_buf;
		if(BillObj->FetchFreight(params[0].lval, &freight) > 0) {
			if(freight.StorageLocID) {
				PPObjLocation obj;
				LocationTbl::Rec rec;
				if(obj.Fetch(freight.StorageLocID, &rec) > 0)
					temp_buf = rec.Name;
			}
		}
		STRNSCPY(ret_buf, temp_buf);
		result->init(ret_buf);
	}
}

static IMPL_DBE_PROC(dbqf_billfrghtdlvraddr_i)
{
	char   ret_buf[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(ret_buf));
	else {
		PPFreight freight;
		SString temp_buf;
		if(BillObj->FetchFreight(params[0].lval, &freight) > 0) {
			if(freight.DlvrAddrID) {
				PPObjLocation obj;
				LocationTbl::Rec rec;
				if(obj.Fetch(freight.DlvrAddrID, &rec) > 0) {
					if(LocationCore::IsEmptyAddressRec(rec))
						temp_buf = "#EMPTY";
					else {
						LocationCore::GetAddress(rec, 0, temp_buf);
						int    name_used = 0;
						if(!temp_buf.NotEmptyS()) {
                            temp_buf = rec.Name;
							name_used = 1;
						}
						if(!temp_buf.NotEmptyS())
                            temp_buf = rec.Code;
						if(!temp_buf.NotEmptyS())
							LocationCore::GetExField(&rec, LOCEXSTR_CONTACT, temp_buf);
						if(!temp_buf.NotEmptyS())
							LocationCore::GetExField(&rec, LOCEXSTR_PHONE, temp_buf);
						if(!temp_buf.NotEmptyS())
							temp_buf = "#EMPTY";
						else if(!name_used && rec.Name[0]) {
							SString & r_tt = SLS.AcquireRvlStr();
							(r_tt = rec.Name).CatDiv('-', 1).Cat(temp_buf);
							temp_buf = r_tt;
						}
					}
				}
				else
					ideqvalstr(freight.DlvrAddrID, temp_buf);
			}
		}
		STRNSCPY(ret_buf, temp_buf);
		result->init(ret_buf);
	}
}

static IMPL_DBE_PROC(dbqf_billagentname_i)
{
	if(option == CALC_SIZE)
		result->init((long)sizeof(((ArticleTbl::Rec *)0)->Name));
	else {
		PPBillExt ext_rec;
		ArticleTbl::Rec ar_rec;
		ar_rec.Name[0] = 0;
        if(BillObj->FetchExt(params[0].lval, &ext_rec) > 0 && ext_rec.AgentID) {
			PPObjArticle ar_obj;
			ar_obj.Fetch(ext_rec.AgentID, &ar_rec);
			if(ar_rec.Name[0] == 0)
				ideqvalstr(ext_rec.AgentID, ar_rec.Name, sizeof(ar_rec.Name));
        }
		result->init(ar_rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_registertext_i)
{
	const  size_t buffer_size = 64;
	if(option == CALC_SIZE)
		result->init((long)buffer_size);
	else {
		SString temp_buf;
		const PPID reg_id = params[0].lval;
		if(reg_id) {
			PPObjRegister reg_obj;
			RegisterTbl::Rec reg_rec;
			if(reg_obj.Fetch(reg_id, &reg_rec) > 0) {
				PPObjRegister::Format(reg_rec, "@regsn - @regno [@date..@expiry]", temp_buf);
			}
		}
		result->init(temp_buf.Trim(buffer_size-1));
	}
}

static IMPL_DBE_PROC(dbqf_objtagtext_ii)
{
	const  size_t buffer_size = 128;
	if(option == CALC_SIZE)
		result->init((long)buffer_size);
	else {
		SString temp_buf;
		const PPID tag_id = params[0].lval;
		const PPID obj_id = params[1].lval;
		if(tag_id && obj_id) {
			PPObjTag tag_obj;
			ObjTagItem tag_item;
			if(tag_obj.FetchTag(obj_id, tag_id, &tag_item) > 0) {
				tag_item.GetStr(temp_buf);
			}
		}
		result->init(temp_buf.Trim(buffer_size-1));
	}
}

static IMPL_DBE_PROC(dbqf_cqtty_rrii)
{
	if(option == CALC_SIZE)
		result->init((long)24);
	else {
		double rest = 0.0;
		if(params[0].Tag == DBConst::lv)
			rest = params[0].lval;
		else if(params[0].Tag == DBConst::rv)
			rest = params[0].rval;
		double upp   = params[1].rval;
		long   trust = params[2].lval;
		long   flags = params[3].lval;
		char   buf[256];
		buf[0] = 0;
		char * p = buf;
		if(!trust)
			*p++ = '#';
		QttyToStr(rest, upp, flags, p, 1); // @@v6.3.1 noabs=1
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_trfrprice_irrr)
{
	uint   p = 0;
	long   op_id    = params[p++].lval;
	long   lot_id   = params[p++].lval;
	LDATE  dt       = params[p++].dval;
	long   oprno    = params[p++].lval;
	double cost     = params[p++].rval;
	double price    = params[p++].rval;
	double discount = params[p++].rval;
	double r = 0;
	if(GetOpType(op_id) == PPOPT_GOODSREVAL) {
		PPObjBill * p_bobj = BillObj;
		ReceiptTbl::Rec lot_rec;
		MEMSZERO(lot_rec);
		lot_rec.ID = lot_id;
		if(p_bobj->trfr->GetLotPrices(&lot_rec, dt, oprno) > 0)
			r = lot_rec.Price;
		else if(p_bobj->trfr->Rcpt.Search(lot_id, &lot_rec) > 0)
			r = TR5(lot_rec.Price);
		else
			r = price;
	}
	else
		r = price - discount;
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_datetime_dt)
{
	const size_t field_len = 20;
	if(option == CALC_SIZE)
		result->init((long)field_len);
	else {
		LDATETIME dtm;
		dtm.Set(params[0].dval, params[1].tval);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		result->init(r_temp_buf.Cat(dtm).Trim(field_len));
	}
}

static IMPL_DBE_PROC(dbqf_durationtotime_dt)
{
	const size_t field_len = 20;
	if(option == CALC_SIZE)
		result->init((long)field_len);
	else {
		LDATETIME dtm = ZERODATETIME;
		long days = dtm.settotalsec(params[0].lval);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		if(days)
			r_temp_buf.Cat(days).CatChar('d').Space();
		result->init(r_temp_buf.Cat(dtm.t, TIMF_HMS).Trim(field_len-1));
	}
}

static IMPL_DBE_PROC(dbqf_world_ismemb_ii)
{
	long   r = 0;
	if(params[0].lval && params[1].lval) {
		PPObjWorld w_obj;
		r = w_obj.IsChildOf(params[0].lval, params[1].lval) ? 1 : 0;
	}
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_checkcsposnode_ii) // (csessID, posNodeID)
{
	long   r = 0;
	if(params[1].lval) {
		if(params[0].lval) {
			PPObjCSession cs_obj;
			CSessionTbl::Rec cs_rec;
			if(cs_obj.Fetch(params[0].lval, &cs_rec) > 0 && cs_rec.CashNodeID == params[1].lval)
				r = 1;
		}
	}
	else
		r = 1;
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_checkcsposnodelist_ii)
{
	long   r = 0;
	const LongArray * p_list = (const LongArray *)params[1].ptrval;
	if(p_list) {
		if(params[0].lval) {
			PPObjCSession cs_obj;
			CSessionTbl::Rec cs_rec;
			if(cs_obj.Fetch(params[0].lval, &cs_rec) > 0 && p_list->lsearch(cs_rec.CashNodeID))
				r = 1;
		}
	}
	else
		r = 1;
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_strbystrgrouppos_ip)
{
	char   text_buf[256];
	if(option == CALC_SIZE)
		result->init((long)sizeof(text_buf));
	else {
		const SStrGroup * p_pool = (const SStrGroup *)params[1].ptrval;
		if(p_pool) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			p_pool->GetS(params[0].lval, r_temp_buf);
			STRNSCPY(text_buf, r_temp_buf);
		}
		else
			PTR32(text_buf)[0] = 0;
		result->init(text_buf);
	}
}

static IMPL_DBE_PROC(dbqf_checkuserid_ii)
{
	long   r = 0;
	if(params[1].lval) {
		if(params[1].lval & PPObjSecur::maskUserGroup) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur sec_rec;
			if(sec_obj.Fetch(params[0].lval, &sec_rec) > 0 && sec_rec.ParentID == (params[1].lval & ~PPObjSecur::maskUserGroup))
				r = 1;
		}
		else if(params[0].lval == params[1].lval)
			r = 1;
	}
	else
		r = 1;
	result->init(r);
}

static IMPL_DBE_PROC(dbqf_checkwmsloc_ii)
{
	long   r = 0;
	if(params[1].lval) {
		if(params[1].lval == params[0].lval)
			r = 1;
		else {
			PPObjLocation loc_obj;
			if(loc_obj.IsMemberOfGroup(params[0].lval, params[1].lval) > 0)
				r = 1;
		}
	}
	else
		r = 1;
	result->init(r);
}

// @vmiller
static IMPL_DBE_PROC(dbqf_strexistsub_ss)
{
	long   r = ExtStrSrch(params[0].sptr, params[1].sptr, 0);
	result->init(r);
}

// @vmiller
static IMPL_DBE_PROC(dbqf_getagrmntsymbol_i)
{
	const size_t result_size = 32;
	if(option == CALC_SIZE) {
		result->init((long)result_size);
	}
	else {
		SString edi_prvdr_symb;
		PPSupplAgreement suppl_agt;
		PPObjArticle::GetSupplAgreement(params[0].lval, &suppl_agt, 1);
		suppl_agt.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, edi_prvdr_symb);
		edi_prvdr_symb.Trim(result_size-1);
		result->init(edi_prvdr_symb);
	}
}

static IMPL_DBE_PROC(dbqf_objname_user_i)
{
	PPSecur rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id) {
			PPObjSecur obj(PPOBJ_USR, 0);
			obj.Fetch(id, &rec);
		}
		result->init(rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_globaluser_i)
{
	PPGlobalUserAcc rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id) {
			PPObjGlobalUserAcc obj(0);
			obj.Fetch(id, &rec);
		}
		result->init(rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_tech_i)
{
	TechTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Code));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Code);
		if(id) {
			PPObjTech obj;
			obj.Fetch(id, &rec);
		}
		result->init(rec.Code);
	}
}

static IMPL_DBE_PROC(dbqf_objname_goodsbytech_i)
{
	TechTbl::Rec rec;
	Goods2Tbl::Rec goods_rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(goods_rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, goods_rec.Name);
		if(id) {
			PPObjTech obj;
			if(obj.Fetch(id, &rec) > 0) {
				PPObjGoods goods_obj;
				goods_obj.Fetch(rec.GoodsID, &goods_rec);
			}
		}
		result->init(goods_rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_oprkind_i)
{
	PPOprKind rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id)
			GetOpData(id, &rec);
		result->init(rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_salcharge_i)
{
	PPSalChargePacket pack;
	if(option == CALC_SIZE)
		result->init((long)sizeof(pack.Rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, pack.Rec.Name);
		if(id) {
			PPObjSalCharge obj;
			obj.Fetch(id, &pack);
		}
		result->init(pack.Rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_bizscore_i)
{
	PPBizScorePacket pack;
	if(option == CALC_SIZE)
		result->init((long)sizeof(pack.Rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, pack.Rec.Name);
		if(id) {
			PPObjBizScore obj;
			obj.Fetch(id, &pack);
		}
		result->init(pack.Rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_arbyacc_i)
{
	if(option == CALC_SIZE || ObjRts.CheckAccID(params[1].lval, PPR_READ))
		dbqf_objname_i <PPObjArticle, ArticleTbl::Rec> (option, result, params);
	else {
		char zero_str[8];
		zero_str[0] = 0;
		result->init(zero_str);
	}
}

static IMPL_DBE_PROC(dbqf_objname_loc_i)
{
	//dbqf_objname_i <PPObjLocation, LocationTbl::Rec> (option, result, params);
	LocationTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id) {
			PPObjLocation obj;
			int    r = obj.Fetch(id, &rec);
			if(rec.Name[0] == 0) {
				if(r > 0) {
					if(rec.Code[0])
						STRNSCPY(rec.Name, rec.Code);
					else {
						SString & r_temp_buf = SLS.AcquireRvlStr();
						if(LocationCore::GetExField(&rec, LOCEXSTR_CONTACT, r_temp_buf) > 0 && r_temp_buf.NotEmptyS())
							STRNSCPY(rec.Name, r_temp_buf);
					}
				}
				if(rec.Name[0] == 0)
					ideqvalstr(id, rec.Name, sizeof(rec.Name));
			}
		}
		// @v9.9.4 result->init(rec.Name);
		result->InitForeignStr(rec.Name); // @v9.9.4
	}
}

static IMPL_DBE_PROC(dbqf_objname_billstatus_i)
	{ dbqf_objname_i <PPObjBillStatus, PPBillStatus> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_ar_i)
	{ dbqf_objname_i <PPObjArticle, ArticleTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_unit_i)
	{ dbqf_objname_i <PPObjUnit, PPUnit> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_prc_i)
	{ dbqf_objname_i <PPObjProcessor, ProcessorTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_goods_i)
	{ dbqf_objname_i <PPObjGoods, Goods2Tbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_person_i)
	{ dbqf_objname_i <PPObjPerson, PersonTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_staff_i)
	{ dbqf_objname_i <PPObjStaffList, PPStaffEntry> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_staffcal_i)
	{ dbqf_objname_i <PPObjStaffCal, PPStaffCal> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_accsheet_i)
	{ dbqf_objname_i <PPObjAccSheet, PPAccSheet> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_quotkind_i)
	{ dbqf_objname_i <PPObjQuotKind, PPQuotKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_cashnode_i)
	{ dbqf_objname_i <PPObjCashNode, PPCashNode> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_scale_i)
	{ dbqf_objname_i <PPObjScale, PPScale> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_psnopkind_i)
	{ dbqf_objname_i <PPObjPsnOpKind, PPPsnOpKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_brand_i)
	{ dbqf_objname_i <PPObjBrand, PPBrand> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objsymb_currency_i)
	{ dbqf_objsymb_i <PPObjCurrency, PPCurrency> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_world_i)
	{ dbqf_objname_i <PPObjWorld, WorldTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_personstatus_i)
	{ dbqf_objname_i <PPObjPersonStatus, PPPersonStatus> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_personcat_i)
	{ dbqf_objname_i <PPObjPersonCat, PPPersonCat> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_amttype_i)
	{ dbqf_objname_i <PPObjAmountType, PPAmountType> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_psnkind_i)
	{ dbqf_objname_i <PPObjPersonKind, PPPersonKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_scardser_i)
	{ dbqf_objname_i <PPObjSCardSeries, PPSCardSeries> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_debtdim_i)
	{ dbqf_objname_i <PPObjDebtDim, PPDebtDim> (option, result, params); }

static IMPL_DBE_PROC(dbqf_objcode_scard_i)
{
	SCardTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(rec.Code));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Code);
		if(id) {
			PPObjSCard sc_obj;
			sc_obj.Fetch(id, &rec);
			if(rec.Code[0] == 0)
				ideqvalstr(id, rec.Code, sizeof(rec.Code));
		}
		// @v9.9.4 result->init(rec.Code);
		result->InitForeignStr(rec.Code); // @v9.9.4
	}
}

static IMPL_DBE_PROC(dbqf_scardownername_i)
{
	char   psn_name[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(psn_name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, psn_name);
		if(id) {
			PPObjSCard sc_obj;
			SCardTbl::Rec rec;
			if(sc_obj.Fetch(id, &rec) > 0 && rec.PersonID) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				GetPersonName(rec.PersonID, r_temp_buf);
				r_temp_buf.CopyTo(psn_name, sizeof(psn_name));
			}
		}
		result->init(psn_name);
	}
}

static IMPL_DBE_PROC(dbqf_locownername_i)
{
	char   psn_name[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(psn_name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, psn_name);
		if(id) {
			PPObjLocation loc_obj;
			LocationTbl::Rec rec;
			if(loc_obj.Fetch(id, &rec) > 0 && rec.OwnerID) {
				SString & r_temp_buf = SLS.AcquireRvlStr();
				GetPersonName(rec.OwnerID, r_temp_buf);
				r_temp_buf.CopyTo(psn_name, sizeof(psn_name));
			}
		}
		result->init(psn_name);
	}
}

static IMPL_DBE_PROC(dbqf_usrpersonname_i)
{
	char   psn_name[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(psn_name));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, psn_name);
		if(id) {
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			PPSecur rec;
			if(sec_obj.Fetch(id, &rec) > 0 && rec.PersonID) {
				SString temp_buf;
				GetPersonName(rec.PersonID, temp_buf);
				temp_buf.CopyTo(psn_name, sizeof(psn_name));
			}
		}
		result->init(psn_name);
	}
}

static IMPL_DBE_PROC(dbqf_ufpfuncname_i)
{
	char   name[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name));
	else {
		long   id = PPDbqFuncPool::helper_dbq_name(params, name);
		if(id) {
			uint16 func_ver = 0;
			int    func_id = PPUserProfileFuncEntry::FromLoggedFuncId(id, &func_ver);
			SString temp_buf;
			PPLoadString(PPSTR_USRPROFILEFUNCNAM, func_id, temp_buf);
			temp_buf.CatChar('/').Cat(func_ver).CopyTo(name, sizeof(name));
		}
		result->init(name);
	}
}

static IMPL_DBE_PROC(dbqf_versionname_i)
{
	char   name[32];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name));
	else {
		long   id = PPDbqFuncPool::helper_dbq_name(params, name);
		if(id) {
			SVerT ver;
			ver.Set((uint32)id);
			SString temp_buf;
			ver.ToStr(temp_buf);
			temp_buf.CopyTo(name, sizeof(name));
		}
		result->init(name);
	}
}

static IMPL_DBE_PROC(dbqf_ufpfuncid_i)
{
	long func_id = 0L;
	char   name[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name));
	else {
		long   id = PPDbqFuncPool::helper_dbq_name(params, name);
		if(id) {
			uint16 func_ver = 0;
			func_id = PPUserProfileFuncEntry::FromLoggedFuncId(id, &func_ver);
		}
		result->init(func_id);
	}
}

static IMPL_DBE_PROC(dbqf_objname_personpost_i)
{
	char   name_buf[64];
	PersonPostTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			PPObjStaffList obj;
			obj.FetchPost(id, &rec);
			SString temp_buf;
			obj.MakeCodeString(&rec, temp_buf);
			temp_buf.CopyTo(name_buf, sizeof(name_buf));
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_stafforgname_i)
{
	char   name_buf[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		name_buf[0] = 0;
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			PPObjStaffList obj;
			PPStaffEntry se_rec;
			if(obj.Fetch(id, &se_rec) > 0 && se_rec.OrgID) {
				PPObjPerson psn_obj;
				PersonTbl::Rec psn_rec;
				if(psn_obj.Fetch(se_rec.OrgID, &psn_rec) > 0)
					STRNSCPY(name_buf, psn_rec.Name);
			}
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_staffdivname_i)
{
	char   name_buf[128];
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		name_buf[0] = 0;
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			PPObjStaffList obj;
			PPStaffEntry se_rec;
			if(obj.Fetch(id, &se_rec) > 0 && se_rec.DivisionID) {
				PPObjLocation loc_obj;
				LocationTbl::Rec loc_rec;
				if(loc_obj.Fetch(se_rec.DivisionID, &loc_rec) > 0)
					STRNSCPY(name_buf, loc_rec.Name);
			}
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objcodecmplx_bill_i)
{
	char   name_buf[48];
	BillTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			PPObjBill * p_bobj = BillObj;
			p_bobj->Fetch(id, &rec);
			SString temp_buf;
			p_bobj->MakeCodeString(&rec, 0, temp_buf);
			temp_buf.CopyTo(name_buf, sizeof(name_buf));
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objcode_bill_i)
{
	char   name_buf[24];
	BillTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			BillObj->Fetch(id, &rec);
			STRNSCPY(name_buf, rec.Code);
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objmemo_bill_i)
{
	char   name_buf[512];
	BillTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			BillObj->Fetch(id, &rec);
			STRNSCPY(name_buf, rec.Memo);
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objname_acctrel_i)
{
	char   name_buf[32];
	AcctRelTbl::Rec rec;
	if(option == CALC_SIZE)
		result->init((long)sizeof(name_buf));
	else {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			AcctRel * p_tbl = &BillObj->atobj->P_Tbl->AccRel;
			if(p_tbl->Fetch(id, &rec) > 0) {
				if(ObjRts.CheckAccID(rec.AccID, PPR_READ))
					((Acct *)&rec.Ac)->ToStr(ACCF_DEFAULT, name_buf);
				else
					STRNSCPY(name_buf, "ACCS DENIED");
			}
			else
				ideqvalstr(id, name_buf, sizeof(name_buf));
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_percent_rr) { result->init(fdivnz(100.0 * params[0].rval, params[1].rval)); }
static IMPL_DBE_PROC(dbqf_percentincdiv_rr) { result->init(fdivnz(100.0 * params[0].rval, params[1].rval+params[0].rval)); }
static IMPL_DBE_PROC(dbqf_percentaddendum_rr) { result->init(fdivnz(100.0 * (params[0].rval-params[1].rval), params[1].rval)); }
static IMPL_DBE_PROC(dbqf_lotclosedate_d) { result->init((params[0].dval != MAXDATE) ? params[0].dval : ZERODATE); }

static IMPL_DBE_PROC(dbqf_invent_diffqtty_i)
{
	long   flags     = params[0].lval;
	double diff_qtty = params[1].rval;
	result->init(INVENT_DIFFSIGN(flags) * diff_qtty);
}

static IMPL_DBE_PROC(dbqf_tacost_rr)
{
	// params[0] - qtty; params[1] - cost
	result->init(R2(fdivnz(params[1].rval, params[0].rval)));
}

static IMPL_DBE_PROC(dbqf_taprice_rrr)
{
	double qtty     = params[0].rval;
	double price    = params[1].rval;
	//double discount = params[2].rval;
	result->init(R2(fdivnz(price/*+discount*/, qtty)));
}

static IMPL_DBE_PROC(dbqf_addedcreditlimit_rii)
{
	double limit = params[0].rval;
	int    term = params[1].lval;
	int    added_term = params[2].lval;
	double _result = (term > 0 && added_term > 0 && limit != 0.0) ? R0((limit / term) * added_term) : 0.0;
	result->init(_result);
}

static IMPL_DBE_PROC(dbqf_idcommsyncid_ii)
{
	char   buf[20];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		PPCommSyncID comm_id;
		comm_id.P = (short)params[0].lval;
		comm_id.I = params[1].lval;
		SString temp_buf;
		comm_id.ToStr(0, temp_buf).CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_idobjtitle_i)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		DS.GetObjectTitle(params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_sysjaction_i)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPLoadString(PPSTR_ACTION, params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_gtajaction_i)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPLoadString(PPSTR_GTA, params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_yeswordbyflag_i)
{
	char buf[12];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		long flags = params[0].lval;
		long flag  = params[1].lval;
		char * p_out_symb = params[2].sptr;
		if(flags & flag) {
			SString temp_buf;
			temp_buf = p_out_symb;
			if(temp_buf.Len() == 0)
				PPLoadString("yes", temp_buf);
			temp_buf.CopyTo(buf, sizeof(buf));
		}
		else
			buf[0] = '\0';
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_budgplanorfact_rii)
{
	double amt     = params[0].rval;
	long   kind    = params[1].lval;
	long   is_fact = params[2].lval;
	if(kind != is_fact)
		amt = 0.0;
	result->init(amt);
}

static IMPL_DBE_PROC(dbqf_chkopjaction_i)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPGetSubStr(PPTXT_CHKOPACTIONLIST, params[0].lval - 1, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_addr_city_name_i)
{
	char   buf[48];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		buf[0] = 0;
		PPID   id = params[0].lval;
		if(id) {
			PPObjLocation loc_obj;
			SString city_name;
			loc_obj.GetCity(id, 0, &city_name, 1);
			city_name.CopyTo(buf, sizeof(buf));
		}
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_addr_ex_field_ii)
{
	char   buf[128];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		buf[0] = 0;
		PPID   id = params[0].lval;
		int    fld_id = params[1].lval;
		if(id) {
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			if(loc_obj.Fetch(id, &loc_rec) > 0) {
				SString temp_buf;
				LocationCore::GetExField(&loc_rec, fld_id, temp_buf);
				temp_buf.CopyTo(buf, sizeof(buf));
			}
		}
		result->init(buf);
	}
}

//static
int PPDbqFuncPool::IdEmpty             = 0;
int PPDbqFuncPool::IdBillDebt          = 0;
int PPDbqFuncPool::IdCQtty             = 0;
int PPDbqFuncPool::IdTrfrPrice         = 0;
int PPDbqFuncPool::IdObjNameBillStatus = 0;
int PPDbqFuncPool::IdObjNameOprKind    = 0;
int PPDbqFuncPool::IdObjNameLoc        = 0;
int PPDbqFuncPool::IdObjNameAr         = 0;
int PPDbqFuncPool::IdObjNameArByAcc    = 0; //
int PPDbqFuncPool::IdObjNameUser       = 0;
int PPDbqFuncPool::IdObjNameGlobalUser = 0; // @v7.3.8
int PPDbqFuncPool::IdObjNameUnit       = 0;
int PPDbqFuncPool::IdObjNameTech       = 0;
int PPDbqFuncPool::IdObjNameGoodsByTech = 0; //
int PPDbqFuncPool::IdObjNamePrc        = 0;
int PPDbqFuncPool::IdObjNameGoods      = 0;
int PPDbqFuncPool::IdObjNamePerson     = 0; //
int PPDbqFuncPool::IdObjNameSalCharge  = 0; //
int PPDbqFuncPool::IdObjNameStaff      = 0; //
int PPDbqFuncPool::IdObjNameStaffCal   = 0; //
int PPDbqFuncPool::IdObjNamePersonPost = 0; //
int PPDbqFuncPool::IdObjStaffOrg       = 0; // @v9.0.3
int PPDbqFuncPool::IdObjStaffDiv       = 0; // @v9.0.3
int PPDbqFuncPool::IdObjNameAccSheet   = 0; //
int PPDbqFuncPool::IdObjNameQuotKind   = 0; //
int PPDbqFuncPool::IdObjNameCashNode   = 0; //
int PPDbqFuncPool::IdObjNameScale      = 0; //
int PPDbqFuncPool::IdObjNamePsnOpKind  = 0; //
int PPDbqFuncPool::IdObjNameBizScore   = 0; //
int PPDbqFuncPool::IdObjNameAcctRel    = 0; //
int PPDbqFuncPool::IdObjNameBrand      = 0; //
int PPDbqFuncPool::IdObjNameWorld      = 0; //
int PPDbqFuncPool::IdObjNamePersonStatus = 0; //
int PPDbqFuncPool::IdObjNamePersonCat  = 0; //
int PPDbqFuncPool::IdObjNameAmountType = 0; //
int PPDbqFuncPool::IdObjNamePsnKind    = 0; //
int PPDbqFuncPool::IdObjSymbCurrency   = 0; //
int PPDbqFuncPool::IdObjCodeBillCmplx  = 0; // (fldBillID)
int PPDbqFuncPool::IdObjCodeBill       = 0; // (fldBillID)
int PPDbqFuncPool::IdObjMemoBill       = 0; // (fldBillID)
int PPDbqFuncPool::IdObjNameSCardSer   = 0; //
int PPDbqFuncPool::IdObjNameDebtDim    = 0; //
int PPDbqFuncPool::IdDateTime          = 0; // (fldDate, fldTime)
int PPDbqFuncPool::IdInventDiffQtty    = 0; //
int PPDbqFuncPool::IdTSesLnPhQtty      = 0; //
int PPDbqFuncPool::IdTSesLnFlags       = 0; //
int PPDbqFuncPool::IdPercent           = 0; // (100 * fld1 / fld2)
int PPDbqFuncPool::IdPercentIncDiv     = 0; // (100 * fld1 / (fld2+fld1))
int PPDbqFuncPool::IdPercentAddendum   = 0; // @v9.8.2 (100 * (fld1-fld2) / fld2)
int PPDbqFuncPool::IdWorldIsMemb       = 0; //
int PPDbqFuncPool::IdTaCost            = 0; //
int PPDbqFuncPool::IdTaPrice           = 0; //
int PPDbqFuncPool::IdCommSyncId        = 0; //
int PPDbqFuncPool::IdDurationToTime    = 0; //
int PPDbqFuncPool::IdObjTitle          = 0; //
int PPDbqFuncPool::IdGoodsStockDim     = 0; //
int PPDbqFuncPool::IdGoodsStockBrutto  = 0; //
int PPDbqFuncPool::IdGoodsStockMin     = 0; //
int PPDbqFuncPool::IdGoodsStockPackage = 0; //
int PPDbqFuncPool::IdGoodsSingleBarcode = 0; //
int PPDbqFuncPool::IdReportTypeName    = 0; //
int PPDbqFuncPool::IdLogFileName       = 0; //
int PPDbqFuncPool::IdSysJActionName    = 0; //
int PPDbqFuncPool::IdGtaJActionName    = 0; // @v7.3.8
int PPDbqFuncPool::IdCounter           = 0; //
int PPDbqFuncPool::IdPropSubStr        = 0; // (ObjType, ObjID, PropID, Sub)
int PPDbqFuncPool::IdCheckUserID       = 0; // (userID, filtUserID)
int PPDbqFuncPool::IdCheckWmsLocID     = 0; //
int PPDbqFuncPool::IdTransportTypeName = 0; //
int PPDbqFuncPool::IdLotCloseDate      = 0; //
int PPDbqFuncPool::IdFormatCycle       = 0; //
int PPDbqFuncPool::IdYesWordByFlag     = 0; //
int PPDbqFuncPool::IdBudgetPlanOrFact  = 0; //
int PPDbqFuncPool::IdChkOpJActionName  = 0; //
int PPDbqFuncPool::IdAddrCityName      = 0; // @v7.0.8  (locID)
int PPDbqFuncPool::IdAddrExField       = 0; // @v7.0.8  (locID, locExFld)
int PPDbqFuncPool::IdObjCodeSCard      = 0; // @v7.2.8  (fldSCardID)
int PPDbqFuncPool::IdSCardOwnerName    = 0; // @v7.2.8  (fldSCardID)
int PPDbqFuncPool::IdUsrPersonName     = 0; // @v7.5.11 (fldUsrID)
int PPDbqFuncPool::IdLocOwnerName      = 0; // @v9.1.5  (fldLocID) Формирует строку с именем персоналии-владельца локации
int PPDbqFuncPool::IdUfpFuncName       = 0; // @v8.1.1  (fldFuncId)
int PPDbqFuncPool::IdVersionText       = 0; // @v8.1.1  (fldVersion)
int PPDbqFuncPool::IdUfpFuncId         = 0; // @v8.1.x  (fldFuncId)
int PPDbqFuncPool::IdCheckCsPosNode    = 0; // @v7.6.3  (csessID, posNodeID)
int PPDbqFuncPool::IdCheckCsPosNodeList = 0; // @v7.6.3  (csessID, (const LongArray *))
int PPDbqFuncPool::IdStrExistSubStr    = 0; // @vmiller
int PPDbqFuncPool::IdAddedCreditLimit  = 0; // @v8.2.4
int PPDbqFuncPool::IdBillFrghtIssueDt  = 0; // @v8.2.9 (billID)
int PPDbqFuncPool::IdBillFrghtArrvlDt  = 0; // @v8.2.9 (billID)
int PPDbqFuncPool::IdBillFrghtDlvrAddr = 0; // @v8.7.9 (billID)
int PPDbqFuncPool::IdGetAgrmntSymbol   = 0; // @vmiller
int PPDbqFuncPool::IdBillAgentName     = 0; // @v8.3.6 (billID) Наименование агента по документу (извлекается из записи расширения документа)
int PPDbqFuncPool::IdRegisterText      = 0; // @v8.4.4 (registerID) Текст описания регистрационного документа
int PPDbqFuncPool::IdObjTagText        = 0; // @v8.4.11(tagid, objid) Текстовое представление тега объекта
int PPDbqFuncPool::IdDateRange         = 0; // @v8.6.4
//int PPDbqFuncPool::IdObjNameOpTypeK    = 0; // @v8.6.
int PPDbqFuncPool::IdOidText           = 0; // @v8.6.11 (objType, objID) Текстовое представление полного OID
int PPDbqFuncPool::IdDateBase          = 0; // @v8.6.11 (dateValue, baseDate) Текстовое представление даты, сжатой в виде количества дней, прошедших с baseDate
int PPDbqFuncPool::IdBillFrghtStrgLoc  = 0; // @v8.8.6
int PPDbqFuncPool::IdSCardExtString    = 0; // @v9.6.1 (scardID, fldId)
int PPDbqFuncPool::IdStrByStrGroupPos  = 0; // @v9.8.3 (position, (const SStrGroup *)) Возвращает строку из пула строк, идентифицируемую позицией position
int PPDbqFuncPool::IdBillDate          = 0; // @v10.0.03

static IMPL_DBE_PROC(dbqf_goodsstockdim_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPObjGoods goods_obj;
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(params[0].lval, &gse, 1) > 0 && gse.CalcVolume(1.0) > 0.0) { // @v6.2.3 useCache=1
			temp_buf.Cat(fdiv1000i(gse.PckgDim.Width),  SFMT_QTTY).CatChar('x');
			temp_buf.Cat(fdiv1000i(gse.PckgDim.Length), SFMT_QTTY).CatChar('x');
			temp_buf.Cat(fdiv1000i(gse.PckgDim.Height), SFMT_QTTY);
		}
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_goodsstockbrutto_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPObjGoods goods_obj;
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(params[0].lval, &gse, 1) > 0)
			temp_buf.Cat(fdiv1000i(gse.Brutto),  MKSFMTD(0, 3, NMBF_NOZERO));
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_goodsstockmin_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPObjGoods goods_obj;
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(params[0].lval, &gse, 1) > 0)
			temp_buf.Cat(gse.GetMinStock(0),  MKSFMTD(0, 3, NMBF_NOZERO | NMBF_NOTRAILZ));
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_goodsstockpack_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPObjGoods goods_obj;
		GoodsStockExt gse;
		if(goods_obj.GetStockExt(params[0].lval, &gse, 1) > 0)
			temp_buf.Cat(gse.Package,  MKSFMTD(0, 3, NMBF_NOZERO | NMBF_NOTRAILZ));
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_goodssinglebarcode_i)
{
	char   buf[24];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		PPObjGoods goods_obj;
		SString & r_temp_buf = SLS.AcquireRvlStr();
		goods_obj.FetchSingleBarcode(params[0].lval, r_temp_buf);
		r_temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_rpttypename_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		if(params[0].lval == ReportFilt::rpttStandart)
			PPLoadString("rptstd", temp_buf);
		else
			PPLoadString("rptlocal", temp_buf);
		temp_buf.ToOem().CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_logfilename_i)
{
	if(option == CALC_SIZE) {
		result->init((long)32);
	}
	else {
		SString logs_buff, log_descr_buff, buff;
		StringSet log_set(';', PPLoadTextS(PPTXT_LOGSNAMES, logs_buff));
		for(uint i=0; log_set.get(&i, log_descr_buff);) {
			uint j = 0;
			StringSet log_descr(',', log_descr_buff);
			log_descr.get(&j, buff);
			if(buff.ToLong() == params[0].lval) {
				log_descr.get(&j, (buff = 0));	// log name - skip
				log_descr.get(&j, (buff = 0));	// log file name -
				result->init(buff);				// - result
			}
		}
	}
}

static IMPL_DBE_PROC(dbqf_counter_i)
{
	long * ptr = (long *)params[0].lval;
	long   val = DEREFPTRORZ(ptr) + 1;
	result->init(val);
	ASSIGN_PTR(ptr, val);
}

//int PPDbqFuncPool::IdPropSubStr        = 0; // @v5.9.10 (ObjType, ObjID, PropID, Sub)

static IMPL_DBE_PROC(dbqf_propsubstr_iiii)
{
	if(option == CALC_SIZE) {
		result->init((long)252);
	}
	else {
		long   obj_type = params[0].lval;
		long   obj_id   = params[1].lval;
		long   prop     = params[2].lval;
		long   sub      = params[3].lval;
		SString strg_buf, str;
		if(PPRef->GetPropVlrString(obj_type, obj_id, prop, strg_buf))
			PPGetExtStrData(sub, strg_buf, str);
		result->init(str);
	}
}

static IMPL_DBE_PROC(dbqf_transptypename_i)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		if(params[0].lval == PPTRTYP_CAR)
			PPLoadString("car", temp_buf);
		else
			PPLoadString("ship", temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_formatcycle_di)
{
	char   buf[64];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		LDATE dt = params[0].dval;
		const PPCycleArray * p_list = (const PPCycleArray *)params[1].lval;
		if(p_list && p_list->getCount()) {
			p_list->formatCycle(dt, buf, sizeof(buf));
		}
		else
			datefmt(&dt, DATF_DMY, buf);
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_daterange_dd)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		DateRange period;
		period.low = params[0].dval;
		period.upp = params[1].dval;
		periodfmtex(&period, buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_datebase_id)
{
	int    sd = params[0].lval;
	LDATE  base = params[1].dval;
	LDATE  _d = (sd <= 0 || !checkdate(base, 1)) ? ZERODATE : plusdate(base, sd);
	result->init(_d);
}

// static
int SLAPI PPDbqFuncPool::Register()
{
	int    ok = 1;
	THROW(DbqFuncTab::RegisterDyn(&IdEmpty,               BTS_STRING, dbqf_empty, 0));
	THROW(DbqFuncTab::RegisterDyn(&IdBillDebt,            BTS_REAL,   dbqf_debt_rrii,  4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtIssueDt,    BTS_DATE,   dbqf_billfrghtissuedt_i, 1, BTS_INT)); // @v8.2.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtArrvlDt,    BTS_DATE,   dbqf_billfrghtarrvldt_i, 1, BTS_INT)); // @v8.2.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtDlvrAddr,   BTS_STRING, dbqf_billfrghtdlvraddr_i, 1, BTS_INT)); // @v8.7.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtStrgLoc,    BTS_STRING, dbqf_billfrghtstrgloc_i, 1, BTS_INT)); // @v8.8.6
	THROW(DbqFuncTab::RegisterDyn(&IdBillAgentName,       BTS_STRING, dbqf_billagentname_i,  1, BTS_INT)); // @v8.3.6
	THROW(DbqFuncTab::RegisterDyn(&IdBillDate,            BTS_DATE,   dbqf_billdate_i, 1, BTS_INT)); // @v10.0.03
	THROW(DbqFuncTab::RegisterDyn(&IdCQtty,               BTS_STRING, dbqf_cqtty_rrii, 4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBillStatus,   BTS_STRING, dbqf_objname_billstatus_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameOprKind,      BTS_STRING, dbqf_objname_oprkind_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameLoc,          BTS_STRING, dbqf_objname_loc_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAr,           BTS_STRING, dbqf_objname_ar_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameArByAcc,      BTS_STRING, dbqf_objname_arbyacc_i,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUser,         BTS_STRING, dbqf_objname_user_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGlobalUser,   BTS_STRING, dbqf_objname_globaluser_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUnit,         BTS_STRING, dbqf_objname_unit_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameTech,         BTS_STRING, dbqf_objname_tech_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoodsByTech,  BTS_STRING, dbqf_objname_goodsbytech_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePrc,          BTS_STRING, dbqf_objname_prc_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoods,        BTS_STRING, dbqf_objname_goods_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePerson,       BTS_STRING, dbqf_objname_person_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSalCharge,    BTS_STRING, dbqf_objname_salcharge_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaff,        BTS_STRING, dbqf_objname_staff_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaffCal,     BTS_STRING, dbqf_objname_staffcal_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonPost,   BTS_STRING, dbqf_objname_personpost_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffOrg,         BTS_STRING, dbqf_stafforgname_i,        1, BTS_INT)); // @v9.0.3
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffDiv,         BTS_STRING, dbqf_staffdivname_i,        1, BTS_INT)); // @v9.0.3
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAccSheet,     BTS_STRING, dbqf_objname_accsheet_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameQuotKind,     BTS_STRING, dbqf_objname_quotkind_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameCashNode,     BTS_STRING, dbqf_objname_cashnode_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameScale,        BTS_STRING, dbqf_objname_scale_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnOpKind,    BTS_STRING, dbqf_objname_psnopkind_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBizScore,     BTS_STRING, dbqf_objname_bizscore_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAcctRel,      BTS_STRING, dbqf_objname_acctrel_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBrand,        BTS_STRING, dbqf_objname_brand_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameWorld,        BTS_STRING, dbqf_objname_world_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonStatus, BTS_STRING, dbqf_objname_personstatus_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonCat,    BTS_STRING, dbqf_objname_personcat_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAmountType,   BTS_STRING, dbqf_objname_amttype_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnKind,      BTS_STRING, dbqf_objname_psnkind_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjSymbCurrency,     BTS_STRING, dbqf_objsymb_currency_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBillCmplx,    BTS_STRING, dbqf_objcodecmplx_bill_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBill,         BTS_STRING, dbqf_objcode_bill_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjMemoBill,         BTS_STRING, dbqf_objmemo_bill_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSCardSer,     BTS_STRING, dbqf_objname_scardser_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameDebtDim,      BTS_STRING, dbqf_objname_debtdim_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeSCard,        BTS_STRING, dbqf_objcode_scard_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSCardOwnerName,      BTS_STRING, dbqf_scardownername_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLocOwnerName,        BTS_STRING, dbqf_locownername_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUsrPersonName,       BTS_STRING, dbqf_usrpersonname_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncName,         BTS_STRING, dbqf_ufpfuncname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdVersionText,         BTS_STRING, dbqf_versionname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncId,           BTS_INT,    dbqf_ufpfuncid_i,           1, BTS_INT));

	THROW(DbqFuncTab::RegisterDyn(&IdTrfrPrice,           BTS_REAL,   dbqf_trfrprice_irrr, 7, BTS_INT, BTS_INT, BTS_DATE, BTS_INT, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDateTime,            BTS_STRING, dbqf_datetime_dt, 2, BTS_DATE, BTS_TIME));
	THROW(DbqFuncTab::RegisterDyn(&IdInventDiffQtty,      BTS_REAL,   dbqf_invent_diffqtty_i,  2, BTS_INT, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnPhQtty,        BTS_REAL,   dbqf_tseslnphqtty_iirr,  4, BTS_INT, BTS_INT, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnFlags,         BTS_STRING, dbqf_tseslnflags_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPercent,             BTS_REAL,   dbqf_percent_rr,         2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdPercentIncDiv,       BTS_REAL,   dbqf_percentincdiv_rr,   2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdPercentAddendum,     BTS_REAL,   dbqf_percentaddendum_rr, 2, BTS_REAL, BTS_REAL)); // @v9.8.2
	THROW(DbqFuncTab::RegisterDyn(&IdWorldIsMemb,         BTS_INT,    dbqf_world_ismemb_ii,    2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTaCost,              BTS_REAL,   dbqf_tacost_rr,          2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTaPrice,             BTS_REAL,   dbqf_taprice_rrr,        3, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDurationToTime,      BTS_STRING, dbqf_durationtotime_dt,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCommSyncId,          BTS_STRING, dbqf_idcommsyncid_ii,    2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjTitle,            BTS_STRING, dbqf_idobjtitle_i,       1, BTS_INT));

	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockDim,       BTS_STRING, dbqf_goodsstockdim_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockBrutto,    BTS_STRING, dbqf_goodsstockbrutto_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockMin,       BTS_STRING, dbqf_goodsstockmin_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockPackage,   BTS_STRING, dbqf_goodsstockpack_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsSingleBarcode,  BTS_STRING, dbqf_goodssinglebarcode_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdReportTypeName,      BTS_STRING, dbqf_rpttypename_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLogFileName,         BTS_STRING, dbqf_logfilename_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSysJActionName,      BTS_STRING, dbqf_sysjaction_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGtaJActionName,      BTS_STRING, dbqf_gtajaction_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCounter,             BTS_INT,    dbqf_counter_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPropSubStr,          BTS_STRING, dbqf_propsubstr_iiii,    4, BTS_INT, BTS_INT, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckUserID,         BTS_INT,    dbqf_checkuserid_ii,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckWmsLocID,       BTS_INT,    dbqf_checkwmsloc_ii,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTransportTypeName,   BTS_STRING, dbqf_transptypename_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLotCloseDate,        BTS_DATE,   dbqf_lotclosedate_d,     1, BTS_DATE));
	THROW(DbqFuncTab::RegisterDyn(&IdFormatCycle,         BTS_STRING, dbqf_formatcycle_di,     2, BTS_DATE, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdYesWordByFlag,       BTS_STRING, dbqf_yeswordbyflag_i,    3, BTS_INT, BTS_INT, BTS_STRING));
	THROW(DbqFuncTab::RegisterDyn(&IdBudgetPlanOrFact,    BTS_REAL,   dbqf_budgplanorfact_rii, 3, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdChkOpJActionName,    BTS_STRING, dbqf_chkopjaction_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdAddrCityName,        BTS_STRING, dbqf_addr_city_name_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdAddrExField,         BTS_STRING, dbqf_addr_ex_field_ii,   2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNode,      BTS_INT,    dbqf_checkcsposnode_ii,  2, BTS_INT, BTS_INT)); // (csessID, posNodeID)
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNodeList,  BTS_INT,    dbqf_checkcsposnodelist_ii, 2, BTS_INT, BTS_PTR)); // (csessID, (const LongArray *))
	THROW(DbqFuncTab::RegisterDyn(&IdStrByStrGroupPos,    BTS_STRING, dbqf_strbystrgrouppos_ip,   2, BTS_INT, BTS_PTR)); // @v9.8.3 (position, (const SStrGroup *))
	THROW(DbqFuncTab::RegisterDyn(&IdStrExistSubStr,      BTS_INT,    dbqf_strexistsub_ss,        2, BTS_STRING, BTS_STRING));
	THROW(DbqFuncTab::RegisterDyn(&IdAddedCreditLimit,    BTS_REAL,   dbqf_addedcreditlimit_rii,  3, BTS_REAL, BTS_INT, BTS_INT)); // @v8.2.4
	THROW(DbqFuncTab::RegisterDyn(&IdGetAgrmntSymbol,     BTS_STRING, dbqf_getagrmntsymbol_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdRegisterText,        BTS_STRING, dbqf_registertext_i,     1, BTS_INT)); // @v8.4.4
	THROW(DbqFuncTab::RegisterDyn(&IdObjTagText,          BTS_STRING, dbqf_objtagtext_ii,      2, BTS_INT, BTS_INT)); // @v8.4.11
	THROW(DbqFuncTab::RegisterDyn(&IdDateRange,           BTS_STRING, dbqf_daterange_dd,       2, BTS_DATE, BTS_DATE)); // @v8.6.4
	THROW(DbqFuncTab::RegisterDyn(&IdOidText,             BTS_STRING, dbqf_oidtext_ii,         2, BTS_INT, BTS_INT)); // @v8.6.11
	THROW(DbqFuncTab::RegisterDyn(&IdDateBase,            BTS_DATE,   dbqf_datebase_id,        2, BTS_INT, BTS_DATE)); // @v8.6.11
	THROW(DbqFuncTab::RegisterDyn(&IdSCardExtString,      BTS_STRING, dbqf_scardextstring_ii,  2, BTS_INT, BTS_INT)); // @v9.6.1
	CATCHZOK
	return ok;
}

// static
void FASTCALL PPDbqFuncPool::InitObjNameFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push((DBFunc)funcId);
}

// static
void FASTCALL PPDbqFuncPool::InitObjTagTextFunc(DBE & rDbe, PPID tagID, DBField & rFld)
{
	rDbe.init();
	rDbe.push(dbconst(tagID));
	rDbe.push(rFld);
	rDbe.push((DBFunc)PPDbqFuncPool::IdObjTagText);
}

//static
void FASTCALL PPDbqFuncPool::InitLongFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push((DBFunc)funcId);
}

//static
void FASTCALL PPDbqFuncPool::InitFunc2Arg(DBE & rDbe, int funcId, DBItem & rA1, DBItem & rA2)
{
	rDbe.init();
	rDbe.push(rA1);
	rDbe.push(rA2);
	rDbe.push((DBFunc)funcId);
}

//static
void FASTCALL PPDbqFuncPool::InitPctFunc(DBE & rDbe, DBField & rFld1, DBField & rFld2, int incDiv)
{
	rDbe.init();
	rDbe.push(rFld1);
	rDbe.push(rFld2);
	if(incDiv == 2)
		rDbe.push((DBFunc)IdPercentAddendum);
	else if(incDiv == 1)
		rDbe.push((DBFunc)PPDbqFuncPool::IdPercentIncDiv);
	else
		rDbe.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
}

//static
void FASTCALL PPDbqFuncPool::InitStrPoolRefFunc(DBE & rDbe, DBField & rFld, SStrGroup * pSg)
{
	rDbe.init();
	rDbe.push(rFld);
	{
		DBConst dbc_ptr;
		dbc_ptr.init(pSg);
		rDbe.push(dbc_ptr);
	}
	rDbe.push((DBFunc)PPDbqFuncPool::IdStrByStrGroupPos);
}

//static
PPID FASTCALL PPDbqFuncPool::helper_dbq_name(const DBConst * params, char * pNameBuf)
{
	PPID   id = 0;
	if(params[0].Tag == DBConst::lv)
		id = params[0].lval;
	else if(params[0].Tag == DBConst::rv)
		id = (long)params[0].rval;
	pNameBuf[0] = 0;
	return id;
}
