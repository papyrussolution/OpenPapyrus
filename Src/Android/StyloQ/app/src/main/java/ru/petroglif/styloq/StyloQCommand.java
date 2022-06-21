// StyloQCommand.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
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
	public static final int sqbcLogin       =  2;
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
		Item()
		{
			Uuid = null;
			ResultExpiryTimeSec = 0;
			BaseCmdId = 0;
			Name = null;
			Description = null;
			Image = null;
			Vd = null;
		}
		UUID  Uuid;                //
		int   ResultExpiryTimeSec; // @v11.2.5 Период истечения срока действия результата в секундах. (<=0 - undefined)
			// Если ResultExpiryPeriodSec то клиент может пользоваться результатом запроса в течении этого времени без
			// повторного обращения к сервису.
		int   BaseCmdId;           // @v11.2.9 Идентификатор базовой команды. Нужен для автоматического
			// определения ряда параметров команды.
		String Name;               // Наименование команды
		String Description;        // Подробное описание команды
		String Image;              // Ссылка на изображение, ассоциированное с командой
		ArrayList<ViewDefinitionEntry> Vd;
	}
	public static class List {
		List()
		{
			TimeStamp = 0;
			ExpirTimeSec = 0;
			Items = null;
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
		boolean FromJson(String jsonText)
		{
			boolean ok = true;
			Z();
			if(SLib.GetLen(jsonText) > 0) {
				try {
					if(SLib.GetLen(jsonText) > 0) {
						JSONObject jsobj = new JSONObject(jsonText);
						Type = jsobj.optString("type", null);
						Format = jsobj.optString("format", null);
						DisplayMethod = jsobj.optString("displaymethod", null);
						Time = jsobj.optLong("time", 0);
						ResultExpiryTimeSec = jsobj.optInt("resultexpirytimesec", 0);
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
					if(cmdItem == null)
						result = i;
					else if(cmdItem != null && pi.CmdUuid != null && cmdItem.Uuid.compareTo(pi.CmdUuid) == 0)
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
