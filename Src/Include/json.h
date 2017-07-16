//
// Copyright (C) 2007 by Rui Maciel
// rui.maciel@gmail.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//  @file json.h A small library that helps deal with JSON-encoded information
// \ingroup JSON
//
// \note error handling is only in a very rudimentary form.
// \author Rui Maciel	rui_maciel@users.sourceforge.net
// \author Sven Herzberg
// \version v1.2
//
#ifndef JSON_H
#define JSON_H

//#include <stdio.h>
//#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// @v8.4.6 #define SIZE_MAX 16386
#define JSON_MAX_STRING_LENGTH (16386-1) // @v8.4.6 (SIZE_MAX-1)-->(16386-1)
//
// Descr: String implementation
//
struct RcString {
	RcString()
	{
		P_Text = 0;
		length = 0;
		max = 0;
	}
	size_t Len() const
	{
		return length;
	}
	char * P_Text; // char c-string */
	size_t length;	// put in place to avoid strlen() calls
	size_t max;	    // usable memory allocated to text minus the space for the nul character
};
//
// The error messages produced by the JSON parsers
//
enum json_error {
	JSON_OK = 1,              // everything went smoothly
	JSON_INCOMPLETE_DOCUMENT, // the parsed document didn't ended
	JSON_WAITING_FOR_EOF,     // A complete JSON document tree was already finished but needs to get to EOF. Other characters beyond whitespaces produce errors
	JSON_MALFORMED_DOCUMENT,  // the JSON document which was fed to this parser is malformed
	JSON_INCOMPATIBLE_TYPE,   // the currently parsed type does not belong here
	JSON_MEMORY,              // an error occurred when allocating memory
	JSON_ILLEGAL_CHARACTER,   // the currently parsed character does not belong here
	JSON_BAD_TREE_STRUCTURE,  // the document tree structure is malformed
	JSON_MAXIMUM_LENGTH,      // the parsed string reached the maximum allowed size
	JSON_UNKNOWN_PROBLEM      // some random, unaccounted problem occurred
};
//
// The JSON document tree node, which is a basic JSON type
//
struct json_t {
	//
	// Descr: The descriptions of the json_value node type
	//
	enum {
		tUnkn = 0,
		tSTRING = 1,
		tNUMBER,
		tOBJECT,
		tARRAY,
		tTRUE,
		tFALSE,
		tNULL
	};
	json_t(int aType);
	~json_t();
	int    Type; // the type of node
	char * P_Text; // The text stored by the node. It stores UTF-8 strings and is used exclusively by the json_t::tSTRING and JSON_NUMBER node types
	//
	// FIFO queue data
	//
	json_t * P_Next;      // The pointer pointing to the next element in the FIFO sibling list
	json_t * P_Previous;  // The pointer pointing to the previous element in the FIFO sibling list
	json_t * P_Parent;    // The pointer pointing to the parent node in the document tree
	json_t * P_Child;     // The pointer pointing to the first child node in the document tree
	json_t * P_ChildEnd; // The pointer pointing to the last child node in the document tree
};
//
// The structure holding all information needed to resume parsing
//
struct json_parsing_info {
	uint   state; // the state where the parsing was left on the last parser run
	uint   lex_state;
	RcString * lex_text;
	char * p;
	int    string_length_limit_reached; // flag informing if the string limit length defined by JSON_MAX_STRING_LENGTH was reached
	json_t * cursor; // pointers to nodes belonging to the document tree which aid the document parsing
};
//
// The structure which holds the pointers to the functions that will be called by the saxy parser whenever their evens are triggered
//
struct json_saxy_functions {
	int (*open_object)();
	int (*close_object)();
	int (*open_array)();
	int (*close_array)();
	int (*new_string)(char *text);
	int (*new_number)(char *text);
	int (*new_true)();
	int (*new_false)();
	int (*new_null)();
	int (*label_value_separator)();
	int (*sibling_separator)();
};
//
// The structure holding the information needed for json_saxy_parse to resume parsing
//
struct json_saxy_parser_status {
	uint   state; // current parser state
	int    string_length_limit_reached; // flag informing if the string limit length defined by JSON_MAX_STRING_LENGTH was reached
	RcString * temp; // temporary string which will be used to build up parsed strings between parser runs.
};
//
// Buils a json_t document by parsing an open file
// @param file a pointer to an object controlling a stream, returned by fopen()
// @param document a reference to a json_t pointer, set to NULL, which will store the parsed document
// @return a json_error error code according to how the parsing operation went.
//
enum json_error json_stream_parse(FILE * file, json_t ** document);
//
// Creates a new JSON value and defines it's type
// @param type the value's type
// @return a pointer to the newly created value structure
//
// json_t * FASTCALL json_new_value(/*json_value_type*/int type);
//
// Creates a new JSON string and defines it's text
// @param text the value's text
// @return a pointer to the newly created JSON string value
//
json_t * json_new_string(const char *text);
//
// Creates a new JSON number and defines it's text. The user is responsible for the number string's correctness
// @param text the value's number
// @return a pointer to the newly created JSON string value
//
json_t * json_new_number(const char *text);
//
// Creates a new JSON object
// @return a pointer to the newly created JSON object value
//
//json_t * json_new_object();
//
// Creates a new JSON array
// @return a pointer to the newly created JSON array value
//
//json_t * json_new_array();
//
// Creates a new JSON null
// @return a pointer to the newly created JSON null value
//
//json_t * json_new_null();
//
// Creates a new JSON true
// @return a pointer to the newly created JSON true value
//
//json_t * json_new_true();
//
// Creates a new JSON false
// @return a pointer to the newly created JSON false value
//
//json_t * json_new_false();
//
// Frees the memory appointed to the value fed as the parameter, as well as all the child nodes
// @param value the root node of the tree being freed
//
void json_free_value(json_t ** value);
//
// Inserts a child node into a parent node, as well as performs some document tree integrity checks.
// @param parent the parent node
// @param child the node being added as a child to parent
// @return the error code corresponding to the operation result
//
enum json_error FASTCALL json_insert_child(json_t * parent, json_t * child);
//
// Inserts a label:value pair into a parent node, as well as performs some document tree integrity checks.
// @param parent the parent node
// @param text_label a char string which serves as the label in the label:value pair
// @param value the value in the label:value pair
// @return the error code corresponding to the operation result
//
enum json_error json_insert_pair_into_object(json_t * parent, const char *text_label, json_t * value);
//
// Produces a JSON markup text document from a document tree
// @param root The document's root node
// @param text a pointer to a char string that will hold the JSON document text.
// @return  a json_error code describing how the operation went
//
enum json_error json_tree_to_string(json_t * root, char **text);
//
// Produces a JSON markup text document from a json_t document tree to a text stream
// @param file a opened file stream
// @param root The document's root node
// @return  a json_error code describing how the operation went
//
enum json_error json_stream_output(FILE * file, json_t * root);
//
// Strips all JSON white spaces from the text string
// @param text a char string holding a JSON document or document snippet
//
void json_strip_white_spaces(char *text);
//
// Formats a JSON markup text contained in the given string
// @param text a JSON formatted document
// @return a pointer to a char string holding the formated document
//
char *json_format_string(const char *text);
//
// Outputs a new UTF8 c-string which replaces all characters that must be escaped with their respective escaped versions
// @param text an UTF8 char text string
// @return an UTF-8 c-string holding the same text string but with escaped characters
//
char * json_escape(const char *text);
//
// Outputs a new UTF-8 c-string which has all escaped characters replaced by
// their unescaped, UTF-8 encoded variants.
//
// @param test a UTF-8 c-string
// @return a newly allocated UTF-8 c-string; free with free()
//
char * json_unescape(char *text);
//
// This function takes care of the tedious task of initializing any instance of json_parsing_info
// @param jpi a pointer to a struct json_parsing_info instance
//
void json_jpi_init(json_parsing_info *jpi);
//
// Produces a document tree sequentially from a JSON markup text fragment
// @param info the information necessary to resume parsing any incomplete document
// @param buffer a null-terminated c-string containing a JSON document fragment
// @return a code describing how the operation ended up
//
enum json_error json_parse_fragment(json_parsing_info * info, const char * buffer);
//
// Produces a document tree from a JSON markup text string that contains a complete document
// @param root a reference to a pointer to a json_t type. The function allocates memory to the passed pointer and sets up the value
// @param text a c-string containing a complete JSON text document
// @return a pointer to the new document tree or NULL if some error occurred
//
enum json_error json_parse_document(json_t ** root, const char * text);
//
// Function to perform a SAX-like parsing of any JSON document or document fragment that is passed to it
// @param jsps a structure holding the status information of the current parser
// @param jsf a structure holding the function pointers to the event functions
// @param c the character to be parsed
// @return a json_error code informing how the parsing went
//
enum json_error json_saxy_parse(json_saxy_parser_status * jsps, json_saxy_functions * jsf, char c);
//
// Searches through the object's children for a label holding the text text_label
// @param object a json_value of type json_t::tOBJECT
// @param text_label the c-string to search for through the object's child labels
// @return a pointer to the first label holding a text equal to text_label or NULL if there is no such label or if object has no children
//
json_t *json_find_first_label(const json_t * object, const char *text_label);
//
// Helper
//
char   * json_get_value(const json_t *object, const char *text_label);
json_t * json_process(json_t *object);

#ifdef __cplusplus
}
#endif
#endif
