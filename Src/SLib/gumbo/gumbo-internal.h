// GUMBO-INTERNAL.H
// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// Author: jdtang@google.com (Jonathan Tang)
// Contains the definition of the top-level GumboParser structure that's
// threaded through basically every internal function in the library.
//
#ifndef __GUMBO_INTERNAL_H
#define __GUMBO_INTERNAL_H

struct GumboParserState;
struct GumboOutput;
struct GumboOptions;
struct GumboTokenizerState;
struct GumboParser;
struct GumboInternalUtf8Iterator;
struct GumboInternalError;

#include "gumbo.h"
//
// http://www.whatwg.org/specs/web-apps/current-work/complete/parsing.html#insertion-mode
// If new enum values are added, be sure to update the kTokenHandlers dispatch
// table in parser.c.
//
enum GumboInsertionMode {
	GUMBO_INSERTION_MODE_INITIAL,
	GUMBO_INSERTION_MODE_BEFORE_HTML,
	GUMBO_INSERTION_MODE_BEFORE_HEAD,
	GUMBO_INSERTION_MODE_IN_HEAD,
	GUMBO_INSERTION_MODE_IN_HEAD_NOSCRIPT,
	GUMBO_INSERTION_MODE_AFTER_HEAD,
	GUMBO_INSERTION_MODE_IN_BODY,
	GUMBO_INSERTION_MODE_TEXT,
	GUMBO_INSERTION_MODE_IN_TABLE,
	GUMBO_INSERTION_MODE_IN_TABLE_TEXT,
	GUMBO_INSERTION_MODE_IN_CAPTION,
	GUMBO_INSERTION_MODE_IN_COLUMN_GROUP,
	GUMBO_INSERTION_MODE_IN_TABLE_BODY,
	GUMBO_INSERTION_MODE_IN_ROW,
	GUMBO_INSERTION_MODE_IN_CELL,
	GUMBO_INSERTION_MODE_IN_SELECT,
	GUMBO_INSERTION_MODE_IN_SELECT_IN_TABLE,
	GUMBO_INSERTION_MODE_IN_TEMPLATE,
	GUMBO_INSERTION_MODE_AFTER_BODY,
	GUMBO_INSERTION_MODE_IN_FRAMESET,
	GUMBO_INSERTION_MODE_AFTER_FRAMESET,
	GUMBO_INSERTION_MODE_AFTER_AFTER_BODY,
	GUMBO_INSERTION_MODE_AFTER_AFTER_FRAMESET
};
//
// Descr: An enum representing the type of token.
//
enum GumboTokenType {
	GUMBO_TOKEN_DOCTYPE,
	GUMBO_TOKEN_START_TAG,
	GUMBO_TOKEN_END_TAG,
	GUMBO_TOKEN_COMMENT,
	GUMBO_TOKEN_WHITESPACE,
	GUMBO_TOKEN_CHARACTER,
	GUMBO_TOKEN_CDATA,
	GUMBO_TOKEN_NULL,
	GUMBO_TOKEN_EOF
};
//
// The ordering of this enum is also used to build the dispatch table for the
// tokenizer state machine, so if it is changed, be sure to update that too.
//
enum GumboTokenizerEnum {
	GUMBO_LEX_DATA,
	GUMBO_LEX_CHAR_REF_IN_DATA,
	GUMBO_LEX_RCDATA,
	GUMBO_LEX_CHAR_REF_IN_RCDATA,
	GUMBO_LEX_RAWTEXT,
	GUMBO_LEX_SCRIPT,
	GUMBO_LEX_PLAINTEXT,
	GUMBO_LEX_TAG_OPEN,
	GUMBO_LEX_END_TAG_OPEN,
	GUMBO_LEX_TAG_NAME,
	GUMBO_LEX_RCDATA_LT,
	GUMBO_LEX_RCDATA_END_TAG_OPEN,
	GUMBO_LEX_RCDATA_END_TAG_NAME,
	GUMBO_LEX_RAWTEXT_LT,
	GUMBO_LEX_RAWTEXT_END_TAG_OPEN,
	GUMBO_LEX_RAWTEXT_END_TAG_NAME,
	GUMBO_LEX_SCRIPT_LT,
	GUMBO_LEX_SCRIPT_END_TAG_OPEN,
	GUMBO_LEX_SCRIPT_END_TAG_NAME,
	GUMBO_LEX_SCRIPT_ESCAPED_START,
	GUMBO_LEX_SCRIPT_ESCAPED_START_DASH,
	GUMBO_LEX_SCRIPT_ESCAPED,
	GUMBO_LEX_SCRIPT_ESCAPED_DASH,
	GUMBO_LEX_SCRIPT_ESCAPED_DASH_DASH,
	GUMBO_LEX_SCRIPT_ESCAPED_LT,
	GUMBO_LEX_SCRIPT_ESCAPED_END_TAG_OPEN,
	GUMBO_LEX_SCRIPT_ESCAPED_END_TAG_NAME,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED_START,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED_DASH,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED_DASH_DASH,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED_LT,
	GUMBO_LEX_SCRIPT_DOUBLE_ESCAPED_END,
	GUMBO_LEX_BEFORE_ATTR_NAME,
	GUMBO_LEX_ATTR_NAME,
	GUMBO_LEX_AFTER_ATTR_NAME,
	GUMBO_LEX_BEFORE_ATTR_VALUE,
	GUMBO_LEX_ATTR_VALUE_DOUBLE_QUOTED,
	GUMBO_LEX_ATTR_VALUE_SINGLE_QUOTED,
	GUMBO_LEX_ATTR_VALUE_UNQUOTED,
	GUMBO_LEX_CHAR_REF_IN_ATTR_VALUE,
	GUMBO_LEX_AFTER_ATTR_VALUE_QUOTED,
	GUMBO_LEX_SELF_CLOSING_START_TAG,
	GUMBO_LEX_BOGUS_COMMENT,
	GUMBO_LEX_MARKUP_DECLARATION,
	GUMBO_LEX_COMMENT_START,
	GUMBO_LEX_COMMENT_START_DASH,
	GUMBO_LEX_COMMENT,
	GUMBO_LEX_COMMENT_END_DASH,
	GUMBO_LEX_COMMENT_END,
	GUMBO_LEX_COMMENT_END_BANG,
	GUMBO_LEX_DOCTYPE,
	GUMBO_LEX_BEFORE_DOCTYPE_NAME,
	GUMBO_LEX_DOCTYPE_NAME,
	GUMBO_LEX_AFTER_DOCTYPE_NAME,
	GUMBO_LEX_AFTER_DOCTYPE_PUBLIC_KEYWORD,
	GUMBO_LEX_BEFORE_DOCTYPE_PUBLIC_ID,
	GUMBO_LEX_DOCTYPE_PUBLIC_ID_DOUBLE_QUOTED,
	GUMBO_LEX_DOCTYPE_PUBLIC_ID_SINGLE_QUOTED,
	GUMBO_LEX_AFTER_DOCTYPE_PUBLIC_ID,
	GUMBO_LEX_BETWEEN_DOCTYPE_PUBLIC_SYSTEM_ID,
	GUMBO_LEX_AFTER_DOCTYPE_SYSTEM_KEYWORD,
	GUMBO_LEX_BEFORE_DOCTYPE_SYSTEM_ID,
	GUMBO_LEX_DOCTYPE_SYSTEM_ID_DOUBLE_QUOTED,
	GUMBO_LEX_DOCTYPE_SYSTEM_ID_SINGLE_QUOTED,
	GUMBO_LEX_AFTER_DOCTYPE_SYSTEM_ID,
	GUMBO_LEX_BOGUS_DOCTYPE,
	GUMBO_LEX_CDATA
};
//
//
//
enum GumboErrorType {
	GUMBO_ERR_UTF8_INVALID,
	GUMBO_ERR_UTF8_TRUNCATED,
	GUMBO_ERR_UTF8_NULL,
	GUMBO_ERR_NUMERIC_CHAR_REF_NO_DIGITS,
	GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON,
	GUMBO_ERR_NUMERIC_CHAR_REF_INVALID,
	GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON,
	GUMBO_ERR_NAMED_CHAR_REF_INVALID,
	GUMBO_ERR_TAG_STARTS_WITH_QUESTION,
	GUMBO_ERR_TAG_EOF,
	GUMBO_ERR_TAG_INVALID,
	GUMBO_ERR_CLOSE_TAG_EMPTY,
	GUMBO_ERR_CLOSE_TAG_EOF,
	GUMBO_ERR_CLOSE_TAG_INVALID,
	GUMBO_ERR_SCRIPT_EOF,
	GUMBO_ERR_ATTR_NAME_EOF,
	GUMBO_ERR_ATTR_NAME_INVALID,
	GUMBO_ERR_ATTR_DOUBLE_QUOTE_EOF,
	GUMBO_ERR_ATTR_SINGLE_QUOTE_EOF,
	GUMBO_ERR_ATTR_UNQUOTED_EOF,
	GUMBO_ERR_ATTR_UNQUOTED_RIGHT_BRACKET,
	GUMBO_ERR_ATTR_UNQUOTED_EQUALS,
	GUMBO_ERR_ATTR_AFTER_EOF,
	GUMBO_ERR_ATTR_AFTER_INVALID,
	GUMBO_ERR_DUPLICATE_ATTR,
	GUMBO_ERR_SOLIDUS_EOF,
	GUMBO_ERR_SOLIDUS_INVALID,
	GUMBO_ERR_DASHES_OR_DOCTYPE,
	GUMBO_ERR_COMMENT_EOF,
	GUMBO_ERR_COMMENT_INVALID,
	GUMBO_ERR_COMMENT_BANG_AFTER_DOUBLE_DASH,
	GUMBO_ERR_COMMENT_DASH_AFTER_DOUBLE_DASH,
	GUMBO_ERR_COMMENT_SPACE_AFTER_DOUBLE_DASH,
	GUMBO_ERR_COMMENT_END_BANG_EOF,
	GUMBO_ERR_DOCTYPE_EOF,
	GUMBO_ERR_DOCTYPE_INVALID,
	GUMBO_ERR_DOCTYPE_SPACE,
	GUMBO_ERR_DOCTYPE_RIGHT_BRACKET,
	GUMBO_ERR_DOCTYPE_SPACE_OR_RIGHT_BRACKET,
	GUMBO_ERR_DOCTYPE_END,
	GUMBO_ERR_PARSER,
	GUMBO_ERR_UNACKNOWLEDGED_SELF_CLOSING_TAG,
};
//
// A simplified representation of the tokenizer state, designed to be more
// useful to clients of this library than the internal representation.  This
// condenses the actual states used in the tokenizer state machine into a few
// values that will be familiar to users of HTML.
//
enum GumboTokenizerErrorState {
	GUMBO_ERR_TOKENIZER_DATA,
	GUMBO_ERR_TOKENIZER_CHAR_REF,
	GUMBO_ERR_TOKENIZER_RCDATA,
	GUMBO_ERR_TOKENIZER_RAWTEXT,
	GUMBO_ERR_TOKENIZER_PLAINTEXT,
	GUMBO_ERR_TOKENIZER_SCRIPT,
	GUMBO_ERR_TOKENIZER_TAG,
	GUMBO_ERR_TOKENIZER_SELF_CLOSING_TAG,
	GUMBO_ERR_TOKENIZER_ATTR_NAME,
	GUMBO_ERR_TOKENIZER_ATTR_VALUE,
	GUMBO_ERR_TOKENIZER_MARKUP_DECLARATION,
	GUMBO_ERR_TOKENIZER_COMMENT,
	GUMBO_ERR_TOKENIZER_DOCTYPE,
	GUMBO_ERR_TOKENIZER_CDATA,
};

#ifdef __cplusplus
extern "C" {
#endif
//
// This contains an implementation of a UTF8 iterator and decoder suitable for
// an HTML5 parser.  This does a bit more than straight UTF-8 decoding.  The
// HTML5 spec specifies that:
// 1. Decoding errors are parse errors.
// 2. Certain other codepoints (eg. control characters) are parse errors.
// 3. Carriage returns and CR/LF groups are converted to line feeds.
// http://www.whatwg.org/specs/web-apps/current-work/multipage/infrastructure.html#decoded-as-utf-8,-with-error-handling
//
// Also, we want to keep track of source positions for error handling.  As a
// result, we fold all that functionality into this decoder, and can't use an
// off-the-shelf library.
//
// This header is internal-only, which is why we prefix functions with only
// utf8_ or utf8_iterator_ instead of gumbo_utf8_.
//
extern const int kUtf8ReplacementChar; // Unicode replacement char.

typedef struct GumboInternalUtf8Iterator {
	const char * _start; // Points at the start of the code point most recently read into 'current'.
	const char * _mark; // Points at the mark.  The mark is initially set to the beginning of the  input.
	const char * _end; // Points past the end of the iter, like a past-the-end iterator in the STL.
	int _current; // The code point under the cursor.
	int _width; // The width in bytes of the current code point.
	GumboSourcePosition _pos; // The SourcePosition for the current location.
	GumboSourcePosition _mark_pos; // The SourcePosition for the mark.
	GumboParser * _parser; // Pointer back to the GumboParser instance, for configuration options and error recording.
} Utf8Iterator;

// Returns true if this Unicode code point is in the list of characters
// forbidden by the HTML5 spec, such as NUL bytes and undefined control chars.
bool FASTCALL utf8_is_invalid_code_point(int c);

// Initializes a new Utf8Iterator from the given byte buffer.  The source does
// not have to be NUL-terminated, but the length must be passed in explicitly.
void utf8iterator_init(GumboParser * parser, const char * source, size_t source_length, Utf8Iterator* iter);

// Advances the current position by one code point.
void FASTCALL utf8iterator_next(Utf8Iterator* iter);

// Returns the current code point as an integer.
int FASTCALL utf8iterator_current(const Utf8Iterator* iter);

// Retrieves and fills the output parameter with the current source position.
void FASTCALL utf8iterator_get_position(const Utf8Iterator* iter, GumboSourcePosition* output);

// Retrieves a character pointer to the start of the current character.
const char * FASTCALL utf8iterator_get_char_pointer(const Utf8Iterator* iter);

// Retrieves a character pointer to 1 past the end of the buffer.  This is
// necessary for certain state machines and string comparisons that would like
// to look directly for ASCII text in the buffer without going through the
// decoder.
const char * FASTCALL utf8iterator_get_end_pointer(const Utf8Iterator* iter);

// If the upcoming text in the buffer matches the specified prefix (which has
// length 'length'), consume it and return true.  Otherwise, return false with
// no other effects.  If the length of the string would overflow the buffer,
// this returns false.  Note that prefix should not contain null bytes because
// of the use of strncmp/strncasecmp internally.  All existing use-cases adhere
// to this.
bool utf8iterator_maybe_consume_match(Utf8Iterator* iter, const char * prefix, size_t length, bool case_sensitive);

// "Marks" a particular location of interest in the input stream, so that it can
// later be reset() to.  There's also the ability to record an error at the
// point that was marked, as oftentimes that's more useful than the last
// character before the error was detected.
void utf8iterator_mark(Utf8Iterator* iter);

// Returns the current input stream position to the mark.
void FASTCALL utf8iterator_reset(Utf8Iterator* iter);

// Sets the position and original text fields of an error to the value at the mark.
void utf8iterator_fill_error_at_mark(Utf8Iterator* iter, struct GumboInternalError* error);
//
//
//
extern const int kGumboNoChar; // Value that indicates no character was produced.

// Certain named character references generate two codepoints, not one, and so
// the consume_char_ref subroutine needs to return this instead of an int.  The
// first field will be kGumboNoChar if no character reference was found; the
// second field will be kGumboNoChar if that is the case or if the character
// reference returns only a single codepoint.
typedef struct {
	int first;
	int second;
} OneOrTwoCodepoints;

// Implements the "consume a character reference" section of the spec.
// This reads in characters from the input as necessary, and fills in a
// OneOrTwoCodepoints struct containing the characters read.  It may add parse
// errors to the GumboParser's errors vector, if the spec calls for it.  Pass a
// space for the "additional allowed char" when the spec says "with no
// additional allowed char".  Returns false on parse error, true otherwise.
bool consume_char_ref(GumboParser * parser, struct GumboInternalUtf8Iterator* input, int additional_allowed_char,
    bool is_in_attribute, OneOrTwoCodepoints* output);
//
// Release the memory used for an GumboAttribute, including the attribute itself.
//
void FASTCALL gumbo_destroy_attribute(GumboAttribute* attribute);
//
// A struct representing a mutable, growable string.  This consists of a
// heap-allocated buffer that may grow (by doubling) as necessary.  When
// converting to a string, this allocates a new buffer that is only as long as
// it needs to be.  Note that the internal buffer here is *not* nul-terminated,
// so be sure not to use ordinary string manipulation functions on it.
//
struct GumboStringBuffer {
	char * data; // A pointer to the beginning of the string.  NULL iff length == 0.
	size_t length; // The length of the string fragment, in bytes.  May be zero.
	size_t capacity; // The capacity of the buffer, in bytes.
};

// Initializes a new GumboStringBuffer.
void FASTCALL gumbo_string_buffer_init(GumboStringBuffer * output);

// Ensures that the buffer contains at least a certain amount of space.  Most
// useful with snprintf and the other length-delimited string functions, which
// may want to write directly into the buffer.
void gumbo_string_buffer_reserve(size_t min_capacity, GumboStringBuffer* output);

// Appends a single Unicode codepoint onto the end of the GumboStringBuffer.
// This is essentially a UTF-8 encoder, and may add 1-4 bytes depending on the
// value of the codepoint.
void FASTCALL gumbo_string_buffer_append_codepoint(int c, GumboStringBuffer* output);

// Appends a string onto the end of the GumboStringBuffer.
void gumbo_string_buffer_append_string(GumboStringPiece * str, GumboStringBuffer* output);

// Converts this string buffer to const char *, alloctaing a new buffer for it.
char * FASTCALL gumbo_string_buffer_to_string(GumboStringBuffer * input);

// Reinitialize this string buffer.  This clears it by setting length=0.  It
// does not zero out the buffer itself.
void FASTCALL gumbo_string_buffer_clear(GumboStringBuffer * input);

// Deallocates this GumboStringBuffer.
void FASTCALL gumbo_string_buffer_destroy(GumboStringBuffer* buffer);
//
// Performs a deep-copy of an GumboStringPiece, allocating a fresh buffer in the
// destination and copying over the characters from source.  Dest should be
// empty, with no buffer allocated; otherwise, this leaks it.
//
void gumbo_string_copy(GumboStringPiece * dest, const GumboStringPiece * source);

// Struct containing all information pertaining to doctype tokens.
typedef struct GumboInternalTokenDocType {
	const char * name;
	const char * public_identifier;
	const char * system_identifier;
	bool force_quirks;
	// There's no way to tell a 0-length public or system ID apart from the
	// absence of a public or system ID, but they're handled different by the
	// spec, so we need bool flags for them.
	bool has_public_identifier;
	bool has_system_identifier;
} GumboTokenDocType;

// Struct containing all information pertaining to start tag tokens.
typedef struct GumboInternalTokenStartTag {
	GumboTag tag;
	GumboVector /* GumboAttribute */ attributes;
	bool is_self_closing;
} GumboTokenStartTag;

// A data structure representing a single token in the input stream.  This
// contains an enum for the type, the source position, a GumboStringPiece
// pointing to the original text, and then a union for any parsed data.
typedef struct GumboInternalToken {
	GumboTokenType type;
	GumboSourcePosition position;
	GumboStringPiece original_text;
	union {
		GumboTokenDocType doc_type;
		GumboTokenStartTag start_tag;
		GumboTag end_tag;
		const char * text; // For comments.
		int character; // For character, whitespace, null, and EOF tokens.
	} v;
} GumboToken;

// Initializes the tokenizer state within the GumboParser object, setting up a
// parse of the specified text.
void gumbo_tokenizer_state_init(GumboParser * parser, const char * text, size_t text_length);

// Destroys the tokenizer state within the GumboParser object, freeing any
// dynamically-allocated structures within it.
void gumbo_tokenizer_state_destroy(GumboParser * parser);
//
// Descr: Sets the tokenizer state to the specified value.  This is needed by some
//   parser states, which alter the state of the tokenizer in response to tags seen.
//
void FASTCALL gumbo_tokenizer_set_state(GumboParser * parser, GumboTokenizerEnum state);

// Flags whether the current node is a foreign content element.  This is
// necessary for the markup declaration open state, where the tokenizer must be
// aware of the state of the parser to properly tokenize bad comment tags.
// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#markup-declaration-open-state
void gumbo_tokenizer_set_is_current_node_foreign(GumboParser * parser, bool is_foreign);

// Lexes a single token from the specified buffer, filling the output with the
// parsed GumboToken data structure.  Returns true for a successful
// tokenization, false if a parse error occurs.
//
// Example:
//   GumboParser parser;
//   GumboToken output;
//   gumbo_tokenizer_state_init(&parser, text, strlen(text));
//   while(gumbo_lex(&parser, &output)) {
//     ...do stuff with output.
//     gumbo_token_destroy(&parser, &token);
//   }
//   gumbo_tokenizer_state_destroy(&parser);
bool gumbo_lex(GumboParser * parser, GumboToken* output);

// Frees the internally-allocated pointers within an GumboToken.  Note that this
// doesn't free the token itself, since oftentimes it will be allocated on the
// stack.  A simple call to free() (or GumboParser->deallocator, if
// appropriate) can handle that.
//
// Note that if you are handing over ownership of the internal strings to some
// other data structure - for example, a parse tree - these do not need to be
// freed.
void FASTCALL gumbo_token_destroy(GumboToken * token);

// Additional data for duplicated attributes.
typedef struct GumboInternalDuplicateAttrError {
	const char * name; // The name of the attribute.  Owned by this struct.
	uint original_index; // The (0-based) index within the attributes vector of the original occurrence.
	uint new_index; // The (0-based) index where the new occurrence would be.
} GumboDuplicateAttrError;
//
// Additional data for tokenizer errors.
// This records the current state and codepoint encountered - this is usually
// enough to reconstruct what went wrong and provide a friendly error message.
typedef struct GumboInternalTokenizerError {
	int codepoint; // The bad codepoint encountered.
	GumboTokenizerErrorState state; // The state that the tokenizer was in at the time.
} GumboTokenizerError;

// Additional data for parse errors.
typedef struct GumboInternalParserError {
	GumboTokenType input_type; // The type of input token that resulted in this error.
	GumboTag input_tag; // The HTML tag of the input token.  TAG_UNKNOWN if this was not a tag token.
	GumboInsertionMode parser_state; // The insertion mode that the parser was in at the time.
	// The tag stack at the point of the error.  Note that this is an GumboVector
	// of GumboTag's *stored by value* - cast the void * to an GumboTag directly to
	// get at the tag.
	GumboVector /* GumboTag */ tag_stack;
} GumboParserError;

// The overall error struct representing an error in decoding/tokenizing/parsing
// the HTML.  This contains an enumerated type flag, a source position, and then
// a union of fields containing data specific to the error.
typedef struct GumboInternalError {
	GumboErrorType type; // The type of error.
	GumboSourcePosition position; // The position within the source file where the error occurred.
	// A pointer to the byte within the original source file text where the error
	// occurred (note that this is not the same as position.offset, as that gives
	// character-based instead of byte-based offsets).
	const char * original_text;
	// Type-specific error information.
	union {
		// The code point we encountered, for:
		// * GUMBO_ERR_UTF8_INVALID
		// * GUMBO_ERR_UTF8_TRUNCATED
		// * GUMBO_ERR_NUMERIC_CHAR_REF_WITHOUT_SEMICOLON
		// * GUMBO_ERR_NUMERIC_CHAR_REF_INVALID
		uint64_t codepoint;
		GumboTokenizerError tokenizer; // Tokenizer errors.
		// Short textual data, for:
		// * GUMBO_ERR_NAMED_CHAR_REF_WITHOUT_SEMICOLON
		// * GUMBO_ERR_NAMED_CHAR_REF_INVALID
		GumboStringPiece text;
		// Duplicate attribute data, for GUMBO_ERR_DUPLICATE_ATTR.
		GumboDuplicateAttrError duplicate_attr;
		// Parser state, for GUMBO_ERR_PARSER and GUMBO_ERR_UNACKNOWLEDGE_SELF_CLOSING_TAG.
		struct GumboInternalParserError parser;
	} v;
} GumboError;

// Adds a new error to the parser's error list, and returns a pointer to it so
// that clients can fill out the rest of its fields.  May return NULL if we're
// already over the max_errors field specified in GumboOptions.
GumboError * gumbo_add_error(GumboParser * parser);
// Initializes the errors vector in the parser.
void gumbo_init_errors(GumboParser * errors);
// Frees all the errors in the 'errors_' field of the parser.
void gumbo_destroy_errors(GumboParser * errors);
// Frees the memory used for a single GumboError.
void FASTCALL gumbo_error_destroy(GumboError * error);
// Prints an error to a string.  This fills an empty GumboStringBuffer with a
// freshly-allocated buffer containing the error message text.  The caller is
// responsible for deleting the buffer.  (Note that the buffer is allocated with
// the allocator specified in the GumboParser config and hence should be freed
// by gumbo_parser_deallocate().)
void gumbo_error_to_string(GumboParser * parser, const GumboError* error, GumboStringBuffer* output);

// Prints a caret diagnostic to a string.  This fills an empty GumboStringBuffer
// with a freshly-allocated buffer containing the error message text.  The
// caller is responsible for deleting the buffer.  (Note that the buffer is
// allocated with the allocator specified in the GumboParser config and hence
// should be freed by gumbo_parser_deallocate().)
void gumbo_caret_diagnostic_to_string(GumboParser * parser, const GumboError* error, const char * source_text, GumboStringBuffer* output);
// Like gumbo_caret_diagnostic_to_string, but prints the text to stdout instead of writing to a string.
void gumbo_print_caret_diagnostic(GumboParser * parser, const GumboError* error, const char * source_text);
//
// Utility function for allocating & copying a null-terminated string into a
// freshly-allocated buffer.  This is necessary for proper memory management; we
// have the convention that all const char * in parse tree structures are
// freshly-allocated, so if we didn't copy, we'd try to delete a literal string
// when the parse tree is destroyed.
//
//char * gumbo_copy_stringz(GumboParser * parser, const char * str);
// Allocate a chunk of memory, using the allocator specified in the Parser's config options.
//void * gumbo_parser_allocate(GumboParser * parser, size_t num_bytes);
// Deallocate a chunk of memory, using the deallocator specified in the Parser's config options.
//void gumbo_parser_deallocate(GumboParser * parser, void * ptr);
// Debug wrapper for printf, to make it easier to turn off debugging info when required.
void gumbo_debug(const char * format, ...);
//
// Initializes a new GumboVector with the specified initial capacity.
void FASTCALL gumbo_vector_init(size_t initial_capacity, GumboVector* vector);
// Frees the memory used by an GumboVector.  Does not free the contained pointers.
void FASTCALL gumbo_vector_destroy(GumboVector* vector);
// Adds a new element to an GumboVector.
void FASTCALL gumbo_vector_add(void * element, GumboVector * vector);
// Removes and returns the element most recently added to the GumboVector.
// Ownership is transferred to caller.  Capacity is unchanged.  If the vector is
// empty, NULL is returned.
void * gumbo_vector_pop(GumboParser * parser, GumboVector* vector);
// Inserts an element at a specific index.  This is potentially O(N) time, but
// is necessary for some of the spec's behavior.
void STDCALL gumbo_vector_insert_at(void * element, uint index, GumboVector * vector);
// Removes an element from the vector, or does nothing if the element is not in the vector.
void FASTCALL gumbo_vector_remove(void * element, GumboVector* vector);
// Removes and returns an element at a specific index.  Note that this is
// potentially O(N) time and should be used sparingly.
void * FASTCALL gumbo_vector_remove_at(uint index, GumboVector* vector);
//
// An overarching struct that's threaded through (nearly) all functions in the
// library, OOP-style.  This gives each function access to the options and
// output, along with any internal state needed for the parse.
//
struct GumboParser {
	const GumboOptions * _options; // Settings for this parse run.
	GumboOutput * _output; // Output for the parse.
	// The internal tokenizer state, defined as a pointer to avoid a cyclic
	// dependency on html5tokenizer.h.  The main parse routine is responsible for
	// initializing this on parse start, and destroying it on parse end.
	// End-users will never see a non-garbage value in this pointer.
	GumboTokenizerState * _tokenizer_state;
	// The internal parser state.  Initialized on parse start and destroyed on
	// parse end; end-users will never see a non-garbage value in this pointer.
	GumboParserState * _parser_state;
};

#ifdef __cplusplus
}
#endif
//
#endif // __GUMBO_INTERNAL_H
