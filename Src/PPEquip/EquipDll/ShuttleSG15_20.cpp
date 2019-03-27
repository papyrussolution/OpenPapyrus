// PRICECHKR.cpp
//
#include <ppdefs.h>
#include <slib.h>
#include <dl600.h>
#include <ppifc_i.c>
#include <ppifc_h.h>
#include <snet.h>
#include <conio.h>

#define PP_QUOTE_KIND_ID	1
#define BUF_SIZE			1024
#define ESC					"\x1b"
#define ETX					"\x03"
#define ESCALIAS			"<ESC>"
#define	ETXALIAS			"<ETX>"
#define SERVERADDR			"127.0.0.1"
//#define SERVERPORT			4701

// Сообщения для вывода на дисплей
#define NOGOODINFO			"Данных о товаре нет"
#define NOPRICE				"Цена товара не найдена"

// Сообщения об ошибках
#define NOPORT				"В ini-файле указан неверный номер порта"
#define COMLIBNOTINIT		"Не инициализирована COM-библиотека"
#define	INITSESSERR			"Сессия Papyrus не была инициализирована"
#define INIFILEREADERR		"Ошибка чтения INI-файла"
#define CONNECTIONERR		"Соединение не установлено"
#define	LOGINERR			"Не удалось зайти в базу данных"
#define INVREQUEST			"Полученные данные не являются запросом от устройства"
#define GOODNOTFOUND		"Товар в базе данных не найден"
#define OPTIONNOTFOUND		"Опция не найдена"
#define SENDERR				"Ошибка отправки сообщения устройству"
#define ERROROCCURED		"Произошла ошибка. Приложение не может быть запущено"

// Cообщения о статусе приложения
#define APPLREADY			"Приложение готово к работе"
#define CONNECTED			"Coeдинение установлено"
#define LOGINSUCCESS		"Вход в базу данных успешно произведен"
#define ISREQUEST			"Запрос на товар с кодом"
#define DATABASESTR			"База данных"
#define LOGINSTR			"Логин"
#define RUB					"р."
#define WEIGHT				"Вес:"
#define	KG					"Кг."
#define HIDEWINDOW			"Перехожу в фоновый режим..."

class PriceChecker {
public:
	enum {
		sg15 = 0,
		sg20
	};
	struct StIniParam {
		StIniParam() : Port(0), HideWindow(0)
		{
		}
		int IsValid() 
		{
			return (Port != 0 && DataBaseName.NotEmpty() && UserName.NotEmpty() &&
                (ArrStorage.getCount() > 0) && (SG15Format.NotEmpty() || SG20Format.NotEmpty())) ? 1 : 0;
		}
		int HideWindow;
		int Port;
		SString DataBaseName;
		SString UserName;
		SString Password;
		StrAssocArray ArrStorage;
		SString SG15Format;
		SString SG20Format;
	};
	struct TagType {
		void Clear() 
		{
			Name = "";
			Options = "";
			Data = "";
		}
		SString Name;
		SString Options;
		SString Data;
	};
	struct StGoodInfo {
		void Clear() 
		{
			Name = "";
			Info = "";
			Price = 0.0;
			Qtty = 0.0;
		}
		SString Name;
		SString Info;
		double Price;
		double Qtty;
	};
	PriceChecker();
	~PriceChecker();
	int Connect(const char * pServerAddr);
	int CloseConnection();
	int InitSession();
	int Login();
	int Logout();
	// Descr: Получает запрос от утройства
	// ARG(rRequest	OUT): Содержит строку запроса
	int GetRequest(SString & rRequest);
	// Descr: Отправляет на устройтво ответ с описанием товара
	// ARG(answer	IN): Информация о товаре
	int PutAnswer(StGoodInfo & goodInfo);
	// Decsr: В базе данных ищет информацию о товаре по полученному запросу
	// returns:
	//		-1 - товар не найден
	//		 0 - ошибка
	//		 1 - информация о товаре получена
	int GetGoodInfo(const char * pRequest, StGoodInfo & rGoodInfo);
	int IsIniParamValid() { return IniParam.IsValid(); }
	int HideWindow() { return IniParam.HideWindow; }
	int LogMessage(const char * pMsg, ...);
	int ConcoleMessage(const char * pMsg, ...);
private:
	// Descr: Разбирает строку вида
	//		<request id=ip_устройства:порт type=sg20>штрихкод</request>
	// Returns:
	//		0 - строка имеет неверный формат
	//		1 - строка разобрана
	int GetFirstTag(const char * pInputStr, TagType & rTag);
	// Descr: Приводит строку с описанием товара к виду
	//		<data id=ip_адрес_утсройства:порт>описание_товара</data>
	// Также добавляет в строку управляющие команды типа "установить шрифт", "очистить экран" и т.д.
	// Команды и возможные форматы вывода на экран зависят от типа устройства (sg15 или sg20).
	// Настройки для каждого типа в SG.ini
	// ARG(pInfo		IN): Содержит описание товара.
	// ARG(rFormatedInfo	IN): Форматированная строка, содержащая описание товара и
	//							некоторые управляющие команды.
	// ARG(type			IN): Тип устройства sgXX
	int FormatAnswer(StGoodInfo * pInfo, SString & rFormatedInfo, int type);
	int GetOption(const char * pOptionType, SString & rOptionVal);
	int GetGoodsByCode(const SString & barcode, SPpyO_Goods * pRec, double * pQtty);
	int GetGoodsPrice(long goodsID, double * price);
	// Замещает все записи <ESC>, <ETX> и <#0x80> на соответствующие символы
	int ExpandAll(SString & rStr);
	// Используется в ExpandAll()
	int ExpandHex(SString & rStr);
	int FillStIniParam();
	// Форматирует вид цены. Добивает нулями до двух знаков после точки
	// ARG(rPrice	IN/OUT): Строка с ценой
	int FormatPrice(SString & rPrice);
	int InitLogFile();
	int CreateGoodInfoMessage(StGoodInfo & goodInfo, SString & rMsg);

	TagType Tag;
	SFile LogFile;
	SString FileLogName;
	InetAddr ServerAddr;
	TcpSocket Socket;
	IPapyrusSession * P_Session;
	StIniParam IniParam;
};

PriceChecker::PriceChecker()
{
	P_Session = NULL;
	InitLogFile();
	if(SUCCEEDED(CoInitialize(NULL))) {
		if(!InitSession()) {
			ConcoleMessage(ERROROCCURED, "");
			LogMessage(INITSESSERR, "");
		}
	}
	else {
		ConcoleMessage(ERROROCCURED, "");
		LogMessage(COMLIBNOTINIT, "");
	}

	FillStIniParam();
}

PriceChecker::~PriceChecker()
{
	CloseConnection();
	Logout();
	CALLPTRMEMB(P_Session, Release());
	CoUninitialize();
}

int PriceChecker::FillStIniParam()
{
	SPathStruc ps;
	SString temp_buf, storage_str;
	TCHAR ret_buf[BUF_SIZE];
	//char fname[MAX_PATH];
	//GetModuleFileName(NULL, fname, MAX_PATH);
	SString module_file_name;
	SSystem::SGetModuleFileName(0, module_file_name);
	ps.Split(module_file_name);
	ps.Nam.Z().Cat("PP");
	ps.Ext.Z().Cat("ini");
	ps.Merge(temp_buf);
	const TCHAR * p_file_name = SUcSwitch(temp_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("Port"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0)
		IniParam.Port = satoi(ret_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("HideWindow"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0)
		IniParam.HideWindow = satoi(ret_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("DbSymb"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0)
		IniParam.DataBaseName = SUcSwitch(ret_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("UserName"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0)
		IniParam.UserName = SUcSwitch(ret_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("Password"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0)
		IniParam.Password = SUcSwitch(ret_buf);
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("Storage"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0) {
		SString left, right;
		size_t i = 0;
		storage_str = SUcSwitch(ret_buf);
		while(storage_str.Divide(';', left, right) && left.NotEmpty()) {
			IniParam.ArrStorage.Add(i++, left);
			storage_str.Z().Cat(right);
		}
	}
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("sg15string"), NULL, ret_buf, SIZEOFARRAY(ret_buf), p_file_name) > 0) {
		IniParam.SG15Format = SUcSwitch(ret_buf);
		ExpandAll(IniParam.SG15Format);
	}
	if(GetPrivateProfileString(_T("ShuttleSG1520"), _T("sg20string"), NULL, ret_buf , SIZEOFARRAY(ret_buf), p_file_name) > 0) {
		IniParam.SG20Format = SUcSwitch(ret_buf);
		ExpandAll(IniParam.SG20Format);
	}
	if(!IniParam.IsValid()) {
		ConcoleMessage(INIFILEREADERR, "");
		LogMessage(INIFILEREADERR, "");
	}
	return 1;
}

int PriceChecker::InitLogFile()
{
	SPathStruc ps;
	SString temp_buf;
	//char fname[MAX_PATH];
	//GetModuleFileName(NULL, fname, MAX_PATH);
	SString module_file_name;
	SSystem::SGetModuleFileName(0, module_file_name);
	ps.Split(module_file_name);
	size_t start = 0, end = 0;
	while(ps.Dir.Search("\\", start, 1, &end)) {
		if(end == ps.Dir.Len() - 1) { // -1 потому что отсчет позиций начинается с нуля
			ps.Dir.Excise(start, end - start + 1);
			ps.Dir.Cat("LOG\\");
		}
		start = end + 1;
	}
	ps.Merge(temp_buf);
	ps.ReplaceExt(temp_buf, "log", 1);
	//ps.ReplaceExt(temp_buf.Z().Cat(fname), "log", 1);
	FileLogName.Z().Cat(temp_buf);
	//LogFile.Open(/*temp_buf*/FileLogName, SFile::mAppend | SFile::mWrite);
	return 1;
}

int PriceChecker::LogMessage(const char * pMsg, ...) 
{
	LogFile.Open(FileLogName, SFile::mAppend | SFile::mWrite);
	if(LogFile.IsValid()) {
		const char ** pp_msg = &pMsg;
		LDATETIME dt_tm;
		SString str;
		getcurdatetime(&dt_tm);
		str.Cat(dt_tm).Tab();
		while(*pp_msg != "") {
			str.Cat(*pp_msg).Space();
			pp_msg++;
		}
		str.CR();
		LogFile.WriteLine(str);
	}
	LogFile.Close();
	return 1;
}

int PriceChecker::ConcoleMessage(const char * pMsg, ...)
{
	const char ** pp_msg = &pMsg;
	SString str;
	while(*pp_msg != "") {
		str.Cat(*pp_msg).Space();
		pp_msg++;
	}
	str.ToOem();
	printf("%s\n", (const char *)str);
	return 1;
}

int PriceChecker::CreateGoodInfoMessage(StGoodInfo & goodInfo, SString & rMsg)
{
	rMsg.Z().Cat(goodInfo.Name).Space();
	if(goodInfo.Qtty > 0) {
		goodInfo.Info.Z().Cat(WEIGHT).Space().Cat(goodInfo.Qtty).Space().Cat(KG);
		rMsg.Cat(goodInfo.Info);
		// Цена за вес товара
		goodInfo.Price = goodInfo.Price * goodInfo.Qtty; // @vmiller
	}
	if(goodInfo.Price > 0)
		rMsg.Cat(goodInfo.Price).Space().Cat(RUB);
	return 1;
}

int PriceChecker::InitSession()
{
	return (CoCreateInstance(CLSID_PPSession, 0, CLSCTX_INPROC_SERVER, IID_IPapyrusSession, (void **)&P_Session) == S_OK);
}

int PriceChecker::Login()
{
	int  ok = 0;
	long ret = 0;
	BSTR db_name = 0, user_name = 0, passw = 0;
	SPathStruc ps;
	if(P_Session) {
		IniParam.DataBaseName.CopyToOleStr(&db_name);
		IniParam.UserName.CopyToOleStr(&user_name);
		IniParam.Password.CopyToOleStr(&passw);
		P_Session->Login(db_name, user_name, passw, &ret);
		SysFreeString(db_name);
		SysFreeString(user_name);
		SysFreeString(passw);
		if(!(ok = (ret > 0))) {
			ConcoleMessage(LOGINERR, ".", DATABASESTR, ":", (const char *)IniParam.DataBaseName, LOGINSTR, ":", (const char *)IniParam.UserName, "");
			LogMessage(LOGINERR, ".", DATABASESTR, ":", (const char *)IniParam.DataBaseName, LOGINSTR, ":", (const char *)IniParam.UserName, "");
		}
	}
	return ok;
}

int PriceChecker::Logout()
{
	long   ret = 0;
	CALLPTRMEMB(P_Session, Logout(&ret));
	return (ret > 0);
}

int PriceChecker::GetGoodsByCode(const SString & rBarcode, SPpyO_Goods * pRec, double * pQtty)
{
	int    ok = 0;
	IPapyrusObjGoods * p_obj = NULL;
	MEMSZERO(*pRec);
	if(CoCreateInstance(CLSID_PPObjGoods, 0, CLSCTX_INPROC_SERVER, IID_IPapyrusObjGoods, (void **)&p_obj) == S_OK) {
		long   ret = 0;
		double qtty = 0.0;
		BSTR   bc = 0;
		rBarcode.CopyToOleStr(&bc);
		p_obj->SearchQttyByBarcode(bc, pRec, &qtty, 1, &ret); // @v8.0.7 adoptSearching 0-->1
		if(ret > 0)
			ASSIGN_PTR(pQtty, qtty);
		p_obj->Release();
		SysFreeString(bc);
		ok = (ret > 0);
	}
	return ok;
}

int PriceChecker::GetGoodsPrice(long goodsID, double * price)
{
	int    ok = 0;
	size_t i = 0;
	IPapyrusRtlPriceExtractor * p_obj = NULL;
	IPapyrusObjLocation * p_loc_obj = NULL;
	SPpyI_RtlExtr retail_ext;
	SPpyO_Location rec_loc;
	BSTR loc_name = 0;
	SString str;

	if(CoCreateInstance(CLSID_PPRtlPriceExtractor, 0, CLSCTX_INPROC_SERVER, IID_IPapyrusRtlPriceExtractor, (void **)&p_obj) == S_OK) {
		if(CoCreateInstance(CLSID_PPObjLocation, 0, CLSCTX_INPROC_SERVER, IID_IPapyrusObjLocation, (void **)&p_loc_obj) == S_OK) {
			long ret = 0;
			for(i = 0; i < IniParam.ArrStorage.getCount(); i++) {
				MEMSZERO(rec_loc);
				str.Z().Cat(IniParam.ArrStorage.Get(i).Txt).CopyToOleStr(&loc_name);
				p_loc_obj->SearchByCode(loc_name, loctWarehouse, &rec_loc, &ret);
				if(ret > 0) {
					//LogMessage("GetGoodsPrice: LocId for ", (const char *)str, " found", "");	// @vmiller
					p_obj->Init(rec_loc.ID, qtfRetailed, (PpyRtlPriceFlags)0, &ret);
					if(ret > 0) {
						//LogMessage("GetGoodsPrice: Price structure have been got", ""); // @vmiller
						MEMSZERO(retail_ext);
						p_obj->GetPrice(goodsID, &retail_ext, &ret);
						if(ret > 0) {
							//LogMessage("GetGoodsPrice: Price have been got", ""); // @vmiller
							if(retail_ext.ExtPrice > 0) {
								ASSIGN_PTR(price, retail_ext.ExtPrice);
							}
							else {
								ASSIGN_PTR(price, retail_ext.Price);
							}
							ok = 1;
							break;
						}
					}
					else
						LogMessage("IPapyrusRtlPriceExtractor not inited", "");
				}
				SysFreeString(loc_name);
			}
			if(i == IniParam.ArrStorage.getCount() + 1)
				LogMessage("Price not found", "");
			SysFreeString(loc_name);
			p_obj->Release();
			p_loc_obj->Release();
		}
		else LogMessage("IPapyrusObjLocation not created", "");
	}
	else
		LogMessage("IPapyrusRtlPriceExtractor not created", "");
	return ok;
}

int PriceChecker::GetFirstTag(const char * pInputStr, TagType & rTag)
{
	rTag.Clear();
	bool NoTag = false, endtag_found = false;
	SString opentag, closetag;
	SString input_str;
	input_str.Z().Cat(pInputStr);
	size_t pos = 0, start = 0, end = 0, datastart = 0, dataend = 0;

	// Ищем стартовый тег
	if(input_str.Search("<", 0, 1, &start) && (start != input_str.Len())) {
		// Если это конечный тег (что является ошибкой), то возвращаем пустой rTag
		if(input_str.C(start + 1) == '/') {
			return 0;
		};
		// if it really is an end-tag, part of it remains in the inputbuffer, but this is discarded on the next pass...
		if(input_str.Search(">", 0, 1, &end) && (end != input_str.Len())) {
			// Стартовый тег найден
			input_str.Sub(start + 1, end - start - 1, opentag);
			if(opentag.Search(" ", start, 1, &pos) && (pos != input_str.Len())) {
				opentag.Sub(0, pos, rTag.Name);
				opentag.Sub(pos + 1, opentag.Len(), rTag.Options);
			}
			else
				rTag.Name = opentag;

			datastart = end + 1;
			dataend = datastart;
			NoTag = false;
			// Ищем завершающий тег
			while(!endtag_found && !NoTag) {
				if(input_str.Search("</", dataend, 1, &start) && (start != input_str.Len())) {
					if(input_str.Search(">", start, 1, &end) && (end != input_str.Len())) {
						// Завершающий тег найден
						input_str.Sub(start + 1, end - start - 1, closetag);
						if(closetag.NotEmpty() && closetag.Search(rTag.Name, 0, 1, &pos) && (pos != input_str.Len())) {
							// Находим данные между тегами
							input_str.Sub(datastart, start - datastart - 1, rTag.Data);
							//if(!isdigit(rTag.Data.C(0)))
							// А то может быть больше одной буквы в начале штрихкода
							while(!isdigit(rTag.Data.C(0)) && rTag.Data.NotEmpty())
								rTag.Data.ShiftLeft();
							input_str.Excise(0, end + 1);
							endtag_found = true;
						}
						else {
							// Пропускаем этот тег и ищем дальше
							endtag_found = false;
							dataend = end;
						}
					}
					else {
						// Звершающего тега нет
						NoTag = true;
						rTag.Clear();
					}
				}
				else {
					// Звершающего тега нет
					NoTag = true;
					rTag.Clear();
				}
			}
		}
	}
	if(NoTag)
		return 0;
	return 1;
}

int PriceChecker::GetOption(const char * pOptionType, SString & rOptionVal) 
{
	int    ok = 1;
	size_t start = 0, end = 0;
	SString str;
	str.Cat(pOptionType).CatChar('=');
	if(Tag.Options.Search(str, 0, 1, &start) && (start != Tag.Options.Len())) {
		// Смотрим разделитель
		if(!Tag.Options.Search(" ", start, 1, &end))
			end = Tag.Options.Len();
		Tag.Options.Sub(start + str.Len(), end - start - str.Len(), rOptionVal);
	}
	else {
		LogMessage(OPTIONNOTFOUND, pOptionType, "");
		rOptionVal.Z();
		ok = 0;
	}
	return ok;
}

int PriceChecker::ExpandHex(SString & rStr)
{
	size_t start = 0, end = 0, pos = 0, val = 0;
	SString valstr;
	// Найдем начало 16-чного числа
	while(rStr.Search("<#", start, 1, &pos)) {
		start = pos;
		if(start != rStr.Len()) {
			// Найдем конец 16-чного числа
			if(!rStr.Search(">", start, 1, &pos))
				pos = rStr.Len();
			end = pos;
			if(end != rStr.Len()) {
				// Получим подстроку с 16-чным числом
				rStr.Sub(start + 2, end - start - 2, valstr);
				val = valstr.ToLong();
				if(val < 256) {
					// Помещаем значение в строку
					valstr.Z().CatChar(val);
					rStr.Excise(start, end - start + 1);
					rStr.Insert(start, valstr);
				}
			}
		}
	}
	return 1;
}

int PriceChecker::ExpandAll(SString & rStr)
{
	size_t start = 0, end = 0;
	rStr.ReplaceStr(ESCALIAS, ESC, 0);
	rStr.ReplaceStr(ETXALIAS, ETX, 0);
	ExpandHex(rStr);
	return 1;
}

int PriceChecker::FormatPrice(SString & rPrice)
{
	SString l_str, r_str;
	rPrice.Divide('.', l_str, r_str);
	while(r_str.Len() < 2)
		r_str.CatChar('0');
	rPrice.Z().Cat(l_str).Dot().Cat(r_str);
	return 1;
}

int PriceChecker::FormatAnswer(StGoodInfo * pInfo, SString & rFormatedInfo, int type)
{
	SString format_str, price_str, id;
	format_str.Z();
	if(type == sg15)
		format_str.Cat(IniParam.SG15Format);
	else if(type == sg20) // @v10.3.0 @fix =-->==
		format_str.Cat(IniParam.SG20Format);
	else
		format_str.Cat(IniParam.SG15Format); // По умолчанию возьмем формат SG15
	if(pInfo->Name.Empty()) {
		format_str.ReplaceStr("{name}", NOGOODINFO, 1);
		format_str.ReplaceStr("{info}", "", 1);
		format_str.ReplaceStr("{price}", "", 1);
	}
	else if(pInfo->Price == 0) {
		format_str.ReplaceStr("{name}", pInfo->Name, 1);
		format_str.ReplaceStr("{info}", NOPRICE, 1);
		format_str.ReplaceStr("{price}", "", 1);
	}
	else {
		format_str.ReplaceStr("{name}", pInfo->Name, 1);
		format_str.ReplaceStr("{info}", pInfo->Info, 1);
		price_str.Z().Cat(pInfo->Price);
		FormatPrice(price_str);
		format_str.ReplaceStr("{price}", price_str, 1);
	}

	GetOption("id", id);
	rFormatedInfo.Z().Cat("<data").Space();
	if(id.NotEmpty())
		rFormatedInfo.Cat("id=").Cat(id);
	rFormatedInfo.CatChar('>').Cat(format_str).Cat("</data>");
	rFormatedInfo.ReplaceCR();

	return 1;
}

int PriceChecker::GetGoodInfo(const char * pRequest, StGoodInfo & rGoodInfo)
{
	int ok = 0;
	double price = 0.0, qtty = 0.0;
	SPpyO_Goods goods_rec;
	SString msg;

	rGoodInfo.Clear();
	GetFirstTag(pRequest, Tag);
	if(Tag.Name.CmpNC("request") == 0) {
		LogMessage(ISREQUEST, (const char *)Tag.Data, "");
		//Tag.Data.Z().Cat("210000000008"); // @vmiller
		if(GetGoodsByCode(Tag.Data, &goods_rec, &qtty) > 0) {
			rGoodInfo.Name.CopyFromOleStr(goods_rec.Name);
			// @vmiller {
			// В алкоголе почему-то выводится вторая строка да еще крокозябами. Избавимся от нее.
			size_t pos = 0;
			char buf[2];
			buf[0] = 0xA;
			buf[1] = 0;
			rGoodInfo.Name.Search(buf, 0, 1, &pos);
			if(pos)
				rGoodInfo.Name.Trim(pos);
			// } @vmiller
			rGoodInfo.Name.ToChar();
			rGoodInfo.Info.CopyFromOleStr(goods_rec.KindText);
			rGoodInfo.Qtty = qtty;
			ok = 1;
			if(GetGoodsPrice(goods_rec.ID, &price) > 0) {
				rGoodInfo.Price = price;
				/*SString price_str;
				price_str.Z().Cat(price);
				FormatPrice(price_str);
				ConcoleMessage((const char *)rGoodInfo.Name, (const char *)price_str, RUB, "");
				LogMessage((const char *)rGoodInfo.Name, (const char *)price_str, RUB, "");*/
			}
			/*else {
				ConcoleMessage((const char *)rGoodInfo.Name, "");
				LogMessage((const char *)rGoodInfo.Name, "");
			}*/
			CreateGoodInfoMessage(rGoodInfo, msg);
			ConcoleMessage(msg.cptr(), "");
			LogMessage(msg.cptr(), "");
		}
		else {
			ConcoleMessage(GOODNOTFOUND, "");
			LogMessage(GOODNOTFOUND, Tag.Data.cptr(), "");
			ok = -1;
		}
	}
	else {
		LogMessage(INVREQUEST, "");
		ok = 0;
	}
	return ok;
}

int PriceChecker::Connect(const char * pAddr)
{
	int    ok = 1;
	if(IniParam.Port <= 0) {
		ConcoleMessage(ERROROCCURED, "");
		LogMessage(NOPORT, "");
		ok = 0;
	}
	if(ok) {
		ServerAddr.Set(pAddr, IniParam.Port);
		if(!Socket.Connect(ServerAddr)) {
			SString port_str;
			port_str.Cat(IniParam.Port);
			ConcoleMessage(CONNECTIONERR, pAddr, port_str.cptr(), "");
			LogMessage(CONNECTIONERR, pAddr, port_str.cptr(), "");
			ok = 0;
		}
	}
	return ok;
}

int PriceChecker::CloseConnection() { return BIN(Socket.Disconnect()); }

int PriceChecker::GetRequest(SString & rRequest)
{
	int    ok = -1;
	SBuffer buf(BUF_SIZE);
	size_t recv_bytes = 0;
	rRequest.Z();
	// @v10.3.8 memzero(/*(void *)*/buf.GetBuf(), BUF_SIZE);
	buf.Z();
	while(Socket.RecvBuf(buf, BUF_SIZE, &recv_bytes) && recv_bytes != 0) {
		rRequest.CatN(static_cast<const char *>(buf.GetBuf()), buf.GetAvailableSize()); // @v10.3.8 Cat()-->CatN(.., buf.GetAvailableSize())
		// @v10.3.8 memzero(/*(void *)*/buf.GetBuf(), BUF_SIZE);
		buf.Z();
		recv_bytes = 0;
		ok = 1;
	}
	return ok;
}

int PriceChecker::PutAnswer(StGoodInfo & goodInfo)
{
	int    ok = 1;
	SString dev_type, formated_info;
	GetOption("type", dev_type);
	if(dev_type.CmpNC("sg15") == 0)
		FormatAnswer(&goodInfo, formated_info, sg15);
	else if(dev_type.CmpNC("sg20"))
		FormatAnswer(&goodInfo, formated_info, sg20);
	else
		FormatAnswer(&goodInfo, formated_info, sg15); // По умолчанию возьмем sg15
	if(!Socket.Send(formated_info, formated_info.Len() + 1, NULL)) {
		LogMessage(SENDERR, "");
		ok = 0;
	}
	return ok;
}

int main()
{
	int    ok = 1;
	char c = 0;
	PriceChecker price_chkr;
	SString cmd = "";
	PriceChecker::StGoodInfo info;

	THROW(price_chkr.IsIniParamValid());
	THROW(price_chkr.Connect(SERVERADDR));
	price_chkr.LogMessage(CONNECTED, "");
	THROW(price_chkr.Login());
	price_chkr.ConcoleMessage(APPLREADY, "");
	price_chkr.LogMessage(LOGINSUCCESS, "");
	if(price_chkr.HideWindow() > 0) {
		SDelay(500);
		price_chkr.ConcoleMessage(HIDEWINDOW, "");
		SDelay(2000);
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
	while(cmd.CmpNC("quit") != 0) {
		cmd = 0;
		if(price_chkr.GetRequest(cmd.Z()) > 0) {
		//cmd.Cat("<request id=10.10.10.11:1030 type=sg15>2100000000141</request>");
			if(price_chkr.GetGoodInfo(cmd, info) != 0)
				price_chkr.PutAnswer(info);
		}
		while(_kbhit()) {
			c = _getche();
			if(c=='\r') {
				c = '\n';
				_putch(c);
			}
			cmd.CatChar(c);
		}
	}
	CATCH
		ok = 0;
		SDelay(2000);
	ENDCATCH;
	return ok;

}
