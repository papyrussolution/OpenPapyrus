// PPDBQF.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
//
#include <pp.h>
#pragma hdrstop

#if 0 // {
/*static*/PPID FASTCALL PPDbqFuncPool::helper_dbq_name(const DBConst * params, char * pNameBuf)
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

int STDCALL DbeInitSize(int option, DBConst * result, size_t s)
{
	if(option == CALC_SIZE) {
		result->init(s);
		return 1;
	}
	else
		return 0;
}

template <class objcls, class objrec> inline void dbqf_objname_i(int option, DBConst * result, const DBConst * params)
{
	objrec rec;
	if(!DbeInitSize(option, result, sizeof(rec.Name))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id) {
			objcls obj;
			obj.Fetch(id, &rec);
			if(rec.Name[0] == 0)
				ideqvalstr(id, rec.Name, sizeof(rec.Name));
		}
		result->InitForeignStr(SLS.AcquireRvlStr() = rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_oidtext_ii)
{
    char   name_buf[128+48];
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PTR32(name_buf)[0] = 0;
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
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PTR32(name_buf)[0] = 0;
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

static IMPL_DBE_PROC(dbqf_unxtext_iii)
{
	char   text_buf[2048];
	if(!DbeInitSize(option, result, sizeof(text_buf))) {
		PTR32(text_buf)[0] = 0;
		PPID   obj_type  = params[0].lval;
		PPID   obj_id    = params[1].lval;
		PPID   text_prop = params[2].lval;
        SString & r_temp_buf = SLS.AcquireRvlStr();
		PPRef->UtrC.GetText(TextRefIdent(obj_type, obj_id, static_cast<int16>(text_prop)), r_temp_buf);
		r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		STRNSCPY(text_buf, r_temp_buf);
		result->init(text_buf);
	}
}

static IMPL_DBE_PROC(dbqf_techcapacity_ir) // @v11.3.10
{
	char   text_buf[128];
	if(!DbeInitSize(option, result, sizeof(text_buf))) {
		PTR32(text_buf)[0] = 0;
		PPID   prc_id  = params[0].lval;
		double capacity = params[1].rval;
        SString & r_temp_buf = SLS.AcquireRvlStr();
		if(capacity > 0.0) {
			bool is_capacity_rev = false;
			if(prc_id) {
				PPObjProcessor prc_obj;
				ProcessorTbl::Rec prc_rec;
				if(prc_obj.GetRecWithInheritance(prc_id, &prc_rec, 1) > 0 && prc_rec.Flags & PRCF_TECHCAPACITYREV)
					is_capacity_rev = true;
			}
			if(is_capacity_rev)
				r_temp_buf.Cat(1.0 / capacity, MKSFMTD(0, 0, 0));
			else
				r_temp_buf.Cat(capacity, MKSFMTD(0, 10, NMBF_NOTRAILZ|NMBF_NOZERO));
		}
		STRNSCPY(text_buf, r_temp_buf);
		result->init(text_buf);
	}
}

template <class objcls, class objrec> inline void dbqf_objsymb_i(int option, DBConst * result, const DBConst * params)
{
	objrec rec;
	if(!DbeInitSize(option, result, sizeof(rec.Symb))) {
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
	char   buf[8];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		PTR32(buf)[0] = 0;
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_tseslnflags_i)
{
	if(!DbeInitSize(option, result, 8)) {
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
	if(!DbeInitSize(option, result, sizeof(ret_buf))) {
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
	if(!DbeInitSize(option, result, sizeof(ret_buf))) {
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
	if(!DbeInitSize(option, result, sizeof(static_cast<const ArticleTbl::Rec *>(0)->Name))) {
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
	if(!DbeInitSize(option, result, buffer_size)) {
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

static IMPL_DBE_PROC(dbqf_objregistertext_iii)
{
	const  size_t buffer_size = 128;
	if(!DbeInitSize(option, result, buffer_size)) {
		SString temp_buf;
		//(registerTypeID, objtype, objid)
		const PPID reg_type = params[0].lval;
		PPObjID oid(params[1].lval, params[2].lval);
		if(reg_type && oid.Obj && oid.Id) {
			PPID   reg_id = 0;
			PPObjRegister reg_obj;
			RegisterTbl::Rec reg_rec;
			if(reg_obj.P_Tbl->SearchByObj(oid, reg_type, &reg_rec) > 0) {
				temp_buf.Z();
				if(reg_rec.Serial[0])
					temp_buf.Cat(reg_rec.Serial);
				if(reg_rec.Num[0])
					temp_buf.CatDivIfNotEmpty('-', 1).Cat(reg_rec.Num);
				if(checkdate(reg_rec.Dt) || checkdate(reg_rec.Expiry)) {
					temp_buf.CatDivIfNotEmpty(' ', 0).CatChar('[');
					if(checkdate(reg_rec.Dt))
						temp_buf.Cat(reg_rec.Dt, DATF_DMY);
					temp_buf.Dot().Dot();
					if(checkdate(reg_rec.Expiry))
						temp_buf.Cat(reg_rec.Expiry, DATF_DMY);
					temp_buf.CatChar(']');
				}
				//PPObjRegister::Format(reg_rec, "@regsn - @regno [@date..@expiry]", temp_buf);
			}
		}
		result->init(temp_buf.Trim(buffer_size-1));
	}
}

static IMPL_DBE_PROC(dbqf_objtagtextnocache_ii)
{
	const  size_t buffer_size = 128;
	if(!DbeInitSize(option, result, buffer_size)) {
		SString temp_buf;
		const PPID tag_id = params[0].lval;
		const PPID obj_id = params[1].lval;
		if(tag_id && obj_id) {
			Reference * p_ref = PPRef;
			if(p_ref) {
				PPObjTag tag_obj;
				PPObjectTag tag_rec;
				if(tag_obj.Fetch(tag_id, &tag_rec) > 0)
					p_ref->Ot.GetTagStr(tag_rec.ObjTypeID, obj_id, tag_id, temp_buf);
			}
		}
		result->init(temp_buf.Trim(buffer_size-1));
	}
}

static IMPL_DBE_PROC(dbqf_objtagtext_ii)
{
	const  size_t buffer_size = 128;
	if(!DbeInitSize(option, result, buffer_size)) {
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
	if(!DbeInitSize(option, result, 24)) {
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
		// @v10.6.4 MEMSZERO(lot_rec);
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
	if(!DbeInitSize(option, result, field_len)) {
		LDATETIME dtm;
		dtm.Set(params[0].dval, params[1].tval);
		SString & r_temp_buf = SLS.AcquireRvlStr();
		result->init(r_temp_buf.Cat(dtm).Trim(field_len));
	}
}

static IMPL_DBE_PROC(dbqf_durationtotime_dt)
{
	const size_t field_len = 20;
	if(!DbeInitSize(option, result, field_len)) {
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
		r = static_cast<long>(w_obj.IsChildOf(params[0].lval, params[1].lval));
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
	const LongArray * p_list = static_cast<const LongArray *>(params[1].ptrval);
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
	if(!DbeInitSize(option, result, sizeof(text_buf))) {
		const SStrGroup * p_pool = static_cast<const SStrGroup *>(params[1].ptrval);
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

static IMPL_DBE_PROC(dbqf_istxtuuideq_ss)
{
	long   is_eq = 0;
	const char * p_s1 = params[0].sptr;
	const char * p_s2 = params[1].sptr;
	if(!isempty(p_s1) && !isempty(p_s2)) {
		S_GUID u1;
		S_GUID u2;
		if(u1.FromStr(p_s1) && u2.FromStr(p_s2) && u1 == u2)
			is_eq = 1;
	}
	result->init(is_eq);
}

static IMPL_DBE_PROC(dbqf_ariscatperson_ii) // @v11.1.9
{
	long    r = 0;
	const PPID ar_id = params[0].lval;
	const PPID cat_id = params[1].lval;
	if(cat_id) {
		if(ar_id) {
			const PPID psn_id = ObjectToPerson(ar_id, 0);
			if(psn_id) {
				PPObjPerson psn_obj;
				PersonTbl::Rec psn_rec;
				if(psn_obj.Fetch(psn_id, &psn_rec) > 0 && psn_rec.CatID == cat_id)
					r = 1;
			}
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
	if(!DbeInitSize(option, result, result_size)) {
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
	if(!DbeInitSize(option, result, sizeof(rec.Name))) {
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
	if(!DbeInitSize(option, result, sizeof(rec.Name))) {
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
	if(!DbeInitSize(option, result, sizeof(rec.Code))) {
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
	if(!DbeInitSize(option, result, sizeof(goods_rec.Name))) {
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
	if(!DbeInitSize(option, result, sizeof(rec.Name))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Name);
		if(id)
			GetOpData(id, &rec);
		result->init(rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objname_salcharge_i)
{
	PPSalChargePacket pack;
	if(!DbeInitSize(option, result, sizeof(pack.Rec.Name))) {
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
	if(!DbeInitSize(option, result, sizeof(pack.Rec.Name))) {
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
		PTR32(zero_str)[0] = 0;
		result->init(zero_str);
	}
}

static IMPL_DBE_PROC(dbqf_objname_loc_i)
{
	//dbqf_objname_i <PPObjLocation, LocationTbl::Rec> (option, result, params);
	LocationTbl::Rec rec;
	if(!DbeInitSize(option, result, sizeof(rec.Name))) {
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

static IMPL_DBE_PROC(dbqf_objname_billstatus_i) { dbqf_objname_i <PPObjBillStatus, PPBillStatus> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_ar_i) { dbqf_objname_i <PPObjArticle, ArticleTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_unit_i) { dbqf_objname_i <PPObjUnit, PPUnit> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_prc_i) { dbqf_objname_i <PPObjProcessor, ProcessorTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_goods_i) { dbqf_objname_i <PPObjGoods, Goods2Tbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_person_i) { dbqf_objname_i <PPObjPerson, PersonTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_staff_i) { dbqf_objname_i <PPObjStaffList, PPStaffEntry> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_staffcal_i) { dbqf_objname_i <PPObjStaffCal, PPStaffCal> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_accsheet_i) { dbqf_objname_i <PPObjAccSheet, PPAccSheet> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_quotkind_i) { dbqf_objname_i <PPObjQuotKind, PPQuotKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_cashnode_i) { dbqf_objname_i <PPObjCashNode, PPCashNode> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_psnopkind_i) { dbqf_objname_i <PPObjPsnOpKind, PPPsnOpKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_brand_i) { dbqf_objname_i <PPObjBrand, PPBrand> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objsymb_currency_i) { dbqf_objsymb_i <PPObjCurrency, PPCurrency> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_world_i) { dbqf_objname_i <PPObjWorld, WorldTbl::Rec> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_personstatus_i) { dbqf_objname_i <PPObjPersonStatus, PPPersonStatus> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_personcat_i) { dbqf_objname_i <PPObjPersonCat, PPPersonCat> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_amttype_i) { dbqf_objname_i <PPObjAmountType, PPAmountType> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_psnkind_i) { dbqf_objname_i <PPObjPersonKind, PPPersonKind> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_scardser_i) { dbqf_objname_i <PPObjSCardSeries, PPSCardSeries> (option, result, params); }
static IMPL_DBE_PROC(dbqf_objname_debtdim_i) { dbqf_objname_i <PPObjDebtDim, PPDebtDim> (option, result, params); }

static IMPL_DBE_PROC(dbqf_objname_scale_i) 
{ 
	PPScalePacket pack;
	if(!DbeInitSize(option, result, sizeof(pack.Rec.Name))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, pack.Rec.Name);
		if(id) {
			PPObjScale sc_obj;
			sc_obj.Fetch(id, &pack);
			if(pack.Rec.Name[0] == 0)
				ideqvalstr(id, pack.Rec.Name, sizeof(pack.Rec.Name));
		}
		result->InitForeignStr(pack.Rec.Name);
	}
}

static IMPL_DBE_PROC(dbqf_objcode_scard_i)
{
	SCardTbl::Rec rec;
	if(!DbeInitSize(option, result, sizeof(rec.Code))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, rec.Code);
		if(id) {
			PPObjSCard sc_obj;
			sc_obj.Fetch(id, &rec);
			if(rec.Code[0] == 0)
				ideqvalstr(id, rec.Code, sizeof(rec.Code));
		}
		result->InitForeignStr(rec.Code);
	}
}

static IMPL_DBE_PROC(dbqf_scardownername_i)
{
	char   psn_name[128];
	if(!DbeInitSize(option, result, sizeof(psn_name))) {
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
	if(!DbeInitSize(option, result, sizeof(psn_name))) {
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
	if(!DbeInitSize(option, result, sizeof(psn_name))) {
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
	if(!DbeInitSize(option, result, sizeof(name))) {
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
	if(!DbeInitSize(option, result, sizeof(name))) {
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
	long   func_id = 0L;
	char   name[128];
	if(!DbeInitSize(option, result, sizeof(name))) {
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
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
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
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PTR32(name_buf)[0] = 0;
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
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PTR32(name_buf)[0] = 0;
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
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
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
	char   name_buf[48]; // @v11.1.12 [24]-->[48]
	BillTbl::Rec rec;
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
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
	// @v11.1.12 BillTbl::Rec rec;
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			// @v11.1.12 BillObj->Fetch(id, &rec);
			// @v11.1.12 STRNSCPY(name_buf, rec.Memo);
			// @v11.1.12 {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			if(id) {
				PPRef->UtrC.GetText(TextRefIdent(PPOBJ_BILL, id, PPTRPROP_MEMO), r_temp_buf);
				r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			STRNSCPY(name_buf, r_temp_buf);
			// } @v11.1.12 
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objmemo_person_i) // @v11.1.12
{
	char   name_buf[512];
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			if(id) {
				PPRef->UtrC.GetText(TextRefIdent(PPOBJ_PERSON, id, PPTRPROP_MEMO), r_temp_buf);
				r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			STRNSCPY(name_buf, r_temp_buf);
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objmemo_personevent_i) // @v11.1.12
{
	char   name_buf[512];
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			SString & r_temp_buf = SLS.AcquireRvlStr();
			if(id) {
				PPRef->UtrC.GetText(TextRefIdent(PPOBJ_PERSONEVENT, id, PPTRPROP_MEMO), r_temp_buf);
				r_temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
			STRNSCPY(name_buf, r_temp_buf);
		}
		result->init(name_buf);
	}
}

static IMPL_DBE_PROC(dbqf_objname_acctrel_i)
{
	char   name_buf[32];
	AcctRelTbl::Rec rec;
	if(!DbeInitSize(option, result, sizeof(name_buf))) {
		PPID   id = PPDbqFuncPool::helper_dbq_name(params, name_buf);
		if(id) {
			AcctRel * p_tbl = &BillObj->atobj->P_Tbl->AccRel;
			if(p_tbl->Fetch(id, &rec) > 0) {
				if(ObjRts.CheckAccID(rec.AccID, PPR_READ))
					reinterpret_cast<const Acct *>(&rec.Ac)->ToStr(ACCF_DEFAULT, name_buf);
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
	const long   flags = params[0].lval;
	const double diff_qtty = params[1].rval;
	result->init(INVENT_DIFFSIGN(flags) * diff_qtty);
}


static IMPL_DBE_PROC(dbqf_inventlnstatus_ii)
{
	if(!DbeInitSize(option, result, 128)) {
		const long   flags = params[0].lval;
		const long   bill_id = params[1].lval;
		SString temp_buf;
		if(flags & INVENTF_WRITEDOFF) {
			PPLoadString("inventf_writedoff", temp_buf);
		}
		else if(flags & (INVENTF_LACK|INVENTF_SURPLUS)) {
			PPLoadString("inventf_unwritedoff", temp_buf);
		}
		else {
			BillTbl::Rec bill_rec;
			if(BillObj->Fetch(bill_id, &bill_rec) > 0 && bill_rec.Flags & BILLF_CLOSEDORDER)
				PPLoadString("inventf_writedoff_zb", temp_buf);
		}
		if(temp_buf.NotEmpty())
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER).Quot('(', ')');
		result->init(temp_buf);
	}
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
		PPCommSyncID comm_id;
		comm_id.P = static_cast<short>(params[0].lval);
		comm_id.I = params[1].lval;
		SString temp_buf;
		comm_id.ToStr(0, temp_buf).CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_idobjtitle_i)
{
	char   buf[64];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		SString temp_buf;
		DS.GetObjectTitle(params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_sysjaction_i)
{
	char   buf[64];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		SString temp_buf;
		PPLoadString(PPSTR_ACTION, params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_gtajaction_i)
{
	char   buf[64];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		SString temp_buf;
		PPLoadString(PPSTR_GTA, params[0].lval, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_yeswordbyflag_i)
{
	char   buf[12];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		long flags = params[0].lval;
		long flag  = params[1].lval;
		char * p_out_symb = params[2].sptr;
		if(flags & flag) {
			SString temp_buf(p_out_symb);
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
	double amt = params[0].rval;
	const long kind    = params[1].lval;
	const long is_fact = params[2].lval;
	if(kind != is_fact)
		amt = 0.0;
	result->init(amt);
}

static IMPL_DBE_PROC(dbqf_chkopjaction_i)
{
	char   buf[64];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		SString temp_buf;
		PPGetSubStr(PPTXT_CHKOPACTIONLIST, params[0].lval - 1, temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_addr_city_name_i)
{
	char   buf[48];
	if(!DbeInitSize(option, result, sizeof(buf))) {
		buf[0] = 0;
		const PPID   id = params[0].lval;
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
int PPDbqFuncPool::IdEmpty       = 0;
int PPDbqFuncPool::IdBillDebt          	 = 0;
int PPDbqFuncPool::IdCQtty             	 = 0;
int PPDbqFuncPool::IdTrfrPrice         	 = 0;
int PPDbqFuncPool::IdObjNameBillStatus 	 = 0;
int PPDbqFuncPool::IdObjNameOprKind    	 = 0;
int PPDbqFuncPool::IdObjNameLoc        	 = 0;
int PPDbqFuncPool::IdObjNameAr         	 = 0;
int PPDbqFuncPool::IdObjNameArByAcc    	 = 0; //
int PPDbqFuncPool::IdObjNameUser       	 = 0;
int PPDbqFuncPool::IdObjNameGlobalUser 	 = 0; // 
int PPDbqFuncPool::IdObjNameUnit       	 = 0;
int PPDbqFuncPool::IdObjNameTech = 0;
int PPDbqFuncPool::IdObjNameGoodsByTech  = 0; //
int PPDbqFuncPool::IdObjNamePrc  = 0;
int PPDbqFuncPool::IdObjNameGoods      	 = 0;
int PPDbqFuncPool::IdObjNamePerson     	 = 0; //
int PPDbqFuncPool::IdObjNameSalCharge  	 = 0; //
int PPDbqFuncPool::IdObjNameStaff      	 = 0; //
int PPDbqFuncPool::IdObjNameStaffCal   	 = 0; //
int PPDbqFuncPool::IdObjNamePersonPost 	 = 0; //
int PPDbqFuncPool::IdObjStaffOrg       	 = 0; // 
int PPDbqFuncPool::IdObjStaffDiv       	 = 0; // 
int PPDbqFuncPool::IdObjNameAccSheet   	 = 0; //
int PPDbqFuncPool::IdObjNameQuotKind   	 = 0; //
int PPDbqFuncPool::IdObjNameCashNode   	 = 0; //
int PPDbqFuncPool::IdObjNameScale      	 = 0; //
int PPDbqFuncPool::IdObjNamePsnOpKind  	 = 0; //
int PPDbqFuncPool::IdObjNameBizScore   	 = 0; //
int PPDbqFuncPool::IdObjNameAcctRel    	 = 0; //
int PPDbqFuncPool::IdObjNameBrand      	 = 0; //
int PPDbqFuncPool::IdObjNameWorld      	 = 0; //
int PPDbqFuncPool::IdObjNamePersonStatus = 0; //
int PPDbqFuncPool::IdObjNamePersonCat    = 0; //
int PPDbqFuncPool::IdObjNameAmountType 	 = 0; //
int PPDbqFuncPool::IdObjNamePsnKind    	 = 0; //
int PPDbqFuncPool::IdObjSymbCurrency   	 = 0; //
int PPDbqFuncPool::IdObjCodeBillCmplx  	 = 0; // (fldBillID)
int PPDbqFuncPool::IdObjCodeBill       	 = 0; // (fldBillID)
int PPDbqFuncPool::IdObjMemoBill       	 = 0; // (fldBillID)
int PPDbqFuncPool::IdObjNameSCardSer   	 = 0; //
int PPDbqFuncPool::IdObjNameDebtDim    	 = 0; //
int PPDbqFuncPool::IdDateTime          	 = 0; // (fldDate, fldTime)
int PPDbqFuncPool::IdInventDiffQtty    	 = 0; //
int PPDbqFuncPool::IdInventLnStatus    	 = 0; // @v10.5.8 (fldFlags, fldBillID)
int PPDbqFuncPool::IdTSesLnPhQtty      	 = 0; //
int PPDbqFuncPool::IdTSesLnFlags       	 = 0; //
int PPDbqFuncPool::IdPercent           	 = 0; // (100 * fld1 / fld2)
int PPDbqFuncPool::IdPercentIncDiv     	 = 0; // (100 * fld1 / (fld2+fld1))
int PPDbqFuncPool::IdPercentAddendum   	 = 0; // (100 * (fld1-fld2) / fld2)
int PPDbqFuncPool::IdWorldIsMemb       	 = 0; //
int PPDbqFuncPool::IdTaCost            	 = 0; //
int PPDbqFuncPool::IdTaPrice           	 = 0; //
int PPDbqFuncPool::IdCommSyncId        	 = 0; //
int PPDbqFuncPool::IdDurationToTime    	 = 0; //
int PPDbqFuncPool::IdObjTitle          	 = 0; //
int PPDbqFuncPool::IdGoodsStockDim     	 = 0; //
int PPDbqFuncPool::IdGoodsStockBrutto  	 = 0; //
int PPDbqFuncPool::IdGoodsStockMin     	 = 0; //
int PPDbqFuncPool::IdGoodsStockPackage 	 = 0; //
int PPDbqFuncPool::IdGoodsSingleBarcode  = 0; //
int PPDbqFuncPool::IdReportTypeName      = 0; //
int PPDbqFuncPool::IdLogFileName       	 = 0; //
int PPDbqFuncPool::IdSysJActionName    	 = 0; //
int PPDbqFuncPool::IdGtaJActionName    	 = 0; // 
int PPDbqFuncPool::IdCounter           	 = 0; //
int PPDbqFuncPool::IdPropSubStr        	 = 0; // (ObjType, ObjID, PropID, Sub)
int PPDbqFuncPool::IdCheckUserID       	 = 0; // (userID, filtUserID)
int PPDbqFuncPool::IdCheckWmsLocID     	 = 0; //
int PPDbqFuncPool::IdTransportTypeName 	 = 0; //
int PPDbqFuncPool::IdLotCloseDate      	 = 0; //
int PPDbqFuncPool::IdFormatCycle       	 = 0; //
int PPDbqFuncPool::IdYesWordByFlag     	 = 0; //
int PPDbqFuncPool::IdBudgetPlanOrFact  	 = 0; //
int PPDbqFuncPool::IdChkOpJActionName  	 = 0; //
int PPDbqFuncPool::IdAddrCityName      	 = 0; // (locID)
int PPDbqFuncPool::IdAddrExField       	 = 0; // (locID, locExFld)
int PPDbqFuncPool::IdObjCodeSCard      	 = 0; // (fldSCardID)
int PPDbqFuncPool::IdSCardOwnerName    	 = 0; // (fldSCardID)
int PPDbqFuncPool::IdUsrPersonName     	 = 0; // (fldUsrID)
int PPDbqFuncPool::IdLocOwnerName      	 = 0; // (fldLocID) Формирует строку с именем персоналии-владельца локации
int PPDbqFuncPool::IdUfpFuncName       	 = 0; // (fldFuncId)
int PPDbqFuncPool::IdVersionText       	 = 0; // (fldVersion)
int PPDbqFuncPool::IdUfpFuncId         	 = 0; // (fldFuncId)
int PPDbqFuncPool::IdCheckCsPosNode    	 = 0; // (csessID, posNodeID)
int PPDbqFuncPool::IdCheckCsPosNodeList  = 0; // (csessID, (const LongArray *))
int PPDbqFuncPool::IdStrExistSubStr      = 0; // @vmiller
int PPDbqFuncPool::IdAddedCreditLimit    = 0; // 
int PPDbqFuncPool::IdBillFrghtIssueDt    = 0; // (billID)
int PPDbqFuncPool::IdBillFrghtArrvlDt    = 0; // (billID)
int PPDbqFuncPool::IdBillFrghtDlvrAddr   = 0; // (billID)
int PPDbqFuncPool::IdGetAgrmntSymbol     = 0; // @vmiller
int PPDbqFuncPool::IdBillAgentName       = 0; // (billID) Наименование агента по документу (извлекается из записи расширения документа)
int PPDbqFuncPool::IdRegisterText        = 0; // (registerID) Текст описания регистрационного документа
int PPDbqFuncPool::IdObjRegisterText     = 0; // (registerTypeID, objtype, objid)
int PPDbqFuncPool::IdObjTagText  = 0; // (tagid, objid) Текстовое представление тега объекта
int PPDbqFuncPool::IdObjTagText_NoCache  = 0; // @v10.3.8
int PPDbqFuncPool::IdDateRange   = 0; // 
//int PPDbqFuncPool::IdObjNameOpTypeK    = 0; // 
int PPDbqFuncPool::IdOidText     = 0; // (objType, objID) Текстовое представление полного OID
int PPDbqFuncPool::IdDateBase    = 0; // (dateValue, baseDate) Текстовое представление даты, сжатой в виде количества дней, прошедших с baseDate
int PPDbqFuncPool::IdBillFrghtStrgLoc    = 0; // 
int PPDbqFuncPool::IdSCardExtString      = 0; // (scardID, fldId)
int PPDbqFuncPool::IdStrByStrGroupPos    = 0; // (position, (const SStrGroup *)) Возвращает строку из пула строк, идентифицируемую позицией position
int PPDbqFuncPool::IdBillDate    = 0; // @v10.0.03
int PPDbqFuncPool::IdUnxText     = 0; // @v10.7.2  
int PPDbqFuncPool::IdIsTxtUuidEq = 0; // @v10.9.10
int PPDbqFuncPool::IdArIsCatPerson       = 0; // @v11.1.9 (fldArticle, personCategoryID) Определяет соотносится ли статья fldArticle с персоналией, имеющей категорию personCategoryID
int PPDbqFuncPool::IdObjMemoPerson       = 0; // @v11.1.12 (fldPersonID)
int PPDbqFuncPool::IdObjMemoPersonEvent  = 0; // @v11.1.12 (fldPersonEventID)
int PPDbqFuncPool::IdTechCapacity        = 0; // @v11.3.10 (fldPrcID, fldCapacity)

static IMPL_DBE_PROC(dbqf_goodsstockdim_i)
{
	char   buf[32];
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
		SString temp_buf;
		if(params[0].lval == ReportFilt::rpttStandart)
			PPLoadString("rptstd", temp_buf);
		else
			PPLoadString("rptlocal", temp_buf);
		temp_buf.CopyTo(buf, sizeof(buf)); // @v10.3.11 @fix temp_buf.ToOem().-->temp_buf.
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_logfilename_i)
{
	if(!DbeInitSize(option, result, 32)) {
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
	long * ptr = reinterpret_cast<long *>(params[0].lval);
	long   val = DEREFPTRORZ(ptr) + 1;
	result->init(val);
	ASSIGN_PTR(ptr, val);
}

//int PPDbqFuncPool::IdPropSubStr        = 0; // @v5.9.10 (ObjType, ObjID, PropID, Sub)

static IMPL_DBE_PROC(dbqf_propsubstr_iiii)
{
	if(!DbeInitSize(option, result, 252)) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
		LDATE dt = params[0].dval;
		const PPCycleArray * p_list = reinterpret_cast<const PPCycleArray *>(params[1].lval);
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
	if(!DbeInitSize(option, result, sizeof(buf))) {
		DateRange period;
		period.low = params[0].dval;
		period.upp = params[1].dval;
		periodfmtex(&period, buf, sizeof(buf));
		result->init(buf);
	}
}

static IMPL_DBE_PROC(dbqf_datebase_id)
{
	const int sd = params[0].lval;
	const LDATE base = params[1].dval;
	const LDATE _d = (sd <= 0 || !checkdate(base, 1)) ? ZERODATE : plusdate(base, sd);
	result->init(_d);
}

/*static*/int PPDbqFuncPool::Register()
{
	//
	// @v10.9.12 Провел эксперимент: определил все параметры вызовов через таблицу и в цикле вызвал 
	// DbqFuncTab::RegisterDyn со значениями из таблицы. Результат: +3Kb к размеру obj-файла. 
	// Другими словами, так как сейчас - хорошо и табличный вариант рассматривать не надо.
	//
	int    ok = 1;
	THROW(DbqFuncTab::RegisterDyn(&IdEmpty,               BTS_STRING, dbqf_empty,                  0));
	THROW(DbqFuncTab::RegisterDyn(&IdBillDebt,            BTS_REAL,   dbqf_debt_rrii,              4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtIssueDt,    BTS_DATE,   dbqf_billfrghtissuedt_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtArrvlDt,    BTS_DATE,   dbqf_billfrghtarrvldt_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtDlvrAddr,   BTS_STRING, dbqf_billfrghtdlvraddr_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillFrghtStrgLoc,    BTS_STRING, dbqf_billfrghtstrgloc_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillAgentName,       BTS_STRING, dbqf_billagentname_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdBillDate,            BTS_DATE,   dbqf_billdate_i,             1, BTS_INT)); // @v10.0.03
	THROW(DbqFuncTab::RegisterDyn(&IdCQtty,               BTS_STRING, dbqf_cqtty_rrii,             4, BTS_REAL, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBillStatus,   BTS_STRING, dbqf_objname_billstatus_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameOprKind,      BTS_STRING, dbqf_objname_oprkind_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameLoc,          BTS_STRING, dbqf_objname_loc_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAr,           BTS_STRING, dbqf_objname_ar_i,           1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameArByAcc,      BTS_STRING, dbqf_objname_arbyacc_i,      2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUser,         BTS_STRING, dbqf_objname_user_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGlobalUser,   BTS_STRING, dbqf_objname_globaluser_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameUnit,         BTS_STRING, dbqf_objname_unit_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameTech,         BTS_STRING, dbqf_objname_tech_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoodsByTech,  BTS_STRING, dbqf_objname_goodsbytech_i,  1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePrc,          BTS_STRING, dbqf_objname_prc_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameGoods,        BTS_STRING, dbqf_objname_goods_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePerson,       BTS_STRING, dbqf_objname_person_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSalCharge,    BTS_STRING, dbqf_objname_salcharge_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaff,        BTS_STRING, dbqf_objname_staff_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameStaffCal,     BTS_STRING, dbqf_objname_staffcal_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonPost,   BTS_STRING, dbqf_objname_personpost_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffOrg,         BTS_STRING, dbqf_stafforgname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjStaffDiv,         BTS_STRING, dbqf_staffdivname_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAccSheet,     BTS_STRING, dbqf_objname_accsheet_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameQuotKind,     BTS_STRING, dbqf_objname_quotkind_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameCashNode,     BTS_STRING, dbqf_objname_cashnode_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameScale,        BTS_STRING, dbqf_objname_scale_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnOpKind,    BTS_STRING, dbqf_objname_psnopkind_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBizScore,     BTS_STRING, dbqf_objname_bizscore_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAcctRel,      BTS_STRING, dbqf_objname_acctrel_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameBrand,        BTS_STRING, dbqf_objname_brand_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameWorld,        BTS_STRING, dbqf_objname_world_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonStatus, BTS_STRING, dbqf_objname_personstatus_i, 1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePersonCat,    BTS_STRING, dbqf_objname_personcat_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameAmountType,   BTS_STRING, dbqf_objname_amttype_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNamePsnKind,      BTS_STRING, dbqf_objname_psnkind_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjSymbCurrency,     BTS_STRING, dbqf_objsymb_currency_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBillCmplx,    BTS_STRING, dbqf_objcodecmplx_bill_i,    1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeBill,         BTS_STRING, dbqf_objcode_bill_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjMemoBill,         BTS_STRING, dbqf_objmemo_bill_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjMemoPerson,       BTS_STRING, dbqf_objmemo_person_i,       1, BTS_INT)); // @v11.1.12
	THROW(DbqFuncTab::RegisterDyn(&IdObjMemoPersonEvent,  BTS_STRING, dbqf_objmemo_personevent_i,  1, BTS_INT)); // @v11.1.12
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameSCardSer,     BTS_STRING, dbqf_objname_scardser_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjNameDebtDim,      BTS_STRING, dbqf_objname_debtdim_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjCodeSCard,        BTS_STRING, dbqf_objcode_scard_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSCardOwnerName,      BTS_STRING, dbqf_scardownername_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLocOwnerName,        BTS_STRING, dbqf_locownername_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUsrPersonName,       BTS_STRING, dbqf_usrpersonname_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncName,         BTS_STRING, dbqf_ufpfuncname_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdVersionText,         BTS_STRING, dbqf_versionname_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUfpFuncId,           BTS_INT,    dbqf_ufpfuncid_i,            1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTrfrPrice,           BTS_REAL,   dbqf_trfrprice_irrr,         7, BTS_INT, BTS_INT, BTS_DATE, BTS_INT, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDateTime,            BTS_STRING, dbqf_datetime_dt,            2, BTS_DATE, BTS_TIME));
	THROW(DbqFuncTab::RegisterDyn(&IdInventDiffQtty,      BTS_REAL,   dbqf_invent_diffqtty_i,      2, BTS_INT, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdInventLnStatus,      BTS_STRING, dbqf_inventlnstatus_ii,      2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnPhQtty,        BTS_REAL,   dbqf_tseslnphqtty_iirr,      4, BTS_INT, BTS_INT, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTSesLnFlags,         BTS_STRING, dbqf_tseslnflags_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPercent,             BTS_REAL,   dbqf_percent_rr,             2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdPercentIncDiv,       BTS_REAL,   dbqf_percentincdiv_rr,       2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdPercentAddendum,     BTS_REAL,   dbqf_percentaddendum_rr,     2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdWorldIsMemb,         BTS_INT,    dbqf_world_ismemb_ii,        2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTaCost,              BTS_REAL,   dbqf_tacost_rr,              2, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdTaPrice,             BTS_REAL,   dbqf_taprice_rrr,            3, BTS_REAL, BTS_REAL, BTS_REAL));
	THROW(DbqFuncTab::RegisterDyn(&IdDurationToTime,      BTS_STRING, dbqf_durationtotime_dt,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCommSyncId,          BTS_STRING, dbqf_idcommsyncid_ii,        2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjTitle,            BTS_STRING, dbqf_idobjtitle_i,           1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockDim,       BTS_STRING, dbqf_goodsstockdim_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockBrutto,    BTS_STRING, dbqf_goodsstockbrutto_i,     1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockMin,       BTS_STRING, dbqf_goodsstockmin_i,        1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsStockPackage,   BTS_STRING, dbqf_goodsstockpack_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGoodsSingleBarcode,  BTS_STRING, dbqf_goodssinglebarcode_i,   1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdReportTypeName,      BTS_STRING, dbqf_rpttypename_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLogFileName,         BTS_STRING, dbqf_logfilename_i,          1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdSysJActionName,      BTS_STRING, dbqf_sysjaction_i,           1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGtaJActionName,      BTS_STRING, dbqf_gtajaction_i,           1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCounter,             BTS_INT,    dbqf_counter_i,              1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdPropSubStr,          BTS_STRING, dbqf_propsubstr_iiii,        4, BTS_INT, BTS_INT, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckUserID,         BTS_INT,    dbqf_checkuserid_ii,         2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckWmsLocID,       BTS_INT,    dbqf_checkwmsloc_ii,         2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdTransportTypeName,   BTS_STRING, dbqf_transptypename_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdLotCloseDate,        BTS_DATE,   dbqf_lotclosedate_d,         1, BTS_DATE));
	THROW(DbqFuncTab::RegisterDyn(&IdFormatCycle,         BTS_STRING, dbqf_formatcycle_di,         2, BTS_DATE, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdYesWordByFlag,       BTS_STRING, dbqf_yeswordbyflag_i,        3, BTS_INT, BTS_INT, BTS_STRING));
	THROW(DbqFuncTab::RegisterDyn(&IdBudgetPlanOrFact,    BTS_REAL,   dbqf_budgplanorfact_rii,     3, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdChkOpJActionName,    BTS_STRING, dbqf_chkopjaction_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdAddrCityName,        BTS_STRING, dbqf_addr_city_name_i,       1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdAddrExField,         BTS_STRING, dbqf_addr_ex_field_ii,       2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNode,      BTS_INT,    dbqf_checkcsposnode_ii,      2, BTS_INT, BTS_INT)); // (csessID, posNodeID)
	THROW(DbqFuncTab::RegisterDyn(&IdCheckCsPosNodeList,  BTS_INT,    dbqf_checkcsposnodelist_ii,  2, BTS_INT, BTS_PTR)); // (csessID, (const LongArray *))
	THROW(DbqFuncTab::RegisterDyn(&IdStrByStrGroupPos,    BTS_STRING, dbqf_strbystrgrouppos_ip,    2, BTS_INT, BTS_PTR)); // (position, (const SStrGroup *))
	THROW(DbqFuncTab::RegisterDyn(&IdStrExistSubStr,      BTS_INT,    dbqf_strexistsub_ss,         2, BTS_STRING, BTS_STRING));
	THROW(DbqFuncTab::RegisterDyn(&IdAddedCreditLimit,    BTS_REAL,   dbqf_addedcreditlimit_rii,   3, BTS_REAL, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdGetAgrmntSymbol,     BTS_STRING, dbqf_getagrmntsymbol_i,      1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdRegisterText,        BTS_STRING, dbqf_registertext_i,         1, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjRegisterText,     BTS_STRING, dbqf_objregistertext_iii,    3, BTS_INT)); // @v10.8.12
	THROW(DbqFuncTab::RegisterDyn(&IdObjTagText,          BTS_STRING, dbqf_objtagtext_ii,          2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdObjTagText_NoCache,  BTS_STRING, dbqf_objtagtextnocache_ii,   2, BTS_INT, BTS_INT)); // @v10.3.8
	THROW(DbqFuncTab::RegisterDyn(&IdDateRange,           BTS_STRING, dbqf_daterange_dd,           2, BTS_DATE, BTS_DATE));
	THROW(DbqFuncTab::RegisterDyn(&IdOidText,             BTS_STRING, dbqf_oidtext_ii,             2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdDateBase,            BTS_DATE,   dbqf_datebase_id,            2, BTS_INT, BTS_DATE));
	THROW(DbqFuncTab::RegisterDyn(&IdSCardExtString,      BTS_STRING, dbqf_scardextstring_ii,      2, BTS_INT, BTS_INT));
	THROW(DbqFuncTab::RegisterDyn(&IdUnxText,             BTS_STRING, dbqf_unxtext_iii,            3, BTS_INT, BTS_INT, BTS_INT)); // @v10.7.2
	THROW(DbqFuncTab::RegisterDyn(&IdIsTxtUuidEq,         BTS_INT,    dbqf_istxtuuideq_ss,         2, BTS_STRING, BTS_STRING)); // @v10.9.10
	THROW(DbqFuncTab::RegisterDyn(&IdArIsCatPerson,       BTS_INT,    dbqf_ariscatperson_ii,       2, BTS_INT, BTS_INT)); // @v11.1.9
	THROW(DbqFuncTab::RegisterDyn(&IdTechCapacity,        BTS_STRING, dbqf_techcapacity_ir,        2, BTS_INT, BTS_REAL)); // @v11.3.10
	CATCHZOK
	return ok;
}

/*static*/void STDCALL PPDbqFuncPool::InitObjNameFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push(static_cast<DBFunc>(funcId));
}

/*static*/void STDCALL PPDbqFuncPool::InitObjTagTextFunc(DBE & rDbe, PPID tagID, DBField & rFld, int dontUseCache)
{
	rDbe.init();
	rDbe.push(dbconst(tagID));
	rDbe.push(rFld);
	rDbe.push(static_cast<DBFunc>(dontUseCache ? PPDbqFuncPool::IdObjTagText_NoCache : PPDbqFuncPool::IdObjTagText));
}

/*static*/void STDCALL PPDbqFuncPool::InitLongFunc(DBE & rDbe, int funcId, DBField & rFld)
{
	rDbe.init();
	rDbe.push(rFld);
	rDbe.push(static_cast<DBFunc>(funcId));
}

/*static*/void STDCALL PPDbqFuncPool::InitFunc2Arg(DBE & rDbe, int funcId, DBItem & rA1, DBItem & rA2)
{
	rDbe.init();
	rDbe.push(rA1);
	rDbe.push(rA2);
	rDbe.push(static_cast<DBFunc>(funcId));
}

/*static*/void STDCALL PPDbqFuncPool::InitPctFunc(DBE & rDbe, DBField & rFld1, DBField & rFld2, int incDiv)
{
	rDbe.init();
	rDbe.push(rFld1);
	rDbe.push(rFld2);
	if(incDiv == 2)
		rDbe.push(static_cast<DBFunc>(IdPercentAddendum));
	else if(incDiv == 1)
		rDbe.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercentIncDiv));
	else
		rDbe.push(static_cast<DBFunc>(PPDbqFuncPool::IdPercent));
}

/*static*/void STDCALL PPDbqFuncPool::InitStrPoolRefFunc(DBE & rDbe, DBField & rFld, SStrGroup * pSg)
{
	rDbe.init();
	rDbe.push(rFld);
	{
		DBConst dbc_ptr;
		dbc_ptr.init(pSg);
		rDbe.push(dbc_ptr);
	}
	rDbe.push(static_cast<DBFunc>(PPDbqFuncPool::IdStrByStrGroupPos));
}

/*static*/PPID FASTCALL PPDbqFuncPool::helper_dbq_name(const DBConst * params, char * pNameBuf)
{
	PPID   id = 0;
	if(params[0].Tag == DBConst::lv)
		id = params[0].lval;
	else if(params[0].Tag == DBConst::rv)
		id = static_cast<long>(params[0].rval);
	PTR32(pNameBuf)[0] = 0;
	return id;
}
