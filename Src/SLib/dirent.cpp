// DIRENT.CPP
// @codepage UTF-8
// Реализация POSIX-dirent для Windows
//
#include <slib-internal.h>
#pragma hdrstop
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

/* For compatibility with Symbian */
#define wdirent _wdirent
#define WDIR _WDIR
#define wopendir _wopendir
#define wreaddir _wreaddir
#define wclosedir _wclosedir
#define wrewinddir _wrewinddir

/* Compatibility with older Microsoft compilers and non-Microsoft compilers */
#if !defined(_MSC_VER) || _MSC_VER < 1400
	#define wcstombs_s dirent_wcstombs_s
	#define mbstowcs_s dirent_mbstowcs_s

	static int  dirent_mbstowcs_s(size_t * pReturnValue, wchar_t * wcstr, size_t sizeInWords, const char * mbstr, size_t count);
	static int  dirent_wcstombs_s(size_t * pReturnValue, char * mbstr, size_t sizeInBytes, const wchar_t * wcstr, size_t count);
	static void dirent_set_errno(int error);
#else
	#define dirent_set_errno _set_errno /* Optimize dirent_set_errno() away on modern Microsoft compilers */
#endif
/* Internal utility functions */
static WIN32_FIND_DATAW * dirent_first(_WDIR * dirp);
static WIN32_FIND_DATAW * dirent_next(_WDIR * dirp);
/*
 * Open directory stream DIRNAME for read and return a pointer to the
 * internal working area that is used to retrieve individual directory
 * entries.
 */
_WDIR * _wopendir(const wchar_t * dirname)
{
	wchar_t * p;
	// Must have directory name 
	if(dirname == NULL || dirname[0] == '\0') {
		dirent_set_errno(ENOENT);
		return NULL;
	}
	// Allocate new _WDIR structure 
	_WDIR * dirp = (_WDIR *)SAlloc::M(sizeof(struct _WDIR));
	if(!dirp)
		return NULL;
	/* Reset _WDIR structure */
	dirp->handle = INVALID_HANDLE_VALUE;
	dirp->patt = NULL;
	dirp->cached = 0;
	/*
	 * Compute the length of full path plus zero terminator
	 *
	 * Note that on WinRT there's no way to convert relative paths
	 * into absolute paths, so just assume it is an absolute path.
	 */
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	// Desktop 
	DWORD n = GetFullPathNameW(dirname, 0, NULL, NULL);
#else
	// WinRT 
	size_t n = wcslen(dirname);
#endif
	// Allocate room for absolute directory name and search pattern 
	dirp->patt = (wchar_t*)SAlloc::M(sizeof(wchar_t) * n + 16);
	if(dirp->patt == NULL)
		goto exit_closedir;
	/*
	 * Convert relative directory name to an absolute one.  This
	 * allows rewinddir() to function correctly even when current
	 * working directory is changed between opendir() and rewinddir().
	 *
	 * Note that on WinRT there's no way to convert relative paths
	 * into absolute paths, so just assume it is an absolute path.
	 */
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
	/* Desktop */
	n = GetFullPathNameW(dirname, n, dirp->patt, NULL);
	if(n <= 0)
		goto exit_closedir;
#else
	/* WinRT */
	// @sobolev wcsncpy_s(dirp->patt, n+1, dirname, n);
	sstrcpy(dirp->patt, dirname); // @sobolev
#endif
	/* Append search pattern \* to the directory name */
	p = dirp->patt + n;
	switch(p[-1]) {
		case '\\':
		case '/':
		case ':':
		    /* Directory ends in path separator, e.g. c:\temp\ */
		    /*NOP*/;
		    break;
		default:
		    /* Directory name doesn't end in path separator */
		    *p++ = '\\';
	}
	*p++ = '*';
	*p = '\0';
	/* Open directory stream and retrieve the first entry */
	if(!dirent_first(dirp))
		goto exit_closedir;
	/* Success */
	return dirp;
	/* Failure */
exit_closedir:
	_wclosedir(dirp);
	return NULL;
}
/*
 * Read next directory entry.
 *
 * Returns pointer to static directory entry which may be overwritten by
 * subsequent calls to _wreaddir().
 */
struct _wdirent * _wreaddir(_WDIR * dirp)
{
	/*
	 * Read directory entry to buffer.  We can safely ignore the return
	 * value as entry will be set to NULL in case of error.
	 */
	struct _wdirent * entry;
	(void)_wreaddir_r(dirp, &dirp->ent, &entry);
	return entry; /* Return pointer to statically allocated directory entry */
}
/*
 * Read next directory entry.
 *
 * Returns zero on success.  If end of directory stream is reached, then sets
 * result to NULL and returns zero.
 */
int _wreaddir_r(_WDIR * dirp, struct _wdirent * entry, struct _wdirent ** result)
{
	/* Read next directory entry */
	WIN32_FIND_DATAW * datap = dirent_next(dirp);
	if(!datap) {
		/* Return NULL to indicate end of directory */
		*result = NULL;
		return /*OK*/ 0;
	}
	/*
	 * Copy file name as wide-character string.  If the file name is too
	 * long to fit in to the destination buffer, then truncate file name
	 * to PATH_MAX characters and zero-terminate the buffer.
	 */
	size_t n = 0;
	while(n < PATH_MAX && datap->cFileName[n] != 0) {
		entry->d_name[n] = datap->cFileName[n];
		n++;
	}
	entry->d_name[n] = 0;
	/* Length of file name excluding zero terminator */
	entry->d_namlen = n;
	/* File type */
	DWORD attr = datap->dwFileAttributes;
	if((attr & FILE_ATTRIBUTE_DEVICE) != 0)
		entry->d_type = DT_CHR;
	else if((attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
		entry->d_type = DT_DIR;
	else
		entry->d_type = DT_REG;
	/* Reset dummy fields */
	entry->d_ino = 0;
	entry->d_off = 0;
	entry->d_reclen = sizeof(struct _wdirent);
	/* Set result address */
	*result = entry;
	return /*OK*/ 0;
}
/*
 * Close directory stream opened by opendir() function.  This invalidates the
 * DIR structure as well as any directory entry read previously by
 * _wreaddir().
 */
static int _wclosedir(_WDIR * dirp)
{
	if(!dirp) {
		dirent_set_errno(EBADF);
		return -1; /*failure*/
	}
	else {
		/* Release search handle */
		if(dirp->handle != INVALID_HANDLE_VALUE)
			FindClose(dirp->handle);
		SAlloc::F(dirp->patt); /* Release search pattern */
		SAlloc::F(dirp); /* Release directory structure */
		return 0; /*success*/
	}
}
/*
 * Rewind directory stream such that _wreaddir() returns the very first
 * file name again.
 */
static void _wrewinddir(_WDIR* dirp)
{
	if(dirp) {
		/* Release existing search handle */
		if(dirp->handle != INVALID_HANDLE_VALUE)
			FindClose(dirp->handle);
		dirent_first(dirp); /* Open new search handle */
	}
}

/* Get first directory entry */
static WIN32_FIND_DATAW * dirent_first(_WDIR * dirp)
{
	if(!dirp)
		return NULL;
	/* Open directory and retrieve the first entry */
	dirp->handle = FindFirstFileExW(dirp->patt, FindExInfoStandard, &dirp->data, FindExSearchNameMatch, NULL, 0);
	if(dirp->handle == INVALID_HANDLE_VALUE)
		goto error;
	/* A directory entry is now waiting in memory */
	dirp->cached = 1;
	return &dirp->data;
error:
	/* Failed to open directory: no directory entry in memory */
	dirp->cached = 0;
	DWORD errorcode = GetLastError(); /* Set error code */
	switch(errorcode) {
		case ERROR_ACCESS_DENIED: dirent_set_errno(EACCES); break; /* No read access to directory */
		case ERROR_DIRECTORY: dirent_set_errno(ENOTDIR); break; /* Directory name is invalid */
		case ERROR_PATH_NOT_FOUND:
		default: dirent_set_errno(ENOENT); /* Cannot find the file */
	}
	return NULL;
}

/* Get next directory entry */
static WIN32_FIND_DATAW * dirent_next(_WDIR * dirp)
{
	/* Is the next directory entry already in cache? */
	if(dirp->cached) {
		/* Yes, a valid directory entry found in memory */
		dirp->cached = 0;
		return &dirp->data;
	}
	/* No directory entry in cache */
	if(dirp->handle == INVALID_HANDLE_VALUE)
		return NULL;
	/* Read the next directory entry from stream */
	if(FindNextFileW(dirp->handle, &dirp->data) == FALSE)
		goto exit_close;
	return &dirp->data; /* Success */
	/* Failure */
exit_close:
	FindClose(dirp->handle);
	dirp->handle = INVALID_HANDLE_VALUE;
	return NULL;
}
//
// Open directory stream using plain old C-string 
//
DIR * opendir(const char * dirname)
{
	/* Must have directory name */
	if(dirname == NULL || dirname[0] == '\0') {
		dirent_set_errno(ENOENT);
		return NULL;
	}
	/* Allocate memory for DIR structure */
	struct DIR * dirp = (DIR *)SAlloc::M(sizeof(struct DIR));
	if(!dirp)
		return NULL;
	/* Convert directory name to wide-character string */
	wchar_t wname[PATH_MAX + 1];
	size_t n;
	int error = mbstowcs_s(&n, wname, PATH_MAX + 1, dirname, PATH_MAX+1);
	if(error)
		goto exit_failure;
	/* Open directory stream using wide-character name */
	dirp->wdirp = _wopendir(wname);
	if(!dirp->wdirp)
		goto exit_failure;
	return dirp; /* Success */
	/* Failure */
exit_failure:
	SAlloc::F(dirp);
	return NULL;
}
//
// Read next directory entry 
//
struct dirent * readdir(DIR * dirp)
{
	// 
	// Read directory entry to buffer.  We can safely ignore the return
	// value as entry will be set to NULL in case of error.
	// 
	struct dirent * entry;
	readdir_r(dirp, &dirp->ent, &entry);
	// Return pointer to statically allocated directory entry 
	return entry;
}
// 
// Read next directory entry into called-allocated buffer.
// 
// Returns zero on success.  If the end of directory stream is reached, then
// sets result to NULL and returns zero.
// 
int readdir_r(DIR * dirp, struct dirent * entry, struct dirent ** result)
{
	/* Read next directory entry */
	WIN32_FIND_DATAW * datap = dirent_next(dirp->wdirp);
	if(!datap) {
		/* No more directory entries */
		*result = NULL;
		return /*OK*/ 0;
	}
	/* Attempt to convert file name to multi-byte string */
	size_t n;
	int error = wcstombs_s(&n, entry->d_name, PATH_MAX + 1, datap->cFileName, PATH_MAX + 1);
	/*
	 * If the file name cannot be represented by a multi-byte string, then
	 * attempt to use old 8+3 file name.  This allows the program to
	 * access files although file names may seem unfamiliar to the user.
	 *
	 * Be ware that the code below cannot come up with a short file name
	 * unless the file system provides one.  At least VirtualBox shared
	 * folders fail to do this.
	 */
	if(error && datap->cAlternateFileName[0] != '\0') {
		error = wcstombs_s(&n, entry->d_name, PATH_MAX + 1, datap->cAlternateFileName, PATH_MAX + 1);
	}
	if(!error) {
		/* Length of file name excluding zero terminator */
		entry->d_namlen = n - 1;
		/* File attributes */
		DWORD attr = datap->dwFileAttributes;
		if((attr & FILE_ATTRIBUTE_DEVICE) != 0)
			entry->d_type = DT_CHR;
		else if((attr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			entry->d_type = DT_DIR;
		else
			entry->d_type = DT_REG;
		/* Reset dummy fields */
		entry->d_ino = 0;
		entry->d_off = 0;
		entry->d_reclen = sizeof(struct dirent);
	}
	else {
		/*
		 * Cannot convert file name to multi-byte string so construct
		 * an erroneous directory entry and return that.  Note that
		 * we cannot return NULL as that would stop the processing
		 * of directory entries completely.
		 */
		entry->d_name[0] = '?';
		entry->d_name[1] = '\0';
		entry->d_namlen = 1;
		entry->d_type = DT_UNKNOWN;
		entry->d_ino = 0;
		entry->d_off = -1;
		entry->d_reclen = 0;
	}
	/* Return pointer to directory entry */
	*result = entry;
	return /*OK*/ 0;
}
// 
// Close directory stream 
// 
int closedir(DIR * dirp)
{
	int ok;
	if(!dirp)
		goto exit_failure;
	/* Close wide-character directory stream */
	ok = _wclosedir(dirp->wdirp);
	dirp->wdirp = NULL;
	/* Release multi-byte character version */
	SAlloc::F(dirp);
	return ok;
exit_failure:
	/* Invalid directory stream */
	dirent_set_errno(EBADF);
	return /*failure*/ -1;
}
// 
// Rewind directory stream to beginning 
// 
void rewinddir(DIR* dirp)
{
	if(dirp)
		_wrewinddir(dirp->wdirp); /* Rewind wide-character string directory stream */
}
// 
// Scan directory for entries 
// 
int scandir(const char * dirname, struct dirent *** namelist, int (*filter)(const struct dirent*), int (* compare)(const struct dirent**, const struct dirent**))
{
	int result;
	/* Open directory stream */
	DIR * dir = opendir(dirname);
	if(!dir) {
		/* Cannot open directory */
		return /*Error*/ -1;
	}
	/* Read directory entries to memory */
	struct dirent * tmp = NULL;
	struct dirent ** files = NULL;
	size_t size = 0;
	size_t allocated = 0;
	while(1) {
		/* Allocate room for a temporary directory entry */
		if(!tmp) {
			tmp = (struct dirent *)SAlloc::M(sizeof(struct dirent));
			if(!tmp)
				goto exit_failure;
		}
		/* Read directory entry to temporary area */
		struct dirent * entry;
		if(readdir_r(dir, tmp, &entry) != /*OK*/ 0)
			goto exit_failure;
		/* Stop if we already read the last directory entry */
		if(entry == NULL)
			goto exit_success;
		/* Determine whether to include the entry in results */
		if(filter && !filter(tmp))
			continue;
		/* Enlarge pointer table to make room for another pointer */
		if(size >= allocated) {
			/* Compute number of entries in the new table */
			size_t num_entries = size * 2 + 16;
			/* Allocate new pointer table or enlarge existing */
			void * p = SAlloc::R(files, sizeof(void *) * num_entries);
			if(!p)
				goto exit_failure;
			/* Got the memory */
			files = (dirent**)p;
			allocated = num_entries;
		}
		/* Store the temporary entry to ptr table */
		files[size++] = tmp;
		tmp = NULL;
	}
exit_failure:
	/* Release allocated file entries */
	for(size_t i = 0; i < size; i++) {
		SAlloc::F(files[i]);
	}
	ZFREE(files); /* Release the pointer table */
	result = /*error*/ -1; /* Exit with error code */
	goto exit_status;
exit_success:
	/* Sort directory entries */
	qsort(files, size, sizeof(void *), (int (*)(const void *, const void *))compare);
	/* Pass pointer table to caller */
	if(namelist)
		*namelist = files;
	result = (int)size; /* Return the number of directory entries read */
exit_status:
	SAlloc::F(tmp); /* Release temporary directory entry, if we had one */
	closedir(dir); /* Close directory stream */
	return result;
}

// Alphabetical sorting 
int alphasort(const struct dirent ** a, const struct dirent ** b) { return strcoll((*a)->d_name, (*b)->d_name); }

// Sort versions 
int versionsort(const struct dirent ** a, const struct dirent ** b) { return strverscmp((*a)->d_name, (*b)->d_name); }
//
// Compare strings 
//
int strverscmp(const char * a, const char * b)
{
	size_t i = 0;
	size_t j;
	// Find first difference 
	while(a[i] == b[i]) {
		if(a[i] == '\0') {
			return 0; // No difference 
		}
		++i;
	}
	// Count backwards and find the leftmost digit 
	j = i;
	while(j > 0 && isdigit(a[j-1])) {
		--j;
	}
	// Determine mode of comparison 
	if(a[j] == '0' || b[j] == '0') {
		// Find the next non-zero digit 
		while(a[j] == '0' && a[j] == b[j]) {
			j++;
		}
		// String with more digits is smaller, e.g 002 < 01 
		if(isdigit(a[j])) {
			if(!isdigit(b[j])) {
				return -1;
			}
		}
		else if(isdigit(b[j])) {
			return 1;
		}
	}
	else if(isdigit(a[j]) && isdigit(b[j])) {
		// Numeric comparison 
		size_t k1 = j;
		size_t k2 = j;
		// Compute number of digits in each string 
		while(isdigit(a[k1])) {
			k1++;
		}
		while(isdigit(b[k2])) {
			k2++;
		}
		// Number with more digits is bigger, e.g 999 < 1000 
		if(k1 < k2)
			return -1;
		else if(k1 > k2)
			return 1;
	}
	// Alphabetical comparison 
	return (int)((uchar)a[i]) - ((uchar)b[i]);
}
#if !defined(_MSC_VER) || _MSC_VER < 1400
	//
	// Convert multi-byte string to wide character string 
	//
	static int dirent_mbstowcs_s(size_t * pReturnValue, wchar_t * wcstr, size_t sizeInWords, const char * mbstr, size_t count)
	{
		// Older Visual Studio or non-Microsoft compiler 
		size_t n = mbstowcs(wcstr, mbstr, sizeInWords);
		if(wcstr && n >= count)
			return /*error*/ 1;
		/* Zero-terminate output buffer */
		if(wcstr && sizeInWords) {
			if(n >= sizeInWords)
				n = sizeInWords - 1;
			wcstr[n] = 0;
		}
		/* Length of multi-byte string with zero terminator */
		if(pReturnValue) {
			*pReturnValue = n + 1;
		}
		return 0; /* Success */
	}
	//
	// Convert wide-character string to multi-byte string 
	//
	static int dirent_wcstombs_s(size_t * pReturnValue, char * mbstr, size_t sizeInBytes, const wchar_t * wcstr, size_t count)
	{
		/* Older Visual Studio or non-Microsoft compiler */
		size_t n = wcstombs(mbstr, wcstr, sizeInBytes);
		if(mbstr && n >= count)
			return /*error*/ 1;
		/* Zero-terminate output buffer */
		if(mbstr && sizeInBytes) {
			if(n >= sizeInBytes) {
				n = sizeInBytes - 1;
			}
			mbstr[n] = '\0';
		}
		/* Length of resulting multi-bytes string WITH zero-terminator */
		if(pReturnValue) {
			*pReturnValue = n + 1;
		}
		return 0; /* Success */
	}
	//
	// Set errno variable 
	//
	static void dirent_set_errno(int error)
	{
		errno = error; /* Non-Microsoft compiler or older Microsoft compiler */
	}
#endif

#ifdef __cplusplus
}
#endif

#if SLTEST_RUNNING // {

#include <locale.h>
#include <direct.h>

static int T_Dirent(STestCase & rCase, const char * pBaseDir)
{
	SString dir_buf;
	SString temp_buf;
	// Basic directory retrieval 
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); // Open directory 
		if(dir == NULL) {
			temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found");
			rCase.SetInfo(temp_buf, 0);
		}
		else {
			// Read entries 
			struct dirent * ent;
			int found = 0;
			while((ent = readdir(dir)) != NULL) {
				// Check each file 
				if(strcmp(ent->d_name, ".") == 0) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 1);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 1);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 1;
				}
				else if(strcmp(ent->d_name, "..") == 0) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 2);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 2);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 2;
				}
				else if(strcmp(ent->d_name, "file") == 0) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 4);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 4);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(strcmp(ent->d_name, "dir") == 0) {
					/* Just a directory */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 3);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 3);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					slfprintf_stderr("Unexpected file %s\n", ent->d_name);
					abort();
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
			closedir(dir);
		}
	}
	// Function opendir() fails if directory doesn't exist 
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("invalid")); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, (long)ENOENT);
	}
	/* Function opendir() fails if pathname is really a file */
	{
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1/file")); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, (long)ENOTDIR);
	}
	/* Function opendir() fails if pathname is a zero-length string */
	{
		DIR * dir = opendir(""); /* Open directory */
		rCase.SLTEST_CHECK_Z(dir);
		rCase.SLTEST_CHECK_EQ(errno, (long)ENOENT);
	}
	/* Rewind of directory stream */
	{
		struct dirent * ent;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLTEST_CHECK_NZ(dir);
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(strcmp(ent->d_name, ".") == 0) {
				found += 1; // Directory itself
			}
			else if(strcmp(ent->d_name, "..") == 0) {
				found += 2; // Parent directory
			}
			else if(strcmp(ent->d_name, "file") == 0) {
				found += 4; /* Regular file */
			}
			else if(strcmp(ent->d_name, "dir") == 0) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name);
				rCase.SetInfo(temp_buf, 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, (long)0xf); // Make sure that all files were found
		rewinddir(dir); // Rewind stream and read entries again 
		found = 0;
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(strcmp(ent->d_name, ".") == 0) {
				found += 1; // Directory itself
			}
			else if(strcmp(ent->d_name, "..") == 0) {
				found += 2; // Parent directory
			}
			else if(strcmp(ent->d_name, "file") == 0) {
				found += 4; /* Regular file */
			}
			else if(strcmp(ent->d_name, "dir") == 0) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name);
				rCase.SetInfo(temp_buf, 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, (long)0xf); // Make sure that all files were found
		closedir(dir);
	}
	/* Rewind with intervening change of working directory */
	{
		struct dirent * ent;
		int found = 0;
		int errorcode;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		rCase.SLTEST_CHECK_NZ(dir);
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(strcmp(ent->d_name, ".") == 0) {
				found += 1; // Directory itself
			}
			else if(strcmp(ent->d_name, "..") == 0) {
				found += 2; // Parent directory
			}
			else if(strcmp(ent->d_name, "file") == 0) {
				found += 4; /* Regular file */
			}
			else if(strcmp(ent->d_name, "dir") == 0) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name);
				rCase.SetInfo(temp_buf, 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
		/* Change working directory */
		errorcode = chdir((dir_buf = pBaseDir).RmvLastSlash());
		rCase.SLTEST_CHECK_Z(errorcode);
		rewinddir(dir); // Rewind stream and read entries again 
		found = 0;
		/* Read entries */
		while((ent = readdir(dir)) != NULL) {
			/* Check each file */
			if(strcmp(ent->d_name, ".") == 0) {
				found += 1; // Directory itself
			}
			else if(strcmp(ent->d_name, "..") == 0) {
				found += 2; // Parent directory
			}
			else if(strcmp(ent->d_name, "file") == 0) {
				found += 4; /* Regular file */
			}
			else if(strcmp(ent->d_name, "dir") == 0) {
				found += 8; /* Just a directory */
			}
			else { // Other file
				temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name);
				rCase.SetInfo(temp_buf, 0);
			}
		}
		rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
		/* Restore working directory */
		errorcode = chdir("..");
		rCase.SLTEST_CHECK_Z(errorcode);
		closedir(dir);
	}
	/* Long file name */
	{
		struct dirent * ent;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("2")); /* Open directory */
		if(dir == NULL) {
			temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found");
			rCase.SetInfo(temp_buf, 0);
		}
		else {
			/* Read entries */
			while((ent = readdir(dir)) != NULL) {
				/* Check each file */
				if(strcmp(ent->d_name, ".") == 0) {
					found += 1; // Directory itself
				}
				else if(strcmp(ent->d_name, "..") == 0) {
					found += 2; // Parent directory
				}
				else if(strcmp(ent->d_name, "file.txt") == 0) {
					/* Regular 8+3 filename */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 8);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 8);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(8L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 4;
				}
				else if(strcmp(ent->d_name, "Testfile-1.2.3.dat") == 0) {
					/* Long file name with multiple dots */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(ent->d_type, (long)DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(ent->d_namlen, 18);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(ent), 18);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(18L, _D_ALLOC_NAMLEN(ent));
	#endif
					found += 8;
				}
				else { // Other file
					temp_buf.Z().Cat("Unexpected file").Space().Cat(ent->d_name);
					rCase.SetInfo(temp_buf, 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
			closedir(dir);
		}
	}
	/* Basic directory retrieval with readdir_r */
	{
		struct dirent ent[10];
		struct dirent * entry;
		size_t i = 0;
		size_t n = 0;
		int found = 0;
		DIR * dir = opendir((dir_buf = pBaseDir).SetLastDSlash().Cat("1")); /* Open directory */
		if(dir == NULL) {
			temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found");
			rCase.SetInfo(temp_buf, 0);
		}
		else {
			/* Read entries to table */
			while(readdir_r(dir, &ent[n], &entry) == /*OK*/ 0 && entry != 0) {
				n++;
				rCase.SLTEST_CHECK_LE(n, 4L);
			}
			/* Make sure that we got all the files from directory */
			rCase.SLTEST_CHECK_EQ(n, 4);
			/* Check entries in memory */
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				/* Check each file */
				if(strcmp(entry->d_name, ".") == 0) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 1);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 1);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(strcmp(entry->d_name, "..") == 0) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 2);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 2);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(strcmp(entry->d_name, "file") == 0) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 4);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 4);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(strcmp(entry->d_name, "dir") == 0) { /* Just a directory */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 3);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 3);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					temp_buf.Z().Cat("Unexpected file").Space().Cat(entry->d_name);
					rCase.SetInfo(temp_buf, 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
			closedir(dir);
		}
	}
	/* Basic directory retrieval with _wreaddir_r */
#ifdef WIN32
	{
		struct _wdirent ent[10];
		struct _wdirent * entry;
		size_t i = 0;
		size_t n = 0;
		int found = 0;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("1");
		SStringU dir_buf_u;
		dir_buf_u.CopyFromMb(cpANSI, dir_buf, dir_buf.Len());
		_WDIR * dir = _wopendir(dir_buf_u/*L"tests/1"*/); /* Open directory */
		if(dir == NULL) {
			temp_buf.Z().Cat("Directory").Space().Cat(dir_buf).Space().Cat("not found");
			rCase.SetInfo(temp_buf, 0);
		}
		else {
			// Read entries to table 
			while(_wreaddir_r(dir, &ent[n], &entry) == /*OK*/ 0 && entry != 0) {
				n++;
				rCase.SLTEST_CHECK_LE(n, 4L);
			}
			rCase.SLTEST_CHECK_EQ(n, 4); // Make sure that we got all the files from directory 
			// Check entries in memory 
			for(i = 0; i < 4; i++) {
				entry = &ent[i];
				// Check each file 
				if(wcscmp(entry->d_name, L".") == 0) { // Directory itself
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 1);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 1);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(1L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 1;
				}
				else if(wcscmp(entry->d_name, L"..") == 0) { // Parent directory
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 2);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 2);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(2L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 2;
				}
				else if(wcscmp(entry->d_name, L"file") == 0) { /* Regular file */
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_REG);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 4);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 4);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(4L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 4;
				}
				else if(wcscmp(entry->d_name, L"dir") == 0) { // Just a directory 
	#ifdef _DIRENT_HAVE_D_TYPE
					rCase.SLTEST_CHECK_EQ(entry->d_type, (long)DT_DIR);
	#endif
	#ifdef _DIRENT_HAVE_D_NAMLEN
					rCase.SLTEST_CHECK_EQ(entry->d_namlen, 3);
	#endif
	#ifdef _D_EXACT_NAMLEN
					rCase.SLTEST_CHECK_EQ(_D_EXACT_NAMLEN(entry), 3);
	#endif
	#ifdef _D_ALLOC_NAMLEN
					rCase.SLTEST_CHECK_LT(3L, _D_ALLOC_NAMLEN(entry));
	#endif
					found += 8;
				}
				else { // Other file
					temp_buf.Z().Cat("Unexpected file");
					rCase.SetInfo(temp_buf, 0);
				}
			}
			rCase.SLTEST_CHECK_EQ(found, 0xfL); // Make sure that all files were found
			_wclosedir(dir);
		}
	}
#endif
	return rCase.GetCurrentStatus();
}

/* Filter and sort functions */
static int only_readme(const struct dirent * entry);
static int no_directories(const struct dirent * entry);
static int reverse_alpha(const struct dirent ** a, const struct dirent ** b);

static int T_ScanDir(STestCase & rCase, const char * pBaseDir)
{
	struct dirent ** files;
	int i;
	int n;
	SString dir_buf;
	SString temp_buf;
	srand((uint)time(NULL)); // Initialize random number generator 
	// Basic scan with simple filter function 
	{
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, only_readme, alphasort); /* Read directory entries */
		rCase.SLTEST_CHECK_EQ(n, 1L);
		rCase.SLTEST_CHECK_Z(strcmp(files[0]->d_name, "README.txt")); /* Make sure that the filter works */
		// Release file names 
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Basic scan with default sorting function */
	{
		/* Read directory entries in alphabetic order */
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, 13L);
		/* Make sure that we got all the names in the proper order */
		rCase.SLTEST_CHECK_Z(strcmp(files[0]->d_name, "."));
		rCase.SLTEST_CHECK_Z(strcmp(files[1]->d_name, ".."));
		rCase.SLTEST_CHECK_Z(strcmp(files[2]->d_name, "3zero.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[3]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[4]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[5]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_Z(strcmp(files[6]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[7]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[8]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[10]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[11]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[12]->d_name, "zebra.dat"));
		/* Release file names */
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Custom filter AND sort function */
	{
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, no_directories, reverse_alpha); /* Read directory entries in alphabetic order */
		rCase.SLTEST_CHECK_EQ(n, 11L);
		/* Make sure that we got file names in the reverse order */
		rCase.SLTEST_CHECK_Z(strcmp(files[0]->d_name, "zebra.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[1]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[2]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[3]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[4]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[5]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[6]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[7]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_Z(strcmp(files[8]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[9]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[10]->d_name, "3zero.dat"));
		/* Release file names */
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Trying to read from non-existent directory leads to an error */
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("invalid");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, -1L);
		rCase.SLTEST_CHECK_Z(files);
		rCase.SLTEST_CHECK_EQ(errno, (long)ENOENT);
	}
	/* Trying to open file as a directory produces ENOTDIR error */
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3/666.dat");
		n = scandir(dir_buf, &files, NULL, alphasort);
		rCase.SLTEST_CHECK_EQ(n, -1L);
		rCase.SLTEST_CHECK_Z(files);
		rCase.SLTEST_CHECK_EQ(errno, (long)ENOTDIR);
	}
	/* Sort files using versionsort() */
	{
		files = NULL;
		(dir_buf = pBaseDir).SetLastDSlash().Cat("3");
		n = scandir(dir_buf, &files, no_directories, versionsort);
		rCase.SLTEST_CHECK_EQ(n, 11L);
		/*
		 * Make sure that we got all the file names in the proper order:
		 * 1.2.4 < 1.2.30 < 1.12.0
		 */
		rCase.SLTEST_CHECK_Z(strcmp(files[0]->d_name, "3zero.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[1]->d_name, "666.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[2]->d_name, "Qwerty-my-aunt.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[3]->d_name, "README.txt"));
		rCase.SLTEST_CHECK_Z(strcmp(files[4]->d_name, "aaa.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[5]->d_name, "dirent.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[6]->d_name, "empty.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[7]->d_name, "sane-1.2.4.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[8]->d_name, "sane-1.2.30.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[9]->d_name, "sane-1.12.0.dat"));
		rCase.SLTEST_CHECK_Z(strcmp(files[10]->d_name, "zebra.dat"));
		/* Release file names */
		for(i = 0; i < n; i++) {
			SAlloc::F(files[i]);
		}
		SAlloc::F(files);
	}
	/* Scan large directory */
	{
		char dirname[PATH_MAX+1];
		int i;
		int ok;
		/* Copy name of temporary directory to variable dirname */
#ifdef WIN32
		i = GetTempPathA(PATH_MAX, dirname);
		rCase.SLTEST_CHECK_LT(0L, i);
#else
		strcpy(dirname, "/tmp/");
		i = strlen(dirname);
#endif
		/* Append random characters to dirname */
		for(int j = 0; j < 10; j++) {
			char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26]; /* Generate random character */
			// Append character to dirname 
			rCase.SLTEST_CHECK_LT(i, (long)PATH_MAX);
			dirname[i++] = c;
		}
		/* Terminate directory name */
		rCase.SLTEST_CHECK_LT(i, (long)PATH_MAX);
		dirname[i] = '\0';
		/* Create directory */
#ifdef WIN32
		ok = CreateDirectoryA(dirname, NULL);
		rCase.SLTEST_CHECK_NZ(ok);
#else
		ok = mkdir(dirname, 0700);
		rCase.SLTEST_CHECK_Z(ok); /*success*/
#endif
		/* Create one thousand files */
		rCase.SLTEST_CHECK_LT((i + 5), (long)PATH_MAX);
		for(int j = 0; j < 1000; j++) {
			FILE * fp;
			/* Construct file name */
			dirname[i] = '/';
			dirname[i+1] = 'z';
			dirname[i+2] = '0' + ((j / 100) % 10);
			dirname[i+3] = '0' + ((j / 10) % 10);
			dirname[i+4] = '0' + (j % 10);
			dirname[i+5] = '\0';
			/* Create file */
			fp = fopen(dirname, "w");
			rCase.SLTEST_CHECK_NZ(fp);
			fclose(fp);
		}
		dirname[i] = '\0'; /* Cut out the file name part */
		/* Scan directory */
		n = scandir(dirname, &files, no_directories, alphasort);
		rCase.SLTEST_CHECK_EQ(n, 1000L);
		/* Make sure that all 1000 files are read back */
		for(int j = 0; j < n; j++) {
			char match[100];
			/* Construct file name */
			match[0] = 'z';
			match[1] = '0' + ((j / 100) % 10);
			match[2] = '0' + ((j / 10) % 10);
			match[3] = '0' + (j % 10);
			match[4] = '\0';
			/* Make sure that file name matches that on the disk */
			rCase.SLTEST_CHECK_Z(strcmp(files[j]->d_name, match));
		}
		/* Release file names */
		for(int j = 0; j < n; j++) {
			SAlloc::F(files[j]);
		}
		SAlloc::F(files);
	}
	return rCase.GetCurrentStatus();
}
//
// Only pass README.txt file 
//
static int only_readme(const struct dirent * entry)
{
	int pass;
	if(strcmp(entry->d_name, "README.txt") == 0) {
		pass = 1;
	}
	else {
		pass = 0;
	}
	return pass;
}
//
// Filter out directories 
//
static int no_directories(const struct dirent * entry)
{
	int pass;
	if(entry->d_type != DT_DIR) {
		pass = 1;
	}
	else {
		pass = 0;
	}
	return pass;
}
//
// Sort in reverse direction 
//
static int reverse_alpha(const struct dirent ** a, const struct dirent ** b)
{
	return strcoll((*b)->d_name, (*a)->d_name);
}
//
//
//
static int T_Strverscmp(STestCase & rCase)
{
	/* Strings without digits are compared as in strcmp() */
	rCase.SLTEST_CHECK_NZ(strverscmp("", "") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abc", "abc") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a", "b") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("b", "a") > 0);
	/* Shorter string is smaller, other things being equal */
	rCase.SLTEST_CHECK_NZ(strverscmp("a", "aa") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("aa", "a") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abcdef", "abcdefg") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("abcdefg", "abcdef") > 0);
	/* Integers with equal length are compared as in strcmp() */
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "0") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "000") == 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "2") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("001", "100") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("100", "001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2020-07-01", "2020-07-02") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("2020-07-02", "2020-07-01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("jan999", "jan999") == 0);
	/* Integers of different length are compared as numbers */
	rCase.SLTEST_CHECK_NZ(strverscmp("jan9", "jan10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("jan10", "jan9") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("999", "1000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000", "999") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("t12-1000", "t12-9999") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("t12-9999", "t12-1000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000!", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000!") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1000Z", "10001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("10001", "1000Z") > 0);
	/* If numbers starts with zero, then longer number is smaller */
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "00") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a000", "a00") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("a00", "a000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000!", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0000", "000Z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000Z", "0000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0", "1.01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0!", "1.01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.01", "1.0~") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0~", "1.01") > 0);
	/* Number having more leading zeros is considered smaller */
	rCase.SLTEST_CHECK_NZ(strverscmp("item-0001", "item-001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-001", "item-0001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-001", "item-01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-01", "item-001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".001") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".001", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".01", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".0001000", ".1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp(".1", ".0001000") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0002", "1.0010000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.0010000", "1.0002") > 0);
	/* Number starting with zero is smaller than any number */
	rCase.SLTEST_CHECK_NZ(strverscmp("item-009", "item-1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-1", "item-009") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-099", "item-2") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("item-2", "item-099") > 0);
	/* Number vs alphabetical comparison */
	rCase.SLTEST_CHECK_NZ(strverscmp("1.001", "1.00!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.00!", "1.001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.001", "1.00x") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1.00x", "1.001") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "x") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("x", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "!") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("!", "1") < 0);
	/* Handling the end of string */
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "011") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("011", "01") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0100", "01000") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01000", "0100") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "1!") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1!", "1") > 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "1z") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1z", "1") > 0);
	/* Ordering 000 < 00 < 01 < 010 < 09 < 0 < 1 < 9 < 10 */
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "00") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("000", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "01") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("00", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "010") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("01", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "09") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("010", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "0") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("09", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "1") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("0", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "9") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("1", "10") < 0);
	rCase.SLTEST_CHECK_NZ(strverscmp("9", "10") < 0);
	/* Compare speed */
	{
#define LENGTH 100
#define REPEAT 1000000
		char a[LENGTH+1];
		char b[LENGTH+1];
		size_t i;
		size_t j;
		const char letters[] = "01234567890123456789abdefghjkpqrtwxyz-/.";
		size_t n = strlen(letters);
		/* Repeat test */
		for(i = 0; i < REPEAT; i++) {
			int diff1;
			int diff2;
			/* Generate two random strings of LENGTH characters */
			for(j = 0; j < LENGTH; j++) {
				a[j] = letters[rand() % n];
				b[j] = letters[rand() % n];
			}
			a[j] = '\0';
			b[j] = '\0';
			/* Compare strings in both directions */
			diff1 = strverscmp(a, b);
			diff2 = strverscmp(b, a);
			/* Must give identical result in both directions */
			rCase.SLTEST_CHECK_NZ((diff1 < 0 && diff2 > 0) || (diff1 == 0 && diff2 == 0) || (diff1 > 0 && diff2 < 0));
		}
	}
	return rCase.GetCurrentStatus();
}

static int T_DirEntUnicode(STestCase & rCase, const char * pLocale)
{
#ifdef WIN32
	wchar_t wpath[MAX_PATH+1];
	char path[MAX_PATH+1];
	DWORD i, j, k, x;
	BOOL ok;
	HANDLE fh;
	_WDIR * wdir;
	struct _wdirent * wentry;
	DIR * dir;
	struct dirent * entry;
	char buffer[100];
	FILE * fp;
	int counter = 0;
	srand(((int)time(NULL)) * 257 + ((int)GetCurrentProcessId())); /* Initialize random number generator */
	/* Set current locale */
	if(!isempty(pLocale)) {
		printf("Locale %s\n", pLocale);
		setlocale(LC_ALL, pLocale);
	}
	else {
		setlocale(LC_ALL, "");
	}
	/****** CREATE FILE WITH UNICODE FILE NAME ******/

	/* Get path to temporary directory (wide-character and ascii) */
	i = GetTempPathW(MAX_PATH, wpath);
	assert(i > 0);
	j = GetTempPathA(MAX_PATH, path);
	assert(j > 0);
	/* Append random directory name */
	for(k = 0; k < 10; k++) {
		char c = "abcdefghijklmnopqrstuvwxyz"[rand() % 26]; /* Generate random character */
		/* Append character to paths */
		assert(i < MAX_PATH &&  j < MAX_PATH);
		wpath[i++] = c;
		path[j++] = c;
	}
	/* Terminate paths */
	assert(i < MAX_PATH &&  j < MAX_PATH);
	wpath[i] = '\0';
	path[j] = '\0';
	k = i; /* Remember the end of directory name */
	/* Create directory using unicode */
	ok = CreateDirectoryW(wpath, NULL);
	if(!ok) {
		DWORD e = GetLastError();
		wprintf(L"Cannot create directory %ls (code %u)\n", wpath, e);
		abort();
	}
	/* Overwrite zero terminator with path separator */
	assert(i < MAX_PATH &&  j < MAX_PATH);
	wpath[i++] = '\\';
	/* Append a few unicode characters */
	assert(i < MAX_PATH);
	wpath[i++] = 0x6d4b;
	assert(i < MAX_PATH);
	wpath[i++] = 0x8bd5;

	/* Terminate string */
	assert(i < MAX_PATH);
	wpath[i] = '\0';
	/* Create file with unicode */
	fh = CreateFileW(wpath, /* Access */ GENERIC_READ | GENERIC_WRITE, /* Share mode */ 0, /* Security attributes */ NULL, /* Creation disposition */ CREATE_NEW,
		/* Attributes */ FILE_ATTRIBUTE_NORMAL, /* Template files */ NULL);
	assert(fh != INVALID_HANDLE_VALUE);
	/* Write some data to file */
	ok = WriteFile(/* File handle */ fh, /* Pointer to data */ "hep\n", /* Number of bytes to write */ 4, /* Number of bytes written */ NULL, /* Overlapped */ NULL);
	assert(ok);
	ok = CloseHandle(fh); /* Close file */
	assert(ok);

	/****** MAKE SURE THAT UNICODE FILE CAN BE READ BY _WREADDIR ******/

	// Zero terminate wide-character path and open directory stream 
	wpath[k] = '\0';
	wdir = _wopendir(wpath);
	if(wdir == NULL) {
		wprintf(L"Cannot open directory %ls\n", wpath);
		abort();
	}
	// Read through entries 
	counter = 0;
	while((wentry = _wreaddir(wdir)) != NULL) {
		// Skip pseudo directories 
		if(wcscmp(wentry->d_name, L".") == 0) {
			continue;
		}
		if(wcscmp(wentry->d_name, L"..") == 0) {
			continue;
		}
		// Found a file 
		counter++;
		assert(wentry->d_type == DT_REG);
		// Append file name to path 
		i = k;
		assert(i < MAX_PATH);
		wpath[i++] = '\\';
		x = 0;
		while(wentry->d_name[x] != '\0') {
			assert(i < MAX_PATH);
			wpath[i++] = wentry->d_name[x++];
		}
		assert(i < MAX_PATH);
		wpath[i] = '\0';
		// Open file for read 
		fh = CreateFileW(wpath, /* Access */ GENERIC_READ,/* Share mode */ 0, /* Security attributes */ NULL,/* Creation disposition */ OPEN_EXISTING, 
			/* Attributes */ FILE_ATTRIBUTE_NORMAL, /* Template files */ NULL);
		assert(fh != INVALID_HANDLE_VALUE);
		// Read data from file 
		ok = ReadFile(/* File handle */ fh, /* Output buffer */ buffer, /* Max number of bytes to read */ sizeof(buffer) - 1, /* Number of bytes actually read */ &x, /* Overlapped */ NULL);
		assert(ok);
		// Make sure that we got the file contents right 
		assert(x == 4);
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		ok = CloseHandle(fh); /* Close file */
		assert(ok);
	}
	assert(counter == 1);
	_wclosedir(wdir); /* Close directory */

	/****** MAKE SURE THAT UNICODE FILE NAME CAN BE READ BY READDIR *****/

	/* Zero terminate ascii path and open directory stream */
	k = j;
	path[k] = '\0';
	dir = opendir(path);
	if(dir == NULL) {
		slfprintf_stderr("Cannot open directory %s\n", path);
		abort();
	}
	/* Read through entries */
	counter = 0;
	while((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if(strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if(strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);
		/* Append file name to path */
		j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		x = 0;
		while(entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';
		/* Open file for read */
		fp = fopen(path, "r");
		if(!fp) {
			slfprintf_stderr("Cannot open file %s\n", path);
			abort();
		}
		/* Read data from file */
		if(fgets(buffer, sizeof(buffer), fp) == NULL) {
			slfprintf_stderr("Cannot read file %s\n", path);
			abort();
		}
		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');
		fclose(fp); /* Close file */
	}
	assert(counter == 1);
	closedir(dir); /* Close directory */
	/****** CREATE FILE WITH UTF-8 ******/

	/* Append UTF-8 file name (åäö.txt) to path */
	j = k;
	path[j++] = '\\';
	path[j++] = '\xc3';
	path[j++] = '\xa5';
	path[j++] = '\xc3';
	path[j++] = '\xa4';
	path[j++] = '\xc3';
	path[j++] = '\xb6';
	path[j++] = '\x2e';
	path[j++] = '\x74';
	path[j++] = '\x78';
	path[j++] = '\x74';
	assert(j < MAX_PATH);
	path[j] = '\0';
	/*
	 * Create file.
	 *
	 * Be ware that the code below creates a different file depending on
	 * the current locale!  For example, if the current locale is
	 * english_us.65001, then the file name will be "åäö.txt" (7
	 * characters).  However, if the current locale is english_us.1252,
	 * then the file name will be "ÃċÃĊÃ¶.txt" (10 characters).
	 */
	printf("Creating %s\n", path);
	fp = fopen(path, "w");
	if(!fp) {
		slfprintf_stderr("Cannot open file %s\n", path);
		abort();
	}
	fputs("hep\n", fp);
	fclose(fp);
	/* Open directory again */
	path[k] = '\0';
	dir = opendir(path);
	if(dir == NULL) {
		slfprintf_stderr("Cannot open directory %s\n", path);
		abort();
	}
	/* Read through entries */
	counter = 0;
	while((entry = readdir(dir)) != NULL) {
		/* Skip pseudo directories */
		if(strcmp(entry->d_name, ".") == 0) {
			continue;
		}
		if(strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		/* Found a file */
		counter++;
		assert(entry->d_type == DT_REG);
		/* Append file name to path */
		j = k;
		assert(j < MAX_PATH);
		path[j++] = '\\';
		x = 0;
		while(entry->d_name[x] != '\0') {
			assert(j < MAX_PATH);
			path[j++] = entry->d_name[x++];
		}
		assert(j < MAX_PATH);
		path[j] = '\0';
		/* Print file name for debugging */
		printf("Opening \"%s\" hex ", path + k + 1);
		x = 0;
		while(entry->d_name[x] != '\0') {
			printf("0x%02x ", (uint)(entry->d_name[x++] & 0xff));
		}
		printf("\n");
		/* Open file for read */
		fp = fopen(path, "r");
		if(!fp) {
			slfprintf_stderr("Cannot open file %s\n", path);
			abort();
		}
		/* Read data from file */
		if(fgets(buffer, sizeof(buffer), fp) == NULL) {
			slfprintf_stderr("Cannot read file %s\n", path);
			abort();
		}
		/* Make sure that we got the file contents right */
		assert(buffer[0] == 'h');
		assert(buffer[1] == 'e');
		assert(buffer[2] == 'p');
		assert(buffer[3] == '\n');
		assert(buffer[4] == '\0');
		fclose(fp); /* Close file */
	}
	assert(counter == 2);
	closedir(dir); /* Close directory */
#else
	/* Linux */
	(void)argc;
	(void)argv;
#endif
	return EXIT_SUCCESS;
}

int DummyProc_dirent() { return 1; } // @forcelink

SLTEST_R(dirent)
{
	// File type macros 
	assert(DTTOIF(DT_REG) == S_IFREG);
	assert(DTTOIF(DT_DIR) == S_IFDIR);
	assert(DTTOIF(DT_FIFO) == S_IFIFO);
	assert(DTTOIF(DT_SOCK) == S_IFSOCK);
	assert(DTTOIF(DT_CHR) == S_IFCHR);
	assert(DTTOIF(DT_BLK) == S_IFBLK);
	assert(IFTODT(S_IFREG) == DT_REG);
	assert(IFTODT(S_IFDIR) == DT_DIR);
	assert(IFTODT(S_IFIFO) == DT_FIFO);
	assert(IFTODT(S_IFSOCK) == DT_SOCK);
	assert(IFTODT(S_IFCHR) == DT_CHR);
	assert(IFTODT(S_IFBLK) == DT_BLK);
	{
		SString base_dir;
		SString path = GetSuiteEntry()->InPath;
		(base_dir = path).SetLastSlash().Cat("dirent");
		T_Strverscmp(*this);
		T_Dirent(*this, base_dir.cptr());
		T_ScanDir(*this, base_dir.cptr());
	}
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING 
