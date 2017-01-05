#ifndef vDOS_PARPORT_H
#define vDOS_PARPORT_H

#ifndef VDOS_H
#include "vDos.h"
#endif
#ifndef vDOS_INOUT_H
#include "inout.h"
#endif

#include "devicePRT.h"

class CParallel
	{
public:
	// Constructor
	CParallel(Bitu portnr, device_PRT* dosdevice);
	
	virtual ~CParallel();

	void Putchar(Bit8u);
	
	Bitu portnum;
	
	// read data line register
	Bitu Read_SR();
	void Write_PR(Bitu);
	void Write_CON(Bitu);

	Bit8u datareg;
	Bit8u controlreg;

	bool autofeed;
	bool ack;
	Bit8u getPrinterStatus();
	void initialize();

private:
	device_PRT* mydosdevice;
	};

extern CParallel* parallelPorts[];

const Bit16u parallel_baseaddr[3] = {0x378, 0x278, 0x3bc};

#endif

