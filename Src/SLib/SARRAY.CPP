// SARRAY.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017
// @codepage windows-1251
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#define _USERPTR___(ptr) (ptr)
#define _USERPTR_PC(ptr) (*(void **)(ptr))
#define _USERPTR(ptr)    ((flags&aryPtrContainer)?(*(void **)(ptr)):(ptr))
#define _USERPTR_C(ptr)  ((flags&aryPtrContainer)?(*(const void **)(ptr)):(ptr))
#define _INTRPTR(ptr)    ((flags&aryPtrContainer)?&(ptr):(ptr))

SLAPI SArray::SArray(uint itemSize, uint aDelta, int o)
{
	isize = itemSize;
	delta = aDelta;
	flags = o;
	data  = 0;
	count = 0;
	pointer = 0;
	Limit = 0;
}

SLAPI SArray::SArray(uint itemSize)
{
	isize = itemSize;
	delta = 8;
	flags = O_ARRAY;
	data  = 0;
	count = 0;
	pointer = 0;
	Limit = 0;
}

SLAPI SArray::SArray(void * pData, uint itemSize, uint aCount, int o)
{
	isize = itemSize;
	count = aCount;
	Limit = aCount;
	delta = 1;
	flags = o;
	data  = (uint8 *)pData;
	pointer = 0;
}

SLAPI SArray::SArray(const SArray & src)
{
	data = 0;
	copy(src);
}

SLAPI SArray::~SArray()
{
	if(flags & aryDataOwner && data) // @v8.7.5 (&& data) ради ускорения //
		freeAll();
}

SArray & FASTCALL SArray::operator = (const SArray & src)
{
	copy(src);
	return *this;
}

int FASTCALL SArray::copy(const SArray & src)
{
	int    ok = 1;
	freeAll();
	if(&src != 0) {
		isize = src.isize;
		delta = src.delta;
		Limit = 0;
		count = 0;
		pointer = src.pointer;
		flags = src.flags;
		if(flags & aryDataOwner) {
			data = 0;
			if(expand(src.count)) {
				count = src.count;
				memcpy(data, src.data, isize * count);
			}
			else
				ok = 0;
		}
		else {
			Limit = src.Limit;
			count = src.count;
			data  = src.data;
		}
	}
	return ok;
}

int FASTCALL SArray::IsEqual(const SArray & src) const
{
	const uint16 tf = (aryPtrContainer|aryEachItem);
	if(!(flags & tf) && !(src.flags & tf))
		if(count == src.count && isize == src.isize)
			return (memcmp(data, src.data, isize * count) == 0) ? 1 : 0;
	return 0;
}

uint SLAPI SArray::getFlag() const
{
	return flags;
}

void FASTCALL SArray::freeItem(void * pItem)
{
	if(flags & aryPtrContainer)
		delete pItem;
}

void SLAPI SArray::setLimit(uint aLimit)
{
	Limit = aLimit;
}

void SLAPI SArray::setDelta(uint aDelta)
{
	delta = aDelta;
}

uint SLAPI SArray::getPointer() const
{
	return pointer;
}

uint FASTCALL SArray::setPointer(uint p)
{
	uint prev = pointer;
	pointer = p;
	return prev;
}

uint SLAPI SArray::incPointer()
{
	return pointer++;
}

uint FASTCALL SArray::incPointerSafe(int incr)
{
    const uint _prev = pointer;
    if(incr > 0) {
		pointer = MIN((pointer + incr), count-1);
    }
    else if(incr < 0) {
		if((-incr) > (int)pointer)
			pointer = 0;
		else
			pointer = pointer + incr;
    }
    return _prev;
}

void * SLAPI SArray::dataPtr() const
{
	return data;
}

void SLAPI SArray::setFlag(uint val, int set)
{
	SETFLAG(flags, val, set);
}

void * FASTCALL SArray::at(uint idx) const
{
	assert(idx < count);
	return _USERPTR(data + idx * isize);
}

void * SLAPI SArray::next()
{
	if(pointer < count) {
		pointer++;
		return _USERPTR(data + (pointer-1) * isize);
	}
	else
		return 0;
}

static size_t FASTCALL CalcArrayDelta(uint itemSize)
{
	const uint min_delta = 8;
	const uint align_bits = 8; // @v9.2.7 6-->8
    uint quant = Lcm(itemSize * min_delta, 1 << align_bits);
    assert((quant % itemSize) == 0);
    return quant / itemSize;
}

int FASTCALL SArray::expand(uint numNewRecs)
{
	int    ok = 1;
	uint   new_count = count + numNewRecs;
	if(new_count > Limit) {
		const size_t _is = (size_t)isize;
		// @v9.1.1 const uint _delta = (delta > 0) ? (uint)delta : 8;
		const  uint _delta = CalcArrayDelta(isize); // @v9.1.1
		uint   new_limit = ((new_count + (_delta - 1)) / _delta) * _delta;
		void * ptr = realloc(data, new_limit * _is);
		if(ptr) {
			data  = (uint8 *)ptr;
			Limit = new_limit;
		}
		else
			ok = (SLibError = SLERR_NOMEM, 0);
	}
	return ok;
}

int SLAPI SArray::shrink()
{
	// @v9.1.1 const  uint _delta = delta;
	const  uint _delta = CalcArrayDelta(isize); // @v9.1.1
	uint   newlimit = ((count + (uint)(_delta - 1)) / (uint)_delta) * (uint)_delta;
	if(newlimit < Limit) {
		Limit = newlimit;
		data  = (uint8 *)realloc(data, Limit * isize);
	}
	return 1;
}

int SLAPI SArray::atPut(uint idx, const void * pItem)
{
	assert(idx < count);
	memmove(data + idx * isize, _INTRPTR(pItem), isize);
	flags &= ~arySorted;
	return 1;
}
//
// Descr: Вставляет в середину блока памяти pDest новый блок pSrc.
//   Размер блока pDest - destSize. Размер блока pSrc - blkSize.
//   Вставка происходит по смещению (blkOffs * blkSize) байт от начала блока pDest.
//   После операции вставки размер блока pDest становится destSize+blkSize.
// ARG(pDest    IN): Указатель на блок-приемник
// ARG(pSrc     IN): Указатель на вставляемый блок
// ARG(blkOffs  IN): Смещение (в блоках blkSize) от указателя pDest по которому вставляется блок pSrc
// ARG(blkSize  IN): Размер блока pSrc в байтах
// ARG(destSize IN): Размер блока-приемника
//
static void SLAPI insmem(void * pDest, const void * pSrc, size_t blkOffs, size_t blkSize, size_t destSize)
{
	if(blkSize == sizeof(uint32)) {
		uint8 * d = PTR8(pDest) + blkOffs * sizeof(uint32);
		if(destSize > blkOffs)
			memmove(d + blkSize, d, (destSize - blkOffs) * sizeof(uint32));
		PTR32(d)[0] = PTR32(pSrc)[0];
	}
	else if(blkSize == sizeof(uint32)*2) {
		uint8 * d = PTR8(pDest) + (blkOffs * sizeof(uint32) * 2);
		if(destSize > blkOffs)
			memmove(d + blkSize, d, ((destSize - blkOffs) * sizeof(uint32) * 2));
		PTR32(d)[0] = PTR32(pSrc)[0];
		PTR32(d)[1] = PTR32(pSrc)[1];
	}
	else {
		uint8 * d = PTR8(pDest) + blkOffs * blkSize;
		if(destSize > blkOffs)
			memmove(d + blkSize, d, (destSize - blkOffs) * blkSize);
		memmove(d, pSrc, blkSize);
	}
}

int FASTCALL SArray::insert(const void * pItem)
{
	//
	// @v7.8.0 Так как данная функция очень часто вызывается, вместо вызова atInsert(count, pItem)
	// сделаем прямую реализацию функции, дабы сэкономить немного времени.
	//
	const  uint _c = count;
	int    ok = 1;
	if(_c < Limit || expand(1)) { // @v8.1.11 (count < limit ||) так быстрее ибо придется реже входить в функцию expand
		const size_t quant = (size_t)isize;
		//insmem(data, _INTRPTR(pItem), _c, quant, _c);
		//static void SLAPI insmem(void * pDest, const void * pSrc, size_t blkOffs, size_t blkSize, size_t destSize)
		{
			const void * p_intr = _INTRPTR(pItem);
			if(quant == sizeof(uint32)) {
				uint8 * d = PTR8(data) + (_c * sizeof(uint32));
				PTR32(d)[0] = PTR32(p_intr)[0];
			}
			else if(quant == sizeof(uint32)*2) {
				uint8 * d = PTR8(data) + (_c * sizeof(uint32) * 2);
				PTR32(d)[0] = PTR32(p_intr)[0];
				PTR32(d)[1] = PTR32(p_intr)[1];
			}
			else {
				uint8 * d = PTR8(data) + _c * quant;
				memmove(d, p_intr, quant);
			}
		}
		count++;
		flags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SArray::atInsert(uint idx, const void * pItem)
{
	int    ok = 1;
	const  uint _c = count;
	assert(idx <= _c);
	if(_c < Limit || expand(1)) { // @v8.1.11 (count < limit ||) так быстрее ибо придется реже входить в функцию expand
		insmem(data, _INTRPTR(pItem), idx, isize, _c);
		count++;
		flags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

int SLAPI SArray::insertChunk(uint numItemsToInsert, const void * pItems)
{
	int    ok = 1;
	if(expand(numItemsToInsert)) {
		uint8 * d = PTR8(data) + count * isize;
		if(pItems)
			memmove(d, pItems, isize * numItemsToInsert);
		else
			memzero(d, isize * numItemsToInsert);
		count += numItemsToInsert;
		flags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

static int substFCMP(const void * i1, const void * i2)
{
#ifndef _WIN32_WCE
	SlThreadLocalArea & tla = SLS.GetTLA();
	return (tla.SAry_PtrContainer ?
		tla.SAry_OrgFCMP(*(void **)i1, *(void **)i2, tla.SAry_SortExtraData) :
		tla.SAry_OrgFCMP(i1, i2, tla.SAry_SortExtraData));
#else
	return (SAry_PtrContainer ?
		SAry_OrgFCMP(*(void **)i1, *(void **)i2, SAry_SortExtraData) :
		SAry_OrgFCMP(i1, i2, SAry_SortExtraData));
#endif
}

int SLAPI SArray::sort(CompFunc fcmp, void * pExtraData /*=0*/)
{
	if(count > 1) {
#ifndef _WIN32_WCE
		SlThreadLocalArea & tla = SLS.GetTLA();
		tla.SAry_PtrContainer  = (flags & aryPtrContainer);
		tla.SAry_OrgFCMP       = fcmp;
		tla.SAry_SortExtraData = pExtraData;
#else
		SAry_PtrContainer  = (flags & aryPtrContainer);
		SAry_OrgFCMP       = fcmp;
		SAry_SortExtraData = pExtraData;
#endif
		qsort(data, count, isize, substFCMP);
	}
	flags |= arySorted;
	return 1;
}
//
// Реализация сортировки массива без вызова стандартной функции qsort.
// Фактически, это - адаптированная реализация qsort, встроенная в класс.
//

//
// this parameter defines the cutoff between using quick sort and
// insertion sort for arrays; arrays with lengths shorter or equal to the
// below value use insertion sort
//
#define CUTOFF 8         // testing shows that this is good value
#define STKSIZ (8*sizeof(void*) - 2)

int SLAPI SArray::Helper_Cmp(const void * i1, const void * i2, CompFunc fcmp, void * pExtraData) const
{
	return (flags & aryPtrContainer) ? fcmp(*(void **)i1, *(void **)i2, pExtraData) : fcmp(i1, i2, pExtraData);
}

int SLAPI SArray::sort2(CompFunc fcmp, void * pExtraData /*=0*/)
{
	/*
		Note: the number of stack entries required is no more than
		1 + log2(num), so 30 is sufficient for any array
	*/
	uint8 * mid;                  // points to middle of subarray
	uint8 * loguy, *higuy;        // traveling pointers for partition step
	size_t size;                 // size of the sub-array
	uint8 * lostk[STKSIZ], * histk[STKSIZ];
	const  size_t num = count;
	const  size_t width = isize;
	if(num > 1 && width > 0) {
		int    stkptr = 0; // stack for saving sub-array to be processed
		// lo and hi - ends of sub-array currently sorting
		uint8 * base = PTR8(data);
		uint8 * lo = base;
		uint8 * hi = base + width * (num-1); // initialize limits
		/*
			this entry point is for pseudo-recursion calling: setting
			lo and hi and jumping to here is like recursion, but stkptr is
			preserved, locals aren't, so we preserve stuff on the stack
		*/
recurse:
		size = (hi - lo) / width + 1; // number of el's to sort
		//
		// below a certain size, it is faster to use a O(n^2) sorting method
		//
		if(size <= CUTOFF) {
			//shortsort(lo, hi, width, comp);
			uint8 * p, * p_max;
			//
			// Note: in assertions below, i and j are alway inside original bound of array to sort.
			//
			while(hi > lo) {
				// A[i] <= A[j] for i <= j, j > hi
				p_max = lo;
				for(p = lo+width; p <= hi; p += width) {
					// A[i] <= A[max] for lo <= i < p
					if(Helper_Cmp(p, p_max, fcmp, pExtraData) > 0) {
						p_max = p;
					}
					// A[i] <= A[max] for lo <= i <= p
				}
				// A[i] <= A[max] for lo <= i <= hi
				memswap(p_max, hi, width);
				// A[i] <= A[hi] for i <= hi, so A[i] <= A[j] for i <= j, j >= hi
				hi -= width;
				// A[i] <= A[j] for i <= j, j > hi, loop top condition established
			}
			// A[i] <= A[j] for i <= j, j > lo, which implies A[i] <= A[j] for i < j, so array is sorted
		}
		else {
			/*
				First we pick a partitioning element.  The efficiency of the
				algorithm demands that we find one that is approximately the median
				of the values, but also that we select one fast.  We choose the
				median of the first, middle, and last elements, to avoid bad
				performance in the face of already sorted data, or data that is made
				up of multiple sorted runs appended together.  Testing shows that a
				median-of-three algorithm provides better performance than simply
				picking the middle element for the latter case.
			*/
			mid = lo + (size / 2) * width;      /* find middle element */
			/* Sort the first, middle, last elements into order */
			if(Helper_Cmp(lo, mid, fcmp, pExtraData) > 0)
				memswap(lo, mid, width);
			if(Helper_Cmp(lo, hi, fcmp, pExtraData) > 0)
				memswap(lo, hi, width);
			if(Helper_Cmp(mid, hi, fcmp, pExtraData) > 0)
				memswap(mid, hi, width);
			/*
				We now wish to partition the array into three pieces, one consisting
				of elements <= partition element, one of elements equal to the
				partition element, and one of elements > than it.  This is done
				below; comments indicate conditions established at every step.
			*/
			loguy = lo;
			higuy = hi;
			/*
				Note that higuy decreases and loguy increases on every iteration,
				so loop must terminate.
			*/
			for(;;) {
				/*
					lo <= loguy < hi, lo < higuy <= hi,
					A[i] <= A[mid] for lo <= i <= loguy,
					A[i] > A[mid] for higuy <= i < hi,
					A[hi] >= A[mid]
				*/
				/*
					The doubled loop is to avoid calling comp(mid,mid), since some
					existing comparison funcs don't work when passed the same
					value for both pointers.
				*/
				if(mid > loguy) {
					do {
						loguy += width;
					} while(loguy < mid && Helper_Cmp(loguy, mid, fcmp, pExtraData) <= 0);
				}
				if(mid <= loguy) {
					do {
						loguy += width;
					} while(loguy <= hi && Helper_Cmp(loguy, mid, fcmp, pExtraData) <= 0);
				}
				/*
					lo < loguy <= hi+1, A[i] <= A[mid] for lo <= i < loguy,
					either loguy > hi or A[loguy] > A[mid]
				*/
				do  {
					higuy -= width;
				} while(higuy > mid && Helper_Cmp(higuy, mid, fcmp, pExtraData) > 0);
				/*
					lo <= higuy < hi, A[i] > A[mid] for higuy < i < hi,
					either higuy == lo or A[higuy] <= A[mid]
				*/
				if(higuy < loguy)
					break;
				/*
					if loguy > hi or higuy == lo, then we would have exited, so
					A[loguy] > A[mid], A[higuy] <= A[mid],
					loguy <= hi, higuy > lo
				*/
				memswap(loguy, higuy, width);
				/*
					If the partition element was moved, follow it.  Only need
					to check for mid == higuy, since before the swap,
					A[loguy] > A[mid] implies loguy != mid.
				*/
				if(mid == higuy)
					mid = loguy;
				/*
					A[loguy] <= A[mid], A[higuy] > A[mid]; so condition at top
					of loop is re-established
				*/
			}
			/*
				A[i] <= A[mid] for lo <= i < loguy,
				A[i] > A[mid] for higuy < i < hi,
				A[hi] >= A[mid]
				higuy < loguy
				implying:
				higuy == loguy-1
				or higuy == hi - 1, loguy == hi + 1, A[hi] == A[mid]
			*/
			/*
				Find adjacent elements equal to the partition element.  The
				doubled loop is to avoid calling comp(mid,mid), since some
				existing comparison funcs don't work when passed the same value
				for both pointers.
			*/
			higuy += width;
			if(mid < higuy) {
				do {
					higuy -= width;
				} while(higuy > mid && Helper_Cmp(higuy, mid, fcmp, pExtraData) == 0);
			}
			if(mid >= higuy) {
				do {
					higuy -= width;
				} while(higuy > lo && Helper_Cmp(higuy, mid, fcmp, pExtraData) == 0);
			}
			/*
				OK, now we have the following:
				higuy < loguy
				lo <= higuy <= hi
				A[i]  <= A[mid] for lo <= i <= higuy
				A[i]  == A[mid] for higuy < i < loguy
				A[i]  >  A[mid] for loguy <= i < hi
				A[hi] >= A[mid]
			*/
			/*
				We've finished the partition, now we want to sort the subarrays
				[lo, higuy] and [loguy, hi].
				We do the smaller one first to minimize stack usage.
				We only sort arrays of length 2 or more.
			*/
			if(higuy - lo >= hi-loguy) {
				if(lo < higuy) {
					lostk[stkptr] = lo;
					histk[stkptr] = higuy;
					++stkptr;
				}                           /* save big recursion for later */
				if(loguy < hi) {
					lo = loguy;
					goto recurse;           /* do small recursion */
				}
			}
			else {
				if(loguy < hi) {
					lostk[stkptr] = loguy;
					histk[stkptr] = hi;
					++stkptr;               /* save big recursion for later */
				}
				if(lo < higuy) {
					hi = higuy;
					goto recurse;           /* do small recursion */
				}
			}
		}
		/*
			We have sorted the array, except for any pending sorts on the stack.
			Check if there are any, and do them.
		*/
		--stkptr;
		if(stkptr >= 0) {
			lo = lostk[stkptr];
			hi = histk[stkptr];
			goto recurse;           /* pop subarray from stack */
		}
		else
			return 1;                 /* all subarrays done */
	}
	return 1;
}
//
// Returns:
//   >0 - если элемент idx на выходе больше, чем key,
//   <0 - если элемент idx на выходе меньше, чем key,
//   0  - если поиск завершился успешно
//
int SLAPI SArray::imp_bsearch(const void * key, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	int    cmp = 1;
	uint   i  = 0;
	if(count) {
		uint   lo  = 0;
		uint   up  = count - 1;
		const  uint8 * p_org = data/* + ofs*/;
		//
		// Из-за популярности функции CMPF_LONG в простых массивах (без aryPtrContainer) выделяем в 
		// отдельную ветку такой случай - это даст значительное ускорение.
		//
		if(fcmp == CMPF_LONG && !(flags & aryPtrContainer)) {
			const long test = *(long *)key;
			while(lo <= up) {
				const uint8 * p = p_org + (i = (lo + up) >> 1) * isize;
				// @v9.6.1 cmp = fcmp(((const char *)_USERPTR_C(p))+ofs, key, pExtraData);
				cmp = CMPSIGN(*(long *)(p+ofs), test); // @v9.6.1
				if(cmp < 0)
					lo = i + 1;
				else if(cmp && i)
					up = i - 1;
				else {
					//
					// На случай, если мы попали не на самый первый элемент, удовлетворяющий условию,
					// перемещаемся "вниз" до тех пор, пока не наткнемся на первый
					// "неудовлетворительный" элемент: мы у цели.
					//
					for(uint j = i; j > 0;) {
						p -= isize;
						// @v9.6.1 if(fcmp(((const char *)_USERPTR_C(p))+ofs, key, pExtraData) == 0)
						if(*(long *)(p+ofs) == test) // @v9.6.1
							i = --j;
						else
							j = 0; // exit loop
					}
					break;
				}
			}
		}
		else {
			while(lo <= up) {
				const uint8 * p = p_org + (i = (lo + up) >> 1) * isize;
				cmp = fcmp(((const char *)_USERPTR_C(p))+ofs, key, pExtraData);
				if(cmp < 0)
					lo = i + 1;
				else if(cmp && i)
					up = i - 1;
				else {
					//
					// На случай, если мы попали не на самый первый элемент, удовлетворяющий условию,
					// перемещаемся "вниз" до тех пор, пока не наткнемся на первый
					// "неудовлетворительный" элемент: мы у цели.
					//
					for(uint j = i; j > 0;) {
						p -= isize;
						if(fcmp(((const char *)_USERPTR_C(p))+ofs, key, pExtraData) == 0)
							i = --j;
						else
							j = 0; // exit loop
					}
					break;
				}
			}
		}
	}
	ASSIGN_PTR(pIdx, i);
	return cmp;
}

int SLAPI SArray::bsearch(const void * key, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	return (imp_bsearch(key, pIdx, fcmp, ofs, pExtraData) == 0) ? 1 : (SLibError = SLERR_ARYITEMNFOUND, 0);
}

int SLAPI SArray::bsearch(const void * key, uint * pIdx, CompFunc fcmp) const
{
	return (imp_bsearch(key, pIdx, fcmp, 0, 0) == 0) ? 1 : (SLibError = SLERR_ARYITEMNFOUND, 0);
}

int SLAPI SArray::isItemEq(const void * pKey, uint pos, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	if(pos < count) {
		const void * p_data = _USERPTR(data + pos * isize);
		if(p_data && fcmp(PTR8(p_data) + ofs, pKey, pExtraData) == 0)
			return 1;
	}
	return 0;
}

int SLAPI SArray::lsearch(const void * pcKey, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	uint   i = pIdx ? (*pIdx * isize) : 0;
	const  uint    lim = count * isize;
	const  uint8 * p_org = PTR8(data)/* + ofs*/;
	if(flags & aryPtrContainer) {
		for(; i < lim; i += sizeof(void *))
			if(fcmp(PTR8(*(void **)(p_org + i)) + ofs, pcKey, pExtraData) == 0) {
				ASSIGN_PTR(pIdx, i / sizeof(void *));
				return 1;
			}
	}
	else {
		p_org += ofs;
		if(fcmp == CMPF_LONG) {
			//
			// Так как функция сравнения 4-байтовых значений очень популярна,
			// применяем для такого случая специальную обработку (без вызова, собственно, функции сравнения).
			//
			const uint32 test = *PTR32(pcKey);
			for(; i < lim; i += isize)
				if(*PTR32(p_org + i) == test) {
					ASSIGN_PTR(pIdx, i / isize);
					return 1;
				}
		}
		else {
			for(; i < lim; i += isize)
				if(fcmp(p_org + i, pcKey, pExtraData) == 0) {
					ASSIGN_PTR(pIdx, i / isize);
					return 1;
				}
		}
	}
#if 0 // old algorithm (proof) {
	const  uint    lim = count * isize;
	const  uint8 * p_org = PTR8(data)/* + ofs*/;
	for(uint i = (pIdx ? (*pIdx * isize) : 0); i < lim; i += isize)
		if(fcmp(((const char *)_USERPTR(p_org + i)) + ofs, pcKey, extraData) == 0) {
			ASSIGN_PTR(pIdx, i / isize);
			return 1;
		}
#endif // } 0
	return ((SLibError = SLERR_ARYITEMNFOUND), 0);
}

int SLAPI SArray::ordInsert(const void * item, uint * idx, CompFunc fcmp, void * pExtraData /*=0*/)
{
	int    ok = 1;
	uint   pos = 0;
	int    cmp;
	if(count) {
		if(!(flags & arySorted))
			sort(fcmp, pExtraData);
		if((cmp = imp_bsearch(item, &pos, fcmp, 0, pExtraData)) == 0)
			if(flags & aryUnique) {
				ASSIGN_PTR(idx, pos);
				ok = (SLibError = SLERR_ARYDUPITEM, 0);
			}
			else
				pos++;
		else if(cmp < 0)
			pos++;
	}
	if(ok) {
		if(atInsert(pos, item)) {
			ASSIGN_PTR(idx, pos);
			flags |= arySorted;
		}
		else
			ok = 0;
	}
	return ok;
}

int SLAPI SArray::swap(uint p1, uint p2)
{
	int    ok = 1;
	if(p1 != p2) {
		const uint c = getCount();
		if(p1 < c && p2 < c) {
			void * ptr1 = (data + p1 * isize);
			void * ptr2 = (data + p2 * isize);
			memswap(ptr1, ptr2, isize);
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

int SLAPI SArray::reverse(uint pos, uint numItems)
{
	const  uint last_pos = pos+numItems-1;
	uint   i = numItems/2;
	if(i && pos < count && last_pos < count)
		do {
			--i;
			memswap(at(pos+i), at(last_pos-i), isize);
		} while(i);
	return 1;
}

int SLAPI SArray::moveItem(uint pos, int dir, uint * pNewPos)
{
	int    ok = 0;
	uint   p2 = UINT_MAX;
	if(dir == 0) {
		if(checkirange(pos, 0, getCount()-2))
			p2 = pos + 1;
	}
	else {
		if(checkirange(pos, 1, getCount()-1))
			p2 = pos - 1;
	}
	if(p2 != UINT_MAX) {
		swap(pos, p2);
		ASSIGN_PTR(pNewPos, p2);
		ok = 1;
	}
	return ok;
}

int SLAPI SArray::shuffle()
{
	int    ok = 1;
	const uint cnt = getCount();
	uint   c = cnt;
	if(c) {
		SRandGenerator & r_rg = SLS.GetTLA().Rg;
		do {
			const uint p = r_rg.GetUniformInt(--c);
			if(p < cnt && p != c)
				memswap(at(p), at(c), isize);
		} while(c);
	}
	return ok;
}

int FASTCALL SArray::atFree(uint idx)
{
	// @v7.0.11 assert(idx < count); // @v7.0.10
	if(idx < count) {
		uint8 * org = data;
		void * item = org + idx * isize;
		if(flags & aryEachItem)
			freeItem(_USERPTR(item));
		count--;
		if(idx < count)
			memmove(item, PTR8(item) + isize, (count-idx) * isize);
		shrink();
		return 1;
	}
	else
		return (SLibError = SLERR_BOUNDS, 0);
}

int SLAPI SArray::freeChunk(uint lowPos, uint uppPos)
{
	int    ok = 0;
	const  uint org_count = count;
	if(lowPos <= uppPos && lowPos < org_count) {
		const uint upp_pos = MIN(uppPos, org_count-1);
		const uint _fc = (upp_pos - lowPos + 1);
		assert(_fc <= org_count);
		if(_fc == org_count) {
			freeAll();
			ok = 1;
		}
		else {
			uint8 * p_org = data;
			void * p_low_item = p_org + lowPos * isize;
			if(flags & aryEachItem) {
				for(uint i = lowPos; i <= upp_pos; i++) {
					void * p_item = p_org + i * isize;
					freeItem(_USERPTR(p_item));
				}
			}
			count -= _fc;
			if(upp_pos < (org_count-1)) {
				memmove(p_low_item, PTR8(p_low_item) + _fc * isize, (org_count-1-upp_pos) * isize);
			}
			shrink();
			ok = 1;
		}
	}
	else
		SLibError = SLERR_BOUNDS;
	return ok;
}

int SLAPI SArray::freeLast()
{
	return count ? atFree(count-1) : (SLibError = SLERR_BOUNDS, 0);
}

void SLAPI SArray::clear()
{
	if(data && flags & aryEachItem) {
		uint8 * p_org = data;
		uint   lim = count * isize;
		for(uint i = 0; i < lim; i += isize)
			freeItem(_USERPTR(p_org + i));
	}
	count = pointer = 0;
}

void SLAPI SArray::freeAll()
{
	if(data) {
		uint8 * p_org = data;
		uint   lim = count * isize;
		if(flags & aryEachItem) {
			for(uint i = 0; i < lim; i += isize)
				freeItem(_USERPTR(p_org + i));
		}
		ZFREE(data);
	}
	count = 0;
	pointer = 0;
	Limit = 0;
}

int FASTCALL SArray::enumItems(uint * pIdx, void ** ppItem) const
{
	uint   i = *pIdx;
	if(i < count) {
		*ppItem = _USERPTR(data + i * isize);
		*pIdx  = i+1;
		return 1;
	}
	else
		return 0;
}
//
// @construction
//
double SLAPI SArray::calcSumDouble(size_t offs) const
{
	const  int8 * ptr = (const int8 *)data;
	const  uint c4 = count / 4;
	uint   j = count;
	double sum = 0;
	for(uint i = 0; i < c4; i++) {
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize;
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize;
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize;
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize;
	}
	j -= c4 * 4;
	if(j) {
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize; --j;
	}
	if(j) {
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize; --j;
	}
	if(j) {
		sum += *(double *)(((uint8 *)_USERPTR_C(ptr)) + offs); ptr += isize; --j;
	}
	return sum;
}

#ifndef _WIN32_WCE // {

int SLAPI SArray::Write(SFile & rFile, long) const
{
	int    ok = 1;
	uint   i = 0;
	uint32 c = count;
	uint32 item_size = isize;
	long   beg_pos = -1;
	THROW(rFile.IsValid());
	beg_pos = rFile.Tell();
	THROW(rFile.Write(&item_size, sizeof(item_size)));
	THROW(rFile.Write(&c, sizeof(c)));
	for(i = 0; i < c; i++) {
		THROW(rFile.Write(at(i), item_size));
	}
	CATCH
		if(beg_pos >= 0)
			rFile.Seek(beg_pos);
		ok = 0;
	ENDCATCH
	return ok;
}

int SLAPI SArray::Read(SFile & rFile, long)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	uint32 c = 0;
	uint32 item_size = 0;
	long   beg_pos = -1;
	clear();
	THROW(rFile.IsValid());
	beg_pos = rFile.Tell();
	THROW(rFile.Read(&item_size, sizeof(item_size)));
	if(item_size != isize) {
		SString msg_buf;
		msg_buf.Cat(item_size).Space().Cat("!=").Space().Cat(isize);
		SLS.SetAddedMsgString(msg_buf);
		CALLEXCEPTV(SLERR_UNMATCHSTREAMARRAY);
	}
	THROW(rFile.Read(&c, sizeof(c)));
	{
		STempBuffer temp_buf(item_size);
		for(uint i = 0; i < c; i++) {
			THROW(rFile.Read((char *)temp_buf, item_size));
			THROW(insert(temp_buf));
		}
	}
	CATCH
		if(beg_pos >= 0)
			rFile.Seek(beg_pos);
		ok = 0;
	ENDCATCH
	return ok;
}
#endif // } _WIN32_WCE
//
//
//
SLAPI SCollection::SCollection(uint aDelta, int o) : SArray(sizeof(void *), aDelta, o)
{
}
//
//
//
int FASTCALL LAssoc::operator == (const LAssoc & s) const
{
	return (Key == s.Key && Val == s.Val) ? 1 : 0;
}

SLAPI LAssocArray::LAssocArray() : TSArray <LAssoc>()
{
}

SLAPI LAssocArray::LAssocArray(const LAssocArray & s) : TSArray <LAssoc> (s)
{
}

LAssocArray & FASTCALL LAssocArray::operator = (const LAssocArray & s)
{
	copy(s);
	return *this;
}

int FASTCALL LAssocArray::operator == (const LAssocArray & s) const
{
	if(count != s.count)
		return 0;
	for(uint i = 0; i < count; i++)
		if(!(at(i) == s.at(i)))
			return 0;
	return 1;
}

int SLAPI LAssocArray::CheckUnique(long key, int binary) const
{
	return Search(key, 0, 0, binary) ? 0 : 1;
}

long FASTCALL LAssocArray::Count(long key) const
{
	long   c = 0;
	for(uint i = 0; i < getCount(); i++)
		if(at(i).Key == key)
			c++;
	return c;
}

int SLAPI LAssocArray::GetListByKey(long key, LongArray & rList) const
{
	int    ok = -1;
	const  uint _c = getCount();
	for(uint i = 0; i < _c; i++) {
		if(at(i).Key == key) {
			rList.addUnique(at(i).Val);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI LAssocArray::GetListByVal(long val, LongArray & rList) const
{
	int    ok = -1;
	for(uint i = 0; i < count; i++) {
		if(at(i).Val == val) {
			rList.addUnique(at(i).Key);
			ok = 1;
		}
	}
	return ok;
}

int SLAPI LAssocArray::Add(long key, long val, uint * pPos, int inOrder)
{
	int    ok = 1;
	uint   pos = 0;
	LAssoc item;
	item.Key = key;
	item.Val = val;
	if(inOrder)
		ok = ordInsert(&item, &pos, CMPF_LONG) ? 1 : 0;
	else {
		ok = insert(&item) ? 1 : 0;
		pos = getCount()-1;
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI LAssocArray::AddUnique(long key, long val, uint * pPos, int inOrder)
{
	return Search(key, 0, pPos, inOrder) ? 0 : Add(key, val, pPos, inOrder);
}

int SLAPI LAssocArray::Update(long key, long val, int binary)
{
	uint   pos = 0;
	if(Search(key, 0, &pos, binary)) {
		at(pos).Val = val;
		return 1;
	}
	else
		return Add(key, val, &pos, binary);
}

int SLAPI LAssocArray::Remove(long key, int binary)
{
	uint   pos = 0;
	if(Search(key, 0, &pos, binary)) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

int SLAPI LAssocArray::Search(long key, long * pVal, uint * pPos, int binary) const
{
	uint   pos = 0;
	int    ok = binary ? bsearch(&key, &pos, CMPF_LONG) : lsearch(&key, &pos, CMPF_LONG);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int SLAPI LAssocArray::SearchByVal(long val, long * pKey, uint * pPos) const
{
	uint   pos = 0;
	int    ok  = lsearch(&val, &pos, CMPF_LONG, sizeof(long));
	if(ok) {
		ASSIGN_PTR(pKey, at(pos).Key);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int SLAPI LAssocArray::SearchPair(long key, long val, uint * pPos) const
{
	LAssoc k;
	k.Key = key;
	k.Val = val;
	return lsearch(&k, pPos, PTR_CMPFUNC(_2long));
}

int SLAPI LAssocArray::Sort()
{
	return SArray::sort(CMPF_LONG);
}

int SLAPI LAssocArray::CheckFlag(long key, long mask, int binary) const
{
	long   val = 0;
	return BIN(Search(key, &val, 0, binary) && (val & mask));
}
//
//
//
SLAPI RAssocArray::RAssocArray() : TSArray <RAssoc>()
{
}

SLAPI RAssocArray::RAssocArray(const RAssocArray & s) : TSArray <RAssoc> (s)
{
}

RAssocArray & FASTCALL RAssocArray::operator = (const RAssocArray & s)
{
	copy(s);
	return *this;
}

int SLAPI RAssocArray::Search(long key, double * pVal, uint * pPos, int binary) const
{
	uint   pos = 0;
	int    ok = binary ? bsearch(&key, &pos, CMPF_LONG) : lsearch(&key, &pos, CMPF_LONG);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

double SLAPI RAssocArray::Get(long key, int binary) const
{
	double val = 0.0;
	return (Search(key, &val, 0, binary) > 0) ? val : 0;
}

int FASTCALL RAssocArray::Has(long key) const
{
	long   _key = key;
	return lsearch(&_key, 0, CMPF_LONG);
}

int SLAPI RAssocArray::GetList(LongArray & rList, int unique) const
{
	int    ok = -1;
	for(uint i = 0; i < getCount(); i++) {
		if(unique) {
			rList.addUnique(at(i).Key);
		}
		else
			rList.add(at(i).Key);
		ok = 1;
	}
	return ok;
}

#define OPTIMIZE_LOOP

int SLAPI RAssocArray::Scale(double rate)
{
#ifdef OPTIMIZE_LOOP
	uint   i = getCount();
	if(i) do { at(--i).Val *= rate; } while(i);
#else
	for(uint i = 0; i < getCount(); i++)
		at(i).Val *= rate;
#endif
	return 1;
}

int SLAPI SLAPI RAssocArray::Distribute(double amount, long flags, int prec, RAssocArray & rResult) const
{
	rResult.clear();
	int    ok = 1;
	const uint c = getCount();
	if(c == 1) {
		const double temp = (flags & dfRound) ? round(amount, prec) : amount;
		rResult.Add(at(0).Key, temp, 0, 0);
	}
	else if(c > 1) {
		const double total = GetTotal();
		if(total != 0.0) {
			double rest = amount;
			for(uint i = 0; i < c; i++) {
				if(flags & dfReset && i == (c-1)) {
					const double temp = (flags & dfRound) ? round(rest, prec) : rest;
					rResult.Add(at(i).Key, temp, 0, 0);
				}
				else {
					double temp = amount * at(i).Val / total;
					temp = (flags & dfRound) ? round(temp, prec) : temp;
					rResult.Add(at(i).Key, temp, 0, 0);
					rest -= temp;
				}
			}
		}
		else
			ok = -1;
	}
	else
		ok = -1;
	return ok;
}

double SLAPI RAssocArray::GetTotal() const
{
	double total = 0.0;
#ifdef OPTIMIZE_LOOP
	uint   i = getCount();
	if(i) do { total += at(--i).Val; } while(i);
#else
	for(uint i = 0; i < getCount(); i++)
		total += at(i).Val;
#endif
	return total;
}

int SLAPI RAssocArray::Add(long key, double val, int additive, int binary)
{
	int    ok = 0;
	uint   pos = 0;
	if(additive && Search(key, 0, &pos, binary) > 0) {
		#ifdef _DEBUG
		RAssoc & r_item = at(pos);
		r_item.Val += val;
		#else
		at(pos).Val += val;
		#endif
		ok = 2;
	}
	else {
		RAssoc item;
		item.Key = key;
		item.Val = val;
		ok = binary ? ordInsert(&item, 0, CMPF_LONG) : insert(&item);
	}
	return ok;
}

int SLAPI RAssocArray::Add(const RAssocArray & rList, int additive, int binary)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		const RAssoc & r_item = rList.at(i);
		Add(r_item.Key, r_item.Val, additive, binary);
	}
	return 1;
}

int SLAPI RAssocArray::Remove(long key, int binary)
{
	uint   pos = 0;
	if(Search(key, 0, &pos, binary)) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

int SLAPI RAssocArray::SortByKey()
{
	return sort(CMPF_LONG);
}

IMPL_CMPFUNC(RAssoc_Val, i1, i2)
{
	const RAssoc * p1 = (const RAssoc *)i1;
	const RAssoc * p2 = (const RAssoc *)i2;
	return (p1->Val < p2->Val) ? -1 : ((p1->Val > p2->Val) ? +1 : 0);
}

IMPL_CMPFUNC(RAssoc_Val_Rev, i1, i2)
{
	const RAssoc * p1 = (const RAssoc *)i1;
	const RAssoc * p2 = (const RAssoc *)i2;
	return (p1->Val < p2->Val) ? +1 : ((p1->Val > p2->Val) ? -1 : 0);
}

int SLAPI RAssocArray::SortByVal()
{
	return sort(PTR_CMPFUNC(RAssoc_Val));
}

int SLAPI RAssocArray::SortByValRev()
{
	return sort(PTR_CMPFUNC(RAssoc_Val_Rev));
}
//
//
//
SLAPI RPairArray::RPairArray() : TSArray <RPair> ()
{
}

SLAPI RPairArray::RPairArray(const RPairArray & rS)  : TSArray <RPair> (rS)
{
}

RPairArray & FASTCALL RPairArray::operator = (const RPairArray & rS)
{
	SArray::copy(rS);
	return *this;
}

RPair & FASTCALL RPairArray::operator [](size_t i) const
{
	return at(i);
}

IMPL_CMPFUNC(RPairX, i1, i2)
{
	int    si;
	CMPCASCADE2(si, (RPair *)i1, (RPair *)i2, X, Y);
	return si;
}

IMPL_CMPFUNC(RPairY, i1, i2)
{
	int    si;
	CMPCASCADE2(si, (RPair *)i1, (RPair *)i2, Y, X);
	return si;
}

int SLAPI RPairArray::SortByX()
{
	return sort(PTR_CMPFUNC(RPairX));
}

int SLAPI RPairArray::SortByY()
{
	return sort(PTR_CMPFUNC(RPairY));
}

#ifndef _WIN32_WCE // {
//
//
//
SLAPI UUIDAssocArray::UUIDAssocArray() : TSArray <UUIDAssoc>()
{
}

int SLAPI UUIDAssocArray::Add(long key, const S_GUID & rVal, uint * pPos)
{
	UUIDAssoc item;
	item.Key = key;
	item.Val = rVal;
	if(insert(&item)) {
		ASSIGN_PTR(pPos, getCount()-1);
		return 1;
	}
	else
		return 0;
}

int SLAPI UUIDAssocArray::Search(long key, S_GUID * pVal, uint * pPos) const
{
	uint   pos = 0;
	if(lsearch(&key, &pos, CMPF_LONG)) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
		return 1;
	}
	else
		return 0;
}

int SLAPI UUIDAssocArray::SearchVal(const S_GUID & rVal, long * pKey, uint * pPos) const
{
	uint   pos = 0;
	for(uint i = 0; i < getCount(); i++) {
		const UUIDAssoc & r_item = at(i);
		if(r_item.Val == rVal) {
			ASSIGN_PTR(pKey, r_item.Key);
			ASSIGN_PTR(pPos, i);
			return 1;
		}
	}
	return 0;
}

#endif // } _WIN32_WCE
//
//
//
SLAPI TaggedStringArray::TaggedStringArray() : TSArray <TaggedString>()
{
}

TaggedStringArray & FASTCALL TaggedStringArray::operator = (const TaggedStringArray & rS)
{
	copy(rS);
	return *this;
}

int SLAPI TaggedStringArray::Search(long id, uint * pPos, int binary) const
{
	uint   pos = 0;
	int    ok = binary ? bsearch(&id, &pos, CMPF_LONG) : lsearch(&id, &pos, CMPF_LONG);
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int SLAPI TaggedStringArray::SearchByText(const char * pTxt, uint * pPos) const
{
	if(pTxt) {
		TaggedString * p_item;
		for(uint i = 0; enumItems(&i, (void **)&p_item);)
			if(stricmp866(p_item->Txt, pTxt) == 0) {
				ASSIGN_PTR(pPos, i-1);
				return 1;
			}
	}
	return 0;
}

int SLAPI TaggedStringArray::Get(long id, SString & rBuf, int binary) const
{
	uint   pos = 0;
	int    ok = Search(id, &pos, binary);
	if(ok > 0)
		rBuf = at(pos).Txt;
	else
		rBuf = 0;
	return ok;
}

int SLAPI TaggedStringArray::Get(long id, char * pBuf, size_t bufLen, int binary) const
{
	SString temp_buf;
	int    r = Get(id, temp_buf, binary);
	temp_buf.CopyTo(pBuf, bufLen);
	return r;
}

int SLAPI TaggedStringArray::Add(long id, const char * pStr)
{
	TaggedString entry;
	entry.Id = id;
	STRNSCPY(entry.Txt, pStr);
	return insert(&entry);
}

int SLAPI TaggedStringArray::SortByID()
{
	return SArray::sort(CMPF_LONG);
}

IMPL_CMPFUNC(TaggedStringByName, i1, i2)
{
	return stricmp866(((char*)i1)+sizeof(long), ((char*)i2)+sizeof(long));
}

int SLAPI TaggedStringArray::SortByText()
{
	return SArray::sort(PTR_CMPFUNC(TaggedStringByName));
}
//
//
//
int SV_Uint32::Init()
{
	P_Data = 0;
	return 1;
}

void SV_Uint32::Destroy()
{
	ZFREE(P_Data);
}

int FASTCALL SV_Uint32::Copy(const SV_Uint32 & s)
{
	Init();
	uint c = s.GetCount();
	if(c) {
		P_Data = (uint32 *)calloc((c+1), sizeof(uint32));
		if(P_Data == 0)
			return (SLibError = SLERR_NOMEM, 0);
		else
			memcpy(P_Data, s.P_Data, (c+1) * sizeof(uint32));
	}
	return 1;
}

int FASTCALL SV_Uint32::IsEqual(const SV_Uint32 & rPat) const
{
	int    ok = 0;
	if(P_Data == 0 && rPat.P_Data == 0)
		ok = 1;
	else if(P_Data && rPat.P_Data) {
		uint   c = P_Data[0];
		if(c == rPat.P_Data[0])
			ok = memcmp(P_Data+1, rPat.P_Data+1, sizeof(uint32) * c) ? 0 : 1;
	}
	return ok;
}

int SV_Uint32::Search(uint val, uint * pPos) const
{
	uint   c = P_Data ? P_Data[0] : 0;
	for(uint i = 1; i <= c; i++)
		if(P_Data[i] == val) {
			ASSIGN_PTR(pPos, i-1);
			return 1;
		}
	return 0;
}

uint SV_Uint32::GetCount() const
{
	return P_Data ? P_Data[0] : 0;
}

uint32 FASTCALL SV_Uint32::Get(uint idx) const
{
	return (idx < GetCount()) ? P_Data[idx+1] : 0;
}

int FASTCALL SV_Uint32::Add(uint32 item)
{
	uint   c = GetCount();
	P_Data = (uint32 *)realloc(P_Data, (c+2) * sizeof(uint32));
	if(P_Data) {
		++c;
		P_Data[c] = item;
		P_Data[0] = c;
		return 1;
	}
	else
		return (SLibError = SLERR_NOMEM, 0);
}

int SV_Uint32::Add(uint count, const uint32 * pSrc)
{
	uint   c = GetCount();
	P_Data = (uint32 *)realloc(P_Data, (c+count+1) * sizeof(uint32));
	if(P_Data) {
		memcpy(P_Data+c+1, pSrc, count * sizeof(uint32));
		P_Data[0] = c+count;
		return 1;
	}
	else
		return (SLibError = SLERR_NOMEM, 0);
}

int FASTCALL SV_Uint32::Write(SBuffer & rBuf) const
{
	uint32 c = GetCount();
	rBuf.Write(&c, sizeof(c));
	if(c)
		rBuf.Write(P_Data+1, sizeof(uint32) * c);
	return 1;
}

int FASTCALL SV_Uint32::Read(SBuffer & rBuf)
{
	uint32 c = 0;
	rBuf.Read(&c, sizeof(c));
	if(c) {
		P_Data = (uint32 *)realloc(P_Data, (c+1) * sizeof(uint32));
		if(P_Data) {
			P_Data[0] = c;
			rBuf.Read(P_Data+1, sizeof(uint32) * c);
			return 1;
		}
		else
			return (SLibError = SLERR_NOMEM, 0);
	}
	else
		Destroy();
	return 1;
}
//
//
//
SLAPI StrAssocArray::StrAssocArray() : SStrGroup()
{
	TextCmpProc = 0; // @v9.5.9
}

SLAPI StrAssocArray::StrAssocArray(const StrAssocArray & rS) : SStrGroup()
{
	Copy(rS);
}

void FASTCALL StrAssocArray::SetTextCmpProc(CompFunc fcmp)
{
	TextCmpProc = fcmp;
}

StrAssocArray & FASTCALL StrAssocArray::operator = (const StrAssocArray & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL StrAssocArray::Copy(const StrAssocArray & rS)
{
	SStrGroup::CopyS(rS);
	TextCmpProc = rS.TextCmpProc; // @v9.5.9
	Assoc = rS.Assoc;
	ParentList = rS.ParentList;
	return 1;
}

int FASTCALL StrAssocArray::IsEqual(const StrAssocArray & s) const
{
	int    ok = 0;
	uint   c = getCount();
	if(c == s.getCount()) {
		ok = 1;
		for(uint i = 0; ok && i < c; i++) {
			Item b = at(i);
			Item sb = s.at(i);
			if(b.Id == sb.Id && b.ParentId == sb.ParentId) {
				if(b.Txt != 0 && sb.Txt != 0) {
					if(strcmp(b.Txt, sb.Txt) != 0)
						ok = 0;
				}
				else if(b.Txt || sb.Txt)
					ok = 0;
			}
		}
	}
	return ok;
}

int FASTCALL StrAssocArray::HasChild(long id) const
{
	return (id == 0) ? (Assoc.getCount() ? 1 : 0) : ParentList.SearchByVal(id, 0, 0);
}

int SLAPI StrAssocArray::GetParent(long id, long * pParentId) const
{
	long   par_id = 0;
	int    ok = ParentList.Search(id, &par_id, 0) ? 1 : -1;
	ASSIGN_PTR(pParentId, par_id);
	return ok;
}

int SLAPI StrAssocArray::GetListByParent(long parentId, int recursive, LongArray & rList) const
{
	int    ok = 1;
	LongArray temp_list;
	if(parentId) {
		const uint c = Assoc.getCount();
		for(uint i = 0; i < c; i++) {
			long   id = Assoc.at(i).Key;
			if(ParentList.SearchPair(id, parentId, 0) > 0)
				temp_list.addUnique(id);
		}
	}
	else {
		const uint c = Assoc.getCount();
		for(uint i = 0; i < c; i++) {
			long   id = Assoc.at(i).Key;
			long   p = 0;
			if(!ParentList.Search(id, &p, 0) || p == 0)
				temp_list.addUnique(id);
		}
	}
	if(recursive) {
		const uint c = temp_list.getCount();
		LongArray temp_list2;
		for(uint i = 0; i < c; i++) {
			GetListByParent(temp_list.get(i), recursive, temp_list2); // @recursion
		}
		temp_list.addUnique(&temp_list2);
	}
	rList.addUnique(&temp_list);
	return ok;
}

int SLAPI StrAssocArray::AddFast(long id, const char * pStr)
{
	LAssoc assc;
	assc.Key = id;
	if(!isempty(pStr)) {
		uint   str_pos = 0;
		AddS(pStr, &str_pos);
		assc.Val = str_pos;
	}
	else
		assc.Val = 0;
	return BIN(Assoc.insert(&assc));
}

int SLAPI StrAssocArray::AddFast(long id, long parentId, const char * pStr)
{
	LAssoc assc;
	assc.Key = id;
	if(!isempty(pStr)) {
		uint   str_pos = 0;
		AddS(pStr, &str_pos);
		assc.Val = str_pos;
	}
	else
		assc.Val = 0;
	int    ok = BIN(Assoc.insert(&assc));
	if(ok && parentId)
		ok = ParentList.Update(id, parentId);
	return ok;
}

int SLAPI StrAssocArray::Add(long id, const char * pStr, int replaceDup)
{
	int    ok = 0;
	uint   pos = 0, str_pos = 0;
	if(replaceDup != -100 && Search(id, &pos)) {
		LAssoc & r_assc = Assoc.at(pos);
		if(replaceDup > 0) {
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				r_assc.Val = str_pos;
			}
			else
				r_assc.Val = 0;
			ok = 2;
		}
		else if(replaceDup < 0) {
			LAssoc assc;
			assc.Key = id;
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				assc.Val = str_pos;
			}
			else
				assc.Val = 0;
			ok = Assoc.insert(&assc) ? 3 : 0;
		}
		else
			ok = -1;
	}
	else {
		LAssoc assc;
		assc.Key = id;
		if(!isempty(pStr)) {
			AddS(pStr, &str_pos);
			assc.Val = str_pos;
		}
		else
			assc.Val = 0;
		ok = BIN(Assoc.insert(&assc));
	}
	return ok;
}

int SLAPI StrAssocArray::Add(long id, long parentId, const char * pStr, int replaceDup /*= 1*/)
{
	int    ok = 0;
	uint   pos = 0, str_pos = 0;
	if(replaceDup != -100 && Search(id, &pos)) {
		LAssoc & r_assc = Assoc.at(pos);
		if(replaceDup > 0) {
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				r_assc.Val = str_pos;
			}
			else
				r_assc.Val = 0;
			if(parentId)
				ParentList.Update(id, parentId);
			else if(ParentList.getCount())
				ParentList.Remove(id);
			ok = 2;
		}
		else if(replaceDup < 0) {
			LAssoc assc;
			assc.Key = id;
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				assc.Val = str_pos;
			}
			else
				assc.Val = 0;
			ok = Assoc.insert(&assc) ? 3 : 0;
			// @todo Вставить родительский идентификатор (logic)
		}
		else
			ok = -1;
	}
	else {
		LAssoc assc;
		assc.Key = id;
		if(!isempty(pStr)) {
			AddS(pStr, &str_pos);
			assc.Val = str_pos;
		}
		else
			assc.Val = 0;
		ok = BIN(Assoc.insert(&assc));
		if(ok && parentId)
			ok = ParentList.Update(id, parentId);
	}
	return ok;
}

int SLAPI StrAssocArray::UpdateByPos(uint pos, long newId)
{
	if(pos < Assoc.getCount()) {
		Assoc.at(pos).Key = newId;
		return 1;
	}
	else
		return 0;
}

int SLAPI StrAssocArray::Swap(uint p1, uint p2)
{
	return Assoc.swap(p1, p2);
}

int SLAPI StrAssocArray::Remove(long id)
{
	int    ok = Assoc.Remove(id) ? 1 : -1;
	ParentList.Remove(id);
	return ok;
}

StrAssocArray & SLAPI StrAssocArray::Clear()
{
	Assoc.freeAll();
	ParentList.freeAll();
	ClearS();
	return *this;
}

int SLAPI StrAssocArray::ClearParents()
{
	ParentList.freeAll();
	return 1;
}

int SLAPI StrAssocArray::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		if(p_pack_handle) {
			const uint c = Assoc.getCount();
			for(uint i = 0; ok && i < c; i++) {
				LAssoc & r_assc = Assoc.at(i);
				uint   _pos = r_assc.Val;
				Pack_Replace(p_pack_handle, _pos);
				r_assc.Val = (long)_pos;
			}
			Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

uint SLAPI StrAssocArray::getCount() const
{
	return Assoc.getCount();
}

int SLAPI StrAssocArray::Helper_RemoveRecursion(long parentId, LongArray & rRecurList, LongArray & rPassedList, LongArray * pBadList)
{
	int    ok = -1;
	LongArray list;
	GetListByParent(parentId, 0, list);
	const uint c = list.getCount();
	if(c) {
		list.sort();
		for(uint i = 0; i < c; i++) {
			long   _id = list.get(i);
			if(rRecurList.lsearch(_id)) {
				if(pBadList)
					pBadList->add(_id);
				ParentList.Remove(_id, 0);
				ok = 1;
			}
			else {
				rRecurList.add(_id);
				int r = Helper_RemoveRecursion(_id, rRecurList, rPassedList, pBadList); // @recursion
				if(ok < 0)
					ok = r;
			}
		}
	}
	rPassedList.add(parentId);
	return ok;
}

int SLAPI StrAssocArray::RemoveRecursion(LongArray * pBadList)
{
	int    ok = -1;
	LongArray recur_list;
	const uint _c = Assoc.getCount();
	if(_c) {
		LongArray passed_list;
		ok = Helper_RemoveRecursion(0, recur_list, passed_list, pBadList);
		passed_list.sortAndUndup();
		for(uint i = 0; i < _c; i++) {
			const long _id = Assoc.at(i).Key;
			if(!passed_list.bsearch(_id)) {
				recur_list.clear();
				int r = Helper_RemoveRecursion(Assoc.at(i).Key, recur_list, passed_list, pBadList);
				if(ok < 0)
					ok = r;
			}
		}
	}
	return ok;
}

uint FASTCALL StrAssocArray::GetTextPos(uint pos) const
{
	uint   tpos = 0;
	if(pos < Assoc.getCount()) {
		const LAssoc & r_assc = Assoc.at(pos);
		tpos = (uint)r_assc.Val;
		if(!tpos || tpos >= Pool.getDataLen())
			tpos = 0;
	}
	return tpos;
}

int SLAPI StrAssocArray::GetTextByPos(uint textPos, SString & rBuf) const
{
	int    ok = 1;
	if(textPos && textPos < Pool.getDataLen()) {
		rBuf = (Pool.getBuf() + textPos);
	}
	else {
		rBuf = 0;
		ok = 0;
	}
	return ok;
}

StrAssocArray::Item FASTCALL StrAssocArray::at(uint pos) const
{
	Item   item;
	item.Id = 0;
	item.ParentId = 0;
	item.Txt = 0;
	if(pos < Assoc.getCount()) {
		const LAssoc & r_assc = Assoc.at(pos);
		item.Id = r_assc.Key;
		uint   pos2 = (uint)r_assc.Val;
		if(pos2 && pos2 < Pool.getDataLen())
			item.Txt = Pool.getBuf() + pos2;
		ParentList.Search(item.Id, &item.ParentId, &(pos2 = 0));
	}
	return item;
}

StrAssocArray::Item FASTCALL StrAssocArray::at_WithoutParent(uint pos) const
{
	Item   item;
	item.Id = 0;
	item.ParentId = 0;
	item.Txt = 0;
	if(pos < Assoc.getCount()) {
		const LAssoc & r_assc = Assoc.at(pos);
		item.Id = r_assc.Key;
		uint   pos2 = (uint)r_assc.Val;
		if(pos2 && pos2 < Pool.getDataLen())
			item.Txt = Pool.getBuf() + pos2;
	}
	return item;
}

int SLAPI StrAssocArray::atFree(uint pos)
{
	if(pos < Assoc.getCount()) {
		ParentList.Remove(Assoc.at(pos).Key);
		Assoc.atFree(pos);
		return 1;
	}
	else
		return (SLibError = SLERR_BOUNDS, 0);
}

int SLAPI StrAssocArray::Search(long id, uint * pPos) const
{
	return Assoc.Search(id, 0, pPos);
}

int SLAPI StrAssocArray::SearchByText(const char * pTxt, int ignoreCase, uint * pPos) const
{
	uint   pos = 0, assc_pos;
	while(Pool.search(pTxt, &pos, ignoreCase)) {
		if(Assoc.lsearch(&pos, &(assc_pos = 0), CMPF_LONG, offsetof(LAssoc, Val))) {
			ASSIGN_PTR(pPos, assc_pos);
			return 1;
		}
		else {
			// @v8.9.12 {
			size_t _len = Pool.getLen(pos);
			uint   _dlen = Pool.getDelimLen();
			pos += (_len + _dlen);
			// } @v8.9.12
		}
	}
	return 0;
}

int SLAPI StrAssocArray::SearchByText(const char * pTxt, CompFunc fcmp, uint * pPos) const
{
	if(pTxt) {
		const  char * p_pool = Pool.getBuf();
		if(p_pool) {
			uint   last_pos = pPos ? *pPos : 0;
			for(uint i = last_pos; i < Assoc.getCount(); i++) {
				uint   str_offs = (uint)Assoc.at(i).Val;
				if(str_offs && p_pool[str_offs] && fcmp(p_pool+str_offs, pTxt, 0) == 0) {
					ASSIGN_PTR(pPos, i);
					return 1;
				}
			}
		}
	}
	/*
	Этот вариант, увы, не правильно работает из-за того, что положение строки в пуле строк
	может не совпадать (в относительном выражении) с положением элемента Assoc.
	uint   pos = 0, next_pos = 0, assc_pos;
	while(Pool.search(pTxt, fcmp, &pos, &next_pos)) {
		if(Assoc.lsearch(&pos, &(assc_pos = 0), CMPF_LONG, offsetof(LAssoc, Val)) && assc_pos >= last_pos) {
			ASSIGN_PTR(pPos, assc_pos);
			return 1;
		}
		else
			pos = next_pos;
	}
	*/
	return 0;
}

int SLAPI StrAssocArray::Get(long id, SString & rBuf) const
{
	int    ok = 0;
	long   str_pos = 0;
	uint   pos;
	rBuf = 0;
	if(Assoc.Search(id, &str_pos, &pos)) {
		GetS(str_pos, rBuf);
		ok = rBuf.NotEmpty() ? 1 : -1;
	}
	return ok;
}

int SLAPI StrAssocArray::GetMaxID(long * pId) const
{
	uint   c = Assoc.getCount();
	if(c) {
		long   _max = -MAXLONG;
		for(uint i = 0; i < c; i++) {
			long   key = Assoc.at(i).Key;
			if(key > _max)
				_max = key;
		}
		ASSIGN_PTR(pId, _max);
		return 1;
	}
	else
		return -1;
}

int SLAPI StrAssocArray::GetMaxLength(uint * pMaxLen) const
{
	uint   max_len = 0;
	uint   c = Assoc.getCount();
	if(c) {
		SString temp_buf;
		for(uint i = 0; i < c; i++) {
			long   str_pos = Assoc.at(i).Val;
			if(GetS(str_pos, temp_buf) && temp_buf.Len() > max_len)
				max_len = temp_buf.Len();
		}
	}
	ASSIGN_PTR(pMaxLen, max_len);
	return max_len ? 1 : -1;
}

IMPL_CMPFUNC(StrAssoc_ByLength, i1, i2)
{
	StringSet * p_ss = (StringSet *)pExtraData;
	uint   pos1 = ((LAssoc *)i1)->Val;
	uint   pos2 = ((LAssoc *)i2)->Val;
	char   stub[8];
	stub[0] = 0;
	size_t ss_data_len = p_ss->getDataLen();
	const char * p_str1 = (pos1 < ss_data_len) ? (p_ss->getBuf() + pos1) : stub;
	const char * p_str2 = (pos2 < ss_data_len) ? (p_ss->getBuf() + pos2) : stub;
	return cmp_ulong(strlen(p_str1), strlen(p_str2));
}

IMPL_CMPFUNC(StrAssoc_ByLength_Descend, i1, i2)
{
	StringSet * p_ss = (StringSet *)pExtraData;
	uint   pos1 = ((LAssoc *)i1)->Val;
	uint   pos2 = ((LAssoc *)i2)->Val;
	char   stub[8];
	stub[0] = 0;
	size_t ss_data_len = p_ss->getDataLen();
	const char * p_str1 = (pos1 < ss_data_len) ? (p_ss->getBuf() + pos1) : stub;
	const char * p_str2 = (pos2 < ss_data_len) ? (p_ss->getBuf() + pos2) : stub;
	return cmp_ulong(strlen(p_str2), strlen(p_str1));
}

// static
int SLAPI StrAssocArray::Helper_CmpFunc_StrAssocByText(const LAssoc * p1, const LAssoc * p2, StrAssocArray * pArray, LAssocArray * pRecurTracer)
{
	long   id[2], par[2];
	id[0] = p1->Key;
	id[1] = p2->Key;
	par[0] = par[1] = 0;
	pArray->ParentList.Search(id[0], &par[0], 0, 1);
	pArray->ParentList.Search(id[1], &par[1], 0, 1);
	if(par[0] == par[1]) {
		uint   pos1 = p1->Val;
		uint   pos2 = p2->Val;
		char   stub[8];
		stub[0] = 0;
		size_t ss_data_len = pArray->Pool.getDataLen();
		const char * p_buf = pArray->Pool.getBuf();
		const char * p_str1 = (pos1 == 0 || pos1 >= ss_data_len) ? stub : (p_buf + pos1);
		const char * p_str2 = (pos2 == 0 || pos2 >= ss_data_len) ? stub : (p_buf + pos2);
		return pArray->TextCmpProc ? pArray->TextCmpProc(p_str1, p_str2, 0) : stricmp866(p_str1, p_str2);
	}
	else {
		LAssocArray recur_tracer;
		SETIFZ(pRecurTracer, &recur_tracer);
		LAssoc pi[2];
		uint p = 0;
		if(par[0] && pArray->Search(par[0], &(p = 0)))
			pi[0] = pArray->Assoc.at(p);
		else {
			pi[0].Key = par[0];
			pi[0].Val = 0;
		}
		if(par[1] && pArray->Search(par[1], &(p = 0)))
			pi[1] = pArray->Assoc.at(p);
		else {
			pi[1].Key = par[1];
			pi[1].Val = 0;
		}
		if(pRecurTracer->SearchPair(pi[0].Key, pi[1].Key, 0)) {
			// cycle
			return 0;
		}
		else {
			pRecurTracer->Add(pi[0].Key, pi[1].Key, 0);
			return StrAssocArray::Helper_CmpFunc_StrAssocByText(&pi[0], &pi[1], pArray, pRecurTracer); // @recursion
		}
	}
}

IMPL_CMPFUNC(StrAssoc_ByText, i1, i2)
{
	return StrAssocArray::Helper_CmpFunc_StrAssocByText((LAssoc *)i1, (LAssoc *)i2, (StrAssocArray *)pExtraData, 0);
}

int SLAPI Helper_CmpFunc_StrAssocByID(long id1, long id2, StrAssocArray * pArray, LAssocArray * pRecurTracer)
{
	long   par[2];
	par[0] = par[1] = 0;
	pArray->ParentList.Search(id1, &par[0], 0, 1);
	pArray->ParentList.Search(id2, &par[1], 0, 1);
	if(par[0] == par[1])
		return cmp_long(id1, id2);
	else {
		LAssocArray recur_tracer;
		SETIFZ(pRecurTracer, &recur_tracer);
		if(pRecurTracer->SearchPair(par[0], par[1], 0)) {
			// cycle
			return 0;
		}
		else {
			pRecurTracer->Add(par[0], par[1], 0);
			return Helper_CmpFunc_StrAssocByID(par[0], par[1], pArray, pRecurTracer); // @recursion
		}
	}
}

IMPL_CMPFUNC(StrAssoc_ByID, i1, i2)
{
	return Helper_CmpFunc_StrAssocByID(*(long *)i1, *(long *)i2, (StrAssocArray *)pExtraData, 0);
}

#if 0 // {

{1, 6, "X01"}
{2, 1, "X02"}
{3, 1, "X03"}
{4, 3, "X04"}
{5, 3, "X05"}
{6, 3, "X06"}

#endif // } 0

int SLAPI StrAssocArray::SortByID()
{
	if(ParentList.getCount() == 0)
		return Assoc.Sort();
	else {
		ParentList.Sort();
		return Assoc.sort(PTR_CMPFUNC(StrAssoc_ByID), this);
	}
}

int SLAPI StrAssocArray::SortByText()
{
	ParentList.Sort();
	return Assoc.sort(PTR_CMPFUNC(StrAssoc_ByText), this);
}

int SLAPI StrAssocArray::SortByLength(int descend)
{
	return Assoc.sort(descend ? PTR_CMPFUNC(StrAssoc_ByLength_Descend) : PTR_CMPFUNC(StrAssoc_ByLength), &Pool);
}

int SLAPI StrAssocArray::Reverse()
{
	return Assoc.reverse(0, Assoc.getCount());
}

int SLAPI StrAssocArray::Test_Cmp(const TaggedStringArray & rTsa, int * pNEqPos) const
{
	int    ok = 1;
	int    neq_pos = -1;
	uint   c = getCount(), i = 0;
	THROW(rTsa.getCount() == c);
	for(i = 0; i < c; i++) {
		const TaggedString & r_ts = rTsa.at(i);
		Item sa = at(i);
		neq_pos = (int)i;
		THROW(sa.Id == r_ts.Id);
		if(sa.Txt) {
			THROW(strcmp(sa.Txt, r_ts.Txt) == 0);
		}
		else {
			THROW(r_ts.Txt[0] == 0);
		}
	}
	CATCHZOK
	ASSIGN_PTR(pNEqPos, neq_pos);
	return ok;
}

int FASTCALL StrAssocArray::Write(SBuffer & rBuf, long o) const
{
	int    ok = 1;
	THROW(rBuf.Write(&Assoc, 0));
	THROW(rBuf.Write(&ParentList, 0));
	THROW(Pool.Write(rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL StrAssocArray::Read(SBuffer & rBuf, long o)
{
	int    ok = 1;
	THROW(rBuf.Read(&Assoc, 0));
	THROW(rBuf.Read(&ParentList, 0));
	THROW(Pool.Read(rBuf));
	CATCHZOK
	return ok;
}

#ifndef _WIN32_WCE // {

int SLAPI StrAssocArray::Write(SFile & rFile, long o) const
{
	int    ok = 1;
	THROW(Assoc.Write(rFile, o));
	THROW(ParentList.Write(rFile, o));
	THROW(Pool.Write(rFile, o));
	CATCHZOK
	return ok;
}

int SLAPI StrAssocArray::Read(SFile & rFile, long o)
{
	int    ok = 1;
	THROW(Assoc.Read(rFile, o));
	THROW(ParentList.Read(rFile, o));
	THROW(Pool.Read(rFile, o));
	CATCHZOK
	return ok;
}

#endif // } _WIN32_WCE
//
//
//
SLAPI StrStrAssocArray::StrStrAssocArray() : SStrGroup()
{
}

StrStrAssocArray & FASTCALL StrStrAssocArray::operator = (const StrStrAssocArray & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL StrStrAssocArray::Copy(const StrStrAssocArray & rS)
{
	L = rS.L;
	SStrGroup::CopyS(rS);
	return 1;
}

int FASTCALL StrStrAssocArray::IsEqual(const StrStrAssocArray & src) const
{
	return 0;
}

uint SLAPI StrStrAssocArray::getCount() const
{
	return L.getCount();
}

StrStrAssocArray::Item FASTCALL StrStrAssocArray::at(uint pos) const
{
	Item   item;
	MEMSZERO(item);
	if(pos < L.getCount()) {
		const  LAssoc & r_assc = L.at(pos);
		uint   key_pos = (uint)r_assc.Key;
		if(key_pos && key_pos < Pool.getDataLen())
			item.Key = Pool.getBuf() + key_pos;
		uint   val_pos = (uint)r_assc.Val;
		if(val_pos && val_pos < Pool.getDataLen())
			item.Val = Pool.getBuf() + val_pos;
	}
	return item;
}

int SLAPI StrStrAssocArray::Search(const char * pKey, SString * pVal, uint * pPos) const
{
	int    ok = 0;
	if(!isempty(pKey)) {
		SString pattern_buf = pKey;
		SString temp_buf;
		for(uint i = 0; !ok && i < L.getCount(); i++) {
			const LAssoc & r_item = L.at(i);
			GetS(r_item.Key, temp_buf);
			if(pattern_buf.Cmp(temp_buf, 0) == 0) {
				ASSIGN_PTR(pPos, i);
				if(pVal) {
					GetS(r_item.Val, *pVal);
				}
				ok = 1;
			}
		}
	}
	return ok;
}

int SLAPI StrStrAssocArray::Add(const char * pKey, const char * pVal, int replaceDup)
{
	int    ok = 1;
	int    done = 0;
	if(replaceDup) {
		uint pos = 0;
		SString val;
		if(Search(pKey, &val, &pos)) {
			uint   sp = 0;
			AddS(pVal, &sp);
			L.at(pos).Val = (long)sp;
			done = 1;
		}
	}
	if(!done) {
		uint   key_pos = 0;
		uint   val_pos = 0;
		AddS(pKey, &key_pos);
		AddS(pVal, &val_pos);
		LAssoc item;
		item.Key = (long)key_pos;
		item.Val = (long)val_pos;
		L.insert(&item);
	}
	return ok;
}

int SLAPI StrStrAssocArray::Remove(const char * pKey)
{
	int    ok = 1;
	uint   pos = 0;
	if(Search(pKey, 0, &pos)) {
		L.atFree(pos);
		ok = 1;
	}
	else
		ok = -1;
	return ok;
}

StrStrAssocArray & SLAPI StrStrAssocArray::Clear()
{
	L.clear();
	SStrGroup::ClearS();
	return *this;
}
//
//
//
SLAPI RealArray::RealArray() : TSArray <double> ()
{
}

SLAPI RealArray::RealArray(const RealArray & s) : TSArray <double> (s)
{
}

RealArray & FASTCALL RealArray::operator = (const RealArray & s)
{
	copy(s);
	return *this;
}

double & FASTCALL RealArray::operator [] (size_t i) const
{
	return at(i);
}

int FASTCALL RealArray::dim(uint numItems)
{
    clear();
	return insertChunk(numItems, 0);
}

double SLAPI RealArray::Sum() const
{
	double sum = 0.0;
	for(uint i = 0; i < count; i++) {
		sum += at(i);
	}
	return sum;
}

int SLAPI RealArray::Sort()
{
	return sort(PTR_CMPFUNC(double));
}
//
// LongArray
//
SLAPI LongArray::LongArray() : TSArray <long>()
{
}

SLAPI LongArray::LongArray(const LongArray & s) : TSArray <long> (s)
{
}

int FASTCALL LongArray::operator == (const LongArray & rS) const
{
	return IsEqual(&rS);
}

int FASTCALL LongArray::operator != (const LongArray & rS) const
{
	return BIN(!IsEqual(&rS));
}

LongArray & FASTCALL LongArray::operator = (const LongArray & s)
{
	copy(s);
	return *this;
}

int FASTCALL LongArray::IsEqual(const LongArray * pS) const
{
	if(pS) {
		const uint c = getCount();
		if(pS->getCount() == c) {
			for(uint i = 0; i < c; i++)
				if(pS->at(i) != at(i))
					return 0;
			return 1;
		}
	}
	return 0;
}

int FASTCALL LongArray::add(long id)
{
	return insert(&id);
}

int FASTCALL LongArray::addnz(long id)
{
	return id ? insert(&id) : -1;
}

int FASTCALL LongArray::addUnique(long id)
{
	return lsearch(id) ? -1 : add(id);
}

int SLAPIV LongArray::addzlist(long first, ...)
{
	int    ok = 1;
	va_list list;
	va_start(list, first);
	long   val;
	ok = add(first);
	while(ok && (val = va_arg(list, long)) != 0) {
		if(!add(val))
			ok = 0;
	}
	va_end(list);
	return ok;
}

int FASTCALL LongArray::add(const LongArray * pS)
{
	int    ok = 1;
	const  uint _c = pS ? pS->getCount() : 0;
	if(_c) {
		/* @v9.0.0
		for(uint i = 0; ok && i < _c; i++)
			if(!add(pS->at(i)))
				ok = 0;
		*/
		ok = insertChunk(_c, pS->dataPtr()); // @v9.0.0
	}
	return ok;
}

int SLAPI LongArray::addExclusive(const LongArray * pS, const LongArray * pExcl)
{
	int   ok = 1;
	const uint _c = pS ? pS->getCount() : 0;
	if(_c) {
		if(pExcl) {
			for(uint i = 0; ok && i < _c; i++) {
				const long val = pS->at(i);
				if(!pExcl || !pExcl->lsearch(val))
					if(!add(val))
						ok = 0;
			}
		}
		else {
			/* @v9.0.0
			for(uint i = 0; ok && i < _c; i++)
				if(!add(pS->at(i)))
					ok = 0;
			*/
			ok = insertChunk(_c, pS->dataPtr()); // @v9.0.0
		}
	}
	return ok;
}

int SLAPI LongArray::addUniqueExclusive(const LongArray * pS, const LongArray * pExcl)
{
	if(pS) {
		const uint c = pS->getCount();
		if(pExcl) {
			for(uint i = 0; i < c; i++) {
				const long val = pS->at(i);
				if(!pExcl || !pExcl->lsearch(val))
					if(!addUnique(val))
						return 0;
			}
		}
		else {
			for(uint i = 0; i < c; i++)
				if(!addUnique(pS->at(i)))
					return 0;
		}
	}
	return 1;
}

int FASTCALL LongArray::addUnique(const LongArray * pS)
{
	return addUniqueExclusive(pS, 0);
}

int SLAPI LongArray::removeByID(long id)
{
	uint   pos = 0;
	return lsearch(id, &pos) ? atFree(pos) : -1;
}

int SLAPI LongArray::ordInsert(long i, uint * p)
{
	return SArray::ordInsert(&i, p, CMPF_LONG);
}

int FASTCALL LongArray::bsearch(long i, uint * p) const
	{ return SArray::bsearch(&i, p, CMPF_LONG); }
int FASTCALL LongArray::bsearch(long i) const
	{ return SArray::bsearch(&i, 0, CMPF_LONG); }
//
// @todo Ускорить функцию, закладываясь на особенности массива LongArray
//
int FASTCALL LongArray::lsearch(long k, uint * pIdx) const
	{ return SArray::lsearch(&k, pIdx, CMPF_LONG); }
int FASTCALL LongArray::lsearch(long k) const
	{ return SArray::lsearch(&k, 0, CMPF_LONG); }

int  SLAPI LongArray::isList() const
{
	return (count > 1) ? 1 : 0;
}

long FASTCALL LongArray::get(uint pos) const
{
	return at(pos);
}

long SLAPI LongArray::getSingle() const
{
	return (count == 1) ? at(0) : 0L;
}

int SLAPI LongArray::setSingleNZ(long id)
{
	freeAll();
	return id ? add(id) : -1;
}

long SLAPI LongArray::getLast() const
{
	return (count ? at(count-1) : 0);
}

int SLAPI LongArray::sort()
{
	return SArray::sort(CMPF_LONG);
}

int SLAPI LongArray::sortAndUndup()
{
	int    ok = 1;
	uint   c = getCount();
	if(SArray::sort(CMPF_LONG)) {
		assert(c == getCount());
		if(c > 1) {
			long   prev = get(--c);
			do {
				const long t = get(--c);
				if(t == prev)
					atFree(c+1);
				prev = t;
			} while(c);
		}
	}
	else
		ok = 0;
	return ok;
}

int SLAPI LongArray::isSorted() const
{
	int   yes = 1;
    const uint c = getCount();
    if(c > 1) {
		long   prev = get(0);
		for(uint i = 1; yes && i < c; i++) {
			const long curr = get(i);
			if(curr < prev)
				yes = 0;
			else
				prev = curr;
		}
    }
    return yes;
}

int SLAPI LongArray::intersect(const LongArray * p, int binary)
{
	if(!p || !p->getCount())
		freeAll();
	else {
		uint c = getCount();
		if(c) do {
			const long id = at(--c);
			if((binary ? p->bsearch(id) : p->lsearch(id)) == 0)
				atFree(c);
		} while(c);
	}
	return 1;
}

#ifndef _WIN32_WCE // {

int SLAPI LongArray::intersect(const UintHashTable * p)
{
	if(!p)
		freeAll();
	else {
		uint c = getCount();
		if(c) do {
			const ulong id = (ulong)at(--c);
			if(!p->Has(id))
				atFree(c);
		} while(c);
	}
	return 1;
}

#endif // } _WIN32_WCE

int SLAPI LongArray::freeByKey(long key, int binarySearch)
{
	uint pos = 0;
	if(binarySearch ? bsearch(key, &pos) : lsearch(key, &pos)) {
		atFree(pos);
		return 1;
	}
	return -1;
}
//
//
//
SLAPI Int64Array::Int64Array() : TSArray <int64> ()
{
}

SLAPI Int64Array::Int64Array(const Int64Array & rS) : TSArray <int64> (rS)
{
}

Int64Array & FASTCALL Int64Array::operator = (const Int64Array & rS)
{
	copy(rS);
	return *this;
}

int64 FASTCALL Int64Array::get(uint pos) const
{
	return at(pos);
}

int Int64Array::lsearch(int64 k, uint * pIdx) const
	{ return SArray::lsearch(&k, pIdx, PTR_CMPFUNC(int64)); }
int Int64Array::lsearch(int64 k) const
	{ return SArray::lsearch(&k, 0, PTR_CMPFUNC(int64)); }

int FASTCALL Int64Array::add(int64 v)
{
	return insert(&v);
}

int FASTCALL Int64Array::add(int32 v)
{
	int64 v64 = (int64)v;
	return insert(&v64);
}
//
//
//
SLAPI FloatArray::FloatArray() : TSArray <float>()
	{}

int FASTCALL FloatArray::operator == (const FloatArray & rS) const
{
	return IsEqual(&rS);
}

int FASTCALL FloatArray::operator != (const FloatArray & rS) const
{
	return BIN(!IsEqual(&rS));
}

int FASTCALL FloatArray::dim(uint numItems)
{
    clear();
	return insertChunk(numItems, 0);
}

int FASTCALL FloatArray::add(float item)
	{ return insert(&item); }
int FASTCALL FloatArray::add(FPoint p)
	{ return insertChunk(2, &p); }
float FASTCALL FloatArray::get(uint i) const
	{ return at(i); }
float & FASTCALL FloatArray::operator [] (size_t i) const
	{ return at(i); }

const FPoint & FASTCALL FloatArray::getPoint(uint i) const
{
	assert(i < (count-1));
	return *(FPoint *)SArray::at(i);
}

int FASTCALL FloatArray::IsEqual(const FloatArray * pS) const
{
	if(pS) {
		const uint c = getCount();
		if(pS->getCount() == c) {
			for(uint i = 0; i < c; i++)
				if(pS->at(i) != at(i))
					return 0;
			return 1;
		}
	}
	return 0;
}
//
//
//
SLAPI FloatStack::FloatStack() : TSStack <float>(4)
{
}

int SLAPI FloatStack::pushPoint(FPoint p)
{
	push(p.Y);
	push(p.X);
	return 1;
}

int SLAPI FloatStack::popPoint(FPoint & rP)
	{ return (pop(rP.X) && pop(rP.Y)) ? 1 : 0; }
//
//
//
SLAPI STimeChunkArray::STimeChunkArray() : SArray(sizeof(STimeChunk))
{
}

SLAPI STimeChunkArray::STimeChunkArray(size_t itemSize) : SArray(itemSize)
{
	assert(itemSize >= sizeof(STimeChunk));
}

int SLAPI STimeChunkArray::Add(const STimeChunk * pEntry, int checkForFree)
{
	int    ok = 0;
	assert(pEntry);
	if(pEntry)
		if(!checkForFree || IsFreeEntry(*pEntry, 0))
			ok = insert(pEntry) ? 1 : 0;
		else
			ok = -1;
	return ok;
}

int SLAPI STimeChunkArray::Helper_Intersect(const STimeChunk & rChunk, STimeChunkArray & rResult) const
{
	int    yes = 0;
	for(uint j = 0; j < getCount(); j++) {
		const STimeChunk * p_item = (const STimeChunk *)at(j);
		STimeChunk sect;
		if(rChunk.Intersect(*p_item, &sect) > 0) {
			rResult.Add(&sect, 0);
			yes = 1;
		}
	}
	return yes;
}

int SLAPI STimeChunkArray::Intersect(const STimeChunkArray * pList, STimeChunkArray * pResult) const
{
	int    yes = 0;
	pResult->clear();
	for(uint i = 0; i < pList->getCount(); i++) {
		const STimeChunk * p_item1 = (const STimeChunk *)pList->at(i);
		if(Helper_Intersect(*p_item1, *pResult))
			yes = 1;
	}
	pResult->Sort();
	return yes;
}

int SLAPI STimeChunkArray::Intersect(const STimeChunk & rChunk, STimeChunkArray * pResult) const
{
	int    yes = 0;
	pResult->clear();
	yes = Helper_Intersect(rChunk, *pResult);
	pResult->Sort();
	return yes;
}

uint SLAPI STimeChunkArray::GetIntersectionOrder(LAssocArray * pOrderList, long tolerance) const
{
	uint   i;
	uint   order = 0;
	STimeChunk sect;
	LongArray level;
	CALLPTRMEMB(pOrderList, freeAll());
	for(i = 0; i < getCount(); i++)
		level.add((long)i);
	for(uint c = level.getCount(); c != 0; c = level.getCount()) {
		LongArray temp_level;
		for(i = 0; i < c; i++) {
			const uint k = (uint)level.get(i);
			CALLPTRMEMB(pOrderList, Update((long)(k+1), order, 1));
			const STimeChunk * p_item_k = (const STimeChunk *)at(k);
			for(uint j = i+1; j < c; j++) {
				const uint m = (uint)level.get(j);
				if(p_item_k->Intersect(*(const STimeChunk *)at(m), &sect)) {
					if(tolerance <= 0 || sect.GetDurationMs() > tolerance)
						temp_level.addUnique(m);
				}
			}
		}
		++order;
		level = temp_level;
	}
	return order;
}

int SLAPI STimeChunkArray::Excise(const STimeChunk * pItem)
{
	int    ok = -1;
	if(pItem) {
		STimeChunk test, sect, * p_entry;
		// @v8.4.11 {
		if(getCount() == 0) {
			STimeChunk inf;
			inf.Start.SetZero();
			inf.Finish.SetFar();
			insert(&inf);
		}
		// } @v8.4.11
		for(uint i = 0; enumItems(&i, (void **)&p_entry);) {
			if(p_entry->Intersect(*pItem, &sect)) {
				if(*p_entry == sect) {
					atFree(--i);
					ok = 1;
				}
				else {
					int    r_start  = cmp(sect.Start,  p_entry->Start);
					int    r_finish = cmp(sect.Finish, p_entry->Finish);
					if(r_start == 0 || r_finish == 0) {
						*p_entry = sect;
					}
					else /*if(r_start > 0)*/ {
						const LDATETIME org_entry_finish = p_entry->Finish;
						STimeChunk next;
						if(!sect.Finish.IsFar())
							sect.Finish.addhs(100);
						// @v8.4.11 {
						if(r_start > 0) {
							p_entry->Finish = sect.Start;
						}
						// } @v8.4.11
						if(r_finish < 0) { // @v8.4.11
							next.Init(sect.Finish, org_entry_finish);
							if(p_entry->Finish.d)
								p_entry->Finish.addhs(-100);
							//
							// i опережает на единицу значение текущего индекса.
							// Кроме того, нам необходо увеличить i для того, чтобы
							// цикл не осматривал новый элемент (он не пересекается с pItem).
							// В итоге: i++ (постфиксная форма существенна).
							//
							atInsert(i++, &next);
						} // @v8.4.11
					}
					ok = 1;
				}
			}
		}
	}
	return ok;
}

int SLAPI STimeChunkArray::Sort(int desc)
{
	return SArray::sort(PTR_CMPFUNC(STimeChunk), (void *)desc);
}

int SLAPI STimeChunkArray::Limit(const STimeChunk * pPeriod)
{
	int    ok = -1;
	if(pPeriod) {
		STimeChunk test, sect, * p_entry;
		test.Init(pPeriod->Start, pPeriod->Finish);
		for(uint i = 0; enumItems(&i, (void **)&p_entry);)
			if(p_entry->Intersect(test, &sect)) {
				if(!(*p_entry == sect)) {
					*p_entry = sect;
					ok = 1;
				}
			}
			else {
				atFree(--i);
				ok = 1;
			}
	}
	return ok;
}

int SLAPI STimeChunkArray::IsFreeEntry(const STimeChunk & rEntry, uint * pPos) const
{
	STimeChunk result, * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);) {
		if(p_item->Intersect(rEntry, &result) && cmp(result.Start, result.Finish) != 0) {
			ASSIGN_PTR(pPos, i-1);
			return 0;
		}
	}
	return 1;
}

int SLAPI STimeChunkArray::IsFreeEntry(const LDATETIME & start, long cont, uint * pPos) const
{
	STimeChunk entry;
	entry.Init(start, cont);
	return start.d ? IsFreeEntry(entry, pPos) : 1;
}

int SLAPI STimeChunkArray::GetFreeList(STimeChunkArray * pList) const
{
	STimeChunk free_entry;
	LDATETIME last;
	last.SetZero();
	for(uint i = 0; i < getCount(); i++) {
		const STimeChunk & entry = *(STimeChunk *)at(i);
		if(cmp(last, entry.Start) < 0) {
			free_entry.Init(last, entry.Start);
			if(!free_entry.Finish.IsFar())
				free_entry.Finish = plusdatetime(free_entry.Finish, -1, 3);
			last = entry.Finish.IsFar() ? entry.Finish : plusdatetime(entry.Finish, 1, 3);
			pList->insert(&free_entry);
		}
		else
			last = entry.Finish.IsFar() ? entry.Finish : plusdatetime(entry.Finish, 1, 3);
	}
	if(!last.IsFar()) {
		free_entry.Start = last;
		free_entry.Finish.SetFar();
		pList->insert(&free_entry);
	}
	return 1;
}

int SLAPI STimeChunkArray::GetBounds(STimeChunk * pBounds, int update) const
{
	int    ok = 0;
	if(pBounds) {
		const STimeChunk * p_chunk;
		STimeChunk bounds;
		if(update)
			bounds = *pBounds;
		else {
			bounds.Start.SetZero();
			bounds.Finish.SetFar();
		}
		for(uint i = 0; enumItems(&i, (void **)&p_chunk);) {
			if(!bounds.Start.d || cmp(p_chunk->Start, bounds.Start) < 0)
				bounds.Start = p_chunk->Start;
			if(bounds.Finish.IsFar() || cmp(p_chunk->Finish, bounds.Finish) > 0)
				bounds.Finish = p_chunk->Finish;
		}
		if(bounds.Start.d)
			ok |= 0x01;
		if(!bounds.Finish.IsFar())
			ok |= 0x02;
		ok |= 0x04;
		ASSIGN_PTR(pBounds, bounds);
	}
	return ok;
}
//
//
//
STimeChunkAssocArray::STimeChunkAssocArray(long id) : STimeChunkArray(sizeof(STimeChunkAssoc))
{
	Id = id;
}

int STimeChunkAssocArray::Get(long chunkId, uint * pPos, STimeChunkAssoc * pItem) const
{
	uint   pos = 0;
	if(lsearch(&chunkId, &pos, CMPF_LONG, offsetof(STimeChunkAssoc, Id))) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pItem, *(STimeChunkAssoc *)at(pos));
		return 1;
	}
	else
		return 0;
}

int STimeChunkAssocArray::Add(long chunkId, const STimeChunk * pChunk, int checkForFree)
{
	return Add(chunkId, 0, pChunk, checkForFree);
}

int STimeChunkAssocArray::Add(long chunkId, long status, const STimeChunk * pChunk, int checkForFree)
{
	STimeChunkAssoc item;
	MEMSZERO(item);
	if(pChunk)
		item.Chunk = *pChunk;
	item.Id = chunkId;
	item.Status = status;
	return insert(&item);
}

int STimeChunkAssocArray::Remove(long chunkId)
{
	uint   pos = 0;
	if(lsearch(&chunkId, &pos, CMPF_LONG, offsetof(STimeChunkAssoc, Id))) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}
//
//
//
static int FASTCALL Subst(const void * i1, const void * i2, void * pExtraData)
{
	return stcomp((TYPEID)pExtraData, i1, i2);
}

SLAPI STypArray::STypArray(TYPEID aType, uint aDelta, int o) : SArray(stsize(aType), aDelta, o)
{
	type = aType;
}

/* @v9.0.5
int FASTCALL STypArray::insert(const void * pItem)
{
	uint p;
	return (flags & arySorted) ? SArray::ordInsert(pItem, &p, Subst, (void *)type) : SArray::insert(pItem);
}
*/

// virtual
void FASTCALL STypArray::freeItem(void * pItem)
{
	stdestroy(type, pItem);
	SArray::freeItem(pItem);
}

SString & SLAPI STypArray::itemToString(uint idx, long fmt, SString & rBuf) const
{
	rBuf = 0;
	const void * p = at(idx);
	if(p) {
		char   temp_buf[256];
		if(sttostr(type, p, fmt, temp_buf))
			rBuf = temp_buf;
	}
	return rBuf;
}

int SLAPI STypArray::search(const void * pKey, uint * pPos) const
{
	return (flags & arySorted) ? bsearch(pKey, pPos, Subst, 0, (void *)type) : lsearch(pKey, pPos, Subst, 0, (void *)type);
}

int SLAPI STypArray::sort()
{
	return SArray::sort(Subst, (void *)type);
}
//
// STree
//
STree::Iter::Iter()
{
	ParentP = 0;
	CurrP = 0;
	CurrDataP = 0;
	NextP = 0;
	Flags = fFirstIter;
	P_Data = 0;
}

STree::Iter::Iter(uint32 parentP)
{
	ParentP = parentP;
	CurrP = 0;
	CurrDataP = 0;
	NextP = 0;
	Flags = (fFirstIter | fOnlyParent);
	P_Data = 0;
}

void * STree::Iter::GetData() const
{
	return CurrP ? P_Data : 0; // Нулевая позиция дерева не может ссылаться на данные.
}

uint32 STree::Iter::GetCurrentPos() const
{
	return CurrP;
}

uint32 STree::Iter::GetParentPos() const
{
	return ParentP;
}
//
//
//
STree::STree(uint itemSize, int o) : SArray(itemSize, 32, (o & (aryDataOwner|aryEachItem|aryPtrContainer)))
{
	Item_ stub;
	MEMSZERO(stub);
	TreeList.insert(&stub);
}

STree & STree::Clear()
{
	clear();
	TreeList.clear();
	SvcFreeList.Clear();
	DatFreeList.Clear();
	Item_ stub;
	MEMSZERO(stub);
	TreeList.insert(&stub);
	return *this;
}

STree::~STree()
{
}

/*
STree::Item_ * FASTCALL STree::Get_(uint p) const
{
	return (STree::Item_ *)TreeList.at(p);
}
*/

void * FASTCALL STree::GetData(uint32 pos) const
{
	void * p_data = 0;
	if(pos < TreeList.getCount()) {
		Item_ & r_item = TreeList.at(pos);
		if(r_item.DataP < getCount())
			p_data = at(r_item.DataP);
	}
	return p_data;
}

int STree::SearchDataP(uint32 dataP, uint32 * pPos) const
{
	int    ok = 0;
	//
	// Поиск блока, ссылающегося на блок данных dataP начинаем
	// с позиции 1, поскольку позиция 0 является эксклюзивной - корневой.
	//
	for(uint pos = 1; TreeList.lsearch(&dataP, &pos, CMPF_LONG); pos++) {
		if(!SvcFreeList.Has(pos)) {
			ASSIGN_PTR(pPos, pos);
			ok = 1;
			break;
		}
	}
	return ok;
}

uint32 FASTCALL STree::GetFirstChildP(uint32 itemP) const
{
	return (itemP < TreeList.getCount()) ? TreeList.at(itemP).FirstChildP : 0;
}

int STree::Search(const void * pKey, uint32 * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData) const
{
	int    ok = 0;
	for(uint pos = 0; SArray::lsearch(pKey, &pos, fcmp, ofs, pExtraData); pos++) {
		if(!DatFreeList.Has(pos)) {
			uint32 svc_pos = 0;
			int    r = SearchDataP(pos, &svc_pos);
			assert(r); // Невозможность найти ссылку на блок данных в дереве является признаком нарушения целостности дерева.
			if(r) {
				ASSIGN_PTR(pIdx, svc_pos);
				ok = 1;
				break;
			}
		}
	}
	return ok;
}

int STree::Search(const void * pKey, Iter & rIter, CompFunc fcmp, size_t ofs, void * pExtraData) const
{
	int    ok = 0;
	while(Enum(rIter)) {
		if(isItemEq(pKey, rIter.CurrDataP, fcmp, ofs, pExtraData)) {
			ok = 1;
			break;
		}
	}
	return ok;
}

STree::Item_ * FASTCALL STree::GetNext_(const Item_ * pItem) const
{
	return (pItem && pItem->NextP) ? &TreeList.at(pItem->NextP) : 0;
}

int FASTCALL STree::SetupIter(Iter & rIter, uint32 currP) const
{
	int    ok = 0;
	if(currP) {
		const Item_ & r_item = TreeList.at(currP);
		rIter.CurrP = currP;
		rIter.CurrDataP = r_item.DataP;
		rIter.NextP = r_item.NextP;
		rIter.P_Data = at(r_item.DataP);
		ok = 1;
	}
	return ok;
}

int FASTCALL STree::Enum(Iter & rIter) const
{
	int    ok = 0;
	Item_ * p_item = 0;
	if(rIter.Flags & Iter::fFirstIter) {
		rIter.Flags &= ~Iter::fFirstIter;
		Item_ & r_par = TreeList.at(rIter.ParentP);
		if(SetupIter(rIter, r_par.FirstChildP))
			ok = 1;
	}
	else if(SetupIter(rIter, rIter.NextP)) // @1
		ok = 1;
	else if(!(rIter.Flags & Iter::fOnlyParent)) {
		uint32 p = 0;
		//
		// Находим на текущем уровне первый элемент, имеющий потомков (все последующие
		// элементы с потомками на этом уровне будут пройдены после обхода потомков
		// первого узла и возврата из стэка: см. следующий блок).
		//
		// @todo Этот цикл можно обойти если на этапе перебора элементов (@1) запомнить первый
		// элемент, имеющий потомков.
		//
		Item_ & r_par = TreeList.at(rIter.ParentP);
		if(r_par.FirstChildP) {
			p_item = &TreeList.at(p = r_par.FirstChildP);
			while(p_item && !p_item->FirstChildP)
				p_item = ((p = p_item->NextP) != 0) ? &TreeList.at(p) : 0;
		}
		//
		// Если на текущем уровне нет элементов с потомками, то возвращаемся не предыдущий уровень
		// в поисках следующего элемента с потомками
		//
		while(!p_item && rIter.Stack.getPointer()) {
			rIter.Stack.pop(p);
			p_item = &TreeList.at(p);
			p = p_item->NextP;
			p_item = p ? &TreeList.at(p) : 0;
			while(p_item && !p_item->FirstChildP)
				p_item = ((p = p_item->NextP) != 0) ? &TreeList.at(p) : 0;
		}
		//
		// Если найден очередной элемент с потомками p_item, то спускаемся на следующий уровень,
		// родителем которого является p_item. При этом в стэк Stack заносим информацию о том,
		// с какого элемента мы спустились.
		//
		if(p_item) {
			rIter.Stack.push(p);
			rIter.ParentP = p;
			if(SetupIter(rIter, p_item->FirstChildP))
				ok = 1;
		}
	}
	return ok;
}

int STree::CreateNode(uint32 parentP, uint32 prevP, uint32 dataP, uint32 * pP)
{
	int    ok = 1;
	uint32 pos = 0;
	STree::Item_ item;
	MEMSZERO(item);
	item.ParentP = parentP;
	item.PrevP = prevP;
	item.DataP = dataP;
	if(SvcFreeList.Enum(&pos) > 0) {
		assert(pos < TreeList.getCount());
		TreeList.atPut(pos, &item);
	}
	else {
		pos = TreeList.getCount();
		TreeList.insert(&item);
	}
	ASSIGN_PTR(pP, pos);
	return ok;
}

int STree::CreateDataEntry(const void * pData, uint32 * pPos)
{
	int    ok = 1;
	uint32 pos = 0;
	if(DatFreeList.Enum(&pos) > 0) {
		assert(pos < getCount());
		atPut(pos, pData);
	}
	else {
		pos = getCount();
		insert(pData);
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int STree::Insert(const void * pData, uint32 parentP, uint32 * pPos)
{
	int    ok = 1;
	uint32 node_pos = 0;
	uint32 data_pos = 0;
	THROW(parentP < TreeList.getCount());
	{
		Item_ & r_parent = TreeList.at(parentP);
		if(r_parent.FirstChildP) {
			uint32 child_p = r_parent.FirstChildP;
			const Item_ * p_child = &TreeList.at(child_p);
			assert(p_child->PrevP == 0);
			assert(p_child->ParentP == parentP);
			while(p_child->NextP) {
				const uint32 prev_p = child_p; // @debug
				p_child = &TreeList.at(child_p = p_child->NextP);
				assert(p_child->ParentP == parentP);
				assert(p_child->PrevP == prev_p);
			}
			THROW(CreateDataEntry(pData, &data_pos));
			THROW(CreateNode(parentP, child_p, data_pos, &node_pos));
			TreeList.at(child_p).NextP = node_pos;
		}
		else {
			THROW(CreateDataEntry(pData, &data_pos));
			THROW(CreateNode(parentP, 0, data_pos, &node_pos));
			TreeList.at(parentP).FirstChildP = node_pos;
			assert(TreeList.at(node_pos).ParentP == parentP);
		}
	}
	ASSIGN_PTR(pPos, node_pos);
	CATCHZOK
	return ok;
}

int STree::InsertBefore(const void * pData, uint32 nextP, uint32 * pPos)
{
	int    ok = 1;
	uint32 node_pos = 0;
	uint32 data_pos = 0;
	THROW(nextP < TreeList.getCount());
	{
		Item_ & r_next = TreeList.at(nextP);
		const uint32 parent_p = r_next.ParentP;
		const uint32 prev_p = r_next.PrevP;
		assert(parent_p < TreeList.getCount());
		Item_ & r_parent = TreeList.at(parent_p);
		assert(prev_p != 0 || r_parent.FirstChildP == nextP);
		THROW(CreateDataEntry(pData, &data_pos));
		THROW(CreateNode(parent_p, prev_p, data_pos, &node_pos));
		if(prev_p == 0) {
			TreeList.at(parent_p).FirstChildP = node_pos;
		}
		else {
			TreeList.at(prev_p).NextP = node_pos;
		}
		TreeList.at(nextP).PrevP = node_pos;
		TreeList.at(node_pos).NextP = nextP;
	}
	ASSIGN_PTR(pPos, node_pos);
	CATCHZOK
	return ok;
}

int STree::Delete(uint32 pos)
{
	int    ok = 1;
	if(pos < TreeList.getCount()) {
		Item_ & r_item = TreeList.at(pos);
		assert(r_item.ParentP < TreeList.getCount());
		Item_ & r_par  = TreeList.at(r_item.ParentP);
		Item_ * p_next = r_item.NextP ? &TreeList.at(r_item.NextP) : 0;
		Item_ * p_prev = r_item.PrevP ? &TreeList.at(r_item.PrevP) : 0;
		if(p_next) {
			assert(p_next->ParentP == r_item.ParentP);
			assert(p_next->PrevP == pos);
			if(p_prev) {
				assert(p_prev->ParentP == r_item.ParentP);
				assert(p_prev->NextP == pos);
				p_prev->NextP = r_item.NextP;
				p_next->PrevP = r_item.PrevP;
			}
			else {
				assert(r_par.FirstChildP == pos);
				r_par.FirstChildP = r_item.NextP;
			}
		}
		else if(p_prev) {
			assert(p_prev->ParentP == r_item.ParentP);
			assert(p_prev->NextP == pos);
			p_prev->NextP = 0;
		}
		else {
			assert(r_par.FirstChildP == pos);
			r_par.FirstChildP = 0;
		}
		SvcFreeList.Add(pos);
		DatFreeList.Add(r_item.DataP);
	}
	return ok;
}


/*int STree::Sort(CompFunc fcmp, long extraData)
{

}*/

static int Helper_TestSTree(const char * pPath, StrAssocArray & rSaList, STree & rTree, long parentId, uint32 parentP, long * pCntr)
{
	int    ok = 1;
	SDirEntry de;
	SString path;
	(path = pPath).SetLastSlash().Cat("*.*");
	for(SDirec direc(path, 0); direc.Next(&de) > 0;) {
		if(!de.IsSelf() && !de.IsUpFolder()) {
			long   id = ++(*pCntr);
			uint32 pos = 0;
			rSaList.Add(id, parentId, de.FileName);
			THROW(rTree.Insert(&id, parentP, &pos));
			if(de.IsFolder()) {
				(path = pPath).SetLastSlash().Cat(de.FileName);
				THROW(Helper_TestSTree(path, rSaList, rTree, id, pos, pCntr)); // @recursion
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI TestSTree()
{
	int    ok = 1;
	StrAssocArray sa_list;
	STree  tree(sizeof(long));
	long   cntr = 0;
	THROW(Helper_TestSTree("D:\\PAPYRUS", sa_list, tree, 0, 0, &cntr));
	{
		SString text_buf, out_buf;
		SFile f_sa("test_stree_sa.txt", SFile::mWrite);
		SFile f_tree("test_stree_tree.txt", SFile::mWrite);
		for(STree::Iter iter; tree.Enum(iter);) {
			long id = *(long *)iter.GetData();
			long parent_id_sa = 0;
			long parent_id_tree = 0;
			void * p_parent = tree.GetData(iter.GetParentPos());
			if(p_parent)
				parent_id_tree = *(long *)p_parent;
			sa_list.Get(id, text_buf = 0);
			sa_list.GetParent(id, &parent_id_sa);

			f_tree.WriteLine((out_buf = 0).Cat(id).CatDiv(';', 2).Cat(parent_id_tree).CatDiv(';', 2).Cat(text_buf).CR());
			f_sa.WriteLine((out_buf = 0).Cat(id).CatDiv(';', 2).Cat(parent_id_sa).CatDiv(';', 2).Cat(text_buf).CR());
		}

	}
	CATCHZOK
	return ok;
}
//
// @TEST {
//
#if SLTEST_RUNNING // {

struct TestFixtureSArray {
	TestFixtureSArray()
	{
		memzero(Tail, sizeof(Tail));
		for(uint i = 0; i < SIZEOFARRAY(Sa); ++i) {
			Sa[i] = SLS.GetTLA().Rg.GetUniformInt(119000);
		}
	}
	uint GetSaCount() const
	{
		return sizeof(Sa) / sizeof(Sa[0]);
	}
	long   Sa[7000];
	long   Tail[17]; // хвост, который не должен измениться ни при каких условиях
};

SLTEST_FIXTURE(SArray, TestFixtureSArray)
{
	uint   i;
	SArray * p_list = 0;
	p_list = new SArray(sizeof(long));
	const uint cs1 = 0,   cc1 = F.GetSaCount() / 4;
	const uint cs2 = cc1, cc2 = cc1 + F.GetSaCount() / 2;
	const uint cs3 = cc2, cc3 = cc2 + F.GetSaCount() / 4;
	//
	// Четверть элементов вставляем в конец списка (самая простая и быстрая операция)
	//
	for(i = cs3; i < cc3; i++) {
		SLTEST_CHECK_NZ(p_list->insert(&F.Sa[i]));
	}
	//
	// Четверть элементов вставляем в начало списка (при этом предшествующая четверть будет постепенно сдвигаться)
	//
	for(i = cs1; i < cc1; i++) {
		SLTEST_CHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Оставшуюся половину вставляем в середину списка (опять заставляет массив сдвигать хвостовые члены)
	//
	for(i = cs2; i < cc2; i++) {
		SLTEST_CHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Тестируем функцию incPointerSafe
	//
	{
		p_list->setPointer(0);
		for(int _inc = -3; _inc < (int)p_list->getCount() + 7; _inc++) {
			const uint prev_ptr = p_list->getPointer();
			SLTEST_CHECK_EQ(p_list->incPointerSafe(_inc), prev_ptr);
			SLTEST_CHECK_CRANGE(p_list->getPointer(), 0UL, p_list->getCount()-1);
		}
		SLTEST_CHECK_EQ(p_list->getPointer(), p_list->getCount()-1);
	}
	//
	// Теперь сравниваем элементы list с элементами F.Sa. Они должны быть в том же порядки и равны
	//
	if(SLTEST_CHECK_EQ((ulong)p_list->getCount(), (ulong)F.GetSaCount())) {
		for(i = 0; i < F.GetSaCount(); i++) {
			SLTEST_CHECK_EQ(*(long *)p_list->at(i), F.Sa[i]);
		}
		long p;
		//
		// Далее, занимаемся поиском. Перебираем все значения от 0 до F.GetSaCount().
		// Сколько раз они встречаются в F.Sa столько раз мы должны их найти в p_list
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong ls_ss = 0;
			ulong ls_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++ls_ss;
			for(i = 0; p_list->lsearch(&p, &i, CMPF_LONG); i++)
				++ls_sl;
			SLTEST_CHECK_EQ(ls_ss, ls_sl);
		}
		//
		// Реверс списка
		//
		p_list->reverse(0, p_list->getCount());
		for(i = 0; i < F.GetSaCount(); i++) {
			SLTEST_CHECK_EQ(*(long *)p_list->at(i), F.Sa[F.GetSaCount()-i-1]);
		}
		//
		// Сортируем список p_list и тестируем правильность сортировки
		//
		p_list->sort(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLTEST_CHECK_LE(*(long *)p_list->at(i), *(long *)p_list->at(i+1));
		}
		//
		// Перемешиваем элементы массива случайным образом
		//
		p_list->shuffle();
		{
			//
			// Убеждаемся, что массив более не отсортирован
			//
			int    sorted = 1;
			for(i = 0; sorted && i < p_list->getCount()-1; i++) {
				if(*(long *)p_list->at(i) > *(long *)p_list->at(i+1))
					sorted = 0;
			}
			SLTEST_CHECK_EQ(sorted, (long)0);
		}
		//
		// Теперь сортируем список p_list методом sort2 и тестируем правильность сортировки
		//
		p_list->sort2(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLTEST_CHECK_LE(*(long *)p_list->at(i), *(long *)p_list->at(i+1));
		}
		//
		// Тестируем бинарный поиск (алгоритм аналогичен тесту последовательного поиска)
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong bs_ss = 0;
			ulong bs_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++bs_ss;
			if(p_list->bsearch(&p, &i, CMPF_LONG)) {
				++bs_sl;
				//
				// Бинарный поиск может найти только самый первый элемент. Остальные элементы
				// выискиваем последовательным перебором
				//
				while(i < p_list->getCount() && *(long *)p_list->at(++i) == p)
					++bs_sl;
			}
			SLTEST_CHECK_EQ(bs_ss, bs_sl);
		}
	}
	// @todo Тест удаления элементов из массива
	delete p_list;
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
//
// } @TEST
//
