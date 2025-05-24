// PP-IFM.H
// Copyright (c) A.Sobolev 2025
// Papyrus Infrastructura-Free-Module
//
#ifndef __PP_IFM_H
#define __PP_IFM_H

#include <slib.h>
//
// Descr: ���� ������������� ���������� ��� ������������ ���� ���������
//
struct CcFiscalCorrection {
	CcFiscalCorrection();
	enum {
		fIncome    = 0x0001, // ������ ����� (������������� ���������). ���� �� �����, �� - ������.
		fByPrecept = 0x0002, // ��������� �� �����������
		fVatFree   = 0x0004  // �������� ���������� �� ���
	};
	double AmtCash;    // @#{>=0} ����� ��������� �������
	double AmtBank;    // @#{>=0} ����� ������������ �������
	double AmtPrepay;  // @#{>=0} ����� �����������
	double AmtPostpay; // @#{>=0} ����� �����������
	double AmtReckon;  // ����� ��������� ��������������
	double AmtVat20;   // ����� ������ �� ������ 20%
	double AmtVat18;   // ����� ������ �� ������ 18%
	double AmtVat10;   // ����� ������ �� ������ 10%
	double AmtVat07;   // @v12.2.5 ����� ������ �� ������ 7%
	double AmtVat05;   // @v12.2.5 ����� ������ �� ������ 5%
	double AmtVat00;   // ����� ������� �� ������ 0%
	double AmtNoVat;   // ����� ������� ��� ������
	double VatRate;    // ������������ ������ ���. ���� VatRate != 0, ����� AmtVat18, AmtVat10, AmtVat00 � AmtNoVat ������������
	LDATE  Dt;         // ���� ��������� ��������� ���������
	long   Flags;      // @flags
	SString Code;      // ����� ��������� ��������� ���������
	SString Reason;    // ��������� ���������
	SString Operator;  // ��� ���������
	SString FiscalSign; // @v12.3.3 ���������� ������� ����
	SString DocMemo;    // @v12.3.4
};

#endif // __PP_IFM_H