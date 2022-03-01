// CmdRAttendancePrereqActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;
import com.google.android.material.textfield.TextInputEditText;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class CmdRAttendancePrereqActivity extends SLib.SlActivity {
	private CommonPrereqModule CPM;
	private static class Param {
		String PrcTitle;
	}
	private enum Tab {
		tabUndef,
		tabGoodsGroups,
		tabGoods,
		tabProcessors,
		tabAttendance,
		tabOrders,
		tabSearch
	}
	private static class TabEntry {
		TabEntry(Tab id, String text, /*View*/SLib.SlFragmentStatic view)
		{
			TabId = id;
			TabText = text;
			TabView = view;
		}
		Tab TabId;
		String TabText;
		/*View*/SLib.SlFragmentStatic TabView;
	}
	private ArrayList<TabEntry> TabList;
	private Param P;
	private static class AttendanceBlock {
		AttendanceBlock()
		{
			WorkHours = new int[24];
			for(int i = 0; i < WorkHours.length; i++) {
				WorkHours[i] = i;
			}
			Days = new SLib.LDATE[14];
			Ware = null;
			Prc = null;
			SelectionDate = SLib.GetCurDate();
			AttendanceTime = null;
		}
		int [] WorkHours;
		SLib.LDATE [] Days;
		SLib.LDATE SelectionDate; // Дата, которая отображается на экране выбора времени посещения //
		SLib.LDATETIME AttendanceTime; // Выбранное время. null - не выбрано //
		CommonPrereqModule.WareEntry Ware;
		CommonPrereqModule.ProcessorEntry Prc;
	}
	private AttendanceBlock AttdcBlk;
	public CmdRAttendancePrereqActivity()
	{
		CPM = new CommonPrereqModule();
		AttdcBlk = new AttendanceBlock();
		P = new Param();
	}
	private boolean SetCurrentAttendancePrc(CommonPrereqModule.ProcessorEntry entry)
	{
		boolean result = false;
		if(entry != null) {
			if(AttdcBlk == null)
				AttdcBlk = new AttendanceBlock();
			AttdcBlk.Prc = entry;
			result = true;
		}
		return result;
	}
	private boolean SetCurrentAttendanceWare(CommonPrereqModule.WareEntry entry)
	{
		boolean result = false;
		if(entry != null) {
			if(AttdcBlk == null)
				AttdcBlk = new AttendanceBlock();
			AttdcBlk.Ware = entry;
			result = true;
		}
		return result;
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_ATTENDANCEPREREQ;
		StyloQApp app_ctx = (StyloQApp)getApplicationContext();
		if(app_ctx != null && (TabList == null || force)) {
			TabList = new ArrayList<TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			if(CPM.GoodsGroupListData != null) {
				final Tab _tab = Tab.tabGoodsGroups;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_goodsgroups, tab_layout_rcid);
				TabList.add(new TabEntry(_tab, SLib.ExpandString(app_ctx, "@{group_pl}"), f));
			}
			if(CPM.GoodsListData != null) {
				final Tab _tab = Tab.tabGoods;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_goods, tab_layout_rcid);
				TabList.add(new TabEntry(_tab, SLib.ExpandString(app_ctx, "@{ware_pl}"), f));
			}
			if(CPM.ProcessorListData != null) {
				final Tab _tab = Tab.tabProcessors;
				String title = (SLib.GetLen(P.PrcTitle) > 0) ? P.PrcTitle : SLib.ExpandString(app_ctx, "@{processor_pl}");
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_processors, tab_layout_rcid);
				TabList.add(new TabEntry(_tab, title, f));
			}
			{
				final Tab _tab = Tab.tabAttendance;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_attendance, tab_layout_rcid);
				TabList.add(new TabEntry(_tab, SLib.ExpandString(app_ctx, "@{orderdocument}"), f));
			}
			{
				final Tab _tab = Tab.tabSearch;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_searchpane, tab_layout_rcid);
				TabList.add(new TabEntry(_tab, SLib.ExpandString(app_ctx, "[search]"), f));
			}
		}
	}
	private void MakeSimpleSearchIndex()
	{
		if(CPM.SimpleSearchIndex == null)
			CPM.SimpleSearchIndex = new ArrayList<CommonPrereqModule.SimpleSearchIndexEntry>();
		else
			CPM.SimpleSearchIndex.clear();
		if(CPM.GoodsListData != null) {
			for(int i = 0; i < CPM.GoodsListData.size(); i++) {
				CommonPrereqModule.WareEntry ware_item = CPM.GoodsListData.get(i);
				if(ware_item != null && ware_item.JsItem != null) {
					int id = ware_item.JsItem.optInt("id", 0);
					if(id > 0) {
						String nm = ware_item.JsItem.optString("nm");
						if(SLib.GetLen(nm) > 0) {
							CPM.AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_NAME, nm, null);
						}
						{
							JSONArray js_code_list = ware_item.JsItem.optJSONArray("code_list");
							if(js_code_list != null && js_code_list.length() > 0) {
								for(int j = 0; j < js_code_list.length(); j++) {
									JSONObject js_code = js_code_list.optJSONObject(j);
									if(js_code != null) {
										String code = js_code.optString("cod");
										if(SLib.GetLen(code) > 0)
											CPM.AddSimpleIndexEntry(SLib.PPOBJ_GOODS, id, SLib.PPOBJATTR_CODE, code, nm);
									}
								}
							}
						}
					}
				}
			}
		}
		if(CPM.GoodsGroupListData != null) {
			for(int i = 0; i < CPM.GoodsGroupListData.size(); i++) {
				JSONObject js_item = CPM.GoodsGroupListData.get(i);
				if(js_item != null) {
					final int id = js_item.optInt("id", 0);
					if(id > 0) {
						String nm = js_item.optString("nm");
						if(SLib.GetLen(nm) > 0)
							CPM.AddSimpleIndexEntry(SLib.PPOBJ_GOODSGROUP, id, SLib.PPOBJATTR_NAME, nm, null);
					}
				}
			}
		}
		if(CPM.ProcessorListData != null) {
			for(int i = 0; i < CPM.ProcessorListData.size(); i++) {
				CommonPrereqModule.ProcessorEntry entry = CPM.ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					int id = entry.JsItem.optInt("id", 0);
					if(id > 0) {
						String nm = entry.JsItem.optString("nm");
						if(SLib.GetLen(nm) > 0)
							CPM.AddSimpleIndexEntry(SLib.PPOBJ_PROCESSOR, id, SLib.PPOBJATTR_NAME, nm, null);
					}
				}
			}
		}
	}
	private TabEntry SearchTabEntry(Tab tab)
	{
		TabEntry result = null;
		if(tab != Tab.tabUndef) {
			View v = findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
			if(v != null && v instanceof ViewPager2) {
				//ViewPager2 view_pager = (ViewPager2)v;
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					if(TabList.get(tidx).TabId == tab)
						result = TabList.get(tidx);
				}
			}
		}
		return result;
	}
	private void GotoTab(Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		if(tab != Tab.tabUndef) {
			ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
			if(view_pager != null) {
				for(int tidx = 0; tidx < TabList.size(); tidx++) {
					final TabEntry te = TabList.get(tidx);
					if(te.TabId == tab) {
						SLib.SlFragmentStatic f = te.TabView;
						if(f != null) {
							view_pager.setCurrentItem(tidx);
							if(recyclerViewToUpdate != 0) {
								View fv2 = view_pager.getChildAt(tidx);
								//f.requireView();
								View fv = f.getView();
								if(fv != null && fv instanceof ViewGroup) {
									View lv = fv.findViewById(recyclerViewToUpdate);
									if(lv != null && lv instanceof RecyclerView) {
										RecyclerView.Adapter gva = ((RecyclerView)lv).getAdapter();
										if(gva != null) {
											if(goToIndex >= 0 && goToIndex < gva.getItemCount()) {
												SetRecyclerListFocusedIndex(gva, goToIndex);
												((RecyclerView)lv).scrollToPosition(goToIndex);
											}
											gva.notifyDataSetChanged();
										}
									}
								}
							}
						}
						break;
					}
				}
			}
		}
	}
	private void SetListBackground(View iv, SLib.RecyclerListAdapter a, int itemIdxToDraw, int objType, int objID)
	{
		int shaperc = 0;
		if(GetRecyclerListFocusedIndex(a) == itemIdxToDraw)
			shaperc = R.drawable.shape_listitem_focused;
		else {
			//shaperc = IsObjInSearchResult(objType, objID) ? R.drawable.shape_listitem_found : R.drawable.shape_listitem;
			shaperc = R.drawable.shape_listitem;
		}
		iv.setBackground(getResources().getDrawable(shaperc, getTheme()));
	}
	private static class PrcByGoodsListAdapter extends ArrayAdapter {
		private int RcId;
		private int GoodsID;
		PrcByGoodsListAdapter(Context ctx, int rcId, int goodsID, ArrayList data)
		{
			super(ctx, rcId, data);
			RcId = rcId;
			GoodsID = goodsID;
		}
		int   GetGoodsID() { return GoodsID; }
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			//Context ctx = parent.getContext();
			if(item != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null) {
					Context _ctx = getContext();
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				}
				if(convertView != null) {
					TextView v = convertView.findViewById(R.id.LVITEM_GENERICNAME);
					if(v != null && item instanceof CommonPrereqModule.ProcessorEntry) {
						CommonPrereqModule.ProcessorEntry entry = (CommonPrereqModule.ProcessorEntry)item;
						if(entry.JsItem != null)
							v.setText(entry.JsItem.optString("nm", ""));
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	private static class GoodsByPrcListAdapter extends ArrayAdapter {
		private int RcId;
		private int PrcID;
		GoodsByPrcListAdapter(Context ctx, int rcId, int prcID, ArrayList data)
		{
			super(ctx, rcId, data);
			RcId = rcId;
			PrcID = prcID;
		}
		int GetPrcID() { return PrcID; }
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			//Context ctx = parent.getContext();
			if(item != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				if(convertView == null) {
					Context _ctx = getContext();
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				}
				if(convertView != null) {
					TextView v = convertView.findViewById(R.id.LVITEM_GENERICNAME);
					if(v != null && item instanceof CommonPrereqModule.WareEntry) {
						CommonPrereqModule.WareEntry ware_entry = (CommonPrereqModule.WareEntry)item;
						if(ware_entry.JsItem != null)
							v.setText(ware_entry.JsItem.optString("nm", ""));
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				Intent intent = getIntent();
				try {
					CPM.GetAttributesFromIntent(intent);
					long doc_id = intent.getLongExtra("SvcReplyDocID", 0);
					String svc_reply_doc_json = null;
					StyloQApp app_ctx = (StyloQApp)getApplication();
					StyloQDatabase db = app_ctx.GetDB();
					if(doc_id > 0) {
						StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
						if(doc_packet != null) {
							byte [] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
							if(SLib.GetLen(raw_doc) > 0)
								svc_reply_doc_json = new String(raw_doc);
						}
					}
					else
						svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
					if(SLib.GetLen(svc_reply_doc_json) > 0) {
						JSONObject js_head = new JSONObject(svc_reply_doc_json);
						{
							JSONObject js_param = js_head.optJSONObject("param");
							if(js_param != null) {
								P.PrcTitle = js_param.optString("prctitle", "");
							}
						}
						CPM.MakeGoodsGroupListFromCommonJson(js_head);
						CPM.MakeProcessorListFromCommonJson(js_head);
						CPM.MakeGoodsListFromCommonJson(js_head);
					}
					MakeSimpleSearchIndex();
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.activity_cmdrattendanceprereq);
					CPM.SetupActivityTitles(this, db);
					ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
					SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_ATTENDANCEPREREQ);
					{
						TabLayout lo_tab = findViewById(R.id.TABLAYOUT_ATTENDANCEPREREQ);
						if(lo_tab != null) {
							CreateTabList(false);
							for(int i = 0; i < TabList.size(); i++) {
								TabLayout.Tab tab = lo_tab.newTab();
								tab.setText(TabList.get(i).TabText);
								lo_tab.addTab(tab);
							}
							SLib.SetupTabLayoutStyle(lo_tab);
							SLib.SetupTabLayoutListener(lo_tab, view_pager);
						}
					}
				} catch(StyloQException | JSONException exn) {
					;//
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				if(srcObj instanceof SLib.FragmentAdapter) {
					CreateTabList(false);
					result = new Integer(TabList.size());
				}
				else if(srcObj instanceof SLib.RecyclerListAdapter) {
					SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
					switch(a.GetListRcId()) {
						case R.id.attendancePrereqGoodsListView: result = new Integer(CPM.GetGoodsListSize()); break;
						case R.id.attendancePrereqGoodsGroupListView: result = new Integer((CPM.GoodsGroupListData != null) ? CPM.GoodsGroupListData.size() : 0); break;
						case R.id.attendancePrereqProcessorListView: result = new Integer((CPM.ProcessorListData != null) ? CPM.ProcessorListData.size() : 0); break;
						case R.id.attendancePrereqAttendanceView: result = new Integer((AttdcBlk.WorkHours != null) ? AttdcBlk.WorkHours.length : 0); break;
						case R.id.searchPaneListView:
						{
							result = new Integer((CPM.SearchResult != null) ? CPM.SearchResult.GetObjTypeCount() : 0);
							//result = new Integer((SearchResult != null && SearchResult.List != null) ? SearchResult.List.size() : 0);
						}
						break;
					}
				}
				break;
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					ViewGroup vg = (ViewGroup)srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_ATTENDANCEPREREQ_ATTENDANCE) {
						if(AttdcBlk == null) {
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_WARE, "");
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_PRC, "");
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, "");
							SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, View.GONE);
						}
						else {
							if(SLib.CheckDate(AttdcBlk.SelectionDate)) {
								String ds = AttdcBlk.SelectionDate.Format(SLib.DATF_ISO8601 | SLib.DATF_CENTURY);
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, ds);
								SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, View.VISIBLE);
								SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, View.VISIBLE);
							}
							else {
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, "");
								SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, View.GONE);
								SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, View.GONE);
							}
							if(AttdcBlk.Ware != null && AttdcBlk.Ware.JsItem != null) {
								String nm = AttdcBlk.Ware.JsItem.optString("nm", "");
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_WARE, nm);
							}
							if(AttdcBlk.Prc != null && AttdcBlk.Prc.JsItem != null) {
								String nm = AttdcBlk.Prc.JsItem.optString("nm", "");
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_PRC, nm);
							}
						}
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemIdx >= 0) {
						if(ev_subj.RvHolder != null) {
							if(srcObj != null && srcObj instanceof SLib.RecyclerListAdapter) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
								if(a.GetListRcId() == R.id.searchPaneListView) {
									CPM.GetSearchPaneListViewItem((StyloQApp)getApplicationContext(), ev_subj.RvHolder.itemView, ev_subj.ItemIdx);
								}
								else if(a.GetListRcId() == R.id.attendancePrereqGoodsListView) {
									CommonPrereqModule.WareEntry cur_entry = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
									if(cur_entry != null && cur_entry.JsItem != null) {
										final int cur_id = cur_entry.JsItem.optInt("id", 0);
										View iv = ev_subj.RvHolder.itemView;
										SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.JsItem.optString("nm", ""));
										{
											double val = cur_entry.JsItem.optDouble("price", 0.0);
											SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_PRICE, (val > 0.0) ? String.format("%.2f", val) : "");
										}
										{
											double val = cur_entry.JsItem.optDouble("stock", 0.0);
											SLib.SetCtrlString(iv, R.id.ORDERPREREQ_GOODS_REST, (val > 0.0) ? String.format("%.0f", val) : "");
										}
										{
											View ctl = iv.findViewById(R.id.buttonOrder);
											if(ctl != null && ctl instanceof Button) {
												Button btn = (Button)ctl;
												double val = 0.0; //GetGoodsQttyInCurrentOrder(cur_id);
												if(val > 0.0)
													btn.setText(String.format("%.0f", val));
												else
													btn.setText("order");
											}
										}
										SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODS, cur_id);
										{
											ImageView ctl = (ImageView)iv.findViewById(R.id.ATTENDANCEPREREQ_GOODS_EXPANDSTATUS);
											if(ctl != null) {
												ListView prc_lv = (ListView)iv.findViewById(R.id.processorByWareListView);
												ArrayList <CommonPrereqModule.ProcessorEntry> prc_list = CPM.GetProcessorListByGoods(cur_id);
												if(cur_entry.PrcExpandStatus == 0 || prc_list == null) {
													ctl.setVisibility(View.GONE);
													if(prc_lv != null)
														prc_lv.setVisibility(View.GONE);
												}
												else if(cur_entry.PrcExpandStatus == 1) {
													ctl.setVisibility(View.VISIBLE);
													ctl.setImageResource(R.drawable.ic_triangleleft03);
													if(prc_lv != null)
														prc_lv.setVisibility(View.GONE);
												}
												else if(cur_entry.PrcExpandStatus == 2) {
													ctl.setVisibility(View.VISIBLE);
													ctl.setImageResource(R.drawable.ic_triangledown03);
													if(prc_lv != null) {
														prc_lv.setVisibility(View.VISIBLE);
														PrcByGoodsListAdapter adapter = new PrcByGoodsListAdapter(iv.getContext(), R.layout.simple_list_item, cur_id, prc_list);
														prc_lv.setAdapter(adapter);
														{
															int total_items_height = SLib.CalcListViewHeight(prc_lv);
															if(total_items_height > 0) {
																ViewGroup.LayoutParams params = prc_lv.getLayoutParams();
																params.height = total_items_height;
																prc_lv.setLayoutParams(params);
																prc_lv.requestLayout();
															}
														}
														adapter.setNotifyOnChange(true);
														prc_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
															@Override public void onItemClick(AdapterView<?> parent, View view, int position, long id)
															{
																Object item = (Object)parent.getItemAtPosition(position);
																Context ctx = parent.getContext();
																if(item != null && ctx != null && ctx instanceof SLib.SlActivity) {
																	SLib.SlActivity activity = (SLib.SlActivity)ctx;
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
												// ***
											}
										}
									}
								}
								else if(a.GetListRcId() == R.id.attendancePrereqGoodsGroupListView) {
									if(CPM.GoodsGroupListData != null && ev_subj.ItemIdx < CPM.GoodsGroupListData.size()) {
										View iv = ev_subj.RvHolder.itemView;
										JSONObject cur_entry = (JSONObject)CPM.GoodsGroupListData.get(ev_subj.ItemIdx);
										if(cur_entry != null) {
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.optString("nm", ""));
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODSGROUP, cur_entry.optInt("id", 0));
										}
									}
								}
								else if(a.GetListRcId() == R.id.attendancePrereqAttendanceView) {
									if(AttdcBlk.WorkHours != null && ev_subj.ItemIdx < AttdcBlk.WorkHours.length) {
										View iv = ev_subj.RvHolder.itemView;
										SLib.SetCtrlString(iv, R.id.LVITEM_HOUR, String.format("%02d", AttdcBlk.WorkHours[ev_subj.ItemIdx]));
									}
								}
								else if(a.GetListRcId() == R.id.attendancePrereqProcessorListView) {
									if(CPM.ProcessorListData != null && ev_subj.ItemIdx < CPM.ProcessorListData.size()) {
										View iv = ev_subj.RvHolder.itemView;
										CommonPrereqModule.ProcessorEntry cur_entry = null;
										cur_entry = CPM.ProcessorListData.get(ev_subj.ItemIdx);
										if(cur_entry != null && cur_entry.JsItem != null) {
											final int cur_id = cur_entry.JsItem.optInt("id", 0);
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.JsItem.optString("nm", ""));
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_PROCESSOR, cur_id);
											{
												ImageView ctl = (ImageView)iv.findViewById(R.id.ATTENDANCEPREREQ_PRC_EXPANDSTATUS);
												if(ctl != null) {
													ListView goods_lv = (ListView)iv.findViewById(R.id.goodsByPrcListView);
													ArrayList <CommonPrereqModule.WareEntry> goods_list = CPM.GetGoodsListByPrc(cur_id);
													if(cur_entry.GoodsExpandStatus == 0 || goods_list == null) {
														ctl.setVisibility(View.GONE);
														if(goods_lv != null)
															goods_lv.setVisibility(View.GONE);
													}
													else if(cur_entry.GoodsExpandStatus == 1) {
														ctl.setVisibility(View.VISIBLE);
														ctl.setImageResource(R.drawable.ic_triangleleft03);
														if(goods_lv != null)
															goods_lv.setVisibility(View.GONE);
													}
													else if(cur_entry.GoodsExpandStatus == 2) {
														ctl.setVisibility(View.VISIBLE);
														ctl.setImageResource(R.drawable.ic_triangledown03);
														if(goods_lv != null) {
															goods_lv.setVisibility(View.VISIBLE);
															GoodsByPrcListAdapter adapter = new GoodsByPrcListAdapter(iv.getContext(), R.layout.simple_list_item, cur_id, goods_list);
															goods_lv.setAdapter(adapter);
															{
																int total_items_height = SLib.CalcListViewHeight(goods_lv);
																if(total_items_height > 0) {
																	ViewGroup.LayoutParams params = goods_lv.getLayoutParams();
																	params.height = total_items_height;
																	goods_lv.setLayoutParams(params);
																	goods_lv.requestLayout();
																}
															}
															adapter.setNotifyOnChange(true);
															goods_lv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
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
													// ***
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
			case SLib.EV_CREATEFRAGMENT:
				if(subj instanceof Integer) {
					int item_idx = (Integer)subj;
					if(TabList != null && item_idx >= 0 && item_idx < TabList.size()) {
						TabEntry cur_entry = (TabEntry)TabList.get(item_idx);
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
						SLib.SlFragmentStatic fragment = (SLib.SlFragmentStatic)srcObj;
						View fv = (View)subj;
						View lv = fv.findViewById(R.id.attendancePrereqGoodsListView);
						if(lv != null) {
							((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
							SetupRecyclerListView(fv, R.id.attendancePrereqGoodsListView, R.layout.attendanceprereq_goodslist_item);
							if(selected_search_objtype == SLib.PPOBJ_GOODS) {
								final int foc_idx = CPM.FindGoodsItemIndexByID(selected_search_objid);
								SetRecyclerListFocusedIndex(((RecyclerView) lv).getAdapter(), foc_idx);
								SLib.RequestRecyclerListViewPosition((RecyclerView) lv, foc_idx);
								CPM.SearchResult.ResetSelectedItemIndex();
							}
						}
						else {
							lv = fv.findViewById(R.id.attendancePrereqGoodsGroupListView);
							if(lv != null) {
								((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
								SetupRecyclerListView(fv, R.id.attendancePrereqGoodsGroupListView, R.layout.simple_list_item);
								if(selected_search_objtype == SLib.PPOBJ_GOODSGROUP) {
									final int foc_idx = CPM.FindGoodsGroupItemIndexByID(selected_search_objid);
									SetRecyclerListFocusedIndex(((RecyclerView) lv).getAdapter(), foc_idx);
									SLib.RequestRecyclerListViewPosition((RecyclerView) lv, foc_idx);
									CPM.SearchResult.ResetSelectedItemIndex();
								}
							}
							else {
								lv = fv.findViewById(R.id.attendancePrereqProcessorListView);
								if(lv != null) {
									((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
									SetupRecyclerListView(fv, R.id.attendancePrereqProcessorListView, R.layout.attendanceprereq_processorlist_item);
									if(selected_search_objtype == SLib.PPOBJ_PROCESSOR) {
										final int foc_idx = CPM.FindGoodsGroupItemIndexByID(selected_search_objid);
										SetRecyclerListFocusedIndex(((RecyclerView) lv).getAdapter(), foc_idx);
										SLib.RequestRecyclerListViewPosition((RecyclerView) lv, foc_idx);
										CPM.SearchResult.ResetSelectedItemIndex();
									}
								}
								else {
									lv = fv.findViewById(R.id.attendancePrereqAttendanceView);
									if(lv != null) {
										((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
										SetupRecyclerListView(fv, R.id.attendancePrereqAttendanceView, R.layout.attendanceprereq_attendance_item);
										//
										{
											View btn = fv.findViewById(R.id.CTL_PREV);
											btn.setOnClickListener(new View.OnClickListener()
												{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }});
										}
										{
											View btn = fv.findViewById(R.id.CTL_NEXT);
											btn.setOnClickListener(new View.OnClickListener()
												{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }});
										}
									}
									else {
										lv = fv.findViewById(R.id.searchPaneListView);
										if(lv != null) {
											((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
											SetupRecyclerListView(fv, R.id.searchPaneListView, R.layout.searchpane_result_item);
											{
												View iv = fv.findViewById(R.id.CTL_SEARCHPANE_INPUT);
												if(iv != null && iv instanceof TextInputEditText) {
													TextInputEditText tiv = (TextInputEditText) iv;
													tiv.requestFocus();
													tiv.addTextChangedListener(new TextWatcher() {
														public void afterTextChanged(Editable s)
														{
															//int cross_icon_id = (s.length() > 0) ? R.drawable.ic_cross01 : 0;
															//tiv.setCompoundDrawablesWithIntrinsicBounds(0, 0, cross_icon_id, 0);
														}
														public void beforeTextChanged(CharSequence s, int start, int count, int after)
														{
														}
														public void onTextChanged(CharSequence s, int start, int before, int count)
														{
															String pattern = s.toString();
															if(SLib.GetLen(pattern) > 3)
																CPM.SearchInSimpleIndex(pattern);
															else if(CPM.SearchResult != null)
																CPM.SearchResult.Clear();
															View lv = findViewById(R.id.searchPaneListView);
															if(lv != null && lv instanceof RecyclerView) {
																RecyclerView.Adapter gva = ((RecyclerView) lv).getAdapter();
																if(gva != null)
																	gva.notifyDataSetChanged();
															}
														}
													});
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
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.buttonOrder);
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.ATTENDANCEPREREQ_GOODS_EXPANDSTATUS);
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.ATTENDANCEPREREQ_PRC_EXPANDSTATUS);
						result = ev_subj.RvHolder;
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && srcObj != null) {
						if(ev_subj.RvHolder == null) {
							if(srcObj instanceof ListView) {
								ListView lv = (ListView)srcObj;
								if(lv.getId() == R.id.searchPaneTerminalListView) {
									if(ev_subj.ItemObj != null && ev_subj.ItemObj instanceof CommonPrereqModule.SimpleSearchIndexEntry) {
										CommonPrereqModule.SimpleSearchIndexEntry se = (CommonPrereqModule.SimpleSearchIndexEntry)ev_subj.ItemObj;
										// ! ev_subj.ItemIdx не согласуется простым образом с ev_subj.ItemObj из-за
										// двухярусной структуры списка.
										CPM.SearchResult.SetSelectedItemIndex(CPM.SearchResult.FindIndexOfItem(se));
										if(se.ObjType == SLib.PPOBJ_GOODS) {
											int _idx = CPM.FindGoodsItemIndexByID(se.ObjID);
											GotoTab(Tab.tabGoods, R.id.attendancePrereqGoodsListView, _idx, -1);
										}
										/*else if(se.ObjType == SLib.PPOBJ_PERSON) {
											int _idx = FindClientItemIndexByID(se.ObjID);
											GotoTab(Tab.tabClients, R.id.attendancePrereqClientsListView, _idx, -1);
										}*/
										/*else if(se.ObjType == SLib.PPOBJ_LOCATION) {
											JSONObject cli_js_obj = FindClientEntryByDlvrLocID(se.ObjID);
											if(cli_js_obj != null) {
												int cli_id = cli_js_obj.optInt("id", 0);
												if(cli_id > 0) {
													int _idx = FindClientItemIndexByID(cli_id);
													int _dlvr_loc_idx = FindDlvrLocEntryIndexInCliEntry(cli_js_obj, se.ObjID);
													GotoTab(Tab.tabClients, R.id.attendancePrereqClientsListView, _idx, _dlvr_loc_idx);
												}
											}
											//tab_to_select = Tab.tabClients;
										}*/
										else if(se.ObjType == SLib.PPOBJ_GOODSGROUP) {
											int _idx = CPM.FindGoodsGroupItemIndexByID(se.ObjID);
											GotoTab(Tab.tabGoodsGroups, R.id.attendancePrereqGoodsGroupListView, _idx, -1);
										}
										else if(se.ObjType == SLib.PPOBJ_PROCESSOR) {
											int _idx = CPM.FindProcessorItemIndexByID(se.ObjID);
											GotoTab(Tab.tabProcessors, R.id.attendancePrereqProcessorListView, _idx, -1);
										}
									}
								}
								else if(lv.getId() == R.id.processorByWareListView) {
									Adapter a = lv.getAdapter();
									if(a instanceof PrcByGoodsListAdapter) {
										int goods_id = ((PrcByGoodsListAdapter)a).GetGoodsID();
										if(goods_id > 0 && ev_subj.ItemObj != null && ev_subj.ItemObj instanceof CommonPrereqModule.ProcessorEntry) {
											CommonPrereqModule.WareEntry ware_entry = CPM.FindGoodsItemByGoodsID(goods_id);
											if(ware_entry != null) {
												SetCurrentAttendanceWare(ware_entry);
												SetCurrentAttendancePrc((CommonPrereqModule.ProcessorEntry) ev_subj.ItemObj);
												GotoTab(Tab.tabAttendance, 0, -1, -1);
											}
										}
									}
								}
								else if(lv.getId() == R.id.goodsByPrcListView) {
									Adapter a = lv.getAdapter();
									if(a instanceof GoodsByPrcListAdapter) {
										int prc_id = ((GoodsByPrcListAdapter)a).GetPrcID();
										if(prc_id > 0 && ev_subj.ItemObj != null && ev_subj.ItemObj instanceof CommonPrereqModule.WareEntry) {
											CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(prc_id);
											if(prc_entry != null) {
												SetCurrentAttendanceWare((CommonPrereqModule.WareEntry) ev_subj.ItemObj);
												SetCurrentAttendancePrc(prc_entry);
												GotoTab(Tab.tabAttendance, 0, -1, -1);
											}
										}
									}
								}
							}
						}
						else if(srcObj instanceof SLib.RecyclerListAdapter) {
							SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
							StyloQApp app_ctx = (StyloQApp)getApplication();
							boolean do_update_goods_list_and_toggle_to_it = false;
							if(a.GetListRcId() == R.id.attendancePrereqGoodsListView) {
								if(app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GetGoodsListSize()) {
									CommonPrereqModule.WareEntry item = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
									if(item != null && ev_subj.ItemView != null) {
										if(ev_subj.ItemView.getId() == R.id.ATTENDANCEPREREQ_GOODS_EXPANDSTATUS) {
											// change expand status
											if(item.PrcExpandStatus == 1) {
												item.PrcExpandStatus = 2;
												a.notifyItemChanged(ev_subj.ItemIdx);
											}
											else if(item.PrcExpandStatus == 2) {
												item.PrcExpandStatus = 1;
												a.notifyItemChanged(ev_subj.ItemIdx);
											}
										}
										else if(ev_subj.ItemView.getId() == R.id.buttonOrder) {
											// select for order
										}
										else {
											SetCurrentAttendanceWare(item);
											GotoTab(Tab.tabAttendance, 0, -1, -1);
										}
									}
								}
							}
							else if(a.GetListRcId() == R.id.attendancePrereqProcessorListView) {
								if(app_ctx != null && CPM.ProcessorListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.ProcessorListData.size()) {
									CommonPrereqModule.ProcessorEntry item = CPM.ProcessorListData.get(ev_subj.ItemIdx);
									if(item != null && ev_subj.ItemView != null) {
										if(ev_subj.ItemView.getId() == R.id.ATTENDANCEPREREQ_PRC_EXPANDSTATUS) {
											// change expand status
											if(item.GoodsExpandStatus == 1) {
												item.GoodsExpandStatus = 2;
												a.notifyItemChanged(ev_subj.ItemIdx);
											}
											else if(item.GoodsExpandStatus == 2) {
												item.GoodsExpandStatus = 1;
												a.notifyItemChanged(ev_subj.ItemIdx);
											}
										}
										else if(ev_subj.ItemView.getId() == R.id.buttonOrder) {
											// select for order
										}
										else {
											SetCurrentAttendancePrc(item);
											GotoTab(Tab.tabAttendance, 0, -1, -1);
										}
									}
								}
							}
							else if(a.GetListRcId() == R.id.attendancePrereqGoodsGroupListView) {
								if(app_ctx != null && CPM.GoodsGroupListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GoodsGroupListData.size()) {
									final int group_id = CPM.GoodsGroupListData.get(ev_subj.ItemIdx).optInt("id", 0);
									if(CPM.SetGoodsFilterByGroup(group_id))
										do_update_goods_list_and_toggle_to_it = true;
									//app_ctx.RunSvcCommand(SvcIdent, ListData.Items.get(ev_subj.ItemIdx));
								}
							}
							if(do_update_goods_list_and_toggle_to_it) {
								GotoTab(Tab.tabGoods, R.id.attendancePrereqGoodsListView, -1, -1);
							}
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				int view_id = (srcObj != null && srcObj instanceof View) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.tbButtonSearch) {
					GotoTab(Tab.tabSearch, 0, -1, -1);
				}
				else if(view_id == R.id.CTL_PREV) {
					if(AttdcBlk != null && SLib.CheckDate(AttdcBlk.SelectionDate)) {
						SLib.LDATE now_date = SLib.GetCurDate();
						if(SLib.LDATE.Difference(now_date, AttdcBlk.SelectionDate) < 0) {
							AttdcBlk.SelectionDate = SLib.LDATE.Plus(AttdcBlk.SelectionDate, -1);
							TabEntry te = SearchTabEntry(Tab.tabAttendance);
							if(te != null && te.TabView != null)
								HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
						}
					}
				}
				else if(view_id == R.id.CTL_NEXT) {
					if(AttdcBlk != null && SLib.CheckDate(AttdcBlk.SelectionDate)) {
						AttdcBlk.SelectionDate = SLib.LDATE.Plus(AttdcBlk.SelectionDate, +1);
						TabEntry te = SearchTabEntry(Tab.tabAttendance);
						if(te != null && te.TabView != null)
							HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				break;
		}
		return result;
	}
}
