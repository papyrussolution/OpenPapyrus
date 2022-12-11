package ru.petroglif.styloq;

import android.view.View;

import java.util.ArrayList;

public class NotificationActivity extends SLib.SlActivity {
	private StyloQApp AppCtx;
	private ArrayList <StyloQInterchange.SvcNotification> ListData;
	public NotificationActivity()
	{
		super();
		AppCtx = null;
		ListData = null;
	}
	private void MakeListData(StyloQDatabase db)
	{
		if(db != null)
			ListData = db.GetNotifivationList(0, null,false);
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
						try {
							StyloQDatabase db = AppCtx.GetDB();
							MakeListData(db);
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
										SLib.SetCtrlString(iv, R.id.CTL_NOTIFICATION_SVC, "some service"); // @todo service name
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
		}
		return result;
	}
}