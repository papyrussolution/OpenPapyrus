// FaceListActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.view.View;
import android.view.Window;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.activity.result.ActivityResult;
import androidx.annotation.RequiresApi;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

public class FaceListActivity extends SLib.SlActivity/*AppCompatActivity*/ {
	static class FaceDialog extends SLib.SlDialog {
		private int CurrentLangId;

		@Override public Object HandleEvent(int ev, Object srcObj, Object subj)
		{
			Object result = null;
			switch(ev) {
				case SLib.EV_CREATE:
					requestWindowFeature(Window.FEATURE_NO_TITLE);
					setContentView(R.layout.dialog_face);
					SetDTS(Data);
					break;
				case SLib.EV_COMMAND:
					int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
					if(view_id == R.id.STDCTL_OKBUTTON) {
						Object data = GetDTS();
						if(data != null) {
							StyloQApp app_ctx = SLib.SlActivity.GetAppCtx(getContext());
							if(app_ctx != null)
								app_ctx.HandleEvent(SLib.EV_IADATAEDITCOMMIT, this, data);
						}
						this.dismiss(); // Close Dialog
					}
					else if(view_id == R.id.STDCTL_CANCELBUTTON) {
						this.dismiss();
					}
					break;
			}
			return result;
		}
		FaceDialog(Context ctx, Object data)
		{
			super(ctx, R.id.DLG_SQFACE, data);
			if(data instanceof StyloQFace)
				Data = data;
			//ResultListener = resultListener;
			CurrentLangId = 0;
		}
		boolean SetDTS(Object objData)
		{
			if(objData != null && objData.getClass() == Data.getClass()) {
				StyloQFace _data = null;
				if(Data != null && Data.getClass().getSimpleName().equals("StyloQFace"))
					_data = (StyloQFace)Data;
				else {
					_data = new StyloQFace();
					Data = _data;
				}
				String text;
				Data = (StyloQFace)objData;
				SLib.SetCtrlString(this, R.id.CTL_SQFACE_CN, _data.Get(StyloQFace.tagCommonName, CurrentLangId));
				SLib.SetCtrlString(this, R.id.CTL_SQFACE_NAME, _data.Get(StyloQFace.tagName, CurrentLangId));
				SLib.SetCtrlString(this, R.id.CTL_SQFACE_PATRONYMIC, _data.Get(StyloQFace.tagPatronymic, CurrentLangId));
				SLib.SetCtrlString(this, R.id.CTL_SQFACE_SURNAME, _data.Get(StyloQFace.tagSurName, CurrentLangId));
			}
			return true;
		}
		Object GetDTS()
		{
			StyloQFace _data = null;
			if(Data != null && Data.getClass().getSimpleName().equals("StyloQFace"))
				_data = (StyloQFace)Data;
			else {
				_data = new StyloQFace();
				Data = _data;
			}
			_data.Set(StyloQFace.tagCommonName, CurrentLangId, SLib.GetCtrlString(this, R.id.CTL_SQFACE_CN));
			_data.Set(StyloQFace.tagName, CurrentLangId, SLib.GetCtrlString(this, R.id.CTL_SQFACE_NAME));
			_data.Set(StyloQFace.tagPatronymic, CurrentLangId, SLib.GetCtrlString(this, R.id.CTL_SQFACE_PATRONYMIC));
			_data.Set(StyloQFace.tagSurName, CurrentLangId, SLib.GetCtrlString(this, R.id.CTL_SQFACE_SURNAME));
			return Data;
		}
	}
	private StyloQDatabase Db;
	private ArrayList <StyloQFace> ListData;
	@RequiresApi(api = Build.VERSION_CODES.KITKAT)
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_face_list);
				{
					StyloQApp app_ctx = GetAppCtx();
					if(app_ctx != null) {
						try {
							Db = app_ctx.GetDB();
							ListData = (Db != null) ? Db.GetFaceList() : new ArrayList<StyloQFace>();
						} catch(StyloQException exn) {
							Db = null;
							ListData = new ArrayList<StyloQFace>();
							app_ctx.DisplayError(this, exn, 5000);
						}
						SetupListView(R.id.faceListView, R.layout.li_face, ListData);
					}
				}
				break;
			case SLib.EV_GETLISTITEMVIEW:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQFace item = (ev_subj.ItemObj != null && ev_subj.ItemObj instanceof StyloQFace) ? (StyloQFace)ev_subj.ItemObj : null;
						TextView ctl = (TextView)ev_subj.ItemView.findViewById(R.id.LVITEM_GENERICNAME);
						if(item != null && ctl != null) {
							ctl.setText(item.GetSimpleText(0));
							ImageView imgv = (ImageView)ev_subj.ItemView.findViewById(R.id.LVITEM_FACEVRF);
							if(imgv != null) {
								int _v = item.GetVerifiability();
								int rcimg = 0;
								if(_v == StyloQFace.vAnonymous)
									rcimg = R.drawable.ic_faceanonymous01;
								else if(_v == StyloQFace.vVerifiable)
									rcimg = R.drawable.ic_faceverifiable01;
								else /*if(_v == StyloQFace.Verifiability.vArbitrary)*/
									rcimg = R.drawable.ic_facearbitrary01;
								if(rcimg != 0)
									imgv.setImageResource(rcimg);
							}
							result = ev_subj.ItemView;
						}
					}
				}
				break;
			case SLib.EV_LISTVIEWITEMCLK:
				{
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent)subj : null;
					if(ev_subj != null) {
						StyloQFace item = (ev_subj.ItemObj != null && ev_subj.ItemObj instanceof StyloQFace) ? (StyloQFace) ev_subj.ItemObj : null;
						if(item != null) {
							//FaceDialog dialog = new FaceDialog(this, item);
							//dialog.show();
							String __data = item.ToJson();
							if(__data != null) {
								int editflags = 0;
								if(ListData != null) {
									for(int i = 0; i < ListData.size(); i++) {
										final StyloQFace iter_item = ListData.get(i);
										if(iter_item != null && iter_item.ID != item.ID) {
											if(item.GetVerifiability() != StyloQFace.vAnonymous && iter_item.GetVerifiability() == StyloQFace.vAnonymous)
												editflags |= StyloQFace.editfDisableAnonym;
											else if(item.GetVerifiability() != StyloQFace.vVerifiable && iter_item.GetVerifiability() == StyloQFace.vVerifiable)
												editflags |= StyloQFace.editfDisableVerifiable;
										}
									}
								}
								if(Db.IsThereFaceRefs(item.BI))
									editflags |= StyloQFace.editfDisableDeletion;
								Intent intent = new Intent(this, FaceActivity.class);
								intent.putExtra("StyloQFaceJson", __data);
								intent.putExtra("ManagedLongId", item.ID);
								intent.putExtra("EditFlags", editflags);
								//startActivity(intent);
								LaunchOtherActivity(intent);
							}
						}
					}
				}
				break;
			case SLib.EV_COMMAND:
				int view_id = View.class.isInstance(srcObj) ? ((View)srcObj).getId() : 0;
				if(view_id == R.id.tbButtonAdd) {
					//FaceDialog dialog = new FaceDialog(this, new StyloQFace());
					//dialog.show();
					//Bundle __data_bundle = new Bundle();
					int editflags = 0;
					if(ListData != null) {
						for(int i = 0; i < ListData.size(); i++) {
							final StyloQFace iter_item = ListData.get(i);
							if(iter_item != null) {
								if(iter_item.GetVerifiability() == StyloQFace.vAnonymous)
									editflags |= StyloQFace.editfDisableAnonym;
								else if(iter_item.GetVerifiability() == StyloQFace.vVerifiable)
									editflags |= StyloQFace.editfDisableVerifiable;
							}
						}
					}
					Intent intent = new Intent(this, FaceActivity.class);
					intent.putExtra("EditFlags", editflags);
					LaunchOtherActivity(intent);
				}
				/*else if(view_id == R.id.tbButtonDelete) {
					SLib.ListViewEvent ev_subj = (subj instanceof SLib.ListViewEvent) ? (SLib.ListViewEvent) subj : null;
					if(ev_subj != null) {
						StyloQFace item = (ev_subj.ItemObj != null && ev_subj.ItemObj instanceof StyloQFace) ? (StyloQFace) ev_subj.ItemObj : null;
						if(item != null && item.ID > 0) {
							try {
								final long id_to_remove = item.ID;
								long removed_id = Db.PutPeerEntry(id_to_remove, null);
								if(removed_id == id_to_remove) {
									int i = ListData.size();
									if(i > 0) do {
										i--;
										StyloQFace iter_item = ListData.get(i);
										if(iter_item != null && iter_item.ID == id_to_remove) {
											ListData.remove(i);
										}
									} while(i > 0);
									NotifyListView_DataSetChanged(R.id.faceListView);
								}
							} catch(StyloQException exn) {
								//exn.printStackTrace();
							}
						}
					}
				}
				break;
				 */
			case SLib.EV_ACTIVITYRESULT:
				if(subj != null && subj instanceof ActivityResult && ((ActivityResult)subj).getResultCode() == RESULT_OK) {
					Intent data = ((ActivityResult)subj).getData();
					String jstext = data.getStringExtra("StyloQFaceJson");
					long   managed_id = data.getLongExtra("ManagedLongId", 0);
					try {
						if(jstext == null && managed_id > 0) { // Deletion
							if(Db.PutPeerEntry(managed_id, null, true) != 0) {
								int i = ListData.size();
								if(i > 0) do {
									StyloQFace iter_item = ListData.get(--i);
									if(iter_item.ID == managed_id)
										ListData.remove(i);
								} while(i > 0);
								NotifyListView_DataSetChanged(R.id.faceListView);
							}
						}
						else if(SLib.GetLen(jstext) > 0) {
							StyloQFace pack = new StyloQFace();
							if(pack.FromJson(jstext)) {
								pack.ID = managed_id;
								StyloQDatabase.SecStoragePacket sp = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kFace);
								sp.Rec.ID = pack.ID;
								sp.Pool.Put(SecretTagPool.tagSelfyFace, jstext.getBytes(StandardCharsets.UTF_8));
								long new_id = Db.PutPeerEntry(sp.Rec.ID, sp, true);
								if(new_id > 0) {
									StyloQFace ex_item = null;
									int ex_item_idx = 0;
									for(int i = 0; ex_item == null && i < ListData.size(); i++) {
										StyloQFace iter_item = ListData.get(i);
										if(iter_item != null && iter_item.ID == new_id) {
											ex_item = iter_item;
											ex_item_idx = i;
										}
									}
									StyloQFace new_db_item = Db.GetFace(new_id, SecretTagPool.tagSelfyFace, ex_item);
									if(new_db_item != null) {
										if(ex_item == null)
											ListData.add(new_db_item);
										NotifyListView_DataSetChanged(R.id.faceListView);
									}
								}
							}
						}
					} catch(StyloQException exn) {
						exn.printStackTrace();
					}
				}
				break;
			case SLib.EV_IADATAEDITCOMMIT:
				if(srcObj != null && srcObj instanceof FaceDialog && subj != null && subj instanceof StyloQFace) {
					StyloQFace pack = (StyloQFace)subj;
					String jstext = pack.ToJson();
					if(Db != null && SLib.GetLen(jstext) > 0) {
						try {
							StyloQDatabase.SecStoragePacket sp = new StyloQDatabase.SecStoragePacket(StyloQDatabase.SecStoragePacket.kFace);
							sp.Rec.ID = pack.ID;
							sp.Pool.Put(SecretTagPool.tagSelfyFace, jstext.getBytes(StandardCharsets.UTF_8));
							long new_id = Db.PutPeerEntry(sp.Rec.ID, sp, true);
							if(new_id > 0) {
								StyloQFace ex_item = null;
								int ex_item_idx = 0;
								for(int i = 0; ex_item == null && i < ListData.size(); i++) {
									StyloQFace iter_item = ListData.get(i);
									if(iter_item != null && iter_item.ID == new_id) {
										ex_item = iter_item;
										ex_item_idx = i;
									}
								}
								StyloQFace new_db_item = Db.GetFace(new_id, SecretTagPool.tagSelfyFace, ex_item);
								if(new_db_item != null) {
									if(ex_item == null)
										ListData.add(new_db_item);
									NotifyListView_DataSetChanged(R.id.faceListView);
								}
							}
						} catch(StyloQException exn) {
							exn.printStackTrace();
						}
					}
				}
				break;
		}
		return result;
	}
	//@Override
	//public boolean onSupportNavigateUp()
	//{
		/*
		NavController navController = Navigation.findNavController(this, R.id.nav_host_fragment_content_face_list);
		return NavigationUI.navigateUp(navController, appBarConfiguration)
				|| super.onSupportNavigateUp();
		 */
		//return false;
	//}
}