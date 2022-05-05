// StyloQFace.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import org.checkerframework.checker.nullness.qual.NonNull;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class StyloQFace {
	// @persistant {
	// Теги лика
	public static final int tagUnkn           =  0; //
	public static final int tagVerifiable_Depricated =  1; // verifiable : bool ("true" || "false")
	public static final int tagCommonName     =  2; // cn : string with optional language shifted on 16 bits left
	public static final int tagName           =  3; // name : string with optional language shifted on 16 bits left
	public static final int tagSurName        =  4; // surname : string with optional language shifted on 16 bits left
	public static final int tagPatronymic     =  5; // patronymic : string with optional language shifted on 16 bits left
	public static final int tagDOB            =  6; // dob : ISO-8601 date representation
	public static final int tagPhone          =  7; // phone : string
	public static final int tagGLN            =  8; // gln : string (numeric)
	public static final int tagCountryIsoSymb =  9; // countryisosymb : string
	public static final int tagCountryIsoCode = 10; // countryisocode : string (numeric)
	public static final int tagCountryName    = 11; // country : string with optional language shifted on 16 bits left
	public static final int tagZIP            = 12; // zip : string
	public static final int tagCityName       = 13; // city : string with optional language shifted on 16 bits left
	public static final int tagStreet         = 14; // street : string with optional language shifted on 16 bits left
	public static final int tagAddress        = 15; // address : string with optional language shifted on 16 bits left
	public static final int tagImage          = 16; // image : mimeformat:mime64
	public static final int tagRuINN          = 17; // ru_inn : string (numeric)
	public static final int tagRuKPP          = 18; // ru_kpp : string (numeric)
	public static final int tagRuSnils        = 19; // ru_snils : string (numeric)
	public static final int tagModifTime      = 20; // modtime : ISO-8601 date-time representation (UTC)
	public static final int tagDescr          = 21; // descr : string
	public static final int tagLatitude       = 22; // lat : real
	public static final int tagLongitude      = 23; // lon : real
	//
	// Замечание по сроку действия: одна сторона передает другой период истечения срока действия в секундах.
	//   Принимающая сторона складывает это значение с текущим epoch-временем и сохраняет на своей стороне
	//   для того, чтобы в последующем принять решение о запросе обновления.
	//
	public static final int tagExpiryPeriodSec = 24; // @v11.2.3 Период истечения срока действия в секундах
	public static final int tagExpiryEpochSec  = 25; // @v11.2.3 Время истечения срока действия (секунды с 1/1/1970)
	public static final int tagEMail           = 26; // @v11.3.0
	public static final int tagVerifiability   = 27; // @v11.3.2 arbitrary || anonymous || verifiable
	public static final int tagStatus          = 28; // @v11.3.6 statusXXX : string
	public static final int tagImageBlobSignature = 29; // @v11.3.8 Сигнатура изображения tagImage для передачи клиенту (само изображение клиент получит от медиатора, предъявив сигнатуру)
	// } @persistent
	public static final int vArbitrary = 0;
	public static final int vAnonymous = 1;
	public static final int vVerifiable = 2;

	public static final int statusUndef = 0;
	public static final int statusPrvMale = SLib.GENDER_MALE; // 1
	public static final int statusPrvFemale = SLib.GENDER_FEMALE; // 2
	public static final int statusPrvGenderQuestioning = SLib.GENDER_QUESTIONING; // 3
	public static final int statusEnterprise = 1000; // Обобщенный статус юридического лица

	StyloQFace()
	{
		ID = 0;
		BI = null;
		L = new HashMap<Integer, String>();
	}
	static boolean IsTagLangDependent(int tag)
	{
		return (tag == tagCommonName || tag == tagName || tag == tagSurName || tag == tagPatronymic ||
			tag == tagCountryName || tag == tagCityName || tag == tagStreet || tag == tagAddress || tag == tagDescr) ? true : false;
	}
	public StyloQFace Z()
	{
		ID = 0;
		BI = null;
		L.clear();
		return this;
	}
	private String GetText_LangTolerant(int tag, int lang)
	{
		String result = null;
		if(IsTagLangDependent(tag)) {
			result = Get(tag, lang);
			if(result == null && lang != 0)
				result = Get(tag, 0);
		}
		else
			result = Get(tag, 0);
		return result;
	}
	public @NonNull String GetSimpleText(int lang)
	{
		String result = GetText_LangTolerant(tagCommonName, lang);
		if(result == null) {
			String sn = GetText_LangTolerant(tagSurName, lang);
			if(sn != null)
				result = sn;
			String nm = GetText_LangTolerant(tagName, lang);
			if(nm != null) {
				if(SLib.GetLen(result) > 0)
					result += " ";
				result += nm;
			}
			String pn = GetText_LangTolerant(tagPatronymic, lang);
			if(pn != null) {
				if(SLib.GetLen(result) > 0)
					result += " ";
				result += pn;
			}
		}
		if(SLib.GetLen(result) == 0)
			result = Get(tagPhone, 0);
		if(SLib.GetLen(result) == 0)
			result = Get(tagGLN, 0);
		if(SLib.GetLen(result) == 0)
			result = Get(tagRuINN, 0);
		if(SLib.GetLen(result) == 0) {
			if(SLib.GetLen(BI) > 0)
				result = Base64.getEncoder().encodeToString(BI);
		}
		if(SLib.GetLen(result) == 0) {
			if(ID > 0) {
				result = "ID" + " " + Long.toString(ID);
			}
		}
		if(SLib.GetLen(result) == 0)
			result = "Empty face entry";
		return result;
	}
	void Set(int tag, int lang, String text)
	{
		int eff_tag = (lang > 0 && IsTagLangDependent(tag)) ? (tag | (lang << 16)) : tag;
		if(SLib.GetLen(text) > 0)
			L.put(eff_tag, text);
		else
			L.remove(eff_tag);
	}
	//int   SetDob(LDATE dt) {}
	//int   SetImage(const SImageBuffer * pImg) {}
	String Get(int tag, int lang)
	{
		String result = null;
		if(lang > 0 && IsTagLangDependent(tag)) {
			int [] eff_tag_order = new int[20];
			int eff_tag_order_idx = 0;
			eff_tag_order[eff_tag_order_idx] = (tag | (lang << 16));
			eff_tag_order_idx++;
			eff_tag_order[eff_tag_order_idx] = (tag);
			eff_tag_order_idx++;
			if(lang != SLib.slangEN) {
				eff_tag_order[eff_tag_order_idx] = (tag | (SLib.slangEN << 16));
				eff_tag_order_idx++;
			}
			if(lang != SLib.slangRU) {
				eff_tag_order[eff_tag_order_idx] = (tag | (SLib.slangRU << 16));
				eff_tag_order_idx++;
			}
			for(int i = 0; result == null && i < eff_tag_order_idx; i++) {
				int eff_tag = eff_tag_order[i];
				result = L.get(eff_tag);
			}
		}
		else {
			result = L.get(tag);
		}
		return result;
	}
	String GetExactly(int tag, int lang)
	{
		int eff_tag = (tag | (lang << 16));
		return L.get(eff_tag);
	}
	int GetVerifiability()
	{
		int result = vArbitrary;
		String val = L.get(tagVerifiability);
		if(SLib.GetLen(val) > 0) {
			if(val.equalsIgnoreCase("arbitrary"))
				result = vArbitrary;
			else if(val.equalsIgnoreCase("verifiable"))
				result = vVerifiable;
			else if(val.equalsIgnoreCase("anonymous"))
				result = vAnonymous;
		}
		return result;
	}
	void SetVerifiability(int v)
	{
		String val = null;
		switch(v) {
			case vArbitrary: val = "arbitrary"; break;
			case vVerifiable: val = "verifiable"; break;
			case vAnonymous: val = "anonymous"; break;
		}
		Set(tagVerifiability, 0, val);
	}
	int GetStatus()
	{
		int    result = statusUndef;
		String val = L.get(tagStatus);
		if(SLib.GetLen(val) > 0) {
			if(val.equalsIgnoreCase("male"))
				result = statusPrvMale;
			else if(val.equalsIgnoreCase("female"))
				result = statusPrvFemale;
			else if(val.equalsIgnoreCase("private"))
				result = statusPrvGenderQuestioning;
			else if(val.equalsIgnoreCase("enterprise"))
				result = statusEnterprise;
			else if(val.equalsIgnoreCase("undef"))
				result = statusUndef;
			else
				result = -1;
		}
		return result;
	}
	boolean SetStatus(int status)
	{
		boolean ok = true;
		String tag_val = null;
		switch(status) {
			case statusPrvMale: tag_val = "male"; break;
			case statusPrvFemale: tag_val = "female"; break;
			case statusPrvGenderQuestioning: tag_val = "private"; break;
			case statusEnterprise: tag_val = "enterprise"; break;
			case statusUndef: tag_val = "undef"; break;
			default:
				assert(false);
				ok = false;
				break;
		}
		if(SLib.GetLen(tag_val) > 0) {
			assert(ok);
			Set(tagStatus, 0, tag_val);
		}
		return ok;
	}
	//void SetVerifiable(boolean v) { Set(tagVerifiable, 0, v ? "true" : "false"); }
	//boolean IsVerifiable()
	//{
	//	String val = Get(tagVerifiable, 0);
	//	return (val != null && val == "true") ? true : false;
	//}
	private static ArrayList <SLib.IntToStrAssoc> TagList;
	private final ArrayList <SLib.IntToStrAssoc> GetTagList()
	{
		if(TagList == null) {
			TagList = new ArrayList<SLib.IntToStrAssoc>();
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagModifTime, "modtime"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagVerifiable_Depricated, "verifialbe"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagCommonName, "cn"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagName, "name"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagSurName, "surname"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagPatronymic, "patronymic"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagDOB, "dob"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagPhone, "phone"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagGLN, "gln"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagCountryIsoSymb, "countryisosymb"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagCountryIsoCode, "countryisocode"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagCountryName, "country"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagZIP, "zip"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagCityName, "city"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagStreet, "street"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagAddress, "address"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagLatitude, "lat"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagLongitude, "lon"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagDescr, "descr"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagImage, "image"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagRuINN, "ruinn"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagRuKPP, "rukpp"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagRuSnils, "rusnils"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagExpiryPeriodSec, "expiryperiodsec"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagExpiryEpochSec,  "expiryepochsec"));
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagEMail,  "email")); // @v11.3.2
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagVerifiability,  "verifiability")); // @v11.3.2
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagStatus,  "status")); // @v11.3.6
			TagList.add(new SLib.IntToStrAssoc(StyloQFace.tagImageBlobSignature,  "imgblobs")); // @v11.3.8
		}
		return TagList;
	}
	//boolean GetImage(SImageBuffer * pImg) {}
	boolean FromJson(String jsonText)
	{
		boolean ok = false;
		try {
			if(SLib.GetLen(jsonText) > 0) {
				JSONObject jsobj = new JSONObject(jsonText);
				ArrayList<SLib.IntToStrAssoc> tag_list = GetTagList();
				Iterator <String> iter = jsobj.keys();
				while(iter.hasNext()) {
					String key = iter.next();
					Object value = jsobj.get(key);
					if(value != null) {
						int lang = 0;
						int tok_id = 0;
						String [] key_fragmets = key.split("\\.");
						String tag_symb = null;
						String lang_symb = null;
						if(key_fragmets.length == 1) {
							tag_symb = key_fragmets[0];
						}
						else if(key_fragmets.length > 1) {
							lang_symb = key_fragmets[key_fragmets.length-1];
							tag_symb = key_fragmets[0];
						}
						if(SLib.GetLen(tag_symb) > 0) {
							for(int j = 0; tok_id == 0 && j < tag_list.size(); j++) {
								if(tag_symb.equalsIgnoreCase(tag_list.get(j).Value)) {
									tok_id = tag_list.get(j).Key;
								}
							}
							if(tok_id > 0) {
								lang = SLib.GetLinguaIdent(lang_symb);
								Class cls = value.getClass();
								String cls_name = cls.getName();
								Set(tok_id, lang, value.toString());
								ok = true;
							}
						}
					}
				}
				/*for(int i = 0; i < tag_list.size(); i++) {
					SLib.IntToStrAssoc tag_entry = tag_list.get(i);
					if(IsTagLangDependent(tag_entry.Key)) {

					}
					else {
						String val = jsobj.optString(tag_entry.Value);
						if(SLib.GetLen(val) > 0) {
							L.put(tag_entry.Key, val);
						}
					}
					//jsobj.get("");
				}*/
			}
		} catch(JSONException exn) {
			ok = false;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return ok;
	}
	JSONObject ToJsonObj()
	{
		JSONObject result = null;
		try {
			if(L.size() > 0) {
				int [] lang_list = GetLanguageList();
				ArrayList<SLib.IntToStrAssoc> tag_list = GetTagList();
				result = new JSONObject();
				for(int i = 0; i < tag_list.size(); i++) {
					SLib.IntToStrAssoc tag_entry = tag_list.get(i);
					if(IsTagLangDependent(tag_entry.Key)) {
						if(lang_list != null) {
							for(int li = 0; li < lang_list.length; li++) {
								int lang = lang_list[li];
								String val = GetExactly(tag_entry.Key, lang);
								if(SLib.GetLen(val) > 0) {
									String lc = SLib.GetLinguaCode(lang);
									if(SLib.GetLen(lc) > 0) {
										String temp_tag = tag_entry.Value;
										temp_tag = temp_tag + "." + lc;
										result.put(temp_tag, val);
									}
								}
							}
						}
						else {
							String val = GetExactly(tag_entry.Key, 0);
							if(SLib.GetLen(val) > 0)
								result.put(tag_entry.Value, val);
						}
					}
					else {
						String val = GetExactly(tag_entry.Key, 0);
						if(SLib.GetLen(val) > 0)
							result.put(tag_entry.Value, val);
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return result;
	}
	String ToJson()
	{
		JSONObject jsobj_ = ToJsonObj();
		return (jsobj_ != null) ? jsobj_.toString() : null;
	}
	int [] GetLanguageList()
	{
		int [] result = null;
		ArrayList <Integer> temp_list = new ArrayList<Integer>();
		Iterator<Map.Entry<Integer, String>> iterator = L.entrySet().iterator();
		while(iterator.hasNext()) {
			Map.Entry<Integer, String> entry = iterator.next();
			int k = entry.getKey();
			if((k & 0xffff0000) != 0) {
				int lang = (k >> 16);
				temp_list.add(lang);
			}
		}
		if(temp_list.size() > 0) {
			result = new int[temp_list.size()];
			for(int i = 0; i < temp_list.size(); i++)
				result[i] = temp_list.get(i);
		}
		return result;
	}
	//private SLib.StrAssocArray L;
	long ID;    // Копия одноименного поля из записи SecTbl.Rec
	byte [] BI; // Копия одноименного поля из записи SecTbl.Rec
	Map<Integer, String> L;
}
