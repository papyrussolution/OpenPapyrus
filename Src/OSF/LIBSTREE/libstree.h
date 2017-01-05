/*

   Copyright (C) 2003-2006 Christian Kreibich <christian@whoop.org>.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies of the Software and its documentation and acknowledgment shall be
   given in the documentation and software packages that this Software was
   used.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */
#ifndef __libstree_h
#define __libstree_h

#include <slib.h>
#include <unistd.h>
#include <sys/queue.h>

#define D0(s) fprintf(stderr, s)
#define D1(s, a1) fprintf(stderr, s, a1)
#define D2(s, a1, a2) fprintf(stderr, s, a1, a2)
#define D3(s, a1, a2, a3) fprintf(stderr, s, a1, a2, a3)

#ifdef __cplusplus
extern "C" {
#endif

#define LST_STRING_END   UINT_MAX
#define LST_EMPTY_STRING UINT_MAX

struct LstString;
struct LstStringIndex;
struct LstNode;
struct LstSuffixTree;

struct LstString : public SArray {
public:
	LstString(size_t itemSize, int o = O_ARRAY) : SArray(itemSize, 8, o)
	{
		Id = 0;
	}
	LstString(const LstString & rS) : SArray(rS)
	{
		Id = rS.Id;
	}
	LstString(size_t itemSize, void * pData) : SArray(itemSize, 8, O_N_O_ARRAY)
	{
		Id = 0;
	}
	int    IsTerminal(uint c) const
	{
		return BIN(c == count);
	}
	SString & ToStr(SString & rBuf) const;

	int    Id;
};

typedef TSCollection <LstString> LstStringSet;
//
// To implement edge-label compression, each edge is associated with a string index structure, describing a
// substring of another string by giving the start and end index (latter is inclusive). Indices start at 0.
// For example, in string "example", index (2,4) --> "amp".
//
struct LstStringIndex {
	LstStringIndex()
	{
		Init();
	}
	LstStringIndex(const LstString * pS, uint startIdx, uint endIdx, uint extraIdx)
	{
		Init();
		P_String = pS;
		StartIndex = startIdx;
		*P_EndIndex = endIdx;
		ExtraIndex = extraIdx;
	}
	LstStringIndex & Init()
	{
		THISZERO();
		P_EndIndex = &EndIndexLocal;
		return *this;
	}
	LstStringIndex & operator = (const LstStringIndex & rS)
	{
		P_String = rS.P_String;
		StartIndex = rS.StartIndex;
		SETIFZ(P_EndIndex, &EndIndexLocal);
		*P_EndIndex = *rS.P_EndIndex;
		ExtraIndex = rS.ExtraIndex;
		return *this;
	}	
	SString & Print(SString & rBuf)
	{
		rBuf = 0;
		if(P_String) {
			if(StartIndex == P_String->getCount())
				rBuf.Cat("<$>");
			else {
				SStringU temp_buf_u;
				SString temp_buf;
				const size_t _is = P_String->getItemSize();
				for(uint i = StartIndex; i <= *P_EndIndex; i++) {
					Helper_PrintItem(i, rBuf, temp_buf_u, temp_buf);
				}
				if(_is == 2)
					temp_buf_u.CopyToUtf8(rBuf, 0);
				if(ExtraIndex > 0) {
					rBuf.CatChar('[');
					Helper_PrintItem(ExtraIndex, rBuf, temp_buf_u = 0, temp_buf = 0);
					if(_is == 2) {
						temp_buf_u.CopyToUtf8(temp_buf = 0, 0);
						rBuf.Cat(temp_buf);
					}
					rBuf.CatChar(']');
				}
			}
		}
		return rBuf;
	}
	const  LstString * P_String; // Which string we're indexing here
	uint   StartIndex; // Start and end indices in the string
	//
	// The end index is kept as a pointer so that it can be adjusted globally, if necessary.
	//
	uint * P_EndIndex;
	uint   EndIndexLocal;
	//
	// For the appended single string items, it's convenient to have the index of that single item handy:
	//
	uint   ExtraIndex;
private:
	int    Helper_PrintItem(uint pos, SString & rBuf, SStringU & rTempBufU, SString & rTempBuf)
	{
		int    ok = 0;
		const size_t _is = P_String->getItemSize();
		assert(pos <= P_String->getCount());
		if(pos == P_String->getCount())
			rBuf.Cat("<$>");
		else {
			const void * p_item = P_String->at(pos);
			if(_is == 1) {
				rBuf.CatChar(*(char *)p_item);
				ok = 1;
			}
			else if(_is == 2) {
				rTempBufU.CatChar(*(wchar_t *)p_item);
				ok = 2;
			}
			else if(_is == 4) {
				rBuf.Cat(*(ulong *)p_item).Space();
				ok = 4;
			}
			else {
				(rTempBuf = 0).EncodeMime64(p_item, _is);
				rBuf.Cat(rTempBuf).Space();
				ok = (int)_is;
			}
		}
		return ok;
	}
};
//
//
//
struct LstEdge {
	static int GetLength(const LstEdge * pEdge)
	{
		return pEdge ? (*(pEdge->Range.P_EndIndex) - pEdge->Range.StartIndex + 1) : 0;
	}
	LstEdge()
	{
		//P_SrcNode = 0;
		//P_DestNode = 0;
		SrcNodeP = 0;
		DestNodeP = 0;
	}
	LstEdge(LstNode * pSrcNode, LstNode * pDestNode, const LstString * pString, uint startIndex, uint endIndex);

	//LstNode * P_SrcNode;
	//LstNode * P_DestNode;
	uint   SrcNodeP;
	uint   DestNodeP;
	LstStringIndex Range;
};

struct LstNode {
	static int IsRoot(const LstNode * pNode)
	{
		return pNode ? (pNode->P_UpEdge == 0) : 0;
	}
	static int IsLeaf(const LstNode * pNode)
	{
		assert(pNode);
		return pNode ? BIN(pNode->GetEdgeCount() == 0) : 0;
	}
	/*
	static LstNode * GetParent(const LstNode * pNode)
	{
		return (pNode && pNode->P_UpEdge) ? pNode->P_UpEdge->P_SrcNode : 0;
	}
	*/
	static uint GetParentP(const LstNode * pNode)
	{
		return (pNode && pNode->P_UpEdge) ? pNode->P_UpEdge->SrcNodeP : 0;
	}
	static int GetStringLength(const LstNode * pNode)
	{
		int depth = 0;
		if(pNode) {
			while(!IsRoot(pNode)) {
				depth += LstEdge::GetLength(pNode->P_UpEdge);
				pNode = pNode->P_UpEdge->P_SrcNode;
			}
		}
		return depth;
	}
	LstNode(int idx)
	{
		static uint LastId = 0;
		MEMSZERO(iteration);
		P_UpEdge = 0;
		P_SuffixLinkNode = 0;
		Index = idx;
		visitors = 0;
		BusVisited = 0;
		Id = LastId++;
	}
	~LstNode()
	{
	}
	//
	// Временная функция необходимая на этапе рефакторинга.
	// Обеспечивает надежное значение количества ветвей, выходящих из узла this.
	//
	uint    GetEdgeCount() const
	{
		uint   _c = 0;
		for(uint i = 0; i < EdgeL.getCount(); i++) {
			if(EdgeL.at(i) != 0)
				_c++;
		}
		return _c;
	}
	int    DeleteEdge(LstEdge * p)
	{
		int    ok = 0;
		if(p) {
			for(uint i = 0; !ok && i < EdgeL.getCount(); i++) {
				if(EdgeL.at(i) == p) {
					EdgeL.atFree(i);
					ok = 1;
				}
			}
		}
		return ok;
	}
	TSCollection <LstEdge> EdgeL;
	TAILQ_ENTRY(LstNode) iteration;  // For DFS/BFS iteration, we maintain a list as well.
	LstEdge * P_UpEdge;
	LstNode * P_SuffixLinkNode;
	int    Index;
	uint   Id;
	uint32 visitors;
	uint   BusVisited;
};

struct LstPathEnd;
//
// LST_NodeVisitCB - callback signature for node visits.
// @node: node currently visited.
// @data: arbitrary data passed through.
//
// This is the signature of the callbacks used in several of
// the algorithms below, that iterate over the tree. They
// call a callback of this signature for every node they visit.
//
// Returns: value > 0 if the iteration algorithm that called
// this node is to proceed beyond this node, or 0 if not. Note
// that this does not necessarily mean that an algorithm will
// abort when the return value is 0.
//
typedef int (*LST_NodeVisitCB)(LstSuffixTree * pTree, LstNode * node, void * data);

struct LstSuffixTree {
	LstSuffixTree()
	{
		P_Phase = 0;
		Ext = 0;
		P_Root = 0;
		LastStringIndex = 0;
		Visitors = 0;
		Flags = 0;
		Flags |= LstSuffixTree::fDup;
		THROW(P_Root = new LstNode(-1));
		NodeL.insert(
		CATCH
			ZDELETE(P_Root);
			Flags |= fError;
		ENDCATCH
	}
	~LstSuffixTree()
	{
		Clear();
	}
	void   Clear();
	int    GetStringIndex(const LstString * pString) const;
	int    AddString(const LstString & rString, uint * pIdx);
	//
	// lst_node_get_string - returns the string on path from root to node.
	// @node: node whose string to return.
	// @max_depth: make string no longer than @max_depth items.
	//
	// Returns: A newly allocated string consisting of all the string
	// elements found when iterating from the root down to @node.
	//
	LstString * GetString(const LstNode * pNode, int max_len);
	LstNode * GetNode(uint nodeP)
	{
		return (nodeP > 0 && nodeP <= NodeL.getCount()) ? NodeL.at(nodeP-1) : 0;
	}

	int    AddStringImpl(const LstString & rString);
	uint   FollowStringSlow(LstNode * pNode, const LstString & rString, LstPathEnd & pEnd);
	void   FollowString(LstNode * pNode, LstStringIndex * pSkipString, LstPathEnd & pEnd);
	LstEdge * FindEdgeWithStartitem(const LstNode * pNode, const LstString * pString, uint index) const;
	LstNode * GetSkipString(const LstPathEnd & rEnd, LstStringIndex & rSkipString);
	void   DeleteEdge(LstEdge * pEdge);
	//
	// lst_alg_dfs - depth-first search of suffix tree.
	// @tree: suffix tree to iterate.
	// @callback: callback to call for each node.
	// @data: user data passed through to callback.
	// 
	// The algorithm iterates the tree in depth-first order, calling
	// @callback for each node visited.
	//
	void   AlgDFS(LST_NodeVisitCB callback, void * pData);
	LstStringSet * AlgLongestSubstring(uint min_len, uint max_len, int lcs);
	//
	// lst_alg_longest_common_substring - computes the lcs for a suffix tree.
	// @tree: tree to use in computation.
	// @min_len: minimum length that common substrings must have to be returned.
	// @max_len: don't return strings longer than @max_len items.
	//
	// The algorithm computes the longest common substring(s) in @tree
	// and returns them as a new string set. This is currently a suboptimal
	// O(n^2) implementation until I have time for the more sophisticated
	// O(n) implementation available. If you want to limit the string length,
	// pass an appropriate value for @max_len, or pass 0 if you want the
	// longest string(s) possible. Similarly, if you want to receive only
	// longest common substrings of at least a certain number of items, use
	// @min_len for that, or pass 0 to indicate interest in everything.
	//
	// Returns: new string set, or %NULL when no strings were found.
	//
	LstStringSet * AlgLongestCommonSubstring(uint min_len, uint max_len);
	//
	// lst_alg_longest_repeated_substring - computes the lrs for a suffix tree.
	// @tree: tree to use in computation.
	// @min_len: minimum length that repeated substrings must have to be returned.
	// @max_len: don't return strings longer than @max_len items.
	//
	// The algorithm computes the longest repeated substring(s) in @tree
	// and returns them as a new string set. This is currently a suboptimal
	// O(n^2) implementation until I have time for the more sophisticated
	// O(n) implementation available. If you want to limit the string length,
	// pass an appropriate value for @max_len, or pass 0 if you want the
	// longest string(s) possible. Similarly, if you want to receive only
	// longest repeated substrings of at least a certain number of items, use
	// @min_len for that, or pass 0 to indicate interest in everything.
	//
	// Returns: new string set, or %NULL when no strings were found.
	//
	LstStringSet * AlgLongestRepeatedSubstring(uint min_len, uint max_len);
	//
	// lst_alg_bus - bottom-up search of suffix tree.
	// @tree: suffix tree to iterate.
	// @callback: callback to call for each node.
	// @data: user data passed through to callback.
	//
	// The algorithm iterates the tree in bottom-up order, calling @callback
	// for each node visited. This algorithm ignores the return value of @callback.
	//
	void   AlgBus(LST_NodeVisitCB callback, void * data);
	//
	// lst_alg_bfs - breadth-first search of suffix tree.
	// @tree: suffix tree to iterate.
	// @callback: callback to call for each node.
	// @data: user data passed through to callback.
	//
	// The algorithm iterates the tree in breadth-first order, calling
	// @callback for each node visited.
	//
	void   AlgBFS(LST_NodeVisitCB callback, void * data);
	//
	// lst_alg_leafs - iterates all leafs in a suffix tree.
	// @tree: suffix tree to visit.
	// @callback: callback to call for each node.
	// @data: user data passed through to callback.
	//
	// The algorithm iterates over all leafs in the tree, calling @callback
	// for each node visited. If @callback returns 0, it stops.
	//
	void   AlgLeafs(LST_NodeVisitCB callback, void * data);
	//
	// Current phase of Ukkonen's algorithm. In order to implement the "Once a leaf, always a leaf"
	// Trick as explained by Gusfield, we make this a pointer to an integer.
	//
	uint * P_Phase;
	//
	// To avoid the O(m) cost of setting that value in stone once a string insertion is over, we make phase
	// point into the following list. A new element is created for every string insertion.
	//
	TSCollection <uint> Phases;
	uint   Ext; // Current extension of Ukkonen's algorithm.
	LstNode * P_Root; // Well ... guess :)
	TSCollection <LstNode> NodeL;
	//
	// A simple table for the strings in the tree, mapping
	// string indices (starting from 1) to strings.
	//
	TSCollection <LstString> SC;
	//
	int    LastStringIndex; // A counter for string index numbers
	uint   Visitors; // If needs_visitors_update is 0, this visitors value can still be used.
	enum {
		fDup               = 0x0001, // Whether or not we allow duplicates in our tree
			// After each string insertion, the visitor bitstrings in the nodes are outdated. We note this in the following
			// flag. It's cleared whenever lst_alg_set_visitors() is called, which happens if necessary at the beginning of
			// lst_stree_remove_string().
		fNeedVisitorUpdate = 0x0002,
		fError             = 0x0004  // Ошибка создания дерева
	};
	long   Flags;
	//
	// lst_alg_set_visitors - builds visitor bitstrings in tree nodes.
	// @tree: tree to update.
	//
	// The algorithm updates the visitor elements in each node of @tree
	// to contain a one-bit for each string index that is contained
	// in the tree.
	//
	// Returns: bitstring representing a node visited by all strings.
	//
	uint   AlgSetVisitors();
private:
	//
	// Descr: Проводит новое ребро между pSrcNode и pDestNode
	//
	LstEdge * EdgeLeafNew(LstNode * pSrcNode, LstNode * pDestNode, const LstString * pString, uint startIndex);
	//
	// Descr: Создает новый узел LstNode(newDestNodeIdx) и проводит новое ребро между pSrcNode и новым узлом
	//
	LstEdge * EdgeLeafNew(LstNode * pSrcNode, int newDestNodeIdx, const LstString * pString, uint startIndex);
	int    ExtendAtNode(const LstString & rString, LstPathEnd & rEnd, uint extendIndex, int * pStopExtensions);
	LstNode * ExtendAtEdge(const LstString & rString, LstPathEnd & rEnd, uint extendIndex, int * pStopExtensions);
};
/**
 * lst_string_new - creates new string.
 * @data: data to store in string.
 * @item_size: size of a single string item, in bytes.
 * @num_items: number of items the string stores.
 *
 * The function creates @item_size * @num_items bytes of memory
 * and copies @data into that area, then returns the new string.
 *
 * Returns: new string, or %NULL when out of memory.
 */
LstString * lst_string_new(const void * data, uint item_size, uint num_items);
/**
 * lst_string_eq - string item comparison.
 * @s1: first string.
 * @item1: item in @s1.
 * @s2: second string.
 * @item2: item in @s2.
 *
 * The function compares the items specified via the input parameters.
 * The way this is implemented depends on the string class for the
 * strings involved, see lst_string_set_class().
 *
 * Returns: value > 0 when equal, 0 otherwise.
 */
int lst_string_eq(const LstString * s1, uint item1, const LstString * s2, uint item2);
/**
 * lst_string_items_common - find string overlap at specific indices.
 * @s1: first string.
 * @off1: item in @s1.
 * @s2: second string.
 * @off2: item in @s2.
 * @max_len: maximum number of items to compare.
 *
 * The function compares items in @s1 and @s2 from the given offsets,
 * counting how many are equal. The way the comparison works depends on
 * the string class active for the strings involved, see lst_string_set_class().
 *
 * Returns: number of identical items.
 */
uint lst_string_items_common(const LstString * s1, uint off1, const LstString * s2, uint off2, uint max_len);
//
//
//
#ifdef __cplusplus
}
#endif
#endif
