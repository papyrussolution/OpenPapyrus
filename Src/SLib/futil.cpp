// FUTIL.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2007, 2008, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020
//
#include <slib-internal.h>
#pragma hdrstop
#include <shlobj.h> // SHGetFolderPath and constants

SDataMoveProgressInfo::SDataMoveProgressInfo()
{
	THISZERO();
}

int FASTCALL fileExists(const char * pFileName)
{
/* @snippet Пример определения существования файла посредством WINAPI (Shlwapi.h)
#ifdef WIN32
	const DWORD attributes = GetFileAttributesA(path);
	// special directory case to drive the network path check
	const BOOL is_directory = (attributes == INVALID_FILE_ATTRIBUTES) ? (GetLastError() == ERROR_BAD_NETPATH) : (FILE_ATTRIBUTE_DIRECTORY & attributes);
	if(is_directory) {
		if(PathIsNetworkPathA(path)) 
			return true;
		else if(PathIsUNCA(path)) 
			return true;
	}
	if(PathFileExistsA(path) == 1) 
		return true;
#endif*/
	return (!isempty(pFileName) && ::access(pFileName, 0) == 0) ? 1 : SLS.SetError(SLERR_FILENOTFOUND, pFileName);
}

#ifdef __WIN32__

static int getdisk()
{
	wchar_t buf[MAXPATH];
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
		char   namebuf[MAXPATH + sizeof(UNIVERSAL_NAME_INFO)];
		namebuf[0] = 0;
		DWORD  len = MAXPATH;
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

/* @v10.5.6 char * replacePath(char * fileName, const char * newPath, int force)
{
	char   drv[MAXDRIVE], dir[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	fnsplit(fileName, drv, dir, nam, ext);
	if(force || (*drv == 0 && *dir == 0))
		strcat(strcat(setLastSlash(strcpy(fileName, newPath)), nam), ext);
	return fileName;
} */

/* @v10.5.6 char * replaceExt(char * fileName, const char * newExt, int force)
{
	char   drv[MAXDRIVE], dir[MAXPATH], nam[MAXFILE], ext[MAXEXT];
	fnsplit(fileName, drv, dir, nam, ext);
	if(force || *ext == 0) {
		if(newExt[0] != '.') {
			ext[0] = '.';
			strnzcpy(ext+1, newExt, sizeof(ext)-1);
		}
		else
			strnzcpy(ext, newExt, sizeof(ext));
		fnmerge(fileName, drv, dir, nam, ext);
	}
	return fileName;
}*/

char * setLastSlash(char * p)
{
	size_t len = sstrlen(p);
	if(len > 0 && !oneof2(p[len-1], '\\', '/')) {
		p[len++] = '\\';
		p[len] = 0;
	}
	return p;
}

char * rmvLastSlash(char * p)
{
	const size_t len = sstrlen(p);
	if(len > 0 && oneof2(p[len-1], '\\', '/'))
		p[len-1] = 0;
	return p;
}

SString & getExecPath(SString & rBuf)
{
	SPathStruc ps(SLS.GetExePath());
	ps.Merge(0, SPathStruc::fNam|SPathStruc::fExt, rBuf);
	return rBuf;
}

#pragma warn -asc

int driveValid(const char * pPath)
{
	int    ok = 0;
	char   dname[4] = "X:\\";
	dname[0] = *pPath;
	uint t = GetDriveType(SUcSwitch(dname));
	if(t != DRIVE_UNKNOWN && t != DRIVE_NO_ROOT_DIR)
		ok = 1;
	else if(pPath[0] == '\\' && pPath[1] == '\\') {
		char   buf[MAXPATH], *p;
		fnsplit(pPath, 0, buf, 0, 0);
		p = ((p = sstrchr(buf+2, '\\')) ? sstrchr(p+1, '\\') : 0);
		if(p) {
			*p = 0;
			setLastSlash(buf);
			ok = GetVolumeInformation(SUcSwitch(buf), 0, 0, 0, 0, 0, 0, 0);
		}
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
			SString temp_buf;
			(temp_buf = pStr).Strip().SetLastSlash().CatChar('*');
			h = FindFirstFile(SUcSwitch(temp_buf), &fd);
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

/* @v10.8.2 (unused) static char * fexpand(char * rpath)
{
#ifdef __WIN32__
	TCHAR * fn = 0;
	TCHAR  buf[MAXPATH];
	::GetFullPathName(SUcSwitch(rpath), SIZEOFARRAY(buf), buf, &fn);
	return strcpy(rpath, SUcSwitch(buf));
#else
	char path[MAXPATH];
	char drive[MAXDRIVE];
	char dir[MAXDIR];
	char file[MAXFILE];
	char ext[MAXEXT];
	char curdir[MAXDIR];
	char * p = dir;
	int flags = fnsplit(rpath, drive, dir, file, ext);
	if((flags & FNF_DRIVE) == 0) {
		drive[0] = getdisk() + 'A';
		drive[1] = ':';
		drive[2] = '\0';
	}
	else
		drive[0] = toupper(drive[0]);
	if(!(flags & FNF_DIRECTORY) || (dir[0] != '\\' && dir[0] != '/')) {
		getcurdir(drive[0] - 'A' + 1, curdir);
		strcat(curdir, dir);
		if(*curdir != '\\' && *curdir != '/') {
			*dir = '\\';
			strcpy(dir + 1, curdir);
		}
		else
			strcpy(dir, curdir);
	}
	while((p = sstrchr(p, '/')) != 0)
		*p = '\\';
	fnmerge(path, drive, squeeze(dir), file, ext);
	return strcpy(rpath, strupr(path));
#endif
}*/

int pathValid(const char * pPath, int existOnly)
{
	SString exp_path(pPath);
	{
		TCHAR * fn = 0;
		TCHAR  fpn_buf[MAXPATH];
		::GetFullPathName(SUcSwitch(exp_path), SIZEOFARRAY(fpn_buf), fpn_buf, &fn);
		exp_path = SUcSwitch(fpn_buf);
	}
	return (exp_path.Len() <= 3) ? driveValid(exp_path) : (existOnly ? IsDirectory(exp_path.RmvLastSlash()) : 1);
}

/* @v10.8.2 (unused)
int validFileName(const char * pFileName)
{
	static const char * illegalChars = ";,=+<>|\"[] \\";
	char   path[MAXPATH];
	char   dir[MAXDIR];
	char   name[MAXFILE];
	char   ext[MAXEXT];
	fnsplit(pFileName, path, dir, name, ext);
	return ((*dir && !pathValid(strcat(path, dir), 1)) || strpbrk(name, illegalChars) ||
		strpbrk(ext + 1, illegalChars) || sstrchr(ext + 1, '.')) ? 0 : 1;
}*/

SString & FASTCALL MakeTempFileName(const char * pDir, const char * pPrefix, const char * pExt, long * pStart, SString & rBuf)
{
	char   prefix[128], ext[128];
	size_t prefix_len = 0;
	long   start = pStart ? *pStart : 1;
	if(pPrefix)
		prefix_len = sstrlen(strnzcpy(prefix, pPrefix, 20)); // @v9.9.12 6-->20
	else
		prefix[0] = 0;
	const uint nd = (prefix_len <= 6) ? (8-prefix_len) : 4;
	if(pExt)
		if(pExt[0] == '.')
			strnzcpy(ext, pExt+1, sizeof(ext));
		else
			strnzcpy(ext, pExt, sizeof(ext));
	else
		ext[0] = 0;
	for(rBuf.Z(); rBuf.Empty() && start < 9999999L;) {
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
	(temp_path = pPath).SetLastSlash().ReplaceChar('/', '\\');
	const char * p = temp_path;
	do {
		if(*p == '\\') {
			if(p[1] == '\\')
				path.CatChar(*p++);
			else if(path.NotEmpty()) {
				int    is_root = 0;
				if(path[0] == path[1] && path[0] == '\\' && !sstrchr(path+2, '\\'))
					is_root = 1;
				if(!is_root && (path[0] && ::access(path, 0) != 0))
					if(::CreateDirectory(SUcSwitch(path), NULL) == 0) {
						SLS.SetAddedMsgString(path);
						ok = (SLibError = SLERR_MKDIRFAULT, 0);
					}
					else
						ok = 1;
				}
			}
		path.CatChar(*p);
	} while(ok && *p++ != 0);
	return ok;
}

int FASTCALL IsWild(const char * f)
{
	return BIN(f && strpbrk(f, "*?") != 0);
}

SString & makeExecPathFileName(const char * pName, const char * pExt, SString & rPath)
{
	HMODULE h_inst = SLS.GetHInst();
	// @v10.3.9 char   drv[MAXDRIVE], dir[MAXDIR];
	// @v10.3.9 char   path[MAXPATH];
	// @v10.3.9 GetModuleFileName(h_inst, path, sizeof(path)); // @unicodeproblem
	// @v10.3.9 fnsplit(path, drv, dir, 0, 0);
	// @v10.3.9 fnmerge(path, drv, dir, pName, pExt);
	// @v10.3.9 rPath = path;
	// @v10.3.9 {
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
	unsigned day   : 5;  /* Days */
	unsigned month : 4;  /* Months */
	unsigned year  : 7;  /* Year - 1980 */
};

struct fat_time {
	unsigned tsec  : 5;  /* Two seconds */
	unsigned min   : 6;  /* Minutes */
	unsigned hour  : 5;  /* Hours */
};

void decode_fat_datetime(uint16 fd, uint16 ft, LDATETIME * dt)
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

void encode_fat_datetime(uint16 * fd, uint16 * ft, const LDATETIME * dt)
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
	if(r == 0)
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
	void  * p_buf  = 0;
	uint32 flen;
	uint32 buflen = SMEGABYTE(4);
	uint32 len, bytes_read_write;
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
				(path = pDir).SetLastSlash().Cat(de.FileName);
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

/*static*/int SFileUtil::GetStat(const char * pFileName, Stat * pStat)
{
	EXCEPTVAR(SLibError);
	int    ok = -1;
	Stat stat;
	MEMSZERO(stat);
#ifdef __WIN32__
	LARGE_INTEGER size;
	HANDLE srchdl = ::CreateFile(SUcSwitch(pFileName), FILE_READ_ATTRIBUTES|FILE_READ_EA|STANDARD_RIGHTS_READ, 
		FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0); 
	SLS.SetAddedMsgString(pFileName);
	THROW_V(srchdl != INVALID_HANDLE_VALUE, SLERR_OPENFAULT);
	SFile::GetTime((int)srchdl, &stat.CrtTime, &stat.AccsTime, &stat.ModTime);
	GetFileSizeEx(srchdl, &size);
	stat.Size = size.QuadPart;
	if(srchdl != INVALID_HANDLE_VALUE)
		CloseHandle(srchdl);
	CATCHZOK
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

/*static*/int SFileUtil::GetSysDir(int pathId, SString & rPath)
{
	/*
	typedef HRESULT (SHFOLDERAPI * SHGETFOLDERPATH)(HWND, int, HANDLE, DWORD, LPTSTR);
	SHGETFOLDERPATH fSHGetFolderPath = 0;
	*/
	rPath = 0;
	int    ok = 0;
	TCHAR  path[MAXPATH];
	int    folder = 0;
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
	return ok;
}

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
	THROW(SLTEST_CHECK_NZ(pathValid(path, 1)));
	(path = GetSuiteEntry()->InPath).SetLastSlash().Cat(test_dir).SetLastSlash().Cat(test_dir_with_files);
	test_dir_with_files = path;
	THROW(SLTEST_CHECK_NZ(pathValid(test_dir_with_files, 1)));
	{
		int64  sz = 0;
		SDirEntry de;
		(temp_buf = test_dir_with_files).SetLastSlash().CatChar('*').Dot().CatChar('*');
		for(SDirec dir(temp_buf, 0); dir.Next(&de) > 0;) {
			if(!de.IsFolder()) {
				sz += de.Size;
				(temp_buf = test_dir_with_files).SetLastSlash().Cat(de.FileName);
				THROW(SLTEST_CHECK_Z(::access(temp_buf, 0)));
				THROW(SLTEST_CHECK_NZ(Win_IsFileExists(temp_buf)));
				file_list.insert(newStr(temp_buf));
			}
		}
		THROW(SLTEST_CHECK_EQ(file_list.getCount(), files_count));
		THROW(SLTEST_CHECK_EQ((long)sz, (long)files_size));
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
		(temp_buf = out_path).SetLastSlash().Cat("тестовый файл с не ansi-символами.txt");
		{
			SFile f_out(temp_buf, SFile::mWrite|SFile::mBinary);
			THROW(SLTEST_CHECK_NZ(f_out.IsValid()));
			{
				uint8 bin_buf[256];
				for(uint i = 0; i < test_file_size/sizeof(bin_buf); i++) {
					SLS.GetTLA().Rg.ObfuscateBuffer(bin_buf, sizeof(bin_buf));
					THROW(SLTEST_CHECK_NZ(f_out.Write(bin_buf, sizeof(bin_buf))));
				}
			}
		}
		{
			SFileUtil::Stat stat;
			THROW(SLTEST_CHECK_NZ(SFileUtil::GetStat(temp_buf, &stat)));
			SLTEST_CHECK_EQ(stat.Size, test_file_size);
		}
	}
	CATCHZOK
	return CurrentStatus;
}

#endif // }
