#include "vDos.h"
#include "bios.h"					// SetLPTPort(..)

#include "parport.h"

void CParallel::Putchar(Bit8u val)
	{
	Bit16u size = 1;
	mydosdevice->Write(&val, &size);
	}

Bitu CParallel::Read_SR()
	{
	if (!ack)
		return 0xdf;
	ack = false;
	return 0x9f;
	}

void CParallel::Write_PR(Bitu val)
	{
	datareg = (Bit8u)val;
	}

void CParallel::Write_CON(Bitu val)
	{
	// init printer if bit 4 is switched on
	// ...
	autofeed = ((val & 0x02) != 0);		// autofeed adds 0xa if 0xd is sent

	// data is strobed to the parallel printer on the falling edge of strobe bit
	if ((!(val & 1)) && (controlreg & 1))
		{
		Putchar(datareg);
		if (autofeed && (datareg == 0xd))
			Putchar(0xa);
		ack = true;
		}
	controlreg = val;
	}

static Bit8u PARALLEL_Read (Bitu port)
	{
	for (Bitu i = 0; i < 3; i++)
		{
		if (parallel_baseaddr[i] == (port&0xfffc) && (parallelPorts[i]))
			{
			switch (port&0x7)
				{
			case 0:
				return parallelPorts[i]->datareg;
			case 1:
				return parallelPorts[i]->Read_SR();
			case 2:
				return 0;
				}
			return 0xff;	
			}
		}
	return 0xff;
	}

static void PARALLEL_Write (Bitu port, Bitu val)
	{
	for (Bitu i = 0; i < 3; i++)
		if (parallel_baseaddr[i] == (port&0xfffc) && parallelPorts[i])
			switch (port & 0x3)
				{
			case 0:
				parallelPorts[i]->Write_PR (val);
				return;
			case 1:
				return;
			case 2:
				parallelPorts[i]->Write_CON (val);
				return;
				}
	}

// Initialisation
CParallel::CParallel(Bitu portno, device_PRT* dosdevice)
	{
	portnum = portno;
	Bit16u base = parallel_baseaddr[portnum];

	for (Bitu i = 0; i < 3; i++)
		{
		IO_RegisterWriteHandler(i+base, PARALLEL_Write);
		IO_RegisterReadHandler(i+base, PARALLEL_Read);
		}
	BIOS_SetLPTPort(portnum, base);
	mydosdevice = dosdevice;
	};

CParallel::~CParallel(void)
	{
	BIOS_SetLPTPort(portnum, 0);
	};

Bit8u CParallel::getPrinterStatus()
	{
	/*	7      not busy
		6      acknowledge
		5      out of paper
		4      selected
		3      I/O error
		2-1    unused
		0      timeout  */
	Bit8u statusreg = Read_SR()^0x48;
	return statusreg&~0x7;
	}

void CParallel::initialize()
	{
	Write_CON(0x08);			// init low
	Write_PR(0);
	Write_CON(0x0c);			// init high
	}

CParallel* parallelPorts[3] = {0, 0, 0};
static device_PRT* dosdevices[9];

void PARALLEL_Init ()
	{
//	Section_prop *section = static_cast <Section_prop*>(sec);
	char pname[] = "LPTx";
	// iterate through first 3 lpt ports and the rest (LPT4-9)
	for (Bitu i = 0; i < 9; i++)
		{
		pname[3] = '1' + i;
		dosdevices[i] = new device_PRT(pname, ConfGetString(pname));
		DOS_AddDevice(dosdevices[i]);
		if (i < 3)
			parallelPorts[i] = new CParallel(i, dosdevices[i]);
		}
	}
