#include "vdos.h"
#include "bios.h"					// SetComPorts(..)

#include "serialport.h"

void CSerial::Putchar(Bit8u val)
	{
	Bit16u size = 1;
	mydosdevice->Write(&val, &size);
	}

static Bit8u SERIAL_Read (Bitu port)
	{
	return 0xff;
	}

static void SERIAL_Write (Bitu port, Bitu val)
	{
	for (Bitu i = 0; i < 4; i++)
		if (serial_baseaddr[i] == port && serialPorts[i])
			{
			serialPorts[i]->Putchar(val);
			return;
			}
	}

// Initialisation
CSerial::CSerial(Bitu portno, device_PRT* dosdevice)
	{
	Bit16u base = serial_baseaddr[portno];
	
	for (Bitu i = 0; i <= 7; i++)
		{
		IO_RegisterWriteHandler(i+base, SERIAL_Write);
		IO_RegisterReadHandler(i+base, SERIAL_Read);
		}
	mydosdevice = dosdevice;
	}

CSerial::~CSerial(void)
	{
	};

bool CSerial::Getchar(Bit8u* data)
	{
	return false;			// TODO actualy read something
	}

CSerial* serialPorts[4] = {0, 0, 0, 0};
static device_PRT* dosdevices[9];

void SERIAL_Init()
	{
	char sname[] = "COMx";
	// iterate through first 4 com ports and the rest (COM5-9)
	for (Bitu i = 0; i < 9; i++)
		{
		sname[3] = '1' + i;
		dosdevices[i] = new device_PRT(sname, ConfGetString(sname));
		DOS_AddDevice(dosdevices[i]);
		if (i < 4)
			serialPorts[i] = new CSerial(i, dosdevices[i]);
		}
	BIOS_SetComPorts((Bit16u*)serial_baseaddr);
	}

