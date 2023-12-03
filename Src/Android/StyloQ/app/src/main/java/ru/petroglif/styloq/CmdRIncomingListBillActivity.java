// CmdRIncomingListBillActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.hardware.Camera;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.IdRes;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.zxing.client.android.Intents;
import com.journeyapps.barcodescanner.ScanContract;
import com.journeyapps.barcodescanner.ScanOptions;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

public class CmdRIncomingListBillActivity extends SLib.SlActivity {
	public CommonPrereqModule CPM;
	private ArrayList <Document.EditAction> DocEditActionList;
	private ArrayList <BusinessEntity.CliDocStatus> DocStatusList;
	enum ScanType {
		Undef,
		Veriy,
		Setting,
		Search
	}
	//
	// Descr: Возможные типы входящих списков
	//
	enum ListDataType {
		Undef,  // Не определен (не должно такого быть иначе как до окончания инициализации)
		Bill,   // Документы
		CCheck, // Кассовые чеки
		TSess,  // Технологические сессии
		ToDo    // Задачи
	}
	private ScanType ScanSource;
	private ListDataType ListType; // @v11.6.6
	private Timer RefreshSvcDataPollTmr; // @v11.6.2
	private long  LastRefreshTime; // @v11.6.4
	private boolean DocsSharingEnabled; // @v11.9.0
	private static final int RefreshSvcDataPollPeriodMs = 1 * 60 * 1000; // @v11.6.2
	private void RefreshCurrentDocStatus()
	{
		if(CPM.TabList != null) {
			ViewPager2 view_pager = (ViewPager2) findViewById(R.id.VIEWPAGER_INCOMINGLISTBILL);
			if(view_pager != null) {
				int tidx = view_pager.getCurrentItem();
				CommonPrereqModule.TabEntry tab_entry = CPM.TabList.get(tidx);
				if(tab_entry != null && tab_entry.TabId == CommonPrereqModule.Tab.tabCurrentDocument)
					HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
			}
		}
	}
	private class RefreshTimerTask extends TimerTask {
		@Override public void run()
		{
			runOnUiThread(new Runnable() { @Override public void run() { RefreshCurrentDocStatus(); }});
		}
	}
	private static class ThreadEngine_RefreshDataSvcPoll implements Runnable { // @v11.6.2
		private StyloQApp AppCtx;
		private byte [] SvcIdent;
		private UUID OrgCmdUuid;
		private SLib.EventHandler Handler;
		ThreadEngine_RefreshDataSvcPoll(StyloQApp appCtx, byte [] svcIdent, UUID orgCmdUuid, SLib.EventHandler handler)
		{
			AppCtx = appCtx;
			SvcIdent = svcIdent;
			OrgCmdUuid = orgCmdUuid;
			Handler = handler;
		}
		@Override public void run()
		{
			if(SLib.GetLen(SvcIdent) > 0 && Handler != null && OrgCmdUuid != null) {
				try {
					StyloQCommand.Item cmd_item = null;
					StyloQDatabase db = AppCtx.GetDB();
					if(db != null) {
						StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
						StyloQDatabase.SecStoragePacket cmdl_pack = db.GetForeignSvcCommandList(SvcIdent);
						StyloQCommand.List cmd_list = (cmdl_pack != null) ? cmdl_pack.GetCommandList() : null;
						if(cmd_list != null && SLib.GetCount(cmd_list.Items) > 0) {
							for(int cmdi = 0; cmd_item == null && cmdi < cmd_list.Items.size(); cmdi++) {
								StyloQCommand.Item iter_cmd_item = cmd_list.Items.get(cmdi);
								if(iter_cmd_item != null && SLib.AreUUIDsEqual(iter_cmd_item.Uuid, OrgCmdUuid))
									cmd_item = iter_cmd_item;
							}
						}
						if(cmd_item != null) {
							JSONObject js_query = new JSONObject();
							String cmd_text = cmd_item.Uuid.toString();
							js_query.put("cmd", cmd_text);
							js_query.put("time", System.currentTimeMillis());
							Object qr = Handler.HandleEvent(SLib.EV_QUERY, null, "GetSvcDataTime");
							if(qr != null && qr instanceof SLib.LDATETIME) {
								SLib.LDATETIME svc_dtm = (SLib.LDATETIME)qr;
								js_query.put("ifchangedsince", SLib.datetimefmt(svc_dtm, SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
							}
							AppCtx.RunSvcCommand(SvcIdent, cmd_item, js_query, true, Handler);
						}
					}
				} catch(StyloQException exn) {
					;
				} catch(JSONException exn) {
					;
				}
			}
		}
	}
	public CmdRIncomingListBillActivity()
	{
		CPM = new CommonPrereqModule(this);
		DocEditActionList = null;
		DocStatusList = null;
		ScanSource = ScanType.Undef;
		ListType = ListDataType.Undef; // @v11.6.6
		RefreshSvcDataPollTmr = null; // @v11.6.2
		LastRefreshTime = 0; // @v11.6.4
		DocsSharingEnabled = false; // @v11.9.0
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_INCOMINGLISTBILL;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && (CPM.TabList == null || force)) {
			CPM.TabList = new ArrayList<CommonPrereqModule.TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			CommonPrereqModule.TabInitEntry[] tab_init_list = {
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabIncomingList, R.layout.layout_incominglist_bill_main, "@{documentlist}", true),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabCurrentDocument, R.layout.layout_incominglist_bill_selected, "@{selecteddocument}", true),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoodsGroups, R.layout.layout_orderprereq_goodsgroups, "@{group_pl}", (CPM.GoodsGroupListData != null)),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabBrands, R.layout.layout_orderprereq_brands, "@{brand_pl}", (CPM.BrandListData != null)),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoods, R.layout.layout_orderprereq_goods, "@{ware_pl}", (CPM.GoodsListData != null)),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabClients, R.layout.layout_orderprereq_clients, "@{client_pl}", (CPM.CliListData != null)),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabXclVerify, R.layout.layout_incominglist_bill_scanmarks,
							"@{filter_styloqincominglistparam_actiondocacceptancemarks}", (CPM.GetActionFlags() & Document.actionDocAcceptanceMarks) != 0),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabXclSetting, R.layout.layout_incominglist_bill_scanmarks,
							"@{filter_styloqincominglistparam_actiondocsettingmarks}", (CPM.GetActionFlags() & Document.actionDocSettingMarks) != 0),
					new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabSearch, R.layout.layout_searchpane, "@{dosearch}", true),
			};
			for(int i = 0; i < tab_init_list.length; i++) {
				final CommonPrereqModule.TabInitEntry _t = tab_init_list[i];
				if(_t != null && _t.Condition) {
					SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_t.Tab.ordinal(), _t.Rc, tab_layout_rcid);
					String title = SLib.ExpandString(app_ctx, _t.Title);
					CPM.TabList.add(new CommonPrereqModule.TabEntry(_t.Tab, title, f));
				}
			}
		}
	}
	private void GetFragmentData(Object entry)
	{
		if(entry != null) {
			ViewGroup vg = null;
			if(entry instanceof SLib.SlFragmentStatic) {
				View v = ((SLib.SlFragmentStatic)entry).getView();
				if(v instanceof ViewGroup)
					vg = (ViewGroup)v;
			}
			else if(entry instanceof ViewGroup)
				vg = (ViewGroup)entry;
			if(vg != null) {
				/*
				int vg_id = vg.getId();
				if(vg_id == R.id.LAYOUT_ORDERPREPREQ_ORDR)
					CPM.UpdateMemoInCurrentDocument(SLib.GetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO));
				 */
			}
		}
	}
	private static class DetailTiListAdapter extends SLib.InternalArrayAdapter {
		private Document Doc;
		DetailTiListAdapter(Context ctx, int rcId, Document doc)
		{
			super(ctx, rcId, doc.TiList);
			Doc = doc;
		}
		Document GetDoc() { return Doc; }
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			//Object item = (Object)getItem(position);
			Document.TransferItem ti = (Doc != null && Doc.TiList != null && position >= 0 && position < Doc.TiList.size()) ? Doc.TiList.get(position) : null;
			Context _ctx = getContext();
			//Context ctx = parent.getContext();
			if(ti != null && _ctx != null && _ctx instanceof CmdRIncomingListBillActivity) {
				CmdRIncomingListBillActivity activity = (CmdRIncomingListBillActivity)_ctx;
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null)
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				if(convertView != null) {
					CommonPrereqModule.WareEntry ware_entry = activity.CPM.FindGoodsItemByGoodsID(ti.GoodsID);
					SLib.SetCtrlString(convertView, R.id.CTL_TI_ITEMN, "#"+Integer.toString(ti.RowIdx));
					SLib.SetCtrlString(convertView, R.id.CTL_TI_GOODSNAME, (ware_entry != null) ? ware_entry.Item.Name : null);
					if(ti.Set != null) {
						SLib.SetCtrlString(convertView, R.id.CTL_TI_QTTY, activity.CPM.FormatQtty(ti.Set.Qtty, ti.UnitID, false));
						SLib.SetCtrlString(convertView, R.id.CTL_TI_PRICE, activity.CPM.FormatCurrency(ti.Set.Price));
						SLib.SetCtrlString(convertView, R.id.CTL_TI_AMOUNT, activity.CPM.FormatCurrency(ti.Set.Qtty * ti.Set.Price));
					}
					else {
						SLib.SetCtrlString(convertView, R.id.CTL_TI_QTTY, null);
						SLib.SetCtrlString(convertView, R.id.CTL_TI_PRICE, null);
						SLib.SetCtrlString(convertView, R.id.CTL_TI_AMOUNT, null);
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	private void MakeSimpleSearchIndex() // @v11.7.6
	{
		CPM.InitSimpleIndex();
		CPM.AddGoodsToSimpleIndex();
		CPM.AddGoodsGroupsToSimpleIndex();
		CPM.AddIncomingDocumentsToSimpleIndex();
	}
	private void GotoTab(CommonPrereqModule.Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		CPM.Implement_GotoTab(tab, R.id.VIEWPAGER_INCOMINGLISTBILL, recyclerViewToUpdate, goToIndex, nestedIndex, -1);
	}
	private CommonPrereqModule.TabEntry SearchTabEntry(CommonPrereqModule.Tab tab)
	{
		return CPM.SearchTabEntry(R.id.VIEWPAGER_INCOMINGLISTBILL, tab);
	}
	private void NotifyTabContentChanged(CommonPrereqModule.Tab tabId, int innerViewId)
	{
		CPM.NotifyTabContentChanged(R.id.VIEWPAGER_INCOMINGLISTBILL, tabId, innerViewId);
	}
	private void NotifyCurrentDocumentChanged()
	{
		NotifyTabContentChanged(CommonPrereqModule.Tab.tabCurrentDocument, R.id.CTL_DOCUMENT_TILIST);
		CommonPrereqModule.TabEntry tab_entry = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
		if(tab_entry != null && tab_entry.TabView != null) {
			CPM.OnCurrentDocumentModification();
			HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
		}
	}
	private void SetupCurrentDocument(boolean gotoTabIfNotEmpty, boolean removeTabIfEmpty)
	{
		//
		// При попытке скрыть и потом показать табы они перерисовываются с искажениями.
		// по этому не будем злоупотреблять такими фокусами.
		//
		ScanSource = ScanType.Undef;
		if(!CPM.IsCurrentDocumentEmpty()) {
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.VISIBLE);
			if((CPM.GetActionFlags() & Document.actionDocSettingMarks) != 0) {
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclSetting, View.VISIBLE);
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.GONE);
				NotifyTabContentChanged(CommonPrereqModule.Tab.tabXclSetting, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
				ScanSource = ScanType.Setting;
			}
			else {
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclSetting, View.GONE);
				if((CPM.GetActionFlags() & Document.actionDocAcceptanceMarks) != 0) {
					CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.VISIBLE);
					ScanSource = ScanType.Veriy;
					NotifyTabContentChanged(CommonPrereqModule.Tab.tabXclVerify, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
				}
				else {
					CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.GONE);
				}
			}
			if(Document.DoesStatusAllowModifications(CPM.GetCurrentDocument().GetDocStatus())) {
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.VISIBLE);
			}
			else {
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.GONE);
			}
			if(gotoTabIfNotEmpty)
				GotoTab(CommonPrereqModule.Tab.tabCurrentDocument, R.id.CTL_DOCUMENT_TILIST, -1, -1);
		}
		else {
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.VISIBLE);
			if(removeTabIfEmpty)
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclSetting, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.GONE);
		}
	}
	private static class MarksByTiListAdapter extends SLib.InternalArrayAdapter {
		private Document.GoodsMarkSettingEntry Data;
		MarksByTiListAdapter(Context ctx, int rcId, Document.GoodsMarkSettingEntry data)
		{
			super(ctx, rcId, (data != null) ? data.XcL : null);
			Data = data;
		}
		Document.GoodsMarkSettingEntry GetData() { return Data; }
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			Context _ctx = getContext();
			//Context ctx = parent.getContext();
			if(item != null && _ctx != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null)
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				if(convertView != null) {
					TextView v = convertView.findViewById(R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_MARK);
					if(v != null && item instanceof Document.LotExtCode) {
						Document.LotExtCode entry = (Document.LotExtCode)item;
						v.setText(entry.Code);
					}
					//SLib.SetCtrlVisibility(convertView, R.id.CTL_BUTTON_MATCH, (Data == null || Data.Ti != null) ? View.GONE : View.VISIBLE);
					//SLib.SetCtrlVisibility(convertView, R.id.STDCTL_DELETEBUTTON, (Data == null || Data.Ti != null) ? View.GONE : View.VISIBLE);
					{
						{
							View btn = convertView.findViewById(R.id.CTL_BUTTON_MATCH);
							if(btn != null) {
								if(_ctx instanceof SLib.SlActivity && Data != null && Data.Ti == null) {
									btn.setVisibility(View.VISIBLE);
									btn.setOnClickListener(new View.OnClickListener() {
										@Override public void onClick(View v)
										{
											SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
											ev_subj.RvHolder = null;
											ev_subj.ItemIdx = position;
											ev_subj.ItemView = v;
											ev_subj.ItemObj = Data;
											((SLib.SlActivity)_ctx).HandleEvent(SLib.EV_COMMAND, v, ev_subj);
										}
									});
								}
								else
									btn.setVisibility(View.GONE);
							}
						}
						{
							View btn = convertView.findViewById(R.id.STDCTL_DELETEBUTTON);
							if(btn != null) {
								if(_ctx instanceof SLib.SlActivity && Data != null && Data.Ti == null) {
									btn.setVisibility(View.VISIBLE);
									btn.setOnClickListener(new View.OnClickListener() {
										@Override public void onClick(View v)
										{
											SLib.ListViewEvent ev_subj = new SLib.ListViewEvent();
											ev_subj.RvHolder = null;
											ev_subj.ItemIdx = position;
											ev_subj.ItemView = v;
											ev_subj.ItemObj = Data;
											((SLib.SlActivity)_ctx).HandleEvent(SLib.EV_COMMAND, v, ev_subj);
										}
									});
								}
								else
									btn.setVisibility(View.GONE);
							}
						}
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	//
	// Descr: Диалог акцепта строки документа
	//
	static class AcceptTiDialog extends SLib.SlDialog { // @v11.6.12
		Document.TransferItem Data;
		CommonPrereqModule CPM;
		CmdRIncomingListBillActivity ActivityParent;
		public AcceptTiDialog(Context ctx, CommonPrereqModule cpm, Document.TransferItem data)
		{
			super(ctx, R.id.DLG_TIACCEPT, data);
			CPM = cpm;
			if(ctx != null && ctx instanceof CmdRIncomingListBillActivity)
				ActivityParent = (CmdRIncomingListBillActivity)ctx;
			if(data instanceof Document.TransferItem)
				Data = Document.TransferItem.Copy((Document.TransferItem)data); // Copy!
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_ti_accept);
					SetDTS(Data);
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON) {
						this.dismiss();
					}
					else if(view_id == R.id.CTLBUT_DOACCEPT) {
						Document.TransferItem _data = (Data != null && Data instanceof Document.TransferItem) ? (Document.TransferItem)Data : null;
						if(_data != null && _data.Set != null) {
							if(_data.SetAccepted == null)
								_data.SetAccepted = new Document.ValuSet();
							_data.SetAccepted.Qtty = _data.Set.Qtty;
							SLib.SetCtrlString(this, R.id.CTL_TI_ACCEPTED_QTTY, SLib.formatdouble(_data.SetAccepted.Qtty, 3));
						}
					}
					else if(view_id == R.id.CTLBUT_DOREJECT) {
						Document.TransferItem _data = (Data != null && Data instanceof Document.TransferItem) ? (Document.TransferItem)Data : null;
						if(_data != null && _data.Set != null) {
							if(_data.SetAccepted == null)
								_data.SetAccepted = new Document.ValuSet();
							_data.SetAccepted.Qtty = 0;
							SLib.SetCtrlString(this, R.id.CTL_TI_ACCEPTED_QTTY, SLib.formatdouble(_data.SetAccepted.Qtty, 3));
						}
					}
					break;
			}
			return result;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
			if(app_ctx != null) {
				//Context ctx = getContext();
				Document.TransferItem _data = (Data != null && Data instanceof Document.TransferItem) ? (Document.TransferItem)Data : null;
				//String hint_text = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SELECTTIROWFORMARKSUBST);
				//SLib.SetCtrlString(this, R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_HINT, hint_text);
				if(_data != null) {
					if(_data.Set != null) {
						SLib.SetCtrlString(this, R.id.CTL_TI_COST, CPM.FormatCurrency(_data.Set.Cost));
						SLib.SetCtrlString(this, R.id.CTL_TI_PRICE, CPM.FormatCurrency(_data.Set.Price));
						SLib.SetCtrlString(this, R.id.CTL_TI_QTTY, SLib.formatdouble(_data.Set.Qtty, 3));
					}
					else {

					}
					if(_data.SetAccepted == null)
						_data.SetAccepted = new Document.ValuSet();
					SLib.SetCtrlString(this, R.id.CTL_TI_ACCEPTED_QTTY, SLib.formatdouble(_data.SetAccepted.Qtty, 3));
				}
			}
			return ok;
		}
		Object GetDTS()
		{
			Document.TransferItem _data = (Data != null && Data instanceof Document.TransferItem) ? (Document.TransferItem)Data : null;
			if(_data != null) {
				String qtty_text = SLib.GetCtrlString(this, R.id.CTL_TI_ACCEPTED_QTTY);
				double accepted_qtty = 0.0;
				if(SLib.GetLen(qtty_text) > 0) {
					STokenRecognizer.TokenArray nta = CPM.RecognizeToken(qtty_text);
					if(nta.Has(STokenRecognizer.SNTOK_NUMERIC_DOT) > 0.0f || nta.Has(STokenRecognizer.SNTOKSEQ_NUMERIC) > 0.0f)
						accepted_qtty = Double.parseDouble(qtty_text);
				}
				if(accepted_qtty > 0.0) {
					if(_data.SetAccepted == null)
						_data.SetAccepted = new Document.ValuSet();
					_data.SetAccepted.Qtty = accepted_qtty;
				}
				else if(_data.SetAccepted != null) {
					_data.SetAccepted.Qtty = accepted_qtty;
				}
				if(_data.SetAccepted != null && _data.Set != null) {
					if(_data.SetAccepted.Cost <= 0.0)
						_data.SetAccepted.Cost = _data.Set.Cost;
					if(_data.SetAccepted.Price <= 0.0)
						_data.SetAccepted.Price = _data.Set.Price;
				}
			}
			return Data;
		}
	}
	static class SubstTiForUnassignedMarkDialog extends SLib.SlDialog {
		CmdRIncomingListBillActivity ActivityParent;
		public static class DataBlock {
			DataBlock()
			{
				Doc = null;
				GmsEntry = null;
				CurEntry = null;
				SelectedIdx = -1;
			}
			Document Doc;
			Document.GoodsMarkSettingEntry GmsEntry;
			Document.LotExtCode CurEntry; // alias of one of GmsEntry.XcL
			int   SelectedIdx; // -1 - undefined or [0..Doc.TiList.size()-1]
		}
		public SubstTiForUnassignedMarkDialog(Context ctx, DataBlock data)
		{
			super(ctx, R.id.DLG_SUBSTTIFORUNASSIGNEDMARK, data);
			if(ctx != null && ctx instanceof CmdRIncomingListBillActivity)
				ActivityParent = (CmdRIncomingListBillActivity)ctx;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_substtiforunassignedmark);
					SetDTS(Data);
					break;
				case SLib.EV_LISTVIEWCOUNT:
					{
						SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter) srcObj;
						switch(a.GetListRcId()) {
							case R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_TILIST:
								{
									DataBlock _data = (Data != null && Data instanceof DataBlock) ? (DataBlock) Data : null;
									if(_data != null && _data.Doc != null && _data.Doc.TiList != null)
										result = new Integer(_data.Doc.TiList.size());
								}
								break;
						}
					}
					break;
				case SLib.EV_GETLISTITEMVIEW:
					{
						SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
						if(ev_subj != null && ev_subj.ItemIdx >= 0) {
							if(ev_subj.RvHolder != null) {
								// RecyclerView
								if(SLib.IsRecyclerListAdapter(srcObj)) {
									SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
									if(a.GetListRcId() == R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_TILIST) {
										DataBlock _data = (Data != null && Data instanceof DataBlock) ? (DataBlock)Data : null;
										if(_data != null && _data.Doc != null && _data.Doc.TiList != null && ev_subj.ItemIdx < _data.Doc.TiList.size()) {
											//CPM.FindGoodsItemByGoodsID()
											Document.TransferItem cur_entry = _data.Doc.TiList.get(ev_subj.ItemIdx);
											View iv = ev_subj.RvHolder.itemView;
											String goods_name = null;
											String qtty_text = null;
											if(ActivityParent != null) {
												CommonPrereqModule.WareEntry ware_entry = ActivityParent.CPM.FindGoodsItemByGoodsID(cur_entry.GoodsID);
												if(ware_entry != null && ware_entry.Item != null)
													goods_name = ware_entry.Item.Name;
											}
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_GOODSNAME, goods_name);
											int mark_qtty = 0;
											int item_qtty = (cur_entry.Set != null) ? (int)cur_entry.Set.Qtty : 0;
											if(cur_entry.XcL != null) {
												for(Document.LotExtCode iter : cur_entry.XcL) {
													mark_qtty++;
												}
											}
											qtty_text = Integer.toString(mark_qtty) + "/" + Integer.toString(Math.abs(item_qtty));
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_QTTYTEXT, qtty_text);
											if(_data.SelectedIdx == ev_subj.ItemIdx)
												iv.setBackgroundResource(R.drawable.shape_listitem_focused);
											else
												iv.setBackgroundResource(R.drawable.shape_listitem);
										}
									}
								}
							}
						}
					}
					break;
				case SLib.EV_LISTVIEWITEMCLK:
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent)subj : null;
					if(ev_subj != null) {
						DataBlock _data = (Data != null && Data instanceof DataBlock) ? (DataBlock)Data : null;
						if(_data != null && _data.Doc != null && _data.Doc.TiList != null) {
							if(ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < _data.Doc.TiList.size()) {
								_data.SelectedIdx = ev_subj.ItemIdx;
								if(SLib.IsRecyclerListAdapter(srcObj))
									((SLib.RecyclerListAdapter)srcObj).notifyDataSetChanged();
							}
						}
					}
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON) {
						this.dismiss();
					}
					break;
			}
			return result;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
			if(app_ctx != null) {
				//Context ctx = getContext();
				DataBlock _data = (Data != null && Data instanceof DataBlock) ? (DataBlock) Data : null;
				String hint_text = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_SELECTTIROWFORMARKSUBST);
				SLib.SetCtrlString(this, R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_HINT, hint_text);
				if(_data != null) {
					String mark = (_data.CurEntry != null) ? _data.CurEntry.Code : null;
					SLib.SetCtrlString(this, R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_MARK, mark);
					View lv = findViewById(R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_TILIST);
					if(lv != null && lv instanceof RecyclerView) {
						((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(/*getContext()*/ActivityParent));
						SLib.RecyclerListAdapter adapter = new SLib.RecyclerListAdapter(ActivityParent, this, R.id.CTL_SUBSTTIFORUNASSIGNEDMARK_TILIST, R.layout.li_substtiforunassignedmark);
						((RecyclerView) lv).setAdapter(adapter);
					}
				}
			}
			return ok;
		}
		Object GetDTS()
		{
			Object result = Data;
			return result;
		}
	}
	private void SetListBackground(View iv, Object adapter, int itemIdxToDraw, int objType, int objID)
	{
		int shaperc = 0;
		if(GetListFocusedIndex(adapter) == itemIdxToDraw)
			shaperc = R.drawable.shape_listitem_focused;
		else {
			boolean is_catched = false;
			if(objID > 0 && !CPM.IsCurrentDocumentEmpty()) {
				final Document _doc = CPM.GetCurrentDocument();
				if(objType == SLib.PPOBJ_PERSON && objID == _doc.H.ClientID) {
					is_catched = true;
				}
				else if(objType == SLib.PPOBJ_LOCATION && objID == _doc.H.DlvrLocID) {
					is_catched = true;
				}
				else if(objType == SLib.PPOBJ_GOODS) {
					if(CPM.HasGoodsInCurrentOrder(objID))
						is_catched = true;
				}
			}
			if(is_catched)
				shaperc = R.drawable.shape_listitem_catched;
			else if(CPM.IsObjInSearchResult(objType, objID))
				shaperc = R.drawable.shape_listitem_found;
			else
				shaperc = R.drawable.shape_listitem;
		}
		iv.setBackground(getResources().getDrawable(shaperc, getTheme()));
	}
	private boolean AddItemToCurrentOrder(Document.TransferItem item)
	{
		boolean result = CPM.AddTransferItemToCurrentDocument(item);
		if(result)
			NotifyCurrentDocumentChanged();
		return result;
	}
	private int ProcessSvcData(final JSONObject jsSvcReplyDoc, final JSONObject jsDocDecl)
	{
		int result = 0;
		if(jsSvcReplyDoc != null) {
			try {
				boolean nothing_to_do = jsSvcReplyDoc.optBoolean("nochanges", false);
				if(nothing_to_do)
					result = -1;
				else {
					ListType = ListDataType.Undef;
					CPM.GetCommonJsonFactors(jsSvcReplyDoc, jsDocDecl);
					DocStatusList = BusinessEntity.CliDocStatus.FromJson(jsSvcReplyDoc.optJSONArray("status_list"));
					CPM.MakeUomListFromCommonJson(jsSvcReplyDoc);
					CPM.MakeGoodsGroupListFromCommonJson(jsSvcReplyDoc);
					CPM.MakeGoodsListFromCommonJson(jsSvcReplyDoc);
					CPM.MakeBrandListFromCommonJson(jsSvcReplyDoc);
					CPM.MakeClientListFromCommonJson(jsSvcReplyDoc);
					CPM.MakeProcessorListFromCommonJson(jsSvcReplyDoc); // @v11.6.5
					CPM.MakeIncomingDocFromCommonJson(jsSvcReplyDoc);
					if(CPM.IncomingDocListData != null) {
						StyloQCommand.DocDeclaration doc_decl = CPM.GetDocDeclaration();
						if(doc_decl != null && SLib.GetLen(doc_decl.DisplayMethod) > 0) {
							if(doc_decl.DisplayMethod.equalsIgnoreCase("incominglistorder"))
								ListType = ListDataType.Bill;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("incominglistccheck"))
								ListType = ListDataType.CCheck;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("incominglisttsess"))
								ListType = ListDataType.TSess;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("incominglisttodo"))
								ListType = ListDataType.ToDo;
							else
								ListType = ListDataType.Undef;
						}
						for(int i = 0; i < CPM.IncomingDocListData.size(); i++) {
							Document doc_entry = CPM.IncomingDocListData.get(i);
							if(doc_entry != null)
								doc_entry.DetailExpandStatus_Ti = (doc_entry.TiList != null && doc_entry.TiList.size() > 0) ? 1 : 0;
						}
					}
					result = 1;
				}
			} catch(JSONException exn) {
				result = 0;
			}
		}
		return result;
	}
	private int ProcessSvcData(final String svcReplyDocJson, final JSONObject jsDocDecl)
	{
		int result = 0;
		if(SLib.GetLen(svcReplyDocJson) > 0) {
			try {
				result = ProcessSvcData(new JSONObject(svcReplyDocJson), jsDocDecl);
			} catch(JSONException exn) {
				result = 0;
			}
		}
		return result;
	}
	private void SetupTabVisibility(boolean refreshView)
	{
		final int action_flags = CPM.GetActionFlags();
		if(action_flags == 0) {
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.GONE);
		}
		else {
			{
				CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabCurrentDocument;
				CPM.SetTabVisibility(tab, View.VISIBLE);
				if(refreshView)
					NotifyTabContentChanged(tab, R.id.CTL_DOCUMENT_TILIST);
			}
			if((action_flags & (Document.actionGoodsItemCorrection|Document.actionCCheckCreat|Document.actionCCheckMod)) != 0) {
				{
					CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabGoodsGroups;
					CPM.SetTabVisibility(tab, View.VISIBLE);
					if(refreshView)
						NotifyTabContentChanged(tab, R.id.orderPrereqGoodsGroupListView);
				}
				{
					CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabBrands;
					if(CPM.BrandListData != null && CPM.BrandListData.size() > 0) {
						CPM.SetTabVisibility(tab, View.VISIBLE);
						NotifyTabContentChanged(tab, R.id.orderPrereqBrandListView);
					}
					else
						CPM.SetTabVisibility(tab, View.GONE);
				}
				{
					CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabGoods;
					CPM.SetTabVisibility(tab, View.VISIBLE);
					NotifyTabContentChanged(tab, R.id.orderPrereqGoodsListView);
				}
			}
			else {
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.GONE);
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
			}
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.GONE);
		}
		if(refreshView) {
			CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabIncomingList;
			NotifyTabContentChanged(tab, R.id.CTL_INCOMINGLIST_BILL_LIST);
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					Intent intent = getIntent();
					try {
						CPM.GetAttributesFromIntent(intent);
						long doc_id = intent.getLongExtra("SvcReplyDocID", 0);
						//StyloQCommand.DocDeclaration doc_decl = null; // @v11.6.6
						JSONObject js_doc_decl = null; // @v11.6.6
						String svc_reply_doc_json = null;
						StyloQApp app_ctx = GetAppCtx();
						StyloQDatabase db = (app_ctx != null) ? app_ctx.GetDB() : null;
						if(db != null) {
							CPM.SetupCurrentState(db); // @v11.7.0
							// @v11.9.0 {
							{
								StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, CPM.SvcIdent);
								if(svc_pack != null) {
									byte[] cfg_bytes = svc_pack.Pool.Get(SecretTagPool.tagConfig);
									if(SLib.GetLen(cfg_bytes) > 0) {
										StyloQConfig svc_cfg = new StyloQConfig();
										if(svc_cfg.FromJson(new String(cfg_bytes))) {
											final int cli_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagCliFlags));
											if((cli_flags & StyloQConfig.clifShareDoc) != 0)
												DocsSharingEnabled = true;
										}
									}
								}
							}
							// } @v11.9.0
							ArrayList <UUID> possible_doc_uuid_list = null;
							if(doc_id > 0) {
								StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
								if(doc_packet != null) {
									byte[] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_doc) > 0)
										svc_reply_doc_json = new String(raw_doc);
									js_doc_decl = doc_packet.Pool.GetJsonObject(SecretTagPool.tagDocDeclaration); // @v11.6.6
								}
							}
							else
								svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
							if(ProcessSvcData(svc_reply_doc_json, js_doc_decl) > 0) {
								if(CPM.IncomingDocListData != null) {
									if(possible_doc_uuid_list == null)
										possible_doc_uuid_list = new ArrayList<UUID>();
									for(int i = 0; i < CPM.IncomingDocListData.size(); i++) {
										Document doc_entry = CPM.IncomingDocListData.get(i);
										if(doc_entry != null) {
											if(doc_entry.H != null && doc_entry.H.Uuid != null)
												possible_doc_uuid_list.add(doc_entry.H.Uuid);
											doc_entry.DetailExpandStatus_Ti = (doc_entry.TiList != null && doc_entry.TiList.size() > 0) ? 1 : 0;
										}
									}
								}
							}
							MakeSimpleSearchIndex(); // @v11.7.6
							//
							requestWindowFeature(Window.FEATURE_NO_TITLE);
							setContentView(R.layout.activity_cmdrincominglist_bill);
							CPM.SetupActivity(db, R.id.VIEWPAGER_INCOMINGLISTBILL, R.id.TABLAYOUT_INCOMINGLISTBILL);

							ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_INCOMINGLISTBILL);
							SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_INCOMINGLISTBILL);
							{
								TabLayout lo_tab = findViewById(R.id.TABLAYOUT_INCOMINGLISTBILL);
								if(lo_tab != null) {
									CreateTabList(false);
									for(int i = 0; i < CPM.TabList.size(); i++) {
										TabLayout.Tab tab = lo_tab.newTab();
										tab.setText(CPM.TabList.get(i).TabText);
										lo_tab.addTab(tab);
									}
									SLib.SetupTabLayoutStyle(lo_tab);
									SLib.SetupTabLayoutListener(this, lo_tab, view_pager);
								}
							}
							SetupTabVisibility(false);
							SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
							if(possible_doc_uuid_list != null && possible_doc_uuid_list.size() > 0 && CPM.RestoreRecentIncomingModDocumentAsCurrent(possible_doc_uuid_list))
								SetupCurrentDocument(true, true);
							else
								SetupCurrentDocument(false, true);
							{
								CPM.Callback_BackButton = new OnBackPressedCallback(true /* enabled by default */) {
									@Override public void handleOnBackPressed() { CPM.BackTab(R.id.VIEWPAGER_INCOMINGLISTBILL); }
								};
								getOnBackPressedDispatcher().addCallback(this, CPM.Callback_BackButton);
							}
							// @v11.6.2 {
							{
								//
								// Задача для регулярного обновления данных
								//
								class TimerTask_RefreshSvcDataPoll extends TimerTask {
									private StyloQApp AppCtx;
									private SLib.EventHandler Handler;
									TimerTask_RefreshSvcDataPoll(StyloQApp appCtx, SLib.EventHandler handler)
									{
										AppCtx = appCtx;
										Handler = handler;
									}
									@Override public void run()
									{
										if(CPM.CmdUuid != null && Handler != null) {
											// @v11.6.4 {
											long last_time_diff = (LastRefreshTime > 0) ? (System.currentTimeMillis() - LastRefreshTime) : 0;
											if(last_time_diff == 0 || last_time_diff > RefreshSvcDataPollPeriodMs) {
												Thread thr = new Thread(new ThreadEngine_RefreshDataSvcPoll(AppCtx, CPM.SvcIdent, CPM.CmdUuid, Handler));
												thr.start();
											}
											// } @v11.6.4
											//runOnUiThread(new Runnable() { @Override public void run() { DocStatusPoll(AppCtx); }});
										}
									}
								}
								RefreshSvcDataPollTmr = new Timer();
								RefreshSvcDataPollTmr.schedule(new TimerTask_RefreshSvcDataPoll(app_ctx, this), 1 * 60 * 1000, RefreshSvcDataPollPeriodMs);
							}
							// } @v11.6.2
						}
					} catch(StyloQException exn) {
						;
					}
				}
				break;
			case SLib.EV_TABSELECTED:
				{
					final CommonPrereqModule.Tab tab_id = CPM.OnTabSelection(subj);
					// @v11.7.6 {
					SLib.SetCtrlVisibility(this, R.id.tbButtonLocalTabConfig, (tab_id == CommonPrereqModule.Tab.tabRegistry) ? View.VISIBLE : View.GONE);
					SLib.SetCtrlVisibility(this, R.id.tbButtonHelp, (tab_id == CommonPrereqModule.Tab.tabRegistry) ? View.VISIBLE : View.GONE);
					if(tab_id == CommonPrereqModule.Tab.tabSearch) {
						CommonPrereqModule.TabEntry te = SearchTabEntry(tab_id);
						if(te != null && te.TabView != null) {
							View v = te.TabView.getView();
							if(v != null && v instanceof ViewGroup) {
								View ftv = ((ViewGroup)v).findViewById(R.id.searchPaneListView);
								if(ftv != null)
									ftv.requestFocus();
							}
						}
					}
					// } @v11.7.6
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				if(srcObj instanceof SLib.FragmentAdapter) {
					CreateTabList(false);
					result = new Integer(CPM.TabList.size());
				}
				else if(SLib.IsRecyclerListAdapter(srcObj)) {
					SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
					switch(a.GetListRcId()) {
						case R.id.CTL_INCOMINGLIST_BILL_LIST: result = new Integer(CPM.IncomingDocListData != null ? CPM.IncomingDocListData.size() : 0); break;
						case R.id.CTL_DOCUMENT_TILIST: result = new Integer(CPM.GetCurrentDocumentTransferListCount()); break;
						case R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST:
							{
								Document _doc = CPM.GetCurrentDocument();
								if(ScanSource == ScanType.Veriy) {
									result = new Integer((_doc == null || _doc.VXcL == null) ? 0 : _doc.VXcL.size());
								}
								else if(ScanSource == ScanType.Setting) {
									result = new Integer((_doc != null) ? _doc.GetGoodsMarkSettingListCount() : 0);
								}
								else
									result = new Integer(0);
							}
							break;
						case R.id.orderPrereqGoodsListView: result = new Integer(CPM.GetGoodsListSize()); break;
						case R.id.orderPrereqGoodsGroupListView: result = new Integer(CPM.GetGoodsGroupCount()); break;
						case R.id.orderPrereqBrandListView: result = new Integer(SLib.GetCount(CPM.BrandListData)); break;
						case R.id.searchPaneListView: result = new Integer(CPM.SearchResult_GetObjTypeCount()); break;
						/*
						case R.id.orderPrereqOrdrListView: result = new Integer(CPM.GetCurrentDocumentTransferListCount()); break;
						case R.id.orderPrereqOrderListView:
							result = new Integer((CPM.OrderHList != null) ? CPM.OrderHList.size() : 0);
							break;
						case R.id.orderPrereqClientsListView: result = new Integer((CPM.CliListData != null) ? CPM.CliListData.size() : 0); break;
						 */
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemIdx >= 0) {
						if(ev_subj.RvHolder != null) { // RecyclerView
							if(SLib.IsRecyclerListAdapter(srcObj)) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter) srcObj;
								switch(a.GetListRcId()) {
									case R.id.searchPaneListView: // @v11.7.6
										CPM.GetSearchPaneListViewItem(ev_subj.RvHolder.itemView, ev_subj.ItemIdx);
										break;
									case R.id.CTL_DOCUMENT_TILIST:
										final int cc = CPM.GetCurrentDocumentTransferListCount();
										if(ev_subj.ItemIdx < cc) {
											StyloQApp app_ctx = GetAppCtx();
											View iv = ev_subj.RvHolder.itemView;
											if(iv != null) {
												if(app_ctx != null && iv instanceof ViewGroup)
													SLib.SubstituteStringSignatures(app_ctx, (ViewGroup)iv);
												final Document _doc = CPM.GetCurrentDocument();
												final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
												if(ti != null) {
													CommonPrereqModule.WareEntry goods_item = CPM.FindGoodsItemByGoodsID(ti.GoodsID);
													int uom_id = (goods_item != null && goods_item.Item != null) ? goods_item.Item.UomID : 0;
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_GOODSNAME, (goods_item != null) ? goods_item.Item.Name : "");
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_PRICE, (ti.Set != null) ? CPM.FormatCurrency(ti.Set.Price) : "");
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_QTTY, (ti.Set != null) ? CPM.FormatQtty(ti.Set.Qtty, uom_id, false) : "");
													double item_amont = (ti.Set != null) ? (ti.Set.Qtty * ti.Set.Price) : 0.0;
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_AMOUNT, " = " + CPM.FormatCurrency(item_amont));
													if(ti.SetAccepted != null && ti.SetAccepted.Qtty > 0.0) {
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_ACCPTED_PRICE, CPM.FormatCurrency(ti.SetAccepted.Price));
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_ACCPTED_QTTY, CPM.FormatQtty(ti.SetAccepted.Qtty, uom_id, false));
														item_amont = (ti.Set != null) ? (ti.SetAccepted.Qtty * ti.SetAccepted.Price) : 0.0;
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_ACCPTED_AMOUNT, " = " + CPM.FormatCurrency(item_amont));
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_TI_ACCEPTED_TEXT, View.VISIBLE);
													}
													else {
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_TI_ACCEPTED_TEXT, View.GONE);
													}
													// @v11.7.0 {
													{
														ImageView ctl = (ImageView) iv.findViewById(R.id.CTL_DOCUMENT_TI_ACCEPTED_STATUS);
														if(ctl != null) {
															int rcid = 0;
															if(ti.Set != null && ti.SetAccepted != null) {
																if(ti.SetAccepted.Qtty > 0.0)
																	rcid = R.drawable.ic_accept01;
																else if(ti.SetAccepted.Qtty == 0.0)
																	rcid = R.drawable.ic_reject01;
															}
															if(rcid != 0) {
																ctl.setVisibility(View.VISIBLE);
																ctl.setImageResource(rcid);
															}
															else
																ctl.setVisibility(View.GONE);
														}
													}
													// } @v11.7.0
												}
											}
										}
										break;
									case R.id.orderPrereqGoodsListView:
										{
											CommonPrereqModule.WareEntry cur_entry = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
											if(cur_entry != null && cur_entry.Item != null) {
												//
												// @todo С этим элементом есть проблема: если пользователь на телефоне поставил слишком большое масштабирование
												// экрана, то значок корзинки становится невидимым (уходит за правую грань экрана).
												// 1. Я попробовал сделать горизонтальное скроллирование в layout_orderprereq_goods.xml, но в этом случае
												// вся табличка становится шире и корзинка всегда уходит за границу.
												// 2. В li_orderprereq_goods.xml убирал <View style="@style/FakeView"/>. В этом случае в зависимости от наличия/
												// отсутствия изображения товара раскладка меняется - не красиво. Это можно компенсировать параметром
												// dontRemoveIfNoImg=true в функции SLib.SetupImage() но тогда, если сервис не предоставляет картинки товаров,
												// для полезной информации остается мало места.
												//
												// В общем, пока не ясно как с этим бороться.
												//
												final int cur_id = cur_entry.Item.ID;
												View iv = ev_subj.RvHolder.itemView;
												SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.Item.Name);
												double val = 0.0;
												val = cur_entry.Item.Price;
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_PRICE, (val > 0.0) ? CPM.FormatCurrency(val) : "");
												val = cur_entry.Item.Stock;
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_REST, (val > 0.0) ? SLib.formatdouble(val, 0) : "");
												if(val > 0) {

												}
												else {

												}
												val = CPM.GetGoodsQttyInCurrentDocument(cur_id);
												if(val > 0.0) {
													SLib.SetCtrlVisibility(iv, R.id.ORDERPREREQ_GOODS_ORDEREDQTY, View.VISIBLE);
													SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_ORDEREDQTY, String.format("%.0f", val));
												}
												else {
													SLib.SetCtrlVisibility(iv, R.id.ORDERPREREQ_GOODS_ORDEREDQTY, View.GONE);
												}
												SLib.SetupImage(this, iv.findViewById(R.id.ORDERPREREQ_GOODS_IMG), cur_entry.Item.ImgBlob, false);
												SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODS, cur_id);
											}
										}
										break;
									case R.id.orderPrereqGoodsGroupListView:
										if(ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GetGoodsGroupCount()) {
											View iv = ev_subj.RvHolder.itemView;
											BusinessEntity.GoodsGroup cur_entry = CPM.GoodsGroupListData.L.get(ev_subj.ItemIdx);
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.Name);
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODSGROUP, cur_entry.ID);
										}
										break;
									case R.id.orderPrereqBrandListView:
										if(SLib.IsInRange(ev_subj.ItemIdx, CPM.BrandListData)) {
											View iv = ev_subj.RvHolder.itemView;
											BusinessEntity.Brand cur_entry = CPM.BrandListData.get(ev_subj.ItemIdx);
											if(cur_entry != null) {
												SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.Name);
												SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_BRAND, cur_entry.ID);
											}
										}
										break;
									case R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST:
										Document _doc = CPM.GetCurrentDocument();
										if(_doc != null) {
											if(ScanSource == ScanType.Veriy) {
												if(SLib.IsInRange(ev_subj.ItemIdx, _doc.VXcL)) {
													View iv = ev_subj.RvHolder.itemView;
													Document.LotExtCode cur_entry = _doc.VXcL.get(ev_subj.ItemIdx);
													if(cur_entry != null) {
														SLib.SetCtrlString(iv, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_MARK, cur_entry.Code);
														Document.GoodsMarkStatus gms = _doc.GetVerificationGoodsMarkStatus(cur_entry.Code);
														int shaperc = 0;
														if(gms == Document.GoodsMarkStatus.Matched)
															shaperc = R.drawable.shape_goodsmark_matched;
														else if(gms == Document.GoodsMarkStatus.Unmatched)
															shaperc = R.drawable.shape_goodsmark_unmatched;
														else
															shaperc = R.drawable.shape_goodsmark_absent; // ?
														iv.setBackground(getResources().getDrawable(shaperc, getTheme()));
													}
												}
											}
											else if(ScanSource == ScanType.Setting) {
												Document.GoodsMarkSettingEntry cur_entry = _doc.GetGoodsMarkSettingListItem(ev_subj.ItemIdx);
												if(cur_entry != null) {
													View iv = ev_subj.RvHolder.itemView;
													String title_text = null;
													if(cur_entry.Ti != null) {
														CommonPrereqModule.WareEntry goods_entry = CPM.FindGoodsItemByGoodsID(cur_entry.Ti.GoodsID);
														String qtty_text = null;
														if(goods_entry != null && goods_entry.Item != null) {
															title_text = goods_entry.Item.Name;
															if(cur_entry.Ti.Set != null)
																qtty_text = CPM.FormatQtty(cur_entry.Ti.Set.Qtty, goods_entry.Item.UomID, false);
														}
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_QTTY, qtty_text);
													}
													else {
														StyloQApp app_ctx = GetAppCtx();
														if(app_ctx != null)
															title_text = app_ctx.GetString("unmatchedmarks");
													}
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_GOODSNAME, title_text);
													if(cur_entry.XcL != null) {
														ListView mark_lv = (ListView) iv.findViewById(R.id.CTL_INCOMINGLIST_BILL_MARKSBYTI);
														if(mark_lv != null) {
															MarksByTiListAdapter adapter = new MarksByTiListAdapter(iv.getContext(), R.layout.li_incominglist_bill_scanmarks, cur_entry);
															mark_lv.setAdapter(adapter);
															{
																int total_items_height = SLib.CalcListViewHeight(mark_lv);
																if(total_items_height > 0) {
																	ViewGroup.LayoutParams params = mark_lv.getLayoutParams();
																	params.height = total_items_height;
																	mark_lv.setLayoutParams(params);
																	mark_lv.requestLayout();
																}
															}
															adapter.setNotifyOnChange(true);
															mark_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
																@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
																{
																	Object item = (Object) parent.getItemAtPosition(position);
																	Context ctx = parent.getContext();
																	if(item != null && ctx != null && ctx instanceof SLib.SlActivity) {
																		SLib.SlActivity activity = (SLib.SlActivity) ctx;
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
										break;
									case R.id.CTL_INCOMINGLIST_BILL_LIST:
										if(CPM.IncomingDocListData != null && ev_subj.ItemIdx < CPM.IncomingDocListData.size()) {
											StyloQApp app_ctx = GetAppCtx();
											View iv = ev_subj.RvHolder.itemView;
											Document cur_entry = CPM.IncomingDocListData.get(ev_subj.ItemIdx);
											if(app_ctx != null && cur_entry.H != null) {
												{
													if(SLib.GetLen(cur_entry.H.Code) > 0)
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_CODE, cur_entry.H.Code);
													SLib.LDATE d = cur_entry.GetNominalDate();
													if(d != null)
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_DATE, d.Format(SLib.DATF_ISO8601 | SLib.DATF_CENTURY));
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_AMOUNT, CPM.FormatCurrency(cur_entry.H.Amount));
													JSONObject cli_entry = (cur_entry.H.ClientID > 0) ? CPM.FindClientEntry(cur_entry.H.ClientID) : null;
													if(cli_entry != null) {
														String nm = cli_entry.optString("nm", null);
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_CLI, View.VISIBLE);
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_CLI, nm);
													}
													else
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_CLI, View.GONE);
													if(SLib.GetLen(cur_entry.H.Memo) > 0) {
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_MEMO, View.VISIBLE);
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_MEMO, cur_entry.H.Memo);
													}
													else
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_MEMO, View.GONE);
												}
												if(ListType == ListDataType.TSess) {
													if(cur_entry.BkList != null && cur_entry.BkList.size() > 0) {
														Document.BookingItem bk_item = cur_entry.BkList.get(0);
														String prc_name = null;
														String timechunk_text = null;
														if(bk_item != null) {
															if(bk_item.PrcID > 0) {
																CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(bk_item.PrcID);
																if(prc_entry != null)
																	prc_name = prc_entry.JsItem.optString("nm", "");
																SLib.STimeChunk tc = bk_item.GetEsimatedTimeChunk();
																if(tc != null)
																	timechunk_text = tc.Format(SLib.DATF_DMY, SLib.TIMF_HM);
															}
														}
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_PRC, prc_name);
														SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TIMECHUNK, timechunk_text);
													}
												}
												else {
													if(cur_entry.H.AgentID > 0)
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_AGENT, View.VISIBLE);
													else
														SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_AGENT, View.VISIBLE);
													SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_SHARE, (DocsSharingEnabled ? View.VISIBLE : View.GONE)); // @v11.9.0
													{
														ImageView ctl = (ImageView)iv.findViewById(R.id.CTL_DOCUMENT_EXPANDSTATUS);
														if(ctl != null) {
															ListView detail_lv = (ListView)iv.findViewById(R.id.CTL_DOCUMENT_DETAILLIST);
															if(detail_lv != null) {
																if(cur_entry.TiList != null) {
																	ctl.setVisibility(View.VISIBLE);
																	if(cur_entry.DetailExpandStatus_Ti == 0 || detail_lv == null) {
																		ctl.setVisibility(View.GONE);
																		SLib.SetCtrlVisibility(detail_lv, View.GONE);
																	}
																	else if(cur_entry.DetailExpandStatus_Ti == 1) {
																		ctl.setVisibility(View.VISIBLE);
																		ctl.setImageResource(R.drawable.ic_triangleleft03);
																		SLib.SetCtrlVisibility(detail_lv, View.GONE);
																	}
																	else if(cur_entry.DetailExpandStatus_Ti == 2) {
																		ctl.setVisibility(View.VISIBLE);
																		ctl.setImageResource(R.drawable.ic_triangledown03);
																		if(detail_lv != null) {
																			SLib.SetCtrlVisibility(detail_lv, View.VISIBLE);
																			DetailTiListAdapter adapter = new DetailTiListAdapter(iv.getContext(), R.layout.li_transferitem_generic, cur_entry);
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
																		}
																	}
																}
																else
																	ctl.setVisibility(View.GONE);
															}
														}
													}
												}
												SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_BILL, (int)cur_entry.H.ID); // @v11.7.6
											}
										}
										break;
									}
							}
						}
					}
				}
				break;
			case SLib.EV_CBSELECTED:
				if(subj != null && subj instanceof SLib.ListViewEvent) {
					SLib.ListViewEvent lve = (SLib.ListViewEvent)subj;
					if(lve.ItemIdx >= 0 && lve.ItemId >= 0) {
						View v = findViewById(R.id.VIEWPAGER_INCOMINGLISTBILL);
						if(v != null && v instanceof ViewPager2) {
							for(CommonPrereqModule.TabEntry iter : CPM.TabList) {
								if(iter != null && iter.TabId == CommonPrereqModule.Tab.tabCurrentDocument) {
									//te = iter;
									Document _doc = CPM.GetCurrentDocument();
									if(_doc != null && _doc.H != null) {
										_doc.H.StatusSurrId = (int)lve.ItemId;
									}
									break;
								}
							}
						}
					}
				}
				break;
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					StyloQApp app_ctx = GetAppCtx();
					ViewGroup vg = (ViewGroup) srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_INCOMINGLIST_BILL_SELECTED) {
						int status_image_rc_id = 0;
						if(CPM.IsCurrentDocumentEmpty()) {
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CLI, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DLVRLOC, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_AMOUNT, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO, "");
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON1, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON2, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON3, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON4, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_DUEDATE_NEXT, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_DUEDATE_PREV, View.GONE);
						}
						else {
							Document _doc = CPM.GetCurrentDocument();
							if(SLib.GetLen(_doc.H.Code) > 0)
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, _doc.H.Code);
							{
								SLib.LDATE d = _doc.GetNominalDate();
								if(d != null)
									SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, d.Format(SLib.DATF_ISO8601 | SLib.DATF_CENTURY));
							}
							{
								if(_doc.H.DueTime == null || !SLib.CheckDate(_doc.H.DueTime.d)) {
									SLib.LDATETIME time_base = _doc.H.GetNominalTimestamp();
									if(time_base != null) {
										if(CPM.GetDefDuePeriodHour() > 0) {
											_doc.H.DueTime = SLib.plusdatetimesec(time_base, CPM.GetDefDuePeriodHour() * 3600);
										}
									}
								}
								if(_doc.H.DueTime != null && SLib.CheckDate(_doc.H.DueTime.d)) {
									SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DUEDATE, _doc.H.DueTime.d.Format(SLib.DATF_ISO8601 | SLib.DATF_CENTURY));
								}
								SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_DUEDATE_NEXT, _doc.H.IncrementDueDate(true) ? View.VISIBLE : View.GONE);
								SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_DUEDATE_PREV, _doc.H.DecrementDueDate(true) ? View.VISIBLE : View.GONE);
								{
									View btn = vg.findViewById(R.id.CTL_DOCUMENT_DUEDATE_PREV);
									if(btn != null) {
										btn.setOnClickListener(new View.OnClickListener() {
											@Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }
										});
									}
								}
								{
									View btn = vg.findViewById(R.id.CTL_DOCUMENT_DUEDATE_NEXT);
									if(btn != null) {
										btn.setOnClickListener(new View.OnClickListener() {
											@Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }
										});
									}
								}
							}
							// Комбо-бокс выбора статуса документа
							if((CPM.GetActionFlags() & Document.actionDocStatus) != 0 && DocStatusList != null && DocStatusList.size() > 0) {
								SLib.SetCtrlVisibility(vg, R.id.CTLGRP_DOCUMENT_STATUS, View.VISIBLE);
								SLib.StrAssocArray str_list = new SLib.StrAssocArray();
								str_list.Set(1000, app_ctx.GetString("unchanged"));
								int iter_surr_id = 0;
								for(int i = 0; i < DocStatusList.size(); i++) {
									final BusinessEntity.CliDocStatus item = DocStatusList.get(i);
									if(item != null && SLib.GetLen(item.Name) > 0) {
										int surr_id = 0;
										if(item.SurrogateId > 0)
											surr_id = item.SurrogateId;
										else
											surr_id = iter_surr_id+1;
										iter_surr_id = surr_id;
										str_list.Set(surr_id, item.Name);
									}
								}
								SLib.SetupStrAssocCombo(app_ctx, vg, R.id.CTLSEL_DOCUMENT_STATUS, str_list, _doc.H.StatusSurrId);
							}
							else {
								SLib.SetCtrlVisibility(vg, R.id.CTLGRP_DOCUMENT_STATUS, View.GONE);
							}
							{
								String cli_name = "";
								String addr = "";
								if(_doc.H.ClientID > 0) {
									JSONObject cli_entry = CPM.FindClientEntry(_doc.H.ClientID);
									if(cli_entry != null)
										cli_name = cli_entry.optString("nm", "");
								}
								if(_doc.H.DlvrLocID > 0) {
									JSONObject cli_entry = CPM.FindClientEntryByDlvrLocID(_doc.H.DlvrLocID);
									if(cli_entry != null) {
										JSONObject dlvrlov_entry = CPM.FindDlvrLocEntryInCliEntry(cli_entry, _doc.H.DlvrLocID);
										if(dlvrlov_entry != null)
											addr = dlvrlov_entry.optString("addr", "");
									}
								}
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CLI, cli_name);
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DLVRLOC, addr);
							}
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO, _doc.H.Memo);
							{
								double amount = CPM.GetAmountOfCurrentDocument();
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_AMOUNT, CPM.FormatCurrency(amount));
							}
							{
								DocEditActionList = Document.GetEditActionsConnectedWithStatus(_doc.GetDocStatus());
								if(DocEditActionList != null && DocEditActionList.size() > 0) {
									int acn_idx = 0;
									for(; acn_idx < DocEditActionList.size(); acn_idx++) {
										Document.EditAction acn = DocEditActionList.get(acn_idx);
										switch(acn_idx) {
											case 0:
												SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON1, View.VISIBLE);
												SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON1, acn.GetTitle(app_ctx));
												break;
											case 1:
												SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON2, View.VISIBLE);
												SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON2, acn.GetTitle(app_ctx));
												break;
											case 2:
												SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON3, View.VISIBLE);
												SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON3, acn.GetTitle(app_ctx));
												break;
											case 3:
												SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON4, View.VISIBLE);
												SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON4, acn.GetTitle(app_ctx));
												break;
										}
									}
									for(; acn_idx < 4; acn_idx++) {
										switch(acn_idx) {
											case 0: SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON1, View.GONE); break;
											case 1: SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON2, View.GONE); break;
											case 2: SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON3, View.GONE); break;
											case 3: SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON4, View.GONE); break;
										}
									}
								}
							}
							{
								View v = vg.findViewById(R.id.CTL_DOCUMENT_STATUSICON);
								if(v != null && v instanceof ImageView) {
									status_image_rc_id = Document.GetImageResourceByDocStatus(_doc.GetDocStatus());
									if(status_image_rc_id == 0)
										v.setVisibility(View.GONE);
								}
							}
							CPM.DrawCurrentDocumentRemoteOpIndicators();
						}
					}
				}
				break;
			case SLib.EV_GETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup)
					GetFragmentData(srcObj);
				break;
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder == null) {
							;
						}
						else {
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.CTL_DOCUMENT_EXPANDSTATUS);
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.buttonOrder);
							result = ev_subj.RvHolder;
						}
					}
				}
				break;
			case SLib.EV_CREATEFRAGMENT: result = CPM.OnEvent_CreateFragment(subj); break;
			case SLib.EV_SETUPFRAGMENT:
				if(subj != null && subj instanceof View) {
					//final int selected_search_idx = CPM.SearchResult_GetSelectedItemIndex();
					//final int selected_search_objtype = (selected_search_idx >= 0) ? CPM.SearchResult.List.get(selected_search_idx).ObjType : 0;
					//final int selected_search_objid = (selected_search_idx >= 0) ? CPM.SearchResult.List.get(selected_search_idx).ObjID : 0;
					final SLib.PPObjID selected_search_oid = CPM.SearchResult_GetSelectedOid();
					if(srcObj != null && srcObj instanceof SLib.SlFragmentStatic) {
						SLib.SlFragmentStatic fragment = (SLib.SlFragmentStatic) srcObj;
						View lv = null;
						View fv = (View)subj;
						boolean settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_GOODS, fv, R.id.orderPrereqGoodsListView, R.layout.li_orderprereq_goods);
						if(!settled) {
							settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_GOODSGROUP, fv, R.id.orderPrereqGoodsGroupListView, R.layout.li_simple);
							if(!settled) {
								settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_BRAND, fv, R.id.orderPrereqBrandListView, R.layout.li_simple);
								if(!settled) {
									lv = fv.findViewById(R.id.CTL_INCOMINGLIST_BILL_LIST);
									if(lv != null) {
										((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
										if(ListType == ListDataType.TSess)
											SetupRecyclerListView(fv, R.id.CTL_INCOMINGLIST_BILL_LIST, R.layout.li_incominglist_tsess_main);
										else
											SetupRecyclerListView(fv, R.id.CTL_INCOMINGLIST_BILL_LIST, R.layout.li_incominglist_bill_main);
									}
									else {
										lv = fv.findViewById(R.id.CTL_DOCUMENT_TILIST);
										if(lv != null) {
											((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
											SetupRecyclerListView(fv, R.id.CTL_DOCUMENT_TILIST, R.layout.li_incominglist_bill_ti);
										}
										else {
											lv = fv.findViewById(R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
											if(lv != null) {
												((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
												if(ScanSource == ScanType.Veriy)
													SetupRecyclerListView(fv, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST, R.layout.li_incominglist_bill_scanmarks);
												else if(ScanSource == ScanType.Setting)
													SetupRecyclerListView(fv, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST, R.layout.li_incominglist_bill_scanmarks_setting);
											}
											else { // @v11.7.6
												lv = fv.findViewById(R.id.searchPaneListView);
												if(lv != null)
													CPM.SetupSearchPaneListView(fv, lv);
											}
										}
									}
								}
							}
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && srcObj != null) {
						boolean debug_mark = false;
						if(ev_subj.RvHolder == null) {
							if(ev_subj.ItemView != null) {
								if(srcObj instanceof ListView && ev_subj.ItemObj != null) {
									ListView lv = (ListView)srcObj;
									/*
									switch(lv.getId()) {
										case R.id.CTL_INCOMINGLIST_BILL_MARKSBYTI:
											debug_mark = true;
											//MarksByTiListAdapter
											Adapter a = lv.getAdapter();
											if(a != null && a instanceof MarksByTiListAdapter) {
												Document.GoodsMarkSettingEntry gms_entry = ((MarksByTiListAdapter)a).GetData();
												if(gms_entry != null && CPM.GetCurrentDocument() != null) {
													SubstTiForUnassignedMarkDialog.DataBlock _data = new SubstTiForUnassignedMarkDialog.DataBlock();
													_data.Doc = CPM.GetCurrentDocument();
													_data.GmsEntry = gms_entry;
													_data.CurEntry = (gms_entry.XcL != null) ? gms_entry.XcL.get(ev_subj.ItemIdx) : null;
													Document.GoodsMarkSettingEntry gse = (_data.Doc != null) ? _data.Doc.GetGoodsMarkSettingListItem(ev_subj.ItemIdx) : null;
													SubstTiForUnassignedMarkDialog dlg = new SubstTiForUnassignedMarkDialog(this, _data);
													dlg.show();
												}
											}
											break;
									}*/
									if(lv.getId() == R.id.searchPaneTerminalListView) {
										final SLib.PPObjID sr_oid = CPM.SearchResult_ProcessSelection(ev_subj.ItemObj);
										if(sr_oid != null && !sr_oid.IsEmpty()) {
											int _idx = -1;
											switch(sr_oid.Type) {
												case SLib.PPOBJ_GOODS:
													_idx = CPM.FindGoodsItemIndexByID(sr_oid.Id);
													if(CPM.Gf != null) {
														CommonPrereqModule.WareEntry goods_item = CPM.GetGoodsListItemByIdx(_idx);
														if(!CPM.CheckGoodsListItemForFilt(goods_item)) {
															CPM.ResetGoodsFiter();
															SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
														}
													}
													GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, _idx, -1);
													break;
												case SLib.PPOBJ_PERSON:
													_idx = CPM.FindClientItemIndexByID(sr_oid.Id);
													GotoTab(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView, _idx, -1);
													break;
												case SLib.PPOBJ_LOCATION:
													JSONObject cli_js_obj = CPM.FindClientEntryByDlvrLocID(sr_oid.Id);
													if(cli_js_obj != null) {
														int cli_id = cli_js_obj.optInt("id", 0);
														if(cli_id > 0) {
															_idx = CPM.FindClientItemIndexByID(cli_id);
															int _dlvr_loc_idx = CPM.FindDlvrLocEntryIndexInCliEntry(cli_js_obj, sr_oid.Id);
															GotoTab(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView, _idx, _dlvr_loc_idx);
														}
													}
													//tab_to_select = Tab.tabClients;
													break;
												case SLib.PPOBJ_GOODSGROUP:
													_idx = CPM.FindGoodsGroupItemIndexByID(sr_oid.Id);
													GotoTab(CommonPrereqModule.Tab.tabGoodsGroups, R.id.orderPrereqGoodsGroupListView, _idx, -1);
													break;
												case SLib.PPOBJ_BRAND:
													_idx = CPM.FindBrandItemIndexByID(sr_oid.Id);
													GotoTab(CommonPrereqModule.Tab.tabBrands, R.id.orderPrereqBrandListView, _idx, -1);
													break;
												case SLib.PPOBJ_BILL:
													_idx = CPM.FindIncomingDocumentItemIndexByID(sr_oid.Id);
													GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, _idx, -1);
													break;
											}
										}
									}
								}
							}
						}
						else if(SLib.IsRecyclerListAdapter(srcObj)) {
							SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
							StyloQApp app_ctx = GetAppCtx();
							boolean do_update_goods_list_and_toggle_to_it = false;
							final int rc_id = a.GetListRcId();
							if(app_ctx != null && ev_subj.ItemIdx >= 0) {
								final int actf = CPM.GetActionFlags();
								switch(rc_id) {
									case R.id.CTL_DOCUMENT_TILIST:
										if((actf & (Document.actionCCheckMod|Document.actionCCheckCreat|Document.actionGoodsItemCorrection)) != 0) {
											final int cc = CPM.GetCurrentDocumentTransferListCount();
											if(ev_subj.ItemIdx < cc) {
												final Document _doc = CPM.GetCurrentDocument();
												final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
												if(ti != null) {
													CommonPrereqModule.TransferItemDialog dialog = new CommonPrereqModule.TransferItemDialog(this, CPM, ti);
													dialog.show();
												}
											}
										}
										else if((actf & Document.actionDocAcceptance) != 0) { // @v11.6.12
											final int cc = CPM.GetCurrentDocumentTransferListCount();
											if(ev_subj.ItemIdx < cc) {
												final Document _doc = CPM.GetCurrentDocument();
												final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
												if(ti != null) {
													AcceptTiDialog dialog = new AcceptTiDialog(this, CPM, ti);
													dialog.show();
												}
											}
										}
										break;
									case R.id.orderPrereqGoodsListView:
										if(ev_subj.ItemIdx < CPM.GetGoodsListSize()) {
											if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.buttonOrder) {
												CommonPrereqModule.WareEntry item = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
												CPM.OpenTransferItemDialog(item, 0.0);
											}
										}
										break;
									case R.id.orderPrereqBrandListView:
										if(SLib.IsInRange(ev_subj.ItemIdx, CPM.BrandListData)) {
											final int brand_id = CPM.BrandListData.get(ev_subj.ItemIdx).ID;
											if(CPM.SetGoodsFilterByBrand(brand_id)) {
												SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.VISIBLE);
												do_update_goods_list_and_toggle_to_it = true;
											}
										}
										break;
									case R.id.orderPrereqGoodsGroupListView:
										if(ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GetGoodsGroupCount()) {
											final int group_id = CPM.GoodsGroupListData.L.get(ev_subj.ItemIdx).ID;
											if(CPM.SetGoodsFilterByGroup(group_id)) {
												SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.VISIBLE);
												do_update_goods_list_and_toggle_to_it = true;
											}
										}
										break;
									case R.id.CTL_INCOMINGLIST_BILL_LIST:
										if(ev_subj.ItemIdx < CPM.IncomingDocListData.size()) {
											Document cur_entry = (Document)CPM.IncomingDocListData.get(ev_subj.ItemIdx);
											if(cur_entry != null && ev_subj.ItemView != null) {
												if(ev_subj.ItemView.getId() == R.id.CTL_DOCUMENT_EXPANDSTATUS) {
													// change expand status
													if(cur_entry.DetailExpandStatus_Ti == 1) {
														cur_entry.DetailExpandStatus_Ti = 2;
														a.notifyItemChanged(ev_subj.ItemIdx);
													}
													else if(cur_entry.DetailExpandStatus_Ti == 2) {
														cur_entry.DetailExpandStatus_Ti = 1;
														a.notifyItemChanged(ev_subj.ItemIdx);
													}
												}
												else {
													boolean done = false;
													CPM.ResetCurrentDocument();
													if(cur_entry.H != null && cur_entry.H.Uuid != null) {
														ArrayList <UUID> possible_doc_uuid_list = new ArrayList<>();
														possible_doc_uuid_list.add(cur_entry.H.Uuid);
														if(CPM.RestoreRecentIncomingModDocumentAsCurrent(possible_doc_uuid_list)) {
															SetupCurrentDocument(true, false);
															done = true;
														}
													}
													if(!done) {
														CPM.SetIncomingDocument(cur_entry);
														SetupCurrentDocument(true, false);
													}
												}
											}
										}
										break;
								}
								if(do_update_goods_list_and_toggle_to_it) {
									GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, -1, -1);
								}
							}
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				{
					Document.EditAction acn = null;
					final int view_id = (srcObj != null && srcObj instanceof View) ? ((View)srcObj).getId() : 0;
					switch(view_id) {
						case R.id.tbButtonBack: finish(); break;
						case R.id.tbButtonSearch:
							CPM.GotoSearchTab(R.id.VIEWPAGER_INCOMINGLISTBILL, 0);
							break;
						case R.id.tbButtonScan:
							{
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabXclSetting);
								//ScanSource = ScanType.Undef;
								if(!CPM.IsCurrentDocumentEmpty()) {
									/*if(te != null && te.TabView != null && SLib.IsChildOf((View) srcObj, te.TabView.getView())) {
										ScanSource = ScanType.Setting;
									}
									else {
										te = SearchTabEntry(CommonPrereqModule.Tab.tabXclVerify);
										if(te != null && te.TabView != null && SLib.IsChildOf((View) srcObj, te.TabView.getView())) {
											ScanSource = ScanType.Veriy;
										}
									}*/
									if(ScanSource != ScanType.Undef) {
										ScanOptions options = new ScanOptions();
										options.setPrompt(""); //
										options.setCameraId(Camera.CameraInfo.CAMERA_FACING_BACK/*CAMERA_FACING_FRONT*/); // Use a specific camera of the device
										options.setOrientationLocked(true);
										options.setBeepEnabled(true);
										options.setCaptureActivity(StyloQZxCaptureActivity.class);
										//options.addExtra("action", scan_action); // Я не понимаю как извлечь эту строку после отработки сканирования :(
											// по-этому, буду ориентироваться на те же признаки, по которым вычислил ее здесь.
										BarcodeLauncher.launch(options);
										/*
										IntentIntegrator integrator = new IntentIntegrator(this); // `this` is the current Activity
										//integrator.setPrompt("Scan a barcode");
										integrator.setCameraId(0);  // Use a specific camera of the device
										integrator.setOrientationLocked(true);
										integrator.setBeepEnabled(true);
										integrator.setCaptureActivity(StyloQZxCaptureActivity.class);
										integrator.addExtra("action", scan_action);
										integrator.initiateScan();
										 */
									}
								}
							}
							break;
						case R.id.CTL_BUTTON_MATCH:
							if(subj != null && subj instanceof SLib.ListViewEvent) {
								SLib.ListViewEvent lve = (SLib.ListViewEvent)subj;
								int code_idx = lve.ItemIdx;
								if(lve.ItemObj != null && lve.ItemObj instanceof Document.GoodsMarkSettingEntry && CPM.GetCurrentDocument() != null) {
									Document.GoodsMarkSettingEntry gmse = (Document.GoodsMarkSettingEntry)lve.ItemObj;
									if(SLib.IsInRange(code_idx, gmse.XcL)) {
										SubstTiForUnassignedMarkDialog.DataBlock _data = new SubstTiForUnassignedMarkDialog.DataBlock();
										_data.Doc = CPM.GetCurrentDocument();
										_data.GmsEntry = gmse;
										_data.CurEntry = (gmse.XcL != null) ? gmse.XcL.get(code_idx) : null;
										Document.GoodsMarkSettingEntry gse = (_data.Doc != null) ? _data.Doc.GetGoodsMarkSettingListItem(code_idx) : null;
										SubstTiForUnassignedMarkDialog dlg = new SubstTiForUnassignedMarkDialog(this, _data);
										dlg.show();
									}
								}
							}
							break;
						case R.id.STDCTL_DELETEBUTTON:
							if(subj != null && subj instanceof SLib.ListViewEvent) {
								SLib.ListViewEvent lve = (SLib.ListViewEvent)subj;
								int code_idx = lve.ItemIdx;
								if(lve.ItemObj != null && lve.ItemObj instanceof Document.GoodsMarkSettingEntry && CPM.GetCurrentDocument() != null) {
									Document.GoodsMarkSettingEntry gmse = (Document.GoodsMarkSettingEntry)lve.ItemObj;
									if(gmse.XcL != null && code_idx >= 0 && code_idx < gmse.XcL.size()) {
										gmse.XcL.remove(code_idx);
										CPM.OnCurrentDocumentModification();
										CommonPrereqModule.Tab tab = CommonPrereqModule.Tab.tabUndef;
										if(ScanSource == ScanType.Setting)
											tab = CommonPrereqModule.Tab.tabXclSetting;
										else if(ScanSource == ScanType.Veriy)
											tab = CommonPrereqModule.Tab.tabXclVerify;
										if(tab != CommonPrereqModule.Tab.tabUndef)
											NotifyTabContentChanged(tab, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
									}
								}
							}
							break;
						case R.id.CTL_DOCUMENT_ACTIONBUTTON1:
							acn = (DocEditActionList != null && DocEditActionList.size() > 0) ? DocEditActionList.get(0) : null;
							break;
						case R.id.CTL_DOCUMENT_ACTIONBUTTON2:
							acn = (DocEditActionList != null && DocEditActionList.size() > 1) ? DocEditActionList.get(1) : null;
							break;
						case R.id.CTL_DOCUMENT_ACTIONBUTTON3:
							acn = (DocEditActionList != null && DocEditActionList.size() > 2) ? DocEditActionList.get(2) : null;
							break;
						case R.id.CTL_DOCUMENT_ACTIONBUTTON4:
							acn = (DocEditActionList != null && DocEditActionList.size() > 3) ? DocEditActionList.get(3) : null;
							break;
					}
					if(acn != null) {
						switch(acn.Action) {
							case Document.editactionClose:
								// Просто закрыть сеанс редактирования документа (изменения и передача сервису не предполагаются)
								CPM.ResetCurrentDocument();
								NotifyCurrentDocumentChanged();
								GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, -1, -1);
								//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
								SetupCurrentDocument(false, true);
								break;
							case Document.editactionSubmit:
								// store document; // Подтвердить изменения документа (передача сервису не предполагается)
								break;
							case Document.editactionSubmitAndTransmit:
							{
								// Подтвердить изменения документа с передачей сервису
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
								if(te != null)
									GetFragmentData(te.TabView);
								ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
								CPM.CommitCurrentDocument();
							}
							break;
							case Document.editactionCancelEdition:
								// Отменить изменения документа (передача сервису не предполагается)
								CPM.ResetCurrentDocument();
								NotifyCurrentDocumentChanged();
								GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, -1, -1);
								//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
								SetupCurrentDocument(false, true);
								break;
							case Document.editactionCancelDocument:
							{
								// Отменить документ с передачей сервису факта отмены
								if(!CPM.IsCurrentDocumentEmpty()) {
									CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
									if(te != null)
										GetFragmentData(te.TabView);
									ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
									//CPM.CancelCurrentDocument();
									if(CPM.CancelCurrentIncomingDocument()) {
										SetupCurrentDocument(false, true);
										GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, -1, -1);
									}
								}
							}
							break;
						}
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				if(srcObj != null) {
					if(srcObj instanceof AcceptTiDialog) {
						if(subj != null && subj instanceof Document.TransferItem) {
							Document.TransferItem _data = (Document.TransferItem)subj;
							boolean do_notify_goods_list = false;
							if(_data.RowIdx == 0) {
								// no way
							}
							else {
								if(CPM.UpdateTransferItemAcceptanceInCurrentDocument(_data)) {
									NotifyCurrentDocumentChanged();
									do_notify_goods_list = true;
								}
							}
						}
					}
					else if(srcObj instanceof CommonPrereqModule.TransferItemDialog) {
						if(subj != null && subj instanceof Document.TransferItem) {
							Document.TransferItem _data = (Document.TransferItem)subj;
							boolean do_notify_goods_list = false;
							if(_data.RowIdx == 0) {
								if(AddItemToCurrentOrder(_data))
									do_notify_goods_list = true;
							}
							else {
								if(CPM.UpdateTransferItemQttyInCurrentDocument(_data)) {
									NotifyCurrentDocumentChanged();
									do_notify_goods_list = true;
								}
							}
							if(do_notify_goods_list) {
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabGoods);
								if(te != null && te.TabView != null) {
									View v = te.TabView.getView();
									if(v != null && v instanceof ViewGroup) {
										View lv = ((ViewGroup) v).findViewById(R.id.orderPrereqGoodsListView);
										if(lv != null && lv instanceof RecyclerView) {
											RecyclerView.Adapter gva = ((RecyclerView) lv).getAdapter();
											if(gva != null)
												gva.notifyDataSetChanged(); // @todo Здесь надо обновлять только одну строку списка товаров
										}
									}
								}
							}
						}
					}
					else if(srcObj instanceof SubstTiForUnassignedMarkDialog && subj != null && subj instanceof SubstTiForUnassignedMarkDialog.DataBlock) {
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
							SubstTiForUnassignedMarkDialog.DataBlock _data = (SubstTiForUnassignedMarkDialog.DataBlock) subj;
							if(_data.CurEntry != null && _data.Doc != null && SLib.IsInRange(_data.SelectedIdx, _data.Doc.TiList)) {
								Document.TransferItem ti = _data.Doc.TiList.get(_data.SelectedIdx);
								if(ti.XcL == null)
									ti.XcL = new ArrayList<Document.LotExtCode>();
								ti.XcL.add(_data.CurEntry);
								if(_data.GmsEntry.XcL != null) {
									//for(Document.LotExtCode iter : _data.GmsEntry.XcL) {
									for(int i = 0; i < _data.GmsEntry.XcL.size(); i++) {
										Document.LotExtCode iter = _data.GmsEntry.XcL.get(i);
										if(iter != null && (iter == _data.CurEntry || iter.Code.equals(_data.CurEntry.Code))) {
											_data.GmsEntry.XcL.remove(i);
											break;
										}
									}
								}
								CPM.OnCurrentDocumentModification();
								NotifyTabContentChanged(CommonPrereqModule.Tab.tabXclSetting, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
							}
						}
					}
				}
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
					StyloQApp app_ctx = GetAppCtx();
					final String org_cmd_name = (ir.OriginalCmdItem != null) ? ir.OriginalCmdItem.Name : null;
					final UUID org_cmd_uuid = (ir.OriginalCmdItem != null) ? ir.OriginalCmdItem.Uuid : null;
					if(SLib.AreUUIDsEqual(CPM.CmdUuid, org_cmd_uuid)) { // @v11.6.2
						// Обработка обновления данных по внутреннему запросу
						if(app_ctx != null && ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS && ir.InfoReply != null) {
							String svc_reply_doc_json = null;
							if(ir.InfoReply instanceof StyloQCommand.DocReference) {
								long doc_id = ((StyloQCommand.DocReference)ir.InfoReply).ID;
								JSONObject js_doc_decl = null;
								try {
									StyloQDatabase db = app_ctx.GetDB();
									ArrayList<UUID> possible_doc_uuid_list = null;
									if(doc_id > 0) {
										StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
										if(doc_packet != null) {
											byte[] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
											if(SLib.GetLen(raw_doc) > 0) {
												svc_reply_doc_json = new String(raw_doc);
												js_doc_decl = doc_packet.Pool.GetJsonObject(SecretTagPool.tagDocDeclaration); // @v11.6.6
											}
										}
									}
									if(ProcessSvcData(svc_reply_doc_json, js_doc_decl) > 0) {
										SetupTabVisibility(true);
										SetupCurrentDocument(false, true);
										LastRefreshTime = System.currentTimeMillis(); // @v11.6.4
									}
								} catch(StyloQException exn) {
									;
								}
							}
							else if(ir.InfoReply instanceof SecretTagPool) {
								JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
								if(js_reply != null) {
									JSONObject js_doc_decl = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagDocDeclaration); // @v11.6.6
									if(ProcessSvcData(js_reply, js_doc_decl) > 0) {
										SetupTabVisibility(true);
										SetupCurrentDocument(false, true);
										LastRefreshTime = System.currentTimeMillis(); // @v11.6.4
									}
								}
							}
						}
					}
					else if(SLib.AreStringsEqualNoCase(org_cmd_name, "PostDocument")) {
						CPM.CurrentDocument_RemoteOp_Finish();
						ScheduleRTmr(null, 0, 0);
						if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
							CPM.ResetCurrentDocument();
							// @v11.5.1 {
							int     new_doclist_view_idx = -1;
							{
								if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
									JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
									if(js_reply != null) {
										JSONObject js_new_doc = js_reply.optJSONObject("doc");
										if(js_new_doc != null) {
											Document new_doc = new Document();
											if(new_doc.FromJsonObj(js_new_doc) && new_doc.H != null && new_doc.H.Uuid != null) {
												if(CPM.IncomingDocListData != null) {
													for(int i = 0; i < CPM.IncomingDocListData.size(); i++) {
														Document iter = CPM.IncomingDocListData.get(i);
														if(iter != null && iter.H != null && SLib.AreUUIDsEqual(iter.H.Uuid, new_doc.H.Uuid)) {
															CPM.IncomingDocListData.set(i, new_doc);
															new_doclist_view_idx = i;
														}
													}
												}
											}
										}
									}
								}
							}
							// } @v11.5.1
							NotifyCurrentDocumentChanged();
							// @v11.5.1 {
							if(new_doclist_view_idx >= 0) {
								NotifyTabContentChanged(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST);
							}
							// } @v11.5.1
							GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, -1, -1);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclSetting, View.GONE);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.GONE);
							// @v11.6.4 {
							// Немедленно отправляем запрос на обновление данных
							// Что бы не было "перехлеста" с регулярным обновлением мы предусмотрели в нем проверку на время последнего получения
							// изменений (LastRefreshTime)
							{
								Thread thr = new Thread(new ThreadEngine_RefreshDataSvcPoll(app_ctx, CPM.SvcIdent, CPM.CmdUuid, this));
								thr.start();
							}
							// } @v11.6.4
						}
						else {
							String err_msg = app_ctx.GetString(ppstr2.PPSTR_ERROR, ppstr2.PPERR_STQ_POSTDOCUMENTFAULT);
							String reply_err_msg = null;
							if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
								JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
								if(js_reply != null) {
									StyloQInterchange.CommonReplyResult crr = StyloQInterchange.GetReplyResult(js_reply);
									reply_err_msg = crr.ErrMsg;
								}
							}
							if(SLib.GetLen(reply_err_msg) > 0)
								err_msg += ": " + reply_err_msg;
							app_ctx.DisplayError(this, err_msg, 0);
						}
					}
					else if(SLib.AreStringsEqualNoCase(org_cmd_name, "CancelDocument")) {
						CPM.CurrentDocument_RemoteOp_Finish();
						ScheduleRTmr(null, 0, 0);
						if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
							CPM.ResetCurrentDocument();
							NotifyCurrentDocumentChanged();
							GotoTab(CommonPrereqModule.Tab.tabIncomingList, R.id.CTL_INCOMINGLIST_BILL_LIST, -1, -1);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclSetting, View.GONE);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabXclVerify, View.GONE);
						}
						else {
							; // @todo
						}
					}
				}
				break;
			case SLib.EV_QUERY:
				if(subj != null && subj instanceof String) {
					String q = (String)subj;
					if(q.equalsIgnoreCase("GetSvcDataTime")) {
						SLib.LDATETIME svc_dtm = CPM.GetSvcDataDtm();
						result = svc_dtm;
					}
				}
				break;
		}
		return result;
	}
	private final ActivityResultLauncher<ScanOptions> BarcodeLauncher = registerForActivityResult(new ScanContract(), activity_result ->
	{
		String contents = activity_result.getContents();
		Intent original_intent = activity_result.getOriginalIntent();
		if(SLib.GetLen(contents) > 0) {
			byte [] bytes_contents = contents.getBytes();
			Intent _intent = this.getIntent();
			boolean is_processed = false;
			StyloQApp app_ctx = (StyloQApp)getApplicationContext();
			if(app_ctx != null) {
				//String _action = original_intent.getStringExtra("action");
				Document _doc = CPM.GetCurrentDocument();
				if(_doc != null) {
					GTIN gtin_chzn = GTIN.ParseChZnCode(contents, 0);
					if(gtin_chzn != null && gtin_chzn.GetChZnParseResult() > 0) {
						String mark = GTIN.PreprocessChZnCode(contents);
						if(ScanSource == ScanType.Veriy) {
							if(_doc.VXcL == null)
								_doc.VXcL = new ArrayList<Document.LotExtCode>();
							Document.LotExtCode lec = new Document.LotExtCode();
							lec.Flags = 0;
							lec.BoxRefN = 0;
							lec.Code = mark;
							_doc.VXcL.add(lec);
							CPM.OnCurrentDocumentModification();
							NotifyTabContentChanged(CommonPrereqModule.Tab.tabXclVerify, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
							is_processed = true;
						}
						else if(ScanSource == ScanType.Setting) {
							try {
								int amr = _doc.AssignGoodsMark(mark, CPM.GoodsListData);
								if(amr > 0) {
									CPM.OnCurrentDocumentModification();
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabXclSetting, R.id.CTL_INCOMINGLIST_BILL_SCANMARKS_LIST);
								}
							} catch(StyloQException exn) {
								app_ctx.DisplayError(this, exn, 0);
							}
						}
					}
					else {
						app_ctx.DisplayError(this, app_ctx.GetErrorText(ppstr2.PPERR_TEXTISNTCHZNMARK, contents), 0);
					}
				}
				//Log.d("MainActivity", "Scanned");
				//Toast.makeText(MainActivity.this, "Scanned: " + result.getContents(), Toast.LENGTH_LONG).show();
			}
			//ScanSource = ScanType.Undef; // ? Не уверен что при continuous сканировании это будет правильно
		}
		else {
			if(original_intent == null) {
				//Log.d("MainActivity", "Cancelled scan");
				//Toast.makeText(MainActivity.this, "Cancelled", Toast.LENGTH_LONG).show();
			}
			else if(original_intent.hasExtra(Intents.Scan.MISSING_CAMERA_PERMISSION)) {
				//Log.d("MainActivity", "Cancelled scan due to missing camera permission");
				//Toast.makeText(MainActivity.this, "Cancelled due to missing camera permission", Toast.LENGTH_LONG).show();
			}
		}
	});
}