 // GLBSRVS.CPP
 // Copyright (c) E.Sobolev 2020
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
				temp_buf.Cat(getcurdatetime_(), DATF_YMD|DATF_CENTURY, TIMF_HMS|TIMF_MSEC).Space().Cat(pPrefix).CatDiv(':', 2);
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

static SString P_VKUrlBase("https://api.vk.com");
static SString P_VKMethodUrlBase("https://api.vk.com/method");
//static const char * P_VKAppId = "7685698"; // "7402217"; // Идентификатор приложения (аргумент client_id в запросах)

VkInterface::ErrorResponse::ErrorResponse() : Code(0)
{
}

VkInterface::ErrorResponse & VkInterface::ErrorResponse::Z()
{
	Code = 0;
	Message.Z();
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
	EndPoint.Z(); // URL для запросов
	//Token.Z();
	GroupId.Z();
	PageId.Z();
	TxtMsg.Z();
	LinkFilePath.Z();
	return *this;
}

enum VkAccessRightGroup {
	vkacsrgStories   = 0x00000001, // (+1) (1 << 0) group Доступ к историям.
	vkacsrgPhotos    = 0x00000004, // (+4) (1 << 2) group Доступ к фотографиям.
	vkacsrgAppWidget = 0x00000040, // (+64) (1 << 6) group Доступ к виджетам приложений сообществ. Это право можно запросить только с помощью метода Client API showGroupSettingsBox.
	vkacsrgMessages  = 0x00001000, // (+4096) (1 << 12) group Доступ к сообщениям сообщества.
	vkacsrgDocs      = 0x00020000, // (+131072) (1 << 17) group Доступ к документам.
	vkacsrgManage    = 0x00040000, // (+262144) (1 << 18) group Доступ к администрированию сообщества. 
};

enum VkAccessRightUser {
	vkacsruNotify        = 0x00000001, // (+1)         (1 << 0) Пользователь разрешил отправлять ему уведомления (для flash/iframe-приложений).
	vkacsruFriends       = 0x00000002, // (+2)         (1 << 1) Доступ к друзьям.
	vkacsruPhotos        = 0x00000004, // (+4)         (1 << 2) Доступ к фотографиям.
	vkacsruAudio         = 0x00000008, // (+8)         (1 << 3) Доступ к аудиозаписям.
	vkacsruVideo         = 0x00000010, // (+16)        (1 << 4) Доступ к видеозаписям.
	vkacsruStories       = 0x00000040, // (+64)        (1 << 6) Доступ к историям.
	vkacsruPages         = 0x00000080, // (+128)       (1 << 7) Доступ к wiki-страницам.
	vkacsruAddRef        = 0x00000100, // (+256)       (1 << 8) Добавление ссылки на приложение в меню слева.
	vkacsruStatus        = 0x00000400, // (+1024)      (1 << 10) Доступ к статусу пользователя.
	vkacsruNotes         = 0x00000800, // (+2048)      (1 << 11) Доступ к заметкам пользователя.
	vkacsruMessages      = 0x00001000, // (+4096)      (1 << 12) Доступ к расширенным методам работы с сообщениями (только для Standalone-приложений, прошедших модерацию).
	vkacsruWall          = 0x00002000, // (+8192)      (1 << 13) Доступ к обычным и расширенным методам работы со стеной.
	// Данное право доступа по умолчанию недоступно для сайтов (игнорируется при попытке авторизации для приложений с типом «Веб-сайт» или по схеме Authorization Code Flow).
	vkacsruAds           = 0x00008000, // (+32768)     (1 << 15) Доступ к расширенным методам работы с рекламным API. Доступно для авторизации по схеме Implicit Flow или Authorization Code Flow.
	vkacsruOffline       = 0x00010000, // (+65536)     (1 << 16) Доступ к API в любое время (при использовании этой опции параметр expires_in, возвращаемый вместе с access_token, содержит 0 — токен бессрочный). Не применяется в Open API.
	vkacsruDocs          = 0x00020000, // (+131072)    (1 << 17) Доступ к документам.
	vkacsruGroups        = 0x00040000, // (+262144)    (1 << 18) Доступ к группам пользователя.
	vkacsruNotifications = 0x00080000, // (+524288)    (1 << 19) Доступ к оповещениям об ответах пользователю.
	vkacsruStats         = 0x00100000, // (+1048576)   (1 << 20) Доступ к статистике групп и приложений пользователя, администратором которых он является.
	vkacsruEmail         = 0x00400000, // (+4194304)   (1 << 22) Доступ к email пользователя.
	vkacsruMarket        = 0x08000000, // (+134217728) (1 << 27) Доступ к товарам. 
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
	uint req_rights = (vkacsruPhotos | vkacsruWall | vkacsruMarket);
	url_buf.CatChar('?').CatEq("client_id", /*P_VKAppId*/AppIdent).
		CatChar('&').CatEq("display", "page").
		CatChar('&').CatEq("redirect_uri", redirect_uri_buf).
		CatChar('&').CatEq("scope", req_rights/*"134225924"*/). // Права доступа 134225924 = 0x8002004
		CatChar('&').CatEq("response_type", "token").
		CatChar('&').CatEq("v", "5.126"); // @v10.9.5 5.52-->5.126
	//url_buf.SetLastDSlash();
	//SString url("https://oauth.vk.com/authorize?client_id=7402217&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=134225924&response_type=token&v=5.52/");
	ShellExecute(0, _T("open"), SUcSwitch(url_buf), NULL, NULL, SW_SHOWNORMAL);
}

VkInterface::VkInterface() : ProtoVer(5, 107, 0), Lth(PPFILNAM_VKTALK_LOG)
{
	PPVersionInfo vi = DS.GetVersionInfo();
	vi.GetTextAttrib(vi.taiVkAppIdent, AppIdent);
}

SString & VkInterface::AppendParamProtoVer(SString & rBuf) const
{
	return rBuf.Cat("v").CatChar('=').Cat(ProtoVer.GetMajor()).Dot().Cat(ProtoVer.GetMinor());
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
	THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, Ib.CliAccsKey) > 0);
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

//int VkInterface::WallPost(const SString &rToken, const long &rGroupID, const long &rUserId, const uint &rGroupFlag, SString &rImgPath, const SString &rTextMsg, SString &rOutput)
//int VkInterface::WallPost(VkStruct & rVkStruct, SString & rOutput)
int VkInterface::WallPost(const SString & rMessage, const SString & rLinkFilePath, SString & rOutput)
{
	int    ok = 1;
	SString temp_buf;
	SString upload_url;
	SString photo, server, hash, id, owner_id;
	SJson * p_json_doc = 0;
	THROW(Photos_GetWallUploadServer(/*rVkStruct,*/temp_buf));
	ParseUploadServer(temp_buf, upload_url);
	{
		THROW(PhotoToReq(upload_url, /*rVkStruct.LinkFilePath*/rLinkFilePath, temp_buf.Z(), "photo"));
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
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
		}
		ZDELETE(p_json_doc);
	}
	{
		THROW(Photos_SaveWallPhoto(/*rVkStruct,*/photo, server, hash, temp_buf.Z()));
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
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
	}
	if(id.Len() > 0 && owner_id.Len() > 0) {
		photo.Z().Cat("photo").Cat(owner_id).CatChar('_').Cat(id);
		//THROW(Wall_Post(rVkStruct, photo, temp_buf));
		THROW(Wall_Post(rMessage, photo, temp_buf));
	}
	CATCHZOK;
	ZDELETE(p_json_doc);
	return ok;
}

int VkInterface::PutWareToMarket(const MarketWareItem & rItem, MarketWareItem & rResultItem)
{
	//rOutput.Z();
	int    ok = 1;
	SString temp_buf;
	SString upload_url;
	SString photo;
	SString server;
	SString hash;
	SString crop_data;
	SString crop_hash;
	SString photo_id;
	SJson * p_json_doc = 0;
	{
		THROW(Photos_GetMarketUploadServer(/*rVkStruct,*/1, temp_buf));
		THROW(ParseUploadServer(temp_buf, upload_url));
		THROW(PhotoToReq(upload_url, rItem.ImgPath, temp_buf, "file"));
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
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
						THROW(p_obj->P_Child)
						hash = p_obj->P_Child->Text;
					}
					else if(p_obj->Text.IsEqiAscii("crop_data")) {
						THROW(p_obj->P_Child)
						crop_data = p_obj->P_Child->Text.Unescape();
					}
					else if(p_obj->Text.IsEqiAscii("crop_hash")) {
						THROW(p_obj->P_Child)
						crop_hash = p_obj->P_Child->Text;
					}
				}
			}
		}
		ZDELETE(p_json_doc);
	}
	{
		THROW(Photos_SaveMarketPhoto(photo, server, hash, crop_data, crop_hash, temp_buf));
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
		for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("response")) {
						const SJson *p_photo = p_obj->P_Child;
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
	{
		SString name, descr;
		name.Cat(rItem.Name).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
		descr.Cat(rItem.Description).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
		SString url(P_VKMethodUrlBase);
		if(rItem.OuterId > 0) {
			//Market_Edit(rItem.OuterId, rItem.Price, rItem.Name, photo_id, 1100, rItem.Description, temp_buf);
			//int VkInterface::Market_Edit(/*const VkStruct & rVkStruct,*/PPID goodsID, double goodsPrice, 
				//const SString & rGoodsName, const SString & rMainPhotoId, ulong catId, const SString & rDescr, SString & rOutput)  //edit goods from market
			url.SetLastDSlash().Cat("market.edit").CatChar('?').CatEq("item_id", rItem.OuterId).Cat("&");
		}
		else {
			//THROW(Market_Add(rItem.Price, rItem.Name, photo_id, 1100, rItem.Description, temp_buf));
			//int  VkInterface::Market_Add(/*const VkStruct &rVkStruct,*/double goodsPrice, const SString & rGoodsName, 
				//const SString & rMainPhotoId, ulong catId, const SString & rDescr, SString & rOutput)
			url.SetLastDSlash().Cat("market.add").CatChar('?');
		}
		url.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).Cat("&")
			.CatEq("access_token", Ib.CliAccsKey).Cat("&");
			AppendParamProtoVer(url).Cat("&")
			.CatEq("name", name).Cat("&")
			.CatEq("category_id", /*catId*/1100LL).Cat("&")
			.CatEq("price", rItem.Price, MKSFMTD(0, 2, 0)).Cat("&")
			.CatEq("main_photo_id", photo_id).Cat("&")
			.CatEq("description", descr);
		THROW(GetRequest(url, temp_buf, ScURL::mfDontVerifySslPeer));
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
		if(ReadError(p_json_doc, LastErrResp) > 0) {
			ok = 0;
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
										/*
										temp_buf = p_response_item->P_Child->Text.Unescape();
										ObjTagItem new_obj_tag_item;
										new_obj_tag_item.SetStr(ifc.GetOuterWareIdentTagID(), temp_buf);
										if(new_obj_tag_item != stored_obj_tag_item)
											p_ref->Ot.PutTag(PPOBJ_GOODS, goods_rec.ID, &new_obj_tag_item, 0);
										*/
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//rOutput.Z().Cat(temp_buf);
	CATCHZOK
	ZDELETE(p_json_doc);
	return ok;
}

int  VkInterface::Photos_GetWallUploadServer(/*const VkStruct & rVkStruct,*/SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getWallUploadServer").CatChar('?')
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
	AppendParamProtoVer(url); //.CatEq("v", "5.107");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK
	return ok;
}

int  VkInterface::Photos_GetMarketUploadServer(/*const VkStruct &rVkStruct,*/const uint mainPhotoFlag, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getMarketUploadServer").CatChar('?')
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).Cat("&")
		.CatEq("main_photo", mainPhotoFlag).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
	AppendParamProtoVer(url);//.CatEq("v", "5.107");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int VkInterface::ParseUploadServer(SString & rJson, SString & rUploadOut)
{
	int    ok = 1;
	SJson * p_json_doc = 0;
	rUploadOut.Z();
	THROW_SL(json_parse_document(&p_json_doc, rJson.cptr()) == JSON_OK);
	for(const SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					if(p_obj->P_Child) {
						for(const SJson * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
							if(p_response->Text.IsEqiAscii("upload_url")) {
								rUploadOut.Z().Cat(p_response->P_Child->Text.Unescape());
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

int  VkInterface::Photos_SaveMarketPhoto(/*const VkStruct &rVkStruct,*/const SString & rImage, const SString & rServer, 
	const SString & rHash, const SString & rCropData, const SString & rCropHash, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.saveMarketPhoto").CatChar('?')
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
	AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.Cat("&")
		.CatEq("photo", rImage).Cat("&")
		.CatEq("server", rServer).Cat("&")
		.CatEq("hash", rHash).Cat("&")
		.CatEq("crop_data", rCropData).Cat("&")
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
		.CatEq("group_id", /*rVkStruct.GroupId*/Ib.GroupId).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.Cat("&")
		.CatEq("photo", rPhoto).Cat("&")
		.CatEq("server", rServer).Cat("&")
		.CatEq("hash", rHash).Cat("&");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

//int  VkInterface::Wall_Post(const VkStruct & rVkStruct, const SString & rVkPhotoName, SString & rOutput)
int VkInterface::Wall_Post(/*const VkStruct &rVkStruct,*/const SString & rMessage, const SString & rVkPhotoName, SString & rOutput)
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	SString temp_buf(rMessage);
	//temp_buf = rVkStruct.TxtMsg;
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("wall.post?")
		.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.Cat("&")
		.CatEq("attachments", rVkPhotoName).Cat("&")
		.CatEq("message", temp_buf);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

#if 0 // {
int  VkInterface::Market_Add(/*const VkStruct &rVkStruct,*/double goodsPrice, const SString & rGoodsName, 
	const SString & rMainPhotoId, ulong catId, const SString & rDescr, SString & rOutput)
{
	rOutput.Z();
	int ok = 1;
	SString url;
	SString name, descr;
	name.Cat(rGoodsName).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	descr.Cat(rDescr).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.add").CatChar('?')
		.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.Cat("&")
		.CatEq("name", name).Cat("&")
		.CatEq("category_id", catId).Cat("&")
		.CatEq("price", goodsPrice).Cat("&")
		.CatEq("main_photo_id", rMainPhotoId).Cat("&")
		.CatEq("description", descr);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}
#endif // } 0
#if 0 // {
int VkInterface::Market_Edit(/*const VkStruct & rVkStruct,*/PPID goodsID, double goodsPrice, 
	const SString & rGoodsName, const SString & rMainPhotoId, ulong catId, const SString & rDescr, SString & rOutput)  //edit goods from market
{
	rOutput.Z();
	int    ok = 1;
	SString url;
	SString name, descr;
	name.Cat(rGoodsName).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	descr.Cat(rDescr).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.edit").CatChar('?')
		.CatEq("owner_id", SString("-").Cat(/*rVkStruct.GroupId*/Ib.GroupId)).Cat("&")
		.CatEq("item_id", goodsID).Cat("&")
		.CatEq("access_token", /*rVkStruct.Token*/Ib.CliAccsKey).Cat("&");
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/.Cat("&")
		.CatEq("name", name).Cat("&")
		.CatEq("category_id", catId).Cat("&")
		.CatEq("price", goodsPrice).Cat("&")
		.CatEq("main_photo_id", rMainPhotoId).Cat("&")
		.CatEq("description", descr);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
} 
#endif // } 0

int VkInterface::ReadError(const SJson * pJs, ErrorResponse & rErr) const
{
	int    ok = -1;
	const  SJson * p_cur = pJs;
	if(SJson::IsObject(p_cur)) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("error")) {
				ok = 1;
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
	return ok;
}

int VkInterface::Market_Get(LongArray & rList) // get all goods from VK market
{
	rList.clear();
	LastErrResp.Z();

	int    ok = 1;
	SJson * p_json_doc = 0;
	SString json_buf;
	SString url, temp_buf;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.get").CatChar('?')
		.CatEq("owner_id", temp_buf.Z().CatChar('-').Cat(Ib.GroupId)).Cat("&")
		.CatEq("access_token", Ib.CliAccsKey).Cat("&");
		AppendParamProtoVer(url)/*.CatEq("v", "5.107")*/;
	THROW(GetRequest(url, temp_buf.Z(), ScURL::mfDontVerifySslPeer));
	{
		long item_id = 0; 
		THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr()) == JSON_OK);
		if(ReadError(p_json_doc, LastErrResp) > 0) {
			ok = 0;
		}
		else { 
			for(SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
				if(SJson::IsObject(p_cur)) {
					for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
						if(p_obj->Text.IsEqiAscii("response")) {
							for(const SJson * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
								if(p_response->Text.IsEqiAscii("items")) {
									//owner_id = p_response->P_Child->Text.Unescape();
									for(const SJson * p_item = p_response->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										for(const SJson * p_item_child = p_item->P_Child; p_item_child; p_item_child = p_item_child->P_Next) {
											if(p_item_child->Text.IsEqiAscii("id")) {
												item_id = p_item_child->P_Child->Text.Unescape().ToLong();
												rList.add(item_id);
											}
											else if(p_item_child->Text.IsEqiAscii("availability")) {
											}
											else if(p_item_child->Text.IsEqiAscii("category")) {
												// obj
											}
											else if(p_item_child->Text.IsEqiAscii("description")) {
											}
											else if(p_item_child->Text.IsEqiAscii("owner_id")) {
											}
											else if(p_item_child->Text.IsEqiAscii("price")) {
												// obj
											}
											else if(p_item_child->Text.IsEqiAscii("thumb_photo")) {
											}
											else if(p_item_child->Text.IsEqiAscii("title")) {
											}
											else if(p_item_child->Text.IsEqiAscii("date")) {
											}
											else if(p_item_child->Text.IsEqiAscii("cart_quantity")) {
											}
										}
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

int  VkInterface::PhotoToReq(SString & rUrl, const SString & rImgPath, SString & rOutput, const char * rDataName)
{
	rOutput.Z();
	int    ok = 1;
	SString temp_buf;
	InetUrl url(rUrl);
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	ScURL::HttpForm hf;
	SBuffer * p_ack_buf = 0;
	hf.AddContentFile(rImgPath, "multipart/form-data", rDataName);
	Lth.Log("req", rUrl, temp_buf = "multipart");
	c.HttpPost(url, ScURL::mfVerbose|ScURL::mfDontVerifySslPeer, hf, &wr_stream);
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	if(p_ack_buf) {
		rOutput.CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
		Lth.Log("rep", 0, rOutput);
	}
	else
		ok = 0;
	return ok;
}

int VkInterface::GetRequest(const SString & rUrl, SString & rOutput, int mflags)
{
	int    ok = 1;
	SString temp_buf;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	SBuffer * p_ack_buf = 0;
	Lth.Log("req", rUrl, temp_buf.Z());
	THROW(c.HttpGet(rUrl, mflags, &wr_stream));
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	THROW(p_ack_buf);
	rOutput.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
	Lth.Log("rep", 0, rOutput);
	CATCHZOK
	return ok;
}

#if 0 // {
int VkInterface::ParceGoodsItemList(const SString & rJsonStr, LongArray & rList) const
{
	int    ok = 1;
	SJson * p_json_doc = 0;
	long item_id = 0; 
	THROW_SL(json_parse_document(&p_json_doc, rJsonStr.cptr()) == JSON_OK);
	for(SJson * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==SJson::tOBJECT) {
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
				gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, db_value.Z());
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
				gua_pack.TagL.PutItemStr(PPTAG_GUA_ACCESSKEY, rP.ApiKey);
				THROW(gua_obj.PutPacket(&temp_id, &gua_pack, 1));
			}
			else {
				PPID   temp_id = 0;
				gua_pack.Rec.ServiceIdent = service_ident;
				STRNSCPY(gua_pack.Rec.Name, "UDS");
				STRNSCPY(gua_pack.Rec.Symb, "UDS");
				gua_pack.TagL.PutItemStr(PPTAG_GUA_LOGIN, rP.Login);
				gua_pack.TagL.PutItemStr(PPTAG_GUA_ACCESSKEY, rP.ApiKey);
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
		int    IsSettled() const
		{
			return BIN(State & stSettled);
		}
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
					// Комбо-боксы надо перестроить - у нас появились новые объекты, которых не было в списках
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
				gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, db_value.Z());
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
			// Если необходимо - создаем зарезервированные теги.
			// Note: Функция PPObject::MakeReserve вызывается со своей собственной транзакцией
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
					gua_pack.TagL.PutItemStr(PPTAG_GUA_ACCESSKEY, rP.ApiKey);
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
			// нужно установить поле группа
			return 1;
		}
		DECL_DIALOG_GETDTS()
		{
			int    ok = 1;
			uint   sel = 0;
			SString temp_buf;
			//нужно вернуть данные о groupid, ownerid and token
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
							if(left.IsEqual("access_token"))
								Data.ApiKey.Z().Cat(right);
							else if(left.IsEqual("user_id"))
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
						gua_pack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, accs_key);
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
	//SString img_path;
	SString def_img_path;
	ObjLinkFiles lf(PPOBJ_GOODS);
	PPWait(1);
	PPGetFilePath(PPPATH_DD, PPFILNAM_NOIMAGE, def_img_path); // @v10.9.4
	if(ifc.Setup(rParam.GuaID, ifc.sfInitStoreAttributes)) {
		Goods2Tbl::Rec goods_rec; 
		PPGoodsPacket pack;
		ObjTagItem stored_obj_tag_item;
		LongArray ex_outer_goods_id_list;
		const int goods_descr_ext_str_id = oneof5(rParam.DescrExtStrId, GDSEXSTR_A, GDSEXSTR_B, GDSEXSTR_C, GDSEXSTR_D, GDSEXSTR_E) ? rParam.DescrExtStrId : GDSEXSTR_A;
		PPWait(1);
		if(!ifc.Market_Get(ex_outer_goods_id_list)) {
			const VkInterface::ErrorResponse & r_err = ifc.GetLastResponseError();
			if(r_err.Code) {
				CALLPTRMEMB(pLogger, LogMsgCode(mfError, PPERR_VK_RESPONSE, (temp_buf = r_err.Message).Transf(CTRANSF_UTF8_TO_INNER)));
			}
		}
		else {
			for(uint i = 0; i < rSrcList.getCount(); i++) {
				PPObjGoods::ExportToGlbSvcItem & r_src_item = rSrcList.at(i);
				const PPID native_goods_id = r_src_item.GoodsID;
				const PPID native_loc_id = r_src_item.LocID;
				if(goods_obj.Search(native_goods_id, &goods_rec) > 0) {
					SString link_file_path;
					int   link_file_type = 0;
					VkInterface::MarketWareItem market_item(r_src_item);
					VkInterface::MarketWareItem result_market_item;
					market_item.Name = goods_rec.Name;
					p_ref->Ot.GetTag(PPOBJ_GOODS, native_goods_id, /*tag_outer_goods_id*/ifc.GetOuterWareIdentTagID(), &stored_obj_tag_item);
					lf.Load(native_goods_id, 0L);
					lf.At(0, market_item.ImgPath);
					if(market_item.ImgPath.NotEmptyS()) {
						link_file_path = market_item.ImgPath;
						link_file_type = 1;
					}
					else {
						CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_UHTT_GOODSNOIMG, &msg_buf, native_goods_id))); // PPTXT_LOG_UHTT_GOODSNOIMG "Для товара @goods нет изображения"
						market_item.ImgPath = def_img_path; // @v10.9.4
						link_file_type = 1;
						if(!fileExists(market_item.ImgPath)) {
							CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NODEFIMG, &msg_buf, native_goods_id))); // "Изображение по-умолчанию не найдено!"
							continue;
						}
					}
					{
						pack.Rec.ID = native_goods_id; // @trick
						pack.Rec.Flags |= GF_EXTPROP; // @trick
						if(goods_obj.GetValueAddedData(native_goods_id, &pack) > 0) {
							if(pack.GetExtStrData(goods_descr_ext_str_id, temp_buf) > 0)
								market_item.Description = temp_buf;
							else
								market_item.Description = goods_rec.Name; // @v10.9.6
							/* @v10.9.6 else {
								CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_NODESCRFORGOODS, &msg_buf, native_goods_id))); // "Изображение по-умолчанию не найдено!"
								continue;
							}*/
						}
						else {
							market_item.Description = goods_rec.Name; // @v10.9.6
							// @v10.9.6 CALLPTRMEMB(pLogger, Log(PPFormatT(PPTXT_LOG_VK_ADDFIELDSFORGOODSPROBLEM, &msg_buf, native_goods_id)));
							// @v10.9.6 continue;
						}
					}
					{
						PPID   outer_goods_id = 0;
						stored_obj_tag_item.GetInt(&outer_goods_id);
						if(!ex_outer_goods_id_list.lsearch(outer_goods_id))
							outer_goods_id = 0;
						market_item.OuterId = outer_goods_id;
					}
					if(!ifc.PutWareToMarket(market_item, result_market_item)) {
						const VkInterface::ErrorResponse & r_err = ifc.GetLastResponseError();
						if(r_err.Code) {
							CALLPTRMEMB(pLogger, LogMsgCode(mfError, PPERR_VK_RESPONSE, (temp_buf = r_err.Message).Transf(CTRANSF_UTF8_TO_INNER)));
						}						
					}
					else {
						ObjTagItem new_obj_tag_item;
						temp_buf.Z().Cat(result_market_item.OuterId);
						new_obj_tag_item.SetStr(/*tag_outer_goods_id*/ifc.GetOuterWareIdentTagID(), temp_buf);
						if(new_obj_tag_item != stored_obj_tag_item) {
							p_ref->Ot.PutTag(PPOBJ_GOODS, goods_rec.ID, &new_obj_tag_item, 0);
						}
					}
					PPWaitPercent(i, rSrcList.getCount());
				}
			}
		}
	}
	//CATCHZOK
	PPWait(0);
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
		LLAssocArray category_assoc_list; // native_id-->outer_id Ассоциации идентификаторов категорий 
			// для избежания повторных попыток создания отдной и той же категории
		StrAssocArray category_name_list; // native_id-->name
		TSCollection <UdsGameInterface::GoodsItem> ex_goods_item_list;
		{
			//
			// Похоже, запрос товаров в UDS возможен только с явно выбранной категорией. Если нет, то возвращается только верхний уровень.
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
						// Перемещаем элемент из временной коллекции в общую
						ex_goods_item_list.insert(p_ex_item);
						local_list.atPut(j, 0);
					}
				}
			}
		}
		{
			for(uint i = 0; i < rSrcList.getCount(); i++) {
				const PPObjGoods::ExportToGlbSvcItem & r_src_item = rSrcList.at(i);
				const PPID native_goods_id = r_src_item.GoodsID;
				const PPID native_loc_id = r_src_item.LocID;
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
									if(category_name_list.SearchByText(temp_buf, 0, &cp))
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
