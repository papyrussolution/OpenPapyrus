// SLIB-PHONENUMBER.CPP
// Copyright (c) A.Sobolev 2022
// @codepage UTF-8
// Интерфейсный модуль для доступа к libphonenumber из SLIB и Papyrus
//
#include <libphonenumber-internal.h>
#pragma hdrstop

using namespace i18n::phonenumbers;

SLibPhoneNumber::SLibPhoneNumber() : H(0), P(0)
{
}

SLibPhoneNumber::~SLibPhoneNumber()
{
	delete static_cast<PhoneNumber *>(H);
	H = 0;
	// P - singlton. Разрушать его нельзя!
	//delete static_cast<PhoneNumberUtil *>(P);
	//P = 0;
}

int SLibPhoneNumber::Parse(const char * pText, const char * pDefaultRegion)
{	
	int    ok = 1;
	int    sl_err_code = 0;
	PhoneNumberUtil::ErrorType et = PhoneNumberUtil::NO_PARSING_ERROR;
	THROW(SETIFZ(H, new PhoneNumber()));
	THROW(SETIFZ(P, PhoneNumberUtil::GetInstance()));
	et = static_cast<PhoneNumberUtil *>(P)->Parse(pText, NZOR(pDefaultRegion, ""), static_cast<PhoneNumber *>(H));
	if(et != PhoneNumberUtil::NO_PARSING_ERROR) {
		switch(et) {
			case PhoneNumberUtil::INVALID_COUNTRY_CODE_ERROR: sl_err_code = SLERR_PHONENMB_INV_COUNTRY_CODE; break;
			case PhoneNumberUtil::NOT_A_NUMBER: sl_err_code = SLERR_PHONENMB_NOT_A_NUMBER; break;
			case PhoneNumberUtil::TOO_SHORT_AFTER_IDD: sl_err_code = SLERR_PHONENMB_TOO_SHORT_AFTER_IDD; break;
			case PhoneNumberUtil::TOO_SHORT_NSN: sl_err_code = SLERR_PHONENMB_TOO_SHORT_NSN; break;
			case PhoneNumberUtil::TOO_LONG_NSN: sl_err_code = SLERR_PHONENMB_TOO_LONG_NSN; break;
		}
		ok = 0;
	}
	CATCHZOK
	return ok;
}

