// FRONTOL.H
// Copyright (c) A.Sobolev 2016, 2020, 2024
// @codepage UTF-8
//
// Определения, используемые кассовыми модулями, работающими с интерфейсом аналогичным ФРОНТОЛ
//
//   Интерфейс для работы с ATOL_CARD
//
#define AC_MAX_OPER_COUNT   250
//
//   Интерфейс к драйверу "Фронтол"
//
//   Схемы в ATOL для PAPYRUS
#define ATOL_INNER_SCHEME           1
#define ATOL_OUTER_SCHEME        1001
#define AC_BIAS_CARD_CAT_CODE   10000 //   BIAS для кода категории карт в ATOL-CARD
#define AC_DEF_CARD_CODE_LEN       40 //   Длина поля кода карты
//   Тип чека
#define FRONTOL_CHK_SALE            0   // Продажа
#define FRONTOL_CHK_RETURN          1   // Возврат
//   Тип оплаты
#define FRONTOL_PAYMENT_CASH        1   // Наличными
//   Тип операции (транзакции) с чеком
#define FRONTOL_OPTYPE_CHKLINEFREE  1   // Строка чека по свободной цене
#define FRONTOL_OPTYPE_STORNOFREE   2   // Сторно строки чека по свободной цене
#define FRONTOL_OPTYPE_CHKLINE     11   // Строка чека
#define FRONTOL_OPTYPE_STORNO      12   // Сторно строки
#define FRONTOL_OPTYPE_PAYM1       40   // Оплата с вводом суммы клиента
#define FRONTOL_OPTYPE_PAYM2       41   // Оплата без ввода суммы клиента
#define FRONTOL_OPTYPE_CHKCLOSED   55   // Закрытие чека
#define FRONTOL_OPTYPE_CANCEL      56   // Отмена чека
#define FRONTOL_OPTYPE_ZREPORT     61   // Z-отчет
#define FRONTOL_OPTYPE_ZREPORT_2   63   // @v10.8.3 Отчет с гашением

struct _FrontolZRepEntry {
	_FrontolZRepEntry(long posN, long zrepN, long sessID) : PosN(posN), ZRepN(zrepN), SessID(sessID) // @v10.8.2 
	{
	}
	long   PosN;
	long   ZRepN;
	long   SessID;
};

class _FrontolZRepArray : public TSArray <_FrontolZRepEntry> {
public:
	_FrontolZRepArray() : TSArray <_FrontolZRepEntry> ()
	{
	}
	int    Search(long posN, long zRepN, uint * pPos) const
	{
		const _FrontolZRepEntry key(posN, zRepN, 0);
		return lsearch(&key, pPos, PTR_CMPFUNC(_2long));
	}
	int    Search(long sessID, uint * pPos) const
	{
		return lsearch(&sessID, pPos, CMPF_LONG);
	}
};

struct AtolGoodsDiscountEntry {
	AtolGoodsDiscountEntry(PPID goodsID, PPID qkID, double absDiscount) : GoodsID(goodsID), QuotKindID(qkID), AbsDiscount(absDiscount)
	{
	}
	long   GetSchemeID() const
	{
		return (GoodsID + 10000);
	}
	PPID   GoodsID;
	PPID   QuotKindID;
	double AbsDiscount;
};

struct FrontolCcPayment {
	FrontolCcPayment() : Amount(0.0), CashAmt(0.0), BankAmt(0.0)
	{
	}
	FrontolCcPayment & Z()
	{
		Amount = 0.0;
		CashAmt = 0.0;
		BankAmt = 0.0;
		CrdSCardList.clear();
		return *this;
	}
	double Amount;
	double CashAmt;
	double BankAmt;
	RAssocArray CrdSCardList;
};
