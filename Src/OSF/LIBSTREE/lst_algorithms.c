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

struct LstNodeIt {
	LstNodeIt(LstNode * pNode)
	{
		THISZERO();
		node = pNode;
	}
	TAILQ_ENTRY(LstNodeIt) items;
	LstNode * node;
};

static LstNodeIt * alg_node_it_new(LstNode * node)
{
	return new LstNodeIt(node);
}

void LstSuffixTree::AlgBFS(LST_NodeVisitCB callback, void * data)
{
	TAILQ_HEAD(qhead, LstNode) queue;
	if(callback) {
		TAILQ_INIT(&queue);
		TAILQ_INSERT_HEAD(&queue, P_Root, iteration);
		while(queue.tqh_first) {
			LstNode * node = queue.tqh_first;
			TAILQ_REMOVE(&queue, queue.tqh_first, iteration);
			if(callback(node, data)) {
				for(LstEdge * edge = node->Kids.lh_first; edge; edge = edge->Siblings.le_next)
					TAILQ_INSERT_TAIL(&queue, edge->P_DestNode, iteration);
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
			if(callback(node, data)) {
				for(LstEdge * edge = node->Kids.lh_first; edge; edge = edge->Siblings.le_next)
					TAILQ_INSERT_HEAD(&stack, edge->P_DestNode, iteration);
			}
		}
	}
}

static int alg_clear_busflag(LstNode * node, void * data)
{
	node->bus_visited = 0;
	return 1;
}

void LstSuffixTree::AlgBus(LST_NodeVisitCB callback, void * data)
{
	TAILQ_HEAD(nodes_s, LstNodeIt) nodes;
	TAILQ_INIT(&nodes);
	AlgBFS(alg_clear_busflag, NULL);
	if(Leafs.lh_first) {
		LstNode * node;
		for(node = Leafs.lh_first; node; node = node->NodeLeafs.le_next) {
			callback(node, data);
			if(node != P_Root) {
				node->P_UpEdge->P_SrcNode->bus_visited++;
				if(node->P_UpEdge->P_SrcNode->bus_visited == 1) {
					LstNodeIt * it = alg_node_it_new(node->P_UpEdge->P_SrcNode);
					TAILQ_INSERT_TAIL(&nodes, it, items);
				}
			}
		}
		while(nodes.tqh_first) {
			LstNodeIt * it = nodes.tqh_first;
			node = it->node;
			TAILQ_REMOVE(&nodes, nodes.tqh_first, items);
			if(node->bus_visited < node->NumKids) {
				TAILQ_INSERT_TAIL(&nodes, it, items);
			}
			else {
				callback(node, data);
				delete it;
				if(node != P_Root) {
					node->P_UpEdge->P_SrcNode->bus_visited++;
					if(node->P_UpEdge->P_SrcNode->bus_visited == 1) {
						it = alg_node_it_new(node->P_UpEdge->P_SrcNode);
						TAILQ_INSERT_TAIL(&nodes, it, items);
					}
				}
			}
		}
	}
}

void LstSuffixTree::AlgLeafs(LST_NodeVisitCB callback, void * data)
{
	if(callback) {
		for(LstNode * node = Leafs.lh_first; node; node = node->NodeLeafs.le_next) {
			if(callback(node, data) == 0)
				break;
		}
	}
}

struct LstLcsData {
	LstSuffixTree * tree;
	int    lcs; // 1 for longest common substring, 0 for longest repeated substring
	uint   all_visitors; // Used in LCS case only
	TAILQ_HEAD(nodes, LstNodeIt) nodes;
	int    deepest;
	int    num_deepest;
	int    max_depth;
};

static int alg_clear_visitors(LstNode * node, void * data)
{
	node->visitors = 0;
	return 1;
}

static int alg_set_visitors(LstNode * node, LstLcsData * data)
{
	if(LstNode::IsRoot(node)) {
		D2("Node %u: visitors %i\n", node->id, node->visitors);
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
		D2("Node %u: visitors %i\n", node->id, node->visitors);
	}
	return 1;
}

uint LstSuffixTree::AlgSetVisitors()
{
	uint   _visitors = 0;
	if(!(Flags & LstSuffixTree::fNeedVisitorUpdate))
		_visitors = Visitors;
	else {
		LstLcsData data;
		memzero(&data, sizeof(LstLcsData));
		data.tree = this;
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
		if(node->NumKids < 1)
			return 0;
	}
	if(data->deepest <= data->max_depth) {
		if(depth >= data->deepest) {
			it = alg_node_it_new(node);
			if(depth > data->deepest) {
				data->deepest = depth;
				data->num_deepest = 0;
			}
			data->num_deepest++;
			TAILQ_INSERT_HEAD(&data->nodes, it, items);
		}
	}
	else if(depth >= data->max_depth) {
		it = alg_node_it_new(node);
		data->num_deepest++;
		TAILQ_INSERT_HEAD(&data->nodes, it, items);
	}
	return 1;
}

LstStringSet * LstSuffixTree::AlgLongestSubstring(uint min_len, uint max_len, int lcs)
{
	LstStringSet * result = NULL;
	LstNodeIt * it;
	LstLcsData data;
	memzero(&data, sizeof(LstLcsData));
	data.tree = this;
	data.lcs = lcs;
	if(lcs)
		data.all_visitors = AlgSetVisitors();
	data.max_depth = (max_len > 0) ? (int)max_len : INT_MAX;
	TAILQ_INIT(&data.nodes);
	/* Now do a DSF finding the node with the largest string-
	* depth that has all strings as visitors.
	*/
	AlgDFS((LST_NodeVisitCB)alg_find_deepest, &data);
	D2("Deepest nodes found -- we have %u longest substring(s) at depth %u.\n", data.num_deepest, data.deepest);

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
			if((uint)LstNode::GetStringLength(it->node) >= min_len) {
				LstString * string = LstNode::GetString(it->node, (int)max_len);
				SETIFZ(result, new LstStringSet);
				result->insert(string);
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

