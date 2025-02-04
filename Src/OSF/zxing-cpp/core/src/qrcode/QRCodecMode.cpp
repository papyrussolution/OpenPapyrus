/*
 * Copyright 2016 Nu-book Inc.
 * Copyright 2016 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop

namespace ZXing::QRCode {

CodecMode CodecModeForBits(int bits, Type type)
{
	if(type == Type::Micro) {
		constexpr CodecMode Bits2Mode[4] = {CodecMode::NUMERIC, CodecMode::ALPHANUMERIC, CodecMode::BYTE, CodecMode::KANJI};
		if(bits < Size(Bits2Mode))
			return Bits2Mode[bits];
	} 
	else if(type == Type::rMQR) {
		constexpr CodecMode Bits2Mode[8] = {
			CodecMode::TERMINATOR, CodecMode::NUMERIC, CodecMode::ALPHANUMERIC, CodecMode::BYTE,
			CodecMode::KANJI, CodecMode::FNC1_FIRST_POSITION, CodecMode::FNC1_SECOND_POSITION, CodecMode::ECI
		};
		if(bits < Size(Bits2Mode))
			return Bits2Mode[bits];
	} 
	else {
		if((bits >= 0x00 && bits <= 0x05) || (bits >= 0x07 && bits <= 0x09) || bits == 0x0d)
			return static_cast<CodecMode>(bits);
	}
	throw FormatError("Invalid codec mode");
}


int CharacterCountBits(CodecMode mode, const Version& version)
{
	int number = version.versionNumber();
	if(version.isMicro()) {
		switch(mode) {
			case CodecMode::NUMERIC:      return std::array{3, 4, 5, 6}[number - 1];
			case CodecMode::ALPHANUMERIC: return std::array{3, 4, 5}[number - 2];
			case CodecMode::BYTE:         return std::array{4, 5}[number - 3];
			case CodecMode::KANJI:        [[fallthrough]];
			case CodecMode::HANZI:        return std::array{3, 4}[number - 3];
			default: return 0;
		}
	}

	int i;
	if(number <= 9)
		i = 0;
	else if(number <= 26)
		i = 1;
	else
		i = 2;

	switch(mode) {
		case CodecMode::NUMERIC:      return std::array{10, 12, 14}[i];
		case CodecMode::ALPHANUMERIC: return std::array{9, 11, 13}[i];
		case CodecMode::BYTE:         return std::array{8, 16, 16}[i];
		case CodecMode::KANJI:        [[fallthrough]];
		case CodecMode::HANZI:        return std::array{8, 10, 12}[i];
		default:                      return 0;
	}
}

int CodecModeBitsLength(const Version& version)
{
	return version.isMicro() ? version.versionNumber() - 1 : 4;
}

int TerminatorBitsLength(const Version& version)
{
	return version.isMicro() ? version.versionNumber() * 2 + 1 : 4;
}
} // namespace ZXing::QRCode
