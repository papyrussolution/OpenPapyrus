// GUMBO-LIB.C
// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: jdtang@google.com (Jonathan Tang)

#include <slib-internal.h>
#pragma hdrstop
#include "gumbo-internal.h"

// Size chosen via statistical analysis of ~60K websites. 99% of text nodes and 98% of attribute names/values fit in this initial size.
static const size_t kDefaultStringBufferSize = 5;
const GumboStringPiece kGumboEmptyString = { NULL, 0 };
const GumboVector kGumboEmptyVector = {NULL, 0, 0};
const int kUtf8ReplacementChar = 0xFFFD;
//
// utf8
//
// Reference material:
// Wikipedia: http://en.wikipedia.org/wiki/UTF-8#Description
// RFC 3629: http://tools.ietf.org/html/rfc3629
// HTML5 Unicode handling:
// http://www.whatwg.org/specs/web-apps/current-work/multipage/syntax.html#preprocessing-the-input-stream
//
// This implementation is based on a DFA-based decoder by Bjoern Hoehrmann
// <bjoern@hoehrmann.de>.  We wrap the inner table-based decoder routine in our
// own handling for newlines, tabs, invalid continuation bytes, and other
// conditions that the HTML5 spec fully specifies but normal UTF8 decoders do
// not handle.
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.  Full text of
// the license agreement and code follows.

// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to
// do
// so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

static const uint8_t utf8d[] = {
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 10,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 8, 8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 12, 12, 12, 12, 0, 12, 12, 12, 12, 12, 0, 12, 0, 12, 12, 12, 24, 12,
	12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12,
	12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12,
	12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12,
	12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
};

uint32_t static inline decode(uint32_t* state, uint32_t* codep, uint32_t byte) 
{
	uint32_t type = utf8d[byte];
	*codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);
	*state = utf8d[256 + *state + type];
	return *state;
}

// END COPIED CODE.

// Adds a decoding error to the parser's error list, based on the current state
// of the Utf8Iterator.
static void add_error(Utf8Iterator* iter, GumboErrorType type) 
{
	GumboParser * parser = iter->_parser;
	GumboError * error = gumbo_add_error(parser);
	if(error) {
		error->type = type;
		error->position = iter->_pos;
		error->original_text = iter->_start;
		// At the point the error is recorded, the code point hasn't been computed
		// yet (and can't be, because it's invalid), so we need to build up the raw
		// hex value from the bytes under the cursor.
		uint64_t code_point = 0;
		for(int i = 0; i < iter->_width; ++i) {
			code_point = (code_point << 8) | (uchar)iter->_start[i];
		}
		error->v.codepoint = code_point;
	}
}
//
// Reads the next UTF-8 character in the iter.
// This assumes that iter->_start points to the beginning of the character.
// When this method returns, iter->_width and iter->_current will be set
// appropriately, as well as any error flags.
//
static void FASTCALL read_char(Utf8Iterator * iter) 
{
	if(iter->_start >= iter->_end) {
		// No input left to consume; emit an EOF and set width = 0.
		iter->_current = -1;
		iter->_width = 0;
		return;
	}
	uint32_t code_point = 0;
	uint32_t state = UTF8_ACCEPT;
	for(const char * c = iter->_start; c < iter->_end; ++c) {
		decode(&state, &code_point, (uint32_t)(uchar)(*c));
		if(state == UTF8_ACCEPT) {
			iter->_width = c - iter->_start + 1;
			// This is the special handling for carriage returns that is mandated by
			// the HTML5 spec.  Since we're looking for particular 7-bit literal
			// characters, we operate in terms of chars and only need a check for iter
			// overrun, instead of having to read in a full next code point.
			// http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#preprocessing-the-input-stream
			if(code_point == '\r') {
				assert(iter->_width == 1);
				const char* next = c + 1;
				if(next < iter->_end && *next == '\n') {
					// Advance the iter, as if the carriage return didn't exist.
					++iter->_start;
					// Preserve the true offset, since other tools that look at it may be
					// unaware of HTML5's rules for converting \r into \n.
					++iter->_pos.offset;
				}
				code_point = '\n';
			}
			if(utf8_is_invalid_code_point(code_point)) {
				add_error(iter, GUMBO_ERR_UTF8_INVALID);
				code_point = kUtf8ReplacementChar;
			}
			iter->_current = code_point;
			return;
		}
		else if(state == UTF8_REJECT) {
			// We don't want to consume the invalid continuation byte of a multi-byte
			// run, but we do want to skip past an invalid first byte.
			iter->_width = c - iter->_start + (c == iter->_start);
			iter->_current = kUtf8ReplacementChar;
			add_error(iter, GUMBO_ERR_UTF8_INVALID);
			return;
		}
	}
	// If we got here without exiting early, then we've reached the end of the
	// iterator.  Add an error for truncated input, set the width to consume the
	// rest of the iterator, and emit a replacement character.  The next time we
	// enter this method, it will detect that there's no input to consume and
	// output an EOF.
	iter->_current = kUtf8ReplacementChar;
	iter->_width = iter->_end - iter->_start;
	add_error(iter, GUMBO_ERR_UTF8_TRUNCATED);
}

static void FASTCALL update_position(Utf8Iterator * iter) 
{
	iter->_pos.offset += iter->_width;
	if(iter->_current == '\n') {
		++iter->_pos.line;
		iter->_pos.column = 1;
	}
	else if(iter->_current == '\t') {
		int tab_stop = iter->_parser->_options->tab_stop;
		iter->_pos.column = ((iter->_pos.column / tab_stop) + 1) * tab_stop;
	}
	else if(iter->_current != -1) {
		++iter->_pos.column;
	}
}

// Returns true if this Unicode code point is in the list of characters
// forbidden by the HTML5 spec, such as undefined control chars.
bool FASTCALL utf8_is_invalid_code_point(int c) 
{
	return (c >= 0x1 && c <= 0x8) || c == 0xB || (c >= 0xE && c <= 0x1F) ||
	       (c >= 0x7F && c <= 0x9F) || (c >= 0xFDD0 && c <= 0xFDEF) ||
	       ((c & 0xFFFF) == 0xFFFE) || ((c & 0xFFFF) == 0xFFFF);
}

void utf8iterator_init(GumboParser * parser, const char* source, size_t source_length, Utf8Iterator* iter) 
{
	iter->_start = source;
	iter->_end = source + source_length;
	iter->_pos.line = 1;
	iter->_pos.column = 1;
	iter->_pos.offset = 0;
	iter->_parser = parser;
	read_char(iter);
}

void FASTCALL utf8iterator_next(Utf8Iterator * iter) 
{
	// We update positions based on the *last* character read, so that the first
	// character following a newline is at column 1 in the next line.
	update_position(iter);
	iter->_start += iter->_width;
	read_char(iter);
}

int FASTCALL utf8iterator_current(const Utf8Iterator* iter) { return iter->_current; }
void utf8iterator_get_position(const Utf8Iterator* iter, GumboSourcePosition* output) { *output = iter->_pos; }
const char* utf8iterator_get_char_pointer(const Utf8Iterator* iter) { return iter->_start; }
const char* utf8iterator_get_end_pointer(const Utf8Iterator* iter) { return iter->_end; }

bool utf8iterator_maybe_consume_match(Utf8Iterator* iter, const char* prefix, size_t length, bool case_sensitive) 
{
	bool matched = (iter->_start + length <= iter->_end) && (case_sensitive ? !strncmp(iter->_start, prefix, length) : !strncasecmp(iter->_start, prefix, length));
	if(matched) {
		for(uint i = 0; i < length; ++i) {
			utf8iterator_next(iter);
		}
		return true;
	}
	else
		return false;
}

void utf8iterator_mark(Utf8Iterator* iter) 
{
	iter->_mark = iter->_start;
	iter->_mark_pos = iter->_pos;
}

// Returns the current input stream position to the mark.
void FASTCALL utf8iterator_reset(Utf8Iterator* iter) 
{
	iter->_start = iter->_mark;
	iter->_pos = iter->_mark_pos;
	read_char(iter);
}

// Sets the position and original text fields of an error to the value at the
// mark.
void utf8iterator_fill_error_at_mark(Utf8Iterator* iter, GumboError* error) 
{
	error->position = iter->_mark_pos;
	error->original_text = iter->_mark;
}
//
// string_buffer
//
static void FASTCALL maybe_resize_string_buffer(size_t additional_chars, GumboStringBuffer* buffer) 
{
	size_t new_length = buffer->length + additional_chars;
	size_t new_capacity = buffer->capacity;
	while(new_capacity < new_length) {
		new_capacity *= 2;
	}
	if(new_capacity != buffer->capacity) {
		char * new_data = static_cast<char *>(SAlloc::M(new_capacity));
		memcpy(new_data, buffer->data, buffer->length);
		SAlloc::F(buffer->data);
		buffer->data = new_data;
		buffer->capacity = new_capacity;
	}
}

void FASTCALL gumbo_string_buffer_init(GumboStringBuffer * output) 
{
	output->data = static_cast<char *>(SAlloc::M(kDefaultStringBufferSize));
	output->length = 0;
	output->capacity = kDefaultStringBufferSize;
}

void gumbo_string_buffer_reserve(size_t min_capacity, GumboStringBuffer * output) 
{
	maybe_resize_string_buffer(min_capacity - output->length, output);
}

void FASTCALL gumbo_string_buffer_append_codepoint(int c, GumboStringBuffer * output) 
{
	// num_bytes is actually the number of continuation bytes, 1 less than the
	// total number of bytes.  This is done to keep the loop below simple and
	// should probably change if we unroll it.
	int num_bytes, prefix;
	if(c <= 0x7f) {
		num_bytes = 0;
		prefix = 0;
	}
	else if(c <= 0x7ff) {
		num_bytes = 1;
		prefix = 0xc0;
	}
	else if(c <= 0xffff) {
		num_bytes = 2;
		prefix = 0xe0;
	}
	else {
		num_bytes = 3;
		prefix = 0xf0;
	}
	maybe_resize_string_buffer(num_bytes + 1, output);
	output->data[output->length++] = prefix | (c >> (num_bytes * 6));
	for(int i = num_bytes - 1; i >= 0; --i) {
		output->data[output->length++] = 0x80 | (0x3f & (c >> (i * 6)));
	}
}

void gumbo_string_buffer_append_string(GumboStringPiece * str, GumboStringBuffer * output) 
{
	maybe_resize_string_buffer(str->length, output);
	memcpy(output->data + output->length, str->data, str->length);
	output->length += str->length;
}

char * FASTCALL gumbo_string_buffer_to_string(GumboStringBuffer * input) 
{
	char * buffer = static_cast<char *>(SAlloc::M(input->length + 1));
	memcpy(buffer, input->data, input->length);
	buffer[input->length] = '\0';
	return buffer;
}

void FASTCALL gumbo_string_buffer_clear(GumboStringBuffer * input) 
{
	input->length = 0;
}

void FASTCALL gumbo_string_buffer_destroy(GumboStringBuffer* buffer) 
{
	SAlloc::F(buffer->data);
}
//
// string_piece
//
bool gumbo_string_equals(const GumboStringPiece * str1, const GumboStringPiece * str2) 
{
	return str1->length == str2->length && !memcmp(str1->data, str2->data, str1->length);
}

bool gumbo_string_equals_ignore_case(const GumboStringPiece * str1, const GumboStringPiece * str2) 
{
	return str1->length == str2->length && !strncasecmp(str1->data, str2->data, str1->length);
}

void gumbo_string_copy(GumboStringPiece* dest, const GumboStringPiece* source) 
{
	dest->length = source->length;
	char * buffer = static_cast<char *>(SAlloc::M(source->length));
	memcpy(buffer, source->data, source->length);
	dest->data = buffer;
}
//
// vector
//
void FASTCALL gumbo_vector_init(size_t initial_capacity, GumboVector* vector) 
{
	vector->length = 0;
	vector->capacity = initial_capacity;
	if(initial_capacity > 0)
		vector->data = static_cast<void **>(SAlloc::M(sizeof(void*) * initial_capacity));
	else
		vector->data = NULL;
}

void FASTCALL gumbo_vector_destroy(GumboVector* vector) 
{
	if(vector->capacity > 0)
		SAlloc::F(vector->data);
}

static void FASTCALL enlarge_vector_if_full(GumboVector* vector) 
{
	if(vector->length >= vector->capacity) {
		if(vector->capacity) {
			size_t old_num_bytes = sizeof(void*) * vector->capacity;
			vector->capacity *= 2;
			size_t num_bytes = sizeof(void*) * vector->capacity;
			void ** temp = static_cast<void **>(SAlloc::M(num_bytes));
			memcpy(temp, vector->data, old_num_bytes);
			SAlloc::F(vector->data);
			vector->data = temp;
		}
		else {
			// 0-capacity vector; no previous array to deallocate.
			vector->capacity = 2;
			vector->data = static_cast<void **>(SAlloc::M(sizeof(void*) * vector->capacity));
		}
	}
}

void FASTCALL gumbo_vector_add(void * element, GumboVector * vector) 
{
	enlarge_vector_if_full(vector);
	assert(vector->data);
	assert(vector->length < vector->capacity);
	vector->data[vector->length++] = element;
}

void * gumbo_vector_pop(GumboParser * parser, GumboVector* vector) 
{
	return vector->length ? vector->data[--vector->length] : 0;
}

int gumbo_vector_index_of(GumboVector* vector, const void * element) 
{
	for(uint i = 0; i < vector->length; ++i) {
		if(vector->data[i] == element)
			return i;
	}
	return -1;
}

void gumbo_vector_insert_at(void * element, uint index, GumboVector * vector) 
{
	assert(index >= 0);
	assert(index <= vector->length);
	enlarge_vector_if_full(vector);
	++vector->length;
	memmove(&vector->data[index + 1], &vector->data[index], sizeof(void*) * (vector->length - index - 1));
	vector->data[index] = element;
}

void FASTCALL gumbo_vector_remove(void * node, GumboVector * vector) 
{
	const int index = gumbo_vector_index_of(vector, node);
	if(index != -1)
		gumbo_vector_remove_at(index, vector);
}

void * FASTCALL gumbo_vector_remove_at(uint index, GumboVector * vector) 
{
	assert(index >= 0);
	assert(index < vector->length);
	void* result = vector->data[index];
	memmove(&vector->data[index], &vector->data[index + 1], sizeof(void*) * (vector->length - index - 1));
	--vector->length;
	return result;
}
//
// error
//
// Prints a formatted message to a StringBuffer.  This automatically resizes the
// StringBuffer as necessary to fit the message.  Returns the number of bytes written.
static int print_message(GumboParser * parser, GumboStringBuffer* output, const char* format, ...) 
{
	va_list args;
	int remaining_capacity = output->capacity - output->length;
	va_start(args, format);
	int bytes_written = _vsnprintf(output->data + output->length, remaining_capacity, format, args);
	va_end(args);
#ifdef _MSC_VER
	if(bytes_written == -1) {
		// vsnprintf returns -1 on MSVC++ if there's not enough capacity, instead of
		// returning the number of bytes that would've been written had there been
		// enough.  In this case, we'll double the buffer size and hope it fits when
		// we retry (letting it fail and returning 0 if it doesn't), since there's
		// no way to smartly resize the buffer.
		gumbo_string_buffer_reserve(output->capacity * 2, output);
		va_start(args, format);
		int result = _vsnprintf(output->data + output->length, remaining_capacity, format, args);
		va_end(args);
		return result == -1 ? 0 : result;
	}
#else
	// -1 in standard C99 indicates an encoding error.  Return 0 and do nothing.
	if(bytes_written == -1) {
		return 0;
	}
#endif
	if(bytes_written >= remaining_capacity) {
		gumbo_string_buffer_reserve(output->capacity + bytes_written, output);
		remaining_capacity = output->capacity - output->length;
		va_start(args, format);
		bytes_written = _vsnprintf(output->data + output->length, remaining_capacity, format, args);
		va_end(args);
	}
	output->length += bytes_written;
	return bytes_written;
}

static void print_tag_stack(GumboParser * parser, const GumboParserError* error, GumboStringBuffer* output) 
{
	print_message(parser, output, "  Currently open tags: ");
	for(uint i = 0; i < error->tag_stack.length; ++i) {
		if(i) {
			print_message(parser, output, ", ");
		}
		GumboTag tag = (GumboTag)(reinterpret_cast<long>(error->tag_stack.data[i]));
		print_message(parser, output, gumbo_normalized_tagname(tag));
	}
	gumbo_string_buffer_append_codepoint('.', output);
}

static void handle_parser_error(GumboParser * parser, const GumboParserError* error, GumboStringBuffer* output) 
{
	if(error->parser_state == GUMBO_INSERTION_MODE_INITIAL && error->input_type != GUMBO_TOKEN_DOCTYPE) {
		print_message(parser, output, "The doctype must be the first token in the document");
		return;
	}
	switch(error->input_type) {
		case GUMBO_TOKEN_DOCTYPE:
		    print_message(parser, output, "This is not a legal doctype");
		    return;
		case GUMBO_TOKEN_COMMENT:
		    // Should never happen; comments are always legal.
		    assert(0);
		    // But just in case...
		    print_message(parser, output, "Comments aren't legal here");
		    return;
		case GUMBO_TOKEN_CDATA:
		case GUMBO_TOKEN_WHITESPACE:
		case GUMBO_TOKEN_CHARACTER:
		    print_message(parser, output, "Character tokens aren't legal here");
		    return;
		case GUMBO_TOKEN_NULL:
		    print_message(parser, output, "Null bytes are not allowed in HTML5");
		    return;
		case GUMBO_TOKEN_EOF:
		    if(error->parser_state == GUMBO_INSERTION_MODE_INITIAL) {
			    print_message(parser, output, "You must provide a doctype");
		    }
		    else {
			    print_message(parser, output, "Premature end of file");
			    print_tag_stack(parser, error, output);
		    }
		    return;
		case GUMBO_TOKEN_START_TAG:
		case GUMBO_TOKEN_END_TAG:
		    print_message(parser, output, "That tag isn't allowed here");
		    print_tag_stack(parser, error, output);
		    // TODO(jdtang): Give more specific messaging.
		    return;
	}
}

// Finds the preceding newline in an original source buffer from a given byte
// location.  Returns a character pointer to the character after that, or a
// pointer to the beginning of the string if this is the first line.
static const char* find_last_newline(const char* original_text, const char* error_location) 
{
	assert(error_location >= original_text);
	const char* c = error_location;
	for(; c != original_text && *c != '\n'; --c) {
		// There may be an error at EOF, which would be a nul byte.
		assert(*c || c == error_location);
	}
	return c == original_text ? c : c + 1;
}
//
// Finds the next newline in the original source buffer from a given byte
// location.  Returns a character pointer to that newline, or a pointer to the
// terminating null byte if this is the last line.
//
static const char * find_next_newline(const char* original_text, const char* error_location) 
{
	const char * c = error_location;
	for(; *c && *c != '\n'; ++c)
		;
	return c;
}

GumboError * gumbo_add_error(GumboParser * parser) 
{
	GumboError * error = 0;
	const int max_errors = parser->_options->max_errors;
	if(max_errors < 0 || parser->_output->errors.length < (uint)max_errors) {
		error = static_cast<GumboError *>(SAlloc::M(sizeof(GumboError)));
		gumbo_vector_add(error, &parser->_output->errors);
	}
	return error;
}

void gumbo_error_to_string(GumboParser * parser, const GumboError* error, GumboStringBuffer* output) 
{
	print_message(parser, output, "@%d:%d: ", error->position.line, error->position.column);
	switch(error->type) {
		case GUMBO_ERR_UTF8_INVALID:
		    print_message(parser, output, "Invalid UTF8 character 0x%x", error->v.codepoint);
		    break;
		case GUMBO_ERR_UTF8_TRUNCATED:
		    print_message(parser, output, "Input stream ends with a truncated UTF8 character 0x%x", error->v.codepoint);
		    break;
		case GUMBO_ERR_NUMERIC_CHAR_REF_NO_DIGITS:
		    print_message(parser, output, "No digits after &# in numeric character reference");
		    break;
		case GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON:
		    print_message(parser, output, "The numeric character reference &#%d should be followed by a semicolon",
			error->v.codepoint);
		    break;
		case GUMBO_ERR_NUMERIC_CHAR_REF_INVALID:
		    print_message(parser, output, "The numeric character reference &#%d; encodes an invalid unicode codepoint",
			error->v.codepoint);
		    break;
		case GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON:
		    // The textual data came from one of the literal strings in the table, and
		    // so it'll be null-terminated.
		    print_message(parser, output, "The named character reference &%.*s should be followed by a semicolon",
				(int)error->v.text.length, error->v.text.data);
		    break;
		case GUMBO_ERR_NAMED_CHAR_REF_INVALID:
		    print_message(parser, output, "The named character reference &%.*s; is not a valid entity name", (int)error->v.text.length, error->v.text.data);
		    break;
		case GUMBO_ERR_DUPLICATE_ATTR:
		    print_message(parser, output, "Attribute %s occurs multiple times, at positions %d and %d",
				error->v.duplicate_attr.name, error->v.duplicate_attr.original_index, error->v.duplicate_attr.new_index);
		    break;
		case GUMBO_ERR_PARSER:
		case GUMBO_ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG:
		    handle_parser_error(parser, &error->v.parser, output);
		    break;
		default:
		    print_message(parser, output, "Tokenizer error with an unimplemented error message");
		    break;
	}
	gumbo_string_buffer_append_codepoint('.', output);
}

void gumbo_caret_diagnostic_to_string(GumboParser * parser, const GumboError* error, const char * source_text, GumboStringBuffer* output) 
{
	gumbo_error_to_string(parser, error, output);
	const char * line_start = find_last_newline(source_text, error->original_text);
	const char * line_end = find_next_newline(source_text, error->original_text);
	GumboStringPiece original_line;
	original_line.data = line_start;
	original_line.length = line_end - line_start;
	gumbo_string_buffer_append_codepoint('\n', output);
	gumbo_string_buffer_append_string(&original_line, output);
	gumbo_string_buffer_append_codepoint('\n', output);
	gumbo_string_buffer_reserve(output->length + error->position.column, output);
	int num_spaces = error->position.column - 1;
	memset(output->data + output->length, ' ', num_spaces);
	output->length += num_spaces;
	gumbo_string_buffer_append_codepoint('^', output);
	gumbo_string_buffer_append_codepoint('\n', output);
}

void gumbo_print_caret_diagnostic(GumboParser * parser, const GumboError* error, const char* source_text) 
{
	GumboStringBuffer text;
	gumbo_string_buffer_init(&text);
	gumbo_caret_diagnostic_to_string(parser, error, source_text, &text);
	printf("%.*s", (int)text.length, text.data);
	gumbo_string_buffer_destroy(&text);
}

void FASTCALL gumbo_error_destroy(GumboError* error) 
{
	if(oneof2(error->type, GUMBO_ERR_PARSER, GUMBO_ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG)) {
		gumbo_vector_destroy(&error->v.parser.tag_stack);
	}
	else if(error->type == GUMBO_ERR_DUPLICATE_ATTR) {
		SAlloc::F((void*)error->v.duplicate_attr.name);
	}
	SAlloc::F(error);
}

void gumbo_init_errors(GumboParser * parser) 
{
	gumbo_vector_init(5, &parser->_output->errors);
}

void gumbo_destroy_errors(GumboParser * parser) 
{
	for(uint i = 0; i < parser->_output->errors.length; ++i)
		gumbo_error_destroy(static_cast<GumboError *>(parser->_output->errors.data[i]));
	gumbo_vector_destroy(&parser->_output->errors);
}
