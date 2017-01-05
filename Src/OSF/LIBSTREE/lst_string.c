/*

   Copyright (C) 2003-2006 Christian Kreibich <christian@whoop.org>.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies of the Software and its documentation and acknowledgment shall be
   given in the documentation and software packages that this Software was
   used.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */
#include "libstree.h"
#include <sys/param.h>

static int string_id_counter;
static int   string_byte_cmp_func(char * item1, char * item2);
static void  string_byte_copy_func(char * src, char * dst);

SString & LstString::ToStr(SString & rBuf)
{
	rBuf = 0;
	LstStringIndex tmp_range;
	tmp_range.P_String = this;
	tmp_range.StartIndex  = 0;
	*(tmp_range.P_EndIndex) = getCount();
	tmp_range.ExtraIndex  = 0;
	return tmp_range.Print(rBuf);
}	

LstString * lst_string_new(const void * data, uint item_size, uint num_items)
{
	LstString * string = 0;
	if(item_size) {
		string = new LstString(item_size);
		if(string) {
			string->Id = ++string_id_counter;
			if(data && num_items) {
				//
				// Logically, we want one more item than given; we treat that as a
				// special end-of-string marker so that no suffix of our string can ever
				// be the prefix of another suffix. For the problems that this would cause, see Gusfield.
				//
				string->insertChunk(num_items, data);
			}
		}
	}
	return string;
}

int lst_string_eq(const LstString * s1, uint item1, const LstString * s2, uint item2)
{
	if(!s1 || !s2 || item1 >= (s1->getCount()+1) || item2 >= (s2->getCount()+1))
		return 0;
	/* Treat the end-of-string markers separately: */
	if(item1 == s1->getCount() || item2 == s2->getCount()) {
		if(item1 == s1->getCount() && item2 == s2->getCount()) {
			if(s1 == s2) {
				D0("Comparing end of identical strings\n");
				return 1;
			}
			else {
				D0("Comparing end of different strings\n");
				return 0;
			}
		}
		else {
			D0("Comparing end and non-end\n");
			return 0;
		}
	}
	const size_t is1 = s1->getItemSize();
	const size_t is2 = s2->getItemSize();
	if(is1 != is2)
		return 0;
	else {
		const void * p1 = s1->at(item1);
		const void * p2 = s2->at(item2);
		switch(is1) {
			case 1: 
				return BIN(*PTR8(p1) == *PTR8(p2));
			case 2:
				return BIN(*PTR16(p1) == *PTR16(p2));
			case 4:
				return BIN(*PTR32(p1) == *PTR32(p2));
			default:
				return BIN(memcmp(p1, p2, is1) == 0);
		}
	}
}

uint lst_string_items_common(const LstString * s1, uint off1, const LstString * s2, uint off2, uint max_len)
{
	if(!s1 || !s2 || off1 > s1->getCount() || off2 > s2->getCount())
		return 0;
	uint   len = MIN(MIN((s1->getCount() + 1) - off1, (s2->getCount() + 1) - off2), max_len);
	for(uint i = 0; i < len; i++) {
		if(!lst_string_eq(s1, off1 + i, s2, off2 + i))
			return i;
	}
	return len;
}

void lst_string_index_copy(LstStringIndex * src, LstStringIndex * dst)
{
	if(src && dst) {
		dst->P_String = src->P_String;
		dst->StartIndex = src->StartIndex;
		*(dst->P_EndIndex) = *(src->P_EndIndex);
		dst->ExtraIndex = src->ExtraIndex;
	}
}

