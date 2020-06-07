// GLBSRVS.CPP
// Copyright (c) E.Sobolev 2020
//
#include <pp.h>
#pragma hdrstop

static const char * P_VKUrlBase = "https://api.vk.com";
static const char * P_VKMethodUrlBase = "https://api.vk.com/method";

struct TokenMsgVk 
{
	SString Token;
	SString TxtMsg;
	SString LinkFilePath;
	uint LinkFileType;
	//1 - фото
};

class SendVkMsgDlg: public TDialog {
	DECL_DIALOG_DATA(TokenMsgVk);
	const DlScope * P_Dl600Scope;
public:
	SendVkMsgDlg(TokenMsgVk &rToken): TDialog(DLG_VK_SENDMSG)
	{
		Data.Token = rToken.Token;
	}
	DECL_DIALOG_SETDTS()
	{

	}
	DECL_DIALOG_GETDTS()
	{

	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
	}

		void VKSendMsg()
	{

	}
};

class PostWallVkDlg: public TDialog {
	DECL_DIALOG_DATA(TokenMsgVk);
	const DlScope * P_Dl600Scope;
public:
	PostWallVkDlg(): TDialog(DLG_VK_POST)
	{

	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		setCtrlString(CTL_VK_POST_TEXT, Data.TxtMsg);
		//PPNamedFilt::ViewDefinition::Entry entry = *pEntry;
		
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{

	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);

		if(event.isCmd(cmSendVkPost)) {
			uint   sel = 0;
			getCtrlString(sel = CTL_VK_POST_TEXT, Data.TxtMsg);
			if(Data.Token.NotEmptyS()) {
				VKPostWall();
			}

		}
		else
			return;
		clearEvent(event);
	}

	void VKPostWall()
	{
		uint   sel = 0;
		getCtrlString(sel = CTL_VK_POST_TEXT, Data.TxtMsg);
		if(Data.Token.NotEmptyS() && Data.TxtMsg.NotEmptyS()) {
			int ok = 1;
			SString url = "";
			url.Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("wall.post?").CatEq("owner_id", "-195490808").Cat("&");
			SString temp_buf = Data.TxtMsg.Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
			url.CatEq("access_token", Data.Token).Cat("&").CatEq("message", temp_buf).Cat("&v=5.52");
			StringSet str_set;
			SString wr_file_name;
			{
				PPGetFilePath(PPPATH_OUT, "test-curl-get.txt", wr_file_name);
				SFile wr_stream(/*buffer*/wr_file_name, SFile::mWrite);
				ScURL c;
				//ok = c.HttpGet("https://api.vk.com/method/wall.post?owner_id=99311444&access_token=f5b377b9a0e2c6eb769f51a72ed375805bfe9ccfcc393ceef0988828c50d0ef7d9902ff0993fd7aec524f&message=Hello%20guys%20its%20test%20vk%20API&v=5.52", ScURL::mfDontVerifySslPeer, &wr_stream);
				ok = c.HttpGet(url, ScURL::mfDontVerifySslPeer, &wr_stream);
			}
		}
	}
};

class VkApiDlg: public TDialog {
	DECL_DIALOG_DATA(TokenMsgVk);
	const DlScope * P_Dl600Scope;
public:
	VkApiDlg(TokenMsgVk &rTokenMsg): TDialog(DLG_VK_API)
	{
		Data.Token = rTokenMsg.Token;
		Data.TxtMsg = rTokenMsg.TxtMsg;
	}
	DECL_DIALOG_SETDTS()
	{
		int    ok = 1;
		RVALUEPTR(Data, pData);
		//PPNamedFilt::ViewDefinition::Entry entry = *pEntry;
		setCtrlString(CTL_TESTVK_TOKEN, Data.Token); // Поле "Zone"
		setCtrlString(CTL_TESTVK_MSG, Data.TxtMsg); // Поле "FieldName"
		return ok;
	}
	DECL_DIALOG_GETDTS()
	{
		int    ok = 1;
		uint   sel = 0;     // Идентификатор управляющего элемента, данные из которого анализировались в момент ошибки
		//PPNamedFilt::ViewDefinition::Entry entry;
		getCtrlString(sel = CTL_TESTVK_TOKEN, Data.Token);
		THROW_PP(Data.Token.NotEmptyS(), PPERR_USERINPUT);
		getCtrlString(sel = CTL_TESTVK_MSG, Data.TxtMsg);
		THROW_PP(Data.TxtMsg.NotEmptyS(), PPERR_USERINPUT);
		ASSIGN_PTR(pData, Data);
		CATCH
			PPErrorByDialog(this, sel);
		ENDCATCH
			return ok;
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCmd(cmVkTestGTA)) {
			long scope = 8192;
			GetVKAccessToken(scope);
		}
		else if(event.isCmd(cmOpenDlgSendMsg)) {
			uint   sel = 0;
			getCtrlString(sel = CTL_TESTVK_TOKEN, Data.Token);
			if(Data.Token.NotEmptyS()) {
				SendVkMsgDlg * dlg = new SendVkMsgDlg(Data);
				if(CheckDialogPtrErr(&dlg)) {
					ExecViewAndDestroy(dlg);
				}
			}
			
		}
		else if(event.isCmd(cmOpenDlgPost)) {
			uint   sel = 0;
			getCtrlString(sel = CTL_TESTVK_TOKEN, Data.Token);
			if(Data.Token.NotEmptyS()) {
				PostWallVkDlg * dlg = new PostWallVkDlg();
				if(CheckDialogPtrErr(&dlg)) {
					ExecViewAndDestroy(dlg);
				}
			}			
		}
		else
			return;
		clearEvent(event);
	}

	void GetVKAccessToken(long& rScope)
	{
		SString url = "https://oauth.vk.com/authorize?client_id=7402217&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=12288&response_type=token&v=5.52/";
		ShellExecute(0, _T("open"), SUcSwitch(url), NULL, NULL, SW_SHOWNORMAL); // @unicodeproblem
	}

};

void GotoVK(PPPsnEventPacket &rPack) //@erikTEMP v10.7.7 
{
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	PPGlobalUserAcc gua_rec;
	PPGlobalUserAccPacket gua_pack;
	gua_obj.SearchBySymb("vk_group", 0, &gua_rec);
	if(gua_obj.GetPacket(gua_rec.ID, &gua_pack)>0) {
		if(gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, temp_buf.Z())>0) {
			TokenMsgVk data;
			data.TxtMsg = rPack.Rec.Memo;
			data.LinkFileType = 1;
			rPack.LinkFiles.At(0, data.LinkFilePath);
			data.Token = temp_buf;
			PostWallVkDlg * dlg = new PostWallVkDlg();
			dlg->setDTS(&data);
			if(CheckDialogPtrErr(&dlg)) {
				ExecViewAndDestroy(dlg);
			}
		}		
	}
}

int PPVkClient::LogIn()
{
	int ok = 1;

	return ok;
}

int PPVkClient::LogOut()
{
	int ok = 1;

	return ok;
}

int PPVkClient::SetAuthToken(const char *pToken)
{
	int ok = 1;

	return ok;
}

int PPVkClient::DelAuthToken()
{
	int ok = 1;

	return ok;
}

int PPVkClient::TakeToken()
{
	int ok = 1;

	return ok;
}

int PPVkClient::CreateURLRequest(const char * pMethod, StringSet & rParams, SString & rResult)
{
	int ok = 1;
	assert(pMethod);
	SString params, method, temp_buf;
	uint pos = 0;
	SString str;
	method = pMethod;
	while(rParams.get(&pos, str)) {
		str.Strip();
		params.Cat(str).Cat("&");
	}
	temp_buf.Z().Cat(method).Cat("?").Cat(params).Cat(Token).Cat("&v=5.52");
	return ok;
}

int PPVkClient::SendRequest(const char & pRequest) 
{
	int ok = 1;

	return ok;
}

void PPVkClient::GetVKAccessToken(long & rScope)
{
	SString url = "https://oauth.vk.com/authorize?client_id=7402217&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=8192&response_type=token&v=5.52/";
	ShellExecute(0, _T("open"), SUcSwitch(url), NULL, NULL, SW_SHOWNORMAL); // @unicodeproblem
}