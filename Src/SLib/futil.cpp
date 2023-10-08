// FUTIL.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <ued.h>
#include <ued-id.h>
#ifdef WIN32
	#include <shlobj.h> // SHGetFolderPath and constants
	#include <Shlwapi.h> // @v11.6.5
#endif

SDataMoveProgressInfo::SDataMoveProgressInfo()
{
	THISZERO();
}

bool FASTCALL fileExists(const char * pFileName)
{
///* @snippet Пример определения существования файла посредством WINAPI (Shlwapi.h)
	bool result = false;
	if(!isempty(pFileName)) {
#ifdef WIN32
		SString _file_name(pFileName);
		if(_file_name.IsAscii()) {
			const DWORD attributes = GetFileAttributesA(_file_name);
			// special directory case to drive the network path check
			const BOOL is_directory = (attributes == INVALID_FILE_ATTRIBUTES) ? (GetLastError() == ERROR_BAD_NETPATH) : BIN(attributes & FILE_ATTRIBUTE_DIRECTORY);
			if(is_directory) {
				result = (PathIsNetworkPathA(_file_name) || PathIsUNCA(_file_name));
			}
			if(!result && PathFileExistsA(_file_name)) 
				result = true;
		}
		else if(_file_name.IsLegalUtf8()) {
			SStringU _file_name_u;
			_file_name_u.CopyFromUtf8R(_file_name, 0);
			const DWORD attributes = GetFileAttributesW(_file_name_u);
			// special directory case to drive the network path check
			const BOOL is_directory = (attributes == INVALID_FILE_ATTRIBUTES) ? (GetLastError() == ERROR_BAD_NETPATH) : BIN(attributes & FILE_ATTRIBUTE_DIRECTORY);
			if(is_directory) {
				result = (PathIsNetworkPathW(_file_name_u) || PathIsUNCW(_file_name_u));
			}
			if(!result && PathFileExistsW(_file_name_u)) 
				result = true;
		}
		else {
			const DWORD attributes = GetFileAttributesA(pFileName);
			// special directory case to drive the network path check
			const BOOL is_directory = (attributes == INVALID_FILE_ATTRIBUTES) ? (GetLastError() == ERROR_BAD_NETPATH) : BIN(attributes & FILE_ATTRIBUTE_DIRECTORY);
			if(is_directory) {
				result = (PathIsNetworkPathA(pFileName) || PathIsUNCA(pFileName));
			}
			if(!result && PathFileExistsA(pFileName)) 
				result = true;
		}
#else
		result = (::access(pFileName, 0) == 0);
#endif
	}
	if(!result)
		SLS.SetError(SLERR_FILENOTFOUND, pFileName);
	return result;
}

#ifdef __WIN32__

static int getdisk()
{
	wchar_t buf[MAX_PATH];
	::GetCurrentDirectoryW(SIZEOFARRAY(buf), buf);
	return *buf-L'A';
}

int pathToUNC(const char * pPath, SString & rUncPath)
{
	int    ok = 1;
	char   disk[4] = "X:\\";
	*disk = *pPath;
	rUncPath = pPath;
	if(GetDriveType(SUcSwitch(disk)) == DRIVE_REMOTE) {
		char   namebuf[MAX_PATH + sizeof(UNIVERSAL_NAME_INFO)];
		namebuf[0] = 0;
		DWORD  len = MAX_PATH;
		DWORD  wstat = WNetGetUniversalName(SUcSwitch(pPath), UNIVERSAL_NAME_INFO_LEVEL, &namebuf, &len);
		if(wstat != NO_ERROR)
			ok = (SLibError = SLERR_INVPATH, 0);
		else
			rUncPath = SUcSwitch(reinterpret_cast<UNIVERSAL_NAME_INFO *>(&namebuf)->lpUniversalName);
	}
	return ok;
}

static int Win_IsFileExists(const char * pFileName)
{
	HANDLE handle = ::CreateFile(SUcSwitch(pFileName), 0/* query access only */, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE/* share mode */,
		NULL/*security attributes*/, OPEN_EXISTING/*disposition*/, FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN/* flags & attributes */, NULL/* template file */);
	if(handle != INVALID_HANDLE_VALUE) {
		::CloseHandle(handle);
		return 1;
	}
	else
		return 0;
}

#endif

char * setLastSlash(char * p)
{
	size_t len = sstrlen(p);
	if(len > 0 && !isdirslash(p[len-1])) {
		p[len++] = '\\';
		p[len] = 0;
	}
	return p;
}

char * rmvLastSlash(char * p)
{
	const size_t len = sstrlen(p);
	if(len > 0 && isdirslash(p[len-1]))
		p[len-1] = 0;
	return p;
}

SString & GetExecPath(SString & rBuf)
{
	SPathStruc ps(SLS.GetExePath());
	ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, rBuf);
	return rBuf;
}

#pragma warn -asc

int driveValid(const char * pPath)
{
	int    ok = 0;
	if(!isempty(pPath)) {
		// @v11.7.10 {
		SPathStruc ps(pPath);
		if(ps.Drv.NotEmpty()) {
			SString temp_buf;
			if(ps.Flags & SPathStruc::fUNC) {
				(temp_buf = "\\\\").Cat(ps.Drv).SetLastSlash();
				ok = GetVolumeInformation(SUcSwitch(temp_buf), 0, 0, 0, 0, 0, 0, 0);
			}
			else {
				(temp_buf = ps.Drv).Colon().SetLastSlash();
				uint t = GetDriveType(SUcSwitch(temp_buf));
				if(t != DRIVE_UNKNOWN && t != DRIVE_NO_ROOT_DIR)
					ok = 1;
			}
		}
		// } @v11.7.10 
		/* @v11.7.10 {
		char   dname[4] = "X:\\";
		dname[0] = *pPath;
		uint t = GetDriveType(SUcSwitch(dname));
		if(t != DRIVE_UNKNOWN && t != DRIVE_NO_ROOT_DIR)
			ok = 1;
		else if(pPath[0] == '\\' && pPath[1] == '\\') {
			char   buf[MAX_PATH];
			fnsplit(pPath, 0, buf, 0, 0);
			char   * p = 0;
			p = ((p = sstrchr(buf+2, '\\')) ? sstrchr(p+1, '\\') : 0);
			if(p) {
				*p = 0;
				setLastSlash(buf);
				ok = GetVolumeInformation(SUcSwitch(buf), 0, 0, 0, 0, 0, 0, 0);
			}
		}*/
	}
	return ok;
}

#pragma warn .asc

int FASTCALL IsDirectory(const char * pStr)
{
	int    yes = 0;
	if(!isempty(pStr)) {
#ifdef __WIN32__
		WIN32_FIND_DATA fd;
		MEMSZERO(fd);
		HANDLE h = FindFirstFile(SUcSwitch(pStr), &fd);
		if(h != INVALID_HANDLE_VALUE) {
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				yes = 1;
			FindClose(h);
		}
		else {
			//SString temp_buf;
			//(temp_buf = pStr).Strip().SetLastSlash().CatChar('*');
			h = FindFirstFile(SUcSwitch(/*temp_buf*/SString(pStr).Strip().SetLastSlash().CatChar('*')), &fd);
			if(h != INVALID_HANDLE_VALUE) {
				yes = 1;
				FindClose(h);
			}
		}
#else
		struct ffblk ff;
		yes = BIN(findfirst(pStr, &ff, FA_DIREC) == 0 && (ff.ff_attrib & FA_DIREC));
#endif
	}
	return yes;
}
//
// fexpand:    reimplementation of pascal's FExpand routine.
// Takes a relative DOS path and makes an absolute path of the form
// drive:\[subdir\ ...]filename.ext
//
// works with '/' or '\' as the subdir separator on input; changes all '/'
// to '\' on output.
//
#ifndef __WIN32__ // {
static char * squeeze(char * path)
{
	char * dest = path;
	char * src = path;
	while(*src) {
		if(*src != '.')
			*dest++ = *src++;       // just copy it...
		else if(*++src == '.') {	// have a '..'
			src += 2;       // skip the following '\'
			dest--; // back up to the previous '\'
			while(*--dest != '\\'); // back up to the previous '\'
			dest++; // move to the next position
		}
		else {
			src++;  // skip the following '\'
			dest += 2;
		}
	}
	*dest = 0;	      // zero terminator
	return path;
}
#endif // } __WIN32__

int pathValid(const char * pPath, int existOnly)
{
	SString exp_path(pPath);
	{
		TCHAR * fn = 0;
		TCHAR  fpn_buf[MAX_PATH];
		::GetFullPathName(SUcSwitch(exp_path), SIZEOFARRAY(fpn_buf), fpn_buf, &fn);
		exp_path = SUcSwitch(fpn_buf);
	}
	return (exp_path.Len() <= 3) ? driveValid(exp_path) : (existOnly ? IsDirectory(exp_path.RmvLastSlash()) : 1);
}

SString & STDCALL MakeTempFileName(const char * pDir, const char * pPrefix, const char * pExt, long * pStart, SString & rBuf)
{
	char   prefix[128], ext[128];
	size_t prefix_len = 0;
	long   start = DEREFPTROR(pStart, 1);
	if(pPrefix)
		prefix_len = sstrlen(strnzcpy(prefix, pPrefix, 20));
	else
		PTR32(prefix)[0] = 0;
	const uint nd = (prefix_len <= 6) ? (8-prefix_len) : 4;
	if(pExt)
		if(pExt[0] == '.')
			strnzcpy(ext, pExt+1, sizeof(ext));
		else
			strnzcpy(ext, pExt, sizeof(ext));
	else
		PTR32(ext)[0] = 0;
	for(rBuf.Z(); rBuf.IsEmpty() && start < 9999999L;) {
		if(pDir)
			(rBuf = pDir).Strip().SetLastSlash();
		rBuf.Cat(prefix).CatLongZ(start++, nd).Dot().Cat(ext); // @v9.9.12 (8-prefix_len)-->nd
		if(fileExists(rBuf))
			rBuf.Z();
	}
	ASSIGN_PTR(pStart, start);
	return rBuf;
}

int createDir(const char * pPath)
{
	int    ok = 1;
	SString path;
	SString temp_path;
	SStringU temp_buf_u;
	// @v11.2.12 (temp_path = pPath).SetLastSlash().ReplaceChar('/', '\\');
	// @v11.8.4 SPathStruc::npfCompensateDotDot
	SPathStruc::NormalizePath(pPath, SPathStruc::npfKeepCase|SPathStruc::npfCompensateDotDot, temp_path).SetLastSlash(); // @v11.2.12 
	const char * p = temp_path;
	do {
		if(*p == '\\') {
			if(p[1] == '\\')
				path.CatChar(*p++);
			else if(path.NotEmpty()) {
				const bool is_root = (path[0] == path[1] && path[0] == '\\' && !sstrchr(path+2, '\\'));
				if(!is_root && (path[0] && ::access(path, 0) != 0)) {
					if(path.IsLegalUtf8()) {
						temp_buf_u.CopyFromUtf8(path);
					}
					else {
						temp_buf_u.CopyFromMb_OUTER(path, path.Len());
					}
					const int cdr = ::CreateDirectoryW(temp_buf_u, NULL);
					if(cdr == 0) {
						SLS.SetAddedMsgString(path);
						ok = (SLibError = SLERR_MKDIRFAULT, 0);
					}
					else
						ok = 1;
				}
			}
		}
		path.CatChar(*p);
	} while(ok && *p++ != 0);
	return ok;
}

bool FASTCALL IsWild(const char * f) { return (!isempty(f) && strpbrk(f, "*?") != 0); }

SString & makeExecPathFileName(const char * pName, const char * pExt, SString & rPath)
{
	HMODULE h_inst = SLS.GetHInst();
	SString path;
	SSystem::SGetModuleFileName(h_inst, path);
	SPathStruc ps(path);
	ps.Nam = pName;
	ps.Ext = pExt;
	ps.Merge(rPath);
	// } @v10.3.9 
	return rPath;
}

int FASTCALL createEmptyFile(const char * pFileName)
{
	int    ok = 0;
	if(pFileName) {
		FILE * f = fopen(pFileName, "w");
		if(f) {
			fclose(f);
			ok = 1;
		}
	}
	return ok;
}
//
//
//
struct fat_date {
	unsigned day   : 5; /* Days */
	unsigned month : 4; /* Months */
	unsigned year  : 7; /* Year - 1980 */
};

struct fat_time {
	unsigned tsec  : 5; /* Two seconds */
	unsigned min   : 6; /* Minutes */
	unsigned hour  : 5; /* Hours */
};

void STDCALL decode_fat_datetime(uint16 fd, uint16 ft, LDATETIME * dt)
{
	union {
		uint16 d;
		struct fat_date fd;
	} fat_d;
	union {
		uint16 t;
		struct fat_time ft;
	} fat_t;
	fat_d.d = fd;
	fat_t.t = ft;
	encodedate(fat_d.fd.day, fat_d.fd.month, fat_d.fd.year + 1980, &dt->d);
	dt->t = encodetime(fat_t.ft.hour, fat_t.ft.min, fat_t.ft.tsec * 2, 0);
}

void STDCALL encode_fat_datetime(uint16 * fd, uint16 * ft, const LDATETIME * dt)
{
	int d, m, y, h, s, hs;
	union {
		uint16 d;
		struct fat_date fd;
	} fat_d;
	union {
		uint16 t;
		struct fat_time ft;
	} fat_t;
	decodedate(&d, &m, &y, &dt->d);
	fat_d.fd.day   = d;
	fat_d.fd.month = m;
	fat_d.fd.year  = y - 1980;
	*fd = fat_d.d;
	decodetime(&h, &m, &s, &hs, &dt->t);
	fat_t.ft.hour  = h;
	fat_t.ft.min   = m;
	fat_t.ft.tsec  = s / 2;
	*ft = fat_t.t;
}

int copyFileByName(const char * pSrcFileName, const char * pDestFileName)
{
#ifdef __WIN32__
	int    r = ::CopyFile(SUcSwitch(pSrcFileName), SUcSwitch(pDestFileName), 0);
	if(!r)
		SLS.SetOsError();
	return r;
#else
	return SCopyFile(pSrcFileName, pDestFileName, 0, 0);
#endif
}

#ifdef __WIN32__

int SCopyFile(const char * pSrcFileName, const char * pDestFileName, SDataMoveProgressProc pp, long shareMode, void * pExtra)
{
	EXCEPTVAR(SLibError);
	int   ok = 1, reply;
	int   quite = 0, cancel = 0;
	HANDLE desthdl = 0;
	void * p_buf  = 0;
	uint32 flen;
	uint32 buflen = SMEGABYTE(4);
	uint32 len;
	DWORD  bytes_read_write;
	SDataMoveProgressInfo scfd;
	SString added_msg;
	SString sys_err_buf; // @v10.3.11
	FILETIME creation_time, last_access_time, last_modif_time;
	HANDLE srchdl = ::CreateFile(SUcSwitch(pSrcFileName), GENERIC_READ, shareMode, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);
	if(srchdl == INVALID_HANDLE_VALUE) {
		added_msg = pSrcFileName;
		//char   tmp_msg_buf[256];
		//::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)tmp_msg_buf, sizeof(tmp_msg_buf), 0);
		//chomp(SCharToOem(tmp_msg_buf));
		//added_msg.CR().CatQStr(tmp_msg_buf);
		// @v10.3.11 {
		if(SSystem::SFormatMessage(sys_err_buf))
			added_msg.CR().CatQStr(sys_err_buf.Chomp().Transf(CTRANSF_OUTER_TO_INNER));
		// } @v10.3.11
		SLS.SetError(SLERR_OPENFAULT, added_msg);
		CALLEXCEPT();
	}
	//SLS.SetAddedMsgString(pSrcFileName);
	//THROW_V(srchdl != INVALID_HANDLE_VALUE, SLERR_OPENFAULT);
	GetFileTime(srchdl, &creation_time, &last_access_time, &last_modif_time);
	desthdl = ::CreateFile(SUcSwitch(pDestFileName), GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(desthdl == INVALID_HANDLE_VALUE) {
		added_msg = pDestFileName;
		//char   tmp_msg_buf[256];
		//::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)tmp_msg_buf, sizeof(tmp_msg_buf), 0);
		//chomp(SCharToOem(tmp_msg_buf));
		//added_msg.CR().CatQStr(tmp_msg_buf);
		// @v10.3.11 {
		if(SSystem::SFormatMessage(sys_err_buf))
			added_msg.CR().CatQStr(sys_err_buf.Chomp().Transf(CTRANSF_OUTER_TO_INNER));
		// } @v10.3.11
		SLS.SetError(SLERR_OPENFAULT, added_msg);
		CALLEXCEPT();
	}
	//SLS.SetAddedMsgString(pDestFileName);
	//THROW_V(desthdl != INVALID_HANDLE_VALUE, SLERR_OPENFAULT);
	flen = ::GetFileSize(srchdl, 0);

	scfd.P_Src   = pSrcFileName;
	scfd.P_Dest  = pDestFileName;
	scfd.SizeTotal = flen;
	scfd.SizeDone  = 0UL;
	scfd.ExtraPtr = pExtra;
	if(pp && !quite) {
		reply = pp(&scfd);
		if(oneof2(reply, SPRGRS_CANCEL, SPRGRS_STOP))
			cancel = 1;
		else if(reply == SPRGRS_QUITE)
			quite = 1;
	}
	if(!cancel && flen) {
		SetFilePointer(srchdl, 0L, NULL, FILE_BEGIN);
		if(flen < buflen)
			buflen = ALIGNSIZE(flen, 10);
		do {
			p_buf = SAlloc::M(buflen);
		} while(!p_buf && (buflen -= SKILOBYTE(1)) >= SKILOBYTE(1));
		THROW(p_buf);
		do {
			THROW_V(ReadFile(srchdl, p_buf, (flen < buflen) ? flen : buflen, &bytes_read_write, NULL), SLERR_READFAULT);
			len = bytes_read_write;
			if(!::WriteFile(desthdl, p_buf, len, &bytes_read_write, NULL)) {
				DWORD err = GetLastError();
				if(err == ERROR_DISK_FULL) {
					CALLEXCEPTV(SLERR_DISKFULL);
				}
				else
					CALLEXCEPTV(SLERR_WRITEFAULT);
			}
			THROW_V(bytes_read_write == len, SLERR_WRITEFAULT);
			flen -= len;
			if(pp && !quite) {
				scfd.SizeDone += len;
				reply = pp(&scfd);
				if(oneof2(reply, SPRGRS_CANCEL, SPRGRS_STOP))
					cancel = 1;
				else if(reply == SPRGRS_QUITE)
					quite = 1;
			}
		} while(!cancel && ok && len == buflen && flen > 0);
	}
	if(cancel)
		ok = -1;
	CATCHZOK
	SAlloc::F(p_buf);
	if(desthdl > 0) {
		SetFileTime(desthdl, &creation_time, &last_access_time, &last_modif_time);
		CloseHandle(desthdl);
	}
	if(srchdl > 0) {
		CloseHandle(srchdl);
	}
	if(cancel || !ok)
		SFile::Remove(pDestFileName);
	return ok;
}

#else /* __WIN32__ */

int SCopyFile(const char * pSrcFileName, const char * pDestFileName, SDataMoveProgressProc pp, void * pExtra)
{
	const size_t KB = 1024;

	EXCEPTVAR(SLibError);
	int   ok = 1, reply;
	int   quite = 0, cancel = 0;
	int   srchdl  = -1;
	int   desthdl = -1;
	void * p_buf  = 0;
	ulong flen;
	uint  buflen = 32 * KB;
	uint  len;
	SDataMoveProgressInfo scfd;
	LDATETIME creation_time, last_access_time, last_modif_time;
	srchdl = open(pSrcFileName, O_RDONLY | O_BINARY, S_IWRITE | S_IREAD);
	THROW_V(srchdl >= 0, SLERR_OPENFAULT);
	getFileTime(srchdl, &creation_time, &last_access_time, &last_modif_time);
	desthdl = open(pDestFileName, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
	THROW_V(desthdl >= 0, SLERR_OPENFAULT);
	lseek(srchdl, 0L, SEEK_END);
	flen = tell(srchdl);
	scfd.SrcFileName   = pSrcFileName;
	scfd.DestFileName  = pDestFileName;
	scfd.TotalFileSize = flen;
	scfd.TransferredBytes = 0UL;
	scfd.ExtraParam = pExtra;
	if(pp && !quite) {
		reply = pp(&scfd);
		if(reply == SPRGRS_CANCEL || reply == SPRGRS_STOP)
			cancel = 1;
		else if(reply == SPRGRS_QUITE)
			quite = 1;
	}
	if(!cancel && flen) {
		lseek(srchdl, 0L, SEEK_SET);
		if(flen < buflen)
			buflen = KB * (1 + (uint)flen / KB);
		do
			p_buf = SAlloc::M(buflen);
		while(!p_buf && (buflen -= KB) >= KB);
		THROW(p_buf);
		do {
			THROW_V((len = read(srchdl, p_buf, (flen < buflen) ? (uint)flen : buflen)) > 0, SLERR_READFAULT);
			THROW_V(write(desthdl, p_buf, len) == (int)len, SLERR_WRITEFAULT);
			flen -= len;
			if(pp && !quite) {
				scfd.TransferredBytes += len;
				reply = pp(&scfd);
				if(reply == SPRGRS_CANCEL || reply == SPRGRS_STOP)
					cancel = 1;
				else if(reply == SPRGRS_QUITE)
					quite = 1;
			}
		} while(!cancel && ok && len == buflen && flen > 0);
	}
	if(cancel)
		ok = -1;
	CATCHZOK
	SAlloc::F(p_buf);
	if(desthdl >= 0) {
		setFileTime(desthdl, &creation_time, &last_access_time, &last_modif_time);
		close(desthdl);
	}
	if(srchdl >= 0)
		close(srchdl);
	if(cancel || !ok)
		SFile::Remove(pDestFileName);
	return ok;
}

#endif /* !__WIN32__ */

int GetFileStat(const char * pFileName, SDirEntry * pEntry)
{
	SDirec direc(pFileName);
	return direc.Next(pEntry);
}

int RemoveDir(const char * pDir)
{
	int    ok = 1;
	if(pDir) {
		SString path, wildcard;
		SDirEntry de;
		for(SDirec dir((wildcard = pDir).SetLastSlash().Cat("*.*")); ok && dir.Next(&de) > 0;) {
			if(!de.IsSelf() && !de.IsUpFolder()) {
				de.GetNameA(pDir, path);
				ok = de.IsFolder() ? RemoveDir(path) /* @recursion */ : SFile::Remove(path);
			}
		}
		if(ok > 0) {
			(path = pDir).RmvLastSlash();
			if(::RemoveDirectory(SUcSwitch(path)) != 0)
				ok = 1;
			else
				ok = 0;
		}
	}
	else
		ok = 0;
	return ok;
}

static const U64UUIDAssoc FsKnownFolder_Assoc_Windows[] = {
	{ UED_FSKNOWNFOLDER_ACCOUNT_PICTURES,       FOLDERID_AccountPictures },
	{ UED_FSKNOWNFOLDER_ADD_NEW_PROGRAMS,       FOLDERID_AddNewPrograms },
	{ UED_FSKNOWNFOLDER_ADMIN_TOOLS,            FOLDERID_AdminTools },
	{ UED_FSKNOWNFOLDER_APP_DATA_DESKTOP,       FOLDERID_AppDataDesktop },
	{ UED_FSKNOWNFOLDER_APP_DATA_DOCUMENTS,     FOLDERID_AppDataDocuments },
	{ UED_FSKNOWNFOLDER_APP_DATA_FAVORITES,     FOLDERID_AppDataFavorites },
	{ UED_FSKNOWNFOLDER_APP_DATA_PROGRAMDATA,   FOLDERID_AppDataProgramData },
	{ UED_FSKNOWNFOLDER_APP_UPDATES,            FOLDERID_AppUpdates },
	{ UED_FSKNOWNFOLDER_APPLICATION_SHORTCUTS,  FOLDERID_ApplicationShortcuts },
	{ UED_FSKNOWNFOLDER_APPS,                   FOLDERID_AppsFolder },
	{ UED_FSKNOWNFOLDER_CD_BURNING,             FOLDERID_CDBurning },
	{ UED_FSKNOWNFOLDER_CAMERA_ROLL,            FOLDERID_CameraRoll },
	{ UED_FSKNOWNFOLDER_CHANGE_REMOVE_PROGRAMS, FOLDERID_ChangeRemovePrograms },
	{ UED_FSKNOWNFOLDER_COMMON_ADMIN_TOOLS,     FOLDERID_CommonAdminTools },
	{ UED_FSKNOWNFOLDER_COMMON_OEM_LINKS,       FOLDERID_CommonOEMLinks },
	{ UED_FSKNOWNFOLDER_COMMON_PROGRAMS,        FOLDERID_CommonPrograms },
	{ UED_FSKNOWNFOLDER_COMMON_START_MENU,      FOLDERID_CommonStartMenu },
	{ UED_FSKNOWNFOLDER_COMMON_STARTUP,         FOLDERID_CommonStartup },
	{ UED_FSKNOWNFOLDER_COMMON_TEMPLATES,       FOLDERID_CommonTemplates },
	{ UED_FSKNOWNFOLDER_COMPUTER,               FOLDERID_ComputerFolder },
	{ UED_FSKNOWNFOLDER_CONFLICT,               FOLDERID_ConflictFolder },
	{ UED_FSKNOWNFOLDER_CONNECTIONS,            FOLDERID_ConnectionsFolder },
	{ UED_FSKNOWNFOLDER_CONTACTS,               FOLDERID_Contacts },
	{ UED_FSKNOWNFOLDER_CONTROLPANEL,           FOLDERID_ControlPanelFolder },
	{ UED_FSKNOWNFOLDER_COOKIES,                FOLDERID_Cookies },
	{ UED_FSKNOWNFOLDER_DESKTOP,                FOLDERID_Desktop },
	{ UED_FSKNOWNFOLDER_DEVICE_METADATA_STORE,  FOLDERID_DeviceMetadataStore },
	{ UED_FSKNOWNFOLDER_DOCUMENTS,              FOLDERID_Documents },
	{ UED_FSKNOWNFOLDER_DOCUMENTS_LIBRARY,      FOLDERID_DocumentsLibrary },
	{ UED_FSKNOWNFOLDER_DOWNLOADS,              FOLDERID_Downloads },
	{ UED_FSKNOWNFOLDER_FAVORITES,              FOLDERID_Favorites },
	{ UED_FSKNOWNFOLDER_FONTS,                  FOLDERID_Fonts },
	{ UED_FSKNOWNFOLDER_GAME_TASKS,             FOLDERID_GameTasks },
	{ UED_FSKNOWNFOLDER_GAMES,                  FOLDERID_Games },
	{ UED_FSKNOWNFOLDER_HISTORY,                FOLDERID_History },
	{ UED_FSKNOWNFOLDER_HOME_GROUP,             FOLDERID_HomeGroup },
	{ UED_FSKNOWNFOLDER_HOME_GROUP_CURRENT_USER, FOLDERID_HomeGroupCurrentUser },
	{ UED_FSKNOWNFOLDER_IMPLICIT_APP_SHORTCUTS,  FOLDERID_ImplicitAppShortcuts },
	{ UED_FSKNOWNFOLDER_INTERNET_CACHE,          FOLDERID_InternetCache },
	{ UED_FSKNOWNFOLDER_INTERNET,                FOLDERID_InternetFolder },
	{ UED_FSKNOWNFOLDER_LIBRARIES,               FOLDERID_Libraries },
	{ UED_FSKNOWNFOLDER_LINKS,                   FOLDERID_Links },
	{ UED_FSKNOWNFOLDER_LOCAL_APP_DATA,          FOLDERID_LocalAppData },
	{ UED_FSKNOWNFOLDER_LOCAL_APP_DATA_LOW,      FOLDERID_LocalAppDataLow },
	{ UED_FSKNOWNFOLDER_LOCALIZED_RESOURCES_DIR, FOLDERID_LocalizedResourcesDir },
	{ UED_FSKNOWNFOLDER_MUSIC,                   FOLDERID_Music },
	{ UED_FSKNOWNFOLDER_MUSIC_LIBRARY, FOLDERID_MusicLibrary },
	{ UED_FSKNOWNFOLDER_NETHOOD,   FOLDERID_NetHood },
	{ UED_FSKNOWNFOLDER_NETWORK,   FOLDERID_NetworkFolder },
	{ UED_FSKNOWNFOLDER_OBJECTS3D, FOLDERID_Objects3D },
	{ UED_FSKNOWNFOLDER_ORIGINAL_IMAGES,  FOLDERID_OriginalImages },
	{ UED_FSKNOWNFOLDER_PHOTO_ALBUMS,     FOLDERID_PhotoAlbums },
	{ UED_FSKNOWNFOLDER_PICTURES,         FOLDERID_Pictures },
	{ UED_FSKNOWNFOLDER_PICTURES_LIBRARY, FOLDERID_PicturesLibrary },
	{ UED_FSKNOWNFOLDER_PLAYLISTS, FOLDERID_Playlists },
	{ UED_FSKNOWNFOLDER_PRINTHOOD, FOLDERID_PrintHood },
	{ UED_FSKNOWNFOLDER_PRINTERS,  FOLDERID_PrintersFolder },
	{ UED_FSKNOWNFOLDER_PROFILE,   FOLDERID_Profile },
	{ UED_FSKNOWNFOLDER_PROGRAM_DATA,  FOLDERID_ProgramData },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES, FOLDERID_ProgramFiles },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES_COMMON,     FOLDERID_ProgramFilesCommon },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES_COMMON_X64, FOLDERID_ProgramFilesCommonX64 },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES_COMMON_X86, FOLDERID_ProgramFilesCommonX86 },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES_X64,        FOLDERID_ProgramFilesX64 },
	{ UED_FSKNOWNFOLDER_PROGRAM_FILES_X86,        FOLDERID_ProgramFilesX86 },
	{ UED_FSKNOWNFOLDER_PROGRAMS,                 FOLDERID_Programs },
	{ UED_FSKNOWNFOLDER_PUBLIC,                   FOLDERID_Public },
	{ UED_FSKNOWNFOLDER_PUBLIC_DESKTOP,    FOLDERID_PublicDesktop },
	{ UED_FSKNOWNFOLDER_PUBLIC_DOCUMENTS,  FOLDERID_PublicDocuments },
	{ UED_FSKNOWNFOLDER_PUBLIC_DOWNLOADS,  FOLDERID_PublicDownloads },
	{ UED_FSKNOWNFOLDER_PUBLIC_GAME_TASKS, FOLDERID_PublicGameTasks },
	{ UED_FSKNOWNFOLDER_PUBLIC_LIBRARIES,  FOLDERID_PublicLibraries },
	{ UED_FSKNOWNFOLDER_PUBLIC_MUSIC,      FOLDERID_PublicMusic },
	{ UED_FSKNOWNFOLDER_PUBLIC_PICTURES,   FOLDERID_PublicPictures },
	{ UED_FSKNOWNFOLDER_PUBLIC_RINGTONES,  FOLDERID_PublicRingtones },
	{ UED_FSKNOWNFOLDER_PUBLIC_USER_TILES, FOLDERID_PublicUserTiles },
	{ UED_FSKNOWNFOLDER_PUBLIC_VIDEOS,     FOLDERID_PublicVideos },
	{ UED_FSKNOWNFOLDER_QUICK_LAUNCH,      FOLDERID_QuickLaunch },
	{ UED_FSKNOWNFOLDER_RECENT,            FOLDERID_Recent },
	//{ UED_FSKNOWNFOLDER_RECORDED_TV, FOLDERID_RecordedTV },
	{ UED_FSKNOWNFOLDER_RECORDED_TV_LIBRARY, FOLDERID_RecordedTVLibrary },
	{ UED_FSKNOWNFOLDER_RECYCLEBIN,   FOLDERID_RecycleBinFolder },
	{ UED_FSKNOWNFOLDER_RESOURCE_DIR, FOLDERID_ResourceDir },
	{ UED_FSKNOWNFOLDER_RINGTONES,    FOLDERID_Ringtones },
	{ UED_FSKNOWNFOLDER_ROAMED_TILE_IMAGES, FOLDERID_RoamedTileImages },
	{ UED_FSKNOWNFOLDER_ROAMING_APP_DATA,   FOLDERID_RoamingAppData },
	{ UED_FSKNOWNFOLDER_ROAMING_TILES,      FOLDERID_RoamingTiles },
	{ UED_FSKNOWNFOLDER_SEARCH_CSC,         FOLDERID_SEARCH_CSC },
	{ UED_FSKNOWNFOLDER_SEARCH_MAPI,        FOLDERID_SEARCH_MAPI },
	{ UED_FSKNOWNFOLDER_SAMPLE_MUSIC,       FOLDERID_SampleMusic },
	{ UED_FSKNOWNFOLDER_SAMPLE_PICTURES,    FOLDERID_SamplePictures },
	{ UED_FSKNOWNFOLDER_SAMPLE_PLAYLISTS,   FOLDERID_SamplePlaylists },
	{ UED_FSKNOWNFOLDER_SAMPLE_VIDEOS,      FOLDERID_SampleVideos },
	{ UED_FSKNOWNFOLDER_SAVED_GAMES,        FOLDERID_SavedGames },
	{ UED_FSKNOWNFOLDER_SAVED_PICTURES,     FOLDERID_SavedPictures },
	{ UED_FSKNOWNFOLDER_SAVED_PICTURES_LIBRARY, FOLDERID_SavedPicturesLibrary },
	{ UED_FSKNOWNFOLDER_SAVED_SEARCHES, FOLDERID_SavedSearches },
	{ UED_FSKNOWNFOLDER_SCREENSHOTS, FOLDERID_Screenshots },
	{ UED_FSKNOWNFOLDER_SEARCH_HISTORY, FOLDERID_SearchHistory },
	{ UED_FSKNOWNFOLDER_SEARCH_HOME, FOLDERID_SearchHome },
	{ UED_FSKNOWNFOLDER_SEARCH_TEMPLATES, FOLDERID_SearchTemplates },
	{ UED_FSKNOWNFOLDER_SEND_TO, FOLDERID_SendTo },
	{ UED_FSKNOWNFOLDER_SIDEBAR_DEFAULT_PARTS, FOLDERID_SidebarDefaultParts },
	{ UED_FSKNOWNFOLDER_SIDEBAR_PARTS, FOLDERID_SidebarParts },
	{ UED_FSKNOWNFOLDER_SKYDRIVE, FOLDERID_SkyDrive },
	{ UED_FSKNOWNFOLDER_SKYDRIVE_CAMERAROLL, FOLDERID_SkyDriveCameraRoll },
	{ UED_FSKNOWNFOLDER_SKYDRIVE_DOCUMENTS, FOLDERID_SkyDriveDocuments },
	{ UED_FSKNOWNFOLDER_SKYDRIVE_PICTURES, FOLDERID_SkyDrivePictures },
	{ UED_FSKNOWNFOLDER_START_MENU, FOLDERID_StartMenu },
	{ UED_FSKNOWNFOLDER_STARTUP, FOLDERID_Startup },
	{ UED_FSKNOWNFOLDER_SYNC_MANAGER, FOLDERID_SyncManagerFolder },
	{ UED_FSKNOWNFOLDER_SYNC_RESULTS, FOLDERID_SyncResultsFolder },
	{ UED_FSKNOWNFOLDER_SYNC_SETUP, FOLDERID_SyncSetupFolder },
	{ UED_FSKNOWNFOLDER_SYSTEM, FOLDERID_System },
	{ UED_FSKNOWNFOLDER_SYSTEM_X86, FOLDERID_SystemX86 },
	{ UED_FSKNOWNFOLDER_TEMPLATES, FOLDERID_Templates },
	//{ UED_FSKNOWNFOLDER_TREE_PROPERTIES, FOLDERID_TreeProperties },
	{ UED_FSKNOWNFOLDER_USER_PINNED, FOLDERID_UserPinned },
	{ UED_FSKNOWNFOLDER_USER_PROFILES, FOLDERID_UserProfiles },
	{ UED_FSKNOWNFOLDER_USER_PROGRAM_FILES, FOLDERID_UserProgramFiles },
	{ UED_FSKNOWNFOLDER_USER_PROGRAM_FILES_COMMON, FOLDERID_UserProgramFilesCommon },
	{ UED_FSKNOWNFOLDER_USERS_FILES, FOLDERID_UsersFiles },
	{ UED_FSKNOWNFOLDER_USERS_LIBRARIES, FOLDERID_UsersLibraries },
	{ UED_FSKNOWNFOLDER_VIDEOS, FOLDERID_Videos },
	{ UED_FSKNOWNFOLDER_VIDEOS_LIBRARY, FOLDERID_VideosLibrary },
	{ UED_FSKNOWNFOLDER_WINDOWS, FOLDERID_Windows },
};

int GetKnownFolderList(TSCollection <SKnownFolderEntry> & rList)
{
	rList.freeAll();
	int    ok = 1;
	SString temp_buf;
	for(uint i = 0; i < SIZEOFARRAY(FsKnownFolder_Assoc_Windows); i++) {
		const U64UUIDAssoc & r_inner_entry = FsKnownFolder_Assoc_Windows[i];
		wchar_t * p_path = 0;
		HRESULT hr = SHGetKnownFolderPath(r_inner_entry.Val, 0/*dwFlags*/, 0/*hToken*/, &p_path);
		SKnownFolderEntry * p_new_entry = rList.CreateNewItem();
		p_new_entry->UED = r_inner_entry.Key;
		p_new_entry->Guid = r_inner_entry.Val;
		if(SUCCEEDED(hr)) {
			p_new_entry->PathUtf8.CopyUtf8FromUnicode(p_path, sstrlen(p_path), 1);
			p_new_entry->Result = 1;
		}
		else {
			p_new_entry->Result = 0;
		}
		CoTaskMemFree(p_path);
	}
	return ok;
}

bool GetKnownFolderPath(uint64 uedFolderId, SString & rPath) // @v11.8.3
{
	rPath.Z();
	bool   ok = false;
	const S_GUID * p_guid = 0;
	if(uedFolderId) {
		if(uedFolderId == UED_FSKNOWNFOLDER_TEMPORARY) {
			const char * p_temp_path = getenv("TMP");
			if(SETIFZ(p_temp_path, getenv("TEMP"))) {
				(rPath = p_temp_path).Strip();
				ok = true;
			}
		}
		else {
			for(uint i = 0; !p_guid && i < SIZEOFARRAY(FsKnownFolder_Assoc_Windows); i++) {
				if(FsKnownFolder_Assoc_Windows[i].Key == uedFolderId) {
					p_guid = &FsKnownFolder_Assoc_Windows[i].Val;
				}
			}
			if(p_guid) {
				wchar_t * p_path = 0;
				HRESULT hr = SHGetKnownFolderPath(*p_guid, 0/*dwFlags*/, 0/*hToken*/, &p_path);
				if(SUCCEEDED(hr)) {
					rPath.CopyUtf8FromUnicode(p_path, sstrlen(p_path), 1);
					ok = true;
				}
				CoTaskMemFree(p_path);
			}
		}
	}
	return ok;
	/*
HRESULT SHGetKnownFolderPath(
  [in]           REFKNOWNFOLDERID rfid,
  [in]           DWORD            dwFlags,
  [in, optional] HANDLE           hToken,
  [out]          PWSTR            *ppszPath
);
	*/

		//Constant 	Description

		//FOLDERID_AccountPictures
		//GUID 	{008ca0b1-55b4-4c56-b8a8-4de4b299d3be}
		//Display Name 	Account Pictures
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\AccountPictures
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_AddNewPrograms
		//GUID 	{de61d971-5ebc-4f02-a3a9-6c82895e5c04}
		//Display Name 	Get Programs
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Add New Programs (found in the Add or Remove Programs item in the Control Panel)
		//Legacy Default Path 	Not applicable—virtual folder
		//
		//FOLDERID_AdminTools
		//GUID 	{724EF170-A42D-4FEF-9F26-B60E846FBA4F}
		//Display Name 	Administrative Tools
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Start Menu\Programs\Administrative Tools
		//CSIDL Equivalent 	CSIDL_ADMINTOOLS
		//Legacy Display Name 	Administrative Tools
		//Legacy Default Path 	%USERPROFILE%\Start Menu\Programs\Administrative Tools
		//
		//FOLDERID_AppDataDesktop
		//GUID 	{B2C5E279-7ADD-439F-B28C-C41FE1BBF672}
		//Display Name 	AppDataDesktop
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Desktop
		//CSIDL Equivalent 	None, value introduced in Windows 10, version 1709
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//This FOLDERID is used internally by .NET applications to enable cross-platform app functionality. It is not intended to be used directly from an application.
		//
		//FOLDERID_AppDataDocuments
		//GUID 	{7BE16610-1F7F-44AC-BFF0-83E15F2FFCA1}
		//Display Name 	AppDataDocuments
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Documents
		//CSIDL Equivalent 	None, value introduced in Windows 10, version 1709
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//This FOLDERID is used internally by .NET applications to enable cross-platform app functionality. It is not intended to be used directly from an application.
		//
		//FOLDERID_AppDataFavorites
		//GUID 	{7CFBEFBC-DE1F-45AA-B843-A542AC536CC9}
		//Display Name 	AppDataFavorites
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Favorites
		//CSIDL Equivalent 	None, value introduced in Windows 10, version 1709
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//This FOLDERID is used internally by .NET applications to enable cross-platform app functionality. It is not intended to be used directly from an application.
		//
		//FOLDERID_AppDataProgramData
		//GUID 	{559D40A3-A036-40FA-AF61-84CB430A4D34}
		//Display Name 	AppDataProgramData
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\ProgramData
		//CSIDL Equivalent 	None, value introduced in Windows 10, version 1709
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//This FOLDERID is used internally by .NET applications to enable cross-platform app functionality. It is not intended to be used directly from an application.
		//
		//FOLDERID_ApplicationShortcuts
		//GUID 	{A3918781-E5F2-4890-B3D9-A7E54332328C}
		//Display Name 	Application Shortcuts
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\Application Shortcuts
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_AppsFolder
		//GUID 	{1e87508d-89c2-42f0-8a7e-645a0f50ca58}
		//Display Name 	Applications
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_AppUpdates
		//GUID 	{a305ce99-f527-492b-8b1a-7e76fa98d6e4}
		//Display Name 	Installed Updates
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	None, value introduced in Windows Vista. In earlier versions of Windows, the information on this page was included in Add or Remove Programs if the Show updates box was checked.
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_CameraRoll
		//GUID 	{AB5FB87B-7CE2-4F83-915D-550846C9537B}
		//Display Name 	Camera Roll
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Pictures\Camera Roll
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_CDBurning
		//GUID 	{9E52AB10-F80D-49DF-ACB8-4330F5687855}
		//Display Name 	Temporary Burn Folder
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\Burn\Burn
		//CSIDL Equivalent 	CSIDL_CDBURN_AREA
		//Legacy Display Name 	CD Burning
		//Legacy Default Path 	%USERPROFILE%\Local Settings\Application Data\Microsoft\CD Burning
		//
		//FOLDERID_ChangeRemovePrograms
		//GUID 	{df7266ac-9274-4867-8d55-3bd661de872d}
		//Display Name 	Programs and Features
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Add or Remove Programs
		//Legacy Default Path 	Not applicable—virtual folder
		//
		//FOLDERID_CommonAdminTools
		//GUID 	{D0384E7D-BAC3-4797-8F14-CBA229B392B5}
		//Display Name 	Administrative Tools
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Start Menu\Programs\Administrative Tools
		//CSIDL Equivalent 	CSIDL_COMMON_ADMINTOOLS
		//Legacy Display Name 	Administrative Tools
		//Legacy Default Path 	%ALLUSERSPROFILE%\Start Menu\Programs\Administrative Tools
		//
		//FOLDERID_CommonOEMLinks
		//GUID 	{C1BAE2D0-10DF-4334-BEDD-7AA20B227A9D}
		//Display Name 	OEM Links
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\OEM Links
		//CSIDL Equivalent 	CSIDL_COMMON_OEM_LINKS
		//Legacy Display Name 	OEM Links
		//Legacy Default Path 	%ALLUSERSPROFILE%\OEM Links
		//
		//FOLDERID_CommonPrograms
		//GUID 	{0139D44E-6AFE-49F2-8690-3DAFCAE6FFB8}
		//Display Name 	Programs
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Start Menu\Programs
		//CSIDL Equivalent 	CSIDL_COMMON_PROGRAMS
		//Legacy Display Name 	Programs
		//Legacy Default Path 	%ALLUSERSPROFILE%\Start Menu\Programs
		//
		//FOLDERID_CommonStartMenu
		//GUID 	{A4115719-D62E-491D-AA7C-E74B8BE3B067}
		//Display Name 	Start Menu
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Start Menu
		//CSIDL Equivalent 	CSIDL_COMMON_STARTMENU
		//Legacy Display Name 	Start Menu
		//Legacy Default Path 	%ALLUSERSPROFILE%\Start Menu
		//
		//FOLDERID_CommonStartup
		//GUID 	{82A5EA35-D9CD-47C5-9629-E15D2F714E6E}
		//Display Name 	Startup
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Start Menu\Programs\StartUp
		//CSIDL Equivalent 	CSIDL_COMMON_STARTUP, CSIDL_COMMON_ALTSTARTUP
		//Legacy Display Name 	Startup
		//Legacy Default Path 	%ALLUSERSPROFILE%\Start Menu\Programs\StartUp
		//
		//FOLDERID_CommonTemplates
		//GUID 	{B94237E7-57AC-4347-9151-B08C6C32D1F7}
		//Display Name 	Templates
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Templates
		//CSIDL Equivalent 	CSIDL_COMMON_TEMPLATES
		//Legacy Display Name 	Templates
		//Legacy Default Path 	%ALLUSERSPROFILE%\Templates
		//
		//FOLDERID_ComputerFolder
		//GUID 	{0AC0837C-BBF8-452A-850D-79D08E667CA7}
		//Display Name 	Computer
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_DRIVES
		//Legacy Display Name 	My Computer
		//Legacy Default Path 	Not applicable—virtual folder
		//
		//FOLDERID_ConflictFolder
		//GUID 	{4bfefb45-347d-4006-a5be-ac0cb0567192}
		//Display Name 	Conflicts
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable. This KNOWNFOLDERID refers to the Windows Vista Synchronization Manager. It is not the folder referenced by the older ISyncMgrConflictFolder.
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_ConnectionsFolder
		//GUID 	{6F0CD92B-2E97-45D1-88FF-B0D186B8DEDD}
		//Display Name 	Network Connections
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_CONNECTIONS
		//Legacy Display Name 	Network Connections
		//Legacy Default Path 	Not applicable—virtual folder
		//
		//FOLDERID_Contacts
		//GUID 	{56784854-C6CB-462b-8169-88E350ACB882}
		//Display Name 	Contacts
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Contacts
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_ControlPanelFolder
		//GUID 	{82A74AEB-AEB4-465C-A014-D097EE346D63}
		//Display Name 	Control Panel
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_CONTROLS
		//Legacy Display Name 	Control Panel
		//Legacy Default Path 	Not applicable—virtual folder
		//
		//FOLDERID_Cookies
		//GUID 	{2B0F765D-C0E9-4171-908E-08A611B84FF6}
		//Display Name 	Cookies
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Cookies
		//CSIDL Equivalent 	CSIDL_COOKIES
		//Legacy Display Name 	Cookies
		//Legacy Default Path 	%USERPROFILE%\Cookies
		//
		//FOLDERID_Desktop
		//GUID 	{B4BFCC3A-DB2C-424C-B029-7FE99A87C641}
		//Display Name 	Desktop
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Desktop
		//CSIDL Equivalent 	CSIDL_DESKTOP, CSIDL_DESKTOPDIRECTORY
		//Legacy Display Name 	Desktop
		//Legacy Default Path 	%USERPROFILE%\Desktop
		//
		//FOLDERID_DeviceMetadataStore
		//GUID 	{5CE4A5E9-E4EB-479D-B89F-130C02886155}
		//Display Name 	DeviceMetadataStore
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\DeviceMetadataStore
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_Documents
		//GUID 	{FDD39AD0-238F-46AF-ADB4-6C85480369C7}
		//Display Name 	Documents
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Documents
		//CSIDL Equivalents 	CSIDL_MYDOCUMENTS, CSIDL_PERSONAL
		//Legacy Display Name 	My Documents
		//Legacy Default Path 	%USERPROFILE%\My Documents
		//
		//FOLDERID_DocumentsLibrary
		//GUID 	{7B0DB17D-9CD2-4A93-9733-46CC89022E7C}
		//Display Name 	Documents
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries\Documents.library-ms
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_Downloads
		//GUID 	{374DE290-123F-4565-9164-39C4925E467B}
		//Display Name 	Downloads
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Downloads
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//  
		//FOLDERID_Favorites
		//GUID 	{1777F761-68AD-4D8A-87BD-30B759FA33DD}
		//Display Name 	Favorites
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Favorites
		//CSIDL Equivalent 	CSIDL_FAVORITES, CSIDL_COMMON_FAVORITES
		//Legacy Display Name 	Favorites
		//Legacy Default Path 	%USERPROFILE%\Favorites
		// 
		//FOLDERID_Fonts
		//GUID 	{FD228CB7-AE11-4AE3-864C-16F3910AB8FE}
		//Display Name 	Fonts
		//Folder Type 	FIXED
		//Default Path 	%windir%\Fonts
		//CSIDL Equivalent 	CSIDL_FONTS
		//Legacy Display Name 	Fonts
		//Legacy Default Path 	%windir%\Fonts
		// 
		//FOLDERID_Games
		//Note: This FOLDERID is deprecated in Windows 10, version 1803 and later versions. In these versions, it returns 0x80070057 - E_INVALIDARG
 		//GUID 	{CAC52C1A-B53D-4edc-92D7-6B2E8AC19434}
		//Display Name 	Games
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_GameTasks
		//GUID 	{054FAE61-4DD8-4787-80B6-090220C4B700}
		//Display Name 	GameExplorer
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\GameExplorer
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_History
		//GUID 	{D9DC8A3B-B784-432E-A781-5A1130A75963}
		//Display Name 	History
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\History
		//CSIDL Equivalent 	CSIDL_HISTORY
		//Legacy Display Name 	History
		//Legacy Default Path 	%USERPROFILE%\Local Settings\History
		// 
		//FOLDERID_HomeGroup
		//GUID 	{52528A6B-B9E3-4ADD-B60D-588C2DBA842D}
		//Display Name 	Homegroup
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_HomeGroupCurrentUser
		//GUID 	{9B74B6A3-0DFD-4f11-9E78-5F7800F2E772}
		//Display Name 	The user's username (%USERNAME%)
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_ImplicitAppShortcuts
		//GUID 	{BCB5256F-79F6-4CEE-B725-DC34E402FD46}
		//Display Name 	ImplicitAppShortcuts
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned\ImplicitAppShortcuts
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_InternetCache
		//GUID 	{352481E8-33BE-4251-BA85-6007CAEDCF9D}
		//Display Name 	Temporary Internet Files
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\Temporary Internet Files
		//CSIDL Equivalent 	CSIDL_INTERNET_CACHE
		//Legacy Display Name 	Temporary Internet Files
		//Legacy Default Path 	%USERPROFILE%\Local Settings\Temporary Internet Files
		// 
		//FOLDERID_InternetFolder
		//GUID 	{4D9F7874-4E0C-4904-967B-40B0D20C3E4B}
		//Display Name 	The Internet
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_INTERNET
		//Legacy Display Name 	Internet Explorer
		//Legacy Default Path 	Not applicable—virtual folder
		// 
		//FOLDERID_Libraries
		//GUID 	{1B3EA5DC-B587-4786-B4EF-BD1DC332AEAE}
		//Display Name 	Libraries
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_Links
		//GUID 	{bfb9d5e0-c6a9-404c-b2b2-ae6db6af4968}
		//Display Name 	Links
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Links
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_LocalAppData
		//GUID 	{F1B32785-6FBA-4FCF-9D55-7B8E7F157091}
		//Display Name 	Local
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA% (%USERPROFILE%\AppData\Local)
		//CSIDL Equivalent 	CSIDL_LOCAL_APPDATA
		//Legacy Display Name 	Application Data
		//Legacy Default Path 	%USERPROFILE%\Local Settings\Application Data
		// 
		//FOLDERID_LocalAppDataLow
		//GUID 	{A520A1A4-1780-4FF6-BD18-167343C5AF16}
		//Display Name 	LocalLow
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\AppData\LocalLow
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_LocalizedResourcesDir
		//GUID 	{2A00375E-224C-49DE-B8D1-440DF7EF3DDC}
		//Display Name 	None
		//Folder Type 	FIXED
		//Default Path 	%windir%\resources\0409 (code page)
		//CSIDL Equivalent 	CSIDL_RESOURCES_LOCALIZED
		//Legacy Display Name 	None
		//Legacy Default Path 	%windir%\resources\0409 (code page)
		// 
		//FOLDERID_Music
		//GUID 	{4BD8D571-6D19-48D3-BE97-422220080E43}
		//Display Name 	Music
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Music
		//CSIDL Equivalent 	CSIDL_MYMUSIC
		//Legacy Display Name 	My Music
		//Legacy Default Path 	%USERPROFILE%\My Documents\My Music
		// 
		//FOLDERID_MusicLibrary
		//GUID 	{2112AB0A-C86A-4FFE-A368-0DE96E47012E}
		//Display Name 	Music
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries\Music.library-ms
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_NetHood
		//GUID 	{C5ABBF53-E17F-4121-8900-86626FC2C973}
		//Display Name 	Network Shortcuts
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Network Shortcuts
		//CSIDL Equivalent 	CSIDL_NETHOOD
		//Legacy Display Name 	NetHood
		//Legacy Default Path 	%USERPROFILE%\NetHood
		// 
		//FOLDERID_NetworkFolder
		//GUID 	{D20BEEC4-5CA8-4905-AE3B-BF251EA09B53}
		//Display Name 	Network
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_NETWORK, CSIDL_COMPUTERSNEARME
		//Legacy Display Name 	My Network Places
		//Legacy Default Path 	Not applicable—virtual folder
		// 
		//FOLDERID_Objects3D
		//GUID 	{31C0DD25-9439-4F12-BF41-7FF4EDA38722}
		//Display Name 	3D Objects
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\3D Objects
		//CSIDL Equivalent 	None, value introduced in Windows 10, version 1703
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_OriginalImages
		//GUID 	{2C36C0AA-5812-4b87-BFD0-4CD0DFB19B39}
		//Display Name 	Original Images
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows Photo Gallery\Original Images
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PhotoAlbums
		//GUID 	{69D2CF90-FC33-4FB7-9A0C-EBB0F0FCB43C}
		//Display Name 	Slide Shows
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Pictures\Slide Shows
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PicturesLibrary
		//GUID 	{A990AE9F-A03B-4E80-94BC-9912D7504104}
		//Display Name 	Pictures
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries\Pictures.library-ms
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_Pictures
		//GUID 	{33E28130-4E1E-4676-835A-98395C3BC3BB}
		//Display Name 	Pictures
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Pictures
		//CSIDL Equivalent 	CSIDL_MYPICTURES
		//Legacy Display Name 	My Pictures
		//Legacy Default Path 	%USERPROFILE%\My Documents\My Pictures
		// 
		//FOLDERID_Playlists
		//GUID 	{DE92C1C7-837F-4F69-A3BB-86E631204A23}
		//Display Name 	Playlists
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Music\Playlists
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PrintersFolder
		//GUID 	{76FC4E2D-D6AD-4519-A663-37BD56068185}
		//Display Name 	Printers
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_PRINTERS
		//Legacy Display Name 	Printers and Faxes
		//Legacy Default Path 	Not applicable—virtual folder
		// 
		//FOLDERID_PrintHood
		//GUID 	{9274BD8D-CFD1-41C3-B35E-B13F55A758F4}
		//Display Name 	Printer Shortcuts
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Printer Shortcuts
		//CSIDL Equivalent 	CSIDL_PRINTHOOD
		//Legacy Display Name 	PrintHood
		//Legacy Default Path 	%USERPROFILE%\PrintHood
		// 
		//FOLDERID_Profile
		//GUID 	{5E6C858F-0E22-4760-9AFE-EA3317B67173}
		//Display Name 	The user's username (%USERNAME%)
		//Folder Type 	FIXED
		//Default Path 	%USERPROFILE% (%SystemDrive%\Users\%USERNAME%)
		//CSIDL Equivalent 	CSIDL_PROFILE
		//Legacy Display Name 	The user's username (%USERNAME%)
		//Legacy Default Path 	%USERPROFILE% (%SystemDrive%\Documents and Settings\%USERNAME%)
		// 
		//FOLDERID_ProgramData
		//GUID 	{62AB5D82-FDC1-4DC3-A9DD-070D1D495D97}
		//Display Name 	ProgramData
		//Folder Type 	FIXED
		//Default Path 	%ALLUSERSPROFILE% (%ProgramData%, %SystemDrive%\ProgramData)
		//CSIDL Equivalent 	CSIDL_COMMON_APPDATA
		//Legacy Display Name 	Application Data
		//Legacy Default Path 	%ALLUSERSPROFILE%\Application Data
		// 
		//FOLDERID_ProgramFiles
		//See Remarks for more information.
		//GUID 	{905e63b6-c1bf-494e-b29c-65b732d3d21a}
		//Display Name 	Program Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		//CSIDL Equivalent 	CSIDL_PROGRAM_FILES
		//Legacy Display Name 	Program Files
		//Legacy Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		// 
		//FOLDERID_ProgramFilesX64
		//This value is not supported on 32-bit operating systems. It also is not supported for 32-bit applications running on 64-bit operating systems. Attempting to use FOLDERID_ProgramFilesX64 in either situation results in an error. See Remarks for more information.
		//GUID 	{6D809377-6AF0-444b-8957-A3773F02200E}
		//Display Name 	Program Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Program Files
		//Legacy Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		// 
		//FOLDERID_ProgramFilesX86
		//See Remarks for more information.
		//GUID 	{7C5A40EF-A0FB-4BFC-874A-C0F2E0B9FA8E}
		//Display Name 	Program Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		//CSIDL Equivalent 	CSIDL_PROGRAM_FILESX86
		//Legacy Display Name 	Program Files
		//Legacy Default Path 	%ProgramFiles% (%SystemDrive%\Program Files)
		// 
		//FOLDERID_ProgramFilesCommon
		//See Remarks for more information.
		//GUID 	{F7F1ED05-9F6D-47A2-AAAE-29D317C6F066}
		//Display Name 	Common Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles%\Common Files
		//CSIDL Equivalent 	CSIDL_PROGRAM_FILES_COMMON
		//Legacy Display Name 	Common Files
		//Legacy Default Path 	%ProgramFiles%\Common Files
		// 
		//FOLDERID_ProgramFilesCommonX64
		//See Remarks for more information.
		//GUID 	{6365D5A7-0F0D-45E5-87F6-0DA56B6A4F7D}
		//Display Name 	Common Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles%\Common Files
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Common Files
		//Legacy Default Path 	%ProgramFiles%\Common Files
		//
		//FOLDERID_ProgramFilesCommonX86
		//See Remarks for more information.
		//GUID 	{DE974D24-D9C6-4D3E-BF91-F4455120B917}
		//Display Name 	Common Files
		//Folder Type 	FIXED
		//Default Path 	%ProgramFiles%\Common Files
		//CSIDL Equivalent 	CSIDL_PROGRAM_FILES_COMMONX86
		//Legacy Display Name 	Common Files
		//Legacy Default Path 	%ProgramFiles%\Common Files
		//
		//FOLDERID_Programs
		//GUID 	{A77F5D77-2E2B-44C3-A6A2-ABA601054A51}
		//Display Name 	Programs
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Start Menu\Programs
		//CSIDL Equivalent 	CSIDL_PROGRAMS
		//Legacy Display Name 	Programs
		//Legacy Default Path 	%USERPROFILE%\Start Menu\Programs
		//
		//FOLDERID_Public
		//GUID 	{DFDF76A2-C82A-4D63-906A-5644AC457385}
		//Display Name 	Public
		//Folder Type 	FIXED
		//Default Path 	%PUBLIC% (%SystemDrive%\Users\Public)
		//CSIDL Equivalent 	None, new for Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicDesktop
		//GUID 	{C4AA340D-F20F-4863-AFEF-F87EF2E6BA25}
		//Display Name 	Public Desktop
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Desktop
		//CSIDL Equivalent 	CSIDL_COMMON_DESKTOPDIRECTORY
		//Legacy Display Name 	Desktop
		//Legacy Default Path 	%ALLUSERSPROFILE%\Desktop
		// 
		//FOLDERID_PublicDocuments
		//GUID 	{ED4824AF-DCE4-45A8-81E2-FC7965083634}
		//Display Name 	Public Documents
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Documents
		//CSIDL Equivalent 	CSIDL_COMMON_DOCUMENTS
		//Legacy Display Name 	Shared Documents
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents
		// 
		//FOLDERID_PublicDownloads
		//GUID 	{3D644C9B-1FB8-4f30-9B45-F670235F79C0}
		//Display Name 	Public Downloads
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Downloads
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicGameTasks
		//GUID 	{DEBF2536-E1A8-4c59-B6A2-414586476AEA}
		//Display Name 	GameExplorer
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\GameExplorer
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicLibraries
		//GUID 	{48DAF80B-E6CF-4F4E-B800-0E69D84EE384}
		//Display Name 	Libraries
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Libraries
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicMusic
		//GUID 	{3214FAB5-9757-4298-BB61-92A9DEAA44FF}
		//Display Name 	Public Music
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Music
		//CSIDL Equivalent 	CSIDL_COMMON_MUSIC
		//Legacy Display Name 	Shared Music
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents\My Music
		// 
		//FOLDERID_PublicPictures
		//GUID 	{B6EBFB86-6907-413C-9AF7-4FC2ABF07CC5}
		//Display Name 	Public Pictures
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Pictures
		//CSIDL Equivalent 	CSIDL_COMMON_PICTURES
		//Legacy Display Name 	Shared Pictures
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents\My Pictures
		// 
		//FOLDERID_PublicRingtones
		//GUID 	{E555AB60-153B-4D17-9F04-A5FE99FC15EC}
		//Display Name 	Ringtones
		//Folder Type 	COMMON
		//Default Path 	%ALLUSERSPROFILE%\Microsoft\Windows\Ringtones
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicUserTiles
		//GUID 	{0482af6c-08f1-4c34-8c90-e17ec98b1e17}
		//Display Name 	Public Account Pictures
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\AccountPictures
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_PublicVideos
		//GUID 	{2400183A-6185-49FB-A2D8-4A392A602BA3}
		//Display Name 	Public Videos
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Videos
		//CSIDL Equivalent 	CSIDL_COMMON_VIDEO
		//Legacy Display Name 	Shared Video
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents\My Videos
		// 
		//FOLDERID_QuickLaunch
		//GUID 	{52a4f021-7b75-48a9-9f6b-4b87a210bc8f}
		//Display Name 	Quick Launch
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Internet Explorer\Quick Launch
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Quick Launch
		//Legacy Default Path 	%APPDATA%\Microsoft\Internet Explorer\Quick Launch
		// 
		//FOLDERID_Recent
		//GUID 	{AE50C081-EBD2-438A-8655-8A092E34987A}
		//Display Name 	Recent Items
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Recent
		//CSIDL Equivalent 	CSIDL_RECENT
		//Legacy Display Name 	My Recent Documents
		//Legacy Default Path 	%USERPROFILE%\Recent
		// 
		//FOLDERID_RecordedTV
		//Not used. This value is undefined as of Windows 7.
		// 
		//FOLDERID_RecordedTVLibrary
		//GUID 	{1A6FDBA2-F42D-4358-A798-B74D745926C5}
		//Display Name 	Recorded TV
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\RecordedTV.library-ms
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_RecycleBinFolder
		//GUID 	{B7534046-3ECB-4C18-BE4E-64CD4CB7D6AC}
		//Display Name 	Recycle Bin
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	CSIDL_BITBUCKET
		//Legacy Display Name 	Recycle Bin
		//Legacy Default Path 	Not applicable—virtual folder
		// 
		//FOLDERID_ResourceDir
		//GUID 	{8AD10C31-2ADB-4296-A8F7-E4701232C972}
		//Display Name 	Resources
		//Folder Type 	FIXED
		//Default Path 	%windir%\Resources
		//CSIDL Equivalent 	CSIDL_RESOURCES
		//Legacy Display Name 	Resources
		//Legacy Default Path 	%windir%\Resources
		// 
		//FOLDERID_Ringtones
		//GUID 	{C870044B-F49E-4126-A9C3-B52A1FF411E8}
		//Display Name 	Ringtones
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\Ringtones
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_RoamingAppData
		//GUID 	{3EB685DB-65F9-4CF6-A03A-E3EF65729F3D}
		//Display Name 	Roaming
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA% (%USERPROFILE%\AppData\Roaming)
		//CSIDL Equivalent 	CSIDL_APPDATA
		//Legacy Display Name 	Application Data
		//Legacy Default Path 	%APPDATA% (%USERPROFILE%\Application Data)
		// 
		//FOLDERID_RoamedTileImages
		//GUID 	{AAA8D5A5-F1D6-4259-BAA8-78E7EF60835E}
		//Display Name 	RoamedTileImages
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\RoamedTileImages
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_RoamingTiles
		//GUID 	{00BCFC5A-ED94-4e48-96A1-3F6217F21990}
		//Display Name 	RoamingTiles
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\RoamingTiles
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SampleMusic
		//GUID 	{B250C668-F57D-4EE1-A63C-290EE7D1AA1F}
		//Display Name 	Sample Music
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Music\Sample Music
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Sample Music
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents\My Music\Sample Music
		// 
		//FOLDERID_SamplePictures
		//GUID 	{C4900540-2379-4C75-844B-64E6FAF8716B}
		//Display Name 	Sample Pictures
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Pictures\Sample Pictures
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Sample Pictures
		//Legacy Default Path 	%ALLUSERSPROFILE%\Documents\My Pictures\Sample Pictures
		// 
		//FOLDERID_SamplePlaylists
		//GUID 	{15CA69B3-30EE-49C1-ACE1-6B5EC372AFB5}
		//Display Name 	Sample Playlists
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Music\Sample Playlists
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SampleVideos
		//GUID 	{859EAD94-2E85-48AD-A71A-0969CB56A6CD}
		//Display Name 	Sample Videos
		//Folder Type 	COMMON
		//Default Path 	%PUBLIC%\Videos\Sample Videos
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SavedGames
		//GUID 	{4C5C32FF-BB9D-43b0-B5B4-2D72E54EAAA4}
		//Display Name 	Saved Games
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Saved Games
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SavedPictures
		//GUID 	{3B193882-D3AD-4eab-965A-69829D1FB59F}
		//Display Name 	Saved Pictures
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Pictures\Saved Pictures
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SavedPicturesLibrary
		//GUID 	{E25B5812-BE88-4bd9-94B0-29233477B6C3}
		//Display Name 	Saved Pictures Library
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries\SavedPictures.library-ms
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SavedSearches
		//GUID 	{7d1d3a04-debb-4115-95cf-2f29da2920da}
		//Display Name 	Searches
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Searches
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_Screenshots
		//GUID 	{b7bede81-df94-4682-a7d8-57a52620b86f}
		//Display Name 	Screenshots
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Pictures\Screenshots
		//CSIDL Equivalent 	None, value introduced in Windows 8
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SEARCH_CSC
		//GUID 	{ee32e446-31ca-4aba-814f-a5ebd2fd6d5e}
		//Display Name 	Offline Files
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SearchHistory
		//GUID 	{0D4C3DB6-03A3-462F-A0E6-08924C41B5D4}
		//Display Name 	History
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\ConnectedSearch\History
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SearchHome
		//GUID 	{190337d1-b8ca-4121-a639-6d472d16972a}
		//Display Name 	Search Results
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SEARCH_MAPI
		//GUID 	{98ec0e18-2098-4d44-8644-66979315a281}
		//Display Name 	Microsoft Office Outlook
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SearchTemplates
		//GUID 	{7E636BFE-DFA9-4D5E-B456-D7B39851D8A9}
		//Display Name 	Templates
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows\ConnectedSearch\Templates
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SendTo
		//GUID 	{8983036C-27C0-404B-8F08-102D10DCFD74}
		//Display Name 	SendTo
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\SendTo
		//CSIDL Equivalent 	CSIDL_SENDTO
		//Legacy Display Name 	SendTo
		//Legacy Default Path 	%USERPROFILE%\SendTo
		// 
		//FOLDERID_SidebarDefaultParts
		//GUID 	{7B396E54-9EC5-4300-BE0A-2482EBAE1A26}
		//Display Name 	Gadgets
		//Folder Type 	COMMON
		//Default Path 	%ProgramFiles%\Windows Sidebar\Gadgets
		//CSIDL Equivalent 	None, new for Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SidebarParts
		//GUID 	{A75D362E-50FC-4fb7-AC2C-A8BEAA314493}
		//Display Name 	Gadgets
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Microsoft\Windows Sidebar\Gadgets
		//CSIDL Equivalent 	None, new for Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SkyDrive
		//GUID 	{A52BBA46-E9E1-435f-B3D9-28DAA648C0F6}
		//Display Name 	OneDrive
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\OneDrive
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SkyDriveCameraRoll
		//GUID 	{767E6811-49CB-4273-87C2-20F355E1085B}
		//Display Name 	Camera Roll
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\OneDrive\Pictures\Camera Roll
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SkyDriveDocuments
		//GUID 	{24D89E24-2F19-4534-9DDE-6A6671FBB8FE}
		//Display Name 	Documents
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\OneDrive\Documents
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SkyDrivePictures
		//GUID 	{339719B5-8C47-4894-94C2-D8F77ADD44A6}
		//Display Name 	Pictures
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\OneDrive\Pictures
		//CSIDL Equivalent 	None, value introduced in Windows 8.1
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_StartMenu
		//GUID 	{625B53C3-AB48-4EC1-BA1F-A1EF4146FC19}
		//Display Name 	Start Menu
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Start Menu
		//CSIDL Equivalent 	CSIDL_STARTMENU
		//Legacy Display Name 	Start Menu
		//Legacy Default Path 	%USERPROFILE%\Start Menu
		// 
		//FOLDERID_Startup
		//GUID 	{B97D20BB-F46A-4C97-BA10-5E3608430854}
		//Display Name 	Startup
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Start Menu\Programs\StartUp
		//CSIDL Equivalent 	CSIDL_STARTUP, CSIDL_ALTSTARTUP
		//Legacy Display Name 	Startup
		//Legacy Default Path 	%USERPROFILE%\Start Menu\Programs\StartUp
		// 
		//FOLDERID_SyncManagerFolder
		//GUID 	{43668BF8-C14E-49B2-97C9-747784D784B7}
		//Display Name 	Sync Center
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_SyncResultsFolder
		//GUID 	{289a9a43-be44-4057-a41b-587a76d7e7f9}
		//Display Name 	Sync Results
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_SyncSetupFolder
		//GUID 	{0F214138-B1D3-4a90-BBA9-27CBC0C5389A}
		//Display Name 	Sync Setup
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_System
		//GUID 	{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}
		//Display Name 	System32
		//Folder Type 	FIXED
		//Default Path 	%windir%\system32
		//CSIDL Equivalent 	CSIDL_SYSTEM
		//Legacy Display Name 	system32
		//Legacy Default Path 	%windir%\system32
		//
		//FOLDERID_SystemX86
		//GUID 	{D65231B0-B2F1-4857-A4CE-A8E7C6EA7D27}
		//Display Name 	System32
		//Folder Type 	FIXED
		//Default Path 	%windir%\system32
		//CSIDL Equivalent 	CSIDL_SYSTEMX86
		//Legacy Display Name 	system32
		//Legacy Default Path 	%windir%\system32
		//
		//FOLDERID_Templates
		//GUID 	{A63293E8-664E-48DB-A079-DF759E0509F7}
		//Display Name 	Templates
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Templates
		//CSIDL Equivalent 	CSIDL_TEMPLATES
		//Legacy Display Name 	Templates
		//Legacy Default Path 	%USERPROFILE%\Templates
		//
		//FOLDERID_TreeProperties
		//Not used in Windows Vista. Unsupported as of Windows 7.
		//
		//FOLDERID_UserPinned
		//GUID 	{9E3995AB-1F9C-4F13-B827-48B24B6C7174}
		//Display Name 	User Pinned
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Internet Explorer\Quick Launch\User Pinned
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_UserProfiles
		//GUID 	{0762D272-C50A-4BB0-A382-697DCD729B80}
		//Display Name 	Users
		//Folder Type 	FIXED
		//Default Path 	%SystemDrive%\Users
		//CSIDL Equivalent 	None, new for Windows Vista
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_UserProgramFiles
		//GUID 	{5CD7AEE2-2219-4A67-B85D-6C9CE15660CB}
		//Display Name 	Programs
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Programs
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_UserProgramFilesCommon
		//GUID 	{BCBD3057-CA5C-4622-B42D-BC56DB0AE516}
		//Display Name 	Programs
		//Folder Type 	PERUSER
		//Default Path 	%LOCALAPPDATA%\Programs\Common
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_UsersFiles
		//GUID 	{f3ce0f7c-4901-4acc-8648-d5d44b04ef8f}
		//Display Name 	The user's full name (for instance, Jean Philippe Bagel) entered when the user account was created.
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_UsersLibraries
		//GUID 	{A302545D-DEFF-464b-ABE8-61C8648D939B}
		//Display Name 	Libraries
		//Folder Type 	VIRTUAL
		//Default Path 	Not applicable—virtual folder
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		//
		//FOLDERID_Videos
		//GUID 	{18989B1D-99B5-455B-841C-AB7C74E4DDFC}
		//Display Name 	Videos
		//Folder Type 	PERUSER
		//Default Path 	%USERPROFILE%\Videos
		//CSIDL 	CSIDL_MYVIDEO
		//Legacy Display Name 	My Videos
		//Legacy Default Path 	%USERPROFILE%\My Documents\My Videos
		//
		//FOLDERID_VideosLibrary
		//GUID 	{491E922F-5643-4AF4-A7EB-4E7A138D8174}
		//Display Name 	Videos
		//Folder Type 	PERUSER
		//Default Path 	%APPDATA%\Microsoft\Windows\Libraries\Videos.library-ms
		//CSIDL Equivalent 	None, value introduced in Windows 7
		//Legacy Display Name 	Not applicable
		//Legacy Default Path 	Not applicable
		// 
		//FOLDERID_Windows
		//GUID 	{F38BF404-1D43-42F2-9305-67DE0B28FC23}
		//Display Name 	Windows
		//Folder Type 	FIXED
		//Default Path 	%windir%
		//CSIDL Equivalent 	CSIDL_WINDOWS
		//Legacy Display Name 	WINDOWS
		//Legacy Default Path 	%windir%

	//FOLDERID_AccountPictures;
}

/*static*/int SFileUtil::GetStat(const char * pFileName, Stat * pStat)
{
	EXCEPTVAR(SLibError);
	int    ok = 1; // @v11.2.0 @fix (-1)-->(1)
	Stat   stat;
	SString _file_name(pFileName);
	MEMSZERO(stat);
#ifdef __WIN32__
	HANDLE srchdl = INVALID_HANDLE_VALUE;
	THROW_V(_file_name.NotEmpty(), SLERR_OPENFAULT);
	{
		SStringU _file_name_u;
		LARGE_INTEGER size = {0, 0};
		if(_file_name.IsAscii()) {
			_file_name_u.CopyFromMb_OUTER(_file_name, _file_name.Len());
		}
		else if(_file_name.IsLegalUtf8()) {
			_file_name_u.CopyFromUtf8R(_file_name, 0);
		}
		else {
			_file_name_u.CopyFromMb_OUTER(_file_name, _file_name.Len());
		}
		srchdl = ::CreateFileW(_file_name_u, FILE_READ_ATTRIBUTES|FILE_READ_EA|STANDARD_RIGHTS_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); 
		SLS.SetAddedMsgString(pFileName);
		THROW_V(srchdl != INVALID_HANDLE_VALUE, SLERR_OPENFAULT);
		SFile::GetTime((int)srchdl, &stat.CrtTime, &stat.AccsTime, &stat.ModTime);
		GetFileSizeEx(srchdl, &size);
		stat.Size = size.QuadPart;
	}
	CATCHZOK
	if(srchdl != INVALID_HANDLE_VALUE)
		CloseHandle(srchdl);
#endif
	ASSIGN_PTR(pStat, stat);
	return ok;
}

/*static*/int SFileUtil::GetDiskSpace(const char * pPath, int64 * pTotal, int64 * pAvail)
{
	int    ok = 1;
	ULARGE_INTEGER avail, total, total_free;
	SString path;
	SPathStruc ps(pPath);
	ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, path);
	if(GetDiskFreeSpaceEx(SUcSwitch(path), &avail, &total, &total_free)) {
		ASSIGN_PTR(pTotal, total.QuadPart);
		ASSIGN_PTR(pAvail, avail.QuadPart);
	}
	else {
		ASSIGN_PTR(pTotal, 0);
		ASSIGN_PTR(pAvail, 0);
		ok = SLS.SetOsError();
	}
	return ok;
}

#if 0 // @v11.8.5 Упразднено в пользу GetKnownFolderPath()
/*static*/int SFileUtil::GetSysDir(int pathId, SString & rPath)
{
	/*
	typedef HRESULT (SHFOLDERAPI * SHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
	SHGETFOLDERPATH fSHGetFolderPath = 0;
	*/
	rPath.Z();
	int    ok = 0;
	TCHAR  path[MAX_PATH];
	int    folder = 0;
	// @v11.0.3 {
	if(pathId == sdTemporary) {
		const char * p_temp_path = getenv("TMP");
		if(SETIFZ(p_temp_path, getenv("TEMP"))) {
			(rPath = p_temp_path).Strip();
			ok = 1;
		}
	}
	else { // } @v11.0.3 
		switch(pathId) {
			case sdSystem: folder = CSIDL_WINDOWS; break;
			case sdProgramFiles: folder = CSIDL_PROGRAM_FILES; break;
			case sdProgramFilesCommon: folder = CSIDL_PROGRAM_FILES_COMMON; break;
			case sdWindows: folder = CSIDL_WINDOWS; break;
			case sdAppData: folder = CSIDL_APPDATA; break;
			case sdAppDataLocal: folder = CSIDL_LOCAL_APPDATA; break;
			case sdCommonDocuments: folder = CSIDL_COMMON_DOCUMENTS; break;
		}
		if(folder) {
			if(SUCCEEDED(SHGetFolderPath(0, folder, 0, SHGFP_TYPE_CURRENT, path))) {
				rPath = SUcSwitch(path);
				ok = 1;
			}
		}
	}
	return ok;
}
#endif // } 0 @v11.8.5 Упразднено в пользу GetKnownFolderPath()

#if SLTEST_RUNNING // {
/*
;
; Аргументы:
; 0 - тестовый подкаталог каталога IN
; 1 - подкаталог тестового каталого, в котором находится большое количество файлов
; 2 - количество файлов, находящихся в каталоге, описанном параметром 1
; 3 - суммарный размер файлов, находящихся в каталоге, описанном параметром 1
;
arglist=Test Directory;Test Directory Level 2\Directory With Many Files;1858;470075
benchmark=access;winfileexists
*/

SLTEST_R(Directory)
{
	int    ok = 1;
	SString path = GetSuiteEntry()->InPath;
	SString out_path = GetSuiteEntry()->OutPath;
	SString test_dir, test_dir_with_files;
	SString temp_buf;
	uint   files_count = 0;
	int64  files_size = 0;
	SStrCollection file_list;
	// @v11.3.1 {
	{
		const char * p_path_to_normalize = "D:\\Papyrus\\Src\\BuildVC2017\\..\\..\\ppy\\bin\\..\\..\\src\\pptest\\out\\lmdb-test";
		SPathStruc::NormalizePath(p_path_to_normalize, SPathStruc::npfCompensateDotDot, temp_buf);
		SLCHECK_EQ(temp_buf, "d:\\papyrus\\src\\pptest\\out\\lmdb-test");
		SPathStruc::NormalizePath(p_path_to_normalize, SPathStruc::npfCompensateDotDot|SPathStruc::npfKeepCase, temp_buf);
		SLCHECK_EQ(temp_buf, "D:\\Papyrus\\src\\pptest\\out\\lmdb-test");
		SPathStruc::NormalizePath(p_path_to_normalize, SPathStruc::npfSlash, temp_buf);
		SLCHECK_EQ(temp_buf, "d:/papyrus/src/buildvc2017/../../ppy/bin/../../src/pptest/out/lmdb-test");
		SPathStruc::NormalizePath(p_path_to_normalize, SPathStruc::npfCompensateDotDot|SPathStruc::npfUpper, temp_buf);
		SLCHECK_EQ(temp_buf, "D:\\PAPYRUS\\SRC\\PPTEST\\OUT\\LMDB-TEST");
	}
	// } @v11.3.1 
	{
		uint   arg_no = 0;
		for(uint ap = 0, arg_no = 0; EnumArg(&ap, temp_buf); arg_no++) {
			switch(arg_no) {
				case 0: test_dir = temp_buf; break;
				case 1: test_dir_with_files = temp_buf; break;
				case 2: files_count = temp_buf.ToLong(); break;
				case 3: files_size = temp_buf.ToLong(); break;
			}
		}
	}
	(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir);
	THROW(SLCHECK_NZ(pathValid(path, 1)));
	(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat(test_dir_with_files);
	test_dir_with_files = path;
	THROW(SLCHECK_NZ(pathValid(test_dir_with_files, 1)));
	{
		int64  sz = 0;
		SDirEntry de;
		(temp_buf = test_dir_with_files).SetLastSlash().CatChar('*').Dot().CatChar('*');
		for(SDirec dir(temp_buf, 0); dir.Next(&de) > 0;) {
			if(!de.IsFolder()) {
				sz += de.Size;
				de.GetNameA(test_dir_with_files, temp_buf);
				THROW(SLCHECK_Z(::access(temp_buf, 0)));
				THROW(SLCHECK_NZ(Win_IsFileExists(temp_buf)));
				file_list.insert(newStr(temp_buf));
			}
		}
		THROW(SLCHECK_EQ(file_list.getCount(), files_count));
		THROW(SLCHECK_EQ((long)sz, (long)files_size));
		// @v11.6.5 {
		{
			// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\TDR\Тестовый каталог в кодировке cp1251\"
			// Проверяем наличие каталога с именем русскими буквами (он есть). Проверка и в кодировках utf8 и 1251 (ANSI)
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251");
			SLCHECK_NZ(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_NZ(fileExists(path));
			// Проверяем наличие файла с именем русскими буквами (он есть). Проверка и в кодировках utf8 и 1251 (ANSI)
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251\\тестовый файл в кодировке 1251.txt");
			SLCHECK_NZ(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_NZ(fileExists(path));
			// Вставляем дефисы вместо пробелов - такого файла нет!
			(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat("TDR\\Тестовый каталог в кодировке cp1251\\тестовый-файл-в-кодировке-1251.txt");
			SLCHECK_Z(fileExists(path));
			path.Transf(CTRANSF_UTF8_TO_OUTER);
			SLCHECK_Z(fileExists(path));
		}
		// } @v11.6.5
		//
		if(pBenchmark) {
			if(sstreqi_ascii(pBenchmark, "access")) {
				for(uint i = 0; i < file_list.getCount(); i++) {
					::access(file_list.at(i), 0);
				}
			}
			else if(sstreqi_ascii(pBenchmark, "winfileexists")) {
				for(uint i = 0; i < file_list.getCount(); i++) {
					Win_IsFileExists(file_list.at(i));
				}
			}
		}
	}
	{
		const int64 test_file_size = 1024 * 1024;
		(temp_buf = out_path).SetLastSlash().Cat("тестовый файл с не ansi-символами.txt"); // source-file in utf-8!
		{
			SFile f_out(temp_buf, SFile::mWrite|SFile::mBinary);
			THROW(SLCHECK_NZ(f_out.IsValid()));
			{
				uint8 bin_buf[256];
				for(uint i = 0; i < test_file_size/sizeof(bin_buf); i++) {
					SObfuscateBuffer(bin_buf, sizeof(bin_buf));
					THROW(SLCHECK_NZ(f_out.Write(bin_buf, sizeof(bin_buf))));
				}
			}
		}
		{
			SFileUtil::Stat stat;
			THROW(SLCHECK_NZ(SFileUtil::GetStat(temp_buf, &stat)));
			SLCHECK_EQ(stat.Size, test_file_size);
		}
	}
	CATCHZOK
	return CurrentStatus;
}

#endif // }
