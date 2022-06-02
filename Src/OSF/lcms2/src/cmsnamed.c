//
//  Little Color Management System
//  Copyright (c) 1998-2020 Marti Maria Saguer
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
#include "lcms2_internal.h"
#pragma hdrstop

// Multilocalized unicode objects. That is an attempt to encapsulate i18n.

// Allocates an empty multi localizad unicode object
cmsMLU* CMSEXPORT cmsMLUalloc(cmsContext ContextID, uint32 nItems)
{
	cmsMLU* mlu;
	// nItems should be positive if given
	if(nItems <= 0) 
		nItems = 2;
	// Create the container
	mlu = (cmsMLU*)_cmsMallocZero(ContextID, sizeof(cmsMLU));
	if(mlu == NULL) 
		return NULL;
	mlu->ContextID = ContextID;
	// Create entry array
	mlu->Entries = (_cmsMLUentry*)_cmsCalloc(ContextID, nItems, sizeof(_cmsMLUentry));
	if(mlu->Entries == NULL) {
		_cmsFree(ContextID, mlu);
		return NULL;
	}
	// Ok, keep indexes up to date
	mlu->AllocatedEntries    = nItems;
	mlu->UsedEntries = 0;
	return mlu;
}

// Grows a mempool table for a MLU. Each time this function is called, mempool size is multiplied times two.
static boolint GrowMLUpool(cmsMLU* mlu)
{
	uint32 size;
	void * NewPtr;
	// Sanity check
	if(mlu == NULL) 
		return FALSE;
	if(mlu->PoolSize == 0)
		size = 256;
	else
		size = mlu->PoolSize * 2;
	// Check for overflow
	if(size < mlu->PoolSize) 
		return FALSE;
	// Reallocate the pool
	NewPtr = _cmsRealloc(mlu->ContextID, mlu->MemPool, size);
	if(NewPtr == NULL) 
		return FALSE;
	mlu->MemPool  = NewPtr;
	mlu->PoolSize = size;
	return TRUE;
}

// Grows a entry table for a MLU. Each time this function is called, table size is multiplied times two.
static boolint GrowMLUtable(cmsMLU* mlu)
{
	uint32 AllocatedEntries;
	_cmsMLUentry * NewPtr;
	// Sanity check
	if(mlu == NULL) 
		return FALSE;
	AllocatedEntries = mlu->AllocatedEntries * 2;
	// Check for overflow
	if(AllocatedEntries / 2 != mlu->AllocatedEntries) 
		return FALSE;
	// Reallocate the memory
	NewPtr = (_cmsMLUentry*)_cmsRealloc(mlu->ContextID, mlu->Entries, AllocatedEntries*sizeof(_cmsMLUentry));
	if(NewPtr == NULL) return FALSE;
	mlu->Entries  = NewPtr;
	mlu->AllocatedEntries = AllocatedEntries;
	return TRUE;
}

// Search for a specific entry in the structure. Language and Country are used.
static int SearchMLUEntry(cmsMLU* mlu, uint16 LanguageCode, uint16 CountryCode)
{
	uint32 i;
	// Sanity check
	if(mlu == NULL) 
		return -1;
	// Iterate whole table
	for(i = 0; i < mlu->UsedEntries; i++) {
		if(mlu->Entries[i].Country  == CountryCode && mlu->Entries[i].Language == LanguageCode) 
			return (int)i;
	}
	// Not found
	return -1;
}

// Add a block of characters to the intended MLU. Language and country are specified.
// Only one entry for Language/country pair is allowed.
static boolint AddMLUBlock(cmsMLU* mlu, uint32 size, const wchar_t * Block, uint16 LanguageCode, uint16 CountryCode)
{
	uint32 Offset;
	uint8 * Ptr;
	// Sanity check
	if(mlu == NULL) 
		return FALSE;
	// Is there any room available?
	if(mlu->UsedEntries >= mlu->AllocatedEntries) {
		if(!GrowMLUtable(mlu)) 
			return FALSE;
	}
	// Only one ASCII string
	if(SearchMLUEntry(mlu, LanguageCode, CountryCode) >= 0) 
		return FALSE; // Only one  is allowed!
	// Check for size
	while((mlu->PoolSize - mlu->PoolUsed) < size) {
		if(!GrowMLUpool(mlu)) 
			return FALSE;
	}
	Offset = mlu->PoolUsed;
	Ptr = (uint8 *)mlu->MemPool;
	if(Ptr == NULL) 
		return FALSE;
	// Set the entry
	memmove(Ptr + Offset, Block, size);
	mlu->PoolUsed += size;
	mlu->Entries[mlu->UsedEntries].StrW     = Offset;
	mlu->Entries[mlu->UsedEntries].Len      = size;
	mlu->Entries[mlu->UsedEntries].Country  = CountryCode;
	mlu->Entries[mlu->UsedEntries].Language = LanguageCode;
	mlu->UsedEntries++;
	return TRUE;
}

// Convert from a 3-char code to a uint16. It is done in this way because some
// compilers don't properly align beginning of strings

static uint16 FASTCALL strTo16(const char str[3])
{
	const uint8 * ptr8 = (const uint8 *)str;
	uint16 n = (uint16)(((uint16)ptr8[0] << 8) | ptr8[1]);
	return n;
}

static void FASTCALL strFrom16(char str[3], uint16 n)
{
	str[0] = (char)(n >> 8);
	str[1] = (char)n;
	str[2] = (char)0;
}

// Add an ASCII entry. Do not add any \0 termination (ICC1v43_2010-12.pdf page 61)
boolint /*CMSEXPORT*/STDCALL cmsMLUsetASCII(cmsMLU* mlu, const char LanguageCode[3], const char CountryCode[3], const char * ASCIIString)
{
	uint32 i, len = (uint32)strlen(ASCIIString);
	wchar_t * WStr;
	boolint rc;
	uint16 Lang  = strTo16(LanguageCode);
	uint16 Cntry = strTo16(CountryCode);
	if(mlu == NULL) return FALSE;
	WStr = (wchar_t *)_cmsCalloc(mlu->ContextID, len,  sizeof(wchar_t));
	if(WStr == NULL) return FALSE;
	for(i = 0; i < len; i++)
		WStr[i] = (wchar_t)ASCIIString[i];
	rc = AddMLUBlock(mlu, len  * sizeof(wchar_t), WStr, Lang, Cntry);
	_cmsFree(mlu->ContextID, WStr);
	return rc;
}

// We don't need any wcs support library
/* @sobolev (replaced with sstrlen) static uint32 mywcslen(const wchar_t * s)
{
	const wchar_t * p = s;
	while(*p)
		p++;
	return (uint32)(p - s);
}*/

// Add a wide entry. Do not add any \0 terminator (ICC1v43_2010-12.pdf page 61)
boolint CMSEXPORT cmsMLUsetWide(cmsMLU* mlu, const char Language[3], const char Country[3], const wchar_t * WideString)
{
	uint16 Lang  = strTo16(Language);
	uint16 Cntry = strTo16(Country);
	uint32 len;
	if(mlu == NULL) return FALSE;
	if(WideString == NULL) return FALSE;
	len = (uint32)(sstrlen(WideString)) * sizeof(wchar_t);
	return AddMLUBlock(mlu, len, WideString, Lang, Cntry);
}

// Duplicating a MLU is as easy as copying all members
cmsMLU * /*CMSEXPORT*/FASTCALL cmsMLUdup(const cmsMLU* mlu)
{
	cmsMLU * NewMlu = NULL;
	// Duplicating a NULL obtains a NULL
	if(mlu == NULL) return NULL;
	NewMlu = cmsMLUalloc(mlu->ContextID, mlu->UsedEntries);
	if(NewMlu == NULL) return NULL;
	// Should never happen
	if(NewMlu->AllocatedEntries < mlu->UsedEntries)
		goto Error;
	// Sanitize...
	if(NewMlu->Entries == NULL || mlu->Entries == NULL) goto Error;
	memmove(NewMlu->Entries, mlu->Entries, mlu->UsedEntries * sizeof(_cmsMLUentry));
	NewMlu->UsedEntries = mlu->UsedEntries;
	// The MLU may be empty
	if(mlu->PoolUsed == 0) {
		NewMlu->MemPool = NULL;
	}
	else {
		// It is not empty
		NewMlu->MemPool = _cmsMalloc(mlu->ContextID, mlu->PoolUsed);
		if(NewMlu->MemPool == NULL) goto Error;
	}
	NewMlu->PoolSize = mlu->PoolUsed;
	if(NewMlu->MemPool == NULL || mlu->MemPool == NULL) 
		goto Error;
	memmove(NewMlu->MemPool, mlu->MemPool, mlu->PoolUsed);
	NewMlu->PoolUsed = mlu->PoolUsed;
	return NewMlu;
Error:
	cmsMLUfree(NewMlu);
	return NULL;
}

// Free any used memory
void /*CMSEXPORT*/FASTCALL cmsMLUfree(cmsMLU* mlu)
{
	if(mlu) {
		_cmsFree(mlu->ContextID, mlu->Entries);
		_cmsFree(mlu->ContextID, mlu->MemPool);
		_cmsFree(mlu->ContextID, mlu);
	}
}

// The algorithm first searches for an exact match of country and language, if not found it uses
// the Language. If none is found, first entry is used instead.
static const wchar_t * _cmsMLUgetWide(const cmsMLU* mlu, uint32 * len, uint16 LanguageCode, uint16 CountryCode, uint16* UsedLanguageCode, uint16* UsedCountryCode)
{
	uint32 i;
	int Best = -1;
	_cmsMLUentry* v;
	if(mlu == NULL) return NULL;
	if(mlu->AllocatedEntries <= 0) return NULL;
	for(i = 0; i < mlu->UsedEntries; i++) {
		v = mlu->Entries + i;
		if(v->Language == LanguageCode) {
			if(Best == -1) 
				Best = (int)i;
			if(v->Country == CountryCode) {
				ASSIGN_PTR(UsedLanguageCode, v->Language);
				ASSIGN_PTR(UsedCountryCode, v->Country);
				if(len != NULL) 
					*len = v->Len;
				return (wchar_t *)((uint8 *)mlu->MemPool + v->StrW); // Found exact match
			}
		}
	}
	// No string found. Return First one
	if(Best == -1)
		Best = 0;
	v = mlu->Entries + Best;
	ASSIGN_PTR(UsedLanguageCode, v->Language);
	ASSIGN_PTR(UsedCountryCode, v->Country);
	if(len != NULL) *len   = v->Len;
	return (wchar_t *)((uint8 *)mlu->MemPool + v->StrW);
}

// Obtain an ASCII representation of the wide string. Setting buffer to NULL returns the len
uint32 /*CMSEXPORT*/STDCALL cmsMLUgetASCII(const cmsMLU* mlu, const char LanguageCode[3], const char CountryCode[3], char * Buffer, uint32 BufferSize)
{
	const wchar_t * Wide;
	uint32 StrLen = 0;
	uint32 ASCIIlen, i;
	uint16 Lang  = strTo16(LanguageCode);
	uint16 Cntry = strTo16(CountryCode);
	// Sanitize
	if(mlu == NULL) return 0;
	// Get WideChar
	Wide = _cmsMLUgetWide(mlu, &StrLen, Lang, Cntry, NULL, NULL);
	if(Wide == NULL) return 0;
	ASCIIlen = StrLen / sizeof(wchar_t);
	// Maybe we want only to know the len?
	if(Buffer == NULL) return ASCIIlen + 1; // Note the zero at the end
	// No buffer size means no data
	if(BufferSize <= 0) return 0;
	// Some clipping may be required
	if(BufferSize < ASCIIlen + 1)
		ASCIIlen = BufferSize - 1;
	// Precess each character
	for(i = 0; i < ASCIIlen; i++) {
		if(Wide[i] == 0)
			Buffer[i] = 0;
		else
			Buffer[i] = (char)Wide[i];
	}
	// We put a termination "\0"
	Buffer[ASCIIlen] = 0;
	return ASCIIlen + 1;
}

// Obtain a wide representation of the MLU, on depending on current locale settings
uint32 CMSEXPORT cmsMLUgetWide(const cmsMLU* mlu, const char LanguageCode[3], const char CountryCode[3], wchar_t * Buffer, uint32 BufferSize)
{
	const wchar_t * Wide;
	uint32 StrLen = 0;
	uint16 Lang  = strTo16(LanguageCode);
	uint16 Cntry = strTo16(CountryCode);
	// Sanitize
	if(mlu == NULL) 
		return 0;
	Wide = _cmsMLUgetWide(mlu, &StrLen, Lang, Cntry, NULL, NULL);
	if(Wide == NULL) return 0;
	// Maybe we want only to know the len?
	if(Buffer == NULL) return StrLen + sizeof(wchar_t);
	// No buffer size means no data
	if(BufferSize <= 0) return 0;
	// Some clipping may be required
	if(BufferSize < StrLen + sizeof(wchar_t))
		StrLen = BufferSize - +sizeof(wchar_t);
	memmove(Buffer, Wide, StrLen);
	Buffer[StrLen / sizeof(wchar_t)] = 0;
	return StrLen + sizeof(wchar_t);
}

// Get also the language and country
CMSAPI boolint CMSEXPORT cmsMLUgetTranslation(const cmsMLU* mlu, const char LanguageCode[3], const char CountryCode[3], char ObtainedLanguage[3], char ObtainedCountry[3])
{
	const wchar_t * Wide;
	uint16 Lang  = strTo16(LanguageCode);
	uint16 Cntry = strTo16(CountryCode);
	uint16 ObtLang, ObtCode;
	// Sanitize
	if(mlu == NULL) return FALSE;
	Wide = _cmsMLUgetWide(mlu, NULL, Lang, Cntry, &ObtLang, &ObtCode);
	if(Wide == NULL) return FALSE;

	// Get used language and code
	strFrom16(ObtainedLanguage, ObtLang);
	strFrom16(ObtainedCountry, ObtCode);
	return TRUE;
}

// Get the number of translations in the MLU object
uint32 CMSEXPORT cmsMLUtranslationsCount(const cmsMLU* mlu)
{
	return mlu ? mlu->UsedEntries : 0;
}

// Get the language and country codes for a specific MLU index
boolint CMSEXPORT cmsMLUtranslationsCodes(const cmsMLU* mlu, uint32 idx, char LanguageCode[3], char CountryCode[3])
{
	_cmsMLUentry * entry;
	if(mlu == NULL) return FALSE;
	if(idx >= mlu->UsedEntries) return FALSE;
	entry = &mlu->Entries[idx];
	strFrom16(LanguageCode, entry->Language);
	strFrom16(CountryCode, entry->Country);
	return TRUE;
}

// Named color lists --------------------------------------------------------------------------------------------

// Grow the list to keep at least NumElements
static boolint FASTCALL GrowNamedColorList(cmsNAMEDCOLORLIST* v)
{
	uint32 size;
	_cmsNAMEDCOLOR * NewPtr;
	if(v == NULL) return FALSE;
	if(v->Allocated == 0)
		size = 64; // Initial guess
	else
		size = v->Allocated * 2;
	// Keep a maximum color lists can grow, 100K entries seems reasonable
	if(size > 1024 * 100) {
		_cmsFree(v->ContextID, (void *)v->List);
		v->List = NULL;
		return FALSE;
	}
	NewPtr = (_cmsNAMEDCOLOR*)_cmsRealloc(v->ContextID, v->List, size * sizeof(_cmsNAMEDCOLOR));
	if(NewPtr == NULL)
		return FALSE;
	v->List      = NewPtr;
	v->Allocated = size;
	return TRUE;
}

// Allocate a list for n elements
cmsNAMEDCOLORLIST* CMSEXPORT cmsAllocNamedColorList(cmsContext ContextID, uint32 n, uint32 ColorantCount, const char * Prefix, const char * Suffix)
{
	cmsNAMEDCOLORLIST* v = (cmsNAMEDCOLORLIST*)_cmsMallocZero(ContextID, sizeof(cmsNAMEDCOLORLIST));
	if(v == NULL) return NULL;
	v->List      = NULL;
	v->nColors   = 0;
	v->ContextID  = ContextID;
	while(v->Allocated < n) {
		if(!GrowNamedColorList(v)) {
			cmsFreeNamedColorList(v);
			return NULL;
		}
	}
	strncpy(v->Prefix, Prefix, sizeof(v->Prefix)-1);
	strncpy(v->Suffix, Suffix, sizeof(v->Suffix)-1);
	v->Prefix[32] = v->Suffix[32] = 0;
	v->ColorantCount = ColorantCount;
	return v;
}

// Free a list
void /*CMSEXPORT*/FASTCALL cmsFreeNamedColorList(cmsNAMEDCOLORLIST* v)
{
	if(v) {
		_cmsFree(v->ContextID, v->List);
		_cmsFree(v->ContextID, v);
	}
}

cmsNAMEDCOLORLIST* CMSEXPORT cmsDupNamedColorList(const cmsNAMEDCOLORLIST* v)
{
	cmsNAMEDCOLORLIST* NewNC;
	if(v == NULL) return NULL;
	NewNC = cmsAllocNamedColorList(v->ContextID, v->nColors, v->ColorantCount, v->Prefix, v->Suffix);
	if(NewNC == NULL) return NULL;
	// For really large tables we need this
	while(NewNC->Allocated < v->Allocated) {
		if(!GrowNamedColorList(NewNC)) {
			cmsFreeNamedColorList(NewNC);
			return NULL;
		}
	}
	memmove(NewNC->Prefix, v->Prefix, sizeof(v->Prefix));
	memmove(NewNC->Suffix, v->Suffix, sizeof(v->Suffix));
	NewNC->ColorantCount = v->ColorantCount;
	memmove(NewNC->List, v->List, v->nColors * sizeof(_cmsNAMEDCOLOR));
	NewNC->nColors = v->nColors;
	return NewNC;
}

// Append a color to a list. List pointer may change if reallocated
boolint CMSEXPORT cmsAppendNamedColor(cmsNAMEDCOLORLIST* NamedColorList, const char * Name, uint16 PCS[3], uint16 Colorant[cmsMAXCHANNELS])
{
	uint32 i;
	if(NamedColorList == NULL) return FALSE;
	if(NamedColorList->nColors + 1 > NamedColorList->Allocated) {
		if(!GrowNamedColorList(NamedColorList)) return FALSE;
	}
	for(i = 0; i < NamedColorList->ColorantCount; i++)
		NamedColorList->List[NamedColorList->nColors].DeviceColorant[i] = Colorant == NULL ? (uint16)0 : Colorant[i];
	for(i = 0; i < 3; i++)
		NamedColorList->List[NamedColorList->nColors].PCS[i] = PCS == NULL ? (uint16)0 : PCS[i];
	if(Name != NULL) {
		strncpy(NamedColorList->List[NamedColorList->nColors].Name, Name, cmsMAX_PATH-1);
		NamedColorList->List[NamedColorList->nColors].Name[cmsMAX_PATH-1] = 0;
	}
	else
		NamedColorList->List[NamedColorList->nColors].Name[0] = 0;
	NamedColorList->nColors++;
	return TRUE;
}

// Returns number of elements
uint32 CMSEXPORT cmsNamedColorCount(const cmsNAMEDCOLORLIST* NamedColorList)
{
	if(NamedColorList == NULL) return 0;
	return NamedColorList->nColors;
}

// Info aboout a given color
boolint CMSEXPORT cmsNamedColorInfo(const cmsNAMEDCOLORLIST* NamedColorList, uint32 nColor, char * Name, char * Prefix, char * Suffix, uint16* PCS, uint16* Colorant)
{
	if(NamedColorList == NULL) return FALSE;
	if(nColor >= cmsNamedColorCount(NamedColorList)) return FALSE;
	// strcpy instead of strncpy because many apps are using small buffers
	if(Name) strcpy(Name, NamedColorList->List[nColor].Name);
	if(Prefix) strcpy(Prefix, NamedColorList->Prefix);
	if(Suffix) strcpy(Suffix, NamedColorList->Suffix);
	if(PCS)
		memmove(PCS, NamedColorList->List[nColor].PCS, 3*sizeof(uint16));
	if(Colorant)
		memmove(Colorant, NamedColorList->List[nColor].DeviceColorant, sizeof(uint16) * NamedColorList->ColorantCount);
	return TRUE;
}

// Search for a given color name (no prefix or suffix)
int32 CMSEXPORT cmsNamedColorIndex(const cmsNAMEDCOLORLIST* NamedColorList, const char * Name)
{
	uint32 i;
	uint32 n;
	if(NamedColorList == NULL) 
		return -1;
	n = cmsNamedColorCount(NamedColorList);
	for(i = 0; i < n; i++) {
		if(cmsstrcasecmp(Name,  NamedColorList->List[i].Name) == 0)
			return (int32)i;
	}
	return -1;
}

// MPE support
// -----------------------------------------------------------------------------------------------------------------

static void FreeNamedColorList(cmsStage * mpe)
{
	cmsNAMEDCOLORLIST* List = (cmsNAMEDCOLORLIST*)mpe->Data;
	cmsFreeNamedColorList(List);
}

static void * DupNamedColorList(cmsStage * mpe)
{
	cmsNAMEDCOLORLIST* List = (cmsNAMEDCOLORLIST*)mpe->Data;
	return cmsDupNamedColorList(List);
}

static void EvalNamedColorPCS(const float In[], float Out[], const cmsStage * mpe)
{
	cmsNAMEDCOLORLIST* NamedColorList = (cmsNAMEDCOLORLIST*)mpe->Data;
	uint16 index = (uint16)_cmsQuickSaturateWord(In[0] * 65535.0);
	if(index >= NamedColorList->nColors) {
		cmsSignalError(NamedColorList->ContextID, cmsERROR_RANGE, "Color %d out of range", index);
		Out[0] = Out[1] = Out[2] = 0.0f;
	}
	else {
		// Named color always uses Lab
		Out[0] = (float)(NamedColorList->List[index].PCS[0] / 65535.0);
		Out[1] = (float)(NamedColorList->List[index].PCS[1] / 65535.0);
		Out[2] = (float)(NamedColorList->List[index].PCS[2] / 65535.0);
	}
}

static void EvalNamedColor(const float In[], float Out[], const cmsStage * mpe)
{
	cmsNAMEDCOLORLIST* NamedColorList = (cmsNAMEDCOLORLIST*)mpe->Data;
	uint16 index = (uint16)_cmsQuickSaturateWord(In[0] * 65535.0);
	uint32 j;
	if(index >= NamedColorList->nColors) {
		cmsSignalError(NamedColorList->ContextID, cmsERROR_RANGE, "Color %d out of range", index);
		for(j = 0; j < NamedColorList->ColorantCount; j++)
			Out[j] = 0.0f;
	}
	else {
		for(j = 0; j < NamedColorList->ColorantCount; j++)
			Out[j] = (float)(NamedColorList->List[index].DeviceColorant[j] / 65535.0);
	}
}

// Named color lookup element
cmsStage * CMSEXPORT _cmsStageAllocNamedColor(cmsNAMEDCOLORLIST* NamedColorList, boolint UsePCS)
{
	return _cmsStageAllocPlaceholder(NamedColorList->ContextID,
		   cmsSigNamedColorElemType,
		   1, UsePCS ? 3 : NamedColorList->ColorantCount,
		   UsePCS ? EvalNamedColorPCS : EvalNamedColor,
		   DupNamedColorList,
		   FreeNamedColorList,
		   cmsDupNamedColorList(NamedColorList));
}

// Retrieve the named color list from a transform. Should be first element in the LUT
cmsNAMEDCOLORLIST* CMSEXPORT cmsGetNamedColorList(cmsHTRANSFORM xform)
{
	_cmsTRANSFORM* v = (_cmsTRANSFORM*)xform;
	cmsStage * mpe  = v->Lut->Elements;
	if(mpe->Type != cmsSigNamedColorElemType) return NULL;
	return (cmsNAMEDCOLORLIST*)mpe->Data;
}

// Profile sequence description routines
// -------------------------------------------------------------------------------------

cmsSEQ* CMSEXPORT cmsAllocProfileSequenceDescription(cmsContext ContextID, uint32 n)
{
	cmsSEQ* Seq;
	uint32 i;
	if(!n) return NULL;
	// In a absolutely arbitrary way, I hereby decide to allow a maxim of 255 profiles linked
	// in a devicelink. It makes not sense anyway and may be used for exploits, so let's close the door!
	if(n > 255) return NULL;
	Seq = (cmsSEQ*)_cmsMallocZero(ContextID, sizeof(cmsSEQ));
	if(Seq == NULL) return NULL;
	Seq->ContextID = ContextID;
	Seq->seq      = (cmsPSEQDESC*)_cmsCalloc(ContextID, n, sizeof(cmsPSEQDESC));
	Seq->n        = n;
	if(Seq->seq == NULL) {
		_cmsFree(ContextID, Seq);
		return NULL;
	}
	for(i = 0; i < n; i++) {
		Seq->seq[i].Manufacturer = NULL;
		Seq->seq[i].Model        = NULL;
		Seq->seq[i].Description  = NULL;
	}
	return Seq;
}

void CMSEXPORT cmsFreeProfileSequenceDescription(cmsSEQ * pseq)
{
	if(pseq) {
		for(uint32 i = 0; i < pseq->n; i++) {
			cmsMLUfree(pseq->seq[i].Manufacturer);
			cmsMLUfree(pseq->seq[i].Model);
			cmsMLUfree(pseq->seq[i].Description);
		}
		_cmsFree(pseq->ContextID, pseq->seq);
		_cmsFree(pseq->ContextID, pseq);
	}
}

cmsSEQ* CMSEXPORT cmsDupProfileSequenceDescription(const cmsSEQ* pseq)
{
	cmsSEQ * NewSeq;
	uint32 i;
	if(pseq == NULL)
		return NULL;
	NewSeq = (cmsSEQ*)_cmsMalloc(pseq->ContextID, sizeof(cmsSEQ));
	if(NewSeq == NULL) 
		return NULL;
	NewSeq->seq      = (cmsPSEQDESC*)_cmsCalloc(pseq->ContextID, pseq->n, sizeof(cmsPSEQDESC));
	if(NewSeq->seq == NULL) 
		goto Error;
	NewSeq->ContextID = pseq->ContextID;
	NewSeq->n        = pseq->n;
	for(i = 0; i < pseq->n; i++) {
		memmove(&NewSeq->seq[i].attributes, &pseq->seq[i].attributes, sizeof(uint64));
		NewSeq->seq[i].deviceMfg   = pseq->seq[i].deviceMfg;
		NewSeq->seq[i].deviceModel = pseq->seq[i].deviceModel;
		memmove(&NewSeq->seq[i].ProfileID, &pseq->seq[i].ProfileID, sizeof(cmsProfileID));
		NewSeq->seq[i].technology  = pseq->seq[i].technology;
		NewSeq->seq[i].Manufacturer = cmsMLUdup(pseq->seq[i].Manufacturer);
		NewSeq->seq[i].Model        = cmsMLUdup(pseq->seq[i].Model);
		NewSeq->seq[i].Description  = cmsMLUdup(pseq->seq[i].Description);
	}
	return NewSeq;
Error:
	cmsFreeProfileSequenceDescription(NewSeq);
	return NULL;
}

// Dictionaries --------------------------------------------------------------------------------------------------------

// Dictionaries are just very simple linked lists

typedef struct _cmsDICT_struct {
	cmsDICTentry* head;
	cmsContext ContextID;
} _cmsDICT;

// Allocate an empty dictionary
cmsHANDLE CMSEXPORT cmsDictAlloc(cmsContext ContextID)
{
	_cmsDICT * dict = (_cmsDICT*)_cmsMallocZero(ContextID, sizeof(_cmsDICT));
	if(dict) 
		dict->ContextID = ContextID;
	return (cmsHANDLE)dict;
}

// Dispose resources
void CMSEXPORT cmsDictFree(cmsHANDLE hDict)
{
	_cmsDICT * dict = (_cmsDICT*)hDict;
	cmsDICTentry * entry, * next;
	assert(dict != NULL);
	// Walk the list freeing all nodes
	entry = dict->head;
	while(entry != NULL) {
		cmsMLUfree(entry->DisplayName);
		cmsMLUfree(entry->DisplayValue);
		_cmsFree(dict->ContextID, entry->Name);
		_cmsFree(dict->ContextID, entry->Value);
		// Don't fall in the habitual trap...
		next = entry->Next;
		_cmsFree(dict->ContextID, entry);
		entry = next;
	}
	_cmsFree(dict->ContextID, dict);
}

// Duplicate a wide char string
static wchar_t * DupWcs(cmsContext ContextID, const wchar_t * ptr)
{
	return ptr ? (wchar_t *)_cmsDupMem(ContextID, ptr, (sstrlen(ptr) + 1) * sizeof(wchar_t)) : NULL;
}

// Add a new entry to the linked list
boolint CMSEXPORT cmsDictAddEntry(cmsHANDLE hDict, const wchar_t * Name, const wchar_t * Value, const cmsMLU * DisplayName, const cmsMLU * DisplayValue)
{
	_cmsDICT* dict = (_cmsDICT*)hDict;
	cmsDICTentry * entry;
	assert(dict != NULL);
	assert(Name != NULL);
	entry = (cmsDICTentry*)_cmsMallocZero(dict->ContextID, sizeof(cmsDICTentry));
	if(!entry) return FALSE;
	entry->DisplayName  = cmsMLUdup(DisplayName);
	entry->DisplayValue = cmsMLUdup(DisplayValue);
	entry->Name = DupWcs(dict->ContextID, Name);
	entry->Value        = DupWcs(dict->ContextID, Value);
	entry->Next = dict->head;
	dict->head = entry;
	return TRUE;
}

// Duplicates an existing dictionary
cmsHANDLE CMSEXPORT cmsDictDup(cmsHANDLE hDict)
{
	_cmsDICT* old_dict = (_cmsDICT*)hDict;
	cmsHANDLE hNew;
	cmsDICTentry * entry;
	assert(old_dict != NULL);
	hNew  = cmsDictAlloc(old_dict->ContextID);
	if(hNew == NULL) 
		return NULL;
	// Walk the list freeing all nodes
	entry = old_dict->head;
	while(entry != NULL) {
		if(!cmsDictAddEntry(hNew, entry->Name, entry->Value, entry->DisplayName, entry->DisplayValue)) {
			cmsDictFree(hNew);
			return NULL;
		}
		entry = entry->Next;
	}
	return hNew;
}

// Get a pointer to the linked list
const cmsDICTentry* CMSEXPORT cmsDictGetEntryList(cmsHANDLE hDict)
{
	_cmsDICT* dict = (_cmsDICT*)hDict;
	return dict ? dict->head : NULL;
}

// Helper For external languages
const cmsDICTentry* CMSEXPORT cmsDictNextEntry(const cmsDICTentry* e)
{
	return e ? e->Next : NULL;
}
