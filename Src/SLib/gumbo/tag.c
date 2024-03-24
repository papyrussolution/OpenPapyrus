// Copyright 2011 Google Inc. All Rights Reserved.
// Licensed under the Apache License, Version 2.0 (the "License");
//
// Author: jdtang@google.com (Jonathan Tang)
//
#include <slib-internal.h>
#pragma hdrstop
#include "gumbo-internal.h"

const char * kGumboTagNames[] = {
#include "tag_strings.h"
	"", // TAG_UNKNOWN
	"", // TAG_LAST
};

static const uchar kGumboTagSizes[] = {
#include "tag_sizes.h"
	0, // TAG_UNKNOWN
	0, // TAG_LAST
};

const char * gumbo_normalized_tagname(GumboTag tag) 
{
	assert(tag <= GUMBO_TAG_LAST);
	return kGumboTagNames[tag];
}

void FASTCALL GumboTagFromOriginalTextC(const GumboStringPiece * pText, SString & rTag)
{
	rTag.Z();
	if(pText && pText->data) {
		assert(pText->length >= 2);
		assert(pText->data[0] == '<');
		assert(pText->data[pText->length - 1] == '>');
		if(pText->data[1] == '/') {
			// End tag.
			assert(pText->length >= 3);
			rTag.CatN(pText->data+2, pText->length-3);
			//pText->data += 2; // Move past </
			//pText->length -= 3;
		}
		else {
			// Start tag.
			size_t len = pText->length-2;
			//pText->data += 1; // Move past <
			//pText->length -= 2;
			// strnchr is apparently not a standard C library function, so I loop
			// explicitly looking for whitespace or other illegal tag characters.
			for(const char * c = pText->data; c != pText->data + pText->length; ++c) {
				if(isspace(*c) || *c == '/') {
					//pText->length = c - pText->data;
					len = c - pText->data;
					break;
				}
			}
			rTag.CatN(pText->data+1, len);
		}
	}	
}

void FASTCALL gumbo_tag_from_original_text(GumboStringPiece * pText) 
{
	if(pText && pText->data) {
		assert(pText->length >= 2);
		assert(pText->data[0] == '<');
		assert(pText->data[pText->length - 1] == '>');
		if(pText->data[1] == '/') {
			// End tag.
			assert(pText->length >= 3);
			pText->data += 2; // Move past </
			pText->length -= 3;
		}
		else {
			// Start tag.
			pText->data += 1; // Move past <
			pText->length -= 2;
			// strnchr is apparently not a standard C library function, so I loop
			// explicitly looking for whitespace or other illegal tag characters.
			for(const char * c = pText->data; c != pText->data + pText->length; ++c) {
				if(isspace(*c) || *c == '/') {
					pText->length = c - pText->data;
					break;
				}
			}
		}
	}
}

static int case_memcmp(const char * s1, const char * s2, uint n) 
{
	while(n--) {
		uchar c1 = tolower(*s1++);
		uchar c2 = tolower(*s2++);
		if(c1 != c2) return (int)c1 - (int)c2;
	}
	return 0;
}

#include "tag_gperf.h"
//#define TAG_MAP_SIZE (sizeof(kGumboTagMap) / sizeof(kGumboTagMap[0]))

GumboTag gumbo_tagn_enum(const char * tagname, uint length) 
{
	if(length) {
		uint key = tag_hash(tagname, length);
		if(key < SIZEOFARRAY(kGumboTagMap)) {
			GumboTag tag = static_cast<GumboTag>(kGumboTagMap[key]);
			if(length == kGumboTagSizes[(int)tag] && !case_memcmp(tagname, kGumboTagNames[(int)tag], length))
				return tag;
		}
	}
	return GUMBO_TAG_UNKNOWN;
}

GumboTag gumbo_tag_enum(const char * tagname) 
{
	return gumbo_tagn_enum(tagname, strlen(tagname));
}
