#ifndef __CPT720_H
#define __CPT720_H

#include <objbase.h>
#include <initguid.h>

//
// GUID defines
//

// {44CADB85-9E2A-4963-B33B-22E56A82F504}
DEFINE_GUID(CLSID_CPT720, 
0x44cadb85, 0x9e2a, 0x4963, 0xb3, 0x3b, 0x22, 0xe5, 0x6a, 0x82, 0xf5, 0x4);
// {6EBE4010-C22A-455a-BE04-D5C0E8382895}
DEFINE_GUID(IID_CPT720Interface, 
0x6ebe4010, 0xc22a, 0x455a, 0xbe, 0x4, 0xd5, 0xc0, 0xe8, 0x38, 0x28, 0x95);

// Percent function
typedef void (*CallbackPrctFunc)(long , long, const char *, size_t);

//
// Error codes defines
//
#define CPT720_ERR_INITCOMPORT    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1000)
#define CPT720_ERR_NOHANDSHAKEACK MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1001)
#define CPT720_ERR_CLOSELINKFAULT MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1002)
#define CPT720_ERR_NOTSOHSTXSYMB  MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1003)
#define CPT720_ERR_NOREPLY        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1004)
#define CPT720_ERR_NAK            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1005)
#define CPT720_ERR_EOT            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1006)
#define CPT720_ERR_CANTOPENFILE   MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1007)
#define CPT720_ERR_DBFWRFAULT     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1008)
#define CPT720_ERR_ERRCRTTBL      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1009)
#define CPT720_ERR_DBFOPENFLT     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 1010)

//
// CPT720Interface
//
interface CPT720Interface : IUnknown {
public:
	// cbr - должно быть равно 9 (значит 19200, скорость обмена данными с терминалом)
	virtual HRESULT __stdcall SetComPortParams(__int16 cbr, __int8 ByteSize, __int8 Parity, __int8 StopBits) = 0;
	virtual HRESULT __stdcall SetComPortTimeouts(unsigned __int16 getNumTries, unsigned __int16 getDelay, 
					unsigned __int16 putNumTries, unsigned __int16 putDelay, unsigned __int16 wGetDelay)     = 0;
	virtual HRESULT __stdcall SetTransmitParams(unsigned __int16 timeout, unsigned __int16 maxTries)         = 0;
	// comPort - номер com-порта, 0 - первый, 1 - второй и т. д.
	virtual HRESULT __stdcall SendGoodsData(int comPort, const char * pPath, CallbackPrctFunc)               = 0;
	virtual HRESULT __stdcall SendSupplData(int comPort, const char * pPath, CallbackPrctFunc)               = 0;
	virtual HRESULT __stdcall ReceiveFiles(int comPort, const char * pBillPath, const char * pBLinePath, 
					const char * pInvPath, const char * pILinePath)                                           = 0;
};

#endif