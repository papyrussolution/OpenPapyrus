// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
// Created by Kenneth Balslev on 19/08/2020.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

XLCellValue::XLCellValue() = default;
XLCellValue::XLCellValue(const OpenXLSX::XLCellValue& other) = default;
XLCellValue::XLCellValue(OpenXLSX::XLCellValue&& other) noexcept = default;
XLCellValue::~XLCellValue() = default;
XLCellValue& OpenXLSX::XLCellValue::operator=(const OpenXLSX::XLCellValue& other) = default;
XLCellValue& OpenXLSX::XLCellValue::operator=(OpenXLSX::XLCellValue&& other) noexcept = default;
/**
 * @details Clears the contents of the XLCellValue object. Setting the value to an empty string is not sufficient
 * (as an empty string is still a valid string). The m_type variable must also be set to XLValueType::Empty.
 */
XLCellValue& XLCellValue::clear()
{
	m_type  = XLValueType::Empty;
	m_value = std::string("");
	return *this;
}
/**
 * @details Sets the value type to XLValueType::Error. The value will be set to an empty string.
 */
XLCellValue& XLCellValue::setError(const std::string &error)
{
	m_type  = XLValueType::Error;
	m_value = error;
	return *this;
}
/**
 * @details Get the value type of the current object.
 */
XLValueType XLCellValue::type() const { return m_type; }
/**
 * @details Get the value type of the current object, as a string representation
 */
std::string XLCellValue::typeAsString() const
{
	switch(type()) {
		case XLValueType::Empty: return "empty";
		case XLValueType::Boolean: return "boolean";
		case XLValueType::Integer: return "integer";
		case XLValueType::Float: return "float";
		case XLValueType::String: return "string";
		default: return "error";
	}
}
/**
 * @details Constructor
 * @pre The cell and cellNode pointers must not be nullptr and must point to valid objects.
 * @post A valid XLCellValueProxy has been created.
 */
XLCellValueProxy::XLCellValueProxy(XLCell* cell, XMLNode* cellNode) : m_cell(cell), m_cellNode(cellNode)
{
	assert(cell != nullptr);
	//assert(cellNode);
	//assert(not cellNode->empty());
}
XLCellValueProxy::~XLCellValueProxy() = default;
XLCellValueProxy::XLCellValueProxy(const XLCellValueProxy& other) = default;
XLCellValueProxy::XLCellValueProxy(XLCellValueProxy&& other) noexcept = default;
/**
 * @details Copy assignment operator. The function is implemented in terms of the templated
 * value assignment operators, i.e. it is the XLCellValue that is that is copied,
 * not the object itself.
 */
XLCellValueProxy& XLCellValueProxy::operator=(const XLCellValueProxy& other)
{
	if(&other != this) {
		*this = other.getValue();
	}
	return *this;
}
/**
 * @details Move assignment operator. Default implementation has been used.
 */
XLCellValueProxy& XLCellValueProxy::operator=(XLCellValueProxy&& other) noexcept = default;
/**
 * @details Implicitly convert the XLCellValueProxy object to a XLCellValue object.
 */
XLCellValueProxy::operator XLCellValue() const { return getValue(); }

/**
 * @details Clear the contents of the cell. This removes all children of the cell node.
 * @pre The m_cellNode must not be null, and must point to a valid XML cell node object.
 * @post The cell node must be valid, but empty.
 */
XLCellValueProxy& XLCellValueProxy::clear()
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== Remove the type attribute
	m_cellNode->remove_attribute("t");
	// ===== Disable space preservation (only relevant for strings).
	m_cellNode->remove_attribute(" xml:space");
	// ===== Remove the value node.
	m_cellNode->remove_child("v");
	// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
	m_cellNode->remove_child("is");
	return *this;
}
/**
 * @details Set the cell value to a error state. This will remove all children and attributes, except
 * the type attribute, which is set to "e"
 * @pre The m_cellNode must not be null, and must point to a valid XML cell node object.
 * @post The cell node must be valid.
 */
XLCellValueProxy& XLCellValueProxy::setError(const std::string &error)
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== If the cell node doesn't have a type attribute, create it.
	if(!m_cellNode->attribute("t"))
		m_cellNode->append_attribute("t");
	// ===== Set the type to "e", i.e. error
	m_cellNode->attribute("t").set_value("e");
	// ===== If the cell node doesn't have a value child node, create it.
	if(!m_cellNode->child("v"))  m_cellNode->append_child("v");
	// ===== Set the child value to the error
	m_cellNode->child("v").text().set(error.c_str());
	// ===== Disable space preservation (only relevant for strings).
	m_cellNode->remove_attribute(" xml:space");
	// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
	m_cellNode->remove_child("is");
	return *this;
}
/**
 * @details Get the value type for the cell.
 * @pre The m_cellNode must not be null, and must point to a valid XML cell node object.
 * @post No change should be made.
 */
XLValueType XLCellValueProxy::type() const
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== If neither a Type attribute or a getValue node is present, the cell is empty.
	if(!m_cellNode->attribute("t") && !m_cellNode->child("v"))  
		return XLValueType::Empty;
	// ===== If a Type attribute is not present, but a value node is, the cell contains a number.
	if(m_cellNode->attribute("t").empty() || (sstreq(m_cellNode->attribute("t").value(), "n") && not m_cellNode->child("v").empty())) {
		if(const std::string numberString = m_cellNode->child("v").text().get();
		    numberString.find('.') != std::string::npos || numberString.find("E-") != std::string::npos || numberString.find("e-") != std::string::npos)
			return XLValueType::Float;
		return XLValueType::Integer;
	}
	// ===== If the cell is of type "s", the cell contains a shared string.
	if(not m_cellNode->attribute("t").empty() && sstreq(m_cellNode->attribute("t").value(), "s"))
		return XLValueType::String;
	// ===== If the cell is of type "inlineStr", the cell contains an inline string.
	if(not m_cellNode->attribute("t").empty() && sstreq(m_cellNode->attribute("t").value(), "inlineStr"))
		return XLValueType::String;
	// ===== If the cell is of type "str", the cell contains an ordinary string.
	if(not m_cellNode->attribute("t").empty() && sstreq(m_cellNode->attribute("t").value(), "str"))
		return XLValueType::String;
	// ===== If the cell is of type "b", the cell contains a boolean.
	if(not m_cellNode->attribute("t").empty() && sstreq(m_cellNode->attribute("t").value(), "b"))
		return XLValueType::Boolean;
	// ===== Otherwise, the cell contains an error.
	return XLValueType::Error; // the m_typeAttribute has the ValueAsString "e"
}
/**
 * @details Get the value type of the current object, as a string representation.
 */
std::string XLCellValueProxy::typeAsString() const
{
	switch(type()) {
		case XLValueType::Empty: return "empty";
		case XLValueType::Boolean: return "boolean";
		case XLValueType::Integer: return "integer";
		case XLValueType::Float: return "float";
		case XLValueType::String: return "string";
		default: return "error";
	}
}
/**
 * @details Set cell to an integer value. This is private helper function for setting the cell value
 * directly in the underlying XML file.
 * @pre The m_cellNode must not be null, and must point to a valid XMLNode object.
 * @post The underlying XMLNode has been updated correctly, representing an integer value.
 */
void XLCellValueProxy::setInteger(int64_t numberValue)
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== If the cell node doesn't have a value child node, create it.
	if(m_cellNode->child("v").empty())  
		m_cellNode->append_child("v");
	// ===== The type ("t") attribute is not required for number values.
	m_cellNode->remove_attribute("t");
	// ===== Set the text of the value node.
	m_cellNode->child("v").text().set(numberValue);
	// ===== Disable space preservation (only relevant for strings).
	m_cellNode->child("v").remove_attribute(m_cellNode->child("v").attribute("xml:space"));
	// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
	m_cellNode->remove_child("is");
}
/**
 * @details Set the cell to a bool value. This is private helper function for setting the cell value
 * directly in the underlying XML file.
 * @pre The m_cellNode must not be null, and must point to a valid XMLNode object.
 * @post The underlying XMLNode has been updated correctly, representing an bool value.
 */
void XLCellValueProxy::setBoolean(bool numberValue)
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== If the cell node doesn't have a type child node, create it.
	if(m_cellNode->attribute("t").empty())  
		m_cellNode->append_attribute("t");
	// ===== If the cell node doesn't have a value child node, create it.
	if(m_cellNode->child("v").empty())  
		m_cellNode->append_child("v");
	// ===== Set the type attribute.
	m_cellNode->attribute("t").set_value("b");
	// ===== Set the text of the value node.
	m_cellNode->child("v").text().set(numberValue ? 1 : 0);
	// ===== Disable space preservation (only relevant for strings).
	m_cellNode->child("v").remove_attribute(m_cellNode->child("v").attribute("xml:space"));
	// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
	m_cellNode->remove_child("is");
}
/**
 * @details Set the cell to a floating point value. This is private helper function for setting the cell value
 * directly in the underlying XML file.
 * @pre The m_cellNode must not be null, and must point to a valid XMLNode object.
 * @post The underlying XMLNode has been updated correctly, representing a floating point value.
 */
void XLCellValueProxy::setFloat(double numberValue)
{
	// check for nan / inf
	if(std::isfinite(numberValue)) {
		// ===== Check that the m_cellNode is valid.
		assert(m_cellNode != nullptr);
		assert(not m_cellNode->empty());
		// ===== If the cell node doesn't have a value child node, create it.
		if(m_cellNode->child("v").empty())  
			m_cellNode->append_child("v");
		// ===== The type ("t") attribute is not required for number values.
		m_cellNode->remove_attribute("t");
		// ===== Set the text of the value node.
		m_cellNode->child("v").text().set(numberValue);
		// ===== Disable space preservation (only relevant for strings).
		m_cellNode->child("v").remove_attribute(m_cellNode->child("v").attribute("xml:space"));
		// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
		m_cellNode->remove_child("is");
	}
	else {
		setError("#NUM!");
		return;
	}
}
/**
 * @details Set the cell to a string value. This is private helper function for setting the cell value
 * directly in the underlying XML file.
 * @pre The m_cellNode must not be null, and must point to a valid XMLNode object.
 * @post The underlying XMLNode has been updated correctly, representing a string value.
 */
void XLCellValueProxy::setString(const char * stringValue)
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== If the cell node doesn't have a type child node, create it.
	if(m_cellNode->attribute("t").empty())  
		m_cellNode->append_attribute("t");
	// ===== If the cell node doesn't have a value child node, create it.
	if(m_cellNode->child("v").empty())  
		m_cellNode->append_child("v");
	// ===== Set the type attribute.
	m_cellNode->attribute("t").set_value("s");
	// ===== Get or create the index in the XLSharedStrings object.
	const auto index = (m_cell->m_sharedStrings.get().stringExists(stringValue)
	    /**/ ? m_cell->m_sharedStrings.get().getStringIndex(stringValue) /**/ : m_cell->m_sharedStrings.get().appendString(stringValue));
	// ===== Set the text of the value node.
	m_cellNode->child("v").text().set(index);
	// ===== Remove the is node (only relevant in case previous cell type was "inlineStr"). // pull request #188
	m_cellNode->remove_child("is");

	/* 2024-04-23: NOTE "embedded" strings are "inline strings" in XLSX, using a node like so:
	 *     <c r="C1" s="3" t="inlineStr"><is><t>An inline string</t></is></c>
	 *  Those should be not confused with the below "str" type, which is I believe only relevant for the cell display format
	 */
	// IMPLEMENTATION FOR EMBEDDED STRINGS:
	//    m_cellNode->attribute("t").set_value("str");
	//    m_cellNode->child("v").text().set(stringValue);
	//
	//    auto s = std::string_view(stringValue);
	//    if (s.front() == ' ' || s.back() == ' ') {
	//        if (!m_cellNode->attribute("xml:space")) m_cellNode->append_attribute("xml:space");
	//        m_cellNode->attribute("xml:space").set_value("preserve");
	//    }
}

/**
 * @details Get a copy of the XLCellValue object for the cell. This is private helper function for returning an
 * XLCellValue object corresponding to the cell value.
 * @pre The m_cellNode must not be null, and must point to a valid XMLNode object.
 * @post No changes should be made.
 */
XLCellValue XLCellValueProxy::getValue() const
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	switch(type()) {
		case XLValueType::Empty: return XLCellValue().clear();
		case XLValueType::Float: return XLCellValue { m_cellNode->child("v").text().as_double() };
		case XLValueType::Integer: return XLCellValue { m_cellNode->child("v").text().as_llong() };
		case XLValueType::String:
			if(sstreq(m_cellNode->attribute("t").value(), "s"))
				return XLCellValue { m_cell->m_sharedStrings.get().getString(static_cast<uint32>(m_cellNode->child("v").text().as_ullong())) };
			else if(sstreq(m_cellNode->attribute("t").value(), "str"))
				return XLCellValue { m_cellNode->child("v").text().get() };
			else if(sstreq(m_cellNode->attribute("t").value(), "inlineStr"))
				return XLCellValue { m_cellNode->child("is").child("t").text().get() };
			else
				throw XLInternalError("Unknown string type");
		case XLValueType::Boolean: return XLCellValue { m_cellNode->child("v").text().as_bool() };
		case XLValueType::Error: return XLCellValue().setError(m_cellNode->child("v").text().as_string());
		default: return XLCellValue().setError("");
	}
}

int32_t XLCellValueProxy::stringIndex() const
{
	if(strcmp(m_cellNode->attribute("t").value(), "s") != 0)
		return -1; // cell value is not a shared string
	return static_cast<int32_t>(m_cellNode->child("v").text().as_ullong(_FFFF64/*-1*/)); // return the shared string index stored for this cell
	/**/ // if, for whatever reason, the underlying XML has no reference stored, also return -1
}

bool XLCellValueProxy::setStringIndex(int32_t newIndex)
{
	if(newIndex < 0 || strcmp(m_cellNode->attribute("t").value(), "s") != 0)
		return false; // cell value is not a shared string
	else
		return m_cellNode->child("v").text().set(newIndex); // set the shared string index directly
}
