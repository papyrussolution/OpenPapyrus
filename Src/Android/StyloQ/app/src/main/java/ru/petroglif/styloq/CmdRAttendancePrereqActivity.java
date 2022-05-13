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
import java.util.Locale;

public class CmdRAttendancePrereqActivity extends SLib.SlActivity {
	private CommonPrereqModule CPM;
	private AttendanceBlock AttdcBlk;
	private Param P;

	private static class Param {
		Param()
		{
			PrcTitle = null;
			MaxScheduleDays = 7;
		}
		String PrcTitle;
		int MaxScheduleDays;
	}
	private static class AttendanceBlock {
		int [] WorkHours;
		private SLib.LDATE SelectionDate_; // Дата, которая отображается на экране выбора времени посещения //
		SLib.LDATETIME AttendanceTime; // Выбранное время. null - не выбрано //
		CommonPrereqModule.WareEntry Ware;
		CommonPrereqModule.ProcessorEntry Prc;
		ArrayList <SLib.STimeChunk> BusyList; // Зависит от Prc
		ArrayList <SLib.STimeChunk> WorktimeList; // Зависит от Prc
		ArrayList <SLib.STimeChunk> CurrentBookingBusyList;
		int Duration; // Ожидаемая продолжительность сессии (prcID, goodsID). assert(goodsID || Duration == 0)
		//
		AttendanceBlock(final Param param)
		{
			Ware = null;
			Prc = null;
			BusyList = null;
			WorktimeList = null;
			CurrentBookingBusyList = null;
			AttendanceTime = null;
			Duration = 0;
			SetSelectionDate(SLib.GetCurDate());
		}
		boolean IncrementSelectedDate(final Param param)
		{
			boolean result = false;
			final SLib.LDATE sel_date = GetSelectionDate();
			if(SLib.CheckDate(sel_date)) {
				final SLib.LDATE now_date = SLib.GetCurDate();
				SLib.LDATE _d = null;
				int increment = 0;
				while(_d == null) {
					increment++;
					_d = SLib.LDATE.Plus(sel_date, increment);
					if(SLib.LDATE.Difference(_d, now_date) > param.MaxScheduleDays) {
						_d = null;
						break;
					}
					else if(!IsDateInWorktime(_d))
						_d = null;
				}
				if(SLib.CheckDate(_d)) {
					SetSelectionDate(_d);
					result = true;
				}
			}
			return result;
		}
		boolean DecrementSelectedDate()
		{
			boolean result = false;
			final SLib.LDATE sel_date = GetSelectionDate();
			if(SLib.CheckDate(sel_date)) {
				SLib.LDATE now_date = SLib.GetCurDate();
				SLib.LDATE _d = null;
				int increment = 0;
				while(_d == null) {
					increment--;
					_d = SLib.LDATE.Plus(sel_date, increment);
					if(SLib.LDATE.Difference(_d, now_date) < 0) {
						_d = null;
						break;
					}
					else if(!IsDateInWorktime(_d))
						_d = null;
				}
				if(SLib.CheckDate(_d)) {
					SetSelectionDate(_d);
					result = true;
				}
			}
			return result;
		}
		SLib.LDATE GetSelectionDate()
		{
			return SelectionDate_;
		}
		boolean IsDateInWorktime(SLib.LDATE dt)
		{
			boolean result = false;
			if(SLib.CheckDate(dt)) {
				if(WorktimeList != null) {
					if(WorktimeList.size() > 0) {
						SLib.STimeChunk hc = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(0, 0, 0, 0)),
								new SLib.LDATETIME(dt, new SLib.LTIME(23, 59, 59, 0)));
						boolean is_intersection = false;
						for(int i = 0; !is_intersection && i < WorktimeList.size(); i++) {
							SLib.STimeChunk wti = WorktimeList.get(i);
							if(wti != null && wti.Intersect(hc) != null)
								is_intersection = true;
						}
						if(is_intersection)
							result = true;
					}
				}
				else
					result = true;
			}
			return result;
		}
		boolean IsHourInWorktime(SLib.LDATE dt, int hour)
		{
			boolean result = false;
			if(SLib.CheckDate(dt) && hour >= 0 && hour < 24) {
				if(WorktimeList != null) {
					if(WorktimeList.size() > 0) {
						SLib.STimeChunk hc = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, 0, 0, 0)),
							new SLib.LDATETIME(dt, new SLib.LTIME(hour, 59, 59, 0)));
						boolean is_intersection = false;
						for(int i = 0; !is_intersection && i < WorktimeList.size(); i++) {
							SLib.STimeChunk wti = WorktimeList.get(i);
							if(wti != null && wti.Intersect(hc) != null)
								is_intersection = true;
						}
						if(is_intersection)
							result = true;
					}
				}
				else
					result = true;
			}
			return result;
		}
		void SetSelectionDate(SLib.LDATE sd)
		{
			if(SLib.CheckDate(sd))
				SelectionDate_ = sd;
			else
				SelectionDate_ = SLib.GetCurDate();
			int workhours_count = 0;
			int [] temp_work_hours = new int[24];
			if(WorktimeList != null) {
				for(int hour = 0; hour < 24; hour++) {
					if(IsHourInWorktime(SelectionDate_, hour)) {
						temp_work_hours[workhours_count] = hour;
						workhours_count++;
					}
				}
			}
			else {
				for(int hour = 0; hour < 24; hour++) {
					temp_work_hours[workhours_count] = hour;
					workhours_count++;
				}
			}
			{
				WorkHours = new int[workhours_count];
				for(int i = 0; i < workhours_count; i++)
					WorkHours[i] = temp_work_hours[i];
			}
		}
		public int GetGoodsID()
		{
			return (Ware != null && Ware.JsItem != null) ? Ware.JsItem.optInt("id", 0) : 0;
		}
		public int GetPrcID()
		{
			return (Prc != null && Prc.JsItem != null) ? Prc.JsItem.optInt("id", 0) : 0;
		}
		int GetWorkhoursCount()
		{
			return (WorkHours != null) ? WorkHours.length : 0;
		}
	}
	public CmdRAttendancePrereqActivity()
	{
		CPM = new CommonPrereqModule(this);
		P = new Param();
		AttdcBlk = new AttendanceBlock(P);
	}
	private void NotifyTabContentChanged(CommonPrereqModule.Tab tabId, int innerViewId)
	{
		ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
		if(view_pager != null && CPM.TabList != null) {
			for(int tidx = 0; tidx < CPM.TabList.size(); tidx++) {
				if(CPM.TabList.get(tidx).TabId == tabId) {
					SLib.SlFragmentStatic f = CPM.TabList.get(tidx).TabView;
					if(f != null) {
						View fv = f.getView();
						if(innerViewId != 0 && fv != null && fv instanceof ViewGroup) {
							View lv = fv.findViewById(innerViewId);
							if(lv != null && lv instanceof RecyclerView) {
								RecyclerView.Adapter gva = ((RecyclerView)lv).getAdapter();
								if(gva != null)
									gva.notifyDataSetChanged();
							}
						}
					}
					break;
				}
			}
		}
	}
	private void UpdateAttendanceView()
	{
		CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
		if(te != null && te.TabView != null) {
			View v = te.TabView.getView();
			if(v != null && v instanceof ViewGroup) {
				View lv = ((ViewGroup)v).findViewById(R.id.attendancePrereqAttendanceView);
				if(lv != null && lv instanceof RecyclerView) {
					RecyclerView.Adapter gva = ((RecyclerView)lv).getAdapter();
					if(gva != null)
						gva.notifyDataSetChanged();
				}
			}
		}
	}
	private void UpdateBookingView()
	{
		CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
		if(te != null && te.TabView != null) {
			View v = te.TabView.getView();
			if(v != null && v instanceof ViewGroup) {
				View lv = ((ViewGroup)v).findViewById(R.id.attendancePrereqBookingListView);
				if(lv != null && lv instanceof RecyclerView) {
					RecyclerView.Adapter gva = ((RecyclerView)lv).getAdapter();
					if(gva != null)
						gva.notifyDataSetChanged();
				}
			}
		}
	}
	private void SetCurrentAttendancePrc(CommonPrereqModule.ProcessorEntry entry)
	{
		if(entry != null) {
			if(AttdcBlk == null)
				AttdcBlk = new AttendanceBlock(P);
			AttdcBlk.Prc = entry;
			int prc_id = AttdcBlk.GetPrcID();
			if(prc_id > 0) {
				AttdcBlk.BusyList = null;
				AttdcBlk.WorktimeList = null;
				JSONArray js_busy_list = AttdcBlk.Prc.JsItem.optJSONArray("busy_list");
				if(js_busy_list != null && js_busy_list.length() > 0) {
					for(int i = 0; i < js_busy_list.length(); i++) {
						JSONObject js_item = js_busy_list.optJSONObject(i);
						if(js_item != null) {
							String low = js_item.optString("low", null);
							String upp = js_item.optString("upp", null);
							if(SLib.GetLen(low) > 0 && SLib.GetLen(upp) > 0) {
								SLib.STimeChunk tc = new SLib.STimeChunk();
								tc.Start = SLib.strtodatetime(low, SLib.DATF_ISO8601, SLib.TIMF_HMS);
								tc.Finish = SLib.strtodatetime(upp, SLib.DATF_ISO8601, SLib.TIMF_HMS);
								if(tc.Start != null && tc.Finish != null) {
									if(AttdcBlk.BusyList == null)
										AttdcBlk.BusyList = new ArrayList<SLib.STimeChunk>();
									AttdcBlk.BusyList.add(tc);
								}
							}
						}
					}
				}
				//
				JSONArray js_worktime_list = AttdcBlk.Prc.JsItem.optJSONArray("worktime");
				if(js_worktime_list != null && js_worktime_list.length() > 0) {
					for(int i = 0; i < js_worktime_list.length(); i++) {
						JSONObject js_item = js_worktime_list.optJSONObject(i);
						if(js_item != null) {
							String low = js_item.optString("low", null);
							String upp = js_item.optString("upp", null);
							if(SLib.GetLen(low) > 0 && SLib.GetLen(upp) > 0) {
								SLib.STimeChunk tc = new SLib.STimeChunk();
								tc.Start = SLib.strtodatetime(low, SLib.DATF_ISO8601, SLib.TIMF_HMS);
								tc.Finish = SLib.strtodatetime(upp, SLib.DATF_ISO8601, SLib.TIMF_HMS);
								if(tc.Start != null && tc.Finish != null) {
									if(AttdcBlk.WorktimeList == null)
										AttdcBlk.WorktimeList = new ArrayList<SLib.STimeChunk>();
									AttdcBlk.WorktimeList.add(tc);
								}
							}
						}
					}
				}
				AttdcBlk.SetSelectionDate(AttdcBlk.GetSelectionDate()); // Пересчитывам календарь в соответствии с процессором
				AttdcBlk.CurrentBookingBusyList = CPM.GetCurrentDocumentBusyList(prc_id);
			}
			else {
				AttdcBlk.CurrentBookingBusyList = null;
			}
			AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
		}
		else if(AttdcBlk != null) {
			AttdcBlk.Prc = null;
			AttdcBlk.BusyList = null;
			AttdcBlk.CurrentBookingBusyList = null;
			AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
		}
		UpdateAttendanceView();
	}
	private void SetCurrentAttendanceWare(CommonPrereqModule.WareEntry entry)
	{
		if(entry != null) {
			if(AttdcBlk == null)
				AttdcBlk = new AttendanceBlock(P);
			AttdcBlk.Ware = entry;
		}
		else if(AttdcBlk != null) {
			AttdcBlk.Ware = null;
		}
		if(AttdcBlk != null)
			AttdcBlk.Duration = CPM.GetServiceDurationForPrc(AttdcBlk.GetPrcID(), AttdcBlk.GetGoodsID());
		UpdateAttendanceView();
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_ATTENDANCEPREREQ;
		StyloQApp app_ctx = (StyloQApp)getApplicationContext();
		if(app_ctx != null && (CPM.TabList == null || force)) {
			CPM.TabList = new ArrayList<CommonPrereqModule.TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			if(CPM.GoodsGroupListData != null) {
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabGoodsGroups;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_goodsgroups, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "@{group_pl}"), f));
			}
			if(CPM.GoodsListData != null) {
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabGoods;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_goods, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "@{ware_pl}"), f));
			}
			if(CPM.ProcessorListData != null) {
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabProcessors;
				String title = (SLib.GetLen(P.PrcTitle) > 0) ? P.PrcTitle : SLib.ExpandString(app_ctx, "@{processor_pl}");
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_processors, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, title, f));
			}
			{
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabAttendance;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_attendance, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "@{booking}"), f));
			}
			{
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabBookingDocument;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_booking, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "@{orderdocument}"), f));
			}
			{
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabOrders;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_attendanceprereq_orders, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "@{booking_pl}"), f));
			}
			{
				final CommonPrereqModule.Tab _tab = CommonPrereqModule.Tab.tabSearch;
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_tab.ordinal(), R.layout.layout_searchpane, tab_layout_rcid);
				CPM.TabList.add(new CommonPrereqModule.TabEntry(_tab, SLib.ExpandString(app_ctx, "[search]"), f));
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
	private CommonPrereqModule.TabEntry SearchTabEntry(CommonPrereqModule.Tab tab)
	{
		CommonPrereqModule.TabEntry result = null;
		if(tab != CommonPrereqModule.Tab.tabUndef) {
			View v = findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
			if(v != null && v instanceof ViewPager2) {
				for(int tidx = 0; tidx < CPM.TabList.size(); tidx++) {
					if(CPM.TabList.get(tidx).TabId == tab)
						result = CPM.TabList.get(tidx);
				}
			}
		}
		return result;
	}
	private void GotoTab(CommonPrereqModule.Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		if(tab != CommonPrereqModule.Tab.tabUndef) {
			ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
			if(view_pager != null) {
				for(int tidx = 0; tidx < CPM.TabList.size(); tidx++) {
					final CommonPrereqModule.TabEntry te = CPM.TabList.get(tidx);
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
				Context _ctx = getContext();
				if(_ctx != null) {
					if(convertView == null) {
						convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
					}
					if(convertView != null) {
						TextView v = convertView.findViewById(R.id.LVITEM_GENERICNAME);
						if(v != null && item instanceof CommonPrereqModule.WareEntry) {
							CommonPrereqModule.WareEntry ware_entry = (CommonPrereqModule.WareEntry) item;
							if(ware_entry.JsItem != null) {
								int goods_id = ware_entry.JsItem.optInt("id", 0);
								v.setText(ware_entry.JsItem.optString("nm", ""));
								if(_ctx instanceof CmdRAttendancePrereqActivity) {
									int duration = ((CmdRAttendancePrereqActivity) _ctx).CPM.GetServiceDurationForPrc(PrcID, goods_id);
									if(duration > 0) {
										Context app_ctx = _ctx.getApplicationContext();
										if(app_ctx != null && app_ctx instanceof StyloQApp) {
											String text = ((StyloQApp) app_ctx).GetString("duration");
											if(SLib.GetLen(text) == 0)
												text = SLib.durationfmt(duration);
											else
												text += ": " + SLib.durationfmt(duration);
											SLib.SetCtrlString(convertView, R.id.CTL_GOODSBYPRC_DURATION, text);
										}
									}
								}
							}
						}
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
								P.MaxScheduleDays = js_param.optInt("MaxScheduleDays", 7);
								if(P.MaxScheduleDays < 1 || P.MaxScheduleDays > 365)
									P.MaxScheduleDays = 7;
							}
						}
						CPM.MakeGoodsGroupListFromCommonJson(js_head);
						CPM.MakeProcessorListFromCommonJson(js_head);
						CPM.MakeGoodsListFromCommonJson(js_head);
					}
					CPM.MakeCurrentDocList(app_ctx);
					MakeSimpleSearchIndex();
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.activity_cmdrattendanceprereq);
					CPM.SetupActivity(db, R.id.VIEWPAGER_ATTENDANCEPREREQ, R.id.TABLAYOUT_ATTENDANCEPREREQ);
					ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
					SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_ATTENDANCEPREREQ);
					{
						TabLayout lo_tab = findViewById(R.id.TABLAYOUT_ATTENDANCEPREREQ);
						if(lo_tab != null) {
							CreateTabList(false);
							for(int i = 0; i < CPM.TabList.size(); i++) {
								TabLayout.Tab tab = lo_tab.newTab();
								tab.setText(CPM.TabList.get(i).TabText);
								lo_tab.addTab(tab);
							}
							SLib.SetupTabLayoutStyle(lo_tab);
							SLib.SetupTabLayoutListener(lo_tab, view_pager);
						}
					}
					SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
				} catch(StyloQException | JSONException exn) {
					;//
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
						case R.id.attendancePrereqGoodsListView: result = new Integer(CPM.GetGoodsListSize()); break;
						case R.id.attendancePrereqGoodsGroupListView: result = new Integer((CPM.GoodsGroupListData != null) ? CPM.GoodsGroupListData.size() : 0); break;
						case R.id.attendancePrereqProcessorListView: result = new Integer((CPM.ProcessorListData != null) ? CPM.ProcessorListData.size() : 0); break;
						case R.id.attendancePrereqAttendanceView:
							result = new Integer(AttdcBlk.GetWorkhoursCount());
							break;
						case R.id.attendancePrereqBookingListView: result = new Integer((CPM.CurrentOrder != null && CPM.CurrentOrder.BkList != null) ? CPM.CurrentOrder.BkList.size() : 0); break;
						case R.id.attendancePrereqOrderListView: result = new Integer((CPM.OrderHList != null) ? CPM.OrderHList.size() : 0); break;
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
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DURATION, "");
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_PRICE, "");
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, "");
							SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, View.GONE);
						}
						else {
							if(SLib.CheckDate(AttdcBlk.GetSelectionDate())) {
								String ds = AttdcBlk.GetSelectionDate().Format(SLib.DATF_ISO8601 | SLib.DATF_CENTURY);
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
							{
								int duration = AttdcBlk.Duration;
								String text = "";
								if(duration > 0) {
									StyloQApp app_ctx = (StyloQApp) getApplication();
									text = app_ctx.GetString("duration");
									if(SLib.GetLen(text) == 0)
										text = SLib.durationfmt(duration);
									else
										text += ": " + SLib.durationfmt(duration);
								}
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DURATION, text);
							}
						}
					}
					else if(vg_id == R.id.LAYOUT_ATTENDANCEPREREQ_BOOKING) {
						if(CPM.CurrentOrder == null || CPM.CurrentOrder.H == null) {
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_PRC, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_TIMECHUNK, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_AMOUNT, "");
						}
						else {
							if(SLib.GetLen(CPM.CurrentOrder.H.Code) > 0)
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, CPM.CurrentOrder.H.Code);
							SLib.LDATE d = null;
							if(CPM.CurrentOrder.H.Time > 0)
								d = SLib.BuildDateByEpoch(CPM.CurrentOrder.H.Time);
							else if(CPM.CurrentOrder.H.CreationTime > 0)
								d = SLib.BuildDateByEpoch(CPM.CurrentOrder.H.CreationTime);
							if(d != null) {
								String ds = d.Format(SLib.DATF_ISO8601|SLib.DATF_CENTURY);
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, ds);
							}
							if(CPM.CurrentOrder.BkList != null && CPM.CurrentOrder.BkList.size() > 0) {
								String goods_name = "";
								String prc_name = "";
								String timechunk_text = "";
								Document.BookingItem bk_item = CPM.CurrentOrder.BkList.get(0);
								if(bk_item != null) {
									CommonPrereqModule.WareEntry goods_entry = CPM.FindGoodsItemByGoodsID(bk_item.GoodsID);
									if(goods_entry != null)
										goods_name = goods_entry.JsItem.optString("nm", "");
									CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(bk_item.PrcID);
									if(prc_entry != null)
										prc_name = prc_entry.JsItem.optString("nm", "");
									if(bk_item.ReqTime != null) {
										//bk_item.ReqTime.
										timechunk_text = SLib.datetimefmt(bk_item.ReqTime, SLib.DATF_ISO8601, SLib.TIMF_HMS);
										if(bk_item.EstimatedDurationSec > 0) {
											SLib.LDATETIME finish_dtm = SLib.plusdatetimesec(bk_item.ReqTime, bk_item.EstimatedDurationSec);
											if(finish_dtm != null) {
												timechunk_text += (".." + SLib.datetimefmt(finish_dtm, SLib.DATF_ISO8601, SLib.TIMF_HMS));
											}
										}
									}
								}
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_PRC, prc_name);
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_TIMECHUNK, timechunk_text);
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
											Context app_ctx = getApplication();
											if(app_ctx != null && app_ctx instanceof StyloQApp) {
												int duration = CPM.GetServiceDurationForPrc(0, cur_id);
												if(duration > 0) {
													String text = ((StyloQApp)app_ctx).GetString("duration");
													if(SLib.GetLen(text) == 0)
														text = SLib.durationfmt(duration);
													else
														text += ": " + SLib.durationfmt(duration);
													SLib.SetCtrlString(iv, R.id.ATTENDANCEPREREQ_GOODS_DURATION, text);
												}
											}
										}
										{
											double val = cur_entry.JsItem.optDouble("price", 0.0);
											SLib.SetCtrlString(iv, R.id.ATTENDANCEPREREQ_GOODS_PRICE, (val > 0.0) ? SLib.formatdouble(val, 2) : "");
										}
										{
											View ctl = iv.findViewById(R.id.buttonOrder);
											if(ctl != null && ctl instanceof Button) {
												Button btn = (Button)ctl;
												double val = 0.0; //GetGoodsQttyInCurrentDocument(cur_id);
												if(val > 0.0)
													btn.setText(String.format("%.0f", val));
												else
													btn.setText("order");
											}
										}
										String blob_signature = cur_entry.JsItem.optString("imgblobs", null);
										SLib.SetupImage(this, iv.findViewById(R.id.ATTENDANCEPREREQ_GOODS_IMG), blob_signature);
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
														PrcByGoodsListAdapter adapter = new PrcByGoodsListAdapter(iv.getContext(), R.layout.li_simple, cur_id, prc_list);
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
										SLib.LDATE dt = AttdcBlk.GetSelectionDate();
										final int hour = AttdcBlk.WorkHours[ev_subj.ItemIdx];
										SLib.SetCtrlString(iv, R.id.LVITEM_HOUR, String.format("%02d", hour));
										{
											for(int i = 0; i < 12; i++) {
												int ctl_id = 0;
												switch(i) {
													case 0: ctl_id = R.id.LVITEM_MIN00; break;
													case 1: ctl_id = R.id.LVITEM_MIN05; break;
													case 2: ctl_id = R.id.LVITEM_MIN10; break;
													case 3: ctl_id = R.id.LVITEM_MIN15; break;
													case 4: ctl_id = R.id.LVITEM_MIN20; break;
													case 5: ctl_id = R.id.LVITEM_MIN25; break;
													case 6: ctl_id = R.id.LVITEM_MIN30; break;
													case 7: ctl_id = R.id.LVITEM_MIN35; break;
													case 8: ctl_id = R.id.LVITEM_MIN40; break;
													case 9: ctl_id = R.id.LVITEM_MIN45; break;
													case 10: ctl_id = R.id.LVITEM_MIN50; break;
													case 11: ctl_id = R.id.LVITEM_MIN55; break;
												}
												View ctl = (ctl_id != 0) ? iv.findViewById(ctl_id) : null;
												if(ctl != null) {
													ctl.setOnClickListener(new View.OnClickListener()
														{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, new Integer(hour)); }});
												}
											}
											/*{
												View btn = fv.findViewById(R.id.CTL_PREV);
												btn.setOnClickListener(new View.OnClickListener()
													{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }});
											}*/
										}
										boolean busy_hour = false;
										boolean busy_hour_cur = false;
										if(AttdcBlk.Prc != null && ((AttdcBlk.BusyList != null && AttdcBlk.BusyList.size() > 0) ||
											AttdcBlk.CurrentBookingBusyList != null && AttdcBlk.CurrentBookingBusyList.size() > 0)) {
											SLib.LDATETIME end_dtm = new SLib.LDATETIME(dt, new SLib.LTIME(hour, 59, 59, 990));
											SLib.STimeChunk cell = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, 0, 0, 100)), end_dtm);
											if(AttdcBlk.BusyList != null) {
												for(int j = 0; !busy_hour && j < AttdcBlk.BusyList.size(); j++) {
													SLib.STimeChunk is = cell.Intersect(AttdcBlk.BusyList.get(j));
													if(is != null)
														busy_hour = true;
												}
											}
											if(AttdcBlk.CurrentBookingBusyList != null) {
												for(int j = 0; !busy_hour_cur && j < AttdcBlk.CurrentBookingBusyList.size(); j++) {
													SLib.STimeChunk is = cell.Intersect(AttdcBlk.CurrentBookingBusyList.get(j));
													if(is != null)
														busy_hour_cur = true;
												}
											}
										}
										for(int i = 0; i < 12; i++) { // Ячейки по 5 минут
											boolean busy = false;
											boolean busy_cur = false;
											if(busy_hour || busy_hour_cur) {
												SLib.LDATETIME end_dtm = new SLib.LDATETIME(dt, new SLib.LTIME(hour, (i+1) * 5 - 1, 59, 990));
												SLib.STimeChunk cell = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, i * 5, 0, 100)), end_dtm);
												if(busy_hour) {
													for(int j = 0; !busy && j < AttdcBlk.BusyList.size(); j++) {
														SLib.STimeChunk is = cell.Intersect(AttdcBlk.BusyList.get(j));
														if(is != null)
															busy = true;
													}
												}
												if(busy_hour_cur) {
													for(int j = 0; !busy_cur && j < AttdcBlk.CurrentBookingBusyList.size(); j++) {
														SLib.STimeChunk is = cell.Intersect(AttdcBlk.CurrentBookingBusyList.get(j));
														if(is != null)
															busy_cur = true;
													}
												}
											}
											int ctl_id = 0;
											switch(i) {
												case 0: ctl_id = R.id.LVITEM_MIN00; break;
												case 1: ctl_id = R.id.LVITEM_MIN05; break;
												case 2: ctl_id = R.id.LVITEM_MIN10; break;
												case 3: ctl_id = R.id.LVITEM_MIN15; break;
												case 4: ctl_id = R.id.LVITEM_MIN20; break;
												case 5: ctl_id = R.id.LVITEM_MIN25; break;
												case 6: ctl_id = R.id.LVITEM_MIN30; break;
												case 7: ctl_id = R.id.LVITEM_MIN35; break;
												case 8: ctl_id = R.id.LVITEM_MIN40; break;
												case 9: ctl_id = R.id.LVITEM_MIN45; break;
												case 10: ctl_id = R.id.LVITEM_MIN50; break;
												case 11: ctl_id = R.id.LVITEM_MIN55; break;
											}
											View ctl = (ctl_id != 0) ? iv.findViewById(ctl_id) : null;
											if(ctl != null) {
												int color = 0;
												if(busy) {
													if(busy_cur)
														color = getResources().getColor(R.color.ListItemFocused, getTheme());
													else
														color = getResources().getColor(R.color.Accent, getTheme());
												}
												else if(busy_cur)
													color = getResources().getColor(R.color.ListItemFocused, getTheme());
												if(color != 0)
													ctl.setBackgroundColor(color);
												else
													ctl.setBackgroundResource(R.drawable.shape_viewframe);
											}
										}
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
											String blob_signature = cur_entry.JsItem.optString("imgblobs", null);
											SLib.SetupImage(this, iv.findViewById(R.id.ATTENDANCEPREREQ_PRC_IMG), blob_signature);
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
															GoodsByPrcListAdapter adapter = new GoodsByPrcListAdapter(iv.getContext(), R.layout.li_goods_by_prc, cur_id, goods_list);
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
								else if(a.GetListRcId() == R.id.attendancePrereqBookingListView) {
									if(CPM.CurrentOrder != null && CPM.CurrentOrder.BkList != null && ev_subj.ItemIdx < CPM.CurrentOrder.BkList.size()) {
										View iv = ev_subj.RvHolder.itemView;
										Document.BookingItem cur_entry = CPM.CurrentOrder.BkList.get(ev_subj.ItemIdx);
										if(cur_entry != null) {
											CommonPrereqModule.WareEntry goods = CPM.FindGoodsItemByGoodsID(cur_entry.GoodsID);
											String goods_name = "";
											if(goods != null && goods.JsItem != null) {
												goods_name = goods.JsItem.optString("nm", "");
											}
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, goods_name);
										}
									}
								}
								else if(a.GetListRcId() == R.id.attendancePrereqOrderListView) { // Список зафиксированных заказов
									if(CPM.OrderHList != null && ev_subj.ItemIdx < CPM.OrderHList.size()) {
										View iv = ev_subj.RvHolder.itemView;
										Document.Head dh = CPM.OrderHList.get(ev_subj.ItemIdx);
										if(dh != null) {
											SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_CODE, (SLib.GetLen(dh.Code) > 0) ? dh.Code : "");
											{
												SLib.LDATE d = null;
												if(dh.Time > 0)
													d = SLib.BuildDateByEpoch(dh.Time);
												else if(dh.CreationTime > 0)
													d = SLib.BuildDateByEpoch(dh.CreationTime);
												if(d != null)
													SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_DATE, d.Format(SLib.DATF_ISO8601|SLib.DATF_CENTURY));
											}
											{
												String amount_text = String.format(Locale.US, "%12.2f", dh.Amount);
												SLib.SetCtrlString(iv, R.id.CTL_DOCUMENT_AMOUNT, amount_text);
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
						SLib.SlFragmentStatic fragment = (SLib.SlFragmentStatic)srcObj;
						View fv = (View)subj;
						View lv = fv.findViewById(R.id.attendancePrereqGoodsListView);
						if(lv != null) {
							((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
							SetupRecyclerListView(fv, R.id.attendancePrereqGoodsListView, R.layout.li_attendanceprereq_goods);
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
								((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
								SetupRecyclerListView(fv, R.id.attendancePrereqGoodsGroupListView, R.layout.li_simple);
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
									((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
									SetupRecyclerListView(fv, R.id.attendancePrereqProcessorListView, R.layout.li_attendanceprereq_processor);
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
										SetupRecyclerListView(fv, R.id.attendancePrereqAttendanceView, R.layout.li_attendanceprereq_attendance);
										{
											View btn = fv.findViewById(R.id.CTL_PREV);
											if(btn != null) {
												btn.setOnClickListener(new View.OnClickListener() {
													@Override public void onClick(View v)
														{ HandleEvent(SLib.EV_COMMAND, v, null); }
												});
											}
										}
										{
											View btn = fv.findViewById(R.id.CTL_NEXT);
											if(btn != null) {
												btn.setOnClickListener(new View.OnClickListener() {
													@Override public void onClick(View v)
														{ HandleEvent(SLib.EV_COMMAND, v, null); }
												});
											}
										}
									}
									else {
										lv = fv.findViewById(R.id.attendancePrereqBookingListView);
										if(lv != null) {
											((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
											SetupRecyclerListView(fv, R.id.attendancePrereqBookingListView, R.layout.li_attendanceprereq_booking);
										}
										else {
											lv = fv.findViewById(R.id.searchPaneListView);
											if(lv != null) {
												((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(this));
												SetupRecyclerListView(fv, R.id.searchPaneListView, R.layout.li_searchpane_result);
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
																StyloQApp app_ctx = (StyloQApp)getApplication();
																boolean sr = CPM.SearchInSimpleIndex(app_ctx, pattern);
																String srit = CPM.SearchResult.GetSearchResultInfoText();
																if(!sr && CPM.SearchResult != null)
																	CPM.SearchResult.Clear();
																SLib.SetCtrlString(fv, R.id.CTL_SEARCHPANE_RESULTINFO, srit);
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
											else {
												lv = fv.findViewById(R.id.attendancePrereqOrderListView);
												if(lv != null) {
													((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
													SetupRecyclerListView(fv, R.id.attendancePrereqOrderListView, R.layout.li_attendanceprereq_order);
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
											GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView, _idx, -1);
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
											GotoTab(CommonPrereqModule.Tab.tabGoodsGroups, R.id.attendancePrereqGoodsGroupListView, _idx, -1);
										}
										else if(se.ObjType == SLib.PPOBJ_PROCESSOR) {
											int _idx = CPM.FindProcessorItemIndexByID(se.ObjID);
											GotoTab(CommonPrereqModule.Tab.tabProcessors, R.id.attendancePrereqProcessorListView, _idx, -1);
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
												SetCurrentAttendancePrc((CommonPrereqModule.ProcessorEntry)ev_subj.ItemObj);
												GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
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
												GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
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
											GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
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
											GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
										}
									}
								}
							}
							else if(a.GetListRcId() == R.id.attendancePrereqGoodsGroupListView) {
								if(app_ctx != null && CPM.GoodsGroupListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < CPM.GoodsGroupListData.size()) {
									final int group_id = CPM.GoodsGroupListData.get(ev_subj.ItemIdx).optInt("id", 0);
									if(CPM.SetGoodsFilterByGroup(group_id)) {
										SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.VISIBLE);
										do_update_goods_list_and_toggle_to_it = true;
									}
									//app_ctx.RunSvcCommand(SvcIdent, ListData.Items.get(ev_subj.ItemIdx));
								}
							}
							if(do_update_goods_list_and_toggle_to_it) {
								GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView, -1, -1);
							}
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				int view_id = (srcObj != null && srcObj instanceof View) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.tbButtonSearch) {
					GotoTab(CommonPrereqModule.Tab.tabSearch, 0, -1, -1);
				}
				else if(view_id == R.id.tbButtonClearFiter) {
					CPM.ResetGoodsFiter();
					SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
					GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView, -1, -1);
				}
				else if(view_id == R.id.CTL_PREV) {
					if(AttdcBlk != null && AttdcBlk.DecrementSelectedDate()) {
						CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
						if(te != null && te.TabView != null) {
							UpdateAttendanceView();
							HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
						}
					}
				}
				else if(view_id == R.id.CTL_NEXT) {
					if(AttdcBlk != null && AttdcBlk.IncrementSelectedDate(P)) {
						CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
						if(te != null && te.TabView != null) {
							UpdateAttendanceView();
							HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
						}
					}
				}
				else if(view_id == R.id.STDCTL_COMMITBUTTON) {
					CPM.CommitCurrentDocument(this);
				}
				else {
					if(view_id == R.id.LVITEM_MIN00 || view_id == R.id.LVITEM_MIN05 || view_id == R.id.LVITEM_MIN10 ||
						view_id == R.id.LVITEM_MIN15 || view_id == R.id.LVITEM_MIN20 || view_id == R.id.LVITEM_MIN25 ||
						view_id == R.id.LVITEM_MIN30 || view_id == R.id.LVITEM_MIN35 || view_id == R.id.LVITEM_MIN40 ||
						view_id == R.id.LVITEM_MIN45 || view_id == R.id.LVITEM_MIN50 || view_id == R.id.LVITEM_MIN55) {
						if(subj != null && subj instanceof Integer) {
							final int goods_id = AttdcBlk.GetGoodsID();
							final int prc_id = AttdcBlk.GetPrcID();
							if(prc_id <= 0) {
								; // @err
							}
							else if(goods_id <= 0) {
								; // @err
							}
							else {
								int hour = (Integer)subj; // Кнопки расписания "заряжены" на час, а не на индекс часа
								if(hour >= 0 && hour < 24) {
									SLib.LDATE dt = AttdcBlk.GetSelectionDate();
									//int hour = AttdcBlk.WorkHours[hour_idx];
									int minuts = 0;
									switch(view_id) {
										case R.id.LVITEM_MIN00: minuts = 0; break;
										case R.id.LVITEM_MIN05: minuts = 5; break;
										case R.id.LVITEM_MIN10: minuts = 10; break;
										case R.id.LVITEM_MIN15: minuts = 15; break;
										case R.id.LVITEM_MIN20: minuts = 20; break;
										case R.id.LVITEM_MIN25: minuts = 25; break;
										case R.id.LVITEM_MIN30: minuts = 30; break;
										case R.id.LVITEM_MIN35: minuts = 35; break;
										case R.id.LVITEM_MIN40: minuts = 40; break;
										case R.id.LVITEM_MIN45: minuts = 45; break;
										case R.id.LVITEM_MIN50: minuts = 50; break;
										case R.id.LVITEM_MIN55: minuts = 55; break;
									}
									SLib.LTIME start_tm = new SLib.LTIME(hour, minuts, 0, 0);
									try {
										if(CPM.CurrentOrder == null)
											CPM.CurrentOrder = new Document(SLib.sqbdtSvcReq, CPM.SvcIdent, (StyloQApp)getApplicationContext());
										if(CPM.CurrentOrder.BkList == null)
											CPM.CurrentOrder.BkList = new ArrayList<Document.BookingItem>();
										CPM.CurrentOrder.BkList.clear();
										Document.BookingItem bk_item = new Document.BookingItem();
										bk_item.GoodsID = goods_id;
										bk_item.PrcID = prc_id;
										bk_item.RowIdx = 1;
										bk_item.ReqTime = new SLib.LDATETIME(dt, start_tm);
										bk_item.EstimatedDurationSec = CPM.GetServiceDurationForPrc(prc_id, goods_id);
										if(bk_item.EstimatedDurationSec <= 0)
											bk_item.EstimatedDurationSec = 3600; // default value
										CPM.CurrentOrder.BkList.add(bk_item);
										AttdcBlk.CurrentBookingBusyList = CPM.GetCurrentDocumentBusyList(prc_id);
										UpdateAttendanceView();
										UpdateBookingView();
									} catch(StyloQException exn) {
										;
									}
								}
							}
						}
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
					if(ir.OriginalCmdItem != null && ir.OriginalCmdItem.Name.equalsIgnoreCase("PostDocument")) {
						CPM.Locker_CommitCurrentDocument = false;
						if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
							SecretTagPool svc_reply_pool = (SecretTagPool)ir.InfoReply;
							byte [] svc_reply_bytes = svc_reply_pool.Get(SecretTagPool.tagRawData);
							if(SLib.GetLen(svc_reply_bytes) > 0) {
								String svc_reply_js_text = new String(svc_reply_bytes);
								try {
									JSONObject sv_reply_js = new JSONObject(svc_reply_js_text);
									if(sv_reply_js != null) {
										JSONObject js_prc = sv_reply_js.optJSONObject("prc");
										if(js_prc != null) {
											int prc_id = js_prc.optInt("id", 0);
											if(prc_id > 0) {
												if(CPM.ProcessorListData != null) {
													boolean found = false;
													for(int i = 0; !found && i < CPM.ProcessorListData.size(); i++) {
														CommonPrereqModule.ProcessorEntry prc_entry = CPM.ProcessorListData.get(i);
														if(prc_entry != null && prc_entry.JsItem != null) {
															int local_prc_id = prc_entry.JsItem.optInt("id", 0);
															if(local_prc_id == prc_id) {
																prc_entry.JsItem = js_prc;
																found = true;
															}
														}
													}
													if(!found) {
														CommonPrereqModule.ProcessorEntry new_prc_entry = new CommonPrereqModule.ProcessorEntry(js_prc);
														CPM.ProcessorListData.add(new_prc_entry);
													}
													NotifyTabContentChanged(CommonPrereqModule.Tab.tabAttendance, 0);
												}
											}
										}
									}
								} catch(JSONException exn) {
									;
								}
							}
						}
						if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
							CPM.MakeCurrentDocList((StyloQApp)getApplication());
							CPM.CurrentOrder = null;
							//NotifyCurrentOrderChanged();
							GotoTab(CommonPrereqModule.Tab.tabOrders, R.id.attendancePrereqOrderListView, -1, -1);
							CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
						}
						else {

						}
					}
				}
				break;
		}
		return result;
	}
}
