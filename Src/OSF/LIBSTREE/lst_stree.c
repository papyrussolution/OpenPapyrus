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
#include "libstree.h"
#include <sys/queue.h>

static int   string_id_counter;
static int   string_byte_cmp_func(char * item1, char * item2);
static void  string_byte_copy_func(char * src, char * dst);

SString & LstString::ToStr(SString & rBuf) const
{
	rBuf = 0;
	LstStringIndex tmp_range(this, 0, getCount(), 0);
	return tmp_range.Print(rBuf);
}	

LstString * lst_string_new(const void * data, uint item_size, uint num_items)
{
	LstString * string = 0;
	if(item_size) {
		string = new LstString(item_size);
		if(string) {
			string->Id = ++string_id_counter;
			if(data && num_items) {
				//
				// Logically, we want one more item than given; we treat that as a
				// special end-of-string marker so that no suffix of our string can ever
				// be the prefix of another suffix. For the problems that this would cause, see Gusfield.
				//
				string->insertChunk(num_items, data);
			}
		}
	}
	return string;
}

int lst_string_eq(const LstString * s1, uint item1, const LstString * s2, uint item2)
{
	if(!s1 || !s2)
		return 0;
	else {
		const uint c1 = s1->getCount();
		const uint c2 = s2->getCount();
		if(item1 >= (s1->getCount()+1) || item2 >= (s2->getCount()+1))
			return 0;
		else {
			// Treat the end-of-string markers separately:
			if(item1 == c1 && item2 == c2) 
				if(s1 == s2)
					return 1; // Comparing end of identical strings
				else
					return 0; // Comparing end of different strings
			else if(item1 == c1 || item2 == c2)
				return 0; // Comparing end and non-end
			else {
				const size_t is1 = s1->getItemSize();
				const size_t is2 = s2->getItemSize();
				if(is1 != is2)
					return 0;
				else {
					const void * p1 = s1->at(item1);
					const void * p2 = s2->at(item2);
					switch(is1) {
						case 1: 
							return BIN(*PTR8(p1) == *PTR8(p2));
						case 2:
							return BIN(*PTR16(p1) == *PTR16(p2));
						case 4:
							return BIN(*PTR32(p1) == *PTR32(p2));
						default:
							return BIN(memcmp(p1, p2, is1) == 0);
					}
				}
			}
		}
	}
}

uint lst_string_items_common(const LstString * s1, uint off1, const LstString * s2, uint off2, uint max_len)
{
	if(!s1 || !s2 || off1 > s1->getCount() || off2 > s2->getCount())
		return 0;
	uint   len = MIN(MIN((s1->getCount() + 1) - off1, (s2->getCount() + 1) - off2), max_len);
	for(uint i = 0; i < len; i++) {
		if(!lst_string_eq(s1, off1 + i, s2, off2 + i))
			return i;
	}
	return len;
}

SString & lst_debug_print_substring(const LstString & rString, uint startIdx, uint endIdx, uint extraIdx, SString & rBuf)
{
	LstStringIndex tmp_range(&rString, startIdx, endIdx, extraIdx);
	return tmp_range.Print(rBuf);
}

LstEdge::LstEdge(LstNode * pSrcNode, LstNode * pDestNode, const LstString * pString, uint startIndex, uint endIndex) :
	Range(pString, startIndex, endIndex, 0)
{
	//MEMSZERO(Siblings);
	P_SrcNode = pSrcNode;
	P_DestNode = pDestNode;
	pDestNode->P_UpEdge = this;
	//LIST_INSERT_HEAD(&pSrcNode->Kids, this, Siblings);
	pSrcNode->EdgeL.insert(this);
}
//
// A path in an implicit suffix tree can end at either a node, or
// at some point in the label of an edge. We remember in each
// extension where we ended using an LstPathEnd structure.
//
struct LstPathEnd {
	void SetNode(LstNode * pNode)
	{
		THISZERO();
		P_Node = pNode;
	}
	void SetEdge(LstEdge * pEdge, uint offs)
	{
		THISZERO();
		P_Edge = pEdge;
		Offset = offs;
	}
	void Advance(LstEdge * pEdge)
	{
		if(P_Node) {
			if(LstEdge::GetLength(pEdge) == 1)
				SetNode(pEdge->P_DestNode);
			else
				SetEdge(pEdge, 1);
		}
		else {
			Offset++;
			if(Offset == (uint)LstEdge::GetLength(P_Edge))
				SetNode(P_Edge->P_DestNode);
		}
	}
	LstNode * P_Node;
	LstEdge * P_Edge;
	uint   Offset;
};

LstEdge * LstSuffixTree::EdgeLeafNew(LstNode * pSrcNode, LstNode * pDestNode, const LstString * pString, uint startIndex)
{
	LstEdge * p_edge = new LstEdge(pSrcNode, pDestNode, pString, startIndex, 0);
	if(p_edge) {
		//
		// For a leaf edge, we make the end index point to the current phase pointer in the tree structure.
		//
		p_edge->Range.P_EndIndex = P_Phase;
	}
	return p_edge;
}

LstEdge * LstSuffixTree::EdgeLeafNew(LstNode * pSrcNode, int newDestNodeIdx, const LstString * pString, uint startIndex)
{
	LstEdge * p_edge = 0;
	LstNode * p_new_dest_node = new LstNode(newDestNodeIdx);
	if(p_new_dest_node) {
		p_edge = new LstEdge(pSrcNode, p_new_dest_node, pString, startIndex, 0);
		if(p_edge) {
			//
			// For a leaf edge, we make the end index point to the current phase pointer in the tree structure.
			//
			p_edge->Range.P_EndIndex = P_Phase;
			//LIST_INSERT_HEAD(&Leafs, p_new_dest_node, NodeLeafs);
			NodeL.insert(p_new_dest_node);
		}
	}
	return p_edge;
}

LstEdge * LstSuffixTree::FindEdgeWithStartitem(const LstNode * pNode, const LstString * pString, uint index) const
{
	LstEdge * p_edge = 0;
	if(!pNode || !pString || index > pString->getCount()) {
		D0("Invalid input\n");
	}
	else {
		for(uint i = 0; !p_edge && i < pNode->EdgeL.getCount(); i++) {
			LstEdge * p_item = pNode->EdgeL.at(i);
			if(p_item && lst_string_eq(p_item->Range.P_String, p_item->Range.StartIndex, pString, index))
				p_edge = p_item;
		}
	}
	return p_edge;
}

void LstSuffixTree::DeleteEdge(LstEdge * pEdge)
{
	if(pEdge) {
		LstNode * p_node = pEdge->P_SrcNode;
		p_node->DeleteEdge(pEdge);
	}
}
/**
 * stree_follow_string_slow - follows an arbitrary string through the tree.
 * @tree: tree to query.
 * @node: node to start at.
 * @string: the string to follow.
 * @end: result argument, see below.
 *
 * The function follows the @skipstring in @tree starting from @node,
 * reporting where in the tree the string ends through the @end
 * result pointer.
 *
 * Returns: number of string items successfully matched.
 */
uint LstSuffixTree::FollowStringSlow(LstNode * pNode, const LstString & rString, LstPathEnd & rEnd)
{
	uint   items_done = 0;
	if(!pNode) {
		D0("Invalid input.\n");
		memzero(&rEnd, sizeof(LstPathEnd));
	}
	else {
		SString temp_buf;
		uint   items_todo = (rString.getCount()+1);
		//
		// Find p_edge where our string starts, making use of the fact
		// that no two out-edges from a pNode can start with the same character.
		//
		while(items_todo > 0) {
			LstEdge * p_edge = FindEdgeWithStartitem(pNode, &rString, items_done);
			if(!p_edge) {
				//D2("Mismatch at pNode %u, at %s\n", pNode->id, (const char *)lst_debug_print_substring(rString, 0, items_done, 0, temp_buf));
				rEnd.SetNode(pNode);
				return items_done;
			}
			else {
				uint   common = lst_string_items_common(p_edge->Range.P_String, p_edge->Range.StartIndex, &rString, items_done, items_todo);
				if(common < (uint)LstEdge::GetLength(p_edge)) {
					//D1("Mismatch in p_edge at %s\n", (const char *)lst_debug_print_substring(rString, 0, items_done + common, 0, temp_buf));
					rEnd.SetEdge(p_edge, common);
					return items_done + common;
				}
				else {
					pNode = p_edge->P_DestNode;
					items_done += LstEdge::GetLength(p_edge);
					items_todo -= LstEdge::GetLength(p_edge);
				}
			}
		}
		rEnd.SetNode(pNode);
	}
	return items_done;
}
/**
 * stree_follow_string - follows an existing string in the tree, using skip/count.
 * @tree: tree to query.
 * @node: node to start at.
 * @skipstring: the string to follow.
 * @end: result argument, see below.
 *
 * The function follows the @skipstring in @tree starting from @node,
 * reporting where in the tree the string ends through the @end
 * result pointer.
 */
void LstSuffixTree::FollowString(LstNode * pNode, LstStringIndex * pSkipString, LstPathEnd & rEnd)
{
	uint items_done = 0, common;
	SString temp_buf, temp_buf2;
	if(!pNode || !pSkipString) {
		D0("Invalid input.\n");
	}
	else {
		const LstString * p_string = pSkipString->P_String;
		assert(p_string);
		//D3("Overlaying p_string %s at pNode %u, empty: %i\n", (const char *)pSkipString->Print(temp_buf), pNode->id, (pSkipString->StartIndex == LST_EMPTY_STRING));
		/* We need to figure out how many p_string items we need to walk down in
		* the tree so that we can then extend by the next item.
		*/
		if(pSkipString->StartIndex == LST_EMPTY_STRING) {
			D0("Empty p_string -- nothing to follow.\n");
			rEnd.SetNode(pNode);
		}
		else {
			uint items_todo = *(pSkipString->P_EndIndex) - pSkipString->StartIndex + 1;
			if(items_todo == 0) {
				rEnd.SetNode(pNode);
			}
			else {
				//
				// Find p_edge where our p_string starts, making use of the fact
				// that no two out-edges from a pNode can start with the same character.
				//
				LstEdge * p_edge = 0;
				while(items_todo > 0) {
					uint edge_len;
					p_edge = FindEdgeWithStartitem(pNode, p_string, pSkipString->StartIndex + items_done);
					if(!p_edge) {
						rEnd.SetNode(pNode);
						return;
					}
					edge_len = (uint)LstEdge::GetLength(p_edge);
					//
					// Follow edges in tree, emplying the Skip/Count Trick as per Gusfield.
					// When the p_string we're looking up is longer than the p_edge's label,
					// we can just skip this p_edge:
					//
					if(items_todo >= edge_len) {
						items_todo -= edge_len;
						items_done += edge_len;
						if(items_todo == 0) {
							//D2("Skipped to pNode %u, last internal %u\n", p_edge->P_DestNode->id, pNode->id);
							rEnd.SetNode(p_edge->P_DestNode);
							return;
						}
						pNode = p_edge->P_DestNode;
						//D1("Skipping to pNode %u.\n", pNode->id);
						continue;
					}
					//
					// When the p_string is shorter than the p_edge, we need to compare
					// the strings and figure out where we stop within the p_edge.
					// This will need a new p_edge as per extension Rule 2.
					//
					break;
				}
				assert(p_edge);
				common = lst_string_items_common(p_edge->Range.P_String, p_edge->Range.StartIndex, p_string, pSkipString->StartIndex + items_done, items_todo);
				p_edge->Range.Print(temp_buf);
				rEnd.SetEdge(p_edge, common);
			}
		}
	}
}
/**
 * stree_get_skipstring - finds the string range we need to cross up to previous node.
 * @tree: tree in which we search.
 * @end: definition of the tree location where we start going up.
 * @skipstring: result pointer, see below.
 *
 * The function finds the string range we need to jump over until
 * we get back up to a node that has a suffix link. If that node is
 * the root, the upstring will be empty.
 *
 * Returns: The node we arrive at. Also, the string range is returned
 * through @skipstring.
 */
LstNode * LstSuffixTree::GetSkipString(const LstPathEnd & rEnd, LstStringIndex & rSkipString)
{
	LstNode * p_result = 0;
	if(rEnd.P_Node) {
		if(rEnd.P_Node->P_SuffixLinkNode) {
			//
			// The node we ended at already has a suffix link,
			// so we need to do nothing. Mark the pSkipString
			// as empty:
			//
			rSkipString.StartIndex = LST_EMPTY_STRING;
			D0("Suffix link at start node\n");
			p_result = rEnd.P_Node->P_SuffixLinkNode;
		}
		//
		// If the node doesn't have a suffix link directly, we must
		// hop up over the complete edge's string. If we pEnd up at
		// the root, the caller must descend as in the naive algorithm,
		// otherwise the pSkipString is just whatever the string attached
		// to the edge is:
		//
		else if(rEnd.P_Node == P_Root) {
			D0("End node is root\n");
			rSkipString.StartIndex = LST_EMPTY_STRING;
			p_result = P_Root;
		}
		else {
			LstNode * p_parent_node = LstNode::GetParent(rEnd.P_Node);
			if(p_parent_node == P_Root) {
				D0("Parent is root\n");
				p_result = P_Root;
			}
			else {
				rSkipString = rEnd.P_Node->P_UpEdge->Range;
				//
				// Follow the node up the edge, and cross over across
				// the suffix link. Then return the node we arrive at.
				//
				D0("Suffix link up from node\n");
				p_result = p_parent_node->P_SuffixLinkNode;
			}
		}
	}
	else {
		// Okay -- the funkier case: we start in the middle of an edge.
		assert(rEnd.P_Edge);
		LstNode * parent_node = rEnd.P_Edge->P_SrcNode;
		if(parent_node == P_Root) {
			D0("Edge src is root\n");
			p_result = P_Root;
		}
		else {
			SString temp_buf;
			rSkipString.P_String = rEnd.P_Edge->Range.P_String;
			rSkipString.StartIndex = rEnd.P_Edge->Range.StartIndex;
			*rSkipString.P_EndIndex = rSkipString.StartIndex + rEnd.Offset - 1;
			p_result = parent_node->P_SuffixLinkNode;
		}
	}
	return p_result;
}
/**
 * stree_extend_at_node - extends a tree by one string item, from a node.
 * @tree: tree to extend.
 * @string: string containing the new item.
 * @end: definition of where to extend from.
 * @extend_index: index in @string to insert into tree.
 * @stop_extensions: result pointer, see below.
 *
 * The function extends the tree by a single string item, down from
 * a node within the tree. The point of insertion is given through @end.
 *
 * Returns: 1 in @stop_extensions if Rule 3 (see Gusfield) applied and we
 * can hence stop extensions for the current phase.
 */
int LstSuffixTree::ExtendAtNode(const LstString & rString, LstPathEnd & rEnd, uint extendIndex, int * pStopExtensions)
{
	int    ok = 1;
	if(LstNode::IsLeaf(rEnd.P_Node)) {
		//D0("Rule 1 -- extending label.\n");
		rEnd.P_Node->P_UpEdge->Range.P_EndIndex[0]++;
		rEnd.SetEdge(rEnd.P_Node->P_UpEdge, LstEdge::GetLength(rEnd.P_Node->P_UpEdge) - 2);
	}
	else {
		LstEdge * p_edge = FindEdgeWithStartitem(rEnd.P_Node, &rString, extendIndex);
		if(!p_edge) {
			//
			// Extension Rule 2:
			//
			//D0("Rule 2 -- adding edge.\n");
			THROW(p_edge = EdgeLeafNew(rEnd.P_Node, Ext, &rString, extendIndex));
		}
		else {
			//
			// Otherwise it's Extension Rule 3, so we do nothing,
			// but only mark that we've applied that rule so we can speed things up.
			//
			//D0("Rule 3 -- marked for stop.\n");
			rEnd.Advance(p_edge);
			*pStopExtensions = 1;
		}
	}
	CATCHZOK
	return ok;
}
/**
 * stree_extend_at_edge - extends a tree by one string item, from within an edge.
 * @tree: tree to extend.
 * @string: string containing the new item.
 * @end: definition of where to extend from.
 * @extend_index: index in @string to insert into tree.
 * @stop_extensions: result pointer, see below.
 *
 * The function extends the tree by a single string item, down from within
 * an edge in the tree. The point of insertion is given through @end.
 *
 * Returns: 1 in @stop_extensions if Rule 3 (see Gusfield) applied and we
 * can hence stop extensions for the current phase.
 */
LstNode * LstSuffixTree::ExtendAtEdge(const LstString & rString, LstPathEnd & rEnd, uint extendIndex, int * pStopExtensions)
{
	LstNode * p_new_inner_node = 0;
	LstEdge * p_new_edge = 0;
	LstEdge & r_end_edge = *rEnd.P_Edge;
	if(lst_string_eq(&rString, extendIndex, r_end_edge.Range.P_String, r_end_edge.Range.StartIndex + rEnd.Offset)) {
		//D2("Rule 3 within edge %u-%u -- marked for stop.\n", r_end_edge.P_SrcNode->id, r_end_edge.P_DestNode->id);
		rEnd.Advance(0);
		*pStopExtensions = 1;
	}
	else {
		THROW(p_new_inner_node = new LstNode(-1));
		LstNode * old_node = r_end_edge.P_DestNode;
		//
		// Carefully carefully carefully -- when we split a leaf edge,
		// we need to figure out what kind of pEnd index to use (the edge-local
		// or global one). It's not enough to check whether it's a leaf or
		// not -- it could be a leaf created for a previous pString. So only
		// make the pEnd index point back into the pTree if it was pointing there
		// originally anyway. However, pTree->phase changes over time, so we
		// must not use that.
		//
		if(LstNode::IsLeaf(old_node)) {
			uint * p_end_index = r_end_edge.Range.P_EndIndex;
			THROW(p_new_edge = EdgeLeafNew(p_new_inner_node, old_node, r_end_edge.Range.P_String, r_end_edge.Range.StartIndex + rEnd.Offset));
			p_new_edge->Range.P_EndIndex = p_end_index;
		}
		else {
			THROW(p_new_edge = new LstEdge(p_new_inner_node, old_node, r_end_edge.Range.P_String, r_end_edge.Range.StartIndex + rEnd.Offset, *r_end_edge.Range.P_EndIndex));
		}
		r_end_edge.Range.P_EndIndex = &r_end_edge.Range.EndIndexLocal;
		*r_end_edge.Range.P_EndIndex = r_end_edge.Range.StartIndex + rEnd.Offset - 1;
		r_end_edge.P_DestNode = p_new_inner_node;
		p_new_inner_node->P_UpEdge = &r_end_edge;
		//
		// Now add another edge to the new node inserted, and label it with the remainder of the pString.
		//
		THROW(p_new_edge = EdgeLeafNew(p_new_inner_node, Ext, &rString, extendIndex));
	}
	CATCH
		ZDELETE(p_new_inner_node);
	ENDCATCH
	return p_new_inner_node;
}
/**
 * stree_add_string - inserts a new string into the tree.
 * @tree: tree to insert into.
 * @string: new string to insert.
 *
 * The function inserts a string into the tree. It recognizes whether
 * it's the first or an additional string and applies Extension Rules
 * 1-3 accordingly.
 */
int LstSuffixTree::AddStringImpl(const LstString & rString)
{
	int    ok = 1;
	LstPathEnd end;
	LstNode * p_node;
	LstNode * p_start_leaf = 0;
	LstNode * p_inner_node;
	LstNode * p_prev_new_node = 0;
	LstNode * p_new_inner_node;
	LstEdge * p_edge;
	uint i, j, stop_extensions = 0, last_extension = 0, use_end = 0, find_skipstring = 1;
	SString outp_buf;
	//D1("Inserting pString %s\n", (const char *)rString.ToStr(outp_buf));
	{
		//
		// stree_next_phase - Trick 3 management.
		// @tree: tree to advance phase pointer in.
		//
		// This function takes care of allowing Trick 3 to work correctly
		// when multiple strings are inserted into the tree, and helps avoiding
		// the O(m) overhead to create a true suffix tree from the intermediate
		// one Gusfield mentions in section 6.1.6. We don't exchange any labels
		// when we consider edge label ends "final", we rather start using
		// different pointers to end offsets from now on, not touching the old
		// ones any more in the future.
		//
		uint * p_phase = 0;
		THROW(p_phase = new uint);
		Phases.insert(p_phase);
		P_Phase = p_phase;
		D0("New tree extension pointer.\n");
	}
	//
	// First and subsequent pString insertions are handled slightly
	// differently -- first one is built from scratch, the others
	// are built after matching them onto the current tree as much
	// as possible, and then proceeding as usual.
	//
	assert(SC.getCount()); // “ак как переданна€ строка rString должны быть в коллекции SC, то SC.getCount() 
		// никак не может быть нулевым
	if(SC.getCount() == 1) { // ѕерва€ добавленна€ строка
		D0("No strings in tree yet.\n");
		THROW(p_edge = EdgeLeafNew(P_Root, (int)0, &rString, 0));
		p_node = p_edge->P_DestNode;
		//
		// Construct first tree -- simply enter a single edge with a single-item label.
		//
		D0("Phase 0 started.\n");
		p_start_leaf = p_node;
		end.SetEdge(p_start_leaf->P_UpEdge, 0);
		last_extension = 0;
		//D_PRINT_TREE(tree);
		// Skip to phase 1:
		i = 1;
	}
	else {
		//
		// Follow new pString through tree, hence finding the number of phases we can skip:
		//
		i = FollowStringSlow(P_Root, rString, end);
		last_extension = -1;
		find_skipstring = 0;
		//D1("Starting new pString at phase %u\n", i);
	}
	//
	// Following Ukkonen's algorithm, we insert the new pString
	// of length |m| in m "phases", constructing tree i+1 from
	// tree i. Each phase i+1 consists of i+1 "extensions", one
	// for each suffix in the pString s[1,i+1].
	//
	// By including the last pString item (our end-of-pString marker),
	// we automatically avoid the issues of having suffixes that are prefixes of other suffixes.
	//
	assert(i != 0);
	for(/* i inited above */; i < (rString.getCount()+1); i++) {
		//D2("Phase %i started ------------------------- %s\n", i, (const char *)lst_debug_print_substring(rString, 0, i-1, i, outp_buf));
		*P_Phase = i;
		use_end = 0;
		if(stop_extensions)
			use_end = 1;
		stop_extensions = 0;
		//
		// Now do the remaining extensions. We don't start at index 0
		// with extensions but rather implement speedup Trick 3 as per Gusfield.
		//
		for(j = last_extension + 1; j < i + 1 && !stop_extensions; j++) {
			Ext = j;
			//D2("Phase %u, extension %i started.\n", i, j);
			//
			// Get the node from which we start to walk down,
			// either found via suffix links or because it's the root:
			//
			LstStringIndex skipstring(&rString, 0, 0, i);
			if(use_end) {
				D0("Re-using last phase's pString end\n");
				use_end = 0;
			}
			else if(find_skipstring) {
				p_node = GetSkipString(end, skipstring);
				if(p_node == P_Root) {
					if(skipstring.StartIndex != LST_EMPTY_STRING) {
						//D0("Starting at root\n");
						//
						// It's the root node -- just follow the path down in the tree as in the naive algorithm.
						//
						skipstring.P_String = &rString;
						skipstring.StartIndex = j;
						*(skipstring.P_EndIndex) = i-1;
						if((i-1) < j)
							skipstring.StartIndex = LST_EMPTY_STRING;
					}
				}
				else {
					//D0("Using suffix link\n");
					//
					// It's not the root node -- exploit suffix
					// links as much as possible. stree_get_skipstring()
					// has already filled in stuff for us.
					//
				}
				FollowString(p_node, &skipstring, end);
			}
			find_skipstring = 1;
			p_new_inner_node = 0;
			p_inner_node = 0;
			//
			// Now extend the found path in the tree by the new character in this phase:
			//
			if(end.P_Node) {
				//
				// We followed the path up to a node. If that node is a leaf,
				// we're done per Extension Rule 1. Otherwise we need to find
				// out if our new character is somewhere on the out-edges
				// of that node, and if not, hook in a new edge.
				//
				p_inner_node = end.P_Node;
				THROW(ExtendAtNode(rString, end, skipstring.ExtraIndex, (int*)&stop_extensions));
			}
			else {
				// 
				// We followed the path down to somewhere within an edge label. 
				// Now we need to check if the item following the common part in the label is what we want to add; 
				// that's a case of Extension Rule 3. Otherwise we need to split
				// the edge and hook in a new edge with the new character.
				// 
				assert(end.P_Edge);
				p_new_inner_node = ExtendAtEdge(rString, end, skipstring.ExtraIndex, (int*)&stop_extensions);
			}
			// 
			// Now take care of suffix links: if we've created a new inner
			// node in the last extension, create a suffix link to either
			// the last inner node we've encountered above, or a new inner node, if we've created one.
			// 
			if(p_prev_new_node)
				p_prev_new_node->P_SuffixLinkNode = NZOR(p_new_inner_node, p_inner_node);
			p_prev_new_node = p_new_inner_node;
			//D_PRINT_TREE(tree);
		}
		//
		// Remember how far we got for the next phase. Also,
		// repeat the last extension if we aborted due to Rule 3.
		//
		last_extension = j - 1 - stop_extensions;
	}
	Flags |= LstSuffixTree::fNeedVisitorUpdate;
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int LstSuffixTree::AddString(const LstString & rString, uint * pIdx)
{
	int    ok = 1;
	uint   idx = 0;
	int    dup = 0;
	if(!(Flags & LstSuffixTree::fDup)) {
		LstPathEnd end;
		uint items_done = FollowStringSlow(P_Root, rString, end);
		if(items_done == rString.getCount())
			dup = 1;
	}
	if(!dup) {
		LstString * p_sc_item = new LstString(rString);
		THROW(p_sc_item);
		SC.insert(p_sc_item);
		idx = SC.getCount();
		AddStringImpl(*p_sc_item);
		ASSIGN_PTR(pIdx, idx);
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

void LstSuffixTree::Clear()
{
	ZDELETE(P_Root); // Clean up the tree itself
	Phases.freeAll();
	NodeL.freeAll();
	SC.freeAll();
}

int LstSuffixTree::GetStringIndex(const LstString * pString) const
{
	int    idx = 0;
	if(pString) {
		for(uint i = 0; !idx && i < SC.getCount(); i++) {
			const LstString * p_item = SC.at(i);
			if(p_item && p_item->Id == pString->Id)
				idx = (int)(i+1);
		}
	}
	return idx;
}

LstString * LstSuffixTree::GetString(const LstNode * pNode, int max_len)
{
	LstString * result = 0;
	if(!LstNode::IsRoot(pNode)) {
		int depth_orig = LstNode::GetStringLength(pNode);
		int depth = depth_orig;
		//D1("String depth is %i\n", depth);
		result = lst_string_new(0, pNode->P_UpEdge->Range.P_String->getItemSize(), depth);
		while(!LstNode::IsRoot(pNode)) {
			const LstEdge * p_edge = pNode->P_UpEdge;
			const int edge_length = LstEdge::GetLength(p_edge);
			const int start = p_edge->Range.StartIndex;
			for(int i = 0; i < edge_length; i++, depth--) {
				//
				// We never copy the last item around, as we're taking care of that
				// through the string constructor anyway
				//
				if(!p_edge->Range.P_String->IsTerminal(start + i)) {
					//D2("Copying item from %i to %i\n", i, depth - 1);
					result->insert(p_edge->Range.P_String->at(start + i));
				}
			}
			/*
			for(int i = LstEdge::GetLength(p_edge) - 1; i >= 0; i--, depth--) {
				//
				// We never copy the last item around, as we're taking care of that
				// through the string constructor anyway
				//
				if(p_edge->range.string->IsTerminal(p_edge->range.start_index + i)) {
					result->num_items--;
				}
				else {
					D(("Copying item from %i to %i\n", i, depth - 1));
					lst_string_item_copy(p_edge->range.string, p_edge->range.start_index + i, result, depth - 1);
				}
			}
			*/
			pNode = LstNode::GetParent(pNode);
		}
		/* Now crop the string to the desired max_len, if wanted: */
		if(max_len > 0 && max_len < depth_orig) {
			//lst_string_item_copy(result, depth_orig, result, max_len);
			//result->num_items = max_len + 1;
			assert(max_len == (int)result->getCount());
			result->insert(result->at(depth_orig));
		}
	}
	return result;
}
//
// Debug functions
//
struct LstSuffixTreeQi {
	LstSuffixTreeQi(LstNode * pNode)
	{
		THISZERO();
		node = pNode;
	}
	TAILQ_ENTRY(LstSuffixTreeQi) entries;
	LstNode * node;
};

void lst_debug_print_tree(LstSuffixTree * tree)
{
	SString line_buf, temp_buf;
	LstEdge * p_edge;
	TAILQ_HEAD(tailhead, LstSuffixTreeQi) queue;
	LstSuffixTreeQi * p_dummy_qi = new LstSuffixTreeQi(0);
	LstSuffixTreeQi * p_qi = new LstSuffixTreeQi(tree->P_Root);
	TAILQ_INIT(&queue);
	TAILQ_INSERT_HEAD(&queue, p_qi, entries);
	TAILQ_INSERT_TAIL(&queue, p_dummy_qi, entries);
	while(queue.tqh_first) {
		p_qi = queue.tqh_first;
		TAILQ_REMOVE(&queue, queue.tqh_first, entries);
		const LstNode * p_node = p_qi->node;
		if(!p_node) {
			if(queue.tqh_first) {
				p_dummy_qi = new LstSuffixTreeQi(0);
				TAILQ_INSERT_TAIL(&queue, p_dummy_qi, entries);
			}
		}
		else {
			/* Output current node: */
			(line_buf = 0).CatChar('[').Cat(p_node->Id).Space().CatChar('(').Cat(p_node->visitors).CatChar(')');
			if(p_node->P_SuffixLinkNode) {
				line_buf.Space().Cat("->").Space().Cat(p_node->P_SuffixLinkNode->Id);
			}
			line_buf.CatChar(']').CR();
			fputs(line_buf, stderr);
			//
			// Now output p_edge labels and adjacent-node ids:
			//
			if(!p_node->GetEdgeCount()) {
				(line_buf = 0).Tab().CatParStr("leaf").CR();
				fputs(line_buf, stderr);
			}
			else {
				//for(p_edge = p_qi->node->Kids.lh_first; p_edge; p_edge = p_edge->Siblings.le_next) {
				const uint _ec = p_node->EdgeL.getCount();
				for(uint i = 0; i < _ec; i++) {
					p_edge = p_node->EdgeL.at(i);
					if(p_edge) {
						//if(p_edge->P_DestNode->Kids.lh_first) {
						if(p_edge->P_DestNode->EdgeL.getCount()) {
							p_edge->Range.Print(temp_buf = 0);
							(line_buf = 0).Tab().CatChar('\'').Cat(temp_buf).CatChar('\'').Space().Cat(p_edge->P_DestNode->Id);
							//p_edge->range.string->ToStr(temp_buf);
							//line_buf.Space().CatParStr(temp_buf);
							line_buf.CR();
							fputs(line_buf, stderr);
							p_qi = new LstSuffixTreeQi(p_edge->P_DestNode);
							TAILQ_INSERT_TAIL(&queue, p_qi, entries);
						}
						else {
							p_edge->Range.Print(temp_buf = 0);
							(line_buf = 0).Tab().CatChar('\'').Cat(temp_buf).CatChar('\'').Space().CatChar('[').Cat(p_edge->P_DestNode->Index).CatChar(']').Space().
								Cat(p_edge->P_DestNode->Id).CatChar((p_edge->Range.P_EndIndex == &p_edge->Range.EndIndexLocal) ? 'l' : 't').
								CatChar((p_edge->Range.P_EndIndex == tree->P_Phase) ? 'c' : ' ');
							//p_edge->range.string->ToStr(temp_buf = 0);
							//line_buf.Space().CatParStr(temp_buf);
							line_buf.CR();
							fputs(line_buf, stderr);
						}
					}
				}
			}
		}
	}
}
//
// Algorighms
//
struct LstNodeIt {
	LstNodeIt(LstNode * pNode)
	{
		THISZERO();
		P_Node = pNode;
	}
	TAILQ_ENTRY(LstNodeIt) items;
	LstNode * P_Node;
};

void LstSuffixTree::AlgBFS(LST_NodeVisitCB callback, void * data)
{
	TAILQ_HEAD(qhead, LstNode) queue;
	if(callback) {
		TAILQ_INIT(&queue);
		TAILQ_INSERT_HEAD(&queue, P_Root, iteration);
		while(queue.tqh_first) {
			LstNode * node = queue.tqh_first;
			TAILQ_REMOVE(&queue, queue.tqh_first, iteration);
			if(callback(this, node, data)) {
				const uint _ec = node->EdgeL.getCount();
				//for(LstEdge * edge = node->Kids.lh_first; edge; edge = edge->Siblings.le_next) {
				for(uint i = 0; i < _ec; i++) {
					LstEdge * p_edge = node->EdgeL.at(i);
					if(p_edge) {
						TAILQ_INSERT_TAIL(&queue, p_edge->P_DestNode, iteration);
					}
				}
			}
		}
	}
}

void LstSuffixTree::AlgDFS(LST_NodeVisitCB callback, void * data)
{
	TAILQ_HEAD(shead, LstNode) stack;
	if(callback) {
		TAILQ_INIT(&stack);
		TAILQ_INSERT_HEAD(&stack, P_Root, iteration);
		while(stack.tqh_first) {
			LstNode * node = stack.tqh_first;
			TAILQ_REMOVE(&stack, stack.tqh_first, iteration);
			if(callback(this, node, data)) {
				const uint _ec = node->EdgeL.getCount();
				//for(LstEdge * edge = node->Kids.lh_first; edge; edge = edge->Siblings.le_next) {
				for(uint i = 0; i < _ec; i++) {
					LstEdge * p_edge = node->EdgeL.at(i);
					assert(p_edge);
					if(p_edge) {
						TAILQ_INSERT_HEAD(&stack, p_edge->P_DestNode, iteration);
					}
				}
			}
		}
	}
}

static int alg_clear_busflag(LstSuffixTree * pTree, LstNode * pNode, void * data)
{
	pNode->BusVisited = 0;
	return 1;
}

void LstSuffixTree::AlgBus(LST_NodeVisitCB callback, void * data)
{
	TAILQ_HEAD(nodes_s, LstNodeIt) nodes;
	TAILQ_INIT(&nodes);
	AlgBFS(alg_clear_busflag, NULL);
	//if(Leafs.lh_first) {
	if(NodeL.getCount()) {
		{
			/*
			for(LstNode * p_node = Leafs.lh_first; p_node; p_node = p_node->NodeLeafs.le_next) {
				callback(p_node, data);
				if(p_node != P_Root) {
					LstNode * p_upedge_srcnode = p_node->P_UpEdge->P_SrcNode;
					if(++p_upedge_srcnode->BusVisited == 1) {
						LstNodeIt * it = new LstNodeIt(p_upedge_srcnode);
						TAILQ_INSERT_TAIL(&nodes, it, items);
					}
				}
			}
			*/
			for(uint i = 0; i < NodeL.getCount(); i++) {
				LstNode * p_node = NodeL.at(i);
				if(p_node) {
					if(p_node != P_Root) {
						LstNode * p_upedge_srcnode = p_node->P_UpEdge->P_SrcNode;
						if(++p_upedge_srcnode->BusVisited == 1) {
							LstNodeIt * it = new LstNodeIt(p_upedge_srcnode);
							TAILQ_INSERT_TAIL(&nodes, it, items);
						}
					}
				}
			}
		}
		{
			while(nodes.tqh_first) {
				LstNodeIt * it = nodes.tqh_first;
				LstNode * p_node = it->P_Node;
				TAILQ_REMOVE(&nodes, nodes.tqh_first, items);
				if(p_node->BusVisited < p_node->GetEdgeCount()) {
					TAILQ_INSERT_TAIL(&nodes, it, items);
				}
				else {
					callback(this, p_node, data);
					delete it;
					if(p_node != P_Root) {
						LstNode * p_upedge_srcnode = p_node->P_UpEdge->P_SrcNode;
						if(++p_upedge_srcnode->BusVisited == 1) {
							it = new LstNodeIt(p_upedge_srcnode);
							TAILQ_INSERT_TAIL(&nodes, it, items);
						}
					}
				}
			}
		}
	}
}

void LstSuffixTree::AlgLeafs(LST_NodeVisitCB callback, void * data)
{
	if(callback) {
		/*
		for(LstNode * node = Leafs.lh_first; node; node = node->NodeLeafs.le_next) {
			if(callback(node, data) == 0)
				break;
		}
		*/
		for(uint i = 0; i < NodeL.getCount(); i++) {
			LstNode * p_node = NodeL.at(i);
			if(p_node) {
				if(callback(this, p_node, data) == 0)
					break;
			}
		}
	}
}

struct LstLcsData {
	LstLcsData(LstSuffixTree * pTree)
	{
		THISZERO();
		tree = pTree;
	}
	LstSuffixTree * tree;
	int    lcs; // 1 for longest common substring, 0 for longest repeated substring
	uint   all_visitors; // Used in LCS case only
	TAILQ_HEAD(nodes, LstNodeIt) nodes;
	int    deepest;
	int    num_deepest;
	int    max_depth;
};

static int alg_clear_visitors(LstSuffixTree * pTree, LstNode * node, void * data)
{
	node->visitors = 0;
	return 1;
}

static int alg_set_visitors(LstSuffixTree * pTree, LstNode * node, LstLcsData * data)
{
	if(LstNode::IsRoot(node)) {
		//D2("Node %u: visitors %i\n", node->id, node->visitors);
	}
	else {
		if(LstNode::IsLeaf(node)) {
			int index = 1 << data->tree->GetStringIndex(node->P_UpEdge->Range.P_String);
			node->visitors = index;
			node->P_UpEdge->P_SrcNode->visitors |= index;
		}
		else {
			node->P_UpEdge->P_SrcNode->visitors |= node->visitors;
		}
		if(node->P_UpEdge->P_SrcNode->visitors > data->all_visitors)
			data->all_visitors = node->P_UpEdge->P_SrcNode->visitors;
		//D2("Node %u: visitors %i\n", node->id, node->visitors);
	}
	return 1;
}

uint LstSuffixTree::AlgSetVisitors()
{
	uint   _visitors = 0;
	if(!(Flags & LstSuffixTree::fNeedVisitorUpdate))
		_visitors = Visitors;
	else {
		LstLcsData data(this);
		/* First, establish the visitor bitstrings in the tree. */
		AlgBus(alg_clear_visitors, NULL);
		AlgBus((LST_NodeVisitCB)alg_set_visitors, &data);
		Flags &= ~LstSuffixTree::fNeedVisitorUpdate;
		Visitors = data.all_visitors;
		_visitors = data.all_visitors;
	}
	return _visitors;
}

static int alg_find_deepest(LstNode * node, LstLcsData * data)
{
	LstNodeIt * it;
	int depth = LstNode::GetStringLength(node);
	if(data->lcs) {
		if(node->visitors != data->all_visitors)
			return 0;
	}
	else {
		if(node->GetEdgeCount() < 1)
			return 0;
	}
	if(data->deepest <= data->max_depth) {
		if(depth >= data->deepest) {
			it = new LstNodeIt(node);
			if(depth > data->deepest) {
				data->deepest = depth;
				data->num_deepest = 0;
			}
			data->num_deepest++;
			TAILQ_INSERT_HEAD(&data->nodes, it, items);
		}
	}
	else if(depth >= data->max_depth) {
		it = new LstNodeIt(node);
		data->num_deepest++;
		TAILQ_INSERT_HEAD(&data->nodes, it, items);
	}
	return 1;
}

LstStringSet * LstSuffixTree::AlgLongestSubstring(uint min_len, uint max_len, int lcs)
{
	LstStringSet * result = NULL;
	LstNodeIt * it;
	LstLcsData data(this);
	data.lcs = lcs;
	if(lcs)
		data.all_visitors = AlgSetVisitors();
	data.max_depth = (max_len > 0) ? (int)max_len : INT_MAX;
	TAILQ_INIT(&data.nodes);
	/* Now do a DSF finding the node with the largest string-
	* depth that has all strings as visitors.
	*/
	AlgDFS((LST_NodeVisitCB)alg_find_deepest, &data);
	//D2("Deepest nodes found -- we have %u longest substring(s) at depth %u.\n", data.num_deepest, data.deepest);

	/* Now, data.num_deepest tells us how many largest substrings
	* we have, and the first num_deepest items in data.nodes are
	* the end nodes in the suffix tree that define these substrings.
	*/
	while((it = data.nodes.tqh_first) != 0) {
		if(--data.num_deepest >= 0) {
			/* Get our longest common string's length, and if it's
			* long enough for our requirements, put it in the result
			* set. We need to allocate that first if we haven't yet
			* inserted any strings.
			*/
			if((uint)LstNode::GetStringLength(it->P_Node) >= min_len) {
				LstString * p_string = GetString(it->P_Node, (int)max_len);
				SETIFZ(result, new LstStringSet);
				result->insert(p_string);
			}
		}
		TAILQ_REMOVE(&data.nodes, it, items);
		delete it;
	}
	return result;
}

LstStringSet * LstSuffixTree::AlgLongestCommonSubstring(uint min_len, uint max_len)
{
	return AlgLongestSubstring(min_len, max_len, 1);
}

LstStringSet * LstSuffixTree::AlgLongestRepeatedSubstring(uint min_len, uint max_len)
{
	return AlgLongestSubstring(min_len, max_len, 0);
}
//
//
//
int main(int argc, char * argv[])
{
	const char * pp_filename_list[] = {
		"D:/Papyrus/Src/PPTEST/DATA/phrases.ru",
		"D:/Papyrus/Src/PPTEST/DATA/phrases.en"
	};
	SString buffer, line_buf;
	LstSuffixTree * p_tree = new LstSuffixTree;
	/*
	for(uint i = 0; i < SIZEOFARRAY(pp_filename_list); i++) {
		SFile inf(pp_filename_list[i], SFile::mRead);
		buffer = 0;
		if(inf.IsValid()) {
			while(inf.ReadLine(line_buf)) {
				buffer.Cat(line_buf);
			}
			LstString * p_new_string = lst_string_new((const char *)buffer, sizeof(char), buffer.Len());
			if(p_new_string) {
				p_tree->AddString(p_new_string);
			}
		}
	}
	*/
	{
		buffer = "xabxa";
		//(buffer = "моторное масло дл€ мотора масл€ного фильтра").ToOem();
		//(buffer = "мотор мотор мотор масло масло масло").ToOem();
		LstString * p_new_string = lst_string_new((const char *)buffer, sizeof(char), buffer.Len());
		p_tree->AddString(*p_new_string, 0);
		delete p_new_string;
	}
	lst_debug_print_tree(p_tree);
	//LstStringSet * p_set = p_tree->AlgLongestCommonSubstring(5, 5);
	//delete p_set;
	ZDELETE(p_tree);
	return 0;
}
