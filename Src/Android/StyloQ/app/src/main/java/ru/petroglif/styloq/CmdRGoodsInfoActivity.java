// CmdRGoodsInfoActivity.java
// Copyright (c) A.Sobolev 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;

import androidx.fragment.app.Fragment;
import androidx.viewpager2.widget.ViewPager2;

import com.google.android.material.tabs.TabLayout;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class CmdRGoodsInfoActivity extends SLib.SlActivity {
	private byte [] SvcIdent; // Получает через intent ("SvcIdent")
	private String CmdName; // Получает через intent ("CmdName")
	private String CmdDescr; // Получает через intent ("CmdDescr")
	private JSONObject Data;
	private String BaseCurrencySymb; // Извлекается из Data ("basecurrency")
	private StyloQDatabase.SecStoragePacket SvcPack = null;

	public CmdRGoodsInfoActivity()
	{
		SvcIdent = null;
		CmdName = null;
		CmdDescr = null;
		Data = null;
		BaseCurrencySymb = null;
	}
	private class TabEntry {
		TabEntry(int id, String text, /*View*/androidx.fragment.app.Fragment view)
		{
			TabId = id;
			TabText = text;
			TabView = view;
		}
		int TabId;
		String TabText;
		/*View*/ androidx.fragment.app.Fragment TabView;
	}
	private ArrayList<TabEntry> TabList;
	private void CreateTabList()
	{
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null && TabList == null) {
			TabList = new ArrayList<TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			{
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(R.layout.layout_goods_basic, R.id.TABLAYOUT_GOODSINFO);
				TabList.add(new TabEntry(1, SLib.ExpandString(app_ctx, "@{general}"), (Fragment)f));
			}
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.activity_cmdrgoodsinfo);
					try {
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
									SvcPack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
									//
									String blob_signature = null;
									if(SvcPack != null) {
										StyloQFace face = SvcPack.GetFace();
										if(face != null)
											blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
										SLib.SetCtrlString(this, R.id.CTL_PAGEHEADER_SVCTITLE, SvcPack.GetSvcName(face));
										StyloQDatabase.SecStoragePacket cmdl_pack = db.GetForeignSvcCommandList(SvcIdent);
									}
									SLib.SetupImage(this, findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature, false);
								}
								/*
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
								 */
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
								Data = new JSONObject(svc_reply_doc_json);
								BaseCurrencySymb = Data.optString("basecurrency", null);
							}
							//
							ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_GOODSINFO);
							SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_GOODSINFO);
							{
								TabLayout lo_tab = findViewById(R.id.TABLAYOUT_GOODSINFO);
								if(lo_tab != null) {
									CreateTabList();
									for(int i = 0; i < TabList.size(); i++) {
										TabLayout.Tab tab = lo_tab.newTab();
										tab.setText(TabList.get(i).TabText);
										lo_tab.addTab(tab);
										//TabLayout.TabView;
									}
									SLib.SetupTabLayoutStyle(lo_tab);
									SLib.SetupTabLayoutListener(this, lo_tab, view_pager);
								}
							}
							{
								View vg = findViewById(R.id.LAYOUT_ACTIVITYROOT);
								if(vg != null && vg instanceof ViewGroup)
									SLib.SubstituteStringSignatures(app_ctx, (ViewGroup) vg);
							}
						}
					} catch(JSONException exn) {
						//
					} catch(StyloQException exn) {
						//exn.printStackTrace();
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				CreateTabList();
				result = new Integer(TabList.size());
				break;
			case SLib.EV_CREATEFRAGMENT:
				if(subj instanceof Integer) {
					int item_idx = (Integer)subj;
					if(TabList != null && item_idx >= 0 && item_idx < TabList.size()) {
						TabEntry cur_entry = (TabEntry)TabList.get(item_idx);
						result = cur_entry.TabView;
					}
				}
				break;
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					ViewGroup vg = (ViewGroup)srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_GOODS_BASIC) {
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null && Data != null) {
							View vroot = findViewById(R.id.LAYOUT_ACTIVITYROOT);
							if(vroot != null && vroot instanceof ViewGroup) {
								//LayoutInflater inflater = this.getLayoutInflater();
								//View vchild = inflater.inflate(R.layout.layout_detail_ware, (ViewGroup) vroot, false);
								JSONObject js_detail = Data.optJSONObject("detail");
								if(js_detail != null) {
									View vimg = findViewById(R.id.CTL_GOODSDETAIL_IMG);
									if(vimg != null)
										SLib.SetupImage(this, vimg, js_detail.optString("imgblobs", null), false);
									{
										final String text = js_detail.optString("nm", null);
										final int ctl_id = R.id.CTL_GOODSDETAIL_NAME;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_NAME;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										final String text = js_detail.optString("brandnm", null);
										final int ctl_id = R.id.CTL_GOODSDETAIL_BRAND;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_BRAND;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										final String text = js_detail.optString("parnm", null);
										final int ctl_id = R.id.CTL_GOODSDETAIL_GROUP;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_GROUP;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										final String text = js_detail.optString("manufnm", null);
										final int ctl_id = R.id.CTL_GOODSDETAIL_MANUF;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_MANUF;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										final double value = js_detail.optDouble("price", 0.0);
										final String text = (value > 0.0) ? SLib.FormatCurrency(value, BaseCurrencySymb) : null;
										final int ctl_id = R.id.CTL_GOODSDETAIL_PRICE;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_PRICE;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										final double value = js_detail.optDouble("stock", 0.0);
										final String text = (value > 0.0) ? SLib.formatdouble(value, 3) : null;
										final int ctl_id = R.id.CTL_GOODSDETAIL_REST;
										final int ctlgrp_id = R.id.CTLGRP_GOODSDETAIL_REST;
										if(SLib.GetLen(text) > 0)
											SLib.SetCtrlString(this, ctl_id, text);
										else
											SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
									}
									{
										JSONArray exttext_js_array = js_detail.optJSONArray("exttext_list");
										final int[] exttext_ctl_ids = {R.id.CTL_GOODSDETAIL_EXTTEXT1, R.id.CTL_GOODSDETAIL_EXTTEXT2,
												R.id.CTL_GOODSDETAIL_EXTTEXT3, R.id.CTL_GOODSDETAIL_EXTTEXT4, R.id.CTL_GOODSDETAIL_EXTTEXT5};
										final int[] exttext_ctllbl_ids = {R.id.CTLLBL_GOODSDETAIL_EXTTEXT1, R.id.CTLLBL_GOODSDETAIL_EXTTEXT2,
												R.id.CTLLBL_GOODSDETAIL_EXTTEXT3, R.id.CTLLBL_GOODSDETAIL_EXTTEXT4, R.id.CTLLBL_GOODSDETAIL_EXTTEXT5};
										final int[] exttext_ctlgrp_ids = {R.id.CTLGRP_GOODSDETAIL_EXTTEXT1, R.id.CTLGRP_GOODSDETAIL_EXTTEXT2,
												R.id.CTLGRP_GOODSDETAIL_EXTTEXT3, R.id.CTLGRP_GOODSDETAIL_EXTTEXT4, R.id.CTLGRP_GOODSDETAIL_EXTTEXT5};
										for(int i = 0; i < exttext_ctl_ids.length; i++) {
											final int ctlgrp_id = exttext_ctlgrp_ids[i];
											if(exttext_js_array != null && i < exttext_js_array.length()) {
												JSONObject exttext_js_entry = exttext_js_array.optJSONObject(i);
												if(exttext_js_entry != null) {
													final String text = exttext_js_entry.optString("text", null);
													final int ctl_id = exttext_ctl_ids[i];
													final int ctllbl_id = exttext_ctllbl_ids[i];
													if(SLib.GetLen(text) > 0) {
														final String descr = exttext_js_entry.optString("title", null);
														SLib.SetCtrlString(this, ctllbl_id, descr);
														SLib.SetCtrlString(this, ctl_id, text);
													}
													else
														SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
												}
											}
											else
												SLib.SetCtrlVisibility(this, ctlgrp_id, View.GONE);
										}
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
}