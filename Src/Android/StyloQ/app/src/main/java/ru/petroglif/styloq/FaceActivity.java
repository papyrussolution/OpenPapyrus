// FaceActivity.java 
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.RadioGroup;

import androidx.fragment.app.Fragment;
import androidx.viewpager2.widget.ViewPager2;
import com.google.android.material.tabs.TabLayout;
import java.util.ArrayList;

public class FaceActivity extends SLib.SlActivity {
	private StyloQFace Data;
	private int CurrentLangId;
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
	private ArrayList <TabEntry> TabList;
	private void CreateTabList()
	{
		StyloQApp app = (StyloQApp)getApplicationContext();
		if(app != null && TabList == null) {
			TabList = new ArrayList<TabEntry>();
			LayoutInflater inflater = LayoutInflater.from(this);
			{
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(R.layout.layout_face_basic, R.id.TABLAYOUT_STQFACE);
				TabList.add(new TabEntry(1, SLib.ExpandString(app, "@{general}"), (Fragment)f));
			}
			{
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(R.layout.layout_face_address, R.id.TABLAYOUT_STQFACE);
				TabList.add(new TabEntry(2, SLib.ExpandString(app, "@{address}"), (Fragment)f));
			}
			{
				SLib.SlFragmentStatic f = SLib.SlFragmentStatic.newInstance(R.layout.layout_face_ids, R.id.TABLAYOUT_STQFACE);
				TabList.add(new TabEntry(3, SLib.ExpandString(app, "@{identifier_pl}"), (Fragment)f));
			}
		}
	}
	private void GetFragmentData(Object entry)
	{
		if(entry != null) {
			ViewGroup vg = null;
			if(entry instanceof SLib.SlFragmentStatic) {
				View v = ((SLib.SlFragmentStatic) entry).getView();
				if(v instanceof ViewGroup)
					vg = (ViewGroup)v;
			}
			else if(entry instanceof ViewGroup)
				vg = (ViewGroup)entry;
			if(vg != null) {
				int vg_id = vg.getId();
				if(vg_id == R.id.LAYOUT_STQFACE_BASIC) {
					View vrbv = findViewById(R.id.CTL_STQFACE_VRF);
					if(vrbv != null && vrbv instanceof RadioGroup) {
						StyloQFace.Verifiability _v;
						RadioGroup vrbc = (RadioGroup)vrbv;
						int ci = vrbc.getCheckedRadioButtonId();
						if(ci == R.id.CTL_STQFACE_VRF_VRF)
							_v = StyloQFace.Verifiability.vVerifiable;
						else if(ci == R.id.CTL_STQFACE_VRF_ANON)
							_v = StyloQFace.Verifiability.vAnonymous;
						else
							_v = StyloQFace.Verifiability.vArbitrary;
						Data.SetVerifiability(_v);
					}
					Data.Set(StyloQFace.tagCommonName, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_CN));
					Data.Set(StyloQFace.tagName, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_NAME));
					Data.Set(StyloQFace.tagPatronymic, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_PATRONYMIC));
					Data.Set(StyloQFace.tagSurName, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_SURNAME));
					Data.Set(StyloQFace.tagPhone, 0, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_PHONE));
					// @todo DayOfBirth
				}
				else if(vg_id == R.id.LAYOUT_STQFACE_ADDRESS) {
					Data.Set(StyloQFace.tagCountryName, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_COUNTRY));
					Data.Set(StyloQFace.tagCityName, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_CITY));
					Data.Set(StyloQFace.tagStreet, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_STREET));
				}
				else if(vg_id == R.id.LAYOUT_STQFACE_IDS) {
					Data.Set(StyloQFace.tagGLN, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_GLN));
					Data.Set(StyloQFace.tagRuINN, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_RUINN));
					Data.Set(StyloQFace.tagRuKPP, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_RUKPP));
					Data.Set(StyloQFace.tagRuSnils, CurrentLangId, SLib.GetCtrlString(vg, R.id.CTL_STQFACE_RUSNILS));
				}
			}
		}
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				{
					Intent intent = getIntent();
					long   managed_id = intent.getLongExtra("ManagedLongId", 0);
					String face_json = intent.getStringExtra("StyloQFaceJson");
					Data = new StyloQFace();
					if(SLib.GetLen(face_json) > 0) {
						Data.FromJson(face_json);
						Data.ID = managed_id;
					}
				}
				requestWindowFeature(Window.FEATURE_NO_TITLE);
				setContentView(R.layout.activity_face);
				ViewPager2 view_pager = (ViewPager2)findViewById(R.id.VIEWPAGER_STQFACE);
				SetupViewPagerWithFragmentAdapter(R.id.VIEWPAGER_STQFACE);
				{
					TabLayout lo_tab = findViewById(R.id.TABLAYOUT_STQFACE);
					if(lo_tab != null) {
						CreateTabList();
						for(int i = 0; i < TabList.size(); i++) {
							TabLayout.Tab tab = lo_tab.newTab();
							tab.setText(TabList.get(i).TabText);
							lo_tab.addTab(tab);
							//TabLayout.TabView;
						}
						SLib.SetupTabLayoutStyle(lo_tab);
						SLib.SetupTabLayoutListener(lo_tab, view_pager);
					}
				}
				{
					Context app_ctx = getApplicationContext();
					if(app_ctx instanceof StyloQApp) {
						View vg = findViewById(R.id.LAYOUT_ACTIVITYROOT);
						if(vg != null && vg instanceof ViewGroup)
							SLib.SubstituteStringSignatures((StyloQApp)app_ctx, (ViewGroup)vg);
					}
				}
				//SetDTS(Data);
				break;
			case SLib.EV_LISTVIEWCOUNT:
				CreateTabList();
				result = new Integer(TabList.size());
				break;
			/*
			case SLib.EV_CREATEVIEWHOLDER:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null && ev_subj.ItemView != null) {
						//if(ev_subj instanceof ViewGroup)
						CreateTabList();
						//LinearLayout _lo = null;//CreateItemLayout(Vdl, 0);
						ViewGroup group = null; //(ViewGroup)findViewById();
						if(group != null) {
							//((ViewGroup)ev_subj.ItemView).addView(_lo);
							result = new SLib.RecyclerListViewHolder(group);
						}
					}
				}
				break;
			*/
			case SLib.EV_CREATEFRAGMENT:
				if(subj instanceof Integer) {
					int item_idx = (Integer)subj;
					if(TabList != null && item_idx >= 0 && item_idx < TabList.size()) {
						TabEntry cur_entry = (TabEntry)TabList.get(item_idx);
						result = cur_entry.TabView;
					}
				}
				break;
			/*
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						if(ev_subj.RvHolder != null) {
							// RecyclerView
							if(TabList != null && ev_subj.ItemIdx >= 0 && ev_subj.ItemIdx < TabList.size()) {
								//
								View iv = ev_subj.RvHolder.itemView;
								TabEntry cur_entry = (TabEntry)TabList.get(ev_subj.ItemIdx);
								//ctl.setText(cur_entry.Name);
								//holder.flagView.setImageResource(state.getFlagResource());
								//{
								//	FragmentManager fmgr = getFragmentManager();
								//	FragmentTransaction ftra = fmgr.beginTransaction();
									// добавляем фрагмент
								//	ftra.replace(R.id.LAYOUT_FRAGMENTCONTAIER_COMMON, cur_entry.TabView, cur_entry.TabText);
								//	ftra.commit();
								//}
							}
						}
						else {
							;
						}
					}
				}
				break;
			*/
			case SLib.EV_SETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup) {
					ViewGroup vg = (ViewGroup)srcObj;
					int vg_id = vg.getId();
					if(vg_id == R.id.LAYOUT_STQFACE_BASIC) {
						StyloQFace.Verifiability _v = Data.GetVerifiability();
						//String is_verifiable_text =  Data.Get(StyloQFace.tagVerifiable_Depricated, 0);
						//boolean is_verifiable = false;
						//if(SLib.GetLen(is_verifiable_text) > 0 && is_verifiable_text.equalsIgnoreCase("true"))
							//is_verifiable = true;
						//SLib.SetCheckboxState(vg, R.id.CTL_STQFACE_VERIFIABLE, is_verifiable);
						View vrbv = findViewById(R.id.CTL_STQFACE_VRF);
						if(vrbv != null && vrbv instanceof RadioGroup) {
							RadioGroup vrbc = (RadioGroup)vrbv;
							if(_v == StyloQFace.Verifiability.vVerifiable) {
								vrbc.check(R.id.CTL_STQFACE_VRF_VRF);
							}
							else if(_v == StyloQFace.Verifiability.vAnonymous) {
								vrbc.check(R.id.CTL_STQFACE_VRF_ANON);
							}
							else /*if(_v == StyloQFace.Verifiability.vArbitrary)*/ {
								vrbc.check(R.id.CTL_STQFACE_VRF_ARB);
							}
						}
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_CN, Data.Get(StyloQFace.tagCommonName, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_NAME, Data.Get(StyloQFace.tagName, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_PATRONYMIC, Data.Get(StyloQFace.tagPatronymic, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_SURNAME, Data.Get(StyloQFace.tagSurName, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_PHONE, Data.Get(StyloQFace.tagPhone, 0));
						// @todo DayOfBirth
					}
					else if(vg_id == R.id.LAYOUT_STQFACE_ADDRESS) {
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_COUNTRY, Data.Get(StyloQFace.tagCountryName, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_CITY, Data.Get(StyloQFace.tagCityName, CurrentLangId));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_STREET, Data.Get(StyloQFace.tagStreet, CurrentLangId));
					}
					else if(vg_id == R.id.LAYOUT_STQFACE_IDS) {
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_GLN, Data.Get(StyloQFace.tagGLN, 0));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_RUINN, Data.Get(StyloQFace.tagRuINN, 0));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_RUKPP, Data.Get(StyloQFace.tagRuKPP, 0));
						SLib.SetCtrlString(vg, R.id.CTL_STQFACE_RUSNILS, Data.Get(StyloQFace.tagRuSnils, 0));
					}
				}
				break;
			case SLib.EV_GETVIEWDATA:
				if(srcObj != null && srcObj instanceof ViewGroup)
					GetFragmentData(srcObj);
				break;
			case SLib.EV_COMMAND:
				int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.STDCTL_OKBUTTON) {
					for(int tidx = 0; tidx < TabList.size(); tidx++) {
						GetFragmentData(TabList.get(tidx).TabView);
					}
					Intent intent = new Intent();
					String result_json = Data.ToJson();
					if(result_json != null) {
						intent.putExtra("StyloQFaceJson", result_json);
						intent.putExtra("ManagedLongId", Data.ID);
					}
					setResult(RESULT_OK, intent);
					finish();
				}
				else if(view_id == R.id.STDCTL_CANCELBUTTON) {
					setResult(RESULT_CANCELED, null);
					finish();
				}
				break;
		}
		return result;
	}
}