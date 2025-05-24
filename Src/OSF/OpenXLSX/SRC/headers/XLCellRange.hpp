// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCELLRANGE_HPP
#define OPENXLSX_XLCELLRANGE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <memory>
#include "XLCell.hpp"
#include "XLCellIterator.hpp"
#include "XLCellReference.hpp"
#include "XLXmlParser.hpp"

namespace OpenXLSX {
/**
 * @brief This class encapsulates the concept of a cell range, i.e. a square area
 * (or subset) of cells in a spreadsheet.
 */
class XLCellRange {
	friend class XLCellIterator;
public:
	/**
	 * @brief Default constructor for variable declaration
	 */
	XLCellRange();
	/**
	 * @brief
	 * @param dataNode
	 * @param topLeft
	 * @param bottomRight
	 * @param sharedStrings
	 */
	explicit XLCellRange(const XMLNode&         dataNode,
	    const XLCellReference& topLeft,
	    const XLCellReference& bottomRight,
	    const XLSharedStrings& sharedStrings);

	/**
	 * @brief Copy constructor
	 * @param other The range object to be copied.
	 * @note This implements the default copy constructor, i.e. memberwise copying.
	 */
	XLCellRange(const XLCellRange& other);

	/**
	 * @brief Move constructor
	 * @param other The range object to be moved.
	 * @note This implements the default move constructor, i.e. memberwise move.
	 */
	XLCellRange(XLCellRange&& other) noexcept;

	/**
	 * @brief Destructor [default]
	 * @note This implements the default destructor.
	 */
	~XLCellRange();

	/**
	 * @brief The copy assignment operator [default]
	 * @param other The range object to be copied and assigned.
	 * @return A reference to the new object.
	 * @throws A std::range_error if the source range and destination range are of different size and shape.
	 * @note This implements the default copy assignment operator.
	 */
	XLCellRange& operator=(const XLCellRange& other);

	/**
	 * @brief The move assignment operator [default].
	 * @param other The range object to be moved and assigned.
	 * @return A reference to the new object.
	 * @note This implements the default move assignment operator.
	 */
	XLCellRange& operator=(XLCellRange&& other) noexcept;

	/**
	 * @brief populate the m_columnStyles
	 * @return a const XLCellReference
	 */
	void fetchColumnStyles();

	/**
	 * @brief get the top left cell
	 * @return a const XLCellReference
	 */
	const XLCellReference topLeft() const;

	/**
	 * @brief get the bottom right cell
	 * @return a const XLCellReference
	 */
	const XLCellReference bottomRight() const;

	/**
	 * @brief get the string reference that corresponds to the represented cell range
	 * @return a std::string range reference, e.g. "A2:Z5"
	 */
	std::string address() const;

	/**
	 * @brief Get the number of rows in the range.
	 * @return The number of rows.
	 */
	uint32_t numRows() const;

	/**
	 * @brief Get the number of columns in the range.
	 * @return The number of columns.
	 */
	uint16_t numColumns() const;

	/**
	 * @brief
	 * @return
	 */
	XLCellIterator begin() const;

	/**
	 * @brief
	 * @return
	 */
	XLCellIterator end() const;

	/**
	 * @brief
	 */
	void clear();

	/**
	 * @brief Templated assignment operator - assign value to a range of cells
	 * @tparam T The type of the value argument.
	 * @param value The value.
	 * @return A reference to the assigned-to object.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	XLCellRange& operator=(T value)
	{
		// forward implementation to templated XLCellValue& XLCellValue::operator=(T value)
		for(auto it = begin(); it != end(); ++it)  it->value() = value;
		return *this;
	}

	/**
	 * @brief Set cell format for a range of cells
	 * @param cellFormatIndex The style to set, corresponding to the nidex of XLStyles::cellStyles()
	 * @returns true on success, false on failure
	 */
	bool setFormat(XLStyleIndex cellFormatIndex);

	//----------------------------------------------------------------------------------------------------------------------
	//           Private Member Variables
	//----------------------------------------------------------------------------------------------------------------------

private:
	std::unique_ptr<XMLNode>  m_dataNode;      /**< */
	XLCellReference m_topLeft;                 /**< reference to the first cell in the range */
	XLCellReference m_bottomRight;             /**< reference to the last cell in the range */
	XLSharedStringsRef m_sharedStrings;        /**< reference to the document shared strings table */
	std::vector<XLStyleIndex> m_columnStyles;  /**< quick access to column styles in the range - populated by fetchColumnStyles() */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCELLRANGE_HPP
