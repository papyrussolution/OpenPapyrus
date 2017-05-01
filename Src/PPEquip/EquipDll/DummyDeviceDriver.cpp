// DummyDeviceDriver.cpp
// Copyright (c) A.Sobolev 2013, 2017
//
#include <ppdrvapi.h>

class PPDrvDummy : public PPBaseDriver {
public:
	enum {
		errNone = 0
	};
	PPDrvDummy() : PPBaseDriver()
	{
	}
	virtual int ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
};

static PPDrvSession::TextTableEntry _ErrMsgTab[] = {
	{ PPDrvDummy::errNone, "No Error" }
};

PPDRV_INSTANCE_ERRTAB(Dummy, 1, 0, PPDrvDummy, _ErrMsgTab);

int PPDrvDummy::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	int    ok = 0;
	SString msg_buf;
	DRVS.Log((msg_buf = "ProcessCommand executed").Space().CatQStr(rCmd), 0xffff);
	if(rCmd == "OPEN") {
		LTIME tm = getcurtime_();
		SDelay(2000);
	}
	else if(rCmd == "CLOSE") {
	}
	else if(rCmd == "CONNECT") {
		(msg_buf = "CONNECT").Space().Cat(pInputData);
		DRVS.Log((msg_buf = "CONNECT").Space().Cat(pInputData), 0xffff);
	}
	else if(rCmd == "LISTEN") {
		static const char * cards[] = {
			"3110000009853",
			"3110000009907",
			"3110000009945",
			"8596154902007"
		};
		const  uint rn = SLS.GetTLA().Rg.GetUniformInt(100000);
		const char * p_card_no = (rn % 10) ? 0 : cards[rn%SIZEOFARRAY(cards)];
		rOutput = p_card_no;
	}
	DRVS.Log((msg_buf = "ProcessCommand finished").Space().CatQStr(rCmd), 0xffff);
	return ok;
}

