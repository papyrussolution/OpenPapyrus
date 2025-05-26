// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

/**
 * @details Assumes each node only has data for one column.
 */
XLColumn::XLColumn(const XMLNode& columnNode) : m_columnNode(std::make_unique<XMLNode>(columnNode)) {}
XLColumn::XLColumn(const XLColumn& other) : m_columnNode(std::make_unique<XMLNode>(*other.m_columnNode)) {}
XLColumn::XLColumn(XLColumn&& other) noexcept = default;
XLColumn::~XLColumn() = default;

XLColumn& XLColumn::operator=(const XLColumn& other)
{
	if(&other != this)  *m_columnNode = *other.m_columnNode;
	return *this;
}

float XLColumn::width() const { return columnNode().attribute("width").as_float(); }

void XLColumn::setWidth(float width)
{
	// Set the 'Width' attribute for the Cell. If it does not exist, create it.
	auto widthAtt = columnNode().attribute("width");
	if(widthAtt.empty())  widthAtt = columnNode().append_attribute("width");

	widthAtt.set_value(width);

	// Set the 'customWidth' attribute for the Cell. If it does not exist, create it.
	auto customAtt = columnNode().attribute("customWidth");
	if(customAtt.empty())  customAtt = columnNode().append_attribute("customWidth");

	customAtt.set_value("1");
}

bool XLColumn::isHidden() const { return columnNode().attribute("hidden").as_bool(); }

void XLColumn::setHidden(bool state)
{
	auto hiddenAtt = columnNode().attribute("hidden");
	if(hiddenAtt.empty())  hiddenAtt = columnNode().append_attribute("hidden");
	if(state)
		hiddenAtt.set_value("1");
	else
		hiddenAtt.set_value("0");
}

XMLNode& XLColumn::columnNode() const { return *m_columnNode; }
/**
 * @details Determine the value of the style attribute - if attribute does not exist, return default value
 */
XLStyleIndex XLColumn::format() const { return columnNode().attribute("style").as_uint(XLDefaultCellFormat); }

/**
 * @brief Set the column style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
 *        If the style attribute does not exist, create it
 */
bool XLColumn::setFormat(XLStyleIndex cellFormatIndex)
{
	XMLAttribute styleAtt = columnNode().attribute("style");
	if(styleAtt.empty())  styleAtt = columnNode().append_attribute("style");
	if(styleAtt.empty())  return false;
	styleAtt.set_value(cellFormatIndex);
	return true;
};
