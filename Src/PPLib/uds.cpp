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
		PPID   GuaID;
		PPGlobalUserAccPacket GuaPack;
		SString CliAccsKey;
		SString CliSecret;
		SString CliIdent;
		SString EndPoint; // URL для запросов
		SString Token;
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
		SString Uid;
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
		int    ID;
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
		int   ID;
		int   InviterID;
		double PointCount;
		double DiscountRate;
		double CashbackRate;
		LDATE DOB;
		LDATETIME DtmCreated;
		LDATETIME DtmLastTransaction;
		MembershipTier Mt;
	};
	struct Customer {
		Customer() : Gender(-1)
		{
		}
		int   Gender;
		SString Uid;
		SString Avatar;
		SString DisplayName;
		SString Phone;
		Participant P;
	};
	SLAPI  UdsGameInterface();
	SLAPI ~UdsGameInterface();
	int    SLAPI Setup(PPID guaID);
	int    SLAPI GetSettings(Settings & rResult);        // GET https://api.uds.app/partner/v2/settings
	int    SLAPI GetTransactionList(); // GET  https://api.uds.app/partner/v2/operations
	int    SLAPI GetTransactionInformation(); // GET  https://api.uds.app/partner/v2/operations
	int    SLAPI GetTransactionInformation2(); // POST https://api.uds.app/partner/v2/operations/calc
	int    SLAPI CreateTransaction();  // POST https://api.uds.app/partner/v2/operations
	int    SLAPI RefundTransaction();  // POST https://api.uds.app/partner/v2/operations/<id>/refund
	int    SLAPI RewardingUsersWithPoints(); // POST https://api.uds.app/partner/v2/operations/reward
	int    SLAPI GetCustomerList(TSCollection <Customer> & rResult); // GET https://api.uds.app/partner/v2/customers
	int    SLAPI FindCustomer();    // GET https://api.uds.app/partner/v2/customers/find
	int    SLAPI GetCustomerInformation(); // GET https://api.uds.app/partner/v2/customers/<id>
	int    SLAPI CreatePriceItem();  // POST https://api.uds.app/partner/v2/goods
	int    SLAPI UpdatePriceItem();  // PUT https://api.uds.app/partner/v2/goods/<id>
	int    SLAPI DeletePriceItem();  // DELETE -s https://api.uds.app/partner/v2/goods/<id>
	int    SLAPI GetPriceItemList(); // GET -s https://api.uds.app/partner/v2/goods
	int    SLAPI GetPriceItemInformation(); // GET -s https://api.uds.app/partner/v2/goods/<id>
private:
};

SLAPI UdsGameInterface::UdsGameInterface()
{
}

SLAPI UdsGameInterface::~UdsGameInterface()
{
}
