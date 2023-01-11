// HelpPane.java
// Copyright (c) A.Sobolev 2023
//
package ru.petroglif.styloq;

import android.content.Context;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;

import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import java.util.ArrayList;

public class HelpPane extends SLib.SlDialog {
	SLib.SlActivity ActivityCtx;
	//DialogData Data;
	public static class LegendEntry {
		LegendEntry()
		{
			IconId = 0;
			Descr = null;
		}
		LegendEntry(int iconId, String descr)
		{
			IconId = iconId;
			Descr = descr;
		}
		int    IconId;
		String Descr;
	}
	public static class DialogData {
		DialogData()
		{
			Text = null;
			Legend = null;
		}
		String Text;
		ArrayList <LegendEntry> Legend;
	}
	public HelpPane(Context ctx, Object data)
	{
		super(ctx, R.id.DLG_HELP_ICONLEGEND, data);
		if(ctx != null && ctx instanceof SLib.SlActivity)
			ActivityCtx = (SLib.SlActivity)ctx;
	}
	@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				requestWindowFeature(Window.FEATURE_NO_TITLE);
				setContentView(R.layout.dialog_hlp_iconlegend);
				SetDTS(Data);
				break;
			case SLib.EV_COMMAND:
				if(srcObj != null && srcObj instanceof View) {
					final int view_id = ((View)srcObj).getId();
					if(view_id == R.id.STDCTL_CLOSEBUTTON)
						this.dismiss();
				}
				break;
			case SLib.EV_LISTVIEWCOUNT:
			{
				SLib.RecyclerListAdapter a = (SLib.RecyclerListAdapter)srcObj;
				if(a.GetListRcId() == R.id.CTL_HELP_ICONLEGENDLIST) {
					DialogData _data = (Data != null && Data instanceof DialogData) ? (DialogData)Data : null;
					result = new Integer((_data != null && _data.Legend != null) ? _data.Legend.size() : 0);
				}
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
							if(a.GetListRcId() == R.id.CTL_HELP_ICONLEGENDLIST) {
								StyloQApp app_ctx = (ActivityCtx != null) ? ActivityCtx.GetAppCtx() : null;
								DialogData _data = (Data != null && Data instanceof DialogData) ? (DialogData)Data : null;
								if(app_ctx != null && _data != null && _data.Legend != null && ev_subj.ItemIdx < _data.Legend.size()) {
									LegendEntry cur_entry = _data.Legend.get(ev_subj.ItemIdx);
									if(cur_entry != null) {
										View iv = ev_subj.RvHolder.itemView;
										SLib.SetCtrlString(iv, R.id.CTL_ICONLEGEND_TEXT, SLib.ExpandString(app_ctx, cur_entry.Descr));
										View img_view = iv.findViewById(R.id.CTL_ICONLEGEND_ICON);
										if(img_view != null && img_view instanceof ImageView) {
											((ImageView) img_view).setImageResource(cur_entry.IconId);
										}
										//CTL_ICONLEGEND_ICON
										//CTL_ICONLEGEND_TEXT
										//SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_CODE, cur_entry.BillCode);
										//SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_DATE, cur_entry.BillDate.Format(SLib.DATF_DMY));
										//SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_AMOUNT, SLib.formatdouble(cur_entry.Amount, 2));
										//SLib.SetCtrlString(iv, R.id.CTL_DEBTDETAILLISTITEM_DEBT, SLib.formatdouble(cur_entry.Debt, 2));
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
	boolean SetDTS(Object objData)
	{
		boolean ok = true;
		StyloQApp app_ctx = (ActivityCtx != null) ? ActivityCtx.GetAppCtx() : null;
		if(app_ctx != null) {
			//Context ctx = getContext();
			DialogData _data = (Data != null && Data instanceof DialogData) ? (DialogData)Data : null;
			if(_data != null) {
				if(SLib.GetLen(_data.Text) > 0) {
					SLib.SetCtrlString(this, R.id.CTL_HELP_TEXT, SLib.ExpandString(app_ctx, _data.Text));
				}
				if(_data.Legend != null) {

				}
				View lv = findViewById(R.id.CTL_HELP_ICONLEGENDLIST);
				if(lv != null && lv instanceof RecyclerView) {
					((RecyclerView)lv).setLayoutManager(new LinearLayoutManager(ActivityCtx));
					SLib.RecyclerListAdapter adapter = new SLib.RecyclerListAdapter(ActivityCtx, this, R.id.CTL_HELP_ICONLEGENDLIST, R.layout.li_iconlegend);
					((RecyclerView) lv).setAdapter(adapter);
				}
			}
		}
		return ok;
	}
	Object GetDTS()
	{
		Object result = Data;
		return result;
	}
}
