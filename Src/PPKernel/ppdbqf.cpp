// PPDBQF.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016
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

template <class objcls, class objrec> inline void dbqf_objname_i(int option, DBConst * result, DBConst * params)
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
		result->init(rec.Name);
	}
}

static void SLAPI dbqf_oidtext_ii(int option, DBConst * result, DBConst * params)
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

template <class objcls, class objrec> inline void dbqf_objsymb_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_empty(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_tseslnflags_i(int option, DBConst * result, DBConst * params)
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
static void SLAPI dbqf_tseslnphqtty_iirr(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_debt_rrii(int option, DBConst * result, DBConst * params)
{
	long   f = params[2].lval;
	long   op_id = params[3].lval;
	double r = ((f & BILLF_NEEDPAYMENT) || CheckOpFlags(op_id, OPKF_RECKON)) ? (params[0].rval - params[1].rval) : 0;
	result->init(r);
}

static void SLAPI dbqf_billfrghtissuedt_i(int option, DBConst * result, DBConst * params)
{
	PPFreight freight;
	result->init((BillObj->FetchFreight(params[0].lval, &freight) > 0) ? freight.IssueDate : ZERODATE);
}

static void SLAPI dbqf_billfrghtarrvldt_i(int option, DBConst * result, DBConst * params)
{
	PPFreight freight;
	result->init((BillObj->FetchFreight(params[0].lval, &freight) > 0) ? freight.ArrivalDate : ZERODATE);
}

static void SLAPI dbqf_billfrghtstrgloc_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_billfrghtdlvraddr_i(int option, DBConst * result, DBConst * params)
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
							SString tt;
							(tt = rec.Name).CatDiv('-', 1).Cat(temp_buf);
							temp_buf = tt;
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

static void SLAPI dbqf_billagentname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_registertext_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objtagtext_ii(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_cqtty_rrii(int option, DBConst * result, DBConst * params)
{
	if(option == CALC_SIZE)
		result->init((long)24);
	else {
		double rest = 0.0;
		if(params[0].tag == DBConst::lv)
			rest = params[0].lval;
		else if(params[0].tag == DBConst::rv)
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

static void SLAPI dbqf_trfrprice_irrr(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_datetime_dt(int option, DBConst * result, DBConst * params)
{
	const size_t field_len = 20;
	if(option == CALC_SIZE)
		result->init((long)field_len);
	else {
		LDATETIME dtm;
		dtm.Set(params[0].dval, params[1].tval);
		SString temp_buf;
		result->init(temp_buf.Cat(dtm).Trim(field_len));
	}
}

static void SLAPI dbqf_durationtotime_dt(int option, DBConst * result, DBConst * params)
{
	const size_t field_len = 20;
	if(option == CALC_SIZE)
		result->init((long)field_len);
	else {
		LDATETIME dtm;
		dtm.SetZero();
		long days = dtm.settotalsec(params[0].lval);
		SString temp_buf;
		if(days)
			temp_buf.Cat(days).CatChar('d').Space();
		result->init(temp_buf.Cat(dtm.t, TIMF_HMS).Trim(field_len-1));
	}
}

static void SLAPI dbqf_world_ismemb_ii(int option, DBConst * result, DBConst * params)
{
	long   r = 0;
	if(params[0].lval && params[1].lval) {
		PPObjWorld w_obj;
		r = w_obj.IsChildOf(params[0].lval, params[1].lval) ? 1 : 0;
	}
	result->init(r);
}

static void SLAPI dbqf_checkcsposnode_ii(int option, DBConst * result, DBConst * params) // (csessID, posNodeID)
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

static void SLAPI dbqf_checkcsposnodelist_ii(int option, DBConst * result, DBConst * params)
{
	long   r = 0;
	const LongArray * p_list = (LongArray *)params[1].ptrval;
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

static void SLAPI dbqf_checkuserid_ii(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_checkwmsloc_ii(int option, DBConst * result, DBConst * params)
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
static void SLAPI dbqf_strexistsub_ss(int option, DBConst * result, DBConst * params)
{
	long   r = 0;
	r = ExtStrSrch(params[0].sptr, params[1].sptr);
	result->init(r);
}

// @vmiller
static void SLAPI dbqf_getagrmntsymbol_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_user_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_globaluser_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_tech_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_goodsbytech_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_oprkind_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_salcharge_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_bizscore_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_billstatus_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjBillStatus, PPBillStatus> (option, result, params); }
static void SLAPI dbqf_objname_ar_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjArticle, ArticleTbl::Rec> (option, result, params); }

static void SLAPI dbqf_objname_arbyacc_i(int option, DBConst * result, DBConst * params)
{
	if(option == CALC_SIZE || ObjRts.CheckAccID(params[1].lval, PPR_READ))
		dbqf_objname_i <PPObjArticle, ArticleTbl::Rec> (option, result, params);
	else {
		char zero_str[8];
		zero_str[0] = 0;
		result->init(zero_str);
	}
}

static void SLAPI dbqf_objname_loc_i(int option, DBConst * result, DBConst * params)
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
						SString temp_buf;
						if(LocationCore::GetExField(&rec, LOCEXSTR_CONTACT, temp_buf) > 0 && temp_buf.NotEmptyS())
							STRNSCPY(rec.Name, temp_buf);
					}
				}
				if(rec.Name[0] == 0)
					ideqvalstr(id, rec.Name, sizeof(rec.Name));
			}
		}
		result->init(rec.Name);
	}
}

static void SLAPI dbqf_objname_unit_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjUnit, PPUnit> (option, result, params); }
static void SLAPI dbqf_objname_prc_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjProcessor, ProcessorTbl::Rec> (option, result, params); }
static void SLAPI dbqf_objname_goods_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjGoods, Goods2Tbl::Rec> (option, result, params); }
static void SLAPI dbqf_objname_person_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjPerson, PersonTbl::Rec> (option, result, params); }
static void SLAPI dbqf_objname_staff_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjStaffList, PPStaffEntry> (option, result, params); }
static void SLAPI dbqf_objname_staffcal_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjStaffCal, PPStaffCal> (option, result, params); }
static void SLAPI dbqf_objname_accsheet_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjAccSheet, PPAccSheet> (option, result, params); }
static void SLAPI dbqf_objname_quotkind_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjQuotKind, PPQuotKind> (option, result, params); }
static void SLAPI dbqf_objname_cashnode_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjCashNode, PPCashNode> (option, result, params); }
static void SLAPI dbqf_objname_scale_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjScale, PPScale> (option, result, params); }
static void SLAPI dbqf_objname_psnopkind_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjPsnOpKind, PPPsnOpKind> (option, result, params); }
static void SLAPI dbqf_objname_brand_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjBrand, PPBrand> (option, result, params); }
static void SLAPI dbqf_objsymb_currency_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objsymb_i <PPObjCurrency, PPCurrency> (option, result, params); }
static void SLAPI dbqf_objname_world_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjWorld, WorldTbl::Rec> (option, result, params); }
static void SLAPI dbqf_objname_personstatus_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjPersonStatus, PPPersonStatus> (option, result, params); }
static void SLAPI dbqf_objname_personcat_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjPersonCat, PPPersonCat> (option, result, params); }
static void SLAPI dbqf_objname_amttype_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjAmountType, PPAmountType> (option, result, params); }
static void SLAPI dbqf_objname_psnkind_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjPersonKind, PPPersonKind> (option, result, params); }
static void SLAPI dbqf_objname_scardser_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjSCardSeries, PPSCardSeries> (option, result, params); }

static void SLAPI dbqf_objname_debtdim_i(int option, DBConst * result, DBConst * params)
	{ dbqf_objname_i <PPObjDebtDim, PPDebtDim> (option, result, params); }


static void SLAPI dbqf_objcode_scard_i(int option, DBConst * result, DBConst * params)
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
		result->init(rec.Code);
	}
}

static void SLAPI dbqf_scardownername_i(int option, DBConst * result, DBConst * params)
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
				SString temp_buf;
				GetPersonName(rec.PersonID, temp_buf);
				temp_buf.CopyTo(psn_name, sizeof(psn_name));
			}
		}
		result->init(psn_name);
	}
}

static void SLAPI dbqf_locownername_i(int option, DBConst * result, DBConst * params)
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
				SString temp_buf;
				GetPersonName(rec.OwnerID, temp_buf);
				temp_buf.CopyTo(psn_name, sizeof(psn_name));
			}
		}
		result->init(psn_name);
	}
}

static void SLAPI dbqf_usrpersonname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_ufpfuncname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_versionname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_ufpfuncid_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_personpost_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_stafforgname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_staffdivname_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objcodecmplx_bill_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objcode_bill_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objmemo_bill_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_objname_acctrel_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_percent_rr(int options, DBConst * result, DBConst * params)
{
	result->init(fdivnz(100 * params[0].rval, params[1].rval));
}

static void SLAPI dbqf_percentincdiv_rr(int options, DBConst * result, DBConst * params)
{
	result->init(fdivnz(100 * params[0].rval, params[1].rval+params[0].rval));
}

static void SLAPI dbqf_invent_diffqtty_i(int option, DBConst * result, DBConst * params)
{
	long   flags     = params[0].lval;
	double diff_qtty = params[1].rval;
	result->init(INVENT_DIFFSIGN(flags) * diff_qtty);
}

static void SLAPI dbqf_tacost_rr(int option, DBConst * result, DBConst * params)
{
	// params[0] - qtty; params[1] - cost
	result->init(R2(fdivnz(params[1].rval, params[0].rval)));
}

static void SLAPI dbqf_taprice_rrr(int option, DBConst * result, DBConst * params)
{
	double qtty     = params[0].rval;
	double price    = params[1].rval;
	//double discount = params[2].rval;
	result->init(R2(fdivnz(price/*+discount*/, qtty)));
}

static void SLAPI dbqf_addedcreditlimit_rii(int option, DBConst * result, DBConst * params)
{
	double limit = params[0].rval;
	int    term = params[1].lval;
	int    added_term = params[2].lval;
	double _result = (term > 0 && added_term > 0 && limit != 0.0) ? R0((limit / term) * added_term) : 0.0;
	result->init(_result);
}

static void SLAPI dbqf_idcommsyncid_ii(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_idobjtitle_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_sysjaction_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_gtajaction_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_yeswordbyflag_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_budgplanorfact_rii(int option, DBConst * result, DBConst * params)
{
	double amt     = params[0].rval;
	long   kind    = params[1].lval;
	long   is_fact = params[2].lval;
	if(kind != is_fact)
		amt = 0.0;
	result->init(amt);
}

static void SLAPI dbqf_chkopjaction_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_addr_city_name_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_addr_ex_field_ii(int option, DBConst * result, DBConst * params)
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
int PPDbqFuncPool::IdObjCodeBill       = 0; // @v7.2.8  (fldBillID)
int PPDbqFuncPool::IdObjMemoBill       = 0; // @v7.2.8  (fldBillID)
int PPDbqFuncPool::IdObjNameSCardSer   = 0; //
int PPDbqFuncPool::IdObjNameDebtDim    = 0; //
int PPDbqFuncPool::IdDateTime          = 0; // (fldDate, fldTime)
int PPDbqFuncPool::IdInventDiffQtty    = 0; //
int PPDbqFuncPool::IdTSesLnPhQtty      = 0; //
int PPDbqFuncPool::IdTSesLnFlags       = 0; //
int PPDbqFuncPool::IdPercent           = 0; // (100 * div / divisor)
int PPDbqFuncPool::IdPercentIncDiv     = 0; // (100 * div / (divisor+div))
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

static void SLAPI dbqf_goodsstockdim_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_lotclosedate_d(int option, DBConst * result, DBConst * params)
{
	result->init((params[0].dval != MAXDATE) ? params[0].dval : ZERODATE);
}

static void SLAPI dbqf_goodsstockbrutto_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_goodsstockmin_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_goodsstockpack_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_goodssinglebarcode_i(int option, DBConst * result, DBConst * params)
{
	char   buf[24];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		PPObjGoods goods_obj;
		goods_obj.FetchSingleBarcode(params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static void SLAPI dbqf_rpttypename_i(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_logfilename_i(int option, DBConst *result, DBConst *params)
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

static void SLAPI dbqf_counter_i(int option, DBConst *result, DBConst *params)
{
	long * ptr = (long *)params[0].lval;
	long   val = (ptr ? *ptr : 0) + 1;
	result->init(val);
	ASSIGN_PTR(ptr, val);
}

//int PPDbqFuncPool::IdPropSubStr        = 0; // @v5.9.10 (ObjType, ObjID, PropID, Sub)

static void SLAPI dbqf_propsubstr_iiii(int option, DBConst *result, DBConst *params)
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

static void SLAPI dbqf_transptypename_i(int option, DBConst * result, DBConst * params)
{
	char   buf[32];
	if(option == CALC_SIZE) {
		result->init((long)sizeof(buf));
	}
	else {
		SString temp_buf;
		if(params[0].lval == PPTRTYP_CAR)
			SLS.LoadString("car", temp_buf);
		else
			SLS.LoadString("ship", temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static void SLAPI dbqf_formatcycle_di(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_daterange_dd(int option, DBConst * result, DBConst * params)
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

static void SLAPI dbqf_datebase_id(int option, DBConst * result, DBConst * params)
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
	THROW(DbqFuncTab::RegisterDyn(&IdEmpty,    0, BTS_STRING, dbqf_empty, 0));
	THROW(DbqFuncTab::RegisterDyn(&IdBillDebt, 0, BTS_REAL,   dbqf_debt_rrii,  4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtIssueDt,  0, BTS_DATE, dbqf_billfrghtissuedt_i, 1, BTS_INT)); // @v8.2.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtArrvlDt,  0, BTS_DATE, dbqf_billfrghtarrvldt_i, 1, BTS_INT)); // @v8.2.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtDlvrAddr, 0, BTS_STRING, dbqf_billfrghtdlvraddr_i, 1, BTS_INT)); // @v8.7.9
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtStrgLoc,  0, BTS_STRING, dbqf_billfrghtstrgloc_i, 1, BTS_INT)); // @v8.8.6
	THROW(DbqFuncTab::RegisterDyn(&IdBillAgentName,     0, BTS_STRING, dbqf_billagentname_i,  1, BTS_INT)); // @v8.3.6
	THROW(DbqFuncTab::RegisterDyn(&IdCQtty,    0, BTS_STRING, dbqf_cqtty_rrii, 4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBillStatus,  0, BTS_STRING, dbqf_objname_billstatus_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameOprKind,     0, BTS_STRING, dbqf_objname_oprkind_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameLoc,         0, BTS_STRING, dbqf_objname_loc_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAr,          0, BTS_STRING, dbqf_objname_ar_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameArByAcc,     0, BTS_STRING, dbqf_objname_arbyacc_i,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUser,        0, BTS_STRING, dbqf_objname_user_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGlobalUser,  0, BTS_STRING, dbqf_objname_globaluser_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUnit,        0, BTS_STRING, dbqf_objname_unit_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameTech,        0, BTS_STRING, dbqf_objname_tech_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoodsByTech, 0, BTS_STRING, dbqf_objname_goodsbytech_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePrc,         0, BTS_STRING, dbqf_objname_prc_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoods,       0, BTS_STRING, dbqf_objname_goods_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePerson,      0, BTS_STRING, dbqf_objname_person_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSalCharge,   0, BTS_STRING, dbqf_objname_salcharge_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaff,       0, BTS_STRING, dbqf_objname_staff_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaffCal,    0, BTS_STRING, dbqf_objname_staffcal_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonPost,  0, BTS_STRING, dbqf_objname_personpost_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffOrg,        0, BTS_STRING, dbqf_stafforgname_i,        1, BTS_INT)); // @v9.0.3
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffDiv,        0, BTS_STRING, dbqf_staffdivname_i,        1, BTS_INT)); // @v9.0.3
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAccSheet,    0, BTS_STRING, dbqf_objname_accsheet_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameQuotKind,    0, BTS_STRING, dbqf_objname_quotkind_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameCashNode,    0, BTS_STRING, dbqf_objname_cashnode_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameScale,       0, BTS_STRING, dbqf_objname_scale_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnOpKind,   0, BTS_STRING, dbqf_objname_psnopkind_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBizScore,    0, BTS_STRING, dbqf_objname_bizscore_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAcctRel,     0, BTS_STRING, dbqf_objname_acctrel_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBrand,       0, BTS_STRING, dbqf_objname_brand_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameWorld,       0, BTS_STRING, dbqf_objname_world_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonStatus, 0, BTS_STRING, dbqf_objname_personstatus_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonCat,   0, BTS_STRING, dbqf_objname_personcat_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAmountType,  0, BTS_STRING, dbqf_objname_amttype_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnKind,     0, BTS_STRING, dbqf_objname_psnkind_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjSymbCurrency,    0, BTS_STRING, dbqf_objsymb_currency_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBillCmplx,   0, BTS_STRING, dbqf_objcodecmplx_bill_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBill,        0, BTS_STRING, dbqf_objcode_bill_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjMemoBill,        0, BTS_STRING, dbqf_objmemo_bill_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSCardSer,    0, BTS_STRING, dbqf_objname_scardser_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameDebtDim,     0, BTS_STRING, dbqf_objname_debtdim_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeSCard,       0, BTS_STRING, dbqf_objcode_scard_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSCardOwnerName,     0, BTS_STRING, dbqf_scardownername_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLocOwnerName,       0, BTS_STRING, dbqf_locownername_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUsrPersonName,      0, BTS_STRING, dbqf_usrpersonname_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncName,        0, BTS_STRING, dbqf_ufpfuncname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdVersionText,        0, BTS_STRING, dbqf_versionname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncId,          0, BTS_INT,    dbqf_ufpfuncid_i,           1, BTS_INT));

	THROW(DbqFuncTab::RegisterDyn(&IdTrfrPrice, 0, BTS_REAL,   dbqf_trfrprice_irrr,
		7, BTS_INT, BTS_INT, BTS_DATE, BTS_INT, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDateTime,  0, BTS_STRING, dbqf_datetime_dt, 2, BTS_DATE, BTS_TIME));
	THROW(DbqFuncTab::RegisterDyn(&IdInventDiffQtty,    0, BTS_REAL,   dbqf_invent_diffqtty_i, 2, BTS_INT, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnPhQtty,      0, BTS_REAL,   dbqf_tseslnphqtty_iirr, 4, BTS_INT, BTS_INT, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnFlags,       0, BTS_STRING, dbqf_tseslnflags_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPercent,           0, BTS_REAL,   dbqf_percent_rr,        2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdPercentIncDiv,     0, BTS_REAL,   dbqf_percentincdiv_rr,  2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdWorldIsMemb,       0, BTS_INT,    dbqf_world_ismemb_ii,   2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTaCost,            0, BTS_REAL,   dbqf_tacost_rr,         2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTaPrice,           0, BTS_REAL,   dbqf_taprice_rrr,       3, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDurationToTime,    0, BTS_STRING, dbqf_durationtotime_dt, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCommSyncId,        0, BTS_STRING, dbqf_idcommsyncid_ii,   2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjTitle,          0, BTS_STRING, dbqf_idobjtitle_i,      1, BTS_INT));

	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockDim,     0, BTS_STRING, dbqf_goodsstockdim_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockBrutto,  0, BTS_STRING, dbqf_goodsstockbrutto_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockMin,     0, BTS_STRING, dbqf_goodsstockmin_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockPackage, 0, BTS_STRING, dbqf_goodsstockpack_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsSingleBarcode, 0, BTS_STRING, dbqf_goodssinglebarcode_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdReportTypeName,    0, BTS_STRING, dbqf_rpttypename_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLogFileName,       0, BTS_STRING, dbqf_logfilename_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSysJActionName,    0, BTS_STRING, dbqf_sysjaction_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGtaJActionName,    0, BTS_STRING, dbqf_gtajaction_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCounter,           0, BTS_INT,    dbqf_counter_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPropSubStr,        0, BTS_STRING, dbqf_propsubstr_iiii,    4, BTS_INT, BTS_INT, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckUserID,       0, BTS_INT,    dbqf_checkuserid_ii,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckWmsLocID,     0, BTS_INT,    dbqf_checkwmsloc_ii,     2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTransportTypeName, 0, BTS_STRING, dbqf_transptypename_i,   1, BTS_INT));  // @v6.3.2 AHTOXA
	THROW(DbqFuncTab::RegisterDyn(&IdLotCloseDate,      0, BTS_DATE,   dbqf_lotclosedate_d,     1, BTS_DATE)); // @v6.3.2
	THROW(DbqFuncTab::RegisterDyn(&IdFormatCycle,       0, BTS_STRING, dbqf_formatcycle_di,     2, BTS_DATE, BTS_INT)); // @v6.3.7
	THROW(DbqFuncTab::RegisterDyn(&IdYesWordByFlag,     0, BTS_STRING, dbqf_yeswordbyflag_i,    3, BTS_INT, BTS_INT, BTS_STRING)); // @v6.5.6 AHTOXA
	THROW(DbqFuncTab::RegisterDyn(&IdBudgetPlanOrFact,  0, BTS_REAL,   dbqf_budgplanorfact_rii, 3, BTS_REAL, BTS_INT, BTS_INT)); // @v6.5.14 AHTOXA
	THROW(DbqFuncTab::RegisterDyn(&IdChkOpJActionName,  0, BTS_STRING, dbqf_chkopjaction_i,     1, BTS_INT)); // @v6.6.10 AHTOXA
	THROW(DbqFuncTab::RegisterDyn(&IdAddrCityName,      0, BTS_STRING, dbqf_addr_city_name_i,   1, BTS_INT));          // @v7.0.9
	THROW(DbqFuncTab::RegisterDyn(&IdAddrExField,       0, BTS_STRING, dbqf_addr_ex_field_ii,   2, BTS_INT, BTS_INT)); // @v7.0.9
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNode,    0, BTS_INT,    dbqf_checkcsposnode_ii,  2, BTS_INT, BTS_INT)); // @v7.6.3  (csessID, posNodeID)
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNodeList,0, BTS_INT,    dbqf_checkcsposnodelist_ii, 2, BTS_INT, BTS_INT)); // @v7.6.3  (csessID, (const LongArray *))
	THROW(DbqFuncTab::RegisterDyn(&IdStrExistSubStr,    0, BTS_INT,    dbqf_strexistsub_ss,     2, BTS_STRING, BTS_STRING)); // @vmiller
	THROW(DbqFuncTab::RegisterDyn(&IdAddedCreditLimit,  0, BTS_REAL,   dbqf_addedcreditlimit_rii, 3, BTS_REAL, BTS_INT, BTS_INT)); // @v8.2.4
	THROW(DbqFuncTab::RegisterDyn(&IdGetAgrmntSymbol,   0, BTS_STRING, dbqf_getagrmntsymbol_i,  1, BTS_INT)); // @vmiller
	THROW(DbqFuncTab::RegisterDyn(&IdRegisterText,      0, BTS_STRING, dbqf_registertext_i,     1, BTS_INT)); // @v8.4.4
	THROW(DbqFuncTab::RegisterDyn(&IdObjTagText,        0, BTS_STRING, dbqf_objtagtext_ii,      2, BTS_INT, BTS_INT)); // @v8.4.11
	THROW(DbqFuncTab::RegisterDyn(&IdDateRange,         0, BTS_STRING, dbqf_daterange_dd,       2, BTS_DATE, BTS_DATE)); // @v8.6.4
	THROW(DbqFuncTab::RegisterDyn(&IdOidText,           0, BTS_STRING, dbqf_oidtext_ii,         2, BTS_INT, BTS_INT)); // @v8.6.11
	THROW(DbqFuncTab::RegisterDyn(&IdDateBase,          0, BTS_DATE,   dbqf_datebase_id,        2, BTS_INT, BTS_DATE)); // @v8.6.11
	CATCHZOK
	return ok;
}

// static
int SLAPI PPDbqFuncPool::InitObjNameFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push((DBFunc)funcId);
	return 1;
}

//static
int SLAPI PPDbqFuncPool::InitLongFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push((DBFunc)funcId);
	return 1;
}

//static
int SLAPI PPDbqFuncPool::InitPctFunc(DBE & rDbe, DBField & rFld1, DBField & rFld2, int incDiv)
{
	rDbe.init();
	rDbe.push(rFld1);
	rDbe.push(rFld2);
	rDbe.push(incDiv ? (DBFunc)PPDbqFuncPool::IdPercentIncDiv : (DBFunc)PPDbqFuncPool::IdPercent);
	return 1;
}

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
