/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#ifndef _NGX_FILES_H_INCLUDED_
#define _NGX_FILES_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

typedef HANDLE ngx_fd_t;
typedef BY_HANDLE_FILE_INFORMATION ngx_file_info_t;
typedef uint64_t ngx_file_uniq_t;

typedef struct {
	uchar * name;
	size_t size;
	void * addr;
	ngx_fd_t fd;
	HANDLE handle;
	ngx_log_t * log;
} ngx_file_mapping_t;

typedef struct {
	HANDLE dir;
	WIN32_FIND_DATA finddata;
	unsigned valid_info : 1;
	unsigned type : 1;
	unsigned ready : 1;
} ngx_dir_t;

typedef struct {
	HANDLE dir;
	WIN32_FIND_DATA finddata;
	unsigned ready : 1;
	unsigned test : 1;
	unsigned no_match : 1;
	uchar * pattern;
	ngx_str_t name;
	size_t last;
	ngx_log_t * log;
} ngx_glob_t;

/* INVALID_FILE_ATTRIBUTES is specified but not defined at least in MSVC6SP2 */
#ifndef INVALID_FILE_ATTRIBUTES
	#define INVALID_FILE_ATTRIBUTES     0xffffffff
#endif
/* INVALID_SET_FILE_POINTER is not defined at least in MSVC6SP2 */
#ifndef INVALID_SET_FILE_POINTER
	#define INVALID_SET_FILE_POINTER    0xffffffff
#endif
#define NGX_INVALID_FILE            INVALID_HANDLE_VALUE
#define NGX_FILE_ERROR              0

ngx_fd_t ngx_open_file(uchar * name, ulong mode, ulong create, ulong access);

#define ngx_open_file_n             "CreateFile()"
#define NGX_FILE_RDONLY             GENERIC_READ
#define NGX_FILE_WRONLY             GENERIC_WRITE
#define NGX_FILE_RDWR               GENERIC_READ|GENERIC_WRITE
#define NGX_FILE_APPEND             FILE_APPEND_DATA|SYNCHRONIZE
#define NGX_FILE_NONBLOCK           0
#define NGX_FILE_CREATE_OR_OPEN     OPEN_ALWAYS
#define NGX_FILE_OPEN               OPEN_EXISTING
#define NGX_FILE_TRUNCATE           CREATE_ALWAYS
#define NGX_FILE_DEFAULT_ACCESS     0
#define NGX_FILE_OWNER_ACCESS       0

#define ngx_open_tempfile(name, persistent, access)			     \
	CreateFile(SUcSwitch(reinterpret_cast<const char *>(name)), GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, \
	    NULL, CREATE_NEW, persistent ? 0 : FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);

#define ngx_open_tempfile_n         "CreateFile()"
#define ngx_close_file              CloseHandle
#define ngx_close_file_n            "CloseHandle()"

ssize_t ngx_read_fd(ngx_fd_t fd, void * buf, size_t size);
#define ngx_read_fd_n               "ReadFile()"

ssize_t ngx_write_fd(ngx_fd_t fd, const void * buf, size_t size);
#define ngx_write_fd_n              "WriteFile()"

ssize_t ngx_write_console(ngx_fd_t fd, const void * buf, size_t size);

#define ngx_linefeed(p)             *p++ = __CR; *p++ = LF;
#define NGX_LINEFEED_SIZE           2
#define NGX_LINEFEED                CRLF

#define ngx_delete_file(name)       DeleteFile(SUcSwitch(reinterpret_cast<const char *>(name)))
#define ngx_delete_file_n           "DeleteFile()"

#define ngx_rename_file(o, n)       MoveFile(SUcSwitch(reinterpret_cast<const char *>(o)), SUcSwitch(reinterpret_cast<const char *>(n)))
#define ngx_rename_file_n           "MoveFile()"
ngx_err_t ngx_win32_rename_file(ngx_str_t * from, ngx_str_t * to, ngx_log_t * log);

ngx_int_t ngx_set_file_time(uchar * name, ngx_fd_t fd, time_t s);
#define ngx_set_file_time_n         "SetFileTime()"

ngx_int_t ngx_file_info(uchar * filename, ngx_file_info_t * fi);
#define ngx_file_info_n             "GetFileAttributesEx()"

#define ngx_fd_info(fd, fi)         GetFileInformationByHandle(fd, fi)
#define ngx_fd_info_n               "GetFileInformationByHandle()"

#define ngx_link_info(name, fi)     ngx_file_info(name, fi)
#define ngx_link_info_n             "GetFileAttributesEx()"

#define ngx_is_dir(fi)      (((fi)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define ngx_is_file(fi)     (((fi)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
#define ngx_is_link(fi)     0
#define ngx_is_exec(fi)     0
#define ngx_file_access(fi) 0
#define ngx_file_size(fi)           (((nginx_off_t)(fi)->nFileSizeHigh << 32) | (fi)->nFileSizeLow)
#define ngx_file_fs_size(fi)        ngx_file_size(fi)
#define ngx_file_uniq(fi)   (*(ngx_file_uniq_t*)&(fi)->nFileIndexHigh)

/* 116444736000000000 is commented in src/os/win32/ngx_time.c */

#define ngx_file_mtime(fi) (time_t)(((((unsigned __int64)(fi)->ftLastWriteTime.dwHighDateTime << 32) | \
	(fi)->ftLastWriteTime.dwLowDateTime) - SlConst::Epoch1600_1970_Offs_100Ns) / 10000000)

ngx_int_t ngx_create_file_mapping(ngx_file_mapping_t * fm);
void ngx_close_file_mapping(ngx_file_mapping_t * fm);

uchar * ngx_realpath(uchar * path, uchar * resolved);
#define ngx_realpath_n              ""
#define ngx_getcwd(buf, size)       GetCurrentDirectoryA(size, reinterpret_cast<char *>(buf)) // @v10.3.11 GetCurrentDirectory-->GetCurrentDirectoryA
#define ngx_getcwd_n                "GetCurrentDirectory()"
#define ngx_path_separator(c)       ((c) == '/' || (c) == '\\')

#define NGX_HAVE_MAX_PATH           1
#define NGX_MAX_PATH                MAX_PATH

#define NGX_DIR_MASK                (uchar *)"/*"
#define NGX_DIR_MASK_LEN            2

ngx_int_t ngx_open_dir(ngx_str_t * name, ngx_dir_t * dir);
#define ngx_open_dir_n              "FindFirstFile()"

ngx_int_t ngx_read_dir(ngx_dir_t * dir);
#define ngx_read_dir_n              "FindNextFile()"

ngx_int_t ngx_close_dir(ngx_dir_t * dir);
#define ngx_close_dir_n             "FindClose()"

#define ngx_create_dir(name, access) CreateDirectory(SUcSwitch(reinterpret_cast<const char *>(name)), NULL)
#define ngx_create_dir_n            "CreateDirectory()"

#define ngx_delete_dir(name)        RemoveDirectory(SUcSwitch(reinterpret_cast<const char *>(name)))
#define ngx_delete_dir_n            "RemoveDirectory()"

#define ngx_dir_access(a)           (a)

#define ngx_de_name(dir)            ((uchar *)(dir)->finddata.cFileName)
#define ngx_de_namelen(dir)         ngx_strlen((dir)->finddata.cFileName)

ngx_int_t ngx_de_info(uchar * name, ngx_dir_t * dir);
#define ngx_de_info_n               "dummy()"

ngx_int_t ngx_de_link_info(uchar * name, ngx_dir_t * dir);
#define ngx_de_link_info_n          "dummy()"

#define ngx_de_is_dir(dir)						     \
	(((dir)->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define ngx_de_is_file(dir)						     \
	(((dir)->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
#define ngx_de_is_link(dir)         0
#define ngx_de_access(dir)          0
#define ngx_de_size(dir)            (((nginx_off_t)(dir)->finddata.nFileSizeHigh << 32) | (dir)->finddata.nFileSizeLow)
#define ngx_de_fs_size(dir)         ngx_de_size(dir)

/* 116444736000000000 is commented in src/os/win32/ngx_time.c */

#define ngx_de_mtime(dir) (time_t)(((((unsigned __int64)(dir)->finddata.ftLastWriteTime.dwHighDateTime << 32) |\
	(dir)->finddata.ftLastWriteTime.dwLowDateTime) - SlConst::Epoch1600_1970_Offs_100Ns) / 10000000)

ngx_int_t ngx_open_glob(ngx_glob_t * gl);
#define ngx_open_glob_n             "FindFirstFile()"

ngx_int_t ngx_read_glob(ngx_glob_t * gl, ngx_str_t * name);
void ngx_close_glob(ngx_glob_t * gl);

ssize_t ngx_read_file(ngx_file_t * file, uchar * buf, size_t size, nginx_off_t offset);
#define ngx_read_file_n             "ReadFile()"

ssize_t ngx_write_file(ngx_file_t * file, uchar * buf, size_t size, nginx_off_t offset);
ssize_t ngx_write_chain_to_file(ngx_file_t * file, ngx_chain_t * ce, nginx_off_t offset, ngx_pool_t * pool);

ngx_int_t ngx_read_ahead(ngx_fd_t fd, size_t n);
#define ngx_read_ahead_n            "ngx_read_ahead_n"

ngx_int_t ngx_directio_on(ngx_fd_t fd);
#define ngx_directio_on_n           "ngx_directio_on_n"

ngx_int_t ngx_directio_off(ngx_fd_t fd);
#define ngx_directio_off_n          "ngx_directio_off_n"

size_t ngx_fs_bsize(uchar * name);

#define ngx_stdout               GetStdHandle(STD_OUTPUT_HANDLE)
#define ngx_stderr               GetStdHandle(STD_ERROR_HANDLE)
#define ngx_set_stderr(fd)       SetStdHandle(STD_ERROR_HANDLE, fd)
#define ngx_set_stderr_n         "SetStdHandle(STD_ERROR_HANDLE)"

#endif /* _NGX_FILES_H_INCLUDED_ */
