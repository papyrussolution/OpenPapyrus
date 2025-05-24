// PP-IFM.H
// Copyright (c) A.Sobolev 2025
// Papyrus Infrastructura-Free-Module
//
#ifndef __PP_IFM_H
#define __PP_IFM_H

#include <slib.h>
//
// Descr: Блок специфической информации для формирования чека коррекции
//
struct CcFiscalCorrection {
	CcFiscalCorrection();
	enum {
		fIncome    = 0x0001, // Приход денег (отрицательная коррекция). Если не стоит, то - расход.
		fByPrecept = 0x0002, // Коррекция по предписанию
		fVatFree   = 0x0004  // Продавец освобожден от НДС
	};
	double AmtCash;    // @#{>=0} Сумма наличного платежа
	double AmtBank;    // @#{>=0} Сумма электронного платежа
	double AmtPrepay;  // @#{>=0} Сумма предоплатой
	double AmtPostpay; // @#{>=0} Сумма постоплатой
	double AmtReckon;  // Сумма встречным представлением
	double AmtVat20;   // Сумма налога по ставке 20%
	double AmtVat18;   // Сумма налога по ставке 18%
	double AmtVat10;   // Сумма налога по ставке 10%
	double AmtVat07;   // @v12.2.5 Сумма налога по ставке 7%
	double AmtVat05;   // @v12.2.5 Сумма налога по ставке 5%
	double AmtVat00;   // Сумма расчета по ставке 0%
	double AmtNoVat;   // Сумма расчета без налога
	double VatRate;    // Единственная ставка НДС. Если VatRate != 0, тогда AmtVat18, AmtVat10, AmtVat00 и AmtNoVat игнорируются
	LDATE  Dt;         // Дата документа основания коррекции
	long   Flags;      // @flags
	SString Code;      // Номер документа основания коррекции
	SString Reason;    // Основание коррекции
	SString Operator;  // Имя оператора
	SString FiscalSign; // @v12.3.3 Фискальный признак чека
	SString DocMemo;    // @v12.3.4
};

#endif // __PP_IFM_H