// UniqBlocks.cpp

#include <7z-internal.h>
#pragma hdrstop

uint CUniqBlocks::AddUniq(const Byte * data, size_t size)
{
	uint left = 0, right = Sorted.Size();
	while(left != right) {
		uint mid = (left + right) / 2;
		uint index = Sorted[mid];
		const CByteBuffer & buf = Bufs[index];
		size_t sizeMid = buf.Size();
		if(size < sizeMid)
			right = mid;
		else if(size > sizeMid)
			left = mid + 1;
		else {
			if(size == 0)
				return index;
			int cmp = memcmp(data, buf, size);
			if(cmp == 0)
				return index;
			if(cmp < 0)
				right = mid;
			else
				left = mid + 1;
		}
	}
	uint index = Bufs.Size();
	Sorted.Insert(left, index);
	Bufs.AddNew().CopyFrom(data, size);
	return index;
}

bool CUniqBlocks::IsOnlyEmpty() const { return (Bufs.Size() == 0 || Bufs.Size() == 1 && Bufs[0].Size() == 0); }

uint64 CUniqBlocks::GetTotalSizeInBytes() const
{
	uint64 size = 0;
	FOR_VECTOR(i, Bufs) {
		size += Bufs[i].Size();
	}
	return size;
}

void CUniqBlocks::GetReverseMap()
{
	uint num = Sorted.Size();
	BufIndexToSortedIndex.ClearAndSetSize(num);
	uint * p = &BufIndexToSortedIndex[0];
	const uint * sorted = &Sorted[0];
	for(uint i = 0; i < num; i++)
		p[sorted[i]] = i;
}

