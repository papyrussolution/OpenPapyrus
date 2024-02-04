/** @file
 * @brief Handle decoding lists of strings with byte lengths
 */
// Copyright (C) 2004,2005,2006,2007,2008,2009,2010 Olly Betts
// @license GNU GPL
//
#ifndef XAPIAN_INCLUDED_BYTE_LENGTH_STRINGS_H
#define XAPIAN_INCLUDED_BYTE_LENGTH_STRINGS_H

// We XOR the length values with this so that they are more likely to coincide
// with lower case ASCII letters, which are likely to be common.  This means
// that zlib should do a better job of compressing tag values - in tests, this
// gave 5% better compression.
#define MAGIC_XOR_VALUE 96

class ByteLengthPrefixedStringItor {
	const uchar * p;
	size_t left;

	ByteLengthPrefixedStringItor(const uchar * p_, size_t left_) : p(p_), left(left_) 
	{
	}
public:
	explicit ByteLengthPrefixedStringItor(const std::string & s) : p(reinterpret_cast<const uchar *>(s.data())), left(s.size()) 
	{
	}
	std::string operator*() const 
	{
		size_t len = *p ^ MAGIC_XOR_VALUE;
		return std::string(reinterpret_cast<const char *>(p + 1), len);
	}
	ByteLengthPrefixedStringItor operator++(int) 
	{
		const uchar * old_p = p;
		size_t old_left = left;
		operator++();
		return ByteLengthPrefixedStringItor(old_p, old_left);
	}
	ByteLengthPrefixedStringItor & operator++() 
	{
		if(!left) {
			throw Xapian::DatabaseCorruptError("Bad synonym data (none left)");
		}
		size_t add = (*p ^ MAGIC_XOR_VALUE) + 1;
		if(left < add) {
			throw Xapian::DatabaseCorruptError("Bad synonym data (too little left)");
		}
		p += add;
		left -= add;
		return *this;
	}
	bool at_end() const { return left == 0; }
};

struct ByteLengthPrefixedStringItorGt {
	/// Return true if and only if a's string is strictly greater than b's.
	bool operator()(const ByteLengthPrefixedStringItor * a, const ByteLengthPrefixedStringItor * b) const { return (**a > **b); }
};

#endif // XAPIAN_INCLUDED_BYTE_LENGTH_STRINGS_H
