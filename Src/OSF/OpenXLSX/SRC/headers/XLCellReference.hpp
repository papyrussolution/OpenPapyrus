// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCELLREFERENCE_HPP
#define OPENXLSX_XLCELLREFERENCE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <cstdint>    // Pull request #276
#include <string>
#include <utility>

namespace OpenXLSX {
using XLCoordinates = std::pair<uint32_t, uint16_t>;

class XLCellReference final {
	friend bool operator==(const XLCellReference& lhs, const XLCellReference& rhs);
	friend bool operator!=(const XLCellReference& lhs, const XLCellReference& rhs);
	friend bool operator<(const XLCellReference& lhs, const XLCellReference& rhs);
	friend bool operator>(const XLCellReference& lhs, const XLCellReference& rhs);
	friend bool operator<=(const XLCellReference& lhs, const XLCellReference& rhs);
	friend bool operator>=(const XLCellReference& lhs, const XLCellReference& rhs);
public:
	/**
	 * @brief Constructor taking a cell address as argument.
	 * @param cellAddress The address of the cell, e.g. 'A1'.
	 * @details The constructor creates a new XLCellReference from a string, e.g. 'A1'. If there's no input,
	 * the default reference will be cell A1.
	 */
	XLCellReference(const std::string& cellAddress = "");    // NOLINT

	/**
	 * @brief Constructor taking the cell coordinates as arguments.
	 * @param row The row number of the cell.
	 * @param column The column number of the cell.
	 */
	XLCellReference(uint32_t row, uint16_t column);

	/**
	 * @brief Constructor taking the row number and the column letter as arguments.
	 * @param row The row number of the cell.
	 * @param column The column letter of the cell.
	 */
	XLCellReference(uint32_t row, const std::string& column);

	/**
	 * @brief Copy constructor
	 * @param other The object to be copied.
	 */
	XLCellReference(const XLCellReference& other);

	/**
	 * @brief
	 * @param other
	 */
	XLCellReference(XLCellReference&& other) noexcept;

	/**
	 * @brief Destructor. Default implementation used.
	 */
	~XLCellReference();

	/**
	 * @brief Assignment operator.
	 * @param other The object to be copied/assigned.
	 * @return A reference to the new object.
	 */
	XLCellReference& operator=(const XLCellReference& other);

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLCellReference& operator=(XLCellReference&& other) noexcept;

	/**
	 * @brief
	 * @return
	 */
	XLCellReference& operator++();

	/**
	 * @brief
	 * @return
	 */
	XLCellReference operator++(int);    // NOLINT

	/**
	 * @brief
	 * @return
	 */
	XLCellReference& operator--();

	/**
	 * @brief
	 * @return
	 */
	XLCellReference operator--(int);    // NOLINT

	/**
	 * @brief Get the row number of the XLCellReference.
	 * @return The row.
	 */
	uint32_t row() const;

	/**
	 * @brief Set the row number for the XLCellReference.
	 * @param row The row number.
	 */
	void setRow(uint32_t row);

	/**
	 * @brief Get the column number of the XLCellReference.
	 * @return The column number.
	 */
	uint16_t column() const;

	/**
	 * @brief Set the column number of the XLCellReference.
	 * @param column The column number.
	 */
	void setColumn(uint16_t column);

	/**
	 * @brief Set both row and column number of the XLCellReference.
	 * @param row The row number.
	 * @param column The column number.
	 */
	void setRowAndColumn(uint32_t row, uint16_t column);

	/**
	 * @brief Get the address of the XLCellReference
	 * @return The address, e.g. 'A1'
	 */
	std::string address() const;

	/**
	 * @brief Set the address of the XLCellReference
	 * @param address The address, e.g. 'A1'
	 * @pre The address input string must be a valid Excel cell reference. Otherwise the behaviour is undefined.
	 */
	void setAddress(const std::string& address);

	//----------------------------------------------------------------------------------------------------------------------
	//           Private Member Functions
	//----------------------------------------------------------------------------------------------------------------------

	// private:

	/**
	 * @brief
	 * @param row
	 * @return
	 */
	static std::string rowAsString(uint32_t row);

	/**
	 * @brief
	 * @param row
	 * @return
	 */
	static uint32_t rowAsNumber(const std::string& row);

	/**
	 * @brief Static helper function to convert column number to column letter (e.g. column 1 becomes 'A')
	 * @param column The column number.
	 * @return The column letter
	 */
	static std::string columnAsString(uint16_t column);

	/**
	 * @brief Static helper function to convert column letter to column number (e.g. column 'A' becomes 1)
	 * @param column The column letter, e.g. 'A'
	 * @return The column number.
	 */
	static uint16_t columnAsNumber(const std::string& column);

	/**
	 * @brief Static helper function to convert cell address to coordinates.
	 * @param address The address to be converted, e.g. 'A1'
	 * @return A std::pair<row, column>
	 */
	static XLCoordinates coordinatesFromAddress(const std::string& address);

	//----------------------------------------------------------------------------------------------------------------------
	//           Private Member Variables
	//----------------------------------------------------------------------------------------------------------------------
private:
	uint32_t m_row { 1 };               /**< The row */
	uint16_t m_column { 1 };            /**< The column */
	std::string m_cellAddress { "A1" }; /**< The address, e.g. 'A1' */
};

/**
 * @brief Helper function to check equality between two XLCellReferences.
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return true if equal; otherwise false.
 */
inline bool operator==(const XLCellReference& lhs, const XLCellReference& rhs)
{
	return lhs.row() == rhs.row() && lhs.column() == rhs.column();
}

/**
 * @brief Helper function to check for in-equality between two XLCellReferences
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return false if equal; otherwise true.
 */
inline bool operator!=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs == rhs); }

/**
 * @brief Helper function to check if one XLCellReference is smaller than another.
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return true if lhs < rhs; otherwise false.
 */
inline bool operator<(const XLCellReference& lhs, const XLCellReference& rhs)
{
	return lhs.row() < rhs.row() || (lhs.row() <= rhs.row() && lhs.column() < rhs.column());
}

/**
 * @brief Helper function to check if one XLCellReference is larger than another.
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return true if lhs > rhs; otherwise false.
 */
inline bool operator>(const XLCellReference& lhs, const XLCellReference& rhs) { return (rhs < lhs); }

/**
 * @brief Helper function to check if one XLCellReference is smaller than or equal to another.
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return true if lhs <= rhs; otherwise false
 */
inline bool operator<=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs > rhs); }

/**
 * @brief Helper function to check if one XLCellReference is larger than or equal to another.
 * @param lhs The first XLCellReference
 * @param rhs The second XLCellReference
 * @return true if lhs >= rhs; otherwise false.
 */
inline bool operator>=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs < rhs); }
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCELLREFERENCE_HPP
