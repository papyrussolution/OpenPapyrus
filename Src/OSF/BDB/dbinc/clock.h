/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005, 2011 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */
/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *  may be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 *	@(#)time.h	8.5 (Berkeley) 5/4/95
 * FreeBSD: src/sys/sys/time.h,v 1.65 2004/04/07 04:19:49 imp Exp
 */
#ifndef _DB_CLOCK_H_
#define	_DB_CLOCK_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This declaration is POSIX-compatible.  Because there are lots of different
 * time.h include file patterns out there, it's easier to declare our own name
 * in all cases than to try and discover if a system has a struct timespec.
 * For the same reason, and because we'd have to #include <sys/time.h> in db.h,
 * we don't export any timespec structures in the DB API, even in places where
 * it would make sense, like the replication statistics information.
 */
typedef struct {
	__time64_t	tv_sec;				/* seconds */
#ifdef HAVE_MIXED_SIZE_ADDRESSING
	int32 tv_nsec;
#else
	long	tv_nsec;			/* nanoseconds */
#endif
} db_timespec;

/* Operations on timespecs */
#undef	timespecclear
#define	timespecclear(tvp)	((tvp)->tv_sec = (tvp)->tv_nsec = 0)
#undef	timespecisset
#define	timespecisset(tvp)	((tvp)->tv_sec || (tvp)->tv_nsec)
#undef	timespeccmp
#define	timespeccmp(tvp, uvp, cmp)					\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?				\
	    ((tvp)->tv_nsec cmp (uvp)->tv_nsec) :			\
	    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#undef timespecadd
/*
 * Note that using timespecadd to add to yourself (i.e. doubling)
 * must be supported.
 */
#define	timespecadd(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec += (uvp)->tv_sec;				\
		(vvp)->tv_nsec += (uvp)->tv_nsec;			\
		if((vvp)->tv_nsec >= SlConst::OneBillion) { \
			(vvp)->tv_sec++;				\
			(vvp)->tv_nsec -= SlConst::OneBillion; \
		}							\
	} while(0)
#undef timespecsub
#define	timespecsub(vvp, uvp)						\
	do {								\
		(vvp)->tv_sec -= (uvp)->tv_sec;				\
		(vvp)->tv_nsec -= (uvp)->tv_nsec;			\
		if((vvp)->tv_nsec < 0) {				\
			(vvp)->tv_sec--;				\
			(vvp)->tv_nsec += SlConst::OneBillion; \
		}							\
	} while(0)

#undef timespecset
#define	timespecset(vvp, sec, nsec)					\
	do {								\
		(vvp)->tv_sec = static_cast<__time64_t>(sec);				\
		(vvp)->tv_nsec = static_cast<long>(nsec);				\
	} while(0)

#define	DB_TIMEOUT_TO_TIMESPEC(t, vvp)					\
	do {								\
		(vvp)->tv_sec = static_cast<__time64_t>((t) / 1000000);		\
		(vvp)->tv_nsec = static_cast<long>(((t) % 1000000) * 1000);	\
	} while(0)

#define	DB_TIMESPEC_TO_TIMEOUT(t, vvp, prec)				\
	do {								\
		t = static_cast<ulong>((vvp)->tv_sec * 1000000);			\
		t += static_cast<ulong>((vvp)->tv_nsec / 1000);			\
		/* Add in 1 usec for lost nsec precision if wanted. */	\
		if(prec)						\
			t++;						\
	} while(0)

#define	TIMESPEC_ADD_DB_TIMEOUT(vvp, t)			        \
	do {							        \
		db_timespec __tmp;				        \
		DB_TIMEOUT_TO_TIMESPEC(t, &__tmp);		        \
		timespecadd((vvp), &__tmp);			        \
	} while(0)

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_CLOCK_H_ */
