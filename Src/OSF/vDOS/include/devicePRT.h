#ifndef vDOS_DEVICEPRT_H
#define vDOS_DEVICEPRT_H

#ifndef VDOS_H
#include "vDos.h"
#endif
#ifndef vDOS_INOUT_H
#include "inout.h"
#endif
#ifndef LPT_SHORTTIMEOUT
#define LPT_SHORTTIMEOUT 1000					// Printers timeout after 1 sec after closing device w/o last formfeed
#define LPT_LONGTIMEOUT 5000					// and after 5 secs w/o receiving any data
#endif


#include "dos_inc.h"
#include <vector>

void LPT_CheckTimeOuts(Bit32u mSecsCurr);

class device_PRT : public DOS_Device
	{
public:
	// Creates a LPT device that communicates with the num-th parallel port, i.e. is LPTnum
	device_PRT(const char* pname, const char* cmd);
	~device_PRT();
	void	Close();
	bool	Read(Bit8u * data, Bit16u * size);
	bool	Write(Bit8u * data, Bit16u * size);
	Bit16u	GetInformation(void);

private:
	void	CommitData();
	char	tmpAscii[10];
	char	tmpUnicode[10];
	bool	useDP;
	bool	nothingSet;
	bool	fastCommit;
	intptr_t DPhandle;					// Handle to previous DOSPrinter process
	Bit32u	DPexitcode;
	std::string destination;			// where to send the output to
	std::string rawdata;				// the raw data sent to LPTx...
	};

#endif