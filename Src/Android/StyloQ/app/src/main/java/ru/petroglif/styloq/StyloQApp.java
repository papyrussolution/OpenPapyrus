// StyloQApp.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.graphics.Color;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.blankj.utilcode.util.ActivityUtils;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.play.core.appupdate.AppUpdateManager;
import com.google.android.play.core.appupdate.AppUpdateManagerFactory;
import com.google.android.play.core.install.InstallState;
import com.google.android.play.core.install.InstallStateUpdatedListener;
import com.google.android.play.core.install.model.InstallStatus;

import org.jetbrains.annotations.NotNull;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;

public class StyloQApp extends SLib.App {
	protected StyloQDatabase Db;
	private ArrayList <IgnitionServerEntry> ISL;
	private Timer SvcPollTmr;
	private AppUpdateManager AppUpdMgr;
	private InstallStateUpdatedListener InstallStateUpdatedListener;

	private void StartupTest()
	{
		{
			assert(SLib.satoi(null) == 0);
			assert(SLib.satoi("") == 0);
			assert(SLib.satoi("0") == 0);
			assert(SLib.satoi("-") == 0);
			assert(SLib.satoi("+") == 0);
			assert(SLib.satoi("0x") == 0);
			assert(SLib.satoi("0x0") == 0);
			assert(SLib.satoi("-0x0") == 0);
			assert(SLib.satoi("1") == 1);
			assert(SLib.satoi("0x1") == 1);
			assert(SLib.satoi("-1") == -1);
			assert(SLib.satoi("+1") == 1);
			assert(SLib.satoi("0x0ff") == 255);
			assert(SLib.satoi(" -0x0ff") == -255);
			assert(SLib.satoi("0xb") == 11);
			assert(SLib.satoi("  1000") == 1000);
			assert(SLib.satoi("  1000  ") == 1000);
			assert(SLib.satoi("  1000nothing  ") == 1000);
			assert(SLib.satoi(" \t 90001011  ") == 90001011);
			assert(SLib.satoi(" abracadabra100  ") == 0);
			assert(SLib.satoi("\t 0xabcdef0") == 0xabcdef0);
			assert(SLib.satoi("\t\t \t 1234567890") == 1234567890);
		}
		SLib.LDATE d = SLib.GetCurDate();
		assert(SLib.CheckDate(d.day(), d.month(), d.year()));
		String ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_DMY|SLib.DATF_CENTURY);
		SLib.LDATE d2 = SLib.strtodate(ds, SLib.DATF_DMY);
		assert(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_GERMAN);
		d2 = SLib.strtodate(ds, SLib.DATF_DMY);
		assert(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_ISO8601);
		d2 = SLib.strtodate(ds, SLib.DATF_ISO8601);
		assert(d2.v == d.v);
		ds = SLib.datefmt(d.day(), d.month(), d.year(), SLib.DATF_JAPAN|SLib.DATF_CENTURY);
		d2 = SLib.strtodate(ds, SLib.DATF_JAPAN);
		assert(d2.v == d.v);
		//
		d = new SLib.LDATE(1, 1, 2001);
		assert(d.DayOfWeek(true) == 1);
		for(int i = 1; i < 365; i++) {
			d = SLib.LDATE.Plus(new SLib.LDATE(1, 1, 2001), i);
			int expected_dow = (i+1) % 7;
			if(expected_dow == 0)
				expected_dow = 7;
			int dow = d.DayOfWeek(true);
			assert(dow == expected_dow);
		}
		//
		{
			SLib.LTIME t = new SLib.LTIME(7, 21, 57, 900);
			int h = t.hour();
			int m = t.minut();
			int s = t.sec();
			int hs = t.hs();
			assert(h == 7);
			assert(m == 21);
			assert(s == 57);
			assert(hs == 90);
			t.settotalsec(50000);
			s = t.totalsec();
			assert(s == 50000);
			int nd = t.settotalsec((24 * 3600) * 2 + 50000);
			s = t.totalsec();
			assert(nd == 2);
			assert(s == (24 * 3600) * 2 + 50000);
			//
			//
			t = new SLib.LTIME(7, 21, 57, 900);
			String ts = SLib.timefmt(t, SLib.TIMF_HMS);
			assert(ts.equals("07:21:57"));
			ts = SLib.timefmt(t, SLib.TIMF_HM);
			assert(ts.equals("07:21"));
			ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_NODIV);
			assert(ts.equals("072157"));
			ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_MSEC);
			assert(ts.equals("07:21:57.900"));
			//ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_TIMEZONE);
			//assert(ts.equals("07:21:57"));
			{
				t = SLib.strtotime("07:21:57", SLib.TIMF_HMS);
				assert(t.hour() == 7);
				assert(t.minut() == 21);
				assert(t.sec() == 57);
				assert(t.hs() == 0);

				t = SLib.strtotime("07:21:57.910", SLib.TIMF_HMS);
				h = t.hour();
				m = t.minut();
				s = t.sec();
				hs = t.hs();
				assert(t.hour() == 7);
				assert(t.minut() == 21);
				assert(t.sec() == 57);
				assert(t.hs() == 91);
			}
			/*{
				for(h = 0; h < 24; h++) {
					for(m = 0; m < 60; m++) {
						for(s = 0; s < 60; s++) {
							for(hs = 0; hs < 100; hs++) {
								t = new SLib.LTIME(h, m, s, hs*10);
								ts = SLib.timefmt(t, SLib.TIMF_HMS|SLib.TIMF_MSEC);
								SLib.LTIME t2 = SLib.strtotime(ts, 0);
								assert(t.v == t2.v);
							}
						}
					}
				}
			}*/
		}
		{
			byte [] long_buf = SLib.LongToBytes(SLib.Ssc_CompressionSignature);
			long lv = SLib.BytesToLong(long_buf, 0);
			assert(lv == SLib.Ssc_CompressionSignature);
		}
	}
	public Object HandleEvent(int cmd, Object srcObj, Object subj)
	{
		Object result = null;
		switch(cmd) {
			case SLib.EV_CREATE:
				{
					StartupTest();
					//
					//String av = GetApplicationVersionText();
					//
					SetCurrentLang(0);
					try {
						Db = new StyloQDatabase(this);
						Db.Open();
						{
							int prev_ver = 0;
							int cur_ver = GetApplicationVersionCode();
							if(cur_ver <= 2) {
								Db.CreateTableInDb(null, StyloQDatabase.SysJournalTable.TBL_NAME, false);
							}
							StyloQDatabase.SysJournalTable.Rec sjrec = Db.GetLastEvent(SLib.PPACN_RECENTVERSIONLAUNCHED, 0);
							if(sjrec != null)
								prev_ver = (int)sjrec.Extra;
							if(prev_ver < cur_ver || cur_ver == 2) {
								Db.Upgrade(cur_ver, prev_ver);
							}
							if(sjrec == null || (int)sjrec.Extra < cur_ver) {
								Db.LogEvent(SLib.PPACN_RECENTVERSIONLAUNCHED, 0, 0, cur_ver, true);
							}
						}
						//
						StyloQConfig cfg_data = new StyloQConfig();
						StyloQDatabase.SecStoragePacket pack = Db.GetOwnPeerEntry();
						if(pack != null) {
							byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
							if(SLib.GetLen(cfg_bytes) > 0) {
								String cfg_json = new String(cfg_bytes);
								cfg_data.FromJson(cfg_json);
								String pref_lang_ref = cfg_data.Get(StyloQConfig.tagPrefLanguage);
								SetCurrentLang(SLib.GetLinguaIdent(pref_lang_ref));
							}
							// @construction StyloQJobService.ScheduleTask(this);
							// @construction {
							{
								class TimerTask_DocStatusPoll extends TimerTask {
									private StyloQApp AppCtx;
									TimerTask_DocStatusPoll(StyloQApp appCtx)
									{
										AppCtx = appCtx;
									}
									@Override public void run()
									{
										Thread thr = new Thread(new StyloQInterchange.ThreadEngine_DocStatusPoll(AppCtx));
										thr.start();
										//runOnUiThread(new Runnable() { @Override public void run() { DocStatusPoll(AppCtx); }});
									}
								}
								SvcPollTmr = new Timer();
								SvcPollTmr.schedule(new TimerTask_DocStatusPoll(this), 30 * 1000, 300 * 1000);
							}
							// } @construction
						}
						{
							AppUpdMgr = AppUpdateManagerFactory.create(this);
							InstallStateUpdatedListener = new InstallStateUpdatedListener() {
								@Override public void onStateUpdate(InstallState state)
								{
									if(state.installStatus() == InstallStatus.DOWNLOADED) {
										;
										//CHECK THIS if AppUpdateType.FLEXIBLE, otherwise you can skip
										//popupSnackbarForCompleteUpdate();
									}
									else if(state.installStatus() == InstallStatus.INSTALLED) {
										if(AppUpdMgr != null) {
											//app_upd_mgr.unregisterListener(install_state_updated_listener);
										}
									}
									else {
										//Log.i(TAG, "InstallStateUpdatedListener: state: " + state.installStatus());
									}
								}
							};
							AppUpdMgr.registerListener(InstallStateUpdatedListener);
						}
					}
					catch(StyloQException e) {
						throw new SQLiteException(GetLastErrMessage(null));
					}
				}
				break;
			case SLib.EV_TERMINTATE:
				if(Db != null)
					Db.Close();
				if(InstallStateUpdatedListener != null && AppUpdMgr != null)
					AppUpdMgr.registerListener(InstallStateUpdatedListener);
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				//
				// Транслируем это событие всем существующим экзеплярам SLib.SlActivity
				// Каждый из них должен сам решить что делать с событием на основе типа данных subj
				// и, возможно, проанализировав obj.
				//
				List<Activity> al = ActivityUtils.getActivityList();
				if(al != null) {
					Activity main_activity = null;
					for(int i = 0; i < al.size(); i++) {
						Activity a = al.get(i);
						if(a != null && a instanceof SLib.SlActivity) {
							((SLib.SlActivity)a).HandleEvent(SLib.EV_IADATAEDITCOMMIT, srcObj, subj);
						}
					}
				}
				break;
		}
		return result;
	}
	public String GetString(String signature)
	{
		return _StrStor.GetString(GetCurrentLang(), signature);
	}
	public String GetString(int strGroup, int strIdent)
	{
		return _StrStor.GetString(GetCurrentLang(), strGroup, strIdent);
	}
	enum DisplayMessageKind {
		Notification,
		Error,
		Warning
	}
	private Toast MakeToast(Context anchorView, DisplayMessageKind msgkind, String text, int duration)
	{
		Toast result = null;
		if(anchorView != null && anchorView instanceof SLib.SlActivity && SLib.GetLen(text) > 0) {
			SLib.SlActivity activity = (SLib.SlActivity)anchorView;
			LayoutInflater inflater = activity.getLayoutInflater();
			View layout = inflater.inflate(R.layout.layout_toast, activity.findViewById(R.id.TOAST_CONTAINER));
			if(layout != null) {
				TextView tv = layout.findViewById(R.id.CTL_TOAST_TEXT);
				if(tv != null) {
					tv.setText(text);
					tv.setTextColor(Color.WHITE);
				}
				result = new Toast(getApplicationContext());
				//toast.setGravity(Gravity.BOTTOM, 0, 40);
				result.setDuration((duration > 0) ? duration : Toast.LENGTH_LONG);
				if(msgkind == DisplayMessageKind.Error)
					layout.setBackgroundResource(R.color.Red);
				else if(msgkind == DisplayMessageKind.Notification)
					layout.setBackgroundResource(R.color.PrimaryDark);
				result.setView(layout);
			}
		}
		return result;
	}
	public void DisplayMessage(Context anchorView, String msg, int duration)
	{
		//Toast window = Toast.makeText(this, msg, duration);
		//window.show();
		Toast window = MakeToast(anchorView, DisplayMessageKind.Notification, msg, duration);
		if(window != null)
			window.show();
	}
	public void DisplayMessage(Context anchorView, int strGroup, int strIdent, int duration)
	{
		String msg = _StrStor.GetString(GetCurrentLang(), strGroup, strIdent);
		if(msg != null) {
			//Toast window = Toast.makeText(this, msg, duration);
			Toast window = MakeToast(anchorView, DisplayMessageKind.Notification, msg, duration);
			if(window != null)
				window.show();
		}
	}
	public void DisplayError(Context anchorView, String msg, int duration)
	{
		//Snackbar window = Snackbar.make(this, v, msg, duration);
		//window.setBackgroundTint(Color.RED);
		//window.setTextColor(Color.WHITE);
		//window.show();
		//Toast window = Toast.makeText(this, msg, duration);
		Toast window = MakeToast(anchorView, DisplayMessageKind.Error, msg, duration);
		if(window != null)
			window.show();
	}
	public void DisplayError(Context anchorView, int errCode, int duration)
	{
		String msg = _StrStor.GetString(GetCurrentLang(), ppstr2.PPSTR_ERROR, errCode);
		if(msg != null)
			DisplayError(anchorView, msg, duration);
	}
	public void DisplayError(Context anchorView, StyloQException exn, int duration)
	{
		String msg = exn.GetMessage(this);
		if(msg != null)
			DisplayError(anchorView, msg, duration);
	}
	public void TestDisplaySnackbar(View anchorView, String text, int duration)
	{
		Snackbar window = Snackbar.make(anchorView, text, duration);
		window.show();
	}
	public StyloQDatabase GetDB() throws StyloQException
	{
		//return Db;
		//try {
			if(Db == null)
				Db = new StyloQDatabase(this);
			if(Db != null && Db.IsOpen() == false)
				Db.Open();
		//}
		/*catch(StyloQException e) {
		}*/
		return (StyloQDatabase)Db;
	}
	public SQLiteDatabase GetDBHandle() { return (Db != null) ? Db.GetHandle() : null; }
	public static StyloQDatabase GetDB(Context ctx) throws StyloQException
	{
		StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
		return (app_ctx != null) ? app_ctx.GetDB() : null;
	}
	public static SQLiteDatabase GetDBHandle(Context ctx)
	{
		StyloQApp app_ctx = (StyloQApp)ctx.getApplicationContext();
		return (app_ctx != null) ? app_ctx.GetDBHandle() : null;
	}
	static class IgnitionServerEntry {
		byte [] SvcIdent;
		String Url;
	}
	public final ArrayList <IgnitionServerEntry> GetIgnitionServerList() throws StyloQException
	{
		if(ISL == null) {
			ISL = new ArrayList<IgnitionServerEntry>();
			try {
				Resources res = getResources();
				InputStream in_strm = res.openRawResource(R.raw.styloq_ignition_servers);
				BufferedReader reader = new BufferedReader(new InputStreamReader(in_strm));
				if(reader != null) {
					for(String line = reader.readLine(); line != null; line = reader.readLine()) {
						if(SLib.GetLen(line) > 0) {
							StringTokenizer tokenizer = new StringTokenizer(line, " ");
							if(tokenizer.countTokens() == 2) {
								IgnitionServerEntry entry = new IgnitionServerEntry();
								entry.SvcIdent = Base64.getDecoder().decode(tokenizer.nextToken());
								entry.Url = tokenizer.nextToken();
								ISL.add(entry);
							}
						}
					}
				}
			} catch(IOException exn) {
				throw new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
			}
		}
		return ISL;
	}
	public final ArrayList <IgnitionServerEntry> GetMediatorList()
	{
		ArrayList <IgnitionServerEntry> result = null;
		ArrayList <Long> mediator_id_list = null;
		try {
			result = GetIgnitionServerList();
			StyloQDatabase db = GetDB();
			if(db != null) {
				mediator_id_list = db.GetMediatorIdList();
				if((mediator_id_list != null && mediator_id_list.size() > 0) /*|| (result != null && result.size() > 0)*/) {
					for(int i = 0; i < mediator_id_list.size(); i++) {
						long mediator_id = mediator_id_list.get(i);
						StyloQDatabase.SecStoragePacket mediator_pack = db.GetPeerEntry(i);
						if(mediator_pack != null && (mediator_pack.Rec.Flags & StyloQDatabase.SecStoragePacket.styloqfMediator) != 0) {
							byte [] cfg_bytes = mediator_pack.Pool.Get(SecretTagPool.tagConfig);
							byte [] svc_ident_from_pool = mediator_pack.Pool.Get(SecretTagPool.tagSvcIdent);
							if(SLib.GetLen(cfg_bytes) > 0 && SLib.GetLen(svc_ident_from_pool) > 0) {
								if(!SLib.AreByteArraysEqual(svc_ident_from_pool, mediator_pack.Rec.BI)) {
									; // @error
								}
								else {
									boolean dup_found = false;
									for(int islidx = 0; !dup_found && islidx < result.size(); islidx++) {
										IgnitionServerEntry isl_entry = result.get(islidx);
										if(isl_entry != null && SLib.AreByteArraysEqual(isl_entry.SvcIdent, svc_ident_from_pool))
											dup_found = true;
									}
									if(!dup_found) {
										StyloQConfig svc_cfg = new StyloQConfig();
										if(svc_cfg.FromJson(new String(cfg_bytes))) {
											String svc_url = svc_cfg.Get(StyloQConfig.tagUrl);
											if(SLib.GetLen(svc_url) > 0) {
												try {
													URI uri = new URI(svc_url);
													int uriprot = SLib.GetUriSchemeId(uri.getScheme());
													if(uriprot == 0 && SLib.GetLen(svc_cfg.Get(StyloQConfig.tagMqbAuth)) <= 0) {
														uriprot = SLib.uripprotHttp;
													}
													if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps) {
														IgnitionServerEntry new_isl_entry = new IgnitionServerEntry();
														new_isl_entry.Url = svc_url;
														new_isl_entry.SvcIdent = svc_ident_from_pool;
														result.add(new_isl_entry);
													}
												} catch(URISyntaxException exn) {
													;
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
		} catch(StyloQException exn) {
			result = null;
		}
		return result;
	}
	public static class InterchangeResult {
		InterchangeResult(SvcQueryResult tag, byte [] svcIdent, String textSubj, Object reply)
		{
			SvcIdent = svcIdent;
			SvcReply = null;
			ResultTag = tag;
			InfoReply = reply;
			OriginalCmdItem = null;
			TextSubj = textSubj;
			RetrActivity = null;
			DocReqList = null;
		}
		InterchangeResult(SvcQueryResult tag, @NotNull StyloQInterchange.DoInterchangeParam param, Object reply)
		{
			ResultTag = tag;
			SvcIdent = param.SvcIdent;
			RetrActivity = param.RetrActivity_;
			OriginalCmdItem = param.OriginalCmdItem;
			DocReqList = param.DocReqList;
			SvcReply = null;
			InfoReply = reply;
			TextSubj = null;
		}
		String GetErrMsg()
		{
			return (InfoReply != null && InfoReply instanceof String) ? (String)InfoReply : null;
		}
		byte [] SvcIdent;
		SecretTagPool SvcReply;
		SvcQueryResult ResultTag;
		Object InfoReply;
		StyloQCommand.Item OriginalCmdItem;
		SLib.SlActivity RetrActivity; // @v11.3.10 activity в которую необходимо вернуть результат исполнения команды
		ArrayList <StyloQInterchange.DocumentRequestEntry> DocReqList; // @v11.3.12 Массив структур для синхронизации состояний документов с сервисом.
			// Ссылка на список, переданный асинхронной процедуре обращения к сервису с, возможно, измененными в результате получения ответа от сервиса, атрибутами.
		String TextSubj;
	}
	void SendSvcReplyToMainThread(@NotNull InterchangeResult subj, Object reply)
	{
		class ThreadEngine_SendSvcReplyToMainLooper implements Runnable {
			private StyloQApp Ctx;
			ThreadEngine_SendSvcReplyToMainLooper(StyloQApp ctx)
			{
				Ctx = ctx;
			}
			@Override public void run() { Ctx.OnSvcQueryResult(subj, reply); }
		}
		Looper lpr = Looper.getMainLooper();
		if(lpr != null) {
			new Handler(lpr).post(new ThreadEngine_SendSvcReplyToMainLooper(this));
		}
	}
	//
	// Descr: Предварительный результат отправки документа сервису. Этот результат
	//   не несет никакой информации о том, как сервис обработал документ и дошел ли документ
	//   вообще до сервиса, поскольку процесс передачи асинхронный и будет известен только
	//   после получения ответа от сервиса.
	//   Эта структура содержит лишь информацию об идентификаторе документа (поскольку, если
	//   функции RunSvcPostDocumentCommand был передан еще не сохраненный документ, то она
	//   внесет его в базу данных) и о результате вызова (но не исполнения) асинхронной процедуры DoSvcRequest().
	//
	public static class PostDocumentResult {
		PostDocumentResult()
		{
			DocID = 0;
			PostResult = false;
		}
		long   DocID;
		boolean PostResult;
	}
	@NotNull public PostDocumentResult RunSvcPostDocumentCommand(byte [] svcIdent, Document doc, SLib.SlActivity retrActivity)
	{
		PostDocumentResult result = new PostDocumentResult();
		if(doc != null && doc.H != null && (doc.TiList != null && doc.TiList.size() > 0) || (doc.BkList != null && doc.BkList.size() > 0)) {
			try {
				if(Db != null) {
					StyloQDatabase.SecStoragePacket svc_pack = Db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svcIdent);
					if(svc_pack != null) {
						long svc_id = svc_pack.Rec.ID;
						JSONObject jsobj = doc.ToJsonObj();
						if(jsobj != null) {
							StyloQCommand.Item org_cmd_item = new StyloQCommand.Item();
							org_cmd_item.Name = "PostDocument";
							//
							SecretTagPool doc_pool = new SecretTagPool();
							JSONObject js_query = new JSONObject();
							UUID doc_uuid = SLib.strtouuid(jsobj.optString("uuid", null));
							js_query.put("cmd", org_cmd_item.Name);
							js_query.put("document", jsobj);

							String js_text_doc = jsobj.toString();
							String js_text_docdecl = null;
							{
								JSONObject js_doc_decl = new JSONObject();
								js_doc_decl.put("type", "generic");
								js_doc_decl.put("format", "json");
								js_doc_decl.put("time", SLib.datetimefmt(new SLib.LDATETIME(System.currentTimeMillis()), SLib.DATF_ISO8601|SLib.DATF_CENTURY, 0));
								js_query.put("declaration", js_doc_decl);
								js_text_docdecl = js_doc_decl.toString();
							}
							//
							// В базе данных мы сохраняем документ в виде "сырого" json (то есть только jsobj)
							// в то время как сервису передаем этот же документ вложенный в объект команды (js_query).
							// Но и то и другое вносится в пул хранения под тегом tagRawData.
							//
							SecretTagPool.DeflateStrategy ds = new SecretTagPool.DeflateStrategy(256);
							doc_pool.Put(SecretTagPool.tagRawData, js_text_doc.getBytes(StandardCharsets.UTF_8), ds);
							doc_pool.Put(SecretTagPool.tagDocDeclaration, js_text_docdecl.getBytes(StandardCharsets.UTF_8));
							{
								byte [] doc_ident = Db.MakeDocumentStorageIdent(svcIdent, doc_uuid);
								//
								// Мы сейчас будем отправлять документ в "дальнее путешествие" до сервиса. По-этому, кроме прочих, устанавливаем
								// флаг styloqfDocTransmission в сохраняемую у нас копию документа.
								// Когда мы получим ответ от сервиса этот флаг надо будет снять.
								//
								int  doc_flags = ((doc.H.Flags & StyloQDatabase.SecStoragePacket.styloqfDocStatusFlags) | StyloQDatabase.SecStoragePacket.styloqfDocTransmission);
								result.DocID = Db.PutDocument(+1, StyloQDatabase.SecStoragePacket.doctypGeneric, doc_flags, doc_ident, svc_id, doc_pool);
								if(result.DocID > 0) {
									StyloQInterchange.RequestBlock blk = new StyloQInterchange.RequestBlock(svc_pack, js_query, org_cmd_item);
									blk.RetrActivity = retrActivity;
									{
										StyloQInterchange.DocumentRequestEntry dre = new StyloQInterchange.DocumentRequestEntry();
										dre.DocUUID = doc.H.Uuid;
										dre.DocID = result.DocID;
										dre.AfterTransmitStatus = doc.GetAfterTransmitStatus();
										if(blk.DocReqList == null)
											blk.DocReqList = new ArrayList<StyloQInterchange.DocumentRequestEntry>();
										blk.DocReqList.add(dre);
									}
									if(StyloQInterchange.DoSvcRequest(this, blk)) {
										result.PostResult = true;
									}
								}
							}
						}
					}
				}
			} catch(JSONException | StyloQException exn) {
				;
			}
		}
		return result;
	}
	public StyloQDatabase.SecStoragePacket LoadCommandSavedResult(byte [] svcIdent, StyloQCommand.Item cmdItem) throws StyloQException
	{
		StyloQDatabase.SecStoragePacket result = null;
		if(cmdItem != null) {
			int doc_type = 0;
			if(cmdItem.BaseCmdId == StyloQCommand.sqbcRsrvOrderPrereq) {
				doc_type = StyloQDatabase.SecStoragePacket.doctypOrderPrereq;
			}
			else if(cmdItem.BaseCmdId == StyloQCommand.sqbcRsrvIndoorSvcPrereq) { // @v11.4.5
				doc_type = StyloQDatabase.SecStoragePacket.doctypIndoorSvcPrereq;
			}
			else if(cmdItem.BaseCmdId == StyloQCommand.sqbcReport) {
				doc_type = StyloQDatabase.SecStoragePacket.doctypReport;
			}
			if(doc_type > 0) {
				byte[] doc_ident = Db.MakeDocumentStorageIdent(svcIdent, cmdItem.Uuid);
				ArrayList<Long> doc_id_list = Db.GetDocIdListByType(-1, doc_type, 0, /*svcIdent*/doc_ident);
				if(doc_id_list != null && doc_id_list.size() > 0) {
					StyloQDatabase.SecStoragePacket recent_pack = null;
					if(doc_id_list.size() == 1) {
						recent_pack = Db.GetPeerEntry(doc_id_list.get(0));
					}
					else {
						// Теоретически, более одного элемента в doc_id_list быть не может
						// Практически же, выберем тот, у которого самый поздний timestamp
						long max_timestamp = 0;
						for(int i = 0; i < doc_id_list.size(); i++) {
							StyloQDatabase.SecStoragePacket p = Db.GetPeerEntry(doc_id_list.get(i));
							if(p != null && p.Rec.TimeStamp > max_timestamp) {
								max_timestamp = p.Rec.TimeStamp;
								recent_pack = p;
							}
						}
					}
					if(recent_pack != null && !StyloQInterchange.IsExpired(recent_pack.Rec.Expiration)) {
						result = recent_pack;
					}
				}
			}
		}
		return result;
	}
	//
	// Descr: Реализует обращение к команде cmdItem сервиса svcIdent.
	// ARG(svcIdent): Бинарный идентификатор сервиса
	// ARG(cmdItem): Дескриптор команды
	// ARG(jsReq): Если != null, то является запросом к сервису, в противном случае запрос
	//   формирутся автоматически на основании исходного дескриптора команды cmdItem.
	// ARG(forceSvcQuery): Если false, то функция будет анализировать возможность повторного
	//   использования предыдущего вызова функции (период действия результата и пр.).
	//   Если forceSvcQuery == true, то обращение к сервису осущетсвляется безусловно.
	// Returns:
	//   true - функция завершилась успешно
	//   false - ошибка
	//
	public boolean RunSvcCommand(byte [] svcIdent, StyloQCommand.Item cmdItem, JSONObject jsReq, boolean forceSvcQuery, SLib.SlActivity retrActivity) throws StyloQException
	{
		boolean ok = false;
		if(Db != null && cmdItem != null && cmdItem.Uuid != null && SLib.GetLen(svcIdent) > 0) {
			try {
				StyloQDatabase.SecStoragePacket svc_pack = Db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, svcIdent);
				if(svc_pack != null) {
					StyloQDatabase.SecStoragePacket __pack = forceSvcQuery ? null : LoadCommandSavedResult(svcIdent, cmdItem);
					if(__pack != null) {
						//
						// Если мы извлекли ранее сохраненный результат с незавершенным
						// сроком действия - эмулируем успешное исполнение команды и уходим.
						//
						StyloQCommand.DocReference doc_ref = new StyloQCommand.DocReference();
						doc_ref.ID = __pack.Rec.ID;
						{
							byte[] raw_doc_decl = __pack.Pool.Get(SecretTagPool.tagDocDeclaration);
							if(SLib.GetLen(raw_doc_decl) > 0) {
								doc_ref.Decl = new StyloQCommand.DocDeclaration();
								doc_ref.Decl.FromJson(new String(raw_doc_decl));
							}
						}
						__pack = null;
						StyloQCommand.StartPendingCommand(svcIdent, cmdItem);
						//SvcReplySubject srsub = new SvcReplySubject(svcIdent, null, cmdItem, null, null);
						StyloQInterchange.DoInterchangeParam fake_param = new StyloQInterchange.DoInterchangeParam(svcIdent);
						fake_param.OriginalCmdItem = cmdItem;
						InterchangeResult ir = new InterchangeResult(SvcQueryResult.SUCCESS, fake_param, null);
						this.SendSvcReplyToMainThread(ir, doc_ref);
						ok = true; // Запрос отправлять будет не нужно!
					}
					else { // Мы не обнаружили ранее сохраненного результата команды (или срок годности такового истек) - отправляем запрос.
						JSONObject js_query = null;
						if(jsReq != null) {
							js_query = jsReq;
						}
						else {
							js_query = new JSONObject();
							String cmd_text = cmdItem.Uuid.toString();
							js_query.put("cmd", cmd_text);
							js_query.put("time", System.currentTimeMillis());
						}
						StyloQInterchange.RequestBlock blk = new StyloQInterchange.RequestBlock(svc_pack, js_query, cmdItem);
						blk.RetrActivity = retrActivity;
						if(StyloQInterchange.DoSvcRequest(this, blk)) {
							StyloQCommand.StartPendingCommand(svcIdent, cmdItem);
							ok = true;
						}
					}
				}
			} catch(JSONException exn) {
				ok = false;
				throw new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
			} catch(StyloQException e) {
				ok = false;
			}
		}
		return ok;
	}
	/* (replaced with StyloQInterchange.DoSvcRequest) private boolean DoSvcRequest(StyloQInterchange.RequestBlock blk)
	{
		boolean ok = false;
		if(blk != null && blk.SvcPack != null) {
			String acspt_url = null;
			String acspt_mqbauth = null;
			String acspt_mqbsecr = null;
			StyloQConfig cfg = null;
			byte[] svc_ident = blk.SvcPack.Pool.Get(SecretTagPool.tagSvcIdent);
			byte[] svc_cfg_bytes = blk.SvcPack.Pool.Get(SecretTagPool.tagConfig);
			if(SLib.GetLen(svc_cfg_bytes) > 0) {
				cfg = new StyloQConfig();
				String js_cfg = new String(svc_cfg_bytes);
				if(cfg.FromJson(js_cfg)) {
					acspt_url = cfg.Get(StyloQConfig.tagUrl);
					acspt_mqbauth = cfg.Get(StyloQConfig.tagMqbAuth);
					acspt_mqbsecr = cfg.Get(StyloQConfig.tagMqbSecret);
				}
			}
			if(SLib.GetLen(acspt_url) > 0) {
				if(acspt_url.indexOf("://") < 0) {
					if(SLib.GetLen(acspt_mqbauth) > 0)
						acspt_url = "amqp://" + acspt_url;
					else
						acspt_url = "http://" + acspt_url;
				}
				try {
					URI uri = new URI(acspt_url);
					int uriprot = SLib.GetUriSchemeId(uri.getScheme());
					if(uriprot == SLib.uripprotUnkn) {
						if(SLib.GetLen(acspt_mqbauth) > 0) {
							acspt_url = "amqp://" + acspt_url;
							uri = new URI(acspt_url);
							uriprot = SLib.GetUriSchemeId(uri.getScheme());
						}
						else {
							acspt_url = "http://" + acspt_url;
							uri = new URI(acspt_url);
							uriprot = SLib.GetUriSchemeId(uri.getScheme());
						}
					}
					if(uriprot == SLib.uripprotHttp || uriprot == SLib.uripprotHttps || uriprot == SLib.uripprotAMQP || uriprot == SLib.uripprotAMQPS) {
						StyloQInterchange.DoInterchangeParam param = new StyloQInterchange.DoInterchangeParam(svc_ident);
						param.AccsPoint = acspt_url;
						param.MqbAuth = acspt_mqbauth;
						param.MqbSecret = acspt_mqbsecr;
						param.OriginalCmdItem = blk.OrgCmdItem;
						if(blk.JsCmd != null)
							param.CommandJson = blk.JsCmd.toString();
						param.RetrActivity_ = blk.RetrActivity; // @v11.3.10
						param.DocReqList = blk.DocReqList; // @v11.3.12
						StyloQInterchange.RunClientInterchange(this, param);
						ok = true;
					}
				} catch(URISyntaxException exn) {
					ok = false;
					throw new StyloQException(ppstr2.PPERR_JEXN_URISYNTAX, exn.getMessage());
				}
			}
		}
		return ok;
	}*/
	enum SvcQueryResult {
		UNDEF,
		SUCCESS,
		ERROR,
		EXCEPTION,
		LOCALREJECTION // Обращения к сервису не было из-за каких-то внутренних причин
	}
	public MainActivity FindMainActivity()
	{
		MainActivity result = null;
		List<Activity> al = ActivityUtils.getActivityList();
		if(al != null) {
			for(int i = 0; result == null && i < al.size(); i++) {
				Activity a = al.get(i);
				if(a != null && a instanceof MainActivity)
					result = (MainActivity)a;
			}
		}
		return result;
	}
	public CommandListActivity FindCommandListActivityBySvcIdent(byte [] svcIdent)
	{
		CommandListActivity result = null;
		if(SLib.GetLen(svcIdent) > 0) {
			List<Activity> al = ActivityUtils.getActivityList();
			if(al != null) {
				Activity cmd_list_activity = null;
				for(int i = 0; result == null && i < al.size(); i++) {
					Activity a = al.get(i);
					if(a != null && a instanceof CommandListActivity) {
						byte[] _svc_ident = ((CommandListActivity) a).GetSvcIdent();
						if(SLib.AreByteArraysEqual(_svc_ident, svcIdent))
							result = (CommandListActivity) a;
					}
				}
			}
		}
		return result;
	}
	//
	// Descr: Функция вызывается для обработки результата обращения к сервису.
	//
	public void OnSvcQueryResult(/*StyloQApp.SvcQueryResult resultTag,*/@NotNull InterchangeResult subj, Object reply)
	{
		if(subj != null) {
			String msg_text;
			String subj_text = subj.TextSubj;
			if(SLib.GetLen(subj_text) == 0 && reply != null && reply instanceof String)
				subj_text = (String) reply;
			StyloQCommand.Item original_cmd_item = subj.OriginalCmdItem;
			SLib.SlActivity retr_activity = subj.RetrActivity;
			MainActivity main_activity = FindMainActivity();
			SLib.SlActivity activity_for_message = (retr_activity != null) ? retr_activity : main_activity;
			if(subj.SvcIdent != null && original_cmd_item != null) {
				StyloQCommand.StopPendingCommand(subj.SvcIdent, original_cmd_item);
			}
			if(subj.ResultTag == StyloQApp.SvcQueryResult.EXCEPTION) {
				if(retr_activity != null)
					retr_activity.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
				else if(SLib.GetLen(subj_text) > 0) {
					msg_text = "Exception: " + subj_text;
					DisplayError(activity_for_message, msg_text, 0);
				}
				else
					DisplayError(activity_for_message, "Unknown exception", 0);
			}
			else if(subj.ResultTag != StyloQApp.SvcQueryResult.ERROR && subj.ResultTag != SvcQueryResult.SUCCESS) {
				if(retr_activity != null)
					retr_activity.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
				else if(SLib.GetLen(subj_text) > 0) {
					msg_text = "UNKN result tag: " + subj_text;
					DisplayError(activity_for_message, msg_text, 0);
				}
				else
					DisplayError(activity_for_message, "Unknown result", 0);
			}
			else {
				byte[] rawdata = null;
				if(original_cmd_item != null || retr_activity != null || subj_text.equalsIgnoreCase("Command")) {
					Class intent_cls = null;
					String svc_doc_json = null;
					// @v11.4.5 GlobalSearchActivity gs_activity = null; // Если мы получили ответ на поисковый запрос, то результат надо будет передать в
					// существующию GlobalSearchActivity. Если таковая отсутствует,
					StyloQCommand.DocReference doc_ref = null;
					Intent intent = null;
					if(reply != null) {
						StyloQCommand.DocDeclaration doc_decl = null;
						if(reply instanceof SecretTagPool) {
							SecretTagPool stp_reply = (SecretTagPool) reply;
							rawdata = stp_reply.Get(SecretTagPool.tagRawData);
							byte[] raw_doc_decl = stp_reply.Get(SecretTagPool.tagDocDeclaration);
							List<Activity> current_activity_list = ActivityUtils.getActivityList();
							if(retr_activity != null) {
								boolean ra_found = false;
								if(current_activity_list != null) {
									for(int i = 0; !ra_found && i < current_activity_list.size(); i++) {
										Activity a = current_activity_list.get(i);
										if(a != null && a instanceof SLib.SlActivity && a == retr_activity)
											ra_found = true;
									}
								}
								if(!ra_found)
									retr_activity = null;
							}
							if(retr_activity == null) { // Если было изначально указано в какую Activity отправлять результат, то не надо создавать что-то еще
								if(SLib.GetLen(raw_doc_decl) > 0) {
									doc_decl = new StyloQCommand.DocDeclaration();
									if(doc_decl.FromJson(new String(raw_doc_decl))) {
										if(doc_decl.DisplayMethod.equalsIgnoreCase("grid"))
											intent_cls = CmdRGridActivity.class;
										else if(doc_decl.DisplayMethod.equalsIgnoreCase("orderprereq"))
											intent_cls = CmdROrderPrereqActivity.class;
										else if(doc_decl.DisplayMethod.equalsIgnoreCase("indoorsvcprereq")) // @v11.4.5
											intent_cls = CmdROrderPrereqActivity.class;
										else if(doc_decl.DisplayMethod.equalsIgnoreCase("attendanceprereq"))
											intent_cls = CmdRAttendancePrereqActivity.class;
										/* @v11.4.5
										else if(doc_decl.DisplayMethod.equalsIgnoreCase("search")) {
											if(current_activity_list != null) {
												for(int i = 0; gs_activity == null && i < current_activity_list.size(); i++) {
													Activity a = current_activity_list.get(i);
													if(a != null && a instanceof GlobalSearchActivity)
														gs_activity = (GlobalSearchActivity) a;
												}
											}
											if(gs_activity == null)
												intent_cls = GlobalSearchActivity.class;
										}*/
										if(doc_decl.Format.equalsIgnoreCase("json"))
											svc_doc_json = new String(stp_reply.Get(SecretTagPool.tagRawData));
									}
								}
								if(intent_cls == null/*&& gs_activity == null*/)
									intent_cls = CmdRSimpleActivity.class;
							}
						}
						else if(reply instanceof StyloQCommand.DocReference) { // В качестве ответа передана ссылка на сохраненный документ
							doc_ref = (StyloQCommand.DocReference) reply;
							doc_decl = doc_ref.Decl;
							if(doc_decl.DisplayMethod.equalsIgnoreCase("grid"))
								intent_cls = CmdRGridActivity.class;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("orderprereq"))
								intent_cls = CmdROrderPrereqActivity.class;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("indoorsvcprereq")) // @v11.4.5
								intent_cls = CmdROrderPrereqActivity.class;
							else if(doc_decl.DisplayMethod.equalsIgnoreCase("attendanceprereq"))
								intent_cls = CmdRAttendancePrereqActivity.class;
							if(intent_cls == null)
								intent_cls = CmdRSimpleActivity.class;
						}
					}
					if(retr_activity != null)
						retr_activity.HandleEvent(SLib.EV_SVCQUERYRESULT, null, subj);
					/* @v11.4.5 else if(gs_activity != null) {
						class SendSearchResultToActivity implements Runnable {
							private GlobalSearchActivity A;
							private String Result;
							SendSearchResultToActivity(GlobalSearchActivity ctx, String result)
							{
								A = ctx;
								Result = result;
							}
							@Override
							public void run()
							{
								A.SetQueryResult(Result);
							}
						}
						Looper lpr = gs_activity.getMainLooper();
						if(lpr != null) {
							new Handler(lpr).post(new SendSearchResultToActivity(gs_activity, svc_doc_json));
						}
						//gs_activity.SetQueryResult(svc_doc_json);
					}*/
					else if(intent_cls != null) {
						intent = new Intent(main_activity, intent_cls);
						if(SLib.GetLen(subj.SvcIdent) > 0)
							intent.putExtra("SvcIdent", subj.SvcIdent);
						if(subj.OriginalCmdItem != null) {
							if(subj.OriginalCmdItem.Uuid != null)
								intent.putExtra("CmdUuid", subj.OriginalCmdItem.Uuid.toString());
							if(SLib.GetLen(subj.OriginalCmdItem.Name) > 0)
								intent.putExtra("CmdName", subj.OriginalCmdItem.Name);
							if(SLib.GetLen(subj.OriginalCmdItem.Description) > 0)
								intent.putExtra("CmdDescr", subj.OriginalCmdItem.Description);
						}
						if(doc_ref != null) {
							intent.putExtra("SvcReplyDocID", doc_ref.ID);
						}
						else if(SLib.GetLen(svc_doc_json) > 0) {
							intent.putExtra("SvcReplyDocJson", svc_doc_json);
						}
						else if(rawdata != null) {
							String svc_reply_text = new String(rawdata);
							intent.putExtra("SvcReplyText", svc_reply_text);
						}
						main_activity.startActivity(intent);
					}
					else {
						/*
						String text_to_display = null;
						if(rawdata != null) {
							text_to_display = new String(rawdata);
						}
						if(SLib.GetLen(text_to_display) == 0)
							text_to_display = "no text";
						DisplayMessage(main_activity, "", 0);
						 */
					}
				}
				else if(subj.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
					msg_text = "Success";
					if(main_activity != null) {
						if(subj_text.equalsIgnoreCase("UpdateCommandList")) {
							if(reply instanceof byte[]) {
								byte[] svc_ident = (byte[]) reply;
								CommandListActivity cmdl_activity = FindCommandListActivityBySvcIdent(svc_ident);
								if(cmdl_activity == null) {
									Intent intent = new Intent(main_activity, CommandListActivity.class);
									intent.putExtra("SvcIdent", svc_ident);
									main_activity.startActivity(intent);
								}
							}
						}
						else if(subj_text.equalsIgnoreCase("RegistrationClientRequest")) {
							if(reply instanceof Long) {
								long new_svc_id = (Long) reply;
								if(new_svc_id > 0)
									main_activity.ReckonServiceEntryCreatedOrUpdated(new_svc_id);
							}
						}
					}
				}
				else if(subj.ResultTag == StyloQApp.SvcQueryResult.ERROR) {
					if(SLib.GetLen(subj_text) > 0) {
						msg_text = "Error: " + subj_text;
						DisplayError(activity_for_message, msg_text, 0);
					}
					else
						DisplayError(activity_for_message, "Unknown error", 0);
				}
			}
		}
	}
}
