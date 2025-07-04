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
//
// Tag Serialization  -----------------------------------------------------------------------------
// This file implements every single tag and tag type as described in the ICC spec. Some types
// have been deprecated, like ncl and Data. There is no implementation for those types as there
// are no profiles holding them. The programmer can also extend this list by defining his own types
// by using the appropriate plug-in. There are three types of plug ins regarding that. First type
// allows to define new tags using any existing type. Next plug-in type allows to define new types
// and the third one is very specific: allows to extend the number of elements in the multiprocessing
// elements special type.
//
// Some broken types
#define cmsCorbisBrokenXYZtype    ((cmsTagTypeSignature)0x17A505B8)
#define cmsMonacoBrokenCurveType  ((cmsTagTypeSignature)0x9478ee00)

// This is the linked list that keeps track of the defined types
typedef struct _cmsTagTypeLinkedList_st {
	cmsTagTypeHandler Handler;
	struct _cmsTagTypeLinkedList_st* Next;
} _cmsTagTypeLinkedList;

// Some macros to define callbacks.
#define READ_FN(x)  Type_ ## x ## _Read
#define WRITE_FN(x) Type_ ## x ## _Write
#define FREE_FN(x)  Type_ ## x ## _Free
#define DUP_FN(x)   Type_ ## x ## _Dup

// Helper macro to define a handler. Callbacks do have a fixed naming convention.
#define TYPE_HANDLER(t, x) { (t), READ_FN(x), WRITE_FN(x), DUP_FN(x), FREE_FN(x), NULL, 0 }

// Helper macro to define a MPE handler. Callbacks do have a fixed naming convention
#define TYPE_MPE_HANDLER(t, x) { (t), READ_FN(x), WRITE_FN(x), GenericMPEdup, GenericMPEfree, NULL, 0 }

// Infinites
#define MINUS_INF   (-1E22F)
#define PLUS_INF    (+1E22F)

// Register a new type handler. This routine is shared between normal types and MPE. LinkedList points to the optional
// list head
static boolint RegisterTypesPlugin(cmsContext id, cmsPluginBase* Data, _cmsMemoryClient pos)
{
	cmsPluginTagType* Plugin = (cmsPluginTagType*)Data;
	_cmsTagTypePluginChunkType* ctx = (_cmsTagTypePluginChunkType*)_cmsContextGetClientChunk(id, pos);
	_cmsTagTypeLinkedList * pt;
	// Calling the function with NULL as plug-in would unregister the plug in.
	if(!Data) {
		// There is no need to set free the memory, as pool is destroyed as a whole.
		ctx->TagTypes = NULL;
		return TRUE;
	}
	// Registering happens in plug-in memory pool.
	pt = (_cmsTagTypeLinkedList*)_cmsPluginMalloc(id, sizeof(_cmsTagTypeLinkedList));
	if(pt == NULL) 
		return FALSE;
	pt->Handler   = Plugin->Handler;
	pt->Next      = ctx->TagTypes;
	ctx->TagTypes = pt;
	return TRUE;
}

// Return handler for a given type or NULL if not found. Shared between normal types and MPE. It first tries the
// additons
// made by plug-ins and then the built-in defaults.
static cmsTagTypeHandler* GetHandler(cmsTagTypeSignature sig, _cmsTagTypeLinkedList* PluginLinkedList, _cmsTagTypeLinkedList* DefaultLinkedList)
{
	_cmsTagTypeLinkedList* pt;
	for(pt = PluginLinkedList; pt; pt = pt->Next) {
		if(sig == pt->Handler.Signature) 
			return &pt->Handler;
	}
	for(pt = DefaultLinkedList; pt; pt = pt->Next) {
		if(sig == pt->Handler.Signature) 
			return &pt->Handler;
	}
	return NULL;
}

// Auxiliary to convert UTF-32 to UTF-16 in some cases
static boolint _cmsWriteWCharArray(cmsIOHANDLER* io, uint32 n, const wchar_t * Array)
{
	assert(io);
	assert(!(Array == NULL && n > 0));
	for(uint32 i = 0; i < n; i++) {
		if(!_cmsWriteUInt16Number(io, (uint16)Array[i])) 
			return FALSE;
	}
	return TRUE;
}

// Auxiliary to read an array of wchar_t
static boolint _cmsReadWCharArray(cmsIOHANDLER* io, uint32 n, wchar_t * Array)
{
	assert(io);
	for(uint32 i = 0; i < n; i++) {
		if(Array) {
			uint16 tmp;
			if(!_cmsReadUInt16Number(io, &tmp)) 
				return FALSE;
			Array[i] = (wchar_t)tmp;
		}
		else {
			if(!_cmsReadUInt16Number(io, NULL)) 
				return FALSE;
		}
	}
	return TRUE;
}

// To deal with position tables
typedef boolint (* PositionTableEntryFn)(struct _cms_typehandler_struct* self,
    cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag);

// Helper function to deal with position tables as described in ICC spec 4.3
// A table of n elements is read, where first comes n records containing offsets and sizes and
// then a block containing the data itself. This allows to reuse same data in more than one entry
static boolint ReadPositionTable(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 Count, 
	uint32 BaseOffset, void * Cargo, PositionTableEntryFn ElementFn)
{
	uint32 i;
	uint32 * ElementOffsets = NULL, * ElementSizes = NULL;
	uint32 currentPosition = io->Tell(io);
	// Verify there is enough space left to read at least two uint32 items for Count items.
	if(((io->ReportedSize - currentPosition) / (2 * sizeof(uint32))) < Count)
		return FALSE;
	// Let's take the offsets to each element
	ElementOffsets = (uint32 *)_cmsCalloc(io->ContextID, Count, sizeof(uint32));
	if(ElementOffsets == NULL) 
		goto Error;
	ElementSizes = (uint32 *)_cmsCalloc(io->ContextID, Count, sizeof(uint32));
	if(ElementSizes == NULL) 
		goto Error;
	for(i = 0; i < Count; i++) {
		if(!_cmsReadUInt32Number(io, &ElementOffsets[i])) goto Error;
		if(!_cmsReadUInt32Number(io, &ElementSizes[i])) goto Error;
		ElementOffsets[i] += BaseOffset;
	}
	// Seek to each element and read it
	for(i = 0; i < Count; i++) {
		if(!io->Seek(io, ElementOffsets[i])) goto Error;
		// This is the reader callback
		if(!ElementFn(self, io, Cargo, i, ElementSizes[i])) goto Error;
	}
	// Success
	_cmsFree(io->ContextID, ElementOffsets);
	_cmsFree(io->ContextID, ElementSizes);
	return TRUE;
Error:
	_cmsFree(io->ContextID, ElementOffsets);
	_cmsFree(io->ContextID, ElementSizes);
	return FALSE;
}

// Same as anterior, but for write position tables
static boolint WritePositionTable(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 SizeOfTag,
    uint32 Count, uint32 BaseOffset, void * Cargo, PositionTableEntryFn ElementFn)
{
	uint32 i;
	uint32 DirectoryPos, CurrentPos, Before;
	uint32 * ElementOffsets = NULL, * ElementSizes = NULL;
	// Create table
	ElementOffsets = (uint32 *)_cmsCalloc(io->ContextID, Count, sizeof(uint32));
	if(ElementOffsets == NULL) goto Error;
	ElementSizes = (uint32 *)_cmsCalloc(io->ContextID, Count, sizeof(uint32));
	if(ElementSizes == NULL) goto Error;
	// Keep starting position of curve offsets
	DirectoryPos = io->Tell(io);
	// Write a fake directory to be filled latter on
	for(i = 0; i < Count; i++) {
		if(!_cmsWriteUInt32Number(io, 0)) goto Error; // Offset
		if(!_cmsWriteUInt32Number(io, 0)) goto Error; // size
	}

	// Write each element. Keep track of the size as well.
	for(i = 0; i < Count; i++) {
		Before = io->Tell(io);
		ElementOffsets[i] = Before - BaseOffset;

		// Callback to write...
		if(!ElementFn(self, io, Cargo, i, SizeOfTag)) goto Error;

		// Now the size
		ElementSizes[i] = io->Tell(io) - Before;
	}

	// Write the directory
	CurrentPos = io->Tell(io);
	if(!io->Seek(io, DirectoryPos)) goto Error;

	for(i = 0; i <  Count; i++) {
		if(!_cmsWriteUInt32Number(io, ElementOffsets[i])) goto Error;
		if(!_cmsWriteUInt32Number(io, ElementSizes[i])) goto Error;
	}
	if(!io->Seek(io, CurrentPos)) goto Error;
	_cmsFree(io->ContextID, ElementOffsets);
	_cmsFree(io->ContextID, ElementSizes);
	return TRUE;
Error:
	_cmsFree(io->ContextID, ElementOffsets);
	_cmsFree(io->ContextID, ElementSizes);
	return FALSE;
}
//
// Type XYZ. Only one value is allowed
//
//The XYZType contains an array of three encoded values for the XYZ tristimulus
//values. Tristimulus values must be non-negative. The signed encoding allows for
//implementation optimizations by minimizing the number of fixed formats.
//
static void * Type_XYZ_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsCIEXYZ* xyz;
	*nItems = 0;
	xyz = (cmsCIEXYZ*)_cmsMallocZero(self->ContextID, sizeof(cmsCIEXYZ));
	if(xyz == NULL) return NULL;
	if(!_cmsReadXYZNumber(io, xyz)) {
		_cmsFree(self->ContextID, xyz);
		return NULL;
	}
	*nItems = 1;
	return (void *)xyz;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_XYZ_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	return _cmsWriteXYZNumber(io, (cmsCIEXYZ*)Ptr);
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_XYZ_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsCIEXYZ));
	CXX_UNUSED(n);
}

static void Type_XYZ_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}

static cmsTagTypeSignature DecideXYZtype(double ICCVersion, const void * Data)
{
	return cmsSigXYZType;
	CXX_UNUSED(ICCVersion);
	CXX_UNUSED(Data);
}
//
// Type chromaticity. Only one value is allowed
//
// The chromaticity tag type provides basic chromaticity data and type of
// phosphors or colorants of a monitor to applications and utilities.
//
static void * Type_Chromaticity_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsCIExyYTRIPLE* chrm;
	uint16 nChans, Table;
	*nItems = 0;
	chrm =  (cmsCIExyYTRIPLE*)_cmsMallocZero(self->ContextID, sizeof(cmsCIExyYTRIPLE));
	if(chrm == NULL) return NULL;

	if(!_cmsReadUInt16Number(io, &nChans)) goto Error;

	// Let's recover from a bug introduced in early versions of lcms1
	if(nChans == 0 && SizeOfTag == 32) {
		if(!_cmsReadUInt16Number(io, NULL)) goto Error;
		if(!_cmsReadUInt16Number(io, &nChans)) goto Error;
	}

	if(nChans != 3) goto Error;

	if(!_cmsReadUInt16Number(io, &Table)) goto Error;

	if(!_cmsRead15Fixed16Number(io, &chrm->Red.x)) goto Error;
	if(!_cmsRead15Fixed16Number(io, &chrm->Red.y)) goto Error;

	chrm->Red.Y = 1.0;

	if(!_cmsRead15Fixed16Number(io, &chrm->Green.x)) goto Error;
	if(!_cmsRead15Fixed16Number(io, &chrm->Green.y)) goto Error;

	chrm->Green.Y = 1.0;

	if(!_cmsRead15Fixed16Number(io, &chrm->Blue.x)) goto Error;
	if(!_cmsRead15Fixed16Number(io, &chrm->Blue.y)) goto Error;

	chrm->Blue.Y = 1.0;

	*nItems = 1;
	return (void *)chrm;

Error:
	_cmsFree(self->ContextID, (void *)chrm);
	return NULL;

	CXX_UNUSED(SizeOfTag);
}

static boolint SaveOneChromaticity(double x, double y, cmsIOHANDLER* io)
{
	if(!_cmsWriteUInt32Number(io, (uint32)_cmsDoubleTo15Fixed16(x))) return FALSE;
	if(!_cmsWriteUInt32Number(io, (uint32)_cmsDoubleTo15Fixed16(y))) return FALSE;
	return TRUE;
}

static boolint Type_Chromaticity_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsCIExyYTRIPLE* chrm = (cmsCIExyYTRIPLE*)Ptr;
	if(!_cmsWriteUInt16Number(io, 3)) return FALSE;     // nChannels
	if(!_cmsWriteUInt16Number(io, 0)) return FALSE;     // Table

	if(!SaveOneChromaticity(chrm->Red.x,   chrm->Red.y, io)) return FALSE;
	if(!SaveOneChromaticity(chrm->Green.x, chrm->Green.y, io)) return FALSE;
	if(!SaveOneChromaticity(chrm->Blue.x,  chrm->Blue.y, io)) return FALSE;
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Chromaticity_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsCIExyYTRIPLE));
	CXX_UNUSED(n);
}

static void Type_Chromaticity_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigColorantOrderType
//
// This is an optional tag which specifies the laydown order in which colorants will
// be printed on an n-colorant device. The laydown order may be the same as the
// channel generation order listed in the colorantTableTag or the channel order of a
// colour space such as CMYK, in which case this tag is not needed. When this is not
// the case (for example, ink-towers sometimes use the order KCMY), this tag may be
// used to specify the laydown order of the colorants.
//
static void * Type_ColorantOrderType_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint8 * ColorantOrder;
	uint32 Count;
	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	if(Count > cmsMAXCHANNELS) return NULL;
	ColorantOrder = (uint8 *)_cmsCalloc(self->ContextID, cmsMAXCHANNELS, sizeof(uint8));
	if(ColorantOrder == NULL) return NULL;

	// We use FF as end marker
	memset(ColorantOrder, 0xFF, cmsMAXCHANNELS * sizeof(uint8));
	if(io->Read(io, ColorantOrder, sizeof(uint8), Count) != Count) {
		_cmsFree(self->ContextID, (void *)ColorantOrder);
		return NULL;
	}
	*nItems = 1;
	return (void *)ColorantOrder;

	CXX_UNUSED(SizeOfTag);
}

static boolint Type_ColorantOrderType_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint8 *  ColorantOrder = (uint8 *)Ptr;
	uint32 i, sz, Count;
	// Get the length
	for(Count = i = 0; i < cmsMAXCHANNELS; i++) {
		if(ColorantOrder[i] != 0xFF) Count++;
	}
	if(!_cmsWriteUInt32Number(io, Count)) return FALSE;
	sz = Count * sizeof(uint8);
	if(!io->Write(io, sz, ColorantOrder)) return FALSE;
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_ColorantOrderType_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, cmsMAXCHANNELS * sizeof(uint8));
	CXX_UNUSED(n);
}

static void Type_ColorantOrderType_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigS15Fixed16ArrayType
//
// This type represents an array of generic 4-byte/32-bit fixed point quantity.
// The number of values is determined from the size of the tag.
//
static void * Type_S15Fixed16_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	double *  array_double;
	uint32 i, n;
	*nItems = 0;
	n = SizeOfTag / sizeof(uint32);
	array_double = (double *)_cmsCalloc(self->ContextID, n, sizeof(double));
	if(array_double == NULL) return NULL;
	for(i = 0; i < n; i++) {
		if(!_cmsRead15Fixed16Number(io, &array_double[i])) {
			_cmsFree(self->ContextID, array_double);
			return NULL;
		}
	}

	*nItems = n;
	return (void *)array_double;
}

static
boolint Type_S15Fixed16_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	double * Value = (double *)Ptr;
	uint32 i;

	for(i = 0; i < nItems; i++) {
		if(!_cmsWrite15Fixed16Number(io, Value[i])) return FALSE;
	}

	return TRUE;

	CXX_UNUSED(self);
}

static void * Type_S15Fixed16_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, n * sizeof(double));
}

static void Type_S15Fixed16_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigU16Fixed16ArrayType
//
// This type represents an array of generic 4-byte/32-bit quantity.
// The number of values is determined from the size of the tag.
//
static void * Type_U16Fixed16_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	double *  array_double;
	uint32 v;
	uint32 i, n;
	*nItems = 0;
	n = SizeOfTag / sizeof(uint32);
	array_double = (double *)_cmsCalloc(self->ContextID, n, sizeof(double));
	if(array_double == NULL) 
		return NULL;
	for(i = 0; i < n; i++) {
		if(!_cmsReadUInt32Number(io, &v)) {
			_cmsFree(self->ContextID, (void *)array_double);
			return NULL;
		}
		// Convert to double
		array_double[i] =  (double)(v / 65536.0);
	}
	*nItems = n;
	return (void *)array_double;
}

static boolint Type_U16Fixed16_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	double * Value = (double *)Ptr;
	for(uint32 i = 0; i < nItems; i++) {
		uint32 v = (uint32)floor(Value[i]*65536.0 + 0.5);
		if(!_cmsWriteUInt32Number(io, v)) 
			return FALSE;
	}
	return TRUE;
	CXX_UNUSED(self);
}

static void * Type_U16Fixed16_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, n * sizeof(double));
}

static void Type_U16Fixed16_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigSignatureType
//
// The signatureType contains a four-byte sequence, Sequences of less than four
// characters are padded at the end with spaces, 20h.
// Typically this type is used for registered tags that can be displayed on many
// development systems as a sequence of four characters.
//
static void * Type_Signature_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsSignature* SigPtr = (cmsSignature*)_cmsMalloc(self->ContextID, sizeof(cmsSignature));
	if(SigPtr == NULL) 
		return NULL;
	if(!_cmsReadUInt32Number(io, SigPtr)) 
		return NULL;
	*nItems = 1;
	return SigPtr;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_Signature_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsSignature* SigPtr = (cmsSignature*)Ptr;
	return _cmsWriteUInt32Number(io, *SigPtr);
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Signature_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, n * sizeof(cmsSignature));
}

static void Type_Signature_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigTextType
//
// The textType is a simple text structure that contains a 7-bit ASCII text string.
// The length of the string is obtained by subtracting 8 from the element size portion
// of the tag itself. This string must be terminated with a 00h byte.
//
static void * Type_Text_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	char * Text = NULL;
	// Create a container
	cmsMLU* mlu = cmsMLUalloc(self->ContextID, 1);
	if(mlu == NULL) return NULL;
	*nItems = 0;
	// We need to store the "\0" at the end, so +1
	if(SizeOfTag == UINT_MAX) goto Error;
	Text = (char *)_cmsMalloc(self->ContextID, SizeOfTag + 1);
	if(Text == NULL) goto Error;

	if(io->Read(io, Text, sizeof(char), SizeOfTag) != SizeOfTag) goto Error;

	// Make sure text is properly ended
	Text[SizeOfTag] = 0;
	*nItems = 1;

	// Keep the result
	if(!cmsMLUsetASCII(mlu, cmsNoLanguage, cmsNoCountry, Text)) goto Error;

	_cmsFree(self->ContextID, Text);
	return (void *)mlu;

Error:
	if(mlu)
		cmsMLUfree(mlu);
	if(Text)
		_cmsFree(self->ContextID, Text);

	return NULL;
}

// The conversion implies to choose a language. So, we choose the actual language.
static boolint Type_Text_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	uint32 size;
	boolint rc;
	char * Text;

	// Get the size of the string. Note there is an extra "\0" at the end
	size = cmsMLUgetASCII(mlu, cmsNoLanguage, cmsNoCountry, NULL, 0);
	if(!size) return FALSE;    // Cannot be zero!

	// Create memory
	Text = (char *)_cmsMalloc(self->ContextID, size);
	if(Text == NULL) return FALSE;

	cmsMLUgetASCII(mlu, cmsNoLanguage, cmsNoCountry, Text, size);

	// Write it, including separator
	rc = io->Write(io, size, Text);

	_cmsFree(self->ContextID, Text);
	return rc;

	CXX_UNUSED(nItems);
}

static void * Type_Text_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsMLUdup((cmsMLU*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_Text_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	cmsMLUfree(mlu);
	return;
	CXX_UNUSED(self);
}

static cmsTagTypeSignature DecideTextType(double ICCVersion, const void * Data)
{
	if(ICCVersion >= 4.0)
		return cmsSigMultiLocalizedUnicodeType;
	return cmsSigTextType;
	CXX_UNUSED(Data);
}
//
// Type cmsSigDataType
//
// General purpose data type
static void * Type_Data_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsICCData* BinData;
	uint32 LenOfData;
	*nItems = 0;
	if(SizeOfTag < sizeof(uint32)) return NULL;
	LenOfData = SizeOfTag - sizeof(uint32);
	if(LenOfData > INT_MAX) return NULL;
	BinData = (cmsICCData*)_cmsMalloc(self->ContextID, sizeof(cmsICCData) + LenOfData - 1);
	if(BinData == NULL) return NULL;
	BinData->len = LenOfData;
	if(!_cmsReadUInt32Number(io, &BinData->flag)) {
		_cmsFree(self->ContextID, BinData);
		return NULL;
	}

	if(io->Read(io, BinData->data, sizeof(uint8), LenOfData) != LenOfData) {
		_cmsFree(self->ContextID, BinData);
		return NULL;
	}

	*nItems = 1;

	return (void *)BinData;
}

static boolint Type_Data_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsICCData* BinData = (cmsICCData*)Ptr;
	if(!_cmsWriteUInt32Number(io, BinData->flag)) return FALSE;
	return io->Write(io, BinData->len, BinData->data);
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Data_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	cmsICCData* BinData = (cmsICCData*)Ptr;
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsICCData) + BinData->len - 1);
	CXX_UNUSED(n);
}

static void Type_Data_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigTextDescriptionType
//
static void * Type_Text_Description_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	char * Text = NULL;
	cmsMLU* mlu = NULL;
	uint32 AsciiCount;
	uint32 i, UnicodeCode, UnicodeCount;
	uint16 ScriptCodeCode, Dummy;
	uint8 ScriptCodeCount;
	*nItems = 0;
	//  One dword should be there
	if(SizeOfTag < sizeof(uint32)) return NULL;
	// Read len of ASCII
	if(!_cmsReadUInt32Number(io, &AsciiCount)) return NULL;
	SizeOfTag -= sizeof(uint32);

	// Check for size
	if(SizeOfTag < AsciiCount) return NULL;

	// All seems Ok, allocate the container
	mlu = cmsMLUalloc(self->ContextID, 1);
	if(mlu == NULL) return NULL;

	// As many memory as size of tag
	Text = (char *)_cmsMalloc(self->ContextID, AsciiCount + 1);
	if(Text == NULL) goto Error;

	// Read it
	if(io->Read(io, Text, sizeof(char), AsciiCount) != AsciiCount) goto Error;
	SizeOfTag -= AsciiCount;

	// Make sure there is a terminator
	Text[AsciiCount] = 0;

	// Set the MLU entry. From here we can be tolerant to wrong types
	if(!cmsMLUsetASCII(mlu, cmsNoLanguage, cmsNoCountry, Text)) goto Error;
	_cmsFree(self->ContextID, (void *)Text);
	Text = NULL;

	// Skip Unicode code
	if(SizeOfTag < 2* sizeof(uint32)) goto Done;
	if(!_cmsReadUInt32Number(io, &UnicodeCode)) goto Done;
	if(!_cmsReadUInt32Number(io, &UnicodeCount)) goto Done;
	SizeOfTag -= 2* sizeof(uint32);

	if(SizeOfTag < UnicodeCount*sizeof(uint16)) goto Done;

	for(i = 0; i < UnicodeCount; i++) {
		if(!io->Read(io, &Dummy, sizeof(uint16), 1)) goto Done;
	}
	SizeOfTag -= UnicodeCount*sizeof(uint16);

	// Skip ScriptCode code if present. Some buggy profiles does have less
	// data that stricttly required. We need to skip it as this type may come
	// embedded in other types.

	if(SizeOfTag >= sizeof(uint16) + sizeof(uint8) + 67) {
		if(!_cmsReadUInt16Number(io, &ScriptCodeCode)) goto Done;
		if(!_cmsReadUInt8Number(io,  &ScriptCodeCount)) goto Done;

		// Skip rest of tag
		for(i = 0; i < 67; i++) {
			if(!io->Read(io, &Dummy, sizeof(uint8), 1)) goto Error;
		}
	}

Done:

	*nItems = 1;
	return mlu;

Error:
	if(Text) _cmsFree(self->ContextID, (void *)Text);
	if(mlu) cmsMLUfree(mlu);
	return NULL;
}

// This tag can come IN UNALIGNED SIZE. In order to prevent issues, we force zeros on description to align it
static boolint Type_Text_Description_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	char * Text = NULL;
	wchar_t * Wide = NULL;
	uint32 len, len_text, len_tag_requirement, len_aligned;
	boolint rc = FALSE;
	char Filler[68];
	// Used below for writing zeroes
	memzero(Filler, sizeof(Filler));
	// Get the len of string
	len = cmsMLUgetASCII(mlu, cmsNoLanguage, cmsNoCountry, NULL, 0);

	// Specification ICC.1:2001-04 (v2.4.0): It has been found that textDescriptionType can contain misaligned data
	//(see clause 4.1 for the definition of 'aligned'). Because the Unicode language
	// code and Unicode count immediately follow the ASCII description, their
	// alignment is not correct if the ASCII count is not a multiple of four. The
	// ScriptCode code is misaligned when the ASCII count is odd. Profile reading and
	// writing software must be written carefully in order to handle these alignment
	// problems.
	//
	// The above last sentence suggest to handle alignment issues in the
	// parser. The provided example (Table 69 on Page 60) makes this clear.
	// The padding only in the ASCII count is not sufficient for a aligned tag
	// size, with the same text size in ASCII and Unicode.

	// Null strings
	if(len <= 0) {
		Text = (char *)_cmsDupMem(self->ContextID, "", sizeof(char));
		Wide = (wchar_t *)_cmsDupMem(self->ContextID, L"", sizeof(wchar_t));
	}
	else {
		// Create independent buffers
		Text = (char *)_cmsCalloc(self->ContextID, len, sizeof(char));
		if(Text == NULL) goto Error;

		Wide = (wchar_t *)_cmsCalloc(self->ContextID, len, sizeof(wchar_t));
		if(Wide == NULL) goto Error;

		// Get both representations.
		cmsMLUgetASCII(mlu, cmsNoLanguage, cmsNoCountry,  Text, len * sizeof(char));
		cmsMLUgetWide(mlu,  cmsNoLanguage, cmsNoCountry,  Wide, len * sizeof(wchar_t));
	}

	// Tell the real text len including the null terminator and padding
	len_text = (uint32)strlen(Text) + 1;
	// Compute an total tag size requirement
	len_tag_requirement = (8+4+len_text+4+4+2*len_text+2+1+67);
	len_aligned = _cmsALIGNLONG(len_tag_requirement);

	// * uint32       count;          * Description length
	// * int8         desc[count]     * NULL terminated ascii string
	// * uint32       ucLangCode;     * UniCode language code
	// * uint32       ucCount;        * UniCode description length
	// * int16        ucDesc[ucCount];* The UniCode description
	// * uint16       scCode;         * ScriptCode code
	// * uint8        scCount;        * ScriptCode count
	// * int8         scDesc[67];     * ScriptCode Description

	if(!_cmsWriteUInt32Number(io, len_text)) goto Error;
	if(!io->Write(io, len_text, Text)) goto Error;

	if(!_cmsWriteUInt32Number(io, 0)) goto Error; // ucLanguageCode

	if(!_cmsWriteUInt32Number(io, len_text)) goto Error;
	// Note that in some compilers sizeof(uint16) != sizeof(wchar_t)
	if(!_cmsWriteWCharArray(io, len_text, Wide)) goto Error;

	// ScriptCode Code & count (unused)
	if(!_cmsWriteUInt16Number(io, 0)) goto Error;
	if(!_cmsWriteUInt8Number(io, 0)) goto Error;

	if(!io->Write(io, 67, Filler)) goto Error;

	// possibly add pad at the end of tag
	if(len_aligned - len_tag_requirement > 0)
		if(!io->Write(io, len_aligned - len_tag_requirement, Filler)) goto Error;

	rc = TRUE;

Error:
	if(Text) _cmsFree(self->ContextID, Text);
	if(Wide) _cmsFree(self->ContextID, Wide);

	return rc;

	CXX_UNUSED(nItems);
}

static void * Type_Text_Description_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsMLUdup((cmsMLU*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_Text_Description_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	cmsMLUfree(mlu);
	return;
	CXX_UNUSED(self);
}

static cmsTagTypeSignature DecideTextDescType(double ICCVersion, const void * Data)
{
	if(ICCVersion >= 4.0)
		return cmsSigMultiLocalizedUnicodeType;
	return cmsSigTextDescriptionType;
	CXX_UNUSED(Data);
}
//
// Type cmsSigCurveType
//
static void * Type_Curve_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint32 Count;
	cmsToneCurve * NewGamma;
	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	switch(Count) {
		case 0: // Linear.
	    {
		    double SingleGamma = 1.0;
		    NewGamma = cmsBuildParametricToneCurve(self->ContextID, 1, &SingleGamma);
		    if(!NewGamma) 
				return NULL;
		    *nItems = 1;
		    return NewGamma;
	    }
		case 1: // Specified as the exponent of gamma function
	    {
		    uint16 SingleGammaFixed;
		    double SingleGamma;
		    if(!_cmsReadUInt16Number(io, &SingleGammaFixed)) 
				return NULL;
		    SingleGamma = _cms8Fixed8toDouble(SingleGammaFixed);
		    *nItems = 1;
		    return cmsBuildParametricToneCurve(self->ContextID, 1, &SingleGamma);
	    }

		default: // Curve

		    if(Count > 0x7FFF)
			    return NULL; // This is to prevent bad guys for doing bad things

		    NewGamma = cmsBuildTabulatedToneCurve16(self->ContextID, Count, NULL);
		    if(!NewGamma) return NULL;

		    if(!_cmsReadUInt16Array(io, Count, NewGamma->Table16)) {
			    cmsFreeToneCurve(NewGamma);
			    return NULL;
		    }

		    *nItems = 1;
		    return NewGamma;
	}

	CXX_UNUSED(SizeOfTag);
}

static boolint Type_Curve_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsToneCurve * Curve = (cmsToneCurve *)Ptr;
	if(Curve->nSegments == 1 && Curve->Segments[0].Type == 1) {
		// Single gamma, preserve number
		uint16 SingleGammaFixed = _cmsDoubleTo8Fixed8(Curve->Segments[0].Params[0]);

		if(!_cmsWriteUInt32Number(io, 1)) return FALSE;
		if(!_cmsWriteUInt16Number(io, SingleGammaFixed)) return FALSE;
		return TRUE;
	}

	if(!_cmsWriteUInt32Number(io, Curve->nEntries)) return FALSE;
	return _cmsWriteUInt16Array(io, Curve->nEntries, Curve->Table16);

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Curve_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsDupToneCurve((cmsToneCurve *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_Curve_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsToneCurve * gamma = (cmsToneCurve *)Ptr;
	cmsFreeToneCurve(gamma);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigParametricCurveType
//
// Decide which curve type to use on writing
static cmsTagTypeSignature DecideCurveType(double ICCVersion, const void * Data)
{
	cmsToneCurve * Curve = (cmsToneCurve *)Data;
	if(ICCVersion < 4.0) return cmsSigCurveType;
	if(Curve->nSegments != 1) return cmsSigCurveType;        // Only 1-segment curves can be saved as parametric
	if(Curve->Segments[0].Type < 0) return cmsSigCurveType;  // Only non-inverted curves
	if(Curve->Segments[0].Type > 5) return cmsSigCurveType;  // Only ICC parametric curves
	return cmsSigParametricCurveType;
}

static void * Type_ParametricCurve_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	static const int ParamsByType[] = { 1, 3, 4, 5, 7 };
	double Params[10];
	uint16 Type;
	int i, n;
	cmsToneCurve * NewGamma;

	if(!_cmsReadUInt16Number(io, &Type)) return NULL;
	if(!_cmsReadUInt16Number(io, NULL)) return NULL; // Reserved

	if(Type > 4) {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown parametric curve type '%d'", Type);
		return NULL;
	}
	memzero(Params, sizeof(Params));
	n = ParamsByType[Type];
	for(i = 0; i < n; i++) {
		if(!_cmsRead15Fixed16Number(io, &Params[i])) return NULL;
	}
	NewGamma = cmsBuildParametricToneCurve(self->ContextID, Type+1, Params);
	*nItems = 1;
	return NewGamma;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_ParametricCurve_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsToneCurve * Curve = (cmsToneCurve *)Ptr;
	int i, nParams;
	static const int ParamsByType[] = { 0, 1, 3, 4, 5, 7 };
	int typen = Curve->Segments[0].Type;
	if(Curve->nSegments > 1 || typen < 1) {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Multisegment or Inverted parametric curves cannot be written");
		return FALSE;
	}
	if(typen > 5) {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported parametric curve");
		return FALSE;
	}

	nParams = ParamsByType[typen];

	if(!_cmsWriteUInt16Number(io, (uint16)(Curve->Segments[0].Type - 1))) return FALSE;
	if(!_cmsWriteUInt16Number(io, 0)) return FALSE;     // Reserved

	for(i = 0; i < nParams; i++) {
		if(!_cmsWrite15Fixed16Number(io, Curve->Segments[0].Params[i])) return FALSE;
	}

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_ParametricCurve_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsDupToneCurve((cmsToneCurve *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_ParametricCurve_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsToneCurve * gamma = (cmsToneCurve *)Ptr;
	cmsFreeToneCurve(gamma);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigDateTimeType
//
// A 12-byte value representation of the time and date, where the byte usage is assigned
// as specified in table 1. The actual values are encoded as 16-bit unsigned integers
// (uInt16Number - see 5.1.6).
//
// All the dateTimeNumber values in a profile shall be in Coordinated Universal Time
// (UTC, also known as GMT or ZULU Time). Profile writers are required to convert local
// time to UTC when setting these values. Programmes that display these values may show
// the dateTimeNumber as UTC, show the equivalent local time (at current locale), or
// display both UTC and local versions of the dateTimeNumber.
//
static void * Type_DateTime_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsDateTimeNumber timestamp;
	struct tm * NewDateTime;
	*nItems = 0;
	NewDateTime = (struct tm*)_cmsMalloc(self->ContextID, sizeof(struct tm));
	if(NewDateTime == NULL) return NULL;
	if(io->Read(io, &timestamp, sizeof(cmsDateTimeNumber), 1) != 1) return NULL;
	_cmsDecodeDateTimeNumber(&timestamp, NewDateTime);
	*nItems = 1;
	return NewDateTime;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_DateTime_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	struct tm * DateTime = (struct tm*)Ptr;
	cmsDateTimeNumber timestamp;
	_cmsEncodeDateTimeNumber(&timestamp, DateTime);
	if(!io->Write(io, sizeof(cmsDateTimeNumber), &timestamp)) return FALSE;
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_DateTime_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(struct tm));
	CXX_UNUSED(n);
}

static void Type_DateTime_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type icMeasurementType
//
/*
   The measurementType information refers only to the internal profile data and is
   meant to provide profile makers an alternative to the default measurement
   specifications.
 */
static void * Type_Measurement_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsICCMeasurementConditions mc;
	memzero(&mc, sizeof(mc));
	if(!_cmsReadUInt32Number(io, &mc.Observer)) return NULL;
	if(!_cmsReadXYZNumber(io,    &mc.Backing)) return NULL;
	if(!_cmsReadUInt32Number(io, &mc.Geometry)) return NULL;
	if(!_cmsRead15Fixed16Number(io, &mc.Flare)) return NULL;
	if(!_cmsReadUInt32Number(io, &mc.IlluminantType)) return NULL;

	*nItems = 1;
	return _cmsDupMem(self->ContextID, &mc, sizeof(cmsICCMeasurementConditions));

	CXX_UNUSED(SizeOfTag);
}

static boolint Type_Measurement_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsICCMeasurementConditions* mc = (cmsICCMeasurementConditions*)Ptr;
	if(!_cmsWriteUInt32Number(io, mc->Observer)) return FALSE;
	if(!_cmsWriteXYZNumber(io,    &mc->Backing)) return FALSE;
	if(!_cmsWriteUInt32Number(io, mc->Geometry)) return FALSE;
	if(!_cmsWrite15Fixed16Number(io, mc->Flare)) return FALSE;
	if(!_cmsWriteUInt32Number(io, mc->IlluminantType)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Measurement_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsICCMeasurementConditions));
	CXX_UNUSED(n);
}

static void Type_Measurement_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigMultiLocalizedUnicodeType
//
//   Do NOT trust SizeOfTag as there is an issue on the definition of profileSequenceDescTag. See the TechNote from
//   Max Derhak and Rohit Patil about this: basically the size of the string table should be guessed and cannot be
//   taken from the size of tag if this tag is embedded as part of bigger structures (profileSequenceDescTag, for instance)
//
static void * Type_MLU_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsMLU* mlu;
	uint32 Count, RecLen, NumOfWchar;
	uint32 SizeOfHeader;
	uint32 Len, Offset;
	uint32 i;
	wchar_t *         Block;
	uint32 BeginOfThisString, EndOfThisString, LargestPosition;

	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	if(!_cmsReadUInt32Number(io, &RecLen)) return NULL;

	if(RecLen != 12) {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "multiLocalizedUnicodeType of len != 12 is not supported.");
		return NULL;
	}

	mlu = cmsMLUalloc(self->ContextID, Count);
	if(mlu == NULL) return NULL;

	mlu->UsedEntries = Count;

	SizeOfHeader = 12 * Count + sizeof(_cmsTagBase);
	LargestPosition = 0;

	for(i = 0; i < Count; i++) {
		if(!_cmsReadUInt16Number(io, &mlu->Entries[i].Language)) goto Error;
		if(!_cmsReadUInt16Number(io, &mlu->Entries[i].Country)) goto Error;

		// Now deal with Len and offset.
		if(!_cmsReadUInt32Number(io, &Len)) goto Error;
		if(!_cmsReadUInt32Number(io, &Offset)) goto Error;

		// Check for overflow
		if(Offset < (SizeOfHeader + 8)) goto Error;
		if(((Offset + Len) < Len) || ((Offset + Len) > SizeOfTag + 8)) goto Error;

		// True begin of the string
		BeginOfThisString = Offset - SizeOfHeader - 8;

		// Adjust to wchar_t elements
		mlu->Entries[i].Len = (Len * sizeof(wchar_t)) / sizeof(uint16);
		mlu->Entries[i].StrW = (BeginOfThisString * sizeof(wchar_t)) / sizeof(uint16);

		// To guess maximum size, add offset + len
		EndOfThisString = BeginOfThisString + Len;
		if(EndOfThisString > LargestPosition)
			LargestPosition = EndOfThisString;
	}

	// Now read the remaining of tag and fill all strings. Subtract the directory
	SizeOfTag   = (LargestPosition * sizeof(wchar_t)) / sizeof(uint16);
	if(SizeOfTag == 0) {
		Block = NULL;
		NumOfWchar = 0;
	}
	else {
		Block = (wchar_t *)_cmsMalloc(self->ContextID, SizeOfTag);
		if(Block == NULL) goto Error;
		NumOfWchar = SizeOfTag / sizeof(wchar_t);
		if(!_cmsReadWCharArray(io, NumOfWchar, Block)) goto Error;
	}

	mlu->MemPool  = Block;
	mlu->PoolSize = SizeOfTag;
	mlu->PoolUsed = SizeOfTag;

	*nItems = 1;
	return (void *)mlu;
Error:
	if(mlu) cmsMLUfree(mlu);
	return NULL;
}

static boolint Type_MLU_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	uint32 HeaderSize;
	uint32 Len, Offset;
	uint32 i;

	if(Ptr == NULL) {
		// Empty placeholder
		if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
		if(!_cmsWriteUInt32Number(io, 12)) return FALSE;
		return TRUE;
	}

	if(!_cmsWriteUInt32Number(io, mlu->UsedEntries)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 12)) return FALSE;

	HeaderSize = 12 * mlu->UsedEntries + sizeof(_cmsTagBase);

	for(i = 0; i < mlu->UsedEntries; i++) {
		Len    =  mlu->Entries[i].Len;
		Offset =  mlu->Entries[i].StrW;

		Len    = (Len * sizeof(uint16)) / sizeof(wchar_t);
		Offset = (Offset * sizeof(uint16)) / sizeof(wchar_t) + HeaderSize + 8;

		if(!_cmsWriteUInt16Number(io, mlu->Entries[i].Language)) return FALSE;
		if(!_cmsWriteUInt16Number(io, mlu->Entries[i].Country)) return FALSE;
		if(!_cmsWriteUInt32Number(io, Len)) return FALSE;
		if(!_cmsWriteUInt32Number(io, Offset)) return FALSE;
	}

	if(!_cmsWriteWCharArray(io, mlu->PoolUsed / sizeof(wchar_t), (wchar_t *)mlu->MemPool)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_MLU_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsMLUdup((cmsMLU*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_MLU_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsMLUfree((cmsMLU*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigLut8Type
//
// Decide which LUT type to use on writing
static cmsTagTypeSignature DecideLUTtypeA2B(double ICCVersion, const void * Data)
{
	cmsPipeline * Lut = (cmsPipeline *)Data;
	if(ICCVersion < 4.0) {
		if(Lut->SaveAs8Bits) return cmsSigLut8Type;
		return cmsSigLut16Type;
	}
	else {
		return cmsSigLutAtoBType;
	}
}

static cmsTagTypeSignature DecideLUTtypeB2A(double ICCVersion, const void * Data)
{
	cmsPipeline * Lut = (cmsPipeline *)Data;
	if(ICCVersion < 4.0) {
		if(Lut->SaveAs8Bits) return cmsSigLut8Type;
		return cmsSigLut16Type;
	}
	else {
		return cmsSigLutBtoAType;
	}
}

/*
   This structure represents a colour transform using tables of 8-bit precision.
   This type contains four processing elements: a 3 by 3 matrix (which shall be
   the identity matrix unless the input colour space is XYZ), a set of one dimensional
   input tables, a multidimensional lookup table, and a set of one dimensional output
   tables. Data is processed using these elements via the following sequence:
   (matrix) -> (1d input tables)  -> (multidimensional lookup table - CLUT) -> (1d output tables)

   Byte Position   Field Length (bytes)  Content Encoded as...
   8                  1          Number of Input Channels (i)    uInt8Number
   9                  1          Number of Output Channels (o)   uInt8Number
   10                 1          Number of CLUT grid points (identical for each side) (g) uInt8Number
   11                 1          Reserved for padding (fill with 00h)

   12..15             4          Encoded e00 parameter   s15Fixed16Number
 */

// Read 8 bit tables as gamma functions
static boolint Read8bitTables(cmsContext ContextID, cmsIOHANDLER* io, cmsPipeline * lut, uint32 nChannels)
{
	uint8 * Temp = NULL;
	uint32 i, j;
	cmsToneCurve * Tables[cmsMAXCHANNELS];
	if(nChannels > cmsMAXCHANNELS) return FALSE;
	if(nChannels <= 0) return FALSE;
	memzero(Tables, sizeof(Tables));
	Temp = (uint8 *)_cmsMalloc(ContextID, 256);
	if(Temp == NULL) 
		return FALSE;
	for(i = 0; i < nChannels; i++) {
		Tables[i] = cmsBuildTabulatedToneCurve16(ContextID, 256, NULL);
		if(Tables[i] == NULL) 
			goto Error;
	}
	for(i = 0; i < nChannels; i++) {
		if(io->Read(io, Temp, 256, 1) != 1) 
			goto Error;
		for(j = 0; j < 256; j++)
			Tables[i]->Table16[j] = (uint16)FROM_8_TO_16(Temp[j]);
	}
	_cmsFree(ContextID, Temp);
	Temp = NULL;
	if(!cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, nChannels, Tables)))
		goto Error;
	for(i = 0; i < nChannels; i++)
		cmsFreeToneCurve(Tables[i]);
	return TRUE;
Error:
	for(i = 0; i < nChannels; i++) {
		cmsFreeToneCurve(Tables[i]);
	}
	_cmsFree(ContextID, Temp);
	return FALSE;
}

static boolint Write8bitTables(cmsContext ContextID, cmsIOHANDLER* io, uint32 n, _cmsStageToneCurvesData* Tables)
{
	int j;
	uint8 val;
	for(uint32 i = 0; i < n; i++) {
		if(Tables) {
			// Usual case of identity curves
			if((Tables->TheCurves[i]->nEntries == 2) && (Tables->TheCurves[i]->Table16[0] == 0) && (Tables->TheCurves[i]->Table16[1] == 65535)) {
				for(j = 0; j < 256; j++) {
					if(!_cmsWriteUInt8Number(io, (uint8)j)) 
						return FALSE;
				}
			}
			else if(Tables->TheCurves[i]->nEntries != 256) {
				cmsSignalError(ContextID, cmsERROR_RANGE, "LUT8 needs 256 entries on prelinearization");
				return FALSE;
			}
			else
				for(j = 0; j < 256; j++) {
					val = (uint8)FROM_16_TO_8(Tables->TheCurves[i]->Table16[j]);
					if(!_cmsWriteUInt8Number(io, val)) 
						return FALSE;
				}
		}
	}
	return TRUE;
}

// Check overflow
static uint32 uipow(uint32 n, uint32 a, uint32 b)
{
	uint32 rv = 1;
	uint32 rc;
	if(!a) 
		return 0;
	if(!n) 
		return 0;
	for(; b > 0; b--) {
		rv *= a;
		// Check for overflow
		if(rv > UINT_MAX / a) 
			return _FFFF32;
	}
	rc = rv * n;
	if(rv != rc / n) 
		return _FFFF32;
	return rc;
}

// That will create a MPE LUT with Matrix, pre tables, CLUT and post tables.
// 8 bit lut may be scaled easely to v4 PCS, but we need also to properly adjust
// PCS on BToAxx tags and AtoB if abstract. We need to fix input direction.

static void * Type_LUT8_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint8 InputChannels, OutputChannels, CLUTpoints;
	uint8 * Temp = NULL;
	cmsPipeline * NewLUT = NULL;
	uint32 nTabSize, i;
	double Matrix[3*3];
	*nItems = 0;
	if(!_cmsReadUInt8Number(io, &InputChannels)) goto Error;
	if(!_cmsReadUInt8Number(io, &OutputChannels)) goto Error;
	if(!_cmsReadUInt8Number(io, &CLUTpoints)) goto Error;
	if(CLUTpoints == 1) goto Error; // Impossible value, 0 for no CLUT and then 2 at least
	// Padding
	if(!_cmsReadUInt8Number(io, NULL)) goto Error;
	// Do some checking
	if(InputChannels == 0 || InputChannels > cmsMAXCHANNELS) goto Error;
	if(OutputChannels == 0 || OutputChannels > cmsMAXCHANNELS) goto Error;
	// Allocates an empty Pipeline
	NewLUT = cmsPipelineAlloc(self->ContextID, InputChannels, OutputChannels);
	if(NewLUT == NULL) goto Error;
	// Read the Matrix
	if(!_cmsRead15Fixed16Number(io,  &Matrix[0])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[1])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[2])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[3])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[4])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[5])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[6])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[7])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[8])) goto Error;

	// Only operates if not identity...
	if((InputChannels == 3) && !_cmsMAT3isIdentity((cmsMAT3*)Matrix)) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_BEGIN, cmsStageAllocMatrix(self->ContextID, 3, 3, Matrix, NULL)))
			goto Error;
	}

	// Get input tables
	if(!Read8bitTables(self->ContextID, io,  NewLUT, InputChannels)) goto Error;

	// Get 3D CLUT. Check the overflow....
	nTabSize = uipow(OutputChannels, CLUTpoints, InputChannels);
	if(nTabSize == (uint32) -1) goto Error;
	if(nTabSize > 0) {
		uint16 * PtrW, * T;

		PtrW = T  = (uint16*)_cmsCalloc(self->ContextID, nTabSize, sizeof(uint16));
		if(T  == NULL) goto Error;

		Temp = (uint8 *)_cmsMalloc(self->ContextID, nTabSize);
		if(Temp == NULL) {
			_cmsFree(self->ContextID, T);
			goto Error;
		}

		if(io->Read(io, Temp, nTabSize, 1) != 1) {
			_cmsFree(self->ContextID, T);
			_cmsFree(self->ContextID, Temp);
			goto Error;
		}
		for(i = 0; i < nTabSize; i++) {
			*PtrW++ = FROM_8_TO_16(Temp[i]);
		}
		_cmsFree(self->ContextID, Temp);
		Temp = NULL;
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, cmsStageAllocCLut16bit(self->ContextID, CLUTpoints, InputChannels, OutputChannels, T))) {
			_cmsFree(self->ContextID, T);
			goto Error;
		}
		_cmsFree(self->ContextID, T);
	}
	// Get output tables
	if(!Read8bitTables(self->ContextID, io,  NewLUT, OutputChannels)) goto Error;
	*nItems = 1;
	return NewLUT;
Error:
	cmsPipelineFree(NewLUT);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// We only allow a specific MPE structure: Matrix plus prelin, plus clut, plus post-lin.
static boolint Type_LUT8_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint32 j, nTabSize, i, n;
	uint8 val;
	cmsPipeline * NewLUT = (cmsPipeline *)Ptr;
	cmsStage * mpe;
	_cmsStageToneCurvesData* PreMPE = NULL, * PostMPE = NULL;
	_cmsStageMatrixData* MatMPE = NULL;
	_cmsStageCLutData* clut = NULL;
	uint32 clutPoints;
	// Disassemble the LUT into components.
	mpe = NewLUT->Elements;
	if(mpe->Type == cmsSigMatrixElemType) {
		MatMPE = (_cmsStageMatrixData*)mpe->Data;
		mpe = mpe->Next;
	}
	if(mpe && mpe->Type == cmsSigCurveSetElemType) {
		PreMPE = (_cmsStageToneCurvesData*)mpe->Data;
		mpe = mpe->Next;
	}

	if(mpe && mpe->Type == cmsSigCLutElemType) {
		clut  = (_cmsStageCLutData*)mpe->Data;
		mpe = mpe->Next;
	}

	if(mpe && mpe->Type == cmsSigCurveSetElemType) {
		PostMPE = (_cmsStageToneCurvesData*)mpe->Data;
		mpe = mpe->Next;
	}

	// That should be all
	if(mpe) {
		cmsSignalError(mpe->ContextID, cmsERROR_UNKNOWN_EXTENSION, "LUT is not suitable to be saved as LUT8");
		return FALSE;
	}

	if(clut == NULL)
		clutPoints = 0;
	else
		clutPoints    = clut->Params->nSamples[0];

	if(!_cmsWriteUInt8Number(io, (uint8)NewLUT->InputChannels)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)NewLUT->OutputChannels)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)clutPoints)) return FALSE;
	if(!_cmsWriteUInt8Number(io, 0)) return FALSE; // Padding

	n = NewLUT->InputChannels * NewLUT->OutputChannels;

	if(MatMPE) {
		for(i = 0; i < n; i++) {
			if(!_cmsWrite15Fixed16Number(io, MatMPE->Double[i])) return FALSE;
		}
	}
	else {
		if(n != 9) return FALSE;

		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
	}

	// The prelinearization table
	if(!Write8bitTables(self->ContextID, io, NewLUT->InputChannels, PreMPE)) return FALSE;

	nTabSize = uipow(NewLUT->OutputChannels, clutPoints, NewLUT->InputChannels);
	if(nTabSize == (uint32) -1) return FALSE;
	if(nTabSize > 0) {
		// The 3D CLUT.
		if(clut) {
			for(j = 0; j < nTabSize; j++) {
				val = (uint8)FROM_16_TO_8(clut->Tab.T[j]);
				if(!_cmsWriteUInt8Number(io, val)) return FALSE;
			}
		}
	}

	// The postlinearization table
	if(!Write8bitTables(self->ContextID, io, NewLUT->OutputChannels, PostMPE)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_LUT8_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsPipelineDup((cmsPipeline *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_LUT8_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsPipelineFree((cmsPipeline *)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigLut16Type
//
// Read 16 bit tables as gamma functions
static boolint Read16bitTables(cmsContext ContextID, cmsIOHANDLER* io, cmsPipeline * lut, uint32 nChannels, uint32 nEntries)
{
	uint32 i;
	cmsToneCurve * Tables[cmsMAXCHANNELS];
	// Maybe an empty table? (this is a lcms extension)
	if(nEntries <= 0) return TRUE;
	// Check for malicious profiles
	if(nEntries < 2) return FALSE;
	if(nChannels > cmsMAXCHANNELS) return FALSE;
	// Init table to zero
	memzero(Tables, sizeof(Tables));
	for(i = 0; i < nChannels; i++) {
		Tables[i] = cmsBuildTabulatedToneCurve16(ContextID, nEntries, NULL);
		if(Tables[i] == NULL) goto Error;

		if(!_cmsReadUInt16Array(io, nEntries, Tables[i]->Table16)) goto Error;
	}
	// Add the table (which may certainly be an identity, but this is up to the optimizer, not the reading code)
	if(!cmsPipelineInsertStage(lut, cmsAT_END, cmsStageAllocToneCurves(ContextID, nChannels, Tables)))
		goto Error;
	for(i = 0; i < nChannels; i++)
		cmsFreeToneCurve(Tables[i]);
	return TRUE;
Error:
	for(i = 0; i < nChannels; i++) {
		cmsFreeToneCurve(Tables[i]);
	}
	return FALSE;
}

static boolint Write16bitTables(cmsContext ContextID, cmsIOHANDLER* io, _cmsStageToneCurvesData* Tables)
{
	uint32 j;
	uint32 i;
	uint16 val;
	uint32 nEntries;
	assert(Tables != NULL);
	nEntries = Tables->TheCurves[0]->nEntries;
	for(i = 0; i < Tables->nCurves; i++) {
		for(j = 0; j < nEntries; j++) {
			val = Tables->TheCurves[i]->Table16[j];
			if(!_cmsWriteUInt16Number(io, val)) return FALSE;
		}
	}
	return TRUE;

	CXX_UNUSED(ContextID);
}

static void * Type_LUT16_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint8 InputChannels, OutputChannels, CLUTpoints;
	cmsPipeline * NewLUT = NULL;
	uint32 nTabSize;
	double Matrix[3*3];
	uint16 InputEntries, OutputEntries;
	*nItems = 0;
	if(!_cmsReadUInt8Number(io, &InputChannels)) return NULL;
	if(!_cmsReadUInt8Number(io, &OutputChannels)) return NULL;
	if(!_cmsReadUInt8Number(io, &CLUTpoints)) return NULL; // 255 maximum
	// Padding
	if(!_cmsReadUInt8Number(io, NULL)) return NULL;

	// Do some checking
	if(InputChannels == 0 || InputChannels > cmsMAXCHANNELS) goto Error;
	if(OutputChannels == 0 || OutputChannels > cmsMAXCHANNELS) goto Error;

	// Allocates an empty LUT
	NewLUT = cmsPipelineAlloc(self->ContextID, InputChannels, OutputChannels);
	if(NewLUT == NULL) goto Error;

	// Read the Matrix
	if(!_cmsRead15Fixed16Number(io,  &Matrix[0])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[1])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[2])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[3])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[4])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[5])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[6])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[7])) goto Error;
	if(!_cmsRead15Fixed16Number(io,  &Matrix[8])) goto Error;

	// Only operates on 3 channels
	if((InputChannels == 3) && !_cmsMAT3isIdentity((cmsMAT3*)Matrix)) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, cmsStageAllocMatrix(self->ContextID, 3, 3, Matrix, NULL)))
			goto Error;
	}

	if(!_cmsReadUInt16Number(io, &InputEntries)) goto Error;
	if(!_cmsReadUInt16Number(io, &OutputEntries)) goto Error;

	if(InputEntries > 0x7FFF || OutputEntries > 0x7FFF) goto Error;
	if(CLUTpoints == 1) goto Error; // Impossible value, 0 for no CLUT and then 2 at least

	// Get input tables
	if(!Read16bitTables(self->ContextID, io,  NewLUT, InputChannels, InputEntries)) goto Error;
	// Get 3D CLUT
	nTabSize = uipow(OutputChannels, CLUTpoints, InputChannels);
	if(nTabSize == (uint32) -1) goto Error;
	if(nTabSize > 0) {
		uint16 * T  = (uint16*)_cmsCalloc(self->ContextID, nTabSize, sizeof(uint16));
		if(T  == NULL) 
			goto Error;
		if(!_cmsReadUInt16Array(io, nTabSize, T)) {
			_cmsFree(self->ContextID, T);
			goto Error;
		}
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, cmsStageAllocCLut16bit(self->ContextID, CLUTpoints, InputChannels, OutputChannels, T))) {
			_cmsFree(self->ContextID, T);
			goto Error;
		}
		_cmsFree(self->ContextID, T);
	}
	// Get output tables
	if(!Read16bitTables(self->ContextID, io,  NewLUT, OutputChannels, OutputEntries)) 
		goto Error;
	*nItems = 1;
	return NewLUT;
Error:
	cmsPipelineFree(NewLUT);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// We only allow some specific MPE structures: Matrix plus prelin, plus clut, plus post-lin.
// Some empty defaults are created for missing parts

static boolint Type_LUT16_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint32 nTabSize;
	cmsPipeline * NewLUT = (cmsPipeline *)Ptr;
	cmsStage * mpe;
	_cmsStageToneCurvesData* PreMPE = NULL, * PostMPE = NULL;
	_cmsStageMatrixData* MatMPE = NULL;
	_cmsStageCLutData* clut = NULL;
	uint32 i, InputChannels, OutputChannels, clutPoints;
	// Disassemble the LUT into components.
	mpe = NewLUT->Elements;
	if(mpe && mpe->Type == cmsSigMatrixElemType) {
		MatMPE = (_cmsStageMatrixData*)mpe->Data;
		if(mpe->InputChannels != 3 || mpe->OutputChannels != 3) return FALSE;
		mpe = mpe->Next;
	}

	if(mpe && mpe->Type == cmsSigCurveSetElemType) {
		PreMPE = (_cmsStageToneCurvesData*)mpe->Data;
		mpe = mpe->Next;
	}

	if(mpe && mpe->Type == cmsSigCLutElemType) {
		clut  = (_cmsStageCLutData*)mpe->Data;
		mpe = mpe->Next;
	}

	if(mpe && mpe->Type == cmsSigCurveSetElemType) {
		PostMPE = (_cmsStageToneCurvesData*)mpe->Data;
		mpe = mpe->Next;
	}

	// That should be all
	if(mpe) {
		cmsSignalError(mpe->ContextID, cmsERROR_UNKNOWN_EXTENSION, "LUT is not suitable to be saved as LUT16");
		return FALSE;
	}

	InputChannels  = cmsPipelineInputChannels(NewLUT);
	OutputChannels = cmsPipelineOutputChannels(NewLUT);

	if(clut == NULL)
		clutPoints = 0;
	else
		clutPoints    = clut->Params->nSamples[0];

	if(!_cmsWriteUInt8Number(io, (uint8)InputChannels)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)OutputChannels)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)clutPoints)) return FALSE;
	if(!_cmsWriteUInt8Number(io, 0)) return FALSE; // Padding

	if(MatMPE) {
		for(i = 0; i < 9; i++) {
			if(!_cmsWrite15Fixed16Number(io, MatMPE->Double[i])) return FALSE;
		}
	}
	else {
		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, 1)) return FALSE;
	}

	if(PreMPE) {
		if(!_cmsWriteUInt16Number(io, (uint16)PreMPE->TheCurves[0]->nEntries)) return FALSE;
	}
	else {
		if(!_cmsWriteUInt16Number(io, 2)) return FALSE;
	}

	if(PostMPE) {
		if(!_cmsWriteUInt16Number(io, (uint16)PostMPE->TheCurves[0]->nEntries)) return FALSE;
	}
	else {
		if(!_cmsWriteUInt16Number(io, 2)) return FALSE;
	}

	// The prelinearization table

	if(PreMPE) {
		if(!Write16bitTables(self->ContextID, io, PreMPE)) return FALSE;
	}
	else {
		for(i = 0; i < InputChannels; i++) {
			if(!_cmsWriteUInt16Number(io, 0)) return FALSE;
			if(!_cmsWriteUInt16Number(io, 0xffff)) return FALSE;
		}
	}

	nTabSize = uipow(OutputChannels, clutPoints, InputChannels);
	if(nTabSize == (uint32) -1) return FALSE;
	if(nTabSize > 0) {
		// The 3D CLUT.
		if(clut) {
			if(!_cmsWriteUInt16Array(io, nTabSize, clut->Tab.T)) return FALSE;
		}
	}

	// The postlinearization table
	if(PostMPE) {
		if(!Write16bitTables(self->ContextID, io, PostMPE)) return FALSE;
	}
	else {
		for(i = 0; i < OutputChannels; i++) {
			if(!_cmsWriteUInt16Number(io, 0)) return FALSE;
			if(!_cmsWriteUInt16Number(io, 0xffff)) return FALSE;
		}
	}

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_LUT16_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsPipelineDup((cmsPipeline *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_LUT16_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsPipelineFree((cmsPipeline *)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigLutAToBType
//
// V4 stuff. Read matrix for LutAtoB and LutBtoA
static cmsStage * ReadMatrix(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 Offset)
{
	double dMat[3*3];
	double dOff[3];
	cmsStage * Mat;
	// Go to address
	if(!io->Seek(io, Offset)) return NULL;
	// Read the Matrix
	if(!_cmsRead15Fixed16Number(io, &dMat[0])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[1])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[2])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[3])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[4])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[5])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[6])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[7])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dMat[8])) return NULL;

	if(!_cmsRead15Fixed16Number(io, &dOff[0])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dOff[1])) return NULL;
	if(!_cmsRead15Fixed16Number(io, &dOff[2])) return NULL;

	Mat = cmsStageAllocMatrix(self->ContextID, 3, 3, dMat, dOff);

	return Mat;
}

//  V4 stuff. Read CLUT part for LutAtoB and LutBtoA

static cmsStage * ReadCLUT(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 Offset, uint32 InputChannels, uint32 OutputChannels)
{
	uint8 gridPoints8[cmsMAXCHANNELS]; // Number of grid points in each dimension.
	uint32 GridPoints[cmsMAXCHANNELS], i;
	uint8 Precision;
	cmsStage * CLUT;
	_cmsStageCLutData* Data;
	if(!io->Seek(io, Offset)) return NULL;
	if(io->Read(io, gridPoints8, cmsMAXCHANNELS, 1) != 1) return NULL;
	for(i = 0; i < cmsMAXCHANNELS; i++) {
		if(gridPoints8[i] == 1) return NULL; // Impossible value, 0 for no CLUT and then 2 at least
		GridPoints[i] = gridPoints8[i];
	}
	if(!_cmsReadUInt8Number(io, &Precision)) return NULL;
	if(!_cmsReadUInt8Number(io, NULL)) return NULL;
	if(!_cmsReadUInt8Number(io, NULL)) return NULL;
	if(!_cmsReadUInt8Number(io, NULL)) return NULL;
	CLUT = cmsStageAllocCLut16bitGranular(self->ContextID, GridPoints, InputChannels, OutputChannels, NULL);
	if(CLUT == NULL) return NULL;
	Data = (_cmsStageCLutData*)CLUT->Data;
	// Precision can be 1 or 2 bytes
	if(Precision == 1) {
		uint8 v;
		for(i = 0; i < Data->nEntries; i++) {
			if(io->Read(io, &v, sizeof(uint8), 1) != 1) {
				cmsStageFree(CLUT);
				return NULL;
			}
			Data->Tab.T[i] = FROM_8_TO_16(v);
		}
	}
	else if(Precision == 2) {
		if(!_cmsReadUInt16Array(io, Data->nEntries, Data->Tab.T)) {
			cmsStageFree(CLUT);
			return NULL;
		}
	}
	else {
		cmsStageFree(CLUT);
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown precision of '%d'", Precision);
		return NULL;
	}
	return CLUT;
}

static cmsToneCurve * ReadEmbeddedCurve(struct _cms_typehandler_struct* self, cmsIOHANDLER* io)
{
	uint32 nItems;
	cmsTagTypeSignature BaseType = _cmsReadTypeBase(io);
	switch(BaseType) {
		case cmsSigCurveType:
		    return (cmsToneCurve *)Type_Curve_Read(self, io, &nItems, 0);
		case cmsSigParametricCurveType:
		    return (cmsToneCurve *)Type_ParametricCurve_Read(self, io, &nItems, 0);
		default:
			{
				char String[5];
				_cmsTagSignature2String(String, (cmsTagSignature)BaseType);
				cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown curve type '%s'", String);
			}
		    return NULL;
	}
}

// Read a set of curves from specific offset
static cmsStage * ReadSetOfCurves(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 Offset, uint32 nCurves)
{
	cmsToneCurve * Curves[cmsMAXCHANNELS];
	uint32 i;
	cmsStage * Lin = NULL;
	if(nCurves > cmsMAXCHANNELS) return FALSE;
	if(!io->Seek(io, Offset)) return FALSE;
	for(i = 0; i < nCurves; i++)
		Curves[i] = NULL;
	for(i = 0; i < nCurves; i++) {
		Curves[i] = ReadEmbeddedCurve(self, io);
		if(Curves[i] == NULL) goto Error;
		if(!_cmsReadAlignment(io)) goto Error;
	}
	Lin = cmsStageAllocToneCurves(self->ContextID, nCurves, Curves);
Error:
	for(i = 0; i < nCurves; i++)
		cmsFreeToneCurve(Curves[i]);

	return Lin;
}

// LutAtoB type

// This structure represents a colour transform. The type contains up to five processing
// elements which are stored in the AtoBTag tag in the following order: a set of one
// dimensional curves, a 3 by 3 matrix with offset terms, a set of one dimensional curves,
// a multidimensional lookup table, and a set of one dimensional output curves.
// Data are processed using these elements via the following sequence:
//
//("A" curves) -> (multidimensional lookup table - CLUT) -> ("M" curves) -> (matrix) -> ("B" curves).
//
/*
   It is possible to use any or all of these processing elements. At least one processing element
   must be included.Only the following combinations are allowed:

   B
   M - Matrix - B
   A - CLUT - B
   A - CLUT - M - Matrix - B

 */

static void * Type_LUTA2B_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint32 BaseOffset;
	uint8 inputChan;        // Number of input channels
	uint8 outputChan;       // Number of output channels
	uint32 offsetB;         // Offset to first "B" curve
	uint32 offsetMat;       // Offset to matrix
	uint32 offsetM;         // Offset to first "M" curve
	uint32 offsetC;         // Offset to CLUT
	uint32 offsetA;         // Offset to first "A" curve
	cmsPipeline * NewLUT = NULL;
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	if(!_cmsReadUInt8Number(io, &inputChan)) return NULL;
	if(!_cmsReadUInt8Number(io, &outputChan)) return NULL;
	if(!_cmsReadUInt16Number(io, NULL)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetB)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetMat)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetM)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetC)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetA)) return NULL;

	if(inputChan == 0 || inputChan >= cmsMAXCHANNELS) return NULL;
	if(outputChan == 0 || outputChan >= cmsMAXCHANNELS) return NULL;

	// Allocates an empty LUT
	NewLUT = cmsPipelineAlloc(self->ContextID, inputChan, outputChan);
	if(NewLUT == NULL) return NULL;

	if(offsetA!= 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetA, inputChan)))
			goto Error;
	}

	if(offsetC != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadCLUT(self, io, BaseOffset + offsetC, inputChan, outputChan)))
			goto Error;
	}

	if(offsetM != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetM, outputChan)))
			goto Error;
	}

	if(offsetMat != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadMatrix(self, io, BaseOffset + offsetMat)))
			goto Error;
	}

	if(offsetB != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetB, outputChan)))
			goto Error;
	}

	*nItems = 1;
	return NewLUT;
Error:
	cmsPipelineFree(NewLUT);
	return NULL;

	CXX_UNUSED(SizeOfTag);
}

// Write a set of curves
static boolint WriteMatrix(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsStage * mpe)
{
	uint32 i, n;
	_cmsStageMatrixData* m = (_cmsStageMatrixData*)mpe->Data;
	n = mpe->InputChannels * mpe->OutputChannels;
	// Write the Matrix
	for(i = 0; i < n; i++) {
		if(!_cmsWrite15Fixed16Number(io, m->Double[i])) return FALSE;
	}
	if(m->Offset) {
		for(i = 0; i < mpe->OutputChannels; i++) {
			if(!_cmsWrite15Fixed16Number(io, m->Offset[i])) return FALSE;
		}
	}
	else {
		for(i = 0; i < mpe->OutputChannels; i++) {
			if(!_cmsWrite15Fixed16Number(io, 0)) return FALSE;
		}
	}

	return TRUE;

	CXX_UNUSED(self);
}

// Write a set of curves
static boolint WriteSetOfCurves(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsTagTypeSignature Type, cmsStage * mpe)
{
	uint32 i, n;
	cmsTagTypeSignature CurrentType;
	cmsToneCurve ** Curves;
	n      = cmsStageOutputChannels(mpe);
	Curves = _cmsStageGetPtrToCurveSet(mpe);
	for(i = 0; i < n; i++) {
		// If this is a table-based curve, use curve type even on V4
		CurrentType = Type;

		if((Curves[i]->nSegments == 0)||
		    ((Curves[i]->nSegments == 2) && (Curves[i]->Segments[1].Type == 0)) )
			CurrentType = cmsSigCurveType;
		else if(Curves[i]->Segments[0].Type < 0)
			CurrentType = cmsSigCurveType;

		if(!_cmsWriteTypeBase(io, CurrentType)) return FALSE;

		switch(CurrentType) {
			case cmsSigCurveType:
			    if(!Type_Curve_Write(self, io, Curves[i], 1)) return FALSE;
			    break;

			case cmsSigParametricCurveType:
			    if(!Type_ParametricCurve_Write(self, io, Curves[i], 1)) return FALSE;
			    break;

			default:
		    {
			    char String[5];

			    _cmsTagSignature2String(String, (cmsTagSignature)Type);
			    cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown curve type '%s'", String);
		    }
			    return FALSE;
		}

		if(!_cmsWriteAlignment(io)) return FALSE;
	}

	return TRUE;
}

static boolint WriteCLUT(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint8 Precision, cmsStage * mpe)
{
	uint8 gridPoints[cmsMAXCHANNELS]; // Number of grid points in each dimension.
	uint32 i;
	_cmsStageCLutData* CLUT = (_cmsStageCLutData*)mpe->Data;
	if(CLUT->HasFloatValues) {
		cmsSignalError(self->ContextID, cmsERROR_NOT_SUITABLE, "Cannot save floating point data, CLUT are 8 or 16 bit only");
		return FALSE;
	}
	memzero(gridPoints, sizeof(gridPoints));
	for(i = 0; i < (uint32)CLUT->Params->nInputs; i++)
		gridPoints[i] = (uint8)CLUT->Params->nSamples[i];

	if(!io->Write(io, cmsMAXCHANNELS*sizeof(uint8), gridPoints)) return FALSE;

	if(!_cmsWriteUInt8Number(io, (uint8)Precision)) return FALSE;
	if(!_cmsWriteUInt8Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt8Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt8Number(io, 0)) return FALSE;

	// Precision can be 1 or 2 bytes
	if(Precision == 1) {
		for(i = 0; i < CLUT->nEntries; i++) {
			if(!_cmsWriteUInt8Number(io, FROM_16_TO_8(CLUT->Tab.T[i]))) return FALSE;
		}
	}
	else if(Precision == 2) {
		if(!_cmsWriteUInt16Array(io, CLUT->nEntries, CLUT->Tab.T)) return FALSE;
	}
	else {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown precision of '%d'", Precision);
		return FALSE;
	}

	if(!_cmsWriteAlignment(io)) return FALSE;

	return TRUE;
}

static boolint Type_LUTA2B_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsPipeline * Lut = (cmsPipeline *)Ptr;
	uint32 inputChan, outputChan;
	cmsStage * A = NULL, * B = NULL, * M = NULL;
	cmsStage * Matrix = NULL;
	cmsStage * CLUT = NULL;
	uint32 offsetB = 0, offsetMat = 0, offsetM = 0, offsetC = 0, offsetA = 0;
	uint32 BaseOffset, DirectoryPos, CurrentPos;
	// Get the base for all offsets
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	if(Lut->Elements)
		if(!cmsPipelineCheckAndRetreiveStages(Lut, 1, cmsSigCurveSetElemType, &B))
			if(!cmsPipelineCheckAndRetreiveStages(Lut, 3, cmsSigCurveSetElemType, cmsSigMatrixElemType, cmsSigCurveSetElemType,
			    &M, &Matrix, &B))
				if(!cmsPipelineCheckAndRetreiveStages(Lut, 3, cmsSigCurveSetElemType, cmsSigCLutElemType,
				    cmsSigCurveSetElemType, &A, &CLUT, &B))
					if(!cmsPipelineCheckAndRetreiveStages(Lut, 5, cmsSigCurveSetElemType, cmsSigCLutElemType,
					    cmsSigCurveSetElemType,
					    cmsSigMatrixElemType, cmsSigCurveSetElemType, &A, &CLUT, &M, &Matrix, &B)) {
						cmsSignalError(self->ContextID,
						    cmsERROR_NOT_SUITABLE,
						    "LUT is not suitable to be saved as LutAToB");
						return FALSE;
					}

	// Get input, output channels
	inputChan  = cmsPipelineInputChannels(Lut);
	outputChan = cmsPipelineOutputChannels(Lut);

	// Write channel count
	if(!_cmsWriteUInt8Number(io, (uint8)inputChan)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)outputChan)) return FALSE;
	if(!_cmsWriteUInt16Number(io, 0)) return FALSE;

	// Keep directory to be filled latter
	DirectoryPos = io->Tell(io);

	// Write the directory
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;

	if(A) {
		offsetA = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, A)) return FALSE;
	}

	if(CLUT) {
		offsetC = io->Tell(io) - BaseOffset;
		if(!WriteCLUT(self, io, (Lut->SaveAs8Bits ? 1U : 2U), CLUT)) return FALSE;
	}
	if(M) {
		offsetM = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, M)) return FALSE;
	}

	if(Matrix) {
		offsetMat = io->Tell(io) - BaseOffset;
		if(!WriteMatrix(self, io, Matrix)) return FALSE;
	}

	if(B) {
		offsetB = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, B)) return FALSE;
	}

	CurrentPos = io->Tell(io);

	if(!io->Seek(io, DirectoryPos)) return FALSE;

	if(!_cmsWriteUInt32Number(io, offsetB)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetMat)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetM)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetC)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetA)) return FALSE;

	if(!io->Seek(io, CurrentPos)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_LUTA2B_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsPipelineDup((cmsPipeline *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_LUTA2B_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsPipelineFree((cmsPipeline *)Ptr);
	return;
	CXX_UNUSED(self);
}

// LutBToA type

static void * Type_LUTB2A_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint8 inputChan;        // Number of input channels
	uint8 outputChan;       // Number of output channels
	uint32 BaseOffset;      // Actual position in file
	uint32 offsetB;         // Offset to first "B" curve
	uint32 offsetMat;       // Offset to matrix
	uint32 offsetM;         // Offset to first "M" curve
	uint32 offsetC;         // Offset to CLUT
	uint32 offsetA;         // Offset to first "A" curve
	cmsPipeline * NewLUT = NULL;
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	if(!_cmsReadUInt8Number(io, &inputChan)) return NULL;
	if(!_cmsReadUInt8Number(io, &outputChan)) return NULL;
	if(inputChan == 0 || inputChan >= cmsMAXCHANNELS) return NULL;
	if(outputChan == 0 || outputChan >= cmsMAXCHANNELS) return NULL;
	// Padding
	if(!_cmsReadUInt16Number(io, NULL)) return NULL;

	if(!_cmsReadUInt32Number(io, &offsetB)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetMat)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetM)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetC)) return NULL;
	if(!_cmsReadUInt32Number(io, &offsetA)) return NULL;

	// Allocates an empty LUT
	NewLUT = cmsPipelineAlloc(self->ContextID, inputChan, outputChan);
	if(NewLUT == NULL) return NULL;

	if(offsetB != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetB, inputChan)))
			goto Error;
	}

	if(offsetMat != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadMatrix(self, io, BaseOffset + offsetMat)))
			goto Error;
	}

	if(offsetM != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetM, inputChan)))
			goto Error;
	}

	if(offsetC != 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadCLUT(self, io, BaseOffset + offsetC, inputChan, outputChan)))
			goto Error;
	}

	if(offsetA!= 0) {
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, ReadSetOfCurves(self, io, BaseOffset + offsetA, outputChan)))
			goto Error;
	}

	*nItems = 1;
	return NewLUT;
Error:
	cmsPipelineFree(NewLUT);
	return NULL;

	CXX_UNUSED(SizeOfTag);
}

/*
   B
   B - Matrix - M
   B - CLUT - A
   B - Matrix - M - CLUT - A
 */

static boolint Type_LUTB2A_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsPipeline * Lut = (cmsPipeline *)Ptr;
	uint32 inputChan, outputChan;
	cmsStage * A = NULL, * B = NULL, * M = NULL;
	cmsStage * Matrix = NULL;
	cmsStage * CLUT = NULL;
	uint32 offsetB = 0, offsetMat = 0, offsetM = 0, offsetC = 0, offsetA = 0;
	uint32 BaseOffset, DirectoryPos, CurrentPos;
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	if(!cmsPipelineCheckAndRetreiveStages(Lut, 1, cmsSigCurveSetElemType, &B))
		if(!cmsPipelineCheckAndRetreiveStages(Lut, 3, cmsSigCurveSetElemType, cmsSigMatrixElemType, cmsSigCurveSetElemType, &B,
		    &Matrix, &M))
			if(!cmsPipelineCheckAndRetreiveStages(Lut, 3, cmsSigCurveSetElemType, cmsSigCLutElemType, cmsSigCurveSetElemType,
			    &B, &CLUT, &A))
				if(!cmsPipelineCheckAndRetreiveStages(Lut, 5, cmsSigCurveSetElemType, cmsSigMatrixElemType,
				    cmsSigCurveSetElemType,
				    cmsSigCLutElemType, cmsSigCurveSetElemType, &B, &Matrix, &M, &CLUT, &A)) {
					cmsSignalError(self->ContextID, cmsERROR_NOT_SUITABLE,
					    "LUT is not suitable to be saved as LutBToA");
					return FALSE;
				}

	inputChan  = cmsPipelineInputChannels(Lut);
	outputChan = cmsPipelineOutputChannels(Lut);

	if(!_cmsWriteUInt8Number(io, (uint8)inputChan)) return FALSE;
	if(!_cmsWriteUInt8Number(io, (uint8)outputChan)) return FALSE;
	if(!_cmsWriteUInt16Number(io, 0)) return FALSE;

	DirectoryPos = io->Tell(io);

	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;

	if(A) {
		offsetA = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, A)) return FALSE;
	}

	if(CLUT) {
		offsetC = io->Tell(io) - BaseOffset;
		if(!WriteCLUT(self, io, (Lut->SaveAs8Bits ? 1U : 2U), CLUT)) return FALSE;
	}
	if(M) {
		offsetM = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, M)) return FALSE;
	}
	if(Matrix) {
		offsetMat = io->Tell(io) - BaseOffset;
		if(!WriteMatrix(self, io, Matrix)) return FALSE;
	}
	if(B) {
		offsetB = io->Tell(io) - BaseOffset;
		if(!WriteSetOfCurves(self, io, cmsSigParametricCurveType, B)) return FALSE;
	}

	CurrentPos = io->Tell(io);

	if(!io->Seek(io, DirectoryPos)) return FALSE;

	if(!_cmsWriteUInt32Number(io, offsetB)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetMat)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetM)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetC)) return FALSE;
	if(!_cmsWriteUInt32Number(io, offsetA)) return FALSE;

	if(!io->Seek(io, CurrentPos)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_LUTB2A_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsPipelineDup((cmsPipeline *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_LUTB2A_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsPipelineFree((cmsPipeline *)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigColorantTableType
//
/*
   The purpose of this tag is to identify the colorants used in the profile by a
   unique name and set of XYZ or L*a*b* values to give the colorant an unambiguous
   value. The first colorant listed is the colorant of the first device channel of
   a lut tag. The second colorant listed is the colorant of the second device channel
   of a lut tag, and so on.
 */
static void * Type_ColorantTable_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint32 i, Count;
	cmsNAMEDCOLORLIST* List;
	char Name[34];
	uint16 PCS[3];
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	if(Count > cmsMAXCHANNELS) {
		cmsSignalError(self->ContextID, cmsERROR_RANGE, "Too many colorants '%d'", Count);
		return NULL;
	}
	List = cmsAllocNamedColorList(self->ContextID, Count, 0, "", "");
	for(i = 0; i < Count; i++) {
		if(io->Read(io, Name, 32, 1) != 1) goto Error;
		Name[32] = 0;
		if(!_cmsReadUInt16Array(io, 3, PCS)) goto Error;
		if(!cmsAppendNamedColor(List, Name, PCS, NULL)) goto Error;
	}
	*nItems = 1;
	return List;
Error:
	*nItems = 0;
	cmsFreeNamedColorList(List);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// Saves a colorant table. It is using the named color structure for simplicity sake
static boolint Type_ColorantTable_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsNAMEDCOLORLIST* NamedColorList = (cmsNAMEDCOLORLIST*)Ptr;
	uint32 i, nColors;
	nColors = cmsNamedColorCount(NamedColorList);
	if(!_cmsWriteUInt32Number(io, nColors)) return FALSE;
	for(i = 0; i < nColors; i++) {
		char root[cmsMAX_PATH];
		uint16 PCS[3];
		memzero(root, sizeof(root));
		if(!cmsNamedColorInfo(NamedColorList, i, root, NULL, NULL, PCS, NULL)) return 0;
		root[32] = 0;

		if(!io->Write(io, 32, root)) return FALSE;
		if(!_cmsWriteUInt16Array(io, 3, PCS)) return FALSE;
	}

	return TRUE;

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_ColorantTable_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	cmsNAMEDCOLORLIST* nc = (cmsNAMEDCOLORLIST*)Ptr;
	return (void *)cmsDupNamedColorList(nc);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_ColorantTable_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsFreeNamedColorList((cmsNAMEDCOLORLIST*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigNamedColor2Type
//
//The namedColor2Type is a count value and array of structures that provide color
//coordinates for 7-bit ASCII color names. For each named color, a PCS and optional
//device representation of the color are given. Both representations are 16-bit values.
//The device representation corresponds to the header's 'color space of data' field.
//This representation should be consistent with the 'number of device components'
//field in the namedColor2Type. If this field is 0, device coordinates are not provided.
//The PCS representation corresponds to the header's PCS field. The PCS representation
//is always provided. Color names are fixed-length, 32-byte fields including null
//termination. In order to maintain maximum portability, it is strongly recommended
//that special characters of the 7-bit ASCII set not be used.
//
static void * Type_NamedColor_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint32 vendorFlag;      // Bottom 16 bits for ICC use
	uint32 count;           // Count of named colors
	uint32 nDeviceCoords;   // Num of device coordinates
	char prefix[32];                 // Prefix for each color name
	char suffix[32];                 // Suffix for each color name
	cmsNAMEDCOLORLIST*   v;
	uint32 i;
	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &vendorFlag)) return NULL;
	if(!_cmsReadUInt32Number(io, &count)) return NULL;
	if(!_cmsReadUInt32Number(io, &nDeviceCoords)) return NULL;
	if(io->Read(io, prefix, 32, 1) != 1) return NULL;
	if(io->Read(io, suffix, 32, 1) != 1) return NULL;
	prefix[31] = suffix[31] = 0;
	v = cmsAllocNamedColorList(self->ContextID, count, nDeviceCoords, prefix, suffix);
	if(v == NULL) {
		cmsSignalError(self->ContextID, cmsERROR_RANGE, "Too many named colors '%d'", count);
		return NULL;
	}
	if(nDeviceCoords > cmsMAXCHANNELS) {
		cmsSignalError(self->ContextID, cmsERROR_RANGE, "Too many device coordinates '%d'", nDeviceCoords);
		goto Error;
	}
	for(i = 0; i < count; i++) {
		uint16 PCS[3];
		uint16 Colorant[cmsMAXCHANNELS];
		char Root[33];
		memzero(Colorant, sizeof(Colorant));
		if(io->Read(io, Root, 32, 1) != 1) goto Error;
		Root[32] = 0; // To prevent exploits
		if(!_cmsReadUInt16Array(io, 3, PCS)) goto Error;
		if(!_cmsReadUInt16Array(io, nDeviceCoords, Colorant)) goto Error;
		if(!cmsAppendNamedColor(v, Root, PCS, Colorant)) goto Error;
	}
	*nItems = 1;
	return (void *)v;
Error:
	cmsFreeNamedColorList(v);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// Saves a named color list into a named color profile
static boolint Type_NamedColor_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsNAMEDCOLORLIST* NamedColorList = (cmsNAMEDCOLORLIST*)Ptr;
	char prefix[32+1]; // Prefix for each color name
	char suffix[32+1]; // Suffix for each color name
	uint32 i, nColors;
	nColors = cmsNamedColorCount(NamedColorList);
	if(!_cmsWriteUInt32Number(io, 0)) return FALSE;
	if(!_cmsWriteUInt32Number(io, nColors)) return FALSE;
	if(!_cmsWriteUInt32Number(io, NamedColorList->ColorantCount)) return FALSE;
	strnzcpy(prefix, (const char *)NamedColorList->Prefix, sizeof(prefix));
	strnzcpy(suffix, (const char *)NamedColorList->Suffix, sizeof(suffix));
	if(!io->Write(io, 32, prefix)) return FALSE;
	if(!io->Write(io, 32, suffix)) return FALSE;
	for(i = 0; i < nColors; i++) {
		uint16 PCS[3];
		uint16 Colorant[cmsMAXCHANNELS];
		char Root[cmsMAX_PATH];
		if(!cmsNamedColorInfo(NamedColorList, i, Root, NULL, NULL, PCS, Colorant)) return 0;
		Root[32] = 0;
		if(!io->Write(io, 32, Root)) return FALSE;
		if(!_cmsWriteUInt16Array(io, 3, PCS)) return FALSE;
		if(!_cmsWriteUInt16Array(io, NamedColorList->ColorantCount, Colorant)) return FALSE;
	}
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_NamedColor_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	cmsNAMEDCOLORLIST* nc = (cmsNAMEDCOLORLIST*)Ptr;
	return (void *)cmsDupNamedColorList(nc);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_NamedColor_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsFreeNamedColorList((cmsNAMEDCOLORLIST*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigProfileSequenceDescType
//
// This type is an array of structures, each of which contains information from the
// header fields and tags from the original profiles which were combined to create
// the final profile. The order of the structures is the order in which the profiles
// were combined and includes a structure for the final profile. This provides a
// description of the profile sequence from source to destination,
// typically used with the DeviceLink profile.

static boolint ReadEmbeddedText(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsMLU** mlu, uint32 SizeOfTag)
{
	uint32 nItems;
	cmsTagTypeSignature BaseType = _cmsReadTypeBase(io);
	switch(BaseType) {
		case cmsSigTextType:
		    if(*mlu) 
				cmsMLUfree(*mlu);
		    *mlu = (cmsMLU*)Type_Text_Read(self, io, &nItems, SizeOfTag);
		    return (*mlu != NULL);
		case cmsSigTextDescriptionType:
		    if(*mlu) 
				cmsMLUfree(*mlu);
		    *mlu =  (cmsMLU*)Type_Text_Description_Read(self, io, &nItems, SizeOfTag);
		    return (*mlu != NULL);

		/*
		   TBD: Size is needed for MLU, and we have no idea on which is the available size
		 */

		case cmsSigMultiLocalizedUnicodeType:
		    if(*mlu) cmsMLUfree(*mlu);
		    *mlu =  (cmsMLU*)Type_MLU_Read(self, io, &nItems, SizeOfTag);
		    return (*mlu != NULL);

		default: return FALSE;
	}
}

static void * Type_ProfileSequenceDesc_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsSEQ* OutSeq;
	uint32 i, Count;
	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	if(SizeOfTag < sizeof(uint32)) return NULL;
	SizeOfTag -= sizeof(uint32);
	OutSeq = cmsAllocProfileSequenceDescription(self->ContextID, Count);
	if(OutSeq == NULL) return NULL;
	OutSeq->n = Count;
	// Get structures as well
	for(i = 0; i < Count; i++) {
		cmsPSEQDESC* sec = &OutSeq->seq[i];
		if(!_cmsReadUInt32Number(io, &sec->deviceMfg)) goto Error;
		if(SizeOfTag < sizeof(uint32)) goto Error;
		SizeOfTag -= sizeof(uint32);
		if(!_cmsReadUInt32Number(io, &sec->deviceModel)) goto Error;
		if(SizeOfTag < sizeof(uint32)) goto Error;
		SizeOfTag -= sizeof(uint32);
		if(!_cmsReadUInt64Number(io, &sec->attributes)) goto Error;
		if(SizeOfTag < sizeof(uint64)) goto Error;
		SizeOfTag -= sizeof(uint64);
		if(!_cmsReadUInt32Number(io, (uint32 *)&sec->technology)) goto Error;
		if(SizeOfTag < sizeof(uint32)) goto Error;
		SizeOfTag -= sizeof(uint32);
		if(!ReadEmbeddedText(self, io, &sec->Manufacturer, SizeOfTag)) goto Error;
		if(!ReadEmbeddedText(self, io, &sec->Model, SizeOfTag)) goto Error;
	}
	*nItems = 1;
	return OutSeq;
Error:
	cmsFreeProfileSequenceDescription(OutSeq);
	return NULL;
}

// Aux--Embed a text description type. It can be of type text description or multilocalized unicode
// and it depends of the version number passed on cmsTagDescriptor structure instead of stack
static boolint SaveDescription(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsMLU* Text)
{
	if(self->ICCVersion < 0x4000000) {
		if(!_cmsWriteTypeBase(io, cmsSigTextDescriptionType)) return FALSE;
		return Type_Text_Description_Write(self, io, Text, 1);
	}
	else {
		if(!_cmsWriteTypeBase(io, cmsSigMultiLocalizedUnicodeType)) return FALSE;
		return Type_MLU_Write(self, io, Text, 1);
	}
}

static boolint Type_ProfileSequenceDesc_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsSEQ* Seq = (cmsSEQ*)Ptr;
	uint32 i;
	if(!_cmsWriteUInt32Number(io, Seq->n)) return FALSE;
	for(i = 0; i < Seq->n; i++) {
		cmsPSEQDESC* sec = &Seq->seq[i];
		if(!_cmsWriteUInt32Number(io, sec->deviceMfg)) return FALSE;
		if(!_cmsWriteUInt32Number(io, sec->deviceModel)) return FALSE;
		if(!_cmsWriteUInt64Number(io, &sec->attributes)) return FALSE;
		if(!_cmsWriteUInt32Number(io, sec->technology)) return FALSE;
		if(!SaveDescription(self, io, sec->Manufacturer)) return FALSE;
		if(!SaveDescription(self, io, sec->Model)) return FALSE;
	}
	return TRUE;
	CXX_UNUSED(nItems);
}

static void * Type_ProfileSequenceDesc_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsDupProfileSequenceDescription((cmsSEQ*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_ProfileSequenceDesc_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsFreeProfileSequenceDescription((cmsSEQ*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigProfileSequenceIdType
//
/*
   In certain workflows using ICC Device Link Profiles, it is necessary to identify the
   original profiles that were combined to create the Device Link Profile.
   This type is an array of structures, each of which contains information for
   identification of a profile used in a sequence
 */

static boolint ReadSeqID(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag)
{
	cmsSEQ* OutSeq = (cmsSEQ*)Cargo;
	cmsPSEQDESC* seq = &OutSeq->seq[n];
	if(io->Read(io, seq->ProfileID.ID8, 16, 1) != 1) return FALSE;
	if(!ReadEmbeddedText(self, io, &seq->Description, SizeOfTag)) return FALSE;
	return TRUE;
}

static void * Type_ProfileSequenceId_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsSEQ* OutSeq;
	uint32 Count;
	uint32 BaseOffset;
	*nItems = 0;
	// Get actual position as a basis for element offsets
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// Get table count
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	SizeOfTag -= sizeof(uint32);
	// Allocate an empty structure
	OutSeq = cmsAllocProfileSequenceDescription(self->ContextID, Count);
	if(OutSeq == NULL) return NULL;
	// Read the position table
	if(!ReadPositionTable(self, io, Count, BaseOffset, OutSeq, ReadSeqID)) {
		cmsFreeProfileSequenceDescription(OutSeq);
		return NULL;
	}
	// Success
	*nItems = 1;
	return OutSeq;
}

static boolint WriteSeqID(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag)
{
	cmsSEQ* Seq = (cmsSEQ*)Cargo;
	if(!io->Write(io, 16, Seq->seq[n].ProfileID.ID8)) return FALSE;
	// Store here the MLU
	if(!SaveDescription(self, io, Seq->seq[n].Description)) return FALSE;
	return TRUE;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_ProfileSequenceId_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsSEQ* Seq = (cmsSEQ*)Ptr;
	// Keep the base offset
	uint32 BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// This is the table count
	if(!_cmsWriteUInt32Number(io, Seq->n)) return FALSE;
	// This is the position table and content
	if(!WritePositionTable(self, io, 0, Seq->n, BaseOffset, Seq, WriteSeqID)) return FALSE;
	return TRUE;
	CXX_UNUSED(nItems);
}

static void * Type_ProfileSequenceId_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsDupProfileSequenceDescription((cmsSEQ*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_ProfileSequenceId_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsFreeProfileSequenceDescription((cmsSEQ*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigUcrBgType
//
/*
   This type contains curves representing the under color removal and black
   generation and a text string which is a general description of the method used
   for the ucr/bg.
 */
static void * Type_UcrBg_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsUcrBg* n = (cmsUcrBg*)_cmsMallocZero(self->ContextID, sizeof(cmsUcrBg));
	uint32 CountUcr, CountBg;
	char * ASCIIString;
	*nItems = 0;
	if(!n) return NULL;
	// First curve is Under color removal
	if(!_cmsReadUInt32Number(io, &CountUcr)) return NULL;
	if(SizeOfTag < sizeof(uint32)) return NULL;
	SizeOfTag -= sizeof(uint32);

	n->Ucr = cmsBuildTabulatedToneCurve16(self->ContextID, CountUcr, NULL);
	if(n->Ucr == NULL) return NULL;

	if(!_cmsReadUInt16Array(io, CountUcr, n->Ucr->Table16)) return NULL;
	if(SizeOfTag < sizeof(uint32)) return NULL;
	SizeOfTag -= CountUcr * sizeof(uint16);

	// Second curve is Black generation
	if(!_cmsReadUInt32Number(io, &CountBg)) return NULL;
	if(SizeOfTag < sizeof(uint32)) return NULL;
	SizeOfTag -= sizeof(uint32);

	n->Bg = cmsBuildTabulatedToneCurve16(self->ContextID, CountBg, NULL);
	if(n->Bg == NULL) return NULL;
	if(!_cmsReadUInt16Array(io, CountBg, n->Bg->Table16)) return NULL;
	if(SizeOfTag < CountBg * sizeof(uint16)) return NULL;
	SizeOfTag -= CountBg * sizeof(uint16);
	if(SizeOfTag == UINT_MAX) return NULL;

	// Now comes the text. The length is specified by the tag size
	n->Desc = cmsMLUalloc(self->ContextID, 1);
	if(n->Desc == NULL) return NULL;

	ASCIIString = (char *)_cmsMalloc(self->ContextID, SizeOfTag + 1);
	if(io->Read(io, ASCIIString, sizeof(char), SizeOfTag) != SizeOfTag) return NULL;
	ASCIIString[SizeOfTag] = 0;
	cmsMLUsetASCII(n->Desc, cmsNoLanguage, cmsNoCountry, ASCIIString);
	_cmsFree(self->ContextID, ASCIIString);

	*nItems = 1;
	return (void *)n;
}

static boolint Type_UcrBg_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsUcrBg* Value = (cmsUcrBg*)Ptr;
	uint32 TextSize;
	char * Text;
	// First curve is Under color removal
	if(!_cmsWriteUInt32Number(io, Value->Ucr->nEntries)) return FALSE;
	if(!_cmsWriteUInt16Array(io, Value->Ucr->nEntries, Value->Ucr->Table16)) return FALSE;

	// Then black generation
	if(!_cmsWriteUInt32Number(io, Value->Bg->nEntries)) return FALSE;
	if(!_cmsWriteUInt16Array(io, Value->Bg->nEntries, Value->Bg->Table16)) return FALSE;

	// Now comes the text. The length is specified by the tag size
	TextSize = cmsMLUgetASCII(Value->Desc, cmsNoLanguage, cmsNoCountry, NULL, 0);
	Text     = (char *)_cmsMalloc(self->ContextID, TextSize);
	if(cmsMLUgetASCII(Value->Desc, cmsNoLanguage, cmsNoCountry, Text, TextSize) != TextSize) return FALSE;

	if(!io->Write(io, TextSize, Text)) return FALSE;
	_cmsFree(self->ContextID, Text);

	return TRUE;

	CXX_UNUSED(nItems);
}

static void * Type_UcrBg_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	cmsUcrBg* Src = (cmsUcrBg*)Ptr;
	cmsUcrBg* NewUcrBg = (cmsUcrBg*)_cmsMallocZero(self->ContextID, sizeof(cmsUcrBg));
	if(NewUcrBg == NULL) return NULL;
	NewUcrBg->Bg   = cmsDupToneCurve(Src->Bg);
	NewUcrBg->Ucr  = cmsDupToneCurve(Src->Ucr);
	NewUcrBg->Desc = cmsMLUdup(Src->Desc);
	return (void *)NewUcrBg;
	CXX_UNUSED(n);
}

static void Type_UcrBg_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	if(Ptr) {
		cmsUcrBg * Src = (cmsUcrBg*)Ptr;
		cmsFreeToneCurve(Src->Ucr);
		cmsFreeToneCurve(Src->Bg);
		cmsMLUfree(Src->Desc);
		_cmsFree(self->ContextID, Ptr);
	}
}
//
// Type cmsSigCrdInfoType
//
/*
   This type contains the PostScript product name to which this profile corresponds
   and the names of the companion CRDs. Recall that a single profile can generate
   multiple CRDs. It is implemented as a MLU being the language code "PS" and then
   country varies for each element:

                nm: PostScript product name
 #0: Rendering intent 0 CRD name
 #1: Rendering intent 1 CRD name
 #2: Rendering intent 2 CRD name
 #3: Rendering intent 3 CRD name
 */

// Auxiliary, read an string specified as count + string
static boolint ReadCountAndSting(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsMLU* mlu, uint32 * SizeOfTag, const char * Section)
{
	uint32 Count;
	char * Text;
	if(*SizeOfTag < sizeof(uint32)) return FALSE;
	if(!_cmsReadUInt32Number(io, &Count)) return FALSE;
	if(Count > UINT_MAX - sizeof(uint32)) return FALSE;
	if(*SizeOfTag < Count + sizeof(uint32)) return FALSE;
	Text     = (char *)_cmsMalloc(self->ContextID, Count+1);
	if(Text == NULL) return FALSE;
	if(io->Read(io, Text, sizeof(uint8), Count) != Count) {
		_cmsFree(self->ContextID, Text);
		return FALSE;
	}
	Text[Count] = 0;
	cmsMLUsetASCII(mlu, "PS", Section, Text);
	_cmsFree(self->ContextID, Text);
	*SizeOfTag -= (Count + sizeof(uint32));
	return TRUE;
}

static boolint WriteCountAndSting(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, cmsMLU* mlu, const char * Section)
{
	uint32 TextSize = cmsMLUgetASCII(mlu, "PS", Section, NULL, 0);
	char * Text     = (char *)_cmsMalloc(self->ContextID, TextSize);
	if(!_cmsWriteUInt32Number(io, TextSize)) return FALSE;
	if(cmsMLUgetASCII(mlu, "PS", Section, Text, TextSize) == 0) return FALSE;
	if(!io->Write(io, TextSize, Text)) return FALSE;
	_cmsFree(self->ContextID, Text);
	return TRUE;
}

static void * Type_CrdInfo_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsMLU* mlu = cmsMLUalloc(self->ContextID, 5);
	*nItems = 0;
	if(!ReadCountAndSting(self, io, mlu, &SizeOfTag, "nm")) goto Error;
	if(!ReadCountAndSting(self, io, mlu, &SizeOfTag, "#0")) goto Error;
	if(!ReadCountAndSting(self, io, mlu, &SizeOfTag, "#1")) goto Error;
	if(!ReadCountAndSting(self, io, mlu, &SizeOfTag, "#2")) goto Error;
	if(!ReadCountAndSting(self, io, mlu, &SizeOfTag, "#3")) goto Error;

	*nItems = 1;
	return (void *)mlu;

Error:
	cmsMLUfree(mlu);
	return NULL;
}

static boolint Type_CrdInfo_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsMLU* mlu = (cmsMLU*)Ptr;
	if(!WriteCountAndSting(self, io, mlu, "nm")) goto Error;
	if(!WriteCountAndSting(self, io, mlu, "#0")) goto Error;
	if(!WriteCountAndSting(self, io, mlu, "#1")) goto Error;
	if(!WriteCountAndSting(self, io, mlu, "#2")) goto Error;
	if(!WriteCountAndSting(self, io, mlu, "#3")) goto Error;
	return TRUE;

Error:
	return FALSE;
	CXX_UNUSED(nItems);
}

static void * Type_CrdInfo_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsMLUdup((cmsMLU*)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_CrdInfo_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsMLUfree((cmsMLU*)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigScreeningType
//
//The screeningType describes various screening parameters including screen
//frequency, screening angle, and spot shape.
//
static void * Type_Screening_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsScreening* sc = NULL;
	uint32 i;
	sc = (cmsScreening*)_cmsMallocZero(self->ContextID, sizeof(cmsScreening));
	if(!sc) return NULL;
	*nItems = 0;
	if(!_cmsReadUInt32Number(io, &sc->Flag)) goto Error;
	if(!_cmsReadUInt32Number(io, &sc->nChannels)) goto Error;

	if(sc->nChannels > cmsMAXCHANNELS - 1)
		sc->nChannels = cmsMAXCHANNELS - 1;

	for(i = 0; i < sc->nChannels; i++) {
		if(!_cmsRead15Fixed16Number(io, &sc->Channels[i].Frequency)) goto Error;
		if(!_cmsRead15Fixed16Number(io, &sc->Channels[i].ScreenAngle)) goto Error;
		if(!_cmsReadUInt32Number(io, &sc->Channels[i].SpotShape)) goto Error;
	}
	*nItems = 1;
	return (void *)sc;
Error:
	_cmsFree(self->ContextID, sc);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_Screening_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsScreening* sc = (cmsScreening*)Ptr;
	uint32 i;
	if(!_cmsWriteUInt32Number(io, sc->Flag)) 
		return FALSE;
	if(!_cmsWriteUInt32Number(io, sc->nChannels)) 
		return FALSE;
	for(i = 0; i < sc->nChannels; i++) {
		if(!_cmsWrite15Fixed16Number(io, sc->Channels[i].Frequency)) return FALSE;
		if(!_cmsWrite15Fixed16Number(io, sc->Channels[i].ScreenAngle)) return FALSE;
		if(!_cmsWriteUInt32Number(io, sc->Channels[i].SpotShape)) return FALSE;
	}
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_Screening_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsScreening));
	CXX_UNUSED(n);
}

static void Type_Screening_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigViewingConditionsType
//
//
//This type represents a set of viewing condition parameters including:
//CIE 'absolute' illuminant white point tristimulus values and CIE 'absolute'
//surround tristimulus values.
//
static void * Type_ViewingConditions_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsICCViewingConditions* vc = NULL;
	vc = (cmsICCViewingConditions*)_cmsMallocZero(self->ContextID, sizeof(cmsICCViewingConditions));
	if(vc == NULL) return NULL;
	*nItems = 0;
	if(!_cmsReadXYZNumber(io, &vc->IlluminantXYZ)) goto Error;
	if(!_cmsReadXYZNumber(io, &vc->SurroundXYZ)) goto Error;
	if(!_cmsReadUInt32Number(io, &vc->IlluminantType)) goto Error;
	*nItems = 1;
	return (void *)vc;
Error:
	if(vc)
		_cmsFree(self->ContextID, vc);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

static boolint Type_ViewingConditions_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsICCViewingConditions* sc = (cmsICCViewingConditions*)Ptr;
	if(!_cmsWriteXYZNumber(io, &sc->IlluminantXYZ)) return FALSE;
	if(!_cmsWriteXYZNumber(io, &sc->SurroundXYZ)) return FALSE;
	if(!_cmsWriteUInt32Number(io, sc->IlluminantType)) return FALSE;
	return TRUE;
	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_ViewingConditions_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return _cmsDupMem(self->ContextID, Ptr, sizeof(cmsICCViewingConditions));
	CXX_UNUSED(n);
}

static void Type_ViewingConditions_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigMultiProcessElementType
//
static void * GenericMPEdup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsStageDup((cmsStage *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void GenericMPEfree(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsStageFree((cmsStage *)Ptr);
	return;
	CXX_UNUSED(self);
}

// Each curve is stored in one or more curve segments, with break-points specified between curve segments.
// The first curve segment always starts at -Infinity, and the last curve segment always ends at +Infinity. The
// first and last curve segments shall be specified in terms of a formula, whereas the other segments shall be
// specified either in terms of a formula, or by a sampled curve.

// Read an embedded segmented curve
static cmsToneCurve * ReadSegmentedCurve(struct _cms_typehandler_struct* self, cmsIOHANDLER* io)
{
	cmsCurveSegSignature ElementSig;
	uint32 i, j;
	uint16 nSegments;
	cmsCurveSegment*  Segments;
	cmsToneCurve * Curve;
	float PrevBreak = MINUS_INF; // - infinite
	// Take signature and channels for each element.
	if(!_cmsReadUInt32Number(io, (uint32 *)&ElementSig)) return NULL;
	// That should be a segmented curve
	if(ElementSig != cmsSigSegmentedCurve) return NULL;
	if(!_cmsReadUInt32Number(io, NULL)) return NULL;
	if(!_cmsReadUInt16Number(io, &nSegments)) return NULL;
	if(!_cmsReadUInt16Number(io, NULL)) return NULL;
	if(nSegments < 1) 
		return NULL;
	Segments = (cmsCurveSegment*)_cmsCalloc(self->ContextID, nSegments, sizeof(cmsCurveSegment));
	if(Segments == NULL) 
		return NULL;

	// Read breakpoints
	for(i = 0; i < (uint32)nSegments - 1; i++) {
		Segments[i].x0 = PrevBreak;
		if(!_cmsReadFloat32Number(io, &Segments[i].x1)) 
			goto Error;
		PrevBreak = Segments[i].x1;
	}
	Segments[nSegments-1].x0 = PrevBreak;
	Segments[nSegments-1].x1 = PLUS_INF;  // A big float number
	// Read segments
	for(i = 0; i < nSegments; i++) {
		if(!_cmsReadUInt32Number(io, (uint32 *)&ElementSig)) 
			goto Error;
		if(!_cmsReadUInt32Number(io, NULL)) 
			goto Error;
		switch(ElementSig) {
			case cmsSigFormulaCurveSeg: {
			    uint16 Type;
			    uint32 ParamsByType[] = {4, 5, 5 };
			    if(!_cmsReadUInt16Number(io, &Type)) 
					goto Error;
			    if(!_cmsReadUInt16Number(io, NULL)) 
					goto Error;
			    Segments[i].Type = Type + 6;
			    if(Type > 2) 
					goto Error;
			    for(j = 0; j < ParamsByType[Type]; j++) {
				    float f;
				    if(!_cmsReadFloat32Number(io, &f)) 
						goto Error;
				    Segments[i].Params[j] = f;
			    }
		    }
		    break;
			case cmsSigSampledCurveSeg: {
			    uint32 Count;
			    if(!_cmsReadUInt32Number(io, &Count)) 
					goto Error;
			    Segments[i].nGridPoints = Count;
			    Segments[i].SampledPoints = (float *)_cmsCalloc(self->ContextID, Count, sizeof(float));
			    if(Segments[i].SampledPoints == NULL) goto Error;

			    for(j = 0; j < Count; j++) {
				    if(!_cmsReadFloat32Number(io, &Segments[i].SampledPoints[j])) goto Error;
			    }
		    }
		    break;
			default:
		    {
			    char String[5];
			    _cmsTagSignature2String(String, (cmsTagSignature)ElementSig);
			    cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown curve element type '%s' found.", String);
		    }
			    goto Error;
		}
	}
	Curve = cmsBuildSegmentedToneCurve(self->ContextID, nSegments, Segments);
	for(i = 0; i < nSegments; i++) {
		if(Segments[i].SampledPoints) _cmsFree(self->ContextID, Segments[i].SampledPoints);
	}
	_cmsFree(self->ContextID, Segments);
	return Curve;
Error:
	if(Segments) {
		for(i = 0; i < nSegments; i++) {
			if(Segments[i].SampledPoints) _cmsFree(self->ContextID, Segments[i].SampledPoints);
		}
		_cmsFree(self->ContextID, Segments);
	}
	return NULL;
}

static boolint ReadMPECurve(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag)
{
	cmsToneCurve ** GammaTables = (cmsToneCurve **)Cargo;
	GammaTables[n] = ReadSegmentedCurve(self, io);
	return (GammaTables[n] != NULL);
	CXX_UNUSED(SizeOfTag);
}

static void * Type_MPEcurve_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsStage * mpe = NULL;
	uint16 InputChans, OutputChans;
	uint32 i;
	cmsToneCurve ** GammaTables;
	*nItems = 0;
	// Get actual position as a basis for element offsets
	uint32 BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	if(!_cmsReadUInt16Number(io, &InputChans)) 
		return NULL;
	if(!_cmsReadUInt16Number(io, &OutputChans)) 
		return NULL;
	if(InputChans != OutputChans) 
		return NULL;
	GammaTables = (cmsToneCurve **)_cmsCalloc(self->ContextID, InputChans, sizeof(cmsToneCurve *));
	if(GammaTables == NULL) 
		return NULL;
	if(ReadPositionTable(self, io, InputChans, BaseOffset, GammaTables, ReadMPECurve)) {
		mpe = cmsStageAllocToneCurves(self->ContextID, InputChans, GammaTables);
	}
	else {
		mpe = NULL;
	}
	for(i = 0; i < InputChans; i++) {
		cmsFreeToneCurve(GammaTables[i]);
	}
	_cmsFree(self->ContextID, GammaTables);
	*nItems = (mpe) ? 1U : 0;
	return mpe;
	CXX_UNUSED(SizeOfTag);
}

// Write a single segmented curve. NO CHECK IS PERFORMED ON VALIDITY
static boolint WriteSegmentedCurve(cmsIOHANDLER* io, cmsToneCurve * g)
{
	uint32 i, j;
	cmsCurveSegment* Segments = g->Segments;
	uint32 nSegments = g->nSegments;
	if(!_cmsWriteUInt32Number(io, cmsSigSegmentedCurve)) goto Error;
	if(!_cmsWriteUInt32Number(io, 0)) goto Error;
	if(!_cmsWriteUInt16Number(io, (uint16)nSegments)) goto Error;
	if(!_cmsWriteUInt16Number(io, 0)) goto Error;
	// Write the break-points
	for(i = 0; i < nSegments - 1; i++) {
		if(!_cmsWriteFloat32Number(io, Segments[i].x1)) goto Error;
	}
	// Write the segments
	for(i = 0; i < g->nSegments; i++) {
		cmsCurveSegment* ActualSeg = Segments + i;
		if(ActualSeg->Type == 0) {
			// This is a sampled curve
			if(!_cmsWriteUInt32Number(io, (uint32)cmsSigSampledCurveSeg)) goto Error;
			if(!_cmsWriteUInt32Number(io, 0)) goto Error;
			if(!_cmsWriteUInt32Number(io, ActualSeg->nGridPoints)) goto Error;

			for(j = 0; j < g->Segments[i].nGridPoints; j++) {
				if(!_cmsWriteFloat32Number(io, ActualSeg->SampledPoints[j])) goto Error;
			}
		}
		else {
			int Type;
			uint32 ParamsByType[] = { 4, 5, 5 };

			// This is a formula-based
			if(!_cmsWriteUInt32Number(io, (uint32)cmsSigFormulaCurveSeg)) goto Error;
			if(!_cmsWriteUInt32Number(io, 0)) goto Error;

			// We only allow 1, 2 and 3 as types
			Type = ActualSeg->Type - 6;
			if(Type > 2 || Type < 0) goto Error;

			if(!_cmsWriteUInt16Number(io, (uint16)Type)) goto Error;
			if(!_cmsWriteUInt16Number(io, 0)) goto Error;

			for(j = 0; j < ParamsByType[Type]; j++) {
				if(!_cmsWriteFloat32Number(io, (float)ActualSeg->Params[j])) goto Error;
			}
		}

		// It seems there is no need to align. Code is here, and for safety commented out
		// if(!_cmsWriteAlignment(io)) goto Error;
	}

	return TRUE;

Error:
	return FALSE;
}

static boolint WriteMPECurve(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag)
{
	_cmsStageToneCurvesData* Curves  = (_cmsStageToneCurvesData*)Cargo;
	return WriteSegmentedCurve(io, Curves->TheCurves[n]);
	CXX_UNUSED(SizeOfTag);
	CXX_UNUSED(self);
}

// Write a curve, checking first for validity
static boolint Type_MPEcurve_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint32 BaseOffset;
	cmsStage * mpe = (cmsStage *)Ptr;
	_cmsStageToneCurvesData* Curves = (_cmsStageToneCurvesData*)mpe->Data;
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// Write the header. Since those are curves, input and output channels are same
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->InputChannels)) return FALSE;
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->InputChannels)) return FALSE;

	if(!WritePositionTable(self, io, 0,
	    mpe->InputChannels, BaseOffset, Curves, WriteMPECurve)) return FALSE;

	return TRUE;

	CXX_UNUSED(nItems);
}

// The matrix is organized as an array of PxQ+Q elements, where P is the number of input channels to the
// matrix, and Q is the number of output channels. The matrix elements are each float32Numbers. The array
// is organized as follows:
// array = [e11, e12, ..., e1P, e21, e22, ..., e2P, ..., eQ1, eQ2, ..., eQP, e1, e2, ..., eQ]

static void * Type_MPEmatrix_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsStage * mpe;
	uint16 InputChans, OutputChans;
	uint32 nElems, i;
	double * Matrix;
	double * Offsets;
	if(!_cmsReadUInt16Number(io, &InputChans)) return NULL;
	if(!_cmsReadUInt16Number(io, &OutputChans)) return NULL;
	// Input and output chans may be ANY (up to 0xffff),
	// but we choose to limit to 16 channels for now
	if(InputChans >= cmsMAXCHANNELS) return NULL;
	if(OutputChans >= cmsMAXCHANNELS) return NULL;
	nElems = (uint32)InputChans * OutputChans;
	Matrix = (double *)_cmsCalloc(self->ContextID, nElems, sizeof(double));
	if(Matrix == NULL) return NULL;
	Offsets = (double *)_cmsCalloc(self->ContextID, OutputChans, sizeof(double));
	if(Offsets == NULL) {
		_cmsFree(self->ContextID, Matrix);
		return NULL;
	}
	for(i = 0; i < nElems; i++) {
		float v;
		if(!_cmsReadFloat32Number(io, &v)) {
			_cmsFree(self->ContextID, Matrix);
			_cmsFree(self->ContextID, Offsets);
			return NULL;
		}
		Matrix[i] = v;
	}
	for(i = 0; i < OutputChans; i++) {
		float v;

		if(!_cmsReadFloat32Number(io, &v)) {
			_cmsFree(self->ContextID, Matrix);
			_cmsFree(self->ContextID, Offsets);
			return NULL;
		}
		Offsets[i] = v;
	}

	mpe = cmsStageAllocMatrix(self->ContextID, OutputChans, InputChans, Matrix, Offsets);
	_cmsFree(self->ContextID, Matrix);
	_cmsFree(self->ContextID, Offsets);

	*nItems = 1;

	return mpe;

	CXX_UNUSED(SizeOfTag);
}

static boolint Type_MPEmatrix_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint32 i, nElems;
	cmsStage * mpe = (cmsStage *)Ptr;
	_cmsStageMatrixData* Matrix = (_cmsStageMatrixData*)mpe->Data;
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->InputChannels)) return FALSE;
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->OutputChannels)) return FALSE;
	nElems = mpe->InputChannels * mpe->OutputChannels;
	for(i = 0; i < nElems; i++) {
		if(!_cmsWriteFloat32Number(io, (float)Matrix->Double[i])) return FALSE;
	}
	for(i = 0; i < mpe->OutputChannels; i++) {
		if(Matrix->Offset == NULL) {
			if(!_cmsWriteFloat32Number(io, 0)) return FALSE;
		}
		else {
			if(!_cmsWriteFloat32Number(io, (float)Matrix->Offset[i])) return FALSE;
		}
	}

	return TRUE;

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

static void * Type_MPEclut_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsStage * mpe = NULL;
	uint16 InputChans, OutputChans;
	uint8 Dimensions8[16];
	uint32 i, nMaxGrids, GridPoints[MAX_INPUT_DIMENSIONS];
	_cmsStageCLutData* clut;
	if(!_cmsReadUInt16Number(io, &InputChans)) return NULL;
	if(!_cmsReadUInt16Number(io, &OutputChans)) return NULL;
	if(InputChans == 0) goto Error;
	if(OutputChans == 0) goto Error;
	if(io->Read(io, Dimensions8, sizeof(uint8), 16) != 16)
		goto Error;
	// Copy MAX_INPUT_DIMENSIONS at most. Expand to uint32
	nMaxGrids = InputChans > MAX_INPUT_DIMENSIONS ? (uint32)MAX_INPUT_DIMENSIONS : InputChans;
	for(i = 0; i < nMaxGrids; i++) {
		if(Dimensions8[i] == 1) goto Error; // Impossible value, 0 for no CLUT and then 2 at least
		GridPoints[i] = (uint32)Dimensions8[i];
	}

	// Allocate the true CLUT
	mpe = cmsStageAllocCLutFloatGranular(self->ContextID, GridPoints, InputChans, OutputChans, NULL);
	if(!mpe) 
		goto Error;
	// Read and sanitize the data
	clut = (_cmsStageCLutData*)mpe->Data;
	for(i = 0; i < clut->nEntries; i++) {
		if(!_cmsReadFloat32Number(io, &clut->Tab.TFloat[i])) goto Error;
	}
	*nItems = 1;
	return mpe;
Error:
	*nItems = 0;
	cmsStageFree(mpe);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// Write a CLUT in floating point
static boolint Type_MPEclut_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint8 Dimensions8[16]; // 16 because the spec says 16 and not max number of channels
	uint32 i;
	cmsStage * mpe = (cmsStage *)Ptr;
	_cmsStageCLutData* clut = (_cmsStageCLutData*)mpe->Data;
	// Check for maximum number of channels supported by lcms
	if(mpe->InputChannels > MAX_INPUT_DIMENSIONS) return FALSE;
	// Only floats are supported in MPE
	if(clut->HasFloatValues == FALSE) return FALSE;
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->InputChannels)) return FALSE;
	if(!_cmsWriteUInt16Number(io, (uint16)mpe->OutputChannels)) return FALSE;
	memzero(Dimensions8, sizeof(Dimensions8));
	for(i = 0; i < mpe->InputChannels; i++)
		Dimensions8[i] = (uint8)clut->Params->nSamples[i];
	if(!io->Write(io, 16, Dimensions8)) return FALSE;
	for(i = 0; i < clut->nEntries; i++) {
		if(!_cmsWriteFloat32Number(io, clut->Tab.TFloat[i])) return FALSE;
	}

	return TRUE;

	CXX_UNUSED(nItems);
	CXX_UNUSED(self);
}

// This is the list of built-in MPE types
static _cmsTagTypeLinkedList SupportedMPEtypes[] = {
	{{ (cmsTagTypeSignature)cmsSigBAcsElemType, NULL, NULL, NULL, NULL, NULL, 0 }, &SupportedMPEtypes[1] }, // Ignore those elements for now
	{{ (cmsTagTypeSignature)cmsSigEAcsElemType, NULL, NULL, NULL, NULL, NULL, 0 }, &SupportedMPEtypes[2] }, // (That's what the spec says)
	{TYPE_MPE_HANDLER((cmsTagTypeSignature)cmsSigCurveSetElemType,     MPEcurve),      &SupportedMPEtypes[3] },
	{TYPE_MPE_HANDLER((cmsTagTypeSignature)cmsSigMatrixElemType,       MPEmatrix),     &SupportedMPEtypes[4] },
	{TYPE_MPE_HANDLER((cmsTagTypeSignature)cmsSigCLutElemType,         MPEclut),        NULL },
};

_cmsTagTypePluginChunkType _cmsMPETypePluginChunk = { NULL };

static boolint ReadMPEElem(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Cargo, uint32 n, uint32 SizeOfTag)
{
	cmsStageSignature ElementSig;
	cmsTagTypeHandler* TypeHandler;
	uint32 nItems;
	cmsPipeline * NewLUT = (cmsPipeline *)Cargo;
	_cmsTagTypePluginChunkType* MPETypePluginChunk  =
	    (_cmsTagTypePluginChunkType*)_cmsContextGetClientChunk(self->ContextID, MPEPlugin);

	// Take signature and channels for each element.
	if(!_cmsReadUInt32Number(io, (uint32 *)&ElementSig)) return FALSE;

	// The reserved placeholder
	if(!_cmsReadUInt32Number(io, NULL)) return FALSE;

	// Read diverse MPE types
	TypeHandler = GetHandler((cmsTagTypeSignature)ElementSig, MPETypePluginChunk->TagTypes, SupportedMPEtypes);
	if(TypeHandler == NULL) {
		char String[5];

		_cmsTagSignature2String(String, (cmsTagSignature)ElementSig);

		// An unknown element was found.
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown MPE type '%s' found.", String);
		return FALSE;
	}

	// If no read method, just ignore the element (valid for cmsSigBAcsElemType and cmsSigEAcsElemType)
	// Read the MPE. No size is given
	if(TypeHandler->ReadPtr) {
		// This is a real element which should be read and processed
		if(!cmsPipelineInsertStage(NewLUT, cmsAT_END, (cmsStage *)TypeHandler->ReadPtr(self, io, &nItems, SizeOfTag)))
			return FALSE;
	}
	return TRUE;
	CXX_UNUSED(SizeOfTag);
	CXX_UNUSED(n);
}

// This is the main dispatcher for MPE
static void * Type_MPE_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint16 InputChans, OutputChans;
	uint32 ElementCount;
	cmsPipeline * NewLUT = NULL;
	uint32 BaseOffset;
	// Get actual position as a basis for element offsets
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// Read channels and element count
	if(!_cmsReadUInt16Number(io, &InputChans)) return NULL;
	if(!_cmsReadUInt16Number(io, &OutputChans)) return NULL;
	if(InputChans == 0 || InputChans >= cmsMAXCHANNELS) return NULL;
	if(OutputChans == 0 || OutputChans >= cmsMAXCHANNELS) return NULL;
	// Allocates an empty LUT
	NewLUT = cmsPipelineAlloc(self->ContextID, InputChans, OutputChans);
	if(NewLUT == NULL) return NULL;
	if(!_cmsReadUInt32Number(io, &ElementCount)) goto Error;
	if(!ReadPositionTable(self, io, ElementCount, BaseOffset, NewLUT, ReadMPEElem)) goto Error;
	// Check channel count
	if(InputChans != NewLUT->InputChannels ||
	    OutputChans != NewLUT->OutputChannels) goto Error;
	// Success
	*nItems = 1;
	return NewLUT;
	// Error
Error:
	cmsPipelineFree(NewLUT);
	*nItems = 0;
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// This one is a liitle bit more complex, so we don't use position tables this time.
static boolint Type_MPE_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	uint32 i, BaseOffset, DirectoryPos, CurrentPos;
	uint32 inputChan, outputChan;
	uint32 ElemCount;
	uint32 * ElementOffsets = NULL, * ElementSizes = NULL, Before;
	cmsStageSignature ElementSig;
	cmsPipeline * Lut = (cmsPipeline *)Ptr;
	cmsStage * Elem = Lut->Elements;
	cmsTagTypeHandler* TypeHandler;
	_cmsTagTypePluginChunkType* MPETypePluginChunk  =
	    (_cmsTagTypePluginChunkType*)_cmsContextGetClientChunk(self->ContextID, MPEPlugin);

	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);

	inputChan  = cmsPipelineInputChannels(Lut);
	outputChan = cmsPipelineOutputChannels(Lut);
	ElemCount  = cmsPipelineStageCount(Lut);

	ElementOffsets = (uint32 *)_cmsCalloc(self->ContextID, ElemCount, sizeof(uint32));
	if(ElementOffsets == NULL) goto Error;

	ElementSizes = (uint32 *)_cmsCalloc(self->ContextID, ElemCount, sizeof(uint32));
	if(ElementSizes == NULL) goto Error;

	// Write the head
	if(!_cmsWriteUInt16Number(io, (uint16)inputChan)) goto Error;
	if(!_cmsWriteUInt16Number(io, (uint16)outputChan)) goto Error;
	if(!_cmsWriteUInt32Number(io, (uint16)ElemCount)) goto Error;

	DirectoryPos = io->Tell(io);

	// Write a fake directory to be filled latter on
	for(i = 0; i < ElemCount; i++) {
		if(!_cmsWriteUInt32Number(io, 0)) goto Error; // Offset
		if(!_cmsWriteUInt32Number(io, 0)) goto Error; // size
	}

	// Write each single tag. Keep track of the size as well.
	for(i = 0; i < ElemCount; i++) {
		ElementOffsets[i] = io->Tell(io) - BaseOffset;

		ElementSig = Elem->Type;

		TypeHandler = GetHandler((cmsTagTypeSignature)ElementSig, MPETypePluginChunk->TagTypes, SupportedMPEtypes);
		if(TypeHandler == NULL) {
			char String[5];

			_cmsTagSignature2String(String, (cmsTagSignature)ElementSig);

			// An unknown element was found.
			cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Found unknown MPE type '%s'", String);
			goto Error;
		}

		if(!_cmsWriteUInt32Number(io, ElementSig)) goto Error;
		if(!_cmsWriteUInt32Number(io, 0)) goto Error;
		Before = io->Tell(io);
		if(!TypeHandler->WritePtr(self, io, Elem, 1)) goto Error;
		if(!_cmsWriteAlignment(io)) goto Error;

		ElementSizes[i] = io->Tell(io) - Before;

		Elem = Elem->Next;
	}

	// Write the directory
	CurrentPos = io->Tell(io);
	if(!io->Seek(io, DirectoryPos)) goto Error;
	for(i = 0; i < ElemCount; i++) {
		if(!_cmsWriteUInt32Number(io, ElementOffsets[i])) goto Error;
		if(!_cmsWriteUInt32Number(io, ElementSizes[i])) goto Error;
	}
	if(!io->Seek(io, CurrentPos)) 
		goto Error;
	_cmsFree(self->ContextID, ElementOffsets);
	_cmsFree(self->ContextID, ElementSizes);
	return TRUE;
Error:
	_cmsFree(self->ContextID, ElementOffsets);
	_cmsFree(self->ContextID, ElementSizes);
	return FALSE;
	CXX_UNUSED(nItems);
}

static void * Type_MPE_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsPipelineDup((cmsPipeline *)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_MPE_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsPipelineFree((cmsPipeline *)Ptr);
	return;
	CXX_UNUSED(self);
}
//
// Type cmsSigVcgtType
//
#define cmsVideoCardGammaTableType    0
#define cmsVideoCardGammaFormulaType  1

// Used internally
typedef struct {
	double Gamma;
	double Min;
	double Max;
} _cmsVCGTGAMMA;

static void * Type_vcgt_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	uint32 TagType, n, i;
	cmsToneCurve ** Curves;
	*nItems = 0;
	// Read tag type
	if(!_cmsReadUInt32Number(io, &TagType)) return NULL;

	// Allocate space for the array
	Curves = (cmsToneCurve **)_cmsCalloc(self->ContextID, 3, sizeof(cmsToneCurve *));
	if(Curves == NULL) return NULL;

	// There are two possible flavors
	switch(TagType) {
		// Gamma is stored as a table
		case cmsVideoCardGammaTableType:
	    {
		    uint16 nChannels, nElems, nBytes;

		    // Check channel count, which should be 3 (we don't support monochrome this time)
		    if(!_cmsReadUInt16Number(io, &nChannels)) goto Error;

		    if(nChannels != 3) {
			    cmsSignalError(self->ContextID,
				cmsERROR_UNKNOWN_EXTENSION,
				"Unsupported number of channels for VCGT '%d'",
				nChannels);
			    goto Error;
		    }

		    // Get Table element count and bytes per element
		    if(!_cmsReadUInt16Number(io, &nElems)) goto Error;
		    if(!_cmsReadUInt16Number(io, &nBytes)) goto Error;

		    // Adobe's quirk fixup. Fixing broken profiles...
		    if(nElems == 256 && nBytes == 1 && SizeOfTag == 1576)
			    nBytes = 2;

		    // Populate tone curves
		    for(n = 0; n < 3; n++) {
			    Curves[n] = cmsBuildTabulatedToneCurve16(self->ContextID, nElems, NULL);
			    if(Curves[n] == NULL) goto Error;

			    // On depending on byte depth
			    switch(nBytes) {
				    // One byte, 0..255
				    case 1:
					for(i = 0; i < nElems; i++) {
						uint8 v;

						if(!_cmsReadUInt8Number(io, &v)) goto Error;
						Curves[n]->Table16[i] = FROM_8_TO_16(v);
					}
					break;

				    // One word 0..65535
				    case 2:
					if(!_cmsReadUInt16Array(io, nElems, Curves[n]->Table16)) goto Error;
					break;

				    // Unsupported
				    default:
					cmsSignalError(self->ContextID,
					    cmsERROR_UNKNOWN_EXTENSION,
					    "Unsupported bit depth for VCGT '%d'",
					    nBytes * 8);
					goto Error;
			    }
		    } // For all 3 channels
	    }
	    break;

		// In this case, gamma is stored as a formula
		case cmsVideoCardGammaFormulaType:
	    {
		    _cmsVCGTGAMMA Colorant[3];

		    // Populate tone curves
		    for(n = 0; n < 3; n++) {
			    double Params[10];

			    if(!_cmsRead15Fixed16Number(io, &Colorant[n].Gamma)) goto Error;
			    if(!_cmsRead15Fixed16Number(io, &Colorant[n].Min)) goto Error;
			    if(!_cmsRead15Fixed16Number(io, &Colorant[n].Max)) goto Error;

			    // Parametric curve type 5 is:
			    // Y = (aX + b)^Gamma + e | X >= d
			    // Y = cX + f             | X < d

			    // vcgt formula is:
			    // Y = (Max - Min) * (X ^ Gamma) + Min

			    // So, the translation is
			    // a = (Max - Min) ^ ( 1 / Gamma)
			    // e = Min
			    // b=c=d=f=0

			    Params[0] = Colorant[n].Gamma;
			    Params[1] = pow((Colorant[n].Max - Colorant[n].Min), (1.0 / Colorant[n].Gamma));
			    Params[2] = 0;
			    Params[3] = 0;
			    Params[4] = 0;
			    Params[5] = Colorant[n].Min;
			    Params[6] = 0;

			    Curves[n] = cmsBuildParametricToneCurve(self->ContextID, 5, Params);
			    if(Curves[n] == NULL) goto Error;
		    }
	    }
	    break;
		// Unsupported
		default:
		    cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unsupported tag type for VCGT '%d'", TagType);
		    goto Error;
	}
	*nItems = 1;
	return (void *)Curves;
// Regret,  free all resources
Error:
	cmsFreeToneCurveTriple(Curves);
	_cmsFree(self->ContextID, Curves);
	return NULL;
	CXX_UNUSED(SizeOfTag);
}

// We don't support all flavors, only 16bits tables and formula
static boolint Type_vcgt_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsToneCurve ** Curves =  (cmsToneCurve **)Ptr;
	uint32 i, j;
	if(cmsGetToneCurveParametricType(Curves[0]) == 5 && cmsGetToneCurveParametricType(Curves[1]) == 5 && cmsGetToneCurveParametricType(Curves[2]) == 5) {
		if(!_cmsWriteUInt32Number(io, cmsVideoCardGammaFormulaType)) return FALSE;
		// Save parameters
		for(i = 0; i < 3; i++) {
			_cmsVCGTGAMMA v;
			v.Gamma = Curves[i]->Segments[0].Params[0];
			v.Min   = Curves[i]->Segments[0].Params[5];
			v.Max   = pow(Curves[i]->Segments[0].Params[1], v.Gamma) + v.Min;
			if(!_cmsWrite15Fixed16Number(io, v.Gamma)) return FALSE;
			if(!_cmsWrite15Fixed16Number(io, v.Min)) return FALSE;
			if(!_cmsWrite15Fixed16Number(io, v.Max)) return FALSE;
		}
	}

	else {
		// Always store as a table of 256 words
		if(!_cmsWriteUInt32Number(io, cmsVideoCardGammaTableType)) return FALSE;
		if(!_cmsWriteUInt16Number(io, 3)) return FALSE;
		if(!_cmsWriteUInt16Number(io, 256)) return FALSE;
		if(!_cmsWriteUInt16Number(io, 2)) return FALSE;
		for(i = 0; i < 3; i++) {
			for(j = 0; j < 256; j++) {
				float v = cmsEvalToneCurveFloat(Curves[i], (float)(j / 255.0));
				uint16 n = _cmsQuickSaturateWord(v * 65535.0);
				if(!_cmsWriteUInt16Number(io, n)) 
					return FALSE;
			}
		}
	}
	return TRUE;
	CXX_UNUSED(self);
	CXX_UNUSED(nItems);
}

static void * Type_vcgt_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	cmsToneCurve ** OldCurves =  (cmsToneCurve **)Ptr;
	cmsToneCurve ** NewCurves;
	NewCurves = (cmsToneCurve **)_cmsCalloc(self->ContextID, 3, sizeof(cmsToneCurve *));
	if(NewCurves == NULL) 
		return NULL;
	NewCurves[0] = cmsDupToneCurve(OldCurves[0]);
	NewCurves[1] = cmsDupToneCurve(OldCurves[1]);
	NewCurves[2] = cmsDupToneCurve(OldCurves[2]);
	return (void *)NewCurves;
	CXX_UNUSED(n);
}

static void Type_vcgt_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsFreeToneCurveTriple((cmsToneCurve **)Ptr);
	_cmsFree(self->ContextID, Ptr);
}
//
// Type cmsSigDictType
//
// Single column of the table can point to wchar or MLUC elements. Holds arrays of data
typedef struct {
	cmsContext ContextID;
	uint32 * Offsets;
	uint32 * Sizes;
} _cmsDICelem;

typedef struct {
	_cmsDICelem Name, Value, DisplayName, DisplayValue;
} _cmsDICarray;

// Allocate an empty array element
static boolint AllocElem(cmsContext ContextID, _cmsDICelem* e,  uint32 Count)
{
	e->Offsets = (uint32 *)_cmsCalloc(ContextID, Count, sizeof(uint32));
	if(e->Offsets == NULL) 
		return FALSE;
	e->Sizes = (uint32 *)_cmsCalloc(ContextID, Count, sizeof(uint32));
	if(e->Sizes == NULL) {
		_cmsFree(ContextID, e->Offsets);
		return FALSE;
	}
	e->ContextID = ContextID;
	return TRUE;
}

// Free an array element
static void FreeElem(_cmsDICelem * e)
{
	if(e) {
		_cmsFree(e->ContextID, e->Offsets);
		_cmsFree(e->ContextID, e->Sizes);
		e->Offsets = e->Sizes = NULL;
	}
}

// Get rid of whole array
static void FreeArray(_cmsDICarray* a)
{
	if(a->Name.Offsets) FreeElem(&a->Name);
	if(a->Value.Offsets) FreeElem(&a->Value);
	if(a->DisplayName.Offsets) FreeElem(&a->DisplayName);
	if(a->DisplayValue.Offsets) FreeElem(&a->DisplayValue);
}

// Allocate whole array
static boolint AllocArray(cmsContext ContextID, _cmsDICarray* a, uint32 Count, uint32 Length)
{
	// Empty values
	memzero(a, sizeof(_cmsDICarray));
	// On depending on record size, create column arrays
	if(!AllocElem(ContextID, &a->Name, Count)) goto Error;
	if(!AllocElem(ContextID, &a->Value, Count)) goto Error;
	if(Length > 16) {
		if(!AllocElem(ContextID, &a->DisplayName, Count)) goto Error;
	}
	if(Length > 24) {
		if(!AllocElem(ContextID, &a->DisplayValue, Count)) goto Error;
	}
	return TRUE;

Error:
	FreeArray(a);
	return FALSE;
}

// Read one element
static boolint ReadOneElem(cmsIOHANDLER* io,  _cmsDICelem* e, uint32 i, uint32 BaseOffset)
{
	if(!_cmsReadUInt32Number(io, &e->Offsets[i])) return FALSE;
	if(!_cmsReadUInt32Number(io, &e->Sizes[i])) return FALSE;

	// An offset of zero has special meaning and shal be preserved
	if(e->Offsets[i] > 0)
		e->Offsets[i] += BaseOffset;
	return TRUE;
}

static boolint ReadOffsetArray(cmsIOHANDLER* io,  _cmsDICarray* a, uint32 Count, uint32 Length, uint32 BaseOffset)
{
	// Read column arrays
	for(uint32 i = 0; i < Count; i++) {
		if(!ReadOneElem(io, &a->Name, i, BaseOffset)) return FALSE;
		if(!ReadOneElem(io, &a->Value, i, BaseOffset)) return FALSE;
		if(Length > 16) {
			if(!ReadOneElem(io, &a->DisplayName, i, BaseOffset)) return FALSE;
		}
		if(Length > 24) {
			if(!ReadOneElem(io, &a->DisplayValue, i, BaseOffset)) return FALSE;
		}
	}
	return TRUE;
}

// Write one element
static boolint WriteOneElem(cmsIOHANDLER* io,  _cmsDICelem* e, uint32 i)
{
	if(!_cmsWriteUInt32Number(io, e->Offsets[i])) return FALSE;
	if(!_cmsWriteUInt32Number(io, e->Sizes[i])) return FALSE;
	return TRUE;
}

static boolint WriteOffsetArray(cmsIOHANDLER* io,  _cmsDICarray* a, uint32 Count, uint32 Length)
{
	for(uint32 i = 0; i < Count; i++) {
		if(!WriteOneElem(io, &a->Name, i)) return FALSE;
		if(!WriteOneElem(io, &a->Value, i)) return FALSE;
		if(Length > 16) {
			if(!WriteOneElem(io, &a->DisplayName, i)) return FALSE;
		}
		if(Length > 24) {
			if(!WriteOneElem(io, &a->DisplayValue, i)) return FALSE;
		}
	}
	return TRUE;
}

static boolint ReadOneWChar(cmsIOHANDLER* io,  _cmsDICelem* e, uint32 i, wchar_t ** wcstr)
{
	uint32 nChars;

	// Special case for undefined strings (see ICC Votable
	// Proposal Submission, Dictionary Type and Metadata TAG Definition)
	if(e->Offsets[i] == 0) {
		*wcstr = NULL;
		return TRUE;
	}
	if(!io->Seek(io, e->Offsets[i])) return FALSE;
	nChars = e->Sizes[i] / sizeof(uint16);
	*wcstr = (wchar_t *)_cmsMallocZero(e->ContextID, (nChars + 1) * sizeof(wchar_t));
	if(*wcstr == NULL) return FALSE;
	if(!_cmsReadWCharArray(io, nChars, *wcstr)) {
		_cmsFree(e->ContextID, *wcstr);
		return FALSE;
	}
	// End of string marker
	(*wcstr)[nChars] = 0;
	return TRUE;
}

/* @sobolev (replaced with sstrlen) static uint32 mywcslen(const wchar_t * s)
{
	const wchar_t * p = s;
	while(*p)
		p++;
	return (uint32)(p - s);
}*/

static boolint WriteOneWChar(cmsIOHANDLER* io,  _cmsDICelem* e, uint32 i, const wchar_t * wcstr, uint32 BaseOffset)
{
	uint32 Before = io->Tell(io);
	uint32 n;
	e->Offsets[i] = Before - BaseOffset;
	if(wcstr == NULL) {
		e->Sizes[i] = 0;
		e->Offsets[i] = 0;
		return TRUE;
	}
	n = sstrlen(wcstr);
	if(!_cmsWriteWCharArray(io,  n, wcstr)) return FALSE;
	e->Sizes[i] = io->Tell(io) - Before;
	return TRUE;
}

static boolint ReadOneMLUC(struct _cms_typehandler_struct* self, cmsIOHANDLER* io,  _cmsDICelem* e, uint32 i, cmsMLU** mlu)
{
	uint32 nItems = 0;
	// A way to get null MLUCs
	if(e->Offsets[i] == 0 || e->Sizes[i] == 0) {
		*mlu = NULL;
		return TRUE;
	}
	if(!io->Seek(io, e->Offsets[i])) return FALSE;
	*mlu = (cmsMLU*)Type_MLU_Read(self, io, &nItems, e->Sizes[i]);
	return *mlu != NULL;
}

static boolint WriteOneMLUC(struct _cms_typehandler_struct* self, cmsIOHANDLER* io,
    _cmsDICelem* e, uint32 i, const cmsMLU* mlu, uint32 BaseOffset)
{
	uint32 Before;
	// Special case for undefined strings (see ICC Votable
	// Proposal Submission, Dictionary Type and Metadata TAG Definition)
	if(mlu == NULL) {
		e->Sizes[i] = 0;
		e->Offsets[i] = 0;
		return TRUE;
	}
	Before = io->Tell(io);
	e->Offsets[i] = Before - BaseOffset;
	if(!Type_MLU_Write(self, io, (void *)mlu, 1)) return FALSE;
	e->Sizes[i] = io->Tell(io) - Before;
	return TRUE;
}

static void * Type_Dictionary_Read(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, uint32 * nItems, uint32 SizeOfTag)
{
	cmsHANDLE hDict;
	uint32 i, Count, Length;
	uint32 BaseOffset;
	_cmsDICarray a;
	wchar_t * NameWCS = NULL, * ValueWCS = NULL;
	cmsMLU * DisplayNameMLU = NULL, * DisplayValueMLU = NULL;
	boolint rc;
	*nItems = 0;
	// Get actual position as a basis for element offsets
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// Get name-value record count
	if(!_cmsReadUInt32Number(io, &Count)) return NULL;
	SizeOfTag -= sizeof(uint32);
	// Get rec length
	if(!_cmsReadUInt32Number(io, &Length)) return NULL;
	SizeOfTag -= sizeof(uint32);

	// Check for valid lengths
	if(Length != 16 && Length != 24 && Length != 32) {
		cmsSignalError(self->ContextID, cmsERROR_UNKNOWN_EXTENSION, "Unknown record length in dictionary '%d'", Length);
		return NULL;
	}
	// Creates an empty dictionary
	hDict = cmsDictAlloc(self->ContextID);
	if(hDict == NULL) return NULL;
	// On depending on record size, create column arrays
	if(!AllocArray(self->ContextID, &a, Count, Length)) goto Error;
	// Read column arrays
	if(!ReadOffsetArray(io, &a, Count, Length, BaseOffset)) goto Error;
	// Seek to each element and read it
	for(i = 0; i < Count; i++) {
		if(!ReadOneWChar(io, &a.Name, i, &NameWCS)) goto Error;
		if(!ReadOneWChar(io, &a.Value, i, &ValueWCS)) goto Error;
		if(Length > 16) {
			if(!ReadOneMLUC(self, io, &a.DisplayName, i, &DisplayNameMLU)) goto Error;
		}
		if(Length > 24) {
			if(!ReadOneMLUC(self, io, &a.DisplayValue, i, &DisplayValueMLU)) goto Error;
		}
		if(NameWCS == NULL || ValueWCS == NULL) {
			cmsSignalError(self->ContextID, cmsERROR_CORRUPTION_DETECTED, "Bad dictionary Name/Value");
			rc = FALSE;
		}
		else {
			rc = cmsDictAddEntry(hDict, NameWCS, ValueWCS, DisplayNameMLU, DisplayValueMLU);
		}
		_cmsFree(self->ContextID, NameWCS);
		_cmsFree(self->ContextID, ValueWCS);
		cmsMLUfree(DisplayNameMLU);
		cmsMLUfree(DisplayValueMLU);
		if(!rc) 
			goto Error;
	}
	FreeArray(&a);
	*nItems = 1;
	return (void *)hDict;
Error:
	FreeArray(&a);
	cmsDictFree(hDict);
	return NULL;
}

static boolint Type_Dictionary_Write(struct _cms_typehandler_struct* self, cmsIOHANDLER* io, void * Ptr, uint32 nItems)
{
	cmsHANDLE hDict = (cmsHANDLE)Ptr;
	const cmsDICTentry* p;
	boolint AnyName, AnyValue;
	uint32 i, Count, Length;
	uint32 DirectoryPos, CurrentPos, BaseOffset;
	_cmsDICarray a;
	if(hDict == NULL) return FALSE;
	BaseOffset = io->Tell(io) - sizeof(_cmsTagBase);
	// Let's inspect the dictionary
	Count = 0; AnyName = FALSE; AnyValue = FALSE;
	for(p = cmsDictGetEntryList(hDict); p; p = cmsDictNextEntry(p)) {
		if(p->DisplayName) AnyName = TRUE;
		if(p->DisplayValue) AnyValue = TRUE;
		Count++;
	}
	Length = 16;
	if(AnyName) Length += 8;
	if(AnyValue) Length += 8;
	if(!_cmsWriteUInt32Number(io, Count)) return FALSE;
	if(!_cmsWriteUInt32Number(io, Length)) return FALSE;
	// Keep starting position of offsets table
	DirectoryPos = io->Tell(io);

	// Allocate offsets array
	if(!AllocArray(self->ContextID, &a, Count, Length)) goto Error;

	// Write a fake directory to be filled latter on
	if(!WriteOffsetArray(io, &a, Count, Length)) goto Error;

	// Write each element. Keep track of the size as well.
	p = cmsDictGetEntryList(hDict);
	for(i = 0; i < Count; i++) {
		if(!WriteOneWChar(io, &a.Name, i,  p->Name, BaseOffset)) goto Error;
		if(!WriteOneWChar(io, &a.Value, i, p->Value, BaseOffset)) goto Error;

		if(p->DisplayName) {
			if(!WriteOneMLUC(self, io, &a.DisplayName, i, p->DisplayName, BaseOffset)) goto Error;
		}

		if(p->DisplayValue) {
			if(!WriteOneMLUC(self, io, &a.DisplayValue, i, p->DisplayValue, BaseOffset)) goto Error;
		}

		p = cmsDictNextEntry(p);
	}

	// Write the directory
	CurrentPos = io->Tell(io);
	if(!io->Seek(io, DirectoryPos)) goto Error;
	if(!WriteOffsetArray(io, &a, Count, Length)) goto Error;
	if(!io->Seek(io, CurrentPos)) goto Error;
	FreeArray(&a);
	return TRUE;
Error:
	FreeArray(&a);
	return FALSE;
	CXX_UNUSED(nItems);
}

static void * Type_Dictionary_Dup(struct _cms_typehandler_struct* self, const void * Ptr, uint32 n)
{
	return (void *)cmsDictDup((cmsHANDLE)Ptr);
	CXX_UNUSED(n);
	CXX_UNUSED(self);
}

static void Type_Dictionary_Free(struct _cms_typehandler_struct* self, void * Ptr)
{
	cmsDictFree((cmsHANDLE)Ptr);
	CXX_UNUSED(self);
}
//
// Type support main routines
//
// This is the list of built-in types
static const _cmsTagTypeLinkedList SupportedTagTypes[] = {
	{TYPE_HANDLER(cmsSigChromaticityType,          Chromaticity),       (_cmsTagTypeLinkedList*)&SupportedTagTypes[1] },
	{TYPE_HANDLER(cmsSigColorantOrderType,         ColorantOrderType),  (_cmsTagTypeLinkedList*)&SupportedTagTypes[2] },
	{TYPE_HANDLER(cmsSigS15Fixed16ArrayType,       S15Fixed16),         (_cmsTagTypeLinkedList*)&SupportedTagTypes[3] },
	{TYPE_HANDLER(cmsSigU16Fixed16ArrayType,       U16Fixed16),         (_cmsTagTypeLinkedList*)&SupportedTagTypes[4] },
	{TYPE_HANDLER(cmsSigTextType,                  Text),               (_cmsTagTypeLinkedList*)&SupportedTagTypes[5] },
	{TYPE_HANDLER(cmsSigTextDescriptionType,       Text_Description),   (_cmsTagTypeLinkedList*)&SupportedTagTypes[6] },
	{TYPE_HANDLER(cmsSigCurveType,                 Curve),              (_cmsTagTypeLinkedList*)&SupportedTagTypes[7] },
	{TYPE_HANDLER(cmsSigParametricCurveType,       ParametricCurve),    (_cmsTagTypeLinkedList*)&SupportedTagTypes[8] },
	{TYPE_HANDLER(cmsSigDateTimeType,              DateTime),           (_cmsTagTypeLinkedList*)&SupportedTagTypes[9] },
	{TYPE_HANDLER(cmsSigLut8Type,                  LUT8),               (_cmsTagTypeLinkedList*)&SupportedTagTypes[10] },
	{TYPE_HANDLER(cmsSigLut16Type,                 LUT16),              (_cmsTagTypeLinkedList*)&SupportedTagTypes[11] },
	{TYPE_HANDLER(cmsSigColorantTableType,         ColorantTable),      (_cmsTagTypeLinkedList*)&SupportedTagTypes[12] },
	{TYPE_HANDLER(cmsSigNamedColor2Type,           NamedColor),         (_cmsTagTypeLinkedList*)&SupportedTagTypes[13] },
	{TYPE_HANDLER(cmsSigMultiLocalizedUnicodeType, MLU),                (_cmsTagTypeLinkedList*)&SupportedTagTypes[14] },
	{TYPE_HANDLER(cmsSigProfileSequenceDescType,   ProfileSequenceDesc), (_cmsTagTypeLinkedList*)&SupportedTagTypes[15] },
	{TYPE_HANDLER(cmsSigSignatureType,             Signature),          (_cmsTagTypeLinkedList*)&SupportedTagTypes[16] },
	{TYPE_HANDLER(cmsSigMeasurementType,           Measurement),        (_cmsTagTypeLinkedList*)&SupportedTagTypes[17] },
	{TYPE_HANDLER(cmsSigDataType,                  Data),               (_cmsTagTypeLinkedList*)&SupportedTagTypes[18] },
	{TYPE_HANDLER(cmsSigLutAtoBType,               LUTA2B),             (_cmsTagTypeLinkedList*)&SupportedTagTypes[19] },
	{TYPE_HANDLER(cmsSigLutBtoAType,               LUTB2A),             (_cmsTagTypeLinkedList*)&SupportedTagTypes[20] },
	{TYPE_HANDLER(cmsSigUcrBgType,                 UcrBg),              (_cmsTagTypeLinkedList*)&SupportedTagTypes[21] },
	{TYPE_HANDLER(cmsSigCrdInfoType,               CrdInfo),            (_cmsTagTypeLinkedList*)&SupportedTagTypes[22] },
	{TYPE_HANDLER(cmsSigMultiProcessElementType,   MPE),                (_cmsTagTypeLinkedList*)&SupportedTagTypes[23] },
	{TYPE_HANDLER(cmsSigScreeningType,             Screening),          (_cmsTagTypeLinkedList*)&SupportedTagTypes[24] },
	{TYPE_HANDLER(cmsSigViewingConditionsType,     ViewingConditions),  (_cmsTagTypeLinkedList*)&SupportedTagTypes[25] },
	{TYPE_HANDLER(cmsSigXYZType,                   XYZ),                (_cmsTagTypeLinkedList*)&SupportedTagTypes[26] },
	{TYPE_HANDLER(cmsCorbisBrokenXYZtype,          XYZ),                (_cmsTagTypeLinkedList*)&SupportedTagTypes[27] },
	{TYPE_HANDLER(cmsMonacoBrokenCurveType,        Curve),              (_cmsTagTypeLinkedList*)&SupportedTagTypes[28] },
	{TYPE_HANDLER(cmsSigProfileSequenceIdType,     ProfileSequenceId),  (_cmsTagTypeLinkedList*)&SupportedTagTypes[29] },
	{TYPE_HANDLER(cmsSigDictType,                  Dictionary),         (_cmsTagTypeLinkedList*)&SupportedTagTypes[30] },
	{TYPE_HANDLER(cmsSigVcgtType,                  vcgt),                NULL }
};

_cmsTagTypePluginChunkType _cmsTagTypePluginChunk = { NULL };

// Duplicates the zone of memory used by the plug-in in the new context
static void DupTagTypeList(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src, int loc)
{
	_cmsTagTypePluginChunkType newHead = { NULL };
	_cmsTagTypeLinkedList*  entry;
	_cmsTagTypeLinkedList*  Anterior = NULL;
	_cmsTagTypePluginChunkType* head = (_cmsTagTypePluginChunkType*)src->chunks[loc];
	// Walk the list copying all nodes
	for(entry = head->TagTypes; entry; entry = entry->Next) {
		_cmsTagTypeLinkedList * newEntry = (_cmsTagTypeLinkedList*)_cmsSubAllocDup(ctx->MemPool, entry, sizeof(_cmsTagTypeLinkedList));
		if(!newEntry)
			return;
		// We want to keep the linked list order, so this is a little bit tricky
		newEntry->Next = NULL;
		if(Anterior)
			Anterior->Next = newEntry;
		Anterior = newEntry;
		if(newHead.TagTypes == NULL)
			newHead.TagTypes = newEntry;
	}
	ctx->chunks[loc] = _cmsSubAllocDup(ctx->MemPool, &newHead, sizeof(_cmsTagTypePluginChunkType));
}

void _cmsAllocTagTypePluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	if(src) {
		// Duplicate the LIST
		DupTagTypeList(ctx, src, TagTypePlugin);
	}
	else {
		static _cmsTagTypePluginChunkType TagTypePluginChunk = { NULL };
		ctx->chunks[TagTypePlugin] = _cmsSubAllocDup(ctx->MemPool, &TagTypePluginChunk, sizeof(_cmsTagTypePluginChunkType));
	}
}

void _cmsAllocMPETypePluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	if(src) {
		// Duplicate the LIST
		DupTagTypeList(ctx, src, MPEPlugin);
	}
	else {
		static _cmsTagTypePluginChunkType TagTypePluginChunk = { NULL };
		ctx->chunks[MPEPlugin] = _cmsSubAllocDup(ctx->MemPool, &TagTypePluginChunk, sizeof(_cmsTagTypePluginChunkType));
	}
}

// Both kind of plug-ins share same structure
boolint _cmsRegisterTagTypePlugin(cmsContext id, cmsPluginBase* Data)
{
	return RegisterTypesPlugin(id, Data, TagTypePlugin);
}

boolint _cmsRegisterMultiProcessElementPlugin(cmsContext id, cmsPluginBase* Data)
{
	return RegisterTypesPlugin(id, Data, MPEPlugin);
}

// Wrapper for tag types
cmsTagTypeHandler* _cmsGetTagTypeHandler(cmsContext ContextID, cmsTagTypeSignature sig)
{
	_cmsTagTypePluginChunkType* ctx = (_cmsTagTypePluginChunkType*)_cmsContextGetClientChunk(ContextID, TagTypePlugin);
	return GetHandler(sig, ctx->TagTypes, (_cmsTagTypeLinkedList*)SupportedTagTypes);
}
//
// Tag support main routines
//
typedef struct _cmsTagLinkedList_st {
	cmsTagSignature Signature;
	cmsTagDescriptor Descriptor;
	struct _cmsTagLinkedList_st* Next;
} _cmsTagLinkedList;

// This is the list of built-in tags. The data of this list can be modified by plug-ins
static _cmsTagLinkedList SupportedTags[] = {
	{ cmsSigAToB0Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutAtoBType, cmsSigLut8Type}, DecideLUTtypeA2B},
	  &SupportedTags[1]},
	{ cmsSigAToB1Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutAtoBType, cmsSigLut8Type}, DecideLUTtypeA2B},
	  &SupportedTags[2]},
	{ cmsSigAToB2Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutAtoBType, cmsSigLut8Type}, DecideLUTtypeA2B},
	  &SupportedTags[3]},
	{ cmsSigBToA0Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutBtoAType, cmsSigLut8Type}, DecideLUTtypeB2A},
	  &SupportedTags[4]},
	{ cmsSigBToA1Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutBtoAType, cmsSigLut8Type}, DecideLUTtypeB2A},
	  &SupportedTags[5]},
	{ cmsSigBToA2Tag,               { 1, 3,  { cmsSigLut16Type,  cmsSigLutBtoAType, cmsSigLut8Type}, DecideLUTtypeB2A},
	  &SupportedTags[6]},

	// Allow corbis  and its broken XYZ type
	{ cmsSigRedColorantTag,         { 1, 2, { cmsSigXYZType, cmsCorbisBrokenXYZtype }, DecideXYZtype}, &SupportedTags[7]},
	{ cmsSigGreenColorantTag,       { 1, 2, { cmsSigXYZType, cmsCorbisBrokenXYZtype }, DecideXYZtype}, &SupportedTags[8]},
	{ cmsSigBlueColorantTag,        { 1, 2, { cmsSigXYZType, cmsCorbisBrokenXYZtype }, DecideXYZtype}, &SupportedTags[9]},

	{ cmsSigRedTRCTag,              { 1, 3, { cmsSigCurveType, cmsSigParametricCurveType, cmsMonacoBrokenCurveType }, DecideCurveType},
	  &SupportedTags[10]},
	{ cmsSigGreenTRCTag,            { 1, 3, { cmsSigCurveType, cmsSigParametricCurveType, cmsMonacoBrokenCurveType }, DecideCurveType},
	  &SupportedTags[11]},
	{ cmsSigBlueTRCTag,             { 1, 3, { cmsSigCurveType, cmsSigParametricCurveType, cmsMonacoBrokenCurveType }, DecideCurveType},
	  &SupportedTags[12]},

	{ cmsSigCalibrationDateTimeTag, { 1, 1, { cmsSigDateTimeType }, NULL}, &SupportedTags[13]},
	{ cmsSigCharTargetTag,          { 1, 1, { cmsSigTextType },     NULL}, &SupportedTags[14]},

	{ cmsSigChromaticAdaptationTag, { 9, 1, { cmsSigS15Fixed16ArrayType }, NULL}, &SupportedTags[15]},
	{ cmsSigChromaticityTag,        { 1, 1, { cmsSigChromaticityType    }, NULL}, &SupportedTags[16]},
	{ cmsSigColorantOrderTag,       { 1, 1, { cmsSigColorantOrderType   }, NULL}, &SupportedTags[17]},
	{ cmsSigColorantTableTag,       { 1, 1, { cmsSigColorantTableType   }, NULL}, &SupportedTags[18]},
	{ cmsSigColorantTableOutTag,    { 1, 1, { cmsSigColorantTableType   }, NULL}, &SupportedTags[19]},

	{ cmsSigCopyrightTag,
	  { 1, 3, { cmsSigTextType,  cmsSigMultiLocalizedUnicodeType, cmsSigTextDescriptionType}, DecideTextType},
	  &SupportedTags[20]},
	{ cmsSigDateTimeTag,            { 1, 1, { cmsSigDateTimeType }, NULL}, &SupportedTags[21]},

	{ cmsSigDeviceMfgDescTag,
	  { 1, 3, { cmsSigTextDescriptionType, cmsSigMultiLocalizedUnicodeType, cmsSigTextType}, DecideTextDescType},
	  &SupportedTags[22]},
	{ cmsSigDeviceModelDescTag,
	  { 1, 3, { cmsSigTextDescriptionType, cmsSigMultiLocalizedUnicodeType, cmsSigTextType}, DecideTextDescType},
	  &SupportedTags[23]},

	{ cmsSigGamutTag,               { 1, 3, { cmsSigLut16Type, cmsSigLutBtoAType, cmsSigLut8Type }, DecideLUTtypeB2A},
	  &SupportedTags[24]},

	{ cmsSigGrayTRCTag,             { 1, 2, { cmsSigCurveType, cmsSigParametricCurveType }, DecideCurveType}, &SupportedTags[25]},
	{ cmsSigLuminanceTag,           { 1, 1, { cmsSigXYZType }, NULL}, &SupportedTags[26]},

	{ cmsSigMediaBlackPointTag,     { 1, 2, { cmsSigXYZType, cmsCorbisBrokenXYZtype }, NULL}, &SupportedTags[27]},
	{ cmsSigMediaWhitePointTag,     { 1, 2, { cmsSigXYZType, cmsCorbisBrokenXYZtype }, NULL}, &SupportedTags[28]},

	{ cmsSigNamedColor2Tag,         { 1, 1, { cmsSigNamedColor2Type }, NULL}, &SupportedTags[29]},

	{ cmsSigPreview0Tag,            { 1, 3,  { cmsSigLut16Type, cmsSigLutBtoAType, cmsSigLut8Type }, DecideLUTtypeB2A},
	  &SupportedTags[30]},
	{ cmsSigPreview1Tag,            { 1, 3,  { cmsSigLut16Type, cmsSigLutBtoAType, cmsSigLut8Type }, DecideLUTtypeB2A}, &SupportedTags[31]},
	{ cmsSigPreview2Tag,            { 1, 3,  { cmsSigLut16Type, cmsSigLutBtoAType, cmsSigLut8Type }, DecideLUTtypeB2A}, &SupportedTags[32]},

	{ cmsSigProfileDescriptionTag, { 1, 3, { cmsSigTextDescriptionType, cmsSigMultiLocalizedUnicodeType, cmsSigTextType}, DecideTextDescType}, &SupportedTags[33]},
	{ cmsSigProfileSequenceDescTag, { 1, 1, { cmsSigProfileSequenceDescType }, NULL},  &SupportedTags[34]},
	{ cmsSigTechnologyTag,          { 1, 1, { cmsSigSignatureType }, NULL},  &SupportedTags[35]},

	{ cmsSigColorimetricIntentImageStateTag,   { 1, 1, { cmsSigSignatureType }, NULL}, &SupportedTags[36]},
	{ cmsSigPerceptualRenderingIntentGamutTag, { 1, 1, { cmsSigSignatureType }, NULL}, &SupportedTags[37]},
	{ cmsSigSaturationRenderingIntentGamutTag, { 1, 1, { cmsSigSignatureType }, NULL}, &SupportedTags[38]},

	{ cmsSigMeasurementTag,         { 1, 1, { cmsSigMeasurementType }, NULL}, &SupportedTags[39]},

	{ cmsSigPs2CRD0Tag,             { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[40]},
	{ cmsSigPs2CRD1Tag,             { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[41]},
	{ cmsSigPs2CRD2Tag,             { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[42]},
	{ cmsSigPs2CRD3Tag,             { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[43]},
	{ cmsSigPs2CSATag,              { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[44]},
	{ cmsSigPs2RenderingIntentTag,  { 1, 1, { cmsSigDataType }, NULL}, &SupportedTags[45]},

	{ cmsSigViewingCondDescTag, { 1, 3, { cmsSigTextDescriptionType, cmsSigMultiLocalizedUnicodeType, cmsSigTextType}, DecideTextDescType}, &SupportedTags[46]},

	{ cmsSigUcrBgTag,               { 1, 1, { cmsSigUcrBgType}, NULL},    &SupportedTags[47]},
	{ cmsSigCrdInfoTag,             { 1, 1, { cmsSigCrdInfoType}, NULL},  &SupportedTags[48]},

	{ cmsSigDToB0Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[49]},
	{ cmsSigDToB1Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[50]},
	{ cmsSigDToB2Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[51]},
	{ cmsSigDToB3Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[52]},
	{ cmsSigBToD0Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[53]},
	{ cmsSigBToD1Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[54]},
	{ cmsSigBToD2Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[55]},
	{ cmsSigBToD3Tag,               { 1, 1, { cmsSigMultiProcessElementType}, NULL}, &SupportedTags[56]},

	{ cmsSigScreeningDescTag,       { 1, 1, { cmsSigTextDescriptionType },    NULL}, &SupportedTags[57]},
	{ cmsSigViewingConditionsTag,   { 1, 1, { cmsSigViewingConditionsType },  NULL}, &SupportedTags[58]},

	{ cmsSigScreeningTag,           { 1, 1, { cmsSigScreeningType},          NULL }, &SupportedTags[59]},
	{ cmsSigVcgtTag,                { 1, 1, { cmsSigVcgtType},               NULL }, &SupportedTags[60]},
	{ cmsSigMetaTag,                { 1, 1, { cmsSigDictType},               NULL }, &SupportedTags[61]},
	{ cmsSigProfileSequenceIdTag,   { 1, 1, { cmsSigProfileSequenceIdType},  NULL }, &SupportedTags[62]},

	{ cmsSigProfileDescriptionMLTag, { 1, 1, { cmsSigMultiLocalizedUnicodeType}, NULL}, &SupportedTags[63]},
	{ cmsSigArgyllArtsTag,          { 9, 1, { cmsSigS15Fixed16ArrayType},    NULL}, NULL}
};

/*
    Not supported                 Why
    =======================       =========================================
    cmsSigOutputResponseTag   ==> WARNING, POSSIBLE PATENT ON THIS SUBJECT!
    cmsSigNamedColorTag       ==> Deprecated
    cmsSigDataTag             ==> Ancient, unused
    cmsSigDeviceSettingsTag   ==> Deprecated, useless
 */

_cmsTagPluginChunkType _cmsTagPluginChunk = { NULL };

// Duplicates the zone of memory used by the plug-in in the new context
static void DupTagList(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	_cmsTagPluginChunkType newHead = { NULL };
	_cmsTagLinkedList*  entry;
	_cmsTagLinkedList*  Anterior = NULL;
	_cmsTagPluginChunkType* head = (_cmsTagPluginChunkType*)src->chunks[TagPlugin];

	// Walk the list copying all nodes
	for(entry = head->Tag; entry; entry = entry->Next) {
		_cmsTagLinkedList * newEntry = (_cmsTagLinkedList*)_cmsSubAllocDup(ctx->MemPool, entry, sizeof(_cmsTagLinkedList));
		if(!newEntry)
			return;
		// We want to keep the linked list order, so this is a little bit tricky
		newEntry->Next = NULL;
		if(Anterior)
			Anterior->Next = newEntry;
		Anterior = newEntry;
		if(newHead.Tag == NULL)
			newHead.Tag = newEntry;
	}
	ctx->chunks[TagPlugin] = _cmsSubAllocDup(ctx->MemPool, &newHead, sizeof(_cmsTagPluginChunkType));
}

void _cmsAllocTagPluginChunk(struct _cmsContext_struct* ctx, const struct _cmsContext_struct* src)
{
	if(src) {
		DupTagList(ctx, src);
	}
	else {
		static _cmsTagPluginChunkType TagPluginChunk = { NULL };
		ctx->chunks[TagPlugin] = _cmsSubAllocDup(ctx->MemPool, &TagPluginChunk, sizeof(_cmsTagPluginChunkType));
	}
}

boolint _cmsRegisterTagPlugin(cmsContext id, cmsPluginBase* Data)
{
	cmsPluginTag* Plugin = (cmsPluginTag*)Data;
	_cmsTagLinkedList * pt;
	_cmsTagPluginChunkType* TagPluginChunk = (_cmsTagPluginChunkType*)_cmsContextGetClientChunk(id, TagPlugin);
	if(!Data) {
		TagPluginChunk->Tag = NULL;
		return TRUE;
	}
	pt = (_cmsTagLinkedList*)_cmsPluginMalloc(id, sizeof(_cmsTagLinkedList));
	if(pt == NULL) 
		return FALSE;
	pt->Signature  = Plugin->Signature;
	pt->Descriptor = Plugin->Descriptor;
	pt->Next       = TagPluginChunk->Tag;
	TagPluginChunk->Tag = pt;
	return TRUE;
}

// Return a descriptor for a given tag or NULL
cmsTagDescriptor* _cmsGetTagDescriptor(cmsContext ContextID, cmsTagSignature sig)
{
	_cmsTagLinkedList* pt;
	_cmsTagPluginChunkType* TagPluginChunk = (_cmsTagPluginChunkType*)_cmsContextGetClientChunk(ContextID, TagPlugin);
	for(pt = TagPluginChunk->Tag; pt; pt = pt->Next) {
		if(sig == pt->Signature) return &pt->Descriptor;
	}
	for(pt = SupportedTags; pt; pt = pt->Next) {
		if(sig == pt->Signature) return &pt->Descriptor;
	}
	return NULL;
}
