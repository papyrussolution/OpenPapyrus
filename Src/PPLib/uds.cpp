// UDS.CPP
// Copyright (c) A.Sobolev 2020
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

#if 0 // moved to scspctrt.cpp {

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
		Customer() : Gender(GENDER_UNDEF), DOB(ZERODATE)
		{
		}
		int    Gender; // GENDER_XXX
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
		SString Code;  // Payment code
		SString Phone; // Phone number in E164 format, for example, +79876543210.
	};
	UdsGameInterface();
	~UdsGameInterface();
	int    Setup(PPID guaID);
	//
	// Descr: Возвращает !0 если последний метод обращения к серверу завершился ошибкой.
	//   По ссылке rErr возвращает состояние последней ошибки.
	//
	int    IsError(Error & rErr) const
	{
		rErr = LastErr;
		return BIN(LastErr.Code);
	}
	int    GetSettings(Settings & rResult);        // GET https://api.uds.app/partner/v2/settings
	int    GetTransactionList(); // GET  https://api.uds.app/partner/v2/operations
	int    GetTransactionInformation(); // GET  https://api.uds.app/partner/v2/operations
	int    GetTransactionInformation2(); // POST https://api.uds.app/partner/v2/operations/calc
	int    CreateTransaction(const Transaction & rT, Transaction & pReplyT);  // POST https://api.uds.app/partner/v2/operations
	int    RefundTransaction();  // POST https://api.uds.app/partner/v2/operations/<id>/refund
	int    RewardingUsersWithPoints(); // POST https://api.uds.app/partner/v2/operations/reward
	int    GetCustomerList(TSCollection <Customer> & rResult); // GET https://api.uds.app/partner/v2/customers
	int    FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase);  // GET https://api.uds.app/partner/v2/customers/find
	int    GetCustomerInformation(int64 id, Customer & rC); // GET https://api.uds.app/partner/v2/customers/<id>
	int    CreatePriceItem();  // POST https://api.uds.app/partner/v2/goods
	int    UpdatePriceItem();  // PUT https://api.uds.app/partner/v2/goods/<id>
	int    DeletePriceItem();  // DELETE -s https://api.uds.app/partner/v2/goods/<id>
	int    GetPriceItemList(); // GET -s https://api.uds.app/partner/v2/goods
	int    GetPriceItemInformation(); // GET -s https://api.uds.app/partner/v2/goods/<id>
private:
	void   PrepareHtmlFields(StrStrAssocArray & rHdrFlds);
	int    ReadError(const SJson * pJs, Error & rErr) const;
	int    ReadMembershipTier(const SJson * pJs, MembershipTier & rT) const;
	int    ReadCustomer(const SJson * pJs, Customer & rC) const;
	int    ReadParticipant(const SJson * pJs, Participant & rP) const;
	int    ReadPurchase(const SJson * pJs, Purchase & rP) const;
	InitBlock Ib;
	Error LastErr;
};

UdsGameInterface::UdsGameInterface()
{
}

UdsGameInterface::~UdsGameInterface()
{
}

int UdsGameInterface::Setup(PPID guaID)
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

void UdsGameInterface::PrepareHtmlFields(StrStrAssocArray & rHdrFlds)
{
	SString temp_buf;
	SFileFormat::GetMime(SFileFormat::Json, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAccept, temp_buf);
	SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAcceptCharset, "utf-8");
	{
		SString login;
		login.Z().Cat(Ib.CliIdent).Colon().Cat(Ib.CliAccsKey).Transf(CTRANSF_INNER_TO_UTF8);
		temp_buf.Z().EncodeMime64(login.cptr(), login.Len());
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrAuthorization, login.Z().Cat("Basic").Space().Cat(temp_buf));
	}
	{
		S_GUID(SCtrGenerate_).ToStr(S_GUID::fmtIDL, temp_buf);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXOriginRequestId, temp_buf);
	}
	{
		LDATETIME now_dtm = getcurdatetime_();
		temp_buf.Z().Cat(now_dtm.d, DATF_ISO8601|DATF_CENTURY).CatChar('T').Cat(now_dtm.t, TIMF_HMS|TIMF_MSEC|TIMF_TIMEZONE);
		SHttpProtocol::SetHeaderField(rHdrFlds, SHttpProtocol::hdrXTimestamp, temp_buf);
	}
}

int UdsGameInterface::ReadError(const SJson * pJs, Error & rErr) const
{
	int    ok = -1;
	const  SJson * p_cur = pJs;
	if(SJson::IsObject(p_cur)) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("errorCode")) {
				rErr.ErrCode = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
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
				rErr.Message = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadMembershipTier(const SJson * pJs, MembershipTier & rT) const
{
	int    ok = 1;
	const  SJson * p_cur = pJs;
	if(p_cur->Type == SJson::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(SJson::IsString(p_cur->P_Child))
					rT.Uid.FromStr(p_cur->P_Child->Text);
				else
					rT.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("name")) {
				rT.Name = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("rate")) {
				rT.Rate = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("conditions")) {
				if(p_cur->P_Child && p_cur->P_Child->Type == SJson::tOBJECT) {
					for(const SJson * p_c = p_cur->P_Child->P_Child; p_c; p_c = p_c->P_Next) {
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

int UdsGameInterface::GetSettings(Settings & rResult)
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	SJson * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("settings"));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
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
					SJson * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(SJson::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							ok = 1;
							if(p_cur->Text.IsEqiAscii("id")) {
								rResult.ID = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
							}
							else if(p_cur->Text.IsEqiAscii("name")) {
								rResult.Name = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("promoCode")) {
								rResult.PromoCode = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("baseDiscountPolicy")) {
								rResult.BaseDiscountPolicy = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("purchaseByPhone")) {
								SETFLAG(rResult.Flags, rResult.fPurchaseByPhone, SJson::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("writeInvoice")) {
								SETFLAG(rResult.Flags, rResult.fWriteInvoice, SJson::IsTrue(p_cur->P_Child));
							}
							else if(p_cur->Text.IsEqiAscii("currency")) {
								rResult.Currency = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("slug")) {
								rResult.Slug = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
							}
							else if(p_cur->Text.IsEqiAscii("loyaltyProgramSettings")) {
								if(p_cur->P_Child && p_cur->P_Child->Type == SJson::tOBJECT) {
									for(SJson * p_lps_item = p_cur->P_Child->P_Child; p_lps_item; p_lps_item = p_lps_item->P_Next) {
										if(p_lps_item->Text.IsEqiAscii("membershipTiers")) {
											if(p_lps_item->P_Child && p_lps_item->P_Child->Type == SJson::tARRAY) {
												for(SJson * p_mt_item = p_lps_item->P_Child->P_Child; p_mt_item; p_mt_item = p_mt_item->P_Next) {
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
											rResult.DeferPointsForDays = SJson::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralCashbackRates")) {
											; // @todo
										}
										else if(p_lps_item->Text.IsEqiAscii("receiptLimit"))
											rResult.ReceiptLimit = SJson::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("cashierAward"))
											rResult.CashierAward = SJson::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("referralReward"))
											rResult.ReferralReward = SJson::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
										else if(p_lps_item->Text.IsEqiAscii("maxScoresDiscount"))
											rResult.MaxScoresDiscount = SJson::IsNumber(p_lps_item->P_Child) ? p_lps_item->P_Child->Text.ToReal() : 0.0;
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

int UdsGameInterface::ReadCustomer(const SJson * pJs, Customer & rC) const
{
	int    ok = 1;
	const  SJson * p_cur = pJs;
	if(p_cur->Type == SJson::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("uid")) {
				if(SJson::IsString(p_cur->P_Child))
					rC.Uid.FromStr(p_cur->P_Child->Text);
				else
					rC.Uid.Z();
			}
			else if(p_cur->Text.IsEqiAscii("phone")) {
				rC.Phone = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("gender")) {
				if(SJson::IsString(p_cur->P_Child)) {
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
				if(SJson::IsString(p_cur->P_Child))
					rC.DOB = strtodate_(p_cur->P_Child->Text, DATF_ISO8601);
				else
					rC.DOB = ZERODATE;
			}
			else if(p_cur->Text.IsEqiAscii("avatar")) {
				rC.Avatar = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("displayName")) {
				rC.DisplayName = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
			}
			else if(p_cur->Text.IsEqiAscii("participant")) {
				ReadParticipant(p_cur->P_Child, rC.P);
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadParticipant(const SJson * pJs, Participant & rP) const
{
	int    ok = 1;
	const  SJson * p_cur = pJs;
	if(p_cur->Type == SJson::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("id")) {
				rP.ID = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("cashbackRate")) {
				rP.CashbackRate = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("dateCreated")) {
				if(SJson::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmCreated, DATF_ISO8601, 0);
				else
					rP.DtmCreated.Z();
			}
			else if(p_cur->Text.IsEqiAscii("discountRate")) {
				rP.DiscountRate = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("inviterId")) {
				rP.InviterID = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToInt64() : 0;
			}
			else if(p_cur->Text.IsEqiAscii("lastTransactionTime")) {
				if(SJson::IsString(p_cur->P_Child))
					strtodatetime(p_cur->P_Child->Text, &rP.DtmLastTransaction, DATF_ISO8601, 0);
				else
					rP.DtmLastTransaction.Z();
			}
			else if(p_cur->Text.IsEqiAscii("points")) {
				rP.PointCount = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("membershipTier")) {
				ReadMembershipTier(p_cur->P_Child, rP.Mt);
			}
		}
	}
	return ok;
}

int UdsGameInterface::ReadPurchase(const SJson * pJs, Purchase & rP) const
{
	int    ok = 1;
	const  SJson * p_cur = pJs;
	if(p_cur->Type == SJson::tOBJECT) {
		for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
			if(p_cur->Text.IsEqiAscii("maxPoints")) {
				rP.MaxPoints = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("total")) {
				rP.Total = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("skipLoyaltyTotal")) {
				rP.SkipLoyaltyTotal = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("discountAmount")) {
				rP.DiscountAmount = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("discountPercent")) {
				rP.DiscountPercent = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("points")) {
				rP.Points = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("pointsPercent")) {
				rP.PointsPercent = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("netDiscount")) {
				rP.NetDiscount = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("netDiscountPercent")) {
				rP.NetDiscountPercent = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("cash")) {
				rP.Cash = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
			else if(p_cur->Text.IsEqiAscii("cashBack")) {
				rP.CashBack = SJson::IsNumber(p_cur->P_Child) ? p_cur->P_Child->Text.ToReal() : 0.0;
			}
		}
	}
	return ok;
}

int UdsGameInterface::GetCustomerInformation(int64 id, Customer & rC) // GET https://api.uds.app/partner/v2/customers/<id>
{
	LastErr.Z();
	int    ok = 1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	SJson * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers").SetLastDSlash().Cat(id));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
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

int UdsGameInterface::GetCustomerList(TSCollection <Customer> & rResult) // GET https://api.uds.app/partner/v2/customers
{
	LastErr.Z();
	int    ok = -1;
	SString temp_buf;
	SString log_buf;
	SString url_buf;
	SString json_buf;
	SJson * p_js_doc = 0;
	ScURL c;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	PrepareHtmlFields(hdr_flds);
	{
		InetUrl url((url_buf = Ib.EndPoint).SetLastDSlash().Cat("customers"));
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
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
					SJson * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(SJson::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("rows")) {
								ok = 1;
								if(p_cur->P_Child && p_cur->P_Child->Type == SJson::tARRAY) {
									for(SJson * p_item = p_cur->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
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

int UdsGameInterface::FindCustomer(const FindCustomerParam & rP, Customer & rC, SString & rCode, Purchase & rPurchase)  // GET https://api.uds.app/partner/v2/customers/find
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
	SJson * p_js_doc = 0;
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
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
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
					SJson * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(SJson::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("user")) {
								ReadCustomer(p_cur->P_Child, rC);
							}
							else if(p_cur->Text.IsEqiAscii("code")) {
								rCode = SJson::IsString(p_cur->P_Child) ? p_cur->P_Child->Text : "";
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

int UdsGameInterface::CreateTransaction(const Transaction & rT, Transaction & rReplyT)  // POST https://api.uds.app/partner/v2/operations
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
	SJson * p_js_doc = 0;
	SJson * p_json_req = 0;
	ScURL c;
	S_GUID tra_guid;
	StrStrAssocArray hdr_flds;
	SBuffer ack_buf;
	SFile wr_stream(ack_buf, SFile::mWrite);
	{
		SFileFormat::GetMime(SFileFormat::Json, temp_buf);
		SHttpProtocol::SetHeaderField(hdr_flds, SHttpProtocol::hdrContentType, temp_buf);
		PrepareHtmlFields(hdr_flds);
	}
	{
		p_json_req = new SJson(SJson::tOBJECT);
		if(rT.Code.NotEmpty())
			p_json_req->InsertString("code", rT.Code);
		if(rT.Cust.Phone.NotEmpty() || !!rT.Cust.Uid) {
			SJson * p_js_participant = new SJson(SJson::tOBJECT);
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
			SJson * p_js_cashier = new SJson(SJson::tOBJECT);
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
			SJson * p_js_receipt = new SJson(SJson::tOBJECT);
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
		SFile f_out_log(PPGetFilePathS(PPPATH_LOG, PPFILNAM_UDSTALK_LOG, temp_buf), SFile::mAppend);
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
					SJson * p_cur = p_js_doc;
					if(ReadError(p_cur, LastErr) > 0) {
						ok = 0;
					}
					else if(SJson::IsObject(p_cur)) {
						for(p_cur = p_cur->P_Child; p_cur; p_cur = p_cur->P_Next) {
							if(p_cur->Text.IsEqiAscii("oneOf")) {
								ok = 1;
								if(p_cur->P_Child && p_cur->P_Child->Type == SJson::tARRAY) {
									for(SJson * p_item = p_cur->P_Child->P_Child; p_item; p_item = p_item->P_Next) {
										if(p_item->Text.IsEqiAscii("id")) {
											rReplyT.ID = SJson::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToInt64() : 0;
										}
										else if(p_item->Text.IsEqiAscii("dateCreated")) {
										}
										else if(p_item->Text.IsEqiAscii("action")) {
											if(SJson::IsString(p_item->P_Child)) {
												if(p_item->P_Child->Text.IsEqiAscii("PURCHASE")) {
													rReplyT.Action = tactPurchase;
												}
											}
										}
										else if(p_item->Text.IsEqiAscii("state")) {
											if(SJson::IsString(p_item->P_Child)) {
												if(p_item->P_Child->Text.IsEqiAscii("NORMAL"))
													rReplyT.State = tstNormal;
												else if(p_item->P_Child->Text.IsEqiAscii("CANCELED"))
													rReplyT.State = tstCanceled;
												else if(p_item->P_Child->Text.IsEqiAscii("REVERSAL"))
													rReplyT.State = tstReversal;
											}
										}
										else if(p_item->Text.IsEqiAscii("points")) {
											rReplyT.Points = SJson::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("cash")) {
											rReplyT.Cash = SJson::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("total")) {
											rReplyT.Total = SJson::IsNumber(p_item->P_Child) ? p_item->P_Child->Text.ToReal() : 0.0;
										}
										else if(p_item->Text.IsEqiAscii("customer")) {
											ReadCustomer(p_item->P_Child, rReplyT.Cust);
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

int TestUdsInterface()
{
	int    ok = -1;
	SString temp_buf;
	UdsGameInterface ifc;
	if(ifc.Setup(0)) {
		UdsGameInterface::Settings s;
		TSCollection <UdsGameInterface::Customer> cust_list;
		ifc.GetSettings(s);
		PPInputStringDialogParam isdp("Input client code");
		SString cli_code;
		if(InputStringDialog(&isdp, cli_code) > 0) {
			int fcr = 0;
			SString cust_code;
			UdsGameInterface::Customer cust;
			UdsGameInterface::Purchase cust_purch;
			{
				// 1099540994296
				// +79142706592 
				// 4f678ec3-3888-4650-af52-efa30db5699a
				UdsGameInterface::FindCustomerParam fcp;
				fcp.Code = cli_code;
				//fcp.Phone = "+79142706592";
				//fcp.Uid.FromStr("4f678ec3-3888-4650-af52-efa30db5699a");
				fcr = ifc.FindCustomer(fcp, cust, cust_code, cust_purch);
			}
			if(fcr > 0) {
				UdsGameInterface::Transaction t;
				UdsGameInterface::Transaction reply_t;
				t.Code = cli_code;
				t.Cust.Uid = cust.Uid;
				t.BillNumber = "CC-TEST-307";
				t.Cashier.ID = 101;
				t.Cashier.Name = "Nicole";
				t.Total = 500.0;
				t.Cash = 500.0;
				t.Points = 0.0;
				t.SkipLoyaltyTotal = 0.01;
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
#endif // } 0 moved to scspctrt.cpp