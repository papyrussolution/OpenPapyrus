// Document.java 
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.Base64;
import java.util.StringTokenizer;
import java.util.UUID;

public class Document {
	//
	// Descr: Действия над документами из входящего списка
	//   Копия соответсвующих значений из Papyrus StyloQIncomingListParam::actionXXX
	//
	public static final int actionDocStatus           = 0x0001; // Изменение статуса
	public static final int actionDocAcceptance       = 0x0002; // Приемка товара (приходные накладные)
	public static final int actionDocAcceptanceMarks  = 0x0004; // Проверка марок в строках входящего документа (приходные накладные)
	public static final int actionDocSettingMarks     = 0x0008; // Расстановка марок на строках исходящего документа (расходные накладные)
	public static final int actionDocInventory        = 0x0010; // Ввод документа инвентаризации
	public static final int actionGoodsItemCorrection = 0x0020; // Корректировка позиций (количество, цена, удаление позиции, замена товара)
	public static final int actionCCheckCreat         = 0x0040; // @v11.5.2 Создание чека
	public static final int actionCCheckMod           = 0x0080; // @v11.5.2 Модификация чека
	public static final int actionCCheckRegPrint      = 0x0100; // @v11.5.2 Печать чека на регистраторе

	public static int IncomingListActionsFromString(final String input)
	{
		int    result = 0;
		if(SLib.GetLen(input) > 0) {
			StringTokenizer toknzr = new StringTokenizer(input, ",");
			int _c = toknzr.countTokens();
			for(int i = 0; i < _c; i++) {
				String tok = toknzr.nextToken();
				tok.trim();
				if(tok.equalsIgnoreCase("DocStatus"))
					result |= actionDocStatus;
				else if(tok.equalsIgnoreCase("DocAcceptance"))
					result |= actionDocAcceptance;
				else if(tok.equalsIgnoreCase("DocAcceptanceMarks"))
					result |= actionDocAcceptanceMarks;
				else if(tok.equalsIgnoreCase("DocSettingMarks"))
					result |= actionDocSettingMarks;
				else if(tok.equalsIgnoreCase("DocInventory"))
					result |= actionDocInventory;
				else if(tok.equalsIgnoreCase("GoodsItemCorrection"))
					result |= actionGoodsItemCorrection;
				// @v11.5.2 {
				else if(tok.equalsIgnoreCase("CCheckCreat"))
					result |= actionCCheckCreat;
				else if(tok.equalsIgnoreCase("CCheckMod"))
					result |= actionCCheckMod;
				else if(tok.equalsIgnoreCase("CCheckRegPrint"))
					result |= actionCCheckRegPrint;
				// } @v11.5.2
			}
		}
		return result;
	}
	public static String IncomingListActionsToString(final int actionFlags)
	{
		String result = "";
		if(actionFlags != 0) {
			if((actionFlags & actionDocStatus) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "DocStatus";
			}
			if((actionFlags & actionDocAcceptance) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "DocAcceptance";
			}
			if((actionFlags & actionDocAcceptanceMarks) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "DocAcceptanceMarks";
			}
			if((actionFlags & actionDocSettingMarks) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "DocSettingMarks";
			}
			if((actionFlags & actionDocInventory) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "DocInventory";
			}
			if((actionFlags & actionGoodsItemCorrection) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "GoodsItemCorrection";
			}
			// @v11.5.2 {
			if((actionFlags & actionCCheckCreat) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "CCheckCreat";
			}
			if((actionFlags & actionCCheckMod) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "CCheckMod";
			}
			if((actionFlags & actionCCheckRegPrint) != 0) {
				if(SLib.GetLen(result) > 0)
					result += ",";
				result += "CCheckRegPrint";
			}
			// } @v11.5.2
		}
		return result;
	}

	Head H;
	ArrayList <TransferItem> TiList;
	ArrayList <BookingItem> BkList; // Список позиций повременных элементов, связанных с процессорами
	ArrayList <LotExtCode> VXcL; // Валидирующий контейнер спецкодов. Применяется для проверки кодов, поступивших с документом в XcL
	ArrayList <LotExtCode> XcL_Unassigned; // Список марок, просканированных в режиме расстановки, для которых не нашлось сопоставления
		// с товарными строками документа.
	private int AfterTransmitStatus; // @transient Флаги статуса документа, которые должны быть установлены в БД после успешной отправки сервису
	public int DetailExpandStatus_Ti; // @transient

	public static class Head {
		public static boolean AreEq(final Head a1, final Head a2)
		{
			return (a1 != null) ? (a2 != null && a1.IsEq(a2)) : (a2 == null);
		}
		Head()
		{
			ID = 0;
			CreationTime = null;
			Time = null;
			DueTime = null;
			SvcOpID = 0;
			InterchangeOpID = 0;
			AgentID = 0; // @v11.4.6
			PosNodeID = 0; // @v11.4.6
			ClientID = 0;
			DlvrLocID = 0;
			Flags = 0;
			StatusSurrId = 0; // @v11.5.1
			Amount = 0;
			Code = null;
			SvcIdent = null;
			Uuid = null;
			OrgCmdUuid = null;
			BaseCurrencySymb = null;
			Memo = null;
		}
		public static Head Copy(Head s)
		{
			Head copy = null;
			if(s != null) {
				copy = new Head();
				copy.ID = s.ID;
				copy.CreationTime = SLib.LDATETIME.Copy(s.CreationTime);
				copy.Time = SLib.LDATETIME.Copy(s.Time);
				copy.DueTime = SLib.LDATETIME.Copy(s.DueTime);
				copy.SvcOpID = s.SvcOpID;
				copy.InterchangeOpID = s.InterchangeOpID;
				copy.AgentID = s.AgentID;
				copy.PosNodeID = s.PosNodeID;
				copy.ClientID = s.ClientID;
				copy.DlvrLocID = s.DlvrLocID;
				copy.Flags = s.Flags;
				copy.StatusSurrId = s.StatusSurrId; // @v11.5.1
				copy.Amount = s.Amount;
				copy.Code = SLib.Copy(s.Code);
				copy.SvcIdent = SLib.Copy(s.SvcIdent);
				copy.Uuid = SLib.Copy(s.Uuid);
				copy.OrgCmdUuid = SLib.Copy(s.OrgCmdUuid);
				copy.BaseCurrencySymb = SLib.Copy(s.BaseCurrencySymb);
				copy.Memo = SLib.Copy(s.Memo);
			}
			return copy;
		}
		boolean IsEq(final Head s)
		{
			boolean yes = true;
			if(s == null)
				yes = false;
			else if(ID != s.ID)
				yes = false;
			else if(!SLib.LDATETIME.ArEq(CreationTime, s.CreationTime))
				yes = false;
			else if(!SLib.LDATETIME.ArEq(DueTime, s.DueTime))
				yes = false;
			else if(ClientID != s.ClientID)
				yes = false;
			else if(AgentID != s.AgentID)
				yes = false;
			else if(PosNodeID != s.PosNodeID)
				yes = false;
			else if(DlvrLocID != s.DlvrLocID)
				yes = false;
			else if(Flags != s.Flags)
				yes = false;
			else if(StatusSurrId != s.StatusSurrId)
				yes = false;
			else if(Amount != s.Amount)
				yes = false;
			else if(!SLib.AreStringsEqual(Code, s.Code))
				yes = false;
			else if(!SLib.AreStringsEqual(BaseCurrencySymb, s.BaseCurrencySymb))
				yes = false;
			else if(!SLib.AreStringsEqual(Memo, s.Memo))
				yes = false;
			else if(!SLib.AreByteArraysEqual(SvcIdent, s.SvcIdent))
				yes = false;
			else if(!SLib.AreUUIDsEqual(Uuid, s.Uuid))
				yes = false;
			else if(!SLib.AreUUIDsEqual(OrgCmdUuid, s.OrgCmdUuid))
				yes = false;
			return yes;
		}
		public SLib.LDATE GetNominalDate()
		{
			SLib.LDATE d = null;
			if(Time != null && SLib.CheckDate(Time.d))
				d = Time.d;
			else if(CreationTime != null && SLib.CheckDate(CreationTime.d))
				d = CreationTime.d;
			return d;
		}
		public SLib.LDATETIME GetNominalTimestamp()
		{
			SLib.LDATETIME dtm = null;
			if(Time != null && SLib.CheckDate(Time.d))
				dtm = Time;
			else if(CreationTime != null && SLib.CheckDate(CreationTime.d))
				dtm = CreationTime;
			return dtm;
		}
		public boolean IncrementDueDate(boolean checkOnly)
		{
			boolean result = false;
			SLib.LDATETIME base_dtm = (DueTime != null && SLib.CheckDate(DueTime.d)) ? DueTime : GetNominalTimestamp();
			if(base_dtm != null && SLib.CheckDate(base_dtm.d)) {
				SLib.LDATE new_date = SLib.LDATE.Plus(base_dtm.d, 1);
				if(SLib.CheckDate(new_date)) {
					if(!checkOnly) {
						if(DueTime == null)
							DueTime = new SLib.LDATETIME(new_date, new SLib.LTIME());
						else
							DueTime.d = new_date;
					}
					result = true;
				}
			}
			return result;
		}
		public boolean DecrementDueDate(boolean checkOnly)
		{
			boolean result = false;
			SLib.LDATETIME nominal_dtm = GetNominalTimestamp();
			if(nominal_dtm != null && SLib.CheckDate(nominal_dtm.d)) {
				if(DueTime != null && SLib.CheckDate(DueTime.d)) {
					if(SLib.LDATE.Difference(DueTime.d, nominal_dtm.d) > 0) {
						SLib.LDATETIME new_dtm = new SLib.LDATETIME(SLib.LDATE.Plus(DueTime.d, -1), DueTime.t);
						if(new_dtm != null && SLib.CheckDate(new_dtm.d)) {
							if(!checkOnly)
								DueTime = new_dtm;
							result = true;
						}
					}
				}
			}
			return result;
		}
		long   ID;
		SLib.LDATETIME CreationTime;
		SLib.LDATETIME Time;
		SLib.LDATETIME DueTime;
		int    InterchangeOpID; // @v11.4.9 OpID-->InterchangeOpID
		int    SvcOpID;   // @v11.4.9 Ид вида операции, определенный на стороне сервиса.
			// Если клиент создает новый документ (в смысле PPObjBill), но не знает точный ид вида операции на
			// стороне сервиса, то определяет InterchangeOpID.
		int    ClientID;  // service-domain-id
		int    DlvrLocID; // service-domain-id
		int    AgentID;   // @v11.4.6 Для документа агентского заказа - ид агента (фактически, он ассоциируется с владельцем нашего устройства)
		int    PosNodeID; // @v11.4.6 Для кассового чека - кассовый узел, к которому он привязывается //
		int    Flags;     // Проекция поля SecTable.Rec.Flags styloqfDocXXX
		int    StatusSurrId; // @v11.5.1 Специальное суррогатное значение, идентифицирующее клиентский статус документа.
			// Значение ссылается на поле BusinessEntity.DocStatus.SurrId.
		double Amount;    // Номинальная сумма документа
		String Code;
		byte [] SvcIdent; // Идентификатор сервиса, с которым связан документ
		UUID Uuid; // Уникальный идентификатор, генерируемый на стороне эмитента
		UUID OrgCmdUuid; // Идентификатор команды сервиса, на основании данных которой сформирован документ.
			// Поле нужно для сопоставления сохраненных документов с данными сервиса.
		String BaseCurrencySymb;
		String Memo;
	}
	//
	// Descr: Возвращает true если статус документа status является завершающим.
	//   По документу с таким статусом любые операции невозможны либо бессмысленны.
	// @todo Потребуются уточнения спецификации!
	//
	public static boolean IsStatusFinished(int status)
	{
		return (status == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstREJECTED ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONACCEPTED ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_SUCC ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_FAIL ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT);
	}
	public static boolean DoesStatusAllowModifications(int status)
	{
		return (status == StyloQDatabase.SecStoragePacket.styloqdocstUNDEF ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT ||
				status == StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC);
	}
	public static final int editactionClose  = 1; // Просто закрыть сеанс редактирования документа (изменения и передача сервису не предполагаются)
	public static final int editactionSubmit = 2; // Подтвердить изменения документа (передача сервису не предполагается)
	public static final int editactionSubmitAndTransmit = 3; // Подтвердить изменения документа с передачей сервису
	public static final int editactionCancelEdition  = 4; // Отменить изменения документа (передача сервису не предполагается)
	public static final int editactionCancelDocument = 5; // Отменить документ с передачей сервису факта отмены
	static class EditAction {
		EditAction(int editaction)
		{
			Action = editaction;
		}
		String GetTitle(StyloQApp appCtx)
		{
			String result = null;
			if(appCtx != null) {
				switch(Action) {
					case editactionClose: // Просто закрыть сеанс редактирования документа (изменения и передача сервису не предполагаются)
						result = appCtx.GetString("but_close");
						break;
					case editactionSubmit: // Подтвердить изменения документа (передача сервису не предполагается)
						result = appCtx.GetString("but_ok");
						break;
					case editactionSubmitAndTransmit: // Подтвердить изменения документа с передачей сервису
						result = appCtx.GetString("but_stq_commitdocument");
						break;
					case editactionCancelEdition: // Отменить изменения документа (передача сервису не предполагается)
						result = appCtx.GetString("but_cancel");
						break;
					case editactionCancelDocument: // Отменить документ с передачей сервису факта отмены
						result = appCtx.GetString("but_stq_canceldocument");
						break;
				}
			}
			return result;
		}
		int    Action;
	}
	public static ArrayList <EditAction> GetEditActionsConnectedWithStatus(int status)
	{
		ArrayList <EditAction> result = new ArrayList<>();
		switch(status) {
			case StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMOD:
				result.add(new EditAction(editactionSubmitAndTransmit));
				result.add(new EditAction(editactionCancelDocument));
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstINCOMINGMODACCEPTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstUNDEF:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstDRAFT:
				result.add(new EditAction(editactionSubmitAndTransmit));
				result.add(new EditAction(editactionCancelDocument));
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC:
				result.add(new EditAction(editactionSubmitAndTransmit));
				result.add(new EditAction(editactionCancelDocument));
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTIONACCEPTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTIONREJECTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstREJECTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstMODIFIED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONACCEPTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONCORRECTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECORRECTIONACCEPTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECORRECTIONREJECTED:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_SUCC:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_FAIL:
				result.add(new EditAction(editactionClose));
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT:
				result.add(new EditAction(editactionClose));
				break;
		}
		return result;
	}
	public static boolean ValidateStatusTransition(int status, int newStatus)
	{
		boolean ok = false;
		switch(status) {
			case StyloQDatabase.SecStoragePacket.styloqdocstUNDEF:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstDRAFT:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstCORRECTED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstREJECTED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstMODIFIED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstEXECUTED);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTIONACCEPTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCORRECTIONREJECTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstREJECTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstMODIFIED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTED:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONACCEPTED ||
						newStatus == StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONCORRECTED);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONACCEPTED:
				ok = (newStatus == StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_SUCC);
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECUTIONCORRECTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECORRECTIONACCEPTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstEXECORRECTIONREJECTED:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_SUCC:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_FAIL:
				break;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT:
				break;
		}
		return ok;
	}
	public static int GetImageResourceByDocStatus(int status)
	{
		switch(status) {
			case StyloQDatabase.SecStoragePacket.styloqdocstFINISHED_SUCC: return R.drawable.ic_styloq_document_finished;
			case StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC: return R.drawable.ic_styloq_document_itrm;
			case StyloQDatabase.SecStoragePacket.styloqdocstDRAFT: return R.drawable.ic_styloq_document_draft;
			case StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED: return R.drawable.ic_styloqdocstatus_approved;
			case StyloQDatabase.SecStoragePacket.styloqdocstREJECTED: return R.drawable.ic_styloqdocstatus_rejected;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLED: return R.drawable.ic_styloqdocstatus_cancelled;
			case StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT: return R.drawable.ic_styloqdocstatus_cancelled; // @todo change icon
		}
		return 0;
	}
	public int GetDocStatus()
	{
		return (H != null) ? StyloQDatabase.SecTable.Rec.GetDocStatus(H.Flags) : 0;
	}
	public boolean SetDocStatus(int s)
	{
		boolean ok = false;
		if(H != null) {
			final int preserve_status = GetDocStatus();
			if(s == preserve_status)
				ok = true;
			else if(((s << 1) & ~StyloQDatabase.SecStoragePacket.styloqfDocStatusFlags) == 0) {
				H.Flags &= ~StyloQDatabase.SecStoragePacket.styloqfDocStatusFlags;
				H.Flags |= ((s << 1) & StyloQDatabase.SecStoragePacket.styloqfDocStatusFlags);
				ok = true;
			}
		}
		return ok;
	}
	//
	// Descr: Специализированная структура используемая как элемент
	//   списка документов для отображения. Одного Head недостаточно: бывает нужно
	//   отображать данные, находящиеся в детализирующих списках (TransferItem, BookingItem).
	//
	public static class DisplayEntry {
		public DisplayEntry()
		{
			H = null;
			SingleBkItem = null;
		}
		public DisplayEntry(Document doc)
		{
			H = doc.H;
			if(doc.BkList != null && doc.BkList.size() == 1) {
				SingleBkItem = doc.BkList.get(0);
			}
		}
		public SLib.LDATE GetNominalDate() { return (H != null) ? H.GetNominalDate() : null; }
		Head   H;
		BookingItem SingleBkItem;
	}
	public static class LotExtCode {
		LotExtCode()
		{
			Flags = 0;
			BoxRefN = 0;
			Code = null;
		}
		LotExtCode(String mark)
		{
			Flags = 0;
			BoxRefN = 0;
			Code = mark;
		}
		public static LotExtCode Copy(final LotExtCode s)
		{
			LotExtCode copy = null;
			if(s != null) {
				copy = new LotExtCode();
				copy.Flags = s.Flags;
				copy.BoxRefN = s.BoxRefN;
				copy.Code = SLib.Copy(s.Code);
			}
			return copy;
		}
		public static ArrayList <LotExtCode> Copy(final ArrayList <LotExtCode> s)
		{
			ArrayList <LotExtCode> copy = null;
			if(s != null) {
				copy = new ArrayList<LotExtCode>();
				for(LotExtCode iter : s)
					copy.add(Copy(iter));
			}
			return copy;
		}
		boolean IsEq(final LotExtCode s)
		{
			return (s != null && Flags == s.Flags && BoxRefN == s.BoxRefN && SLib.AreStringsEqual(Code, s.Code));
		}
		int    Flags;
		int    BoxRefN;
		String Code;
	}
	// Так как одна строка может иметь более одного набора значений {qtty, cost, price},
	// то выделяем такой набор в отдельную структуру.
	public static class ValuSet {
		public static boolean ArEq(final ValuSet a1, final ValuSet a2)
		{
			return (a1 != null) ? (a2 != null && a1.IsEq(a2)) : (a2 == null);
		}
		public ValuSet()
		{
			Qtty = 0.0;
			Cost = 0.0;
			Price = 0.0;
			Discount = 0.0;
		}
		public static ValuSet Copy(final ValuSet s)
		{
			ValuSet copy = null;
			if(s != null) {
				copy = new ValuSet();
				copy.Qtty = s.Qtty;
				copy.Cost = s.Cost;
				copy.Price = s.Price;
				copy.Discount = s.Discount;
			}
			return copy;
		}
		boolean IsEq(final ValuSet s)
		{
			return (s != null && Qtty == s.Qtty && Cost == s.Cost && Price == s.Price && Discount == s.Discount);
		}
		public double GetAmount_Cost() { return (Qtty * Cost); }
		public double GetAmount_Price() { return (Qtty * (Price-Discount)); }
		public boolean IsEmpty() { return !(Qtty > 0.0 || Cost > 0.0 || Price > 0.0); }
		double Qtty;
		double Cost;
		double Price;
		double Discount;
	}
	public static class TransferItem {
		public boolean CanMerge(final TransferItem testItem)
		{
			boolean result = false;
			if(testItem != null && testItem.GoodsID == GoodsID)
				result = true;
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
			CcQueue = 0; // @v11.5.2
			Serial = null; // @v11.5.2
			Set = new ValuSet();
			SetAccepted = null; // @v11.4.8
			XcL = null;
		}
		public static TransferItem Copy(final TransferItem s)
		{
			TransferItem copy = null;
			if(s != null) {
				copy = new TransferItem();
				copy.RowIdx = s.RowIdx;
				copy.GoodsID = s.GoodsID;
				copy.UnitID = s.UnitID;
				copy.Flags = s.Flags;
				copy.CcQueue = s.CcQueue; // @v11.5.2
				copy.Serial = s.Serial; // @v11.5.2
				copy.Set = ValuSet.Copy(s.Set);
				copy.SetAccepted = ValuSet.Copy(s.SetAccepted);
				copy.XcL = LotExtCode.Copy(s.XcL);
			}
			return copy;
		}
		boolean IsEq(final TransferItem s)
		{
			boolean yes = true;
			if(!(s != null && RowIdx == s.RowIdx && GoodsID == s.GoodsID && UnitID == s.UnitID && Flags == s.Flags && CcQueue == s.CcQueue && ValuSet.ArEq(Set, s.Set)))
				yes = false;
			else if(!SLib.AreStringsEqual(Serial, s.Serial)) // @v11.5.2
				yes = false;
			else {
				if(XcL != null) {
					if(s.XcL != null) {
						final int c1 = XcL.size();
						final int c2 = s.XcL.size();
						if(c1 != c2)
							yes = false;
						else if(c1 > 0) {
							for(int i = 0; yes && i < c1; i++) {
								final LotExtCode i1 = XcL.get(i);
								final LotExtCode i2 = s.XcL.get(i);
								if(i1 != null) {
									if(i2 == null || !i1.IsEq(i2))
										yes = false;
								}
								else if(i2 != null)
									yes = false;
							}
						}
					}
					else
						yes = false;
				}
				else if(s.XcL != null)
					yes = false;
			}
			return yes;
		}
		double GetAmount_Cost() { return (Set != null) ? Set.GetAmount_Cost() : 0.0; }
		double GetAmount_Price() { return (Set != null) ? Set.GetAmount_Price() : 0.0; }
		//
		// Descr: Битовые флаги, выставляемые в поле Flags {
		//
		// Следующие 5 флагов - проекция флагов CCheckPacket::LineExt::fXXX
		// Хотя их значения совпадают с прототипами, это - не существенно и для других флагов может быть не так.
		//
		public static int fCcGroup         = 0x0001;
		public static int fCcModifier      = 0x0002;
		public static int fCcPartOfComplex = 0x0004;
		public static int fCcQuotedByGift  = 0x0008;
		public static int fCcFixedPrice    = 0x0010;
		// }
		int    RowIdx;  // [1..]
		int    GoodsID; // service-domain-id
		int    UnitID;  // service-domain-id
		int    Flags;
		int    CcQueue; // @v11.5.2 Проекция поля CCheckPacket::LineExt::Queue
		String Serial; // @v11.5.2
		ValuSet Set;
		ValuSet SetAccepted; // @v11.4.8 Набор величин, с которыми согласна принимающая сторона (например при инвентаризации, приемке приходного документа и т.д.)
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
		public static BookingItem Copy(final BookingItem s)
		{
			BookingItem copy = null;
			if(s != null) {
				copy = new BookingItem();
				copy.RowIdx = s.RowIdx;
				copy.PrcID = s.PrcID;
				copy.GoodsID = s.GoodsID;
				copy.Flags = s.Flags;
				copy.ReqTime = SLib.LDATETIME.Copy(s.ReqTime);
				copy.EstimatedDurationSec = s.EstimatedDurationSec;
				copy.Set = ValuSet.Copy(s.Set);
				copy.Memo = SLib.Copy(s.Memo);
			}
			return copy;
		}
		boolean IsEq(final BookingItem s)
		{
			boolean yes = true;
			if(!(s != null && RowIdx == s.RowIdx && PrcID == s.PrcID && GoodsID == s.GoodsID && Flags == s.Flags &&
				EstimatedDurationSec == s.EstimatedDurationSec && ValuSet.ArEq(Set, s.Set)))
				yes = false;
			else if(SLib.AreStringsEqual(Memo, s.Memo)) {
				yes = false;
			}
			else if(!SLib.LDATETIME.ArEq(ReqTime, s.ReqTime))
				yes = false;
			return yes;
		}
		double GetAmount_Price()
		{
			double result = 0.0;
			if(Set != null) {
				result = (Set.Qtty > 0.0) ? (Set.Price * Set.Qtty) : Set.Price;
			}
			return result;
		}
		SLib.STimeChunk GetEsimatedTimeChunk()
		{
			SLib.STimeChunk result = null;
			if(ReqTime != null && SLib.CheckDate(ReqTime.d)) {
				result = new SLib.STimeChunk();
				result.Start = ReqTime;
				if(EstimatedDurationSec >= 0)
					result.Finish = SLib.plusdatetimesec(ReqTime, EstimatedDurationSec);
				else
					result.Finish = ReqTime;
			}
			return result;
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
		AfterTransmitStatus = 0;
		DetailExpandStatus_Ti = 0;
		H = null;
		TiList = null;
		BkList = null;
		VXcL = null;
		XcL_Unassigned = null;
	}
	Document(int interchangeOpID, final byte [] svcIdent, StyloQApp appCtx) throws StyloQException
	{
		AfterTransmitStatus = 0;
		DetailExpandStatus_Ti = 0;
		H = null;
		TiList = null;
		BkList = null;
		VXcL = null;
		XcL_Unassigned = null;
		if(interchangeOpID > 0) {
			H = new Document.Head();
			H.CreationTime = new SLib.LDATETIME(System.currentTimeMillis());
			H.InterchangeOpID = interchangeOpID;
			H.SvcIdent = svcIdent; // @v11.4.1 @fix
			SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT); // Новый док автоматом является draft-документом
			H.Uuid = UUID.randomUUID();
			if(SLib.GetLen(svcIdent) > 0 && appCtx != null) {
				StyloQDatabase db = appCtx.GetDB();
				if(db != null)
					H.Code = db.MakeDocumentCode(svcIdent);
			}
		}
	}
	public static Document Copy(final Document s)
	{
		Document copy = null;
		if(s != null) {
			copy = new Document();
			copy.H = Head.Copy(s.H);
			{
				if(s.TiList != null) {
					copy.TiList = new ArrayList<TransferItem>();
					for(TransferItem iter : s.TiList)
						copy.TiList.add(TransferItem.Copy(iter));
				}
				else
					copy.TiList = null;
			}
			{
				if(s.BkList != null) {
					copy.BkList = new ArrayList<BookingItem>();
					for(BookingItem iter : s.BkList)
						copy.BkList.add(BookingItem.Copy(iter));
				}
				else
					copy.BkList = null;
			}
			copy.VXcL = LotExtCode.Copy(s.VXcL);
			copy.XcL_Unassigned = LotExtCode.Copy(s.XcL_Unassigned);
			copy.AfterTransmitStatus = s.AfterTransmitStatus;
			copy.DetailExpandStatus_Ti = s.DetailExpandStatus_Ti;
		}
		return copy;
	}
	boolean IsEq(final Document s)
	{
		boolean yes = true;
		if(!(s != null && Head.AreEq(H, s.H)))
			yes = false;
		else {
			if(TiList != null) {
				if(s.TiList != null) {
					final int c1 = TiList.size();
					final int c2 = s.TiList.size();
					if(c1 != c2)
						yes = false;
					else if(c1 > 0) {
						for(int i = 0; yes && i < c1; i++) {
							final TransferItem i1 = TiList.get(i);
							final TransferItem i2 = s.TiList.get(i);
							if(i1 != null) {
								if(i2 == null || !i1.IsEq(i2))
									yes = false;
							}
							else if(i2 != null)
								yes = false;
						}
					}
				}
				else
					yes = false;
			}
			else if(s.TiList != null)
				yes = false;
			if(yes) {
				if(BkList != null) {
					if(s.BkList != null) {
						final int c1 = BkList.size();
						final int c2 = s.BkList.size();
						if(c1 != c2)
							yes = false;
						else if(c1 > 0) {
							for(int i = 0; yes && i < c1; i++) {
								final BookingItem i1 = BkList.get(i);
								final BookingItem i2 = s.BkList.get(i);
								if(i1 != null) {
									if(i2 == null || !i1.IsEq(i2))
										yes = false;
								}
								else if(i2 != null)
									yes = false;
							}
						}
					}
					else
						yes = false;
				}
				else if(s.BkList != null)
					yes = false;
			}
			if(yes) {
				if(VXcL != null) {
					if(s.VXcL != null) {
						final int c1 = VXcL.size();
						final int c2 = s.VXcL.size();
						if(c1 != c2)
							yes = false;
						else if(c1 > 0) {
							for(int i = 0; yes && i < c1; i++) {
								final LotExtCode i1 = VXcL.get(i);
								final LotExtCode i2 = s.VXcL.get(i);
								if(i1 != null) {
									if(i2 == null || !i1.IsEq(i2))
										yes = false;
								}
								else if(i2 != null)
									yes = false;
							}
						}
					}
					else
						yes = false;
				}
				else if(s.VXcL != null)
					yes = false;
			}
		}
		return yes;
	}
	Document Z()
	{
		H = null;
		TiList = null;
		BkList = null;
		VXcL = null;
		XcL_Unassigned = null;
		return this;
	}
	public SLib.LDATE GetNominalDate() { return (H != null) ? H.GetNominalDate() : null; }
	int GetAfterTransmitStatus()
	{
		return AfterTransmitStatus;
	}
	boolean SetAfterTransmitStatus(int s)
	{
		if(((s << 1) & ~StyloQDatabase.SecStoragePacket.styloqfDocStatusFlags) == 0)
			AfterTransmitStatus = s;
		return true;
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
				final TransferItem ti = TiList.get(i);
				if(ti != null)
					amount += ti.GetAmount_Price();
			}
		}
		if(BkList != null) {
			for(int i = 0; i < BkList.size(); i++) {
				final BookingItem bi = BkList.get(i);
				if(bi != null)
					amount += bi.GetAmount_Price();
			}
		}
		return amount;
	}
	//
	// Следующие две функции (GetGoodsMarkSettingListCount и GetGoodsMarkSettingListItem)
	// предназначены для управления отображением списка при присвоении марок позициям документа.
	//
	int GetGoodsMarkSettingListCount()
	{
		int result = 0;
		if(XcL_Unassigned != null && XcL_Unassigned.size() > 0)
			result++;
		if(TiList != null) {
			for(int i = 0; i < TiList.size(); i++) {
				final TransferItem ti = TiList.get(i);
				if(ti != null && ti.XcL != null && ti.XcL.size() > 0)
					result++;
			}
		}
		return result;
	}
	public static class GoodsMarkSettingEntry {
		GoodsMarkSettingEntry(ArrayList<LotExtCode> xcl, TransferItem ti)
		{
			XcL = xcl;
			Ti = ti;
		}
		final ArrayList<LotExtCode> XcL;
		final TransferItem Ti;
	}
	GoodsMarkSettingEntry GetGoodsMarkSettingListItem(int idx)
	{
		GoodsMarkSettingEntry result = null;
		int iter_idx = 0;
		if(XcL_Unassigned != null && XcL_Unassigned.size() > 0) {
			if(iter_idx == idx) {
				result = new GoodsMarkSettingEntry(XcL_Unassigned, null);
			}
			iter_idx++;
		}
		if(result == null) {
			if(TiList != null) {
				for(int i = 0; result == null && i < TiList.size(); i++) {
					final TransferItem ti = TiList.get(i);
					if(ti != null && ti.XcL != null && ti.XcL.size() > 0) {
						if(iter_idx == idx) {
							result = new GoodsMarkSettingEntry(ti.XcL, ti);
						}
						iter_idx++;
					}
				}
			}
		}
		return result;
	}
	int AssignGoodsMark(String mark, final ArrayList</*BusinessEntity.Goods*/CommonPrereqModule.WareEntry> goodsRefList) throws StyloQException
	{
		int ok = -1;
		GTIN m = GTIN.ParseChZnCode(mark, 0);
		SLib.THROW(m != null && m.GetChZnParseResult() > 0, ppstr2.PPERR_TEXTISNTCHZNMARK, mark);
		String gtin14 = m.GetToken(GTIN.fldGTIN14);
		final int gtin14_len = SLib.GetLen(gtin14);
		SLib.THROW(gtin14_len > 0, ppstr2.PPERR_TEXTISNTCHZNMARK, mark);
		assert(gtin14_len == 14);
		final int fmr = FindMark(mark);
		SLib.THROW(fmr < 0 || fmr == 10000/*верифицирующий пул*/, ppstr2.PPERR_DUPEXTLOTCODEINBILL, mark);
		String pattern = gtin14.substring(1);
		if(goodsRefList != null && goodsRefList.size() > 0 && TiList != null && TiList.size() > 0) {
			for(int tiidx = 0; ok < 0 && tiidx < TiList.size(); tiidx++) {
				TransferItem ti = TiList.get(tiidx);
				if(ti != null && ti.GoodsID > 0) {
					for(int gidx = 0; gidx < goodsRefList.size(); gidx++) {
						CommonPrereqModule.WareEntry _we = goodsRefList.get(gidx);
						BusinessEntity.Goods goods_item = (_we != null) ? goodsRefList.get(gidx).Item : null;
						if(goods_item != null && goods_item.ID == ti.GoodsID) {
							BusinessEntity.GoodsCode fc = goods_item.SearchCode(pattern);
							if(fc != null) {
								if(ti.XcL == null) {
									ti.XcL = new ArrayList<LotExtCode>();
									ti.XcL.add(new LotExtCode(mark));
									ok = 1;
								}
								else {
									final LotExtCode ex_lec = FindLotExtCode(ti.XcL, mark);
									SLib.THROW(ex_lec == null, ppstr2.PPERR_DUPEXTLOTCODEINBILL, mark);
									ti.XcL.add(new LotExtCode(mark));
									ok = 1;
								}
							}
						}
					}
				}
			}
		}
		if(ok < 0) {
			if(XcL_Unassigned == null) {
				XcL_Unassigned = new ArrayList<LotExtCode>();
				XcL_Unassigned.add(new LotExtCode(mark));
				ok = 2;
			}
			else {
				final LotExtCode ex_lec = FindLotExtCode(XcL_Unassigned, mark);
				SLib.THROW(ex_lec == null, ppstr2.PPERR_DUPEXTLOTCODEINBILL, mark);
				XcL_Unassigned.add(new LotExtCode(mark));
				ok = 2;
			}
		}
		return ok;
	}
	static LotExtCode FindLotExtCode(final ArrayList<LotExtCode> list, final String mark)
	{
		LotExtCode result = null;
		final int mark_len = SLib.GetLen(mark);
		if(mark_len > 0 && list != null && list.size() > 0) {
			String pattern = GTIN.PreprocessChZnCode(mark);
			for(int i = 0; result == null && i < list.size(); i++) {
				final LotExtCode item = list.get(i);
				if(item != null && item.Code != null && pattern.equalsIgnoreCase(GTIN.PreprocessChZnCode(item.Code)))
					result = item;
			}
		}
		return result;
	}
	//
	// Descr: Ищет марку среди всех контейнеров документа.
	//   Если марка найдена, то возвращает значение >= 0, иначе -1.
	//
	int FindMark(String mark)
	{
		int    result = -1;
		if(SLib.GetLen(mark) > 0) {
			{
				final int _c = SLib.GetCount(TiList);
				for(int i = 0; result < 0 && i < _c; i++) {
					final TransferItem ti = TiList.get(i);
					if(ti != null) {
						if(FindLotExtCode(ti.XcL, mark) != null)
							result = i;
					}
				}
			}
			if(result < 0 && FindLotExtCode(VXcL, mark) != null)
				result = 10000;
			else if(result < 0 && FindLotExtCode(XcL_Unassigned, mark) != null)
				result = 20000;
		}
		return result;
	}
	//
	// Descr: Статусы кодов маркировки (в пуле проверки по отношению к существующим в пакете или
	// существующих в пакете по отношению к пулу проверки)
	//
	public enum GoodsMarkStatus {
		Unkn, // Неизвестный
		Matched, // Проверяемая марка сопоставлена одной из марок в пакете, либо марка из пакета
			// имеет соответствующую пару в пуле проверки.
		Unmatched, // Проверяемая марка не может быть сопоставлена ни одной из марок в пакете
		Absent // Марка из пакета отсутствует в пуле проверки
	}
	GoodsMarkStatus GetVerificationGoodsMarkStatus(String mark)
	{
		GoodsMarkStatus result = GoodsMarkStatus.Unmatched;
		final int mark_len = SLib.GetLen(mark);
		if(mark_len > 0 && TiList != null && TiList.size() > 0) {
			for(int i = 0; result == GoodsMarkStatus.Unmatched && i < TiList.size(); i++) {
				final TransferItem ti = TiList.get(i);
				final LotExtCode lec = (ti != null) ? FindLotExtCode(ti.XcL, mark) : null;
				if(lec != null)
					result = GoodsMarkStatus.Matched;
			}
		}
		return result;
	}
	GoodsMarkStatus GetInnerGoodsMarkStatus(String mark)
	{
		final LotExtCode lec = FindLotExtCode(VXcL, mark);
		return (lec != null) ? GoodsMarkStatus.Matched : GoodsMarkStatus.Absent;
	}
	//
	// Descr: Выполняет завершающие операции над документом, включающие
	//   внутрениие расчеты и проверку инвариантов.
	//
	boolean Finalize()
	{
		boolean ok = true;
		if(H != null) {
			// @v11.4.6 {
			if(H.Time == null || !SLib.CheckDate(H.Time.d)) {
				H.Time = new SLib.LDATETIME(System.currentTimeMillis());
			}
			// } @v11.4.6
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
				if(H.Uuid != null)
					result.put("uuid", H.Uuid.toString());
				if(H.OrgCmdUuid != null) // @v11.4.0
					result.put("orgcmduuid", H.OrgCmdUuid.toString());
				if(SLib.GetLen(H.Code) > 0)
					result.put("code", H.Code);
				if(SLib.GetLen(H.SvcIdent) > 0) {
					String svc_ident_hex = Base64.getEncoder().encodeToString(H.SvcIdent);
					if(SLib.GetLen(svc_ident_hex) > 0)
						result.put("svcident", svc_ident_hex);
				}
				if(SLib.GetLen(H.BaseCurrencySymb) > 0) {
					result.put("basecurrency", H.BaseCurrencySymb);
				}
				if(H.CreationTime != null)
					result.put("crtm", SLib.datetimefmt(H.CreationTime, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				if(H.Time != null)
					result.put("tm", SLib.datetimefmt(H.Time, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				if(H.DueTime != null)
					result.put("duetm", SLib.datetimefmt(H.DueTime, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
				result.put("svcopid", H.SvcOpID);
				result.put("icopid", H.InterchangeOpID);
				if(H.PosNodeID > 0) { // @v11.4.6
					result.put("posnodeid", H.PosNodeID);
				}
				if(H.AgentID > 0) { // @v11.4.6
					result.put("agentid", H.AgentID);
				}
				if(H.ClientID > 0) {
					result.put("cliid", H.ClientID);
				}
				if(H.DlvrLocID > 0) {
					result.put("dlvrlocid", H.DlvrLocID);
				}
				if(H.StatusSurrId > 0) { // @v11.5.1
					result.put("statussurrid", H.StatusSurrId);
				}
				// @v11.4.7 {
				if(H.Amount > 0)
					result.put("amount", H.Amount);
				// } @v11.4.7
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
								// @v11.4.8 {
								if(ti.XcL != null && ti.XcL.size() > 0) {
									JSONArray js_xcl = null;
									for(int xci = 0; xci < ti.XcL.size(); xci++) {
										JSONObject js_xce = new JSONObject();
										LotExtCode xce = ti.XcL.get(xci);
										if(xce != null && SLib.GetLen(xce.Code) > 0) {
											js_xce.put("cod", xce.Code);
											if(xce.BoxRefN > 0)
												js_xce.put("boxrefn", xce.BoxRefN);
											if(xce.Flags != 0)
												js_xce.put("flags", xce.Flags);
											if(js_xcl == null)
												js_xcl = new JSONArray();
											js_xcl.put(js_xce);
										}
									}
									if(js_xcl != null)
										js_item.put("xcl", js_xcl);
								}
								// } @v11.4.8
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
				// @v11.4.8 {
				if(VXcL != null && VXcL.size() > 0) {
					JSONArray js_xcl = null;
					for(int xci = 0; xci < VXcL.size(); xci++) {
						JSONObject js_xce = new JSONObject();
						LotExtCode xce = VXcL.get(xci);
						if(xce != null && SLib.GetLen(xce.Code) > 0) {
							js_xce.put("cod", xce.Code);
							if(xce.BoxRefN > 0)
								js_xce.put("boxrefn", xce.BoxRefN);
							if(xce.Flags != 0)
								js_xce.put("flags", xce.Flags);
							if(js_xcl == null)
								js_xcl = new JSONArray();
							js_xcl.put(js_xce);
						}
					}
					if(js_xcl != null)
						result.put("vxcl", js_xcl);
				}
				// } @v11.4.8
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
								js_item.put("memo", bi.Memo);
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
	boolean FromJsonObj(JSONObject jsObj)
	{
		boolean result = false;
		Z();
		if(jsObj != null) {
			try {
				H = new Head();
				H.ID = jsObj.optLong("id", 0);
				// @v11.4.0 {
				H.Uuid = SLib.strtouuid(jsObj.optString("uuid", null));
				H.OrgCmdUuid = SLib.strtouuid(jsObj.optString("orgcmduuid", null));
				// } @v11.4.0
				//SimpleDateFormat sdf = new SimpleDateFormat("yyyy-mm-dd'T'HH:mm:ss", Locale.getDefault());
				H.CreationTime = SLib.strtodatetime(jsObj.optString("crtm", null), SLib.DATF_ISO8601, SLib.TIMF_HMS);
				H.Time = SLib.strtodatetime(jsObj.optString("tm", null), SLib.DATF_ISO8601, SLib.TIMF_HMS);
				H.DueTime = SLib.strtodatetime(jsObj.optString("duetm", null), SLib.DATF_ISO8601, SLib.TIMF_HMS);
				H.SvcOpID = jsObj.optInt("svcopid", 0);
				H.InterchangeOpID = jsObj.optInt("icopid", 0);
				H.ClientID = jsObj.optInt("cliid", 0);
				H.PosNodeID = jsObj.optInt("posnodeid", 0);
				H.AgentID = jsObj.optInt("agentid", 0);
				H.DlvrLocID = jsObj.optInt("dlvrlocid", 0);
				H.StatusSurrId = jsObj.optInt("statussurrid", 0); // @v11.5.1
				{
					String svc_ident_hex = jsObj.optString("svcident", null);
					if(SLib.GetLen(svc_ident_hex) > 0)
						H.SvcIdent = Base64.getDecoder().decode(svc_ident_hex);
				}
				{
					String base_currency_symb = jsObj.optString("basecurrency", null);
					if(SLib.GetLen(base_currency_symb) > 0)
						H.BaseCurrencySymb = base_currency_symb;
				}
				H.Code = jsObj.optString("code", null);
				H.Amount = jsObj.optDouble("amount", 0.0); // @v11.4.7
				H.Memo = jsObj.optString("memo", null);
				JSONArray js_ti_list = jsObj.optJSONArray("ti_list");
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
							// @v11.4.8 {
							{
								JSONArray js_xcl = js_item.optJSONArray("xcl");
								if(js_xcl != null && js_xcl.length() > 0) {
									for(int xci = 0; xci < js_xcl.length(); xci++) {
										JSONObject js_xce = js_xcl.optJSONObject(xci);
										if(js_xce != null) {
											LotExtCode xce = new LotExtCode();
											xce.Code = js_xce.optString("cod", null);
											if(SLib.GetLen(xce.Code) > 0) {
												xce.BoxRefN = js_xce.optInt("boxrefn", 0);
												xce.Flags = js_xce.optInt("flags", 0);
												if(ti.XcL == null)
													ti.XcL = new ArrayList<LotExtCode>();
												ti.XcL.add(xce);
											}
										}
									}
								}
							}
							// } @v11.4.8
							if(TiList == null)
								TiList = new ArrayList<TransferItem>();
							TiList.add(ti);
						}
					}
				}
				// @v11.4.8 {
				{
					JSONArray js_xcl = jsObj.optJSONArray("vxcl");
					if(js_xcl != null && js_xcl.length() > 0) {
						for(int xci = 0; xci < js_xcl.length(); xci++) {
							JSONObject js_xce = js_xcl.optJSONObject(xci);
							if(js_xce != null) {
								LotExtCode xce = new LotExtCode();
								xce.Code = js_xce.optString("cod", null);
								if(SLib.GetLen(xce.Code) > 0) {
									xce.BoxRefN = js_xce.optInt("boxrefn", 0);
									xce.Flags = js_xce.optInt("flags", 0);
									if(VXcL == null)
										VXcL = new ArrayList<LotExtCode>();
									VXcL.add(xce);
								}
							}
						}
					}
				}
				// } @v11.4.8
				JSONArray js_bk_list = jsObj.optJSONArray("bk_list");
				if(js_bk_list != null && js_bk_list.length() > 0) {
					for(int i = 0; i < js_bk_list.length(); i++) {
						JSONObject js_item = js_bk_list.optJSONObject(i);
						if(js_item != null) {
							BookingItem bi = new BookingItem();
							bi.RowIdx = js_item.optInt("rowid", 0);
							bi.PrcID = js_item.optInt("prcid", 0);
							bi.GoodsID = js_item.optInt("goodsid", 0);
							bi.Flags = js_item.optInt("flags", 0);
							// @todo Не все поля считаны!
							{
								String req_time = js_item.optString("reqtime", null);
								bi.ReqTime = SLib.strtodatetime(req_time, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0);
								bi.EstimatedDurationSec = js_item.optInt("estimateddurationsec", 0);
								JSONObject js_set = js_item.optJSONObject("set");
								if(js_set != null) {
									bi.Set = new ValuSet();
									bi.Set.Qtty = js_set.optDouble("qtty", 0.0);
									bi.Set.Cost = js_set.optDouble("cost", 0.0);
									bi.Set.Price = js_set.optDouble("price", 0.0);
									bi.Set.Discount = js_set.optDouble("discount", 0.0);
								}
								bi.Memo = js_item.optString("memo", null);
							}
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
	boolean FromJson(String jsText)
	{
		boolean result = false;
		Z();
		if(SLib.GetLen(jsText) > 0) {
			try {
				JSONObject jsobj = new JSONObject(jsText);
				result = FromJsonObj(jsobj);
			} catch(JSONException exn) {
				result = false;
			}
		}
		return result;
	}
}
