 // GLBSRVS.CPP
 // Copyright (c) Erik Sobolev 2020, 2021, 2022, 2024, 2025
 //
#include <pp.h>
#pragma hdrstop

PPGlobalServiceLogTalkingHelper::PPGlobalServiceLogTalkingHelper(uint logFileNameId) 
{
	PPGetFilePath(PPPATH_LOG, logFileNameId, LogFileName);
}

void PPGlobalServiceLogTalkingHelper::Log(const char * pPrefix, const char * pTargetUrl, const SString & rMsg)
{
	if(LogFileName.NotEmpty()) {
		SFile  f_log(LogFileName, SFile::mAppend);
		if(f_log.IsValid()) {
			if(!isempty(pPrefix)) {
				SString temp_buf;
				temp_buf.CatCurDateTime(DATF_YMD|DATF_CENTURY, TIMF_HMS|TIMF_MSEC).Space().Cat(pPrefix).CatDiv(':', 2);
				if(!isempty(pTargetUrl))
					temp_buf.Space().CatEq("url", pTargetUrl);
				f_log.WriteLine(temp_buf.CR());
			}
			if(rMsg.NotEmpty())
				f_log.WriteLine(rMsg);
			f_log.WriteBlancLine();
		}
	}
}

// @v12.2.6 static SString P_VKUrlBase("https://api.vk.com");
static const char * P_VKMethodUrlBase = "https://api.vk.com/method";
//static const char * P_VKAppId = "7685698"; // "7402217"; // ������������� ���������� (�������� client_id � ��������)

VkInterface::ErrorResponse::ErrorResponse() : Code(0)
{
}

VkInterface::ErrorResponse & VkInterface::ErrorResponse::Z()
{
	Code = 0;
	Message.Z();
	return *this;
}

int VkInterface::ErrorResponse::FromJsonObj(const SJson * pJs) // @v12.2.6
{
	Z();
	int    ok = -1;
	if(SJson::IsObject(pJs)) {
		bool is_error = false;
		// {"error":"ERR_UPLOAD_BAD_IMAGE_SIZE: market photo min size 400x400","bwact":"do_add","server":532236,"_sig":"c808663daa0ad9ed3df608cb24226234"}
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("error")) {
				is_error = true;
				if(p_cur->P_Child) {
					if(p_cur->P_Child->Type == SJson::tSTRING) {
						(Message = p_cur->P_Child->Text).Unescape();
					}
					else if(p_cur->P_Child->Type == SJson::tOBJECT) { 
						for(const SJson * p_response = p_cur->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
							if(p_response->P_Child) {
								if(p_response->Text.IsEqiAscii("error_code")) {
									Code = p_response->P_Child->Text.ToLong();
								}
								else if(p_response->Text.IsEqiAscii("error_msg")) {
									Message = p_response->P_Child->Text.Unescape();
								}
							}
						}
					}
				}
			}
			else if(is_error) { // "error" was occured
				// Image upload error tags
				if(p_cur->Text.IsEqiAscii("bwact")) {
					;
				}
				else if(p_cur->Text.IsEqiAscii("server")) {
					;
				}
				else if(p_cur->Text.IsEqiAscii("_sig")) {
					;
				}
			}
		}
		if(is_error)
			ok = 1;
	}
	else
		ok = 0;
	return ok;
}

VkInterface::ImageReplyMeta::ImageReplyMeta()
{
}
		
VkInterface::ImageReplyMeta & VkInterface::ImageReplyMeta::Z()
{
	Size.Z();
	Kid.Z();
	return *this;
}

VkInterface::PostImageReply::PostImageReply() : UserId(0), GroupId(0), AlbumId(0), AppId(0)
{
}
		
VkInterface::PostImageReply & VkInterface::PostImageReply::Z()
{
	UserId = 0;
	GroupId = 0;
	AlbumId = 0;
	AppId = 0;
	Sha.Z();
	Secret.Z();
	Hash.Z();
	Server.Z();
	RequestId.Z();
	Photo.Z();
	CropData.Z();
	CropHash.Z();
	Meta.Z();
	Err.Z();
	OriginalReplyText.Z();
	return *this;
}

VkInterface::InitBlock::InitBlock() : GuaID(0), OuterWareIdentTagID(0), LinkFileType(0)
{
}

VkInterface::InitBlock & VkInterface::InitBlock::Z()
{
	GuaID = 0;
	OuterWareIdentTagID = 0;
	LinkFileType = 0;
	GuaPack.Z();
	CliIdent.Z();
	CliAccsKey.Z();
	EndPoint.Z(); // URL ��� ��������
	//Token.Z();
	GroupId.Z();
	PageId.Z();
	TxtMsg.Z();
	LinkFilePath.Z();
	return *this;
}

enum VkAccessRightGroup {
	vkacsrgStories   = 0x00000001, // (+1) (1 << 0) group ������ � ��������.
	vkacsrgPhotos    = 0x00000004, // (+4) (1 << 2) group ������ � �����������.
	vkacsrgAppWidget = 0x00000040, // (+64) (1 << 6) group ������ � �������� ���������� ���������. ��� ����� ����� ��������� ������ � ������� ������ Client API showGroupSettingsBox.
	vkacsrgMessages  = 0x00001000, // (+4096) (1 << 12) group ������ � ���������� ����������.
	vkacsrgDocs      = 0x00020000, // (+131072) (1 << 17) group ������ � ����������.
	vkacsrgManage    = 0x00040000, // (+262144) (1 << 18) group ������ � ����������������� ����������. 
};

enum VkAccessRightUser {
	vkacsruNotify        = 0x00000001, // (+1)         (1 << 0) ������������ �������� ���������� ��� ����������� (��� flash/iframe-����������).
	vkacsruFriends       = 0x00000002, // (+2)         (1 << 1) ������ � �������.
	vkacsruPhotos        = 0x00000004, // (+4)         (1 << 2) ������ � �����������.
	vkacsruAudio         = 0x00000008, // (+8)         (1 << 3) ������ � ������������.
	vkacsruVideo         = 0x00000010, // (+16)        (1 << 4) ������ � ������������.
	vkacsruStories       = 0x00000040, // (+64)        (1 << 6) ������ � ��������.
	vkacsruPages         = 0x00000080, // (+128)       (1 << 7) ������ � wiki-���������.
	vkacsruAddRef        = 0x00000100, // (+256)       (1 << 8) ���������� ������ �� ���������� � ���� �����.
	vkacsruStatus        = 0x00000400, // (+1024)      (1 << 10) ������ � ������� ������������.
	vkacsruNotes         = 0x00000800, // (+2048)      (1 << 11) ������ � �������� ������������.
	vkacsruMessages      = 0x00001000, // (+4096)      (1 << 12) ������ � ����������� ������� ������ � ����������� (������ ��� Standalone-����������, ��������� ���������).
	vkacsruWall          = 0x00002000, // (+8192)      (1 << 13) ������ � ������� � ����������� ������� ������ �� ������.
	// ������ ����� ������� �� ��������� ���������� ��� ������ (������������ ��� ������� ����������� ��� ���������� � ����� ����-���� ��� �� ����� Authorization Code Flow).
	vkacsruAds           = 0x00008000, // (+32768)     (1 << 15) ������ � ����������� ������� ������ � ��������� API. �������� ��� ����������� �� ����� Implicit Flow ��� Authorization Code Flow.
	vkacsruOffline       = 0x00010000, // (+65536)     (1 << 16) ������ � API � ����� ����� (��� ������������� ���� ����� �������� expires_in, ������������ ������ � access_token, �������� 0 � ����� ����������). �� ����������� � Open API.
	vkacsruDocs          = 0x00020000, // (+131072)    (1 << 17) ������ � ����������.
	vkacsruGroups        = 0x00040000, // (+262144)    (1 << 18) ������ � ������� ������������.
	vkacsruNotifications = 0x00080000, // (+524288)    (1 << 19) ������ � ����������� �� ������� ������������.
	vkacsruStats         = 0x00100000, // (+1048576)   (1 << 20) ������ � ���������� ����� � ���������� ������������, ��������������� ������� �� ��������.
	vkacsruEmail         = 0x00400000, // (+4194304)   (1 << 22) ������ � email ������������.
	vkacsruMarket        = 0x08000000, // (+134217728) (1 << 27) ������ � �������. 
};

void VkInterface::GetVKAccessToken()
{
	// P_VKAppId
	SString url_buf(InetUrl::MkHttps("oauth.vk.com", "authorize"));
	SString redirect_uri_buf(InetUrl::MkHttps("oauth.vk.com", "blank.html"));
	/*
				temp_buf.Z();
				temp_buf.CatEq("limit", count_limit);
				temp_buf.CatChar('&').CatEq("offset", count_offset);
				temp_buf.CatChar('&').CatEq("folder", doc_fold_id);
				temp_buf.CatChar('&').CatEq("created_from", created_from);
				temp_buf.CatChar('&').CatEq("created_to", created_to);

				134225924 = 0x8002004
	*/
	uint req_rights = (vkacsruPhotos|vkacsruWall|vkacsruMarket|vkacsruOffline); // @v12.2.4 vkacsruOffline
	url_buf.CatChar('?').CatEq("client_id", /*P_VKAppId*/AppIdent).
		CatChar('&').CatEq("display", "page").
		CatChar('&').CatEq("redirect_uri", redirect_uri_buf).
		CatChar('&').CatEq("scope", req_rights/*"134225924"*/). // ����� ������� 134225924 = 0x8002004
		CatChar('&').CatEq("response_type", "token").
		CatChar('&').CatEq("v", "5.126");
	//url_buf.SetLastDSlash();
	//SString url("https://oauth.vk.com/authorize?client_id=7402217&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=134225924&response_type=token&v=5.52/");
	::ShellExecute(0, _T("open"), SUcSwitch(url_buf), NULL, NULL, SW_SHOWNORMAL);
}

VkInterface::SimpleRef::SimpleRef() : Id(0)
{
}

VkInterface::WareCategory::WareCategory() : Id(0)
{
}

VkInterface::WarePrice::WarePrice() : Amount(0)
{
}

VkInterface::MarketWareItem::MarketWareItem() : 
	PPObjGoods::ExportToGlbSvcItem(), OuterId(0), OwnerId(0), Availability(0), CartQtty(0.0), Date(0), CurrencyId(0)
{
}

VkInterface::MarketWareItem::MarketWareItem(const PPObjGoods::ExportToGlbSvcItem & rS) : 
	PPObjGoods::ExportToGlbSvcItem(rS), OuterId(0), OwnerId(0), Availability(0), CartQtty(0.0), Date(0), CurrencyId(0)
{
}

VkInterface::VkInterface() : ProtoVer(5, 131, 0)/*@v12.2.6 (5.105-->5.131)*/, Lth(PPFILNAM_VKTALK_LOG), LastRequestClk(0)
{
	PPVersionInfo vi = DS.GetVersionInfo();
	vi.GetTextAttrib(vi.taiVkAppIdent, AppIdent);
}

SString & VkInterface::AppendParamProtoVer(SString & rBuf) const
{
	return rBuf.Cat("v").Eq().Cat(ProtoVer.GetMajor()).Dot().Cat(ProtoVer.GetMinor());
}

PPID VkInterface::GetOuterWareIdentTagID() const { return Ib.OuterWareIdentTagID; }
const VkInterface::ErrorResponse & VkInterface::GetLastResponseError() const { return LastErrResp; }

int VkInterface::Setup(PPID guaID, uint flags)
{
	const  PPID service_ident = PPGLS_VK;
	int    ok = 1;
	Ib.Z();
	PPObjGlobalUserAcc gua_obj;
	if(!guaID) {
		PPGlobalUserAcc gua_rec;
		if(gua_obj.SearchBySymb("vk_acc", 0, &gua_rec) > 0 && gua_rec.ServiceIdent == service_ident) {
			guaID = gua_rec.ID;
		}
		else {
			PPIDArray gua_list;
			if(gua_obj.GetListByServiceIdent(service_ident, &gua_list) > 0 && gua_list.getCount() == 1)
				guaID = gua_list.get(0);
		}
	}
	THROW(guaID);
	THROW(gua_obj.GetPacket(guaID, &Ib.GuaPack) > 0);
	Ib.GuaID = guaID;
	//THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_LOGIN, Ib.CliIdent) > 0);
	THROW(Ib.GuaPack.GetAccessKey(Ib.CliAccsKey) > 0);
	//Ib.EndPoint = InetUrl::MkHttps("api.uds.app", "partner/v2");
	{
		Reference * p_ref = PPRef;
		PPObjTag obj_tag;
		PPID   tag_outer_goods_id = 0;
		if(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_SOCIALGROUPCODE, Ib.GroupId) > 0) {
		}
		else {
			//CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NOGROUPIDINGUA, &msg_buf)));
		}
		if(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_SOCIALPAGECODE, Ib.PageId) > 0) {
		}
		else {
			//CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NOPAGEIDINGUA, &msg_buf)));
		}
		if(flags & sfInitStoreAttributes) {
			{
				const ObjTagItem * p_tag_item = Ib.GuaPack.TagL.GetItem(PPTAG_GUA_OUTERWAREIDTAG);
				long    outer_wareid_tag_id = 0;
				if(p_tag_item && p_tag_item->GetInt(&outer_wareid_tag_id)) {
					PPObjTag tag_obj;
					PPObjectTag tag_rec;
					if(tag_obj.Fetch(outer_wareid_tag_id, &tag_rec) > 0) {
						assert(tag_rec.ID == outer_wareid_tag_id); // @paranoic
						Ib.OuterWareIdentTagID = tag_rec.ID;
					}
				}
			}
			/*if(obj_tag.FetchBySymb("VK_GOODS_ID", &tag_outer_goods_id) > 0) {
				Ib.OuterWareIdTagID = tag_outer_goods_id;
			}
			else {
				;
			}*/
		}
		/*gua_obj.SearchBySymb("vk_acc", 0, &gua_rec); 
		if(gua_obj.GetPacket(gua_rec.ID, &gua_pack) <= 0) {
			CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NOGUA, &msg_buf)));
		}*/
		/*else if(gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, vk_struct.Token) <= 0) {
			CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NOTOKENINGUA, &msg_buf)));
		}*/
	}
	CATCHZOK
	return ok;
}

int VkInterface::WallPost(const SString & rMessage, const SString & rLinkFilePath, SString & rOutput)
{
	int    ok = 1;
	SString reply_buf;
	SString temp_buf;
	SString upload_url;
	//SString photo;
	//SString server;
	//SString hash;
	PostImageReply reply;
	SJson * p_json_doc = 0;
	THROW(Photos_GetWallUploadServer(upload_url));
	{
		THROW(PhotoToReq(upload_url, rLinkFilePath, reply, "photo"));
		//THROW_SL(p_json_doc = SJson::Parse(reply_buf.cptr()));
		//if(reply.Err.Code)
		if(LastErrResp.FromJsonObj(p_json_doc) > 0) {
			; // @err
		}
		else {
			/*
			for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("server")) {
							THROW(p_obj->P_Child)
							server = p_obj->P_Child->Text;
						}
						else if(p_obj->Text.IsEqiAscii("photo")) {
							THROW(p_obj->P_Child)
							photo = p_obj->P_Child->Text.Unescape();
						}
						else if(p_obj->Text.IsEqiAscii("hash")) {
							THROW(p_obj->P_Child);
							hash = p_obj->P_Child->Text;
						}
					}
				}
			}*/
		}
		ZDELETE(p_json_doc);
	}
	{
		SString id;
		SString owner_id;
		THROW(Photos_SaveWallPhoto(reply.Photo, reply.Server, reply.Hash, reply_buf));
		THROW_SL(p_json_doc = SJson::Parse(reply_buf));
		for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type==SJson::tOBJECT) {
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("response")) {
						const SJson *p_photo = p_obj->P_Child;
						if(p_photo->P_Child) {
							for(const SJson * p_response = p_photo->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
								if(p_response->Text.IsEqiAscii("owner_id")) {
									owner_id = p_response->P_Child->Text.Unescape();
								}
								else if(p_response->Text.IsEqiAscii("id")) {
									id = p_response->P_Child->Text.Unescape();
								}
							}
						}
					}
				}
			}
		}
		ZDELETE(p_json_doc);
		if(id.Len() && owner_id.Len()) {
			temp_buf.Z().Cat("photo").Cat(owner_id).CatChar('_').Cat(id);
			THROW(Wall_Post(rMessage, temp_buf, reply_buf));
		}
	}
	CATCHZOK;
	ZDELETE(p_json_doc);
	return ok;
}

int VkInterface::PutWareToMarket(const MarketWareItem & rItem, MarketWareItem & rResultItem)
{
	int    ok = 1;
	SString reply_buf;
	SString upload_url;
	//SString photo_id;
	SJson * p_json_doc = 0;
	PostImageReply post_img_reply;
	// @v12.2.7 {
	int64   photo_id = 0;
	if(PublishImage(VkInterface::ccatMarketProductPhoto, rItem.ImgPath, &photo_id) > 0) {
		;
	}
	else {
		photo_id = 0;
	}
	// } @v12.2.7 
#if 0 // @v12.2.7 {
	{
		THROW(Photos_GetMarketUploadServer(/*rVkStruct,*/1, upload_url));
		THROW(PhotoToReq(upload_url, rItem.ImgPath, post_img_reply, "file"));
		//THROW_SL(p_json_doc = SJson::Parse(temp_buf));
		//THROW(ReadError(p_json_doc, LastErrResp) < 0);
		if(LastErrResp.FromJsonObj(p_json_doc) > 0) {
			; // @err
		}
		else {
			/*for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->P_Child) {
							if(p_obj->Text.IsEqiAscii("server"))
								server = p_obj->P_Child->Text;
							else if(p_obj->Text.IsEqiAscii("photo"))
								photo = p_obj->P_Child->Text.Unescape();
							else if(p_obj->Text.IsEqiAscii("hash"))
								hash = p_obj->P_Child->Text;
							else if(p_obj->Text.IsEqiAscii("crop_data"))
								crop_data = p_obj->P_Child->Text.Unescape();
							else if(p_obj->Text.IsEqiAscii("crop_hash"))
								crop_hash = p_obj->P_Child->Text;
						}
					}
				}
			}*/
		}
		ZDELETE(p_json_doc);
	}
	{
		THROW(Photos_SaveMarketPhoto(post_img_reply.Photo, post_img_reply.Server, post_img_reply.Hash, post_img_reply.CropData, post_img_reply.CropHash, reply_buf));
		THROW_SL(p_json_doc = SJson::Parse(reply_buf));
		for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("response")) {
						const SJson * p_photo = p_obj->P_Child;
						if(p_photo->P_Child) {
							for(const SJson * p_response = p_photo->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
								if(p_response->Text.IsEqiAscii("id")) {
									photo_id = p_response->P_Child->Text.Unescape();
								}
							}
						}
					}
				}
			}
		}
		ZDELETE(p_json_doc);
	}
#endif // } 0 @v12.2.7
	{
		SString name;
		SString descr;
		bool   test_item_not_found_error = false;
		(name = rItem.Name).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
		(descr = rItem.Description).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
		SString url_buf(P_VKMethodUrlBase);
		if(rItem.OuterId > 0) {
			url_buf.SetLastDSlash().Cat("market.edit").CatChar('?').CatEq("item_id", rItem.OuterId).CatChar('&');
		}
		else {
			url_buf.SetLastDSlash().Cat("market.add").CatChar('?');
		}
		url_buf.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).CatChar('&')
			.CatEq("access_token", Ib.CliAccsKey).CatChar('&');
			AppendParamProtoVer(url_buf).CatChar('&')
			.CatEq("name", name).CatChar('&')
			.CatEq("category_id", /*catId*/1100LL).CatChar('&')
			.CatEq("price", rItem.Price, MKSFMTD_020).CatChar('&')
			.CatEq("main_photo_id", photo_id).CatChar('&')
			.CatEq("description", descr);
		THROW(GetRequest(url_buf, reply_buf, ScURL::mfDontVerifySslPeer));
		THROW_SL(p_json_doc = SJson::Parse(reply_buf));
		//THROW(ReadError(p_json_doc, LastErrResp) < 0);
		if(LastErrResp.FromJsonObj(p_json_doc) > 0) {
			; // @err
		}
		else {
			for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Type == SJson::tOBJECT) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("response")) {
							const SJson * p_response_node = p_obj->P_Child;
							if(p_response_node->P_Child) {
								for(const SJson * p_response_item = p_response_node->P_Child; p_response_item; p_response_item = p_response_item->P_Next) {
									if(p_response_item->Text.IsEqiAscii("market_item_id")) {
										rResultItem.OuterId = p_response_item->P_Child->Text.ToInt64();
									}
								}
							}
						}
					}
				}
			}
		}
		ZDELETE(p_json_doc);
	}
	CATCHZOK
	delete p_json_doc;
	return ok;
}

int VkInterface::ParseUploadServer(const SString & rJson, SString & rUploadOut)
{
	int    ok = 0;
	rUploadOut.Z();
	SJson * p_json_doc = SJson::Parse(rJson.cptr());
	THROW_SL(p_json_doc);
	for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response") && p_obj->P_Child) {
					for(const SJson * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
						if(p_response->Text.IsEqiAscii("upload_url")) {
							rUploadOut = p_response->P_Child->Text.Unescape();
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_json_doc;
	return ok;
}

int VkInterface::Photos_GetWallUploadServer(SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	SString temp_buf;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getWallUploadServer").CatChar('?')
		.CatEq("group_id", Ib.GroupId).CatChar('&')
		.CatEq("access_token", Ib.CliAccsKey).CatChar('&');
	AppendParamProtoVer(url); //.CatEq("v", "5.107");
	THROW(GetRequest(url, temp_buf, ScURL::mfDontVerifySslPeer));
	THROW(ParseUploadServer(temp_buf, rOutput));
	CATCHZOK
	return ok;
}

int VkInterface::Photos_GetMarketUploadServer(const uint mainPhotoFlag, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	SString temp_buf;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getMarketUploadServer").CatChar('?')
		.CatEq("group_id", Ib.GroupId).CatChar('&')
		.CatEq("main_photo", mainPhotoFlag).CatChar('&')
		.CatEq("access_token", Ib.CliAccsKey).CatChar('&');
	AppendParamProtoVer(url);//.CatEq("v", "5.107");
	THROW(GetRequest(url, temp_buf, ScURL::mfDontVerifySslPeer));
	THROW(ParseUploadServer(temp_buf, rOutput));
	CATCHZOK;
	return ok;
}

int  VkInterface::Photos_SaveMarketPhoto(/*const VkStruct &rVkStruct,*/const SString & rImage, const SString & rServer, 
	const SString & rHash, const SString & rCropData, const SString & rCropHash, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.saveMarketPhoto").CatChar('?')
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).CatChar('&')
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).CatChar('&');
	AppendParamProtoVer(url).CatChar('&')
		.CatEq("photo", rImage).CatChar('&')
		.CatEq("server", rServer).CatChar('&')
		.CatEq("hash", rHash).CatChar('&')
		.CatEq("crop_data", rCropData).CatChar('&')
		.CatEq("crop_hash", rCropHash);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int VkInterface::Photos_SaveWallPhoto(/*const VkStruct & rVkStruct,*/const SString & rPhoto, const SString & rServer, const SString & rHash, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.saveWallPhoto").CatChar('?')
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).CatChar('&')
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).CatChar('&');
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.CatChar('&')
		.CatEq("photo", rPhoto).CatChar('&')
		.CatEq("server", rServer).CatChar('&')
		.CatEq("hash", rHash).CatChar('&');
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int VkInterface::Wall_Post(const SString & rMessage, const SString & rVkPhotoName, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	SString temp_buf(rMessage);
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("wall.post?")
		.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).CatChar('&')
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).CatChar('&');
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.CatChar('&')
		.CatEq("attachments", rVkPhotoName).CatChar('&')
		.CatEq("message", temp_buf);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

bool VkInterface::GetUploadServerUrl(int vkContentCategory, SString & rBuf) // @v12.2.6
{
	rBuf.Z();
	bool   ok = false;
	static const SIntToSymbTabEntry query_url_list[] = {
		{ ccatMarketProductPhoto, "market.getProductPhotoUploadServer" },
		{ ccatPhotos, "photos.getUploadServer" },
		{ ccatPhotosWall, "photos.getWallUploadServer" },
		{ ccatPhotosOwner, "photos.getOwnerPhotoUploadServer" },
		{ ccatPhotosMessages, "photos.getMessagesUploadServer" },
		{ ccatPhotosChat, "photos.getChatUploadServer" },
		{ ccatPhotosMarketAlbum, "photos.getMarketAlbumUploadServer" }
	};
	const char * p_query_url = SIntToSymbTab_GetSymbPtr(query_url_list, SIZEOFARRAY(query_url_list), vkContentCategory);
	if(!p_query_url) {
		; // @todo @err
	}
	else {
		SString reply_buf;
		SString url_buf(P_VKMethodUrlBase);
		url_buf.SetLastDSlash().Cat(p_query_url);
		url_buf.CatChar('?').CatEq("access_token", Ib.CliAccsKey).CatChar('&').CatEq("group_id", Ib.GroupId);
		AppendParamProtoVer(url_buf.CatChar('&'));
		THROW(GetRequest(url_buf, reply_buf, ScURL::mfDontVerifySslPeer));
		if(reply_buf.NotEmpty()) {
			SJson * p_js_reply = SJson::Parse(reply_buf);
			if(SJson::IsObject(p_js_reply)) {
				for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
					if(p_cur->Text.IsEqiAscii("response")) {
						if(SJson::IsObject(p_cur->P_Child)) {
							for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
								if(p_cur2->Text.IsEqiAscii("upload_url")) {
									(rBuf = p_cur2->P_Child->Text).Unescape();
									ok = true;
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int VkInterface::SaveProductPhoto(const PostImageReply & rPIR, int64 * pPhotoId)
{
	int    ok = -1;
	int64  photo_id = 0;
	SString temp_buf;
	SString reply_buf;
	SString url_buf(P_VKMethodUrlBase);
	url_buf.SetLastDSlash().Cat("market.saveProductPhoto");
	(temp_buf = rPIR.OriginalReplyText)/*.Escape()*/;
	url_buf.CatChar('?').CatEq("upload_response", temp_buf).CatChar('&').
	CatEq("access_token", Ib.CliAccsKey);
	AppendParamProtoVer(url_buf.CatChar('&'));
	THROW(GetRequest(url_buf, reply_buf, ScURL::mfDontVerifySslPeer));
	if(reply_buf.NotEmpty()) {
		SJson * p_js_reply = SJson::Parse(reply_buf);
		if(SJson::IsObject(p_js_reply)) {
			/*
				{
					"response": {
						"photo_id": 457239017,
						"photo": {
							"album_id": -53,
							"date": 1739632103,
							"id": 457239017,
							"owner_id": -224529551,
							"sizes": [
								{
									"height": 75,
									"type": "s",
									"width": 59,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=59x75"
								},
								{
									"height": 130,
									"type": "m",
									"width": 102,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=102x130"
								},
								{
									"height": 591,
									"type": "x",
									"width": 464,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=464x591"
								},
								{
									"height": 166,
									"type": "o",
									"width": 130,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=130x166"
								},
								{
									"height": 255,
									"type": "p",
									"width": 200,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=200x255"
								},
								{
									"height": 408,
									"type": "q",
									"width": 320,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=320x408"
								},
								{
									"height": 591,
									"type": "r",
									"width": 464,
									"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu&cs=464x591"
								}
							],
							"text": "",
							"user_id": 100,
							"web_view_token": "6e9162d423aba03774",
							"has_tags": false,
							"orig_photo": {
								"height": 591,
								"type": "base",
								"url": "https://sun9-77.userapi.com/s/v1/ig2/quhs9NrY2MTk5YP1rl59G8xc0_feYM5cN95olst6bBTfSn7DOtZ9Zxtdkuii0oT9ahLrOp5rjN6ntGwZa9NnMjAy.jpg?quality=95&as=32x41,48x61,72x92,108x138,160x204,240x306,360x459,464x591&from=bu",
								"width": 464
							}
						}
					}
				}
			*/
			for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
				if(p_cur->Text.IsEqiAscii("response")) {
					if(SJson::IsObject(p_cur->P_Child)) {
						for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
							if(p_cur2->Text.IsEqiAscii("photo_id")) {
								photo_id = p_cur2->P_Child->Text.ToInt64();
								ok = 1;
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ASSIGN_PTR(pPhotoId, photo_id);
	return ok;
}

int VkInterface::PublishImage(int vkContentCategory, const char * pImgPath, int64 * pResultPhotoId) // @v12.2.6 @construction
{
	int    ok = -1;
	int64  photo_id = 0;
	SString upload_url;
	SString img_path;
	PostImageReply post_img_reply;
	SFsPath::NormalizePath(pImgPath, SFsPath::npfCompensateDotDot|SFsPath::npfSlash, img_path);
	if(fileExists(img_path)) {
		if(GetUploadServerUrl(vkContentCategory, upload_url)) {
			assert(upload_url.NotEmpty());
			if(upload_url.NotEmpty()) {
				int r = PhotoToReq(upload_url, img_path, post_img_reply, "file");
				if(r > 0) {
					if(SaveProductPhoto(post_img_reply, &photo_id) > 0) {
						assert(photo_id != 0);
						ok = 1;
					}
				}
			}
		}
	}
	//CATCHZOK
	ASSIGN_PTR(pResultPhotoId, photo_id);
	return ok;
}

int VkInterface::ReadError_(const SJson * pJs, ErrorResponse & rErr) const
{
	// {"error":"ERR_UPLOAD_BAD_IMAGE_SIZE: market photo min size 400x400","bwact":"do_add","server":532236,"_sig":"c808663daa0ad9ed3df608cb24226234"}
	int    ok = -1;
	const SJson * p_cur = pJs;
	if(SJson::IsObject(p_cur)) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("error")) {
				ok = 1;
				if(p_cur->P_Child) {
					if(p_cur->P_Child->Type == SJson::tSTRING) {
						rErr.Message = p_cur->P_Child->Text.Unescape();
					}
					else if(p_cur->P_Child->Type == SJson::tOBJECT) { 
						for(const SJson * p_response = p_cur->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
							if(p_response->Text.IsEqiAscii("error_code")) {
								if(p_response->P_Child)
									rErr.Code = p_response->P_Child->Text.Unescape().ToLong();
							}
							else if(p_response->Text.IsEqiAscii("error_msg")) {
								if(p_response->P_Child)
									rErr.Message = p_response->P_Child->Text.Unescape();
							}
						}
					}
				}
			}
			else if(ok == 1) { // "error" was occured
				// Image upload error tags
				if(p_cur->Text.IsEqiAscii("bwact")) {
					;
				}
				else if(p_cur->Text.IsEqiAscii("server")) {
					;
				}
				else if(p_cur->Text.IsEqiAscii("_sig")) {
					;
				}
			}
		}
	}
	return ok;
}

int VkInterface::ParseSimpleRef(const SJson * pJs, SimpleRef & rItem) const
{
	int    ok = 1;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("id")) {
				rItem.Id = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("name")) {
				rItem.Name = p_jsn->P_Child->Text.Unescape();
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int VkInterface::ParseWareCategory(const SJson * pJs, WareCategory & rItem) const
{
	int    ok = 1;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("id")) {
				rItem.Id = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("name")) {
				rItem.Name = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("section")) {
				ParseSimpleRef(p_jsn->P_Child, rItem.Section);
			}
		}
	}
	else
		ok = 0;
	return ok;
}

int VkInterface::ParseWarePrice(const SJson * pJs, WarePrice & rItem) const
{
	int    ok = 1;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("amount")) {
				rItem.Amount = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("text")) {
				rItem.Text = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("currency")) {
				ParseSimpleRef(p_jsn->P_Child, rItem.Currency);
			}
		}
	}
	else
		ok = 0;
	return ok;
	return ok;
}

int VkInterface::ParseWareItem(const SJson * pJs, MarketWareItem & rItem) const
{
	int    ok = 1;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("id")) {
				rItem.OuterId = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("availability")) {
				rItem.Availability = p_jsn->P_Child->Text.ToLong();
			}
			else if(p_jsn->Text.IsEqiAscii("category")) {
				ParseWareCategory(p_jsn->P_Child, rItem.Category);
			}
			else if(p_jsn->Text.IsEqiAscii("description")) {
				rItem.Description = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("owner_id")) {
				rItem.OwnerId = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("price")) {
				WarePrice wp;
				if(ParseWarePrice(p_jsn->P_Child, wp)) {
					rItem.Price = static_cast<double>(wp.Amount) / 100.0;
					rItem.CurrencyId = static_cast<int>(wp.Currency.Id);
				}
			}
			else if(p_jsn->Text.IsEqiAscii("thumb_photo")) {
				rItem.ThumbPhoto = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("title")) {
				rItem.Name = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("date")) {
				rItem.Date = p_jsn->P_Child->Text.ToInt64();
			}
			else if(p_jsn->Text.IsEqiAscii("cart_quantity")) {
				rItem.CartQtty = p_jsn->P_Child->Text.ToReal();
			}
		}
	}
	return ok;
}

int VkInterface::Market_Get(long offs, long maxItems, TSCollection <MarketWareItem> & rList) // get all goods from VK market
{
	rList.freeAll();
	LastErrResp.Z();

	int    ok = 1;
	SJson * p_json_doc = 0;
	SString json_buf;
	SString url, temp_buf;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.get").CatChar('?')
		.CatEq("owner_id", temp_buf.Z().CatChar('-').Cat(Ib.GroupId)).CatChar('&')
		.CatEq("access_token", Ib.CliAccsKey).CatChar('&');
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/;
	if(offs > 0) {
		url.CatChar('&').Cat(offs);
	}
	if(maxItems > 0) {
		url.CatChar('&').Cat(maxItems);
	}
	THROW(GetRequest(url, temp_buf.Z(), ScURL::mfDontVerifySslPeer));
	{
		long item_id = 0; 
		THROW_SL(p_json_doc = SJson::Parse(temp_buf.cptr()));
		//if(ReadError(p_json_doc, LastErrResp) > 0) {
		if(LastErrResp.FromJsonObj(p_json_doc) > 0) {
			ok = 0;
		}
		else { 
			for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
				if(SJson::IsObject(p_cur)) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("response")) {
							for(const SJson * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
								if(p_response->Text.IsEqiAscii("items")) {
									//owner_id = p_response->P_Child->Text.Unescape();
									for(const SJson * p_item = p_response->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										MarketWareItem * p_new_entry = rList.CreateNewItem();
										ParseWareItem(p_item, *p_new_entry);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	ZDELETE(p_json_doc);
	return ok;
} 

int VkInterface::PhotoToReq(const SString & rUrlBuf, const SString & rImgPath, PostImageReply & rReply, const char * pDataName)
{
	int    ok = 1;
	SString temp_buf;
	SString reply_buf;
	InetUrl url(rUrlBuf);
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	ScURL::HttpForm hf(c);
	SBuffer * p_ack_buf = 0;
	//hf.AddContentFile(rImgPath, "multipart/form-data", pDataName);
	{
		SFileFormat ff;
		const int ffr = ff.Identify(rImgPath);
		if(oneof2(ffr, 2, 3) && oneof3(ff, SFileFormat::Jpeg, SFileFormat::Png, SFileFormat::Gif)) {
			//SFileFormat::GetMime(ff, temp_buf);
			//ps.Split(local_path_src);
			//ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_fname);
			if(!hf.AddContentFile(rImgPath, "multipart/form-data", pDataName))
				ok = 0;
		}
		else {
			ok = 0; // not supported image file
		}
	}
	if(ok) {
		Lth.Log("req", rUrlBuf, (temp_buf = "multipart/form-data").CatDiv(':', 0).Cat(rImgPath));
		c.HttpPost(url, ScURL::mfVerbose|ScURL::mfDontVerifySslPeer, 0, hf, &wr_stream);
		p_ack_buf = static_cast<SBuffer *>(wr_stream);
		if(p_ack_buf) {
			reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
			Lth.Log("rep", 0, reply_buf);
			// @construction
			rReply.OriginalReplyText = reply_buf;
			SJson * p_js_reply = SJson::Parse(reply_buf);
			if(rReply.Err.FromJsonObj(p_js_reply) > 0) {
				; // @err
			}
			else {
				if(SJson::IsObject(p_js_reply)) {
					for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {
						if(p_cur->Text.IsEqiAscii("sha")) {
							(rReply.Sha = p_cur->P_Child->Text).Unescape();
						}
						else if(p_cur->Text.IsEqiAscii("secret")) {
							(rReply.Secret = p_cur->P_Child->Text).Unescape();
						}
						else if(p_cur->Text.IsEqiAscii("meta")) {
							if(SJson::IsObject(p_cur->P_Child)) {
								for(const SJson * p_cur2 = p_cur->P_Child->P_Child; p_cur2; p_cur2 = p_cur2->P_Next) {
									if(p_cur2->Text.IsEqiAscii("height")) {
										rReply.Meta.Size.y = p_cur2->P_Child->Text.ToLong();
									}
									else if(p_cur2->Text.IsEqiAscii("width")) {
										rReply.Meta.Size.x = p_cur2->P_Child->Text.ToLong();
									}
									else if(p_cur2->Text.IsEqiAscii("kid")) {
										(rReply.Meta.Kid = p_cur2->P_Child->Text).Unescape();
									}
								}
							}
						}
						else if(p_cur->Text.IsEqiAscii("hash")) {
							(rReply.Hash = p_cur->P_Child->Text).Unescape();
						}
						else if(p_cur->Text.IsEqiAscii("server")) {
							(rReply.Server = p_cur->P_Child->Text).Unescape();
						}
						else if(p_cur->Text.IsEqiAscii("user_id")) {
							rReply.UserId = p_cur->P_Child->Text.ToInt64();
						}
						else if(p_cur->Text.IsEqiAscii("group_id")) {
							rReply.GroupId = p_cur->P_Child->Text.ToInt64();
						}
						else if(p_cur->Text.IsEqiAscii("request_id")) {
							(rReply.RequestId = p_cur->P_Child->Text).Unescape();
						}
						else if(p_cur->Text.IsEqiAscii("album_id")) {
							rReply.AlbumId = p_cur->P_Child->Text.ToInt64();
						}
						else if(p_cur->Text.IsEqiAscii("app_id")) {
							rReply.AppId = p_cur->P_Child->Text.ToInt64();
						}
						else if(p_cur->Text.IsEqiAscii("photo"))
							(rReply.Photo = p_cur->P_Child->Text).Unescape();
						else if(p_cur->Text.IsEqiAscii("crop_data"))
							(rReply.CropData = p_cur->P_Child->Text).Unescape();
						else if(p_cur->Text.IsEqiAscii("crop_hash"))
							(rReply.CropHash = p_cur->P_Child->Text).Unescape();
					}
				}
				else
					ok = 0;
			}
		}
	}
	return ok;
}

int VkInterface::GetRequest(const SString & rUrl, SString & rOutput, int mflags)
{
	rOutput.Z();
	const  uint max_request_per_sec = 3;
	int    ok = 1;
	SString temp_buf;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	SBuffer * p_ack_buf = 0;
	Lth.Log("req", rUrl, temp_buf.Z());
	if(LastRequestClk) {
		const uint64 cc = static_cast<uint64>(clock());
		if((cc - LastRequestClk) < (1000 / max_request_per_sec)) {
			SDelay(1000 / max_request_per_sec);
		}
	}
	int qr = c.HttpGet(rUrl, mflags, &wr_stream);
	LastRequestClk = clock();
	THROW(qr);
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	THROW(p_ack_buf);
	rOutput.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
	Lth.Log("rep", 0, rOutput);
	CATCHZOK
	return ok;
}

/*static*/int VkInterface::Test() // @v12.2.6
{
	int    ok = -1;
	int64  photo_id = 0;
	const  PPID service_ident = PPGLS_VK;
	VkInterface ifc; // Test
	PPObjGlobalUserAcc gua_obj;
	PPIDArray gua_id_list;
	SString img_path;
	PPGetPath(PPPATH_TESTROOT, img_path);
	img_path.SetLastSlash().Cat("data").SetLastSlash().Cat("test10.jpg");
	gua_obj.GetListByServiceIdent(service_ident, &gua_id_list);
	PPID   gua_id = gua_id_list.getSingle();
	if(gua_id) {
		if(ifc.Setup(gua_id, ifc.sfInitStoreAttributes)) {
			ifc.PublishImage(VkInterface::ccatMarketProductPhoto, img_path, &photo_id);
		}
	}
	return ok;
}

#if 0 // {
int VkInterface::ParceGoodsItemList(const SString & rJsonStr, LongArray & rList) const
{
	int    ok = 1;
	SJson * p_json_doc = 0;
	long item_id = 0; 
	THROW_SL(p_json_doc = SJson::Parse(rJsonStr));
	for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					for(const SJson * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
						if(p_response->Text.IsEqiAscii("items")) {
							//owner_id = p_response->P_Child->Text.Unescape();
							for(const SJson *p_item = p_response->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
								for(const SJson *p_item_child = p_item->P_Child; p_item_child; p_item_child = p_item_child->P_Next) {
									if(p_item_child->Text.IsEqiAscii("id")) {
										item_id = p_item_child->P_Child->Text.Unescape().ToLong();
										rList.insert(&item_id);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	ZDELETE(p_json_doc);
	CATCHZOK;
	return ok;
}
#endif // } 0
//
//
//
struct SetupGlobalServiceUDS_Param {
	SetupGlobalServiceUDS_Param() : SCardSerID(0), GuaID(0)
	{
	}
	SString Login;
	SString ApiKey;
	PPID   SCardSerID;
	PPID   GuaID;
};

static int Setup_GlobalService_UDS_InitParam(SetupGlobalServiceUDS_Param & rP, int force)
{
	int    ok = 1;
	const  PPID service_ident = PPGLS_UDS;
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	PPObjSCardSeries scs_obj;
	PPGlobalUserAcc gua_rec;
	PPSCardSeries2 scs_rec;
	PPIDArray gua_candidate_list;
	PPIDArray scs_candidate_list;
	PPSCardConfig sc_cfg;
	PPObjSCard::ReadConfig(&sc_cfg);
	if(!rP.GuaID && gua_obj.GetListByServiceIdent(service_ident, &gua_candidate_list) > 0 && gua_candidate_list.getCount() == 1)
		rP.GuaID = gua_candidate_list.get(0);
	if(rP.GuaID) {
		PPGlobalUserAccPacket gua_pack;
		if(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0) {
			if(force) {
			}
			else {
				SString db_value;
				gua_pack.TagL.GetItemStr(PPTAG_GUA_LOGIN, db_value);
				if(db_value.NotEmptyS() && !rP.Login.NotEmptyS())
					rP.Login = db_value;
				gua_pack.GetAccessKey(db_value);
				if(db_value.NotEmptyS() && !rP.ApiKey.NotEmptyS()) {
					rP.ApiKey = db_value;
				}
			}
		}
	}
	if(!rP.SCardSerID) {
		for(SEnum en = scs_obj.Enum(0); en.Next(&scs_rec) > 0;) {
			if(scs_rec.SpecialTreatment == SCRDSSPCTRT_UDS) {
				scs_candidate_list.add(scs_rec.ID);
			}
		}
		if(scs_candidate_list.getCount() == 1)
			rP.SCardSerID = scs_candidate_list.get(0);
	}
	if(force) {
		PPGlobalUserAccPacket gua_pack;
		PPSCardSerPacket scs_pack;
		THROW_PP(rP.Login.NotEmptyS(), PPERR_LOGINNEEDED);
		THROW_PP(rP.ApiKey.NotEmptyS(), PPERR_APIKEYNEEDED);
		{
			PPTransaction tra(1);
			THROW(tra);
			if(rP.GuaID) {
				PPID   temp_id = gua_pack.Rec.ID;
				THROW(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0);
				THROW_PP_S(gua_pack.Rec.ServiceIdent == service_ident, PPERR_INVGUASERVICEIDENT, temp_buf.Z());
				gua_pack.TagL.PutItemStr(PPTAG_GUA_LOGIN, rP.Login);
				gua_pack.SetAccessKey(rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
			}
			else {
				PPID   temp_id = 0;
				gua_pack.Rec.ServiceIdent = service_ident;
				STRNSCPY(gua_pack.Rec.Name, "UDS");
				STRNSCPY(gua_pack.Rec.Symb, "UDS");
				gua_pack.TagL.PutItemStr(PPTAG_GUA_LOGIN, rP.Login);
				gua_pack.SetAccessKey(rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
			}
			if(rP.SCardSerID) {
				THROW(scs_obj.GetPacket(rP.SCardSerID, &scs_pack) > 0);
				THROW_PP_S(scs_pack.Rec.SpecialTreatment == SCRDSSPCTRT_UDS, PPERR_INVSCSSPECIALTREATMENT, temp_buf.Z());
				THROW_PP_S(scs_pack.Rec.PersonKindID != 0, PPERR_UNDEFSCSPERSONKIND, scs_pack.Rec.Name);
				THROW_PP_S(scs_pack.Rec.Flags & SCRDSF_BONUS, PPERR_SCARDSERMUSTBEBONUS, scs_pack.Rec.Name);
			}
			else {
				PPID   temp_id = 0;
				scs_pack.Rec.SpecialTreatment = SCRDSSPCTRT_UDS;
				scs_pack.Rec.Flags |= SCRDSF_BONUS;
				scs_pack.Rec.PersonKindID = sc_cfg.PersonKindID;
				STRNSCPY(scs_pack.Rec.Name, "UDS");
				STRNSCPY(scs_pack.Rec.Symb, "UDS");
				STRNSCPY(scs_pack.Eb.CodeTempl, "UDS%07[1..100000]");
				THROW(scs_obj.PutPacket(&temp_id, &scs_pack, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

int PPGlobalServiceHighLevelImplementations::Setup_UDS()
{
	class SetupGlobalServiceUDS_Dialog : public TDialog {
		DECL_DIALOG_DATA(SetupGlobalServiceUDS_Param);
	public:
		SetupGlobalServiceUDS_Dialog(DlgDataType & rData) : TDialog(DLG_SU_UDS), Data(rData), State(0)
		{
			Setup_GlobalService_UDS_InitParam(Data, 0);
			setCtrlString(CTL_SUUDS_LOGIN, Data.Login);
			setCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
			SetupPPObjCombo(this, CTLSEL_SUUDS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0);
			{
				SCardSeriesFilt scs_filt;
				scs_filt.Flags = scs_filt.fOnlySeries;
				scs_filt.SpecialTreatment = SCRDSSPCTRT_UDS;
				SetupPPObjCombo(this, CTLSEL_SUUDS_SCARDSER, PPOBJ_SCARDSERIES, Data.SCardSerID, 0);
			}
			{
				SString temp_buf;
				selectCtrl(CTL_SUUDS_LOGIN);
				PPLoadStringDescription("setupglbsvc_uds_login", temp_buf);
				setStaticText(CTL_SUUDS_HINT, temp_buf);
			}
		}
		bool   IsSettled() const { return LOGIC(State & stSettled); }
	private:
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCmd(cmConfigure)) {
				getCtrlString(CTL_SUUDS_LOGIN, Data.Login);
				getCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
				getCtrlData(CTLSEL_SUUDS_GUA, &Data.GuaID);
				getCtrlData(CTLSEL_SUUDS_SCARDSER, &Data.SCardSerID);
				if(!Setup_GlobalService_UDS_InitParam(Data, 1)) {
					PPError();
				}
				else {
					setCtrlString(CTL_SUUDS_LOGIN, Data.Login);
					setCtrlString(CTL_SUUDS_APIKEY, Data.ApiKey);
					//
					// �����-����� ���� ����������� - � ��� ��������� ����� �������, ������� �� ���� � �������
					//
					SetupPPObjCombo(this, CTLSEL_SUUDS_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, 0);
					{
						SCardSeriesFilt scs_filt;
						scs_filt.Flags = scs_filt.fOnlySeries;
						scs_filt.SpecialTreatment = SCRDSSPCTRT_UDS;
						SetupPPObjCombo(this, CTLSEL_SUUDS_SCARDSER, PPOBJ_SCARDSERIES, Data.SCardSerID, 0);
					}
					State |= stSettled;
					//enableCommand(cmConfigure, 0);
				}
			}
			else if(TVBROADCAST && TVCMD == cmReceivedFocus) {
				SString temp_buf;
				if(event.isCtlEvent(CTL_SUUDS_LOGIN)) {
					PPLoadStringDescription("setupglbsvc_uds_login", temp_buf);
				}
				else if(event.isCtlEvent(CTL_SUUDS_APIKEY)) {
					PPLoadStringDescription("setupglbsvc_uds_apikey", temp_buf);
				}
				setStaticText(CTL_SUUDS_HINT, temp_buf);
			}
			else
				return;
			clearEvent(event);
		}
		enum {
			stSettled = 0x0001
		};
		long   State;
	};
	int    ok = -1;
	SetupGlobalServiceUDS_Param param;
	SetupGlobalServiceUDS_Dialog * dlg = new SetupGlobalServiceUDS_Dialog(param);
	if(CheckDialogPtrErr(&dlg)) {
		ExecView(dlg);
		if(dlg->IsSettled())
			ok = 1;
	}
	delete dlg;
	return ok;
}

struct SetupGlobalServiceWildberries_Param {
	SetupGlobalServiceWildberries_Param() : GuaID(0)
	{
	}
	SString ApiKey;
	PPID   GuaID;
};

static int Setup_GlobalService_Wildberries_InitParam(SetupGlobalServiceWildberries_Param & rP, int force)
{
	int    ok = 1;
	const  PPID service_ident = PPGLS_WILDBERRIES;
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	PPObjSCardSeries scs_obj;
	PPGlobalUserAcc gua_rec;
	PPSCardSeries2 scs_rec;
	PPIDArray gua_candidate_list;
	PPIDArray scs_candidate_list;
	if(!rP.GuaID && gua_obj.GetListByServiceIdent(service_ident, &gua_candidate_list) > 0 && gua_candidate_list.getCount() == 1)
		rP.GuaID = gua_candidate_list.get(0);
	if(rP.GuaID) {
		PPGlobalUserAccPacket gua_pack;
		if(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0) {
			if(force) {
			}
			else {
				SString db_value;
				gua_pack.GetAccessKey(db_value);
				if(db_value.NotEmptyS() && !rP.ApiKey.NotEmptyS()) {
					rP.ApiKey = db_value;
				}
			}
		}
	}
	if(force) {
		PPGlobalUserAccPacket gua_pack;
		THROW_PP(rP.ApiKey.NotEmptyS(), PPERR_APIKEYNEEDED);
		{
			PPTransaction tra(1);
			THROW(tra);
			if(rP.GuaID) {
				PPID   temp_id = gua_pack.Rec.ID;
				THROW(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0);
				THROW_PP_S(gua_pack.Rec.ServiceIdent == service_ident, PPERR_INVGUASERVICEIDENT, temp_buf.Z());
				gua_pack.SetAccessKey(rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
			}
			else {
				PPID   temp_id = 0;
				gua_pack.Rec.ServiceIdent = service_ident;
				STRNSCPY(gua_pack.Rec.Name, "Wildberries");
				STRNSCPY(gua_pack.Rec.Symb, "WILDBERRIES");
				gua_pack.SetAccessKey(rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
			}
			THROW(tra.Commit());
		}
	}
	CATCHZOK
	return ok;
}

/*static*/int PPGlobalServiceHighLevelImplementations::Setup_Wildberries() // @v12.2.0
{
	class SetupGlobalServiceWildberries_Dialog : public TDialog {
		DECL_DIALOG_DATA(SetupGlobalServiceWildberries_Param);
		enum {
			dummyFirst = 1,
			brushInvalid,
			brushApiKeyValid,
			brushApiKeyEmpty,
		};
	public:
		SetupGlobalServiceWildberries_Dialog(DlgDataType & rData) : TDialog(DLG_SU_WB), Data(rData), ApiKeyBrushIdent(0)
		{
			Ptb.SetBrush(brushInvalid, SPaintObj::bsSolid, GetColorRef(SClrCoral), 0);
			Ptb.SetBrush(brushApiKeyValid, SPaintObj::bsSolid, GetColorRef(SClrAqua),  0);
			Ptb.SetBrush(brushApiKeyEmpty, SPaintObj::bsSolid, GetColorRef(SClrCadetblue),  0);
			const int max_text_len = 512;
			{
				TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_SUWB_APIKEY));
				CALLPTRMEMB(p_il, SetupMaxTextLen(max_text_len));
			}
			Setup_GlobalService_Wildberries_InitParam(Data, false);
			setCtrlString(CTL_SUWB_APIKEY, Data.ApiKey);
			SetupPPObjCombo(this, CTLSEL_SUWB_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, reinterpret_cast<void *>(PPGLS_WILDBERRIES));
			SetupCtrls();
		}
	private:
		int    Configure()
		{
			int    ok = -1;
			SString temp_buf;
			SString accs_key;
			SString name_buf;
			getCtrlString(CTL_SUWB_APIKEY, accs_key);
			if(accs_key.NotEmpty()) {
				PPID gua_id = 0;
				PPGlobalUserAccPacket gua_pack;
				PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult tdr;
				THROW(PPMarketplaceInterface_Wildberries::ParseApiToken(accs_key, &tdr));
				gua_id = getCtrlLong(CTLSEL_SUWB_GUA);
				if(gua_id) {
					THROW(GuaObj.GetPacket(gua_id, &gua_pack) > 0); // @todo @err
					THROW(gua_pack.Rec.ServiceIdent == PPGLS_WILDBERRIES); // @todo @err
					gua_pack.SetAccessKey(accs_key);
					THROW(GuaObj.PutPacket(&gua_id, &gua_pack, 1));
					setCtrlLong(CTLSEL_SUWB_GUA, gua_id);
					setCtrlString(CTL_SUWB_APIKEY, accs_key);
					ok = 1;
				}
				else {
					PPLoadString("marketplace", temp_buf);
					name_buf.Z().Cat(temp_buf);
					PPLoadString("brand_wildberries", temp_buf);
					name_buf.Space().Cat(temp_buf);
					{
						// ���������� ������������ �����
						temp_buf = name_buf;
						long _c = 0;
						while(!GuaObj.CheckDupName(0, temp_buf)) {
							(temp_buf = name_buf).Space().CatChar('#').Cat(++_c);
						}
						name_buf = temp_buf;
					}
					STRNSCPY(gua_pack.Rec.Name, name_buf);
					gua_pack.Rec.ServiceIdent = PPGLS_WILDBERRIES;
					gua_pack.SetAccessKey(accs_key);
					assert(gua_id == 0);
					THROW(GuaObj.PutPacket(&gua_id, &gua_pack, 1));
					Data.GuaID = gua_id;
					SetupPPObjCombo(this, CTLSEL_SUWB_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, reinterpret_cast<void *>(PPGLS_WILDBERRIES));
					setCtrlString(CTL_SUWB_APIKEY, accs_key);
					ok = 1;
				}
				if(ok > 0) {
					PPMarketplaceConfig cfg;
					PrcssrMarketplaceInterchange::ReadConfig(&cfg);
					THROW(PrcssrMarketplaceInterchange::AutoConfigure(cfg, 0, 1));
					THROW(PrcssrMarketplaceInterchange::WriteConfig(&cfg, 1));
				}
			}
			CATCHZOKPPERR
			return ok;
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			if(event.isCbSelected(CTLSEL_SUWB_GUA)) {
				const PPID gua_id = getCtrlLong(CTLSEL_SUWB_GUA);
				if(gua_id) {
					PPGlobalUserAccPacket gua_pack;
					if(GuaObj.GetPacket(gua_id, &gua_pack) > 0) {
						SString accs_key;
						gua_pack.GetAccessKey(accs_key);
						setCtrlString(CTL_SUWB_APIKEY, accs_key);
					}
				}
			}
			else if(event.isCmd(cmMarketplaceCfg)) {
				PrcssrMarketplaceInterchange::EditConfig();
			}
			else if(event.isCmd(cmConfigure)) {
				Configure();
			}
			/*else if(event.isCmd(cmAutoConfigure)) { // @v12.2.1
				PPMarketplaceConfig cfg;
				int is_new = PrcssrMarketplaceInterchange::ReadConfig(&cfg);
				if(PrcssrMarketplaceInterchange::AutoConfigure(cfg, 0, 1)) {
					if(PrcssrMarketplaceInterchange::WriteConfig(&cfg, 1)) {
						;
					}
					else {
						PPError();
					}
				}
			}*/
			else if(event.isCmd(cmInputUpdated)) {
				if(event.isCtlEvent(CTL_SUWB_APIKEY)) {
					SetupCtrls();
				}
			}
			else if(event.isCmd(cmCtlColor)) {
				TDrawCtrlData * p_dc = static_cast<TDrawCtrlData *>(TVINFOPTR);
				if(p_dc && getCtrlHandle(CTL_SUWB_APIKEY) == p_dc->H_Ctl) {
					if(ApiKeyBrushIdent) {
						::SetBkMode(p_dc->H_DC, TRANSPARENT);
						p_dc->H_Br = static_cast<HBRUSH>(Ptb.Get(ApiKeyBrushIdent));
					}
				}
				else
					return;
			}
			else
				return;
			clearEvent(event);
		}
		void   SetupCtrls()
		{
			SString temp_buf;
			getCtrlString(CTL_SUWB_APIKEY, temp_buf);
			int brush_ident = 0;
			SString info_buf;
			bool enable_configure = false;
			if(temp_buf.NotEmpty()) {
				PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult tdr;
				if(PPMarketplaceInterface_Wildberries::ParseApiToken(temp_buf, &tdr)) {
					enable_configure = true;
					info_buf.CatEq("id", tdr.Id, S_GUID::fmtIDL).CR().
						CatEq("seller-id", tdr.SellerId, S_GUID::fmtIDL).CR().
						CatEq("expiry", tdr.ExpiryDtm, DATF_ISO8601CENT, 0).CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fTest)
							info_buf.Cat("test").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fReadOnly)
							info_buf.Cat("read-only").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Content)
							info_buf.Cat("accs-content").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Analitycs)
							info_buf.Cat("accs-analitycs").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Prices)
							info_buf.Cat("accs-prices").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Marketplace)
							info_buf.Cat("accs-marktetplace").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Statistics)
							info_buf.Cat("accs-statistics").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Promo)
							info_buf.Cat("accs-promo").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_QAndReviews)
							info_buf.Cat("accs-qandreviews").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Chat)
							info_buf.Cat("accs-chat").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Deliveries)
							info_buf.Cat("accs-deliveries").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Returns)
							info_buf.Cat("accs-returns").CR();
						if(tdr.Flags & PPMarketplaceInterface_Wildberries::ApiTokenDecodeResult::fAccs_Documents)
							info_buf.Cat("accs-documents").CR();
					ApiKeyBrushIdent = brushApiKeyValid;
				}
				else {
					info_buf = "invalid key";
					ApiKeyBrushIdent = brushInvalid;
				}
			}
			else {
				ApiKeyBrushIdent = brushApiKeyEmpty;
			}
			setStaticText(CTL_SUWB_HINT, info_buf);
			enableCommand(cmConfigure, enable_configure);
		}
		PPObjGlobalUserAcc GuaObj;
		SPaintToolBox Ptb;
		int   ApiKeyBrushIdent;
	};
	int    ok = -1;
	SetupGlobalServiceWildberries_Param param;
	SetupGlobalServiceWildberries_Dialog * dlg = new SetupGlobalServiceWildberries_Dialog(param);
	if(CheckDialogPtrErr(&dlg)) {
		ExecView(dlg);
		//if(dlg->IsSettled())
			//ok = 1;
	}
	delete dlg;
	return ok;
}

SString & VkInterface::GetAppIdent(SString & rBuf) const
{
	return (rBuf = AppIdent);
}

struct SetupGlobalServiceVK_Param {
	SetupGlobalServiceVK_Param() : GuaID(0)
	{
	}
	SString Login;
	SString ApiKey;
	SString PageIdent;
	SString GroupIdent;
	PPID   GuaID;
};

static int Setup_GlobalService_VK_InitParam(SetupGlobalServiceVK_Param & rP, int force)
{
	int    ok = 1;
	const  PPID service_ident = PPGLS_VK;
	SString temp_buf;
	PPObjGlobalUserAcc gua_obj;
	PPObjTag tag_obj;
	{
		PPGlobalUserAccPacket gua_pack;
		if(!force) {
			if(rP.GuaID && gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0) {
				SString db_value;
				gua_pack.GetAccessKey(db_value);
				if(db_value.NotEmptyS() && !rP.ApiKey.NotEmptyS())
					rP.ApiKey = db_value;
				gua_pack.TagL.GetItemStr(PPTAG_GUA_SOCIALGROUPCODE, db_value.Z());
				if(db_value.NotEmptyS() && !rP.GroupIdent.NotEmptyS())
					rP.GroupIdent = db_value;
				gua_pack.TagL.GetItemStr(PPTAG_GUA_SOCIALPAGECODE, db_value.Z());
				if(db_value.NotEmptyS() && !rP.PageIdent.NotEmptyS())
					rP.PageIdent = db_value;
			}
		}
		else {
			THROW(rP.ApiKey.NotEmpty());
			THROW(rP.GroupIdent.NotEmpty());
			THROW(rP.PageIdent.NotEmpty());
			//
			// ���� ���������� - ������� ����������������� ����.
			// Note: ������� PPObject::MakeReserve ���������� �� ����� ����������� �����������
			//
			PPObjectTag tag_rec;
			int  do_make_reserved = 0;
			if(tag_obj.Search(PPTAG_GUA_OUTERWAREIDTAG, &tag_rec) <= 0)
				do_make_reserved = 1;
			if(tag_obj.Search(PPTAG_GUA_SOCIALGROUPCODE, &tag_rec) <= 0)
				do_make_reserved = 1;
			if(tag_obj.Search(PPTAG_GUA_SOCIALPAGECODE, &tag_rec) <= 0)
				do_make_reserved = 1;
			if(tag_obj.Search(PPTAG_GUA_ACCESSKEY, &tag_rec) <= 0)
				do_make_reserved = 1;
			if(do_make_reserved) {
				THROW(tag_obj.MakeReserved(0));
			}
			{
				PPID   temp_id = 0;
				PPTransaction tra(1);
				THROW(tra);
				if(rP.GuaID) {
					temp_id = rP.GuaID;
					THROW(gua_obj.GetPacket(rP.GuaID, &gua_pack) > 0);
					THROW_PP_S(gua_pack.Rec.ServiceIdent == service_ident, PPERR_INVGUASERVICEIDENT, temp_buf.Z());
				}
				else {
					// SMGRPID SMPAGEID
					gua_pack.Rec.ServiceIdent = service_ident;
					STRNSCPY(gua_pack.Rec.Name, "VK account");
					STRNSCPY(gua_pack.Rec.Symb, "VKACC");
				}
				{
					gua_pack.SetAccessKey(rP.ApiKey);
					gua_pack.TagL.PutItemStr(PPTAG_GUA_SOCIALGROUPCODE, rP.GroupIdent);
					gua_pack.TagL.PutItemStr(PPTAG_GUA_SOCIALPAGECODE, rP.PageIdent);
					{
						if(!gua_pack.TagL.GetItem(PPTAG_GUA_OUTERWAREIDTAG)) {
							PPID  outerwareid_tag_id = 0;
							{
								PPObjectTag temp_rec;
								PPObjTagPacket new_tag_pack;
								PPID   new_tag_id = 0;
								PPLoadString("goodsident", temp_buf);
								temp_buf.Space().Cat("VK");
								STRNSCPY(new_tag_pack.Rec.Name, temp_buf);
								STRNSCPY(new_tag_pack.Rec.Symb, "OUTERWAREID-VK");
								new_tag_pack.Rec.ObjTypeID = PPOBJ_GOODS;
								new_tag_pack.Rec.TagDataType = OTTYP_STRING;
								if(tag_obj.SearchBySymb(new_tag_pack.Rec.Symb, 0, &temp_rec) > 0) {
									outerwareid_tag_id = temp_rec.ID;
								}
								else {
									THROW(tag_obj.AddItem(&outerwareid_tag_id, &new_tag_pack.Rec, 0));
								}
							}
							{
								ObjTagItem tag_item;
								tag_item.SetInt(PPTAG_GUA_OUTERWAREIDTAG, outerwareid_tag_id);
								gua_pack.TagL.PutItem(PPTAG_GUA_OUTERWAREIDTAG, &tag_item);
							}
						}
					}
				}
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 0));
				THROW(tra.Commit());
				//
				rP.GuaID = temp_id;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPGlobalServiceHighLevelImplementations::Setup_VK()
{
	class SetupGlobalServiceVK_Dialog : public TDialog {
		DECL_DIALOG_DATA(SetupGlobalServiceVK_Param);
		//DECL_DIALOG_DATA(VkInterface::InitBlock);
		const long GlobalService;
	public:
		SetupGlobalServiceVK_Dialog(SetupGlobalServiceVK_Param & rData) : TDialog(/*DLG_VKGUACFG*/DLG_SU_VK), Data(rData), GlobalService(PPGLS_VK)
		{
			const int max_text_len = 512;
			{
				TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_SUVK_URL));
				CALLPTRMEMB(p_il, SetupMaxTextLen(max_text_len));
			}
			{
				TInputLine * p_il = static_cast<TInputLine *>(getCtrlView(CTL_SUVK_APIKEY));
				CALLPTRMEMB(p_il, SetupMaxTextLen(max_text_len));
			}
			SetupPPObjCombo(this, CTLSEL_SUVK_GUA, PPOBJ_GLOBALUSERACC, Data.GuaID, OLW_CANINSERT, reinterpret_cast<void *>(GlobalService));
			{
				SString info_buf;
				SString temp_buf;
				info_buf.Cat("App ident").CatDiv(':', 2).Cat(Ifc.GetAppIdent(temp_buf));
				setCtrlString(CTL_SUVK_INFO, info_buf);
			}
			SetupGua(true);
		}
		DECL_DIALOG_SETDTS()
		{
			PPGlobalUserAccConfig cfg;
			GuaObj.FetchConfig(&cfg);
			RVALUEPTR(Data, pData);
			// ����� ���������� ���� ������
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			//����� ������� ������ � groupid, ownerid and token
			getCtrlString(CTL_SUVK_LOGIN, Data.Login);
			getCtrlString(CTL_SUVK_APIKEY, Data.ApiKey);
			getCtrlString(CTL_SUVK_GROUPIDENT, Data.GroupIdent);
			getCtrlString(CTL_SUVK_PAGEIDENT, Data.PageIdent);
			getCtrlData(CTLSEL_SUVK_GUA, &Data.GuaID);
			ASSIGN_PTR(pData, Data);
			//CATCHZOKPPERRBYDLG
			return ok;
		}
		int    IsSettled()
		{
			return 0;
		}
	private:
		void Configure()
		{
			getDTS(0);
			if(Setup_GlobalService_VK_InitParam(Data, 1)) {
				// ..
			}
			else
				PPError();
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			SString temp_buf;
			if(event.isCmd(cmQueryToken)) {
				Ifc.GetVKAccessToken();
			}
			else if(event.isCmd(cmConfigure)) {
				Configure();
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_SUVK_URL/*CTL_VKGUACFG_LOGIN_URL*/)) {
				UI_LOCAL_LOCK_ENTER
					getCtrlString(CTL_SUVK_URL/*CTL_VKGUACFG_LOGIN_URL*/, temp_buf.Z());
					{
						InetUrl url(temp_buf);
						url.GetComponent(InetUrl::cRef, 0, temp_buf);
						SString left;
						SString right;
						StringSet ss('&', temp_buf);
						for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
							temp_buf.Divide('=', left, right);
							if(left.IsEq("access_token"))
								Data.ApiKey.Z().Cat(right);
							else if(left.IsEq("user_id"))
								Data.PageIdent.Z().Cat(right);
						}
						setCtrlString(CTL_SUVK_APIKEY, Data.ApiKey);
						setCtrlString(CTL_SUVK_PAGEIDENT, Data.PageIdent);
					}
				UI_LOCAL_LOCK_LEAVE
			}
			else if(event.isCmd(cmInputUpdated) && event.isCtlEvent(CTL_SUVK_GROUPIDENT/*CTL_VKGUACFG_GROUP_ID*/)) {
				UI_LOCAL_LOCK_ENTER
					getCtrlString(CTL_SUVK_GROUPIDENT/*CTL_VKGUACFG_GROUP_ID*/, temp_buf.Z());
					Data.GroupIdent = temp_buf;
				UI_LOCAL_LOCK_LEAVE
			}
			else if(TVBROADCAST && TVCMD == cmReceivedFocus) {
				SString temp_buf;
				if(event.isCtlEvent(CTL_SUVK_GROUPIDENT)) {
					PPLoadStringDescription("setupglbsvc_vk_groupident", temp_buf);
				}
				/*else if(event.isCtlEvent(CTL_SUUDS_APIKEY)) {
					PPLoadStringDescription("setupglbsvc_uds_apikey", temp_buf);
				}*/
				setStaticText(CTL_SUVK_HINT, temp_buf);
			}
			else if(event.isCbSelected(CTLSEL_SUVK_GUA)) {
				SetupGua(false);
			}
			else
				return;
			clearEvent(event);
		}
		void SetupGua(bool force)
		{
			UI_LOCAL_LOCK_ENTER
				PPID   gua_id = getCtrlLong(CTLSEL_SUVK_GUA);
				if(force || gua_id != Data.GuaID) {
					if(force && !gua_id) {
						PPIDArray gua_list;
						if(GuaObj.GetListByServiceIdent(GlobalService, &gua_list) > 0 && gua_list.getCount() == 1) {
							gua_id = gua_list.get(0);
							setCtrlLong(CTLSEL_SUVK_GUA, gua_id);
						}
					}
					Data.GuaID = gua_id;
					PPGlobalUserAccPacket gua_pack;
					SString group_code;
					SString page_code;
					SString accs_key;
					if(GuaObj.GetPacket(gua_id, &gua_pack) > 0) {
						gua_pack.TagL.GetItemStr(PPTAG_GUA_SOCIALGROUPCODE, group_code);
						gua_pack.TagL.GetItemStr(PPTAG_GUA_SOCIALPAGECODE, page_code);
						gua_pack.GetAccessKey(accs_key);
					}
					setCtrlString(CTL_SUVK_APIKEY, accs_key);
					setCtrlString(CTL_SUVK_GROUPIDENT, group_code);
					setCtrlString(CTL_SUVK_PAGEIDENT, page_code);
				}
			UI_LOCAL_LOCK_LEAVE
		}
		PPObjGlobalUserAcc GuaObj;
		VkInterface Ifc;
	};
	int    ok = -1;
	SetupGlobalServiceVK_Param param;
	SetupGlobalServiceVK_Dialog * dlg = new SetupGlobalServiceVK_Dialog(param);
	if(CheckDialogPtrErr(&dlg)) {
		ExecView(dlg);
		if(dlg->IsSettled())
			ok = 1;
	}
	delete dlg;
	return ok;
}
//
//
//
/*static*/int PPGlobalServiceHighLevelImplementations::ExportGoods_VK(
	const PPObjGoods::ExportToGlbSvcParam & rParam, const TSVector <PPObjGoods::ExportToGlbSvcItem> & rSrcList, PPLogger * pLogger)
{
	assert(rParam.GlobalService == PPGLS_VK);
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPObjGoods goods_obj;
	VkInterface ifc;
	SString temp_buf;
	SString msg_buf;
	SString def_img_path;
	ObjLinkFiles lf(PPOBJ_GOODS);
	PPWaitStart();
	PPGetFilePath(PPPATH_DD, PPFILNAM_NOIMAGE, def_img_path);
	if(ifc.Setup(rParam.GuaID, ifc.sfInitStoreAttributes)) {
		PPGoodsPacket pack;
		TSCollection <VkInterface::MarketWareItem> ex_outer_goods_id_list;
		const int goods_descr_ext_str_id = oneof5(rParam.DescrExtStrId, GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E) ? rParam.DescrExtStrId : GDSEXSTR_A;
		PPWaitStart();
		if(!ifc.Market_Get(0, 200, ex_outer_goods_id_list)) {
			const VkInterface::ErrorResponse & r_err = ifc.GetLastResponseError();
			if(r_err.Code) {
				CALLPTRMEMB(pLogger, LogMsgCode(mfError, PPERR_VK_RESPONSE, (temp_buf = r_err.Message).Transf(CTRANSF_UTF8_TO_INNER)));
			}
		}
		else {
			for(uint i = 0; i < rSrcList.getCount(); i++) {
				PPObjGoods::ExportToGlbSvcItem & r_src_item = rSrcList.at(i);
				if(r_src_item.Price <= 0.01) {
					PPSetError(PPERR_INVGOODSPRICE, GetGoodsName(r_src_item.GoodsID, temp_buf));
					CALLPTRMEMB(pLogger, LogLastError());
				}
				else {
					const  PPID native_goods_id = r_src_item.GoodsID;
					const  PPID native_loc_id = r_src_item.LocID;
					if(goods_obj.GetPacket(native_goods_id, &pack, 0) > 0) {
						SString link_file_path;
						int   link_file_type = 0;
						VkInterface::MarketWareItem market_item(r_src_item);
						VkInterface::MarketWareItem result_market_item;
						market_item.Name = pack.Rec.Name;
						const ObjTagItem * p_stored_tag_item = pack.TagL.GetItem(ifc.GetOuterWareIdentTagID());
						lf.Load(native_goods_id, 0L);
						lf.At(0, market_item.ImgPath);
						if(market_item.ImgPath.NotEmptyS()) {
							link_file_path = market_item.ImgPath;
							link_file_type = 1;
						}
						else {
							CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_UHTT_GOODSNOIMG, &msg_buf, native_goods_id))); // PPTXT_LOG_UHTT_GOODSNOIMG "��� ������ @goods ��� �����������"
							market_item.ImgPath = def_img_path;
							link_file_type = 1;
							if(!fileExists(market_item.ImgPath)) {
								CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NODEFIMG, &msg_buf, native_goods_id))); // "����������� ��-��������� �� �������!"
								continue;
							}
						}
						if(pack.GetExtStrData(goods_descr_ext_str_id, temp_buf) > 0)
							market_item.Description = temp_buf;
						else
							market_item.Description = pack.Rec.Name;
						{
							int64 outer_goods_id_by_tag = 0;
							if(p_stored_tag_item)
								p_stored_tag_item->GetInt64(&outer_goods_id_by_tag);
							if(ex_outer_goods_id_list.lsearch(&outer_goods_id_by_tag, 0, PTR_CMPFUNC(int64), offsetof(VkInterface::MarketWareItem, OuterId))) {
								market_item.OuterId = outer_goods_id_by_tag;
							}
							//market_item.OuterId = outer_goods_id_by_tag;
						}
						if(!ifc.PutWareToMarket(market_item, result_market_item)) {
							const VkInterface::ErrorResponse & r_err = ifc.GetLastResponseError();
							if(r_err.Message.NotEmpty()) {
								CALLPTRMEMB(pLogger, LogMsgCode(mfError, PPERR_VK_RESPONSE, (temp_buf = r_err.Message).Transf(CTRANSF_UTF8_TO_INNER)));
							}
						}
						else if(result_market_item.OuterId > 0) {
							ObjTagItem new_obj_tag_item;
							temp_buf.Z().Cat(result_market_item.OuterId);
							new_obj_tag_item.SetStr(ifc.GetOuterWareIdentTagID(), temp_buf);
							if(!p_stored_tag_item || new_obj_tag_item != *p_stored_tag_item) {
								p_ref->Ot.PutTag(PPOBJ_GOODS, pack.Rec.ID, &new_obj_tag_item, 1);
							}
						}
					}
				}
				PPWaitPercent(i, rSrcList.getCount());
			}
		}
	}
	PPWaitStop();
	return ok;
}

/*static*/int PPGlobalServiceHighLevelImplementations::ExportGoods_UDS(
	const PPObjGoods::ExportToGlbSvcParam & rParam, const TSVector <PPObjGoods::ExportToGlbSvcItem> & rSrcList, PPLogger * pLogger)
{
	assert(rParam.GlobalService == PPGLS_UDS);
	int    ok = -1;
	PPObjGoods goods_obj;
	UdsGameInterface ifc;
	SString temp_buf;
	Reference * p_ref = PPRef;
	if(ifc.Setup(0)) {
		SString ex_outer_goods_id;
		SString category_name;
		SString description_buf;
		Goods2Tbl::Rec goods_rec; 
		PPGoodsPacket pack;
		ObjTagItem stored_obj_tag_item;
		//LAssocArray goods_category_assoc_list; // native_id-->native_parent_id
		LLAssocArray category_assoc_list; // native_id-->outer_id ���������� ��������������� ��������� 
			// ��� ��������� ��������� ������� �������� ������ � ��� �� ���������
		StrAssocArray category_name_list; // native_id-->name
		TSCollection <UdsGameInterface::GoodsItem> ex_goods_item_list;
		{
			//
			// ������, ������ ������� � UDS �������� ������ � ���� ��������� ����������. ���� ���, �� ������������ ������ ������� �������.
			//
			{
				UdsGameInterface::GoodsItemFilt filt;
				filt.Count = 50;
				uint   local_total_count = 0;
				uint   fetched_count = 0;
				while(ifc.GetPriceItemList(filt, ex_goods_item_list, &fetched_count, &local_total_count) > 0) {
					filt.Offset += fetched_count;
				}
			}
			{
				TSCollection <UdsGameInterface::GoodsItem> local_list;
				for(uint i = 0; i < ex_goods_item_list.getCount(); i++) {
					const UdsGameInterface::GoodsItem * p_ex_item = ex_goods_item_list.at(i);
					if(p_ex_item->Type == UdsGameInterface::GoodsItem::typCategory) {
						UdsGameInterface::GoodsItemFilt filt;
						filt.ParentId = p_ex_item->OuterId;
						filt.Count = 50;
						uint   local_total_count = 0;
						uint   fetched_count = 0;
						while(ifc.GetPriceItemList(filt, local_list, &fetched_count, &local_total_count) > 0) {
							filt.Offset += fetched_count;
						}						
					}
				}
				for(uint j = 0; j < local_list.getCount(); j++) {
					const UdsGameInterface::GoodsItem * p_ex_item = local_list.at(j);
					if(p_ex_item) {
						// ���������� ������� �� ��������� ��������� � �����
						ex_goods_item_list.insert(p_ex_item);
						local_list.atPut(j, 0);
					}
				}
			}
		}
		{
			for(uint i = 0; i < rSrcList.getCount(); i++) {
				const PPObjGoods::ExportToGlbSvcItem & r_src_item = rSrcList.at(i);
				const  PPID native_goods_id = r_src_item.GoodsID;
				const  PPID native_loc_id = r_src_item.LocID;
				if(goods_obj.Search(native_goods_id, &goods_rec) > 0) {
					//GoodsRestViewItem view_item;
					//GetItem(native_goods_id, native_loc_id, &view_item);
					ex_outer_goods_id.Z();
					description_buf.Z();
					category_name.Z();
					int64 outer_category_id = 0;
					long  native_category_id = 0;
					const UdsGameInterface::GoodsItem * p_ex_goods_item = 0;
					if(p_ref->Ot.GetTag(PPOBJ_GOODS, native_goods_id, ifc.GetOuterWareIdentTagID(), &stored_obj_tag_item) > 0) {
						stored_obj_tag_item.GetStr(ex_outer_goods_id);
						uint ex_goods_item_idx = UdsGameInterface::SearchOuterIdentInCollection(ex_goods_item_list, ex_outer_goods_id);
						if(ex_goods_item_idx)
							p_ex_goods_item = ex_goods_item_list.at(ex_goods_item_idx-1);
					}
					{
						if(rParam.DescrExtStrId) {
							pack.Rec.ID = native_goods_id; // @trick
							pack.Rec.Flags |= GF_EXTPROP; // @trick
							if(goods_obj.GetValueAddedData(native_goods_id, &pack) > 0 && pack.GetExtStrData(rParam.DescrExtStrId, temp_buf) > 0)
								description_buf = temp_buf;
						}
						if(rParam.CategoryObject == PPObjGoods::ExportToGlbSvcParam::coTag) {
							if(rParam.CategoryTagID) {
								if(p_ref->Ot.GetTagStr(PPOBJ_GOODS, native_goods_id, rParam.CategoryTagID, temp_buf) > 0) {
									uint   cp = 0;
									if(category_name_list.SearchByText(temp_buf, &cp))
										native_category_id = category_name_list.Get(cp).Id;
									else {
										long max_cat_id = 0;
										category_name_list.GetMaxID(&max_cat_id);
										native_category_id = max_cat_id+1;
										category_name_list.Add(native_category_id, temp_buf);
									}
									category_name = temp_buf;
								}
							}
						}
						else if(rParam.CategoryObject == PPObjGoods::ExportToGlbSvcParam::coGoodsGrpName) {
							Goods2Tbl::Rec grp_rec;
							if(goods_obj.Fetch(goods_rec.ParentID, &grp_rec) > 0) {
								native_category_id = grp_rec.ID;
								category_name = grp_rec.Name;
							}
						}
						if(category_name.NotEmpty()) {
							assert(native_category_id != 0);
							int64 temp_id64 = 0;
							if(category_assoc_list.Search(native_category_id, &temp_id64, 0) > 0) {
								assert(temp_id64 != 0);
								outer_category_id = temp_id64;
							}
							else {
								for(uint catidx = 0; catidx < ex_goods_item_list.getCount(); catidx++) {
									const UdsGameInterface::GoodsItem * p_ex_item = ex_goods_item_list.at(catidx);
									if(p_ex_item && p_ex_item->Type == UdsGameInterface::GoodsItem::typCategory) {
										(temp_buf = p_ex_item->Name).Transf(CTRANSF_UTF8_TO_INNER);
										if(temp_buf.IsEqNC(category_name)) {
											outer_category_id = p_ex_item->OuterId;	
											break;
										}
									}
								}
							}
							if(!outer_category_id) {
								UdsGameInterface::GoodsItem new_category_item;
								UdsGameInterface::GoodsItem ret_category_item;
								new_category_item.Type = UdsGameInterface::GoodsItem::typCategory;
								(new_category_item.Name = category_name).Transf(CTRANSF_INNER_TO_UTF8);
								if(ifc.CreatePriceItem(new_category_item, ret_category_item) > 0) {
									outer_category_id = ret_category_item.OuterId;
									category_assoc_list.Add(native_category_id, outer_category_id, 0);
								}
							}
						}
					}
					if(!(rParam.Flags & rParam.fOnlyUnassocItems) || !p_ex_goods_item) {
						S_GUID native_uuid;
						UdsGameInterface::GoodsItem ret_item;
						THROW(goods_obj.GetUuid(native_goods_id, native_uuid, true, 1));
						{
							UdsGameInterface::GoodsItem item_to_send;
							item_to_send.Type = UdsGameInterface::GoodsItem::typItem;
							item_to_send.ParentId = outer_category_id;
							item_to_send.Ident.Z().Cat(native_uuid, S_GUID::fmtIDL|S_GUID::fmtLower);
							(item_to_send.Name = goods_rec.Name).Transf(CTRANSF_INNER_TO_UTF8);
							if(description_buf.NotEmptyS())
								(item_to_send.Description = description_buf).Transf(CTRANSF_INNER_TO_UTF8);
							item_to_send.Price = r_src_item.Price;
							int   transfer_result = 0;
							if(p_ex_goods_item) {
								item_to_send.OuterId = p_ex_goods_item->OuterId;
								transfer_result = ifc.UpdatePriceItem(item_to_send, ret_item);
							}
							else {
								transfer_result = ifc.CreatePriceItem(item_to_send, ret_item);
							}
							if(transfer_result > 0) {
								if(ifc.GetOuterWareIdentTagID()) {
									ObjTagItem new_obj_tag_item;
									temp_buf.Z().Cat(ret_item.OuterId);
									new_obj_tag_item.SetStr(ifc.GetOuterWareIdentTagID(), temp_buf);
									p_ref->Ot.PutTag(PPOBJ_GOODS, goods_rec.ID, &new_obj_tag_item, 1);
								}
							}
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
class GoogleApiInterface {
public:
	GoogleApiInterface()
	{
	}
	~GoogleApiInterface()
	{
	}
	int     Auth();
};

int GoogleApiInterface::Auth()
{
	int    ok = 0;
	//
	// ��������� ����:
	// https://accounts.google.com/o/oauth2/v2/auth?scope=https://www.google.com/m8/feeds&access_type=offline&include_granted_scopes=true&redirect_uri=urn:ietf:wg:oauth:2.0:oob&response_type=code&client_id=921647******-l5jcha3bt7r6q******bhtsgk*****um6.apps.googleusercontent.com
	//     scope � ������, � �������� �� �������� ������. ��� ��������� ��� www.google.com/m8/feeds. 
	//             ������ ������� ������ �������� ��������� �� ������ developers.google.com/identity/protocols/googlescopes;
	//     access_type � ��� �������. ���� ��� ����� ����� ��������� ������ ��� ������� ������������, ����������� ����������� �������� offline. 
	//             ����� �������� �������� online, �� ��� ��� ������ ���������� ����� ������ ��� ����������� ���������� � ������������ � ��������
	//     redirect_uri � client_id � ������, ������� ������� � ����� �������, ������� ��� ������ �� ������ �����
	//
	// ��������� ������:
	// end-point: https://www.googleapis.com/oauth2/v4/token
	// content-type ������� ������ ���� application/x-www-form-urlencoded
	//
	return ok;
}
//
//
//
class AptekaRuInterface {
public:
	static constexpr PPID BillIdentTag = PPTAG_BILL_ORGORDERIDENT; // @v12.2.2 PPTAG_BILL_EDIIDENT-->PPTAG_BILL_ORGORDERIDENT

	struct ReplyEntry {
		ReplyEntry();
		ReplyEntry & Z();
		int    FromJsonObj(const SJson * pJs);

		enum {
			stUnkn    = 0,
			stSuccess = 1,
			stNotFound,
			stConflict,
		};
		PPID  SrcID;
		int   Status;
		SString Code;
		SString Msg;
	};
	explicit AptekaRuInterface(PPID guaID);
	~AptekaRuInterface();
	bool   IsValid() const;
	int    Auth(bool test);
	int    ReportAcceptance(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList);
	int    ReportBuyOut(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList);
	int    ReportReturn(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList);
	const  PPGlobalUserAccPacket & GetGuaPack() const { return GuaPack; }
private:
	int    GetUrl(bool forceTest, SString & rUrlBuf) const;
	SString & MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf);
	int    ParseReply(const SJson * pJs, TSCollection <ReplyEntry> & rRepList) const;
	int    MakeBillCodeList(const PPIDArray & rIdList, StringSet & rSs, PPIDArray & rResultIdList);
	int    MakeOrderNumsRequest(const StringSet & rSs, SString & rBuf) const;

	PPGlobalServiceLogTalkingHelper Lth;
	PPID   GuaID;
	PPGlobalUserAccPacket GuaPack;
	//
	SString Token;
	uint   TokenLifeTime_Minuts;
};

/*
�������� ��� - https://pharmapi.apteka.tech
�������� ��� - https://pharmapi.apteka.ru
*/

AptekaRuInterface::ReplyEntry::ReplyEntry() : SrcID(0), Status(stUnkn)
{
}

AptekaRuInterface::ReplyEntry & AptekaRuInterface::ReplyEntry::Z()
{
	SrcID = 0;
	Status = stUnkn;
	Code.Z();
	Msg.Z();
	return *this;
}

int AptekaRuInterface::ReplyEntry::FromJsonObj(const SJson * pJs)
{
	int    ok = 0;
	Z();
	if(SJson::IsObject(pJs)) {
		SString temp_buf;
		for(const SJson * p_jsn = pJs->P_Child; p_jsn; p_jsn = p_jsn->P_Next) {
			if(p_jsn->Text.IsEqiAscii("orderNum")) {
				Code = p_jsn->P_Child->Text.Unescape();
			}
			else if(p_jsn->Text.IsEqiAscii("status")) {
				temp_buf = p_jsn->P_Child->Text.Unescape();
				if(temp_buf.IsEqiAscii("Unknown")) {
					Status = stUnkn;
				}
				else if(temp_buf.IsEqiAscii("Success")) {
					Status = stSuccess;
				}
				else if(temp_buf.IsEqiAscii("NotFound")) {
					Status = stNotFound;
				}
				else if(temp_buf.IsEqiAscii("Conflict")) {
					Status = stConflict;
				}
			}
			else if(p_jsn->Text.IsEqiAscii("errorMessage")) {
				Msg = p_jsn->P_Child->Text.Unescape();
			}
		}
		if(Code.NotEmpty())
			ok = 1;
	}
	return ok;
}

int AptekaRuInterface::GetUrl(bool forceTest, SString & rUrlBuf) const
{
	rUrlBuf.Z();
	int    ok = 1;
	if(forceTest || (GuaPack.Rec.ID && GuaPack.Rec.Flags & PPGlobalUserAcc::fSandBox)) {
		rUrlBuf = "https://pharmapi.apteka.tech";
	}
	else {
		rUrlBuf = "https://pharmapi.apteka.ru";
	}
	return ok;
}

AptekaRuInterface::AptekaRuInterface(PPID guaID) : Lth(PPFILNAM_APTEKARUTALK_LOG), GuaID(guaID), TokenLifeTime_Minuts(0)
{
	if(GuaID) {
		PPObjGlobalUserAcc gua_obj;
		if(gua_obj.GetPacket(GuaID, &GuaPack) > 0) {
			;
		}
		else
			GuaID = 0;
	}
}
	
AptekaRuInterface::~AptekaRuInterface()
{
}

bool AptekaRuInterface::IsValid() const
{
	return (GuaID != 0);
}

SString & AptekaRuInterface::MakeHeaderFields(const char * pToken, StrStrAssocArray * pHdrFlds, SString & rBuf)
{
	StrStrAssocArray hdr_flds;
	SETIFZ(pHdrFlds, &hdr_flds);
	SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrContentType, "application/json;charset=UTF-8");
	SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrCacheControl, "no-cache");
	SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAcceptLang, "ru");
	SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAccept, "application/json");
	if(!isempty(pToken)) {
		SString temp_buf;
		(temp_buf = "Bearer").Space().Cat(pToken);
		SHttpProtocol::SetHeaderField(*pHdrFlds, SHttpProtocol::hdrAuthorization, temp_buf);
	}
	SHttpProtocol::PutHeaderFieldsIntoString(*pHdrFlds, rBuf);
	return rBuf;
}
	
int AptekaRuInterface::Auth(bool test)
{
	Token.Z();
	TokenLifeTime_Minuts = 0;
	//
	int    ok = 1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString reply_buf;
	SString user;
	SString secret;
	THROW_PP(GuaPack.Rec.ID, PPERR_APTEKARU_AUTHFAULT_NOGUA);
	GuaPack.TagL.GetItemStr(PPTAG_GUA_LOGIN, user);
	//GuaPack.GetAccessKey(secret);
	GuaPack.TagL.GetItemStr(PPTAG_GUA_SECRET, secret);		
	THROW_PP(user.NotEmpty() && secret.NotEmpty(), PPERR_APTEKARU_AUTHFAULT_INVATTRS);
	{
		SJson js_req(SJson::tOBJECT);
		js_req.InsertString("login", user);
		js_req.InsertString("password", secret);
		js_req.ToStr(req_buf);
		THROW(GetUrl(test, url_buf));
		{
			ScURL c;
			url_buf.SetLastDSlash().Cat("Auth");
			InetUrl url(url_buf);
			StrStrAssocArray hdr_flds;
			MakeHeaderFields(0/*token*/, &hdr_flds, temp_buf);
			SBuffer ack_buf;
			SFile wr_stream(ack_buf.Z(), SFile::mWrite);
			Lth.Log("req", url_buf, req_buf);
			THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
			{
				SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
				if(p_ack_buf) {
					reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
					Lth.Log("rep", 0, reply_buf);
					SJson * p_js_reply = SJson::Parse(reply_buf);
					if(SJson::IsObject(p_js_reply)) {
						for(const SJson * p_cur = p_js_reply->P_Child; p_cur; p_cur = p_cur->P_Next) {							
							if(p_cur->Text.IsEqiAscii("token")) {
								Token = p_cur->P_Child->Text.Unescape();
							}
							else if(p_cur->Text.IsEqiAscii("lifetimeInMinutes")) {
								TokenLifeTime_Minuts = static_cast<uint>(p_cur->P_Child->Text.ToLong());
							}
							else {
								;
							}
						}
					}
					else {
						CALLEXCEPT_PP_S(PPERR_APTEKARU_AUTHFAULT, reply_buf.Transf(CTRANSF_UTF8_TO_INNER));
					}
					ZDELETE(p_js_reply);
				}
				THROW_PP(Token.NotEmpty(), PPERR_APTEKARU_AUTHFAULT);
			}
		}
	}
	CATCHZOK
	return ok;
}

int AptekaRuInterface::ParseReply(const SJson * pJs, TSCollection <ReplyEntry> & rRepList) const
{
	int    ok = 0;
	if(SJson::IsObject(pJs)) {
		for(const SJson * p_cur = pJs->P_Child; p_cur; p_cur = p_cur->P_Next) {							
			if(p_cur->Text.IsEqiAscii("orderNums")) {
				if(SJson::IsArray(p_cur->P_Child)) {
					ok = 1;
					for(const SJson * p_js_item = p_cur->P_Child->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
						uint new_item_pos = 0;
						ReplyEntry * p_re = rRepList.CreateNewItem(&new_item_pos);
						if(p_re) {
							if(p_re->FromJsonObj(p_js_item)) {
								; // ok
							}
							else {
								rRepList.atFree(new_item_pos);
								p_re = 0;
							}
						}
					}
				}
			}
			else {
				;
			}
		}
	}
	return ok;
}

int AptekaRuInterface::MakeBillCodeList(const PPIDArray & rIdList, StringSet & rSs, PPIDArray & rResultIdList)
{
	int    ok = -1;
	rSs.Z();
	rResultIdList.Z();
	if(rIdList.getCount()) {
		PPObjBill * p_bobj = BillObj;
		Reference * p_ref = PPRef;
		if(p_bobj && p_ref) {
			SString temp_buf;
			BillTbl::Rec bill_rec;
			for(uint i = 0; i < rIdList.getCount(); i++) {
				const PPID bill_id = rIdList.get(i);
				if(!rResultIdList.lsearch(bill_id) && p_bobj->Fetch(bill_id, &bill_rec) > 0) {
					//temp_buf = bill_rec.Code;
					if(p_ref->Ot.GetTagStr(PPOBJ_BILL, bill_id, AptekaRuInterface::BillIdentTag, temp_buf) > 0) {
						assert(temp_buf.NotEmpty());
						if(temp_buf.NotEmpty()) {
							rSs.add(temp_buf);
							rResultIdList.add(bill_id);
							ok = 1;
						}
					}
				}
			}
		}
	}
	return ok;
}

int AptekaRuInterface::MakeOrderNumsRequest(const StringSet & rSs, SString & rBuf) const
{
	rBuf.Z();
	int    ok = 1;
	const uint ssc_count = rSs.getCount();
	if(ssc_count) {
		SJson js_req(SJson::tOBJECT);
		SJson * p_js_code_array = SJson::CreateArr();
		SString temp_buf;
		for(uint ssp = 0; rSs.get(&ssp, temp_buf);) {
			p_js_code_array->InsertChild(SJson::CreateString(temp_buf));
		}
		js_req.Insert("orderNums", p_js_code_array);
		js_req.ToStr(rBuf);
	}
	else
		ok = -1;
	return ok;
}

int AptekaRuInterface::ReportAcceptance(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList)
{
	int    ok = -1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString reply_buf;
	if(Token.NotEmpty()) {
		StringSet ss_code;
		PPIDArray sent_id_list;
		if(test) {
			/*
				{
					"orderNums":[
						"TE-21000200",      //������ �������� �����
						"TE-21000409",      //������ � ����������
						"TE-21000400",      //������ ����������� ������
						"TE-21000204"       //������ �� ������ � ��
					]
				}
			*/
			ss_code.add("TE-21000200");
			ss_code.add("TE-21000409");
			ss_code.add("TE-21000400");
			ss_code.add("TE-21000204");
		}
		else {
			MakeBillCodeList(rBillIdList, ss_code, sent_id_list);
		}
		const uint ssc_count = ss_code.getCount();
		if(ssc_count) {
			assert(test || ssc_count == sent_id_list.getCount());
			if(test || ssc_count == sent_id_list.getCount()) {
				MakeOrderNumsRequest(ss_code, req_buf);
				if(GetUrl(test, url_buf)) {
					ScURL c;
					url_buf.SetLastDSlash().Cat("Pharm/ShippedOrders");
					InetUrl url(url_buf);
					StrStrAssocArray hdr_flds;
					MakeHeaderFields(Token, &hdr_flds, temp_buf);
					SBuffer ack_buf;
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, reply_buf);
							SJson * p_js_reply = SJson::Parse(reply_buf);
							if(ParseReply(p_js_reply, rRepList)) {
								ok = 1;
							}
							ZDELETE(p_js_reply);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
	
int AptekaRuInterface::ReportBuyOut(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList)
{
	int    ok = -1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString reply_buf;
	if(Token.NotEmpty()) {
		StringSet ss_code;
		PPIDArray sent_id_list;
		if(test) {
		}
		else {
			MakeBillCodeList(rBillIdList, ss_code, sent_id_list);
		}
		const uint ssc_count = ss_code.getCount();
		if(ssc_count) {
			assert(test || ssc_count == sent_id_list.getCount());
			if(test || ssc_count == sent_id_list.getCount()) {
				MakeOrderNumsRequest(ss_code, req_buf);
				if(GetUrl(test, url_buf)) {
					ScURL c;
					url_buf.SetLastDSlash().Cat("Pharm/PaidOrders");
					InetUrl url(url_buf);
					StrStrAssocArray hdr_flds;
					MakeHeaderFields(Token, &hdr_flds, temp_buf);
					SBuffer ack_buf;
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, reply_buf);
							SJson * p_js_reply = SJson::Parse(reply_buf);
							if(ParseReply(p_js_reply, rRepList)) {
								ok = 1;
							}
							ZDELETE(p_js_reply);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
	
int AptekaRuInterface::ReportReturn(bool test, const PPIDArray & rBillIdList, TSCollection <ReplyEntry> & rRepList)
{
	int    ok = -1;
	SString temp_buf;
	SString url_buf;
	SString req_buf;
	SString reply_buf;
	if(Token.NotEmpty()) {
		StringSet ss_code;
		PPIDArray sent_id_list;
		if(test) {
		}
		else {
			MakeBillCodeList(rBillIdList, ss_code, sent_id_list);
		}
		const uint ssc_count = ss_code.getCount();
		if(ssc_count) {
			assert(test || ssc_count == sent_id_list.getCount());
			if(test || ssc_count == sent_id_list.getCount()) {
				MakeOrderNumsRequest(ss_code, req_buf);
				if(GetUrl(test, url_buf)) {
					ScURL c;
					url_buf.SetLastDSlash().Cat("Pharm/ReturnedOrders");
					InetUrl url(url_buf);
					StrStrAssocArray hdr_flds;
					MakeHeaderFields(Token, &hdr_flds, temp_buf);
					SBuffer ack_buf;
					SFile wr_stream(ack_buf.Z(), SFile::mWrite);
					Lth.Log("req", url_buf, req_buf);
					THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose|ScURL::mfTcpKeepAlive, &hdr_flds, req_buf, &wr_stream));
					{
						SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
						if(p_ack_buf) {
							reply_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
							Lth.Log("rep", 0, reply_buf);
							SJson * p_js_reply = SJson::Parse(reply_buf);
							if(ParseReply(p_js_reply, rRepList)) {
								ok = 1;
							}
							ZDELETE(p_js_reply);
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
IMPLEMENT_PPFILT_FACTORY(PrcssrAptekaRu); PrcssrAptekaRuFilt::PrcssrAptekaRuFilt() : PPBaseFilt(PPFILT_PRCSSRAPTEKARU, 0, 0)
{
	SetFlatChunk(offsetof(PrcssrAptekaRuFilt, ReserveStart),
		offsetof(PrcssrAptekaRuFilt, ReserveEnd)-offsetof(PrcssrAptekaRuFilt, ReserveStart)+sizeof(ReserveEnd));
	Init(1, 0);
}

PrcssrAptekaRuFilt & FASTCALL PrcssrAptekaRuFilt::operator = (const PrcssrAptekaRuFilt & rS)
{
	Copy(&rS, 0);
	return *this;
}

PrcssrAptekaRu::PrcssrAptekaRu()
{
}
	
int PrcssrAptekaRu::InitParam(PrcssrAptekaRuFilt * pParam)
{
	CALLPTRMEMB(pParam, Init(1, 0));
	return 1;
}

int PrcssrAptekaRu::Init(const PrcssrAptekaRuFilt * pParam)
{
	if(!RVALUEPTR(P, pParam))
		MEMSZERO(P);
	return 1;
}

int PrcssrAptekaRu::EditParam(PrcssrAptekaRuFilt * pParam)
{
	int    ok = -1;
	PrcssrAptekaRuFilt  data;
	if(pParam)
		data = *pParam;
	TDialog * dlg = new TDialog(DLG_PRCRAPTRU);
	if(CheckDialogPtr(&dlg)) {
		dlg->SetupCalPeriod(CTLCAL_PRCRAPTRU_PERIOD, CTL_PRCRAPTRU_PERIOD);
		SetPeriodInput(dlg, CTL_PRCRAPTRU_PERIOD, data.Period);
		SetupPPObjCombo(dlg, CTLSEL_PRCRAPTRU_GUA, PPOBJ_GLOBALUSERACC, data.GuaID, 0, reinterpret_cast<void *>(PPGLS_APTEKARU));
		SetupLocationCombo(dlg, CTLSEL_PRCRAPTRU_LOC, data.LocID, OLW_CANSELUPLEVEL, LOCTYP_WAREHOUSE, 0);
		{
			PPIDArray op_type_list;
			op_type_list.add(PPOPT_GOODSRECEIPT);
			SetupOprKindCombo(dlg, CTLSEL_PRCRAPTRU_RCPOP, data.RcptOpID, 0, &op_type_list, 0);
		}
		{
			ObjTagFilt ot_filt;
			ot_filt.ObjTypeID = PPOBJ_BILL;
			ot_filt.Flags |= ObjTagFilt::fOnlyTags;
			SetupObjTagCombo(dlg, CTLSEL_PRCRAPTRU_RCPTAG, data.RcptTagID, 0, &ot_filt);
		}
		dlg->AddClusterAssoc(CTL_PRCRAPTRU_FLAGS, 0, PrcssrAptekaRuFilt::fTest);
		dlg->SetClusterData(CTL_PRCRAPTRU_FLAGS, data.Flags);
		while(ok < 0 && ExecView(dlg) == cmOK) {
			uint sel = 0;
			GetPeriodInput(dlg, sel = CTL_PRCRAPTRU_PERIOD, &data.Period);
			dlg->getCtrlData(sel = CTLSEL_PRCRAPTRU_GUA, &data.GuaID);
			if(!data.GuaID) {
				PPSetError(PPERR_GLOBALUSERACCNEEDED);
				PPErrorByDialog(dlg, sel);
			}
			else {
				dlg->getCtrlData(sel = CTLSEL_PRCRAPTRU_LOC, &data.LocID);
				dlg->getCtrlData(sel = CTLSEL_PRCRAPTRU_RCPOP, &data.RcptOpID);
				if(!data.RcptOpID) {
					PPSetError(PPERR_OPRKINDNEEDED);
					PPErrorByDialog(dlg, sel);
				}
				else {
					dlg->GetClusterData(CTL_PRCRAPTRU_FLAGS, &data.Flags);
					dlg->getCtrlData(sel = CTLSEL_PRCRAPTRU_RCPTAG, &data.RcptTagID);
					ok = 1;
				}
			}
		}
	}
	delete dlg;
	ASSIGN_PTR(pParam, data);
	return ok;
}

int PrcssrAptekaRu::Run()
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	const int   unclaimed_order_gap_days = 14; // ������ � ������� �������� ����� ������ ���� ���� ����������� ����������� ���� ������������ ����������.
	const bool is_test = LOGIC(P.Flags & PrcssrAptekaRuFilt::fTest);
	THROW(P.GuaID); // @todo @err
	{
		PPObjArticle ar_obj;
		PPObjLocation loc_obj;
		DateRange nominal_period; // ������, �� ������� �� ������������ ����� �����������
		DateRange ext_period;     // ������, ����������� �� unclaimed_order_gap_days ���� ����� � ����� ����� ����� ������ ���������, ������� ���� ���� ���������, ���� ����������.
		PPIDArray acceptance_bill_list; // ������ ���������� �������, ������������� ������� ext_period
		PPIDArray nominal_acceptance_bill_list; // // ������ ���������� �������, ������������� ������� nominal_period
		PPIDArray buyout_bill_list;
		PPIDArray return_bill_list; // ������ ���������� �������� ����������
		PPIDArray return_by_bill_list; // ������ ���������� �������, � ������� ��������� ��������� �������� ����������
		PPID   suppl_psn_id = 0;
		PPID   suppl_ar_id = 0;
		SString order_code;
		const  PPID suppl_acs_id = GetSupplAccSheet();
		AptekaRuInterface ifc(P.GuaID);
		THROW(ifc.IsValid());
		THROW(suppl_psn_id = ifc.GetGuaPack().Rec.PersonID); // @todo @err
		{
			nominal_period = P.Period;
			nominal_period.Actualize(ZERODATE);
			ext_period = nominal_period;
			if(checkdate(ext_period.low)) {
				ext_period.low = plusdate(ext_period.low, -unclaimed_order_gap_days);
			}
		}
		{
			ObjIdListFilt loc_list;
			if(P.LocID) {
				PPIDArray src_loc_list;
				PPIDArray temp_loc_list;
				src_loc_list.add(P.LocID);
				loc_obj.ResolveWarehouseList(&src_loc_list, temp_loc_list);
				loc_list.Set(&temp_loc_list);
			}
			P.Period.Actualize(ZERODATE);
			ar_obj.P_Tbl->PersonToArticle(suppl_psn_id, suppl_acs_id, &suppl_ar_id);
			THROW(suppl_ar_id);
			{
				BillFilt filt;
				filt.Period = ext_period;
				filt.OpID = P.RcptOpID;
				filt.ObjectID = suppl_ar_id;
				filt.LocList = loc_list;
				if(P.RcptTagID) {
					filt.P_TagF = new TagFilt;
					filt.P_TagF->TagsRestrict.Add(P.RcptTagID, PPConst::P_TagValRestrict_Exist, 1);
				}
				filt.P_TagF->TagsRestrict.Add(AptekaRuInterface::BillIdentTag, PPConst::P_TagValRestrict_Exist, 1);
				filt.Flags |= BillFilt::fNoTempTable;
				PPViewBill v_bill;
				if(v_bill.Init_(&filt)) {
					BillViewItem view_item;
					for(v_bill.InitIteration(PPViewBill::OrdByDefault); v_bill.NextIteration(&view_item) > 0;) {
						if(p_ref->Ot.GetTagStr(PPOBJ_BILL, view_item.ID, AptekaRuInterface::BillIdentTag, order_code) > 0) { // @paranoic (�������� ������� ������ ������������� ���!)
							if(nominal_period.CheckDate(view_item.Dt)) {
								nominal_acceptance_bill_list.add(view_item.ID);
							}
							acceptance_bill_list.add(view_item.ID);
						}
					}
				}
				nominal_acceptance_bill_list.sortAndUndup();
				acceptance_bill_list.sortAndUndup();
			}
			{
				PPObjCSession cs_obj;
				CCheckCore * p_cc = cs_obj.P_Cc;
				for(uint i = 0; i < acceptance_bill_list.getCount(); i++) {
					const PPID bill_id = acceptance_bill_list.get(i);
					{
						//
						// ���� �������� ����� ��� PPTAG_BILL_LINKCCHECKUUID � �� �������� ����� ���� �����
						// ������� "�����" ���, �� ������� �������� �����������.
						//
						S_GUID uuid;
						if(p_ref->Ot.GetTagGuid(PPOBJ_BILL, bill_id, PPTAG_BILL_LINKCCHECKUUID, uuid) > 0 && !!uuid) {
							PPIDArray cc_id_list;
							cs_obj.GetListByUuid(uuid, 90, cc_id_list);
							if(cc_id_list.getCount()) {
								bool suited = false;
								for(uint ccidx = 0; !suited && ccidx < cc_id_list.getCount(); ccidx++) {
									const PPID cc_id = cc_id_list.get(ccidx);
									CCheckTbl::Rec cc_rec;
									if(p_cc->Search(cc_id, &cc_rec) > 0 && nominal_period.CheckDate(cc_rec.Dt))
										suited = true;											
								}
								if(suited)
									buyout_bill_list.add(bill_id);
							}
						}
					}
					{
						BillTbl::Rec ret_bill_rec;
						for(DateIter di; p_bobj->P_Tbl->EnumLinks(bill_id, &di, BLNK_RETURN, &ret_bill_rec) > 0;) {
							if(ret_bill_rec.LinkBillID == bill_id) { // @paranoic (���� �� ��� ��������� �� �����������, �� ������� EnumLinks �� ������� �� ���������� ��������)
								if(nominal_period.CheckDate(ret_bill_rec.Dt)) {
									return_bill_list.add(ret_bill_rec.ID);
									return_by_bill_list.add(bill_id);
									break;
								}
							}
						}
					}
				}
			}
		}
		THROW(ifc.Auth(is_test));
		{
			TSCollection <AptekaRuInterface::ReplyEntry> rep_list;
			ifc.ReportAcceptance(is_test, nominal_acceptance_bill_list, rep_list);
		}
		{
			TSCollection <AptekaRuInterface::ReplyEntry> rep_list;
			ifc.ReportBuyOut(is_test, buyout_bill_list, rep_list);
		}
		{
			TSCollection <AptekaRuInterface::ReplyEntry> rep_list;
			ifc.ReportReturn(is_test, return_by_bill_list, rep_list);
		}
	}
	CATCHZOK
	return ok;
}

int DoAptekaRuInterchange()
{
	int    ok = -1;
	PrcssrAptekaRu prcssr;
	PrcssrAptekaRuFilt param;
	prcssr.InitParam(&param);
	if(prcssr.EditParam(&param) > 0) {
		if(prcssr.Init(&param) && prcssr.Run())
			ok = 1;
		else
			ok = PPErrorZ();
	}
	return ok;
}
