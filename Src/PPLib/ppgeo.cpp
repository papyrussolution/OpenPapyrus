// PPGEO.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop
//
//
//
/*
	// Заголовок определяющий заголовочную точку и параметры всего кластера
	Indicator
	ID
	Tile
	Lat
	Lon
	// Последующие точки
	[
		{
			InferiorIndicator : byte
			Lat               : int8 || int16 || int32
			Lon               : int8 || int16 || int32
			TileLevel         : byte || 0
		}
	]
	// Связанные с точками объекты
	[

		{
			LinkIndicator : byte
			NodePos       : byte
			ID            : int32 || int64
		}
	]
*/
SLAPI PPOsm::NodeCluster::NodeCluster()
{
	SBaseBuffer::Init();
}

SLAPI PPOsm::NodeCluster::~NodeCluster()
{
	SBaseBuffer::Destroy();
}

//static
uint FASTCALL PPOsm::NodeCluster::GetPossiblePackCount(const Node * pN, size_t count)
{
	uint   possible_count = 0;
	const  int64 id = pN->ID;
	static const uint __row[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
	for(size_t i = 0; i < SIZEOFARRAY(__row); i++) {
		const uint r = __row[i];
		if((id & (r-1)) == id && count >= r) {
			possible_count = 1;
			if(r > 1) {
				uint prev_zvalue = pN[0].T.GetZValue();
				for(uint j = 1; j < r; j++) {
					const uint zval = pN[j].T.GetZValue();
					if(zval != prev_zvalue) {
						for(size_t k = i; k > 0; k--) {
							if(j >= __row[k]) {
								possible_count = __row[k];
								break;
							}
						}
						break;
					}
					else
						prev_zvalue = zval;
				}
			}
			break;
		}
	}
	return possible_count;
}

int SLAPI PPOsm::NodeCluster::Put(const Node * pN, size_t count, size_t * pActualCount)
{
	int    ok = 1;
	const  uint possible_count = GetPossiblePackCount(pN, count);
    {
    	uint8 indicator = 0;
        switch(possible_count) {
        	case 128: indicator = 7; break;
        	case 64: indicator = 6; break;
        	case 32: indicator = 5; break;
        	case 16: indicator = 4; break;
        	case 8: indicator = 3; break;
        	case 4: indicator = 2; break;
        	case 2: indicator = 1; break;
        	case 1: indicator = 0; break;
        	default:
				CALLEXCEPT(); // invalid count
        }
        if(pN[0].ID <= ULONG_MAX) {
			indicator |= indfId32;
        }
        //Put()
    }
    CATCHZOK
	return ok;
}
//
//
//
PPOsm::NodeTbl::NodeTbl(BDbDatabase * pDb) : BDbTable(BDbTable::Config("geomap.db->node", BDbTable::idxtypHash, 0), pDb)
{
}

PPOsm::NodeTbl::~NodeTbl()
{
}
//
//
//
SLAPI PPOsm::Node::Node()
{
	ID = 0;
};

SLAPI PPOsm::Way::Way()
{
	ID = 0;
}

SLAPI PPOsm::RelMember::RelMember()
{
	RefID = 0;
	TypeSymbID = 0;
	RoleSymbID = 0;
}

SLAPI PPOsm::Relation::Relation()
{
	ID = 0;
}

SLAPI PPOsm::Tag::Tag()
{
	KeySymbID = 0;
	ValID = 0;
}

SLAPI PPOsm::PPOsm() : Ht(1024*1024, 0), Grid(12)
{
	LastSymbID = 0;
}

SLAPI PPOsm::~PPOsm()
{
}

int SLAPI PPOsm::LoadGeoGrid()
{
    int    ok = -1;
    SString path, filename;
    PPGetPath(PPPATH_DD, path);
    path.SetLastSlash();
    (filename = path).Cat("planet-*-grid-12.txt");
    SDirEntry de;
    for(SDirec dir(filename, 0); dir.Next(&de) > 0;) {
		if(de.IsFile()) {
			(filename = path).Cat(de.FileName);
			THROW_SL(Grid.Load(filename));
		}
    }
    CATCHZOK
    return ok;
}

int SLAPI PPOsm::BuildHashAssoc()
{
	return Ht.BuildAssoc();
}

uint FASTCALL PPOsm::SearchSymb(const char * pSymb) const
{
	uint   val = 0;
	uint   pos = 0;
	return Ht.Search(pSymb, &val, &pos) ? val : 0;
}

uint FASTCALL PPOsm::CreateSymb(const char * pSymb)
{
	uint   val = 0;
	uint   pos = 0;
	if(!Ht.Search(pSymb, &val, &pos)) {
        val = ++LastSymbID;
        if(!Ht.Add(pSymb, val))
			val = 0;
	}
	return val;
}

int SLAPI PPOsm::GetSymbByID(uint id, SString & rSymb) const
{
	return Ht.GetByAssoc(id, rSymb);
}
//
//
//
SLAPI PPGeoTrackItem::PPGeoTrackItem()
{
	THISZERO();
}

PPGeoTrackItem & FASTCALL PPGeoTrackItem::operator = (const PPGeoTrackItem & rS)
{
	memcpy(this, &rS, sizeof(*this));
	return *this;
}

PPGeoTrackItem & FASTCALL PPGeoTrackItem::operator = (const GeoTrackTbl::Rec & rS)
{
	Oid.Set(rS.ObjType, rS.ObjID);
	ExtOid.Set(rS.ExtObjType, rS.ExtObjID);
	Dtm.Set(GeoTrackCore::ConvertStorageDate(rS.Dts2010), rS.Tm);
	ExtEvent = rS.ExtEvent;
	Flags = rS.Flags;
	Latitude = rS.Latitude;
	Longitude = rS.Longitude;
	Altitude = rS.Altitude;
	Speed = rS.Speed * 10;
	return *this;
}

int FASTCALL PPGeoTrackItem::Get(GeoTrackTbl::Rec & rD) const
{
	MEMSZERO(rD);
	rD.ObjType = (int16)Oid.Obj;
	rD.ObjID = Oid.Id;
	rD.ExtObjType = (int16)ExtOid.Obj;
	rD.ExtObjID = ExtOid.Id;
	rD.Dts2010 = GeoTrackCore::GetStorageDate(Dtm.d);
	rD.Tm = Dtm.t;
	rD.ExtEvent = (int16)ExtEvent;
	rD.Flags = Flags;
	rD.Latitude = Latitude;
	rD.Longitude = Longitude;
	rD.Altitude = (int16)R0(Altitude);
	rD.Speed = (int16)R0(Speed / 10);
	return 1;
}

//static
LDATE FASTCALL GeoTrackCore::ConvertStorageDate(int16 sd)
{
	return (sd == 0) ? ZERODATE : plusdate(encodedate(1, 1, 2010), sd);
}

//static
int16 FASTCALL GeoTrackCore::GetStorageDate(LDATE dt)
{
	int16  result = 0;
	if(dt) {
		const LDATE _basedt = encodedate(1, 1, 2010);
		if(dt >= _basedt)
			result = (int16)diffdate(dt, _basedt);
	}
	return result;
}

SLAPI GeoTrackCore::GeoTrackCore() : GeoTrackTbl()
{
}

int SLAPI GeoTrackCore::Search(PPObjID oid, LDATETIME dtm, PPGeoTrackItem * pItem)
{
	int    ok = 1;
	GeoTrackTbl::Key0 k0;
	MEMSZERO(k0);
	k0.ObjType = (int16)oid.Obj;
	k0.ObjID = oid.Id;
	k0.Dts2010 = GetStorageDate(dtm.d);
	k0.Tm = dtm.t;
	if(search(0, &k0, spEq)) {
		ASSIGN_PTR(pItem, data);
	}
	else
		ok = PPDbSearchError();
	return ok;
}

int SLAPI GeoTrackCore::PutItem(const PPGeoTrackItem & rItem, int use_ta)
{
	int    ok = 1;
	int    r;
	PPGeoTrackItem founded_item;
    GeoTrackTbl::Rec rec;
    THROW(rItem.Get(rec));
    {
    	PPTransaction tra(use_ta);
    	THROW(tra);
    	THROW(r = Search(rItem.Oid, rItem.Dtm, &founded_item));
    	if(r < 0) {
			THROW_DB(insertRecBuf(&rec));
    	}
		else {
			if(rItem.ExtOid.Obj && rItem.ExtOid.Id) {
				if(founded_item.ExtOid.Id == 0) {
					THROW_DB(updateRecBuf(&rec));
				}
			}
			ok = -1;
		}
		THROW(tra.Commit());
    }
	CATCHZOK
	return ok;
}

int SLAPI GeoTrackCore::PutChunk(const TSArray <PPGeoTrackItem> & rList, int use_ta)
{
	int    ok = -1;
	const  uint _c = rList.getCount();
	if(_c) {
    	PPTransaction tra(use_ta);
    	THROW(tra);
    	for(uint i = 0; i < _c; i++) {
			if(PutItem(rList.at(i), 0) > 0)
				ok = 1;
    	}
    	THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(GeoTracking); SLAPI GeoTrackingFilt::GeoTrackingFilt() : PPBaseFilt(PPFILT_GEOTRACKING, 0, 0)
{
	SetFlatChunk(offsetof(GeoTrackingFilt, ReserveStart),
		offsetof(GeoTrackingFilt, Reserve)+sizeof(Reserve)-offsetof(GeoTrackingFilt, ReserveStart));
	Init(1, 0);
}

int  SLAPI GeoTrackingFilt::IsEmpty() const
{
	if(!Period.IsZero())
		return 0;
	else if(Oi.Obj)
		return 0;
	else if(ExtOi.Obj)
		return 0;
	else if(Flags)
		return 0;
	else
		return 1;
}
//
//
//
SLAPI PPViewGeoTracking::PPViewGeoTracking() : PPView(0, &Filt, PPVIEW_GEOTRACKING)
{

}

SLAPI PPViewGeoTracking::~PPViewGeoTracking()
{
}

//virtual
PPBaseFilt * SLAPI PPViewGeoTracking::CreateFilt(void * extraPtr) const
{
	GeoTrackingFilt * p_filt = new GeoTrackingFilt;
	return p_filt;
}

//virtual
int  SLAPI PPViewGeoTracking::EditBaseFilt(PPBaseFilt * pBaseFilt)
{
	class GeoTrackFiltDialog : public TDialog {
	public:
		GeoTrackFiltDialog() : TDialog(DLG_GEOTRFILT)
		{
			SetupCalPeriod(CTLCAL_GEOTRFILT_PERIOD, CTL_GEOTRFILT_PERIOD);
			ObjTypeList.addzlist(PPOBJ_STYLOPALM, 0);
			ExtObjTypeList.addzlist(PPOBJ_BILL, PPOBJ_LOCATION, 0);
		}
		int    setDTS(const GeoTrackingFilt * pData)
		{
			int    ok = 1;
			if(pData)
				Data = *pData;
			SetPeriodInput(this, CTL_GEOTRFILT_PERIOD, &Data.Period);
			SetupObjListCombo(this, CTLSEL_GEOTRFILT_OBJT, Data.Oi.Obj, &ObjTypeList);
			SetupObjListCombo(this, CTLSEL_GEOTRFILT_EXTOBJT, Data.ExtOi.Obj, &ExtObjTypeList);
			SetupObjType();
			return ok;
		}
		int    getDTS(GeoTrackingFilt * pData)
		{
			int    ok = 1;
			GetPeriodInput(this, CTL_GEOTRFILT_PERIOD, &Data.Period);
			Data.Oi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_OBJT);
			if(Data.Oi.Obj)
				Data.Oi.Id = getCtrlLong(CTLSEL_GEOTRFILT_OBJ);
			else
				Data.Oi.Id = 0;
			Data.ExtOi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_EXTOBJT);
			Data.ExtOi.Id = 0;
			ASSIGN_PTR(pData, Data);
			return ok;
		}
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_GEOTRFILT_OBJT)) {
				PPID   preserved_obj_type = Data.Oi.Obj;
				Data.Oi.Obj = getCtrlLong(CTLSEL_GEOTRFILT_OBJT);
				if(Data.Oi.Obj != preserved_obj_type) {
					Data.Oi.Id = 0;
					SetupObjType();
				}
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupObjType()
		{
			if(Data.Oi.Obj == PPOBJ_STYLOPALM) {
				disableCtrl(CTLSEL_GEOTRFILT_OBJ, 0);
				SetupPPObjCombo(this, CTLSEL_GEOTRFILT_OBJ, Data.Oi.Obj, Data.Oi.Id, 0, 0);
			}
			else {
				disableCtrl(CTLSEL_GEOTRFILT_OBJ, 1);
			}
			disableCtrl(CTLSEL_GEOTRFILT_EXTOBJ, 1);
		}
		GeoTrackingFilt Data;
		PPIDArray ObjTypeList;
		PPIDArray ExtObjTypeList;
	};
	if(!Filt.IsA(pBaseFilt))
		return 0;
	GeoTrackingFilt * p_filt = (GeoTrackingFilt *)pBaseFilt;
	DIALOG_PROC_BODY(GeoTrackFiltDialog, p_filt);
}

//virtual
int SLAPI PPViewGeoTracking::Init_(const PPBaseFilt * pFilt)
{
	int    ok = 1;
	BExtInsert * p_bei = 0;
	Counter.Init();
	THROW(Helper_InitBaseFilt(pFilt));
	Filt.Period.Actualize(ZERODATE);
	CATCHZOK
	return ok;
}

int SLAPI PPViewGeoTracking::InitIteration()
{
	const  LDATE base_date = encodedate(1, 1, 2010);

	int    ok = 1;
	DBQ  * dbq = 0;
	GeoTrackTbl::Key0 k0, k0_;
	MEMSZERO(k0);
	ZDELETE(P_IterQuery);
	THROW_MEM(P_IterQuery = new BExtQuery(&T, 0));
	P_IterQuery->selectAll();
	if(Filt.Oi.Obj) {
		k0.ObjType = (int16)Filt.Oi.Obj;
		k0.ObjID = Filt.Oi.Id;
		if(Filt.Period.low > base_date) {
			k0.Dts2010 = GeoTrackCore::GetStorageDate(Filt.Period.low);
		}
	}
	dbq = ppcheckfiltid(dbq, T.ObjType, Filt.Oi.Obj);
	dbq = ppcheckfiltid(dbq, T.ObjID, Filt.Oi.Id);
	dbq = ppcheckfiltid(dbq, T.ExtObjType, Filt.ExtOi.Obj);
	dbq = ppcheckfiltid(dbq, T.ExtObjID, Filt.ExtOi.Id);
	if(!Filt.Period.IsZero()) {
		if(Filt.Period.low > base_date) {
            dbq = &(*dbq && T.Dts2010 >= diffdate(Filt.Period.low, base_date));
		}
		if(Filt.Period.upp > base_date) {
			dbq = &(*dbq && T.Dts2010 <= diffdate(Filt.Period.upp, base_date));
		}
	}
	P_IterQuery->where(*dbq);
	k0_ = k0;
	Counter.Init(P_IterQuery->countIterations(0, &k0_, spGe));
	P_IterQuery->initIteration(0, &k0, spGe);
	CATCHZOK
	return ok;
}

int SLAPI PPViewGeoTracking::NextIteration(GeoTrackingViewItem * pItem)
{
	int    ok = -1;
	if(P_IterQuery) {
		while(ok < 0 && P_IterQuery->nextIteration() > 0) {
			T.copyBufTo(pItem);
			ok = 1;
		}
	}
	return ok;
}

int FASTCALL PPViewGeoTracking::CheckRecForFilt(const GeoTrackTbl::Rec * pRec)
{
	return 1;
}

//virtual
DBQuery * SLAPI PPViewGeoTracking::CreateBrowserQuery(uint * pBrwId, SString * pSubTitle)
{
	int    add_dbe = 0;
	uint   brw_id = BROWSER_GEOTRACKING;
	DBQuery * q = 0;
	DBE    dbe_obj;
	DBE    dbe_extobj;
	DBE    dbe_dt;
	DBQ  * dbq = 0;
	GeoTrackTbl * t = new GeoTrackTbl();
	const  LDATE base_date = encodedate(1, 1, 2010);

	THROW(CheckTblPtr(t));
	{
		dbe_obj.init();
		dbe_obj.push(t->ObjType);
		dbe_obj.push(t->ObjID);
		dbe_obj.push((DBFunc)PPDbqFuncPool::IdOidText);
	}
	{
		dbe_extobj.init();
		dbe_extobj.push(t->ExtObjType);
		dbe_extobj.push(t->ExtObjID);
		dbe_extobj.push((DBFunc)PPDbqFuncPool::IdOidText);
	}
	{
		dbe_dt.init();
		dbe_dt.push(t->Dts2010);
		dbe_dt.push(dbconst(base_date));
		dbe_dt.push((DBFunc)PPDbqFuncPool::IdDateBase);
	}
	dbq = ppcheckfiltid(dbq, t->ObjType, Filt.Oi.Obj);
	dbq = ppcheckfiltid(dbq, t->ObjID, Filt.Oi.Id);
	dbq = ppcheckfiltid(dbq, t->ExtObjType, Filt.ExtOi.Obj);
	dbq = ppcheckfiltid(dbq, t->ExtObjID, Filt.ExtOi.Id);
	if(!Filt.Period.IsZero()) {
		if(Filt.Period.low > base_date) {
            dbq = &(*dbq && t->Dts2010 >= diffdate(Filt.Period.low, base_date));
		}
		if(Filt.Period.upp > base_date) {
			dbq = &(*dbq && t->Dts2010 <= diffdate(Filt.Period.upp, base_date));
		}
	}
	q = & select(
		t->ObjType,   // #0
		t->ObjID,     // #1
		t->Dts2010,   // #2
		t->Tm,        // #3
		dbe_dt,       // #4
		dbe_obj,      // #5
		dbe_extobj,   // #6
		t->Latitude,  // #7
		t->Longitude, // #8
		t->Altitude,  // #9
		t->Speed,     // #10
		0L).from(t, 0L);
	q->where(*dbq);
	THROW(CheckQueryPtr(q));
	CATCH
		if(q)
			ZDELETE(q);
		else {
			delete t;
		}
	ENDCATCH
	ASSIGN_PTR(pBrwId, brw_id);
	return q;
}

int SLAPI PPViewGeoTracking::CalcTotal(GeoTrackingTotal * pTotal)
{
	int    ok = 1;
    GeoTrackingTotal total;
    GeoTrackingViewItem item;
    PPObjIDArray obj_list;
    for(InitIteration(); NextIteration(&item) > 0;) {
		total.Count++;
		PPObjID oi;
		oi.Set(item.ObjType, item.ObjID);
		if(!obj_list.Search(oi, 0)) {
			THROW(obj_list.Add(oi.Obj, oi.Id));
		}
    }
    total.ObjCount = obj_list.getCount();
    CATCHZOK
    ASSIGN_PTR(pTotal, total);
    return ok;
}

//virtual
int SLAPI PPViewGeoTracking::ViewTotal()
{
	int    ok = -1;
	TDialog * dlg = 0;
	GeoTrackingTotal total;
	THROW(CalcTotal(&total));
	THROW(CheckDialogPtr(&(dlg = new TDialog(DLG_GEOTRTOTAL)), 1));
    dlg->setCtrlLong(CTL_GEOTRTOTAL_COUNT, total.Count);
    dlg->setCtrlLong(CTL_GEOTRTOTAL_OBJCOUNT, total.ObjCount);
    ExecViewAndDestroy(dlg);
    dlg = 0;
	CATCHZOK
	delete dlg;
	return ok;
}

//virtual
int SLAPI PPViewGeoTracking::Detail(const void *, PPViewBrowser * pBrw)
{
	return -1;
}

int SLAPI PPViewGeoTracking::Export()
{
	const  int gpx_ver_major = 1;
	const  int gpx_ver_minor = 1;
	const  char * p_schema_url = "http://www.topografix.com/GPX/";
	int    ok = -1;
	SString out_file_name, path;
	xmlTextWriterPtr writer = 0;
	SXml::WNode * p_n_trk = 0;
	SXml::WNode * p_n_trkseg = 0;
	THROW(InitIteration());
	if(GetCounter().GetTotal()) {
		SString fn_suffix;
        if(Filt.Oi.Obj && Filt.Oi.Id) {
			GetObjectName(Filt.Oi.Obj, Filt.Oi.Id, fn_suffix);
        }
		(out_file_name = 0).Cat("geo-tracking");
		if(fn_suffix.NotEmptyS()) {
			out_file_name.CatChar('-').Cat(fn_suffix.Transf(CTRANSF_INNER_TO_OUTER));
		}
		out_file_name.Dot().Cat("gpx");
		PPGetFilePath(PPPATH_OUT, out_file_name, path);
		THROW(writer = xmlNewTextWriterFilename(path, 0));
		{
			//xmlTextWriterSetIndent(writer, 1);
			//xmlTextWriterSetIndentString(writer, (const xmlChar*)" ");
			xmlTextWriterStartDocument(writer, 0, "utf-8", "yes");
			{
				SString out_buf, temp_buf;
				SString schema_loc;
				(schema_loc = p_schema_url).Cat(gpx_ver_major).CatChar('/').Cat(gpx_ver_minor);
				SXml::WNode n_gpx(writer, "gpx");
					n_gpx.PutAttrib("xmlns", schema_loc);
					(temp_buf = 0).Cat(gpx_ver_major).CatChar('.').Cat(gpx_ver_minor);
					n_gpx.PutAttrib("version", temp_buf);
					{
						PPVersionInfo vi = DS.GetVersionInfo();
						SVerT ver = vi.GetVersion();
						vi.GetProductName(temp_buf);
						(out_buf = 0).Cat(temp_buf);
						out_buf.Space().Cat(ver.ToStr(temp_buf));
					}
					n_gpx.PutAttrib("creator", out_buf);
					n_gpx.PutAttrib("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
					(temp_buf = schema_loc).Space().Cat(p_schema_url).Cat(gpx_ver_major).CatChar('/').Cat(gpx_ver_minor).CatChar('/').Cat("gpx.xsd");
					n_gpx.PutAttrib("xsi:schemaLocation", temp_buf);
				GeoTrackingViewItem item;
				PPObjID last_oid;
				last_oid.Set(0, 0);
				while(NextIteration(&item) > 0) {
					PPObjID oid;
					oid.Set(item.ObjType, item.ObjID);
					LDATETIME dtm;
					dtm.Set(GeoTrackCore::ConvertStorageDate(item.Dts2010), item.Tm);
					if(!p_n_trk || oid != last_oid) {
						ZDELETE(p_n_trk);
						THROW_MEM(p_n_trk = new SXml::WNode(writer, "trk"));
						{
							GetObjectTitle(oid.Obj, out_buf = 0);
							GetObjectName(oid.Obj, oid.Id, temp_buf);
							out_buf.Space().CatBrackStr(temp_buf).Transf(CTRANSF_INNER_TO_UTF8);
							p_n_trk->PutInner("name", out_buf);
						}
						ZDELETE(p_n_trkseg);
						THROW_MEM(p_n_trkseg = new SXml::WNode(writer, "trkseg"));
					}
					assert(p_n_trkseg != 0);
                    {
                    	SXml::WNode n_trkpt(writer, "trkpt");
							n_trkpt.PutAttrib("lat", (out_buf = 0).Cat(item.Latitude, MKSFMTD(0, 20, NMBF_NOTRAILZ)));
							n_trkpt.PutAttrib("lon", (out_buf = 0).Cat(item.Longitude, MKSFMTD(0, 20, NMBF_NOTRAILZ)));
							n_trkpt.PutInner("ele", (out_buf = 0).Cat((double)item.Altitude, MKSFMTD(0, 20, NMBF_NOTRAILZ)));
							(out_buf = 0).Cat(dtm, DATF_ISO8601|DATF_CENTURY, 0);
							n_trkpt.PutInner("time", out_buf);
                    }
					last_oid = oid;
				}
				ZDELETE(p_n_trkseg);
				ZDELETE(p_n_trk);
			}
			xmlTextWriterEndDocument(writer);
		}
	}
	CATCHZOKPPERR
	xmlFreeTextWriter(writer);
	return ok;
}

//virtual
int SLAPI PPViewGeoTracking::ProcessCommand(uint ppvCmd, const void * pHdr, PPViewBrowser * pBrw)
{
	int    ok = PPView::ProcessCommand(ppvCmd, pHdr, pBrw);
	int    update = 0;
	if(ok == -2) {
		switch(ppvCmd) {
			case PPVCMD_EXPORT:
				ok = -1;
				Export();
				break;
		}
	}
	if(ok > 0 && update > 0) {
		CALLPTRMEMB(pBrw, Update());
	}
	return (update > 0) ? ok : ((ok <= 0) ? ok : -1);
}

