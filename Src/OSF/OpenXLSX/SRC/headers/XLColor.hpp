// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCOLOR_HPP
#define OPENXLSX_XLCOLOR_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <cstdint>    // Pull request #276
#include <string>

namespace OpenXLSX {
class XLColor {
	//----------------------------------------------------------------------------------------------------------------------
	//           Public Member Functions
	//----------------------------------------------------------------------------------------------------------------------

	friend bool operator==(const XLColor& lhs, const XLColor& rhs);
	friend bool operator!=(const XLColor& lhs, const XLColor& rhs);
public:
	XLColor();
	/**
	 * @brief
	 * @param alpha
	 * @param red
	 * @param green
	 * @param blue
	 */
	XLColor(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);

	/**
	 * @brief
	 * @param red
	 * @param green
	 * @param blue
	 */
	XLColor(uint8_t red, uint8_t green, uint8_t blue);

	/**
	 * @brief
	 * @param hexCode
	 */
	explicit XLColor(const std::string& hexCode);

	/**
	 * @brief
	 * @param other
	 */
	XLColor(const XLColor& other);

	/**
	 * @brief
	 * @param other
	 */
	XLColor(XLColor&& other) noexcept;

	/**
	 * @brief
	 */
	~XLColor();

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLColor& operator=(const XLColor& other);

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLColor& operator=(XLColor&& other) noexcept;

	/**
	 * @brief
	 * @param alpha
	 * @param red
	 * @param green
	 * @param blue
	 */
	void set(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);

	/**
	 * @brief
	 * @param red
	 * @param green
	 * @param blue
	 */
	void set(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0);

	/**
	 * @brief
	 * @param hexCode
	 */
	void set(const std::string& hexCode);

	/**
	 * @brief
	 * @return
	 */
	uint8_t alpha() const;

	/**
	 * @brief
	 * @return
	 */
	uint8_t red() const;

	/**
	 * @brief
	 * @return
	 */
	uint8_t green() const;

	/**
	 * @brief
	 * @return
	 */
	uint8_t blue() const;

	/**
	 * @brief
	 * @return
	 */
	std::string hex() const;

	//----------------------------------------------------------------------------------------------------------------------
	//           Private Member Variables
	//----------------------------------------------------------------------------------------------------------------------

private:
	uint8_t m_alpha { 255 };

	uint8_t m_red { 0 };

	uint8_t m_green { 0 };

	uint8_t m_blue { 0 };
};
}    // namespace OpenXLSX

namespace OpenXLSX
{
/**
 * @brief
 * @param lhs
 * @param rhs
 * @return
 */
inline bool operator==(const XLColor& lhs, const XLColor& rhs)
{
	return lhs.alpha() == rhs.alpha() && lhs.red() == rhs.red() && lhs.green() == rhs.green() && lhs.blue() == rhs.blue();
}

/**
 * @brief
 * @param lhs
 * @param rhs
 * @return
 */
inline bool operator!=(const XLColor& lhs, const XLColor& rhs) { return !(lhs == rhs); }
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCOLOR_HPP
