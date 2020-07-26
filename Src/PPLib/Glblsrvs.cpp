 //GLBSRVS.CPP
 //Copyright (c) E.Sobolev 2020

#include <pp.h>
#pragma hdrstop

static SString P_VKUrlBase = "https://api.vk.com";
static SString P_VKMethodUrlBase = "https://api.vk.com/method";
static SString P_VKAppId = "7402217";

void PPVkClient::GetVKAccessToken()
{
	SString url = "https://oauth.vk.com/authorize?client_id=7402217&display=page&redirect_uri=https://oauth.vk.com/blank.html&scope=134225924&response_type=token&v=5.52/";
	ShellExecute(0, _T("open"), SUcSwitch(url), NULL, NULL, SW_SHOWNORMAL); // @unicodeproblem
}

//int PPVkClient::WallPost(const SString &rToken, const long &rGroupID, const long &rUserId, const uint &rGroupFlag, SString &rImgPath, const SString &rTextMsg, SString &rOutput)
int PPVkClient::WallPost(VkStruct &rVkStruct, SString &rOutput)
{
	int ok = 1;
	SString temp_buf;
	SString upload_url;
	SString photo, server, hash, id, owner_id;
	json_t * p_json_doc = 0;
	THROW(Photos_GetWallUploadServer(rVkStruct, temp_buf.Z()));
	ParseUploadServer(temp_buf, upload_url);
	THROW(PhotoToReq(upload_url, rVkStruct.LinkFilePath, temp_buf.Z(), "photo"));
	THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
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
	THROW(Photos_SaveWallPhoto(rVkStruct, photo, server, hash, temp_buf.Z()));
	THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					const json_t *p_photo = p_obj->P_Child;
					if(p_photo->P_Child) {
						for(const json_t * p_response = p_photo->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
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
	if(id.Len() > 0 && owner_id.Len() > 0) {
		photo.Z().Cat("photo").Cat(owner_id).Cat("_").Cat(id);
		THROW(Wall_Post(rVkStruct, photo, temp_buf.Z()));
	}
	CATCHZOK;
	ZDELETE(p_json_doc);
	return ok;
}

//int  PPVkClient::AddGoodToMarket(const VkStruct &rVkStruct, const SString &rName, const SString &rDescr, const uint &rCategory, double &rPrice, SString &rOutput)
int  PPVkClient::AddGoodToMarket(const VkStruct &rVkStruct, const Goods2Tbl::Rec &rGoodsRes, const SString &rDescr, const double price, const double oldPrice, const PPID vkMarketItemID, SString &rOutput)
{
	int ok = 1;
	SString temp_buf;
	SString upload_url;
	SString photo, server, hash, crop_data, crop_hash, photo_id;
	json_t * p_json_doc = 0;
	THROW(Photos_GetMarketUploadServer(rVkStruct, 1, temp_buf.Z()));
	THROW(ParseUploadServer(temp_buf, upload_url));
	THROW(PhotoToReq(upload_url, rVkStruct.LinkFilePath, temp_buf.Z(), "file"));
	THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
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
	THROW(Photos_SaveMarketPhoto(rVkStruct, photo, server, hash, crop_data, crop_hash, temp_buf.Z()));
	THROW_SL(json_parse_document(&p_json_doc, temp_buf.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					const json_t *p_photo = p_obj->P_Child;
					if(p_photo->P_Child) {
						for(const json_t * p_response = p_photo->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
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
	if(vkMarketItemID > 0) {
		Market_Edit(rVkStruct, vkMarketItemID, price, rGoodsRes.Name, photo_id, 1100, rDescr, temp_buf.Z());
	}
	else {
		THROW(Market_Add(rVkStruct, price, rGoodsRes.Name, photo_id, 1100, rDescr, temp_buf.Z()));
	}
	rOutput.Z().Cat(temp_buf);
	CATCHZOK;
	ZDELETE(p_json_doc);
	return ok;
}

int  PPVkClient::Photos_GetWallUploadServer(const VkStruct &rVkStruct, SString &rOutput)
{
	int ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getWallUploadServer?")
		.Cat("group_id").Eq().Cat(rVkStruct.GroupId).Cat("&")
		.Cat("access_token").Eq().Cat(rVkStruct.Token).Cat("&")
		.Cat("v=5.107");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int  PPVkClient::Photos_GetMarketUploadServer(const VkStruct &rVkStruct, const uint mainPhotoFlag, SString &rOutput)
{
	int ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.getMarketUploadServer?")
		.Cat("group_id").Eq().Cat(rVkStruct.GroupId).Cat("&")
		.Cat("main_photo").Eq().Cat(mainPhotoFlag).Cat("&")
		.Cat("access_token").Eq().Cat(rVkStruct.Token).Cat("&")
		.Cat("v=5.107");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int PPVkClient::ParseUploadServer(SString &rJson, SString &rUploadOut)
{
	int ok = 1;
	json_t * p_json_doc = 0;
	rUploadOut.Z();
	THROW_SL(json_parse_document(&p_json_doc, rJson.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					if(p_obj->P_Child) {
						for(const json_t * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
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

int  PPVkClient::Photos_SaveMarketPhoto(const VkStruct &rVkStruct, const SString &rPhoto, const SString &rServer, const SString &rHash, const SString &rCropData, const SString &rCropHash, SString &rOutput)
{
	int ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.saveMarketPhoto?")
		.CatEq("group_id", rVkStruct.GroupId).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107").Cat("&")
		.CatEq("photo", rPhoto).Cat("&")
		.CatEq("server", rServer).Cat("&")
		.CatEq("hash", rHash).Cat("&")
		.CatEq("crop_data", rCropData).Cat("&")
		.CatEq("crop_hash", rCropHash);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int PPVkClient::Photos_SaveWallPhoto(const VkStruct &rVkStruct, const SString &rPhoto, const SString &rServer, const SString &rHash, SString &rOutput)
{
	int ok = 1;
	SString url;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("photos.saveWallPhoto?")
		.CatEq("group_id", rVkStruct.GroupId).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107").Cat("&")
		.CatEq("photo", rPhoto).Cat("&")
		.CatEq("server", rServer).Cat("&")
		.CatEq("hash", rHash).Cat("&");
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int  PPVkClient::Wall_Post(const VkStruct &rVkStruct, const SString &rVkPhotoName, SString &rOutput)
{
	int ok = 1;
	SString url;
	SString temp_buf;
	temp_buf = rVkStruct.TxtMsg;
	temp_buf.Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("wall.post?")
		.CatEq("owner_id", SString("-").Cat(rVkStruct.GroupId)).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107").Cat("&")
		.CatEq("attachments", rVkPhotoName).Cat("&")
		.CatEq("message", temp_buf);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int  PPVkClient::Market_Add(const VkStruct &rVkStruct, double goodsPrice, const SString &rGoodsName, const SString &rMainPhotoId, ulong catId, const SString &rDescr, SString &rOutput)
{
	int ok = 1;
	SString url;
	SString name, descr;
	name.Cat(rGoodsName).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	descr.Cat(rDescr).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.add?")
		.CatEq("owner_id", SString("-").Cat(rVkStruct.GroupId)).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107").Cat("&")
		.CatEq("name", name).Cat("&")
		.CatEq("category_id", catId).Cat("&")
		.CatEq("price", goodsPrice).Cat("&")
		.CatEq("main_photo_id", rMainPhotoId).Cat("&")
		.CatEq("description", descr);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
}

int PPVkClient::Market_Edit(const VkStruct &rVkStruct, PPID goods_id, double goodsPrice, const SString &rGoodsName, const SString &rMainPhotoId, ulong catId, const SString &rDescr, SString &rOutput) 
{
	int ok = 1;
	SString url;
	SString name, descr;
	name.Cat(rGoodsName).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	descr.Cat(rDescr).Transf(CTRANSF_INNER_TO_UTF8).ToUrl();
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.edit?")
		.CatEq("owner_id", SString("-").Cat(rVkStruct.GroupId)).Cat("&")
		.CatEq("item_id", goods_id).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107").Cat("&")
		.CatEq("name", name).Cat("&")
		.CatEq("category_id", catId).Cat("&")
		.CatEq("price", goodsPrice).Cat("&")
		.CatEq("main_photo_id", rMainPhotoId).Cat("&")
		.CatEq("description", descr);
	THROW(GetRequest(url, rOutput, ScURL::mfDontVerifySslPeer));
	CATCHZOK;
	return ok;
} //edit goods from market

int PPVkClient::Market_Get(const VkStruct &rVkStruct, SString &rOutput)
{
	int ok = 1;
	SString url, temp_buf;
	url.Z().Cat(P_VKMethodUrlBase).SetLastDSlash().Cat("market.get?")
		.CatEq("owner_id", SString("-").Cat(rVkStruct.GroupId)).Cat("&")
		.CatEq("access_token", rVkStruct.Token).Cat("&")
		.CatEq("v", "5.107");
	THROW(GetRequest(url, temp_buf.Z(), ScURL::mfDontVerifySslPeer));
	rOutput.Z().Cat(temp_buf);



	CATCHZOK;
	return ok;
} // get all goods from VK market

int  PPVkClient::PhotoToReq(SString &rUrl, const SString &rImgPath, SString &rOutput, const char * rDataName)
{
	int ok = 1;
	SString temp_buf;
	InetUrl url(rUrl);
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	ScURL::HttpForm hf;
	SBuffer * p_ack_buf = 0;
	hf.AddContentFile(rImgPath, "multipart/form-data", rDataName);
	c.HttpPost(url, ScURL::mfVerbose|ScURL::mfDontVerifySslPeer, hf, &wr_stream);
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	if(p_ack_buf) {
		temp_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
		rOutput = temp_buf;
	}
	else {
		ok = 0;
	}
	return ok;
}

int PPVkClient::GetRequest(const SString &rUrl, SString &rOutput, int mflags)
{
	int ok = 1;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf.Z(), SFile::mWrite);
	ScURL c;
	SBuffer * p_ack_buf = 0;
	THROW(c.HttpGet(rUrl, mflags, &wr_stream));
	p_ack_buf = static_cast<SBuffer *>(wr_stream);
	THROW(p_ack_buf);
	rOutput.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
	CATCHZOK
	return ok;
}

int PPVkClient::ParceGoodsItemList(const SString & rJsonStr, LongArray & rList) const
{
	int ok = 1;
	json_t * p_json_doc = 0;
	long item_id = 0; 
	THROW_SL(json_parse_document(&p_json_doc, rJsonStr.cptr())==JSON_OK);
	for(json_t * p_cur = p_json_doc; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type==json_t::tOBJECT) {
			for(const json_t * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("response")) {
					for(const json_t * p_response = p_obj->P_Child->P_Child; p_response; p_response = p_response->P_Next) {
						if(p_response->Text.IsEqiAscii("items")) {
							//owner_id = p_response->P_Child->Text.Unescape();
							for(const json_t *p_item = p_response->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
								for(const json_t *p_item_child = p_item->P_Child; p_item_child; p_item_child = p_item_child->P_Next) {
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
