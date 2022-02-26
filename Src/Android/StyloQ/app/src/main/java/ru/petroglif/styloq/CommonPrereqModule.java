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
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

public class CommonPrereqModule {
	public byte[] SvcIdent; // Получает через intent ("SvcIdent")
	public String CmdName; // Получает через intent ("CmdName")
	public String CmdDescr; // Получает через intent ("CmdDescr")
	public ArrayList<SimpleSearchIndexEntry> SimpleSearchIndex;
	public SimpleSearchResult SearchResult;
	public ArrayList<JSONObject> GoodsGroupListData;
	public GoodsFilt Gf;
	public ArrayList <WareEntry> GoodsListData;
	public static class WareEntry {
		WareEntry(JSONObject jsItem)
		{
			JsItem = jsItem;
			PrcExpandStatus = 0;
		}
		JSONObject JsItem;
		int   PrcExpandStatus; // 0 - no processors, 1 - processors collapsed, 2 - processors expanded
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

	public ArrayList<ProcessorEntry> ProcessorListData;

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

		SimpleSearchResult()
		{
			ObjTypeCount = 0;
			ObjTypeList = new int[64];
			SelectedItemIdx = -1;
		}
		void Clear()
		{
			List = null;
			ObjTypeCount = 0;
			SelectedItemIdx = -1;
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
	public void AddSimpleIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
	{
		//SimpleSearchIndexEntry(int objType, int objID, int attr, final String text, final String displayText)
		SimpleSearchIndex.add(new SimpleSearchIndexEntry(objType, objID, attr, text.toLowerCase(), displayText));
	}
	public boolean SearchInSimpleIndex(String pattern)
	{
		boolean result = false;
		if(SearchResult == null)
			SearchResult = new CommonPrereqModule.SimpleSearchResult();
		SearchResult.Clear();
		SearchResult.Pattern = pattern;
		if(SimpleSearchIndex != null && SLib.GetLen(pattern) > 3) {
			pattern = pattern.toLowerCase();
			for(int i = 0; i < SimpleSearchIndex.size(); i++) {
				CommonPrereqModule.SimpleSearchIndexEntry entry = SimpleSearchIndex.get(i);
				if(entry != null && SLib.GetLen(entry.Text) > 0) {
					if(entry.Text.indexOf(pattern) >= 0) {
						boolean skip = false;
						if(entry.Attr == SLib.PPOBJATTR_CODE && pattern.length() < 5)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUINN && pattern.length() < 8)
							skip = true;
						else if(entry.Attr == SLib.PPOBJATTR_RUKPP && pattern.length() < 6)
							skip = true;
						if(!skip) {
							SearchResult.Add(entry);
							result = true;
						}
					}
				}
			}
		}
		return result;
	}
	public CommonPrereqModule()
	{
		SvcIdent = null;
		CmdName = null;
		CmdDescr = null;
		GoodsGroupListData = null;
		GoodsListData = null;
		Gf = null;
	}
	public void GetAttributesFromIntent(Intent intent)
	{
		if(intent != null) {
			SvcIdent = intent.getByteArrayExtra("SvcIdent");
			CmdName = intent.getStringExtra("CmdName");
			CmdDescr = intent.getStringExtra("CmdDescr");
		}
	}
	public void SetupActivityTitles(SLib.SlActivity activity, StyloQDatabase db) throws StyloQException
	{
		if(activity != null && db != null) {
			String title_text = null;
			if(SLib.GetLen(SvcIdent) > 0) {
				StyloQDatabase.SecStoragePacket svc_packet = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
				String svc_name = (svc_packet != null) ? svc_packet.GetSvcName() : null;
				if(SLib.GetLen(svc_name) > 0)
					SLib.SetCtrlString(this, R.id.CTL_ACTIVITYHEAD_SVCNAME, svc_name);
			}
			if(SLib.GetLen(CmdName) > 0)
				title_text = CmdName;
			if(SLib.GetLen(CmdDescr) > 0)
				title_text = (SLib.GetLen(title_text) > 0) ? (title_text + "\n" + CmdDescr) : CmdDescr;
			if(SLib.GetLen(title_text) > 0)
				SLib.SetCtrlString(this, R.id.CTL_ACTIVITYHEAD_TITLE, title_text);
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
									WareEntry ware_entry = FindGoodsItemByGoodsID(iter_id);
									if(ware_entry != null) {
										if(result == null)
											result = new ArrayList<WareEntry>();
										result.add(ware_entry);
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
	private static class SearchDetailListAdapter extends ArrayAdapter {
		private int RcId;
		SearchDetailListAdapter(Context ctx, int rcId, ArrayList data)
		{
			super(ctx, rcId, data);
			RcId = rcId;
		}
		@Override public View getView(int position, View convertView, ViewGroup parent)
		{
			// Get the data item for this position
			Object item = (Object)getItem(position);
			//Context ctx = parent.getContext();
			if(item != null) {
				// Check if an existing view is being reused, otherwise inflate the view
				Context _ctx = getContext();
				if(convertView == null) {
					convertView = LayoutInflater.from(_ctx).inflate(RcId, parent, false);
				}
				if(convertView != null) {
					if(item instanceof CommonPrereqModule.SimpleSearchIndexEntry) {
						CommonPrereqModule.SimpleSearchIndexEntry se = (CommonPrereqModule.SimpleSearchIndexEntry)item;
						String pattern = null;
						if(_ctx instanceof CmdROrderPrereqActivity)
							pattern = ((CmdROrderPrereqActivity)_ctx).GetSimpleSearchResultPattern();
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
									int color_found = _ctx.getResources().getColor(R.color.FoundListItem, _ctx.getTheme());
									if(fp_start > 0) {
										SpannableString ss = new SpannableString(se.Text.substring(0, fp_start-1));
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
	public void GetSearchPaneListViewItem(StyloQApp appCtx, View itemView, int itemIdx)
	{
		if(appCtx != null && SearchResult != null && itemView != null && itemIdx < SearchResult.GetObjTypeCount()) {
			int obj_type = SearchResult.GetObjTypeByIndex(itemIdx);
			//StyloQApp app_ctx = (StyloQApp)getApplicationContext();
			String obj_type_title = SLib.GetObjectTitle(appCtx, obj_type);
			SLib.SetCtrlString(itemView, R.id.LVITEM_GENERICNAME, (obj_type_title != null) ? obj_type_title : "");
			{
				ListView detail_lv = (ListView)itemView.findViewById(R.id.searchPaneTerminalListView);
				ArrayList <CommonPrereqModule.SimpleSearchIndexEntry> detail_list = SearchResult.GetListByObjType(obj_type);
				if(detail_lv != null && detail_list != null) {
					SearchDetailListAdapter adapter = new SearchDetailListAdapter(/*this*/itemView.getContext(), R.layout.searchpane_resultdetail_item, detail_list);
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
