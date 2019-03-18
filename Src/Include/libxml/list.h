/*
 * Summary: lists interfaces
 * Description: this module implement the list support used in
 * various place in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Gary Pennington <Gary.Pennington@uk.sun.com>
 */
#ifndef __XML_LINK_INCLUDE__
#define __XML_LINK_INCLUDE__

#ifdef __cplusplus
extern "C" {
#endif

struct xmlLink;
struct xmlList;

//typedef xmlLink * xmlLinkPtr;
//typedef xmlList * xmlListPtr;
/**
 * xmlListDeallocator:
 * @lk:  the data to deallocate
 *
 * Callback function used to free data from a list.
 */
typedef void (*xmlListDeallocator)(xmlLink * lk);
/**
 * xmlListDataCompare:
 * @data0: the first data
 * @data1: the second data
 *
 * Callback function used to compare 2 data.
 *
 * Returns 0 is equality, -1 or 1 otherwise depending on the ordering.
 */
typedef int (*xmlListDataCompare)(const void * data0, const void * data1);
/**
 * xmlListWalker:
 * @data: the data found in the list
 * @user: extra user provided data to the walker
 *
 * Callback function used when walking a list with xmlListWalk().
 *
 * Returns 0 to stop walking the list, 1 otherwise.
 */
typedef int (*xmlListWalker)(const void * data, const void * user);
//
// Creation/Deletion 
//
XMLPUBFUN xmlList * XMLCALL xmlListCreate(xmlListDeallocator deallocator, xmlListDataCompare compare);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlListDelete(xmlList * l);
//
// Basic Operators 
//
XMLPUBFUN void * XMLCALL xmlListSearch(xmlList * pList, void * data);
XMLPUBFUN void * XMLCALL xmlListReverseSearch(xmlList * l, void * data);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlListInsert(xmlList * pList, void * data);
XMLPUBFUN int XMLCALL xmlListAppend(xmlList * l, void * data);
XMLPUBFUN int XMLCALL xmlListRemoveFirst(xmlList * l, void * data);
XMLPUBFUN int XMLCALL xmlListRemoveLast(xmlList * l, void * data);
XMLPUBFUN int XMLCALL xmlListRemoveAll(xmlList * l, void * data);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlListClear(xmlList * l);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlListEmpty(const xmlList * l);
XMLPUBFUN xmlLink * /*XMLCALL*/FASTCALL xmlListFront(xmlList * l);
XMLPUBFUN xmlLink * XMLCALL xmlListEnd(xmlList * l);
XMLPUBFUN int XMLCALL xmlListSize(xmlList * l);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlListPopFront(xmlList * l);
XMLPUBFUN void XMLCALL xmlListPopBack(xmlList * l);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlListPushFront(xmlList * l, void * data);
XMLPUBFUN int XMLCALL xmlListPushBack(xmlList * l, void * data);
//
// Advanced Operators 
//
XMLPUBFUN void XMLCALL xmlListReverse(xmlList * l);
XMLPUBFUN void XMLCALL xmlListSort(xmlList * l);
XMLPUBFUN void XMLCALL xmlListWalk(xmlList * l, xmlListWalker walker, const void * user);
XMLPUBFUN void XMLCALL xmlListReverseWalk(xmlList * l, xmlListWalker walker, const void * user);
XMLPUBFUN void XMLCALL xmlListMerge(xmlList * l1, xmlList * l2);
XMLPUBFUN xmlList * XMLCALL xmlListDup(const xmlList * old);
XMLPUBFUN int XMLCALL xmlListCopy(xmlList * cur, const xmlList * old);
// Link operators 
XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlLinkGetData(xmlLink * lk);
/* xmlListUnique() */
/* xmlListSwap */
#ifdef __cplusplus
}
#endif

#endif /* __XML_LINK_INCLUDE__ */
