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
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import androidx.annotation.IdRes;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import androidx.viewpager2.widget.ViewPager2;
import com.google.android.material.tabs.TabLayout;
import com.google.android.material.textfield.TextInputEditText;

import org.jetbrains.annotations.NotNull;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.TimerTask;

public class CmdRAttendancePrereqActivity extends SLib.SlActivity {
	public  CommonPrereqModule CPM;
	private AttendanceBlock AttdcBlk;
	private Param P;
	private ViewDescriptionList VdlDocs; // Описание таблицы просмотра существующих заказов
	private ArrayList <Document.EditAction> DocEditActionList;
	private void RefreshCurrentDocStatus()
	{
		if(CPM.TabList != null) {
			ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
			if(view_pager != null) {
				int tidx = view_pager.getCurrentItem();
				CommonPrereqModule.TabEntry tab_entry = CPM.TabList.get(tidx);
				if(tab_entry != null && tab_entry.TabId == CommonPrereqModule.Tab.tabBookingDocument) {
					HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
				}
			}
		}
	}
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshCurrentDocStatus(); }}); }
	}
	private static class Param {
		Param()
		{
			PrcTitle = null;
			MaxScheduleDays = 7;
			TimeSheetDiscreteness = 5; // @v11.4.3
		}
		String PrcTitle;
		int MaxScheduleDays;
		int TimeSheetDiscreteness; // 5 || 10 || 15
	}
	private static class AttendanceBlock {
		int [] WorkHours;
		private SLib.LDATE SelectionDate_; // Дата, которая отображается на экране выбора времени посещения //
		SLib.LDATETIME AttendanceTime; // Выбранное время. null - не выбрано //
		CommonPrereqModule.WareEntry Ware;
		CommonPrereqModule.ProcessorEntry Prc;
		CommonPrereqModule.ProcessorEntry ArbitraryPrc; // Процессор, выбираемый произвольным образом если клиент выбрал только товар(услугу)
		SLib.STimeChunkArray BusyList; // Зависит от Prc
		SLib.STimeChunkArray WorktimeList; // Зависит от Prc
		SLib.STimeChunkArray CurrentBookingBusyList;
		int Duration; // Ожидаемая продолжительность сессии (prcID, goodsID). assert(goodsID || Duration == 0)
		//
		AttendanceBlock(final Param param)
		{
			Ware = null;
			Prc = null;
			ArbitraryPrc = null;
			BusyList = null;
			WorktimeList = null;
			CurrentBookingBusyList = null;
			AttendanceTime = null;
			Duration = 0;
			WorkHours = null;
			SetSelectionDate(SLib.GetCurDate());
		}
		void Reset()
		{
			Ware = null;
			Prc = null;
			ArbitraryPrc = null;
			BusyList = null;
			WorktimeList = null;
			CurrentBookingBusyList = null;
			AttendanceTime = null;
			Duration = 0;
			WorkHours = null;
			SetSelectionDate(SLib.GetCurDate());
		}
		boolean IncrementSelectedDate(final Param param, boolean checkOnly)
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
					if(!checkOnly)
						SetSelectionDate(_d);
					result = true;
				}
			}
			return result;
		}
		boolean DecrementSelectedDate(boolean checkOnly)
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
					if(!checkOnly)
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
			return (Ware != null && Ware.Item != null) ? Ware.Item.ID : 0;
		}
		private CommonPrereqModule.ProcessorEntry GetPrcEntry()
		{
			if(Prc != null)
				return Prc;
			else if(ArbitraryPrc != null)
				return ArbitraryPrc;
			else
				return null;
		}
		//
		// Descr: Возвращает текущий установленный процессор.
		//   Если fixedOnly и Prc == null то возвращает 0 даже если
		//   определен произвольно выбранный процессор (ArbitraryPrc)
		//
		public int GetPrcID(boolean fixedOnly)
		{
			if(Prc != null && Prc.JsItem != null)
				return Prc.JsItem.optInt("id", 0);
			else if(!fixedOnly && ArbitraryPrc != null && ArbitraryPrc.JsItem != null)
				return ArbitraryPrc.JsItem.optInt("id", 0);
			else
				return 0;
			//return (Prc != null && Prc.JsItem != null) ? Prc.JsItem.optInt("id", 0) : 0;
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
		VdlDocs = null;
		DocEditActionList = null;
	}
	private void ResetAttendanceBlock()
	{
		if(AttdcBlk != null)
			AttdcBlk.Reset();
		UpdateAttendanceView();
	}
	private void NotifyTabContentChanged(CommonPrereqModule.Tab tabId, int innerViewId)
	{
		CPM.NotifyTabContentChanged(R.id.VIEWPAGER_ATTENDANCEPREREQ, tabId, innerViewId);
	}
	private void NotifyCurrentDocumentChanged()
	{
		NotifyTabContentChanged(CommonPrereqModule.Tab.tabBookingDocument, R.id.attendancePrereqBookingListView);
		CommonPrereqModule.TabEntry tab_entry = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
		if(tab_entry != null && tab_entry.TabView != null)
			HandleEvent(SLib.EV_SETVIEWDATA, tab_entry.TabView.getView(), null);
	}
	private void UpdateAttendanceView()
	{
		CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
		if(te != null && te.TabView != null) {
			View v = te.TabView.getView();
			if(v != null && v instanceof ViewGroup) {
				HandleEvent(SLib.EV_SETVIEWDATA, v, null); // @v11.4.4
				//
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
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.VISIBLE);
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
	private SLib.STimeChunkArray GetBusyListByPrc(CommonPrereqModule.ProcessorEntry prcEntry)
	{
		SLib.STimeChunkArray result = null;
		JSONArray js_busy_list = (prcEntry != null && prcEntry.JsItem != null) ? prcEntry.JsItem.optJSONArray("busy_list") : null;
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
							if(result == null)
								result = new SLib.STimeChunkArray();
							result.add(tc);
						}
					}
				}
			}
		}
		return result;
	}
	private SLib.STimeChunkArray GetWorktimeListByPrc(CommonPrereqModule.ProcessorEntry prcEntry)
	{
		SLib.STimeChunkArray result = null;
		JSONArray js_worktime_list = (prcEntry != null && prcEntry.JsItem != null) ? prcEntry.JsItem.optJSONArray("worktime") : null;
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
							if(result == null)
								result = new SLib.STimeChunkArray();
							result.add(tc);
						}
					}
				}
			}
		}
		return result;
	}
	private void SetupCurrentAttendanceByCurrentDocument()
	{
		if(!CPM.IsCurrentDocumentEmpty()) {
			if(AttdcBlk == null)
				AttdcBlk = new AttendanceBlock(P);
			Document _doc = CPM.GetCurrentDocument();
			if(_doc != null && _doc.BkList != null && _doc.BkList.size() > 0) {
				String goods_name = "";
				String prc_name = "";
				String timechunk_text = "";
				final Document.BookingItem bk_item = _doc.BkList.get(0);
				if(bk_item != null) {
					CommonPrereqModule.WareEntry goods_entry = CPM.FindGoodsItemByGoodsID(bk_item.GoodsID);
					CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(bk_item.PrcID);
					int prc_id = 0;
					if(prc_entry != null && prc_entry.JsItem != null)
						prc_id = prc_entry.JsItem.optInt("id", 0);
					if(bk_item.ReqTime != null)
						AttdcBlk.SetSelectionDate(bk_item.ReqTime.d);
					AttdcBlk.Ware = goods_entry;
					AttdcBlk.Prc = prc_entry;
					AttdcBlk.BusyList = GetBusyListByPrc(AttdcBlk.Prc);
					AttdcBlk.WorktimeList = GetWorktimeListByPrc(AttdcBlk.Prc);
					AttdcBlk.SetSelectionDate(AttdcBlk.GetSelectionDate()); // Пересчитывам календарь в соответствии с процессором
					AttdcBlk.CurrentBookingBusyList = CPM.GetCurrentDocumentBusyList(prc_id);
					AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
				}
			}
		}
	}
	private boolean SetCurrentAttendancePrc(CommonPrereqModule.ProcessorEntry entry)
	{
		boolean result = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(entry != null) {
				if(AttdcBlk == null)
					AttdcBlk = new AttendanceBlock(P);
				//int prc_id = AttdcBlk.GetPrcID(false);
				int prc_id = entry.JsItem.optInt("id", 0);
				int ware_id = AttdcBlk.GetGoodsID();
				if(ware_id > 0) {
					if(prc_id > 0) {
						ArrayList <CommonPrereqModule.WareEntry> goods_list = CPM.GetGoodsListByPrc(prc_id);
						boolean ware_belong_to_prc = false;
						if(goods_list != null && goods_list.size() > 0) {
							for(int gidx = 0; !ware_belong_to_prc && gidx < goods_list.size(); gidx++) {
								CommonPrereqModule.WareEntry inner_entry = goods_list.get(gidx);
								if(inner_entry != null && inner_entry.Item != null) {
									if(inner_entry.Item.ID == ware_id)
										ware_belong_to_prc = true;
								}
							}
						}
						if(ware_belong_to_prc) {
							AttdcBlk.Prc = entry;
							AttdcBlk.BusyList = GetBusyListByPrc(AttdcBlk.Prc);
							AttdcBlk.WorktimeList = GetWorktimeListByPrc(AttdcBlk.Prc);
							AttdcBlk.SetSelectionDate(AttdcBlk.GetSelectionDate()); // Пересчитывам календарь в соответствии с процессором
							AttdcBlk.CurrentBookingBusyList = CPM.GetCurrentDocumentBusyList(prc_id);
							AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
							result = true;
						}
						else {
							app_ctx.DisplayError(this, ppstr2.PPERR_STQ_GOODSNOTBELONGTOPRC, 0);
						}
					}
					else {
						AttdcBlk.CurrentBookingBusyList = null;
						AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
						result = true;
					}
				}
				else {
					app_ctx.DisplayError(this, ppstr2.PPERR_STQ_BEFOREPRCGOODSNEEDED, 0);
				}
			}
			else if(AttdcBlk != null) {
				AttdcBlk.Prc = null;
				AttdcBlk.BusyList = null;
				AttdcBlk.CurrentBookingBusyList = null;
				AttdcBlk.Duration = CPM.GetServiceDurationForPrc(0, AttdcBlk.GetGoodsID());
				result = true;
			}
			if(result) {
				UpdateAttendanceView();
				NotifyTabContentChanged(CommonPrereqModule.Tab.tabProcessors, R.id.attendancePrereqProcessorListView);
			}
		}
		return result;
	}
	private boolean SetCurrentAttendanceArbitraryPrc(SLib.LDATETIME startDtm)
	{
		boolean result = false;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(AttdcBlk != null) {
				final int _current_prc_id = AttdcBlk.GetPrcID(true);
				final int goods_id = AttdcBlk.GetGoodsID();
				if(goods_id > 0 && _current_prc_id == 0) {
					ArrayList<CommonPrereqModule.ProcessorEntry> prc_list = CPM.GetProcessorListByGoods(goods_id);
					if(prc_list != null && prc_list.size() > 0) {
						if(prc_list.size() == 1) {
							AttdcBlk.ArbitraryPrc = prc_list.get(0);
							result = true;
						}
						else {
							Collections.shuffle(prc_list);
							if(startDtm != null) {
								for(int i = 0; !result && i < prc_list.size(); i++) {
									CommonPrereqModule.ProcessorEntry prc_entry = prc_list.get(i);
									final int iter_prc_id = (prc_entry != null && prc_entry.JsItem != null) ? prc_entry.JsItem.optInt("id", 0) : 0;
									if(iter_prc_id > 0) {
										SLib.STimeChunkArray busy_list = GetBusyListByPrc(prc_entry);
										if(busy_list != null) {
											AttdcBlk.ArbitraryPrc = prc_entry;
											result = true;
										}
										else {
											int estimated_duration_sec = CPM.GetServiceDurationForPrc(iter_prc_id, goods_id);
											if(estimated_duration_sec <= 0)
												estimated_duration_sec = 3600; // default value
											SLib.LDATETIME end_dt = SLib.plusdatetimesec(startDtm, estimated_duration_sec);
											SLib.STimeChunk tc = new SLib.STimeChunk(startDtm, end_dt);
											if(busy_list == null || busy_list.Intersect(tc) == null) {
												AttdcBlk.ArbitraryPrc = prc_entry;
												result = true;
											}
										}
									}
								}
							}
							else {
								AttdcBlk.ArbitraryPrc = prc_list.get(0);
								result = true;
							}
						}
					}
				}
			}
		}
		return result;
	}
	private boolean SetCurrentAttendanceWare(CommonPrereqModule.WareEntry entry)
	{
		boolean result = true;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			if(entry != null) {
				if(AttdcBlk == null) {
					AttdcBlk = new AttendanceBlock(P);
					AttdcBlk.Ware = entry;
				}
				else {
					final int entry_goods_id = entry.Item.ID;
					final int prc_id = AttdcBlk.GetPrcID(true);
					if(prc_id > 0) {
						ArrayList<CommonPrereqModule.ProcessorEntry> entry_prc_list = CPM.GetProcessorListByGoods(entry_goods_id);
						boolean found = false;
						for(int i = 0; !found && i < entry_prc_list.size(); i++) {
							final CommonPrereqModule.ProcessorEntry iter_prc_entry = entry_prc_list.get(i);
							if(iter_prc_entry != null && iter_prc_entry.JsItem != null) {
								int iter_prc_id = iter_prc_entry.JsItem.optInt("id", 0);
								if(iter_prc_id == prc_id)
									found = true;
							}
						}
						if(found)
							AttdcBlk.Ware = entry;
						else {
							app_ctx.DisplayError(this, ppstr2.PPERR_STQ_PRCNOTBELONGTOGOODS, 0);
							result = false;
						}
					}
					else {
						AttdcBlk.Ware = entry;
					}
				}
			}
			else if(AttdcBlk != null) {
				AttdcBlk.Ware = null;
			}
			if(AttdcBlk != null) {
				final int prc_id = AttdcBlk.GetPrcID(false);
				final int goods_id = AttdcBlk.GetGoodsID();
				if(prc_id == 0 && goods_id > 0) {
					SLib.STimeChunkArray united_worktime_list = null;
					SLib.STimeChunkArray intersected_busy_list = null;
					ArrayList<CommonPrereqModule.ProcessorEntry> prc_list = CPM.GetProcessorListByGoods(goods_id);
					if(prc_list != null) {
						for(int prcidx = 0; prcidx < prc_list.size(); prcidx++) {
							final CommonPrereqModule.ProcessorEntry prc_entry = prc_list.get(prcidx);
							SLib.STimeChunkArray wtlist = GetWorktimeListByPrc(prc_entry);
							SLib.STimeChunkArray busy_list = GetBusyListByPrc(prc_entry);
							if(wtlist != null) {
								united_worktime_list = wtlist.Union(united_worktime_list);
							}
							if(busy_list != null)
								intersected_busy_list = (intersected_busy_list == null) ? busy_list : busy_list.Intersect(intersected_busy_list);
						}
					}
					AttdcBlk.BusyList = intersected_busy_list;
					AttdcBlk.WorktimeList = united_worktime_list;
					AttdcBlk.SetSelectionDate(AttdcBlk.GetSelectionDate()); // Пересчитывам календарь в соответствии с товаром
				}
				AttdcBlk.Duration = CPM.GetServiceDurationForPrc(prc_id, goods_id);
			}
			if(result) {
				UpdateAttendanceView();
				NotifyTabContentChanged(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView);
			}
		}
		else
			result = false;
		return result;
	}
	private void CreateTabList(boolean force)
	{
		final int tab_layout_rcid = R.id.TABLAYOUT_ATTENDANCEPREREQ;
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && (CPM.TabList == null || force)) {
			CPM.TabList = new ArrayList<CommonPrereqModule.TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			CommonPrereqModule.TabInitEntry [] tab_init_list = {
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoodsGroups, R.layout.layout_attendanceprereq_goodsgroups, "@{group_pl}", (CPM.GoodsGroupListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabGoods, R.layout.layout_attendanceprereq_goods, "@{ware_service_pl}", (CPM.GoodsListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabProcessors, R.layout.layout_attendanceprereq_processors, "@{processor_pl}", (CPM.ProcessorListData != null)),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabAttendance, R.layout.layout_attendanceprereq_attendance, /*"@{booking}"*/"@{selection_time}", true),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabBookingDocument, R.layout.layout_attendanceprereq_booking, "@{orderdocument}", true),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabOrders, R.layout.layout_attendanceprereq_orders, "@{booking_pl}", true),
				new CommonPrereqModule.TabInitEntry(CommonPrereqModule.Tab.tabSearch, R.layout.layout_searchpane, "[search]", true),
			};
			for(int i = 0; i < tab_init_list.length; i++) {
				final CommonPrereqModule.TabInitEntry _t = tab_init_list[i];
				if(_t != null && _t.Condition) {
					SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(_t.Tab.ordinal(), _t.Rc, tab_layout_rcid);
					String title = (_t.Tab == CommonPrereqModule.Tab.tabProcessors && SLib.GetLen(P.PrcTitle) > 0) ? P.PrcTitle : SLib.ExpandString(app_ctx, _t.Title);
					CPM.TabList.add(new CommonPrereqModule.TabEntry(_t.Tab, title, f));
				}
			}
		}
	}
	private void MakeSimpleSearchIndex()
	{
		CPM.InitSimpleIndex();
		CPM.AddGoodsToSimpleIndex();
		CPM.AddGoodsGroupsToSimpleIndex();
		if(CPM.ProcessorListData != null) {
			for(int i = 0; i < CPM.ProcessorListData.size(); i++) {
				CommonPrereqModule.ProcessorEntry entry = CPM.ProcessorListData.get(i);
				if(entry != null && entry.JsItem != null) {
					int id = entry.JsItem.optInt("id", 0);
					if(id > 0) {
						String nm = entry.JsItem.optString("nm");
						CPM.AddSimpleIndexEntry(SLib.PPOBJ_PROCESSOR, id, SLib.PPOBJATTR_NAME, nm, null);
					}
				}
			}
		}
	}
	private CommonPrereqModule.TabEntry SearchTabEntry(CommonPrereqModule.Tab tab)
	{
		return CPM.SearchTabEntry(R.id.VIEWPAGER_ATTENDANCEPREREQ, tab);
	}
	private void GotoTab(CommonPrereqModule.Tab tab, @IdRes int recyclerViewToUpdate, int goToIndex, int nestedIndex)
	{
		CPM.GotoTab(tab, R.id.VIEWPAGER_ATTENDANCEPREREQ, recyclerViewToUpdate, goToIndex, nestedIndex);
	}
	private void SetListBackground(View iv, Object adapter, int itemIdxToDraw, int objType, int objID)
	{
		int shaperc = 0;
		if(GetListFocusedIndex(adapter) == itemIdxToDraw)
			shaperc = R.drawable.shape_listitem_focused;
		else {
			boolean is_catched = false;
			if(objID > 0) {
				if(!CPM.IsCurrentDocumentEmpty()) {
					final Document _doc = CPM.GetCurrentDocument();
					if(objType == SLib.PPOBJ_PERSON && objID == _doc.H.ClientID)
						is_catched = true;
					else if(objType == SLib.PPOBJ_LOCATION && objID == _doc.H.DlvrLocID)
						is_catched = true;
					else if(objType == SLib.PPOBJ_GOODS && CPM.HasGoodsInCurrentOrder(objID))
						is_catched = true;
					else if(objType == SLib.PPOBJ_PROCESSOR && CPM.HasPrcInCurrentOrder(objID))
						is_catched = true;
				}
				if(!is_catched && AttdcBlk != null) {
					if(objType == SLib.PPOBJ_PROCESSOR) {
						if(AttdcBlk.GetPrcID(false) == objID)
							is_catched = true;
					}
					else if(objType == SLib.PPOBJ_GOODS) {
						if(AttdcBlk.GetGoodsID() == objID)
							is_catched = true;
					}
				}
			}
			if(is_catched)
				shaperc = R.drawable.shape_listitem_catched;
			else if(CPM.IsObjInSearchResult(objType, objID))
				shaperc = R.drawable.shape_listitem_found;
			else
				shaperc = R.drawable.shape_listitem;
		}
		if(shaperc != 0)
			iv.setBackground(getResources().getDrawable(shaperc, getTheme()));
	}
	private static class PrcByGoodsListAdapter extends SLib.InternalArrayAdapter {
		private int GoodsID;
		PrcByGoodsListAdapter(Context ctx, int rcId, int goodsID, ArrayList data)
		{
			super(ctx, rcId, data);
			GoodsID = goodsID;
		}
		int   GetGoodsID() { return GoodsID; }
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
					TextView v = convertView.findViewById(R.id.LVITEM_GENERICNAME);
					if(v != null && item instanceof CommonPrereqModule.ProcessorEntry) {
						int prc_id = 0;
						CommonPrereqModule.ProcessorEntry entry = (CommonPrereqModule.ProcessorEntry) item;
						if(entry.JsItem != null) {
							prc_id = entry.JsItem.optInt("id", 0);
							v.setText(entry.JsItem.optString("nm", ""));
						}
						if(_ctx instanceof CmdRAttendancePrereqActivity)
							((CmdRAttendancePrereqActivity)_ctx).SetListBackground(convertView, this, position, SLib.PPOBJ_PROCESSOR, prc_id);
					}
				}
			}
			return convertView; // Return the completed view to render on screen
		}
	}
	@NotNull private static String MakeDurationText(StyloQApp appCtx, int duration)
	{
		String text = "";
		if(duration > 0 && appCtx != null) {
			text = appCtx.GetString("duration");
			if(SLib.GetLen(text) == 0)
				text = SLib.durationfmt(duration);
			else
				text += ": " + SLib.durationfmt(duration);
		}
		return text;
	}
	private static class GoodsByPrcListAdapter extends SLib.InternalArrayAdapter {
		private int PrcID;
		GoodsByPrcListAdapter(Context ctx, int rcId, int prcID, ArrayList data)
		{
			super(ctx, rcId, data);
			PrcID = prcID;
		}
		int GetPrcID() { return PrcID; }
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
					if(v != null && item instanceof CommonPrereqModule.WareEntry) {
						CommonPrereqModule.WareEntry ware_entry = (CommonPrereqModule.WareEntry)item;
						if(ware_entry.Item != null) {
							int goods_id = ware_entry.Item.ID;
							v.setText(ware_entry.Item.Name);
							if(_ctx instanceof CmdRAttendancePrereqActivity) {
								StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(_ctx);
								if(app_ctx != null) {
									CommonPrereqModule _cpm = ((CmdRAttendancePrereqActivity)_ctx).CPM;
									int duration = _cpm.GetServiceDurationForPrc(PrcID, goods_id);
									double price = (ware_entry.PrcPrice > 0.0) ? ware_entry.PrcPrice : _cpm.GetPriceForPrc(PrcID, goods_id);
									if(duration > 0)
										SLib.SetCtrlString(convertView, R.id.CTL_GOODSBYPRC_DURATION, MakeDurationText(app_ctx, duration));
									if(price > 0.0) {
										String text = _cpm.FormatCurrency(price);
										SLib.SetCtrlString(convertView, R.id.CTL_GOODSBYPRC_PRICE, text);
									}
								}
								((CmdRAttendancePrereqActivity)_ctx).SetListBackground(convertView, this, position, SLib.PPOBJ_GOODS, goods_id);
							}
						}
					}
				}
			}
			return convertView; // Return the completed view to render on screen
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
				int vg_id = vg.getId();
				if(vg_id == R.id.LAYOUT_ATTENDANCEPREREQ_BOOKING) {
					if(CPM.UpdateMemoInCurrentDocument(SLib.GetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO)))
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
			CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.VISIBLE);
			if(Document.DoesStatusAllowModifications(CPM.GetCurrentDocument().GetDocStatus())) {
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabProcessors, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabAttendance, View.VISIBLE);
			}
			else {
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabProcessors, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.GONE);
				//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabAttendance, View.GONE);
			}
			if(gotoTabIfNotEmpty)
				GotoTab(CommonPrereqModule.Tab.tabBookingDocument, R.id.attendancePrereqBookingListView, -1, -1);
		}
		else {
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoods, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabProcessors, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabGoodsGroups, View.VISIBLE);
			//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabAttendance, View.VISIBLE);
			if(removeTabIfEmpty)
				CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.GONE);
		}
		SetupCurrentAttendanceByCurrentDocument();
		UpdateAttendanceView();
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
							{
								JSONObject js_param = js_head.optJSONObject("param");
								if(js_param != null) {
									P.PrcTitle = js_param.optString("prctitle", "");
									P.MaxScheduleDays = js_param.optInt("MaxScheduleDays", 7);
									if(P.MaxScheduleDays < 1 || P.MaxScheduleDays > 365)
										P.MaxScheduleDays = 7;
									P.TimeSheetDiscreteness = js_param.optInt("TimeSheetDiscreteness", 5);
									// @v11.4.3 {
									if(P.TimeSheetDiscreteness != 5 && P.TimeSheetDiscreteness != 10 && P.TimeSheetDiscreteness != 15)
										P.TimeSheetDiscreteness = 5;
									// } @v11.4.3
								}
							}
							CPM.GetCommonJsonFactors(js_head);
							CPM.MakeGoodsGroupListFromCommonJson(js_head);
							CPM.MakeProcessorListFromCommonJson(js_head);
							CPM.MakeGoodsListFromCommonJson(js_head);
						}
						CPM.RestoreRecentDraftDocumentAsCurrent(); // @v11.4.0
						CPM.MakeCurrentDocList();
						MakeSimpleSearchIndex();
						requestWindowFeature(Window.FEATURE_NO_TITLE);
						setContentView(R.layout.activity_cmdrattendanceprereq);
						CPM.SetupActivity(db, R.id.VIEWPAGER_ATTENDANCEPREREQ, R.id.TABLAYOUT_ATTENDANCEPREREQ);
						ViewPager2 view_pager = (ViewPager2) findViewById(R.id.VIEWPAGER_ATTENDANCEPREREQ);
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
								if(CPM.IsCurrentDocumentEmpty())
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.GONE);
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabOrders, (CPM.OrderHList == null || CPM.OrderHList.size() <= 0) ? View.GONE : View.VISIBLE);
							}
						}
						SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
					}
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
						case R.id.attendancePrereqAttendanceView: result = new Integer((AttdcBlk != null) ? AttdcBlk.GetWorkhoursCount() : 0); break;
						case R.id.attendancePrereqBookingListView: result = new Integer(CPM.GetCurrentDocumentBookingListCount()); break;
						case R.id.attendancePrereqOrderListView: result = new Integer((CPM.OrderHList != null) ? CPM.OrderHList.size() : 0); break;
						case R.id.searchPaneListView: result = new Integer((CPM.SearchResult != null) ? CPM.SearchResult.GetObjTypeCount() : 0);
						break;
					}
				}
				break;
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					StyloQApp app_ctx = GetAppCtx();
					ViewGroup vg = (ViewGroup)srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_ATTENDANCEPREREQ_ATTENDANCE) {
						if(AttdcBlk == null) {
							SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_WARE, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_PRC, View.GONE);
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
								String ds = AttdcBlk.GetSelectionDate().Format(/*SLib.DATF_ISO8601 | SLib.DATF_CENTURY*/SLib.DATF_INTERNET);
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, ds);
								SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, AttdcBlk.DecrementSelectedDate(true) ? View.VISIBLE : View.INVISIBLE);
								SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, AttdcBlk.IncrementSelectedDate(P, true) ? View.VISIBLE : View.INVISIBLE);
							}
							else {
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DATE, "");
								SLib.SetCtrlVisibility(vg, R.id.CTL_PREV, View.GONE);
								SLib.SetCtrlVisibility(vg, R.id.CTL_NEXT, View.GONE);
							}
							if(AttdcBlk.Ware != null && AttdcBlk.Ware.Item != null) {
								String nm = AttdcBlk.Ware.Item.Name;
								SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_WARE, nm);
								SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_WARE, View.VISIBLE);
							}
							else {
								SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_WARE, View.GONE);
							}
							{
								View view_prctxt = vg.findViewById(R.id.CTL_ATTENDANCE_PRC);
								if(view_prctxt != null) {
									if(AttdcBlk.Prc != null && AttdcBlk.Prc.JsItem != null) {
										String nm = AttdcBlk.Prc.JsItem.optString("nm", "");
										SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_PRC, nm);
										SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_PRC, View.VISIBLE);
										view_prctxt.setBackgroundResource(0);
									}
									else if(AttdcBlk.ArbitraryPrc != null && AttdcBlk.ArbitraryPrc.JsItem != null) {
										String nm = AttdcBlk.ArbitraryPrc.JsItem.optString("nm", "");
										SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_PRC, nm);
										SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_PRC, View.VISIBLE);
										view_prctxt.setBackgroundResource(R.drawable.shape_arbitraryprcframe);
									}
									else
										SLib.SetCtrlVisibility(vg, R.id.CTL_ATTENDANCE_BACK_PRC, View.GONE);
								}
							}
							SLib.SetCtrlString(vg, R.id.CTL_ATTENDANCE_DURATION, MakeDurationText(app_ctx, AttdcBlk.Duration));
						}
					}
					else if(vg_id == R.id.LAYOUT_ATTENDANCEPREREQ_BOOKING) {
						int status_image_rc_id = 0;
						if(CPM.IsCurrentDocumentEmpty()) {
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_PRC, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_TIMECHUNK, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_AMOUNT, "");
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO, "");
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON1, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON2, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON3, View.GONE);
							SLib.SetCtrlVisibility(vg, R.id.CTL_DOCUMENT_ACTIONBUTTON4, View.GONE);
						}
						else {
							final Document _doc = CPM.GetCurrentDocument();
							if(SLib.GetLen(_doc.H.Code) > 0)
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_CODE, _doc.H.Code);
							SLib.LDATE d = _doc.GetNominalDate();
							if(d != null)
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_DATE, d.Format(SLib.DATF_ISO8601|SLib.DATF_CENTURY));
							SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_MEMO, _doc.H.Memo);
							if(_doc.BkList != null && _doc.BkList.size() > 0) {
								String goods_name = "";
								String prc_name = "";
								String timechunk_text = "";
								final Document.BookingItem bk_item = _doc.BkList.get(0);
								if(bk_item != null) {
									CommonPrereqModule.WareEntry goods_entry = CPM.FindGoodsItemByGoodsID(bk_item.GoodsID);
									if(goods_entry != null)
										goods_name = goods_entry.Item.Name;
									CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(bk_item.PrcID);
									if(prc_entry != null)
										prc_name = prc_entry.JsItem.optString("nm", "");
									SLib.STimeChunk tc = bk_item.GetEsimatedTimeChunk();
									if(tc != null)
										timechunk_text = tc.Format(SLib.DATF_DMY, SLib.TIMF_HM);
								}
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_PRC, prc_name);
								SLib.SetCtrlString(vg, R.id.CTL_DOCUMENT_TIMECHUNK, timechunk_text);
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
						if(ev_subj.RvHolder != null) {
							if(srcObj != null && srcObj instanceof SLib.RecyclerListAdapter) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
								if(a.GetListRcId() == R.id.searchPaneListView) {
									CPM.GetSearchPaneListViewItem(ev_subj.RvHolder.itemView, ev_subj.ItemIdx);
								}
								else if(a.GetListRcId() == R.id.attendancePrereqGoodsListView) {
									CommonPrereqModule.WareEntry cur_entry = CPM.GetGoodsListItemByIdx(ev_subj.ItemIdx);
									if(cur_entry != null && cur_entry.Item != null) {
										final int cur_id = cur_entry.Item.ID;
										View iv = ev_subj.RvHolder.itemView;
										SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, cur_entry.Item.Name);
										{
											StyloQApp app_ctx = GetAppCtx();
											int duration = CPM.GetServiceDurationForPrc(0, cur_id);
											if(duration > 0)
												SLib.SetCtrlString(iv, R.id.ATTENDANCEPREREQ_GOODS_DURATION, MakeDurationText(app_ctx, duration));
										}
										{
											double val = cur_entry.Item.Price;
											SLib.SetCtrlString(iv, R.id.ATTENDANCEPREREQ_GOODS_PRICE, (val > 0.0) ? CPM.FormatCurrency(val) : "");
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
										String blob_signature = cur_entry.Item.ImgBlob;
										SLib.SetupImage(this, iv.findViewById(R.id.ATTENDANCEPREREQ_GOODS_IMG), blob_signature, false);
										SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_GOODS, cur_id);
										{
											ImageView ctl = (ImageView)iv.findViewById(R.id.ATTENDANCEPREREQ_GOODS_EXPANDSTATUS);
											if(ctl != null) {
												ListView prc_lv = (ListView)iv.findViewById(R.id.processorByWareListView);
												ArrayList <CommonPrereqModule.ProcessorEntry> prc_list = CPM.GetProcessorListByGoods(cur_id);
												if(cur_entry.PrcExpandStatus == 0 || prc_list == null) {
													ctl.setVisibility(View.GONE);
													SLib.SetCtrlVisibility(prc_lv, View.GONE);
												}
												else if(cur_entry.PrcExpandStatus == 1) {
													ctl.setVisibility(View.VISIBLE);
													ctl.setImageResource(R.drawable.ic_triangleleft03);
													SLib.SetCtrlVisibility(prc_lv, View.GONE);
												}
												else if(cur_entry.PrcExpandStatus == 2) {
													ctl.setVisibility(View.VISIBLE);
													ctl.setImageResource(R.drawable.ic_triangledown03);
													if(prc_lv != null) {
														prc_lv.setVisibility(View.VISIBLE);
														PrcByGoodsListAdapter adapter = new PrcByGoodsListAdapter(iv.getContext(), R.layout.li_simple_sublist, cur_id, prc_list);
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
										if(P.TimeSheetDiscreteness == 15) {
											for(int i = 0; i < 4; i++) {
												int ctl_id = 0;
												switch(i) {
													case 0: ctl_id = R.id.LVITEM_MIN00; break;
													case 1: ctl_id = R.id.LVITEM_MIN15; break;
													case 2: ctl_id = R.id.LVITEM_MIN30; break;
													case 3: ctl_id = R.id.LVITEM_MIN45; break;
												}
												View ctl = (ctl_id != 0) ? iv.findViewById(ctl_id) : null;
												if(ctl != null) {
													ctl.setOnClickListener(new View.OnClickListener()
													{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, new Integer(hour)); }});
												}
											}
										}
										else if(P.TimeSheetDiscreteness == 10) {
											for(int i = 0; i < 6; i++) {
												int ctl_id = 0;
												switch(i) {
													case 0: ctl_id = R.id.LVITEM_MIN00; break;
													case 1: ctl_id = R.id.LVITEM_MIN10; break;
													case 2: ctl_id = R.id.LVITEM_MIN20; break;
													case 3: ctl_id = R.id.LVITEM_MIN30; break;
													case 4: ctl_id = R.id.LVITEM_MIN40; break;
													case 5: ctl_id = R.id.LVITEM_MIN50; break;
												}
												View ctl = (ctl_id != 0) ? iv.findViewById(ctl_id) : null;
												if(ctl != null) {
													ctl.setOnClickListener(new View.OnClickListener()
														{ @Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, new Integer(hour)); }});
												}
											}
										}
										else { // 5
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
										}
										boolean busy_hour = false;
										boolean busy_hour_cur = false;
										if(/*AttdcBlk.Prc != null &&*/((AttdcBlk.BusyList != null && AttdcBlk.BusyList.size() > 0) ||
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
										if(P.TimeSheetDiscreteness == 15) {
											for(int i = 0; i < 4; i++) { // Ячейки по 15 минут
												boolean busy = false;
												boolean busy_cur = false;
												if(busy_hour || busy_hour_cur) {
													SLib.LDATETIME end_dtm = new SLib.LDATETIME(dt, new SLib.LTIME(hour, (i + 1) * P.TimeSheetDiscreteness - 1, 59, 990));
													SLib.STimeChunk cell = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, i * P.TimeSheetDiscreteness, 0, 100)), end_dtm);
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
													case 1: ctl_id = R.id.LVITEM_MIN15; break;
													case 2: ctl_id = R.id.LVITEM_MIN30; break;
													case 3: ctl_id = R.id.LVITEM_MIN45; break;
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
													if(ctl instanceof TextView)
														((TextView)ctl).setText(String.format("%02d:%02d", hour, i * P.TimeSheetDiscreteness));
												}
											}
										}
										else if(P.TimeSheetDiscreteness == 10) {
											for(int i = 0; i < 6; i++) { // Ячейки по 10 минут
												boolean busy = false;
												boolean busy_cur = false;
												if(busy_hour || busy_hour_cur) {
													SLib.LDATETIME end_dtm = new SLib.LDATETIME(dt, new SLib.LTIME(hour, (i + 1) * P.TimeSheetDiscreteness - 1, 59, 990));
													SLib.STimeChunk cell = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, i * P.TimeSheetDiscreteness, 0, 100)), end_dtm);
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
													case 1: ctl_id = R.id.LVITEM_MIN10; break;
													case 2: ctl_id = R.id.LVITEM_MIN20; break;
													case 3: ctl_id = R.id.LVITEM_MIN30; break;
													case 4: ctl_id = R.id.LVITEM_MIN40; break;
													case 5: ctl_id = R.id.LVITEM_MIN50; break;
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
													if(ctl instanceof TextView)
														((TextView)ctl).setText(String.format("%02d:%02d", hour, i * P.TimeSheetDiscreteness));
												}
											}
										}
										else { // 5
											for(int i = 0; i < 12; i++) { // Ячейки по 5 минут
												boolean busy = false;
												boolean busy_cur = false;
												if(busy_hour || busy_hour_cur) {
													SLib.LDATETIME end_dtm = new SLib.LDATETIME(dt, new SLib.LTIME(hour, (i + 1) * P.TimeSheetDiscreteness - 1, 59, 990));
													SLib.STimeChunk cell = new SLib.STimeChunk(new SLib.LDATETIME(dt, new SLib.LTIME(hour, i * P.TimeSheetDiscreteness, 0, 100)), end_dtm);
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
											SLib.SetupImage(this, iv.findViewById(R.id.ATTENDANCEPREREQ_PRC_IMG), blob_signature, false);
											SetListBackground(iv, a, ev_subj.ItemIdx, SLib.PPOBJ_PROCESSOR, cur_id);
											{
												ImageView ctl = (ImageView)iv.findViewById(R.id.ATTENDANCEPREREQ_PRC_EXPANDSTATUS);
												if(ctl != null) {
													ListView goods_lv = (ListView)iv.findViewById(R.id.goodsByPrcListView);
													ArrayList <CommonPrereqModule.WareEntry> goods_list = CPM.GetGoodsListByPrc(cur_id);
													if(cur_entry.GoodsExpandStatus == 0 || goods_list == null) {
														ctl.setVisibility(View.GONE);
														SLib.SetCtrlVisibility(goods_lv, View.GONE);
													}
													else if(cur_entry.GoodsExpandStatus == 1) {
														ctl.setVisibility(View.VISIBLE);
														ctl.setImageResource(R.drawable.ic_triangleleft03);
														SLib.SetCtrlVisibility(goods_lv, View.GONE);
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
									final int bk_list_count = CPM.GetCurrentDocumentBookingListCount();
									if(ev_subj.ItemIdx < bk_list_count) {
										View iv = ev_subj.RvHolder.itemView;
										final Document _doc = CPM.GetCurrentDocument();
										final Document.BookingItem cur_entry = _doc.BkList.get(ev_subj.ItemIdx);
										if(cur_entry != null) {
											CommonPrereqModule.WareEntry goods = CPM.FindGoodsItemByGoodsID(cur_entry.GoodsID);
											String goods_name = (goods != null && goods.Item != null) ? goods.Item.Name : "";
											SLib.SetCtrlString(iv, R.id.LVITEM_GENERICNAME, goods_name);
											if(cur_entry.Set != null && cur_entry.Set.Price > 0.0) {
												SLib.SetCtrlString(iv, R.id.ATTENDANCEPREREQ_BI_PRICE, CPM.FormatCurrency(cur_entry.Set.Price));
											}
										}
									}
								}
								else if(a.GetListRcId() == R.id.attendancePrereqOrderListView) { // Список зафиксированных заказов
									if(CPM.OrderHList != null && ev_subj.ItemIdx < CPM.OrderHList.size()) {
										View iv = ev_subj.RvHolder.itemView;
										Document.DisplayEntry cur_entry = CPM.OrderHList.get(ev_subj.ItemIdx);
										if(cur_entry != null && cur_entry.H != null) {
											final int _vdlc = VdlDocs.GetCount();
											for(int i = 0; i < _vdlc; i++) {
												//TextView ctl = (TextView)iv.findViewById(i+1);
												View ctl_view = iv.findViewById(i+1);
												if(ctl_view != null) {
													ViewDescriptionList.Item di = VdlDocs.Get(i);
													if(di != null) {
														String text = null;
														if(i == 0) { // indicator image
															if(ctl_view instanceof ImageView) {
																final int ds = StyloQDatabase.SecTable.Rec.GetDocStatus(cur_entry.H.Flags);
																int ir = Document.GetImageResourceByDocStatus(ds);
																if(ir != 0)
																	((ImageView)ctl_view).setImageResource(ir);
															}
														}
														else if(ctl_view instanceof TextView) {
															if(i == 1) { // date
																SLib.LDATE d = cur_entry.GetNominalDate();
																if(d != null)
																	text = d.Format(SLib.DATF_DMY);
															}
															else if(i == 2) { // code
																text = cur_entry.H.Code;
															}
															else if(i == 3) { // amount
																text = CPM.FormatCurrency(cur_entry.H.Amount);
															}
															else if(i == 4) { // time
																if(cur_entry.SingleBkItem != null && cur_entry.SingleBkItem.ReqTime != null) {
																	SLib.STimeChunk tc = cur_entry.SingleBkItem.GetEsimatedTimeChunk();
																	if(tc != null)
																		text = tc.Format(SLib.DATF_DMY, SLib.TIMF_HM);
																}
															}
															else if(i == 5) { // memo
																text = cur_entry.H.Memo;
															}
															((TextView)ctl_view).setText(text);
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
										int  rc_line = 0;
										if(P.TimeSheetDiscreteness == 15) {
											rc_line = R.layout.li_attendanceprereq_attendance_15min;
										}
										else if(P.TimeSheetDiscreteness == 10) {
											rc_line = R.layout.li_attendanceprereq_attendance_10min;
										}
										else {
											rc_line = R.layout.li_attendanceprereq_attendance_05min;
										}
										SetupRecyclerListView(fv, R.id.attendancePrereqAttendanceView, rc_line);
										{
											View btn = fv.findViewById(R.id.CTL_PREV);
											if(btn != null) {
												btn.setOnClickListener(new View.OnClickListener() {
													@Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }
												});
											}
										}
										{
											View btn = fv.findViewById(R.id.CTL_NEXT);
											if(btn != null) {
												btn.setOnClickListener(new View.OnClickListener() {
													@Override public void onClick(View v) { HandleEvent(SLib.EV_COMMAND, v, null); }
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
																boolean sr = CPM.SearchInSimpleIndex(pattern);
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
													StyloQApp app_ctx = GetAppCtx();
													if(app_ctx != null) {
														SLib.Margin fld_mrgn = new SLib.Margin(8, 12, 8, 12);
														VdlDocs = new ViewDescriptionList();
														{ // #0
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 1;
															col.Flags |= ViewDescriptionList.Item.fImage;
															col.FixedWidth = 32;
															col.FixedHeight = 32;
															col.Mrgn = fld_mrgn;
															VdlDocs.AddItem(col);
														}
														{ // #1
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 2;
															col.Title = app_ctx.GetString("billdate");
															col.StyleRcId = R.style.OrderListItemText;
															col.Mrgn = fld_mrgn;
															VdlDocs.AddItem(col);
														}
														{ // #2
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 3;
															col.Title = app_ctx.GetString("billno");
															col.TotalFunc = SLib.AGGRFUNC_COUNT;
															col.StyleRcId = R.style.OrderListItemText;
															col.Mrgn = fld_mrgn;
															VdlDocs.AddItem(col);
														}
														{ // #3
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 4;
															col.Title = app_ctx.GetString("billamount");
															col.TotalFunc = SLib.AGGRFUNC_SUM;
															col.StyleRcId = R.style.OrderListItemText;
															col.Mrgn = fld_mrgn;
															col.ForceAlignment = -1;
															VdlDocs.AddItem(col);
														}
														{ // #4
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 5;
															col.Title = app_ctx.GetString("time");
															col.StyleRcId = R.style.OrderListItemText;
															col.Mrgn = fld_mrgn;
															VdlDocs.AddItem(col);
														}
														{ // #5
															ViewDescriptionList.Item col = new ViewDescriptionList.Item();
															col.Id = 6;
															col.Title = app_ctx.GetString("memo");
															col.StyleRcId = R.style.OrderListItemText;
															col.Mrgn = fld_mrgn;
															VdlDocs.AddItem(col);
														}
														if(CPM.OrderHList != null && CPM.OrderHList.size() > 0) {
															final int _vdlc = VdlDocs.GetCount();
															assert (_vdlc > 0);
															for(int i = 0; i < _vdlc; i++) {
																ViewDescriptionList.DataPreprocessBlock dpb = VdlDocs.StartDataPreprocessing(this, i);
																if(dpb != null && dpb.ColumnDescription != null) {
																	for(int j = 0; j < CPM.OrderHList.size(); j++) {
																		Document.DisplayEntry cur_entry = CPM.OrderHList.get(j);
																		if(cur_entry != null && cur_entry.H != null) {
																			String text = null;
																			if(i == 0) { // status image
																				; // По-моему, здесь ничего замерять не надо - мы и так зафиксировали размер элемента
																			}
																			else if(i == 1) { // date
																				SLib.LDATE d = cur_entry.GetNominalDate();
																				if(d != null)
																					VdlDocs.DataPreprocessingIter(dpb, d.Format(SLib.DATF_DMY));
																			}
																			else if(i == 2) { // code
																				VdlDocs.DataPreprocessingIter(dpb, cur_entry.H.Code);
																			}
																			else if(i == 3) { // amount
																				text = CPM.FormatCurrency(cur_entry.H.Amount);
																				VdlDocs.DataPreprocessingIter(dpb, new Double(cur_entry.H.Amount), text);
																			}
																			else if(i == 4) { // time
																				if(cur_entry.SingleBkItem != null && cur_entry.SingleBkItem.ReqTime != null) {
																					SLib.STimeChunk tc = cur_entry.SingleBkItem.GetEsimatedTimeChunk();
																					if(tc != null) {
																						text = tc.Format(SLib.DATF_DMY, SLib.TIMF_HM);
																						VdlDocs.DataPreprocessingIter(dpb, text);
																					}
																				}
																			}
																			else if(i == 5) { // memo
																				VdlDocs.DataPreprocessingIter(dpb, null, cur_entry.H.Memo);
																			}
																		}
																	}
																	VdlDocs.FinishDataProcessing(dpb);
																	dpb = null;
																}
															}
															{
																LinearLayout header_layout = (LinearLayout) fv.findViewById(R.id.attendancePrereqOrderListHeader);
																if(header_layout != null) {
																	LinearLayout _lo_ = ViewDescriptionList.CreateItemLayout(VdlDocs, this, 1);
																	if(_lo_ != null)
																		header_layout.addView(_lo_);
																}
																if(VdlDocs.IsThereTotals()) {
																	LinearLayout bottom_layout = (LinearLayout) fv.findViewById(R.id.attendancePrereqOrderListBottom);
																	if(bottom_layout != null) {
																		LinearLayout _lo_ = ViewDescriptionList.CreateItemLayout(VdlDocs, this, 2);
																		if(_lo_ != null)
																			bottom_layout.addView(_lo_);
																	}
																}
															}
														}
														((RecyclerView) lv).setLayoutManager(new LinearLayoutManager(this));
														SetupRecyclerListView(fv, R.id.attendancePrereqOrderListView, 0);
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
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder == null) {
							if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.attendancePrereqOrderListView) {
								LinearLayout _lo = ViewDescriptionList.CreateItemLayout(VdlDocs, this,0);
								if(_lo != null) {
									SLib.RecyclerListAdapter adapter = (srcObj != null && srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter)srcObj : null;
									result = new SLib.RecyclerListViewHolder(_lo, adapter);
								}
							}
						}
						else {
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.buttonOrder);
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.ATTENDANCEPREREQ_GOODS_EXPANDSTATUS);
							SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.ATTENDANCEPREREQ_PRC_EXPANDSTATUS);
							result = ev_subj.RvHolder;
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && srcObj != null) {
						if(ev_subj.RvHolder == null) {
							if(srcObj instanceof ListView && ev_subj.ItemObj != null) {
								ListView lv = (ListView)srcObj;
								switch(lv.getId()) {
									case R.id.searchPaneTerminalListView:
										if(ev_subj.ItemObj instanceof CommonPrereqModule.SimpleSearchIndexEntry) {
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
										break;
									case R.id.processorByWareListView:
										{
											Adapter a = lv.getAdapter();
											if(a instanceof PrcByGoodsListAdapter) {
												int goods_id = ((PrcByGoodsListAdapter) a).GetGoodsID();
												if(goods_id > 0 && ev_subj.ItemObj instanceof CommonPrereqModule.ProcessorEntry) {
													CommonPrereqModule.WareEntry ware_entry = CPM.FindGoodsItemByGoodsID(goods_id);
													if(ware_entry != null) {
														boolean swr = SetCurrentAttendanceWare(ware_entry);
														boolean spr = SetCurrentAttendancePrc((CommonPrereqModule.ProcessorEntry)ev_subj.ItemObj);
														if(swr && spr)
															GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
													}
												}
											}
										}
										break;
									case R.id.goodsByPrcListView:
										{
											Adapter a = lv.getAdapter();
											if(a instanceof GoodsByPrcListAdapter) {
												int prc_id = ((GoodsByPrcListAdapter) a).GetPrcID();
												if(prc_id > 0 && ev_subj.ItemObj instanceof CommonPrereqModule.WareEntry) {
													CommonPrereqModule.ProcessorEntry prc_entry = CPM.FindProcessorItemByID(prc_id);
													if(prc_entry != null) {
														if(SetCurrentAttendanceWare((CommonPrereqModule.WareEntry) ev_subj.ItemObj) &&
															SetCurrentAttendancePrc(prc_entry))
															GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
													}
												}
											}
										}
										break;
								}
							}
						}
						else if(srcObj instanceof SLib.RecyclerListAdapter) {
							SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
							StyloQApp app_ctx = GetAppCtx();
							boolean do_update_goods_list_and_toggle_to_it = false;
							final int rc_id = a.GetListRcId();
							if(app_ctx != null && ev_subj.ItemIdx >= 0) {
								switch(rc_id) {
									case R.id.attendancePrereqGoodsListView:
										if(ev_subj.ItemIdx < CPM.GetGoodsListSize()) {
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
												else if(SetCurrentAttendanceWare(item))
													GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
											}
										}
										break;
									case R.id.attendancePrereqProcessorListView:
										if(CPM.ProcessorListData != null && ev_subj.ItemIdx < CPM.ProcessorListData.size()) {
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
													final int ware_id = (AttdcBlk != null) ? AttdcBlk.GetGoodsID() : 0;
													if(ware_id == 0 && item.GoodsExpandStatus == 1) {
														item.GoodsExpandStatus = 2;
														a.notifyItemChanged(ev_subj.ItemIdx);
													}
													else if(SetCurrentAttendancePrc(item))
														GotoTab(CommonPrereqModule.Tab.tabAttendance, 0, -1, -1);
												}
											}
										}
										break;
									case R.id.attendancePrereqGoodsGroupListView:
										if(CPM.GoodsGroupListData != null && ev_subj.ItemIdx < CPM.GoodsGroupListData.size()) {
											final int group_id = CPM.GoodsGroupListData.get(ev_subj.ItemIdx).optInt("id", 0);
											if(CPM.SetGoodsFilterByGroup(group_id)) {
												SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.VISIBLE);
												do_update_goods_list_and_toggle_to_it = true;
											}
										}
										break;
									case R.id.attendancePrereqOrderListView:
										if(CPM.OrderHList != null && ev_subj.ItemIdx < CPM.OrderHList.size()) {
											Document.DisplayEntry entry = CPM.OrderHList.get(ev_subj.ItemIdx);
											if(entry != null) {
												if(CPM.LoadDocument(entry.H.ID)) {
													SetupCurrentDocument(true, false);
													//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.VISIBLE);
													//GotoTab(CommonPrereqModule.Tab.tabBookingDocument, R.id.attendancePrereqBookingListView, -1, -1);
												}
											}
										}
										break;
								}
							}
							if(do_update_goods_list_and_toggle_to_it) {
								GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView, -1, -1);
							}
						}
					}
				}
				break;
			case SLib.EV_GETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup)
					GetFragmentData(srcObj);
				break;
			case SLib.EV_COMMAND:
				Document.EditAction acn = null;
				int minuts = -1;
				final int view_id = (srcObj != null && srcObj instanceof View) ? ((View)srcObj).getId() : 0;
				switch(view_id) {
					case R.id.tbButtonSearch:
						GotoTab(CommonPrereqModule.Tab.tabSearch, 0, -1, -1);
						break;
					case R.id.tbButtonClearFiter:
						CPM.ResetGoodsFiter();
						SLib.SetCtrlVisibility(this, R.id.tbButtonClearFiter, View.GONE);
						GotoTab(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView, -1, -1);
						break;
					case R.id.CTL_PREV:
						if(AttdcBlk != null && AttdcBlk.DecrementSelectedDate(false)) {
							CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
							if(te != null && te.TabView != null) {
								UpdateAttendanceView();
								HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
							}
						}
						break;
					case R.id.CTL_NEXT:
						if(AttdcBlk != null && AttdcBlk.IncrementSelectedDate(P, false)) {
							CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabAttendance);
							if(te != null && te.TabView != null) {
								UpdateAttendanceView();
								HandleEvent(SLib.EV_SETVIEWDATA, te.TabView.getView(), null);
							}
						}
						break;
					case R.id.CTL_DOCUMENT_ACTIONBUTTON1:
						if(DocEditActionList != null && DocEditActionList.size() > 0)
							acn = DocEditActionList.get(0);
						break;
					case R.id.CTL_DOCUMENT_ACTIONBUTTON2:
						if(DocEditActionList != null && DocEditActionList.size() > 1)
							acn = DocEditActionList.get(1);
						break;
					case R.id.CTL_DOCUMENT_ACTIONBUTTON3:
						if(DocEditActionList != null && DocEditActionList.size() > 2)
							acn = DocEditActionList.get(2);
						break;
					case R.id.CTL_DOCUMENT_ACTIONBUTTON4:
						if(DocEditActionList != null && DocEditActionList.size() > 3)
							acn = DocEditActionList.get(3);
						break;
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
				if(minuts >= 0 && subj != null && subj instanceof Integer) {
					final int goods_id = AttdcBlk.GetGoodsID();
					int prc_id = AttdcBlk.GetPrcID(true);
					final int hour = (Integer)subj; // Кнопки расписания "заряжены" на час, а не на индекс часа
					if(goods_id > 0) {
						if(hour >= 0 && hour < 24) {
							SLib.LDATETIME start_dtm = new SLib.LDATETIME(AttdcBlk.GetSelectionDate(), new SLib.LTIME(hour, minuts, 0, 0));
							//SLib.LDATE dt = AttdcBlk.GetSelectionDate();
							//int hour = AttdcBlk.WorkHours[hour_idx];
							//SLib.LTIME start_tm = new SLib.LTIME(hour, minuts, 0, 0);
							if(prc_id <= 0) {
								if(SetCurrentAttendanceArbitraryPrc(start_dtm)) {
									prc_id = AttdcBlk.GetPrcID(false);
								}
							}
							if(prc_id > 0) {
								CommonPrereqModule.ProcessorEntry prc_entry = AttdcBlk.GetPrcEntry();
								if(prc_entry != null) {
									int estimated_duration_sec = CPM.GetServiceDurationForPrc(prc_id, goods_id);
									if(estimated_duration_sec <= 0)
										estimated_duration_sec = 3600; // default value
									boolean is_intersection = true;
									{
										SLib.STimeChunkArray busy_list = GetBusyListByPrc(prc_entry);
										if(busy_list == null)
											is_intersection = false;
										else {
											SLib.LDATETIME end_dt = SLib.plusdatetimesec(start_dtm, estimated_duration_sec);
											SLib.STimeChunk tc = new SLib.STimeChunk(start_dtm, end_dt);
											if(busy_list.Intersect(tc) == null)
												is_intersection = false;
										}
									}
									if(!is_intersection) {
										Document.BookingItem bk_item = new Document.BookingItem();
										bk_item.GoodsID = goods_id;
										bk_item.PrcID = prc_id;
										bk_item.RowIdx = 1;
										bk_item.ReqTime = start_dtm;
										bk_item.EstimatedDurationSec = estimated_duration_sec;
										bk_item.Set.Qtty = 1;
										bk_item.Set.Price = CPM.GetPriceForPrc(bk_item.PrcID, bk_item.GoodsID);
										SLib.STimeChunkArray new_busy_list = CPM.PutBookingItemToCurrentDocument(bk_item);
										if(new_busy_list != null) {
											AttdcBlk.CurrentBookingBusyList = new_busy_list;
											UpdateAttendanceView();
											UpdateBookingView();
											NotifyTabContentChanged(CommonPrereqModule.Tab.tabProcessors, R.id.attendancePrereqProcessorListView);
											NotifyTabContentChanged(CommonPrereqModule.Tab.tabGoods, R.id.attendancePrereqGoodsListView);
										}
									}
								}
							}
						}
					}
				}
				if(acn != null) {
					/*else if(view_id == R.id.STDCTL_COMMITBUTTON) {
						CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
						if(te != null)
							GetFragmentData(te.TabView);
						CPM.CommitCurrentDocument();
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON) {
						CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
						if(te != null)
							GetFragmentData(te.TabView);
						CPM.CancelCurrentDocument();
					}*/
					switch(acn.Action) {
						case Document.editactionClose:
							// Просто закрыть сеанс редактирования документа (изменения и передача сервису не предполагаются)
							{
								CPM.ResetCurrentDocument();
								ResetAttendanceBlock();
							}
							NotifyCurrentDocumentChanged();
							GotoTab(CommonPrereqModule.Tab.tabOrders, R.id.orderPrereqOrderListView, -1, -1);
							//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
							SetupCurrentDocument(false, true);
							break;
						case Document.editactionSubmit:
							// store document; // Подтвердить изменения документа (передача сервису не предполагается)
							break;
						case Document.editactionSubmitAndTransmit:
							{
								// Подтвердить изменения документа с передачей сервису
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
								if(te != null)
									GetFragmentData(te.TabView);
								ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
								CPM.CommitCurrentDocument();
							}
							break;
						case Document.editactionCancelEdition:
							// Отменить изменения документа (передача сервису не предполагается)
							{
								CPM.ResetCurrentDocument();
								ResetAttendanceBlock();
							}
							NotifyCurrentDocumentChanged();
							GotoTab(CommonPrereqModule.Tab.tabOrders, R.id.orderPrereqOrderListView, -1, -1);
							//CPM.SetTabVisibility(CommonPrereqModule.Tab.tabCurrentOrder, View.GONE);
							SetupCurrentDocument(false, true);
							break;
						case Document.editactionCancelDocument:
							{
								// Отменить документ с передачей сервису факта отмены
								CommonPrereqModule.TabEntry te = SearchTabEntry(CommonPrereqModule.Tab.tabBookingDocument);
								if(te != null)
									GetFragmentData(te.TabView);
								ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
								CPM.CancelCurrentDocument();
							}
							break;
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
					if(ir.OriginalCmdItem != null) {
						if(ir.OriginalCmdItem.Name.equalsIgnoreCase("PostDocument")) {
							CPM.CurrentDocument_RemoteOp_Finish();
							ScheduleRTmr(null, 0, 0);
							if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
								SecretTagPool svc_reply_pool = (SecretTagPool)ir.InfoReply;
								JSONObject sv_reply_js = svc_reply_pool.GetJsonObject(SecretTagPool.tagRawData);
								JSONObject js_prc = (sv_reply_js != null) ? sv_reply_js.optJSONObject("prc") : null;
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
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								CPM.MakeCurrentDocList();
								{
									CPM.ResetCurrentDocument();
									ResetAttendanceBlock();
								}
								//NotifyCurrentOrderChanged();
								if(CPM.OrderHList != null && CPM.OrderHList.size() > 0) {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabOrders, View.VISIBLE);
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabOrders, R.id.attendancePrereqOrderListView);
									GotoTab(CommonPrereqModule.Tab.tabOrders, R.id.attendancePrereqOrderListView, -1, -1);
								}
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.GONE);
							}
							else {
								; // @todo
								SLib.LOG_e("CmdRAttendancePrereqActivity " + "EV_SVCQUERYRESULT ir.ResultTag != StyloQApp.SvcQueryResult.SUCCESS");
							}
						}
						else if(ir.OriginalCmdItem.Name.equalsIgnoreCase("CancelDocument")) {
							CPM.CurrentDocument_RemoteOp_Finish();
							ScheduleRTmr(null, 0, 0);
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								CPM.MakeCurrentDocList();
								{
									CPM.ResetCurrentDocument();
									ResetAttendanceBlock();
								}
								//NotifyCurrentOrderChanged();
								if(CPM.OrderHList != null && CPM.OrderHList.size() > 0) {
									CPM.SetTabVisibility(CommonPrereqModule.Tab.tabOrders, View.VISIBLE);
									NotifyTabContentChanged(CommonPrereqModule.Tab.tabOrders, R.id.attendancePrereqOrderListView);
									GotoTab(CommonPrereqModule.Tab.tabOrders, R.id.attendancePrereqOrderListView, -1, -1);
								}
								CPM.SetTabVisibility(CommonPrereqModule.Tab.tabBookingDocument, View.GONE);
							}
							else {
								; // @todo
								SLib.LOG_e("CmdRAttendancePrereqActivity " + "EV_SVCQUERYRESULT ir.ResultTag != StyloQApp.SvcQueryResult.SUCCESS");
							}
						}
					}
				}
				break;
		}
		return result;
	}
}
