#include <slib.h>
#include "vDos.h"
#include "dos_system.h"
#include "Shlwapi.h"
#include "dos_inc.h"
#include "support.h"
#include <sys/stat.h>
#include <time.h>
#include <direct.h>
#include <fcntl.h>
#include <stdlib.h>

bool WildFileCmp(const char * file, const char * wild)
{
	char file_name[9];
	char file_ext[4];
	char wild_name[9];
	char wild_ext[4];
	const char * find_ext;
	Bitu r;

	strcpy(wild_name, strcpy(file_name, "        "));
	strcpy(wild_ext, strcpy(file_ext, "   "));

	find_ext = strrchr(file, '.');
	if(find_ext) {
		Bitu size = (Bitu)(find_ext-file);
		if(size > 8)
			size = 8;
		memcpy(file_name, file, size);
		find_ext++;
		memcpy(file_ext, find_ext, (strlen(find_ext)>3) ? 3 : strlen(find_ext));
	}
	else
		memcpy(file_name, file, (strlen(file) > 8) ? 8 : strlen(file));
	upcase(file_name);
	upcase(file_ext);
	find_ext = strrchr(wild, '.');
	if(find_ext) {
		Bitu size = (Bitu)(find_ext-wild);
		if(size > 8)
			size = 8;
		memcpy(wild_name, wild, size);
		find_ext++;
		memcpy(wild_ext, find_ext, (strlen(find_ext)>3) ? 3 : strlen(find_ext));
	}
	else
		memcpy(wild_name, wild, (strlen(wild) > 8) ? 8 : strlen(wild));
	upcase(wild_name);
	upcase(wild_ext);
	// Names are right do some checking
	r = 0;
	while(r <8) {
		if(wild_name[r] == '*')
			break;
		if(wild_name[r] != '?' && wild_name[r] != file_name[r])
			return false;
		r++;
	}
	r = 0;
	while(r < 3) {
		if(wild_ext[r] == '*')
			return true;
		if(wild_ext[r] != '?' && wild_ext[r] != file_ext[r])
			return false;
		r++;
	}
	return true;
}

DOS_Drive::DOS_Drive(const char* startdir, Bit8u driveNo)
{
	curdir[0] = 0;
	SetBaseDir(startdir);
	label[0] = driveNo+'A';
	strcpy(label+1, "_DRIVE");
	while(Bit8u c = *startdir++)
		VolSerial = c + (VolSerial << 6) + (VolSerial << 16) - VolSerial;
	Mem_Stosb(dWord2Ptr(dos.tables.mediaid)+driveNo*2, 0xF8); // Set the correct media byte in the table (harddisk)
}

void DOS_Drive::SetBaseDir(const char* startdir)
{
	strcpy(basedir, startdir);
	if(basedir[strlen(basedir)-1] != '\\')
		strcat(basedir, "\\");
	remote = true;
	if(startdir[0] != '\\') { // Assume \\... is remote
		char rootDir[] = " :\\";
		rootDir[0] = startdir[0];
		if(!(GetDriveType(rootDir)&DRIVE_REMOTE))
			remote = false;
	}
}

bool DOS_Drive::FileCreate(DOS_File ** file, char * name, Bit16u attr)
{
	char win_name[MAX_PATH_LEN];
	int attribs = FILE_ATTRIBUTE_NORMAL;
	if(attr&7) // Read-only (1), Hidden (2), System (4) are the same in DOS and Windows
		attribs = FILE_ATTRIBUTE_READONLY;
	strcat(strcpy(win_name, basedir), name);
	HANDLE handle = CreateFile(win_name,
	    GENERIC_READ|GENERIC_WRITE,
	    FILE_SHARE_READ|FILE_SHARE_WRITE,
	    NULL,
	    CREATE_ALWAYS,
	    attribs,
	    NULL);
	if(handle == INVALID_HANDLE_VALUE) {
		Bit32u errorno = GetLastError();
		DOS_SetError((Bit16u)errorno);
		vLog("File creation failed: %s\nErrorno: %d", win_name, errorno);
		return false;
	}
	*file = new Disk_File(name, handle);
	(*file)->flags = OPEN_READWRITE;
	return true;
}

bool DOS_Drive::FileOpen(DOS_File ** file, char * name, Bit32u flags)
{
	int ohFlag, shhFlag;

	switch(flags&0xf)
	{
		case OPEN_READ:
		    ohFlag = GENERIC_READ;
		    break;
		case OPEN_WRITE:
		    ohFlag = GENERIC_WRITE;
		    break;
		case OPEN_READWRITE:
		    ohFlag = GENERIC_READ|GENERIC_WRITE;
		    break;
		default:
		    DOS_SetError(DOSERR_ACCESS_CODE_INVALID);
		    return false;
	}
	switch(flags&0x70)
	{
		case 0x10:
		    shhFlag = 0;
		    break;
		case 0x20:
		    shhFlag = FILE_SHARE_READ;
		    break;
		case 0x30:
		    shhFlag = FILE_SHARE_WRITE;
		    break;
		default:
		    shhFlag = FILE_SHARE_READ|FILE_SHARE_WRITE;
	}
	char win_name[MAX_PATH_LEN];
	int len = strlen(name);
	if(!stricmp(name, "AUTOEXEC.BAT")) // Redirect it to vDos' autoexec.txt
		strcpy(win_name, "autoexec.txt");
	else if(!stricmp(name, "4DOS.HLP") || (len > 8 && !strnicmp(name+len-9, "\\4DOS.HLP", 9))) // Redirect it to that in vDos' folder
		strcpy(strrchr(strcpy(win_name, _pgmptr), '\\'), "\\4DOS.HLP");
	else
		strcat(strcpy(win_name, basedir), name);
	HANDLE handle = CreateFile(win_name, ohFlag, shhFlag, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(handle == INVALID_HANDLE_VALUE) {
		DOS_SetError((Bit16u)GetLastError());
		return false;
	}
	*file = new Disk_File(name, handle);
	(*file)->flags = flags; // For the inheritance flag and maybe check for others.
	return true;
}

bool DOS_Drive::GetFileAttr(char* name, Bit16u* attr)
{
	char win_name[MAX_PATH_LEN];
	strcat(strcpy(win_name, basedir), name);
	Bit32u attribs = GetFileAttributes(win_name);
	if(attribs == INVALID_FILE_ATTRIBUTES) {
		DOS_SetError((Bit16u)GetLastError());
		return false;
	}
	*attr = (Bit16u)(attribs&0x3f); // Only return lower 6 bits
	return true;
}

bool DOS_Drive::FileExists(const char* name)
{
	char win_name[MAX_PATH_LEN];
	Bit32u attribs = GetFileAttributes(strcat(strcpy(win_name, basedir), name));
	return (attribs != INVALID_FILE_ATTRIBUTES && !(attribs&FILE_ATTRIBUTE_DIRECTORY));
}

bool DOS_Drive::FileUnlink(char * name)
{
	char win_name[MAX_PATH_LEN];
	if(DeleteFile(strcat(strcpy(win_name, basedir), name)))
		return true;
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

#define maxWinSearches 256                                                                                                                      //
	                                                                                                                                        // Should
	                                                                                                                                        // be
	                                                                                                                                        // ennough!
static struct {                                                                                                                                         //
	                                                                                                                                                // To
	                                                                                                                                                // keep
	                                                                                                                                                // track
	                                                                                                                                                // of
	                                                                                                                                                // open
	                                                                                                                                                // Window
	                                                                                                                                                // searches
	PhysPt dta_pt;                                                                                                                                  //
	                                                                                                                                                // DTA
	                                                                                                                                                // address
	Bit16u psp;                                                                                                                                     //
	                                                                                                                                                // PSP
	                                                                                                                                                // segment
	                                                                                                                                                // of
	                                                                                                                                                // program
	                                                                                                                                                // initiating
	                                                                                                                                                // search
	HANDLE sHandle;                                                                                                                                 //
	                                                                                                                                                // Search
	                                                                                                                                                // handle
	bool root;                                                                                                                                      //
	                                                                                                                                                // Search
	                                                                                                                                                // is
	                                                                                                                                                // at
	                                                                                                                                                // root
	                                                                                                                                                // DOS
	                                                                                                                                                // drive
	                                                                                                                                                // (no
	                                                                                                                                                // "."
	                                                                                                                                                // or
	                                                                                                                                                // "..")
	bool wildcard;                                                                                                                                  //
	                                                                                                                                                // Search
	                                                                                                                                                // has
	                                                                                                                                                // wildcard
} WinSearches[maxWinSearches];

static int openWinSearches = 0;                                                                                                         //
                                                                                                                                        // Number
                                                                                                                                        // of
                                                                                                                                        // active
                                                                                                                                        // Windows
                                                                                                                                        // serach
                                                                                                                                        // handles
static char srch_dir[MAX_PATH_LEN];                                                                                                     //
                                                                                                                                        // Windows
                                                                                                                                        // search
                                                                                                                                        // directory

static void closeWinSeach(DOS_DTA &dta)
{
	PhysPt dta_Addr = dta.GetPt();
	for(int i = 0; i < openWinSearches; i++)
		if(WinSearches[i].dta_pt == dta_Addr) {                                                                         //
		                                                                                                                // Match
		                                                                                                                // found
			if(WinSearches[i].sHandle != INVALID_HANDLE_VALUE)                                              //
				                                                                                        // If
				                                                                                        // search
				                                                                                        // started,
				                                                                                        // close
				                                                                                        // handle
				FindClose(WinSearches[i].sHandle);
			openWinSearches--;
			if(i < openWinSearches)                                                                                                 //
				                                                                                                                // Compact
				                                                                                                                // WinSearches
				                                                                                                                // down
				memmove(&WinSearches[i], &WinSearches[i+1], sizeof(WinSearches[1])*(openWinSearches-i));
			return;
		}
}

void closeWinSeachByPSP(Bit16u psp)
{
	for(int i = 0; i < openWinSearches; i++)
		if(WinSearches[i].psp == psp) {                                                                                         //
		                                                                                                                        // Match
		                                                                                                                        // found
			if(WinSearches[i].sHandle != INVALID_HANDLE_VALUE)                                              //
				                                                                                        // If
				                                                                                        // serach
				                                                                                        // started,
				                                                                                        // close
				                                                                                        // handle
				FindClose(WinSearches[i].sHandle);
			openWinSearches--;
			if(i < openWinSearches)                                                                                                 //
				                                                                                                                // Compact
				                                                                                                                // WinSearches
				                                                                                                                // down
				memmove(&WinSearches[i], &WinSearches[i+1], sizeof(WinSearches[1])*(openWinSearches-i));
			return;
		}
}

bool DOS_Drive::FindFirst(char* _dir, DOS_DTA & dta)
{
	closeWinSeach(dta);                                                                                                                             //
	                                                                                                                                                // Close
	                                                                                                                                                // Windows
	                                                                                                                                                // search
	                                                                                                                                                // handle
	                                                                                                                                                // if
	                                                                                                                                                // entry
	                                                                                                                                                // for
	                                                                                                                                                // this
	                                                                                                                                                // dta
	if(!TestDir(_dir)) {
		DOS_SetError(DOSERR_PATH_NOT_FOUND);
		return false;
	}
	strcat(strcpy(srch_dir, basedir), _dir);                                                                                //
	                                                                                                                        // Should
	                                                                                                                        // be
	                                                                                                                        // on
	                                                                                                                        // a
	                                                                                                                        // per
	                                                                                                                        // program
	                                                                                                                        // base,
	                                                                                                                        // but
	                                                                                                                        // only
	                                                                                                                        // used
	                                                                                                                        // shortly
	                                                                                                                        // so
	                                                                                                                        // for
	                                                                                                                        // now
	                                                                                                                        // OK
	if(srch_dir[strlen(srch_dir)-1] != '\\')
		strcat(srch_dir, "\\");

	Bit8u sAttr;
	char srch_pattern[DOS_NAMELENGTH_ASCII];
	dta.GetSearchParams(sAttr, srch_pattern);
	if(!strcmp(srch_pattern, "        .   "))                                                                               //
		                                                                                                                // Special
		                                                                                                                // complete
		                                                                                                                // empty,
		                                                                                                                // what
		                                                                                                                // about
		                                                                                                                // abc.
		                                                                                                                // ?
		                                                                                                                // (abc.*?)
		strcat(srch_dir, "*.*");
	else {
		int j = strlen(srch_dir);
		for(int i = 0; i < DOS_NAMELENGTH_ASCII; i++)                                                           //
			                                                                                                // Pattern
			                                                                                                // had
			                                                                                                // 8.3
			                                                                                                // format
			                                                                                                // with
			                                                                                                // embedded
			                                                                                                // spaces,
			                                                                                                // for
			                                                                                                // now
			                                                                                                // simply
			                                                                                                // remove
			                                                                                                // them
			                                                                                                // (
			                                                                                                // "abc
			                                                                                                // d
			                                                                                                //
			                                                                                                //
			                                                                                                // .a
			                                                                                                // b"?)
			if(srch_pattern[i] != ' ')
				srch_dir[j++] = srch_pattern[i];
	}
	// Windows "finds" LPT1-9/COM1-9 which are never returned in a (DOS)DIR
	if(((!strnicmp(srch_pattern, "LPT",
				    3) ||
			    !strnicmp(srch_pattern, "COM", 3)) && (srch_pattern[3] > '0' && srch_pattern[3] <= '9' && srch_pattern[4] ==' '))
	    || ((!strnicmp(srch_pattern, "PRN", 3) || !strnicmp(srch_pattern, "AUX", 3)) && srch_pattern[3] ==' ')) {
		DOS_SetError(DOSERR_NO_MORE_FILES);
		return false;
	}
	DOS_SetError(DOSERR_NONE);
	if((sAttr&DOS_ATTR_VOLUME) && WildFileCmp(GetLabel(), srch_pattern)) {
		dta.SetResult(GetLabel(), 0, 0, 0, DOS_ATTR_VOLUME);
		return true;
	}
	if(!(sAttr&(DOS_ATTR_VOLUME))) {                                                                                                                                //
		                                                                                                                                                        // Remove
		int fLen = strlen(srch_dir);
		if(fLen > 11 && !stricmp("\\4HELP.EXE", srch_dir+fLen-10)) {                                    //
		                                                                                                // Global
		                                                                                                // check
		                                                                                                // for
		                                                                                                // "4help.exe"
		                                                                                                // to
		                                                                                                // start
		                                                                                                // by
		                                                                                                // 4DOS
		                                                                                                // help
		                                                                                                // function
			dta.SetResult("4HELP.EXE", 16384, 0, 0, DOS_ATTR_ARCHIVE);
			return true;
		}
		if((fLen > 11 && !stricmp("\\COMMAND.*", srch_dir+fLen-10))
		    || (fLen > 13 && !stricmp("\\COMMAND.COM", srch_dir+fLen-12))) {                    // Global check
		                                                                                        // for
		                                                                                        // "COMMAND.COM"
			dta.SetResult("COMMAND.COM", 16384, 0, 0, DOS_ATTR_ARCHIVE);
			return true;
		}
		if((fLen > 14 && !stricmp("\\AUTOEXEC.BAT", srch_dir+fLen-13))) {
			dta.SetResult("AUTOEXEC.BAT", 16384, 0, 0, DOS_ATTR_ARCHIVE);
			return true;
		}
	}
	if(openWinSearches == maxWinSearches)                                                                                   //
		                                                                                                                // We'll
		                                                                                                                // need
		                                                                                                                // a
		                                                                                                                // new
		                                                                                                                // Windows
		                                                                                                                // search
		                                                                                                                // handle
		E_Exit("Maximum number of Windows search handles exceeded");                            // Shouldn't
	                                                                                                // happen of
	                                                                                                // course
	WinSearches[openWinSearches].dta_pt = dta.GetPt();
	WinSearches[openWinSearches].psp = dos.psp();
	WinSearches[openWinSearches].sHandle = INVALID_HANDLE_VALUE;
	WinSearches[openWinSearches].root = !strncmp(basedir, srch_dir, strrchr(srch_dir, '\\')-srch_dir);
	WinSearches[openWinSearches].wildcard = (strpbrk(srch_dir, "?*") != NULL);
	openWinSearches++;
	return DoFindNext();
}

bool isDosName(char* fName)                                                                                                                     //
                                                                                                                                                // Check
                                                                                                                                                // for
                                                                                                                                                // valid
                                                                                                                                                // DOS
                                                                                                                                                // filename,
                                                                                                                                                // 8.3
                                                                                                                                                // and
                                                                                                                                                // no
                                                                                                                                                // forbidden
                                                                                                                                                // characters
{
	if(!strcmp(fName, ".") || !strcmp(fName, ".."))                                                                 //
		                                                                                                        // "."
		                                                                                                        // and
		                                                                                                        // ".."
		                                                                                                        // specials
		return true;
	if(strpbrk(fName, "+[] "))
		return false;
	char* pos = strchr(fName, '.');
	if(pos) {                                                                                                                                               //
	                                                                                                                                                        // If
	                                                                                                                                                        // extension
		if(strlen(pos) > 4 || strchr(pos+1, '.') || pos - fName > 8)                            // Max 3 chars,
			                                                                                //  max 1
			                                                                                // "extension",
			                                                                                // max name = 8
			                                                                                // chars
			return false;
	}
	else if(strlen(fName) > 8)                                                                                                              //
		                                                                                                                                // Max
		                                                                                                                                // name
		                                                                                                                                // =
		                                                                                                                                // 8
		                                                                                                                                // chars
		return false;
	return true;
}

bool DoFindNext(void)
{
	Bit8u srch_attr;
	char srch_pattern[DOS_NAMELENGTH_ASCII];

	DOS_DTA dta(dos.dta());
	dta.GetSearchParams(srch_attr, srch_pattern);
	srch_attr ^= (DOS_ATTR_VOLUME);
	WIN32_FIND_DATA search_data;
	Bit8u find_attr;
	PhysPt dtaAddress = dta.GetPt();

	DOS_SetError(DOSERR_NO_MORE_FILES);                                                                                             //
	                                                                                                                                // DOS
	                                                                                                                                // returns
	                                                                                                                                // this
	                                                                                                                                // on
	                                                                                                                                // error
	int winSearchEntry = -1;                                                                                                                //
	                                                                                                                                        // Is
	                                                                                                                                        // a
	                                                                                                                                        // FindFirst
	                                                                                                                                        // executed?
	for(int i = 0; i < openWinSearches; i++)
		if(WinSearches[i].dta_pt == dtaAddress)
			winSearchEntry = i;
	if(winSearchEntry == -1)
		return false;
	if(WinSearches[winSearchEntry].sHandle == INVALID_HANDLE_VALUE) { // Actually a FindFirst continuation
		if((WinSearches[winSearchEntry].sHandle = FindFirstFile(srch_dir, &search_data)) == INVALID_HANDLE_VALUE) {
			closeWinSeach(dta); // Invalidate this entry/dta search
			DOS_SetError((Bit16u)GetLastError());
			if(dos.errorcode == DOSERR_FILE_NOT_FOUND) // Windows returns this if not found
				DOS_SetError(DOSERR_NO_MORE_FILES); // DOS returns this instead
			return false;
		}
	}
	else if(!FindNextFile(WinSearches[winSearchEntry].sHandle, &search_data)) {
		closeWinSeach(dta);                                                                                                                     //
		                                                                                                                                        // Close
		                                                                                                                                        // this
		                                                                                                                                        // entry/dta
		                                                                                                                                        // search
		return false;
	}
	do {
		if(search_data.cFileName[0] == '.' && WinSearches[winSearchEntry].root)         // Don't show . and ..
			                                                                        // folders in vDos root
			                                                                        // drives
			continue;
		bool sfn = false;
		if(!isDosName(search_data.cFileName))                                                                           //
			                                                                                                        // If
			                                                                                                        // it's
			                                                                                                        // not
			                                                                                                        // a
			                                                                                                        // DOS
			                                                                                                        // 8.3
			                                                                                                        // name
			if(!WinSearches[winSearchEntry].wildcard)
				sfn = true;                                                                                                                     //
			                                                                                                                                        // We
			                                                                                                                                        // allow
			                                                                                                                                        // it
			                                                                                                                                        // only
			                                                                                                                                        // if
			                                                                                                                                        // Find
			                                                                                                                                        // was
			                                                                                                                                        // called
			                                                                                                                                        // to
			                                                                                                                                        // test
			                                                                                                                                        // the
			                                                                                                                                        // existance
			                                                                                                                                        // of
			                                                                                                                                        // a
			                                                                                                                                        // specific
			                                                                                                                                        // SFN
			                                                                                                                                        // name
			                                                                                                                                        // (no
			                                                                                                                                        // wildcards)
			else
				continue;
//		if (!(search_data.dwFileAttributes & srch_attr))
//			continue;
		if(search_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			find_attr = DOS_ATTR_DIRECTORY;
		else
			find_attr = DOS_ATTR_ARCHIVE;
		if(~srch_attr & find_attr & (DOS_ATTR_DIRECTORY | DOS_ATTR_HIDDEN | DOS_ATTR_SYSTEM))
			continue;
		// file is okay, setup everything to be copied in DTA Block
		char find_name[DOS_NAMELENGTH_ASCII];
		Bit16u find_date, find_time;
		Bit32u find_size;
		strcpy(find_name, sfn ? srch_pattern : search_data.cFileName);                          // If LFN of a
		                                                                                        // SFN was
		                                                                                        // found, we
		                                                                                        // can't use
		                                                                                        // that, use the
		                                                                                        // search
		                                                                                        // pattern (has
		                                                                                        // to be the
		                                                                                        // SFN)
		upcase(find_name);
		find_size = search_data.nFileSizeLow;
		FILETIME fTime;
		FileTimeToLocalFileTime(&search_data.ftLastWriteTime, &fTime);
		FileTimeToDosDateTime(&fTime, &find_date, &find_time);
		dta.SetResult(find_name, find_size, find_date, find_time, find_attr);
		if(!WinSearches[winSearchEntry].wildcard)                                                                       //
			                                                                                                        // If
			                                                                                                        // search
			                                                                                                        // w/o
			                                                                                                        // wildcards
			closeWinSeach(dta);                                                                                                             //
		                                                                                                                                        // Close
		                                                                                                                                        // this
		                                                                                                                                        // entry/dta
		                                                                                                                                        // search
		DOS_SetError(DOSERR_NONE);
		return true;
	}
	while(FindNextFile(WinSearches[winSearchEntry].sHandle, &search_data));
	closeWinSeach(dta);                                                                                                                             //
	                                                                                                                                                // Close
	                                                                                                                                                // this
	                                                                                                                                                // entry/dta
	                                                                                                                                                // search
	return false;
}

bool DOS_Drive::MakeDir(char * dir)
{
	char win_dir[MAX_PATH_LEN];
	if(CreateDirectory(strcat(strcpy(win_dir, basedir), dir), NULL))
		return true;
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

bool DOS_Drive::RemoveDir(char * dir)
{
	char win_dir[MAX_PATH_LEN];
	if(RemoveDirectory(strcat(strcpy(win_dir, basedir), dir)))
		return true;
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

bool DOS_Drive::Rename(char* oldname, char* newname)
{
	char winold[MAX_PATH_LEN];
	char winnew[MAX_PATH_LEN];
	if(MoveFile(strcat(strcpy(winold, basedir), oldname), strcat(strcpy(winnew, basedir), newname)))
		return true;
	Bit16u error = (Bit16u)GetLastError();
	if(error == ERROR_ALREADY_EXISTS)                                                                                               //
		                                                                                                                        // Not
		                                                                                                                        // kwnown
		                                                                                                                        // by
		                                                                                                                        // DOS
		error = DOSERR_ACCESS_DENIED;
	DOS_SetError(error);
	return false;
}

bool DOS_Drive::TestDir(char* dir)
{
	char win_dir[MAX_PATH_LEN];
	strcat(strcpy(win_dir, basedir), dir);
	if(win_dir[strlen(win_dir)-1] != '\\') // Make sure PathFileExists() only considers paths
		strcat(win_dir, "\\");
	// @sobolev if(PathFileExists(win_dir))
	if(::fileExists(win_dir))
		return true;
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

bool Disk_File::Read(Bit8u* data, Bit16u* size)
{
	if((flags&0xf) == OPEN_WRITE) { // Check if file opened in write-only mode
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}
	Bit32u bytesRead;
	for(int tries = 3; tries; tries--) { // Try three times
		if(ReadFile(hFile, static_cast<LPVOID>(data), (Bit32u)*size, &bytesRead, NULL)) {
			*size = (Bit16u)bytesRead;
			if(bytesRead) // Only if something is indeed read, skip the Idle function
				idleSkip = true;
			return true;
		}
		Sleep(25); // If failed (should be region locked), wait 25 millisecs
	}
	DOS_SetError((Bit16u)GetLastError());
	*size = 0; // Is this OK ??
	return false;
}

bool Disk_File::Write(Bit8u* data, Bit16u* size)
{
	if((flags&0xf) == OPEN_READ) { // Check if file opened in read-only mode
		DOS_SetError(DOSERR_ACCESS_DENIED);
		return false;
	}
	if(*size == 0)
		if(SetEndOfFile(hFile))
			return true;
		else {
			DOS_SetError((Bit16u)GetLastError());
			return false;
		}
	Bit32u bytesWritten;
	for(int tries = 3; tries; tries--) { // Try three times
		if(WriteFile(hFile, data, (Bit32u)*size, &bytesWritten, NULL)) {
			*size = (Bit16u)bytesWritten;
			idleSkip = true;
			return true;
		}
		Sleep(25); // If failed (should be region locked? (not documented in MSDN)), wait 25 millisecs
	}
	DOS_SetError((Bit16u)GetLastError());
	*size = 0; // Is this OK ??
	return false;
}

bool Disk_File::LockFile(Bit8u mode, Bit32u pos, Bit32u size)
{
	if(mode > 1) {
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
	}
	BOOL bRet = false;
	//auto lockFunct = (mode == 0) ? ::LockFile : ::UnlockFile;
	for(int tries = 3; tries; tries--) { // Try three times
		if(mode == 0) {
			bRet = ::LockFile(hFile, pos, 0, size, 0);
		}
		else {
			bRet = ::UnlockFile(hFile, pos, 0, size, 0);
		}
		//if((bRet = lockFunct(hFile, pos, 0, size, 0)))
		if(bRet)
			return true;
		Sleep(25); // If failed, wait 25 millisecs
	}
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

bool Disk_File::Seek(Bit32u* pos, Bit32u type)
{
	if(type > 2) {
		DOS_SetError(DOSERR_FUNCTION_NUMBER_INVALID);
		return false;
	}
	Bit32s dwPtr = SetFilePointer(hFile, *pos, NULL, type);
	if(dwPtr != INVALID_SET_FILE_POINTER) {                                                                                 //
	                                                                                                                        // If
	                                                                                                                        // succes
		*pos = (Bit32u)dwPtr;
		return true;
	}
	DOS_SetError((Bit16u)GetLastError());
	return false;
}

void Disk_File::Close()
{
	if(refCtr == 1)                                                                                                                                 //
		                                                                                                                                        // Only
		                                                                                                                                        // close
		                                                                                                                                        // if
		                                                                                                                                        // one
		                                                                                                                                        // reference
		                                                                                                                                        // left
		CloseHandle(hFile);
}

Bit16u Disk_File::GetInformation(void)
{
	return 0;
}

Disk_File::Disk_File(const char* _name, HANDLE handle)
{
	hFile = handle;
	UpdateDateTimeFromHost();

	attr = DOS_ATTR_ARCHIVE;

	name = 0;
	SetName(_name);
}

void Disk_File::UpdateDateTimeFromHost(void)
{
	FILETIME ftWrite;
	SYSTEMTIME stUTC, stLocal;
	time = date = 1;
	if(GetFileTime(hFile, NULL, NULL, &ftWrite)) {
		FileTimeToSystemTime(&ftWrite, &stUTC);                                                                         //
		                                                                                                                // Convert
		                                                                                                                // the
		                                                                                                                // last-write
		                                                                                                                // time
		                                                                                                                // to
		                                                                                                                // local
		                                                                                                                // time
		SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
		SystemTimeToFileTime(&stLocal, &ftWrite);
		FileTimeToDosDateTime(&ftWrite, &date, &time);
	}
	return;
}

