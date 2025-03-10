/*-
 * Copyright (c) 2003-2009 Tim Kientzle
 * Copyright (c) 2010-2012 Michihiro NAKAJIMA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#include "archive_platform.h"
#pragma hdrstop
__FBSDID("$FreeBSD$");

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <winioctl.h>
#include "archive_read_disk_private.h"

#ifndef O_BINARY
#define O_BINARY        0
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
/* Old SDKs do not provide IO_REPARSE_TAG_SYMLINK */
#define IO_REPARSE_TAG_SYMLINK 0xA000000CL
#endif

/*-
 * This is a new directory-walking system that addresses a number
 * of problems I've had with fts(3).  In particular, it has no
 * pathname-length limits (other than the size of 'int'), handles
 * deep logical traversals, uses considerably less memory, and has
 * an opaque interface (easier to modify in the future).
 *
 * Internally, it keeps a single list of "tree_entry" items that
 * represent filesystem objects that require further attention.
 * Non-directories are not kept in memory: they are pulled from
 * readdir(), returned to the client, then freed as soon as possible.
 * Any directory entry to be traversed gets pushed onto the stack.
 *
 * There is surprisingly little information that needs to be kept for
 * each item on the stack.  Just the name, depth (represented here as the
 * string length of the parent directory's pathname), and some markers
 * indicating how to get back to the parent (via chdir("..") for a
 * regular dir or via fchdir(2) for a symlink).
 */

struct restore_time {
	const wchar_t           * full_path;
	FILETIME lastWriteTime;
	FILETIME lastAccessTime;
	mode_t filetype;
};

struct tree_entry {
	int depth;
	struct tree_entry       * next;
	struct tree_entry       * parent;
	size_t full_path_dir_length;
	archive_wstring name;
	archive_wstring full_path;
	size_t dirname_length;
	int64 dev;
	int64 ino;
	int flags;
	int filesystem_id;
	/* How to restore time of a directory. */
	struct restore_time restore_time;
};

struct filesystem {
	int64 dev;
	int synthetic;
	int remote;
	DWORD bytesPerSector;
};

/* Definitions for tree_entry.flags bitmap. */
#define isDir           1  /* This entry is a regular directory. */
#define isDirLink       2  /* This entry is a symbolic link to a directory. */
#define needsFirstVisit 4  /* This is an initial entry. */
#define needsDescent    8  /* This entry needs to be previsited. */
#define needsOpen       16 /* This is a directory that needs to be opened. */
#define needsAscent     32 /* This entry needs to be postvisited. */

/*
 * On Windows, "first visit" is handled as a pattern to be handed to
 * _findfirst().  This is consistent with Windows conventions that
 * file patterns are handled within the application.  On Posix,
 * "first visit" is just returned to the client.
 */

#define MAX_OVERLAPPED  8
#define READ_BUFFER_SIZE        (1024 * 64) /* Default to 64KB per
	                                       https://technet.microsoft.com/en-us/library/cc938632.aspx */
#define DIRECT_IO       0/* Disabled */
#define ASYNC_IO        1// enabled
/*
 * Local data for this package.
 */
struct tree {
	struct tree_entry       * stack;
	struct tree_entry       * current;
	HANDLE d;
	WIN32_FIND_DATAW _findData;
	WIN32_FIND_DATAW        * findData;
	int flags;
	int visit_type;
	int tree_errno; // Error code from last failed operation
	archive_wstring full_path; // A full path with "\\?\" prefix
	size_t full_path_dir_length;
	archive_wstring path; // Dynamically-sized buffer for holding path
	const wchar_t * basename; // Last path element
	size_t dirname_length; // Leading dir length
	int depth;
	BY_HANDLE_FILE_INFORMATION lst;
	BY_HANDLE_FILE_INFORMATION st;
	int descend;
	struct restore_time restore_time; // How to restore time of a file

	struct entry_sparse {
		int64 length;
		int64 offset;
	} * sparse_list, * current_sparse;

	int  sparse_count;
	int  sparse_list_size;
	char initial_symlink_mode;
	char symlink_mode;
	struct filesystem * current_filesystem;
	struct filesystem * filesystem_table;
	int initial_filesystem_id;
	int current_filesystem_id;
	int max_filesystem_id;
	int allocated_filesystem;
	HANDLE entry_fh;
	int entry_eof;
	int64 entry_remaining_bytes;
	int64 entry_total;
	int ol_idx_doing;
	int ol_idx_done;
	int ol_num_doing;
	int ol_num_done;
	int64 ol_remaining_bytes;
	int64 ol_total;
	struct la_overlapped {
		OVERLAPPED ol;
		Archive * _a;
		uchar * buff;
		size_t buff_size;
		int64 offset;
		size_t bytes_expected;
		size_t bytes_transferred;
	} ol[MAX_OVERLAPPED];
	int direct_io;
	int async_io;
};

#define bhfi_dev(bhfi)  ((bhfi)->dwVolumeSerialNumber)
/* Treat FileIndex as i-node. We should remove a sequence number
 * which is high-16-bits of nFileIndexHigh. */
#define bhfi_ino(bhfi)  ((((int64)((bhfi)->nFileIndexHigh & 0x0000FFFFUL)) << 32) + (bhfi)->nFileIndexLow)

/* Definitions for tree.flags bitmap. */
#define hasStat         16 /* The st entry is valid. */
#define hasLstat        32 /* The lst entry is valid. */
#define needsRestoreTimes 128

static int tree_dir_next_windows(struct tree * t, const wchar_t * pattern);

/* Initiate/terminate a tree traversal. */
static struct tree * tree_open(const wchar_t *, int, int);
static struct tree * tree_reopen(struct tree *, const wchar_t *, int);
static void tree_close(struct tree *);
static void tree_free(struct tree *);
static void tree_push(struct tree *, const wchar_t *, const wchar_t *, int, int64, int64, struct restore_time *);
/*
 * tree_next() returns Zero if there is no next entry, non-zero if
 * there is.  Note that directories are visited three times.
 * Directories are always visited first as part of enumerating their
 * parent; that is a "regular" visit.  If tree_descend() is invoked at
 * that time, the directory is added to a work list and will
 * subsequently be visited two more times: once just after descending
 * into the directory ("postdescent") and again just after ascending
 * back to the parent ("postascent").
 *
 * TREE_ERROR_DIR is returned if the descent failed (because the
 * directory couldn't be opened, for instance).  This is returned
 * instead of TREE_POSTDESCENT/TREE_POSTASCENT.  TREE_ERROR_DIR is not a
 * fatal error, but it does imply that the relevant subtree won't be
 * visited.  TREE_ERROR_FATAL is returned for an error that left the
 * traversal completely hosed.  Right now, this is only returned for
 * chdir() failures during ascent.
 */
#define TREE_REGULAR            1
#define TREE_POSTDESCENT        2
#define TREE_POSTASCENT         3
#define TREE_ERROR_DIR          -1
#define TREE_ERROR_FATAL        -2

static int tree_next(struct tree *);

/*
 * Return information about the current entry.
 */

/*
 * The current full pathname, length of the full pathname, and a name
 * that can be used to access the file.  Because tree does use chdir
 * extensively, the access path is almost never the same as the full
 * current path.
 *
 */
static const wchar_t * tree_current_path(struct tree *);
static const wchar_t * tree_current_access_path(struct tree *);

/*
 * Request the lstat() or stat() data for the current path.  Since the
 * tree package needs to do some of this anyway, and caches the
 * results, you should take advantage of it here if you need it rather
 * than make a redundant stat() or lstat() call of your own.
 */
static const BY_HANDLE_FILE_INFORMATION * tree_current_stat(struct tree *);
static const BY_HANDLE_FILE_INFORMATION * tree_current_lstat(struct tree *);

/* The following functions use tricks to avoid a certain number of
 * stat()/lstat() calls. */
/* "is_physical_dir" is equivalent to S_ISDIR(tree_current_lstat()->st_mode) */
static int tree_current_is_physical_dir(struct tree *);
/* "is_physical_link" is equivalent to S_ISLNK(tree_current_lstat()->st_mode) */
static int tree_current_is_physical_link(struct tree *);
/* Instead of archive_entry_copy_stat for BY_HANDLE_FILE_INFORMATION */
static void tree_archive_entry_copy_bhfi(ArchiveEntry *, struct tree *, const BY_HANDLE_FILE_INFORMATION *);
/* "is_dir" is equivalent to S_ISDIR(tree_current_stat()->st_mode) */
static int tree_current_is_dir(struct tree *);
static int update_current_filesystem(struct archive_read_disk * a, int64 dev);
static int setup_current_filesystem(struct archive_read_disk *);
static int tree_target_is_same_as_parent(struct tree *, const BY_HANDLE_FILE_INFORMATION *);
static int _archive_read_disk_open_w(Archive *, const wchar_t *);
static int _archive_read_free(Archive *);
static int _archive_read_close(Archive *);
static int _archive_read_data_block(Archive *, const void **, size_t *, int64 *);
static int _archive_read_next_header(Archive *, ArchiveEntry **);
static int _archive_read_next_header2(Archive *, ArchiveEntry *);
static const char * trivial_lookup_gname(void *, int64 gid);
static const char * trivial_lookup_uname(void *, int64 uid);
static int setup_sparse(struct archive_read_disk *, ArchiveEntry *);
static int close_and_restore_time(HANDLE, struct tree *, struct restore_time *);
static int setup_sparse_from_disk(struct archive_read_disk *, ArchiveEntry *, HANDLE);
static int la_linkname_from_handle(HANDLE, wchar_t **, int *);
static int la_linkname_from_pathw(const wchar_t *, wchar_t **, int *);
static void     entry_symlink_from_pathw(ArchiveEntry *, const wchar_t * path);

typedef struct _REPARSE_DATA_BUFFER {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;

		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;

		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	} DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;
/*
 * Reads the target of a symbolic link
 *
 * Returns 0 on success and -1 on failure
 * outbuf is allocated in the function
 */
static int la_linkname_from_handle(HANDLE h, wchar_t ** linkname, int * linktype)
{
	DWORD inbytes;
	REPARSE_DATA_BUFFER * buf;
	BY_HANDLE_FILE_INFORMATION st;
	size_t len;
	BYTE * indata;
	wchar_t * tbuf;
	BOOL ret = GetFileInformationByHandle(h, &st);
	if(ret == 0 || (st.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
		return -1;
	}
	indata = static_cast<BYTE *>(SAlloc::M(MAXIMUM_REPARSE_DATA_BUFFER_SIZE));
	ret = DeviceIoControl(h, FSCTL_GET_REPARSE_POINT, NULL, 0, indata, 1024, &inbytes, NULL);
	if(ret == 0) {
		la_dosmaperr(GetLastError());
		SAlloc::F(indata);
		return -1;
	}
	buf = (REPARSE_DATA_BUFFER*)indata;
	if(buf->ReparseTag != IO_REPARSE_TAG_SYMLINK) {
		SAlloc::F(indata);
		// File is not a symbolic link
		errno = EINVAL;
		return -1;
	}
	len = buf->SymbolicLinkReparseBuffer.SubstituteNameLength;
	if(len <= 0) {
		SAlloc::F(indata);
		return -1;
	}
	tbuf = static_cast<wchar_t *>(SAlloc::M(len + 1 * sizeof(wchar_t)));
	if(tbuf == NULL) {
		SAlloc::F(indata);
		return -1;
	}
	memcpy(tbuf, &((BYTE*)buf->SymbolicLinkReparseBuffer.PathBuffer)[buf->SymbolicLinkReparseBuffer.SubstituteNameOffset], len);
	SAlloc::F(indata);
	tbuf[len / sizeof(wchar_t)] = L'\0';
	*linkname = tbuf;
	// 
	// Translate backslashes to slashes for libarchive internal use
	// 
	while(*tbuf != L'\0') {
		if(*tbuf == L'\\')
			*tbuf = L'/';
		tbuf++;
	}
	*linktype = (st.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? AE_SYMLINK_TYPE_DIRECTORY : AE_SYMLINK_TYPE_FILE;
	return 0;
}

/*
 * Returns AE_SYMLINK_TYPE_FILE, AE_SYMLINK_TYPE_DIRECTORY or -1 on error
 */
static int la_linkname_from_pathw(const wchar_t * path, wchar_t ** outbuf, int * linktype)
{
	const DWORD flag = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT;
	int ret;
	HANDLE h = CreateFileW(path, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, flag, NULL);
	if(h == INVALID_HANDLE_VALUE) {
		la_dosmaperr(GetLastError());
		return -1;
	}
	ret = la_linkname_from_handle(h, outbuf, linktype);
	CloseHandle(h);
	return ret;
}

static void entry_symlink_from_pathw(ArchiveEntry * entry, const wchar_t * path)
{
	wchar_t * linkname = NULL;
	int linktype;
	int ret = la_linkname_from_pathw(path, &linkname, &linktype);
	if(ret)
		return;
	if(linktype >= 0) {
		archive_entry_copy_symlink_w(entry, linkname);
		archive_entry_set_symlink_type(entry, linktype);
	}
	SAlloc::F(linkname);
	return;
}

static struct archive_vtable * archive_read_disk_vtable(void)                               
{
	static struct archive_vtable av;
	static int inited = 0;
	if(!inited) {
		av.archive_free = _archive_read_free;
		av.archive_close = _archive_read_close;
		av.archive_read_data_block = _archive_read_data_block;
		av.archive_read_next_header = _archive_read_next_header;
		av.archive_read_next_header2 = _archive_read_next_header2;
		inited = 1;
	}
	return (&av);
}

const char * archive_read_disk_gname(Archive * _a, int64 gid)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	if(ARCHIVE_OK != __archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, "archive_read_disk_gname"))
		return NULL;
	if(a->lookup_gname == NULL)
		return NULL;
	return ((*a->lookup_gname)(a->lookup_gname_data, gid));
}

const char * archive_read_disk_uname(Archive * _a, int64 uid)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	if(ARCHIVE_OK != __archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, "archive_read_disk_uname"))
		return NULL;
	if(a->lookup_uname == NULL)
		return NULL;
	return ((*a->lookup_uname)(a->lookup_uname_data, uid));
}

int archive_read_disk_set_gname_lookup(Archive * _a, void * private_data,
    const char * (*lookup_gname)(void * pPrivate, int64 gid), void (*cleanup_gname)(void * pPrivate))
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(&a->archive, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	if(a->cleanup_gname && a->lookup_gname_data)
		(a->cleanup_gname)(a->lookup_gname_data);
	a->lookup_gname = lookup_gname;
	a->cleanup_gname = cleanup_gname;
	a->lookup_gname_data = private_data;
	return ARCHIVE_OK;
}

int archive_read_disk_set_uname_lookup(Archive * _a, void * private_data,
    const char * (*lookup_uname)(void * pPrivate, int64 uid), void (*cleanup_uname)(void * pPrivate))
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(&a->archive, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	if(a->cleanup_uname && a->lookup_uname_data)
		(a->cleanup_uname)(a->lookup_uname_data);
	a->lookup_uname = lookup_uname;
	a->cleanup_uname = cleanup_uname;
	a->lookup_uname_data = private_data;
	return ARCHIVE_OK;
}
/*
 * Create a new archive_read_disk object and initialize it with global state.
 */
Archive * archive_read_disk_new(void)                 
{
	struct archive_read_disk * a = (struct archive_read_disk *)SAlloc::C(1, sizeof(*a));
	if(!a)
		return NULL;
	a->archive.magic = ARCHIVE_READ_DISK_MAGIC;
	a->archive.state = ARCHIVE_STATE_NEW;
	a->archive.vtable = archive_read_disk_vtable();
	a->entry = archive_entry_new2(&a->archive);
	a->lookup_uname = trivial_lookup_uname;
	a->lookup_gname = trivial_lookup_gname;
	a->flags = ARCHIVE_READDISK_MAC_COPYFILE;
	return (&a->archive);
}

static int _archive_read_free(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	int r = ARCHIVE_OK;
	if(_a) {
		archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY | ARCHIVE_STATE_FATAL, __FUNCTION__);
		if(a->archive.state != ARCHIVE_STATE_CLOSED)
			r = _archive_read_close(&a->archive);
		else
			r = ARCHIVE_OK;
		tree_free(a->tree);
		if(a->cleanup_gname && a->lookup_gname_data)
			(a->cleanup_gname)(a->lookup_gname_data);
		if(a->cleanup_uname && a->lookup_uname_data)
			(a->cleanup_uname)(a->lookup_uname_data);
		archive_string_free(&a->archive.error_string);
		archive_entry_free(a->entry);
		a->archive.magic = 0;
		SAlloc::F(a);
	}
	return r;
}

static int _archive_read_close(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY | ARCHIVE_STATE_FATAL, __FUNCTION__);
	if(a->archive.state != ARCHIVE_STATE_FATAL)
		a->archive.state = ARCHIVE_STATE_CLOSED;
	tree_close(a->tree);
	return ARCHIVE_OK;
}

static void setup_symlink_mode(struct archive_read_disk * a, char symlink_mode,
    int follow_symlinks)
{
	a->symlink_mode = symlink_mode;
	a->follow_symlinks = follow_symlinks;
	if(a->tree) {
		a->tree->initial_symlink_mode = a->symlink_mode;
		a->tree->symlink_mode = a->symlink_mode;
	}
}

int archive_read_disk_set_symlink_logical(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	setup_symlink_mode(a, 'L', 1);
	return ARCHIVE_OK;
}

int archive_read_disk_set_symlink_physical(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	setup_symlink_mode(a, 'P', 0);
	return ARCHIVE_OK;
}

int archive_read_disk_set_symlink_hybrid(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	setup_symlink_mode(a, 'H', 1); /* Follow symlinks initially. */
	return ARCHIVE_OK;
}

int archive_read_disk_set_atime_restored(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	a->flags |= ARCHIVE_READDISK_RESTORE_ATIME;
	if(a->tree)
		a->tree->flags |= needsRestoreTimes;
	return ARCHIVE_OK;
}

int archive_read_disk_set_behavior(Archive * _a, int flags)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	int r = ARCHIVE_OK;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	a->flags = flags;
	if(flags & ARCHIVE_READDISK_RESTORE_ATIME)
		r = archive_read_disk_set_atime_restored(_a);
	else {
		if(a->tree)
			a->tree->flags &= ~needsRestoreTimes;
	}
	return r;
}

/*
 * Trivial implementations of gname/uname lookup functions.
 * These are normally overridden by the client, but these stub
 * versions ensure that we always have something that works.
 */
static const char * trivial_lookup_gname(void * private_data, int64 gid)
{
	(void)private_data; /* UNUSED */
	(void)gid; /* UNUSED */
	return NULL;
}

static const char * trivial_lookup_uname(void * private_data, int64 uid)
{
	(void)private_data; /* UNUSED */
	(void)uid; /* UNUSED */
	return NULL;
}

static int64 align_num_per_sector(struct tree * t, int64 size)
{
	int64 surplus;

	size += t->current_filesystem->bytesPerSector -1;
	surplus = size % t->current_filesystem->bytesPerSector;
	size -= surplus;
	return (size);
}

static int start_next_async_read(struct archive_read_disk * a, struct tree * t)
{
	struct tree::la_overlapped * olp;
	DWORD buffbytes, rbytes;
	if(t->ol_remaining_bytes == 0)
		return (ARCHIVE_EOF);
	olp = &(t->ol[t->ol_idx_doing]);
	t->ol_idx_doing = (t->ol_idx_doing + 1) % MAX_OVERLAPPED;
	/* Allocate read buffer. */
	if(olp->buff == NULL) {
		void * p;
		size_t s = (size_t)align_num_per_sector(t, READ_BUFFER_SIZE);
		p = VirtualAlloc(NULL, s, MEM_COMMIT, PAGE_READWRITE);
		if(!p) {
			archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
			a->archive.state = ARCHIVE_STATE_FATAL;
			return ARCHIVE_FATAL;
		}
		olp->buff = static_cast<uchar *>(p);
		olp->buff_size = s;
		olp->_a = &a->archive;
		olp->ol.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
		if(olp->ol.hEvent == NULL) {
			la_dosmaperr(GetLastError());
			archive_set_error(&a->archive, errno, "CreateEvent failed");
			a->archive.state = ARCHIVE_STATE_FATAL;
			return ARCHIVE_FATAL;
		}
	}
	else
		ResetEvent(olp->ol.hEvent);
	buffbytes = (DWORD)olp->buff_size;
	if(buffbytes > t->current_sparse->length)
		buffbytes = (DWORD)t->current_sparse->length;
	/* Skip hole. */
	if(t->current_sparse->offset > t->ol_total) {
		t->ol_remaining_bytes -= t->current_sparse->offset - t->ol_total;
	}
	olp->offset = t->current_sparse->offset;
	olp->ol.Offset = (DWORD)(olp->offset & 0xffffffff);
	olp->ol.OffsetHigh = (DWORD)(olp->offset >> 32);
	if(t->ol_remaining_bytes > buffbytes) {
		olp->bytes_expected = buffbytes;
		t->ol_remaining_bytes -= buffbytes;
	}
	else {
		olp->bytes_expected = (size_t)t->ol_remaining_bytes;
		t->ol_remaining_bytes = 0;
	}
	olp->bytes_transferred = 0;
	t->current_sparse->offset += buffbytes;
	t->current_sparse->length -= buffbytes;
	t->ol_total = t->current_sparse->offset;
	if(t->current_sparse->length == 0 && t->ol_remaining_bytes > 0)
		t->current_sparse++;
	if(!ReadFile(t->entry_fh, olp->buff, buffbytes, &rbytes, &(olp->ol))) {
		DWORD lasterr = GetLastError();
		if(lasterr == ERROR_HANDLE_EOF) {
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Reading file truncated");
			a->archive.state = ARCHIVE_STATE_FATAL;
			return ARCHIVE_FATAL;
		}
		else if(lasterr != ERROR_IO_PENDING) {
			if(lasterr == ERROR_NO_DATA)
				errno = EAGAIN;
			else if(lasterr == ERROR_ACCESS_DENIED)
				errno = EBADF;
			else
				la_dosmaperr(lasterr);
			archive_set_error(&a->archive, errno, "Read error");
			a->archive.state = ARCHIVE_STATE_FATAL;
			return ARCHIVE_FATAL;
		}
	}
	else
		olp->bytes_transferred = rbytes;
	t->ol_num_doing++;
	return (t->ol_remaining_bytes == 0) ? ARCHIVE_EOF : ARCHIVE_OK;
}

static void cancel_async(struct tree * t)
{
	if(t->ol_num_doing != t->ol_num_done) {
		CancelIo(t->entry_fh);
		t->ol_num_doing = t->ol_num_done = 0;
	}
}

static int _archive_read_data_block(Archive * _a, const void ** buff, size_t * size, int64 * offset)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	struct tree * t = a->tree;
	struct tree::la_overlapped * olp;
	DWORD bytes_transferred;
	int r = ARCHIVE_FATAL;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_DATA, __FUNCTION__);
	if(t->entry_eof || t->entry_remaining_bytes <= 0) {
		r = ARCHIVE_EOF;
		goto abort_read_data;
	}
	/*
	 * Make a request to read the file in asynchronous.
	 */
	if(t->ol_num_doing == 0) {
		do {
			r = start_next_async_read(a, t);
			if(r == ARCHIVE_FATAL)
				goto abort_read_data;
			if(!t->async_io)
				break;
		} while(r == ARCHIVE_OK && t->ol_num_doing < MAX_OVERLAPPED);
	}
	else {
		if((r = start_next_async_read(a, t)) == ARCHIVE_FATAL)
			goto abort_read_data;
	}
	olp = &(t->ol[t->ol_idx_done]);
	t->ol_idx_done = (t->ol_idx_done + 1) % MAX_OVERLAPPED;
	if(olp->bytes_transferred)
		bytes_transferred = (DWORD)olp->bytes_transferred;
	else if(!GetOverlappedResult(t->entry_fh, &(olp->ol), &bytes_transferred, TRUE)) {
		la_dosmaperr(GetLastError());
		archive_set_error(&a->archive, errno, "GetOverlappedResult failed");
		a->archive.state = ARCHIVE_STATE_FATAL;
		r = ARCHIVE_FATAL;
		goto abort_read_data;
	}
	t->ol_num_done++;
	if(bytes_transferred == 0 || olp->bytes_expected != bytes_transferred) {
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Reading file truncated");
		a->archive.state = ARCHIVE_STATE_FATAL;
		r = ARCHIVE_FATAL;
		goto abort_read_data;
	}
	*buff = olp->buff;
	*size = bytes_transferred;
	*offset = olp->offset;
	if(olp->offset > t->entry_total)
		t->entry_remaining_bytes -= olp->offset - t->entry_total;
	t->entry_total = olp->offset + *size;
	t->entry_remaining_bytes -= *size;
	if(t->entry_remaining_bytes == 0) {
		/* Close the current file descriptor */
		close_and_restore_time(t->entry_fh, t, &t->restore_time);
		t->entry_fh = INVALID_HANDLE_VALUE;
		t->entry_eof = 1;
	}
	return ARCHIVE_OK;
abort_read_data:
	*buff = NULL;
	*size = 0;
	*offset = t->entry_total;
	if(t->entry_fh != INVALID_HANDLE_VALUE) {
		cancel_async(t);
		/* Close the current file descriptor */
		close_and_restore_time(t->entry_fh, t, &t->restore_time);
		t->entry_fh = INVALID_HANDLE_VALUE;
	}
	return r;
}

static int next_entry(struct archive_read_disk * a, struct tree * t, ArchiveEntry * entry)
{
	const char* name;
	int descend, r;
	const BY_HANDLE_FILE_INFORMATION * st = NULL;
	const BY_HANDLE_FILE_INFORMATION * lst = NULL;
	t->descend = 0;
	do {
		switch(tree_next(t)) {
			case TREE_ERROR_FATAL:
			    archive_set_error(&a->archive, t->tree_errno, "%ls: Unable to continue traversing directory tree", tree_current_path(t));
			    a->archive.state = ARCHIVE_STATE_FATAL;
			    return ARCHIVE_FATAL;
			case TREE_ERROR_DIR:
			    archive_set_error(&a->archive, t->tree_errno, "%ls: Couldn't visit directory", tree_current_path(t));
			    return ARCHIVE_FAILED;
			case 0:
			    return (ARCHIVE_EOF);
			case TREE_POSTDESCENT:
			case TREE_POSTASCENT:
			    break;
			case TREE_REGULAR:
			    lst = tree_current_lstat(t);
			    if(lst == NULL) {
				    archive_set_error(&a->archive, t->tree_errno, "%ls: Cannot stat", tree_current_path(t));
				    return ARCHIVE_FAILED;
			    }
			    break;
		}
	} while(lst == NULL);

	archive_entry_copy_pathname_w(entry, tree_current_path(t));

	/*
	 * Perform path matching.
	 */
	if(a->matching) {
		r = archive_match_path_excluded(a->matching, entry);
		if(r < 0) {
			archive_set_error(&(a->archive), errno, "Failed : %s", archive_error_string(a->matching));
			return r;
		}
		if(r) {
			if(a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive), a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}
	/*
	 * Distinguish 'L'/'P'/'H' symlink following.
	 */
	switch(t->symlink_mode) {
		case 'H':
		    /* 'H': After the first item, rest like 'P'. */
		    t->symlink_mode = 'P';
		/* 'H': First item (from command line) like 'L'. */
		// @fallthrough
		case 'L':
		    /* 'L': Do descend through a symlink to dir. */
		    descend = tree_current_is_dir(t);
		    /* 'L': Follow symlinks to files. */
		    a->symlink_mode = 'L';
		    a->follow_symlinks = 1;
		    /* 'L': Archive symlinks as targets, if we can. */
		    st = tree_current_stat(t);
		    if(st && !tree_target_is_same_as_parent(t, st))
			    break;
		/* If stat fails, we have a broken symlink;
		 * in that case, don't follow the link. */
		// @fallthrough
		default:
		    /* 'P': Don't descend through a symlink to dir. */
		    descend = tree_current_is_physical_dir(t);
		    /* 'P': Don't follow symlinks to files. */
		    a->symlink_mode = 'P';
		    a->follow_symlinks = 0;
		    /* 'P': Archive symlinks as symlinks. */
		    st = lst;
		    break;
	}

	if(update_current_filesystem(a, bhfi_dev(st)) != ARCHIVE_OK) {
		a->archive.state = ARCHIVE_STATE_FATAL;
		return ARCHIVE_FATAL;
	}
	if(t->initial_filesystem_id == -1)
		t->initial_filesystem_id = t->current_filesystem_id;
	if(a->flags & ARCHIVE_READDISK_NO_TRAVERSE_MOUNTS) {
		if(t->initial_filesystem_id != t->current_filesystem_id)
			return (ARCHIVE_RETRY);
	}
	t->descend = descend;

	tree_archive_entry_copy_bhfi(entry, t, st);

	/* Save the times to be restored. This must be in before
	 * calling archive_read_disk_descend() or any chance of it,
	 * especially, invoking a callback. */
	t->restore_time.lastWriteTime = st->ftLastWriteTime;
	t->restore_time.lastAccessTime = st->ftLastAccessTime;
	t->restore_time.filetype = archive_entry_filetype(entry);

	/*
	 * Perform time matching.
	 */
	if(a->matching) {
		r = archive_match_time_excluded(a->matching, entry);
		if(r < 0) {
			archive_set_error(&(a->archive), errno, "Failed : %s", archive_error_string(a->matching));
			return r;
		}
		if(r) {
			if(a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive), a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}
	/* Lookup uname/gname */
	name = archive_read_disk_uname(&(a->archive), archive_entry_uid(entry));
	if(name)
		archive_entry_copy_uname(entry, name);
	name = archive_read_disk_gname(&(a->archive), archive_entry_gid(entry));
	if(name)
		archive_entry_copy_gname(entry, name);
	/*
	 * Perform owner matching.
	 */
	if(a->matching) {
		r = archive_match_owner_excluded(a->matching, entry);
		if(r < 0) {
			archive_set_error(&(a->archive), errno, "Failed : %s", archive_error_string(a->matching));
			return r;
		}
		if(r) {
			if(a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive), a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}
	/*
	 * File attributes
	 */
	if((a->flags & ARCHIVE_READDISK_NO_FFLAGS) == 0) {
		const int supported_attrs =
		    FILE_ATTRIBUTE_READONLY |
		    FILE_ATTRIBUTE_HIDDEN |
		    FILE_ATTRIBUTE_SYSTEM;
		DWORD file_attrs = st->dwFileAttributes & supported_attrs;
		if(file_attrs != 0)
			archive_entry_set_fflags(entry, file_attrs, 0);
	}
	/*
	 * Invoke a meta data filter callback.
	 */
	if(a->metadata_filter_func) {
		if(!a->metadata_filter_func(&(a->archive), a->metadata_filter_data, entry))
			return (ARCHIVE_RETRY);
	}
	archive_entry_copy_sourcepath_w(entry, tree_current_access_path(t));
	r = ARCHIVE_OK;
	if(archive_entry_filetype(entry) == AE_IFREG && archive_entry_size(entry) > 0) {
		DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
		if(t->async_io)
			flags |= FILE_FLAG_OVERLAPPED;
		if(t->direct_io)
			flags |= FILE_FLAG_NO_BUFFERING;
		else
			flags |= FILE_FLAG_SEQUENTIAL_SCAN;
		t->entry_fh = CreateFileW(tree_current_access_path(t),
			GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, flags, NULL);
		if(t->entry_fh == INVALID_HANDLE_VALUE) {
			la_dosmaperr(GetLastError());
			archive_set_error(&a->archive, errno, "Couldn't open %ls", tree_current_path(a->tree));
			return ARCHIVE_FAILED;
		}
		/* Find sparse data from the disk. */
		if(archive_entry_hardlink(entry) == NULL && (st->dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0)
			r = setup_sparse_from_disk(a, entry, t->entry_fh);
	}
	return r;
}

static int _archive_read_next_header(Archive * _a, ArchiveEntry ** entryp)
{
	int ret;
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	*entryp = NULL;
	ret = _archive_read_next_header2(_a, a->entry);
	*entryp = a->entry;
	return ret;
}

static int _archive_read_next_header2(Archive * _a, ArchiveEntry * entry)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	int r;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA, __FUNCTION__);
	struct tree * t = a->tree;
	if(t->entry_fh != INVALID_HANDLE_VALUE) {
		cancel_async(t);
		close_and_restore_time(t->entry_fh, t, &t->restore_time);
		t->entry_fh = INVALID_HANDLE_VALUE;
	}
	archive_entry_clear(entry);
	while((r = next_entry(a, t, entry)) == ARCHIVE_RETRY)
		archive_entry_clear(entry);
	/*
	 * EOF and FATAL are persistent at this layer.  By
	 * modifying the state, we guarantee that future calls to
	 * read a header or read data will fail.
	 */
	switch(r) {
		case ARCHIVE_EOF:
		    a->archive.state = ARCHIVE_STATE_EOF;
		    break;
		case ARCHIVE_OK:
		case ARCHIVE_WARN:
		    t->entry_total = 0;
		    if(archive_entry_filetype(entry) == AE_IFREG) {
			    t->entry_remaining_bytes = archive_entry_size(entry);
			    t->entry_eof = (t->entry_remaining_bytes == 0) ? 1 : 0;
			    if(!t->entry_eof && setup_sparse(a, entry) != ARCHIVE_OK)
				    return ARCHIVE_FATAL;
		    }
		    else {
			    t->entry_remaining_bytes = 0;
			    t->entry_eof = 1;
		    }
		    t->ol_idx_doing = t->ol_idx_done = 0;
		    t->ol_num_doing = t->ol_num_done = 0;
		    t->ol_remaining_bytes = t->entry_remaining_bytes;
		    t->ol_total = 0;
		    a->archive.state = ARCHIVE_STATE_DATA;
		    break;
		case ARCHIVE_RETRY:
		    break;
		case ARCHIVE_FATAL:
		    a->archive.state = ARCHIVE_STATE_FATAL;
		    break;
	}
	__archive_reset_read_data(&a->archive);
	return r;
}

static int setup_sparse(struct archive_read_disk * a, ArchiveEntry * entry)
{
	struct tree * t = a->tree;
	int64 aligned, length, offset;
	int i;
	t->sparse_count = archive_entry_sparse_reset(entry);
	if(t->sparse_count+1 > t->sparse_list_size) {
		SAlloc::F(t->sparse_list);
		t->sparse_list_size = t->sparse_count + 1;
		t->sparse_list = static_cast<struct tree::entry_sparse *>(SAlloc::M(sizeof(t->sparse_list[0]) * t->sparse_list_size));
		if(t->sparse_list == NULL) {
			t->sparse_list_size = 0;
			archive_set_error(&a->archive, ENOMEM, "Can't allocate data");
			a->archive.state = ARCHIVE_STATE_FATAL;
			return ARCHIVE_FATAL;
		}
	}
	/*
	 * Get sparse list and make sure those offsets and lengths are
	 * aligned by a sector size.
	 */
	for(i = 0; i < t->sparse_count; i++) {
		archive_entry_sparse_next(entry, &offset, &length);
		aligned = align_num_per_sector(t, offset);
		if(aligned != offset) {
			aligned -= t->current_filesystem->bytesPerSector;
			length += offset - aligned;
		}
		t->sparse_list[i].offset = aligned;
		aligned = align_num_per_sector(t, length);
		t->sparse_list[i].length = aligned;
	}
	aligned = align_num_per_sector(t, archive_entry_size(entry));
	if(i == 0) {
		t->sparse_list[i].offset = 0;
		t->sparse_list[i].length = aligned;
	}
	else {
		int j, last = i;
		t->sparse_list[i].offset = aligned;
		t->sparse_list[i].length = 0;
		for(i = 0; i < last; i++) {
			if((t->sparse_list[i].offset + t->sparse_list[i].length) <= t->sparse_list[i+1].offset)
				continue;
			/*
			 * Now sparse_list[i+1] is overlapped by sparse_list[i].
			 * Merge those two.
			 */
			length = t->sparse_list[i+1].offset - t->sparse_list[i].offset;
			t->sparse_list[i+1].offset = t->sparse_list[i].offset;
			t->sparse_list[i+1].length += length;
			/* Remove sparse_list[i]. */
			for(j = i; j < last; j++) {
				t->sparse_list[j].offset = t->sparse_list[j+1].offset;
				t->sparse_list[j].length = t->sparse_list[j+1].length;
			}
			last--;
		}
	}
	t->current_sparse = t->sparse_list;
	return ARCHIVE_OK;
}

int archive_read_disk_set_matching(Archive * _a, Archive * _ma, void (*_excluded_func)(Archive *, void *, ArchiveEntry *), void * _client_data)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	a->matching = _ma;
	a->excluded_cb_func = _excluded_func;
	a->excluded_cb_data = _client_data;
	return ARCHIVE_OK;
}

int archive_read_disk_set_metadata_filter_callback(Archive * _a, int (*_metadata_filter_func)(Archive *, void *, ArchiveEntry *), void * _client_data)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_ANY, __FUNCTION__);
	a->metadata_filter_func = _metadata_filter_func;
	a->metadata_filter_data = _client_data;
	return ARCHIVE_OK;
}

int archive_read_disk_can_descend(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	struct tree * t = a->tree;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA, __FUNCTION__);
	return (t->visit_type == TREE_REGULAR && t->descend);
}

/*
 * Called by the client to mark the directory just returned from
 * tree_next() as needing to be visited.
 */
int archive_read_disk_descend(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	struct tree * t = a->tree;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA, __FUNCTION__);
	if(t->visit_type != TREE_REGULAR || !t->descend)
		return ARCHIVE_OK;
	if(tree_current_is_physical_dir(t)) {
		tree_push(t, t->basename, t->full_path.s, t->current_filesystem_id, bhfi_dev(&(t->lst)), bhfi_ino(&(t->lst)), &t->restore_time);
		t->stack->flags |= isDir;
	}
	else if(tree_current_is_dir(t)) {
		tree_push(t, t->basename, t->full_path.s, t->current_filesystem_id, bhfi_dev(&(t->st)), bhfi_ino(&(t->st)), &t->restore_time);
		t->stack->flags |= isDirLink;
	}
	t->descend = 0;
	return ARCHIVE_OK;
}

int archive_read_disk_open(Archive * _a, const char * pathname)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_wstring wpath;
	int ret;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_NEW | ARCHIVE_STATE_CLOSED, __FUNCTION__);
	archive_clear_error(&a->archive);
	/* Make a wchar_t string from a char string. */
	archive_string_init(&wpath);
	if(archive_wstring_append_from_mbs(&wpath, pathname, strlen(pathname)) != 0) {
		if(errno == ENOMEM)
			archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
		else
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "Can't convert a path to a wchar_t string");
		a->archive.state = ARCHIVE_STATE_FATAL;
		ret = ARCHIVE_FATAL;
	}
	else
		ret = _archive_read_disk_open_w(_a, wpath.s);
	archive_wstring_free(&wpath);
	return ret;
}

int archive_read_disk_open_w(Archive * _a, const wchar_t * pathname)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_NEW | ARCHIVE_STATE_CLOSED, __FUNCTION__);
	archive_clear_error(&a->archive);
	return (_archive_read_disk_open_w(_a, pathname));
}

static int _archive_read_disk_open_w(Archive * _a, const wchar_t * pathname)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	if(a->tree)
		a->tree = tree_reopen(a->tree, pathname, a->flags & ARCHIVE_READDISK_RESTORE_ATIME);
	else
		a->tree = tree_open(pathname, a->symlink_mode, a->flags & ARCHIVE_READDISK_RESTORE_ATIME);
	if(a->tree == NULL) {
		archive_set_error(&a->archive, ENOMEM, "Can't allocate directory traversal data");
		a->archive.state = ARCHIVE_STATE_FATAL;
		return ARCHIVE_FATAL;
	}
	a->archive.state = ARCHIVE_STATE_HEADER;
	return ARCHIVE_OK;
}
/*
 * Return a current filesystem ID which is index of the filesystem entry
 * you've visited through archive_read_disk.
 */
int archive_read_disk_current_filesystem(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_DATA, __FUNCTION__);
	return (a->tree->current_filesystem_id);
}

static int update_current_filesystem(struct archive_read_disk * a, int64 dev)
{
	struct tree * t = a->tree;
	int i, fid;
	if(t->current_filesystem && t->current_filesystem->dev == dev)
		return ARCHIVE_OK;
	for(i = 0; i < t->max_filesystem_id; i++) {
		if(t->filesystem_table[i].dev == dev) {
			/* There is the filesystem ID we've already generated. */
			t->current_filesystem_id = i;
			t->current_filesystem = &(t->filesystem_table[i]);
			return ARCHIVE_OK;
		}
	}
	/*
	 * There is a new filesystem, we generate a new ID for.
	 */
	fid = t->max_filesystem_id++;
	if(t->max_filesystem_id > t->allocated_filesystem) {
		size_t s = t->max_filesystem_id * 2;
		void * p = SAlloc::R(t->filesystem_table, s * sizeof(*t->filesystem_table));
		if(!p) {
			archive_set_error(&a->archive, ENOMEM, "Can't allocate tar data");
			return ARCHIVE_FATAL;
		}
		t->filesystem_table = (struct filesystem *)p;
		t->allocated_filesystem = (int)s;
	}
	t->current_filesystem_id = fid;
	t->current_filesystem = &(t->filesystem_table[fid]);
	t->current_filesystem->dev = dev;
	return (setup_current_filesystem(a));
}
/*
 * Returns 1 if current filesystem is generated filesystem, 0 if it is not
 * or -1 if it is unknown.
 */
int archive_read_disk_current_filesystem_is_synthetic(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_DATA, __FUNCTION__);
	return (a->tree->current_filesystem->synthetic);
}

/*
 * Returns 1 if current filesystem is remote filesystem, 0 if it is not
 * or -1 if it is unknown.
 */
int archive_read_disk_current_filesystem_is_remote(Archive * _a)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC, ARCHIVE_STATE_DATA, __FUNCTION__);
	return (a->tree->current_filesystem->remote);
}

/*
 * If symlink is broken, statfs or statvfs will fail.
 * Use its directory path instead.
 */
static wchar_t * safe_path_for_statfs(struct tree * t)
{
	wchar_t * cp, * p = NULL;
	const wchar_t * path = tree_current_access_path(t);
	if(tree_current_stat(t) == NULL) {
		p = _wcsdup(path);
		cp = wcsrchr(p, '/');
		if(cp && wcslen(cp) >= 2) {
			cp[1] = '.';
			cp[2] = '\0';
			path = p;
		}
	}
	else
		p = _wcsdup(path);
	return (p);
}
/*
 * Get conditions of synthetic and remote on Windows
 */
static int setup_current_filesystem(struct archive_read_disk * a)
{
	struct tree * t = a->tree;
	wchar_t vol[256];
	wchar_t * path;
	t->current_filesystem->synthetic = -1; // Not supported
	path = safe_path_for_statfs(t);
	if(!GetVolumePathNameW(path, vol, SIZEOFARRAY(vol))) {
		SAlloc::F(path);
		t->current_filesystem->remote = -1;
		t->current_filesystem->bytesPerSector = 0;
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "GetVolumePathName failed: %d", (int)GetLastError());
		return ARCHIVE_FAILED;
	}
	SAlloc::F(path);
	switch(GetDriveTypeW(vol)) {
		case DRIVE_UNKNOWN:
		case DRIVE_NO_ROOT_DIR: t->current_filesystem->remote = -1; break;
		case DRIVE_REMOTE: t->current_filesystem->remote = 1; break;
		default: t->current_filesystem->remote = 0; break;
	}
	if(!GetDiskFreeSpaceW(vol, NULL, &(t->current_filesystem->bytesPerSector), NULL, NULL)) {
		t->current_filesystem->bytesPerSector = 0;
		archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC, "GetDiskFreeSpace failed: %d", (int)GetLastError());
		return ARCHIVE_FAILED;
	}
	return ARCHIVE_OK;
}

static int close_and_restore_time(HANDLE h, struct tree * t, struct restore_time * rt)
{
	HANDLE handle;
	int r = 0;
	if(h == INVALID_HANDLE_VALUE && AE_IFLNK == rt->filetype)
		return 0;
	/* Close a file descriptor.
	 * It will not be used for SetFileTime() because it has been opened
	 * by a read only mode.
	 */
	if(h != INVALID_HANDLE_VALUE)
		CloseHandle(h);
	if((t->flags & needsRestoreTimes) == 0)
		return r;
	handle = CreateFileW(rt->full_path, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if(handle == INVALID_HANDLE_VALUE) {
		errno = EINVAL;
		return -1;
	}
	if(SetFileTime(handle, NULL, &rt->lastAccessTime, &rt->lastWriteTime) == 0) {
		errno = EINVAL;
		r = -1;
	}
	else
		r = 0;
	CloseHandle(handle);
	return r;
}
/*
 * Add a directory path to the current stack.
 */
static void tree_push(struct tree * t, const wchar_t * path, const wchar_t * full_path,
    int filesystem_id, int64 dev, int64 ino, struct restore_time * rt)
{
	struct tree_entry * te = static_cast<struct tree_entry *>(SAlloc::C(1, sizeof(*te)));
	te->next = t->stack;
	te->parent = t->current;
	if(te->parent)
		te->depth = te->parent->depth + 1;
	t->stack = te;
	archive_string_init(&te->name);
	archive_wstrcpy(&te->name, path);
	archive_string_init(&te->full_path);
	archive_wstrcpy(&te->full_path, full_path);
	te->flags = needsDescent | needsOpen | needsAscent;
	te->filesystem_id = filesystem_id;
	te->dev = dev;
	te->ino = ino;
	te->dirname_length = t->dirname_length;
	te->full_path_dir_length = t->full_path_dir_length;
	te->restore_time.full_path = te->full_path.s;
	if(rt) {
		te->restore_time.lastWriteTime = rt->lastWriteTime;
		te->restore_time.lastAccessTime = rt->lastAccessTime;
		te->restore_time.filetype = rt->filetype;
	}
}
/*
 * Append a name to the current dir path.
 */
static void tree_append(struct tree * t, const wchar_t * name, size_t name_length)
{
	size_t size_needed;
	t->path.s[t->dirname_length] = L'\0';
	t->path.length = t->dirname_length;
	/* Strip trailing '/' from name, unless entire name is "/". */
	while(name_length > 1 && name[name_length - 1] == L'/')
		name_length--;
	/* Resize pathname buffer as needed. */
	size_needed = name_length + t->dirname_length + 2;
	archive_wstring_ensure(&t->path, size_needed);
	/* Add a separating '/' if it's needed. */
	if(t->dirname_length > 0 && t->path.s[archive_strlen(&t->path)-1] != L'/')
		archive_wstrappend_wchar(&t->path, L'/');
	t->basename = t->path.s + archive_strlen(&t->path);
	archive_wstrncat(&t->path, name, name_length);
	t->restore_time.full_path = t->basename;
	if(t->full_path_dir_length > 0) {
		t->full_path.s[t->full_path_dir_length] = L'\0';
		t->full_path.length = t->full_path_dir_length;
		size_needed = name_length + t->full_path_dir_length + 2;
		archive_wstring_ensure(&t->full_path, size_needed);
		/* Add a separating '\' if it's needed. */
		if(t->full_path.s[archive_strlen(&t->full_path)-1] != L'\\')
			archive_wstrappend_wchar(&t->full_path, L'\\');
		archive_wstrncat(&t->full_path, name, name_length);
		t->restore_time.full_path = t->full_path.s;
	}
}
/*
 * Open a directory tree for traversal.
 */
static struct tree * tree_open(const wchar_t * path, int symlink_mode, int restore_time)                      
{
	struct tree * t = static_cast<struct tree *>(SAlloc::C(1, sizeof(*t)));
	archive_string_init(&(t->full_path));
	archive_string_init(&t->path);
	archive_wstring_ensure(&t->path, 15);
	t->initial_symlink_mode = symlink_mode;
	return (tree_reopen(t, path, restore_time));
}

static struct tree * tree_reopen(struct tree * t, const wchar_t * path, int restore_time)                       
{
	archive_wstring ws;
	wchar_t * pathname, * p, * base;
	t->flags = (restore_time != 0) ? needsRestoreTimes : 0;
	t->visit_type = 0;
	t->tree_errno = 0;
	t->full_path_dir_length = 0;
	t->dirname_length = 0;
	t->depth = 0;
	t->descend = 0;
	t->current = NULL;
	t->d = INVALID_HANDLE_VALUE;
	t->symlink_mode = t->initial_symlink_mode;
	archive_string_empty(&(t->full_path));
	archive_string_empty(&t->path);
	t->entry_fh = INVALID_HANDLE_VALUE;
	t->entry_eof = 0;
	t->entry_remaining_bytes = 0;
	t->initial_filesystem_id = -1;

	/* Get wchar_t strings from char strings. */
	archive_string_init(&ws);
	archive_wstrcpy(&ws, path);
	pathname = ws.s;
	/* Get a full-path-name. */
	p = __la_win_permissive_name_w(pathname);
	if(!p)
		goto failed;
	archive_wstrcpy(&(t->full_path), p);
	SAlloc::F(p);

	/* Convert path separators from '\' to '/' */
	for(p = pathname; *p != L'\0'; ++p) {
		if(*p == L'\\')
			*p = L'/';
	}
	base = pathname;

	/* First item is set up a lot like a symlink traversal. */
	/* printf("Looking for wildcard in %s\n", path); */
	if((base[0] == L'/' && base[1] == L'/' &&
	    base[2] == L'?' && base[3] == L'/' &&
	    (wcschr(base+4, L'*') || wcschr(base+4, L'?'))) ||
	    (!(base[0] == L'/' && base[1] == L'/' &&
	    base[2] == L'?' && base[3] == L'/') &&
	    (wcschr(base, L'*') || wcschr(base, L'?')))) {
		// It has a wildcard in it...
		// Separate the last element.
		p = wcsrchr(base, L'/');
		if(p) {
			*p = L'\0';
			tree_append(t, base, p - base);
			t->dirname_length = archive_strlen(&t->path);
			base = p + 1;
		}
		p = wcsrchr(t->full_path.s, L'\\');
		if(p) {
			*p = L'\0';
			t->full_path.length = wcslen(t->full_path.s);
			t->full_path_dir_length = archive_strlen(&t->full_path);
		}
	}
	tree_push(t, base, t->full_path.s, 0, 0, 0, NULL);
	archive_wstring_free(&ws);
	t->stack->flags = needsFirstVisit;
	/*
	 * Debug flag for Direct IO(No buffering) or Async IO.
	 * Those dependent on environment variable switches
	 * will be removed until next release.
	 */
	{
		const char * e;
		if((e = getenv("LIBARCHIVE_DIRECT_IO")) != NULL) {
			if(e[0] == '0')
				t->direct_io = 0;
			else
				t->direct_io = 1;
			slfprintf_stderr("LIBARCHIVE_DIRECT_IO=%s\n", (t->direct_io) ? "Enabled" : "Disabled");
		}
		else
			t->direct_io = DIRECT_IO;
		if((e = getenv("LIBARCHIVE_ASYNC_IO")) != NULL) {
			if(e[0] == '0')
				t->async_io = 0;
			else
				t->async_io = 1;
			slfprintf_stderr("LIBARCHIVE_ASYNC_IO=%s\n", (t->async_io) ? "Enabled" : "Disabled");
		}
		else
			t->async_io = ASYNC_IO;
	}
	return (t);
failed:
	archive_wstring_free(&ws);
	tree_free(t);
	return NULL;
}

static int tree_descent(struct tree * t)
{
	t->dirname_length = archive_strlen(&t->path);
	t->full_path_dir_length = archive_strlen(&t->full_path);
	t->depth++;
	return 0;
}

/*
 * We've finished a directory; ascend back to the parent.
 */
static int tree_ascend(struct tree * t)
{
	struct tree_entry * te;

	te = t->stack;
	t->depth--;
	close_and_restore_time(INVALID_HANDLE_VALUE, t, &te->restore_time);
	return 0;
}

/*
 * Pop the working stack.
 */
static void tree_pop(struct tree * t)
{
	struct tree_entry * te;

	t->full_path.s[t->full_path_dir_length] = L'\0';
	t->full_path.length = t->full_path_dir_length;
	t->path.s[t->dirname_length] = L'\0';
	t->path.length = t->dirname_length;
	if(t->stack == t->current && t->current)
		t->current = t->current->parent;
	te = t->stack;
	t->stack = te->next;
	t->dirname_length = te->dirname_length;
	t->basename = t->path.s + t->dirname_length;
	t->full_path_dir_length = te->full_path_dir_length;
	while(t->basename[0] == L'/')
		t->basename++;
	archive_wstring_free(&te->name);
	archive_wstring_free(&te->full_path);
	SAlloc::F(te);
}

/*
 * Get the next item in the tree traversal.
 */
static int tree_next(struct tree * t)
{
	int r;
	while(t->stack) {
		/* If there's an open dir, get the next entry from there. */
		if(t->d != INVALID_HANDLE_VALUE) {
			r = tree_dir_next_windows(t, NULL);
			if(!r)
				continue;
			return r;
		}
		if(t->stack->flags & needsFirstVisit) {
			wchar_t * d = t->stack->name.s;
			t->stack->flags &= ~needsFirstVisit;
			if(!(d[0] == L'/' && d[1] == L'/' && d[2] == L'?' && d[3] == L'/') && (wcschr(d, L'*') || wcschr(d, L'?'))) {
				r = tree_dir_next_windows(t, d);
				if(!r)
					continue;
				return r;
			}
			else {
				HANDLE h = FindFirstFileW(t->stack->full_path.s, &t->_findData); // @sobolev (upd-from-repo) (d)-->(t->stack->full_path.s) 
				if(h == INVALID_HANDLE_VALUE) {
					la_dosmaperr(GetLastError());
					t->tree_errno = errno;
					t->visit_type = TREE_ERROR_DIR;
					return (t->visit_type);
				}
				t->findData = &t->_findData;
				FindClose(h);
			}
			/* Top stack item needs a regular visit. */
			t->current = t->stack;
			tree_append(t, t->stack->name.s,
			    archive_strlen(&(t->stack->name)));
			//t->dirname_length = t->path_length;
			//tree_pop(t);
			t->stack->flags &= ~needsFirstVisit;
			return (t->visit_type = TREE_REGULAR);
		}
		else if(t->stack->flags & needsDescent) {
			/* Top stack item is dir to descend into. */
			t->current = t->stack;
			tree_append(t, t->stack->name.s,
			    archive_strlen(&(t->stack->name)));
			t->stack->flags &= ~needsDescent;
			r = tree_descent(t);
			if(r) {
				tree_pop(t);
				t->visit_type = r;
			}
			else
				t->visit_type = TREE_POSTDESCENT;
			return (t->visit_type);
		}
		else if(t->stack->flags & needsOpen) {
			t->stack->flags &= ~needsOpen;
			r = tree_dir_next_windows(t, L"*");
			if(!r)
				continue;
			return r;
		}
		else if(t->stack->flags & needsAscent) {
			/* Top stack item is dir and we're done with it. */
			r = tree_ascend(t);
			tree_pop(t);
			t->visit_type = r != 0 ? r : TREE_POSTASCENT;
			return (t->visit_type);
		}
		else {
			/* Top item on stack is dead. */
			tree_pop(t);
			t->flags &= ~hasLstat;
			t->flags &= ~hasStat;
		}
	}
	return (t->visit_type = 0);
}

static int tree_dir_next_windows(struct tree * t, const wchar_t * pattern)
{
	const wchar_t * name;
	size_t namelen;
	int r;
	for(;;) {
		if(pattern) {
			archive_wstring pt;
			archive_string_init(&pt);
			archive_wstring_ensure(&pt, archive_strlen(&(t->full_path)) + 2 + wcslen(pattern));
			archive_wstring_copy(&pt, &(t->full_path));
			archive_wstrappend_wchar(&pt, L'\\');
			archive_wstrcat(&pt, pattern);
			t->d = FindFirstFileW(pt.s, &t->_findData);
			archive_wstring_free(&pt);
			if(t->d == INVALID_HANDLE_VALUE) {
				la_dosmaperr(GetLastError());
				t->tree_errno = errno;
				r = tree_ascend(t); /* Undo "chdir" */
				tree_pop(t);
				t->visit_type = r != 0 ? r : TREE_ERROR_DIR;
				return (t->visit_type);
			}
			t->findData = &t->_findData;
			pattern = NULL;
		}
		else if(!FindNextFileW(t->d, &t->_findData)) {
			FindClose(t->d);
			t->d = INVALID_HANDLE_VALUE;
			t->findData = NULL;
			return 0;
		}
		name = t->findData->cFileName;
		namelen = wcslen(name);
		t->flags &= ~hasLstat;
		t->flags &= ~hasStat;
		if(name[0] == L'.' && name[1] == L'\0')
			continue;
		if(name[0] == L'.' && name[1] == L'.' && name[2] == L'\0')
			continue;
		tree_append(t, name, namelen);
		return (t->visit_type = TREE_REGULAR);
	}
}

//#define EPOC_TIME ARCHIVE_LITERAL_ULL(116444736000000000)
static void fileTimeToUtc(const FILETIME * filetime, time_t * t, long * ns)
{
	ULARGE_INTEGER utc;
	utc.HighPart = filetime->dwHighDateTime;
	utc.LowPart  = filetime->dwLowDateTime;
	if(utc.QuadPart >= SlConst::Epoch1600_1970_Offs_100Ns) {
		utc.QuadPart -= SlConst::Epoch1600_1970_Offs_100Ns;
		/* milli seconds base */
		*t = (time_t)(utc.QuadPart / 10000000);
		/* nano seconds base */
		*ns = (long)(utc.QuadPart % 10000000) * 100;
	}
	else {
		*t = 0;
		*ns = 0;
	}
}

static void entry_copy_bhfi(ArchiveEntry * entry, const wchar_t * path, const WIN32_FIND_DATAW * findData,
    const BY_HANDLE_FILE_INFORMATION * bhfi)
{
	time_t secs;
	long nsecs;
	mode_t mode;
	fileTimeToUtc(&bhfi->ftLastAccessTime, &secs, &nsecs);
	archive_entry_set_atime(entry, secs, nsecs);
	fileTimeToUtc(&bhfi->ftLastWriteTime, &secs, &nsecs);
	archive_entry_set_mtime(entry, secs, nsecs);
	fileTimeToUtc(&bhfi->ftCreationTime, &secs, &nsecs);
	archive_entry_set_birthtime(entry, secs, nsecs);
	archive_entry_set_ctime(entry, secs, nsecs);
	archive_entry_set_dev(entry, bhfi_dev(bhfi));
	archive_entry_set_ino64(entry, bhfi_ino(bhfi));
	if(bhfi->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		archive_entry_set_nlink(entry, bhfi->nNumberOfLinks + 1);
	else
		archive_entry_set_nlink(entry, bhfi->nNumberOfLinks);
	archive_entry_set_size(entry, (((int64)bhfi->nFileSizeHigh) << 32) + bhfi->nFileSizeLow);
	archive_entry_set_uid(entry, 0);
	archive_entry_set_gid(entry, 0);
	archive_entry_set_rdev(entry, 0);
	mode = S_IRUSR | S_IRGRP | S_IROTH;
	if((bhfi->dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0)
		mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	if((bhfi->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && findData && findData->dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
		mode |= S_IFLNK;
		entry_symlink_from_pathw(entry, path);
	}
	else if(bhfi->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
	else {
		const wchar_t * p;
		mode |= S_IFREG;
		p = wcsrchr(path, L'.');
		if(p && wcslen(p) == 4) {
			switch(p[1]) {
				case L'B': case L'b':
				    if((p[2] == L'A' || p[2] == L'a') && (p[3] == L'T' || p[3] == L't'))
					    mode |= S_IXUSR | S_IXGRP | S_IXOTH;
				    break;
				case L'C': case L'c':
				    if(((p[2] == L'M' || p[2] == L'm' ) && (p[3] == L'D' || p[3] == L'd' )))
					    mode |= S_IXUSR | S_IXGRP | S_IXOTH;
				    break;
				case L'E': case L'e':
				    if(oneof2(p[2], L'X', L'x') && oneof2(p[3], L'E', L'e'))
					    mode |= S_IXUSR | S_IXGRP | S_IXOTH;
				    break;
				default:
				    break;
			}
		}
	}
	archive_entry_set_mode(entry, mode);
}

static void tree_archive_entry_copy_bhfi(ArchiveEntry * entry, struct tree * t, const BY_HANDLE_FILE_INFORMATION * bhfi)
{
	entry_copy_bhfi(entry, tree_current_path(t), t->findData, bhfi);
}

static int tree_current_file_information(struct tree * t, BY_HANDLE_FILE_INFORMATION * st, int sim_lstat)
{
	HANDLE h;
	int r;
	DWORD flag = FILE_FLAG_BACKUP_SEMANTICS;
	if(sim_lstat && tree_current_is_physical_link(t))
		flag |= FILE_FLAG_OPEN_REPARSE_POINT;
	h = CreateFileW(tree_current_access_path(t), 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, flag, NULL);
	if(h == INVALID_HANDLE_VALUE) {
		la_dosmaperr(GetLastError());
		t->tree_errno = errno;
		return 0;
	}
	r = GetFileInformationByHandle(h, st);
	CloseHandle(h);
	return r;
}
/*
 * Get the stat() data for the entry just returned from tree_next().
 */
static const BY_HANDLE_FILE_INFORMATION * tree_current_stat(struct tree * t)
{
	if(!(t->flags & hasStat)) {
		if(!tree_current_file_information(t, &t->st, 0))
			return NULL;
		t->flags |= hasStat;
	}
	return (&t->st);
}
/*
 * Get the lstat() data for the entry just returned from tree_next().
 */
static const BY_HANDLE_FILE_INFORMATION * tree_current_lstat(struct tree * t)
{
	if(!(t->flags & hasLstat)) {
		if(!tree_current_file_information(t, &t->lst, 1))
			return NULL;
		t->flags |= hasLstat;
	}
	return (&t->lst);
}
/*
 * Test whether current entry is a dir or link to a dir.
 */
static int tree_current_is_dir(struct tree * t)
{
	return t->findData ? (t->findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) : 0;
}
/*
 * Test whether current entry is a physical directory.  Usually, we
 * already have at least one of stat() or lstat() in memory, so we
 * use tricks to try to avoid an extra trip to the disk.
 */
static int tree_current_is_physical_dir(struct tree * t)
{
	return tree_current_is_physical_link(t) ? 0 : tree_current_is_dir(t);
}
/*
 * Test whether current entry is a symbolic link.
 */
static int tree_current_is_physical_link(struct tree * t)
{
	return t->findData ? ((t->findData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (t->findData->dwReserved0 == IO_REPARSE_TAG_SYMLINK)) : 0;
}

/*
 * Test whether the same file has been in the tree as its parent.
 */
static int tree_target_is_same_as_parent(struct tree * t, const BY_HANDLE_FILE_INFORMATION * st)
{
	int64 dev = bhfi_dev(st);
	int64 ino = bhfi_ino(st);
	for(struct tree_entry * te = t->current->parent; te; te = te->parent) {
		if(te->dev == dev && te->ino == ino)
			return 1;
	}
	return 0;
}
/*
 * Return the access path for the entry just returned from tree_next().
 */
static const wchar_t * tree_current_access_path(struct tree * t) { return (t->full_path.s); }
/*
 * Return the full path for the entry just returned from tree_next().
 */
static const wchar_t * tree_current_path(struct tree * t) { return (t->path.s); }
/*
 * Terminate the traversal.
 */
static void tree_close(struct tree * t)
{
	if(t) {
		if(t->entry_fh != INVALID_HANDLE_VALUE) {
			cancel_async(t);
			close_and_restore_time(t->entry_fh, t, &t->restore_time);
			t->entry_fh = INVALID_HANDLE_VALUE;
		}
		/* Close the handle of FindFirstFileW */
		if(t->d != INVALID_HANDLE_VALUE) {
			FindClose(t->d);
			t->d = INVALID_HANDLE_VALUE;
			t->findData = NULL;
		}
		/* Release anything remaining in the stack. */
		while(t->stack)
			tree_pop(t);
	}
}
/*
 * Release any resources.
 */
static void tree_free(struct tree * t)
{
	if(t) {
		archive_wstring_free(&t->path);
		archive_wstring_free(&t->full_path);
		SAlloc::F(t->sparse_list);
		SAlloc::F(t->filesystem_table);
		for(int i = 0; i < MAX_OVERLAPPED; i++) {
			if(t->ol[i].buff)
				VirtualFree(t->ol[i].buff, 0, MEM_RELEASE);
			CloseHandle(t->ol[i].ol.hEvent);
		}
		SAlloc::F(t);
	}
}
// 
// Populate the archive_entry with metadata from the disk.
// 
int archive_read_disk_entry_from_file(Archive * _a, ArchiveEntry * entry, int fd, const struct _stat * st)
{
	struct archive_read_disk * a = (struct archive_read_disk *)_a;
	const wchar_t * path;
	const wchar_t * wname;
	const char * name;
	HANDLE h;
	BY_HANDLE_FILE_INFORMATION bhfi;
	DWORD fileAttributes = 0;
	int r;
	archive_clear_error(_a);
	wname = archive_entry_sourcepath_w(entry);
	SETIFZQ(wname, archive_entry_pathname_w(entry));
	if(wname == NULL) {
		archive_set_error(&a->archive, EINVAL, "Can't get a wide character version of the path");
		return ARCHIVE_FAILED;
	}
	path = __la_win_permissive_name_w(wname);
	if(!st) {
		/*
		 * Get metadata through GetFileInformationByHandle().
		 */
		if(fd >= 0) {
			h = (HANDLE)_get_osfhandle(fd);
			r = GetFileInformationByHandle(h, &bhfi);
			if(!r) {
				la_dosmaperr(GetLastError());
				archive_set_error(&a->archive, errno, "Can't GetFileInformationByHandle");
				return ARCHIVE_FAILED;
			}
			entry_copy_bhfi(entry, path, NULL, &bhfi);
		}
		else {
			WIN32_FIND_DATAW findData;
			DWORD flag, desiredAccess;
			h = FindFirstFileW(path, &findData);
			if(h == INVALID_HANDLE_VALUE) {
				la_dosmaperr(GetLastError());
				archive_set_error(&a->archive, errno, "Can't FindFirstFileW");
				return ARCHIVE_FAILED;
			}
			FindClose(h);
			flag = FILE_FLAG_BACKUP_SEMANTICS;
			if(!a->follow_symlinks && (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && (findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK)) {
				flag |= FILE_FLAG_OPEN_REPARSE_POINT;
				desiredAccess = 0;
			}
			else if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				desiredAccess = 0;
			}
			else
				desiredAccess = GENERIC_READ;
			h = CreateFileW(path, desiredAccess, FILE_SHARE_READ, NULL, OPEN_EXISTING, flag, NULL);
			if(h == INVALID_HANDLE_VALUE) {
				la_dosmaperr(GetLastError());
				archive_set_error(&a->archive, errno, "Can't CreateFileW");
				return ARCHIVE_FAILED;
			}
			r = GetFileInformationByHandle(h, &bhfi);
			if(!r) {
				la_dosmaperr(GetLastError());
				archive_set_error(&a->archive, errno, "Can't GetFileInformationByHandle");
				CloseHandle(h);
				return ARCHIVE_FAILED;
			}
			entry_copy_bhfi(entry, path, &findData, &bhfi);
		}
		fileAttributes = bhfi.dwFileAttributes;
	}
	else {
		archive_entry_copy_stat(entry, st);
		if(st->st_mode & S_IFLNK)
			entry_symlink_from_pathw(entry, path);
		h = INVALID_HANDLE_VALUE;
	}
	/* Lookup uname/gname */
	name = archive_read_disk_uname(_a, archive_entry_uid(entry));
	if(name)
		archive_entry_copy_uname(entry, name);
	name = archive_read_disk_gname(_a, archive_entry_gid(entry));
	if(name)
		archive_entry_copy_gname(entry, name);
	/*
	 * File attributes
	 */
	if((a->flags & ARCHIVE_READDISK_NO_FFLAGS) == 0) {
		const int supported_attrs = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
		DWORD file_attrs = fileAttributes & supported_attrs;
		if(file_attrs != 0)
			archive_entry_set_fflags(entry, file_attrs, 0);
	}
	/*
	 * Can this file be sparse file ?
	 */
	if(archive_entry_filetype(entry) != AE_IFREG || archive_entry_size(entry) <= 0 || archive_entry_hardlink(entry)) {
		if(h != INVALID_HANDLE_VALUE && fd < 0)
			CloseHandle(h);
		return ARCHIVE_OK;
	}
	if(h == INVALID_HANDLE_VALUE) {
		if(fd >= 0) {
			h = (HANDLE)_get_osfhandle(fd);
		}
		else {
			h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			if(h == INVALID_HANDLE_VALUE) {
				la_dosmaperr(GetLastError());
				archive_set_error(&a->archive, errno, "Can't CreateFileW");
				return ARCHIVE_FAILED;
			}
		}
		r = GetFileInformationByHandle(h, &bhfi);
		if(!r) {
			la_dosmaperr(GetLastError());
			archive_set_error(&a->archive, errno, "Can't GetFileInformationByHandle");
			if(h != INVALID_HANDLE_VALUE && fd < 0)
				CloseHandle(h);
			return ARCHIVE_FAILED;
		}
		fileAttributes = bhfi.dwFileAttributes;
	}
	/* Sparse file must be set a mark, FILE_ATTRIBUTE_SPARSE_FILE */
	if((fileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) == 0) {
		if(fd < 0)
			CloseHandle(h);
		return ARCHIVE_OK;
	}
	r = setup_sparse_from_disk(a, entry, h);
	if(fd < 0)
		CloseHandle(h);
	return r;
}

/*
 * Windows sparse interface.
 */
#if defined(__MINGW32__) && !defined(FSCTL_QUERY_ALLOCATED_RANGES)
#define FSCTL_QUERY_ALLOCATED_RANGES 0x940CF
typedef struct {
	LARGE_INTEGER FileOffset;
	LARGE_INTEGER Length;
} FILE_ALLOCATED_RANGE_BUFFER;
#endif

static int setup_sparse_from_disk(struct archive_read_disk * a, ArchiveEntry * entry, HANDLE handle)
{
	FILE_ALLOCATED_RANGE_BUFFER range, * outranges = NULL;
	size_t outranges_size;
	int64 entry_size = archive_entry_size(entry);
	int exit_sts = ARCHIVE_OK;
	range.FileOffset.QuadPart = 0;
	range.Length.QuadPart = entry_size;
	outranges_size = 2048;
	outranges = (FILE_ALLOCATED_RANGE_BUFFER*)SAlloc::M(outranges_size);
	if(outranges == NULL) {
		archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
		exit_sts = ARCHIVE_FATAL;
		goto exit_setup_sparse;
	}
	for(;;) {
		DWORD retbytes;
		BOOL ret;
		for(;;) {
			ret = DeviceIoControl(handle, FSCTL_QUERY_ALLOCATED_RANGES, &range, sizeof(range), outranges, (DWORD)outranges_size, &retbytes, NULL);
			if(ret == 0 && GetLastError() == ERROR_MORE_DATA) {
				SAlloc::F(outranges);
				outranges_size *= 2;
				outranges = (FILE_ALLOCATED_RANGE_BUFFER*)SAlloc::M(outranges_size);
				if(outranges == NULL) {
					archive_set_error(&a->archive, ENOMEM, SlTxtOutOfMem);
					exit_sts = ARCHIVE_FATAL;
					goto exit_setup_sparse;
				}
				continue;
			}
			else
				break;
		}
		if(ret) {
			if(retbytes > 0) {
				DWORD i;
				DWORD n = retbytes / sizeof(outranges[0]);
				if(n == 1 && outranges[0].FileOffset.QuadPart == 0 && outranges[0].Length.QuadPart == entry_size)
					break; /* This is not sparse. */
				for(i = 0; i < n; i++)
					archive_entry_sparse_add_entry(entry, outranges[i].FileOffset.QuadPart, outranges[i].Length.QuadPart);
				range.FileOffset.QuadPart = outranges[n-1].FileOffset.QuadPart + outranges[n-1].Length.QuadPart;
				range.Length.QuadPart = entry_size - range.FileOffset.QuadPart;
				if(range.Length.QuadPart > 0)
					continue;
			}
			else {
				/* The entire file is a hole. Add one data block of size 0 at the end. */
				archive_entry_sparse_add_entry(entry, entry_size, 0);
			}
			break;
		}
		else {
			la_dosmaperr(GetLastError());
			archive_set_error(&a->archive, errno, "DeviceIoControl Failed: %lu", GetLastError());
			exit_sts = ARCHIVE_FAILED;
			goto exit_setup_sparse;
		}
	}
exit_setup_sparse:
	SAlloc::F(outranges);
	return (exit_sts);
}

#endif
