// BusinessEntity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.ArrayList;

public class BusinessEntity {
	public static class Uom {
		public Uom()
		{
			ID = 0;
			Name = null;
			Symb = null;
			Flags = 0;
			Fragmentation = 0;
			Rounding = 0.0;
			BaseUnitID = 0;
			BaseRatio = 0.0;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				ID = jsObj.optInt("id", 0);
				if(ID > 0) {
					Name = jsObj.optString("nm", null);
					if(jsObj.optBoolean("int")) {
						Flags |= fIntVal;
					}
					Fragmentation = jsObj.optInt("fragmentation", 0);
					Rounding = jsObj.optDouble("rounding", 0.0);
					result = true;
				}
			}
			return result;
		}
		//
		// Descr: Флаги единиц измерения. Идентичны флагам в Papyrus PPUnit2::XXX (с дополнительным префиксом f)
		//
		public static final int fSI       = 0x0001; // (S) Единица системы СИ
		public static final int fPhysical = 0x0002; // (P) Физическая единица
		public static final int fTrade    = 0x0004; // (T) Торговая единица (может не иметь однозначного физ. эквивалента)
		public static final int fHide     = 0x0008; // (H) Единицу не следует показывать
		public static final int fIntVal   = 0x0010; // (I) Единица может быть только целочисленной
		public static final int fCommon   = 0x0020; // Унифицированная единица измерения (имеет конкретные габариты и, возможно, массу и емкость) //
		public static final int fDefault  = 0x0040; // @transient Флаг только для передачи данных, информирующий о том, что единица измерения
			// применяется "по умолчанию" для товаров, у которых единица измерения не определена.

		int   ID;
		String Name;
		String Symb;
		int    Flags;
		int    Fragmentation;
		double Rounding;
		int    BaseUnitID;
		double BaseRatio;
	}
	public static class Brand {
		Brand()
		{
			ID = 0;
			Name = null;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				ID = jsObj.optInt("id", 0);
				if(ID > 0) {
					Name = jsObj.optString("nm", null);
					result = true;
				}
			}
			return result;
		}
		int    ID;
		String Name;
	}
	public static class QuotKind {
		QuotKind()
		{
			ID = 0;
			Name = null;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				ID = jsObj.optInt("id", 0);
				if(ID > 0) {
					Name = jsObj.optString("nm", null);
					result = true;
				}
			}
			return result;
		}
		int    ID;
		String Name;
	}
	public static class GoodsCode {
		GoodsCode()
		{
			Code = null;
			Qtty = 0.0;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				Code = jsObj.optString("cod", null);
				if(SLib.GetLen(Code) > 0) {
					Qtty = jsObj.optDouble("qty", 0.0);
					result = true;
				}
			}
			return result;
		}
		String Code;
		double Qtty;
	}
	public static class Quot {
		Quot()
		{
			ID = 0;
			Val = 0.0;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				ID = jsObj.optInt("id", 0);
				if(ID > 0) {
					Val = jsObj.optDouble("val", 0.0);
					result = true;
				}
			}
			return result;
		}
		int    ID;
		double Val;
	}
	public static class Goods {
		public static class ExtText {
			ExtText()
			{
				Title = null;
				Text = null;
			}
			public boolean FromJsonObj(JSONObject jsObj)
			{
				boolean result = false;
				if(jsObj != null) {
					Title = jsObj.optString("title", null);
					Text = jsObj.optString("text", null);
					if(SLib.GetLen(Text) > 0)
						result = true;
				}
				return result;
			}
			String Title;
			String Text;
		}
		public Goods()
		{
			ID = 0;
			ParentID = 0;
			UomID = 0;
			BrandID = 0;
			ManufID = 0;
			UnitPerPack = 0.0;
			OrdQtyMult = 0.0;
			OrdMinQty = 0.0;
			Stock = 0.0;
			Cost = 0.0;
			Price = 0.0;
			Name = null;
			ParentName = null;
			BrandName = null;
			ManufName = null;
			ManufDtm = null;
			CodeList = null;
			QuotList = null;
			ExtTextList = null;
			ImgBlob = null;
		}
		public boolean FromJsonObj(JSONObject jsObj)
		{
			boolean result = false;
			if(jsObj != null) {
				ID = jsObj.optInt("id", 0);
				if(ID > 0) {
					ParentID = jsObj.optInt("parid", 0);
					UomID = jsObj.optInt("uomid", 0);
					BrandID = jsObj.optInt("brandid", 0);
					ManufID = jsObj.optInt("manufid", 0);
					UnitPerPack = jsObj.optDouble("upp", 0.0);
					OrdMinQty = jsObj.optDouble("ordminqty", 0.0);
					OrdQtyMult = jsObj.optDouble("ordqtymult", 0.0);
					Stock = jsObj.optDouble("stock", 0.0);
					Cost = jsObj.optDouble("cost", 0.0);
					Price = jsObj.optDouble("price", 0.0);
					Name = jsObj.optString("nm", null);
					ParentName = jsObj.optString("parnm", null);
					BrandName = jsObj.optString("brandnm", null);
					ManufName = jsObj.optString("manufnm", null);
					{
						String manuf_dtm_text = jsObj.optString("manuftm", null);
						if(SLib.GetLen(manuf_dtm_text) > 0)
							ManufDtm = SLib.strtodatetime(manuf_dtm_text, SLib.DATF_ISO8601, SLib.TIMF_HMS);
						else
							ManufDtm = null;
					}
					ImgBlob = jsObj.optString("imgblobs", null);
					{
						JSONArray js_list = jsObj.optJSONArray("code_list");
						if(js_list != null && js_list.length() > 0) {
							for(int i = 0; i < js_list.length(); i++) {
								JSONObject js_item = js_list.optJSONObject(i);
								if(js_item != null) {
									GoodsCode item = new GoodsCode();
									if(item.FromJsonObj(js_item)) {
										if(CodeList == null)
											CodeList = new ArrayList<GoodsCode>();
										CodeList.add(item);
									}
								}
							}
						}
					}
					{
						JSONArray js_list = jsObj.optJSONArray("quot_list");
						if(js_list != null && js_list.length() > 0) {
							for(int i = 0; i < js_list.length(); i++) {
								JSONObject js_item = js_list.optJSONObject(i);
								if(js_item != null) {
									Quot item = new Quot();
									if(item.FromJsonObj(js_item)) {
										if(QuotList == null)
											QuotList = new ArrayList<Quot>();
										QuotList.add(item);
									}
								}
							}
						}
					}
					{
						JSONArray js_list = jsObj.optJSONArray("exttext_list");
						if(js_list != null && js_list.length() > 0) {
							for(int i = 0; i < js_list.length(); i++) {
								JSONObject js_item = js_list.optJSONObject(i);
								if(js_item != null) {
									ExtText item = new ExtText();
									if(item.FromJsonObj(js_item)) {
										if(ExtTextList == null)
											ExtTextList = new ArrayList<ExtText>();
										ExtTextList.add(item);
									}
								}
							}
						}
					}
					result = true;
				}
			}
			return result;
		}
		public GoodsCode SearchCode(final String code)
		{
			GoodsCode result = null;
			if(SLib.GetLen(code) > 0 && CodeList != null && CodeList.size() > 0) {
				for(int i = 0; result == null && i < CodeList.size(); i++) {
					final GoodsCode item = CodeList.get(i);
					if(item != null && SLib.GetLen(item.Code) > 0) {
						if(SLib.AreStringsEqual(item.Code, code))
							result = item;
					}
				}
			}
			return result;
		}
		int    ID;
		int    ParentID;
		int    UomID;
		int    BrandID;
		int    ManufID;
		double UnitPerPack;
		double OrdQtyMult; // Кратность заказа (<=0 - не определено)
		double OrdMinQty;  // Минимальный заказ (<=0 - не определено)
		double Stock; // Контекстно-зависимая величина. Остаток на складе.
		double Cost;  // Контекстно-зависимая величина. Цена поступления.
		double Price; // Контекстно-зависимая величина. Цена реализации.
		String Name;
		String ParentName; // Используется в том случае, если нет отдельного списка родительских групп
		String BrandName; // Используется в том случае, если нет отдельного списка брендов
		String ManufName;
		SLib.LDATETIME ManufDtm;
		ArrayList <GoodsCode> CodeList;
		ArrayList <Quot> QuotList;
		ArrayList <ExtText> ExtTextList;
		String ImgBlob;
	}
}
