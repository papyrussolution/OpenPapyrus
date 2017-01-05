// STRSARY.CPP
// Copyright (c) Sobolev A. 1995, 2007
//
#include <malloc.h>
#include <iostream.h>
#include <slib.h>
#include <objstore.h>

#if 0

int SLAPI SArray::write(Rostream os)
{
	os.write((PCchar)&count, sizeof(count));
	os.write((PCchar)&pointer, sizeof(pointer));
	os.write((PCchar)&isize, sizeof(isize));
	os.write((PCchar)&hsize, sizeof(hsize));
	os.write((PCchar)&limit, sizeof(limit));
	os.write((PCchar)&delta, sizeof(delta));
	os.write((PCchar)&flags, sizeof(flags));
	os.write((PCchar)data, count*isize+hsize);
	return 1;
}

int SLAPI SArray::read(Ristream is)
{
	freeAll();
	is.read((Pchar)&count, sizeof(count));
	is.read((Pchar)&pointer, sizeof(pointer));
	is.read((Pchar)&isize, sizeof(isize));
	is.read((Pchar)&hsize, sizeof(hsize));
	is.read((Pchar)&limit, sizeof(limit));
	is.read((Pchar)&delta, sizeof(delta));
	is.read((Pchar)&flags, sizeof(flags));
	data = malloc(limit * isize + hsize);
	if(data)
		is.read((Pchar)data, count*isize+hsize);
	else {
		is.seekg(count*isize+hsize, ios::cur);
		SLibError = SLERR_NOMEM;
		return 0;
	}
	return 1;
}

#endif // 0

int SLAPI SArray::write(RObjStore os)
{
	os.Write(&count, offsetof(SArray, data) - offsetof(SArray, count));
	if(flags & aryEachItem)
		for(uint i = 0; i < count; i++)
			writeItem(os, at(i));
	else
		os.Write(data, count*isize+hsize);
	return 1;
}

int SLAPI SArray::read(RObjStore os)
{
	freeAll();
	os.Read(&count, offsetof(SArray, data) - offsetof(SArray, count));
	data = malloc(limit * isize + hsize);
	if(data)
		if(flags & aryEachItem) {
			os.Read(data, hsize);
			for(uint i = 0; i < count; i++)
				readItem(os, at(i));
		}
		else
			os.Read(data, count*isize+hsize);
	else {
		os.Seek(count*isize+hsize + os.Tell());
		SLibError = SLERR_NOMEM;
		return 0;
	}
	return 1;
}

int SLAPI SArray::writeItem(RObjStore, Pvoid)
{
	return 0;
}

int SLAPI SArray::readItem(RObjStore, Pvoid)
{
	return 0;
}
