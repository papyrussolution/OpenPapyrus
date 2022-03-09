// SARTRE-UED-STARTUP.CPP
//
#include <slib-internal.h>
#pragma hdrstop
//
// public API {
//
typedef uint64_t UED

struct UEDContxt;

UEDContxt * UedContextCreate();
void  UedContextRef(UEDContext *);
void  UedContextDestroy(UEDContext *);

UED UedIsMeta(UEDContxt * pCtx, const UED * pU, const UED * pMeta);
UED UedGetMeta(UEDContxt * pCtx, const UED * pU);
UED UedValidate(UEDContxt * pCtx, const UED * pU);

const UED * UedSetReal(UEDContxt * pCtx, double v);
const UED * UedGetReal(UEDContxt * pCtx, const UED * pU, double * pV);

const UED * UedOp1(UEDContext * pCtx, UED op, const UED * pA1);
const UED * UedOp2(UEDContext * pCtx, UED op, const UED * pA1, const UED * pA2);
//
// } public API
//

//
// private module {
//
class UEDContext {
public:
	UEDContext()
	{
	}
	const UED * Op1(UED op, const UED * pA1);
	const UED * Op2(UED op, const UED * pA1, const UED * pA2);
private:
};

//
// } private module
//
