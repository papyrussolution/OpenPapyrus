// SOBLK.CPP
// Copyright (c) A.Sobolev 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

class Backend_SelectObjectBlock {
public:
	//
	// Операторы
	//
	enum {
		oSelect = 1,
		oSet,
		oCreate,
		oGrant,
		oRevoke,
		oGet,
		oCCheckCreate,
		oCCheckAddLine,
		oCCheckFinish,
		oSCardRest,
		oSCardDeposit,
		oSCardWithdraw,
		oGtaCheckIn,
		oBillCreate,                // @v7.4.12
		oBillAddLine,               // @v7.4.12
		oBillFinish,                // @v7.4.12
		oDraftTransitGoodsRest,     // @v7.6.0
		oSetPersonRel,              // @v7.6.1
		oGetPersonRel,              // @v7.6.1
		oDraftTransitGoodsRestList, // @v7.6.4
		oSetObjectTag,              // @v7.7.0
		oIncObjectTag,              // @v7.7.0
		oDecObjectTag,              // @v7.7.0
		oGetObjectTag,              // @v7.7.0
		oGetGoodsMatrix,            // @v8.1.9
		oGetTSessPlaceStatus,       // @v8.7.8
		oTSessCipCheckIn,           // @v8.8.2
		oTSessCipCancel,            // @v8.8.2
	};
	//
	// Критерии
	//
	enum {
		cID = 1,
		cCode,
		cName,
		cSubName,
		cParent,
		cBrand,
		cManuf,
		cBarcode,
		cArCode,
		cOwner,
		cKind,
		cRegister,
		cSeller,
		cLocation,
		cLocWorld,
		cBuyer,
		cArticle,
		cGoods,
		cGoodsGroup,
		cCurrency,
		cValue,
		cActual,
		cCategory,
		cStatus,
		cPassword,
		cFormat,    // Формат вывода результатов
		cMatrix,
		cMatrixLoc,
		cPassive,
		cGeneric, // @v10.7.7
		cAndFlags,
		cNotFlags,
		cPeriod,
		cSCard,
		cSeries,
		cPosNode,
		cGlobalUser,
		cObjType,
		cObjID,
		cGtaOp,
		cQuantity,
		cPrice,
		cAmount,
		cDiscount,
		cCount,
		cDuration,
		cType,
		cGrouping,
		cDate,
		cTime,
		cMemo,
		cAgent,
		cOp,
		cDlvrLoc,
		cPage,
		cLast,
		cUhttStore,
		cReverse,
		cPersonRelType,
		cPrimary,
		cSecondary,
		cTag,
		cPerson,
		cTagExist,
		cTagValue,  // Подкритерием (обязательным) служит символ тега

		cClass,
		cGcDimX,
		cGcDimY,
		cGcDimZ,
		cGcGimW,
		cGcKind,
		cGcGrade,
		cGcAdd,
		cGcAdd2,
		cQuotKind,

		cSeparate,
		cStripSSfx, //
		cPhone,     //
		cSince,     //
		cUUID,      //
		cProcessor, //
		cPlace,     //
		cTSession,  //
		cPinCode,   //
		cCip,       //

		cGeoTracking // @v10.1.5
	};
	//
	// Подкритерии
	//
	enum {
		scID = 1,
		scCode,
		scName,
		scDef,
		scXml,    // Формат вывода результатов FORMAT.XML
		scTddo,   // Формат вывода результатов FORMAT.TDDO (требует дополнительного аргумента - имя шаблона)
		scBinary, // Формат вывода результатов FORMAT.BINARY
		scJson,   // Формат вывода результатов FORMAT.JSON (DL600 структура)
		scXmlUtf8 // Формат вывода результатов FORMAT.XMLUTF8 (по умолчанию для xml кодировка ANSI)
	};

	Backend_SelectObjectBlock();
	~Backend_SelectObjectBlock();
	//
	// Descr: Разбирает текстовую команду pStr.
	// ARG(pStr     IN): Командная строка
	// ARG(pEndPos OUT): Не используется.
	// Returns:
	//  >0 - команда разобрана успешно.
	//  0  - ошибка
	//
	int    Parse(const char * pStr);
	//
	// Descr: Выполняет предварительно разобранную функцией Parse команду.
	//
	int    Execute(PPJobSrvReply & rReply);
private:
	void   destroy()
	{
		Operator = 0;
		ObjType = 0;
		ObjTypeExt = 0;
		OutFormat = 0;
		OutTemplate.Z();
		Page = 0;
		IdList.clear();
		TagExistList.clear();
		SrchCodeList.clear();
		ZDELETEFAST(P_GoodsF);
		ZDELETEFAST(P_PsnF);
		ZDELETEFAST(P_PsnRelF);
		ZDELETEFAST(P_GgF);
		ZDELETEFAST(P_BrF);
		ZDELETEFAST(P_QF);
		ZDELETEFAST(P_GaF);
		ZDELETEFAST(P_LocF);
		ZDELETEFAST(P_LocTspF);
		ZDELETEFAST(P_WrldF);
		ZDELETEFAST(P_SpecSerF);
		ZDELETEFAST(P_SCardF);
		ZDELETEFAST(P_CurRateF);
		ZDELETEFAST(P_UhttSCardOpF);
		ZDELETEFAST(P_BillF);
		ZDELETEFAST(P_GeoTrF); // @v10.1.5
		ZDELETEFAST(P_DtGrF);
		ZDELETEFAST(P_UhttStorF);
		ZDELETEFAST(P_WorkbookF);
		ZDELETEFAST(P_SetBlk);
		ZDELETEFAST(P_TagBlk);
		ZDELETEFAST(P_TSesF);
		ZDELETEFAST(P_PrcF);
		ZDELETEFAST(P_UuidList);
		Separate = 0;
		ExtFiltFlags = 0;
		DL600StrucName.Z();
	}
	int    CheckInCriterion(int criterion, int subcriterion, const SString & rArg);
	int    ResolveCrit_QuotKind(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_OprKind(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Brand(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Loc(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_GoodsGrp(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Goods(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Cur(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_CurRateType(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_ArByPerson(int subcriterion, const SString & rArg, PPID accSheetID, PPID * pID);
	int    ResolveCrit_Article(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Person(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_PersonKind(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_PersonCat(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_PersonStatus(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_TSessStatus(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_PersonRelType(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_SCard(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_PosNode(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_GoodsGrpListByPosNode(int subcriterion, const SString & rArg, PPIDArray * pIdList);
	int    ResolveCrit_GlobalUser(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_Page(const SString & rArg);
	int    ResolveCrit_UhttStore(int subcriterion, const SString & rArg, PPID * pUhttsID, PPID * pSellerID, PPID * pLocID, long * pUsFlags);
	int    ResolveCrit_Tag(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_GoodsClass(int subcriterion, const SString & rArg, PPID * pID);
	int    ResolveCrit_GoodsProp(PPID clsID, int prop, int subcriterion, const SString & rArg, ObjIdListFilt & rList);
	int    ResolveCrit_Since(const SString & rArg, LDATETIME * pDtm);
	int    ResolveCrit_Processor(int subcriterion, const SString & rArg, PPID * pID);
	int    FASTCALL TrimResultListByPage(StrAssocArray & rList) const;
	int    FASTCALL IncAndTestCounterForPage(int32 * pC) const;
	int    ProcessSelection_Goods(PPJobSrvReply & rResult);
	int    ProcessSelection_TSession(int _Op, const SCodepageIdent xmlCp, PPJobSrvReply & rResult);
	void   FASTCALL AddSrchCode_ByName(const SString & rArg);
	void   FASTCALL AddSrchCode_BySymb(const SString & rArg);

	struct LocalGoodsGroupFilt {
		LocalGoodsGroupFilt();
		enum {
			fSubName = 0x0001
		};
		PPID    ParentID;
		long    Flags;
		SString Name;
	};
	struct LocalBrandFilt {
		LocalBrandFilt();
		enum {
			fSubName = 0x0001
		};
		PPID   OwnerID;
		long   Flags;
		SString Name;
	};
	struct LocalGlobalAccFilt {
		LocalGlobalAccFilt();

		PPIDArray OwnerList;
	};
	struct LocalDraftTransitGoodsRestFilt {
		LocalDraftTransitGoodsRestFilt();

		PPID   GoodsID;
		PPID   LocID;
		LDATE  Dt;
	};
	struct LocalQuotFilt : public QuotFilt {
		LocalQuotFilt();

		enum {
			lfNonZeroDraftRestOnly = 0x0001
		};
		long   LocalFlags;
		SString GoodsSubText;
	};
	struct LocalUhttStoreFilt {
		LocalUhttStoreFilt();

		PPID   OwnerID;
	};
	struct LocalWorkbookFilt {
		LocalWorkbookFilt();

		enum {
			fStripSSfx = 0x0001
		};
		PPID   ParentID;
		PPID   Type;
		long   Flags;
	};
	struct LocalTSessPlaceFilt {
		LocalTSessPlaceFilt()
		{
			PTR32(PlaceCode)[0] = 0;
		}
		char   PlaceCode[24];
	};
	struct SetBlock {
		SetBlock();
		~SetBlock();

		struct SetQuotBlock {
			PPID   QuotKindID;
			PPID   GoodsID;
			PPID   SellerID;  // ->Article.ID
			PPID   LocID;
			PPID   BuyerID;   // ->Article.ID
			PPID   CurID;
			double Quot;
		};
		struct SetGoodsArCodeBlock {
			PPID   GoodsID;
			PPID   SellerID;
			char   Code[64];
		};
		struct SetPersonBlock {
			PPID   ID;
			char   Name[128];
			uint   KindCount;
			PPID   KindList[32];
			PPID   StatusID;
			PPID   CatID;
		};
		struct SetGlobalAccBlock {
			PPID   ID;
			char   Name[48];
			char   Symb[20];
			char   Password[40];
			S_GUID_Base LocalDbUuid;
			long   LocalUserID;
			PPID   PersonID;
		};
		struct SetCCheck {
			PPID   ID;
			PPID   PosNodeID;
			PPID   LocID;
			PPID   SCardID;
			PPID   GoodsID;
			double Qtty;
			double Price;
			double Amount;
			double Discount;
		};
		struct SetBill {
			PPID   ID;
			LDATE  Dt;
			char   Code[32];
			PPID   OpID;
			PPID   LocID;
			PPID   OwnerID;
			PPID   ArID;
			PPID   DlvrLocID;
			PPID   AgentID;
			PPID   CurID;

			PPID   GoodsID;
			PPID   TSessID;
			char   Serial[24];
			char   PlaceCode[24];
			double Qtty;
			double Cost;
			double Price;
			double Discount;
			double Amount;
		};
		struct SCardBlock {
			PPID   ID;
			double Amount;
		};
		struct Gta {
			int    GtaOp;
			PPID   GlobalUserID;
			PPObjID_Base Oi; // @v10.8.12 PPObjID-->PPObjID_Base
			long   Count;
			long   Duration;
		};
		struct StyloBlock {
		};
		struct ProcessorBlock {
		};
		struct TSessionBlock {
			PPID   TSessID;
            S_GUID_Base TSessUUID;
            PPID   CipID;
            PPID   PersonID;
            PPID   UhttStoreID;
            PPID   QuotKindID;
            char   PlaceCode[32];
            char   PinCode[32];
		};
		struct CheckInPersonBlock {
		};
		union {
			SetQuotBlock Q;
			SetGoodsArCodeBlock AC;
			SetPersonBlock P;
			SetGlobalAccBlock GA;
			SetCCheck C;
			SetBill   B;
			SCardBlock SC;
			Gta GT;
			StyloBlock St;
			ProcessorBlock Prc;
			TSessionBlock TS;
			CheckInPersonBlock CIP;
		} U;
		SString Memo;
		DlRtm * P_Rtm;
	};
	struct TagBlock : public ObjTagItem {
		TagBlock();
		PPID   ObjType;
		// Список идентификаторов объектов заносится в Backend_SelectObjectBlock::IdList
		SString Value;
	};
	DlRtm * P_DlRtm;
	int    Operator;
	PPID   ObjType;    // Тип объекта, фигурирующий в запросе
	long   ObjTypeExt; // Дополнительное значение типа объекта, связанное с запрошенным символом типа объекта
	SString DL600StrucName;
	SString JSONString;
	StringSet SrchCodeList;
	PPIDArray IdList;
	PPIDArray TagExistList; // Список типов тэгов, существование которых (со связкой ИЛИ) определяет список извлекаемых объектов

	int    Separate;
	long   ExtFiltFlags;
	IntRange Page;     // Диапазон номеров страниц для выборки
	enum {
		fmtXml = 1, // default
		fmtTddo,
		fmtBinary,
		fmtJson,    // @v7.6.5 @Muxa
		fmtXmlUtf8  // Явно задается вывод xml в кодировке UTF-8 (по умолчанию - ANSI)
	};
	int    OutFormat;
	SString OutTemplate; // Наименование шаблона вывода резульатов в формате TDDO
	PPObjGoods GObj;
	PPObjGoodsGroup GgObj;
	PPObjPerson PsnObj;
	PPObjArticle ArObj;
	PPObjTag TagObj;
	PPObjSCard * P_ScObj;
	PPObjTSession * P_TSesObj;
	GoodsFilt * P_GoodsF;
	PersonFilt * P_PsnF;
	PersonRelFilt * P_PsnRelF;
	LocalGoodsGroupFilt * P_GgF;
	LocalBrandFilt * P_BrF;
	LocalGlobalAccFilt * P_GaF;
	LocationFilt * P_LocF;
	LocalTSessPlaceFilt * P_LocTspF;
	PPObjWorld::SelFilt * P_WrldF;
	SpecSeriesFilt * P_SpecSerF;
	LocalQuotFilt * P_QF;
	SCardFilt * P_SCardF;
	CurRateFilt * P_CurRateF;
	UhttSCardOpFilt * P_UhttSCardOpF;
	BillFilt * P_BillF;
	GeoTrackingFilt * P_GeoTrF;
	LocalDraftTransitGoodsRestFilt * P_DtGrF;
	LocalUhttStoreFilt * P_UhttStorF;
	LocalWorkbookFilt * P_WorkbookF;
	TSessionFilt * P_TSesF;
	ProcessorFilt * P_PrcF;
	Quotation2Core * P_Qc;
	SetBlock * P_SetBlk;
	TagBlock * P_TagBlk;   // Значение тэга объекта (SETOBJECTTAG, INCOBJECTTAG, DECOBJECTTAG, GETOBJECTTAG)
	SelectObjectBlock::DistribCCheck * P_DCc;
	UuidArray * P_UuidList;
	StrAssocArray ResultList;
	SString ResultText;
};

SelectObjectBlock::SelectObjectBlock() : P_BSob(new Backend_SelectObjectBlock())
{
}

SelectObjectBlock::~SelectObjectBlock()
{
	delete P_BSob;
}

int FASTCALL SelectObjectBlock::Parse(const char * pStr) { return P_BSob->Parse(pStr); }
int FASTCALL SelectObjectBlock::Execute(PPJobSrvReply & rReply) { return P_BSob->Execute(rReply); }

//
// CCHECKCREATE POSNODE.CODE(UHTT) LOCATION.CODE(wh01) SCARD.CODE(1111122) AMOUNT(1219.40)
// CCHECKLINE ID(221117) GOODS.CODE(4601236) QTTY(1) PRICE(14.10) DISCOUNT(0)
// CCHECKFINISH ID(221117)
//
SelectObjectBlock::DistribCCheck::Header::Header()
{
	THISZERO();
}

SelectObjectBlock::DistribCCheck::Line::Line()
{
	THISZERO();
}

SelectObjectBlock::DistribCCheck::DistribCCheck() : P_CsObj(0)
{
}

SelectObjectBlock::DistribCCheck::~DistribCCheck()
{
	delete P_CsObj;
}

int SelectObjectBlock::DistribCCheck::Begin(PPID * pID, const Header & rHdr)
{
	int    ok = 1;
	PPID   cc_id = 0;
	PPID   cs_id = 0;
	PPCashNode cn_rec;
	LocationTbl::Rec loc_rec;
	THROW_MEM(SETIFZ(P_CsObj, new PPObjCSession));
	THROW(CnObj.Fetch(rHdr.PosNodeID, &cn_rec) > 0);
	THROW(cn_rec.CashType == PPCMT_DISTRIB);
	THROW(LocObj.Fetch(rHdr.LocID, &loc_rec) > 0);
	THROW_PP(loc_rec.Type == LOCTYP_ADDRESS, PPERR_LOCMUSTBEADDR);
	THROW(loc_rec.OwnerID != 0);
	if(rHdr.SCardID) {
		PPObjSCardSeries scs_obj;
		PPSCardSeries scs_rec;
		SCardTbl::Rec sc_rec;
		THROW(P_CsObj->P_Cc->Cards.Search(rHdr.SCardID, &sc_rec) > 0);
		THROW(scs_obj.Fetch(sc_rec.SeriesID, &scs_rec) > 0);
	}
	THROW(IsValidIEEE(rHdr.Amount));
	THROW(rHdr.Amount >= 0.0);
	THROW(IsValidIEEE(rHdr.Discount));
	THROW_SL(checkdate(rHdr.Dtm.d, 1));
	{
		PPTransaction tra(1);
		CCheckTbl::Rec cc_rec;
		// @v10.6.4 MEMSZERO(cc_rec);
		cc_rec.CashID = rHdr.PosNodeID;
		{
			PPSecur sec_rec;
			PPObjSecur sec_obj(PPOBJ_USR, 0);
			if(sec_obj.Fetch(LConfig.UserID, &sec_rec) > 0)
				cc_rec.UserID = sec_rec.PersonID;
		}
		if(!rHdr.Dtm.d) {
			getcurdatetime(&cc_rec.Dt, &cc_rec.Tm);
		}
		else {
			cc_rec.Dt = rHdr.Dtm.d;
			cc_rec.Tm = rHdr.Dtm.t;
		}
		cc_rec.Flags |= (CCHKF_SYNC|CCHKF_SKIP);
		cc_rec.SCardID = rHdr.SCardID;
		LDBLTOMONEY(rHdr.Amount, cc_rec.Amount);
		LDBLTOMONEY(rHdr.Discount, cc_rec.Discount);
		THROW(tra);
		{
			CCheckTbl::Rec cc_code_rec;
			if(P_CsObj->P_Cc->GetLastCheckByCode(rHdr.PosNodeID, &cc_code_rec) > 0)
				cc_rec.Code = cc_code_rec.Code + 1;
			else
				cc_rec.Code = 1;
		}
		{
			//
			// Для распределенных чеков сессии создаются по одной на каждый день для каждой точки.
			//
			long   sess_number = 0;
			CSessionTbl::Rec last_cs_rec;
			THROW(P_CsObj->P_Tbl->GetLastNumber(rHdr.PosNodeID, rHdr.LocID, &sess_number, &last_cs_rec));
			if(!sess_number || last_cs_rec.Dt != cc_rec.Dt) {
				sess_number++;
				LDATETIME cs_dtm = getcurdatetime_();
				THROW(P_CsObj->P_Tbl->CreateSess(&cs_id, rHdr.PosNodeID, rHdr.LocID, sess_number, cs_dtm, 0));
			}
			else {
				cs_id = last_cs_rec.ID;
			}
			cc_rec.SessID = cs_id;
		}
		{
			//
			// Проверка на отсутствие дублирования по ключу {CashID; Dt; Tm}
			//
			CCheckTbl::Key1 cck1;
			MEMSZERO(cck1);
			cck1.Dt = cc_rec.Dt;
			cck1.Tm = cc_rec.Tm;
			cck1.CashID = cc_rec.CashID;
			while(P_CsObj->P_Cc->search(1, &cck1, spEq)) {
				cc_rec.Tm.v++;
				cck1.Tm = cc_rec.Tm;
			}
		}
		THROW(P_CsObj->P_Cc->Add(&cc_id, &cc_rec, 0));
		THROW(tra.Commit());
	}
	CATCH
		cc_id = 0;
		ok = 0;
	ENDCATCH
	ASSIGN_PTR(pID, cc_id);
	return ok;
}

int SelectObjectBlock::DistribCCheck::AddLine(PPID ccID, const Line & rLn)
{
	int    ok = -1;
	return ok;
}

int SelectObjectBlock::DistribCCheck::Finish(PPID ccID, PPID * pNewCcID)
{
	int    ok = 1;
	PPID   cc_id = 0;
	THROW_MEM(SETIFZ(P_CsObj, new PPObjCSession));
	{
		PPTransaction tra(1);
		CCheckPacket cc_pack;
		THROW(tra);
		THROW(P_CsObj->P_Cc->LoadPacket(ccID, 0, &cc_pack) > 0);
		THROW(cc_pack.Rec.Flags & CCHKF_SKIP);
		THROW(P_CsObj->P_Cc->RemovePacket(ccID, 0));
		//
		// SetupAmount следует использовать только в том случае, если сумма чека обязана соответствовать
		// суммам по строкам. На начальном этапе распределенный чеки будут работать без строк, потому
		// SetupAmount безусловно не используем.
		//
		//cc_pack.SetupAmount(0, 0);
		cc_pack.Rec.Flags &= ~CCHKF_SKIP;
		cc_pack.Rec.ID = 0;
		THROW(P_CsObj->P_Cc->TurnCheck(&cc_pack, 0));
		THROW(tra.Commit());
		cc_id = cc_pack.Rec.ID;
	}
	CATCHZOK
	ASSIGN_PTR(pNewCcID, cc_id);
	return ok;
}
/*

------------------------------------------------
Examples:

SELECT goods BY goodsgroup.code(33)
SELECT goods BY brand(10014) goodsgroup(14)
SELECT goods BY barcode(4607089460013)
SELECT person BY kind.symb(BANK)
SELECT person BY register.def(10022, 5550005)
SELECT person BY register.code(INN, 100100202204)
SELECT personkind by id(1002) id(1003)

SELECT person BY kind.symb(BANK) format.binary

SELECT geotracking BY period(1/5/2018..31/5/2018) objtype(BILL)

format.xml
format.tddo(template_name)
format.bin

SELECT DL600 UhttPerson by id(21319)
SET QUOT KIND(BASE) SELLER.CODE(10001) LOCATION.CODE(230223300391) GOODS(52117) CUR.CODE() VALUE(24.10)
SET GOODSARCODE GOODS.BARCODE(46072740) SELLER.CODE(10001) VALUE(15219)

SET DL600 STRUCTNAME JSON-строка

CREATE PERSON name(my name) kind.code(user)
CREATE GLOBALUSER name(abracad) password(x2-19) datasource(uuid)

-------

CHECK_GLOBAL_CREDENTIAL

GRANT remove on bill to user(Alex)
REVOKE remove on bill from user(Alex)

------------------------------------------------

CCHECKCREATE POSNODE.CODE(UHTT) LOCATION.CODE(wh01) SCARD.CODE(1111122) AMOUNT(1219.40)
CCHECKLINE ID(221117) GOODS.CODE(4601236) QTTY(1) PRICE(14.10) DISCOUNT(0)
CCHECKFINISH ID(221117)

SCARDREST ID(14519)
SCARDREST CODE(1111122)
SCARDWITHDRAW ID(14519) AMOUNT(300.00)
SCARDWITHDRAW CODE(1111122) AMOUNT(300.00)

------------------------------------------------

set_expr ::= 'SET' set_object set_criteria_expr
set_object ::=
	'QUOT'       | // KIND, GOODS, VALUE обязательны
	'GOODSARCODE'  // GOODS, SELLER, VALUE обязательны
set_criteria_expr ::= criteria_list


select_expr ::= 'SELECT' selection_object criteria_expr
selection_object ::=
	dl600_selection_expr |
	'QUOT'       |
	'GOODS'      |
	'GOODSGROUP' |
	'BRAND'      |
	'PERSONKIND' |
	'PERSON'     |
	'COUNTRY'    |
	'CITY'
dl600_selection_expr ::= 'DL600' struct_name
criteria_expr ::= 'BY' criteria_list
criteria_list ::= criterion_term | criteria_list criterion_term
criterion_term ::=
	crit_descr '(' text ')' |
	crit_descr '.' crit_modif '(' text ')'

crit_descr ::=
	'ID'         |  // Идентификатор объекта
	'CODE'       |  // Код (символ) объекта
	'NAME'       |  // Наименование объекта
	'SUBNAME'    |  // Подстрока в наименовании объекта
	'PARENT'     |  // Родительский объект
	'BRAND'      |  // Брэнд (для товаров)
	'MANUF'      |  // Производитель (для товаров)
	'BARCODE'    |  // Штрихкод (для товаров)
	'ARCODE'     |  // Код по контрагенту (для товаров)
	'OWNER'      |  // Владелец
	'KIND'       |  // Вид
	'REGISTER'   |  // Номер регистрационного документа
	'SELLER'     |  // Продавец
	'LOCATION'   |  // Локация (склад, адрес)
	'LOCWORLD'   |  // Географический объект, которому принадлежит локация //
	'BUYER'      |  // Покупатель
	'GOODS'      |  // Товар
	'GOODSGROUP' |  // Товарная группа
	'CUR'        |  // Валюта
	'VALUE'      |  // Значение
	'CATEGORY'      // Категория (персоналии)

crit_modif ::=
	'ID'         |  // Идентификатор объекта
	'CODE'       |  // Код (символ) объекта
	'NAME'       |  // Наименование объекта
	'DEF'           // Подкритерий по умолчанию (обычно ID)

Возможные значения crit_descr для разных значений selection_object:

dl600_selection_expr
	id
GOODS
	id(text)
	name(text)
	subname(text)
	parent(text)
		crit_modif:
			id
			code
			name
	brand(text)
		crit_modif:
			id
			code
			name
	manuf(text)
		crit_modif:
			id
			code
			name
	barcode(text)
	arcode(articleID, text)
	actual
	matrix
	passive(text)
GOODSGROUP
	id(text)
	code(text)
	name(text)
	subname(text)
	parent(text)
		crit_modif:
			id
			code
			name
BRAND
	id(text)
	name(text)
	subname(text)
	owner(text)
		crit_modif:
			id
			code
			name
PERSONKIND
	id(text)
	name(text)
PERSON
	id(text)
	name(text)
	subname(text)
	kind(text)
		crit_modif:
			id
			code
			name
	register(register_text, number_text[, serial_text])
		crit_modif:
			def
			id
			code
	city
COUNTRY
	id
	code
	name
	subname
CITY
	id
	code
	name
	subname
	country

GEOTRACKING
	objtype
	objid
	period

QUOT
	ACTUAL
	KIND             ID, CODE
	SELLER           ID, CODE, NAME
	LOCATION         ID, CODE
	LOCWORLD         ID, CODE
	BUYER            ID, CODE, NAME
	ARTICLE          ID
	GOODS            ID, CODE
	GOODSGROUP       ID, CODE, NAME
	BRAND            ID, NAME
	CUR              ID, CODE

GLOBALUSER
	id
	name

PROCESSOR
	id(text)
	code(text)
	name(text)
	subname(text)
	parent(text)
		crit_modif
			id
			code
			name

TSESSION
	id(text)
	code(text)
	processor(text)
		crit_modif:
			id
			code
			name
	status(text)
		planned pending inprocess closed canceled
	since(datetime)
    before(datetime)

STYLOPALM
	id(text)
	code(text)
	name(text)
	parent(text)
		crit_modif
			id
			code
			name

CREATE PROCESSOR
CREATE TSESSION
CREATE STYLOPALM


SET QUOT
	KIND
	GOODS
	SELLER
	LOCATION
	BUYER
	CUR
	VALUE

CREATE PERSON
	KIND             ID, CODE
	STATUS           ID, CODE, NAME
	NAME
	CATEGORY         ID, CODE

CREATE GLOBALUSER
	NAME
	PASSWORD
	owner            id code name

BILLCREATE  OP DATE CODE OWNER LOCATION ARTICLE CUR AGENT DLVRLOC MEMO
BILLADDLINE ID GOODS SERIES QTTY PRICE DISCOUNT AMOUNT
BILLFINISH ID

SETPERSONREL PIRMARY SECONDARY PERSONRELTYPE
GETPERSONREL [PIRMARY] [SECONDARY] [PERSONRELTYPE]

SETOBJECTTAG TAG.CODE(LikeCount) PERSON(95302) VALUE(10)
INCOBJECTTAG TAG.CODE(DislikeCount) PERSON(95302)
DECOBJECTTAG TAG.CODE(DislikeCount) PERSON(95302)
GETOBJECTTAG TAG.CODE(DislikeCount) PERSON(95302)

GETOBJATTACHMENTINFO
	OBJTYPE
	ID

*/
static int FASTCALL Helper_ProcessTddo(long dataId, void * pAry, const char * pDataName, const SString & rOutTemplate, PPJobSrvReply & rResult)
{
	int       ok = 1;
	SString   temp_buf, txt_buf, file_name;
	StringSet in_line(',', rOutTemplate);
	StringSet ext_param_list;
	uint   ip = 0;
	THROW_PP(in_line.get(&ip, file_name), PPERR_CMDSEL_MISSTDDOTEMPLATE);
	THROW(Tddo::LoadFile(file_name, temp_buf));
	while(in_line.get(&ip, txt_buf))
		ext_param_list.add(txt_buf.Strip());
	{
		Tddo t;
		t.SetInputFileName(file_name);
		DlRtm::ExportParam ep;
		PPFilt _pf(pAry);
		_pf.ID = dataId;
		ep.P_F = &_pf;
		THROW(t.Process(pDataName, temp_buf, /*dataId, pAry*/ep, &ext_param_list, rResult)); // @badcast
		rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
	}
	CATCHZOK
	return ok;
}

Backend_SelectObjectBlock::LocalGoodsGroupFilt::LocalGoodsGroupFilt() : ParentID(0), Flags(0)
{
}

Backend_SelectObjectBlock::LocalBrandFilt::LocalBrandFilt() : OwnerID(0), Flags(0)
{
}

Backend_SelectObjectBlock::LocalGlobalAccFilt::LocalGlobalAccFilt()
{
}

Backend_SelectObjectBlock::LocalDraftTransitGoodsRestFilt::LocalDraftTransitGoodsRestFilt() : GoodsID(0), LocID(0), Dt(ZERODATE)
{
}

Backend_SelectObjectBlock::LocalUhttStoreFilt::LocalUhttStoreFilt() : OwnerID(0)
{
}

Backend_SelectObjectBlock::LocalWorkbookFilt::LocalWorkbookFilt() : ParentID(0), Type(0), Flags(0)
{
}

Backend_SelectObjectBlock::LocalQuotFilt::LocalQuotFilt() : QuotFilt(), LocalFlags(0)
{
}

Backend_SelectObjectBlock::SetBlock::SetBlock() : P_Rtm(0)
{
	MEMSZERO(U);
}

Backend_SelectObjectBlock::TagBlock::TagBlock() : ObjTagItem(), ObjType(0)
{
}

Backend_SelectObjectBlock::SetBlock::~SetBlock()
{
	delete P_Rtm;
}

Backend_SelectObjectBlock::Backend_SelectObjectBlock() : Operator(0), ObjType(0), ObjTypeExt(0), P_ScObj(0),
	P_TSesObj(0), P_GoodsF(0), P_PsnF(0), P_PsnRelF(0), P_GgF(0), P_BrF(0), P_QF(0), P_GaF(0), P_LocF(0),
	P_LocTspF(0), P_WrldF(0), P_SpecSerF(0), P_SCardF(0), P_CurRateF(0), P_UhttSCardOpF(0), P_BillF(0), P_GeoTrF(0),
	P_DtGrF(0), P_UhttStorF(0), P_WorkbookF(0), P_TSesF(0), P_PrcF(0), P_Qc(0), P_SetBlk(0), P_TagBlk(0),
	P_DCc(0), OutFormat(fmtXml), Separate(0), P_UuidList(0), P_DlRtm(0), ExtFiltFlags(0)
{
	Page = 0;
}

Backend_SelectObjectBlock::~Backend_SelectObjectBlock()
{
	destroy();
	ZDELETEFAST(P_ScObj);
	ZDELETEFAST(P_TSesObj);
	ZDELETEFAST(P_Qc);
	ZDELETEFAST(P_DCc);
}

int Backend_SelectObjectBlock::Parse(const char * pStr)
{
	static const SIntToSymbTabEntry crit_titles[] = {
		{ cID,            "ID"   },
		{ cCode,          "CODE" },
		{ cName,          "NAME" },
		{ cSubName,       "SUBNAME" },
		{ cParent,        "PARENT" },
		{ cBrand,         "BRAND" },
		{ cManuf,         "MANUF" },
		{ cBarcode,       "BARCODE" },
		{ cArCode,        "ARCODE" },
		{ cOwner,         "OWNER" },
		{ cKind,          "KIND" },
		{ cRegister,      "REGISTER" },
		{ cSeller,        "SELLER" },
		{ cLocation,      "LOCATION" },
		{ cLocWorld,      "LOCWORLD" },
		{ cBuyer,         "BUYER" },
		{ cArticle,       "ARTICLE" },
		{ cGoods,         "GOODS" },
		{ cGoodsGroup,    "GOODSGROUP" },
		{ cCurrency,      "CUR" },
		{ cValue,         "VALUE" },
		{ cActual,        "ACTUAL" },
		{ cCategory,      "CATEGORY" },
		{ cPassword,      "PASSWORD" },
		{ cStatus,        "STATUS"  },
		{ cFormat,        "FORMAT"  },
		{ cMatrix,        "MATRIX"  },
		{ cMatrixLoc,     "MATRIXLOC" },
		{ cPassive,       "PASSIVE" },
		{ cGeneric,       "GENERIC" }, // @v10.7.7
		{ cAndFlags,      "ANDFLAGS" },
		{ cNotFlags,      "NOTFLAGS" },
		{ cPeriod,        "PERIOD" },
		{ cSCard,         "SCARD" },
		{ cSeries,        "SERIES" },
		{ cPosNode,       "POSNODE" },
		{ cGlobalUser,    "GLOBALUSER" },
		{ cObjType,       "OBJTYPE" },
		{ cObjID,         "OBJID"   },
		{ cGtaOp,         "GTAOP"   },
		{ cQuantity,      "QTTY" },
		{ cPrice,         "PRICE" },
		{ cAmount,        "AMOUNT" },
		{ cDiscount,      "DISCOUNT" },
		{ cCount,         "COUNT" },
		{ cDuration,      "DURATION" },
		{ cType,          "TYPE" },
		{ cGrouping,      "GROUPING" },
		{ cDate,          "DATE" },
		{ cTime,          "TIME" },
		{ cMemo,          "MEMO" },
		{ cAgent,         "AGENT" },
		{ cOp,            "OP"    },
		{ cDlvrLoc,       "DLVRLOC" },
		{ cPage,          "PAGE" },
		{ cLast,          "LAST" },
		{ cUhttStore,     "UHTTSTORE"     },
		{ cReverse,       "REVERSE"       },
		{ cPersonRelType, "PERSONRELTYPE" },
		{ cPrimary,       "PRIMARY"       },
		{ cSecondary,     "SECONDARY"     },
		{ cTag,           "TAG"           },
		{ cPerson,        "PERSON"        },
		{ cTagExist,      "TAGEXIST"      },
		{ cTagValue,      "TAGVALUE"      },
		{ cClass,         "CLASS"         },
		{ cGcDimX,        "GCDIMX"        },
		{ cGcDimY,        "GCDIMY"        },
		{ cGcDimZ,        "GCDIMZ"        },
		{ cGcGimW,        "GCDIMW"        },
		{ cGcKind,        "GCKIND"        },
		{ cGcGrade,       "GCGRADE"       },
		{ cGcAdd,         "GCADD"         },
		{ cGcAdd2,        "GCADD2"        },
		{ cQuotKind,      "QUOTKIND"      },
		{ cSeparate,      "SEPARATE"  },
		{ cStripSSfx,     "STRIPSSFX" },
		{ cPhone,		  "PHONE"     },
		{ cSince,         "SINCE"     },
		{ cUUID,          "GUID"      },
		{ cUUID,          "UUID"      },
		{ cProcessor,     "PROCESSOR" },
		{ cPlace,         "PLACE"     },
		{ cTSession,      "TSESSION"  },
		{ cPinCode,       "PINCODE"   },
		{ cCip,           "CIP"       }
		/*
		{ cCountry,  "COUNTRY" },
		{ cCity,     "CITY" }
		*/
	};
	static const SIntToSymbTabEntry subcrit_titles[] = {
		{ scID,      "ID"   },
		{ scCode,    "CODE" },
		{ scCode,    "SYMBOL" }, // @v10.1.6
		{ scCode,    "SYMB" }, // @v10.1.6
		{ scName,    "NAME" },
		{ scDef,     "DEF"  },
		{ scXml,     "XML"  },
		{ scTddo,    "TDDO" },
		{ scBinary,  "BIN"  },
		{ scJson,    "JSON" },
		{ scXmlUtf8, "XMLUTF8" }
	};
	struct OperatorItem {
		const char * P_Word;
		int    Op;
		PPID   ObjType;
	};
	static const OperatorItem operator_list[] = {
		{ "SELECT",             oSelect, 0 },
		{ "SET",                oSet,    0 },
		{ "CREATE",             oCreate, 0 },
		{ "GRANT",              oGrant,  0 },
		{ "REVOKE",             oRevoke, 0 },
		{ "CCHECKCREATE",       oCCheckCreate,   PPOBJ_CCHECK },
		{ "CCHECKADDLINE",      oCCheckAddLine,  PPOBJ_CCHECK },
		{ "CCHECKFINISH",       oCCheckFinish,   PPOBJ_CCHECK },
		{ "BILLCREATE",         oBillCreate,     PPOBJ_BILL },
		{ "BILLADDLINE",        oBillAddLine,    PPOBJ_BILL },
		{ "BILLFINISH",         oBillFinish,     PPOBJ_BILL },
		{ "DRAFTTRANSITGOODSREST",     oDraftTransitGoodsRest,     PPOBJ_BILL },
		{ "DRAFTTRANSITGOODSRESTLIST", oDraftTransitGoodsRestList, PPOBJ_BILL },
		{ "SCARDREST",          oSCardRest,      PPOBJ_SCARD },
		{ "SCARDDEPOSIT",       oSCardDeposit,   PPOBJ_SCARD },
		{ "SCARDWITHDRAW",      oSCardWithdraw,  PPOBJ_SCARD },
		{ "GTACHECKIN",         oGtaCheckIn,     PPOBJ_GTA },
		{ "SETPERSONREL",       oSetPersonRel,   PPOBJ_PERSON },
		{ "GETPERSONREL",       oGetPersonRel,   PPOBJ_PERSON },
		{ "SETOBJECTTAG",       oSetObjectTag,   PPOBJ_TAGVALUE },
		{ "INCOBJECTTAG",       oIncObjectTag,   PPOBJ_TAGVALUE },
		{ "DECOBJECTTAG",       oDecObjectTag,   PPOBJ_TAGVALUE },
		{ "GETOBJECTTAG",       oGetObjectTag,   PPOBJ_TAGVALUE },
		{ "GETGOODSMATRIX",     oGetGoodsMatrix, PPOBJ_QUOT2    },
		{ "GETTSESSPLACESTATUS", oGetTSessPlaceStatus, PPOBJ_TSESSION },
		{ "TSESSCIPCHECKIN", oTSessCipCheckIn, PPOBJ_TSESSION },
		{ "TSESSCIPCANCEL",  oTSessCipCancel,  PPOBJ_TSESSION }
	};
	destroy();

	int    ok = 1;
	size_t pos = 0;
	uint   i;
	SStrScan scan(pStr);
	SString temp_buf, added_msg, obj, crit, sub_crit;
	pos = scan.Offs;
	THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_SELECT);
	temp_buf.ToUpper();
	for(i = 0; !Operator && i < SIZEOFARRAY(operator_list); i++) {
		if(temp_buf == operator_list[i].P_Word) {
			Operator = operator_list[i].Op;
			ObjType = operator_list[i].ObjType;
		}
	}
	THROW_PP(Operator, PPERR_CMDSEL_EXP_SELECT);
	pos = scan.Offs;
	if(!ObjType) {
		THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_OBJTYPE);
		THROW(ObjType = GetObjectTypeBySymb(temp_buf, &ObjTypeExt));
	}
	if(Operator == oSet && ObjType == PPOBJ_DL600DATA) {
		THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_DL600STRUCTNAME);
		DL600StrucName = temp_buf;
		THROW_PP(temp_buf = (SString)scan.Skip(), PPERR_CMDSEL_EXP_JSONSTRUCTURE);
		JSONString = temp_buf;
	}
	else {
		if(Operator == oSet) {
			THROW_PP(oneof2(ObjType, PPOBJ_QUOT2, PPOBJ_GOODSARCODE), PPERR_CMDSEL_INVOBJINOP_SET);
		}
		else if(Operator == oCreate) {
			THROW_PP(oneof2(ObjType, PPOBJ_PERSON, PPOBJ_GLOBALUSERACC), PPERR_CMDSEL_INVOBJINOP_CREATE);
		}
		if(ObjType == PPOBJ_DL600DATA) {
			THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_DL600STRUCTNAME);
			DL600StrucName = temp_buf;
		}
		if(scan.Skip().GetIdent(temp_buf)) {
			SString arg_buf;
			if(Operator == oSelect) {
				THROW_PP(temp_buf.IsEqiAscii("BY"), PPERR_CMDSEL_EXP_BY);
				THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_CRITERION);
			}
			do {
				int    sub_criterion = 0;
				arg_buf.Z();
				/* @v10.0.05 for(i = 0; !criterion && i < SIZEOFARRAY(crit_titles); i++) {
					if(temp_buf.IsEqNC(crit_titles[i].P_Text)) {
						crit = temp_buf;
						criterion = crit_titles[i].ID;
						(added_msg = obj).Space().Cat("BY").Space().Cat(crit);
					}
				}*/
				// @v10.0.05 {
				const int criterion = SIntToSymbTab_GetId(crit_titles, SIZEOFARRAY(crit_titles), temp_buf);
				if(criterion) {
					crit = temp_buf;
					(added_msg = obj).Space().Cat("BY").Space().Cat(crit);
				}
				// } @v10.0.05
				if(oneof4(criterion, cActual, cMatrix, cLast, cStripSSfx)) {
					//
					// Одиночные критерии (не требующие параметров): 'ACTUAL' 'MATRIX' 'LAST', 'STRIPSSFX'
					//
					sub_criterion = 0;
					arg_buf.Z();
				}
				else {
					if(scan[0] == '.') {
						scan.Incr();
						THROW_PP(scan.Skip().GetIdent(temp_buf), PPERR_CMDSEL_EXP_SUBCRITERION);
						if(criterion == cTagValue) {
							//
							// Специальный случай: для запроса TAGVALUE подкритерием является символ тега
							//
							PPID   tag_id = 0;
							THROW(TagObj.FetchBySymb(temp_buf, &tag_id) > 0);
							sub_crit = temp_buf;
							sub_criterion = tag_id;
						}
						else {
							/* @v10.0.05 for(i = 0; !sub_criterion && i < SIZEOFARRAY(subcrit_titles); i++) {
								if(temp_buf.IsEqNC(subcrit_titles[i].P_Text)) {
									sub_crit = temp_buf;
									sub_criterion = subcrit_titles[i].ID;
									(added_msg = obj).Space().Cat("BY").Space().Cat(crit).Dot().Cat(sub_crit);
								}
							}*/
							// @v10.0.05 {
							sub_criterion = SIntToSymbTab_GetId(subcrit_titles, SIZEOFARRAY(subcrit_titles), temp_buf);
							if(sub_criterion) {
								sub_crit = temp_buf;
								(added_msg = obj).Space().Cat("BY").Space().Cat(crit).Dot().Cat(sub_crit);
							}
							// } @v10.0.05
						}
					}
					THROW_PP(scan.Skip()[0] == '(', PPERR_CMDSEL_EXP_LEFTPAR);
					{
						char   c;
						int    par_count = 0;
						do {
							scan.Incr();
							c = scan[0];
							THROW_PP(c, PPERR_CMDSEL_EXP_RIGHTPAR);
							if(c == ')') {
								if(par_count)
									par_count--;
								else {
									scan.Incr();
									break;
								}
							}
							else {
								if(c == '(')
									par_count++;
							}
							arg_buf.CatChar(c);
						} while(c);
					}
				}
				THROW(CheckInCriterion(criterion, sub_criterion, arg_buf));
			} while(scan.Skip().GetIdent(temp_buf));
		}
	}
	CATCHZOK
	return ok;
}

//
// Локальная структура описания селектора интернет-магазина Universe-HTT
//
struct LocalSelectorDescr {
	LocalSelectorDescr() : ID(0), Attr(0), Clsf(0), Type(0), TagID(0)
	{
	}
	explicit LocalSelectorDescr(const StrAssocArray::Item & rSi) : ID(rSi.Id), Attr(0), Clsf(0), Type(0), Title(rSi.Txt)
	{
	}
	void   Set(const char * pCrit, const char * pPart)
	{
		Crit = pCrit;
		Part = pPart;
	}
	int    FASTCALL Set(const PPUhttStoreSelDescr::Entry & rSdEntry)
	{
		int    ok = 1;
		Attr = rSdEntry.Attr;
		//
		switch(Attr) {
			case PPUhttStoreSelDescr::attrGroup:       Set("PARENT", "id"); break;
			case PPUhttStoreSelDescr::attrBrand:       Set("BRAND", "id"); break;
			case PPUhttStoreSelDescr::attrName:        Set("NAME", "text"); break;
			case PPUhttStoreSelDescr::attrPeriod:      Set("PERIOD", "text"); break;
			case PPUhttStoreSelDescr::attrProcessor:   Set("PROCESSOR", "id"); break;
			case PPUhttStoreSelDescr::attrCity:        Set("CITY", "id"); break;
			case PPUhttStoreSelDescr::attrClass:
				Clsf = rSdEntry.GcClsf;
				switch(rSdEntry.GcClsf) {
					case PPGdsCls::eX: Set("GCDIMX", "text"); break;
					case PPGdsCls::eY: Set("GCDIMY", "text"); break;
					case PPGdsCls::eZ: Set("GCDIMZ", "text"); break;
					case PPGdsCls::eW: Set("GCDIMW", "text"); break;
					case PPGdsCls::eKind:  Set("GCKIND", "id");  break;
					case PPGdsCls::eGrade: Set("GCGRADE", "id"); break;
					case PPGdsCls::eAdd:   Set("GCADD", "id");   break;
					case PPGdsCls::eAdd2:  Set("GCADD2", "id");  break;
				}
				break;
			case PPUhttStoreSelDescr::attrTag:
				{
					Crit = "TAGVALUE";
					PPObjTag tag_obj;
					PPObjectTag tag_rec;
					if(rSdEntry.TagID && tag_obj.Fetch(rSdEntry.TagID, &tag_rec) > 0) {
						TagID = tag_rec.ID; // @v10.7.9
						Crit.Dot().Cat(tag_rec.Symb);
					}
					Part = "text";
				}
				break;
		}
		return ok;
	}
	SJson * CreateJSON() const
	{
		SJson * p_jsel = SJson::CreateObj();
		SString temp_buf, o_buf, txt_buf;
		p_jsel->InsertString("ID", temp_buf.Z().Cat(ID));
		p_jsel->InsertString("Attr", temp_buf.Z().Cat(Attr));
		p_jsel->InsertString("Clsf", temp_buf.Z().Cat(Clsf));
		p_jsel->InsertString("Title", (temp_buf = Title).Transf(CTRANSF_INNER_TO_OUTER).Escape());
		p_jsel->InsertString("Crit", (temp_buf = Crit).Escape());
		p_jsel->InsertString("Subcrit", temp_buf.Z());
		p_jsel->InsertString("Part", Part);
		//
		SJson * p_jsel_val_ary = SJson::CreateArr();
		if(Attr != PPUhttStoreSelDescr::attrName) {
			PPIDArray parent_id_list, named_id_list;
			for(uint j = 0, k = Values.getCount(); j < k; j++) {
				SJson * p_jsel_val = SJson::CreateObj();
				StrAssocArray::Item _val = Values.Get(j);
				p_jsel_val->InsertString("ID", temp_buf.Z().Cat(_val.Id));
				p_jsel_val->InsertString("Txt", (temp_buf = _val.Txt).Transf(CTRANSF_INNER_TO_OUTER).Escape()); // @v10.7.7 Escape()
				p_jsel_val->InsertString("PID", temp_buf.Z().Cat(_val.ParentId));
				named_id_list.add(_val.Id);
				if(_val.ParentId)
					parent_id_list.add(_val.ParentId);
				//
				json_insert_child(p_jsel_val_ary, p_jsel_val);
			}
			/* @v10.7.9 if(parent_id_list.getCount() && Attr == PPUhttStoreSelDescr::attrTag && Crit.Divide('.', o_buf, txt_buf) > 0) {
				PPID tag_id = 0;
				PPObjTag tag_obj;
				PPObjectTag tag_rec;
				Reference * p_ref = PPRef;
				if(tag_obj.FetchBySymb(txt_buf, &tag_id) > 0 && tag_obj.Fetch(tag_id, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_ENUM && tag_rec.TagEnumID) {
					parent_id_list.sortAndUndup();
					named_id_list.sortAndUndup();
					for(uint k = 0; k < parent_id_list.getCount(); k++) {
						const PPID parent_id = parent_id_list.get(k);
						if(named_id_list.addUnique(parent_id) > 0) {
							ReferenceTbl::Rec tag_item_rec;
							if(p_ref->GetItem(tag_rec.TagEnumID, parent_id, &tag_item_rec) > 0) {
								const PPID next_parent_id = tag_item_rec.Val2;
								assert(tag_item_rec.ObjID == parent_id);
								SJson * p_jsel_val = SJson::CreateObj();
								p_jsel_val->Insert("ID", json_new_string(temp_buf.Z().Cat(parent_id)));
								p_jsel_val->Insert("Txt", json_new_string((temp_buf = tag_item_rec.ObjName).Transf(CTRANSF_INNER_TO_OUTER)));
								p_jsel_val->Insert("PID", json_new_string(temp_buf.Z().Cat(next_parent_id)));
								json_insert_child(p_jsel_val_ary, p_jsel_val);
								named_id_list.add(parent_id);
								if(next_parent_id && !parent_id_list.lsearch(next_parent_id))
									parent_id_list.add(next_parent_id);
							}
						}
					}
				}
			}*/
		}
		p_jsel->Insert("Values", p_jsel_val_ary);
		//json_insert_child(p_jsel_ary, p_jsel);
		return p_jsel;
	}
	int    ID;
	int    Attr;
	int    Clsf;
	int    Type;
	PPID   TagID;
	SString Title;
	SString Crit;
	SString Subcrit;
	SString Part;
	StrAssocArray Values;
};

/*

select person by name("А") format tddo(fn, )
select tddo(fn, arg_list)

*/

int FASTCALL Backend_SelectObjectBlock::TrimResultListByPage(StrAssocArray & rList) const
{
	int    ok = 1;
	if(!Page.IsZero()) {
		uint   c = rList.getCount();
		uint   start_pos = 0;
		uint   end_pos = c;
		if(Page.low > 0)
			start_pos = (uint)Page.low - 1;
		if(Page.upp > 0)
			end_pos = (uint)Page.upp - 1;
		if(end_pos >= start_pos) {
			if(start_pos >= c)
				rList.Z();
			else {
				//
				// Сначала удаляем хвостовые элементы
				//
				if(c > end_pos) {
					do {
						rList.AtFree(--c);
					} while(c > end_pos);
				}
				//
				// Теперь удаляем элементы, предшествующие start_pos
				//
				c = start_pos;
				if(c) do {
					rList.AtFree(--c);
				} while(c);
			}
		}
	}
	else
		ok = -1;
	return ok;
}

int FASTCALL Backend_SelectObjectBlock::IncAndTestCounterForPage(int32 * pC) const
{
	int    ok = 1;
	assert(pC);
	(*pC)++;
	if(!Page.IsZero()) {
		if((*pC) < Page.low)
			ok = -1;
		else if((*pC) > Page.upp)
			ok = 0;
	}
	return ok;
}

int Backend_SelectObjectBlock::ProcessSelection_TSession(int _Op, const SCodepageIdent xmlCp, PPJobSrvReply & rResult)
{
	int    ok = -1;
	int    use_filt = 1;
	PPViewTSession * p_view = 0;
	TSessionTbl::Rec tses_rec;
	SString tses_text, temp_buf, o_buf, txt_buf;
	PPObjUhttStore store_obj;
	PPUhttStore store_rec;
	const PPID store_id = (P_TSesF && P_TSesF->UhttStoreID && store_obj.Search(P_TSesF->UhttStoreID, &store_rec) > 0) ? store_rec.ID : 0;
	THROW_MEM(SETIFZ(P_TSesObj, new PPObjTSession));
	if(IdList.getCount()) {
		for(uint i = 0; i < IdList.getCount(); i++) {
			if(P_TSesObj->Search(IdList.at(i), &tses_rec) > 0) {
				P_TSesObj->MakeName(&tses_rec, tses_text);
				THROW_SL(ResultList.Add(tses_rec.ID, 0, tses_text));
			}
		}
		use_filt = 0;
	}
	if(P_UuidList) {
		for(uint i = 0; i < P_UuidList->getCount(); i++) {
			const S_GUID & r_uuid = P_UuidList->at(i);
            if(P_TSesObj->SearchByGuid(r_uuid, &tses_rec) > 0) {
				P_TSesObj->MakeName(&tses_rec, tses_text);
				THROW_SL(ResultList.Add(tses_rec.ID, 0, tses_text));
            }
		}
		use_filt = 0;
	}
	if(use_filt) {
		THROW_MEM(p_view = new PPViewTSession);
		if(P_TSesF) {
			THROW(p_view->Init_(P_TSesF));
		}
		else {
			TSessionFilt filt;
			THROW(p_view->Init_(&filt));
		}
		TSessionViewItem item;
		for(p_view->InitIteration(0); p_view->NextIteration(&item) > 0;) {
			P_TSesObj->MakeName(&item, tses_text);
			THROW_SL(ResultList.Add(item.ID, 0, tses_text));
		}
	}
	ResultList.SortByText();
	//
	//
	//
	if(_Op == oGetTSessPlaceStatus) {
		const PPID qk_id = (P_TSesF && P_TSesF->QuotKindID) ? P_TSesF->QuotKindID : PPQUOTK_BASE;
		assert(qk_id); // @paranoic
		PPObjTSession::PlaceStatus status_item;
		TSCollection <PPObjTSession::PlaceStatus> status_list;
		if(P_LocTspF && P_LocTspF->PlaceCode[0] && !sstreqi_ascii(P_LocTspF->PlaceCode, "all")) {
            for(uint j = 0; j < ResultList.getCount(); j++) {
                const PPID tsess_id = ResultList.Get(j).Id;
                THROW(P_TSesObj->GetPlaceStatus(tsess_id, P_LocTspF->PlaceCode, qk_id, (store_id ? store_rec.LocID : 0), status_item));
                {
					PPObjTSession::PlaceStatus * p_new_item = new PPObjTSession::PlaceStatus(status_item);
					THROW_MEM(p_new_item);
					THROW_SL(status_list.insert(p_new_item));
                }
			}
		}
		else {
			PPCheckInPersonMngr ci_mgr;
			PPProcessorPacket::PlaceDescription pd;
			ProcessorPlaceCodeTemplate ppct;
			PPProcessorPacket prc_pack;
			StringSet ss_places;
			for(uint j = 0; j < ResultList.getCount(); j++) {
				const PPID tsess_id = ResultList.Get(j).Id;
				if(P_TSesObj->Search(tsess_id, &tses_rec) > 0 && P_TSesObj->PrcObj.GetPacket(tses_rec.PrcID, &prc_pack) > 0) {
					const uint pdc = prc_pack.Ext.GetPlaceDescriptionCount();
					if(pdc) {
						temp_buf.Z();
						LongArray place_pos_list; // Список позиций мест, добавленных на этой итерации
						LongArray goods_list;
						PPCheckInPersonArray cilist; // Список регистраций для сессии. По нему будем идентифицировать занятые места
						THROW(ci_mgr.GetList(PPCheckInPersonItem::kTSession, tsess_id, cilist));
						for(uint pdi = 0; pdi < pdc; pdi++) {
							if(prc_pack.Ext.GetPlaceDescription(pdi, pd) && ppct.Parse(pd.Range)) {
								ss_places.clear();
								ppct.Generate(ss_places);
								for(uint pp = 0; ss_places.get(&pp, temp_buf);) {
									PPObjTSession::PlaceStatus * p_new_item = new PPObjTSession::PlaceStatus;
									THROW_MEM(p_new_item);
									ProcessorPlaceCodeTemplate::NormalizeCode(temp_buf);
									p_new_item->PlaceCode = temp_buf;
									p_new_item->Status = 1;
									for(uint n = 0; p_new_item->Status == 1 && n < cilist.GetCount(); n++) {
										const PPCheckInPersonItem & r_ci = cilist.Get(n);
										ProcessorPlaceCodeTemplate::NormalizeCode(temp_buf = r_ci.PlaceCode);
										const int current_status = r_ci.GetStatus();
										if(temp_buf == p_new_item->PlaceCode && current_status != PPCheckInPersonItem::statusCanceled) {
											if(current_status == PPCheckInPersonItem::statusCheckedIn)
												p_new_item->Status = -2;
											else if(current_status == PPCheckInPersonItem::statusRegistered)
												p_new_item->Status = -1;
											else // Строго говоря, это состояние невозможно
												p_new_item->Status = -1;
											p_new_item->CipID = r_ci.ID;
											p_new_item->RegPersonID = r_ci.GetPerson();
										}
									}
									p_new_item->TSessID = tsess_id;
									p_new_item->GoodsID = pd.GoodsID;
									p_new_item->Descr = pd.Descr;
									p_new_item->Price = 0.0;
									if(pd.GoodsID)
										goods_list.add(pd.GoodsID);
									place_pos_list.add((long)status_list.getCount());
									THROW_SL(status_list.insert(p_new_item));
								}
							}
						}
						{
							goods_list.sortAndUndup();
							const QuotIdent qi(getcurdate_(), (store_id ? store_rec.LocID : prc_pack.Rec.LocID), qk_id, 0, 0);
							for(uint gi = 0; gi < goods_list.getCount(); gi++) {
								const PPID goods_id = goods_list.get(gi);
								double quot = 0.0;
								if(GObj.GetQuotExt(goods_id, qi, &quot, 1) > 0) {
									for(uint si = 0; si < place_pos_list.getCount(); si++) {
										PPObjTSession::PlaceStatus * p_item = status_list.at(place_pos_list.get(si));
										if(p_item->GoodsID == goods_id)
											p_item->Price = quot;
									}
								}
							}
						}
					}
				}
			}
		}
		if(OutFormat == fmtTddo) {
			THROW(Helper_ProcessTddo(0, &status_list, "UhttTSessPlaceStatusList", OutTemplate, rResult));
			ok = 1;
		}
		else if(OutFormat == fmtBinary) {
			ok = -1;
		}
		else if(OutFormat == fmtJson) {
			THROW(PPExportDL600DataToJson(DL600StrucName, 0, &status_list, ResultText));
			THROW(rResult.WriteString(ResultText));
			rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
			ok = 1;
		}
		else {
			THROW(PPExportDL600DataToBuffer("UhttTSessPlaceStatusList", &status_list, xmlCp, ResultText));
			THROW(rResult.WriteString(ResultText));
			rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
			ok = 1;
		}
	}
	else {
		if(OutFormat == fmtTddo) {
			THROW(Helper_ProcessTddo(0, &ResultList, "StrAssocArray", OutTemplate, rResult));
		}
		else if(OutFormat == fmtBinary) {
			TrimResultListByPage(ResultList);
			THROW_SL(ResultList.Write(rResult, 0));
			ok = 1;
		}
		else if(OutFormat == fmtJson) {
			StrAssocArray selector_list;
			int32 _c = 0;
			//if(p_view && p_view->GetSelectorListInfo(selector_list) && selector_list.getCount()) {
			if(store_id) {
				PPObjProcessor prc_obj;
				PPCheckInPersonMngr ci_mgr;
				ProcessorPlaceCodeTemplate ppct;
				SJson _jpack(SJson::tOBJECT);
				TSCollection <LocalSelectorDescr> sdescr_list;
				CALLPTRMEMB(p_view, GetSelectorListInfo(selector_list));
				//
				// Формирование списка селекторов
				//
				{
					PPUhttStoreSelDescr::Entry sd_entry;
					for(uint i = 0; i < selector_list.getCount(); i++) {
						StrAssocArray::Item si = selector_list.at_WithoutParent(i);
						if(p_view->GetSelectorListItem(si.Id, sd_entry)) {
							LocalSelectorDescr * p_sdescr = new LocalSelectorDescr(si);
							p_sdescr->Set(sd_entry);
							sdescr_list.insert(p_sdescr);
						}
					}
				}
				//
				// Формирование ответа
				//
				{
					SJson * p_jhdr = SJson::CreateObj();
					SJson * p_jsel_ary = SJson::CreateArr();
					SJson * p_jitem_list = SJson::CreateArr();
					_jpack.Insert("Header", p_jhdr);
					p_jhdr->InsertString("Class", temp_buf.Z().Cat(0L));
					p_jhdr->Insert("Selectors", p_jsel_ary);
					_jpack.Insert("Items", p_jitem_list);
					if(Separate == 1) {
						uint pos = 0;
						PPViewTSession::UhttStoreExt iter_ext;
						for(uint j = 0; j < ResultList.getCount(); j++) {
							const PPID tsess_id = ResultList.Get(j).Id;
							if(P_TSesObj->Search(tsess_id, &tses_rec) > 0) {
								if(p_view && p_view->GetUhttStoreExtension(tses_rec, iter_ext) > 0 && iter_ext.SfList.getCount()) {
									int sel_idx = 0;
									for(uint i = 0; i < iter_ext.SfList.getCount(); i++) {
										LocalSelectorDescr * p_sdescr = sdescr_list.at(sel_idx);
										if(p_sdescr) {
											pos = 0;
											const StrAssocArray::Item & r_item = iter_ext.SfList.Get(i);
											if(r_item.Id || r_item.Txt) {
												if(p_sdescr->Values.SearchByText(r_item.Txt, 1, &pos) == 0)
													p_sdescr->Values.AddFast(r_item.Id, r_item.ParentId, r_item.Txt);
											}
										}
										sel_idx++;
									}
								}
							}
						}
						for(uint i = 0, n = sdescr_list.getCount(); i < n; i++) {
							const LocalSelectorDescr * p_sdescr = sdescr_list.at(i);
							if(p_sdescr)
								json_insert_child(p_jsel_ary, p_sdescr->CreateJSON());
						}
					}
					else {
						PPProcessorPacket::ExtBlock tses_ext;
						PPProcessorPacket::ExtBlock prc_ext;
						PPProcessorPacket::PlaceDescription pd;
						StringSet ss_places(";");
						StringSet ss_places_busy(";");
						ProcessorTbl::Rec prc_rec;
						PPCheckInPersonArray ci_list;
						const LDATETIME _cdtm = getcurdatetime_();
						for(uint j = 0; j < ResultList.getCount(); j++) {
							const PPID tsess_id = ResultList.Get(j).Id;
							if(P_TSesObj->Search(tsess_id, &tses_rec) > 0 && prc_obj.Fetch(tses_rec.PrcID, &prc_rec) > 0) {
								tses_ext.destroy();
								P_TSesObj->GetExtention(tsess_id, &tses_ext);
								prc_obj.GetExtention(tses_rec.PrcID, &prc_ext);
								if(prc_ext.GetCipLockTimeout() > 0 && checkdate(tses_rec.StDt)) {
									LDATETIME stdtm;
									stdtm.Set(tses_rec.StDt, tses_rec.StTm);
									const long _d = diffdatetimesec(stdtm, _cdtm);
									if(_d < prc_ext.GetCipLockTimeout())
										tses_rec.Flags |= TSESF_CIPREGLOCKED;
								}
								{
									const uint pdc = prc_ext.GetPlaceDescriptionCount();
									for(uint pdi = 0; pdi < pdc; pdi++)
										if(prc_ext.GetPlaceDescription(pdi, pd) && ppct.Parse(pd.Range))
											ppct.Generate(ss_places);
								}
								SJson * p_jitem = SJson::CreateObj();
								//
								p_jitem->InsertString("ID", temp_buf.Z().Cat(tses_rec.ID));
								p_jitem->InsertString("PrcID", temp_buf.Z().Cat(tses_rec.PrcID));
								p_jitem->InsertString("Num", temp_buf.Z().Cat(tses_rec.Num));
								p_jitem->InsertString("Status", temp_buf.Z().Cat(tses_rec.Status));
								p_jitem->InsertString("Flags", temp_buf.Z().Cat(tses_rec.Flags));
								p_jitem->InsertString("StDate", temp_buf.Z().Cat(tses_rec.StDt, DATF_DMY|DATF_CENTURY));
								p_jitem->InsertString("StTime", temp_buf.Z().Cat(tses_rec.StTm, TIMF_HMS));
								p_jitem->InsertString("FinDate", temp_buf.Z().Cat(tses_rec.FinDt, DATF_DMY|DATF_CENTURY));
								p_jitem->InsertString("FinTime", temp_buf.Z().Cat(tses_rec.FinTm, TIMF_HMS));
								{
									temp_buf.Z();
									ObjTagItem tag;
									if(TagObj.FetchTag(tses_rec.ID, PPTAG_TSESS_DESCR, &tag) > 0)
										tag.GetStr(temp_buf);
									p_jitem->InsertString("Descr", temp_buf.Transf(CTRANSF_INNER_TO_OUTER).Escape());
								}
								// @v11.0.4 p_jitem->Insert("Memo", json_new_string((temp_buf = tses_rec.Memo).Transf(CTRANSF_INNER_TO_OUTER).Escape()));
								// @v11.0.4 {
								temp_buf.Z();
								if(tses_ext.GetExtStrData(PRCEXSTR_MEMO, temp_buf) > 0) {
									p_jitem->InsertString("Memo", temp_buf.Transf(CTRANSF_INNER_TO_OUTER).Escape());
								}
								// } @v11.0.4 
								temp_buf.Z();
								if(tses_ext.GetExtStrData(PRCEXSTR_DETAILDESCR, temp_buf) > 0) {
									p_jitem->InsertString("Detail", temp_buf.Transf(CTRANSF_INNER_TO_OUTER).Escape());
								}
								//uint   reg_count = 0;
								//uint   ci_count = 0;
								//uint   cancel_count = 0;
								PPCheckInPersonItem::Total rcount;
								ss_places.clear();
								ss_places_busy.clear();
								//
								p_jitem->InsertString("PrcName", (temp_buf = prc_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).Escape());
								p_jitem->InsertString("PrcSymb", (temp_buf = prc_rec.Code).Transf(CTRANSF_INNER_TO_OUTER).Escape());
								p_jitem->InsertString("CipMax", temp_buf.Z().Cat(prc_rec.CipMax));
								ci_list.Z();
								if(ci_mgr.GetList(PPCheckInPersonItem::kTSession, tses_rec.ID, ci_list) > 0) {
									//ci_list.Count(&reg_count, &ci_count, &cancel_count);
									ci_list.Count(rcount);
									const uint ci_count = ci_list.GetCount();
									for(uint cii = 0; cii < ci_count; cii++) {
										const  PPCheckInPersonItem & r_ci = ci_list.Get(cii);
										if(r_ci.PlaceCode[0])
											ss_places_busy.add(r_ci.PlaceCode);
									}
								}
								//
								p_jitem->InsertString("CipAvl", temp_buf.Z().Cat((long)prc_rec.CipMax - (long)rcount.RegCount));
								p_jitem->InsertString("Places", (temp_buf = ss_places.getBuf()).Transf(CTRANSF_INNER_TO_OUTER).Escape());
								p_jitem->InsertString("PlacesBusy", (temp_buf = ss_places_busy.getBuf()).Transf(CTRANSF_INNER_TO_OUTER).Escape());
								//
								json_insert_child(p_jitem_list, p_jitem);
							}
						}
					}
				}
				{
					THROW_SL(_jpack.ToStr(ResultText));
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
					ok = 1;
				}
			}
			else {
				THROW(PPExportDL600DataToJson(DL600StrucName, &ResultList, 0, ResultText));
				THROW(rResult.WriteString(ResultText));
				rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				ok = 1;
			}
		}
		else {
			THROW(PPExportDL600DataToBuffer("StrAssocArray", &ResultList, xmlCp, ResultText));
			THROW(rResult.WriteString(ResultText));
			rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
		}
	}
	CATCHZOK
	delete p_view;
	return ok;
}

int Backend_SelectObjectBlock::ProcessSelection_Goods(PPJobSrvReply & rResult)
{
	int    ok = -1;
	Reference * p_ref = PPRef;
	int    use_filt = 1;
	SString temp_buf, o_buf, txt_buf;
	Goods2Tbl::Rec goods_rec;
	if(IdList.getCount()) {
		for(uint i = 0; i < IdList.getCount(); i++) {
			if(GObj.Fetch(IdList.at(i), &goods_rec) > 0)
				THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
		}
		use_filt = 0;
	}
	if(SrchCodeList.getCount()) {
		for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
			if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
				if(o_buf.ToLong() == 2) {
					GoodsCodeSrchBlock blk;
					txt_buf.CopyTo(blk.Code, sizeof(blk.Code));
					blk.ArID = 0;
					if(GObj.SearchByCodeExt(&blk) > 0) {
						if(blk.Flags & GoodsCodeSrchBlock::fList) {
							for(uint i = 0; i < blk.P_List->getCount(); i++) {
								if(GObj.Fetch(blk.P_List->at(i).GoodsID, &goods_rec) > 0)
									THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
							}
						}
						else {
							THROW_SL(ResultList.Add(blk.Rec.ID, blk.Rec.ParentID, blk.Rec.Name));
						}
					}
				}
				else if(o_buf.ToLong() == 3) {
					ArGoodsCodeTbl::Rec arc_rec;
					if(GObj.P_Tbl->SearchByArCode(P_GoodsF->CodeArID, txt_buf, &arc_rec, &goods_rec) > 0) {
						THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
					}
				}
			}
		}
		use_filt = 0;
	}
	if(TagExistList.getCount()) {
		UintHashTable id_list;
		for(uint i = 0; i < TagExistList.getCount(); i++) {
			const PPID tag_id = TagExistList.get(i);
			p_ref->Ot.GetObjectList(PPOBJ_GOODS, tag_id, id_list);
		}
		for(ulong lp = 0; id_list.Enum(&lp);) {
			if(GObj.Fetch((long)lp, &goods_rec) > 0) {
				THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
			}
		}
		use_filt = 0;
	}
	if(use_filt) {
		if(OutFormat == fmtBinary) {
			THROW(GoodsIterator::GetListByFilt(P_GoodsF, &ResultList, 1));
			ResultList.SortByText();
			TrimResultListByPage(ResultList);
			THROW_SL(ResultList.Write(rResult, 0));
			ok = 1;
		}
		else {
			PPID   cls_id = 0;
			StrAssocArray selector_list;
			int32 _c = 0;
			GoodsIterator iter(P_GoodsF, GoodsIterator::ordByName);
			if(iter.GetSelectorListInfo(&cls_id, selector_list) && selector_list.getCount()) {
				// StrAssocArray CritList;
				SJson _jpack(SJson::tOBJECT);
				TSCollection <LocalSelectorDescr> sdescr_list;
				//
				// Формирование списка селекторов
				//
				{
					PPUhttStoreSelDescr::Entry sd_entry;
					for(uint i = 0; i < selector_list.getCount(); i++) {
						StrAssocArray::Item si = selector_list.at_WithoutParent(i);
						if(iter.GetSelectorListItem(si.Id, sd_entry)) {
							LocalSelectorDescr * p_sdescr = new LocalSelectorDescr(si);
							p_sdescr->Set(sd_entry);
							sdescr_list.insert(p_sdescr);
						}
					}
				}
				//
				// Формирование ответа
				//
				{
					SJson * p_jhdr = SJson::CreateObj();
					SJson * p_jsel_ary = SJson::CreateArr();
					SJson * p_jitem_list = SJson::CreateArr();
					//
					_jpack.Insert("Header", p_jhdr);
					p_jhdr->InsertString("Class", temp_buf.Z().Cat(cls_id));
					p_jhdr->Insert("Selectors", p_jsel_ary);
					_jpack.Insert("Items", p_jitem_list);
					//
					if(Separate == 1) {
						GoodsIterator::Ext iter_ext;
						uint pos = 0;
						while(iter.Next(&goods_rec, &iter_ext) > 0) {
							if(iter_ext.SfList.getCount()) {
								int sel_idx = 0;
								for(uint i = 0; i < iter_ext.SfList.getCount(); i++) {
									LocalSelectorDescr * p_sdescr = sdescr_list.at(sel_idx);
									if(p_sdescr) {
										pos = 0;
										StrAssocArray::Item _item = iter_ext.SfList.Get(i);
										if(_item.Id) {
											if(!p_sdescr->Values.Search(_item.Id, &pos))
												p_sdescr->Values.AddFast(_item.Id, _item.ParentId, _item.Txt);
										}
										else if(!isempty(_item.Txt)) {
											if(!p_sdescr->Values.SearchByText(_item.Txt, 1, &pos))
												p_sdescr->Values.AddFast(_item.Id, _item.ParentId, _item.Txt);
										}
									}
									sel_idx++;
								}
							}
						}
						// @v10.7.9 {
						{
							PPObjTag tag_obj;
							for(uint i = 0, n = sdescr_list.getCount(); i < n; i++) {
								LocalSelectorDescr * p_sdescr = sdescr_list.at(i);
								if(p_sdescr && p_sdescr->Attr == PPUhttStoreSelDescr::attrTag && p_sdescr->TagID) {
									PPObjectTag tag_rec;
									if(tag_obj.Fetch(p_sdescr->TagID, &tag_rec) > 0 && tag_rec.TagDataType == OTTYP_ENUM && tag_rec.TagEnumID) {
										const uint vc = p_sdescr->Values.getCount();
										for(uint vi = 0; vi < vc; vi++) {
											for(PPID par_id = p_sdescr->Values.Get(vi).ParentId; par_id;) {
												uint vpos = 0;
												if(p_sdescr->Values.Search(par_id, &vpos)) {
													par_id = p_sdescr->Values.Get(vpos).ParentId;
												}
												else {
													ReferenceTbl::Rec en_rec;
													if(p_ref->GetItem(tag_rec.TagEnumID, par_id, &en_rec) > 0) {
														p_sdescr->Values.AddFast(par_id, en_rec.Val2, en_rec.ObjName);
														par_id = en_rec.Val2;
													}
													else
														par_id = 0;
												}
											}
										}
										p_sdescr->Values.SortByTextInTreeOrder();
									}
								}
							}
						}
						// } @v10.7.9 
						{
							for(uint i = 0, n = sdescr_list.getCount(); i < n; i++) {
								const LocalSelectorDescr * p_sdescr = sdescr_list.at(i);
								if(p_sdescr)
									json_insert_child(p_jsel_ary, p_sdescr->CreateJSON());
							}
						}
					}
					else {
						GoodsIterator::Ext iter_ext;
						while(iter.Next(&goods_rec, &iter_ext) > 0) {
							SJson * p_jitem = SJson::CreateObj();
							p_jitem->InsertString("ID", temp_buf.Z().Cat(goods_rec.ID));
							p_jitem->InsertString("Name", (temp_buf = goods_rec.Name).Transf(CTRANSF_INNER_TO_OUTER).Escape());
							p_jitem->InsertString("CurID", temp_buf.Z().Cat(iter_ext.CurID));
							p_jitem->InsertString("Price", temp_buf.Z().Cat(iter_ext.Price, SFMT_MONEY));
							p_jitem->InsertString("Rest", temp_buf.Z().Cat(iter_ext.Rest, MKSFMTD(0, 3, 0)));
							p_jitem->InsertString("PriceDt", temp_buf.Z().Cat(iter_ext.PriceDtm.d, DATF_DMY).Escape());
							p_jitem->InsertString("PriceTm", temp_buf.Z().Cat(iter_ext.PriceDtm.t, TIMF_HM).Escape());
							json_insert_child(p_jitem_list, p_jitem);
						}
					}
				}
				{
					THROW_SL(_jpack.ToStr(ResultText));
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
					ok = 1;
				}
			}
			else {
				SCycleTimer timer(15000);
				while(iter.Next(&goods_rec) > 0) {
					if(timer.Check(0)) {
						const IterCounter & r_cntr = iter.GetIterCounter();
						temp_buf.Z().CatPercentMsg(r_cntr, r_cntr.GetTotal(), 0);
						rResult.SendInformer(temp_buf);
					}
					int _t = IncAndTestCounterForPage(&_c);
					if(_t > 0) {
						THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
					}
					else if(!_t)
						break;
				}
			}
		}
	}
	/*
	if(ObjType == PPOBJ_GOODSARCODE) {
		ResultText = 0;
		for(uint i = 0; i < ResultList.getCount(); i++) {
			UhttGoodsArCodeIdent ident;
			ident.GoodsID = ResultList.at(i).Id;
			ident.ArID = P_GoodsF->CodeArID;
			STRNSCPY(ident.Name, ResultList.at(i).Txt);
			if(GObj.P_Tbl->GetArCode(ident.ArID, ident.GoodsID, temp_buf) > 0) {
				temp_buf.CopyTo(ident.Code, sizeof(ident.Code));
				THROW(PPExportDL600DataToBuffer("UhttGoodsArCode", &ident, temp_buf));
				temp_buf.Chomp();
				if(i)
					ResultText.Space();
				ResultText.Cat(temp_buf);
			}
		}
		rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
		THROW(rResult.WriteString(ResultText));
		ok = 2;
	}
	*/
	CATCHZOK
	return ok;
}

int Backend_SelectObjectBlock::Execute(PPJobSrvReply & rResult)
{
	int     ok = 1;
	Reference * p_ref = PPRef;
	PPObjBill * p_bobj = BillObj;
	int    done = 0; // Признак того, что результирующий список сформирован
	int    use_filt = 0;
	PPGta  gta_blk;
	PPID   temp_id = 0;
	SString temp_buf;
	SString o_buf;
	SString txt_buf;
	ResultList.Z();
	ResultText.Z();
	const PPThreadLocalArea & r_tla_c = DS.GetConstTLA();
	const SCodepageIdent _xmlcp((OutFormat == fmtXmlUtf8) ? cpUTF8 : cpANSI);
	if(r_tla_c.GlobAccID && r_tla_c.State & PPThreadLocalArea::stExpTariffTa) {
		gta_blk.GlobalUserID = r_tla_c.GlobAccID;
		gta_blk.ObjId.Obj = ObjType;
		switch(Operator) {
			case oSet:
				if(ObjType == PPOBJ_DL600DATA) {
					// @todo этот случай обрабатывается отдельно
				}
				else if(ObjType == PPOBJ_QUOT2) {
					gta_blk.Op = GTAOP_OBJADD;
				}
				else if(ObjType == PPOBJ_GOODSARCODE) {
					gta_blk.Op = GTAOP_OBJADD;
				}
				break;
			case oSelect:        gta_blk.Op = GTAOP_OBJGET; break;
			case oCCheckCreate:  gta_blk.Op = GTAOP_CCHECKCREATE; break;
			case oSCardWithdraw: gta_blk.Op = GTAOP_SCARDWITHDRAW; break;
			case oSCardDeposit:  gta_blk.Op = GTAOP_SCARDDEPOSIT; break;
			case oSCardRest:     gta_blk.Op = GTAOP_OBJGET; break;
			case oBillFinish:    gta_blk.Op = GTAOP_BILLCREATE; break;
		}
		if(p_bobj) {
			p_bobj->InitGta(gta_blk);
			if(gta_blk.Quot != 0.0) {
				THROW_PP((gta_blk.SCardRest + gta_blk.SCardMaxCredit) > 0.0, PPERR_GTAOVERDRAFT);
			}
		}
	}
	switch(ObjType) {
		case PPOBJ_DL600DATA:
			if(Operator == oSet) {
				// @Muxa {
				SJson * p_json_doc = SJson::Parse(JSONString);
				THROW_SL(p_json_doc);
				{
					// is_local_err нужна для того, чтоб аккуратно разрушить p_json_doc до перескока на CATCH
					bool   is_local_err = false;
					DlContext * p_ctx = DS.GetInterfaceContext(PPSession::ctxtExportData);
					if(p_ctx) {
						DlRtm * p_rtm = p_ctx->GetRtm(DL600StrucName.cptr());
						if(p_rtm) {
							long id = 0;
							if(p_rtm->SetByJSON(p_json_doc, id))
								rResult.SetString(temp_buf.Z().Cat(id));
							else
								is_local_err = true;
						}
						else
							is_local_err = true;
					}
					else
						is_local_err = true;
					ZDELETE(p_json_doc);
					THROW(is_local_err);
				}
				// } @Muxa
			}
			else {
				if(IdList.getCount()) {
					THROW(DL600StrucName.NotEmptyS());
					if(Operator == oSelect && OutFormat == fmtTddo) {
						//
						// Специальный случай: запрашивается DL600-структура и формат вывода TDDO -
						// сразу формируем TDDO-результат по первому найденному идентификатору.
						//
						/*
						StringSet in_line(',', OutTemplate);
						StringSet ext_param_list;
						SString file_name;
						const char * p_data_name = DL600StrucName;
						uint   ip = 0;
						THROW_PP(in_line.get(&ip, file_name), PPERR_CMDSEL_MISSTDDOTEMPLATE);
						THROW(Tddo::LoadFile(file_name.Strip(), temp_buf));
						while(in_line.get(&ip, txt_buf))
							ext_param_list.add(txt_buf.Strip());
						{
							Tddo t;
							t.SetInputFileName(file_name); // @v7.5.10
							THROW(t.Process(DL600StrucName, temp_buf, IdList.at(0), 0, &ext_param_list, rResult));
						}
						rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
						*/
						THROW(Helper_ProcessTddo(IdList.at(0), 0, DL600StrucName, OutTemplate, rResult));
						done = 1;
					}
					else {
						for(uint i = 0; i < IdList.getCount(); i++) {
							THROW(PPExportDL600DataToBuffer(DL600StrucName, IdList.at(i), _xmlcp, temp_buf));
							temp_buf.Chomp();
							if(i)
								ResultText.Space();
							ResultText.Cat(temp_buf);
						}
						rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
						THROW(rResult.WriteString(ResultText));
					}
					ok = 2;
				}
			}
			break;
		case PPOBJ_CCHECK:
			// @Muxa {
			{
				PPGlobalAccRights rights_blk(PPTAG_GUA_SCARDRIGHTS);
				if(rights_blk.IsAllow(PPGlobalAccRights::fOperation)) {
					if(Operator == oCCheckCreate) {
						PPID   cc_id = 0;
						SelectObjectBlock::DistribCCheck::Header hdr;
						THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
						THROW_MEM(SETIFZ(P_DCc, new SelectObjectBlock::DistribCCheck));
						hdr.PosNodeID = P_SetBlk->U.C.PosNodeID;
						hdr.LocID = P_SetBlk->U.C.LocID;
						hdr.SCardID = P_SetBlk->U.C.SCardID;
						hdr.Amount = P_SetBlk->U.C.Amount;
						hdr.Discount = P_SetBlk->U.C.Discount;
						THROW(P_DCc->Begin(&cc_id, hdr));
						rResult.SetString(temp_buf.Z().Cat(cc_id));
					}
					else if(Operator == oCCheckAddLine) {
						SelectObjectBlock::DistribCCheck::Line ln;
						THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
						THROW_MEM(SETIFZ(P_DCc, new SelectObjectBlock::DistribCCheck));
						ln.GoodsID = P_SetBlk->U.C.GoodsID;
						ln.Qtty = P_SetBlk->U.C.Qtty;
						ln.Price = P_SetBlk->U.C.Price;
						ln.Discount = P_SetBlk->U.C.Discount;
						THROW(P_DCc->AddLine(P_SetBlk->U.C.ID, ln));
						rResult.SetAck();
					}
					else if(Operator == oCCheckFinish) {
						PPID   cc_id = 0;
						THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
						THROW_MEM(SETIFZ(P_DCc, new SelectObjectBlock::DistribCCheck));
						THROW(P_DCc->Finish(P_SetBlk->U.C.ID, &cc_id));
						rResult.SetString(temp_buf.Z().Cat(cc_id));
					}
				}
				else {
					ok = PPSetError(PPERR_NORIGHTS, DS.GetTLA().GlobAccName);
				}
			}
			// }
			break;
		case PPOBJ_BILL:
			{
				if(Operator == oSelect) {
					int32 _c = 0;
					if(IdList.getCount()) {
						BillTbl::Rec bill_rec;
						for(uint i = 0; i < IdList.getCount(); i++) {
							if(p_bobj->Search(IdList.get(i), &bill_rec) > 0) {
								int _t = IncAndTestCounterForPage(&_c);
								if(_t > 0) {
									PPObjBill::MakeCodeString(&bill_rec, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
									ResultList.Add(bill_rec.ID, temp_buf);
								}
								else if(!_t)
									break;
							}
						}
					}
					else {
						PPViewBill bview;
						BillViewItem bitem;
						SETIFZ(P_BillF, new BillFilt);
						THROW(bview.Init_(P_BillF));
						for(bview.InitIteration(PPViewBill::OrdByDate); bview.NextIteration(&bitem) > 0;) {
							int _t = IncAndTestCounterForPage(&_c);
							if(_t > 0) {
								PPObjBill::MakeCodeString(&bitem, PPObjBill::mcsAddOpName|PPObjBill::mcsAddLocName, temp_buf);
								ResultList.Add(bitem.ID, temp_buf);
							}
							else if(!_t)
								break;
						}
					}
				}
				else if(Operator == oBillCreate) {
					THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
					{
						long   temp_bill_id = 0;
						PPBillPacket pack;
						PPBillPacket::SetupObjectBlock sob;
						PPObjCurrency cur_obj;
						PPCurrency cur_rec;
						PPOprKind op_rec;
						LocationTbl::Rec loc_rec;
						THROW(GetOpData(P_SetBlk->U.B.OpID, &op_rec) > 0);
						THROW(PsnObj.LocObj.Fetch(P_SetBlk->U.B.LocID, &loc_rec) > 0);
						if(op_rec.OpTypeID == PPOPT_DRAFTTRANSIT) {
							THROW_PP_S(loc_rec.Type == LOCTYP_ADDRESS, PPERR_INCLOCTYPE, PsnObj.LocObj.MakeCodeString(&loc_rec, 0, temp_buf));
							/*
							if(P_SetBlk->B.OwnerID) {
							}
							*/
						}
						else {
							THROW_PP_S(loc_rec.Type == LOCTYP_WAREHOUSE, PPERR_INCLOCTYPE, PsnObj.LocObj.MakeCodeString(&loc_rec, 0, temp_buf));
						}
						THROW(pack.CreateBlank2(P_SetBlk->U.B.OpID, P_SetBlk->U.B.Dt, P_SetBlk->U.B.LocID, 1));
						THROW(pack.SetupObject(P_SetBlk->U.B.ArID, sob));
						if(P_SetBlk->U.B.DlvrLocID) {
							THROW(PsnObj.LocObj.Fetch(P_SetBlk->U.B.DlvrLocID, &loc_rec) > 0);
							THROW_PP_S(loc_rec.Type == LOCTYP_ADDRESS, PPERR_INCLOCTYPE, PsnObj.LocObj.MakeCodeString(&loc_rec, 0, temp_buf));
							if(pack.Rec.Object == 0 && loc_rec.OwnerID && op_rec.AccSheetID) {
								PPID   ar_id = 0;
								ArObj.P_Tbl->PersonToArticle(loc_rec.OwnerID, op_rec.AccSheetID, &ar_id);
								if(ar_id)
									THROW(pack.SetupObject(ar_id, sob));
							}
							THROW_PP_S((loc_rec.Flags & LOCF_STANDALONE) || (loc_rec.OwnerID == ObjectToPerson(pack.Rec.Object, 0)), PPERR_DLVRADDNOTOWNEDBYBILLAR,
								PsnObj.LocObj.MakeCodeString(&loc_rec, 0, temp_buf));
							pack.SetFreight_DlvrAddrOnly(P_SetBlk->U.B.DlvrLocID);
						}
						temp_buf = P_SetBlk->U.B.Code;
						if(temp_buf.NotEmptyS()) {
							temp_buf.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(pack.Rec.Code, sizeof(pack.Rec.Code));
						}
						if(P_SetBlk->U.B.CurID) {
							THROW(cur_obj.Fetch(P_SetBlk->U.B.CurID, &cur_rec) > 0);
							pack.Rec.CurID = P_SetBlk->U.B.CurID;
						}
						if(P_SetBlk->U.B.AgentID) {
							ArticleTbl::Rec ar_rec;
							THROW(ArObj.Fetch(P_SetBlk->U.B.AgentID, &ar_rec) > 0);
						}
						temp_buf = P_SetBlk->Memo;
						if(temp_buf.NotEmptyS()) {
							// @v11.1.12 temp_buf.Transf(CTRANSF_OUTER_TO_INNER).CopyTo(pack.Rec.Memo, sizeof(pack.Rec.Memo));
							pack.SMemo = temp_buf.Transf(CTRANSF_OUTER_TO_INNER); // @v11.1.12
						}
						pack.GenerateGuid(0);
						THROW(p_bobj->GetCrBillEntry(temp_bill_id, &pack));
						rResult.SetString(temp_buf.Z().Cat(temp_bill_id));
					}
				}
				else if(Operator == oBillAddLine) {
					THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
					THROW_PP(P_SetBlk->U.B.ID, PPERR_CMDSEL_ARGABS_ID);
					THROW_PP(P_SetBlk->U.B.GoodsID, PPERR_CMDSEL_ARGABS_GOODS);
					{
						long   temp_bill_id = P_SetBlk->U.B.ID;
						PPBillPacket pack;
						THROW(p_bobj->GetCrBillEntry(temp_bill_id, &pack));
						{
							PPTransferItem ti(&pack.Rec, TISIGN_UNDEF);
							double val;
							THROW(ti.SetupGoods(P_SetBlk->U.B.GoodsID, 0));
							val = R6(P_SetBlk->U.B.Qtty);
							THROW_PP_S(val >= 0.0 && val <= 1.E6, PPERR_CMDSEL_ARGINV_QTTY, temp_buf.Z().Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
							// @v7.9.2 ti.Quantity_ = (val > 0.0) ? val : 1.0;
							ti.Quantity_ = val; // @v7.9.2
							val = R5(P_SetBlk->U.B.Price);
							THROW_PP_S(val >= 0.0 && val <= 1.E9, PPERR_CMDSEL_ARGINV_PRICE, temp_buf.Z().Cat(val, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
							ti.Price = val;
							if(P_SetBlk->U.B.Amount > 0.0) {
								if(ti.Price == 0.0) {
									ti.Price = R5(fabs(P_SetBlk->U.B.Amount / ti.Quantity_));
								}
								else {
									THROW_PP(fabs(ti.Price - fabs(P_SetBlk->U.B.Amount / ti.Quantity_)) < 1.0E-6, PPERR_CMDSEL_ASSERTAMTPRICE);
								}
							}
							{
								temp_buf = P_SetBlk->U.B.Serial;
								if(P_SetBlk->U.B.TSessID) {
									THROW(pack.AddTSessCip(P_SetBlk->U.B.TSessID, P_SetBlk->U.B.PlaceCode, 0));
									if(temp_buf.IsEmpty()) {
										temp_buf.Cat("TSES#").Cat(P_SetBlk->U.B.TSessID);
										if(P_SetBlk->U.B.PlaceCode[0])
											temp_buf.Colon().Cat("PLACE#").Cat(P_SetBlk->U.B.PlaceCode);
									}
								}
								THROW(pack.LoadTItem(&ti, 0, temp_buf.Strip().ToOem()));
							}
						}
						THROW(p_bobj->SetCrBillEntry(temp_bill_id, &pack));
					}
					rResult.SetAck();
				}
				else if(Operator == oBillFinish) {
					THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
					THROW_PP(P_SetBlk->U.B.ID, PPERR_CMDSEL_ARGABS_ID);
					{
						long   temp_bill_id = P_SetBlk->U.B.ID;
						PPBillPacket pack;
						THROW(p_bobj->GetCrBillEntry(temp_bill_id, &pack));
						THROW(p_bobj->__TurnPacket(&pack, 0, 0, 1));
						THROW(p_bobj->SetCrBillEntry(temp_bill_id, 0)); // Удаляем экземпляр документа из кэша
						P_SetBlk->U.B.ID = pack.Rec.ID;
						rResult.SetString(temp_buf.Z().Cat(pack.Rec.ID));
					}
				}
				else if(Operator == oDraftTransitGoodsRest) {
					THROW_PP(P_DtGrF, PPERR_EMPTYPPOBJECT);
					THROW_PP(P_DtGrF->GoodsID, PPERR_CMDSEL_ARGABS_GOODS);
					THROW_PP(P_DtGrF->LocID, PPERR_CMDSEL_ARGABS_LOC);
					{
						double rest = 0.0;
						PPID   rest_op_id = 0;
						PPID   order_op_id = 0;
						PPOprKind op_rec;
						if(GetOpBySymb("GOODSREST", &op_rec) > 0)
							rest_op_id = op_rec.ID;
						if(GetOpBySymb("DRAFTORDER", &op_rec) > 0)
							order_op_id = op_rec.ID;
						if(rest_op_id)
							p_bobj->CalcDraftTransitRest(rest_op_id, order_op_id, P_DtGrF->GoodsID, P_DtGrF->LocID, 0 /* flags */, &rest, 0);
						rResult.SetString(temp_buf.Z().Cat(rest, MKSFMTD(0, 6, NMBF_NOTRAILZ)));
					}
				}
				else if(Operator == oDraftTransitGoodsRestList) {
					THROW_PP(P_DtGrF, PPERR_EMPTYPPOBJECT);
					THROW_PP(P_DtGrF->GoodsID, PPERR_CMDSEL_ARGABS_GOODS);
					{
						TSVector <UhttGoodsRestVal> r_list; // @v9.8.11 TSArray-->TSVector
						PPID   rest_op_id = 0;
						PPID   order_op_id = 0;
						PPOprKind op_rec;
						if(GetOpBySymb("GOODSREST", &op_rec) > 0)
							rest_op_id = op_rec.ID;
						if(GetOpBySymb("DRAFTORDER", &op_rec) > 0)
							order_op_id = op_rec.ID;
						if(rest_op_id) {
							// get locations
							PPIDArray loc_id_ary;
							CpTransfTbl::Key1 k;
							MEMSZERO(k);
							k.GoodsID = P_DtGrF->GoodsID;
							BExtQuery q(p_bobj->P_CpTrfr, 1);
							q.selectAll().where(p_bobj->P_CpTrfr->GoodsID == P_DtGrF->GoodsID);
							for(q.initIteration(false, &k, spGe); q.nextIteration() > 0;) {
								CpTransfTbl::Rec cpt_rec;
								p_bobj->P_CpTrfr->copyBufTo(&cpt_rec);
								loc_id_ary.addUnique(cpt_rec.LocID);
							}
							//
							for(uint i = 0; i < loc_id_ary.getCount(); i++) {
								UhttGoodsRestVal gr_val;
								gr_val.GoodsID = P_DtGrF->GoodsID;
								gr_val.LocID = loc_id_ary.at(i);
								if(gr_val.LocID > 0) {
									p_bobj->CalcDraftTransitRest(rest_op_id, order_op_id, gr_val.GoodsID, gr_val.LocID, 0 /* flags */, &gr_val.Rest, &gr_val.RestBillDt);
									r_list.insert(&gr_val);
								}
							}
							if(OutFormat == fmtBinary) {
								THROW_SL(rResult.Write(&r_list, 0));
							}
							else if(OutFormat == fmtTddo) {
								THROW(Helper_ProcessTddo(0, &r_list, "UhttDraftTransitGoodsRestList", OutTemplate, rResult));
							}
							else {
								THROW(PPExportDL600DataToBuffer("UhttDraftTransitGoodsRestList", &r_list, _xmlcp, ResultText));
								THROW(rResult.WriteString(ResultText));
								rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
							}
							ok = 2;
						}
					}
				}
			}
			break;
		case PPOBJ_TAGVALUE:
			THROW_PP(P_TagBlk, PPERR_CMDSEL_EMPTYTAGCRIT);
			THROW_PP(IdList.getCount(), PPERR_CMDSEL_EMPTYTAGGEDOBJLIST);
			{
				PPObjectTag tag_rec;
				ObjTagItem tag;
				THROW(TagObj.Fetch(P_TagBlk->TagID, &tag_rec) > 0);
				THROW_PP_S(P_TagBlk->ObjType == tag_rec.ObjTypeID, PPERR_CMDSEL_MISMTAGGEDOBJTYPE, tag_rec.Name);
				if(oneof3(Operator, oSetObjectTag, oIncObjectTag, oDecObjectTag)) {
					double incr = 0.0;
					if(oneof2(Operator, oIncObjectTag, oDecObjectTag)) {
						THROW_PP_S(oneof3(tag_rec.TagDataType, OTTYP_NUMBER, OTTYP_INT, OTTYP_DATE), PPERR_CMDSEL_INCNOTNUMERICTAG, tag_rec.Name);
						if(P_TagBlk->Value.NotEmptyS()) {
							incr = P_TagBlk->Value.ToReal();
							THROW_PP_S(incr > 0.0, PPERR_CMDSEL_INCTAGINVVAL, P_TagBlk->Value);
						}
						else {
							incr = 1.0;
						}
						if(Operator == oDecObjectTag)
							incr = -incr;
					}
					{
						PPTransaction tra(1);
						THROW(tra);
						for(uint i = 0; i < IdList.getCount(); i++) {
							const PPID obj_id = IdList.get(i);
							if(obj_id) {
								if(Operator == oSetObjectTag) {
									if(P_TagBlk->Value.NotEmptyS()) {
										THROW(tag.SetStr(P_TagBlk->TagID, P_TagBlk->Value));
										THROW(p_ref->Ot.PutTag(P_TagBlk->ObjType, obj_id, &tag, 0));
									}
									else {
										THROW(p_ref->Ot.PutTag(P_TagBlk->ObjType, obj_id, 0, 0));
									}
								}
								else if(oneof2(Operator, oIncObjectTag, oDecObjectTag)) {
									if(p_ref->Ot.GetTag(P_TagBlk->ObjType, obj_id, P_TagBlk->TagID, &tag) > 0) {
										THROW(tag.AddReal(incr) > 0);
										THROW(p_ref->Ot.PutTag(P_TagBlk->ObjType, obj_id, &tag, 0));
									}
									else {
										THROW(tag.Init(P_TagBlk->TagID));
										THROW(tag.AddReal(incr) > 0);
										THROW(p_ref->Ot.PutTag(P_TagBlk->ObjType, obj_id, &tag, 0));
									}
								}
							}
						}
						THROW(tra.Commit());
					}
					rResult.SetAck();
					ok = 1;
				}
				else if(Operator == oGetObjectTag) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						const PPID obj_id = IdList.get(i);
						if(obj_id && p_ref->Ot.GetTag(P_TagBlk->ObjType, obj_id, P_TagBlk->TagID, &tag) > 0) {
							tag.GetStr(temp_buf);
							THROW_SL(ResultList.Add(obj_id, 0, temp_buf));
						}
					}
				}
			}
			break;
		case PPOBJ_QUOT2:
			if(Operator == oSelect) {
				PPViewQuot qview;
				PPQuotItemArray temp_list;
				if(!P_QF) {
					P_QF = new LocalQuotFilt;
					P_QF->Flags |= QuotFilt::fActualOnly;
				}
				P_QF->Flags |= QuotFilt::fListOnly;
				THROW(qview.Init_(P_QF));
				const PPQuotItemArray * p_qlist = qview.GetQList();
				if(p_qlist && (!Page.IsZero() || P_QF->GoodsSubText.NotEmpty() || (P_QF->LocalFlags & P_QF->lfNonZeroDraftRestOnly))) {
					//
					// @todo Здесь следует скопировать в temp_list только необходимые элементы.
					// (а не копировать все и вырезать не нужное).
					//
					temp_list = *p_qlist;
					uint   c = temp_list.getCount();
					if(P_QF->GoodsSubText.NotEmpty()) {
						if(c) do {
							PPQuotItem_ & r_item = temp_list.at(--c);
							Goods2Tbl::Rec goods_rec;
							if(GObj.Fetch(r_item.GoodsID, &goods_rec) > 0) {
								if(!ExtStrSrch(goods_rec.Name, P_QF->GoodsSubText, 0))
									temp_list.atFree(c);
							}
							else
								temp_list.atFree(c);
						} while(c);
						c = temp_list.getCount();
					}
					if(P_QF->LocalFlags & P_QF->lfNonZeroDraftRestOnly) {
						if(c && p_bobj) {
							PPOprKind op_rec;
							const PPID rest_op_id = (GetOpBySymb("GOODSREST", &op_rec) > 0) ? op_rec.ID : 0;
							const PPID order_op_id = (GetOpBySymb("DRAFTORDER", &op_rec) > 0) ? op_rec.ID : 0;
							if(rest_op_id) {
								do {
									PPQuotItem_ & r_item = temp_list.at(--c);
									double rest = 0.0;
									p_bobj->CalcDraftTransitRest(rest_op_id, order_op_id, r_item.GoodsID, r_item.LocID, 0 /* flags */, &rest, 0);
									if(rest <= 0.0) {
										temp_list.atFree(c);
									}
								} while(c);
							}
						}
						c = temp_list.getCount();
					}
					if(!Page.IsZero()) {
						uint   start_pos = 0;
						uint   end_pos = c;
						if(Page.low > 0)
							start_pos = (uint)Page.low - 1;
						if(Page.upp > 0)
							end_pos = (uint)Page.upp - 1;
						if(end_pos >= start_pos) {
							if(start_pos >= c)
								temp_list.clear();
							else {
								//
								// Сначала удаляем хвостовые элементы
								//
								if(c > end_pos) {
									do {
										temp_list.atFree(--c);
									} while(c > end_pos);
								}
								//
								// Теперь удаляем элементы, предшествующие start_pos
								//
								c = start_pos;
								if(c) do {
									temp_list.atFree(--c);
								} while(c);
							}
						}
					}
					p_qlist = &temp_list;
				}
				if(OutFormat == fmtBinary) {
					THROW_SL(rResult.Write(p_qlist, 0));
				}
				else if(OutFormat == fmtTddo) {
					THROW(Helper_ProcessTddo(0, const_cast<PPQuotItemArray *>(p_qlist), "QuotArray", OutTemplate, rResult)); // @badcast
				}
				else if(OutFormat == fmtJson) {
					PPExportDL600DataToJson(DL600StrucName, 0, const_cast<PPQuotItemArray *>(p_qlist), ResultText); // @badcast
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				else {
					THROW(PPExportDL600DataToBuffer("QuotArray", const_cast<PPQuotItemArray *>(p_qlist), _xmlcp, ResultText)); // @badcast
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				done = 1;
				ok = 2;
			}
			else if(Operator == oGetGoodsMatrix) {
				PPIDArray mtx_list;
				GObj.P_Tbl->GetMatrix(P_QF->LocList, 0, &mtx_list);
				if(OutFormat == fmtBinary) {
					THROW_SL(rResult.Write(&mtx_list, 0));
				}
				else if(OutFormat == fmtTddo) {
					THROW(Helper_ProcessTddo(0, &mtx_list, "LongArray", OutTemplate, rResult));
				}
				else if(OutFormat == fmtJson) {
					PPExportDL600DataToJson(DL600StrucName, 0, &mtx_list, ResultText);
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				else {
					THROW(PPExportDL600DataToBuffer("LongArray", &mtx_list, _xmlcp, ResultText));
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				ok = 2;
			}
			else if(Operator == oSet) {
				PPQuot q;
				THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
				SETIFZ(P_Qc, new Quotation2Core);
				q.Kind  = P_SetBlk->U.Q.QuotKindID;
				q.SellerArID = P_SetBlk->U.Q.SellerID;
				q.LocID = P_SetBlk->U.Q.LocID;
				q.ArID  = P_SetBlk->U.Q.BuyerID;
				q.CurID = P_SetBlk->U.Q.CurID;
				q.GoodsID = P_SetBlk->U.Q.GoodsID;
				q.Quot = P_SetBlk->U.Q.Quot;
				THROW(P_Qc->Set(q, 0, 1, 1));
				rResult.SetAck();
				ok = 1;
			}
			break;
		case PPOBJ_GOODSARCODE:
			if(Operator == oSelect) {
				THROW_PP(P_GoodsF->CodeArID, PPERR_CMDSEL_UNIDENTSELLER);
				// no break: управление передается в 'case PPOBJ_GOODS'
			}
			else {
				if(Operator == oSet) {
					THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
					THROW_PP(P_SetBlk->U.AC.GoodsID, PPERR_CMDSEL_UNIDENTGOODS);
					THROW_PP(P_SetBlk->U.AC.SellerID, PPERR_CMDSEL_UNIDENTSELLER);
					THROW(GObj.P_Tbl->SetArCode(P_SetBlk->U.AC.GoodsID, P_SetBlk->U.AC.SellerID, P_SetBlk->U.AC.Code, 1));
					rResult.SetAck();
					ok = 1;
				}
				break;
			}
			// no break: для Operator == oSelect управление передается в 'case PPOBJ_GOODS'
		case PPOBJ_GOODS:
			if(Operator == oSelect) {
				int    r = ProcessSelection_Goods(rResult);
				THROW(r);
				if(r > 0)
					done = 1;
				ok = 1;
			}
			break;
		case PPOBJ_GOODSGROUP:
			{
				Goods2Tbl::Rec goods_rec;
				use_filt = 1;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(GgObj.Fetch(IdList.at(i), &goods_rec) > 0)
							THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								BarcodeTbl::Rec bc_rec;
								if(GgObj.SearchCode(txt_buf, &bc_rec) > 0 && GgObj.Fetch(bc_rec.GoodsID, &goods_rec) > 0)
									THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
							}
							else if(_ep == 1) {
								if(GgObj.SearchByName(txt_buf, &temp_id, &goods_rec) > 0)
									THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					GoodsGroupIterator iter(P_GgF ? P_GgF->ParentID : 0);
					PPID   id = 0;
					while(iter.Next(&id, temp_buf) > 0) {
						if(GgObj.Fetch(id, &goods_rec) > 0) {
							if(P_GgF && P_GgF->Name.NotEmpty()) {
								if(P_GgF->Flags & LocalGoodsGroupFilt::fSubName) {
									if((temp_buf = goods_rec.Name).Search(P_GgF->Name, 0, 1, 0))
										THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
								}
								else if(P_GgF->Name.IsEqNC(goods_rec.Name))
									THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
							}
							else
								THROW_SL(ResultList.Add(goods_rec.ID, goods_rec.ParentID, goods_rec.Name));
						}
					}
				}
			}
			break;
		case PPOBJ_BRAND:
			{
				PPObjBrand brand_obj;
				use_filt = 1;
				if(IdList.getCount()) {
					PPBrand brand_rec;
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(brand_obj.Fetch(IdList.at(i), &brand_rec) > 0)
							THROW_SL(ResultList.Add(brand_rec.ID, 0, brand_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							if(o_buf.ToLong() == 1) {
								Goods2Tbl::Rec temp_brand_rec;
								if(brand_obj.SearchByName(txt_buf, &temp_id, &temp_brand_rec) > 0)
									THROW_SL(ResultList.Add(temp_brand_rec.ID, 0, temp_brand_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					union {
						Goods2Tbl::Key2 k2;
						Goods2Tbl::Key4 k4;
					} k_;
					int32  _c = 0;
					Goods2Tbl * p_tbl = GObj.P_Tbl;
					BExtQuery q(p_tbl, 2);
					DBQ * dbq = &(p_tbl->Kind == PPGDSK_BRAND);
					if(P_BrF && P_BrF->OwnerID)
						dbq = &(*dbq && p_tbl->ManufID == P_BrF->OwnerID);
					q.select(p_tbl->ID, p_tbl->Name, p_tbl->ManufID, p_tbl->Flags, 0L).where(*dbq);
					MEMSZERO(k_);
					k_.k2.Kind = PPGDSK_BRAND;
					for(q.initIteration(false, &k_, spGe); q.nextIteration() > 0;) {
						int _t = IncAndTestCounterForPage(&_c);
						if(_t > 0) {
							if(P_BrF && P_BrF->Name.NotEmpty()) {
								if(P_BrF->Flags & LocalBrandFilt::fSubName) {
									if((temp_buf = p_tbl->data.Name).Search(P_BrF->Name, 0, 1, 0))
										THROW_SL(ResultList.Add(p_tbl->data.ID, 0, p_tbl->data.Name));
								}
								else if(P_BrF->Name.IsEqNC(p_tbl->data.Name))
									THROW_SL(ResultList.Add(p_tbl->data.ID, 0, p_tbl->data.Name));
							}
							else
								THROW_SL(ResultList.Add(p_tbl->data.ID, 0, p_tbl->data.Name));
						}
						else if(!_t)
							break;
					}
				}
			}
			break;
		case PPOBJ_PERSON:
			if(Operator == oSelect) {
				use_filt = 1;
				PersonTbl::Rec psn_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(PsnObj.Fetch(IdList.at(i), &psn_rec) > 0)
							THROW_SL(ResultList.Add(psn_rec.ID, 0, psn_rec.Name));
					}
					use_filt = 0;
				}
				if(use_filt) {
					int32 _c = 0;
					PPViewPerson psn_view;
					PersonViewItem vi;
					psn_view.Init_(P_PsnF);
					for(psn_view.InitIteration(); psn_view.NextIteration(&vi) > 0;) {
						int _t = IncAndTestCounterForPage(&_c);
						if(_t > 0) {
							THROW_SL(ResultList.Add(vi.ID, 0, vi.Name));
						}
						else if(!_t)
							break;
					}
				}
			}
			else if(Operator == oCreate) {
				PPID   new_id = 0;
				PPPersonPacket pack;
				THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
				THROW_PP(!isempty(strip(P_SetBlk->U.P.Name)), PPERR_NAMENEEDED);
				pack.Rec.CatID = P_SetBlk->U.P.CatID;
				if(P_SetBlk->U.P.KindCount) {
					for(uint i = 0; i < P_SetBlk->U.P.KindCount && P_SetBlk->U.P.KindList[i] != 0; i++) {
						pack.Kinds.add(P_SetBlk->U.P.KindList[i]);
					}
				}
				else
					pack.Kinds.add(PPPRK_UNKNOWN);
				pack.Kinds.sort();
				pack.Rec.Status = P_SetBlk->U.P.StatusID;
				if(!pack.Rec.Status) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					for(uint i = 0; !pack.Rec.Status && i < pack.Kinds.getCount(); i++) {
						if(pk_obj.Fetch(pack.Kinds.get(i), &pk_rec) > 0 && pk_rec.DefStatusID)
							pack.Rec.Status = pk_rec.DefStatusID;
					}
				}
				SETIFZ(pack.Rec.Status, PPPRS_LEGAL);
				STRNSCPY(pack.Rec.Name, P_SetBlk->U.P.Name);
				THROW(PsnObj.PutPacket(&new_id, &pack, 1));
			}
			else if(Operator == oSetPersonRel) {
				THROW_PP(P_PsnRelF, PPERR_EMPTYPPOBJECT);
				THROW_PP(P_PsnRelF->PrmrPersonID, PPERR_UNDEFPRMRPSN);
				THROW_PP(P_PsnRelF->ScndPersonID, PPERR_UNDEFSCNDPSN);
				THROW_PP(P_PsnRelF->RelTypeID,    PPERR_UNDEFRELTYPE);
				{
					int    found = 0;
					LAssocArray rel_list;
					PPTransaction tra(1);
					THROW(tra);
					THROW(PsnObj.P_Tbl->GetRelList(P_PsnRelF->PrmrPersonID, &rel_list, 0));
					if(rel_list.SearchPair(P_PsnRelF->ScndPersonID, P_PsnRelF->RelTypeID, 0))
						found = 1;
					else {
						uint pos = 0;
						THROW_SL(rel_list.Add(P_PsnRelF->ScndPersonID, P_PsnRelF->RelTypeID, &pos, 0));
						THROW(PsnObj.P_Tbl->PutRelList(P_PsnRelF->PrmrPersonID, &rel_list, 0));
					}
					THROW(tra.Commit());
				}
			}
			else if(Operator == oGetPersonRel) {
				LAssocArray rel_list;
				THROW_PP(P_PsnRelF, PPERR_EMPTYPPOBJECT);
				THROW_PP(P_PsnRelF->PrmrPersonID || P_PsnRelF->ScndPersonID, PPERR_UNDEFPRMRSCNDPSN);
				//THROW_PP(P_PsnRelF->RelTypeID, PPERR_UNDEFRELTYPE);
				{
					if(P_PsnRelF->PrmrPersonID) {
						THROW(PsnObj.P_Tbl->GetRelList(P_PsnRelF->PrmrPersonID, &rel_list, 0));
						if(P_PsnRelF->ScndPersonID || P_PsnRelF->RelTypeID) {
							//
							// Удаляем из списка лишние элементы
							//
							uint   c = rel_list.getCount();
							if(c) do {
								LAssoc & r_item = rel_list.at(--c);
								if(P_PsnRelF->ScndPersonID && r_item.Key != P_PsnRelF->ScndPersonID)
									rel_list.atFree(c);
								else if(P_PsnRelF->RelTypeID && r_item.Val != P_PsnRelF->RelTypeID)
									rel_list.atFree(c);
							} while(c);
						}
					}
					else if(P_PsnRelF->ScndPersonID) {
						THROW(PsnObj.P_Tbl->GetRelList(P_PsnRelF->ScndPersonID, &rel_list, 1));
						if(P_PsnRelF->RelTypeID) {
							//
							// Удаляем из списка лишние элементы
							//
							uint   c = rel_list.getCount();
							if(c) do {
								LAssoc & r_item = rel_list.at(--c);
								if(P_PsnRelF->RelTypeID && r_item.Val != P_PsnRelF->RelTypeID)
									rel_list.atFree(c);
							} while(c);
						}
					}
					{
						PersonTbl::Rec psn_rec;
						for(uint i = 0; i < rel_list.getCount(); i++) {
							if(PsnObj.Fetch(rel_list.at(i).Key, &psn_rec) > 0)
								THROW_SL(ResultList.Add(psn_rec.ID, 0, psn_rec.Name));
						}
					}
				}
			}
			break;
		case PPOBJ_PERSONKIND:
			{
				use_filt = 1;
				PPObjPersonKind pk_obj; // PPObjReference Reference
				PPPersonKind pk_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++)
						if(pk_obj.Fetch(IdList.at(i), &pk_rec) > 0)
							THROW_SL(ResultList.Add(pk_rec.ID, 0, pk_rec.Name));
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(pk_obj.SearchBySymb(txt_buf, &(temp_id = 0), &pk_rec) > 0) {
									THROW_SL(ResultList.Add(pk_rec.ID, 0, pk_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(pk_obj.SearchByName(txt_buf, &temp_id, &pk_rec) > 0)
									THROW_SL(ResultList.Add(pk_rec.ID, 0, pk_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = pk_obj.P_Ref->Enum(ObjType, 0); en.Next(&pk_rec) > 0;) {
						THROW_SL(ResultList.Add(pk_rec.ID, 0, pk_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_PRSNCATEGORY:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjPersonCat prsct_obj;
				PPPersonCat prsct_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(prsct_obj.Fetch(IdList.at(i), &prsct_rec) > 0)
							THROW_SL(ResultList.Add(prsct_rec.ID, 0, prsct_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(prsct_obj.P_Ref->SearchSymb(PPOBJ_PRSNCATEGORY, &(temp_id = 0), txt_buf, offsetof(PPPersonCat, Symb)) > 0) {
									if(prsct_obj.Fetch(temp_id, &prsct_rec) > 0)
										THROW_SL(ResultList.Add(prsct_rec.ID, 0, prsct_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(prsct_obj.SearchByName(txt_buf, &temp_id, &prsct_rec) > 0)
									THROW_SL(ResultList.Add(prsct_rec.ID, 0, prsct_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = prsct_obj.P_Ref->Enum(ObjType, 0); en.Next(&prsct_rec) > 0;) {
						THROW_SL(ResultList.Add(prsct_rec.ID, 0, prsct_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_PRSNSTATUS:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjPersonStatus prsnst_obj;
				PPPersonStatus prsnst_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(prsnst_obj.Search(IdList.at(i), &prsnst_rec) > 0)
							THROW_SL(ResultList.Add(prsnst_rec.ID, 0, prsnst_rec.Name));
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = prsnst_obj.P_Ref->Enum(ObjType, 0); en.Next(&prsnst_rec) > 0;) {
						THROW_SL(ResultList.Add(prsnst_rec.ID, 0, prsnst_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_GLOBALUSERACC:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(gua_obj.Search(IdList.at(i), &gua_rec) > 0)
							THROW_SL(ResultList.Add(gua_rec.ID, 0, gua_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(gua_obj.SearchBySymb(txt_buf, &(temp_id = 0), &gua_rec) > 0) {
									THROW_SL(ResultList.Add(gua_rec.ID, 0, gua_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(gua_obj.SearchByName(txt_buf, &temp_id, &gua_rec) > 0)
									THROW_SL(ResultList.Add(gua_rec.ID, 0, gua_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					if(P_GaF && P_GaF->OwnerList.getCount()) {
						for(uint i = 0; i < P_GaF->OwnerList.getCount(); i++) {
							const PPID owner_id = P_GaF->OwnerList.get(i);
							for(SEnum en = gua_obj.P_Ref->EnumByIdxVal(ObjType, 2, owner_id); en.Next(&gua_rec) > 0;) {
								THROW_SL(ResultList.Add(gua_rec.ID, 0, gua_rec.Name));
							}
						}
					}
					else {
						for(SEnum en = gua_obj.P_Ref->Enum(ObjType, 0); en.Next(&gua_rec) > 0;) {
							THROW_SL(ResultList.Add(gua_rec.ID, 0, gua_rec.Name));
						}
					}
				}
			}
			else if(Operator == oCreate) {
				PPID   new_id = 0;
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				// @v10.6.5 @ctr MEMSZERO(gua_rec);
				THROW_PP(P_SetBlk, PPERR_EMPTYPPOBJECT);
				gua_rec.Tag  = ObjType;
				STRNSCPY(gua_rec.Name, P_SetBlk->U.GA.Name);
				gua_rec.PersonID = P_SetBlk->U.GA.PersonID;
				THROW(gua_obj.CheckName(gua_rec.ID, gua_rec.Name, 1));
				Reference::Encrypt(Reference::crymRef2, P_SetBlk->U.GA.Password, gua_rec.Password, sizeof(gua_rec.Password));
				THROW(gua_obj.P_Ref->AddItem(ObjType, &new_id, &gua_rec, 1));
			}
			break;
		case PPOBJ_STYLOPALM:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjStyloPalm sp_obj;
				PPStyloPalm sp_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(sp_obj.Search(IdList.at(i), &sp_rec) > 0)
							THROW_SL(ResultList.Add(sp_rec.ID, 0, sp_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(sp_obj.SearchBySymb(txt_buf, &(temp_id = 0), &sp_rec) > 0) {
									THROW_SL(ResultList.Add(sp_rec.ID, 0, sp_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(sp_obj.SearchByName(txt_buf, &temp_id, &sp_rec) > 0)
									THROW_SL(ResultList.Add(sp_rec.ID, 0, sp_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = sp_obj.P_Ref->Enum(ObjType, 0); en.Next(&sp_rec) > 0;) {
						THROW_SL(ResultList.Add(sp_rec.ID, 0, sp_rec.Name));
					}
				}
			}
			else if(Operator == oCreate) {

			}
			break;
		case PPOBJ_PROCESSOR:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjProcessor prc_obj;
				ProcessorTbl::Rec prc_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(prc_obj.Search(IdList.at(i), &prc_rec) > 0)
							THROW_SL(ResultList.Add(prc_rec.ID, 0, prc_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(prc_obj.SearchByCode(txt_buf, &(temp_id = 0), &prc_rec) > 0) {
									THROW_SL(ResultList.Add(prc_rec.ID, 0, prc_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(prc_obj.SearchByName(0, txt_buf, &temp_id, &prc_rec) > 0)
									THROW_SL(ResultList.Add(prc_rec.ID, 0, prc_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					PPViewProcessor view;
					if(P_PrcF) {
						THROW(view.Init_(P_PrcF));
					}
					else {
						ProcessorFilt filt;
						THROW(view.Init_(&filt));
					}
					ProcessorViewItem item;
					for(view.InitIteration(); view.NextIteration(&item) > 0;) {
						THROW_SL(ResultList.Add(item.ID, 0, item.Name));
					}
				}
			}
			else if(Operator == oCreate) {

			}
			break;
		case PPOBJ_TSESSION:
			if(oneof2(Operator, oSelect, oGetTSessPlaceStatus)) {
				int    r = ProcessSelection_TSession(Operator, _xmlcp, rResult);
				THROW(r);
				if(r > 0)
					done = 1;
				ok = 1;
			}
			else if(oneof2(Operator, oTSessCipCheckIn, oTSessCipCancel)) {
				PPID   tses_id = 0;
                THROW_PP(P_SetBlk, PPERR_TSESSCIPOP_NOCRIT);
                {
                	TSessionTbl::Rec tses_rec;
                	THROW_MEM(SETIFZ(P_TSesObj, new PPObjTSession));
					if(P_SetBlk->U.TS.TSessID) {
						THROW(P_TSesObj->Search(P_SetBlk->U.TS.TSessID, &tses_rec) > 0);
						tses_id = tses_rec.ID;
					}
					else if(!P_SetBlk->U.TS.TSessUUID.IsZero()) {
						if(P_TSesObj->SearchByGuid(P_SetBlk->U.TS.TSessUUID, &tses_rec) > 0) {
							tses_id = tses_rec.ID;
						}
					}
					THROW_PP(tses_id, PPERR_TSESSCIPOP_SESSUNDEF);
					{
						TSessionPacket pack;
						uint   cip_pos = 0;
						SString crit_place_code(P_SetBlk->U.TS.PlaceCode);
						SString place_code;
						ProcessorPlaceCodeTemplate::NormalizeCode(crit_place_code);
						PPObjUhttStore store_obj;
						PPUhttStore store_rec;
						const PPID store_id = (P_SetBlk->U.TS.UhttStoreID && store_obj.Search(P_SetBlk->U.TS.UhttStoreID, &store_rec) > 0) ? store_rec.ID : 0;
						THROW_PP(P_SetBlk->U.TS.CipID || crit_place_code.NotEmpty(), PPERR_TSESSCIPOP_NOITEMCRIT);
						THROW(P_TSesObj->GetPacket(tses_id, &pack, PPObjTSession::gpoLoadLines) > 0);
						{
							int    do_turn = 0;
							PPCheckInPersonItem ci_item;
							for(uint i = 0; !cip_pos && i < pack.CiList.GetCount(); i++) {
								const PPCheckInPersonItem & r_ci_item = pack.CiList.Get(i);
								ProcessorPlaceCodeTemplate::NormalizeCode(place_code = r_ci_item.PlaceCode);
								if(P_SetBlk->U.TS.CipID) {
									if(r_ci_item.ID == P_SetBlk->U.TS.CipID) {
										THROW_PP(crit_place_code.IsEmpty() || place_code == crit_place_code, PPERR_TSESSCIPOP_INVPLACECODE);
										cip_pos = i+1;
									}
								}
								else if(crit_place_code.NotEmpty()) {
									if(place_code == crit_place_code)
										cip_pos = i+1;
								}
							}
							//
							if(cip_pos == 0) {
								PPObjTSession::PlaceStatus place_status;
								THROW_PP(Operator == oTSessCipCheckIn, PPERR_TSESSCIPOP_INVCIPID); // Нельзя отменить заказ на не заказанное место
								P_TSesObj->GetPlaceStatus(tses_id, crit_place_code,
									P_SetBlk->U.TS.QuotKindID, (store_id ? store_rec.LocID : 0), place_status);
								THROW_PP_S(place_status.Status > 0, PPERR_TSESSCIPOP_PLACESTATUS, crit_place_code);
								ci_item.SetAnonym();
								ci_item.Kind = PPCheckInPersonItem::kTSession;
								ci_item.PrmrID = tses_id;
								ci_item.CiCount = 1;
								ci_item.RegCount = 1;
								ci_item.Amount = place_status.Price;
								STRNSCPY(ci_item.PlaceCode, crit_place_code);
								THROW(ci_item.CheckIn(PPCheckInPersonItem::opfVerifyPinCode, P_SetBlk->U.TS.PinCode) > 0);
								do_turn = 1;
							}
							else {
								ci_item = pack.CiList.Get(cip_pos-1);
                                if(Operator == oTSessCipCheckIn) {
									THROW(ci_item.CheckIn(PPCheckInPersonItem::opfVerifyPinCode, P_SetBlk->U.TS.PinCode) > 0);
									do_turn = 1;
                                }
                                else if(Operator == oTSessCipCancel) {
									THROW(ci_item.Cancel(PPCheckInPersonItem::opfVerifyPinCode, P_SetBlk->U.TS.PinCode) > 0);
									do_turn = 1;
                                }
							}
							if(do_turn) {
								PPCheckInPersonConfig cipc(*P_TSesObj, pack);
								THROW(cipc);
								if(cip_pos) {
									THROW(pack.CiList.UpdateItem(cip_pos-1, ci_item, &cipc));
								}
								else {
									THROW(pack.CiList.AddItem(ci_item, &cipc, &cip_pos));
									cip_pos++;
								}
								THROW(P_TSesObj->PutPacket(&tses_id, &pack, 1));
							}
						}
					}
                }
			}
			break;
		case PPOBJ_LOCATION:
			{
				SETIFZ(P_LocF, new LocationFilt);
				use_filt = 1;
				LocationTbl::Rec loc_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(PsnObj.LocObj.Fetch(IdList.at(i), &loc_rec) > 0)
							THROW_SL(ResultList.Add(loc_rec.ID, loc_rec.ParentID, loc_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(PsnObj.LocObj.P_Tbl->SearchCode(LOCTYP_ADDRESS, temp_buf, &(temp_id = 0), &loc_rec) > 0)
							THROW_SL(ResultList.Add(loc_rec.ID, loc_rec.ParentID, loc_rec.Name));
					}
					use_filt = 0;
				}
				if(use_filt) {
					P_LocF->LocType = LOCTYP_ADDRESS;
					StrAssocArray * p_temp_list = PsnObj.LocObj.MakeList_(P_LocF, 0);
					if(p_temp_list) {
						ResultList = *p_temp_list;
						ZDELETE(p_temp_list);
					}
					/* @v
					for(SEnum en = PsnObj.LocObj.P_Tbl->Enum(LOCTYP_ADDRESS, P_LocF->Owner, LocationCore::eoParentAsOwner); en.Next(&loc_rec) > 0;) {
						THROW_SL(ResultList.Add(loc_rec.ID, loc_rec.ParentID, loc_rec.Name));
					}
					*/
				}
			}
			break;
		case PPOBJ_WORLD:
			{
				use_filt = 1;
				PPObjWorld w_obj;
				WorldTbl::Rec w_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(w_obj.Fetch(IdList.at(i), &w_rec) > 0)
							THROW_SL(ResultList.Add(w_rec.ID, 0, w_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 1) {
								int kind = (ObjTypeExt == WORLDOBJ_COUNTRY) ? WORLDOBJ_COUNTRY : WORLDOBJ_CITY;
								if(w_obj.SearchByName(kind, txt_buf, &w_rec) > 0)
									THROW_SL(ResultList.Add(w_rec.ID, w_rec.ParentID, w_rec.Name));
							}
							else if(_ep == 2) {
								if(w_obj.SearchByCode(txt_buf, &w_rec) > 0)
									THROW_SL(ResultList.Add(w_rec.ID, w_rec.ParentID, w_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					SVector temp_list(sizeof(WorldTbl::Rec)); // @v10.6.7 SArray-->SVector
					if(!P_WrldF) {
						P_WrldF = new PPObjWorld::SelFilt();
						P_WrldF->KindFlags = (ObjTypeExt == WORLDOBJ_COUNTRY) ? WORLDOBJ_COUNTRY : WORLDOBJ_CITY;
					}
					THROW(w_obj.GetListByFilt(*P_WrldF, &temp_list));
					uint n = temp_list.getCount();
					for(uint i = 0; i < n; i++) {
						const WorldTbl::Rec * p_rec = static_cast<const WorldTbl::Rec *>(temp_list.at(i));
						THROW_SL(ResultList.Add(p_rec->ID, p_rec->ParentID, p_rec->Name));
					}
				}
			}
			break;
		case PPOBJ_CURRENCY:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjCurrency cur_obj;
				PPCurrency cur_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(cur_obj.Fetch(IdList.at(i), &cur_rec) > 0)
							THROW_SL(ResultList.Add(cur_rec.ID, 0, cur_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(cur_obj.SearchBySymb(txt_buf, &(temp_id = 0), &cur_rec) > 0) {
									THROW_SL(ResultList.Add(cur_rec.ID, 0, cur_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(cur_obj.SearchByName(txt_buf, &temp_id, &cur_rec) > 0)
									THROW_SL(ResultList.Add(cur_rec.ID, 0, cur_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = cur_obj.P_Ref->Enum(ObjType, 0); en.Next(&cur_rec) > 0;) {
						THROW_SL(ResultList.Add(cur_rec.ID, 0, cur_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_CURRATETYPE:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjCurRateType crt_obj;
				PPCurRateType crt_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(crt_obj.Search(IdList.at(i), &crt_rec) > 0)
							THROW_SL(ResultList.Add(crt_rec.ID, 0, crt_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(crt_obj.SearchBySymb(txt_buf, &(temp_id = 0), &crt_rec) > 0) {
									THROW_SL(ResultList.Add(crt_rec.ID, 0, crt_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(crt_obj.SearchByName(txt_buf, &temp_id, &crt_rec) > 0)
									THROW_SL(ResultList.Add(crt_rec.ID, 0, crt_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = crt_obj.P_Ref->Enum(ObjType, 0); en.Next(&crt_rec) > 0;) {
						THROW_SL(ResultList.Add(crt_rec.ID, 0, crt_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_CURRATEIDENT:
			if(Operator == oSelect) {
				PPViewCurRate     view;
				CurRateViewItem   item;
				UhttCurRateIdent  cri;
				TSArray <UhttCurRateIdent> list;
				SETIFZ(P_CurRateF, new CurRateFilt());
				int  actual = BIN(P_CurRateF->Flags & CurRateFilt::fActualOnly);
				SETIFZ(P_CurRateF->RateTypeID, DS.LCfg().BaseRateTypeID);
				THROW_PP(P_CurRateF->RateTypeID, PPERR_CMDSEL_EXP_CRITERION);
				PPIDArray id_ary;
				if(P_CurRateF->CurID > 0)
					id_ary.addUnique(P_CurRateF->CurID);
				else {
					PPID   id = 0;
					PPObjCurrency  obj_cur;
					while(obj_cur.EnumItems(&id, 0) > 0)
						id_ary.addUnique(id);
				}
				for(uint i = 0; i < id_ary.getCount(); i++) {
					P_CurRateF->CurID = id_ary.at(i);
					THROW(view.Init_(P_CurRateF));
					THROW(view.InitIteration());
					MEMSZERO(cri);
					while(view.NextIteration(&item) > 0) {
						cri.Ident.CurID = item.CurID;
						cri.Ident.Dt = item.Dt;
						cri.Rate = item.Rate;
						if(!actual)
							THROW_SL(list.insert(&cri));
					}
					if(actual && cri.Ident.CurID > 0)
						THROW_SL(list.insert(&cri));
				}
				if(OutFormat == fmtBinary) {
					THROW_SL(rResult.Write(&list, 0));
				}
				else if(OutFormat == fmtTddo) {
					THROW(Helper_ProcessTddo(0, &list, "UhttCurRateArray", OutTemplate, rResult));
				}
				else if(OutFormat == fmtJson) {
					PPExportDL600DataToJson(DL600StrucName, 0, &list, ResultText);
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				else {
					THROW(PPExportDL600DataToBuffer("UhttCurRateArray", &list, _xmlcp, ResultText));
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				ok = 2;
			}
			break;
		case PPOBJ_UNIT:
			if(Operator == oSelect) {
				use_filt = 1;
				PPObjUnit unit_obj;
				PPUnit unit_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(unit_obj.Search(IdList.at(i), &unit_rec) > 0)
							THROW_SL(ResultList.Add(unit_rec.ID, 0, unit_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							if(o_buf.ToLong() == 1) {
								if(unit_obj.SearchByName(txt_buf, &temp_id, &unit_rec) > 0)
									THROW_SL(ResultList.Add(unit_rec.ID, 0, unit_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = unit_obj.P_Ref->Enum(ObjType, 0); en.Next(&unit_rec) > 0;) {
						if(unit_rec.Flags & ExtFiltFlags)
							THROW_SL(ResultList.Add(unit_rec.ID, 0, unit_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_SPECSERIES:
			if(Operator == oSelect) {
				PPViewSpecSeries   view;
				SpecSeriesViewItem item;
				SETIFZ(P_SpecSerF, new SpecSeriesFilt());
				THROW(view.Init_(P_SpecSerF));
				THROW(view.InitIteration());
				while(view.NextIteration(&item) > 0) {
					if(P_SpecSerF->Serial.IsEmpty() || P_SpecSerF->Serial == item.Serial)
						THROW_SL(ResultList.Add(item.ID, 0, item.Serial));
				}
			}
			break;
		case PPOBJ_SCARD:
			SETIFZ(P_ScObj, new PPObjSCard);
			THROW_MEM(P_ScObj);
			if(Operator == oSelect) {
				use_filt = 1;
				SCardTbl::Rec sc_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(P_ScObj->Search(IdList.at(i), &sc_rec) > 0)
							THROW_SL(ResultList.Add(sc_rec.ID, 0, sc_rec.Code));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							if(o_buf.ToLong() == 1) {
								if(P_ScObj->SearchCode(0, txt_buf, &sc_rec) > 0) {
									THROW_SL(ResultList.Add(sc_rec.ID, 0, sc_rec.Code));
								}
								else {
									ObjTagItem  tag;
									SString     number, prefix;
									if(DS.GetTLA().GlobAccID > 0) {
										if(p_ref->Ot.GetTag(PPOBJ_GLOBALUSERACC, DS.GetTLA().GlobAccID, PPTAG_GUA_SCARDPREFIX, &tag) > 0) {
											prefix = tag.Val.PStr;
											if(prefix.NotEmpty()) {
												number.Z().Cat(prefix).Cat(txt_buf);
												if(P_ScObj->SearchCode(0, number, &sc_rec) > 0)
													THROW_SL(ResultList.Add(sc_rec.ID, 0, sc_rec.Code));
											}
										}
									}
								}
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					PPViewSCard sc_view;
					SETIFZ(P_SCardF, new SCardFilt());
					P_SCardF->Flags |= (SCardFilt::fNoTempTable|SCardFilt::fNoEmployer);
					THROW(sc_view.Init_(P_SCardF));
					{
						const StrAssocArray & r_list = sc_view.GetList();
						for(uint i = 0; i < r_list.getCount(); i++) {
							StrAssocArray::Item item = r_list.at_WithoutParent(i);
							THROW_SL(ResultList.Add(item.Id, 0, item.Txt));
						}
					}
				}
			}
			else if(Operator == oSCardRest) {
				if(P_SetBlk) {
					double rest = 0.0;
					THROW_PP_S(P_SetBlk->U.SC.ID, PPERR_CMDSEL_UNIDENTSCARD, temp_buf.Z());
					THROW(P_ScObj->P_Tbl->GetRest(P_SetBlk->U.SC.ID, ZERODATE, &rest));
					rResult.SetString(temp_buf.Z().Cat(rest, SFMT_MONEY));
				}
			}
			else if(oneof2(Operator, oSCardDeposit, oSCardWithdraw)) {
				// @Muxa {
				if(P_SetBlk) {
					PPGlobalAccRights  rights_blk(PPTAG_GUA_SCARDRIGHTS);
					if(rights_blk.IsAllow(PPGlobalAccRights::fOperation)) {
						double rest = 0.0;
						const  double amount = R2(fabs(P_SetBlk->U.SC.Amount));
						SCardTbl::Rec sc_rec;
						PPObjSCardSeries scs_obj;
						PPSCardSeries2 scs_rec;
						THROW_PP_S(P_SetBlk->U.SC.ID, PPERR_CMDSEL_UNIDENTSCARD, temp_buf.Z());
						THROW_PP_S(amount >= 0.01, PPERR_CMDSEL_INVAMOUNT, temp_buf.Z().Cat(amount, SFMT_MONEY));
						THROW(P_ScObj->Search(P_SetBlk->U.SC.ID, &sc_rec) > 0);
						THROW(scs_obj.Search(sc_rec.SeriesID, &scs_rec) > 0);
						THROW_PP_S(oneof2(scs_rec.GetType(), scstCredit, scstBonus), PPERR_SCARDMUSTBECRDBNS, sc_rec.Code);
						THROW(P_ScObj->P_Tbl->GetRest(sc_rec.ID, ZERODATE, &rest));
						rest += sc_rec.MaxCredit;
						if(Operator != oSCardDeposit) {
							// @v10.9.3 {
							if((rest - amount) <= -0.01) {
								temp_buf.Z().Cat(sc_rec.Code).Space().
									CatEq("amount", amount, MKSFMTD(0, 8, NMBF_NOTRAILZ)).Space().CatEq("rest", rest, MKSFMTD(0, 8, NMBF_NOTRAILZ));
								CALLEXCEPT_PP_S(PPERR_SCARDRESTNOTENOUGH, temp_buf);
							}
							// } @v10.9.3 
							// @v10.9.3 THROW_PP_S(((rest - amount) > -0.01), PPERR_SCARDRESTNOTENOUGH, sc_rec.Code); // @v10.9.3 (amount<=rest)-->((rest-amount)>-0.01)
						}
						{
							TSVector <SCardCore::UpdateRestNotifyEntry> urn_list;
							SCardCore::OpBlock ob;
							ob.SCardID = sc_rec.ID;
							ob.Amount = (Operator == oSCardDeposit) ? amount : -amount;
							THROW(P_ScObj->P_Tbl->PutOpBlk(ob, &urn_list, 1));
							P_ScObj->FinishSCardUpdNotifyList(urn_list);
						}
					}
					else {
						PPSetError(PPERR_NORIGHTS);
						DS.GetTLA().AddedMsgStrNoRights = DS.GetTLA().GlobAccName;
						ok = 0;
					}
				}
				// }
			}
			break;
		case PPOBJ_UHTTSCARDOP:
			if(Operator == oSelect) {
				PPViewUhttSCardOp view;
				SETIFZ(P_UhttSCardOpF, new UhttSCardOpFilt());
				P_UhttSCardOpF->GlobalAccID = DS.GetTLA().GlobAccID;
				view.Init_(P_UhttSCardOpF);
				view.InitIteration();
				const TSArray <UhttSCardOpViewItem> * p_list = view.GetList();
				if(OutFormat == fmtBinary) {
					THROW_SL(rResult.Write(p_list, 0));
				}
				else if(OutFormat == fmtTddo) {
					THROW(Helper_ProcessTddo(0, (void *)p_list, "UhttSCardOpArray", OutTemplate, rResult)); // @badcast
				}
				else if(OutFormat == fmtJson) {
					PPExportDL600DataToJson(DL600StrucName, 0, (void *)p_list, ResultText); // @badcast
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				else {
					THROW(PPExportDL600DataToBuffer("UhttSCardOpArray", (void *)p_list, _xmlcp, ResultText)); // @badcast
					THROW(rResult.WriteString(ResultText));
					rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
				}
				ok = 2;
			}
			break;
		case PPOBJ_UHTTSTORE:
			if(Operator == oSelect) {
				THROW_MEM(SETIFZ(P_UhttStorF, new LocalUhttStoreFilt));
				use_filt = 1;
				PPObjUhttStore uhs_obj;
				PPUhttStore uht_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(uhs_obj.Search(IdList.at(i), &uht_rec) > 0)
							THROW_SL(ResultList.Add(uht_rec.ID, 0, uht_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(uhs_obj.SearchBySymb(txt_buf, &(temp_id = 0), &uht_rec) > 0) {
									THROW_SL(ResultList.Add(uht_rec.ID, 0, uht_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(uhs_obj.SearchByName(txt_buf, &temp_id, &uht_rec) > 0)
									THROW_SL(ResultList.Add(uht_rec.ID, 0, uht_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = uhs_obj.P_Ref->Enum(ObjType, 0); en.Next(&uht_rec) > 0;) {
						if(P_UhttStorF->OwnerID == 0 || P_UhttStorF->OwnerID == uht_rec.PersonID)
							THROW_SL(ResultList.Add(uht_rec.ID, 0, uht_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_OPRKIND:
			{
				use_filt = 1;
				PPObjOprKind oprkind_obj;
				PPOprKind    oprkind_rec;
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(oprkind_obj.Search(IdList.at(i), &oprkind_rec) > 0)
							THROW_SL(ResultList.Add(oprkind_rec.ID, 0, oprkind_rec.Name));
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(oprkind_obj.SearchBySymb(txt_buf, &(temp_id = 0), &oprkind_rec) > 0) {
									THROW_SL(ResultList.Add(oprkind_rec.ID, 0, oprkind_rec.Name));
								}
							}
							else if(_ep == 1) {
								if(oprkind_obj.SearchByName(txt_buf, &temp_id, &oprkind_rec) > 0)
									THROW_SL(ResultList.Add(oprkind_rec.ID, 0, oprkind_rec.Name));
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					for(SEnum en = oprkind_obj.P_Ref->Enum(ObjType, 0); en.Next(&oprkind_rec) > 0;) {
						THROW_SL(ResultList.Add(oprkind_rec.ID, 0, oprkind_rec.Name));
					}
				}
			}
			break;
		case PPOBJ_GTA:
			if(Operator == oGtaCheckIn) {
				GtaJournalCore * p_gtaj = DS.GetGtaJ();
				if(p_gtaj) {
					PPGta  local_gta_blk; // не путать с gta_blk, объявленным в начале функции
					local_gta_blk.Op = P_SetBlk->U.GT.GtaOp;
					local_gta_blk.GlobalUserID = P_SetBlk->U.GT.GlobalUserID;
					local_gta_blk.ObjId = P_SetBlk->U.GT.Oi;
					local_gta_blk.Count = P_SetBlk->U.GT.Count;
					local_gta_blk.Duration = P_SetBlk->U.GT.Duration;
					local_gta_blk.Dtm = getcurdatetime_();
					THROW(p_gtaj->CheckInOp(local_gta_blk, 1));
				}
			}
			break;
		case PPOBJ_WORKBOOK:
			if(Operator == oSelect) {
				SETIFZ(P_WorkbookF, new LocalWorkbookFilt);
				THROW_MEM(P_WorkbookF);
				use_filt = 1;
				PPObjWorkbook wb_obj;
				WorkbookTbl::Rec wb_rec;
				PPIDArray temp_list; // @v10.2.5
				if(IdList.getCount()) {
					for(uint i = 0; i < IdList.getCount(); i++) {
						if(wb_obj.Fetch(IdList.at(i), &wb_rec) > 0) {
							// @v10.2.5 THROW_SL(ResultList.Add(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
							temp_list.add(wb_rec.ID); // @v10.2.5
						}
					}
					use_filt = 0;
				}
				if(SrchCodeList.getCount()) {
					SString left_buf, ssfx_buf;
					for(uint p = 0; SrchCodeList.get(&p, temp_buf);) {
						if(temp_buf.Divide(',', o_buf, txt_buf) > 0) {
							if(P_WorkbookF->Flags & LocalWorkbookFilt::fStripSSfx) {
								if(txt_buf.Divide('/', left_buf, ssfx_buf) > 0) {
									txt_buf = left_buf;
								}
							}
							const long _ep = o_buf.ToLong();
							if(_ep == 2) {
								if(wb_obj.SearchBySymb(txt_buf, &(temp_id = 0), &wb_rec) > 0) {
									// @v10.2.5 THROW_SL(ResultList.Add(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
									temp_list.add(wb_rec.ID); // @v10.2.5
								}
							}
							else if(_ep == 1) {
								if(wb_obj.SearchByName(txt_buf, &temp_id, &wb_rec) > 0) {
									// @v10.2.5 THROW_SL(ResultList.Add(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
									temp_list.add(wb_rec.ID); // @v10.2.5
								}
							}
						}
					}
					use_filt = 0;
				}
				if(use_filt) {
					if(P_WorkbookF->ParentID) {
						for(SEnum en = wb_obj.P_Tbl->EnumByParent(P_WorkbookF->ParentID, 0); en.Next(&wb_rec) > 0;) {
							if(P_WorkbookF->Type == 0 || wb_rec.Type == P_WorkbookF->Type) {
								// @v10.2.5 THROW_SL(ResultList.AddFast(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
								temp_list.add(wb_rec.ID); // @v10.2.5
							}
						}
					}
					else if(P_WorkbookF->Type) {
						for(SEnum en = wb_obj.P_Tbl->EnumByType(P_WorkbookF->Type, 0); en.Next(&wb_rec) > 0;) {
							// @v10.2.5 THROW_SL(ResultList.AddFast(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
							temp_list.add(wb_rec.ID); // @v10.2.5
						}
					}
					else {
						for(SEnum en = wb_obj.P_Tbl->Enum(0); en.Next(&wb_rec) > 0;) {
							// @v10.2.5 THROW_SL(ResultList.AddFast(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
							temp_list.add(wb_rec.ID); // @v10.2.5
						}
					}
				}
				// @v10.2.5 {
				if(temp_list.getCount()) {
					temp_list.sortAndUndup();
					wb_obj.SortIdListByRankAndName(temp_list);
					for(uint i = 0; i < temp_list.getCount(); i++) {
						if(wb_obj.Fetch(temp_list.get(i), &wb_rec) > 0) {
							THROW_SL(ResultList.AddFast(wb_rec.ID, wb_rec.ParentID, wb_rec.Name));
						}
					}
				}
				// } @v10.2.5
			}
			break;
		case PPOBJ_GEOTRACKING:
			if(Operator == oSelect) {
				PPViewGeoTracking view;
				if(view.Init_(P_GeoTrF)) {
					if(OutFormat == fmtJson) {
						PPExportDL600DataToJson(/*DL600StrucName*/"GeoTracking", &view, ResultText);
						THROW(rResult.WriteString(ResultText));
						rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
					}
					else {
						THROW(PPExportDL600DataToBuffer("GeoTracking", &view, _xmlcp, ResultText));
						THROW(rResult.WriteString(ResultText));
						rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
					}
					ok = 2;
				}
			}
			break;
		default:
			ok = 0;
			break;
	}
	if(gta_blk.GlobalUserID) {
		int     gta_ok = 1;
		switch(Operator) {
			case oSelect:
				if(ResultList.getCount()) {
					if(ResultList.getCount() == 1)
						gta_blk.ObjId.Id = ResultList.Get(0).Id;
					gta_blk.Count = ResultList.getCount();
				}
				else
					gta_ok = 0;
				break;
			case oSet:
				if(ObjType == PPOBJ_DL600DATA) {
					gta_ok = 0; // @todo этот случай обрабатывается отдельно
				}
				else if(ObjType == PPOBJ_QUOT2) {
					gta_blk.ObjId.Id = P_SetBlk->U.Q.GoodsID;
				}
				else if(ObjType == PPOBJ_GOODSARCODE) {
					gta_blk.ObjId.Id = P_SetBlk->U.AC.GoodsID;
				}
				break;
			case oCCheckCreate: gta_blk.ObjId.Id = P_SetBlk->U.C.SCardID; break;
			case oSCardWithdraw: gta_blk.ObjId.Id = P_SetBlk->U.SC.ID; break;
			case oSCardDeposit: gta_blk.ObjId.Id = P_SetBlk->U.SC.ID; break;
			case oSCardRest: gta_blk.ObjId.Id = P_SetBlk->U.SC.ID; break;
			case oBillFinish: gta_blk.ObjId.Id = P_SetBlk->U.B.ID; break;
		}
		gta_blk.Duration = ZEROTIME;
		gta_blk.Dtm = getcurdatetime_();
		if(gta_ok) {
			GtaJournalCore * p_gtaj = DS.GetGtaJ();
			if(p_gtaj)
				THROW(p_gtaj->CheckInOp(gta_blk, 1));
		}
	}
	if(oneof4(Operator, oSelect, oGetPersonRel, oGetObjectTag, oGetGoodsMatrix)) {
		if(!done && ok == 1) {
			if(OutFormat == fmtBinary) {
				THROW_SL(ResultList.Write(rResult, 0));
			}
			else if(OutFormat == fmtTddo) {
				THROW(Helper_ProcessTddo(0, &ResultList, "StrAssocArray", OutTemplate, rResult));
			}
			else if(OutFormat == fmtJson) {
				PPExportDL600DataToJson(DL600StrucName, &ResultList, 0, ResultText);
				THROW(rResult.WriteString(ResultText));
				rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
			}
			else {
				THROW(PPExportDL600DataToBuffer("StrAssocArray", &ResultList, _xmlcp, ResultText));
				THROW(rResult.WriteString(ResultText));
				rResult.SetDataType(PPJobSrvReply::htGenericText, 0);
			}
		}
	}
	else if(oneof10(Operator, oSet, oCCheckCreate, oCCheckAddLine, oCCheckFinish, oSCardRest, oBillCreate, oBillFinish,
		oDraftTransitGoodsRest, oDraftTransitGoodsRestList, oGetTSessPlaceStatus)) {
	}
	else
		rResult.SetAck();
	CATCHZOK
	return ok;
}

int Backend_SelectObjectBlock::ResolveCrit_QuotKind(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjQuotKind qk_obj;
				qk_obj.P_Ref->SearchSymb(PPOBJ_QUOTKIND, &id, rArg, offsetof(PPQuotKind, Symb));
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_OprKind(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjOprKind qk_obj;
				qk_obj.P_Ref->SearchSymb(PPOBJ_OPRKIND, &id, rArg, offsetof(PPOprKind, Symb));
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}


int Backend_SelectObjectBlock::ResolveCrit_PersonKind(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   kind_id = 0, temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			kind_id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjPersonKind pk_obj;
				pk_obj.SearchByName(rArg, &(temp_id = 0), 0);
				kind_id = temp_id;
			}
			break;
		case scCode:
			{
				PPObjPersonKind pk_obj;
				pk_obj.SearchBySymb(rArg, &(temp_id = 0), 0);
				kind_id = temp_id;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, kind_id);
	return (kind_id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_PersonCat(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   cat_id = 0, temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			cat_id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjPersonCat pc_obj;
				pc_obj.SearchByName(rArg, &(temp_id = 0), 0);
				cat_id = temp_id;
			}
			break;
		case scCode:
			{
				PPObjPersonCat pc_obj;
				pc_obj.SearchBySymb(rArg, &(temp_id = 0), 0);
				cat_id = temp_id;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, cat_id);
	return (cat_id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_TSessStatus(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID    status = 0;
	int     temp_status = 0;
	switch(subcriterion) {
		case 0:
			temp_status = rArg.ToLong();
			if(PPObjTSession::ValidateStatus(temp_status)) {
				status = temp_status;
			}
			else {
				temp_status = PPObjTSession::ResolveStatusSymbol(rArg);
				if(PPObjTSession::ValidateStatus(temp_status))
					status = temp_status;
				else
					PPSetError(PPERR_INVTSESSIONSTATUS, rArg);
			}
			break;
		case scID:
			temp_status = rArg.ToLong();
			if(PPObjTSession::ValidateStatus(temp_status))
				status = temp_status;
			else
				PPSetError(PPERR_INVTSESSIONSTATUS, rArg);
			break;
		case scName:
		case scCode:
			temp_status = PPObjTSession::ResolveStatusSymbol(rArg);
			if(PPObjTSession::ValidateStatus(temp_status))
				status = temp_status;
			else
				PPSetError(PPERR_INVTSESSIONSTATUS, rArg);
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
	}
	ASSIGN_PTR(pID, status);
	return status;
}

int Backend_SelectObjectBlock::ResolveCrit_PersonStatus(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0, temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjReference st_obj(PPOBJ_PRSNSTATUS, 0);
				st_obj.SearchByName(rArg, &(temp_id = 0), 0);
				id = temp_id;
			}
			break;
		case scCode:
			{
				PPObjReference st_obj(PPOBJ_PRSNSTATUS, 0);
				st_obj.SearchBySymb(rArg, &(temp_id = 0), 0);
				id = temp_id;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_PersonRelType(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   rel_type_id = 0, temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			rel_type_id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjPersonRelType prt_obj;
				prt_obj.SearchByName(rArg, &(temp_id = 0), 0);
				rel_type_id = temp_id;
			}
			break;
		case scCode:
			{
				PPObjPersonRelType prt_obj;
				prt_obj.SearchBySymb(rArg, &(temp_id = 0), 0);
				rel_type_id = temp_id;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, rel_type_id);
	return (rel_type_id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Brand(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjBrand brand_obj;
				brand_obj.SearchByName(rArg, &id, 0);
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_GoodsClass(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	PPID   temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scName:
			{
				PPObjGoodsClass gc_obj;
				gc_obj.SearchByName(rArg, &(temp_id = 0), 0);
				id = temp_id;
			}
			break;
		case scCode:
			{
				PPObjGoodsClass gc_obj;
				gc_obj.SearchBySymb(rArg, &(temp_id = 0), 0);
				id = temp_id;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_GoodsProp(PPID clsID, int prop, int subcriterion, const SString & rArg, ObjIdListFilt & rList)
{
	rList.Set(0);
	int    ok = -1;
	THROW_PP(clsID, PPERR_CMDSEL_GCPROPCRITWOCLS);
	{
		PPObjGoodsClass gc_obj;
		PPGdsClsPacket gc_pack;
		THROW(gc_obj.Fetch(clsID, &gc_pack) > 0);
		{
			StringSet ss(',', rArg);
			SString temp_buf;
			PPID   temp_id = 0;
			for(uint j = 0; ss.get(&j, temp_buf);) {
				switch(subcriterion) {
					case 0:
					case scID:
						temp_id = rArg.ToLong();
						rList.Add(temp_id);
						break;
					case scName:
						{
							gc_pack.PropNameToID(prop, rArg, &(temp_id = 0), 0, 0);
							rList.Add(temp_id);
						}
						break;
					case scCode:
						{
							gc_pack.PropSymbToID(prop, rArg, &(temp_id = 0));
							rList.Add(temp_id);
						}
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
				}
			}
			if(rList.GetCount())
				ok = 1;
		}
	}
	CATCH
		rList.Set(0);
		ok = 0;
	ENDCATCH
	return ok;
}

int Backend_SelectObjectBlock::ResolveCrit_Loc(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjLocation loc_obj;
				if(loc_obj.P_Tbl->SearchCode(LOCTYP_WAREHOUSE, rArg, &id) > 0) {
				}
				else if(loc_obj.P_Tbl->SearchCode(LOCTYP_ADDRESS, rArg, &(id = 0)) > 0) {
				}
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Processor(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjProcessor prc_obj;
				if(prc_obj.SearchByCode(rArg, &id, 0) > 0) {
				}
			}
			break;
		case scName:
			{
				PPObjProcessor prc_obj;
				if(prc_obj.SearchByName(0, rArg, &id, 0) > 0) {
				}
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_SCard(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				SCardTbl::Rec sc_rec;
				SETIFZ(P_ScObj, new PPObjSCard);
				if(P_ScObj->SearchCode(0, rArg, &sc_rec) > 0)
					id = sc_rec.ID;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_GlobalUser(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjGlobalUserAcc gua_obj;
				PPGlobalUserAcc gua_rec;
				if(gua_obj.SearchBySymb(rArg, &id, &gua_rec) > 0) {
					;
				}
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_PosNode(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjCashNode cn_obj;
				if(cn_obj.SearchBySymb(rArg, &id) > 0) {
					;
				}
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_GoodsGrpListByPosNode(int subcriterion, const SString & rArg, PPIDArray * pIdList)
{
	int ok  = 1;
	PPID id = 0;
	PPID cn_id = 0;
	PPObjTouchScreen obj_ts;
	PPObjCashNode obj_cn;
	PPCashNode cn_rec;

	THROW(ResolveCrit_PosNode(subcriterion, rArg, &cn_id));
	THROW(obj_cn.Fetch(cn_id, &cn_rec) > 0);
	if(cn_rec.Flags & CASHF_SYNC) {
		PPSyncCashNode synccn;
		PPTouchScreenPacket ts_pack;

		THROW(obj_cn.GetSync(cn_id, &synccn));
		THROW(obj_ts.GetPacket(synccn.TouchScreenID, &ts_pack) > 0);
		ASSIGN_PTR(pIdList, ts_pack.GrpIDList);
	}
	else
		pIdList->add(cn_rec.GoodsGrpID);
	CATCHZOK
	return ok;
}

int Backend_SelectObjectBlock::ResolveCrit_GoodsGrp(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	BarcodeTbl::Rec bc_rec;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			if(GgObj.SearchCode(rArg, &bc_rec) > 0)
				id = bc_rec.GoodsID;
			break;
		case scName:
			GgObj.SearchByName(rArg, &id, 0);
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Goods(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	BarcodeTbl::Rec bc_rec;
	Goods2Tbl::Rec goods_rec;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			if(GObj.SearchByBarcode(rArg, &bc_rec, &goods_rec, 0) > 0)
				id = bc_rec.GoodsID;
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Cur(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID: id = rArg.ToLong(); break;
		case scCode:
			{
				PPObjCurrency cur_obj;
				cur_obj.SearchSymb(&id, rArg);
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_CurRateType(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		case scCode:
			{
				PPObjCurRateType obj;
				obj.SearchBySymb(rArg, &id);
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Person(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID: id = rArg.ToLong(); break;
		case scCode:
		case scName: PsnObj.P_Tbl->SearchByName(rArg, &id, 0); break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Tag(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	Reference * p_ref = PPRef;
	switch(subcriterion) {
		case 0:
		case scID: id = rArg.ToLong(); break;
		case scCode: p_ref->SearchSymb(PPOBJ_TAG, &id, rArg, offsetof(PPObjectTag, Symb)); break;
		case scName: p_ref->SearchName(PPOBJ_TAG, &id, rArg); break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Article(int subcriterion, const SString & rArg, PPID * pID)
{
	PPID   id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			id = rArg.ToLong();
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	ASSIGN_PTR(pID, id);
	return (id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_ArByPerson(int subcriterion, const SString & rArg, PPID accSheetID, PPID * pID)
{
	const PPID acs_id = GetSupplAccSheet();
	PPID   temp_id = 0;
	switch(subcriterion) {
		case 0:
		case scID:
			temp_id = rArg.ToLong();
			break;
		case scCode:
			if(acs_id) { // @todo Возможно ошибка
				PPObjAccSheet acs_obj;
				PPAccSheet acs_rec;
				if(acs_obj.Fetch(accSheetID, &acs_rec) > 0 && acs_rec.Assoc == PPOBJ_PERSON && acs_rec.ObjGroup) {
					PPObjPersonKind pk_obj;
					PPPersonKind pk_rec;
					if(pk_obj.Fetch(acs_rec.ObjGroup, &pk_rec) > 0 && pk_rec.CodeRegTypeID) {
						PPIDArray list;
						if(PsnObj.GetListByRegNumber(pk_rec.CodeRegTypeID, pk_rec.ID, rArg, list) > 0)
							temp_id = list.get(0);
					}
				}
			}
			break;
		case scName:
			PsnObj.P_Tbl->SearchByName(rArg, &temp_id, 0);
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pID, 0);
			return 0;
	}
	if(temp_id && accSheetID) { // @todo Возможно ошибка
		PPID   ar_id = 0;
		ArObj.P_Tbl->PersonToArticle(temp_id, acs_id, &ar_id);
		temp_id = ar_id;
	}
	else
		temp_id = 0;
	ASSIGN_PTR(pID, temp_id);
	return (temp_id ? 1 : -1);
}

int Backend_SelectObjectBlock::ResolveCrit_Since(const SString & rArg, LDATETIME * pDtm)
{
	int    ok = 1;
	LDATETIME dtm;
	if(strtodatetime(rArg, &dtm, DATF_DMY, TIMF_HMS) && checkdate(dtm.d))
		ok = 1;
	else if(strtodatetime(rArg, &dtm, DATF_ISO8601, TIMF_HMS) && checkdate(dtm.d))
		ok = 1;
	else {
		PPSetError(PPERR_CMDSEL_INVSINCECRIT, rArg);
		ok = 0;
	}
	ASSIGN_PTR(pDtm, dtm);
	return ok;
}

int Backend_SelectObjectBlock::ResolveCrit_Page(const SString & rArg)
{
	int    ok = 1;
	IntRange page;
	SString temp_buf = rArg;
	SString start_buf, count_buf;
	THROW_PP_S(Page.IsZero(), PPERR_CMDSEL_DBLDECL_PAGE, rArg);
	temp_buf.Strip();
	page = 0;
	if(temp_buf.Divide(',', start_buf, count_buf) > 0) {
		page.low = start_buf.ToLong();
		int    c = count_buf.ToLong();
		THROW_PP_S(page.low > 0, PPERR_CMDSEL_INVPAGESTART, rArg);
		THROW_PP_S(c > 0, PPERR_CMDSEL_INVPAGECOUNT, rArg);
		page.upp = page.low + c;
	}
	else {
		int    c = temp_buf.ToLong();
		THROW_PP_S(c > 0, PPERR_CMDSEL_INVPAGECOUNT, rArg);
		page.low = 1;
		page.upp = page.low + c;
	}
	Page = page;
	CATCHZOK
	return ok;
}

int Backend_SelectObjectBlock::ResolveCrit_UhttStore(int subcriterion, const SString & rArg, PPID * pUhttsID, PPID * pSellerID, PPID * pLocID, long * pUsFlags)
{
	int    ok = 1;
	PPID   temp_id = 0;
	PPID   uhtts_id = 0;
	PPID   seller_id = 0;
	PPID   loc_id = 0;
	long   us_flags = 0;
	PPObjUhttStore uhs_obj;
	PPUhttStore rec;
	MEMSZERO(rec);
	switch(subcriterion) {
		case 0:
		case scID:
			temp_id = rArg.ToLong();
			if(uhs_obj.Search(temp_id, &rec)) {
				uhtts_id = rec.ID;
				seller_id = rec.PersonID;
				loc_id = rec.LocID;
				us_flags = rec.Flags;
			}
			break;
		case scCode:
			if(uhs_obj.SearchBySymb(rArg, &(temp_id = 0), &rec)) {
				uhtts_id = rec.ID;
				seller_id = rec.PersonID;
				loc_id = rec.LocID;
				us_flags = rec.Flags;
			}
			break;
		default:
			PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
			ASSIGN_PTR(pUhttsID, 0);
			ASSIGN_PTR(pSellerID, 0);
			ASSIGN_PTR(pLocID, 0);
			ASSIGN_PTR(pUsFlags, 0);
			ok = 0;
	}
	if(ok) {
		ASSIGN_PTR(pUhttsID, uhtts_id);
		ASSIGN_PTR(pSellerID, seller_id);
		ASSIGN_PTR(pLocID, loc_id);
		ASSIGN_PTR(pUsFlags, us_flags);
		if(rec.ID == 0) {
			ok = 0;
		}
		else if(seller_id == 0) {
			PPSetError(PPERR_CMDSEL_UHTTSTSELLERNDEF, rec.Name);
			ok = 0;
		}
		else if(loc_id == 0) {
			PPSetError(PPERR_CMDSEL_UHTTSTLOCNDEF, rec.Name);
			ok = 0;
		}
	}
	return ok;
}

/*

SELECT
	DL600
		ID
	QUOT
		ACTUAL   Критерий без параметров, означающий, что необходимо извлечь только актуальные котировки.
			Если этот критерий не указан, то извлекаются все значения подходящих котировок. Если указан,
			то извлекаются только актуальные (текущие) значения подходящих котировок.

		KIND     Вид котировки.
			ID   (default) Идентификатор вида котировки
			CODE Символ вида котировки

		SELLER   Продавец (публикатор котировки).
			Персоналия (транслируется в аналитическую статью таблицы GetSupplAccSheet())
			ID   (default) Идентификатор персоналии
			CODE  Номер поискового регистра вида персоналии (не путать с поисковым регистром таблицы аналитических статей)
			NAME  Наименование персоналии (поиск по точному соответсвию имени без учета регистров символов)

		BUYER    Покупатель. Персоналия (транслируется в аналитическую статью таблицы GetSupplAccSheet())
			ID   (default) Идентификатор персоналии
			CODE  Номер поискового регистра вида персоналии (не путать с поисковым регистром таблицы аналитических статей)
			NAME  Наименование персоналии (поиск по точному соответсвию имени без учета регистров символов)

		LOCATION Адрес продавца. Имеется в виду либо склад, либо любой адрес персоналии (юридический, фактический, либо адрес доставки).
			ID    (default) Идентификатор адреса.
			CODE  Код адреса. Сначала ищется код склада, в случае неудачи - код адреса.

		LOCWORLD Географический объект, которому должен принадлежать адрес продавца.
			ID   (default) Идентификатор географического объекта.
			CODE Символ географического объекта.

		GOODS    Товар, к которому относятся котировки.
			ID   (default) Идентификатор товара.
			CODE Код товара (без привязки к контрагенту).

		GOODSGROUP Товарная группа, для товаров которой необходимо извлечь котировки.
			Отменяется критерием GOODS.
			ID   (default) Идентификатор товарной группы.
			CODE  Код группы.
			NAME  Наименование группы (поиск по точному соответсвию имени без учета регистров символов).

		BRAND     Торговая марка, для товаров которой необходимо извлечь котировки.
			Отменяется критерием GOODS.
			ID   (default) Идентификатор брэнда.
			NAME  Наименование брэнда (поиск по точному соответсвию имени без учета регистров символов).

		CURRENCY Валюта значения котировки. Если не указана, то извлекаются котировки без ограничения по
			валюте, в которой они определены.
			ID   (default) Идентификатор валюты.
			CODE Символ валюты (не путать с цифровым кодом валюты).

		return: UhttQuot
		note: Выборка котировок не предусматривает множественных критериев. Таким образом,
		если одни и тот же критерий указан несколько раз, то в рассмотрение берется только
		последнее значение этого критерия.

	GOODS

	GOODSGROUP
	BRAND
	PERSONKIND
	PERSON
	CITY
	COUNTRY
SET
	QUOT
		KIND
		LOCATION
		SELLER
		BUYER
		GOODS
		CURRENCY
		VALUE
	GOODSARCODE
		GOODS
		SELLER
		VALUE
*/

void FASTCALL Backend_SelectObjectBlock::AddSrchCode_ByName(const SString & rArg)
{
	SString temp_buf;
	SrchCodeList.add(temp_buf.Cat(1).Comma().Cat(rArg));
}

void FASTCALL Backend_SelectObjectBlock::AddSrchCode_BySymb(const SString & rArg)
{
	SString temp_buf;
	SrchCodeList.add(temp_buf.Cat(2).Comma().Cat(rArg));
}

int Backend_SelectObjectBlock::CheckInCriterion(int criterion, int subcriterion, const SString & rArg)
{
	int    ok = 1, r;
	Reference * p_ref = PPRef;
	PPID   temp_id = 0;
	long   local_flags = 0;
	SString temp_buf;
	if(criterion == cFormat) {
		THROW_PP(oneof3(Operator, oSelect, oGetGoodsMatrix, oGetTSessPlaceStatus), PPERR_CMDSEL_MISSFORMATCRIT);
		THROW_PP(OutFormat == 0, PPERR_CMDSEL_DUPFORMATCRIT);
		switch(subcriterion) {
			case scXml: OutFormat = fmtXml; break;
			case scXmlUtf8: OutFormat = fmtXmlUtf8; break;
			case scBinary: OutFormat = fmtBinary; break;
			case scTddo:
				OutFormat = fmtTddo;
				THROW_PP(rArg.NotEmpty(), PPERR_CMDSEL_MISSTDDOTEMPLATE);
				OutTemplate = rArg;
				break;
			case scJson:
				OutFormat = fmtJson;
				THROW_PP(rArg.NotEmpty(), PPERR_CMDSEL_EXP_DL600STRUCTNAME);
				DL600StrucName = rArg;
			default:
				PPSetError(PPERR_CMDSEL_INVSUBCRITERION);
		}
	}
	else {
		switch(ObjType) {
			case PPOBJ_CCHECK:
				if(Operator == oCCheckCreate) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cPosNode:
							THROW(r = ResolveCrit_PosNode(subcriterion, rArg, &P_SetBlk->U.C.PosNodeID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTPOSNODE, rArg);
							break;
						case cLocation:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_SetBlk->U.C.LocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						case cSCard:
							THROW(r = ResolveCrit_SCard(subcriterion, rArg, &P_SetBlk->U.C.SCardID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTSCARD, rArg);
							break;
						case cAmount: P_SetBlk->U.C.Amount = rArg.ToReal(); break;
						case cDiscount: P_SetBlk->U.C.Discount = rArg.ToReal(); break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oCCheckAddLine) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cID: P_SetBlk->U.C.ID = rArg.ToLong(); break;
						case cQuantity: P_SetBlk->U.C.Qtty = rArg.ToReal(); break;
						case cPrice: P_SetBlk->U.C.Price = rArg.ToReal(); break;
						case cDiscount: P_SetBlk->U.C.Discount = rArg.ToReal(); break;
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_SetBlk->U.C.GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oCCheckFinish) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cID: P_SetBlk->U.C.ID = rArg.ToLong(); break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				break;
			case PPOBJ_BILL:
				if(Operator == oSelect) {
					SETIFZ(P_BillF, new BillFilt);
					switch(criterion) {
						case cPeriod:
							THROW(strtoperiod(rArg, &P_BillF->Period, 0));
							break;
						case cDate:
							{
								const LDATE dt = strtodate_((temp_buf = rArg).Strip(), DATF_DMY);
								THROW_SL(checkdate(dt));
								P_BillF->Period.SetDate(dt);
							}
							break;
						case cOp:
							THROW(ResolveCrit_OprKind(subcriterion, rArg, &P_BillF->OpID));
							break;
						case cLocation:
					   		{
								PPID   loc_id = 0;
								THROW(r = ResolveCrit_Loc(subcriterion, rArg, &loc_id));
								THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
								P_BillF->LocList.Add(loc_id);
							}
							break;
						case cArticle:
							// @v8.9.0 THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &(temp_id = 0)))
							THROW(ResolveCrit_Article(subcriterion, rArg, &(temp_id = 0))) // @v8.9.0
							P_BillF->ObjectID = temp_id;
							break;
						case cCurrency:
							THROW(ResolveCrit_Cur(subcriterion, rArg, &P_BillF->CurID));
							break;
						case cAgent:
							THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetAgentAccSheet(), &P_BillF->AgentID))
							break;
						case cCount:
							P_BillF->Count = rArg.ToLong();
							break;
						case cPage:
							THROW(ResolveCrit_Page(rArg));
							break;
						case cLast:
							P_BillF->Flags |= BillFilt::fDescOrder;
							break;
						case cID:
							IdList.addUnique(rArg.ToLong());
							break;
						case cUUID:
							THROW_MEM(SETIFZ(P_UuidList, new UuidArray));
							{
								S_GUID uuid;
								THROW_SL(uuid.FromStr(rArg));
								THROW_SL(P_UuidList->insert(&uuid));
							}
							break;
						case cSince:
							{
								LDATETIME since_dtm;
								THROW(ResolveCrit_Since(rArg, &since_dtm));
								if(since_dtm.d) {
									SETIFZ(P_BillF->P_SjF, new SysJournalFilt);
									P_BillF->P_SjF->ObjType = PPOBJ_BILL;
									P_BillF->P_SjF->Period.low = since_dtm.d;
									if(since_dtm.t)
										P_BillF->P_SjF->BegTm = since_dtm.t;
									P_BillF->P_SjF->ActionIDList.add(PPACN_OBJADD);
									P_BillF->P_SjF->ActionIDList.add(PPACN_TURNBILL);
								}
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oDraftTransitGoodsRest) {
					SETIFZ(P_DtGrF, new LocalDraftTransitGoodsRestFilt);
					switch(criterion) {
						case cDate:
							P_DtGrF->Dt = strtodate_((temp_buf = rArg).Strip(), DATF_DMY);
							THROW_SL(checkdate(P_DtGrF->Dt));
							break;
						case cLocation:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_DtGrF->LocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_DtGrF->GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oDraftTransitGoodsRestList) {
					SETIFZ(P_DtGrF, new LocalDraftTransitGoodsRestFilt);
					switch(criterion) {
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_DtGrF->GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oBillCreate) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cDate:
							P_SetBlk->U.B.Dt = strtodate_((temp_buf = rArg).Strip(), DATF_DMY);
							THROW_SL(checkdate(P_SetBlk->U.B.Dt));
							break;
						case cOp:
							THROW(ResolveCrit_OprKind(subcriterion, rArg, &P_SetBlk->U.B.OpID));
							break;
						case cCode:
							rArg.CopyTo(P_SetBlk->U.B.Code, sizeof(P_SetBlk->U.B.Code));
							break;
						case cLocation:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_SetBlk->U.B.LocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						case cArticle:
							// @v8.9.0 THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &(temp_id = 0)));
							THROW(ResolveCrit_Article(subcriterion, rArg, &(temp_id = 0))); // @v8.9.0
							P_SetBlk->U.B.ArID = temp_id;
							break;
						case cDlvrLoc:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_SetBlk->U.B.DlvrLocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						case cCurrency:
							THROW(ResolveCrit_Cur(subcriterion, rArg, &P_SetBlk->U.B.CurID));
							break;
						case cAgent:
							THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetAgentAccSheet(), &(temp_id = 0)))
							P_SetBlk->U.B.AgentID = temp_id;
							break;
						case cMemo:
							(P_SetBlk->Memo = rArg).Strip();
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oBillAddLine) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_SetBlk->U.B.GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						case cID: P_SetBlk->U.B.ID = rArg.ToLong(); break;
						case cSeries: rArg.CopyTo(P_SetBlk->U.B.Serial, sizeof(P_SetBlk->U.B.Serial)); break;
						case cQuantity: P_SetBlk->U.B.Qtty = rArg.ToReal(); break;
						case cPrice: P_SetBlk->U.B.Price = rArg.ToReal(); break;
						case cDiscount: P_SetBlk->U.B.Discount = rArg.ToReal(); break;
						case cAmount: P_SetBlk->U.B.Amount = rArg.ToReal(); break;
						case cTSession: P_SetBlk->U.B.TSessID = rArg.ToLong(); break;
						case cPlace: STRNSCPY(P_SetBlk->U.B.PlaceCode, rArg); break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oBillFinish) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cID: P_SetBlk->U.B.ID = rArg.ToLong(); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_DL600DATA:
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
			case PPOBJ_TAGVALUE:
				SETIFZ(P_TagBlk, new TagBlock);
				switch(criterion) {
					case cTag:
						THROW_PP_S(P_TagBlk->TagID == 0, PPERR_CMDSEL_DBLDECL_TAG, rArg);
						THROW(ResolveCrit_Tag(subcriterion, rArg, &P_TagBlk->TagID));
						break;
					case cPerson:
						THROW_PP_S(oneof2(P_TagBlk->ObjType, 0, PPOBJ_PERSON), PPERR_CMDSEL_MIXTAGOBJ, rArg);
						THROW(ResolveCrit_Person(subcriterion, rArg, &temp_id));
						P_TagBlk->ObjType = PPOBJ_PERSON;
						if(temp_id)
							IdList.addUnique(temp_id);
						break;
					case cGoods:
						THROW_PP_S(oneof2(P_TagBlk->ObjType, 0, PPOBJ_GOODS), PPERR_CMDSEL_MIXTAGOBJ, rArg);
						THROW(ResolveCrit_Goods(subcriterion, rArg, &temp_id));
						P_TagBlk->ObjType = PPOBJ_GOODS;
						if(temp_id)
							IdList.addUnique(temp_id);
						break;
					case cGlobalUser:
						THROW_PP_S(oneof2(P_TagBlk->ObjType, 0, PPOBJ_GLOBALUSERACC), PPERR_CMDSEL_MIXTAGOBJ, rArg);
						THROW(ResolveCrit_GlobalUser(subcriterion, rArg, &temp_id));
						P_TagBlk->ObjType = PPOBJ_GLOBALUSERACC;
						if(temp_id)
							IdList.addUnique(temp_id);
						break;
					/*
					case cUhttStore:
						THROW_PP_S(oneof2(P_TagBlk->ObjType, 0, PPOBJ_UHTTSTORE), PPERR_CMDSEL_MIXTAGOBJ, rArg);
						THROW(ResolveCrit_UhttStore(subcriterion, rArg, &temp_id, &local_flags));
						P_TagBlk->ObjType = PPOBJ_UHTTSTORE;
						if(temp_id)
							IdList.addUnique(temp_id);
						break;
					*/
					case cValue:
						THROW_PP(oneof3(Operator, oSetObjectTag, oIncObjectTag, oDecObjectTag), PPERR_CMDSEL_INVCRITERION);
						(P_TagBlk->Value = rArg).Strip();
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_QUOT2:
				if(Operator == oSelect) {
					SETIFZ(P_QF, new LocalQuotFilt);
					switch(criterion) {
						case cActual: P_QF->Flags |= QuotFilt::fActualOnly; break;
						case cKind:
						case cQuotKind: THROW(ResolveCrit_QuotKind(subcriterion, rArg, &P_QF->QuotKindID)); break;
						case cSeller:
						case cBuyer:
							THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &(temp_id = 0)))
							if(criterion == cSeller)
								P_QF->SellerID = temp_id;
							else if(criterion == cBuyer)
								P_QF->ArID = temp_id;
							break;
						case cArticle:
							THROW(ResolveCrit_Article(subcriterion, rArg, &(temp_id = 0)));
							P_QF->ArID = temp_id;
							break;
						case cLocation:
							THROW(ResolveCrit_Loc(subcriterion, rArg, &P_QF->LocID));
							break;
						case cLocWorld:
							temp_id = 0;
							switch(subcriterion) {
								case 0:
								case scID: temp_id = rArg.ToLong(); break;
								case scCode:
									{
										PPObjWorld w_obj;
										WorldTbl::Rec w_rec;
										if(w_obj.SearchByCode(rArg, &w_rec) > 0)
											temp_id = w_rec.ID;
									}
									break;
								default:
									CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
									break;
							}
							if(temp_id)
								P_QF->SellerLocWorldID = temp_id;
							break;
						case cGoods: THROW(ResolveCrit_Goods(subcriterion, rArg, &P_QF->GoodsID)); break;
						case cGoodsGroup: THROW(ResolveCrit_GoodsGrp(subcriterion, rArg, &P_QF->GoodsGrpID)); break;
						case cBrand: THROW(ResolveCrit_Brand(subcriterion, rArg, &P_QF->BrandID)); break;
						case cSubName: P_QF->GoodsSubText = rArg; break;
						case cCurrency: THROW(ResolveCrit_Cur(subcriterion, rArg, &P_QF->CurID)); break;
						case cPage: THROW(ResolveCrit_Page(rArg)); break;
						case cUhttStore:
							{
								PPID   uhtts_id = 0;
								THROW(ResolveCrit_UhttStore(subcriterion, rArg, &uhtts_id, &P_QF->SellerID, &P_QF->LocID, &local_flags));
								SETFLAG(P_QF->LocalFlags, P_QF->lfNonZeroDraftRestOnly, BIN(local_flags & PPUhttStore::fDontShowZeroRestItems));
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oSet) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cKind:
							THROW(r = ResolveCrit_QuotKind(subcriterion, rArg, &P_SetBlk->U.Q.QuotKindID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTQUOTKIND, rArg);
							break;
						case cLocation:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_SetBlk->U.Q.LocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						case cSeller:
						case cBuyer:
							THROW(r = ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &(temp_id = 0)))
							if(criterion == cSeller) {
								THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTSELLER, rArg);
								P_SetBlk->U.Q.SellerID = temp_id;
							}
							else if(criterion == cBuyer) {
								THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTBUYER, rArg);
								P_SetBlk->U.Q.BuyerID = temp_id;
							}
							break;
						case cArticle:
							THROW(ResolveCrit_Article(subcriterion, rArg, &(temp_id = 0)));
							P_SetBlk->U.Q.BuyerID = temp_id;
							break;
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_SetBlk->U.Q.GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						case cCurrency:
							THROW(r = ResolveCrit_Cur(subcriterion, rArg, &P_SetBlk->U.Q.CurID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTCUR, rArg);
							break;
						case cValue:
							P_SetBlk->U.Q.Quot = rArg.ToReal();
							break;
						case cUhttStore:
							{
								PPID   uhtts_id = 0;
								THROW(ResolveCrit_UhttStore(subcriterion, rArg, &uhtts_id, &P_SetBlk->U.Q.SellerID, &P_SetBlk->U.Q.LocID, &local_flags));
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oGetGoodsMatrix) {
					SETIFZ(P_QF, new LocalQuotFilt);
					switch(criterion) {
						case cLocation:
							{
								PPID   loc_id = 0;
								THROW(r = ResolveCrit_Loc(subcriterion, rArg, &loc_id));
								THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
								P_QF->LocList.Add(loc_id);
							}
							break;
						case cGoods:
							{
								THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_QF->GoodsID));
								THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				break;
			case PPOBJ_GOODSARCODE:
				if(Operator == oSelect) {
					SETIFZ(P_GoodsF, new GoodsFilt);
					P_GoodsF->Flags |= GoodsFilt::fShowArCode;
					// no break: управление передается в 'case PPOBJ_GOODS'
				}
				else if(Operator == oSet) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cSeller:
							THROW(r = ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &P_SetBlk->U.AC.SellerID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTSELLER, rArg);
							break;
						case cGoods:
							THROW(r = ResolveCrit_Goods(subcriterion, rArg, &P_SetBlk->U.AC.GoodsID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTGOODS, rArg);
							break;
						case cValue:
							rArg.CopyTo(P_SetBlk->U.AC.Code, sizeof(P_SetBlk->U.AC.Code));
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
					break;
				}
				// no break: для Operator == oSelect управление передается в 'case PPOBJ_GOODS'
			case PPOBJ_GOODS:
				SETIFZ(P_GoodsF, new GoodsFilt);
				switch(criterion) {
					case cID:
						IdList.addUnique(rArg.ToLong());
						break;
					case cTagExist:
						{
							PPID   tag_id = 0;
							THROW(ResolveCrit_Tag(subcriterion, rArg, &tag_id));
							TagExistList.addUnique(tag_id);
						}
						break;
					case cTagValue:
						{
							SString norm_buf;
							PPID   tag_id = subcriterion;
							THROW_PP(tag_id, PPERR_CMDSEL_UNDEFTAGINTAGVALCRIT);
							THROW_PP(rArg.NotEmpty(), PPERR_CMDSEL_UNDEFVALINTAGVALCRIT);
							THROW_PP_S(TagObj.NormalizeTextCriterion(tag_id, rArg, norm_buf) != 0, PPERR_CMDSEL_INVTAGVALCRIT, rArg);
							TagFilt::SetRestriction(norm_buf, temp_buf.Z());
							SETIFZ(P_GoodsF->P_TagF, new TagFilt);
							THROW_MEM(P_GoodsF->P_TagF);
							P_GoodsF->P_TagF->TagsRestrict.Add(tag_id, temp_buf);
						}
						break;
					case cName: P_GoodsF->PutExtssData(P_GoodsF->extssNameText, rArg); break;
					case cSubName: P_GoodsF->PutExtssData(P_GoodsF->extssNameText, rArg); break;
					case cParent:
						temp_id = 0;
						THROW(ResolveCrit_GoodsGrp(subcriterion, rArg, &temp_id));
						if(temp_id)
							P_GoodsF->GrpIDList.Add(temp_id);
						break;
					case cBrand:
						temp_id = 0;
						THROW(ResolveCrit_Brand(subcriterion, rArg, &temp_id));
						P_GoodsF->BrandList.Add(temp_id);
						break;
					case cManuf:
						THROW(ResolveCrit_Person(subcriterion, rArg, &temp_id));
						if(temp_id) {
							P_GoodsF->ManufID = temp_id;
							//P_GoodsF->Flags &= ~GoodsFilt::fHidePassive;
						}
						break;
					case cUhttStore:
						THROW(ResolveCrit_UhttStore(subcriterion, rArg, &temp_id, 0, 0, 0));
						if(temp_id)
							P_GoodsF->UhttStoreID = temp_id;
						break;
					case cQuotKind:
						THROW(ResolveCrit_QuotKind(subcriterion, rArg, &temp_id));
						if(temp_id)
							P_GoodsF->RestrictQuotKindID = temp_id;
						break;
					case cClass:
						THROW(ResolveCrit_GoodsClass(subcriterion, rArg, &temp_id));
						if(temp_id)
							P_GoodsF->Ep.GdsClsID = temp_id;
						break;
					case cGcDimX: strtorrng(rArg, P_GoodsF->Ep.DimX_Rng); break;
					case cGcDimY: strtorrng(rArg, P_GoodsF->Ep.DimY_Rng); break;
					case cGcDimZ: strtorrng(rArg, P_GoodsF->Ep.DimZ_Rng); break;
					case cGcGimW: strtorrng(rArg, P_GoodsF->Ep.DimW_Rng); break;
					case cGcKind:
						THROW(ResolveCrit_GoodsProp(P_GoodsF->Ep.GdsClsID, PPGdsCls::eKind, subcriterion, rArg, P_GoodsF->Ep.KindList));
						break;
					case cGcGrade:
						THROW(ResolveCrit_GoodsProp(P_GoodsF->Ep.GdsClsID, PPGdsCls::eGrade, subcriterion, rArg, P_GoodsF->Ep.GradeList));
						break;
					case cGcAdd:
						THROW(ResolveCrit_GoodsProp(P_GoodsF->Ep.GdsClsID, PPGdsCls::eAdd, subcriterion, rArg, P_GoodsF->Ep.AddObjList));
						break;
					case cGcAdd2:
						THROW(ResolveCrit_GoodsProp(P_GoodsF->Ep.GdsClsID, PPGdsCls::eAdd2, subcriterion, rArg, P_GoodsF->Ep.AddObj2List));
						break;
					case cSeller:
						THROW(ResolveCrit_ArByPerson(subcriterion, rArg, GetSupplAccSheet(), &temp_id));
						P_GoodsF->CodeArID = temp_id;
						break;
					case cCode:
					case cBarcode:
						AddSrchCode_BySymb(rArg);
						break;
					case cArCode:
						SrchCodeList.add(temp_buf.Z().Cat(3).Comma().Cat(rArg));
						break;
					case cActual:
						P_GoodsF->Flags |= GoodsFilt::fActualOnly;
						break;
					case cMatrix:
						P_GoodsF->Flags |= GoodsFilt::fRestrictByMatrix;
						break;
					case cMatrixLoc:
						P_GoodsF->Flags |= GoodsFilt::fRestrictByMatrix;
						THROW(ResolveCrit_Loc(subcriterion, rArg, &temp_id));
						if(temp_id)
							P_GoodsF->MtxLocID = temp_id;
						break;
					case cPassive:
						if(rArg.OneOf(';', "no;false;0;default", 1))
							P_GoodsF->Flags |= GoodsFilt::fHidePassive;
						else
							P_GoodsF->Flags &= ~GoodsFilt::fHidePassive;
						break;
						// @v10.7.7 {
					case cGeneric:
						if(rArg.OneOf(';', "no;false;0;default", 1))
							P_GoodsF->Flags |= GoodsFilt::fHideGeneric;
						else
							P_GoodsF->Flags &= ~GoodsFilt::fHideGeneric;
						break;
						// } @v10.7.7 
					case cLocation:
						THROW(ResolveCrit_Loc(subcriterion, rArg, &temp_id));
						P_GoodsF->LocList.Add(temp_id);
						break;
					case cPage:
						THROW(ResolveCrit_Page(rArg));
						break;
					case cPosNode:
						{
							PPIDArray grp_list;
							THROW(ResolveCrit_GoodsGrpListByPosNode(subcriterion, rArg, &grp_list));
							P_GoodsF->GrpIDList.Set(&grp_list);
						}
						break;
					case cSeparate:
						Separate = rArg.ToLong();
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_GOODSGROUP:
				SETIFZ(P_GgF, new LocalGoodsGroupFilt);
				switch(criterion) {
					case 0:
					case cID:
						IdList.addUnique(rArg.ToLong());
						break;
					case cName:
						P_GgF->Name = rArg;
						break;
					case cSubName:
						P_GgF->Name = rArg;
						P_GgF->Flags |= LocalGoodsGroupFilt::fSubName;
						break;
					case cCode:
						AddSrchCode_BySymb(rArg);
						break;
					case cParent:
						THROW(ResolveCrit_GoodsGrp(subcriterion, rArg, &P_GgF->ParentID));
						break;
					case cPage:
						THROW(ResolveCrit_Page(rArg));
						break;
					case cPosNode:
						THROW(ResolveCrit_GoodsGrpListByPosNode(subcriterion, rArg, &IdList));
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_BRAND:
				SETIFZ(P_BrF, new LocalBrandFilt);
				switch(criterion) {
					case cID:
						IdList.addUnique(rArg.ToLong());
						break;
					case cName:
						P_BrF->Name = rArg;
						break;
					case cSubName:
						P_BrF->Name = rArg;
						P_BrF->Flags |= LocalBrandFilt::fSubName;
						break;
					case cOwner:
						THROW(ResolveCrit_Person(subcriterion, rArg, &P_BrF->OwnerID));
						break;
					case cPage:
						THROW(ResolveCrit_Page(rArg));
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_PERSON:
				if(Operator == oCreate) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cName: rArg.CopyTo(P_SetBlk->U.P.Name, sizeof(P_SetBlk->U.P.Name)); break;
						case cStatus:
							THROW(ResolveCrit_PersonStatus(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_SetBlk->U.P.StatusID = temp_id;
							break;
						case cKind:
							THROW(ResolveCrit_PersonKind(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_SetBlk->U.P.KindList[P_SetBlk->U.P.KindCount++] = temp_id;
							break;
						case cCategory:
							THROW(ResolveCrit_PersonCat(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_SetBlk->U.P.CatID = temp_id;
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(Operator == oSelect) {
					SETIFZ(P_PsnF, new PersonFilt);
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName:
							P_PsnF->PutExtssData(PersonFilt::extssNameText, rArg);
							P_PsnF->Flags |= PersonFilt::fPrecName;
							break;
						case cSubName: P_PsnF->PutExtssData(PersonFilt::extssNameText, rArg); break;
						case cKind: THROW(ResolveCrit_PersonKind(subcriterion, rArg, &P_PsnF->Kind)); break;
						case cStatus: THROW(ResolveCrit_PersonStatus(subcriterion, rArg, &P_PsnF->Status)); break;
						case cCategory: THROW(ResolveCrit_PersonCat(subcriterion, rArg, &P_PsnF->Category)); break;
						case cRegister:
							SETIFZ(P_PsnF->P_RegF, new RegisterFilt);
							{
								SString reg_type_buf, number_buf, serial_buf;
								StringSet ss(',', rArg);
								uint j = 0;
								if(ss.get(&j, reg_type_buf))
									if(ss.get(&j, number_buf))
										ss.get(&j, serial_buf);
								reg_type_buf.Strip();
								serial_buf.Strip();
								THROW_PP(number_buf.NotEmptyS(), PPERR_CMDSEL_REGNUMBERARGEMPTY);
								switch(subcriterion) {
									case 0:
									case scDef:
										THROW_PP(P_PsnF->Kind, PPERR_CMDSEL_UNDEFPSNKFORDEFREG);
										{
											PPObjPersonKind pk_obj;
											PPPersonKind2 pk_rec;
											THROW_PP(pk_obj.Fetch(P_PsnF->Kind, &pk_rec) > 0 && pk_rec.CodeRegTypeID, PPERR_CMDSEL_PSNKHASNTSRCHREGT);
											P_PsnF->P_RegF->RegTypeID = pk_rec.CodeRegTypeID;
										}
										break;
									case scID:
										P_PsnF->P_RegF->RegTypeID = reg_type_buf.ToLong();
										break;
									case scCode:
										{
											PPObjRegisterType rt_obj;
											if(rt_obj.SearchSymb(&(temp_id = 0), reg_type_buf) > 0)
												P_PsnF->P_RegF->RegTypeID = temp_id;
											else
												; // @error register type code not found
										}
										break;
									default:
										CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
										break;
								}
								P_PsnF->P_RegF->NmbPattern = number_buf;
								P_PsnF->P_RegF->SerPattern = serial_buf;
							}
							break;
						case cPage: THROW(ResolveCrit_Page(rArg)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				else if(Operator == oSetPersonRel) {
					SETIFZ(P_PsnRelF, new PersonRelFilt);
					switch(criterion) {
						case cPrimary: THROW(ResolveCrit_Person(subcriterion, rArg, &P_PsnRelF->PrmrPersonID)); break;
						case cSecondary: THROW(ResolveCrit_Person(subcriterion, rArg, &P_PsnRelF->ScndPersonID)); break;
						case cPersonRelType: THROW(ResolveCrit_PersonRelType(subcriterion, rArg, &P_PsnRelF->RelTypeID)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				else if(Operator == oGetPersonRel) {
					SETIFZ(P_PsnRelF, new PersonRelFilt);
					switch(criterion) {
						case cPrimary: THROW(ResolveCrit_Person(subcriterion, rArg, &P_PsnRelF->PrmrPersonID)); break;
						case cSecondary: THROW(ResolveCrit_Person(subcriterion, rArg, &P_PsnRelF->ScndPersonID)); break;
						case cPersonRelType: THROW(ResolveCrit_PersonRelType(subcriterion, rArg, &P_PsnRelF->RelTypeID)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_PERSONKIND:
			case PPOBJ_PRSNCATEGORY:
			case PPOBJ_PRSNSTATUS:
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					case cName: AddSrchCode_ByName(rArg); break;
					case cCode: AddSrchCode_BySymb(rArg); break;
					case cPage: THROW(ResolveCrit_Page(rArg)); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
			case PPOBJ_GLOBALUSERACC:
				if(Operator == oCreate) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cName: rArg.CopyTo(P_SetBlk->U.GA.Name, sizeof(P_SetBlk->U.GA.Name)); break;
						case cPassword: rArg.CopyTo(P_SetBlk->U.GA.Password, sizeof(P_SetBlk->U.GA.Password)); break;
						case cOwner: THROW(ResolveCrit_Person(subcriterion, rArg, &P_SetBlk->U.GA.PersonID)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				else if(Operator == oSelect) {
					SETIFZ(P_GaF, new LocalGlobalAccFilt);
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						case cOwner:
							THROW(ResolveCrit_Person(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_GaF->OwnerList.add(temp_id);
							break;
						case cPage: THROW(ResolveCrit_Page(rArg)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_LOCATION:
				SETIFZ(P_LocF, new LocationFilt);
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					case cCode: SrchCodeList.add(rArg); break;
					case cOwner:
						THROW(ResolveCrit_Person(subcriterion, rArg, &temp_id));
						if(temp_id)
							P_LocF->Owner = temp_id;
						break;
					case cPhone:
						P_LocF->LocType = LOCTYP_ADDRESS;
						P_LocF->SetExField(LocationFilt::exfPhone, rArg);
						break;
					case cPage:
						THROW(ResolveCrit_Page(rArg));
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_WORLD:
				SETIFZ(P_WrldF, new PPObjWorld::SelFilt);
				P_WrldF->KindFlags = (ObjTypeExt == WORLDOBJ_COUNTRY) ? WORLDOBJ_COUNTRY : WORLDOBJ_CITY;
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					case cCode: AddSrchCode_BySymb(rArg); break;
					case cName: AddSrchCode_ByName(rArg); break;
					case cSubName: P_WrldF->SubName = rArg; break;
					case cParent:
						switch(subcriterion) {
							case 0:
							case scID: P_WrldF->ParentID = rArg.ToLong(); break;
							case scCode:
								{
									PPObjWorld w_obj;
									WorldTbl::Rec w_rec;
									if(w_obj.SearchByCode(rArg, &w_rec) > 0)
										P_WrldF->ParentID = w_rec.ID;
								}
								break;
							default:
								CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
								break;
						}
						break;
					case cPage:
						THROW(ResolveCrit_Page(rArg));
						break;
					default:
						CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
						break;
				}
				break;
			case PPOBJ_CURRENCY:
			case PPOBJ_CURRATETYPE:
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					case cName: AddSrchCode_ByName(rArg); break;
					case cCode: AddSrchCode_BySymb(rArg); break;
					case cPage: THROW(ResolveCrit_Page(rArg)); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
			case PPOBJ_CURRATEIDENT:
				SETIFZ(P_CurRateF, new CurRateFilt());
				switch(criterion) {
					case cCurrency: THROW(ResolveCrit_Cur(subcriterion, rArg, &P_CurRateF->CurID)); break;
					case cActual: P_CurRateF->Flags |= CurRateFilt::fActualOnly; break;
					case cPeriod: THROW(strtoperiod(rArg, &P_CurRateF->Period, 0)); break;
					case cType: THROW(ResolveCrit_CurRateType(subcriterion, rArg, &P_CurRateF->RateTypeID)); break;
					case cPage: THROW(ResolveCrit_Page(rArg)); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
			case PPOBJ_UNIT:
				switch(criterion) {
					case cID: IdList.addUnique(rArg.ToLong()); break;
					case cName: AddSrchCode_ByName(rArg); break;
					case cAndFlags: ExtFiltFlags = rArg.ToLong(); break;
					case cPage: THROW(ResolveCrit_Page(rArg)); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
			case PPOBJ_SPECSERIES:
				SETIFZ(P_SpecSerF, new SpecSeriesFilt());
				switch(criterion) {
					case cCode: P_SpecSerF->Serial = rArg; break;
					case cGoods: THROW(ResolveCrit_Goods(subcriterion, rArg, &P_SpecSerF->GoodsID)); break;
					case cPeriod: THROW(strtoperiod(rArg, &P_SpecSerF->Period, 0)); break;
					case cPage: THROW(ResolveCrit_Page(rArg)); break;
					default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
				}
				break;
/*
PROCESSOR
	id(text)
	code(text)
	name(text)
	subname(text)
	parent(text)
		crit_modif
			id
			code
			name

TSESSION
	id(text)
	code(text)
	processor(text)
		crit_modif:
			id
			code
			name
	status(text)
		planned pending inprocess closed canceled
	since(datetime)
    before(datetime)

STYLOPALM
	id(text)
	code(text)
	name(text)
	parent(text)
		crit_modif
			id
			code
			name
*/
			case PPOBJ_PROCESSOR:
				if(Operator == oSelect) {
					THROW_MEM(SETIFZ(P_PrcF, new ProcessorFilt()));
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						case cSubName:
							// @todo
							break;
						case cParent:
							THROW(r = ResolveCrit_Processor(subcriterion, rArg, &P_PrcF->ParentID));
							break;
						case cLocation:
							THROW(r = ResolveCrit_Loc(subcriterion, rArg, &P_PrcF->LocID));
							THROW_PP_S(r > 0, PPERR_CMDSEL_UNIDENTLOC, rArg);
							break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_TSESSION:
				if(oneof2(Operator, oTSessCipCheckIn, oTSessCipCancel)) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cID:
							THROW_PP(!P_SetBlk->U.TS.TSessID && P_SetBlk->U.TS.TSessUUID.IsZero(), PPERR_TSESSCIPOP_TSESSAMB)
							P_SetBlk->U.TS.TSessID = rArg.ToLong();
							break;
						case cUUID:
							THROW_PP(!P_SetBlk->U.TS.TSessID && P_SetBlk->U.TS.TSessUUID.IsZero(), PPERR_TSESSCIPOP_TSESSAMB)
							THROW_SL(P_SetBlk->U.TS.TSessUUID.FromStr(rArg));
							break;
						case cUhttStore:
							{
								PPID   uhtts_id = 0;
								PPID   seller_id = 0;
								PPID   loc_id = 0;
								THROW(ResolveCrit_UhttStore(subcriterion, rArg, &uhtts_id, &seller_id, &loc_id, &local_flags));
								P_SetBlk->U.TS.UhttStoreID = uhtts_id;
							}
							break;
						case cQuotKind:
							THROW(ResolveCrit_QuotKind(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_SetBlk->U.TS.QuotKindID = temp_id;
							break;
						case cPlace:
							STRNSCPY(P_SetBlk->U.TS.PlaceCode, rArg);
							break;
						case cPinCode:
							STRNSCPY(P_SetBlk->U.TS.PinCode, rArg);
                            break;
						case cCip:
							P_SetBlk->U.TS.CipID = rArg.ToLong();
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(oneof2(Operator, oSelect, oGetTSessPlaceStatus)) {
					THROW_MEM(SETIFZ(P_TSesF, new TSessionFilt));
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						case cUUID:
							THROW_MEM(SETIFZ(P_UuidList, new UuidArray));
							{
								S_GUID uuid;
								THROW_SL(uuid.FromStr(rArg));
								THROW_SL(P_UuidList->insert(&uuid));
							}
							break;
						case cParent:
							switch(subcriterion) {
								case 0:
								case scID:
									//P_WrldF->ParentID = rArg.ToLong();
									break;
								case scCode:
									{
										/*
										PPObjWorld w_obj;
										WorldTbl::Rec w_rec;
										if(w_obj.SearchByCode(rArg, &w_rec) > 0)
											P_WrldF->ParentID = w_rec.ID;
										*/
									}
									break;
								case scName:
									break;
								default:
									CALLEXCEPT_PP(PPERR_CMDSEL_INVSUBCRITERION);
									break;
							}
							break;
						case cProcessor:
							THROW(r = ResolveCrit_Processor(subcriterion, rArg, &P_TSesF->PrcID));
							break;
						case cStatus:
							THROW(ResolveCrit_TSessStatus(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_TSesF->StatusFlags |= (1 << temp_id);
							break;
						case cSince:
							{
								LDATETIME since_dtm;
								THROW(ResolveCrit_Since(rArg, &since_dtm));
								P_TSesF->StPeriod.low = since_dtm.d;
								P_TSesF->StTime = since_dtm.t;
							}
                            break;
						case cUhttStore:
							{
								PPID   uhtts_id = 0;
								PPID   seller_id = 0;
								PPID   loc_id = 0;
								THROW(ResolveCrit_UhttStore(subcriterion, rArg, &uhtts_id, &seller_id, &loc_id, &local_flags));
								P_TSesF->UhttStoreID = uhtts_id;
							}
							break;
						case cQuotKind:
							THROW(ResolveCrit_QuotKind(subcriterion, rArg, &temp_id));
							if(temp_id)
								P_TSesF->QuotKindID = temp_id;
							break;
						case cPlace:
							if(Operator == oGetTSessPlaceStatus) {
								THROW_MEM(SETIFZ(P_LocTspF, new LocalTSessPlaceFilt));
								STRNSCPY(P_LocTspF->PlaceCode, rArg);
							}
							else {
								CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							}
							break;
						case cSeparate:
							Separate = rArg.ToLong();
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				break;
			case PPOBJ_STYLOPALM:
				if(Operator == oSelect) {
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_SCARD:
				if(Operator == oSelect) {
					SETIFZ(P_SCardF, new SCardFilt());
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cOwner: P_SCardF->PersonID = rArg.ToLong(); break;
						case cSeries: P_SCardF->SeriesID = rArg.ToLong(); break;
						case cCode: AddSrchCode_ByName(rArg); break;
						case cSubName:
							{
								(temp_buf = rArg).Strip();
								if(temp_buf[0] == '*') {
									temp_buf.ShiftLeft();
								}
								else {
									P_SCardF->Flags |= SCardFilt::fNumberFromBeg;
								}
								if(temp_buf.NotEmptyS()) {
									P_SCardF->Number = temp_buf;
								}
							}
							break;
						case cPage: THROW(ResolveCrit_Page(rArg)); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				else if(Operator == oSCardRest) {
					SCardTbl::Rec sc_rec;
					SETIFZ(P_SetBlk, new SetBlock);
					SETIFZ(P_ScObj, new PPObjSCard);
					THROW_MEM(P_ScObj);
					switch(criterion) {
						case cID:
							THROW_PP(P_SetBlk->U.SC.ID == 0, PPERR_CMDSEL_ONLYONECRITOBJENABLED);
							P_SetBlk->U.SC.ID = rArg.ToLong();
							THROW(P_ScObj->Search(P_SetBlk->U.SC.ID, &sc_rec) > 0);
							break;
						case cCode:
							{
							// @Muxa @v7.3.9 {
								SString number, hash, temp_buf, prefix;
								PPObjGlobalUserAcc gua_obj;
								PPGlobalUserAcc    gua_rec;
								ObjTagItem         tag;
								SString            temp_hash;
								PPGlobalAccRights  rights_blk(PPTAG_GUA_SCARDRIGHTS);
								THROW_PP(P_SetBlk->U.SC.ID == 0, PPERR_CMDSEL_ONLYONECRITOBJENABLED);
								if(rights_blk.IsAllow(PPGlobalAccRights::fOperation)) {
									rArg.Divide('@', number, hash);
									if(P_ScObj->SearchCode(0, number, &sc_rec) > 0) {
										P_SetBlk->U.SC.ID = sc_rec.ID;
									}
									else {
										if(DS.GetTLA().GlobAccID > 0) {
											if(p_ref->Ot.GetTag(PPOBJ_GLOBALUSERACC, DS.GetTLA().GlobAccID, PPTAG_GUA_SCARDPREFIX, &tag) > 0) {
												prefix = tag.Val.PStr;
												if(prefix.NotEmpty()) {
													temp_buf.Z().Cat(prefix).Cat(number);
													if(P_ScObj->SearchCode(0, temp_buf, &sc_rec) > 0) {
														P_SetBlk->U.SC.ID = sc_rec.ID;
													}
												}
											}
										}
									}
								}
								else {
									int   r = rArg.Divide('@', number, hash);
									THROW_PP(r > 0, PPERR_INVSCARDNUM);
									if(P_ScObj->SearchCode(0, number, &sc_rec) > 0) {
										PPObjSCard::CalcSCardHash(number, temp_hash);
										THROW_PP(temp_hash == hash, PPERR_INVSCARDHASH);
										P_SetBlk->U.SC.ID = sc_rec.ID;
									}
									else {
										for(SEnum en = gua_obj.P_Ref->Enum(PPOBJ_GLOBALUSERACC, 0); en.Next(&gua_rec) > 0;) {
											if(p_ref->Ot.GetTag(PPOBJ_GLOBALUSERACC, gua_rec.ID, PPTAG_GUA_SCARDPREFIX, &tag) > 0) {
												prefix = tag.Val.PStr;
												if(prefix.NotEmpty()) {
													temp_buf.Z().Cat(prefix).Cat(number);
													if(P_ScObj->SearchCode(0, temp_buf, &sc_rec) > 0) {
														PPObjSCard::CalcSCardHash(temp_buf, temp_hash);
														THROW_PP(temp_hash == hash, PPERR_INVSCARDHASH);
														P_SetBlk->U.SC.ID = sc_rec.ID;
														break;
													}
												}
											}
										}
									}
								}
								THROW_PP_S(P_SetBlk->U.SC.ID > 0, PPERR_SCARDNOTFOUND, number);
							// } @Muxa @v7.3.9
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				else if(oneof2(Operator, oSCardDeposit, oSCardWithdraw)) {
					SCardTbl::Rec sc_rec;
					SETIFZ(P_SetBlk, new SetBlock);
					SETIFZ(P_ScObj, new PPObjSCard);
					THROW_MEM(P_ScObj);
					switch(criterion) {
						case cID:
							THROW_PP_S(P_SetBlk->U.SC.ID == 0, PPERR_CMDSEL_ONLYONECRITOBJENABLED, rArg);
							P_SetBlk->U.SC.ID = rArg.ToLong();
							THROW(P_ScObj->Search(P_SetBlk->U.SC.ID, &sc_rec) > 0);
							break;
						case cCode:
							{
								THROW_PP(rArg.NotEmpty(), PPERR_INVSCARDNUM);
								THROW_PP_S(P_SetBlk->U.SC.ID == 0, PPERR_CMDSEL_ONLYONECRITOBJENABLED, rArg);
								if(P_ScObj->SearchCode(0, rArg, &sc_rec) > 0) {
									P_SetBlk->U.SC.ID = sc_rec.ID;
								}
								else {
									if(DS.GetTLA().GlobAccID > 0) {
										SString     temp_buf, prefix;
										ObjTagItem  tag;
										if(p_ref->Ot.GetTag(PPOBJ_GLOBALUSERACC, DS.GetTLA().GlobAccID, PPTAG_GUA_SCARDPREFIX, &tag) > 0) {
											prefix = tag.Val.PStr;
											if(prefix.NotEmpty()) {
												temp_buf.Z().Cat(prefix).Cat(rArg);
												if(P_ScObj->SearchCode(0, temp_buf, &sc_rec) > 0) {
													P_SetBlk->U.SC.ID = sc_rec.ID;
												}
											}
										}
									}
								}
								THROW_PP_S(P_SetBlk->U.SC.ID > 0, PPERR_SCARDNOTFOUND, rArg);
							}
							break;
						case cAmount:
							P_SetBlk->U.SC.Amount = rArg.ToReal();
							THROW_PP_S(P_SetBlk->U.SC.Amount > 0.0, PPERR_CMDSEL_INVAMOUNT, rArg);
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				break;
			case PPOBJ_UHTTSCARDOP:
				if(Operator == oSelect) {
					SETIFZ(P_UhttSCardOpF, new UhttSCardOpFilt());
					switch(criterion) {
						case cPeriod: THROW(strtoperiod(rArg, &P_UhttSCardOpF->Period, 0)); break;
						case cGrouping: P_UhttSCardOpF->Grp = rArg.ToLong(); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_UHTTSTORE:
				if(Operator == oSelect) {
					SETIFZ(P_UhttStorF, new LocalUhttStoreFilt);
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						case cOwner: P_UhttStorF->OwnerID = rArg.ToLong(); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_OPRKIND:
				if(Operator == oSelect) {
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_GTA:
				if(Operator == oGtaCheckIn) {
					SETIFZ(P_SetBlk, new SetBlock);
					switch(criterion) {
						case cGlobalUser:
							THROW(ResolveCrit_GlobalUser(subcriterion, rArg, &P_SetBlk->U.GT.GlobalUserID));
							break;
						case cObjType:
							{
								long   objtypeext = 0;
								THROW(P_SetBlk->U.GT.Oi.Obj = GetObjectTypeBySymb(rArg, &objtypeext));
							}
							break;
						case cObjID:
							P_SetBlk->U.GT.Oi.Id = rArg.ToLong();
							break;
						case cGtaOp:
							{
								// @v10.5.7 {
								static const SIntToSymbTabEntry gta_op_list[] = {
									{ GTAOP_NOOP, "NOOP" },
									{ GTAOP_OBJGET, "OBJGET" },
									{ GTAOP_OBJADD, "OBJADD" },
									{ GTAOP_OBJMOD, "OBJMOD" },
									{ GTAOP_OBJRMV, "OBJRMV" },
									{ GTAOP_CCHECKCREATE, "CCHECKCREATE" },
									{ GTAOP_SCARDWITHDRAW, "SCARDWITHDRAW" },
									{ GTAOP_SCARDDEPOSIT, "SCARDDEPOSIT" },
									{ GTAOP_FILEUPLOAD, "FILEUPLOAD" },
									{ GTAOP_FILEDOWNLOAD, "FILEDOWNLOAD" },
									{ GTAOP_BILLCREATE, "BILLCREATE" },
									{ GTAOP_SMSSEND, "SMSSEND" }
								};
								P_SetBlk->U.GT.GtaOp = SIntToSymbTab_GetId(gta_op_list, SIZEOFARRAY(gta_op_list), rArg);
								SETIFZ(P_SetBlk->U.GT.GtaOp, GTAOP_NOOP);
								// } @v10.5.7 
								/* @v10.5.7 
								if(rArg.IsEqiAscii("NOOP"))
									P_SetBlk->U.GT.GtaOp = GTAOP_NOOP;
								else if(rArg.IsEqiAscii("OBJGET"))
									P_SetBlk->U.GT.GtaOp = GTAOP_OBJGET;
								else if(rArg.IsEqiAscii("OBJADD"))
									P_SetBlk->U.GT.GtaOp = GTAOP_OBJADD;
								else if(rArg.IsEqiAscii("OBJMOD"))
									P_SetBlk->U.GT.GtaOp = GTAOP_OBJMOD;
								else if(rArg.IsEqiAscii("OBJRMV"))
									P_SetBlk->U.GT.GtaOp = GTAOP_OBJRMV;
								else if(rArg.IsEqiAscii("CCHECKCREATE"))
									P_SetBlk->U.GT.GtaOp = GTAOP_CCHECKCREATE;
								else if(rArg.IsEqiAscii("SCARDWITHDRAW"))
									P_SetBlk->U.GT.GtaOp = GTAOP_SCARDWITHDRAW;
								else if(rArg.IsEqiAscii("SCARDDEPOSIT"))
									P_SetBlk->U.GT.GtaOp = GTAOP_SCARDDEPOSIT;
								else if(rArg.IsEqiAscii("FILEUPLOAD"))
									P_SetBlk->U.GT.GtaOp = GTAOP_FILEUPLOAD;
								else if(rArg.IsEqiAscii("FILEDOWNLOAD"))
									P_SetBlk->U.GT.GtaOp = GTAOP_FILEDOWNLOAD;
								else if(rArg.IsEqiAscii("BILLCREATE"))
									P_SetBlk->U.GT.GtaOp = GTAOP_BILLCREATE;
								else if(rArg.IsEqiAscii("SMSSEND"))
									P_SetBlk->U.GT.GtaOp = GTAOP_SMSSEND;
								else
									P_SetBlk->U.GT.GtaOp = GTAOP_NOOP;
								*/
							}
							break;
						case cCount:
							P_SetBlk->U.GT.Count = rArg.ToLong();
							break;
						case cDuration:
							{
								LTIME tm = ZEROTIME;
								strtotime(rArg, TIMF_HMS, &tm);
								P_SetBlk->U.GT.Duration = tm.totalsec();
							}
							break;
						default:
							CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION);
							break;
					}
				}
				break;
			case PPOBJ_WORKBOOK:
				if(Operator == oSelect) {
					SETIFZ(P_WorkbookF, new LocalWorkbookFilt);
					switch(criterion) {
						case cID: IdList.addUnique(rArg.ToLong()); break;
						case cName: AddSrchCode_ByName(rArg); break;
						case cCode: AddSrchCode_BySymb(rArg); break;
						case cParent: P_WorkbookF->ParentID = rArg.ToLong(); break;
						case cType: P_WorkbookF->Type = rArg.ToLong(); break;
						case cStripSSfx: P_WorkbookF->Flags |= LocalWorkbookFilt::fStripSSfx; break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			case PPOBJ_GEOTRACKING:
				if(Operator == oSelect) {
					SETIFZ(P_GeoTrF, new GeoTrackingFilt);
					switch(criterion) {
						case cPeriod: THROW(strtoperiod(rArg, &P_GeoTrF->Period, 0)); break;
						case cObjType:
							{
								long   objtypeext = 0;
								THROW(P_GeoTrF->Oi.Obj = GetObjectTypeBySymb(rArg, &objtypeext));
							}
							break;
						case cObjID:
							P_GeoTrF->Oi.Id = rArg.ToLong();
							break;
						default: CALLEXCEPT_PP(PPERR_CMDSEL_INVCRITERION); break;
					}
				}
				break;
			default:
				ok = 0;
				break;
		}
	}
	CATCHZOK
	return ok;
}
