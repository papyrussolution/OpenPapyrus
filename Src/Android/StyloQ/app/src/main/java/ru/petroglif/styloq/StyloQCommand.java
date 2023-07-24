// StyloQCommand.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.location.Location;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.StringTokenizer;
import java.util.UUID;

public class StyloQCommand {
	//
	// Descr: Идентификаторы базовых типов команд (соответствуют StyloQCommandList::sqbcXXX in pp.h)
	//
	// Префиксы sqbcRsrv имеют команды, определяющие специализированный формат данных для того, чтобы было
	// проще реализовать 2-фазное взаимодействие между клиентом и сервисом. Фактически, это hardcoding-варианты
	// для ускорения начального этапа разработки и они несколько выпадают из общего концептуального замысла.
	//
	public static final int sqbcEmpty       =  0;
	public static final int sqbcRegister    =  1;
	public static final int sqbcDtLogin     =  2; // Команда авторизации в desktop-сеансе Papyrus с мобильного телефона
	public static final int sqbcPersonEvent =  3;
	public static final int sqbcReport      =  4;
	public static final int sqbcSearch      =  5; // Поисковый запрос
	public static final int sqbcObjTransmit = 21; // @special Передача данных между разделами papyrus. Кроме этой, мы резервируем
		// еще 9 (до 30 включительно) номеров для этой технологии, поскольку там достаточно разнообразных вариантов.
		// last obj transmit command = 30
	public static final int sqbcPosProtocolHost  = 31; // @special Данные в формате papyrus-pos-protocol со стороны хоста
	public static final int sqbcPosProtocolFront = 32; // @special Данные в формате papyrus-pos-protocol со стороны кассового узла
		// Резервируем еще 8 (до 40 включительно) номеров для этого протокола.
		// last pos protocol command = 40
	public static final int sqbcRsrvOrderPrereq  = 101; // Модуль данных, передаваемых сервисом клиенту чтобы тот мог сформировать
		// заказ. Дополнительные параметры определяют особенности модуля: заказ от конечного клиента,
		// агентский заказ, заказ на месте и т.д.
	public static final int sqbcRsrvAttendancePrereq = 102; // @v11.3.2 Модуль данных, передаваемых сервисом клиенту для формирования записи на обслуживаение.
	public static final int sqbcRsrvPushIndexContent = 103; // @v11.4.5 Параметры передачи сервисам-медиаторам данных для поисковой индексации
	public static final int sqbcRsrvIndoorSvcPrereq  = 104; // @v11.4.5 Параметры обслуживания внутри помещения сервиса (horeca, shop, etc)
		// Данные строятся на основании параметров, определяемых кассовым узлом.
	public static final int sqbcGoodsInfo            = 105; // @v11.4.5 Параметры, определяющие вывод информации об одном товаре
	public static final int sqbcLocalBarcodeSearch   = 106; // @v11.4.5 Поиск в пределах сервиса (преимущественно) по штрихкоду.
		// Если сервис предоставяет такую функцию, то она отображается в виде иконки на экране мобильного устройства, а не в общем списке.
	public static final int sqbcIncomingListOrder    = 107;  // @v11.4.6 Список входящих заказов (команда обязательно ассоциируется с внутренним объектом данных: персоналией, пользователем etc)
	public static final int sqbcIncomingListCCheck   = 108;  // @v11.4.7 Список входящих кассовых чеков (команда обязательно ассоциируется с внутренним объектом данных: персоналией, пользователем etc)
	public static final int sqbcIncomingListTSess    = 109;  // @v11.4.7 Список входящих технологических сессий (команда обязательно ассоциируется с внутренним объектом данных: персоналией, пользователем etc)
	public static final int sqbcIncomingListTodo     = 110;  // @v11.4.7 Список входящих задач (команда обязательно ассоциируется с внутренним объектом данных: персоналией, пользователем etc)
	public static final int sqbcDebtList             = 111;  // @v11.5.4 Список долговых документов по контрагенту. В общем случае, это могут быть как долги и покупателей, так и наша
		// задолженность перед поставщиками. Пока предполагается использовать как вспомогательную команду для получения долговой ведомости по клиенту в рамках
		// работы с более высокоуровневыми командами (eg sqbcRsrvOrderPrereq)

	public static class ViewDefinitionEntry {
		String Zone;
		String FieldName;
		String Text;
		int    TotalFunc; // SLib.AGGRFUNC_XXX
	}
	//
	// Descr: Ограниченный клиентский вариант представления команды сервиса.
	// Полный вариант определен в Papyrus class StyloQCommandList
	//
	public static class Item {
		//
		// Descr: Флаги команды. Некоторые из этих флагов применимы только на стороне сервиса,
		//   тем не менее, для полноты соответствия они здесь перечислены.
		//
		public static final int fResultPersistent = 0x0001;
		public static final int fPrepareAhead     = 0x0002; // @v11.4.6 Данные команды готовятся заранее чтобы при запросе от клиента отдать их максимально быстро.
			// Эта опция не может быть применена в случае, если параметры команды содержат ссылку на контекстные данные!
			// Поскольку не ясно какой именно результат должен быть подготовлен заранее.
		public static final int fNotify_ObjNew    = 0x0004; // @v11.5.9 Опция извещения: извещать о новых объектах
		public static final int fNotify_ObjUpd    = 0x0008; // @v11.5.9 Опция извещения: извещать об измененных объектах
		public static final int fNotify_ObjStatus = 0x0010; // @v11.5.9 Опция извещения: извещать об изменении статусов объектов

		public static final int fNotify_MASK = (fNotify_ObjNew|fNotify_ObjUpd|fNotify_ObjStatus);
		Item()
		{
			Uuid = null;
			ResultExpiryTimeSec = 0;
			BaseCmdId = 0;
			Flags = 0;
			Name = null;
			Description = null;
			Image = null;
			Vd = null;
			MaxDistM = 0.0;
			GeoLocDistTo = null;
		}
		//
		// Descr: Если для команды определено максимальное расстояние от клиента до целевой локации,
		//   то функция возвращает это расстояние (в метрах). В противном случае возвращает 0.
		//
		double GetGeoDistanceRestriction()
		{
			return (MaxDistM > 0.0 && GeoLocDistTo != null && !GeoLocDistTo.IsZero() && GeoLocDistTo.IsValid()) ? MaxDistM : 0.0;
		}
		boolean CanEvaluateDistance(final SLib.GeoPosLL currentGeoLoc)
		{
			return (GeoLocDistTo != null && !GeoLocDistTo.IsZero() && GeoLocDistTo.IsValid() &&
				currentGeoLoc != null && !currentGeoLoc.IsZero() && currentGeoLoc.IsValid());
		}
		double GetGeoDistance(final SLib.GeoPosLL currentGeoLoc)
		{
			double dist_m = -1.0;
			if(CanEvaluateDistance(currentGeoLoc)) {
				float [] _distance = new float[3];
				Location.distanceBetween(currentGeoLoc.Lat, currentGeoLoc.Lon, GeoLocDistTo.Lat, GeoLocDistTo.Lon, _distance);
				dist_m = _distance[0];
			}
			return dist_m;
		}
		UUID  Uuid;                //
		int   ResultExpiryTimeSec; // @v11.2.5 Период истечения срока действия результата в секундах. (<=0 - undefined)
			// Если ResultExpiryPeriodSec то клиент может пользоваться результатом запроса в течении этого времени без
			// повторного обращения к сервису.
		int   BaseCmdId;           // @v11.2.9 Идентификатор базовой команды. Нужен для автоматического
			// определения ряда параметров команды.
		int   Flags;               // @v11.5.9
		String Name;               // Наименование команды
		String Description;        // Подробное описание команды
		String Image;              // Ссылка на изображение, ассоциированное с командой
		double MaxDistM;           // @v11.6.5 Требуемая сервисом максимальная дистация до GeoLocDistTo
		SLib.GeoPosLL GeoLocDistTo; // @v11.6.5 Геолокация, для которой задана максимальная дистация MaxDistM.
		ArrayList<ViewDefinitionEntry> Vd;
	}
	public static class List {
		public List()
		{
			TimeStamp = 0;
			ExpirTimeSec = 0;
			Items = null;
		}
		private static boolean IsHidden(final Item item)
		{
			return (item == null || item.BaseCmdId == sqbcLocalBarcodeSearch || item.BaseCmdId == sqbcDebtList); // @v11.5.4 !sqbcDebtList
		}
		//
		// Descr: Возвращает количество элементов, которые должны быть отображены
		//   в списке команд. В это число не включаются команды со специальным
		//   типом отображения (поиск по штрихкоду, например)
		//
		public int GetViewCount()
		{
			int    result = 0;
			if(Items != null) {
				for(int i = 0; i < Items.size(); i++) {
					if(!IsHidden(Items.get(i)))
						result++;
				}
			}
			return result;
		}
		public Item GetViewItem(int idx)
		{
			Item result = null;
			if(SLib.IsInRange(idx, Items)) {
				int counter = 0;
				for(int i = 0; result == null && i < Items.size(); i++) {
					final Item item = Items.get(i);
					if(!IsHidden(item)) {
						if(counter == idx)
							result = item;
						counter++;
					}
				}
			}
			return result;
		}
		public Item GetItemWithParticularBaseId(int baseCmdId)
		{
			Item result = null;
			if(Items != null) {
				for(int i = 0; result == null && i < Items.size(); i++) {
					final Item item = Items.get(i);
					if(item != null && item.BaseCmdId == baseCmdId) // @v11.5.4 !sqbcDebtList
						result = item;
				}
			}
			return result;
		}
		public Item GetByUuid(UUID cmdUuid)
		{
			Item result = null;
			if(Items != null && cmdUuid != null) {
				for(int i = 0; result == null && i < Items.size(); i++) {
					final Item item = Items.get(i);
					if(item != null && SLib.AreUUIDsEqual(item.Uuid, cmdUuid))
						result = item;
				}
			}
			return result;
		}
		long TimeStamp;
		long ExpirTimeSec;
		public ArrayList <Item> Items;
	}
	//
	// Descr: Предварительные статусы команд, которые могут быть исполнены сервисом
	//
	public static final int prestatusUnkn = 0; // Неизвестный статус (неопределенное либо ошибочное состояние)
	public static final int prestatusPending = 1; // Ожидается результат исполнения команды сервисом
	public static final int prestatusInstant = 2; // Моментальное исполение сервисом (запрос-ответ)
	public static final int prestatusActualResultStored = 3; // Ранее полученный результат сохранен и актуален
	public static final int prestatusQueryNeeded = 4; // Запрос может быть длительным и ранее сохраненного
		// результата нет либо он просрочен.
	public static class CommandPrestatus {
		CommandPrestatus()
		{
			S = prestatusUnkn;
			WaitingTimeMs = 0;
		}
		int   S; // prestatusXXX
		int   WaitingTimeMs; // Текущее время ожидания в миллисекундах (для S==prestatusPending)
	}
	public static String MakeNotifyList(int flags)
	{
		String result = null;
		if((flags & Item.fNotify_MASK) != 0) {
			if((flags & Item.fNotify_ObjNew) != 0) {
				final String flag_mnem = "objnew";
				if(result == null)
					result = flag_mnem;
				else
					result += ("," + flag_mnem);
			}
			if((flags & Item.fNotify_ObjUpd) != 0) {
				final String flag_mnem = "objupd";
				if(result == null)
					result = flag_mnem;
				else
					result += ("," + flag_mnem);
			}
			if((flags & Item.fNotify_ObjStatus) != 0) {
				final String flag_mnem = "objstatus";
				if(result == null)
					result = flag_mnem;
				else
					result += ("," + flag_mnem);
			}
		}
		return result;
	}
	static List FromJson(String jsonText)
	{
		// На стороне сервера Papyrus json формируется функцией StyloQCommandList::CreateJsonForClient
		List result = null;
		try {
			if(SLib.GetLen(jsonText) > 0) {
				JSONObject jsobj = new JSONObject(jsonText);
				result = new List();
				result.TimeStamp = jsobj.optLong("time", 0);
				result.ExpirTimeSec = jsobj.optLong("expir_time_sec", 0);
				JSONArray ary = jsobj.getJSONArray("item_list");
				if(ary != null) {
					for(int i = 0; i < ary.length(); i++) {
						JSONObject jsitem = ary.getJSONObject(i);
						if(jsitem != null) {
							Item item = new Item();
							item.Uuid = SLib.strtouuid(jsitem.optString("uuid", null));
							item.BaseCmdId = jsitem.optInt("basecmdid", 0); // @v11.2.9
							item.ResultExpiryTimeSec = jsitem.optInt("result_expir_time_sec", 0); // @v11.2.5
							item.Name = jsitem.getString("name");
							item.Description = jsitem.optString("descr", "");
							// @v11.5.9 {
							{
								String notify_options = jsitem.optString("notify", null);
								if(SLib.GetLen(notify_options) > 0) {
									StringTokenizer toknzr = new StringTokenizer(notify_options, ",");
									final int _c = toknzr.countTokens();
									for(int j = 0; j < _c; j++) {
										String tok = toknzr.nextToken();
										tok.trim();
										if(tok.equalsIgnoreCase("objnew"))
											item.Flags |= Item.fNotify_ObjNew;
										else if(tok.equalsIgnoreCase("objupd"))
											item.Flags |= Item.fNotify_ObjUpd;
										else if(tok.equalsIgnoreCase("objstatus"))
											item.Flags |= Item.fNotify_ObjStatus;
									}
								}
							}
							// } @v11.5.9
							// @v11.6.5 {
							{
								JSONObject js_maxdist = jsitem.optJSONObject("maxdistto");
								if(js_maxdist != null) {
									item.MaxDistM = js_maxdist.optDouble("dist", 0.0);
									if(item.MaxDistM > 0.0) {
										item.GeoLocDistTo = new SLib.GeoPosLL(js_maxdist.optDouble("lat", 0.0), js_maxdist.optDouble("lon", 0.0));
										if(!item.GeoLocDistTo.IsValid() || item.GeoLocDistTo.IsZero()) {
											item.MaxDistM = 0.0;
											item.GeoLocDistTo = null;
										}
									}
									else {
										item.MaxDistM = 0.0;
										item.GeoLocDistTo = null;
									}
								}
							}
							// } @v11.6.5
							if(result.Items == null)
								result.Items = new ArrayList<Item>();
							result.Items.add(item);
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
			new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
		}
		return result;
	}
	public static class DocDeclaration {
		public DocDeclaration()
		{
			Z();
		}
		DocDeclaration Z()
		{
			Type = null;
			Format = null;
			DisplayMethod = null;
			Time = 0;
			ResultExpiryTimeSec = 0;
			return this;
		}
		boolean FromJsonObj(final JSONObject jsObj)
		{
			boolean ok = false;
			Z();
			if(jsObj != null) {
				Type = jsObj.optString("type", null);
				Format = jsObj.optString("format", null);
				DisplayMethod = jsObj.optString("displaymethod", null);
				Time = jsObj.optLong("time", 0);
				ResultExpiryTimeSec = jsObj.optInt("resultexpirytimesec", 0);
				if(SLib.GetLen(Type) > 0 || SLib.GetLen(Format) > 0 || SLib.GetLen(DisplayMethod) > 0)
					ok = true;
			}
			return ok;
		}
		boolean FromJson(String jsonText)
		{
			boolean ok = true;
			Z();
			if(SLib.GetLen(jsonText) > 0) {
				try {
					if(SLib.GetLen(jsonText) > 0) {
						ok = FromJsonObj(new JSONObject(jsonText));
					}
				} catch(JSONException exn) {
					ok = false;
					Z();
					new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
				}
			}
			else
				ok = false;
			return ok;
		}
		String Type;
		String Format;
		String DisplayMethod;
		long   Time;
		int    ResultExpiryTimeSec;
	}
	//
	// Descr: Специализированная структура, предназначенная для передачи
	//   между объектами ссылки на документ.
	//   Содержит идентификатор документа в локальной базе данных и
	//   декларацию документа.
	//
	public static class DocReference {
		DocReference()
		{
			ID = 0;
			Decl = null;
		}
		long   ID;
		DocDeclaration Decl;
	}
	//
	//
	//
	private static class Pending {
		Pending(byte [] svcIdent, Item cmdItem)
		{
			SvcIdent = svcIdent;
			if(cmdItem != null && cmdItem.Uuid != null)
				CmdUuid = cmdItem.Uuid;
			TimeStart = System.currentTimeMillis();
		}
		byte [] SvcIdent;
		UUID CmdUuid;
		long TimeStart;
	}
	private static Object PendingMutex;
	private static void InitPendingMutex()
	{
		if(PendingMutex == null)
			PendingMutex = new Object();
	}
	private static ArrayList <Pending> PendingList;
	private static int SearchPendingEntry(byte [] svcIdent, Item cmdItem)
	{
		int    result = -1;
		if(PendingList != null && SLib.GetLen(svcIdent) > 0) {
			for(int i = 0; result < 0 && i < PendingList.size(); i++) {
				Pending pi = PendingList.get(i);
				if(pi != null && SLib.AreByteArraysEqual(pi.SvcIdent, svcIdent)) {
					if(cmdItem == null || SLib.AreUUIDsEqual(cmdItem.Uuid, pi.CmdUuid))
						result = i;
				}
			}
		}
		return result;
	}
	//
	// Returns:
	//   0 - ни одна команда сервиса svcIdent не находится в состоянии ожидания.
	//   -1 - хотя бы одна команда сервиса svcIdent находится в состоянии ожидания.
	//   >0 - команда cmdItem сервиса svcIdent находится в состоянии ожидания.
	//      Конкретное возвращенное значение равно количеству миллисекунд, прошедших с запуска.
	//      Значение 1 скорее всего означает, что время ожидания вычислить не удалось.
	//
	public static int IsCommandPending(byte [] svcIdent, Item cmdItem)
	{
		int result = 0;
		if(SLib.GetLen(svcIdent) > 0) {
			InitPendingMutex();
			synchronized(PendingMutex) {
				int idx = SearchPendingEntry(svcIdent, cmdItem);
				if(idx >= 0) {
					if(cmdItem != null) {
						Pending pi = PendingList.get(idx);
						result = (int)(System.currentTimeMillis() - pi.TimeStart);
						pi = null;
						if(result <= 0)
							result = 1;
					}
					else
						result = -1;
				}
				else if(cmdItem != null) {
					if(SearchPendingEntry(svcIdent, null) >= 0)
						result = -1;
				}
			}
		}
		return result;
	}
	public static void StartPendingCommand(byte [] svcIdent, Item cmdItem)
	{
		if(SLib.GetLen(svcIdent) > 0 && cmdItem != null) {
			InitPendingMutex();
			synchronized(PendingMutex) {
				int idx = SearchPendingEntry(svcIdent, cmdItem);
				if(idx < 0) {
					if(PendingList == null)
						PendingList = new ArrayList<Pending>();
					PendingList.add(new Pending(svcIdent, cmdItem));
				}
			}
		}
	}
	public static void StopPendingCommand(byte [] svcIdent, Item cmdItem)
	{
		if(SLib.GetLen(svcIdent) > 0 && cmdItem != null) {
			InitPendingMutex();
			synchronized(PendingMutex) {
				int idx = SearchPendingEntry(svcIdent, cmdItem);
				if(idx >= 0)
					PendingList.remove(idx);
			}
		}
	}
}
