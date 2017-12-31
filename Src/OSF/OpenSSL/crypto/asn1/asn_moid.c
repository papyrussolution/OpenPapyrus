/*
 * Copyright 2002-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

/* Simple ASN1 OID module: add all objects in a given section */

static int do_create(const char * value, const char * name);

static int oid_module_init(CONF_IMODULE * md, const CONF * cnf)
{
	int i;
	STACK_OF(CONF_VALUE) *sktmp;
	CONF_VALUE * oval;
	const char * oid_section = CONF_imodule_get_value(md);
	if((sktmp = NCONF_get_section(cnf, oid_section)) == NULL) {
		ASN1err(ASN1_F_OID_MODULE_INIT, ASN1_R_ERROR_LOADING_SECTION);
		return 0;
	}
	for(i = 0; i < sk_CONF_VALUE_num(sktmp); i++) {
		oval = sk_CONF_VALUE_value(sktmp, i);
		if(!do_create(oval->value, oval->name)) {
			ASN1err(ASN1_F_OID_MODULE_INIT, ASN1_R_ADDING_OBJECT);
			return 0;
		}
	}
	return 1;
}

static void oid_module_finish(CONF_IMODULE * md)
{
}

void ASN1_add_oid_module(void)
{
	CONF_module_add("oid_section", oid_module_init, oid_module_finish);
}

/*-
 * Create an OID based on a name value pair. Accept two formats.
 * shortname = 1.2.3.4
 * shortname = some long name, 1.2.3.4
 */

static int do_create(const char * value, const char * name)
{
	int nid;
	ASN1_OBJECT * oid;
	const char * ln, * ostr;
	char * lntmp;
	const char * p = strrchr(value, ',');
	if(!p) {
		ln = name;
		ostr = value;
	}
	else {
		ln = NULL;
		ostr = p + 1;
		if(!*ostr)
			return 0;
		while(isspace((uchar)*ostr))
			ostr++;
	}
	nid = OBJ_create(ostr, name, ln);
	if(nid == NID_undef)
		return 0;
	if(p) {
		ln = value;
		while(isspace((uchar)*ln))
			ln++;
		p--;
		while(isspace((uchar)*p)) {
			if(p == ln)
				return 0;
			p--;
		}
		p++;
		lntmp = (char*)OPENSSL_malloc((p - ln) + 1);
		if(lntmp == NULL)
			return 0;
		memcpy(lntmp, ln, p - ln);
		lntmp[p - ln] = 0;
		oid = OBJ_nid2obj(nid);
		oid->ln = lntmp;
	}
	return 1;
}

