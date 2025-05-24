// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

XLColor::XLColor() = default;
XLColor::XLColor(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) : m_alpha(alpha), m_red(red), m_green(green), m_blue(blue) {}
XLColor::XLColor(uint8_t red, uint8_t green, uint8_t blue) : m_red(red), m_green(green), m_blue(blue) {}
XLColor::XLColor(const std::string& hexCode) { set(hexCode); }
XLColor::XLColor(const XLColor& other) = default;
XLColor::XLColor(XLColor&& other) noexcept = default;
XLColor::~XLColor() = default;
XLColor& XLColor::operator=(const XLColor& other) = default;
XLColor& XLColor::operator=(XLColor&& other) noexcept = default;

void XLColor::set(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue)
{
	m_alpha = alpha;
	m_red   = red;
	m_green = green;
	m_blue  = blue;
}

void XLColor::set(uint8_t red, uint8_t green, uint8_t blue)
{
	m_red   = red;
	m_green = green;
	m_blue  = blue;
}

void XLColor::set(const std::string& hexCode)
{
	std::string alpha;
	std::string red;
	std::string green;
	std::string blue;

	auto temp = hex();

	constexpr int hexCodeSizeWithoutAlpha = 6;
	constexpr int hexCodeSizeWithAlpha    = 8;

	if(hexCode.size() == hexCodeSizeWithoutAlpha) {
		alpha = hex().substr(0, 2);
		red   = hexCode.substr(0, 2);
		green = hexCode.substr(2, 2);
		blue  = hexCode.substr(4, 2);
	}

	else if(hexCode.size() == hexCodeSizeWithAlpha) {
		alpha = hexCode.substr(0, 2);
		red   = hexCode.substr(2, 2);
		green = hexCode.substr(4, 2);
		blue  = hexCode.substr(6, 2);// NOLINT
	}
	else
		throw XLInputError("Invalid color code");

	constexpr int hexBase = 16;
	m_alpha               = static_cast<uint8_t>(stoul(alpha, nullptr, hexBase));
	m_red                 = static_cast<uint8_t>(stoul(red, nullptr, hexBase));
	m_green               = static_cast<uint8_t>(stoul(green, nullptr, hexBase));
	m_blue                = static_cast<uint8_t>(stoul(blue, nullptr, hexBase));
}

uint8_t XLColor::alpha() const { return m_alpha; }
uint8_t XLColor::red() const { return m_red; }
uint8_t XLColor::green() const { return m_green; }
uint8_t XLColor::blue() const { return m_blue; }

std::string XLColor::hex() const
{
	std::stringstream str;
	constexpr int hexBase = 16;

	if(m_alpha < hexBase)  str << "0";
	str << std::hex << static_cast<int>(m_alpha);

	if(m_red < hexBase)  str << "0";
	str << std::hex << static_cast<int>(m_red);

	if(m_green < hexBase)  str << "0";
	str << std::hex << static_cast<int>(m_green);

	if(m_blue < hexBase)  str << "0";
	str << std::hex << static_cast<int>(m_blue);

	return (str.str());
}
