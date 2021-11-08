// SARRAY.CPP
// Copyright (c) Sobolev A. 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop

#define _USERPTR___(ptr) (ptr)
#define _USERPTR_PC(ptr) (*static_cast<void **>(ptr))
#define _USERPTR(ptr)    ((VFlags&aryPtrContainer)?(*reinterpret_cast<void **>(ptr)):(ptr))
#define _USERPTR_C(ptr)  ((VFlags&aryPtrContainer)?(*reinterpret_cast<const void * const *>(ptr)):(ptr))
#define _INTRPTR(ptr)    ((VFlags&aryPtrContainer)?&(ptr):(ptr))

/*static*/uint FASTCALL SVectorBase::GetCount(const SVectorBase * pB)
{
	return pB ? pB->getCount() : 0;
}

SVectorBase::SVectorBase(uint itemSize, uint o) : isize(itemSize), VFlags(o), P_VData(0), count(0), pointer(0), Limit(0)
{
}

SVectorBase::~SVectorBase()
{
}

int FASTCALL SVectorBase::CopyS(const SVectorBase & rS)
{
	int    ok = 1;
	if(&rS != 0) {
		isize = rS.isize;
		//delta = rS.delta;
		Limit = 0;
		count = 0;
		pointer = rS.pointer;
		VFlags = rS.VFlags;
		if(VFlags & aryDataOwner) {
			P_VData = 0;
			if(expand(rS.count)) {
				count = rS.count;
				memcpy(P_VData, rS.P_VData, isize * count);
			}
			else
				ok = 0;
		}
		else {
			Limit = rS.Limit;
			count = rS.count;
			P_VData = rS.P_VData;
		}
	}
	return ok;
}

static size_t FASTCALL CalcArrayDelta(uint itemSize)
{
	const uint min_delta = 8;
	const uint align_bits = 8;
    uint quant = Lcm(itemSize * min_delta, 1 << align_bits);
    assert((quant % itemSize) == 0);
    return quant / itemSize;
}

static uint FASTCALL SnapUpCount(uint newCount, uint itemSize)
{
	const size_t low_limit_bytes = 256;
	size_t s = newCount * itemSize;
	uint   result = newCount;
	// Если запрашивается первый элемент, то на один и распределяем пространство (один элемент в массиве очень популярное значение)
	if(result > 1) {
		if(s < low_limit_bytes)
			result = (low_limit_bytes / itemSize);
		else {
			size_t j = s;
			j |= (j >>  1);
			j |= (j >>  2);
			j |= (j >>  4);
			j |= (j >>  8);
			j |= (j >> 16);
			result = (j + 1) / itemSize; // Least power of two greater than i
		}
		assert(result >= newCount);
	}
	return result;
}

int FASTCALL SVectorBase::expand(uint numNewRecs)
{
	int    ok = 1;
	uint   new_count = count + numNewRecs;
	if(new_count > Limit) {
		const  size_t _is = static_cast<size_t>(isize);
		uint   new_limit = SnapUpCount(new_count, _is);
		assert(new_limit >= new_count);
		void * ptr = SAlloc::R(P_VData, new_limit * _is);
		while(!ptr && (new_limit > new_count)) {
			const uint minus_delta = ((new_limit - new_count) / 2);
			if(minus_delta) {
				new_limit -= minus_delta;
				ptr = SAlloc::R(P_VData, new_limit * _is);
			}
			else
				break;
		}
		if(ptr) {
			memzero(PTR8(ptr) + count * _is, (new_limit - count) * _is);
			P_VData = static_cast<uint8 *>(ptr);
			Limit = new_limit;
		}
		else
			ok = 0;
	}
	return ok;
}

/*
int FASTCALL SVectorBase::expand(uint numNewRecs)
{
	int    ok = 1;
	uint   new_count = count + numNewRecs;
	if(new_count > Limit) {
		const size_t _is = (size_t)isize;
		// @v9.1.1 const uint _delta = (delta > 0) ? (uint)delta : 8;
		const  uint _delta = CalcArrayDelta(isize); // @v9.1.1
		uint   new_limit = ((new_count + (_delta - 1)) / _delta) * _delta;
		void * ptr = SAlloc::R(data, new_limit * _is);
		if(ptr) {
			memzero(PTR8(ptr) + count * _is, (new_limit - count) * _is); // @v9.8.1
			data = (uint8 *)ptr;
			Limit = new_limit;
		}
		else
			ok = 0;
	}
	return ok;
}
*/

int SVectorBase::Shrink()
{
	// @v10.7.3 {
	int    ok = -1;
	if(count == 0) {
		ZFREE(P_VData);
		Limit = 0;
		ok = 1;
	}
	else if(fdivui(Limit, count) >= 1.2) {
		const uint newlimit = ALIGNSIZE(count, 4);
		if(newlimit < Limit) {
			void * p_new_ptr = SAlloc::M(newlimit * isize);
			if(p_new_ptr) {
				memcpy(p_new_ptr, P_VData, count * isize);
				SAlloc::F(P_VData);
				P_VData = static_cast<uint8 *>(p_new_ptr);
				Limit = newlimit;
				ok = 1;
			}
			else
				ok = 0;
		}
	}
	return ok;
	// } @v10.7.3 
}

void * SVectorBase::atPut(uint idx, const void * pItem)
{
	assert(idx < count);
	void * p_dest_ptr = P_VData + idx * isize;
	memmove(p_dest_ptr, _INTRPTR(pItem), isize);
	VFlags &= ~arySorted;
	return _USERPTR(p_dest_ptr);
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
static void insmem(void * pDest, const void * pSrc, size_t blkOffs, size_t blkSize, size_t destSize)
{
	if(blkSize == sizeof(uint32)) {
		uint8 * d = PTR8(pDest) + blkOffs * sizeof(uint32);
		if(destSize > blkOffs)
			memmove(d + blkSize, d, (destSize - blkOffs) * sizeof(uint32));
		PTR32(d)[0] = PTR32C(pSrc)[0];
	}
	else if(blkSize == sizeof(uint32)*2) {
		uint8 * d = PTR8(pDest) + (blkOffs * sizeof(uint32) * 2);
		if(destSize > blkOffs)
			memmove(d + blkSize, d, ((destSize - blkOffs) * sizeof(uint32) * 2));
		PTR32(d)[0] = PTR32C(pSrc)[0];
		PTR32(d)[1] = PTR32C(pSrc)[1];
	}
	else {
		uint8 * d = PTR8(pDest) + blkOffs * blkSize;
		if(destSize > blkOffs)
			memmove(d + blkSize, d, (destSize - blkOffs) * blkSize);
		memmove(d, pSrc, blkSize);
	}
}

int FASTCALL SVectorBase::insert(const void * pItem)
{
	//
	// @v7.8.0 Так как данная функция очень часто вызывается, вместо вызова atInsert(count, pItem)
	// сделаем прямую реализацию функции, дабы сэкономить немного времени.
	//
	const  uint _c = count;
	int    ok = 1;
	if(_c < Limit || expand(1)) { // @v8.1.11 (count < limit ||) так быстрее ибо придется реже входить в функцию expand
		const size_t quant = static_cast<size_t>(isize);
		//insmem(data, _INTRPTR(pItem), _c, quant, _c);
		//static void insmem(void * pDest, const void * pSrc, size_t blkOffs, size_t blkSize, size_t destSize)
		{
			const void * p_intr = _INTRPTR(pItem);
			uint8 * d = PTR8(P_VData) + _c * quant;
			if(quant == sizeof(uint32)) {
				PTR32(d)[0] = PTR32C(p_intr)[0];
			}
			else if(quant == sizeof(uint32)*2) {
				PTR32(d)[0] = PTR32C(p_intr)[0];
				PTR32(d)[1] = PTR32C(p_intr)[1];
			}
			else {
				memmove(d, p_intr, quant);
			}
		}
		count++;
		VFlags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL SVectorBase::atInsert(uint idx, const void * pItem)
{
	int    ok = 1;
	const  uint _c = count;
	assert(idx <= _c);
	if(_c < Limit || expand(1)) {
		insmem(P_VData, _INTRPTR(pItem), idx, isize, _c);
		count++;
		VFlags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

int SVectorBase::insertChunk(uint numItemsToInsert, const void * pItems)
{
	int    ok = 1;
	if(expand(numItemsToInsert)) {
		uint8 * d = PTR8(P_VData) + count * isize;
		if(pItems)
			memmove(d, pItems, isize * numItemsToInsert);
		else
			memzero(d, isize * numItemsToInsert);
		count += numItemsToInsert;
		VFlags &= ~arySorted;
	}
	else
		ok = 0;
	return ok;
}

uint FASTCALL SVectorBase::setPointer(uint p)
{
	uint prev = pointer;
	pointer = p;
	return prev;
}

uint FASTCALL SVectorBase::incPointerSafe(int incr)
{
    const uint _prev = pointer;
    if(incr > 0) {
		pointer = MIN((pointer + incr), count-1);
    }
    else if(incr < 0) {
		if((-incr) > static_cast<int>(pointer))
			pointer = 0;
		else
			pointer = pointer + incr;
    }
    return _prev;
}

uint   SVectorBase::getFlags() const { return VFlags; }
void   SVectorBase::setFlag(uint val, int set) { SETFLAG(VFlags, val, set); }
uint   SVectorBase::incPointer() { return pointer++; }
void * SVectorBase::dataPtr() const { return P_VData; }

void * FASTCALL SVectorBase::at(uint idx) const
{
	assert(idx < count);
	return _USERPTR(P_VData + idx * isize);
}

void * SVectorBase::next()
{
	if(pointer < count) {
		pointer++;
		return _USERPTR(P_VData + (pointer-1) * isize);
	}
	else
		return 0;
}

int FASTCALL SVectorBase::enumItems(uint * pIdx, void ** ppItem) const
{
	uint   i = *pIdx;
	if(i < count) {
		*ppItem = _USERPTR(P_VData + i * isize);
		*pIdx  = i+1;
		return 1;
	}
	else
		return 0;
}

int SVectorBase::swap(uint p1, uint p2)
{
	int    ok = 1;
	if(p1 != p2) {
		const uint c = getCount();
		if(p1 < c && p2 < c) {
			void * ptr1 = (P_VData + p1 * isize);
			void * ptr2 = (P_VData + p2 * isize);
			memswap(ptr1, ptr2, isize);
		}
		else
			ok = 0;
	}
	else
		ok = -1;
	return ok;
}

void SVectorBase::reverse(uint pos, uint numItems)
{
	const  uint last_pos = pos+numItems-1;
	uint   i = numItems/2;
	if(i && pos < count && last_pos < count)
		do {
			--i;
			memswap(at(pos+i), at(last_pos-i), isize);
		} while(i);
}

int SVectorBase::moveItem(uint pos, int dir, uint * pNewPos)
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

int SVectorBase::shuffle()
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

int SVectorBase::Helper_Cmp(const void * i1, const void * i2, CompFunc fcmp, void * pExtraData) const
{
	return (VFlags & aryPtrContainer) ? fcmp(*static_cast<const void * const *>(i1), *static_cast<const void * const *>(i2), pExtraData) : fcmp(i1, i2, pExtraData);
}

static int substFCMP(const void * i1, const void * i2)
{
#ifndef _WIN32_WCE
	SlThreadLocalArea & tla = SLS.GetTLA();
	return (tla.SAry_PtrContainer ?
		tla.SAry_OrgFCMP(*static_cast<const void * const *>(i1), *static_cast<void * const *>(i2), tla.SAry_SortExtraData) :
		tla.SAry_OrgFCMP(i1, i2, tla.SAry_SortExtraData));
#else
	return (SAry_PtrContainer ?
		SAry_OrgFCMP(*(void **)i1, *(void **)i2, SAry_SortExtraData) :
		SAry_OrgFCMP(i1, i2, SAry_SortExtraData));
#endif
}

void SVectorBase::sort(CompFunc fcmp, void * pExtraData /*=0*/)
{
	if(count > 1) {
#ifndef _WIN32_WCE
		SlThreadLocalArea & tla = SLS.GetTLA();
		tla.SAry_PtrContainer  = (VFlags & aryPtrContainer);
		tla.SAry_OrgFCMP       = fcmp;
		tla.SAry_SortExtraData = pExtraData;
#else
		SAry_PtrContainer  = (VFlags & aryPtrContainer);
		SAry_OrgFCMP       = fcmp;
		SAry_SortExtraData = pExtraData;
#endif
		qsort(P_VData, count, isize, substFCMP);
	}
	VFlags |= arySorted;
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
#define STKSIZ (8*sizeof(void *) - 2)

void SVectorBase::sort2(CompFunc fcmp, void * pExtraData /*=0*/)
{
	//
	// Note: the number of stack entries required is no more than
	// 1 + log2(num), so 30 is sufficient for any array
	//
	uint8 * mid;                  // points to middle of subarray
	uint8 * loguy, *higuy;        // traveling pointers for partition step
	size_t size;                 // size of the sub-array
	uint8 * lostk[STKSIZ];
	uint8 * histk[STKSIZ];
	const  size_t num = count;
	const  size_t width = isize;
	if(num > 1 && width > 0) {
		int    stkptr = 0; // stack for saving sub-array to be processed
		// lo and hi - ends of sub-array currently sorting
		uint8 * base = PTR8(P_VData);
		uint8 * lo = base;
		uint8 * hi = base + width * (num-1); // initialize limits
		//
		// this entry point is for pseudo-recursion calling: setting
		// lo and hi and jumping to here is like recursion, but stkptr is
		// preserved, locals aren't, so we preserve stuff on the stack
		//
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
			//
			// First we pick a partitioning element.  The efficiency of the
			// algorithm demands that we find one that is approximately the median
			// of the values, but also that we select one fast.  We choose the
			// median of the first, middle, and last elements, to avoid bad
			// performance in the face of already sorted data, or data that is made
			// up of multiple sorted runs appended together.  Testing shows that a
			// median-of-three algorithm provides better performance than simply
			// picking the middle element for the latter case.
			//
			mid = lo + (size / 2) * width; // find middle element
			// Sort the first, middle, last elements into order
			if(Helper_Cmp(lo, mid, fcmp, pExtraData) > 0)
				memswap(lo, mid, width);
			if(Helper_Cmp(lo, hi, fcmp, pExtraData) > 0)
				memswap(lo, hi, width);
			if(Helper_Cmp(mid, hi, fcmp, pExtraData) > 0)
				memswap(mid, hi, width);
			//
			// We now wish to partition the array into three pieces, one consisting
			// of elements <= partition element, one of elements equal to the
			// partition element, and one of elements > than it.  This is done
			// below; comments indicate conditions established at every step.
			//
			loguy = lo;
			higuy = hi;
			//
			// Note that higuy decreases and loguy increases on every iteration, so loop must terminate.
			//
			for(;;) {
				//
				// lo <= loguy < hi, lo < higuy <= hi,
				// A[i] <= A[mid] for lo <= i <= loguy,
				// A[i] > A[mid] for higuy <= i < hi,
				// A[hi] >= A[mid]
				//
				// The doubled loop is to avoid calling comp(mid,mid), since some
				// existing comparison funcs don't work when passed the same value for both pointers.
				//
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
				//
				// lo < loguy <= hi+1, A[i] <= A[mid] for lo <= i < loguy,
				// either loguy > hi or A[loguy] > A[mid]
				//
				do  {
					higuy -= width;
				} while(higuy > mid && Helper_Cmp(higuy, mid, fcmp, pExtraData) > 0);
				//
				// lo <= higuy < hi, A[i] > A[mid] for higuy < i < hi,
				// either higuy == lo or A[higuy] <= A[mid]
				//
				if(higuy < loguy)
					break;
				//
				// if loguy > hi or higuy == lo, then we would have exited, so
				// A[loguy] > A[mid], A[higuy] <= A[mid],
				// loguy <= hi, higuy > lo
				//
				memswap(loguy, higuy, width);
				//
				// If the partition element was moved, follow it.  Only need
				// to check for mid == higuy, since before the swap,
				// A[loguy] > A[mid] implies loguy != mid.
				//
				if(mid == higuy)
					mid = loguy;
				//
				// A[loguy] <= A[mid], A[higuy] > A[mid]; so condition at top of loop is re-established
				//
			}
			//
			// A[i] <= A[mid] for lo <= i < loguy,
			// A[i] > A[mid] for higuy < i < hi,
			// A[hi] >= A[mid]
			// higuy < loguy
			// implying:
			// higuy == loguy-1
			// or higuy == hi - 1, loguy == hi + 1, A[hi] == A[mid]
			//
			// Find adjacent elements equal to the partition element.  The
			// doubled loop is to avoid calling comp(mid,mid), since some
			// existing comparison funcs don't work when passed the same value for both pointers.
			//
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
			//
			// OK, now we have the following:
			// higuy < loguy
			// lo <= higuy <= hi
			// A[i]  <= A[mid] for lo <= i <= higuy
			// A[i]  == A[mid] for higuy < i < loguy
			// A[i]  >  A[mid] for loguy <= i < hi
			// A[hi] >= A[mid]
			//
			// We've finished the partition, now we want to sort the subarrays [lo, higuy] and [loguy, hi].
			// We do the smaller one first to minimize stack usage.
			// We only sort arrays of length 2 or more.
			//
			if(higuy - lo >= hi-loguy) {
				if(lo < higuy) {
					lostk[stkptr] = lo;
					histk[stkptr] = higuy;
					++stkptr;
				} // save big recursion for later
				if(loguy < hi) {
					lo = loguy;
					goto recurse; // do small recursion
				}
			}
			else {
				if(loguy < hi) {
					lostk[stkptr] = loguy;
					histk[stkptr] = hi;
					++stkptr; // save big recursion for later
				}
				if(lo < higuy) {
					hi = higuy;
					goto recurse; // do small recursion
				}
			}
		}
		//
		// We have sorted the array, except for any pending sorts on the stack.
		// Check if there are any, and do them.
		//
		--stkptr;
		if(stkptr >= 0) {
			lo = lostk[stkptr];
			hi = histk[stkptr];
			goto recurse; // pop subarray from stack
		}
		else
			return; // all subarrays done
	}
}

double SVectorBase::sumDouble(uint offs, uint firstIdx, uint lastIdx) const
{
	double result = 0.0;
	assert(firstIdx >= 0 && firstIdx < count);
	assert(lastIdx >= 0 && lastIdx < count && lastIdx >= firstIdx);
	if(firstIdx < count && lastIdx >= firstIdx && lastIdx < count) {
		const size_t _is1 = isize;
		const size_t _is2 = isize*2;
		const size_t _is3 = isize*3;
		const size_t _is4 = isize*4;
		const size_t _is5 = isize*5;
		const size_t _is6 = isize*6;
		const size_t _is7 = isize*7;
		const uint  _cr = lastIdx - firstIdx + 1;
		const uint _c8 = (_cr & ~0x07);
		uint  i = firstIdx;
		const uint8 * p = (P_VData + offs + _is1 * firstIdx);
		for(; i < _c8; i += 8) {
			result += (*PTRDBLC(p) + *PTRDBLC(p + _is1) + *PTRDBLC(p + _is2) + *PTRDBLC(p + _is3) + *PTRDBLC(p + _is4) + *PTRDBLC(p + _is5) + *PTRDBLC(p + _is6) + *PTRDBLC(p + _is7));
			p += (_is1 * 8);
		}
		for(; i <= lastIdx; i++) {
			result += *PTRDBLC(p);
			p += _is1;
		}
	}
	return result;
}
//
// Returns:
//   >0 - если элемент idx на выходе больше, чем key,
//   <0 - если элемент idx на выходе меньше, чем key,
//   0  - если поиск завершился успешно
//
int SVectorBase::imp_bsearch(const void * key, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	int    cmp = 1;
	uint   i  = 0;
	if(count) {
		uint   lo  = 0;
		uint   up  = count - 1;
		const  uint8 * p_org = P_VData/* + ofs*/;
		//
		// Из-за популярности функции CMPF_LONG в простых массивах (без aryPtrContainer) выделяем в
		// отдельную ветку такой случай - это даст значительное ускорение.
		//
		if(fcmp == CMPF_LONG && !(VFlags & aryPtrContainer)) {
			const long test = *static_cast<const long *>(key);
			while(lo <= up) {
				const uint8 * p = p_org + (i = (lo + up) >> 1) * isize;
				cmp = CMPSIGN(*reinterpret_cast<const long *>(p+ofs), test);
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
						if(*reinterpret_cast<const long *>(p+ofs) == test)
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
				cmp = fcmp(PTR8C(_USERPTR_C(p))+ofs, key, pExtraData);
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
						if(fcmp(PTR8C(_USERPTR_C(p))+ofs, key, pExtraData) == 0)
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

int SVectorBase::bsearch(const void * key, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
	{ return (imp_bsearch(key, pIdx, fcmp, ofs, pExtraData) == 0) ? 1 : (SLibError = SLERR_ARYITEMNFOUND, 0); }
int SVectorBase::bsearch(const void * key, uint * pIdx, CompFunc fcmp) const
	{ return (imp_bsearch(key, pIdx, fcmp, 0, 0) == 0) ? 1 : (SLibError = SLERR_ARYITEMNFOUND, 0); }

int SVectorBase::isItemEq(const void * pKey, uint pos, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	if(pos < count) {
		const void * p_data = _USERPTR(P_VData + pos * isize);
		if(p_data && fcmp(PTR8C(p_data) + ofs, pKey, pExtraData) == 0)
			return 1;
	}
	return 0;
}

int SVectorBase::lsearch(const void * pcKey, uint * pIdx, CompFunc fcmp, size_t ofs, void * pExtraData /*=0*/) const
{
	uint   i = pIdx ? (*pIdx * isize) : 0;
	const  uint    lim = count * isize;
	const  uint8 * p_org = PTR8(P_VData);
	if(VFlags & aryPtrContainer) {
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
			const uint32 test = *PTR32C(pcKey);
			for(; i < lim; i += isize)
				if(*PTR32C(p_org + i) == test) {
					ASSIGN_PTR(pIdx, i / isize);
					return 1;
				}
		}
		else if(fcmp == CMPF_INT64) {
			const uint64 test = *PTR64C(pcKey);
			for(; i < lim; i += isize)
				if(*PTR64C(p_org + i) == test) {
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

int SVectorBase::ordInsert(const void * item, uint * idx, CompFunc fcmp, void * pExtraData /*=0*/)
{
	int    ok = 1;
	uint   pos = 0;
	int    cmp;
	if(count) {
		if(!(VFlags & arySorted))
			sort(fcmp, pExtraData);
		if((cmp = imp_bsearch(item, &pos, fcmp, 0, pExtraData)) == 0)
			if(VFlags & aryUnique) {
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
			VFlags |= arySorted;
		}
		else
			ok = 0;
	}
	return ok;
}
//
//
//
SVector::SVector(uint itemSize, /*uint aDelta,*/ uint o /*= O_ARRAY*/) : SVectorBase(itemSize, /*aDelta,*/ o)
{
	assert(!(o & aryEachItem));
}

SVector::SVector(uint itemSize) : SVectorBase(itemSize, O_ARRAY)
{
}

SVector::SVector(void * pData, uint itemSize, uint aCount, uint o) : SVectorBase(itemSize, o)
{
	assert(!(o & aryEachItem));
	count = aCount;
	Limit = aCount;
	P_VData = static_cast<uint8 *>(pData);
}

SVector::SVector(const SVector & rS) : SVectorBase(rS.isize, rS.VFlags)
{
	assert(!(rS.VFlags & aryEachItem));
	copy(rS);
}

SVector::~SVector()
{
	if(VFlags & aryDataOwner && P_VData) // (&& P_VData) ради ускорения //
		freeAll();
}

int FASTCALL SVector::copy(const SVector & rS)
{
	assert(!(rS.VFlags & aryEachItem));
	freeAll();
    return SVectorBase::CopyS(rS);
}

SVector & FASTCALL SVector::operator = (const SVector & rS)
{
	copy(rS);
	return *this;
}

int FASTCALL SVector::IsEqual(const SVector & rS) const
{
	const uint16 tf = (aryPtrContainer|aryEachItem);
	if(!(VFlags & tf) && !(rS.VFlags & tf))
		if(count == rS.count && isize == rS.isize)
			return BIN(memcmp(P_VData, rS.P_VData, isize * count) == 0);
	return 0;
}

int FASTCALL SVector::atFree(uint idx)
{
	if(idx < count) {
		uint8 * org = P_VData;
		void * item = org + idx * isize;
		count--;
		if(idx < count)
			memmove(item, PTR8(item) + isize, (count-idx) * isize);
		// @v10.7.3 shrink();
		return 1;
	}
	else
		return (SLibError = SLERR_BOUNDS, 0);
}

int SVector::freeLast()
{
	return count ? atFree(count-1) : (SLibError = SLERR_BOUNDS, 0);
}

void SVector::freeAll()
{
	if(P_VData) //@speedcritical
		ZFREE(P_VData);
	count = 0;
	pointer = 0;
	Limit = 0;
}

void SVector::clear()
{
	count = pointer = 0;
}

int SVector::freeChunk(uint lowPos, uint uppPos)
{
	int    ok = 0;
	const  uint org_count = count;
	if(lowPos <= uppPos && lowPos < org_count) {
		const uint upp_pos = MIN(uppPos, org_count-1);
		const uint _fc = (upp_pos - lowPos + 1);
		assert(_fc <= org_count);
		if(_fc == org_count) {
			freeAll();
		}
		else {
			uint8 * p_org = P_VData;
			void * p_low_item = p_org + lowPos * isize;
			count -= _fc;
			if(upp_pos < (org_count-1)) {
				memmove(p_low_item, PTR8(p_low_item) + _fc * isize, (org_count-1-upp_pos) * isize);
			}
			// @v10.7.3 shrink();
		}
		ok = 1;
	}
	else
		SLibError = SLERR_BOUNDS;
	return ok;
}

int SVector::Write(SFile & rFile, long) const
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

int SVector::Read(SFile & rFile, long)
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
			THROW(rFile.Read(temp_buf.vptr(), item_size));
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
//
//
//
SArray::SArray(uint itemSize, uint o) : SVectorBase(itemSize, o)
{
}

SArray::SArray(uint itemSize) : SVectorBase(itemSize, O_ARRAY)
{
}

SArray::SArray(void * pData, uint itemSize, uint aCount, uint o) : SVectorBase(itemSize, o)
{
	count = aCount;
	Limit = aCount;
	P_VData = static_cast<uint8 *>(pData);
}

SArray::SArray(const SArray & rS) : SVectorBase(rS.isize, rS.VFlags)
{
	copy(rS);
}

SArray::SArray(const SVector & rS) : SVectorBase(rS.getItemSize(), rS.getFlags())
{
	SVectorBase::CopyS(rS);
}

SArray::~SArray()
{
	if(VFlags & aryDataOwner && P_VData)
		freeAll();
}

SArray & FASTCALL SArray::operator = (const SArray & rS)
{
	copy(rS);
	return *this;
}

int FASTCALL SArray::copy(const SArray & rS)
{
	freeAll();
    return SVectorBase::CopyS(rS);
}

int FASTCALL SArray::IsEqual(const SArray & src) const
{
	const uint16 tf = (aryPtrContainer|aryEachItem);
	if(!(VFlags & tf) && !(src.VFlags & tf))
		if(count == src.count && isize == src.isize)
			return BIN(memcmp(P_VData, src.P_VData, isize * count) == 0);
	return 0;
}

/*virtual*/void FASTCALL SArray::freeItem(void * pItem)
{
	if(VFlags & aryPtrContainer)
		delete pItem;
}

int FASTCALL SArray::atFree(uint idx)
{
	if(idx < count) {
		uint8 * org = P_VData;
		void * item = org + idx * isize;
		if(VFlags & aryEachItem)
			freeItem(_USERPTR(item));
		count--;
		if(idx < count)
			memmove(item, PTR8(item) + isize, (count-idx) * isize);
		// @v10.7.3 shrink();
		return 1;
	}
	else
		return (SLibError = SLERR_BOUNDS, 0);
}

int SArray::freeChunk(uint lowPos, uint uppPos)
{
	int    ok = 0;
	const  uint org_count = count;
	if(lowPos <= uppPos && lowPos < org_count) {
		const uint upp_pos = MIN(uppPos, org_count-1);
		const uint _fc = (upp_pos - lowPos + 1);
		assert(_fc <= org_count);
		if(_fc == org_count)
			freeAll();
		else {
			uint8 * p_org = P_VData;
			void * p_low_item = p_org + lowPos * isize;
			if(VFlags & aryEachItem) {
				for(uint i = lowPos; i <= upp_pos; i++) {
					void * p_item = p_org + i * isize;
					freeItem(_USERPTR(p_item));
				}
			}
			count -= _fc;
			if(upp_pos < (org_count-1)) {
				memmove(p_low_item, PTR8(p_low_item) + _fc * isize, (org_count-1-upp_pos) * isize);
			}
			// @v10.7.3 shrink();
		}
		ok = 1;
	}
	else
		SLibError = SLERR_BOUNDS;
	return ok;
}

int SArray::freeLast()
{
	return count ? atFree(count-1) : (SLibError = SLERR_BOUNDS, 0);
}

void SArray::clear()
{
	if(P_VData && VFlags & aryEachItem) {
		uint8 * p_org = P_VData;
		const uint lim = count * isize;
		for(uint i = 0; i < lim; i += isize)
			freeItem(_USERPTR(p_org + i));
	}
	count = pointer = 0;
}

void SArray::freeAll()
{
	if(P_VData) {
		uint8 * p_org = P_VData;
		const uint lim = count * isize;
		if(VFlags & aryEachItem) {
			for(uint i = 0; i < lim; i += isize)
				freeItem(_USERPTR(p_org + i));
		}
		ZFREE(P_VData);
	}
	count = 0;
	pointer = 0;
	Limit = 0;
}
//
// @construction
//
/*double SArray::calcSumDouble(size_t offs) const
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
}*/

#ifndef _WIN32_WCE // {

int SArray::Write(SFile & rFile, long) const
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

int SArray::Read(SFile & rFile, long)
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
SCollection::SCollection() : SArray(sizeof(void *), O_COLLECTION)
{
}

SCollection::SCollection(uint o) : SArray(sizeof(void *), o)
{
}

int FASTCALL SCollection::Helper_CreateNewItem(void * pNewItem, uint * pPos)
{
	if(pNewItem && insert(pNewItem)) {
		ASSIGN_PTR(pPos, count-1);
		return 1;
	}
	else
		return 0;
}

int FASTCALL SCollection::Helper_CreateNewItem(void * pNewItem)
{
	return BIN(pNewItem && insert(pNewItem));
}
//
//
//
LAssoc::LAssoc() 
{
	Key = 0;
	Val = 0;
}

LAssoc::LAssoc(int32 key, int32 val) 
{
	Key = key;
	Val = val;
}

LAssoc::LAssoc(const LAssoc & rS) 
{
	Key = rS.Key;
	Val = rS.Val;
}

int FASTCALL LAssoc::operator == (const LAssoc & s) const
{
	return BIN(Key == s.Key && Val == s.Val);
}

LAssocArray::LAssocArray() : /*TSArray*/TSVector <LAssoc>()
{
}

LAssocArray::LAssocArray(const LAssocArray & s) : /*TSArray*/TSVector <LAssoc> (s)
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
	else {
		for(uint i = 0; i < count; i++)
			if(!(at(i) == s.at(i)))
				return 0;
		return 1;
	}
}

int LAssocArray::CheckUnique(long key, int binary) const
{
	const int is_found = binary ? BSearch(key, 0, 0) : Search(key, 0, 0);
	return is_found ? 0 : 1;
}

long FASTCALL LAssocArray::Count(long key) const
{
	long   c = 0;
	for(uint i = 0; i < getCount(); i++)
		if(at(i).Key == key)
			c++;
	return c;
}

int LAssocArray::GetKeyList(LongArray & rList) const
{
	rList.clear();
	const  uint _c = getCount();
	for(uint i = 0; i < _c; i++) { rList.add(at(i).Key); }
	rList.sortAndUndup();
	return rList.getCount() ? 1 : -1;
}

int LAssocArray::GetValList(LongArray & rList) const
{
	rList.clear();
	const  uint _c = getCount();
	for(uint i = 0; i < _c; i++) { rList.add(at(i).Val); }
	rList.sortAndUndup();
	return rList.getCount() ? 1 : -1;
}

int LAssocArray::GetListByKey(long key, LongArray & rList) const
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

int LAssocArray::GetListByVal(long val, LongArray & rList) const
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

int LAssocArray::Add(long key, long val)
{
	return Add(key, val, 0);
}

int LAssocArray::Add(long key, long val, uint * pPos, int inOrder)
{
	int    ok = 1;
	uint   pos = 0;
	LAssoc item(key, val);
	if(inOrder)
		ok = BIN(ordInsert(&item, &pos, CMPF_LONG));
	else {
		ok = BIN(insert(&item));
		pos = getCount()-1;
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int LAssocArray::AddUnique(long key, long val, uint * pPos, int inOrder)
{
	const int is_found = inOrder ? BSearch(key, 0, pPos) : Search(key, 0, pPos);
	return is_found ? 0 : Add(key, val, pPos, inOrder);
}

int FASTCALL LAssocArray::IncrementValueByKey(long key)
{
	int    ok = 1;
	uint   pos = 0;
	long   c = 0;
	if(Search(key, &c, &pos))
		at(pos).Val = (c+1);
	else {
		Add(key, 1);
		ok = 2;
	}
	return ok;
}

int LAssocArray::Update(long key, long val, int binary)
{
	uint   pos = 0;
	const  int is_found = binary ? BSearch(key, 0, &pos) : Search(key, 0, &pos);
	if(is_found) {
		at(pos).Val = val;
		return 1;
	}
	else
		return Add(key, val, &pos, binary);
}

int LAssocArray::Remove(long key, int binary)
{
	uint   pos = 0;
	const  int is_found = binary ? BSearch(key, 0, &pos) : Search(key, 0, &pos);
	if(is_found) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

int LAssocArray::Search(long key, long * pVal, uint * pPos/*, int binary*/) const
{
	uint   pos = 0;
	//int    ok = binary ? bsearch(&key, &pos, CMPF_LONG) : lsearch(&key, &pos, CMPF_LONG);
	int    ok = lsearch(&key, &pos, CMPF_LONG);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LAssocArray::BSearch(long key, long * pVal, uint * pPos) const
{
	uint   pos = 0;
	int    ok = bsearch(&key, &pos, CMPF_LONG);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LAssocArray::SearchByVal(long val, long * pKey, uint * pPos) const
{
	uint   pos = 0;
	int    ok  = lsearch(&val, &pos, CMPF_LONG, sizeof(long));
	if(ok) {
		ASSIGN_PTR(pKey, at(pos).Key);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LAssocArray::SearchPair(long key, long val, uint * pPos) const
{
	LAssoc k(key, val);
	// @v11.1.12 return lsearch(&k, pPos, PTR_CMPFUNC(_2long));
	return lsearch(&k, pPos, CMPF_INT64); // @v11.1.12
}

IMPL_CMPFUNC(LAssoc_Val, i1, i2)
{
	const LAssoc * p1 = static_cast<const LAssoc *>(i1);
	const LAssoc * p2 = static_cast<const LAssoc *>(i2);
	return (p1->Val < p2->Val) ? -1 : ((p1->Val > p2->Val) ? +1 : 0);
}

IMPL_CMPFUNC(LAssoc_KeyVal, i1, i2)
{
	const LAssoc * p1 = static_cast<const LAssoc *>(i1);
	const LAssoc * p2 = static_cast<const LAssoc *>(i2);
	RET_CMPCASCADE2(p1, p2, Key, Val);
}

IMPL_CMPFUNC(LAssoc_ValKey, i1, i2)
{
	const LAssoc * p1 = static_cast<const LAssoc *>(i1);
	const LAssoc * p2 = static_cast<const LAssoc *>(i2);
	RET_CMPCASCADE2(p1, p2, Val, Key);
}

void LAssocArray::Sort() { SVector::sort(CMPF_LONG); }
void LAssocArray::SortByVal() { SVector::sort(PTR_CMPFUNC(LAssoc_Val)); }
void LAssocArray::SortByKeyVal() { SVector::sort(PTR_CMPFUNC(LAssoc_KeyVal)); }
void LAssocArray::SortByValKey() { SVector::sort(PTR_CMPFUNC(LAssoc_ValKey)); }

int LAssocArray::CheckFlag(long key, long mask, int binary) const
{
	long   val = 0;
	const  int is_found = binary ? BSearch(key, &val, 0) : Search(key, &val, 0);
	return BIN(is_found && (val & mask));
}
//
//
//
int FASTCALL LLAssoc::operator == (const LLAssoc & s) const
{
	return BIN(Key == s.Key && Val == s.Val);
}

LLAssocArray::LLAssocArray() : TSVector <LLAssoc>()
{
}

LLAssocArray::LLAssocArray(const LLAssocArray & s) : TSVector <LLAssoc> (s)
{
}

LLAssocArray & FASTCALL LLAssocArray::operator = (const LLAssocArray & s)
{
	copy(s);
	return *this;
}

int FASTCALL LLAssocArray::operator == (const LLAssocArray & s) const
{
	int    ok = 1;
	if(count != s.count)
		ok = 0;
	else {
		for(uint i = 0; ok && i < count; i++)
			if(!(at(i) == s.at(i)))
				ok = 0;
	}
	return ok;
}

int LLAssocArray::CheckUnique(int64 key, int binary) const
{
	return (binary ? BSearch(key, 0, 0) : Search(key, 0, 0)) ? 0 : 1;
}

long FASTCALL LLAssocArray::Count(int64 key) const
{
	long   c = 0;
	for(uint i = 0; i < getCount(); i++)
		if(at(i).Key == key)
			c++;
	return c;
}

int LLAssocArray::GetListByKey(int64 key, Int64Array & rList) const
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

int LLAssocArray::GetListByVal(int64 val, Int64Array & rList) const
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

int LLAssocArray::Add(int64 key, int64 val, uint * pPos, int inOrder)
{
	int    ok = 1;
	uint   pos = 0;
	LLAssoc item;
	item.Key = key;
	item.Val = val;
	if(inOrder)
		ok = BIN(ordInsert(&item, &pos, CMPF_INT64));
	else {
		ok = BIN(insert(&item));
		pos = getCount()-1;
	}
	ASSIGN_PTR(pPos, pos);
	return ok;
}

int LLAssocArray::AddUnique(int64 key, int64 val, uint * pPos, int inOrder)
{
	const int is_found = inOrder ? BSearch(key, 0, pPos) : Search(key, 0, pPos);
	return is_found ? 0 : Add(key, val, pPos, inOrder);
}

int LLAssocArray::Update(int64 key, int64 val, int binary)
{
	uint   pos = 0;
	const  int is_found = binary ? BSearch(key, 0, &pos) : Search(key, 0, &pos);
	if(is_found) {
		at(pos).Val = val;
		return 1;
	}
	else
		return Add(key, val, &pos, binary);
}

int LLAssocArray::Remove(int64 key, int binary)
{
	uint   pos = 0;
	const  int is_found = binary ? BSearch(key, 0, &pos) : Search(key, 0, &pos);
	if(is_found) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

int LLAssocArray::Search(int64 key, int64 * pVal, uint * pPos/*, int binary*/) const
{
	uint   pos = 0;
	//int    ok = binary ? bsearch(&key, &pos, CMPF_INT64) : lsearch(&key, &pos, CMPF_INT64);
	int    ok = lsearch(&key, &pos, CMPF_INT64);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LLAssocArray::BSearch(int64 key, int64 * pVal, uint * pPos) const
{
	uint   pos = 0;
	//int    ok = binary ? bsearch(&key, &pos, CMPF_INT64) : lsearch(&key, &pos, CMPF_INT64);
	int    ok = bsearch(&key, &pos, CMPF_INT64);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LLAssocArray::SearchByVal(int64 val, int64 * pKey, uint * pPos) const
{
	uint   pos = 0;
	int    ok  = lsearch(&val, &pos, CMPF_INT64, sizeof(int64));
	if(ok) {
		ASSIGN_PTR(pKey, at(pos).Key);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

int LLAssocArray::SearchPair(int64 key, int64 val, uint * pPos) const
{
	LLAssoc k;
	k.Key = key;
	k.Val = val;
	return lsearch(&k, pPos, PTR_CMPFUNC(_2int64));
}

void LLAssocArray::Sort()
{
	SVector::sort(CMPF_INT64);
}
//
//
//
RAssocArray::RAssocArray() : TSVector <RAssoc>()
{
}

RAssocArray::RAssocArray(const RAssocArray & s) : TSVector <RAssoc> (s)
{
}

RAssocArray & FASTCALL RAssocArray::operator = (const RAssocArray & s)
{
	copy(s);
	return *this;
}

int RAssocArray::Search(long key, double * pVal, uint * pPos, int binary) const
{
	uint   pos = 0;
	int    ok = binary ? bsearch(&key, &pos, CMPF_LONG) : lsearch(&key, &pos, CMPF_LONG);
	if(ok) {
		ASSIGN_PTR(pVal, at(pos).Val);
		ASSIGN_PTR(pPos, pos);
	}
	return ok;
}

double RAssocArray::Get(long key, int binary) const
{
	double val = 0.0;
	return (Search(key, &val, 0, binary) > 0) ? val : 0;
}

int FASTCALL RAssocArray::Has(long key) const
{
	long   _key = key;
	return lsearch(&_key, 0, CMPF_LONG);
}

int RAssocArray::GetList(LongArray & rList, int unique) const
{
	int    ok = -1;
	for(uint i = 0; i < getCount(); i++) {
		if(unique)
			rList.addUnique(at(i).Key);
		else
			rList.add(at(i).Key);
		ok = 1;
	}
	return ok;
}

int RAssocArray::SearchMaxVal(uint * pPos) const
{
	int    ok = 0;
	double max_val = -SMathConst::Max;
	uint   max_val_pos = 0;
	for(uint i = 0; i < getCount(); i++) {
		const RAssoc & r_item = at(i);
		if(max_val < r_item.Val) {
			max_val = r_item.Val;
			max_val_pos = i;
			ok = 1;
		}
	}
	ASSIGN_PTR(pPos, max_val_pos);
	return ok;
}

void RAssocArray::Scale(double rate)
{
	for(uint i = 0; i < getCount(); i++)
		at(i).Val *= rate;
}

int RAssocArray::Distribute(double amount, long flags, int prec, RAssocArray & rResult) const
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

double RAssocArray::GetTotal() const
{
	double total = 0.0;
	for(uint i = 0; i < getCount(); i++)
		total += at(i).Val;
	return total;
}

int RAssocArray::Add(long key, double val, int additive, int binary)
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

int RAssocArray::Add(const RAssocArray & rList, int additive, int binary)
{
	for(uint i = 0; i < rList.getCount(); i++) {
		const RAssoc & r_item = rList.at(i);
		Add(r_item.Key, r_item.Val, additive, binary);
	}
	return 1;
}

int RAssocArray::Remove(long key, int binary)
{
	uint   pos = 0;
	if(Search(key, 0, &pos, binary)) {
		atFree(pos);
		return 1;
	}
	else
		return 0;
}

void RAssocArray::SortByKey()
{
	sort(CMPF_LONG);
}

IMPL_CMPFUNC(RAssoc_Val, i1, i2)
{
	const RAssoc * p1 = static_cast<const RAssoc *>(i1);
	const RAssoc * p2 = static_cast<const RAssoc *>(i2);
	return (p1->Val < p2->Val) ? -1 : ((p1->Val > p2->Val) ? +1 : 0);
}

IMPL_CMPFUNC(RAssoc_Val_Rev, i1, i2)
{
	const RAssoc * p1 = static_cast<const RAssoc *>(i1);
	const RAssoc * p2 = static_cast<const RAssoc *>(i2);
	return (p1->Val < p2->Val) ? +1 : ((p1->Val > p2->Val) ? -1 : 0);
}

void RAssocArray::SortByVal()
{
	sort(PTR_CMPFUNC(RAssoc_Val));
}

void RAssocArray::SortByValRev()
{
	sort(PTR_CMPFUNC(RAssoc_Val_Rev));
}
//
//
//
RPairArray::RPairArray() : TSVector <RPair> ()
{
}

RPairArray::RPairArray(const RPairArray & rS)  : TSVector <RPair> (rS)
{
}

RPairArray & FASTCALL RPairArray::operator = (const RPairArray & rS)
{
	SVector::copy(rS); // @v9.8.4 SArray-->SVector
	return *this;
}

RPair & FASTCALL RPairArray::operator [](size_t i) const
{
	return at(i);
}

IMPL_CMPFUNC(RPairX, i1, i2) { RET_CMPCASCADE2(static_cast<const RPair *>(i1), static_cast<const RPair *>(i2), X, Y); }
IMPL_CMPFUNC(RPairY, i1, i2) { RET_CMPCASCADE2(static_cast<const RPair *>(i1), static_cast<const RPair *>(i2), Y, X); }

void RPairArray::SortByX()
{
	sort(PTR_CMPFUNC(RPairX));
}

void RPairArray::SortByY()
{
	sort(PTR_CMPFUNC(RPairY));
}

#ifndef _WIN32_WCE // {
//
//
//
UUIDAssocArray::UUIDAssocArray() : TSVector <UUIDAssoc>()
{
}

int UUIDAssocArray::Add(long key, const S_GUID & rVal, uint * pPos)
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

int UUIDAssocArray::Search(long key, S_GUID * pVal, uint * pPos) const
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

int UUIDAssocArray::SearchVal(const S_GUID_Base & rVal, long * pKey, uint * pPos) const
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

int UUIDAssocArray::GetMaxKey(long * pMaxKey, uint * pPos) const
{
	int    ok = -1;
	long   max_key = MINLONG;
	uint   max_key_pos = 0;
	for(uint i = 0; i < getCount(); i++) {
		const UUIDAssoc & r_item = at(i);
		if(max_key < r_item.Key) {
			max_key = r_item.Key;
			max_key_pos = i;
			if(ok < 0)
				ok = 1;
		}
		else if(max_key == r_item.Key && i)
			ok = 2;
	}
	if(ok > 0) {
		ASSIGN_PTR(pMaxKey, max_key);
		ASSIGN_PTR(pPos, max_key_pos);
	}
	return ok;
}

#endif // } _WIN32_WCE
//
//
//
void SV_Uint32::Init()
{
	P_Data = 0;
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
		P_Data = static_cast<uint32 *>(SAlloc::C((c+1), sizeof(uint32)));
		if(P_Data == 0)
			return 0;
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
	P_Data = static_cast<uint32 *>(SAlloc::R(P_Data, (c+2) * sizeof(uint32)));
	if(P_Data) {
		++c;
		P_Data[c] = item;
		P_Data[0] = c;
		return 1;
	}
	else
		return 0;
}

int SV_Uint32::Add(uint count, const uint32 * pSrc)
{
	uint   c = GetCount();
	P_Data = static_cast<uint32 *>(SAlloc::R(P_Data, (c+count+1) * sizeof(uint32)));
	if(P_Data) {
		memcpy(P_Data+c+1, pSrc, count * sizeof(uint32));
		P_Data[0] = c+count;
		return 1;
	}
	else
		return 0;
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
		P_Data = static_cast<uint32 *>(SAlloc::R(P_Data, (c+1) * sizeof(uint32)));
		if(P_Data) {
			P_Data[0] = c;
			rBuf.Read(P_Data+1, sizeof(uint32) * c);
			return 1;
		}
		else
			return 0;
	}
	else
		Destroy();
	return 1;
}
//
//
//
StrAssocArray::StrAssocArray() : LAssocArray(), SStrGroup(), TextCmpProc(0)
{
}

StrAssocArray::StrAssocArray(const StrAssocArray & rS) : SStrGroup()
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
	LAssocArray::copy(rS);
	SStrGroup::CopyS(rS);
	TextCmpProc = rS.TextCmpProc;
	ParentList = rS.ParentList;
	return 1;
}

int FASTCALL StrAssocArray::IsEqual(const StrAssocArray & s) const
{
	int    ok = 0;
	uint   c = LAssocArray::getCount();
	if(c == s.LAssocArray::getCount()) {
		ok = 1;
		for(uint i = 0; ok && i < c; i++) {
			Item b = Get(i);
			Item sb = s.Get(i);
			if(b.Id == sb.Id && b.ParentId == sb.ParentId) {
				if(b.Txt != 0 && sb.Txt != 0) {
					if(!sstreq(b.Txt, sb.Txt))
						ok = 0;
				}
				else if(b.Txt || sb.Txt)
					ok = 0;
			}
		}
	}
	return ok;
}

uint StrAssocArray::getPointer() const { return LAssocArray::getPointer(); }
uint StrAssocArray::setPointer(uint p) { return LAssocArray::setPointer(p); }
uint StrAssocArray::incPointer() { return LAssocArray::incPointer(); }

int FASTCALL StrAssocArray::HasChild(long id) const
{
	return (id == 0) ? BIN(LAssocArray::getCount()) : ParentList.SearchByVal(id, 0, 0);
}

int StrAssocArray::GetParent(long id, long * pParentId) const
{
	long   par_id = 0;
	int    ok = ParentList.Search(id, &par_id, 0) ? 1 : -1;
	ASSIGN_PTR(pParentId, par_id);
	return ok;
}

int StrAssocArray::GetIdList(LongArray & rList) const
{
	int    ok = -1;
	rList.clear();
	const uint c = LAssocArray::getCount();
	if(c) {
		for(uint i = 0; i < c; i++) {
			const long id = LAssocArray::at(i).Key;
			rList.add(id);
		}
		rList.sortAndUndup();
		ok = 1;
	}
	return ok;
}

int StrAssocArray::GetListByParent(long parentId, int recursive, LongArray & rList) const
{
	int    ok = 1;
	LongArray temp_list;
	if(parentId) {
		const uint c = LAssocArray::getCount();
		for(uint i = 0; i < c; i++) {
			const long id = LAssocArray::at(i).Key;
			if(ParentList.SearchPair(id, parentId, 0) > 0)
				temp_list.add(id); // @v10.7.9 addUnique-->add
		}
	}
	else {
		const uint c = LAssocArray::getCount();
		for(uint i = 0; i < c; i++) {
			const  long id = LAssocArray::at(i).Key;
			long   p = 0;
			if(!ParentList.Search(id, &p, 0) || p == 0)
				temp_list.add(id); // @v10.7.9 addUnique-->add
		}
	}
	temp_list.sortAndUndup(); // @v10.7.9 
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

int StrAssocArray::AddFast(long id, const char * pStr)
{
	LAssoc assc(id, 0);
	if(!isempty(pStr)) {
		uint   str_pos = 0;
		AddS(pStr, &str_pos);
		assc.Val = str_pos;
	}
	else
		assc.Val = 0;
	return BIN(LAssocArray::insert(&assc));
}

int StrAssocArray::AddFast(long id, long parentId, const char * pStr)
{
	LAssoc assc(id, 0);
	if(!isempty(pStr)) {
		uint   str_pos = 0;
		AddS(pStr, &str_pos);
		assc.Val = str_pos;
	}
	else
		assc.Val = 0;
	int    ok = BIN(LAssocArray::insert(&assc));
	if(ok && parentId)
		ok = ParentList.Update(id, parentId);
	return ok;
}

int StrAssocArray::Add(long id, const char * pStr, int replaceDup)
{
	int    ok = 0;
	uint   pos = 0, str_pos = 0;
	if(replaceDup != -100 && Search(id, &pos)) {
		LAssoc & r_assc = LAssocArray::at(pos);
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
			LAssoc assc(id, 0);
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				assc.Val = str_pos;
			}
			else
				assc.Val = 0;
			ok = LAssocArray::insert(&assc) ? 3 : 0;
		}
		else
			ok = -1;
	}
	else {
		LAssoc assc(id, 0);
		if(!isempty(pStr)) {
			AddS(pStr, &str_pos);
			assc.Val = str_pos;
		}
		else
			assc.Val = 0;
		ok = BIN(LAssocArray::insert(&assc));
	}
	return ok;
}

int StrAssocArray::Add(long id, long parentId, const char * pStr, int replaceDup /*= 1*/)
{
	int    ok = 0;
	uint   pos = 0, str_pos = 0;
	if(replaceDup != -100 && Search(id, &pos)) {
		LAssoc & r_assc = LAssocArray::at(pos);
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
			LAssoc assc(id, 0);
			if(!isempty(pStr)) {
				AddS(pStr, &str_pos);
				assc.Val = str_pos;
			}
			else
				assc.Val = 0;
			ok = LAssocArray::insert(&assc) ? 3 : 0;
			// @todo Вставить родительский идентификатор (logic)
		}
		else
			ok = -1;
	}
	else {
		LAssoc assc(id, 0);
		if(!isempty(pStr)) {
			AddS(pStr, &str_pos);
			assc.Val = str_pos;
		}
		else
			assc.Val = 0;
		ok = BIN(LAssocArray::insert(&assc));
		if(ok && parentId)
			ok = ParentList.Update(id, parentId);
	}
	return ok;
}

int StrAssocArray::UpdateByPos(uint pos, long newId)
{
	if(pos < LAssocArray::getCount()) {
		LAssocArray::at(pos).Key = newId;
		return 1;
	}
	else
		return 0;
}

int StrAssocArray::Swap(uint p1, uint p2)
{
	return LAssocArray::swap(p1, p2);
}

int StrAssocArray::Move(uint idxFrom, uint idxTo)
{
	int    ok = 1;
	const  uint _c = LAssocArray::getCount();
	if(idxFrom < _c && idxTo <= _c) {
		if(idxFrom == idxTo || (idxTo == _c && idxFrom == (_c-1))) 
			ok = -1;
		else {
			/*
				Следующие действия весьма дорогие с точки зрения производительности.
				Было бы правильнее оценить смещение от 0 до idxFrom и idxTo а также расстояние
				между idxFrom и idxTo. На основании этого анализа можно было бы принять решение
				о целесообразности использования нескольких Swap()
			*/
			LAssoc temp = LAssocArray::at(idxFrom);
			LAssocArray::atFree(idxFrom);
			LAssocArray::atInsert(idxTo, &temp);
		}
	}
	else
		ok = 0;
	return ok;
}

int StrAssocArray::Remove(long id)
{
	int    ok = LAssocArray::Remove(id) ? 1 : -1;
	ParentList.Remove(id);
	return ok;
}

StrAssocArray & StrAssocArray::Z()
{
	LAssocArray::clear();
	ParentList.clear();
	ClearS();
	return *this;
}

void StrAssocArray::ClearParents()
{
	ParentList.clear();
}

int StrAssocArray::Pack()
{
	int    ok = -1;
	if(Pool.getDataLen()) {
		void * p_pack_handle = Pack_Start();
		if(p_pack_handle) {
			const uint c = LAssocArray::getCount();
			for(uint i = 0; i < c; i++) {
				LAssoc & r_assc = LAssocArray::at(i);
				uint   _pos = r_assc.Val;
				Pack_Replace(p_pack_handle, _pos);
				r_assc.Val = static_cast<long>(_pos);
			}
			Pack_Finish(p_pack_handle);
			ok = 1;
		}
		else
			ok = 0;
	}
	return ok;
}

int StrAssocArray::Helper_RemoveRecursion(long parentId, LongArray & rRecurList, LongArray & rPassedList, LongArray * pBadList)
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
				CALLPTRMEMB(pBadList, add(_id));
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

int StrAssocArray::RemoveRecursion(LongArray * pBadList)
{
	int    ok = -1;
	LongArray recur_list;
	const uint _c = LAssocArray::getCount();
	if(_c) {
		LongArray passed_list;
		ok = Helper_RemoveRecursion(0, recur_list, passed_list, pBadList);
		passed_list.sortAndUndup();
		for(uint i = 0; i < _c; i++) {
			const long _id = LAssocArray::at(i).Key;
			if(!passed_list.bsearch(_id)) {
				recur_list.clear();
				int r = Helper_RemoveRecursion(LAssocArray::at(i).Key, recur_list, passed_list, pBadList);
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
	if(pos < LAssocArray::getCount()) {
		const LAssoc & r_assc = LAssocArray::at(pos);
		tpos = static_cast<uint>(r_assc.Val);
		if(!tpos || tpos >= Pool.getDataLen())
			tpos = 0;
	}
	return tpos;
}

int StrAssocArray::GetTextByPos(uint textPos, SString & rBuf) const
{
	int    ok = 1;
	if(textPos && textPos < Pool.getDataLen()) {
		rBuf = (Pool.getBuf() + textPos);
	}
	else {
		rBuf.Z();
		ok = 0;
	}
	return ok;
}

long FASTCALL StrAssocArray::GetKeyByIdx(uint pos) const
{
	return (pos < LAssocArray::getCount()) ? LAssocArray::at(pos).Key : 0;
}

StrAssocArray::Item FASTCALL StrAssocArray::Get(uint pos) const
{
	Item   item;
	item.Id = 0;
	item.ParentId = 0;
	item.Txt = 0;
	if(pos < LAssocArray::getCount()) {
		const LAssoc & r_assc = LAssocArray::at(pos);
		item.Id = r_assc.Key;
		uint   pos2 = static_cast<uint>(r_assc.Val);
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
	if(pos < LAssocArray::getCount()) {
		const LAssoc & r_assc = LAssocArray::at(pos);
		item.Id = r_assc.Key;
		uint   pos2 = static_cast<uint>(r_assc.Val);
		if(pos2 && pos2 < Pool.getDataLen())
			item.Txt = Pool.getBuf() + pos2;
	}
	return item;
}

int StrAssocArray::AtFree(uint pos)
{
	if(pos < LAssocArray::getCount()) {
		ParentList.Remove(LAssocArray::at(pos).Key);
		LAssocArray::atFree(pos);
		return 1;
	}
	else
		return (SLibError = SLERR_BOUNDS, 0);
}

int StrAssocArray::Search(long id, uint * pPos) const
{
	return LAssocArray::Search(id, 0, pPos);
}

int FASTCALL StrAssocArray::Search(long id) const
{
	return LAssocArray::Search(id, 0, 0);
}

int StrAssocArray::SearchByText(const char * pTxt, int ignoreCase, uint * pPos) const
{
	uint   pos = 0, assc_pos;
	while(Pool.search(pTxt, &pos, ignoreCase)) {
		if(LAssocArray::lsearch(&pos, &(assc_pos = 0), CMPF_LONG, offsetof(LAssoc, Val))) {
			ASSIGN_PTR(pPos, assc_pos);
			return 1;
		}
		else {
			size_t _len = Pool.getLen(pos);
			uint   _dlen = Pool.getDelimLen();
			pos += (_len + _dlen);
		}
	}
	return 0;
}

int StrAssocArray::SearchByText(const char * pTxt, CompFunc fcmp, uint * pPos) const
{
	if(pTxt) {
		const  char * p_pool = Pool.getBuf();
		if(p_pool) {
			uint   last_pos = DEREFPTRORZ(pPos);
			for(uint i = last_pos; i < LAssocArray::getCount(); i++) {
				uint   str_offs = static_cast<uint>(LAssocArray::at(i).Val);
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

int StrAssocArray::GetText(long id, SString & rBuf) const
{
	int    ok = 0;
	long   str_pos = 0;
	uint   pos;
	rBuf.Z();
	if(LAssocArray::Search(id, &str_pos, &pos)) {
		GetS(str_pos, rBuf);
		ok = rBuf.NotEmpty() ? 1 : -1;
	}
	return ok;
}

int StrAssocArray::GetTextBinary(long id, SString & rBuf) const
{
	int    ok = 0;
	long   str_pos = 0;
	uint   pos;
	rBuf.Z();
	if(LAssocArray::BSearch(id, &str_pos, &pos)) {
		GetS(str_pos, rBuf);
		ok = rBuf.NotEmpty() ? 1 : -1;
	}
	return ok;
}

int StrAssocArray::GetMaxID(long * pId) const
{
	const uint c = LAssocArray::getCount();
	if(c) {
		long   _max = -MAXLONG;
		for(uint i = 0; i < c; i++) {
			long   key = LAssocArray::at(i).Key;
			SETMAX(_max, key);
		}
		ASSIGN_PTR(pId, _max);
		return 1;
	}
	else
		return -1;
}

int StrAssocArray::GetMaxLength(uint * pMaxLen) const
{
	uint   max_len = 0;
	const  uint c = LAssocArray::getCount();
	if(c) {
		SString temp_buf;
		for(uint i = 0; i < c; i++) {
			const long str_pos = LAssocArray::at(i).Val;
			if(GetS(str_pos, temp_buf) && temp_buf.Len() > max_len)
				max_len = temp_buf.Len();
		}
	}
	ASSIGN_PTR(pMaxLen, max_len);
	return max_len ? 1 : -1;
}

IMPL_CMPFUNC(StrAssoc_ByLength, i1, i2)
{
	StringSet * p_ss = static_cast<StringSet *>(pExtraData);
	uint   pos1 = static_cast<const LAssoc *>(i1)->Val;
	uint   pos2 = static_cast<const LAssoc *>(i2)->Val;
	char   stub[8];
	stub[0] = 0;
	size_t ss_data_len = p_ss->getDataLen();
	const char * p_str1 = (pos1 < ss_data_len) ? (p_ss->getBuf() + pos1) : stub;
	const char * p_str2 = (pos2 < ss_data_len) ? (p_ss->getBuf() + pos2) : stub;
	return cmp_ulong(sstrlen(p_str1), sstrlen(p_str2));
}

IMPL_CMPFUNC(StrAssoc_ByLength_Descend, i1, i2)
{
	StringSet * p_ss = static_cast<StringSet *>(pExtraData);
	uint   pos1 = static_cast<const LAssoc *>(i1)->Val;
	uint   pos2 = static_cast<const LAssoc *>(i2)->Val;
	char   stub[16];
	PTR32(stub)[0] = 0;
	size_t ss_data_len = p_ss->getDataLen();
	const char * p_str1 = (pos1 < ss_data_len) ? (p_ss->getBuf() + pos1) : stub;
	const char * p_str2 = (pos2 < ss_data_len) ? (p_ss->getBuf() + pos2) : stub;
	return cmp_ulong(sstrlen(p_str2), sstrlen(p_str1));
}

/*static*/int StrAssocArray::Helper_CmpFunc_StrAssocByText(const LAssoc * p1, const LAssoc * p2, StrAssocArray * pArray, LAssocArray * pRecurTracer)
{
	long   id[2], par[2];
	id[0] = p1->Key;
	id[1] = p2->Key;
	par[0] = par[1] = 0;
	pArray->ParentList.BSearch(id[0], &par[0], 0);
	pArray->ParentList.BSearch(id[1], &par[1], 0);
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
			pi[0] = pArray->LAssocArray::at(p);
		else {
			pi[0].Key = par[0];
			pi[0].Val = 0;
		}
		if(par[1] && pArray->Search(par[1], &(p = 0)))
			pi[1] = pArray->LAssocArray::at(p);
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
	{ return StrAssocArray::Helper_CmpFunc_StrAssocByText(static_cast<const LAssoc *>(i1), static_cast<const LAssoc *>(i2), static_cast<StrAssocArray *>(pExtraData), 0); }

static int Helper_CmpFunc_StrAssocByID(long id1, long id2, StrAssocArray * pArray, LAssocArray * pRecurTracer)
{
	long   par[2];
	par[0] = par[1] = 0;
	pArray->ParentList.BSearch(id1, &par[0], 0);
	pArray->ParentList.BSearch(id2, &par[1], 0);
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
	{ return Helper_CmpFunc_StrAssocByID(*static_cast<const long *>(i1), *static_cast<const long *>(i2), static_cast<StrAssocArray *>(pExtraData), 0); }

#if 0 // {

{1, 6, "X01"}
{2, 1, "X02"}
{3, 1, "X03"}
{4, 3, "X04"}
{5, 3, "X05"}
{6, 3, "X06"}

#endif // } 0

void StrAssocArray::SortByID()
{
	if(ParentList.getCount() == 0)
		LAssocArray::Sort();
	else {
		ParentList.Sort();
		LAssocArray::sort(PTR_CMPFUNC(StrAssoc_ByID), this);
	}
}

void StrAssocArray::SortByText()
{
	ParentList.Sort();
	LAssocArray::sort(PTR_CMPFUNC(StrAssoc_ByText), this);
}

void StrAssocArray::Helper_SortByTextInTreeOrder(long parentId, StrAssocArray & rTempList)
{
	LongArray child_list;
	GetListByParent(parentId, 0, child_list);
	child_list.sort();
	for(uint i = 0; i < getCount(); i++) {
		const long key = GetKeyByIdx(i);
		if(child_list.bsearch(key)) {
			StrAssocArray::Item item = Get(i);
			assert(item.Id == key);
			assert(item.ParentId == parentId);
			rTempList.AddFast(item.Id, item.ParentId, item.Txt);
			if(HasChild(item.Id)) {
				Helper_SortByTextInTreeOrder(item.Id, rTempList); // @recursion
			}
		}
	}
}

void StrAssocArray::SortByTextInTreeOrder()
{
	SortByText();
	LongArray child_list;
	StrAssocArray temp_list;
	Helper_SortByTextInTreeOrder(0, temp_list);
	*this = temp_list;
}

void StrAssocArray::SortByLength(int descend)
{
	LAssocArray::sort(descend ? PTR_CMPFUNC(StrAssoc_ByLength_Descend) : PTR_CMPFUNC(StrAssoc_ByLength), &Pool);
}

void StrAssocArray::Reverse()
{
	LAssocArray::reverse(0, LAssocArray::getCount());
}

int FASTCALL StrAssocArray::Write(SBuffer & rBuf, long o) const
{
	int    ok = 1;
	THROW(rBuf.Write(static_cast<const LAssocArray *>(this), 0));
	THROW(rBuf.Write(&ParentList, 0));
	THROW(Pool.Write(rBuf));
	CATCHZOK
	return ok;
}

int FASTCALL StrAssocArray::Read(SBuffer & rBuf, long o)
{
	int    ok = 1;
	THROW(rBuf.Read(static_cast<LAssocArray *>(this), 0));
	THROW(rBuf.Read(&ParentList, 0));
	THROW(Pool.Read(rBuf));
	CATCHZOK
	return ok;
}

#ifndef _WIN32_WCE // {

int StrAssocArray::Write(SFile & rFile, long o) const
{
	int    ok = 1;
	THROW(LAssocArray::Write(rFile, o));
	THROW(ParentList.Write(rFile, o));
	THROW(Pool.Write(rFile, o));
	CATCHZOK
	return ok;
}

int StrAssocArray::Read(SFile & rFile, long o)
{
	int    ok = 1;
	THROW(LAssocArray::Read(rFile, o));
	THROW(ParentList.Read(rFile, o));
	THROW(Pool.Read(rFile, o));
	CATCHZOK
	return ok;
}

#endif // } _WIN32_WCE
//
//
//
StrStrAssocArray::StrStrAssocArray() : SStrGroup()
{
}

StrStrAssocArray::StrStrAssocArray(const StrStrAssocArray & rS) : SStrGroup(rS), L(rS.L) // @v10.3.4
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

uint StrStrAssocArray::getCount() const
{
	return L.getCount();
}

StrStrAssocArray::Item FASTCALL StrStrAssocArray::at(uint pos) const
{
	Item   item;
	MEMSZERO(item);
	if(pos < L.getCount()) {
		const  LAssoc & r_assc = L.at(pos);
		const uint key_pos = static_cast<uint>(r_assc.Key);
		const uint val_pos = static_cast<uint>(r_assc.Val);
		if(key_pos && key_pos < Pool.getDataLen())
			item.Key = Pool.getBuf() + key_pos;
		if(val_pos && val_pos < Pool.getDataLen())
			item.Val = Pool.getBuf() + val_pos;
	}
	return item;
}

int StrStrAssocArray::Search(const char * pKey, SString * pVal, uint * pPos) const
{
	int    ok = 0;
	if(!isempty(pKey)) {
		const SString pattern_buf(pKey);
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

int StrStrAssocArray::Add(const char * pKey, const char * pVal, int replaceDup)
{
	int    ok = 1;
	int    done = 0;
	if(replaceDup) {
		uint pos = 0;
		SString val;
		if(Search(pKey, &val, &pos)) {
			uint   sp = 0;
			AddS(pVal, &sp);
			L.at(pos).Val = static_cast<long>(sp);
			done = 1;
		}
	}
	if(!done) {
		uint   key_pos = 0;
		uint   val_pos = 0;
		AddS(pKey, &key_pos);
		AddS(pVal, &val_pos);
		LAssoc item(static_cast<long>(key_pos), static_cast<long>(val_pos));
		L.insert(&item);
	}
	return ok;
}

int StrStrAssocArray::Remove(const char * pKey)
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

StrStrAssocArray & StrStrAssocArray::Z()
{
	L.clear();
	SStrGroup::ClearS();
	return *this;
}
//
//
//
RealArray::RealArray() : TSVector <double> ()
{
}

RealArray::RealArray(const RealArray & s) : TSVector <double> (s)
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

int RealArray::add(double v)
{
	return insert(&v);
}

int FASTCALL RealArray::dim(uint numItems)
{
    clear();
	return insertChunk(numItems, 0);
}

double RealArray::StrideDifference(uint idx, uint stride) const
{
	assert(idx < count && idx >= stride);
	return (idx < count && idx >= stride) ? (PTRDBLC(P_VData)[idx] - PTRDBLC(P_VData)[idx-stride]) : 0.0;
}

double RealArray::Sum() const
{
	return count ? SVectorBase::sumDouble(0, 0, count-1) : 0.0;
	/*double sum = 0.0;
	for(uint i = 0; i < count; i++) {
		sum += at(i);
	}
	return sum;*/
}

void RealArray::Sort()
{
	sort(PTR_CMPFUNC(double));
}
//
// LongArray
//
LongArray::LongArray() : /*TSArray*/TSVector <long>()
{
}

LongArray::LongArray(const LongArray & s) : /*TSArray*/TSVector <long> (s)
{
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

int CDECL LongArray::addzlist(long first, ...)
{
	va_list list;
	va_start(list, first);
	long   val;
	int    ok = add(first);
	while(ok && (val = va_arg(list, long)) != 0) {
		if(!add(val))
			ok = 0;
	}
	va_end(list);
	return ok;
}

int FASTCALL LongArray::add(const LongArray * pS)
{
	const  uint _c = SVectorBase::GetCount(pS);
	return _c ? insertChunk(_c, pS->dataPtr()) : 1;
}

int LongArray::addExclusive(const LongArray * pS, const LongArray * pExcl)
{
	int   ok = 1;
	//const uint _c = pS ? pS->getCount() : 0;
	const  uint _c = SVectorBase::GetCount(pS);
	if(_c) {
		if(pExcl) {
			for(uint i = 0; ok && i < _c; i++) {
				const long val = pS->at(i);
				if(!pExcl->lsearch(val))
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

int LongArray::addUniqueExclusive(const LongArray * pS, const LongArray * pExcl)
{
	if(pS) {
		const uint c = pS->getCount();
		if(pExcl) {
			for(uint i = 0; i < c; i++) {
				const long val = pS->at(i);
				if(!pExcl->lsearch(val))
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

int FASTCALL LongArray::removeByID(long id)
{
	uint   pos = 0;
	return lsearch(id, &pos) ? atFree(pos) : -1;
}

int    FASTCALL LongArray::operator == (const LongArray & rS) const { return IsEqual(&rS); }
int    FASTCALL LongArray::operator != (const LongArray & rS) const { return BIN(!IsEqual(&rS)); }
int    FASTCALL LongArray::add(long id) { return insert(&id); }
int    FASTCALL LongArray::addnz(long id) { return id ? insert(&id) : -1; }
int    FASTCALL LongArray::addUnique(long id) { return lsearch(id) ? -1 : add(id); }
int    FASTCALL LongArray::addUnique(const LongArray * pS) { return addUniqueExclusive(pS, 0); }
int    LongArray::ordInsert(long i, uint * p) { return SVector::ordInsert(&i, p, CMPF_LONG); }
int    FASTCALL LongArray::bsearch(long i, uint * p) const { return SVector::bsearch(&i, p, CMPF_LONG); }
int    FASTCALL LongArray::bsearch(long i) const { return SVector::bsearch(&i, 0, CMPF_LONG); }
int    LongArray::isList() const { return BIN(count > 1); }
long   FASTCALL LongArray::get(uint pos) const { return at(pos); }
long   LongArray::getSingle() const { return (count == 1) ? at(0) : 0L; }
long   LongArray::getLast() const { return (count ? at(count-1) : 0); }
void   LongArray::sort() { SVector::sort(CMPF_LONG); }
//
// @todo Ускорить функцию, закладываясь на особенности массива LongArray
//
int FASTCALL LongArray::lsearch(long k, uint * pIdx) const { return SVector::lsearch(&k, pIdx, CMPF_LONG); }
int FASTCALL LongArray::lsearch(long k) const { return SVector::lsearch(&k, 0, CMPF_LONG); }

int FASTCALL LongArray::setSingleNZ(long id)
{
	clear(); // @v9.6.8 freeAll-->clear
	return id ? add(id) : -1;
}

void LongArray::sortAndUndup()
{
	uint   c = getCount();
	if(c > 1) {
		SVector::sort(CMPF_LONG);
		assert(c == getCount()); // @paranoic
		long   prev = get(--c);
		do {
			const long t = get(--c);
			if(t == prev)
				atFree(c+1);
			prev = t;
		} while(c);
	}
}

int LongArray::isSorted() const
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

void LongArray::intersect(const LongArray * p, int binary)
{
	if(!p || !p->getCount())
		clear(); // @v10.5.8 freeAll()-->clear()
	else {
		uint c = getCount();
		if(c) do {
			const long id = at(--c);
			if((binary ? p->bsearch(id) : p->lsearch(id)) == 0)
				atFree(c);
		} while(c);
	}
}

#ifndef _WIN32_WCE // {

void LongArray::intersect(const UintHashTable * p)
{
	if(!p)
		freeAll();
	else {
		uint c = getCount();
		if(c) do {
			const ulong id = static_cast<ulong>(at(--c));
			if(!p->Has(id))
				atFree(c);
		} while(c);
	}
}

#endif // } _WIN32_WCE

LongArray & LongArray::Z()
{
	clear();
	return *this;
}

int LongArray::freeByKey(long key, int binarySearch)
{
	uint pos = 0;
	if(binarySearch ? bsearch(key, &pos) : lsearch(key, &pos)) {
		atFree(pos);
		return 1;
	}
	else
		return -1;
}

int LongArray::Serialize__construction(int dir, long flags, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	uint8  ind = 0;
	if(dir > 0) {
		const uint _c = getCount();
		if(_c == 0) {
			ind = 1;
			THROW(rBuf.Write(&ind, sizeof(ind)));
		}
		else if(_c > 16) {
			if(flags & srlzfSortAndUndup) {
				sortAndUndup();
			}
			long    up_diff = 0;
			long    dn_diff = 0;
			{
				long    prev_item = get(0);
				for(uint i = 1; i < _c; i++) {
					const long item = get(i);
					const long diff = item - prev_item;
					if(diff > 0) {
						SETMAX(up_diff, diff);
					}
					else if(diff < 0) {
						SETMIN(dn_diff, diff);
					}
					prev_item = item;
				}
			}
			if(up_diff <= MAXCHAR && dn_diff >= MINCHAR) {
				ind = 2;
				THROW(rBuf.Write(&ind, sizeof(ind)));
				long    item = get(0);
				{
					THROW(pSCtx->Serialize(+1, item, rBuf));
					{
						long    prev_item = item;
						for(uint i = 0; i < _c; i++) {
							item = get(i);
							const long diff = item - prev_item;
							assert(diff <= MAXCHAR && diff >= MINCHAR);
							int8 _diff = static_cast<int8>(diff);
							THROW(rBuf.Write(&_diff, sizeof(_diff)));
						}
					}
				}
			}
			else if(up_diff <= MAXSHORT && dn_diff >= MINSHORT) {
				ind = 3;
				THROW(rBuf.Write(&ind, sizeof(ind)));
				long    item = get(0);
				{
					THROW(pSCtx->Serialize(+1, item, rBuf));
					{
						long    prev_item = item;
						for(uint i = 0; i < _c; i++) {
							item = get(i);
							const long diff = item - prev_item;
							assert(diff <= MAXSHORT && diff >= MINSHORT);
							int16 _diff = static_cast<int16>(diff);
							THROW(rBuf.Write(&_diff, sizeof(_diff)));
						}
					}
				}
			}
			else {
				ind = 4;
				THROW(rBuf.Write(&ind, sizeof(ind)));
				{

				}
			}
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
Int64Array::Int64Array() : TSVector <int64> ()
{
}

Int64Array::Int64Array(const Int64Array & rS) : TSVector <int64> (rS)
{
}

Int64Array & FASTCALL Int64Array::operator = (const Int64Array & rS)
{
	copy(rS);
	return *this;
}

int FASTCALL Int64Array::IsEqual(const Int64Array * pS) const
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

int    FASTCALL Int64Array::operator == (const Int64Array & rS) const { return IsEqual(&rS); }
int    FASTCALL Int64Array::operator != (const Int64Array & rS) const { return BIN(!IsEqual(&rS)); }
int64  FASTCALL Int64Array::get(uint pos) const { return at(pos); }
int    Int64Array::lsearch(int64 k, uint * pIdx) const { return SVector::lsearch(&k, pIdx, PTR_CMPFUNC(int64)); }
int    FASTCALL Int64Array::lsearch(int64 key) const { return SVector::lsearch(&key, 0, PTR_CMPFUNC(int64)); }
int    FASTCALL Int64Array::bsearch(int64 key) const { return SVector::bsearch(&key, 0, PTR_CMPFUNC(int64)); }
int    FASTCALL Int64Array::addUnique(int64 v) { return lsearch(v) ? -1 : add(v); }
int    FASTCALL Int64Array::add(int64 v) { return insert(&v); }
void   Int64Array::sort() { SVector::sort(CMPF_INT64); }

int FASTCALL Int64Array::add(int32 v)
{
	int64 v64 = static_cast<int64>(v);
	return insert(&v64);
}

int FASTCALL Int64Array::add(const Int64Array * pS)
{
	const  uint _c = SVectorBase::GetCount(pS);
	return _c ? insertChunk(_c, pS->dataPtr()) : 1;
}

void Int64Array::sortAndUndup()
{
	uint   c = getCount();
	SVector::sort(CMPF_INT64);
	assert(c == getCount()); // @paranoic
	if(c > 1) {
		int64  prev = get(--c);
		do {
			const int64 t = get(--c);
			if(t == prev)
				atFree(c+1);
			prev = t;
		} while(c);
	}
}

void Int64Array::intersect(const Int64Array * pList, int binary)
{
	if(!pList || !pList->getCount())
		clear();
	else {
		uint c = getCount();
		if(c) do {
			const int64 id = at(--c);
			if((binary ? pList->bsearch(id) : pList->lsearch(id)) == 0)
				atFree(c);
		} while(c);
	}
}
//
//
//
FloatArray::FloatArray() : TSVector <float>() {}
int    FASTCALL FloatArray::operator == (const FloatArray & rS) const { return IsEqual(&rS); }
int    FASTCALL FloatArray::operator != (const FloatArray & rS) const { return BIN(!IsEqual(&rS)); }
int    FASTCALL FloatArray::add(float item) { return insert(&item); }
int    FASTCALL FloatArray::add(SPoint2F p) { return insertChunk(2, &p); }
float  FASTCALL FloatArray::get(uint i) const { return at(i); }
float & FASTCALL FloatArray::operator [] (size_t i) const { return at(i); }

int FASTCALL FloatArray::dim(uint numItems)
{
    clear();
	return insertChunk(numItems, 0);
}

const SPoint2F & FASTCALL FloatArray::getPoint(uint i) const
{
	assert(i < (count-1));
	return *static_cast<const SPoint2F *>(SVector::at(i)); // @v9.8.4 SArray-->SVector
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
DateTimeArray::DateTimeArray() : TSVector <LDATETIME>()
{
}

int FASTCALL DateTimeArray::add(LDATETIME dtm)
{
	return insert(&dtm);
}

LDATETIME FASTCALL DateTimeArray::get(uint idx) const
{
	return at(idx);
}

void DateTimeArray::sort()
{
	SVector::sort(PTR_CMPFUNC(LDATETIME));
}
//
//
//
FloatStack::FloatStack() : TSStack <float>(/*4*/)
{
}

void FloatStack::pushPoint(SPoint2F p)
{
	push(p.y);
	push(p.x);
}

int FloatStack::popPoint(SPoint2F & rP)
	{ return BIN(pop(rP.x) && pop(rP.y)); }
//
//
//
STimeChunkArray::STimeChunkArray() : SArray(sizeof(STimeChunk))
{
}

STimeChunkArray::STimeChunkArray(size_t itemSize) : SArray(itemSize)
{
	assert(itemSize >= sizeof(STimeChunk));
}

int STimeChunkArray::Add(const STimeChunk * pEntry, int checkForFree)
{
	int    ok = 0;
	assert(pEntry);
	if(pEntry) {
		ok = (!checkForFree || IsFreeEntry(*pEntry, 0)) ? insert(pEntry) : -1;
	}
	return ok;
}

int STimeChunkArray::Helper_Intersect(const STimeChunk & rChunk, STimeChunkArray & rResult) const
{
	int    yes = 0;
	for(uint j = 0; j < getCount(); j++) {
		const STimeChunk * p_item = static_cast<const STimeChunk *>(at(j));
		STimeChunk sect;
		if(rChunk.Intersect(*p_item, &sect) > 0) {
			rResult.Add(&sect, 0);
			yes = 1;
		}
	}
	return yes;
}

int STimeChunkArray::Intersect(const STimeChunkArray * pList, STimeChunkArray * pResult) const
{
	int    yes = 0;
	pResult->clear();
	for(uint i = 0; i < pList->getCount(); i++) {
		const STimeChunk * p_item1 = static_cast<const STimeChunk *>(pList->at(i));
		if(Helper_Intersect(*p_item1, *pResult))
			yes = 1;
	}
	pResult->Sort();
	return yes;
}

int STimeChunkArray::Intersect(const STimeChunk & rChunk, STimeChunkArray * pResult) const
{
	int    yes = 0;
	pResult->clear();
	yes = Helper_Intersect(rChunk, *pResult);
	pResult->Sort();
	return yes;
}

uint STimeChunkArray::GetIntersectionOrder(LAssocArray * pOrderList, long tolerance) const
{
	uint   i;
	uint   order = 0;
	STimeChunk sect;
	LongArray level;
	CALLPTRMEMB(pOrderList, clear()); // @v10.6.5 freeAll()-->clear()
	for(i = 0; i < getCount(); i++)
		level.add(static_cast<long>(i));
	for(uint c = level.getCount(); c != 0; c = level.getCount()) {
		LongArray temp_level;
		for(i = 0; i < c; i++) {
			const uint k = static_cast<uint>(level.get(i));
			CALLPTRMEMB(pOrderList, Update(static_cast<long>(k+1), order, 1));
			const STimeChunk * p_item_k = static_cast<const STimeChunk *>(at(k));
			for(uint j = i+1; j < c; j++) {
				const uint m = static_cast<uint>(level.get(j));
				if(p_item_k->Intersect(*static_cast<const STimeChunk *>(at(m)), &sect)) {
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

int STimeChunkArray::Excise(const STimeChunk * pItem)
{
	int    ok = -1;
	if(pItem) {
		STimeChunk test, sect, * p_entry;
		if(getCount() == 0) {
			STimeChunk inf;
			inf.Finish.SetFar();
			insert(&inf);
		}
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
						if(r_start > 0) {
							p_entry->Finish = sect.Start;
						}
						if(r_finish < 0) {
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
						}
					}
					ok = 1;
				}
			}
		}
	}
	return ok;
}

void STimeChunkArray::Sort(int desc)
{
	SArray::sort(PTR_CMPFUNC(STimeChunk), reinterpret_cast<void *>(desc));
}

int STimeChunkArray::Limit(const STimeChunk * pPeriod)
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

int STimeChunkArray::IsFreeEntry(const STimeChunk & rEntry, uint * pPos) const
{
	STimeChunk result;
	// @v10.3.12 {
	for(uint i = 0; i < getCount(); i++) {
		const STimeChunk * p_item = static_cast<const STimeChunk *>(at(i));
		if(p_item->Intersect(rEntry, &result) && cmp(result.Start, result.Finish) != 0) {
			ASSIGN_PTR(pPos, i);
			return 0;
		}
	}
	// } @v10.3.12 
	/* @v10.3.12
	STimeChunk * p_item;
	for(uint i = 0; enumItems(&i, (void **)&p_item);) {
		if(p_item->Intersect(rEntry, &result) && cmp(result.Start, result.Finish) != 0) {
			ASSIGN_PTR(pPos, i-1);
			return 0;
		}
	}
	*/
	return 1;
}

int STimeChunkArray::IsFreeEntry(const LDATETIME & start, long cont, uint * pPos) const
{
	STimeChunk entry;
	entry.Init(start, cont);
	return (!!start.d) ? IsFreeEntry(entry, pPos) : 1;
}

int STimeChunkArray::GetFreeList(STimeChunkArray * pList) const
{
	STimeChunk free_entry;
	LDATETIME last = ZERODATETIME;
	for(uint i = 0; i < getCount(); i++) {
		const STimeChunk & entry = *static_cast<const STimeChunk *>(at(i));
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

int STimeChunkArray::GetBounds(STimeChunk * pBounds, int update) const
{
	int    ok = 0;
	if(pBounds) {
		const STimeChunk * p_chunk;
		STimeChunk bounds;
		if(update)
			bounds = *pBounds;
		else
			bounds.Finish.SetFar();
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
STimeChunkAssoc::STimeChunkAssoc() : Id(0), Status(0)
{
}

STimeChunkAssocArray::STimeChunkAssocArray(long id) : STimeChunkArray(sizeof(STimeChunkAssoc)), Id(id)
{
}

int STimeChunkAssocArray::Get(long chunkId, uint * pPos, STimeChunkAssoc * pItem) const
{
	uint   pos = 0;
	if(lsearch(&chunkId, &pos, CMPF_LONG, offsetof(STimeChunkAssoc, Id))) {
		ASSIGN_PTR(pPos, pos);
		ASSIGN_PTR(pItem, *static_cast<const STimeChunkAssoc *>(at(pos)));
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
	RVALUEPTR(item.Chunk, pChunk);
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
	return stcomp(reinterpret_cast<TYPEID>(pExtraData), i1, i2);
}

STypArray::STypArray(TYPEID aType, uint o) : SArray(stsize(aType), o), type(aType)
{
}

/*virtual*/void FASTCALL STypArray::freeItem(void * pItem)
{
	stdestroy(type, pItem);
	SArray::freeItem(pItem);
}

SString & STypArray::itemToString(uint idx, long fmt, SString & rBuf) const
{
	rBuf.Z();
	const void * p = at(idx);
	if(p) {
		char   temp_buf[256];
		if(sttostr(type, p, fmt, temp_buf))
			rBuf = temp_buf;
	}
	return rBuf;
}

int STypArray::search(const void * pKey, uint * pPos) const
{
	return (VFlags & arySorted) ? bsearch(pKey, pPos, Subst, 0, reinterpret_cast<void *>(type)) : lsearch(pKey, pPos, Subst, 0, reinterpret_cast<void *>(type));
}

void STypArray::sort()
{
	SArray::sort(Subst, reinterpret_cast<void *>(type));
}
//
// STree
//
STree::Iter::Iter() : ParentP(0), CurrP(0), CurrDataP(0), NextP(0), Flags(fFirstIter), P_Data(0)
{
}

STree::Iter::Iter(uint32 parentP) : ParentP(parentP), CurrP(0), CurrDataP(0), NextP(0), Flags(fFirstIter | fOnlyParent),  P_Data(0)
{
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
STree::STree(uint itemSize, uint o) : SArray(itemSize, /*32,*/ (o & (aryDataOwner|aryEachItem|aryPtrContainer)))
{
	Item_ stub;
	MEMSZERO(stub);
	TreeList.insert(&stub);
}

STree & STree::Z()
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

/*STree::Item_ * FASTCALL STree::Get_(uint p) const
{
	return (STree::Item_ *)TreeList.at(p);
}*/

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
	const  Item_ * p_item = 0;
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
		const Item_ & r_par = TreeList.at(rIter.ParentP);
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

int STree::UpdateNodeParent(uint32 nodeP, uint32 parentP)
{
	int    ok = 1;
	THROW(nodeP < TreeList.getCount());
	THROW(parentP < TreeList.getCount());
	{
		Item_ & r_node = TreeList.at(nodeP);
		if(r_node.ParentP != parentP) {
			assert(r_node.ParentP < TreeList.getCount());
			Item_ & r_old_parent = TreeList.at(r_node.ParentP);
			Item_ & r_new_parent = TreeList.at(parentP);
			const uint next_p = r_node.NextP;
			if(r_node.PrevP) {
				assert(r_old_parent.FirstChildP != nodeP);
				Item_ & r_prev = TreeList.at(r_node.PrevP);
				assert(r_prev.NextP == nodeP);
				r_prev.NextP = next_p;
				if(next_p) {
					Item_ & r_next = TreeList.at(next_p);
					assert(r_next.PrevP == nodeP);
					r_next.PrevP = r_node.PrevP;
					r_node.NextP = 0;
				}
				r_node.PrevP = 0;
			}
			else {
				assert(r_old_parent.FirstChildP == nodeP);
				if(next_p) {
					Item_ & r_next = TreeList.at(next_p);
					assert(r_next.PrevP == nodeP);
					r_next.PrevP = 0;
					r_old_parent.FirstChildP = next_p;
					r_node.NextP = 0;
				}
			}
			if(!r_new_parent.FirstChildP) {
				r_new_parent.FirstChildP = nodeP;
				r_node.NextP = 0;
				r_node.PrevP = 0;
			}
			else {
				uint last_p = r_new_parent.FirstChildP;
				Item_ * p_iter = &TreeList.at(last_p);
				while(p_iter->NextP) {
					last_p = p_iter->NextP;
					p_iter = &TreeList.at(last_p);
				}
				r_node.PrevP = last_p;
				p_iter->NextP = nodeP;
			}
		}
		else
			ok = -1;
	}
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
//
//
//
#ifndef NDEBUG // {
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

int TestSTree()
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
			const void * p_data = iter.GetData();
			if(p_data) {
				long id = *static_cast<const long *>(p_data);
				long parent_id_sa = 0;
				long parent_id_tree = 0;
				void * p_parent = tree.GetData(iter.GetParentPos());
				if(p_parent)
					parent_id_tree = *static_cast<const long *>(p_parent);
				sa_list.GetText(id, text_buf);
				sa_list.GetParent(id, &parent_id_sa);

				f_tree.WriteLine(out_buf.Z().Cat(id).CatDiv(';', 2).Cat(parent_id_tree).CatDiv(';', 2).Cat(text_buf).CR());
				f_sa.WriteLine(out_buf.Z().Cat(id).CatDiv(';', 2).Cat(parent_id_sa).CatDiv(';', 2).Cat(text_buf).CR());
			}
		}

	}
	CATCHZOK
	return ok;
}
#endif
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
	uint GetSaCount() const { return SIZEOFARRAY(Sa); }
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
			SLTEST_CHECK_EQ(*static_cast<const long *>(p_list->at(i)), F.Sa[F.GetSaCount()-i-1]);
		}
		//
		// Сортируем список p_list и тестируем правильность сортировки
		//
		p_list->sort(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLTEST_CHECK_LE(*static_cast<const long *>(p_list->at(i)), *static_cast<const long *>(p_list->at(i+1)));
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
			SLTEST_CHECK_EQ(sorted, 0L);
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
//
// Descr: Тест класса SVector.
//   На момент создания полностью повторяет тест SArray. Необходимо убедиться, что
//   SVector в большинстве аспектов функционирует также правильно как и SArray.
//
SLTEST_FIXTURE(SVector, TestFixtureSArray)
{
	uint   i;
	SVector * p_list = 0;
	p_list = new SVector(sizeof(long));
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
			SLTEST_CHECK_EQ(sorted, 0L);
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
