// MainActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.location.Location;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.recyclerview.widget.RecyclerView;

import com.google.zxing.client.android.Intents;
import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import org.json.JSONException;
import org.json.JSONObject;

import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Calendar;
import java.util.List;
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
						int lang_id_list[] = {SLib.slangRU, SLib.slangEN, SLib.slangDE, SLib.slangNL, SLib.slangPT, SLib.slangES};
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
					// @v11.7.4 {
					{
						String nad_txt = _data.Get(StyloQConfig.tagNotificationActualDays);
						int nad = SLib.satoi(nad_txt);
						if(nad < 0 || nad > 366)
							nad = 0;
						nad_txt = Integer.toString(nad);
						SLib.SetCtrlString(this, R.id.CTL_PRIVCFG_NOTIFICATIONACTIALDAYS, nad_txt);
					}
					// } @v11.7.4
					// @v12.2.10 {
					{
						final int user_flags = SLib.satoi(_data.Get(StyloQConfig.tagUserFlags));
						SLib.SetCheckboxState(this, R.id.CTL_PRIVCFG_CLIPBOARDBCSCANNER, (user_flags & StyloQConfig.userfClipboardBcScanner) != 0);
					}
					// } @v12.2.10
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
					// @v11.7.4 {
					{
						String nad_txt = SLib.GetCtrlString(this, R.id.CTL_PRIVCFG_NOTIFICATIONACTIALDAYS);
						int nad = SLib.satoi(nad_txt);
						if(nad < 0 || nad > 366)
							nad = 0;
						nad_txt = Integer.toString(nad);
						_data.Set(StyloQConfig.tagNotificationActualDays, nad_txt);
					}
					// } @v11.7.4
					// @v12.2.10 {
					{
						final boolean _f = SLib.GetCheckboxState(this, R.id.CTL_PRIVCFG_CLIPBOARDBCSCANNER);
						int user_flags = SLib.satoi(_data.Get(StyloQConfig.tagUserFlags));
						if(_f)
							user_flags |= StyloQConfig.userfClipboardBcScanner;
						else
							user_flags &= ~StyloQConfig.userfClipboardBcScanner;
						_data.Set(StyloQConfig.tagUserFlags, Integer.toString(user_flags));
					}
					// } @v12.2.10
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
		SLib.SlActivity ActivityCtx;
		//private FusedLocationProviderClient FusedLocClient;
		private boolean SvcGpslocSettingAllowed;
		SvcInfoDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_STQSERVICE, data);
			SvcGpslocSettingAllowed = false;
			if(ctx != null && ctx instanceof SLib.SlActivity)
				ActivityCtx = (SLib.SlActivity)ctx;
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
				case SLib.EV_GEOLOCDETECTED:
					if(subj != null && subj instanceof Location) {
						Context ctx = getContext();
						StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
						StyloQDatabase.SecStoragePacket _data = (Data != null && Data instanceof StyloQDatabase.SecStoragePacket) ? (StyloQDatabase.SecStoragePacket)Data : null;
						if(app_ctx != null && _data != null) {
							app_ctx.RunSvcCommand_SetGeoLoc(_data.Rec.BI, 0, (Location) subj, this);
						}
					}
					break;
				case SLib.EV_SVCQUERYRESULT:
					if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
						StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult) subj;
						Context ctx = getContext();
						StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
						if(app_ctx != null) {
							if(ir.OriginalCmdItem != null && SLib.GetLen(ir.OriginalCmdItem.Name) > 0 && ir.OriginalCmdItem.Name.equalsIgnoreCase("setgeoloc")) {
								//
								// Если пришел результат успешной передачи геокоординат сервиса, то сервис
								// нам прислал новое представление своего лика, которое мы сохраним в базе данных.
								//
								StyloQDatabase.SecStoragePacket _data = (Data != null && Data instanceof StyloQDatabase.SecStoragePacket) ? (StyloQDatabase.SecStoragePacket)Data : null;
								if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
									if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
										SecretTagPool svc_reply_pool = (SecretTagPool)ir.InfoReply;
										JSONObject sv_reply_js = svc_reply_pool.GetJsonObject(SecretTagPool.tagRawData);
										String reply_result = sv_reply_js.optString("result");
										if(reply_result != null && reply_result.equalsIgnoreCase("ok") && _data != null) {
											JSONObject js_face = (sv_reply_js != null) ? sv_reply_js.optJSONObject("svcface") : null;
											if(js_face != null) {
												try {
													StyloQDatabase db = app_ctx.GetDB();
													StyloQDatabase.SecStoragePacket svc_pack = db.GetPeerEntry(_data.Rec.ID);
													if(svc_pack != null && svc_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
														byte [] face_bytes = js_face.toString().getBytes(StandardCharsets.UTF_8);
														svc_pack.Pool.Put(SecretTagPool.tagFace, face_bytes);
														final long id = db.PutPeerEntry(svc_pack.Rec.ID, svc_pack, true);
													}
												} catch(StyloQException exn) {
													;
												}

											}
										}
									}
									String reply_msg = null;
									String reply_errmsg = null;
								}
							}
						}
					}
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
					else if(view_id == R.id.CTL_BUTTON_GEOLOCMARK) { // @v11.6.0
						Context ctx = getContext();
						StyloQApp app_ctx = (ctx != null) ? (StyloQApp)ctx.getApplicationContext() : null;
						if(SvcGpslocSettingAllowed) {
							StyloQDatabase.SecStoragePacket _data = (Data != null && Data instanceof StyloQDatabase.SecStoragePacket) ? (StyloQDatabase.SecStoragePacket)Data : null;
							if(_data != null)
								SLib.QueryCurrentGeoLoc(ActivityCtx, new SLib.PPObjID(SLib.PPOBJ_STYLOQBINDERY, (int)_data.Rec.ID), this);
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
						// @v11.6.0 {
						{
							SvcGpslocSettingAllowed = false;
							{
								byte[] cfg_bytes = _data.Pool.Get(SecretTagPool.tagConfig);
								if(SLib.GetLen(cfg_bytes) > 0) {
									StyloQConfig svc_cfg = new StyloQConfig();
									if(svc_cfg.FromJson(new String(cfg_bytes))) {
										final int cli_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagCliFlags));
										if((cli_flags & StyloQConfig.clifSvcGPS) != 0) {
											if(ContextCompat.checkSelfPermission(app_ctx, Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED) {
												SvcGpslocSettingAllowed = true;
											}
										}
									}
								}
								SLib.SetCtrlVisibility(this, R.id.CTL_BUTTON_GEOLOCMARK, SvcGpslocSettingAllowed ? View.VISIBLE : View.GONE);
							}
							// @v12.2.2 {
							{
								boolean is_svc_archived = false;
								byte[] cfg_bytes = _data.Pool.Get(SecretTagPool.tagPrivateConfig);
								if(SLib.GetLen(cfg_bytes) > 0) {
									StyloQConfig svc_cfg = new StyloQConfig();
									if(svc_cfg.FromJson(new String(cfg_bytes))) {
										final int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
										is_svc_archived = (user_flags & StyloQConfig.userfSvcArchived) != 0;
									}
								}
								SLib.SetCheckboxState(this, R.id.CTL_STQSERVICE_ARCHIVED, is_svc_archived);
							}
							// } @v12.2.2
						}
						// } @v11.6.0
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
						//
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
					// @v12.2.2 {
					{
						boolean is_svc_archived = SLib.GetCheckboxState(this, R.id.CTL_STQSERVICE_ARCHIVED);
						byte [] cfg_bytes = _data.Pool.Get(SecretTagPool.tagPrivateConfig);
						StyloQConfig svc_cfg = new StyloQConfig();
						if(SLib.GetLen(cfg_bytes) > 0) {
							if(!svc_cfg.FromJson(new String(cfg_bytes))) {
								svc_cfg = new StyloQConfig(); // о
							}
						}
						int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
						final int org_user_flags = user_flags;
						if(is_svc_archived)
							user_flags |= StyloQConfig.userfSvcArchived;
						else
							user_flags &= ~StyloQConfig.userfSvcArchived;
						if(user_flags != org_user_flags) {
							svc_cfg.Set(StyloQConfig.tagUserFlags, Integer.toString(user_flags));
							//
							String cfg_json = svc_cfg.ToJson();
							cfg_bytes = cfg_json.getBytes();
							if(SLib.GetLen(cfg_bytes) > 0) {
								_data.Pool.Put(SecretTagPool.tagPrivateConfig, cfg_bytes);
							}
						}
					}
					// } @v12.2.2
					result = Data;
				} catch(StyloQException exn) {
					result = null;
				}
			}
			return result;
		}
	}
	private static class ListEntry {
		public static int statusTouched = 0x0001; // На элемент нажали пальцем. Для временного изменения окраски.
		public static int statusPending = 0x0002; // Элемент ожидает завершения операции, запущенной по нажатию пользователем
		public ListEntry(StyloQDatabase db, long svcID)
		{
			SvcID = svcID;
			Status = 0;
			PendingTimeMs = -1;
			IsArchived = false;
			Setup(db);
		}
		public void Setup(StyloQDatabase db)
		{
			Pack = null;
			Face = null;
			CmdListExpiration = -1; // undefined
			if(db != null && SvcID > 0) {
				try {
					Pack = db.GetPeerEntry(SvcID);
					if(Pack != null) {
						Face = Pack.GetFace();
						// @v12.2.2 {
						byte[] cfg_bytes = Pack.Pool.Get(SecretTagPool.tagPrivateConfig);
						if(SLib.GetLen(cfg_bytes) > 0) {
							StyloQConfig svc_cfg = new StyloQConfig();
							if(svc_cfg.FromJson(new String(cfg_bytes))) {
								final int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
								IsArchived = (user_flags & StyloQConfig.userfSvcArchived) != 0;
							}
						}
						// } @v12.2.2
					}
					{
						byte [] svc_ident = Pack.Pool.Get(SecretTagPool.tagSvcIdent);
						if(SLib.GetLen(svc_ident) > 0) {
							if(Pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
								StyloQDatabase.SecStoragePacket cmdlist_pack = db.GetForeignSvcCommandList(svc_ident);
								if(cmdlist_pack != null)
									CmdListExpiration = cmdlist_pack.Rec.Expiration;
							}
						}
					}
				} catch(StyloQException exn) {
					;
				}
			}
		}
		long   SvcID;
		StyloQDatabase.SecStoragePacket Pack;
		StyloQFace Face; // Предварительно извлечен из Pack
		long   CmdListExpiration;
		int    Status;
		int    PendingTimeMs; // Текущее время ожидания результата вызова функции
		boolean IsArchived;
	}
	public static final int CUSTOMIZED_REQUEST_CODE = 0x0000ffff;
	private ArrayList <ListEntry> ListData;
	private boolean IsThereArchivedServices; // @v12.2.2 если true, то в базе данных есть архивные сервисы
	private int ShowArchivedSvcTag; // @v12.2.2 1 - показывать только активные сервисы, 2 - показывать только архивные сервисы, other - показывать все
	private Timer RefreshNotificationIcon_Tmr;
	//private int TouchedListItemIdx; // Элемент, на который нажали пальцем. Для временного изменения окраски.

	private ActivityResultContracts.RequestMultiplePermissions MultiplePermissionsContract;
	private ActivityResultLauncher <String[]> MultiplePermissionLauncher;

	private boolean HasSystemPermissions(final String[] permissions)
	{
		boolean result = false;
		if(permissions != null) {
			result = true;
			for(int pidx = 0; pidx < permissions.length; pidx++) {
				final String permission = permissions[pidx];
				int r = ActivityCompat.checkSelfPermission(this, permission);
				if(r != PackageManager.PERMISSION_GRANTED) {
					Log.d("StyloQ.Permissions", "Permission is not granted: " + permission);
					result = false;
				}
				else
					Log.d("StyloQ.Permissions", "Permission already granted: " + permission);
			}
		}
		return result;
	}
	private void AskSystemPermissions(final String[] permissions, ActivityResultLauncher<String[]> multiplePermissionLauncher)
	{
		if(permissions != null && permissions.length > 0) {
			if(!HasSystemPermissions(permissions/*PermissionList*/)) {
				Log.d("StyloQ.Permissions", "Launching multiple contract permission launcher for ALL required permissions");
				multiplePermissionLauncher.launch(permissions);
			}
			else {
				Log.d("StyloQ.Permissions", "All permissions are already granted");
			}
		}
	}

	public MainActivity()
	{
		super();
		ListData = null;
		RefreshNotificationIcon_Tmr = null;
		IsThereArchivedServices = false; // @v12.2.2 если true, то в базе данных есть архивные сервисы
		ShowArchivedSvcTag = 0; // @v12.2.2 1 - показывать только активные сервисы, 2 - показывать только архивные сервисы
		//TouchedListItemIdx = -1;
	}
	private void MakeListData(StyloQDatabase db)
	{
		ListData = null;
		IsThereArchivedServices = false;
		try {
			ArrayList<Long> id_list = db.GetForeignSvcIdList(true, true);
			if(id_list != null && id_list.size() > 0) {
				ListData = new ArrayList<ListEntry>();
				for(int i = 0; i < id_list.size(); i++) {
					ListEntry new_entry = new ListEntry(db, id_list.get(i));
					// @v12.2.2 {
					if(new_entry.IsArchived)
						IsThereArchivedServices = true;
					// } @v12.2.2
					ListData.add(new_entry);
				}
			}
			// @v12.2.2 {
			if(!IsThereArchivedServices)
				ShowArchivedSvcTag = 0;
			else if(ShowArchivedSvcTag == 1 || ShowArchivedSvcTag == 2) {
				int i = ListData.size();
				if(i > 0) do {
					i--;
					ListEntry entry = ListData.get(i);
					boolean do_remove = false;
					if(entry == null)
						do_remove = true;
					else if(ShowArchivedSvcTag == 1 && entry.IsArchived)
						do_remove = true;
					else if(ShowArchivedSvcTag == 2 && !entry.IsArchived)
						do_remove = true;
					if(do_remove)
						ListData.remove(i);
				} while(i > 0);
			}
			// } @v12.2.2
		} catch(StyloQException exn) {
			;
		}
	}
	private void UpdateSvcListView()
	{
		View v = findViewById(R.id.serviceListView);
		if(v != null && v instanceof RecyclerView) {
			RecyclerView rv = (RecyclerView)v;
			RecyclerView.Adapter a = rv.getAdapter();
			if(a != null)
				a.notifyDataSetChanged();
		}
	}
	private void RefreshStatus()
	{
		if(ListData != null) {
			UpdateSvcListView();
		}
	}
	/* @v11.7.5 private void SetupNotificationIcon(StyloQDatabase db)
	{
		try {
			if(db == null) {
				StyloQApp app_ctx = (StyloQApp)getApplication();
				if(app_ctx != null)
					db = app_ctx.GetDB();
			}
			if(db != null) {
				View v = SLib.FindViewById(this, R.id.tbButtonNotifications);
				if(v != null && v instanceof ImageButton) {
					int notification_actual_days = 0;
					SLib.LDATETIME since = null;
					{
						StyloQConfig cfg_data = new StyloQConfig();
						StyloQDatabase.SecStoragePacket pack = db.GetOwnPeerEntry();
						if(pack != null) {
							byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
							if(SLib.GetLen(cfg_bytes) > 0) {
								String cfg_json = new String(cfg_bytes);
								cfg_data.FromJson(cfg_json);
								String nad_text = cfg_data.Get(StyloQConfig.tagNotificationActualDays);
								notification_actual_days = SLib.satoi(nad_text);
							}
						}
					}
					if(notification_actual_days > 0) {
						SLib.LDATE now_date = SLib.GetCurDate();
						SLib.LDATE since_date = SLib.LDATE.Plus(now_date, -notification_actual_days);
						since = new SLib.LDATETIME(since_date, new SLib.LTIME());
					}
					ImageButton button = (ImageButton) v;
					if(db.IsThereUnprocessedNotifications(0, since))
						button.setImageResource(R.drawable.ic_bell_accent01);
					else if(db.IsThereAnyNotifications(0, null))
						button.setImageResource(R.drawable.ic_bell01);
					else
						button.setVisibility(View.GONE);
				}
			}
		} catch(StyloQException exn) {
			;
		}
	}*/
	private class RefreshNotificationIcon_TimerTask extends TimerTask {
		SLib.EventHandler Handler;
		RefreshNotificationIcon_TimerTask(SLib.EventHandler handler)
		{
			Handler = handler;
		}
		@Override public void run()
		{
			StyloQApp app_ctx = (StyloQApp)getApplication();
			if(app_ctx != null)
				app_ctx.QueryNotificationListStatus(0, Handler);
			//runOnUiThread(new Runnable() { @Override public void run() { SetupNotificationIcon(null); }});
		}
	}
	private class ResetTouchedListItemIdx_TimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { SetTouchedItemIndex(-1); }}); }
	}
	private class UpdateNetworkStateIndicator_TimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { UpdateNetworkStateIndicator(); }}); }
	}
	private boolean NotifyListItemChanged(int idx)
	{
		boolean result = false;
		View v = findViewById(R.id.serviceListView);
		if(v != null && v instanceof RecyclerView) {
			RecyclerView view = (RecyclerView)v;
			RecyclerView.Adapter adapter = view.getAdapter();
			if(adapter != null && idx < adapter.getItemCount()) {
				adapter.notifyItemChanged(idx);
				result = true;
			}
		}
		return result;
	}
	void UpdateNetworkStateIndicator()
	{
		StyloQApp app_ctx = (StyloQApp)getApplication();
		if(app_ctx != null) {
			View v = findViewById(R.id.CTL_NETWORKSTATE_INDICATOR);
			if(v != null && v instanceof ImageView) {
				int ir = 0;
				NetworkConnectionInfoManager.Status st = app_ctx.GetNetworkStatus();
				if(app_ctx.IsNetworkDisabled())
					ir = R.drawable.ic_mobile_network_off;
				else if(st != null) {
					if(st.Transp == NetworkConnectionInfoManager.Transport.Unavailable) {
						ir = R.drawable.ic_mobile_network_disconnected;
					}
					else if(st.Transp == NetworkConnectionInfoManager.Transport.Ethernet) {
						ir = R.drawable.ic_mobile_network_wifi; // @todo сделать картинку
					}
					else if(st.Transp == NetworkConnectionInfoManager.Transport.WIFI) {
						ir = R.drawable.ic_mobile_network_wifi;
					}
					else if(st.Transp == NetworkConnectionInfoManager.Transport.Cellular) {
						if(st.CellularQuality == 4)
							ir = R.drawable.ic_network_cellular_signal_excellent;
						else if(st.CellularQuality == 3)
							ir = R.drawable.ic_network_cellular_signal_good;
						else if(st.CellularQuality == 2)
							ir = R.drawable.ic_network_cellular_signal_ok;
						else
							ir = R.drawable.ic_network_cellular_signal_weak;
					}
					else if(st.Transp == NetworkConnectionInfoManager.Transport.GenericAvailable) {
						ir = R.drawable.ic_network_cellular_signal_ok;
					}
					else
						ir = R.drawable.ic_mobile_network_disconnected;
				}
				((ImageView)v).setImageResource(ir);
			}
		}
	}
	private void SetTouchedItemIndex(int idx)
	{
		if(ListData != null) {
			for(int i = 0; i < ListData.size(); i++) {
				ListEntry entry = ListData.get(i);
				final int preserve_status = entry.Status;
				int new_status = preserve_status;
				boolean set = false;
				if(idx == i) {
					new_status |= ListEntry.statusTouched;
					set = true;
				}
				else {
					new_status &= ~ListEntry.statusTouched;
				}
				if(new_status != preserve_status) {
					entry.Status = new_status;
					if(set) {
						if(NotifyListItemChanged(i)) {
							Timer tmr = new Timer();
							tmr.schedule(new ResetTouchedListItemIdx_TimerTask(), 2000);
						}
					}
					else
						NotifyListItemChanged(i);
				}
			}
		}
	}
	private boolean GetSvcCommandList(StyloQDatabase.SecStoragePacket svcPack, boolean forceQuery)
	{
		boolean ok = false;
		if(svcPack != null) {
			StyloQApp app_ctx = (StyloQApp) getApplication();
			try {
				StyloQDatabase db = (app_ctx != null) ? app_ctx.GetDB() : null;
				if(db != null) {
					boolean do_request = true;
					byte[] svc_ident = svcPack.Pool.Get(SecretTagPool.tagSvcIdent);
					if(SLib.GetLen(svc_ident) > 0) {
						if(!forceQuery && svcPack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
							StyloQDatabase.SecStoragePacket cmd_list_pack = db.GetForeignSvcCommandList(svc_ident);
							if(cmd_list_pack != null && !StyloQInterchange.IsExpired(cmd_list_pack.Rec.Expiration)) {
								CommandListActivity cmdl_activity = app_ctx.FindCommandListActivityBySvcIdent(svc_ident);
								if(cmdl_activity == null) {
									//cmdl_activity.
									Intent intent = new Intent(this, CommandListActivity.class);
									intent.putExtra("SvcIdent", svc_ident);
									this.startActivity(intent);
								}
								else {
									Intent intent = new Intent(this, CommandListActivity.class);
									intent.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
									this.startActivity(intent);
								}
								do_request = false;
							}
						}
						if(do_request) {
							// Если из базы данных не удалось получить список команд, то обращаемся к сервису
							try {
								JSONObject js_query = new JSONObject();
								js_query.put("cmd", "GetCommandList");
								StyloQInterchange.RequestBlock blk = new StyloQInterchange.RequestBlock(svcPack, js_query, null);
								if(StyloQInterchange.DoSvcRequest(app_ctx, blk))
									ok = true;
							} catch(JSONException exn) {
								ok = false;
								new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
							}
						}
					}
					else
						ok = false;
				}
				else
					ok = false;
			} catch(StyloQException exn) {
				ok = false;
			}
		}
		return ok;
	}
	private void SetupShowArchived_Button()
	{
		View v_button = findViewById(R.id.tbButtonToggleArchivedServices);
		if(v_button != null && v_button instanceof ImageButton) {
			int rc_img = 0;
			if(IsThereArchivedServices) {
				if(ShowArchivedSvcTag == 2) {
					rc_img = R.drawable.ic_activity01;
				}
				else /*if(ShowArchivedSvcTag == 1)*/ {
					rc_img = R.drawable.ic_archive01;
					ShowArchivedSvcTag = 1;
				}
			}
			if(rc_img != 0) {
				v_button.setVisibility(View.VISIBLE);
				((ImageButton)v_button).setImageResource(rc_img);
			}
			else {
				v_button.setVisibility(View.GONE);
			}
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
					StyloQApp app_ctx = (StyloQApp) getApplication();
					if(app_ctx != null) {
						// @debug {
							/* List <InputMethodInfo> im_list = null;
							{
								InputMethodManager ime_mgr = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
								if(ime_mgr != null) {
									im_list = ime_mgr.getEnabledInputMethodList();
									if(im_list != null) {

									}
								}
							}*/
						// } @debug
						//app_ctx.DisplayMetrics(); // @test
						try {
							StyloQDatabase db = app_ctx.GetDB();
							if(db != null) {
								db.SetupPeerInstance();
								db.GetDefaultFace(null, true); // Если первый запуск, то создаем defaul-лик
								MakeListData(db);
								SetupShowArchived_Button(); // @v12.2.2
								// @v11.6.0 {
								{
									final String[] permission_list = {
											Manifest.permission.ACCESS_FINE_LOCATION,
											//Manifest.permission.ACCESS_BACKGROUND_LOCATION,
											Manifest.permission.ACCESS_COARSE_LOCATION
									};

									MultiplePermissionsContract = new ActivityResultContracts.RequestMultiplePermissions();
									MultiplePermissionLauncher = registerForActivityResult(MultiplePermissionsContract, isGranted ->
									{
										Log.d("StyloQ.Permissions", "Launcher result: " + isGranted.toString());
									});

									AskSystemPermissions(permission_list, MultiplePermissionLauncher);
								}
								// } @v11.6.0
								// @v11.7.5 SetupNotificationIcon(db);
								app_ctx.QueryNotificationListStatus(0, this); // @v11.7.5
							}
							{
								//View v = findViewById(R.id.CTL_MAIN_VERSION);
								//if(v != null && v instanceof TextView) {
								PackageInfo pInfo = app_ctx.getPackageManager().getPackageInfo(app_ctx.getPackageName(), 0);
								String version = pInfo.versionName;
								SLib.SetCtrlString(this, R.id.CTL_MAIN_VERSION, version);
								{
									if(RefreshNotificationIcon_Tmr == null) {
										RefreshNotificationIcon_Tmr = new Timer();
										RefreshNotificationIcon_Tmr.schedule(new RefreshNotificationIcon_TimerTask(this), 2000, 30000);
									}
								}
							}
							// @v12.0.6 {
							app_ctx.SetupNetworkStatusManager(this);
							{
								Timer tmr = new Timer();
								tmr.schedule(new UpdateNetworkStateIndicator_TimerTask(), 500, 12000);
							}
							// } @v12.0.6
						} catch(StyloQException | PackageManager.NameNotFoundException e) {
							;
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
								MakeListData(db);
								UpdateSvcListView();
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
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.CTL_BUTTON_FORCEUPDATE); // @v12.0.0
						result = ev_subj.RvHolder;
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = SLib.IsRecyclerListAdapter(srcObj) ? (SLib.RecyclerListAdapter)srcObj : null;
					int _count = 0;
					if(adapter != null && adapter.GetItemRcId() == R.layout.li_service && ListData != null) {
						_count = ListData.size();
					}
					result = new Integer(_count);
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(ev_subj != null && app_ctx != null && SLib.IsInRange(ev_subj.ItemIdx, ListData)) {
						ListEntry cur_entry = ListData.get(ev_subj.ItemIdx);
						if(cur_entry != null) {
							if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.buttonInfo) {
								// Здесь показать информацию о сервисе
								if(cur_entry.Pack != null) {
									SvcInfoDialog dialog = new SvcInfoDialog(this, cur_entry.Pack);
									dialog.show();
								}
							}
							// @v12.0.0 {
							else if(ev_subj.ItemView != null && ev_subj.ItemView.getId() == R.id.CTL_BUTTON_FORCEUPDATE) {
								SetTouchedItemIndex(ev_subj.ItemIdx);
								GetSvcCommandList(cur_entry.Pack, true);
							}
							// } @v12.0.0
							else {
								SetTouchedItemIndex(ev_subj.ItemIdx);
								GetSvcCommandList(cur_entry.Pack, false);
							}
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMLONGCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					StyloQApp app_ctx = (StyloQApp)getApplication();
					if(ev_subj != null && app_ctx != null && SLib.IsInRange(ev_subj.ItemIdx, ListData)) {
						SetTouchedItemIndex(ev_subj.ItemIdx);
						ListEntry cur_entry = ListData.get(ev_subj.ItemIdx);
						if(cur_entry != null) {
							GetSvcCommandList(cur_entry.Pack, true);
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
							if(SLib.IsInRange(ev_subj.ItemIdx, ListData)) {
								final ListEntry cur_entry = ListData.get(ev_subj.ItemIdx);
								if(cur_entry != null) {
									boolean force_update_cmd_is_visible = false; // @v12.0.0
									View iv = ev_subj.RvHolder.itemView;
									if((cur_entry.Status & ListEntry.statusTouched) != 0)
										iv.setBackgroundResource(R.drawable.shape_listitem_focused);
									else
										iv.setBackgroundResource(R.drawable.shape_listitem);
									TextView ctl = (TextView)iv.findViewById(R.id.LVITEM_SVCNAME);
									if(cur_entry.Pack != null) { // @v11.4.0 @fix
										if(!StyloQInterchange.IsExpired(cur_entry.CmdListExpiration)) {
											force_update_cmd_is_visible = true;
										}
										if(ctl != null)
											ctl.setText(cur_entry.Pack.GetSvcName(cur_entry.Face));
										View img_view = iv.findViewById(R.id.LVITEM_IMG);
										if(cur_entry.Face != null && img_view != null && img_view instanceof ImageView) {
											String blob_signature = cur_entry.Face.Get(StyloQFace.tagImageBlobSignature, 0);
											SLib.SetupImage(this, img_view, blob_signature, false);
										}
									}
									{
										View v = iv.findViewById(R.id.CTL_BUTTON_FORCEUPDATE);
										if(v != null) {
											v.setVisibility(force_update_cmd_is_visible ? View.VISIBLE : View.GONE);
										}
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
				else if(view_id == R.id.CTL_NETWORKSTATE_INDICATOR) { // @v12.0.11
					StyloQApp app_ctx = (StyloQApp)getApplication();
					app_ctx.ToggleNetworkDisabledTag();
					UpdateNetworkStateIndicator();
				}
				else if(view_id == R.id.tbButtonFaces) {
					Intent intent = new Intent(this, FaceListActivity.class);
					startActivity(intent);
				}
				else if(view_id == R.id.tbButtonSearch) {
					Intent intent = new Intent(this, GlobalSearchActivity.class);
					startActivity(intent);
				}
				else if(view_id == R.id.tbButtonNotifications) { // @v11.5.10
					Intent intent = new Intent(this, NotificationActivity.class);
					startActivity(intent);
				}
				else if(view_id == R.id.tbButtonToggleArchivedServices) { // @v12.2.2
					if(IsThereArchivedServices) {
						if(ShowArchivedSvcTag == 2) {
							ShowArchivedSvcTag = 1;
						}
						else /*if(ShowArchivedSvcTag == 1)*/ {
							ShowArchivedSvcTag = 2;
						}
						try {
							StyloQApp app_ctx = (StyloQApp)getApplication();
							StyloQDatabase db = app_ctx.GetDB();
							if(db != null) {
								MakeListData(db);
								UpdateSvcListView();
							}
							SetupShowArchived_Button();
						} catch(StyloQException exn) {
							;
						}
					}
					else {

					}
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
								// @v12.2.2 {
								boolean org_is_svc_archived = false;
								boolean is_svc_archived = false;
								{
									byte[] cfg_bytes = org_pack.Pool.Get(SecretTagPool.tagPrivateConfig);
									if(SLib.GetLen(cfg_bytes) > 0) {
										StyloQConfig svc_cfg = new StyloQConfig();
										if(svc_cfg.FromJson(new String(cfg_bytes))) {
											final int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
											org_is_svc_archived = (user_flags & StyloQConfig.userfSvcArchived) != 0;
										}
									}
								}
								{
									byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
									if(SLib.GetLen(cfg_bytes) > 0) {
										StyloQConfig svc_cfg = new StyloQConfig();
										if(svc_cfg.FromJson(new String(cfg_bytes))) {
											final int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
											is_svc_archived = (user_flags & StyloQConfig.userfSvcArchived) != 0;
										}
									}
								}
								boolean do_store = false;
								// } @v12.2.2
								if(!SLib.AreByteArraysEqual(org_face_ref, face_ref)) {
									org_pack.Pool.Put(SecretTagPool.tagAssignedFaceRef, face_ref);
									do_store = true;
								}
								// @v12.2.2 {
								if(is_svc_archived != org_is_svc_archived) {
									byte [] cfg_bytes = org_pack.Pool.Get(SecretTagPool.tagPrivateConfig);
									StyloQConfig svc_cfg = new StyloQConfig();
									if(SLib.GetLen(cfg_bytes) > 0) {
										if(!svc_cfg.FromJson(new String(cfg_bytes))) {
											svc_cfg = new StyloQConfig();
										}
									}
									int user_flags = SLib.satoi(svc_cfg.Get(StyloQConfig.tagUserFlags));
									if(is_svc_archived)
										user_flags |= StyloQConfig.userfSvcArchived;
									else
										user_flags &= ~StyloQConfig.userfSvcArchived;
									{
										svc_cfg.Set(StyloQConfig.tagUserFlags, Integer.toString(user_flags));
										String cfg_json = svc_cfg.ToJson();
										cfg_bytes = cfg_json.getBytes();
										if(SLib.GetLen(cfg_bytes) > 0) {
											org_pack.Pool.Put(SecretTagPool.tagPrivateConfig, cfg_bytes);
											do_store = true;
										}
									}
								}
								// } @v12.2.2
								if(do_store) {
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
												StyloQInterchange.CommonReplyResult crr = StyloQInterchange.GetReplyResult(js_reply);
												if(crr.Status > 0)
													login_result = true;
												reply_msg = crr.Msg;
												reply_errmsg = crr.ErrMsg;
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
								// @v11.4.6 {
								try {
									StyloQDatabase db = app_ctx.GetDB();
									if(db != null) {
										MakeListData(db);
										UpdateSvcListView();
									}
								} catch(StyloQException e) {
									;
								}
								// } @v11.4.6
							}
						}
					}
				}
				break;
			case SLib.EV_ASYNCREPLY:
				//Handler.HandleEvent(SLib.EV_ASYNCREPLY, "QueryNotificationListStatus", ns);
				if(srcObj != null && srcObj instanceof String) {
					if(((String)srcObj).equalsIgnoreCase("QueryNotificationListStatus")) {
						if(subj != null && subj instanceof StyloQApp.NotificationListStatus) {
							StyloQApp.NotificationListStatus ns = (StyloQApp.NotificationListStatus)subj;
							View v = SLib.FindViewById(this, R.id.tbButtonNotifications);
							if(v != null && v instanceof ImageButton) {
								ImageButton button = (ImageButton) v;
								if(ns.IsThereUnprocessedNotifications)
									button.setImageResource(R.drawable.ic_bell_accent01);
								else if(ns.IsThereAnyNotifications)
									button.setImageResource(R.drawable.ic_bell01);
								else
									button.setVisibility(View.GONE);
							}
						}
					}
				}
				break;
			case SLib.EV_ASYNCSET:
				if(srcObj != null && srcObj instanceof String) {
					if(((String)srcObj).equalsIgnoreCase("NetworkConnectionStatus")) {
						if(subj != null && subj instanceof NetworkConnectionInfoManager.Status) {
							StyloQApp app_ctx = (StyloQApp)getApplication();
							if(app_ctx != null) {
								app_ctx.SetNetworkStatus((NetworkConnectionInfoManager.Status)subj);
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
		if(ListData != null) {
			for(int i = 0; i < ListData.size(); i++)
				if(ListData.get(i).SvcID == svcId) {
					return i;
				}
		}
		return -1;
	}
	public void ReckonServiceEntryCreatedOrUpdated(long svcId)
	{
		if(svcId > 0) {
			StyloQApp app_ctx = (StyloQApp)getApplication();
			if(app_ctx != null) {
				try {
					StyloQDatabase db = app_ctx.GetDB();
					if(db != null) {
						MakeListData(db);
						UpdateSvcListView();
						SetupShowArchived_Button();
					}
				} catch(StyloQException e) {
					;
				}
				/* @v12.2.2
				View v = findViewById(R.id.serviceListView);
				RecyclerView view = (v != null && v instanceof RecyclerView) ? (RecyclerView)findViewById(R.id.serviceListView) : null;
				RecyclerView.Adapter adapter = (view != null) ? view.getAdapter() : null;
				if(adapter != null) {
					boolean found = false;
					int found_idx = -1;
					if(ListData != null) {
						found_idx = FindServiceEntryInListData(svcId);
						if(found_idx >= 0)
							found = true;
					}
					else
						ListData = new ArrayList<ListEntry>();
					if(!found) {
						try {
							StyloQDatabase db = app_ctx.GetDB();
							if(db != null) {
								ListEntry new_entry = new ListEntry(db, svcId);
								ListData.add(new_entry);
								adapter.notifyItemInserted(ListData.size() - 1);
							}
						} catch(StyloQException exn) {
							;
						}
					}
					else
						adapter.notifyItemChanged(found_idx);
				}*/
			}
		}
	}
	public void ReckonServiceEntryDeleted(long svcId)
	{
		if(svcId > 0) {
			View v = findViewById(R.id.serviceListView);
			RecyclerView view = (v != null && v instanceof RecyclerView) ? (RecyclerView)findViewById(R.id.serviceListView) : null;
			RecyclerView.Adapter adapter = (view != null) ? view.getAdapter() : null;
			if(adapter != null && ListData != null) {
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
	@Override protected void onActivityResult(int requestCode, int resultCode, Intent data)
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
					//byte [] bin_code = contents.getBytes();
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
								param.RetrHandler_ = this;
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