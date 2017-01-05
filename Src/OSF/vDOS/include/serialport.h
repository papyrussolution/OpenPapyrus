#ifndef vDOS_SERIALPORT_H
#define vDOS_SERIALPORT_H

#ifndef VDOS_H
#include "vDos.h"
#endif
#ifndef vDOS_INOUT_H
#include "inout.h"
#endif

#include "devicePRT.h"

class CSerial
	{
public:
	// Constructor takes com port number (0-3)
	CSerial(Bitu portnr, device_PRT* dosdevice);
	virtual ~CSerial();
		
	void Putchar(Bit8u data);
	bool Getchar(Bit8u* data);
private:
	DOS_Device* mydosdevice;
	};

extern CSerial* serialPorts[];
const Bit16u serial_baseaddr[] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};

#endif
