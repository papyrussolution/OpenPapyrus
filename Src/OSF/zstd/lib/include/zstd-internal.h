// ZSTD-INTERNAL.H
//
#ifndef __ZSTD_INTERNAL_H
#define __ZSTD_INTERNAL_H

#include <slib.h>
#if defined(_MSC_VER)
	#include <intrin.h>
#endif
#include <xxhash.h> /* XXH64_* */
//#include <zstd_internal.h> /* includes zstd.h */
#define ZSTD_MULTITHREAD
#define ZSTD_GZCOMPRESS
#define ZSTD_GZDECOMPRESS
#define ZSTD_LZMACOMPRESS
#define ZSTD_LZMADECOMPRESS
#define ZSTD_LZ4COMPRESS
#define ZSTD_LZ4DECOMPRESS

#endif // !__ZSTD_INTERNAL_H

