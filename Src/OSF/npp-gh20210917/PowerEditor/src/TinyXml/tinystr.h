/*
   www.sourceforge.net/projects/tinyxml
   Original file by Yves Berquin.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product documentation
   would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
 */

#ifndef TINYXML_INCLUDED
#include "tinyxml.h"
#endif //TINYXML_INCLUDED
#include <cassert>

#ifndef TIXML_USE_STL

#ifndef TIXML_STRING_INCLUDED
#define TIXML_STRING_INCLUDED

#pragma warning( disable : 4514 )

/*
   TiXmlString is an emulation of the string template.
   Its purpose is to allow compiling TinyXML on compilers with no or poor STL support.
   Only the member functions relevant to the TinyXML project have been implemented.
   The buffer allocation is made by a simplistic power of 2 like mechanism : if we increase
   a generic_string and there's no more room, we allocate a buffer twice as big as we need.
 */
class TiXmlString {
public:
	// TiXmlString constructor, based on a generic_string
	TiXmlString (const TCHAR * instring);

	// TiXmlString empty constructor
	TiXmlString ()
	{
		allocated = 0;
		cstring = NULL;
		current_length = 0;
	}

	// TiXmlString copy constructor
	TiXmlString (const TiXmlString& copy);

	// TiXmlString destructor
	~TiXmlString ()
	{
		empty_it();
	}

	// Convert a TiXmlString into a classical TCHAR *
	const TCHAR * c_str() const
	{
		if(allocated)
			return cstring;
		return TEXT("");
	}

	// Return the length of a TiXmlString
	unsigned length() const
	{
		return ( allocated ) ? current_length : 0;
	}

	// TiXmlString = operator
	void operator =(const TCHAR * content);

	// = operator
	void operator =(const TiXmlString & copy);

	// += operator. Maps to append
	TiXmlString& operator +=(const TCHAR * suffix)
	{
		append(suffix);
		return *this;
	}

	// += operator. Maps to append
	TiXmlString& operator +=(TCHAR single)
	{
		append(single);
		return *this;
	}
	// += operator. Maps to append
	TiXmlString& operator +=(TiXmlString & suffix)
	{
		append(suffix);
		return *this;
	}
	bool operator == (const TiXmlString & compare) const;
	bool operator <(const TiXmlString & compare) const;
	bool operator >(const TiXmlString & compare) const;

	// Checks if a TiXmlString is empty
	bool empty() const { return length() ? false : true; }

	// Checks if a TiXmlString contains only whitespace (same rules as isspace)
	// Not actually used in tinyxml. Conflicts with a C macro, TEXT("isblank"),
	// which is a problem. Commenting out. -lee
//    bool isblank () const;

	// single TCHAR extraction
	const TCHAR& at(unsigned index) const
	{
		assert(index < length());
		return cstring [index];
	}

	// find a TCHAR in a generic_string. Return TiXmlString::notfound if not found
	unsigned find(TCHAR lookup) const
	{
		return find(lookup, 0);
	}

	// find a TCHAR in a generic_string from an offset. Return TiXmlString::notfound if not found
	unsigned find(TCHAR tofind, unsigned offset) const;

	/*	Function to reserve a big amount of data when we know we'll need it. Be aware that this
	            function clears the content of the TiXmlString if any exists.
	 */
	void reserve(unsigned size)
	{
		empty_it();
		if(size) {
			allocated = size;
			cstring = new TCHAR [size];
			cstring [0] = 0;
			current_length = 0;
		}
	}

	// [] operator
	TCHAR& operator [](unsigned index) const
	{
		assert(index < length());
		return cstring [index];
	}

	// Error value for find primitive
	enum {      notfound = 0xffffffff,
		    npos = notfound };

	void append(const TCHAR * str, int len);

protected:

	// The base generic_string
	TCHAR * cstring;
	// Number of chars allocated
	unsigned allocated;
	// Current generic_string size
	unsigned current_length;

	// New size computation. It is simplistic right now : it returns twice the amount
	// we need
	unsigned assign_new_size(unsigned minimum_to_allocate)
	{
		return minimum_to_allocate * 2;
	}

	// Internal function that clears the content of a TiXmlString
	void empty_it()
	{
		if(cstring)
			delete [] cstring;
		cstring = NULL;
		allocated = 0;
		current_length = 0;
	}

	void append(const TCHAR * suffix);

	// append function for another TiXmlString
	void append(const TiXmlString & suffix)
	{
		append(suffix.c_str());
	}

	// append for a single TCHAR. This could be improved a lot if needed
	void append(TCHAR single)
	{
		TCHAR smallstr [2];
		smallstr [0] = single;
		smallstr [1] = 0;
		append(smallstr);
	}
};

/*
   TiXmlOutStream is an emulation of std::ostream. It is based on TiXmlString.
   Only the operators that we need for TinyXML have been developped.
 */
class TiXmlOutStream : public TiXmlString
{
public:
	TiXmlOutStream () : TiXmlString() 
	{
	}
	// TiXmlOutStream << operator. Maps to TiXmlString::append
	TiXmlOutStream & operator <<(const TCHAR * in)
	{
		append(in);
		return (*this);
	}
	// TiXmlOutStream << operator. Maps to TiXmlString::append
	TiXmlOutStream & operator <<(const TiXmlString & in)
	{
		append(in.c_str());
		return (*this);
	}
};

#endif  // TIXML_STRING_INCLUDED
#endif  // TIXML_USE_STL
