// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

XLCellRange::XLCellRange() : m_dataNode(std::make_unique<XMLNode>(XMLNode{})), m_topLeft(XLCellReference("A1")),
	m_bottomRight(XLCellReference("A1")), m_sharedStrings(XLSharedStringsDefaulted), m_columnStyles{}
{
}
/**
 * @details From the two XLCellReference objects, the constructor calculates the dimensions of the range.
 * If the range exceeds the current bounds of the spreadsheet, the spreadsheet is resized to fit.
 * @pre
 * @post
 */
XLCellRange::XLCellRange(const XMLNode&         dataNode,
    const XLCellReference& topLeft,
    const XLCellReference& bottomRight,
    const XLSharedStrings& sharedStrings)
	: m_dataNode(std::make_unique<XMLNode>(dataNode)),
	m_topLeft(topLeft),
	m_bottomRight(bottomRight),
	m_sharedStrings(sharedStrings),
	m_columnStyles{}
{
	if(m_topLeft.row() > m_bottomRight.row() || m_topLeft.column() > m_bottomRight.column()) {
		using namespace std::literals::string_literals;
		throw XLInputError("XLCellRange constructor: topLeft ("s + topLeft.address() + ")"s
		          /**/             + " does not point to a lower or equal row and column than bottomRight ("s + bottomRight.address() + ")"s);
	}
}

XLCellRange::XLCellRange(const XLCellRange& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)), m_topLeft(other.m_topLeft),
	m_bottomRight(other.m_bottomRight), m_sharedStrings(other.m_sharedStrings), m_columnStyles(other.m_columnStyles)
{
}

XLCellRange::XLCellRange(XLCellRange&& other) noexcept = default;
XLCellRange::~XLCellRange() = default;

XLCellRange& XLCellRange::operator=(const XLCellRange& other)
{
	if(&other != this) {
		*m_dataNode     = *other.m_dataNode;
		m_topLeft       = other.m_topLeft;
		m_bottomRight   = other.m_bottomRight;
		m_sharedStrings = other.m_sharedStrings;
		m_columnStyles  = other.m_columnStyles;
	}
	return *this;
}

XLCellRange& XLCellRange::operator=(XLCellRange&& other) noexcept
{
	if(&other != this) {
		*m_dataNode     = *other.m_dataNode;
		m_topLeft       = other.m_topLeft;
		m_bottomRight   = other.m_bottomRight;
		m_sharedStrings = std::move(other.m_sharedStrings);
		m_columnStyles  = other.m_columnStyles;
	}
	return *this;
}

/**
 * @details Predetermine all defined column styles & gather them in a vector for performant access when XLCellIterator creates new cells
 */
void XLCellRange::fetchColumnStyles()
{
	XMLNode cols = m_dataNode->parent().child("cols");
	uint16_t vecPos = 0;
	XMLNode col = cols.first_child_of_type(pugi::node_element);
	while(not col.empty()) {
		uint16_t minCol = static_cast<uint16_t>(col.attribute("min").as_int(0));
		uint16_t maxCol = static_cast<uint16_t>(col.attribute("max").as_int(0));
		if(minCol > maxCol || !minCol || !maxCol) {
			using namespace std::literals::string_literals;
			throw XLInputError(
				      "column attributes min (\""s + col.attribute("min").value() + "\") and max (\""s + col.attribute("min").value() + "\")"s
				      " must be set and min must not be larger than max"s
				      );
		}
		if(maxCol > m_columnStyles.size())  m_columnStyles.resize(maxCol);                // resize m_columnStyles if necessary
		for( ; vecPos + 1 < minCol; ++vecPos ) m_columnStyles[ vecPos ] = XLDefaultCellFormat; // set all non-defined columns to default
		XLStyleIndex colStyle = col.attribute("style").as_uint(XLDefaultCellFormat);      // acquire column style attribute
		for( ; vecPos < maxCol; ++vecPos ) m_columnStyles[ vecPos ] = colStyle;       // set all covered columns to defined style
		col = col.next_sibling_of_type(pugi::node_element); // advance to next <col> entry, if any
	}
}

const XLCellReference XLCellRange::topLeft() const { return m_topLeft; }
const XLCellReference XLCellRange::bottomRight() const { return m_bottomRight; }
/**
 * @details Evaluate the top left and bottom right cells as string references and
 *          concatenate them with a colon ':'
 */
std::string XLCellRange::address() const { return m_topLeft.address() + ":" + m_bottomRight.address(); }
uint32_t XLCellRange::numRows() const { return m_bottomRight.row() + 1 - m_topLeft.row(); }
uint16_t XLCellRange::numColumns() const { return m_bottomRight.column() + 1 - m_topLeft.column(); }
XLCellIterator XLCellRange::begin() const { return XLCellIterator(*this, XLIteratorLocation::Begin, &m_columnStyles); }
XLCellIterator XLCellRange::end() const { return XLCellIterator(*this, XLIteratorLocation::End, &m_columnStyles); }

void XLCellRange::clear()
{
	for(auto& cell : *this)  cell.value().clear();
}
/**
 * @details Iterate over the full range and set format for each existing cell
 */
bool XLCellRange::setFormat(size_t cellFormatIndex)
{
	// ===== Iterate over all cells in the range
	for(auto it = begin(); it != end(); ++it)
		if(!it->setCellFormat(cellFormatIndex)) // attempt to set cell format
			return false;                   // fail if any setCellFormat failed
	return true; // success if loop finished nominally
}
