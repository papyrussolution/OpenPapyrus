package ru.petroglif.styloq;

import android.content.Intent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import org.json.JSONException;
import org.json.JSONObject;

public class DetailActivity extends SLib.SlActivity {
	private String ResultText;
	private byte [] SvcIdent;
	private SLib.PPObjID Oid;
	public DetailActivity()
	{
		super();
		ResultText = null;
		SvcIdent = null;
		Oid = null;
	}
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_detail);
				StyloQApp app_ctx = GetAppCtx();
				if(app_ctx != null) {
					try {
						StyloQDatabase db = app_ctx.GetDB();
						Intent intent = getIntent();
						ResultText = intent.getStringExtra("SvcReplyText");
						SvcIdent = intent.getByteArrayExtra("SvcIdent");
						if(SLib.GetLen(SvcIdent) > 0) {
							StyloQDatabase.SecStoragePacket svc_pack = db.SearchGlobalIdentEntry(StyloQDatabase.SecStoragePacket.kForeignService, SvcIdent);
							String blob_signature = null;
							if(svc_pack != null) {
								StyloQFace face = svc_pack.GetFace();
								if(face != null)
									blob_signature = face.Get(StyloQFace.tagImageBlobSignature, 0);
								SLib.SetCtrlString(this, R.id.CTL_PAGEHEADER_SVCTITLE, svc_pack.GetSvcName(face));
							}
							SLib.SetupImage(this, findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature);
						}
						if(SLib.GetLen(ResultText) > 0) {
							JSONObject jsobj = new JSONObject(ResultText);
							if(jsobj != null) {
								int repl_result = StyloQInterchange.GetReplyResult(jsobj);
								String reply_msg = jsobj.optString("msg");
								String reply_errmsg = jsobj.optString("errmsg");

								//SLib.SetupImage(this, findViewById(R.id.CTLIMG_PAGEHEADER_SVC), blob_signature);

								Oid = SLib.PPObjID.Identify(jsobj.optString("objtype", null), jsobj.optString("objid", null));
								if(Oid != null) {
									if(Oid.Type == SLib.PPOBJ_GOODS) {
										View vroot = findViewById(R.id.LAYOUT_ACTIVITYROOT);
										if(vroot != null && vroot instanceof ViewGroup) {
											LayoutInflater inflater = this.getLayoutInflater();
											View vchild = inflater.inflate(R.layout.layout_detail_ware, (ViewGroup) vroot, false);
											if(vchild != null) {
												JSONObject js_detail = jsobj.optJSONObject("detail");
												if(js_detail != null) {
													((ViewGroup) vroot).addView(vchild);
													SLib.SetCtrlString(this, R.id.CTL_GOODSDETAIL_NAME, js_detail.optString("nm", null));
													View vimg = findViewById(R.id.CTL_GOODSDETAIL_IMG);
													if(vimg != null)
														SLib.SetupImage(this, vimg, js_detail.optString("imgblobs", null));
												}
											}
										}
									}
									else if(Oid.Type == SLib.PPOBJ_PROCESSOR) {

									}
									else if(Oid.Type == SLib.PPOBJ_STYLOQBINDERY) {

									}
								}
								/*if(SLib.GetLen(reply_msg) > 0) {
									SLib.SetCtrlString(this, R.id.cmdResultText, reply_msg);
								}
								else if(SLib.GetLen(reply_errmsg) > 0) {
									SLib.SetCtrlString(this, R.id.cmdResultText, reply_errmsg);
								}*/
							}
							//SetCtrlString()
						}
					} catch(JSONException exn) {
						;//e.printStackTrace();
					} catch(StyloQException exn) {
						;
					}
				}
				break;
		}
		return result;
	}
}