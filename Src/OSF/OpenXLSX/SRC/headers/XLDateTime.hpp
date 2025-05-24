// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLDATETIME_HPP
#define OPENXLSX_XLDATETIME_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <ctime>
#include "XLException.hpp"

// ========== CLASS AND ENUM TYPE DEFINITIONS ========== //
namespace OpenXLSX {
class XLDateTime {
public:
	/**
	 * @brief Constructor.
	 */
	XLDateTime();
	/**
	 * @brief Constructor taking an Excel time point serial number as an argument.
	 * @param serial Excel time point serial number.
	 */
	explicit XLDateTime(double serial);

	/**
	 * @brief Constructor taking a std::tm struct as an argument.
	 * @param timepoint A std::tm struct.
	 */
	explicit XLDateTime(const std::tm& timepoint);

	/**
	 * @brief Constructor taking a unixtime format (seconds since 1/1/1970) as an argument.
	 * @param unixtime A time_t number.
	 */
	explicit XLDateTime(time_t unixtime);

	/**
	 * @brief Copy constructor.
	 * @param other Object to be copied.
	 */
	XLDateTime(const XLDateTime& other);

	/**
	 * @brief Move constructor.
	 * @param other Object to be moved.
	 */
	XLDateTime(XLDateTime&& other) noexcept;

	/**
	 * @brief Destructor
	 */
	~XLDateTime();

	/**
	 * @brief Copy assignment operator.
	 * @param other Object to be copied.
	 * @return Reference to the copied-to object.
	 */
	XLDateTime& operator=(const XLDateTime& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to be moved.
	 * @return Reference to the moved-to object.
	 */
	XLDateTime& operator=(XLDateTime&& other) noexcept;

	/**
	 * @brief Assignment operator taking an Excel date/time serial number as an argument.
	 * @param serial A floating point value with the serial number.
	 * @return Reference to the copied-to object.
	 */
	XLDateTime& operator=(double serial);

	/**
	 * @brief Assignment operator taking a std::tm object as an argument.
	 * @param timepoint std::tm object with the time point
	 * @return Reference to the copied-to object.
	 */
	XLDateTime& operator=(const std::tm& timepoint);

	/**
	 * @brief Implicit conversion to Excel date/time serial number (any floating point type).
	 * @tparam T Type to convert to (any floating point type).
	 * @return Excel date/time serial number.
	 */
	template <typename T,
	    typename = std::enable_if_t<std::is_floating_point_v<T> > >
	operator T() const    // NOLINT
	{
		return serial();
	}

	/**
	 * @brief Implicit conversion to std::tm object.
	 * @return std::tm object.
	 */
	operator std::tm() const;    // NOLINT

	/**
	 * @brief Get the date/time in the form of an Excel date/time serial number.
	 * @return A double with the serial number.
	 */
	double serial() const;

	/**
	 * @brief Get the date/time in the form of a std::tm struct.
	 * @return A std::tm struct with the time point.
	 */
	std::tm tm() const;

private:
	double m_serial { 1.0 }; /**<  */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLDATETIME_HPP
