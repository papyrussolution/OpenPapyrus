/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2017 Oracle and/or its affiliates.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef _DB_SWAP_H_
#define	_DB_SWAP_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Little endian <==> big endian 64-bit swap macros.
 *	M_64_SWAP	swap a memory location
 *	P_64_COPY	copy potentially unaligned 4 byte quantities
 *	P_64_SWAP	swap a referenced memory location
 */
#undef	M_64_SWAP
#define	M_64_SWAP(a) {							\
	uint64 _tmp;							\
	_tmp = (uint64)a;						\
	((uint8 *)&(a))[0] = ((uint8 *)&_tmp)[7];			\
	((uint8 *)&(a))[1] = ((uint8 *)&_tmp)[6];			\
	((uint8 *)&(a))[2] = ((uint8 *)&_tmp)[5];			\
	((uint8 *)&(a))[3] = ((uint8 *)&_tmp)[4];			\
	((uint8 *)&(a))[4] = ((uint8 *)&_tmp)[3];			\
	((uint8 *)&(a))[5] = ((uint8 *)&_tmp)[2];			\
	((uint8 *)&(a))[6] = ((uint8 *)&_tmp)[1];			\
	((uint8 *)&(a))[7] = ((uint8 *)&_tmp)[0];			\
}
#undef	P_64_COPYSWAP
#define	P_64_COPYSWAP(a, b) do {					\
	((uint8 *)b)[0] = ((uint8 *)a)[7];			\
	((uint8 *)b)[1] = ((uint8 *)a)[6];			\
	((uint8 *)b)[2] = ((uint8 *)a)[5];			\
	((uint8 *)b)[3] = ((uint8 *)a)[4];			\
	((uint8 *)b)[4] = ((uint8 *)a)[3];			\
	((uint8 *)b)[5] = ((uint8 *)a)[2];			\
	((uint8 *)b)[6] = ((uint8 *)a)[1];			\
	((uint8 *)b)[7] = ((uint8 *)a)[0];			\
} while (0)
#undef	P_64_COPY
#define	P_64_COPY(a, b) {						\
	((uint8 *)b)[0] = ((uint8 *)a)[0];			\
	((uint8 *)b)[1] = ((uint8 *)a)[1];			\
	((uint8 *)b)[2] = ((uint8 *)a)[2];			\
	((uint8 *)b)[3] = ((uint8 *)a)[3];			\
	((uint8 *)b)[4] = ((uint8 *)a)[4];			\
	((uint8 *)b)[5] = ((uint8 *)a)[5];			\
	((uint8 *)b)[6] = ((uint8 *)a)[6];			\
	((uint8 *)b)[7] = ((uint8 *)a)[7];			\
}
#undef	P_64_SWAP
#define	P_64_SWAP(a) {							\
	uint64 _tmp;							\
	P_64_COPY(a, &_tmp);						\
	((uint8 *)a)[0] = ((uint8 *)&_tmp)[7];			\
	((uint8 *)a)[1] = ((uint8 *)&_tmp)[6];			\
	((uint8 *)a)[2] = ((uint8 *)&_tmp)[5];			\
	((uint8 *)a)[3] = ((uint8 *)&_tmp)[4];			\
	((uint8 *)a)[4] = ((uint8 *)&_tmp)[3];			\
	((uint8 *)a)[5] = ((uint8 *)&_tmp)[2];			\
	((uint8 *)a)[6] = ((uint8 *)&_tmp)[1];			\
	((uint8 *)a)[7] = ((uint8 *)&_tmp)[0];			\
}

/*
 * Little endian <==> big endian 32-bit swap macros.
 *	P_32_COPY	copy potentially unaligned 4 byte quantities
 *	P_32_COPYSWAP	copy and swap potentially unaligned 4 byte quantities
 *	P_32_SWAP	swap a referenced memory location
 *	M_32_SWAP	swap a memory location
 */
#undef	P_32_COPY
#define	P_32_COPY(a, b) do {						\
	((uint8 *)b)[0] = ((uint8 *)a)[0];			\
	((uint8 *)b)[1] = ((uint8 *)a)[1];			\
	((uint8 *)b)[2] = ((uint8 *)a)[2];			\
	((uint8 *)b)[3] = ((uint8 *)a)[3];			\
} while (0)
#undef	P_32_COPYSWAP
#define	P_32_COPYSWAP(a, b) do {					\
	((uint8 *)b)[0] = ((uint8 *)a)[3];			\
	((uint8 *)b)[1] = ((uint8 *)a)[2];			\
	((uint8 *)b)[2] = ((uint8 *)a)[1];			\
	((uint8 *)b)[3] = ((uint8 *)a)[0];			\
} while (0)
#undef	P_32_SWAP
#define	P_32_SWAP(a) do {						\
	uint32 _tmp;							\
	P_32_COPY(a, &_tmp);						\
	P_32_COPYSWAP(&_tmp, a);					\
} while (0)
#undef	M_32_SWAP
#define	M_32_SWAP(a) P_32_SWAP(&(a))

/*
 * Little endian <==> big endian 16-bit swap macros.
 *	P_16_COPY	copy potentially unaligned 2 byte quantities
 *	P_16_COPYSWAP	copy and swap potentially unaligned 2 byte quantities
 *	P_16_SWAP	swap a referenced memory location
 *	M_16_SWAP	swap a memory location
 */
#undef	P_16_COPY
#define	P_16_COPY(a, b) do {						\
	((uint8 *)b)[0] = ((uint8 *)a)[0];			\
	((uint8 *)b)[1] = ((uint8 *)a)[1];			\
} while (0)
#undef	P_16_COPYSWAP
#define	P_16_COPYSWAP(a, b) do {					\
	((uint8 *)b)[0] = ((uint8 *)a)[1];			\
	((uint8 *)b)[1] = ((uint8 *)a)[0];			\
} while (0)
#undef	P_16_SWAP
#define	P_16_SWAP(a) do {						\
	uint16 _tmp;							\
	P_16_COPY(a, &_tmp);						\
	P_16_COPYSWAP(&_tmp, a);					\
} while (0)
#undef	M_16_SWAP
#define	M_16_SWAP(a) P_16_SWAP(&(a))

#undef	SWAP64
#define	SWAP64(p) {							\
	P_64_SWAP(p);							\
	(p) += sizeof(uint64);					\
}
#undef	SWAP32
#define	SWAP32(p) {							\
	P_32_SWAP(p);							\
	(p) += sizeof(uint32);					\
}
#undef	SWAP16
#define	SWAP16(p) {							\
	P_16_SWAP(p);							\
	(p) += sizeof(uint16);					\
}

/*
 * Berkeley DB has local versions of htonl() and ntohl() that operate on
 * pointers to the right size memory locations; the portability magic for
 * finding the real system functions isn't worth the effort.
 */
#undef	DB_HTONL_SWAP
#define	DB_HTONL_SWAP(env, p) do {					\
	if (F_ISSET((env), ENV_LITTLEENDIAN))				\
		P_32_SWAP(p);						\
} while (0)
#undef	DB_NTOHL_SWAP
#define	DB_NTOHL_SWAP(env, p) do {					\
	if (F_ISSET((env), ENV_LITTLEENDIAN))				\
		P_32_SWAP(p);						\
} while (0)

#undef	DB_NTOHLL_COPYIN
#define	DB_NTOHLL_COPYIN(env, i, p) do {				\
	uint8 *tmp;							\
	tmp = (uint8 *)&(i);						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		tmp[7] = *p++;						\
		tmp[6] = *p++;						\
		tmp[5] = *p++;						\
		tmp[4] = *p++;						\
		tmp[3] = *p++;						\
		tmp[2] = *p++;						\
		tmp[1] = *p++;						\
		tmp[0] = *p++;						\
	} else {							\
		memcpy(&(i), p, sizeof(uint64));			\
		p = (uint8 *)p + sizeof(uint64);			\
	}								\
} while (0)

#undef	DB_NTOHL_COPYIN
#define	DB_NTOHL_COPYIN(env, i, p) do {					\
	uint8 *tmp;							\
	tmp = (uint8 *)&(i);						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		tmp[3] = *p++;						\
		tmp[2] = *p++;						\
		tmp[1] = *p++;						\
		tmp[0] = *p++;						\
	} else {							\
		memcpy(&(i), p, sizeof(uint32));			\
		p = (uint8 *)p + sizeof(uint32);			\
	}								\
} while (0)

#undef	DB_NTOHS_COPYIN
#define	DB_NTOHS_COPYIN(env, i, p) do {					\
	uint8 *tmp;							\
	tmp = (uint8 *)&(i);						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		tmp[1] = *p++;						\
		tmp[0] = *p++;						\
	} else {							\
		memcpy(&(i), p, sizeof(uint16));			\
		p = (uint8 *)p + sizeof(uint16);			\
	}								\
} while (0)

#undef	DB_HTONLL_COPYOUT
#define	DB_HTONLL_COPYOUT(env, p, i) do {				\
	uint8 *tmp;							\
	tmp = (uint8 *)p;						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		*tmp++ = ((uint8 *)&(i))[7];				\
		*tmp++ = ((uint8 *)&(i))[6];				\
		*tmp++ = ((uint8 *)&(i))[5];				\
		*tmp++ = ((uint8 *)&(i))[4];				\
		*tmp++ = ((uint8 *)&(i))[3];				\
		*tmp++ = ((uint8 *)&(i))[2];				\
		*tmp++ = ((uint8 *)&(i))[1];				\
		*tmp++ = ((uint8 *)&(i))[0];				\
	} else								\
		memcpy(p, &(i), sizeof(uint64));			\
	p = (uint8 *)p + sizeof(uint64);				\
} while (0)

#undef	DB_HTONL_COPYOUT
#define	DB_HTONL_COPYOUT(env, p, i) do {				\
	uint8 *tmp;							\
	tmp = (uint8 *)p;						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		*tmp++ = ((uint8 *)&(i))[3];				\
		*tmp++ = ((uint8 *)&(i))[2];				\
		*tmp++ = ((uint8 *)&(i))[1];				\
		*tmp++ = ((uint8 *)&(i))[0];				\
	} else								\
		memcpy(p, &(i), sizeof(uint32));			\
	p = (uint8 *)p + sizeof(uint32);				\
} while (0)

#undef	DB_HTONS_COPYOUT
#define	DB_HTONS_COPYOUT(env, p, i) do {				\
	uint8 *tmp;							\
	tmp = (uint8 *)p;						\
	if (F_ISSET(env, ENV_LITTLEENDIAN)) {				\
		*tmp++ = ((uint8 *)&(i))[1];				\
		*tmp++ = ((uint8 *)&(i))[0];				\
	} else								\
		memcpy(p, &i, sizeof(uint16));			\
	p = (uint8 *)p + sizeof(uint16);				\
} while (0)

/*
 * Helper macros for swapped logs.  We write logs in little endian format to
 * minimize disruption on x86 when upgrading from native byte order to
 * platform-independent logs.
 */
#define	LOG_SWAPPED(env) !F_ISSET(env, ENV_LITTLEENDIAN)

#define	LOGCOPY_64(env, x, p) do {					\
	if (LOG_SWAPPED(env))						\
		P_64_COPYSWAP((p), (x));				\
	else								\
		memcpy((x), (p), sizeof(uint64));			\
} while (0)

#define	LOGCOPY_32(env, x, p) do {					\
	if (LOG_SWAPPED(env))						\
		P_32_COPYSWAP((p), (x));				\
	else								\
		memcpy((x), (p), sizeof(uint32));			\
} while (0)

#define	LOGCOPY_16(env, x, p) do {					\
	if (LOG_SWAPPED(env))						\
		P_16_COPYSWAP((p), (x));				\
	else								\
		memcpy((x), (p), sizeof(uint16));			\
} while (0)

#define	LOGCOPY_TOLSN(env, lsnp, p) do {				\
	LOGCOPY_32((env), &(lsnp)->file, (p));				\
	LOGCOPY_32((env), &(lsnp)->offset,				\
	    (uint8 *)(p) + sizeof(uint32));			\
} while (0)

#define	LOGCOPY_FROMLSN(env, p, lsnp) do {				\
	LOGCOPY_32((env), (p), &(lsnp)->file);				\
	LOGCOPY_32((env),						\
	    (uint8 *)(p) + sizeof(uint32), &(lsnp)->offset);	\
} while (0)

#if defined(__cplusplus)
}
#endif

#endif /* !_DB_SWAP_H_ */
