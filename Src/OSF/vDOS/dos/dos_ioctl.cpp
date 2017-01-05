#include <string.h>
#include "vDos.h"
#include "mem.h"
#include "regs.h"
#include "dos_inc.h"

bool DOS_IOCTL(void)
	{
	Bitu handle = 0;
	if (reg_al < 4 || reg_al == 6 || (reg_al == 7) || reg_al == 0x0a)				// These use a file handle
		{
		handle = RealHandle(reg_bx);
		if (handle >= DOS_FILES || !Files[handle])
			{
			DOS_SetError(DOSERR_INVALID_HANDLE);
			return false;
			}
		}
	switch (reg_al)
		{
	case 0:																			// Get Device Information
		if (Files[handle]->GetInformation() & 0x8000)								// Check for device
			reg_dx = Files[handle]->GetInformation();
		else
			{
			Bit8u hdrive = Files[handle]->GetDrive();
			if (hdrive == 0xff)
				hdrive = 2;															// Defaulting to C: (in time look at this, shouldn''t happen)
			reg_dx = (Files[handle]->GetInformation()&0xffe0)|hdrive;				// Return drive number in lower 5 bits for block devices
			}
		reg_ax = reg_dx;															// Destroyed officially
		return true;
	case 1:																			// Set Device Information
		if (reg_dh != 0)
			{
			DOS_SetError(DOSERR_DATA_INVALID);
			return false;
			}
		if ((Files[handle]->GetInformation()&0x8000) == 0)							// Check for device
			{
			DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
			return false;
			}
		return true;
	case 2:																			// Read from Device Control Channel
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
	case 3:																			// Write to Device Control Channel
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
	case 6:																			// Get input status
		if (Files[handle]->GetInformation() & 0x8000)								// Check for device
			reg_al = (Files[handle]->GetInformation() & 0x40) ? 0 : 0xff;
		else
			{																		// FILE
			Bit32u oldlocation = 0;
			Files[handle]->Seek(&oldlocation, DOS_SEEK_CUR);
			Bit32u endlocation = 0;
			Files[handle]->Seek(&endlocation, DOS_SEEK_END);
			if (oldlocation < endlocation)
				reg_al = 0xff;														// Still data available
			else
				reg_al = 0;															// EOF or beyond
			Files[handle]->Seek(&oldlocation, DOS_SEEK_SET);						// Restore filelocation
			}
		return true;
	case 7:																			// Get output status
		reg_al = 0xff;																// Ready
		return true;
	case 8:																			// Check if block device removable
	case 9:																			// Check if block device remote
		{			
		Bit8u drive = reg_bl;
		if (!drive)
			drive = DOS_GetDefaultDrive();
		else
			drive--;
		if (!((drive < DOS_DRIVES) && Drives[drive]))
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		if (reg_al == 8)
			reg_ax = 1;																// Fixed
		else if (Drives[drive]->remote)
			reg_dx = 0x1000;
		else
			reg_dx = 0x0200;														// Local, drive doesn't support direct I/O
		return true;
		}
	case 0x0a:																		// Is device or handle remote?
		{
		reg_dx = 0;																	// Local for now
		if ((Files[handle]->GetInformation() & 0x8000) == 0)						// If file
			{
			Bit8u hdrive = Files[handle]->GetDrive();
			if (hdrive == 0xff)
				hdrive = 2;															// Defaulting to C: (in time look at this, shouldn''t happen)
			if (Drives[hdrive]->remote)
				reg_dx = 0x8000;
			}
		return true;
		}
	case 0x0b:																		// Set sharing retry count
		return true;
	case 0x0e:																		// Get logical drive map
		reg_al = 0;																	// Only 1 logical drive assigned
		return true;
	case 0x52:																		// DETERMINE DOS TYPE/GET DR DOS VERSION
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
	default:
		vLog("DOS:IOCTL Call %2X unhandled", reg_al);
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		break;
		}
	return false;
	}

bool DOS_GetSTDINStatus(void)
	{
	Bit32u handle = RealHandle(STDIN);
	if (handle == 0xFF)
		return false;
	if (Files[handle] && (Files[handle]->GetInformation() & 64))
		return false;
	return true;
	}
