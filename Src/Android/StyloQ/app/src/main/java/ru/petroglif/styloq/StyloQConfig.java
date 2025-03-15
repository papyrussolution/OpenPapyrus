// StyloQConfig.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class StyloQConfig {
	// @persistent {
	// Теги конфигурации
	public static int tagUnkn            = 0;
	public static int tagUrl             = 1;
	public static int tagMqbAuth         = 2;
	public static int tagMqbSecret       = 3;
	public static int tagLoclUrl         = 4; // URL локальной обработки запросов (отдельная машина или сеанс)
	public static int tagLoclMqbAuth     = 5; // Login MQ-брокера локальной обработки запросов (отдельная машина или сеанс)
	public static int tagLoclMqbSecret   = 6; // Secret MQ-брокера локальной обработки запросов (отдельная машина или сеанс)
	public static int tagFeatures        = 7; // Флаги особенностей сервиса
	//
	// Замечание по сроку действия: одна сторона передает другой период истечения срока действия в секундах.
	//   Принимающая сторона складывает это значение с текущим epoch-временем и сохраняет на своей стороне
	//   для того, чтобы в последующем принять решение о запросе обновления.
	//
	public static int tagExpiryPeriodSec = 8; // @v11.2.3 Период истечения срока действия в секундах
	public static int tagExpiryEpochSec  = 9; // @v11.2.3 Время истечения срока действия (секунды с 1/1/1970)
	public static int tagPrefLanguage    = 10; // @v11.2.5 (private config) Предпочтительный язык
	public static int tagDefFace         = 11; // @v11.2.5 (private config) Лик, используемый клиентом по умолчанию
	public static int tagRole            = 12; // @v11.2.8 StyloQConfig::roleXXX Роль записи
	public static int tagCliFlags        = 13; // @v11.6.0 StyloQConfig::clifXXX Флаги клиента на стороне сервиса. То есть, после сопоставления клиента, сервис может
		// присвоить ему какие-либо флаги, например, с целью наделить его какими-то полномочиями
	public static int tagNotificationActualDays = 14; // @v11.7.4 (private config) Количество дней актуальности уведомлений
	public static int tagUserFlags       = 15; // @v12.0.11 StyloQConfig::userfXXX Флаги пользовательского сеанса. Хранятся только на стороне этого приложения (сервису не передаются).
	// } @persistent
	// Роли записи
	public static int roleUndef          = 0;
	public static int roleClient         = 1;
	public static int rolePublicService  = 2;
	public static int rolePrivateService = 3;
	public static int roleMediator       = 4;

	public static int featrfMediator = 0x0001; // Сервис выполняет функции медиатора (обслуживание других сервисов и клиентов)
	//
	// Descr: Флаги для тега tagCliFlags
	//
	public static int clifFaceSelfModifying = 0x0001; // Явное разрешение на автоматическое изменение лика клиентом. Это флаг избыточен, так как
				// вопрос изменения лика клиента решается автоматически на остновании признака StyloQCore::styloqfAutoObjMatching.
				// Но в том, случае, если сервис "хочет" явно разрешить клиенту обновлять свой лик, то может установить этот флаг.
	public static int clifSvcGPS            = 0x0002; // Клиенту разрешается установить GPS-координаты сервиса
	public static int clifPsnAdrGPS         = 0x0004; // Клиенту разрешается устанавливать GPS-координаты адресов доставки перосналий (контакты, покупатели etc)
	public static int clifShareDoc          = 0x0008; // @v11.9.0 Клиенту разрешается делиться документами
	//
	// Descr: Флаги для тега tagUserFlags
	//
	public static int userfNetworkDisabled  = 0x0001; // @v12.0.11 Пользователь намеренно отключил функции сетевого обмена
	public static int userfSvcArchived        = 0x0002; // @v12.2.2  Сервис архивирован (более не используется)
	public static int userfClipboardBcScanner = 0x0004; // @v12.2.10 Коды от встроенного сканера штрихкодов получать через буфер обмена (в противном
		// случае, система будет пытаться получать коды методами, специфичными для сканера - успех очень не гарантирован).

	private Map<Integer, String> L;

	StyloQConfig()
	{
		L = new HashMap<Integer, String>();
	}
	StyloQConfig Z()
	{
		L.clear();
		return this;
	}
	void Set(int tag, String text)
	{
		if(SLib.GetLen(text) > 0)
			L.put(tag, text);
		else
			L.remove(tag);
	}
	String Get(int tag)
	{
		return L.get(tag);
	}
	private static ArrayList<SLib.IntToStrAssoc> TagList;
	private final ArrayList <SLib.IntToStrAssoc> GetTagList()
	{
		if(TagList == null) {
			TagList = new ArrayList<SLib.IntToStrAssoc>();
			TagList.add(new SLib.IntToStrAssoc(tagUrl, "url"));
			TagList.add(new SLib.IntToStrAssoc(tagMqbAuth, "mqbauth"));
			TagList.add(new SLib.IntToStrAssoc(tagMqbSecret, "mqbsecret"));
			TagList.add(new SLib.IntToStrAssoc(tagLoclUrl,         "loclurl"));
			TagList.add(new SLib.IntToStrAssoc(tagLoclMqbAuth,     "loclmqbauth"));
			TagList.add(new SLib.IntToStrAssoc(tagLoclMqbSecret,   "loclmqbsecret"));
			TagList.add(new SLib.IntToStrAssoc(tagFeatures,        "features"));
			TagList.add(new SLib.IntToStrAssoc(tagExpiryPeriodSec, "expiryperiodsec"));
			TagList.add(new SLib.IntToStrAssoc(tagExpiryEpochSec,  "expiryepochsec"));
			TagList.add(new SLib.IntToStrAssoc(tagPrefLanguage,  "preflang")); // @v11.2.5
			TagList.add(new SLib.IntToStrAssoc(tagDefFace,  "defface")); // @v11.2.5
			TagList.add(new SLib.IntToStrAssoc(tagRole,  "role")); // @v11.2.8
			TagList.add(new SLib.IntToStrAssoc(tagCliFlags,  "cliflags")); // @v11.6.0
			TagList.add(new SLib.IntToStrAssoc(tagNotificationActualDays,  "notificationactualdays")); // @v11.7.4
			TagList.add(new SLib.IntToStrAssoc(tagUserFlags,  "userflags")); // @v12.0.11
		}
		return TagList;
	}
	boolean FromJsonObj(JSONObject jsObj)
	{
		boolean ok = false;
		if(jsObj != null) {
			try {
				ArrayList<SLib.IntToStrAssoc> tag_list = GetTagList();
				Iterator<String> iter = jsObj.keys();
				while(iter.hasNext()) {
					String key = iter.next();
					Object value = jsObj.get(key);
					if(value != null) {
						int tok_id = 0;
						if(SLib.GetLen(key) > 0) {
							for(int j = 0; tok_id == 0 && j < tag_list.size(); j++) {
								if(key.equalsIgnoreCase(tag_list.get(j).Value)) {
									tok_id = tag_list.get(j).Key;
								}
							}
							if(tok_id > 0) {
								Class cls = value.getClass();
								String cls_name = cls.getName();
								Set(tok_id, value.toString());
								ok = true;
							}
						}
					}
				}
			} catch(JSONException exn) {
				ok = false;
				new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
			}
		}
		return ok;
	}
	boolean FromJson(String jsonText)
	{
		boolean ok = false;
		try {
			if(SLib.GetLen(jsonText) > 0) {
				ok = FromJsonObj(new JSONObject(jsonText));
			}
		} catch(JSONException exn) {
			ok = false;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return ok;
	}
	String ToJson()
	{
		String result = null;
		try {
			if(L.size() > 0) {
				ArrayList<SLib.IntToStrAssoc> tag_list = GetTagList();
				JSONObject jsobj = new JSONObject();
				for(int i = 0; i < tag_list.size(); i++) {
					SLib.IntToStrAssoc tag_entry = tag_list.get(i);
					String val = Get(tag_entry.Key);
					if(SLib.GetLen(val) > 0)
						jsobj.put(tag_entry.Value, val);
				}
				result = jsobj.toString();
			}
		} catch(JSONException exn) {
			result = null;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return result;
	}
}
