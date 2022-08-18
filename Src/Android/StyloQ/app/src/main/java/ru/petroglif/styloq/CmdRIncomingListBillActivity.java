// CmdRIncomingListBillActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageView;
import android.widget.ListView;

import androidx.annotation.IdRes;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class CmdRIncomingListBillActivity extends SLib.SlActivity {
	public CommonPrereqModule CPM;
	private ArrayList <Document.EditAction> DocEditActionList;
	private int ActionFlags;
	public CmdRIncomingListBillActivity()
	{
		CPM = new CommonPrereqModule(this);
		ActionFlags = 0;
		DocEditActionList = null;
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_INCOMINGLISTBILL;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && (CPM.TabList == null || force)) {
			CPM.TabList = new ArrayList<CommonPrereqModule.TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			CmdROrderPrereqActivity.TabInitEntry[] tab_init_list = {
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabIncomingList, R.layout.layout_incominglist_bill_main, "--MAIN--", true),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabCurrentOrder, R.layout.layout_incominglist_bill_selected, "@{selecteddocument}", true),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabGoodsGroups, R.layout.layout_orderprereq_goodsgroups, "@{group_pl}", (CPM.GoodsGroupListData != null)),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabBrands, R.layout.layout_orderprereq_brands, "@{brand_pl}", (CPM.BrandListData != null)),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabGoods, R.layout.layout_orderprereq_goods, "@{ware_pl}", (CPM.GoodsListData != null)),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabClients, R.layout.layout_orderprereq_clients, "@{client_pl}", (CPM.CliListData != null)),
					new CmdROrderPrereqActivity.TabInitEntry(CommonPrereqModule.Tab.tabSearch, R.layout.layout_searchpane, "[search]", true),
			};
			for(int i = 0; i < tab_init_list.length; i++) {
				final CmdROrderPrereqActivity.TabInitEntry _t = tab_init_list[i];
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
					SLib.SetCtrlString(convertView, R.id.CTL_TI_GOODSNAME, (ware_entry != null) ? ware_entry.JsItem.optString("nm", null) : null);
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
	private void GotoTab(CommonPrereqModule.Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		CPM.GotoTab(tab, R.id.VIEWPAGER_INCOMINGLISTBILL, recyclerViewToUpdate, goToIndex, nestedIndex);
	}
	private void SetupCurrentDocument(boolean gotoTabIfNotEmpty, boolean removeTabIfEmpty)
	{
		//
		// При попытке скрыть и потом показать табы они перерисовываются с искажениями.
		// по этому не будем злоупотреблять такими фокусами.
		//
		if(!CPM.IsCurrentDocumentEmpty()) {
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.VISIBLE);
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
				GotoTab(CommonPrereqModule.Tab.tabCurrentOrder, R.id.CTL_DOCUMENT_TILIST, -1, -1);
		}
		else {
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.VISIBLE);
			if(removeTabIfEmpty)
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
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
						String svc_reply_doc_json = null;
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
							StyloQDatabase db = app_ctx.GetDB();
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
								CPM.GetCommonJsonFactors(js_head);
								ActionFlags = Document.IncomingListActionsFromString(js_head.optString("actions", null)); // @v11.4.8
								CPM.MakeUomListFromCommonJson(js_head);
								CPM.MakeGoodsGroupListFromCommonJson(js_head);
								CPM.MakeGoodsListFromCommonJson(js_head);
								CPM.MakeBrandListFromCommonJson(js_head);
								CPM.MakeClientListFromCommonJson(js_head);
								CPM.MakeIncomingDocFromCommonJson(js_head);
								if(CPM.IncomingDocListData != null) {
									for(int i = 0; i < CPM.IncomingDocListData.size(); i++) {
										Document doc_entry = CPM.IncomingDocListData.get(i);
										if(doc_entry != null) {
											if(doc_entry.TiList != null && doc_entry.TiList.size() > 0)
												doc_entry.DetailExpandStatus_Ti = 1;
											else
												doc_entry.DetailExpandStatus_Ti = 0;
										}
									}
								}
								//
								//MakeSimpleSearchIndex();
							}
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
									SLib.SetupTabLayoutListener(lo_tab, view_pager);
									/*
									if(CPM.IsCurrentDocumentEmpty())
										CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
									if(CPM.OrderHList == null || CPM.OrderHList.size() <= 0)
										CPM.SetTabVisibility(CommonPrereqModule.Tab.tabOrders, View.GONE);
									 */
									//SetTabVisibility(Tab.tabSearch, View.GONE);
								}
							}
							/*
								CommonPrereqModule.Tab.tabCurrentOrder
								CommonPrereqModule.Tab.tabGoodsGroups
								CommonPrereqModule.Tab.tabBrands
								CommonPrereqModule.Tab.tabGoods
								CommonPrereqModule.Tab.tabClients
							 */
							if(ActionFlags == 0) {
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.GONE);
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.GONE);
							}
							else {
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.VISIBLE);
								if((ActionFlags & Document.actionGoodsItemCorrection) != 0) {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, (CPM.BrandListData != null && CPM.BrandListData.size() > 0) ? View.VISIBLE : View.GONE);
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
								}
								else {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBrands, View.GONE);
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
								}
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabClients, View.GONE);
							}
							SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
						}
					} catch(StyloQException exn) {
						;
					} catch(JSONException exn) {
						;
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				if(srcObj instanceof SLib.FragmentAdapter) {
					CreateTabList(false);
					result = new Integer(CPM.TabList.size());
				}
				else if(srcObj instanceof SLib.RecyclerListAdapter) {
					SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
					switch(a.GetListRcId()) {
						case R.id.CTL_INCOMINGLIST_BILL_LIST: result = new Integer(CPM.IncomingDocListData != null ? CPM.IncomingDocListData.size() : 0); break;
						case R.id.CTL_DOCUMENT_TILIST: result = new Integer(CPM.GetCurrentDocumentTransferListCount()); break;
						/*
						case R.id.orderPrereqGoodsListView: result = new Integer(CPM.GetGoodsListSize()); break;
						case R.id.orderPrereqGoodsGroupListView: result = new Integer((CPM.GoodsGroupListData != null) ? CPM.GoodsGroupListData.size() : 0); break;
						case R.id.orderPrereqBrandListView: result = new Integer((CPM.BrandListData != null) ? CPM.BrandListData.size() : 0); break;
						case R.id.orderPrereqOrdrListView: result = new Integer(CPM.GetCurrentDocumentTransferListCount()); break;
						case R.id.orderPrereqOrderListView:
							result = new Integer((CPM.OrderHList != null) ? CPM.OrderHList.size() : 0);
							break;
						case R.id.orderPrereqClientsListView: result = new Integer((CPM.CliListData != null) ? CPM.CliListData.size() : 0); break;
						case R.id.searchPaneListView:
							result = new Integer((CPM.SearchResult != null) ? CPM.SearchResult.GetObjTypeCount() : 0);
						break;
						 */
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemIdx >= 0) {
						if(ev_subj.RvHolder != null) {
							// RecyclerView
							if(srcObj != null && srcObj instanceof SLib.RecyclerListAdapter) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter) srcObj;
								if(a.GetListRcId() == R.id.CTL_DOCUMENT_TILIST) {
									final int cc = CPM.GetCurrentDocumentTransferListCount();
									if(ev_subj.ItemIdx < cc) {
										View iv = ev_subj.RvHolder.itemView;
										final Document _doc = CPM.GetCurrentDocument();
										final Document.TransferItem ti = _doc.TiList.get(ev_subj.ItemIdx);
										if(ti != null) {
											CommonPrereqModule.WareEntry goods_item = CPM.FindGoodsItemByGoodsID(ti.GoodsID);
											int    uom_id = 0;
											if(goods_item != null && goods_item.JsItem != null)
												uom_id = goods_item.JsItem.optInt("uomid", 0);
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_GOODSNAME, (goods_item != null) ? goods_item.JsItem.optString("nm", "") : "");
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_PRICE, (ti.Set != null) ? CPM.FormatCurrency(ti.Set.Price) : "");
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_QTTY, (ti.Set != null) ? CPM.FormatQtty(ti.Set.Qtty, uom_id, false) : "");
											double item_amont = (ti.Set != null) ? (ti.Set.Qtty * ti.Set.Price) : 0.0;
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_TI_AMOUNT, " = " + CPM.FormatCurrency(item_amont));
										}
									}
								}
								else if(a.GetListRcId() == R.id.CTL_INCOMINGLIST_BILL_LIST) {
									if(CPM.IncomingDocListData != null && ev_subj.ItemIdx < CPM.IncomingDocListData.size()) {
										View iv = ev_subj.RvHolder.itemView;
										Document cur_entry = (Document)CPM.IncomingDocListData.get(ev_subj.ItemIdx);
										if(cur_entry.H != null) {
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
											if(cur_entry.H.AgentID > 0) {
												SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_AGENT, View.VISIBLE);
											}
											else
												SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_AGENT, View.VISIBLE);
											if(SLib.GetLen(cur_entry.H.Memo) > 0) {
												SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_MEMO, View.VISIBLE);
												SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_MEMO, cur_entry.H.Memo);
											}
											else
												SLib.SetCtrlVisibility(iv, R.id.CTL_DOCUMENT_MEMO, View.GONE);
											{
												ImageView ctl = (ImageView) iv.findViewById(R.id.CTL_DOCUMENT_EXPANDSTATUS);
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
														else {
															ctl.setVisibility(View.GONE);
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
									if(status_image_rc_id == 0) {
										v.setVisibility(View.GONE);
									}
								}
							}
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
							result = ev_subj.RvHolder;
						}
					}
				}
				break;
			case SLib.EV_CREATEFRAGMENT:
				if(subj instanceof Integer) {
					int item_idx = (Integer)subj;
					if(CPM.TabList != null && item_idx >= 0 && item_idx < CPM.TabList.size()) {
						CommonPrereqModule.TabEntry cur_entry = (CommonPrereqModule.TabEntry)CPM.TabList.get(item_idx);
						if(cur_entry.TabView != null)
							result = cur_entry.TabView;
					}
				}
				break;
			case SLib.EV_SETUPFRAGMENT:
				if(subj != null && subj instanceof View) {
					final int selected_search_idx = (CPM.SearchResult != null) ? CPM.SearchResult.GetSelectedItemIndex() : -1;
					final int selected_search_objtype = (selected_search_idx >= 0) ? CPM.SearchResult.List.get(selected_search_idx).ObjType : 0;
					final int selected_search_objid = (selected_search_idx >= 0) ? CPM.SearchResult.List.get(selected_search_idx).ObjID : 0;
					if(srcObj != null && srcObj instanceof SLib.SlFragmentStatic) {
						SLib.SlFragmentStatic fragment = (SLib.SlFragmentStatic) srcObj;
						View fv = (View) subj;
						View lv = fv.findViewById(R.id.CTL_INCOMINGLIST_BILL_LIST);
						if(lv != null) {
							((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
							SetupRecyclerListView(fv, R.id.CTL_INCOMINGLIST_BILL_LIST, R.layout.li_incominglist_bill_main);
						}
						else {
							lv = fv.findViewById(R.id.CTL_DOCUMENT_TILIST);
							if(lv != null) {
								((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
								SetupRecyclerListView(fv, R.id.CTL_DOCUMENT_TILIST, R.layout.li_incominglist_bill_ti);
							}
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && srcObj != null) {
						if(ev_subj.RvHolder == null) {

						}
						else if(srcObj instanceof SLib.RecyclerListAdapter) {
							SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
							StyloQApp app_ctx = GetAppCtx();
							//boolean do_update_goods_list_and_toggle_to_it = false;
							final int rc_id = a.GetListRcId();
							if(app_ctx != null && ev_subj.ItemIdx >= 0) {
								switch(rc_id) {
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
													CPM.SetIncomingDocument(cur_entry);
													SetupCurrentDocument(true, false);
												}
											}
										}
										break;
								}
							}
						}
					}
				}
				break;
		}
		return result;
	}
}