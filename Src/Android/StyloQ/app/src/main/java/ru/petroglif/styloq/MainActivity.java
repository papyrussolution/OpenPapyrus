// MainActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.recyclerview.widget.RecyclerView;

import com.google.zxing.client.android.Intents;
import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import org.json.JSONException;
import org.json.JSONObject;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Calendar;
import java.util.Timer;
import java.util.TimerTask;

public class MainActivity extends SLib.SlActivity/*AppCompatActivity*/ {
	static class PrivateConfigDialog extends SLib.SlDialog {
		PrivateConfigDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_PRIVCFG, data);
			if(data instanceof StyloQConfig)
				Data = data;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					{
						requestWindowFeature(Window.FEATURE_NO_TITLE);
						setContentView(R.layout.dialog_privateconfig);
						Context ctx = getContext();
						StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
						if(app_ctx != null)
							setTitle(SLib.ExpandString(app_ctx, "@{styloqprivconfig}"));
						SetDTS(Data);
					}
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							Context ctx = getContext();
							StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON)
						this.dismiss(); // Close Dialog
					break;
			}
			return null;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			try {
				Context ctx = getContext();
				StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
				int    pref_lang_id = 0;
				int    def_face_id = 0;
				if(app_ctx != null) {
					//
					StyloQConfig _data = null;
					if(Data != null && Data.getClass().getSimpleName().equals("StyloQConfig"))
						_data = (StyloQConfig)Data;
					else {
						_data = new StyloQConfig();
						Data = _data;
					}
					//
					String pref_lang_ref = _data.Get(StyloQConfig.tagPrefLanguage);
					String face_hex = _data.Get(StyloQConfig.tagDefFace);
					byte [] def_face_ref = (SLib.GetLen(face_hex) > 0) ? Base64.getDecoder().decode(face_hex) : null;
					//
					SLib.StrAssocArray face_list = new SLib.StrAssocArray();
					SLib.StrAssocArray lang_list = new SLib.StrAssocArray();
					ArrayList <StyloQFace> raw_face_list = app_ctx.Db.GetFaceList();
					String undef_text = SLib.ExpandString(app_ctx, "@{undefined_neut}");
					face_list.Set(0, undef_text);
					for(int i = 0; i < raw_face_list.size(); i++) {
						StyloQFace raw_face_item = raw_face_list.get(i);
						if(SLib.GetLen(def_face_ref) > 0 && SLib.AreByteArraysEqual(def_face_ref, raw_face_item.BI))
							def_face_id = (int)raw_face_item.ID;
						face_list.Set((int)raw_face_item.ID, raw_face_item.GetSimpleText(0));
					}
					{
						int lang_id_list[] = {SLib.slangRU, SLib.slangEN,
								SLib.slangDE, SLib.slangNL, SLib.slangPT, SLib.slangES};
						lang_list.Set(0, undef_text);
						for(int i = 0; i < lang_id_list.length; i++) {
							String lang_code = SLib.GetLinguaCode(lang_id_list[i]);
							if(SLib.GetLen(pref_lang_ref) > 0 && pref_lang_ref.equalsIgnoreCase(lang_code))
								pref_lang_id = lang_id_list[i];
							lang_list.Set(lang_id_list[i], lang_code);
						}
					}
					//
					SLib.SetupStrAssocCombo(app_ctx, this, R.id.CTLSEL_PRIVCFG_PREFLANG, lang_list, pref_lang_id);
					SLib.SetupStrAssocCombo(app_ctx, this, R.id.CTLSEL_PRIVCFG_DEFFACE, face_list, def_face_id);
				}
			} catch(StyloQException exn) {
				//e.printStackTrace();
				ok = false;
			}
			return ok;
		}
		Object GetDTS()
		{
			Object result = null;
			Context ctx = getContext();
			StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
			if(app_ctx != null) {
				StyloQConfig _data = null;
				try {
					if(Data != null && Data.getClass().getSimpleName().equals("SecStoragePacket"))
						_data = (StyloQConfig)Data;
					else {
						_data = new StyloQConfig();
						Data = _data;
					}
					int lang_id = SLib.GetStrAssocComboData(this, R.id.CTLSEL_PRIVCFG_PREFLANG);
					int face_id = SLib.GetStrAssocComboData(this, R.id.CTLSEL_PRIVCFG_DEFFACE);
					//
					if(face_id > 0) {
						ArrayList<StyloQFace> raw_face_list = (face_id > 0) ? app_ctx.Db.GetFaceList() : null;
						if(raw_face_list != null) {
							for(int i = 0; i < raw_face_list.size(); i++) {
								StyloQFace raw_face_item = raw_face_list.get(i);
								if(raw_face_item.ID == face_id && SLib.GetLen(raw_face_item.BI) > 0) {
									String face_hex = Base64.getEncoder().encodeToString(raw_face_item.BI);
									_data.Set(StyloQConfig.tagDefFace, face_hex);
									break;
								}
							}
						}
					}
					else
						_data.Set(StyloQConfig.tagDefFace, null);
					if(lang_id > 0)
						_data.Set(StyloQConfig.tagPrefLanguage, SLib.GetLinguaCode(lang_id));
					else
						_data.Set(StyloQConfig.tagPrefLanguage, null);
					result = Data;
				} catch(StyloQException exn) {
					//e.printStackTrace();
					result = null;
				}
			}
			return result;
		}
	}
	static class SvcInfoDialog extends SLib.SlDialog {
		SvcInfoDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_STQSERVICE, data);
			if(data instanceof StyloQDatabase.SecStoragePacket)
				Data = data;
			//ResultListener = resultListener;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_service_info);
					SetDTS(Data);
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON || view_id == R.id.STDCTL_CLOSEBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							Context ctx = getContext();
							StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_DELETEBUTTON) {
						if(Data != null && Data instanceof StyloQDatabase.SecStoragePacket) {
							Context ctx = getContext();
							StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
							if(app_ctx != null) {
								try {
									StyloQDatabase db = app_ctx.GetDB();
									db.DeleteForeignSvc(((StyloQDatabase.SecStoragePacket)Data).Rec.ID);
									app_ctx.HandleEvent(SLib.EV_IADATADELETECOMMIT, this, Data);
									this.dismiss(); // Close Dialog
								} catch(StyloQException exn) {
									;
								}
							}
						}
					}
					/*else if(view_id == R.id.STDCTL_CLOSEBUTTON) {
						this.dismiss();
					}*/
					break;
			}
			return result;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			try {
				if(objData != null && objData.getClass() == Data.getClass()) {
					Context ctx = getContext();
					StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
					if(app_ctx != null) {
						StyloQDatabase.SecStoragePacket _data = null;
						if(Data != null && Data instanceof StyloQDatabase.SecStoragePacket)
							_data = (StyloQDatabase.SecStoragePacket)Data;
						else {
							_data = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kForeignService);
							Data = _data;
						}
						String text;
						SLib.SetCtrlString(this, R.id.CTL_STQSERVICE_CN, _data.GetSvcName(null));
						byte [] face_ref = _data.Pool.Get(SecretTagPool.tagAssignedFaceRef);
						int    face_ref_id = 0;
						SLib.StrAssocArray face_list = new SLib.StrAssocArray();
						ArrayList <StyloQFace> raw_face_list = app_ctx.Db.GetFaceList();
						face_list.Set(0, SLib.ExpandString(app_ctx, "@{undefined_neut}"));
						for(int i = 0; i < raw_face_list.size(); i++) {
							StyloQFace raw_face_item = raw_face_list.get(i);
							if(SLib.GetLen(face_ref) > 0 && SLib.AreByteArraysEqual(face_ref, raw_face_item.BI))
								face_ref_id = (int)raw_face_item.ID;
							face_list.Set((int)raw_face_item.ID, raw_face_item.GetSimpleText(0));
						}
						SLib.SetupStrAssocCombo(app_ctx, this, R.id.CTLSEL_STQSERVICE_PREFFACE, face_list, face_ref_id);
					}
				}
			} catch(StyloQException exn) {
				//e.printStackTrace();
				ok = false;
			}
			return ok;
		}
		Object GetDTS()
		{
			Object result = null;
			Context ctx = getContext();
			StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
			if(app_ctx != null) {
				StyloQDatabase.SecStoragePacket _data = null;
				try {
					if(Data != null && Data instanceof StyloQDatabase.SecStoragePacket)
						_data = (StyloQDatabase.SecStoragePacket)Data;
					else {
						_data = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kForeignService);
						Data = _data;
					}
					int face_ref_id = SLib.GetStrAssocComboData(this, R.id.CTLSEL_STQSERVICE_PREFFACE);
					ArrayList <StyloQFace> raw_face_list = (face_ref_id > 0) ? app_ctx.Db.GetFaceList() : null;
					if(raw_face_list != null) {
						for(int i = 0; i < raw_face_list.size(); i++) {
							StyloQFace raw_face_item = raw_face_list.get(i);
							if(raw_face_item.ID == face_ref_id && SLib.GetLen(raw_face_item.BI) > 0) {
								_data.Pool.Put(SecretTagPool.tagAssignedFaceRef, raw_face_item.BI);
								break;
							}
						}
					}
					result = Data;
				} catch(StyloQException exn) {
					result = null;
				}
			}
			return result;
		}
	}
	public final int CUSTOMIZED_REQUEST_CODE = 0x0000ffff;
	private ArrayList <Long> SvcListData;
	private int TouchedListItemIdx; // Элемент, на который нажали пальцем. Для временного изменения окраски.
	public MainActivity()
	{
		super();
		SvcListData = null;
		TouchedListItemIdx = -1;
	}
	private class ResetTouchedListItemIdx_TimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { SetTouchedItemIndex(-1); }}); }
	}
	private boolean NotifyListItemChanged(int idx)
	{
		boolean result = false;
		View v = findViewById(R.id.serviceListView);
		if(v != null && v instanceof RecyclerView) {
			RecyclerView view = (RecyclerView) v;
			RecyclerView.Adapter adapter = view.getAdapter();
			if(adapter != null && idx < adapter.getItemCount()) {
				adapter.notifyItemChanged(idx);
				result = true;
			}
		}
		return result;
	}
	private void SetTouchedItemIndex(int idx)
	{
		if(idx >= 0) {
			TouchedListItemIdx = idx;
			if(NotifyListItemChanged(idx)) {
				Timer tmr = new Timer();
				tmr.schedule(new ResetTouchedListItemIdx_TimerTask(), 2000);
			}
		}
		else {
			final int _idx = TouchedListItemIdx;
			TouchedListItemIdx = -1;
			NotifyListItemChanged(_idx);
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					setContentView(R.layout.activity_main);
					SetupRecyclerListView(null, R.id.serviceListView, R.layout.li_service);
					{
						StyloQApp app_ctx = (StyloQApp)getApplication();
						if(app_ctx != null) {
							try {
								StyloQDatabase db = app_ctx.GetDB();
								if(db != null) {
									db.SetupPeerInstance();
									SvcListData = db.GetForeignSvcIdList(true);
								}
							} catch(StyloQException e) {
								;
							}
						}
					}
				}
				break;
			case SLib.EV_ACTIVITYRESUME:
				{
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(app_ctx != null) {
						try {
							StyloQDatabase db = app_ctx.GetDB();
							if(db != null) {
								SvcListData = db.GetForeignSvcIdList(true);
								View v = findViewById(R.id.serviceListView);
								RecyclerView view = (v != null && v instanceof RecyclerView) ? (RecyclerView)findViewById(R.id.serviceListView) : null;
								RecyclerView.Adapter adapter = (view != null) ? view.getAdapter() : null;
								if(adapter != null)
									adapter.notifyDataSetChanged();
							}
						} catch(StyloQException e) {
							;
						}
					}
				}
				break;
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.buttonInfo);
						result = ev_subj.RvHolder;
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = (srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter)srcObj : null;
					int _count = 0;
					if(adapter != null && adapter.GetRcId() == R.layout.li_service && SvcListData != null) {
						_count = SvcListData.size();
					}
					result = new Integer(_count);
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(ev_subj != null && app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < SvcListData.size()) {
						long svc_id = SvcListData.get(ev_subj.ItemIdx);
						if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.buttonInfo) {
							// Здесь показать информацию о сервисе
							StyloQDatabase.SecStoragePacket svc_pack = null;
							try {
								StyloQDatabase db = app_ctx.GetDB();
								if(db != null)
									svc_pack = db.GetPeerEntry(svc_id);
							} catch(StyloQException exn) {
								;
							}
							if(svc_pack != null) {
								SvcInfoDialog dialog = new SvcInfoDialog(this, svc_pack);
								dialog.show();
							}
						}
						else {
							SetTouchedItemIndex(ev_subj.ItemIdx);
							try {
								StyloQDatabase db = app_ctx.GetDB();
								if(db != null) {
									StyloQDatabase.SecStoragePacket cur_entry = db.GetPeerEntry(svc_id);
									if(cur_entry != null)
										app_ctx.GetSvcCommandList(cur_entry, false);
								}
							} catch(StyloQException exn) {
								;
							}
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMLONGCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(ev_subj != null && app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < SvcListData.size()) {
						SetTouchedItemIndex(ev_subj.ItemIdx);
						long svc_id = SvcListData.get(ev_subj.ItemIdx);
						try {
							StyloQDatabase db = app_ctx.GetDB();
							if(db != null) {
								StyloQDatabase.SecStoragePacket cur_entry = db.GetPeerEntry(svc_id);
								if(cur_entry != null)
									app_ctx.GetSvcCommandList(cur_entry, true);
							}
						} catch(StyloQException exn) {
							;
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
							if(SvcListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < SvcListData.size()) {
								long cur_id = SvcListData.get(ev_subj.ItemIdx);
								StyloQApp app_ctx = (StyloQApp) getApplication();
								if(app_ctx != null) {
									try {
										StyloQDatabase db = app_ctx.GetDB();
										if(db != null) {
											StyloQDatabase.SecStoragePacket cur_entry = db.GetPeerEntry(cur_id);
											if(cur_entry != null) {
												View iv = ev_subj.RvHolder.itemView;
												if(TouchedListItemIdx == ev_subj.ItemIdx)
													iv.setBackgroundResource(R.drawable.shape_listitem_focused);
												else
													iv.setBackgroundResource(R.drawable.shape_listitem);
												TextView ctl = (TextView)iv.findViewById(R.id.LVITEM_SVCNAME);
												StyloQFace face = cur_entry.GetFace();
												if(face != null) { // @v11.4.0 @fix
													if(ctl != null)
														ctl.setText(cur_entry.GetSvcName(face));
													View img_view = iv.findViewById(R.id.LVITEM_IMG);
													if(img_view != null && img_view instanceof ImageView) {
														String blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
														SLib.SetupImage(this, img_view, blob_signature);
													}
												}
											}
										}
									} catch(StyloQException exn) {
										;
									}
								}
							}
						}
						else {
							// Если имеем дело с обычным ListView
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.tbButtonScan) {
					/* @debug {
						String test_inv = "A3amEJbWcO3PTpuxcgQ2UoHvXoA0=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJSRUdJU1RFUiJ9";
						StyloQInterchange.Invitation inv = StyloQInterchange.AcceptInvitation(test_inv);
						TextView v_info = (TextView)findViewById(R.id.info_text);
						if(inv != null) {
							String info_buf = null;
							info_buf = "SvcIdent: " + android.util.Base64.encodeToString(inv.SvcIdent, 0) + "\n" +
									"Capabilities: " + Integer.toString(inv.Capabilities) + "\n" +
									"Access Point: " + inv.AccessPoint + "\n" +
									"Command: " + inv.CommandJson;
							v_info.setText(info_buf);
						}
						else
							v_info.setText("Error parsing of invatation");
					}*/
					IntentIntegrator integrator = new IntentIntegrator(this); // `this` is the current Activity
					//integrator.setPrompt("Scan a barcode");
					integrator.setCameraId(0);  // Use a specific camera of the device
					integrator.setOrientationLocked(true);
					integrator.setBeepEnabled(true);
					integrator.setCaptureActivity(StyloQZxCaptureActivity.class);
					integrator.initiateScan();
				}
				else if(view_id == R.id.tbButtonConfig) {
					StyloQApp app_ctx = (StyloQApp)getApplication();
					try {
						StyloQDatabase db = app_ctx.GetDB();
						StyloQConfig _data = new StyloQConfig();
						StyloQDatabase.SecStoragePacket pack = db.GetOwnPeerEntry();
						if(pack != null) {
							byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
							if(SLib.GetLen(cfg_bytes) > 0) {
								String cfg_json = new String(cfg_bytes);
								_data.FromJson(cfg_json);
							}
							PrivateConfigDialog dialog = new PrivateConfigDialog(this, _data);
							dialog.show();
						}
					} catch(StyloQException exn) {
					}
				}
				else if(view_id == R.id.tbButtonFaces) {
					Intent intent = new Intent(this, FaceListActivity.class);
					startActivity(intent);
				}
				else if(view_id == R.id.tbButtonSearch) {
					Intent intent = new Intent(this, GlobalSearchActivity.class);
					startActivity(intent);
				}
				/*else if(view_id == R.id.tbButtonTest) {
					StyloQApp app_ctx = (StyloQApp)getApplication();
					try {
						// (it works!) ctx.TestDisplaySnackbar(v_info, "Тестовый текст для snackbar'а", -2);
						TestScan();
						if(false) {
							//
							// Отладка StrStore
							//
							InputStream stream = getResources().openRawResource(R.raw.ppstr2);
							String test_str_result;
							if(stream != null) {
								StrStore sst = new StrStore();
								if(sst.Load(stream)) {
									test_str_result = sst.GetString(app_ctx.GetCurrentLang(), "databaserecover");
									test_str_result = sst.GetString(app_ctx.GetCurrentLang(), ppstr2.PPSTR_ERROR, ppstr2.PPERR_SMS_PASSWNEEDED);
									test_str_result = sst.GetString(app_ctx.GetCurrentLang(), ppstr2.PPSTR_INFORMATION, ppstr2.PPINF_RECOVERSUCCESS);
								}
							}
						}
						if(false) {
							//View status_view = findViewById(R.id.info_text);
							//ctx.DisplayError(null, "Тест сообщения об ошибке", 60);
							app_ctx.DisplayMessage(null, ppstr2.PPSTR_TEXT, ppstr2.PPTXT_UNDEFIMPGOODSNAME, 5000);
						}
						if(false) {
							SRP.Test();
							//
							{
								StyloQInterchange ic = new StyloQInterchange(app_ctx);
								ic.TestAmq();
							}
							{
								if(testDatabase()) {
									//{
									//	StyloQDatabase dbs = ((StyloQApp)getApplication()).GetDB();
									//	dbs.DropTable("SecTable");
									//	long own_peer_id = dbs.SetupPeerInstance();
									//	if(own_peer_id > 0) {
									//		v_info.setText(":) It seems everything is OK");
									//	}
									//	else
									//		v_info.setText(":( SetupPeerInstance test failed");
									//}
								}
							}
						}
					} //catch(NoSuchAlgorithmException e) {
						//e.printStackTrace();
						//v_info.setText(e.toString());
					//} catch(IllegalStateException e) {
					//v_info.setText(e.toString());
					//}
					catch(StyloQException exn) {
						exn.printStackTrace();
					}
				}*/
				break;
			case SLib.EV_IADATADELETECOMMIT:
				if(srcObj != null && subj != null) {
					StyloQApp app_ctx = (StyloQApp)getApplication();
					try {
						StyloQDatabase db = app_ctx.GetDB();
						if(srcObj instanceof MainActivity.SvcInfoDialog && subj instanceof StyloQDatabase.SecStoragePacket) {
							//
							// Ответ на закрытие диалога информации о сервисе по факту удаления объекта
							//
							StyloQDatabase.SecStoragePacket pack = (StyloQDatabase.SecStoragePacket)subj;
							ReckonServiceEntryDeleted(pack.Rec.ID);
						}
					} catch(StyloQException exn) {
						;
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				if(srcObj != null && subj != null) {
					StyloQApp app_ctx = (StyloQApp)getApplication();
					try {
						StyloQDatabase db = app_ctx.GetDB();
						if(srcObj instanceof MainActivity.SvcInfoDialog && subj instanceof StyloQDatabase.SecStoragePacket) {
							//
							// Ответ на закрытие диалога информации о сервисе
							// Пока там можно изменить только привязку лика к сервису
							//
							StyloQDatabase.SecStoragePacket pack = (StyloQDatabase.SecStoragePacket) subj;
							StyloQDatabase.SecStoragePacket org_pack = db.GetPeerEntry(pack.Rec.ID);
							if(org_pack != null) {
								byte[] org_face_ref = org_pack.Pool.Get(SecretTagPool.tagAssignedFaceRef);
								byte[] face_ref = pack.Pool.Get(SecretTagPool.tagAssignedFaceRef);
								if(!SLib.AreByteArraysEqual(org_face_ref, face_ref)) {
									org_pack.Pool.Put(SecretTagPool.tagAssignedFaceRef, face_ref);
									db.PutPeerEntry(org_pack.Rec.ID, org_pack, true);
									ReckonServiceEntryCreatedOrUpdated(org_pack.Rec.ID);
								}
							}
						}
						else if(srcObj instanceof MainActivity.PrivateConfigDialog && subj instanceof StyloQConfig) {
							//
							// Ответ на закрытие диалога локальной конфигурации
							//
							StyloQConfig pack = (StyloQConfig)subj;
							StyloQDatabase.SecStoragePacket own_pack = db.GetOwnPeerEntry();
							if(own_pack != null) {
								String cfg_json = pack.ToJson();
								if(cfg_json != null) {
									byte [] cfg_bytes = cfg_json.getBytes();
									if(SLib.GetLen(cfg_bytes) > 0) {
										own_pack.Pool.Put(SecretTagPool.tagPrivateConfig, cfg_bytes);
										if(db.PutPeerEntry(own_pack.Rec.ID, own_pack, true) > 0) {
											String pref_lang_ref = pack.Get(StyloQConfig.tagPrefLanguage);
											int pl = SLib.GetLinguaIdent(pref_lang_ref);
											app_ctx.SetCurrentLang(pl);
										}
									}
								}
							}
						}
					} catch(StyloQException exn){
						//exn.printStackTrace();
					}
				}
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(app_ctx != null) {
						StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult) subj;
						String reply_msg = null;
						String reply_errmsg = null;
						if(ir.OriginalCmdItem != null) {
							if(ir.OriginalCmdItem.BaseCmdId == StyloQCommand.sqbcDtLogin) {
								boolean login_result = false;
								if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
									SecretTagPool rpool = (SecretTagPool) ir.InfoReply;
									byte[] raw_reply_bytes = rpool.Get(SecretTagPool.tagRawData);
									if(SLib.GetLen(raw_reply_bytes) > 0) {
										String reply_js_text = new String(raw_reply_bytes);
										if(SLib.GetLen(reply_js_text) > 0) {
											try {
												JSONObject js_reply = new JSONObject(reply_js_text);
												if(js_reply != null) {
													int repl_result = StyloQInterchange.GetReplyResult(js_reply);
													if(repl_result > 0)
														login_result = true;
													reply_msg = js_reply.optString("msg", null);
													reply_errmsg = js_reply.optString("errmsg", null);
												}
											} catch(JSONException exn) {
												;
											}
										}
									}
								}
								if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS && login_result) {
									if(SLib.GetLen(reply_msg) <= 0) {
										if(SLib.GetLen(reply_errmsg) > 0)
											reply_msg = reply_errmsg;
										else
											reply_msg = "OK";
									}
									app_ctx.DisplayMessage(this, reply_msg, 0);
								}
								else {
									if(SLib.GetLen(reply_errmsg) <= 0) {
										if(SLib.GetLen(reply_msg) > 0)
											reply_errmsg = reply_msg;
										else
											reply_errmsg = "ERROR";
									}
									app_ctx.DisplayError(this, reply_errmsg, 0);
								}
							}
						}
					}
				}
				break;
		}
		return result;
	}
	private int FindServiceEntryInListData(long svcId)
	{
		if(SvcListData != null) {
			for(int i = 0; i < SvcListData.size(); i++)
				if(SvcListData.get(i) == svcId) {
					return i;
				}
		}
		return -1;
	}
	public void ReckonServiceEntryCreatedOrUpdated(long svcId)
	{
		if(svcId > 0) {
			View v = findViewById(R.id.serviceListView);
			RecyclerView view = (v != null && v instanceof RecyclerView) ? (RecyclerView)findViewById(R.id.serviceListView) : null;
			RecyclerView.Adapter adapter = (view != null) ? view.getAdapter() : null;
			if(adapter != null) {
				boolean found = false;
				int found_idx = -1;
				if(SvcListData != null) {
					found_idx = FindServiceEntryInListData(svcId);
					if(found_idx >= 0)
						found = true;
				}
				else
					SvcListData = new ArrayList<Long>();
				if(!found) {
					SvcListData.add(new Long(svcId));
					adapter.notifyItemInserted(SvcListData.size()-1);
				}
				else
					adapter.notifyItemChanged(found_idx);
			}
		}
	}
	public void ReckonServiceEntryDeleted(long svcId)
	{
		if(svcId > 0) {
			View v = findViewById(R.id.serviceListView);
			RecyclerView view = (v != null && v instanceof RecyclerView) ? (RecyclerView)findViewById(R.id.serviceListView) : null;
			RecyclerView.Adapter adapter = (view != null) ? view.getAdapter() : null;
			if(adapter != null && SvcListData != null) {
				int found_idx = FindServiceEntryInListData(svcId);
				if(found_idx >= 0)
					adapter.notifyItemRemoved(found_idx);
			}
		}
	}
	//@Override
	//protected void onCreate(Bundle savedInstanceState)
	//{
	//	super.onCreate(savedInstanceState);
	//	setContentView(R.layout.activity_main);
	//	TextView v_info = (TextView)findViewById(R.id.info_text);
	//	v_info.setText("Здесь всякая информация будет");
	//}
	public boolean testDatabase() throws StyloQException
	{
		boolean ok = true;
		ArrayList<StyloQDatabase.TestTable.Rec> test_collection = new ArrayList<StyloQDatabase.TestTable.Rec>();
		Calendar cal = Calendar.getInstance();
		cal.set(2020, 12, 31);
		StyloQDatabase dbs = ((StyloQApp)getApplication()).GetDB();
		dbs.DropTable("TestTable");
		Database.Table tbl = dbs.CreateTable("TestTable");
		if(tbl != null) {
			final String tn = tbl.GetName();
			{
				Database.Transaction tra = new Database.Transaction(dbs, true);
				for(int i = 0; i < 1000; i++) {
					StyloQDatabase.TestTable.Rec rec = new StyloQDatabase.TestTable.Rec();
					rec.ID = 0;
					rec.I32F = i+1;
					rec.I64F = i * 1000000;
					rec.FixLenTextF = "String #" + i + 1;
					rec.VarLenTextF = "Variable length string #" + i + 1;

					cal.add(Calendar.DATE, 1);
					rec.DTF = cal.get(Calendar.MILLISECOND);
					BigInteger bn = SLib.GenerateRandomBigNumber(160);
					rec.FixLenBlobF = bn.toByteArray();
					bn = SLib.GenerateRandomBigNumber(512);
					rec.VarLenBlobF = bn.toByteArray();
					rec.ID = tbl.Insert(rec);
					test_collection.add(rec);
				}
				tra.Commit();
			}
			{
				for(int i = 0; i < 1000; i++) {
					StyloQDatabase.TestTable.Rec rec = test_collection.get(i);
					String query = "SELECT * FROM " + tn + " WHERE id=" + rec.ID;
					android.database.Cursor cur = dbs.GetHandle().rawQuery(query, null);
					if(cur != null) {
						int cc = 0;
						if(cur.moveToFirst()) do {
							cc++;
							StyloQDatabase.TestTable.Rec dbrec = new StyloQDatabase.TestTable.Rec();
							dbrec.Init();
							dbrec.Set(cur);
							if(dbrec.ID != rec.ID)
								ok = false;
							if(dbrec.I32F != rec.I32F)
								ok = false;
							if(dbrec.I64F != rec.I64F)
								ok = false;
							if(dbrec.DTF != rec.DTF)
								ok = false;
							if(!SLib.AreByteArraysEqual(dbrec.FixLenBlobF, rec.FixLenBlobF))
								ok = false;
							if(!SLib.AreByteArraysEqual(dbrec.VarLenBlobF, rec.VarLenBlobF))
								ok = false;
							if(!dbrec.FixLenTextF.equals(rec.FixLenTextF))
								ok = false;
							if(!dbrec.VarLenTextF.equals(rec.VarLenTextF))
								ok = false;
						} while(cur.moveToNext());
						if(cc != 1)
							ok = false;
					}
				}
			}
			if(ok) {
				ok = true; // @debug (breakpoint)
				for(int i = 0; i < 1000; i++) {
					StyloQDatabase.TestTable.Rec rec = test_collection.get(i);
					String query = "SELECT * FROM " + tn + " WHERE FixLenBlobF=x'" +
							SLib.ByteArrayToHexString(rec.FixLenBlobF) + "'";
					//rec.FixLenBlobF.
					android.database.Cursor cur = dbs.GetHandle().rawQuery(query, null);
					if(cur != null) {
						int cc = 0;
						if(cur.moveToFirst()) do {
							cc++;
							StyloQDatabase.TestTable.Rec dbrec = new StyloQDatabase.TestTable.Rec();
							dbrec.Init();
							dbrec.Set(cur);
							if(dbrec.ID != rec.ID)
								ok = false;
							if(dbrec.I32F != rec.I32F)
								ok = false;
							if(dbrec.I64F != rec.I64F)
								ok = false;
							if(dbrec.DTF != rec.DTF)
								ok = false;
							if(!SLib.AreByteArraysEqual(dbrec.FixLenBlobF, rec.FixLenBlobF))
								ok = false;
							if(!SLib.AreByteArraysEqual(dbrec.VarLenBlobF, rec.VarLenBlobF))
								ok = false;
							if(!dbrec.FixLenTextF.equals(rec.FixLenTextF))
								ok = false;
							if(!dbrec.VarLenTextF.equals(rec.VarLenTextF))
								ok = false;
						} while(cur.moveToNext());
						if(cc != 1)
							ok = false;
					}
				}
			}
		}
		return ok;
	}
	void TestScan()
	{
		StyloQApp app_ctx = (StyloQApp)getApplication();
		//app_ctx.Db.DropTable("SecTable"); // @debug
		// (reg)
		//String test_scan = "AmITihEDBDBJsCTgZtbnXY6rtcBA=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJSRUdJU1RFUiJ9";

		//String test_scan = "AuWpoJJiRrlmPjNhLYGlJkS4Lcl8=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJSRUdJU1RFUiJ9";
		//String test_scan = "AuWpoJJiRrlmPjNhLYGlJkS4Lcl8=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJEVExPR0lOIn0=";
		// (sample) String test_scan = "AuWpoJJiRrlmPjNhLYGlJkS4Lcl8=&tajqht41HkOAu/evMPPwIA==&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJEVExPR0lOIn0=";
		// sann
		// (granit) String test_scan = "AwBORAorRXn83orZi6Yhox/G2m7o=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjY5LjI0MQ==&eyJjbWQiOiJSRUdJU1RFUiJ9"; // granit
		// (login) String test_scan = "A9bzO2m1Cus4XGExCic/7hhwTFeQ=&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJEVExPR0lOIn0="; // dtlogin
		//String test_scan = "46209443";
		//String test_scan ="ADicYWqXpGWGjQXS7CY+ejAyzF4o=&tajqht41HkOAu/evMPPwIA==&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJSRUdJU1RFUiJ9";
		String test_scan ="AuWpoJJiRrlmPjNhLYGlJkS4Lcl8=&tajqht41HkOAu/evMPPwIA==&AAAAAA==&YW1xcDovL0FkbWluOkNYOFUza005d1RRYkAyMTMuMTY2LjcwLjIyMQ==&eyJjbWQiOiJEVExPR0lOIn0=";
		//String test_scan = "DicYWqXpGWGjQXS7CY+ejAyzF4o=";
		StyloQInterchange.Invitation inv = null;
		/*try {
			String test_inv = test_scan;
			inv = StyloQInterchange.AcceptInvitation(test_inv);
		} catch(StyloQException exn) {
			;
		}*/
		//try {
			if(false) {
				byte [] svc_ident = Base64.getDecoder().decode("4RA8CYgG0PzF/bVebwsJ80qBa5g=");
				StyloQInterchange.DoInterchangeParam param = new StyloQInterchange.DoInterchangeParam(svc_ident);
				param.CommandJson = "{cmd=\"echo\"}";
				StyloQInterchange.RunClientInterchange(app_ctx, param);
			}
			{ // Если test_scan == null or invalid
				//
				// Отладка http-интерфейса
				//
				//const char * p_svc_ident_mime = "/TV5LgPLqvrjL7kAaMnt8a1Kjt8="; // pft
				//const char * p_amq_server = "amqp://213.166.70.221";
				//const char * p_http_server = "http://192.168.0.205/styloq";
				//inv = new StyloQInterchange.Invitation();
				//inv.SvcIdent = Base64.getDecoder().decode("/TV5LgPLqvrjL7kAaMnt8a1Kjt8=");
				//inv.AccessPoint = "http://192.168.0.205/styloq";
				//inv.AccessPoint = "amqp://213.166.70.221";
				//inv.CommandJson = null;
			}
			//
			if(inv != null) {
				/*
				String info_buf = null;
				String svc_ident_mime = Base64.getEncoder().encodeToString(inv.SvcIdent);
				info_buf = "SvcIdent: " + svc_ident_mime + "\n" +
						"Capabilities: " + Integer.toString(inv.Capabilities) + "\n" +
						"Access Point: " + inv.AccessPoint + "\n" +
						"Command: " + inv.CommandJson;
				v_info.setText(info_buf);
				 */
				{
					StyloQInterchange.DoInterchangeParam param = new StyloQInterchange.DoInterchangeParam(inv.SvcIdent);
					param.LoclAddendum = inv.LoclAddendum; // @v11.2.3
					param.AccsPoint = inv.AccessPoint;
					param.CommandJson = inv.CommandJson;
					param.SvcCapabilities = inv.Capabilities;
					StyloQInterchange.RunClientInterchange(app_ctx, param);
				}
			}
			else {
				// Error parsing of invatation
			}
		/*} catch(StyloQException exn) {
			app_ctx.DisplayError(null, exn, 5000);
		} catch(JSONException exn) {
			app_ctx.DisplayError(null, new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage()), 5000);
		}*/
	}
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		//String barcode_text;
		StyloQApp app_ctx = (StyloQApp)getApplication();
		if(app_ctx != null) {
			if(requestCode != CUSTOMIZED_REQUEST_CODE && requestCode != IntentIntegrator.REQUEST_CODE) {
				// This is important, otherwise the result will not be passed to the fragment
				super.onActivityResult(requestCode, resultCode, data);
			}
			else {
				switch(requestCode) {
					case CUSTOMIZED_REQUEST_CODE: {
						//toast.makeText(this, "REQUEST_CODE = " + requestCode, Toast.LENGTH_LONG).show();
						break;
					}
					default:
						break;
				}
				IntentResult result = IntentIntegrator.parseActivityResult(resultCode, data);
				String contents = (result != null) ? result.getContents() : null;
				if(contents == null) {
					Intent original_intent = result.getOriginalIntent();
					if(original_intent == null) {
						//Log.d("MainActivity", "Cancelled scan");
						//Toast.makeText(this, "Cancelled", Toast.LENGTH_LONG).show();
					}
					else if(original_intent.hasExtra(Intents.Scan.MISSING_CAMERA_PERMISSION)) {
						//Log.d("MainActivity", "Cancelled scan due to missing camera permission");
						//Toast.makeText(this, "Cancelled due to missing camera permission", Toast.LENGTH_LONG).show();
					}
				}
				else {
					//Log.d("MainActivity", "Scanned");
					//Toast.makeText(this, "Scanned: " + result.getContents(), Toast.LENGTH_LONG).show();
					//TextView v_info = (TextView) findViewById(R.id.info_text);
					try {
						StyloQInterchange.Invitation inv = StyloQInterchange.AcceptInvitation(contents);
						if(inv != null) {
							/*
							String info_buf;
							info_buf = "SvcIdent: " + Base64.getEncoder().encodeToString(inv.SvcIdent) + "\n" +
									"Capabilities: " + Integer.toString(inv.Capabilities) + "\n" +
									"Access Point: " + inv.AccessPoint + "\n" +
									"Command: " + inv.CommandJson;
							v_info.setText(info_buf);
							 */
							{
								StyloQInterchange.DoInterchangeParam param = new StyloQInterchange.DoInterchangeParam(inv.SvcIdent);
								param.LoclAddendum = inv.LoclAddendum;
								param.AccsPoint = inv.AccessPoint;
								param.CommandJson = inv.CommandJson;
								param.SvcCapabilities = inv.Capabilities;
								{
									StyloQCommand.Item fake_org_cmd = new StyloQCommand.Item();
									fake_org_cmd.Name = "dtlogin";
									fake_org_cmd.BaseCmdId = StyloQCommand.sqbcDtLogin;
									param.OriginalCmdItem = fake_org_cmd;
								}
								param.RetrActivity_ = this;
								StyloQInterchange.RunClientInterchange(app_ctx, param);
							}
						}
						else {
							// Error parsing of invatation
						}
					} catch(StyloQException exn) {
						app_ctx.DisplayError(this, exn, 5000);
						//String msg = exn.GetMessage(this);
						//v_info.setText((msg != null) ? msg : "unkn exception");
					}
				}
			}
		}
	}
}