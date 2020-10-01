// UDS.CPP
// Copyright (c) A.Sobolev 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

class UdsGameInterface {
public:
	struct InitBlock {
		InitBlock() : GuaID(0)
		{
		}
		InitBlock & Z()
		{
			GuaID = 0;
			GuaPack.Z();
			CliIdent.Z();
			CliAccsKey.Z();
			EndPoint.Z();
			return *this;
		}
		PPID   GuaID;
		PPGlobalUserAccPacket GuaPack;
		SString CliIdent;
		SString CliAccsKey;
		SString EndPoint; // URL для запросов
	};
	struct Error {
		Error() : Code(0)
		{
		}
		Error & Z()
		{
			Code = 0;
			ErrCode.Z();
			Message.Z();
			return *this;
		}
		int    Code; // -1 error but numeric code is undefined
		SString ErrCode; // text code
		SString Message; // text message
	};
	struct MembershipTier {
		struct Conditions {
			Conditions() : TotalCashSpent(0.0), EffInvitedCount(0.0)
			{
			}
			double TotalCashSpent;
			double EffInvitedCount;
		};
		MembershipTier() : Rate(0.0)
		{
		}
		S_GUID Uid;
		SString Name;
		double Rate;
		Conditions C;
	};
	struct Settings {
		enum {
			fPurchaseByPhone = 0x0001,
			fWriteInvoice    = 0x0002,
			fBurnPointsOnPurchase  = 0x0004,
			fBurnPointsOnPricelist = 0x0008
		};
		Settings() : ID(0), Flags(0), MaxScoresDiscount(0.0), CashierAward(0.0), ReferralReward(0.0), ReceiptLimit(0.0), DeferPointsForDays(0.0)
		{
		}
		int64  ID;
		uint   Flags;
		double MaxScoresDiscount;
		double CashierAward;
		double ReferralReward;
		double ReceiptLimit;
		double DeferPointsForDays;
		SString Name;
		SString PromoCode;
		SString Currency;
		SString BaseDiscountPolicy;
		SString Slug;
		MembershipTier BaseMt;
		TSCollection <MembershipTier> MtList;
	};
	struct Participant {
		Participant() : ID(0), InviterID(0), PointCount(0.0), DiscountRate(0.0), CashbackRate(0.0), DOB(ZERODATE),
			DtmCreated(ZERODATETIME), DtmLastTransaction(ZERODATETIME)
		{
		}
		int64 ID;
		int64 InviterID;
		double PointCount;
		double DiscountRate;
		double CashbackRate;
		LDATE DOB;
		LDATETIME DtmCreated;
		LDATETIME DtmLastTransaction;
		MembershipTier Mt;
	};
	struct Customer {
		Customer() : Gender(-1), DOB(ZERODATE)
		{
		}
		int    Gender; // 0 - male, 1 - female
		LDATE  DOB;
		S_GUID Uid;
		SString Avatar;
		SString DisplayName;
		SString Phone;
		Participant P;
	};
	struct Purchase {
		double MaxPoints;
		double Total;
		double SkipLoyaltyTotal;
		double DiscountAmount;
		double DiscountPercent;
		double Points;
		double PointsPercent;
		double NetDiscount;
		double NetDiscountPercent;
		double Cash;
		double CashBack;
	};
	struct Ref {
		Ref() : ID(0)
		{
		}
		int64  ID;
		SString Name;
	};

	enum {
		tactUndef = 0,
		tactPurchase = 1,
	};
	enum {
		tstUndef = 0,
		tstNormal,   // NORMAL
		tstCanceled, // CANCELED
		tstReversal  // REVERSAL
	};
	struct Transaction {
		Transaction() : ID(0), Dtm(ZERODATETIME), Action(tactUndef), State(tstUndef), Points(0.0), Cash(0.0), Total(0.0), SkipLoyaltyTotal(0.0)
		{
		}
		int64  ID;
		LDATETIME Dtm;
		int    Action; // tactXXX
		int    State;  // tstXXX
		SString Code;       // При создании транзакции: код, сканируемый с телефона 
		SString BillNumber; // При создании транзакции: номер чека или документа
		Customer Cust;
		Ref    Cashier;
		Ref    Branch;
		double Points;
		double Cash;
		double Total;
		double SkipLoyaltyTotal; // При создании транзакции: A part of the bill amount for which cashback is not credited and to which the discount does not apply (in currency units).
	};
	struct FindCustomerParam {
		FindCustomerParam() : Total(0.0), SkipLoyaltyTotal(0.0), Flags(0)
		{
		}
		enum {
			fExchangeCode = 0x0001 // Exchange existing payment promo code (if present in query) to a new long-term one. Old promo code will be deactivated.
		};
		S_GUID Uid;    // Customer UID
		double Total;  // Total bill amount (in currency units).
		double SkipLoyaltyTotal; // A part of the bill amount for which cashback is not credited and to which the discount does not apply (in currency units).
		long   Flags;
		SString Code; // Payment code
		SString Phone; // Phone number in E164 format, for example, +79876543210.
	};
	SLAPI  UdsGameInterface();
	SLAPI ~UdsGameInterface();
	int    SLAPI Setup(PPID guaID);
	//
	// Descr: Возвращает !0 если последний метод обращения к серверу завершился ошибкой.
	//   По ссылке rErr возвращает состояние последней ошибки.
	//
	int    SLAPI IsError(Error & rErr) const
	{
		rErr = LastErr;
		return BIN(LastErr.Code);
	}
	int    SLAPI GetSettings(Settings & rResult);        // GET https://api.uds.app/partner/v2/settings
	int    SLAPI GetTransactionList(); // GET  https://api.uds.app/partner/v2/operations
	int    SLAPI GetTransactionInformation(); // GET  https://api.uds.app/partner/v2/operations
	int    SLAPI GetTransactionInformation2(); // POST https://api.uds.app/partner/v2/operations/calc
	int    SLAPI CreateTransaction(const Transaction & rT, Transaction & pReplyT);  // POST https://api.uds.app/partner/v2/operations
	int    SLAPI RefundTransaction();  // POST https://api.uds.app/partner/v2/operations/<id>/refund
	int    SLAPI RewardingUsersWithPoints(); // POST https://api.uds.app/partner/v2/operations/reward
	int    SLAPI GetCustomerList(TSCollection <Customer> & rResult); // GET https://api.uds.app/partner/v2/customers
	int    SLAPI FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase);  // GET https://api.uds.app/partner/v2/customers/find
	int    SLAPI GetCustomerInformation(int64 id, Customer & rC); // GET https://api.uds.app/partner/v2/customers/<id>
	int    SLAPI CreatePriceItem();  // POST https://api.uds.app/partner/v2/goods
	int    SLAPI UpdatePriceItem();  // PUT https://api.uds.app/partner/v2/goods/<id>
	int    SLAPI DeletePriceItem();  // DELETE -s https://api.uds.app/partner/v2/goods/<id>
	int    SLAPI GetPriceItemList(); // GET -s https://api.uds.app/partner/v2/goods
	int    SLAPI GetPriceItemInformation(); // GET -s https://api.uds.app/partner/v2/goods/<id>
private:
	void   SLAPI PrepareHtmlFields(StrStrAssocArray & rHdrFlds);
	int    SLAPI ReadError(const json_t * pJs, Error & rErr) const;
	int    SLAPI ReadMembershipTier(const json_t * pJs, MembershipTier & rT) const;
	int    SLAPI ReadCustomer(const json_t * pJs, Customer & rC) const;
	int    SLAPI ReadParticipant(const json_t * pJs, Participant & rP) const;
	int    SLAPI ReadPurchase(const json_t * pJs, Purchase & rP) const;
	InitBlock Ib;
	Error LastErr;
};

SLAPI UdsGameInterface::UdsGameInterface()
{
}

SLAPI UdsGameInterface::~UdsGameInterface()
{
}

int SLAPI UdsGameInterface::Setup(PPID guaID)
{
	int    ok = 1;
	PPObjGlobalUserAcc gua_obj;
	if(!guaID) {
		PPID single_gua_id = 0;
		PPGlobalUserAcc gua_rec;
		for(SEnum en = gua_obj.Enum(0); en.Next(&gua_rec) > 0;) {
			if(gua_rec.ServiceIdent == PPGLS_UDS) {
				if(!single_gua_id)
					single_gua_id = gua_rec.ID;
				else {
					single_gua_id = 0;
					break;
				}
			}
		}
		if(single_gua_id)
			guaID = single_gua_id;
	}
	THROW(guaID);
	Ib.GuaID = guaID;
	THROW(gua_obj.GetPacket(guaID, &Ib.GuaPack) > 0);
	THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_LOGIN, Ib.CliIdent) > 0);
	THROW(Ib.GuaPack.TagL.GetItemStr(PPTAG_GUA_ACCESSKEY, Ib.CliAccsKey) > 0);
	Ib.EndPoint = InetUrl::MkHttps("api.uds.app", "partner/v2");
	CATCHZOK
	return ok;
}

void SLAPI UdsGameInterface::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
{
	SString temp_buf;
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAccept, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAcceptCharset, "utf-8");
	{
		SString login;
		login.Z().Cat(Ib.CliIdent).CatChar(':').Cat(Ib.CliAccsKey).Transf(CTRANSF_INNER_TO_UTF8);
		temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthorization, login.Z().Cat("Basic").Space().Cat(temp_buf));
	}
	{
		S_GUID guid;
		guid.Generate();
		guid.ToStr(S_GUID::fmtIDL, temp_buf);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXOriginRequestId, temp_buf);
	}
	{
		LDATETIME now_dtm = getcurdatetime_();
		temp_buf.Z().Cat(now_dtm.d, DATF_ISO8601|DATF_CENTURY).CatChar('T').Cat(now_dtm.t, TIMF_HMS|TIMF_MSEC|TIMF_TIMEZONE);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXTimestamp, temp_buf);
	}
}

int SLAPI UdsGameInterface::ReadError(const json_t * pJs, Error & rErr) const
{
	int    ok = -1;
	const  json_t * p_cur = pJs;
	if(json_t::IsObject(p_cur)) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("errorCode")) {
				rErr.ErrCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
				if(rErr.ErrCode.IsEqiAscii("notFound")) {
					rErr.Code = 404;
				}
				else if(rErr.ErrCode.IsEqiAscii("badRequest")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("invalidChecksum")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("withdrawNotPermitted")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("insufficientFunds")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("priceListOnly")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("purchaseByPhoneDisabled")) {
					rErr.Code = 400;
				}
				else if(rErr.ErrCode.IsEqiAscii("cashierNotFound")) {
					rErr.Code = 404;
				}
				else if(rErr.ErrCode.IsEqiAscii("receiptExists")) {
					rErr.Code = 409;
				}
				ok = 1;
			}
			else if(p_cur->Text.IsEqiAscii("message")) {
				rErr.Message = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
		}
	}
	return ok;
}

int SLAPI UdsGameInterface::ReadMembershipTier(const json_t * pJs, MembershipTier & rT) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(json_t::IsString(p_cur->P_Child))
					rT.Uid.FromStr(p_cur->P_Child->Text);
				else
					rT.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				rT.Name = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("rate")) {
				rT.Rate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("conditions")) {
				if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tOBJECT) {
					for(const json_t * p_c = p_cur->P_Child->P_Child; p_c; p_c = p_c->P_Next) {
						if(p_c->Text.IsEqiAscii("totalCashSpent")) {
							; // @todo
						}
						else if(p_c->Text.IsEqiAscii("effectiveInvitedCount")) {
							; // @todo
						}
					}
				}
			}
		}
	}
	return ok;
}

int SLAPI UdsGameInterface::GetSettings(Settings & rResult)
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("settings"));
		PPGetFilePath(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf);
		SFile f_out_log(temp_buf, SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							ok = 1;
							if(p_cur->Text.IsEqiAscii("id")) {
								rResult.ID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
							}
							else if(p_cur->Text.IsEqiAscii("name")) {
								rResult.Name = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("promoCode")) {
								rResult.PromoCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("baseDiscountPolicy")) {
								rResult.BaseDiscountPolicy = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("purchaseByPhone")) {
								SETFLAG(rResult.Flags, rResult.fPurchaseByPhone, json_t::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("writeInvoice")) {
								SETFLAG(rResult.Flags, rResult.fWriteInvoice, json_t::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("currency")) {
								rResult.Currency = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("slug")) {
								rResult.Slug = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("loyaltyProgramSettings")) {
								if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tOBJECT) {
									for(json_t * p_lps_item = p_cur->P_Child->P_Child; p_lps_item; p_lps_item = p_lps_item->P_Next) {
										if(p_lps_item->Text.IsEqiAscii("membershipTiers")) {
											if(p_lps_item->P_Child && p_lps_item->P_Child->Type == json_t::tARRAY) {
												for(json_t * p_mt_item = p_lps_item->P_Child->P_Child; p_mt_item; p_mt_item = p_mt_item->P_Next) {
													MembershipTier * p_new_mt = rResult.MtList.CreateNewItem();
													if(p_new_mt) {
														ReadMembershipTier(p_mt_item, *p_new_mt);
													}
												}
											}
										}
										else if(p_lps_item->Text.IsEqiAscii("baseMembershipTier")) {
											ReadMembershipTier(p_lps_item->P_Child, rResult.BaseMt);
										}
										else if(p_lps_item->Text.IsEqiAscii("deferPointsForDays"))
											rResult.DeferPointsForDays = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralCashbackRates")) {
											; // @todo
										}
										else if(p_lps_item->Text.IsEqiAscii("receiptLimit"))
											rResult.ReceiptLimit = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("cashierAward"))
											rResult.CashierAward = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralReward"))
											rResult.ReferralReward = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("maxScoresDiscount"))
											rResult.MaxScoresDiscount = json_t::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
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
	delete p_js_doc;
	return ok;
}

int SLAPI UdsGameInterface::ReadCustomer(const json_t * pJs, Customer & rC) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(json_t::IsString(p_cur->P_Child))
					rC.Uid.FromStr(p_cur->P_Child->Text);
				else
					rC.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("phone")) {
				rC.Phone = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("gender")) {
				if(json_t::IsString(p_cur->P_Child)) {
					if(p_cur->P_Child->Text.IsEqiAscii("male"))
						rC.Gender = 0;
					else if(p_cur->P_Child->Text.IsEqiAscii("female"))
						rC.Gender = 1;
					else
						rC.Gender = -1;
				}
				else
					rC.Gender = -1;
			}
			else if(p_cur->Text.IsEqiAscii("birthDate")) {
				if(json_t::IsString(p_cur->P_Child))
					rC.DOB = strtodate_(p_cur->P_Child->Text, DATF_ISO8601);
				else
					rC.DOB = ZERODATE;
			}
			else if(p_cur->Text.IsEqiAscii("avatar")) {
				rC.Avatar = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("displayName")) {
				rC.DisplayName = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("participant")) {
				ReadParticipant(p_cur->P_Child, rC.P);
			}
		}
	}
	return ok;
}

int SLAPI UdsGameInterface::ReadParticipant(const json_t * pJs, Participant & rP) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id")) {
				rP.ID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("cashbackRate")) {
				rP.CashbackRate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("dateCreated")) {
				if(json_t::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmCreated, DATF_ISO8601, 0);
				else
					rP.DtmCreated.Z();
			}
			else if(p_cur->Text.IsEqiAscii("discountRate")) {
				rP.DiscountRate = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("inviterId")) {
				rP.InviterID = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("lastTransactionTime")) {
				if(json_t::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmLastTransaction, DATF_ISO8601, 0);
				else
					rP.DtmLastTransaction.Z();
			}
			else if(p_cur->Text.IsEqiAscii("points")) {
				rP.PointCount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("membershipTier")) {
				ReadMembershipTier(p_cur->P_Child, rP.Mt);
			}
		}
	}
	return ok;
}

int SLAPI UdsGameInterface::ReadPurchase(const json_t * pJs, Purchase & rP) const
{
	int    ok = 1;
	const  json_t * p_cur = pJs;
	if(p_cur->Type == json_t::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("maxPoints")) {
				rP.MaxPoints = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("total")) {
				rP.Total = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("skipLoyaltyTotal")) {
				rP.SkipLoyaltyTotal = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("discountAmount")) {
				rP.DiscountAmount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("discountPercent")) {
				rP.DiscountPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("points")) {
				rP.Points = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("pointsPercent")) {
				rP.PointsPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("netDiscount")) {
				rP.NetDiscount = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("netDiscountPercent")) {
				rP.NetDiscountPercent = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("cash")) {
				rP.Cash = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("cashBack")) {
				rP.CashBack = json_t::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
		}
	}
	return ok;
}

int SLAPI UdsGameInterface::GetCustomerInformation(int64 id, Customer & rC) // GET https://api.uds.app/partner/v2/customers/<id>
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers").SetLastDSlash().Cat(id));
		PPGetFilePath(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf);
		SFile f_out_log(temp_buf, SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					ReadCustomer(p_js_doc, rC);
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	return ok;
}

int SLAPI UdsGameInterface::GetCustomerList(TSCollection <Customer> & rResult) // GET https://api.uds.app/partner/v2/customers
{
	LastErr.Z();
	int    ok = -1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers"));
		PPGetFilePath(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf);
		SFile f_out_log(temp_buf, SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("rows")) {
								ok = 1;
								if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tARRAY) {
									for(json_t * p_item = p_cur->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										Customer * p_new_cust = rResult.CreateNewItem();
										if(p_new_cust) {
											ReadCustomer(p_item, *p_new_cust);
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
	delete p_js_doc;
	return ok;
}

int SLAPI UdsGameInterface::FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase)  // GET https://api.uds.app/partner/v2/customers/find
{
	LastErr.Z();
	int    ok = 1;
	/*
		curl -H 'Accept: application/json' \
			 -H "X-Origin-Request-Id: $(uuidgen)" \
			 -H "X-Timestamp: $(date --iso-8601=seconds --utc)" \
			 -u "<companyId>:<api_key>"
			 -X GET -s https://api.uds.app/partner/v2/customers/find?code=456123&phone=+71234567898&uid=23684cea-ca50-4c5c-b399-eb473e85b5ad
	*/
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		uint   arg_count = 0;
		(url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers").SetLastDSlash().Cat("find");
		if(rP.Code.NotEmpty())
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("code", rP.Code);
		if(rP.Phone.NotEmpty())
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("phone", rP.Phone);
		if(!rP.Uid.IsZero()) {
			rP.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
			url_buf.CatChar(arg_count ? '&' : '?').CatEq("uid", temp_buf);
		}
		InetUrl url(url_buf);
		PPGetFilePath(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf);
		SFile f_out_log(temp_buf, SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpGet(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("user")) {
								ReadCustomer(p_cur, rC);
							}
							else if(p_cur->Text.IsEqiAscii("code")) {
								rCode = json_t::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("purchase")) {
								ReadPurchase(p_cur->P_Child, rPurchase);
							}
							ok = 1;
						}
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_js_doc;
	return ok;
}

int SLAPI UdsGameInterface::CreateTransaction(const Transaction & rT, Transaction & rReplyT)  // POST https://api.uds.app/partner/v2/operations
{
	/*
	{
		"code": "string",
		"participant": {
			"uid": "string",
			"phone": "string"
		},
		"nonce": "string",
		"cashier": {
			"externalId": "string",
			"name": "string"
		},
		"receipt": {
			"total": 0,
			"cash": 0,
			"points": 0,
			"number": "string",
			"skipLoyaltyTotal": 0
		}
	}
	*/
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	json_t * p_js_doc = 0;
	json_t * p_json_req = 0;
	ScURL c;
	S_GUID tra_guid;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		p_json_req = new json_t(json_t::tOBJECT);
		if(rT.Code.NotEmpty())
			p_json_req->InsertString("code", rT.Code);
		if(rT.Cust.Phone.NotEmpty() || !!rT.Cust.Uid) {
			json_t * p_js_participant = new json_t(json_t::tOBJECT);
			if(!!rT.Cust.Uid) {
				rT.Cust.Uid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
				p_js_participant->InsertString("uid", temp_buf);
			}
			else
				p_js_participant->InsertNull("uid");
			if(rT.Cust.Phone.NotEmpty())
				p_js_participant->InsertString("phone", rT.Cust.Phone);
			else
				p_js_participant->InsertNull("phone");
			p_json_req->Insert("participant", p_js_participant);
		}
		else
			p_json_req->InsertNull("participant");
		tra_guid.Generate();
		tra_guid.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		p_json_req->InsertString("nonce", temp_buf);
		if(rT.Cashier.ID || rT.Cashier.Name.NotEmpty()) {
			json_t * p_js_cashier = new json_t(json_t::tOBJECT);
			if(rT.Cashier.ID) {
				temp_buf.Z().Cat(rT.Cashier.ID);
				p_js_cashier->InsertString("externalId", temp_buf);
			}
			if(rT.Cashier.Name.NotEmpty()) {
				p_js_cashier->InsertString("name", rT.Cashier.Name);
			}
			p_json_req->Insert("cashier", p_js_cashier);
		}
		{
			json_t * p_js_receipt = new json_t(json_t::tOBJECT);
			p_js_receipt->InsertDouble("total", rT.Total, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertDouble("cash", rT.Cash, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertDouble("points", rT.Points, MKSFMTD(0, 2, 0));
			p_js_receipt->InsertString("number", rT.BillNumber);
			p_js_receipt->InsertDouble("skipLoyaltyTotal", rT.SkipLoyaltyTotal, MKSFMTD(0, 2, 0));
			p_json_req->Insert("receipt", p_js_receipt);
		}
		THROW_SL(json_tree_to_string(p_json_req, json_buf));
	}
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("operations"));
		PPGetFilePath(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf);
		SFile f_out_log(temp_buf, SFile::mAppend);
		log_buf.Z().Cat("req").CatDiv(':', 2).Cat(url_buf).Space().Cat(json_buf);
		f_out_log.WriteLine(log_buf.CR());
		THROW_SL(c.HttpPost(url, ScURL::mfDontVerifySslPeer|ScURL::mfVerbose, &hdr_flds, json_buf, &wr_stream));
		{
			SBuffer * p_ack_buf = static_cast<SBuffer *>(wr_stream);
			if(p_ack_buf) {
				json_buf.Z().CatN(p_ack_buf->GetBufC(), p_ack_buf->GetAvailableSize());
				log_buf.Z().Cat("rep").CatDiv(':', 2).Cat(json_buf);
				f_out_log.WriteLine(log_buf.CR().CR());
				if(json_parse_document(&p_js_doc, json_buf) == JSON_OK) {
					json_t * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(json_t::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("oneOf")) {
								ok = 1;
								if(p_cur->P_Child && p_cur->P_Child->Type == json_t::tARRAY) {
									for(json_t * p_item = p_cur->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										if(p_item->Text.IsEqiAscii("id")) {
											rReplyT.ID = json_t::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToInt64() : 0;
										}
										else if(p_item->Text.IsEqiAscii("dateCreated")) {
										}
										else if(p_item->Text.IsEqiAscii("action")) {
											if(json_t::IsString(p_item->P_Child)) {
												if(p_item->P_Child->Text.IsEqiAscii("PURCHASE")) {
													rReplyT.Action = tactPurchase;
												}
											}
										}
										else if(p_item->Text.IsEqiAscii("state")) {
											if(json_t::IsString(p_item->P_Child)) {
												if(p_item->P_Child->Text.IsEqiAscii("NORMAL"))
													rReplyT.State = tstNormal;
												else if(p_item->P_Child->Text.IsEqiAscii("CANCELED"))
													rReplyT.State = tstCanceled;
												else if(p_item->P_Child->Text.IsEqiAscii("REVERSAL"))
													rReplyT.State = tstReversal;
											}
										}
										else if(p_item->Text.IsEqiAscii("points")) {
											rReplyT.Points = json_t::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("cash")) {
											rReplyT.Cash = json_t::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("total")) {
											rReplyT.Total = json_t::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("customer")) {
											ReadCustomer(p_item, rReplyT.Cust);
										}
										else if(p_item->Text.IsEqiAscii("cashier")) {
										}
										else if(p_item->Text.IsEqiAscii("branch")) {
										}
										else if(p_item->Text.IsEqiAscii("origin")) {
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
	delete p_js_doc;
	delete p_json_req;
	return ok;
}

int SLAPI TestUdsInterface()
{
	int    ok = -1;
	UdsGameInterface ifc;
	if(ifc.Setup(0)) {
		UdsGameInterface::Settings s;
		TSCollection <UdsGameInterface::Customer> cust_list;
		ifc.GetSettings(s);
		{
			const char * p_code = "748470";
			int fcr = 0;
			SString cust_code;
			UdsGameInterface::Customer cust;
			UdsGameInterface::Purchase cust_purch;
			{
				// 1099540994296
				// +79142706592 
				// 4f678ec3-3888-4650-af52-efa30db5699a
				UdsGameInterface::FindCustomerParam fcp;
				fcp.Code = p_code;
				//fcp.Phone = "+79142706592";
				//fcp.Uid.FromStr("4f678ec3-3888-4650-af52-efa30db5699a");
				fcr = ifc.FindCustomer(fcp, cust, cust_code, cust_purch);
			}
			if(fcr > 0) {
				UdsGameInterface::Transaction t;
				UdsGameInterface::Transaction reply_t;
				t.Code = p_code;
				t.Cust.Uid = cust.Uid;
				t.BillNumber = "CC-TEST-307";
				t.Cashier.ID = 101;
				t.Cashier.Name = "Nicole";
				t.Total = 500.0;
				t.Cash = 500.0;
				t.Points = 0.0;
				t.SkipLoyaltyTotal = 0.0;
				ifc.CreateTransaction(t, reply_t);
			}
		}
		ifc.GetCustomerList(cust_list);
		{
			UdsGameInterface::Customer cust;
			ifc.GetCustomerInformation(1099541301566, cust);
		}
		ok = 1;
	}
	return ok;
}
