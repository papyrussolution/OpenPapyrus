// MyMap.h

#ifndef __COMMON_MYMAP_H
#define __COMMON_MYMAP_H

class CMap32 {
	struct CNode {
		uint32 Key;
		uint32 Keys[2];
		uint32 Values[2];
		uint16 Len;
		Byte IsLeaf[2];
	};
	CRecordVector<CNode> Nodes;
public:
	void Clear() 
	{
		Nodes.Clear();
	}
	bool Find(uint32 key, uint32 &valueRes) const throw();
	bool Set(uint32 key, uint32 value); // returns true, if there is such key already
};

#endif
