/*
 * bind_config.c - Parse the SSH server configuration file
 * This file is part of the SSH Library
 * Copyright (c) 2019 by Red Hat, Inc.
 *
 * Author: Anderson Toshiyuki Sasaki <ansasaki@redhat.com>
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <libssh-internal.h>
#pragma hdrstop
#ifdef HAVE_GLOB_H
	#include <glob.h>
#endif

#define MAX_LINE_SIZE 1024

/* Flags used for the parser state */
#define PARSING     1
#define IN_MATCH    (1<<1)

struct ssh_bind_config_keyword_table_s {
	const char * name;
	enum ssh_bind_config_opcode_e opcode;
	bool allowed_in_match;
};

static struct ssh_bind_config_keyword_table_s ssh_bind_config_keyword_table[] = {
	{ "include", BIND_CFG_INCLUDE, false },
	{ "hostkey", BIND_CFG_HOSTKEY, false },
	{ "listenaddress", BIND_CFG_LISTENADDRESS, false },
	{ "port", BIND_CFG_PORT, false },
	{ "loglevel", BIND_CFG_LOGLEVEL, true },
	{ "ciphers", BIND_CFG_CIPHERS, false },
	{ "macs", BIND_CFG_MACS, false },
	{ "kexalgorithms", BIND_CFG_KEXALGORITHMS, false },
	{ "match", BIND_CFG_MATCH, true },
	{ "pubkeyacceptedkeytypes", BIND_CFG_PUBKEY_ACCEPTED_KEY_TYPES, true },
	{ "hostkeyalgorithms", BIND_CFG_HOSTKEY_ALGORITHMS, true },
	{ 0, BIND_CFG_UNKNOWN, false }
};

enum ssh_bind_config_match_e {
	BIND_MATCH_UNKNOWN = -1,
	BIND_MATCH_ALL,
	BIND_MATCH_USER,
	BIND_MATCH_GROUP,
	BIND_MATCH_HOST,
	BIND_MATCH_LOCALADDRESS,
	BIND_MATCH_LOCALPORT,
	BIND_MATCH_RDOMAIN,
	BIND_MATCH_ADDRESS,
};

struct ssh_bind_config_match_keyword_table_s {
	const char * name;
	enum ssh_bind_config_match_e opcode;
};

static struct ssh_bind_config_match_keyword_table_s ssh_bind_config_match_keyword_table[] = {
	{ "all", BIND_MATCH_ALL },
	{ "user", BIND_MATCH_USER },
	{ "group", BIND_MATCH_GROUP },
	{ "host", BIND_MATCH_HOST },
	{ "localaddress", BIND_MATCH_LOCALADDRESS },
	{ "localport", BIND_MATCH_LOCALPORT },
	{ "rdomain", BIND_MATCH_RDOMAIN },
	{ "address", BIND_MATCH_ADDRESS },
	{ 0, BIND_MATCH_UNKNOWN },
};

static enum ssh_bind_config_opcode_e ssh_bind_config_get_opcode(char * keyword, uint32_t * parser_flags) 
{
	for(int i = 0; ssh_bind_config_keyword_table[i].name != NULL; i++) {
		if(strcasecmp(keyword, ssh_bind_config_keyword_table[i].name) == 0) {
			if((*parser_flags & IN_MATCH) && !(ssh_bind_config_keyword_table[i].allowed_in_match)) {
				return BIND_CFG_NOT_ALLOWED_IN_MATCH;
			}
			return ssh_bind_config_keyword_table[i].opcode;
		}
	}
	return BIND_CFG_UNKNOWN;
}

static int ssh_bind_config_parse_line(ssh_bind bind, const char * line, uint count, uint32_t * parser_flags, uint8 * seen);

static void local_parse_file(ssh_bind bind, const char * filename, uint32_t * parser_flags, uint8 * seen)
{
	char line[MAX_LINE_SIZE] = {0};
	uint count = 0;
	int rv;
	FILE * f = fopen(filename, "r");
	if(f == NULL) {
		SSH_LOG(SSH_LOG_RARE, "Cannot find file %s to load", filename);
		return;
	}
	SSH_LOG(SSH_LOG_PACKET, "Reading additional configuration data from %s", filename);
	while(fgets(line, sizeof(line), f)) {
		count++;
		rv = ssh_bind_config_parse_line(bind, line, count, parser_flags, seen);
		if(rv < 0) {
			fclose(f);
			return;
		}
	}
	fclose(f);
	return;
}

#if defined(HAVE_GLOB) && defined(HAVE_GLOB_GL_FLAGS_MEMBER)
static void local_parse_glob(ssh_bind bind, const char * fileglob, uint32_t * parser_flags, uint8 * seen)
{
	glob_t globbuf = { .gl_flags = 0, };
	u_int i;
	int rt = glob(fileglob, GLOB_TILDE, NULL, &globbuf);
	if(rt == GLOB_NOMATCH) {
		globfree(&globbuf);
		return;
	}
	else if(rt != 0) {
		SSH_LOG(SSH_LOG_RARE, "Glob error: %s", fileglob);
		globfree(&globbuf);
		return;
	}
	for(i = 0; i < globbuf.gl_pathc; i++) {
		local_parse_file(bind, globbuf.gl_pathv[i], parser_flags, seen);
	}
	globfree(&globbuf);
}
#endif /* HAVE_GLOB HAVE_GLOB_GL_FLAGS_MEMBER */

static enum ssh_bind_config_match_e ssh_bind_config_get_match_opcode(const char * keyword) 
{
	for(size_t i = 0; ssh_bind_config_match_keyword_table[i].name != NULL; i++) {
		if(strcasecmp(keyword, ssh_bind_config_match_keyword_table[i].name) == 0) {
			return ssh_bind_config_match_keyword_table[i].opcode;
		}
	}
	return BIND_MATCH_UNKNOWN;
}

static int ssh_bind_config_parse_line(ssh_bind bind, const char * line, uint count, uint32_t * parser_flags, uint8 * seen)
{
	enum ssh_bind_config_opcode_e opcode;
	const char * p = NULL;
	char * s = NULL, * x = NULL;
	char * keyword = NULL;
	size_t len;
	int rc = 0;
	if(bind == NULL) {
		return -1;
	}
	if((line == NULL) || (parser_flags == NULL)) {
		ssh_set_error_invalid(bind);
		return -1;
	}
	x = s = sstrdup(line);
	if(s == NULL) {
		ssh_set_error_oom(bind);
		return -1;
	}
	/* Remove trailing spaces */
	for(len = strlen(s) - 1; len > 0; len--) {
		if(!isspace(s[len])) {
			break;
		}
		s[len] = '\0';
	}
	keyword = ssh_config_get_token(&s);
	if(keyword == NULL || *keyword == '#' || *keyword == '\0' || *keyword == '\n') {
		ZFREE(x);
		return 0;
	}
	opcode = ssh_bind_config_get_opcode(keyword, parser_flags);
	if((*parser_flags & PARSING) && opcode != BIND_CFG_HOSTKEY && opcode != BIND_CFG_INCLUDE && opcode != BIND_CFG_MATCH && opcode > BIND_CFG_UNSUPPORTED) { /* Ignore all unknown types here */
		/* Skip all the options that were already applied */
		if(seen[opcode] != 0) {
			ZFREE(x);
			return 0;
		}
		seen[opcode] = 1;
	}

	switch(opcode) {
		case BIND_CFG_INCLUDE:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
#if defined(HAVE_GLOB) && defined(HAVE_GLOB_GL_FLAGS_MEMBER)
			    local_parse_glob(bind, p, parser_flags, seen);
#else
			    local_parse_file(bind, p, parser_flags, seen);
#endif /* HAVE_GLOB */
		    }
		    break;

		case BIND_CFG_HOSTKEY:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_HOSTKEY, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set Hostkey value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_LISTENADDRESS:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDADDR, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set ListenAddress value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_PORT:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDPORT_STR, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set Port value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_CIPHERS:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_CIPHERS_C_S, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set C->S Ciphers value '%s'", count, p);
				    break;
			    }
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_CIPHERS_S_C, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set S->C Ciphers value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_MACS:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_HMAC_C_S, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set C->S MAC value '%s'", count, p);
				    break;
			    }
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_HMAC_S_C, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set S->C MAC value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_LOGLEVEL:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    int value = -1;
			    if(sstreqi_ascii(p, "quiet")) {
				    value = SSH_LOG_NONE;
			    }
			    else if(sstreqi_ascii(p, "fatal") || sstreqi_ascii(p, "error") || sstreqi_ascii(p, "info")) {
				    value = SSH_LOG_WARN;
			    }
			    else if(sstreqi_ascii(p, "verbose")) {
				    value = SSH_LOG_INFO;
			    }
			    else if(sstreqi_ascii(p, "DEBUG") || sstreqi_ascii(p, "DEBUG1")) {
				    value = SSH_LOG_DEBUG;
			    }
			    else if(sstreqi_ascii(p, "DEBUG2") || sstreqi_ascii(p, "DEBUG3")) {
				    value = SSH_LOG_TRACE;
			    }
			    if(value != -1) {
				    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_LOG_VERBOSITY, &value);
				    if(rc) {
					    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set LogLevel value '%s'", count, p);
				    }
			    }
		    }
		    break;
		case BIND_CFG_KEXALGORITHMS:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_KEY_EXCHANGE, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set KexAlgorithms value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_MATCH: {
		    bool negate;
		    int result = PARSING;
		    size_t args = 0;
		    enum ssh_bind_config_match_e opt;
		    const char * p2 = NULL;

		    /* The options set in Match blocks should be applied when a connection
		     * is accepted, and not right away when parsing the file (as it is
		     * currently done). This means the configuration files should be parsed
		     * again or the options set in the Match blocks should be stored and
		     * applied as necessary. */

		    /* If this is the first Match block, erase the seen table to allow
		     * options to be overridden. Erasing the seen table was the easiest way
		     * to allow overriding an option, but only for the first occurrence of
		     * an option in a Match block. This is sufficient for the current
		     * implementation which supports only the 'All' criterion, meaning the
		     * options can be applied right away. */
		    if(!(*parser_flags & IN_MATCH)) {
			    memset(seen, 0x00, BIND_CFG_MAX * sizeof(uint8));
		    }

		    /* In this line the PARSING bit is cleared from the flags */
		    *parser_flags = IN_MATCH;
		    do {
			    p = p2 = ssh_config_get_str_tok(&s, NULL);
			    if(isempty(p)) {
				    break;
			    }
			    args++;
			    SSH_LOG(SSH_LOG_TRACE, "line %d: Processing Match keyword '%s'",
				count, p);

			    /* If the option is prefixed with ! the result should be negated */
			    negate = false;
			    if(p[0] == '!') {
				    negate = true;
				    p++;
			    }

			    opt = ssh_bind_config_get_match_opcode(p);
			    switch(opt) {
				    case BIND_MATCH_ALL:
					p = ssh_config_get_str_tok(&s, NULL);
					if(args == 1 && isempty(p)) {
						// The "all" keyword does not accept arguments or modifiers
						if(negate == true) {
							result = 0;
						}
						break;
					}
					ssh_set_error(bind, SSH_FATAL, "line %d: ERROR - Match all cannot be combined with other Match attributes", count);
					ZFREE(x);
					return -1;
				    case BIND_MATCH_USER:
				    case BIND_MATCH_GROUP:
				    case BIND_MATCH_HOST:
				    case BIND_MATCH_LOCALADDRESS:
				    case BIND_MATCH_LOCALPORT:
				    case BIND_MATCH_RDOMAIN:
				    case BIND_MATCH_ADDRESS:
					/* Only "All" is supported for now */
					/* Skip one argument */
					p = ssh_config_get_str_tok(&s, NULL);
					if(isempty(p)) {
						SSH_LOG(SSH_LOG_WARN, "line %d: Match keyword '%s' requires argument\n", count, p2);
						ZFREE(x);
						return -1;
					}
					args++;
					SSH_LOG(SSH_LOG_WARN, "line %d: Unsupported Match keyword '%s', ignoring\n", count, p2);
					result = 0;
					break;
				    case BIND_MATCH_UNKNOWN:
				    default:
					ssh_set_error(bind, SSH_FATAL, "ERROR - Unknown argument '%s' for Match keyword", p);
					ZFREE(x);
					return -1;
			    }
		    } while(!isempty(p));
		    if(args == 0) {
			    ssh_set_error(bind, SSH_FATAL, "ERROR - Match keyword requires an argument");
			    ZFREE(x);
			    return -1;
		    }
		    /* This line only sets the PARSING flag if all checks passed */
		    *parser_flags |= result;
		    break;
	    }
		case BIND_CFG_PUBKEY_ACCEPTED_KEY_TYPES:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_PUBKEY_ACCEPTED_KEY_TYPES, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set PubKeyAcceptedKeyTypes value '%s'",
					count, p);
			    }
		    }
		    break;
		case BIND_CFG_HOSTKEY_ALGORITHMS:
		    p = ssh_config_get_str_tok(&s, NULL);
		    if(p && (*parser_flags & PARSING)) {
			    rc = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_HOSTKEY_ALGORITHMS, p);
			    if(rc) {
				    SSH_LOG(SSH_LOG_WARN, "line %d: Failed to set HostkeyAlgorithms value '%s'", count, p);
			    }
		    }
		    break;
		case BIND_CFG_NOT_ALLOWED_IN_MATCH:
		    SSH_LOG(SSH_LOG_WARN, "Option not allowed in Match block: %s, line: %d", keyword, count);
		    break;
		case BIND_CFG_UNKNOWN:
		    SSH_LOG(SSH_LOG_WARN, "Unknown option: %s, line: %d", keyword, count);
		    break;
		case BIND_CFG_UNSUPPORTED:
		    SSH_LOG(SSH_LOG_WARN, "Unsupported option: %s, line: %d", keyword, count);
		    break;
		case BIND_CFG_NA:
		    SSH_LOG(SSH_LOG_WARN, "Option not applicable: %s, line: %d", keyword, count);
		    break;
		default:
		    ssh_set_error(bind, SSH_FATAL, "ERROR - unimplemented opcode: %d", opcode);
		    ZFREE(x);
		    return -1;
		    break;
	}
	ZFREE(x);
	return rc;
}

int ssh_bind_config_parse_file(ssh_bind bind, const char * filename)
{
	char line[MAX_LINE_SIZE] = {0};
	uint count = 0;
	uint32_t parser_flags;
	int rv;
	/* This local table is used during the parsing of the current file (and
	 * files included recursively in this file) to prevent an option to be
	 * redefined, i.e. the first value set is kept. But this DO NOT prevent the
	 * option to be redefined later by another file. */
	uint8 seen[BIND_CFG_MAX] = {0};
	FILE * f = fopen(filename, "r");
	if(f) {
		SSH_LOG(SSH_LOG_PACKET, "Reading configuration data from %s", filename);
		parser_flags = PARSING;
		while(fgets(line, sizeof(line), f)) {
			count++;
			rv = ssh_bind_config_parse_line(bind, line, count, &parser_flags, seen);
			if(rv) {
				fclose(f);
				return -1;
			}
		}
		fclose(f);
	}
	return 0;
}
