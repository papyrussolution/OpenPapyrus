/*	Copyright (c) 1989 Citadel	*/
/*	   All Rights Reserved    	*/

/*man---------------------------------------------------------------------------
NAME
	 blkio - block buffered input/output library

SYNOPSIS
	 #include <blkio.h>

DESCRIPTION
	 blkio is a buffered input/output library for structured files.
	 Because structured files are primarily accessed randomly rather
	 than sequentially, they are better modeled as collections of
	 blocks rather than as streams of characters.  This library may be
	 used with files which fit the following criteria:

		  o A header of arbitrary (possibly zero) but fixed
			length appears at the beginning of the file.
		  o The data following the header is arranged in
			blocks of uniform size.

	 Files fitting this model are referred to in the documentation as
	 block files.

	 A file to be accessed with the blkio library is declared to be a
	 pointer to a defined type BLKFILE.  The bopen function creates
	 certain descriptive data for the file and returns a pointer to
	 designate it in all further transactions.

SEE ALSO
	 bclose, bcloseall, bexit, bflpop, bflpush, bflush, bgetb, bgetbf,
	 bgeth, bgethf, bopen, bputb, bputbf, bputh, bputhf, bsetbuf,
	 bsetvbuf, bsync, lockb.

------------------------------------------------------------------------------*/
#ifndef __BLKIO_H
#define __BLKIO_H

#ifndef __STDDEF_H
#	include <stddef.h>
#endif
#ifndef __STDIO_H
#	include <stdio.h>
#endif
#ifndef __SLIB_H
#	include <slib.h>
#endif

/* constants */
#define BOPEN_MAX	(FOPEN_MAX > 60 ? FOPEN_MAX : 60)
					/* max # block files open at once */
#define NIL		((bpos_t)0)	/* nil file pointer */
#define NUL		('\0')		/* nul char */

/* block file open types */
#define BF_READ		1 /*("r")*/
#define BF_RDWR		2 /*("r+")*/
#define BF_CRTR		3 /*("w+")*/
#define BF_CREATE	4 /*("c")*/

/* macro for number elements in a static array */
#define nelems(v)	(sizeof((v)) / sizeof(*(v)))

/* macro for sizeof a structure member */
#define sizeofm(struct_t, member) ((size_t)(sizeof(((struct_t *)0)->member)))

/* type definitions */
typedef unsigned long bpos_t; /* block file position */

typedef union {	 /* file desciptor type */
	char	c;	 /* character file descriptor */
	short	s;	 /* short int file descriptor */
	int	i;		 /* int file descriptor */
} fd_t;

typedef struct {	/* block structure */
	bpos_t	bn;		/* block number */
	int	flags;		/* block status flags */
	size_t	more;	/* link to more recently accessed block */
	size_t	less;	/* link to less recently accessed block */
} block_t;

typedef struct {	 /* block file control structure */
	fd_t	fd;      /* file descriptor for buffered file */
	int	flags;	 /* buffer status flags */
	size_t	hdrsize; /* size of file header */
	size_t	blksize; /* size of blocks */
	size_t	bufcnt;	 /* number blocks to buffer (0 if unbuffered) */
	bpos_t	endblk;	 /* first block past end of file */
	size_t	most;	 /* most recently accessed block [1..bufcnt] */
	size_t	least;	 /* least recently accessed block [1..bufcnt] */
	block_t *blockp; /* doubly linked list of blocks */
	void *	blkbuf;	 /* buffer storage for header and blocks */
} BLKFILE;

SPTRDEF(BLKFILE);

#ifdef __cplusplus
extern "C" {
#endif

/* function declarations */
int		SLAPI bclose(BLKFILE *bp);
int		SLAPI bcloseall(void);
void		SLAPI bexit(int status);
int		SLAPI bflpop(BLKFILE *bp, bpos_t *bnp);
int		SLAPI bflpush(BLKFILE *bp, const bpos_t *bnp);
int		SLAPI bflush(BLKFILE *bp);
int		SLAPI bgetb(BLKFILE *bp, bpos_t bn, void *buf);
int		SLAPI bgetbf(BLKFILE *bp, bpos_t bn, size_t offset, void *buf, size_t bufsize);
int		SLAPI bgeth(BLKFILE *bp, void *buf);
int		SLAPI bgethf(BLKFILE *bp, size_t offset, void *buf, size_t bufsize);
BLKFILE * 	SLAPI bopen(const char *filename, int type, size_t hdrsize, size_t blksize, size_t bufcnt);
int		SLAPI bputb(BLKFILE *bp, bpos_t bn, const void *buf);
int		SLAPI bputbf(BLKFILE *bp, bpos_t bn, size_t offset, const void *buf, size_t bufsize);
int		SLAPI bputh(BLKFILE *bp, const void *buf);
int		SLAPI bputhf(BLKFILE *bp, size_t offset, const void *buf, size_t bufsize);
int		SLAPI bsetbuf(BLKFILE *bp, void *buf);
int		SLAPI bsetvbuf(BLKFILE *bp, void *buf, size_t blksize, size_t bufcnt);
int		SLAPI bsync(BLKFILE *bp);
int		SLAPI lockb(BLKFILE *bp, int ltype, bpos_t start, bpos_t len);

#ifdef __cplusplus
}
#endif

/* lock types */

#define B_UNLCK		(0)	/* unlock */
#define B_RDLCK		(1)	/* read lock */
#define B_WRLCK		(2)	/* write lock */
#define B_RDLKW		(3)	/* read lock, wait */
#define B_WRLKW		(4)	/* write lock, wait */

/* error codes */

#define BEOS		(0)		/* start of blkio error code domain */
#define BEMFILE		(BEOS - 1)	/* too many block files open */
#define BENOPEN		(BEOS - 2)	/* block file is not open */
#define BENBUF		(BEOS - 3)	/* buffering is off */
#define BEBUF		(BEOS - 4)	/* buffering is on */
#define BEBOUND		(BEOS - 5)	/* block boundary error */
#define BEEOF		(BEOS - 6)	/* past end of file */
#define BENFL		(BEOS - 7)	/* no free list */
#define BEPANIC		(BEOS - 8)	/* internal blkio error */
#define BEWRITE		(BEOS - 9)	/* writing error (Probably disk is full) (Sobolev) */
#define BELOCKED	(BEOS - 10)	/* rec is locked (Sobolev) */

#endif	/* __BLKIO_H */
