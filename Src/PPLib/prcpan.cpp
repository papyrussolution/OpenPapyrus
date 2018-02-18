// PRCPAN.CPP
// Copyright (c) A.Sobolev 2005, 2006, 2007, 2010, 2011, 2013, 2015, 2016, 2017
//
#include <pp.h>
#pragma hdrstop

#define UNDEF_SIGN 10000

class PrcPaneDialog : public TDialog {
public:
	PrcPaneDialog();
	int    setDTS(PPID prcID);
private:
	//
	// Состояния панели процессоров
	//
	enum {
		sUNDEF = 0,             // Неопределенное (такое состояние недопустимо)
		sEMPTY_NOSESS,          // На процессоре нет активной сессии
		sEMPTY_SESS,            // На процессоре есть активная сессия //
		sGOODS_NOQTTY,          // Выбран товар - количество не определено
		sGOODS_QTTY,            // Выбран товар - количество определено
		sIDLE,                  // Состояние простоя процессора
		sREST,                  // Состояние ввода остатка партии на конец сессии
		sREST_SERIAL_NOQTTY,    // Ввод остатка - серийный номер определен - количество не определено
		sREST_SERIAL_QTTY,      // Ввод остатка - серийный номер определен - количество определено
		sEXPIRY,                // @v5.1.10 Ввод срока годности
		sCOST                   // @v5.2.0  Ввод стоимости
	};
	struct Header {
		PPID   PrcID;
		ProcessorTbl::Rec PrcRec;
		PPID   SessID;
		char   SessText[64];
		PPID   MainGoodsID;        // Основной товар технологии
		char   MainGoodsName[64];  // Наименование основного товара технологии
		double MainGoodsPack;      // Емкость упаковки основного товара
		PPID   LinkBillID;         // @v5.1.3 ИД связанного документа
	};
	struct Entry : public TSessLineTbl::Rec {
		char   GoodsName[64];
		double Pack;       // Емкость упаковки
		double PackQtty;   // Количество упаковок
	};
	DECL_HANDLE_EVENT;
	virtual int FASTCALL valid(ushort command);
	int    isRestState() const
	{
		return (State == sREST || State == sREST_SERIAL_NOQTTY || State == sREST_SERIAL_QTTY) ? 1 : 0;
	}
	int    setupProcessor(PPID prcID, int init);
	void   updateStatus(int forceUpdate);
	void   showMessage(int msgKind, int msgCode, const char * pAddMsg);
	void   clearPanel();
	//
	// ARG(mode IN):
	//   0 - вызывает диалог выбора товара
	//   1 - main goods of session
	//   2 - выбор по штрихкоду или серийному номеру
	//
	void   selectGoods(int mode);
	void   setupGoods(PPID goodsID);
	int    setupQtty(double qtty, int inPack);
	int    setupPrice(double price);
	void   setupSign();
	void   setupHint();
	void   setIdleMode(int set);
	void   clearInput(int selectOnly);
	int    getInput()
	{
		getCtrlString(CTL_PRCPAN_INPUT, Input);
		return Input.NotEmptyS() ? 1 : 0;
	}
	void   acceptQuantity(int inPack); // @<<processEnter
	void   ProcessEnter();
	int    processBill(const char * pInp);
	void   printLastLine()
	{
		if(State == sEMPTY_SESS && LastOprNo && H.SessID && H.PrcRec.PrinterID)
			if(!TSesObj.PrintBarLabel(H.SessID, LastOprNo, 1, 1))
				showMessage(mfError, 0, 0);
	}
	void   stopSession();
	int    switchProcessor(PPID objType, PPID objID);

	PPObjTSession TSesObj;
	PPObjGoods GObj;

	Header H;
	Entry  E;
	TGSArray TgsList;
	int    State;
	long   LastOprNo;
	SString Input;
	SString IdleContText;
	PPID   IdleSessID;
	PPID   NewGoodsGrpID;
	LDATETIME LastPrcStatusCheckTime;
	int    CanSwitch; // Если !0, то панель можно переключать между процессорами
	struct SwitchPrcEntry {
		PPID   BillID;
		PPID   PrcID;
		PPID   SessID;
	};
	TSArray <SwitchPrcEntry> SwitchPrcList;
};

/*
F2   - выбор товара
F10  - выбор основного товара
/999 - количество упаковок
*999 - количество единиц
*/

/*
Состояния панели:
1 - EMPTY_NOSESS Панель пустая. Нет активной сессии.
	Доступные операции:
	1.  ESC - выход
2 - EMPTY_SESS   Панель пустая. Сессия активна.
	Доступные операции:
	4.  Штрихкод или серийный номер - выбор товара по коду
	3.  F10 - выбор основного товара
	8.  F5  - перевести в режим простоя //
	10. STOP - остановить текущую сессию и ждать новую
	9.  F7  - печать последней строки
	1.  ESC - выход
3 - GOODS_NOQTTY Выбран товар. Количество - 0
	Доступные операции:
	4.  Штрихкод или серийный номер - выбор товара по коду
	5.  *количество - ввод количества торговых единиц
	6.  /количество - ввод количества упаковок (если задана емкость упаковки)
	3.  F10 - выбор основного товара
	1.  ESC - выход
4 - GOODS_QTTY   Выбран товар. Количество - не ноль
	Доступные операции:
	4.  Штрихкод или серийный номер - выбор товара по коду
	5.  *количество - ввод количества торговых единиц
	6.  /количество - ввод количества упаковок (если задана емкость упаковки)
	3.  F10 - выбор основного товара
	2.  ENTER - провести строку
	1.  ESC - выход
5 - IDLE         Простой
	7.  ENTER - восстановить рабочее состояние процессора
	1.  ESC - выход
6 - REST               Состояние ввода остатка партии на конец сессии
	4.  Штрихкод или серийный номер - выбор товара по коду
	12. ESC - выход из состояния ввода остатка
7 - REST_SERIAL_NOQTTY Ввод остатка - серийный номер определен - количество не определено
	4.  Штрихкод или серийный номер - выбор товара по коду
	5.  *количество - ввод количества торговых единиц
	6.  /количество - ввод количества упаковок (если задана емкость упаковки)
	12. ESC - выход из состояния ввода остатка
8 - REST_SERIAL_QTTY   Ввод остатка - серийный номер определен - количество определено
	4.  Штрихкод или серийный номер - выбор товара по коду
	5.  *количество - ввод количества торговых единиц
	6.  /количество - ввод количества упаковок (если задана емкость упаковки)
	2.  ENTER - провести строку
	12. ESC - выход из состояния ввода остатка
9 - EXPIRY             Ввод срока годности лота
	13. ДДММГГ - срок годности товара
	14. ESC - выход из состояния ввода срока годности
*/

void PrcPaneDialog::setupHint()
{
	PPIDArray hint_list;
	if(State == sUNDEF) {
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sEMPTY_NOSESS) {
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sEMPTY_SESS) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT03);
		hint_list.add(PPTXT_PRCPAN_HINT08);
		hint_list.add(PPTXT_PRCPAN_HINT09);
		hint_list.add(PPTXT_PRCPAN_HINT10);
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sGOODS_NOQTTY) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT05);
		hint_list.add(PPTXT_PRCPAN_HINT06);
		hint_list.add(PPTXT_PRCPAN_HINT03);
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sGOODS_QTTY) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT05);
		hint_list.add(PPTXT_PRCPAN_HINT06);
		hint_list.add(PPTXT_PRCPAN_HINT03);
		hint_list.add(PPTXT_PRCPAN_HINT02);
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sIDLE) {
		hint_list.add(PPTXT_PRCPAN_HINT07);
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	else if(State == sREST) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT12);
	}
	else if(State == sREST_SERIAL_NOQTTY) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT05);
		hint_list.add(PPTXT_PRCPAN_HINT06);
		hint_list.add(PPTXT_PRCPAN_HINT12);
	}
	else if(State == sREST_SERIAL_QTTY) {
		hint_list.add(PPTXT_PRCPAN_HINT04);
		hint_list.add(PPTXT_PRCPAN_HINT05);
		hint_list.add(PPTXT_PRCPAN_HINT06);
		hint_list.add(PPTXT_PRCPAN_HINT02);
		hint_list.add(PPTXT_PRCPAN_HINT12);
	}
	else if(State == sEXPIRY) {
		hint_list.add(PPTXT_PRCPAN_HINT13);
		hint_list.add(PPTXT_PRCPAN_HINT14);
	}
	else {
		hint_list.add(PPTXT_PRCPAN_HINT01);
	}
	SString temp_buf, hint, keyb;
	for(uint i = 0; i < CTL_PRCPAN_NUMHINTS; i++) {
		int    r = 0;
		if(i < hint_list.getCount()) {
			uint   idx = hint_list.at(i);
			if(PPLoadText(idx, temp_buf) > 0) {
				if(temp_buf.Divide('=', hint, keyb) > 0)
					setStaticText(CTL_PRCPAN_HINT1 + i + CTL_PRCPAN_KBHINTBIAS, keyb);
				setStaticText(CTL_PRCPAN_HINT1 + i, hint);
				r = 1;
			}
		}
		if(!r) {
			temp_buf.Z();
			setStaticText(CTL_PRCPAN_HINT1 + i, temp_buf);
			setStaticText(CTL_PRCPAN_HINT1 + i + CTL_PRCPAN_KBHINTBIAS, temp_buf);
		}
	}
}

PrcPaneDialog::PrcPaneDialog() : TDialog(DLG_PRCPAN), State(sUNDEF), NewGoodsGrpID(0), IdleSessID(0), LastOprNo(0), CanSwitch(0)
{
	SString font_face;
	MEMSZERO(H);
	MEMSZERO(E);
	LastPrcStatusCheckTime.SetZero();
	PPGetSubStr(PPTXT_FONTFACE, PPFONT_IMPACT, font_face);
	SetCtrlFont(CTL_PRCPAN_QTTY,      font_face, 32);
	SetCtrlFont(CTL_PRCPAN_QTTYPACK,  font_face, 32);
	SetCtrlFont(CTL_PRCPAN_TOTALQTTY, font_face, 32);
	SetCtrlFont(CTL_PRCPAN_TOTALPACK, font_face, 32);
	PPLoadText(PPTXT_PRCPAN_IDLECONT, IdleContText);
}

int FASTCALL PrcPaneDialog::valid(ushort command)
{
	int    r = 1;
	int    prev_state = State;
	if(command == cmCancel) {
		if(State == sREST) {
			State = sEMPTY_SESS;
			clearPanel();
			r = 0;
		}
		else if(State == sREST_SERIAL_NOQTTY) {
			State = sGOODS_NOQTTY;
			clearPanel();
			r = 0;
		}
		else if(State == sREST_SERIAL_QTTY) {
			State = sGOODS_QTTY;
			clearPanel();
			r = 0;
		}
		else if(E.GoodsID) {
			clearPanel();
			r = 0;
		}
		else if(PPMessage(mfConf|mfYesNo, PPCFM_CLOSEPRCPANE) == cmYes)
			r = 1;
		else
			r = 0;
	}
	else
		r = TDialog::valid(command);
	if(State != prev_state)
		setupHint();
	return r;
}

int PrcPaneDialog::setupProcessor(PPID prcID, int init)
{
	int    ok = 1;
	uint   i;
	MEMSZERO(H);
	MEMSZERO(E);
	H.PrcID = prcID;
	TSesObj.GetPrc(H.PrcID, &H.PrcRec, 1, 1);
	if(init) {
		CanSwitch = 0;
		SwitchPrcList.freeAll();
		PPIDArray sp_list;
		if(TSesObj.IsPrcSwitchable(H.PrcID, &sp_list) > 0) {
			CanSwitch = 1;
			for(i = 0; i < sp_list.getCount(); i++) {
				SwitchPrcEntry entry;
				entry.PrcID  = sp_list.at(i);
				entry.BillID = entry.SessID = 0;
				THROW_SL(SwitchPrcList.insert(&entry));
			}
		}
	}
	TgsList.Destroy();
	setCtrlString(CTL_PRCPAN_PRCNAME, H.PrcRec.Name);
	updateStatus(1);
	setupHint();
	CATCHZOK
	return ok;
}

int PrcPaneDialog::setDTS(PPID prcID)
{
	setStaticText(CTL_PRCPAN_ST_EXPIRY, 0);
	setStaticText(CTL_PRCPAN_ST_PRICE,  0);
	setStaticText(CTL_PRCPAN_ST_SUM,    0);
	return setupProcessor(prcID, 1);
}

void PrcPaneDialog::clearInput(int selectOnly)
{
	if(selectOnly) {
		TInputLine * p_il = (TInputLine*)getCtrlView(CTL_PRCPAN_INPUT);
		CALLPTRMEMB(p_il, selectAll(true));
	}
	else {
		Input = 0;
		setCtrlString(CTL_PRCPAN_INPUT, Input);
		selectCtrl(CTL_PRCPAN_INPUT);
	}
}

void PrcPaneDialog::acceptQuantity(int inPack)
{
	double qtty = Input.ToReal();
	if(setupQtty(qtty, inPack))
		clearInput(0);
	else
		showMessage(mfError, 0, 0);
}

void PrcPaneDialog::setIdleMode(int set)
{
	int    ok = 0;
	if(set) {
		if(State == sEMPTY_SESS) {
			PPID   idle_sess_id = 0;
			if(TSesObj.EditNewIdleSession(H.PrcID, H.SessID, &idle_sess_id) > 0) {
				IdleSessID = idle_sess_id;
				State = sIDLE;
			}
		}
	}
	else {
		if(State == sIDLE) {
			TSessionTbl::Rec rec;
			if(TSesObj.Search(IdleSessID, &rec) > 0 && TSesObj.SetSessionState(&rec, TSESST_CLOSED, 0) > 0) {
				getcurdatetime(&rec.FinDt, &rec.FinTm);
				if(TSesObj.PutRec(&IdleSessID, &rec, 1)) {
					IdleSessID = 0;
					updateStatus(1);
					ok = 1;
				}
			}
			if(!ok)
				showMessage(mfError, 0, 0);
		}
	}
}

void PrcPaneDialog::stopSession()
{
	int    ok = 1;
	if(oneof3(State, sEMPTY_SESS, sGOODS_NOQTTY, sGOODS_QTTY)) {
		if(CONFIRM(PPCFM_STOPTSESS)) {
			uint pos = 0;
			TSessionTbl::Rec rec;
			THROW(TSesObj.Search(H.SessID, &rec) > 0);
			THROW(TSesObj.SetSessionState(&rec, TSESST_CLOSED, 0) > 0);
			getcurdatetime(&rec.FinDt, &rec.FinTm);
			THROW(TSesObj.PutRec(&H.SessID, &rec, 1));
			if(SwitchPrcList.lsearch(&H.PrcID, &pos, CMPF_LONG, offsetof(SwitchPrcEntry, PrcID))) {
				SwitchPrcList.at(pos).BillID = 0;
				SwitchPrcList.at(pos).SessID = 0;
			}
			updateStatus(1);
			ok = 1;
		}
	}
	CATCH
		ok = 0;
		showMessage(mfError, 0, 0);
	ENDCATCH
}

int PrcPaneDialog::switchProcessor(PPID objType, PPID objID)
{
	int    ok = -1;
	if(CanSwitch) {
		uint   i, pos = 0;
		PPID   prc_id = 0;
		PPID   sess_id = 0;
		SwitchPrcEntry * p_entry;
		BillTbl::Rec bill_rec;
		TSessionTbl::Rec ses_rec;
		THROW(BillObj->Search(objID, &bill_rec) > 0);
		if(SwitchPrcList.lsearch(&objID, &pos, CMPF_LONG)) {
			THROW(setupProcessor(SwitchPrcList.at(pos).PrcID, 0));
			ok = 1;
		}
		else if(TSesObj.SearchByLinkBillID(objID, &ses_rec) > 0) {
			if(oneof2(ses_rec.Status, TSESST_PLANNED, TSESST_PENDING)) {
				THROW(TSesObj.SetSessionState(&ses_rec, TSESST_INPROCESS, 0));
				sess_id = ses_rec.ID;
				THROW(TSesObj.PutRec(&sess_id, &ses_rec, 1));
			}
			else if(ses_rec.Status == TSESST_INPROCESS) {
				if(SwitchPrcList.lsearch(&ses_rec.PrcID, &(pos = 0), CMPF_LONG, offsetof(SwitchPrcEntry, PrcID))) {
					p_entry = &SwitchPrcList.at(pos);
					THROW(setupProcessor(p_entry->PrcID, 0));
					p_entry->BillID = objID;
					p_entry->SessID = ses_rec.ID;
					ok = 1;
				}
				else {
					PPSetAddedMsgObjName(PPOBJ_PROCESSOR, SwitchPrcList.at(pos).PrcID);
					CALLEXCEPT_PP(PPERR_FOREIGNPRCBILL);
				}
			}
			else if(oneof2(ses_rec.Status, TSESST_CLOSED, TSESST_CANCELED)) {
				CALLEXCEPT_PP(PPERR_BILLPRCSESSCLOSED);
			}
		}
		else {
			for(i = 0; ok <= 0 && SwitchPrcList.enumItems(&i, (void **)&p_entry);) {
				if(p_entry->BillID == 0) {
					LDATETIME curdtm = getcurdatetime_();
					if(TSesObj.IsProcessorBusy(p_entry->PrcID, 0, TSESK_SESSION, curdtm, 3600, &sess_id) < 0) {
						int    r = TSesObj.IsProcessorInProcess(p_entry->PrcID, TSESK_SESSION, 0);
						ProcessorTbl::Rec prc_rec;
						THROW(r);
						if(r < 0) {
							THROW(TSesObj.GetPrc(p_entry->PrcID, &prc_rec, 1, 1) > 0);
							THROW(TSesObj.CreateOnlineByLinkBill(&(sess_id = 0), &prc_rec, &bill_rec));
							THROW(setupProcessor(p_entry->PrcID, 0));
							p_entry->BillID = bill_rec.ID;
							p_entry->SessID = sess_id;
							ok = 1;
						}
					}
				}
			}
			THROW_PP(ok > 0, PPERR_NOFREEPRC);
		}
	}
	CATCHZOK
	return ok;
}

int PrcPaneDialog::processBill(const char * pInp)
{
	//
	// * BILL XX BILL_ID * (*BILLXX21357*)
	// BILL PRCCODE X ARTICLE_ID X BILL_ID
	//
	int    ok = 1;
	int    r = -1;
	uint   pos = 4; // 4 == sstrlen("BILL")
	BillTbl::Rec bill_rec;
	SString prc_code, ar_id_code, bill_id_code;
	while(pInp[pos] != 0 && pInp[pos] != 'X')
		prc_code.CatChar(pInp[pos++]);
	if(pInp[pos] == 'X')
		pos++;
	while(pInp[pos] != 0 && pInp[pos] != 'X')
		ar_id_code.CatChar(pInp[pos++]);
	if(pInp[pos] == 'X')
		pos++;
	while(pInp[pos] != 0 && pInp[pos] != 'X')
		bill_id_code.CatChar(pInp[pos++]);
	if(CanSwitch) {
		THROW(switchProcessor(PPOBJ_BILL, bill_id_code.ToLong()));
	}
	else {
		PPID   sess_id = 0;
		THROW_PP(State == sEMPTY_NOSESS, PPERR_PRCPANMUSTBEFREE);
		if(prc_code.NotEmpty()) {
			ProcessorTbl::Rec prc_rec;
			THROW_PP_S(TSesObj.GetPrcByCode(prc_code, &prc_rec) > 0, PPERR_BILLFORANOTHERPRC, prc_code);
			THROW_PP_S(prc_rec.ID == H.PrcID, PPERR_BILLFORANOTHERPRC, prc_rec.Name);
		}
		THROW(BillObj->Search(bill_id_code.ToLong(), &bill_rec) > 0);
		THROW(TSesObj.CreateOnlineByLinkBill(&sess_id, &H.PrcRec, &bill_rec));
		updateStatus(1);
		ok = 1;
	}
	CATCH
		showMessage(mfError, 0, 0);
		ok = 0;
	ENDCATCH
	return ok;
}

int PrcPaneDialog::setupPrice(double price)
{
	int    ok = -1;
	E.Price = (price > 0) ? R5(price) : 0;
	if(E.Price > 0) {
		SString temp_buf;
		// @v9.1.4 PPGetWord(PPWORD_PRICE, 0, temp_buf);
		PPLoadString("price", temp_buf); // @v9.1.4
		temp_buf.CatDiv(':', 2).Cat(E.Price, MKSFMTD(0, 2, NMBF_NOZERO));
		setStaticText(CTL_PRCPAN_ST_PRICE, temp_buf);
		PPGetWord(PPWORD_SUM, 0, temp_buf);
		temp_buf.CatDiv(':', 2).Cat(E.Price * E.Qtty, MKSFMTD(0, 2, NMBF_NOZERO));
		setStaticText(CTL_PRCPAN_ST_SUM, temp_buf);
		ok = 1;
	}
	else {
		setStaticText(CTL_PRCPAN_ST_PRICE, 0);
		setStaticText(CTL_PRCPAN_ST_SUM, 0);
	}
	return ok;
}

void PrcPaneDialog::ProcessEnter()
{
	int    prev_state = State;
	SString temp_buf;
	setCtrlString(CTL_PRCPAN_INFO, (const char *)0);
	if(State == sIDLE) {
		if(CONFIRM(PPCFM_PRCIDLECANCEL))
			setIdleMode(0);
	}
	else if(getInput()) {
		if(State == sEXPIRY) {
			LDATE  dt = ZERODATE;
			strtodate(Input, DATF_DMY, &dt);
			if(dt == 0) {
				E.Expiry = dt;
				setStaticText(CTL_PRCPAN_ST_EXPIRY, temp_buf);
			}
			if(checkdate(&dt)) {
				E.Expiry = dt;
				PPGetWord(PPWORD_BESTBEFORE, 0, temp_buf);
				temp_buf.CatDiv(':', 2).Cat(dt, DATF_DMY | DATF_CENTURY);
				setStaticText(CTL_PRCPAN_ST_EXPIRY, temp_buf);
				State = (E.Qtty > 0) ? sGOODS_QTTY : sGOODS_NOQTTY;
			}
			else
				showMessage(mfError, PPERR_SLIB, 0);
		}
		else if(State == sCOST) {
			if(setupPrice(Input.ToReal()) > 0)
				State = (E.Qtty > 0) ? sGOODS_QTTY : sGOODS_NOQTTY;
		}
		else if(Input.C(0) == '*') {
			Input.ShiftLeft(1);
			acceptQuantity(0);
		}
		else if(Input.Last() == '*') {
			Input.TrimRight();
			acceptQuantity(0);
		}
		else if(Input.C(0) == '/') {
			Input.ShiftLeft(1);
			acceptQuantity(1);
		}
		else if(Input.Last() == '/') {
			Input.TrimRight();
			acceptQuantity(1);
		}
		else if(isdec((uchar)Input.C(0))) {
			selectGoods(2);
		}
		else {
			SString tb = Input;
			tb.ToUpper();
			if(tb.Cmp("STOP", 0) == 0 || tb.Cmp("СТОП", 0) == 0 || tb.Transf(CTRANSF_INNER_TO_OUTER).Cmp("СТОП", 0) == 0)
				stopSession();
			else if(tb.Cmp("PRCREST", 0) == 0 || tb.Cmp("ОСТ", 0) == 0 || tb.Transf(CTRANSF_INNER_TO_OUTER).Cmp("ОСТ", 0) == 0) {
				if(State == sEMPTY_SESS)
					State = sREST;
				else if(State == sGOODS_NOQTTY)
					State = sREST_SERIAL_NOQTTY;
				else if(State == sGOODS_QTTY)
					State = sREST_SERIAL_QTTY;
				else {
					;
				}
			}
			else if(tb.Cmp("PRCFONT", 0) == 0 || tb.Cmp("ШРИФТ", 0) == 0 || tb.Transf(CTRANSF_INNER_TO_OUTER).Cmp("ШРИФТ", 0) == 0) {
				if(H.PrcRec.PrinterID) {
					if(!BarcodeLabelPrinter::UpLoad(H.PrcRec.PrinterID, "FONTS", 1))
						showMessage(mfError, 0, 0);
				}
			}
			else if(tb.CmpPrefix("BILL", 0) == 0) {
				if(!processBill(tb))
					showMessage(mfError, 0, 0);
			}
			else if(tb.CmpPrefix("EXPY", 0) == 0) {
				if(oneof3(State, sGOODS_QTTY, sGOODS_NOQTTY, sEXPIRY)) {
					showMessage(mfInfo, PPINF_INPUTEXPIRY, 0);
					State = sEXPIRY;
				}
				else
					showMessage(mfError, PPERR_PRCPAN_UNABLEXPIRY, 0);
  			}
			else if(tb.CmpPrefix("COST", 0) == 0) {
				if(oneof3(State, sGOODS_QTTY, sGOODS_NOQTTY, sCOST)) {
					if(TSesObj.GetConfig().Flags & PPTSessConfig::fUsePricing) {
						showMessage(mfInfo, PPINF_INPUTCOST, 0);
						State = sCOST;
					}
					else
						showMessage(mfError, PPERR_PRCPAN_NOCOSTCFG, 0);
				}
				else
					showMessage(mfError, PPERR_PRCPAN_UNABLCOST, 0);
			}
		}
		clearInput(0);
	}
	else if(E.GoodsID && E.Qtty > 0) {
		long   oprno = 0;
		TSessLineTbl::Rec rec;
		TSesObj.InitLinePacket(&rec, H.SessID);
		E.TSessID = rec.TSessID;
		E.UserID  = rec.UserID;
		E.Dt      = rec.Dt;
		E.Tm      = rec.Tm;
		if(oneof3(State, sREST, sREST_SERIAL_QTTY, sREST_SERIAL_NOQTTY)) {
			E.Flags |= TSESLF_REST;
			E.Sign   = 0;
		}
		if(TSesObj.PutLine(H.SessID, &oprno, &E, 1)) {
			LastOprNo = E.OprNo;
			showMessage(mfInfo, PPINF_TSESSLINETURNED, 0);
			if(H.PrcRec.Flags & PRCF_PRINTNEWLINE_PANE && H.PrcRec.PrinterID) {
				int    num_copies = (E.PackQtty > 1) ? (int)E.PackQtty : 0;
				if(!TSesObj.PrintBarLabel(H.SessID, E.OprNo, num_copies, 1))
					showMessage(mfError, 0, 0);
			}
			clearPanel();
		}
		else
			showMessage(mfError, 0, 0);
	}
	if(State != prev_state) {
		selectCtrl(CTL_PRCPAN_INPUT);
		setupHint();
	}
}

IMPL_HANDLE_EVENT(PrcPaneDialog)
{
	int    prev_state = State;
	if(TVCOMMAND) {
		if(TVCMD == cmOK) {
			ProcessEnter();
			clearEvent(event);
		}
	}
	TDialog::handleEvent(event);
	if(TVBROADCAST) {
		if(TVCMD == cmIdle)
			updateStatus(0);
		else
			return;
	}
	else if(TVKEYDOWN) {
		switch(TVKEY) {
			case kbF2:
				selectGoods(0);
				break;
			case kbF3:
				if(H.SessID) {
					TSessLineFilt filt(H.SessID);
					ViewTSessLine(&filt);
				}
				break;
			case kbF5:
				setIdleMode(1);
				break;
			case kbF7:
				printLastLine();
				break;
			case kbF9:
				if(oneof4(State, sGOODS_QTTY, sGOODS_NOQTTY, sREST_SERIAL_NOQTTY, sREST_SERIAL_QTTY)) {
					double qtty = 0;
					if(E.GoodsID && PPGoodsCalculator(E.GoodsID, H.SessID, 0, 0, &qtty) > 0)
						if(setupQtty(qtty, 0))
							clearInput(0);
						else
							showMessage(mfError, 0, 0);
				}
				break;
			case kbF10:
				selectGoods(1);
				break;
			default:
				return;
		}
	}
	else
		return;
	if(State != prev_state)
		setupHint();
	clearEvent(event);
}

void PrcPaneDialog::showMessage(int msgKind, int msgCode, const char * pAddMsg)
{
	SString msg_buf;
	PPGetMessage(msgKind, NZOR(msgCode, PPErrCode), pAddMsg, 1, msg_buf);
	setCtrlString(CTL_PRCPAN_INFO, msg_buf);
}

void PrcPaneDialog::clearPanel()
{
	MEMSZERO(E);
	setupGoods(0);
	setCtrlData(CTL_PRCPAN_QTTYPACK, &E.PackQtty);
	setCtrlData(CTL_PRCPAN_QTTY,     &E.Qtty);
	setCtrlData(CTL_PRCPAN_SERIAL, 0);
}

void PrcPaneDialog::selectGoods(int mode)
{
	setCtrlString(CTL_PRCPAN_INFO, (const char *)0);
	if(H.SessID && oneof6(State, sEMPTY_SESS, sGOODS_NOQTTY, sGOODS_QTTY, sREST, sREST_SERIAL_NOQTTY, sREST_SERIAL_QTTY)) {
		if(mode == 0) {
			ExtGoodsSelDialog * dlg = new ExtGoodsSelDialog(0, NewGoodsGrpID);
			if(CheckDialogPtrErr(&dlg)) {
				TIDlgInitData tidi;
				PPIDArray goods_list;
				TGSArray tgs_list;
				if(TSesObj.GetGoodsStrucList(H.SessID, 1, &tgs_list) > 0 && tgs_list.GetGoodsList(&goods_list) > 0) {
					dlg->setSelectionByGoodsList(&goods_list);
					dlg->setDTS(&tidi);
				}
				else if(NewGoodsGrpID == 0) {
					//GetDefScaleData(&tidi);
					dlg->setDTS(&tidi);
				}
				if(ExecView(dlg) == cmOK)
					if(dlg->getDTS(&tidi) > 0) {
						setupGoods(tidi.GoodsID);
						if(tidi.Quantity > 0)
							setupQtty(tidi.Quantity, (tidi.Flags & TIDIF_PACKS) ? 1 : 0);
					}
				delete dlg;
			}
		}
		else if(mode == 1) {
			setupGoods(H.MainGoodsID);
		}
		else if(mode == 2) {
			int    r = -1, err_code = PPERR_BARCODEORSERNFOUND;
			SString code = Input;
			BarcodeTbl::Rec bc_rec;
			if(GObj.SearchByBarcode(code, &bc_rec, 0, 1) > 0) {
				setupGoods(bc_rec.GoodsID);
				if(bc_rec.Qtty > 1)
					setupQtty(bc_rec.Qtty, 0);
				r = 1;
			}
			else {
				PPObjTSession::SelectBySerialParam ssp(H.SessID, code);
				int    r2 = TSesObj.SelectBySerial(&ssp);
				if(r2 == 1 || (r2 == 2 && isRestState())) {
					setupGoods(ssp.GoodsID);
					if(ssp.Qtty > 1)
						setupQtty(ssp.Qtty, 0);
					STRNSCPY(E.Serial, ssp.Serial);
					E.LotID = ssp.LotID;
					E.Sign = -1;
					setCtrlData(CTL_PRCPAN_SERIAL, E.Serial);
					setupSign();
					r = 1;
				}
				else if(r2 == 2)
					err_code = PPERR_SERIALUSED;
				else if(r2 == -2) {
					//
					// Лот с серийным номером найден, но находится на другом складе либо
					// закончился. Следовательно, инициализируем ИД товара, количество. Серийный номер не используем.
					//
					setupGoods(ssp.GoodsID);
					if(ssp.Qtty > 1)
						setupQtty(ssp.Qtty, 0);
					memzero(E.Serial, sizeof(E.Serial));
					E.LotID = 0;
					E.Sign = -1;
					setCtrlData(CTL_PRCPAN_SERIAL, E.Serial);
					setupSign();
					r = 1;
				}
			}
			if(r > 0) {
				clearInput(0);
				if(State == sGOODS_QTTY && H.PrcRec.Flags & PRCF_ONECLICKTURN_PANE)
					ProcessEnter();
			}
			else {
				showMessage(mfError, err_code, code);
				clearPanel();
			}
		}
	}
}

int PrcPaneDialog::setupQtty(double qtty, int inPack)
{
	int    ok = 1;
	if(oneof4(State, sGOODS_NOQTTY, sGOODS_QTTY, sREST_SERIAL_NOQTTY, sREST_SERIAL_QTTY)) {
		THROW_PP(E.GoodsID, PPERR_PRCPAN_GOODSUNDEF);
		THROW_PP(!inPack || E.Pack > 0, PPERR_PRCPAN_PACKUNDEF);
		if(inPack) {
			E.PackQtty = qtty;
			E.Qtty = E.Pack * qtty;
		}
		else {
			E.PackQtty = (E.Pack > 0) ? R6(qtty / E.Pack) : 0;
			E.Qtty = qtty;
		}
		setCtrlData(CTL_PRCPAN_QTTYPACK, &E.PackQtty);
		setCtrlData(CTL_PRCPAN_QTTY,     &E.Qtty);
		if(oneof2(State, sGOODS_NOQTTY, sGOODS_QTTY))
			State = (E.Qtty > 0) ? sGOODS_QTTY : sGOODS_NOQTTY;
		else if(oneof2(State, sREST_SERIAL_NOQTTY, sREST_SERIAL_QTTY))
			State = (E.Qtty > 0) ? sREST_SERIAL_QTTY : sREST_SERIAL_NOQTTY;
	}
	CATCH
		showMessage(mfError, 0, 0);
		ok = 0;
	ENDCATCH
	return ok;
}

void PrcPaneDialog::setupSign()
{
	int    idx;
	if(E.Sign == UNDEF_SIGN)
		idx = 3;
	else if(E.Sign < 0)
		idx = 0;
	else if(E.Sign > 0)
		idx = 1;
	else
		idx = 2;
	SString sign_text;
	PPGetSubStr(PPTXT_TSESSLINESIGN, idx, sign_text);
	setCtrlString(CTL_PRCPAN_SIGN, sign_text);
}

void PrcPaneDialog::setupGoods(PPID goodsID)
{
	int    ok = 1;
	double qtty = 0;
	int    qtty_in_pack = 0;
	E.TSessID = H.SessID;
	E.GoodsID = 0;
	E.GoodsName[0] = 0;
	E.Serial[0] = 0;
	E.Pack = 0;
	E.PackQtty = 0;
	E.Qtty = 0;
	E.Expiry = ZERODATE;
	setStaticText(CTL_PRCPAN_ST_EXPIRY, 0);
	E.Sign   = UNDEF_SIGN;
	setupPrice(0);
	if(oneof3(State, sEMPTY_SESS, sGOODS_NOQTTY, sGOODS_QTTY) || isRestState()) {
		Goods2Tbl::Rec goods_rec;
		if(GObj.Fetch(goodsID, &goods_rec) > 0) {
			E.GoodsID = goodsID;
			STRNSCPY(E.GoodsName, goods_rec.Name);
			//
			int    sign = 0;
			PPID   op_id = 0;
			if(H.LinkBillID) {
				BillTbl::Rec bill_rec;
				if(BillObj->Search(H.LinkBillID, &bill_rec) > 0)
					op_id = bill_rec.OpID;
			}
			else
				op_id = H.PrcRec.WrOffOpID;
			if(op_id) {
				PPID   op_type_id = GetOpType(op_id);
				if(oneof2(op_type_id, PPOPT_GOODSRECEIPT, PPOPT_DRAFTRECEIPT))
					sign = +1;
				else if(oneof3(op_type_id, PPOPT_GOODSEXPEND, PPOPT_DRAFTEXPEND, PPOPT_GOODSORDER))
					sign = -1;
			}
			if(sign == 0) {
				if(TgsList.SearchGoods(E.GoodsID, &sign) > 0)
					E.Sign = sign;
			}
			else
				E.Sign = sign;
			if(E.Sign > 0)
				TSesObj.GenerateSerial(&E);
			//
			GoodsStockExt gse;
			if(GObj.GetStockExt(goodsID, &gse) > 0) {
				E.Pack = gse.Package;
				if(E.Pack > 1) {
					qtty_in_pack = 1;
					qtty = 1;
				}
				else
					qtty = 1;
			}
			State = isRestState() ? sREST_SERIAL_NOQTTY : sGOODS_NOQTTY;
		}
		else
			State = H.SessID ? sEMPTY_SESS : sEMPTY_NOSESS;
	}
	setCtrlString(CTL_PRCPAN_GOODSNAME, E.GoodsName);
	setCtrlData(CTL_PRCPAN_PACK, &E.Pack);
	setCtrlData(CTL_PRCPAN_SERIAL, E.Serial);
	setupQtty(qtty, qtty_in_pack);
	setupSign();
}

void PrcPaneDialog::updateStatus(int forceUpdate)
{
	if(H.PrcID) {
		LDATETIME curdtm = getcurdatetime_();
		const long dif = diffdatetimesec(curdtm, LastPrcStatusCheckTime);
		if(forceUpdate || dif > 3) {
			const PPID prev_sess_id = H.SessID;
			TSessionTbl::Rec ses_rec;
			H.SessID = 0;
			if(TSesObj.IsProcessorInProcess(H.PrcID, 0, &ses_rec) > 0) {
				H.SessID = ses_rec.ID;
				H.LinkBillID = ses_rec.LinkBillID;
			}
			if(forceUpdate || H.SessID != prev_sess_id) {
				SString temp_buf;
				H.SessText[0] = 0;
				H.MainGoodsID = 0;
				H.MainGoodsName[0] = 0;
				TgsList.Destroy();
				if(H.SessID) {
					//SString ses_buf;
					TSesObj.MakeName(&ses_rec, temp_buf);
					temp_buf.CopyTo(H.SessText, sizeof(H.SessText));
					if(ses_rec.TechID) {
						TechTbl::Rec tec_rec;
						Goods2Tbl::Rec goods_rec;
						if(TSesObj.GetTech(ses_rec.TechID, &tec_rec) > 0 && GObj.Fetch(tec_rec.GoodsID, &goods_rec) > 0) {
							H.MainGoodsID = goods_rec.ID;
							STRNSCPY(H.MainGoodsName, goods_rec.Name);
							//
							GoodsStockExt gse;
							H.MainGoodsPack = (GObj.GetStockExt(H.MainGoodsID, &gse) > 0) ? gse.Package : 0;
						}
					}
					TSesObj.GetGoodsStrucList(H.SessID, 1, &TgsList);
					State = sEMPTY_SESS;
				}
				else {
					PPLoadText(PPTXT_PRCISFREE, temp_buf);
					STRNSCPY(H.SessText, temp_buf);
					clearPanel();
					State = sEMPTY_NOSESS;
				}
				LastOprNo = 0;
				BillTbl::Rec bill_rec;
				SString main_goods_label_buf;
				if(H.LinkBillID && BillObj->Search(H.LinkBillID, &bill_rec) > 0) {
					//SString bill_code_buf;
					PPObjBill::MakeCodeString(&bill_rec, 1, temp_buf);
					PPGetSubStr(PPTXT_LAB_PRCPAN_MAINGOODS, 1, main_goods_label_buf);
					setLabelText(CTL_PRCPAN_MAINGOODS, main_goods_label_buf);
					setCtrlString(CTL_PRCPAN_MAINGOODS, temp_buf);
				}
				else {
					PPGetSubStr(PPTXT_LAB_PRCPAN_MAINGOODS, 0, main_goods_label_buf);
					setLabelText(CTL_PRCPAN_MAINGOODS, main_goods_label_buf);
					setCtrlString(CTL_PRCPAN_MAINGOODS, H.MainGoodsName);
				}
				setCtrlString(CTL_PRCPAN_CURSESS, H.SessText);
			}
			{
				double num_pack = 0;
				if(H.SessID) {
					setCtrlReal(CTL_PRCPAN_TOTALQTTY, ses_rec.ActQtty);
					if(H.MainGoodsPack)
						num_pack = R2(ses_rec.ActQtty / H.MainGoodsPack);
				}
				else
					setCtrlReal(CTL_PRCPAN_TOTALQTTY, 0);
				setCtrlReal(CTL_PRCPAN_TOTALPACK, num_pack);
			}
			//
			// Определяем наличие режима простоя //
			//
			if(TSesObj.IsProcessorInProcess(H.PrcID, TSESK_IDLE, &ses_rec) > 0) {
				IdleSessID = ses_rec.ID;
				State = sIDLE;
				{
					LDATETIME idle_start;
					idle_start.Set(ses_rec.StDt, ses_rec.StTm);
					LTIME  idle_cont_tm;
					idle_cont_tm.settotalsec(diffdatetimesec(curdtm, idle_start));
					SString temp_buf = IdleContText;
					temp_buf.Cat(idle_cont_tm);
					setCtrlString(CTL_PRCPAN_INFO, temp_buf);
				}
			}
			else if(State == sIDLE)
				State = H.SessID ? sEMPTY_SESS : sEMPTY_NOSESS;
			LastPrcStatusCheckTime = curdtm;
		}
	}
}

int SLAPI ExecPrcPane(PPID prcID)
{
	int    ok = -1;
	if(prcID || ListBoxSelDialog(PPOBJ_PROCESSOR, &prcID, 0) > 0) {
		PrcPaneDialog * dlg = new PrcPaneDialog;
		if(CheckDialogPtrErr(&dlg)) {
			dlg->setDTS(prcID);
			ExecView(dlg);
			ok = 1;
		}
		else
			ok = 0;
		delete dlg;
	}
	return ok;
}
