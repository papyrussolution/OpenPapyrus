/*====================================================================*
   -  Copyright (C) 2001 Leptonica.  All rights reserved.
   -
   -  Redistribution and use in source and binary forms, with or without
   -  modification, are permitted provided that the following conditions
   -  are met:
   -  1. Redistributions of source code must retain the above copyright
   -     notice, this list of conditions and the following disclaimer.
   -  2. Redistributions in binary form must reproduce the above
   -     copyright notice, this list of conditions and the following
   -     disclaimer in the documentation and/or other materials
   -     provided with the distribution.
*====================================================================*/
/*!
 * \file  ptabasic.c
 * <pre>
 *
 *      Pta creation, destruction, copy, clone, empty
 *           PTA            *ptaCreate()
 *           PTA            *ptaCreateFromNuma()
 *           void            ptaDestroy()
 *           PTA            *ptaCopy()
 *           PTA            *ptaCopyRange()
 *           PTA            *ptaClone()
 *           int32         ptaEmpty()
 *
 *      Pta array extension
 *           int32         ptaAddPt()
 *           static int32  ptaExtendArrays()
 *
 *      Pta insertion and removal
 *           int32         ptaInsertPt()
 *           int32         ptaRemovePt()
 *
 *      Pta accessors
 *           int32         ptaGetRefcount()
 *           int32         ptaChangeRefcount()
 *           int32         ptaGetCount()
 *           int32         ptaGetPt()
 *           int32         ptaGetIPt()
 *           int32         ptaSetPt()
 *           int32         ptaGetArrays()
 *
 *      Pta serialized for I/O
 *           PTA            *ptaRead()
 *           PTA            *ptaReadStream()
 *           PTA            *ptaReadMem()
 *           int32         ptaWriteDebug()
 *           int32         ptaWrite()
 *           int32         ptaWriteStream()
 *           int32         ptaWriteMem()
 *
 *      Ptaa creation, destruction
 *           PTAA           *ptaaCreate()
 *           void            ptaaDestroy()
 *
 *      Ptaa array extension
 *           int32         ptaaAddPta()
 *           static int32  ptaaExtendArray()
 *
 *      Ptaa accessors
 *           int32         ptaaGetCount()
 *           int32         ptaaGetPta()
 *           int32         ptaaGetPt()
 *
 *      Ptaa array modifiers
 *           int32         ptaaInitFull()
 *           int32         ptaaReplacePta()
 *           int32         ptaaAddPt()
 *           int32         ptaaTruncate()
 *
 *      Ptaa serialized for I/O
 *           PTAA           *ptaaRead()
 *           PTAA           *ptaaReadStream()
 *           PTAA           *ptaaReadMem()
 *           int32         ptaaWrite()
 *           int32         ptaaWriteStream()
 *           int32         ptaaWriteMem()
 * </pre>
 */
#include "allheaders.h"
#pragma hdrstop

static const uint32 MaxArraySize = 100000000; /* 100 million */
static const uint32 MaxPtrArraySize = 10000000; /* 10 million */
static const int32 InitialArraySize = 50; /*!< n'importe quoi */

/* Static functions */
static int32 ptaExtendArrays(PTA * pta);
static int32 ptaaExtendArray(PTAA * ptaa);

/*---------------------------------------------------------------------*
*                Pta creation, destruction, copy, clone               *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaCreate()
 *
 * \param[in]    n    initial array sizes
 * \return  pta, or NULL on error.
 */
PTA * ptaCreate(int32 n)
{
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(n <= 0 || n > MaxArraySize)
		n = InitialArraySize;

	pta = (PTA*)SAlloc::C(1, sizeof(PTA));
	pta->n = 0;
	pta->nalloc = n;
	ptaChangeRefcount(pta, 1); /* sets to 1 */
	pta->x = (float *)SAlloc::C(n, sizeof(float));
	pta->y = (float *)SAlloc::C(n, sizeof(float));
	if(!pta->x || !pta->y) {
		ptaDestroy(&pta);
		return (PTA*)ERROR_PTR("x and y arrays not both made", procName, NULL);
	}

	return pta;
}

/*!
 * \brief   ptaCreateFromNuma()
 *
 * \param[in]    nax   [optional] can be null
 * \param[in]    nay
 * \return  pta, or NULL on error.
 */
PTA * ptaCreateFromNuma(NUMA * nax,
    NUMA * nay)
{
	int32 i, n;
	float startx, delx, xval, yval;
	PTA       * pta;

	PROCNAME(__FUNCTION__);

	if(!nay)
		return (PTA*)ERROR_PTR("nay not defined", procName, NULL);
	n = numaGetCount(nay);
	if(nax && numaGetCount(nax) != n)
		return (PTA*)ERROR_PTR("nax and nay sizes differ", procName, NULL);

	pta = ptaCreate(n);
	numaGetParameters(nay, &startx, &delx);
	for(i = 0; i < n; i++) {
		if(nax)
			numaGetFValue(nax, i, &xval);
		else /* use implicit x values from nay */
			xval = startx + i * delx;
		numaGetFValue(nay, i, &yval);
		ptaAddPt(pta, xval, yval);
	}

	return pta;
}

/*!
 * \brief   ptaDestroy()
 *
 * \param[in,out]   ppta   will be set to null before returning
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) Decrements the ref count and, if 0, destroys the pta.
 *      (2) Always nulls the input ptr.
 * </pre>
 */
void ptaDestroy(PTA ** ppta)
{
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(ppta == NULL) {
		L_WARNING("ptr address is NULL!\n", procName);
		return;
	}

	if((pta = *ppta) == NULL)
		return;

	ptaChangeRefcount(pta, -1);
	if(ptaGetRefcount(pta) <= 0) {
		SAlloc::F(pta->x);
		SAlloc::F(pta->y);
		SAlloc::F(pta);
	}
	*ppta = NULL;
}

/*!
 * \brief   ptaCopy()
 *
 * \param[in]    pta
 * \return  copy of pta, or NULL on error
 */
PTA * ptaCopy(PTA * pta)
{
	int32 i;
	float x, y;
	PTA       * npta;

	PROCNAME(__FUNCTION__);

	if(!pta)
		return (PTA*)ERROR_PTR("pta not defined", procName, NULL);

	if((npta = ptaCreate(pta->nalloc)) == NULL)
		return (PTA*)ERROR_PTR("npta not made", procName, NULL);

	for(i = 0; i < pta->n; i++) {
		ptaGetPt(pta, i, &x, &y);
		ptaAddPt(npta, x, y);
	}

	return npta;
}

/*!
 * \brief   ptaCopyRange()
 *
 * \param[in]    ptas
 * \param[in]    istart    starting index in ptas
 * \param[in]    iend      ending index in ptas; use 0 to copy to end
 * \return  0 if OK, 1 on error
 */
PTA * ptaCopyRange(PTA * ptas,
    int32 istart,
    int32 iend)
{
	int32 n, i, x, y;
	PTA * ptad;

	PROCNAME(__FUNCTION__);

	if(!ptas)
		return (PTA*)ERROR_PTR("ptas not defined", procName, NULL);
	n = ptaGetCount(ptas);
	if(istart < 0)
		istart = 0;
	if(istart >= n)
		return (PTA*)ERROR_PTR("istart out of bounds", procName, NULL);
	if(iend <= 0 || iend >= n)
		iend = n - 1;
	if(istart > iend)
		return (PTA*)ERROR_PTR("istart > iend; no pts", procName, NULL);

	if((ptad = ptaCreate(iend - istart + 1)) == NULL)
		return (PTA*)ERROR_PTR("ptad not made", procName, NULL);
	for(i = istart; i <= iend; i++) {
		ptaGetIPt(ptas, i, &x, &y);
		ptaAddPt(ptad, x, y);
	}

	return ptad;
}

/*!
 * \brief   ptaClone()
 *
 * \param[in]    pta
 * \return  ptr to same pta, or NULL on error
 */
PTA * ptaClone(PTA * pta)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return (PTA*)ERROR_PTR("pta not defined", procName, NULL);

	ptaChangeRefcount(pta, 1);
	return pta;
}

/*!
 * \brief   ptaEmpty()
 *
 * \param[in]    pta
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      This only resets the Pta::n field, for reuse
 * </pre>
 */
l_ok ptaEmpty(PTA * pta)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("ptad not defined", procName, 1);
	pta->n = 0;
	return 0;
}

/*---------------------------------------------------------------------*
*                         Pta array extension                         *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaAddPt()
 *
 * \param[in]    pta
 * \param[in]    x, y
 * \return  0 if OK, 1 on error
 */
l_ok ptaAddPt(PTA       * pta,
    float x,
    float y)
{
	int32 n;

	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

	n = pta->n;
	if(n >= pta->nalloc) {
		if(ptaExtendArrays(pta))
			return ERROR_INT("extension failed", procName, 1);
	}

	pta->x[n] = x;
	pta->y[n] = y;
	pta->n++;
	return 0;
}

/*!
 * \brief   ptaExtendArrays()
 *
 * \param[in]    pta
 * \return  0 if OK; 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Doubles the size of the array.
 *      (2) The max number of points is 100M.
 * </pre>
 */
static int32 ptaExtendArrays(PTA * pta)
{
	size_t oldsize, newsize;

	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	if(pta->nalloc > MaxArraySize)
		return ERROR_INT("pta at maximum size; can't extend", procName, 1);
	oldsize = 4 * pta->nalloc;
	if(pta->nalloc > MaxArraySize / 2) {
		newsize = 4 * MaxArraySize;
		pta->nalloc = MaxArraySize;
	}
	else {
		newsize = 2 * oldsize;
		pta->nalloc *= 2;
	}
	if((pta->x = (float *)reallocNew((void**)&pta->x,
	    oldsize, newsize)) == NULL)
		return ERROR_INT("new x array not returned", procName, 1);
	if((pta->y = (float *)reallocNew((void**)&pta->y,
	    oldsize, newsize)) == NULL)
		return ERROR_INT("new y array not returned", procName, 1);

	return 0;
}

/*---------------------------------------------------------------------*
*                     Pta insertion and removal                       *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaInsertPt()
 *
 * \param[in]    pta
 * \param[in]    index   at which pt is to be inserted
 * \param[in]    x, y    point values
 * \return  0 if OK; 1 on error
 */
l_ok ptaInsertPt(PTA * pta,
    int32 index,
    int32 x,
    int32 y)
{
	int32 i, n;

	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	n = ptaGetCount(pta);
	if(index < 0 || index > n) {
		L_ERROR("index %d not in [0,...,%d]\n", procName, index, n);
		return 1;
	}

	if(n > pta->nalloc) {
		if(ptaExtendArrays(pta))
			return ERROR_INT("extension failed", procName, 1);
	}
	pta->n++;
	for(i = n; i > index; i--) {
		pta->x[i] = pta->x[i - 1];
		pta->y[i] = pta->y[i - 1];
	}
	pta->x[index] = x;
	pta->y[index] = y;
	return 0;
}

/*!
 * \brief   ptaRemovePt()
 *
 * \param[in]    pta
 * \param[in]    index    of point to be removed
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This shifts pta[i] --> pta[i - 1] for all i > index.
 *      (2) It should not be used repeatedly on large arrays,
 *          because the function is O(n).
 * </pre>
 */
l_ok ptaRemovePt(PTA * pta,
    int32 index)
{
	int32 i, n;

	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	n = ptaGetCount(pta);
	if(index < 0 || index >= n) {
		L_ERROR("index %d not in [0,...,%d]\n", procName, index, n - 1);
		return 1;
	}

	/* Remove the point */
	for(i = index + 1; i < n; i++) {
		pta->x[i - 1] = pta->x[i];
		pta->y[i - 1] = pta->y[i];
	}
	pta->n--;
	return 0;
}

/*---------------------------------------------------------------------*
*                           Pta accessors                             *
*---------------------------------------------------------------------*/
int32 ptaGetRefcount(PTA * pta)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	return pta->refcount;
}

int32 ptaChangeRefcount(PTA * pta,
    int32 delta)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	pta->refcount += delta;
	return 0;
}

/*!
 * \brief   ptaGetCount()
 *
 * \param[in]    pta
 * \return  count, or 0 if no pta
 */
int32 ptaGetCount(PTA * pta)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 0);

	return pta->n;
}

/*!
 * \brief   ptaGetPt()
 *
 * \param[in]    pta
 * \param[in]    index    into arrays
 * \param[out]   px       [optional] float x value
 * \param[out]   py       [optional] float y value
 * \return  0 if OK; 1 on error
 */
l_ok ptaGetPt(PTA * pta,
    int32 index,
    float * px,
    float * py)
{
	PROCNAME(__FUNCTION__);

	if(px) *px = 0;
	if(py) *py = 0;
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	if(index < 0 || index >= pta->n)
		return ERROR_INT("invalid index", procName, 1);

	if(px) *px = pta->x[index];
	if(py) *py = pta->y[index];
	return 0;
}

/*!
 * \brief   ptaGetIPt()
 *
 * \param[in]    pta
 * \param[in]    index    into arrays
 * \param[out]   px       [optional] integer x value
 * \param[out]   py       [optional] integer y value
 * \return  0 if OK; 1 on error
 */
l_ok ptaGetIPt(PTA * pta,
    int32 index,
    int32 * px,
    int32 * py)
{
	PROCNAME(__FUNCTION__);

	if(px) *px = 0;
	if(py) *py = 0;
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	if(index < 0 || index >= pta->n)
		return ERROR_INT("invalid index", procName, 1);

	if(px) *px = (int32)(pta->x[index] + 0.5);
	if(py) *py = (int32)(pta->y[index] + 0.5);
	return 0;
}

/*!
 * \brief   ptaSetPt()
 *
 * \param[in]    pta
 * \param[in]    index    into arrays
 * \param[in]    x, y
 * \return  0 if OK; 1 on error
 */
l_ok ptaSetPt(PTA       * pta,
    int32 index,
    float x,
    float y)
{
	PROCNAME(__FUNCTION__);

	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	if(index < 0 || index >= pta->n)
		return ERROR_INT("invalid index", procName, 1);

	pta->x[index] = x;
	pta->y[index] = y;
	return 0;
}

/*!
 * \brief   ptaGetArrays()
 *
 * \param[in]    pta
 * \param[out]   pnax    [optional] numa of x array
 * \param[out]   pnay    [optional] numa of y array
 * \return  0 if OK; 1 on error or if pta is empty
 *
 * <pre>
 * Notes:
 *      (1) This copies the internal arrays into new Numas.
 * </pre>
 */
l_ok ptaGetArrays(PTA    * pta,
    NUMA ** pnax,
    NUMA ** pnay)
{
	int32 i, n;
	NUMA * nax, * nay;

	PROCNAME(__FUNCTION__);

	if(!pnax && !pnay)
		return ERROR_INT("no output requested", procName, 1);
	if(pnax) *pnax = NULL;
	if(pnay) *pnay = NULL;
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	if((n = ptaGetCount(pta)) == 0)
		return ERROR_INT("pta is empty", procName, 1);

	if(pnax) {
		if((nax = numaCreate(n)) == NULL)
			return ERROR_INT("nax not made", procName, 1);
		*pnax = nax;
		for(i = 0; i < n; i++)
			nax->array[i] = pta->x[i];
		nax->n = n;
	}
	if(pnay) {
		if((nay = numaCreate(n)) == NULL)
			return ERROR_INT("nay not made", procName, 1);
		*pnay = nay;
		for(i = 0; i < n; i++)
			nay->array[i] = pta->y[i];
		nay->n = n;
	}
	return 0;
}

/*---------------------------------------------------------------------*
*                       Pta serialized for I/O                        *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaRead()
 *
 * \param[in]    filename
 * \return  pta, or NULL on error
 */
PTA * ptaRead(const char * filename)
{
	FILE * fp;
	PTA   * pta;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return (PTA*)ERROR_PTR("filename not defined", procName, NULL);

	if((fp = fopenReadStream(filename)) == NULL)
		return (PTA*)ERROR_PTR("stream not opened", procName, NULL);
	pta = ptaReadStream(fp);
	fclose(fp);
	if(!pta)
		return (PTA*)ERROR_PTR("pta not read", procName, NULL);
	return pta;
}

/*!
 * \brief   ptaReadStream()
 *
 * \param[in]    fp    file stream
 * \return  pta, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) It is OK for the pta to be empty (n == 0).
 * </pre>

 */
PTA * ptaReadStream(FILE * fp)
{
	char typestr[128]; /* hardcoded below in fscanf */
	int32 i, n, ix, iy, type, version;
	float x, y;
	PTA       * pta;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return (PTA*)ERROR_PTR("stream not defined", procName, NULL);

	if(fscanf(fp, "\n Pta Version %d\n", &version) != 1)
		return (PTA*)ERROR_PTR("not a pta file", procName, NULL);
	if(version != PTA_VERSION_NUMBER)
		return (PTA*)ERROR_PTR("invalid pta version", procName, NULL);
	if(fscanf(fp, " Number of pts = %d; format = %127s\n", &n, typestr) != 2)
		return (PTA*)ERROR_PTR("not a pta file", procName, NULL);
	if(n < 0)
		return (PTA*)ERROR_PTR("num pts <= 0", procName, NULL);
	if(n > MaxArraySize)
		return (PTA*)ERROR_PTR("too many pts", procName, NULL);
	if(n == 0) L_INFO("the pta is empty\n", procName);

	if(!strcmp(typestr, "float"))
		type = 0;
	else /* typestr is "integer" */
		type = 1;
	if((pta = ptaCreate(n)) == NULL)
		return (PTA*)ERROR_PTR("pta not made", procName, NULL);
	for(i = 0; i < n; i++) {
		if(type == 0) { /* data is float */
			if(fscanf(fp, "   (%f, %f)\n", &x, &y) != 2) {
				ptaDestroy(&pta);
				return (PTA*)ERROR_PTR("error reading floats", procName, NULL);
			}
			ptaAddPt(pta, x, y);
		}
		else { /* data is integer */
			if(fscanf(fp, "   (%d, %d)\n", &ix, &iy) != 2) {
				ptaDestroy(&pta);
				return (PTA*)ERROR_PTR("error reading ints", procName, NULL);
			}
			ptaAddPt(pta, ix, iy);
		}
	}

	return pta;
}

/*!
 * \brief   ptaReadMem()
 *
 * \param[in]    data    serialization in ascii
 * \param[in]    size    of data in bytes; can use strlen to get it
 * \return  pta, or NULL on error
 */
PTA * ptaReadMem(const uint8  * data,
    size_t size)
{
	FILE * fp;
	PTA   * pta;

	PROCNAME(__FUNCTION__);

	if(!data)
		return (PTA*)ERROR_PTR("data not defined", procName, NULL);
	if((fp = fopenReadFromMemory(data, size)) == NULL)
		return (PTA*)ERROR_PTR("stream not opened", procName, NULL);

	pta = ptaReadStream(fp);
	fclose(fp);
	if(!pta) L_ERROR("pta not read\n", procName);
	return pta;
}

/*!
 * \brief   ptaWriteDebug()
 *
 * \param[in]    filename
 * \param[in]    pta
 * \param[in]    type       0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Debug version, intended for use in the library when writing
 *          to files in a temp directory with names that are compiled in.
 *          This is used instead of ptaWrite() for all such library calls.
 *      (2) The global variable LeptDebugOK defaults to 0, and can be set
 *          or cleared by the function setLeptDebugOK().
 * </pre>
 */
l_ok ptaWriteDebug(const char * filename,
    PTA         * pta,
    int32 type)
{
	PROCNAME(__FUNCTION__);

	if(LeptDebugOK) {
		return ptaWrite(filename, pta, type);
	}
	else {
		L_INFO("write to named temp file %s is disabled\n", procName, filename);
		return 0;
	}
}

/*!
 * \brief   ptaWrite()
 *
 * \param[in]    filename
 * \param[in]    pta
 * \param[in]    type       0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 */
l_ok ptaWrite(const char * filename,
    PTA         * pta,
    int32 type)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return ERROR_INT("filename not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

	if((fp = fopenWriteStream(filename, "w")) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = ptaWriteStream(fp, pta, type);
	fclose(fp);
	if(ret)
		return ERROR_INT("pta not written to stream", procName, 1);
	return 0;
}

/*!
 * \brief   ptaWriteStream()
 *
 * \param[in]    fp      file stream
 * \param[in]    pta
 * \param[in]    type    0 for float values; 1 for integer values
 * \return  0 if OK; 1 on error
 */
l_ok ptaWriteStream(FILE * fp,
    PTA * pta,
    int32 type)
{
	int32 i, n, ix, iy;
	float x, y;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

	n = ptaGetCount(pta);
	fprintf(fp, "\n Pta Version %d\n", PTA_VERSION_NUMBER);
	if(type == 0)
		fprintf(fp, " Number of pts = %d; format = float\n", n);
	else /* type == 1 */
		fprintf(fp, " Number of pts = %d; format = integer\n", n);
	for(i = 0; i < n; i++) {
		if(type == 0) { /* data is float */
			ptaGetPt(pta, i, &x, &y);
			fprintf(fp, "   (%f, %f)\n", x, y);
		}
		else { /* data is integer */
			ptaGetIPt(pta, i, &ix, &iy);
			fprintf(fp, "   (%d, %d)\n", ix, iy);
		}
	}

	return 0;
}

/*!
 * \brief   ptaWriteMem()
 *
 * \param[out]   pdata    data of serialized pta; ascii
 * \param[out]   psize    size of returned data
 * \param[in]    pta
 * \param[in]    type     0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes a pta in memory and puts the result in a buffer.
 * </pre>
 */
l_ok ptaWriteMem(uint8  ** pdata,
    size_t * psize,
    PTA       * pta,
    int32 type)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(pdata) *pdata = NULL;
	if(psize) *psize = 0;
	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

#if HAVE_FMEMOPEN
	if((fp = open_memstream((char**)pdata, psize)) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = ptaWriteStream(fp, pta, type);
	fputc('\0', fp);
	fclose(fp);
	*psize = *psize - 1;
#else
	L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
	if((fp = fopenWriteWinTempfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
	if((fp = tmpfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
	ret = ptaWriteStream(fp, pta, type);
	rewind(fp);
	*pdata = l_binaryReadStream(fp, psize);
	fclose(fp);
#endif  /* HAVE_FMEMOPEN */
	return ret;
}

/*---------------------------------------------------------------------*
*                     PTAA creation, destruction                      *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaaCreate()
 *
 * \param[in]    n    initial number of ptrs
 * \return  ptaa, or NULL on error
 */
PTAA * ptaaCreate(int32 n)
{
	PTAA  * ptaa;

	PROCNAME(__FUNCTION__);

	if(n <= 0 || n > MaxPtrArraySize)
		n = InitialArraySize;

	ptaa = (PTAA*)SAlloc::C(1, sizeof(PTAA));
	ptaa->n = 0;
	ptaa->nalloc = n;
	if((ptaa->pta = (PTA**)SAlloc::C(n, sizeof(PTA *))) == NULL) {
		ptaaDestroy(&ptaa);
		return (PTAA*)ERROR_PTR("pta ptrs not made", procName, NULL);
	}
	return ptaa;
}

/*!
 * \brief   ptaaDestroy()
 *
 * \param[in,out]   pptaa   will be set to null before returning
 * \return  void
 */
void ptaaDestroy(PTAA  ** pptaa)
{
	int32 i;
	PTAA    * ptaa;

	PROCNAME(__FUNCTION__);

	if(pptaa == NULL) {
		L_WARNING("ptr address is NULL!\n", procName);
		return;
	}

	if((ptaa = *pptaa) == NULL)
		return;

	for(i = 0; i < ptaa->n; i++)
		ptaDestroy(&ptaa->pta[i]);
	SAlloc::F(ptaa->pta);
	SAlloc::F(ptaa);
	*pptaa = NULL;
}

/*---------------------------------------------------------------------*
*                          PTAA array extension                       *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaaAddPta()
 *
 * \param[in]    ptaa
 * \param[in]    pta         to be added
 * \param[in]    copyflag    L_INSERT, L_COPY, L_CLONE
 * \return  0 if OK, 1 on error
 */
l_ok ptaaAddPta(PTAA    * ptaa,
    PTA * pta,
    int32 copyflag)
{
	int32 n;
	PTA * ptac;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

	if(copyflag == L_INSERT) {
		ptac = pta;
	}
	else if(copyflag == L_COPY) {
		if((ptac = ptaCopy(pta)) == NULL)
			return ERROR_INT("ptac not made", procName, 1);
	}
	else if(copyflag == L_CLONE) {
		if((ptac = ptaClone(pta)) == NULL)
			return ERROR_INT("pta clone not made", procName, 1);
	}
	else {
		return ERROR_INT("invalid copyflag", procName, 1);
	}

	n = ptaaGetCount(ptaa);
	if(n >= ptaa->nalloc) {
		if(ptaaExtendArray(ptaa)) {
			if(copyflag != L_INSERT)
				ptaDestroy(&ptac);
			return ERROR_INT("extension failed", procName, 1);
		}
	}

	ptaa->pta[n] = ptac;
	ptaa->n++;
	return 0;
}

/*!
 * \brief   ptaaExtendArray()
 *
 * \param[in]    ptaa
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This doubles the pta ptr array size.
 *      (2) The max number of pta ptrs is 10M.
 * </pre>
 *
 */
static int32 ptaaExtendArray(PTAA  * ptaa)
{
	size_t oldsize, newsize;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	oldsize = ptaa->nalloc * sizeof(PTA *);
	newsize = 2 * oldsize;
	if(newsize > 8 * MaxPtrArraySize)
		return ERROR_INT("newsize > 80 MB; too large", procName, 1);

	if((ptaa->pta = (PTA**)reallocNew((void**)&ptaa->pta,
	    oldsize, newsize)) == NULL)
		return ERROR_INT("new ptr array not returned", procName, 1);

	ptaa->nalloc *= 2;
	return 0;
}

/*---------------------------------------------------------------------*
*                          Ptaa accessors                             *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaaGetCount()
 *
 * \param[in]    ptaa
 * \return  count, or 0 if no ptaa
 */
int32 ptaaGetCount(PTAA  * ptaa)
{
	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 0);

	return ptaa->n;
}

/*!
 * \brief   ptaaGetPta()
 *
 * \param[in]    ptaa
 * \param[in]    index         to the i-th pta
 * \param[in]    accessflag    L_COPY or L_CLONE
 * \return  pta, or NULL on error
 */
PTA * ptaaGetPta(PTAA    * ptaa,
    int32 index,
    int32 accessflag)
{
	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return (PTA*)ERROR_PTR("ptaa not defined", procName, NULL);
	if(index < 0 || index >= ptaa->n)
		return (PTA*)ERROR_PTR("index not valid", procName, NULL);

	if(accessflag == L_COPY)
		return ptaCopy(ptaa->pta[index]);
	else if(accessflag == L_CLONE)
		return ptaClone(ptaa->pta[index]);
	else
		return (PTA*)ERROR_PTR("invalid accessflag", procName, NULL);
}

/*!
 * \brief   ptaaGetPt()
 *
 * \param[in]    ptaa
 * \param[in]    ipta   to the i-th pta
 * \param[in]    jpt    index to the j-th pt in the pta
 * \param[out]   px     [optional] float x value
 * \param[out]   py     [optional] float y value
 * \return  0 if OK; 1 on error
 */
l_ok ptaaGetPt(PTAA       * ptaa,
    int32 ipta,
    int32 jpt,
    float * px,
    float * py)
{
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(px) *px = 0;
	if(py) *py = 0;
	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	if(ipta < 0 || ipta >= ptaa->n)
		return ERROR_INT("index ipta not valid", procName, 1);

	pta = ptaaGetPta(ptaa, ipta, L_CLONE);
	if(jpt < 0 || jpt >= pta->n) {
		ptaDestroy(&pta);
		return ERROR_INT("index jpt not valid", procName, 1);
	}

	ptaGetPt(pta, jpt, px, py);
	ptaDestroy(&pta);
	return 0;
}

/*---------------------------------------------------------------------*
*                        Ptaa array modifiers                         *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaaInitFull()
 *
 * \param[in]    ptaa    can have non-null ptrs in the ptr array
 * \param[in]    pta     to be replicated into the entire ptr array
 * \return  0 if OK; 1 on error
 */
l_ok ptaaInitFull(PTAA  * ptaa,
    PTA   * pta)
{
	int32 n, i;
	PTA * ptat;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);

	n = ptaa->nalloc;
	ptaa->n = n;
	for(i = 0; i < n; i++) {
		ptat = ptaCopy(pta);
		ptaaReplacePta(ptaa, i, ptat);
	}
	return 0;
}

/*!
 * \brief   ptaaReplacePta()
 *
 * \param[in]    ptaa
 * \param[in]    index   to the index-th pta
 * \param[in]    pta     insert and replace any existing one
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Any existing pta is destroyed, and the input one
 *          is inserted in its place.
 *      (2) If %index is invalid, return 1 (error)
 * </pre>
 */
l_ok ptaaReplacePta(PTAA    * ptaa,
    int32 index,
    PTA * pta)
{
	int32 n;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	if(!pta)
		return ERROR_INT("pta not defined", procName, 1);
	n = ptaaGetCount(ptaa);
	if(index < 0 || index >= n)
		return ERROR_INT("index not valid", procName, 1);

	ptaDestroy(&ptaa->pta[index]);
	ptaa->pta[index] = pta;
	return 0;
}

/*!
 * \brief   ptaaAddPt()
 *
 * \param[in]    ptaa
 * \param[in]    ipta   to the i-th pta
 * \param[in]    x,y    point coordinates
 * \return  0 if OK; 1 on error
 */
l_ok ptaaAddPt(PTAA      * ptaa,
    int32 ipta,
    float x,
    float y)
{
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);
	if(ipta < 0 || ipta >= ptaa->n)
		return ERROR_INT("index ipta not valid", procName, 1);

	pta = ptaaGetPta(ptaa, ipta, L_CLONE);
	ptaAddPt(pta, x, y);
	ptaDestroy(&pta);
	return 0;
}

/*!
 * \brief   ptaaTruncate()
 *
 * \param[in]    ptaa
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This identifies the largest index containing a pta that
 *          has any points within it, destroys all pta above that index,
 *          and resets the count.
 * </pre>
 */
l_ok ptaaTruncate(PTAA  * ptaa)
{
	int32 i, n, np;
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);

	n = ptaaGetCount(ptaa);
	for(i = n - 1; i >= 0; i--) {
		pta = ptaaGetPta(ptaa, i, L_CLONE);
		if(!pta) {
			ptaa->n--;
			continue;
		}
		np = ptaGetCount(pta);
		ptaDestroy(&pta);
		if(np == 0) {
			ptaDestroy(&ptaa->pta[i]);
			ptaa->n--;
		}
		else {
			break;
		}
	}
	return 0;
}

/*---------------------------------------------------------------------*
*                       Ptaa serialized for I/O                       *
*---------------------------------------------------------------------*/
/*!
 * \brief   ptaaRead()
 *
 * \param[in]    filename
 * \return  ptaa, or NULL on error
 */
PTAA * ptaaRead(const char * filename)
{
	FILE * fp;
	PTAA  * ptaa;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return (PTAA*)ERROR_PTR("filename not defined", procName, NULL);

	if((fp = fopenReadStream(filename)) == NULL)
		return (PTAA*)ERROR_PTR("stream not opened", procName, NULL);
	ptaa = ptaaReadStream(fp);
	fclose(fp);
	if(!ptaa)
		return (PTAA*)ERROR_PTR("ptaa not read", procName, NULL);
	return ptaa;
}

/*!
 * \brief   ptaaReadStream()
 *
 * \param[in]    fp    file stream
 * \return  ptaa, or NULL on error
 *
 * <pre>
 * Notes:
 *      (1) It is OK for the ptaa to be empty (n == 0).
 * </pre>
 */
PTAA * ptaaReadStream(FILE * fp)
{
	int32 i, n, version;
	PTA * pta;
	PTAA    * ptaa;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return (PTAA*)ERROR_PTR("stream not defined", procName, NULL);

	if(fscanf(fp, "\nPtaa Version %d\n", &version) != 1)
		return (PTAA*)ERROR_PTR("not a ptaa file", procName, NULL);
	if(version != PTA_VERSION_NUMBER)
		return (PTAA*)ERROR_PTR("invalid ptaa version", procName, NULL);
	if(fscanf(fp, "Number of Pta = %d\n", &n) != 1)
		return (PTAA*)ERROR_PTR("not a ptaa file", procName, NULL);
	if(n < 0)
		return (PTAA*)ERROR_PTR("num pta ptrs <= 0", procName, NULL);
	if(n > MaxPtrArraySize)
		return (PTAA*)ERROR_PTR("too many pta ptrs", procName, NULL);
	if(n == 0) L_INFO("the ptaa is empty\n", procName);

	if((ptaa = ptaaCreate(n)) == NULL)
		return (PTAA*)ERROR_PTR("ptaa not made", procName, NULL);
	for(i = 0; i < n; i++) {
		if((pta = ptaReadStream(fp)) == NULL) {
			ptaaDestroy(&ptaa);
			return (PTAA*)ERROR_PTR("error reading pta", procName, NULL);
		}
		ptaaAddPta(ptaa, pta, L_INSERT);
	}

	return ptaa;
}

/*!
 * \brief   ptaaReadMem()
 *
 * \param[in]    data    serialization in ascii
 * \param[in]    size    of data in bytes; can use strlen to get it
 * \return  ptaa, or NULL on error
 */
PTAA * ptaaReadMem(const uint8  * data,
    size_t size)
{
	FILE * fp;
	PTAA  * ptaa;

	PROCNAME(__FUNCTION__);

	if(!data)
		return (PTAA*)ERROR_PTR("data not defined", procName, NULL);
	if((fp = fopenReadFromMemory(data, size)) == NULL)
		return (PTAA*)ERROR_PTR("stream not opened", procName, NULL);

	ptaa = ptaaReadStream(fp);
	fclose(fp);
	if(!ptaa) L_ERROR("ptaa not read\n", procName);
	return ptaa;
}

/*!
 * \brief   ptaaWriteDebug()
 *
 * \param[in]    filename
 * \param[in]    ptaa
 * \param[in]    type      0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Debug version, intended for use in the library when writing
 *          to files in a temp directory with names that are compiled in.
 *          This is used instead of ptaaWrite() for all such library calls.
 *      (2) The global variable LeptDebugOK defaults to 0, and can be set
 *          or cleared by the function setLeptDebugOK().
 * </pre>
 */
l_ok ptaaWriteDebug(const char * filename,
    PTAA        * ptaa,
    int32 type)
{
	PROCNAME(__FUNCTION__);

	if(LeptDebugOK) {
		return ptaaWrite(filename, ptaa, type);
	}
	else {
		L_INFO("write to named temp file %s is disabled\n", procName, filename);
		return 0;
	}
}

/*!
 * \brief   ptaaWrite()
 *
 * \param[in]    filename
 * \param[in]    ptaa
 * \param[in]    type      0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 */
l_ok ptaaWrite(const char * filename,
    PTAA        * ptaa,
    int32 type)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(!filename)
		return ERROR_INT("filename not defined", procName, 1);
	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);

	if((fp = fopenWriteStream(filename, "w")) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = ptaaWriteStream(fp, ptaa, type);
	fclose(fp);
	if(ret)
		return ERROR_INT("ptaa not written to stream", procName, 1);
	return 0;
}

/*!
 * \brief   ptaaWriteStream()
 *
 * \param[in]    fp      file stream
 * \param[in]    ptaa
 * \param[in]    type    0 for float values; 1 for integer values
 * \return  0 if OK; 1 on error
 */
l_ok ptaaWriteStream(FILE * fp,
    PTAA    * ptaa,
    int32 type)
{
	int32 i, n;
	PTA * pta;

	PROCNAME(__FUNCTION__);

	if(!fp)
		return ERROR_INT("stream not defined", procName, 1);
	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);

	n = ptaaGetCount(ptaa);
	fprintf(fp, "\nPtaa Version %d\n", PTA_VERSION_NUMBER);
	fprintf(fp, "Number of Pta = %d\n", n);
	for(i = 0; i < n; i++) {
		pta = ptaaGetPta(ptaa, i, L_CLONE);
		ptaWriteStream(fp, pta, type);
		ptaDestroy(&pta);
	}

	return 0;
}

/*!
 * \brief   ptaaWriteMem()
 *
 * \param[out]   pdata    data of serialized ptaa; ascii
 * \param[out]   psize    size of returned data
 * \param[in]    ptaa
 * \param[in]    type     0 for float values; 1 for integer values
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) Serializes %ptaa in memory and puts the result in a buffer.
 * </pre>
 */
l_ok ptaaWriteMem(uint8  ** pdata,
    size_t * psize,
    PTAA      * ptaa,
    int32 type)
{
	int32 ret;
	FILE * fp;

	PROCNAME(__FUNCTION__);

	if(pdata) *pdata = NULL;
	if(psize) *psize = 0;
	if(!pdata)
		return ERROR_INT("&data not defined", procName, 1);
	if(!psize)
		return ERROR_INT("&size not defined", procName, 1);
	if(!ptaa)
		return ERROR_INT("ptaa not defined", procName, 1);

#if HAVE_FMEMOPEN
	if((fp = open_memstream((char**)pdata, psize)) == NULL)
		return ERROR_INT("stream not opened", procName, 1);
	ret = ptaaWriteStream(fp, ptaa, type);
	fputc('\0', fp);
	fclose(fp);
	*psize = *psize - 1;
#else
	L_INFO("work-around: writing to a temp file\n", procName);
  #ifdef _WIN32
	if((fp = fopenWriteWinTempfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #else
	if((fp = tmpfile()) == NULL)
		return ERROR_INT("tmpfile stream not opened", procName, 1);
  #endif  /* _WIN32 */
	ret = ptaaWriteStream(fp, ptaa, type);
	rewind(fp);
	*pdata = l_binaryReadStream(fp, psize);
	fclose(fp);
#endif  /* HAVE_FMEMOPEN */
	return ret;
}
