// EMMEARINE.CPP
// Работа со считывателями карт по EM Marine по протоколу Wiegand26
//
#include <ppdrvapi.h>

extern PPDrvSession DRVS;

#define EXPORT	extern "C" __declspec (dllexport)
#define THROWERR(expr,val)     { if(!(expr)) { DRVS.SetErrCode(val); goto __scatch; } }

// Коды ошибок
#define RDRERR_OPENPORTFAILED  400 // Ошибка открытия порта
#define RDRERR_WRITEPORTFAILED 401 // Ошибка записи в порт
#define RDRERR_READPORTFAILED  402 // Ошибка чтения из порта
#define RDRERR_NOREPLY         403 // Устройство не отвечает
#define RDRERR_INVREPLY        404 // Неверный ответ устройства

class PPDrvReader : public PPBaseDriver {
public:
	enum {
		stError     = 0x0001,
		stConnected = 0x0002
	};
	PPDrvReader()
	{
		SString file_name;
		getExecPath(file_name);
		DRVS.SetLogFileName(file_name.SetLastSlash().Cat("Reader.log"));
		State = 0;
		ReEmmId = 0;
		Scan.RegisterRe("^[0-9]+,[0-9][0-9][0-9][0-9][0-9]", &ReEmmId);
	}
	~PPDrvReader()
	{
		Disconnect();
	};
	virtual int ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput);
	int    Connect(int port);
	int    Listen(SString & rOutput);
private:
	HANDLE Handle;
	SCommPort CommPort;
	long   State;
	long   ReEmmId;
	SStrScan Scan;

	int    Disconnect();
	int    CardCodeToString(const uint8 * pCardCode, SString & rBuf) const;
	uint8  CalcCheckCode(const uint8 * pBuf, size_t dataLen) const;
};

static PPDrvSession::TextTableEntry _ErrMsgTab[] = {
	{ RDRERR_OPENPORTFAILED,  "Ошибка открытия порта" },
	{ RDRERR_WRITEPORTFAILED, "Ошибка записи в порт" },
	{ RDRERR_READPORTFAILED,  "Ошибка чтения из порта" },
	{ RDRERR_NOREPLY,         "Устройство не отвечает" },
	{ RDRERR_INVREPLY,        "Неверный ответ устройства" }
};

PPDRV_INSTANCE_ERRTAB(Reader, 1, 0, PPDrvReader, _ErrMsgTab);

uint8 PPDrvReader::CalcCheckCode(const uint8 * pBuf, size_t dataLen) const
{
	uint8 c = 0;
	for(size_t i = 1; i < dataLen; i++) {
		c ^= pBuf[i];
	}
	return c;
}

int PPDrvReader::CardCodeToString(const uint8 * pCardCode, SString & rBuf) const
{
	(rBuf = 0).CatLongZ((long)pCardCode[2], 3).CatChar(',').Cat(swapw(*(uint16 *)(pCardCode+3)));
	return 1;
}
//
// Returns:
//   -1 - если соединение было установлено раньше
//    0 - ошибка
//    1 - соединение успешно установлено
int PPDrvReader::Connect(int portNum)
{
	int    ok = 1;
	if(!(State & stConnected)) {
		CommPortTimeouts cpt;
		Disconnect();
		CommPort.SetReadCyclingParams(10, 10);
		cpt.Get_NumTries = 0;
		cpt.Get_Delay    = 20;
		cpt.Put_NumTries = 0;
		cpt.Put_Delay    = 0;
		cpt.W_Get_Delay  = 0;
		CommPort.SetTimeouts(&cpt);
		THROWERR(CommPort.InitPort(portNum), RDRERR_OPENPORTFAILED);
		State |= stConnected;
	}
	else
		ok = -1;
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH;
	return ok;
}

int PPDrvReader::Disconnect()
{
	int    ok = 1;
	if(State & stConnected) {
		State &= ~stConnected;
	}
	else
		ok = -1;
	return ok;
}
//
// Returns:
//   -1 - нет входящих данных
//    0 - ошибка
//    1 - данные успешно считаны
//
int PPDrvReader::Listen(SString & rOutput)
{
	int    ok = 1, chr;
	uint8  cs = 0;
	uint8  data_buf[512];
	size_t data_size = 0;
	size_t eff_data_size = 0;
	SString msg_buf, temp_buf;

	MEMSZERO(data_buf);
	while(CommPort.GetChr(&chr) && data_size < sizeof(data_buf))
		data_buf[data_size++] = (uint8)chr;
	//THROWERR(data_size, RDRERR_NOREPLY);
	rOutput = 0;
	// Номер карты получается в виде 16-ой строки. Переводим в 10-ую. Символы берем попарно.
	if(data_size) {
		int    all_are_hex = 1;
		int    all_are_ascii = 1;
		int    cr_term = 0;
		uint   i;
		eff_data_size = data_size;
		for(i = 0; i < data_size; i++) {
			if(oneof2(data_buf[i], '\xD', '\xA')) {
				eff_data_size = i;
				break;
			}
			else {
				if(!isascii(data_buf[i])) {
					all_are_ascii = 0;
				}
				if(!ishex(data_buf[i])) {
					all_are_hex = 0;
				}
			}
		}
		temp_buf = 0;
		if(all_are_hex) {
			for(i = 0; i < data_size; i++)
				temp_buf.CatChar(data_buf[i]);
			DRVS.Log((msg_buf = "EmMarine driver readed HEX:").Space().Cat(temp_buf), 0xffff);
			i = 2; // Первые два байта (служебные пропускаем).
			while(i < (data_size-1) && data_buf[i] == '0' && data_buf[i+1] == '0')
				i += 2; // Пропускаем так же нули
			uint32 value = 0;
			//
			// Нам надо забрать 6 байт каждый из который представляет hex-полубайт.
			// Пример: F6ED7B
			//
			int    c = 16;
			while(c >= 0 && i < (data_size-1)) {
				uint8 byte = (hex(data_buf[i]) << 4) | hex(data_buf[i+1]);
				value = (value | (byte << c));
				c -= 8;
				i += 2;
			}
			rOutput.CatLongZ(value, 10);
			DRVS.Log((msg_buf = "EmMarine driver readed CODE:").Space().Cat(rOutput), 0xffff);
		}
		else if(all_are_ascii) {
			for(i = 0; i < data_size; i++) {
				temp_buf.CatChar(data_buf[i]);
			}
			temp_buf.Chomp();
			DRVS.Log((msg_buf = "EmMarine driver readed ASCII:").Space().Cat(temp_buf), 0xffff);
			{
				Scan.Set(temp_buf, 0);
				while(Scan[0] != 0) {
					if(Scan.GetRe(ReEmmId, rOutput)) {
						DRVS.Log((msg_buf = "EmMarine driver ASCII code detected:").Space().Cat(rOutput), 0xffff);
						break;
					}
					else
						Scan.Incr();
				}
			}
		}
		/*
		else if(data_size == 5) {
			CardCodeToString(data_buf, rOutput);
			DRVS.Log((msg_buf = "EmMarine driver readed 5 byte BINARY:").Space().Cat(rOutput), 0xffff);
		}*/
		else {
			for(i = 0; i < data_size; i++)
				temp_buf.CatHex(data_buf[i]);
			DRVS.Log((msg_buf = "EmMarine driver readed BINARY:").Space().Cat(temp_buf), 0xffff);
		}
		/*
		for(uint i = 0; i < data_size;) {
			if(ishex(data_buf[i]) && ishex(data_buf[i + 1])) {
				uint dig = hex(data_buf[i]) * 10 + hex(data_buf[i + 1]);
				rOutput.Cat(dig);
			}
			i += 2;
		}
		*/
	}
	/*
	CATCH
		ok = 0;
	ENDCATCH;
	*/
	return ok;
}

// virtual
int PPDrvReader::ProcessCommand(const SString & rCmd, const char * pInputData, SString & rOutput)
{
	int    err = 0;
	SString value;
	PPDrvInputParamBlock pb(pInputData);
	if(rCmd == "CONNECT") {
		int    port = (pb.Get("PORT", value) > 0) ? value.ToLong() : 0;
		THROW(Connect(port));
	}
	else if(rCmd == "LISTEN") {
		THROW(Listen(rOutput));
	}
	else if(rCmd == "INIT") {
	}
	else if(rCmd == "RELEASE") {
	}
	else { // Если дана неизвестная  команда, то сообщаем об этом
		DRVS.SetErrCode(serrInvCommand);
		err = 1;
	}
	CATCH
		err = 1;
		{
			SString msg_buf;
			DRVS.GetErrText(-1, value);
			DRVS.Log((msg_buf = "Reader: error").CatDiv(':', 2).Cat(value), 0xffff);
		}
	ENDCATCH;
	return err;
}
