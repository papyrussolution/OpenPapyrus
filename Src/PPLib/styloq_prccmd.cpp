// STYLOQ_PRCCMD.CPP
// Copyright (c) A.Sobolev 2022, 2023
//
#include <pp.h>
#pragma hdrstop

PPStyloQInterchange::MakeInnerGoodsEntryBlock::MakeInnerGoodsEntryBlock()
{
	P_Ep = new PPEgaisProcessor(PPEgaisProcessor::cfUseVerByConfig, 0, 0);
}

PPStyloQInterchange::MakeInnerGoodsEntryBlock::~MakeInnerGoodsEntryBlock()
{
	delete P_Ep;
}

int PPStyloQInterchange::MakeInnerGoodsEntryBlock::Make(PPID goodsID, double cost, double price, double rest, double unitPerPack, InnerGoodsEntry & rItem)
{
	rItem.Z();
	int    ok = -1;
	Goods2Tbl::Rec goods_rec;
	if(GObj.Fetch(goodsID, &goods_rec) > 0) {
		rItem.GoodsID = goodsID;
		GoodsStockExt gse;
		GrpIdList.addnz(goods_rec.ParentID);
		BrandIdList.addnz(goods_rec.BrandID);
		UnitIdList.addnz(goods_rec.UnitID);
		rItem.Rest = rest;
		rItem.Cost = cost;
		rItem.Price = price;
		rItem.UnitPerPack = unitPerPack;
		gse.Z();
		if(GObj.GetStockExt(goods_rec.ID, &gse, 1) > 0 && gse.MinShippmQtty > 0.0) {
			if(gse.GseFlags & GoodsStockExt::fMultMinShipm)
				rItem.OrderQtyMult = gse.MinShippmQtty;
			else
				rItem.OrderMinQty = gse.MinShippmQtty;
		}
		// @v11.5.0 {
		if(goods_rec.GoodsTypeID) {
			PPGoodsType2 gt_rec;
			if(GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0) {
				if(gt_rec.Flags & GTF_GMARKED)
					rItem.ChZnMarkCat = gt_rec.ChZnProdType ? gt_rec.ChZnProdType : GTCHZNPT_UNKN;
			}
		}
		if(P_Ep && P_Ep->IsAlcGoods(goods_rec.ID)) {
			PrcssrAlcReport::GoodsItem agi;
			if(P_Ep->PreprocessGoodsItem(goods_rec.ID, 0, 0, 0, agi) && agi.StatusFlags & agi.stMarkWanted) {
				rItem.Flags |= InnerGoodsEntry::fEgais;
			}
		}
		// } @v11.5.0 
		ok = 1;
	}
	return ok;
}
//
//
//
PPID PPStyloQInterchange::ProcessCommand_IndexingContent(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SString & rResult)
{
	rResult.Z();
	PPID   doc_id = 0;
	SString js_doc_text;
	SBinaryChunk doc_ident;
	SBinaryChunk cli_ident;
	SSecretTagPool doc_pool;
	THROW(pDocument != 0); // @todo @err
	THROW(rCliPack.Pool.Get(SSecretTagPool::tagClientIdent, &cli_ident)); // @todo @err
	THROW(P_T->MakeDocumentStorageIdent(cli_ident, ZEROGUID, doc_ident));
	{
		SBinarySet::DeflateStrategy ds(256);
		THROW_SL(pDocument->ToStr(js_doc_text));
		THROW_SL(doc_pool.Put(SSecretTagPool::tagRawData, js_doc_text, js_doc_text.Len(), &ds));
	}
	THROW(P_T->PutDocument(&doc_id, cli_ident, -1/*input*/, StyloQCore::doctypIndexingContent, doc_ident, doc_pool, 1/*use_ta*/));
	rResult.CatEq("stored-document-id", doc_id).CatDiv(';', 2).CatEq("raw-json-size", js_doc_text.Len()+1).CatDiv(';', 2).CatEq("result-pool-size", doc_pool.GetDataLen());
	CATCH
		doc_id = 0;
	ENDCATCH
	return doc_id;
}

SJson * PPStyloQInterchange::ProcessCommand_GetGoodsInfo(const SBinaryChunk & rOwnIdent, const StyloQCore::StoragePacket & rCliPack, 
	const StyloQCommandList::Item * pGiCmd, PPID goodsID, const char * pGoodsCode)
{
	assert(!pGiCmd || pGiCmd->BaseCmdId == StyloQCommandList::sqbcGoodsInfo);
	SJson * p_js_result = 0;//SJson::CreateObj();
	PPBaseFilt * p_base_filt = 0;
	if(goodsID || !isempty(pGoodsCode)) {
		PPObjBill * p_bobj = BillObj;
		bool   goods_pack_inited = false;
		PPGoodsPacket goods_pack;
		PPObjGoods goods_obj;
		BarcodeTbl::Rec bc_rec;
		Goods2Tbl::Rec goods_rec;
		if(goodsID && goods_obj.GetPacket(goodsID, &goods_pack, 0) > 0) {
			goods_pack_inited = true;
		}
		else if(!isempty(pGoodsCode)) {
			if(goods_obj.SearchByBarcode(pGoodsCode, &bc_rec, &goods_rec, 1/*adoptSearch*/) > 0) {
				if(goods_obj.GetPacket(goods_rec.ID, &goods_pack, 0) > 0) {
					goodsID = goods_rec.ID;
					goods_pack_inited = true;
				}
			}
			else { // Попытаться найти код по статье (если rCliPack ассоциирован с подходящей аналитической статьей)
				;
			}
		}
		if(goods_pack_inited) {
			StyloQGoodsInfoParam * p_gi_param = 0;
			if(pGiCmd) {
				uint   pos = 0;
				const  StyloQGoodsInfoParam pattern_filt;
				SBuffer temp_filt_buf(pGiCmd->Param);
				if(temp_filt_buf.GetAvailableSize()) {
					if(PPView::ReadFiltPtr(temp_filt_buf, &p_base_filt)) {
						if(p_base_filt && p_base_filt->GetSignature() == pattern_filt.GetSignature())
							p_gi_param = static_cast<StyloQGoodsInfoParam *>(p_base_filt);
					}
				}
			}
			PPID   loc_id = 0;
			PPID   posnode_id = 0;
			InnerGoodsEntry goods_entry(goodsID);
			if(p_gi_param) {
				if(p_gi_param->PosNodeID) {
					PPObjCashNode cn_obj;
					PPSyncCashNode scn;
					if(cn_obj.GetSync(p_gi_param->PosNodeID, &scn) > 0) {
						posnode_id = p_gi_param->PosNodeID;
						CPosProcessor cpp(posnode_id, 0, 0, 0, /*pDummy*/0);
						long   ext_rgi_flags = 0;
						if(scn.LocID)
							loc_id = scn.LocID;
						RetailGoodsInfo rgi;
						if(cpp.GetRgi(goodsID, 0.0, ext_rgi_flags, rgi)) {
							goods_entry.ChZnMarkCat = rgi.ChZnMarkCat; // @v11.5.0
							goods_entry.Flags = rgi.Flags; // @v11.5.0
							goods_entry.ManufDtm = rgi.ManufDtm;
							if(!(rgi.Flags & RetailGoodsInfo::fDisabledQuot) && (rgi.Price > 0.0)) {
								goods_entry.Price = rgi.Price;
							}
							if(checkdate(rgi.Expiry))
								goods_entry.Expiry = rgi.Expiry;
						}
					}
				}
				if(!loc_id)
					loc_id = p_gi_param->LocID;
				if(p_gi_param->Flags & StyloQGoodsInfoParam::fShowRest) {
					GoodsRestParam rp;
					rp.CalcMethod = GoodsRestParam::pcmMostRecent;
					rp.GoodsID = goods_pack.Rec.ID;
					rp.Date = ZERODATE;
					rp.LocID = loc_id;
					p_bobj->trfr->GetRest(rp);
					goods_entry.Rest = rp.Total.Rest;
				}
			}
			p_js_result = MakeObjJson_Goods(rOwnIdent, goods_pack, &goods_entry, 0, p_gi_param, 0/*pStat*/);
		}
	}
	ZDELETE(p_base_filt);
	return p_js_result;
}

SJson * PPStyloQInterchange::ProcessCommand_GetBlob(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SBinaryChunk & rBlob)
{
	rBlob.Z();
	int    ok = 1;
	SJson * p_js_result = SJson::CreateObj();
	SString blob_path;
	SString _signature;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("getblob")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("signature")) {
					_signature = p_obj->P_Child->Text.Unescape();
				}
			}
		}
	}
	THROW(_signature.NotEmpty()); // @err
	THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
	{
		assert(blob_path.NotEmpty());
		//SBuffer content_buf; // Бинарное представление BLOB'а
		const size_t nominal_readbuf_size = SMEGABYTE(1);
		STempBuffer rb(nominal_readbuf_size+64/*insurance*/); // Временный буфер для чтения файла порциями
		THROW_SL(rb.IsValid());
		{
			{
				SFileStorage sfs(blob_path);
				int64   fs = 0; // file-size
				SHandle rh; // read-handle
				size_t  actual_size = 0;
				THROW_SL(sfs.IsValid());
				rh = sfs.GetFile(_signature, &fs);
				THROW_SL(rh);
				do {
					THROW_SL(sfs.Read(rh, rb, nominal_readbuf_size, &actual_size));
					THROW_SL(rBlob.Cat(rb, actual_size));
					//THROW_SL(content_buf.Write(rb, actual_size));
				} while(actual_size);
				{
					uint64 rd_bytes = sfs.GetTotalRdSize(rh);
					THROW_SL(rd_bytes);
					//assert(content_buf.GetAvailableSize() == rd_bytes);
					assert(rBlob.Len() == rd_bytes);
				}
			}
			{
				//const size_t preserve_rd_offs = content_buf.GetRdOffs();
				//const size_t _content_size = content_buf.GetAvailableSize();
				//binary256 _hash = SlHash::Sha256(0, content_buf.GetBufC(content_buf.GetRdOffs()), _content_size);
				binary256 _hash = SlHash::Sha256(0, rBlob.PtrC(), rBlob.Len());
				//assert(preserve_rd_offs == content_buf.GetRdOffs());
				{
					SString temp_buf;
					SFileFormat ff;
					const int fir = ff.IdentifyBuffer(rBlob.PtrC(), rBlob.Len());
					if(fir == 2) {
						SFileFormat::GetMime(ff, temp_buf);
						if(temp_buf.NotEmpty())
							p_js_result->InsertString("contenttype", temp_buf);
					}
					p_js_result->InsertInt64("contentsize", static_cast<int64>(rBlob.Len()));
					SlHash::GetAlgorithmSymb(SHASHF_SHA256, temp_buf);
					assert(temp_buf.NotEmpty());
					p_js_result->InsertString("hashalg", temp_buf.Escape());
					temp_buf.Z().EncodeMime64(&_hash, sizeof(_hash));
					p_js_result->InsertString("hash", temp_buf.Escape());
					//rBlob.Put(content_buf.GetBufC(content_buf.GetRdOffs()), _content_size); // @v11.3.8 
					// @v11.3.8 THROW_SL(temp_buf.EncodeMime64(content_buf.GetBufC(content_buf.GetRdOffs()), _content_size));
					// @v11.3.8 p_js_result->InsertString("content", temp_buf.Escape());
				}
			}
		}
	}
	// Все временные буферы разрушены - теперь сформируем результат
	//THROW_SL(js_reply.ToStr(rResult));
	CATCH
		ZDELETE(p_js_result);
	ENDCATCH
	return p_js_result;
}

SJson * PPStyloQInterchange::ProcessCommand_ReqBlobInfoList(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument)
{
	int    ok = 1;
	SJson * p_js_result = 0;
	SString temp_buf;
	Stq_ReqBlobInfoList req_list;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("requestblobinfolist")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("list")) {
					if(p_obj->P_Child && p_obj->P_Child->IsArray()) {
						for(const SJson * p_inr = p_obj->P_Child->P_Child; p_inr; p_inr = p_inr->P_Next) {
							if(p_inr->IsObject()) {
								Stq_ReqBlobInfoEntry * p_entry = req_list.CreateNewItem();
								for(const SJson * p_itm = p_inr->P_Child; p_itm; p_itm = p_itm->P_Next) {
									if(p_itm->Text.IsEqiAscii("signature")) {
										if(p_itm->P_Child)
											(p_entry->Signature = p_itm->P_Child->Text).Unescape();
									}
									else if(p_itm->Text.IsEqiAscii("hashalg")) {
										if(p_itm->P_Child) {
											p_entry->HashAlg = SlHash::IdentifyAlgorithmSymb(p_itm->P_Child->Text.Unescape());
										}
									}
									else if(p_itm->Text.IsEqiAscii("hash")) {
										if(p_itm->P_Child) {
											p_entry->Hash.FromMime64(p_itm->P_Child->Text.Unescape());
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
	{
		p_js_result = SJson::CreateArr();
		if(req_list.getCount()) {
			SString blob_path;
			THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
			{
				SBinaryChunk bc_my_hash;
				SFileStorage sfs(blob_path);
				SBuffer content_buf; // Бинарное представление BLOB'а
				const size_t nominal_readbuf_size = SMEGABYTE(1);
				STempBuffer rb(nominal_readbuf_size+64/*insurance*/); // Временный буфер для чтения файла порциями
				THROW_SL(sfs.IsValid());
				THROW_SL(rb.IsValid());
				for(uint i = 0; i < req_list.getCount(); i++) {
					const Stq_ReqBlobInfoEntry * p_entry = req_list.at(i);
					assert(p_entry);
					if(p_entry) {
						if(p_entry->Signature.NotEmpty()) {
							int64   fs = 0; // file-size
							size_t  actual_size = 0;
							content_buf.Z();
							SHandle rh = sfs.GetFile(p_entry->Signature, &fs); // read-handle
							if(rh) {
								do {
									THROW_SL(sfs.Read(rh, rb, nominal_readbuf_size, &actual_size));
									THROW_SL(content_buf.Write(rb, actual_size));
								} while(actual_size);
								{
									uint64 rd_bytes = sfs.GetTotalRdSize(rh);
									THROW_SL(rd_bytes);
									assert(content_buf.GetAvailableSize() == rd_bytes);
									THROW_SL(SlHash::CalcBufferHash(p_entry->HashAlg, content_buf.GetBufC(content_buf.GetRdOffs()), content_buf.GetAvailableSize(), bc_my_hash));
									if(bc_my_hash == p_entry->Hash) {
										//	
									}
									else {
										SJson * p_js_item = SJson::CreateObj();
										p_js_item->InsertString("signature", (temp_buf = p_entry->Signature).Escape());
										SlHash::GetAlgorithmSymb(p_entry->HashAlg, temp_buf);
										assert(temp_buf.NotEmpty());
										p_js_item->InsertString("hashalg", temp_buf.Escape());
										p_js_item->InsertString("hash", bc_my_hash.Mime64(temp_buf).Escape());
										p_js_result->InsertChild(p_js_item);
									}
								}
								sfs.CloseFile(rh);
							}
							else {
								SJson * p_js_item = SJson::CreateObj();
								p_js_item->InsertString("signature", (temp_buf = p_entry->Signature).Escape());
								p_js_item->InsertBool("missing", true);
								p_js_result->InsertChild(p_js_item);							
							}
						}
					}
				}
			}
		}
	}
	CATCH
		ZDELETE(p_js_result);
	ENDCATCH
	return p_js_result;
}

int PPStyloQInterchange::ProcessCommand_StoreBlob(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, const SBinaryChunk & rBlob, SString & rResult)
{
	//SFileStorage
	// storeblob
	/*
		{
			cmd: storeblob
			time: epoch-secondes
			contenttype: mime-declaration
			contentsize: size-of-content (in raw format - not in encoded version)
			hashalg: hashing-algorithm (sha-256 or other)
			hash: hash-value (base64 encoded)
			signature: text-signature-for-storing-data
			content: content-data (base64 encoded)
		}
	*/
	rResult.Z();
	int    ok = 1;
	long   _time = 0;
	SString blob_path;
	SFileFormat _content_type;
	int64  _content_size = 0;
	int    _hash_alg = 0;
	//SBinaryChunk blob_data;
	SBinaryChunk _hash;
	SString _signature;
	THROW(rBlob.Len()); // @err
	//const char * p_content_b64 = 0;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					THROW(p_obj->P_Child->Text.IsEqiAscii("storeblob")); // @err
				}
				else if(p_obj->Text.IsEqiAscii("time")) {
					_time = p_obj->P_Child->Text.ToLong();
				}
				else if(p_obj->Text.IsEqiAscii("contenttype")) {
					_content_type.IdentifyMime(p_obj->P_Child->Text);
				}
				else if(p_obj->Text.IsEqiAscii("contentsize")) {
					_content_size = p_obj->P_Child->Text.ToInt64();
				}
				else if(p_obj->Text.IsEqiAscii("hashalg")) {
					_hash_alg = SlHash::IdentifyAlgorithmSymb(p_obj->P_Child->Text);
				}
				else if(p_obj->Text.IsEqiAscii("hash")) {
					THROW_SL(_hash.FromMime64(p_obj->P_Child->Text.Unescape()));
				}
				else if(p_obj->Text.IsEqiAscii("signature")) {
					_signature = p_obj->P_Child->Text.Unescape();
				}
				/*else if(p_obj->Text.IsEqiAscii("content")) {
					p_content_b64 = p_obj->P_Child->Text.Unescape();
				}*/
			}
		}
	}
	//THROW(p_content_b64); // @err
	THROW(_signature.NotEmpty()); // @err
	THROW(_hash_alg); // @err
	THROW(_hash.Len()); // @err
	//THROW_SL(blob_data.FromMime64(p_content_b64));
	THROW_SL(SlHash::VerifyBuffer(_hash_alg, _hash.PtrC(), _hash.Len(), rBlob.PtrC(), rBlob.Len()));
	THROW(PPStyloQInterchange::GetBlobStoragePath(blob_path));
	{
		assert(blob_path.NotEmpty());
		SFileStorage sfs(blob_path);
		THROW_SL(sfs.IsValid());
		THROW_SL(sfs.PutFile(_signature, rBlob.PtrC(), rBlob.Len()));
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_Search(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SString & rResult, SString & rDocDeclaration)
{
	rResult.Z();
	int    ok = -1;
	uint   max_result_count = 0;
	SString temp_buf;
	SString plain_query;
	for(const SJson * p_cur = pDocument; p_cur; p_cur = p_cur->P_Next) {
		if(p_cur->Type == SJson::tOBJECT) {								
			for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
				if(p_obj->Text.IsEqiAscii("cmd")) {
					;
				}
				else if(p_obj->Text.IsEqiAscii("plainquery")) {
					if(SJson::IsString(p_obj->P_Child)) {
						(plain_query = p_obj->P_Child->Text).Unescape();
					}
				}
				else if(p_obj->Text.IsEqiAscii("maxresultcount")) {
					if(SJson::IsNumber(p_obj->P_Child))
						max_result_count = p_obj->P_Child->Text.ToULong();
				}
			}
		}
	}
	if(plain_query.NotEmptyS()) {
		PPFtsInterface fts_db(false/*forUpdate*/, 30000);
		TSCollection <PPFtsInterface::SearchResultEntry> result_list;
		THROW(fts_db);
		THROW(fts_db.Search(plain_query, max_result_count, result_list));
		/*if(result_list.getCount())*/ {
			/* json
				searchresult
				query
				scope_list
					{
						scope
						scopeident
						nm
					}
				result_list
					{
						scope
						scopeident
						objtype
						objid
						rank
						weight
						text
					}
			*/
			SJson js_reply(SJson::tOBJECT);
			js_reply.InsertString("query", (temp_buf = plain_query).Escape());
			{
				SJson * p_js_list = SJson::CreateArr();
				SBinaryChunk bc_temp;
				PPFtsInterface::SurrogateScopeList sslist;
				for(uint i = 0; i < result_list.getCount(); i++) {
					const PPFtsInterface::SearchResultEntry * p_ri = result_list.at(i);
					if(p_ri) {
						p_ri->MakeSurrogateScopeIdent(bc_temp);
						if(!sslist.Search(bc_temp, 0)) {
							SJson * p_js_si = SJson::CreateObj();
							temp_buf.Z();
							switch(p_ri->Scope) {
								case PPFtsInterface::scopePPDb: temp_buf = "ppdb"; break;
								case PPFtsInterface::scopeStyloQSvc: temp_buf = "styloqsvc"; break;
								//case PPFtsInterface::scopeUndef: 
								default: temp_buf = "undef"; break;
							}
							p_js_si->InsertString("scope", temp_buf);
							p_js_si->InsertString("scopeident", (temp_buf = p_ri->ScopeIdent).Unescape());
							if(p_ri->Scope == PPFtsInterface::scopeStyloQSvc) {
								if(bc_temp.FromMime64(temp_buf)) {
									StyloQCore::StoragePacket sp;
									StyloQFace face_pack;
									temp_buf.Z();
									if(P_T->SearchGlobalIdentEntry(StyloQCore::kForeignService, bc_temp, &sp) > 0) {
										if(sp.GetFace(SSecretTagPool::tagFace, face_pack) > 0) {
											face_pack.GetRepresentation(0, temp_buf);
										}
									}
									else if(P_T->SearchGlobalIdentEntry(StyloQCore::kNativeService, bc_temp, &sp) > 0) {
										if(sp.GetFace(SSecretTagPool::tagSelfyFace, face_pack) > 0) {
											face_pack.GetRepresentation(0, temp_buf);
										}										
									}
									if(temp_buf.NotEmpty()) {
										p_js_si->InsertString("nm", temp_buf.Escape());
									}
								}
							}
							p_js_list->InsertChild(p_js_si);
							//
							sslist.insert(new SBinaryChunk(bc_temp));
						}
					}
				}
				js_reply.Insert("scope_list", p_js_list);
			}
			{
				SJson * p_js_list = SJson::CreateArr();
				for(uint i = 0; i < result_list.getCount(); i++) {
					const PPFtsInterface::SearchResultEntry * p_ri = result_list.at(i);
					if(p_ri) {
						SJson * p_js_ri = SJson::CreateObj();
						temp_buf.Z();
						switch(p_ri->Scope) {
							case PPFtsInterface::scopePPDb: temp_buf = "ppdb"; break;
							case PPFtsInterface::scopeStyloQSvc: temp_buf = "styloqsvc"; break;
							//case PPFtsInterface::scopeUndef: 
							default: temp_buf = "undef"; break;
						}
						p_js_ri->InsertString("scope", temp_buf);
						p_js_ri->InsertString("scopeident", (temp_buf = p_ri->ScopeIdent).Unescape());
						if(DS.GetObjectTypeSymb(p_ri->ObjType, temp_buf)) {
							p_js_ri->InsertString("objtype", temp_buf);
							p_js_ri->InsertInt64("objid", p_ri->ObjId);
						}
						p_js_ri->InsertInt64("rank", p_ri->Rank);
						p_js_ri->InsertDouble("weight", p_ri->Weight, MKSFMTD(0, 6, NMBF_NOTRAILZ));
						if(p_ri->Text.NotEmpty()) {
							p_js_ri->InsertString("text", (temp_buf = p_ri->Text).Escape());
						}
						p_js_list->InsertChild(p_js_ri);
					}
				}
				js_reply.Insert("result_list", p_js_list);
			}
			js_reply.ToStr(rResult);
			{
				StyloQCommandList::Item fake_cmd_item;
				fake_cmd_item.BaseCmdId = StyloQCommandList::sqbcSearch;
				THROW(MakeDocDeclareJs(fake_cmd_item, 0, rDocDeclaration));
			}
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

SJson * PPStyloQInterchange::ProcessCommand_RequestDocumentStatusList(const SBinaryChunk & rOwnIdent, const StyloQCore::StoragePacket & rCliPack, const SJson * pJsCommand)
{
	SJson * p_js_reply = SJson::CreateObj();
	SJson * p_js_resultlist = SJson::CreateArr();
	/*
		js_doc_item.put("orgcmduuid", local_doc.H.OrgCmdUuid.toString());
		js_doc_item.put("uuid", local_doc.H.Uuid.toString());
		js_doc_item.put("cid", local_doc.H.ID);
		js_doc_item.put("cst", doc_status);
	*/
	SString temp_buf;
	const SJson * p_js_list = 0;
	{
		for(const SJson * p_cur = pJsCommand; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {								
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("cmd")) {
						temp_buf = SJson::Unescape(p_obj->P_Child->Text);
						assert(temp_buf.IsEqiAscii("requestdocumentstatuslist")); // В противном случае мы не должны были попасть в эту функцию вообще
					}
					else if(p_obj->Text.IsEqiAscii("list")) {
						p_js_list = p_obj->P_Child;
					}
				}
			}
		}
	}
	THROW(SJson::IsArray(p_js_list)); // @err
	{
		SString db_symb;
		DbProvider * p_dict = CurDict;
		if(!p_dict || !p_dict->GetDbSymb(db_symb)) {
			PPSetError(PPERR_SESSNAUTH);
			PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
			CALLEXCEPT_PP(PPERR_SQ_CMDSETLOADINGFAULT);
		}
		{
			assert(db_symb.NotEmpty());
			// Декларация объектов должна располагаться после проверки авторизации в базе данных (see above)
			Reference * p_ref = PPRef;
			PPObjBill * p_bobj = BillObj;
			PPObjTSession tses_obj;
			StyloQCommandList full_cmd_list;
			StyloQCommandList::GetFullList(db_symb, full_cmd_list);
			for(const SJson * p_js_item = p_js_list->P_Child; p_js_item; p_js_item = p_js_item->P_Next) {
				if(SJson::IsObject(p_js_item)) {
					S_GUID org_cmd_uuid;
					S_GUID doc_uuid;
					int64 cid = 0;
					int   cst = 0;
					for(const SJson * p_js_f = p_js_item->P_Child; p_js_f; p_js_f = p_js_f->P_Next) {
						if(p_js_f->Text.IsEqiAscii("orgcmduuid"))
							org_cmd_uuid.FromStr(SJson::Unescape(p_js_f->P_Child->Text));
						else if(p_js_f->Text.IsEqiAscii("uuid"))
							doc_uuid.FromStr(SJson::Unescape(p_js_f->P_Child->Text));
						else if(p_js_f->Text.IsEqiAscii("cid"))
							cid = p_js_f->P_Child->Text.ToInt64();
						else if(p_js_f->Text.IsEqiAscii("cst"))
							cst = p_js_f->P_Child->Text.ToLong();
					}
					if(!!doc_uuid) {
						PPID obj_type = PPOBJ_BILL;
						PPID obj_tag_id = PPTAG_BILL_UUID;
						PPIDArray result_obj_id_list;
						const StyloQCommandList::Item * p_cmd_item = !org_cmd_uuid ? 0 : full_cmd_list.GetByUuid(org_cmd_uuid);
						if(p_cmd_item && p_cmd_item->BaseCmdId == StyloQCommandList::sqbcRsrvAttendancePrereq) {
							obj_type = PPOBJ_TSESSION; // Искать надо среди тех сессий
							obj_tag_id = PPTAG_TSESS_UUID;
						}
						assert(oneof2(obj_type, PPOBJ_BILL, PPOBJ_TSESSION)); // В дальнейшем могут быть добавлены другие типы объектов!
						assert(obj_tag_id != 0);
						p_ref->Ot.SearchObjectsByGuid(obj_type, obj_tag_id, doc_uuid, &result_obj_id_list);
						if(result_obj_id_list.getCount() == 1) {
							PPID my_id = result_obj_id_list.get(0);
							SJson * p_js_result_entry = 0;
							if(obj_type == PPOBJ_BILL) {
								BillTbl::Rec bill_rec;
								if(p_bobj->Search(my_id, &bill_rec) > 0) {
									int new_status = 0;
									if(bill_rec.EdiOp == PPEDIOP_ORDER) {
										if(cst == StyloQCore::styloqdocstWAITFORAPPROREXEC) {
											if(bill_rec.Flags2 & BILLF2_DECLINED)
												new_status = StyloQCore::styloqdocstREJECTED;
											else if(p_bobj->CheckStatusFlag(bill_rec.StatusID, BILSTF_READYFOREDIACK))
												new_status = StyloQCore::styloqdocstAPPROVED;
										}
									}
									if(new_status) {
										p_js_result_entry = new SJson(SJson::tOBJECT);
										p_js_result_entry->InsertString("uuid", temp_buf.Z().Cat(doc_uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
										if(cid)
											p_js_result_entry->InsertInt64("cid", cid);
										p_js_result_entry->InsertInt("sid", bill_rec.ID);
										p_js_result_entry->InsertInt("sst", new_status);
									}
								}
							}
							else if(obj_type == PPOBJ_TSESSION) {
								TSessionPacket tses_pack;
								if(tses_obj.GetPacket(my_id, &tses_pack, PPObjTSession::gpoLoadLines) > 0) {
									int new_status = 0;
									if(cst == StyloQCore::styloqdocstWAITFORAPPROREXEC) {
										if(tses_pack.Rec.Status == TSESST_PENDING)
											new_status = StyloQCore::styloqdocstAPPROVED;
										else if(tses_pack.Rec.Status == TSESST_CANCELED)
											new_status = StyloQCore::styloqdocstREJECTED;
									}
									if(new_status) {
										p_js_result_entry = new SJson(SJson::tOBJECT);
										p_js_result_entry->InsertString("uuid", temp_buf.Z().Cat(doc_uuid, S_GUID::fmtIDL|S_GUID::fmtLower));
										if(cid)
											p_js_result_entry->InsertInt64("cid", cid);
										p_js_result_entry->InsertInt("sid", tses_pack.Rec.ID);
										p_js_result_entry->InsertInt("sst", new_status);
									}
								}
							}
							else {
								; // @err
							}
							if(p_js_result_entry) {
								p_js_resultlist->InsertChild(p_js_result_entry);
								p_js_result_entry = 0;
							}
						}
					}
				}
			}
		}
	}
	p_js_reply->InsertString("result", "ok");
	p_js_reply->InsertNz("list", p_js_resultlist);
	CATCH
		ZDELETE(p_js_reply);
	ENDCATCH
	return p_js_reply;
}

int PPStyloQInterchange::ProcessCommand_PersonEvent(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SJson * pJsCmd, const SGeoPosLL & rGeoPos)
{
	int    ok = 1;
	PPID   new_id = 0;
	assert(rCmdItem.BaseCmdId == StyloQCommandList::sqbcPersonEvent);
	{
		SSerializeContext sctx;
		SString temp_buf;
		const LDATETIME _now = getcurdatetime_();
		PPObjPersonEvent psnevobj;
		//PPPsnEventPacket pe_pack;
		StyloQPersonEventParam param;
		THROW(rCmdItem.Param.GetAvailableSize());
		{
			SBuffer temp_non_const_buf(rCmdItem.Param);
			// @v11.6.2 THROW(psnevobj.SerializePacket(-1, &pe_pack, temp_non_const_buf, &sctx));
			THROW(param.Serialize(-1, temp_non_const_buf, &sctx)); // @v11.6.2 
		}
		param.PePack.Rec.Dt = _now.d;
		param.PePack.Rec.Tm = _now.t;
		THROW(param.PePack.Rec.OpID);
		if(param.PePack.Rec.PersonID == ROBJID_CONTEXT) {
			THROW(FetchPersonFromClientPacket(rCliPack, &param.PePack.Rec.PersonID, true/*logResult*/) > 0);
		}
		if(param.PePack.Rec.SecondID == ROBJID_CONTEXT) {
			THROW(FetchPersonFromClientPacket(rCliPack, &param.PePack.Rec.SecondID, true/*logResult*/) > 0);
		}
		// @v11.3.12 {
		if(pJsCmd) {
			const SJson * p_js_memo = pJsCmd->FindChildByKey("memo");
			if(p_js_memo) {
				(temp_buf = p_js_memo->Text).Unescape().Transf(CTRANSF_UTF8_TO_INNER);
				if(temp_buf.NotEmptyS())
					param.PePack.SMemo = temp_buf;
			}
		}
		// } @v11.3.12 
		THROW(psnevobj.PutPacket(&new_id, &param.PePack, 1));
	}
	CATCHZOK
	return ok;
}

SJson * PPStyloQInterchange::ProcessCommand_PostDocument(const SBinaryChunk & rOwnIdent, const StyloQCore::StoragePacket & rCliPack, 
	const SJson * pDeclaration, const SJson * pDocument, PPID * pResultID)
{
	int    result_obj_type = 0;
	PPID   result_obj_id = 0;
	bool   do_store_original_doc = false; // Индикатор необходимости сохранить входящий док в реестре (если проведение "родной" операции было успешным)
	bool   do_return_updated_incoming_list = false; // @v11.6.4 Индикатор необходимости вернуть клиенту обновленный список входящих документов
	SString temp_buf;
	SString db_symb;
	SJson * p_js_reply = 0;
	StyloQCommandList::Item org_cmd_item; // Если org_cmd_item.BaseCmdId == 0, то считаем, что экземпляр не инициализирован
	StyloQDocumentPrereqParam * p_cmd_doc_filt = 0;
	//PPID   result_bill_id = 0;
	//PPID   result_tses_id = 0;
	SBinaryChunk cli_ident;
	Document doc;
	DocumentDeclaration ddecl(0, 0);
	DbProvider * p_dict = CurDict;
	THROW_PP(p_dict && p_dict->GetDbSymb(db_symb), PPERR_SESSNAUTH);
	{
		PPObjGoods gobj;
		Goods2Tbl::Rec goods_rec;
		THROW(rCliPack.Pool.Get(SSecretTagPool::tagClientIdent, &cli_ident)); // @todo @err
		THROW(doc.FromJsonObject(pDocument));
		THROW(doc.Uuid); // @err
		if(doc.InterchangeOpID == 0) {
			THROW(doc.SvcOpID > 0 && GetOpData(doc.SvcOpID, 0) > 0);
		}
		else {
			THROW(oneof3(doc.InterchangeOpID, PPEDIOP_ORDER, StyloQCommandList::sqbdtSvcReq, StyloQCommandList::sqbdtCCheck));
		}
		THROW(checkdate(doc.CreationTime.d) || checkdate(doc.Time.d));
		if(pDeclaration) {
			ddecl.FromJsonObject(pDeclaration);
		}
		if(!!doc.OrgCmdUuid) {
			StyloQCommandList full_cmd_list;
			StyloQCommandList * p_target_cmd_list = 0;
			if(StyloQCommandList::GetFullList(db_symb, full_cmd_list)) {
				const StyloQCommandList::Item * p_cmd = full_cmd_list.GetByUuid(doc.OrgCmdUuid);
				if(p_cmd)
					org_cmd_item = *p_cmd;
			}
		}
		// @v11.6.4 {
		if(oneof2(org_cmd_item.BaseCmdId, StyloQCommandList::sqbcIncomingListCCheck, StyloQCommandList::sqbcIncomingListOrder)) {
			do_return_updated_incoming_list = true;
		}
		// } @v11.6.4 
		if(doc.InterchangeOpID == StyloQCommandList::sqbdtCCheck) { // Кассовые чеки
			PPID   pos_node_id = 0;
			PPObjCashNode cn_obj;
			PPSyncCashNode cn_pack;
			if(doc.PosNodeID > 0 && cn_obj.GetSync(doc.PosNodeID, &cn_pack) > 0) {
				pos_node_id = doc.PosNodeID;
			}
			else if(org_cmd_item.BaseCmdId == StyloQCommandList::sqbcRsrvIndoorSvcPrereq) {
				PPID   temp_pos_node_id = 0;
				org_cmd_item.Param.ReadStatic(&temp_pos_node_id, sizeof(temp_pos_node_id));
				if(temp_pos_node_id && cn_obj.GetSync(temp_pos_node_id, &cn_pack) > 0)
					pos_node_id = temp_pos_node_id;
			}
			if(pos_node_id) {
				THROW(doc.TiList.getCount()); // @todo @err
				{
					{
						for(uint i = 0; i < doc.TiList.getCount(); i++) {
							const Document::__TransferItem * p_item = doc.TiList.at(i);
							THROW(p_item);
							THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
							THROW(p_item->Set.Qtty > 0.0);
						}
					}
					{
						PPID   cc_id = 0;
						CPosProcessor cpp(pos_node_id, 0, 0, 0, 0);
						PPIDArray ex_cc_list;
						CCheckCore & r_cc = cpp.GetCc();
						if(r_cc.GetListByUuid(doc.Uuid, ex_cc_list) > 0) {
							assert(ex_cc_list.getCount());
							if(ex_cc_list.getCount()) {
								ex_cc_list.sort();
								cc_id = ex_cc_list.getLast();
							}
						}
						if(cc_id) {
							CCheckPacket org_pack;
							THROW(cpp.RestoreSuspendedCheck(cc_id, &org_pack, 0));
							for(uint i = 0; i < doc.TiList.getCount(); i++) {
								const Document::__TransferItem * p_item = doc.TiList.at(i);
								if(gobj.Search(p_item->GoodsID, &goods_rec) > 0 && p_item->Set.Qtty > 0.0) {
									const CCheckItem * p_ex_line = cpp.FindLine(p_item->RowIdx);
									if(p_ex_line) {
										CCheckItem line_to_update(*p_ex_line);
										line_to_update.Quantity = p_item->Set.Qtty;
										cpp.UpdateLine(&line_to_update);
									}
									else {
										CPosProcessor::PgsBlock pgsb(p_item->Set.Qtty);
										THROW(cpp.SetupNewRow(goods_rec.ID, /*fabs(CmdBlk.U.AL.Qtty), 0, 0*/pgsb) > 0);
										//THROW(cpp.Backend_SetModifList(CmdBlk.ModifList));
										THROW(cpp.AcceptRow() > 0);
									}
								}
							}
						}
						else {
							for(uint i = 0; i < doc.TiList.getCount(); i++) {
								const Document::__TransferItem * p_item = doc.TiList.at(i);
								if(gobj.Search(p_item->GoodsID, &goods_rec) > 0 && p_item->Set.Qtty > 0.0) {
									CPosProcessor::PgsBlock pgsb(p_item->Set.Qtty);
									THROW(cpp.SetupNewRow(goods_rec.ID, /*fabs(CmdBlk.U.AL.Qtty), 0, 0*/pgsb) > 0);
									//THROW(cpp.Backend_SetModifList(CmdBlk.ModifList));
									THROW(cpp.AcceptRow() > 0);
								}
							}
						}
						cpp.SetupUuid(doc.Uuid);
						THROW(cpp.AcceptCheck(&cc_id, 0, 0, 0, CPosProcessor::accmSuspended) > 0);
						{
							result_obj_type = PPOBJ_CCHECK;
							result_obj_id = cc_id;
							{
								assert(result_obj_id);
								S_GUID _uuid;
								p_js_reply = SJson::CreateObj();
								p_js_reply->InsertInt("result-objtype", result_obj_type);
								p_js_reply->InsertInt("document-id", result_obj_id);
								//PPRef->Ot.GetTagGuid(PPOBJ_BILL, result_obj_id, PPTAG_BILL_UUID, _uuid);
								if(!!_uuid) {
									temp_buf.Z().Cat(_uuid, S_GUID::fmtIDL);
									p_js_reply->InsertString("document-uuid", temp_buf);
								}
								p_js_reply->InsertString("result", "ok");
							}
							do_store_original_doc = true;
						}
					}
				}
			}
		}
		else if(doc.InterchangeOpID == StyloQCommandList::sqbdtSvcReq) { // Технологические сессии
			THROW(doc.BkList.getCount()); // @todo @err
			{
				PPObjTSession tses_obj;
				PPObjArticle ar_obj;
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				PPIDArray tec_candidate_list;
				ProcessorTbl::Rec prc_rec;
				TechTbl::Rec tec_rec;
				TSessionTbl::Rec ex_rec;
				TSessionPacket tses_pack;
				THROW(doc.BkList.getCount() == 1);
				const Document::BookingItem * p_item = doc.BkList.at(0);
				const bool is_ex_tsess = (tses_obj.SearchByGuid(doc.Uuid, &ex_rec) > 0);
				THROW(p_item);
				THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
				THROW(checkdate(p_item->ReqTime.d) && checktime(p_item->ReqTime.t));
				if(p_item->PrcID) {
					THROW(tses_obj.GetPrc(p_item->PrcID, &prc_rec, 1, 0) > 0);
					THROW(tses_obj.GetTechByGoods(p_item->GoodsID, p_item->PrcID, &tec_rec) > 0); // @todo @err
				}
				else {
					// Найти подходящую технологию и любой свободный процессор
					LAssocArray prc_tec_list;
					tec_candidate_list.Z();
					tses_obj.TecObj.GetListByGoods(p_item->GoodsID, &tec_candidate_list);
					THROW(tec_candidate_list.getCount()); // @err no tech for such a ware
					tec_candidate_list.sortAndUndup();
					for(uint j = 0; j < tec_candidate_list.getCount(); j++) {
						const PPID tec_id = tec_candidate_list.get(j);
						if(tses_obj.GetTech(tec_id, &tec_rec) > 0) {
							
						}
					}
				}
				if(is_ex_tsess) {
					THROW(tses_obj.GetPacket(ex_rec.ID, &tses_pack, 0) > 0);
					result_obj_id = ex_rec.ID;
				}
				else {
					tses_obj.InitPacket(&tses_pack, TSESK_SESSION, p_item->PrcID, 0, -1);
				}
				{
					tses_pack.Rec.TechID = tec_rec.ID;
					tses_pack.Rec.StDt = p_item->ReqTime.d;
					tses_pack.Rec.StTm = p_item->ReqTime.t;
					{
						double duration = 0.0;
						if(tec_rec.Capacity > 0.0) {
							duration = 1.0 / tec_rec.Capacity;
						}
						LDATETIME dtm_finish = p_item->ReqTime;
						if(duration == 0.0)
							duration = 3600.0;
						dtm_finish.addsec(R0i(duration));
						if(checkdate(dtm_finish.d) && checktime(dtm_finish.t)) {
							tses_pack.Rec.FinDt = dtm_finish.d;
							tses_pack.Rec.FinTm = dtm_finish.t;
							tses_pack.Rec.PlannedTiming = R0i(duration); // @v11.3.12
						}
						tses_pack.Rec.PlannedQtty = 1.0; // @v11.3.12
					}
					// @v11.3.12 {
					(temp_buf = doc.Memo).Strip().Transf(CTRANSF_UTF8_TO_INNER);
					tses_pack.Ext.PutExtStrData(PRCEXSTR_MEMO, temp_buf);
					// } @v11.3.12
					tses_pack.SetGuid(doc.Uuid);
					{
						PPTransaction tra(1);
						THROW(tra);
						if(prc_rec.WrOffOpID) {
							PPOprKind op_rec;
							if(GetOpData(prc_rec.WrOffOpID, &op_rec) > 0) {
								if(op_rec.AccSheetID && acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0) {
									PPID   person_id = 0;
									if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
										PPID ar_id = 0;
										if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0)
											tses_pack.Rec.ArID = ar_id;
									}
								}
							}
						}
						THROW(tses_obj.PutPacket(&result_obj_id, &tses_pack, 0));
						THROW(tra.Commit());
						result_obj_type = PPOBJ_TSESSION;
						{
							assert(result_obj_id);
							S_GUID _uuid;
							p_js_reply = SJson::CreateObj();
							p_js_reply->InsertInt("result-objtype", result_obj_type);
							p_js_reply->InsertInt("document-id", result_obj_id);
							if(tses_pack.GetGuid(_uuid) && !!_uuid) {
								temp_buf.Z().Cat(_uuid, S_GUID::fmtIDL);
								p_js_reply->InsertString("document-uuid", temp_buf);
							}
							if(p_item->PrcID) {
								long   max_schedule_days = 7; // @todo Здесь должно быть уточненное значение из оригинальной команды, но для этого необходимо
									// получить uuid этой команды от клиента. Требуется доработка протокола.
								SJson * p_js_prc = MakeRsrvAttendancePrereqResponse_Prc(rOwnIdent, p_item->PrcID, 0/*qkID*/, tses_obj, max_schedule_days, 0, 0);
								p_js_reply->InsertNz("prc", p_js_prc);
							}
							p_js_reply->InsertString("result", "ok");
						}
						do_store_original_doc = true;
					}
				}
			}
		}
		else { // Собственно, документы
			const PPID  svc_op_id = doc.SvcOpID;
			PPID   _op_id = 0; // Финальный вид операции создаваемого или модифицируемого документа 
			PPID   loc_id = 0;
			PPID   stylopalm_id = 0;
			PPOprKind op_rec;
			StyloQIncomingListParam incl_param;
			const TSVector <PPStyloQInterchange::Document::CliStatus> * p_cli_status_list = 0;
			if(org_cmd_item.BaseCmdId == StyloQCommandList::sqbcRsrvOrderPrereq) {
				SBuffer param_buf(org_cmd_item.Param);
				p_cmd_doc_filt = StyloQDocumentPrereqParam::Read(param_buf);
				if(p_cmd_doc_filt)
					stylopalm_id = p_cmd_doc_filt->PalmID;
			}
			else if(org_cmd_item.BaseCmdId == StyloQCommandList::sqbcIncomingListOrder) {
				if(org_cmd_item.GetSpecialParam<StyloQIncomingListParam>(incl_param)) {
					p_cli_status_list = &incl_param.StatusList;
				}
			}
			if(svc_op_id) {
				_op_id = svc_op_id;
			}
			else {
				PPIDArray loc_list;
				THROW(doc.InterchangeOpID == PPEDIOP_ORDER); // @todo @err
				if(p_cmd_doc_filt) { // @v11.5.0
					GetContextualOpAndLocForOrder(*p_cmd_doc_filt, &_op_id, &loc_list); // @v11.4.9
					loc_id = loc_list.getSingle(); // @v11.4.9
				}
			}
			const LDATE due_date = checkdate(doc.DueTime.d) ? doc.DueTime.d : ZERODATE;
			const LDATE nominal_doc_date = (p_cmd_doc_filt && p_cmd_doc_filt->Flags & StyloQDocumentPrereqParam::fDlvrDateAsNominal && checkdate(due_date)) ? due_date : 
				(checkdate(doc.Time.d) ? doc.Time.d : (checkdate(doc.CreationTime.d) ? doc.CreationTime.d : getcurdate_()));
			THROW(GetOpData(_op_id, &op_rec) > 0);
			temp_buf.Z().Cat(doc.Code).CatDiv('-', 1).Cat(nominal_doc_date, DATF_DMY);
			THROW_PP_S(doc.TiList.getCount(), PPERR_INCOMINGBILLHASNTTI, temp_buf);
			{
				{
					for(uint i = 0; i < doc.TiList.getCount(); i++) {
						const Document::__TransferItem * p_item = doc.TiList.at(i);
						THROW(p_item);
						THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
						if(p_item->Set.Qtty < 0.0) {
							// Если клиент передал нам отрицательное количество, то мы должны убедиться что это допустимо.
							// Предполагается, что клиент при этом знает вид операции (svc_op_id) который будет использован
							// для создания или модификации документа.
							// Такая ситуация может возникнуть если клиент отправил запрос на изменение документа, которых 
							// до этого сервис переслал клиенту, либо при создании документа клиентом, но в рамках спецификации,
							// определенной сервисом.
							THROW(PPTransferItem::GetSign(svc_op_id, 0) < 0); // @todo @ett
						}
						else {
							THROW(p_item->Set.Qtty > 0.0); // @todo @err
						}
					}
				}
				{
					PPObjBill * p_bobj = BillObj;
					PPObjPerson psn_obj;
					PPObjArticle ar_obj;
					PPBillPacket bpack_; // Пакет нового документа
					PPBillPacket ex_bpack_; // Пакет документа, найденный по UUID
					PPBillPacket * p_bpack = 0; // Указатель на пакет, который буде сохраняться в БД (&bpack || &ex_bpack)
					PPObjAccSheet acs_obj;
					PPAccSheet acs_rec;
					BillTbl::Rec ex_bill_rec;
					bool   is_new_pack = true;
					THROW(p_bobj);
					if(p_bobj->SearchByGuid(doc.Uuid, &ex_bill_rec) > 0/*&& ex_bill_rec.EdiOp == PPEDIOP_ORDER*/) {
						if(p_bobj->ExtractPacket(ex_bill_rec.ID, &ex_bpack_) > 0) {
							/*if(ex_bpack_.BTagL.GetItemStr(PPTAG_BILL_EDICHANNEL, temp_buf) > 0 && temp_buf.IsEqiAscii("STYLOQ"))*/ {
								p_bpack = &ex_bpack_;
								is_new_pack = false;
							}
						}
					}
					{
						PPTransaction tra(1);
						THROW(tra);
						if(!p_bpack) {
							THROW(bpack_.CreateBlank_WithoutCode(_op_id, 0, loc_id, 0));
							STRNSCPY(bpack_.Rec.Code, doc.Code);
							bpack_.SetupEdiAttributes(PPEDIOP_ORDER, "STYLOQ", 0); // @!
							bpack_.SetGuid(doc.Uuid);
							// @v11.4.8 {
							if(checkdate(doc.CreationTime.d)) {
								ObjTagItem tag_item;
								tag_item.SetTimestamp(PPTAG_BILL_CREATEDTM, doc.CreationTime);
								bpack_.BTagL.PutItem(PPTAG_BILL_CREATEDTM, &tag_item);
							}
							// } @v11.4.8 
							// @v11.6.2 {
							if(doc.CreationGeoLoc.IsValid() && !doc.CreationGeoLoc.IsZero()) {
								bpack_.BTagL.PutItemStr(PPTAG_BILL_GPSCOORD, temp_buf.Z().Cat(doc.CreationGeoLoc.Lat).CatChar(',').Cat(doc.CreationGeoLoc.Lon));
							}
							// } @v11.6.2 
							p_bpack = &bpack_;
						}
						else {
							_op_id = p_bpack->Rec.OpID;
							THROW(!svc_op_id || _op_id == svc_op_id); // @todo @err
							THROW(GetOpData(_op_id, &op_rec) > 0);
						}
						assert(op_rec.ID > 0);
						assert(p_bpack);
						assert(p_bpack->Rec.OpID == _op_id);
						if(is_new_pack || !ddecl.ActionFlags)
							p_bpack->Rec.Dt = nominal_doc_date;
						if(is_new_pack || !ddecl.ActionFlags)
							p_bpack->Rec.DueDate = due_date;
						if(is_new_pack || !ddecl.ActionFlags || (ddecl.ActionFlags & (StyloQIncomingListParam::actionDocAcceptance|StyloQIncomingListParam::actionDocStatus)))
							(p_bpack->SMemo = doc.Memo).Strip().Transf(CTRANSF_UTF8_TO_INNER);
						// @v11.5.1 {
						if(is_new_pack || ddecl.ActionFlags & StyloQIncomingListParam::actionDocStatus) {
							if(doc.StatusSurrId > 0 && p_cli_status_list) {
								Document::CliStatus::SetToBill(*p_cli_status_list, doc.StatusSurrId, *p_bpack);
							}
						}
						// } @v11.5.1 
						{
							const PPID agent_acs_id = GetAgentAccSheet();
							PPID  person_id = 0;
							ArticleTbl::Rec ar_rec;
							if(is_new_pack || !ddecl.ActionFlags) {
								if(agent_acs_id) {
									if(doc.AgentID > 0 && ar_obj.Search(doc.AgentID, &ar_rec) > 0 && ar_rec.AccSheetID == agent_acs_id) { // @v11.4.8
										p_bpack->Ext.AgentID = doc.AgentID;
									}
									else if(acs_obj.Fetch(agent_acs_id, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
										if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
											PPID ar_id = 0;
											if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0)
												p_bpack->Ext.AgentID = ar_id;
										}
									}
								}
							}
							if(is_new_pack || !ddecl.ActionFlags) {
								if(doc.ClientID) {
									THROW(ar_obj.Search(doc.ClientID, &ar_rec) > 0); // @err
									THROW(ar_rec.AccSheetID == op_rec.AccSheetID); // @err
									p_bpack->Rec.Object = ar_rec.ID;
									if(doc.DlvrLocID) {
										PPID cli_psn_id = ObjectToPerson(ar_rec.ID, 0);
										if(cli_psn_id) {
											PPIDArray dlvr_loc_list;
											psn_obj.GetDlvrLocList(cli_psn_id, &dlvr_loc_list);
											if(dlvr_loc_list.lsearch(doc.DlvrLocID))
												p_bpack->SetFreight_DlvrAddrOnly(doc.DlvrLocID);
										}
									}
								}
								else if(op_rec.AccSheetID && acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
									if(AcceptStyloQClientAsPerson(rCliPack, acs_rec.ObjGroup, &person_id, 0) > 0) {
										PPID ar_id = 0;
										if(ar_obj.P_Tbl->PersonToArticle(person_id, acs_rec.ID, &ar_id) > 0)
											p_bpack->Rec.Object = ar_id;
									}
								}
							}
						}
						// @v11.4.8 {
						if(!is_new_pack) {
							if(!ddecl.ActionFlags) {
								LongArray ti_idx_list_to_remove;
								for(uint j = 0; j < p_bpack->GetTCount(); j++) {
									const PPTransferItem & r_ti = p_bpack->ConstTI(j);
									bool   ti_found = false;
									for(uint i = 0; !ti_found && i < doc.TiList.getCount(); i++) {
										const Document::__TransferItem * p_item = doc.TiList.at(i);
										if(p_item->RowIdx > 0 && r_ti.RByBill == p_item->RowIdx)
											ti_found = true;
									}
									if(!ti_found)
										ti_idx_list_to_remove.add(j+1);
								}
								if(ti_idx_list_to_remove.getCount()) {
									ti_idx_list_to_remove.sortAndUndup();
									uint i = ti_idx_list_to_remove.getCount();
									do {
										uint ti_idx = ti_idx_list_to_remove.get(--i);
										assert(ti_idx > 0 && ti_idx <= p_bpack->GetTCount()); // @paranoic
										if(ti_idx > 0 && ti_idx <= p_bpack->GetTCount()) { // @paranoic
											p_bpack->RemoveRow(ti_idx-1);
										}
									} while(i);
								}
							}
						}
						// } @v11.4.8 
						{
							PPLotExtCodeContainer::MarkSet ms;
							for(uint i = 0; i < doc.TiList.getCount(); i++) {
								const Document::__TransferItem * p_item = doc.TiList.at(i);
								uint   ex_row_pos = 0;
								THROW(p_item);
								THROW(gobj.Search(p_item->GoodsID, &goods_rec) > 0);
								// Выше мы уже проверили количество THROW(p_item->Set.Qtty > 0.0);
								if(p_item->RowIdx > 0 && p_bpack->SearchTI(p_item->RowIdx, &ex_row_pos)) {
									PPTransferItem & r_ti = p_bpack->TI(ex_row_pos);
									if(is_new_pack || !ddecl.ActionFlags) {
										if(p_item->GoodsID != r_ti.GoodsID)
											r_ti.SetupGoods(p_item->GoodsID);
										r_ti.Cost  = p_item->Set.Cost;
										r_ti.Price = p_item->Set.Price;
										r_ti.Discount = p_item->Set.Discount;
										r_ti.Quantity_ = fabs(p_item->Set.Qtty);
										r_ti.SetupSign(p_bpack->Rec.OpID);
									}
									if(is_new_pack || !ddecl.ActionFlags || (ddecl.ActionFlags & StyloQIncomingListParam::actionDocSettingMarks)) {
										if(p_item->XcL.getCount()) {
											p_bpack->XcL.Get(i+1, 0, ms);
											for(uint xci = 0; xci < p_item->XcL.getCount(); xci++) {
												Document::LotExtCode & r_lec = p_item->XcL.at(xci);
												if(!isempty(r_lec.Code))
													ms.AddNum(0, r_lec.Code, 1);
											}
											if(ms.GetCount())
												p_bpack->XcL.Set_2(i+1, &ms);
										}
									}
								}
								else {
									if(is_new_pack || !ddecl.ActionFlags) {
										PPTransferItem ti(&p_bpack->Rec, TISIGN_UNDEF);
										if(p_item->RowIdx >= 0) {
											ti.RByBill = p_item->RowIdx;
											ti.TFlags |= PPTransferItem::tfForceNew;
										}
										ti.SetupGoods(p_item->GoodsID);
										ti.Cost  = p_item->Set.Cost;
										ti.Price = p_item->Set.Price;
										ti.Discount = p_item->Set.Discount;
										ti.Quantity_ = fabs(p_item->Set.Qtty);
										ti.SetupSign(p_bpack->Rec.OpID);
										const uint new_item_idx = p_bpack->GetTCount();
										THROW(p_bpack->LoadTItem(&ti, 0, 0));
										assert(p_bpack->GetTCount() == new_item_idx+1);
										if(p_item->XcL.getCount()) {
											ms.Z();
											for(uint xci = 0; xci < p_item->XcL.getCount(); xci++) {
												Document::LotExtCode & r_lec = p_item->XcL.at(xci);
												if(!isempty(r_lec.Code))
													ms.AddNum(0, r_lec.Code, 1);
											}
											if(ms.GetCount())
												p_bpack->XcL.Set_2(new_item_idx+1, &ms);
										}
									}
								}
							}
							if(is_new_pack || !ddecl.ActionFlags || (ddecl.ActionFlags & StyloQIncomingListParam::actionDocAcceptanceMarks)) {
								if(doc.VXcL.getCount()) {
									p_bpack->_VXcL.Get(-1, 0, ms);
									for(uint xci = 0; xci < doc.VXcL.getCount(); xci++) {
										Document::LotExtCode & r_lec = doc.VXcL.at(xci);
										if(!isempty(r_lec.Code)) {
											ms.AddNum(0, r_lec.Code, 1);
										}
									}
									p_bpack->_VXcL.AddValidation(ms);
								}
							}
							if(p_bpack->Rec.ID) {
								THROW(p_bpack->InitAmounts(0));
								THROW(p_bobj->FillTurnList(p_bpack));
								THROW(p_bobj->UpdatePacket(p_bpack, 0));
							}
							else {
								THROW(p_bobj->__TurnPacket(p_bpack, 0, 1, 0));
							}
							THROW(tra.Commit());
							result_obj_type = PPOBJ_BILL;
							result_obj_id = p_bpack->Rec.ID;
							{
								assert(result_obj_id);
								S_GUID _uuid;
								p_js_reply = SJson::CreateObj();
								p_js_reply->InsertInt("result-objtype", result_obj_type);
								p_js_reply->InsertInt("document-id", result_obj_id);
								PPRef->Ot.GetTagGuid(PPOBJ_BILL, result_obj_id, PPTAG_BILL_UUID, _uuid);
								if(!!_uuid) {
									temp_buf.Z().Cat(_uuid, S_GUID::fmtIDL);
									p_js_reply->InsertString("document-uuid", temp_buf);
								}
								// @v11.5.1 {
								{
									//
									// Возвращаем клиенту новый документ, чтобы он мог обновить его в своем хранилище
									//
									PPBillPacket new_bpack;
									if(p_bobj->ExtractPacket(result_obj_id, &new_bpack) > 0) {
										PPStyloQInterchange::Document new_doc;
										if(new_doc.FromBillPacket(new_bpack, p_cli_status_list, 0)) {
											SJson * p_js_doc = new_doc.ToJsonObject();
											THROW(p_js_doc);
											p_js_reply->Insert("doc", p_js_doc);
											p_js_doc = 0;
										}
									}
								}
								// } @v11.5.1 
								p_js_reply->InsertString("result", "ok");
							}
							do_store_original_doc = true;
						}
					}
				}
			}
		}
		if(do_store_original_doc) {
			//PPID PPStyloQInterchange::ProcessCommand_IndexingContent(const StyloQCore::StoragePacket & rCliPack, const SJson * pDocument, SString & rResult)
			//rResult.Z();
			assert(p_js_reply != 0); // Иначе do_store_original_doc должен был быть false
			PPID   inner_doc_id = 0;
			SBinaryChunk doc_ident;
			SSecretTagPool doc_pool;
			THROW(P_T->MakeDocumentStorageIdent(cli_ident, doc.Uuid, doc_ident));
			{
				SBinarySet::DeflateStrategy ds(256);
				THROW_SL(pDocument->ToStr(temp_buf));
				THROW_SL(doc_pool.Put(SSecretTagPool::tagRawData, temp_buf, temp_buf.Len(), &ds));
				if(pDeclaration) {
					THROW_SL(pDeclaration->ToStr(temp_buf));
					THROW_SL(doc_pool.Put(SSecretTagPool::tagDocDeclaration, temp_buf, temp_buf.Len(), &ds));
				}
			}
			THROW(P_T->PutDocument(&inner_doc_id, cli_ident, -1/*input*/, StyloQCore::doctypGeneric, doc_ident, doc_pool, 1/*use_ta*/));
			//rResult.CatEq("stored-document-id", doc_id).CatDiv(';', 2).CatEq("raw-json-size", js_doc_text.Len()+1).CatDiv(';', 2).CatEq("result-pool-size", doc_pool.GetDataLen());
		}
		// @v11.6.4 {
		if(do_return_updated_incoming_list) {
			if(org_cmd_item.BaseCmdId == StyloQCommandList::sqbcIncomingListCCheck) {
				
			}
			else if(org_cmd_item.BaseCmdId == StyloQCommandList::sqbcIncomingListOrder) {
			}
		}
		// } @v11.6.4 
	}
	CATCH
		result_obj_type = 0;
		result_obj_id = 0;
		ZDELETE(p_js_reply);
	ENDCATCH
	delete p_cmd_doc_filt;
	ASSIGN_PTR(pResultID, result_obj_id);
	return p_js_reply;
}

PPStyloQInterchange::MakePrcJsListParam::MakePrcJsListParam(const SBinaryChunk & rBcOwnIdent) : R_BcOwnIdent(rBcOwnIdent), QuotKindID(0), LocID(0), MaxScheduleDays(0)
{
}

PPStyloQInterchange::MakePrcJsListParam::MakePrcJsListParam(const SBinaryChunk & rBcOwnIdent, const StyloQAttendancePrereqParam & rOuterParam) : 
	R_BcOwnIdent(rBcOwnIdent), QuotKindID(rOuterParam.QuotKindID), LocID(rOuterParam.LocID), MaxScheduleDays(inrangeordefault(rOuterParam.MaxScheduleDays, 1L, 365L, 7L))
{
}

PPStyloQInterchange::MakePrcJsListParam::MakePrcJsListParam(const SBinaryChunk & rBcOwnIdent, const StyloQIncomingListParam & rOuterParam) :
	R_BcOwnIdent(rBcOwnIdent), QuotKindID(0), LocID(0), MaxScheduleDays(0)
{
	if(rOuterParam.P_TsF) {
		QuotKindID = rOuterParam.P_TsF->QuotKindID;
		if(rOuterParam.P_TsF->PrcID) {
			PPObjProcessor prc_obj;
			ProcessorTbl::Rec prc_rec;
			if(prc_obj.Fetch(rOuterParam.P_TsF->PrcID, &prc_rec) > 0) {
				LocID = prc_rec.LocID;
			}
		}
	}
	MaxScheduleDays = 7L; // @default @todo
}

int PPStyloQInterchange::MakePrcJsList(const MakePrcJsListParam & rParam, SJson * pJsObj, const PPIDArray & rPrcList, PPObjTSession & rTSesObj, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	int    ok = 1;
	assert(pJsObj != 0);
	THROW(pJsObj);
	if(rPrcList.getCount()) {
		//prc_id_list.sortAndUndup();
		LAssocArray goods_to_prc_list;
		PPObjStaffCal scal_obj;
		PrcBusyArray busy_list;
		SJson * p_js_prc_list = SJson::CreateArr();
		for(uint prcidx = 0; prcidx < rPrcList.getCount(); prcidx++) {
			const PPID prc_id = rPrcList.get(prcidx);
			SJson * p_js_prc = MakeRsrvAttendancePrereqResponse_Prc(rParam.R_BcOwnIdent, prc_id, rParam.QuotKindID, rTSesObj, rParam.MaxScheduleDays, &goods_to_prc_list, pStat);
			if(p_js_prc)
				p_js_prc_list->InsertChild(p_js_prc);
		}
		pJsObj->Insert("processor_list", p_js_prc_list);
		{
			PPIDArray goods_id_list;
			PPIDArray goodsgrp_id_list;
			PPIDArray uom_id_list;
			Goods2Tbl::Rec goodsgroup_rec;
			PPObjUnit unit_obj;
			PPUnit u_rec;
			goods_to_prc_list.SortByKeyVal();
			goods_to_prc_list.GetKeyList(goods_id_list);
			{
				Goods2Tbl::Rec goods_rec;
				for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
					const PPID goods_id = goods_id_list.get(gidx);
					if(rTSesObj.GObj.Search(goods_id, &goods_rec) > 0) {
						if(goods_rec.ParentID > 0 && rTSesObj.GObj.Fetch(goods_rec.ParentID, &goodsgroup_rec) > 0 && goodsgroup_rec.Kind == PPGDSK_GROUP) {
							goodsgrp_id_list.add(goods_rec.ParentID);
						}
						if(goods_rec.UnitID > 0 && unit_obj.Fetch(goods_rec.UnitID, &u_rec) > 0) {
							uom_id_list.add(goods_rec.UnitID);
						}
					}
				}
				// (done by MakeObjArrayJson_GoodsGroup) goodsgrp_id_list.sortAndUndup();
				// (done by MakeObjArrayJson_Uom) uom_id_list.sortAndUndup();
			}
			pJsObj->InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(rParam.R_BcOwnIdent, goodsgrp_id_list, false, pStat));
			pJsObj->InsertNz("uom_list", MakeObjArrayJson_Uom(rParam.R_BcOwnIdent, uom_id_list, pStat));
			{
				SJson * p_js_goodslist = SJson::CreateArr();
				PPGoodsPacket goods_pack;
				PPIDArray qk_list;
				qk_list.addnz(rParam.QuotKindID);
				qk_list.add(PPQUOTK_BASE);
				for(uint gidx = 0; gidx < goods_id_list.getCount(); gidx++) {
					const PPID goods_id = goods_id_list.get(gidx);
					if(rTSesObj.GObj.GetPacket(goods_id, &goods_pack, 0) > 0) {
						SJson * p_js_ware = MakeObjJson_Goods(rParam.R_BcOwnIdent, goods_pack, 0, 0, 0, pStat);
						THROW(p_js_ware);
						{
							// "price"
							double price = 0.0;
							for(uint qkidx = 0; price == 0.0 && qkidx < qk_list.getCount(); qkidx++) {
								const PPID qk_id = qk_list.get(qkidx);
								assert(qk_id != 0);
								const PPID ar_id = 0L;
								const QuotIdent qi(rParam.LocID, qk_id, 0/*curID*/, ar_id);
								double result = 0.0;
								if(rTSesObj.GObj.GetQuotExt(goods_id, qi, 0.0, 0.0, &result, 1) > 0) {
									if(result > 0.0)
										price = result;
								}
							}
							if(price > 0.0) {
								p_js_ware->InsertDouble("price", price, MKSFMTD(0, 2, 0));
							}
						}
						p_js_goodslist->InsertChild(p_js_ware);
					}
				}
				pJsObj->Insert("goods_list", p_js_goodslist);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_RsrvAttendancePrereq(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos,
	SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	/*
		param

		quotkind_list
		warehouse_list
		goodsgroup_list [ { id; nm } ]
		goods_list
			ware
		processor_list [ { id; nm; goods_list [ {id} ] } ]
	*/
	SString temp_buf;
	PPObjTSession tses_obj;
	PPObjUnit unit_obj;
	PPObjPerson psn_obj;
	StyloQAttendancePrereqParam param;
	Stq_CmdStat_MakeRsrv_Response stat; // @v11.3.8
	StyloQBlobInfo blob_info; // @v11.3.8
	SJson js(SJson::tOBJECT);
	PPIDArray goods_id_list; // @reusable
	PPIDArray prc_id_list;
	LAssocArray goods_to_prc_list;
	long   max_schedule_days = 7;
	long   time_sheet_discreteness = 15;
	PPUserFuncProfiler ufp(PPUPRF_STYLOQ_CMD_ATTENDANCEPREREQ);
	// @v11.6.2 {
	if(!!rCmdItem.Uuid)
		js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
	js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
	// } @v11.6.2 
	StqInsertIntoJs_BaseCurrency(&js); // @v11.3.12
	{
		//if(rCmdItem.GetAttendanceParam(param) > 0) {
		if(rCmdItem.GetSpecialParam<StyloQAttendancePrereqParam>(param) > 0) {
			max_schedule_days = inrangeordefault(param.MaxScheduleDays, 1L, 365L, 7L);
			SJson * p_js_param = SJson::CreateObj();
			if(param.PrcTitle.NotEmpty())
				p_js_param->InsertString("prctitle", (temp_buf = param.PrcTitle).Transf(CTRANSF_INNER_TO_UTF8).Escape());
			p_js_param->InsertInt("MaxScheduleDays", max_schedule_days); // @v11.3.10
			// @v11.4.3 {
			if(oneof3(param.TimeSheetDiscreteness, 5, 10, 15))
				time_sheet_discreteness = param.TimeSheetDiscreteness;
			else 
				time_sheet_discreteness = 5;
			p_js_param->InsertInt("TimeSheetDiscreteness", time_sheet_discreteness); 
			// } @v11.4.3 
			js.Insert("param", p_js_param);
		}
	}
	{
		ProcessorTbl::Rec prc_rec;
		for(SEnum en = tses_obj.PrcObj.P_Tbl->Enum(PPPRCK_PROCESSOR, 0); en.Next(&prc_rec) > 0;) {
			prc_id_list.add(prc_rec.ID);
		}
	}
	if(prc_id_list.getCount()) {
		SBinaryChunk bc_own_ident;
		THROW(GetOwnIdent(bc_own_ident, 0));
		MakePrcJsListParam mpjsl_param(bc_own_ident, param);
		prc_id_list.sortAndUndup();
		THROW(MakePrcJsList(mpjsl_param, &js, prc_id_list, tses_obj, &stat));
	}
	THROW(js.ToStr(rResult));
	// @debug {
	if(prccmdFlags & prccmdfDebugOutput) {
		SString out_file_name;
		PPGetFilePath(PPPATH_OUT, "stq-rsrvattendanceprereq.json", out_file_name);
		SFile f_out(out_file_name, SFile::mWrite);
		f_out.WriteLine(rResult);
	}
	// } @debug
	THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
	StoreOidListWithBlob(stat.BlobOidList.SortAndUndup());
	ufp.SetFactor(0, static_cast<double>(stat.GoodsCount));
	ufp.SetFactor(1, static_cast<double>(stat.PrcCount));
	ufp.Commit();
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_RsrvIndoorSvcPrereq(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, 
	SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	PPObjIDArray bloboid_list;
	Stq_CmdStat_MakeRsrv_Response stat;
	PPUserFuncProfiler ufp(PPUPRF_STYLOQ_CMD_INDOORSVCPREREQ);
	PPID   posnode_id = 0;
	SString temp_buf;
	PPObjCashNode cn_obj;
	PPCashNode cn_rec;
	PPSyncCashNode cn_sync_pack;
	SBinaryChunk bc_own_ident;
	PPID   agent_psn_id = 0;
	THROW(GetOwnIdent(bc_own_ident, 0));
	THROW(rCmdItem.Param.ReadStatic(&posnode_id, sizeof(posnode_id))); // @todo err
	{
		PPID   local_person_id = 0;
		if(FetchPersonFromClientPacket(rCliPack, &local_person_id, true/*logResult*/) > 0) {
			// ??? agent_psn_id = local_person_id;
		}
		if(posnode_id == ROBJID_CONTEXT) {
			if(!local_person_id && rCliPack.Rec.LinkObjType == PPOBJ_CASHNODE) {
				posnode_id = rCliPack.Rec.LinkObjID;
			}
			else
				posnode_id = 0;
		}
	}
	THROW_PP(posnode_id, PPERR_STQ_UNDEFPOSFORINDOORSVC);
	THROW(cn_obj.Fetch(posnode_id, &cn_rec) > 0);
	THROW_PP(cn_rec.Flags & CASHF_SYNC, PPERR_STQ_POSFORINDOORSVCMUSTBESYNC);
	THROW(cn_obj.GetSync(posnode_id, &cn_sync_pack) > 0);
	//THROW(stp_obj.GetPacket(posnode_id, &stp_pack) > 0);
	{
		const bool is_agent_orders = (rCmdItem.ObjTypeRestriction == PPOBJ_PERSON && rCmdItem.ObjGroupRestriction == PPPRK_AGENT);
		SJson js(SJson::tOBJECT);
		// @v11.6.2 {
		if(!!rCmdItem.Uuid)
			js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
		js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
		// } @v11.6.2 
		StqInsertIntoJs_BaseCurrency(&js);
		js.InsertInt("posnodeid", posnode_id);
		THROW(MakeRsrvIndoorSvcPrereqResponse_ExportGoods(bc_own_ident, &cn_sync_pack, 0, 0, &js, &stat));
		THROW(js.ToStr(rResult));
		// @debug {
		if(prccmdFlags & prccmdfDebugOutput) {
			SString out_file_name;
			PPGetFilePath(PPPATH_OUT, "stq-rsrvindoorsvcprereq.json", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			f_out.WriteLine(rResult);
		}
		// } @debug
		THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
	}
	StoreOidListWithBlob(stat.BlobOidList.SortAndUndup());
	ufp.SetFactor(0, static_cast<double>(stat.GoodsCount));
	ufp.SetFactor(1, static_cast<double>(stat.ClientCount));
	ufp.Commit();
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::MakeRsrvPriceListResponse_ExportGoods(const StyloQCommandList::Item & rCmdItem, const SBinaryChunk & rOwnIdent, 
	const StyloQDocumentPrereqParam & rParam, SJson * pJs, Stq_CmdStat_MakeRsrv_Response * pStat)
{
	/*
		quotkind_list [ { id; nm } ]
		goodsgroup_list [ { id; parid; nm } ]
		warehouse_list [ { id; nm } ]
		uom_list [ { id; nm; (fragmentation|rounding|int) } ]
		brand_list [ { id; nm } ]
		goods_list [ { id; nm; parid; uomid; code_list [ { cod; qty } ]; brandid; upp; price; stock; (ordqtymult|ordminqty); quot_list [ { id; val } ] ]
	*/
	int    ok = 1;
	const  LDATETIME now_dtm = getcurdatetime_();
	const bool hide_stock = LOGIC(rParam.Flags & StyloQDocumentPrereqParam::fHideStock); // @v11.6.4
	PPObjBill * p_bobj = BillObj;
	SString temp_buf;
	MakeInnerGoodsEntryBlock mige_blk;
	PPObjQuotKind qk_obj;
	SVector goods_list(sizeof(InnerGoodsEntry));
	PPIDArray temp_loc_list;
	StyloQBlobInfo blob_info;
	PPStyloPalmPacket palm_pack;
	bool   export_zstock = false; // Экспортировать нулевые остатки
	bool   export_expiry_tags = false; // @v11.6.2 Если true, то для агентских заказов пользователь может видеть дополнительную информацию о сроке годности товаров.
	int    ahead_expiry_days = 0; // @v11.6.2 Если export_expiry_tags, то эта переменная получает количество дней до истечения срока годности, когда этот срок становится критичным.
		// Значение извлекается из конфигурации документов (PPBillConfig::WarnLotExpirDays)
	SString cmd_name;
	(cmd_name = rCmdItem.Name).Transf(CTRANSF_UTF8_TO_INNER);
	if(rParam.PalmID) {
		PPObjStyloPalm sp_obj;
		THROW(sp_obj.GetPacket(rParam.PalmID, &palm_pack));
		if(palm_pack.Rec.Flags & PLMF_EXPZSTOCK)
			export_zstock = true;
		if(palm_pack.Rec.Flags & PLMF_EXPGOODSEXPIRYTAGS) { // @v11.6.2
			export_expiry_tags = true;
			ahead_expiry_days = BillObj->GetConfig().WarnLotExpirDays;
			SETMAX(ahead_expiry_days, 0);
		}
	}
	else {
		assert(palm_pack.Rec.ID == 0);
	}
	if(!rParam.PalmID || !(palm_pack.Rec.Flags & PLMF_BLOCKED)) {
		PPIDArray final_loc_list; // @v11.6.2 Финальный список складов, с которыми работает агент. may be empty!
		const PPID single_loc_id = rParam.PalmID ? palm_pack.LocList.GetSingle() : rParam.LocID;
		PPID   single_qk_id = 0; // Наиболее приоритетный вид котировки из списка таковых
		PPIDArray qk_list;
		PPIDArray goods_list_by_qk;
		PPIDArray temp_list;
		if(rParam.PalmID) {
			palm_pack.LocList.Get(final_loc_list);
			palm_pack.QkList__.Get(qk_list);
		}
		else {
			final_loc_list.add(rParam.LocID);
			qk_list.addnz(rParam.QuotKindID);
		}
		THROW_PP_S(final_loc_list.getCount(), PPERR_STQ_UNDEFWHFORORDERPREREQ, cmd_name);
		qk_obj.ArrangeList(now_dtm, qk_list, 0);
		if(qk_list.getCount()) {
			single_qk_id = qk_list.get(0);
			SJson * p_js_list = 0;
			for(uint i = 0; i < qk_list.getCount(); i++) {
				const PPID qk_id = qk_list.get(i);
				PPQuotKind qk_rec;
				if(qk_obj.Fetch(qk_id, &qk_rec) > 0) {
					if(export_zstock) {
						mige_blk.GObj.P_Tbl->GetListByQuotKind(qk_id, single_loc_id, temp_list);
						goods_list_by_qk.add(&temp_list);
						if(single_loc_id) {
							mige_blk.GObj.P_Tbl->GetListByQuotKind(qk_id, 0, temp_list);
							goods_list_by_qk.add(&temp_list);
						}
					}
					SETIFZ(p_js_list, SJson::CreateArr());
					SJson * p_jsobj = SJson::CreateObj();
					p_jsobj->InsertInt("id", qk_rec.ID);
					p_jsobj->InsertString("nm", (temp_buf = qk_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_list->InsertChild(p_jsobj);
				}
			}
			pJs->InsertNz("quotkind_list", p_js_list);
			p_js_list = 0;
			// @v11.5.2 {
			if(goods_list_by_qk.getCount()) {
				goods_list_by_qk.sortAndUndup();
				if(palm_pack.Rec.ID && palm_pack.Rec.GoodsGrpID) {
					uint j = goods_list_by_qk.getCount();
					if(j) do {
						const PPID goods_id = goods_list_by_qk.get(--j);
						if(mige_blk.GObj.BelongToGroup(goods_id, palm_pack.Rec.GoodsGrpID, 0) > 0) {
							;
						}
						else
							goods_list_by_qk.atFree(j);
					} while(j);
				}
			}
			// } @v11.5.2 
		}
		{
			PPIDArray gt_quasi_unlim_list;
			{
				PPObjGoodsType gt_obj;
				PPGoodsType gt_rec;
				for(SEnum en = gt_obj.Enum(0); en.Next(&gt_rec) > 0;) {
					if(gt_rec.Flags & (GTF_QUASIUNLIM|GTF_UNLIMITED)) 
						gt_quasi_unlim_list.add(gt_rec.ID);
				}
			}
			GoodsRestFilt gr_filt;
			PPViewGoodsRest gr_view;
			GoodsRestViewItem gr_item;
			Goods2Tbl::Rec goods_rec;
			//GoodsStockExt gse;
			//PPEgaisProcessor ep(PPEgaisProcessor::cfUseVerByConfig, 0, 0);
			gr_filt.LocList.Set(&final_loc_list);
			if(rParam.PalmID) {
				gr_filt.GoodsGrpID = palm_pack.Rec.GoodsGrpID;
				if(palm_pack.Rec.Flags & PLMF_EXPZSTOCK || gt_quasi_unlim_list.getCount())
					gr_filt.Flags |= GoodsRestFilt::fNullRest;
			}
			gr_filt.CalcMethod = GoodsRestParam::pcmLastLot;
			gr_filt.WaitMsgID  = PPTXT_WAIT_GOODSREST;
			THROW(gr_view.Init_(&gr_filt));
			for(gr_view.InitIteration(); gr_view.NextIteration(&gr_item) > 0;) {
				if(mige_blk.GObj.Fetch(gr_item.GoodsID, &goods_rec) > 0) {
					const bool is_quasi_unlim = gt_quasi_unlim_list.lsearch(goods_rec.GoodsTypeID);
					if(export_zstock || gr_item.Rest > 0.0 || is_quasi_unlim) {
						InnerGoodsEntry goods_entry(gr_item.GoodsID);
						if(mige_blk.Make(gr_item.GoodsID, gr_item.Cost, gr_item.Price, gr_item.Rest, gr_item.UnitPerPack, goods_entry) > 0) {
							// @v11.6.4 {
							if(!hide_stock) { // Если установлен общий признак сокрытия остатков, то нет смысла расходовать ресурсы на передачу InnerGoodsEntry::fDontShowRest для конкретного товара
								if(is_quasi_unlim)
									goods_entry.Flags |= InnerGoodsEntry::fDontShowRest;
							}
							// } @v11.6.4 
							goods_list.insert(&goods_entry);
							goods_list_by_qk.removeByID(goods_entry.GoodsID);
						}
					}
				}
			}
			{
				for(uint i = 0; i < goods_list_by_qk.getCount(); i++) {
					const PPID   goods_id = goods_list_by_qk.get(i);
					double cost = 0.0;
					double price = 0.0;
					double rest = 0.0;
					double unitperpack = 0.0;
					for(uint qkidx = 0; qkidx < qk_list.getCount(); qkidx++) {
						const PPID qk_id = qk_list.get(qkidx);
						double quot = 0.0;
						QuotIdent qi(QIDATE(now_dtm.d), single_loc_id, qk_id);
						if(mige_blk.GObj.GetQuotExt(goods_id, qi, 0.0, 0.0, &quot, 1) > 0) {
							price = quot;
							break;
						}
					}
					if(price > 0.0) {
						InnerGoodsEntry goods_entry(goods_id);
						if(mige_blk.Make(goods_id, cost, price, rest, unitperpack, goods_entry) > 0) {
							goods_list.insert(&goods_entry);
						}
					}
				}
			}
		}
		// (done by MakeObjArrayJson_GoodsGroup) grp_id_list.sortAndUndup();
		// (done by MakeObjArrayJson_Brand) brand_id_list.sortAndUndup();
		// (done by MakeObjArrayJson_Uom) unit_id_list.sortAndUndup();
		if(rParam.PalmID && palm_pack.Rec.Flags & PLMF_EXPLOC) {
			//
			// Склады
			//
			SJson * p_js_list = 0;
			PPIDArray loc_list;
			PPObjLocation loc_obj;
			LocationTbl::Rec loc_rec;
			loc_obj.GetWarehouseList(&loc_list, 0);
			for(uint i = 0; i < loc_list.getCount(); i++) {
				const PPID loc_id = loc_list.get(i);
				if(rParam.PalmID && final_loc_list.lsearch(loc_id) && loc_obj.Fetch(loc_id, &loc_rec) > 0) {
					SETIFZ(p_js_list, SJson::CreateArr());
					SJson * p_jsobj = SJson::CreateObj();
					p_jsobj->InsertInt("id", loc_rec.ID);
					p_jsobj->InsertString("nm", (temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape());
					p_js_list->InsertChild(p_jsobj);
				}
			}
			pJs->InsertNz("warehouse_list", p_js_list);
		}
		{
			const bool are_groups_hierarchical = LOGIC(rParam.Flags & StyloQDocumentPrereqParam::fUseHierarchGroups);
			pJs->InsertBool("goodsgroup_list_hierarchical", are_groups_hierarchical); // @v11.6.4
			pJs->InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(rOwnIdent, mige_blk.GrpIdList, are_groups_hierarchical, pStat));
		}
		pJs->InsertNz("uom_list", MakeObjArrayJson_Uom(rOwnIdent, mige_blk.UnitIdList, pStat));
		if((rParam.PalmID && palm_pack.Rec.Flags & PLMF_EXPBRAND) || (rParam.Flags & StyloQDocumentPrereqParam::fUseBrands)) {
			pJs->InsertNz("brand_list", MakeObjArrayJson_Brand(rOwnIdent, mige_blk.BrandIdList, 0, pStat));
		}
		{
			SJson * p_goods_list = 0;
			PPGoodsPacket goods_pack;
			for(uint glidx = 0; glidx < goods_list.getCount(); glidx++) {
				const InnerGoodsEntry & r_goods_entry = *static_cast<const InnerGoodsEntry *>(goods_list.at(glidx));
				const PPObjID oid(PPOBJ_GOODS, r_goods_entry.GoodsID);
				if(mige_blk.GObj.GetPacket(r_goods_entry.GoodsID, &goods_pack, PPObjGoods::gpoSkipQuot) > 0) {
					SJson * p_jsobj = MakeObjJson_Goods(rOwnIdent, goods_pack, &r_goods_entry, 0, 0, pStat);
					THROW(p_jsobj);
					// @v11.6.2 {
					if(export_expiry_tags) {
						PPID   critical_lot_id = 0;
						LDATE  critical_expiry = ZERODATE;
						for(uint locidx = 0; locidx < final_loc_list.getCount(); locidx++) {
							const PPID _loc_id = final_loc_list.get(locidx);
							PPID   local_critical_lot_id = 0;
							LDATE  local_critical_expiry = ZERODATE;
							if(p_bobj->trfr->Rcpt.GetMostCriticalExpiryDate(r_goods_entry.GoodsID, _loc_id, &local_critical_lot_id, &local_critical_expiry) > 0) {
								if(!critical_expiry || local_critical_expiry < critical_expiry) {
									critical_expiry = local_critical_expiry;
									critical_lot_id = local_critical_lot_id;
								}
							}
							if(checkdate(critical_expiry)) {
								assert(critical_lot_id != 0);
								p_jsobj->InsertString("expiry", temp_buf.Z().Cat(critical_expiry, DATF_ISO8601CENT));
								if(diffdate(critical_expiry, now_dtm.d) <= ahead_expiry_days)
									p_jsobj->InsertBool("outofexpiry", "true");
							}
							else {
								assert(critical_lot_id == 0);
							}
						}
					}
					// } @v11.6.2 
					{
						//
						// Котировки
						//
						SJson * p_js_quot_list = 0;
						Transfer * p_trfr = BillObj->trfr;
						for(uint i = 0; i < qk_list.getCount(); i++) {
							const PPID qk_id = qk_list.get(i);
							double quot = 0.0;
							QuotIdent qi(QIDATE(now_dtm.d), single_loc_id, qk_id);
							if(mige_blk.GObj.GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
								SJson * p_js_quot = SJson::CreateObj();
								p_js_quot->InsertInt("id", qk_id);
								p_js_quot->InsertDouble("val", quot, MKSFMTD(0, 2, NMBF_NOTRAILZ));
								SETIFZ(p_js_quot_list, SJson::CreateArr());
								p_js_quot_list->InsertChild(p_js_quot);
							}
							else if(!single_loc_id && (rParam.PalmID && palm_pack.LocList.GetCount())) {
								GoodsRestParam grparam;
								grparam.LocList = palm_pack.LocList.Get();
								grparam.GoodsID = r_goods_entry.GoodsID;
								grparam.DiffParam = GoodsRestParam::_diffLoc;
								p_trfr->GetCurRest(grparam);
								temp_loc_list.clear();
								if(grparam.Total.Rest > 0.0) {
									for(uint grvidx = 0; grvidx < grparam.getCount(); grvidx++) {
										GoodsRestVal & r_grv = grparam.at(grvidx);
										if(r_grv.Rest > 0.0)
											temp_loc_list.add(r_grv.LocID);
									}
								}
								{
									const PPID target_loc_id = temp_loc_list.getCount() ? temp_loc_list.get(0) : palm_pack.LocList.Get(0);
									QuotIdent qi(QIDATE(now_dtm.d), target_loc_id, qk_id);
									if(mige_blk.GObj.GetQuotExt(r_goods_entry.GoodsID, qi, r_goods_entry.Cost, r_goods_entry.Price, &quot, 1) > 0) {
										SJson * p_js_quot = SJson::CreateObj();
										p_js_quot->InsertInt("id", qk_id);
										p_js_quot->InsertDouble("val", quot, MKSFMTD(0, 2, NMBF_NOTRAILZ));
										SETIFZ(p_js_quot_list, SJson::CreateArr());
										p_js_quot_list->InsertChild(p_js_quot);
									}
								}
							}
						}
						p_jsobj->InsertNz("quot_list", p_js_quot_list);
					}
					SETIFZ(p_goods_list, SJson::CreateArr());
					p_goods_list->InsertChild(p_jsobj);
				}
			}
			{
				pJs->InsertBool("hidestock", hide_stock); // @v11.6.4
				pJs->InsertNz("goods_list", p_goods_list);
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_RsrvOrderPrereq(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack,
	SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	//StoreOidListWithBlob
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	PPObjIDArray bloboid_list;
	Stq_CmdStat_MakeRsrv_Response stat;
	PPUserFuncProfiler ufp(PPUPRF_STYLOQ_CMD_ORDERPREREQ);
	PPID   _stylopalm_id = 0;
	SString temp_buf;
	PPObjGoods goods_obj;
	PPObjArticle ar_obj;
	PPObjStyloPalm stp_obj;
	PPStyloPalmPacket stp_pack;
	SBinaryChunk bc_own_ident;
	bool   use_clidebt = false; // Если true, то для агентских заказов пользователь может видеть долги клиентов
	bool   export_expiry_tags = false; // @v11.6.2 Если true, то для агентских заказов пользователь может видеть дополнительную информацию о сроке годности товаров.
	int    ahead_expiry_days = 0; // @v11.6.2 Если export_expiry_tags, то эта переменная получает количество дней до истечения срока годности, когда этот срок становится критичным.
		// Значение извлекается из конфигурации документов (PPBillConfig::WarnLotExpirDays)
	PPID   agent_psn_id = 0;
	StyloQDocumentPrereqParam * p_filt = 0;
	SBuffer param_buf(rCmdItem.Param);
	THROW(GetOwnIdent(bc_own_ident, 0));
	THROW(p_filt = StyloQDocumentPrereqParam::Read(param_buf)); // @v11.5.0
	// @v11.5.0 THROW(rCmdItem.Param.ReadStatic(&stylopalm_id, sizeof(stylopalm_id))); // @todo err
	//if(stylopalm_id == ROBJID_CONTEXT) {
	_stylopalm_id = p_filt->PalmID;
	if(_stylopalm_id == ROBJID_CONTEXT) {
		PPID   local_person_id = 0;
		PPIDArray stp_id_list;
		THROW(FetchPersonFromClientPacket(rCliPack, &local_person_id, true/*logResult*/) > 0);
		if(local_person_id && stp_obj.GetListByPerson(local_person_id, stp_id_list) > 0) {
			_stylopalm_id = stp_id_list.get(0);
			p_filt->PalmID = _stylopalm_id;
			agent_psn_id = local_person_id;
		}
	}
	if(_stylopalm_id) {
		THROW(stp_obj.GetPacket(_stylopalm_id, &stp_pack) > 0);
		// @v11.5.2 {
		assert(p_filt->PalmID == _stylopalm_id);
		if(stp_pack.Rec.AgentID) {
			PPID   acs_id = 0;
			agent_psn_id = ObjectToPerson(stp_pack.Rec.AgentID, &acs_id);
			if(acs_id != GetAgentAccSheet()) {
				agent_psn_id = 0;
			}
			if(stp_pack.Rec.Flags & PLMF_EXPCLIDEBT) { // @v11.5.4
				use_clidebt = true;
			}
			if(stp_pack.Rec.Flags & PLMF_EXPGOODSEXPIRYTAGS) { // @v11.6.2
				export_expiry_tags = true;
				ahead_expiry_days = BillObj->GetConfig().WarnLotExpirDays;
				if(ahead_expiry_days < 0)
					ahead_expiry_days = 0;
			}
		}
		// } @v11.5.2 
	}
	{
		const bool is_agent_orders = (rCmdItem.ObjTypeRestriction == PPOBJ_PERSON && rCmdItem.ObjGroupRestriction == PPPRK_AGENT);
		SJson js(SJson::tOBJECT);
		// @v11.6.2 {
		if(!!rCmdItem.Uuid)
			js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
		js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
		// } @v11.6.2 
		StqInsertIntoJs_BaseCurrency(&js);
		// @v11.4.9 {
		{
			PPID   op_id = 0;
			GetContextualOpAndLocForOrder(*p_filt, &op_id, 0); 
			if(op_id > 0)
				js.InsertInt("svcopid", op_id);
		}
		// } @v11.4.9 
		if(is_agent_orders && agent_psn_id) {
			js.InsertInt("agentid", agent_psn_id);
		}
		// @v11.4.8 {
		{
			PPClientAgreement agt;
			if(ar_obj.GetClientAgreement(0, agt, 0) > 0) {
				if(agt.DefDuePeriodHour > 0 && agt.DefDuePeriodHour <= (24*180)) {
					js.InsertInt("dueperiodhr", agt.DefDuePeriodHour);
				}
			}
		}
		// } @v11.4.8 
		// @v11.6.2 {
		if(export_expiry_tags) {
			if(ahead_expiry_days > 0)
				js.InsertInt("aheadexpirydays", ahead_expiry_days);
		}
		// } @v11.6.2 
		// @v11.5.0 {
		assert(p_filt);
		if(p_filt->Flags & StyloQDocumentPrereqParam::fUseBarcodeSearch) {
			js.InsertBool("searchbarcode", true);
		}
		// @v11.5.4 {
		if(use_clidebt) { 
			js.InsertBool("useclidebt", true);
		}
		// } @v11.5.4 
		// @v11.6.4 {
		if(p_filt->Flags & StyloQDocumentPrereqParam::fDlvrDateAsNominal) {
			js.InsertBool("duedateasnominal", true);
		}
		// } @v11.6.4
		{
			const PPGoodsConfig & r_gcfg = goods_obj.GetConfig();
			{
				temp_buf = r_gcfg.WghtPrefix;
				if(temp_buf.NotEmptyS() && temp_buf.IsDigit())
					js.InsertString("barcodeweightprefix", temp_buf);
			}
			{
				temp_buf = r_gcfg.WghtCntPrefix;
				if(temp_buf.NotEmptyS() && temp_buf.IsDigit())
					js.InsertString("barcodecountprefix", temp_buf);
			}
		}
		// } @v11.5.0 
		THROW(MakeRsrvPriceListResponse_ExportGoods(rCmdItem, bc_own_ident, /*&stp_pack*/*p_filt, &js, &stat));
		if(is_agent_orders) {
			THROW(MakeRsrvPriceListResponse_ExportClients(bc_own_ident, /*&stp_pack*/*p_filt, &js, &stat));
		}
		THROW(js.ToStr(rResult));
		// @debug {
		if(prccmdFlags & prccmdfDebugOutput) {
			SString out_file_name;
			PPGetFilePath(PPPATH_OUT, "stq-rsrvorderprereq.json", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			f_out.WriteLine(rResult);
		}
		// } @debug
		THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
	}
	StoreOidListWithBlob(stat.BlobOidList.SortAndUndup());
	ufp.SetFactor(0, static_cast<double>(stat.GoodsCount));
	ufp.SetFactor(1, static_cast<double>(stat.ClientCount));
	ufp.Commit();
	CATCHZOK
	delete p_filt;
	return ok;
}

int PPStyloQInterchange::ProcessCommand_Report(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const SGeoPosLL & rGeoPos, 
	SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	SString temp_buf;
	SJson * p_js = 0;
	PPView * p_view = 0;
	PPBaseFilt * p_filt = 0;
	DlRtm  * p_rtm = 0;
	assert(rCmdItem.BaseCmdId == StyloQCommandList::sqbcReport);
	{
		// @debug {
			const PPThreadLocalArea & r_tla = DS.GetConstTLA();
			assert((&r_tla) != 0);
		// } @debug
		rResult.Z();
		SString dl600_name(rCmdItem.Vd.GetStrucSymb());
		uint p = 0;
		LoadViewSymbList();
		long   view_id = ViewSymbList.SearchByText(rCmdItem.ViewSymb, 1, &p) ? ViewSymbList.at_WithoutParent(p).Id : 0;
		THROW_PP_S(view_id, PPERR_NAMEDFILTUNDEFVIEWID, rCmdItem.ViewSymb);
		THROW(PPView::CreateInstance(view_id, &p_view));
		{
			if(rCmdItem.Param.GetAvailableSize()) {
				SBuffer temp_non_const_buf(rCmdItem.Param);
				THROW(PPView::ReadFiltPtr(temp_non_const_buf, &p_filt));
			}
			else {
				THROW(p_filt = p_view->CreateFilt(PPView::GetDescriptionExtra(p_view->GetViewId())));
			}
			THROW(p_view->Init_(p_filt));
			if(!dl600_name.NotEmptyS()) {
				if(p_view->GetDefReportId()) {
					SReport rpt(p_view->GetDefReportId(), 0);
					THROW(rpt.IsValid());
					dl600_name = rpt.getDataName();
				}
			}
			{
				DlContext ctx;
				PPFilt f(p_view);
				DlRtm::ExportParam ep;
				THROW(ctx.InitSpecial(DlContext::ispcExpData));
				THROW(ctx.CreateDlRtmInstance(dl600_name, &p_rtm));
				ep.P_F = &f;
				ep.Sort = 0;
				ep.Flags |= (DlRtm::ExportParam::fIsView|DlRtm::ExportParam::fInheritedTblNames);
				ep.Flags &= ~DlRtm::ExportParam::fDiff_ID_ByScope;
				SETFLAG(ep.Flags, DlRtm::ExportParam::fCompressXml, /*pNf->Flags & PPNamedFilt::fCompressXml*/0);
				ep.Flags |= (DlRtm::ExportParam::fDontWriteXmlDTD|DlRtm::ExportParam::fDontWriteXmlTypes);
				/*
				if(pNf->VD.GetCount() > 0)
					ep.P_ViewDef = &pNf->VD;
				*/
				if(rCmdItem.Vd.GetCount()) {
					ep.P_ViewDef = &rCmdItem.Vd;
				}
				ep.Cp = cpUTF8;
				// format == SFileFormat::Json)
				{
					ep.Flags |= DlRtm::ExportParam::fJsonStQStyle; 
					THROW(p_js = p_rtm->ExportJson(ep));
					THROW_SL(p_js->ToStr(rResult));
				}
				// @debug {
				if(prccmdFlags & prccmdfDebugOutput) {
					SString out_file_name;
					PPGetFilePath(PPPATH_OUT, "stq-report.json", out_file_name);
					SFile f_out(out_file_name, SFile::mWrite);
					f_out.WriteLine(rResult);
				}
				// } @debug
				THROW(MakeDocDeclareJs(rCmdItem, dl600_name, rDocDeclaration));
			}
		}
	}
	CATCHZOK
	delete p_js;
	delete p_rtm;
	delete p_filt;
	delete p_view;
	return ok;
}

int PPStyloQInterchange::ProcessCommand_IncomingListTSess(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, const LDATETIME * pIfChangedSince, SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	LDATETIME _ifchangedsince = ZERODATETIME;
	SJson * p_js_doc_list = 0;
	SString temp_buf;
	PPIDArray goods_id_list;
	SBinaryChunk bc_own_ident;
	StyloQIncomingListParam param;
	Stq_CmdStat_MakeRsrv_Response stat;
	if(pIfChangedSince && checkdate(pIfChangedSince->d))
		_ifchangedsince = *pIfChangedSince;
	THROW(GetOwnIdent(bc_own_ident, 0));
	THROW(rCmdItem.GetSpecialParam<StyloQIncomingListParam>(param));
	SETIFZQ(param.P_TsF, new TSessionFilt);
	{
		PPIDArray prc_id_list;
		PPID   prc_id = param.P_TsF ? param.P_TsF->PrcID : 0;
		PPID   local_prc_id = 0;
		PPObjTSession tses_obj;
		if(FetchProcessorFromClientPacket(rCliPack, &local_prc_id, true/*logResult*/) > 0) {
			;
		}
		if(prc_id == ROBJID_CONTEXT) {
			param.P_TsF->PrcID = local_prc_id;
			prc_id = local_prc_id;
		}
		if(prc_id)
			prc_id_list.add(prc_id);
		else {
			ProcessorTbl::Rec prc_rec;
			for(SEnum en = tses_obj.PrcObj.P_Tbl->Enum(PPPRCK_PROCESSOR, 0); en.Next(&prc_rec) > 0;) {
				prc_id_list.add(prc_rec.ID);
			}
		}
		{
			SJson js(SJson::tOBJECT);
			PPIDArray goods_id_list;
			if(prc_id_list.getCount()) {
				SBinaryChunk bc_own_ident;
				THROW(GetOwnIdent(bc_own_ident, 0));
				MakePrcJsListParam mpjsl_param(bc_own_ident, param);
				prc_id_list.sortAndUndup();
				THROW(MakePrcJsList(mpjsl_param, &js, prc_id_list, tses_obj, &stat));
			}
			//
			{
				TSessionFilt _filt(*param.P_TsF);
				PPViewTSession _view;
				TSessionViewItem _item;
				_filt.PrcID = prc_id_list.getSingle();
				if(CConfig.Flags & CCFLG_DEBUG && !param.Period.IsZero())
					_filt.StPeriod = param.Period;
				else if(param.LookbackDays > 0)
					_filt.StPeriod.Set(plusdate(dtm_now.d, -param.LookbackDays), ZERODATE);
				else
					_filt.StPeriod.Set(dtm_now.d, ZERODATE);
				THROW(_view.Init_(&_filt));
				{
					THROW_SL(p_js_doc_list = SJson::CreateArr());
					for(_view.InitIteration(0); _view.NextIteration(&_item) > 0;) {
						TSessionPacket tses_pack;
						if(tses_obj.GetPacket(_item.ID, &tses_pack, 0) > 0) {
							Document doc;
							doc.FromTSessionPacket(tses_pack, &goods_id_list);
							{
								SJson * p_js_doc = doc.ToJsonObject();
								THROW(p_js_doc);
								p_js_doc_list->InsertChild(p_js_doc);
								p_js_doc = 0;
							}
						}
					}
					assert(p_js_doc_list);
					js.Insert("doc_list", p_js_doc_list);
				}
			}
			//
			THROW(js.ToStr(rResult));
			// @debug {
			if(prccmdFlags & prccmdfDebugOutput) {
				SString out_file_name;
				PPGetFilePath(PPPATH_OUT, "stq-incominglisttsess.json", out_file_name);
				SFile f_out(out_file_name, SFile::mWrite);
				f_out.WriteLine(rResult);
			}
			// } @debug
			THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_IncomingListCCheck(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, 
	const LDATETIME * pIfChangedSince, SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	LDATETIME _ifchangedsince = ZERODATETIME;
	SJson * p_js_doc_list = 0;
	SString temp_buf;
	PPIDArray goods_id_list;
	SBinaryChunk bc_own_ident;
	StyloQIncomingListParam param;
	Stq_CmdStat_MakeRsrv_Response stat;
	if(pIfChangedSince && checkdate(pIfChangedSince->d))
		_ifchangedsince = *pIfChangedSince;
	THROW(GetOwnIdent(bc_own_ident, 0));
	THROW(rCmdItem.GetSpecialParam<StyloQIncomingListParam>(param));
	{
		PPObjCashNode cn_obj;
		PPCashNode cn_rec;
		PPSyncCashNode cn_sync_pack;
		PPID   posnode_id = param.P_CcF ? param.P_CcF->NodeList.GetSingle() : 0;
		{
			PPID   local_person_id = 0;
			if(FetchPersonFromClientPacket(rCliPack, &local_person_id, true/*logResult*/) > 0) {
				// ??? agent_psn_id = local_person_id;
			}
			if(posnode_id == ROBJID_CONTEXT) {
				posnode_id = (!local_person_id && rCliPack.Rec.LinkObjType == PPOBJ_CASHNODE) ? rCliPack.Rec.LinkObjID : 0;
			}
		}
		THROW_PP(posnode_id, PPERR_STQ_UNDEFPOSFORINDOORSVC);
		THROW(cn_obj.Fetch(posnode_id, &cn_rec) > 0);
		THROW_PP(cn_rec.Flags & CASHF_SYNC, PPERR_STQ_POSFORINDOORSVCMUSTBESYNC);
		THROW(cn_obj.GetSync(posnode_id, &cn_sync_pack) > 0);
		{
			SJson js(SJson::tOBJECT);
			PPObjBill * p_bobj = BillObj;
			MakeInnerGoodsEntryBlock mige_blk;
			Goods2Tbl::Rec goods_rec;
			CPosProcessor cpp(cn_sync_pack.ID, 0, 0, CPosProcessor::ctrfForceInitGroupList, /*pDummy*/0);
			//PPObjGoods goods_obj;
			const PPID single_loc_id = cn_sync_pack.LocID;
			DateRange period;
			TSVector <CCheckViewItem> cclist;
			CCheckCore & r_cc = cpp.GetCc();
			bool nothing_to_do = false;
			if(!!_ifchangedsince) {
				CheckOpJrnl * p_oj = r_cc.GetOpJrnl();
				if(p_oj) {
					TSVector <CheckOpJrnl::Rec> oj_rec_list;
					nothing_to_do = true;
					p_oj->GetListSince(_ifchangedsince, oj_rec_list);
					for(uint ojidx = 0; nothing_to_do && ojidx < oj_rec_list.getCount(); ojidx++) {
						const CheckOpJrnl::Rec & r_oj_rec = oj_rec_list.at(ojidx);
						if(oneof3((r_oj_rec.Action-1), CCheckCore::logRestored, CCheckCore::logSuspended, CCheckCore::logRemoved)) {
							nothing_to_do = false;
						}
					}
				}
			}
			{
				// @v11.6.2 {
				if(!!rCmdItem.Uuid)
					js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
				js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
				// } @v11.6.2 
				StqInsertIntoJs_BaseCurrency(&js); // @v11.5.8
				js.InsertInt("posnodeid", posnode_id); // @v11.5.8
				{
					if(param.ActionFlags) {
						Document::IncomingListActionsToString(param.ActionFlags, temp_buf);
						if(temp_buf.NotEmptyS())
							js.InsertString("actions", temp_buf);
					}
				}
			}
			if(nothing_to_do) {
				js.InsertBool("nochanges", true);
			}
			else {
				if(CConfig.Flags & CCFLG_DEBUG && !param.Period.IsZero())
					period = param.Period;
				else if(param.LookbackDays > 0)
					period.Set(plusdate(dtm_now.d, -param.LookbackDays), ZERODATE);
				else
					period.Set(dtm_now.d, ZERODATE);
				cpp.Backend_GetCCheckList(&period, 0, cclist);
				// StyloQCommandList::sqbdtCCheck
				THROW_SL(p_js_doc_list = SJson::CreateArr());
				for(uint i = 0; i < cclist.getCount(); i++) {
					const CCheckViewItem & r_item = cclist.at(i);
					CCheckPacket cc_pack;
					if(r_cc.LoadPacket(r_item.ID, 0, &cc_pack) > 0) {
						PPStyloQInterchange::Document doc;
						THROW(doc.FromCCheckPacket(cc_pack, posnode_id, &goods_id_list));
						{
							SJson * p_js_doc = doc.ToJsonObject();
							THROW(p_js_doc);
							p_js_doc_list->InsertChild(p_js_doc);
							p_js_doc = 0;
						}
						//cli_id_list.addnz(bpack.Rec.Object);
					}
				}
				assert(p_js_doc_list);
				js.Insert("doc_list", p_js_doc_list);
				p_js_doc_list = 0;
				THROW(MakeRsrvIndoorSvcPrereqResponse_ExportGoods(bc_own_ident, &cn_sync_pack, &goods_id_list, 0, &js, &stat)); // @v11.5.6
				/* @v11.5.6
				if(goods_id_list.getCount()) {
					goods_id_list.sortAndUndup();
					SVector goods_list(sizeof(InnerGoodsEntry)); // ###
					{
						for(uint i = 0; i < goods_id_list.getCount(); i++) {
							const PPID goods_id = goods_id_list.get(i);
							assert(goods_id > 0);
							if(mige_blk.GObj.Fetch(goods_id, &goods_rec) > 0) {
								InnerGoodsEntry goods_entry(goods_id);
								GoodsRestParam rp;
								rp.CalcMethod = GoodsRestParam::pcmMostRecent;
								rp.GoodsID = goods_id;
								rp.Date = ZERODATE;
								rp.LocList.setSingleNZ(single_loc_id);
								p_bobj->trfr->GetRest(rp);
								if(mige_blk.Make(goods_id, rp.Total.Cost, rp.Total.Price, rp.Total.Rest, rp.Total.UnitsPerPack, goods_entry) > 0) {
									goods_list.insert(&goods_entry);
								}
							}
						}
					}
					js.InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(bc_own_ident, mige_blk.GrpIdList, 0));
					js.InsertNz("brand_list", MakeObjArrayJson_Brand(bc_own_ident, mige_blk.BrandIdList, 0, 0));
					js.InsertNz("uom_list", MakeObjArrayJson_Uom(bc_own_ident, mige_blk.UnitIdList, 0));
					{
						SJson * p_js_goods_list = 0;
						PPGoodsPacket goods_pack;
						for(uint i = 0; i < goods_list.getCount(); i++) {
							const InnerGoodsEntry & r_goods_entry = *static_cast<const InnerGoodsEntry *>(goods_list.at(i));
							if(mige_blk.GObj.GetPacket(r_goods_entry.GoodsID, &goods_pack, PPObjGoods::gpoSkipQuot) > 0) {
								SJson * p_jsobj = MakeObjJson_Goods(bc_own_ident, goods_pack, &r_goods_entry, 0, 0, 0);
								if(p_jsobj) {
									SETIFZ(p_js_goods_list, SJson::CreateArr());
									p_js_goods_list->InsertChild(p_jsobj);
								}
							}
						}
						js.InsertNz("goods_list", p_js_goods_list);
						p_js_goods_list = 0;
					}
				}
				*/
			}
			THROW(js.ToStr(rResult));
			// @debug {
			if(prccmdFlags & prccmdfDebugOutput) {
				SString out_file_name;
				PPGetFilePath(PPPATH_OUT, "stq-incominglistccheck.json", out_file_name);
				SFile f_out(out_file_name, SFile::mWrite);
				f_out.WriteLine(rResult);
			}
			// } @debug
			THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
		}
	}
	CATCHZOK
	return ok;
}

static IMPL_CMPFUNC(SvcNotification_ObjNominalTime, i1, i2)
{
	const PPStyloQInterchange::SvcNotification * p1 = static_cast<const PPStyloQInterchange::SvcNotification *>(i1);
	const PPStyloQInterchange::SvcNotification * p2 = static_cast<const PPStyloQInterchange::SvcNotification *>(i2);
	return cmp(p1->ObjNominalTime, p2->ObjNominalTime);
}

int PPStyloQInterchange::ProcessCommand_RequestNotificationList(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, SJson * pJsArray)
{
	assert(pJsArray != 0);
	int    ok = -1;
	SString temp_buf;
	if(pJsArray) {
		PPObjBill * p_bobj = BillObj;
		switch(rCmdItem.BaseCmdId) {
			case StyloQCommandList::sqbcIncomingListOrder:
				{
					// Копия прямого исполнения команды sqbcIncomingListOrder
					//IntermediateReply(3*60*1000, 1000, pSessSecret, intermediateReplyProc, pIntermediateReplyExtra);
					/*if(ProcessCommand_IncomingListOrder(StyloQCommandList::Item(*p_targeted_item), cli_pack, reply_text_buf, temp_buf.Z(), prccmdfReqNotification)) {
						reply_doc.Put(reply_text_buf, reply_text_buf.Len());
						if(temp_buf.Len())
							reply_doc_declaration.Put(temp_buf, temp_buf.Len());
						cmd_reply_ok = true;									
					}
					else {
						PPLogMessage(PPFILNAM_ERR_LOG, 0, LOGMSGF_LASTERR|LOGMSGF_TIME|LOGMSGF_DBINFO);
						PPSetError(PPERR_SQ_CMDFAULT_INCOMINGLIST);
					}*/

					PPIDArray obj_id_list;
					SysJournal * p_sj = DS.GetTLA().P_SysJ;
					if(p_sj) {
						LDATETIME since;
						StyloQIncomingListParam param;
						THROW(rCmdItem.GetSpecialParam<StyloQIncomingListParam>(param));
						if(param.LookbackDays > 0) {
							since.Set(plusdate(getcurdate_(), -param.LookbackDays), ZEROTIME);
						}
						else {
							since.Set(getcurdate_(), ZEROTIME);
						}
						PPIDArray act_list;
						TSVector <SysJournalTbl::Rec> rec_list;
						act_list.addzlist(PPACN_TURNBILL, 0L); // @?
						p_sj->GetObjListByEventSince(PPOBJ_BILL, &act_list, since, obj_id_list, &rec_list);
						/* evnt_list
							{
								id - суррогатный 128bit-идентификатор, формируемый по набору полей. 
									используется на стороне клиента для избежания дубликатов
								evnt_org_time
								evnt_iss_time
								evnt
								objtype
								objid
								msgsign|msgid
								msg
							}
						*/
						const LDATETIME dtm_now = getcurdatetime_();
						TSCollection <SvcNotification> nlist;
						{
							SString obj_text;
							for(uint i = 0; i < rec_list.getCount(); i++) {
								const SysJournalTbl::Rec & r_rec = rec_list.at(i);
								BillTbl::Rec bill_rec;
								if(r_rec.ObjType && r_rec.ObjID && p_bobj->Search(r_rec.ObjID, &bill_rec) > 0) {
									SvcNotification * p_item = nlist.CreateNewItem();
									p_item->CmdUuid = rCmdItem.Uuid; // @v11.5.11
									p_item->EventOrgTime.Set(r_rec.Dt, r_rec.Tm);
									p_item->EventIssueTime = dtm_now;
									p_item->ObjNominalTime.Set(bill_rec.Dt, ZEROTIME);
									p_item->Oid.Set(r_rec.ObjType, r_rec.ObjID);
									if(r_rec.Action == PPACN_TURNBILL) {
										p_item->EventId = PPEVENTTYPE_ORDERCREATED;
										PPLoadString("eventtype_ordercreated", temp_buf);
										PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddObjName, obj_text);
										temp_buf.CatDiv(':', 2).Cat(obj_text);
										temp_buf.Transf(CTRANSF_INNER_TO_UTF8);
										p_item->Message = temp_buf;
									}
									else {
										p_item->EventId = r_rec.Action; // @?
									}
									p_item->GenerateIdent();
								}
							}
						}
						{
							nlist.sort(PTR_CMPFUNC(SvcNotification_ObjNominalTime));
							for(uint i = 0; i < nlist.getCount(); i++) {
								const SvcNotification * p_item = nlist.at(i);
								if(p_item) {
									SJson * p_js_item = p_item->ToJson();
									if(p_js_item) {
										pJsArray->InsertChild(p_js_item);
										ok = 1;
									}
								}
							}
						}
						//js.Insert("evnt_list", p_js_list);
					}
				}
				break;
			case StyloQCommandList::sqbcIncomingListCCheck:
				break;
			case StyloQCommandList::sqbcIncomingListTodo:
				break;
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_IncomingListOrder(const StyloQCommandList::Item & rCmdItem, const StyloQCore::StoragePacket & rCliPack, 
	const LDATETIME * pIfChangedSince, SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = 1;
	const  LDATETIME dtm_now = getcurdatetime_();
	SJson * p_js_doc_list = 0;
	SBinaryChunk bc_own_ident;
	StyloQIncomingListParam param;
	THROW(GetOwnIdent(bc_own_ident, 0));
	THROW(rCmdItem.GetSpecialParam<StyloQIncomingListParam>(param));
	{
		PPObjBill * p_bobj = BillObj;
		//PPID   op_id = 0;
		//PPID   loc_id = 0;
		//int    lookback_days = 365; // @debug value 365
		SString temp_buf;
		PPOprKind op_rec;
		PPID   local_person_id = 0;
		SString cmd_name;
		(cmd_name = rCmdItem.Name).Transf(CTRANSF_UTF8_TO_INNER);
		THROW(param.StQBaseCmdId == StyloQCommandList::sqbcIncomingListOrder);
		THROW(param.P_BF); // @err
		THROW_PP_S(param.P_BF->OpID, PPERR_STQ_UNDEFOPFORINCOMINGLIST, cmd_name); // @err
		if(FetchPersonFromClientPacket(rCliPack, &local_person_id, true/*logResult*/) > 0) {
			// ??? agent_psn_id = local_person_id;
		}
		THROW(GetOpData(param.P_BF->OpID, &op_rec) > 0);
		{
			struct ListEntry {
				ListEntry(const BillViewItem & rS)
				{
					Dt = rS.Dt;
					ID = rS.ID;
				}
				LDATE  Dt; // @firstmember
				PPID   ID;
			};
			SJson js(SJson::tOBJECT);
			MakeInnerGoodsEntryBlock mige_blk;
			Goods2Tbl::Rec goods_rec;
			SVector temp_entry_list(sizeof(ListEntry));
			PPIDArray goods_id_list;
			PPIDArray cli_id_list; // Список идентификаторов аналитический статей клиентов
			BillFilt bill_filt;
			PPViewBill bill_view;
			BillViewItem bill_item;
			if(CConfig.Flags & CCFLG_DEBUG && !param.Period.IsZero()) {
				bill_filt.Period = param.Period;
			}
			else if(param.LookbackDays > 0) {
				bill_filt.Period.low = plusdate(dtm_now.d, -param.LookbackDays);
				bill_filt.Period.upp = ZERODATE;
			}
			else {
				bill_filt.Period.low = dtm_now.d;
				bill_filt.Period.upp = ZERODATE;
			}
			bill_filt.OpID = param.P_BF->OpID;
			bill_filt.LocList = param.P_BF->LocList;
			if(op_rec.OpTypeID == PPOPT_GOODSORDER) {
				bill_filt.Flags |= (BillFilt::fOrderOnly|BillFilt::fUnshippedOnly);
				bill_filt.Bbt = bbtOrderBills;
			}
			else if(op_rec.OpTypeID == PPOPT_INVENTORY) {
				bill_filt.Flags |= (BillFilt::fInvOnly);
				bill_filt.Bbt = bbtInventoryBills;
			}
			THROW(bill_view.Init_(&bill_filt));
			for(bill_view.InitIteration(PPViewBill::OrdByDefault); bill_view.NextIteration(&bill_item) > 0;) {
				ListEntry new_entry(bill_item);
				temp_entry_list.insert(&new_entry);
			}
			if(temp_entry_list.getCount()) {
				// @v11.6.2 {
				if(!!rCmdItem.Uuid)
					js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
				js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
				// } @v11.6.2 
				StqInsertIntoJs_BaseCurrency(&js);
				if(param.ActionFlags) {
					Document::IncomingListActionsToString(param.ActionFlags, temp_buf);
					if(temp_buf.NotEmptyS())
						js.InsertString("actions", temp_buf);
				}
				if(param.StatusList.getCount()) {
					SJson * p_js_list = SJson::CreateArr();
					for(uint i = 0; i < param.StatusList.getCount(); i++) {
						const PPStyloQInterchange::Document::CliStatus & r_item = param.StatusList.at(i);
						SJson * p_js_item = r_item.ToJsonObj();
						if(p_js_item) {
							p_js_list->InsertChild(p_js_item);
							p_js_item = 0;
						}
					}
					js.Insert("status_list", p_js_list);
					p_js_list = 0;
				}
				THROW_SL(p_js_doc_list = SJson::CreateArr());
				temp_entry_list.sort2(PTR_CMPFUNC(LDATE));
				uint _c = temp_entry_list.getCount();
				if(_c) do {
					const ListEntry & r_entry = *static_cast<const ListEntry *>(temp_entry_list.at(--_c));
					PPBillPacket bpack;
					if(p_bobj->ExtractPacketWithFlags(r_entry.ID, &bpack, BPLD_LOADINVLINES) > 0) {
						bool do_skip = false;
						assert(IsOpBelongTo(bpack.Rec.OpID, param.P_BF->OpID));
						if(param.Flags & StyloQIncomingListParam::fBillWithMarksOnly) {
							if(!bpack.XcL.GetCount())
								do_skip = true;
						}
						if(!do_skip && param.Flags & StyloQIncomingListParam::fBillWithMarkedGoodsOnly) {
							bool is_there_wanted_goods = false;
							for(uint i = 0; !is_there_wanted_goods && i < bpack.GetTCount(); i++) {
								const PPTransferItem & r_ti = bpack.ConstTI(i);
								const PPID goods_id = labs(r_ti.GoodsID);
								PPGoodsType2 gt_rec;
								if(mige_blk.GObj.Fetch(goods_id, &goods_rec) > 0 && goods_rec.GoodsTypeID && mige_blk.GObj.FetchGoodsType(goods_rec.GoodsTypeID, &gt_rec) > 0) {
									if(gt_rec.Flags & GTF_GMARKED)
										is_there_wanted_goods = true;
								}
							}
							if(!is_there_wanted_goods)
								do_skip = true;
						}
						if(!do_skip) {
							PPStyloQInterchange::Document doc;
							THROW(doc.FromBillPacket(bpack, &param.StatusList, &goods_id_list));
							{
								SJson * p_js_doc = doc.ToJsonObject();
								THROW(p_js_doc);
								p_js_doc_list->InsertChild(p_js_doc);
								p_js_doc = 0;
							}
							cli_id_list.addnz(bpack.Rec.Object);
						}
					}
				} while(_c);
				assert(p_js_doc_list);
				js.Insert("doc_list", p_js_doc_list);
				p_js_doc_list = 0;
				if(goods_id_list.getCount()) {
					goods_id_list.sortAndUndup();
					SVector goods_list(sizeof(InnerGoodsEntry)); // ###
					{
						for(uint i = 0; i < goods_id_list.getCount(); i++) {
							const PPID goods_id = goods_id_list.get(i);
							assert(goods_id > 0);
							if(mige_blk.GObj.Fetch(goods_id, &goods_rec) > 0) {
								InnerGoodsEntry goods_entry(goods_id);
								GoodsRestParam rp;
								rp.CalcMethod = GoodsRestParam::pcmMostRecent;
								rp.GoodsID = goods_id;
								rp.Date = ZERODATE;
								param.P_BF->LocList.Get(rp.LocList);
								p_bobj->trfr->GetRest(rp);
								if(mige_blk.Make(goods_id, rp.Total.Cost, rp.Total.Price, rp.Total.Rest, rp.Total.UnitsPerPack, goods_entry) > 0) {
									goods_list.insert(&goods_entry);
								}
							}
						}
					}
					js.InsertNz("goodsgroup_list", MakeObjArrayJson_GoodsGroup(bc_own_ident, mige_blk.GrpIdList, false, 0));
					js.InsertNz("brand_list", MakeObjArrayJson_Brand(bc_own_ident, mige_blk.BrandIdList, 0, 0));
					js.InsertNz("uom_list", MakeObjArrayJson_Uom(bc_own_ident, mige_blk.UnitIdList, 0));
					{
						SJson * p_js_goods_list = 0;
						PPGoodsPacket goods_pack;
						for(uint i = 0; i < goods_list.getCount(); i++) {
							const InnerGoodsEntry & r_goods_entry = *static_cast<const InnerGoodsEntry *>(goods_list.at(i));
							if(mige_blk.GObj.GetPacket(r_goods_entry.GoodsID, &goods_pack, PPObjGoods::gpoSkipQuot) > 0) {
								SJson * p_jsobj = MakeObjJson_Goods(bc_own_ident, goods_pack, &r_goods_entry, 0, 0, 0);
								if(p_jsobj) {
									SETIFZ(p_js_goods_list, SJson::CreateArr());
									p_js_goods_list->InsertChild(p_jsobj);
								}
							}
						}
						js.InsertNz("goods_list", p_js_goods_list);
						p_js_goods_list = 0;
					}
				}
			}
			{
				if(cli_id_list.getCount()) {
					cli_id_list.sortAndUndup();
					SJson * p_js_client_list = 0;
					PPObjArticle ar_obj;
					PPObjAccSheet acs_obj;
					PPObjPerson psn_obj;
					PersonTbl::Rec psn_rec;
					ArticleTbl::Rec ar_rec;
					SString name;
					SString inn_buf;
					SString kpp_buf;
					SString addr;
					PPIDArray dlvr_loc_list;
					for(uint i = 0; i < cli_id_list.getCount(); i++) {
						const PPID ar_id = cli_id_list.get(i);
						if(ar_obj.Search(ar_id, &ar_rec) > 0) {
							PPID   acs_id = 0;
							PPID   psn_id = ObjectToPerson(ar_id, &acs_id);
							if(psn_id && psn_obj.Fetch(psn_id, &psn_rec) > 0) {
								name = psn_rec.Name;
								psn_obj.GetRegNumber(psn_id, PPREGT_TPID, inn_buf);
								psn_obj.GetRegNumber(psn_id, PPREGT_KPP, kpp_buf);
								{
									dlvr_loc_list.clear();
									psn_obj.GetDlvrLocList(psn_id, &dlvr_loc_list);
								}
							}
							else
								name = ar_rec.Name;
							SJson * p_js_obj = SJson::CreateObj();
							p_js_obj->InsertInt("id", ar_rec.ID);
							p_js_obj->InsertString("nm", name.Strip().Transf(CTRANSF_INNER_TO_UTF8).Escape());
							if(inn_buf.NotEmpty()) {
								p_js_obj->InsertString("ruinn", inn_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
								if(kpp_buf.NotEmpty())
									p_js_obj->InsertString("rukpp", kpp_buf.Transf(CTRANSF_INNER_TO_UTF8).Escape());
							}
							if(dlvr_loc_list.getCount()) {
								SJson * p_js_dlvrloc_list = 0;
								for(uint locidx = 0; locidx < dlvr_loc_list.getCount(); locidx++) {
									const PPID dlvr_loc_id = dlvr_loc_list.at(locidx);
									psn_obj.LocObj.GetAddress(dlvr_loc_id, 0, addr);
									if(addr.NotEmptyS()) {
										SJson * p_js_adr = SJson::CreateObj();
										p_js_adr->InsertInt("id", dlvr_loc_id);
										p_js_adr->InsertString("addr", (temp_buf = addr).Transf(CTRANSF_INNER_TO_UTF8).Escape());
										SETIFZ(p_js_dlvrloc_list, SJson::CreateArr());
										p_js_dlvrloc_list->InsertChild(p_js_adr);
									}
								}
								p_js_obj->InsertNz("dlvrloc_list", p_js_dlvrloc_list);
							}
							SETIFZQ(p_js_client_list, SJson::CreateArr());
							p_js_client_list->InsertChild(p_js_obj);
						}
					}
					js.InsertNz("client_list", p_js_client_list);
				}
			}
			THROW(js.ToStr(rResult));
			// @debug {
			if(prccmdFlags & prccmdfDebugOutput) {
				SString out_file_name;
				PPGetFilePath(PPPATH_OUT, "stq-incominglistorder.json", out_file_name);
				SFile f_out(out_file_name, SFile::mWrite);
				f_out.WriteLine(rResult);
			}
			// } @debug
			THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
		}
	}
	CATCHZOK
	return ok;
}

int PPStyloQInterchange::ProcessCommand_DebtList(const StyloQCommandList::Item & rCmdItem, const SJson * pJsCmd, const StyloQCore::StoragePacket & rCliPack, 
	SString & rResult, SString & rDocDeclaration, uint prccmdFlags)
{
	int    ok = -1;
	const  LDATETIME dtm_now = getcurdatetime_();
	const  bool use_omt_paym_amt = LOGIC(CConfig.Flags2 & CCFLG2_USEOMTPAYMAMT);
	PPID   ar_id = 0;
	SString temp_buf;
	PPObjBill * p_bobj = BillObj;
	PPObjArticle ar_obj;
	ArticleTbl::Rec ar_rec;
	SBinaryChunk bc_own_ident;
	THROW(GetOwnIdent(bc_own_ident, 0));
	{
		for(const SJson * p_cur = pJsCmd; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Type == SJson::tOBJECT) {								
				for(const SJson * p_obj = p_cur->P_Child; p_obj; p_obj = p_obj->P_Next) {
					if(p_obj->Text.IsEqiAscii("arid")) {
						ar_id = p_obj->P_Child->Text.ToLong();
					}
				}
			}
		}
	}
	THROW(ar_id > 0 && ar_obj.Search(ar_id, &ar_rec) > 0);
	{
		SJson js(SJson::tOBJECT);
		PayableBillList pb_list;
		double total_debt = 0.0;
		uint   total_bill_count = 0;
		//PayableBillListItem * p_pb_item;
		DebtTrnovrFilt debt_filt;
		debt_filt.AccSheetID = ar_rec.AccSheetID;
		debt_filt.Flags |= (DebtTrnovrFilt::fDebtOnly|DebtTrnovrFilt::fNoTempTable);
		PPViewDebtTrnovr debt_view;
		THROW(debt_view.Init_(&debt_filt));
		{
			SJson * p_js_list = new SJson(SJson::tARRAY);
			debt_view.GetPayableBillList(ar_id, 0L, &pb_list);
			for(uint i = 0; i < pb_list.getCount(); i++) {
				const PayableBillListItem & r_pb_item = pb_list.at(i);
				BillTbl::Rec bill_rec;
				if(p_bobj->Search(r_pb_item.ID, &bill_rec) > 0) {
					const double amt = BR2(bill_rec.Amount);
					double paym = 0.0;				
					if(use_omt_paym_amt)
						paym = bill_rec.PaymAmount;
					else
						p_bobj->P_Tbl->CalcPayment(r_pb_item.ID, 0, 0, r_pb_item.CurID, &paym);
					const double debt = R2(amt - paym);
					if(debt > 0.0) {
						PPBillExt bill_ext;
						p_bobj->FetchExt(bill_rec.ID, &bill_ext);
						total_debt += debt;
						total_bill_count++;
						SJson * p_js_list_item = new SJson(SJson::tOBJECT);
						p_js_list_item->InsertInt("billid", bill_rec.ID);
						(temp_buf = bill_rec.Code).Transf(CTRANSF_INNER_TO_UTF8).Escape();
						p_js_list_item->InsertString("billcode", temp_buf);
						temp_buf.Z().Cat(bill_rec.Dt, DATF_ISO8601CENT).Transf(CTRANSF_INNER_TO_UTF8).Escape();
						p_js_list_item->InsertString("billdate", temp_buf);
						p_js_list_item->InsertDouble("amt", amt, MKSFMTD(0, 2, NMBF_OMITEPS));
						p_js_list_item->InsertDouble("debt", debt, MKSFMTD(0, 2, NMBF_OMITEPS));
						p_js_list_item->InsertInt("agentid", bill_ext.AgentID);
						p_js_list->InsertChild(p_js_list_item);
						p_js_list_item = 0;
					}
				}
			}
			// @v11.6.2 {
			if(!!rCmdItem.Uuid)
				js.InsertString("orgcmduuid", temp_buf.Z().Cat(rCmdItem.Uuid, S_GUID::fmtIDL).Escape());
			// } @v11.6.2 
			js.InsertString("time", temp_buf.Z().Cat(dtm_now, DATF_ISO8601CENT, 0));
			js.InsertInt("arid", ar_id);
			(temp_buf = ar_rec.Name).Transf(CTRANSF_INNER_TO_UTF8).Escape();
			js.InsertString("arname", temp_buf);
			js.InsertInt("count", total_bill_count);
			js.InsertDouble("debt", total_debt, MKSFMTD(0, 2, NMBF_OMITEPS));
			js.Insert("debt_list", p_js_list);
		}
		THROW(js.ToStr(rResult));
		// @debug {
		if(prccmdFlags & prccmdfDebugOutput) {
			SString out_file_name;
			PPGetFilePath(PPPATH_OUT, "stq-debtlist.json", out_file_name);
			SFile f_out(out_file_name, SFile::mWrite);
			f_out.WriteLine(rResult);
		}
		// } @debug
		THROW(MakeDocDeclareJs(rCmdItem, 0, rDocDeclaration));
	}
	CATCHZOK
	return ok;
}