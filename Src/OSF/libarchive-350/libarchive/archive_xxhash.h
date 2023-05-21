/*-
 * Copyright (c) 2014 Michihiro NAKAJIMA All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef ARCHIVE_XXHASH_H_INCLUDED
#define ARCHIVE_XXHASH_H_INCLUDED

#ifndef __LIBARCHIVE_BUILD
	#error This header is only to be used internally to libarchive.
#endif

/*typedef enum { 
	XXH_OK = 0, 
	XXH_ERROR 
} XXH_errorcode;*/

struct archive_xxhash {
	uint32 (* XXH32)(const void * input, size_t len, uint seed);
	void * (* XXH32_init)(uint seed);
	XXH_errorcode (* XXH32_update)(XXH32_state_t * state, const void * input, size_t len);
	uint32 (* XXH32_digest)(const XXH32_state_t * state);
};

extern const struct archive_xxhash __archive_xxhash;

#endif
