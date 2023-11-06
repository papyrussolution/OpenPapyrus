// JSON.CPP
// Copyright (C) 2007 by Rui Maciel rui.maciel@gmail.com
// @codepage UTF-8
//
// This program is free software; you can redistribute it and/or modify it under the terms of the
// GNU Library General Public License as published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// You should have received a copy of the GNU Library General Public License along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// Adopted to SLIB by A.Sobolev 2010-2021, 2022
//
#include <slib-internal.h>
#pragma hdrstop
// @v10.9.4 (inlined in slib.h) #include <json.h>

#define JSON_MAX_STRING_LENGTH (16386-1)
//
// Descr: String implementation
//
struct RcString {
	RcString() : P_Text(0), length(0), max(0)
	{
	}
	size_t Len() const { return length; }
	char * P_Text; // char c-string */
	size_t length;	// put in place to avoid strlen() calls
	size_t max;	    // usable memory allocated to text minus the space for the nul character
};
//
// The structure holding all information needed to resume parsing
//
struct JsonParsingBlock {
	JsonParsingBlock() : state(0), lex_state(0), P_Cur(0), cursor(0)/* @v11.3.12 (unused), string_length_limit_reached(0)*/
	{
	}
	~JsonParsingBlock()
	{
	}
	uint   state; // the state where the parsing was left on the last parser run
	uint   lex_state;
	SString Text;
	const  char * P_Cur;
	// @v11.3.12 (unused) int    string_length_limit_reached; // flag informing if the string limit length defined by JSON_MAX_STRING_LENGTH was reached
	SJson * cursor; // pointers to nodes belonging to the document tree which aid the document parsing
};

enum LEX_VALUE {
	LEX_MORE = 0,
	LEX_INVALID_CHARACTER,
	LEX_TRUE,
	LEX_FALSE,
	LEX_NULL,
	LEX_BEGIN_OBJECT,
	LEX_END_OBJECT,
	LEX_BEGIN_ARRAY,
	LEX_END_ARRAY,
	LEX_NAME_SEPARATOR,
	LEX_VALUE_SEPARATOR,
	LEX_STRING,
	LEX_NUMBER,
	LEX_ERROR,
	LEX_MEMORY
};

#define RSTRING_INCSTEP 5
#define RSTRING_DEFAULT 8

enum rstring_code {
	RS_MEMORY,
	RS_OK = 1,
	RS_UNKNOWN
};

//typedef enum rui_string_error_codes rstring_code;

static RcString * FASTCALL rcs_create(size_t length)
{
	RcString * rcs = static_cast<RcString *>(SAlloc::M(sizeof(RcString))); // allocates memory for a struct RcString
	if(rcs) {
		rcs->max = length;
		rcs->length = 0;
		THROW(rcs->P_Text = static_cast<char *>(SAlloc::M((rcs->max + 1) * sizeof(char))));
		rcs->P_Text[0] = '\0';
	}
	CATCH
		ZFREE(rcs);
	ENDCATCH
	return rcs;
}

static void FASTCALL rcs_free(RcString ** rcs)
{
	assert(rcs);
	if(*rcs) {
		ZFREE((*rcs)->P_Text);
		ZFREE(*rcs);
	}
}

static rstring_code FASTCALL rcs_resize(RcString * rcs, size_t length)
{
	assert(rcs);
	char * p_temp = static_cast<char *>(SAlloc::R(rcs->P_Text, sizeof(char) * (length + 1))); // length plus '\0'
	if(!p_temp) {
		SAlloc::F(rcs);
		return RS_MEMORY;
	}
	else {
		rcs->P_Text = p_temp;
		rcs->max = length;
		rcs->P_Text[rcs->max] = '\0';
		return RS_OK;
	}
}

static rstring_code FASTCALL rcs_catc(RcString * pre, const char c)
{
	assert(pre);
	if(pre->max <= pre->length)
		if(rcs_resize(pre, pre->max + RSTRING_INCSTEP) != RS_OK)
			return RS_MEMORY;
	pre->P_Text[pre->length] = c;
	pre->length++;
	pre->P_Text[pre->length] = '\0';
	return RS_OK;
}

/*static*/SJson * SJson::CreateObj() { return new SJson(tOBJECT); }
/*static*/SJson * SJson::CreateArr() { return new SJson(tARRAY); }

/*static*/SJson * SJson::CreateString(const char * pText)
{
	SJson * p_result = new SJson(tSTRING);
	p_result->Text = pText;
	return p_result;
}

/*static*/SJson * FASTCALL SJson::Parse(const char * pText)
{
	SJson * p_result = 0;
	if(json_parse_document(&p_result, pText) != JSON_OK) {
		ZDELETE(p_result);
	}
	return p_result;
}

/*static*/SJson * FASTCALL SJson::ParseFile(const char * pFileName)
{
	SJson * p_result = 0;
	THROW(!isempty(pFileName));
	THROW(fileExists(pFileName));
	{
		SString temp_buf;
		SFile f_inp(pFileName, SFile::mRead|SFile::mBinary);
		THROW(f_inp.IsValid());
		{
			STempBuffer in_buf(4096);
			size_t actual_size = 0;
			THROW(in_buf.IsValid());
			THROW(f_inp.ReadAll(in_buf, 0, &actual_size));
			temp_buf.Z().CatN(in_buf, actual_size);
		}
		p_result = SJson::Parse(temp_buf);
	}
	CATCH
		ZDELETE(p_result);
	ENDCATCH
	return p_result;
}

/*static*/const SString & FASTCALL SJson::Unescape(const SString & rRawText)
{
	if(!rRawText.IsEmpty() && rRawText.HasChr('\\')) {
		SString & r_temp_buf = SLS.AcquireRvlStr();
		(r_temp_buf = rRawText).Unescape();
		return r_temp_buf;
	}
	else
		return rRawText;
}

/*static*/int  FASTCALL SJson::GetBoolean(const SJson * pN) 
{ 
	int    result = -1;
	if(pN) {
		if(pN->Type == SJson::tTRUE)
			result = 1;
		else if(pN->Type == SJson::tFALSE)
			result = 0;
	}
	return result;
}

SJson::SJson(int aType) : Type(aType), P_Next(0), P_Previous(0), P_Parent(0), P_Child(0), P_ChildEnd(0), State(0)
{
}

SJson::~SJson()
{
	//
	// Велик соблазн использовать здесь рекурсивное удаление внутренних объектов, но нельзя!
	// При больших цепочках возможно переполнение стека.
	//
	if(P_Next) {
		for(SJson * p_next = P_Next; p_next;) {
			SJson * p_temp = p_next->P_Next;
			p_next->P_Next = 0;
			delete p_next;
			p_next = p_temp;
		}
		P_Next = 0;
	}
	if(P_Child) {
		for(SJson * p_child = P_Child; p_child;) {
			SJson * p_temp = p_child->P_Child;
			p_child->P_Child = 0;
			delete p_child;
			p_child = p_temp;
		}
		P_Child = 0;
	}
}

bool SJson::IsValid() const { return !(State & 0x0001); }

uint SJson::GetArrayCount() const
{
	uint   result = 0;
	if(IsArray()) {
		for(const SJson * p_inr = P_Child; p_inr; p_inr = p_inr->P_Next) {
			result++;
		}
	}
	return result;
}

/*static*/bool FASTCALL SJson::FormatText(const char * pSrcJsText, SString & rBuf)
{
	return LOGIC(json_format_string(pSrcJsText, rBuf));
}

int FASTCALL SJson::ToStr(SStringU & rBuf) const
{
	rBuf.Z();
	SString temp_buf;
	int   ok = ToStr(temp_buf);
	if(ok) {
		ok = rBuf.CopyFromUtf8Strict(temp_buf.cptr(), temp_buf.Len());
	}
	return ok;
}

int FASTCALL SJson::ToStr(SString & rBuf) const
{
	//return json_tree_to_string(this, rBuf);
	int    ok = 1;
	SString temp_buf;
	assert(this);
	rBuf.Z();
	const SJson * p_cursor = this;
	// start the convoluted fun
state1: // open value
	if(p_cursor->P_Previous && p_cursor != this) { // if cursor is children and not root than it is a followup sibling
		rBuf.Comma();
	}
	switch(p_cursor->Type) {
		case SJson::tSTRING:
			// append the "text"\0, which means 1 + wcslen(cursor->text) + 1 + 1
			// set the new output size
			rBuf.CatChar('\"').Cat(p_cursor->Text).CatChar('\"');
			if(p_cursor->P_Parent) {
				if(p_cursor->P_Parent->Type == SJson::tOBJECT)	{ // cursor is label in label:value pair
					// error checking: if parent is object and cursor is string then cursor must have a single child
					THROW_S(p_cursor->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE); // malformed document tree: label without value in label:value pair
					rBuf.Colon();
				}
			}
			else {	// does not have a parent
				// is root label in label:value pair
				THROW_S(p_cursor->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE); // malformed document tree: label without value in label:value pair
				rBuf.Colon();
			}
			break;
		case SJson::tNUMBER: // must not have any children
			// set the new size
			rBuf.Cat(p_cursor->Text);
			goto state2; // close value
			break;
		case SJson::tOBJECT:
			rBuf.CatChar('{');
			if(p_cursor->P_Child) {
				p_cursor = p_cursor->P_Child;
				goto state1; // open value
			}
			else
				goto state2; // close value
			break;
		case SJson::tARRAY:
			rBuf.CatChar('[');
			if(p_cursor->P_Child) {
				p_cursor = p_cursor->P_Child;
				goto state1;
			}
			else
				goto state2; // close value
			break;
		case SJson::tTRUE: // must not have any children
			rBuf.Cat("true");
			goto state2; // close value
			break;
		case SJson::tFALSE: // must not have any children
			rBuf.Cat("false");
			goto state2; // close value
			break;
		case SJson::tNULL: // must not have any children
			rBuf.Cat("null");
			goto state2; // close value
			break;
		default: 
			CALLEXCEPT_S(SLERR_JSON_UNKNOWN_PROBLEM); 
			break;
	}
	if(p_cursor->P_Child) {
		p_cursor = p_cursor->P_Child;
		goto state1; // open value */
	}
	else // does not have any children
		goto state2; // close value
state2: // close value
	switch(p_cursor->Type) {
		case SJson::tOBJECT: rBuf.CatChar('}'); break;
		case SJson::tARRAY:  rBuf.CatChar(']'); break;
		case SJson::tSTRING: break;
		case SJson::tNUMBER: break;
		case SJson::tTRUE:   break;
		case SJson::tFALSE: break;
		case SJson::tNULL:   break;
		default: CALLEXCEPT_S(SLERR_JSON_UNKNOWN_PROBLEM); break;
	}
	if(!p_cursor->P_Parent || p_cursor == this)
		goto end;
	else if(p_cursor->P_Next) {
		p_cursor = p_cursor->P_Next;
		goto state1; // open value
	}
	else {
		p_cursor = p_cursor->P_Parent;
		goto state2; // close value
	}
end:
	CATCHZOK
	return ok;
}

void FASTCALL SJson::AssignText(const SString & rT)
{
	Text = rT;
}

const SJson * FASTCALL SJson::FindChildByKey(const char * pKey) const
{
	const SJson * p_result = 0;
	if(!isempty(pKey)) {
		for(const SJson * p_c = P_Child; !p_result && p_c; p_c = p_c->P_Next) {
			if(p_c->Text.IsEqiAscii(pKey) && p_c->P_Child)
				p_result = p_c->P_Child;
		}
	}
	return p_result;
}

enum json_error json_stream_parse(FILE * file, SJson ** document)
{
	char   buffer[1024]; // hard-coded value 
	enum   json_error error = JSON_INCOMPLETE_DOCUMENT;
	JsonParsingBlock state;
	assert(file); // must be an open stream
	assert(document); // must be a valid pointer reference
	assert(*document == NULL); // only accepts a null SJson pointer, to avoid memory leaks
	while(oneof4(error, SLERR_JSON_WAITING_FOR_EOF, JSON_WAITING_FOR_EOF, SLERR_JSON_INCOMPLETE_DOCUMENT, JSON_INCOMPLETE_DOCUMENT)) {
		if(fgets(buffer, 1024, file)) {
			error = json_parse_fragment(state, buffer) ? JSON_OK : (enum json_error)SLibError;
			switch(error) {
				case JSON_OK:
				case SLERR_JSON_WAITING_FOR_EOF:
				case JSON_WAITING_FOR_EOF:
				case SLERR_JSON_INCOMPLETE_DOCUMENT:
				case JSON_INCOMPLETE_DOCUMENT:
					break;
				default:
					ZDELETE(state.cursor);
					return error;
					break;
			}
		}
		else {
			if(error == JSON_WAITING_FOR_EOF)
				error = JSON_OK;
			else
				error = JSON_UNKNOWN_PROBLEM; // @todo refine this error code
		}
	}
	if(error == JSON_OK)
		*document = state.cursor;
	return error;
}

SJson * FASTCALL json_new_string(const char * pText)
{
	// @v10.9.4 assert(pText);
	SJson * p_new_object = new SJson(SJson::tSTRING);
	if(p_new_object)
		p_new_object->Text = pText;
	return p_new_object;
}

SJson * FASTCALL json_new_number(const char * pText)
{
	assert(pText);
	SJson * p_new_object = new SJson(SJson::tNUMBER);
	if(p_new_object)
		p_new_object->Text = pText;
	return p_new_object;
}

SJson * json_new_null()
{
	SJson * p_new_object = new SJson(SJson::tNULL);
	return p_new_object;
}

void FASTCALL json_free_value(SJson ** ppValue)
{
	if(ppValue && *ppValue) {
		delete *ppValue;
		*ppValue = 0;
	}
}

int FASTCALL json_insert_child(SJson * pParent, SJson * pChild)
{
	int    ok = 1;
	// @todo change the child list from FIFO to LIFO, in order to get rid of the child_end pointer
	assert(pParent); // the parent must exist
	assert(pChild); // the child must exist
	assert(pParent != pChild); // parent and child must not be the same. if they are, it will enter an infinite loop
	// enforce tree structure correctness
	switch(pParent->Type) {
		case SJson::tSTRING:
			// a string accepts every JSON type as a child value
			// therefore, the sanity check must be performed on the child node
			switch(pChild->Type) {
				case SJson::tSTRING:
				case SJson::tNUMBER:
				case SJson::tTRUE:
				case SJson::tFALSE:
				case SJson::tNULL:
					THROW_S(!pChild->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
				case SJson::tOBJECT:
				case SJson::tARRAY:
					break;
				default:
					//return JSON_BAD_TREE_STRUCTURE;
					CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE); // this part should never be reached
					break;
			}
			break;
		case SJson::tOBJECT: // JSON objects may only accept JSON string objects which already have child nodes of their own
			THROW_S(pChild->Type == SJson::tSTRING, SLERR_JSON_BAD_TREE_STRUCTURE);
			break;
		case SJson::tARRAY:
			switch(pChild->Type) {
				case SJson::tSTRING:
				case SJson::tTRUE:
				case SJson::tFALSE:
				case SJson::tNULL:
				case SJson::tNUMBER:
					THROW_S(!pChild->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
				case SJson::tOBJECT:
				case SJson::tARRAY:
					break;
				default:
					CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
			}
			break;
		default:
			CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
			break;
	}
	pChild->P_Parent = pParent;
	if(pParent->P_Child) {
		pChild->P_Previous = pParent->P_ChildEnd;
		pParent->P_ChildEnd->P_Next = pChild;
		pParent->P_ChildEnd = pChild;
	}
	else {
		pParent->P_Child = pChild;
		pParent->P_ChildEnd = pChild;
	}
	CATCHZOK
	return ok;
}

int FASTCALL SJson::InsertChild(SJson * pChild)
{
	int    ok = 1;
	// @todo change the child list from FIFO to LIFO, in order to get rid of the child_end pointer
	assert(pChild); // the child must exist
	assert(this != pChild); // parent and child must not be the same. if they are, it will enter an infinite loop
	// enforce tree structure correctness
	switch(Type) {
		case SJson::tSTRING:
			// a string accepts every JSON type as a child value
			// therefore, the sanity check must be performed on the child node
			switch(pChild->Type) {
				case SJson::tSTRING:
				case SJson::tNUMBER:
				case SJson::tTRUE:
				case SJson::tFALSE:
				case SJson::tNULL:
					THROW_S(!pChild->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
				case SJson::tOBJECT:
				case SJson::tARRAY:
					break;
				default:
					//return JSON_BAD_TREE_STRUCTURE;
					CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE); // this part should never be reached
					break;
			}
			break;
		case SJson::tOBJECT: // JSON objects may only accept JSON string objects which already have child nodes of their own
			THROW_S(pChild->Type == SJson::tSTRING, SLERR_JSON_BAD_TREE_STRUCTURE);
			break;
		case SJson::tARRAY:
			switch(pChild->Type) {
				case SJson::tSTRING:
				case SJson::tTRUE:
				case SJson::tFALSE:
				case SJson::tNULL:
				case SJson::tNUMBER:
					THROW_S(!pChild->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
				case SJson::tOBJECT:
				case SJson::tARRAY:
					break;
				default:
					CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
					break;
			}
			break;
		default:
			CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
			break;
	}
	pChild->P_Parent = this;
	if(P_Child) {
		pChild->P_Previous = P_ChildEnd;
		P_ChildEnd->P_Next = pChild;
		P_ChildEnd = pChild;
	}
	else {
		P_Child = pChild;
		P_ChildEnd = pChild;
	}
	CATCHZOK
	return ok;
}

int SJson::Insert(const char * pTextLabel, SJson * pValue)
{
	int    ok = 1;
	// verify if the parameters are valid
	assert(pTextLabel);
	assert(pValue);
	assert(this != pValue);
	// enforce type coherence
	assert(Type == SJson::tOBJECT);
	// create label json_value
	SJson * p_label = json_new_string(pTextLabel);
	THROW(p_label);
	// insert value and check for error
	THROW(json_insert_child(p_label, pValue));
	THROW(json_insert_child(this, p_label)); // insert value and check for error
	CATCHZOK
	return ok;
}

int SJson::InsertNz(const char * pTextLabel, SJson * pValue)
{
	int    ok = 1;
	// verify if the parameters are valid
	assert(pTextLabel);
	if(pValue) {
		assert(this != pValue);
		// enforce type coherence
		assert(Type == SJson::tOBJECT);
		// create label json_value
		SJson * p_label = json_new_string(pTextLabel);
		THROW(p_label);
		// insert value and check for error
		THROW(json_insert_child(p_label, pValue));
		THROW(json_insert_child(this, p_label)); // insert value and check for error
	}
	else
		ok = -1;
	CATCHZOK
	return ok;	
}

int SJson::InsertString(const char * pTextLabel, const char * pStr) { return Insert(pTextLabel, json_new_string(pStr)); }
int SJson::InsertStringNe(const char * pTextLabel, const char * pStr) { return isempty(pStr) ? -1 : Insert(pTextLabel, json_new_string(pStr)); }
int SJson::InsertNumber(const char * pTextLabel, const char * pStr) { return Insert(pTextLabel, json_new_number(pStr)); }
int FASTCALL SJson::InsertNull(const char * pTextLabel) { return Insert(pTextLabel, json_new_null()); }
int SJson::InsertDouble(const char * pTextLabel, double val, long fmt) { return Insert(pTextLabel, json_new_number(SLS.AcquireRvlStr().Cat(val, fmt))); }
int SJson::InsertInt(const char * pTextLabel, int val) { return Insert(pTextLabel, json_new_number(SLS.AcquireRvlStr().Cat(val))); }
int SJson::InsertIntNz(const char * pTextLabel, int val) { return (val != 0) ? Insert(pTextLabel, json_new_number(SLS.AcquireRvlStr().Cat(val))) : -1; }
int SJson::InsertInt64(const char * pTextLabel, int64 val) { return Insert(pTextLabel, json_new_number(SLS.AcquireRvlStr().Cat(val))); }
int SJson::InsertBool(const char * pTextLabel, bool val) { return Insert(pTextLabel, new SJson(val ? SJson::tTRUE : SJson::tFALSE)); }

#if 0 // @v11.2.7 (replaced with SJson::ToStr) {
int FASTCALL json_tree_to_string(const SJson * pRoot, SString & rBuf)
{
	int    ok = 1;
	assert(pRoot);
	rBuf.Z();
	const SJson * cursor = pRoot;
	// start the convoluted fun
state1: // open value
	if(cursor->P_Previous && cursor != pRoot) { // if cursor is children and not root than it is a followup sibling
		rBuf.Comma();
	}
	switch(cursor->Type) {
		case SJson::tSTRING:
			// append the "text"\0, which means 1 + wcslen(cursor->text) + 1 + 1
			// set the new output size
			rBuf.CatChar('\"').Cat(cursor->Text).CatChar('\"');
			if(cursor->P_Parent) {
				if(cursor->P_Parent->Type == SJson::tOBJECT)	{ // cursor is label in label:value pair
					// error checking: if parent is object and cursor is string then cursor must have a single child
					THROW_S(cursor->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE); // malformed document tree: label without value in label:value pair
					rBuf.Colon();
				}
			}
			else {	// does not have a parent
				// is root label in label:value pair
				THROW_S(cursor->P_Child, SLERR_JSON_BAD_TREE_STRUCTURE); // malformed document tree: label without value in label:value pair
				rBuf.Colon();
			}
			break;
		case SJson::tNUMBER: // must not have any children
			// set the new size
			rBuf.Cat(cursor->Text);
			goto state2; // close value
			break;
		case SJson::tOBJECT:
			rBuf.CatChar('{');
			if(cursor->P_Child) {
				cursor = cursor->P_Child;
				goto state1; // open value
			}
			else
				goto state2; // close value
			break;
		case SJson::tARRAY:
			rBuf.CatChar('[');
			if(cursor->P_Child) {
				cursor = cursor->P_Child;
				goto state1;
			}
			else
				goto state2; // close value
			break;
		case SJson::tTRUE: // must not have any children
			rBuf.Cat("true");
			goto state2; // close value
			break;
		case SJson::tFALSE: // must not have any children
			rBuf.Cat("false");
			goto state2; // close value
			break;
		case SJson::tNULL: // must not have any children
			rBuf.Cat("null");
			goto state2; // close value
			break;
		default: 
			CALLEXCEPT_S(SLERR_JSON_UNKNOWN_PROBLEM); 
			break;
	}
	if(cursor->P_Child) {
		cursor = cursor->P_Child;
		goto state1; // open value */
	}
	else // does not have any children
		goto state2; // close value
state2: // close value
	switch(cursor->Type) {
		case SJson::tOBJECT: rBuf.CatChar('}'); break;
		case SJson::tARRAY:  rBuf.CatChar(']'); break;
		case SJson::tSTRING: break;
		case SJson::tNUMBER: break;
		case SJson::tTRUE:   break;
		case SJson::tFALSE: break;
		case SJson::tNULL:   break;
		default: CALLEXCEPT_S(SLERR_JSON_UNKNOWN_PROBLEM); break;
	}
	if(!cursor->P_Parent || cursor == pRoot)
		goto end;
	else if(cursor->P_Next) {
		cursor = cursor->P_Next;
		goto state1; // open value
	}
	else {
		cursor = cursor->P_Parent;
		goto state2; // close value
	}
end:
	CATCHZOK
	return ok;
}
#endif // } 0 @v11.2.7 (replaced with SJson::ToStr)

enum json_error json_stream_output(SJson * root, SString & rBuf)
{
	assert(root);
	//assert(file); // the file stream must be opened
	SJson * cursor = root;
	// set up the output and temporary rwstrings
// start the convoluted fun
state1: // open value
	if(cursor->P_Previous && cursor != root) { // if cursor is children and not root than it is a followup sibling
		rBuf.Comma(); // append comma
	}
	switch(cursor->Type) {
		case SJson::tSTRING:
			// append the "text"\0, which means 1 + wcslen(cursor->text) + 1 + 1
			// set the new output size
			rBuf.CatQStr(cursor->Text);
			if(cursor->P_Parent) {
				if(cursor->P_Parent->Type == SJson::tOBJECT) { // cursor is label in label:value pair
					if(cursor->P_Child) // error checking: if parent is object and cursor is string then cursor must have a single child
						rBuf.Colon();
					else // malformed document tree: label without value in label:value pair
						return JSON_BAD_TREE_STRUCTURE;
				}
			}
			else { // does not have a parent
				if(cursor->P_Child) // is root label in label:value pair
					rBuf.Colon();
				else // malformed document tree: label without value in label:value pair
					return JSON_BAD_TREE_STRUCTURE;
			}
			break;
		case SJson::tNUMBER:
			// must not have any children
			// set the new size
			rBuf.Cat(cursor->Text);
			goto state2; // close value
			break;
		case SJson::tOBJECT:
			rBuf.CatChar('{');
			if(cursor->P_Child) {
				cursor = cursor->P_Child;
				goto state1; // open value
			}
			else
				goto state2; // close value
			break;
		case SJson::tARRAY:
			rBuf.CatChar('[');
			if(cursor->P_Child) {
				cursor = cursor->P_Child;
				goto state1;
			}
			else
				goto state2; // close value
			break;
		case SJson::tTRUE: // must not have any children
			rBuf.Cat("true");
			goto state2; // close value
			break;
		case SJson::tFALSE: // must not have any children
			rBuf.Cat("false");
			goto state2; // close value
			break;
		case SJson::tNULL: // must not have any children
			rBuf.Cat("null");
			goto state2; // close value
			break;
		default:
			goto error;
	}
	if(cursor->P_Child) {
		cursor = cursor->P_Child;
		goto state1; // open value
	}
	else
		// does not have any children
		goto state2; // close value
state2: // close value
	switch(cursor->Type) {
		case SJson::tOBJECT: rBuf.CatChar('}'); break;
		case SJson::tARRAY: rBuf.CatChar(']'); break;
		case SJson::tSTRING: break;
		case SJson::tNUMBER: break;
		case SJson::tTRUE:   break;
		case SJson::tFALSE: break;
		case SJson::tNULL:   break;
		default:          goto error;
	}
	if(!cursor->P_Parent || cursor == root)
		goto end;
	else if(cursor->P_Next) {
		cursor = cursor->P_Next;
		goto state1; // open value
	}
	else {
		cursor = cursor->P_Parent;
		goto state2; // close value
	}
error:
	return JSON_UNKNOWN_PROBLEM;
end:
	rBuf.CR();
	return JSON_OK;
}

void json_strip_white_spaces(char * text)
{
	assert(text);
	size_t in = 0;
	size_t out = 0;
	size_t length = sstrlen(text);
	int    state = 0; // possible states: 0 -> document, 1 -> inside a string
	while(in < length) {
		switch(text[in]) {
			case '\x20': // space
			case '\x09': // horizontal tab
			case '\x0A': // line feed or new line
			case '\x0D': // Carriage return
				if(state == 1)
					text[out++] = text[in];
				break;
			case '\"':
				switch(state) {
					case 0: // not inside a JSON string
						state = 1;
						break;
					case 1: // inside a JSON string
						if(text[in-1] != '\\')
							state = 0;
						break;
					default:
						assert(0);
				}
				text[out++] = text[in];
				break;
			default:
				text[out++] = text[in];
		}
		++in;
	}
	text[out] = '\0';
}

int json_format_string(const char * pText, SString & rBuf)
{
	int    ok = 1;
	size_t pos = 0;
	uint   indentation = 0; // the current indentation level
	uint   line_no = 0;
	char   loop;
	size_t text_length = sstrlen(pText);
	rBuf.Z();
	while(pos < text_length) {
		switch(pText[pos]) {
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				pos++;
				break;
			// @v11.7.10 {
			case '[':
				indentation++;
				rBuf.CatChar('[').CR();
				line_no++;
				if(indentation)
					rBuf.Tab(indentation);
				pos++;
				break;
			case ']':
				THROW(indentation > 0);
				indentation--;
				rBuf.CR();
				line_no++;
				if(indentation)
					rBuf.Tab(indentation);
				rBuf.CatChar(']');
				pos++;
				break;
			// } @v11.7.10 
			case '{':
				indentation++;
				rBuf.CatChar('{').CR();
				line_no++;
				if(indentation)
					rBuf.Tab(indentation);
				pos++;
				break;
			case '}':
				THROW(indentation > 0);
				indentation--;
				rBuf.CR();
				line_no++;
				if(indentation)
					rBuf.Tab(indentation);
				rBuf.CatChar('}');
				pos++;
				break;
			case ':':
				rBuf.CatDiv(':', 2);
				pos++;
				break;
			case ',':
				rBuf.Comma().CR();
				line_no++;
				if(indentation)
					rBuf.Tab(indentation);
				pos++;
				break;
			case '\"':	// open string
				rBuf.CatChar(pText[pos++]);
				for(loop = 1; loop;) { // inner string loop trigger is enabled
					const char cc = pText[pos];
					if(cc == '\\') // escaped sequence
						rBuf.CatChar(pText[pos++]);
					else if(cc == '\"') // reached end of string
						loop = 0;
					rBuf.CatChar(pText[pos++]);
					if(pos >= text_length)
						loop = 0;
				}
				break;
			default:
				rBuf.CatChar(pText[pos++]);
				break;
		}
	}
	CATCHZOK
	return ok;
}

#if 0 // @construction {
static int FASTCALL Lexer2(JsonParsingBlock * pBlk, const char * pBuffer)
{
	char   chr = 0; // temporary character
	assert(pBuffer);
	//assert(text);
	pBlk->Text.Z();
	SETIFZ(pBlk->P_Cur, pBuffer);
	while(*pBlk->P_Cur) {
		switch(pBlk->lex_state) {
			case 0: // Root document
				switch(*(pBlk->P_Cur)++) {
					case '\x20': // space 
					case '\x09': // horizontal tab 
					case '\x0A': // line feed or new line 
					case '\x0D': // Carriage return 
						break;
					case '{': return LEX_BEGIN_OBJECT;
					case '}': return LEX_END_OBJECT;
					case '[': return LEX_BEGIN_ARRAY;
					case ']': return LEX_END_ARRAY;
					case ':': return LEX_NAME_SEPARATOR;
					case ',': return LEX_VALUE_SEPARATOR;
					case '\"':
						pBlk->Text.Z();
						pBlk->lex_state = 1; // inside a JSON string 
						break;
					case 't': pBlk->lex_state =  7; break; // true: 1
					case 'f': pBlk->lex_state = 10; break; // false: 1
					case 'n': pBlk->lex_state = 14; break; // false: 1
					case '-':
						pBlk->Text.Z().CatChar('-');
						pBlk->lex_state = 17; // number: '0'
						break;
					case '0':
						pBlk->Text.Z().CatChar('0');
						pBlk->lex_state = 18; // number: '0' 
						break;
					case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.Z().CatChar(*(pBlk->P_Cur - 1));
						pBlk->lex_state = 19; // number: decimal followup
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 1:	// inside a JSON string
				switch(*pBlk->P_Cur) {
					case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
					case 10: // line feed
					case 11: case 12:
					case 13: // carriage return
					case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22:
					case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
						// ASCII control characters can only be present in a JSON string if they are escaped. If not then the document is invalid
						return LEX_INVALID_CHARACTER;
						break;
					case '\"': // close JSON string
						// it is expected that, in the routine that calls this function, text is set to NULL
						pBlk->lex_state = 0;
						++pBlk->P_Cur;
						return LEX_STRING;
						break;
					case '\\':
						pBlk->Text.BSlash();
						pBlk->lex_state = 2; // inside a JSON string: start escape sequence
						break;
					default:
						pBlk->Text.CatChar(*pBlk->P_Cur);
						break;
				}
				++pBlk->P_Cur;
				break;
			case 2: // inside a JSON string: start escape sequence
				switch(*pBlk->P_Cur) {
					case '\\':
					case '\"':
					case '/':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						pBlk->lex_state = 1; // inside a JSON string
						break;
					case 'u':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						pBlk->lex_state = 3; // inside a JSON string: escape unicode
						break;
					default:
						return LEX_INVALID_CHARACTER;
				}
				++pBlk->P_Cur;
				break;
			case 3: // inside a JSON string: escape unicode
				if(ishex(*pBlk->P_Cur)) {
					pBlk->Text.CatChar(*pBlk->P_Cur);
					pBlk->lex_state = 4; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++pBlk->P_Cur;
				break;
			case 4:	// inside a JSON string: escape unicode
				if(ishex(*pBlk->P_Cur)) {
					pBlk->Text.CatChar(*pBlk->P_Cur);
					pBlk->lex_state = 5; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++pBlk->P_Cur;
				break;
			case 5:	// inside a JSON string: escape unicode
				if(ishex(*pBlk->P_Cur)) {
					pBlk->Text.CatChar(*pBlk->P_Cur);
					pBlk->lex_state = 6; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++pBlk->P_Cur;
				break;
			case 6:	// inside a JSON string: escape unicode
				if(ishex(*pBlk->P_Cur)) {
					pBlk->Text.CatChar(*pBlk->P_Cur);
					pBlk->lex_state = 1; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++pBlk->P_Cur;
				break;
			case 7: // true: 1
				switch(*(pBlk->P_Cur)++) {
					case 'r': pBlk->lex_state = 8; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 8: // true: 2
				switch(*(pBlk->P_Cur)++) {
					case 'u': pBlk->lex_state = 9; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 9: // true: 3
				switch(*(pBlk->P_Cur)++) {
					case 'e': pBlk->lex_state = 0; return LEX_TRUE;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 10: // false: 1
				switch(*(pBlk->P_Cur)++) {
					case 'a': pBlk->lex_state = 11; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 11: // false: 2 
				switch(*(pBlk->P_Cur)++) {
					case 'l': pBlk->lex_state = 12; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 12: // false: 3 
				switch(*(pBlk->P_Cur)++) {
					case 's': pBlk->lex_state = 13; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 13: // false: 4 
				switch(*(pBlk->P_Cur)++) {
					case 'e': pBlk->lex_state = 0; return LEX_FALSE;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 14: // null: 1 
				switch(*(pBlk->P_Cur)++) {
					case 'u': pBlk->lex_state = 15; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 15: // null: 2 
				switch(*(pBlk->P_Cur)++) {
					case 'l': pBlk->lex_state = 16; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 16: // null: 3
				switch(*(pBlk->P_Cur)++) {
					case 'l': pBlk->lex_state = 0; return LEX_NULL;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 17: // number: minus sign
				switch(*pBlk->P_Cur) {
					case '0':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 18; // number: '0'
						break;
					case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 19; // number: decimal followup 
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 18: // number: '0'
				switch(*pBlk->P_Cur) {
					case '\x20': // space 
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++pBlk->P_Cur;
					case ']':
					case '}':
					case ',':
						pBlk->lex_state = 0;
						return LEX_NUMBER;
					case '.':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 20; // number: frac start 
						break;
					case 'e':
					case 'E':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 22; // number: exp start
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 19: // number: int followup
				switch(*pBlk->P_Cur) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++pBlk->P_Cur;
					case ']':
					case '}':
					case ',':
						pBlk->lex_state = 0;
						return LEX_NUMBER;
					case '.':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 20; // number: frac start
						break;
					case 'e':
					case 'E':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 22; // number: exp start
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 20: // number: frac start
				{
					switch(*pBlk->P_Cur) {
						case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
							pBlk->Text.CatChar(*pBlk->P_Cur);
							++pBlk->P_Cur;
							pBlk->lex_state = 21; // number: frac continue 
							break;
						default: return LEX_INVALID_CHARACTER;
					}
				}
				break;
			case 21: // number: frac continue
				switch(*pBlk->P_Cur) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++pBlk->P_Cur;
					case ']':
					case '}':
					case ',':
						pBlk->lex_state = 0;
						return LEX_NUMBER;
					case 'e':
					case 'E':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 22; // number: exp start
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 22: // number: exp start
				switch(*pBlk->P_Cur) {
					case '-':
					case '+':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 23; // number: exp continue 
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 24; // number: exp end 
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 23: // number: exp continue
				switch(*pBlk->P_Cur) {
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						pBlk->lex_state = 24; // number: exp end
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 24: // number: exp end
				switch(*pBlk->P_Cur) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++pBlk->P_Cur;
					case ']':
					case '}':
					case ',':
						pBlk->lex_state = 0;
						return LEX_NUMBER;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						pBlk->Text.CatChar(*pBlk->P_Cur);
						++pBlk->P_Cur;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			default: return LEX_INVALID_CHARACTER;
		}
	}
	pBlk->P_Cur = NULL;
	return LEX_MORE;
}
#endif // } 0 @construction

static int FASTCALL Lexer(const char * pBuffer, const char ** p, uint * state, SString & rText)
{
	assert(pBuffer);
	assert(p);
	assert(state);
	//assert(text);
	rText.Z();
	if(*p == NULL)
		*p = pBuffer;
	while(**p != '\0') {
		switch(*state) {
			case 0:	// Root document 
				switch(*(*p)++) {
					case '\x20': // space 
					case '\x09': // horizontal tab 
					case '\x0A': // line feed or new line 
					case '\x0D': // Carriage return 
						break;
					case '{': return LEX_BEGIN_OBJECT;
					case '}': return LEX_END_OBJECT;
					case '[': return LEX_BEGIN_ARRAY;
					case ']': return LEX_END_ARRAY;
					case ':': return LEX_NAME_SEPARATOR;
					case ',': return LEX_VALUE_SEPARATOR;
					case '\"':
						rText.Z();
						*state = 1;	/* inside a JSON string */
						break;
					case 't': *state =  7; break; // true: 1
					case 'f': *state = 10; break; // false: 1
					case 'n': *state = 14; break; // false: 1
					case '-':
						rText.Z().CatChar('-');
						*state = 17; // number: '0'
						break;
					case '0':
						rText.Z().CatChar('0');
						*state = 18; // number: '0' 
						break;
					case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.Z().CatChar(*(*p - 1));
						*state = 19; // number: decimal followup
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 1:	// inside a JSON string
				switch(**p) {
					case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9:
					case 10: // line feed
					case 11: case 12:
					case 13: // carriage return
					case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21: case 22:
					case 23: case 24: case 25: case 26: case 27: case 28: case 29: case 30: case 31:
						// ASCII control characters can only be present in a JSON string if they are escaped. If not then the document is invalid
						return LEX_INVALID_CHARACTER;
						break;
					case '\"':	// close JSON string
						// it is expected that, in the routine that calls this function, text is set to NULL
						*state = 0;
						++*p;
						return LEX_STRING;
						break;
					case '\\':
						rText.BSlash();
						*state = 2;	// inside a JSON string: start escape sequence
						break;
					default:
						rText.CatChar(**p);
						break;
				}
				++*p;
				break;
			case 2: // inside a JSON string: start escape sequence
				switch(**p) {
					case '\\':
					case '\"':
					case '/':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						rText.CatChar(**p);
						*state = 1;	// inside a JSON string
						break;
					case 'u':
						rText.CatChar(**p);
						*state = 3;	// inside a JSON string: escape unicode
						break;
					default:
						return LEX_INVALID_CHARACTER;
				}
				++*p;
				break;
			case 3: // inside a JSON string: escape unicode
				if(ishex(**p)) {
					rText.CatChar(**p);
					*state = 4; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
				break;
			case 4:	// inside a JSON string: escape unicode
				if(ishex(**p)) {
					rText.CatChar(**p);
					*state = 5; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
				break;
			case 5:	// inside a JSON string: escape unicode
				if(ishex(**p)) {
					rText.CatChar(**p);
					*state = 6; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
				break;
			case 6:	// inside a JSON string: escape unicode
				if(ishex(**p)) {
					rText.CatChar(**p);
					*state = 1; // inside a JSON string: escape unicode
				}
				else
					return LEX_INVALID_CHARACTER;
				++*p;
				break;
			case 7:	/* true: 1 */
				switch(*(*p)++) {
					case 'r': *state = 8; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 8:	/* true: 2 */
				switch(*(*p)++) {
					case 'u': *state = 9; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 9:	/* true: 3 */
				switch(*(*p)++) {
					case 'e': *state = 0; return LEX_TRUE;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 10:	/* false: 1 */
				switch(*(*p)++) {
					case 'a': *state = 11; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 11:	/* false: 2 */
				switch(*(*p)++) {
					case 'l': *state = 12; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 12:	/* false: 3 */
				switch(*(*p)++) {
					case 's': *state = 13; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 13:	/* false: 4 */
				switch(*(*p)++) {
					case 'e': *state = 0; return LEX_FALSE;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 14:	/* null: 1 */
				switch(*(*p)++) {
					case 'u': *state = 15; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 15:	/* null: 2 */
				switch(*(*p)++) {
					case 'l': *state = 16; break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 16:	/* null: 3 */
				switch(*(*p)++) {
					case 'l': *state = 0; return LEX_NULL;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 17: // number: minus sign
				switch(**p) {
					case '0':
						rText.CatChar(**p);
						++*p;
						*state = 18; // number: '0'
						break;
					case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						*state = 19;	/* number: decimal followup */
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 18:	/* number: '0' */
				switch(**p) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++*p;
					case ']':
					case '}':
					case ',':
						*state = 0;
						return LEX_NUMBER;
					case '.':
						rText.CatChar(**p);
						++*p;
						*state = 20; // number: frac start
						break;
					case 'e':
					case 'E':
						rText.CatChar(**p);
						++*p;
						*state = 22; // number: exp start
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 19: // number: int followup
				switch(**p) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++*p;
					case ']':
					case '}':
					case ',':
						*state = 0;
						return LEX_NUMBER;
					case '.':
						rText.CatChar(**p);
						++*p;
						*state = 20; // number: frac start
						break;
					case 'e':
					case 'E':
						rText.CatChar(**p);
						++*p;
						*state = 22; // number: exp start
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 20: // number: frac start
				{
					switch(**p) {
						case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
							rText.CatChar(**p);
							++*p;
							*state = 21;	/* number: frac continue */
							break;
						default: return LEX_INVALID_CHARACTER;
					}
				}
				break;
			case 21: // number: frac continue
				switch(**p) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++*p;
					case ']':
					case '}':
					case ',':
						*state = 0;
						return LEX_NUMBER;
					case 'e':
					case 'E':
						rText.CatChar(**p);
						++*p;
						*state = 22;	/* number: exp start */
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 22: // number: exp start
				switch(**p) {
					case '-':
					case '+':
						rText.CatChar(**p);
						++*p;
						*state = 23;	/* number: exp continue */
						break;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						*state = 24;	/* number: exp end */
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 23: // number: exp continue
				switch(**p) {
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						*state = 24;	/* number: exp end */
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			case 24: // number: exp end
				switch(**p) {
					case '\x20': // space
					case '\x09': // horizontal tab
					case '\x0A': // line feed or new line
					case '\x0D': // Carriage return
						++*p;
					case ']':
					case '}':
					case ',':
						*state = 0;
						return LEX_NUMBER;
					case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
						rText.CatChar(**p);
						++*p;
						break;
					default: return LEX_INVALID_CHARACTER;
				}
				break;
			default: return LEX_INVALID_CHARACTER;
		}
	}
	*p = NULL;
	return LEX_MORE;
}

/*enum json_error*/int FASTCALL json_parse_fragment(JsonParsingBlock & rBlk, const char * pBuffer)
{
	int    ok = 1;
	SJson * p_temp = 0;
	assert(pBuffer);
	rBlk.P_Cur = pBuffer;
	while(*rBlk.P_Cur != '\0') {
		switch(rBlk.state) {
			case 0:	// starting point 
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_BEGIN_OBJECT: rBlk.state = 1; break; // begin object
					case LEX_BEGIN_ARRAY:  rBlk.state = 7; break; // begin array
					case LEX_INVALID_CHARACTER: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT); break;
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT); break;
				}
				break;
			case 1: // open object
				if(!rBlk.cursor) {
					THROW(rBlk.cursor = SJson::CreateObj());
				}
				else {
					// perform tree sanity check
					assert(oneof2(rBlk.cursor->Type, SJson::tSTRING, SJson::tARRAY));
					THROW(p_temp = SJson::CreateObj());
					THROW(json_insert_child(rBlk.cursor, p_temp));
					rBlk.cursor = p_temp;
					p_temp = NULL;
				}
				rBlk.state = 2; // just entered an object
				break;
			case 2: // opened object
				//
				// perform tree sanity checks
				//
				assert(rBlk.cursor);
				assert(rBlk.cursor->Type == SJson::tOBJECT);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_STRING:
						THROW(p_temp = new SJson(SJson::tSTRING));
						p_temp->AssignText(rBlk.Text);
						THROW(json_insert_child(rBlk.cursor, p_temp));
						rBlk.cursor = p_temp;
						p_temp = NULL;
						rBlk.state = 5; // label, pre label:value separator
						break;
					case LEX_END_OBJECT:
						if(!rBlk.cursor->P_Parent)
							rBlk.state = 99; // finished document. only accept whitespaces until EOF
						else {
							rBlk.cursor = rBlk.cursor->P_Parent;
							switch(rBlk.cursor->Type) {
								case SJson::tSTRING: // perform tree sanity checks
									assert(rBlk.cursor->P_Parent);
									rBlk.cursor = rBlk.cursor->P_Parent;
									THROW_S(rBlk.cursor->Type == SJson::tOBJECT, SLERR_JSON_BAD_TREE_STRUCTURE);
									rBlk.state = 3; // finished adding a field to an object
									break;
								case SJson::tARRAY: rBlk.state = 9; break;
								default: CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE); break;
							}
						}
						break;
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT); break;
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT); break;
				}
				break;
			case 3: // finished adding a field to an object
				// perform tree sanity checks
				assert(rBlk.cursor);
				assert(rBlk.cursor->Type == SJson::tOBJECT);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_VALUE_SEPARATOR:
						rBlk.state = 4; // sibling, post-object
						break;
					case LEX_END_OBJECT:
						if(!rBlk.cursor->P_Parent)
							rBlk.state = 99;	/* parse until EOF */
						else {
							rBlk.cursor = rBlk.cursor->P_Parent;
							switch(rBlk.cursor->Type) {
								case SJson::tSTRING: // perform tree sanity checks
									assert(rBlk.cursor->P_Parent);
									rBlk.cursor = rBlk.cursor->P_Parent;
									THROW_S(rBlk.cursor->Type == SJson::tOBJECT, SLERR_JSON_BAD_TREE_STRUCTURE);
									rBlk.state = 3; // finished adding a field to an object
									break;
								case SJson::tARRAY:
									rBlk.state = 9;
									break;
								default: CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE); break;
							}
						}
						break;
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT); break;
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT); break;
				}
				break;
			case 4: // sibling, post-object
				assert(rBlk.cursor);
				assert(rBlk.cursor->Type == SJson::tOBJECT);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_STRING:
						THROW(p_temp = new SJson(SJson::tSTRING));
						p_temp->AssignText(rBlk.Text);
						THROW(json_insert_child(rBlk.cursor, p_temp));
						rBlk.cursor = p_temp;
						p_temp = 0;
						rBlk.state = 5;
						break;
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
					case LEX_INVALID_CHARACTER: CALLEXCEPT_S(SLERR_JSON_ILLEGAL_CHARACTER);
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
						//slfprintf_stderr("JSON: state %d: defaulted\n", rBlk.state);

				}
				break;
			case 5: // label, pre name separator
				/* perform tree sanity checks */
				assert(rBlk.cursor);
				assert(rBlk.cursor->Type == SJson::tSTRING);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_NAME_SEPARATOR: rBlk.state = 6; break; /* label, pos label:value separator */
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
						//slfprintf_stderr("JSON: state %d: defaulted\n", rBlk.state);
				}
				break;
			case 6: // label, pos name separator
				{
					// perform tree sanity checks 
					assert(rBlk.cursor);
					assert(rBlk.cursor->Type == SJson::tSTRING);
					const uint value = Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text);
					switch(value) {
						case LEX_STRING:
							THROW(p_temp = new SJson(SJson::tSTRING));
							p_temp->AssignText(rBlk.Text);
							THROW(json_insert_child(rBlk.cursor, p_temp));
							if(!rBlk.cursor->P_Parent)
								rBlk.state = 99; // finished document. only accepts whitespaces until EOF
							else
								rBlk.cursor = rBlk.cursor->P_Parent;
							p_temp = 0;
							rBlk.state = 3; // finished adding a field to an object
							break;
						case LEX_NUMBER:
							THROW(p_temp = new SJson(SJson::tNUMBER));
							p_temp->AssignText(rBlk.Text);
							THROW(json_insert_child(rBlk.cursor, p_temp));
							if(!rBlk.cursor->P_Parent)
								rBlk.state = 99; // finished document. only accepts whitespaces until EOF
							else
								rBlk.cursor = rBlk.cursor->P_Parent;
							p_temp = 0;
							rBlk.state = 3; // finished adding a field to an object
							break;
						case LEX_TRUE:
							THROW(p_temp = new SJson(SJson::tTRUE));
							THROW(json_insert_child(rBlk.cursor, p_temp));
							if(!rBlk.cursor->P_Parent)
								rBlk.state = 99; // finished document. only accepts whitespaces until EOF
							else
								rBlk.cursor = rBlk.cursor->P_Parent;
							p_temp = 0;
							rBlk.state = 3; // finished adding a field to an object
							break;
						case LEX_FALSE:
							THROW(p_temp = new SJson(SJson::tFALSE));
							THROW(json_insert_child(rBlk.cursor, p_temp));
							if(!rBlk.cursor->P_Parent)
								rBlk.state = 99; // finished document. only accepts whitespaces until EOF
							else
								rBlk.cursor = rBlk.cursor->P_Parent;
							p_temp = 0;
							rBlk.state = 3; // finished adding a field to an object
							break;
						case LEX_NULL:
							THROW(p_temp = new SJson(SJson::tNULL));
							THROW(json_insert_child(rBlk.cursor, p_temp));
							if(!rBlk.cursor->P_Parent)
								rBlk.state = 99;	/* finished document. only accepts whitespaces until EOF */
							else
								rBlk.cursor = rBlk.cursor->P_Parent;
							p_temp = NULL;
							rBlk.state = 3;	/* finished adding a field to an object */
							break;
						case LEX_BEGIN_OBJECT: rBlk.state = 1; break;
						case LEX_BEGIN_ARRAY:  rBlk.state = 7; break;
						case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
						case LEX_MEMORY: CALLEXCEPT_S(SLERR_NOMEM);
						case LEX_INVALID_CHARACTER: CALLEXCEPT_S(SLERR_JSON_ILLEGAL_CHARACTER);
						default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
							//slfprintf_stderr("JSON: state %d: defaulted\n", rBlk.state);
					}
				}
				break;
			case 7: // open array
				if(rBlk.cursor == NULL) {
					THROW(rBlk.cursor = SJson::CreateArr());
				}
				else { // perform tree sanity checks
					assert(oneof2(rBlk.cursor->Type, SJson::tARRAY, SJson::tSTRING));
					THROW(p_temp = SJson::CreateArr());
					THROW(json_insert_child(rBlk.cursor, p_temp));
					rBlk.cursor = p_temp;
					p_temp = 0;
				}
				rBlk.state = 8; // just entered an array
				break;
			case 8: // just entered an array
				// perform tree sanity checks
				assert(rBlk.cursor);
				assert(rBlk.cursor->Type == SJson::tARRAY);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_STRING:
						THROW(p_temp = new SJson(SJson::tSTRING));
						p_temp->AssignText(rBlk.Text);
						THROW(json_insert_child(rBlk.cursor, p_temp));
						p_temp = 0;
						rBlk.state = 9;	/* label, pre label:value separator */
						break;
					case LEX_NUMBER:
						THROW(p_temp = new SJson(SJson::tNUMBER));
						p_temp->AssignText(rBlk.Text);
						THROW(json_insert_child(rBlk.cursor, p_temp));
						p_temp = NULL;
						rBlk.state = 9;	/* label, pre label:value separator */
						break;
					case LEX_TRUE:
						THROW(p_temp = new SJson(SJson::tTRUE));
						THROW(json_insert_child(rBlk.cursor, p_temp));
						rBlk.state = 9;	/* label, pre label:value separator */
						break;
					case LEX_FALSE:
						THROW(p_temp = new SJson(SJson::tFALSE));
						THROW(json_insert_child(rBlk.cursor, p_temp));
						rBlk.state = 9;	/* label, pre label:value separator */
						break;
					case LEX_NULL:
						THROW(p_temp = new SJson(SJson::tNULL));
						THROW(json_insert_child(rBlk.cursor, p_temp));
						rBlk.state = 9;	/* label, pre label:value separator */
						break;
					case LEX_BEGIN_ARRAY: rBlk.state = 7; break; // open array
					case LEX_END_ARRAY:
						if(!rBlk.cursor->P_Parent) {
							// TODO implement this 
							rBlk.state = 99;	/* finished document. only accept whitespaces until EOF */
						}
						else {
							rBlk.cursor = rBlk.cursor->P_Parent;
							switch(rBlk.cursor->Type) {
								case SJson::tSTRING:
									THROW_S(rBlk.cursor->P_Parent, SLERR_JSON_BAD_TREE_STRUCTURE);
									rBlk.cursor = rBlk.cursor->P_Parent;
									THROW_S(rBlk.cursor->Type == SJson::tOBJECT, SLERR_JSON_BAD_TREE_STRUCTURE);
									rBlk.state = 3;	/* followup to adding child to array */
									break;
								case SJson::tARRAY: rBlk.state = 9; break; // followup to adding child to array
								default: CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
							}
						}
						break;
					case LEX_BEGIN_OBJECT: rBlk.state = 1; break; // open object
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
					case LEX_INVALID_CHARACTER: CALLEXCEPT_S(SLERR_JSON_ILLEGAL_CHARACTER);
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
						// slfprintf_stderr("JSON: state %d: defaulted\n", rBlk.state);
				}
				break;
			case 9: // followup to adding child to array
				// TODO perform tree sanity checks
				assert(rBlk.cursor);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_VALUE_SEPARATOR:
						rBlk.state = 8;
						break;
					case LEX_END_ARRAY:
						if(!rBlk.cursor->P_Parent)
							rBlk.state = 99; // finished document. only accept whitespaces until EOF
						else {
							rBlk.cursor = rBlk.cursor->P_Parent;
							switch(rBlk.cursor->Type) {
								case SJson::tSTRING:
									if(!rBlk.cursor->P_Parent) {
										rBlk.state = 99; // finished document. only accept whitespaces until EOF
									}
									else {
										rBlk.cursor = rBlk.cursor->P_Parent;
										THROW_S(rBlk.cursor->Type == SJson::tOBJECT, SLERR_JSON_BAD_TREE_STRUCTURE);
										rBlk.state = 3; // followup to adding child to array
									}
									break;
								case SJson::tARRAY: rBlk.state = 9; break; // followup to adding child to array
								default: CALLEXCEPT_S(SLERR_JSON_BAD_TREE_STRUCTURE);
							}
						}
						break;
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
				}
				break;
			case 99: // finished document. only accept whitespaces until EOF
				// perform tree sanity check
				assert(rBlk.cursor->P_Parent == NULL);
				switch(Lexer(pBuffer, &rBlk.P_Cur, &rBlk.lex_state, rBlk.Text)) {
					case LEX_MORE: CALLEXCEPT_S(SLERR_JSON_WAITING_FOR_EOF);
					case LEX_MEMORY: CALLEXCEPT_S(SLERR_NOMEM);
					default: CALLEXCEPT_S(SLERR_JSON_MALFORMED_DOCUMENT);
				}
				break;
			default: CALLEXCEPT_S(SLERR_JSON_UNKNOWN_PROBLEM);
		}
	}
	rBlk.P_Cur = NULL;
	/*
	if(rBlk.state == 99)
		return JSON_WAITING_FOR_EOF;
	else
		return JSON_INCOMPLETE_DOCUMENT;
	*/
	THROW_S(rBlk.state != 99, SLERR_JSON_WAITING_FOR_EOF);
	CALLEXCEPT_S(SLERR_JSON_INCOMPLETE_DOCUMENT);
	CATCHZOK
	return ok;
}

enum json_error json_parse_document(SJson ** ppRoot, const char * pText)
{
	enum json_error error = JSON_OK;
	if(!isempty(pText)) {
		assert(ppRoot);
		assert(*ppRoot == NULL);
		// initialize the parsing structure
		JsonParsingBlock jpi;
		error = json_parse_fragment(jpi, pText) ? JSON_OK : (enum json_error)SLibError;
		if(oneof3(error, SLERR_JSON_WAITING_FOR_EOF, JSON_WAITING_FOR_EOF, JSON_OK)) {
			*ppRoot = jpi.cursor;
			error = JSON_OK;
		}
	}
	else {
		error = JSON_EMPTY_DOCUMENT;
		SLS.SetError(SLERR_JSON_EMPTYDOCUMENT);
	}
	return error;
}

json_saxy_parser_status::json_saxy_parser_status() : State(0), StringLengthLimitReached(0), P_Temp(0)
{
}

int json_saxy_parser_status::StoreCharInTempString(char c)
{
	int    ok = 1;
	P_Temp = rcs_create(12);
	if(P_Temp) {
		if(rcs_catc(P_Temp, c) != RS_OK)
			ok = 0;
	}
	else
		ok = 0/*JSON_MEMORY*/;
	return ok;
}

void json_saxy_parser_status::FreeTempString()
{
	rcs_free(&P_Temp);
}

enum json_error json_saxy_parse(json_saxy_parser_status * jsps, json_saxy_functions * jsf, char c)
{
	// @todo handle a string instead of a single char
	// make sure everything is in it's place
	assert(jsps);
	assert(jsf);
	// goto where we left off
	switch(jsps->State) {
		case 0:  goto state0;  break; // general state. everything goes.
		case 1:  goto state1;  break; // parse string
		case 2:  goto state2;  break; // parse string: escaped character
		case 3:  goto state3;  break; // parse string: escaped unicode 1
		case 4:  goto state4;  break; // parse string: escaped unicode 2
		case 5:  goto state5;  break; /* parse string: escaped unicode 3 */
		case 6:  goto state6;  break; /* parse string: escaped unicode 4 */
		case 7:  goto state7;  break; /* parse true: tr */
		case 8:  goto state8;  break; /* parse true: tru */
		case 9:  goto state9;  break; /* parse true: true */
		case 10: goto state10; break; /* parse false: fa */
		case 11: goto state11; break; /* parse false: fal */
		case 12: goto state12; break; /* parse false: fals */
		case 13: goto state13; break; /* parse false: false */
		case 14: goto state14; break; /* parse null: nu */
		case 15: goto state15; break; /* parse null: nul */
		case 16: goto state16; break; /* parse null: null */
		case 17: goto state17; break; /* parse number: 0 */
		case 18: goto state18; break; /* parse number: start fraccional part */
		case 19: goto state19; break; /* parse number: fraccional part */
		case 20: goto state20; break; /* parse number: start exponent part */
		case 21: goto state21; break; /* parse number: exponent part */
		case 22: goto state22; break; /* parse number: exponent sign part */
		case 23: goto state23; break; /* parse number: start negative */
		case 24: goto state24; break; /* parse number: decimal part */
		case 25: goto state25; break; /* open object */
		case 26: goto state26; break; /* close object/array */
		case 27: goto state27; break; /* sibling followup */
		default: return JSON_UNKNOWN_PROBLEM; /* oops... this should never be reached */
	}
	state0: // starting point
	{
		switch(c) {
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				break;
			case '\"': // starting a string
				jsps->StringLengthLimitReached = 0;
				jsps->State = 1;
				break;
			case '{':
				if(jsf->open_object)
					jsf->open_object();
				jsps->State = 25; // open object
				break;
			case '}':
				if(jsf->close_object)
					jsf->close_object();
				jsps->State = 26; // close object/array
				break;
			case '[':
				if(jsf->open_array)
					jsf->open_array();
				// jsps->State = 0; // redundant
				break;
			case ']':
				if(jsf->close_array)
					jsf->close_array();
				jsps->State = 26;	/* close object/array */
				break;
			case 't': jsps->State = 7; break; // parse true: tr
			case 'f': jsps->State = 10; break; // parse false: fa
			case 'n': jsps->State = 14; break; // parse null: nu
			case ':':
				if(jsf->label_value_separator)
					jsf->label_value_separator();
				// jsps->State = 0; // redundant
				break;
			case ',':
				if(jsf->sibling_separator)
					jsf->sibling_separator();
				jsps->State = 27;	/* sibling followup */
				break;
			case '0':
				jsps->StringLengthLimitReached = 0;
				jsps->State = 17;	/* parse number: 0 */
				if(!jsps->StoreCharInTempString('0'))
					return JSON_MEMORY;
				break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				jsps->StringLengthLimitReached = 0;
				jsps->State = 24;	/* parse number: decimal */
				if(!jsps->StoreCharInTempString(c))
					return JSON_MEMORY;
				break;
			case '-':
				jsps->StringLengthLimitReached = 0;
				jsps->State = 23;	/* number: */
				jsps->P_Temp = NULL;
				if(!jsps->StoreCharInTempString('-'))
					return JSON_MEMORY;
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state1: // parse string
	{
		switch(c) {
			case '\\':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH-1)) // check if there is space for a two character escape sequence
						rcs_catc((jsps->P_Temp), '\\');
					else
						jsps->StringLengthLimitReached = 1;
				}
				jsps->State = 2;	/* parse string: escaped character */
				break;
			case '\"':	/* end of string */
				if(jsps->P_Temp) {
					jsps->State = 0;	/* starting point */
					if(jsf->new_string)
						jsf->new_string(jsps->P_Temp->P_Text); /*copied or integral? */
					jsps->FreeTempString();
				}
				else
					return JSON_UNKNOWN_PROBLEM;
				break;
			default:
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH) // check if there is space for a two character escape sequence
						rcs_catc((jsps->P_Temp), c);
					else
						jsps->StringLengthLimitReached = 1;
				}
				break;
		}
		return JSON_OK;
	}
state2: // parse string: escaped character
		switch(c) {
			case '\"':
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH)
						rcs_catc((jsps->P_Temp), c);
					else
						jsps->StringLengthLimitReached = 1;
				}
				break;
			case 'u':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH - 4))
						rcs_catc((jsps->P_Temp), 'u');
					else
						jsps->StringLengthLimitReached = 1;
				}
				jsps->State = 3;	/* parse string: escaped unicode 1; */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
state3: // parse string: escaped unicode 1
		if(ishex(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH - 3))
					rcs_catc((jsps->P_Temp), 'u');
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 4; // parse string. escaped unicode 2
			return JSON_OK;
		}
		else
			return JSON_ILLEGAL_CHARACTER;
state4: // parse string: escaped unicode 2
		if(ishex(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH - 2)
					rcs_catc((jsps->P_Temp), c);
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 5;	/* parse string. escaped unicode 3 */
			return JSON_OK;
		}
		else
			return JSON_ILLEGAL_CHARACTER;
state5: // parse string: escaped unicode 3
		if(ishex(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH-1))
					rcs_catc((jsps->P_Temp), c);
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 6; // parse string. escaped unicode 4
			return JSON_OK;
		}
		else
			return JSON_ILLEGAL_CHARACTER;
state6: // parse string: escaped unicode 4
		if(ishex(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH)
					rcs_catc((jsps->P_Temp), c);
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 1;	/* parse string */
			return JSON_OK;
		}
		else
			return JSON_ILLEGAL_CHARACTER;
state7: // parse true: tr
		if(c != 'r') {
			return JSON_ILLEGAL_CHARACTER;
		}
		else {
			jsps->State = 8; // parse true: tru
			return JSON_OK;
		}
state8: // parse true: tru
		if(c != 'u')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 9;	/* parse true: true */
			return JSON_OK;
		}
state9: // parse true: true
		if(c != 'e')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 0;	/* back to general state. */
			if(jsf->new_true)
				jsf->new_true();
			return JSON_OK;
		}
state10: // parse false: fa
		if(c != 'a')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 11;	/* parse true: fal */
			return JSON_OK;
		}
state11: // parse false: fal
		if(c != 'l')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 12;	/* parse true: fals */
			return JSON_OK;
		}
state12: // parse false: fals
		if(c != 's')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 13;	/* parse true: false */
			return JSON_OK;
		}
state13: // parse false: false
		if(c != 'e')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 0;	/* general state. everything goes. */
			if(jsf->new_false)
				jsf->new_false();
			return JSON_OK;
		}
state14: // parse null: nu
		if(c != 'u')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 15;	/* parse null: nul */
			return JSON_OK;
		}
state15: // parse null: nul
		if(c != 'l')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 16;	/* parse null: null */
			return JSON_OK;
		}
state16: // parse null: null
		if(c != 'l')
			return JSON_ILLEGAL_CHARACTER;
		else {
			jsps->State = 0;	/* general state. everything goes. */
			if(jsf->new_null)
				jsf->new_null();
			return JSON_OK;
		}
state17: // parse number: 0
	{
		switch(c) {
		case '.':
			if(!jsps->StoreCharInTempString('.'))
				return JSON_MEMORY;
			jsps->State = 18;	/* parse number: fraccional part */
			break;
		case '\x20':
		case '\x09':
		case '\x0A':
		case '\x0D':	/* JSON insignificant white spaces */
			if(!jsps->P_Temp)
				return JSON_MEMORY;
			if(jsf->new_number)
				jsf->new_number((jsps->P_Temp)->P_Text);
			jsps->FreeTempString();
			jsps->State = 0;
			break;
		case '}':
			if(!jsps->P_Temp)
				return JSON_MEMORY;
			if(jsf->new_number)
				jsf->new_number((jsps->P_Temp)->P_Text);
			jsps->FreeTempString();
			if(jsf->open_object)
				jsf->close_object();
			jsps->State = 26;	/* close object/array */
			break;
		case ']':
			if(!jsps->P_Temp)
				return JSON_MEMORY;
			if(jsf->new_number)
				jsf->new_number((jsps->P_Temp)->P_Text);
			jsps->FreeTempString();
			if(jsf->open_object)
				jsf->close_array();
			jsps->State = 26; // close object/array
			break;
		case ',':
			if(!jsps->P_Temp)
				return JSON_MEMORY;
			if(jsf->new_number)
				jsf->new_number((jsps->P_Temp)->P_Text);
			jsps->FreeTempString();
			if(jsf->open_object)
				jsf->label_value_separator();
			jsps->State = 27;	/* sibling followup */
			break;
		default:
			return JSON_ILLEGAL_CHARACTER;
			break;
		}
		return JSON_OK;
	}
state18: // parse number: start fraccional part
      	if(isdec(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH / 2))
					rcs_catc((jsps->P_Temp), c);
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 19;	/* parse number: fractional part */
			return JSON_OK;
      	}
      	else
			return JSON_ILLEGAL_CHARACTER;
state19: // parse number: fraccional part
	{
		switch(c) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH/2))
						rcs_catc((jsps->P_Temp), c);
					else
						jsps->StringLengthLimitReached = 1;
				}
				// jsps->State = 19; // parse number: fractional part
				break;
			case 'e':
			case 'E':
				rcs_catc((jsps->P_Temp), c);
				jsps->State = 20;	/* parse number: start exponent part */
				break;
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				jsps->State = 0;
				break;
			case '}':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->open_object)
					jsf->close_object();
				jsps->State = 26;	/* close object/array */
				break;
			case ']':
				if(jsf->new_number) {
					if(!jsps->P_Temp)
						return JSON_MEMORY;
					jsf->new_number((jsps->P_Temp)->P_Text);
					jsps->FreeTempString();
				}
				else {
					jsps->FreeTempString();
					jsps->P_Temp = NULL;
				}
				if(jsf->open_object)
					jsf->close_array();
				jsps->State = 26;	/* close object/array */
				break;
			case ',':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->label_value_separator)
					jsf->label_value_separator();
				jsps->State = 27;	/* sibling followup */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
	state20:			/* parse number: start exponent part */
	{
		switch(c) {
			case '+':
			case '-':
				jsps->StringLengthLimitReached = 0;
				rcs_catc((jsps->P_Temp), c);
				jsps->State = 22;	/* parse number: exponent sign part */
				break;
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH)
						rcs_catc((jsps->P_Temp), c);
					else
						jsps->StringLengthLimitReached = 1;
				}
				jsps->State = 21;	/* parse number: exponent part */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state21: // parse number: exponent part
	{
		switch(c) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH)
						rcs_catc((jsps->P_Temp), c);
					else
						jsps->StringLengthLimitReached = 1;
				}
				// jsps->State = 21; // parse number: exponent part
				break;
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				jsps->State = 0;
				break;
			case '}':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->open_object)
					jsf->close_object();
				jsps->State = 26;	/* close object */
				break;
			case ']':
				if(jsf->new_number) {
					if(!jsps->P_Temp)
						return JSON_MEMORY;
					jsf->new_number((jsps->P_Temp)->P_Text);
					ZFREE(jsps->P_Temp);
				}
				else {
					ZFREE(jsps->P_Temp);
				}
				if(jsf->open_object)
					jsf->close_array();
				jsps->State = 26;	/* close object/array */
				break;
			case ',':
				if(jsf->new_number) {
					if(!jsps->P_Temp)
						return JSON_MEMORY;
					jsf->new_number((jsps->P_Temp)->P_Text);
					ZFREE(jsps->P_Temp);
				}
				else {
					ZFREE(jsps->P_Temp);
				}
				if(jsf->label_value_separator)
					jsf->label_value_separator();
				jsps->State = 27;	/* sibling followup */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state22: // parse number: start exponent part
		if(isdec(c)) {
			if(!jsps->StringLengthLimitReached) {
				if(jsps->P_Temp->Len() < JSON_MAX_STRING_LENGTH)
					rcs_catc((jsps->P_Temp), c);
				else
					jsps->StringLengthLimitReached = 1;
			}
			jsps->State = 21;	/* parse number: exponent part */
			return JSON_OK;
		}
		else
			return JSON_ILLEGAL_CHARACTER;
state23: // parse number: start negative
	{
		switch(c) {
			case '0':
				rcs_catc((jsps->P_Temp), c);
				jsps->State = 17;	/* parse number: 0 */
				break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH / 2)) {
						if(!jsps->StoreCharInTempString(c))
							return JSON_MEMORY;
					}
					else
						jsps->StringLengthLimitReached = 1;
				}
				jsps->State = 24;	/* parse number: start decimal part */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state24: // parse number: decimal part
	{
		switch(c) {
			case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				if(!jsps->StringLengthLimitReached) {
					if(jsps->P_Temp->Len() < (JSON_MAX_STRING_LENGTH / 2)) {
						if(!jsps->StoreCharInTempString(c))
							return JSON_MEMORY;
					}
					else
						jsps->StringLengthLimitReached = 1;
				}
				/* jsps->State = 24; // parse number: decimal part*/
				break;
			case '.':
				if(!jsps->StoreCharInTempString('.'))
					return JSON_MEMORY;
				jsps->State = 18;	/* parse number: start exponent part */
				break;
			case 'e':
			case 'E':
				if(!jsps->StoreCharInTempString(c))
					return JSON_MEMORY;
				jsps->StringLengthLimitReached = 0;	/* reset to accept the exponential part */
				jsps->State = 20;	/* parse number: start exponent part */
				break;
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				jsps->State = 0;
				break;
			case '}':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->open_object)
					jsf->close_object();
				jsps->State = 26;	/* close object/array */
				break;
			case ']':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->open_object)
					jsf->close_array();
				jsps->State = 26;	/* close object/array */
				break;
			case ',':
				if(!jsps->P_Temp)
					return JSON_MEMORY;
				if(jsf->new_number)
					jsf->new_number((jsps->P_Temp)->P_Text);
				jsps->FreeTempString();
				if(jsf->label_value_separator)
					jsf->label_value_separator();
				jsps->State = 27;	/* sibling followup */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state25:			/* open object */
	{
		switch(c) {
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				break;
			case '\"':
				jsps->P_Temp = NULL;
				jsps->State = 1;
				break;
			case '}':
				if(jsf->close_object)
					jsf->close_object();
				jsps->State = 26; // close object
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state26: // close object/array
	{
		switch(c) {
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				break;
			case '}':
				if(jsf->close_object)
					jsf->close_object();
				// jsp->state = 26; // close object
				break;
			case ']':
				if(jsf->close_array)
					jsf->close_array();
				// jsps->State = 26;       // close object/array
				break;
			case ',':
				if(jsf->sibling_separator)
					jsf->sibling_separator();
				jsps->State = 27;	/* sibling followup */
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
state27: // sibling followup
	{
		switch(c) {
			case '\x20':
			case '\x09':
			case '\x0A':
			case '\x0D': // JSON insignificant white spaces
				break;
			case '\"':
				jsps->State = 1;
				jsps->P_Temp = NULL;
				break;
			case '{':
				if(jsf->open_object)
					jsf->open_object();
				jsps->State = 25;	/*open object */
				break;
			case '[':
				if(jsf->open_array)
					jsf->open_array();
				// jsps->State = 0; // redundant
				break;
			case 't': jsps->State = 7; break; // parse true: tr
			case 'f': jsps->State = 10; break; // parse false: fa
			case 'n': jsps->State = 14; break; // parse null: nu
			case '0':
				jsps->State = 17; // parse number: 0
				if(!jsps->StoreCharInTempString('0'))
					return JSON_MEMORY;
				break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
				jsps->State = 24;	/* parse number: decimal */
				if(!jsps->StoreCharInTempString(c))
					return JSON_MEMORY;
				break;
			case '-':
				jsps->State = 23;	/* number: */
				if(!jsps->StoreCharInTempString('-'))
					return JSON_MEMORY;
				break;
			default:
				return JSON_ILLEGAL_CHARACTER;
				break;
		}
		return JSON_OK;
	}
	return JSON_UNKNOWN_PROBLEM;
}

SJson * json_find_first_label(const SJson * object, const char * text_label)
{
	SJson * cursor;
	assert(object);
	assert(text_label);
	assert(object->Type == SJson::tOBJECT);
	for(cursor = object->P_Child; cursor; cursor = cursor->P_Next)
		if(cursor->Text == text_label)
			break;
	return cursor;
}

const char * json_get_value(const SJson * object, const char * text_label)
{
	SJson * cursor = json_find_first_label(object, text_label);
	return (cursor && cursor->P_Child) ? cursor->P_Child->Text.cptr() : 0;
}

SJson * json_process(SJson * cursor)
{
	int exit_flg = 0;
	switch(cursor->Type) {
		case SJson::tARRAY:
		case SJson::tOBJECT:
			if(cursor->P_Child) {
				cursor = cursor->P_Child;
				return cursor;
			}
			else
				cursor = cursor->P_Next;
			break;
	}
	if(cursor->P_Child) {
		cursor = cursor->P_Child;
	}
	else {
		while(1) {
			if(cursor->P_Parent == NULL)
				break;
			else if(cursor->P_Next) {
				cursor = cursor->P_Next;
				break;
			}
			else {
				cursor = cursor->P_Parent;
				continue;
			}
		}
	}
	return cursor;
}
//
//
//
#if SLTEST_RUNNING // {
//
// @construction
//
static SString & Helper_JsonToXmlEncTag(const char * pText, const char * pDefText, SString & rBuf)
{
	if(pText) {
		XMLReplaceSpecSymb(rBuf = pText, "&<>\'");
	}
	else {
		rBuf = pDefText;
	}
	rBuf.ReplaceChar(' ', '_');
	return rBuf;
}

static SString & Helper_JsonToXmlEncText(const char * pText, const char * pDefText, SString & rBuf)
{
	if(pText) {
		XMLReplaceSpecSymb(rBuf = pText, "&<>\'");
	}
	else {
		rBuf = pDefText;
	}
	return rBuf;
}

static int Helper_JsonToXmlDOM(SJson * pNode, int superJsonType, SXml::WDoc & rXmlDoc)
{
	int    ok = 1;
	SString temp_buf;
	SString val_buf;
	for(SJson * p_cur = pNode; p_cur; p_cur = p_cur->P_Next) {
		switch(p_cur->Type) {
			case SJson::tARRAY:
				{
					//SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag((pNode->P_Parent ? pNode->P_Parent->P_Text : 0), "array", temp_buf));
					THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
				}
				break;
			case SJson::tOBJECT:
				{
					//SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag((pNode->P_Parent ? pNode->P_Parent->P_Text : 0), "object", temp_buf));
					THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
				}
				break;
			case SJson::tSTRING:
			case SJson::tNUMBER:
				{
					SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag(p_cur->Text, "value", temp_buf));
					if(p_cur->P_Child) {
						THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
					}
					else {
						//SXml::WNode xn(rXmlDoc, "value", Helper_JsonToXmlEncText(p_cur->P_Text, 0, val_buf));
					}
				}
				break;
			case SJson::tTRUE:
				if(p_cur->P_Child) {
					THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
				}
				else {
					SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag(p_cur->Text, "bool", temp_buf), "true");
				}
				break;
			case SJson::tFALSE:
				if(p_cur->P_Child) {
					THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
				}
				else {
					SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag(p_cur->Text, "bool", temp_buf), "false");
				}
				break;
			case SJson::tNULL:
				if(p_cur->P_Child) {
					THROW(Helper_JsonToXmlDOM(p_cur->P_Child, p_cur->Type, rXmlDoc));   // @recursion
				}
				else {
					SXml::WNode xn(rXmlDoc, Helper_JsonToXmlEncTag(p_cur->Text, "null", temp_buf), "null");
				}
				break;
			default:
				break;
		}
	}
	CATCHZOK
	return ok;
}

int JsonToXmlDOM(const char * pJsonText, SString & rXmlText)
{
	rXmlText = 0;
	int    ok = 1;
	SJson * p_json_doc = NULL;
	xmlTextWriter * p_writer = 0;
	xmlBuffer * p_xml_buf = 0;
	THROW(json_parse_document(&p_json_doc, pJsonText) == JSON_OK);
	{
		THROW(p_xml_buf = xmlBufferCreate());
		THROW(p_writer = xmlNewTextWriterMemory(p_xml_buf, 0));
		xmlTextWriterSetIndent(p_writer, 1);
        {
        	SXml::WDoc _doc(p_writer, cpUTF8);
        	THROW(Helper_JsonToXmlDOM(p_json_doc, 0, _doc));
			xmlTextWriterFlush(p_writer);
			rXmlText.CopyFromN((char *)p_xml_buf->content, p_xml_buf->use);
        }
	}
	CATCHZOK
	xmlFreeTextWriter(p_writer);
	xmlBufferFree(p_xml_buf);
	json_free_value(&p_json_doc);
	return ok;
}

int XmlToJsonDOM(const char * pXmlText, SString & rJsonText)
{
	int    ok = 1;
	return ok;
}

SLTEST_R(JSON)
{
	int    ok = 1;
	struct InputEntry {
		const char * P_InFileName;
		int    Valid;
	};
	static const InputEntry inp_entries[] = {
		{ "binary_dependencies.json", 1 },
		{ "tests.json", 1 },
		{ "pass1.json", 1 },
		{ "pass2.json", 1 },
		{ "pass3.json", 1 },
		{ "USStates.json", 1 },
	};
	for(uint i = 0; i < SIZEOFARRAY(inp_entries); i++) {
		SString in_file_name(MakeInputFilePath(inp_entries[i].P_InFileName));
		THROW(fileExists(in_file_name));
		{
			SString json_buf;
			SString xml_buf;
			SString temp_buf;
			SFile f_in(in_file_name, SFile::mRead);
			THROW(f_in.IsValid());
			{
				STempBuffer rd_buf(4096);
				size_t actual_size = 0;
				THROW(rd_buf.IsValid());
				while(f_in.Read(rd_buf, rd_buf.GetSize(), &actual_size) && actual_size) {
					json_buf.CatN(rd_buf, actual_size);
				}
				SLCHECK_NZ(JsonToXmlDOM(json_buf, xml_buf));
				{
					SPathStruc ps(in_file_name);
					ps.Nam.CatChar('-').Cat("result");
					ps.Ext = "xml";
					ps.Drv.Z();
					ps.Dir.Z();
					ps.Merge(temp_buf);
					SString out_file_name(MakeOutputFilePath(temp_buf));
					SFile f_out(out_file_name, SFile::mWrite);
					THROW(f_out.IsValid());
					f_out.WriteLine(xml_buf);
				}
			}
		}
	}
	CATCHZOK
	return CurrentStatus;
}

#endif
