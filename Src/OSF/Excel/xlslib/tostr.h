/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TOSTR_H
#define TOSTR_H

class str_stream;

// ... helper - allow explicit conversion to string
class as_string
{
};

inline std::ostream & operator<< ( std::ostream & streamOut, const as_string &)
{
	return streamOut;
}

namespace Private
{
	// what should we return when calling write_to_stream ?
	template< class type>
	class return_from_write_to_stream
	{
	public:
		typedef const str_stream & return_type;
	};

	template<>
	class return_from_write_to_stream< as_string>
	{
	public:
		typedef std::string return_type;
	};

	// forward declaration
	template< class type>
	inline typename return_from_write_to_stream< type>::return_type
	write_to_stream ( const str_stream & streamOut, const type & value);
}


// forward declaration
template<class type>
inline typename Private::return_from_write_to_stream<type>::return_type operator<< (const str_stream & streamOut, const type & value);

// str_stream - allow stream usage, and then conversion to string
class str_stream
{
public:
	// default construction
	str_stream(){}

	// allow to_string like usage
	template<class type>
	str_stream(const type & value)
	{
		*this << value;
	}

	std::stringstream & underlying_stream() const
	{ return m_streamOut; }

	operator std::string() const
	{
		return m_streamOut.str();
	}

private:
	mutable std::stringstream m_streamOut;

#ifndef NDEBUG
public:
	void recalculate_string() const
	{ m_string = m_streamOut.str(); }
private:
	mutable std::string m_string;
#endif
};

namespace Private
{
	template< class type>
	inline typename return_from_write_to_stream< type>::return_type
	write_to_stream ( const str_stream & streamOut, const type & value)
	{
#if ((__BCPLUSPLUS__ >= 00) && (__BCPLUSPLUS__ <= 0x0600))
		// I am not sure why Borland Development Suite (2006) complains
		// about ambiguous << operator. RLN 111215
		// RadStudio 2010 doesn't compain I am presuming R.S. 2009 is also OK
		streamOut.underlying_stream().operator<<(value);
#else
		streamOut.underlying_stream()<< value;
#endif
#ifndef NDEBUG
		streamOut.recalculate_string();
#endif
		return streamOut;
	}
}

template< class type>
inline typename Private::return_from_write_to_stream< type>::return_type operator<< ( const str_stream & streamOut, const type & value)
{
	return Private::write_to_stream( streamOut, value);
}

// allow function IO manipulators
inline const str_stream & operator<< ( const str_stream & streamOut, std::ios_base & (*func)( std::ios_base&) )
{
	func( streamOut.underlying_stream());
	return streamOut;
}

inline const str_stream & operator<< ( const str_stream & streamOut, std::basic_ios< char> & (*func)(std::basic_ios< char> &) )
{
	func( streamOut.underlying_stream());
	return streamOut;
}

inline const str_stream & operator<< ( const str_stream & streamOut, std::ostream & (*func)( std::ostream &) )
{
	func( streamOut.underlying_stream());
	return streamOut;
}

#endif
