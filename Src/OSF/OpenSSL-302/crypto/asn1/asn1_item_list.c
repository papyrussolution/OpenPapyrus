/*
 * Copyright 2000-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include <internal/openssl-crypto-internal.h>
#pragma hdrstop
#include "asn1_item_list.h"

const ASN1_ITEM * ASN1_ITEM_lookup(const char * name)
{
	for(size_t i = 0; i < SIZEOFARRAY(asn1_item_list); i++) {
		const ASN1_ITEM * it = ASN1_ITEM_ptr(asn1_item_list[i]);
		if(strcmp(it->sname, name) == 0)
			return it;
	}
	return NULL;
}

const ASN1_ITEM * ASN1_ITEM_get(size_t i)
{
	if(i >= SIZEOFARRAY(asn1_item_list))
		return NULL;
	return ASN1_ITEM_ptr(asn1_item_list[i]);
}
