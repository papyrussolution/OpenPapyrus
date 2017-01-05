//
// SMARTPTR.h
// Стародуб А. 2003
// Реализация Smart-указателя на интерфейс
//
#ifndef __SMARTPTR_H
#define __SMARTPTR_H

#include <assert.h>

template <class ClassType, const IID * pIID> class SmartPtr
{
public:
	SmartPtr() {P_I = NULL;}
	SmartPtr(ClassType *pI)
	{
		if((P_I = pI) != NULL)
			P_I->AddRef();
	}
	SmartPtr(IUnknown *pI)
	{
		P_I = NULL;
		if(pI != NULL)
			pI->QueryInterface(*pIID, (void**)&P_I);
	}
	~SmartPtr() 
	{
		Release();
	}
	void Release()
	{
		if(P_I != NULL) {
			ClassType *p_old = P_I;
			P_I = NULL;
			p_old->Release();
		}
	}

	operator ClassType*() {return P_I;}
	ClassType &  operator*() {assert(P_I != NULL); return *P_I;}
	ClassType ** operator&() {assert(P_I != NULL); return &P_I;}
	ClassType *  operator->() {assert(P_I != NULL); return P_I;}

	ClassType * operator=(ClassType * pI)
	{
		if(P_I != pI) {
			IUnknown *p_old = P_I;
			P_I = pI;
			if(P_I != NULL)
				P_I->AddRef();
			if(p_old != NULL)
				p_old->Release();
		}
		return P_I;
	}
	ClassType * operator=(IUnknown *pI)
	{
		IUnknown *p_old = P_I;
		P_I = NULL;
		if(pI != NULL) {
			HRESULT hr = pI->QueryInterface(*pIID, (void**)&P_I);
			assert(SUCCEEDED(hr) && (P_I != NULL));
		}
		if(p_old != NULL)
			p_old->Release();
		return P_I;
	}

	int operator!() {return (P_I == NULL) ? 1 : 0;}
	operator int() const {return (P_I != NULL) ? 1 : 0;}

	const IID &iid() {return *pIID;}

	HRESULT CreateInstance(const CLSID &clsid, IUnknown *pI, DWORD clsctx)
	{
		Release();
		return CoCreateInstance(clsid, pI, clsctx, *pIID, (void**)&P_I);
	}
private:
	ClassType *P_I;
};

#endif