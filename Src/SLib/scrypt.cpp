// SCRYPT.CPP
// Copyright (c) A.Sobolev 2007
//
#include <slib.h>

class SCryptCipher {
public:
	enum Algorithm {
		algSLib = 1,
		algRijndael,
		algAES = algRijndael
	};
	static SCryptCipher * CreateInstance(Algorithm alg, uint level);

protected:
	virtual void Enc(const uint8 * pSrc, uint8 * pDest) const;
	virtual void Dec(const uint8 * pSrc, uint8 * pDest) const;

	int    Alg;
	uint   Level;
};
