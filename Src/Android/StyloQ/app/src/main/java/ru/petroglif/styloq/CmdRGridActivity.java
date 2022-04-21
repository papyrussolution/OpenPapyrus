// CmdRGridActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import android.text.TextPaint;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;

public class CmdRGridActivity extends SLib.SlActivity {
	public CmdRGridActivity()
	{
		SvcIdent = null;
		ListData = null;
		Vdl = null;
	}
	static class ViewDescrItem {
		ViewDescrItem()
		{
			TotalFunc = 0;
			RTotalResult = 0.0;
			ITotalResult = 0;
			LayoutWeight = 0.0f;
			LayoutWidth = 0.0f;
			AllNumeric = false;
		}
		String Zone;
		String FieldName;
		String Title;
		int   TotalFunc;
		double RTotalResult; // @v11.3.1
		int    ITotalResult; // @v11.3.1
		boolean AllNumeric;
		float LayoutWeight;
		float LayoutWidth; // @v11.2.11
	}
	private byte [] SvcIdent; // Получает через intent ("SvcIdent")
	private String CmdName; // Получает через intent ("CmdName")
	private String CmdDescr; // Получает через intent ("CmdDescr")
	private JSONArray ListData;
	private ArrayList <ViewDescrItem> Vdl;
	//
	// ARG(phase IN): 0 - preprocess, 1 - running
	// ARG(level IN): 0 - detail, 1 - header, 2 - footer
	//
	private TextView CreateBaseEntryTextView(ArrayList <ViewDescrItem> viewDesription, int phase, int level, int columnIdx/*0..*/)
	{
		TextView result = null;
		if(viewDesription != null && columnIdx >= 0 && columnIdx < viewDesription.size()) {
			ViewDescrItem di = viewDesription.get(columnIdx);
			result = new TextView(this);
			result.setSingleLine();
			int alignment = View.TEXT_ALIGNMENT_TEXT_START;
			if(phase == 1 && di.AllNumeric && level == 0)
				alignment = View.TEXT_ALIGNMENT_TEXT_END;
			result.setTextAlignment(alignment);
			/*
			if(phase == 1)
				result.setAutoSizeTextTypeWithDefaults(TextView.AUTO_SIZE_TEXT_TYPE_UNIFORM);
			 */
			if(level == 0)
				result.setId(columnIdx + 1);
			else if(level == 1)
				result.setText(di.Title);
			else if(level == 2) {
				if(di.TotalFunc > 0)
					if(di.TotalFunc == SLib.AGGRFUNC_COUNT)
						result.setText(Integer.toString(di.ITotalResult));
					else
						result.setText(Double.toString(di.RTotalResult));
				else
					result.setText("");
			}
			int lo_width = 0;
			float lo_weight = 0.0f/*di.LayoutWeight*/;
			if(phase == 0)
				lo_width = 0;
			else
				lo_width = (int)di.LayoutWidth;
			LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(/*LinearLayout.LayoutParams.WRAP_CONTENT*/lo_width,
					LinearLayout.LayoutParams.MATCH_PARENT, lo_weight);
			lp.setMargins(6, 2, 6, 2);
			result.setLayoutParams(lp);
		}
		return result;
	}
	//
	// ARG(level IN): 0 - detail, 1 - header, 2 - footer
	//
	private LinearLayout CreateItemLayout(ArrayList <ViewDescrItem> viewDesription, int level)
	{
		LinearLayout result = new LinearLayout(this);
		result.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
			LinearLayout.LayoutParams.WRAP_CONTENT));
		result.setOrientation(LinearLayout.HORIZONTAL);
		if(viewDesription != null && viewDesription.size() > 0) {
			for(int i = 0; i < viewDesription.size(); i++) {
				TextView tv2 = CreateBaseEntryTextView(viewDesription, 1, level, i);
				if(tv2 != null)
					result.addView(tv2);
			}
		}
		return  result;
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					try {
						setContentView(R.layout.activity_cmdrgrid);
						Intent intent = getIntent();
						StyloQApp app_ctx = (StyloQApp)getApplicationContext();
						StyloQDatabase db = app_ctx.GetDB();
						String svc_reply_doc_json = null;
						SvcIdent = intent.getByteArrayExtra("SvcIdent");
						CmdName = intent.getStringExtra("CmdName");
						CmdDescr = intent.getStringExtra("CmdDescr");
						{
							String title_text = null;
							if(SLib.GetLen(SvcIdent) > 0) {
								StyloQDatabase.SecStoragePacket svc_packet = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
								String svc_name = (svc_packet != null) ? svc_packet.GetSvcName(null) : null;
								if(SLib.GetLen(svc_name) > 0)
									SLib.SetCtrlString(this, R.id.tbSvcName, svc_name);
							}
							if(SLib.GetLen(CmdName) > 0) {
								title_text = CmdName;
							}
							if(SLib.GetLen(CmdDescr) > 0) {
								if(SLib.GetLen(title_text) > 0)
									title_text = title_text + "\n" + CmdDescr;
								else
									title_text = CmdDescr;
							}
							if(SLib.GetLen(title_text) > 0) {
								SLib.SetCtrlString(this, R.id.tbTitle, title_text);
							}
						}
						boolean there_is_totals = false;
						long doc_id = intent.getLongExtra("SvcReplyDocID", 0);
						if(doc_id > 0) {
							StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
							if(doc_packet != null) {
								byte [] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
								if(SLib.GetLen(raw_doc) > 0)
									svc_reply_doc_json = new String(raw_doc);
							}
						}
						else {
							svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
						}
						if(SLib.GetLen(svc_reply_doc_json) > 0) {
							JSONObject js = new JSONObject(svc_reply_doc_json);
							JSONObject js_vd = js.optJSONObject("ViewDescription");
							ListData = js.optJSONArray("Iter");
							if(js_vd != null) {
								JSONArray _vdl = js_vd.optJSONArray("Items");
								if(_vdl != null) {
									Vdl = new ArrayList<ViewDescrItem>();
									int i;
									for(i = 0; i < _vdl.length(); i++) {
										JSONObject _vdl_item = (JSONObject) _vdl.get(i);
										if(_vdl_item != null) {
											ViewDescrItem new_item = new ViewDescrItem();
											new_item.Zone = _vdl_item.optString("Zone", "");
											new_item.FieldName = _vdl_item.optString("FieldName", "");
											new_item.Title = _vdl_item.optString("Text", "");
											new_item.TotalFunc = _vdl_item.optInt("TotalFunc", 0);
											Vdl.add(new_item);
										}
									}
									double [] sum_len_list = new double[Vdl.size()];
									float [] max_len_list = new float[Vdl.size()];
									long [] cnt_list = new long[Vdl.size()];

									//Paint p = new Paint();
									//TextPaint tp = result.getPaint();
									//CreateBaseEntryTextView(Vdl, 0, i);
									for(i = 0; i < _vdl.length(); i++) {
										ViewDescrItem di = Vdl.get(i);
										TextView tv = CreateBaseEntryTextView(Vdl, 0, 0, i);
										TextPaint tp = tv.getPaint();
										di.AllNumeric = true;
										double rtotal = 0.0;
										int itotal = 0;
										int _total_count = 0;
										double _total_sum = 0.0;
										double _total_max = -Double.MAX_VALUE;
										double _total_min = Double.MAX_VALUE;
										for(int j = 0; j < ListData.length(); j++) {
											JSONObject cur_entry = (JSONObject)ListData.get(j);
											String val = cur_entry.optString(di.FieldName, "");
											_total_count++;
											float tw = tp.measureText(val);
											if(tw > 0.0) {
												if(SLib.IsNumeric(val)) {
													double rv = Double.valueOf(val);
													_total_sum += rv;
													if(_total_max < rv)
														_total_max = rv;
													if(_total_min > rv)
														_total_min = rv;
												}
												else {
													di.AllNumeric = false;
												}
												sum_len_list[i] += tw;
												cnt_list[i]++;
												if(max_len_list[i] < tw)
													max_len_list[i] = tw;
											}
											/*if(val.length() > 0) {
												sum_len_list[i] += val.length();
												cnt_list[i]++;
											}*/
										}
										if(di.TotalFunc > 0) {
											switch(di.TotalFunc) {
												case SLib.AGGRFUNC_COUNT: di.ITotalResult = _total_count; break;
												case SLib.AGGRFUNC_SUM: di.RTotalResult = _total_sum; break;
												case SLib.AGGRFUNC_AVG: di.RTotalResult = (_total_count > 0) ? _total_sum / _total_count : 0.0; break;
												case SLib.AGGRFUNC_MAX: di.RTotalResult = (_total_max > -Double.MAX_VALUE) ? _total_max : 0.0; break;
												case SLib.AGGRFUNC_MIN: di.RTotalResult = (_total_min < Double.MAX_VALUE) ? _total_min : 0.0; break;
											}
											there_is_totals = true;
										}
									}
									{
										for(i = 0; i < _vdl.length(); i++) {
											double avgl;
											if(cnt_list[i] > 0)
												avgl = (double) sum_len_list[i] / (double) cnt_list[i];
											else
												avgl = 1.0;
											Vdl.get(i).LayoutWeight = (float) avgl;
											Vdl.get(i).LayoutWidth = max_len_list[i];
										}
									}
								}
							}
						}
						{
							LinearLayout header_layout = (LinearLayout)findViewById(R.id.gridCommandViewHeader);
							if(header_layout != null) {
								LinearLayout _lo_ = CreateItemLayout(Vdl, 1);
								if(_lo_ != null)
									header_layout.addView(_lo_);
							}
						}
						{
							if(there_is_totals) {
								LinearLayout bottom_layout = (LinearLayout) findViewById(R.id.gridCommandViewBottom);
								if(bottom_layout != null) {
									LinearLayout _lo_ = CreateItemLayout(Vdl, 2);
									if(_lo_ != null)
										bottom_layout.addView(_lo_);
								}
							}
						}
						SetupRecyclerListView(null, R.id.gridCommandView, null);
						//RecyclerView lv = (RecyclerView)findViewById(R.id.gridCommandView);
						//if(lv != null) {
							//lv.add
						//}
						/*
						ResultText = intent.getStringExtra("SvcReplyText");
						if(SLib.GetLen(ResultText) > 0) {
							try {
								JSONObject jsobj = new JSONObject(ResultText);
								if(jsobj != null) {
									String reply_result = jsobj.optString("result");
									String reply_msg = jsobj.optString("msg");
									String reply_errmsg = jsobj.optString("errmsg");
									if(SLib.GetLen(reply_msg) > 0) {
										SetCtrlString(R.id.cmdResultText, reply_msg);
									}
									else if(SLib.GetLen(reply_errmsg) > 0) {
										SetCtrlString(R.id.cmdResultText, reply_errmsg);
									}
								}
							} catch(JSONException e) {
								//e.printStackTrace();
							}
							//SetCtrlString()
						}
						 */
					} catch(JSONException exn) {
						//
					} catch(StyloQException exn) {
						//exn.printStackTrace();
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = (srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter) srcObj : null;
					result = new Integer((adapter != null && ListData != null) ? ListData.length() : 0);
				}
				break;
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemView != null) {
						LinearLayout _lo = CreateItemLayout(Vdl, 0);
						if(_lo != null) {
							//((ViewGroup)ev_subj.ItemView).addView(_lo);
							SLib.RecyclerListAdapter adapter = (srcObj != null && srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter)srcObj : null;
							result = new SLib.RecyclerListViewHolder(_lo, adapter);
						}
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder != null) {
							// RecyclerView
							if(ListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < ListData.length()) {
								//
								View iv = ev_subj.RvHolder.itemView;
								{
									int color_row_res = ((ev_subj.ItemIdx % 2) == 0) ? R.color.GridInterleavedOdd : R.color.GridInterleavedEven;
									iv.setBackgroundColor(getResources().getColor(color_row_res, getTheme()));
								}
								try {
									JSONObject cur_entry = (JSONObject)ListData.get(ev_subj.ItemIdx);
									for(int i = 0; i < Vdl.size(); i++) {
										TextView ctl = (TextView) iv.findViewById(i+1);
										if(ctl != null) {
											ViewDescrItem di = Vdl.get(i);
											String val = cur_entry.optString(di.FieldName, "");
											ctl.setText(val);
										}
									}
								} catch(JSONException exn) {
									//exn.printStackTrace();
								}
								//ctl.setText(cur_entry.Name);
								//holder.flagView.setImageResource(state.getFlagResource());
							}
						}
						else {
							;
						}
					}
				}
				break;
		}
		return result;
	}
}