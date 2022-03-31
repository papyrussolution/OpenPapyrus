// CommandListActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.view.View;
import android.widget.ImageView;

import androidx.recyclerview.widget.RecyclerView;

import org.jetbrains.annotations.NotNull;

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
			//RecyclerView view = (RecyclerView)((parentView == null) ? findViewById(rcListView) : parentView.findViewById(rcListView));
		}
	}
	@NotNull private StyloQCommand.CommandPrestatus GetCommandStatus(final StyloQCommand.Item cmdItem)
	{
		StyloQCommand.CommandPrestatus result = new StyloQCommand.CommandPrestatus();
		try {
			if(cmdItem != null && SLib.GetLen(SvcIdent) > 0) {
				Context app_ctx = getApplicationContext();
				int pending = StyloQCommand.IsCommandPending(SvcIdent, cmdItem);
				if(pending != 0) {
					result.S = StyloQCommand.prestatusPending;
					if(pending > 0)
						result.WaitingTimeMs = pending;
				}
				else {
					if(app_ctx != null && app_ctx instanceof StyloQApp) {
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
					StyloQApp app_ctx = (StyloQApp)getApplicationContext();
					SetupRecyclerListView(null, R.id.commandListView, R.layout.command_list_item);
					try {
						Db = app_ctx.GetDB();
						if(Db != null) {
							ListData = null/*Db.GetFaceList()*/;
							if(SLib.GetLen(SvcIdent) > 0) {
								if(SvcPack == null) {
									SvcPack = Db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
								}
								if(SvcPack != null) {
									SLib.SetCtrlString(this, R.id.tbTitle, SvcPack.GetSvcName());
									StyloQDatabase.SecStoragePacket cmdl_pack = Db.GetForeignSvcCommandList(SvcIdent);
									ListData = cmdl_pack.GetCommandList();
									if(ListData == null)
										ListData = new StyloQCommand.List();
								}
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
						app_ctx.DisplayError(null, exn, 5000);
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
						if(adapter.GetRcId() == R.layout.command_list_item && ListData != null && ListData.Items != null) {
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
						StyloQApp app_ctx = (StyloQApp)getApplication();
						if(app_ctx != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < ListData.Items.size()) {
							boolean force_query = (ev == SLib.EV_LISTVIEWITEMLONGCLK) ? true : false;
							app_ctx.RunSvcCommand(SvcIdent, ListData.Items.get(ev_subj.ItemIdx), force_query);
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
												case StyloQCommand.prestatusPending: rcid = R.drawable.ic_stopwatch; break;
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
		}
		return result;
	}
}