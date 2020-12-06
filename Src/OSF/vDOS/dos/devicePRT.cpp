//
//
#include "vDos.h"
#include "devicePRT.h"
#include "process.h"
#include <Shellapi.h>
#include "support.h"

void LPT_CheckTimeOuts(Bit32u mSecsCurr)
	{
	for (int dn = 0; dn < DOS_DEVICES; dn++)
		if (Devices[dn] && Devices[dn]->timeOutAt != 0)
			if (Devices[dn]->timeOutAt <= mSecsCurr)
				{
				Devices[dn]->timeOutAt = 0;
				Devices[dn]->Close();
				return;																// One device per run cycle
				}
	}

bool device_PRT::Read(Bit8u * data, Bit16u * size)
	{
	if (!strcmp("clipboard", destination.c_str()))
		if (OpenClipboard(NULL))
			{	
			if (HANDLE cbText = GetClipboardData(CF_UNICODETEXT))
				{
				Bit16u *p = (Bit16u *)GlobalLock(cbText);
				*size = Unicode2Ascii(p, data, *size);
				GlobalUnlock(cbText);
				}
			CloseClipboard();
			return true;
			}
	*size = 0;
	return true;
	}

bool device_PRT::Write(Bit8u * data, Bit16u * size)
	{
	Bit8u * datasrc = data;
	Bit8u * datadst = data;

	int numSpaces = 0;
	for (Bit16u idx = *size; idx; idx--)
		{
		if (*datasrc == 0x0c)
			ffWasLast = true;
		else if (!isspace(*datasrc))
			ffWasLast = false;
		if (*datasrc == ' ')														// Put spaces on hold
			numSpaces++;
		else
			{
			if (numSpaces && *datasrc != 0x0a && *datasrc != 0x0d)					// Spaces on hold and not end of line
				while (numSpaces--)
					*(datadst++) = ' ';
			numSpaces = 0;
			*(datadst++) = *datasrc;
			}
		datasrc++;
		}
	while (numSpaces--)
		*(datadst++) = ' ';
	if (Bit16u newsize = datadst - data)											// If data
		{
		if (rawdata.capacity() < 100000)											// Prevent repetive size allocations
			rawdata.reserve(100000);
		rawdata.append((char *)data, newsize);
		if (printTimeout)
			timeOutAt = GetTickCount()+LPT_LONGTIMEOUT;								// Long timeout so data is printed w/o Close()
		}
	return true;
	}

void device_PRT::Close()
	{
	rawdata.erase(rawdata.find_last_not_of(" \n\r\t")+1);							// Remove trailing white space
	if (!rawdata.size())															// Nothing captured/to do
		return;
	int len = rawdata.size();
	if (len > 2 && rawdata[len-3] == 0x0c && rawdata[len-2] == 27 && rawdata[len-1] == 64)	// <ESC>@ after last FF?
		{
		rawdata.erase(len-2, 2);
		ffWasLast = true;
		}
	if (!ffWasLast && timeOutAt && !fastCommit)										// For programs initializing the printer in a seperate module
		{
		timeOutAt = GetTickCount() + LPT_SHORTTIMEOUT;								// Short timeout if ff was not last
		return;
		}
	CommitData();
	}

void tryPCL2PDF(char * filename, bool postScript, bool openIt)
	{
	char pcl6Path[512];																// Try to start gswin32c/pcl6 from where vDos was started
	strcpy(strrchr(strcpy(pcl6Path+1, _pgmptr), '\\'), postScript ? "\\gswin32c.exe" : "\\pcl6.exe");
	if (_access(pcl6Path+1, 4))														// If not found/readable
		{
		MessageBox(vDosHwnd, "Could not find pcl6 or gswin32c to handle printjob", "vDos - Error", MB_OK|MB_ICONWARNING);
		return;
		}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	ZeroMemory(&pi, sizeof(pi));

	pcl6Path[0] ='"';																// Surround path with quotes to be sure
	strcat(pcl6Path, "\" -sDEVICE=pdfwrite -o ");
	strcat(pcl6Path, filename);
	pcl6Path[strlen(pcl6Path)-3] = 0;												// Replace .asc by .pdf
	strcat (pcl6Path, "pdf ");
	strcat(pcl6Path, filename);
	if (CreateProcess(NULL, pcl6Path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))	// Start pcl6/gswin32c.exe
		{
		WaitForSingleObject(pi.hProcess, INFINITE);									// Wait for pcl6/gswin32c to exit
		DWORD exitCode = -1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);													// Close process and thread handles
		CloseHandle(pi.hThread);
		if (exitCode != 0)
			MessageBox(vDosHwnd, "pcl6 or gswin32c could not convert printjob to PDF", "vDos - Error", MB_OK|MB_ICONWARNING);
		else if (openIt)
			{
			strcpy(pcl6Path, filename);										
			pcl6Path[strlen(pcl6Path)-3] = 0;										// Replace .asc by .pdf
			strcat(pcl6Path, "pdf");
			if (!_access(pcl6Path, 4))												// If generated PDF file found/readable
				ShellExecute(NULL, "open", pcl6Path, NULL, NULL, SW_SHOWNORMAL);	// Open/show it
			}
		}
	return;
	}

void device_PRT::CommitData()
	{
	timeOutAt = 0;
	DPexitcode = -1;
	if (DPhandle != -1)																// DOSprinter previously used
		GetExitCodeProcess((HANDLE)DPhandle, &DPexitcode);

	FILE* fh = fopen(tmpAscii, DPexitcode == STILL_ACTIVE ? "ab" : "wb");			// Append or write to ASCII file
	if (fh)
		{
		fwrite(rawdata.c_str(), rawdata.size(), 1, fh);
		fclose(fh);
		fh = fopen(tmpUnicode, DPexitcode == STILL_ACTIVE ? "a+b" : "w+b");			// The same for Unicode file (it's eventually read)
		if (fh)
			{
			if (DPexitcode != STILL_ACTIVE)
				fprintf(fh, "\xff\xfe");											// It's a Unicode text file
			for (Bit32u i = 0; i < rawdata.size(); i++)
				{
				Bit16u textChar =  (Bit8u)rawdata[i];
				switch (textChar)
					{
				case 9: // Tab
				case 12: // Formfeed
					fwrite(&textChar, 1, 2, fh);
					break;
				case 10: // Linefeed (combination)
				case 13:
					fwrite("\x0d\x00\x0a\x00", 1, 4, fh);
					if (i < rawdata.size() -1 && textChar == 23-rawdata[i+1])
						i++;
					break;
				default:
					if (textChar >= 32) // Forget about further control characters?
						fwrite(cpMap+textChar, 1, 2, fh);
					break;
					}
				}
			}
		}
	if (!fh)
		{
		rawdata.clear();
		MessageBox(NULL, "Could not save printerdata", "vDos - Warning", MB_OK|MB_ICONSTOP);
		return;
		}
	if (!stricmp(destination.c_str(), "clipboard"))									// Copy to clipboard, Unicode file handle is still open
		{
		if (OpenClipboard(NULL))
			{
			if (EmptyClipboard())
				{
				int bytes = ftell(fh);
				HGLOBAL hCbData = GlobalAlloc(NULL, bytes);
				Bit8u* pChData = (Bit8u*)GlobalLock(hCbData);
				if (pChData)
					{
					fseek(fh, 2, SEEK_SET);											// Skip Unicode signature
					fread(pChData, 1, bytes-2, fh);
					pChData[bytes-2] = 0;
					pChData[bytes-1] = 0;
					SetClipboardData(CF_UNICODETEXT, hCbData);
					GlobalUnlock(hCbData);
					}
				}
			CloseClipboard();
			}
		fclose(fh);
		rawdata.clear();
		return;
		}

	fclose(fh);																		// No longer needed
	if (useDP)
		{
		if (nothingSet)																// DP was assumed, nothing set
			{
			if (!rawdata.find("\x1b%-12345X@") || !rawdata.find("\x1b\x45"))		// It's PCL (rawdata isn't empty at this point, so test is ok)
				{																	// Postscript can be embedded (some WP drivers)
				tryPCL2PDF(tmpAscii, rawdata.find("\n%!PS") < MIN(rawdata.length(), 60), true);	// A line should start with the signature in the first 70s characters or so
				rawdata.clear();
				return;
				}
			if (rawdata.find("%!PS") == 0)											// It's Postscript
				{
				tryPCL2PDF(tmpAscii, true, true);
				rawdata.clear();
				return;
				}
			}
		if (DPexitcode != STILL_ACTIVE)												// If DOSprinter isn't still running
			{
			char dpPath[256];														// Try to start it from where vDos was started
			strcpy(strrchr(strcpy(dpPath, _pgmptr), '\\'), "\\DOSPrinter.exe");
			DPhandle = _spawnl(P_NOWAIT, dpPath, "DOSPrinter.exe", destination.c_str(), NULL);
			if (DPhandle == -1)
				MessageBox(vDosHwnd, "Could not start DOSPrinter to handle printjob", "vDos - Error", MB_OK|MB_ICONWARNING);
			}
		}
	else if (stricmp(destination.c_str(), "dummy"))									// Windows command or program assumed
		{
		if (rawdata.find("\x1b%-12345X@") == 0)										// It's PCL (rawdata isn't empty at this point, so test is ok)
			tryPCL2PDF(tmpAscii, rawdata.find("\n%!PS") < MIN(rawdata.length(), 60), false);	// a line should start with the signature in the first 70s characters or so
		else if (rawdata.find("%!PS") == 0)											// It's Postscript
			tryPCL2PDF(tmpAscii, true, false);
		if (destination[0] == '@')													// If the commandline starts with '@' assume program to be started hidden							
			{
			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			ZeroMemory(&pi, sizeof(pi));

			if (!ExpandEnvironmentStrings(destination.c_str()+1, (char *)dos_copybuf, 4096))	// Replace %% Windows variables
				strcpy((char *)dos_copybuf, destination.c_str()+1);							// Should always work, just to be too sure
			if (CreateProcess(NULL, (char *)dos_copybuf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))	// Start program
				{
				CloseHandle(pi.hProcess);											// Close process and thread handles
				CloseHandle(pi.hThread);
				}
			}
		else
			system(destination.c_str());											// Let Windows decide what is meant				
		}
	rawdata.clear();																// Fall thru
	}

Bit16u device_PRT::GetInformation(void)
	{
//	return 0x80A0;
	return 0x80E0;																	// dBase IV checks for not ready
	}

static char* PD_select[] = {"/SEL", "/PDF", "/RTF"};
static char DP_lCode[] = "  ";

device_PRT::device_PRT(const char *pname, const char* cmd)
	{
// pname: LPT1-LPT9, COM1-COM9
// cmd:
//  1.  Not set or empty						: DOSPrinter is assumed with "/SEL /LINES /CPIA /LEFT0.50 /TOP0.50 /Lngxx" switches.
//												  If the data is recognized being PCL or Postscript, pcl6/gswin32c is started.
//	2.	/SEL, /PDF or /RTF...					: DOSPrinter is called with these switches (and /Lngxx if not included).
//  3.	clipboard								: Data is put on the clipboard.
//	4.	dummy									: Data is discarded, output in #LPT1-9/#COM1-9 is in ASCII.
//	5.	<Windows command/program> [options]		: Fallthru, cCommand/program [options] is started.
	SetName(pname);

	strcat(strcat(strcpy(tmpAscii, "#"), pname), ".asc");							// Save ASCII data to #LPTx/#COMx.asc (NB LPTx/COMx. cannot be used)
	strcat(strcat(strcpy(tmpUnicode, "#"), pname), ".txt");							// Save Unicode data to #LPTx/#COMx.asc (NB LPTx/COMx. cannot be used)
	DPhandle = -1;

	if (wpVersion && pname[3] == '9' || !stricmp("clip", cmd))						// LPT9/COM9 in combination with WP or "clip"
		{
		destination = "clipboard";
		fastCommit = true;
		}
	else
		{
		destination = cmd;
		fastCommit = false;
		}

	nothingSet = false;
	if (destination.empty())														// Not defined or invalid setup, use DOSPrinter with standard switches
		{
		destination = "/SEL /LINES /CPIA /LEFT0.50 /TOP0.50";
		useDP = true;
		nothingSet = true;
		}
	else 
		{
		useDP = false;																// Test if set for using DOSPrinter with switches
		for (int i = 0; i < 3; i++)
			if (!strnicmp(destination.c_str(), PD_select[i], 4))
				useDP = true;
		}
	if (useDP)
		{
		char *upperDest = new char[destination.size() + 1];
		for (unsigned int i = 0; i < destination.size(); i++)
			upperDest[i] = toupper(destination[i]);
		upperDest[destination.size()] = '\0';
		if (!strstr(upperDest, "/LNG"))												// If language not set in switches
			{
			if (DP_lCode[0] == ' ')
				{
				int langID = GetSystemDefaultLangID()&0x1ff;						// Determine UI language for DOSPrinter
				int suppID[] = {0x16, 0x0a, 0x0c, 0x1a, 0x1b, 0x24, 0x0e, 0x10, 0x03, 0x13, 0x07, 0x00};
				char *suppLN[] = {"PT", "ES", "FR", "HR", "SI", "SI", "HU", "IT", "CA", "NL", "DE"};
				DP_lCode[0] = 'x';
				for (int i = 0; suppID[i] != 0; i++)								// LCIDToLocaleName not supported in Win XP
					if (langID == suppID[i])										// So we do it "by hand"
						strcpy(DP_lCode, suppLN[i]);
				}
			if (DP_lCode[0] != 'x')
				{
				destination += " /lng";
				destination += DP_lCode;
				}
			}
		destination += " ";
		destination += tmpAscii;
		delete upperDest;
		}
	}

device_PRT::~device_PRT()
	{
	}
