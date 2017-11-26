/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2009 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "xlslib-internal.h"
#pragma hdrstop

using namespace xlslib_core;

const uint32 xlslib_core::summaryFormat[] = {
	0xf29f85e0,
	0x10684ff9,
	0x000891ab,
	0xd9b3272b
};
const uint32 xlslib_core::docSummaryFormat[] = {
	0xd5cdd502,
	0x101b2e9c,
	0x00089793,
	0xaef92c2b
};
const uint32 xlslib_core::hpsfValues[] = {
	30,     // HPSF_STRING,
	11,     // HPSF_BOOL,
	2,      // HPSF_INT16,
	3,      // HPSF_INT32,
	64,     // HPSF_INT64
};

HPSFitem::HPSFitem(uint16 v, const std::string& str) :
	propID(v),
	variant(HPSF_STRING),
	value(),
	offset(0)
{
	value.str = new std::string(str);
}

HPSFitem::HPSFitem(uint16 v, bool val) :
	propID(v),
	variant(HPSF_BOOL),
	value(),
	offset(0)
{
	value.isOn = val;
}

HPSFitem::HPSFitem(uint16 v, uint16 val) :
	propID(v),
	variant(HPSF_INT16),
	value(),
	offset(0)
{
	value.val16 = val;
}

HPSFitem::HPSFitem(uint16 v, uint32 val) :
	propID(v),
	variant(HPSF_INT32),
	value(),
	offset(0)
{
	value.val32 = val;
}

HPSFitem::HPSFitem(uint16 v, uint64 val) :
	propID(v),
	variant(HPSF_INT64),
	value(),
	offset(0)
{
	value.val64 = val;
}

HPSFitem::~HPSFitem()
{
	if(variant == HPSF_STRING) {
		delete value.str;
	}
}

size_t HPSFitem::GetSize()
{
	size_t size;
	switch(variant)	{
	case HPSF_STRING:
		size = value.str->length() + 1 + 4;         // 1 for null terminator, 4 for length field
		// round up to the next 4-byte boundary:
		size = (size + 4 - 1) & ~3ul;
		XL_ASSERT(size >= 4);
		XL_ASSERT((size % 4) == 0);
		break;
	case HPSF_BOOL:
		size = 2 + 2; // 2 + 2 padding
		break;
	case HPSF_INT16:
		size = 2 + 2; // 2 + 2 padding
		break;
	case HPSF_INT32:
		size = 4; // 0 padding
		break;
	case HPSF_INT64:
		size = 8; // 0 padding
		break;
	default:
		size = 0; // 0 padding
		break;
	}

	return size + 4;    // variant at the start
}

hpsf_doc_t::hpsf_doc_t(docType_t dt) : docType(dt), itemList()
{
}

hpsf_doc_t::~hpsf_doc_t()
{
	HPSF_Set_Itor_t	hIter;
	HPSF_Set_Itor_t	hBegin		= itemList.begin();
	HPSF_Set_Itor_t	hEnd		= itemList.end();
	for(hIter=hBegin; hIter != hEnd; ++hIter) {
		delete *hIter;
	}
}

void hpsf_doc_t::insert(HPSFitem *item)
{
	HPSFitem*		existingItem;
	bool success;
	do {
		std::pair<HPSF_Set_Itor_t, bool> ret = itemList.insert(item);
		success = ret.second;

		if(!success) {
			existingItem = *(ret.first);
			delete existingItem;
			itemList.erase(existingItem);
		}
	} while(!success);
}

uint64 hpsf_doc_t::unix2mstime(time_t unixTime)
{
	uint64 msTime;
	msTime   = (uint64)unixTime * (uint64)1000000 + FILETIME2UNIX_NS;
	msTime	*= (uint64)10;
	return msTime;
}

size_t hpsf_doc_t::GetSize(void) const
{
	return SUMMARY_SIZE;    // this file will only be this size, ever (zero padded)
}

CUnit* hpsf_doc_t::GetData(CDataStorage &datastore) const
{
	return datastore.MakeCHPSFdoc(*this);   // NOTE: this pointer HAS to be deleted elsewhere.
}

//
// http://poi.apache.org/hpsf/internals.html
// or google "DocumentSummaryInformation and UserDefined Property Sets" and look for MSDN hits
//
CHPSFdoc::CHPSFdoc(CDataStorage &datastore, const hpsf_doc_t& docdef) :
	CUnit(datastore)
{
	HPSF_Set_ConstItor_t hBegin, hEnd, hIter;
	const uint32 *fmt;
	size_t sectionListOffset;
	size_t numProperties;
	size_t itemOffset;

	numProperties = docdef.itemList.size();
	fmt = (docdef.docType == HPSF_SUMMARY ? summaryFormat : docSummaryFormat);

	int ret = Inflate(SUMMARY_SIZE);    // this file will only be this size, ever (zero padded)
	if (ret == NO_ERRORS) {
		// Header
		AddValue16(0xfffe); // signature
		AddValue16(0);
#ifdef __OSX__
		AddValue32(1);      // Macintosh
#else
		AddValue32(2);      // WIN32
#endif
		AddValue32(0); AddValue32(0); AddValue32(0); AddValue32(0);     // CLASS
		AddValue32(1);      // One section

		// The section (this is a list but in this case just 1 section so can shorten logic)
		AddValue32(fmt[0]); // Class ID
		AddValue32(fmt[1]);
		AddValue32(fmt[2]);
		AddValue32(fmt[3]);

		// offset to the data (would vary if multiple sections, but since one only its easy
		sectionListOffset = GetDataSize() + 4;      // offset from the start of this stream to first byte after this offset     [i_a]
		AddValue32((uint32)sectionListOffset);                // where this section starts (right after this tag!)

		// Start of Section 1: sectionListOffset starts here
		AddValue32(0);                              // length of the section - updated later
		AddValue32((uint32)numProperties);                    //

		// now write the propertyLists - the values, and where to find the payloads
		itemOffset	= 8 + numProperties * 8;    // where I am now, then allow for propertyLists
		hBegin		= docdef.itemList.begin();
		hEnd		= docdef.itemList.end();
		for(hIter=hBegin; hIter != hEnd; ++hIter) {
			HPSFitem	*item = *hIter;

			item->SetOffset(itemOffset);

			AddValue32(item->GetPropID());          // Variant (ie type)
			AddValue32((uint32)itemOffset);   // where the actual data will be found

			itemOffset += item->GetSize();
		}
		SetValueAt32((uint32)itemOffset, (uint32)sectionListOffset);
		//printf("Think size is %d\n", (int)itemOffset);

		// Now we can write out the actual data
		hBegin		= docdef.itemList.begin();
		hEnd		= docdef.itemList.end();
		for(hIter=hBegin; hIter != hEnd; ++hIter) {
			HPSFitem		*item = *hIter;
			size_t len;
			size_t padding;
			uint16 variant;
			hValue value;

			value	= item->GetValue();
			variant	= item->GetVariant();
			//printf("PROPERTY[%d]: ActualOffset=%d savedOffset=%d variant=%d realVariant=%d val64=%llx\n", item->GetPropID(),
			//  m_nDataSize - sectionListOffset, item->GetOffset(), variant,  hpsfValues[variant], value.val64);
			AddValue32(hpsfValues[variant]);

			switch(variant)	{
			case HPSF_STRING:
				len = value.str->length() + 1;  // length of string plus null terminator
				// padding = (len % 4) + 1;		// string terminator is the "1"
				// round up to the next 4-byte boundary to determine the padding;
				// take the mandatory NUL sentinel into account as well:
				padding = 1 + ((4 - len) & 3);
				XL_ASSERT(padding + len - 1 >= 4);
				XL_ASSERT((padding + len - 1) % 4 == 0);
				AddValue32((uint32)len);
				AddDataArray((const uint8 *)value.str->c_str(), len-1);
				break;
			case HPSF_BOOL:
				padding = 2;
				AddValue16(value.isOn ? 0xFFFF : 0x0000);   // per MSDN google of VT_BOOL
				break;
			case HPSF_INT16:
				padding = 2;
				AddValue16(value.val16);
				break;
			case HPSF_INT32:
				padding = 0;
				AddValue32(value.val32);
				break;
			case HPSF_INT64:
				padding = 0;
				AddValue64(value.val64);
				break;
			default:
				padding = 0;
				break;
			}
			AddFixedDataArray(0, padding);
		}

		//printf("Actual size = %d\n", m_nDataSize - sectionListOffset);
		XL_ASSERT(GetDataSize() <= GetSize());
		XL_ASSERT(GetDataSize() <= SUMMARY_SIZE);
		AddFixedDataArray(0, SUMMARY_SIZE - GetDataSize());
		XL_ASSERT(GetDataSize() <= GetSize());
	}
}

CHPSFdoc::~CHPSFdoc()
{
}
