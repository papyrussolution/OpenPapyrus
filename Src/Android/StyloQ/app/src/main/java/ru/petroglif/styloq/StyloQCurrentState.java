// StyloQCurrentState.java
// Copyright (c) A.Sobolev 2023
// Реализует структуру хранения текущего состояния. Это состояние дифференцируется по
// сервисам и командам сервисов. Состояние для всех команд одного сервиса хранится в
// отдельной записи реестра StyloQ.
//
package ru.petroglif.styloq;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Base64;
import java.util.UUID;

/*
CurrentState scheme:
{
	commandUuid: GUID
	openedDocID: int
	CommonPrereqModule.RegistryFilt: object
}
 */
public class StyloQCurrentState { // @v11.7.0 @construction
	StyloQCurrentState(final byte [] svcIdent, final UUID orgCmdUuid)
	{
		SvcIdent = svcIdent;
		OrgCmdUuid = orgCmdUuid;
		OpenedDocID = 0;
		Rf = null;
	}
	public boolean IsEmpty()
	{
		return (OpenedDocID == 0 && (Rf == null || Rf.IsEmpty()));
	}
	public JSONObject ToJsonObj()
	{
		JSONObject result = null;
		if(SvcIdent != null && OrgCmdUuid != null) {
			try {
				result = new JSONObject();
				String svc_ident_text = Base64.getEncoder().encodeToString(SvcIdent);
				result.put("svcident", svc_ident_text);
				//
				String cmd_text = OrgCmdUuid.toString();
				result.put("orgcmduuid", cmd_text);
				if(OpenedDocID > 0) {
					result.put("openeddocid", OpenedDocID);
				}
				if(Rf != null && !Rf.IsEmpty()) {
					JSONObject js_rf = Rf.ToJsonObj();
					if(js_rf != null) {
						result.put("registryfilt", js_rf);
					}
				}
			} catch(JSONException exn) {
				result = null;
			}
		}
		return result;
	}
	public boolean FromJsonObj(JSONObject jsObj)
	{
		boolean result = false;
		if(jsObj != null) {
			{
				String svc_ident_hex = jsObj.optString("svcident", null);
				if(SLib.GetLen(svc_ident_hex) > 0)
					SvcIdent = Base64.getDecoder().decode(svc_ident_hex);
				else
					SvcIdent = null;
				OrgCmdUuid = SLib.strtouuid(jsObj.optString("orgcmduuid", null));
				OpenedDocID = jsObj.optInt("openeddocid");
				JSONObject js_rf = jsObj.optJSONObject("registryfilt");
				if(js_rf != null) {
					Rf = new CommonPrereqModule.RegistryFilt();
					if(!Rf.FromJsonObj(js_rf))
						Rf = null;
				}
				if(SvcIdent != null && OrgCmdUuid != null)
					result = true;
			}
		}
		return result;
	}
	byte [] SvcIdent;
	UUID  OrgCmdUuid;
	long  OpenedDocID; // Документ, который был открыт на момент остановки активити
		// команды, допускающей редактирование документов.
	CommonPrereqModule.RegistryFilt Rf; // Для команд, которые предусматривают
		// просмотр списка собственных документов - фильтр отображения этих документов.
}
