// SHTRIHMF.CPP
// Copyright (c) A.Starodub 2009, 2010, 2011, 2013, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage windows-1251
// ��������� (�����������) � �������� "�����-�-��-�"
//
#include <pp.h>
#pragma hdrstop
//
//   ��������� � �������� "�����-�-��-�"
//
//   ����� � SHTRIH ��� PAPYRUS
#define SHTRIHMFRK_INNER_SCHEME           1
#define SHTRIHMFRK_OUTER_SCHEME        1001
//   ����� ���� ���� �����
#define AC_DEF_CARD_CODE_LEN       24
// ������ �� ���������� �����
#define DISCOUNT_BYSCARD    3
//   ��� ����
#define CHK_SALE            1   // �������
#define CHK_RETURN          2   // �������
//   ��� ������
#define PAYMENT_CASH        1   // ���������
//   ��� �������� (����������) � �����
#define OPTYPE_CHKLINE        11   // ������ ����
#define OPTYPE_STORNO         12   // ������ ������
#define OPTYPE_RETURN         13   // �������
#define OPTYPE_CHKDISCOUNT    37   // ������ �� ���
#define OPTYPE_CHKEXTRACHARGE 38   // �������� �� ���
#define OPTYPE_PAYM1          40   // ������ � ������ ����� �������
#define OPTYPE_PAYM2          41   // ������ ��� ����� ����� �������
#define OPTYPE_CHKCLOSED      55   // �������� ����
#define OPTYPE_CANCEL         56   // ������ ����
#define OPTYPE_ZREPORT        61   // Z-�����
#define OPTYPE_POSDISDETAIL   70   // ����������� ������ �� �������
#define OPTYPE_CHKDISDETAIL   71   // ����������� ������ �� ���

class ACS_SHTRIHMFRK : public PPAsyncCashSession {
public:
	ACS_SHTRIHMFRK(PPID id);
	virtual int ExportData(int updOnly);
	virtual int GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd = 0);
	virtual int ImportSession(int);
	virtual int FinishImportSession(PPIDArray *);
	virtual int SetGoodsRestLoadFlag(int updOnly);
protected:
	virtual int ExportSCard(FILE * pFile, int updOnly);
	int    GetZRepList(LAssocArray *);
	int    ImportFiles();

	int    UseInnerAutoDscnt;
private:
	int    ConvertWareList(const char * pImpPath, int numSmena);
	DateRange ChkRepPeriod;
	PPIDArray LogNumList;
	SString PathRpt;
	SString PathFlag;
	int    ImpExpTimeout;
	StringSet ImpPaths;
	StringSet ExpPaths;
	SString ImportedFiles;
	LAssocArray ZRepList;
};

class CM_SHTRIHMFRK : public PPCashMachine {
public:
	CM_SHTRIHMFRK(PPID cashID) : PPCashMachine(cashID) {}
	PPAsyncCashSession * AsyncInterface() { return new ACS_SHTRIHMFRK(NodeID); }
};

REGISTER_CMT(SHTRIHMFRK, false, true);

ACS_SHTRIHMFRK::ACS_SHTRIHMFRK(PPID id) : PPAsyncCashSession(id), UseInnerAutoDscnt(0)
{
	PPIniFile ini_file;
	ini_file.GetInt(PPINISECT_CONFIG, PPINIPARAM_SHTRIHMFRK_TIMEOUT, &ImpExpTimeout);
	ChkRepPeriod.Z();
}

int ACS_SHTRIHMFRK::ExportSCard(FILE * pFile, int updOnly)
{
	int   ok = 1, r = 0;
	PPWaitStart();
	long   scheme_id = 0;
	PPID   ser_id = 0;
	const  char   * p_format = "%s\n";
	PPObjSCardSeries scs_obj;
	PPSCardSeries ser_rec;
	PPObjSCard   s_crd_obj;
	PPObjPerson  psn_obj;
	for(ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
		if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
			PPSCardSerPacket scs_pack;
			SCardTbl::Key2 k;
			MEMSZERO(k);
			k.SeriesID = ser_id;
			THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
			while(s_crd_obj.P_Tbl->search(2, &k, spGt) && k.SeriesID == ser_id) {
				int    denied_card = 0;
				SString temp_buf;
				SString buf;
				SString psn_name;
				SCardTbl::Rec card_rec;
				s_crd_obj.P_Tbl->CopyBufTo(&card_rec);
				s_crd_obj.SetInheritance(&scs_pack, &card_rec);
				uint  pos = 0;
				PersonTbl::Rec psn_rec;
				denied_card = (card_rec.PDis && !(card_rec.Flags & SCRDF_CLOSED) && !(card_rec.Expiry && card_rec.Expiry < LConfig.OperDate)) ? 0 : 1;
				if(psn_obj.Search(card_rec.PersonID, &psn_rec) > 0) {
					(psn_name = psn_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
				}
				else
					psn_name = 0;
				scheme_id = card_rec.ID;
				//
				// ������� ���� �������������� ������/��������
				//
				{
					temp_buf.Z().CatChar('!');
					temp_buf.Cat(scheme_id).Semicol();                     // #1 ��� �����
					temp_buf.Cat(card_rec.Code).Cat(psn_name).Semicol();   // #2 �������� �����
					temp_buf.Cat(1L);                                      // #3 ����������� �� �����
					THROW_PP(fprintf(pFile, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
				}
				//
				// ������� �������������� ������/��������
				//
				{
					temp_buf.Z().CatChar('@');
					temp_buf.Cat(scheme_id).Semicol();                         // #1  ��� �����
					temp_buf.Cat(card_rec.ID).Semicol();                       // #2  ���
					temp_buf.Cat(psn_name).Semicol();                          // #3  ������������
					temp_buf.Cat(1L).Cat(psn_name).Semicol();                  // #4  ��� ������
					temp_buf.Cat(1L).Semicol();                                // #5  ��� ������
					temp_buf.Cat(fdiv100i(card_rec.PDis)).Semicol();           // #6  ������ ������
					temp_buf.Cat(psn_name).Semicol();                          // #7  ����� ��� ����
					temp_buf.Cat(card_rec.Dt, DATF_GERMANCENT).Semicol();      // #8  ��������� ����
					temp_buf.Cat(card_rec.Expiry, DATF_GERMANCENT).Semicol();  // #9  �������� ����
					temp_buf.Cat("00:00:00").Semicol();                        // #10 ��������� ����� //
					temp_buf.Cat("24:00:00").Cat(psn_name).Semicol();          // #11 �������� ����� //
					temp_buf.Cat(0L).Semicol();                                // #12 ����� ���������� ��� ������
					temp_buf.Cat(7L).Semicol();                                // #13 ����� ��������� ��� ������
					temp_buf.Cat(0L).Semicol();                                // #14 ��������� ����������
					temp_buf.Cat(0L).Semicol();                                // #15 �������� ����������
					temp_buf.Cat(0L).Cat(psn_name).Semicol();                  // #16 ��������� �����
					temp_buf.Cat(0L);                                          // #17 �������� �����
					THROW_PP(fprintf(pFile, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
				}
				//
				// ������� ���������� ����
				//
				{
					temp_buf.Z().CatChar('%');
					// buf.Z().CatCharN(' ', AC_DEF_CARD_CODE_LEN - sstrlen(card_rec.Code)).Cat(card_rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
					temp_buf.Cat(card_rec.Code).Semicol();            // #1 ����� �����
					temp_buf.Cat(psn_name).Semicol();                 // #2 �������� �����
					temp_buf.Cat(psn_name).Semicol();                 // #3 ����� ��� ����
					temp_buf.Cat(0L/*card_rec.Turnover*/).Semicol();  // #4 ����� ���������� //
					temp_buf.Cat(scheme_id).Semicol();                // #5 ��� ����� �������������� ������
					temp_buf.Cat(denied_card).Semicol();              // #6 ����� ���������
					temp_buf.Cat(1L).Semicol();                       // #7 �� ����� ���������� �� �����
					temp_buf.Semicol();                               // #8 �� ������������ //
					temp_buf.Cat(0L).Semicol();                       // #9 ������������ ��� ���������
					temp_buf.CatCharN(';', 3);                        // #10-#12 �� ������������ //
					temp_buf.Cat(0L).Semicol();                       // #13 ��� ����� ������������� ������
					temp_buf.CatCharN(';', 4);                        // #14-#18 �� ������������ //
					THROW_PP(fprintf(pFile, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
				}
			}
		}
	}
	CATCHZOK
	PPWaitStop();
	return ok;
}

int ACS_SHTRIHMFRK::SetGoodsRestLoadFlag(int updOnly)
{
	int    ok = 1;
	Flags &= ~PPACSF_LOADRESTWOSALES;
	return ok;
}

int ACS_SHTRIHMFRK::ExportData(int updOnly)
{
	int    ok = 1, load_groups = 0;
	const  char * p_format = "%s\n";
	PPID   prev_goods_id = 0, stat_id = 0;
	LAssocArray  grp_n_level_ary;
	SString   f_str, name;
	SString   path_goods;
	PPUnit    unit_rec;
	PPObjUnit unit_obj;
	PPAsyncCashNode               cn_data;
	AsyncCashGoodsInfo            gds_info;
	AsyncCashGoodsIterator * p_gds_iter = 0;
	AsyncCashGoodsGroupInfo       grp_info;
	AsyncCashGoodsGroupIterator * p_grp_iter = 0;
	PPGoodsConfig goods_cfg = GetGoodsCfg();
	PPIDArray quot_list, goods_list;
	FILE * p_file = 0;
	const  int check_dig = BIN(goods_cfg.Flags & GCF_BCCHKDIG);

	THROW(GetNodeData(&cn_data) > 0);
	if(!P_Dls)
		THROW_MEM(P_Dls = new DeviceLoadingStat);
	P_Dls->StartLoading(&stat_id, dvctCashs, NodeID, 1);
	PPWaitStart();
	THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_SHTRIHMFRK_IMP_TXT,  path_goods));
	PPSetAddedMsgString(path_goods);
	THROW_PP(p_file = fopen(path_goods, "w"), PPERR_CANTOPENFILE);
	THROW_MEM(p_gds_iter = new AsyncCashGoodsIterator(NodeID, (updOnly ? ACGIF_UPDATEDONLY : 0), SinceDlsID, P_Dls));
	THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_INITFILE, f_str));
	THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	THROW_PP(fprintf(p_file, p_format, "#") > 0,  PPERR_EXPFILEWRITEFAULT);
	load_groups = (cn_data.Flags & CASHF_EXPGOODSGROUPS) ? 1 : 0;
	if(load_groups) {
		THROW_MEM(p_grp_iter = new AsyncCashGoodsGroupIterator(NodeID, 0, P_Dls));
		while(p_grp_iter->Next(&grp_info) > 0) {
			f_str = 0;
			f_str.Cat(grp_info.ID).CatCharN(';', 2);      // #1 - �� ������ �������, #2 - �� ������������ //
			name = grp_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                 // #3 - ������������ ������ �������
			f_str.Cat(name).Semicol();                 // #4 - ����� ��� ���� (?)
			f_str.CatCharN(';', 11);                      // #5-#15 - �� ����������
			f_str.Cat(grp_info.ParentID).Semicol();    // #16 - �� ������������ ������
			f_str.CatChar('0');                           // #17 - ������� �������� ������ (0)
			THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
			grp_n_level_ary.Add(grp_info.ID, grp_info.Level, 0);
		}
	}
	while(p_gds_iter->Next(&gds_info) > 0) {
		size_t bclen;
	   	if(gds_info.ID != prev_goods_id) {
			long level = 0;
			f_str = 0;
			f_str.Cat(gds_info.ID).Semicol();                                 // #1 - �� ������
			if((bclen = sstrlen(gds_info.BarCode)) != 0) {                     // #2 - �������� ��������
				gds_info.AdjustBarcode(check_dig);
				const int wp = goods_cfg.IsWghtPrefix(gds_info.BarCode);
				if(wp == 1)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(goods_cfg.WghtPrefix));
				else if(wp == 2)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(goods_cfg.WghtCntPrefix));
				else
					AddCheckDigToBarcode(gds_info.BarCode);
				f_str.Cat(gds_info.BarCode);
			}
			f_str.Semicol();
			name = gds_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                                     // #3 - ������������ ������
			f_str.Cat(name).Semicol();                                     // #4 - ����� ��� ����
			f_str.Cat(gds_info.Price, SFMT_MONEY).Semicol();               // #5 - ���� ������
			if(CheckCnFlag(CASHF_EXPGOODSREST))
				f_str.Cat(gds_info.Rest, SFMT_QTTY);                       // #6 - �������
			f_str.Semicol();
			/*
			if(UseInnerAutoDscnt && gds_info.NoDis <= 0)
				f_str.Cat(SHTRIHMFRK_INNER_SCHEME);                        // #7 - ��� ����� ���������� �������������� ������
			*/
			f_str.Semicol();
		   	unit_obj.Fetch(gds_info.UnitID, &unit_rec);
			f_str.CatChar((unit_rec.Flags & PPUnit::IntVal) ? '0' : '1');  // #8 - ��������� ������� ���-��
			f_str.Semicol();
			f_str.Cat((gds_info.DivN == 0) ? 1 : gds_info.DivN).Semicol(); // #9 - ����� ������
			f_str.Cat((gds_info.NoDis <= 0) ? 99L : -1L).Semicol();       // #10 - ������������ ������� ������
			f_str.Semicol();											  // #11 - ��� ��������� ����� (���� �� ����������)
			f_str.Semicol();                                              // #12 - �������
			f_str.CatCharN(';', 3);                                       // #13-#15 - �� ����������
			if(load_groups && gds_info.ParentID && grp_n_level_ary.Search(gds_info.ParentID, &level, 0))
				f_str.Cat(gds_info.ParentID).Semicol();                   // #16 - �� ������ �������
			else
				f_str.CatChar('0').Semicol();
			f_str.CatChar('1');                                           // #17 - ������� ������ (1)
			THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
			goods_list.add(gds_info.ID);
		}
		else {
			f_str.Z().CatChar('#').Cat(gds_info.ID).Semicol();          // #1 - �� ������
			if((bclen = sstrlen(gds_info.BarCode)) != 0) {
				gds_info.AdjustBarcode(check_dig);
				const int wp = GetGoodsCfg().IsWghtPrefix(gds_info.BarCode);
				if(wp == 1)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtPrefix));
				else if(wp == 2)
					STRNSCPY(gds_info.BarCode, gds_info.BarCode+sstrlen(GetGoodsCfg().WghtCntPrefix));
				else
					AddCheckDigToBarcode(gds_info.BarCode);
				f_str.Cat(gds_info.BarCode);                              // #2 - �������������� ��������
			}
			f_str.Semicol();
			name = gds_info.Name;
			name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			f_str.Cat(name).Semicol();                                    // #3 - ������������ ������
			f_str.Cat(name).Semicol();                                    // #4 - ������������ ��� �����
			f_str.Cat(gds_info.Price, SFMT_MONEY).Semicol();              // #5 - ���� ������
			f_str.CatCharN(';', 4);                                       // #6-#9 �� ����������
			f_str.Cat((gds_info.UnitPerPack) ? gds_info.UnitPerPack : 1); // #10 - �����������
			THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
		}
	   	prev_goods_id = gds_info.ID;
		PPWaitPercent(p_gds_iter->GetIterCounter());
	}
	//
	// ������� ������� ������� �����-�����
	//
	/* @v6.3.x ��������� �������� ��������� ����� ������������� �� �����
	if((goods_cfg.Flags & GCF_ENABLEWP) && sstrlen(goods_cfg.WghtPrefix)) {
		f_str.Z().CatChar('(').Cat(goods_cfg.WghtPrefix).Semicol();       // #1 - ������� �������� ���������
		f_str.Cat(1L).Semicol();                                            // #2 - ������� ��� ������ ����� ��� ��� ������ (1)
		f_str.Cat(5L).Semicol();                                            // #3 - ����� ������ ��� ��� (5)
		f_str.Cat(0.001).Semicol();                                         // #4 - ����������� ���� (0.001)
		f_str.Cat(0.001);                                                      // #5 - ��������� ���� (0.001)
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	*/
	//THROW(ExportSCard(p_file, updOnly));
	//int ACS_SHTRIHMFRK::ExportSCard(FILE * pFile, int updOnly)
	{
		int    r = 0;
		long   scheme_id = 0;
		PPID   ser_id = 0;
		const  char * p_format = "%s\n";
		PPObjSCardSeries scs_obj;
		PPSCardSeries ser_rec;
		PPObjSCard  s_crd_obj;
		LAssocArray scard_quot_ary;
		SString temp_buf;
		SString buf;
		SString psn_name;
		AsyncCashSCardsIterator iter(NodeID, updOnly, P_Dls, stat_id);
		scard_quot_ary.freeAll();
		for(PPID ser_id = 0; scs_obj.EnumItems(&ser_id, &ser_rec) > 0;) {
			if(!(ser_rec.Flags & SCRDSF_CREDIT)) {
				PPSCardSerPacket scs_pack;
				AsyncCashSCardInfo info;
				THROW(scs_obj.GetPacket(ser_id, &scs_pack) > 0);
				THROW_SL(scard_quot_ary.Add(ser_rec.ID, ser_rec.QuotKindID_s, 0));
				if(ser_rec.QuotKindID_s > 0)
					quot_list.addUnique(ser_rec.QuotKindID_s);
				for(iter.Init(&scs_pack); iter.Next(&info) > 0;) {
					int    denied_card = 0;
					s_crd_obj.SetInheritance(&scs_pack, &info.Rec);
					uint  pos = 0;
					denied_card = (info.Rec.PDis && !(info.Rec.Flags & SCRDF_CLOSED) && !(info.Rec.Expiry && info.Rec.Expiry < LConfig.OperDate)) ? 0 : 1;
					(psn_name = info.PsnName).Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
					scheme_id = info.Rec.ID;
					//
					// ������� ���� �������������� ������/��������
					//
					{
						temp_buf.Z().CatChar('!');
						temp_buf.Cat(scheme_id).Semicol();                         // #1 ��� �����
						temp_buf.Cat(info.Rec.Code).Cat(psn_name).Semicol();       // #2 �������� �����
						temp_buf.Cat(1L);                                          // #3 ����������� �� �����
						THROW_PP(fprintf(p_file, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					//
					// ������� �������������� ������/��������
					//
					{
						temp_buf.Z().CatChar('@');
						temp_buf.Cat(scheme_id).Semicol();                         // #1  ��� �����
						temp_buf.Cat(info.Rec.ID).Semicol();                       // #2  ���
						temp_buf.Cat(psn_name).Semicol();                          // #3  ������������
						temp_buf.Cat(1L).Cat(psn_name).Semicol();                  // #4  ��� ������
						temp_buf.Cat(1L).Semicol();                                // #5  ��� ������
						temp_buf.Cat(fdiv100i(info.Rec.PDis)).Semicol();           // #6  ������ ������
						temp_buf.Cat(psn_name).Semicol();                          // #7  ����� ��� ����
						temp_buf.Cat(info.Rec.Dt, DATF_GERMANCENT).Semicol();      // #8  ��������� ����
						temp_buf.Cat(info.Rec.Expiry, DATF_GERMANCENT).Semicol();  // #9  �������� ����
						temp_buf.Cat("00:00:00").Semicol();                        // #10 ��������� ����� //
						temp_buf.Cat("24:00:00").Cat(psn_name).Semicol();          // #11 �������� ����� //
						temp_buf.Cat(0L).Semicol();                                // #12 ����� ���������� ��� ������
						temp_buf.Cat(7L).Semicol();                                // #13 ����� ��������� ��� ������
						temp_buf.Cat(0L).Semicol();                                // #14 ��������� ����������
						temp_buf.Cat(0L).Semicol();                                // #15 �������� ����������
						temp_buf.Cat(0L).Cat(psn_name).Semicol();                  // #16 ��������� �����
						temp_buf.Cat(0L);                                          // #17 �������� �����
						temp_buf.Cat(ser_rec.QuotKindID_s);                        // #18 ��� �������������� ����
						THROW_PP(fprintf(p_file, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					//
					// ������� ���������� ����
					//
					{
						temp_buf.Z().CatChar('%');
						// buf.Z().CatCharN(' ', AC_DEF_CARD_CODE_LEN - sstrlen(info.Rec.Code)).Cat(info.Rec.Code).Transf(CTRANSF_INNER_TO_OUTER);
						temp_buf.Cat(info.Rec.Code).Semicol();            // #1 ����� �����
						temp_buf.Cat(psn_name).Semicol();                 // #2 �������� �����
						temp_buf.Cat(psn_name).Semicol();                 // #3 ����� ��� ����
						temp_buf.Cat(0L/*info.Rec.Turnover*/).Semicol();  // #4 ����� ���������� //
						temp_buf.Cat(scheme_id).Semicol();                // #5 ��� ����� �������������� ������
						temp_buf.Cat(denied_card).Semicol();              // #6 ����� ���������
						temp_buf.Cat(1L).Semicol();                       // #7 �� ����� ���������� �� �����
						temp_buf.Semicol();                               // #8 �� ������������ //
						temp_buf.Cat(0L).Semicol();                       // #9 ������������ ��� ���������
						temp_buf.CatCharN(';', 3);                        // #10-#12 �� ������������ //
						temp_buf.Cat(0L).Semicol();                       // #13 ��� ����� ������������� ������
						temp_buf.CatCharN(';', 4);                        // #14-#18 �� ������������ //
						THROW_PP(fprintf(p_file, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
					}
					iter.SetStat();
				}
			}
		}
	}
	//
	// �������� �������������� ���/���������
	//
	if(goods_list.getCount() && quot_list.getCount()) {
		const LDATE cur_dt = getcurdate_();
		uint quot_count = quot_list.getCount();
		uint goods_count = goods_list.getCount();
		for(uint i = 0; i < quot_count; i++) {
			PPID quot_kind_id = quot_list.at(i);
			SString quot_name;
			GetObjectName(PPOBJ_QUOTKIND, quot_kind_id, quot_name);
			quot_name.Transf(CTRANSF_INNER_TO_OUTER).ReplaceChar(';', 0xA4);
			for(uint j = 0; j < goods_count; j++) {
				PPID goods_id = goods_list.at(j);
				RetailPriceExtractor::ExtQuotBlock ext_block(quot_kind_id);
				RetailPriceExtractor rtl_extr(0, &ext_block, 0, ZERODATETIME, 0);
				RetailExtrItem price_item;
				if(rtl_extr.GetPrice(goods_id, 0, 1, &price_item) > 0 && price_item.Price > 0.0) {
					SString temp_buf;
					temp_buf.Z().CatChar('?');
					temp_buf.Cat(goods_id).Semicol();         // #1 ���
					temp_buf.Cat(quot_kind_id).Semicol();     // #2 ��� ��� ����.
					temp_buf.Cat(quot_name).Semicol();        // #3 ������������
					temp_buf.Cat(price_item.Price);           // #4 ����
					THROW_PP(fprintf(p_file, p_format, temp_buf.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
				}
			}
		}
	}
	//
	if(updOnly) {
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_REPLACEQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	else {
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_CLEAR, f_str));
		f_str.Cat("{AUT_S}").Cat("{DIS_C}");
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
		THROW(PPGetSubStr(PPTXT_SHTRIHMFRK_CMDSTRINGS, PPSHTRIHMFRKCS_ADDQTTY, f_str));
		THROW_PP(fprintf(p_file, p_format, f_str.cptr()) > 0, PPERR_EXPFILEWRITEFAULT);
	}
	SFile::ZClose(&p_file);
	PPWaitStop();
	PPWaitStart();
	THROW(DistributeFile_(path_goods, 0/*pEndFileName*/, dfactCopy, 0, 0));
	if(stat_id)
		P_Dls->FinishLoading(stat_id, 1, 1);
	CATCH
		SFile::ZClose(&p_file);
		ok = 0;
	ENDCATCH
	delete p_gds_iter;
	delete p_grp_iter;
	PPWaitStop();
	return ok;
}

int ACS_SHTRIHMFRK::GetSessionData(int * pSessCount, int * pIsForwardSess, DateRange * pPrd /*=0*/)
{
	int    ok = -1;
	TDialog * dlg = 0;
	ZRepList.freeAll();
	ImportedFiles = 0;
	if(!pPrd) {
		dlg = new TDialog(DLG_SELSESSRNG);
		if(CheckDialogPtrErr(&dlg)) {
			SString dt_buf;
			ChkRepPeriod.SetDate(LConfig.OperDate);
			dlg->SetupCalPeriod(CTLCAL_DATERNG_PERIOD, CTL_DATERNG_PERIOD);
			SetPeriodInput(dlg, CTL_DATERNG_PERIOD, ChkRepPeriod);
			PPWaitStop();
			for(int valid_data = 0; !valid_data && ExecView(dlg) == cmOK;) {
				if(dlg->getCtrlString(CTL_DATERNG_PERIOD, dt_buf) && ChkRepPeriod.FromStr(dt_buf, 0) && !ChkRepPeriod.IsZero()) {
					SETIFZ(ChkRepPeriod.upp, plusdate(LConfig.OperDate, 2));
					if(diffdate(ChkRepPeriod.upp, ChkRepPeriod.low) >= 0)
						ok = valid_data = 1;
				}
				if(ok < 0)
					PPErrorByDialog(dlg, CTL_DATERNG_PERIOD, PPERR_INVPERIODINPUT);
			}
			PPWaitStart();
		}
	}
	else {
		ChkRepPeriod = *pPrd;
		ok = 1;
	}
	if(ok > 0) {
		PPAsyncCashNode acn;
		THROW(GetNodeData(&acn) > 0);
		acn.GetLogNumList(LogNumList);
		{
			SString alt_imp_params;
			PPIniFile  ini_file;
			if(!PathRpt.NotEmptyS())
				THROW(PPGetFilePath(PPPATH_IN, PPFILNAM_SHTRIHMFRK_EXP_TXT,  PathRpt));
			if(!PathFlag.NotEmptyS())
				THROW(PPGetFilePath(PPPATH_OUT, PPFILNAM_SHTRIHMFRK_EXP_FLAG, PathFlag));
		}
		THROW_PP(acn.ExpPaths.NotEmptyS() || acn.ImpFiles.NotEmptyS(), PPERR_INVFILESET);
		ImpPaths.Z();
		ImpPaths.setDelim(";");
		{
			SString & r_list = (acn.ImpFiles.NotEmpty()) ? acn.ImpFiles : acn.ExpPaths;
			ImpPaths.setBuf(r_list, r_list.Len() + 1);
		}
		ExpPaths.Z();
		ExpPaths.setDelim(";");
		{
			SString & r_list = (acn.ExpPaths.NotEmpty()) ? acn.ExpPaths : acn.ImpFiles;
			ExpPaths.setBuf(r_list, r_list.Len() + 1);
		}
		THROW(ImportFiles());
		THROW(GetZRepList(&ZRepList));
	}
	CATCHZOK
	ASSIGN_PTR(pSessCount, (ok > 0) ? ZRepList.getCount() : 0);
	ASSIGN_PTR(pIsForwardSess, (ok > 0) ? 1 : 0);
	delete dlg;
	return ok;
}

int ACS_SHTRIHMFRK::ImportFiles()
{
	const  PPEquipConfig & r_eq_cfg = CC.GetEqCfg();
	long   delay_quant = 5 * 60 * 1000; // 5 ���
	const  char * p_ftp_flag = "ftp:";
	int    ok = 1, ftp_connected = 0, notify_timeout = (ImpExpTimeout) ? ImpExpTimeout : (1 * 60 * 60 * 1000); // ������� �� ��������� - 1 ���.
	double timeouts_c = fdivi(notify_timeout, delay_quant);
	uint   set_no = 0;
	SString imp_path, exp_path, path_rpt, path_flag, str_imp_paths;
	LDATE  first_date = ChkRepPeriod.low, last_date = ChkRepPeriod.upp;
	StringSet imp_paths(";");
	PPInternetAccount acct;
	PPObjInternetAccount obj_acct;
	WinInetFTP ftp;

	ImportedFiles = 0;
	SETIFZ(last_date, plusdate(LConfig.OperDate, 2));
	first_date = plusdate(first_date, -1);
	last_date  = plusdate(last_date, 1);
	if(r_eq_cfg.FtpAcctID)
		THROW(obj_acct.Get(r_eq_cfg.FtpAcctID, &acct));
	for(uint i = 0; ImpPaths.get(&i, imp_path);) {
		if(imp_path.HasPrefixIAscii(p_ftp_flag)) {
			SString ftp_path, ftp_path_flag, ftp_dir, file_name;
			SFsPath sp;
			imp_path.ShiftLeft(sstrlen(p_ftp_flag));
			if(!ftp_connected) {
				THROW(ftp.Init());
				THROW(ftp.Connect(&acct));
				ftp_connected = 1;
			}
			{
				SFsPath sp(imp_path);
				sp.Merge(0, SFsPath::fNam|SFsPath::fExt, ftp_dir);
				sp.Split(PathRpt);
				sp.Merge(0, SFsPath::fDrv|SFsPath::fDir, file_name);
				(ftp_path = ftp_dir).SetLastSlash().Cat(file_name);
				sp.Split(PathFlag);
				sp.Merge(0, SFsPath::fDrv|SFsPath::fDir|SFsPath::fExt, file_name);
				(ftp_path_flag = ftp_dir).SetLastSlash().Cat(file_name);
			}
			/* ������ ����� ����������� ��� �������� Z ������
			SFile query_file(PathFlag, SFile::mWrite);
			query_file.Close();
			THROW(ftp.SafePut(PathFlag, ftp_path_flag, 0, 0, 0));
			SDelay(60 * 1000);
			*/
			path_rpt = PathRpt;
			for(double j = 0; j < timeouts_c; j++) {
				if(ftp.SafeGet(path_rpt, ftp_path, 0, 0, 0) > 0) {
					ftp.SafeDelete(ftp_path, 0);
					if(ImportedFiles.Len())
						ImportedFiles.Semicol();
					ImportedFiles.Cat(path_rpt);
					ok = 1;
					break;
				}
				else
					SDelay(delay_quant);
			}
		}
		else {
			if(str_imp_paths.NotEmptyS())
				str_imp_paths.Semicol();
			str_imp_paths.Cat(imp_path);
		}
	}
	if(str_imp_paths.Len())
		imp_paths.setBuf(str_imp_paths, str_imp_paths.Len() + 1);
	for(uint i = 0; imp_paths.get(&i, imp_path);) {
		set_no++;
		{
			int exp_path_found = 0;
			for(uint j = 0, n = 0; !exp_path_found && ExpPaths.get(&j, exp_path);)
				if(++n == set_no)
					exp_path_found = 1;
			if(!exp_path_found)
				exp_path = imp_path;
		}
		if(::access(imp_path, 0) == 0) {
			path_rpt  = PathRpt;
			path_flag = PathFlag;
			SFsPath::ReplacePath(path_rpt,   imp_path, 1);
			SFsPath::ReplacePath(path_flag,  exp_path, 1);
			if(ok > 0) {
				for(double j = 0; j < timeouts_c; j++) {
					if(fileExists(path_rpt)) {
						if(ImportedFiles.Len())
							ImportedFiles.Semicol();
						ImportedFiles.Cat(path_rpt);
						ok = 1;
						break;
					}
					else
						SDelay(delay_quant);
				}
			}
		}
		else {
			// log message
		}
	}
	{
		const char * p_prefix = "slp";
		SString path, temp_path;
		SString temp_dir;
		for(uint file_no = 0; path.GetSubFrom(ImportedFiles, ';', file_no) > 0; file_no++) {
			if(fileExists(path)) {
				SFsPath sp(path);
				sp.Merge(0, SFsPath::fNam|SFsPath::fExt, temp_dir);
				// AHTOXA �������� ��������� ������, ���� �� ���-�� ����� ������ 30 {
				{
					uint   files_count = 0;
					LDATETIME dtm;
					dtm.SetFar();
					SString firstf_path;
					SDirEntry sde;
					dtm.SetMax();
					(temp_path = temp_dir.SetLastSlash()).Cat(p_prefix).Cat("?????.txt");
					for(SDirec sd(temp_path); sd.Next(&sde) > 0;) {
						LDATETIME iter_dtm;
						iter_dtm.SetNs100(sde.ModTm_);
						if(dtm.IsFar() || cmp(iter_dtm, dtm) < 0) {
							dtm = iter_dtm;
							sde.GetNameA(temp_dir, firstf_path);
						}
						files_count++;
					}
					if(files_count > 29)
						SFile::Remove(firstf_path);
				}
                MakeTempFileName(temp_dir.SetLastSlash(), p_prefix, "txt", 0, temp_path);
				SCopyFile(path, temp_path, 0, 0, 0);
			}
		}
	}
	CATCHZOK
	return ok;
}

int ACS_SHTRIHMFRK::GetZRepList(LAssocArray * pZRepList)
{
	int    ok = 1;
	SString path, buf;
	StringSet ss(';', ImportedFiles);
	LAssocArray zrep_list;
	for(uint i = 0, file_no = 0; ss.get(&i, path); file_no++) {
		uint   pos = 0;
		long   op_type = 0, nsmena = 0, cash_no = 0;
		LAssocArray zrep_ary; // ���� {�����_�����; �����_�����}
		SString imp_file_name = path;
		SFile  imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name
		PPSetAddedMsgString(imp_file_name);
		THROW_SL(imp_file.IsValid());
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		//
		// �������� ������ Z-������� � ������ zrep_list.
		//
		while(imp_file.ReadLine(buf) > 0) {
			StringSet ss1(';', buf);
			ss1.get(&(pos = 0), buf);                                                      // #1 ��� ���������� (�� ����������)
			ss1.get(&pos, buf);                                                            // #2 ���� ����������
 			ss1.get(&pos, buf);                                                            // #3 ����� ����������
			ss1.get(&pos, buf);                                                            // #4 ��� ���������� (��������)
			op_type = buf.ToLong();
			if(op_type == OPTYPE_ZREPORT) {
				ss1.get(&pos, buf);
				cash_no = buf.ToLong();													   // #5 ����� ���
				ss1.get(&pos, buf);                                                        // #6 ����������
				ss1.get(&pos, buf);                                                        // #7 ����������
				ss1.get(&pos, buf);                                                        // #8 ����� �����
				nsmena = buf.ToLong();
				if(LogNumList.lsearch(cash_no))
					zrep_list.Add(file_no, nsmena, 0);
			}
		}
	}
	ASSIGN_PTR(pZRepList, zrep_list);
	CATCHZOK
	return ok;
}

int ACS_SHTRIHMFRK::ConvertWareList(const char * pImpPath, int numSmena)
{
	int    ok = 1;
	int    smena_found = 0;
	uint   pos = 0;
	long   op_type = 0;
	long   count = 0;
	long   prev_smena_pos = 0;
	PPID   grp_id = 0;
	PPID   goods_id = 0;
	LDATE  dt = ZERODATE;
	LTIME  tm = ZEROTIME;
	SString   buf, wait_msg;
	StringSet ss(';', 0);
	SCardCore     sc_core;
	PPGoodsPacket gds_pack;
	PPObjGoods    goods_obj;
	SString imp_file_name(pImpPath);
	SFile  imp_file(imp_file_name, SFile::mRead); // PathRpt-->imp_file_name
	PPSetAddedMsgString(imp_file_name);
	THROW_SL(imp_file.IsValid());
	{
		PPTransaction tra(1);
		THROW(tra);
		prev_smena_pos = 0;
		//
		// ������� � ����� �������� ������� ��� z ������
		//
		for(pos = 0; pos < 3; pos++)
			imp_file.ReadLine(buf);
		prev_smena_pos = imp_file.Tell();
		while(imp_file.ReadLine(buf) > 0) {
			ss.Z();
			ss.add(buf);
			ss.get(&(pos = 0), buf);        // #1 ��� ���������� (�� ����������)
			ss.get(&pos, buf);              // #2 ���� ����������
			ss.get(&pos, buf);              // #3 ����� ����������
			ss.get(&pos, buf);              // #4 ��� ���������� (��������)
			op_type = buf.ToLong();
			if(op_type == OPTYPE_ZREPORT) {
				ss.get(&pos, buf);          // #5 ����� ��� - ����������
				ss.get(&pos, buf);          // #6 ����������
				ss.get(&pos, buf);          // #7 ����������
				ss.get(&pos, buf);          // #8 ����� �����
				if(buf.ToLong() == numSmena) {
					 smena_found = 1;
					 break;
				}
				else
					prev_smena_pos = imp_file.Tell();
			}
		}
		PPLoadText(PPTXT_ACSCLS_IMPORT, wait_msg);
		PPWaitPercent(numSmena, wait_msg);
		//
		// ������������� �� ��, ��� ��� ������������� ���� ���������, ���� �������
		//
		if(smena_found) {
			int    chk_type = 0;
			double total_discount = 0.0;
			double total_amount = 0.0;
			CCheckPacket check_pack;
			BExtInsert bei(P_TmpCclTbl);
			imp_file.Seek(prev_smena_pos);
			while(imp_file.ReadLine(buf) > 0) {
				long   chk_no = 0;
				long   cash_no = 0;
				long   line_no = 0; // @debug
				ss.Z();
				ss.add(buf);
				ss.get(&(pos = 0), buf);            // #1 ��� ���������� (�� ����������)
				line_no = buf.ToLong();             // @v6.6.3 @debug
				ss.get(&pos, buf);                  // #2 ���� ����������
				strtodate(buf.Strip(), DATF_DMY, &dt);
				ss.get(&pos, buf);                  // #3 ����� ����������
				strtotime(buf.Strip(), TIMF_HMS, &tm);
				ss.get(&pos, buf);                  // #4 ��� ���������� (��������)
				op_type = buf.ToLong();
				ss.get(&pos, buf);                  // #5 ����� ���
				cash_no = buf.ToLong();
				ss.get(&pos, buf);                  // #6 ����� ����
				chk_no = buf.ToLong();
				ss.get(&pos, buf);                  // #7 ��� ������� - ����������
				if(oneof3(op_type, OPTYPE_CHKLINE, OPTYPE_STORNO, OPTYPE_RETURN)) { // ������ ����, ������ ������ ��� �������
					double price = 0.0, qtty = 0.0;
					double line_sum = 0.0;
					ss.get(&pos, buf);
					goods_id = buf.ToLong();        // #8 ��� ������
					ss.get(&pos, buf);              // #9 ����� ������ - ����������
					ss.get(&pos, buf);
					price = R5(buf.ToReal());       // #10 ����
					ss.get(&pos, buf);
					qtty = R6(buf.ToReal());        // #11 ����������
					ss.get(&pos, buf);
					line_sum = buf.ToReal();        // #12 ����� �� ������
					if(qtty != 0.0 && line_sum != 0.0)
						price = line_sum / qtty;
					if(op_type == OPTYPE_STORNO) {
						//
						// ��� �������� ������ ������� �������������� ������, ������� ��� �� ����� � �� �� ����������.
						//
						uint   c = check_pack.GetCount();
						if(c)
							do {
								const CCheckLineTbl::Rec & r_line = check_pack.GetLineC(--c);
								if(r_line.GoodsID == labs(goods_id) && fabs(r_line.Quantity) == fabs(qtty)) {
									check_pack.RemoveLine_(c);
									break;
								}
							} while(c);
					}
					else {
						check_pack.InsertItem(labs(goods_id), qtty, price, 0, 0);
						if(op_type == OPTYPE_RETURN)
							chk_type = CHK_RETURN;
						else if(oneof2(op_type, OPTYPE_CHKLINE, OPTYPE_STORNO) && chk_type != CHK_RETURN)
							chk_type = CHK_SALE;
					}
				}
				else if(oneof2(op_type, OPTYPE_CHKEXTRACHARGE, OPTYPE_CHKDISDETAIL)) {
					long   dis_kind = 0;
					SCardTbl::Rec sc_rec;
					SString card_code;
					ss.get(&pos, buf);
					card_code = buf;                // #8  ����� ���������� �����, ���� 9-�� �������� == DISCOUNT_BYSCARD
					ss.get(&pos, buf);
					dis_kind = buf.ToLong();        // #9  ��� ������
					ss.get(&pos, buf);              // #10 ����������
					ss.get(&pos, buf);              // #11 ������� ������ - ����������
					ss.get(&pos, buf);              // #12 ����� ������
					total_discount = fabs(R5(buf.ToReal()));
					if(op_type == OPTYPE_CHKEXTRACHARGE)
						total_discount = -total_discount;
					//check_pack.SetTotalDiscount(total_discount, (op_type == OPTYPE_CHKEXTRACHARGE) ? CCheckPacket::stdfPlus : 0);
					if(op_type == OPTYPE_CHKDISDETAIL && dis_kind == DISCOUNT_BYSCARD && card_code.NotEmptyS() && sc_core.SearchCode(0, card_code, &sc_rec) > 0)
						check_pack.Rec.SCardID = sc_rec.ID;
					else
						check_pack.Rec.SCardID = 0;
				}
				else if(oneof2(op_type, OPTYPE_PAYM1, OPTYPE_PAYM2)) { // ������
					long   paym_type;
					double submit = 0.0; // �����
					ss.get(&pos, buf);                // #8  - ��� ��������� �����
					ss.get(&pos, buf);                // #9  - ����������
					ss.get(&pos, buf);                // #10 - ����� �����
					submit = buf.ToReal();
					ss.get(&pos, buf);
					paym_type = buf.ToLong();         // #11 - ����� ���� ������
					ss.get(&pos, buf);                // #12 - ����� ����
					if(chk_type == CHK_RETURN)
						total_amount = -R2(-buf.ToReal()-submit);
					else
						total_amount = R2(buf.ToReal()-submit);
					if(paym_type != PAYMENT_CASH)
						check_pack.Rec.Flags |= CCHKF_BANKING;
				}
				else if(op_type == OPTYPE_CHKCLOSED) {
					if(LogNumList.lsearch(cash_no)) {
						int    hour = 0;
						int    min = 0;
						PPID   id = 0;
						LDATETIME  dttm;
						CCheckLineTbl::Rec cchkl_rec;
						dttm.Set(dt, tm);
						if(oneof2(chk_type, CHK_SALE, CHK_RETURN)) {
							long   fl  = (chk_type == CHK_RETURN) ? CCHKF_RETURN : 0;
							fl |= CCHKF_TEMPSESS;
							SETFLAG(fl, CCHKF_BANKING, check_pack.Rec.Flags & CCHKF_BANKING);
							THROW(AddTempCheck(&id, numSmena, fl, cash_no, chk_no, 0, 0, dttm, 0, 0));
						}
						if(check_pack.Rec.SCardID)
							THROW(AddTempCheckSCardID(id, check_pack.Rec.SCardID));
						PPID   chk_id = P_TmpCcTbl->data.ID;
						{
							//
							// ��������� ������ �� ��� � ������������ ����� ���� �� ��������, ��������
							// �� ������� ����� (total_amount)
							//
							double chk_amt = 0.0;
							double chk_dis = 0.0;
							check_pack.CalcAmount(&chk_amt, &chk_dis);
							if(chk_amt != total_amount || total_discount != 0.0) {
								double new_discount = chk_dis + (fabs(chk_amt) - fabs(total_amount));
								check_pack.SetTotalDiscount__(fabs(new_discount), (new_discount < 0.0) ? CCheckPacket::stdfPlus : 0);
								check_pack.CalcAmount(&chk_amt, &chk_dis);
							}
							THROW(SetTempCheckAmounts(chk_id, /*chk_amt*/total_amount, chk_dis));
						}
						for(uint chk_pos = 0; check_pack.EnumLines(&chk_pos, &cchkl_rec);) {
							TempCCheckLineTbl::Rec ccl_rec;
							ccl_rec.CheckID   = chk_id;
							ccl_rec.CheckCode = chk_no;
							ccl_rec.Dt        = P_TmpCcTbl->data.Dt;
							ccl_rec.DivID     = 0;
							ccl_rec.GoodsID   = cchkl_rec.GoodsID;
							ccl_rec.Quantity  = cchkl_rec.Quantity;
							ccl_rec.Price     = cchkl_rec.Price;
							ccl_rec.Dscnt     = cchkl_rec.Dscnt;
							THROW_DB(bei.insert(&ccl_rec));
							/*
							{
								double price = intmnytodbl(cchkl_rec.Price);
								double line_amount = R2(cchkl_rec.Quantity * price);
								double line_discount = R2(cchkl_rec.Quantity * cchkl_rec.Dscnt);
								THROW(AddTempCheckAmounts(chk_id, line_amount - line_discount, line_discount));
							}
							*/
						}
					}
					check_pack.Z();
					chk_type = 0;
					total_amount = total_discount = 0.0;
				}
				else if(op_type == OPTYPE_CANCEL) { // ������ ����
					check_pack.Z();
					chk_type = 0;
					total_amount = total_discount = 0.0;
				}
				else if(op_type == OPTYPE_ZREPORT) {// ������� ������ z ������ (�.�. ����� ��������� � ��)
					int    r = 0;
					int    h = 0;
					int    m = 0;
					PPID   chk_id = 0;
					double amount = 0.0;
					LDATETIME dttm;
					dttm.Set(dt, tm);
					ss.get(&pos, buf); // #8  - ����������
					ss.get(&pos, buf); // #9  - ����������
					ss.get(&pos, buf); // #10 - ����������
					ss.get(&pos, buf); // #11 - ����������
					ss.get(&pos, buf); // #12 - ����� ����
					amount = R2(buf.ToReal());
					r = AddTempCheck(&chk_id, numSmena, CCHKF_ZCHECK, cash_no, chk_no, 0, 0, dttm, amount, 0);
					THROW(r);
					break;
				}
			}
			THROW_DB(bei.flash());
		}
		THROW(tra.Commit());
	}
	CATCHZOK
	return ok;
}

int ACS_SHTRIHMFRK::ImportSession(int idx)
{
	int    ok = 1;
	SString path;
	if(idx >= 0 && idx < static_cast<int>(ZRepList.getCount())) {
		const long file_no   = ZRepList.at(idx).Key;
		const long num_smena = ZRepList.at(idx).Val;
		if(path.GetSubFrom(ImportedFiles, ';', file_no) > 0 && fileExists(path)) {
			THROW(CreateTables());
			THROW(ConvertWareList(path, num_smena));
		}
	}
	CATCHZOK
	return ok;
}

int ACS_SHTRIHMFRK::FinishImportSession(PPIDArray * pSessList)
{
	//
	// @v6.6.5 AHTOXA ������ ����� �������.
	// ��� ��� Minipos ���������� ���� � ������� � ��� �����, ��-�� ���� ��� ��������� ������
	//
	StringSet ss(';', ImportedFiles);
	SString path;
	for(uint i = 0; ss.get(&i, path);)
		SFile::Remove(path);
	//
	return 1; // pSessList->addUnique(&SessAry);
}
