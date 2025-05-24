// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCOLUMN_HPP
#define OPENXLSX_XLCOLUMN_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <memory>
#include "XLStyles.hpp"          // XLStyleIndex
#include "XLXmlParser.hpp"

namespace OpenXLSX {
/**
 * @brief
 */
class XLColumn {
public:
	/**
	 * @brief Constructor
	 * @param columnNode A pointer to the XMLNode for the column.
	 */
	explicit XLColumn(const XMLNode& columnNode);

	/**
	 * @brief Copy Constructor [deleted]
	 */
	XLColumn(const XLColumn& other);

	/**
	 * @brief Move Constructor
	 * @note The move constructor has been explicitly deleted.
	 */
	XLColumn(XLColumn&& other) noexcept;

	/**
	 * @brief Destructor
	 */
	~XLColumn();

	/**
	 * @brief Copy assignment operator [deleted]
	 */
	XLColumn& operator=(const XLColumn& other);

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLColumn& operator=(XLColumn&& other) noexcept = default;

	/**
	 * @brief Get the width of the column.
	 * @return The width of the column.
	 */
	float width() const;

	/**
	 * @brief Set the width of the column
	 * @param width The width of the column
	 */
	void setWidth(float width);

	/**
	 * @brief Is the column hidden?
	 * @return The state of the column.
	 */
	bool isHidden() const;

	/**
	 * @brief Set the column to be shown or hidden.
	 * @param state The state of the column.
	 */
	void setHidden(bool state);

	/**
	 * @brief Get the XMLNode object for the column.
	 * @return The XMLNode for the column
	 */
	XMLNode& columnNode() const;

	/**
	 * @brief Get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style assigned to the column.
	 *        This value is stored in the col attributes like so: style="2"
	 * @returns The index of the applicable format style
	 */
	XLStyleIndex format() const;

	/**
	 * @brief Set the column style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
	 * @param cellFormatIndex The style to set, corresponding to the index of XLStyles::cellStyles()
	 * @returns true on success, false on failure
	 */
	bool setFormat(XLStyleIndex cellFormatIndex);

private:
	std::unique_ptr<XMLNode> m_columnNode; /**< A pointer to the XMLNode object for the column. */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCOLUMN_HPP
