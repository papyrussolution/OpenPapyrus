package ru.petroglif.styloq;

import org.json.JSONObject;

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
}
