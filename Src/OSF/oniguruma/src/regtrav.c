// regtrav.c -  Oniguruma (regular expression library)
// Copyright (c) 2002-2019  K.Kosako All rights reserved.
//
#include "regint.h"
#pragma hdrstop

#ifdef USE_CAPTURE_HISTORY

static int capture_tree_traverse(OnigCaptureTreeNode* node, int at, int (* callback_func)(int, int, int, int, int, void *), int level, void * arg)
{
	int r, i;
	if(node == (OnigCaptureTreeNode*)0)
		return 0;
	if((at & ONIG_TRAVERSE_CALLBACK_AT_FIRST) != 0) {
		r = (*callback_func)(node->group, node->beg, node->end, level, ONIG_TRAVERSE_CALLBACK_AT_FIRST, arg);
		if(r != 0) return r;
	}
	for(i = 0; i < node->num_childs; i++) {
		r = capture_tree_traverse(node->childs[i], at, callback_func, level + 1, arg);
		if(r != 0) return r;
	}
	if((at & ONIG_TRAVERSE_CALLBACK_AT_LAST) != 0) {
		r = (*callback_func)(node->group, node->beg, node->end, level, ONIG_TRAVERSE_CALLBACK_AT_LAST, arg);
		if(r != 0) return r;
	}
	return 0;
}

#endif /* USE_CAPTURE_HISTORY */

extern int onig_capture_tree_traverse(OnigRegion* region, int at, int (* callback_func)(int, int, int, int, int, void *), void * arg)
{
#ifdef USE_CAPTURE_HISTORY
	return capture_tree_traverse(region->history_root, at, callback_func, 0, arg);
#else
	return ONIG_NO_SUPPORT_CONFIG;
#endif
}
