#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "vDos.h"
#include "bios.h"
#include "mem.h"
#include "regs.h"
#include "dos_inc.h"
#include "Shlwapi.h"

DOS_File ** Files;
DOS_Drive * Drives[DOS_DRIVES];

bool NormalizePath(char * path)														// Strips names to max 8 and extensions to max 3 positions
	{
	if (strpbrk(path, "+[] "))														// Returns false if forbidden chars
		return false;

	// no checking for all invalid sequences, just strip to 8.3 and return into passed string (result will never be longer)
	char * output = path;
	bool inExt = false;
	int count = 0;
	while (char c = *path++)
		{
		if (c == '\\' || c == '/')
			{
			count = 0;
			inExt = false;
			*output++ = '\\';
			}
		else if (inExt)
			{
			if ((!count && c == '.') || (count++ < 3 && c != '.'))					// Misses nnn.e.e, what to do with that?
				*output++ = c;
			}
		else if (c == '.')
			{
			count = 0;
			inExt = true;
			*output++ = c;
			}
		else if (count++ < 8)
			*output++ = c;
		}
	*output = 0;
	return true;
	}

Bit8u DOS_GetDefaultDrive(void)
	{
	return dos.current_drive;
	}

void DOS_SetDefaultDrive(Bit8u drive)
	{
	if (drive <= DOS_DRIVES && Drives[drive])
		{
		DOS_SetError(DOSERR_NONE);
		dos.current_drive = drive;
		DOS_SDA(DOS_SDA_SEG, DOS_SDA_OFS).SetDrive(drive);
		}
	else
		DOS_SetError(DOSERR_INVALID_DRIVE);

	}

#define maxCached 16
static char makename_out[DOS_PATHLENGTH * 2];
static char makename_in[DOS_PATHLENGTH * 2];
static char prevname_in[maxCached][DOS_PATHLENGTH * 2];
static char prevname_out[maxCached][DOS_PATHLENGTH * 2];
static int cachedIdx = 0;

bool DOS_MakeName(char const* const name, char* fullname, Bit8u* drive) // Nb, returns path w/o leading '\\'!
{
	if(!name || *name == 0 || *name == ' ') { // Both \0 and space are seperators and empty filenames report file not found
		DOS_SetError(DOSERR_FILE_NOT_FOUND);
		return false;
	}
	char const* name_ptr = name;
	if (name_ptr[1] == ':') { // First get the drive
		*drive = (*name_ptr | 0x20)-'a';
		name_ptr += 2;
	}
	else
		*drive = DOS_GetDefaultDrive();
	if(*drive >= DOS_DRIVES || !Drives[*drive] || strlen(name_ptr) >= DOS_PATHLENGTH) {
		vLog("Invalid path reference: %s", name);
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false; 
	}
	char * oPtr = makename_in;
	if (*name_ptr != '\\' && *name_ptr != '/' && *Drives[*drive]->curdir)
		oPtr += strlen(strcat(strcat(strcpy(oPtr, "\\"), Drives[*drive]->curdir), "\\"));
	char const * iPtr = name_ptr;
	for (int sRem = 0; *iPtr; iPtr++) { // Strip bare filenames and extensions from trailing spaces
		if (*iPtr == ' ')
			sRem++;
		else {
			if (*iPtr != '.' && *iPtr != '\\')
				for (; sRem; sRem--)
					*oPtr++ = ' ';
			else
				sRem = 0;
			*oPtr++ = *iPtr;
			}
		}
	*oPtr = 0;
	for (int i = 0; i < maxCached; i++) // Don't go into the routines below if already done before
		if (!stricmp(makename_in, prevname_in[i])) {
			strcpy(fullname, prevname_out[i] + (*prevname_out[i] =='\\' ? 1 : 0));	// Leading '\\' dropped again
			return true;
		}
	if (!(PathCanonicalize(makename_out, makename_in) && NormalizePath(makename_out))) { 
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false; 
		}
	cachedIdx = cachedIdx%maxCached; // Save the new to be checked file entry
	strcpy(prevname_in[cachedIdx], makename_in);
	strcpy(prevname_out[cachedIdx], makename_out);
	cachedIdx++;
	strcpy(fullname, makename_out + (*makename_out =='\\' ? 1 : 0)); // Leading '\\' dropped again
	return true;	
	}

bool DOS_GetCurrentDir(Bit8u drive, char * const buffer)
{
	if (!drive)
		drive = DOS_GetDefaultDrive();
	else
		{
		drive--;
		if (drive >= DOS_DRIVES || !Drives[drive])
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		}
	strcpy(buffer, Drives[drive]->curdir);
	DOS_SetError(DOSERR_NONE);
	return true;
	}

bool DOS_ChangeDir(char const* const dir)
	{
	Bit8u drive;
	const char *testdir = dir;
	if (*testdir && testdir[1] == ':')
		{
		drive = (*dir|0x20)-'a';
		if (drive >= DOS_DRIVES || !Drives[drive])
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		testdir += 2;
		}
	size_t len = strlen(testdir);
	char fulldir[DOS_PATHLENGTH];
	if (!len || !DOS_MakeName(dir, fulldir, &drive) || (*fulldir && testdir[len-1] == '\\'))
		{ }
	else if (Drives[drive]->TestDir(fulldir))
		{
		for (int i = 0; fulldir[i]; i++)											// Names in MS-DOS are allways uppercase
			fulldir[i] = toupper(fulldir[i]);
		strcpy(Drives[drive]->curdir, fulldir);
		DOS_SetError(DOSERR_NONE);
		return true;
		}
	DOS_SetError(DOSERR_PATH_NOT_FOUND);
	return false;
	}

bool DOS_MakeDir(char const* const dir)
	{
	Bit8u drive;
	const char *testdir = dir;
	if (*testdir && testdir[1] == ':')
		{
		drive = (*testdir | 0x20)-'a';
		if (drive >= DOS_DRIVES || !Drives[drive])
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		testdir += 2;
		}
	size_t len = strlen(testdir);
	char fulldir[DOS_PATHLENGTH];
	if (!len || !DOS_MakeName(dir, fulldir, &drive) || (*fulldir && testdir[len-1] == '\\'))
		{
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false;
		}
	return Drives[drive]->MakeDir(fulldir);
	}

bool DOS_RemoveDir(char const * const dir)
	{
	// We need to do the test before the removal we as can not rely on
	// Windows to forbid removal of the current directory.
	Bit8u drive;
	const char *testdir = dir;
	if (*testdir && testdir[1] == ':')
		{
		drive = (*testdir | 0x20)-'a';
		if (drive >= DOS_DRIVES || !Drives[drive])
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		testdir += 2;
		}
	size_t len = strlen(testdir);
	char fulldir[DOS_PATHLENGTH];
	if (!len || !DOS_MakeName(dir, fulldir, &drive) || (*fulldir && testdir[len-1] == '\\'))
		{
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false;
		}
	if (!strcmp(Drives[drive]->curdir, fulldir))									// Test if it's current directory
		{
		DOS_SetError(DOSERR_REMOVE_CURRENT_DIRECTORY);
		return false;
		}
	return Drives[drive]->RemoveDir(fulldir);
	}

bool DOS_Rename(char const * const oldname, char const * const newname)
	{
	Bit8u driveold;
	char fullold[DOS_PATHLENGTH];
	if (!DOS_MakeName(oldname, fullold, &driveold))
		{
		DOS_SetError(DOSERR_FILE_NOT_FOUND);
		return false;
		}
	Bit8u drivenew;
	char fullnew[DOS_PATHLENGTH];
	if (!DOS_MakeName(newname, fullnew, &drivenew))
		return false;
	if ((DOS_FindDevice(oldname) != DOS_DEVICES) || (DOS_FindDevice(newname) != DOS_DEVICES))	// No tricks with devices
		{
		DOS_SetError(DOSERR_FILE_NOT_FOUND);
		return false;
		}
	if (driveold != drivenew)														// Must be on the same drive
		{
		DOS_SetError(DOSERR_NOT_SAME_DEVICE);
		return false;
		}
	return Drives[drivenew]->Rename(fullold, fullnew);
	}

bool DOS_FindFirst(char * search, Bit16u attr)
	{
	DOS_DTA dta(dos.dta());
	size_t len = strlen(search);
	if (!len || (len == 2 && search[1] == ':')
		|| (len && search[len-1] == '\\' && !((len > 2) && (search[len-2] == ':') && (attr == DOS_ATTR_VOLUME))))
		{ 
		DOS_SetError(DOSERR_NO_MORE_FILES);
		return false;
		}
	char fullsearch[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(search, fullsearch, &drive))
		return false;
	char dir[DOS_PATHLENGTH];														// Split the search in dir and pattern
	char pattern[DOS_PATHLENGTH];
	char * find_last = strrchr(fullsearch, '\\');
	if (!find_last)																	// No dir
		{
		strcpy(pattern, fullsearch);
		dir[0] = 0;
		}
	else
		{
		*find_last = 0;
		strcpy(pattern, find_last+1);
		strcpy(dir, fullsearch);
		}
	dta.SetupSearch(drive, (Bit8u)attr, pattern);
	if (bool device = (DOS_FindDevice(search) != DOS_DEVICES))						// Check for devices. FindDevice checks for leading subdir as well
		{
		if (!(attr&DOS_ATTR_DEVICE))
			return false;
		if (find_last = strrchr(pattern, '.'))
			*find_last = 0;
		dta.SetResult(pattern, 0, 0, 0, DOS_ATTR_DEVICE);							// TODO use current date and time
		return true;
		}
	 return Drives[drive]->FindFirst(dir, dta);
	}

bool DOS_FindNext(void)
	{
	return DoFindNext();
	}

static Bit8u GetHandle(Bit16u entry)
	{
	Bit8u handle = RealHandle(entry);
	if (handle == DOS_FILES || !Files[handle])
		{
		DOS_SetError(DOSERR_INVALID_HANDLE);
		return DOS_FILES;
		}
	return handle;
	}

bool DOS_ReadFile(Bit16u entry, Bit8u* data, Bit16u* amount)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	return Files[handle]->Read(data, amount);
	}

bool DOS_WriteFile(Bit16u entry, Bit8u* data, Bit16u* amount)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	return Files[handle]->Write(data, amount);
	}

bool DOS_SeekFile(Bit16u entry, Bit32u* pos, Bit32u type)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	return Files[handle]->Seek(pos, type);
	}

bool DOS_LockFile(Bit16u entry, Bit8u mode, Bit32u pos, Bit32u size)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	return Files[handle]->LockFile(mode, pos, size);
	}

bool DOS_CloseFile(Bit16u entry)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	Files[handle]->Close();
	DOS_PSP psp(dos.psp());
	psp.SetFileHandle(entry, 0xff);

	if (Files[handle]->RemoveRef() <= 0)
		{
		delete Files[handle];
		Files[handle] = 0;
		}
	return true;
	}

bool DOS_FlushFile(Bit16u entry)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	return true;
	}

static bool PathExists(char const* const name)
	{
	char temp[MAX_PATH_LEN];
	char* lead = strrchr(strcpy(temp, name), '\\');
	if (!lead || lead == temp)
		return true;
	*lead = 0;
	Bit8u drive;
	char fulldir[DOS_PATHLENGTH];
	if (!DOS_MakeName(temp, fulldir, &drive) || !Drives[drive]->TestDir(fulldir))
		return false;
	return true;
	}

bool DOS_CreateFile(char const* name, Bit16u attributes, Bit16u* entry)
	{
	if (DOS_FindDevice(name) != DOS_DEVICES)										// Creating a device is the same as opening it
		return DOS_OpenFile(name, OPEN_READWRITE, entry);
	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))
		return false;
	Bit8u fh;
	for (fh = 0; fh < DOS_FILES; fh++)												// Check for a free file handle
		if (!Files[fh])
			break;
	if (fh == DOS_FILES)
		{
		DOS_SetError(DOSERR_TOO_MANY_OPEN_FILES);
		return false;
		}
	DOS_PSP psp(dos.psp());															// We have a position in the main table, now find one in the psp table
	*entry = psp.FindFreeFileEntry();
	if (*entry == 0xff)
		{
		DOS_SetError(DOSERR_TOO_MANY_OPEN_FILES);
		return false;
		}
	if (attributes&DOS_ATTR_DIRECTORY)												// Don't allow directories to be created
		{
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
		}
	if (Drives[drive]->FileCreate(&Files[fh], fullname, attributes))
		{ 
		Files[fh]->SetDrive(drive);
		Files[fh]->AddRef();
		psp.SetFileHandle(*entry, fh);
		return true;
		}
	return false;
	}

static int wpOpenDevCount = 0;														// For WP - write file to clipboard, we have to return File not found once and twice normal
																					// WP opens the file (device) to check if it exists - creates/closes - opens/writes/closes - opens/seeks/closes
static DWORD wpOpenDevTime = 0;

bool DOS_OpenFile(char const* name, Bit8u flags, Bit16u* entry)
	{
	Bit8u devnum = DOS_FindDevice(name);											// First check for devices
	bool device = (devnum != DOS_DEVICES);
	if (device && wpVersion)														// WP - clipboard
		{
		if (GetTickCount() > wpOpenDevTime + 1000)									// Recalibrate after some (1/2 sec) time
			{
			wpOpenDevCount = 0;
			wpOpenDevTime = GetTickCount();
			}
		if ((devnum == 8 || devnum == 17) && !(++wpOpenDevCount&2))					// LPT9/COM9 (1-4 rejected by WP, 9 reserved for Edward Mendelson's macros)
			{
			DOS_SetError(PathExists(name) ? DOSERR_FILE_NOT_FOUND : DOSERR_PATH_NOT_FOUND);
			return false;
			}
		}

	Bit16u attr;
	if (!device && DOS_GetFileAttr(name, &attr))									// DON'T allow directories to be openened.(skip test if file is device).
		if ((attr&DOS_ATTR_DIRECTORY) || (attr&DOS_ATTR_VOLUME))
			{
			DOS_SetError(DOSERR_ACCESS_DENIED);
			return false;
			}

	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))										// Check if the name is correct
		return false;
	Bit8u handle;
	for (handle = 0; handle < DOS_FILES; handle++)									// Check for a free file handle
		if (!Files[handle])
			break;
	if (handle == DOS_FILES)
		{
		DOS_SetError(DOSERR_TOO_MANY_OPEN_FILES);
		return false;
		}
	DOS_PSP psp(dos.psp());															// We have a position in the main table, now find one in the psp table
	*entry = psp.FindFreeFileEntry();
	if (*entry == 0xff)
		{
		DOS_SetError(DOSERR_TOO_MANY_OPEN_FILES);
		return false;
		}
	bool exists = false;
	if (device)
		Files[handle] = new DOS_Device(*Devices[devnum]);
	else if (exists = Drives[drive]->FileOpen(&Files[handle], fullname, flags))
		Files[handle]->SetDrive(drive);
	if (exists || device)
		{
		Files[handle]->AddRef();
		psp.SetFileHandle(*entry, handle);
		return true;
		}
	return false;
	}

bool DOS_OpenFileExtended(char const* name, Bit16u flags, Bit16u createAttr, Bit16u action, Bit16u* entry, Bit16u* status)
	{
	// FIXME: Not yet supported : Bit 13 of flags (int 0x24 on critical error)
	Bit16u result = 0;
	if (action == 0 || action&0xffec)
		{
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
		}
	if (DOS_OpenFile(name, (Bit8u)(flags&0xff), entry))								// File already exists
		{
		switch (action & 0x0f)
			{
		case 0:																		// Failed
			DOS_CloseFile(*entry);
			DOS_SetError(DOSERR_FILE_ALREADY_EXISTS);
			return false;
		case 1:																		// File open (already done)
			result = 1;
			break;
		case 2:																		// Replace
			DOS_CloseFile(*entry);
			if (!DOS_CreateFile(name, createAttr, entry))
				return false;
			result = 3;
			break;
			}
		}
	else																			// File doesn't exist
		{
		if ((action&0xf0) == 0)
			return false;															// Uses error code from failed open
		if (!DOS_CreateFile(name, createAttr, entry))								// Create File
			return false;															// Uses error code from failed create
		result = 2;
		}
	*status = result;																// Success
	return true;
	}

bool DOS_DeleteFile(char const* const name)
	{
	if ((DOS_FindDevice(name) != DOS_DEVICES))
		return true;
	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))
		return false;
	return Drives[drive]->FileUnlink(fullname);
	}

bool DOS_GetFileAttr(char const* const name, Bit16u* attr)
	{
	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))
		return false;
	return Drives[drive]->GetFileAttr(fullname, attr);
	}

bool DOS_SetFileAttr(char const* const name, Bit16u /*attr*/) 
	// This function does not change the file attributs
	// It just tests if file is available 
	{
	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))
		return false;	
	Bit16u attrTemp;
	return Drives[drive]->GetFileAttr(fullname, &attrTemp);
	}

bool DOS_Canonicalize(char const* const name, char* const big)
	{
	// TODO Add Better support for devices and shit but will it be needed i doubt it :) 
	if (!DOS_MakeName(name, &big[3], (Bit8u *)big))
		return false;
	big[0] += 'A';
	big[1] = ':';
	big[2] = '\\';
	return true;
	}

bool DOS_GetFreeDiskSpace(Bit8u drive, Bit16u* bytes_sector, Bit8u* sectors_cluster, Bit16u* total_clusters, Bit16u* free_clusters)
	{
	DOS_SetError(DOSERR_NONE);
	if (!drive)
		drive = DOS_GetDefaultDrive();
	else
		{
		drive--;
		if (drive >= DOS_DRIVES || !Drives[drive])
			{
			DOS_SetError(DOSERR_INVALID_DRIVE);
			return false;
			}
		}
	*bytes_sector = 512;															// 512*8*32767 == ~131 MB total size, *30720  == ~125 MB free size
	*sectors_cluster = 8;
	*total_clusters = 32767;														// 16 bit, to be save highest bit = 0
	*free_clusters = 30720;
	return true;
	}

bool DOS_GetAllocationInfo(Bit8u drive, Bit16u* bytes_sector, Bit8u* sectors_cluster, Bit16u* total_clusters)
	{
	Bit16u free_clusters;
	if (DOS_GetFreeDiskSpace(drive, bytes_sector, sectors_cluster, total_clusters, &free_clusters))
		{
		SegSet16(ds,RealSeg(dos.tables.mediaid));
		reg_bx = RealOff(dos.tables.mediaid+drive*2);
		return true;
		}
	return false;
	}

bool DOS_DuplicateEntry(Bit16u entry, Bit16u* newentry)
	{
	// Dont duplicate console handles
/*	if (entry<=STDPRN) {
		*newentry = entry;
		return true;
	};
*/	
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	DOS_PSP psp(dos.psp());
	*newentry = psp.FindFreeFileEntry();
	if (*newentry == 0xff)
		{
		DOS_SetError(DOSERR_TOO_MANY_OPEN_FILES);
		return false;
		}
	Files[handle]->AddRef();	
	psp.SetFileHandle(*newentry, handle);
	return true;
	}

bool DOS_ForceDuplicateEntry(Bit16u entry, Bit16u newentry)
	{
	if (entry == newentry)
		{
		DOS_SetError(DOSERR_INVALID_HANDLE);
		return false;
		}
	Bit8u orig = GetHandle(entry);
	if (orig == DOS_FILES)
		return false;
	Bit8u newone = RealHandle(newentry);
	if (newone < DOS_FILES && Files[newone])
		DOS_CloseFile(newentry);
	DOS_PSP psp(dos.psp());
	Files[orig]->AddRef();
	psp.SetFileHandle(newentry, orig);
	return true;
	}

bool DOS_CreateTempFile(char* const name, Bit16u* entry)							// Check to see if indeed 13 0 bytes at end to receive temp filename?
	{
	size_t namelen = strlen(name);
	char * tempname = name+namelen;
	if (namelen == 0 || name[namelen-1] != '\\')									// No ending '\' append it
		*tempname++ = '\\';
	tempname[8] = 0;																// No extension
	do																				// Add random crap to the end of the path and try to create
		{
		dos.errorcode = 0;
		for (int i = 0; i < 8; i++)
			tempname[i] = (rand()%26)+'A';
		}
	while ((!DOS_CreateFile(name, 0, entry)) && (dos.errorcode == DOSERR_FILE_ALREADY_EXISTS));
	return dos.errorcode == 0;
	}

static bool isvalid(const char in)
	{
	return (Bit8u(in) > 0x1F) && (!strchr(":.;,=+ \t/\"[]<>|", in));
	}

bool DOS_FileExists(char const* const name)
	{
	char fullname[DOS_PATHLENGTH];
	Bit8u drive;
	if (!DOS_MakeName(name, fullname, &drive))
		return false;
	return Drives[drive]->FileExists(fullname);
	}

bool DOS_GetFileDate(Bit16u entry, Bit16u* otime, Bit16u* odate)
	{
	Bit8u handle = GetHandle(entry);
	if (handle == DOS_FILES)
		return false;
	Files[handle]->UpdateDateTimeFromHost();
	*otime = Files[handle]->time;
	*odate = Files[handle]->date;
	return true;
	}

void DOS_SetupFiles(void)
	{
	Files = new DOS_File* [DOS_FILES];												// Setup the File Handles
	for (int i = 0; i < DOS_FILES; i++)
		Files[i] = 0;
	for (int i = 0; i < DOS_DRIVES; i++)											// Setup the Virtual Disk System
		Drives[i] = 0;
	}
