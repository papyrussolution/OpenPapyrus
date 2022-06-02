// CmdRGridActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class CmdRGridActivity extends SLib.SlActivity {
	public CmdRGridActivity()
	{
		SvcIdent = null;
		ListData = null;
		Vdl = null;
	}
	private byte [] SvcIdent; // Получает через intent ("SvcIdent")
	private String CmdName; // Получает через intent ("CmdName")
	private String CmdDescr; // Получает через intent ("CmdDescr")
	private JSONArray ListData;
	ViewDescriptionList Vdl;
	/*
	//
	// ARG(level IN): 0 - detail, 1 - header, 2 - footer
	//
	private LinearLayout CreateItemLayout(ViewDescriptionList viewDesription, int level)
	{
		LinearLayout result = new LinearLayout(this);
		result.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
			LinearLayout.LayoutParams.WRAP_CONTENT));
		result.setOrientation(LinearLayout.HORIZONTAL);
		if(viewDesription != null) {
			final int cc = viewDesription.GetCount();
			for(int i = 0; i < cc; i++) {
				TextView tv2 = viewDesription.CreateBaseEntryTextView(this,1, level, i);
				if(tv2 != null)
					result.addView(tv2);
			}
		}
		return  result;
	}
	*/
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					try {
						setContentView(R.layout.activity_cmdrgrid);
						Intent intent = getIntent();
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
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
							long doc_id = intent.getLongExtra("SvcReplyDocID", 0);
							if(doc_id > 0) {
								StyloQDatabase.SecStoragePacket doc_packet = db.GetPeerEntry(doc_id);
								if(doc_packet != null) {
									byte[] raw_doc = doc_packet.Pool.Get(SecretTagPool.tagRawData);
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
									Vdl = new ViewDescriptionList();
									if(Vdl.FromJsonObj(js_vd)) {
										final int _vdlc = Vdl.GetCount();
										assert (_vdlc > 0);
										SLib.Margin fld_mrgn = new SLib.Margin(4, 2, 4, 2);
										for(int i = 0; i < _vdlc; i++) {
											ViewDescriptionList.Item vdl_item = Vdl.Get(i);
											if(vdl_item != null) {
												vdl_item.Mrgn = fld_mrgn;
												vdl_item.StyleRcId = R.style.ReportListItemText;
												ViewDescriptionList.DataPreprocessBlock dpb = Vdl.StartDataPreprocessing(this, i);
												if(dpb != null && dpb.ColumnDescription != null) {
													for(int j = 0; j < ListData.length(); j++) {
														JSONObject cur_entry = (JSONObject) ListData.get(j);
														if(cur_entry != null)
															Vdl.DataPreprocessingIter(dpb, cur_entry.optString(dpb.ColumnDescription.FieldName, ""));
													}
													Vdl.FinishDataProcessing(dpb);
													dpb = null;
												}
											}
										}
									}
								}
							}
							{
								LinearLayout header_layout = (LinearLayout) findViewById(R.id.gridCommandViewHeader);
								if(header_layout != null) {
									LinearLayout _lo_ = ViewDescriptionList.CreateItemLayout(Vdl, this, 1);
									if(_lo_ != null)
										header_layout.addView(_lo_);
								}
							}
							{
								if(Vdl != null && Vdl.IsThereTotals()) {
									LinearLayout bottom_layout = (LinearLayout) findViewById(R.id.gridCommandViewBottom);
									if(bottom_layout != null) {
										LinearLayout _lo_ = ViewDescriptionList.CreateItemLayout(Vdl, this, 2);
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
						}
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
						LinearLayout _lo = ViewDescriptionList.CreateItemLayout(Vdl, this,0);
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
								View iv = ev_subj.RvHolder.itemView;
								{
									int color_row_res = ((ev_subj.ItemIdx % 2) == 0) ? R.color.GridInterleavedOdd : R.color.GridInterleavedEven;
									iv.setBackgroundColor(getResources().getColor(color_row_res, getTheme()));
								}
								try {
									JSONObject cur_entry = (JSONObject)ListData.get(ev_subj.ItemIdx);
									final int _vdlc = Vdl.GetCount();
									for(int i = 0; i < _vdlc; i++) {
										TextView ctl = (TextView)iv.findViewById(i+1);
										if(ctl != null) {
											ViewDescriptionList.Item di = Vdl.Get(i);
											if(di != null) {
												String val = cur_entry.optString(di.FieldName, "");
												ctl.setText(val);
											}
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