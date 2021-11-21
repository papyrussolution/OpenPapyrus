// CUSTOMDISPLAY.H
//
#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val)     {if(!(expr)){SetError(val);goto __scatch;}}

int	ErrorCode = 0; 

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Коды ошибок
#define CUSTDISP_NOTENOUGHPARAM		300	// Не достаточно параметров для работы устройства
#define CUSTDISP_UNCNKOWNCOMMAND	301	// Передана неизвестная команда
#define CUSTDISP_NOTINITED			302	// Ошибка инициализации
#define CUSTDISP_NOTCONNECTED		303	// Соединение не установлено

#define CUSTDISP_PRINTTOPORT		400	// Ошибка записи в порт
#define CUSTDISP_NOTENOUGHMEM		401 // Недостаточный размер выходного массива
#define CUSTDISP_USBLIB				402 // Ошибка библиотеки для работы с usb

// Вертикальная табуляция
#define VERT_UPP	0
#define VERT_DOWN	1
#define VERT_CURPOS	2

// Горизонтальная табуляция
#define LEFT		0
#define RIGHT		1
#define CENTER		2
#define CURPOS		3

// Флаги
#define F_USB		1 // Дисплей работает через usb-порт без эмуляции RS-232

struct ErrMessage {
	uint Id;
	const char * P_Msg;
};

ErrMessage ErrMsg[] = {
	{CUSTDISP_NOTENOUGHPARAM,	"Не достаточно параметров для работы устройства"},
	{CUSTDISP_UNCNKOWNCOMMAND,	"Передана неизвестная команда"},
	{CUSTDISP_NOTINITED,		"Ошибка инициализации"},
	{CUSTDISP_NOTCONNECTED,		"Соединение не установлено"},
	{CUSTDISP_PRINTTOPORT,		"Ошибка записи в порт"},
	{CUSTDISP_NOTENOUGHMEM,		"Недостаточный размер выходного массива"},
	{CUSTDISP_USBLIB,			"Ошибка библиотеки для работы с usb"}
};

int	FASTCALL SetError(int errCode);
int FASTCALL SetError(int errCode) { ErrorCode = errCode; return 1; }
int Init();
int Release();

class CustomDisplayEquip {
public:
	CustomDisplayEquip();
	~CustomDisplayEquip();
	int RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize);
	int Connect();
	int Disconnect();
	int GetLastErrorText(char * pBuf, size_t bufSize);
	int GetConfig(char * pBuf, size_t bufSize);
	int PrintLine();
	int ClearDisplay();
	void ClearParams();

	int LastError;
	int Port;
	int VerTab;
	int Align;
	long Flags;
	SCommPort CommPort;
	SString Text;
	SString LastStr;
};

int CustomDisplayEquip::RunOneCommand(const char * pCmd, const char * pInputData, char * pOutputData, size_t outSize)
{
	int ok = 0, val = 0;
	SString s_param, param_val, out_data, r_error, params;
	if(LastError == CUSTDISP_NOTENOUGHMEM) {
		strnzcpy(pOutputData, LastStr, (int)outSize);
		LastError = 0;
	}
	else { // if(LastError != NOTENOUGHMEM)
		StringSet set_pairs(';', pInputData);
		if(sstreqi_ascii(pCmd, "CONNECT")) {
			for(uint i = 0; set_pairs.get(&i, params);) {
				params.Divide('=', s_param, param_val);
				if(s_param.CmpNC("PORT") == 0)
					Port = param_val.ToLong();
				else if(s_param.CmpNC("FLAGS") == 0)
					Flags = param_val.ToLong();
			}
			// new comment
			/*if(!(Flags & F_USB))
				THROWERR(Connect(), CUSTDISP_NOTCONNECTED);*/
			THROW(Connect()); //new
		}
		else if(sstreqi_ascii(pCmd, "DISCONNECT")) {
			if(!(Flags & F_USB))
				THROW(Disconnect());
		}
		else if(sstreqi_ascii(pCmd, "GETCONFIG"))
			THROW(GetConfig(pOutputData, outSize))
		else if(sstreqi_ascii(pCmd, "GETLASTERRORTEXT")) 
			ok = GetLastErrorText(pOutputData, outSize);
		else if(sstreqi_ascii(pCmd, "PUTLINE")) {
			for(uint i = 0; set_pairs.get(&i, params);) {
				params.Divide('=', s_param, param_val);
				if(s_param.CmpNC("TEXT") == 0)
					Text = param_val;
				else if(s_param.CmpNC("VERTAB") == 0)
					VerTab = param_val.ToLong();
				else if(s_param.CmpNC("ALIGN") == 0)
					Align = param_val.ToLong();
			}
			if(!(Flags & F_USB))
				THROW(PrintLine());
		}
		else if(sstreqi_ascii(pCmd, "CLEARDISPLAY")) {
			if(!(Flags & F_USB))
				THROW(ClearDisplay())
		}
		else { // Если дана неизвестная  команда, то сообщаем об этом
			THROWERR(0, CUSTDISP_UNCNKOWNCOMMAND);
		}
	}
	CATCH
		ok = 1;
		LastError = ErrorCode;
		_itoa(ErrorCode, pOutputData, 10);
	ENDCATCH;
	ErrorCode = 0;
	return ok;
}

CustomDisplayEquip::CustomDisplayEquip() : Flags(0), LastError(0), Port(0), VerTab(VERT_CURPOS), Align(LEFT), Text("")
{
}

CustomDisplayEquip::~CustomDisplayEquip()
{
}

// Пришлось для каждого дисплея сделать свой connect из-за того, что для Posiflex каждый раз
// приходится устанавливать кодовую таблицу
int CustomDisplayEquip::Connect()
{
	//return CommPort.InitPort(Port); // new comment
	return 1; // new
}

int CustomDisplayEquip::Disconnect()
{
	return 1;
}

void CustomDisplayEquip::ClearParams()
{
	VerTab = VERT_CURPOS;
	Align = LEFT;
	Text = "";
}

int CustomDisplayEquip::GetLastErrorText(char * pBuf, size_t bufSize) 
{
	int ok = 1;
	SString msg;
	for(uint i = 0; i < SIZEOFARRAY(ErrMsg); i++) {
		if(LastError == ErrMsg[i].Id) {
			(msg = 0).Cat(ErrMsg[i].P_Msg);
			break;
		}
	}
	if(msg.IsEmpty())
		msg.Cat("Error ").Cat(LastError);
	if(msg.BufSize() > bufSize)	
		ok = 2;
	else
		memcpy(pBuf, msg, msg.Len() + 1);
	return ok;
}

int CustomDisplayEquip::ClearDisplay() 
{
	// Исправила, потому что иногда глючный штриховский дисплей выдавал ошибку 0.
	//return CommPort.PutChr(0x0c);
	CommPort.PutChr(0x0c);
	return 1;
}

int CustomDisplayEquip::GetConfig(char * pBuf, size_t bufSize)
{
	return 1;
}

int CustomDisplayEquip::PrintLine()
{
	return 1;
}