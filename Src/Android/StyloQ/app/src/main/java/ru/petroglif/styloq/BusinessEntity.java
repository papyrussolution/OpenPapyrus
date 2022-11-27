// BusinessEntity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;
import org.json.JSONArray;
import org.json.JSONException;
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
			ChZnCat = 0; // @v11.5.0
			Egais = false; // @v11.5.0
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
					ChZnCat = jsObj.optInt("chzncat", 0); // @v11.5.0
					Egais = jsObj.optBoolean("egais", false); // @v11.5.0
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
					ManufDtm = SLib.strtodatetime(jsObj.optString("manuftm", null), SLib.DATF_ISO8601, SLib.TIMF_HMS);
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
		int    ChZnCat;    // @v11.5.0 GTCHZNPT_XXX Категория маркировки честный знак
		boolean Egais;     // @v11.5.0 Если true, то товар маркируется EGAI-кодами
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
	public static class PreprocessBarcodeResult {
		String FinalCode;
		String ChZnMark;
		double Qtty;
	}
	public static PreprocessBarcodeResult PreprocessBarcode(String code, String wghtPrefix, String cntPrefix)
	{
		PreprocessBarcodeResult result = null;
		final int code_len = SLib.GetLen(code);
		if(code_len > 1) {
			GTIN gtin_chzn = GTIN.ParseChZnCode(code, 0);
			if(gtin_chzn != null && gtin_chzn.GetChZnParseResult() > 0) {
				String gtin14 = gtin_chzn.GetToken(GTIN.fldGTIN14);
				if(SLib.GetLen(gtin14) == 14) {
					if(result == null)
						result = new PreprocessBarcodeResult();
					result.ChZnMark = GTIN.PreprocessChZnCode(code);
					result.FinalCode = gtin14.substring(1);
					result.Qtty = 1.0;
				}
			}
			else if(SLib.IsDecimal(code)) {
				if(result == null)
					result = new PreprocessBarcodeResult();
				if(code_len == 13 || code_len == 12) {
					final int wght_pfx_len = SLib.GetLen(wghtPrefix);
					final int cnt_pfx_len = SLib.GetLen(cntPrefix);
					if(wght_pfx_len == 2 && code.startsWith(wghtPrefix)) {
						result.FinalCode = code.substring(0, 7);
						String wght_text = code.substring(7, 12);
						result.Qtty = Double.parseDouble(wght_text) / 1000.0;
					}
					else if(cnt_pfx_len == 2 && code.startsWith(cntPrefix)) {
						result.FinalCode = code.substring(0, 7);
						String count_text = code.substring(7, 12);
						result.Qtty = Double.parseDouble(count_text);
					}
					else {
						result.FinalCode = code;
						result.Qtty = 1.0;
					}
				}
				else {
					result.FinalCode = code;
					result.Qtty = 1.0;
				}
			}
		}
		return result;
	}
	//
	// Descr: Специализированная структура, проецирующая статусы документов сервиса на клиентские варинаты.
	//   Является отображением структуры Papyrus PPStyloQInterchange::Document::CliStatus
	//
	public static class CliDocStatus {
		public CliDocStatus()
		{
			SurrogateId = 0;
			StatusID = 0;
			ReservedCase = rUndef;
			Flags = 0;
			Name = null;
		}
		boolean IsValuable()
		{
			return ((StatusID != 0 || (Flags & fBillFlagDeclined) != 0) && SLib.GetLen(Name) > 0);
		}
		public static ArrayList <CliDocStatus> FromJson(JSONArray jsArr)
		{
			ArrayList <CliDocStatus> result = null;
			if(jsArr != null && jsArr.length() > 0) {
				for(int i = 0; i < jsArr.length(); i++) {
					CliDocStatus new_entry = new CliDocStatus();
					if(new_entry.FromJson(jsArr.optJSONObject(i))) {
						if(result == null)
							result = new ArrayList<CliDocStatus>();
						result.add(new_entry);
					}
				}
			}
			return result;
		}
		public boolean FromJson(JSONObject jsObj)
		{
			boolean ok = false;
			if(jsObj != null) {
				SurrogateId = jsObj.optInt("surrid", 0);
				StatusID = jsObj.optInt("statusid", 0);
				ReservedCase = jsObj.optInt("rsrvdcase", 0);
				Flags = jsObj.optInt("flags", 0);
				Name = jsObj.optString("nm", null);
				if(IsValuable())
					ok = true;
			}
			return ok;
		}
		//
		// Descr: Зарезервированные варианты статусов, проецируемые на клиента
		//
		public static final int rUndef    = 0;
		public static final int rAccepted = 1;
		public static final int rRejected = 2;

		public static final int fBillFlagDeclined = 0x0001; // Вместо (или одновременно) указания статуса, документу присваивается флаг BILLF2_DECLINED
		int    SurrogateId;  // Суррогатный идентификатор элемента в списке, передаваемом клиенту. Если на клиенте устанавливается
			// соответствующее значение, то сервису передается информация для смены статуса.
		int    StatusID;
		int    ReservedCase; // DocStatus::rXXX
		int    Flags;
		String Name;
	}
	//
	// Descr: Структура, представляющая элемент списка долговых документов.
	//   Используется для предоставления справки о долгах контрагента, в частности, в агентских заказах.
	//
	public static class DebtEntry {
		DebtEntry()
		{
			BillID = 0;
			BillDate = null;
			BillCode = null;
			AgentID = 0;
			Amount = 0.0;
			Debt = 0.0;
		}
		public static DebtEntry Copy(final DebtEntry s)
		{
			DebtEntry result = null;
			if(s != null) {
				result = new DebtEntry();
				result.BillID = s.BillID;
				result.BillDate = SLib.LDATE.Copy(s.BillDate);
				result.BillCode = SLib.Copy(s.BillCode);
				result.AgentID = s.AgentID;
				result.Amount = s.Amount;
				result.Debt = s.Debt;
			}
			return result;
		}
		public boolean FromJson(JSONObject jsObj)
		{
			boolean ok = false;
			if(jsObj != null) {
				BillID = jsObj.optInt("billid", 0);
				{
					String date_text = jsObj.optString("billdate", null);
					if(SLib.GetLen(date_text) > 0)
						BillDate = SLib.strtodate(date_text, SLib.DATF_ISO8601);
					else
						BillDate = null;
				}
				BillCode = jsObj.optString("billcode", null);
				AgentID = jsObj.optInt("agentid", 0);
				Amount = jsObj.optDouble("amt", 0.0);
				Debt = jsObj.optDouble("debt", 0.0);
				ok = true;
			}
			return ok;
		}
		public JSONObject ToJsonObj()
		{
			JSONObject result = null;
			try {
				if(BillID > 0) {
					result = new JSONObject();
					result.put("billid", BillID);
					if(BillDate != null) {
						String date_text = BillDate.Format(SLib.DATF_ISO8601|SLib.DATF_CENTURY);
						if(SLib.GetLen(date_text) > 0)
							result.put("billdate", date_text);
					}
					if(SLib.GetLen(BillCode) > 0) {
						result.put("billcode", BillCode);
					}
					if(AgentID > 0)
						result.put("agentid", AgentID);
					result.put("amt", Amount);
					result.put("debt", Debt);
				}
			} catch(JSONException exn) {
				result = null;
			}
			return result;
		}
		int    BillID;
		SLib.LDATE  BillDate;
		String BillCode;
		int    AgentID;
		double Amount;
		double Debt;
	}
	//
	// Descr: Долговая выписка по одному контрагенту.
	//
	public static class ArDebtList {
		ArDebtList()
		{
			ArID = 0;
			ArName = null;
			SvcTime = null;
			CliTime = null;
			Debt = 0.0;
			List = null;
		}
		public static ArDebtList Copy(final ArDebtList s)
		{
			ArDebtList result = null;
			if(s != null) {
				result = new ArDebtList();
				result.ArID = s.ArID;
				result.ArName = SLib.Copy(s.ArName);
				result.SvcTime = SLib.LDATETIME.Copy(s.SvcTime);
				result.CliTime = SLib.LDATETIME.Copy(s.CliTime);
				result.Debt = s.Debt;
				if(s.List != null) {
					result.List = new ArrayList<DebtEntry>();
					for(DebtEntry iter : s.List)
						result.List.add(DebtEntry.Copy(iter));
				}
			}
			return result;
		}
		public boolean FromJson(JSONObject jsObj)
		{
			boolean ok = false;
			int    items_count = 0;
			if(jsObj != null) {
				SvcTime = SLib.strtodatetime(jsObj.optString("time", null), SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0);
				{
					CliTime = SLib.strtodatetime(jsObj.optString("clitime", null), SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0);
					if(CliTime == null)
						CliTime = new SLib.LDATETIME(System.currentTimeMillis());
				}
				ArID = jsObj.optInt("arid", 0);
				ArName = jsObj.optString("arname", null);
				items_count = jsObj.optInt("count", 0);
				Debt = jsObj.optDouble("debt", 0.0);
				JSONArray js_list = jsObj.optJSONArray("debt_list");
				if(js_list != null && js_list.length() > 0) {
					List = new ArrayList<DebtEntry>();
					for(int i = 0; i < js_list.length(); i++) {
						final JSONObject js_item = js_list.optJSONObject(i);
						if(js_item != null) {
							DebtEntry entry = new DebtEntry();
							if(entry.FromJson(js_item))
								List.add(entry);
						}
					}
				}
				else
					List = null;
				ok = true;
			}
			return ok;
		}
		public JSONObject ToJsonObj()
		{
			JSONObject result = null;
			final int items_count = SLib.GetCount(List);
			int real_items_count = 0;
			try {
				result = new JSONObject();
				result.put("arid", ArID);
				if(SLib.GetLen(ArName) > 0)
					result.put("arname", ArName);
				if(SvcTime != null) {
					String dtm_text = SLib.datetimefmt(SvcTime, SLib.DATF_ISO8601 | SLib.DATF_CENTURY, 0);
					if(SLib.GetLen(dtm_text) > 0)
						result.put("time", dtm_text);
				}
				if(CliTime != null) {
					String dtm_text = SLib.datetimefmt(CliTime, SLib.DATF_ISO8601 | SLib.DATF_CENTURY, 0);
					if(SLib.GetLen(dtm_text) > 0)
						result.put("clitime", dtm_text);
				}
				result.put("count", items_count);
				result.put("debt", Debt);
				JSONArray js_list = new JSONArray();
				for(int i = 0; i < items_count; i++) {
					final DebtEntry entry = List.get(i);
					if(entry != null) {
						JSONObject js_entry = entry.ToJsonObj();
						if(js_entry != null) {
							js_list.put(js_entry);
							real_items_count++;
						}
					}
				}
				result.put("debt_list", js_list);
			} catch(JSONException exn) {
				result = null;
			}
			return result;
		}
		int    ArID;
		String ArName;
		// Предусматриваем время сервиса и время клиента из-за того, что часы на них могут быть
		// рассинхронизированы.
		SLib.LDATETIME SvcTime; // Время формирования выписки сервисом
		SLib.LDATETIME CliTime; // Время получения выписки клиентом
		double Debt;   // Итоговых долг по контрагенту
		ArrayList <DebtEntry> List;
	}
	//
	// Descr: Общий список долговых выписок по контрагентам.
	//   Сохраняется в реестре StyloQ с типом документа StyloQDatabase.doctypDebtList
	//
	public static class DebtList {
		public static class ShortReplyEntry {
			ShortReplyEntry()
			{
				ArID = 0;
				Debt = 0.0;
				Timestamp = null;
			}
			public boolean IsExpired(final StyloQCommand.Item cmdQueryDebt)
			{
				boolean result = false;
				final int expiry_time_sec = (cmdQueryDebt != null && cmdQueryDebt.BaseCmdId == StyloQCommand.sqbcDebtList && cmdQueryDebt.ResultExpiryTimeSec > 0) ?
					cmdQueryDebt.ResultExpiryTimeSec : (3600 * 24);
				if(Timestamp != null) {
					SLib.LDATETIME expiry_time = SLib.plusdatetimesec(Timestamp, expiry_time_sec);
					if(expiry_time != null && SLib.Cmp(expiry_time, new SLib.LDATETIME(System.currentTimeMillis())) < 0)
						result = true;
				}
				return result;
			}
			int    ArID;
			double Debt;
			SLib.LDATETIME Timestamp;
		}
		public static DebtList Copy(final DebtList s)
		{
			DebtList result = null;
			if(s != null) {
				result = new DebtList();
				if(s.List != null) {
					result.List = new ArrayList<ArDebtList>();
					for(ArDebtList iter : s.List)
						result.List.add(ArDebtList.Copy(iter));
				}
			}
			return result;
		}
		DebtList()
		{
			List = null;
		}
		ShortReplyEntry GetDebt(int arID)
		{
			ShortReplyEntry result = null;
			if(arID > 0) {
				final int _c = SLib.GetCount(List);
				for(int i = 0; result == null && i < _c; i++) {
					final ArDebtList entry = List.get(i);
					if(entry != null && entry.ArID == arID) {
						result = new ShortReplyEntry();
						result.ArID = entry.ArID;
						result.Debt = entry.Debt;
						result.Timestamp = (entry.CliTime != null) ? entry.CliTime : entry.SvcTime;
					}
				}
			}
			return result;
		}
		ArDebtList GetDebtDetail(int arID)
		{
			ArDebtList result = null;
			if(arID > 0) {
				final int _c = SLib.GetCount(List);
				for(int i = 0; result == null && i < _c; i++) {
					ArDebtList entry = List.get(i);
					if(entry != null && entry.ArID == arID)
						result = entry;
				}
			}
			return result;
		}
		int Include(ArDebtList newEntry)
		{
			int    result_idx = -1;
			if(newEntry != null && newEntry.ArID > 0) {
				if(List != null) {
					int i = List.size();
					if(i > 0) do {
						i--;
						ArDebtList entry = List.get(i);
						if(entry == null || entry.ArID <= 0 || entry.ArID == newEntry.ArID) {
							List.remove(i);
						}
					} while(i > 0);
				}
				else
					List = new ArrayList<ArDebtList>();
				if(List.add(newEntry)) {
					assert(List.size() > 0);
					result_idx = List.size()-1;
				}
			}
			return result_idx;
		}
		boolean FromJson(JSONObject jsObj)
		{
			boolean ok = false;
			List = null;
			if(jsObj != null) {
				try {
					JSONArray js_list = jsObj.optJSONArray("list");
					if(js_list != null) {
						for(int i = 0; i < js_list.length(); i++) {
							ArDebtList entry = new ArDebtList();
							if(entry.FromJson(js_list.getJSONObject(i))) {
								if(List == null)
									List = new ArrayList<ArDebtList>();
								List.add(entry);
								ok = true;
							}
						}
					}
				} catch(JSONException exn) {
					ok = false;
				}
			}
			return ok;
		}
		JSONObject ToJsonObj()
		{
			JSONObject result = null;
			try {
				result = new JSONObject();
				JSONArray js_list = new JSONArray();
				final int _c = SLib.GetCount(List);
				for(int i = 0; i < _c; i++) {
					final ArDebtList entry = List.get(i);
					if(entry != null) {
						JSONObject js_entry = entry.ToJsonObj();
						if(js_entry != null)
							js_list.put(js_entry);
					}
				}
				result.put("list", js_list);
			} catch(JSONException exn) {
				result = null;
			}
			return result;
		}
		ArrayList <ArDebtList> List;
	}
}
