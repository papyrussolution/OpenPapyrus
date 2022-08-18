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

	public static int IncomingListActionsFromString(final String input)
	{
		int    result = 0;
		if(SLib.GetLen(input) > 0) {
			StringTokenizer toknzr = new StringTokenizer(input, ",");
			int _c = toknzr.countTokens();
			for(int i = 0; i < _c; i++) {
				String tok = toknzr.nextToken();
				tok.trim();
				if(tok.equalsIgnoreCase("docstatus"))
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
			}
		}
		return result;
	}

	Head H;
	ArrayList <TransferItem> TiList;
	ArrayList <BookingItem> BkList; // Список позиций повременных элементов, связанных с процессорами
	ArrayList <LotExtCode> VXcL; // Валидирующий контейнер спецкодов. Применяется для проверки кодов, поступивших с документом в XcL
	private int AfterTransmitStatus; // @transient Флаги статуса документа, которые должны быть установлены в БД после успешной отправки сервису
	public int DetailExpandStatus_Ti; // @transient

	public static class Head {
		public static boolean ArEq(final Head a1, final Head a2)
		{
			return (a1 != null) ? (a2 != null && a1.IsEq(a2)) : (a2 == null);
		}
		Head()
		{
			ID = 0;
			CreationTime = null;
			Time = null;
			DueTime = null;
			OpID = 0;
			AgentID = 0; // @v11.4.6
			PosNodeID = 0; // @v11.4.6
			ClientID = 0;
			DlvrLocID = 0;
			Flags = 0;
			Amount = 0;
			Code = null;
			SvcIdent = null;
			Uuid = null;
			OrgCmdUuid = null;
			BaseCurrencySymb = null;
			Memo = null;
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
		int    OpID;
		int    ClientID;  // service-domain-id
		int    DlvrLocID; // service-domain-id
		int    AgentID;   // @v11.4.6 Для документа агентского заказа - ид агента (фактически, он ассоциируется с владельцем нашего устройства)
		int    PosNodeID; // @v11.4.6 Для кассового чека - кассовый узел, к которому он привязывается //
		int    Flags;     // Проекция поля SecTable.Rec.Flags styloqfDocXXX
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
		boolean IsEq(final ValuSet s)
		{
			return (s != null && Qtty == s.Qtty && Cost == s.Cost && Price == s.Price && Discount == s.Discount);
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
			Set = new ValuSet();
			SetAccepted = null; // @v11.4.8
			XcL = null;
		}
		boolean IsEq(final TransferItem s)
		{
			boolean yes = true;
			if(!(s != null && RowIdx == s.RowIdx && GoodsID == s.GoodsID && UnitID == s.UnitID && Flags == s.Flags && ValuSet.ArEq(Set, s.Set)))
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
		int    RowIdx;  // [1..]
		int    GoodsID; // service-domain-id
		int    UnitID;  // service-domain-id
		int    Flags;
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
	}
	Document(int opID, final byte [] svcIdent, StyloQApp appCtx) throws StyloQException
	{
		TiList = null;
		BkList = null;
		VXcL = null;
		if(opID > 0) {
			H = new Document.Head();
			H.CreationTime = new SLib.LDATETIME(System.currentTimeMillis());
			H.OpID = opID;
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
	boolean IsEq(final Document s)
	{
		boolean yes = true;
		if(!(s != null && Head.ArEq(H, s.H)))
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
				result.put("opid", H.OpID);
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
				H.OpID = jsObj.optInt("opid", 0);
				H.ClientID = jsObj.optInt("cliid", 0);
				H.PosNodeID = jsObj.optInt("posnodeid", 0);
				H.AgentID = jsObj.optInt("agentid", 0);
				H.DlvrLocID = jsObj.optInt("dlvrlocid", 0);
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
								if(SLib.GetLen(req_time) > 0) {
									bi.ReqTime = SLib.strtodatetime(req_time, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0);
								}
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
