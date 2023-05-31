package ru.petroglif.styloq;

import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.TimerTask;

public class NotificationActivity extends SLib.SlActivity {
	private StyloQApp AppCtx;
	private ArrayList <StyloQInterchange.SvcNotification> ListData;
	private int NotificationActualDays; // @v11.7.4 Проекция конфигурационного параметра tagNotificationActualDays
	private class RefreshTimerTask extends TimerTask {
		@Override public void run() { runOnUiThread(new Runnable() { @Override public void run() { RefreshStatus(); }}); }
	}
	public NotificationActivity()
	{
		super();
		AppCtx = null;
		ListData = null;
		NotificationActualDays = 0;
	}
	private void RefreshStatus()
	{
		if(ListData != null) {
			View v = findViewById(R.id.notificationListView);
			if(v != null && v instanceof RecyclerView) {
				RecyclerView rv = (RecyclerView)v;
				RecyclerView.Adapter a = rv.getAdapter();
				if(a != null)
					a.notifyDataSetChanged();
			}
		}
	}
	private void MakeListData(StyloQDatabase db)
	{
		if(db != null) {
			SLib.LDATETIME since = null;
			if(NotificationActualDays > 0) {
				SLib.LDATE now_date = SLib.GetCurDate();
				SLib.LDATE since_date = SLib.LDATE.Plus(now_date, -NotificationActualDays);
				since = new SLib.LDATETIME(since_date, new SLib.LTIME());
			}
			ListData = db.GetNotifivationList(0, since, false);
			if(SLib.GetCount(ListData) > 1) {
				ListData.sort(new Comparator<StyloQInterchange.SvcNotification>() {
					@Override public int compare(StyloQInterchange.SvcNotification lh, StyloQInterchange.SvcNotification rh)
					{
						final SLib.LDATETIME dtm1 = (lh != null) ? lh.EventOrgTime : null;
						final SLib.LDATETIME dtm2 = (rh != null) ? rh.EventOrgTime : null;
						return SLib.Cmp(dtm2, dtm1); // Сортировка в обратном порядке (новые записи выше старых)
					}
				});
			}
		}
	}
	@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_notification);
				SetupRecyclerListView(null, R.id.notificationListView, R.layout.li_notification);
				{
					AppCtx = (StyloQApp)getApplication();
					if(AppCtx != null) {
						{
							View vg = findViewById(R.id.CTL_PAGEHEADER_ROOT);
							if(vg != null && vg instanceof ViewGroup)
								SLib.SubstituteStringSignatures(AppCtx, (ViewGroup) vg);
						}
						try {
							StyloQDatabase db = AppCtx.GetDB();
							{
								StyloQConfig cfg_data = new StyloQConfig();
								StyloQDatabase.SecStoragePacket pack = db.GetOwnPeerEntry();
								if(pack != null) {
									byte[] cfg_bytes = pack.Pool.Get(SecretTagPool.tagPrivateConfig);
									if(SLib.GetLen(cfg_bytes) > 0) {
										String cfg_json = new String(cfg_bytes);
										cfg_data.FromJson(cfg_json);
										String nad_text = cfg_data.Get(StyloQConfig.tagNotificationActualDays);
										NotificationActualDays = SLib.satoi(nad_text);
										if(NotificationActualDays < 0)
											NotificationActualDays = 0;
									}
								}
							}
							MakeListData(db);
							ScheduleRTmr(new RefreshTimerTask(), 1000, 750);
						} catch(StyloQException exn) {
							;
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
				{
					SLib.RecyclerListAdapter adapter = SLib.IsRecyclerListAdapter(srcObj) ? (SLib.RecyclerListAdapter)srcObj : null;
					result = new Integer((adapter != null && ListData != null) ? ListData.size() : 0);
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemIdx >= 0) {
						if(ev_subj.RvHolder != null) {
							// RecyclerView
							if(SLib.IsRecyclerListAdapter(srcObj)) {
								SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
								if(a.GetListRcId() == R.id.notificationListView) {
									if(ListData != null && ev_subj.ItemIdx < ListData.size()) {
										//CPM.FindGoodsItemByGoodsID()
										StyloQInterchange.SvcNotification cur_entry = ListData.get(ev_subj.ItemIdx);
										View iv = ev_subj.RvHolder.itemView;
										String svc_name = null;
										byte [] svc_ident = null;
										StyloQCommand.Item cmd_item = null;
										if(cur_entry.SvcID > 0 && AppCtx != null) {
											try {
												StyloQDatabase db = AppCtx.GetDB();
												if(db != null) {
													StyloQDatabase.SecStoragePacket svc_pack = db.GetPeerEntry(cur_entry.SvcID);
													if(svc_pack != null) {
														svc_ident = svc_pack.Rec.BI;
														svc_name = svc_pack.GetSvcName(null);
														if(SLib.GetLen(svc_ident) > 0) {
															StyloQDatabase.SecStoragePacket cmdl_pack = db.GetForeignSvcCommandList(svc_ident);
															StyloQCommand.List cmd_list = (cmdl_pack != null) ? cmdl_pack.GetCommandList() : null;
															cmd_item = (cmd_list != null) ? cmd_list.GetByUuid(cur_entry.CmdUuid) : null;
														}
													}
												}
											} catch(StyloQException exn) {
												;
											}
										}
										{
											StyloQCommand.CommandPrestatus prestatus = AppCtx.GetCommandStatus(svc_ident, cmd_item);
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
												ImageView ctl = (ImageView)iv.findViewById(R.id.CTL_IND_STATUS);
												if(ctl != null) {
													int rcid = 0;
													switch(prestatus.S) {
														case StyloQCommand.prestatusQueryNeeded: rcid = R.drawable.ic_generic_server; break;
														case StyloQCommand.prestatusActualResultStored: rcid = R.drawable.ic_generic_document; break;
														case StyloQCommand.prestatusPending: rcid = R.drawable.ic_stopwatch; break;
														case StyloQCommand.prestatusUnkn: rcid = 0; break;
														default: rcid = /*R.drawable.ic_generic_command*/0; break;
													}
													ctl.setImageResource(rcid);
												}
											}
										}
										SLib.SetCtrlString(iv, R.id.CTL_NOTIFICATION_SVC, /*"some service"*/svc_name); // @todo service name
										SLib.SetCtrlString(iv, R.id.CTL_NOTIFICATION_TIME, SLib.datetimefmt(cur_entry.EventOrgTime, SLib.DATF_DMY, SLib.TIMF_HM));
										SLib.SetCtrlString(iv, R.id.CTL_NOTIFICATION_TEXT, cur_entry.Message);
										if(AppCtx != null && !cur_entry.Processed) {
											AppCtx.RegisterSeenNotification(cur_entry.InternalID);
										}
									}
								}
							}
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && AppCtx != null && SLib.IsInRange(ev_subj.ItemIdx, ListData)) {
						StyloQInterchange.SvcNotification cur_entry = ListData.get(ev_subj.ItemIdx);
						if(cur_entry != null) {
							if(cur_entry.SvcID > 0 && cur_entry.CmdUuid != null) {
								try {
									StyloQDatabase db = AppCtx.GetDB();
									StyloQDatabase.SecStoragePacket svc_pack = db.GetPeerEntry(cur_entry.SvcID);
									if(svc_pack != null && svc_pack.Rec.Kind == StyloQDatabase.SecStoragePacket.kForeignService) {
										final byte [] svc_ident = svc_pack.Rec.BI;
										if(SLib.GetLen(svc_ident) > 0) {
											StyloQDatabase.SecStoragePacket cmdl_pack = db.GetForeignSvcCommandList(svc_ident);
											StyloQCommand.List cmd_list = (cmdl_pack != null) ? cmdl_pack.GetCommandList() : null;
											StyloQCommand.Item cmd_item = (cmd_list != null) ? cmd_list.GetByUuid(cur_entry.CmdUuid) : null;
											if(cmd_item != null && StyloQCommand.IsCommandPending(svc_ident, cmd_item) == 0) {
												AppCtx.RunSvcCommand(svc_ident, cmd_item, null, false, null);
											}
										}
									}
								} catch(StyloQException exn) {
									;
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