// SSTACK.CPP
// Copyright (c) Sobolev A. 1995..2000, 2002, 2003, 2007, 2008, 2010, 2016, 2017, 2019, 2020, 2021, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
//
// Definitions same as in sarray.cpp
//
#define _USERPTR(ptr) ((VFlags&aryPtrContainer)?(*static_cast<void **>(ptr)):(ptr))
#define _INTRPTR(ptr) ((VFlags&aryPtrContainer)?&(ptr):(ptr))
#define _BDATA        PTR8(P_VData)

SStack::SStack(uint itemSize, uint o) : SArray(itemSize, o)
{
	pointer = 0;
	if(VFlags & aryRing) {
		;
	}
}

void * FASTCALL SStack::push(const void * pItem)
{
	void * p_result = 0;
	if(pointer < count) {
		if(VFlags & aryEachItem) {
			void * p_item = _BDATA + pointer * isize;
			freeItem(_USERPTR(p_item));
		}
		p_result = atPut(pointer++, pItem);
	}
	else if(VFlags & aryRing && Limit && count >= Limit) {
		//
		// Для кольцевого стека при достижении первого лимита (размер кольца
		// задается параметром delta) начинаем заносить элементы в начало массива.
		//
		assert(count == Limit);
		if(VFlags & aryEachItem) {
			void * p_item = _BDATA + pointer * isize;
			freeItem(_USERPTR(p_item));
		}
		p_result = atPut(0, pItem);
		pointer = 1;
	}
	else if(insert(pItem)) {
		pointer++;
		assert(count > 0);
		p_result = at(count-1);
	}
	return p_result;
}

void * SStack::pop()
{
	if(pointer)
		return at(--pointer);
	else if(VFlags & aryRing && Limit && count >= Limit) {
		//
		// Для кольцевого стека при достижении нулевой границы массива
		// (если массив распределен более чем на 0 элементов) извлекаем
		// предыдущий элемент из конца массива.
		// @attention Таким образом кольцевой стек лишен проверки на
		//   извлечение элемента, ниже минимальной границы стека.
		//
		assert(count == Limit);
		pointer = Limit-1;
		return at(pointer);
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, nullptr);
}

int FASTCALL SStack::pop(void * pItem)
{
	if(pointer) {
		pointer--;
		if(pItem)
			memcpy(pItem, at(pointer), isize);
		return 1;
	}
	else if(VFlags & aryRing && Limit && count >= Limit) {
		assert(count == Limit);
		pointer = Limit-1;
		if(pItem)
			memcpy(pItem, at(pointer), isize);
		return 2;
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, 0);
}

int SStack::undo(uint ptr)
{
	return checkupper(ptr, count+1) ? ((pointer = ptr), 1) : 0;
}

void * SStack::peek() const
{
	if(pointer)
		return at(pointer-1);
	else if(VFlags & aryRing && Limit && count >= Limit) {
		assert(count == Limit);
		return at(Limit-1);
	}
	else
		return (SLibError = SLERR_ARYZEROCOUNT, nullptr);
}
//
//
//
SQueue::SQueue(uint itemSize, uint queueSize, uint o) : SArray(itemSize, /*8,*/ o), QueueSize(queueSize), Front(0), End(0)
{
}

SQueue::SQueue(const SQueue & rS) : SArray(rS), QueueSize(rS.QueueSize), Front(rS.Front), End(rS.End)
{
}

int FASTCALL SQueue::copy(const SQueue & src)
{
	QueueSize = src.QueueSize;
	Front     = src.Front;
	End       = src.End;
	return SArray::copy(src);
}

void SQueue::clear()
{
	SArray::clear();
	Front = End = 0;
}

int FASTCALL SQueue::push(const void * pItem)
{
	if(count < QueueSize) {
		if(!atInsert(End % QueueSize, pItem))
			return 0;
	}
	else {
		atPut(End % QueueSize, pItem);
	}
	End++;
	if((End - Front) > QueueSize)
		Front++;
	//
	// Normalyze values of Front and End
	//
	uint   f = Front / QueueSize;
	if(f > 1) {
		End -= (f-1) * QueueSize;
		Front -= (f-1) * QueueSize;
	}
	return 1;
}

void * SQueue::pop()
{
	void * p_item;
	if(Front < End) {
		p_item = at(Front % QueueSize);
		Front++;
	}
	else
		p_item = 0;
	return p_item;
}

void * SQueue::peek() const
{
	return (Front < End) ? at(Front % QueueSize) : 0;
}

uint SQueue::getNumItems() const
{
	return (End - Front);
}

void * SQueue::get(uint p) const
{
	return checkupper(p, End - Front) ? at((Front + p) % QueueSize) : 0;
}

#if 0 // {

SStack

class SStackRing

#endif // } 0
