// PPNAMEDFILT.CPP
// Copyright (c) P.Andrianov 2011, 2014, 2016, 2018, 2019
// @codepage UTF-8
//
#include <pp.h>
#pragma hdrstop

static const long Current_PPNamedFilt_Ver = 2;

SLAPI PPNamedFilt::PPNamedFilt() : ID(0), Ver(Current_PPNamedFilt_Ver), ViewID(0), Flags(0) //@erik ver 0-->1 // @v10.5.3 ver 1-->2
{
	memzero(Reserve, sizeof(Reserve));
}

PPNamedFilt & FASTCALL PPNamedFilt::operator = (const PPNamedFilt & s)
{
	ID = s.ID;
	Ver = s.Ver;
	ViewID = s.ViewID;
	Flags = s.Flags;
	Name = s.Name;
	DbSymb = s.DbSymb;
	Symb = s.Symb;
	ViewSymb = s.ViewSymb;
	Param = s.Param;
	VD = s.VD;
	DestGuaList = s.DestGuaList; // @v10.5.3
	return *this;
}

int SLAPI PPNamedFilt::Write(SBuffer & rBuf, long p) // @erik const -> notConst
{
	SSerializeContext sctx;
	int    ok = 1;
	Ver = Current_PPNamedFilt_Ver; // @v10.5.3
	THROW_SL(rBuf.Write(ID));
	THROW_SL(rBuf.Write(Ver));
	THROW_SL(rBuf.Write(ViewID));
	THROW_SL(rBuf.Write(Flags));
	THROW_SL(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Write(Name));
	THROW_SL(rBuf.Write(DbSymb));
	THROW_SL(rBuf.Write(Symb));
	THROW_SL(rBuf.Write(ViewSymb));
	THROW_SL(rBuf.Write(Param));
	THROW_SL(VD.Serialize(+1, rBuf, &sctx));
	THROW(DestGuaList.Serialize(+1, rBuf, &sctx)); // @v10.5.3
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFilt::Read(SBuffer & rBuf, long p)
{
	int    ok = 1;
	THROW_SL(rBuf.Read(ID));
	THROW_SL(rBuf.Read(Ver));
	THROW_SL(rBuf.Read(ViewID));
	THROW_SL(rBuf.Read(Flags)); // @v8.4.2 (за счет Reserve)
	THROW_SL(rBuf.Read(Reserve, sizeof(Reserve)));
	THROW_SL(rBuf.Read(Name));
	THROW_SL(rBuf.Read(DbSymb));
	THROW_SL(rBuf.Read(Symb));
	THROW_SL(rBuf.Read(ViewSymb));
	THROW_SL(rBuf.Read(Param));
	if(Ver > 0) {
		SSerializeContext sctx;
		THROW(VD.Serialize(-1, rBuf, &sctx));
		// @v10.5.3 {
		if(Ver > 1) {
			THROW(DestGuaList.Serialize(-1, rBuf, &sctx));
		}
		// } @v10.5.3 
	}
	CATCHZOK
	return ok;
}

SLAPI PPNamedFilt::~PPNamedFilt()
{
	Param.Z();
}

SLAPI PPNamedFiltPool::PPNamedFiltPool(const char * pDbSymb, const int readOnly) : TSCollection <PPNamedFilt>(), DbSymb(pDbSymb), Flags(0)
{
	SETFLAG(Flags, fReadOnly, readOnly);
}

const SString & SLAPI PPNamedFiltPool::GetDbSymb() const
{
	return DbSymb;
}

int SLAPI PPNamedFiltPool::IsNamedFiltSuited(const PPNamedFilt * pNFilt) const
{
	if(DbSymb.Empty() || DbSymb.CmpNC(pNFilt->DbSymb) == 0)
		return 1;
	else
		return PPSetError(PPERR_NFSTRNGFORPOOL);
}

uint SLAPI PPNamedFiltPool::GetCount() const
{
	uint   c = 0;
	if(DbSymb.NotEmpty()) {
		for(uint i = 0; i < getCount(); i++)
			if(DbSymb.CmpNC(at(i)->DbSymb) == 0)
				c++;
	}
	else
		c = getCount();
	return c;
}

int SLAPI PPNamedFiltPool::CheckUniqueNamedFilt(const PPNamedFilt * pNFilt) const
{
	int    ok = 1;
	for(uint i = 0; i < getCount(); i++) {
		const PPNamedFilt * p_nfilt = at(i);
		if(p_nfilt->ID != pNFilt->ID) {
			THROW_PP_S(p_nfilt->Name.CmpNC(pNFilt->Name) != 0, PPERR_DUPNFNAME, p_nfilt->Name);
			if(pNFilt->Symb[0])
				THROW_PP_S(stricmp(p_nfilt->Symb, pNFilt->Symb) != 0, PPERR_DUPNFSYMB, p_nfilt->Symb);
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFiltPool::Enum(PPID * pID, PPNamedFilt * pNFilt, int ignoreDbSymb) const
{
	for(uint i = 0; i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && (p_nfilt->ID > *pID)) {
			ASSIGN_PTR(pNFilt, *p_nfilt);
			(*pID) = p_nfilt->ID;
			return 1;
		}
	}
	return 0;
}

const PPNamedFilt * SLAPI PPNamedFiltPool::GetByID(PPID namedFiltID, int ignoreDbSymb) const
{
	const PPNamedFilt * p_filt = 0;
	for(uint i = 0; !p_filt && i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && p_nfilt->ID == namedFiltID)
			p_filt = p_nfilt;
	}
	return p_filt;
}

const  PPNamedFilt * SLAPI PPNamedFiltPool::GetBySymb(const char * pSymb, int ignoreDbSymb) const
{
	const PPNamedFilt * p_filt = 0;
	for(uint i = 0; !p_filt && i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if((ignoreDbSymb || IsNamedFiltSuited(p_nfilt)) && p_nfilt->Symb.CmpNC(pSymb) == 0)
			p_filt = p_nfilt;
	}
	if(!p_filt)
		PPSetError(PPERR_NAMEDFILTNFOUND, pSymb);
	return p_filt;
}

int SLAPI PPNamedFiltPool::PutNamedFilt(PPID * pNamedFiltID, const PPNamedFilt * pNFilt)
{
	int    ok = -1;
	PPID   max_id = 0;
	THROW_PP((Flags & fReadOnly) == 0, PPERR_NFPOOLISREADONLY);
	THROW(!pNFilt || IsNamedFiltSuited(pNFilt));
	for(uint i = 0; i < getCount(); i++) {
		PPNamedFilt * p_nfilt = at(i);
		if(p_nfilt->ID == *pNamedFiltID) {
			THROW(!pNFilt || IsNamedFiltSuited(p_nfilt));
			if(pNFilt)
				*p_nfilt = *pNFilt;
			else
				atFree(i);
			ok = 1;
		}
		else if(p_nfilt->ID > max_id)
			max_id = p_nfilt->ID;
	}
	if (ok < 0 && pNFilt) {
		PPNamedFilt * p_nfilt = CreateNewItem();
		THROW_SL(p_nfilt);
		*p_nfilt = *pNFilt;
		p_nfilt->ID = max_id + 1;
		ASSIGN_PTR(pNamedFiltID, p_nfilt->ID);
	}
	CATCHZOK
	return ok;
}

SLAPI PPNamedFiltMngr::PPNamedFiltMngr() : LastLoading(ZERODATETIME)
{
	SString name;
	P_Rez = new TVRez(makeExecPathFileName("pp", "res", name), 1);
	PPGetFilePath(PPPATH_BIN, PPFILNAM_NFPOOL, FilePath);
}

SLAPI PPNamedFiltMngr::~PPNamedFiltMngr()
{
	delete P_Rez;
}

int SLAPI PPNamedFiltMngr::LoadResource(PPID viewID, SString & rSymb, SString & rText, long * pFlags) const
{
	int    ok = 1;
	long   flags;
	if(P_Rez) {
		THROW_PP(P_Rez->findResource((uint)viewID, PP_RCDECLVIEW), PPERR_RESFAULT);
		P_Rez->getString(rSymb, 2); /*0 - 866, 1 - w_char, 2 - 1251*/
		P_Rez->getString(rText, 2);
		flags = (long)P_Rez->getUINT();
		pFlags = &flags;
	}
	CATCHZOK
	return ok;
}

int SLAPI PPNamedFiltMngr::GetResourceLists(StrAssocArray * pSymbList, StrAssocArray * pTextList) const
{
	int    ok = 1;
	SString text;
	SString symb;
	long flags;
	pSymbList->Z();
	pTextList->Z();
	if(P_Rez) {
		ulong pos = 0;
		for(uint rsc_id = 0; P_Rez->enumResources(PP_RCDECLVIEW, &rsc_id, &pos) > 0;) {
			if(!oneof4(rsc_id, PPVIEW_GOODSGROUP, PPVIEW_REGISTER, PPVIEW_TAG, PPVIEW_BBOARD)) { // Исключаем фиктивные PPView
				THROW(LoadResource(rsc_id, symb, text, &flags));
				THROW_SL(pSymbList->Add(rsc_id, symb));
				PPExpandString(text, CTRANSF_UTF8_TO_INNER); // @v10.1.6
				THROW_SL(pTextList->Add(rsc_id, text));
			}
		}
		pTextList->SortByText();
	}
	CATCHZOK
	return ok;
}

#define NFSTRGSIGN 'SFPP'

struct NamedFiltStrgHeader { // @persistent @size=64
	long   Signature;        // const=NFSTRGSIGN
	long   Reserve1[2];
	uint32 Count;
	char   Reserve2[48];
};

int SLAPI PPNamedFiltMngr::LoadPool(const char * pDbSymb, PPNamedFiltPool * pPool, int readOnly)
{
	int    ok = 1;
	pPool->Flags &= ~PPNamedFiltPool::fReadOnly;
	pPool->freeAll();
	if(fileExists(FilePath)) {
		SFile f;
		NamedFiltStrgHeader hdr;
		THROW_SL(f.Open(FilePath, SFile::mRead | SFile::mBinary));
		THROW_SL(f.Read(&hdr, sizeof(hdr)));
		THROW_PP(hdr.Signature == NFSTRGSIGN, PPERR_NFSTRGCORRUPTED);
		for(uint i = 0; i < hdr.Count; i++) {
			PPID   id = 0;
			PPNamedFilt nf;
			SBuffer buf;
			THROW_SL(f.Read(buf));
			THROW(nf.Read(buf, 0));
			id = nf.ID;
			THROW(pPool->PutNamedFilt(&id, &nf));
		}
	}
	else
		ok = (PPSetErrorSLib(), -1);
	LastLoading = getcurdatetime_();
	pPool->DbSymb = pDbSymb;
	CATCHZOK
	SETFLAG(pPool->Flags, PPNamedFiltPool::fReadOnly, readOnly);
	return ok;
}

int SLAPI PPNamedFiltMngr::SavePool(const PPNamedFiltPool * pPool) const
{
	int    ok = 1;
	SFile  f;
	SBuffer buf;
	NamedFiltStrgHeader hdr;
	THROW_SL(f.Open(FilePath, SFile::mWrite | SFile::mBinary));
	MEMSZERO(hdr);
	hdr.Signature = NFSTRGSIGN;
	hdr.Count = pPool->getCount();
	THROW_SL(f.Write(&hdr, sizeof(hdr)));
	for(uint i = 0; i < hdr.Count; i++) {
		buf.Z();
		PPNamedFilt * p_nfilt = pPool->at(i);
		THROW(p_nfilt->Write(buf, 0));
		THROW_SL(f.Write(buf));
	}
	CATCHZOK
	return ok;
}
//
// Descr: Отвечает за диалог "Список фильтров"
//
class FiltPoolDialog : public PPListDialog {
public:
	FiltPoolDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pData) : PPListDialog(DLG_FILTPOOL, CTL_FILTPOOL_LIST), P_Mngr(pMngr), P_Data(pData)
	{
		CALLPTRMEMB(P_Mngr, GetResourceLists(&CmdSymbList, &CmdTextList));
		updateList(-1);
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPNamedFiltMngr * P_Mngr; // @notowned
	PPNamedFiltPool * P_Data; // @notowned
	StrAssocArray CmdSymbList;
	StrAssocArray CmdTextList;
};

//
// Descr: Обновляет таблицу фильтров в диалоге "Список фильтров"
//
int FiltPoolDialog::setupList()
{
	int    ok = 1;
	SString temp_buf;
	PPNamedFilt  nfilt;
	StringSet ss(SLBColumnDelim);
	for(PPID id = 0; ok && P_Data->Enum(&id, &nfilt, 0/*1 - ForAllDb*/);) {
		ss.clear();
		ss.add(temp_buf.Z().Cat(nfilt.ID));
		ss.add(nfilt.Name);
		ss.add(nfilt.Symb);
		uint    pos = 0;
		temp_buf.Z();
		if(CmdSymbList.SearchByText(nfilt.ViewSymb, 1, &pos)) {
			StrAssocArray::Item symb_item = CmdSymbList.at_WithoutParent(pos);
			uint  tpos = 0;
			if(CmdTextList.Search(symb_item.Id, &tpos))
				temp_buf = CmdTextList.at_WithoutParent(tpos).Txt;
		}
		ss.add(temp_buf);
		if(!addStringToList(id, ss.getBuf()))
			ok = 0;
	}
	return ok;
}
//
// Descr: Создает и отображает диалог "Список фильтров"
//
int SLAPI ViewFiltPool()
{
	int    ok = -1;
	PPNamedFiltMngr mngr;
	PPNamedFiltPool pool(0, 0);
	SString db_symb;
	FiltPoolDialog * dlg = 0;
	THROW_PP(CurDict->GetDbSymb(db_symb) > 0, PPERR_DBSYMBUNDEF);
	THROW(mngr.LoadPool(db_symb, &pool, 0));
	THROW(CheckDialogPtrErr(&(dlg = new FiltPoolDialog(&mngr, &pool))));
	while(ExecView(dlg) == cmOK) {
		if(mngr.SavePool(&pool)) {
			ok = 1;
			break;
		}
		else
			PPError();
	}
	CATCHZOKPPERR
	delete dlg;
	return ok;
}

//
// Descr: Класс, отвечающий за диалог "Создать фильтр"
//
class FiltItemDialog : public TDialog {
public:
	FiltItemDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pPool) : TDialog(DLG_FILTITEM), P_Mngr(pMngr), P_Pool(pPool)
	{
		P_Mngr->GetResourceLists(&CmdSymbList, &CmdTextList);
	}
	//
	// Descr: Заполняет интерфейс данными из pData
	//
	int    setDTS(const PPNamedFilt * pData);
	//
	// Descr: Заполняет pData данными из интерфейса
	//
	int    getDTS(PPNamedFilt * pData);
private:
	DECL_HANDLE_EVENT;
	int    ChangeBaseFilter();    // Обрабатывает изменение базового фильтра
	int    ViewMobColumnList(); //@erik 10.5.0
	PPNamedFilt Data;          // Собственно, редактируемый именованный фильтр
	StrAssocArray CmdSymbList; // Список ассоциаций для обьектов PPView {id, символ}
	StrAssocArray CmdTextList; // Список ассоциаций для обьектов PPView {id, описание} (упорядоченный по возрастанию)
	PPNamedFiltPool * P_Pool;  // @notowned
	PPNamedFiltMngr * P_Mngr;  // @notowned
};
//
// Descr: Отображает диалог редактирования именованного фильтра
//
int SLAPI EditFiltItem(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pNFiltPool, PPNamedFilt * pData)
{
	DIALOG_PROC_BODY_P2(FiltItemDialog, pMngr, pNFiltPool, pData);
}

//
// Descr: Обрабатывает события диалога "Создать фильтр"
//
IMPL_HANDLE_EVENT(FiltItemDialog)
{
	TDialog::handleEvent(event);
	uint pos;
	// Событие: выбор элемента в комбобоксе
	if(event.isCbSelected(CTLSEL_FILTITEM_CMD)) {
		PPID view_id = getCtrlLong(CTLSEL_FILTITEM_CMD);
		if(view_id && CmdTextList.Search(view_id, &pos)) {
			// Если пользователь еще не ввел название, возможно он захочет совпадающее с именем PPView
			SString name;
			getCtrlString(CTL_FILTITEM_NAME, name);
			if(!name.NotEmptyS()) {
				setCtrlString(CTL_FILTITEM_NAME, CmdTextList.Get(pos).Txt);
			}
			Data.Param.Z(); // Поменялся PPView, фильтр устарел
			enableCommand(cmCmdParam, 1);
		}
		else
			enableCommand(cmCmdParam, 0);
	}
	// Событие: нажатие кнопки "Фильтр.."
	else if(event.isCmd(cmCmdParam)) {
		ChangeBaseFilter();
	}
	else if(event.isCmd(cmOutFields)) {
		ViewMobColumnList();
	}
	// @v10.5.3 {
	else if(event.isCmd(cmGuaList)) {
		PPIDArray id_list;
		Data.DestGuaList.Get(id_list);
		ListToListData ltld(PPOBJ_GLOBALUSERACC, 0, &id_list);
		ltld.TitleStrID = 0; // PPTXT_XXX;
		if(ListToListDialog(&ltld) > 0)
			Data.DestGuaList.Set(&id_list);
	}
	// } @v10.5.3 
	clearEvent(event);
}

int FiltItemDialog::ChangeBaseFilter()
{
	int    ok = 1;
	size_t sav_offs = 0;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	PPID   view_id = getCtrlLong(CTLSEL_FILTITEM_CMD);
	if(view_id && CmdSymbList.Search(view_id)) {
		sav_offs = Data.Param.GetRdOffs();
		THROW(PPView::CreateInstance(view_id, &p_view) > 0);
		{
			if(Data.Param.GetAvailableSize()) {
				THROW(PPView::ReadFiltPtr(Data.Param, &p_filt));
			}
			SETIFZ(p_filt, p_view->CreateFilt(0));
			if((ok = p_view->EditBaseFilt(p_filt)) > 0) {
				Data.Param.Z();
				THROW(p_view->WriteFiltPtr(Data.Param, p_filt));
			}
			else
				Data.Param.SetRdOffs(sav_offs);
		}

	}
	CATCH
		Data.Param.SetRdOffs(sav_offs);
	ENDCATCH
	ZDELETE(p_filt);
	ZDELETE(p_view);
	return ok;
}

int FiltItemDialog::setDTS(const PPNamedFilt * pData)
{
	int    ok = 1;
	PPID   view_id = 0;                            // Идентификатор обьекта PPView
	Data = *pData;
	setCtrlString(CTL_FILTITEM_NAME, Data.Name); // Поле "Наименование"
	setCtrlString(CTL_FILTITEM_SYMB, Data.Symb); // Поле "Символ"
	setCtrlLong(CTL_FILTITEM_ID, Data.ID);       // Поле "ID"
	disableCtrl(CTL_FILTITEM_ID, 1);             // Идентификатор только отображаетс
	view_id = Data.ViewID; // Может быть и ноль, если это создание (а не редактирование) именованного фильтра
	//
	// Инициализировать комбобокс:
	//   отсортированный по описанию список {id PPView, описание PPView}
	//   активным элементом сделать элемент с идентификатором view_id
	//
	SetupStrAssocCombo(this, CTLSEL_FILTITEM_CMD, &CmdTextList, view_id, 0);
	AddClusterAssoc(CTL_FILTITEM_FLAGS, 0, PPNamedFilt::fDontWriteXmlDTD);
	SetClusterData(CTL_FILTITEM_FLAGS, Data.Flags);
	return ok;
}

int FiltItemDialog::getDTS(PPNamedFilt * pData)
{
	int    ok = 1;
	PPID   view_id = 0; // Идентификатор обьекта PPView
	uint   sel = 0;     // Идентификатор управляющего элемента, данные из которого анализировались в момент ошибки
	uint pos;
	getCtrlString(sel = CTL_FILTITEM_NAME, Data.Name);
	THROW_PP(Data.Name.NotEmptyS(), PPERR_NFILTNAMENEEDED);
	getCtrlString(sel = CTL_FILTITEM_SYMB, Data.Symb);
	THROW_PP(stricmp(Data.Symb, "") != 0, PPERR_SYMBNEEDED);
	THROW(P_Pool->CheckUniqueNamedFilt(&Data));
	getCtrlData(sel = CTLSEL_FILTITEM_CMD, &view_id);
	THROW_PP(CmdSymbList.Search(view_id, &pos), PPERR_INVNFCMD);
	GetClusterData(CTL_FILTITEM_FLAGS, &Data.Flags);
	Data.ViewSymb = CmdSymbList.Get(pos).Txt;
	Data.ViewID = view_id;
	ASSIGN_PTR(pData, Data);
	CATCH
		// Вывести сообщение о ошибке и активировать породивший его управляющий элемент
		ok = PPErrorByDialog(this, sel);
	ENDCATCH
	return ok;
}

//
// Descr: Обрабатывает нажатие кнопки "Добавить.."
//
int FiltPoolDialog::addItem(long * pPos, long * pID)
{
	int ok = -1;
	PPNamedFilt  nfilt;
	nfilt.DbSymb = P_Data->GetDbSymb();
	if(EditFiltItem(P_Mngr, P_Data, &nfilt) > 0) {
		PPID   id = 0;
		THROW(P_Data->PutNamedFilt(&id, &nfilt));
		ASSIGN_PTR(pID, id);
		ok = 1;
	}
	CATCHZOKPPERR
		return ok;
}

//
// Descr: Обрабатывает редактирование именованного фильтра
//
int FiltPoolDialog::editItem(long pos, long id)
{
	int    ok = -1;
	const  PPNamedFilt * p_nfilt = P_Data->GetByID(id, 1);
	if(p_nfilt) {
		PPNamedFilt nfilt = *p_nfilt;
		// Вызываем диалог редактировани
		if(EditFiltItem(P_Mngr, P_Data, &nfilt) > 0) {
			// OK - сохраняем данные
			ok = P_Data->PutNamedFilt(&id, &nfilt);
		}
		if(!ok)
			PPError();
	}
	else
		ok = PPError();
	return ok;
}

//
// Descr: Обрабатывает удаление именованного фильтра из списка
//
int FiltPoolDialog::delItem(long pos, long id)
{
	return P_Data->PutNamedFilt(&id, 0);
}

PPNamedFilt::ViewDefinition::Entry::Entry() : TotalFunc(0)
{
}

PPNamedFilt::ViewDefinition::Entry & PPNamedFilt::ViewDefinition::Entry::Z()
{
	Zone.Z();
	FieldName.Z();
	Text.Z();
	TotalFunc = 0;
	return *this;
}

PPNamedFilt::ViewDefinition::ViewDefinition()
{
}

//@erik v10.5.0
uint PPNamedFilt::ViewDefinition::GetCount() const
{
	return L.getCount();
}

int PPNamedFilt::ViewDefinition::SearchEntry(const char * pZone, const char *pFieldName, uint * pPos, InnerEntry * pInnerEntry) const
{
	int ok = 0;
	SString temp_buf;
	for(uint i = 0; !ok && i < L.getCount(); i++) {
		const InnerEntry & r_inner_entry = L.at(i);
		GetS(r_inner_entry.ZoneP, temp_buf);
		if(temp_buf.IsEqiAscii(pZone)) {
			GetS(r_inner_entry.FieldNameP, temp_buf);
			if(temp_buf.IsEqiAscii(pFieldName)) {
				ok = 1;
				ASSIGN_PTR(pPos, i);
				ASSIGN_PTR(pInnerEntry, r_inner_entry);
			}
		}
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::SetEntry(const Entry & rE)
{
	int ok = 0;
	uint pos = 0;
	InnerEntry new_entry;
	if(SearchEntry(rE.Zone, rE.FieldName, &pos, 0)) {
		AddS(rE.Zone, &new_entry.ZoneP);
		AddS(rE.FieldName, &new_entry.FieldNameP);
		AddS(rE.Text, &new_entry.TextP);
		new_entry.TotalFunc = rE.TotalFunc;
		L.at(pos) = new_entry;
		ok = 1;
	}
	else {
		AddS(rE.Zone, &new_entry.ZoneP);
		AddS(rE.FieldName, &new_entry.FieldNameP);
		AddS(rE.Text, &new_entry.TextP);
		new_entry.TotalFunc = rE.TotalFunc;
		L.insert(&new_entry);
		ok = 2;
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::GetEntry(const uint pos, Entry & rE) const
{
	rE.Z();
	int ok = 0;
	if(pos < L.getCount()) {
		const InnerEntry & r_entry = L.at(pos);
		GetS(r_entry.ZoneP, rE.Zone);
		GetS(r_entry.FieldNameP, rE.FieldName);
		GetS(r_entry.TextP, rE.Text);
		rE.TotalFunc = r_entry.TotalFunc;
		ok = 1;
	}
	return ok;
}

int PPNamedFilt::ViewDefinition::RemoveEntryByPos(uint pos)
{
	int    ok = 0;
	if(pos < GetCount() && L.atFree(pos))
		ok = 1;
	return ok;
}

int PPNamedFilt::ViewDefinition::XmlWriter(void * param)
{
	int ok = 1;
	return ok;
}

int PPNamedFilt::ViewDefinition::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pCtx)
{
	int    ok = 1;
	THROW_SL(pCtx->Serialize(dir, &L, rBuf));
	THROW_SL(SStrGroup::SerializeS(dir, rBuf, pCtx));
	CATCHZOK
	return ok;
}

class MobileClmnValListDialog : public PPListDialog {
public:
	MobileClmnValListDialog() : PPListDialog(DLG_MOBCLMNN, CTL_MOBCLMNN_LIST)
	{
		updateList(-1);
	}
	int setDTS(const PPNamedFilt::ViewDefinition * pData)
	{
		int    ok = 1;
		Data = *pData;	
		PPNamedFilt::ViewDefinition::Entry mobTypeClmn;
		StringSet ss(SLBColumnDelim);
		for(uint i = 0; ok && i < Data.GetCount(); i++) {
			PPID id = static_cast<PPID>(i + 1);
			Data.GetEntry(i, mobTypeClmn);
			ss.clear();
			((ss += mobTypeClmn.Zone) += mobTypeClmn.FieldName) += mobTypeClmn.Text;
			if(!addStringToList(id, ss.getBuf()))
				ok = 0;
		}
		updateList(1, 1);
		return ok;
	}
	int getDTS(PPNamedFilt::ViewDefinition * pData)
	{
		int    ok = 1;
		if(Data.GetCount() >= 0){
			ASSIGN_PTR(pData, Data);
		}
		else
			ok = 0;
		return ok;
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPNamedFilt::ViewDefinition Data;
};

//
// Descr: Создает и отображает диалог "Список "
//
int FiltItemDialog::ViewMobColumnList()
{
	DIALOG_PROC_BODY(MobileClmnValListDialog, &Data.VD);
}

//
// Descr: Класс, отвечающий за диалог "добавить элемент"
//
class MobileClmnValItemDialog : public TDialog {
public:
	MobileClmnValItemDialog(PPNamedFilt::ViewDefinition * pViewDef) : TDialog(DLG_MOBCLEDT), P_Data(pViewDef)
	{
	}
	int    setDTS(const PPNamedFilt::ViewDefinition::Entry * pData);
	int    getDTS(PPNamedFilt::ViewDefinition::Entry * pData);
private:
	// Собственно, редактируемый элемент
	PPNamedFilt::ViewDefinition * P_Data;  // @notowned
};

//
// Descr: Заполняет интерфейс данными из pData
//
int MobileClmnValItemDialog::setDTS(const PPNamedFilt::ViewDefinition::Entry * pEntry)
{
	int    ok = 1;
	PPNamedFilt::ViewDefinition::Entry entry = *pEntry;
	setCtrlString(CTL_MOBCLEDT_Z, entry.Zone); // Поле "Zone"
	setCtrlString(CTL_MOBCLEDT_FN, entry.FieldName); // Поле "FieldName"
	setCtrlString(CTL_MOBCLEDT_TXT, entry.Text); // Поле "Text"
	return ok;
}
//
// Descr: Заполняет pData данными из интерфейса
//
int MobileClmnValItemDialog::getDTS(PPNamedFilt::ViewDefinition::Entry * pEntry)
{
	int    ok = 1;
	uint   sel = 0;     // Идентификатор управляющего элемента, данные из которого анализировались в момент ошибки
	PPNamedFilt::ViewDefinition::Entry entry;
	getCtrlString(sel = CTL_MOBCLEDT_Z, entry.Zone);
	THROW(entry.Zone.NotEmptyS());
	getCtrlString(sel = CTL_MOBCLEDT_FN, entry.FieldName);
	THROW(entry.FieldName.NotEmptyS());
	getCtrlString(sel = CTL_MOBCLEDT_TXT, entry.Text);
	THROW(entry.Text.NotEmptyS());
	ASSIGN_PTR(pEntry, entry);
	CATCHZOK
	return ok;
}

//
// Descr: Отображает диалог редактирования 
//
int SLAPI EditMobTypeClmn(PPNamedFilt::ViewDefinition * pViewDef, PPNamedFilt::ViewDefinition::Entry * pEntry)
{
	DIALOG_PROC_BODY_P1(MobileClmnValItemDialog, pViewDef, pEntry);
}
//
// Descr: загрузка и перезагрузка списка всех отображаемых полей
//
int MobileClmnValListDialog::setupList()
{
	int    ok = 1;
	PPNamedFilt::ViewDefinition::Entry entry;
	StringSet ss(SLBColumnDelim);
	for(uint i = 0; ok && i < Data.GetCount(); i++) {
		PPID id = static_cast<PPID>(i + 1);
		Data.GetEntry(i, entry);
		ss.clear();
		((ss += entry.Zone) += entry.FieldName) += entry.Text;
		if(!addStringToList(id, ss.getBuf()))
			ok = 0;
	}
	return ok;
}

//
//Descr: Обрабатывает нажатие кнопки "Добавить.."
//
int MobileClmnValListDialog::addItem(long * pPos, long * pID)
{
	int    ok = 0;
	PPNamedFilt::ViewDefinition::Entry entry;
	if(EditMobTypeClmn(&Data, &entry) > 0) {
		Data.SetEntry(entry);
		ok = 1;
	}
	return ok;
}
//
// Descr: Обрабатывает редактирование
//
int MobileClmnValListDialog::editItem(long pos, long id)
{
	int    ok = -1;
	PPNamedFilt::ViewDefinition::Entry entry;
	if(Data.GetEntry(pos, entry)) {
		// Вызываем диалог редактировани
		if(EditMobTypeClmn(&Data, &entry) > 0) {
			// OK - сохраняем данные
			ok = Data.SetEntry(entry);
		}
	}
	else {
		ok = 0;
	}
	return ok;
}
//
// Descr: Обрабатывает удаление
//
int MobileClmnValListDialog::delItem(long pos, long id)
{
	return Data.RemoveEntryByPos(pos);
}
//@erik