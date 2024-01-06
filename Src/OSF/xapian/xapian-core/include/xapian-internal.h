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
#include "stdclamp.h"
#include "min_non_zero.h"
#include "replicationprotocol.h"
#include "errno_to_string.h"
#include "safesysstat.h"
#include "filetests.h"
#include "fileutils.h"
#include "safefcntl.h"
#include "safeunistd.h"
#include "safenetdb.h"
#include "io_utils.h"
#include "realtime.h"
#include "parseint.h"
#include "fd.h"
#include "alignment_cast.h"
//#include "wordaccess.h"
#ifndef PACKAGE
	#error config.h must be included first in each C++ source file
#endif
/* @sobolev (replaced with sbswap32/sbswap16/sbswap64)
inline uint16_t do_bswap(uint16_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP16
	return __builtin_bswap16(value);
# elif HAVE_DECL__BYTESWAP_USHORT
	return _byteswap_ushort(value);
#else
	return (value << 8) | (value >> 8);
#endif
}

inline uint32_t do_bswap(uint32_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP32
	return __builtin_bswap32(value);
# elif HAVE_DECL__BYTESWAP_ULONG
	return _byteswap_ulong(value);
#else
	return (value << 24) | ((value & 0xff00) << 8) | ((value >> 8) & 0xff00) | (value >> 24);
#endif
}

inline uint64_t do_bswap(uint64_t value) 
{
#if HAVE_DECL___BUILTIN_BSWAP64
	return __builtin_bswap64(value);
# elif HAVE_DECL__BYTESWAP_UINT64
	return _byteswap_uint64(value);
#else
	return (value << 56) | ((value & 0xff00) << 40) | ((value & 0xff0000) << 24) |
	       ((value & 0xff000000) << 8) | ((value >> 8) & 0xff000000) | ((value >> 24) & 0xff0000) |
	       ((value >> 40) & 0xff00) | (value >> 56);
#endif
}*/

template <typename UINT> inline UINT do_aligned_read(const uchar * ptr)
{
	UINT value = *alignment_cast<const UINT*>(ptr);
#ifndef SL_BIGENDIAN
	//const uint v2 = sbswap32(value);
	value = /*do_bswap*/sbswap32(value);
	//assert(v2 == value);
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
	//const uint v2 = sbswap32(v);
	v = /*do_bswap*/sbswap32(v);
	//assert(v2 == v);
#endif
	*alignment_cast<UINT*>(ptr) = v;
}

template <typename UINT> inline UINT do_unaligned_read(const uchar * ptr)
{
	UINT value;
	memcpy(&value, ptr, sizeof(UINT));
#ifndef SL_BIGENDIAN
	//const uint v2 = sbswap32(value);
	value = /*do_bswap*/sbswap32(value);
	//assert(v2 == value);
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
	//const uint v2 = sbswap32(v);
	v = /*do_bswap*/sbswap32(v);
	//assert(v2 == v);
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
	inline double log2(double x) { return std::log(x) / std::log(2.0); }
#endif
//
#endif // __XAPIAN_INTERNAL_H
