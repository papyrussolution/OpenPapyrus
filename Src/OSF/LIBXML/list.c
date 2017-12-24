/*
 * list.c: lists handling implementation
 *
 * Copyright (C) 2000 Gary Pennington and Daniel Veillard.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 * Author: Gary.Pennington@uk.sun.com
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop
///
// Type definition are kept internal
//
struct xmlLink {
	xmlLink * next;
	xmlLink * prev;
	void * data;
};

struct xmlList {
	xmlLink * sentinel;
	void (* linkDeallocator)(xmlLink *);
	int (* linkCompare)(const void *, const void*);
};

/************************************************************************
*                                    *
*                Interfaces                *
*                                    *
************************************************************************/

/**
 * xmlLinkDeallocator:
 * @l:  a list
 * @lk:  a link
 *
 * Unlink and deallocate @lk from list @l
 */
static void FASTCALL xmlLinkDeallocator(xmlList * l, xmlLink * lk)
{
	(lk->prev)->next = lk->next;
	(lk->next)->prev = lk->prev;
	if(l->linkDeallocator)
		l->linkDeallocator(lk);
	SAlloc::F(lk);
}
/**
 * xmlLinkCompare:
 * @data0:  first data
 * @data1:  second data
 *
 * Compares two arbitrary data
 *
 * Returns -1, 0 or 1 depending on whether data1 is greater equal or smaller
 *          than data0
 */
static int xmlLinkCompare(const void * data0, const void * data1)
{
	if(data0 < data1)
		return -1;
	else if(data0 == data1)
		return 0;
	else
		return 1;
}
/**
 * xmlListLowerSearch:
 * @l:  a list
 * @data:  a data
 *
 * Search data in the ordered list walking from the beginning
 *
 * Returns the link containing the data or NULL
 */
static xmlLink * FASTCALL xmlListLowerSearch(xmlList * pList, void * data)
{
	xmlLink * lk = 0;
	if(pList) {
		for(lk = pList->sentinel->next; lk != pList->sentinel && pList->linkCompare(lk->data, data) <0; lk = lk->next) 
			;
	}
	return lk;
}
/**
 * xmlListHigherSearch:
 * @l:  a list
 * @data:  a data
 *
 * Search data in the ordered list walking backward from the end
 *
 * Returns the link containing the data or NULL
 */
static xmlLink * FASTCALL xmlListHigherSearch(xmlList * pList, void * data)
{
	xmlLink * lk = 0;
	if(pList) {
		for(lk = pList->sentinel->prev; lk != pList->sentinel && pList->linkCompare(lk->data, data) >0; lk = lk->prev)
			;
	}
	return lk;
}
/**
 * xmlListSearch:
 * @l:  a list
 * @data:  a data
 *
 * Search data in the list
 *
 * Returns the link containing the data or NULL
 */
static xmlLink * FASTCALL xmlListLinkSearch(xmlList * pList, void * data)
{
	xmlLink * lk;
	if(pList == NULL)
		return 0;
	else {
		lk = xmlListLowerSearch(pList, data);
		if(lk == pList->sentinel)
			return NULL;
		else
			return (pList->linkCompare(lk->data, data) == 0) ? lk : NULL;
	}
}
/**
 * xmlListLinkReverseSearch:
 * @l:  a list
 * @data:  a data
 *
 * Search data in the list processing backward
 *
 * Returns the link containing the data or NULL
 */
static xmlLink * xmlListLinkReverseSearch(xmlList * l, void * data)
{
	xmlLink * lk;
	if(l == NULL)
		return 0;
	lk = xmlListHigherSearch(l, data);
	if(lk == l->sentinel)
		return NULL;
	else
		return (l->linkCompare(lk->data, data) == 0) ? lk : NULL;
}
/**
 * xmlListCreate:
 * @deallocator:  an optional deallocator function
 * @compare:  an optional comparison function
 *
 * Create a new list
 *
 * Returns the new list or NULL in case of error
 */
xmlList * xmlListCreate(xmlListDeallocator deallocator, xmlListDataCompare compare)
{
	xmlList * l;
	if(NULL == (l = (xmlList *)SAlloc::M(sizeof(xmlList)))) {
		xmlGenericError(0, "Cannot initialize memory for list");
		return 0;
	}
	/* Initialize the list to NULL */
	memzero(l, sizeof(xmlList));
	/* Add the sentinel */
	if(NULL ==(l->sentinel = (xmlLink *)SAlloc::M(sizeof(xmlLink)))) {
		xmlGenericError(0, "Cannot initialize memory for sentinel");
		SAlloc::F(l);
		return 0;
	}
	l->sentinel->next = l->sentinel;
	l->sentinel->prev = l->sentinel;
	l->sentinel->data = NULL;
	/* If there is a link deallocator, use it */
	if(deallocator != NULL)
		l->linkDeallocator = deallocator;
	/* If there is a link comparator, use it */
	if(compare != NULL)
		l->linkCompare = compare;
	else /* Use our own */
		l->linkCompare = xmlLinkCompare;
	return l;
}

/**
 * xmlListSearch:
 * @l:  a list
 * @data:  a search value
 *
 * Search the list for an existing value of @data
 *
 * Returns the value associated to @data or NULL in case of error
 */
void * xmlListSearch(xmlList * pList, void * data)
{
	if(pList) {
		xmlLink * lk = xmlListLinkSearch(pList, data);
		if(lk)
			return (lk->data);
	}
	return 0;
}
/**
 * xmlListReverseSearch:
 * @l:  a list
 * @data:  a search value
 *
 * Search the list in reverse order for an existing value of @data
 *
 * Returns the value associated to @data or NULL in case of error
 */
void * xmlListReverseSearch(xmlList * pList, void * data)
{
	if(pList) {
		xmlLink * lk = xmlListLinkReverseSearch(pList, data);
		if(lk)
			return (lk->data);
	}
	return 0;
}
/**
 * xmlListInsert:
 * @l:  a list
 * @data:  the data
 *
 * Insert data in the ordered list at the beginning for this value
 *
 * Returns 0 in case of success, 1 in case of failure
 */
int xmlListInsert(xmlList * pList, void * data)
{
	if(pList == NULL)
		return 1;
	else {
		xmlLink * lkPlace = xmlListLowerSearch(pList, data);
		/* Add the new link */
		xmlLink * lkNew = (xmlLink *)SAlloc::M(sizeof(xmlLink));
		if(lkNew == NULL) {
			xmlGenericError(0, "Cannot initialize memory for new link");
			return 1;
		}
		else {
			lkNew->data = data;
			lkPlace = lkPlace->prev;
			lkNew->next = lkPlace->next;
			(lkPlace->next)->prev = lkNew;
			lkPlace->next = lkNew;
			lkNew->prev = lkPlace;
			return 0;
		}
	}
}
/**
 * xmlListAppend:
 * @l:  a list
 * @data:  the data
 *
 * Insert data in the ordered list at the end for this value
 *
 * Returns 0 in case of success, 1 in case of failure
 */
int xmlListAppend(xmlList * pList, void * data)
{
	xmlLink * lkPlace;
	xmlLink * lkNew;
	if(pList == NULL)
		return 1;
	lkPlace = xmlListHigherSearch(pList, data);
	/* Add the new link */
	lkNew = (xmlLink *)SAlloc::M(sizeof(xmlLink));
	if(lkNew == NULL) {
		xmlGenericError(0, "Cannot initialize memory for new link");
		return 1;
	}
	lkNew->data = data;
	lkNew->next = lkPlace->next;
	(lkPlace->next)->prev = lkNew;
	lkPlace->next = lkNew;
	lkNew->prev = lkPlace;
	return 0;
}
/**
 * xmlListDelete:
 * @l:  a list
 *
 * Deletes the list and its associated data
 */
void FASTCALL xmlListDelete(xmlList * pList)
{
	if(pList) {
		xmlListClear(pList);
		SAlloc::F(pList->sentinel);
		SAlloc::F(pList);
	}
}
/**
 * xmlListRemoveFirst:
 * @l:  a list
 * @data:  list data
 *
 * Remove the first instance associated to data in the list
 *
 * Returns 1 if a deallocation occured, or 0 if not found
 */
int xmlListRemoveFirst(xmlList * pList, void * data)
{
	if(pList) {
		// Find the first instance of this data 
		xmlLink * lk = xmlListLinkSearch(pList, data);
		if(lk) {
			xmlLinkDeallocator(pList, lk);
			return 1;
		}
	}
	return 0;
}
/**
 * xmlListRemoveLast:
 * @l:  a list
 * @data:  list data
 *
 * Remove the last instance associated to data in the list
 *
 * Returns 1 if a deallocation occured, or 0 if not found
 */
int xmlListRemoveLast(xmlList * l, void * data)
{
	if(l == NULL)
		return 0;
	else {
		// Find the last instance of this data 
		xmlLink * lk = xmlListLinkReverseSearch(l, data);
		if(lk) {
			xmlLinkDeallocator(l, lk);
			return 1;
		}
		else
			return 0;
	}
}
/**
 * xmlListRemoveAll:
 * @l:  a list
 * @data:  list data
 *
 * Remove the all instance associated to data in the list
 *
 * Returns the number of deallocation, or 0 if not found
 */
int xmlListRemoveAll(xmlList * l, void * data)
{
	int count = 0;
	if(l) {
		while(xmlListRemoveFirst(l, data))
			count++;
	}
	return count;
}
/**
 * xmlListClear:
 * @l:  a list
 *
 * Remove the all data in the list
 */
void FASTCALL xmlListClear(xmlList * pList)
{
	if(pList) {
		xmlLink * lk = pList->sentinel->next;
		while(lk != pList->sentinel) {
			xmlLink * next = lk->next;
			xmlLinkDeallocator(pList, lk);
			lk = next;
		}
	}
}
/**
 * xmlListEmpty:
 * @l:  a list
 *
 * Is the list empty ?
 *
 * Returns 1 if the list is empty, 0 if not empty and -1 in case of error
 */
int FASTCALL xmlListEmpty(xmlList * l)
{
	return l ? (l->sentinel->next == l->sentinel) : -1;
}
/**
 * xmlListFront:
 * @l:  a list
 *
 * Get the first element in the list
 *
 * Returns the first element in the list, or NULL
 */
xmlLink * FASTCALL xmlListFront(xmlList * l)
{
	return l ? l->sentinel->next : 0;
}
/**
 * xmlListEnd:
 * @l:  a list
 *
 * Get the last element in the list
 *
 * Returns the last element in the list, or NULL
 */
xmlLink * xmlListEnd(xmlList * l)
{
	return l ? l->sentinel->prev : 0;
}
/**
 * xmlListSize:
 * @l:  a list
 *
 * Get the number of elements in the list
 *
 * Returns the number of elements in the list or -1 in case of error
 */
int xmlListSize(xmlList * l)
{
	xmlLink * lk;
	int count = 0;
	if(l == NULL)
		return -1;
	/* @todo keep a counter in xmlList instead */
	for(lk = l->sentinel->next; lk != l->sentinel; lk = lk->next, count++) ;
	return count;
}
/**
 * xmlListPopFront:
 * @l:  a list
 *
 * Removes the first element in the list
 */
void FASTCALL xmlListPopFront(xmlList * l)
{
	if(!xmlListEmpty(l))
		xmlLinkDeallocator(l, l->sentinel->next);
}
/**
 * xmlListPopBack:
 * @l:  a list
 *
 * Removes the last element in the list
 */
void xmlListPopBack(xmlList * l)
{
	if(!xmlListEmpty(l))
		xmlLinkDeallocator(l, l->sentinel->prev);
}
/**
 * xmlListPushFront:
 * @l:  a list
 * @data:  new data
 *
 * add the new data at the beginning of the list
 *
 * Returns 1 if successful, 0 otherwise
 */
int FASTCALL xmlListPushFront(xmlList * l, void * data)
{
	xmlLink * lkPlace;
	xmlLink * lkNew;
	if(l == NULL)
		return 0;
	lkPlace = l->sentinel;
	// Add the new link 
	lkNew = (xmlLink *)SAlloc::M(sizeof(xmlLink));
	if(lkNew == NULL) {
		xmlGenericError(0, "Cannot initialize memory for new link");
		return 0;
	}
	lkNew->data = data;
	lkNew->next = lkPlace->next;
	(lkPlace->next)->prev = lkNew;
	lkPlace->next = lkNew;
	lkNew->prev = lkPlace;
	return 1;
}

/**
 * xmlListPushBack:
 * @l:  a list
 * @data:  new data
 *
 * add the new data at the end of the list
 *
 * Returns 1 if successful, 0 otherwise
 */
int xmlListPushBack(xmlList * l, void * data)
{
	xmlLink * lkPlace;
	xmlLink * lkNew;
	if(l == NULL)
		return 0;
	lkPlace = l->sentinel->prev;
	/* Add the new link */
	if(NULL ==(lkNew = (xmlLink *)SAlloc::M(sizeof(xmlLink)))) {
		xmlGenericError(0, "Cannot initialize memory for new link");
		return 0;
	}
	lkNew->data = data;
	lkNew->next = lkPlace->next;
	(lkPlace->next)->prev = lkNew;
	lkPlace->next = lkNew;
	lkNew->prev = lkPlace;
	return 1;
}
/**
 * xmlLinkGetData:
 * @lk:  a link
 *
 * See Returns.
 *
 * Returns a pointer to the data referenced from this link
 */
void * FASTCALL xmlLinkGetData(xmlLink * lk)
{
	return lk ? lk->data : 0;
}
/**
 * xmlListReverse:
 * @l:  a list
 *
 * Reverse the order of the elements in the list
 */
void xmlListReverse(xmlList * l)
{
	if(l) {
		xmlLink * lkPrev = l->sentinel;
		xmlLink * lk;
		for(lk = l->sentinel->next; lk != l->sentinel; lk = lk->next) {
			lkPrev->next = lkPrev->prev;
			lkPrev->prev = lk;
			lkPrev = lk;
		}
		/* Fix up the last node */
		lkPrev->next = lkPrev->prev;
		lkPrev->prev = lk;
	}
}
/**
 * xmlListSort:
 * @l:  a list
 *
 * Sort all the elements in the list
 */
void xmlListSort(xmlList * l)
{
	if(l) {
		if(!xmlListEmpty(l)) {
			/* I think that the real answer is to implement quicksort, the
			 * alternative is to implement some list copying procedure which
			 * would be based on a list copy followed by a clear followed by
			 * an insert. This is slow...
			 */
			xmlList * lTemp = xmlListDup(l);
			if(lTemp) {
				xmlListClear(l);
				xmlListMerge(l, lTemp);
				xmlListDelete(lTemp);
			}
		}
	}
}
/**
 * xmlListWalk:
 * @l:  a list
 * @walker:  a processing function
 * @user:  a user parameter passed to the walker function
 *
 * Walk all the element of the first from first to last and
 * apply the walker function to it
 */
void xmlListWalk(xmlList * l, xmlListWalker walker, const void * user)
{
	if(l && walker) {
		for(xmlLink * lk = l->sentinel->next; lk != l->sentinel; lk = lk->next) {
			if((walker(lk->data, user)) == 0)
				break;
		}
	}
}
/**
 * xmlListReverseWalk:
 * @l:  a list
 * @walker:  a processing function
 * @user:  a user parameter passed to the walker function
 *
 * Walk all the element of the list in reverse order and
 * apply the walker function to it
 */
void xmlListReverseWalk(xmlList * l, xmlListWalker walker, const void * user)
{
	if(l && walker) {
		for(xmlLink * lk = l->sentinel->prev; lk != l->sentinel; lk = lk->prev) {
			if((walker(lk->data, user)) == 0)
				break;
		}
	}
}
/**
 * xmlListMerge:
 * @l1:  the original list
 * @l2:  the new list
 *
 * include all the elements of the second list in the first one and
 * clear the second list
 */
void xmlListMerge(xmlList * l1, xmlList * l2)
{
	xmlListCopy(l1, l2);
	xmlListClear(l2);
}
/**
 * xmlListDup:
 * @old:  the list
 *
 * Duplicate the list
 *
 * Returns a new copy of the list or NULL in case of error
 */
xmlList * xmlListDup(const xmlList * old)
{
	// 
	// Hmmm, how to best deal with allocation issues when copying
	// lists. If there is a de-allocator, should responsibility lie with
	// the new list or the old list. Surely not both. I'll arbitrarily
	// set it to be the old list for the time being whilst I work out the answer
	// 
	xmlList * cur = old ? xmlListCreate(NULL, old->linkCompare) : 0;
	if(cur) {
		if(xmlListCopy(cur, old) != 0)
			cur = 0;
	}
	return cur;
}
/**
 * xmlListCopy:
 * @cur:  the new list
 * @old:  the old list
 *
 * Move all the element from the old list in the new list
 *
 * Returns 0 in case of success 1 in case of error
 */
int xmlListCopy(xmlList * cur, const xmlList * old)
{
	// Walk the old tree and insert the data into the new one 
	xmlLink * lk;
	if((old == NULL) || (cur == NULL))
		return 1;
	for(lk = old->sentinel->next; lk != old->sentinel; lk = lk->next) {
		if(0 != xmlListInsert(cur, lk->data)) {
			xmlListDelete(cur);
			return 1;
		}
	}
	return 0;
}

/* xmlListUnique() */
/* xmlListSwap */
#define bottom_list
//#include "elfgcchack.h"
