// GlobalSearchActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SearchView;

import androidx.recyclerview.widget.RecyclerView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Base64;
import java.util.Collections;

public class GlobalSearchActivity extends SLib.SlActivity implements SearchView.OnQueryTextListener {
	private String SearchPattern;
	private JSONObject JsSearchResult;
	private JSONArray JsScopeList;
	private JSONArray JsResultList;
	public GlobalSearchActivity()
	{
		super();
		SearchPattern = null;
		JsSearchResult = null;
	}
	@Override public boolean onQueryTextSubmit(String query)
	{
		boolean result = false;
		SearchPattern = query;
		if(SLib.GetLen(SearchPattern) > 0) {
			StyloQApp app_ctx = (StyloQApp)getApplication();
			ArrayList <StyloQApp.IgnitionServerEntry> isl = app_ctx.GetIgnitionServerList();
			if(isl != null && isl.size() > 0) {
				Collections.shuffle(isl);
				StyloQApp.IgnitionServerEntry ise = isl.get(0);
				StyloQInterchange.DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(ise.SvcIdent);
				inner_param.AccsPoint = ise.Url;
				JSONObject js_query = new JSONObject();
				try {
					js_query.put("cmd", "search");
					js_query.put("plainquery", SearchPattern);
					js_query.put("maxresultcount", 128);
					inner_param.CommandJson = js_query.toString();
					StyloQInterchange.RunClientInterchange(app_ctx, inner_param);
					result = true;
				} catch(JSONException exn) {
					;
				}
			}
		}
		return false;
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
	public void SetQueryResult(String svcDocJson)
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
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					Intent intent = getIntent();
					setContentView(R.layout.activity_global_search);
					StyloQApp app_ctx = (StyloQApp)getApplicationContext();
					View vg = findViewById(R.id.LAYOUT_ACTIVITYROOT);
					if(vg != null && vg instanceof ViewGroup)
						SLib.SubstituteStringSignatures(app_ctx, (ViewGroup)vg);
					SetupRecyclerListView(null, R.id.CTL_GLOBALSEARCH_RESULTLIST, R.layout.li_global_search);
					//
					View inpv = findViewById(R.id.CTL_GLOBALSEARCH_INPUT);
					if(inpv != null && inpv instanceof SearchView)
						((SearchView)inpv).setOnQueryTextListener(this);
					//
					String svc_reply_doc_json = intent.getStringExtra("SvcReplyDocJson");
					if(svc_reply_doc_json != null) {
						SetQueryResult(svc_reply_doc_json);
					}
				}
				break;
			case SLib.EV_ACTIVITYSTART:
				{
					/* @doesnt-work
					View inpv = findViewById(R.id.CTL_GLOBALSEARCH_INPUT);
					if(inpv != null && inpv.requestFocus()) {
						InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
						imm.showSoftInput(inpv, InputMethodManager.SHOW_IMPLICIT);
					}*/
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
									StyloQApp app_ctx = (StyloQApp)getApplicationContext();
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
						StyloQApp app_ctx = (StyloQApp)getApplication();
						if(app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < JsResultList.length()) {
							try {
								JSONObject js_entry = JsResultList.getJSONObject(ev_subj.ItemIdx);
								if(js_entry != null) {
									String scope = js_entry.optString("scope", null);
									String scope_ident = js_entry.optString("scopeident", null);
									String obj_type = js_entry.optString("objtype", null);
									long obj_id = js_entry.optLong("objid", 0);
									if(SLib.GetLen(scope) > 0 && scope.equalsIgnoreCase("styloqsvc") && SLib.GetLen(scope_ident) > 0) {
										byte [] svc_ident = Base64.getDecoder().decode(scope_ident);
										if(SLib.GetLen(svc_ident) > 0) {
											StyloQInterchange.DoInterchangeParam inner_param = new StyloQInterchange.DoInterchangeParam(svc_ident);
											JSONObject js_query = new JSONObject();
											js_query.put("cmd", "register");
											js_query.put("time", System.currentTimeMillis());
											inner_param.CommandJson = js_query.toString();
											StyloQInterchange.RunClientInterchange(app_ctx, inner_param);
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
		}
		return result;
	}
}