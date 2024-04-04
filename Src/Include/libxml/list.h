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

//#ifdef __cplusplus
//extern "C" {
//#endif

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
XMLPUBFUN xmlList * xmlListCreate(xmlListDeallocator deallocator, xmlListDataCompare compare);
XMLPUBFUN void FASTCALL xmlListDelete(xmlList * pList);
//
// Basic Operators 
//
XMLPUBFUN void * xmlListSearch(xmlList * pList, void * data);
XMLPUBFUN void * xmlListReverseSearch(xmlList * pList, void * data);
XMLPUBFUN int FASTCALL xmlListInsert(xmlList * pList, void * data);
XMLPUBFUN int xmlListAppend(xmlList * pList, void * data);
XMLPUBFUN int xmlListRemoveFirst(xmlList * pList, void * data);
XMLPUBFUN int xmlListRemoveLast(xmlList * pList, void * data);
XMLPUBFUN int xmlListRemoveAll(xmlList * pList, void * data);
XMLPUBFUN void FASTCALL xmlListClear(xmlList * pList);
XMLPUBFUN int FASTCALL xmlListEmpty(const xmlList * l);
XMLPUBFUN xmlLink * FASTCALL xmlListFront(xmlList * pList);
XMLPUBFUN xmlLink * xmlListEnd(xmlList * pList);
XMLPUBFUN int xmlListSize(xmlList * pList);
XMLPUBFUN void FASTCALL xmlListPopFront(xmlList * pList);
XMLPUBFUN void xmlListPopBack(xmlList * pList);
XMLPUBFUN int FASTCALL xmlListPushFront(xmlList * pList, void * data);
XMLPUBFUN int xmlListPushBack(xmlList * pList, void * data);
//
// Advanced Operators 
//
XMLPUBFUN void xmlListReverse(xmlList * pList);
XMLPUBFUN void xmlListSort(xmlList * pList);
XMLPUBFUN void xmlListWalk(xmlList * pList, xmlListWalker walker, const void * user);
XMLPUBFUN void xmlListReverseWalk(xmlList * pList, xmlListWalker walker, const void * user);
XMLPUBFUN void xmlListMerge(xmlList * l1, xmlList * l2);
XMLPUBFUN xmlList * xmlListDup(const xmlList * old);
XMLPUBFUN int xmlListCopy(xmlList * cur, const xmlList * old);
// Link operators 
XMLPUBFUN void * FASTCALL xmlLinkGetData(xmlLink * lk);
/* xmlListUnique() */
/* xmlListSwap */
//#ifdef __cplusplus
//}
//#endif

#endif /* __XML_LINK_INCLUDE__ */
