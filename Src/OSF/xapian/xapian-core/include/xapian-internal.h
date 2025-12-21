// XAPIAN-INTERNAL.H
//
#ifndef __XAPIAN_INTERNAL_H
#define __XAPIAN_INTERNAL_H

#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#include <config.h>
#include <dirent.h> // @sobolev SLIB implementation
#include <xapian.h>
#include "str.h"
#include "omassert.h"
#include "debuglog.h"
#include "heap.h"
#include "stringutils.h"
#include "pack.h"
#include "overflow.h"
// @sobolev (inlined below) #include "log2.h"
//#include "internaltypes.h"
// The standard marks these types as optional, as an implementation may not
// directly support a type of the appropriate width.  If there are platforms
// we care about which lack them, we could use wider types with some care
// around where we read and write them.
typedef uint32_t uint4;
//
#include "serialise-double.h"
// @v11.9.4 #include "stdclamp.h"
#include "min_non_zero.h"
#include "replicationprotocol.h"
#include "errno_to_string.h"
#include "safesysstat.h"
//#include "filetests.h"
/** Test if a file exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path is a regular file, or a symbolic link which
 *	    resolves to a regular file.
 */
inline bool file_exists(const char * path) 
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

/** Test if a file exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path is a regular file, or a symbolic link which
 *	    resolves to a regular file.
 */
inline bool file_exists(const std::string & path) { return file_exists(path.c_str()); }

/** Returns the size of a file.
 *
 *  @param path	The path to test
 *
 *  errno is set to 0 (upon success), or the error returned by stat(), or
 *  EINVAL (if the path isn't a regular file or a symlink resolving to a
 *  regular file).
 *
 *  If the file's size is larger than the maximum value off_t can represent,
 *  then stat() will fail with errno=EOVERFLOW, and so will this function.
 *  There doesn't seem to be a way to determine the file size in this case,
 *  short of reading it all.  This is only likely if the LFS check in configure
 *  doesn't work out how to enable largefile support.
 *
 *  @return The size of the file, or 0 if it doesn't exist or isn't a file.
 */
inline off_t file_size(const char * path) 
{
	struct stat st;
	if(stat(path, &st) == 0) {
		if(S_ISREG(st.st_mode)) {
			errno = 0;
			return st.st_size;
		}
		errno = EINVAL;
	}
	return 0;
}

/** Returns the size of a file.
 *
 *  @param path	The path to test
 *
 *  Note: If the file's size is larger than the maximum value off_t can
 *  represent, then stat() will fail with EOVERFLOW, and so will this
 *  function.  There doesn't seem to be a way to determine the file size
 *  in this case, short of reading it all.  This is only likely if the LFS
 *  check in configure doesn't work out how to enable largefile support.
 *
 *  @return The size of the file, or 0 if it doesn't exist or isn't a file;
 *	    errno is set to 0 (upon success), or the error returned by
 *	    stat(), or EINVAL (if the path isn't a regular file or a symlink
 *	    resolving to a regular file).
 */
inline off_t file_size(const std::string & path) { return file_size(path.c_str()); }

/** Returns the size of a file.
 *
 *  @param fd	The file descriptor for the file.
 *
 *  Note: If the file's size is larger than the maximum value off_t can
 *  represent, then stat() will fail with EOVERFLOW, and so will this
 *  function.  There doesn't seem to be a way to determine the file size
 *  in this case, short of reading it all.  This is only likely if the LFS
 *  check in configure doesn't work out how to enable largefile support.
 *
 *  @return The size of the file, or 0 if it doesn't exist or isn't a file;
 *	    errno is set to 0 (upon success), or the error returned by
 *	    stat(), or EINVAL (if the path isn't a regular file or a symlink
 *	    resolving to a regular file).
 */
inline off_t file_size(int fd) 
{
	struct stat st;
	if(fstat(fd, &st) == 0) {
		if(S_ISREG(st.st_mode)) {
			errno = 0;
			return st.st_size;
		}
		errno = EINVAL;
	}
	return 0;
}

/** Test if a directory exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path is a directory, or a symbolic link which resolves
 *	    to a directory.
 */
inline bool dir_exists(const char * path) 
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/** Test if a directory exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path is a directory, or a symbolic link which resolves
 *	    to a directory.
 */
inline bool dir_exists(const std::string & path) { return dir_exists(path.c_str()); }

/** Test if a path exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path exists (and is not a dangling symlink).
 */
inline bool path_exists(const char * path) 
{
	struct stat st;
	return stat(path, &st) == 0;
}

/** Test if a path exists.
 *
 *  @param path	The path to test
 *
 *  @return true if @a path exists (and is not a dangling symlink).
 */
inline bool path_exists(const std::string & path) { return path_exists(path.c_str()); }
//
//#include "fileutils.h"
/** Remove a directory, and its contents.
 *
 *  If dirname doesn't refer to a file or directory, no error is generated.
 *
 *  Note - this doesn't currently cope with directories which contain
 *  subdirectories.
 */
void removedir(const std::string &dirname);

/** Resolve @a path relative to @a base.
 *
 *  Return @a path qualified to work as if you did "chdir(<directory which base
 *  is in>)" first.
 */
void resolve_relative_path(std::string & path, const std::string & base);
//
#include "safefcntl.h"
#include "safeunistd.h"
#include "safenetdb.h"
#include "io_utils.h"
#include "realtime.h"
#include "parseint.h"
//#include "fd.h"
class FD {
	int fd;
	/// Prevent copying.
	FD(const FD &) = delete;
	/// Prevent assignment between FD objects.
	FD& operator = (const FD&) = delete;
public:
	FD() : fd(-1) 
	{
	}
	FD(int fd_) : fd(fd_) 
	{
	}
	~FD() 
	{
		if(fd != -1) 
			::_close(fd);
	}
	FD& operator = (int fd_) 
	{
		if(fd != -1) ::_close(fd);
		fd = fd_;
		return *this;
	}
	operator int() const { return fd; }
	int close() 
	{
		// Don't check for -1 here, so that close(FD) sets errno as close(int)
		// would.
		int fd_to_close = fd;
		fd = -1;
		return ::_close(fd_to_close);
	}
};

inline int close(FD& fd) { return fd.close(); }
//
//#include "alignment_cast.h"
/** Cast a pointer we know is suitably aligned.
 *
 *  Has the same effect as reinterpret_cast<T> but avoids warnings about
 *  alignment issues.
 *
 *  Version for const pointers.
 */
template<typename T, typename U> typename std::enable_if<std::is_const<typename std::remove_pointer<U>::type>::value, T>::type alignment_cast(U ptr)
{
    return static_cast<T>(static_cast<const void*>(ptr));
}

/** Cast a pointer we know is suitably aligned.
 *
 *  Has the same effect as reinterpret_cast<T> but avoids warnings about
 *  alignment issues.
 *
 *  Version for non-const pointers.
 */
template<typename T, typename U> typename std::enable_if<!std::is_const<typename std::remove_pointer<U>::type>::value, T>::type alignment_cast(U ptr)
{
    return static_cast<T>(static_cast<void*>(ptr));
}
// 
//#include "wordaccess.h"
#ifndef PACKAGE
	#error config.h must be included first in each C++ source file
#endif
// @v12.3.1 /* @sobolev (replaced with sbswap32/sbswap16/sbswap64)
inline uint16_t do_bswap(uint16_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP16
	return __builtin_bswap16(value);
#elif HAVE_DECL__BYTESWAP_USHORT
	return _byteswap_ushort(value);
#else
	return (value << 8) | (value >> 8);
#endif
}

inline uint32_t do_bswap(uint32_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP32
	return __builtin_bswap32(value);
#elif HAVE_DECL__BYTESWAP_ULONG
	return _byteswap_ulong(value);
#else
	return (value << 24) | ((value & 0xff00) << 8) | ((value >> 8) & 0xff00) | (value >> 24);
#endif
}

inline uint64_t do_bswap(uint64_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP64
	return __builtin_bswap64(value);
#elif HAVE_DECL__BYTESWAP_UINT64
	return _byteswap_uint64(value);
#else
	return (value << 56) | ((value & 0xff00) << 40) | ((value & 0xff0000) << 24) |
	       ((value & 0xff000000) << 8) | ((value >> 8) & 0xff000000) | ((value >> 24) & 0xff0000) |
	       ((value >> 40) & 0xff00) | (value >> 56);
#endif
}
// @v12.3.1 */

template <typename UINT> inline UINT do_aligned_read(const uchar * ptr)
{
	UINT value = *alignment_cast<const UINT*>(ptr);
#ifndef SL_BIGENDIAN
	value = do_bswap(value);
#endif
	return value;
}

template <typename T, typename UINT> inline void do_aligned_write(uchar * ptr, T value)
{
	if(std::is_signed<T>::value) {
		AssertRel(value, >=, 0);
	}
	if(sizeof(T) > sizeof(UINT)) {
		AssertEq(value, T(UINT(value)));
	}
	UINT v = UINT(value);
#ifndef SL_BIGENDIAN
	v = do_bswap(v);
#endif
	*alignment_cast<UINT*>(ptr) = v;
}

template <typename UINT> inline UINT do_unaligned_read(const uchar * ptr)
{
	UINT value;
	memcpy(&value, ptr, sizeof(UINT));
#ifndef SL_BIGENDIAN
	value = do_bswap(value);
#endif
	return value;
}

template <typename T, typename UINT> inline void do_unaligned_write(uchar * ptr, T value)
{
	if(std::is_signed<T>::value) {
		AssertRel(value, >=, 0);
	}
	if(sizeof(T) > sizeof(UINT)) {
		AssertEq(value, T(UINT(value)));
	}
	UINT v = UINT(value);
#ifndef SL_BIGENDIAN
	v = do_bswap(v);
#endif
	memcpy(ptr, &v, sizeof(UINT));
}

inline uint32_t aligned_read4(const uchar * ptr) { return do_aligned_read<uint32_t>(ptr); }
inline uint32_t unaligned_read4(const uchar * ptr) { return do_unaligned_read<uint32_t>(ptr); }
inline uint16_t aligned_read2(const uchar * ptr) { return do_aligned_read<uint16_t>(ptr); }
inline uint16_t unaligned_read2(const uchar * ptr) { return do_unaligned_read<uint16_t>(ptr); }
template <typename T> inline void aligned_write4(uchar * ptr, T value) { do_aligned_write<T, uint32_t>(ptr, value); }
template <typename T> inline void unaligned_write4(uchar * ptr, T value) { do_unaligned_write<T, uint32_t>(ptr, value); }
template <typename T> inline void aligned_write2(uchar * ptr, T value) { do_aligned_write<T, uint16_t>(ptr, value); }
template <typename T> inline void unaligned_write2(uchar * ptr, T value) { do_unaligned_write<T, uint16_t>(ptr, value); }
//
#include "posixy_wrapper.h"
#include "serialise.h"
#include "serialise-error.h"
#include "backends/databaseinternal.h"
#include "backends/uuids.h"
#include "weightinternal.h"
#include "smallvector.h"
#include "terminfo.h"
#include "backends/positionlist.h"
#include "termlist.h"
#include "enquireinternal.h"
#include "result.h"
#include "msetinternal.h"
#include "editdistance.h"
#include "queryvector.h"
#include "queryinternal.h"
#include "backends/multi.h"
#include "rsetinternal.h"
#include "replication.h"
#include "vectortermlist.h"
#include "backends/postlist.h"
#include "postingiteratorinternal.h"
#include "backends/documentinternal.h"
#include "documenttermlist.h"
#include "backends/valuelist.h"
#include "documentvaluelist.h"
#include "remoteprotocol.h"
#include "backends/multi/multi_termlist.h"
#include "backends/backends.h"
#include "backends/databasereplicator.h"
#include "backends/empty_database.h"
#include "backends/multi/multi_database.h"
#include "backends/alltermslist.h"
#include "backends/multi/multi_alltermslist.h"
#include "backends/slowvaluelist.h"
#include "backends/leafpostlist.h"
#include "backends/remote/remote-database.h"
#include "unicode/description_append.h"
#include "valuestreamdocument.h"
#include "wrapperpostlist.h"
#include "selectpostlist.h"
#include "deciderpostlist.h"
#include "synonympostlist.h"
#include "postlisttree.h"
#include "andmaybepostlist.h"
#include "andnotpostlist.h"
#include "boolorpostlist.h"
#include "exactphrasepostlist.h"
#include "externalpostlist.h"
#include "maxpostlist.h"
#include "multiandpostlist.h"
#include "multixorpostlist.h"
#include "nearpostlist.h"
#include "orpositionlist.h"
#include "orpospostlist.h"
#include "orpostlist.h"
#include "phrasepostlist.h"
#include "localsubmatch.h"
#include "queryoptimiser.h"
#include "valuerangepostlist.h"
#include "valuegepostlist.h"
#include "extraweightpostlist.h"
#include "remotesubmatch.h"
#include "matcher.h"
#include "msetcmp.h"
#include "collapser.h"
#include "matchtimeout.h"
#include "spymaster.h"
#include "protomset.h"
#include "backends/databasehelpers.h"
#include "backends/valuestats.h"
#include "backends/flint_lock.h"
#include "backends/byte_length_strings.h"
#include "backends/prefix_compressed_strings.h"
#include "backends/inmemory/inmemory_positionlist.h"
#include "backends/contiguousalldocspostlist.h"
#include "remoteconnection.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include "replicatetcpclient.h"
#include "progclient.h"
#include "remotetcpclient.h"
#include "remoteserver.h"
#include "expandweight.h"
#include "esetinternal.h"
#include "popcount.h"
#include "ortermlist.h"
#include "termlistmerger.h"
#include "closefrom.h"
#include "bitstream.h"
#include <xapian/types.h>
#include <xapian/compactor.h>
#include <xapian/constants.h>
#include "pack.h"
#include "keyword.h"
#include "backends\glass\glass_defs.h"
#include "backends\glass\glass_freelist.h"
#include "backends\glass\glass_cursor.h"
#include "backends\glass\glass_lazytable.h"
#include "backends\glass\glass_positionlist.h"
#include "backends\glass\glass_dbcheck.h"
#include "compression_stream.h"
#include "backends\glass\glass_table.h"
#include "backends\glass\glass_check.h"
#include "backends\glass\glass_changes.h"
#include "backends\glass\glass_version.h"
#include "backends\glass\glass_inverter.h"
#include "backends\glass\glass_docdata.h"
#include "backends\glass\glass_postlist.h"
#include "backends\glass\glass_spelling.h"
#include "backends\glass\glass_synonym.h"
#include "backends\glass\glass_termlisttable.h"
#include "backends\glass\glass_values.h"
#include "backends\glass\glass_database.h"
#include "backends\glass\glass_alldocspostlist.h"
#include "backends\glass\glass_alltermslist.h"
#include "backends\glass\glass_document.h"
#include "backends\glass\glass_metadata.h"
#include "backends\glass\glass_replicate_internal.h"
#include "backends\glass\glass_spellingwordslist.h"
#include "backends\glass\glass_termlist.h"
#include "backends\glass\glass_valuelist.h"
#include "backends\glass\glass_databasereplicator.h"
//exp10.h
#if !HAVE_DECL_EXP10
	#if HAVE_DECL___EXP10
		inline double exp10(double x) { return __exp10(x); }
	#elif defined HAVE___BUILTIN_EXP10
		inline double exp10(double x) { return __builtin_exp10(x); }
	#else
		inline double exp10(double x) { return std::pow(10.0, x); }
	#endif
#endif
//
//log2.h
#if !HAVE_DECL_LOG2
	// @v12.5.1 inline double log2(double x) { return std::log(x) / std::log(2.0); }
#endif
//
#endif // __XAPIAN_INTERNAL_H
