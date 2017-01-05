// TRANSP.CPP
// Copyright (c) A.Sobolev 1999, 2006, 2010, 2016
// Transpositions
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

struct Transpos {
	Transpos();
	~Transpos();
	int    Init(long sz, long stop, long pattern[]);
	int    CheckForTermination();
	void   Swap(long i, long j);
	int    Transposition(long order);
	void   Print();
	void   GetTransposition(long buf[]);

	long * List;
	long   Size;
	long * Pattern;
	long   Stop;
	long   Counter;
};

Transpos::Transpos()
{
	List = 0;
	Size = 0;
	List = 0;
	Pattern = 0;
	Stop = 0;
	Counter = 0;
}

Transpos::~Transpos()
{
	free(List);
	free(Pattern);
}

int Transpos::Init(long sz, long stop, long pattern[])
{
	int    i;
	Size = sz;
	List = (long*)malloc((size_t)(sizeof(long) * Size));
	for(i = 0; i < Size; i++)
		List[i] = i+1;
	Pattern = 0;
	if(pattern) {
		Pattern = (long*)malloc((size_t)(sizeof(long) * Size));
		for(i = 0; i < Size; i++)
			Pattern[i] = pattern[i];
	}
	Stop = stop;
	Counter = 1;
	return 1;
}

void Transpos::GetTransposition(long buf[])
{
	for(int i = 0; i < Size; i++)
		buf[i] = List[i];
}

int Transpos::CheckForTermination()
{
	if(List && Pattern) {
		for(int i = 0; i < Size; i++)
			if(List[i] != Pattern[i])
				return (Counter == Stop) ? 1 : 0;
		return 1;
	}
	return (Counter == Stop) ? 1 : 0;
}

void Transpos::Print()
{
	/*
	if(List) {
		SString temp_buf;
		for(long i = 0; i < Size; i++)
			temp_buf.Cat(List[i]).Space();
		temp_buf.CR();
		printf(temp_buf);
	}
	*/
}

void Transpos::Swap(long _i, long _j)
{
	size_t i = (size_t)_i, j = (size_t)_j;
	long   t = List[i-1];
   	List[i-1] = List[j-1];
	List[j-1] = t;
}

int Transpos::Transposition(long order)
{
	long   i;
	if(CheckForTermination())
		return 1;
	if(order <= Size) {
		if(Transposition(order+1))
			return 1;
		for(i = order; i > 1; i--) {
			Swap(i, i-1);
			Counter++;
			Print();
			if(Transposition(order+1))
				return 1;
		}
		for(i = 1; i < order; i++)
			Swap(i, i+1);
	}
	return 0;
}

long GetTranspositionNumber(long list[], long count)
{
	Transpos trans;
	trans.Init(count, 0, list);
	trans.Print();
	trans.Transposition(1);
	//printf("Total = %ld\n", trans.Counter);
	return trans.Counter;
}

int GetNthTransposition(long list[], long count, long n)
{
	Transpos trans;
	trans.Init(count, n, 0);
	trans.Print();
	int r = trans.Transposition(1);
	//printf("Total = %ld\n", trans.Counter);
	if(r > 0)
		trans.GetTransposition(list);
	return r;
}

#if 0 // {

#define LIST_SIZE 4

void main()
{
	long list[LIST_SIZE];
	if(GetNthTransposition(list, LIST_SIZE, 5L) > 0)
		GetTranspositionNumber(list, LIST_SIZE);
}

#endif // 0 }

