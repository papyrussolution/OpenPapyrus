// GlobalSearchActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SearchView;

import androidx.recyclerview.widget.RecyclerView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Base64;
import java.util.Collections;
import java.util.TimerTask;

public class GlobalSearchActivity extends SLib.SlActivity implements SearchView.OnQueryTextListener {
	/*private static class ScopeEntry {
		String Scope;
		String Ident;
	}
	private static class ResultEntry {
		int    DisplayStatus;
		String ScopeIdent;
		SLib.PPObjID Oid;
		long   Rank;
		double Weight;
		String Text;
	}*/
	private String SearchPattern;
	private JSONObject JsSearchResult;
	private JSONArray JsScopeList;
	private JSONArray JsResultList;
	//private ArrayList <ScopeEntry> ScopeList;
	//private ArrayList <ResultEntry> ResultList;
	private int  CalledIndex; // Индекс элемента JsResultList, по которому ожидается выполнение запроса детализации
	private int  CalledIndexFinished; // Индекс элемента JsResultList, по которому завершено выполнение запроса детализации
	private long CallingStartTm; // Время ожидания
	public GlobalSearchActivity()
	{
		super();
		SearchPattern = null;
		JsSearchResult = null;
		CalledIndex = -1;
		CalledIndexFinished = -1;
		CallingStartTm = 0;
	}
	private void RefreshStatus()
	{
		if(JsResultList != null) {
			View v = findViewById(R.id.CTL_GLOBALSEARCH_RESULTLIST);
			if(v != null && v instanceof RecyclerView) {
				RecyclerView rv = (RecyclerView)v;
				RecyclerView.Adapter a = rv.getAdapter();
				if(a != null) {
					if(CalledIndex >= 0)
						a.notifyItemChanged(CalledIndex);
					if(CalledIndexFinished >= 0)
						a.notifyItemChanged(CalledIndexFinished);
					//a.notifyDataSetChanged();
				}
			}
		}
	}
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshStatus(); }}); }
	}
	@Override public boolean onQueryTextSubmit(String query)
	{
		boolean result = false;
		SearchPattern = query;
		if(SLib.GetLen(SearchPattern) > 0) {
			StyloQApp app_ctx = GetAppCtx();
			try {
				ArrayList<StyloQApp.IgnitionServerEntry> isl = app_ctx.GetIgnitionServerList();
				if(isl != null && isl.size() > 0) {
					Collections.shuffle(isl);
					StyloQApp.IgnitionServerEntry ise = isl.get(0);
					StyloQInterchange.DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(ise.SvcIdent);
					inner_param.RetrActivity_ = this;
					inner_param.AccsPoint = ise.Url;
					JSONObject js_query = new JSONObject();
					{
						StyloQCommand.Item fake_org_cmd = new StyloQCommand.Item();
						fake_org_cmd.Name = "search";
						fake_org_cmd.BaseCmdId = 0;
						inner_param.OriginalCmdItem = fake_org_cmd;
					}
					js_query.put("cmd", "search");
					js_query.put("plainquery", SearchPattern);
					js_query.put("maxresultcount", 128);
					inner_param.CommandJson = js_query.toString();
					StyloQInterchange.RunClientInterchange(app_ctx, inner_param);
					result = true;
				}
			} catch(JSONException exn) {
				;
			} catch(StyloQException exn) {
				;
			}
		}
		return result;
	}
	@Override public boolean onQueryTextChange(String newText)
	{
		return false;
	}
	private JSONObject SearchScope(String scope, String scopeIdent)
	{
		JSONObject result = null;
		if(JsScopeList != null && SLib.GetLen(scope) > 0 && SLib.GetLen(scopeIdent) > 0) {
			try {
				for(int i = 0; result == null && i < JsScopeList.length(); i++) {
					JSONObject item = JsScopeList.getJSONObject(i);
					if(item != null) {
						String s = item.optString("scope", "");
						if(s.equalsIgnoreCase(scope)) {
							String si = item.optString("scopeident", "");
							if(si.equalsIgnoreCase(scopeIdent))
								result = item;
						}
					}
				}
			} catch(JSONException exn) {
				;
			}
		}
		return result;
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					Intent intent = getIntent();
					setContentView(R.layout.activity_global_search);
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						View vg = findViewById(R.id.LAYOUT_ACTIVITYROOT);
						if(vg != null && vg instanceof ViewGroup)
							SLib.SubstituteStringSignatures(app_ctx, (ViewGroup) vg);
						SetupRecyclerListView(null, R.id.CTL_GLOBALSEARCH_RESULTLIST, R.layout.li_global_search);
						//
						View inpv = findViewById(R.id.CTL_GLOBALSEARCH_INPUT);
						if(inpv != null && inpv instanceof SearchView) {
							((SearchView)inpv).setOnQueryTextListener(this);
						}
						//
						String svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
						if(svc_reply_doc_json != null) {
							SetQueryResult(svc_reply_doc_json);
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				break;
			case SLib.EV_ACTIVITYSTART:
				{
					/* @doesnt-work
					View inpv = findViewById(R.id.CTL_GLOBALSEARCH_INPUT);
					if(inpv != null && inpv.requestFocus()) {
						InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
						imm.showSoftInput(inpv, InputMethodManager.SHOW_IMPLICIT);
					}*/
					CalledIndex = -1;
					CalledIndexFinished = -1;
					CallingStartTm = 0;
				}
				break;
			case SLib.EV_ACTIVITYRESUME:
				{
					CalledIndex = -1;
					CallingStartTm = 0;
					RefreshStatus();
					//CalledIndexFinished = -1;
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = (srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter)srcObj : null;
					int _count = 0;
					if(adapter != null) {
						if(adapter.GetRcId() == R.layout.li_global_search && JsResultList != null)
							_count = JsResultList.length();
					}
					result = new Integer(_count);
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder != null) {
							try {
								// RecyclerView
								if(JsResultList != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < JsResultList.length()) {
									View iv = ev_subj.RvHolder.itemView;
									JSONObject js_entry = JsResultList.getJSONObject(ev_subj.ItemIdx);
									String rtext = js_entry.optString("text", "");
									SLib.SetCtrlString(iv, R.id.LVITEM_SEARCH_TEXT, rtext);
									String scope_name = "";
									String key_s = js_entry.optString("scope", "");
									if(SLib.GetLen(key_s) > 0) {
										//String obj_type = js_entry.optString("objtype", "");
										/*if(key_s.equalsIgnoreCase("styloqsvc")) {
											scope_name = app_ctx.GetString("styloq_binderykind_foreignservice");
										}*/
										if(SLib.GetLen(scope_name) <= 0) {
											String key_si = js_entry.optString("scopeident", "");
											if(SLib.GetLen(key_si) > 0) {
												JSONObject js_scope = SearchScope(key_s, key_si);
												if(js_scope != null)
													scope_name = js_scope.optString("nm", "");
											}
										}
									}
									SLib.SetCtrlString(iv, R.id.LVITEM_SEARCH_SCOPE, scope_name);
									View tv_ind_et = iv.findViewById(R.id.LVITEM_SEARCH_IND_EXECUTETIME);
									ImageView iv_ind = (ImageView)iv.findViewById(R.id.LVITEM_SEARCH_IND_STATUS);
									if(CalledIndex == ev_subj.ItemIdx) {
										if(CallingStartTm > 0 && tv_ind_et != null) {
											final long now_ms = System.currentTimeMillis();
											long ellapsed_ms = now_ms - CallingStartTm;
											final int sec = (int)(ellapsed_ms / 1000);
											final int h = (int)(sec / 3600);
											String timewatch_text = ((h > 0) ? Integer.toString(h) + ":" : "") + String.format("%02d:%02d", (sec % 3600) / 60, (sec % 60));
											if(tv_ind_et.getVisibility() != View.VISIBLE)
												tv_ind_et.setVisibility(View.VISIBLE);
											SLib.SetCtrlString(iv, R.id.LVITEM_SEARCH_IND_EXECUTETIME, timewatch_text);
										}
										if(iv_ind != null) {
											if(iv_ind.getVisibility() != View.VISIBLE)
												iv_ind.setVisibility(View.VISIBLE);
											iv_ind.setImageResource(R.drawable.ic_stopwatch);
										}
									}
									else /*if(CalledIndexFinished == ev_subj.ItemIdx)*/ {
										if(tv_ind_et != null && tv_ind_et.getVisibility() == View.VISIBLE)
											tv_ind_et.setVisibility(View.GONE);
										if(iv_ind != null && iv_ind.getVisibility() == View.VISIBLE)
											iv_ind.setVisibility(View.GONE);
										//CalledIndexFinished = -1; // Один раз перерисовали в изначальном виде - больше не надо
									}
								}
							} catch(JSONException exn) {
								;
							}
						}
						else {
							// Если имеем дело с обычным ListView
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQApp app_ctx = GetAppCtx();
						if(CalledIndex < 0 && app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < JsResultList.length()) {
							try {
								JSONObject js_entry = JsResultList.getJSONObject(ev_subj.ItemIdx);
								if(js_entry != null) {
									String scope = js_entry.optString("scope", null);
									String scope_ident = js_entry.optString("scopeident", null);
									String obj_type = js_entry.optString("objtype", null);
									long obj_id = js_entry.optLong("objid", 0);
									if(SLib.GetLen(scope) > 0 && scope.equalsIgnoreCase("styloqsvc") && SLib.GetLen(scope_ident) > 0) {
										byte [] svc_ident = Base64.getDecoder().decode(scope_ident);
										if(SLib.GetLen(svc_ident) > 0 && SLib.GetLen(obj_type) > 0 && obj_id > 0) {
											StyloQInterchange.DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(svc_ident);
											JSONObject js_query = new JSONObject();
											/* @construction
											js_query.put("cmd", "onsrchr"); // on search result
											if(SLib.GetLen(obj_type) > 0)
												js_query.put("objtype", obj_type);
											if(obj_id > 0)
												js_query.put("objid", obj_id);
											 */
											{
												StyloQCommand.Item fake_org_cmd = new StyloQCommand.Item();
												fake_org_cmd.Name = "onsrchr";
												fake_org_cmd.BaseCmdId = 0;
												inner_param.OriginalCmdItem = fake_org_cmd;
											}
											js_query.put("cmd", /*"register"*/"onsrchr");
											js_query.put("time", System.currentTimeMillis());
											//if(SLib.GetLen(obj_type) > 0)
												js_query.put("objtype", obj_type);
											//if(obj_id > 0)
												js_query.put("objid", obj_id);
											inner_param.CommandJson = js_query.toString();
											inner_param.RetrActivity_ = this;
											StyloQInterchange.RunClientInterchange(app_ctx, inner_param);
											CalledIndex = ev_subj.ItemIdx;
											CallingStartTm = System.currentTimeMillis();
											RefreshStatus();
											ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
										}
									}
								}
							} catch(JSONException e) {
								;
							}
						}
					}
				}
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp app_ctx = GetAppCtx();
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult) subj;
					if(app_ctx != null && ir.OriginalCmdItem != null) {
						if(ir.OriginalCmdItem.Name.equalsIgnoreCase("search")) {
							CalledIndexFinished = -1;
							CalledIndex = -1;
							CallingStartTm = 0;
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								if(ir.InfoReply != null) {
									if(ir.InfoReply instanceof SecretTagPool) {
										JsSearchResult = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
										if(JsSearchResult != null) {
											JsScopeList = JsSearchResult.optJSONArray("scope_list");
											JsResultList = JsSearchResult.optJSONArray("result_list");
											View v = findViewById(R.id.CTL_GLOBALSEARCH_RESULTLIST);
											if(v != null && v instanceof RecyclerView) {
												RecyclerView rv = (RecyclerView)v;
												RecyclerView.Adapter a = rv.getAdapter();
												if(a != null)
													a.notifyDataSetChanged();
											}
										}
									}
								}
							}
							else {
								;
							}
						}
						else if(ir.OriginalCmdItem.Name.equalsIgnoreCase("onsrchr")) {
							CalledIndexFinished = CalledIndex;
							CalledIndex = -1;
							CallingStartTm = 0;
							ScheduleRTmr(null, 0, 0);
							RefreshStatus();
							if(ir.InfoReply != null) {
								if(ir.InfoReply instanceof SecretTagPool) {
									JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
									if(js_reply != null) {
										StyloQInterchange.CommonReplyResult crr = StyloQInterchange.GetReplyResult(js_reply);
										if(crr.Status > 0) {
											String disp_meth = js_reply.optString("displaymethod", "");
											Class intent_cls = null;
											if(disp_meth.equalsIgnoreCase("goodsinfo"))
												intent_cls = CmdRGoodsInfoActivity.class;
											else
												intent_cls = DetailActivity.class;
											Intent intent = new Intent(this, intent_cls);
											intent.putExtra("SvcIdent", ir.SvcIdent);
											intent.putExtra("SvcReplyDocJson", js_reply.toString());
											LaunchOtherActivity(intent);
										}
										else if(crr.Status == 0) {

										}
									}
								}
								else if(ir.InfoReply instanceof String) {
									if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
										app_ctx.DisplayMessage(this, (String)ir.InfoReply, 0);
									}
									else if(ir.ResultTag == StyloQApp.SvcQueryResult.ERROR) {
										app_ctx.DisplayError(this, (String)ir.InfoReply, 0);
									}
									else if(ir.ResultTag == StyloQApp.SvcQueryResult.EXCEPTION) {
										app_ctx.DisplayError(this, (String)ir.InfoReply, 0);
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
	public void SetQueryResult(String svcDocJson) // @toremove
	{
		if(SLib.GetLen(svcDocJson) > 0) {
			try {
				JsSearchResult = new JSONObject(svcDocJson);
				JsScopeList = JsSearchResult.optJSONArray("scope_list");
				JsResultList = JsSearchResult.optJSONArray("result_list");
			} catch(JSONException exn) {
				JsSearchResult = null;
				JsScopeList = null;
				JsResultList = null;
			}
			View v = findViewById(R.id.CTL_GLOBALSEARCH_RESULTLIST);
			if(v != null && v instanceof RecyclerView) {
				RecyclerView rv = (RecyclerView)v;
				RecyclerView.Adapter a = rv.getAdapter();
				if(a != null)
					a.notifyDataSetChanged();
			}
		}
	}
}