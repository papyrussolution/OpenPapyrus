// Document.java 
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Locale;
import java.util.UUID;

public class Document {
	public static class Head {
		long   ID;
		long   CreationTime;
		long   Time;
		long   DueTime;
		int    OpID;
		int    ClientID;  // service-domain-id
		int    DlvrLocID; // service-domain-id
		double Amount;    // Номинальная сумма документа
		String Code;
		byte [] SvcIdent;
		UUID Uuid; // Уникальный идентификатор, генерируемый на стороне эмитента
		String Memo;
	}
	public static class LotExtCode {
		int    Flags;
		int    BoxRefN;
		String Code;
	}
	// Так как одна строка может иметь более одного набора значений {qtty, cost, price},
	// то выделяем такой набор в отдельную структуру.
	public static class ValuSet {
		public ValuSet()
		{
			Qtty = 0.0;
			Cost = 0.0;
			Price = 0.0;
			Discount = 0.0;
		}
		public double GetAmount_Cost() { return (Qtty * Cost); }
		public double GetAmount_Price() { return (Qtty * (Price-Discount)); }
		public final boolean IsEmpty()
		{
			return !(Qtty > 0.0 || Cost > 0.0 || Price > 0.0);
		}
		double Qtty;
		double Cost;
		double Price;
		double Discount;
	}
	public static class TransferItem {
		public boolean CanMerge(final TransferItem testItem)
		{
			boolean result = false;
			if(testItem != null && testItem.GoodsID == GoodsID) {
				result = true;
			}
			return result;
		}
		public boolean Merge(final TransferItem testItem)
		{
			boolean result = false;
			if(CanMerge(testItem)) {
				if(testItem.Set != null && testItem.Set.Qtty != 0) {
					if(Set == null)
						Set = new ValuSet();
					double org_qtty = Set.Qtty;
					Set.Qtty += testItem.Set.Qtty;
					if(Set.Cost > 0) {
						if(testItem.Set.Cost > 0)
							Set.Cost = (Set.Cost * Math.abs(org_qtty) + testItem.Set.Cost * Math.abs(testItem.Set.Qtty)) / Math.abs(Set.Qtty);
					}
					else if(testItem.Set.Cost > 0)
						Set.Cost = testItem.Set.Cost;
					if(Set.Price > 0) {
						if(testItem.Set.Price > 0)
							Set.Price = (Set.Price * Math.abs(org_qtty) + testItem.Set.Price * Math.abs(testItem.Set.Qtty)) / Math.abs(Set.Qtty);
					}
					else if(testItem.Set.Price > 0)
						Set.Price = testItem.Set.Price;
				}
				result = true;
			}
			return result;
		}
		TransferItem()
		{
			RowIdx = 0;
			GoodsID = 0;
			UnitID = 0;
			Flags = 0;
			Set = new ValuSet();
			XcL = null;
		}
		double GetAmount_Cost() { return (Set != null) ? Set.GetAmount_Cost() : 0.0; }
		double GetAmount_Price() { return (Set != null) ? Set.GetAmount_Price() : 0.0; }
		int    RowIdx;  // [1..]
		int    GoodsID; // service-domain-id
		int    UnitID;  // service-domain-id
		int    Flags;
		ValuSet Set;
		ArrayList <LotExtCode> XcL;
	}
	public static class BookingItem {
		BookingItem()
		{
			RowIdx = 0;
			PrcID = 0;
			GoodsID = 0;
			Flags = 0;
			ReqTime = null;
			EstimatedDurationSec = 0;
			Set = new ValuSet();
			Memo = null;
		}
		int    RowIdx; // [1..]
		int    PrcID;
		int    GoodsID;
		int    Flags;
		SLib.LDATETIME ReqTime;
		int    EstimatedDurationSec;
		ValuSet Set;
		String Memo;
	}
	Document()
	{
		H = null;
		TiList = null;
		BkList = null;
		VXcL = null;
	}
	Document(int opID, final byte [] svcIdent, StyloQApp appCtx) throws StyloQException
	{
		TiList = null;
		BkList = null;
		VXcL = null;
		if(opID > 0) {
			H = new Document.Head();
			H.CreationTime = System.currentTimeMillis();
			H.OpID = opID;
			H.Uuid = UUID.randomUUID();
			if(SLib.GetLen(svcIdent) > 0 && appCtx != null) {
				StyloQDatabase db = appCtx.GetDB();
				if(db != null)
					H.Code = db.MakeDocumentCode(svcIdent);
			}
		}
	}
	Document Z()
	{
		H = null;
		TiList = null;
		BkList = null;
		VXcL = null;
		return this;
	}
	double GetNominalAmount()
	{
		return (H != null) ? H.Amount : 0.0;
	}
	double CalcNominalAmount()
	{
		double amount = 0.0;
		if(TiList != null) {
			for(int i = 0; i < TiList.size(); i++) {
				TransferItem ti = TiList.get(i);
				if(ti != null)
					amount += ti.GetAmount_Price();
			}
		}
		return amount;
	}
	//
	// Descr: Выполняет завершающие операции над документом, включающие
	//   внутрениие расчеты и проверку инвариантов.
	//
	boolean Finalize()
	{
		boolean ok = true;
		if(H != null) {
			H.Amount = CalcNominalAmount();
		}
		return ok;
	}
	JSONObject ToJsonObj()
	{
		JSONObject result = new JSONObject();
		try {
			if(H != null) {
				result.put("id", H.ID);
				if(H.Uuid != null) {
					result.put("uuid", H.Uuid.toString());
				}
				if(SLib.GetLen(H.Code) > 0)
					result.put("code", H.Code);
				if(SLib.GetLen(H.SvcIdent) > 0) {
					String svc_ident_hex = Base64.getEncoder().encodeToString(H.SvcIdent);
					if(SLib.GetLen(svc_ident_hex) > 0)
						result.put("svcident", svc_ident_hex);
				}
				if(H.CreationTime > 0) {
					SLib.LDATETIME dtm = new SLib.LDATETIME(H.CreationTime);
					result.put("crtm", SLib.datetimefmt(dtm, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				}
				if(H.Time > 0) {
					SLib.LDATETIME dtm = new SLib.LDATETIME(H.Time);
					result.put("tm", SLib.datetimefmt(dtm, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				}
				if(H.DueTime > 0) {
					SLib.LDATETIME dtm = new SLib.LDATETIME(H.DueTime);
					result.put("duetm", SLib.datetimefmt(dtm, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				}
				result.put("opid", H.OpID);
				if(H.ClientID > 0) {
					result.put("cliid", H.ClientID);
				}
				if(H.DlvrLocID > 0) {
					result.put("dlvrlocid", H.DlvrLocID);
				}
				if(SLib.GetLen(H.Memo) > 0)
					result.put("memo", H.Memo);
				if(TiList != null && TiList.size() > 0) {
					JSONArray js_list = new JSONArray();
					boolean is_list_empty = true;
					for(int i = 0; i < TiList.size(); i++) {
						TransferItem ti = TiList.get(i);
						if(ti != null) {
							JSONObject js_item = new JSONObject();
							js_item.put("rowidx", ti.RowIdx);
							if(ti.GoodsID > 0) {
								js_item.put("goodsid", ti.GoodsID);
								if(ti.UnitID > 0)
									js_item.put("unitid", ti.UnitID);
								js_item.put("flags", ti.Flags);
								if(ti.Set != null) {
									JSONObject js_ti_set = new JSONObject();
									boolean is_empty = true;
									if(ti.Set.Qtty != 0.0) {
										js_ti_set.put("qtty", ti.Set.Qtty);
										is_empty = false;
									}
									if(ti.Set.Cost != 0.0) {
										js_ti_set.put("cost", ti.Set.Cost);
										is_empty = false;
									}
									if(ti.Set.Price != 0.0) {
										js_ti_set.put("price", ti.Set.Price);
										is_empty = false;
									}
									if(ti.Set.Discount != 0.0) {
										js_ti_set.put("discount", ti.Set.Discount);
										is_empty = false;
									}
									if(is_empty)
										js_ti_set = null;
									else
										js_item.put("set", js_ti_set);
								}
								js_list.put(js_item);
								is_list_empty = false;
							}
						}
					}
					if(is_list_empty)
						js_list = null;
					else
						result.put("ti_list", js_list);
				}
				if(BkList != null && BkList.size() > 0) {
					JSONArray js_list = new JSONArray();
					for(int i = 0; i < BkList.size(); i++) {
						BookingItem bi = BkList.get(i);
						if(bi != null) {
							JSONObject js_item = new JSONObject();
							js_item.put("rowidx", bi.RowIdx);
							js_item.put("prcid", bi.PrcID);
							js_item.put("goodsid", bi.GoodsID);
							js_item.put("flags", bi.Flags);
							if(bi.ReqTime != null) {
								js_item.put("reqtime", SLib.datetimefmt(bi.ReqTime, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
							}
							if(bi.EstimatedDurationSec > 0) {
								js_item.put("estimateddurationsec", bi.EstimatedDurationSec);
							}
							if(bi.Set != null && !bi.Set.IsEmpty()) {
								JSONObject js_set = new JSONObject();
								if(bi.Set.Qtty > 0.0)
									js_set.put("qtty", bi.Set.Qtty);
								if(bi.Set.Cost > 0.0)
									js_set.put("cost", bi.Set.Cost);
								if(bi.Set.Price > 0.0)
									js_set.put("price", bi.Set.Price);
								js_item.put("set", js_set);
							}
							if(SLib.GetLen(bi.Memo) > 0)
								js_item.put("set", bi.Memo);
							js_list.put(js_item);
						}
					}
					result.put("bk_list", js_list);
				}
			}
		} catch(JSONException exn) {
			//exn.printStackTrace();
			result = null;
		}
		return result;
	}
	String ToJson()
	{
		JSONObject jsobj = ToJsonObj();
		return (jsobj != null) ? jsobj.toString() : null;
	}
	boolean FromJson(String jsText)
	{
		boolean result = false;
		Z();
		if(SLib.GetLen(jsText) > 0) {
			try {
				JSONObject jsobj = new JSONObject(jsText);
				H = new Head();
				H.ID = jsobj.optLong("id", 0);
				SimpleDateFormat sdf = new SimpleDateFormat("yyyy-mm-dd'T'HH:mm:ss", Locale.getDefault());
				{
					String ts = jsobj.optString("crtm", "");
					if(SLib.GetLen(ts) > 0) {
						try {
							java.util.Date jd = sdf.parse(ts);
							H.CreationTime = jd.getTime();
						} catch(ParseException exn) {}
					}
				}
				{
					String ts = jsobj.optString("tm", "");
					if(SLib.GetLen(ts) > 0) {
						try {
							java.util.Date jd = sdf.parse(ts);
							H.Time = jd.getTime();
						} catch(ParseException exn) {}
					}
				}
				{
					String ts = jsobj.optString("duetm", "");
					if(SLib.GetLen(ts) > 0) {
						try {
							java.util.Date jd = sdf.parse(ts);
							H.DueTime = jd.getTime();
						} catch(ParseException exn) {}
					}
				}
				H.OpID = jsobj.optInt("opid", 0);
				H.ClientID = jsobj.optInt("cliid", 0);
				H.DlvrLocID = jsobj.optInt("dlvrlocid", 0);
				String svc_ident_hex = jsobj.optString("svcident", null);
				if(SLib.GetLen(svc_ident_hex) > 0) {
					H.SvcIdent = Base64.getDecoder().decode(svc_ident_hex);
				}
				H.Code = jsobj.optString("code", null);
				H.Memo = jsobj.optString("memo", null);
				JSONArray js_ti_list = jsobj.optJSONArray("ti_list");
				if(js_ti_list != null && js_ti_list.length() > 0) {
					for(int i = 0; i < js_ti_list.length(); i++) {
						JSONObject js_item = js_ti_list.getJSONObject(i);
						if(js_item != null) {
							TransferItem ti = new TransferItem();
							ti.RowIdx = js_item.optInt("rowidx", 0);
							ti.GoodsID = js_item.optInt("goodsid", 0);
							ti.UnitID = js_item.optInt("unitid", 0);
							ti.Flags = js_item.optInt("flags", 0);
							JSONObject js_set = js_item.optJSONObject("set");
							if(js_set != null) {
								ti.Set = new ValuSet();
								ti.Set.Qtty = js_set.optDouble("qtty", 0.0);
								ti.Set.Cost = js_set.optDouble("cost", 0.0);
								ti.Set.Price = js_set.optDouble("price", 0.0);
								ti.Set.Discount = js_set.optDouble("discount", 0.0);
							}
							if(TiList == null)
								TiList = new ArrayList<TransferItem>();
							TiList.add(ti);
						}
					}
				}
				JSONArray js_bk_list = jsobj.optJSONArray("bk_list");
				if(js_bk_list != null && js_bk_list.length() > 0) {
					for(int i = 0; i < js_bk_list.length(); i++) {
						JSONObject js_item = js_ti_list.getJSONObject(i);
						if(js_item != null) {
							BookingItem bi = new BookingItem();
							bi.RowIdx = js_item.optInt("rowid", 0);
							bi.PrcID = js_item.optInt("prcid", 0);
							bi.GoodsID = js_item.optInt("goodsid", 0);
							bi.Flags = js_item.optInt("flags", 0);
							// @todo Не все поля считаны!
							if(BkList == null)
								BkList = new ArrayList<BookingItem>();
							BkList.add(bi);
						}
					}
				}
				result = true;
			} catch(JSONException exn) {
				result = false;
				//exn.printStackTrace();
			}
		}
		return result;
	}
	Head H;
	ArrayList <TransferItem> TiList;
	ArrayList <BookingItem> BkList; // Список позиций повременных элементов, связанных с процессорами
	ArrayList <LotExtCode> VXcL; // Валидирующий контейнер спецкодов. Применяется для проверки кодов, поступивших с документом в XcL
}
