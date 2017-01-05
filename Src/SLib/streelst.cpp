// STREELST.CPP
// Copyright (c) A.Starodub 2003, 2005, 2006, 2007, 2008, 2010, 2012, 2014
//
#include <slib.h>
#pragma hdrstop

#if 0 // {

IMPL_CMPFUNC(STreeListItem, i1, i2)
{
	STreeListItem * p_i1 = (STreeListItem*)i1;
	STreeListItem * p_i2 = (STreeListItem*)i2;
	if(p_i1->ParentID < p_i2->ParentID)
		return -1;
	else if(p_i1->ParentID > p_i2->ParentID)
		return 1;
	else
		return rus_stricmp(p_i1->Text, p_i2->Text);
}

SLAPI STreeList::STreeList(uint itemSize) : SArray(itemSize, 1, O_ARRAY)
{
}

int SLAPI STreeList::enumItems(uint * pI, long parent, void ** ppItem) const
{
	int    r = 0;
	if(*pI < count) {
		if((r = lsearch(&parent, pI, CMPF_LONG, 0)) > 0) {
			*ppItem = at(*pI);
			(*pI)++;
		}
	}
	return r;
}

int SLAPI STreeList::hasItem(long id, uint * pPos) const
{
	return lsearch(&id, pPos, CMPF_LONG, offsetof(STreeListItem, ID));
}

int FASTCALL STreeList::hasChild(long parent) const
{
	return lsearch(&parent, 0, CMPF_LONG, 0);
}

int SLAPI STreeList::getParent(long child, long * pParent) const
{
	long   parent = 0;
	uint   pos = 0;
	if(lsearch(&child, &pos, CMPF_LONG, sizeof(long)) > 0)
		parent = *(long *)at(pos);
	ASSIGN_PTR(pParent, parent);
	return (parent != 0);
}

int SLAPI STreeList::GetListByParent(long parentId, int recursive, LongArray & rList) const
{
	int    r = -1;
	uint   i;
	LongArray temp_list;
	void * p_item = 0;
	for(i = 0; enumItems(&i, parentId, &p_item);) {
		THROW(temp_list.add(((long *)p_item)[1])); // @v8.1.0 addUnique-->add
	}
	if(temp_list.getCount())
		r = 1;
	if(recursive) {
		LongArray temp_list2;
		for(i = 0; i < temp_list.getCount(); i++) {
			THROW(GetListByParent(temp_list.get(i), 1, temp_list2)); // @recursion
		}
		THROW(temp_list.add(&temp_list2)); // @v8.1.0 addUnique-->add
	}
	temp_list.sortAndUndup(); // @v8.1.0
	rList.addUnique(&temp_list);
	CATCH
		r = 0;
	ENDCATCH
	return r;
}

#if 0 // {
int SLAPI STreeList::getChildList(long parent, LongArray * pChildList) const
{
	int    r = -1;
	if(pChildList) {
		if(hasChild(parent)) {
			r = 1;
			void * p_item = 0;
			for(uint i = 0; enumItems(&i, parent, &p_item);) {
				long   child_id = ((long *)p_item)[1];
				THROW(pChildList->addUnique(child_id));
				if(/*parent != 0 &&*/ child_id != 0) // @v5.2.3 VADIM
					THROW(getChildList(child_id, pChildList)); // @recursion
			}
		}
	}
	CATCH
		r = 0;
	ENDCATCH
	return r;
}
#endif // } 0

#endif // } 0
