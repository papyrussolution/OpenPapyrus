// CommandListActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.location.Location;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.recyclerview.widget.RecyclerView;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.MultiFormatWriter;
import com.google.zxing.WriterException;
import com.google.zxing.client.android.Intents;
import com.google.zxing.common.BitMatrix;
import com.google.zxing.integration.android.IntentIntegrator;
import com.google.zxing.integration.android.IntentResult;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Base64;
import java.util.TimerTask;

public class CommandListActivity extends SLib.SlActivity {
	private byte [] SvcIdent = null;
	private StyloQDatabase.SecStoragePacket SvcPack = null;
	private StyloQDatabase Db = null;
	private StyloQCommand.List ListData;
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshStatus(); }}); }
	}
	static class InvitationDialog extends SLib.SlDialog {
		SLib.SlActivity ActivityCtx;
		String InvitationData;
		InvitationDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_SVCINVITATION, data);
			if(ctx != null && ctx instanceof SLib.SlActivity)
				ActivityCtx = (SLib.SlActivity)ctx;
			InvitationData = (data != null && data instanceof String) ? (String)data : null;
		}
		private Bitmap MakeQrCodeImage(String dataSeq)
		{
			Bitmap result = null;
			if(ActivityCtx != null) {
				StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(ActivityCtx);
				if(app_ctx != null) {
					try {
						int _org_w = (int)app_ctx.getResources().getDimension(R.dimen.invitationqrcodeside);
						int _org_h = (int)app_ctx.getResources().getDimension(R.dimen.invitationqrcodeside);
						BitMatrix matrix = new MultiFormatWriter().encode(dataSeq, BarcodeFormat.QR_CODE, _org_w, _org_h);
						int bitmatrix_width = matrix.getWidth();
						int bitmatrix_height = matrix.getHeight();
						int[] pixels = new int[bitmatrix_width * bitmatrix_height];
						int color_fg = ActivityCtx.getResources().getColor(R.color.BarcodeFg, ActivityCtx.getTheme());
						int color_bg = ActivityCtx.getResources().getColor(R.color.BarcodeBg, ActivityCtx.getTheme());
						for(int y = 0; y < bitmatrix_height; y++) {
							int offset = y * bitmatrix_width;
							for(int x = 0; x < bitmatrix_width; x++) {
								pixels[offset + x] = matrix.get(x, y) ? color_fg : color_bg;
							}
						}
						Bitmap bitmap = Bitmap.createBitmap(bitmatrix_width, bitmatrix_height, Bitmap.Config.ARGB_4444);
						bitmap.setPixels(pixels, 0, bitmatrix_width, 0, 0, bitmatrix_width, bitmatrix_height);
						result = bitmap;
					} catch(WriterException exn) {
						//exn.printStackTrace();
						result = null;
					}
				}
			}
			return result;
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					{
						requestWindowFeature(Window.FEATURE_NO_TITLE);
						setContentView(R.layout.dialog_svcinvitation);
						StyloQApp app_ctx = ActivityCtx != null ? SLib.SlActivity.GetAppCtx(ActivityCtx) : null;
						if(app_ctx != null) {
							//setTitle(SLib.ExpandString(app_ctx, "@{personevent}"));
							{
								View vg = findViewById(R.id.DLG_SVCINVITATION);
								if(vg != null && vg instanceof ViewGroup)
									SLib.SubstituteStringSignatures(app_ctx, (ViewGroup)vg);
							}
							{
								View v = findViewById(R.id.CTLIMG_SVCINVITATION_QRCODE);
								if(v != null && v instanceof ImageView) {
									Bitmap bmp = MakeQrCodeImage(InvitationData);
									if(bmp != null)
										((ImageView)v).setImageBitmap(bmp);
								}
							}
							if(ActivityCtx instanceof CommandListActivity) {
								String fmt_text = app_ctx.GetString(ppstr2.PPSTR_TEXT, ppstr2.PPTXT_STQ_HINT_FRIENDINVITATION);
								String svc_name = ((CommandListActivity)ActivityCtx).SvcPack.GetSvcName(null);
								if(SLib.GetLen(fmt_text) > 0 && SLib.GetLen(svc_name) > 0) {
									String hint_text = String.format(fmt_text, svc_name);
									SLib.SetCtrlString(this, R.id.CTL_SVCINVITATION_HINT, hint_text);
								}
							}
						}
						//SetDTS(Data);
					}
					break;
				case SLib.EV_COMMAND:
					if(srcObj != null && srcObj instanceof View) {
						final int view_id = ((View)srcObj).getId();
						if(view_id == R.id.STDCTL_CLOSEBUTTON) {
							this.dismiss();
						}
					}
					break;
			}
			return result;
		}
	}
	static class PersonEvent {
		PersonEvent()
		{
			OpID = 0;
			Dtm = null;
			SrcCmdItem = null;
			CurrentGeoLoc = null;
			Memo = null;
		}
		int   OpID;
		SLib.LDATETIME Dtm;
		StyloQCommand.Item SrcCmdItem; // Исходная команда сервиса, на основании которой формируется событие
		SLib.GeoPosLL CurrentGeoLoc; // @v11.6.4 Геолокация текущего положения клиента
		String Memo;
	}
	static class PersonEventDialog extends SLib.SlDialog {
		CommandListActivity ActivityParent;
		PersonEventDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_PERSONEVENT, data);
			if(ctx != null && ctx instanceof CommandListActivity)
				ActivityParent = (CommandListActivity)ctx;
			if(data != null && data instanceof PersonEvent) {
				Data = data;
			}
		}
		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					{
						requestWindowFeature(Window.FEATURE_NO_TITLE);
						setContentView(R.layout.dialog_personevent);
						StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
						if(app_ctx != null)
							setTitle(SLib.ExpandString(app_ctx, "@{personevent}"));
						SetDTS(Data);
					}
					break;
				case SLib.EV_COMMAND:
					if(srcObj != null && srcObj instanceof View) {
						final int view_id = ((View)srcObj).getId();
						if(view_id == R.id.STDCTL_OKBUTTON) {
							Object data = GetDTS();
							if(data != null) {
								StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
								if(app_ctx != null)
									app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
							}
							this.dismiss();
						}
						else if(view_id == R.id.STDCTL_CANCELBUTTON) {
							this.dismiss();
						}
					}
					break;
			}
			return result;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			String cmd_name = null;
			String memo = null;
			PersonEvent _data = null;
			if(objData != null && objData instanceof PersonEvent) {
				_data = (PersonEvent)Data;
				cmd_name = _data.SrcCmdItem.Name;
				memo = _data.Memo;
			}
			else {
				_data = new PersonEvent();
				Data = _data;
			}
			SLib.SetCtrlString(this, R.id.CTL_PERSONEVENT_OPNAME, cmd_name);
			{
				double dist_m = -1.0;
				if(_data.SrcCmdItem != null && _data.SrcCmdItem.CanEvaluateDistance(_data.CurrentGeoLoc) && _data.SrcCmdItem.MaxDistM > 0.0) {
					dist_m = _data.SrcCmdItem.GetGeoDistance(_data.CurrentGeoLoc);
				}
				if(dist_m >= 0.0) {
					SLib.SetCtrlVisibility(this, R.id.CTLGRP_PERSONEVENT_GEOLOC, View.VISIBLE);
					String dist_m_text = SLib.formatdouble(dist_m, 1) + "m";
					SLib.SetCtrlString(this, R.id.CTL_PERSONEVENT_DIST, dist_m_text);
				}
				else {
					SLib.SetCtrlVisibility(this, R.id.CTLGRP_PERSONEVENT_GEOLOC, View.GONE);
				}
			}
			SLib.SetCtrlString(this, R.id.CTL_PERSONEVENT_MEMO, memo);
			return ok;
		}
		Object GetDTS()
		{
			Object result = null;
			StyloQApp app_ctx = (ActivityParent != null) ? ActivityParent.GetAppCtx() : null;
			if(app_ctx != null) {
				PersonEvent _data = null;
				if(Data != null && Data instanceof PersonEvent)
					_data = (PersonEvent)Data;
				else {
					_data = new PersonEvent();
					Data = _data;
				}
				_data.Memo = SLib.GetCtrlString(this, R.id.CTL_PERSONEVENT_MEMO);
				result = Data;
			}
			return result;
		}
	}

	public CommandListActivity()
	{
		super();
		ListData = null;
	}
	private void RefreshStatus()
	{
		if(ListData != null) {
			View v = findViewById(R.id.commandListView);
			if(v != null && v instanceof RecyclerView) {
				RecyclerView rv = (RecyclerView)v;
				RecyclerView.Adapter a = rv.getAdapter();
				if(a != null)
					a.notifyDataSetChanged();
			}
		}
	}
	public final byte [] GetSvcIdent()
	{
		return SvcIdent;
	}
	private void Helper_RunCmd(StyloQCommand.Item cmdItem, SLib.GeoPosLL geoPos, boolean forceQuery)
	{
		StyloQApp app_ctx = GetAppCtx();
		if(app_ctx != null) {
			try {
				if(cmdItem.BaseCmdId == StyloQCommand.sqbcPersonEvent) {
					PersonEvent pe = new PersonEvent();
					pe.SrcCmdItem = cmdItem;
					pe.CurrentGeoLoc = geoPos;
					double _max_dist = cmdItem.GetGeoDistanceRestriction();
					double _dist = (_max_dist > 0.0) ? cmdItem.GetGeoDistance(pe.CurrentGeoLoc) : 0.0;
					if(_dist > _max_dist) {
						app_ctx.DisplayError(this, ppstr2.PPERR_STQ_PSNEV_MAXDISTRESTRICTION, 0);
					}
					else {
						PersonEventDialog dialog = new PersonEventDialog(this, pe);
						dialog.getWindow().setLayout(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
						dialog.show();
					}
				}
				else
					app_ctx.RunSvcCommand(SvcIdent, cmdItem, null, forceQuery, null);
			} catch(StyloQException exn) {
				;
			}
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					setContentView(R.layout.activity_command_list);
					Intent intent = getIntent();
					SvcIdent = intent.getByteArrayExtra("SvcIdent");
					{
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
							SetupRecyclerListView(null, R.id.commandListView, R.layout.li_command);
							{
								View vg = findViewById(R.id.CTL_PAGEHEADER_ROOT);
								if(vg != null && vg instanceof ViewGroup)
									SLib.SubstituteStringSignatures(app_ctx, (ViewGroup)vg);
							}
							try {
								Db = app_ctx.GetDB();
								if(Db != null) {
									ListData = null/*Db.GetFaceList()*/;
									if(SLib.GetLen(SvcIdent) > 0) {
										if(SvcPack == null) {
											SvcPack = Db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
										}
										String blob_signature = null;
										if(SvcPack != null) {
											StyloQFace face = SvcPack.GetFace();
											if(face != null)
												blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
											SLib.SetCtrlString(this, R.id.CTL_PAGEHEADER_SVCTITLE, SvcPack.GetSvcName(face));
											StyloQDatabase.SecStoragePacket cmdl_pack = Db.GetForeignSvcCommandList(SvcIdent);
											ListData = cmdl_pack.GetCommandList();
											if(ListData == null)
												ListData = new StyloQCommand.List();
										}
										SLib.SetupImage(this, findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature, false);
										{
											StyloQCommand.Item scan_barcode_cmd_item = ListData.GetItemWithParticularBaseId(StyloQCommand.sqbcLocalBarcodeSearch);
											SLib.SetCtrlVisibility(this, R.id.tbButtonScan, (scan_barcode_cmd_item != null) ? View.VISIBLE : View.GONE);
										}
									}
								}
								if(ListData == null)
									ListData = new StyloQCommand.List();
								ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
							} catch(StyloQException exn) {
								Db = null;
								ListData = new StyloQCommand.List();
								app_ctx.DisplayError(this, exn, 5000);
							}
						}
					}
				}
				break;
			// @v11.9.2 {
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.CTL_BUTTON_RUNCMD);
						SLib.SetupRecyclerListViewHolderAsClickListener(ev_subj.RvHolder, ev_subj.ItemView, R.id.CTL_BUTTON_FORCEUPDATE);
						result = ev_subj.RvHolder;
					}
				}
				break;
			// } @v11.9.2
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = SLib.IsRecyclerListAdapter(srcObj) ? (SLib.RecyclerListAdapter)srcObj : null;
					int _count = 0;
					if(adapter != null) {
						if(adapter.GetItemRcId() == R.layout.li_command && ListData != null)
							_count = ListData.GetViewCount();
					}
					result = new Integer(_count);
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				// @v11.9.2 Активация команды теперь по кнопке, а не по произвольному нажатию элемента списка {
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
							StyloQCommand.Item cmd_item = ListData.GetViewItem(ev_subj.ItemIdx);
							if(cmd_item != null && StyloQCommand.IsCommandPending(SvcIdent, cmd_item) == 0) {
								if(ev_subj.ItemView != null) {
									if(ev_subj.ItemView.getId() == R.id.CTL_BUTTON_RUNCMD || ev_subj.ItemView.getId() == R.id.CTL_BUTTON_FORCEUPDATE) {
										double _max_dist = cmd_item.GetGeoDistanceRestriction();
										if(_max_dist > 0.0) {
											if(app_ctx.IsNetworkDisabled()) { // @v12.2.3 при блокированной сети нельзя запускать такую команду
												; // @todo message
											}
											else {
												// Команда будет запущена после получения координат (see case SLib.EV_GEOLOCDETECTED here)
												if(SLib.QueryCurrentGeoLoc(this, cmd_item, this) != 0) {
													; // будем ждать ответа
												}
												else {
													// Ошибка: скорее всего нет прав на получение координат
													app_ctx.DisplayError(this, ppstr2.PPERR_STQ_EXCECMD_GEOLOCDISABLED, 0);
												}
											}
										}
										else {
											if(ev_subj.ItemView.getId() == R.id.CTL_BUTTON_RUNCMD) {
												Helper_RunCmd(cmd_item, null, false);
												RefreshStatus();
											}
											else if(ev_subj.ItemView.getId() == R.id.CTL_BUTTON_FORCEUPDATE) {
												if(app_ctx.IsNetworkDisabled()) { // @v12.2.3
													; // @todo message
												}
												else {
													Helper_RunCmd(cmd_item, null, true);
													RefreshStatus();
												}
											}
										}
									}
								}
							}
						}
					}
				}
				// } @v11.9.2
				break;
			/* @v11.9.2 case SLib.EV_LISTVIEWITEMLONGCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null) {
							final boolean force_query = (ev == SLib.EV_LISTVIEWITEMLONGCLK) ? true : false;
							StyloQCommand.Item cmd_item = ListData.GetViewItem(ev_subj.ItemIdx);
							if(cmd_item != null) {
								if(StyloQCommand.IsCommandPending(SvcIdent, cmd_item) == 0) { // @v11.4.8
									double _max_dist = cmd_item.GetGeoDistanceRestriction();
									if(_max_dist > 0.0) {
										// Команда будет запущена после получения координат (see case SLib.EV_GEOLOCDETECTED here)
										if(SLib.QueryCurrentGeoLoc(this, cmd_item, this) != 0) {
											; // будем ждать ответа
										}
										else {
											// Ошибка: скорее всего нет прав на получение координат
											app_ctx.DisplayError(this, ppstr2.PPERR_STQ_EXCECMD_GEOLOCDISABLED, 0);
										}
									}
									else {
										Helper_RunCmd(cmd_item, null, force_query);
										RefreshStatus();
									}
								}
							}
						}
					}
				}
				break;*/
			case SLib.EV_GEOLOCDETECTED:
				if(subj != null && subj instanceof Location && srcObj != null && srcObj instanceof StyloQCommand.Item) {
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						//StyloQDatabase.SecStoragePacket _data = (Data != null && Data instanceof StyloQDatabase.SecStoragePacket) ? (StyloQDatabase.SecStoragePacket)Data : null;
						//app_ctx.RunSvcCommand_SetGeoLoc(((CmdROrderPrereqActivity)ctx).CPM.SvcIdent, oid.Id, (Location) subj, this);
						SLib.GeoPosLL geopos = new SLib.GeoPosLL((Location)subj);
						Helper_RunCmd((StyloQCommand.Item)srcObj, geopos, true);
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder != null) {
							// RecyclerView
							if(ListData != null && SLib.IsInRange(ev_subj.ItemIdx, ListData.Items)) {
								StyloQApp app_ctx = GetAppCtx();
								if(app_ctx != null) {
									View iv = ev_subj.RvHolder.itemView;
									StyloQCommand.Item cur_entry = ListData.GetViewItem(ev_subj.ItemIdx);
									SLib.SetCtrlString(iv, R.id.LVITEM_CMDNAME, cur_entry.Name);
									{
										StyloQCommand.CommandPrestatus prestatus = app_ctx.GetCommandStatus(SvcIdent, cur_entry);
										{
											String timewatch_text = "";
											if(prestatus.WaitingTimeMs > 0) {
												final int totalsec = prestatus.WaitingTimeMs / 1000;
												final int h = totalsec / 3600;
												timewatch_text = ((h > 0) ? Integer.toString(h) + ":" : "") + String.format("%02d:%02d", (totalsec % 3600) / 60, (totalsec % 60));
											}
											SLib.SetCtrlString(iv, R.id.CTL_IND_EXECUTETIME, timewatch_text);
										}
										{
											int rcid = 0;
											boolean force_update_cmd_is_visible = false;
											switch(prestatus.S) {
												case StyloQCommand.prestatusQueryNeeded:
													if(app_ctx.IsNetworkDisabled())
														rcid = R.drawable.ic_generic_server_unavailable;
													else
														rcid = R.drawable.ic_generic_server;
													break;
												case StyloQCommand.prestatusActualResultStored:
													rcid = R.drawable.ic_generic_document;
													if(!app_ctx.IsNetworkDisabled())
														force_update_cmd_is_visible = true;
													break;
												case StyloQCommand.prestatusPending:
													rcid = R.drawable.ic_stopwatch;
													break;
												default:
													if(app_ctx.IsNetworkDisabled())
														rcid = R.drawable.ic_generic_command_unavailable;
													else
														rcid = R.drawable.ic_generic_command;
													break;
											}
											{
												View v = iv.findViewById(R.id.CTL_BUTTON_RUNCMD);
												if(v != null && v instanceof ImageButton) {
													if(rcid != 0)
														((ImageButton)v).setImageResource(rcid);
												}
											}
											{
												View v = iv.findViewById(R.id.CTL_BUTTON_FORCEUPDATE);
												if(v != null) {
													v.setVisibility(force_update_cmd_is_visible ? View.VISIBLE : View.GONE);
												}
											}
											/*{
												ImageView ctl = (ImageView) iv.findViewById(R.id.CTL_IND_STATUS);
												if(ctl != null) {
													ctl.setImageResource(rcid);
												}
											}*/
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
			case SLib.EV_IADATAEDITCOMMIT:
				if(srcObj != null && srcObj instanceof PersonEventDialog && subj != null && subj instanceof PersonEvent) {
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						PersonEvent _data = (PersonEvent)subj;
						if(_data.SrcCmdItem != null) {
							JSONObject js_query = new JSONObject();
							String cmd_text = _data.SrcCmdItem.Uuid.toString();
							try {
								js_query.put("cmd", cmd_text);
								js_query.put("time", System.currentTimeMillis());
								if(SLib.GetLen(_data.Memo) > 0)
									js_query.put("memo", _data.Memo);
								app_ctx.RunSvcCommand(SvcIdent, _data.SrcCmdItem, js_query,true, this);
							} catch(JSONException exn) {
								; // @todo
							} catch(StyloQException exn) {
								; // @todo
							}
						}
					}
				}
				break;
			case SLib.EV_SVCQUERYRESULT:
				if(subj != null && subj instanceof StyloQApp.InterchangeResult) {
					StyloQApp app_ctx = GetAppCtx();
					StyloQApp.InterchangeResult ir = (StyloQApp.InterchangeResult)subj;
					if(app_ctx != null && ir.OriginalCmdItem != null) {
						boolean done = false;
						String reply_msg = null;
						String reply_errmsg = null;
						int reply_errcode = 0;
						if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
							JSONObject js_reply = ((SecretTagPool)ir.InfoReply).GetJsonObject(SecretTagPool.tagRawData);
							StyloQInterchange.CommonReplyResult crr = StyloQInterchange.GetReplyResult(js_reply);
							if(crr.Status == 0) {
								reply_errcode = crr.ErrCode;
								reply_errmsg = crr.ErrMsg;
								if(SLib.GetLen(reply_errmsg) > 0)
									app_ctx.DisplayError(this, reply_errmsg, 0);
								else if(reply_errcode > 0)
									app_ctx.DisplayError(this, "Error" + ": " + reply_errcode, 0);
								else
									app_ctx.DisplayError(this, "Error" + ": " + "unknown", 0);
								done = true;
							}
							else if(crr.Status > 0){
								String disp_meth = js_reply.optString("displaymethod", "");
								if(disp_meth.equalsIgnoreCase("goodsinfo")) {
									Intent intent = new Intent(this, CmdRGoodsInfoActivity.class);
									intent.putExtra("SvcIdent", ir.SvcIdent);
									intent.putExtra("SvcReplyDocJson", js_reply.toString());
									LaunchOtherActivity(intent);
									done = true;
								}
								else {
									// @todo Это - заглушка. Здесь надо работать!
									reply_msg = crr.Msg;
									reply_errmsg = crr.ErrMsg;
								}
							}
						}
						if(!done) {
							if(ir.ResultTag == StyloQApp.SvcQueryResult.SUCCESS) {
								if(SLib.GetLen(reply_msg) <= 0)
									reply_msg = "OK";
								app_ctx.DisplayMessage(this, reply_msg, 0);
							}
							else {
								if(SLib.GetLen(reply_errmsg) <= 0)
									reply_errmsg = "ERROR";
								app_ctx.DisplayError(this, reply_errmsg, 0);
							}
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.tbButtonScan) {
					StyloQCommand.Item scan_barcode_cmd_item = ListData.GetItemWithParticularBaseId(StyloQCommand.sqbcLocalBarcodeSearch);
					if(SLib.GetLen(SvcIdent) > 0 && scan_barcode_cmd_item != null && SLib.GetLen(scan_barcode_cmd_item.Name) > 0) {
						IntentIntegrator integrator = new IntentIntegrator(this); // `this` is the current Activity
						//integrator.setPrompt("Scan a barcode");
						integrator.setCameraId(0);  // Use a specific camera of the device
						integrator.setOrientationLocked(true);
						integrator.setBeepEnabled(true);
						integrator.setCaptureActivity(StyloQZxCaptureActivity.class);
						integrator.addExtra("cmd", scan_barcode_cmd_item.Name);
						integrator.addExtra("basecmdid", new Integer(scan_barcode_cmd_item.BaseCmdId));
						String svc_ident_hex = Base64.getEncoder().encodeToString(SvcIdent);
						if(SLib.GetLen(svc_ident_hex) > 0)
							integrator.addExtra("svcident", svc_ident_hex);
						integrator.initiateScan();
					}
				}
				else if(view_id == R.id.tbButtonInvite) {
					try {
						StyloQInterchange.Invitation inv = new StyloQInterchange.Invitation();
						inv.SvcIdent = SvcIdent;
						{
							JSONObject js_query = new JSONObject();
							js_query.put("cmd", "register");
							inv.CommandJson = js_query.toString();
						}
						String inv_text = StyloQInterchange.MakeInvitation(inv);
						StyloQInterchange.Invitation test_inv = StyloQInterchange.AcceptInvitation(inv_text);
						assert(test_inv != null && SLib.AreByteArraysEqual(test_inv.SvcIdent, SvcIdent));
						InvitationDialog dialog = new InvitationDialog(this, inv_text);
						dialog.show();
					} catch(StyloQException | JSONException exn) {
						; // @todo
					}
				}
				break;
		}
		return result;
	}
	@Override protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		//String barcode_text;
		StyloQApp app_ctx = (StyloQApp)getApplication();
		if(app_ctx != null) {
			if(requestCode != MainActivity.CUSTOMIZED_REQUEST_CODE && requestCode != IntentIntegrator.REQUEST_CODE) {
				// This is important, otherwise the result will not be passed to the fragment
				super.onActivityResult(requestCode, resultCode, data);
			}
			else {
				switch(requestCode) {
					case MainActivity.CUSTOMIZED_REQUEST_CODE: {
						//toast.makeText(this, "REQUEST_CODE = " + requestCode, Toast.LENGTH_LONG).show();
						break;
					}
					default:
						break;
				}
				IntentResult result = IntentIntegrator.parseActivityResult(resultCode, data);
				if(result != null) {
					String barcode = result.getContents();
					String barcode_std = result.getFormatName();
					Intent original_intent = result.getOriginalIntent();
					if(SLib.GetLen(barcode) > 0) {
						//Log.d("MainActivity", "Scanned");
						//Toast.makeText(this, "Scanned: " + result.getContents(), Toast.LENGTH_LONG).show();
						//TextView v_info = (TextView) findViewById(R.id.info_text);
						StyloQCommand.Item scan_barcode_cmd_item = ListData.GetItemWithParticularBaseId(StyloQCommand.sqbcLocalBarcodeSearch);
						//String target_cmd_symb = data.getStringExtra("cmd");
						//int    target_basecmdid = data.getIntExtra("basecmdid", 0);
						//String target_svc_ident_hex = data.getStringExtra("svcident");
						//byte [] local_svc_ident = (SLib.GetLen(target_svc_ident_hex) > 0) ? Base64.getDecoder().decode(target_svc_ident_hex) : null;
						if(scan_barcode_cmd_item != null) {
							try {
								//StyloQInterchange.Invitation inv = StyloQInterchange.AcceptInvitation(contents);
								StyloQInterchange.DoInterchangeParam param = new StyloQInterchange.DoInterchangeParam(SvcIdent);
								JSONObject js_query = new JSONObject();
								String cmd_text = scan_barcode_cmd_item.Uuid.toString();
								if(SLib.GetLen(cmd_text) > 0) {
									js_query.put("cmd", cmd_text);
									js_query.put("time", System.currentTimeMillis());
									js_query.put("barcode", barcode);
									if(SLib.GetLen(barcode_std) > 0)
										js_query.put("barcodestd", barcode_std);
									param.LoclAddendum = null;
									param.AccsPoint = null;
									param.CommandJson = js_query.toString();
									param.SvcCapabilities = 0;
									param.OriginalCmdItem = scan_barcode_cmd_item;
									param.RetrHandler_ = this;
									StyloQInterchange.RunClientInterchange(app_ctx, param);
								}
							/*} catch(StyloQException exn) {
								app_ctx.DisplayError(this, exn, 5000);
								//String msg = exn.GetMessage(this);
								//v_info.setText((msg != null) ? msg : "unkn exception");
							*/} catch(JSONException exn) {
								StyloQException stq_exn = new StyloQException(ppstr2.PPERR_JEXN_JSON, exn.getMessage());
								app_ctx.DisplayError(this, stq_exn, 5000);
							}
						}
					}
					else {
						if(original_intent == null) {
							//Log.d("MainActivity", "Cancelled scan");
							//Toast.makeText(this, "Cancelled", Toast.LENGTH_LONG).show();
						}
						else if(original_intent.hasExtra(Intents.Scan.MISSING_CAMERA_PERMISSION)) {
							//Log.d("MainActivity", "Cancelled scan due to missing camera permission");
							//Toast.makeText(this, "Cancelled due to missing camera permission", Toast.LENGTH_LONG).show();
						}
					}
				}
			}
		}
	}
}