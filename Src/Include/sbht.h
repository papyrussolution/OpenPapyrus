//
// SBHT.H
//
#ifndef __SBHT_H
#define __SBHT_H


struct _LDATETIME {
	signed long t;
	signed long d;
};

struct SBHTCmdBuf { // @persistent
	enum {
		cmCheckConnection = 1,
		cmGetGoods        = 2,
		cmPutTSessLine    = 3,
		cmLogout          = 4
	};
	signed short  Cmd;
	signed short  Reserve;
	signed long   RetCode;
	unsigned long BufSize;
};

struct SBHTTSessLineRec {
	long   BillID;
	char   PrcCode[9];
	char   ArCode[9];
	signed short  Reserve;
	char   Serial[32];
	_LDATETIME Dtm;
	double Qtty;
};

struct SBHTGoodsRec {
	long ID;
	char Name[32];
};


#endif

