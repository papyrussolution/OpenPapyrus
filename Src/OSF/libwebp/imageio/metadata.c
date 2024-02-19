// Copyright 2012 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
//  Metadata types and functions.
//
#include <libwebp-internal.h>
#pragma hdrstop
#include "./metadata.h"

void MetadataInit(Metadata* const metadata) 
{
	if(metadata == NULL) return;
	memzero(metadata, sizeof(*metadata));
}

void MetadataPayloadDelete(MetadataPayload* const payload) 
{
	if(payload == NULL) 
		return;
	SAlloc::F(payload->bytes);
	payload->bytes = NULL;
	payload->size = 0;
}

void MetadataFree(Metadata* const metadata) 
{
	if(metadata == NULL) return;
	MetadataPayloadDelete(&metadata->exif);
	MetadataPayloadDelete(&metadata->iccp);
	MetadataPayloadDelete(&metadata->xmp);
}

int MetadataCopy(const char* metadata, size_t metadata_len, MetadataPayload* const payload) 
{
	if(metadata == NULL || metadata_len == 0 || payload == NULL) 
		return 0;
	payload->bytes = (uint8*)SAlloc::M(metadata_len);
	if(payload->bytes == NULL) 
		return 0;
	payload->size = metadata_len;
	memcpy(payload->bytes, metadata, metadata_len);
	return 1;
}
