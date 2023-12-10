/*
 * Copyright 2016 Nu-book Inc.
 * Copyright 2016 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop

namespace ZXing {
void BitArray::bitwiseXOR(const BitArray& other)
{
	if(size() != other.size()) {
		throw std::invalid_argument("BitArray::xor(): Sizes don't match");
	}
	for(size_t i = 0; i < _bits.size(); i++) {
		// The last int could be incomplete (i.e. not have 32 bits in
		// it) but there is no problem since 0 XOR 0 == 0.
		_bits[i] ^= other._bits[i];
	}
}

ByteArray BitArray::toBytes(int bitOffset, int numBytes) const
{
	ByteArray res(numBytes == -1 ? (size() - bitOffset + 7) / 8 : numBytes);
	for(int i = 0; i < Size(res); i++)
		for(int j = 0; j < 8; j++)
			AppendBit(res[i], (numBytes != -1 || bitOffset < size()) ? get(bitOffset++) : 0);
	return res;
}
//
//
//
BitArrayView::BitArrayView(const BitArray & bits) : bits(bits), cur(bits.begin())
{
}

BitArrayView & BitArrayView::skipBits(int n)
{
	if(n > bits.size())
		throw std::out_of_range("BitArrayView::skipBits() out of range.");
	cur += n;
	return *this;
}

int BitArrayView::peakBits(int n) const
{
	assert(n <= 32);
	if(n > bits.size())
		throw std::out_of_range("BitArrayView::peakBits() out of range.");
	int res = 0;
	for(auto i = cur; n > 0; --n, i++)
		AppendBit(res, *i);
	return res;
}

int BitArrayView::readBits(int n)
{
	int res = peakBits(n);
	cur += n;
	return res;
}
} // ZXing
