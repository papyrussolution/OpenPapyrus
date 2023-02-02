/* Copyright (c) 2007, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * Author: falmeida@google.com (Filipe Almeida)
 */

#ifndef SECURITY_STREAMHTMLPARSER_JSPARSER_H
#define SECURITY_STREAMHTMLPARSER_JSPARSER_H

#include <config.h>
#include "htmlparser/statemachine.h"

#ifdef __cplusplus
namespace ctemplate_htmlparser {
#endif /* __cplusplus */

/* Size of the ring buffer used to lookup the last token in the javascript
 * stream. The size is pretty much arbitrary at this point but must be bigger
 * than the biggest token we want to lookup plus 3: Two delimiters plus an empty
 * ring buffer slot. */
#define JSPARSER_RING_BUFFER_SIZE 18

enum js_state_external_enum {
	JSPARSER_STATE_TEXT,
	JSPARSER_STATE_Q,
	JSPARSER_STATE_DQ,
	JSPARSER_STATE_REGEXP,
	JSPARSER_STATE_COMMENT
};

/* Stores the context of the javascript parser.
 *
 * If this structure is changed, jsparser_new(), jsparser_copy() and
 * jsparser_reset() should be updated accordingly.
 */
typedef struct jsparser_ctx_s {
	statemachine_ctx * statemachine; /* Reference to the statemachine context. */
	/* Reference to the statemachine definition.
	 *
	 * It should be readonly and contain the same values across jsparser
	 * instances.
	 */
	/* TODO(falmeida): Change statemachine_def to const. */
	statemachine_definition * statemachine_def;
	int buffer_start; /* Index to the start of the buffer. */
	int buffer_end; /* Index the current writing position (end of the buffer plus one). */
	char buffer[JSPARSER_RING_BUFFER_SIZE]; /* Ring buffer used to lookup the last token. */
} jsparser_ctx;

void jsparser_reset(jsparser_ctx * ctx);
jsparser_ctx * jsparser_new(void);

/* Returns a pointer to a context which is a duplicate of the jsparser src.
 */
jsparser_ctx * jsparser_duplicate(jsparser_ctx * src);

/* Copies the context of the jsparser pointed to by src to the jsparser dst.
 */
void jsparser_copy(jsparser_ctx * dst, jsparser_ctx * src);
int jsparser_state(jsparser_ctx * ctx);
int jsparser_parse(jsparser_ctx * ctx, const char * str, int size);

void jsparser_delete(jsparser_ctx * ctx);

/**
 * Ring buffer functions.
 *
 * These functions are only exported for testing and should not be called from
 * outside of jsparser.c in production code.
 */

/* Appends a character to the ring buffer.
 *
 * Sequences of whitespaces and newlines are folded into one character.
 */
void jsparser_buffer_append_chr(jsparser_ctx * js, char chr);

/* Appends a string to the ring buffer.
 *
 * Sequences of whitespaces and newlines are folded into one character.
 */
void jsparser_buffer_append_str(jsparser_ctx * js, const char * str);

/* Returns the last appended character and removes it from the buffer. If the
 * buffer is empty, then it returns ASCII 0 ('\0').
 */
char jsparser_buffer_pop(jsparser_ctx * js);

/* Returns the value of the character at a certain index in the buffer or an
 * ASCII 0 ('\0') character if the index is extends beyond the size of the
 * buffer, either because we don't have as many characters in the buffer, or
 * because the index points to a place bigger than the size of the buffer..
 *
 * Index positions must be negative, where -1 is the last character appended to
 * the buffer.
 */
char jsparser_buffer_get(jsparser_ctx * js, int pos);

/* Sets the value of the character at a certain index in the buffer. Returns
 * true if the write was successful or false if there was an attempt to write
 * outside of the buffer boundaries.
 *
 * Index positions are negative, were -1 is the last character appended to the
 * buffer. Using positive integers for the index will result in undefined
 * behaviour.
 */
int jsparser_buffer_set(jsparser_ctx * js, int pos, char value);

/* Copies a slice of the buffer to the string pointed to by output. start and
 * end are the indexes of the sliced region. If the start argument extends
 * beyond the beginning of the buffer, the slice will only contain characters
 * starting from beginning of the buffer.
 */
void jsparser_buffer_slice(jsparser_ctx * js, char * buffer, int start, int end);

/* Copy the last javascript identifier or keyword found in the buffer to the
 * string pointed by identifier.
 */
int jsparser_buffer_last_identifier(jsparser_ctx * js, char * identifier);

#define jsparser_parse_chr(a, b) jsparser_parse(a, &(b), 1);
#ifdef __cplusplus
#define jsparser_parse_str(a, b) jsparser_parse(a, b, \
	    static_cast<int>(strlen(b)));
#else
#define jsparser_parse_str(a, b) jsparser_parse(a, b, (int)strlen(b));
#endif

#ifdef __cplusplus
}  /* namespace security_streamhtmlparser */
#endif /* __cplusplus */

#endif /* SECURITY_STREAMHTMLPARSER_JSPARSER_H */
