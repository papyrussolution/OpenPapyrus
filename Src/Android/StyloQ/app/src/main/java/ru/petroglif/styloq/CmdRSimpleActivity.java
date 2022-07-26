// CmdRSimpleActivity.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import android.content.Intent;
import org.json.JSONException;
import org.json.JSONObject;

public class CmdRSimpleActivity extends SLib.SlActivity {
	private String ResultText;
	public Object HandleEvent(int ev, Object srcObj, Object subj)
	{
		Object result = null;
		switch(ev) {
			case SLib.EV_CREATE:
				setContentView(R.layout.activity_cmdrsimple);
				Intent intent = getIntent();
				ResultText = intent.getStringExtra("SvcReplyText");
				if(SLib.GetLen(ResultText) > 0) {
					try {
						JSONObject jsobj = new JSONObject(ResultText);
						if(jsobj != null) {
							StyloQInterchange.CommonReplyResult crr = StyloQInterchange.GetReplyResult(jsobj);
							if(SLib.GetLen(crr.Msg) > 0)
								SLib.SetCtrlString(this, R.id.cmdResultText, crr.Msg);
							else if(SLib.GetLen(crr.ErrMsg) > 0)
								SLib.SetCtrlString(this, R.id.cmdResultText, crr.ErrMsg);
						}
					} catch(JSONException e) {
						//e.printStackTrace();
					}
					//SetCtrlString()
				}
				break;
		}
		return result;
	}
}