// IE_BILL.CPP
// Copyright (c) A.Starodub 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
//
#include <pp.h>
#pragma hdrstop
#include <ppsoapclient.h>
//
//
//
DocNalogRu_Base::FileInfo::FileInfo() : SenderPersonID(0), ReceiverPersonID(0), ProviderPersonID(0), Flags(0), CurDtm(ZERODATETIME)
{
}

DocNalogRu_Base::Address::Address() : RuRegionCode(0)
{
}

DocNalogRu_Base::Participant::Participant() : PartyQ(0), PersonID(0), LocID(0)
{
}

DocNalogRu_Base::GoodsItem::GoodsItem() : RowN(0), Qtty(0.0), Price(0.0), PriceWoVat(0.0), PriceSum(0.0), PriceSumWoVat(0.0), VatRate(0.0)
{
}

DocNalogRu_Base::DocumentInfo::DocumentInfo() : Dt(ZERODATE), InvcDate(ZERODATE)
{
}

DocNalogRu_Base::Participant * DocNalogRu_Base::DocumentInfo::GetParticipant(int partQ, bool createIfNExists)
{
	Participant * p_result = 0;
	if(partQ) {
		for(uint i = 0; !p_result && i < ParticipantList.getCount(); i++) {
			if(ParticipantList.at(i)->PartyQ == partQ)
				p_result = ParticipantList.at(i);
		}
		if(!p_result && createIfNExists) {
			p_result = ParticipantList.CreateNewItem();
			if(p_result)
				p_result->PartyQ = partQ;
		}
	}
	return p_result;
}

DocNalogRu_Base::DocNalogRu_Base() : TsHt(2048)
{
}

SString & FASTCALL DocNalogRu_Base::Helper_GetToken(long tokId)
{
	TokBuf.Z();
	if(!TsHt.Get(tokId, &TokBuf)) {
		PPLoadStringS(PPSTR_HASHTOKEN_C, tokId, TokBuf);
		TsHt.Put(tokId, TokBuf);
	}
	return TokBuf;
}

const SString & FASTCALL DocNalogRu_Base::GetToken_Ansi(long tokId)
	{ return Helper_GetToken(tokId).Transf(CTRANSF_INNER_TO_OUTER); }
const  SString & FASTCALL DocNalogRu_Base::GetToken_Ansi_Pe0(long n)
	{ return Helper_GetToken(PPHSC_RU_PE0).Transf(CTRANSF_INNER_TO_OUTER).CatLongZ(n, 11); }
const  SString & FASTCALL DocNalogRu_Base::GetToken_Ansi_Pe1(long n)
	{ return Helper_GetToken(PPHSC_RU_PE1).Transf(CTRANSF_INNER_TO_OUTER).CatLongZ(n, 11); }
const SString & FASTCALL DocNalogRu_Base::GetToken_Utf8(long tokId)
	{ return Helper_GetToken(tokId).Transf(CTRANSF_INNER_TO_UTF8); }

class DocNalogRu_Reader : public DocNalogRu_Base {
public:
	DocNalogRu_Reader() : DocNalogRu_Base()
	{
	}
	int    ReadFile(const char * pFileName, FileInfo & rHeader, TSCollection <DocumentInfo> & rDocList)
	{
		int    ok = -1;
		xmlParserCtxt * p_ctx = 0;
		xmlDoc * p_xml_doc = 0;
		SString temp_buf;
		if(fileExists(pFileName)) {
			xmlNode * p_root = 0;
			SString extra_key;
			SString extra_val;
			SString norm_barcode; // @v11.4.2 нормализованное представление штрихкода (только как параметр-заглушка для DiagBarcode)
			THROW(p_ctx = xmlNewParserCtxt());
			{
				SFile f_in(pFileName, SFile::mRead);
				int64 fs = 0;
				f_in.CalcSize(&fs);
				if(fs > 0) {
					STempBuffer temp_input(static_cast<size_t>(fs + 64)); // 64 insurance
					size_t actual_size = 0;
					THROW_SL(temp_input.IsValid());
					THROW_SL(f_in.Read(temp_input, temp_input.GetSize(), &actual_size));
					if(actual_size) {
						SString input_str;
						input_str.CatN(temp_input, actual_size);
						input_str.Helper_MbToMb(cp1251, cpUTF8);
						THROW_LXML(p_xml_doc = xmlCtxtReadMemory(p_ctx, input_str.cptr(), input_str.Len(), 0, "UTF-8", XML_PARSE_NOENT), p_ctx);
					}
				}
			}
			//THROW_LXML(p_xml_doc = xmlCtxtReadFile(p_ctx, pFileName, "WINDOWS-1251", XML_PARSE_NOENT), p_ctx);
			THROW(p_root = xmlDocGetRootElement(p_xml_doc));
			if(SXml::IsName(p_root, GetToken_Utf8(PPHSC_RU_FILE))) {
				if(SXml::GetAttrib(p_root, GetToken_Utf8(PPHSC_RU_IDFILE), temp_buf))
					rHeader.FileId = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_root, GetToken_Utf8(PPHSC_RU_VERFORM), temp_buf))
					rHeader.FileFormatVer = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_root, GetToken_Utf8(PPHSC_RU_VERPROG), temp_buf))
					rHeader.ProgVer = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				for(const xmlNode * p_n = p_root->children; p_n; p_n = p_n->next) {
					if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_EDISIDESINFO))) {
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_RECEIVERID), temp_buf)) {
							rHeader.SenderIdent = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_RECEIVERID), temp_buf)) {
							rHeader.ReceiverIdent =  temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						}
						for(const xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
							if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_EDIPROVIDERINFO))) { // Провайдер обмена
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_NAMEOFORG), temp_buf)) {
								}
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_INNJUR), temp_buf)) {
								}
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_EDIPROVIDERID), temp_buf)) {
								}
							}
						}
					}
					else if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_DOCUMENT))) {
						DocumentInfo * p_new_doc = rDocList.CreateNewItem();
						ok = 1;
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_KND), temp_buf))
							p_new_doc->KND = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_FUNCTION), temp_buf))
							p_new_doc->Function = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_NAMEOFDOC2), temp_buf)) {
						}
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_NAMEOFDOC), temp_buf)) {
						}
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_SELLERINFODATE), temp_buf)) {
						}
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_SELLERINFOTIME), temp_buf)) {
						}
						if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_NAMEECSUBJCOMP), temp_buf)) {
						}
						for(const xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
							if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_INVOICEHEADER))) {
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_INVOICENUMBER), temp_buf)) {
									p_new_doc->InvcCode = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								}
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_INVOICEDATE), temp_buf)) {
									p_new_doc->InvcDate = strtodate_(temp_buf, DATF_GERMANCENT);
								}
								if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_CODEOKV), temp_buf)) {
								}
								for(const xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
									if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_INVOICECORRECTION))) {
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_INVOICECORR_NOCODE), temp_buf)) {
										}
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_INVOICECORR_NODATE), temp_buf)) {
										}
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_SELLERINFO))) {
										Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_SELLER, true);
										if(p_part)
											ReadParticipant(p_n3, *p_part);
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_CONSIGNORINFO))) {
										Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_CONSIGNOR, true);
										if(p_part)
											ReadParticipant(p_n3, *p_part);
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_CONSIGNEEINFO))) {
										Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_CONSIGNEE, true);
										if(p_part)
											ReadParticipant(p_n3, *p_part);
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_BUYERINFO))) {
										Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_BUYER, true);
										if(p_part)
											ReadParticipant(p_n3, *p_part);
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_EXTRA1))) {
										for(const xmlNode * p_n4 = p_n3->children; p_n4; p_n4 = p_n4->next) {
											if(SXml::IsName(p_n4, GetToken_Utf8(PPHSC_RU_TEXTINF))) {
												if(ReadExtraValue(p_n4, extra_key, extra_val) > 0) {
													if(extra_key.IsEqiUtf8(GetToken_Utf8(PPHSC_RU_EXTRA_GLN_SUPPL))) { // @v11.1.12 CmpNC-->IsEqiUtf8
														Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_SELLER, true);
														if(p_part)
															p_part->GLN = extra_val.Transf(CTRANSF_UTF8_TO_INNER);
													}
													else if(extra_key.IsEqiUtf8(GetToken_Utf8(PPHSC_RU_EXTRA_GLN_CONSIGNOR))) { // @v11.1.12 CmpNC-->IsEqiUtf8
														Participant * p_part = p_new_doc->GetParticipant(EDIPARTYQ_CONSIGNOR, true);
														if(p_part)
															p_part->GLN = extra_val.Transf(CTRANSF_UTF8_TO_INNER);
													}
													else if(extra_key.IsEqiUtf8(GetToken_Utf8(PPHSC_RU_EXTRA_WAYBILLCODE))) { // @v11.1.12 CmpNC-->IsEqiUtf8
														p_new_doc->Code = extra_val.Transf(CTRANSF_UTF8_TO_INNER);
													}
													else if(extra_key.IsEqiUtf8(GetToken_Utf8(PPHSC_RU_EXTRA_WAYBILLDATE))) { // @v11.1.12 CmpNC-->IsEqiUtf8
														p_new_doc->Dt = strtodate_(extra_val, DATF_GERMANCENT);
													}
												}
											}
										}
									}
								}
							}
							else if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_INVOICETAB))) {
								for(const xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
									if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_WAREINFO))) {
										GoodsItem * p_item = p_new_doc->GoodsItemList.CreateNewItem();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_LINENUMBER), temp_buf))
											p_item->RowN = temp_buf.ToLong();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WARENAME), temp_buf))
											p_item->GoodsName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WAREOKEI), temp_buf))
											p_item->OKEI = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WAREQTTY), temp_buf))
											p_item->Qtty = temp_buf.ToReal();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WAREPRICE), temp_buf))
											p_item->PriceWoVat = temp_buf.ToReal();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WAREAMTWOVAT), temp_buf))
											p_item->PriceSumWoVat = temp_buf.ToReal();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_TAXRATE), temp_buf))
											p_item->VatRate = temp_buf.ToReal();
										if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_WAREAMT), temp_buf))
											p_item->PriceSum = temp_buf.ToReal();
										for(const xmlNode * p_n4 = p_n3->children; p_n4; p_n4 = p_n4->next) {
											if(SXml::IsName(p_n4, GetToken_Utf8(PPHSC_RU_EXCISE))) {
											}
											else if(SXml::IsName(p_n4, GetToken_Utf8(PPHSC_RU_AMTTAX))) {
											}
											else if(SXml::IsName(p_n4, GetToken_Utf8(PPHSC_RU_WAREEXTRAINFO))) {
												// PPHSC_RU_WAREEXTRAINFO
												// PPHSC_RU_WARECODE
												//
												if(SXml::GetAttrib(p_n4, GetToken_Utf8(PPHSC_RU_WARETYPE), temp_buf)) {
												}
												if(SXml::GetAttrib(p_n4, GetToken_Utf8(PPHSC_RU_UNITNAME), temp_buf)) {
													p_item->UOM = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
												}
												if(SXml::GetAttrib(p_n4, GetToken_Utf8(PPHSC_RU_WARECODE), temp_buf)) { // @v11.4.2
													// @v11.4.2 {
													temp_buf.Strip();
													int   bc_diag = 0;
													int   bc_std = 0;
													int   dbcr = PPObjGoods::DiagBarcode(temp_buf, &bc_diag, &bc_std, &norm_barcode);
													if(oneof4(bc_std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE)) {
														// Валидные коды с '2' в префиксе принимаем
														if(dbcr > 0 || (dbcr < 0 && bc_diag == PPObjGoods::cddFreePrefixEan13)) {
															p_item->GTIN = temp_buf;
														}
													}
													// } @v11.4.2 
												}
												for(const xmlNode * p_n5 = p_n4->children; p_n5; p_n5 = p_n5->next) {
													if(SXml::IsName(p_n5, GetToken_Utf8(PPHSC_RU_WAREIDENTBLOCK))) {
														for(const xmlNode * p_n6 = p_n5->children; p_n6; p_n6 = p_n6->next) {
															if(SXml::GetContentByName(p_n6, GetToken_Utf8(PPHSC_RU_WAREIDENT_PACKCODE), temp_buf) > 0) {
																p_item->MarkList.add(temp_buf.Transf(CTRANSF_UTF8_TO_INNER)); // номер марки
															}
															else if(SXml::GetContentByName(p_n6, GetToken_Utf8(PPHSC_RU_WAREIDENT_KIZ), temp_buf) > 0) { // @v10.8.7
																p_item->MarkList.add(temp_buf.Transf(CTRANSF_UTF8_TO_INNER)); // номер марки
															}
														}
													}
												}
											}
											else if(SXml::IsName(p_n4, GetToken_Utf8(PPHSC_RU_EXTRA2))) {
												if(ReadExtraValue(p_n4, extra_key, extra_val) > 0) {
													if(extra_key.IsEqiUtf8(GetToken_Utf8(PPHSC_RU_EXTRA_BARCODE))) { // @v11.1.12 CmpNC-->IsEqiUtf8
														// @v11.4.2 {
														extra_val.Strip().Transf(CTRANSF_UTF8_TO_INNER);
														/* Код под вопросом: в целом, он правильнее, чем безусловное присваивание (p_item->GTIN = extra_val)
														однако то уже работает давно, а это может привести к проблемам.
														if(p_item->GTIN.IsEmpty()) {
															int   bc_diag = 0;
															int   bc_std = 0;
															int   dbcr = PPObjGoods::DiagBarcode(extra_val, &bc_diag, &bc_std, &norm_barcode);
															if(oneof4(bc_std, BARCSTD_EAN13, BARCSTD_EAN8, BARCSTD_UPCA, BARCSTD_UPCE)) {
																if(dbcr > 0 || (dbcr < 0 && bc_diag == PPObjGoods::cddFreePrefixEan13)) {
																	p_item->GTIN = extra_val;
																}
															}
														}*/
														// @v11.4.2 {
														p_item->GTIN = extra_val; // @v11.4.2 see comment above
													}
												}
											}
										}
									}
									else if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_TOTALTOPAYM))) {
									}
								}
							}
							else if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_WARETRANSFINFO))) {
								for(const xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
									if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_WARETRANSFINFO2))) { // СвПер
									}
								}
							}
							else if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_SIGNER))) {
								for(const xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
									if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_JUR_S))) {
									}
								}
							}
						}
					}
				}
			}
		}
		CATCHZOK
		xmlFreeDoc(p_xml_doc);
		xmlFreeParserCtxt(p_ctx);
		return ok;
	}
private:
	// Читает атрибуты тега <??? Идентиф="key" Значен="val"/>
	int    ReadExtraValue(const xmlNode * pNode, SString & rKey, SString & rVal)
	{
		rKey.Z();
		rVal.Z();
		SString temp_buf;
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_IDENTIF), temp_buf))
			rKey = temp_buf;
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_VAL), temp_buf))
			rVal = temp_buf;
		return BIN(rKey.NotEmpty() && rVal.NotEmpty());
	}
	int    ReadAddress(const xmlNode * pNode, Address & rResult)
	{
		int    ok = 1;
		SString temp_buf;
		for(const xmlNode * p_n = pNode->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_ADDR_RF))) {
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_INDEX), temp_buf))
					rResult.ZIP = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_REGIONCODE), temp_buf))
					rResult.RuRegionCode = temp_buf.ToLong();
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_DESTRICT), temp_buf))
					rResult.Destrict = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_CITY), temp_buf))
					rResult.City = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_STREET), temp_buf))
					rResult.Street = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_HOUSE), temp_buf))
					rResult.House = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_HOUSECORP), temp_buf))
					rResult.HouseCorp = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_APARTM), temp_buf))
					rResult.Apart = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
			}
		}
		return ok;
	}
	int    ReadFIO(const xmlNode * pNode, FIO & rResult)
	{
		SString temp_buf;
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_SURNAME), temp_buf))
			rResult.Surname = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_NAME), temp_buf))
			rResult.Name = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_PATRONYMIC), temp_buf))
			rResult.Patronymic = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
		return 1;
	}
	int    ReadParticipant(const xmlNode * pNode, Participant & rResult)
	{
		int    ok = 1;
		SString temp_buf;
		if(SXml::GetAttrib(pNode, GetToken_Utf8(PPHSC_RU_OKPO), temp_buf))
			rResult.OKPO = temp_buf;
		for(const xmlNode * p_n = pNode->children; p_n; p_n = p_n->next) {
			if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_IDPARTICIPANT))) {
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_OKPO), temp_buf))
					rResult.OKPO = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				for(const xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_JURINFO))) {
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_NAMEOFORG), temp_buf))
							rResult.Appellation = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_INNJUR), temp_buf))
							rResult.INN = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_KPP), temp_buf))
							rResult.KPP = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					}
					// @v10.8.4 {
					else if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_PRIVEINFO))) {
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_INNPHS), temp_buf))
							rResult.INN = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_KPP), temp_buf))
							rResult.KPP = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						for(const xmlNode * p_n3 = p_n2->children; p_n3; p_n3 = p_n3->next) {
							if(SXml::IsName(p_n3, GetToken_Utf8(PPHSC_RU_FIO))) {
								if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_SURNAME), temp_buf))
									rResult.Surname = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_NAME), temp_buf))
									rResult.Name_ = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
								if(SXml::GetAttrib(p_n3, GetToken_Utf8(PPHSC_RU_PATRONYMIC), temp_buf))
									rResult.Patronymic = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
							}
						}
					}
					// } @v10.8.4
				}
			}
			else if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_ADDRESS))) {
				ReadAddress(p_n, rResult.Addr);
			}
			else if(SXml::IsName(p_n, GetToken_Utf8(PPHSC_RU_BANKACCINFO))) {
				if(SXml::GetAttrib(p_n, GetToken_Utf8(PPHSC_RU_ACCNO), temp_buf))
					rResult.BA.Account = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
				for(const xmlNode * p_n2 = p_n->children; p_n2; p_n2 = p_n2->next) {
					if(SXml::IsName(p_n2, GetToken_Utf8(PPHSC_RU_BANKINFO))) {
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_BANKNAME), temp_buf))
							rResult.BA.BankName = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
						if(SXml::GetAttrib(p_n2, GetToken_Utf8(PPHSC_RU_BANKBIC), temp_buf))
							rResult.BA.BIC = temp_buf.Transf(CTRANSF_UTF8_TO_INNER);
					}
				}
			}
		}
		return ok;
	}
};
//
//
//
IMPL_CMPFUNC(Sdr_Bill, i1, i2)
{
	const Sdr_Bill * p1 = static_cast<const Sdr_Bill *>(i1);
	const Sdr_Bill * p2 = static_cast<const Sdr_Bill *>(i2);
	int    si = CMPSIGN(p1->Date, p2->Date);
	SETIFZ(si, strcmp(p1->Code, p2->Code));
	SETIFZ(si, strcmp(p1->ID, p2->ID));
	return si;
}

IMPL_CMPFUNC(Sdr_BRow, i1, i2)
{
	const Sdr_BRow * p1 = static_cast<const Sdr_BRow *>(i1);
	const Sdr_BRow * p2 = static_cast<const Sdr_BRow *>(i2);
    int    si = CMPSIGN(p1->BillDate, p2->BillDate);
	SETIFZ(si, strcmp(p1->BillCode, p2->BillCode));
	SETIFZ(si, strcmp(p1->BillID, p2->BillID));
    return si;
}

IMPL_CMPFUNC(Sdr_BRow_ID, i1, i2)
{
	const Sdr_BRow * p1 = static_cast<const Sdr_BRow *>(i1);
	const Sdr_BRow * p2 = static_cast<const Sdr_BRow *>(i2);
	int    si = strcmp(p1->BillID, p2->BillID);
	SETIFZ(si, CMPSIGN(p1->BillDate, p2->BillDate));
	SETIFZ(si, strcmp(p1->INN, p2->INN));
    return si;
}
//
//
//
IMPLEMENT_IMPEXP_HDL_FACTORY(BILL, PPBillImpExpParam);
IMPLEMENT_IMPEXP_HDL_FACTORY(BROW, PPBillImpExpParam);

PPBillImpExpParam::PPBillImpExpParam(uint recId, long flags) : PPImpExpParam(recId, flags), Flags(0), ImpOpID(0), PredefFormat(piefUndef)
{
	Object1SrchCode.Z();
	Object2SrchCode.Z();
}

/*virtual*/int PPBillImpExpParam::MakeExportFileName(const void * extraPtr, SString & rResult) const
{
	rResult.Z();
	int    ok = 1;
	int    use_ps = 0;
	int    _is_subst = 0;
	SString _file_name = FileName;
	_file_name.Transf(CTRANSF_INNER_TO_OUTER);
	if(_file_name.IsEqiAscii(":buffer:")) {
		rResult = _file_name;
	}
	else {
		SString temp_buf;
		SPathStruc ps(_file_name);
		SPathStruc ps_temp;
		if(ps.Drv.IsEmpty() && ps.Dir.IsEmpty()) {
			PPGetPath(PPPATH_OUT, temp_buf);
			ps_temp.Split(temp_buf);
			ps.Drv = ps_temp.Drv;
			ps.Dir = ps_temp.Dir;
			use_ps = 1;
		}
		{
			size_t gp = 0;
			if(ps.Nam.Search("#guid", 0, 1, &gp)) {
				S_GUID(SCtrGenerate()).ToStr(S_GUID::fmtIDL, temp_buf.Z());
				ps.Nam.ReplaceStr("#guid", temp_buf, 0);
				use_ps = 1;
			}
		}
		if(extraPtr) {
			const PPBillPacket * p_pack = static_cast<const PPBillPacket *>(extraPtr);
			PPObjBill * p_bobj = BillObj;
			ps.Nam.Transf(CTRANSF_OUTER_TO_INNER); // @v10.3.11
			if(p_bobj && p_bobj->SubstText(p_pack, ps.Nam, temp_buf) == 2) {
				// @v10.3.11 temp_buf.Transf(CTRANSF_INNER_TO_OUTER);
				ps.Nam = temp_buf;
				use_ps = 1;
				ok = 100;
			}
			ps.Nam.Transf(CTRANSF_INNER_TO_OUTER); // @v10.3.11
		}
		char   cntr[128];
		uint   cn = 0;
		const uint fnl = static_cast<const uint>(ps.Nam.Len());
		for(uint i = 0; i < fnl; i++)
			if(ps.Nam.C(i) == '?')
				cntr[cn++] = '0';
		if(cn) {
			SString nam;
			int    overflow = 1;
			do {
				overflow = 1;
				for(int j = cn-1; overflow && j >= 0; j--) {
					if(cntr[j] < '9') {
						cntr[j]++;
						for(int z = j+1; z < static_cast<int>(cn); z++)
							cntr[z] = '0';
						overflow = 0;
					}
				}
				nam = 0;
				for(uint i = 0, k = 0; i < fnl; i++) {
					if(ps.Nam.C(i) == '?')
						nam.CatChar(cntr[k++]);
					else
						nam.CatChar(ps.Nam.C(i));
				}
				ps_temp.Copy(&ps, 0xffff);
				ps_temp.Nam = nam;
				ps_temp.Merge(rResult);
			} while(!overflow && fileExists(rResult));
			if(overflow)
				ok = PPSetError(PPERR_EXPFNTEMPLATEOVERFLOW, FileName);
			else
				ok = 100; // Имя создано по шаблону
		}
		else {
			if(use_ps)
				ps.Merge(rResult);
			else
				rResult = _file_name;
		}
	}
	return ok;
}

/*virtual*/int PPBillImpExpParam::PreprocessImportFileSpec(StringSet & rList)
{
	// mailfrom:coke@gmail.com?subj=orders
	int    ok = -1;
	SString _file_spec;
	(_file_spec = FileName).Transf(CTRANSF_INNER_TO_OUTER);
	{
		InetUrl url;
		const int urlpr = url.Parse(_file_spec);
		const int url_prot = (urlpr > 0) ? url.GetProtocol() : 0;
		{
			SString temp_buf;
			SPathStruc ps;
			if(url_prot > 0) {
				url.GetComponent(InetUrl::cPath, 0, temp_buf);
				ps.Split(temp_buf);
			}
			else
				ps.Split(_file_spec);
			SString templ = ps.Nam;
			SString name = ps.Nam;
			SString wildcard;
			PPImpExpParam::PtTokenList result_list;
			if(PPObjBill::ParseText(name, templ, result_list, &wildcard) > 0) {
				ps.Nam = wildcard;
				ps.Merge(temp_buf);
				if(url_prot > 0) {
					SString url_path;
					url_path.EncodeUrl(temp_buf, 0);
					url.SetComponent(InetUrl::cPath, temp_buf);
					url.Composite(0, _file_spec);
				}
				else
					_file_spec = temp_buf;
			}
		}
		THROW(GetFilesFromSource(_file_spec, rList, 0));
		/*if(url_prot > 0) {
			if(url_prot == InetUrl::protFile) {
			}
			else if(oneof2(url_prot, InetUrl::protFtp, InetUrl::protFtps)) {
			}
			else if(oneof2(url_prot, InetUrl::protHttp, InetUrl::protHttps)) {
			}
			else if(oneof2(url_prot, InetUrl::protPOP3, InetUrl::protPOP3S)) {
			}
			else if(url_prot == InetUrl::protMailFrom) {

			}
			if(urlpr > 0 && url_prot != InetUrl::protFile) {

			}
		}
		//THROW(GetFilesFromSource(wildcard, 0));
		//ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, path);*/
	}
	//rList.Scan(path, wildcard);
    if(rList.getCount())
		ok = 1;
	CATCHZOK
	return ok;
}

/*virtual*/int PPBillImpExpParam::PreprocessImportFileName(const SString & rFileName, /*StrAssocArray*/PPImpExpParam::PtTokenList & rResultList)
{
	rResultList.Z();
	int    ok = 1;
	SPathStruc ps(FileName);
	SString templ = ps.Nam;
	ps.Split(rFileName);
	SString name = ps.Nam;
	//StrAssocArray result_list;
	ok = PPObjBill::ParseText(name, templ, rResultList, 0);
	return ok;
}

/*virtual*/int PPBillImpExpParam::SerializeConfig(int dir, PPConfigDatabase::CObjHeader & rHdr, SBuffer & rTail, SSerializeContext * pSCtx)
{
	int    ok = 1;
	SString temp_buf;
	StrAssocArray param_list;
	PPObjOprKind op_obj;
	PPOprKind op_rec;
	THROW(PPImpExpParam::SerializeConfig(dir, rHdr, rTail, pSCtx));
	if(dir > 0) {
		if(ImpOpID && op_obj.Search(ImpOpID, &op_rec) > 0) {
			if(op_rec.Symb[0])
				temp_buf = op_rec.Symb;
			else
				temp_buf.Z().Cat(ImpOpID);
		}
		else
			temp_buf.Z();
		if(temp_buf.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_IMPOPSYMB, temp_buf);
		if(Flags)
			param_list.Add(IMPEXPPARAM_BILH_FLAGS, temp_buf.Z().Cat(Flags));
		if(Object1SrchCode.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_SRCHCODE1, (temp_buf = Object1SrchCode).Strip());
		if(Object2SrchCode.NotEmpty())
			param_list.Add(IMPEXPPARAM_BILH_SRCHCODE2, (temp_buf = Object2SrchCode).Strip());
		if(PredefFormat)
			param_list.Add(IMPEXPPARAM_BILH_PREDEFFMT, temp_buf.Z().Cat(PredefFormat));
	}
	THROW_SL(pSCtx->Serialize(dir, param_list, rTail));
	if(dir < 0) {
		Flags = 0;
		PredefFormat = piefUndef;
		ImpOpID = 0;
		Object1SrchCode.Z();
		Object2SrchCode.Z();
		for(uint i = 0; i < param_list.getCount(); i++) {
			StrAssocArray::Item item = param_list.at_WithoutParent(i);
			temp_buf = item.Txt;
			switch(item.Id) {
				case IMPEXPPARAM_BILH_IMPOPSYMB:
					{
						PPID   op_id = 0;
						if(op_obj.SearchBySymb(temp_buf, &op_id) > 0)
							ImpOpID = op_id;
						else if(op_obj.Search(temp_buf.ToLong(), &op_rec) > 0)
							ImpOpID = op_rec.ID;
					}
					break;
				case IMPEXPPARAM_BILH_FLAGS: Flags = temp_buf.ToLong(); break;
				case IMPEXPPARAM_BILH_SRCHCODE1: Object1SrchCode = temp_buf; break;
				case IMPEXPPARAM_BILH_SRCHCODE2: Object2SrchCode = temp_buf; break;
				case IMPEXPPARAM_BILH_PREDEFFMT: PredefFormat = temp_buf.ToLong(); break;
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImpExpParam::WriteIni(PPIniFile * pFile, const char * pSect) const
{
	int    ok = 1;
	SString params;
	SString fld_name;
	SString param_val;
	THROW(PPImpExpParam::WriteIni(pFile, pSect));
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_BILH, params));
	if(Direction != 0) {
		PPObjOprKind op_obj;
		PPOprKind op_rec;
		PPGetSubStr(params, IMPEXPPARAM_BILH_IMPOPSYMB, fld_name);
		if(ImpOpID && op_obj.Search(ImpOpID, &op_rec) > 0) {
			if(op_rec.Symb[0])
				param_val = op_rec.Symb;
			else
				param_val.Z().Cat(ImpOpID);
		}
		else
			param_val = 0;
		pFile->AppendParam(pSect, fld_name, param_val, 1);
		PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE1, fld_name);
		pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(Object1SrchCode), 1);
		PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE2, fld_name);
		pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(Object2SrchCode), 1);
	}
	PPGetSubStr(params, IMPEXPPARAM_BILH_FLAGS, fld_name);
	pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(Flags), 1);
	PPGetSubStr(params, IMPEXPPARAM_BILH_PREDEFFMT, fld_name);
	pFile->AppendParam(pSect, fld_name, param_val.Z().Cat(PredefFormat), 1);
	CATCHZOK
	return ok;
}

int PPBillImpExpParam::ReadIni(PPIniFile * pFile, const char * pSect, const StringSet * pExclParamList)
{
	ImpOpID = 0;
	Flags = 0;
	PredefFormat = piefUndef;
	int    ok = 1;
	SString params, fld_name, param_val;
	StringSet excl;
	RVALUEPTR(excl, pExclParamList);
	THROW(PPLoadText(PPTXT_IMPEXPPARAM_BILH, params));
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_IMPOPSYMB, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0) {
			PPObjOprKind op_obj;
			PPOprKind op_rec;
			PPID   op_id = 0;
			if(op_obj.SearchBySymb(param_val, &op_id) > 0)
				ImpOpID = op_id;
			else if(op_obj.Search(param_val.ToLong(), &op_rec) > 0)
				ImpOpID = op_rec.ID;
		}
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_FLAGS, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Flags = param_val.ToLong();
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE1, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Object1SrchCode = param_val;
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_SRCHCODE2, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			Object2SrchCode = param_val;
	}
	if(PPGetSubStr(params, IMPEXPPARAM_BILH_PREDEFFMT, fld_name)) {
		excl.add(fld_name);
		if(pFile->GetParam(pSect, fld_name, param_val) > 0)
			PredefFormat = param_val.ToLong();
	}
	THROW(PPImpExpParam::ReadIni(pFile, pSect, &excl));
	CATCHZOK
	return ok;
}
//
//
//
BillHdrImpExpDialog::BillHdrImpExpDialog() : ImpExpParamDialog(DLG_IMPEXPBILH)
{
}

IMPL_HANDLE_EVENT(BillHdrImpExpDialog)
{
	ImpExpParamDialog::handleEvent(event);
	if(event.isClusterClk(CTL_IMPEXP_DIR))
		SetupCtrls(GetClusterData(CTL_IMPEXP_DIR));
	else if(event.isClusterClk(CTL_IMPEXPBILH_FLAGS))
		GetClusterData(CTL_IMPEXPBILH_FLAGS, &Data.Flags);
	else if(event.isCbSelected(CTLSEL_IMPEXPBILH_PDFMT))
		SetupCtrls(GetClusterData(CTL_IMPEXP_DIR));
	else
		return;
	clearEvent(event);
}

void BillHdrImpExpDialog::SetupCtrls(long direction)
{
	// @v10.7.11 disableCtrls(direction == 0, CTL_IMPEXPBILH_FLAGS, CTLSEL_IMPEXPBILH_IMPOP, 0);
	// @v10.7.11 disableCtrl(CTL_IMPEXPBILH_FLAGS, direction == 0);
	disableCtrl(CTLSEL_IMPEXPBILH_IMPOP, direction == 0);
	DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 0, direction == 0);
	// @v10.7.11 DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 1, direction == 0 && !(Data.Flags & PPBillImpExpParam::fImpRowsFromSameFile));
	DisableClusterItem(CTL_IMPEXPBILH_FLAGS, 3, direction);
	{
		long pf = getCtrlLong(CTLSEL_IMPEXPBILH_PDFMT);
		const char * p_ssign = 0;
		if(pf == piefCokeOrder)
			p_ssign = "supplierinn";
		else
			p_ssign = "impexpparambillh_obj2srchcode";
		if(p_ssign) {
			SString temp_buf;
			setLabelText(CTL_IMPEXPBILH_SRCHCODE2, PPLoadStringS(p_ssign, temp_buf));
		}
	}
}

int BillHdrImpExpDialog::setDTS(const PPBillImpExpParam * pData)
{
	RVALUEPTR(Data, pData);
	ImpExpParamDialog::setDTS(&Data);
	SetupStringCombo(this, CTLSEL_IMPEXPBILH_PDFMT, PPTXT_PREDEFIMPEXPBILLFMT, Data.PredefFormat);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 0, PPBillImpExpParam::fImpRowsFromSameFile);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 1, PPBillImpExpParam::fImpExpRowsOnly);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 2, PPBillImpExpParam::fRestrictByMatrix);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 3, PPBillImpExpParam::fExpOneByOne);
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 4, PPBillImpExpParam::fDontIdentGoodsByName); // @v10.5.0
	AddClusterAssoc(CTL_IMPEXPBILH_FLAGS, 5, PPBillImpExpParam::fCreateAbsenceGoods); // @v10.4.12 @v10.5.0 4-->5
	SetClusterData(CTL_IMPEXPBILH_FLAGS, Data.Flags);

	PPIDArray op_types;
	op_types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_ACCTURN, PPOPT_GOODSORDER, 0L);
	SetupOprKindCombo(this, CTLSEL_IMPEXPBILH_IMPOP, Data.ImpOpID, 0, &op_types, 0);
	setCtrlString(CTL_IMPEXPBILH_SRCHCODE1, Data.Object1SrchCode);
	setCtrlString(CTL_IMPEXPBILH_SRCHCODE2, Data.Object2SrchCode);
	SetupCtrls(Data.Direction);
	return 1;
}

int BillHdrImpExpDialog::getDTS(PPBillImpExpParam * pData)
{
	int    ok = 1;
	THROW(ImpExpParamDialog::getDTS(&Data));
	getCtrlData(CTLSEL_IMPEXPBILH_PDFMT, &Data.PredefFormat);
	GetClusterData(CTL_IMPEXPBILH_FLAGS, &Data.Flags);
	getCtrlData(CTLSEL_IMPEXPBILH_IMPOP, &Data.ImpOpID);
	getCtrlString(CTL_IMPEXPBILH_SRCHCODE1, Data.Object1SrchCode);
	getCtrlString(CTL_IMPEXPBILH_SRCHCODE2, Data.Object2SrchCode);
	ASSIGN_PTR(pData, Data);
	CATCH
		ok = PPErrorByDialog(this, 0);
	ENDCATCH
	return ok;
}

class BillHdrImpExpCfgListDialog : public ImpExpCfgListDialog {
public:
	BillHdrImpExpCfgListDialog() : ImpExpCfgListDialog()
	{
		SetParams(PPFILNAM_IMPEXP_INI, PPREC_BILL, &Param, 0);
	}
private:
	virtual int EditParam(const char * pIniSection);
	PPBillImpExpParam Param;
};

int BillHdrImpExpCfgListDialog::EditParam(const char * pIniSection)
{
	int    ok = -1;
	BillHdrImpExpDialog * dlg = 0;
	PPBillImpExpParam param;
	SString ini_file_name, sect;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		int    direction = 0;
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		THROW(CheckDialogPtr(&(dlg = new BillHdrImpExpDialog())));
		THROW(LoadSdRecord(PPREC_BILL, &param.InrRec));
		direction = param.Direction;
		if(!isempty(pIniSection))
			THROW(param.ReadIni(&ini_file, pIniSection, 0));
		dlg->setDTS(&param);
		while(ok <= 0 && ExecView(dlg) == cmOK)
			if(dlg->getDTS(&param)) {
				int is_new = (pIniSection && *pIniSection && param.Direction == direction) ? 0 : 1;
				if(!isempty(pIniSection))
					if(is_new)
						ini_file.RemoveSection(pIniSection);
					else
						ini_file.ClearSection(pIniSection);
				PPSetError(PPERR_DUPOBJNAME);
				if((!is_new || ini_file.IsSectExists(param.Name) == 0) && param.WriteIni(&ini_file, param.Name) && ini_file.FlashIniBuf())
					ok = 1;
				else
					PPError();
			}
			else
				PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

int EditBillImpExpParams(int editBill)
{
	int    ok = -1;
	PPBillImpExpParam param;
	ImpExpParamDialog * dlg = 0;
	if(editBill) {
		BillHdrImpExpCfgListDialog * p_list_dlg = new BillHdrImpExpCfgListDialog();
		THROW(CheckDialogPtrErr(&p_list_dlg));
		ok = (ExecViewAndDestroy(p_list_dlg) == cmOK) ? 1 : -1;
	}
	else {
		THROW(CheckDialogPtr(&(dlg = new ImpExpParamDialog(DLG_IMPEXP, 0))));
		THROW(ok = EditImpExpParams(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, dlg));
	}
	CATCHZOK
	delete dlg;
	return ok;
}
//
// Import/Export Bills
//
int FASTCALL GetCliBnkSections(StringSet * pSectNames, int kind, PPCliBnkImpExpParam * pParam, uint maxBackup, PPLogger * pLogger); // @prototype
//
//
//
PPBillIterchangeFilt::PPBillIterchangeFilt() : LocID(0)
{
	Period.Z();
}
//
//
//
PPBillImpExpBaseProcessBlock::TransmitParam::TransmitParam() : InetAccID(0)
{
}

void PPBillImpExpBaseProcessBlock::TransmitParam::Reset()
{
	InetAccID = 0;
	AddrList.Z();
	Subject.Z();
}

PPBillImpExpBaseProcessBlock::SearchBlock::SearchBlock() : Dt(ZERODATE), SurveyDays(7)
{
}

PPBillImpExpBaseProcessBlock::PPBillImpExpBaseProcessBlock() : P_BObj(BillObj), DisabledOptions(0)
{
	Z();
}

PPBillImpExpBaseProcessBlock & PPBillImpExpBaseProcessBlock::Z()
{
	// Эта функция не сбрасывает DisabledOptions поскольку поле транзиентное и устанавливается
	// на прикладном уровне выше вызова Reset()
	Flags = 0;
	OpID = 0;
	LocID = 0;
	PosNodeID = 0;
	GuaID = 0; // @v10.6.5
	Period.Z();
	Tp.Reset();
	return *this;
}

int PPBillImpExpBaseProcessBlock::SerializeParam(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	int    is_ver_ok = 0;
	PPVersionInfo vi = DS.GetVersionInfo();
	SVerT cur_ver = vi.GetVersion();
	SVerT ser_ver;
	if(dir > 0) {
		if(BillParam.Name.NotEmpty())
			CfgNameBill = BillParam.Name;
		if(BRowParam.Name.NotEmpty())
			CfgNameBRow = BRowParam.Name;
		THROW_SL(cur_ver.Serialize(dir, rBuf, pCtx));
		is_ver_ok = 1;
	}
	else if(dir < 0) {
		const size_t preserve_offs = rBuf.GetRdOffs();
		int    mj = 0, mn = 0, rz = 0;
		THROW_SL(ser_ver.Serialize(dir, rBuf, pCtx));
		ser_ver.Get(&mj, &mn, &rz);
		if((mj >= 8 && mj <= 20) && (mn >= 0 && mn <= 12) && (rz >= 0 && rz <= 16)) {
			is_ver_ok = 1;
			if(ser_ver.IsGt(10, 6, 4))
				is_ver_ok = 2;
		}
		else
            rBuf.SetRdOffs(preserve_offs);
	}
	THROW_SL(pCtx->Serialize(dir, CfgNameBill, rBuf));
	THROW_SL(pCtx->Serialize(dir, CfgNameBRow, rBuf));
	THROW_SL(pCtx->Serialize(dir, Flags, rBuf));
	THROW_SL(pCtx->Serialize(dir, OpID, rBuf));
	THROW_SL(pCtx->Serialize(dir, LocID, rBuf));
	THROW_SL(pCtx->Serialize(dir, Period.low, rBuf));
	THROW_SL(pCtx->Serialize(dir, Period.upp, rBuf));
	if(dir > 0 || is_ver_ok) {
        THROW_SL(pCtx->Serialize(dir, Tp.InetAccID, rBuf));
        THROW_SL(pCtx->Serialize(dir, Tp.AddrList, rBuf));
        THROW_SL(pCtx->Serialize(dir, Tp.Subject, rBuf));
		// @v10.6.5 {
		if(dir > 0 || is_ver_ok >= 2)
			THROW_SL(pCtx->Serialize(dir, GuaID, rBuf));
		// } @v10.6.5
	}
	CATCHZOK
	return ok;
}

int PPBillImpExpBaseProcessBlock::Select(int import)
{
	class SelectBillImpCfgDialog : public TDialog {
	public:
		enum {
			ctlgroupEmailList = 1
		};
		SelectBillImpCfgDialog(uint dlgId, const char * pIniFileName, int import) : TDialog(dlgId), P_Data(0), Import(import)
		{
			P_IniFile = new PPIniFile(pIniFileName, 0, 1, 1);
			SetupCalPeriod(CTLCAL_IEBILLSEL_PERIOD, CTL_IEBILLSEL_PERIOD);
			if(!Import)
				addGroup(ctlgroupEmailList, new EmailCtrlGroup(CTL_IEBILLSEL_MAILADR, cmEMailList));
		}
		~SelectBillImpCfgDialog()
		{
			delete P_IniFile;
		}
		int    setDTS(PPBillImpExpBaseProcessBlock * pData)
		{
			P_Data = pData;
			HdrList.Z();
			LineList.Z();
			int    ok = 1;
			uint   id = 0;
			uint   p = 0;
			SString sect; // @vmiller
			PPBillImpExpParam param;
			THROW_INVARG(P_Data);
			THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &HdrList, Import ? 2 : 1));
			THROW(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BROW, &param, &LineList, Import ? 2 : 1));
			THROW(LoadSdRecord(PPREC_BILL, &P_Data->BillParam.InrRec)); // @vmiller (для отображения в фильтре иконки)
			THROW(LoadSdRecord(PPREC_BROW, &P_Data->BRowParam.InrRec)); // @vmiller (для отображения в фильтре иконки)
			P_Data->BillParam.ProcessName(2, sect = P_Data->CfgNameBill); // @vmiller (для отображения в фильтре иконки)
			id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)HdrList.Get(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
			SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, HdrList, (long)id, 0);
			P_Data->BRowParam.ProcessName(2, sect = P_Data->CfgNameBRow); // @vmiller (для отображения в фильтре иконки)
			id = (LineList.SearchByText(sect, 1, &(p = 0)) > 0) ? (uint)LineList.Get(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
			SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BROW, LineList, (long)id, 0);
			SetTech();
			if(Import) {
				PPIDArray op_types;
				op_types.addzlist(PPOPT_GOODSRECEIPT, PPOPT_GOODSEXPEND, PPOPT_DRAFTRECEIPT, PPOPT_DRAFTEXPEND, PPOPT_ACCTURN, PPOPT_GOODSORDER, 0L);
				PPID   init_op_id = NZOR(P_Data->OpID, P_Data->BillParam.ImpOpID);
				SetupOprKindCombo(this, CTLSEL_IEBILLSEL_OP, init_op_id, 0, &op_types, 0);
				SetupPPObjCombo(this, CTLSEL_IEBILLSEL_LOC, PPOBJ_LOCATION, P_Data->LocID, 0, 0);
				SetupControlsForEdi();
				SetPeriodInput(this, CTL_IEBILLSEL_PERIOD, &P_Data->Period);
				disableCtrls((P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|
					PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fChZnImpExp)), CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0);
				disableCtrls((P_Data->Flags & (PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fChZnImpExp)), CTLSEL_IEBILLSEL_OP, 0L);
				disableCtrls(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp), CTLSEL_IEBILLSEL_GUA, 0L); // @v10.6.5
				//
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 0, PPBillImpExpBaseProcessBlock::fTestMode);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 1, PPBillImpExpBaseProcessBlock::fDontRemoveTags);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 2, PPBillImpExpBaseProcessBlock::fEgaisVer3);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 3, PPBillImpExpBaseProcessBlock::fEgaisVer4); // @v11.0.12
				SetClusterData(CTL_IEBILLSEL_FLAGS, P_Data->Flags);
				DisableClusterItem(CTL_IEBILLSEL_FLAGS, 2, !(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp));
				// @v10.6.5 {
				if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp) {
					SetupPPObjCombo(this, CTLSEL_IEBILLSEL_GUA, PPOBJ_GLOBALUSERACC, P_Data->GuaID, 0, reinterpret_cast<void *>(0));
				}
				// } @v10.6.5
			}
			else {
				disableCtrls(1, CTLSEL_IEBILLSEL_OP, CTLSEL_IEBILLSEL_LOC, CTL_IEBILLSEL_PERIOD, 0L);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 0, PPBillImpExpBaseProcessBlock::fSignExport);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 1, PPBillImpExpBaseProcessBlock::fTestMode);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 2, PPBillImpExpBaseProcessBlock::fEgaisVer3);
				AddClusterAssoc(CTL_IEBILLSEL_FLAGS, 3, PPBillImpExpBaseProcessBlock::fEgaisVer4); // @v11.0.12
				SetClusterData(CTL_IEBILLSEL_FLAGS, P_Data->Flags);
				DisableClusterItem(CTL_IEBILLSEL_FLAGS, 2, !(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp));
				SetupPPObjCombo(this, CTLSEL_IEBILLSEL_MAILACC, PPOBJ_INTERNETACCOUNT, P_Data->Tp.InetAccID, 0,
					reinterpret_cast<void *>(PPObjInternetAccount::filtfMail)/*INETACCT_ONLYMAIL*/);
				{
					EmailCtrlGroup::Rec grp_rec(&P_Data->Tp.AddrList);
					setGroupData(ctlgroupEmailList, &grp_rec);
				}
				setCtrlString(CTL_IEBILLSEL_MAILSUBJ, P_Data->Tp.Subject);
				SetupWordSelector(CTL_IEBILLSEL_MAILSUBJ, new TextHistorySelExtra("emailsubj-exportbills-common"), 0, 2, WordSel_ExtraBlock::fFreeText); // @v10.7.7
			}
			CATCHZOKPPERR
			return ok;
		}
		int    getDTS()
		{
			int    ok = 1;
			uint   __id = 0;
			SString sect;
			THROW_INVARG(P_IniFile);
			THROW_INVARG(P_Data);
			GetTech();
			const  long bill_struc_id = getCtrlLong(CTLSEL_IEBILLSEL_BILL);
			const  int  is_full_edi_process = BIN(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp && bill_struc_id == 1000);
			SETFLAG(P_Data->Flags, PPBillImpExpBaseProcessBlock::fFullEdiProcess, is_full_edi_process);
			if(Import) {
				P_Data->OpID = getCtrlLong(CTLSEL_IEBILLSEL_OP);
				P_Data->LocID = getCtrlLong(CTLSEL_IEBILLSEL_LOC);
				SETIFZ(P_Data->LocID, LConfig.Location);
				// @v10.6.5 {
				if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp)
					P_Data->GuaID = getCtrlLong(CTLSEL_IEBILLSEL_GUA);
				// } @v10.6.5
				if(!(P_Data->Flags & (PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fChZnImpExp)) && !is_full_edi_process) {
					THROW_PP(P_Data->OpID, PPERR_INVOPRKIND);
				}
				GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags);
				THROW(GetPeriodInput(this, CTL_IEBILLSEL_PERIOD, &P_Data->Period));
				GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags);
			}
			else {
				GetClusterData(CTL_IEBILLSEL_FLAGS, &P_Data->Flags);
				getCtrlData(CTLSEL_IEBILLSEL_MAILACC, &P_Data->Tp.InetAccID);
				{
					EmailCtrlGroup::Rec grp_rec;
					getGroupData(ctlgroupEmailList, &grp_rec);
					P_Data->Tp.AddrList = grp_rec.AddrList;
				}
				getCtrlString(CTL_IEBILLSEL_MAILSUBJ, P_Data->Tp.Subject);
			}
			if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
				PPCliBnkImpExpParam cb_param;
				LoadSdRecord(PPREC_CLIBNKDATA, &cb_param.InrRec);
				//getCtrlData(CTLSEL_IEBILLSEL_BILL, &(id = 0));
				THROW_PP(bill_struc_id, PPERR_INVBILLIMPEXPCFG);
				HdrList.GetText(bill_struc_id, sect);
				cb_param.ProcessName(1, sect);
				P_Data->CfgNameBill = sect;
			}
			else if(!Import || (!is_full_edi_process && !(P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|
				PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fChZnImpExp)))) {
				P_Data->BillParam.Init(Import);
				P_Data->BRowParam.Init(Import);
				THROW(LoadSdRecord(PPREC_BILL, &P_Data->BillParam.InrRec));
				THROW(LoadSdRecord(PPREC_BROW, &P_Data->BRowParam.InrRec));
				if(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)) {
					//getCtrlData(CTLSEL_IEBILLSEL_BILL, &(id = 0));
					THROW_PP(bill_struc_id, PPERR_INVBILLIMPEXPCFG);
					HdrList.GetText(bill_struc_id, sect);
					if(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)) {
						if(sect.HasPrefixIAscii("DLL_"))
							P_Data->BillParam.BaseFlags |= PPImpExpParam::bfDLL;
						P_Data->BillParam.ProcessName(1, sect);
						THROW(P_Data->BillParam.ReadIni(P_IniFile, sect, 0));
						__id = getCtrlLong(CTLSEL_IEBILLSEL_BROW);
						if(!(P_Data->BillParam.BaseFlags & PPImpExpParam::bfDLL) && !P_Data->BillParam.PredefFormat) {
							if(__id || GetOpType(P_Data->OpID) != PPOPT_ACCTURN) {
								THROW_PP(__id, PPERR_INVBILLIMPEXPCFG);
								LineList.GetText(__id, sect);
								P_Data->BRowParam.ProcessName(1, sect);
								THROW(P_Data->BRowParam.ReadIni(P_IniFile, sect, 0));
							}
						}
					}
					else {
						P_Data->BillParam.BaseFlags |= PPImpExpParam::bfDLL;
						P_Data->BillParam.EDIDocType = bill_struc_id;
						P_Data->BillParam.Name = sect;
					}
				}
			}
			CATCHZOKPPERR
			return ok;
		}
	private:
		int    SetupControlsForEdi()
		{
			int    ok = -1;
			// @vmiller {
			if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
				// Заполняем списком типов документов
				long   id = 0; // @v10.3.12 uint-->long
				uint   p;
				HdrList.Z();
				SString buf;
				SString sect;
				PPLoadText(Import ? PPTXT_EDIIMPCMD : PPTXT_EDIEXPCMD, buf);
				StringSet ss(';', buf);
				for(uint i = 0; ss.get(&i, buf);) {
					uint j = 0;
					StringSet ss1(',', buf);
					ss1.get(&j, buf.Z());
					id = buf.ToLong();
					ss1.get(&j, buf.Z());
					ss1.get(&j, buf.Z());
					HdrList.Add(id, buf);
				}
				P_Data->BillParam.ProcessName(2, sect = P_Data->CfgNameBill); // @vmiller (для отображения в фильтре иконки)
				id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? HdrList.Get(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
				SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, HdrList, id, 0);
				ok = 1;
			}
			// } @vmiller
			return ok;
		}
		DECL_HANDLE_EVENT
		{
			TDialog::handleEvent(event);
			// @vmiller {
			if(event.isClusterClk(CTL_IEBILLSEL_TECH)) {
				if(P_Data) {
					long   id = 0; // @v10.3.12 uint-->long
					uint   p = 0;
					SString sect;
					SString buf;
					PPBillImpExpParam param;
					GetTech();
					disableCtrls(0, CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0L);
					disableCtrls(!(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp), CTLSEL_IEBILLSEL_GUA, 0L); // @v10.6.5
					disableCtrls((P_Data->Flags & (PPBillImpExpBaseProcessBlock::fUhttImport|
						PPBillImpExpBaseProcessBlock::fEgaisImpExp|PPBillImpExpBaseProcessBlock::fChZnImpExp)), CTLSEL_IEBILLSEL_BILL, CTLSEL_IEBILLSEL_BROW, 0L);
					if(P_Data->Flags & (PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fPaymOrdersExp))
						disableCtrl(CTLSEL_IEBILLSEL_BROW, 1);
					disableCtrls((P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp), CTLSEL_IEBILLSEL_OP, 0L);
					DisableClusterItem(CTL_IEBILLSEL_FLAGS, 2, !(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp));
					if(SetupControlsForEdi() > 0) {
					}
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp) {
					}
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp) {
						HdrList.Z();
						PPCliBnkImpExpParam cb_param;
						StringSet sections;
						GetCliBnkSections(&sections, Import ? 2 : 1, &cb_param, 0, 0);
						for(uint i = 0; sections.get(&i, buf);) {
							cb_param.ProcessName(2, sect = buf);
							HdrList.Add(i+1, sect);
						}
						SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, HdrList, id, 0);
					}
					// @v10.6.5 {
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp) {
						SetupPPObjCombo(this, CTLSEL_IEBILLSEL_GUA, PPOBJ_GLOBALUSERACC, P_Data->GuaID, 0, reinterpret_cast<void *>(0));
					}
					// } @v10.6.5
					else {
						HdrList.Z();
						if(GetImpExpSections(PPFILNAM_IMPEXP_INI, PPREC_BILL, &param, &HdrList, Import ? 2 : 1) > 0) {
							id = (HdrList.SearchByText(sect, 1, &(p = 0)) > 0) ? HdrList.Get(p).Id : 0; // @vmiller (для отображения в фильтре иконки)
							SetupStrAssocCombo(this, CTLSEL_IEBILLSEL_BILL, HdrList, id, 0);
						}
					}
				}
			}
			// } @vmiller
			else if(event.isCbSelected(CTLSEL_IEBILLSEL_BILL)) {
				long   hdr_id = getCtrlLong(CTLSEL_IEBILLSEL_BILL);
				SString sect;
				HdrList.GetText(hdr_id, sect);
				if(sect.NotEmpty()) {
					uint p = 0;
					if(LineList.SearchByText(sect, 1, &p) > 0)
						setCtrlLong(CTLSEL_IEBILLSEL_BROW, LineList.Get(p).Id);
					if(P_Data && P_IniFile) {
						P_Data->BillParam.ProcessName(1, sect);
						P_Data->BillParam.ReadIni(P_IniFile, sect, 0);
						if(P_Data->BillParam.ImpOpID)
							setCtrlLong(CTLSEL_IEBILLSEL_OP, P_Data->BillParam.ImpOpID);
					}
				}
			}
			else if(event.isCbSelected(CTLSEL_IEBILLSEL_BROW)) {
				long row_id = getCtrlLong(CTLSEL_IEBILLSEL_BROW);
				if(!getCtrlLong(CTLSEL_IEBILLSEL_BILL)) {
					SString sect;
					LineList.GetText(row_id, sect);
					if(sect.NotEmpty()) {
						uint p = 0;
						if(HdrList.SearchByText(sect, 1, &p) > 0)
							setCtrlLong(CTLSEL_IEBILLSEL_BILL, HdrList.Get(p).Id);
					}
				}
			}
			else
				return;
			clearEvent(event);
		}
		void SetTech()
		{
			ushort v = 0;
			if(P_Data) {
				if(Import) {
					if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fUhttImport)
						v = 1;
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)
						v = 2;
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)
						v = 3;
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fChZnImpExp)
						v = 4;
					else
						v = 0;
				}
				else {
					DisableClusterItem(CTL_IEBILLSEL_TECH, 1, BIN(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fEdiImpExp));
					DisableClusterItem(CTL_IEBILLSEL_TECH, 2, BIN(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fEgaisImpExp));
					DisableClusterItem(CTL_IEBILLSEL_TECH, 3, BIN(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fPaymOrdersExp));
					if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp)
						v = 1;
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fEgaisImpExp)
						v = 2;
					else if(P_Data->Flags & PPBillImpExpBaseProcessBlock::fPaymOrdersExp && !(P_Data->DisabledOptions & PPBillImpExpBaseProcessBlock::fPaymOrdersExp))
						v = 3;
					else
						v = 0;
				}
			}
			setCtrlUInt16(CTL_IEBILLSEL_TECH, v);
		}
		void GetTech()
		{
			if(P_Data) {
				ushort v = getCtrlUInt16(CTL_IEBILLSEL_TECH);
				long   _f = P_Data->Flags;
				if(Import) {
					_f &= ~(PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fEgaisImpExp|
						PPBillImpExpBaseProcessBlock::fUhttImport|PPBillImpExpBaseProcessBlock::fPaymOrdersExp|
						PPBillImpExpBaseProcessBlock::fChZnImpExp);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fUhttImport,   v == 1);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fEdiImpExp,    v == 2);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fEgaisImpExp,  v == 3);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fChZnImpExp,   v == 4);
				}
				else {
					_f &= ~(PPBillImpExpBaseProcessBlock::fEdiImpExp|PPBillImpExpBaseProcessBlock::fEgaisImpExp|
						PPBillImpExpBaseProcessBlock::fPaymOrdersExp|PPBillImpExpBaseProcessBlock::fChZnImpExp);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fEdiImpExp, v == 1);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fEgaisImpExp, v == 2);
					SETFLAG(_f, PPBillImpExpBaseProcessBlock::fPaymOrdersExp, v == 3);
				}
				P_Data->Flags = _f;
			}
		}
		PPIniFile * P_IniFile;
		int    Import;
		PPBillImpExpBaseProcessBlock * P_Data; // @notowned
		StrAssocArray HdrList;
		StrAssocArray LineList;
	};
	int    ok = -1;
	SString ini_file_name;
	SelectBillImpCfgDialog * dlg = 0;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		BillParam.Direction = BIN(import);
		BRowParam.Direction = BIN(import);
		THROW(CheckDialogPtr(&(dlg = new SelectBillImpCfgDialog((import ? DLG_RUNIE_BILL_IMP : DLG_RUNIE_BILL_EXP), ini_file_name, import))));
		{
			PrcssrAlcReport::Config parc;
			if(PrcssrAlcReport::ReadConfig(&parc) > 0) {
				SETFLAG(Flags, fEgaisVer3, BIN(parc.E.Flags & parc.fEgaisVer3Fmt));
				SETFLAG(Flags, fEgaisVer4, BIN(parc.E.Flags & parc.fEgaisVer4Fmt)); // @v11.0.12
			}
			else {
				Flags &= ~fEgaisVer3; // @v11.0.12 @fix (Flags &= fEgaisVer3)-->(Flags &= ~fEgaisVer3)
				Flags &= ~fEgaisVer4; // @v11.0.12
			}
		}
		THROW(dlg->setDTS(this));
		while(ok <= 0 && ExecView(dlg) == cmOK) {
			if(dlg->getDTS())
				ok = 1;
		}
	}
	CATCHZOK
	delete dlg;
	return ok;
}
//
//
//
PPBillImporter::PPBillImporter() : P_Cc(0), P_Btd(0)
{
	Init();
}

PPBillImporter::~PPBillImporter()
{
	SString path;
	Logger.Save(PPGetFilePathS(PPPATH_LOG, PPFILNAM_IMPEXP_LOG, path), 0);
	delete P_Btd;
	delete P_Cc;
}

void PPBillImporter::Init()
{
	PPBillImpExpBaseProcessBlock::Z();
	AccSheetID = 0;
	LineIdSeq = 0;
	Bills.freeAll();
	BillsRows.freeAll();
	Logger.Clear();
}

int PPBillImporter::LoadConfig(int import)
{
	int    ok = 1;
	SString name;
	SString ini_file_name;
	THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
	{
		PPIniFile ini_file(ini_file_name, 0, 1, 1);
		BillParam.Init(import);
		BRowParam.Init(import);
		THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		name = CfgNameBill;
		if(Flags & PPBillImpExpBaseProcessBlock::fEdiImpExp) {
			BillParam.BaseFlags |= PPImpExpParam::bfDLL;
			BillParam.Name = CfgNameBill;
		}
		else {
			BillParam.ProcessName(2, name); // @vmiller
			// @vmiller (impexp) {
			if(name.HasPrefixIAscii("DLL_"))
				BillParam.BaseFlags |= PPImpExpParam::bfDLL;
			// } @vmiller (impexp)
			BillParam.ProcessName(1, name);
			THROW(BillParam.ReadIni(&ini_file, name, 0));
			name = CfgNameBRow;
			BRowParam.ProcessName(2, name);
			BRowParam.ProcessName(1, name);
			THROW(BRowParam.ReadIni(&ini_file, name, 0));
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::InitUhttImport(PPID opID, PPID locID, PPID posNodeID)
{
	int    ok = 1;
	PPOprKind op_rec;
	Init();
	OpID = opID;
	LocID = NZOR(locID, LConfig.Location);
	PosNodeID = posNodeID;
	Flags |= fUhttImport;
	SETFLAG(Flags, fCreateCc, PosNodeID);
	if(Flags & fCreateCc) {
		SETIFZ(P_Cc, new CCheckCore);
	}
	THROW(OpID && LocID);
	THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
	THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
	if(GetOpData(OpID, &op_rec) > 0)
		AccSheetID = op_rec.AccSheetID;
	CATCHZOK
	return ok;
}

int PPBillImporter::Init(const PPBillImpExpParam * pBillParam, const PPBillImpExpParam * pBRowParam, PPID opID, PPID locID)
{
	int    ok = 1;
	PPOprKind op_rec;
	Init();
	OpID = opID;
	LocID = NZOR(locID, LConfig.Location);
	RVALUEPTR(BillParam, pBillParam);
	RVALUEPTR(BRowParam, pBRowParam);
	if(!pBillParam || !pBRowParam || !OpID) {
		THROW(pBillParam || LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		THROW(pBRowParam || LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		THROW(ok = Select(1));
		if(ok > 0 && GetOpData(OpID, &op_rec) > 0)
			AccSheetID = op_rec.AccSheetID;
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::Helper_EnsurePersonArticle(PPID psnID, PPID accSheetID, PPID psnKindID, PPID * pArID)
{
	int    ok = 1;
	PPID   ar_id = 0;
	ArticleTbl::Rec ar_rec;
	if(ArObj.P_Tbl->SearchObjRef(accSheetID, psnID, &ar_rec) > 0) {
		ar_id = ar_rec.ID;
	}
	else {
		if(PsnObj.P_Tbl->IsBelongToKind(psnID, psnKindID) <= 0)
			THROW(PsnObj.P_Tbl->AddKind(psnID, psnKindID, 0) > 0);
		THROW(ArObj.CreateObjRef(&(ar_id = 0), accSheetID, psnID, 0, 0) > 0);
	}
	ASSIGN_PTR(pArID, ar_id);
	CATCHZOK
	return ar_id;
}

int PPBillImporter::RunUhttImport()
{
	int    ok = -1;
	const  PPID loc_id = NZOR(LocID, LConfig.Location);
	int32  uhtt_loc_id = 0;
	SString clb_buf, serial_buf;
	SString fmt_buf, msg_buf;
	SString temp_buf;
	PPOprKind op_rec;
	PPObjAccSheet acs_obj;
	LocationTbl::Rec loc_rec;
	UhttLocationPacket uhtt_loc_pack;
	PPUhttClient uhtt_cli;
	THROW(GetOpData(OpID, &op_rec) > 0);
	THROW(uhtt_cli.Auth());
	THROW_PP(LocObj.Fetch(loc_id, &loc_rec) > 0, PPERR_LOCSYMBUNDEF);
	THROW_PP_S(loc_rec.Code[0], PPERR_LOCSYMBUNDEF, loc_rec.Name);
	THROW(uhtt_cli.GetLocationByCode(loc_rec.Code, uhtt_loc_pack) > 0);
	uhtt_loc_id = uhtt_loc_pack.ID;
	{
		TSCollection <UhttBillPacket> result_list;
		UhttBillFilter uhtt_filt;
		PPIDArray psn_list, ar_list, kind_list;
		uhtt_filt.Since = plusdate(getcurdate_(), -1);
		uhtt_filt.OpSymb = "DRAFTORDER";
		uhtt_filt.LocID = uhtt_loc_id;
		THROW(uhtt_cli.GetBill(uhtt_filt, result_list));
		for(uint i = 0; i < result_list.getCount(); i++) {
			const UhttBillPacket * p_uhtt_pack = result_list.at(i);
			if(p_uhtt_pack) {
				BillTbl::Rec same_bill_rec;
				if(!p_uhtt_pack->Uuid.IsZero() && P_BObj->SearchByGuid(p_uhtt_pack->Uuid, &same_bill_rec) > 0) {
					PPFormatT(PPTXT_LOG_IMPBILLFOUND, &msg_buf, same_bill_rec.ID);
					Logger.Log(msg_buf);
				}
				else {
					PPBillPacket pack;
					PPPersonPacket psn_pack;
					PPID   dlvr_loc_id = 0;
					PPLocationPacket dlvr_loc_pack;
					PPTransaction tra(1);
					THROW(tra);
					THROW(pack.CreateBlank(OpID, 0, loc_id, 0));
					pack.Rec.Dt = p_uhtt_pack->Dtm;
					if(!p_uhtt_pack->Uuid.IsZero()) {
						pack.SetGuid(p_uhtt_pack->Uuid);
					}
					if(p_uhtt_pack->DlvrLocID) {
						THROW(uhtt_cli.GetLocationByID(p_uhtt_pack->DlvrLocID, uhtt_loc_pack));
						uhtt_cli.ConvertLocationPacket(uhtt_loc_pack, dlvr_loc_pack);
						LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, dlvr_loc_pack.Code, &dlvr_loc_id, &dlvr_loc_pack);
					}
					if(op_rec.AccSheetID) {
   						PPAccSheet acs_rec;
						PPID   ar_id = 0;
						PPID   psn_id = 0;
						THROW(acs_obj.Fetch(op_rec.AccSheetID, &acs_rec) > 0);
						if(acs_rec.Assoc == PPOBJ_PERSON) {
							const  PPID psn_kind_id = acs_rec.ObjGroup;
							if(p_uhtt_pack->Contractor.ID) {
								//
								// С пакетом документа пришла информация о контрагенте
								//
								ar_list.clear();
								//
								// Попытка идентификации контрагента по ИНН
								//
								(temp_buf = p_uhtt_pack->Contractor.INN).Strip();
								THROW(PsnObj.GetListByRegNumber(PPREGT_TPID, 0, temp_buf, psn_list));
								if(psn_list.getCount()) {
									psn_id = psn_list.get(0);
									THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
								}
								if(!ar_id) {
									//
									// Попытка идентификации контрагента по коду персоналии Universe-HTT
									//
									p_uhtt_pack->Contractor.GetUhttContragentCode(temp_buf);
									THROW(PsnObj.GetListByRegNumber(PPREGT_UHTTCLID, 0, temp_buf, psn_list));
									if(psn_list.getCount()) {
										psn_id = psn_list.get(0);
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
								if(!ar_id) {
									//
									// Не нашли аналога персоналии в БД.
									// Создаем новую персоналию.
									//
									psn_pack.destroy();
									if(uhtt_cli.ConvertPersonPacket(p_uhtt_pack->Contractor, psn_kind_id, psn_pack) > 0) {
										THROW(PsnObj.PutPacket(&psn_id, &psn_pack, 0));
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
							}
							else {
								if(dlvr_loc_id) {
									PersonTbl::Rec dlvr_loc_owner_rec;
									if(dlvr_loc_pack.OwnerID && PsnObj.Search(dlvr_loc_pack.OwnerID, &dlvr_loc_owner_rec) > 0) {
										psn_id = dlvr_loc_owner_rec.ID;
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
								if(!psn_id) {
									if(!dlvr_loc_pack.IsEmptyAddress()) {
										PersonTbl::Rec temp_psn_rec;
										temp_buf = "Anonymous client Universe-HTT";
										kind_list.clear();
										psn_pack.destroy();
										kind_list.add(psn_kind_id);
										if(PsnObj.SearchFirstByName(temp_buf, &kind_list, 0, &temp_psn_rec) > 0) {
											psn_id = temp_psn_rec.ID;
											THROW(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0);
											dlvr_loc_pack.OwnerID = psn_id;
											psn_pack.AddDlvrLoc(dlvr_loc_pack);
										}
										else {
											temp_buf.CopyTo(psn_pack.Rec.Name, sizeof(psn_pack.Rec.Name));
											psn_pack.Rec.Status = PPPRS_LEGAL;
											psn_pack.Kinds.addUnique(psn_kind_id);
											psn_pack.AddDlvrLoc(dlvr_loc_pack);
										}
										THROW(PsnObj.PutPacket(&psn_id, &psn_pack, 0));
										THROW(Helper_EnsurePersonArticle(psn_id, op_rec.AccSheetID, psn_kind_id, &ar_id));
									}
								}
							}
							if(ar_id) {
								PPBillPacket::SetupObjectBlock sob;
								THROW(pack.SetupObject(ar_id, sob));
							}
							if(psn_id && !dlvr_loc_pack.IsEmptyAddress()) {
								//
								// Если с сервера был получен адрес доставки, то необходимо убедиться, что
								// адрес включен в список адресов персоналии и установить этот адрес во фрахт документа.
								//
								uint   dlvr_loc_idx = 0;
								PPLocationPacket _loc_pack;
								psn_pack.destroy();
								THROW(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0);
								for(uint j = 0; !dlvr_loc_idx && psn_pack.EnumDlvrLoc(&j, &_loc_pack);) {
									if(strcmp(_loc_pack.Code, dlvr_loc_pack.Code) == 0) {
										dlvr_loc_idx = j;
									}
								}
								if(!dlvr_loc_idx) {
									PPID   _psn_id = psn_pack.Rec.ID;
									dlvr_loc_pack.ID = 0;
									psn_pack.AddDlvrLoc(dlvr_loc_pack);
									THROW(PsnObj.PutPacket(&_psn_id, &psn_pack, 0));
									if(LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, dlvr_loc_pack.Code, &dlvr_loc_id, &dlvr_loc_pack) > 0)
										pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
								}
								else
									pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
							}
						}
					}
					if(!dlvr_loc_id && !dlvr_loc_pack.IsEmptyAddress()) {
						dlvr_loc_pack.Flags |= LOCF_STANDALONE;
						THROW(LocObj.PutPacket(&dlvr_loc_id, &dlvr_loc_pack, 0));
						pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
					}
					for(uint j = 0; j < p_uhtt_pack->Items.getCount(); j++) {
						const UhttBillPacket::BillItem & r_item = p_uhtt_pack->Items.at(j);
						UhttGoodsPacket uhtt_goods_pack;
						PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
						clb_buf.Z();
						serial_buf.Z();
						if(uhtt_cli.ResolveGoodsByUhttID(r_item.GoodsID, &uhtt_goods_pack, &ti.GoodsID, 0) > 0) {
							ti.Quantity_ = fabs(r_item.Quantity);
							ti.Price = fabs(r_item.Price);
							P_BObj->SetupImportedPrice(&pack, &ti, PPObjBill::sipfAllowZeroPrice);
							THROW(pack.LoadTItem(&ti, clb_buf, serial_buf));
						}
						else {
							// @log
						}
					}
					pack.SetQuantitySign(-1);
					THROW(P_BObj->__TurnPacket(&pack, 0, 0, 0));
					ok = 1;
					Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					if(Flags & fCreateCc && P_Cc && pack.GetTCount()) {
						PPObjCashNode cn_obj;
						PPCashNode cn_rec;
						if(cn_obj.Search(PosNodeID, &cn_rec) > 0) {
							CCheckPacket cc_pack;
							// @v10.8.7 @ctr cc_pack.Init();
							cc_pack.Rec.CashID = PosNodeID;
							// @v8.4.7 @fix (отложенный чек не должен быть привязан к сессии) cc_pack.Rec.SessID = cn_rec.CurSessID;
							cc_pack.Rec.Dt = p_uhtt_pack->Dtm;
							cc_pack.Rec.Tm = p_uhtt_pack->Dtm;
							SETIFZ(cc_pack.Rec.Dt, getcurdate_());
							SETIFZ(cc_pack.Rec.Tm, getcurtime_());
							PPObjPerson::GetCurUserPerson(&cc_pack.Rec.UserID, 0);
							cc_pack.Rec.Flags |= (CCHKF_DELIVERY|CCHKF_SYNC|CCHKF_SUSPENDED|CCHKF_IMPORTED);
							{
								CCheckTbl::Rec last_chk_rec;
								if(P_Cc->GetLastCheckByCode(PosNodeID, &last_chk_rec) > 0)
									cc_pack.Rec.Code = last_chk_rec.Code + 1;
							}
							if(dlvr_loc_id)
								cc_pack.Ext.AddrID = dlvr_loc_id;
							{
								temp_buf = "UHTT";
								/* @v11.1.12
								if(pack.Rec.Memo[0])
									temp_buf.Space().Cat(pack.Rec.Memo);
								*/
								// @v11.1.12 {
								if(pack.SMemo.NotEmpty())
									temp_buf.Space().Cat(pack.SMemo);
								// } @v11.1.12 
								temp_buf.CopyTo(cc_pack.Ext.Memo, sizeof(cc_pack.Ext.Memo));
							}
							PPTransferItem * p_ti;
							for(uint ln = 0; pack.EnumTItems(&ln, &p_ti);) {
								const double _qtty = fabs(p_ti->Quantity_);
								THROW(cc_pack.InsertItem(p_ti->GoodsID, _qtty, p_ti->NetPrice(), 0));
							}
							cc_pack.SetupAmount(0, 0);
							THROW(P_Cc->TurnCheck(&cc_pack, 0));
							ok += 100;
						}
					}
					THROW(tra.Commit());
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

const Sdr_Bill * PPBillImporter::SearchBillForRow(const SString & rBillIdent, const Sdr_BRow & rRow) const
{
	uint   p = 0;
	while(Bills.lsearch(rBillIdent, &p, PTR_CMPFUNC(Pchar))) {
		const long _int_bill_id = rBillIdent.ToLong(); // @debug
		const Sdr_Bill * p_bill = &Bills.at(p);
		if((!rRow.BillDate || rRow.BillDate == p_bill->Date) && (!rRow.INN[0] || sstreq(rRow.INN, p_bill->INN))) {
			return p_bill;
		}
		else
			p++;
	}
	return 0;
}

int PPBillImporter::SearchNextRowForBill(const Sdr_Bill & rBill, uint * pPos) const
{
    uint p = DEREFPTRORZ(pPos);
	while(BillsRows.lsearch(rBill.ID, &p, PTR_CMPFUNC(Pchar))) {
		const Sdr_BRow * p_row = &BillsRows.at(p);
		if((!p_row->BillDate || p_row->BillDate == rBill.Date) && (!p_row->INN[0] || sstreq(p_row->INN, rBill.INN))) {
			ASSIGN_PTR(pPos, p);
			return 1;
		}
		else
			p++;
	}
	return 0;
}

int PPBillImporter::AddBRow(Sdr_BRow & rRow, uint * pRowId)
{
	LineIdSeq++;
	rRow.LineId = LineIdSeq;
	ASSIGN_PTR(pRowId, rRow.LineId);
	return BillsRows.insert(&rRow) ? 1 : PPSetErrorSLib();
}

static int ResolveFormula(const char * pFormula, const SdRecord & rInrRec, double * pResult)
{
	int    ok = 0;
	double result = 0.0;
	if(isempty(pFormula))
		ok = -1;
	else {
		ImpExpExprEvalContext expr_ctx(rInrRec);
		if(PPExprParser::CalcExpression(pFormula, &result, 0, &expr_ctx) > 0)
			ok = 1;
	}
	ASSIGN_PTR(pResult, result);
	return ok;
}

int PPBillImporter::ProcessDynField(const SdRecord & rDynRec, uint dynFldN, PPImpExpParam & rIep, ObjTagList & rTagList)
{
	int    ok = -1;
	SdbField dyn_fld;
	if(rDynRec.GetFieldByPos(dynFldN, &dyn_fld) && dyn_fld.InnerFormula.NotEmptyS()) {
		char   temp_str_[1024];
		SString temp_buf;
		SdbField inner_fld;
		SdbField outer_fld;
		uint   inner_fld_pos = 0;
		uint   outer_fld_pos = 0;
		SStrScan scan;
		PPObjectTag tag_rec;
		double rval = 0.0;
		PPID   tag_id = 0;
		scan.Set(dyn_fld.InnerFormula, 0);
		if(scan.GetIdent(temp_buf.Z())) {
			if(temp_buf.IsEqiAscii("lottag")) {
				scan.Skip();
				if(scan[0] == '.') {
					scan.Incr(1);
					(temp_buf = scan).Strip();
					if(TagObj.SearchBySymb(temp_buf, &tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT) {
						int   r = 0;
						const TYPEID typ = dyn_fld.T.Typ;
						const int    base_typ = stbase(typ);
						ObjTagItem tag_item;
						if(base_typ == BTS_INT) {
							long ival = 0;
							sttobase(typ, rDynRec.GetDataC(dynFldN), &ival);
							if(ival) {
								tag_item.SetInt(tag_id, ival);
								r = 1;
							}
						}
						else if(base_typ == BTS_REAL) {
							sttobase(typ, rDynRec.GetDataC(dynFldN), &rval);
							if(rval != 0.0) {
								tag_item.SetReal(tag_id, rval);
								r = 1;
							}
						}
						else if(base_typ == BTS_STRING) {
							sttobase(typ, rDynRec.GetDataC(dynFldN), temp_str_);
							if(temp_str_[0]) {
								(temp_buf = temp_str_).Strip().Transf(CTRANSF_OUTER_TO_INNER);
								tag_item.SetStr(tag_id, temp_buf);
								r = 1;
							}
						}
						else if(base_typ == BTS_DATE) {
							LDATE dval = ZERODATE;
							sttobase(typ, rDynRec.GetDataC(dynFldN), &dval);
							if(checkdate(dval)) {
								tag_item.SetDate(tag_id, dval);
								r = 1;
							}
						}
						if(r) {
							rTagList.PutItem(tag_id, &tag_item);
						}
					}
				}
			}
			else if(temp_buf.IsEqiAscii("costformula")) {
				scan.Skip();
				if(scan[0] == '.') {
					scan.Incr(1);
					const int fr = ResolveFormula((temp_buf = scan).Strip(), rIep.InrRec, &rval);
					if(fr > 0) {
						if(rval > 0.0) {
							//brow_.Cost = rval;
							if(rIep.InrRec.SearchName("Cost", &inner_fld_pos)) {
								rIep.InrRec.GetFieldByPos(inner_fld_pos, &inner_fld);
								void * p_inner_fld_data = rIep.InrRec.GetData(inner_fld_pos);
								stcast(MKSTYPE(S_FLOAT, 8), inner_fld.T.Typ, &rval, p_inner_fld_data, 0);
							}
						}
						else
							Logger.LogString(PPTXT_BROWCOSTFORMINVRES, temp_buf);
					}
					else if(fr == 0)
						Logger.LogString(PPTXT_BROWCOSTFORMINV, temp_buf);
				}
			}
			else if(temp_buf.IsEqiAscii("priceformula")) {
				scan.Skip();
				if(scan[0] == '.') {
					scan.Incr(1);
					const int fr = ResolveFormula((temp_buf = scan).Strip(), rIep.InrRec, &rval);
					if(fr > 0) {
						if(rval > 0.0) {
							//brow_.Price = rval;
							if(rIep.InrRec.SearchName("Price", &inner_fld_pos)) {
								rIep.InrRec.GetFieldByPos(inner_fld_pos, &inner_fld);
								void * p_inner_fld_data = rIep.InrRec.GetData(inner_fld_pos);
								stcast(MKSTYPE(S_FLOAT, 8), inner_fld.T.Typ, &rval, p_inner_fld_data, 0);
							}
						}
						else
							Logger.LogString(PPTXT_BROWPRICEFORMINVRES, temp_buf);
					}
					else if(fr == 0)
						Logger.LogString(PPTXT_BROWPRICEFORMINV, temp_buf);
				}
			}
			// @erik v10.5.2 {
			else if(temp_buf.IsEqiAscii("qttyformula")){
				scan.Skip();
				if(scan[0] == '.') {
					scan.Incr(1);
					const int fr = ResolveFormula((temp_buf = scan).Strip(), rIep.InrRec, &rval);
					if(fr > 0) {
						if(rval > 0.0) {
							//brow_.Price = rval;
							if(rIep.InrRec.SearchName("Quantity", &inner_fld_pos)) {
								rIep.InrRec.GetFieldByPos(inner_fld_pos, &inner_fld);
								void * p_inner_fld_data = rIep.InrRec.GetData(inner_fld_pos);
								stcast(MKSTYPE(S_FLOAT, 8), inner_fld.T.Typ, &rval, p_inner_fld_data, 0);
							}
						}
						else
							Logger.LogString(PPTXT_BROWQUANTITYFORMINVRES, temp_buf);
					}
					else if(fr == 0)
						Logger.LogString(PPTXT_BROWQUANTITYFORMINV, temp_buf);
				}
			}
			// } @erik
			else {
				enum {
					dfkFormula = 1, // formula[inner_field].formula
					dfkToken,       // token[inner_field].outer_field divisor number (token[CntragID].CLIENTID_DISTRIB _ 1)
					dfkIdentByTag   // identbytag[inner_field].outer_field tag_symbol
				};
				const int dfk = (temp_buf.IsEqiAscii("formula") ? dfkFormula :
					(temp_buf.IsEqiAscii("token") ? dfkToken : (temp_buf.IsEqiAscii("identbytag") ? dfkIdentByTag : 0)));
				if(dfk && scan.Skip()[0] == '[') {
					scan.Incr(1);
					if(scan.Skip().GetIdent(temp_buf) && scan.Skip()[0] == ']' && scan.Skip()[1] == '.') {
						uint   fld_pos = 0;
						SString in_fld_name = temp_buf.Strip();
						scan.Incr(2);
						SString form_buf;
						if((form_buf = scan).NotEmptyS() && rIep.InrRec.SearchName(in_fld_name, &fld_pos)) {
							rIep.InrRec.GetFieldByPos(fld_pos, &inner_fld);
							if(dfk == dfkFormula) {
								if(btnumber(stbase(inner_fld.T.Typ))) {
									ImpExpExprEvalContext expr_ctx(rIep.InrRec);
									if(PPExprParser::CalcExpression(form_buf, &rval, 0, &expr_ctx) > 0)
										stcast(T_DOUBLE, inner_fld.T.Typ, &rval, rIep.InrRec.GetData(fld_pos), 0);
									else
										Logger.LogString(PPTXT_BROWFORMINVRES, form_buf);
								}
							}
							else if(dfk == dfkIdentByTag) {
								SStrScan arg_scan(form_buf);
								arg_scan.Skip();
								if(arg_scan.GetIdent(temp_buf) && rIep.OtrRec.SearchName(temp_buf.Strip(), &outer_fld_pos)) {
									arg_scan.Skip();
									(temp_buf = arg_scan).Strip();
									if(TagObj.SearchBySymb(temp_buf, &tag_id, &tag_rec) > 0 && rIep.OtrRec.GetFieldByPos(outer_fld_pos, &outer_fld)) {
										PPIDArray obj_id_list;
										const void * p_outer_fld_data = rIep.OtrRec.GetDataC(outer_fld_pos);
										PTR32(temp_str_)[0] = 0;
										sttostr(outer_fld.T.Typ, p_outer_fld_data, 0, temp_str_);
										temp_buf = temp_str_;
										PPRef->Ot.SearchObjectsByStrExactly(tag_rec.ObjTypeID, tag_rec.ID, temp_buf, &obj_id_list);
										if(obj_id_list.getCount()) {
											const long obj_id = obj_id_list.get(0);
											void * p_inner_fld_data = rIep.InrRec.GetData(fld_pos);
											stcast(MKSTYPE(S_INT, 4), inner_fld.T.Typ, &obj_id, p_inner_fld_data, 0);
										}
									}
								}
							}
							else if(dfk == dfkToken) {
								// fld divider token_number[1..]
								SString tok_result_buf;
								tok_result_buf.Z();
								SStrScan arg_scan(form_buf);
								arg_scan.Skip();
								if(arg_scan.GetIdent(temp_buf) && rIep.OtrRec.SearchName(temp_buf.Strip(), &outer_fld_pos)) {
									SString foreign_fld = temp_buf;
									SString dividers;
									do {
										arg_scan.Skip();
										if(arg_scan.Is("space")) {
											dividers.Space();
											arg_scan.Incr(sstrlen("space"));
										}
										else if(arg_scan.Is("tab")) {
											dividers.Tab();
											arg_scan.Incr(sstrlen("tab"));
										}
										else if(arg_scan.Is("\\t")) {
											dividers.Tab();
											arg_scan.Incr(sstrlen("\\t"));
										}
										else if(oneof11(arg_scan[0], '_', ',', ';', '.', '-', ':', '%', '^', '$', '@', '#')) {
											dividers.CatChar(arg_scan[0]);
											arg_scan.Incr();
										}
										else
											break;
									} while(1);
									if(dividers.NotEmpty() && arg_scan.GetNumber(temp_buf)) {
										const long tok_n = temp_buf.ToLong();
										if(tok_n > 0 && rIep.OtrRec.GetFieldByPos(outer_fld_pos, &outer_fld)) {
											const void * p_outer_fld_data = rIep.OtrRec.GetDataC(outer_fld_pos);
											PTR32(temp_str_)[0] = 0;
											sttostr(outer_fld.T.Typ, p_outer_fld_data, 0, temp_str_);
											temp_buf = temp_str_;
											StringSet ss_tok;
											temp_buf.Tokenize(dividers, ss_tok);
											long   t_ = 0;
											for(uint ssp = 0; ss_tok.get(&ssp, temp_buf);) {
												t_++;
												if(t_ == tok_n) {
													tok_result_buf = temp_buf;
													break;
												}
											}
										}
									}
								}
								stcast(MKSTYPE(S_ZSTRING, tok_result_buf.Len()+1), inner_fld.T.Typ, (void *)tok_result_buf.cptr(), rIep.InrRec.GetData(fld_pos), 0);
							}
						}
					}
				}
			}
		}
	}
	return ok;
}

static uint SplitBarcodeList(const char * pSrcBuf, SString & rPlainResult, StringSet & rSs)
{
	uint    result = 0; // count of codes
	rSs.Z();
	rPlainResult.Z();
	if(!isempty(pSrcBuf)) {
		(rPlainResult = pSrcBuf).Chomp().TrimRightChr(';').TrimRightChr(',').Strip();
		if(rPlainResult.NotEmpty()) {
			SString temp_buf;
			rPlainResult.Transf(CTRANSF_OUTER_TO_INNER);
			if(rPlainResult.HasChr(';')) {
				StringSet ss(';', rPlainResult);
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					temp_buf.Chomp();
					if(temp_buf.NotEmptyS() && temp_buf.IsDigit()) {
						rSs.add(temp_buf);
						result++;
					}
				}
			}
			else if(rPlainResult.HasChr(',')) {
				StringSet ss(',', rPlainResult);
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					temp_buf.Chomp();
					if(temp_buf.NotEmptyS() && temp_buf.IsDigit()) {
						rSs.add(temp_buf);
						result++;
					}
				}
			}
			else {
				rSs.add(rPlainResult);
				result++;
			}
		}
	}
	return result;
}

int PPBillImporter::ReadRows(PPImpExp * pImpExp, int mode/*linkByLastInsBill*/, const /*StrAssocArray*/PPImpExpParam::PtTokenList * pFnFldList)
{
	int    ok = 1;
	long   count = 0;
	const  LDATE cdate = getcurdate_();
	PPID   ar_id_by_code2 = 0;
	SString def_inn;
	SString temp_buf;
	SString inn;
	SString bill_ident;
	SString barcode_buf; // @v11.4.5
	StringSet barcode_list; // @v11.4.5
	StrAssocArray articles;
	//PPObjTag tag_obj;
	SdRecord dyn_rec;
	SdbField dyn_fld, inner_fld;
	THROW_INVARG(pImpExp);
	THROW(pImpExp->InitDynRec(&dyn_rec));
	pImpExp->GetNumRecs(&count);
	def_inn = BillParam.Object1SrchCode;
	if(BillParam.Object2SrchCode.NotEmpty()) {
		PPID   reg_type_id = PPREGT_TPID;
		SString code = BillParam.Object2SrchCode;
		const PPID suppl_acs_id = GetSupplAccSheet();
		if(PPObjArticle::GetSearchingRegTypeID(suppl_acs_id, 0, 1, &reg_type_id) > 0) {
			PPIDArray psn_list, ar_list;
			PsnObj.GetListByRegNumber(reg_type_id, 0, code, psn_list);
			ArObj.GetByPersonList(suppl_acs_id, &psn_list, &ar_list);
			if(ar_list.getCount() == 1)
				ar_id_by_code2 = ar_list.get(0);
		}
	}
	for(long i = 0; i < count; i++) {
		uint   p = 0;
		PPID   id = 0;
		Sdr_BRow brow_;
		BarcodeTbl::Rec bcrec;
		// @v10.7.9 @ctr MEMSZERO(brow_);
		// @v10.7.9 @ctr MEMSZERO(bcrec);
		if(pFnFldList) {
			AssignFnFieldToRecord(*pFnFldList, 0, &brow_);
		}
		THROW(pImpExp->ReadRecord(&brow_, sizeof(brow_), &dyn_rec));
		// @v10.9.4 {
		(temp_buf = brow_.BillCode).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.BillCode, temp_buf);
		(temp_buf = brow_.BillID).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.BillID, temp_buf);
		// } @v10.9.4
		const uint barcode_count = SplitBarcodeList(brow_.Barcode, barcode_buf, barcode_list); // @v11.4.5
		STRNSCPY(brow_.Barcode, barcode_buf);
		(temp_buf = brow_.GoodsName).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.GoodsName, temp_buf);
		// @v10.5.0 {
		(temp_buf = brow_.GoodsGroup).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.GoodsGroup, temp_buf);
		// } @v10.5.0
		// @v10.5.3 {
		(temp_buf = brow_.BrandName).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.BrandName, temp_buf);
		// } @v10.5.3
		// @v11.4.4 {
		(temp_buf = brow_.ArCode).Transf(CTRANSF_OUTER_TO_INNER);
		STRNSCPY(brow_.ArCode, temp_buf);
		// } @v11.4.4
		if(mode == 1/*linkByLastInsBill*/)
			STRNSCPY(brow_.BillID, Bills.at(Bills.getCount() - 1).ID);
		else if(mode == 2) {
			if(isempty(brow_.BillID))
				STRNSCPY(brow_.BillID, brow_.BillCode);
			SETIFZ(brow_.BillDate, brow_.InvcDate);
			SETIFZ(brow_.BillDate, brow_.DueDate);
			SETIFZ(brow_.BillDate, brow_.PaymDate);
			SETIFZ(brow_.BillDate, cdate);
			if(!Period.CheckDate(brow_.BillDate))
				continue;
		}
		(bill_ident = brow_.BillID).Strip();
		if(mode != 2 && bill_ident.IsEmpty()) { // нет сопоставления для строки документа
			SString msg, file_name, goods_info, word;
			pImpExp->GetFileName(file_name);
			SPathStruc sp(file_name);
			(file_name = sp.Nam).Dot().Cat(sp.Ext);
			PPLoadText(PPTXT_BROWLINKNOTFOUND, temp_buf);
			if(brow_.Barcode[0]) {
				PPLoadString("barcode", goods_info);
				goods_info.Space().Cat(brow_.Barcode).Space();
			}
			PPLoadString("ware", word);
			goods_info.Cat(word).Space().Cat(brow_.GoodsName);
			msg.Printf(temp_buf, file_name.cptr(), goods_info.cptr());
			Logger.Log(msg);
		}
		else { //if(mode == 2 || Bills.lsearch(bill_ident, &(p = 0), PTR_CMPFUNC(Pchar)) > 0) { // bsearch->lsearch
			const Sdr_Bill * p_bill = (mode == 2) ? 0 : SearchBillForRow(bill_ident, brow_);
			if(mode == 2 || p_bill) {
				uint   pos = 0;
				PPID   goods_id = 0;
				Goods2Tbl::Rec goods_rec;
				ArGoodsCodeTbl::Rec code_rec;
				if(brow_.GoodsID && GObj.Fetch(brow_.GoodsID, &goods_rec) > 0 && goods_rec.Kind == PPGDSK_GOODS) {
					goods_id = goods_rec.ID;
				}
				else if(CConfig.Flags & CCFLG_USEARGOODSCODE && (brow_.Barcode[0] || brow_.ArCode[0])) {
					PPID   ar_id = 0;
					ArticleTbl::Rec ar_rec;
					// @v10.7.9 @ctr MEMSZERO(code_rec);
					if(brow_.CntragID && ArObj.Fetch(brow_.CntragID, &ar_rec) > 0) {
						ar_id = brow_.CntragID;
					}
					else {
						inn = p_bill ? p_bill->INN : brow_.INN;
						if(!inn.NotEmptyS())
							inn = def_inn;
						if(articles.SearchByText(inn, 0, &pos) > 0)
							ar_id = articles.Get(pos).Id;
						else if(ResolveINN(inn, (p_bill ? p_bill->DlvrAddrID : 0), (p_bill ? p_bill->DlvrAddrCode : 0),
							bill_ident, AccSheetID, &ar_id, 0) > 0 && ar_id > 0) {
							THROW_SL(articles.Add(ar_id, inn, 0));
						}
					}
					if(ar_id > 0) {
						brow_.CntragID = ar_id;
						if(brow_.ArCode[0] && GObj.P_Tbl->SearchByArCode(ar_id, brow_.ArCode, &code_rec) > 0)
							goods_id = code_rec.GoodsID;
						else if(brow_.Barcode[0] && GObj.P_Tbl->SearchByArCode(ar_id, brow_.Barcode, &code_rec) > 0)
							goods_id = code_rec.GoodsID;
					}
				}
				if(!goods_id) {
					// @v11.4.5 {
					if(barcode_count) {
						for(uint bcidx = 0, ssp = 0; !goods_id && bcidx < barcode_count; bcidx++) {
							const int blgr = barcode_list.get(&ssp, temp_buf);
							assert(blgr);
							if(temp_buf.NotEmptyS() && GObj.SearchByBarcode(temp_buf, &bcrec, 0, 1/*adoptSearching*/) > 0)
								goods_id = bcrec.GoodsID;
						}
					}
					if(!goods_id && !(BillParam.Flags & BillParam.fDontIdentGoodsByName) && GObj.SearchByName(brow_.GoodsName, &id) > 0) // @v10.5.0 fDontIdentGoodsByName
						goods_id = id;
					// } @v11.4.5 
					#if 0 // @v11.4.5 {
					if(brow_.Barcode[0] && GObj.SearchByBarcode(brow_.Barcode, &bcrec, 0, 1/*adoptSearching*/) > 0)
						goods_id = bcrec.GoodsID;
					else if(!(BillParam.Flags & BillParam.fDontIdentGoodsByName) && GObj.SearchByName(brow_.GoodsName, &id) > 0) // @v10.5.0 fDontIdentGoodsByName
						goods_id = id;
					#endif // } 0 @v11.4.5
				}
				if(!goods_id && ar_id_by_code2 && brow_.ArCode[0]) {
					if(GObj.P_Tbl->SearchByArCode(ar_id_by_code2, brow_.ArCode, &code_rec) > 0)
						goods_id = code_rec.GoodsID;
				}
				brow_.GoodsID = goods_id;
				ObjTagList tag_list;
				tag_list.ObjType = PPOBJ_LOT;
				if(dyn_rec.GetCount()) {
					for(uint j = 0; j < dyn_rec.GetCount(); j++) {
						ProcessDynField(dyn_rec, j, pImpExp->GetParam(), tag_list);
					}
				}
				{
					uint    row_id = 0;
					THROW(AddBRow(brow_, &row_id));
					if(tag_list.GetCount())
						TagC.Set(row_id, &tag_list);
				}
			}
		}
		if(mode != 1 /* !linkByLastInsBill*/)
			PPWaitPercent(i + 1, count);
	}
	if(mode == 2) {
		// @v8.7.1 BillsRows.sort(PTR_CMPFUNC(Sdr_BRow));
	}
	else {
		BillsRows.sort(PTR_CMPFUNC(Sdr_BRow_ID));
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::ReadSpecXmlData()
{
	int    ok = 1;
	uint   p = 0;
	long   count = 0;
	long   row_rec_count = 0;
	StrAssocArray articles;
	PPImpExp ie(&BillParam, 0);
	PPImpExp * p_ie_row = 0;
	THROW(ie.OpenFileForReading(0));
	p_ie_row = &ie;
	ie.GetNumRecs(&count);
	for(long i = 0; i < count; i++) {
		Sdr_Bill bill;
		// @v10.7.9 @ctr MEMSZERO(bill);
		THROW(ie.ReadRecord(&bill, sizeof(bill)));
		SETIFZ(bill.Date, getcurdate_());
		if(CheckBill(&bill)) {
            int    r = AddBillToList(&bill, 0);
            THROW(r);
            if(r > 0) {
				ie.Push(&BRowParam);
				THROW(ReadRows(&ie, 1, 0));
				ie.Pop();
			}
		}
		PPWaitPercent(i + 1, count);
	}
	ie.CloseFile();
	CATCHZOK
	return ok;
}
/*
InitImport
GetErrorMessage
GetImportObj
ReplyImportObjStatus
InitImportIter
NextImportIter
FinishImpExp
*/
int PPBillImporter::AddBillToList(Sdr_Bill * pBill, long extraBillId)
{
	int    ok = 1;
	int    found = 0;
	assert(pBill);
	THROW_INVARG(pBill);
	for(uint j = 0; ok > 0 && j < Bills.getCount(); j++) {
		const Sdr_Bill & r_rec = Bills.at(j);
		if(pBill->Date == r_rec.Date && (!pBill->Code[0] || stricmp(pBill->Code, r_rec.Code) == 0) && (!pBill->ID[0] || stricmp(pBill->ID, r_rec.ID) == 0))
			ok = -1;
	}
	if(ok > 0) {
		SString bid(pBill->ID);
		if(!bid.NotEmptyS() && pBill->Code[0] != 0)
			(bid = pBill->Code).Strip();
		else if(pBill->Code[0] == 0 && bid.NotEmptyS()) {
			STRNSCPY(pBill->Code, bid);
		}
		else if(!bid.NotEmptyS() && extraBillId)
			(bid = "@docimp").Cat(extraBillId);
		STRNSCPY(pBill->ID, bid);
		SETIFZ(pBill->Date, getcurdate_());
		THROW_SL(Bills.insert(pBill));
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::AssignFnFieldToRecord(const /*StrAssocArray*/PPImpExpParam::PtTokenList & rFldList, Sdr_Bill * pRecHdr, Sdr_BRow * pRecRow)
{
	int    ok = 1;
	if(pRecHdr || pRecRow) {
		//
		// Гарантируем не нулевые указатели pRecHdr и pRecRow (дабы упростить последующий код)
		//
		Sdr_Bill hdr_stub;
		Sdr_BRow row_stub;
		SString token_text;
        SETIFZ(pRecHdr, &hdr_stub);
        SETIFZ(pRecRow, &row_stub);
		for(uint i = 0; i < rFldList.GetCount(); i++) {
			//StrAssocArray::Item item = rFldList.at_WithoutParent(i);
			long   token_id = 0;
			long   ext_id = 0;
			if(rFldList.Get(i, &token_id, &ext_id, token_text)) {
				LDATE dt = ZERODATE;
				switch(token_id) {
					case PPSYM_BILLNO:
						STRNSCPY(pRecHdr->Code, token_text);
						STRNSCPY(pRecRow->BillCode, token_text);
						break;
					case PPSYM_DATE:
						{
							strtodate(token_text, DATF_DMY, &dt);
							pRecHdr->Date = dt;
							pRecRow->BillDate = dt;
						}
						break;
					case PPSYM_DUEDATE: // @v10.4.8
						{
							strtodate(token_text, DATF_DMY, &dt);
							if(checkdate(dt)) {
								pRecHdr->DueDate = dt;
								pRecRow->DueDate = dt;
							}
						}
						break;
					case PPSYM_FGDATE:
						{
							strtodate(token_text, DATF_DMY|DATF_CENTURY|DATF_NODIV, &dt);
							pRecHdr->Date = dt;
							pRecRow->BillDate = dt;
						}
						break;
					case PPSYM_FGDUEDATE: // @v10.4.8
						{
							strtodate(token_text, DATF_DMY|DATF_CENTURY|DATF_NODIV, &dt);
							if(checkdate(dt)) {
								pRecHdr->DueDate = dt;
								pRecRow->DueDate = dt;
							}
						}
						break;
					case PPSYM_PAYDATE:
						{
							strtodate(token_text, DATF_DMY, &dt);
							pRecHdr->PaymDate = dt;
							pRecRow->PaymDate = dt;
						}
						break;
					case PPSYM_INVOICEDATE:
						{
							strtodate(token_text, DATF_DMY, &dt);
							pRecHdr->InvoiceDate = dt;
							pRecRow->InvcDate = dt;
						}
						break;
					case PPSYM_INVOICENO:
						STRNSCPY(pRecHdr->InvoiceCode, token_text);
						STRNSCPY(pRecRow->InvcCode, token_text);
						break;
					case PPSYM_LOCCODE:
						STRNSCPY(pRecHdr->LocCode, token_text);
						STRNSCPY(pRecRow->LocCode, token_text);
						break;
					case PPSYM_DLVRLOCCODE:
						STRNSCPY(pRecHdr->DlvrAddrCode, token_text);
						STRNSCPY(pRecRow->DlvrAddrCode, token_text);
						break;
					case PPSYM_DLVRLOCID:
						pRecHdr->DlvrAddrID = token_text.ToLong();
						pRecRow->DlvrAddrID = token_text.ToLong();
						break;
					case PPSYM_DLVRLOCTAG: // @v10.4.1
						if(!isempty(token_text)) {
							PPObjectTag tag_rec;
							if(TagObj.Fetch(ext_id, &tag_rec) > 0) {
								if(tag_rec.ObjTypeID == PPOBJ_LOCATION) {
									PPIDArray loc_list;
									if(PPRef->Ot.SearchObjectsByStr(PPOBJ_LOCATION, tag_rec.ID, token_text, &loc_list) > 0) {
										assert(loc_list.getCount() > 0);
										if(loc_list.getCount()) {
											pRecHdr->DlvrAddrID = loc_list.get(0);
											pRecRow->DlvrAddrID = loc_list.get(0);
										}
									}
								}
							}
						}
						break;
					case PPSYM_INN:
						STRNSCPY(pRecHdr->INN, token_text);
						STRNSCPY(pRecRow->INN, token_text);
						break;
					case PPSYM_DUMMY:
						break;
					default:
						break;
				}
			}
		}
	}
	return ok;
}

int PPBillImporter::ReadData()
{
	int    ok = -1;
	int    r = -1;
	char   _err_buf[1024];
	_err_buf[0] = 0;
	ImpExpDll imp_dll;
	SString err_msg, path, wildcard, filename, temp_buf;
	StringSet ss_files;
	Sdr_Bill bill;
	SPathStruc ps;
	const int h_r_eq_f = (BillParam.DataFormat == BRowParam.DataFormat) ? BillParam.DataFormat : -1;
	const int imp_rows_from_same_file = BIN(BillParam.Flags & PPBillImpExpParam::fImpRowsFromSameFile &&
		oneof4(h_r_eq_f, PPImpExpParam::dfText, PPImpExpParam::dfDbf, PPImpExpParam::dfExcel, PPImpExpParam::dfXml));
	const  int imp_rows_only = BIN(imp_rows_from_same_file && BillParam.Flags & PPBillImpExpParam::fImpExpRowsOnly);
	if(BillParam.BaseFlags & PPImpExpParam::bfDLL) {
		int    obj_id = 0, sess_id = 0;
		Sdr_BRow brow;
		BillParam.ImpExpParamDll.FileName = BillParam.FileName;
		THROW_SL(imp_dll.InitLibrary(BillParam.ImpExpParamDll.DllPath, 2));
		Sdr_ImpExpHeader hdr;
		// @v10.7.9 @ctr MEMSZERO(hdr);
		hdr.PeriodLow = Period.low;
		hdr.PeriodUpp = Period.upp;
		BillParam.ImpExpParamDll.Login.CopyTo(hdr.EdiLogin, sizeof(hdr.EdiLogin));
		BillParam.ImpExpParamDll.Password.CopyTo(hdr.EdiPassword, sizeof(hdr.EdiPassword));
		BillParam.ImpExpParamDll.OperType.CopyTo(hdr.EdiDocType, sizeof(hdr.EdiDocType));
		if(!imp_dll.InitImport(&hdr, BillParam.ImpExpParamDll.FileName, &sess_id)) {
			imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
			CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
		}
		//
		// Выполняется, пока есть документы для импорта
		//
		while((r = imp_dll.GetImportObj(sess_id, "BILLS", &bill, &obj_id, BillParam.ImpExpParamDll.OperType)) == 1) {
			Sdr_DllImpObjStatus imp_obj_stat;
			// @v10.7.9 @ctr MEMSZERO(imp_obj_stat);
			// Проверка документа и заполнение Sdr_ImpObjStatus
			GetDocImpStatus(&bill, imp_obj_stat);
			imp_dll.ReplyImportObjStatus(sess_id, obj_id, &imp_obj_stat);
			if(imp_obj_stat.DocStatus == statIsSuchDoc) {
				int    edi_op = 0;
				SString ordresp_cmd, desadv_cmd;
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_ORDRSP, ordresp_cmd));
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_DESADV, desadv_cmd));
				if(BillParam.ImpExpParamDll.OperType.IsEqiAscii("ALCODESADV"))
					edi_op = PPEDIOP_ALCODESADV;
				else if(ordresp_cmd.IsEqNC(BillParam.ImpExpParamDll.OperType))
					edi_op = PPEDIOP_ORDERRSP;
				else if(desadv_cmd.IsEqNC(BillParam.ImpExpParamDll.OperType))
					edi_op = PPEDIOP_DESADV;
				if(edi_op == PPEDIOP_ALCODESADV) {
					THROW(AddBillToList(&bill, obj_id));
					if(imp_dll.InitImportIter(sess_id, obj_id)) {
						while(imp_dll.NextImportIter(sess_id, obj_id, &brow) == 1) {
							uint   p = 0;
							PPID   id = 0;
							BarcodeTbl::Rec bcrec;
							if(brow.Barcode[0] && GObj.SearchByBarcode(brow.Barcode, &bcrec, 0, 1) > 0)
								brow.GoodsID = bcrec.GoodsID;
							else if(GObj.SearchByName(brow.GoodsName, &id) > 0)
								brow.GoodsID = id;
							STRNSCPY(brow.BillID, bill.ID);
							STRNSCPY(brow.BillCode, bill.Code);
							{
								uint   row_id = 0;
								THROW(AddBRow(brow, &row_id));
								ok = 1;
							}
						}
					}
					else {
						imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
						CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
					}
				}
				else if(oneof2(edi_op, PPEDIOP_ORDERRSP, PPEDIOP_DESADV)) {
					if(CheckBill(&bill)) {
						bill.EdiOp = edi_op;
						THROW(AddBillToList(&bill, obj_id));
					}
					if(imp_dll.InitImportIter(sess_id, obj_id)) {
						while(imp_dll.NextImportIter(sess_id, obj_id, &brow) == 1) {
							uint   p = 0;
							PPID   id = 0;
							BarcodeTbl::Rec bcrec;
							if(brow.Barcode[0] && GObj.SearchByBarcode(brow.Barcode, &bcrec, 0, 1) > 0)
								brow.GoodsID = bcrec.GoodsID;
							else if(GObj.SearchByName(brow.GoodsName, &id) > 0)
								brow.GoodsID = id;
							STRNSCPY(brow.BillID, bill.ID);
							STRNSCPY(brow.BillCode, bill.Code);
							{
								uint   row_id = 0;
								THROW(AddBRow(brow, &row_id));
								ok = 1;
							}
						}
					}
					else {
						imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
						CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
					}
				}
			}
		}
		if(!r) {
			imp_dll.GetErrorMessage(_err_buf, sizeof(_err_buf));
			CALLEXCEPT_PP_S(PPERR_IMPEXP_DLL, (err_msg = _err_buf).Transf(CTRANSF_OUTER_TO_INNER));
		}
		THROW(r);
		THROW(imp_dll.FinishImpExp());
	}
	else if(h_r_eq_f == PPImpExpParam::dfXml && BillParam.FileName.IsEqNC(BRowParam.FileName) && !imp_rows_only) {
		THROW(BillParam.PreprocessImportFileSpec(ss_files));
		for(uint ssp = 0; ss_files.get(&ssp, filename);) {
			uint   p = 0;
			long   count = 0;
			long   row_rec_count = 0;
			StrAssocArray articles;
			PPImpExp ie(&BillParam, 0);
			PPImpExp * p_ie_row = 0;
			PPImpExpParam::PtTokenList fn_fld_list;
			BillParam.PreprocessImportFileName(filename, fn_fld_list);
			THROW(ie.OpenFileForReading(filename));
			p_ie_row = &ie;
			ie.GetNumRecs(&count);
			for(long i = 0; i < count; i++) {
				MEMSZERO(bill);
				AssignFnFieldToRecord(fn_fld_list, &bill, 0);
				THROW(ie.ReadRecord(&bill, sizeof(bill)));
				SETIFZ(bill.Date, getcurdate_());
				if(CheckBill(&bill)) {
					int   ir = AddBillToList(&bill, 0);
					THROW(ir);
					if(ir > 0) {
						ie.Push(&BRowParam);
						THROW(ReadRows(&ie, 1, &fn_fld_list));
						ie.Pop();
					}
				}
				PPWaitPercent(i + 1, count);
			}
			ie.CloseFile();
			if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
				ToRemoveFiles.add(filename);
			}
			ok = 1;
		}
	}
	else {
		uint   p = 0;
		long   count = 0;
		PPImpExp ie(&BillParam, 0);
		PPImpExp ie_row(&BRowParam, 0);
		SString bid;
		BillsRows.freeAll();
		THROW(BillParam.PreprocessImportFileSpec(ss_files));
		ss_files.sortAndUndup();
		for(uint ssp = 0, fi = 0; ss_files.get(&ssp, filename); fi++) {
			PPImpExpParam::PtTokenList fn_fld_list;
			BillParam.PreprocessImportFileName(filename, fn_fld_list);
			if(imp_rows_only) {
				SdrBillRowArray preserve_rows = BillsRows;
				BillsRows.freeAll();
				THROW(ie_row.OpenFileForReading(filename));
				THROW(ReadRows(&ie_row, 2, &fn_fld_list));
				ie_row.CloseFile();
				//@erik v10.6.11 {
				if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
					ToRemoveFiles.add(filename);
				}
				// } @erik
				{
					SString fn_for_hash;
					ps.Split(filename);
					ps.Merge(SPathStruc::fNam|SPathStruc::fExt, fn_for_hash);
					fn_for_hash.Strip().ToLower();
					LDATE last_date = ZERODATE;
					SString last_code;
					SString bill_code;
					SString last_ident;
					SString bill_ident;
					long   cc_ = 0;
					LongArray row_idx_list_to_remove; // Список строк, которые надо удалить из-за того, что они принадлежат дубликатам документов
					for(uint ln = 0; ln < BillsRows.getCount(); ln++) {
						Sdr_BRow & r_row = BillsRows.at(ln);
						(bill_ident = r_row.BillID).Strip();
						(bill_code = r_row.BillCode).Strip();
						const SString org_bill_ident = bill_ident;
						const SString org_bill_code = bill_code;
						const int new_bill = BIN(r_row.BillDate != last_date || last_code != org_bill_code || last_ident != org_bill_ident);
						if(new_bill)
							cc_++;
						if(bill_ident.IsEmpty())
							bill_ident.Z().CatLongZ(cc_, 6);
						bill_ident.CopyTo(r_row.BillID, sizeof(r_row.BillID));
						//
						if(bill_code.IsEmpty()) {
							bill_code.Z().Cat(fn_for_hash).Cat(bill.Date, MKSFMT(0, DATF_YMD|DATF_CENTURY|DATF_NODIV));
							const uint32 _h = SlHash::BobJenc(bill_code, bill_code.Len());
							bill_code.Z().Cat("H-").Cat(_h).CatChar('-').CatLongZ(cc_, 6);
						}
						bill_code.CopyTo(r_row.BillCode, sizeof(r_row.BillCode));
						if(new_bill) {
							MEMSZERO(bill);
							bill.Date = r_row.BillDate;
							bill_code.CopyTo(bill.Code, sizeof(bill.Code));
							bill_ident.CopyTo(bill.ID, sizeof(bill.ID));
                            bill.CntragID = r_row.CntragID;
							bill.CntragNo = r_row.CntragNo;
                            STRNSCPY(bill.CntragName, r_row.CntragName);
                            bill.DueDate = r_row.DueDate;
                            STRNSCPY(bill.INN, r_row.INN);
                            STRNSCPY(bill.LocCode, r_row.LocCode);
                            STRNSCPY(bill.LocID, r_row.LocID);
                            STRNSCPY(bill.LocName, r_row.LocName);
                            bill.Obj2ID = r_row.Obj2ID;
                            STRNSCPY(bill.Obj2INN, r_row.Obj2INN);
                            STRNSCPY(bill.Obj2Name, r_row.Obj2Name);
                            bill.Obj2No = r_row.Obj2No;
                            STRNSCPY(bill.AgentINN, r_row.AgentINN);
							bill.AgentPersonID = r_row.AgentPersonID;
							bill.DlvrAddrID = r_row.DlvrAddrID;
							STRNSCPY(bill.DlvrAddrCode, r_row.DlvrAddrCode);
                            STRNSCPY(bill.Memo, r_row.BillMemo);
                            // @todo не все поля перенесены из r_row в bill
							STRNSCPY(bill.InvoiceCode, r_row.InvcCode); // @v11.0.9
							bill.InvoiceDate = r_row.InvcDate; // @v11.0.9
							//
							int    bidx_found = 0;
                            if(CheckBill(&bill)) {
								for(uint bidx = 0; bidx < Bills.getCount(); bidx++) {
									const Sdr_Bill & r_item = Bills.at(bidx);
									if(r_item.Date == bill.Date && sstreq(r_item.Code, bill.Code)) {
										if(r_item.CntragID) {
											if(r_item.CntragID == bill.CntragID)
												bidx_found = 1;
										}
										else if(r_item.CntragName[0]) {
											if(sstreq(r_item.CntragName, bill.CntragName))
												bidx_found = 1;
										}
										else if(r_item.CntragNo) {
											if(r_item.CntragNo == bill.CntragNo)
												bidx_found = 1;
										}
										else
											bidx_found = 1;
									}
								}
                            }
							else
								bidx_found = 1;
							if(!bidx_found) {
								last_date = r_row.BillDate;
								last_code = org_bill_code;
								last_ident = org_bill_ident;
								THROW_SL(Bills.insert(&bill));
							}
							else
								row_idx_list_to_remove.add(ln);
						}
					}
					{
						uint _p = row_idx_list_to_remove.getCount();
						if(_p) {
							row_idx_list_to_remove.sortAndUndup();
							do {
								const uint row_idx_to_remove = (uint)row_idx_list_to_remove.get(--_p);
								BillsRows.atFree(row_idx_to_remove);
							} while(_p);
						}
					}
				}
				{
					SForEachVectorItem(BillsRows, i) { THROW_SL(preserve_rows.insert(&BillsRows.at(i))); }
					BillsRows = preserve_rows;
				}
			}
			else {
				long   bill_line_no = 0;
				int    accept_rows = 0;
				THROW(ie.OpenFileForReading(filename));
				ie.GetNumRecs(&count);
				for(bill_line_no = 0; bill_line_no < count; bill_line_no++) {
					MEMSZERO(bill);
					AssignFnFieldToRecord(fn_fld_list, &bill, 0);
					THROW(ie.ReadRecord(&bill, sizeof(bill)));
					if(bill.Date == ZERODATE)
						getcurdate(&bill.Date);
					if(CheckBill(&bill)) {
						int    ir = AddBillToList(&bill, fi);
						THROW(ir);
						if(ir > 0)
							accept_rows = 1;
					}
					PPWaitPercent(bill_line_no + 1, count);
					if(imp_rows_from_same_file)
						break;
				}
				ie.CloseFile();
				if(imp_rows_from_same_file && accept_rows) {
					THROW(ie_row.OpenFileForReading(filename));
					THROW(ReadRows(&ie_row, 1, &fn_fld_list));
					ie_row.CloseFile();
				}
				Bills.sort(PTR_CMPFUNC(Pchar));
				if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles)
					ToRemoveFiles.add(filename);
			}
			ok = 1;
		}
		if(!imp_rows_from_same_file) {
			THROW_PP_S(GetOpType(BillParam.ImpOpID) != PPOPT_ACCTURN, PPERR_BILLIMPOPCANTBIACC, CfgNameBRow);
			THROW(BRowParam.PreprocessImportFileSpec(ss_files));
			for(uint ssp = 0; ss_files.get(&ssp, filename);) {
				/*StrAssocArray*/PPImpExpParam::PtTokenList fn_fld_list;
				BillParam.PreprocessImportFileName(filename, fn_fld_list);
				THROW(ie_row.OpenFileForReading(filename));
				THROW(ReadRows(&ie_row, 0, &fn_fld_list));
				ie_row.CloseFile();
				if(BillParam.BaseFlags & PPImpExpParam::bfDeleteSrcFiles) {
					ToRemoveFiles.add(filename);
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::ResolveUnitPerPack(PPID goodsID, PPID locID, LDATE dt, double * pUpp)
{
	double upp = 0.0;
	ReceiptCore * p_rcpt = (P_BObj && P_BObj->trfr) ? &P_BObj->trfr->Rcpt : 0;
	if(p_rcpt) {
		ReceiptTbl::Rec lot_rec;
		// @v10.6.4 MEMSZERO(lot_rec);
		if(p_rcpt->GetLastLot(goodsID, locID, dt, &lot_rec) > 0)
			upp = lot_rec.UnitPerPack;
		if(upp <= 0.0) {
			LDATE  iter_dt = dt;
			long   iter_oprno = MAXLONG;
			while(p_rcpt->EnumLastLots(goodsID, locID, &iter_dt, &iter_oprno, &lot_rec) > 0) {
				if(lot_rec.UnitPerPack > 0.0 || diffdate(dt, iter_dt) > 180) {
					break;
				}
			}
			upp = lot_rec.UnitPerPack;
		}
		SETMAX(upp, 0.0);
	}
	ASSIGN_PTR(pUpp, upp);
	return (upp > 0.0) ? 1 : -1;
}

int PPBillImporter::CreateAbsenceGoods(ResolveGoodsItem & rRgi, int use_ta)
{
	int    ok = -1;
	SString temp_buf;
	SString msg_buf;
	SString fmt_buf;
	const PPGoodsConfig & r_gcfg = GObj.GetConfig();
	if(r_gcfg.DefGroupID && r_gcfg.DefUnitID) {
		PPObjGoodsGroup gg_obj;
		PPObjBrand brand_obj;
		SString goods_name_buf;
		PPTransaction tra(use_ta);
		THROW(tra);
		{
			//ResolveGoodsItem & r_rgi = goods_list.at(i);
			if(isempty(rRgi.GoodsName)) {
				Logger.Log(PPLoadTextS(PPTXT_CANTCREATEWAREWONAME, msg_buf));
			}
			else if(!rRgi.ResolvedGoodsID) {
				PPGoodsPacket gpack;
				PPID   parent_id = 0;
				{
					temp_buf = rRgi.GroupName;
					if(temp_buf.NotEmptyS()) {
						PPID   folder_id = 0;
						if(!gg_obj.AddSimple(&parent_id, gpkndOrdinaryGroup, folder_id, temp_buf /*grpname*/, 0, r_gcfg.DefUnitID, 0)) {
							PPGetLastErrorMessage(1, msg_buf);
							Logger.Log(msg_buf);
							parent_id = 0;
						}
					}
				}
				SETIFZ(parent_id, r_gcfg.DefGroupID);
				if(GObj.InitPacket(&gpack, gpkndGoods, parent_id, 0, /*rRgi.Barcode*/0)) {
					const  size_t max_nm_len = sizeof(static_cast<const Goods2Tbl::Rec *>(0)->Name)-1;
					PPID   by_name_id = 0;
					SString suffix;
					long   uc = 1;
					// @v11.4.5 {
					SString barcode_buf;
					StringSet barcode_list;
					const  uint barcode_count = SplitBarcodeList(rRgi.Barcode, barcode_buf, barcode_list); // @v11.4.5
					for(uint bcidx = 0, ssp = 0; bcidx < barcode_count; bcidx++) {
						//temp_buf
						const int blgr = barcode_list.get(&ssp, temp_buf);
						assert(blgr);
						if(temp_buf.NotEmptyS())
							gpack.AddCode(temp_buf, 0, 1.0);
					}
					// } @v11.4.5
					(goods_name_buf = rRgi.GoodsName).Strip();
					while(GObj.SearchByName(goods_name_buf, &by_name_id, 0) > 0) {
						suffix.Z().Space().CatChar('#').Cat(++uc);
						(goods_name_buf = rRgi.GoodsName).Strip();
						size_t sum_len = goods_name_buf.Len() + suffix.Len();
						if(sum_len > max_nm_len)
							goods_name_buf.Trim(max_nm_len-suffix.Len());
						goods_name_buf.Cat(suffix);
					}
					STRNSCPY(gpack.Rec.Name, goods_name_buf);
					STRNSCPY(gpack.Rec.Abbr, goods_name_buf);
					gpack.Rec.UnitID = r_gcfg.DefUnitID;
					if(rRgi.ArID && rRgi.ArCode[0]) {
						ArGoodsCodeTbl::Rec ar_code_rec;
						ar_code_rec.ArID = rRgi.ArID;
						ar_code_rec.Pack = 1000; // 1.0
						STRNSCPY(ar_code_rec.Code, rRgi.ArCode);
						gpack.ArCodes.insert(&ar_code_rec);
					}
					if(rRgi.ManufName[0]) {
						temp_buf = rRgi.ManufName;
						if(temp_buf.NotEmptyS()) {
							PPID   manuf_id = 0;
							THROW(PsnObj.AddSimple(&manuf_id, temp_buf, PPPRK_MANUF, PPPRS_LEGAL, 0));
							gpack.Rec.ManufID = manuf_id;
						}
					}
					if(rRgi.BrandName[0]) {
						temp_buf = rRgi.BrandName;
						if(temp_buf.NotEmptyS()) {
							PPID   brand_id = 0;
							THROW(brand_obj.AddSimple(&brand_id, temp_buf, 0, 0));
							gpack.Rec.BrandID = brand_id;
						}
					}
					if(rRgi.VatRate > 0.0 && rRgi.VatRate <= 40.0) {
						PPID   tax_grp_id = 0;
						if(GObj.GTxObj.GetByScheme(&tax_grp_id, rRgi.VatRate, 0.0, 0.0, 0, 0/*use_ta*/) > 0)
							gpack.Rec.TaxGrpID = tax_grp_id;
					}
					PPID goods_id = 0;
					if(GObj.PutPacket(&goods_id, &gpack, 0)) {
						PPLoadText(PPTXT_WAREAUTOCREATED, fmt_buf);
						Logger.Log(msg_buf.Printf(fmt_buf, gpack.Rec.Name));
						rRgi.ResolvedGoodsID = goods_id;
						ok = 1;
					}
					else {
						PPGetLastErrorMessage(1, temp_buf);
						PPLoadText(PPTXT_ERRACCEPTGOODS, fmt_buf);
						Logger.Log(msg_buf.Printf(fmt_buf, PPOBJ_GOODS, gpack.Rec.Name, temp_buf.cptr()));
					}
				}
			}
		}
		THROW(tra.Commit());
	}
	else {
		// Для автоматического создания товаров в конфигурации товаров должны быть указаны группа и единица измерения по умолчанию
		Logger.Log(PPLoadTextS(PPTXT_TOCREATEWARECFGOPTNEEDED, msg_buf));
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::Import(int useTa)
{
	int    ok = 1;
	SString temp_buf;
	SString goods_name_buf;
	SString qc_code;
	SString TTN;
	SString fmt_buf, msg_buf;
	SString serial, sn_templt;
	PPObjAccSheet acs_obj;
	PPAccSheet acs_rec;
	ObjTransmContext ot_ctx(0, &Logger); // Контекст, необходимый для автоматического приходования дефицита классом BillTransmDeficit
	PPObjectTag tag_rec;
	ObjTagItem tag_item;
	ObjTagList tag_list;
	PPID   mnf_lot_tag_id = 0;
	PPOprKind op_rec;
	GetOpData(OpID, &op_rec);
	const int restrict_by_matrix = BIN(BillParam.Flags & PPBillImpExpParam::fRestrictByMatrix);
	const int need_price_restrict = BIN(op_rec.ExtFlags & OPKFX_RESTRICTPRICE);
	const PPGoodsConfig & r_gcfg = GObj.GetConfig();
	PPWaitStop();
	{
		const  PPID temp_tag_id_1 = BillParam.ImpExpParamDll.ManufTagID;
		const  PPID temp_tag_id_2 = P_BObj->GetConfig().MnfCountryLotTagID;
		if(temp_tag_id_1 || temp_tag_id_2) {
			if(temp_tag_id_1 && TagObj.Fetch(temp_tag_id_1, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON)
				mnf_lot_tag_id = temp_tag_id_1;
			else if(temp_tag_id_2 && TagObj.Fetch(temp_tag_id_2, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON)
				mnf_lot_tag_id = temp_tag_id_2;
		}
	}
	{
		//
		// Обработка неразрешенных товаров
		//
		int    r = -1;
		ResolveGoodsItemList goods_list;
		LongArray unres_pos_list;
		for(uint i = 0; i < BillsRows.getCount(); i++) {
			const Sdr_BRow & r_row = BillsRows.at(i);
			if(!r_row.GoodsID) {
				ResolveGoodsItem item;
				STRNSCPY(item.GoodsName, r_row.GoodsName);
				STRNSCPY(item.GroupName, r_row.GoodsGroup); // @v10.5.0
				STRNSCPY(item.BrandName, r_row.BrandName); // @v10.5.3
				item.VatRate = r_row.VatRate; // @v10.5.9
				STRNSCPY(item.Barcode, r_row.Barcode);
				// @v10.4.12 {
				(temp_buf = r_row.LotManuf).Transf(CTRANSF_OUTER_TO_INNER);
				STRNSCPY(item.ManufName, temp_buf);
				// } @v10.4.12
				item.Quantity = r_row.Quantity;
				if(r_row.CntragID && r_row.ArCode[0]) {
					item.ArID = r_row.CntragID;
					STRNSCPY(item.ArCode, r_row.ArCode);
				}
				THROW_SL(goods_list.insert(&item));
				unres_pos_list.add(i);
			}
		}
		if(goods_list.getCount()) {
			// @v10.4.12 {
			assert(goods_list.getCount() == unres_pos_list.getCount());
			if(BillParam.Flags & PPBillImpExpParam::fCreateAbsenceGoods) {
				if(r_gcfg.DefGroupID && r_gcfg.DefUnitID) {
					PPObjGoodsGroup gg_obj;
					PPObjBrand brand_obj;
					PPTransaction tra(useTa);
					THROW(tra);
					for(uint i = 0; i < goods_list.getCount(); i++) {
						ResolveGoodsItem & r_rgi = goods_list.at(i);
						if(!r_rgi.ResolvedGoodsID && CreateAbsenceGoods(r_rgi, 0) > 0) {
							const PPID goods_id = r_rgi.ResolvedGoodsID;
							assert(goods_id);
							BillsRows.at(unres_pos_list.get(i)).GoodsID = goods_id;
							//
							// Теперь, когда новый товар создан, нам надо найти все аналогичные позиции в списке (если они там есть)
							// и сопоставить их с созданным товаров.
							// Мы сделаем это, основываясь, в порядке уменьшения приоритета, либо на коде товара, либо на коде по статье,
							// либо на наименовании. Обращаю внимание на то, что сравнение наименований точное с учетом регистра.
							//
							if(r_rgi.Barcode[0]) {
								for(uint j = i+1; j < goods_list.getCount(); j++) {
									ResolveGoodsItem & r_rgi2 = goods_list.at(j);
									if(sstreqi_ascii(r_rgi2.Barcode, r_rgi.Barcode)) {
										r_rgi2.ResolvedGoodsID = goods_id;
										BillsRows.at(unres_pos_list.get(j)).GoodsID = goods_id;
									}
								}
							}
							else if(r_rgi.ArID && r_rgi.ArCode[0]) {
								for(uint j = i+1; j < goods_list.getCount(); j++) {
									ResolveGoodsItem & r_rgi2 = goods_list.at(j);
									if(r_rgi2.ArID == r_rgi.ArID && sstreqi_ascii(r_rgi2.ArCode, r_rgi.ArCode)) {
										r_rgi2.ResolvedGoodsID = goods_id;
										BillsRows.at(unres_pos_list.get(j)).GoodsID = goods_id;
									}
								}
							}
							else {
								for(uint j = i+1; j < goods_list.getCount(); j++) {
									ResolveGoodsItem & r_rgi2 = goods_list.at(j);
									if(sstreq(r_rgi2.GoodsName, r_rgi.GoodsName)) {
										r_rgi2.ResolvedGoodsID = goods_id;
										BillsRows.at(unres_pos_list.get(j)).GoodsID = goods_id;
									}
								}
							}
						}
					}
					THROW(tra.Commit());
				}
				else {
					// Для автоматического создания товаров в конфигурации товаров должны быть указаны группа и единица измерения по умолчанию
					PPLoadText(PPTXT_TOCREATEWARECFGOPTNEEDED, msg_buf);
					Logger.Log(msg_buf);
				}
				{
					assert(goods_list.getCount() == unres_pos_list.getCount());
					uint i = goods_list.getCount();
					if(i) do {
						ResolveGoodsItem & r_rgi = goods_list.at(--i);
						if(r_rgi.ResolvedGoodsID) {
							goods_list.atFree(i);
							unres_pos_list.atFree(i);
						}
					} while(i);
					assert(goods_list.getCount() == unres_pos_list.getCount());
				}
			}
			// } @v10.4.12
			if(goods_list.getCount()) {
				if(DS.CheckExtFlag(ECF_SYSSERVICE)) // @v10.1.0
					ok = -1;
				else {
					if(ResolveGoodsDlg(&goods_list, RESOLVEGF_SHOWBARCODE|RESOLVEGF_MAXLIKEGOODS|RESOLVEGF_SHOWEXTDLG) > 0) {
						for(uint i = 0; i < goods_list.getCount(); i++)
							BillsRows.at(unres_pos_list.get(i)).GoodsID = goods_list.at(i).ResolvedGoodsID;
						r = 1;
					}
					else
						ok = -1;
				}
			}
		}
	}
	if(ok > 0) {
		PrcssrAlcReport::Config alc_rep_cfg;
		int    alc_rep_cfg_inited = 0;
		{
			ZDELETE(P_Btd);
			if(GetOpType(OpID) == PPOPT_GOODSEXPEND) {
				THROW_MEM(P_Btd = new BillTransmDeficit);
				PPObjectTransmit::ReadConfig(&ot_ctx.Cfg);
			}
		}
		Bills.sort(PTR_CMPFUNC(Sdr_Bill));
		PPTransaction tra(useTa);
		THROW(tra);
		PPWaitStart();
		for(uint i__ = 0; i__ < Bills.getCount(); i__++) {
			uint   pos = 0;
			int    is_draft_rcpt = BIN(GetOpType(OpID) == PPOPT_DRAFTRECEIPT);
			PPBillPacket pack;
			Sdr_Bill bill = Bills.at(i__);
			if(bill.EdiOp == PPEDIOP_ALCODESADV) {
				PPID   alc_manuf_lot_tag_id = 0;
				PPID   alc_manuf_kind_id = 0;
				PPID   alc_cat_lot_tag_id = 0;
				SETIFZ(alc_rep_cfg_inited, (!alc_rep_cfg_inited && PrcssrAlcReport::ReadConfig(&alc_rep_cfg) > 0) ? 1 : -1);
				if(alc_rep_cfg_inited < 0) {
				}
				else if(BillToBillRec(&bill, &pack)) {
					PPID   main_bill_id = 0;
					BillTbl::Rec main_bill_rec;
					if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &main_bill_id, &main_bill_rec) > 0) {
						int    pack_updated = 0;
						THROW(P_BObj->ExtractPacket(main_bill_id, &pack) > 0);
						{
							if(alc_rep_cfg.CategoryTagID && TagObj.Fetch(alc_rep_cfg.CategoryTagID, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT)
								alc_cat_lot_tag_id = alc_rep_cfg.CategoryTagID;
							if(alc_rep_cfg.LotManufTagList.getCount()) {
								for(uint k = 0; !alc_manuf_lot_tag_id && k < alc_rep_cfg.LotManufTagList.getCount(); k++) {
									PPID tag_id = alc_rep_cfg.LotManufTagList.get(k);
									if(tag_id && TagObj.Fetch(tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT && tag_rec.TagEnumID == PPOBJ_PERSON) {
										alc_manuf_lot_tag_id = tag_id;
										alc_manuf_kind_id = tag_rec.LinkObjGrp;
									}
								}
							}
							for(pos = 0; SearchNextRowForBill(bill, &pos); pos++) {
								const  Sdr_BRow & r_row = BillsRows.at(pos);
								if(r_row.LineNo > 0 && r_row.LineNo <= (int)pack.GetTCount()) {
									const uint ti_idx = r_row.LineNo-1;
									int    tag_list_updated = 0;
									ObjTagList * p_tag_list = pack.LTagL.Get(ti_idx);
									if(!RVALUEPTR(tag_list, p_tag_list))
										tag_list.Destroy();
									PPTransferItem & r_ti = pack.TI(ti_idx);
									if(r_ti.GoodsID == r_row.GoodsID) {
										if(alc_cat_lot_tag_id && r_row.AlcoCatCode[0]) {
											if(tag_item.SetStr(alc_cat_lot_tag_id, r_row.AlcoCatCode)) {
												tag_list.PutItem(alc_cat_lot_tag_id, &tag_item);
												tag_list_updated = 1;
											}
										}
										if(alc_manuf_lot_tag_id && r_row.ManufINN[0]) {
											PPID   manuf_id = 0;
											PPIDArray psn_list;
                                            PsnObj.GetListByRegNumber(PPREGT_TPID, alc_manuf_kind_id, r_row.ManufINN, psn_list);
											if(psn_list.getCount()) {
												if(r_row.ManufKPP[0]) {
													for(uint k = 0; !manuf_id && k < psn_list.getCount(); k++) {
														const PPID psn_id = psn_list.get(k);
														if(PsnObj.GetRegNumber(psn_id, PPREGT_KPP, ZERODATE, temp_buf.Z()) > 0 && temp_buf.IsEqNC(r_row.ManufKPP))
															manuf_id = psn_id;
													}
												}
												else
													manuf_id = psn_list.get(0);
											}
											else if((temp_buf = r_row.LotManuf).NotEmptyS()) {
												PPPersonPacket psn_pack;
												STRNSCPY(psn_pack.Rec.Name, temp_buf);
												psn_pack.Kinds.add(alc_manuf_kind_id);
												{
													RegisterTbl::Rec reg_rec;
													PPObjRegister::InitPacket(&reg_rec, PPREGT_TPID, PPObjID(PPOBJ_PERSON, 0), r_row.ManufINN);
													psn_pack.Regs.insert(&reg_rec);
												}
												if(r_row.ManufKPP[0]) {
													RegisterTbl::Rec reg_rec;
													PPObjRegister::InitPacket(&reg_rec, PPREGT_KPP, PPObjID(PPOBJ_PERSON, 0), r_row.ManufKPP);
													psn_pack.Regs.insert(&reg_rec);
												}
												THROW(PsnObj.PutPacket(&manuf_id, &psn_pack, 0));
											}
											if(manuf_id) {
												if(tag_item.SetInt(alc_manuf_lot_tag_id, manuf_id)) {
													tag_list.PutItem(alc_manuf_lot_tag_id, &tag_item);
													tag_list_updated = 1;
												}
											}
										}
									}
									if(tag_list_updated) {
										pack.LTagL.Set(ti_idx, &tag_list);
										pack_updated = 1;
									}
								}
							}
						}
						if(pack_updated) {
							THROW(P_BObj->UpdatePacket(&pack, 0));
						}
					}
					else {
						PPLoadText(PPTXT_EDIRELMAINNFOUND, temp_buf);
					}
				}
			}
			else {
				THROW(pack.CreateBlank2(OpID, bill.Date, LocID, 0));
				if(BillToBillRec(&bill, &pack) > 0) {
					int    is_bad_packet = 0; // Признак того, что документ не удалось правильно преобразовать
					for(pos = 0; !is_bad_packet && SearchNextRowForBill(bill, &pos); pos++) {
						const Sdr_BRow & r_row = BillsRows.at(pos);
						(serial = r_row.Serial).Strip().Transf(CTRANSF_OUTER_TO_INNER);
						if(!r_row.GoodsID) {
							;
						}
						else if(restrict_by_matrix && !GObj.BelongToMatrix(r_row.GoodsID, pack.Rec.LocID)) {
							Logger.LogLastError();
						}
						else if(pack.OpTypeID == PPOPT_GOODSEXPEND) {
							ILTI   ilti;
							long   ciltif_ = CILTIF_OPTMZLOTS|CILTIF_SUBSTSERIAL|CILTIF_ALLOWZPRICE/*|CILTIF_SYNC*/;
							int    lot_id_exists = 0;
							if(r_row.LotID > 0) {
								ReceiptTbl::Rec temp_lot_rec;
								lot_id_exists = BIN(P_BObj->trfr->Rcpt.Search(r_row.LotID, &temp_lot_rec) > 0);
								if(lot_id_exists && temp_lot_rec.GoodsID == r_row.GoodsID && temp_lot_rec.LocID == LocID) {
									ilti.LotSyncID = r_row.LotID;
									ciltif_ |= CILTIF_USESYNCLOT;
								}
							}
							ilti.Setup(r_row.GoodsID, -1, r_row.Quantity, r_row.Cost, r_row.Price);
							const int rconv = P_BObj->ConvertILTI(&ilti, &pack, 0, ciltif_, serial.NotEmpty() ? serial.cptr() : 0, 0);
							if(!rconv) {
								Logger.LogLastError();
								SETIFZ(is_bad_packet, 1);
							}
							else if(ilti.HasDeficit()) {
								if(P_Btd) {
									THROW(P_Btd->AddItem(&ilti, serial, &pack.Rec, 1));
								}
								PPLoadString("deficit", msg_buf);
								msg_buf.Space().CatChar('[').Cat(GetGoodsName(r_row.GoodsID, temp_buf));
								if(serial.NotEmpty())
									msg_buf.CatDiv(';', 2).CatEq("sn", serial);
								if(lot_id_exists)
									msg_buf.CatDiv(';', 2).CatEq("lotid", r_row.LotID);
								msg_buf.CatChar(']');
								msg_buf.CatDiv('=', 1).Cat(ilti.Rest, MKSFMTD(0, 6, 0));
								Logger.Log(msg_buf);
								is_bad_packet = 100;
							}
						}
						else {
							PPID   qcert_id = 0;
							double upp = 0.0;
							int    upp_inited = 0;
							PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
							ti.GoodsID  = r_row.GoodsID;
							ti.Cost     = r_row.Cost;
							ti.Price    = r_row.Price;
							if(r_row.LotID > 0) {
								ReceiptTbl::Rec temp_lot_rec;
								const int lot_id_exists = BIN(P_BObj->trfr->Rcpt.Search(r_row.LotID, &temp_lot_rec) > 0);
								if(ti.Flags & PPTFR_RECEIPT) {
									if(!lot_id_exists && !pack.SearchLot(r_row.LotID, 0)) {
										ti.LotID = r_row.LotID;
										ti.TFlags |= PPTransferItem::tfForceLotID;
									}
								}
								else {
									if(lot_id_exists && temp_lot_rec.GoodsID == ti.GoodsID && temp_lot_rec.LocID == LocID) {
										THROW(ti.SetupLot(r_row.LotID, &temp_lot_rec, 0));
									}
								}
							}
							if(checkdate(r_row.Expiry))
								ti.Expiry = r_row.Expiry;
							if(is_draft_rcpt || (ti.Flags & PPTFR_RECEIPT)) {
								if(r_row.UnitPerPack > 0.0)
									upp = r_row.UnitPerPack;
								if(upp <= 0.0)
									ResolveUnitPerPack(ti.GoodsID, pack.Rec.LocID, pack.Rec.Dt, &upp);
								upp_inited = 0;
								ti.UnitPerPack = upp;
							}
							if(r_row.Quantity != 0.0)
								ti.Quantity_ = fabs(r_row.Quantity);
							else if(r_row.PckgQtty != 0.0) {
								if(upp <= 0.0) {
									GoodsStockExt gse;
									if(GObj.GetStockExt(ti.GoodsID, &gse, 1) > 0)
										upp = gse.Package;
								}
								if(upp <= 0.0 && !upp_inited) {
									ResolveUnitPerPack(ti.GoodsID, pack.Rec.LocID, pack.Rec.Dt, &upp);
									upp_inited = 0;
								}
								if(upp > 0.0)
									ti.Quantity_ = fabs(r_row.PckgQtty * upp);
							}
							THROW(P_BObj->SetupImportedPrice(&pack, &ti, 0));
							//
							// Обработка сертификата качества.
							// Если код сертификата пустой, то считаем, что сертификата нет
							//
							(qc_code = r_row.QcCode).Strip().Transf(CTRANSF_OUTER_TO_INNER);
							if(qc_code.NotEmpty() && QcObj.SearchByCode(qc_code, &qcert_id, 0) <= 0) {
								qcert_id = 0;
								QualityCertTbl::Rec qc_rec;
								// @v10.6.4 MEMSZERO(qc_rec);
								qc_code.CopyTo(qc_rec.Code, sizeof(qc_rec.Code));
								STRNSCPY(qc_rec.BlankCode, (temp_buf = r_row.QcBc).Transf(CTRANSF_OUTER_TO_INNER));
								STRNSCPY(qc_rec.SPrDate, (temp_buf = r_row.QcPrDate).Transf(CTRANSF_OUTER_TO_INNER));
								STRNSCPY(qc_rec.Manuf, (temp_buf = r_row.QcManuf).Transf(CTRANSF_OUTER_TO_INNER));
								STRNSCPY(qc_rec.Etc, (temp_buf = r_row.QcEtc).Transf(CTRANSF_OUTER_TO_INNER));
								qc_rec.InitDate = r_row.QcInitDate;
								qc_rec.Expiry = r_row.QcExpiry;
								if(r_row.QcRegOrg[0]) {
									(temp_buf = r_row.QcRegOrg).Transf(CTRANSF_OUTER_TO_INNER);
									THROW(PsnObj.AddSimple(&qc_rec.RegOrgan, temp_buf, PPPRK_BUSADMIN, PPPRS_LEGAL, 0));
								}
								THROW(QcObj.PutPacket(&qcert_id, &qc_rec, 0));
							}
							ti.QCert = qcert_id;
							(temp_buf = r_row.CLB).Strip().Transf(CTRANSF_OUTER_TO_INNER);
							if(oneof3(pack.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT, PPOPT_GOODSORDER)) {
								if(serial.NotEmpty()) {
									P_BObj->AdjustSerialForUniq(ti.GoodsID, ti.LotID, 0, serial);
								}
								else if(P_BObj->GetConfig().Flags & BCF_AUTOSERIAL) {
									sn_templt = GObj.IsAsset(ti.GoodsID) ? P_BObj->Cfg.InvSnTemplt : P_BObj->Cfg.SnTemplt;
									P_BObj->GetSnByTemplate(pack.Rec.Code, labs(ti.GoodsID), &pack.LTagL/*SnL*/, sn_templt, serial);
								}
							}
							{
								int    new_item_pos = -1;
								THROW(pack.LoadTItem(&ti, temp_buf, serial));
								new_item_pos = pack.GetTCount()-1;
								if(pack.OpTypeID == PPOPT_GOODSRECEIPT || pack.IsDraft()) { // @v11.1.6 (|| pack.IsDraft())
									ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
									if(p_tag_list && p_tag_list->GetCount()) {
										pack.LTagL.Set(new_item_pos, p_tag_list);
										if(temp_buf.NotEmpty())
											pack.LTagL.AddNumber(PPTAG_LOT_CLB, new_item_pos, temp_buf);
										if(serial.NotEmpty())
											pack.LTagL.AddNumber(PPTAG_LOT_SN, new_item_pos, serial);
									}
								}
								if(need_price_restrict) {
									RealRange price_range;
									if(P_BObj->GetPriceRestrictions(pack, ti, new_item_pos, &price_range) > 0) {
										PPTransferItem & r_ti = pack.TI(new_item_pos);
										if(!price_range.CheckValEps(r_ti.Price, 1E-7)) { // @v10.2.12
											if(price_range.low > 0.0 && r_ti.Price < price_range.low) {
												PPFormatT(PPTXT_LOG_IMPBILLPRICEBOUNDLO, &msg_buf, ti.GoodsID, r_ti.Price, price_range.low);
												Logger.Log(msg_buf);
												r_ti.Price = price_range.low;
											}
											else if(price_range.upp > 0.0 && r_ti.Price > price_range.upp) {
												PPFormatT(PPTXT_LOG_IMPBILLPRICEBOUNDUP, &msg_buf, ti.GoodsID, r_ti.Price, price_range.upp);
												Logger.Log(msg_buf);
												r_ti.Price = price_range.upp;
											}
										}
									}
								}
								TTN = r_row.TTN;
								// Запоминамем в теге строки имя производителя/импортера
								if(pack.OpTypeID == PPOPT_GOODSRECEIPT && mnf_lot_tag_id) {
									(temp_buf = r_row.LotManuf).Transf(CTRANSF_OUTER_TO_INNER);
									PPID   manuf_id = 0;
									if(temp_buf.NotEmptyS()) {
										THROW(PsnObj.AddSimple(&manuf_id, temp_buf, PPPRK_MANUF, PPPRS_LEGAL, 0));
									}
									if(manuf_id) {
										int    new_item_pos = -1;
										new_item_pos = pack.GetTCount()-1;
										ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
										if(!RVALUEPTR(tag_list, p_tag_list))
											tag_list.Destroy();
										tag_item.Init(mnf_lot_tag_id);
										//tag_item.TagDataType = OTTYP_OBJLINK;
										tag_item.SetInt(tag_item.TagID, manuf_id);
										tag_list.PutItem(tag_item.TagID, &tag_item);
										pack.LTagL.Set(new_item_pos, &tag_list);
										TagC.Set(new_item_pos, &tag_list);
									}
								}
								// Запомним код вид товара
								if(pack.OpTypeID == PPOPT_GOODSRECEIPT && BillParam.ImpExpParamDll.GoodsKindTagID && r_row.GoodKindCode) {
									int    new_item_pos = -1;
									new_item_pos = pack.GetTCount()-1;
									ObjTagList * p_tag_list = TagC.Get(r_row.LineId);
									if(!RVALUEPTR(tag_list, p_tag_list))
										tag_list.Destroy();
									tag_item.Init(BillParam.ImpExpParamDll.GoodsKindTagID);
									tag_item.TagDataType = OTTYP_NUMBER;
									tag_item.SetInt(tag_item.TagID, r_row.GoodKindCode);
									tag_list.PutItem(tag_item.TagID, &tag_item);
									pack.LTagL.Set(new_item_pos, &tag_list);
									TagC.Set(new_item_pos, &tag_list);
								}
							}
						}
					}
					if(is_bad_packet == 100)
						Logger.LogMsgCode(mfError, PPERR_BILLNOTIMPORTED, bill.ID);
					else if(is_bad_packet)
						Logger.LogMsgCode(mfError, PPERR_BILLNOTIMPORTED, bill.ID);
					else {
						if(acs_obj.Fetch(pack.AccSheetID, &acs_rec) > 0) {
							if((acs_rec.Flags & ACSHF_USECLIAGT) || pack.AccSheetID == GetSellAccSheet()) {
								PPClientAgreement ca_rec;
								if(ArObj.GetClientAgreement(pack.Rec.Object, ca_rec, 1) > 0) {
									if(!(ca_rec.Flags & AGTF_DEFAULT))
										if(GetAgentAccSheet() && ca_rec.DefAgentID && !pack.Ext.AgentID)
											pack.Ext.AgentID = ca_rec.DefAgentID;
									if(ca_rec.DefPayPeriod >= 0 && pack.Rec.Flags & BILLF_NEEDPAYMENT)
										pack.SetPayDate(plusdate(pack.Rec.Dt, ca_rec.DefPayPeriod), 0);
								}
							}
							else if((acs_rec.Flags & ACSHF_USESUPPLAGT) || pack.AccSheetID == GetSupplAccSheet()) {
								PPSupplAgreement sa_rec;
								if(ArObj.GetSupplAgreement(pack.Rec.Object, &sa_rec, 1) > 0) {
									if(!(sa_rec.Flags & AGTF_DEFAULT))
										if(GetAgentAccSheet() && sa_rec.DefAgentID && !pack.Ext.AgentID)
											pack.Ext.AgentID = sa_rec.DefAgentID;
									if(sa_rec.DefPayPeriod >= 0 && pack.Rec.Flags & BILLF_NEEDPAYMENT)
										pack.SetPayDate(plusdate(pack.Rec.Dt, sa_rec.DefPayPeriod), 0);
								}
							}
						}
						if(BillParam.ImpExpParamDll.TTNTagID) {
							tag_item.Init(BillParam.ImpExpParamDll.TTNTagID);
							tag_item.TagDataType = OTTYP_STRING;
							tag_item.SetStr(tag_item.TagID, TTN);
							pack.BTagL.PutItem(tag_item.TagID, &tag_item);
						}
						pack.SetQuantitySign(-1);
						if(CheckOpFlags(pack.Rec.OpID, OPKF_NEEDVALUATION)) {
							int    is_valuation_modif = 0;
							P_BObj->AutoCalcPrices(&pack, 0, &is_valuation_modif);
						}
						THROW(P_BObj->__TurnPacket(&pack, 0, 0, 0));
						Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
					}
				}
			}
			PPWaitPercent(i__ + 1, Bills.getCount());
		}
		THROW(tra.Commit());
		if(P_Btd) {
			ot_ctx.Cfg.Flags |= (DBDXF_TURNTOTALDEFICITE|DBDXF_CALCTOTALDEFICITE);
			ot_ctx.Cfg.Flags &= ~(DBDXF_SUBSTDEFICITGOODS|DBDXF_TWOPASSRCV);
			THROW(P_Btd->ProcessDeficit(&ot_ctx, 0));
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::ResolveINN(const char * pINN, PPID dlvrLocID, const char * pDlvrLocCode,
	const char * pBillId, PPID accSheetID, PPID * pArID, int logErr /*=1*/)
{
	int    ok = -1;
	PPID   ar_id = 0;
	PPID   reg_type_id = PPREGT_TPID;
	SString code(pINN);
	if(!code.NotEmptyS()) {
		if(PPObjArticle::GetSearchingRegTypeID(accSheetID, 0, 1, &reg_type_id) > 0)
			code = BillParam.Object1SrchCode;
	}
	if(code.NotEmptyS()) {
		PPIDArray psn_list, ar_list;
		THROW(PsnObj.GetListByRegNumber(reg_type_id, 0, code, psn_list));
		THROW(ArObj.GetByPersonList(accSheetID, &psn_list, &ar_list));
		if(ar_list.getCount()) {
			if(ar_list.getCount() > 1) {
				PPID   dlvr_loc_id = 0;
				LocationTbl::Rec loc_rec;
				if(dlvrLocID && PsnObj.LocObj.Search(dlvrLocID, &loc_rec) > 0) {
					uint   _ppos = 0;
                    if(loc_rec.OwnerID && psn_list.lsearch(loc_rec.OwnerID, &_ppos)) {
						ArObj.P_Tbl->PersonToArticle(loc_rec.OwnerID, accSheetID, &ar_id);
                    }
				}
				else if(!isempty(pDlvrLocCode)) {
                    PPIDArray loc_list;
                    PsnObj.LocObj.P_Tbl->GetListByCode(LOCTYP_ADDRESS, pDlvrLocCode, &loc_list);
                    for(uint loc_idx = 0; !ar_id && loc_idx < loc_list.getCount(); loc_idx++) {
						const PPID loc_id = loc_list.get(loc_idx);
						uint   _ppos = 0;
                        if(PsnObj.LocObj.Search(loc_id, &loc_rec) > 0 && loc_rec.OwnerID && psn_list.lsearch(loc_rec.OwnerID, &_ppos))
							ArObj.P_Tbl->PersonToArticle(loc_rec.OwnerID, accSheetID, &ar_id);
                    }
				}
			}
			SETIFZ(ar_id, ar_list.get(0));
			ASSIGN_PTR(pArID, ar_id);
			ok = 1;
		}
	}
	if(ok < 0 && logErr) {
		char   stub[32];
		SString msg, err;
		PPLoadString(PPMSG_ERROR, PPERR_BILLNOTIMPORTED2, msg);
		if(pBillId == 0) {
			PTR32(stub)[0] = 0;
			pBillId = stub;
		}
		Logger.Log(err.Printf(msg.cptr(), pBillId, pINN));
	}
	CATCHZOK
	return ok;
}

// @vmiller
int PPBillImporter::ResolveGLN(const char * pGLN, /*const char * pLocCode,*/const char * pBillId, PPID accSheetID, PPID * pArID, int logErr /*=1*/)
{

	int    ok = -1;
	PPID   ar_id = 0;
	assert(pGLN/*|| pLocCode*/);
	THROW_INVARG(pGLN/*|| pLocCode*/);
	//
	ok = PsnObj.ResolveGLN_Article(pGLN, accSheetID, &ar_id);
	if(ok < 0) {
		//
		// Если персоналия не найдена по ее GLN или ее GLN вообще не определена, то ищем по коду склада (трактуя его как GLN)
		//
		//ok = LocObj.ResolveWarehouseByCode(pLocCode, accSheetID, &ar_id);
		if(/*ok < 0 &&*/logErr) {
			char   stub[32];
			SString msg, err;
			PPLoadString(PPMSG_ERROR, PPERR_BILLNOTIMPORTED2, msg);
			if(pBillId == 0) {
				stub[0] = 0;
				pBillId = stub;
			}
			Logger.Log(err.Printf(msg.cptr(), pBillId, ""));
		}
	}
	CATCHZOK
	ASSIGN_PTR(pArID, ar_id);
	return ok;
}

int PPBillImpExpBaseProcessBlock::SearchEdiOrder(const SearchBlock & rBlk, BillTbl::Rec * pOrderRec)
{
	int    ok = -1;
	PPID   order_bill_id = 0;
	SString order_bill_code = rBlk.Code; // pBill->OrderBillNo;
	SString dlvr_loc_code = rBlk.DlvrLocCode; // pBill->DlvrAddrCode;
	if(P_BObj) {
		if(order_bill_code.NotEmptyS() && dlvr_loc_code.NotEmptyS()) {
			DateRange period;
			period.Z();
			if(/*pBill->OrderDate*/rBlk.Dt) {
				period.upp = rBlk.Dt; // pBill->OrderDate;
				period.low = rBlk.Dt; // pBill->OrderDate;
			}
			else {
				// Ограничим поиск документов последней неделей от текущего дня
				period.upp = getcurdate_();
				period.low = plusdate(period.upp, -rBlk.SurveyDays);
			}
			BillCore * t = P_BObj->P_Tbl;
			BillTbl::Key1 k1;
			BillTbl::Rec bill_rec;
			BExtQuery q(t, 1);
			q.select(t->ID, t->Code, t->Object, t->LocID, 0).where(daterange(t->Dt, &period));
			MEMSZERO(k1);
			k1.Dt = period.low;
			for(q.initIteration(false, &k1, spGe); ok < 0 && q.nextIteration() > 0;) {
				t->copyBufTo(&bill_rec);
				if(order_bill_code.IsEqNC(bill_rec.Code)) {
					PPFreight freight;
					LocationTbl::Rec loc_rec;
					// Смотрим адрес доставки. Он может быть в DlvrAddrCode и в LocCode
					if(t->GetFreight(bill_rec.ID, &freight) && LocObj.Fetch(freight.DlvrAddrID, &loc_rec) > 0 && dlvr_loc_code.IsEqNC(loc_rec.Code)) {
						order_bill_id = bill_rec.ID;
						ok = 1;
					}
					else if(LocObj.Fetch(bill_rec.LocID, &loc_rec) > 0 && dlvr_loc_code.IsEqNC(loc_rec.Code)) {
						order_bill_id = bill_rec.ID;
						ok = 1;
					}
				}
			}
			if(ok > 0) {
				if(P_BObj->Search(order_bill_id, &bill_rec) > 0) {
					ASSIGN_PTR(pOrderRec, bill_rec);
				}
				else
					ok = -1;
			}
		}
		else
			ok = -2;
	}
	return ok;
}
//
// @vmiller
// Descr: На данный момент проверяет наличие документа заказа (только драфт-документы), чьи параметры переданы в pBill
//
int PPBillImporter::GetDocImpStatus(Sdr_Bill * pBill, Sdr_DllImpObjStatus & rStatus)
{
	int    ok = 1;
	rStatus.DocStatus = statNoSuchDoc;
	if(pBill) {
		SString msg_buf, temp_buf;
		PPLoadText(PPTXT_EDIBILLRCVD, msg_buf);
		msg_buf.Space().CatBrackStr(pBill->EdiOpSymb);
		msg_buf.CatDiv(':', 2).CatEq("CODE", (temp_buf = pBill->Code).Transf(CTRANSF_OUTER_TO_INNER)).CatDiv(',', 2).CatEq("DATE", pBill->Date);
		if(pBill->EdiOp == PPEDIOP_ALCODESADV) {
			PPBillPacket pack;
			if(BillToBillRec(pBill, &pack)) {
				PPID   main_bill_id = 0;
				BillTbl::Rec main_bill_rec;
				if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &main_bill_id, &main_bill_rec) > 0) {
					PPLoadText(PPTXT_EDIRELMAINFOUND, temp_buf);
					temp_buf.Space().CatChar('(').CatEq("ID", main_bill_id).CatChar(')');
					rStatus.DocStatus = statIsSuchDoc;
				}
				else {
					PPLoadText(PPTXT_EDIRELMAINNFOUND, temp_buf);
				}
			}
		}
		else {
			msg_buf.CatDiv(',', 2).CatEq("ORDERCODE", (temp_buf = pBill->OrderBillNo).Transf(CTRANSF_OUTER_TO_INNER)).
				CatDiv(',', 2).CatEq("ORDERDATE", pBill->OrderDate);
			BillTbl::Rec order_bill_rec;
			SearchBlock sblk;
			sblk.Code = pBill->OrderBillNo;
			sblk.DlvrLocCode = pBill->DlvrAddrCode;
			sblk.Dt = pBill->OrderDate;
			sblk.SurveyDays = 7;
			int   sr = SearchEdiOrder(/*pBill*/sblk, &order_bill_rec);
			if(sr > 0) {
				ltoa(order_bill_rec.ID, pBill->OrderBillID, 10);
				PPLoadText(PPTXT_EDIRELORDERFOUNT, temp_buf);
				temp_buf.Space().CatChar('(').CatEq("ID", pBill->OrderBillID).CatChar(')');
				rStatus.DocStatus = statIsSuchDoc;
			}
			else {
				PPLoadText(PPTXT_EDIRELORDERNFOUNT, temp_buf);
			}
		}
        msg_buf.CatDiv(';', 2).Cat(temp_buf);
		Logger.Log(msg_buf);
	}
	return ok;
}

int PPBillImporter::CheckBill(const Sdr_Bill * pBill)
{
	int    ok = 0;
	if(pBill) {
		PPBillPacket pack;
		if(BillToBillRec(pBill, &pack) && Period.CheckDate(pack.Rec.Dt)) {
			if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, 0, 0) <= 0) // 5121033726
				ok = 1;
			else {
				SString msg_buf;
				PPObjBill::MakeCodeString(&pack.Rec, PPObjBill::mcsAddLocName|PPObjBill::mcsAddOpName, msg_buf);
				Logger.LogMsgCode(mfError, PPERR_DOC_ALREADY_EXISTS, msg_buf.cptr());
			}
		}
	}
	return ok;
}

int PPBillImporter::BillToBillRec(const Sdr_Bill * pBill, PPBillPacket * pPack)
{
	int    ok = 0;
	if(pBill && pPack) {
		SString temp_buf;
		PPID   ar_id = 0;
		ArticleTbl::Rec ar_rec;
		PersonTbl::Rec psn_rec;
		PPOprKind op_rec;
		PPObjAccSheet acs_obj;
		PPAccSheet acs_rec;
		THROW(GetOpData(OpID, &op_rec) > 0);
		const PPID acs_id = op_rec.AccSheetID;
		if(acs_id && acs_obj.Fetch(acs_id, &acs_rec) > 0) {
			if(pBill->CntragID && ArObj.Search(pBill->CntragID, &ar_rec) > 0 && ar_rec.AccSheetID == acs_id)
				ar_id = pBill->CntragID;
			else if(pBill->CntragNo && acs_id && ArObj.P_Tbl->SearchNum(acs_id, pBill->CntragNo, &ar_rec) > 0)
				ar_id = ar_rec.ID;
			else if(pBill->RegistryCode[0]) {
				if(acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(acs_rec.ObjGroup, &pk_rec) > 0 && pk_rec.CodeRegTypeID)
						ArObj.SearchByRegCode(acs_id, pk_rec.CodeRegTypeID, pBill->RegistryCode, &ar_id, 0);
				}
			}
			if(!ar_id) {
				// Так, даже если персоналия найдена по GLN, выведет ошибку, что она не найдена по INN
				//if(!ar_id)
				//	ResolveINN(pBill->INN, pBill->ID, acs_id, &ar_id);
				//if(!ar_id /*&& (!isempty(pBill->GLN) || !isempty(pBill->LocCode))*/)
				//	ResolveGLN(pBill->GLN, pBill->LocCode, pBill->ID, acs_id, &ar_id);
				//
				// А вот так ошибка выведется, только если она есть
				// Если INN пустое, то ищем по GLN
				if(!ar_id && isempty(pBill->INN)) {
					if(!isempty(pBill->GLN))
						ResolveGLN(pBill->GLN, pBill->ID, acs_id, &ar_id);
					if(!ar_id && !isempty(pBill->LocCode)) {
						//int PPObjLocation::ResolveWarehouseByCode(const char * pCode, PPID accSheetID, PPID * pArID)
						PPIDArray loc_list;
						LocationTbl::Rec loc_rec;
						LocObj.P_Tbl->GetListByCode(LOCTYP_WAREHOUSE, pBill->LocCode, &loc_list);
						if(loc_list.getCount() && LocObj.Fetch(loc_list.at(0), &loc_rec))
							ArObj.GetByPerson(acs_id, loc_rec.OwnerID, &ar_id);
					}
				}
				// Иначе ищем по INN
				if(!ar_id)
					ResolveINN(pBill->INN, pBill->DlvrAddrID, pBill->DlvrAddrCode, pBill->ID, acs_id, &ar_id);
			}
			if(!ar_id && pBill->DlvrAddrID) {
				LocationTbl::Rec loc_rec;
				PPID   temp_ar_id = 0;
				if(LocObj.Fetch(pBill->DlvrAddrID, &loc_rec) && ArObj.GetByPerson(acs_id, loc_rec.OwnerID, &temp_ar_id) > 0)
					ar_id = temp_ar_id;
			}
			if(acs_rec.Assoc == PPOBJ_PERSON) {
				if(!ar_id && pBill->CntragEMail[0]) {
					PPIDArray psn_by_em_list;
					PPIDArray loc_by_em_list;
					PsnObj.SearchEmail(pBill->CntragEMail, 0, &psn_by_em_list, /*&loc_by_em_list*/0);
					if(psn_by_em_list.getCount()) {
						for(uint i = 0; !ar_id && i < psn_by_em_list.getCount(); i++) {
							const PPID psn_by_em_id = psn_by_em_list.get(i);
							if(PsnObj.Fetch(psn_by_em_id, &psn_rec) > 0) {
								PPID temp_ar_id = 0;
								if(ArObj.P_Tbl->PersonToArticle(psn_by_em_id, acs_id, &temp_ar_id) > 0)
									ar_id = temp_ar_id;
							}
						}
					}
				}
				if(!ar_id && pBill->CntragPhone[0]) {

				}
			}
		}
		if(ar_id || !acs_id) {
			pPack->Rec.Object = ar_id;
			const PPID psn_id = ObjectToPerson(ar_id, 0);
			if(op_rec.AccSheet2ID) {
				ArticleTbl::Rec ar_rec;
				PPID   obj2id = 0;
				if(pBill->Obj2No && ArObj.P_Tbl->SearchNum(op_rec.AccSheet2ID, pBill->Obj2No, &ar_rec) > 0)
					pPack->Rec.Object2 = ar_rec.ID;
				else if(pBill->Obj2INN[0] && ResolveINN(pBill->Obj2INN, 0, 0, pBill->ID, op_rec.AccSheet2ID, &obj2id, 0) > 0)
					pPack->Rec.Object2 = obj2id;
				else if(pBill->Obj2GLN[0] && ResolveGLN(pBill->Obj2GLN, pBill->ID, op_rec.AccSheet2ID, &obj2id, 0) > 0)
					pPack->Rec.Object2 = obj2id;
			}
			SETIFZ(pPack->Rec.Dt, pBill->Date);
			SETIFZ(pPack->Rec.OpID, OpID);
			SETIFZ(pPack->Rec.LocID, LocID);
			if(isempty(pBill->LocID) && !isempty(pBill->LocCode)) {
				PPID loc_id = 0;
				if(LocObj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, pBill->LocCode, &loc_id) > 0)
					pPack->Rec.LocID = loc_id;
			}
			if(!isempty(pBill->OrderBillID)) {
				const PPID link_bill_id = atol(pBill->OrderBillID);
				BillTbl::Rec link_bill_rec;
				if(P_BObj->Search(link_bill_id, &link_bill_rec) > 0 && GetOpType(link_bill_rec.OpID) == PPOPT_DRAFTRECEIPT)
					pPack->Rec.LinkBillID = link_bill_id;
			}
			pPack->Rec.DueDate = pBill->DueDate;
			STRNSCPY(pPack->Rec.Code, pBill->Code);
			if(pBill->InvoiceCode[0]) {
				STRNSCPY(pPack->Ext.InvoiceCode, pBill->InvoiceCode);
			}
			if(checkdate(pBill->InvoiceDate))
				pPack->Ext.InvoiceDate = pBill->InvoiceDate;
			pPack->Rec.Amount  = pBill->Amount;
			pPack->Rec.CurID   = pBill->CurID;
			pPack->Rec.CRate   = pBill->CRate;
			if(checkdate(pBill->PaymDate) && pBill->Amount)
				pPack->SetPayDate(pBill->PaymDate, pBill->Amount);
			// @v11.1.12 STRNSCPY(pPack->Rec.Memo, (temp_buf = pBill->Memo).Transf(CTRANSF_OUTER_TO_INNER));
			pPack->SMemo = (temp_buf = pBill->Memo).Transf(CTRANSF_OUTER_TO_INNER); // @v11.1.12
			// @v10.9.6 (temp_buf = pPack->Rec.Code).Transf(CTRANSF_OUTER_TO_INNER);
			// @v10.9.6 STRNSCPY(pPack->Rec.Code, temp_buf);
			(temp_buf = pPack->Ext.InvoiceCode).Transf(CTRANSF_OUTER_TO_INNER);
			STRNSCPY(pPack->Ext.InvoiceCode, temp_buf);
			{
				PPID	agent_id = 0;
				if(pBill->AgentPersonID && ArObj.P_Tbl->PersonToArticle(pBill->AgentPersonID, GetAgentAccSheet(), &agent_id) > 0)
					pPack->Ext.AgentID = agent_id;
				else if(pBill->AgentINN[0] && ResolveINN(pBill->AgentINN, 0, 0, pBill->ID, GetAgentAccSheet(), &agent_id, 0) > 0)
					pPack->Ext.AgentID = agent_id;
				else if(pBill->AgentGLN[0] && ResolveGLN(pBill->AgentGLN, pBill->ID, GetAgentAccSheet(), &agent_id, 0) > 0)
					pPack->Ext.AgentID = agent_id;
			}
			{
				int    dlvr_loc_id_proof = 0;
				if(psn_id && (pBill->DlvrAddrID || pBill->DlvrAddrCode[0])) {
					LocationTbl::Rec dlvr_loc_rec;
					if(pBill->DlvrAddrID && LocObj.Search(pBill->DlvrAddrID, &dlvr_loc_rec) > 0) {
						if(dlvr_loc_rec.OwnerID == psn_id)
							dlvr_loc_id_proof = dlvr_loc_rec.ID;
					}
					if(!dlvr_loc_id_proof && pBill->DlvrAddrCode[0]) {
						PPID loc_id = 0;
						if(LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, pBill->DlvrAddrCode, &loc_id) > 0) {
							if(LocObj.Search(loc_id, &dlvr_loc_rec) > 0) {
								if(dlvr_loc_rec.OwnerID == psn_id)
									dlvr_loc_id_proof = dlvr_loc_rec.ID;
							}
						}
					}
				}
				if(dlvr_loc_id_proof && op_rec.Flags & OPKF_FREIGHT)
					pPack->SetFreight_DlvrAddrOnly(dlvr_loc_id_proof);
			}
			if(Flags & PPBillImporter::fEdiImpExp)
				pPack->Rec.Flags2 |= BILLF2_DONTCLOSDRAFT;
			pPack->Rec.EdiOp = static_cast<int16>(pBill->EdiOp);
			ok = 1;
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::DoFullEdiProcess()
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	PPEdiProcessor::ProviderImplementation * p_prvimp = 0;
	SString temp_buf;
	SString msg_buf;
	ObjIdListFilt suppl_list;
	ObjIdListFilt cli_list;
	PPIDArray temp_id_list;
	PPSupplAgreement suppl_agt;
	PPClientAgreement cli_agt;
	PPObjEdiProvider ediprv_obj;
	PPEdiProvider ediprv_rec;
	PPEdiProviderPacket ediprv_pack;
	PPBillIterchangeFilt be_filt;
	be_filt.LocID = this->LocID;
	be_filt.Period = this->Period;
	PPID   main_org_id = GetMainOrgID();
	long   ediprvimp_ctr_flags = 0;
	if(Flags & fTestMode)
		ediprvimp_ctr_flags |= PPEdiProcessor::ProviderImplementation::ctrfTestMode;
	for(SEnum en = ediprv_obj.Enum(0); en.Next(&ediprv_rec) > 0;) {
		p_prvimp = (ediprv_rec.Flags & PPEdiProvider::fPassive) ? 0 : PPEdiProcessor::CreateProviderImplementation(ediprv_rec.ID, main_org_id, ediprvimp_ctr_flags);
		if(p_prvimp) {
			PPEdiProcessor prc(p_prvimp, &Logger);
			PPEdiProcessor::DocumentInfo doc_inf;
			PPEdiProcessor::DocumentInfoList doc_info_list;
			if(!prc.GetDocumentList(be_filt, doc_info_list)) {
				Logger.LogLastError();
			}
			else if(doc_info_list.GetCount()) {
				TSCollection <PPEdiProcessor::Packet> doc_pack_list;
				for(uint i = 0; i < doc_info_list.GetCount(); i++) {
					if(doc_info_list.GetByIdx(i, doc_inf)) {
						PPEdiProcessor::Packet doc_pack(doc_inf.EdiOp);
						if(!prc.ReceiveDocument(&doc_inf, doc_pack_list))
							Logger.LogLastError();
					}
				}
				if(doc_pack_list.getCount()) {
					for(uint didx = 0; didx < doc_pack_list.getCount(); didx++) {
						PPEdiProcessor::Packet * p_pack = doc_pack_list.at(didx);
						if(p_pack) {
							if(p_pack->DocType == PPEDIOP_ORDER) {
								PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_pack->P_Data);
								if(p_bp && ((!this->LocID || p_bp->Rec.LocID == this->LocID) /*&& this->Period.CheckDate(p_bp->Rec.Dt)*/)) {
									PPID   ex_bill_id = 0;
									BillTbl::Rec ex_bill_rec;
									if(P_BObj->P_Tbl->SearchAnalog(&p_bp->Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
										PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName, temp_buf).Quot('(', ')');
										if(PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, temp_buf, 1, msg_buf))
											Logger.Log(msg_buf);
									}
									else {
										int    is_valuation_modif = 0;
										if(CheckOpFlags(p_bp->Rec.OpID, OPKF_NEEDVALUATION))
											P_BObj->AutoCalcPrices(p_bp, 0, &is_valuation_modif);
										if(P_BObj->__TurnPacket(p_bp, 0, 1, 1))
											Logger.LogAcceptMsg(PPOBJ_BILL, p_bp->Rec.ID, 0);
										else
											Logger.LogLastError();
									}
								}
							}
							else if(p_pack->DocType == PPEDIOP_ORDERRSP) {
								PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_pack->P_Data);
								PPBillPacket * p_bp_org = static_cast<PPBillPacket *>(p_pack->P_ExtData);
								assert(p_bp);
								assert(p_bp_org);
								if(p_bp && p_bp_org && p_bp_org->Rec.ID) {
									const PPID order_bill_id = p_bp_org->Rec.ID;
									if(p_ref->Ot.GetTagStr(PPOBJ_BILL, order_bill_id, PPTAG_BILL_EDIORDRSPRCV, temp_buf) > 0) {
									}
									else {
										int   all_equal = 1;
										int   prices_equal = 1;
										int   exc_items = 0; // Признак того, что в ORDRSP присутствуют лишние строки
										for(uint i = 0; all_equal && i < p_bp_org->GetTCount(); i++) {
											const PPTransferItem & r_org_ti = p_bp_org->ConstTI(i);
											uint rsp_pos = 0;
											if(p_bp->SearchTI(r_org_ti.RByBill, &rsp_pos)) {
												const PPTransferItem & r_rsp_ti = p_bp->ConstTI(rsp_pos);
												if(r_rsp_ti.GoodsID != r_org_ti.GoodsID)
													all_equal = 0;
												else if(!feqeps(r_rsp_ti.Quantity_, r_org_ti.Quantity_, 1E-7))
													all_equal = 0;
												else if(r_rsp_ti.Cost != r_org_ti.Cost)
													prices_equal = 0;
											}
											else
												all_equal = 0;
										}
										for(uint j = 0; !exc_items && j < p_bp->GetTCount(); j++) {
											const PPTransferItem & r_rsp_ti = p_bp->ConstTI(j);
											uint org_pos = 0;
											if(!p_bp_org->SearchTI(r_rsp_ti.RByBill, &org_pos))
												exc_items = 1;
										}
										{
											ObjTagItem tag_item;
											temp_buf.Z();
											temp_buf.Cat(all_equal ? "ACCEPTED" : "CHANGED");
											temp_buf.Space().Cat(p_bp->Rec.Code).Space().Cat(p_bp->Rec.Dt, DATF_ISO8601CENT);
											if(!tag_item.SetStr(PPTAG_BILL_EDIORDRSPRCV, temp_buf) || !p_ref->Ot.PutTag(PPOBJ_BILL, order_bill_id, &tag_item, 1)) {
												Logger.LogLastError();
											}
											else if(!all_equal || exc_items) {
												assert(p_bp->Rec.LinkBillID == p_bp_org->Rec.ID); // Связка должна была быть установлена ранее
												p_bp->Rec.LinkBillID = p_bp_org->Rec.ID; // Несмотря на параноидальную проверку выше, страхуемся
												if(!P_BObj->__TurnPacket(p_bp, 0, 0, 1)) {
													Logger.LogLastError();
												}
											}
										}
									}
								}
							}
							else if(p_pack->DocType == PPEDIOP_DESADV) {
								PPBillPacket * p_bp = static_cast<PPBillPacket *>(p_pack->P_Data);
								if(p_bp && ((!this->LocID || p_bp->Rec.LocID == this->LocID) /*&& this->Period.CheckDate(p_bp->Rec.Dt)*/)) {
									PPID   ex_bill_id = 0;
									BillTbl::Rec ex_bill_rec;
									if(P_BObj->P_Tbl->SearchAnalog(&p_bp->Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
										PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName, temp_buf).Quot('(', ')');
										if(PPGetMessage(mfError, PPERR_DOC_ALREADY_EXISTS, temp_buf, 1, msg_buf))
											Logger.Log(msg_buf);
									}
									else {
										int    is_valuation_modif = 0;
										if(CheckOpFlags(p_bp->Rec.OpID, OPKF_NEEDVALUATION))
											P_BObj->AutoCalcPrices(p_bp, 0, &is_valuation_modif);
										if(P_BObj->__TurnPacket(p_bp, 0, 1, 1))
											Logger.LogAcceptMsg(PPOBJ_BILL, p_bp->Rec.ID, 0);
										else
											Logger.LogLastError();
									}
								}
							}
							else if(p_pack->DocType == PPEDIOP_RECADV) {
								PPEdiProcessor::RecadvPacket * p_recadv_pack = static_cast<PPEdiProcessor::RecadvPacket *>(p_pack->P_Data);
								if(p_recadv_pack && p_recadv_pack->DesadvBillCode.NotEmpty() && checkdate(p_recadv_pack->DesadvBillDate)) {
									PPID   desadv_bill_id = 0;
									BillTbl::Rec desadv_bill_rec;
									BillTbl::Rec desadv_bill_rec_pattern;
									// @v10.7.9 @ctr MEMSZERO(desadv_bill_rec_pattern);
									STRNSCPY(desadv_bill_rec_pattern.Code, p_recadv_pack->DesadvBillCode);
									desadv_bill_rec_pattern.Dt = p_recadv_pack->DesadvBillDate;
									desadv_bill_rec_pattern.Object = p_recadv_pack->RBp.Rec.Object;
									if(P_BObj->P_Tbl->SearchAnalog(&desadv_bill_rec_pattern, BillCore::safCheckEdiOp|BillCore::safIgnoreOp, &desadv_bill_id, &desadv_bill_rec) > 0) {
										if(p_ref->Ot.GetTagStr(PPOBJ_BILL, desadv_bill_id, PPTAG_BILL_EDIRECADVRCV, temp_buf) > 0) {
										}
										else {
											ObjTagItem tag_item;
											temp_buf.Z();
											temp_buf.Cat(p_recadv_pack->AllRowsAccepted ? "ACCEPTED" : "CHANGED");
											temp_buf.Space().Cat(p_recadv_pack->RBp.Rec.Code).Space().Cat(p_recadv_pack->RBp.Rec.Dt, DATF_ISO8601CENT);
											if(!tag_item.SetStr(PPTAG_BILL_EDIRECADVRCV, temp_buf) || !p_ref->Ot.PutTag(PPOBJ_BILL, desadv_bill_id, &tag_item, 1)) {
												Logger.LogLastError();
											}
											else if(!p_recadv_pack->AllRowsAccepted) {
												p_recadv_pack->RBp.Rec.LinkBillID = desadv_bill_id;
												if(!P_BObj->__TurnPacket(&p_recadv_pack->RBp, 0, 0, 1)) {
													Logger.LogLastError();
												}
											}
										}
									}
								}
							}
							else if(p_pack->DocType == PPEDIOP_INVOIC) {
								; // @todo
							}
						}
					}
				}
			}
			if(ediprv_rec.Symb[0]) {
				if(!suppl_list.IsExists()) {
					ArObj.P_Tbl->GetListBySheet(GetSupplAccSheet(), &temp_id_list, 0);
					suppl_list.Set(&temp_id_list);
				}
				temp_id_list.clear();
				for(uint i = 0; i < suppl_list.GetCount(); i++) {
					const PPID ar_id = suppl_list.Get(i);
					if(ArObj.GetSupplAgreement(ar_id, &suppl_agt, 0) > 0 &&
						suppl_agt.Ep.GetExtStrData(PPSupplAgreement::ExchangeParam::extssEDIPrvdrSymb, temp_buf) && temp_buf.IsEqiAscii(ediprv_rec.Symb)) {
						temp_id_list.add(ar_id);
					}
				}
				if(temp_id_list.getCount()) {
					// temp_id_list содержит идентификаторы поставщиков, которые, согласно соглашению, используют данного провайдера
					prc.SendOrders(be_filt, temp_id_list);
					prc.SendRECADV(be_filt, temp_id_list); // @v10.2.12
				}
			}
			{
				temp_id_list.clear();
				if(!cli_list.IsExists()) {
					ArObj.P_Tbl->GetListBySheet(GetSellAccSheet(), &temp_id_list, 0);
					cli_list.Set(&temp_id_list);
				}
				temp_id_list.clear();
				for(uint i = 0; i < cli_list.GetCount(); i++) {
					const PPID ar_id = cli_list.Get(i);
					if(ArObj.GetClientAgreement(ar_id, cli_agt, 0) > 0 && cli_agt.EdiPrvID == ediprv_rec.ID)
						temp_id_list.add(ar_id);
				}
				if(temp_id_list.getCount()) {
					// temp_id_list содержит идентификаторы покупателей, которые, согласно соглашению, используют данного провайдера
					prc.SendOrderRsp(be_filt, temp_id_list);
					prc.SendDESADV(PPEDIOP_DESADV, be_filt, temp_id_list);
					if(ediprv_rec.SuppOpFlags & (1 << PPEDIOP_INVOIC)) {
						prc.SendDESADV(PPEDIOP_INVOIC, be_filt, temp_id_list);
					}
					if(ediprv_rec.SuppOpFlags & (1 << PPEDIOP_ALCODESADV)) {
						prc.SendDESADV(PPEDIOP_ALCODESADV, be_filt, temp_id_list);
					}
				}
			}
		}
		ZDELETE(p_prvimp);
	}
	ZDELETE(p_prvimp);
	return ok;
}

class PredefImportRecord_Coke {
public:
	static int FASTCALL Compare(const void * p1, const void * p2, void * extraData)
	{
		const PredefImportRecord_Coke * i1 = static_cast<const PredefImportRecord_Coke *>(p1);
		const PredefImportRecord_Coke * i2 = static_cast<const PredefImportRecord_Coke *>(p2);
		return strcmp(i1->OrderIdent, i2->OrderIdent);
	}
	PredefImportRecord_Coke() : DeliveryDate(ZERODATE), OrderDate(ZERODATE), QtyCase(0.0), QtyBottle(0.0), FreeCase(0.0), 
		Discount(0.0), PriceCase(0.0), PriceEA(0.0), CasesDiscounted(0.0), PcsDiscounted(0.0), GrossValue(0.0)
	{
	}
	PredefImportRecord_Coke & Z()
	{
		DeliveryDate = ZERODATE;
		OrderDate = ZERODATE;
		QtyCase = 0.0;
		QtyBottle = 0.0;
		FreeCase = 0.0;
		Discount = 0.0;
		PriceCase = 0.0;
		PriceEA = 0.0;
		CasesDiscounted = 0.0;
		PcsDiscounted = 0.0;
		GrossValue = 0.0;
		InputChannel.Z();
		DiscountPromoIdent.Z();
		ACTGRINUM.Z();
		OrderIdent.Z();
		ClientIdent.Z();
		ClientName.Z();
		ClientAddr.Z();
		ClientDistrib.Z();
		City.Z();
		ProductCode.Z();
		ProductName.Z();
		DTC.Z();
		SalesRepIdent.Z();
		INN.Z();
		return *this;
	}
	bool   Read(const xmlNode * pParentNode)
	{
		Z();
		SString temp_buf;
		for(const xmlNode * p_c = pParentNode->children; p_c; p_c = p_c->next) {
			if(SXml::GetContentByName(p_c, "ORDERID", temp_buf)) {
				OrderIdent = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "DELIVERY_DATE", temp_buf)) {
				DeliveryDate = strtodate_(temp_buf, DATF_GERMAN);
			}
			else if(SXml::GetContentByName(p_c, "SALES_REPID", temp_buf)) {
				SalesRepIdent = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CLIENTID", temp_buf)) {
				ClientIdent = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CLIENT_NAME", temp_buf)) {
				ClientName = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CLIENT_ADDRESS", temp_buf)) {
				ClientAddr = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "PRODUCT_CODE", temp_buf)) {
				ProductCode = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "PRODUCT_NAME", temp_buf)) {
				ProductName = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "QTY_CASES", temp_buf)) {
				QtyCase = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "DTC", temp_buf)) {
				DTC = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CLIENTID_DISTRIB", temp_buf)) {
				ClientDistrib = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "FISCAL_NUMBER", temp_buf)) {
				INN = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "QTY_BOTTLES", temp_buf)) {
				QtyBottle = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "ORDER_DATE", temp_buf)) {
				OrderDate = strtodate_(temp_buf, DATF_GERMAN);
			}
			else if(SXml::GetContentByName(p_c, "FREE_CASE", temp_buf)) {
				FreeCase = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "DISCOUNT", temp_buf)) {
				Discount = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "PRICE_CASE", temp_buf)) {
				PriceCase = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "PRICE_EA", temp_buf)) {
				PriceEA = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "ACTGRINUM", temp_buf)) {
				ACTGRINUM = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CITY", temp_buf)) {
				City = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "CASES_DISCOUNTED", temp_buf)) {
				CasesDiscounted = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "PCS_DISCOUNTED", temp_buf)) {
				PcsDiscounted = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "DISCOUNT_PROMO_ID", temp_buf)) {
				DiscountPromoIdent = temp_buf;
			}
			else if(SXml::GetContentByName(p_c, "GROSS_VALUE", temp_buf)) {
				GrossValue = temp_buf.ToReal();
			}
			else if(SXml::GetContentByName(p_c, "INPUT_CHANNEL", temp_buf)) {
				InputChannel = temp_buf;
			}
		}
		return (OrderIdent.NotEmpty() && ProductCode.NotEmpty() && (QtyBottle > 0.0 || QtyCase > 0.0));
	}
	LDATE  DeliveryDate;
	LDATE  OrderDate;
	double QtyCase;         // Количество упаковок
	double QtyBottle;       // Количество единиц
	double FreeCase;        // Количество бесплатных упаковок (промо)
	double Discount;        // Скидка на упаковку без НДС
	double PriceCase;       // Цена за упаковку без НДС 
	double PriceEA;         // Цена за единицу без НДС
	double CasesDiscounted; //  
	double PcsDiscounted;   //
	double GrossValue;      // Сумма с НДС
	SString InputChannel;
	SString DiscountPromoIdent;
	SString ACTGRINUM;
	SString OrderIdent;
	SString ClientIdent;
	SString ClientName;
	SString ClientAddr;
	SString ClientDistrib; // Специальная конструкция cliid_dlvraddrid : то есть, наш идентификатор клиента и наш же ид адреса, разделенные символом '_'
	SString City;
	SString ProductCode;
	SString ProductName;
	SString DTC;
	SString SalesRepIdent;
	SString INN/*FiscalNumber*/;
};

int PPBillImporter::Helper_AcceptCokeData(const SCollection * pRowList, PPID opID, PPID supplArID)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	if(pRowList && pRowList->getCount()) {
		//rec_list.sort(PredefImportRecord_Coke::Compare);
		SString prev_order_ident;
		SString temp_buf;
		SString msg_buf;
		SString left_buf, right_buf;
		ArticleTbl::Rec ar_rec;
		PPID   agent_tag_id = 0;
		PPID   promo_lot_tag_id = 0;
		LongArray row_pos_list;
		PPObjectTag tag_rec;
		PPOprKind op_rec;
		THROW(GetOpData(opID, &op_rec) > 0);
		if(TagObj.SearchBySymb("AGENT-COKE", &agent_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_PERSON) {
			;
		}
		else
			agent_tag_id = 0;
		if(TagObj.SearchBySymb("COKE-PROMOLOTCODE", &promo_lot_tag_id, &tag_rec) > 0 && tag_rec.ObjTypeID == PPOBJ_LOT) {
			;
		}
		else
			promo_lot_tag_id = 0;
		for(uint rowidx = 0; rowidx < pRowList->getCount();) { // Инкремента нет: он осуществляется во внутреннем цикле do {} while()
			const uint preserve_idx = rowidx;
			const PredefImportRecord_Coke * p_item = static_cast<const PredefImportRecord_Coke *>(pRowList->at(rowidx));
			assert(p_item);
			//if(p_item->OrderIdent != prev_order_ident) {
			PPID   ar_id = 0;
			PPID   dlvr_loc_id = 0;
			PPBillPacket::SetupObjectBlock sob;
			PPBillPacket pack;
			const SString current_order_ident = p_item->OrderIdent;
			pack.CreateBlank_WithoutCode(BillParam.ImpOpID, 0, LocID, 0);
			STRNSCPY(pack.Rec.Code, p_item->OrderIdent);
			pack.Rec.Dt = checkdate(p_item->OrderDate) ? p_item->OrderDate : getcurdate_();
			if(checkdate(p_item->DeliveryDate))
				pack.Rec.DueDate = p_item->DeliveryDate;
			if(p_item->ClientDistrib.NotEmpty() && p_item->ClientDistrib.Divide('_', left_buf, right_buf) > 0) {
				if(ArObj.Search(left_buf.ToLong(), &ar_rec) > 0) { // @v11.4.3 Fetch-->Search
					ar_id = ar_rec.ID;
					dlvr_loc_id = right_buf.ToLong();
				}
			}
			if(!ar_id && p_item->INN.NotEmpty()) {
				ResolveINN(p_item->INN, 0, 0, 0, op_rec.AccSheetID, &ar_id, 0);
			}
			if(ar_id) {
				// @v11.4.3 {
				if(GetOpType(opID) == PPOPT_GOODSORDER)
					sob.Flags |= PPBillPacket::SetupObjectBlock::fEnableStop;
				// } @v11.4.3 
				if(!pack.SetupObject(ar_id, sob)) {
					Logger.LogLastError();
					rowidx++; // @important // @v11.4.7 @fix
				}
				else {
					if(dlvr_loc_id) {
						pack.SetFreight_DlvrAddrOnly(dlvr_loc_id);
					}
					if(agent_tag_id && p_item->DTC.NotEmpty()) {
						PPIDArray agent_psn_list;
						if(p_ref->Ot.SearchObjectsByStr(PPOBJ_PERSON, agent_tag_id, p_item->DTC, &agent_psn_list) > 0) {
							assert(agent_psn_list.getCount());
							for(uint apidx = 0; apidx < agent_psn_list.getCount(); apidx++) {
								const PPID apid = agent_psn_list.get(apidx);
								PPID  agent_ar_id = 0;
								if(ArObj.P_Tbl->PersonToArticle(apid, GetAgentAccSheet(), &agent_ar_id) > 0) {
									assert(agent_ar_id > 0);
									pack.Ext.AgentID = agent_ar_id;
									break;
								}
							}
						}
					}
					bool skip_this_doc = false;
					if(!Period.IsZero() && !Period.CheckDate(pack.Rec.Dt)) {
						skip_this_doc = true;
						rowidx++; // @important // @v11.4.7 @fix
					}
					else {
						do {
							PPTransferItem ti;
							Goods2Tbl::Rec goods_rec;
							ti.Init(&pack.Rec, 0, 0);
							if(p_item->ProductCode.NotEmpty()) {
								ArGoodsCodeTbl::Rec agc_rec;
								if(GObj.P_Tbl->SearchByArCode(supplArID, p_item->ProductCode, &agc_rec, &goods_rec) > 0) {
									ti.SetupGoods(goods_rec.ID);
								}
								else {
									if(BillParam.Flags & PPBillImpExpParam::fCreateAbsenceGoods) {
										ResolveGoodsItem rgi;
										rgi.ArID = supplArID;
										STRNSCPY(rgi.ArCode, p_item->ProductCode);
										(temp_buf = p_item->ProductName).Transf(CTRANSF_UTF8_TO_INNER);
										STRNSCPY(rgi.GoodsName, temp_buf);
										if(CreateAbsenceGoods(rgi, 1) > 0) {
											assert(rgi.ResolvedGoodsID);
											ti.SetupGoods(rgi.ResolvedGoodsID);
										}
									}
								}
							}
							if(ti.GoodsID && GObj.Fetch(ti.GoodsID, &goods_rec) > 0) {
								double unit_per_pack = 0.0;
								double net_price_wo_vat = 0.0;
								double discount_wo_vat = 0.0;
								ti.Quantity_ = p_item->QtyBottle;
								if(p_item->QtyCase > 0.0 && p_item->QtyBottle > 0) {
									unit_per_pack = p_item->QtyBottle / p_item->QtyCase;
								}
								if(p_item->PriceEA > 0.0) {
									net_price_wo_vat = p_item->PriceEA;
								}
								else if(p_item->PriceCase > 0.0 && unit_per_pack > 0.0) {
									net_price_wo_vat = p_item->PriceCase / unit_per_pack;
								}
								if(p_item->Discount > 0.0 && unit_per_pack > 0.0) { // Скидка без НДС на упаковку
									discount_wo_vat = p_item->Discount / unit_per_pack;
								}
								{
									GTaxVect vect;
									PPGoodsTaxEntry gtx;
									if(GObj.GTxObj.FetchByID(goods_rec.TaxGrpID, &gtx) > 0) {
										vect.Calc_(&gtx, net_price_wo_vat+discount_wo_vat, 1.0, GTAXVF_AFTERTAXES, 0);
										ti.Price = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE | GTAXVF_VAT);
										if(discount_wo_vat > 0.0) {
											vect.Calc_(&gtx, discount_wo_vat, 1.0, GTAXVF_AFTERTAXES, 0);
											ti.Discount = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE | GTAXVF_VAT);
										}
									}
									else {
										ti.Price = (net_price_wo_vat + discount_wo_vat);
										ti.Discount = discount_wo_vat;
									}
								}
								row_pos_list.Z();
								pack.InsertRow(&ti, &row_pos_list);
								if(promo_lot_tag_id && row_pos_list.getCount() == 1 && p_item->DiscountPromoIdent.NotEmpty()) {
									const long row_pos = row_pos_list.get(0);
									ObjTagList * p_lot_tag_list = pack.LTagL.Get(row_pos);
									ObjTagList stub_lot_tag_list;
									SETIFZQ(p_lot_tag_list, &stub_lot_tag_list);
									p_lot_tag_list->PutItemStr(promo_lot_tag_id, p_item->DiscountPromoIdent);
									pack.LTagL.Set(row_pos, p_lot_tag_list);
								}
							}
							else
								skip_this_doc = false;
							// Теперь инкрементируем индекс строки и после этого пытаемся получить следующую (если индекс не вышел за пределы списка).
							rowidx++; 
							p_item = (rowidx < pRowList->getCount()) ? static_cast<const PredefImportRecord_Coke *>(pRowList->at(rowidx)) : 0;
						} while(p_item && p_item->OrderIdent == current_order_ident);
					}
					if(!skip_this_doc) {
						BillTbl::Rec ex_bill_rec;
						PPID   ex_bill_id = 0;
						if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safIgnoreDate, &ex_bill_id, &ex_bill_rec) > 0) {
							PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, msg_buf);
							Logger.LogMsgCode(mfError, PPERR_DOC_ALREADY_EXISTS, msg_buf);
						}
						else if(P_BObj->__TurnPacket(&pack, 0, 1, 1))
							Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
						else
							Logger.LogLastError();
					}
				}
			}
			else {
				msg_buf.Z().Cat(p_item->OrderIdent).Space().Cat(p_item->OrderDate, DATF_DMY|DATF_CENTURY);
				if(p_item->INN.NotEmpty())
					msg_buf.Space().CatEq("INN", p_item->INN);
				(temp_buf = p_item->ClientName).Transf(CTRANSF_UTF8_TO_INNER);
				if(temp_buf.NotEmpty()) {
					msg_buf.Space().Cat(temp_buf);
				}
				PPSetError(PPERR_CANTIDENTIMPBILLAR, msg_buf);
				Logger.LogLastError();
				rowidx++; // @important
			}
			//}
			// @v11.4.7 @fix {
			if(rowidx == preserve_idx)
				rowidx++;
			// } @v11.4.7 @fix 
			assert(rowidx > preserve_idx || (rowidx < pRowList->getCount())); // Защита от зацикливания //
		}
	}
	CATCHZOK
	return ok;
}

int PPBillImporter::Run()
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString file_name;
	SString temp_buf;
	SString msg_buf;
	LineIdSeq = 0;
	PPWaitStart();
	Period.Actualize(ZERODATE);
	ToRemoveFiles.clear();
	if(Flags & PPBillImporter::fUhttImport) {
		THROW(RunUhttImport());
	}
	else if(Flags & PPBillImporter::fEgaisImpExp) {
		long   cflags = (Flags & PPBillImporter::fTestMode) ? PPEgaisProcessor::cfDebugMode : 0;
		if(Flags & PPBillImporter::fEgaisVer4) { // @v11.0.12
			cflags |= PPEgaisProcessor::cfVer4;
			cflags &= ~PPEgaisProcessor::cfVer3;
		}
		else if(Flags & PPBillImporter::fEgaisVer3) {
			cflags |= PPEgaisProcessor::cfVer3;
			cflags &= ~PPEgaisProcessor::cfVer4;
		}
		PPEgaisProcessor ep(cflags, &Logger, 0);
		THROW(ep);
		if(Flags & PPBillImporter::fDontRemoveTags)
			ep.SetNonRvmTagMode(1);
		THROW(ep.CheckLic());
		{
			PPBillIterchangeFilt sbp;
			sbp.LocID = LocID;
			sbp.Period = Period;
			TSVector <PPEgaisProcessor::UtmEntry> utm_list;
			THROW(ep.GetUtmList(LocID, utm_list));
			PPWaitStart();
			for(uint i = 0; i < utm_list.getCount(); i++) {
                ep.SetUtmEntry(LocID, &utm_list.at(i), &Period);
				ep.SendBillActs(sbp);
				ep.SendBillRepeals(sbp);
				ep.SendBills(sbp);
				ep.ReadInput(LocID, &Period, 0);
				ep.SetUtmEntry(0, 0, 0);
			}
			PPWaitStop();
		}
	}
	else if(Flags & PPBillImporter::fChZnImpExp) { // @v10.6.4
		PPChZnPrcssr prcssr(&Logger);
		PPChZnPrcssr::Param param;
		param.GuaID = GuaID;
		param.LocID = LocID;
		param.Period = Period;
		THROW(prcssr.Run(param));
	}
	else if(Flags & PPBillImporter::fEdiImpExp && Flags & PPBillImporter::fFullEdiProcess) {
		THROW(DoFullEdiProcess());
	}
	else if(BillParam.PredefFormat == piefCokeOrder) { // @v11.3.8
		xmlParserCtxt * p_ctx = 0;
		StringSet ss_files;
		PPOprKind op_rec;
		PPID   suppl_ar_id = 0;
		THROW_PP_S(BillParam.ImpOpID, PPERR_UNDEFBILLIMPOP, BillParam.Name);
		THROW(GetOpData(BillParam.ImpOpID, &op_rec) > 0);
		THROW(BillParam.PreprocessImportFileSpec(ss_files));
		THROW_PP_S(BillParam.Object2SrchCode.NotEmpty(), PPERR_BILLIMPSRCCOD2NEEDED, BillParam.Name);
		THROW(ResolveINN(BillParam.Object2SrchCode, 0, 0, 0, GetSupplAccSheet(), &suppl_ar_id, 0)); // @err
		assert(suppl_ar_id);
		THROW(p_ctx = xmlNewParserCtxt());
		for(uint ssfp = 0; ss_files.get(&ssfp, temp_buf);) {
			if(fileExists(temp_buf)) {
				SString extra_key;
				SString extra_val;
				xmlDoc * p_xml_doc = xmlCtxtReadFile(p_ctx, temp_buf, 0, XML_PARSE_NOENT);
				if(p_xml_doc) {
					const xmlNode * p_root = xmlDocGetRootElement(p_xml_doc);
					if(p_root && SXml::IsName(p_root, "NewDataSet")) {
						TSCollection <PredefImportRecord_Coke> rec_list;
						for(const xmlNode * p_rec = p_root->children; p_rec; p_rec = p_rec->next) {
							if(SXml::IsName(p_rec, "Table")) {
								PredefImportRecord_Coke * p_new_rec = rec_list.CreateNewItem();
								THROW_SL(p_new_rec);
								if(!p_new_rec->Read(p_rec)) {
									assert(rec_list.getCount());
									rec_list.atFree(rec_list.getCount()-1);
								}
							}
						}
						// Важно: следующая функция закладывается на то, что строки rec_list
						// отсортированы по номеру документа. При этом, мы явно не сортируем строки
						// из-за того, что из-за этого собъется внутренний порядок строк заказа.
						// Следовательно, мы надеемся на то, что поставщик передает заказы в таком виде,
						// что строки одного заказа следуют одна-за-другой не перемешиваясь с иными заказами!
						Helper_AcceptCokeData(&rec_list, BillParam.ImpOpID, suppl_ar_id);
					}
					xmlFreeDoc(p_xml_doc);
					p_xml_doc = 0;
				}
				else {
					PPSetLibXmlError(p_ctx);
				}
			}
		}
		xmlFreeParserCtxt(p_ctx);
		//
	}
	else if(oneof3(BillParam.PredefFormat, piefNalogR, piefNalogR_ON_NSCHFDOPPRMARK, piefNalogR_ON_NSCHFDOPPR)) { // @v11.2.1 piefNalogR_ON_NSCHFDOPPR 
		DocNalogRu_Reader reader;
		StringSet ss_files;
		PPOprKind op_rec;
		TSCollection <DocNalogRu_Reader::DocumentInfo> doc_list;
		const PPGoodsConfig & r_gcfg = GObj.GetConfig();
		THROW_PP_S(BillParam.ImpOpID, PPERR_UNDEFBILLIMPOP, CfgNameBill);
		THROW(GetOpData(BillParam.ImpOpID, &op_rec) > 0);
		THROW(BillParam.PreprocessImportFileSpec(ss_files));
		for(uint ssfp = 0; ss_files.get(&ssfp, temp_buf);) {
			if(fileExists(temp_buf)) {
				DocNalogRu_Reader::FileInfo fi;
				reader.ReadFile(temp_buf, fi, doc_list);
			}
		}
		{
			const PPID  contragent_acs_id = op_rec.AccSheetID;
			const PPID  main_org_id = GetMainOrgID();
			if(!contragent_acs_id) {

			}
			else {
				for(uint docidx = 0; docidx < doc_list.getCount(); docidx++) {
					DocNalogRu_Reader::DocumentInfo * p_doc = doc_list.at(docidx);
					if(p_doc) {
						int    skip = 0;
						PPBillPacket pack;
						LDATE  init_bill_date = ZERODATE;
						SString init_bill_code;
						PPID   seller_ar_id = 0;
						PPID   buyer_ar_id = 0;
						if(checkdate(p_doc->Dt))
							init_bill_date = p_doc->Dt;
						else if(checkdate(p_doc->InvcDate))
							init_bill_date = p_doc->InvcDate;
						else
							init_bill_date = getcurdate_();
						if(p_doc->Code.NotEmpty())
							init_bill_code = p_doc->Code;
						else if(p_doc->InvcCode.NotEmpty())
							init_bill_code = p_doc->InvcCode;
						DocNalogRu_Reader::Participant * p_seller = p_doc->GetParticipant(EDIPARTYQ_SELLER, false);
						SETIFZ(p_seller, p_doc->GetParticipant(EDIPARTYQ_CONSIGNOR, false));
						DocNalogRu_Reader::Participant * p_buyer = p_doc->GetParticipant(EDIPARTYQ_BUYER, false);
						SETIFZ(p_buyer, p_doc->GetParticipant(EDIPARTYQ_CONSIGNEE, false));
						{
							// @v11.2.7 {
							struct ResolveBlock {
								static PPID Resolve(PPBillImporter * pMaster, DocNalogRu_Reader::Participant * pParticipant, const SString & rInitBillCode, PPID acsID)
								{
									PPID   ar_id = 0;
									if(pParticipant) {
										if(pParticipant->GLN.NotEmpty())
											pMaster->ResolveGLN(pParticipant->GLN, rInitBillCode, acsID, &ar_id, 0);
										if(!ar_id)
											pMaster->ResolveINN(pParticipant->INN, 0, 0, rInitBillCode, acsID, &ar_id, 0);
									}
									return ar_id;
								}
							};
							seller_ar_id = ResolveBlock::Resolve(this, p_seller, init_bill_code, contragent_acs_id);
							buyer_ar_id  = ResolveBlock::Resolve(this, p_buyer, init_bill_code, contragent_acs_id);
							// } @v11.2.7 
							/* @v11.2.7 if(p_seller) {
								PPID   ar_id = 0;
								if(p_seller->GLN.NotEmpty())
									ResolveGLN(p_seller->GLN, init_bill_code, contragent_acs_id, &ar_id, 0);
								if(!ar_id)
									ResolveINN(p_seller->INN, 0, 0, init_bill_code, contragent_acs_id, &ar_id, 0);
								if(ar_id)
									seller_ar_id = ar_id;
							}
							if(p_buyer) {
								PPID   ar_id = 0;
								if(p_buyer->GLN.NotEmpty())
									ResolveGLN(p_buyer->GLN, init_bill_code, contragent_acs_id, &ar_id, 0);
								if(!ar_id)
									ResolveINN(p_buyer->INN, 0, 0, init_bill_code, contragent_acs_id, &ar_id, 0);
								if(ar_id)
									buyer_ar_id = ar_id;
							}*/
						}
						pack.CreateBlank2(BillParam.ImpOpID, init_bill_date, LocID, 0);
						if(seller_ar_id) {
							PPBillPacket::SetupObjectBlock sob;
							if(!pack.SetupObject(seller_ar_id, sob)) {
								Logger.LogLastError();
								skip = 1;
							}
						}
						else {
							//PPERR_CANTIDENTIMPBILLAR
							msg_buf.Z().Cat(init_bill_code).Space().Cat(init_bill_date, DATF_DMY|DATF_CENTURY);
							if(p_seller) {
								if(p_seller->GLN.NotEmpty())
									msg_buf.Space().CatEq("GLN", p_seller->GLN);
								if(p_seller->INN.NotEmpty())
									msg_buf.Space().CatEq("INN", p_seller->INN);
							}
							PPSetError(PPERR_CANTIDENTIMPBILLAR, msg_buf);
							Logger.LogLastError(); // @v10.8.2
							skip = 1;
						}
						if(!skip) {
							STRNSCPY(pack.Rec.Code, init_bill_code);
							if(p_doc->InvcCode.NotEmpty()) {
								STRNSCPY(pack.Ext.InvoiceCode, p_doc->InvcCode);
							}
							if(checkdate(p_doc->InvcDate)) {
								pack.Ext.InvoiceDate = p_doc->InvcDate;
							}
							BillTbl::Rec ex_bill_rec;
							PPID   ex_bill_id = 0;
							if(P_BObj->P_Tbl->SearchAnalog(&pack.Rec, BillCore::safDefault, &ex_bill_id, &ex_bill_rec) > 0) {
								PPObjBill::MakeCodeString(&ex_bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, msg_buf);
								Logger.LogMsgCode(mfError, PPERR_DOC_ALREADY_EXISTS, msg_buf);
								skip = 1;
							}
						}
						if(!skip) {
							for(uint rowidx = 0; rowidx < p_doc->GoodsItemList.getCount(); rowidx++) {
								DocNalogRu_Reader::GoodsItem * p_item = p_doc->GoodsItemList.at(rowidx);
								PPID   goods_id = 0;
								SString barcode;
								Goods2Tbl::Rec goods_rec;
								BarcodeTbl::Rec bc_rec;
								if(p_item->GTIN) {
									if(GObj.SearchByBarcode(p_item->GTIN, &bc_rec, &goods_rec, 1) > 0)
										goods_id = goods_rec.ID;
									barcode = p_item->GTIN;
								}
								if(!goods_id) {
									GtinStruc gts;
									for(uint markssp = 0; p_item->MarkList.get(&markssp, temp_buf);) {
										const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
										if(pczcr > 0 && gts.GetToken(GtinStruc::fldGTIN14, &temp_buf)) {
											if(GObj.SearchByBarcode(temp_buf, &bc_rec, &goods_rec, 1) > 0) {
												goods_id = goods_rec.ID;
											}
											if(barcode.IsEmpty()) {
												if(temp_buf.Len() == 14)
													temp_buf.ShiftLeftChr('0');
												barcode = temp_buf;
											}
											break; // Все марки одного товара имеют один и тот же GTIN. Стало быть есть смысл исследовать на GTIN только первую марку
										}
									}
								}
								if(!goods_id) {
									// @v10.8.8 {
									if(p_item->GoodsName.NotEmpty() && GObj.SearchByName(p_item->GoodsName, &goods_id, &goods_rec) > 0) {
										assert(goods_id == goods_rec.ID);
										if(barcode.NotEmpty()) {
											if(!GObj.P_Tbl->AddBarcode(goods_rec.ID, barcode, 1.0, 1))
												Logger.LogLastError();
										}
									}
									else {
										assert(goods_id == 0); // } @v10.8.8
										ResolveGoodsItem rgi;
										STRNSCPY(rgi.Barcode, barcode);
										STRNSCPY(rgi.GoodsName, p_item->GoodsName);
										rgi.VatRate = p_item->VatRate;
										if(CreateAbsenceGoods(rgi, 1) > 0) {
											assert(rgi.ResolvedGoodsID > 0);
											if(GObj.Search(rgi.ResolvedGoodsID, &goods_rec) > 0)
												goods_id = goods_rec.ID;
										}
										/*if(r_gcfg.DefGroupID && r_gcfg.DefUnitID) {
											PPGoodsPacket new_goods_pack;
											GObj.InitPacket(&new_goods_pack, gpkndGoods, r_gcfg.DefGroupID, 0, barcode);
											if(p_item->GoodsName.NotEmpty())
												STRNSCPY(new_goods_pack.Rec.Name, p_item->GoodsName);
											//if(barcode.NotEmpty())
												//new_goods_pack.Codes.Add(barcode, 0, 1.0);
											//new_goods_pack.Rec.ParentID = r_gcfg.DefGroupID;
											new_goods_pack.Rec.UnitID = r_gcfg.DefUnitID;
											if(p_item->VatRate > 0.0 && p_item->VatRate <= 40.0) {
												PPID   tax_grp_id = 0;
												if(GObj.GTxObj.GetByScheme(&tax_grp_id, p_item->VatRate, 0.0, 0.0, 0, 1) > 0)
													new_goods_pack.Rec.TaxGrpID = tax_grp_id;
											}
											if(GObj.PutPacket(&goods_id, &new_goods_pack, 1)) {
												goods_rec = new_goods_pack.Rec;
											}
											else {
												Logger.LogLastError();
											}
										}
										else {
											// Для автоматического создания товаров в конфигурации товаров должны быть указаны группа и единица измерения по умолчанию
											PPLoadText(PPTXT_TOCREATEWARECFGOPTNEEDED, msg_buf);
											Logger.Log(msg_buf);
										}*/
									}
								}
								if(goods_id) {
									PPTransferItem ti;
									ti.Init(&pack.Rec);
									ti.RByBill = p_item->RowN;
									ti.SetupGoods(goods_id);
									ti.Quantity_ = p_item->Qtty;
									if(p_item->Price > 0.0)
										ti.Cost = p_item->Price;
									// @v10.8.2 {
									else if(p_item->PriceSum > 0.0) {
										ti.Cost = R5(fabs(p_item->PriceSum / p_item->Qtty));
									}
									else if(p_item->PriceSumWoVat > 0.0) {
										GTaxVect vect;
										PPGoodsTaxEntry gtx;
										if(GObj.GTxObj.FetchByID(goods_rec.TaxGrpID, &gtx) > 0) {
											vect.Calc_(&gtx, p_item->PriceSumWoVat, fabs(p_item->Qtty), GTAXVF_AFTERTAXES, 0);
											ti.Cost = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE | GTAXVF_VAT) / fabs(p_item->Qtty);
										}
									}
									// } @v10.8.2
									else if(p_item->PriceWoVat > 0.0) {
										GTaxVect vect;
										PPGoodsTaxEntry gtx;
										if(GObj.GTxObj.FetchByID(goods_rec.TaxGrpID, &gtx) > 0) {
											vect.Calc_(&gtx, p_item->PriceWoVat, 1.0, GTAXVF_AFTERTAXES, 0); // @v10.8.1 @fix (p_item->PriceSumWoVat, p_item->Qtty)-->(p_item->PriceWoVat, 1.0)
											ti.Cost = vect.GetValue(GTAXVF_AFTERTAXES | GTAXVF_EXCISE | GTAXVF_VAT);
										}
									}
									{
										// @v10.9.7 @todo Автоматом установить цену реализации (например, по расценке)
									}
									LongArray pos_list;
									pack.InsertRow(&ti, &pos_list);
									if(pos_list.getCount()) {
										const uint new_pos = pos_list.get(0);
										for(uint markssp = 0; p_item->MarkList.get(&markssp, temp_buf);) {
											if(temp_buf.NotEmpty())
												pack.XcL.Add(new_pos+1, 0, 0, temp_buf, 0);
										}
									}
								}
								else {
									skip = 1;
								}
							}
							if(P_BObj->__TurnPacket(&pack, 0, 1, 1))
								Logger.LogAcceptMsg(PPOBJ_BILL, pack.Rec.ID, 0);
							else
								Logger.LogLastError();
						}
					}
				}
			}
		}
	}
	else {
		if(Flags & PPBillImporter::fEdiImpExp) {
			//
			// Если импортируем через EDI, то необходимо импортировать данные для всех провайдеров, указанных
			// в impexp.ini для выбранного типа документов
			//
			PPIniFile ini_file(PPGetFilePathS(PPPATH_BIN, PPFILNAM_IMPEXP_INI, file_name), 0, 1, 1);
			StringSet sects;
			const PPBillImpExpParam preserve_bill_param = BillParam; // Запомним начальные настройки конфигурации
			if(ini_file.GetSections(&sects) > 0) {
				uint sect_pos = 0;
				SString type_str;
				// Импортируем документы, пока получаем настройки для импорта данного типа документа (BillParam.Name) (перечисление в PPTXT_EDIIMPCMD)
				while(sects.get(&sect_pos, temp_buf)) {
					size_t start_pos = 0;
					uint   end_pos = 0;
					(type_str = BillParam.Name).Transf(CTRANSF_INNER_TO_OUTER);
					if(temp_buf.Search("IMP@BILL@DLL_", 0, 1, &(start_pos = 0)) > 0 && temp_buf.Search(BillParam.Name, 0, 1, &(start_pos = 0)) > 0) {
						if(!BillParam.ReadIni(&ini_file, temp_buf, 0)) {
							PPSetError(PPERR_IMPEXPCFGRDFAULT, temp_buf);
							Logger.LogLastError();
						}
						else {
							int    r = ReadData();
							if(r > 0)
								Import(1);
							else if(!r) {
								Logger.LogLastError();
							}
							BillParam = preserve_bill_param;
						}
					}
				}
			}
		}
		THROW(ReadData());
		THROW(Import(1));
		for(uint i = 0; ToRemoveFiles.get(&i, file_name);) {
			if(fileExists(file_name))
				SFile::Remove(file_name);
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int ImportBills(PPBillImpExpParam * pBillParam, PPBillImpExpParam * pBRowParam, PPID opID, PPID locID)
{
	int    ok = -1, r = 1;
	PPBillImporter b_i;
	THROW(r = b_i.Init(pBillParam, pBRowParam, opID, locID));
	if(r > 0) {
		THROW(b_i.Run());
		ok = 1;
	}
	CATCHZOKPPERR
	return ok;
}
//
// Export
//
class BillExpExprEvalContext : public BillContext {
public:
	BillExpExprEvalContext(const PPBillPacket * pPack, const SdRecord & rRec) : BillContext(pPack, pPack->Rec.CurID, 0), R_Rec(rRec)
	{
	}
	virtual int Resolve(const char * pSymb, double * pVal)
	{
		if(PPImpExp::ResolveVarName(pSymb, R_Rec, pVal) > 0)
			return 1;
		else {
			TempBuf = pSymb;
			TempBuf.ReplaceChar('@', ' ').Strip();
			return BillContext::Resolve(TempBuf, pVal);
		}
	}
private:
	const SdRecord & R_Rec;
	SString TempBuf; // @allocreuse
};

// @vmiller
class SignBillDialog : public TDialog {
public:
	SignBillDialog(const char * pFileName) : TDialog(DLG_SIGNBILL), CurPosInList(0), FileName(pFileName)
	{
		SetupStrListBox(this, CTL_SIGNBILL_SIGNER);
		SetupStrListBox(this, CTL_SIGNBILL_SIGNFILE);
		DrawList();
	}
private:
	DECL_HANDLE_EVENT
	{
		TDialog::handleEvent(event);
		if(event.isCtlEvent(CTL_SIGNBILL_SIGNER) && event.isCmd(cmLBItemFocused)) {
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SIGNBILL_SIGNER));
			if(p_list) {
				p_list->getCurID(&CurPosInList);
				p_list->getCurString(SignerName.Z());
			}
		}
		else if(event.isCtlEvent(CTL_SIGNBILL_SIGNFILE) && event.isCmd(cmLBItemFocused)) {
			SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SIGNBILL_SIGNFILE));
			if(p_list) {
				p_list->getCurString(SignFileName.Z());
				SPathStruc spath(FileName);
				SPathStruc sign_spath(SignFileName);
				spath.Nam = sign_spath.Nam;
				spath.Ext = sign_spath.Ext;
				spath.Merge(SignFileName);
			}
		}
		else if(event.isCmd(cmSignBill)) {
			SString msg_buf, fmt_buf;
			if(SignerName.NotEmpty() && FileName.NotEmpty()) {
				if(Eds.SignData(SignerName.Transf(CTRANSF_INNER_TO_OUTER), FileName, SignFileName)) {
					PPLoadText(PPTXT_EDS_FILESIGNED, fmt_buf);
					msg_buf.Printf(fmt_buf, FileName.cptr(), SignFileName.cptr());
					PPOutputMessage(msg_buf, mfYes | mfLargeBox);
				}
				else
					PPError();
			}
			else {
				PPLoadText(PPTXT_EDS_NOFILEORSIGNOWNER, msg_buf);
				PPOutputMessage(msg_buf, mfYes | mfLargeBox);
			}

		}
		clearEvent(event);
	}
	int    DrawList();

	long   CurPosInList;
	SString FileName;
	SString SignFileName;
	SString SignerName;
	PPEds Eds;
};

// @vmiller
int SignBillDialog::DrawList()
{
	int    ok = 1;
	StrAssocArray names_arr, files_arr;
	SString signer_name, file_name;
	{
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SIGNBILL_SIGNER));
		if(p_list) {
			THROW(Eds.GetSignerNamesInStore(names_arr));
			if(p_list) {
				for(uint i = 0; i < names_arr.getCount(); i++) {
					names_arr.GetText(i, signer_name);
					THROW_SL(p_list->addItem(i, signer_name.Transf(CTRANSF_OUTER_TO_INNER)));
				}
			}
			p_list->Draw_();
		}
	}
	{
		SmartListBox * p_list = static_cast<SmartListBox *>(getCtrlView(CTL_SIGNBILL_SIGNFILE));
		if(p_list) {
			THROW(Eds.GetSignFilesForDoc(FileName, files_arr));
			if(p_list) {
				for(uint i = 0; i < files_arr.getCount(); i++) {
					files_arr.GetText(i, file_name);
					THROW_SL(p_list->addItem(i, file_name.Transf(CTRANSF_OUTER_TO_INNER)));
				}
			}
			p_list->Draw_();
		}
	}
	CATCHZOKPPERR;
	return ok;
}

PPBillExporter::PPBillExporter() :  P_IEBill(0), P_IEBRow(0)
{
}

PPBillExporter::~PPBillExporter()
{
	Destroy();
}

void PPBillExporter::Destroy()
{
	const int del_row = BIN(P_IEBill != P_IEBRow);
	ZDELETE(P_IEBill);
	if(del_row)
		ZDELETE(P_IEBRow);
}

int PPBillExporter::Init(const PPBillImpExpParam * pBillParam, const PPBillImpExpParam * pBRowParam, const PPBillPacket * pFirstPack, StringSet * pResultFileList)
{
	Destroy();
	int    ok = 1;
	SString temp_buf;
	PPBillImpExpBaseProcessBlock::Z();
	RVALUEPTR(BillParam, pBillParam);
	RVALUEPTR(BRowParam, pBRowParam);
	if(!pBillParam || !pBRowParam) {
		if(!pBillParam)
			THROW(LoadSdRecord(PPREC_BILL, &BillParam.InrRec));
		if(!pBRowParam)
			THROW(LoadSdRecord(PPREC_BROW, &BRowParam.InrRec));
		if(pFirstPack && !Tp.AddrList.getCount()) {
			if(pFirstPack->GetContextEmailAddr(temp_buf) > 0)
				Tp.AddrList.Add(1, temp_buf);
		}
		THROW(ok = Select(0));
	}
	if(ok > 0) {
		if(Flags & fPaymOrdersExp) {
		}
		else {
			if(BillParam.PredefFormat && oneof5(BillParam.PredefFormat, piefNalogR_Invoice, piefNalogR_REZRUISP, piefNalogR_SCHFDOPPR, 
				piefNalogR_ON_NSCHFDOPPRMARK, piefNalogR_ON_NSCHFDOPPR)) { // @v11.2.1 piefNalogR_ON_NSCHFDOPPR
				BillParam.DataFormat = PPImpExpParam::dfXml;
			}
			THROW_MEM(P_IEBill = new PPImpExp(&BillParam, pFirstPack));
			THROW(!P_IEBill->IsCtrError());
			if(!(BillParam.BaseFlags & PPImpExpParam::bfDLL) && !(Flags & fEgaisImpExp)) {
				if(!BillParam.PredefFormat) {
					if(BillParam.DataFormat == PPImpExpParam::dfXml && BRowParam.DataFormat == PPImpExpParam::dfXml && BillParam.FileName.IsEqNC(BRowParam.FileName)) {
						P_IEBRow = P_IEBill;
					}
					else {
						THROW_MEM(P_IEBRow = new PPImpExp(&BRowParam, pFirstPack));
						THROW(!P_IEBRow->IsCtrError());
						THROW(P_IEBRow->OpenFileForWriting(0, 1, pResultFileList));
					}
				}
				// Если имя файла задано шаблоном, то оно могло измениться { //
				BillParam.FileName = P_IEBill->GetParamConst().FileName;
				if(P_IEBRow)
					BRowParam.FileName = P_IEBRow->GetParamConst().FileName;
				// }
				if(!BillParam.PredefFormat)
					THROW(P_IEBill->OpenFileForWriting(0, 1, pResultFileList));
			}
		}
	}
	CATCHZOK
	return ok;
}

int PPBillExporter::SignBill()
{
	int    ok = 1;
	TDialog * p_sign_dlg = 0;
	if(Flags & fSignExport) {
		THROW(CheckDialogPtr(&(p_sign_dlg = new SignBillDialog(BillParam.FileName))));
		ExecView(p_sign_dlg);
	}
	CATCHZOK
	delete p_sign_dlg;
	return ok;
}

int PPBillExporter::PutPacket(PPBillPacket * pPack, int sessId /*=0*/, ImpExpDll * pImpExpDll /*=0*/)
{
	int    ok = -1;
	int    obj_id = 0;
	int    use_dll = 0;
	const  int use_ar_code = BIN(pPack->Rec.Object && (CConfig.Flags & CCFLG_USEARGOODSCODE));
	PPObjWorld world_obj;
	SString temp_buf, err_msg;
	Sdr_Bill bill;
	PPTransferItem * p_ti = 0;
	Reference * p_ref = PPRef;
	ReceiptCore * p_rcpt = (P_BObj && P_BObj->trfr) ? &P_BObj->trfr->Rcpt : 0;
	GTaxVect vect;
	THROW_INVARG(pPack && P_IEBill && (P_IEBRow || (sessId && pImpExpDll)));
	{
		const SString edi_op_symb = BillParam.ImpExpParamDll.OperType;
		SString ttn_tag_value;
		if(BillParam.ImpExpParamDll.TTNTagID) {
			ObjTagItem tag_item;
			if(p_ref->Ot.GetTag(PPOBJ_BILL, pPack->Rec.ID, BillParam.ImpExpParamDll.TTNTagID, &tag_item) > 0)
				tag_item.GetStr(ttn_tag_value);
		}
		if(sessId && pImpExpDll) {
            int    edi_op = 0;
            if(edi_op_symb.IsEqiAscii("RECADV"))
				edi_op = PPEDIOP_RECADV;
			else if(edi_op_symb.IsEqiAscii("ORDER"))
				edi_op = PPEDIOP_ORDER;
			PPObjBill::MakeCodeString(&pPack->Rec, PPObjBill::mcsAddOpName, temp_buf);
            pPack->Rec.EdiOp = edi_op;
			THROW(BillRecToBill(pPack, &bill));
			THROW_PP_S(edi_op != PPEDIOP_RECADV || bill.DesadvBillNo[0], PPERR_EDI_SPRRECADVWOTTN, temp_buf);
			// Если отклоненный заказ
			if(pPack->Rec.Flags2 & BILLF2_DECLINED) {
				SString decline_order;
				THROW(PPLoadString(PPSTR_IMPEXPCMD, IMPEXPCMD_DECLNORDER, decline_order));
				THROW(pImpExpDll->SetExportObj(sessId, "BILLS", &bill, &obj_id, decline_order));
			}
			else {
				THROW(pImpExpDll->SetExportObj(sessId, "BILLS", &bill, &obj_id, edi_op_symb));
			}
			THROW(pImpExpDll->InitExportIter(sessId, obj_id));
		}
		else {
			if(!(BillParam.Flags & BillParam.fImpExpRowsOnly)) { // @v10.4.6
				BillExpExprEvalContext expr_ctx(pPack, P_IEBill->GetParamConst().InrRec);
				P_IEBill->SetExprContext(&expr_ctx);
				THROW(BillRecToBill(pPack, &bill));
				THROW(P_IEBill->AppendRecord(&bill, sizeof(bill)));
				P_IEBRow->Push(&BRowParam);
			}
		}
		for(uint i = 0; pPack->EnumTItems(&i, &p_ti) > 0;) {
			const PPID goods_id = labs(p_ti->GoodsID);
			Goods2Tbl::Rec goods_rec;
			Goods2Tbl::Rec gg_rec; // @v10.5.0
			Goods2Tbl::Rec brand_rec; // @v10.5.3
			Sdr_BRow brow;
			BarcodeArray bcd_ary;
			// @v10.7.9 @ctr MEMSZERO(brow);
			STRNSCPY(brow.BillID, bill.ID);
			brow.LineNo = i;
			brow.GoodsID = goods_id;
			brow.LotID = p_ti->LotID;
			if(GObj.Fetch(goods_id, &goods_rec) <= 0)
				MEMSZERO(goods_rec);
			(temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.GoodsName, sizeof(brow.GoodsName));
			// @v10.5.0 {
			if(goods_rec.ParentID && GObj.Fetch(goods_rec.ParentID, &gg_rec) > 0)
				(temp_buf = gg_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.GoodsGroup, sizeof(brow.GoodsGroup));
			// } @v10.5.0
			// @v10.5.3 {
			if(goods_rec.BrandID && GObj.Fetch(goods_rec.BrandID, &brand_rec) > 0)
				(temp_buf = brand_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.BrandName, sizeof(brow.BrandName));
			// } @v10.5.3
			if(use_ar_code) {
				if(GObj.P_Tbl->GetArCode(pPack->Rec.Object, goods_id, temp_buf, 0) > 0) {
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER); // @v10.4.5
					temp_buf.CopyTo(brow.ArCode, sizeof(brow.ArCode));
				}
				if(GObj.P_Tbl->GetArCode(0, goods_id, temp_buf, 0) > 0) {
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER); // @v10.4.5
					temp_buf.CopyTo(brow.ArCodeOwn, sizeof(brow.ArCodeOwn));
				}
			}
			if(GObj.ReadBarcodes(goods_id, bcd_ary) > 0) {
				bcd_ary.GetSingle(temp_buf);
				temp_buf.CopyTo(brow.Barcode, sizeof(brow.Barcode));
			}
			if(goods_rec.ManufID) {
				PPID   country_id = 0;
				PPCountryBlock mcb;
				PsnObj.GetCountry(goods_rec.ManufID, &country_id, &mcb);
				mcb.Name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.ManufCountryName, sizeof(brow.ManufCountryName));
				mcb.Code.CopyTo(brow.ManufCountryOKSM, sizeof(brow.ManufCountryOKSM));
			}
			{
				PPID   org_lot_id = 0;
				ReceiptTbl::Rec org_lot_rec;
				if(p_rcpt->SearchOrigin(brow.LotID, &org_lot_id, 0, &org_lot_rec) > 0) {
					//
					// Ищем дату документа из лота
					//
					BillTbl::Rec bill_rec;
					ObjTagList org_lot_tag_list;
					//
					// В дальнейшем нам понадобится список тегов лота
					//
					P_BObj->GetTagListByLot(org_lot_id, 0, &org_lot_tag_list);
					if(P_BObj->Search(org_lot_rec.BillID, &bill_rec) > 0)
						brow.LotDocDate = bill_rec.Dt;
					//
					// Идентифицируем вид товарной продукции по тегу оригинального лота #1
					//
					if(BillParam.ImpExpParamDll.GoodsKindTagID) {
						PPID   gds_kind_tag_id = BillParam.ImpExpParamDll.GoodsKindTagID;
						const  ObjTagItem * p_goods_kind_tag = org_lot_tag_list.GetItem(gds_kind_tag_id);
						if(p_goods_kind_tag) {
							long   kind_code = 0;
							p_goods_kind_tag->GetInt(&kind_code);
							brow.GoodKindCode = kind_code; // Код вида товара
						}
					}
					{
						//
						// Смотрим ИД производителя/импортера, его имя, ИНН и КПП
						//
						PersonTbl::Rec pers_rec;
						PersonReq persrec;
						RegisterTbl::Rec reg_rec;
						const  ObjTagItem * p_manuf_tag = org_lot_tag_list.GetItem(BillParam.ImpExpParamDll.ManufTagID);
						if(p_manuf_tag) {
							long   manuf_id = 0;
							int    r = 0;
							p_manuf_tag->GetInt(&manuf_id);
							if(PsnObj.Search(manuf_id, &pers_rec) > 0)
								(temp_buf = pers_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.LotManuf, sizeof(brow.LotManuf));
							if(PsnObj.GetPersonReq(manuf_id, &persrec) > 0) {
								STRNSCPY(brow.ManufINN, persrec.TPID);
								STRNSCPY(brow.ManufKPP, persrec.KPP);
							}
							// Если не нашли ИНН здесь, то, скорее всего это филиал. Тогда смотрим ИНН у головной организации
							if(isempty(persrec.TPID)) {
								PPIDArray id_list;
								if(PsnObj.GetRelPersonList(manuf_id, PPPRK_MAIN, 0, &id_list) > 0) {
									// Ищем по первой записи в отношениях
									if(PsnObj.Search(id_list.get(0), &pers_rec) > 0)
										if(PsnObj.GetPersonReq(pers_rec.ID, &persrec) > 0)
											STRNSCPY(brow.ManufINN, persrec.TPID);
								}
							}
							//
							// Смотрим, является ли персоналия производителем
							//
							if(BillParam.ImpExpParamDll.IsManufTagID) {
								ObjTagItem tag_item;
								if(p_ref->Ot.GetTag(PPOBJ_PERSON, manuf_id, BillParam.ImpExpParamDll.IsManufTagID, &tag_item) > 0) {
									long   is_manuf = 0;
									tag_item.GetInt(&is_manuf);
									brow.IsManuf = is_manuf;
								}
							}
							//
							// Смотрим адрес производителя
							//
							LocationTbl::Rec loc_rec;
							WorldTbl::Rec world_rec;
							PPLocAddrStruc loc_addr_st;
							SString address, city_name, region_name;
							// @v10.6.4 MEMSZERO(world_rec);
							// @v10.6.4 MEMSZERO(loc_rec);
							if(pers_rec.MainLoc)
								r = LocObj.Fetch(pers_rec.MainLoc, &loc_rec);
							else if(pers_rec.RLoc)
								r = LocObj.Fetch(pers_rec.RLoc, &loc_rec);
							if(r && loc_rec.CityID && world_obj.Fetch(loc_rec.CityID, &world_rec) > 0) {
								city_name = world_rec.Name;
								while(world_rec.ParentID > 0) {
									if(world_obj.Search(world_rec.ParentID, &world_rec) > 0) {
										if(world_rec.ZIP[0]) {
											region_name = world_rec.Name;
											world_rec.ParentID = 0;
										}
									}
									else
										world_rec.ParentID = 0;
								}
							}
							PsnObj.GetAddress(manuf_id, address);
							loc_addr_st.Recognize(address.Transf(CTRANSF_INNER_TO_OUTER));
							if(world_rec.ZIP[0] == 0)
								if(loc_addr_st.GetText(PPLocAddrStruc::tZip, temp_buf))
									temp_buf.CopyTo(world_rec.ZIP, sizeof(world_rec.ZIP));
							STRNSCPY(brow.ManufIndex, world_rec.ZIP);
							brow.ManufRegionCode = BillParam.ImpExpParamDll.ManufRegionCode; // Вытаскиваем из ini-файла
							if(region_name.IsEmpty())
								if(loc_addr_st.GetText(PPLocAddrStruc::tLocalArea, temp_buf))
									temp_buf.CopyTo(brow.ManufDistrict, sizeof(brow.ManufDistrict));
							if(city_name.IsEmpty()) {
								if(loc_addr_st.GetText(PPLocAddrStruc::tCity, temp_buf))
									temp_buf.CopyTo(brow.ManufCityName, sizeof(brow.ManufCityName));
							}
							else
								city_name.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.ManufCityName, sizeof(brow.ManufCityName));
							if(loc_addr_st.GetText(PPLocAddrStruc::tStreet, temp_buf))
								temp_buf.CopyTo(brow.ManufStreet, sizeof(brow.ManufStreet));
							if(loc_addr_st.GetText(PPLocAddrStruc::tHouse, temp_buf))
								brow.ManufHouse = temp_buf.ToLong();
							if(loc_addr_st.GetText(PPLocAddrStruc::tCorp, temp_buf))
								brow.ManufHousing = temp_buf.ToLong();
							//
							// Смотрим инфу о лицензии
							//
							if(PsnObj.GetRegister(manuf_id, BillParam.ImpExpParamDll.AlcoLicenseRegID, &reg_rec) > 0) {
								brow.LicenseID = reg_rec.ID;
								STRNSCPY(brow.LicenseNum, reg_rec.Num);
								brow.LicenseDate = reg_rec.Dt;
								brow.LicenseExpiry = reg_rec.Expiry;
								(temp_buf = reg_rec.Serial).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.LicenseSerial, sizeof(brow.LicenseSerial));
								STRNSCPY(brow.LicenseNum, reg_rec.Num);
								if(PsnObj.Search(reg_rec.RegOrgID, &pers_rec) > 0)
									(temp_buf = pers_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.RegAuthority, sizeof(brow.RegAuthority));
							}
						}
					}
				}
				//
				// Если в точке #1 не удалось идентифицировать вид товарной продукции, то определяем ее из классификатора товара
				//
				if(!brow.GoodKindCode) {
					const SString goods_kind_symb = BillParam.ImpExpParamDll.GoodsKindSymb;
					GoodsExtTbl::Rec goods_ext_rec;
					if(goods_kind_symb.NotEmpty() && GObj.P_Tbl->GetExt(goods_rec.ID, &goods_ext_rec)) {
						if(goods_kind_symb.IsEqiAscii("X"))
							brow.GoodKindCode = goods_ext_rec.X;
						else if(goods_kind_symb.IsEqiAscii("Y"))
							brow.GoodKindCode = goods_ext_rec.Y;
						else if(goods_kind_symb.IsEqiAscii("Z"))
							brow.GoodKindCode = goods_ext_rec.Z;
						else if(goods_kind_symb.IsEqiAscii("W"))
							brow.GoodKindCode = goods_ext_rec.W;
					}
				}
				//
				// Проверяем, принадлежит ли товар альтернативной группе "Пиво"
				//
				if(BillParam.ImpExpParamDll.BeerGrpID) {
					if(GObj.BelongToGroup(goods_rec.ID, BillParam.ImpExpParamDll.BeerGrpID))
						brow.IsBeer = 1;
				}
				//
				// Проверяем, принадлежит ли товар альтернативной группе "Алкоголь"
				//
				if(BillParam.ImpExpParamDll.AlcoGrpID) {
					if(GObj.BelongToGroup(goods_rec.ID, BillParam.ImpExpParamDll.AlcoGrpID))
						brow.IsAlco = 1;
					brow.GoodGrpID = goods_rec.ParentID; // Тип товара
				}
				//
				// Запишем номер ТТН
				//
				if(ttn_tag_value.NotEmpty())
					STRNSCPY(brow.TTN, ttn_tag_value);
				else if(!isempty(bill.Memo))
					STRNSCPY(brow.TTN, bill.Memo);
				//
				// Запишем ГТД
				//
				if(P_BObj->GetClbNumberByLot(org_lot_id, 0, temp_buf) > 0)
					temp_buf.CopyTo(brow.GTD, sizeof(brow.GTD));
			}
			if(p_ti->Flags & PPTFR_REVAL) {
				brow.Quantity = p_ti->Rest_;
				brow.Cost     = p_ti->Cost;
				brow.Price    = p_ti->Price;
				brow.Discount = 0.0;
			}
			else {
				double upp = 0.0;
				brow.Quantity = p_ti->Quantity_;
				{
					upp = p_ti->UnitPerPack;
					if(upp <= 0.0) {
						GoodsStockExt gse;
						if(GObj.GetStockExt(p_ti->GoodsID, &gse, 1) > 0)
							upp = gse.Package;
					}
					if(upp <= 0.0) {
						ReceiptTbl::Rec lot_rec;
						if(p_rcpt && p_rcpt->GetLastLot(p_ti->GoodsID, pPack->Rec.LocID, pPack->Rec.Dt, &lot_rec) > 0)
							upp = lot_rec.UnitPerPack;
					}
					if(upp > 0.0) {
						brow.UnitPerPack = upp;
						brow.PckgQtty = (double)(long)(brow.Quantity / upp);
					}
				}
				brow.Cost     = p_ti->Cost;
				brow.Price    = p_ti->Price;
				brow.Discount = p_ti->Discount;
				if(p_ti->Flags & PPTFR_INDEPPHQTTY)
					brow.PhQtty = p_ti->WtQtty;
				else {
					double phuperu = 0.0;
					//
					// Поиск объема единицы в полях расширения
					//
					if(BillParam.ImpExpParamDll.GoodsVolSymb.NotEmpty()) {
						SString goods_vol_symb = BillParam.ImpExpParamDll.GoodsVolSymb;
						if(goods_vol_symb.NotEmpty()) {
							PPGdsClsPacket gds_cls_pack;
							GoodsExtTbl::Rec goods_ext_rec;
							GObj.FetchCls(goods_rec.ID, 0, &gds_cls_pack);
							if(GObj.P_Tbl->GetExt(goods_rec.ID, &goods_ext_rec)) {
								int    dim = 0;
								if(goods_vol_symb.IsEqiAscii("X"))
									dim = PPGdsCls::eX;
								else if(goods_vol_symb.IsEqiAscii("Y"))
									dim = PPGdsCls::eY;
								else if(goods_vol_symb.IsEqiAscii("Z"))
									dim = PPGdsCls::eZ;
								else if(goods_vol_symb.IsEqiAscii("W"))
									dim = PPGdsCls::eW;
								if(dim)
									gds_cls_pack.GetExtDim(&goods_ext_rec, dim, &phuperu);
							}
						}
					}
					if(phuperu <= 0.0)
						GObj.GetPhUPerU(p_ti->GoodsID, 0, &phuperu);
					brow.PhQtty = p_ti->Quantity_ * phuperu;
				}
			}
			{
				PPUnit u_rec;
				if(GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
					(temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.UnitName, sizeof(brow.UnitName));
					(temp_buf = u_rec.Code).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.UnitCode, sizeof(brow.UnitCode));
				}
				if(GObj.FetchUnit(goods_rec.PhUnitID, &u_rec) > 0)
					(temp_buf = u_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.PhUnitName, sizeof(brow.PhUnitName));
			}
			{
				vect.CalcTI(*p_ti, pPack->Rec.OpID, TIAMT_PRICE, 0);
				brow.VatRate = vect.GetTaxRate(GTAX_VAT, 0);
				brow.VatSum  = vect.GetValue(GTAXVF_VAT);
				vect.CalcTI(*p_ti, pPack->Rec.OpID, TIAMT_COST, 0);
				brow.CVatRate = vect.GetTaxRate(GTAX_VAT, 0);
				brow.CVatSum  = vect.GetValue(GTAXVF_VAT);
			}
			brow.Expiry = p_ti->Expiry;
			pPack->LTagL.GetNumber(PPTAG_LOT_SN, i-1, temp_buf);
			STRNSCPY(brow.Serial, temp_buf);
			pPack->LTagL.GetNumber(PPTAG_LOT_CLB, i-1, temp_buf);
			STRNSCPY(brow.CLB, temp_buf);
			if(p_ti->QCert) {
				QualityCertTbl::Rec qc_rec;
				if(QcObj.Search(p_ti->QCert, &qc_rec) > 0) {
					STRNSCPY(brow.QcCode, (temp_buf = qc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcBc, (temp_buf = qc_rec.BlankCode).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcPrDate, (temp_buf = qc_rec.SPrDate).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcManuf, (temp_buf = qc_rec.Manuf).Transf(CTRANSF_INNER_TO_OUTER));
					STRNSCPY(brow.QcEtc, (temp_buf = qc_rec.Etc).Transf(CTRANSF_INNER_TO_OUTER));
					brow.QcInitDate = qc_rec.InitDate;
					brow.QcExpiry = qc_rec.Expiry;
					if(GetPersonName(qc_rec.RegOrgan, temp_buf) > 0)
						temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(brow.QcRegOrg, sizeof(brow.QcRegOrg));
				}
			}
			{
				//
				// Переносим значения полей шапки документа в строку
				//
				STRNSCPY(brow.BillCode, bill.Code);
				brow.BillDate = bill.Date;
				brow.OpID = bill.OpID;
				STRNSCPY(brow.OpSymb, bill.OpSymb);
				STRNSCPY(brow.InvcCode, bill.InvoiceCode);
				brow.InvcDate = bill.InvoiceDate;
				STRNSCPY(brow.INN, bill.INN);
				brow.CntragID = bill.CntragID;
				STRNSCPY(brow.CntragName, bill.CntragName);
				brow.CntragNo = bill.CntragNo;
				STRNSCPY(brow.RegistryCode, bill.RegistryCode);
				STRNSCPY(brow.Obj2Name, bill.Obj2Name);
				STRNSCPY(brow.Obj2INN, bill.Obj2INN);
				brow.Obj2ID = bill.Obj2ID;
				brow.Obj2No = bill.Obj2No;
				brow.DueDate = bill.DueDate;
				brow.PaymDate = bill.PaymDate;
				brow.BillAmt = bill.Amount;
				brow.CRate = bill.CRate;
				brow.CurID = bill.CurID;
				brow.BaseAmount = bill.BaseAmount;
				brow.CostWithSTax = bill.CostWithSTax;
				brow.BillVatSum = bill.VatSum;
				STRNSCPY(brow.BillMemo, bill.Memo);
				STRNSCPY(brow.LocID, bill.LocID);
				STRNSCPY(brow.LocName, bill.LocName);
				STRNSCPY(brow.LocCode, bill.LocCode);
				STRNSCPY(brow.City, bill.City);
				STRNSCPY(brow.Addr, bill.Addr);
				STRNSCPY(brow.OrderBillID, bill.OrderBillID);
				STRNSCPY(brow.OrderBillNo, bill.OrderBillNo);
				brow.DlvrAddrID = bill.DlvrAddrID;
				STRNSCPY(brow.DlvrAddrCode, bill.DlvrAddrCode);
				STRNSCPY(brow.DlvrAddr, bill.DlvrAddr);
				STRNSCPY(brow.AgentName, bill.AgentName);
				STRNSCPY(brow.AgentINN, bill.AgentINN);
				brow.AgentPersonID = bill.AgentPersonID;
			}
			{
				if(sessId && pImpExpDll) {
					THROW(pImpExpDll->NextExportIter(sessId, obj_id, &brow));
				}
				else {
					pPack->SetTPointer(i-1);
					GoodsContext ctx(p_ti, pPack);
					P_IEBRow->SetExprContext(&ctx);
					THROW(P_IEBRow->AppendRecord(&brow, sizeof(brow)));
					P_IEBRow->SetExprContext(0);
				}
			}
		}
		if(!(sessId && pImpExpDll))
			P_IEBRow->Pop();
	}
	CATCHZOK
	CALLPTRMEMB(P_IEBill, SetExprContext(0)); // Необходимо обнулить контекст выражений, так как мы передали объект со стека.
	return ok;
}

int PPBillExporter::BillRecToBill(const PPBillPacket * pPack, Sdr_Bill * pBill)
{
	int    ok = 0;
	SString temp_buf, str_city;
	if(pBill && pPack) {
		ArticleTbl::Rec ar_rec;
		LocationTbl::Rec loc_rec;
		PPOprKind op_rec;
		memzero(pBill, sizeof(*pBill));
		ltoa(pPack->Rec.ID, pBill->ID, 10);
		GetReg(pPack->Rec.Object, PPREGT_TPID, temp_buf);
		STRNSCPY(pBill->INN, temp_buf);
		GetReg(pPack->Rec.Object, PPREGT_GLN, temp_buf);
		STRNSCPY(pBill->GLN, temp_buf);
		pBill->Date = pPack->Rec.Dt;
		pBill->DueDate = pPack->Rec.DueDate;
		pPack->GetLastPayDate(&pBill->PaymDate);
		(temp_buf = pPack->Rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
		STRNSCPY(pBill->Code, temp_buf);
		temp_buf = pPack->Ext.InvoiceCode[0] ? pPack->Ext.InvoiceCode : pPack->Rec.Code;
		STRNSCPY(pBill->InvoiceCode, temp_buf.Transf(CTRANSF_INNER_TO_OUTER));
		pBill->InvoiceDate = NZOR(pPack->Ext.InvoiceDate, pPack->Rec.Dt);
		if(GetOpData(pPack->Rec.OpID, &op_rec) > 0) {
			pBill->OpID = op_rec.ID;
			STRNSCPY(pBill->OpSymb, op_rec.Symb);
		}
		pBill->CurID        = pPack->Rec.CurID;
		pBill->CRate        = pPack->Rec.CRate;
		pBill->Amount       = pPack->Rec.Amount;
		pBill->CostWithSTax = BIN(pPack->Rec.Flags & BILLF_RMVEXCISE);
		pBill->VatSum = pPack->Amounts.Get(PPAMT_VATAX, 0L);
		if(pPack->Rec.Object) {
			pBill->CntragID = pPack->Rec.Object;
			if(ArObj.Fetch(pPack->Rec.Object, &ar_rec) > 0) {
				pBill->CntragNo = ar_rec.Article;
				(temp_buf = ar_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->CntragName, sizeof(pBill->CntragName));
				{
					PPID   psn_id = ObjectToPerson(pPack->Rec.Object, 0);
					PPPersonPacket psn_pack;
					if(PsnObj.GetPacket(psn_id, &psn_pack, 0) > 0) {
						if(psn_pack.GetSrchRegNumber(0, temp_buf) > 0)
							temp_buf.CopyTo(pBill->RegistryCode, sizeof(pBill->RegistryCode));
                        psn_pack.ELA.GetSinglePhone(temp_buf, 0);
                        STRNSCPY(pBill->CntragPhone, temp_buf);
                        {
                        	StringSet ss_email;
							psn_pack.ELA.GetListByType(PPELK_EMAIL, ss_email);
                            if(ss_email.getCount() && ss_email.get(0U, temp_buf))
								STRNSCPY(pBill->CntragEMail, temp_buf);
						}
					}
				}
			}
		}
		if(pPack->Rec.Object2) {
			if(ArObj.Fetch(pPack->Rec.Object2, &ar_rec) > 0) {
				pBill->Obj2ID = ar_rec.ID;
				pBill->Obj2No = ar_rec.Article;
				STRNSCPY(pBill->Obj2Name, (temp_buf = ar_rec.Name).Transf(CTRANSF_INNER_TO_OUTER));
			}
			GetReg(pPack->Rec.Object2, PPREGT_TPID, temp_buf);
			temp_buf.CopyTo(pBill->Obj2INN, sizeof(pBill->Obj2INN));
			GetReg(pPack->Rec.Object2, PPREGT_GLN, temp_buf);
			temp_buf.CopyTo(pBill->Obj2GLN, sizeof(pBill->Obj2GLN));
		}
		// @v11.1.12 (temp_buf = pPack->Rec.Memo).Transf(CTRANSF_INNER_TO_OUTER);
		(temp_buf = pPack->SMemo).Transf(CTRANSF_INNER_TO_OUTER); // @v11.1.12
		STRNSCPY(pBill->Memo, temp_buf);
		if(LocObj.Search(pPack->Rec.LocID, &loc_rec) > 0) {
			ltoa(loc_rec.ID, pBill->LocID, 10);
			(temp_buf = loc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->LocName, sizeof(pBill->LocName));
			(temp_buf = loc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->LocCode, sizeof(pBill->LocCode));
			LocObj.GetCity(loc_rec.ID, 0, &str_city, 1);
			str_city.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->City, sizeof(pBill->City));
			LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
			if(temp_buf.NotEmptyS())
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->Addr, sizeof(pBill->Addr));
			else {
				LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
				temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->Addr, sizeof(pBill->Addr));
			}
		}
		{
			PPIDArray ord_bill_list;
			BillTbl::Rec ord_bill_rec;
			const PPID org_link_bill_id = pPack->Rec.LinkBillID;
			if(pPack->Rec.EdiOp == PPEDIOP_RECADV) {
				BillTbl::Rec desadv_bill_rec;
				PPID   desadv_bill_id = 0;
				for(PPID link_bill_id = org_link_bill_id; !desadv_bill_id && P_BObj->Search(link_bill_id, &desadv_bill_rec) > 0; link_bill_id = desadv_bill_rec.LinkBillID) {
					if(desadv_bill_rec.EdiOp == PPEDIOP_DESADV) {
						desadv_bill_id = desadv_bill_rec.ID;
						(temp_buf = desadv_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
						STRNSCPY(pBill->DesadvBillNo, temp_buf);
						STRNSCPY(pBill->DesadvBillID, temp_buf.Z().Cat(desadv_bill_rec.ID));
						pBill->DesadvBillDt = desadv_bill_rec.Dt;
						if(desadv_bill_rec.LinkBillID && P_BObj->Search(desadv_bill_rec.LinkBillID, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, temp_buf.Z().Cat(ord_bill_rec.ID));
							pBill->OrderDate = ord_bill_rec.Dt;
						}
					}
				}
			}
			else {
				if(P_BObj->P_Tbl->GetListOfOrdersByLading(pPack->Rec.ID, &ord_bill_list) > 0) {
					for(uint j = 0; j < ord_bill_list.getCount(); j++) {
						PPID  ord_bill_id = ord_bill_list.get(j);
						ltoa(ord_bill_id, pBill->OrderBillID, 10);
						if(P_BObj->Search(ord_bill_id, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, temp_buf.Z().Cat(ord_bill_rec.ID));
							pBill->OrderDate = ord_bill_rec.Dt;
							break; // Даже если с отгрузкой связано более одного заказа, здесь мы можем сохранить ссылку лишь на один
						}
					}
				}
				// @vmiller {
				if(isempty(pBill->OrderBillNo)) {
					if(pPack->Rec.LinkBillID) {
						BillTbl::Rec ord_bill_rec;
						ltoa(pPack->Rec.LinkBillID, pBill->OrderBillID, 10);
						if(P_BObj->Search(pPack->Rec.LinkBillID, &ord_bill_rec) > 0) {
							(temp_buf = ord_bill_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
							STRNSCPY(pBill->OrderBillNo, temp_buf);
							STRNSCPY(pBill->OrderBillID, temp_buf.Z().Cat(ord_bill_rec.ID));
							pBill->OrderDate = ord_bill_rec.Dt;
						}
					}
				}
				// } @vmiller
			}
		}
		{
			PPID   dlvr_addr_id = 0;
			if(pPack->GetDlvrAddrID())
				dlvr_addr_id = pPack->GetDlvrAddrID();
			else if(oneof2(pPack->Rec.EdiOp, PPEDIOP_ORDER, PPEDIOP_RECADV)) {
				dlvr_addr_id = pPack->Rec.LocID;
			}
			if(dlvr_addr_id && LocObj.Search(dlvr_addr_id, &loc_rec) > 0) {
				pBill->DlvrAddrID = dlvr_addr_id;
				(temp_buf = loc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
				STRNSCPY(pBill->DlvrAddrCode, temp_buf);
				LocationCore::GetExField(&loc_rec, LOCEXSTR_FULLADDR, temp_buf);
				if(temp_buf.NotEmptyS())
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->DlvrAddr, sizeof(pBill->DlvrAddr));
				else {
					LocationCore::GetExField(&loc_rec, LOCEXSTR_SHORTADDR, temp_buf);
					temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->DlvrAddr, sizeof(pBill->DlvrAddr));
				}
			}
		}
		if(pPack->Ext.AgentID) {
			GetArticleName(pPack->Ext.AgentID, temp_buf);
			temp_buf.Transf(CTRANSF_INNER_TO_OUTER).CopyTo(pBill->AgentName, sizeof(pBill->AgentName));
			GetReg(pPack->Ext.AgentID, PPREGT_TPID, temp_buf);
			temp_buf.CopyTo(pBill->AgentINN, sizeof(pBill->AgentINN));
			GetReg(pPack->Ext.AgentID, PPREGT_GLN, temp_buf);
			temp_buf.CopyTo(pBill->AgentGLN, sizeof(pBill->AgentGLN));
			pBill->AgentPersonID = ObjectToPerson(pPack->Ext.AgentID, 0);
		}
		{
			PPID   main_org_id = 0;
			if(GetMainOrgID(&main_org_id)) {
				RegisterTbl::Rec reg_rec;
				if(PsnObj.GetRegister(main_org_id, PPREGT_GLN, &reg_rec) > 0)
					STRNSCPY(pBill->MainGLN, reg_rec.Num);
				// @vmiller {
				if(PsnObj.GetRegister(main_org_id, PPREGT_TPID, &reg_rec) > 0)
					STRNSCPY(pBill->MainINN, reg_rec.Num);
				// } @vmiller
			}
		}
		// @vmiller {
		// Запишем префикс документа
		if(!isempty(BillParam.ImpExpParamDll.XmlPrefix))
			STRNSCPY(pBill->XmlPrefix, BillParam.ImpExpParamDll.XmlPrefix);
		//
		// Запишем ИД тега типа товара "Пиво"
		//
		if(BillParam.ImpExpParamDll.BeerGrpID)
			pBill->BeerGrpID = BillParam.ImpExpParamDll.BeerGrpID;
		{
			//
			// Запишем GUID документа
			//
			S_GUID guid;
			if(P_BObj->GetGuid((temp_buf = pBill->ID).ToLong(), &guid) > 0)
				STRNSCPY(pBill->GUID, guid.ToStr(S_GUID::fmtIDL, temp_buf));
		}
		//
		// Запишем номер и дату регистра контрагента
		{
			const  PPBillConfig & cfg = DS.GetTLA().P_BObj->GetConfig(); //P_BObj->GetConfig();
			RegisterTbl::Rec reg_rec;
			if(cfg.ContractRegTypeID && PsnObj.GetRegister(ObjectToPerson(pPack->Rec.Object), cfg.ContractRegTypeID, &reg_rec) > 0) {
				STRNSCPY(pBill->CntractCode, reg_rec.Num);
				pBill->CntractDt = reg_rec.Dt;
				pBill->CntractExpry = reg_rec.Expiry;
			}
		}
		// } @vmiller
		ok = 1;
	}
	return ok;
}

int PPBillExporter::GetReg(PPID arID, PPID regTypeID, SString & rRegNum)
{
	rRegNum.Z();
	int    ok = -1;
	RegisterTbl::Rec reg_rec;
	if(arID && regTypeID) {
		// @v11.3.2 {
		PPID   acc_sheet_id = 0;
		PPID   lnk_obj_id = 0;
		if(GetArticleSheetID(arID, &acc_sheet_id, &lnk_obj_id) > 0) {
			PPObjAccSheet acc_sheet_obj;
			PPAccSheet acs_rec;
			if(acc_sheet_obj.Fetch(acc_sheet_id, &acs_rec) > 0) {
				if(acs_rec.Assoc == PPOBJ_PERSON) {
					if(PsnObj.GetRegister(lnk_obj_id, regTypeID, &reg_rec) > 0) {
						rRegNum = reg_rec.Num;
						ok = 1;
					}
				}
				else if(acs_rec.Assoc == PPOBJ_LOCATION) {
					if(LocObj.GetRegister(lnk_obj_id, regTypeID, ZERODATE, 1/*inheritFromOwner*/, &reg_rec) > 0) {
						rRegNum = reg_rec.Num;
						ok = 1;
					}
				}
			}
		}
		// } @v11.3.2 
		/* @v11.3.2 if(PsnObj.GetRegister(ObjectToPerson(arID), regTypeID, &reg_rec) > 0) {
			rRegNum = reg_rec.Num;
			ok = 1;
		}*/
	}
	return ok;
}
//
// @vmiller
// Descr: Получает список документов, которые были успешно отправлены и ставит в тегах этих документов пометку
//
int PPBillExporter::CheckBillsWasExported(ImpExpDll * pExpDll)
{
	int    ok = 1, r = 1;
	{
		Sdr_DllImpExpReceipt exp_rcpt;
		// @v10.7.9 @ctr MEMSZERO(exp_rcpt);
		PPTransaction tra(1);
		THROW(tra);
		while((r = pExpDll->EnumExpReceipt(&exp_rcpt)) == 1) {
			if(exp_rcpt.ID) {
				PPBillPacket bill_pack;
				uint declined = 0;
				if(P_BObj->ExtractPacket(exp_rcpt.ID, &bill_pack))
					declined = bill_pack.Rec.Flags2 & BILLF2_DECLINED;
				if(BillParam.ImpExpParamDll.RcptTagID && (!isempty(exp_rcpt.ReceiptNumber) || declined)) { // Если отмена заказа, то ReceiptNumber не обязательно должен быть непустым
					ObjTagList tag_list;
					ObjTagItem tag_item;
					P_BObj->GetTagList(exp_rcpt.ID, &tag_list);
					tag_item.Init(BillParam.ImpExpParamDll.RcptTagID);
					tag_item.TagDataType = OTTYP_STRING;
					tag_item.SetStr(tag_item.TagID, declined ? "" : exp_rcpt.ReceiptNumber); // Если отмена заказа, то очищаем тег
					tag_list.PutItem(tag_item.TagID, &tag_item);
					THROW(P_BObj->SetTagList(exp_rcpt.ID, &tag_list, 0));
				}
			}
			//if(!isempty(exp_rcpt.ReceiptNumber) && exp_rcpt.ID && /*P_IEBill->.GetParam()*/BillParam.ImpExpParamDll.RcptTagID) {
			//	// Записываем номер квитанции об отправке в тег документа
			//	ObjTagList tag_list;
			//	ObjTagItem tag_item;
			//	BillObj->GetTagList(exp_rcpt.ID, &tag_list);
			//	tag_item.Init(/*P_IEBill->.GetParam()*/BillParam.ImpExpParamDll.RcptTagID);
			//	tag_item.TagDataType = OTTYP_STRING;
			//	tag_item.SetStr(tag_item.TagID, exp_rcpt.ReceiptNumber);
			//	tag_list.PutItem(tag_item.TagID, &tag_item);
			//	THROW(BillObj->SetTagList(exp_rcpt.ID, &tag_list, 0));
			//}
			MEMSZERO(exp_rcpt);
		}
		THROW(r);
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}
//
//
//
int DocNalogRu_Generator::MakeOutFileIdent(FileInfo & rHi)
{
	int    ok = 1;
	SString temp_buf;
	(rHi.FileId = rHi.FormatPrefix).CatChar('_').Cat(rHi.ReceiverIdent).CatChar('_').Cat(rHi.SenderIdent).CatChar('_').
		Cat(temp_buf.Z().Cat(rHi.CurDtm.d, DATF_ANSI|DATF_CENTURY|DATF_NODIV)).CatChar('_').Cat(S_GUID(SCtrGenerate()), S_GUID::fmtIDL);
	return ok;
}

int DocNalogRu_Generator::MakeOutFileName(const char * pFileIdent, SString & rFileName)
{
	int    ok = 1;
	SPathStruc ps(rFileName);
	SFileFormat::GetExt(SFileFormat::Xml, ps.Ext);
	ps.Nam = pFileIdent;
	ps.Merge(rFileName);
	return ok;
}

int DocNalogRu_Generator::CreateHeaderInfo(const char * pFormatPrefix, PPID senderID, PPID rcvrID, PPID providerID, const char * pBaseFileName, FileInfo & rInfo)
{
	int    ok = 1;
	Reference * p_ref = PPRef;
	SString temp_buf;
	rInfo.CurDtm = getcurdatetime_();
	rInfo.SenderPersonID = senderID;
	rInfo.ReceiverPersonID = rcvrID;
	rInfo.ProviderPersonID = providerID;
	rInfo.FormatPrefix = pFormatPrefix;
	rInfo.Uuid.Z();
	rInfo.SenderIdent.Z();
	rInfo.ReceiverIdent.Z();
	rInfo.ProviderIdent.Z();
	rInfo.ProviderName.Z();
	rInfo.ProviderINN.Z();
	rInfo.FileId.Z();
	rInfo.FileName.Z();
	PersonTbl::Rec psn_rec;
	if(rInfo.ProviderPersonID) {
		if(PsnObj.Search(rInfo.ProviderPersonID, &psn_rec) > 0) {
			PsnObj.GetRegNumber(rInfo.ProviderPersonID, PPREGT_DTO_ID, rInfo.ProviderIdent);
			rInfo.ProviderName = psn_rec.Name;
			PsnObj.GetRegNumber(rInfo.ProviderPersonID, PPREGT_TPID, rInfo.ProviderINN);
		}
		else
			rInfo.ProviderPersonID = 0;
	}
	if(rInfo.SenderPersonID) {
		if(PsnObj.Search(rInfo.SenderPersonID, &psn_rec) > 0) {
			p_ref->Ot.GetTagStr(PPOBJ_PERSON, rInfo.SenderPersonID, PPTAG_PERSON_ENALOGID, rInfo.SenderIdent);
			if(psn_rec.Flags & PSNF_NOVATAX)
				rInfo.Flags |= rInfo.fVatFree;
		}
		else
			rInfo.SenderPersonID = 0;
	}
	if(rInfo.ReceiverPersonID) {
		if(PsnObj.Search(rInfo.ReceiverPersonID, &psn_rec) > 0)
			p_ref->Ot.GetTagStr(PPOBJ_PERSON, rInfo.ReceiverPersonID, PPTAG_PERSON_ENALOGID, rInfo.ReceiverIdent);
		else
			rInfo.ReceiverPersonID = 0;
	}
	MakeOutFileIdent(rInfo);
	rInfo.FileName = pBaseFileName;
	MakeOutFileName(rInfo.FileId, rInfo.FileName);
	return ok;
}

DocNalogRu_Generator::File::File(DocNalogRu_Generator & rG, const FileInfo & rHi) : N(rG.P_X, rG.GetToken_Ansi(PPHSC_RU_FILE))
{
	Reference * p_ref = PPRef;
	SString temp_buf;
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_IDFILE)/*"ИдФайл"*/, rHi.FileId);
	{
		SString ver_buf;
		PPVersionInfo vi = DS.GetVersionInfo();
		SVerT ver = vi.GetVersion();
		//vi.GetProductName(ver_buf);
		vi.GetTextAttrib(vi.taiProductName, ver_buf);
		ver_buf.Space().Cat(ver.ToStr(temp_buf));
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_VERPROG)/*"ВерсПрог"*/, ver_buf);
	}
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_VERFORM)/*"ВерсФорм"*/, "5.01");
	{
		SXml::WNode n_(rG.P_X, rG.GetToken_Ansi(PPHSC_RU_EDISIDESINFO)); // Сведения об участниках электронного документооборота
		if(rHi.ProviderPersonID) {
			// Сведения об операторе электронного документооборота отправителя информации исполнителя
			// Обязателен при направлении документа через оператора ЭДО СФ
			SXml::WNode n_op(rG.P_X, rG.GetToken_Ansi(PPHSC_RU_EDIPROVIDERINFO));
			n_op.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_NAMEOFORG), rG.EncText(rHi.ProviderName)); // Наименование
			n_op.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_INNJUR), rHi.ProviderINN); // ИНН
			// Идентификатор оператора электронного оборота счетов-фактур и первичных документов (оператор ЭДО СФ) -
			// символьный трехзначный код. В значении идентификатора допускаются символы латинского алфавита A-Z, a–z,
			// цифры 0–9, знаки «@», «.», «-».
			//Значение идентификатора регистронезависимо. При включении оператора ЭДО СФ в сеть доверенных операторов
			// ЭДО СФ ФНС России, идентификатор присваивается ФНС России
			n_op.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_EDIPROVIDERID), rG.EncText(rHi.ProviderIdent));
		}
		if(rHi.SenderIdent.NotEmpty()) {
			if(rHi.ProviderIdent.NotEmpty())
				(temp_buf = rHi.ProviderIdent).CatChar('-').Cat(rHi.SenderIdent);
			else
				temp_buf = rHi.SenderIdent;
		}
		else
			temp_buf.Z();
		n_.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_SENDERID), temp_buf);
		//
		if(rHi.ReceiverIdent.NotEmpty()) {
			if(rHi.ProviderIdent.NotEmpty())
				(temp_buf = rHi.ProviderIdent).CatChar('-').Cat(rHi.ReceiverIdent);
			else
				temp_buf = rHi.ReceiverIdent;
		}
		else
			temp_buf.Z();
		n_.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_RECEIVERID), temp_buf);
	}
}

DocNalogRu_Generator::Document::Document(DocNalogRu_Generator & rG, const DocumentInfo & rInfo) : N(rG.P_X, rG.GetToken_Ansi(PPHSC_RU_DOCUMENT))
{
	SString temp_buf;
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_KND), rInfo.KND); // Код по Классификатору налоговой документации
	if(rInfo.Function.NotEmpty())
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_FUNCTION), rInfo.Function);
	//Функция="СЧФДОП"
	/*
	if(!!rDtm) {
		temp_buf.Z().Cat(rDtm.d, DATF_GERMANCENT);
		N.PutAttrib("ДатаИнфЗак", temp_buf);
		temp_buf.Z().Cat(rDtm.t, TIMF_HMS|TIMF_DOTDIV);
		N.PutAttrib("ВремИнфЗак", temp_buf);
	}
	*/
	if(oneof2(rInfo.KND, "1115131", "1115101")) {
		LDATETIME dtm_now = getcurdatetime_();
		temp_buf.Z().Cat(dtm_now.d, DATF_GERMANCENT);
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_SELLERINFODATE), temp_buf);
		temp_buf.Z().Cat(dtm_now.t, TIMF_HMS|TIMF_DOTDIV);
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_SELLERINFOTIME), temp_buf);
	}
	if(rInfo.NameOfDoc.NotEmpty()) {
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_NAMEOFDOC), rG.EncText(temp_buf = rInfo.NameOfDoc));
	}
	if(rInfo.NameOfDoc2.NotEmpty()) {
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_NAMEOFDOC2), rG.EncText(temp_buf = rInfo.NameOfDoc2));
	}
	if(rInfo.Subj.NotEmpty()) {
		N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_NAMEECSUBJCOMP)/*"НаимЭконСубСост"*/, rG.EncText(temp_buf = rInfo.Subj));
		if(rInfo.SubjReason.NotEmpty()) {
			N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_REASONECSUBJCOMP)/*"ОснДоверОргСост"*/, rG.EncText(temp_buf = rInfo.SubjReason));
		}
	}
}

DocNalogRu_Generator::Invoice::Invoice(DocNalogRu_Generator & rG, const PPBillPacket & rBp) : N(rG.P_X, rG.GetToken_Ansi(PPHSC_RU_INVOICEHEADER))
{
	SString temp_buf;
	// @v11.1.12 BillCore::GetCode(temp_buf = rBp.Rec.Code);
	temp_buf = rBp.Rec.Code; // @v11.1.12 
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_INVOICENUMBER), rG.EncText(temp_buf));
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_INVOICEDATE), temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT));
	N.PutAttrib(rG.GetToken_Ansi(PPHSC_RU_CODEOKV), "643");
}

DocNalogRu_Generator::DocNalogRu_Generator() : P_X(0), P_Doc(0), Flags(0)
{
	// @v10.9.9 {
	PPIniFile ini_file;
	int    iv = 0;
	if(ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_EXPCHZNGTINSER, &iv) > 0 && iv == 1)
		Flags |= fExpChZnMarksGTINSER;
	// } @v10.9.9
}

DocNalogRu_Generator::~DocNalogRu_Generator()
{
	EndDocument();
}

int DocNalogRu_Generator::StartDocument(const char * pFileName)
{
	int    ok = 1;
	ZDELETE(P_Doc);
	xmlFreeTextWriter(P_X);
	P_X = 0;
	THROW(P_X = xmlNewTextWriterFilename(pFileName, 0));
	xmlTextWriterSetIndent(P_X, 1);
	xmlTextWriterSetIndentTab(P_X);
	THROW_SL(P_Doc = new SXml::WDoc(P_X, cp1251));
	CATCHZOK
	return ok;
}

void DocNalogRu_Generator::EndDocument()
{
	xmlTextWriterFlush(P_X);
	ZDELETE(P_Doc);
	xmlFreeTextWriter(P_X);
	P_X = 0;
}

int DocNalogRu_Generator::WriteInvoiceItems(const FileInfo & rHi, const PPBillPacket & rBp)
{
	int    ok = 1;
	int    tiamt = 0;
	double total_amt = 0.0;
	double total_amt_wovat = 0.0;
	double total_vat = 0.0;
	const  long exclude_tax_flags = (rHi.Flags & FileInfo::fVatFree) ? GTAXVF_VAT : 0L;
	SString temp_buf;
	SString unit_name;
	SString unit_code;
	SString goods_code;
	PPLotExtCodeContainer::MarkSet::Entry msentry;
	PPLotExtCodeContainer::MarkSet ext_codes_set;
	if(rBp.OutAmtType == TIAMT_COST)
		tiamt = TIAMT_COST;
	else if(rBp.OutAmtType == TIAMT_PRICE)
		tiamt = TIAMT_PRICE;
	else
		tiamt = TIAMT_AMOUNT;
	SXml::WNode n_t(P_X, GetToken_Ansi(PPHSC_RU_INVOICETAB));
	for(uint item_idx = 0; item_idx < rBp.GetTCount(); item_idx++) {
	// <СведТов НалСт="18%" НомСтр="1" НаимТов="Товар" ОКЕИ_Тов="796" КолТов="5" ЦенаТов="1.00" СтТовБезНДС="5.00" СтТовУчНал="5.90">
		double vat_sum = 0.0;
		double excise_sum = 0.0;
		const PPTransferItem & r_ti = rBp.ConstTI(item_idx);
		const double qtty_local = fabs(r_ti.Qtty());
		const PPID goods_id = labs(r_ti.GoodsID);
		unit_name.Z();
		unit_code.Z();
		goods_code.Z();
		// my -- {
		// <СведТов КолТов="1.00" НаимТов="ТО системы защиты от краж ООО "Лента" ТК-180 Петрозаводск, пр. Комсомольский, 27"
			// НалСт="без НДС" НомСтр="1" ОКЕИ_Тов="796" СтТовБезНДС="9000.00" СтТовУчНал="9000.00" ЦенаТов="9000.00"></СведТов>
		// } -- my
		// <СведТов
			// НомСтр="1"
			// НаимТов="ТО системы защиты от краж ООО "Лента" ТК-180 Петрозаводск, пр. Комсомольский, 27"
			// ОКЕИ_Тов="796"
			// КолТов="1"
			// ЦенаТов="9000.00"
			// СтТовБезНДС="9000.00"
			// НалСт="без НДС"
			// СтТовУчНал="9000.00">
		// </СведТов>
		// !!! <ДопСведТов ПрТовРаб="3" КодТов="00000000027" НаимЕдИзм="шт"/>
		SXml::WNode n_item(P_X, GetToken_Ansi(PPHSC_RU_WAREINFO));
		temp_buf.Z().Cat(r_ti.RByBill);
		n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_LINENUMBER), temp_buf);
		{
			Goods2Tbl::Rec goods_rec;
			if(GObj.Fetch(goods_id, &goods_rec) > 0) {
				PPUnit u_rec;
				temp_buf = goods_rec.Name;
				n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WARENAME), EncText(temp_buf));
				if(GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
					(unit_name = u_rec.Name).Transf(CTRANSF_INNER_TO_OUTER); // @v10.6.11
					unit_code = isempty(u_rec.Code) ? "0000" : u_rec.Code;
				}
				else
					unit_code = "0000";
				n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREOKEI), unit_code);
				GObj.GetSingleBarcode(goods_id, goods_code);
				if(goods_code.IsEmpty())
					goods_code.CatLongZ(goods_id, 11);
				else
					goods_code.Transf(CTRANSF_INNER_TO_OUTER);
			}
		}
		temp_buf.Z().Cat(qtty_local, MKSFMTD(0, 2, 0)); // @v10.6.10  MKSFMTD(0, 6, NMBF_NOTRAILZ)-->MKSFMTD(0, 2, 0)
		n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREQTTY), temp_buf);
		{
			/*
			<xs:restriction base="xs:string">
				<xs:maxLength value="7"/>
				<xs:minLength value="1"/>
				<xs:enumeration value="0%"/>
				<xs:enumeration value="10%"/>
				<xs:enumeration value="18%"/>
				<xs:enumeration value="10/110"/>
				<xs:enumeration value="18/118"/>
				<xs:enumeration value="без НДС"/>
			</xs:restriction>
			*/
			GTaxVect vect;
			long   exclude_tax_flags = GTAXVF_SALESTAX;
			vect.CalcTI(r_ti, rBp.Rec.OpID, tiamt, exclude_tax_flags); // @v10.9.6 TIAMT_PRICE-->tiamt
			{
				temp_buf.Z();
				if(rHi.Flags & FileInfo::fVatFree) {
					temp_buf.Cat(GetToken_Ansi(PPHSC_RU_NOVAT_VAL));
				}
				else {
					double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
					temp_buf.Cat(vat_rate, MKSFMTD(0, 0, NMBF_NOTRAILZ)).CatChar('%');
				}
				n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_TAXRATE), temp_buf);
			}
			//
			const double amt_wo_tax = vect.GetValue(GTAXVF_AFTERTAXES);
			temp_buf.Z().Cat(amt_wo_tax / fabs(r_ti.Quantity_), MKSFMTD(0, 2, 0)); // @v10.6.10 MKSFMTD(0, 11, NMBF_NOTRAILZ)-->MKSFMTD(0, 2, 0)
			n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREPRICE), temp_buf);
			//
			total_amt_wovat += amt_wo_tax;
			temp_buf.Z().Cat(amt_wo_tax, MKSFMTD(0, 2, /*NMBF_NOTRAILZ*/0));
			n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREAMTWOVAT), temp_buf);
			//
			const double amt = vect.GetValue(GTAXVF_BEFORETAXES);
			total_amt += amt;
			temp_buf.Z().Cat(amt, MKSFMTD(0, 2, /*NMBF_NOTRAILZ*/0));
			n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREAMT), temp_buf);

			vat_sum = vect.GetValue(GTAXVF_VAT);
			excise_sum = vect.GetValue(GTAXVF_EXCISE);
		}
		{
			SXml::WNode n_e(P_X, GetToken_Ansi(PPHSC_RU_EXCISE));
			if(excise_sum != 0.0)
				n_e.PutInner(GetToken_Ansi(PPHSC_RU_AMTEXCISE), temp_buf.Z().Cat(fabs(excise_sum), MKSFMTD(0, 2, 0)));
			else {
				temp_buf = GetToken_Ansi(PPHSC_RU_NOEXCISE_VAL);
				n_e.PutInner(GetToken_Ansi(PPHSC_RU_NOEXCISE_TAG), temp_buf);
			}
		}
		{
			SXml::WNode n_e(P_X, GetToken_Ansi(PPHSC_RU_AMTTAX));
			if(vat_sum != 0.0)
				n_e.PutInner(GetToken_Ansi(/*PPHSC_RU_AMTVAT*/PPHSC_RU_AMTTAX), temp_buf.Z().Cat(fabs(vat_sum), MKSFMTD(0, 2, 0)));
			else {
				temp_buf = GetToken_Ansi(PPHSC_RU_NOVAT_VAL);
				n_e.PutInner(GetToken_Ansi(PPHSC_RU_NOVAT_TAG), temp_buf);
			}
			total_vat += vat_sum;
		}
		// @v10.6.11 {
		{
			SXml::WNode n_e(P_X, GetToken_Ansi(PPHSC_RU_WAREEXTRAINFO));
			// !!! <ДопСведТов ПрТовРаб="3" КодТов="00000000027" НаимЕдИзм="шт"/>
			n_e.PutAttrib(GetToken_Ansi(PPHSC_RU_WARECODE), goods_code);
			n_e.PutAttrib(GetToken_Ansi(PPHSC_RU_UNITNAME), unit_name);
			{
				if(rBp.XcL.Get(item_idx+1, 0, ext_codes_set) > 0 && ext_codes_set.GetCount()) {
					SXml::WNode n_marks(P_X, GetToken_Ansi(PPHSC_RU_WAREIDENTBLOCK));
					SString chzn_gtin14_buf;
					SString chzn_serial_buf;
					StringSet ss;
					ext_codes_set.GetByBoxID(0, ss);
					for(uint ecsp = 0; ss.get(&ecsp, temp_buf);) {
						bool  is_mark_accepted = false;
						if(Flags & fExpChZnMarksGTINSER) {
							GtinStruc gts;
							const int pczcr = PPChZnPrcssr::ParseChZnCode(temp_buf, gts, 0);
							if(pczcr) {
								gts.GetToken(GtinStruc::fldGTIN14, &chzn_gtin14_buf);
								gts.GetToken(GtinStruc::fldSerial, &chzn_serial_buf);
								if(chzn_gtin14_buf.NotEmpty() && chzn_serial_buf.NotEmpty()) {
									temp_buf.Z().Cat("01").Cat(chzn_gtin14_buf).Cat("21").Cat(chzn_serial_buf);
									n_marks.PutInner(GetToken_Ansi(PPHSC_RU_WAREIDENT_PACKCODE), EncText(temp_buf));
									is_mark_accepted = true;
								}
							}
						}
						if(!is_mark_accepted) {
							n_marks.PutInner(GetToken_Ansi(PPHSC_RU_WAREIDENT_PACKCODE), EncText(temp_buf));
						}
					}
				}
			}
		}
		// } @v10.6.11
	}
	{
		SXml::WNode n_t(P_X, GetToken_Ansi(PPHSC_RU_TOTALTOPAYM)/*"ВсегоОпл"*/);
		n_t.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREAMTWOVATTOTAL), temp_buf.Z().Cat(total_amt_wovat, MKSFMTD(0, 2, 0)));
		n_t.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREAMTTOTAL), temp_buf.Z().Cat(total_amt, MKSFMTD(0, 2, 0)));
		{
			SXml::WNode n_e(P_X, GetToken_Ansi(PPHSC_RU_AMTTAXTOTAL));
			if(total_vat != 0.0)
				n_e.PutInner(GetToken_Ansi(/*PPHSC_RU_AMTVAT*/PPHSC_RU_AMTTAX), temp_buf.Z().Cat(fabs(total_vat), MKSFMTD(0, 2, 0)));
			else {
				temp_buf = GetToken_Ansi(PPHSC_RU_NOVAT_VAL);
				n_e.PutInner(GetToken_Ansi(PPHSC_RU_NOVAT_TAG), temp_buf);
			}
		}
	}
	return ok;
}

int DocNalogRu_Generator::WriteParticipant(const char * pHeaderTag, PPID psnID)
{
	int    ok = 1;
	PPPersonPacket psn_pack;
	THROW(PsnObj.GetPacket(psnID, &psn_pack, 0) > 0);
	{
		SString temp_buf;
		SString inn;
		psn_pack.GetRegNumber(PPREGT_TPID, inn);
		SXml::WNode n_h(P_X, pHeaderTag);
		{
			SXml::WNode n_id(P_X, GetToken_Ansi(PPHSC_RU_IDPARTICIPANT));
			if(inn.Len() == 12) { // Физическое лицо
				SXml::WNode n__(P_X, GetToken_Ansi(PPHSC_RU_PRIVEINFO));
				n__.PutAttrib(GetToken_Ansi(PPHSC_RU_INNPHS), EncText(inn));
				WriteFIO(psn_pack.Rec.Name, 0, false);
			}
			else if(inn.Len() == 10) { // Юридическое лицо
				SXml::WNode n__(P_X, GetToken_Ansi(PPHSC_RU_ORGINFO));
				if(psn_pack.GetExtName(temp_buf) <= 0)
					temp_buf = psn_pack.Rec.Name;
				n__.PutAttrib(GetToken_Ansi(PPHSC_RU_NAMEOFORG), EncText(temp_buf));
				n__.PutAttrib(GetToken_Ansi(PPHSC_RU_INNJUR), EncText(inn));
				psn_pack.GetRegNumber(PPREGT_KPP, temp_buf);
				n__.PutAttribSkipEmpty(GetToken_Ansi(PPHSC_RU_KPP), EncText(temp_buf));
			}
			else { // Вероятно, иностранец. Хотя, может быть просто не вбит ИНН

			}
			if(!LocationCore::IsEmptyAddressRec(psn_pack.RLoc)) {
				THROW(WriteAddress(psn_pack.RLoc, 0, PPHSC_RU_ADDRESS));
			}
			else if(!LocationCore::IsEmptyAddressRec(psn_pack.Loc)) {
				THROW(WriteAddress(psn_pack.Loc, 0, PPHSC_RU_ADDRESS));
			}
		}
	}
	CATCHZOK
	return ok;
}

int DocNalogRu_Generator::WriteFIO(const char * pName, long parentTokId, bool asTags)
{
	int    ok = 0;
	SString temp_buf;
	SString first_name, sername, patronimic;
	SString fullname_buf(pName);
	fullname_buf.Strip().ReplaceStr(",", " ", 0).ReplaceStr(";", " ", 0).ReplaceStr("  ", " ", 0);
	{
		StringSet ss;
		fullname_buf.Tokenize(" ,;.", ss);
		for(uint ssp = 0, n = 0; ss.get(&ssp, temp_buf);) {
			n++;
			if(n == 1)
				sername = temp_buf.Strip();
			else if(n == 2)
				first_name = temp_buf.Strip();
			else if(n == 3)
				patronimic = temp_buf.Strip();
		}
		if(sername.NotEmpty() && first_name.NotEmpty()) {
			SXml::WNode n_fio(P_X, GetToken_Ansi(NZOR(parentTokId, PPHSC_RU_FIO)));
			if(asTags) {
				n_fio.PutInner(GetToken_Ansi(PPHSC_RU_SURNAME), EncText(sername));
				n_fio.PutInner(GetToken_Ansi(PPHSC_RU_NAME), EncText(first_name));
				n_fio.PutInnerSkipEmpty(GetToken_Ansi(PPHSC_RU_PATRONYMIC), EncText(patronimic));
			}
			else {
				n_fio.PutAttrib(GetToken_Ansi(PPHSC_RU_SURNAME), EncText(sername));
				n_fio.PutAttrib(GetToken_Ansi(PPHSC_RU_NAME), EncText(first_name));
				n_fio.PutAttribSkipEmpty(GetToken_Ansi(PPHSC_RU_PATRONYMIC), EncText(patronimic));
			}
			ok = 1;
		}
	}
	return ok;
}

int DocNalogRu_Generator::Underwriter(PPID psnID)
{
	int    ok = 1;
	int    is_free = 0; // Индивидуальный предприниматель
	SString inn;
	SString temp_buf;
	PersonTbl::Rec psn_rec;
	const  PPID main_org_id = GetMainOrgID();
	if(main_org_id && PsnObj.Search(main_org_id, &psn_rec) > 0) {
		RegisterTbl::Rec reg_rec;
		if(PsnObj.GetRegister(main_org_id, PPREGT_TPID, getcurdate_(), &reg_rec) > 0) {
			inn = reg_rec.Num;
			if(inn.Len() == 12)
				is_free = 1;
		}
		if(!psnID) {
			psnID = is_free ? main_org_id : CConfig.MainOrgDirector_;
		}
		if(psnID && PsnObj.Search(psnID, &psn_rec) > 0) {
			SXml::WNode n_uw(P_X, GetToken_Ansi(PPHSC_RU_SIGNER));
			n_uw.PutAttrib(GetToken_Ansi(PPHSC_RU_AREAOFAUTHORITY), "0");
			n_uw.PutAttrib(GetToken_Ansi(PPHSC_RU_STATUS), "1");
			temp_buf = GetToken_Ansi(PPHSC_RU_OFFICIAL_DUTIES);
			n_uw.PutAttrib(GetToken_Ansi(PPHSC_RU_FOUNDATIONOFAUTHORITY), temp_buf);
			if(is_free) {
				SXml::WNode n_p(P_X, GetToken_Ansi(PPHSC_RU_PRIVE_S));
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_INNPHS), inn);
				WriteFIO(psn_rec.Name, 0, false);
			}
			else {
				SXml::WNode n_p(P_X, GetToken_Ansi(PPHSC_RU_JUR_S));
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_INNJUR), inn);
				temp_buf = GetToken_Ansi(PPHSC_RU_DIRECTOR);
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_STAFFPOSITION), temp_buf);
				WriteFIO(psn_rec.Name, 0, false);
			}
		}
	}
	return ok;
}

int DocNalogRu_Generator::WriteAddress(const PPLocationPacket & rP, int regionCode, int hdrTag /*PPHSC_RU_ADDRESS||PPHSC_RU_ORGADDR*/)
{
	int    ok = 1;
	PPID   country_id = 0;
	PPCountryBlock cb;
	SString temp_buf;
	SString addr_text;
	LocationCore::GetAddress(rP, 0, addr_text);
	PPLocAddrStruc las;
	las.Recognize((temp_buf = addr_text).Transf(CTRANSF_INNER_TO_OUTER));
	long    hdr_tag = oneof2(hdrTag, PPHSC_RU_ADDRESS, PPHSC_RU_ORGADDR) ? hdrTag : PPHSC_RU_ADDRESS;
	SXml::WNode n__(P_X, GetToken_Ansi(hdr_tag));
	if(hdr_tag == PPHSC_RU_ORGADDR) {
		n__.PutInner(GetToken_Ansi(PPHSC_RU_ADDR_COUNTRYCODE2), "643");
		{
			las.GetText(PPLocAddrStruc::tZip, temp_buf);
			if(temp_buf.IsEmpty())
				LocationCore::GetExField(&rP, LOCEXSTR_ZIP, temp_buf);
			n__.PutInner(GetToken_Ansi(PPHSC_RU_INDEX), temp_buf);
		}
		temp_buf.Z().CatLongZ(regionCode, 2);
		n__.PutInner(GetToken_Ansi(PPHSC_RU_REGIONCODE), temp_buf);
		//n__.PutInner(GetToken_Ansi(PPHSC_RU_DESTRICT), "");
		if(rP.CityID && GetObjectName(PPOBJ_WORLD, rP.CityID, temp_buf) > 0 && temp_buf.NotEmpty()) {
			n__.PutInner(GetToken_Ansi(PPHSC_RU_CITY), EncText(temp_buf));
			//n_i.PutAttrib("НаселПункт", "");
		}
		else if(las.GetText(PPLocAddrStruc::tCity, temp_buf)) {
			n__.PutInner(GetToken_Ansi(PPHSC_RU_CITY), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
		}
		//n__.PutInner(GetToken_Ansi(PPHSC_RU_TOWN), "");
		if(las.GetText(PPLocAddrStruc::tStreet, temp_buf)) {
			n__.PutInner(GetToken_Ansi(PPHSC_RU_STREET), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
		}
		if(las.GetText(PPLocAddrStruc::tHouse, temp_buf)) {
			n__.PutInner(GetToken_Ansi(PPHSC_RU_HOUSE), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
		}
		//n__.PutInner(GetToken_Ansi(PPHSC_RU_HOUSECORP), "");
		if(las.GetText(PPLocAddrStruc::tApart, temp_buf))
			n__.PutInner(GetToken_Ansi(PPHSC_RU_APARTM), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
	}
	else {
		if(PsnObj.LocObj.GetCountry(&rP, &country_id, &cb) > 0 && !cb.IsNative) {
			// Иностранец
			SXml::WNode n_i(P_X, GetToken_Ansi(PPHSC_RU_ADDR_OFFSHR));
			n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_ADDR_COUNTRYCODE), cb.Code);
			n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_ADDR_TEXT), EncText(addr_text));
		}
		else {
			// Резидент
			SXml::WNode n_i(P_X, GetToken_Ansi(PPHSC_RU_ADDR_RF));
			{
				las.GetText(PPLocAddrStruc::tZip, temp_buf);
				if(temp_buf.IsEmpty())
					LocationCore::GetExField(&rP, LOCEXSTR_ZIP, temp_buf);
				n_i.PutAttribSkipEmpty(GetToken_Ansi(PPHSC_RU_INDEX), EncText(temp_buf));
			}
			temp_buf.Z().CatLongZ(regionCode, 2);
			n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_REGIONCODE), temp_buf); // req
			//n_i.PutAttrib("Район", "");
			if(rP.CityID && GetObjectName(PPOBJ_WORLD, rP.CityID, temp_buf) > 0 && temp_buf.NotEmpty()) {
				n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_CITY), EncText(temp_buf));
				//n_i.PutAttrib("НаселПункт", "");
			}
			else if(las.GetText(PPLocAddrStruc::tCity, temp_buf)) {
				n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_CITY), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
			}
			if(las.GetText(PPLocAddrStruc::tStreet, temp_buf)) {
				n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_STREET), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
			}
			if(las.GetText(PPLocAddrStruc::tHouse, temp_buf)) {
				n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_HOUSE), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
			}
			//n_i.PutAttrib("Корпус", "");
			if(las.GetText(PPLocAddrStruc::tApart, temp_buf))
				n_i.PutAttrib(GetToken_Ansi(PPHSC_RU_APARTM), EncText(temp_buf.Transf(CTRANSF_OUTER_TO_INNER)));
		}
	}
	return ok;
}

//int WriteNalogRuPersonBlock(PPObjPerson & rPsnObj, PPID psnID, xmlTextWriter * pWriter)
int DocNalogRu_Generator::WriteOrgInfo_VatLedger(const char * pScopeXmlTag, PPID personID, PPID addrLocID, LDATE actualDate, long flags)
{
	int    ok = -1;
	assert(!isempty(pScopeXmlTag));
    if(P_X && personID && !isempty(pScopeXmlTag)) {
		PersonTbl::Rec psn_rec;
    	if(PsnObj.Fetch(personID, &psn_rec) > 0) {
			SString inn, kpp;
			PsnObj.GetRegNumber(personID, PPREGT_TPID, inn);
			inn.Strip();
			PsnObj.GetRegNumber(personID, PPREGT_KPP, kpp);
			kpp.Strip();
			{
				SXml::WNode n__(P_X, pScopeXmlTag);
				if(inn.Len() == 12) {
					SXml::WNode n(P_X, GetToken_Ansi(PPHSC_RU_PERSONINFO_PHS)/*"СведИП"*/);
					//n.PutAttrib("ИННФЛ", inn); // @todo PPHSC_RU_INNPHS
					n.PutAttrib(GetToken_Ansi(PPHSC_RU_INNPHS), inn);
				}
				else if(inn.Len() == 10) {
					SXml::WNode n(P_X, GetToken_Ansi(PPHSC_RU_PERSONINFO_JUR)/*"СведЮЛ"*/);
					//n.PutAttrib("ИННЮЛ", inn);
					n.PutAttrib(GetToken_Ansi(PPHSC_RU_INNJUR), inn);
					//n.PutAttrib("КПП", kpp);
					n.PutAttribSkipEmpty(GetToken_Ansi(PPHSC_RU_KPP), kpp);
				}
				ok = 1;
			}
    	}
    }
    return ok;
}

int DocNalogRu_Generator::WriteOrgInfo(const char * pScopeXmlTag, PPID personID, PPID addrLocID, LDATE actualDate, long flags)
{
	int    ok = 1;
	int    region_code = 0;
	int    j_status = 0; // 1 - росс юр, 2 - росс ип, 3 - иностранец (не ТС), 4 - иностранец (таможенный союз)
	SString inn, kpp;
	SString temp_buf;
	RegisterTbl::Rec reg_rec;
	PPPersonPacket psn_pack;
	PPLocationPacket loc_pack;
	THROW(PsnObj.GetPacket(personID, &psn_pack, PGETPCKF_USEINHERITENCE) > 0);
	if(addrLocID)
		PsnObj.LocObj.GetPacket(addrLocID, &loc_pack);
	if(psn_pack.Regs.GetRegister(PPREGT_TPID, actualDate, 0, &reg_rec) > 0) {
		(inn = reg_rec.Num).Strip();
	}
	if(loc_pack.Regs.GetRegister(PPREGT_KPP, actualDate, 0, &reg_rec) > 0)
		(kpp = reg_rec.Num).Strip();
	else if(psn_pack.Regs.GetRegister(PPREGT_KPP, actualDate, 0, &reg_rec) > 0)
		(kpp = reg_rec.Num).Strip();
	if(kpp.NotEmpty()) {
		kpp.Sub(0, 2, temp_buf.Z());
		region_code = temp_buf.ToLong();
	}
	else if(inn.NotEmpty()) {
		inn.Sub(0, 2, temp_buf.Z());
		region_code = temp_buf.ToLong();
	}
	if(inn.Len() == 12) {
		j_status = 2;
	}
	else {
		j_status = 1;
	}
	{
		SXml::WNode n__(P_X, pScopeXmlTag);
		{
			SXml::WNode n_id(P_X, GetToken_Ansi(PPHSC_RU_IDPARTICIPANT));
			if(j_status == 2) {
				SXml::WNode n_p(P_X, GetToken_Ansi(PPHSC_RU_PRIVEINFO));
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_INNPHS), inn);
				WriteFIO(psn_pack.Rec.Name, 0, false);
			}
			else {
				SXml::WNode n_p(P_X, GetToken_Ansi(PPHSC_RU_JURINFO));
				if(psn_pack.GetExtName(temp_buf) <= 0)
					temp_buf = psn_pack.Rec.Name;
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_NAMEOFORG), EncText(temp_buf));
				n_p.PutAttrib(GetToken_Ansi(PPHSC_RU_INNJUR), inn);
				n_p.PutAttribSkipEmpty(GetToken_Ansi(PPHSC_RU_KPP), kpp);
			}
		}
		{
			// @v10.1.5 поменял местами MainLoc и RLoc (приоритет у MainLoc)
			if(loc_pack.ID && loc_pack.ID == addrLocID)
				WriteAddress(loc_pack, region_code, PPHSC_RU_ADDRESS);
			else if(psn_pack.Rec.MainLoc && PsnObj.LocObj.GetPacket(psn_pack.Rec.MainLoc, &loc_pack) > 0)
				WriteAddress(loc_pack, region_code, PPHSC_RU_ADDRESS);
			else if(psn_pack.Rec.RLoc && PsnObj.LocObj.GetPacket(psn_pack.Rec.RLoc, &loc_pack) > 0)
				WriteAddress(loc_pack, region_code, PPHSC_RU_ADDRESS);
		}
	}
	CATCHZOK
	return ok;
}

const SString & FASTCALL DocNalogRu_Generator::EncText(const SString & rS)
{
	EncBuf = rS;
	EncBuf.ReplaceChar('\x07', ' ');
	XMLReplaceSpecSymb(EncBuf, "&<>\'");
	return EncBuf.Transf(CTRANSF_INNER_TO_OUTER);
}

int DocNalogRu_Generator::GetAgreementParams(PPID arID, SString & rAgtCode, LDATE & rAgtDate, LDATE & rAgtExpiry)
{
	rAgtCode.Z();
	rAgtDate = ZERODATE;
	rAgtExpiry = ZERODATE;
	int    ok = -1;
	ArticleTbl::Rec ar_rec;
	if(ArObj.Fetch(arID, &ar_rec) > 0) {
		const int agt_kind = ArObj.GetAgreementKind(&ar_rec);
		if(agt_kind == 1) {
			PPClientAgreement cli_agt;
			if(ArObj.GetClientAgreement(arID, cli_agt, 0) > 0) {
				// @v11.2.0 if(!isempty(cli_agt.Code2)) { // @v10.2.9 Code-->Code2
				if(cli_agt.Code_.NotEmpty()) { // @v11.2.0 // @v11.2.1 @fix IsEmpty-->NotEmpty
					// @v11.2.0 rAgtCode = cli_agt.Code2; // @v10.2.9 Code-->Code2
					rAgtCode = cli_agt.Code_; // @v11.2.0
					if(checkdate(cli_agt.BegDt))
						rAgtDate = cli_agt.BegDt;
					rAgtExpiry = cli_agt.Expiry;
					ok = 1;
				}
			}
		}
		/*
		else if(agt_kind == 2) {
			PPSupplAgreement sa_rec;
			if(g.ArObj.GetSupplAgreement(arID, &sa_rec, 0) > 0) {
			}
		}
		*/
	}
	return ok;
}

//DP_REZRUISP_1_990_01_05_01_01.xsd
int WriteBill_NalogRu2_DP_REZRUISP(const PPBillPacket & rBp, const SString & rFileName, SString & rResultFileName)
{
	int    ok = 1;
	rResultFileName.Z(); // @v10.9.8
	DocNalogRu_Generator g;
	{
		PPObjAccSheet acs_obj;
		PPOprKind op_rec;
		PPOprKind link_op_rec;
		SString temp_buf;
		PPID   dto_id = 0; // PPOBJ_PERSON
		PPID   main_org_id = GetMainOrgID(); // PPOBJ_PERSON
		PPID   contragent_id = ObjectToPerson(rBp.Rec.Object, 0); // PPOBJ_PERSON
		DocNalogRu_Generator::FileInfo _hi;
		THROW(g.CreateHeaderInfo("DP_REZRUISP", main_org_id, contragent_id, dto_id, rFileName, _hi));

		THROW(GetOpData(rBp.Rec.OpID, &op_rec) > 0);
		if(op_rec.LinkOpID) {
			THROW(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0);
		}
		THROW(g.StartDocument(_hi.FileName));
        {
			DocNalogRu_Generator::File f(g, _hi);
			// Наименование экономического субъекта – составителя информации исполнителя
			GetMainOrgName(temp_buf);
			DocNalogRu_Generator::DocumentInfo docinfo;
			docinfo.KND = "1175012";
			docinfo.Subj = temp_buf;
			DocNalogRu_Generator::Document d(g, docinfo);
			// Дата формирования документа о передаче результатов работ (документа об оказании услуг), информация исполнителя
			d.N.PutAttrib(g.GetToken_Ansi(PPHSC_RU_EXECUTORINFODATE), temp_buf.Z().Cat(_hi.CurDtm.d, DATF_GERMANCENT));
			// Время формирования документа о передаче результатов работ (документа об оказании услуг), информация исполнителя
			d.N.PutAttrib(g.GetToken_Ansi(PPHSC_RU_EXECUTORINFOTIME), temp_buf.Z().Cat(_hi.CurDtm.t, TIMF_HMS|TIMF_DOTDIV));
			{
				// Сведения документа кроме сведений о передаче результатов работ (о предъявлении оказанных услуг)
				SXml::WNode n_(g.P_X, "СвДокПРУ");
				{
					SXml::WNode n_2(g.P_X, g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC_S));
					temp_buf = g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC_S_SRVC);
					n_2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC2), temp_buf);
					n_2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC), g.EncText(temp_buf = op_rec.Name));
					SXml::WNode n_3(g.P_X, g.GetToken_Ansi(PPHSC_RU_DOCIDENT));
					// @v11.1.12 BillCore::GetCode(temp_buf = rBp.Rec.Code);
					temp_buf = rBp.Rec.Code; // @v11.1.12 
					n_3.PutAttrib("НомДокПРУ", g.EncText(temp_buf));
					n_3.PutAttrib("ДатаДокПРУ", temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT));
					// SXml::WNode n_3(g.P_X, "ИспрДокПРУ");
					// SXml::WNode n_3(g.P_X, "ДенИзм");
					{
						SXml::WNode n_4(g.P_X, g.GetToken_Ansi(PPHSC_RU_TRANSACTIONCONTENT1));
						{
							//SXml::WNode n_41(g.P_X, "ЗагСодОпер"); // @optional
                            g.WriteParticipant(g.GetToken_Ansi(PPHSC_RU_EXECUTOR), main_org_id);
                            g.WriteParticipant(g.GetToken_Ansi(PPHSC_RU_CUSTOMER),  ObjectToPerson(rBp.Rec.Object));
							SXml::WNode n_44(g.P_X, g.GetToken_Ansi(PPHSC_RU_FOUNDATION));
							if(rBp.BTagL.GetItemStr(PPTAG_BILL_STATECONTRACTID, temp_buf) > 0)
								SXml::WNode n_45(g.P_X, g.GetToken_Ansi(PPHSC_RU_STATECONTRID), g.EncText(temp_buf));
							SXml::WNode n_46(g.P_X, "ВидОперации", g.EncText(temp_buf = op_rec.Name));
							{
								SXml::WNode n_47(g.P_X, "ОписРабот"); // [1..]
								n_47.PutAttribSkipEmpty("НачРабот", ""); // date @optional
								n_47.PutAttribSkipEmpty("КонРабот", ""); // date @optional
								n_47.PutAttribSkipEmpty("СтБезНДСИт", ""); // money @optional
								n_47.PutAttribSkipEmpty("СумНДСИт", ""); // money @optional
								n_47.PutAttribSkipEmpty("СтУчНДСИт", ""); // money @optional
								double total_amt = 0.0;
								double total_amt_wovat = 0.0;
								for(uint i = 0; i < rBp.GetTCount(); i++) {
									const PPTransferItem & r_ti = rBp.ConstTI(i);
									Goods2Tbl::Rec goods_rec;
									if(r_ti.Flags & PPTFR_UNLIM && g.GObj.Fetch(r_ti.GoodsID, &goods_rec) > 0) {
										double vat_sum = 0.0;
										double excise_sum = 0.0;
										SXml::WNode n_471(g.P_X, g.GetToken_Ansi(PPHSC_RU_JOB)); // [1..]
										n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_NUMBER), temp_buf.Z().Cat(r_ti.RByBill)); // @optional
										n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_JOBNAME), g.EncText(temp_buf = goods_rec.Name)); // @optional
										{
											PPUnit u_rec;
											if(g.GObj.FetchUnit(goods_rec.UnitID, &u_rec) > 0) {
												n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_UNITNAME), g.EncText(temp_buf = u_rec.Name)); // @optional
												if(u_rec.Code[0]) {
													n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_OKEI), g.EncText(temp_buf = u_rec.Code)); // @optional
												}
												else
													n_471.PutAttrib(g.GetToken_Ansi(PPHSC_RU_OKEI), "0000");
											}
											else {
												temp_buf = g.GetToken_Ansi(PPHSC_RU_UNITNAME_HOUR);
												n_471.PutAttrib(g.GetToken_Ansi(PPHSC_RU_UNITNAME), temp_buf);
												n_471.PutAttrib(g.GetToken_Ansi(PPHSC_RU_OKEI), "356");
											}
										}
										{
											double qtty = fabs(r_ti.Qtty());
											double price = r_ti.NetPrice();
											temp_buf.Z().Cat(qtty, MKSFMTD(0, 6, NMBF_NOTRAILZ));
											n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_QUANTITY), temp_buf); // @optional
											temp_buf.Z().Cat(price, MKSFMTD(0, 2, 0));
											n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_PRICE), temp_buf); // @optional
											//

											GTaxVect vect;
											long   exclude_tax_flags = GTAXVF_SALESTAX;
											vect.CalcTI(r_ti, rBp.Rec.OpID, TIAMT_PRICE, exclude_tax_flags);
											double vat_rate = vect.GetTaxRate(GTAX_VAT, 0);
											temp_buf.Z().Cat(vat_rate, MKSFMTD(0, 0, NMBF_NOTRAILZ)).CatChar('%');
											n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_TAXRATE), temp_buf); // @optional
											//
											double amt_wo_tax = vect.GetValue(GTAXVF_AFTERTAXES);
											temp_buf.Z().Cat(amt_wo_tax / fabs(r_ti.Quantity_), MKSFMTD(0, 11, NMBF_NOTRAILZ));
											//n_item.PutAttrib(GetToken_Ansi(PPHSC_RU_WAREPRICE), temp_buf);
											//
											total_amt_wovat += amt_wo_tax;
											temp_buf.Z().Cat(amt_wo_tax, MKSFMTD(0, 2, /*NMBF_NOTRAILZ*/0));
											n_471.PutAttribSkipEmpty("СтоимБезНДС", temp_buf); // @optional
											//
											double amt = vect.GetValue(GTAXVF_BEFORETAXES);
											total_amt += amt;
											temp_buf.Z().Cat(amt, MKSFMTD(0, 2, /*NMBF_NOTRAILZ*/0));
											//n_item.PutAttrib("СтТовУчНал", temp_buf);

											vat_sum = vect.GetValue(GTAXVF_VAT);
											temp_buf.Z().Cat(vat_sum, MKSFMTD(0, 2, 0));
											n_471.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_AMTVAT), temp_buf); // @optional
											excise_sum = vect.GetValue(GTAXVF_EXCISE);

											temp_buf.Z().Cat(price * qtty, MKSFMTD(0, 2, 0));
											n_471.PutAttribSkipEmpty("СтоимУчНДС", temp_buf); // @optional
										}
										n_471.PutAttribSkipEmpty("КоррСчДебет", ""); // @optional
										n_471.PutAttribSkipEmpty("КоррСчКредит", ""); // @optional
										{
											//SXml::WNode n_4711(g.P_X, "Описание"); // @optional
											//SXml::WNode n_4712(g.P_X, "ИнфПолеОписРабот"); // @optional
										}
									}
								}
							}
							SXml::WNode n_48(g.P_X, g.GetToken_Ansi(PPHSC_RU_EXTRA1));
							n_48.PutAttribSkipEmpty("ИдФайлИнфПол", ""); // @optional
							{
								SString agt_code;
								LDATE  agt_date;
								LDATE  agt_expiry;
								if(g.GetAgreementParams(rBp.Rec.Object, agt_code, agt_date, agt_expiry) > 0) {
									{
										SXml::WNode n_481(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
										temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACT);
										n_481.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
										n_481.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), g.EncText(agt_code));
									}
									if(checkdate(agt_date)) {
										SXml::WNode n_482(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
										temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACTDATE);
										n_482.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
										n_482.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), temp_buf.Z().Cat(agt_date, DATF_GERMANCENT));
									}
									if(checkdate(agt_expiry)) {
										SXml::WNode n_483(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
										temp_buf = g.GetToken_Ansi(PPHSC_RU_PERIOD);
										n_483.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
										n_483.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), temp_buf.Z().Cat(agt_expiry, DATF_GERMANCENT));
									}
								}
								else {
									SXml::WNode n_481(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
								}
							}
						}
					}
				}
			}
			{
				// Содержание факта хозяйственной жизни (2) - сведения о передаче результатов работ (о предъявлении оказанных услуг)
				SXml::WNode n_(g.P_X, g.GetToken_Ansi(PPHSC_RU_TRANSACTIONCONTENT2));
				PPLoadText(PPTXT_NALOGRU_WORKSWERETRANSFERED, temp_buf); // "Результаты работ переданы (услуги оказаны)";
				n_.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONTOFOP), g.EncText(temp_buf));
				// Дата передачи результатов работ (предъявления оказанных услуг)
				// Обязателен, если ДатаПер не совпадает с ДатаДокПРУ
				temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT);
				n_.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_WARETRANSFDATE), temp_buf);
				/*{
					SXml::WNode n_2(g.P_X, "СвПерВещи"); // Сведения о передаче вещи, изготовленной  по договору подряда
					n_2.PutAttribSkipEmpty("ДатаПерВещ", ""); // @optional Дата передачи вещи, изготовленной  по договору
					n_2.PutAttribSkipEmpty("СвПерВещ", "");   // @optional Сведения о передаче
					SXml::WNode n_3(g.P_X, "ИнфПолФХЖ2");
				}*/
			}
			g.Underwriter(0);
		}
		g.EndDocument();
		rResultFileName = _hi.FileName; // @v10.9.8
	}
	CATCHZOK
	return ok;
}

int WriteBill_NalogRu2_Invoice2(const PPBillPacket & rBp, const char * pHeaderSymb, const SString & rFileName, SString & rResultFileName)
{
	int    ok = 1;
	rResultFileName.Z(); // @v10.9.8
	assert(!isempty(pHeaderSymb)); // @v11.2.1
	DocNalogRu_Generator g;
	THROW(!isempty(pHeaderSymb)); // @v11.2.1
	{
		PPObjAccSheet acs_obj;
		PPOprKind op_rec;
		PPOprKind link_op_rec;
		SString temp_buf;
		PPID   main_org_id = 0; // PPOBJ_PERSON
		PPID   dto_id = 0; // PPOBJ_PERSON
		rBp.GetMainOrgID_(&main_org_id);
		PPID   contragent_id = ObjectToPerson(rBp.Rec.Object, 0); // PPOBJ_PERSON
		DocNalogRu_Generator::FileInfo _hi;
		SString agt_code;
		LDATE  agt_date;
		LDATE  agt_expiry;
		g.GetAgreementParams(rBp.Rec.Object, agt_code, agt_date, agt_expiry);
		THROW(g.CreateHeaderInfo(pHeaderSymb/*"ON_NSCHFDOPPRMARK"*/, main_org_id, contragent_id, dto_id, rFileName, _hi));
		THROW(GetOpData(rBp.Rec.OpID, &op_rec) > 0);
		if(op_rec.LinkOpID) {
			THROW(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0);
		}
		THROW(g.StartDocument(_hi.FileName));
        {
			DocNalogRu_Generator::File f(g, _hi);
			GetMainOrgName(temp_buf);
			DocNalogRu_Generator::DocumentInfo docinfo;
			docinfo.KND = "1115131";
			docinfo.Subj = temp_buf;
			docinfo.Function = "СЧФДОП";
			(docinfo.NameOfDoc = g.GetToken_Utf8(PPHSC_RU_NAMEOFDOC_INVCSHIPM)).Transf(CTRANSF_UTF8_TO_INNER);
			(docinfo.NameOfDoc2 = g.GetToken_Utf8(PPHSC_RU_NAMEOFDOC2_SHIPM)).Transf(CTRANSF_UTF8_TO_INNER);
			DocNalogRu_Generator::Document d(g, docinfo);
			{
				DocNalogRu_Generator::Invoice inv(g, rBp);
				PPID   consignee_psn_id = 0;
				PPID   consignee_loc_id = 0;
				PPID   shipper_psn_id = 0;
				PPID   shipper_loc_id = 0;
				PPID   suppl_psn_id = 0;
				PPID   suppl_loc_id = 0;
				PPID   buyer_psn_id = 0;
				int    do_skip = 0;
				int    is_intrexpend = 0;
				{
					if(oneof2(rBp.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)) {
						PPID   ar2_main_org_id = 0;
						if(IsIntrExpndOp(rBp.Rec.OpID)) {
							consignee_psn_id = main_org_id;
							consignee_loc_id = PPObjLocation::ObjToWarehouse(rBp.Rec.Object);
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
							is_intrexpend = 1;
						}
						else {
							consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
							consignee_loc_id = rBp.GetDlvrAddrID();
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
						}
					}
					else if(oneof2(rBp.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
						//wb_type = wbtInvcToMe;
						consignee_psn_id = main_org_id;
						consignee_loc_id = rBp.Rec.LocID;
						buyer_psn_id = consignee_psn_id;
						shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
						shipper_loc_id = rBp.GetDlvrAddrID();
					}
					else if(rBp.OpTypeID == PPOPT_GOODSRETURN) {
						if(op_rec.LinkOpID) {
							if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
								consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								consignee_loc_id = rBp.GetDlvrAddrID();
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = main_org_id;
								shipper_loc_id = rBp.Rec.LocID;
							}
							else if(link_op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
								consignee_psn_id = main_org_id;
								consignee_loc_id = rBp.Rec.LocID;
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								shipper_loc_id = rBp.GetDlvrAddrID();
							}
						}
					}
				}
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_SELLERINFO), shipper_psn_id, /*shipper_loc_id*/0, rBp.Rec.Dt, 0);
				// @v10.8.2 {
				{
					SXml::WNode n_1(g.P_X, g.GetToken_Ansi(PPHSC_RU_CONSIGNORINFO));
					g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_CONSIGNORINFO2), shipper_psn_id, shipper_loc_id, rBp.Rec.Dt, 0);
				}
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_CONSIGNEEINFO), buyer_psn_id, consignee_loc_id, rBp.Rec.Dt, 0);
				// } @v10.8.2
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_BUYERINFO), buyer_psn_id, 0, rBp.Rec.Dt, 0);
				// @v11.3.1 {
				{
					//<ДокПодтвОтгр НаимДокОтгр="Накладная" НомДокОтгр="21-00491132391" ДатаДокОтгр="08.07.2021"/>
					SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOC));
					temp_buf = g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAM_BILL);
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAME), temp_buf);
					temp_buf = rBp.Rec.Code;
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNO), g.EncText(temp_buf));
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCDATE), temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT));
				}
				// } @v11.3.1
			}
			g.WriteInvoiceItems(_hi, rBp);
			{
				SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_WARETRANSFINFO));
				{
					SXml::WNode n_1(g.P_X, g.GetToken_Ansi(PPHSC_RU_WARETRANSFINFO2));
					PPLoadText(PPTXT_NALOGRU_UPDOPCONTENT, temp_buf);
					n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONTOFOP), g.EncText(temp_buf));
					temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT);
					n_1.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_WARETRANSFDATE), temp_buf);
					{
						SXml::WNode n_11(g.P_X, g.GetToken_Ansi(PPHSC_RU_BASISFORWARETRANSFER));
						if(agt_code.NotEmpty()) {
							temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACT);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFBASISFORWARETRANSFER), temp_buf);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NUMBOFBASISFORWARETRANSFER), g.EncText(agt_code));
							temp_buf.Z().Cat(checkdate(agt_date) ? agt_date : encodedate(1, 1, 2017), DATF_GERMANCENT);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_DATEOFBASISFORWARETRANSFER), g.EncText(temp_buf));
						}
						else {
							// @v10.6.12 temp_buf = g.GetToken_Ansi(PPHSC_RU_ABSENCE);
							temp_buf = g.GetToken_Ansi(PPHSC_RU_NODOCOFBASISFORWARETRANSFER); // @v10.6.12
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFBASISFORWARETRANSFER), temp_buf);
						}
						// @v11.0.2 {
						/* @v11.1.12 
						if(!isempty(rBp.Rec.Memo))
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_ADDENDUMOFBASISFORWARETRANSFER), g.EncText(temp_buf.Z().Cat(rBp.Rec.Memo)));
						*/
						// @v11.1.12 {
						if(rBp.SMemo.NotEmpty())
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_ADDENDUMOFBASISFORWARETRANSFER), g.EncText(temp_buf = rBp.SMemo));
						// } @v11.1.12 
						// } @v11.0.2
					}
					{
						//SXml::WNode n_12(g.P_X, "СвЛицПер");
					}
					{
						//SXml::WNode n_13(g.P_X, "ТранГруз");
					}
					{
						//SXml::WNode n_14(g.P_X, "СвПерВещи");
					}
				}
			}
			g.Underwriter(0);
		}
		g.EndDocument();
		rResultFileName = _hi.FileName; // @v10.9.8
	}
	CATCHZOK
	return ok;
}

int WriteBill_NalogRu2_Invoice(const PPBillPacket & rBp, const SString & rFileName, SString & rResultFileName)
{
	int    ok = 1;
	rResultFileName.Z();
	DocNalogRu_Generator g;
	{
		PPObjAccSheet acs_obj;
		PPOprKind op_rec;
		PPOprKind link_op_rec;
		SString temp_buf;
		PPID   main_org_id = 0; // PPOBJ_PERSON
		PPID   dto_id = 0; // PPOBJ_PERSON
		rBp.GetMainOrgID_(&main_org_id);
		PPID   contragent_id = ObjectToPerson(rBp.Rec.Object, 0); // PPOBJ_PERSON
		DocNalogRu_Generator::FileInfo _hi;
		THROW(g.CreateHeaderInfo("ON_SFAKT", main_org_id, contragent_id, dto_id, rFileName, _hi));
		THROW(GetOpData(rBp.Rec.OpID, &op_rec) > 0);
		if(op_rec.LinkOpID) {
			THROW(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0);
		}
		THROW(g.StartDocument(_hi.FileName));
        {
			DocNalogRu_Generator::File f(g, _hi);
			GetMainOrgName(temp_buf);
			DocNalogRu_Generator::DocumentInfo docinfo;
			docinfo.KND = "1115101";
			docinfo.Subj = temp_buf;
			DocNalogRu_Generator::Document d(g, docinfo);
			{
				DocNalogRu_Generator::Invoice inv(g, rBp);
				PPID   main_org_id = 0;
				PPID   consignee_psn_id = 0;
				PPID   consignee_loc_id = 0;
				PPID   shipper_psn_id = 0;
				PPID   shipper_loc_id = 0;
				PPID   suppl_psn_id = 0;
				PPID   suppl_loc_id = 0;
				PPID   buyer_psn_id = 0;
				int    do_skip = 0;
				int    is_intrexpend = 0;
				{
					if(oneof2(rBp.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)) {
						PPID   ar2_main_org_id = 0;
						if(IsIntrExpndOp(rBp.Rec.OpID)) {
							consignee_psn_id = main_org_id;
							consignee_loc_id = PPObjLocation::ObjToWarehouse(rBp.Rec.Object);
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
							is_intrexpend = 1;
						}
						else {
							consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
							consignee_loc_id = rBp.GetDlvrAddrID();
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
						}
					}
					else if(oneof2(rBp.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
						//wb_type = wbtInvcToMe;
						consignee_psn_id = main_org_id;
						consignee_loc_id = rBp.Rec.LocID;
						buyer_psn_id = consignee_psn_id;
						shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
						shipper_loc_id = rBp.GetDlvrAddrID();
					}
					else if(rBp.OpTypeID == PPOPT_GOODSRETURN) {
						if(op_rec.LinkOpID) {
							if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
								consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								consignee_loc_id = rBp.GetDlvrAddrID();
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = main_org_id;
								shipper_loc_id = rBp.Rec.LocID;
							}
							else if(link_op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
								consignee_psn_id = main_org_id;
								consignee_loc_id = rBp.Rec.LocID;
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								shipper_loc_id = rBp.GetDlvrAddrID();
							}
						}
					}
				}
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_SELLERINFO), shipper_psn_id, /*shipper_loc_id*/0, rBp.Rec.Dt, 0); // @v10.8.7 shipper_loc_id-->0
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_BUYERINFO), buyer_psn_id, 0, rBp.Rec.Dt, 0);
				// @v11.3.0 {
				{
					//<ДокПодтвОтгр НаимДокОтгр="Накладная" НомДокОтгр="21-00491132391" ДатаДокОтгр="08.07.2021"/>
					SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOC));
					temp_buf = g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAM_BILL);
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAME), temp_buf);
					temp_buf = rBp.Rec.Code;
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNO), g.EncText(temp_buf));
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCDATE), temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT));
				}
				// } @v11.3.0
			}
			g.WriteInvoiceItems(_hi, rBp);
			g.Underwriter(0);
		}
		g.EndDocument();
		rResultFileName = _hi.FileName; // @v10.9.8
	}
	CATCHZOK
	return ok;
}
//
//           Универсальный передаточный документ КНД=1115125 scheme=ON_SCHFDOPPR_1_995_01_05_01_02.xsd
// @v10.6.10 Универсальный передаточный документ КНД=1115131 scheme=ON_NSCHFDOPR_1_995_01_05_01_02.xsd
//
int WriteBill_NalogRu2_UPD(const PPBillPacket & rBp, const SString & rFileName, SString & rResultFileName)
{
	int    ok = 1;
	rResultFileName.Z();
	DocNalogRu_Generator g;
	{
		PPObjAccSheet acs_obj;
		PPOprKind op_rec;
		PPOprKind link_op_rec;
		SString temp_buf;

		PPID   main_org_id = 0; // PPOBJ_PERSON
		PPID   dto_id = 0; // PPOBJ_PERSON
		rBp.GetMainOrgID_(&main_org_id);
		PPID   contragent_id = ObjectToPerson(rBp.Rec.Object, 0); // PPOBJ_PERSON
		DocNalogRu_Generator::FileInfo _hi;
		SString agt_code;
		LDATE  agt_date = ZERODATE;
		LDATE  agt_expiry = ZERODATE;
		g.GetAgreementParams(rBp.Rec.Object, agt_code, agt_date, agt_expiry);
		THROW(g.CreateHeaderInfo("ON_NSCHFDOPR", main_org_id, contragent_id, dto_id, rFileName, _hi)); // @v10.6.10 ON_SCHFDOPPR-->ON_NSCHFDOPR
		THROW(GetOpData(rBp.Rec.OpID, &op_rec) > 0);
		if(op_rec.LinkOpID) {
			THROW(GetOpData(op_rec.LinkOpID, &link_op_rec) > 0);
		}
		THROW(g.StartDocument(_hi.FileName));
        {
			DocNalogRu_Generator::File f(g, _hi);
			GetMainOrgName(temp_buf);
			DocNalogRu_Generator::DocumentInfo docinfo;
			docinfo.KND = "1115131"; // @v10.6.10 1115125-->1115131
			docinfo.Subj = temp_buf;
			DocNalogRu_Generator::Document d(g, docinfo);
			// СЧФ - счет-фактура, применяемый при расчетах по налогу на добавленную стоимость;
			// СЧФДОП - счет-фактура, применяемый при расчетах по налогу на добавленную стоимость, и документ об отгрузке товаров (выполнении работ), передаче имущественных прав (документ об оказании услуг);
			// ДОП - документ об отгрузке товаров (выполнении работ), передаче имущественных прав (документ об оказании услуг).
			// Под отгрузкой товаров понимается в том числе  передача (поставка, отпуск) товара (груза)
			temp_buf = g.GetToken_Ansi(PPHSC_RU_DOP);
			d.N.PutAttrib(g.GetToken_Ansi(PPHSC_RU_FUNCTION), temp_buf);
			// Наименование документа по факту хозяйственной жизни
			// При Функция=СЧФ не формируется.
			// При Функция=СЧФДОП или Функция=ДОП ПоФактХЖ=Документ об отгрузке товаров (выполнении работ), передаче имущественных прав (документ об оказании услуг)
			PPLoadText(PPTXT_NALOGRU_SHIPPINGBILL, temp_buf);
			d.N.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC2), g.EncText(temp_buf));
			// Наименование первичного документа, определенное организацией (согласованное сторонами сделки)
			// При Функция=СЧФ не формируется.
			// При Функция=СЧФДОП принимает значение «Счет-фактура и документ об отгрузке товаров (выполнении работ), передаче имущественных прав (документ об оказании услуг)».
			// При Функция=ДОП самостоятельно установленное наименование документа или «Документ об отгрузке товаров (выполнении работ), передаче имущественных прав (Документ об оказании услуг)» (по умолчанию)
			temp_buf = op_rec.Name;
			d.N.PutAttribSkipEmpty(g.GetToken_Ansi(PPHSC_RU_NAMEOFDOC), g.EncText(temp_buf));
			//d.N.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SELLERINFODATE), temp_buf.Z().Cat(_hi.CurDtm.d, DATF_GERMANCENT));
			//d.N.PutAttrib(g.GetToken_Ansi(PPHSC_RU_SELLERINFOTIME), temp_buf.Z().Cat(_hi.CurDtm.t, TIMF_HMS|TIMF_DOTDIV));
			{
				DocNalogRu_Generator::Invoice inv(g, rBp);
				PPID   consignee_psn_id = 0;
				PPID   consignee_loc_id = 0;
				PPID   shipper_psn_id = 0;
				PPID   shipper_loc_id = 0;
				PPID   suppl_psn_id = 0;
				PPID   suppl_loc_id = 0;
				PPID   buyer_psn_id = 0;
				int    do_skip = 0;
				int    is_intrexpend = 0;
				{
					if(oneof2(rBp.OpTypeID, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND)) {
						PPID   ar2_main_org_id = 0;
						if(IsIntrExpndOp(rBp.Rec.OpID)) {
							consignee_psn_id = main_org_id;
							consignee_loc_id = PPObjLocation::ObjToWarehouse(rBp.Rec.Object);
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
							is_intrexpend = 1;
						}
						else {
							consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
							consignee_loc_id = rBp.GetDlvrAddrID();
							buyer_psn_id = consignee_psn_id;
							shipper_psn_id = main_org_id;
							shipper_loc_id = rBp.Rec.LocID;
						}
					}
					else if(oneof2(rBp.OpTypeID, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT)) {
						//wb_type = wbtInvcToMe;
						consignee_psn_id = main_org_id;
						consignee_loc_id = rBp.Rec.LocID;
						buyer_psn_id = consignee_psn_id;
						shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
						shipper_loc_id = rBp.GetDlvrAddrID();
					}
					else if(rBp.OpTypeID == PPOPT_GOODSRETURN) {
						if(op_rec.LinkOpID) {
							if(link_op_rec.OpTypeID == PPOPT_GOODSRECEIPT) {
								//wb_type = wbtRetFromMe;
								consignee_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								consignee_loc_id = rBp.GetDlvrAddrID();
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = main_org_id;
								shipper_loc_id = rBp.Rec.LocID;
							}
							else if(link_op_rec.OpTypeID == PPOPT_GOODSEXPEND) {
								//wb_type = wbtRetToMe;
								consignee_psn_id = main_org_id;
								consignee_loc_id = rBp.Rec.LocID;
								buyer_psn_id = consignee_psn_id;
								shipper_psn_id = ObjectToPerson(rBp.Rec.Object, 0);
								shipper_loc_id = rBp.GetDlvrAddrID();
							}
						}
					}
				}
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_SELLERINFO), shipper_psn_id, /*shipper_loc_id*/0, rBp.Rec.Dt, 0); // @v10.8.7 shipper_loc_id-->0
				g.WriteOrgInfo(g.GetToken_Ansi(PPHSC_RU_BUYERINFO), buyer_psn_id, 0, rBp.Rec.Dt, 0);
				{
					SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_TRANSACTIONCONTENTEX1));
					if(rBp.BTagL.GetItemStr(PPTAG_BILL_STATECONTRACTID, temp_buf) > 0)
						n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_STATECONTRID), temp_buf);
					temp_buf = g.GetToken_Ansi(PPHSC_RU_CURRENCYNAME_RUB);
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CURRENCYNAME), temp_buf);
				}
				// @v11.1.7 {
				{
					//<ДокПодтвОтгр НаимДокОтгр="Накладная" НомДокОтгр="21-00491132391" ДатаДокОтгр="08.07.2021"/>
					SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOC));
					temp_buf = g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAM_BILL);
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNAME), temp_buf);
					// @v11.1.12 BillCore::GetCode(temp_buf = rBp.Rec.Code);
					temp_buf = rBp.Rec.Code; // @v11.1.12 
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCNO), g.EncText(temp_buf));
					n.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONFSHIPMDOCDATE), temp_buf.Z().Cat(rBp.Rec.Dt, DATF_GERMANCENT));
				}
				// } @v11.1.7 
				{
					SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_EXTRA1));
					if(agt_code.NotEmpty()) {
						{
							SXml::WNode n_1(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
							temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACT);
							n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
							n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), g.EncText(agt_code));
						}
						if(checkdate(agt_date)) {
							SXml::WNode n_2(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
							temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACTDATE);
							n_2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
							n_2.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), temp_buf.Z().Cat(agt_date, DATF_GERMANCENT));
						}
						if(checkdate(agt_expiry)) {
							SXml::WNode n_3(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF)); // [0..20]
							temp_buf = g.GetToken_Ansi(PPHSC_RU_PERIOD);
							n_3.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), temp_buf);
							n_3.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), temp_buf.Z().Cat(agt_expiry, DATF_GERMANCENT));
						}
					}
					else {
						SXml::WNode n_1(g.P_X, g.GetToken_Ansi(PPHSC_RU_TEXTINF));
						n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_IDENTIF), "none");
						n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_VAL), "none");
					}
				}
			}
			g.WriteInvoiceItems(_hi, rBp);
			{
				SXml::WNode n(g.P_X, g.GetToken_Ansi(PPHSC_RU_WARETRANSFINFO));
				{
					SXml::WNode n_1(g.P_X, g.GetToken_Ansi(PPHSC_RU_WARETRANSFINFO2));
					PPLoadText(PPTXT_NALOGRU_UPDOPCONTENT, temp_buf);
					n_1.PutAttrib(g.GetToken_Ansi(PPHSC_RU_CONTOFOP), g.EncText(temp_buf));
					{
						SXml::WNode n_11(g.P_X, g.GetToken_Ansi(PPHSC_RU_BASISFORWARETRANSFER));
						if(agt_code.NotEmpty()) {
							temp_buf = g.GetToken_Ansi(PPHSC_RU_CONTRACT);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFBASISFORWARETRANSFER), temp_buf);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NUMBOFBASISFORWARETRANSFER), g.EncText(agt_code));
							temp_buf.Z().Cat(checkdate(agt_date) ? agt_date : encodedate(1, 1, 2017), DATF_GERMANCENT);
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_DATEOFBASISFORWARETRANSFER), g.EncText(temp_buf));
						}
						else {
							// @v10.6.12 temp_buf = g.GetToken_Ansi(PPHSC_RU_ABSENCE);
							temp_buf = g.GetToken_Ansi(PPHSC_RU_NODOCOFBASISFORWARETRANSFER); // @v10.6.12
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_NAMEOFBASISFORWARETRANSFER), temp_buf);
						}
						// @v11.0.2 {
						/* @v11.1.12 
						if(!isempty(rBp.Rec.Memo))
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_ADDENDUMOFBASISFORWARETRANSFER), g.EncText(temp_buf.Z().Cat(rBp.Rec.Memo)));
						*/
						// @v11.1.12 {
						if(rBp.SMemo.NotEmpty())
							n_11.PutAttrib(g.GetToken_Ansi(PPHSC_RU_ADDENDUMOFBASISFORWARETRANSFER), g.EncText(temp_buf = rBp.SMemo));
						// } @v11.1.12 
						// } @v11.0.2
					}
					{
						//SXml::WNode n_12(g.P_X, "СвЛицПер");
					}
					{
						//SXml::WNode n_13(g.P_X, "ТранГруз");
					}
					{
						//SXml::WNode n_14(g.P_X, "СвПерВещи");
					}
				}
				/*{
					SXml::WNode n_2(g.P_X, "ИнфПолФХЖ3");
					{
						SXml::WNode n_21(g.P_X, "ТекстИнф");
					}
				}*/
			}
			g.Underwriter(0);
		}
		g.EndDocument();
		rResultFileName = _hi.FileName; // @v10.9.8
	}
	CATCHZOK
	return ok;
}

int WriteBill_ExportMarks(const PPBillPacket & rBp, const SString & rFileName, SString & rResultFileName)
{
	int    ok = 1;
	rResultFileName.Z(); // @v10.9.8
	SString file_name;
	SString temp_buf;
	SString output;
	SString line_name;
	SString name_buf;
	LongArray idx_list;
	PPObjBill::MakeCodeString(&rBp.Rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName|PPObjBill::mcsAddObjName,temp_buf);
	output.Cat(temp_buf.Transf(CTRANSF_INNER_TO_OUTER)).CR();
	uint item_count = rBp.GetTCount();
	for(uint tidx = 0; tidx < item_count; tidx++) {
		const PPTransferItem & r_ti = rBp.ConstTI(tidx);
		GetGoodsName(r_ti.GoodsID, name_buf);
		output.Tab(1).CatChar('[').Cat(r_ti.RByBill).CatChar(']').Space().Cat(name_buf.Transf(CTRANSF_INNER_TO_OUTER)).CatDiv(',', 2).Cat("Qtty").CatDiv(':', 2).Cat(r_ti.Qtty()).CR();
		output.Tab(2).Cat("FSRARINFA").CatDiv(':', 2);
		if(rBp.LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARINFA, temp_buf.Z()) <= 0)
			temp_buf = "none";
		output.Cat(temp_buf).Semicol();
		output.Tab().Cat("FSRARINFB").CatDiv(':', 2);
		if(rBp.LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARINFB, temp_buf.Z()) <= 0)
			temp_buf = "none";
		output.Cat(temp_buf).Semicol();
		output.Tab().Cat("FSRARLOTGOODSCODE").CatDiv(':', 2);
		if(rBp.LTagL.GetTagStr(tidx, PPTAG_LOT_FSRARLOTGOODSCODE, temp_buf.Z()) <= 0)
			temp_buf = "none";
		output.Cat(temp_buf).CR();
		//
		PPLotExtCodeContainer marks = rBp.XcL;
		StringSet ss;
		PPLotExtCodeContainer::MarkSet ms;
		PPLotExtCodeContainer::MarkSet::Entry msentry;
		idx_list.clear();
		marks.Get(tidx, &idx_list, ms);
		for(uint boxidx = 0; boxidx < ms.GetCount(); boxidx++) {
			if(ms.GetByIdx(boxidx, msentry)) {
				if(msentry.Flags & PPLotExtCodeContainer::fBox) {
					ms.GetByBoxID(msentry.BoxID, ss);
					for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
						output.Tab(2).Cat(temp_buf).CR();
					}
				}
			}
		}
		ms.GetByBoxID(0, ss);
		for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
			output.Tab(2).Cat(temp_buf).CR();
		}
	}
	{
		SFile f_out(rFileName, fileExists(rFileName) ? SFile::mAppend : SFile::mWrite);
		if(f_out.IsValid())
			f_out.WriteLine(output);
	}
	rResultFileName = rFileName; // @v10.9.8
	return ok;
}
