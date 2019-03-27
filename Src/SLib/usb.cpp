// USB.CPP
// Для работы с usb-устройствами
//

// С классом USB работать намного проще, чем с HID.
// Для USB достаточно просто открыть устройство. Дальше можно спокойно читать и писать в него.
// Для HID необходимо еще сделать дополнительные настройки. Нижеследующие заметки относятся к этой теме.

// Заметка о том, что такое Usages
// Usages являются частью отчетного дескриптора и поставляется разработчику приложения с информацией о том,
// какое управление используется (Ха-ха, как же - поставляется). Кроме того, Usage тег указывает
// одобренный поставщиком способ использования для определенного элемента управления или группы элементов
// управления. Хотя отчетные дескрипторы описывают формат данных, Usage тег определяет, что должно быть
// сделано c данными.

// Заметка о том, что такое отчеты
// Используя USB-терминологию, устройство может отправлять и получать транзакцию каждый USB-кадр.
// Транзакция может быть составлена из нескольких пакетов, но имеет ограниченный размер (8 байтов
// для низкоскоростных, 64 байта для высокоскоростных устройств). Передача одной или нескольких транзакций
// создает набор данных, которые имеет смысл для устройства, например, Input, Output и Feature отчеты.
// Короче, отчеты, это команды и данные, передаваемые на устройство.

// Заметки списаны с http://www.microterm.ru/d/20158/d/hid_rus.pdf, где на чистом русском языке кратко
// описано функционирование usb.

#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <pp.h>

extern "C" {
	#include <Cfgmgr32.h>
	#include <setupapi.h>
	#include <winddk/hidsdi.h>
	#include <winddk/hidpi.h>
}

#define USB_GUID					"A5DCBF10L-6530-11D2-901F-00C04FB951ED"
#define HID_SUBSTR					"hid"
#define USB_SUBSTR					"usb"
#define OUTPUT_REPORT_BYTE_LENGTH	65 // Значение для высокоскоростных USB-устройств
#define INTPUT_REPORT_BYTE_LENGTH	65 // Значение для высокоскоростных USB-устройств
#define TIME_WAIT					1000 // Время ожидания (в миллисекундах)

SString & GetErrorStr();
//
// Descr: Возвращает детальную информацию об устройстве (используется в GetDeviceList())
//
int GetDevProperty(HDEVINFO handle, SP_DEVINFO_DATA * pDevInfo, int property, SString & rStr);

UsbDevDescrSt::UsbDevDescrSt()
{
}

UsbDevDescrSt::UsbDevDescrSt(const UsbDevDescrSt & rSrc)
{
	Path = rSrc.Path;
	Type = rSrc.Type;
	Class = rSrc.Class;
	ClassGUID = rSrc.ClassGUID;
	Description = rSrc.Description;
	SerialNumber = rSrc.SerialNumber;
	Driver = rSrc.Driver;
	HardwareID = rSrc.HardwareID;
	Manufacturer = rSrc.Manufacturer;
	PDOName = rSrc.PDOName;
	ServiceName = rSrc.ServiceName;
}

UsbDevDescrSt & UsbDevDescrSt::Clear()
{
	ClassGUID.Z();
	Path.Z();
	Type.Z();
	Class.Z();
	Description.Z();
	SerialNumber.Z();
	Driver.Z();
	HardwareID.Z();
	Manufacturer.Z();
	PDOName.Z();
	ServiceName.Z();
	return *this;
}

UsbBasicDescrSt::UsbBasicDescrSt() : P_Parent(0)
{
}

UsbBasicDescrSt::UsbBasicDescrSt(const UsbBasicDescrSt & rSrc)  : P_Parent(0)
{
	Pid = rSrc.Pid;
	Vid = rSrc.Vid;
	SerialNumber = rSrc.SerialNumber;
}

UsbBasicDescrSt & UsbBasicDescrSt::Clear()
{
	P_Parent = 0;
	Pid.Z();
	Vid.Z();
	SerialNumber.Z();
	return *this;
}

int UsbBasicDescrSt::operator == (const UsbBasicDescrSt & s) const
{
	return BIN(Vid.CmpNC(s.Vid) == 0 && Pid.CmpNC(s.Pid) == 0 && SerialNumber.CmpNC(s.SerialNumber) == 0);
}

SUsbDevice::SUsbDevice()
{
	DevClass = 0;
	OutputReportByteLength = 0;
	IntputReportByteLength = 0;
	Handle = INVALID_HANDLE_VALUE;
	Event = INVALID_HANDLE_VALUE; // new
	Ovl.hEvent = INVALID_HANDLE_VALUE; // new
	Ovl.Offset = 0; // new
	Ovl.OffsetHigh = 0; // new
	Description.Clear();
}

SUsbDevice::SUsbDevice(const UsbDevDescrSt * pDevDescr)
{
	if(pDevDescr) {
		DevClass = 0;
		OutputReportByteLength = 0;
		IntputReportByteLength = 0;
		Handle = INVALID_HANDLE_VALUE;
		Event = INVALID_HANDLE_VALUE; // new
		Description.Path = pDevDescr->Path;
		Description.Type = pDevDescr->Type;
		Description.Class = pDevDescr->Class;
		memcpy(&Description.ClassGUID, &pDevDescr->ClassGUID, sizeof(S_GUID));
		Description.Description = pDevDescr->Description;
		Description.SerialNumber = pDevDescr->SerialNumber;
		Description.Driver = pDevDescr->Driver;
		Description.HardwareID = pDevDescr->HardwareID;
		Description.Manufacturer = pDevDescr->Manufacturer;
		Description.PDOName = pDevDescr->PDOName;
		Description.ServiceName = pDevDescr->ServiceName;
	}
}

SUsbDevice::SUsbDevice(const SUsbDevice & rSrc)
{
	DevClass = rSrc.DevClass;
	OutputReportByteLength = rSrc.OutputReportByteLength;
	IntputReportByteLength = rSrc.IntputReportByteLength;
	Handle = rSrc.Handle;
	Event = rSrc.Event; // new
	Description = rSrc.Description;
	Children = rSrc.Children; // @vmiller Копирование TScollection
	ExtBuffer.Copy(rSrc.ExtBuffer);
}

SUsbDevice::~SUsbDevice()
{
	if(Handle != INVALID_HANDLE_VALUE)
		Close();
}

//static
// Разбирает имя устройства
// Разделитель между данными - #. Серийный номер идет после второго разделителя.
// После серийного номера идет GUID, который содержится в {}
// Пример Path:
//		\\?\usb#vid_05f9&pid_2203#s#n_e12g14133#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
// s#n_e12g14133 - принимаем за серийный номер. У других устройств может быть написано просто
//					e12g14133, то есть без s#n_
int SUsbDevice::ParsePath(const char * pPath, UsbBasicDescrSt & rDescr)
{
	int    ok = 0;
	SString str, left, right;
	if(pPath) {
		(str = pPath).Divide('#', left, right);
		(str = right).Divide('#', left, right);
		(str = left).Divide('&', rDescr.Vid, rDescr.Pid);
		(str = right).Divide('{', rDescr.SerialNumber, right);
		if(rDescr.SerialNumber.Last() == '#')
			rDescr.SerialNumber.TrimRight(); // Обрежем #
		ok = 1;
	}
	return ok;
}

//static
// Разбирает имя устройства
// Разделитель между данными - \. Серийный номер идет после второго разделителя.
// Пример Path:
//		HID\VID_05F9&PID_2203\7&3B4F0974&0&0000
int SUsbDevice::ParseSymbPath(const char * pPath, UsbBasicDescrSt & rDescr)
{
	int    ok = 0;
	SString str, left, right;
	if(pPath) {
		(str = pPath).Divide('\\', left, right);
		(str = right).Divide('\\', left, right);
		rDescr.SerialNumber = right;
		(str = left).Divide('&', rDescr.Vid, rDescr.Pid);
		rDescr.SerialNumber = right;
		ok = 1;
	}
	return ok;
}

int GetDevProperty(HDEVINFO handle, SP_DEVINFO_DATA * pDevInfo, int property, SString & rStr)
{
	int    ok = 1;
	DWORD  retnd_size;
	BYTE * p_buf = 0;
	THROW_S(handle != INVALID_HANDLE_VALUE, SLERR_USB);
	THROW_S(pDevInfo, SLERR_USB);
	if(!SetupDiGetDeviceRegistryProperty(handle, pDevInfo, (DWORD)property, NULL, NULL, 0, &retnd_size))
		THROW_S_S(GetLastError() == ERROR_INSUFFICIENT_BUFFER, SLERR_USB, GetErrorStr());
	if(retnd_size) {
		p_buf = new BYTE [retnd_size];
		memzero(p_buf, retnd_size);
		THROW_S_S(SetupDiGetDeviceRegistryProperty(handle, pDevInfo, (DWORD)property, NULL, p_buf, retnd_size, &retnd_size), SLERR_USB, GetErrorStr());
		rStr = (const char *)p_buf;
	}
	CATCHZOK
	ZDELETE(p_buf);
	return ok;
}

class SUsbDvcIfcData {
public:
	SUsbDvcIfcData();
	int    FASTCALL Get(void * pHandle, SP_DEVICE_INTERFACE_DATA * pDvcIfcData);
	int    GetPropString(void * pHandle, int prop, SString & rBuf);
	void   Reset();

	enum {
		fActive  = 0x0001,
		fDefault = 0x0002,
		fRemoved = 0x0004
	};
	S_GUID ClsGuid;
	S_GUID IfcClsGuid;
	uint32 Flags;
	uint32 DevInst;
	SString Path;
private:
	void * P_DvcInfo;

};

SUsbDvcIfcData::SUsbDvcIfcData() : P_DvcInfo(0), Flags(0), DevInst(0)
{
}

void SUsbDvcIfcData::Reset()
{
	ClsGuid.Z();
	IfcClsGuid.Z();
	DevInst = 0;
	Flags = 0;
	Path.Z();
	ZFREE(P_DvcInfo);
}

int FASTCALL SUsbDvcIfcData::Get(void * pHandle, SP_DEVICE_INTERFACE_DATA * pDvcIfcData)
{
	Reset();

	int    ok = 1;
	TCHAR  __buffer[256];
	size_t allocated_size = 0;
	uint32 ret_size = 0;
	SP_DEVICE_INTERFACE_DETAIL_DATA * p_dev_ifc_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA *)__buffer;
	SP_DEVINFO_DATA devinfo_data;
	ret_size = sizeof(__buffer);
	memzero(p_dev_ifc_detail, ret_size);
	p_dev_ifc_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	MEMSZERO(devinfo_data);
	devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
	int    ret = SetupDiGetDeviceInterfaceDetail(pHandle, pDvcIfcData, p_dev_ifc_detail, ret_size, &ret_size, &devinfo_data);
	if(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		THROW(p_dev_ifc_detail = (SP_DEVICE_INTERFACE_DETAIL_DATA *)SAlloc::M(ret_size));
		allocated_size = ret_size;
		memzero(p_dev_ifc_detail, ret_size);
		p_dev_ifc_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		MEMSZERO(devinfo_data);
		devinfo_data.cbSize = sizeof(SP_DEVINFO_DATA);
		ret = SetupDiGetDeviceInterfaceDetail(pHandle, pDvcIfcData, p_dev_ifc_detail, ret_size, &ret_size, &devinfo_data);
	}
	THROW_S(ret, SLERR_WINDOWS);
	THROW(P_DvcInfo = SAlloc::M(sizeof(devinfo_data)));
	memcpy(P_DvcInfo, &devinfo_data, sizeof(devinfo_data));
	ClsGuid.Init(devinfo_data.ClassGuid);
	DevInst = devinfo_data.DevInst;
	IfcClsGuid.Init(pDvcIfcData->InterfaceClassGuid);
	SETFLAG(Flags, fActive,  pDvcIfcData->Flags & SPINT_ACTIVE);
	SETFLAG(Flags, fDefault, pDvcIfcData->Flags & SPINT_DEFAULT);
	SETFLAG(Flags, fRemoved, pDvcIfcData->Flags & SPINT_REMOVED);
	Path = SUcSwitch(p_dev_ifc_detail->DevicePath); // @unicodeproblem
	CATCHZOK
	if(allocated_size) {
		ZFREE(p_dev_ifc_detail);
		allocated_size = 0;
	}
	return ok;
}

int SUsbDvcIfcData::GetPropString(void * pHandle, int prop, SString & rBuf)
{
	rBuf.Z();

	int    ok = 1;
	uint8  __buffer[512];
	uint8 * p_data = __buffer;
	size_t  allocated_size = 0;
	uint32  size = sizeof(__buffer);
	uint32  prop_type = 0;
	//SP_DEVINFO_DATA dev_info;

	//MEMSZERO(dev_info);
	//dev_info.cbSize = sizeof(dev_info);
	THROW(P_DvcInfo); // @todo error
	THROW_S(pHandle != INVALID_HANDLE_VALUE, SLERR_USB);
	int    ret = SetupDiGetDeviceRegistryProperty(pHandle, static_cast<SP_DEVINFO_DATA *>(P_DvcInfo), (DWORD)prop, &prop_type, p_data, size, &size);
	if(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		THROW(p_data = static_cast<uint8 *>(SAlloc::M(size)));
		allocated_size = size;
		//MEMSZERO(dev_info);
		//dev_info.cbSize = sizeof(dev_info);
		ret = SetupDiGetDeviceRegistryProperty(pHandle, static_cast<SP_DEVINFO_DATA *>(P_DvcInfo), (DWORD)prop, &prop_type, p_data, size, &size);
	}
	THROW(ret);
	if(oneof3(prop_type, REG_SZ, REG_MULTI_SZ, REG_EXPAND_SZ)) {
		rBuf = reinterpret_cast<const char *>(p_data);
	}
	CATCHZOK
	if(allocated_size) {
		ZFREE(p_data);
	}
	return ok;
}

// static
int SUsbDevice::GetDeviceList(TSCollection <SUsbDevice> & rList)
{
	int    ok = 1, cur_dev_index = 0, end_while = 0;
	size_t pos = 0;
	S_GUID usb_guid;
	HDEVINFO pnp_handle = 0;
	SP_DEVICE_INTERFACE_DATA dev_ifc_data;
	SString str;
	UsbDevDescrSt dev_descr;
	UsbBasicDescrSt basic_descr;
	SUsbDvcIfcData did;
	// Дескрпитор класса USB
	usb_guid.FromStr(USB_GUID);
	// Получаем дескриптор PnP для класса USB
	THROW_S_S((pnp_handle = SetupDiGetClassDevs(reinterpret_cast<const GUID *>(usb_guid.Data), NULL, 0,  DIGCF_PRESENT | DIGCF_DEVICEINTERFACE)) != INVALID_HANDLE_VALUE, SLERR_USB, GetErrorStr());
	// Цикл по всем устройствам в классе
	while(!end_while) {
		memzero(&dev_ifc_data, sizeof(SP_DEVICE_INTERFACE_DATA));
		dev_ifc_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		if(SetupDiEnumDeviceInterfaces(pnp_handle, NULL, reinterpret_cast<const GUID *>(usb_guid.Data), cur_dev_index, &dev_ifc_data)) {
			dev_descr.Clear();
			THROW(did.Get(pnp_handle, &dev_ifc_data));
			// Имя PnP-устройства
			dev_descr.Path = did.Path;
			// Из имени устройства вытащим серийный номер
			// Разделитель между данными - #. Серийный номер идет после второго разделителя.
			// После серийного номера идет GUID, который содержится в {}
			// Пример Path:
			//		\\?\usb#vid_05f9&pid_2203#s#n_e12g14133#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
			// s#n_e12g14133 - принимаем за серийный номер. У других устройств может быть написано просто
			//					e12g14133, то есть без s#n_
			basic_descr.Clear();
			ParsePath(dev_descr.Path, basic_descr);
			dev_descr.SerialNumber = basic_descr.SerialNumber;
			//
			// Тип устройства
			//
			switch(GetDriveType(SUcSwitch(did.Path))) { // @unicodeproblem
				case DRIVE_UNKNOWN: dev_descr.Type = "DRIVE_UNKNOWN"; break;
				case DRIVE_NO_ROOT_DIR: dev_descr.Type = "DRIVE_NO_ROOT_DIR"; break;
				case DRIVE_REMOVABLE: dev_descr.Type = "DRIVE_REMOVABLE"; break;
				case DRIVE_FIXED: dev_descr.Type = "DRIVE_FIXED"; break;
				case DRIVE_REMOTE: dev_descr.Type = "DRIVE_REMOTE"; break;
				case DRIVE_CDROM: dev_descr.Type = "DRIVE_CDROM"; break;
				case DRIVE_RAMDISK: dev_descr.Type = "DRIVE_RAMDISK"; break;
			}

			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_CLASS, dev_descr.Class), SLERR_USB, GetErrorStr());
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_DEVICEDESC, dev_descr.Description), SLERR_USB, GetErrorStr());
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_DRIVER,     dev_descr.Driver), SLERR_USB, GetErrorStr());
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_HARDWAREID, dev_descr.HardwareID), SLERR_USB, GetErrorStr());
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_MFG,        dev_descr.Manufacturer), SLERR_USB, GetErrorStr());
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_SERVICE,    dev_descr.ServiceName), SLERR_USB, GetErrorStr());
			// @? GUID класса уже есть в did {
			//THROW_S_S(did.GetPropString(pnp_handle, SPDRP_CLASSGUID, str), SLERR_USB, GetErrorStr());
			//dev_descr.ClassGUID.FromStr(str);
			// }
			dev_descr.ClassGUID = did.ClsGuid;
			THROW_S_S(did.GetPropString(pnp_handle, SPDRP_PHYSICAL_DEVICE_OBJECT_NAME, dev_descr.PDOName), SLERR_USB, GetErrorStr());
			// Для удобства преобразуем это имя, ибо оно имеет вид \Device\USBPDO-1, к примеру.
			// Отсечем начало, оставив только USBPDO-1
			{
				SString lstr, rstr;
				str = dev_descr.PDOName;
				for(;str.Divide('\\', lstr, rstr) > 0;) {
					str = rstr;
				}
				dev_descr.PDOName = str;
			}
			SUsbDevice * p_new_dvc = new SUsbDevice(&dev_descr);
			THROW_S(p_new_dvc, SLERR_NOMEM);
			{
				//
				// Ищем дочерние устройства
				//
				DWORD  device_ret = 0;
				DWORD  device_next = did.DevInst;
				TCHAR  buf[1024];
				buf[0] = 0;
				while(CM_Get_Child(&device_next, device_next, 0) == 0) {
					device_ret = device_next;
					// Получим строку вида:
					//		HID\VID_05F9&PID_2203\7&3B4F0974&0&0000
					if(CM_Get_Device_ID(device_ret, buf, SIZEOFARRAY(buf), 0) == CR_SUCCESS) { // @unicodeproblem
						// Делим ее на pid, vid и серийный номер
						basic_descr.Clear();
						ParseSymbPath(SUcSwitch(buf), basic_descr);
						UsbBasicDescrSt * p_child = new UsbBasicDescrSt(basic_descr);
						THROW_S(p_child, SLERR_NOMEM);
						p_child->P_Parent = p_new_dvc;
						p_new_dvc->Children.insert(p_child);
					}
				}
			}
			THROW(rList.insert(p_new_dvc));
		}
		else /* @sobolev if(GetLastError() == ERROR_NO_MORE_ITEMS)*/ {
			end_while = 1;
		}
		// Следующее устройство
		cur_dev_index++;
	}
	CATCH
		ok = -1;
	ENDCATCH;
	//ZDELETE(p_buf);
	if(pnp_handle)
		SetupDiDestroyDeviceInfoList(pnp_handle);
	return (cur_dev_index - 1);
}

int SUsbDevice::IsDev(const char * pPid, const char * pVid)
{
	size_t pos = 0;
	if(pPid && pVid)
		if(Description.HardwareID.Search(pPid, 0, 1, &(pos = 0)) && Description.HardwareID.Search(pVid, 0, 1, &(pos = 0)))
			return 1;
	return 0;
}

int SUsbDevice::SetConfig()
{
	int    ok = 1, r = 0;
	char * p_buf = 0;
	SString str;
	USAGE usage;
	PHIDP_PREPARSED_DATA p_prep_data = 0;
	PHIDP_VALUE_CAPS p_val_cap = 0;
	PHIDP_VALUE_CAPS p_init_val_cap = 0;
	USHORT num_values = 0, num_caps = 0;
	HIDP_CAPS hid_caps;

	THROW_S_S(HidD_GetPreparsedData(Handle, &p_prep_data), SLERR_USB, GetErrorStr());
	memzero(&hid_caps, sizeof(HIDP_CAPS));
	THROW_S_S(HidP_GetCaps(p_prep_data, &hid_caps) == HIDP_STATUS_SUCCESS, SLERR_USB, GetErrorStr());
	p_val_cap = p_init_val_cap = new HIDP_VALUE_CAPS [sizeof(PHIDP_VALUE_CAPS) * hid_caps.NumberOutputValueCaps];
    num_caps = hid_caps.NumberOutputValueCaps;
	// Получаем настройки устройства
	THROW_S_S(HidP_GetValueCaps(HidP_Output, p_val_cap, &num_caps, p_prep_data) == HIDP_STATUS_SUCCESS, SLERR_USB, GetErrorStr());
    num_values = 0;
    for(size_t i = 0; i < hid_caps.NumberOutputValueCaps; i++, p_val_cap++) {
        if(p_val_cap->IsRange) {
            num_values += p_val_cap->Range.UsageMax - p_val_cap->Range.UsageMin + 1;
        }
        else
            num_values++;
    }
	// Возьмем самое последнее значение usage. По крайней мере, оно подходит для указания размера
	// передаваемых на устройство данных и именно для Viki Vision
	p_val_cap = p_init_val_cap;
	p_val_cap += num_values - 1;
	if(p_val_cap->IsRange)
		usage = p_val_cap->Range.UsageMax;
	else
		usage = p_val_cap->NotRange.Usage;

	// Настраиваем устройство для отправки данных
	p_buf = new char[hid_caps.OutputReportByteLength];
	memzero(p_buf, hid_caps.OutputReportByteLength);
	if((r = HidP_SetUsageValue(HidP_Output,	// Тип операции (запись на устройство)
        hid_caps.UsagePage,
        0, // All Collections.
        usage,
        1, // Передаваемые данные (число байт данных). Но, по ходу, вообще все что угодно можно сюда ставить (опять же. Это прокатывает с VikiVision)
        p_prep_data,
        p_buf, // Указатель на выходящий Report
		hid_caps.OutputReportByteLength)) != HIDP_STATUS_SUCCESS) {
		switch(r) { // Проблема в том, что GetLastError не вернет эти коды
			case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
				str = "The routine successfully set the usage value";
				break;
			case HIDP_STATUS_INVALID_PREPARSED_DATA:
				str = "The usage does not exist in the specified report, but it does exist in a different report of the specified type";
				break;
			case HIDP_STATUS_REPORT_DOES_NOT_EXIST:
				str = "The preparsed data is not valid";
				break;
			case HIDP_STATUS_USAGE_NOT_FOUND:
				str = "There are no reports of the specified type";
				break;
			case HIDP_STATUS_INVALID_REPORT_LENGTH:
				str = "The usage does not exist in any report of the specified report type";
				break;
			case HIDP_STATUS_INVALID_REPORT_TYPE:
				str = "The report length is not valid";
				break;
		}
		THROW_S_S(0, SLERR_USB, str);
	}
	OutputReportByteLength = hid_caps.OutputReportByteLength;
	IntputReportByteLength = hid_caps.InputReportByteLength;
	CATCHZOK
	ZDELETE(p_buf);
	ZDELETE(p_init_val_cap);
	p_val_cap = 0;
	return ok;
}

int SUsbDevice::Open()
{
	int    ok = 1;
	SString str;
	Close();
	// @unicodeproblem
	if((Handle = ::CreateFile(SUcSwitch(Description.Path), GENERIC_READ|GENERIC_WRITE, 
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, /*0*/FILE_FLAG_OVERLAPPED, 0)) == INVALID_HANDLE_VALUE) {
		// Если устройство только читает // @unicodeproblem
		THROW_S((Handle = ::CreateFile(SUcSwitch(Description.Path), GENERIC_READ, 
			FILE_SHARE_READ, NULL, OPEN_EXISTING, /*0*/FILE_FLAG_OVERLAPPED, 0)) != INVALID_HANDLE_VALUE, SLERR_USB);
	}
	if(!Description.Class.CmpNC(HID_SUBSTR)) {
		DevClass = clsHid;
		THROW(SetConfig()); // У него своя специфика кодов ошибок
	}
	else if(!Description.Class.CmpNC(USB_SUBSTR)) {
		DevClass = clsUsb;
		OutputReportByteLength = OUTPUT_REPORT_BYTE_LENGTH;
		IntputReportByteLength = INTPUT_REPORT_BYTE_LENGTH;
	}
	else {
		THROW_S(0, SLERR_USB_HIDUSBCLASSFAILED);
	}
	// new {
	// Создаем события с пользовательским сбросом
	Event = CreateEvent(NULL, TRUE, TRUE, 0);
	THROW_S(Event != INVALID_HANDLE_VALUE, SLERR_USB);
	Ovl.hEvent = Event;
	Ovl.Offset = 0;
	Ovl.OffsetHigh = 0;
	// } new
	CATCHZOK
	return ok;
}

int SUsbDevice::Close()
{
	if(Handle != INVALID_HANDLE_VALUE) {
		CloseHandle(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}
	if(Event != INVALID_HANDLE_VALUE) {
		CloseHandle(Event);
		Event = INVALID_HANDLE_VALUE;
	}
	return 1;
}

int SUsbDevice::Write(const void * pBuf, size_t bufSize)
{
	int    ok = 1, res = 0;
	size_t buf_size = bufSize;
	DWORD  retnd_size = 0;
	char * p_buf = 0;
	char * p_start_buf = 0;
	char * p_segm_buf = 0;
	SString str;
	THROW_S(Handle != INVALID_HANDLE_VALUE, SLERR_USB);
	THROW_S(Event != INVALID_HANDLE_VALUE, SLERR_USB);
	THROW_S(pBuf, SLERR_USB);
	if(OutputReportByteLength) {
		p_segm_buf = new char[OutputReportByteLength];
		memzero(p_segm_buf, OutputReportByteLength);
		p_buf = p_start_buf = new char[bufSize];
		memcpy(p_buf, pBuf, bufSize);
		if(DevClass == clsHid) {
			// В цикле подаем команду на устройство
			for(; buf_size; ) {
				memzero(p_segm_buf, OutputReportByteLength);
				// В начало буфера нулевой байт оставляем нулем, а в 1-й пишем размер передаваемой подстроки
				*(p_segm_buf + 1) = (buf_size < (OutputReportByteLength - 2)) ? (char)buf_size : (OutputReportByteLength - 2);
				memcpy(p_segm_buf + 2, p_buf, OutputReportByteLength - 2);
				p_buf += OutputReportByteLength - 2;
				buf_size -= (buf_size < (OutputReportByteLength - 2)) ? (char)buf_size : (OutputReportByteLength - 2);
				retnd_size = 0;
				THROW_S_S(WriteFile(Handle, p_segm_buf, OutputReportByteLength, (LPDWORD)&retnd_size, /*NULL*/(LPOVERLAPPED)&Ovl), SLERR_USB, GetErrorStr()); // @v10.2.3 @fix retnd_size-->&retnd_size
				THROW_S_S(WaitForSingleObject(Event, INFINITE) != WAIT_FAILED, SLERR_USB, GetErrorStr()); // new Ждем окончания работы процесса
			}
		}
		else {
			// В цикле подаем команду на устройство
			for(; buf_size; ) {
				memzero(p_segm_buf, OutputReportByteLength);
				*p_segm_buf = (buf_size < OutputReportByteLength) ? (char)buf_size : OutputReportByteLength;
				memcpy(p_segm_buf + 1, p_buf, (buf_size < (OutputReportByteLength - 1)) ? buf_size : (OutputReportByteLength - 1));
				p_buf += (buf_size < (OutputReportByteLength - 1)) ? buf_size : (OutputReportByteLength - 1);
				buf_size -= (buf_size < (OutputReportByteLength - 1)) ? buf_size : (OutputReportByteLength - 1);
				retnd_size = 0;
				THROW_S_S(WriteFile(Handle, p_segm_buf, OutputReportByteLength, &retnd_size, /*NULL*/(LPOVERLAPPED)&Ovl), SLERR_USB, GetErrorStr()); // new
				THROW_S_S(WaitForSingleObject(Event, INFINITE) != WAIT_FAILED, SLERR_USB, GetErrorStr()); // new Ждем окончания работы процесса
			}
		}
	}
	CATCHZOK
	ZDELETE(p_start_buf);
	ZDELETE(p_segm_buf);
	return ok;
}

// Descr: Запускаем, пока функция не вернет -1. Перед этой функцией желательно вызвать GetInputReportDataLength()
// ARG(pBuf		OUT): Буфер, куда запишутся считанные данные
// ARG(bufSize	 IN): Размер выходного буфера. Желательно, чтобы размер был не менее SUsbDevice::IntputReportByteLength
// Returns:
//		-1 - входных данных нет
//		 0 - ошибка
//		 > 1 - число считанных байт
int SUsbDevice::Read(void * pBuf, size_t bufSize)
{
	int ok = 1, res = 0;
	DWORD  retnd_size = 0;
	char * p_buf = 0;

	THROW_S(Handle != INVALID_HANDLE_VALUE, SLERR_USB);
	THROW_S(Event != INVALID_HANDLE_VALUE, SLERR_USB);
	THROW_S(pBuf, SLERR_USB);
	p_buf = new char[IntputReportByteLength];
	memzero(p_buf, IntputReportByteLength);
	// Будем надеятся, что операция чтения у usb и hid происходит одинаково
	if(!ReadFile(Handle, p_buf, (DWORD)bufSize, NULL, (LPOVERLAPPED)&Ovl)) { // Если вернул FALSE, то это либо ошибка,
			// либо чтение закончилось асинхронно. В это случае смотрим дальше: либо опять-таки ошибка, либо все нормально.
		res = GetLastError();
		// ERROR_HANDLE_EOF - чтение окончено
		// ERROR_IO_PENDING - процесс в режиме ожидания, что в общем-то не является ошибкой
		THROW_S_S((res == ERROR_HANDLE_EOF) || (res == ERROR_IO_PENDING), SLERR_USB, GetErrorStr());
	}
	// Функции ниже, можно сказать, взаимоисключающие. Обе ждут завершения процесса.
	// Но в WaitForSingleObject() можно задать время этого ожидания, а GetOverlappedResult() возвращает
	// количество считанных байт.
	// Так что выбираем, что лучше.
	// 1-й вариант
	THROW_S_S((res = WaitForSingleObject(Event, TIME_WAIT)) != WAIT_FAILED, SLERR_USB, GetErrorStr()); // Ждем окончания работы процесса в течение TIME_WAIT миллисекунд
	// Если процедура завершилась по таймауту, то, значит, входных данных нет
	if(res == WAIT_TIMEOUT)
        ok = -1;
	// 2-й вариант
	// Получим число считанных байт
	// При этом, если res был равен ERROR_HANDLE_EOF, то функция ниже вернет ноль
	if(ok) {
		GetOverlappedResult(Handle, (LPOVERLAPPED)&Ovl, &retnd_size, FALSE); // Последний параметр говорит, что мы не ждем окончания выполнения операции чтения
	}
	memcpy(pBuf, p_buf, bufSize);
	CATCHZOK
	ZDELETE(p_buf);
	return ok;
}

const UsbDevDescrSt & SUsbDevice::GetDescription() const
{
	return Description;
}

const TSCollection <UsbBasicDescrSt> & SUsbDevice::GetChildren() const
{
	return Children;
}

SString & GetErrorStr()
{
	static SString err_msg;
	//TCHAR  buf[256];
	//ulong  code = GetLastError();
	//::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), buf, SIZEOFARRAY(buf), 0); // @unicodeproblem
	//(err_msg = SUcSwitch(buf)).ToOem();
	// @v10.3.11 {
	SSystem::SFormatMessage(err_msg); 
	err_msg.Chomp().Transf(CTRANSF_OUTER_TO_INNER);
	// } @v10.3.11 
	return err_msg;
}

int SUsbDevice::GetInputReportDataLength()
{
	return IntputReportByteLength;
}
//
//
//

// User32.lib

typedef WINUSERAPI UINT (WINAPI * GETRAWINPUTDATA)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);
typedef WINUSERAPI UINT (WINAPI * GETRAWINPUTDEVICEINFOA)(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize);
//typedef WINUSERAPI UINT (WINAPI * GETRAWINPUTDEVICEINFOW)(HANDLE hDevice, UINT uiCommand, LPVOID pData, PUINT pcbSize);
typedef WINUSERAPI BOOL (WINAPI * REGISTERRAWINPUTDEVICES)(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);

static int RawInputProcStatus;
static GETRAWINPUTDATA GetRawInputDataProc;
static GETRAWINPUTDEVICEINFOA GetRawInputDeviceInfoProc;
static REGISTERRAWINPUTDEVICES RegisterRawInputDevicesProc;
/*
		typedef NET_API_STATUS (WINAPI * NETREMOTETOD)(LPCWSTR UncServerName, LPBYTE* BufferPtr);
		typedef NET_API_STATUS (WINAPI * NETAPIBUFFERFREE)(LPVOID Buffer);
		SDynLibrary lib_netapi("NETAPI32.DLL");
		if(lib_netapi.IsValid()) {
			NETREMOTETOD proc_NetRemoteTOD = (NETREMOTETOD)lib_netapi.GetProcAddr("NetRemoteTOD");
*/

int SRawInputData::InitRawInputProc(int unreg)
{
	static SDynLibrary * p_lib_user32 = 0;
	if((unreg && RawInputProcStatus > 0) || (!unreg && RawInputProcStatus == 0)) {
		ENTER_CRITICAL_SECTION
		if(unreg) {
			if(RawInputProcStatus > 0) {
				RegisterRawInputDevicesProc = 0;
				GetRawInputDataProc = 0;
				GetRawInputDeviceInfoProc = 0;
				ZDELETE(p_lib_user32);
				RawInputProcStatus = 0;
			}
		}
		else if(RawInputProcStatus == 0) {
			p_lib_user32 = new SDynLibrary("USER32.DLL");
			if(p_lib_user32 && p_lib_user32->IsValid()) {
				RegisterRawInputDevicesProc = (REGISTERRAWINPUTDEVICES)p_lib_user32->GetProcAddr("RegisterRawInputDevices");
				GetRawInputDataProc = (GETRAWINPUTDATA)p_lib_user32->GetProcAddr("GetRawInputData");
				GetRawInputDeviceInfoProc = (GETRAWINPUTDEVICEINFOA)p_lib_user32->GetProcAddr("GetRawInputDeviceInfoA");
				if(RegisterRawInputDevicesProc && GetRawInputDataProc && GetRawInputDeviceInfoProc)
					RawInputProcStatus = 1;
				else {
					ZDELETE(p_lib_user32);
					RawInputProcStatus = -1;
				}
			}
			else {
				ZDELETE(p_lib_user32);
				RawInputProcStatus = -1;
			}
		}
		LEAVE_CRITICAL_SECTION
	}
	return RawInputProcStatus;
}


SRawInputInitArray::SRawInputInitArray() : SVector(sizeof(RAWINPUTDEVICE)) // @v9.8.7 SArray-->SVector
{
}

int SRawInputInitArray::Add(uint16 usagePage, uint16 usage, uint flags, HWND target)
{
	RAWINPUTDEVICE rid;
	rid.usUsagePage = usagePage;
	rid.usUsage = usage;
	rid.dwFlags = flags;
	rid.hwndTarget = target;
	return insert(&rid);
}

//static
int SRawInputData::Register(SRawInputInitArray * pRiia)
{
	int    ok = 0;
	if(InitRawInputProc(0) > 0) {
		if(pRiia) {
			ok = RegisterRawInputDevicesProc((RAWINPUTDEVICE *)pRiia->dataPtr(), pRiia->getCount(), sizeof(RAWINPUTDEVICE));
		}
		else {
			RAWINPUTDEVICE rid;
			MEMSZERO(rid);
			ok = RegisterRawInputDevicesProc(&rid, 0, sizeof(RAWINPUTDEVICE));
		}
	}
	return ok;
}

SRawInputData::SRawInputData() : AllocatedSize(0), P_Buf(FixedBuffer)
{
}

SRawInputData::~SRawInputData()
{
	Reset();
}

void SRawInputData::Reset()
{
	if(AllocatedSize) {
		ZFREE(P_Buf);
		AllocatedSize = 0;
	}
	P_Buf = FixedBuffer;
	memzero(P_Buf, sizeof(FixedBuffer));
}

SRawInputData::operator RAWINPUT * ()
{
	return static_cast<RAWINPUT *>(P_Buf);
}

int FASTCALL SRawInputData::Get(/*long*/void * rawInputHandle)
{
	int    ok = 1;
	uint   buf_size = 0;
	Reset();
	THROW(InitRawInputProc(0) > 0);
	GetRawInputDataProc((HRAWINPUT)rawInputHandle, RID_INPUT, NULL, &buf_size, sizeof(RAWINPUTHEADER));
	if(buf_size > sizeof(FixedBuffer)) {
		THROW(P_Buf = SAlloc::M(buf_size));
		if(P_Buf) {
			memzero(P_Buf, buf_size);
			AllocatedSize = buf_size;
		}
		else {
			memzero(P_Buf, sizeof(FixedBuffer));
			P_Buf = FixedBuffer;
		}
	}
	int    ret = GetRawInputDataProc((HRAWINPUT)rawInputHandle, RID_INPUT, P_Buf, &buf_size, sizeof(RAWINPUTHEADER));
	THROW(ret >= 0);
	CATCH
		Reset();
		ok = 0;
	ENDCATCH
	return ok;
}

int FASTCALL SRawInputData::GetDeviceName(SString & rBuf)
{
	rBuf.Z();

	int    ok = -1;
	char   __buffer[512];
	void * p_buf = __buffer;
	size_t alloc_sz = 0;
	if(InitRawInputProc(0) > 0) {
		uint   data_size = sizeof(__buffer);
		HANDLE h_dvc = ((RAWINPUT *)P_Buf)->header.hDevice;
		int    ret = GetRawInputDeviceInfoProc(h_dvc, RIDI_DEVICENAME, NULL, &data_size);
		THROW(ret >= 0);
		if(data_size) {
			if(data_size > sizeof(__buffer)) {
				THROW_MEM(p_buf = SAlloc::M(data_size));
				memzero(p_buf, data_size);
				alloc_sz = data_size;
			}
			ret = GetRawInputDeviceInfoProc(h_dvc, RIDI_DEVICENAME, p_buf, &data_size);
			THROW(ret > 0);
			rBuf = (const char *)p_buf;
			ok = 1;
		}
	}
	CATCHZOK
	if(alloc_sz)
		SAlloc::F(p_buf);
	return ok;
}
