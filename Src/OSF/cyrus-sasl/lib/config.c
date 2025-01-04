/* SASL Config file API
 * Rob Siemborski
 * Tim Martin (originally in Cyrus distribution)
 */
// Copyright (c) 1998-2016 Carnegie Mellon University.  All rights reserved.
//
#include <sasl-internal.h>
#pragma hdrstop

struct configlist {
	char * key;
	char * value;
};

static struct configlist * configlist = NULL;
static int nconfiglist = 0;

#define CONFIGLISTGROWSIZE 100

int sasl_config_init(const char * filename)
{
	FILE * infile;
	int lineno = 0;
	int alloced = 0;
	char buf[4096];
	char * p, * key;
	char * tail;
	int result;

	nconfiglist = 0;

	infile = fopen(filename, "r");
	if(!infile) {
		return SASL_CONTINUE;
	}

	while(fgets(buf, sizeof(buf), infile)) {
		lineno++;

		if(buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1] = '\0';
		for(p = buf; *p && isspace((int)*p); p++);
		if(!*p || *p == '#') continue;

		key = p;
		while(*p && (isalnum((int)*p) || *p == '-' || *p == '_')) {
			if(isupper((int)*p)) *p = (char)tolower(*p);
			p++;
		}
		if(*p != ':') {
			fclose(infile);
			return SASL_CONFIGERR;
		}
		*p++ = '\0';

		while(*p && isspace((int)*p)) p++;

		if(!*p) {
			fclose(infile);
			return SASL_CONFIGERR;
		}

		/* Now strip trailing spaces, if any */
		tail = p + strlen(p) - 1;
		while(tail > p && isspace((int)*tail)) {
			*tail = '\0';
			tail--;
		}
		if(nconfiglist == alloced) {
			alloced += CONFIGLISTGROWSIZE;
			configlist = (struct configlist *)sasl_REALLOC((char *)configlist, alloced * sizeof(struct configlist));
			if(configlist == NULL) {
				fclose(infile);
				return SASL_NOMEM;
			}
		}

		result = _sasl_strdup(key,
			&(configlist[nconfiglist].key),
			NULL);
		if(result != SASL_OK) {
			fclose(infile);
			return result;
		}
		result = _sasl_strdup(p,
			&(configlist[nconfiglist].value),
			NULL);
		if(result != SASL_OK) {
			fclose(infile);
			return result;
		}

		nconfiglist++;
	}
	fclose(infile);

	return SASL_OK;
}

const char * sasl_config_getstring(const char * key, const char * def)
{
	int opt;

	for(opt = 0; opt < nconfiglist; opt++) {
		if(*key == configlist[opt].key[0] &&
		    !strcmp(key, configlist[opt].key))
			return configlist[opt].value;
	}
	return def;
}

void sasl_config_done(void)
{
	int opt;

	for(opt = 0; opt < nconfiglist; opt++) {
		if(configlist[opt].key) sasl_FREE(configlist[opt].key);
		if(configlist[opt].value) sasl_FREE(configlist[opt].value);
	}

	sasl_FREE(configlist);
	configlist = NULL;
	nconfiglist = 0;
}
