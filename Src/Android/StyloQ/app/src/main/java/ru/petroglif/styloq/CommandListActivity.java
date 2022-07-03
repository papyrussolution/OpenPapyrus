// CommandListActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageView;
import android.widget.LinearLayout;

import androidx.recyclerview.widget.RecyclerView;

import org.jetbrains.annotations.NotNull;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Timer;
import java.util.TimerTask;

public class CommandListActivity extends SLib.SlActivity {
	private byte [] SvcIdent = null;
	private StyloQDatabase.SecStoragePacket SvcPack = null;
	private StyloQDatabase Db = null;
	private StyloQCommand.List ListData;
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshStatus(); }}); }
	}
	private Timer RTmr;
	//private RefreshTimerTask RTmrTask;

	static class PersonEvent {
		PersonEvent()
		{
			OpID = 0;
			Dtm = null;
			SrcCmdItem = null;
			Memo = null;
		}
		int   OpID;
		SLib.LDATETIME Dtm;
		StyloQCommand.Item SrcCmdItem; // Исходная команда сервиса, на основании которой формируется событие
		String Memo;
	}
	static class PersonEventDialog extends SLib.SlDialog {
		PersonEventDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_PERSONEVENT, data);
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
						StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
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
								StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
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
			return null;
		}
		boolean SetDTS(Object objData)
		{
			boolean ok = true;
			String cmd_name = null;
			String memo = null;
			if(objData != null && objData instanceof PersonEvent) {
				cmd_name = ((PersonEvent)Data).SrcCmdItem.Name;
				memo = ((PersonEvent)Data).Memo;
			}
			SLib.SetCtrlString(this, R.id.CTL_PERSONEVENT_OPNAME, cmd_name);
			SLib.SetCtrlString(this, R.id.CTL_PERSONEVENT_MEMO, memo);
			return ok;
		}
		Object GetDTS()
		{
			Object result = null;
			StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
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
	@NotNull private StyloQCommand.CommandPrestatus GetCommandStatus(final StyloQCommand.Item cmdItem)
	{
		StyloQCommand.CommandPrestatus result = new StyloQCommand.CommandPrestatus();
		try {
			if(cmdItem != null && SLib.GetLen(SvcIdent) > 0) {
				int pending = StyloQCommand.IsCommandPending(SvcIdent, cmdItem);
				if(pending != 0) {
					result.S = StyloQCommand.prestatusPending;
					if(pending > 0)
						result.WaitingTimeMs = pending;
				}
				else {
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						StyloQDatabase.SecStoragePacket pack = ((StyloQApp) app_ctx).LoadCommandSavedResult(SvcIdent, cmdItem);
						if(pack != null) {
							result.S = StyloQCommand.prestatusActualResultStored;
						}
					}
					if(result.S == StyloQCommand.prestatusUnkn) {
						if(cmdItem.BaseCmdId == StyloQCommand.sqbcRsrvOrderPrereq || cmdItem.BaseCmdId == StyloQCommand.sqbcReport)
							result.S = StyloQCommand.prestatusQueryNeeded;
						else
							result.S = StyloQCommand.prestatusInstant;
					}
				}
			}
		} catch(StyloQException exn) {
			result.S = StyloQCommand.prestatusUnkn;
		}
		return result;
	}
	public final byte [] GetSvcIdent()
	{
		return SvcIdent;
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_command_list);
				/*{
					// toolbar
					Toolbar toolbar = (Toolbar)findViewById(R.id.toolbarCommandList);
					setSupportActionBar(toolbar);
					// add back arrow to toolbar
					ActionBar aca = getSupportActionBar();
					if(aca != null){
						aca.setDisplayHomeAsUpEnabled(true);
						aca.setDisplayShowHomeEnabled(true);
					}
				}*/
				Intent intent = getIntent();
				SvcIdent = intent.getByteArrayExtra("SvcIdent");
				{
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						SetupRecyclerListView(null, R.id.commandListView, R.layout.li_command);
						{
							View vg = findViewById(R.id.CTL_PAGEHEADER_ROOT);
							if(vg != null && vg instanceof ViewGroup)
								SLib.SubstituteStringSignatures(app_ctx, (ViewGroup) vg);
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
									SLib.SetupImage(this, findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature);
								}
							}
							if(ListData == null)
								ListData = new StyloQCommand.List();
							{
								RTmr = new Timer();
								RTmr.schedule(new RefreshTimerTask(), 1000, 750);
							}
						} catch(StyloQException exn) {
							Db = null;
							ListData = new StyloQCommand.List();
							app_ctx.DisplayError(this, exn, 5000);
						}
					}
				}
				break;
			case SLib.EV_DESTROY:
				if(RTmr != null) {
					RTmr.cancel();
					RTmr = null;
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = (srcObj instanceof SLib.RecyclerListAdapter) ? (SLib.RecyclerListAdapter)srcObj : null;
					int _count = 0;
					if(adapter != null) {
						if(adapter.GetRcId() == R.layout.li_command && ListData != null && ListData.Items != null) {
							_count = ListData.Items.size();
						}
					}
					result = new Integer(_count);
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
			case SLib.EV_LISTVIEWITEMLONGCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQApp app_ctx = GetAppCtx();
						if(app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < ListData.Items.size()) {
							boolean force_query = (ev == SLib.EV_LISTVIEWITEMLONGCLK) ? true : false;
							StyloQCommand.Item cmd_item = ListData.Items.get(ev_subj.ItemIdx);
							if(cmd_item != null) {
								if(cmd_item.BaseCmdId == StyloQCommand.sqbcPersonEvent) {
									// @construction
									PersonEvent pe = new PersonEvent();
									pe.SrcCmdItem = cmd_item;
									PersonEventDialog dialog = new PersonEventDialog(this, pe);
									dialog.getWindow().setLayout(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT);
									dialog.show();
								}
								else
									app_ctx.RunSvcCommand(SvcIdent, cmd_item, null, force_query, null);
							}
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
							if(ListData != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < ListData.Items.size()) {
								View iv = ev_subj.RvHolder.itemView;
								StyloQCommand.Item cur_entry = ListData.Items.get(ev_subj.ItemIdx);
								SLib.SetCtrlString(iv, R.id.LVITEM_CMDNAME, cur_entry.Name);
								{
									StyloQCommand.CommandPrestatus prestatus = GetCommandStatus(cur_entry);
									{
										String timewatch_text = "";
										if(prestatus.WaitingTimeMs > 0) {
											final int totalsec = prestatus.WaitingTimeMs / 1000;
											final int h = totalsec / 3600;
											timewatch_text = ((h > 0) ? Integer.toString(h) + ":" : "") + String.format("%02d:%02d", (totalsec % 3600) / 60, (totalsec % 60));
										}
										SLib.SetCtrlString(iv, R.id.LVITEM_IND_EXECUTETIME, timewatch_text);
									}
									{
										ImageView ctl = (ImageView)iv.findViewById(R.id.LVITEM_STATUSINDICATOR);
										if(ctl != null) {
											int rcid = 0;
											switch(prestatus.S) {
												case StyloQCommand.prestatusQueryNeeded: rcid = R.drawable.ic_generic_server; break;
												case StyloQCommand.prestatusActualResultStored: rcid = R.drawable.ic_generic_document; break;
												case StyloQCommand.prestatusPending:
														rcid = R.drawable.ic_stopwatch;
														break;
												default: rcid = R.drawable.ic_generic_command; break;
											}
											ctl.setImageResource(rcid);
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
						String reply_msg = null;
						String reply_errmsg = null;
						if(ir.InfoReply != null && ir.InfoReply instanceof SecretTagPool) {
							byte [] reply_raw_data = ((SecretTagPool)ir.InfoReply).Get(SecretTagPool.tagRawData);
							if(SLib.GetLen(reply_raw_data) > 0) {
								String json_text = new String(reply_raw_data);
								if(SLib.GetLen(json_text) > 0) {
									try {
										JSONObject js_reply = new JSONObject(json_text);
										if(js_reply != null) {
											reply_msg = js_reply.optString("msg", null);
											reply_errmsg = js_reply.optString("errmsg", null);
										}
									} catch(JSONException exn) {
										;
									}
								}
							}
						}
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
				break;
		}
		return result;
	}
}