// CmdROrderPrereqActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.hardware.Camera;
import android.location.Location;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.IdRes;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.zxing.client.android.Intents;
import com.journeyapps.barcodescanner.ScanContract;
import com.journeyapps.barcodescanner.ScanOptions;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.TimerTask;

public class CmdROrderPrereqActivity extends SLib.SlActivity {
	public  CommonPrereqModule CPM;
	private JSONArray WharehouseListData;
	private JSONArray QuotKindListData;
	private ViewDescriptionList VdlDocs; // Описание таблицы просмотра существующих заказов
	private ArrayList <Document.EditAction> DocEditActionList;
	private BusinessEntity.DebtList DbtL; // @v11.5.4
	private StyloQCommand.Item CmdQueryDebt; // @v11.5.4 Команда сервиса, используемая для запроса долгового реестра
	private boolean DlvrLocGpslocSettingAllowed; // @v11.6.1 0
	private void RefreshCurrentDocStatus()
	{
		if(CPM.TabList != null) {
			ViewPager2 view_pager = (ViewPager2) findViewById(R.id.VIEWPAGER_ORDERPREREQ);
			if(view_pager != null) {
				int tidx = view_pager.getCurrentItem();
				CommonPrereqModule.TabEntry tab_entry = CPM.TabList.get(tidx);
				if(tab_entry != null && tab_entry.TabId == CommonPrereqModule.Tab.tabCurrentDocument) {
					HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
				}
			}
		}
	}
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshCurrentDocStatus(); }}); }
	}
	public CmdROrderPrereqActivity()
	{
		CPM = new CommonPrereqModule(this);
		DocEditActionList = null;
		DbtL = null;
		CmdQueryDebt = null;
		DlvrLocGpslocSettingAllowed = false;
	}
	private void MakeSimpleSearchIndex()
	{
		CPM.InitSimpleIndex();
		CPM.AddGoodsToSimpleIndex();
		CPM.AddGoodsGroupsToSimpleIndex();
		CPM.AddBrandsToSimpleIndex();
		if(CPM.CliListData != null) {
			for(int i = 0; i < CPM.CliListData.size(); i++) {
				CommonPrereqModule.CliEntry ce = CPM.CliListData.get(i);
				if(ce != null && ce.JsItem != null) {
					int id = ce.JsItem.optInt("id", 0);
					if(id > 0) {
						String nm = ce.JsItem.optString("nm", null);
						CPM.AddSimpleIndexEntry(SLib.PPOBJ_PERSON, id, SLib.PPOBJATTR_NAME, nm, nm);
						String ruinn = ce.JsItem.optString("ruinn");
						CPM.AddSimpleIndexEntry(SLib.PPOBJ_PERSON, id, SLib.PPOBJATTR_RUINN, ruinn, nm);
						String rukpp = ce.JsItem.optString("rukpp");
						CPM.AddSimpleIndexEntry(SLib.PPOBJ_PERSON, id, SLib.PPOBJATTR_RUKPP, rukpp, nm);
						JSONArray dlvr_loc_list = ce.JsItem.optJSONArray("dlvrloc_list");
						if(dlvr_loc_list != null) {
							for(int j = 0; j < dlvr_loc_list.length(); j++) {
								JSONObject js_item = dlvr_loc_list.optJSONObject(j);
								if(js_item != null) {
									final int loc_id = js_item.optInt("id", 0);
									if(loc_id > 0) {
										String addr = js_item.optString("addr");
										CPM.AddSimpleIndexEntry(SLib.PPOBJ_LOCATION, loc_id, SLib.PPOBJATTR_RAWADDR, addr, nm);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	static class DebtDetailDialog extends SLib.SlDialog {
		CmdROrderPrereqActivity ActivityCtx;
		public DebtDetailDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_DEBTDETAILLIST, data);
			if(ctx instanceof CmdROrderPrereqActivity)
				ActivityCtx = (CmdROrderPrereqActivity)ctx;
			if(data instanceof BusinessEntity.ArDebtList)
				Data = data;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_debtdetaillist);
					SetDTS(Data);
					break;
				case SLib.EV_COMMAND:
					if(srcObj != null && srcObj instanceof View) {
						final int view_id = ((View) srcObj).getId();
						if(view_id == R.id.STDCTL_CLOSEBUTTON) {
							this.dismiss();
						}
					}
					break;
				case SLib.EV_LISTVIEWCOUNT:
					{
						SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
						if(a.GetListRcId() == R.id.CTL_DEBTDETAILLIST_LIST) {
							BusinessEntity.ArDebtList _data = (Data != null && Data instanceof BusinessEntity.ArDebtList) ? (BusinessEntity.ArDebtList)Data : null;
							result = new Integer((_data != null && _data.List != null) ? _data.List.size() : 0);
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
									if(a.GetListRcId() == R.id.CTL_DEBTDETAILLIST_LIST) {
										BusinessEntity.ArDebtList _data = (Data != null && Data instanceof BusinessEntity.ArDebtList) ? (BusinessEntity.ArDebtList)Data : null;
										if(_data != null && _data.List != null && ev_subj.ItemIdx < _data.List.size()) {
											//CPM.FindGoodsItemByGoodsID()
											BusinessEntity.DebtEntry cur_entry = _data.List.get(ev_subj.ItemIdx);
											View iv = ev_subj.RvHolder.itemView;
											SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_CODE, cur_entry.BillCode);
											SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_DATE, cur_entry.BillDate.Format(SLib.DATF_DMY));
											SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_AMOUNT, SLib.formatdouble(cur_entry.Amount, 2));
											SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_DEBT, SLib.formatdouble(cur_entry.Debt, 2));
										}
									}
								}
							}
						}
					}
					break;
			}
			return result;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			StyloQApp app_ctx = (ActivityCtx != null) ? ActivityCtx.GetAppCtx() : null;
			if(app_ctx != null) {
				//Context ctx = getContext();
				BusinessEntity.ArDebtList _data = (Data != null && Data instanceof BusinessEntity.ArDebtList) ? (BusinessEntity.ArDebtList)Data : null;
				if(_data != null) {
					String hint_text = _data.ArName; // @todo
					SLib.SetCtrlString(this, R.id.CTL_DEBTDETAILLIST_HEADER, hint_text);
					View lv = findViewById(R.id.CTL_DEBTDETAILLIST_LIST);
					if(lv != null && lv instanceof RecyclerView) {
						((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(ActivityCtx));
						SLib.RecyclerListAdapter adapter = new SLib.RecyclerListAdapter(ActivityCtx, this, R.id.CTL_DEBTDETAILLIST_LIST, R.layout.li_debtdetaillist);
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
	int FindClientItemIndexByID(int id)
	{
		int result = -1;
		if(CPM.CliListData != null && id > 0) {
			for(int i = 0; result < 0 && i < CPM.CliListData.size(); i++) {
				final int iter_id = CPM.CliListData.get(i).JsItem.optInt("id", 0);
				if(iter_id == id)
					result = i;
			}
		}
		return result;
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_ORDERPREREQ;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && (CPM.TabList == null || force)) {
			CPM.TabList = new ArrayList<CommonPrereqModule.TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			CommonPrereqModule.TabInitEntry[] tab_init_list = {
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoodsGroups, R.layout.layout_orderprereq_goodsgroups, "@{group_pl}", (CPM.GoodsGroupListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabBrands, R.layout.layout_orderprereq_brands, "@{brand_pl}", (CPM.BrandListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoods, R.layout.layout_orderprereq_goods, "@{ware_pl}", (CPM.GoodsListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabClients, R.layout.layout_orderprereq_clients, "@{client_pl}", (CPM.CliListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabCurrentDocument, R.layout.layout_orderprereq_ordr, "@{orderdocument}", true),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabRegistry, R.layout.layout_orderprereq_registry, "@{booking_pl}", true),
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
	private CommonPrereqModule.TabEntry SearchTabEntry(CommonPrereqModule.Tab tab)
	{
		return CPM.SearchTabEntry(R.id.VIEWPAGER_ORDERPREREQ, tab);
	}
	//
	// Descr: Активирует вкладку с идентификатором tab
	// Note: Класс CommonPrepreqModule нуждается в доступе к этому методу, потому он - public
	//
	public void GotoTab(CommonPrereqModule.Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		CPM.Implement_GotoTab(tab, R.id.VIEWPAGER_ORDERPREREQ, recyclerViewToUpdate, goToIndex, nestedIndex, -1);
	}
	private void NotifyTabContentChanged(CommonPrereqModule.Tab tabId, int innerViewId)
	{
		CPM.NotifyTabContentChanged(R.id.VIEWPAGER_ORDERPREREQ, tabId, innerViewId);
	}
	private void NotifyDocListChanged() { NotifyTabContentChanged(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView); }
	private void NotifyCurrentDocumentChanged()
	{
		NotifyTabContentChanged(CommonPrereqModule.Tab.tabCurrentDocument, R.id.orderPrereqOrdrListView);
		NotifyTabContentChanged(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView);
		CommonPrereqModule.TabEntry tab_entry = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
		if(tab_entry != null && tab_entry.TabView != null) {
			CPM.OnCurrentDocumentModification(); // @v11.4.8
			HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
		}
	}
	private boolean SetCurrentOrderClient(JSONObject cliItem, JSONObject dlvrLocItem)
	{
		boolean result = false;
		//try {
		JSONObject final_cli_js = null;
		if(CPM.CliListData != null) {
			if(dlvrLocItem != null) {
				final_cli_js = CPM.FindClientEntryByDlvrLocID(dlvrLocItem.optInt("id", 0));
			}
			else if(cliItem != null) {
				final_cli_js = cliItem;
				int cli_id = cliItem.optInt("id", 0);
				if(cli_id > 0) {

				}
			}
			if(final_cli_js != null) {
				boolean do_init_geloc = (CPM.GetCurrentDocument() == null);
				int cli_id = final_cli_js.optInt("id", 0);
				int dlvrloc_id = (dlvrLocItem != null) ? dlvrLocItem.optInt("id", 0) : 0;
				result = CPM.SetClientToCurrentDocument(SLib.PPEDIOP_ORDER, cli_id, dlvrloc_id, false);
				if(result) {
					if(do_init_geloc) {
						// Отправляем запрос на получение гео-координат с objid типа документ и нулевым идентификатором.
						// HandleEvent по этой паре определит что речь идет о вновь созданном документе.
						SLib.QueryCurrentGeoLoc(this, new SLib.PPObjID(SLib.PPOBJ_BILL, 0), this);
					}
					CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.VISIBLE);
					NotifyCurrentDocumentChanged();
					NotifyTabContentChanged(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView);
				}
			}
		}
		//} catch(StyloQException exn) { result = false; }
		return result;
	}
	//
	private boolean AddItemToCurrentOrder(Document.TransferItem item)
	{
		boolean do_init_geloc = (CPM.GetCurrentDocument() == null);
		boolean result = CPM.AddTransferItemToCurrentDocument(item);
		if(result) {
			if(do_init_geloc) {
				// Отправляем запрос на получение гео-координат с objid типа документ и нулевым идентификатором.
				// HandleEvent по этой паре определит что речь идет о вновь созданном документе.
				SLib.QueryCurrentGeoLoc(this, new SLib.PPObjID(SLib.PPOBJ_BILL, 0), this);
			}
			NotifyCurrentDocumentChanged();
		}
		return result;
	}
	private static class DlvrLocListAdapter extends SLib.InternalArrayAdapter implements SLib.EventHandler {
		//private int RcId;
		private boolean DlvrLocGpslocSettingAllowed;
		DlvrLocListAdapter(Context ctx, int rcId, ArrayList data, boolean isDlvrLocGpslocSettingAllowed)
		{
			super(ctx, rcId, data);
			DlvrLocGpslocSettingAllowed = isDlvrLocGpslocSettingAllowed;
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
					TextView v = convertView.findViewById(R.id.LVITEM_GENERICNAME);
					if(v != null) {
						int loc_id = 0;
						double geoloc_lat = 0.0;
						double geoloc_lon = 0.0;
						if(item instanceof JSONObject) {
							JSONObject js_item = (JSONObject)item;
							loc_id = js_item.optInt("id", 0);
							geoloc_lat = js_item.optDouble("lat", 0.0);
							geoloc_lon = js_item.optDouble("lon", 0.0);
							v.setText(js_item.optString("addr", ""));
						}
						if(_ctx instanceof CmdROrderPrereqActivity)
							((CmdROrderPrereqActivity)_ctx).SetListBackground(convertView, this, position, SLib.PPOBJ_LOCATION, loc_id);
						{
							View grpv = convertView.findViewById(R.id.CTLGRP_LOCATION_GEOLOC);
							View bv = convertView.findViewById(R.id.CTL_BUTTON_GEOLOCMARK);
							if(bv != null && bv instanceof ImageButton) {
								/*if(DlvrLocGpslocSettingAllowed) {
									if(grpv != null)
										grpv.setVisibility(View.VISIBLE);
									else
										bv.setVisibility(View.VISIBLE);
									{
										if(geoloc_lat != 0.0 && geoloc_lon != 0.0) {
											String geo_loc_text = String.format("%.6f, %.6f", geoloc_lat, geoloc_lon);
											SLib.SetCtrlString(convertView, R.id.CTL_LOCATION_GEOLOC, geo_loc_text);
										}
									}
									bv.setOnClickListener(new View.OnClickListener() {
										@Override public void onClick(View v)
										{
											Context ctx = getContext();
											if(ctx != null && ctx instanceof CmdROrderPrereqActivity) {
												HandleEvent(SLib.EV_COMMAND, v, new Integer(position));
											}
										}});
								}
								else*/{
									if(grpv != null)
										grpv.setVisibility(View.GONE);
									else
										bv.setVisibility(View.GONE);
								}
							}
						}
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			switch(ev) {
				case SLib.EV_COMMAND:
					if(srcObj != null && srcObj instanceof View) {
						final int view_id = ((View)srcObj).getId();
						if(view_id == R.id.CTL_BUTTON_GEOLOCMARK) {
							Context ctx = getContext();
							if(ctx != null && ctx instanceof CmdROrderPrereqActivity) {
								StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
								if(DlvrLocGpslocSettingAllowed) {
									int position = (subj != null && subj instanceof Integer) ? (Integer)subj : 0;
									if(position >= 0 && position < getCount()) {
										int loc_id = 0;
										Object item = getItem(position);
										if(item != null && item instanceof JSONObject) {
											JSONObject js_item = (JSONObject) item;
											loc_id = js_item.optInt("id", 0);
											if(loc_id > 0) {
												SLib.QueryCurrentGeoLoc((CmdROrderPrereqActivity) ctx, new SLib.PPObjID(SLib.PPOBJ_LOCATION, loc_id), this);
											}
										}
									}
								}
							}
						}
					}
					break;
				case SLib.EV_GEOLOCDETECTED:
					if(subj != null && subj instanceof Location) {
						Context ctx = getContext();
						StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
						SLib.PPObjID oid = (srcObj != null && srcObj instanceof SLib.PPObjID) ? (SLib.PPObjID)srcObj : null;
						if(app_ctx != null && oid != null && oid.Type == SLib.PPOBJ_LOCATION && oid.Id > 0) {
							//StyloQDatabase.SecStoragePacket _data = (Data != null && Data instanceof StyloQDatabase.SecStoragePacket) ? (StyloQDatabase.SecStoragePacket)Data : null;
							if(ctx instanceof CmdROrderPrereqActivity) {
								app_ctx.RunSvcCommand_SetGeoLoc(((CmdROrderPrereqActivity)ctx).CPM.SvcIdent, oid.Id, (Location) subj, this);
							}
						}
					}
					break;
				case SLib.EV_SVCQUERYRESULT:
					if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
						StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
						Context ctx = getContext();
						StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
						if(app_ctx != null) {
							if(ir.OriginalCmdItem != null && SLib.GetLen(ir.OriginalCmdItem.Name) > 0 && ir.OriginalCmdItem.Name.equalsIgnoreCase("setgeoloc")) {
								if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
									if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
										SecretTagPool svc_reply_pool = (SecretTagPool) ir.InfoReply;
										JSONObject sv_reply_js = svc_reply_pool.GetJsonObject(SecretTagPool.tagRawData);
										String reply_result = sv_reply_js.optString("result");
										if(reply_result != null && reply_result.equalsIgnoreCase("ok")) {
											JSONObject reply_loc_js = sv_reply_js.optJSONObject("dlvrloc");
											if(reply_loc_js != null) {

											}
										}
									}
								}
							}
						}
					}
					break;
			}
			return null;
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
				int vg_id = vg.getId();
				if(vg_id == R.id.LAYOUT_ORDERPREPREQ_ORDR) {
					boolean umr = CPM.UpdateMemoInCurrentDocument(SLib.GetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO));
					if(umr)
						CPM.OnCurrentDocumentModification();
				}
			}
		}
	}
	private void SetupCurrentDocument(boolean gotoTabIfNotEmpty, boolean removeTabIfEmpty)
	{
		//
		// При попытке скрыть и потом показать табы они перерисовываются с искажениями.
		// по этому не будем злоупотреблять такими фокусами.
		//
		if(!CPM.IsCurrentDocumentEmpty()) {
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.VISIBLE);
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
				GotoTab(CommonPrereqModule.Tab.tabCurrentDocument, R.id.orderPrereqOrdrListView, -1, -1);
		}
		else {
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.VISIBLE);
			if(removeTabIfEmpty)
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
		}
	}
	private void PreprocessRegistryData()
	{
		final int rhlist_count = SLib.GetCount(CPM.RegistryHList);
		final int _vdlc = VdlDocs.GetCount();
		assert (_vdlc > 0);
		for(int i = 0; i < _vdlc; i++) {
			ViewDescriptionList.DataPreprocessBlock dpb = VdlDocs.StartDataPreprocessing(this, i);
			if(dpb != null && dpb.ColumnDescription != null) {
				for(int j = 0; j < /*CPM.RegistryHList.size()*/rhlist_count; j++) {
					assert(CPM.RegistryHList != null);
					Document.DisplayEntry cur_entry = CPM.RegistryHList.get(j);
					if(cur_entry != null && cur_entry.H != null) {
						String text = null;
						if(dpb.ColumnDescription.Id == 1) { // status image
							; // По-моему, здесь ничего замерять не надо - мы и так зафиксировали размер элемента
						}
						else if(dpb.ColumnDescription.Id == 2) { // date
							SLib.LDATE d = cur_entry.GetNominalDate(CPM.GetOption_DueDateAsNominal());
							if(d != null)
								VdlDocs.DataPreprocessingIter(dpb, d.Format(SLib.DATF_DMY));
						}
						else if(dpb.ColumnDescription.Id == 3) { // code
							VdlDocs.DataPreprocessingIter(dpb, cur_entry.H.Code);
						}
						else if(dpb.ColumnDescription.Id == 4) { // amount
							text = CPM.FormatCurrency(cur_entry.H.Amount);
							VdlDocs.DataPreprocessingIter(dpb, new Double(cur_entry.H.Amount), text);
						}
						else if(dpb.ColumnDescription.Id == 5) { // client
							if(cur_entry.H.ClientID > 0) {
								JSONObject cli_entry = CPM.FindClientEntry(cur_entry.H.ClientID);
								if(cli_entry != null)
									text = cli_entry.optString("nm", "");
							}
							VdlDocs.DataPreprocessingIter(dpb, text);
						}
						else if(dpb.ColumnDescription.Id == 6) { // memo
							VdlDocs.DataPreprocessingIter(dpb, null, cur_entry.H.Memo);
						}
					}
				}
				VdlDocs.FinishDataProcessing(dpb);
				dpb = null;
			}
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
						CmdQueryDebt = null;
						DbtL = null;
						CPM.GetAttributesFromIntent(intent);
						long doc_id = intent.getLongExtra("SvcReplyDocID", 0);
						String svc_reply_doc_json = null;
						StyloQApp app_ctx = GetAppCtx();
						StyloQDatabase db = (app_ctx != null) ? app_ctx.GetDB() : null;
						if(db != null) {
							CPM.SetupCurrentState(db); // @v11.7.0
							if(doc_id > 0) {
								StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
								if(doc_packet != null) {
									byte[] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_doc) > 0)
										svc_reply_doc_json = new String(raw_doc);
								}
							}
							else
								svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
							if(SLib.GetLen(svc_reply_doc_json) > 0) {
								JSONObject js_head = new JSONObject(svc_reply_doc_json);
								CPM.GetCommonJsonFactors(js_head, null);
								CPM.MakeUomListFromCommonJson(js_head);
								CPM.MakeGoodsGroupListFromCommonJson(js_head);
								CPM.MakeGoodsListFromCommonJson(js_head);
								CPM.MakeBrandListFromCommonJson(js_head);
								CPM.MakeQuotKindListFromCommonJson(js_head); // @v11.6.8
								WharehouseListData = js_head.optJSONArray("warehouse_list");
								QuotKindListData = js_head.optJSONArray("quotkind_list");
								CPM.MakeClientListFromCommonJson(js_head);
								if(CPM.GetOption_UseCliDebt()) {
									StyloQDatabase.SecStoragePacket cmdl_pack = db.GetForeignSvcCommandList(CPM.SvcIdent);
									StyloQCommand.List cmd_list = cmdl_pack.GetCommandList();
									CmdQueryDebt = cmd_list.GetItemWithParticularBaseId(StyloQCommand.sqbcDebtList);
									if(CmdQueryDebt != null)
										DbtL = app_ctx.LoadDebtList(CPM.SvcIdent);
								}
								CPM.RestoreRecentDraftDocumentAsCurrent(); // @v11.4.0
								CPM.MakeCurrentDocList();
								MakeSimpleSearchIndex();
							}
							{
								DlvrLocGpslocSettingAllowed = false;
								StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, CPM.SvcIdent);
								if(svc_pack != null) {
									byte[] cfg_bytes = svc_pack.Pool.Get(SecretTagPool.tagConfig);
									if(SLib.GetLen(cfg_bytes) > 0) {
										StyloQConfig svc_cfg = new StyloQConfig();
										if(svc_cfg.FromJson(new String(cfg_bytes))) {
											final int cli_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagCliFlags));
											if((cli_flags & StyloQConfig.clifPsnAdrGPS) != 0) {
												if(ContextCompat.checkSelfPermission(app_ctx, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
													DlvrLocGpslocSettingAllowed = true;
												}
											}
										}
									}
								}
							}
							requestWindowFeature(Window.FEATURE_NO_TITLE);
							setContentView(R.layout.activity_cmdrorderprereq);
							CPM.SetupActivity(db, R.id.VIEWPAGER_ORDERPREREQ, R.id.TABLAYOUT_ORDERPREREQ);
							ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ORDERPREREQ);
							SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_ORDERPREREQ);
							{
								TabLayout lo_tab = findViewById(R.id.TABLAYOUT_ORDERPREREQ);
								if(lo_tab != null) {
									CreateTabList(false);
									for(int i = 0; i < CPM.TabList.size(); i++) {
										CommonPrereqModule.TabEntry te = CPM.TabList.get(i);
										if(te != null) {
											TabLayout.Tab tab = lo_tab.newTab();
											int icon_rc_id = 0;
											if(te.TabId == CommonPrereqModule.Tab.tabGoods)
												icon_rc_id = R.drawable.ic_obj_goods_kanji_054c1;
											if(te.TabId == CommonPrereqModule.Tab.tabGoodsGroups)
												icon_rc_id = R.drawable.ic_obj_goodsgroup;
											else if(te.TabId == CommonPrereqModule.Tab.tabBrands)
												icon_rc_id = R.drawable.ic_obj_brand01;
											else if(te.TabId == CommonPrereqModule.Tab.tabClients)
												icon_rc_id = R.drawable.ic_client01;
											else if(te.TabId == CommonPrereqModule.Tab.tabCurrentDocument)
												icon_rc_id = R.drawable.ic_shoppingcart01;
											else if(te.TabId == CommonPrereqModule.Tab.tabSearch)
												icon_rc_id = R.drawable.ic_search;
											if(icon_rc_id != 0) {
												//Drawable draw = getResources().getDrawable(icon_rc_id, getTheme());
												Drawable draw = AppCompatResources.getDrawable(this, icon_rc_id);
												if(draw != null) {
													//draw = new ScaleDrawable(draw, 0, 24, 24);
													//draw.setBounds(0, 0, 10, 10);
													tab.setIcon(draw);
												}
												//tab.setIcon(icon_rc_id);
											}
											tab.setText(te.TabText);
											lo_tab.addTab(tab);
										}
									}
									SLib.SetupTabLayoutStyle(lo_tab);
									SLib.SetupTabLayoutListener(this, lo_tab, view_pager);
									if(CPM.IsCurrentDocumentEmpty())
										CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
									if(SLib.GetCount(CPM.RegistryHList) <= 0)
										CPM.SetTabVisibility(CommonPrereqModule.Tab.tabRegistry, View.GONE);
									//SetTabVisibility(Tab.tabSearch, View.GONE);
								}
							}
							SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
							{
								CPM.Callback_BackButton = new OnBackPressedCallback(true /* enabled by default */) {
									@Override public void handleOnBackPressed() { CPM.BackTab(R.id.VIEWPAGER_ORDERPREREQ); }
								};
								getOnBackPressedDispatcher().addCallback(this, CPM.Callback_BackButton);
							}
						}
					} catch(JSONException exn) {
						//exn.printStackTrace();
					} catch(StyloQException exn) {
						//exn.printStackTrace();
					}
				}
				break;
			case SLib.EV_TABSELECTED:
				{
					final CommonPrereqModule.Tab tab_id = CPM.OnTabSelection(subj);
					SLib.SetCtrlVisibility(this, R.id.tbButtonLocalTabConfig, (tab_id == CommonPrereqModule.Tab.tabRegistry) ? View.VISIBLE : View.GONE);
					SLib.SetCtrlVisibility(this, R.id.tbButtonHelp, (tab_id == CommonPrereqModule.Tab.tabRegistry) ? View.VISIBLE : View.GONE);
					// @v11.6.3 @seva {
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
					// } @v11.6.3 @seva
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
						case R.id.orderPrereqGoodsListView: result = new Integer(CPM.GetGoodsListSize()); break;
						case R.id.orderPrereqGoodsGroupListView: result = new Integer(CPM.GetGoodsGroupCount()); break;
						case R.id.orderPrereqBrandListView: result = new Integer(SLib.GetCount(CPM.BrandListData)); break;
						case R.id.orderPrereqOrdrListView: result = new Integer(CPM.GetCurrentDocumentTransferListCount()); break;
						case R.id.orderPrereqRegistryListView:
							final int _c = SLib.GetCount(CPM.RegistryHList);
							result = new Integer((_c > 0) ? _c : 1);
							break;
						case R.id.orderPrereqClientsListView: result = new Integer(SLib.GetCount(CPM.CliListData)); break;
						case R.id.searchPaneListView: result = new Integer(CPM.SearchResult_GetObjTypeCount()); break;
					}
				}
				break;
			case SLib.EV_GETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup)
					GetFragmentData(srcObj);
				break;
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					StyloQApp app_ctx = GetAppCtx();
					ViewGroup vg = (ViewGroup)srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_SEARCHPANE) {

					}
					else if(vg_id == R.id.LAYOUT_ORDERPREPREQ_ORDR) {
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
							// @v11.4.8 {
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
							// } @v11.4.8
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
							status_image_rc_id = Document.GetImageResourceByDocStatus(_doc.GetDocStatus());
						}
						{
							//
							View back_cli_img_view = findViewById(R.id.CTL_DOCUMENT_BACK_CLI);
							// Агентские заказы - требуется указание клиента
							if(CPM.GetAgentID() > 0 && CPM.GetCurrentDocument() != null && Document.DoesStatusAllowModifications(CPM.GetCurrentDocument().GetDocStatus())) {
								if(back_cli_img_view != null) {
									back_cli_img_view.setVisibility(ViewGroup.VISIBLE);
									back_cli_img_view.setOnClickListener(new View.OnClickListener() {
										@Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }
									});
								}
							}
							else
								SLib.SetCtrlVisibility(back_cli_img_view, View.GONE);
						}
						{
							View v = findViewById(R.id.CTL_DOCUMENT_STATUSICON);
							if(v != null && v instanceof ImageView)
								((ImageView)v).setImageResource(status_image_rc_id);
						}
						CPM.DrawCurrentDocumentRemoteOpIndicators();
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemIdx >= 0) {
						if(ev_subj.RvHolder != null) { // RecyclerView
							if(SLib.IsRecyclerTreeListAdapter(srcObj)) {
								AmrTreeView.Adapter a = (AmrTreeView.Adapter)srcObj;
								switch(a.GetListRcId()) {
									case R.id.orderPrereqGoodsGroupListView:
										if(ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GetGoodsGroupCount()) {
											View iv = ev_subj.RvHolder.itemView;
											BusinessEntity.GoodsGroup cur_entry = null;
											if(ev_subj.ItemObj != null && ev_subj.ItemObj instanceof BusinessEntity.GoodsGroup)
												cur_entry = (BusinessEntity.GoodsGroup)ev_subj.ItemObj; // Это - правильная ветка
											else
												cur_entry = CPM.GoodsGroupListData.L.get(ev_subj.ItemIdx); // Это - неправильная ветка
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.Name);
											{
												View ctl = iv.findViewById(R.id.LVITEM_EXPANDSTATUS);
												if(ctl != null && ctl instanceof ImageView) {
													ImageView imgv = (ImageView)ctl;
													if(ev_subj.IsFolder) {
														imgv.setVisibility(View.VISIBLE);
														if(ev_subj.IsCollapsedFolder)
															imgv.setImageResource(R.drawable.ic_triangleright03);
														else
															imgv.setImageResource(R.drawable.ic_triangledown03);
													}
													else
														imgv.setVisibility(View.GONE);
												}
											}
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODSGROUP, cur_entry.ID);
										}
										break;
								}
							}
							else if(SLib.IsRecyclerListAdapter(srcObj)) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
								switch(a.GetListRcId()) {
									case R.id.searchPaneListView:
										CPM.GetSearchPaneListViewItem(ev_subj.RvHolder.itemView, ev_subj.ItemIdx);
										break;
									case R.id.orderPrereqClientsListView:
										if(SLib.IsInRange(ev_subj.ItemIdx, CPM.CliListData)) {
											View iv = ev_subj.RvHolder.itemView;
											CommonPrereqModule.CliEntry cur_entry = (CommonPrereqModule.CliEntry)CPM.CliListData.get(ev_subj.ItemIdx);
											final int cur_cli_id = cur_entry.JsItem.optInt("id", 0);
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.JsItem.optString("nm", ""));
											{
												View v_debt_text = iv.findViewById(R.id.CTL_ORDERPREREQ_CLI_DEBT);
												if(DbtL != null) {
													SLib.SetCtrlVisibility(iv, R.id.LAYOUT_ORDERPREREQ_CLI_DEBT, View.VISIBLE);
													BusinessEntity.DebtList.ShortReplyEntry de = DbtL.GetDebt(cur_cli_id);
													if(v_debt_text != null) {
														String text;
														int shaperc = 0;
														if(de != null) {
															final boolean is_expired = de.IsExpired(CmdQueryDebt);
															if(de.Debt > 0) {
																shaperc = is_expired ? R.drawable.shape_debtvalue_undef : R.drawable.shape_debtvalue_positive;
																text = SLib.formatdouble(de.Debt, 2);
															}
															else {
																shaperc = is_expired ? R.drawable.shape_debtvalue_undef : R.drawable.shape_debtvalue_zero;
																text = is_expired ? "0" : "";
															}
														}
														else {
															shaperc = R.drawable.shape_debtvalue_undef;
															text = "";
														}
														v_debt_text.setBackground(getResources().getDrawable(shaperc, getTheme()));
														SLib.SetCtrlString(iv, R.id.CTL_ORDERPREREQ_CLI_DEBT, text);
													}
												}
												else {
													SLib.SetCtrlVisibility(iv, R.id.LAYOUT_ORDERPREREQ_CLI_DEBT, View.GONE);
												}
											}
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_PERSON, cur_cli_id);
											{
												ImageView ctl = (ImageView)iv.findViewById(R.id.ORDERPREREQ_CLI_EXPANDSTATUS);
												if(ctl != null) {
													ListView dlvrloc_lv = (ListView)iv.findViewById(R.id.dlvrLocListView);
													ArrayList <JSONObject> dlvr_loc_list = cur_entry.GetDlvrLocListAsArray();
													if(cur_entry.AddrExpandStatus == 0 || dlvr_loc_list == null) {
														ctl.setVisibility(View.INVISIBLE);
														SLib.SetCtrlVisibility(dlvrloc_lv, View.GONE);
													}
													else if(cur_entry.AddrExpandStatus == 1) {
														ctl.setVisibility(View.VISIBLE);
														ctl.setImageResource(R.drawable.ic_triangleleft03);
														SLib.SetCtrlVisibility(dlvrloc_lv, View.GONE);
													}
													else if(cur_entry.AddrExpandStatus == 2) {
														ctl.setVisibility(View.VISIBLE);
														ctl.setImageResource(R.drawable.ic_triangledown03);
														if(dlvrloc_lv != null) {
															dlvrloc_lv.setVisibility(View.VISIBLE);
															DlvrLocListAdapter adapter = new DlvrLocListAdapter(
																/*this*/iv.getContext(), R.layout.li_dlvrloc_by_client_sublist, dlvr_loc_list, DlvrLocGpslocSettingAllowed);
															dlvrloc_lv.setAdapter(adapter);
															{
																int total_items_height = SLib.CalcListViewHeight(dlvrloc_lv);
																if(total_items_height > 0) {
																	ViewGroup.LayoutParams params = dlvrloc_lv.getLayoutParams();
																	params.height = total_items_height;
																	dlvrloc_lv.setLayoutParams(params);
																	dlvrloc_lv.requestLayout();
																}
															}
															adapter.setNotifyOnChange(true);
															dlvrloc_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
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
												if(cur_entry.Expiry != null) {
													SLib.SetCtrlVisibility(iv, R.id.CTLGRP_GOODS_EXPIRY, View.VISIBLE);
													View v_text = iv.findViewById(R.id.ORDERPREREQ_GOODS_EXPIRY);
													if(v_text != null && v_text instanceof TextView) {
														SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_EXPIRY, cur_entry.Expiry.Format(SLib.DATF_DMY));
														int shaperc = 0;
														int ahead_expiry_days = CPM.GetAheadExpiryDays();
														SLib.LDATE now_date = SLib.GetCurDate();
														if(SLib.LDATE.Difference(cur_entry.Expiry, now_date) <= ahead_expiry_days)
															shaperc = R.drawable.shape_expiry_alert;
														else
															shaperc = R.drawable.shape_expiry_normal;
														v_text.setBackground(getResources().getDrawable(shaperc, getTheme()));
													}

												}
												else
													SLib.SetCtrlVisibility(iv, R.id.CTLGRP_GOODS_EXPIRY, View.GONE);
												BusinessEntity.SelectedPrice sp = cur_entry.Item.QueryPrice(CPM.QkListData, 0/*cliID*/);
												double _price = sp.GetValue();
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_PRICE, (_price > 0.0) ? CPM.FormatCurrency(_price) : "");
												{
													final double stock = cur_entry.Item.Stock;
													View stock_icon_view = iv.findViewById(R.id.CTL_GOODS_STOCKIMAGE);
													final boolean hide_stock = (CPM.GetOption_HideStock() || cur_entry.Item.HideStock);
													if(!hide_stock)
														SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_REST, (stock > 0.0) ? SLib.formatdouble(stock, 0) : "");
													if(stock_icon_view != null && stock_icon_view instanceof ImageView) {
														if(hide_stock)
															stock_icon_view.setVisibility(View.GONE);
														else {
															stock_icon_view.setVisibility(View.VISIBLE);
															if(stock > 0)
																((ImageView) stock_icon_view).setImageResource(R.drawable.ic_goodsstock01);
															else
																((ImageView) stock_icon_view).setImageResource(R.drawable.ic_goodsstock01_accented);
														}
													}
												}
												double _qtty = CPM.GetGoodsQttyInCurrentDocument(cur_id);
												if(_qtty > 0.0) {
													SLib.SetCtrlVisibility(iv, R.id.ORDERPREREQ_GOODS_ORDEREDQTY, View.VISIBLE);
													SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_ORDEREDQTY, String.format("%.0f", _qtty));
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
											final BusinessEntity.GoodsGroup cur_entry = CPM.GoodsGroupListData.L.get(ev_subj.ItemIdx);
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
									case R.id.orderPrereqRegistryListView: // Список зафиксированных заказов
										if(SLib.GetCount(CPM.RegistryHList) <= 0) {
											if(ev_subj.ItemIdx == 0) {
												// Пустая строка
												View iv = ev_subj.RvHolder.itemView;
												final int _vdlc = VdlDocs.GetCount();
												for(int i = 0; i < _vdlc; i++) {
													View ctl_view = iv.findViewById(i+1);
													if(ctl_view != null) {
														ViewDescriptionList.Item di = VdlDocs.Get(i);
														if(di != null) {
															if(di.Id == 1) { // indicator image
																if(ctl_view instanceof ImageView) {
																	((ImageView)ctl_view).setImageResource(0);
																}
															}
															else if(ctl_view instanceof TextView){
																((TextView)ctl_view).setText("");
															}
														}
													}
												}
											}
										}
										else if(SLib.IsInRange(ev_subj.ItemIdx, CPM.RegistryHList)) {
											View iv = ev_subj.RvHolder.itemView;
											Document.DisplayEntry cur_entry = CPM.RegistryHList.get(ev_subj.ItemIdx);
											if(cur_entry != null && cur_entry.H != null) {
												final int _vdlc = VdlDocs.GetCount();
												for(int i = 0; i < _vdlc; i++) {
													View ctl_view = iv.findViewById(i+1);
													if(ctl_view != null) {
														ViewDescriptionList.Item di = VdlDocs.Get(i);
														if(di != null) {
															String text = null;
															if(di.Id == 1) { // indicator image
																if(ctl_view instanceof ImageView) {
																	final int ds = StyloQDatabase.SecTable.Rec.GetDocStatus(cur_entry.H.Flags);
																	int ir = Document.GetImageResourceByDocStatus(ds);
																	if(ir != 0)
																		((ImageView)ctl_view).setImageResource(ir);
																}
															}
															else if(ctl_view instanceof TextView){
																if(di.Id == 2) { // date
																	SLib.LDATE d = cur_entry.GetNominalDate(CPM.GetOption_DueDateAsNominal());
																	if(d != null)
																		text = d.Format(SLib.DATF_DMY);
																}
																else if(di.Id == 3) { // code
																	text = cur_entry.H.Code;
																}
																else if(di.Id == 4) { // amount
																	text = CPM.FormatCurrency(cur_entry.H.Amount);
																}
																else if(di.Id == 5) { // client
																	if(cur_entry.H.ClientID > 0) {
																		JSONObject cli_entry = CPM.FindClientEntry(cur_entry.H.ClientID);
																		if(cli_entry != null)
																			text = cli_entry.optString("nm", "");
																	}
																}
																else if(di.Id == 6) { // memo
																	text = cur_entry.H.Memo;
																}
																((TextView)ctl_view).setText(text);
															}
														}
													}
												}
											}
										}
										break;
									case R.id.orderPrereqOrdrListView: // Текущий заказ (точнее, его строки)
										final int cc = CPM.GetCurrentDocumentTransferListCount();
										if(ev_subj.ItemIdx < cc) {
											View iv = ev_subj.RvHolder.itemView;
											final Document _doc = CPM.GetCurrentDocument();
											final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
											if(ti != null) {
												CommonPrereqModule.WareEntry goods_item = CPM.FindGoodsItemByGoodsID(ti.GoodsID);
												int    uom_id = (goods_item != null && goods_item.Item != null) ? goods_item.Item.UomID : 0;
												SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, (goods_item != null) ? goods_item.Item.Name : "");
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_TI_PRICE, (ti.Set != null) ? CPM.FormatCurrency(ti.Set.Price) : "");
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_TI_QTTY, (ti.Set != null) ? CPM.FormatQtty(ti.Set.Qtty, uom_id, false) : "");
												double item_amont = (ti.Set != null) ? (ti.Set.Qtty * ti.Set.Price) : 0.0;
												SLib.SetCtrlString(iv, R.id.ORDERPREREQ_TI_AMOUNT, " = " + CPM.FormatCurrency(item_amont));
											}
										}
										break;
								}
							}
						}
						else {
							;
						}
					}
				}
				break;
			case SLib.EV_CREATEFRAGMENT: result = CPM.OnEvent_CreateFragment(subj); break;
			case SLib.EV_SETUPFRAGMENT:
				if(subj != null && subj instanceof View) {
					SLib.PPObjID selected_search_oid = CPM.SearchResult_GetSelectedOid();
					assert(selected_search_oid != null);
					if(srcObj != null && srcObj instanceof SLib.SlFragmentStatic) {
						SLib.SlFragmentStatic fragment = (SLib.SlFragmentStatic)srcObj;
						View lv = null;
						View fv = (View)subj;
						boolean settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_GOODS, fv, R.id.orderPrereqGoodsListView, R.layout.li_orderprereq_goods);
						if(!settled) {
							lv = fv.findViewById(R.id.orderPrereqRegistryListView);
							if(lv != null) {
								StyloQApp app_ctx = GetAppCtx();
								if(app_ctx != null) {
									SLib.Margin fld_mrgn = new SLib.Margin(8, 12, 8, 12);
									VdlDocs = new ViewDescriptionList();
									{ // #0
										ViewDescriptionList.Item col = new ViewDescriptionList.Item(1, null, 0);
										col.Flags |= ViewDescriptionList.Item.fImage;
										col.FixedWidth = 32;
										col.FixedHeight = 32;
										col.Mrgn = fld_mrgn;
										VdlDocs.AddItem(col);
									}
									{ // #1
										ViewDescriptionList.Item col = new ViewDescriptionList.Item(2, app_ctx.GetString(/*"billdate"*/"date"), R.style.OrderListItemText);
										col.Mrgn = fld_mrgn;
										VdlDocs.AddItem(col);
									}
									{ // #2
										ViewDescriptionList.Item col = new ViewDescriptionList.Item(3, app_ctx.GetString(/*"billno"*/"number"), R.style.OrderListItemText);
										col.TotalFunc = SLib.AGGRFUNC_COUNT;
										col.Mrgn = fld_mrgn;
										VdlDocs.AddItem(col);
									}
									{ // #3
										ViewDescriptionList.Item col = new ViewDescriptionList.Item(4, app_ctx.GetString(/*"billamount"*/"amount"), R.style.OrderListItemText);
										col.TotalFunc = SLib.AGGRFUNC_SUM;
										col.Mrgn = fld_mrgn;
										col.ForceAlignment = -1;
										VdlDocs.AddItem(col);
									}
									if(CPM.GetAgentID() > 0) { // Агентские заказы - требуется указание клиента
										{ // #4
											ViewDescriptionList.Item col = new ViewDescriptionList.Item(5, app_ctx.GetString("client"), R.style.OrderListItemText);
											col.Mrgn = fld_mrgn;
											VdlDocs.AddItem(col);
										}
									}
									{ // #5|#4
										ViewDescriptionList.Item col = new ViewDescriptionList.Item(6, app_ctx.GetString("memo"), R.style.OrderListItemText);
										col.Mrgn = fld_mrgn;
										VdlDocs.AddItem(col);
									}
									PreprocessRegistryData();
									{
										LinearLayout header_layout = (LinearLayout) fv.findViewById(R.id.orderPrereqRegistryListHeader);
										if(header_layout != null)
											ViewDescriptionList.SetupItemLayout(VdlDocs, this, header_layout, 1);
										if(VdlDocs.IsThereTotals()) {
											LinearLayout bottom_layout = (LinearLayout)fv.findViewById(R.id.orderPrereqRegistryListBottom);
											if(bottom_layout != null)
												ViewDescriptionList.SetupItemLayout(VdlDocs, this, bottom_layout, 2);
										}
									}
									((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
									SetupRecyclerListView(fv, R.id.orderPrereqRegistryListView, /*R.layout.li_orderprereq_order*/0);
								}
							}
							else {
								settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_GOODSGROUP, fv, R.id.orderPrereqGoodsGroupListView, R.layout.li_simple);
								if(!settled) {
									settled = CPM.OnSetupFragment_SetupObjListView(SLib.PPOBJ_BRAND, fv, R.id.orderPrereqBrandListView, R.layout.li_simple);
									if(!settled) {
										settled = CPM.OnSetupFragment_SetupObjListView(0, fv, R.id.orderPrereqOrdrListView, R.layout.li_orderprereq_ordrti);
										if(!settled) {
											settled = CPM.OnSetupFragment_SetupObjListView(0, fv, R.id.orderPrereqRegistryListView, R.layout.li_orderprereq_order);
											if(!settled) {
												lv = fv.findViewById(R.id.orderPrereqClientsListView);
												if(lv != null) {
													((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
													SetupRecyclerListView(fv, R.id.orderPrereqClientsListView, R.layout.li_orderprereq_client);
													if(selected_search_oid.Type == SLib.PPOBJ_PERSON) {
														SLib.RequestRecyclerListViewPosition((RecyclerView) lv, FindClientItemIndexByID(selected_search_oid.Id));
														CPM.SearchResult_ResetSelectedItemIndex();
													}
													else if(selected_search_oid.Type == SLib.PPOBJ_LOCATION) {
														// @todo
													}
												}
												else {
													lv = fv.findViewById(R.id.searchPaneListView);
													if(lv != null) {
														CPM.SetupSearchPaneListView(fv, lv);
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
				break;
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj != null && subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder == null) {
							if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.orderPrereqRegistryListView) {
								LinearLayout _lo = ViewDescriptionList.SetupItemLayout(VdlDocs, this, null, 0);
								if(_lo != null) {
									SLib.RecyclerListAdapter adapter = SLib.IsRecyclerListAdapter(srcObj) ? (SLib.RecyclerListAdapter)srcObj : null;
									result = new SLib.RecyclerListViewHolder(_lo, adapter);
								}
							}
						}
						else {
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.buttonOrder);
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.ORDERPREREQ_CLI_EXPANDSTATUS);
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.CTL_ORDERPREREQ_CLI_DEBT); // @v11.5.4
							result = ev_subj.RvHolder;
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent)subj : null;
					if(ev_subj != null && srcObj != null) {
						if(ev_subj.RvHolder == null) {
							if(srcObj instanceof ListView) {
								if(((ListView)srcObj).getId() == R.id.searchPaneTerminalListView) {
									final SLib.PPObjID sr_oid = CPM.SearchResult_ProcessSelection(ev_subj.ItemObj);
									if(sr_oid != null && !sr_oid.IsEmpty()) {
										int _idx = -1;
										switch(sr_oid.Type) {
											case SLib.PPOBJ_GOODS:
												_idx = CPM.FindGoodsItemIndexByID(sr_oid.Id);
												GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, _idx, -1);
												break;
											case SLib.PPOBJ_PERSON:
												_idx = FindClientItemIndexByID(sr_oid.Id);
												GotoTab(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView, _idx, -1);
												break;
											case SLib.PPOBJ_LOCATION:
												JSONObject cli_js_obj = CPM.FindClientEntryByDlvrLocID(sr_oid.Id);
												if(cli_js_obj != null) {
													int cli_id = cli_js_obj.optInt("id", 0);
													if(cli_id > 0) {
														_idx = FindClientItemIndexByID(cli_id);
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
										}
									}
								}
								else if(((ListView)srcObj).getId() == R.id.dlvrLocListView) {
									if(ev_subj.ItemObj != null && ev_subj.ItemObj instanceof JSONObject) {
										if(SetCurrentOrderClient(null, (JSONObject)ev_subj.ItemObj)) {
											GotoTab(CommonPrereqModule.Tab.tabCurrentDocument, R.id.orderPrereqOrdrListView, -1, -1);
										}
									}
								}
							}
						}
						else if(SLib.IsRecyclerTreeListAdapter(srcObj)) {
							AmrTreeView.Adapter a = (AmrTreeView.Adapter)srcObj;
							StyloQApp app_ctx = GetAppCtx();
							boolean do_update_goods_list_and_toggle_to_it = false;
							final int rc_id = a.GetListRcId();
							if(app_ctx != null && ev_subj.ItemIdx >= 0) {
								if(rc_id == R.id.orderPrereqGoodsGroupListView) {
									if(ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GetGoodsGroupCount()) {
										final int group_id = CPM.GoodsGroupListData.L.get(ev_subj.ItemIdx).ID;
										if(CPM.SetGoodsFilterByGroup(group_id)) {
											SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.VISIBLE);
											do_update_goods_list_and_toggle_to_it = true;
										}
									}
								}
								if(do_update_goods_list_and_toggle_to_it) {
									GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, -1, -1);
								}
							}
						}
						else if(SLib.IsRecyclerListAdapter(srcObj)) {
							SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
							StyloQApp app_ctx = GetAppCtx();
							boolean do_update_goods_list_and_toggle_to_it = false;
							final int rc_id = a.GetListRcId();
							if(app_ctx != null && ev_subj.ItemIdx >= 0) {
								switch(rc_id) {
									case R.id.orderPrereqGoodsListView:
										if(ev_subj.ItemIdx < CPM.GetGoodsListSize()) {
											if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.buttonOrder) {
												CommonPrereqModule.WareEntry item = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
												CPM.OpenTransferItemDialog(item, 0.0);
											}
										}
										break;
									case R.id.orderPrereqClientsListView:
										if(SLib.IsInRange(ev_subj.ItemIdx, CPM.CliListData)) {
											CommonPrereqModule.CliEntry item = CPM.CliListData.get(ev_subj.ItemIdx);
											if(item != null && ev_subj.ItemView != null) {
												if(ev_subj.ItemView.getId() == R.id.ORDERPREREQ_CLI_EXPANDSTATUS) {
													// change expand status
													if(item.AddrExpandStatus == 1) {
														item.AddrExpandStatus = 2;
														a.notifyItemChanged(ev_subj.ItemIdx);
													}
													else if(item.AddrExpandStatus == 2) {
														item.AddrExpandStatus = 1;
														a.notifyItemChanged(ev_subj.ItemIdx);
													}
												}
												else if(ev_subj.ItemView.getId() == R.id.CTL_ORDERPREREQ_CLI_DEBT) {
													if(DbtL != null) {
														final int cur_cli_id = item.JsItem.optInt("id", 0);
														if(cur_cli_id > 0) {
															BusinessEntity.DebtList.ShortReplyEntry de = DbtL.GetDebt(cur_cli_id);
															if(de == null || de.IsExpired(CmdQueryDebt)) {
																if(CmdQueryDebt != null) {
																	// query
																	boolean force_query = true;
																	try {
																		JSONObject js_query = new JSONObject();
																		String cmd_text = CmdQueryDebt.Uuid.toString();
																		js_query.put("cmd", cmd_text);
																		js_query.put("time", System.currentTimeMillis());
																		js_query.put("arid", cur_cli_id);
																		app_ctx.RunSvcCommand(CPM.SvcIdent, CmdQueryDebt, js_query, force_query, this);
																	} catch(StyloQException | JSONException exn) {
																		;
																	}
																}
															}
															else {
																// detail
																if(de.Debt > 0.0) {
																	BusinessEntity.ArDebtList debt_detail = DbtL.GetDebtDetail(cur_cli_id);
																	if(debt_detail != null) {
																		DebtDetailDialog dialog = new DebtDetailDialog(this, debt_detail);
																		dialog.show();
																	}
																}
															}
														}
													}
												}
												else {
													// @v11.4.8 {
													ArrayList <JSONObject> dlvr_loc_list = item.GetDlvrLocListAsArray();
													if(dlvr_loc_list == null || dlvr_loc_list.size() == 0) {
														// У контрагента нет адресов доставки - можно выбрать просто заголовочную запись
														if(SetCurrentOrderClient(item.JsItem, null)) {
															GotoTab(CommonPrereqModule.Tab.tabCurrentDocument, R.id.orderPrereqOrdrListView, -1, -1);
														}
													}
													// } @v11.4.8
												}
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
									case R.id.orderPrereqOrdrListView:
										final int cc = CPM.GetCurrentDocumentTransferListCount();
										if(ev_subj.ItemIdx < cc) {
											final Document _doc = CPM.GetCurrentDocument();
											final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
											if(ti != null) {
												CommonPrereqModule.TransferItemDialog dialog = new CommonPrereqModule.TransferItemDialog(this, CPM, ti);
												dialog.show();
											}
										}
										break;
									case R.id.orderPrereqRegistryListView:
										if(SLib.IsInRange(ev_subj.ItemIdx, CPM.RegistryHList)) {
											Document.DisplayEntry entry = CPM.RegistryHList.get(ev_subj.ItemIdx);
											if(entry != null) {
												if(CPM.LoadDocument(entry.H.ID)) {
													SetupCurrentDocument(true, false);
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
			case SLib.EV_CBSELECTED:
				if(subj != null && subj instanceof SLib.ListViewEvent) {
					SLib.ListViewEvent lve = (SLib.ListViewEvent)subj;
					if(lve.ItemIdx >= 0 && lve.ItemId >= 0) {
						CommonPrereqModule.TabEntry te = null;
						View v = findViewById(R.id.VIEWPAGER_ORDERPREREQ);
						if(v != null && v instanceof ViewPager2) {
							for(CommonPrereqModule.TabEntry iter : CPM.TabList) {
								if(iter != null && iter.TabId == CommonPrereqModule.Tab.tabSearch) {
									te = iter;
									break;
								}
							}
							if(te != null && te.TabView != null)
								CPM.SelectSearchPaneObjRestriction(te.TabView.getView(), (int)lve.ItemId);
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
						case R.id.tbButtonHelp:
							{
								final CommonPrereqModule.Tab current_tab_id = CPM.GetCurrentTabId();
								if(current_tab_id == CommonPrereqModule.Tab.tabRegistry) {
									StyloQApp app_ctx = GetAppCtx();
									if(app_ctx != null) {
										HelpPane.DialogData help_data = new HelpPane.DialogData();
										help_data.Text = SLib.ExpandString(app_ctx, "@{styloq_hlp_registrylist_order}");
										help_data.Legend = new ArrayList<HelpPane.LegendEntry>();
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloq_document_finished, "@{styloqdocstatus_finished_succ}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloq_document_itrm, "@{styloqdocstatus_waitforapprorexec}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloq_document_draft, "@{styloqdocstatus_draft}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloqdocstatus_approved, "@{styloqdocstatus_approved}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloqdocstatus_rejected, "@{styloqdocstatus_rejected}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloqdocstatus_cancelled, "@{styloqdocstatus_cancelled}"));
										help_data.Legend.add(new HelpPane.LegendEntry(R.drawable.ic_styloqdocstatus_cancelled, "@{styloqdocstatus_cancelleddraft}")); // @todo change icon
										HelpPane dialog = new HelpPane(this, help_data);
										dialog.show();
									}
								}
							}
							break;
						case R.id.tbButtonSearch:
							{
								final CommonPrereqModule.Tab current_tab_id = CPM.GetCurrentTabId();
								int   obj_to_search = -1;
								if(current_tab_id == CommonPrereqModule.Tab.tabClients)
									obj_to_search = SLib.PPOBJ_PERSON;
								else if(current_tab_id == CommonPrereqModule.Tab.tabGoods)
									obj_to_search = SLib.PPOBJ_GOODS;
								else if(current_tab_id == CommonPrereqModule.Tab.tabGoodsGroups)
									obj_to_search = SLib.PPOBJ_GOODSGROUP;
								else if(current_tab_id == CommonPrereqModule.Tab.tabBrands)
									obj_to_search = SLib.PPOBJ_BRAND;
								CPM.GotoSearchTab(R.id.VIEWPAGER_ORDERPREREQ, obj_to_search);
							}
							break;
						case R.id.tbButtonScan:
							{
								ScanOptions options = new ScanOptions();
								options.setPrompt(""); //
								options.setCameraId(Camera.CameraInfo.CAMERA_FACING_BACK/*CAMERA_FACING_FRONT*/); // Use a specific camera of the device
								options.setOrientationLocked(true);
								options.setBeepEnabled(true);
								options.setCaptureActivity(StyloQZxCaptureActivity.class);
								//options.createScanIntent()
								BarcodeLauncher.launch(options);
							}
							break;
						case R.id.tbButtonClearFiter:
							CPM.ResetGoodsFiter();
							SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
							GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, -1, -1);
							break;
						case R.id.tbButtonLocalTabConfig:
							{
								if(CPM.Cs != null) {
									CommonPrereqModule.RegistryFiltDialog dialog = new CommonPrereqModule.RegistryFiltDialog(this, CPM.Cs.Rf);
									dialog.getWindow().setLayout(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
									dialog.show();
								}
							}
							break;
						case R.id.CTLBUT_SEARCHPANE_OPTIONS:
							{
								CommonPrereqModule.TabEntry te = null;
								View v = findViewById(R.id.VIEWPAGER_ORDERPREREQ);
								if(v != null && v instanceof ViewPager2) {
									for(int tidx = 0; te == null && tidx < CPM.TabList.size(); tidx++) {
										if(CPM.TabList.get(tidx).TabId == CommonPrereqModule.Tab.tabSearch)
											te = CPM.TabList.get(tidx);
									}
									if(te != null && te.TabView != null)
										CPM.OpenSearchPaneObjRestriction(te.TabView.getView());
								}
							}
							break;
						case R.id.CTL_DOCUMENT_DUEDATE_NEXT:
							{
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
								if(te != null) {
									Document cd = CPM.GetCurrentDocument();
									if(cd != null && cd.H != null && cd.H.IncrementDueDate(false)) {
										GetFragmentData(te.TabView); // @v11.4.8
										NotifyCurrentDocumentChanged();
									}
								}
							}
							break;
						case R.id.CTL_DOCUMENT_DUEDATE_PREV:
							{
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
								if(te != null) {
									Document cd = CPM.GetCurrentDocument();
									if(cd != null && cd.H != null && cd.H.DecrementDueDate(false)) {
										GetFragmentData(te.TabView); // @v11.4.8
										NotifyCurrentDocumentChanged();
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
						case R.id.CTL_DOCUMENT_BACK_CLI:
							{
								final Document cd = CPM.GetCurrentDocument();
								if(cd != null && cd.H != null) {
									if(Document.DoesStatusAllowModifications(cd.GetDocStatus())) {
										//cd.H.ClientID > 0 &&
										JSONObject cli_js_obj = CPM.FindClientEntry(cd.H.ClientID);
										if(cli_js_obj != null) {
											int _idx = FindClientItemIndexByID(cd.H.ClientID);
											int _dlvr_loc_idx = (cd.H.DlvrLocID > 0) ? CPM.FindDlvrLocEntryIndexInCliEntry(cli_js_obj, cd.H.DlvrLocID) : 0;
											GotoTab(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView, _idx, _dlvr_loc_idx);
										}
										else
											GotoTab(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView, -1, -1);
									}
								}
							}
							break;
					}
					if(acn != null) {
						switch(acn.Action) {
							case Document.editactionClose:
								{
									// Просто закрыть сеанс редактирования документа (/*изменения и*/передача сервису не предполагаются)
									{
										//
										// Для драфт-документа сохраняем изменения //
										//
										Document _doc = CPM.GetCurrentDocument();
										if(_doc != null && _doc.GetDocStatus() == StyloQDatabase.SecStoragePacket.styloqdocstDRAFT) {
											CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
											if(te != null)
												GetFragmentData(te.TabView);
											NotifyCurrentDocumentChanged();
										}
									}
									CPM.ResetCurrentDocument();
									NotifyCurrentDocumentChanged();
									GotoTab(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView, -1, -1);
									//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
									SetupCurrentDocument(false, true);
								}
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
								GotoTab(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView, -1, -1);
								//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
								SetupCurrentDocument(false, true);
								break;
							case Document.editactionCancelDocument:
								{
									// Отменить документ с передачей сервису факта отмены
									CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabCurrentDocument);
									if(te != null)
										GetFragmentData(te.TabView);
									ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
									CPM.CancelCurrentDocument();
								}
								break;
						}
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				if(srcObj != null) {
					if(srcObj instanceof CommonPrereqModule.TransferItemDialog) {
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
					else if(srcObj instanceof SLib.ConfirmDialog) {

					}
					else if(srcObj instanceof CommonPrereqModule.RegistryFiltDialog) {
						if(subj != null && subj instanceof CommonPrereqModule.RegistryFilt) {
							if(CPM.Cs != null) {
								CPM.Cs.Rf = (CommonPrereqModule.RegistryFilt)subj;
								CPM.RegisterCurrentStateModification(null); // @v11.7.0
								CPM.MakeCurrentDocList();
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabRegistry);
								if(te != null && te.TabView != null) {
									View fv = te.TabView.getView();
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView);
									if(VdlDocs.IsThereTotals()) {
										LinearLayout bottom_layout = (LinearLayout) fv.findViewById(R.id.orderPrereqRegistryListBottom);
										if(bottom_layout != null) {
											PreprocessRegistryData();
											ViewDescriptionList.SetupItemLayout(VdlDocs, this, bottom_layout, 2);
											fv.refreshDrawableState();
										}
									}
								}
							}
						}
					}
				}
				break;
			case SLib.EV_GEOLOCDETECTED:
				if(subj != null && subj instanceof Location) {
					StyloQApp app_ctx = GetAppCtx();
					SLib.PPObjID oid = (srcObj != null && srcObj instanceof SLib.PPObjID) ? (SLib.PPObjID)srcObj : null;
					if(app_ctx != null && oid != null && oid.Type == SLib.PPOBJ_BILL) {
						if(oid.Id == 0) {
							Document current_doc = CPM.GetCurrentDocument();
							if(current_doc != null && current_doc.H.CreationGeoLoc == null) {
								current_doc.H.CreationGeoLoc = new SLib.GeoPosLL((Location)subj);
							}
						}
					}
				}
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
					StyloQApp app_ctx = GetAppCtx();
					if(ir.OriginalCmdItem != null) {
						if(ir.OriginalCmdItem.BaseCmdId == StyloQCommand.sqbcDebtList) {
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
									JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
									BusinessEntity.ArDebtList ard_list = new BusinessEntity.ArDebtList();
									if(ard_list.FromJson(js_reply)) {
										if(DbtL.Include(ard_list) >= 0) {
											app_ctx.StoreDebtList(CPM.SvcIdent, DbtL);
											// @todo Здесь достаточно обновить только одну позицию списка, а не весь список!
											NotifyTabContentChanged(CommonPrereqModule.Tab.tabClients, R.id.orderPrereqClientsListView);
										}
									}
								}
							}
							else {
								// @todo
							}
						}
						else if(ir.OriginalCmdItem.Name.equalsIgnoreCase("PostDocument")) {
							CPM.CurrentDocument_RemoteOp_Finish();
							ScheduleRTmr(null, 0, 0);
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								CPM.MakeCurrentDocList();
								CPM.ResetCurrentDocument();
								NotifyCurrentDocumentChanged();
								if(SLib.GetCount(CPM.RegistryHList) > 0) {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabRegistry, View.VISIBLE);
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView);
									GotoTab(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView, -1, -1);
								}
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
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
						else if(ir.OriginalCmdItem.Name.equalsIgnoreCase("CancelDocument")) {
							CPM.CurrentDocument_RemoteOp_Finish();
							ScheduleRTmr(null, 0, 0);
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								CPM.MakeCurrentDocList();
								CPM.ResetCurrentDocument();
								NotifyCurrentDocumentChanged();
								if(SLib.GetCount(CPM.RegistryHList) > 0) {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabRegistry, View.VISIBLE);
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView);
									GotoTab(CommonPrereqModule.Tab.tabRegistry, R.id.orderPrereqRegistryListView, -1, -1);
								}
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentDocument, View.GONE);
							}
							else {
								; // @todo
							}
						}
					}
				}
				break;
		}
		return result;
	}
	private final ActivityResultLauncher<ScanOptions> BarcodeLauncher = registerForActivityResult(new ScanContract(), activity_result ->
	{
		String contents = activity_result.getContents();
		byte [] contents_bytes = (contents != null) ? contents.getBytes() : null; // @debug
		Intent original_intent = activity_result.getOriginalIntent();
		BusinessEntity.PreprocessBarcodeResult pbcr = CPM.PreprocessBarcode(contents);
		if(pbcr != null) {
			//byte[] bytes_contents = contents.getBytes();
			//Intent _intent = this.getIntent();
			//boolean is_processed = false;
			StyloQApp app_ctx = (StyloQApp)getApplicationContext();
			if(app_ctx != null) {
				//String _action = original_intent.getStringExtra("action");
				ArrayList<CommonPrereqModule.WareEntry> goods_list = CPM.SearchGoodsItemsByBarcode(pbcr.FinalCode);
				if(goods_list != null) {
					assert(goods_list.size() > 0);
					CommonPrereqModule.WareEntry goods_entry = goods_list.get(0);
					final int _idx = CPM.FindGoodsItemIndexByID(goods_entry.Item.ID);
					GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.orderPrereqGoodsListView, _idx, -1);
					CPM.OpenTransferItemDialog(goods_entry, pbcr.Qtty);
				}
				else {
					app_ctx.DisplayError(this, app_ctx.GetErrorText(ppstr2.PPERR_GDSBYBARCODENFOUND, contents), 0);
				}
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