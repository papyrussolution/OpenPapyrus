// CommonPrereqModule.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.style.BackgroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.nio.charset.StandardCharsets;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Currency;
import java.util.UUID;

public class CommonPrereqModule {
	public byte[] SvcIdent; // Получает через intent ("SvcIdent")
	public String CmdName; // Получает через intent ("CmdName")
	public String CmdDescr; // Получает через intent ("CmdDescr")
	public UUID CmdUuid;  // Получает через intent ("CmdUuid")
	public ArrayList<SimpleSearchIndexEntry> SimpleSearchIndex;
	public SimpleSearchResult SearchResult;
	public ArrayList <CliEntry> CliListData;
	public ArrayList<JSONObject> GoodsGroupListData;
	public GoodsFilt Gf;
	private String BaseCurrencySymb;
	private int AgentID; // Если исходный документ для формирования заказов ассоциирован
		// с агентом, то в этом поле устанавливается id этого агента (field agentid)
	public ArrayList <WareEntry> GoodsListData;
	public ArrayList <CommonPrereqModule.TabEntry> TabList;
	public ArrayList /*<Document.Head>*/<Document.DisplayEntry> OrderHList;
	public ArrayList<ProcessorEntry> ProcessorListData;
	private Document CurrentOrder;
	protected boolean Locker_CommitCurrentDocument;
	private SLib.SlActivity ActivityInstance;
	private int ViewPagerResourceId;
	private int TabLayoutResourceId;
	public static class CliEntry {
		CliEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			AddrExpandStatus = 0;
			if(JsItem != null) {
				JSONArray dlvr_loc_list = JsItem.optJSONArray("dlvrloc_list");
				if(dlvr_loc_list != null && dlvr_loc_list.length() > 0)
					AddrExpandStatus = 1;
			}
		}
		public ArrayList <JSONObject> GetDlvrLocListAsArray()
		{
			ArrayList <JSONObject> result = null;
			JSONArray dlvr_loc_list = JsItem.optJSONArray("dlvrloc_list");
			if(dlvr_loc_list != null && dlvr_loc_list.length() > 0) {
				result = new ArrayList<JSONObject>();
				try {
					for(int i = 0; i < dlvr_loc_list.length(); i++) {
						Object dlvr_loc_list_item_obj = dlvr_loc_list.get(i);
						if(dlvr_loc_list_item_obj != null && dlvr_loc_list_item_obj instanceof JSONObject)
							result.add((JSONObject)dlvr_loc_list_item_obj);
					}
				} catch(JSONException exn) {
					result = null;
				}
			}
			return result;
		}
		int   AddrExpandStatus; // 0 - no addressed, 1 - addresses collapsed, 2 - addresses expanded
		JSONObject JsItem;
	}
	public enum Tab {
		tabUndef,
		tabGoodsGroups,
		tabBrands,
		tabGoods,
		tabClients,
		tabProcessors,
		tabAttendance,
		tabCurrentOrder,
		tabBookingDocument,
		tabOrders,
		tabSearch
	}
	public static class TabEntry {
		TabEntry(CommonPrereqModule.Tab id, String text, /*View*/SLib.SlFragmentStatic view)
		{
			TabId = id;
			TabText = text;
			TabView = view;
		}
		CommonPrereqModule.Tab TabId;
		String TabText;
		/*View*/SLib.SlFragmentStatic TabView;
	}
	private StyloQApp GetAppCtx()
	{
		return (ActivityInstance != null) ? ActivityInstance.GetAppCtx() : null;
	}
	// sqbdtSvcReq
	protected final boolean IsCurrentDocumentEmpty()
	{
		return (CurrentOrder == null || CurrentOrder.H == null);
	}
	protected final int GetCurrentDocumentTransferListCount()
	{
		return (CurrentOrder != null && CurrentOrder.TiList != null) ? CurrentOrder.TiList.size() : 0;
	}
	protected final Document GetCurrentDocument()
	{
		return CurrentOrder;
	}
	protected final int GetCurrentDocumentBookingListCount()
	{
		return (CurrentOrder != null && CurrentOrder.BkList != null) ? CurrentOrder.BkList.size() : 0;
	}
	protected void InitCurrenDocument(int opID) throws StyloQException
	{
		if(CurrentOrder == null) {
			StyloQApp app_ctx = GetAppCtx();
			if(app_ctx != null) {
				CurrentOrder = new Document(opID, SvcIdent, app_ctx);
				CurrentOrder.H.BaseCurrencySymb = GetBaseCurrencySymb();
				CurrentOrder.H.OrgCmdUuid = CmdUuid;
			}
		}
	}
	protected void ResetCurrentDocument()
	{
		CurrentOrder = null;
	}
	protected boolean LoadDocument(long id)
	{
		boolean ok = false;
		StyloQApp app_ctx = GetAppCtx();
		if(id > 0 && app_ctx != null) {
			try {
				StyloQDatabase db = app_ctx.GetDB();
				if(db != null) {
					StyloQDatabase.SecStoragePacket pack = db.GetPeerEntry(id);
					if(pack != null && pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kDocIncoming || pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kDocOutcoming) {
						if(pack.Pool != null) {
							byte [] rawdata = pack.Pool.Get(SecretTagPool.tagRawData);
							if(SLib.GetLen(rawdata) > 0) {
								String txt_rawdata = new String(rawdata);
								if(SLib.GetLen(txt_rawdata) > 0) {
									Document rd = new Document();
									if(rd.FromJson(txt_rawdata)) {
										rd.H.Flags = pack.Rec.Flags;
										// На этапе разработки было множество проблем, по этому,
										// вероятно расхождение между идентификаторами в json и в заголовке записи.
										if(rd.H.ID != pack.Rec.ID)
											rd.H.ID = pack.Rec.ID;
										if(CurrentOrder != null) {
											StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT, StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
										}
										CurrentOrder = rd;
										ok = true;
									}
								}
							}
						}
					}
				}
			} catch(StyloQException exn) {
				ok = false;
			}
		}
		return ok;
	}
	protected void RestoreRecentDraftDocumentAsCurrent(/*int opID*/)
	{
		//public StyloQDatabase.SecStoragePacket FindRecentDraftDoc(int docType, long correspondId, byte [] ident, UUID orgCmdUuid)
		if(CurrentOrder == null) {
			StyloQApp app_ctx = GetAppCtx();
			if(app_ctx != null) {
				try {
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null) {
						StyloQDatabase.SecStoragePacket rdd = db.FindRecentDraftDoc(StyloQDatabase.SecStoragePacket.doctypGeneric,
							0, null, CmdUuid);
						if(rdd != null) {
							if(rdd.Pool != null) {
								byte [] rawdata = rdd.Pool.Get(SecretTagPool.tagRawData);
								if(SLib.GetLen(rawdata) > 0) {
									String txt_rawdata = new String(rawdata);
									if(SLib.GetLen(txt_rawdata) > 0) {
										Document rd = new Document();
										if(rd.FromJson(txt_rawdata)) {
											// На этапе разработки было множество проблем, по этому,
											// вероятно расхождение между идентификаторами в json и в заголовке записи.
											if(rd.H.ID != rdd.Rec.ID)
												rd.H.ID = rdd.Rec.ID;
											rd.H.Flags = rdd.Rec.Flags;
											CurrentOrder = rd;
										}
									}
								}
							}
						}
					}
				} catch(StyloQException exn) {
					;
				}
			}
		}
	}
	private boolean StoreCurrentDocument(int reqStatus, int newStatus)
	{
		boolean result = true;
		int    turn_doc_result = -1;
		try {
			StyloQApp app_ctx = GetAppCtx();
			assert(CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null);
			if(app_ctx != null && CurrentOrder != null && CurrentOrder.H != null && CurrentOrder.H.Uuid != null) {
				if(CurrentOrder.GetDocStatus() == 0)
					CurrentOrder.SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
				final int preserve_status = CurrentOrder.GetDocStatus();
				if((reqStatus == 0 || preserve_status == reqStatus) && (newStatus == 0 || CurrentOrder.SetDocStatus(newStatus))) {
					turn_doc_result = 0;
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null && SLib.GetLen(SvcIdent) > 0) {
						StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
						JSONObject jsobj = CurrentOrder.ToJsonObj();
						if(svc_pack != null && jsobj != null) {
							SecretTagPool doc_pool = new SecretTagPool();
							JSONObject js_query = new JSONObject();
							String js_text_doc = jsobj.toString();
							String js_text_docdecl = null;
							{
								JSONObject js_doc_decl = new JSONObject();
								js_doc_decl.put("type", "generic");
								js_doc_decl.put("format", "json");
								js_doc_decl.put("time", SLib.datetimefmt(new SLib.LDATETIME(System.currentTimeMillis()), SLib.DATF_ISO8601 | SLib.DATF_CENTURY, 0));
								js_query.put("declaration", js_doc_decl);
								js_text_docdecl = js_doc_decl.toString();
							}
							//
							// В базе данных мы сохраняем документ в виде "сырого" json (то есть только jsobj)
							// в то время как сервису передаем этот же документ вложенный в объект команды (js_query).
							// Но и то и другое вносится в пул хранения под тегом tagRawData.
							//
							SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
							doc_pool.Put(SecretTagPool.tagRawData, js_text_doc.getBytes(StandardCharsets.UTF_8), ds);
							doc_pool.Put(SecretTagPool.tagDocDeclaration, js_text_docdecl.getBytes(StandardCharsets.UTF_8));
							//
							long svc_id = svc_pack.Rec.ID;
							byte[] doc_ident = db.MakeDocumentStorageIdent(SvcIdent, CurrentOrder.H.Uuid);
							long doc_id = db.PutDocument(+1, StyloQDatabase.SecStoragePacket.doctypGeneric, CurrentOrder.H.Flags, doc_ident, svc_id, doc_pool);
							if(doc_id > 0) {
								assert(CurrentOrder.H.ID == 0 || CurrentOrder.H.ID == doc_id);
								if(CurrentOrder.H.ID == 0 || CurrentOrder.H.ID == doc_id) {
									CurrentOrder.H.ID = doc_id;
									turn_doc_result = 1;
								}
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			turn_doc_result = 0;
		} catch(JSONException exn) {
			turn_doc_result = 0;
		}
		return (turn_doc_result > 0) ? true : ((turn_doc_result == 0) ? false : result);
	}
	private boolean OnCurrentDocumentModification()
	{
		return StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT, StyloQDatabase.SecStoragePacket.styloqdocstDRAFT);
	}
	protected boolean UpdateMemoInCurrentDocument(String memo)
	{
		boolean result = false;
		if(!IsCurrentDocumentEmpty()) {
			int len1 = SLib.GetLen(memo);
			int len2 = SLib.GetLen(CurrentOrder.H.Memo);
			if(len1 != len2 || (len1 > 0 && !memo.equals(CurrentOrder.H.Memo))) {
				CurrentOrder.H.Memo = memo;
				OnCurrentDocumentModification();
				result = true;
			}
		}
		return result;
	}
	protected boolean UpdateTransferItemQttyInCurrentDocument(final Document.TransferItem srcData)
	{
		boolean result = false;
		if(srcData != null && srcData.Set != null && GetCurrentDocumentTransferListCount() > 0) {
			for(int i = 0; !result && i < GetCurrentDocumentTransferListCount(); i++) {
				Document.TransferItem ti = CurrentOrder.TiList.get(i);
				if(ti.RowIdx == srcData.RowIdx) {
					if(srcData.Set.Qtty > 0)
						CurrentOrder.TiList.get(i).Set.Qtty = srcData.Set.Qtty;
					else
						CurrentOrder.TiList.remove(i);
					result = true;
					break;
				}
			}
			if(result)
				OnCurrentDocumentModification();
		}
		return result;
	}
	protected boolean AddTransferItemToCurrentDocument(Document.TransferItem item)
	{
		boolean result = false;
		try {
			if(item != null && item.GoodsID > 0 && item.Set != null && item.Set.Qtty > 0.0) {
				CommonPrereqModule.WareEntry goods_item = FindGoodsItemByGoodsID(item.GoodsID);
				double price = goods_item.JsItem.optDouble("price", 0.0);
				InitCurrenDocument(SLib.PPEDIOP_ORDER);
				Document.TransferItem ti = item;
				ti.Set.Price = price;
				int max_row_idx = 0;
				boolean merged = false;
				if(CurrentOrder.TiList != null) {
					for(int i = 0; !merged && i < CurrentOrder.TiList.size(); i++) {
						Document.TransferItem iter_ti = CurrentOrder.TiList.get(i);
						merged = iter_ti.Merge(ti);
						if(max_row_idx < iter_ti.RowIdx)
							max_row_idx = iter_ti.RowIdx;
					}
				}
				if(!merged) {
					ti.RowIdx = max_row_idx + 1;
					if(CurrentOrder.TiList == null)
						CurrentOrder.TiList = new ArrayList<Document.TransferItem>();
					CurrentOrder.TiList.add(ti);
				}
				SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.VISIBLE);
				//NotifyCurrentOrderChanged();
				OnCurrentDocumentModification();
				result = true;
			}
		} catch(StyloQException exn) {
			;
		}
		return result;
	}
	protected SLib.STimeChunkArray PutBookingItemToCurrentDocument(Document.BookingItem item)
	{
		SLib.STimeChunkArray result = null;
		try {
			if(item != null) {
				InitCurrenDocument(SLib.sqbdtSvcReq);
				if(CurrentOrder.BkList == null)
					CurrentOrder.BkList = new ArrayList<Document.BookingItem>();
				CurrentOrder.BkList.clear();
				/*
				Document.BookingItem bk_item = new Document.BookingItem();
				bk_item.GoodsID = goods_id;
				bk_item.PrcID = prc_id;
				bk_item.RowIdx = 1;
				bk_item.ReqTime = new SLib.LDATETIME(dt, start_tm);
				bk_item.EstimatedDurationSec = CPM.GetServiceDurationForPrc(prc_id, goods_id);
				if(bk_item.EstimatedDurationSec <= 0)
					bk_item.EstimatedDurationSec = 3600; // default value
				 */
				CurrentOrder.BkList.add(item);
				OnCurrentDocumentModification();
				result = GetCurrentDocumentBusyList(item.PrcID);
			}
		} catch(StyloQException exn) {
			;
		}
		return result;
	}
	protected boolean SetClientToCurrentDocument(int opID, int cliID, int dlvrLocID, boolean forceUpdate)
	{
		boolean result = false;
		try {
			StyloQApp app_ctx = GetAppCtx();
			if(cliID > 0 && app_ctx != null) {
				JSONObject new_cli_entry = FindClientEntry(cliID);
				if(new_cli_entry != null) {
					if(CurrentOrder == null) {
						InitCurrenDocument(opID);
						//CurrentOrder = new Document(SLib.PPEDIOP_ORDER, SvcIdent, app_ctx);
						CurrentOrder.H.ClientID = cliID;
						CurrentOrder.H.DlvrLocID = dlvrLocID;
						CurrentOrder.H.BaseCurrencySymb = BaseCurrencySymb;
						result = true;
					}
					else {
						if(CurrentOrder.H.ClientID == 0) {
							CurrentOrder.H.ClientID = cliID;
							CurrentOrder.H.DlvrLocID = dlvrLocID;
							result = true;
						}
						else if(CurrentOrder.H.ClientID == cliID) {
							if(dlvrLocID != CurrentOrder.H.DlvrLocID) {
								CurrentOrder.H.DlvrLocID = dlvrLocID;
								result = true;
							}
						}
						else {
							// Здесь надо как-то умнО обработать изменение контрагента
							if(forceUpdate) {
								CurrentOrder.H.ClientID = cliID;
								CurrentOrder.H.DlvrLocID = dlvrLocID;
								result = true;
							}
							else {
								final int st = CurrentOrder.GetDocStatus();
								if(st == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT || st == 0) {
									JSONObject prev_cli_entry = FindClientEntry(CurrentOrder.H.ClientID);
									if(prev_cli_entry != null) {
										String prev_cli_name = prev_cli_entry.optString("nm", "");
										String new_cli_name = new_cli_entry.optString("nm", "");
										String msg_addendum = prev_cli_name + " -> " + new_cli_name;
										String text_fmt = app_ctx.GetString(ppstr2.PPSTR_CONFIRMATION, ppstr2.PPCFM_STQ_CHANGEORDCLI);
										class OnResultListener implements SLib.ConfirmationListener {
											@Override
											public void OnResult(SLib.ConfirmationResult r)
											{
												if(r == SLib.ConfirmationResult.YES) {
													SetClientToCurrentDocument(opID, cliID, dlvrLocID, true);
												}
											}
										}
										SLib.Confirm_YesNo(ActivityInstance, String.format(text_fmt, msg_addendum), new OnResultListener());
									}
									else {

									}
								}
								else {

								}
							}
						}
					}
					if(result)
						OnCurrentDocumentModification();
				}
				else {
					; // @err
				}

			}
		} catch(StyloQException exn) {
			result = false;
		}
		return result;
	}
	public SLib.STimeChunkArray GetCurrentDocumentBusyList(int prcID)
	{
		SLib.STimeChunkArray result = null;
		if(prcID > 0 && CurrentOrder != null && CurrentOrder.BkList != null && CurrentOrder.BkList.size() > 0) {
			for(int i = 0; i < CurrentOrder.BkList.size(); i++) {
				Document.BookingItem bi = CurrentOrder.BkList.get(i);
				if(bi != null && bi.PrcID == prcID) {
					SLib.STimeChunk tc = bi.GetEsimatedTimeChunk();
					if(tc != null) {
						if(result == null)
							result = new SLib.STimeChunkArray();
						result.add(tc);
					}
				}
			}
		}
		return result;
	}
	protected double GetAmountOfCurrentDocument()
	{
		double result = 0.0;
		if(CurrentOrder != null) {
			if(CurrentOrder.TiList != null) {
				for(int i = 0; i < CurrentOrder.TiList.size(); i++) {
					Document.TransferItem ti = CurrentOrder.TiList.get(i);
					if(ti != null && ti.Set != null)
						result += Math.abs(ti.Set.Qtty * ti.Set.Price);
				}
			}
			if(CurrentOrder.BkList != null) {
				for(int i = 0; i < CurrentOrder.BkList.size(); i++) {
					Document.BookingItem bi = CurrentOrder.BkList.get(i);
					if(bi != null && bi.Set != null)
						result += Math.abs(bi.Set.Qtty * bi.Set.Price);
				}
			}
		}
		return result;
	}
	protected boolean CommitCurrentDocument()
	{
		boolean ok = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(!Locker_CommitCurrentDocument) {
				Locker_CommitCurrentDocument = true;
				if(CurrentOrder != null && CurrentOrder.Finalize()) {
					final int s = CurrentOrder.GetDocStatus();
					if(s == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT)
						CurrentOrder.SetAfterTransmitStatus(StyloQDatabase.SecStoragePacket.styloqdocstWAITFORAPPROREXEC);
					// @todo Здесь еще долго со статусами разбираться придется!
					StyloQApp.PostDocumentResult result = app_ctx.RunSvcPostDocumentCommand(SvcIdent, CurrentOrder, ActivityInstance);
					ok = result.PostResult;
					if(ok) {
						;
					}
				}
				else
					Locker_CommitCurrentDocument = false;
			}
		}
		return ok;
	}
	protected boolean CancelCurrentDocument()
	{
		boolean ok = false;
		if(!Locker_CommitCurrentDocument) {
			Locker_CommitCurrentDocument = true;
			ok = Helper_CancelCurrentDocument(false);
			if(!ok)
				Locker_CommitCurrentDocument = false;
		}
		return ok;
	}
	private boolean Helper_CancelCurrentDocument(boolean force)
	{
		boolean ok = true;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(CurrentOrder != null && CurrentOrder.Finalize()) {
				final int st = CurrentOrder.GetDocStatus();
				if(st == 0 || st == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT) {
					// Такой документ просто удаляем
					if(!force) {
						class OnResultListener implements SLib.ConfirmationListener {
							@Override
							public void OnResult(SLib.ConfirmationResult r)
							{
								if(r == SLib.ConfirmationResult.YES) {
									Helper_CancelCurrentDocument(true);
								}
							}
						}
						String text = app_ctx.GetString(ppstr2.PPSTR_CONFIRMATION, ppstr2.PPCFM_STQ_RMVORD_DRAFT);
						SLib.Confirm_YesNo(ActivityInstance, text, new OnResultListener());
					}
					else {
						boolean local_err = false;
						if(CurrentOrder.H.ID > 0) {
							//SetDocStatus(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT); // Новый док автоматом является draft-документом
							boolean r = StoreCurrentDocument(StyloQDatabase.SecStoragePacket.styloqdocstDRAFT, StyloQDatabase.SecStoragePacket.styloqdocstCANCELLEDDRAFT);
							//
							// Далее эмулируем ответ от сервиса, ибо в данном случае никакого обращения не было (драфт-документ у сервиса не бывал)
							//
							StyloQApp.InterchangeResult subj = null;
							if(r) {
								subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.SUCCESS, SvcIdent, "", null);
							}
							else {
								subj = new StyloQApp.InterchangeResult(StyloQApp.SvcQueryResult.ERROR, SvcIdent, "", null);
							}
							subj.OriginalCmdItem = new StyloQCommand.Item();
							subj.OriginalCmdItem.Name = "CancelDocument";
							ActivityInstance.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
						}
					}
				}
				else if(st == StyloQDatabase.SecStoragePacket.styloqdocstAPPROVED) {
					// Надо отправить уведомление об отмене сервису
				}
			}
		}
		return ok;
	}
	public boolean HasPrcInCurrentOrder(int prcID)
	{
		boolean result = false;
		if(CurrentOrder != null) {
			if(CurrentOrder.BkList != null) {
				for(int i = 0; !result && i < CurrentOrder.BkList.size(); i++) {
					Document.BookingItem bi = CurrentOrder.BkList.get(i);
					if(bi != null && bi.PrcID == prcID)
						result = true;
				}
			}
		}
		return result;
	}
	public boolean HasGoodsInCurrentOrder(int goodsID)
	{
		boolean result = false;
		if(CurrentOrder != null) {
			if(CurrentOrder.TiList != null) {
				for(int i = 0; !result && i < CurrentOrder.TiList.size(); i++) {
					Document.TransferItem ti = CurrentOrder.TiList.get(i);
					if(ti != null && ti.GoodsID == goodsID)
						result = true;
				}
			}
			if(!result) {
				if(CurrentOrder.BkList != null) {
					for(int i = 0; !result && i < CurrentOrder.BkList.size(); i++) {
						Document.BookingItem bi = CurrentOrder.BkList.get(i);
						if(bi != null && bi.GoodsID == goodsID)
							result = true;
					}
				}
			}
		}
		return result;
	}
	protected Document.TransferItem SearchGoodsItemInCurrentOrderTi(int goodsID)
	{
		Document.TransferItem result = null;
		if(CurrentOrder != null && CurrentOrder.TiList != null) {
			for(int i = 0; result == null && i < CurrentOrder.TiList.size(); i++) {
				Document.TransferItem ti = CurrentOrder.TiList.get(i);
				if(ti != null && ti.GoodsID == goodsID)
					result = ti;
			}
		}
		return result;
	}
	protected double GetGoodsQttyInCurrentDocument(int goodsID)
	{
		double result = 0.0;
		if(CurrentOrder != null && CurrentOrder.TiList != null) {
			for(int i = 0; i < CurrentOrder.TiList.size(); i++) {
				Document.TransferItem ti = CurrentOrder.TiList.get(i);
				if(ti != null && ti.GoodsID == goodsID)
					result += Math.abs(ti.Set.Qtty);
			}
		}
		return result;
	}
	public static class WareEntry {
		WareEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			PrcExpandStatus = 0;
			PrcPrice = 0.0;
		}
		JSONObject JsItem;
		int   PrcExpandStatus; // 0 - no processors, 1 - processors collapsed, 2 - processors expanded
		double PrcPrice; // Специфическая цена товара для конкретного процессора
	}
	public static class ProcessorEntry {
		ProcessorEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			GoodsExpandStatus = 0;
		}
		JSONObject JsItem;
		int   GoodsExpandStatus; // 0 - no goods, 1 - goods collapsed, 2 - goods expanded
	}
	public static class SimpleSearchIndexEntry {
		SimpleSearchIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
		{
			ObjType = objType;
			ObjID = objID;
			Attr = attr;
			Text = text;
			DisplayText = displayText;
		}
		int ObjType; // PPOBJ_XXX
		int ObjID;
		int Attr; // PPOBJATTR_XXX
		final String Text;
		final String DisplayText;
	}

	public static class SimpleSearchResult {
		String Pattern;
		public ArrayList<SimpleSearchIndexEntry> List;
		private int[] ObjTypeList;
		private int ObjTypeCount;
		private int SelectedItemIdx;
		private String SearchResultInfoText;

		SimpleSearchResult()
		{
			ObjTypeCount = 0;
			ObjTypeList = new int[64];
			SelectedItemIdx = -1;
			SearchResultInfoText = null;
		}
		void Clear()
		{
			List = null;
			ObjTypeCount = 0;
			SelectedItemIdx = -1;
			SearchResultInfoText = null;
		}
		String GetSearchResultInfoText()
		{
			return SearchResultInfoText;
		}
		void SetSelectedItemIndex(int idx)
		{
			if(List != null && idx >= 0 && idx < List.size())
				SelectedItemIdx = idx;
			else
				SelectedItemIdx = -1;
		}
		void ResetSelectedItemIndex()
		{
			SelectedItemIdx = -1;
		}
		int GetSelectedItemIndex()
		{
			return SelectedItemIdx;
		}
		int FindIndexOfItem(final SimpleSearchIndexEntry item)
		{
			int result = -1;
			if(List != null && List.size() > 0) {
				for(int i = 0; result < 0 && i < List.size(); i++) {
					final SimpleSearchIndexEntry se = List.get(i);
					if(se != null && se.ObjType == item.ObjType && se.ObjID == item.ObjID)
						result = i;
				}
			}
			return result;
		}
		void Add(SimpleSearchIndexEntry entry)
		{
			if(entry != null) {
				boolean obj_type_found = false;
				for(int j = 0; !obj_type_found && j < ObjTypeCount; j++) {
					if(ObjTypeList[j] == entry.ObjType)
						obj_type_found = true;
				}
				if(!obj_type_found) {
					ObjTypeList[ObjTypeCount] = entry.ObjType;
					ObjTypeCount++;
				}
				if(List == null)
					List = new ArrayList<SimpleSearchIndexEntry>();
				List.add(entry);
			}
		}
		ArrayList<SimpleSearchIndexEntry> GetListByObjType(int objType)
		{
			ArrayList<SimpleSearchIndexEntry> result = null;
			if(List != null) {
				for(int i = 0; i < List.size(); i++) {
					SimpleSearchIndexEntry se = List.get(i);
					if(se != null && se.ObjType == objType) {
						if(result == null)
							result = new ArrayList<SimpleSearchIndexEntry>();
						result.add(se);
					}
				}
			}
			return result;
		}
		final boolean IsThereObj(int objType, int objID)
		{
			boolean result = false;
			if(objType > 0 && objID > 0 && List != null) {
				for(int i = 0; !result && i < List.size(); i++) {
					SimpleSearchIndexEntry se = List.get(i);
					if(se != null && se.ObjType == objType && se.ObjID == objID)
						result = true;
				}
			}
			return result;
		}
		public final int GetObjTypeCount()
		{
			return ObjTypeCount;
		}
		public final int GetObjTypeByIndex(int idx)
		{
			return (idx >= 0 && idx < ObjTypeCount) ? ObjTypeList[idx] : 0;
		}
	}
	public void InitSimpleIndex()
	{
		if(SimpleSearchIndex == null)
			SimpleSearchIndex = new ArrayList<SimpleSearchIndexEntry>();
		else
			SimpleSearchIndex.clear();
	}
	public void AddSimpleIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
	{
		//SimpleSearchIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
		SimpleSearchIndex.add(new SimpleSearchIndexEntry(objType, objID, attr, text.toLowerCase(), displayText));
	}
	public void AddGoodsToSimpleIndex()
	{
		if(GoodsListData != null) {
			for(int i = 0; i < GoodsListData.size(); i++) {
				CommonPrereqModule.WareEntry ware_item = GoodsListData.get(i);
				if(ware_item != null && ware_item.JsItem != null) {
					int id = ware_item.JsItem.optInt("id", 0);
					if(id > 0) {
						String nm = ware_item.JsItem.optString("nm");
						if(SLib.GetLen(nm) > 0) {
							AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_NAME, nm, null);
						}
						{
							JSONArray js_code_list = ware_item.JsItem.optJSONArray("code_list");
							if(js_code_list != null && js_code_list.length() > 0) {
								for(int j = 0; j < js_code_list.length(); j++) {
									JSONObject js_code = js_code_list.optJSONObject(j);
									if(js_code != null) {
										String code = js_code.optString("cod");
										if(SLib.GetLen(code) > 0)
											AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_CODE, code, nm);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	public void AddGoodsGroupsToSimpleIndex()
	{
		if(GoodsGroupListData != null) {
			for(int i = 0; i < GoodsGroupListData.size(); i++) {
				JSONObject js_item = GoodsGroupListData.get(i);
				if(js_item != null) {
					final int id = js_item.optInt("id", 0);
					if(id > 0) {
						String nm = js_item.optString("nm");
						if(SLib.GetLen(nm) > 0)
							AddSimpleIndexEntry(SLib.PPOBJ_GOODSGROUP, id, SLib.PPOBJATTR_NAME, nm, null);
					}
				}
			}
		}
	}
	public boolean SearchInSimpleIndex(String pattern)
	{
		final int min_pattern_len = 4;
		final int min_pattern_len_code = 5;
		final int min_pattern_len_ruinn = 8;
		final int min_pattern_len_rukpp = 6;
		final int max_result_count = 100;
		boolean result = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(SearchResult == null)
				SearchResult = new CommonPrereqModule.SimpleSearchResult();
			SearchResult.Clear();
			SearchResult.Pattern = pattern;
			if(SimpleSearchIndex != null && SLib.GetLen(pattern) >= min_pattern_len) {
				pattern = pattern.toLowerCase();
				for(int i = 0; i < SimpleSearchIndex.size(); i++) {
					CommonPrereqModule.SimpleSearchIndexEntry entry = SimpleSearchIndex.get(i);
					if(entry != null && SLib.GetLen(entry.Text) > 0 && entry.Text.indexOf(pattern) >= 0) {
						boolean skip = false;
						if(entry.Attr == SLib.PPOBJATTR_CODE && pattern.length() < min_pattern_len_code)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUINN && pattern.length() < min_pattern_len_ruinn)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUKPP && pattern.length() < min_pattern_len_rukpp)
							skip = true;
						if(!skip) {
							if(SearchResult.List != null && SearchResult.List.size() >= max_result_count) {
								// Если количество результатов превышает некий порог, то считаем поиск безуспешным - пусть
								// клиент вводит что-то более длинное для релевантности результата
								SearchResult.Clear();
								String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_TOOMANYRESULTS);
								if(SLib.GetLen(fmt_buf) > 0)
									SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(max_result_count));
								result = false;
								break;
							}
							else {
								SearchResult.Add(entry);
								result = true;
							}
						}
					}
				}
				if(result) {
					String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_SUCCESS);
					if(SLib.GetLen(fmt_buf) > 0)
						SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(SearchResult.List.size()));
				}
			}
			else {
				String fmt_buf = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SMPLSRCHRESULT_TOOSHRTPATTERN);
				if(SLib.GetLen(fmt_buf) > 0)
					SearchResult.SearchResultInfoText = String.format(fmt_buf, Integer.toString(min_pattern_len_code));
			}
		}
		return result;
	}
	public String GetSimpleSearchResultPattern()
	{
		return (SearchResult != null) ? SearchResult.Pattern : null;
	}
	public final boolean IsObjInSearchResult(int objType, int objID)
	{
		return (SearchResult != null) ? SearchResult.IsThereObj(objType, objID) : false;
	}
	public CommonPrereqModule(SLib.SlActivity activityInstance)
	{
		SvcIdent = null;
		CmdName = null;
		CmdDescr = null;
		CmdUuid = null;
		GoodsGroupListData = null;
		GoodsListData = null;
		BaseCurrencySymb = null;
		AgentID = 0;
		Gf = null;
		CurrentOrder = null;
		OrderHList = null;
		ProcessorListData = null;
		Locker_CommitCurrentDocument = false;
		ActivityInstance = activityInstance;
		ViewPagerResourceId = 0;
		TabLayoutResourceId = 0;
	}
	String GetBaseCurrencySymb() { return BaseCurrencySymb; }
	int   GetAgentID() { return AgentID; }
	public void GetAttributesFromIntent(Intent intent)
	{
		if(intent != null) {
			SvcIdent = intent.getByteArrayExtra("SvcIdent");
			CmdName = intent.getStringExtra("CmdName");
			CmdDescr = intent.getStringExtra("CmdDescr");
			String cmd_uuid_text = intent.getStringExtra("CmdUuid");
			if(SLib.GetLen(cmd_uuid_text) > 0) {
				CmdUuid = UUID.fromString(cmd_uuid_text);
			}
		}
	}
	public void SetupActivity(StyloQDatabase db, int viewPagerResourceId, int tabLayoutResourceId) throws StyloQException
	{
		if(ActivityInstance != null && db != null) {
			ViewPagerResourceId = viewPagerResourceId;
			TabLayoutResourceId = tabLayoutResourceId;
			String title_text = null;
			String blob_signature = null;
			if(SLib.GetLen(SvcIdent) > 0) {
				StyloQDatabase.SecStoragePacket svc_packet = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
				String svc_name = null;
				if(svc_packet != null) {
					StyloQFace face = svc_packet.GetFace();
					if(face != null) {
						blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
						svc_name = svc_packet.GetSvcName(face);
					}
				}
				if(SLib.GetLen(svc_name) > 0)
					SLib.SetCtrlString(ActivityInstance, R.id.CTL_PAGEHEADER_SVCTITLE, svc_name);
			}
			if(SLib.GetLen(CmdName) > 0)
				title_text = CmdName;
			if(SLib.GetLen(CmdDescr) > 0)
				title_text = (SLib.GetLen(title_text) > 0) ? (title_text + "\n" + CmdDescr) : CmdDescr;
			if(SLib.GetLen(title_text) > 0)
				SLib.SetCtrlString(ActivityInstance, R.id.CTL_PAGEHEADER_TOPIC, title_text);
			SLib.SetupImage(ActivityInstance, ActivityInstance.findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature);
		}
	}
	public void SetTabVisibility(CommonPrereqModule.Tab tabId, int visibilityMode)
	{
		if(ActivityInstance != null && ViewPagerResourceId != 0 && TabLayoutResourceId != 0) {
			ViewPager2 view_pager = (ViewPager2)ActivityInstance.findViewById(ViewPagerResourceId);
			if(view_pager != null && TabList != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tabId) {
						View v_lo_tab = ActivityInstance.findViewById(TabLayoutResourceId);
						if(v_lo_tab != null && v_lo_tab instanceof TabLayout) {
							TabLayout lo_tab = (TabLayout)v_lo_tab;
							((ViewGroup)lo_tab.getChildAt(0)).getChildAt(tidx).setVisibility(visibilityMode);
						}
						break;
					}
				}
			}
		}
	}
	public void MakeCurrentDocList()
	{
		if(OrderHList != null)
			OrderHList.clear();
		try {
			if(SvcIdent != null) {
				StyloQApp app_ctx = GetAppCtx();
				StyloQDatabase db = (app_ctx != null) ? app_ctx.GetDB() : null;
				if(db != null) {
					StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
					if(svc_pack != null) {
						long svc_id = svc_pack.Rec.ID;
						ArrayList<Long> doc_id_list = db.GetDocIdListByType(+1, StyloQDatabase.SecStoragePacket.doctypGeneric, svc_id, null);
						if(doc_id_list != null) {
							for(int i = 0; i < doc_id_list.size(); i++) {
								long local_doc_id = doc_id_list.get(i);
								StyloQDatabase.SecStoragePacket local_doc_pack = db.GetPeerEntry(local_doc_id);
								if(local_doc_pack != null) {
									byte [] raw_doc = local_doc_pack.Pool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_doc) > 0) {
										String json_doc = new String(raw_doc);
										Document local_doc = new Document();
										if(local_doc.FromJson(json_doc)) {
											if(local_doc.H != null)
												local_doc.H.Flags = local_doc_pack.Rec.Flags;
											if(OrderHList == null)
												OrderHList = new ArrayList<Document.DisplayEntry>();
											// Эти операторы нужны на начальном этапе разработки поскольку
											// финализация пакета документа появилась не сразу {
											if(local_doc.GetNominalAmount() == 0.0)
												local_doc.H.Amount = local_doc.CalcNominalAmount();
											// }
											OrderHList.add(new Document.DisplayEntry(local_doc));
											local_doc.H = null;
										}

									}
								}
							}
						}
					}
				}
			}
		} catch(StyloQException exn) {
			;
		}
	}
	public static class GoodsFilt {
		ArrayList <Integer> GroupIdList;
		ArrayList <Integer> BrandList;
	}
	public ProcessorEntry FindProcessorItemByID(int id)
	{
		ProcessorEntry result = null;
		if(ProcessorListData != null && id > 0) {
			for(int i = 0; result == null && i < ProcessorListData.size(); i++) {
				ProcessorEntry entry = ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					final int iter_id = entry.JsItem.optInt("id", 0);
					if(iter_id == id)
						result = entry;
				}
			}
		}
		return result;
	}
	public int FindProcessorItemIndexByID(int id)
	{
		int result = -1;
		if(ProcessorListData != null && id > 0) {
			for(int i = 0; result < 0 && i < ProcessorListData.size(); i++) {
				ProcessorEntry entry = ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					final int iter_id = entry.JsItem.optInt("id", 0);
					if(iter_id == id)
						result = i;
				}
			}
		}
		return result;
	}
	public boolean IsThereProcessorAssocWithGoods(int goodsID)
	{
		boolean result = false;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				for(int i = 0; !result && i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							for(int j = 0; !result && j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								if(js_goods_item != null) {
									int iter_id = js_goods_item.optInt("id", 0);
									if(iter_id == goodsID)
										result = true;
								}
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = false;
		}
		return result;
	}
	public ArrayList <ProcessorEntry> GetProcessorListByGoods(int goodsID)
	{
		ArrayList <ProcessorEntry> result = null;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				for(int i = 0; i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							boolean found = false;
							for(int j = 0; !found && j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								if(js_goods_item != null) {
									int iter_id = js_goods_item.optInt("id", 0);
									if(iter_id == goodsID) {
										if(result == null)
											result = new ArrayList<ProcessorEntry>();
										result.add(entry);
										found = true;
									}
								}
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public double GetPriceForPrc(int prcID, int goodsID)
	{
		double result = 0.0;
		if(goodsID > 0) {
			try {
				if(prcID > 0 && ProcessorListData != null) {
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										result = js_goods_item.optDouble("price", 0.0);
										break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
									}
								}
							}
							break;
						}
					}
				}
				if(result <= 0.0) {
					WareEntry ware_entry = FindGoodsItemByGoodsID(goodsID);
					if(ware_entry != null)
						result = ware_entry.JsItem.optDouble("price", 0.0);
				}
			} catch(JSONException exn) {
				;
			}
		}
		return result;
	}
	public int GetServiceDurationForPrc(int prcID, int goodsID)
	{
		int   result = 0;
		try {
			if(goodsID > 0 && ProcessorListData != null) {
				if(prcID > 0) {
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										result = js_goods_item.optInt("duration", 0);
										break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
									}
								}
							}
							break;
						}
					}
				}
				else {
					int _sum = 0;
					int _count = 0;
					for(int i = 0; i < ProcessorListData.size(); i++) {
						ProcessorEntry entry = ProcessorListData.get(i);
						if(entry != null && entry.JsItem != null) {
							JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
							if(js_goods_list != null) {
								for(int j = 0; j < js_goods_list.length(); j++) {
									JSONObject js_goods_item = js_goods_list.getJSONObject(j);
									int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
									if(iter_id == goodsID) {
										int _duration = js_goods_item.optInt("duration", 0);
										if(_duration > 0) {
											_sum += _duration;
											_count++;
											break; // В этом цикле такой же товар больше не встретится (если, конечно, нет ошибок в данных)
										}
									}
								}
							}
						}
					}
					if(_sum > 0) {
						assert(_count > 0);
						result = (int)(((double)_sum) / ((double)_count));
					}
				}
			}
		} catch(JSONException exn) {
			result = 0;
		}
		return result;
	}
	public ArrayList <WareEntry> GetGoodsListByPrc(int prcID)
	{
		ArrayList <WareEntry> result = null;
		try {
			if(prcID > 0 && ProcessorListData != null && GoodsListData != null) {
				for(int i = 0; i < ProcessorListData.size(); i++) {
					ProcessorEntry entry = ProcessorListData.get(i);
					if(entry != null && entry.JsItem != null && entry.JsItem.optInt("id", 0) == prcID) {
						JSONArray js_goods_list = entry.JsItem.optJSONArray("goods_list");
						if(js_goods_list != null) {
							for(int j = 0; j < js_goods_list.length(); j++) {
								JSONObject js_goods_item = js_goods_list.getJSONObject(j);
								int iter_id = (js_goods_item != null) ? js_goods_item.optInt("id", 0) : 0;
								if(iter_id > 0) {
									WareEntry org_ware_entry = FindGoodsItemByGoodsID(iter_id);
									if(org_ware_entry != null) {
										int duration = (js_goods_item != null) ? js_goods_item.optInt("duration", 0) : 0;
										double price = (js_goods_item != null) ? js_goods_item.optDouble("price", 0.0) : 0.0;
										if(result == null)
											result = new ArrayList<WareEntry>();
										WareEntry new_ware_entry = new WareEntry(org_ware_entry.JsItem);
										new_ware_entry.PrcPrice = price;
										result.add(new_ware_entry);
									}
								}
							}
						}
						break;
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public void ResetGoodsFiter()
	{
		Gf = null;
	}
	public boolean SetGoodsFilterByBrand(int brandID)
	{
		boolean ok = true;
		if(brandID > 0) {
			if(Gf == null)
				Gf = new GoodsFilt();
			if(Gf.BrandList == null)
				Gf.BrandList = new ArrayList<Integer>();
			Gf.GroupIdList = null;
			Gf.BrandList.clear();
			Gf.BrandList.add(brandID);
		}
		else
			ok = false;
		return ok;
	}
	public boolean SetGoodsFilterByGroup(int groupID)
	{
		boolean ok = true;
		if(groupID > 0) {
			if(Gf == null)
				Gf = new GoodsFilt();
			if(Gf.GroupIdList == null)
				Gf.GroupIdList = new ArrayList<Integer>();
			Gf.BrandList = null;
			Gf.GroupIdList.clear();
			Gf.GroupIdList.add(groupID);
		}
		else
			ok = false;
		return ok;
	}
	public String FormatCurrency(double val)
	{
		NumberFormat format = NumberFormat.getCurrencyInstance();
		format.setMaximumFractionDigits(2);
		if(SLib.GetLen(BaseCurrencySymb) > 0)
			format.setCurrency(Currency.getInstance(BaseCurrencySymb));
		return format.format(val);
	}
	public void GetCommonJsonFactors(JSONObject jsHead) throws JSONException
	{
		BaseCurrencySymb = jsHead.optString("basecurrency", null);
		AgentID = jsHead.optInt("agentid", 0);
	}
	public void MakeGoodsListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = jsHead.optJSONArray("goods_list");
		if(temp_array != null) {
			GoodsListData = new ArrayList<WareEntry>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject) {
					int goods_id = ((JSONObject) temp_obj).optInt("id", 0);
					WareEntry new_entry = new WareEntry((JSONObject)temp_obj);
					if(ProcessorListData != null && IsThereProcessorAssocWithGoods(goods_id))
						new_entry.PrcExpandStatus = 1;
					else
						new_entry.PrcExpandStatus = 0;
					GoodsListData.add(new_entry);
				}
			}
			Collections.sort(GoodsListData, new Comparator<WareEntry>() {
				@Override public int compare(WareEntry lh, WareEntry rh)
				{
					String ls = lh.JsItem.optString("nm", "");
					String rs = rh.JsItem.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeProcessorListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		//ProcessorListData = jsHead.optJSONArray("processor_list");
		JSONArray temp_array = jsHead.optJSONArray("processor_list");
		if(temp_array != null) {
			ProcessorListData = new ArrayList<ProcessorEntry>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject) {
					JSONObject js_prc_item = (JSONObject)temp_obj;
					ProcessorEntry new_entry = new ProcessorEntry(js_prc_item);
					JSONArray js_goods_list = js_prc_item.getJSONArray("goods_list");
					if(js_goods_list != null && js_goods_list.length() > 0)
						new_entry.GoodsExpandStatus = 1;
					else
						new_entry.GoodsExpandStatus = 0;
					ProcessorListData.add(new_entry);
				}
			}
			Collections.sort(ProcessorListData, new Comparator<ProcessorEntry>() {
				@Override public int compare(ProcessorEntry lh, ProcessorEntry rh)
				{
					String ls = lh.JsItem.optString("nm", "");
					String rs = rh.JsItem.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public void MakeGoodsGroupListFromCommonJson(JSONObject jsHead) throws JSONException
	{
		JSONArray temp_array = jsHead.optJSONArray("goodsgroup_list");
		if(temp_array != null) {
			GoodsGroupListData = new ArrayList<JSONObject>();
			for(int i = 0; i < temp_array.length(); i++) {
				Object temp_obj = temp_array.get(i);
				if(temp_obj != null && temp_obj instanceof JSONObject)
					GoodsGroupListData.add((JSONObject)temp_obj);
			}
			Collections.sort(GoodsGroupListData, new Comparator<JSONObject>() {
				@Override public int compare(JSONObject lh, JSONObject rh)
				{
					String ls = lh.optString("nm", "");
					String rs = rh.optString("nm", "");
					return ls.toLowerCase().compareTo(rs.toLowerCase());
				}
			});
		}
	}
	public int FindGoodsGroupItemIndexByID(int id)
	{
		int result = -1;
		if(GoodsGroupListData != null && id > 0) {
			for(int i = 0; result < 0 && i < GoodsGroupListData.size(); i++) {
				final int iter_id = GoodsGroupListData.get(i).optInt("id", 0);
				if(iter_id == id)
					result = i;
			}
		}
		return result;
	}
	public WareEntry FindGoodsItemByGoodsID(int goodsID)
	{
		WareEntry result = null;
		if(GoodsListData != null && goodsID > 0) {
			for(int i = 0; result == null && i < GoodsListData.size(); i++) {
				final int goods_id = GoodsListData.get(i).JsItem.optInt("id", 0);
				if(goods_id == goodsID)
					result = GoodsListData.get(i);
			}
		}
		return result;
	}
	int FindGoodsItemIndexByID(int id)
	{
		int result = -1;
		if(GoodsListData != null && id > 0) {
			for(int i = 0; result < 0 && i < GoodsListData.size(); i++) {
				final int iter_id = GoodsListData.get(i).JsItem.optInt("id", 0);
				if(iter_id == id)
					result = i;
			}
		}
		return result;
	}
	public WareEntry GetGoodsListItemByIdx(int idx)
	{
		WareEntry result = null;
		if(GoodsListData != null && idx >= 0 && idx < GoodsListData.size()) {
			if(Gf != null) {
				int counter = 0;
				for(int i = 0; result == null && i < GoodsListData.size(); i++) {
					if(CheckGoodsListItemForFilt(GoodsListData.get(i))) {
						if(counter == idx)
							result = GoodsListData.get(i);
						else
							counter++;
					}
				}
			}
			else
				result = GoodsListData.get(idx);
		}
		return result;
	}
	public int GetGoodsListSize()
	{
		int result = 0;
		if(GoodsListData != null) {
			if(Gf != null) {
				for(int i = 0; i < GoodsListData.size(); i++) {
					if(CheckGoodsListItemForFilt(GoodsListData.get(i)))
						result++;
				}
			}
			else
				result = GoodsListData.size();
		}
		return result;
	}
	public boolean CheckGoodsListItemForFilt(WareEntry item)
	{
		boolean result = false;
		if(item == null || item.JsItem == null)
			result = false;
		else if(Gf == null)
			result =  true;
		else {
			result = true;
			if(Gf.GroupIdList != null && Gf.GroupIdList.size() > 0) {
				int parid = item.JsItem.optInt("parid", 0);
				result = false;
				if(parid > 0) {
					for(int i = 0; !result && i < Gf.GroupIdList.size(); i++)
						if(Gf.GroupIdList.get(i) == parid)
							result = true;
				}
			}
			if(result && Gf.BrandList != null && Gf.BrandList.size() > 0) {
				int brand_id = item.JsItem.optInt("brandid", 0);
				result = false;
				if(brand_id > 0) {
					for(int i = 0; !result && i < Gf.BrandList.size(); i++)
						if(Gf.BrandList.get(i) == brand_id)
							result = true;
				}
			}
		}
		return result;
	}
	public JSONObject FindClientEntry(int cliID)
	{
		JSONObject result = null;
		if(CliListData != null && cliID > 0) {
			for(int i = 0; i < CliListData.size(); i++) {
				CommonPrereqModule.CliEntry ce = CliListData.get(i);
				if(ce != null && ce.JsItem != null) {
					int _id = ce.JsItem.optInt("id", 0);
					if(_id == cliID) {
						result = ce.JsItem;
						break;
					}
				}
			}
		}
		return result;
	}
	public JSONObject FindDlvrLocEntryInCliEntry(JSONObject cliJs, int dlvrLocID)
	{
		JSONObject result = null;
		try {
			if(CliListData != null && cliJs != null && dlvrLocID > 0) {
				JSONArray dvlrloc_list_js = cliJs.optJSONArray("dlvrloc_list");
				if(dvlrloc_list_js != null && dvlrloc_list_js.length() > 0) {
					for(int j = 0; j < dvlrloc_list_js.length(); j++) {
						JSONObject dlvrloc_js = dvlrloc_list_js.getJSONObject(j);
						if(dlvrloc_js != null) {
							int iter_id = dlvrloc_js.optInt("id", 0);
							if(iter_id == dlvrLocID) {
								result = dlvrloc_js;
								break;
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = null;
		}
		return result;
	}
	public int FindDlvrLocEntryIndexInCliEntry(JSONObject cliJs, int dlvrLocID)
	{
		int result = -1;
		try {
			if(CliListData != null && cliJs != null && dlvrLocID > 0) {
				JSONArray dvlrloc_list_js = cliJs.optJSONArray("dlvrloc_list");
				if(dvlrloc_list_js != null && dvlrloc_list_js.length() > 0) {
					for(int j = 0; j < dvlrloc_list_js.length(); j++) {
						JSONObject dlvrloc_js = dvlrloc_list_js.getJSONObject(j);
						if(dlvrloc_js != null) {
							int iter_id = dlvrloc_js.optInt("id", 0);
							if(iter_id == dlvrLocID) {
								result = j;
								break;
							}
						}
					}
				}
			}
		} catch(JSONException exn) {
			result = -1;
		}
		return result;
	}
	public JSONObject FindClientEntryByDlvrLocID(int dlvrLocID)
	{
		JSONObject result = null;
		if(CliListData != null && dlvrLocID > 0) {
			for(int i = 0; result == null && i < CliListData.size(); i++) {
				CommonPrereqModule.CliEntry ce = CliListData.get(i);
				if(ce != null && ce.JsItem != null) {
					if(FindDlvrLocEntryInCliEntry(ce.JsItem, dlvrLocID) != null)
						result = ce.JsItem;
				}
			}
		}
		return result;
	}
	private static class SearchDetailListAdapter extends SLib.InternalArrayAdapter {
		//private int RcId;
		SearchDetailListAdapter(Context ctx, int rcId, ArrayList data)
		{
			super(ctx, rcId, data);
			//RcId = rcId;
		}
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			//Context ctx = parent.getContext();
			Context _ctx = getContext();
			if(item != null && _ctx != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null) {
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				}
				if(convertView != null) {
					if(item instanceof CommonPrereqModule.SimpleSearchIndexEntry) {
						CommonPrereqModule.SimpleSearchIndexEntry se = (CommonPrereqModule.SimpleSearchIndexEntry)item;
						String pattern = null;
						if(_ctx instanceof CmdROrderPrereqActivity)
							pattern = ((CmdROrderPrereqActivity)_ctx).CPM.GetSimpleSearchResultPattern();
						else if(_ctx instanceof CmdRAttendancePrereqActivity)
							pattern = ((CmdRAttendancePrereqActivity)_ctx).CPM.GetSimpleSearchResultPattern();
						{
							//SLib.SetCtrlString(convertView, R.id.CTL_SEARCHPANE_FOUNDTEXT, se.Text);
							TextView v = convertView.findViewById(R.id.CTL_SEARCHPANE_FOUNDTEXT);
							if(v != null) {
								final int text_len = SLib.GetLen(se.Text);
								final int pat_len = SLib.GetLen(pattern);
								final int fp_start = (pat_len > 0) ? se.Text.indexOf(pattern) : -1;
								if(fp_start >= 0) {
									int fp_end = fp_start+pat_len;
									SpannableStringBuilder spbldr = new SpannableStringBuilder();
									int color_reg = _ctx.getResources().getColor(R.color.ListItemRegular, _ctx.getTheme());
									int color_found = _ctx.getResources().getColor(R.color.ListItemFound, _ctx.getTheme());
									{
										SpannableString ss = new SpannableString(se.Text.substring(0, fp_start));
										ss.setSpan(new BackgroundColorSpan(color_reg), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									{
										SpannableString ss = new SpannableString(se.Text.substring(fp_start, fp_end));
										ss.setSpan(new BackgroundColorSpan(color_found), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									if(fp_end < (text_len-1)) {
										SpannableString ss = new SpannableString(se.Text.substring(fp_end, text_len-1));
										ss.setSpan(new BackgroundColorSpan(color_reg), 0, ss.length(), 0);
										spbldr.append(ss);
									}
									//v.setText(se.Text);
									v.setText(spbldr, TextView.BufferType.SPANNABLE);
								}
								else
									v.setText(se.Text);
							}
						}
						{
							TextView v = convertView.findViewById(R.id.CTL_SEARCHPANE_FOUNDDETAIL);
							if(v != null) {
								if(SLib.GetLen(se.DisplayText) > 0) {
									v.setVisibility(View.VISIBLE);
									v.setText(se.DisplayText);
								}
								else
									v.setVisibility(View.GONE);
							}
						}
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	public void GetSearchPaneListViewItem(View itemView, int itemIdx)
	{
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && SearchResult != null && itemView != null && itemIdx < SearchResult.GetObjTypeCount()) {
			int obj_type = SearchResult.GetObjTypeByIndex(itemIdx);
			String obj_type_title = SLib.GetObjectTitle(app_ctx, obj_type);
			SLib.SetCtrlString(itemView, R.id.LVITEM_GENERICNAME, (obj_type_title != null) ? obj_type_title : "");
			{
				ListView detail_lv = (ListView)itemView.findViewById(R.id.searchPaneTerminalListView);
				ArrayList <CommonPrereqModule.SimpleSearchIndexEntry> detail_list = SearchResult.GetListByObjType(obj_type);
				if(detail_lv != null && detail_list != null) {
					SearchDetailListAdapter adapter = new SearchDetailListAdapter(/*this*/itemView.getContext(), R.layout.li_searchpane_resultdetail, detail_list);
					detail_lv.setAdapter(adapter);
					{
						int total_items_height = SLib.CalcListViewHeight(detail_lv);
						if(total_items_height > 0) {
							ViewGroup.LayoutParams params = detail_lv.getLayoutParams();
							params.height = total_items_height;
							detail_lv.setLayoutParams(params);
							detail_lv.requestLayout();
						}
					}
					adapter.setNotifyOnChange(true);
					detail_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
						@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
						{
							Object item = (Object)parent.getItemAtPosition(position);
							Context ctx = parent.getContext();
							if(item != null && ctx != null && ctx instanceof SLib.SlActivity) {
								SLib.SlActivity activity = (SLib.SlActivity)parent.getContext();
								SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
								ev_subj.ItemIdx = position;
								ev_subj.ItemId = id;
								ev_subj.ItemObj = item;
								ev_subj.ItemView = view;
								//ev_subj.ParentView = parent;
								activity.HandleEvent(SLib.EV_LISTVIEWITEMCLK, parent, ev_subj);
							}
						}
					});
				}
			}
		}
	}
}
