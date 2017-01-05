// PPNAMEDFILT.CPP
// Copyright (c) P.Andrianov 2011, 2014, 2016
// @codepage windows-1251
//
#include <pp.h>
#pragma hdrstop

SLAPI PPNamedFilt::PPNamedFilt()
{
	ID = 0;
	Ver = 0;
	ViewID = 0;
	Flags = 0;
	memzero(Reserve, sizeof(Reserve));
}

PPNamedFilt & FASTCALL PPNamedFilt::operator = (const PPNamedFilt & s)
{
	ID       = s.ID;
	Ver      = s.Ver;
	ViewID   = s.ViewID;
	Flags    = s.Flags;
	Name     = s.Name;
	DbSymb   = s.DbSymb;
	Symb     = s.Symb;
	ViewSymb = s.ViewSymb;
	Param    = s.Param;
	return *this;
}

int SLAPI PPNamedFilt::Write(SBuffer & rBuf, long p) const
{
	int    ok = 1;
	THROW(rBuf.Write(ID));
	THROW(rBuf.Write(Ver));
	THROW(rBuf.Write(ViewID));
	THROW(rBuf.Write(Flags)); // @v8.4.2 (за счет Reserve)
	THROW(rBuf.Write(Reserve, sizeof(Reserve)));
	THROW(rBuf.Write(Name));
	THROW(rBuf.Write(DbSymb));
	THROW(rBuf.Write(Symb));
	THROW(rBuf.Write(ViewSymb));
	THROW(rBuf.Write(Param));
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

int SLAPI PPNamedFilt::Read(SBuffer & rBuf, long p)
{
	int    ok = 1;
	THROW(rBuf.Read(ID));
	THROW(rBuf.Read(Ver));
	THROW(rBuf.Read(ViewID));
	THROW(rBuf.Read(Flags)); // @v8.4.2 (за счет Reserve)
	THROW(rBuf.Read(Reserve, sizeof(Reserve)));
	THROW(rBuf.Read(Name));
	THROW(rBuf.Read(DbSymb));
	THROW(rBuf.Read(Symb));
	THROW(rBuf.Read(ViewSymb));
	THROW(rBuf.Read(Param));
	CATCH
		ok = PPSetErrorSLib();
	ENDCATCH
	return ok;
}

SLAPI PPNamedFilt::~PPNamedFilt()
{
	Param.Clear();
}

SLAPI PPNamedFiltPool::PPNamedFiltPool(const char * pDbSymb, const int readOnly) : TSCollection <PPNamedFilt> ()
{
	DbSymb = pDbSymb;
	Flags = 0;
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
	if(ok < 0 && pNFilt) {
		PPNamedFilt * p_nfilt = new PPNamedFilt;
		THROW_MEM(p_nfilt);
		*p_nfilt = *pNFilt;
		p_nfilt->ID = max_id+1;
		THROW_SL(insert(p_nfilt));
		ASSIGN_PTR(pNamedFiltID, p_nfilt->ID);
	}
	CATCHZOK
	return ok;
}

SLAPI PPNamedFiltMngr::PPNamedFiltMngr()
{
	SString name;
	long   PP  = 0x00005050L; // "PP"
	long   EXT = 0x00534552L; // "RES"
	P_Rez = new TVRez(makeExecPathFileName((const char*)&PP, (const char*)&EXT, name), 1);
	LastLoading.SetZero();
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
	pSymbList->Clear();
	pTextList->Clear();
	if(P_Rez) {
		ulong pos = 0;
		for(uint   rsc_id = 0; P_Rez->enumResources(PP_RCDECLVIEW, &rsc_id, &pos) > 0;) {
			// Исключаем фиктивные PPView
			if((rsc_id != PPVIEW_GOODSGROUP)&& // "Группы товаров"
				(rsc_id != PPVIEW_REGISTER)&&   // "Регистры"
				(rsc_id != PPVIEW_TAG)&&        // "Теги"
				(rsc_id != PPVIEW_BBOARD)) {    // "Запущенные серверные задачи"
					THROW(LoadResource(rsc_id, symb, text, &flags));
					THROW_SL(pSymbList->Add(rsc_id, symb));
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
		buf.Clear();
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
	FiltPoolDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pData) : PPListDialog(DLG_FILTPOOL, CTL_FILTPOOL_LIST)
	{
		P_Mngr = pMngr;
		P_Data = pData;
		updateList(-1);
	}
private:
	virtual int setupList();
	virtual int addItem(long * pPos, long * pID);
	virtual int editItem(long pos, long id);
	virtual int delItem(long pos, long id);

	PPNamedFiltMngr * P_Mngr; // @notowned
	PPNamedFiltPool * P_Data; // @notowned
};

//
// Descr: Обновляет таблицу фильтров в диалоге "Список фильтров"
//
int FiltPoolDialog::setupList()
{
	int    ok = 1;
	PPNamedFilt  nfilt;
	StringSet ss(SLBColumnDelim);
	for(PPID id = 0; ok && P_Data->Enum(&id, &nfilt, 0/*1 - ForAllDb*/);) {
		ss.clear(1);
		(ss += nfilt.Name) += nfilt.Symb;
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
	THROW(CheckDialogPtr(&(dlg = new FiltPoolDialog(&mngr, &pool)), 1));
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
	FiltItemDialog(PPNamedFiltMngr * pMngr, PPNamedFiltPool * pPool) : TDialog(DLG_FILTITEM)
	{
		P_Mngr = pMngr;
		P_Mngr->GetResourceLists(&CmdSymbList, &CmdTextList);
		P_Pool = pPool;
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
		PPID view_id  = getCtrlLong(CTLSEL_FILTITEM_CMD);
		if(view_id && CmdTextList.Search(view_id, &pos)) {
			// Если пользователь еще не ввел название, возможно он захочет совпадающее с именем PPView
			SString name;
			getCtrlString(CTL_FILTITEM_NAME, name);
			if(!name.NotEmptyS()) {
				setCtrlString(CTL_FILTITEM_NAME, CmdTextList.at(pos).Txt);
			}
			Data.Param.Clear(); // Поменялся PPView, фильтр устарел
			enableCommand(cmCmdParam, 1);
		}
		else
			enableCommand(cmCmdParam, 0);
	}
	// Событие: нажатие кнопки "Фильтр.."
	else if(event.isCmd(cmCmdParam)) {
		ChangeBaseFilter();
	}
	clearEvent(event);
}

int FiltItemDialog::ChangeBaseFilter()
{
	int    ok = 1;
	size_t sav_offs = 0;
	PPBaseFilt * p_filt = 0;
	PPView * p_view = 0;
	PPID   view_id = getCtrlLong(CTLSEL_FILTITEM_CMD);
	if(view_id && CmdSymbList.Search(view_id, 0)) {
		sav_offs = Data.Param.GetRdOffs();
		THROW(PPView::CreateInstance(view_id, &p_view) > 0);
		{
			if(Data.Param.GetAvailableSize()) {
				THROW(PPView::ReadFiltPtr(Data.Param, &p_filt));
			}
			SETIFZ(p_filt, p_view->CreateFilt(0));
			if((ok = p_view->EditBaseFilt(p_filt)) > 0) {
				Data.Param.Clear();
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
	Data.ViewSymb = CmdSymbList.at(pos).Txt;
	Data.ViewID = view_id;
	ASSIGN_PTR(pData, Data);
	CATCH
		// Вывести сообщение о ошибке и активировать породивший его управляющий элемент
		ok = PPErrorByDialog(this, sel, -1);
	ENDCATCH
	return ok;
}

//
// Descr: Обрабатывает нажатие кнопки "Добавить.."
//
int FiltPoolDialog::addItem(long * pPos, long * pID)
{
	int    ok = -1;
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
	else {
		ok = PPError();
	}
	return ok;
}

//
// Descr: Обрабатывает удаление именованного фильтра из списка
//
int FiltPoolDialog::delItem(long pos, long id)
{
	return P_Data->PutNamedFilt(&id, 0);
}
