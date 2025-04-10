// COMPLTEST.CPP
// Copiright (c) A.Sobolev 2011, 2012, 2025
// @codepage UTF-8
//
#include <pp.h>
#include <ppdlgs.h>
#pragma hdrstop
//
// Все нижеперечисленные файлы должны находиться в папках, указанных в конфигурации импорта/экспорта
//
// Если в базе уже есть такие товары или документы, то тест завершится с ошибкой
//
// Поля файла экспорта товаров universe-goods.txt:
// GrpName - Имя группы
// Name - Наименование товара
// Code - Штрихкод с форсированной контрольной цифрой
// ManufName - Изготовитель
// Country	- Страна изготовитель
// Brand - Бренд
// UnitName - Наименование единицы измерени
// PhUnitName - Наименование физической единицы измерени
// PhUPerU - Соотношение физ ед / торг ед
// UnitsPerPack - Емкость упаковки лота
// Vat - Ставка НДС
// ExpiryPeriod - Срок годности в днях
// PckgQtty - Емкость упаковки поставки (количество торговых единиц)
// PckgBrutto - Масса брутто упаковки поставки, г
// PckgLength - Длина упаковки поставки, мм
// PckgWidth - Ширина упаковки поставки, мм
// PckgHeight - Высота упаковки поставки, мм
//
// Поля файла импорта товаров universe-goods.txt (возможны поля как в файле для экспорта):
// Code - Штрихкод с форсированной контрольной цифрой
// Name - Наименование товара
// Brand - Бренд
// GrpName - Имя группы
// Country - Страна изготовитель
// ManufName - Изготовитель
// UnitName - Наименование единицы измерени
// PhUnitName - Наименование физической единицы измерени
// PhUPerU - Соотношение физ ед / торг ед
// UnitsPerPack - Емкость упаковки лота
// ExpiryPeriod - Срок годности в днях
// Price - Цена реализации
//
// Поля файла экспорта документов testxmlexp.xml
// ID - ИД документа
// Code - Номер документа
// Date - Дата
// OpID - ИД вида операции
// Amount - Сумма
// INN - ИНН контрагента
// CntragName - Имя контрагента
// RegistryCode - Код контрагента
// Obj2Name - Наименование дополнительного объекта по документу
// DueDate - Дата исполнения документа
// AgentName - Имя агента
// AgentINN - ИНН агента
// Memo - комментарий
// CntragID - ИД контрагента
//
// Поля файла экспорта строк документов testxmlexpln.exp
// BillID - ИД документа
// BillCode - Номер документа
// GoodsName - Наименование товара
// Quantity - Количество
// PckgQtty - Количество упаковок
// UnitPerPack - Емкость упаковки
// Cost - Цена поступлени
// Price - Цена реализации
// Discount - Сумма скидки по строке
//
// Поля файла импорта документов testxmlimp.xml
// Аналогично testxmlexp.xml
//
// Поля файла импорта строк документов testxmlimpln.xml
// Аналогично testxmlexpln.exp
//

#if SLTEST_RUNNING // {

class ComplTestState {
public:
	enum Objects {
		tGood = 1,
		tGrpGood = 2,
		tBill = 3,
		tAll = 4,
	};
	enum Factor {
		factGoodsCount = 1,
		factBillCount,
	};
	/*
	enum Factors {
		numOfBills = 23,
		numOfRecs = 1000,
	};
	*/
	struct St {
		St();
		int IsEqual(const St &);
		long GoodsCount;
		long GoodsGroupCount;
		long BillCount;
		double RestQtty;
		double RestCost;
	};
	int GetState(uint object);
	int StoreState();
	double GetFactorVal(uint st, uint factor);
	int CmpState(uint st1, uint st2, uint object);

	ComplTestState()
	{
		pBillFlt = 0;
	}
	~ComplTestState()
	{
		delete pBillFlt;
	}
private:
	PPObjGoodsGroup ObjGroup;
	GoodsGroupTotal GroupTotal;
	PPViewBill BillView;
	BillFilt * pBillFlt;
	BillTotal BillTotal;
	St Store;
	TSArray <St> StateList;
};

ComplTestState::St::St()
{
	GoodsCount = 0;
	GoodsGroupCount = 0;
	BillCount = 0;
	RestQtty = 0;
	RestCost = 0;
}

int ComplTestState::GetState(uint object)
{
	int ok = 1;
	switch(object) {
		case tGood:
			{
				GoodsRestTotal grest;
				PPViewGoodsRest v_grest;
				GoodsIterator giter((GoodsFilt *)0, 0, 0);
				Store.GoodsCount = giter.GetIterCounter().GetTotal();
				v_grest.GetTotal(&grest);
				int resttotal = grest.Count;
			}
			break;
		case tGrpGood:
			THROW(ObjGroup.CalcTotal(&GroupTotal));
			Store.GoodsGroupCount = GroupTotal.Count;
			break;
		case tBill:
			THROW(BillView.Init_(pBillFlt));
			THROW(BillView.CalcTotal(&BillTotal));
			Store.BillCount = BillTotal.Count;
			break;
		case tAll:
			{
				GoodsRestTotal grest;
				PPViewGoodsRest v_grest;
				GoodsIterator giter((GoodsFilt *)0, 0, 0);
				Store.GoodsCount = giter.GetIterCounter().GetTotal();
				v_grest.GetTotal(&grest);
				int resttotal = grest.Count;
			}
			THROW(ObjGroup.CalcTotal(&GroupTotal));
			Store.GoodsGroupCount = GroupTotal.Count;
			THROW(BillView.Init_(pBillFlt));
			THROW(BillView.CalcTotal(&BillTotal));
			Store.BillCount = BillTotal.Count;
			break;
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int ComplTestState::StoreState()
{
	int    ok = 1;
	int    cur_state_id = 0;
	THROW_SL(StateList.insert(&Store));
	cur_state_id = StateList.getCount()-1;
	CATCH
		ok = 0;
	ENDCATCH
	return cur_state_id;
}

double ComplTestState::GetFactorVal(uint st, uint factor)
{
	uint fact_val = 0;
	switch(factor) {
		case factGoodsCount:
			fact_val = StateList.at(st).GoodsCount - StateList.at(st).GoodsGroupCount;
			break;
		case factBillCount:
			fact_val = StateList.at(st).BillCount;
			break;
	}
	return fact_val;
}

int ComplTestState::CmpState(uint st1, uint st2, uint object)
{
	int ok = 1;
	switch(object) {
		case tGood:
			if(StateList.at(st1).GoodsCount != (StateList.at(st2).GoodsCount - (StateList.at(st2).GoodsGroupCount - StateList.at(st1).GoodsGroupCount)))
				ok = 0;
			break;
		case tGrpGood:
			if(StateList.at(st1).GoodsGroupCount != StateList.at(st2).GoodsGroupCount)
				ok = 0;
			break;
		case tBill:
			if(StateList.at(st1).BillCount != StateList.at(st2).BillCount)
				ok = 0;
			break;
		case tAll:
			if(StateList.at(st1).GoodsCount != (StateList.at(st2).GoodsCount - (StateList.at(st2).GoodsGroupCount - StateList.at(st1).GoodsGroupCount)))
				ok = 0;
			if(StateList.at(st1).GoodsGroupCount != StateList.at(st2).GoodsGroupCount)
				ok = 0;
			if(StateList.at(st1).BillCount != StateList.at(st2).BillCount)
				ok = 0;
			break;
	}
	return ok;
}

SLTEST_R(ComplTest)
{
	int ok = 1;
	uint arg_no = 0;
	uint state1 =0;
	uint state2 = 0;
	uint substate = 0;
	uint num_recs = 0;
	uint num_bills = 0;
    SString sure;
	SString arg;
	SysJournal * p_sj = DS.GetTLA().P_SysJ;
	SysJournalTbl::Rec sj_rec;
	LDATETIME dtm;
	LDATETIME cur_dtm;
	SString out_buf;
	ComplTestState obj_test_state;
	PPObjGoods obj_goods;
	ObjCollection obj_coll;
	PPObject * p_obj = 0;
	PPObjIDArray obj_id_list;
	PPObjBill obj_bill;
	PPBillImpExpParam bill_param;
	PPBillImpExpParam brow_param;

	if(EnumArg(&arg_no, arg)) {
		num_recs = arg.ToLong();
		if(EnumArg(&arg_no, arg)) {
			num_bills = arg.ToLong();
			if(EnumArg(&arg_no, arg)) {
				sure = arg;
			}
		}
	}

	//
	// Фиксируем состояние базы до импорта
	//
	THROW(SLTEST_CHECK_NZ(p_sj));
	THROW(SLTEST_CHECK_NZ(obj_test_state.GetState(obj_test_state.tAll)));
	state1 = obj_test_state.StoreState();

	dtm = getcurdatetime_();
	//
	// Импортируем товары из файла текстового формата и фиксируем состояние системы
	//
	THROW(SLTEST_CHECK_NZ(obj_goods.Import("Universe-HTT", 0, 1)));
	THROW(SLTEST_CHECK_NZ(obj_test_state.GetState(obj_test_state.tGood)));
	THROW(SLTEST_CHECK_NZ(obj_test_state.GetState(obj_test_state.tGrpGood)));
	state2 = obj_test_state.StoreState();
	//
	// Проверяем, все ли товары занесены в базу
	//
	if(sure.CmpNC("yes") == 0) {
		double gc = obj_test_state.GetFactorVal(state1, obj_test_state.factGoodsCount);
		THROW(SLTEST_CHECK_EQ((gc + num_recs), obj_test_state.GetFactorVal(state2, obj_test_state.factGoodsCount)));
	}
	state1 = state2;
	//
	// Импортируем документы из файла xml-формата и фиксируем состояние системы
	//
	THROW(SLTEST_CHECK_NZ(LoadSdRecord(PPREC_BILL, &bill_param.InnerRec)));
	bill_param.Name = "test-xml";
	THROW(SLTEST_CHECK_NZ(LoadSdRecord(PPREC_BROW, &brow_param.InnerRec)));
	brow_param.Name = "test-xml";
	{
		SString ini_file_name;
		THROW(PPGetFilePath(PPPATH_BIN, PPFILNAM_IMPEXP_INI, ini_file_name));
		{
			PPIniFile ini_file(ini_file_name, 0, 1, 1);
			bill_param.Direction = 1;
			brow_param.Direction = 1;
			SString sect = "test-xml";
			bill_param.ProcessName(1, sect);
			bill_param.ReadIni(&ini_file, sect, 0);
			sect = "test-xml";
			brow_param.ProcessName(1, sect);
			brow_param.ReadIni(&ini_file, sect, 0);
		}
	}
	THROW(SLTEST_CHECK_NZ(ImportBills(&bill_param, &brow_param, bill_param.ImpOpID, 0)));
	THROW(SLTEST_CHECK_NZ(obj_test_state.GetState(obj_test_state.tBill)));
	state2 = obj_test_state.StoreState();
	//
	// Проверяем, все ли документы занесены в базу
	//
	if(sure.CmpNC("yes") == 0) {
		double bc = obj_test_state.GetFactorVal(state1, obj_test_state.factBillCount);
		THROW(SLTEST_CHECK_EQ((bc + num_bills), obj_test_state.GetFactorVal(state2, obj_test_state.factBillCount)));
	}
	//
	// Удаляем товары, группы товаров (если они создавались при импорте) и документы из базы
	//
	cur_dtm = MAXDATETIME;
	uint count = 0;
	int r = p_sj->GetEvent(0, -1, &cur_dtm, 0, &sj_rec);
	THROW(SLTEST_CHECK_NZ(r));
	while(r > 0 && cmp(cur_dtm, dtm) > 0) {
		obj_coll.GetObjectPtr(sj_rec.ObjType);
		THROW(SLTEST_CHECK_NZ(obj_id_list.Add(sj_rec.ObjType, sj_rec.ObjID)));
		out_buf = 0;
		THROW(SLTEST_CHECK_NZ(p_sj->GetEvent(0, -1, &cur_dtm, 0, &sj_rec)));
		count++;
	}
	{
		PPTransaction tra(0, 1);
		THROW(SLTEST_CHECK_NZ(tra));
		for(uint i =0; i < count; i++) {
			p_obj = obj_coll.GetObjectPtr(obj_id_list.at(i).Obj);
			THROW(SLTEST_CHECK_NZ(p_obj->Remove(obj_id_list.at(i).Id, 0, 0)));
			out_buf = 0;
		}
		THROW(SLTEST_CHECK_NZ(tra.Commit()));
	}
	//
	// Сверяем с начальным состоянием базы
	//
	THROW(SLTEST_CHECK_NZ(obj_test_state.GetState(obj_test_state.tAll)));
	state2 = obj_test_state.StoreState();
	THROW(SLTEST_CHECK_NZ(obj_test_state.CmpState(0, state2, obj_test_state.tAll)));

	CATCH
		ok = 0;
	ENDCATCH
	return ok;

}
#endif // } SLTEST_RUNNING
