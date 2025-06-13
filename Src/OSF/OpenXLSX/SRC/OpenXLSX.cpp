// OpenXLSX.cpp
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
//#include <OpenXLSX-internal.hpp>
#define SLIB_INCLUDE_CPPSTDLIBS
#include <slib.h>
#ifdef CHARCONV_ENABLED
	#include <charconv>
#endif
#include <OpenXLSX.hpp>
#include <zippy.hpp>
#ifdef ENABLE_NOWIDE
	#include <nowide/fstream.hpp>
#endif
#if defined(_WIN32)
	#include <random>
#endif

// don't use "stat" directly because windows has compatibility-breaking defines
#if defined(_WIN32)    // moved below includes to make it absolutely clear that this is module-local
	#define STAT _stat                 // _stat should be available in standard environment on Windows
	#define STATSTRUCT struct _stat    // struct _stat also exists - split the two names in case the struct _stat must not be used on windows
#else
	#define STAT stat
	#define STATSTRUCT struct stat
#endif

using namespace OpenXLSX;

namespace OpenXLSX {
	constexpr const bool XLRemoveAttributes = true;      // helper variables for appendAndSetNodeAttribute, parameter removeAttributes
	constexpr const bool XLKeepAttributes   = false;      //

	/**
	 * @brief Get rid of compiler warnings about unused variables (-Wunused-variable) or unused parameters (-Wunusued-parameter)
	 * @param (unnamed) the variable / parameter which is intentionally unused (e.g. for function stubs)
	 */
	template <class T> void ignore(const T&) 
	{
	}
	/**
	 * @brief find the index of nodeName in nodeOrder
	 * @param nodeName search this
	 * @param nodeOrder in this
	 * @return index of nodeName in nodeOrder
	 * @return -1 if nodeName is not an element of nodeOrder
	 * @note this function uses a vector of std::string_view because std::string is not constexpr-capable, and in
	 *        a future c++20 build, a std::span could be used to have the node order in any OpenXLSX class be a constexpr
	 */
	constexpr const int SORT_INDEX_NOT_FOUND = -1;

	int findStringInVector(std::string const & nodeName, std::vector <std::string_view> const & nodeOrder);
	/**
	 * @brief copy all leading pc_data nodes from fromNode to toNode
	 * @param parent parent node that can perform sibling insertions
	 * @param fromNode node whose preceeding whitespaces shall be duplicated
	 * @param toNode node before which the duplicated whitespaces shall be inserted
	 * @return N/A
	 */
	void copyLeadingWhitespaces(XMLNode & parent, XMLNode fromNode, XMLNode toNode);
	/**
	 * @brief ensure that node with nodeName exists in parent and return it
	 * @param parent parent node that can perform sibling insertions
	 * @param nodename name of the node to be (created &) returned
	 * @param nodeOrder optional vector of a predefined element node sequence required by MS Office
	 * @param force_ns optional force nodeName namespace
	 * @return the requested XMLNode or an empty node if the insert operation failed
	 * @note 2024-12-19: appendAndGetNode will attempt to perform an ordered insert per nodeOrder if provided
	 *       Once sufficiently tested, this functionality might be generalized (e.g. in XLXmlParser OpenXLSX_xml_node)
	 */
	XMLNode appendAndGetNode(XMLNode & parent, std::string const & nodeName, std::vector< std::string_view > const & nodeOrder = {}, bool force_ns = false);
	XMLAttribute appendAndGetAttribute(XMLNode & node, std::string const & attrName, std::string const & attrDefaultVal);
	XMLAttribute appendAndSetAttribute(XMLNode & node, std::string const & attrName, std::string const & attrVal);
	/**
	 * @brief ensure that node with nodeName exists in parent, has an attribute with attrName and return that attribute
	 * @param parent parent node that can perform sibling insertions
	 * @param nodename name of the node under which attribute attrName shall exist
	 * @param attrName name of the attribute to get for node nodeName
	 * @param attrDefaultVal value to assign to the attribute if it has to be created
	 * @param nodeOrder optional vector of a predefined element node sequence required by MS Office, passed through to appendAndGetNode
	 * @returns the requested XMLAttribute or an empty node if the operation failed
	 */
	XMLAttribute appendAndGetNodeAttribute(XMLNode & parent, std::string const & nodeName, std::string const & attrName, std::string const & attrDefaultVal,
		std::vector <std::string_view> const & nodeOrder = {});
	/**
	 * @brief ensure that node with nodeName exists in parent, has an attribute with attrName, set attribute value and return that attribute
	 * @param parent parent node that can perform sibling insertions
	 * @param nodename name of the node under which attribute attrName shall exist
	 * @param attrName name of the attribute to set for node nodeName
	 * @param attrVal value to assign to the attribute
	 * @param removeAttributes if true, all other attributes of the node with nodeName will be deleted
	 * @param nodeOrder optional vector of a predefined element node sequence required by MS Office, passed through to appendAndGetNode
	 * @returns the XMLAttribute that was modified or an empty node if the operation failed
	 */
	XMLAttribute appendAndSetNodeAttribute(XMLNode & parent, std::string const & nodeName, std::string const & attrName, std::string const & attrVal,
		bool removeAttributes = XLKeepAttributes, std::vector< std::string_view > const & nodeOrder = {});
	/**
	 * @brief special bool attribute getter function for tags that should have a val="true" or val="false" attribute,
	 *       but when omitted shall default to "true"
	 * @param parent node under which tagName shall be found
	 * @param tagName name of the boolean tag to evaluate
	 * @param attrName (default: "val") name of boolean attribute that shall default to true
	 * @returns true if parent & tagName exist, and attribute with attrName is either omitted or as_bool() returns true. Otherwise return false
	 * @note this will create and explicitly set attrName if omitted
	 */
	bool getBoolAttributeWhenOmittedMeansTrue(XMLNode & parent, std::string const & tagName, std::string const & attrName = "val");
	/**
	 * @brief Get a string representation of pugi::xml_node_type
	 * @param t the pugi::xml_node_type of a node
	 * @return a std::string containing the descriptive name of the node type
	 */
	std::string XLValueTypeString(XLValueType t)
	{
		using namespace std::literals::string_literals;
		switch(t) {
			case XLValueType::Empty: return "Empty"s;
			case XLValueType::Boolean: return "Boolean"s;
			case XLValueType::Integer: return "Integer"s;
			case XLValueType::Float: return "Float"s;
			case XLValueType::Error: return "Error"s;
			case XLValueType::String: return "String"s;
		}
		throw XLInternalError(__func__ + "Invalid XLValueType."s);
	}	
	/**
	 * @brief Get a string representation of pugi::xml_node_type
	 * @param t the pugi::xml_node_type of a node
	 * @return a std::string containing the descriptive name of the node type
	 */
	std::string xml_node_type_string(pugi::xml_node_type t)
	{
		using namespace std::literals::string_literals;
		switch(t) {
			case pugi::node_null: return "node_null"s;
			case pugi::node_document: return "node_document"s;
			case pugi::node_element: return "node_element"s;
			case pugi::node_pcdata: return "node_pcdata"s;
			case pugi::node_cdata: return "node_cdata"s;
			case pugi::node_comment: return "node_comment"s;
			case pugi::node_pi: return "node_pi"s;
			case pugi::node_declaration: return "node_declaration"s;
			case pugi::node_doctype: return "node_doctype"s;
		}
		throw XLInternalError("Invalid XML node type.");
	}
	/**
	 * @brief Get a string representation of OpenXLSX::XLContentType
	 * @param t an OpenXLSX::XLContentType value
	 * @return a std::string containing the descriptive name of the content type
	 */
	std::string XLContentTypeString(OpenXLSX::XLContentType const & t)
	{
		switch(t) {
			case XLContentType::Workbook: return "Workbook";
			case XLContentType::Relationships: return "Relationships";
			case XLContentType::WorkbookMacroEnabled: return "WorkbookMacroEnabled";
			case XLContentType::Worksheet: return "Worksheet";
			case XLContentType::Chartsheet: return "Chartsheet";
			case XLContentType::ExternalLink: return "ExternalLink";
			case XLContentType::Theme: return "Theme";
			case XLContentType::Styles: return "Styles";
			case XLContentType::SharedStrings: return "SharedStrings";
			case XLContentType::Drawing: return "Drawing";
			case XLContentType::Chart: return "Chart";
			case XLContentType::ChartStyle: return "ChartStyle";
			case XLContentType::ChartColorStyle: return "ChartColorStyle";
			case XLContentType::ControlProperties: return "ControlProperties";
			case XLContentType::CalculationChain: return "CalculationChain";
			case XLContentType::VBAProject: return "VBAProject";
			case XLContentType::CoreProperties: return "CoreProperties";
			case XLContentType::ExtendedProperties: return "ExtendedProperties";
			case XLContentType::CustomProperties: return "CustomProperties";
			case XLContentType::VMLDrawing: return "VMLDrawing";
			case XLContentType::Comments: return "Comments";
			case XLContentType::Table: return "Table";
			case XLContentType::Unknown: return "Unknown";
		}
		return "invalid";
	}
	/**
	 * @brief Get a string representation of OpenXLSX::XLRelationshipType
	 * @param t an OpenXLSX::XLRelationshipType value
	 * @return std::string containing the descriptive name of the relationship type
	 */
	std::string XLRelationshipTypeString(OpenXLSX::XLRelationshipType const & t)
	{
		using namespace OpenXLSX;
		switch(t) {
			case XLRelationshipType::CoreProperties: return "CoreProperties";
			case XLRelationshipType::ExtendedProperties: return "ExtendedProperties";
			case XLRelationshipType::CustomProperties: return "CustomProperties";
			case XLRelationshipType::Workbook: return "Workbook";
			case XLRelationshipType::Worksheet: return "Worksheet";
			case XLRelationshipType::Chartsheet: return "Chartsheet";
			case XLRelationshipType::Dialogsheet: return "Dialogsheet";
			case XLRelationshipType::Macrosheet: return "Macrosheet";
			case XLRelationshipType::CalculationChain: return "CalculationChain";
			case XLRelationshipType::ExternalLink: return "ExternalLink";
			case XLRelationshipType::ExternalLinkPath: return "ExternalLinkPath";
			case XLRelationshipType::Theme: return "Theme";
			case XLRelationshipType::Styles: return "Styles";
			case XLRelationshipType::Chart: return "Chart";
			case XLRelationshipType::ChartStyle: return "ChartStyle";
			case XLRelationshipType::ChartColorStyle: return "ChartColorStyle";
			case XLRelationshipType::Image: return "Image";
			case XLRelationshipType::Drawing: return "Drawing";
			case XLRelationshipType::VMLDrawing: return "VMLDrawing";
			case XLRelationshipType::SharedStrings: return "SharedStrings";
			case XLRelationshipType::PrinterSettings: return "PrinterSettings";
			case XLRelationshipType::VBAProject: return "VBAProject";
			case XLRelationshipType::ControlProperties: return "ControlProperties";
			case XLRelationshipType::Comments: return "Comments";
			case XLRelationshipType::Table: return "Table";
			case XLRelationshipType::Unknown: return "Unknown";
		}
		return "invalid";
	}

	XMLNode getRowNode(XMLNode sheetDataNode, uint32 rowNumber)
	{
		if(rowNumber < 1 || rowNumber > OpenXLSX::MAX_ROWS) {     // 2024-05-28: added range check
			using namespace std::literals::string_literals;
			throw XLCellAddressError("rowNumber "s + std::to_string(rowNumber) + " is outside valid range [1;"s + std::to_string(OpenXLSX::MAX_ROWS) + "]"s);
		}
		// ===== Get the last child of sheetDataNode that is of type node_element.
		XMLNode result = sheetDataNode.last_child_of_type(pugi::node_element);
		// ===== If there are now rows in the worksheet, or the requested row is beyond the current max row, append a new row to the end.
		if(result.empty() || (rowNumber > result.attribute("r").as_ullong())) {
			result = sheetDataNode.append_child("row");
			result.append_attribute("r") = rowNumber;
			// result.append_attribute("x14ac:dyDescent") = "0.2";
			// result.append_attribute("spans")           = "1:1";
		}
		// ===== If the requested node is closest to the end, start from the end and search backwards.
		else if(result.attribute("r").as_ullong() - rowNumber < rowNumber) {
			while(!result.empty() && (result.attribute("r").as_ullong() > rowNumber))  
				result = result.previous_sibling_of_type(pugi::node_element);
			// ===== If the backwards search failed to locate the requested row
			if(result.empty() || (result.attribute("r").as_ullong() != rowNumber)) {
				if(result.empty())
					result = sheetDataNode.prepend_child("row"); // insert a new row node at datasheet begin. When saving, this will keep whitespace formatting towards next row node
				else
					result = sheetDataNode.insert_child_after("row", result);
				result.append_attribute("r") = rowNumber;
				// result.append_attribute("x14ac:dyDescent") = "0.2";
				// result.append_attribute("spans")           = "1:1";
			}
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			result = sheetDataNode.first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested rowNumber is <= the row number of the last node_element, therefore this loop will halt.
			while(result.attribute("r").as_ullong() < rowNumber)  
				result = result.next_sibling_of_type(pugi::node_element);
			// ===== If the forwards search failed to locate the requested row
			if(result.attribute("r").as_ullong() > rowNumber) {
				result = sheetDataNode.insert_child_before("row", result);
				result.append_attribute("r") = rowNumber;
				// result.append_attribute("x14ac:dyDescent") = "0.2";
				// result.append_attribute("spans")           = "1:1";
			}
		}
		return result;
	}
	/**
	 * @brief get the style attribute s for the indicated column, if any is set
	 * @param rowNode the row node from which to obtain the parent that should hold the <cols> node
	 * @param colNo the column for which to obtain the style
	 * @return the XLStyleIndex stored for the column, or XLDefaultCellFormat if none
	 */
	XLStyleIndex getColumnStyle(XMLNode rowNode, uint16 colNo)
	{
		XMLNode cols = rowNode.parent().parent().child("cols");
		if(not cols.empty()) {
			XMLNode col = cols.first_child_of_type(pugi::node_element);
			while(not col.empty()) {
				if(col.attribute("min").as_int(MAX_COLS+1) <= colNo && col.attribute("max").as_int(0) >= colNo) // found
					return col.attribute("style").as_uint(XLDefaultCellFormat);
				col = col.next_sibling_of_type(pugi::node_element);
			}
		}
		return XLDefaultCellFormat; // if no col style was found
	}
	/**
	 * @brief set the cell reference, and a default cell style attribute if and only if row or column style is != XLDefaultCellFormat
	 * @note row style takes precedence over column style
	 * @param cellNode the cell XML node
	 * @param cellRef the cell reference (attribute r) to set
	 * @param rowNode the row node for the cell
	 * @param colNo the column number for this cell (to try and fetch a column style)
	 * @param colStyles an optional std::vector<XLStyleIndex> that contains all pre-evaluated column styles,
	 *         and can be used to avoid performance impact from lookup
	 */
	void setDefaultCellAttributes(XMLNode cellNode, const std::string & cellRef, XMLNode rowNode, uint16 colNo, std::vector<XLStyleIndex> const & colStyles = {})
	{
		cellNode.append_attribute("r").set_value(cellRef.c_str());
		XMLAttribute rowStyle = rowNode.attribute("s");
		XLStyleIndex cellStyle;    // scope declaration
		if(rowStyle.empty()) {     // if no explicit row style is set
			if(colStyles.size() > 0) // if colStyles were provided
				cellStyle = (colNo > colStyles.size() ? XLDefaultCellFormat : colStyles[ colNo - 1 ]); // determine cellStyle from colStyles
			else // else: no colStyles provided
				cellStyle = getColumnStyle(rowNode, colNo); // so perform a lookup
		}
		else // else: an explicit row style is set
			cellStyle = rowStyle.as_uint(XLDefaultCellFormat); // use the row style
		if(cellStyle != XLDefaultCellFormat)  // if cellStyle was determined as not the default style (no point in setting that)
			cellNode.append_attribute("s").set_value(cellStyle);
	}
	/**
	 * @brief Retrieve the xml node representing the cell at the given row and column. If the node doesn't
	 * exist, it will be created.
	 * @param rowNode The row node under which to find the cell.
	 * @param columnNumber The column at which to find the cell.
	 * @param rowNumber (optional) row number of the row node, if already known, defaults to 0
	 * @param colStyles an optional std::vector<XLStyleIndex> that contains all pre-evaluated column styles,
	 *         and can be used to avoid performance impact from lookup
	 * @return The xml node representing the requested cell.
	 */
	XMLNode getCellNode(XMLNode rowNode, uint16 columnNumber, uint32 rowNumber = 0, std::vector<XLStyleIndex> const & colStyles = {})
	{
		if(columnNumber < 1 || columnNumber > OpenXLSX::MAX_COLS) {     // 2024-08-05: added range check
			using namespace std::literals::string_literals;
			throw XLException("XLWorksheet::column: columnNumber "s + std::to_string(columnNumber) + " is outside allowed range [1;"s + std::to_string(MAX_COLS) + "]"s);
		}
		if(rowNode.empty())  
			return XMLNode{};    // 2024-05-28: return an empty node in case of empty rowNode
		XMLNode cellNode = rowNode.last_child_of_type(pugi::node_element);
		if(!rowNumber)  
			rowNumber = rowNode.attribute("r").as_uint(); // if not provided, determine from rowNode
		auto cellRef  = XLCellReference(rowNumber, columnNumber);
		// ===== If there are no cells in the current row, or the requested cell is beyond the last cell in the row...
		if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)) {
			// ===== append a new node to the end.
			cellNode = rowNode.append_child("c");
			setDefaultCellAttributes(cellNode, cellRef.address(), rowNode, columnNumber, colStyles);
		}
		// ===== If the requested node is closest to the end, start from the end and search backwards...
		else if(XLCellReference(cellNode.attribute("r").value()).column() - columnNumber < columnNumber) {
			while(not cellNode.empty() && (XLCellReference(cellNode.attribute("r").value()).column() > columnNumber))
				cellNode = cellNode.previous_sibling_of_type(pugi::node_element);
			// ===== If the backwards search failed to locate the requested cell
			if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)) {
				if(cellNode.empty()) // If between row begin and higher column number, only non-element nodes exist
					cellNode = rowNode.prepend_child("c"); // insert a new cell node at row begin. When saving, this will keep whitespace formatting towards next cell node
				else
					cellNode = rowNode.insert_child_after("c", cellNode);
				setDefaultCellAttributes(cellNode, cellRef.address(), rowNode, columnNumber, colStyles);
			}
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			cellNode = rowNode.first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested columnNumber is <= the column number of the last node_element, therefore this loop will halt:
			while(XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
			// ===== If the forwards search failed to locate the requested cell
			if(XLCellReference(cellNode.attribute("r").value()).column() > columnNumber) {
				cellNode = rowNode.insert_child_before("c", cellNode);
				setDefaultCellAttributes(cellNode, cellRef.address(), rowNode, columnNumber, colStyles);
			}
		}
		return cellNode;
	}

	int findStringInVector(std::string const & nodeName, std::vector <std::string_view> const & nodeOrder)
	{
		for(int i = 0; static_cast<size_t>(i) < nodeOrder.size(); ++i)
			if(nodeName == nodeOrder[i])
				return i;
		return SORT_INDEX_NOT_FOUND;
	}

	void copyLeadingWhitespaces(XMLNode & parent, XMLNode fromNode, XMLNode toNode)
	{
		fromNode = fromNode.previous_sibling(); // move to preceeding whitespace node, if any
		// loop from back to front, inserting in the same order before toNode
		while(fromNode.type() == pugi::node_pcdata) { // loop ends on pugi::node_element or node_null
			toNode = parent.insert_child_before(pugi::node_pcdata, toNode); // prepend as toNode a new pcdata node
			toNode.set_value(fromNode.value()); //  with the value of fromNode
			fromNode = fromNode.previous_sibling();
		}
	}

	XMLNode appendAndGetNode(XMLNode & parent, std::string const & nodeName, std::vector <std::string_view> const & nodeOrder /*= {}*/, bool force_ns /*= false*/)
	{
		if(parent.empty())  
			return XMLNode{};
		XMLNode nextNode = parent.first_child_of_type(pugi::node_element);
		if(nextNode.empty())  
			return parent.prepend_child(nodeName.c_str(), force_ns); // nothing to sort, whitespaces "belong" to parent closing tag
		XMLNode node{}; // empty until successfully created;
		int nodeSortIndex = (nodeOrder.size() > 1 ? findStringInVector(nodeName, nodeOrder) : SORT_INDEX_NOT_FOUND);
		if(nodeSortIndex != SORT_INDEX_NOT_FOUND) {  // can't sort anything if nodeOrder contains less than 2 entries or does not contain nodeName
			// ===== Find first node to follow nodeName per nodeOrder
			while(not nextNode.empty() && findStringInVector(nextNode.name(), nodeOrder) < nodeSortIndex)
				nextNode = nextNode.next_sibling_of_type(pugi::node_element);
			// ===== Evaluate search result
			if(not nextNode.empty()) { // found nodeName or a node before which nodeName should be inserted
				if(nextNode.name() == nodeName) // if nodeName was found
					node = nextNode; // use existing node
				else { // else: a node was found before which nodeName must be inserted
					node = parent.insert_child_before(nodeName.c_str(), nextNode, force_ns); // insert before nextNode without whitespaces
					copyLeadingWhitespaces(parent, node, nextNode);
				}
			}
			// else: no node was found before which nodeName should be inserted: proceed as usual
		}
		else // no possibility to perform ordered insert - attempt to locate existing node:
			node = parent.child(nodeName.c_str());
		if(node.empty() ) {   // neither nodeName, nor a node following per nodeOrder was found
			// ===== There is no reference to perform an ordered insert for nodeName
			nextNode = parent.last_child_of_type(pugi::node_element);           // at least one element node must exist, tested at begin of function
			node = parent.insert_child_after(nodeName.c_str(), nextNode, force_ns); // append as the last element node, but before final whitespaces
			copyLeadingWhitespaces(parent, nextNode, node); // duplicate the prefix whitespaces of nextNode to node
		}
		return node;
	}

	XMLAttribute appendAndGetAttribute(XMLNode & node, std::string const & attrName, std::string const & attrDefaultVal)
	{
		if(node.empty())  
			return XMLAttribute{};
		else {
			XMLAttribute attr = node.attribute(attrName.c_str());
			if(attr.empty()) {
				attr = node.append_attribute(attrName.c_str());
				attr.set_value(attrDefaultVal.c_str());
			}
			return attr;
		}
	}

	XMLAttribute appendAndSetAttribute(XMLNode & node, std::string const & attrName, std::string const & attrVal)
	{
		if(node.empty())  
			return XMLAttribute{};
		else {
			XMLAttribute attr = node.attribute(attrName.c_str());
			if(attr.empty())
				attr = node.append_attribute(attrName.c_str());
			attr.set_value(attrVal.c_str()); // silently fails on empty attribute, which is intended here
			return attr;
		}
	}

	XMLAttribute appendAndGetNodeAttribute(XMLNode & parent, std::string const & nodeName, std::string const & attrName, std::string const & attrDefaultVal,
		std::vector <std::string_view> const & nodeOrder /*= {}*/)
	{
		if(parent.empty())
			return XMLAttribute{};
		else {
			XMLNode node = appendAndGetNode(parent, nodeName, nodeOrder);
			return appendAndGetAttribute(node, attrName, attrDefaultVal);
		}
	}

	XMLAttribute appendAndSetNodeAttribute(XMLNode & parent, std::string const & nodeName, std::string const & attrName, std::string const & attrVal,
		bool removeAttributes /*= XLKeepAttributes*/, std::vector <std::string_view> const & nodeOrder /*= {}*/)
	{
		if(parent.empty())  
			return XMLAttribute{};
		else {
			XMLNode node = appendAndGetNode(parent, nodeName, nodeOrder);
			if(removeAttributes)  
				node.remove_attributes();
			return appendAndSetAttribute(node, attrName, attrVal);
		}
	}

	bool getBoolAttributeWhenOmittedMeansTrue(XMLNode & parent, std::string const & tagName, std::string const & attrName /*= "val"*/)
	{
		if(parent.empty())  
			return false; // can't do anything
		else {
			XMLNode tagNode = parent.child(tagName.c_str());
			if(tagNode.empty() )  
				return false;   // if tag does not exist: return false
			XMLAttribute valAttr = tagNode.attribute(attrName.c_str());
			if(valAttr.empty() ) {                // if no attribute with attrName exists: default to true
				appendAndSetAttribute(tagNode, attrName, "true"); // explicitly create & set attribute
				return true;
			}
			// if execution gets here: attribute with attrName exists
			return valAttr.as_bool(); // return attribute value
		}
	}
}
//
// XLCell.cpp
//
XLCell::XLCell() : m_cellNode(nullptr), m_sharedStrings(XLSharedStringsDefaulted),
	m_valueProxy(XLCellValueProxy(this, m_cellNode.get())), m_formulaProxy(XLFormulaProxy(this, m_cellNode.get()))
{
}
/**
 * @details This constructor creates a XLCell object based on the cell XMLNode input parameter, and is
 * intended for use when the corresponding cell XMLNode already exist.
 * If a cell XMLNode does not exist (i.e., the cell is empty), use the relevant constructor to create an XLCell
 * from a XLCellReference parameter.
 */
XLCell::XLCell(const XMLNode& cellNode, const XLSharedStrings& sharedStrings) : m_cellNode(std::make_unique<XMLNode>(cellNode)),
	m_sharedStrings(sharedStrings), m_valueProxy(XLCellValueProxy(this, m_cellNode.get())),
	m_formulaProxy(XLFormulaProxy(this, m_cellNode.get()))
{
}

XLCell::XLCell(const XLCell& other) : m_cellNode(other.m_cellNode ? std::make_unique<XMLNode>(*other.m_cellNode) : nullptr),
	m_sharedStrings(other.m_sharedStrings), m_valueProxy(XLCellValueProxy(this, m_cellNode.get())),
	m_formulaProxy(XLFormulaProxy(this, m_cellNode.get()))
{
}

XLCell::XLCell(XLCell&& other) noexcept : m_cellNode(std::move(other.m_cellNode)),
	m_sharedStrings(std::move(other.m_sharedStrings)), m_valueProxy(XLCellValueProxy(this, m_cellNode.get())),
	m_formulaProxy(XLFormulaProxy(this, m_cellNode.get()))
{
}

XLCell::~XLCell() = default;

XLCell& XLCell::operator=(const XLCell& other)
{
	if(&other != this) {
		XLCell temp = other;
		std::swap(*this, temp);
	}
	return *this;
}

XLCell& XLCell::operator=(XLCell&& other) noexcept
{
	if(&other != this) {
		m_cellNode      = std::move(other.m_cellNode);
		m_sharedStrings = std::move(other.m_sharedStrings);
		m_valueProxy    = XLCellValueProxy(this, m_cellNode.get());
		m_formulaProxy  = XLFormulaProxy(this, m_cellNode.get());// pull request #160
	}
	return *this;
}

void XLCell::copyFrom(XLCell const& other)
{
	using namespace std::literals::string_literals;
	if(!m_cellNode) {
		// copyFrom invoked by empty XLCell: create a new cell with reference & m_cellNode from other
		m_cellNode      = std::make_unique<XMLNode>(*other.m_cellNode);
		m_sharedStrings = other.m_sharedStrings; // TBD: check for XLSharedStringsDefaulted and avoid copy?
		m_valueProxy    = XLCellValueProxy(this, m_cellNode.get());
		m_formulaProxy  = XLFormulaProxy(this, m_cellNode.get());
		return;
	}
	if((&other != this) && (*other.m_cellNode == *m_cellNode)) // nothing to do
		return;
	// If m_cellNode points to a different XML node than other
	if((&other != this) && (*other.m_cellNode != *m_cellNode)) {
		m_cellNode->remove_children();
		// Copy all XML child nodes
		for(XMLNode child = other.m_cellNode->first_child(); not child.empty(); child = child.next_sibling())  
			m_cellNode->append_copy(child);
		{
			// Delete all XML attributes that are not the cell reference ("r")
			// 2024-07-26 BUGFIX: for-loop was invalidating loop variable with remove_attribute(attr) before advancing to next element
			XMLAttribute attr = m_cellNode->first_attribute();
			while(!attr.empty()) {
				XMLAttribute nextAttr = attr.next_attribute(); // get a handle on next attribute before potentially removing attr
				if(!sstreq(attr.name(), "r"))
					m_cellNode->remove_attribute(attr);// remove all but the cell reference
				attr = nextAttr; // advance to previously stored next attribute
			}
		}
		{
			// Copy all XML attributes that are not the cell reference ("r")
			for(auto attr = other.m_cellNode->first_attribute(); not attr.empty(); attr = attr.next_attribute())
				if(!sstreq(attr.name(), "r"))
					m_cellNode->append_copy(attr);
		}
	}
}

bool XLCell::empty() const { return (!m_cellNode) || m_cellNode->empty(); }
/**
 * @todo 2024-08-10 TBD whether body can be replaced with !empty() (performance?)
 */
XLCell::operator bool() const { return m_cellNode && (not m_cellNode->empty() ); } // ===== 2024-05-28: replaced explicit bool evaluation
/**
 * @details This function returns a const reference to the cellReference property.
 */
XLCellReference XLCell::cellReference() const 
{ 
	return XLCellReference { m_cellNode ? m_cellNode->attribute("r").value() : "" }; 
}
/**
 * @details This function returns a const reference to the cell reference by the offset from the current one.
 */
XLCell XLCell::offset(uint16 rowOffset, uint16 colOffset) const
{
	const XLCellReference offsetRef(cellReference().row() + rowOffset, cellReference().column() + colOffset);
	const auto rownode  = getRowNode(m_cellNode->parent().parent(), offsetRef.row());
	const auto cellnode = getCellNode(rownode, offsetRef.column());
	return XLCell { cellnode, m_sharedStrings.get() };
}

bool XLCell::hasFormula() const { return (m_cellNode && !m_cellNode->child("f").empty()); /*evaluate child XMLNode as boolean*/ }
XLFormulaProxy& XLCell::formula() { return m_formulaProxy; }
/**
 * @details get the value of the s attribute of the cell node
 */
size_t XLCell::cellFormat() const { return m_cellNode->attribute("s").as_uint(0); }
/**
 * @details set the s attribute of the cell node, pointing to an xl/styles.xml cellXfs index
 *          the attribute will be created if not existant, function will fail if attribute creation fails
 */
bool XLCell::setCellFormat(size_t cellFormatIndex)
{
	XMLAttribute attr = m_cellNode->attribute("s");
	if(attr.empty() && not m_cellNode->empty())
		attr = m_cellNode->append_attribute("s");
	attr.set_value(cellFormatIndex); // silently fails on empty attribute, which is intended here
	return attr.empty() == false;
}

void XLCell::print(std::basic_ostream<char>& ostr) const { m_cellNode->print(ostr); }

XLCellAssignable::XLCellAssignable(const XLCell & other) : XLCell(other) 
{
}

XLCellAssignable::XLCellAssignable(const XLCellAssignable & other) : XLCell(other)
{
}

XLCellAssignable::XLCellAssignable(XLCell && other) : XLCell(std::move(other)) 
{
}

XLCellAssignable& XLCellAssignable::operator=(const XLCell& other)
{
	copyFrom(other);
	return *this;
}

XLCellAssignable& XLCellAssignable::operator=(const XLCellAssignable& other)
{
	copyFrom(other);
	return *this;
}

XLCellAssignable& XLCellAssignable::operator=(XLCell&& other) noexcept
{
	copyFrom(other);
	return *this;
}

XLCellAssignable& XLCellAssignable::operator=(XLCellAssignable&& other) noexcept
{
	copyFrom(other);
	return *this;
}

const XLFormulaProxy& XLCell::formula() const { return m_formulaProxy; }
/**
 * @details clear cell contents except for those identified by keep
 */
void XLCell::clear(uint32 keep)
{
	// ===== Clear attributes
	XMLAttribute attr = m_cellNode->first_attribute();
	while(!attr.empty()) {
		XMLAttribute nextAttr = attr.next_attribute();
		std::string attrName = attr.name();
		if((attrName == "r")/*if this is cell reference (must always remain untouched)*/
		    ||((keep & XLKeepCellStyle) && attrName == "s")/*or style shall be kept & this is style*/
		    ||((keep & XLKeepCellType ) && attrName == "t")/*or type shall be kept & this is type*/)
			attr = XMLAttribute{};                    // empty attribute won't get deleted
		// ===== Remove all non-kept attributes
		if(not attr.empty())  
			m_cellNode->remove_attribute(attr);
		attr = nextAttr; // advance to previously determined next cell node attribute
	}
	// ===== Clear node children
	XMLNode node = m_cellNode->first_child();
	while(!node.empty()) {
		XMLNode nextNode = node.next_sibling();
		// ===== Only preserve non-whitespace nodes
		if(node.type() == pugi::node_element) {
			std::string nodeName = node.name();
			if(((keep & XLKeepCellValue  ) && nodeName == "v")// if value shall be kept & this is value
			    ||((keep & XLKeepCellFormula) && nodeName == "f")) // or formula shall be kept & this is formula
				node = XMLNode{}; // empty node won't get deleted
		}
		// ===== Remove all non-kept cell node children
		if(not node.empty())  
			m_cellNode->remove_child(node);
		node = nextNode; // advance to previously determined next cell node child
	}
}

XLCellValueProxy & XLCell::value() { return m_valueProxy; }
const XLCellValueProxy & XLCell::value() const { return m_valueProxy; }
bool XLCell::isEqual(const XLCell& lhs, const XLCell& rhs) { return *lhs.m_cellNode == *rhs.m_cellNode; }
//
// XLCellIterator.cpp
//
namespace OpenXLSX { // utility functions findRowNode and findCellNode
	XMLNode findRowNode(XMLNode sheetDataNode, uint32 rowNumber)
	{
		if(rowNumber < 1 || rowNumber > OpenXLSX::MAX_ROWS) {
			using namespace std::literals::string_literals;
			throw XLCellAddressError("rowNumber "s + std::to_string(rowNumber) + " is outside valid range [1;"s + std::to_string(OpenXLSX::MAX_ROWS) + "]"s);
		}
		// ===== Get the last child of sheetDataNode that is of type node_element.
		XMLNode rowNode = sheetDataNode.last_child_of_type(pugi::node_element);
		// ===== If there are now rows in the worksheet, or the requested row is beyond the current max row, return an empty node
		if(rowNode.empty() || (rowNumber > rowNode.attribute("r").as_ullong()))
			return XMLNode{};
		// ===== If the requested node is closest to the end, start from the end and search backwards.
		if(rowNode.attribute("r").as_ullong() - rowNumber < rowNumber) {
			while(!rowNode.empty() && (rowNode.attribute("r").as_ullong() > rowNumber))
				rowNode = rowNode.previous_sibling_of_type(pugi::node_element);
			if(rowNode.empty() || (rowNode.attribute("r").as_ullong() != rowNumber))
				return XMLNode{};
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			rowNode = sheetDataNode.first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested rowNumber is <= the row number of the last node_element, therefore this loop will halt.
			while(rowNode.attribute("r").as_ullong() < rowNumber)
				rowNode = rowNode.next_sibling_of_type(pugi::node_element);
			if(rowNode.attribute("r").as_ullong() > rowNumber)
				return XMLNode{};
		}
		return rowNode;
	}

	XMLNode findCellNode(XMLNode rowNode, uint16 columnNumber)
	{
		if(columnNumber < 1 || columnNumber > OpenXLSX::MAX_COLS) {
			using namespace std::literals::string_literals;
			throw XLException("XLWorksheet::column: columnNumber "s + std::to_string(columnNumber) + " is outside allowed range [1;"s + std::to_string(MAX_COLS) + "]"s);
		}
		if(rowNode.empty())  
			return XMLNode{};
		XMLNode cellNode = rowNode.last_child_of_type(pugi::node_element);
		// ===== If there are no cells in the current row, or the requested cell is beyond the last cell in the row...
		if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
			return XMLNode{};
		// ===== If the requested node is closest to the end, start from the end and search backwards...
		if(XLCellReference(cellNode.attribute("r").value()).column() - columnNumber < columnNumber) {
			while(!cellNode.empty() && (XLCellReference(cellNode.attribute("r").value()).column() > columnNumber))
				cellNode = cellNode.previous_sibling_of_type(pugi::node_element);
			if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
				return XMLNode{};
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			cellNode = rowNode.first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested columnNumber is <= the column number of the last node_element, therefore this loop will halt:
			while(XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
			if(XLCellReference(cellNode.attribute("r").value()).column() > columnNumber)
				return XMLNode{};
		}
		return cellNode;
	}
} // namespace OpenXLSX

XLCellIterator::XLCellIterator(const XLCellRange& cellRange, XLIteratorLocation loc, std::vector<XLStyleIndex> const * colStyles) : 
	m_dataNode(std::make_unique<XMLNode>(*cellRange.m_dataNode)), m_topLeft(cellRange.m_topLeft), m_bottomRight(cellRange.m_bottomRight),
	m_sharedStrings(cellRange.m_sharedStrings), m_endReached(false), m_hintNode(), m_hintRow(0), m_currentCell(),
	m_currentCellStatus(XLNotLoaded), m_currentRow(0), m_currentColumn(0), m_colStyles(colStyles)
{
	if(loc == XLIteratorLocation::End)
		m_endReached = true;
	else {
		m_currentRow    = m_topLeft.row();
		m_currentColumn = m_topLeft.column();
	}
	if(m_colStyles == nullptr)
		throw XLInternalError("XLCellIterator constructor parameter colStyles must not be nullptr");
// std::cout << "XLCellIterator constructed with topLeft " << m_topLeft.address() << " and bottomRight " << m_bottomRight.address() << std::endl;
// std::cout << "XLCellIterator m_endReached is " << ( m_endReached ? "true" : "false" ) << std::endl;
}

XLCellIterator::~XLCellIterator() = default;

XLCellIterator::XLCellIterator(const XLCellIterator& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)),
	m_topLeft(other.m_topLeft), m_bottomRight(other.m_bottomRight), m_sharedStrings(other.m_sharedStrings), m_endReached(other.m_endReached),
	m_hintNode(other.m_hintNode), m_hintRow(other.m_hintRow), m_currentCell(other.m_currentCell), m_currentCellStatus(other.m_currentCellStatus),
	m_currentRow(other.m_currentRow), m_currentColumn(other.m_currentColumn), m_colStyles(other.m_colStyles)
{
}

XLCellIterator::XLCellIterator(XLCellIterator&& other) noexcept = default;

XLCellIterator& XLCellIterator::operator=(const XLCellIterator& other)
{
	if(&other != this) {
		*m_dataNode     = *other.m_dataNode;
		m_topLeft       =  other.m_topLeft;
		m_bottomRight   =  other.m_bottomRight;
		m_sharedStrings =  other.m_sharedStrings;
		m_endReached    =  other.m_endReached;
		m_hintNode      =  other.m_hintNode;
		m_hintRow       =  other.m_currentRow;
		m_currentCell   =  other.m_currentCell;
		m_currentCellStatus = other.m_currentCellStatus;
		m_currentRow    =  other.m_currentRow;
		m_currentColumn =  other.m_currentColumn;
		m_colStyles     =  other.m_colStyles;
	}
	return *this;
}

XLCellIterator& XLCellIterator::operator=(XLCellIterator&& other) noexcept = default;
// {
//     if (&other != this) {
//         m_dataNode      = std::move(other.m_dataNode);
//         m_topLeft       = std::move(other.m_topLeft);
//         m_bottomRight   = std::move(other.m_bottomRight);
//         m_sharedStrings = std::move(other.m_sharedStrings);
//         m_endReached    = other.m_endReached;
//         m_hintNode      = std::move(other.m_hintNode);
//         m_hintRow       = other.m_currentRow;
//         m_currentCell   = std::move(other.m_currentCell);
//         m_currentCellStatus = other.m_currentCellStatus;
//         m_currentRow    =  other.m_currentRow;
//         m_currentColumn =  other.m_currentColumn;
//         m_colStyles     =  other.m_colStyles;
//     }
//
//     return *this;
// }

/**
 * @brief update m_currentCell by fetching (or inserting) a cell at m_currentRow, m_currentColumn
 */
void XLCellIterator::updateCurrentCell(bool createIfMissing)
{
	// ===== Quick exit checks - can't be true when m_endReached
	if(m_currentCellStatus == XLLoaded)  
		return; // nothing to do, cell is already loaded
	if(!createIfMissing && m_currentCellStatus == XLNoSuchCell)  
		return; // nothing to do, cell has already been determined as missing
	// At this stage, m_currentCellStatus is XLUnloaded or XLNoSuchCell and createIfMissing == true
	if(m_endReached)
		throw XLInputError("XLCellIterator updateCurrentCell: iterator should not be dereferenced when endReached() == true");
	// ===== Cell needs to be updated
	if(m_hintNode.empty()) { // no hint has been established: fetch first cell node the "tedious" way
		if(createIfMissing) // getCellNode / getRowNode create missing cells
			m_currentCell = XLCell(getCellNode(getRowNode(*m_dataNode, m_currentRow), m_currentColumn, 0, *m_colStyles), m_sharedStrings.get());
		else // findCellNode / findRowNode return an empty cell for missing cells
			m_currentCell = XLCell(findCellNode(findRowNode(*m_dataNode, m_currentRow), m_currentColumn), m_sharedStrings.get());
	}
	else {
		// ===== Find or create, and fetch an XLCell at m_currentRow, m_currentColumn
		if(m_currentRow == m_hintRow) { // new cell is within the same row
			// ===== Start from m_hintNode and search forwards...
			XMLNode cellNode = m_hintNode.next_sibling_of_type(pugi::node_element);
			uint16 colNo = 0;
			while(!cellNode.empty()) {
				colNo = XLCellReference(cellNode.attribute("r").value()).column();
				if(colNo >= m_currentColumn) 
					break; // if desired cell was reached / passed, break before incrementing cellNode
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
			}
			if(colNo != m_currentColumn)  
				cellNode = XMLNode{};// if a higher column number was found, set empty node (means: "missing")
			// ===== Create missing cell node if createIfMissing == true
			if(createIfMissing && cellNode.empty()) {
				cellNode = m_hintNode.parent().insert_child_after("c", m_hintNode);
				setDefaultCellAttributes(cellNode, XLCellReference(m_currentRow, m_currentColumn).address(), m_hintNode.parent(),
				    /**/ m_currentColumn, *m_colStyles);
			}
			m_currentCell = XLCell(cellNode, m_sharedStrings.get()); // cellNode.empty() can be true if createIfMissing == false and cell is not found
		}
		else if(m_currentRow > m_hintRow) {
			// ===== Start from m_hintNode parent row and search forwards...
			XMLNode rowNode = m_hintNode.parent().next_sibling_of_type(pugi::node_element);
			uint32 rowNo = 0;
			while(!rowNode.empty()) {
				rowNo = static_cast<uint32>(rowNode.attribute("r").as_ullong());
				if(rowNo >= m_currentRow)
					break;// if desired row was reached / passed, break before incrementing rowNode
				rowNode = rowNode.next_sibling_of_type(pugi::node_element);
			}
			if(rowNo != m_currentRow)  rowNode = XMLNode{};// if a higher row number was found, set empty node (means: "missing")
			// ===== Create missing row node if createIfMissing == true
			if(createIfMissing && rowNode.empty()) {
				rowNode = m_dataNode->insert_child_after("row", m_hintNode.parent());
				rowNode.append_attribute("r").set_value(m_currentRow);
			}
			if(rowNode.empty()) // if row could not be found / created
				m_currentCell = XLCell{}; // make sure m_currentCell is set to an empty cell
			else {      // else: row found
				if(createIfMissing) {
					// ===== Pass the already known m_currentRow to getCellNode so that it does not have to be fetched again
					m_currentCell = XLCell(getCellNode(rowNode, m_currentColumn, m_currentRow, *m_colStyles), m_sharedStrings.get());
				}
				else // ===== Do a "soft find" if a missing cell shall not be created
					m_currentCell = XLCell(findCellNode(rowNode, m_currentColumn), m_sharedStrings.get());
			}
		}
		else
			throw XLInternalError("XLCellIterator::updateCurrentCell: an internal error occured (m_currentRow < m_hintRow)");
	}
	if(m_currentCell.empty()) // if cell is confirmed missing
		m_currentCellStatus = XLNoSuchCell; // mark this status for further calls to updateCurrentCell()
	else {
		// ===== If the current cell exists, update the hints
		m_hintNode   = *m_currentCell.m_cellNode;// 2024-08-11: don't store a full XLCell, just the XMLNode, for better performance
		m_hintRow    = m_currentRow;
		m_currentCellStatus = XLLoaded; // mark cell status for further calls to updateCurrentCell()
	}
}

XLCellIterator& XLCellIterator::operator++()
{
	if(m_endReached)
		throw XLInputError("XLCellIterator: tried to increment beyond end operator");
	if(m_currentColumn < m_bottomRight.column())
		++m_currentColumn;
	else if(m_currentRow < m_bottomRight.row()) {
		++m_currentRow;
		m_currentColumn = m_topLeft.column();
	}
	else
		m_endReached = true;
	m_currentCellStatus = XLNotLoaded; // trigger a new attempt to locate / create the cell via updateCurrentCell
	return *this;
}

XLCellIterator XLCellIterator::operator++(int)
{
	auto oldIter(*this);
	++(*this);
	return oldIter;
}

XLCell& XLCellIterator::operator*()
{
	// std::cout << "XLCellIterator dereference operator* invoked" << std::endl;
	updateCurrentCell(XLCreateIfMissing);
	return m_currentCell;
}

XLCellIterator::pointer XLCellIterator::operator->()
{
// std::cout << "XLCellIterator dereference operator-> invoked" << std::endl;
	updateCurrentCell(XLCreateIfMissing);
	return &m_currentCell;
}

bool XLCellIterator::operator==(const XLCellIterator& rhs) const
{
	// BUGFIX 2024-08-10: there was no test for (!m_currentCell && rhs.m_currentCell),
	//     leading to a potential dereference of a nullptr in m_currentCell::m_cellNode

	if(m_endReached && rhs.m_endReached)  
		return true;// If both iterators are end iterators
	if((m_currentColumn != rhs.m_currentColumn)       // if iterators point to a different column or row
	    ||(m_currentRow    != rhs.m_currentRow))
		return false;                                 // that means no match
	// CAUTION: for performance reasons, disabled all checks whether this and rhs are iterators on the same worksheet & range
	return true;
	// if (*m_dataNode != *rhs.m_dataNode) return false;     // TBD: iterators over different worksheets may never match
	// TBD if iterators shall be considered not equal if they were created on different XLCellRanges
	// this would require checking the topLeft and bottomRight references, potentially costing CPU time

	// return m_currentCell == rhs.m_currentCell;   // match only if cell nodes are equal
	// CAUTION: in the current code, that means iterators that point to the same column & row in different worksheets,
	// and cells that do not exist in both sheets, will be considered equal
}

bool XLCellIterator::operator!=(const XLCellIterator& rhs) const { return !(*this == rhs); }

bool XLCellIterator::cellExists()
{
	// ===== Update m_currentCell once so that cellExists will always test the correct cell (an empty cell if current cell doesn't exist)
	updateCurrentCell(XLDoNotCreateIfMissing);
	return not m_currentCell.empty();
}
/**
 * @details
 * @note 2024-06-03: implemented a calculated distance based on m_currentCell, m_topLeft and m_bottomRight (if m_endReached)
 *                   accordingly, implemented defined setting of m_endReached at all times
 */
uint64 XLCellIterator::distance(const XLCellIterator& last)
{
	// Determine rows and columns, taking into account beyond-the-end iterators
	const uint32 row = (m_endReached ? m_bottomRight.row() : m_currentRow);
	const uint16 col = (m_endReached ? m_bottomRight.column() + 1 : m_currentColumn);
	const uint32 lastRow = (last.m_endReached ? last.m_bottomRight.row() : last.m_currentRow);
	// ===== lastCol can store +1 for beyond-the-end iterator without overflow because MAX_COLS is less than max uint16
	const uint16 lastCol = (last.m_endReached ? last.m_bottomRight.column() + 1 : last.m_currentColumn);
	const uint16 rowWidth = m_bottomRight.column() - m_topLeft.column() + 1; // amount of cells in a row of the iterator range
	const int64_t distance =  ((int64_t)(lastRow) - row) * rowWidth/*row distance * rowWidth*/ + (int64_t)(lastCol) - col/*+ column distance (may be negative)*/;
	if(distance < 0)
		throw XLInputError("XLCellIterator::distance is negative");
	return static_cast<uint64>(distance); // after excluding negative result: cast back to positive value
}

std::string XLCellIterator::address() const
{
	const uint32 row = (m_endReached ? m_bottomRight.row() : m_currentRow);
	const uint16 col = (m_endReached ? m_bottomRight.column() + 1 : m_currentColumn);
	return (m_endReached ? "END(" : "") + XLCellReference(row, col).address() + (m_endReached ? ")" : "");
}
//
// XLCellRange.cpp
//
XLCellRange::XLCellRange() : m_dataNode(std::make_unique<XMLNode>(XMLNode{})), m_topLeft(XLCellReference("A1")),
	m_bottomRight(XLCellReference("A1")), m_sharedStrings(XLSharedStringsDefaulted), m_columnStyles{}
{
}
/**
 * @details From the two XLCellReference objects, the constructor calculates the dimensions of the range.
 * If the range exceeds the current bounds of the spreadsheet, the spreadsheet is resized to fit.
 */
XLCellRange::XLCellRange(const XMLNode & dataNode, const XLCellReference& topLeft, const XLCellReference& bottomRight, const XLSharedStrings& sharedStrings) : 
	m_dataNode(std::make_unique<XMLNode>(dataNode)), m_topLeft(topLeft), m_bottomRight(bottomRight), m_sharedStrings(sharedStrings), m_columnStyles{}
{
	if(m_topLeft.row() > m_bottomRight.row() || m_topLeft.column() > m_bottomRight.column()) {
		using namespace std::literals::string_literals;
		throw XLInputError("XLCellRange constructor: topLeft ("s + topLeft.address() + ")"s + 
			" does not point to a lower or equal row and column than bottomRight ("s + bottomRight.address() + ")"s);
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
	uint16 vecPos = 0;
	XMLNode col = cols.first_child_of_type(pugi::node_element);
	while(!col.empty()) {
		uint16 minCol = static_cast<uint16>(col.attribute("min").as_int(0));
		uint16 maxCol = static_cast<uint16>(col.attribute("max").as_int(0));
		if(minCol > maxCol || !minCol || !maxCol) {
			using namespace std::literals::string_literals;
			throw XLInputError("column attributes min (\""s + col.attribute("min").value() + "\") and max (\""s + col.attribute("min").value() + "\")"s
				      " must be set and min must not be larger than max"s);
		}
		if(maxCol > m_columnStyles.size())  
			m_columnStyles.resize(maxCol); // resize m_columnStyles if necessary
		for(; vecPos + 1 < minCol; ++vecPos) 
			m_columnStyles[vecPos] = XLDefaultCellFormat; // set all non-defined columns to default
		XLStyleIndex colStyle = col.attribute("style").as_uint(XLDefaultCellFormat);      // acquire column style attribute
		for(; vecPos < maxCol; ++vecPos) 
			m_columnStyles[vecPos] = colStyle; // set all covered columns to defined style
		col = col.next_sibling_of_type(pugi::node_element); // advance to next <col> entry, if any
	}
}

const XLCellReference XLCellRange::topLeft() const { return m_topLeft; }
const XLCellReference XLCellRange::bottomRight() const { return m_bottomRight; }
/**
 * @details Evaluate the top left and bottom right cells as string references and concatenate them with a colon ':'
 */
std::string XLCellRange::address() const { return m_topLeft.address() + ":" + m_bottomRight.address(); }
uint32 XLCellRange::numRows() const { return m_bottomRight.row() + 1 - m_topLeft.row(); }
uint16 XLCellRange::numColumns() const { return m_bottomRight.column() + 1 - m_topLeft.column(); }
XLCellIterator XLCellRange::begin() const { return XLCellIterator(*this, XLIteratorLocation::Begin, &m_columnStyles); }
XLCellIterator XLCellRange::end() const { return XLCellIterator(*this, XLIteratorLocation::End, &m_columnStyles); }

void XLCellRange::clear()
{
	for(auto& cell : *this)
		cell.value().clear();
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
//
// XLCellReference.cpp
//
constexpr uint8 alphabetSize = 26;
constexpr uint8 asciiOffset = 64;

namespace {
	bool addressIsValid(uint32 row, uint16 column)
	{
		return !(row < 1 || row > OpenXLSX::MAX_ROWS || column < 1 || column > OpenXLSX::MAX_COLS);
	}
}
/**
 * @details The constructor creates a new XLCellReference from a string, e.g. 'A1'. If there's no input,
 * the default reference will be cell A1.
 */
XLCellReference::XLCellReference(const std::string& cellAddress)
{
	if(!cellAddress.empty())  
		setAddress(cellAddress);
	if(cellAddress.empty() && !addressIsValid(m_row, m_column)) { // 2024-04-25: throw exception on empty string // @sobolev @fix ||-->&& 
		throw XLCellAddressError("Cell reference is invalid");
		// ===== 2024-05-27: below code is obsolete due to exception on invalid cellAddress
		// m_row         = 1;
		// m_column      = 1;
		// m_cellAddress = "A1";
	}
}
/**
 * @details This constructor creates a new XLCellReference from a given row and column number, e.g. 1,1 (=A1)
 * @todo consider swapping the arguments.
 */
XLCellReference::XLCellReference(uint32 row, uint16 column)
{
	if(!addressIsValid(row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	setRowAndColumn(row, column);
}
/**
 * @details This constructor creates a new XLCellReference from a row number and the column name (e.g. 1, A)
 * @todo consider swapping the arguments.
 */
XLCellReference::XLCellReference(uint32 row, const std::string& column)
{
	if(!addressIsValid(row, columnAsNumber(column)))  
		throw XLCellAddressError("Cell reference is invalid");
	setRowAndColumn(row, columnAsNumber(column));
}

XLCellReference::XLCellReference(const XLCellReference& other) = default;
XLCellReference::XLCellReference(XLCellReference&& other) noexcept = default;
XLCellReference::~XLCellReference() = default;
XLCellReference& XLCellReference::operator=(const XLCellReference& other) = default;
XLCellReference& XLCellReference::operator=(XLCellReference&& other) noexcept = default;

XLCellReference& XLCellReference::operator++()
{
	if(m_column < MAX_COLS) {
		setColumn(m_column + 1);
	}
	else if(m_column == MAX_COLS && m_row < MAX_ROWS) {
		m_column = 1;
		setRow(m_row + 1);
	}
	else if(m_column == MAX_COLS && m_row == MAX_ROWS) {
		m_column      = 1;
		m_row         = 1;
		m_cellAddress = "A1";
	}
	return *this;
}

XLCellReference XLCellReference::operator++(int)
{
	auto oldRef(*this);
	++(*this);
	return oldRef;
}

XLCellReference& XLCellReference::operator--()
{
	if(m_column > 1) {
		setColumn(m_column - 1);
	}
	else if(m_column == 1 && m_row > 1) {
		m_column = MAX_COLS;
		setRow(m_row - 1);
	}
	else if(m_column == 1 && m_row == 1) {
		m_column      = MAX_COLS;
		m_row         = MAX_ROWS;
		m_cellAddress = "XFD1048576"; // this address represents the very last cell that an excel spreadsheet can reference / support
	}
	return *this;
}

XLCellReference XLCellReference::operator--(int)
{
	auto oldRef(*this);
	--(*this);
	return oldRef;
}
/**
 * @details Returns the m_row property.
 */
uint32 XLCellReference::row() const { return m_row; }
/**
 * @details Sets the row of the XLCellReference objects. If the number is larger than 16384 (the maximum),
 * the row is set to 16384.
 */
void XLCellReference::setRow(uint32 row)
{
	if(!addressIsValid(row, m_column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_row = row;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}
/**
 * @details Returns the m_column property.
 */
uint16 XLCellReference::column() const { return m_column; }
/**
 * @details Sets the column of the XLCellReference object. If the number is larger than 1048576 (the maximum),
 * the column is set to 1048576.
 */
void XLCellReference::setColumn(uint16 column)
{
	if(!addressIsValid(m_row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_column      = column;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}
/**
 * @details Sets row and column of the XLCellReference object. Checks that row and column is less than
 * or equal to the maximum row and column numbers allowed by Excel.
 */
void XLCellReference::setRowAndColumn(uint32 row, uint16 column)
{
	if(!addressIsValid(row, column))  
		throw XLCellAddressError("Cell reference is invalid");
	m_row         = row;
	m_column      = column;
	m_cellAddress = columnAsString(m_column) + rowAsString(m_row);
}
/**
 * @details Returns the m_cellAddress property.
 */
std::string XLCellReference::address() const { return m_cellAddress; }
/**
 * @details Sets the address of the XLCellReference object, e.g. 'B2'. Checks that row and column is less than
 * or equal to the maximum row and column numbers allowed by Excel.
 */
void XLCellReference::setAddress(const std::string& address)
{
	const auto [fst, snd] = coordinatesFromAddress(address);
	m_row         = fst;
	m_column      = snd;
	m_cellAddress = address;
}

std::string XLCellReference::rowAsString(uint32 row)
{
#ifdef CHARCONV_ENABLED
	std::array<char, 7> str {};
	const auto * p = std::to_chars(str.data(), str.data() + str.size(), row).ptr;
	return std::string { str.data(), static_cast<uint16>(p - str.data()) };
#else
	std::string result;
	while(row != 0) {
		int rem = row % 10;
		result += (rem > 9) ? (rem - 10) + 'a' : rem + '0';
		row = row / 10;
	}
	for(uint i = 0; i < result.length() / 2; i++)  
		std::swap(result[i], result[result.length() - i - 1]);
	return result;
#endif
}

uint32 XLCellReference::rowAsNumber(const std::string& row)
{
#ifdef CHARCONV_ENABLED
	uint32 value = 0;
	std::from_chars(row.data(), row.data() + row.size(), value);
	return value;
#else
	return stoul(row);
#endif
}
/**
 * @details Helper method to calculate the column letter from column number.
 */
std::string XLCellReference::columnAsString(uint16 column)
{
	std::string result;
	// ===== If there is one letter in the Column Name:
	if(column <= alphabetSize)  
		result += static_cast<char>(column + asciiOffset);
	// ===== If there are two letters in the Column Name:
	else if(column > alphabetSize && column <= alphabetSize * (alphabetSize + 1)) {
		result += static_cast<char>((column - (alphabetSize + 1)) / alphabetSize + asciiOffset + 1);
		result += static_cast<char>((column - (alphabetSize + 1)) % alphabetSize + asciiOffset + 1);
	}
	// ===== If there are three letters in the Column Name:
	else {
		result += static_cast<char>((column - 703) / (alphabetSize * alphabetSize) + asciiOffset + 1);
		result += static_cast<char>(((column - 703) / alphabetSize) % alphabetSize + asciiOffset + 1);
		result += static_cast<char>((column - 703) % alphabetSize + asciiOffset + 1);
	}
	return result;
}
/**
 * @details Helper method to calculate the column number from column letter.
 * @throws XLInputError
 * @note 2024-06-03: added check for valid address
 */
uint16 XLCellReference::columnAsNumber(const std::string& column)
{
	uint64 letterCount = 0;
	uint32 colNo = 0;
	for(const auto letter : column) {
		if(isasciiupr(letter)) { // allow only uppercase letters
			++letterCount;
			colNo = colNo * 26 + (letter - 'A' + 1);
		}
		else
			break;
	}
	// ===== If the full string was decoded and colNo is within allowed range [1;MAX_COLS]
	if(letterCount == column.length() && colNo > 0 && colNo <= MAX_COLS)
		return static_cast<uint16>(colNo);
	throw XLInputError("XLCellReference::columnAsNumber - column \"" + column + "\" is invalid");
	/* 2024-06-19 OBSOLETE CODE:
	   // uint16 result = 0;
	   // uint16 factor = 1;
	   //
	   // for (int16 i = static_cast<int16>(column.size() - 1); i >= 0; --i) {
	   //     result += static_cast<uint16>((column[static_cast<uint64>(i)] - asciiOffset) * factor);
	   //     factor *= alphabetSize;
	   // }
	   //
	   // return result;
	 */
}
/**
 * @details Helper method for calculating the coordinates from the cell address.
 * @throws XLInputError
 * @note 2024-06-03: added check for valid address
 */
XLCoordinates XLCellReference::coordinatesFromAddress(const std::string& address)
{
	uint64 letterCount = 0;
	uint32 colNo = 0;
	for(const auto letter : address) {
		if(isasciiupr(letter)) { // allow only uppercase letters
			++letterCount;
			colNo = colNo * 26 + (letter - 'A' + 1);
		}
		else
			break;
	}
	// ===== If address contains between 1 and 3 letters and has at least 1 more character for the row
	if(colNo > 0 && colNo <= MAX_COLS && address.length() > letterCount) {
		size_t pos = static_cast<size_t>(letterCount);
		uint64 rowNo = 0;
		for(; pos < address.length() && std::isdigit(address[pos]); ++pos) // check digits
			rowNo = rowNo * 10 + (address[pos] - '0');
		if(pos == address.length() && rowNo <= MAX_ROWS) // full address was < 4 letters + only digits
			return std::make_pair(rowNo, colNo);
	}
	throw XLInputError("XLCellReference::coordinatesFromAddress - address \"" + address + "\" is invalid");
	/* 2024-06-19 OBSOLETE CODE
	   // auto it = std::find_if(address.begin(), address.end(), ::isdigit);
	   // auto columnPart = std::string(address.begin(), it);
	   // auto rowPart = std::string(it, address.end());
	   //
	   // return std::make_pair(rowAsNumber(rowPart), columnAsNumber(columnPart));
	 */
}

static const SIntToSymbTabEntry XLValueTypeSymbList[] = {
	{ static_cast<int>(XLValueType::Empty), "empty" },
	{ static_cast<int>(XLValueType::Boolean), "boolean" },
	{ static_cast<int>(XLValueType::Integer), "integer" },
	{ static_cast<int>(XLValueType::Float), "float" },
	{ static_cast<int>(XLValueType::String), "string" },
};

std::string OpenXLSX::GetValueTypeSymb(XLValueType typ) // @sobolev
{
	const char * p_symb = SIntToSymbTab_GetSymbPtr(XLValueTypeSymbList, SIZEOFARRAY(XLValueTypeSymbList), static_cast<int>(typ));
	return std::string(p_symb ? p_symb : "error");
}
//
// XLCellValue.cpp
//
double VisitXLCellValueTypeToDouble::operator()(std::string v) const 
{
	throw XLValueTypeError("string is not convertible to double."); // disable if implicit conversion of string to double shall be allowed
	size_t pos;
	double dVal = stod(v, &pos);
	while(v[pos] == ' ' || v[pos] == '\t')  
		++pos; // skip over potential trailing whitespaces
	// NOTE: std::string zero-termination is guaranteed, so the above loop will halt
	if(pos != v.length())
		throw XLValueTypeError("string is not convertible to double."); // throw if the *full value* does not convert to double
	return dVal;
}

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
std::string XLCellValue::typeAsString() const { return GetValueTypeSymb(type()); }
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
	if(!m_cellNode->child("v"))  
		m_cellNode->append_child("v");
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
std::string XLCellValueProxy::typeAsString() const { return GetValueTypeSymb(type()); }

std::string XLCellValueProxy::getString() const
{
	try {
		return std::visit(VisitXLCellValueTypeToString(), getValue().m_value);
	}
	catch(std::string s) {
		throw XLValueTypeError("XLCellValue object is not convertible to string.");
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
	if(!sstreq(m_cellNode->attribute("t").value(), "s"))
		return -1; // cell value is not a shared string
	else
		return static_cast<int32_t>(m_cellNode->child("v").text().as_ullong(_FFFF64/*-1*/)); // return the shared string index stored for this cell
			/**/ // if, for whatever reason, the underlying XML has no reference stored, also return -1
}

bool XLCellValueProxy::setStringIndex(int32_t newIndex)
{
	if(newIndex < 0 || !sstreq(m_cellNode->attribute("t").value(), "s"))
		return false; // cell value is not a shared string
	else
		return m_cellNode->child("v").text().set(newIndex); // set the shared string index directly
}
//
// XLColor.cpp
//
XLColor::XLColor() = default;
XLColor::XLColor(uint8 alpha, uint8 red, uint8 green, uint8 blue) : m_alpha(alpha), m_red(red), m_green(green), m_blue(blue) 
{
}

XLColor::XLColor(uint8 red, uint8 green, uint8 blue) : m_red(red), m_green(green), m_blue(blue) 
{
}

XLColor::XLColor(const std::string& hexCode) { set(hexCode); }
XLColor::XLColor(const XLColor& other) = default;
XLColor::XLColor(XLColor&& other) noexcept = default;
XLColor::~XLColor() = default;
XLColor& XLColor::operator=(const XLColor& other) = default;
XLColor& XLColor::operator=(XLColor&& other) noexcept = default;

void XLColor::set(uint8 alpha, uint8 red, uint8 green, uint8 blue)
{
	m_alpha = alpha;
	m_red   = red;
	m_green = green;
	m_blue  = blue;
}

void XLColor::set(uint8 red, uint8 green, uint8 blue)
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
		blue  = hexCode.substr(6, 2);
	}
	else
		throw XLInputError("Invalid color code");
	constexpr int hexBase = 16;
	m_alpha = static_cast<uint8>(stoul(alpha, nullptr, hexBase));
	m_red   = static_cast<uint8>(stoul(red, nullptr, hexBase));
	m_green = static_cast<uint8>(stoul(green, nullptr, hexBase));
	m_blue  = static_cast<uint8>(stoul(blue, nullptr, hexBase));
}

uint8 XLColor::alpha() const { return m_alpha; }
uint8 XLColor::red() const { return m_red; }
uint8 XLColor::green() const { return m_green; }
uint8 XLColor::blue() const { return m_blue; }

std::string XLColor::hex() const
{
	std::stringstream str;
	constexpr int hexBase = 16;
	if(m_alpha < hexBase)  
		str << "0";
	str << std::hex << static_cast<int>(m_alpha);
	if(m_red < hexBase)  
		str << "0";
	str << std::hex << static_cast<int>(m_red);
	if(m_green < hexBase)  
		str << "0";
	str << std::hex << static_cast<int>(m_green);
	if(m_blue < hexBase)  
		str << "0";
	str << std::hex << static_cast<int>(m_blue);
	return (str.str());
}
//
// XLColumn.cpp
//
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
	if(widthAtt.empty())  
		widthAtt = columnNode().append_attribute("width");
	widthAtt.set_value(width);
	// Set the 'customWidth' attribute for the Cell. If it does not exist, create it.
	auto customAtt = columnNode().attribute("customWidth");
	if(customAtt.empty())  
		customAtt = columnNode().append_attribute("customWidth");
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
	if(styleAtt.empty())  
		styleAtt = columnNode().append_attribute("style");
	if(styleAtt.empty())  
		return false;
	else {
		styleAtt.set_value(cellFormatIndex);
		return true;
	}
};
//
// XLComments.cpp
//
namespace {
	// module-local utility functions
	std::string getCommentString(XMLNode const & commentNode)
	{
		std::string result{};
		using namespace std::literals::string_literals;
		XMLNode textElement = commentNode.child("text").first_child_of_type(pugi::node_element);
		while(!textElement.empty()) {
			if(textElement.name() == "t"s) {
				result += textElement.first_child().value();
			}
			else if(textElement.name() == "r"s) { // rich text
				XMLNode richTextSubnode = textElement.first_child_of_type(pugi::node_element);
				while(!richTextSubnode.empty()) {
					if(textElement.name() == "t"s) {
						result += textElement.first_child().value();
					}
					else if(textElement.name() == "rPr"s) {
					} // ignore rich text formatting info
					else {
					} // ignore other nodes
					richTextSubnode = richTextSubnode.next_sibling_of_type(pugi::node_element);
				}
			}
			else {
			}   // ignore other elements (for now)
			textElement = textElement.next_sibling_of_type(pugi::node_element);
		}
		return result;
	}
}

// ========== XLComment Member Functions

// XLComment::XLComment() : m_commentNode(std::make_unique<XMLNode>()) {}

XLComment::XLComment(const XMLNode& node) : m_commentNode(std::make_unique<XMLNode>(node))
{
}
/**
 * @details
 * @note Function body moved to cpp module as it uses "not" keyword for readability, which MSVC sabotages with non-CPP compatibility.
 * @note For the library it is reasonable to expect users to compile it with MSCV /permissive- flag, but for the user's own projects the header files shall "just work"
 */
bool XLComment::valid() const { return m_commentNode != nullptr &&(not m_commentNode->empty()); }
/**
 * @brief Getter functions
 */
std::string XLComment::ref() const { return m_commentNode->attribute("ref").value(); }
std::string XLComment::text() const { return getCommentString(*m_commentNode); }
uint16 XLComment::authorId() const { return static_cast<uint16>(m_commentNode->attribute("authorId").as_uint()); }

/**
 * @brief Setter functions
 */
bool XLComment::setText(std::string newText)
{
	m_commentNode->remove_children(); // clear previous text
	XMLNode tNode = m_commentNode->prepend_child("text").prepend_child("t"); // insert <text><t/></text> nodes
	tNode.append_attribute("xml:space").set_value("preserve");            // set <t> node attribute xml:space
	return tNode.prepend_child(pugi::node_pcdata).set_value(newText.c_str()); // finally, insert <t> node_pcdata value
}

bool XLComment::setAuthorId(uint16 newAuthorId) { return appendAndSetAttribute(*m_commentNode, "authorId", std::to_string(newAuthorId)).empty() == false; }

// ========== XLComments Member Functions

XLComments::XLComments() : XLXmlFile(nullptr), m_vmlDrawing(std::make_unique<XLVmlDrawing>())
{
}
/**
 * @details The constructor creates an instance of the superclass, XLXmlFile
 */
XLComments::XLComments(XLXmlData* xmlData) : XLXmlFile(xmlData), m_vmlDrawing(std::make_unique<XLVmlDrawing>())
{
	if(xmlData->getXmlType() != XLContentType::Comments)
		throw XLInternalError("XLComments constructor: Invalid XML data.");
	OXlXmlDoc & doc = xmlDocument();
	if(doc.document_element().empty()) // handle a bad (no document element) comments XML file
		doc.load_string(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<comments xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
			" xmlns:xdr=\"http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing\">\n"
			"</comments>",
			pugi_parse_settings);
	XMLNode rootNode = doc.document_element();
	bool docNew = rootNode.first_child_of_type(pugi::node_element).empty(); // check and store status: was document empty?
	m_authors = appendAndGetNode(rootNode, "authors", m_nodeOrder);       // (insert and) get authors node
	if(docNew)
		rootNode.prepend_child(pugi::node_pcdata).set_value("\n\t");// if document was empty: prefix the newly inserted authors node with a single tab
	m_commentList = appendAndGetNode(rootNode, "commentList", m_nodeOrder); // (insert and) get commentList node -> this should now copy the whitespace prefix of m_authors
	// whitespace formatting / indentation for closing tags:
	if(m_authors.first_child().empty())
		m_authors.append_child(pugi::node_pcdata).set_value("\n\t");
	if(m_commentList.first_child().empty())
		m_commentList.append_child(pugi::node_pcdata).set_value("\n\t");
}
/**
 * @details copy-construct an XLComments object
 */
XLComments::XLComments(const XLComments& other) : XLXmlFile(other), m_authors(other.m_authors),
	m_commentList(other.m_commentList), m_vmlDrawing(std::make_unique<XLVmlDrawing>(*other.m_vmlDrawing)),
	// m_vmlDrawing(std::make_unique<XLVmlDrawing>(other.m_vmlDrawing ? *other.m_vmlDrawing : XLVmlDrawing())) // this can be used if other.m_vmlDrawing can be uninitialized
	m_hintNode(other.m_hintNode), m_hintIndex(other.m_hintIndex)
{
}

XLComments::XLComments(XLComments&& other) noexcept : XLXmlFile(other), m_authors(std::move(other.m_authors)),
	m_commentList(std::move(other.m_commentList)), m_vmlDrawing(std::move(other.m_vmlDrawing)), m_hintNode(other.m_hintNode),
	m_hintIndex(other.m_hintIndex)
{
}

XLComments& XLComments::operator = (XLComments&& other) noexcept
{
	if(&other != this) {
		XLXmlFile::operator=(std::move(other));
		m_authors          = std::move(other.m_authors);
		m_commentList      = std::move(other.m_commentList);
		m_vmlDrawing       = std::move(other.m_vmlDrawing);
		m_hintNode         = std::move(other.m_hintNode);
		m_hintIndex        = other.m_hintIndex;
	}
	return *this;
}

XLComments& XLComments::operator=(const XLComments& other)
{
	if(&other != this) {
		XLComments temp = other; // copy-construct
		*this = std::move(temp); // move-assign & invalidate temp
	}
	return *this;
}

bool XLComments::setVmlDrawing(XLVmlDrawing &vmlDrawing)
{
	m_vmlDrawing = std::make_unique<XLVmlDrawing>(vmlDrawing);
	return true;
}

XMLNode XLComments::authorNode(uint16 index) const
{
	XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
	uint16 i = 0;
	while(!auth.empty() && i != index) {
		++i;
		auth = auth.next_sibling_of_type(pugi::node_element);
	}
	return auth; // auth.empty() will be true if not found
}
/**
 * @brief find a comment XML node by index (sequence within source XML)
 * @param index the position (0-based) of the comment node to return
 * @throws XLException if index is out of bounds vs. XLComments::count()
 */
XMLNode XLComments::commentNode(size_t index) const
{
	if(m_hintNode.empty() || m_hintIndex > index) { // check if m_hintNode can be used - otherwise initialize it
		m_hintNode = m_commentList.first_child_of_type(pugi::node_element);
		m_hintIndex = 0;
	}
	while(!m_hintNode.empty() && m_hintIndex < index) {
		++m_hintIndex;
		m_hintNode = m_hintNode.next_sibling_of_type(pugi::node_element);
	}
	if(m_hintNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLComments::commentNode: index "s + std::to_string(index) + " is out of bounds"s);
	}
	return m_hintNode; // can be empty XMLNode if index is >= count
}

XMLNode XLComments::commentNode(const std::string& cellRef) const
{
	return m_commentList.find_child_by_attribute("comment", "ref", cellRef.c_str());
}

uint16 XLComments::authorCount() const
{
	XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
	uint16 count = 0;
	while(!auth.empty()) {
		++count;
		auth = auth.next_sibling_of_type(pugi::node_element);
	}
	return count;
}

std::string XLComments::author(uint16 index) const
{
	XMLNode auth = authorNode(index);
	if(auth.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLComments::author: index "s + std::to_string(index) + " is out of bounds"s);
	}
	return auth.first_child().value(); // author name is stored as a node_pcdata within the author node
}

bool XLComments::deleteAuthor(uint16 index)
{
	XMLNode auth = authorNode(index);
	if(auth.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLComments::deleteAuthor: index "s + std::to_string(index) + " is out of bounds"s);
	}
	else {
		while(auth.previous_sibling().type() == pugi::node_pcdata) // remove leading whitespaces
			m_authors.remove_child(auth.previous_sibling());
		m_authors.remove_child(auth);                     // then remove author node itself
	}
	return true;
}

/**
 * @details insert author and return index
 */
uint16 XLComments::addAuthor(const std::string& authorName)
{
	XMLNode auth = m_authors.first_child_of_type(pugi::node_element);
	uint16 index = 0;
	while(!auth.next_sibling_of_type(pugi::node_element).empty()) {
		++index;
		auth = auth.next_sibling_of_type(pugi::node_element);
	}
	if(auth.empty()) { // if this is the first entry
		auth = m_authors.prepend_child("author"); // insert new node
		m_authors.prepend_child(pugi::node_pcdata).set_value("\n\t\t"); // prefix first author with second level indentation
	}
	else { // found the last author node at index
		auth = m_authors.insert_child_after("author", auth);       // append a new author
		copyLeadingWhitespaces(m_authors, auth.previous_sibling(), auth); // copy whitespaces prefix from previous author
		++index;                                                   // increment index
	}
	auth.prepend_child(pugi::node_pcdata).set_value(authorName.c_str());
	return index;
}

size_t XLComments::count() const
{
	XMLNode comment = m_commentList.first_child_of_type(pugi::node_element);
	size_t count = 0;
	while(!comment.empty()) {
		// if (comment.name() == "comment") // TBD: safe-guard against potential rogue node
		++count;
		comment = comment.next_sibling_of_type(pugi::node_element);
	}
	return count;
}

uint16 XLComments::authorId(const std::string& cellRef) const
{
	XMLNode comment = commentNode(cellRef);
	return static_cast<uint16>(comment.attribute("authorId").as_uint());
}

bool XLComments::deleteComment(const std::string& cellRef)
{
	XMLNode comment = commentNode(cellRef);
	if(comment.empty())  
		return false;
	else {
		m_commentList.remove_child(comment);
		m_hintNode = XMLNode{}; // reset hint after modification of comment list
		m_hintIndex = 0;
	}
	// ===== Delete the shape associated with the comment.
	OpenXLSX::ignore(m_vmlDrawing->deleteShape(cellRef)); // disregard if deleteShape fails
	return true;
}

// comment entries:
//  attribute ref -> cell
//  attribute authorId -> author index in array
//  node text
//     subnode t -> regular text
//        attribute xml:space="preserve" - seems useful to always apply
//        subnode pc_data -> the comment text
//     subnode r -> rich text, repetition of:
//        subnode rPr -> rich text formatting
//           subnode sz -> font size (int)
//           subnode rFont -> font name (string)
//           subnode family -> TBC: font family (int)
//        subnode t -> regular text like above

XLComment XLComments::get(size_t index) const { return XLComment(commentNode(index)); }
std::string XLComments::get(const std::string& cellRef) const { return getCommentString(commentNode(cellRef)); }

bool XLComments::set(std::string const& cellRef, std::string const& commentText, uint16 authorId_)
{
	XLCellReference destRef(cellRef);
	uint32 destRow = destRef.row();
	uint16 destCol = destRef.column();
	bool newCommentCreated = false; // if false, try to find an existing shape before creating one
	using namespace std::literals::string_literals;
	XMLNode comment = m_commentList.first_child_of_type(pugi::node_element);
	while(!comment.empty()) {
		if(comment.name() == "comment"s) { // safeguard against rogue nodes
			XLCellReference ref(comment.attribute("ref").value());
			if(ref.row() > destRow ||(ref.row() == destRow && ref.column() >= destCol)) // abort when node or a node behind it is found
				break;
		}
		comment = comment.next_sibling_of_type(pugi::node_element);
	}
	if(comment.empty()) {                                                 // no comments yet or this will be the last comment
		comment = m_commentList.last_child_of_type(pugi::node_element);
		if(comment.empty()) {                                            // if this is the only comment so far
			comment = m_commentList.prepend_child("comment");                       // prepend new comment
			m_commentList.insert_child_before(pugi::node_pcdata, comment).set_value("\n\t\t"); // insert double indent before comment
		}
		else {
			comment = m_commentList.insert_child_after("comment", comment);         // insert new comment at end of list
			copyLeadingWhitespaces(m_commentList, comment.previous_sibling(), comment); // and copy whitespaces prefix from previous comment
		}
		newCommentCreated = true;
	}
	else {
		XLCellReference ref(comment.attribute("ref").value());
		if(ref.row() != destRow || ref.column() != destCol) {         // if node has to be inserted *before* this one
			comment = m_commentList.insert_child_before("comment", comment); // insert new comment
			copyLeadingWhitespaces(m_commentList, comment, comment.next_sibling()); // and copy whitespaces prefix from next node
			newCommentCreated = true;
		}
		else // node exists / was found
			comment.remove_children(); // clear node content
	}

	// ===== If the list of nodes was modified, re-set m_hintNode that is used to access nodes by index
	if(newCommentCreated) {
		m_hintNode = XMLNode{}; // reset hint after modification of comment list
		m_hintIndex = 0;
	}

	// now that we have a valid comment node: update attributes and content
	if(comment.attribute("ref").empty())                               // if ref has to be created
		comment.append_attribute("ref").set_value(destRef.address().c_str()); // then do so - otherwise it can remain untouched
	appendAndSetAttribute(comment, "authorId", std::to_string(authorId_)); // update authorId
	XMLNode tNode = comment.prepend_child("text").prepend_child("t");  // insert <text><t/></text> nodes
	tNode.append_attribute("xml:space").set_value("preserve");         // set <t> node attribute xml:space
	tNode.prepend_child(pugi::node_pcdata).set_value(commentText.c_str()); // finally, insert <t> node_pcdata value

	if(m_vmlDrawing->valid()) {
		XLShape cShape{};
		bool newShapeNeeded = newCommentCreated; // on new comments: create new shape
		if(!newCommentCreated) {
			try {
				cShape = shape(cellRef); // for existing comments, try to access existing shape
			}
			catch(XLException const &) {
				newShapeNeeded = true; // not found: create fresh
			}
		}
		if(newShapeNeeded)
			cShape = m_vmlDrawing->createShape();

		cShape.setFillColor("#ffffc0");
		cShape.setStroked(true);
		// setType: already done by XLVmlDrawing::createShape
		cShape.setAllowInCell(false);
		{
			// XLShapeStyle shapeStyle("position:absolute;margin-left:100pt;margin-top:0pt;width:50pt;height:50.0pt;mso-wrap-style:none;v-text-anchor:middle;visibility:hidden");
			XLShapeStyle shapeStyle{}; // default construct with meaningful values
			cShape.setStyle(shapeStyle);
		}
		XLShapeClientData clientData = cShape.clientData();
		clientData.setObjectType("Note");
		clientData.setMoveWithCells();
		clientData.setSizeWithCells();
		{
			constexpr const uint16 leftColOffset = 1;
			constexpr const uint16 widthCols = 2;
			constexpr const uint16 topRowOffset = 1;
			constexpr const uint16 heightRows = 2;
			uint16 anchorLeftCol;
			uint16 anchorRightCol;
			if(OpenXLSX::MAX_COLS - destCol > leftColOffset + widthCols) {
				anchorLeftCol  = (destCol - 1) + leftColOffset;
				anchorRightCol = (destCol - 1) + leftColOffset + widthCols;
			}
			else { // if anchor would overflow MAX_COLS: move column anchor to the left of destCol
				anchorLeftCol  = (destCol - 1) - leftColOffset - widthCols;
				anchorRightCol = (destCol - 1) - leftColOffset;
			}
			uint32 anchorTopRow;
			uint32 anchorBottomRow;
			if(OpenXLSX::MAX_ROWS - destRow > topRowOffset + heightRows) {
				anchorTopRow    = (destRow - 1) + topRowOffset;
				anchorBottomRow = (destRow - 1) + topRowOffset + heightRows;
			}
			else { // if anchor would overflow MAX_ROWS: move row anchor to the top of destCol
				anchorTopRow    = (destRow - 1) - topRowOffset - heightRows;
				anchorBottomRow = (destRow - 1) - topRowOffset;
			}
			if(anchorRightCol > MAX_SHAPE_ANCHOR_COLUMN)
				std::cout << "XLComments::set WARNING: anchoring comment shapes beyond column "s
					/**/ + XLCellReference::columnAsString(MAX_SHAPE_ANCHOR_COLUMN) + " may not get displayed correctly (LO Calc, TBD in Excel)"s << std::endl;
			if(anchorBottomRow > MAX_SHAPE_ANCHOR_ROW)
				std::cout << "XLComments::set WARNING: anchoring comment shapes beyond row "s
					/**/ + std::to_string(MAX_SHAPE_ANCHOR_ROW) + " may not get displayed correctly (LO Calc, TBD in Excel)"s << std::endl;
			uint16 anchorLeftOffsetInCell = 10;
			uint16 anchorRightOffsetInCell = 10;
			uint16 anchorTopOffsetInCell = 5;
			uint16 anchorBottomOffsetInCell = 5;
			// clientData.setAnchor("3, 23, 0, 0, 4, 25, 3, 5");
			using namespace std::literals::string_literals;
			clientData.setAnchor(
				std::to_string(anchorLeftCol)           + ","s
				+ std::to_string(anchorLeftOffsetInCell)  + ","s
				+ std::to_string(anchorTopRow)            + ","s
				+ std::to_string(anchorTopOffsetInCell)   + ","s
				+ std::to_string(anchorRightCol)          + ","s
				+ std::to_string(anchorRightOffsetInCell) + ","s
				+ std::to_string(anchorBottomRow)         + ","s
				+ std::to_string(anchorBottomOffsetInCell)
				);
		}
		clientData.setAutoFill(false);
		clientData.setTextVAlign(XLShapeTextVAlign::Top);
		clientData.setTextHAlign(XLShapeTextHAlign::Left);
		clientData.setRow(destRow - 1); // row and column are zero-indexed in XLShapeClientData
		clientData.setColumn(destCol - 1); // ..

		// 	<v:shadow on="t" obscured="t" color="black"/>
		// 	<v:fill o:detectmouseclick="t" type="solid" color2="#00003f"/>
		// 	<v:stroke color="#3465a4" startarrow="block" startarrowwidth="medium" startarrowlength="medium" joinstyle="round" endcap="flat"/>
		// 	<x:ClientData ObjectType="Note">
		// 		<x:MoveWithCells/>
		// 		<x:SizeWithCells/>
		// 		<x:Anchor>3, 23, 0, 0, 4, 25, 3, 5</x:Anchor>
		// 		<x:AutoFill>False</x:AutoFill>
		// 		<x:TextVAlign>Top</x:TextVAlign>
		// 		<x:TextHAlign>Left</x:TextHAlign>
		// 		<x:Row>0</x:Row>
		// 		<x:Column>2</x:Column>
		// 	</x:ClientData>
		// </v:shape>
	}
	else
		throw XLException("XLComments::set: can not set (format) any comments when VML Drawing object is invalid");

	return true;
}

XLShape XLComments::shape(std::string const& cellRef)
{
	if(!m_vmlDrawing->valid())
		throw XLException("XLComments::shape: can not access any shapes when VML Drawing object is invalid");
	XMLNode shape = m_vmlDrawing->shapeNode(cellRef);
	if(shape.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLComments::shape: not found for cell "s + cellRef + " - was XLComment::set invoked first?"s);
	}
	return XLShape(shape);
}

/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLComments::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }
//
// XLContentTypes.cpp
//
namespace { // anonymous namespace for local functions
	const std::string applicationOpenXmlOfficeDocument = "application/vnd.openxmlformats-officedocument";
	const std::string applicationOpenXmlPackage        = "application/vnd.openxmlformats-package";
	const std::string applicationMicrosoftExcel        = "application/vnd.ms-excel";
	const std::string applicationMicrosoftOffice       = "application/vnd.ms-office";
	/**
	 * @details
	 * @note 2024-08-31: In line with a change to hardcoded XML relationship domains in XLRelationships.cpp, replaced the repeating
	 *          hardcoded strings here with the above declared constants, preparing a potential need for a similar "dumb" fallback implementation
	 */
	XLContentType GetContentTypeFromString(const std::string& typeString)
	{
		XLContentType type { 
			XLContentType::Unknown 
		};
		if(typeString == applicationMicrosoftExcel + ".sheet.macroEnabled.main+xml")
			type = XLContentType::WorkbookMacroEnabled;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.sheet.main+xml")
			type = XLContentType::Workbook;
		else if(typeString == applicationOpenXmlPackage + ".relationships+xml")
			type = XLContentType::Relationships;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.worksheet+xml")
			type = XLContentType::Worksheet;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.chartsheet+xml")
			type = XLContentType::Chartsheet;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.externalLink+xml")
			type = XLContentType::ExternalLink;
		else if(typeString == applicationOpenXmlOfficeDocument + ".theme+xml")
			type = XLContentType::Theme;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.styles+xml")
			type = XLContentType::Styles;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.sharedStrings+xml")
			type = XLContentType::SharedStrings;
		else if(typeString == applicationOpenXmlOfficeDocument + ".drawing+xml")
			type = XLContentType::Drawing;
		else if(typeString == applicationOpenXmlOfficeDocument + ".drawingml.chart+xml")
			type = XLContentType::Chart;
		else if(typeString == applicationMicrosoftOffice + ".chartstyle+xml")
			type = XLContentType::ChartStyle;
		else if(typeString == applicationMicrosoftOffice + ".chartcolorstyle+xml")
			type = XLContentType::ChartColorStyle;
		else if(typeString == applicationMicrosoftExcel + ".controlproperties+xml")
			type = XLContentType::ControlProperties;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.calcChain+xml")
			type = XLContentType::CalculationChain;
		else if(typeString == applicationMicrosoftOffice + ".vbaProject")
			type = XLContentType::VBAProject;
		else if(typeString == applicationOpenXmlPackage + ".core-properties+xml")
			type = XLContentType::CoreProperties;
		else if(typeString == applicationOpenXmlOfficeDocument + ".extended-properties+xml")
			type = XLContentType::ExtendedProperties;
		else if(typeString == applicationOpenXmlOfficeDocument + ".custom-properties+xml")
			type = XLContentType::CustomProperties;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.comments+xml")
			type = XLContentType::Comments;
		else if(typeString == applicationOpenXmlOfficeDocument + ".spreadsheetml.table+xml")
			type = XLContentType::Table;
		else if(typeString == applicationOpenXmlOfficeDocument + ".vmlDrawing")
			type = XLContentType::VMLDrawing;
		else
			type = XLContentType::Unknown;
		return type;
	}

	std::string GetStringFromType(XLContentType type)
	{
		std::string typeString;
		if(type == XLContentType::WorkbookMacroEnabled)
			typeString = applicationMicrosoftExcel + ".sheet.macroEnabled.main+xml";
		else if(type == XLContentType::Workbook)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.sheet.main+xml";
		else if(type == XLContentType::Relationships)
			typeString = applicationOpenXmlPackage + ".relationships+xml";
		else if(type == XLContentType::Worksheet)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.worksheet+xml";
		else if(type == XLContentType::Chartsheet)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.chartsheet+xml";
		else if(type == XLContentType::ExternalLink)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.externalLink+xml";
		else if(type == XLContentType::Theme)
			typeString = applicationOpenXmlOfficeDocument + ".theme+xml";
		else if(type == XLContentType::Styles)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.styles+xml";
		else if(type == XLContentType::SharedStrings)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.sharedStrings+xml";
		else if(type == XLContentType::Drawing)
			typeString = applicationOpenXmlOfficeDocument + ".drawing+xml";
		else if(type == XLContentType::Chart)
			typeString = applicationOpenXmlOfficeDocument + ".drawingml.chart+xml";
		else if(type == XLContentType::ChartStyle)
			typeString = applicationMicrosoftOffice + ".chartstyle+xml";
		else if(type == XLContentType::ChartColorStyle)
			typeString = applicationMicrosoftOffice + ".chartcolorstyle+xml";
		else if(type == XLContentType::ControlProperties)
			typeString = applicationMicrosoftExcel + ".controlproperties+xml";
		else if(type == XLContentType::CalculationChain)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.calcChain+xml";
		else if(type == XLContentType::VBAProject)
			typeString = applicationMicrosoftOffice + ".vbaProject";
		else if(type == XLContentType::CoreProperties)
			typeString = applicationOpenXmlPackage + ".core-properties+xml";
		else if(type == XLContentType::ExtendedProperties)
			typeString = applicationOpenXmlOfficeDocument + ".extended-properties+xml";
		else if(type == XLContentType::CustomProperties)
			typeString = applicationOpenXmlOfficeDocument + ".custom-properties+xml";
		else if(type == XLContentType::Comments)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.comments+xml";
		else if(type == XLContentType::Table)
			typeString = applicationOpenXmlOfficeDocument + ".spreadsheetml.table+xml";
		else if(type == XLContentType::VMLDrawing)
			typeString = applicationOpenXmlOfficeDocument + ".vmlDrawing";
		else
			throw XLInternalError("Unknown ContentType");
		return typeString;
	}
}    // anonymous namespace

XLContentItem::XLContentItem() : m_contentNode(std::make_unique<XMLNode>()) 
{
}

XLContentItem::XLContentItem(const XMLNode& node) : m_contentNode(std::make_unique<XMLNode>(node)) 
{
}

XLContentItem::XLContentItem(const XLContentItem& other) : m_contentNode(std::make_unique<XMLNode>(*other.m_contentNode)) 
{
}

XLContentItem::XLContentItem(XLContentItem&& other) noexcept = default;
XLContentItem::~XLContentItem() = default;

XLContentItem& XLContentItem::operator=(const XLContentItem& other)
{
	if(&other != this)
		*m_contentNode = *other.m_contentNode;
	return *this;
}

XLContentItem& XLContentItem::operator=(XLContentItem&& other) noexcept = default;

XLContentType XLContentItem::type() const { return GetContentTypeFromString(m_contentNode->attribute("ContentType").value()); }
std::string XLContentItem::path() const { return m_contentNode->attribute("PartName").value(); }
XLContentTypes::XLContentTypes() = default;
XLContentTypes::XLContentTypes(XLXmlData* xmlData) : XLXmlFile(xmlData) {}
XLContentTypes::~XLContentTypes() = default;
XLContentTypes::XLContentTypes(const XLContentTypes& other) = default;
XLContentTypes::XLContentTypes(XLContentTypes&& other) noexcept = default;
XLContentTypes& XLContentTypes::operator=(const XLContentTypes& other) = default;
XLContentTypes& XLContentTypes::operator=(XLContentTypes&& other) noexcept = default;
/**
 * @details
 * @note 2024-07-22: added more intelligent whitespace support
 */
void XLContentTypes::addOverride(const std::string& path, XLContentType type)
{
	const std::string typeString = GetStringFromType(type);
	XMLNode lastOverride = xmlDocument().document_element().last_child_of_type(pugi::node_element); // see if there's a last element
	XMLNode node{}; // scope declaration
	// Create new node in the [Content_Types].xml file
	if(lastOverride.empty())
		node = xmlDocument().document_element().prepend_child("Override");
	else { // if last element found
		// ===== Insert node after previous override
		node = xmlDocument().document_element().insert_child_after("Override", lastOverride);
		// ===== Using whitespace nodes prior to lastOverride as a template, insert whitespaces between lastOverride and the new node
		XMLNode copyWhitespaceFrom = lastOverride; // start looking for whitespace nodes before previous override
		XMLNode insertBefore = node;          // start inserting the same whitespace nodes before new override
		while(copyWhitespaceFrom.previous_sibling().type() == pugi::node_pcdata) { // empty node returns pugi::node_null
			// Advance to previous "template" whitespace node, ensured to exist in while-condition
			copyWhitespaceFrom = copyWhitespaceFrom.previous_sibling();
			// ===== Insert a whitespace node
			insertBefore = xmlDocument().document_element().insert_child_before(pugi::node_pcdata, insertBefore);
			insertBefore.set_value(copyWhitespaceFrom.value()); // copy the a whitespace in sequence node value
		}
	}
	node.append_attribute("PartName").set_value(path.c_str());
	node.append_attribute("ContentType").set_value(typeString.c_str());
}

void XLContentTypes::deleteOverride(const std::string& path)
{
	xmlDocument().document_element().remove_child(xmlDocument().document_element().find_child_by_attribute("PartName", path.c_str()));
}

void XLContentTypes::deleteOverride(const XLContentItem& item) { deleteOverride(item.path()); }

XLContentItem XLContentTypes::contentItem(const std::string& path)
{
	return XLContentItem(xmlDocument().document_element().find_child_by_attribute("PartName", path.c_str()));
}

std::vector<XLContentItem> XLContentTypes::getContentItems()
{
	std::vector<XLContentItem> result;
	XMLNode item = xmlDocument().document_element().first_child_of_type(pugi::node_element);
	while(!item.empty()) {
		if(sstreq(item.name(), "Override"))
			result.emplace_back(item);
		item = item.next_sibling_of_type(pugi::node_element);
	}
	return result;
}
//
// XLDateTime.cpp
//
namespace {
	bool isLeapYear(int year)
	{
		if(year == 1900)  
			return true; // historical Excel Date error inherited from older spreadsheet apps
		if(year % 400 == 0 || (year % 4 == 0 && year % 100 != 0))  
			return true;
		return false;
	}
	int daysInMonth(int month, int year)
	{
		switch(month) {
			case 1: return 31;
			case 2: return (isLeapYear(year) ? 29 : 28);
			case 3: return 31;
			case 4: return 30;
			case 5: return 31;
			case 6: return 30;
			case 7: return 31;
			case 8: return 31;
			case 9: return 30;
			case 10: return 31;
			case 11: return 30;
			case 12: return 31;
			default: return 0;
		}
	}

	int dayOfWeek(double serial)
	{
		const auto day = static_cast<int32_t>(serial) % 7;
		return (day == 0 ? 6 : day - 1);
	}
}

namespace OpenXLSX {
	XLDateTime::XLDateTime() = default;
	/**
	 * @details Constructor taking an Excel date/time serial number as an argument.
	 */
	XLDateTime::XLDateTime(double serial) : m_serial(serial)
	{
		if(serial < 1.0)  
			throw XLDateTimeError("Excel date/time serial number is invalid (must be >= 1.0.)"); // don't permit dates before 1900-01-01T00:00:00.000
	}
	/**
	 * @details Constructor taking a std::tm object as an argument.
	 */
	XLDateTime::XLDateTime(const std::tm& timepoint)
	{
		// ===== Check validity of tm struct.
		// ===== Only year, month and day of the month are checked. Other variables are ignored.
		if(timepoint.tm_year < 0)
			throw XLDateTimeError("Invalid year. Must be >= 0.");
		if(timepoint.tm_mon < 0 || timepoint.tm_mon > 11)
			throw XLDateTimeError("Invalid month. Must be >= 0 or <= 11.");
		if(timepoint.tm_mday <= 0 || timepoint.tm_mday > daysInMonth(timepoint.tm_mon + 1, timepoint.tm_year + 1900))
			throw XLDateTimeError("Invalid day. Must be >= 1 or <= total days in the month.");
		// ===== Count the number of days for full years past 1900
		for(int i = 0; i < timepoint.tm_year; ++i) {
			m_serial += (isLeapYear(1900 + i) ? 366 : 365);
		}
		// ===== Count the number of days for full months of the last year
		for(int i = 0; i < timepoint.tm_mon; ++i) {
			m_serial += daysInMonth(i + 1, timepoint.tm_year + 1900);
		}
		// ===== Add the number of days of the month, minus one.
		// ===== (The reason for the 'minus one' is that unlike the other fields in the struct,
		// ===== tm_day represents the date of a month, whereas the other fields typically
		// ===== represents the number of whole units since the start).
		m_serial += timepoint.tm_mday - 1;
		// ===== Convert hour, minute and second to fraction of a full day.
		const int32_t seconds = timepoint.tm_hour * 3600 + timepoint.tm_min * 60 + timepoint.tm_sec;
		m_serial += seconds / 86400.0;
	}
	/**
	 * @details Constructor taking a unixtime format (seconds since 1/1/1970) as an argument.
	 */
	XLDateTime::XLDateTime(time_t unixtime)
	{
		// There are 86400 seconds in a day
		// There are 25569 days between 1/1/1970 and 30/12/1899 (the epoch used by Excel)
		m_serial = ( static_cast<double>(unixtime) / 86400 ) + 25569;
	}

	XLDateTime::XLDateTime(const XLDateTime& other) = default;
	XLDateTime::XLDateTime(XLDateTime&& other) noexcept = default;
	XLDateTime::~XLDateTime() = default;
	XLDateTime& XLDateTime::operator=(const XLDateTime& other) = default;
	XLDateTime& XLDateTime::operator=(XLDateTime&& other) noexcept = default;

	XLDateTime& XLDateTime::operator=(double serial)
	{
		XLDateTime temp(serial);
		std::swap(*this, temp);
		return *this;
	}

	XLDateTime& XLDateTime::operator=(const std::tm& timepoint)
	{
		XLDateTime temp(timepoint);
		std::swap(*this, temp);
		return *this;
	}

	XLDateTime::operator std::tm() const { return tm(); }
	/**
	 * @details Get the time point as an Excel date/time serial number.
	 */
	double XLDateTime::serial() const { return m_serial; }
	/**
	 * @details Get the time point as a std::tm object.
	 */
	std::tm XLDateTime::tm() const
	{
		// ===== Create and initialize the resulting object.
		std::tm result {};
		result.tm_year  = 0;
		result.tm_mon   = 0;
		result.tm_mday  = 0;
		result.tm_wday  = 0;
		result.tm_yday  = 0;
		result.tm_hour  = 0;
		result.tm_min   = 0;
		result.tm_sec   = 0;
		result.tm_isdst = -1;
		double serial   = m_serial;

		// ===== Count the number of whole years since 1900.
		while(serial > 1) {   // 2025-01-10: safeguard against infinite loop (preemptive)
			const int days = (isLeapYear(result.tm_year + 1900) ? 366 : 365);
			if(days + 1 > serial)
				break;// 2025-01-10: BUGFIX: break on days + 1 > serial (was days >= serial) to account for fractions
			serial -= days;
			++result.tm_year;
		}
		// ===== Calculate the day of the year, and the day of the week
		result.tm_yday = static_cast<int>(serial) - 1;
		result.tm_wday = dayOfWeek(m_serial);
		// ===== Count the number of whole months in the year.
		while(result.tm_mon < 11) {    // 2025-01-10 BUGFIX: break on December to prevent infinite loop after adjusted bugfix below
			int days = daysInMonth(result.tm_mon + 1, 1900 + result.tm_year);
			if(days + 1 > serial)
				break;// 2025-01-10: BUGFIX: break on days + 1 > serial (was days >= serial) to account for fractions
			serial -= days;
			++result.tm_mon;
		}
		// ===== Calculate the number of days.
		result.tm_mday = static_cast<int>(serial);
		serial -= result.tm_mday;
		// ===== Calculate the number of hours.
		result.tm_hour = static_cast<int>(serial * 24);
		serial -= (result.tm_hour / 24.0);
		// ===== Calculate the number of minutes.
		result.tm_min = static_cast<int>(serial * 24 * 60);
		serial -= (result.tm_min / (24.0 * 60.0));
		// ===== Calculate the number of seconds.
		result.tm_sec = static_cast<int>(lround(serial * 24 * 60 * 60));
		// ===== BEGIN: pass rounded overflow back up the date components. BUGFIX 2025-01-10: added this overflow handling to address issue #138
		if(result.tm_sec >= 60) {
			result.tm_sec -= 60;
			++result.tm_min;
			if(result.tm_min >= 60) {
				result.tm_min -= 60;
				++result.tm_hour;
				if(result.tm_hour >= 24) {
					result.tm_hour -= 24;
					++result.tm_mday;
					int days = daysInMonth(result.tm_mon + 1, 1900 + result.tm_year);
					if(result.tm_mday >= days) {
						result.tm_mday -= days;
						++result.tm_mon;
						if(result.tm_mon >= 12) {
							result.tm_mon -= 12;
							++result.tm_year;
						}
					}
				}
			}
		} // END: pass rounded overflow back up the date components
		return result;
	}
} // namespace OpenXLSX
//
// XLDocument.cpp
//
namespace {
	constexpr int templateSize       = 7714;
	constexpr uchar templateData[7714] = {
		0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xb5, 0x55, 0x30, 0x23, 0xf4, 0x00, 0x00, 0x00,
		0x4c, 0x02, 0x00, 0x00, 0x0b, 0x00, 0x08, 0x02, 0x5f, 0x72, 0x65, 0x6c, 0x73, 0x2f, 0x2e, 0x72, 0x65, 0x6c, 0x73, 0x20, 0xa2, 0x04,
		0x02, 0x28, 0xa0, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xac, 0x92, 0x4d, 0x4f, 0xc3, 0x30, 0x0c, 0x86, 0xef, 0x48, 0xfc,
		0x87, 0xc8, 0xf7, 0xd5, 0xdd, 0x90, 0x10, 0x42, 0x4b, 0x77, 0x41, 0x48, 0xbb, 0x21, 0x54, 0x7e, 0x80, 0x49, 0xdc, 0x0f, 0xb5, 0x8d,
		0xa3, 0x24, 0x1b, 0xdd, 0xbf, 0x27, 0x1c, 0x10, 0x54, 0x1a, 0x83, 0x03, 0x47, 0x7f, 0xbd, 0x7e, 0xfc, 0xca, 0xdb, 0xdd, 0x3c, 0x8d,
		0xea, 0xc8, 0x21, 0xf6, 0xe2, 0x34, 0xac, 0x8b, 0x12, 0x14, 0x3b, 0x23, 0xb6, 0x77, 0xad, 0x86, 0x97, 0xfa, 0x71, 0x75, 0x07, 0x2a,
		0x26, 0x72, 0x96, 0x46, 0x71, 0xac, 0xe1, 0xc4, 0x11, 0x76, 0xd5, 0xf5, 0xd5, 0xf6, 0x99, 0x47, 0x4a, 0x79, 0x28, 0x76, 0xbd, 0x8f,
		0x2a, 0xab, 0xb8, 0xa8, 0xa1, 0x4b, 0xc9, 0xdf, 0x23, 0x46, 0xd3, 0xf1, 0x44, 0xb1, 0x10, 0xcf, 0x2e, 0x57, 0x1a, 0x09, 0x13, 0xa5,
		0x1c, 0x86, 0x16, 0x3d, 0x99, 0x81, 0x5a, 0xc6, 0x4d, 0x59, 0xde, 0x62, 0xf8, 0xae, 0x01, 0xd5, 0x42, 0x53, 0xed, 0xad, 0x86, 0xb0,
		0xb7, 0x37, 0xa0, 0xea, 0x93, 0xcf, 0x9b, 0x7f, 0xd7, 0x96, 0xa6, 0xe9, 0x0d, 0x3f, 0x88, 0x39, 0x4c, 0xec, 0xd2, 0x99, 0x15, 0xc8,
		0x73, 0x62, 0x67, 0xd9, 0xae, 0x7c, 0xc8, 0x6c, 0x21, 0xf5, 0xf9, 0x1a, 0x55, 0x53, 0x68, 0x39, 0x69, 0xb0, 0x62, 0x9e, 0x72, 0x3a,
		0x22, 0x79, 0x5f, 0x64, 0x6c, 0xc0, 0xf3, 0x44, 0x9b, 0xbf, 0x13, 0xfd, 0x7c, 0x2d, 0x4e, 0x9c, 0xc8, 0x52, 0x22, 0x34, 0x12, 0xf8,
		0x32, 0xcf, 0x47, 0xc7, 0x25, 0xa0, 0xf5, 0x7f, 0x5a, 0xb4, 0x34, 0xf1, 0xcb, 0x9d, 0x79, 0xc4, 0x37, 0x09, 0xc3, 0xab, 0xc8, 0xf0,
		0xc9, 0x82, 0x8b, 0x1f, 0xa8, 0xde, 0x01, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00, 0x08,
		0x00, 0x00, 0x00, 0x21, 0x00, 0x47, 0x88, 0xbc, 0xe2, 0x5d, 0x03, 0x00, 0x00, 0x35, 0x08, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x78,
		0x6c, 0x2f, 0x77, 0x6f, 0x72, 0x6b, 0x62, 0x6f, 0x6f, 0x6b, 0x2e, 0x78, 0x6d, 0x6c, 0xac, 0x55, 0x6d, 0x6f, 0xa3, 0x38, 0x10, 0xfe,
		0x7e, 0xd2, 0xfd, 0x07, 0xc4, 0x77, 0x8a, 0x4d, 0xcc, 0x4b, 0x50, 0xe9, 0x2a, 0x90, 0xa0, 0xab, 0xb4, 0x5d, 0x55, 0x6d, 0xb6, 0xfb,
		0xf1, 0xe4, 0x80, 0x29, 0x56, 0x01, 0x73, 0xc6, 0x34, 0xa9, 0xaa, 0xfd, 0xef, 0x3b, 0x76, 0x42, 0xda, 0x6e, 0x57, 0xa7, 0x5c, 0xf7,
		0xaa, 0xd4, 0xc6, 0x9e, 0xe1, 0xf1, 0x33, 0x33, 0xcf, 0x98, 0xf3, 0x4f, 0xbb, 0xb6, 0xb1, 0x1e, 0x99, 0x1c, 0xb8, 0xe8, 0x12, 0x1b,
		0x9f, 0x21, 0xdb, 0x62, 0x5d, 0x21, 0x4a, 0xde, 0xdd, 0x27, 0xf6, 0xd7, 0x75, 0xee, 0x44, 0xb6, 0x35, 0x28, 0xda, 0x95, 0xb4, 0x11,
		0x1d, 0x4b, 0xec, 0x27, 0x36, 0xd8, 0x9f, 0x2e, 0xfe, 0xfc, 0xe3, 0x7c, 0x2b, 0xe4, 0xc3, 0x46, 0x88, 0x07, 0x0b, 0x00, 0xba, 0x21,
		0xb1, 0x6b, 0xa5, 0xfa, 0xd8, 0x75, 0x87, 0xa2, 0x66, 0x2d, 0x1d, 0xce, 0x44, 0xcf, 0x3a, 0xb0, 0x54, 0x42, 0xb6, 0x54, 0xc1, 0x52,
		0xde, 0xbb, 0x43, 0x2f, 0x19, 0x2d, 0x87, 0x9a, 0x31, 0xd5, 0x36, 0xae, 0x87, 0x50, 0xe0, 0xb6, 0x94, 0x77, 0xf6, 0x1e, 0x21, 0x96,
		0xa7, 0x60, 0x88, 0xaa, 0xe2, 0x05, 0x5b, 0x8a, 0x62, 0x6c, 0x59, 0xa7, 0xf6, 0x20, 0x92, 0x35, 0x54, 0x01, 0xfd, 0xa1, 0xe6, 0xfd,
		0x30, 0xa1, 0xb5, 0xc5, 0x29, 0x70, 0x2d, 0x95, 0x0f, 0x63, 0xef, 0x14, 0xa2, 0xed, 0x01, 0x62, 0xc3, 0x1b, 0xae, 0x9e, 0x0c, 0xa8,
		0x6d, 0xb5, 0x45, 0x7c, 0x79, 0xdf, 0x09, 0x49, 0x37, 0x0d, 0x84, 0xbd, 0xc3, 0xbe, 0xb5, 0x93, 0xf0, 0x0b, 0xe0, 0x1f, 0x23, 0x18,
		0xbc, 0xe9, 0x24, 0x30, 0xbd, 0x3b, 0xaa, 0xe5, 0x85, 0x14, 0x83, 0xa8, 0xd4, 0x19, 0x40, 0xbb, 0x7b, 0xd2, 0xef, 0xe2, 0xc7, 0xc8,
		0xc5, 0xf8, 0x4d, 0x0a, 0x76, 0xef, 0x73, 0x70, 0x1a, 0x12, 0x71, 0x25, 0x7b, 0xe4, 0xba, 0x86, 0x47, 0x56, 0x32, 0xf8, 0x20, 0xab,
		0xe0, 0x88, 0x15, 0xbc, 0x80, 0x61, 0xf4, 0xdb, 0x68, 0x18, 0xa4, 0x65, 0xb4, 0x12, 0x43, 0xf2, 0x3e, 0x88, 0xe6, 0x1f, 0xb9, 0x79,
		0xf6, 0xc5, 0x79, 0xc5, 0x1b, 0x76, 0xb7, 0x97, 0xae, 0x45, 0xfb, 0xfe, 0x0b, 0x6d, 0x75, 0xa5, 0x1a, 0xdb, 0x6a, 0xe8, 0xa0, 0x56,
		0x25, 0x57, 0xac, 0x4c, 0xec, 0x10, 0x96, 0x62, 0xcb, 0xde, 0x6c, 0xc8, 0xb1, 0x4f, 0x47, 0xde, 0x80, 0x15, 0xa3, 0x08, 0x7b, 0xb6,
		0x7b, 0x71, 0x94, 0xf3, 0xb5, 0xb4, 0x4a, 0x56, 0xd1, 0xb1, 0x51, 0x6b, 0x10, 0xf2, 0x04, 0x0f, 0x8e, 0x41, 0x30, 0xf7, 0x7c, 0xed,
		0x09, 0xc2, 0x58, 0x34, 0x8a, 0xc9, 0x8e, 0x2a, 0x96, 0x89, 0x4e, 0x81, 0x0e, 0x0f, 0x71, 0xfd, 0xae, 0xe6, 0x0c, 0x76, 0x56, 0x0b,
		0x50, 0xb8, 0x75, 0xc3, 0xfe, 0x19, 0xb9, 0x64, 0xd0, 0x58, 0xa0, 0x2f, 0x88, 0x15, 0x46, 0x5a, 0xc4, 0x74, 0x33, 0x5c, 0x53, 0x55,
		0x5b, 0xa3, 0x6c, 0x12, 0xdb, 0xfd, 0x3a, 0x40, 0xf0, 0xee, 0x5a, 0x8a, 0x06, 0xba, 0xd3, 0x5d, 0xb2, 0x47, 0xd6, 0x88, 0xde, 0xf4,
		0xc5, 0x86, 0x77, 0x5e, 0xe1, 0xbe, 0x52, 0x28, 0x7d, 0xdf, 0x0e, 0xff, 0x41, 0xa3, 0xb4, 0xd0, 0x81, 0xbb, 0x10, 0xf9, 0x9e, 0xdd,
		0xfe, 0xf9, 0xe7, 0x2c, 0x00, 0x49, 0x19, 0x4f, 0x3a, 0xbc, 0x56, 0xd2, 0x82, 0xe7, 0xcb, 0xe5, 0x67, 0xa8, 0xc5, 0x2d, 0x7d, 0x84,
		0xca, 0x40, 0xfd, 0xcb, 0x43, 0xe3, 0x5e, 0x42, 0xea, 0xa3, 0xbf, 0x9f, 0x17, 0xd9, 0x6c, 0xb6, 0x08, 0xf2, 0xd4, 0xc1, 0x01, 0x9a,
		0x39, 0x5e, 0x40, 0xb0, 0xb3, 0x20, 0x28, 0x77, 0xd2, 0x30, 0x0b, 0x73, 0x2f, 0x45, 0xab, 0x60, 0x1e, 0x7d, 0x87, 0x28, 0x64, 0x10,
		0x17, 0x82, 0x8e, 0xaa, 0x3e, 0x54, 0x5b, 0x63, 0x26, 0x36, 0x21, 0xbf, 0x30, 0x5d, 0xd1, 0xdd, 0x64, 0xc1, 0x28, 0x1e, 0x79, 0xf9,
		0x72, 0xfe, 0x33, 0x3a, 0xfc, 0x39, 0x7a, 0xfe, 0x69, 0x98, 0x6c, 0xdf, 0x75, 0xa4, 0xfa, 0x5e, 0xbb, 0xe3, 0x6c, 0x3b, 0xbc, 0xe8,
		0x42, 0x2f, 0xad, 0xdd, 0x37, 0xde, 0x95, 0x62, 0x9b, 0xd8, 0x5e, 0x18, 0x41, 0x34, 0x4f, 0xd3, 0x12, 0xfb, 0x01, 0x2c, 0xb7, 0xc6,
		0xf8, 0x8d, 0x97, 0xaa, 0x06, 0x8f, 0x08, 0x91, 0xe3, 0xde, 0x5f, 0x8c, 0xdf, 0xd7, 0xc0, 0x18, 0x87, 0x44, 0x6f, 0x82, 0xfe, 0x35,
		0xb3, 0xc4, 0x7e, 0xce, 0x73, 0x32, 0x4f, 0x23, 0xbc, 0x70, 0xb0, 0x17, 0x2e, 0x9c, 0x94, 0x90, 0x0c, 0x12, 0x40, 0x52, 0x27, 0xcf,
		0x51, 0x3e, 0x5b, 0xe5, 0x51, 0x9e, 0xe5, 0x73, 0xc3, 0xc8, 0x7d, 0x45, 0xc9, 0xdc, 0xa0, 0x40, 0xcd, 0xcc, 0x56, 0x67, 0x54, 0x7f,
		0xab, 0x6f, 0x55, 0x0c, 0x57, 0xb5, 0x9e, 0x75, 0x76, 0xe1, 0x59, 0xc6, 0xfa, 0x0c, 0x79, 0x59, 0x62, 0x53, 0xbd, 0xe9, 0xb5, 0x82,
		0x36, 0x05, 0xa8, 0x5c, 0x4f, 0xc6, 0x31, 0xc2, 0xc8, 0x9b, 0x6b, 0x0f, 0xb6, 0x53, 0x9f, 0x07, 0x65, 0x66, 0x10, 0x18, 0x07, 0x7a,
		0x98, 0xa0, 0x45, 0x88, 0xe6, 0xc4, 0x41, 0xab, 0x99, 0xef, 0x90, 0x68, 0xee, 0x39, 0x11, 0x99, 0x79, 0x4e, 0x46, 0x96, 0xde, 0xca,
		0x0f, 0x57, 0xcb, 0x55, 0xea, 0xeb, 0xfa, 0xe8, 0x2f, 0x40, 0xfc, 0x7f, 0xdc, 0x83, 0x46, 0xe7, 0xf1, 0xf4, 0x69, 0xd1, 0x2c, 0x6b,
		0x2a, 0xd5, 0x5a, 0xd2, 0xe2, 0x01, 0x3e, 0x48, 0x37, 0xac, 0x4a, 0xe9, 0x00, 0x4a, 0xda, 0x07, 0x04, 0x7c, 0x5f, 0x93, 0x4d, 0xfd,
		0x28, 0x45, 0x33, 0xa0, 0x48, 0x72, 0x9c, 0x3b, 0x04, 0xcf, 0x91, 0x93, 0xa6, 0x01, 0x71, 0xfc, 0x65, 0x3e, 0xf3, 0x43, 0xbc, 0xcc,
		0x56, 0x7e, 0xfe, 0x42, 0x56, 0x87, 0x5f, 0x7d, 0xf0, 0x16, 0x8a, 0x5c, 0xf3, 0x36, 0xa3, 0x6a, 0x84, 0x0e, 0xd5, 0xcd, 0x69, 0xd6,
		0xb1, 0x1e, 0xf3, 0xc3, 0xee, 0x71, 0xb3, 0xda, 0x6f, 0x1c, 0xea, 0xf4, 0xa6, 0xe9, 0xe2, 0x9b, 0xa5, 0xce, 0xfb, 0xe1, 0xed, 0x7f,
		0x73, 0xbc, 0x85, 0xe8, 0x1b, 0x76, 0xa2, 0x73, 0x7e, 0x77, 0xa2, 0x63, 0xf6, 0xe5, 0x6a, 0x7d, 0x65, 0xb4, 0xf1, 0xcb, 0x00, 0x5c,
		0x93, 0x60, 0x3d, 0x1a, 0x59, 0xb8, 0x53, 0x59, 0x2e, 0x7e, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14,
		0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xf0, 0x08, 0x58, 0xf4, 0xa5, 0x02, 0x00, 0x00, 0x52, 0x06, 0x00, 0x00, 0x0d,
		0x00, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x73, 0x2e, 0x78, 0x6d, 0x6c, 0xa4, 0x55, 0x6d, 0x6b, 0xdb, 0x30,
		0x10, 0xfe, 0x3e, 0xd8, 0x7f, 0x10, 0xfa, 0xee, 0xca, 0x76, 0xe3, 0x2c, 0x09, 0xb6, 0xcb, 0xd2, 0xd4, 0x50, 0xe8, 0xc6, 0xa0, 0x1d,
		0xec, 0xab, 0x62, 0xcb, 0x89, 0xa8, 0x5e, 0x8c, 0x24, 0x67, 0xce, 0xc6, 0xfe, 0xfb, 0x4e, 0x76, 0x5e, 0x1c, 0x3a, 0xb6, 0xd1, 0x7e,
		0x89, 0x4e, 0xe7, 0xd3, 0x73, 0xcf, 0xdd, 0x73, 0x52, 0xd2, 0x9b, 0x4e, 0x0a, 0xb4, 0x63, 0xc6, 0x72, 0xad, 0x32, 0x1c, 0x5d, 0x85,
		0x18, 0x31, 0x55, 0xea, 0x8a, 0xab, 0x4d, 0x86, 0xbf, 0x3e, 0x15, 0xc1, 0x0c, 0x23, 0xeb, 0xa8, 0xaa, 0xa8, 0xd0, 0x8a, 0x65, 0x78,
		0xcf, 0x2c, 0xbe, 0xc9, 0xdf, 0xbf, 0x4b, 0xad, 0xdb, 0x0b, 0xf6, 0xb8, 0x65, 0xcc, 0x21, 0x80, 0x50, 0x36, 0xc3, 0x5b, 0xe7, 0x9a,
		0x05, 0x21, 0xb6, 0xdc, 0x32, 0x49, 0xed, 0x95, 0x6e, 0x98, 0x82, 0x2f, 0xb5, 0x36, 0x92, 0x3a, 0xd8, 0x9a, 0x0d, 0xb1, 0x8d, 0x61,
		0xb4, 0xb2, 0xfe, 0x90, 0x14, 0x24, 0x0e, 0xc3, 0x29, 0x91, 0x94, 0x2b, 0x3c, 0x20, 0x2c, 0x64, 0xf9, 0x3f, 0x20, 0x92, 0x9a, 0xe7,
		0xb6, 0x09, 0x4a, 0x2d, 0x1b, 0xea, 0xf8, 0x9a, 0x0b, 0xee, 0xf6, 0x3d, 0x16, 0x46, 0xb2, 0x5c, 0xdc, 0x6f, 0x94, 0x36, 0x74, 0x2d,
		0x80, 0x6a, 0x17, 0x4d, 0x68, 0x89, 0xba, 0x68, 0x6a, 0x62, 0xd4, 0x99, 0x63, 0x92, 0xde, 0xfb, 0x22, 0x8f, 0xe4, 0xa5, 0xd1, 0x56,
		0xd7, 0xee, 0x0a, 0x70, 0x89, 0xae, 0x6b, 0x5e, 0xb2, 0x97, 0x74, 0xe7, 0x64, 0x4e, 0x68, 0x79, 0x46, 0x02, 0xe4, 0xd7, 0x21, 0x45,
		0x09, 0x09, 0xe3, 0x8b, 0xda, 0x3b, 0xf3, 0x4a, 0xa4, 0x09, 0x31, 0x6c, 0xc7, 0xbd, 0x7c, 0x38, 0x4f, 0x6b, 0xad, 0x9c, 0x45, 0xa5,
		0x6e, 0x95, 0x03, 0x31, 0x81, 0xa8, 0x6f, 0xc1, 0xe2, 0x59, 0xe9, 0xef, 0xaa, 0xf0, 0x9f, 0xbc, 0x73, 0x88, 0xca, 0x53, 0xfb, 0x03,
		0xed, 0xa8, 0x00, 0x4f, 0x8c, 0x49, 0x9e, 0x96, 0x5a, 0x68, 0x83, 0x1c, 0x48, 0x07, 0x9d, 0x8b, 0xbc, 0x47, 0x51, 0xc9, 0x86, 0x88,
		0x5b, 0x2a, 0xf8, 0xda, 0x70, 0xef, 0xac, 0xa9, 0xe4, 0x62, 0x3f, 0xb8, 0xfb, 0x73, 0xbd, 0xda, 0x87, 0x38, 0xc9, 0xa1, 0xf7, 0x3e,
		0x8a, 0x78, 0x1e, 0x87, 0xc5, 0xc2, 0x21, 0x2e, 0xc4, 0x89, 0x55, 0xec, 0x09, 0x80, 0x23, 0x4f, 0x41, 0x3e, 0xc7, 0x8c, 0x2a, 0x60,
		0x83, 0x0e, 0xf6, 0xd3, 0xbe, 0x81, 0xf4, 0x0a, 0x26, 0x6d, 0x80, 0xe9, 0xe3, 0xfe, 0x11, 0xbd, 0x31, 0x74, 0x1f, 0xc5, 0xc9, 0xe8,
		0x00, 0xe9, 0x13, 0xe6, 0xe9, 0x5a, 0x9b, 0x0a, 0x26, 0xfb, 0xdc, 0x8f, 0xa3, 0x2b, 0x4f, 0x05, 0xab, 0x1d, 0x10, 0x35, 0x7c, 0xb3,
		0xf5, 0xab, 0xd3, 0x0d, 0xfc, 0xae, 0xb5, 0x73, 0xa0, 0x7e, 0x9e, 0x56, 0x9c, 0x6e, 0xb4, 0xa2, 0xc2, 0x97, 0x32, 0x80, 0x9c, 0x0c,
		0x28, 0xa7, 0x64, 0x42, 0x3c, 0xfa, 0xe9, 0xff, 0x56, 0x5f, 0x60, 0x77, 0x35, 0x52, 0xad, 0x2c, 0xa4, 0xbb, 0xaf, 0x32, 0x0c, 0xf7,
		0xc8, 0x37, 0xe1, 0x68, 0x42, 0x21, 0x07, 0x73, 0xc0, 0x1b, 0x36, 0x1e, 0x7f, 0x8c, 0x36, 0x60, 0xbf, 0x19, 0x16, 0x75, 0xf5, 0x25,
		0x3e, 0x20, 0x8e, 0x68, 0x5f, 0x90, 0x3e, 0xa5, 0x47, 0x5e, 0xef, 0x0c, 0x7f, 0xf6, 0xd7, 0x55, 0xc0, 0xe4, 0x1c, 0x20, 0xd0, 0xba,
		0xe5, 0xc2, 0x71, 0xf5, 0x07, 0xc2, 0x80, 0x59, 0x75, 0xe7, 0x16, 0x84, 0x5e, 0x01, 0xe7, 0xaf, 0x5e, 0xdf, 0x9c, 0x53, 0x16, 0xe8,
		0x44, 0xc5, 0x6a, 0xda, 0x0a, 0xf7, 0x74, 0xfa, 0x98, 0xe1, 0xb3, 0xfd, 0x89, 0x55, 0xbc, 0x95, 0xf1, 0x29, 0xea, 0x0b, 0xdf, 0x69,
		0xd7, 0x43, 0x64, 0xf8, 0x6c, 0x3f, 0x78, 0xa5, 0xa2, 0xa9, 0xcf, 0xc1, 0x3a, 0xf7, 0x60, 0x61, 0xbc, 0x60, 0x45, 0xad, 0xe1, 0x19,
		0xfe, 0x79, 0xb7, 0xfc, 0x30, 0x5f, 0xdd, 0x15, 0x71, 0x30, 0x0b, 0x97, 0xb3, 0x60, 0x72, 0xcd, 0x92, 0x60, 0x9e, 0x2c, 0x57, 0x41,
		0x32, 0xb9, 0x5d, 0xae, 0x56, 0xc5, 0x3c, 0x8c, 0xc3, 0xdb, 0x5f, 0xa3, 0x07, 0xe0, 0x0d, 0xd7, 0xbf, 0x7f, 0xaf, 0xf2, 0x14, 0x2e,
		0xd6, 0xc2, 0x0a, 0x78, 0x24, 0xcc, 0xa1, 0xd8, 0x43, 0x89, 0x8f, 0x67, 0x5f, 0x86, 0x47, 0x9b, 0x81, 0x7e, 0x3f, 0xa3, 0x40, 0x7b,
		0xcc, 0x7d, 0x1e, 0x4f, 0xc3, 0x8f, 0x49, 0x14, 0x06, 0xc5, 0x75, 0x18, 0x05, 0x93, 0x29, 0x9d, 0x05, 0xb3, 0xe9, 0x75, 0x12, 0x14,
		0x49, 0x14, 0xaf, 0xa6, 0x93, 0xe5, 0x5d, 0x52, 0x24, 0x23, 0xee, 0xc9, 0x2b, 0x9f, 0x89, 0x90, 0x44, 0xd1, 0xf0, 0xe0, 0x78, 0xf2,
		0xc9, 0xc2, 0x71, 0xc9, 0x04, 0x57, 0x47, 0xad, 0x8e, 0x0a, 0x8d, 0xbd, 0x20, 0x12, 0x6c, 0xff, 0x52, 0x04, 0x39, 0x2a, 0x41, 0xce,
		0x7f, 0x06, 0xf9, 0x6f, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00,
		0x00, 0x21, 0x00, 0xc1, 0x17, 0x10, 0xbe, 0x4e, 0x07, 0x00, 0x00, 0xc6, 0x20, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x78, 0x6c, 0x2f,
		0x74, 0x68, 0x65, 0x6d, 0x65, 0x2f, 0x74, 0x68, 0x65, 0x6d, 0x65, 0x31, 0x2e, 0x78, 0x6d, 0x6c, 0xec, 0x59, 0xcd, 0x8b, 0x1b, 0x37,
		0x14, 0xbf, 0x17, 0xfa, 0x3f, 0x0c, 0x73, 0x77, 0xfc, 0x35, 0xe3, 0x8f, 0x25, 0xde, 0xe0, 0xcf, 0x6c, 0x93, 0xdd, 0x24, 0x64, 0x9d,
		0x94, 0x1c, 0xb5, 0xb6, 0xec, 0x51, 0x56, 0x33, 0x32, 0x92, 0xbc, 0x1b, 0x13, 0x02, 0x25, 0x39, 0xf5, 0x52, 0x28, 0xa4, 0xa5, 0x97,
		0x42, 0x6f, 0x3d, 0x94, 0xd2, 0x40, 0x03, 0x0d, 0xbd, 0xf4, 0x8f, 0x09, 0x24, 0xb4, 0xe9, 0x1f, 0xd1, 0x27, 0xcd, 0xd8, 0x23, 0xad,
		0xe5, 0x24, 0x9b, 0x6c, 0x4a, 0x5a, 0x76, 0x0d, 0x8b, 0x47, 0xfe, 0xbd, 0xa7, 0xa7, 0xf7, 0x9e, 0x7e, 0x7a, 0xf3, 0x74, 0xf1, 0xd2,
		0xbd, 0x98, 0x7a, 0x47, 0x98, 0x0b, 0xc2, 0x92, 0x96, 0x5f, 0xbe, 0x50, 0xf2, 0x3d, 0x9c, 0x8c, 0xd8, 0x98, 0x24, 0xd3, 0x96, 0x7f,
		0x6b, 0x38, 0x28, 0x34, 0x7c, 0x4f, 0x48, 0x94, 0x8c, 0x11, 0x65, 0x09, 0x6e, 0xf9, 0x0b, 0x2c, 0xfc, 0x4b, 0xdb, 0x9f, 0x7e, 0x72,
		0x11, 0x6d, 0xc9, 0x08, 0xc7, 0xd8, 0x03, 0xf9, 0x44, 0x6c, 0xa1, 0x96, 0x1f, 0x49, 0x39, 0xdb, 0x2a, 0x16, 0xc5, 0x08, 0x86, 0x91,
		0xb8, 0xc0, 0x66, 0x38, 0x81, 0xdf, 0x26, 0x8c, 0xc7, 0x48, 0xc2, 0x23, 0x9f, 0x16, 0xc7, 0x1c, 0x1d, 0x83, 0xde, 0x98, 0x16, 0x2b,
		0xa5, 0x52, 0xad, 0x18, 0x23, 0x92, 0xf8, 0x5e, 0x82, 0x62, 0x50, 0x7b, 0x7d, 0x32, 0x21, 0x23, 0xec, 0x0d, 0x95, 0x4a, 0x7f, 0x7b,
		0xa9, 0xbc, 0x4f, 0xe1, 0x31, 0x91, 0x42, 0x0d, 0x8c, 0x28, 0xdf, 0x57, 0xaa, 0xb1, 0x25, 0xa1, 0xb1, 0xe3, 0xc3, 0xb2, 0x42, 0x88,
		0x85, 0xe8, 0x52, 0xee, 0x1d, 0x21, 0xda, 0xf2, 0x61, 0x9e, 0x31, 0x3b, 0x1e, 0xe2, 0x7b, 0xd2, 0xf7, 0x28, 0x12, 0x12, 0x7e, 0x68,
		0xf9, 0x25, 0xfd, 0xe7, 0x17, 0xb7, 0x2f, 0x16, 0xd1, 0x56, 0x26, 0x44, 0xe5, 0x06, 0x59, 0x43, 0x6e, 0xa0, 0xff, 0x32, 0xb9, 0x4c,
		0x60, 0x7c, 0x58, 0xd1, 0x73, 0xf2, 0xe9, 0xc1, 0x6a, 0xd2, 0x20, 0x08, 0x83, 0x5a, 0x7b, 0xa5, 0x5f, 0x03, 0xa8, 0x5c, 0xc7, 0xf5,
		0xeb, 0xfd, 0x5a, 0xbf, 0xb6, 0xd2, 0xa7, 0x01, 0x68, 0x34, 0x82, 0x95, 0xa6, 0xb6, 0xd8, 0x3a, 0xeb, 0x95, 0x6e, 0x90, 0x61, 0x0d,
		0x50, 0xfa, 0xd5, 0xa1, 0xbb, 0x57, 0xef, 0x55, 0xcb, 0x16, 0xde, 0xd0, 0x5f, 0x5d, 0xb3, 0xb9, 0x1d, 0xaa, 0x8f, 0x85, 0xd7, 0xa0,
		0x54, 0x7f, 0xb0, 0x86, 0x1f, 0x0c, 0xba, 0xe0, 0x45, 0x0b, 0xaf, 0x41, 0x29, 0x3e, 0x5c, 0xc3, 0x87, 0x9d, 0x66, 0xa7, 0x67, 0xeb,
		0xd7, 0xa0, 0x14, 0x5f, 0x5b, 0xc3, 0xd7, 0x4b, 0xed, 0x5e, 0x50, 0xb7, 0xf4, 0x6b, 0x50, 0x44, 0x49, 0x72, 0xb8, 0x86, 0x2e, 0x85,
		0xb5, 0x6a, 0x77, 0xb9, 0xda, 0x15, 0x64, 0xc2, 0xe8, 0x8e, 0x13, 0xde, 0x0c, 0x83, 0x41, 0xbd, 0x92, 0x29, 0xcf, 0x51, 0x90, 0x0d,
		0xab, 0xec, 0x52, 0x53, 0x4c, 0x58, 0x22, 0x37, 0xe5, 0x5a, 0x8c, 0xee, 0x32, 0x3e, 0x00, 0x80, 0x02, 0x52, 0x24, 0x49, 0xe2, 0xc9,
		0xc5, 0x0c, 0x4f, 0xd0, 0x08, 0xb2, 0xb8, 0x8b, 0x28, 0x39, 0xe0, 0xc4, 0xdb, 0x25, 0xd3, 0x08, 0x12, 0x6f, 0x86, 0x12, 0x26, 0x60,
		0xb8, 0x54, 0x29, 0x0d, 0x4a, 0x55, 0xf8, 0xaf, 0x3e, 0x81, 0xfe, 0xa6, 0x23, 0x8a, 0xb6, 0x30, 0x32, 0xa4, 0x95, 0x5d, 0x60, 0x89,
		0x58, 0x1b, 0x52, 0xf6, 0x78, 0x62, 0xc4, 0xc9, 0x4c, 0xb6, 0xfc, 0x2b, 0xa0, 0xd5, 0x37, 0x20, 0x2f, 0x9e, 0x3d, 0x7b, 0xfe, 0xf0,
		0xe9, 0xf3, 0x87, 0xbf, 0x3d, 0x7f, 0xf4, 0xe8, 0xf9, 0xc3, 0x5f, 0xb2, 0xb9, 0xb5, 0x2a, 0x4b, 0x6e, 0x07, 0x25, 0x53, 0x53, 0xee,
		0xd5, 0x8f, 0x5f, 0xff, 0xfd, 0xfd, 0x17, 0xde, 0x5f, 0xbf, 0xfe, 0xf0, 0xea, 0xf1, 0x37, 0xe9, 0xd4, 0x27, 0xf1, 0xc2, 0xc4, 0xbf,
		0xfc, 0xf9, 0xcb, 0x97, 0xbf, 0xff, 0xf1, 0x3a, 0xf5, 0xb0, 0xe2, 0xdc, 0x15, 0x2f, 0xbe, 0x7d, 0xf2, 0xf2, 0xe9, 0x93, 0x17, 0xdf,
		0x7d, 0xf5, 0xe7, 0x4f, 0x8f, 0x1d, 0xda, 0xdb, 0x1c, 0x1d, 0x98, 0xf0, 0x21, 0x89, 0xb1, 0xf0, 0xae, 0xe1, 0x63, 0xef, 0x26, 0x8b,
		0x61, 0x81, 0x0e, 0xfb, 0xf1, 0x01, 0x3f, 0x9d, 0xc4, 0x30, 0x42, 0xc4, 0x92, 0x40, 0x11, 0xe8, 0x76, 0xa8, 0xee, 0xcb, 0xc8, 0x02,
		0x5e, 0x5b, 0x20, 0xea, 0xc2, 0x75, 0xb0, 0xed, 0xc2, 0xdb, 0x1c, 0x58, 0xc6, 0x05, 0xbc, 0x3c, 0xbf, 0x6b, 0xd9, 0xba, 0x1f, 0xf1,
		0xb9, 0x24, 0x8e, 0x99, 0xaf, 0x46, 0xb1, 0x05, 0xdc, 0x63, 0x8c, 0x76, 0x18, 0x77, 0x3a, 0xe0, 0xaa, 0x9a, 0xcb, 0xf0, 0xf0, 0x70,
		0x9e, 0x4c, 0xdd, 0x93, 0xf3, 0xb9, 0x89, 0xbb, 0x89, 0xd0, 0x91, 0x6b, 0xee, 0x2e, 0x4a, 0xac, 0x00, 0xf7, 0xe7, 0x33, 0xa0, 0x57,
		0xe2, 0x52, 0xd9, 0x8d, 0xb0, 0x65, 0xe6, 0x0d, 0x8a, 0x12, 0x89, 0xa6, 0x38, 0xc1, 0xd2, 0x53, 0xbf, 0xb1, 0x43, 0x8c, 0x1d, 0xab,
		0xbb, 0x43, 0x88, 0xe5, 0xd7, 0x3d, 0x32, 0xe2, 0x4c, 0xb0, 0x89, 0xf4, 0xee, 0x10, 0xaf, 0x83, 0x88, 0xd3, 0x25, 0x43, 0x72, 0x60,
		0x25, 0x52, 0x2e, 0xb4, 0x43, 0x62, 0x88, 0xcb, 0xc2, 0x65, 0x20, 0x84, 0xda, 0xf2, 0xcd, 0xde, 0x6d, 0xaf, 0xc3, 0xa8, 0x6b, 0xd5,
		0x3d, 0x7c, 0x64, 0x23, 0x61, 0x5b, 0x20, 0xea, 0x30, 0x7e, 0x88, 0xa9, 0xe5, 0xc6, 0xcb, 0x68, 0x2e, 0x51, 0xec, 0x52, 0x39, 0x44,
		0x31, 0x35, 0x1d, 0xbe, 0x8b, 0x64, 0xe4, 0x32, 0x72, 0x7f, 0xc1, 0x47, 0x26, 0xae, 0x2f, 0x24, 0x44, 0x7a, 0x8a, 0x29, 0xf3, 0xfa,
		0x63, 0x2c, 0x84, 0x4b, 0xe6, 0x3a, 0x87, 0xf5, 0x1a, 0x41, 0xbf, 0x0a, 0x0c, 0xe3, 0x0e, 0xfb, 0x1e, 0x5d, 0xc4, 0x36, 0x92, 0x4b,
		0x72, 0xe8, 0xd2, 0xb9, 0x8b, 0x18, 0x33, 0x91, 0x3d, 0x76, 0xd8, 0x8d, 0x50, 0x3c, 0x73, 0xda, 0x4c, 0x92, 0xc8, 0xc4, 0x7e, 0x26,
		0x0e, 0x21, 0x45, 0x91, 0x77, 0x83, 0x49, 0x17, 0x7c, 0x8f, 0xd9, 0x3b, 0x44, 0x3d, 0x43, 0x1c, 0x50, 0xb2, 0x31, 0xdc, 0xb7, 0x09,
		0xb6, 0xc2, 0xfd, 0x66, 0x22, 0xb8, 0x05, 0xe4, 0x6a, 0x9a, 0x94, 0x27, 0x88, 0xfa, 0x65, 0xce, 0x1d, 0xb1, 0xbc, 0x8c, 0x99, 0xbd,
		0x1f, 0x17, 0x74, 0x82, 0xb0, 0x8b, 0x65, 0xda, 0x3c, 0xb6, 0xd8, 0xb5, 0xcd, 0x89, 0x33, 0x3b, 0x3a, 0xf3, 0xa9, 0x95, 0xda, 0xbb,
		0x18, 0x53, 0x74, 0x8c, 0xc6, 0x18, 0x7b, 0xb7, 0x3e, 0x73, 0x58, 0xd0, 0x61, 0x33, 0xcb, 0xe7, 0xb9, 0xd1, 0x57, 0x22, 0x60, 0x95,
		0x1d, 0xec, 0x4a, 0xac, 0x2b, 0xc8, 0xce, 0x55, 0xf5, 0x9c, 0x60, 0x01, 0x65, 0x92, 0xaa, 0x6b, 0xd6, 0x29, 0x72, 0x97, 0x08, 0x2b,
		0x65, 0xf7, 0xf1, 0x94, 0x6d, 0xb0, 0x67, 0x6f, 0x71, 0x82, 0x78, 0x16, 0x28, 0x89, 0x11, 0xdf, 0xa4, 0xf9, 0x1a, 0x44, 0xdd, 0x4a,
		0x5d, 0x38, 0xe5, 0x9c, 0x54, 0x7a, 0x9d, 0x8e, 0x0e, 0x4d, 0xe0, 0x35, 0x02, 0xe5, 0x1f, 0xe4, 0x8b, 0xd3, 0x29, 0xd7, 0x05, 0xe8,
		0x30, 0x92, 0xbb, 0xbf, 0x49, 0xeb, 0x8d, 0x08, 0x59, 0x67, 0x97, 0x7a, 0x16, 0xee, 0x7c, 0x5d, 0x70, 0x2b, 0x7e, 0x6f, 0xb3, 0xc7,
		0x60, 0x5f, 0xde, 0x3d, 0xed, 0xbe, 0x04, 0x19, 0x7c, 0x6a, 0x19, 0x20, 0xf6, 0xb7, 0xf6, 0xcd, 0x10, 0x51, 0x6b, 0x82, 0x3c, 0x61,
		0x86, 0x08, 0x0a, 0x0c, 0x17, 0xdd, 0x82, 0x88, 0x15, 0xfe, 0x5c, 0x44, 0x9d, 0xab, 0x5a, 0x6c, 0xee, 0x94, 0x9b, 0xd8, 0x9b, 0x36,
		0x0f, 0x03, 0x14, 0x46, 0x56, 0xbd, 0x13, 0x93, 0xe4, 0x8d, 0xc5, 0xcf, 0x89, 0xb2, 0x27, 0xfc, 0x77, 0xca, 0x1e, 0x77, 0x01, 0x73,
		0x06, 0x05, 0x8f, 0x5b, 0xf1, 0xfb, 0x94, 0x3a, 0x9b, 0x28, 0x65, 0xe7, 0x44, 0x81, 0xb3, 0x09, 0xf7, 0x1f, 0x2c, 0x6b, 0x7a, 0x68,
		0x9e, 0xdc, 0xc0, 0x70, 0x92, 0xac, 0x73, 0xd6, 0x79, 0x55, 0x73, 0x5e, 0xd5, 0xf8, 0xff, 0xfb, 0xaa, 0x66, 0xd3, 0x5e, 0x3e, 0xaf,
		0x65, 0xce, 0x6b, 0x99, 0xf3, 0x5a, 0xc6, 0xf5, 0xf6, 0xf5, 0x41, 0x6a, 0x99, 0xbc, 0x7c, 0x81, 0xca, 0x26, 0xef, 0xf2, 0xe8, 0x9e,
		0x4f, 0xbc, 0xb1, 0xe5, 0x33, 0x21, 0x94, 0xee, 0xcb, 0x05, 0xc5, 0xbb, 0x42, 0x77, 0x7d, 0x04, 0xbc, 0xd1, 0x8c, 0x07, 0x30, 0xa8,
		0xdb, 0x51, 0xba, 0x27, 0xb9, 0x6a, 0x01, 0xce, 0x22, 0xf8, 0x9a, 0x35, 0x98, 0x2c, 0xdc, 0x94, 0x23, 0x2d, 0xe3, 0x71, 0x26, 0x3f,
		0x27, 0x32, 0xda, 0x8f, 0xd0, 0x0c, 0x5a, 0x43, 0x65, 0xdd, 0xc0, 0x9c, 0x8a, 0x4c, 0xf5, 0x54, 0x78, 0x33, 0x26, 0xa0, 0x63, 0xa4,
		0x87, 0x75, 0x2b, 0x15, 0x9f, 0xd0, 0xad, 0xfb, 0x4e, 0xf3, 0x78, 0x8f, 0x8d, 0xd3, 0x4e, 0x67, 0xb9, 0xac, 0xba, 0x9a, 0xa9, 0x0b,
		0x05, 0x92, 0xf9, 0x78, 0x29, 0x5c, 0x8d, 0x43, 0x97, 0x4a, 0xa6, 0xe8, 0x5a, 0x3d, 0xef, 0xde, 0xad, 0xd4, 0xeb, 0x7e, 0xe8, 0x54,
		0x77, 0x59, 0x97, 0x06, 0x28, 0xd9, 0xd3, 0x18, 0x61, 0x4c, 0x66, 0x1b, 0x51, 0x75, 0x18, 0x51, 0x5f, 0x0e, 0x42, 0x14, 0x5e, 0x67,
		0x84, 0x5e, 0xd9, 0x99, 0x58, 0xd1, 0x74, 0x58, 0xd1, 0x50, 0xea, 0x97, 0xa1, 0x5a, 0x46, 0x71, 0xe5, 0x0a, 0x30, 0x6d, 0x15, 0x15,
		0x78, 0xe5, 0xf6, 0xe0, 0x45, 0xbd, 0xe5, 0x87, 0x41, 0xda, 0x41, 0x86, 0x66, 0x1c, 0x94, 0xe7, 0x63, 0x15, 0xa7, 0xb4, 0x99, 0xbc,
		0x8c, 0xae, 0x0a, 0xce, 0x99, 0x46, 0x7a, 0x93, 0x33, 0xa9, 0x99, 0x01, 0x50, 0x62, 0x2f, 0x33, 0x20, 0x8f, 0x74, 0x53, 0xd9, 0xba,
		0x71, 0x79, 0x6a, 0x75, 0x69, 0xaa, 0xbd, 0x45, 0xa4, 0x2d, 0x23, 0x8c, 0x74, 0xb3, 0x8d, 0x30, 0xd2, 0x30, 0x82, 0x17, 0xe1, 0x2c,
		0x3b, 0xcd, 0x96, 0xfb, 0x59, 0xc6, 0xba, 0x99, 0x87, 0xd4, 0x32, 0x4f, 0xb9, 0x62, 0xb9, 0x1b, 0x72, 0x33, 0xea, 0x8d, 0x0f, 0x11,
		0x6b, 0x45, 0x22, 0x27, 0xb8, 0x81, 0x26, 0x26, 0x53, 0xd0, 0xc4, 0x3b, 0x6e, 0xf9, 0xb5, 0x6a, 0x08, 0xb7, 0x2a, 0x23, 0x34, 0x6b,
		0xf9, 0x13, 0xe8, 0x18, 0xc3, 0xd7, 0x78, 0x06, 0xb9, 0x23, 0xd4, 0x5b, 0x17, 0xa2, 0x53, 0xb8, 0x76, 0x19, 0x49, 0x9e, 0x6e, 0xf8,
		0x77, 0x61, 0x96, 0x19, 0x17, 0xb2, 0x87, 0x44, 0x94, 0x3a, 0x5c, 0x93, 0x4e, 0xca, 0x06, 0x31, 0x91, 0x98, 0x7b, 0x94, 0xc4, 0x2d,
		0x5f, 0x2d, 0x7f, 0x95, 0x0d, 0x34, 0xd1, 0x1c, 0xa2, 0x6d, 0x2b, 0x57, 0x80, 0x10, 0x3e, 0x5a, 0xe3, 0x9a, 0x40, 0x2b, 0x1f, 0x9b,
		0x71, 0x10, 0x74, 0x3b, 0xc8, 0x78, 0x32, 0xc1, 0x23, 0x69, 0x86, 0xdd, 0x18, 0x51, 0x9e, 0x4e, 0x1f, 0x81, 0xe1, 0x53, 0xae, 0x70,
		0xfe, 0xaa, 0xc5, 0xdf, 0x1d, 0xac, 0x24, 0xd9, 0x1c, 0xc2, 0xbd, 0x1f, 0x8d, 0x8f, 0xbd, 0x03, 0x3a, 0xe7, 0x37, 0x11, 0xa4, 0x58,
		0x58, 0x2f, 0x2b, 0x07, 0x8e, 0x89, 0x80, 0x8b, 0x83, 0x72, 0xea, 0xcd, 0x31, 0x81, 0x9b, 0xb0, 0x15, 0x91, 0xe5, 0xf9, 0x77, 0xe2,
		0x60, 0xca, 0x68, 0xd7, 0xbc, 0x8a, 0xd2, 0x39, 0x94, 0x8e, 0x23, 0x3a, 0x8b, 0x50, 0x76, 0xa2, 0x98, 0x64, 0x9e, 0xc2, 0x35, 0x89,
		0xae, 0xcc, 0xd1, 0x4f, 0x2b, 0x1f, 0x18, 0x4f, 0xd9, 0x9a, 0xc1, 0xa1, 0xeb, 0x2e, 0x3c, 0x98, 0xaa, 0x03, 0xf6, 0xbd, 0x4f, 0xdd,
		0x37, 0x1f, 0xd5, 0xca, 0x73, 0x06, 0x69, 0xe6, 0x67, 0xa6, 0xc5, 0x2a, 0xea, 0xd4, 0x74, 0x93, 0xe9, 0x87, 0x3b, 0xe4, 0x0d, 0xab,
		0xf2, 0x43, 0xd4, 0xb2, 0x2a, 0xa5, 0x6e, 0xfd, 0x4e, 0x2d, 0x72, 0xae, 0x6b, 0x2e, 0xb9, 0x0e, 0x12, 0xd5, 0x79, 0x4a, 0xbc, 0xe1,
		0xd4, 0x7d, 0x8b, 0x03, 0xc1, 0x30, 0x2d, 0x9f, 0xcc, 0x32, 0x4d, 0x59, 0xbc, 0x4e, 0xc3, 0x8a, 0xb3, 0xb3, 0x51, 0xdb, 0xb4, 0x33,
		0x2c, 0x08, 0x0c, 0x4f, 0xd4, 0x36, 0xf8, 0x6d, 0x75, 0x46, 0x38, 0x3d, 0xf1, 0xae, 0x27, 0x3f, 0xc8, 0x9d, 0xcc, 0x5a, 0x75, 0x40,
		0x2c, 0xeb, 0x4a, 0x9d, 0xf8, 0xfa, 0xca, 0xdc, 0xbc, 0xd5, 0x66, 0x07, 0x77, 0x81, 0x3c, 0x7a, 0x70, 0x7f, 0x38, 0xa7, 0x52, 0xe8,
		0x50, 0x42, 0x6f, 0x97, 0x23, 0x28, 0xfa, 0xd2, 0x1b, 0xc8, 0x94, 0x36, 0x60, 0x8b, 0xdc, 0x93, 0x59, 0x8d, 0x08, 0xdf, 0xbc, 0x39,
		0x27, 0x2d, 0xff, 0x7e, 0x29, 0x6c, 0x07, 0xdd, 0x4a, 0xd8, 0x2d, 0x94, 0x1a, 0x61, 0xbf, 0x10, 0x54, 0x83, 0x52, 0xa1, 0x11, 0xb6,
		0xab, 0x85, 0x76, 0x18, 0x56, 0xcb, 0xfd, 0xb0, 0x5c, 0xea, 0x75, 0x2a, 0x0f, 0xe0, 0x60, 0x91, 0x51, 0x5c, 0x0e, 0xd3, 0xeb, 0xfa,
		0x01, 0x5c, 0x61, 0xd0, 0x45, 0x76, 0x69, 0xaf, 0xc7, 0xd7, 0x2e, 0xee, 0xe3, 0xe5, 0x2d, 0xcd, 0x85, 0x11, 0x8b, 0x8b, 0x4c, 0x5f,
		0xcc, 0x17, 0xb5, 0xe1, 0xfa, 0xe2, 0xbe, 0x5c, 0xd9, 0x7c, 0x71, 0xef, 0x11, 0x20, 0x9d, 0xfb, 0xb5, 0xca, 0xa0, 0x59, 0x6d, 0x76,
		0x6a, 0x85, 0x66, 0xb5, 0x3d, 0x28, 0x04, 0xbd, 0x4e, 0xa3, 0xd0, 0xec, 0xd6, 0x3a, 0x85, 0x5e, 0xad, 0x5b, 0xef, 0x0d, 0x7a, 0xdd,
		0xb0, 0xd1, 0x1c, 0x3c, 0xf0, 0xbd, 0x23, 0x0d, 0x0e, 0xda, 0xd5, 0x6e, 0x50, 0xeb, 0x37, 0x0a, 0xb5, 0x72, 0xb7, 0x5b, 0x08, 0x6a,
		0x25, 0x65, 0x7e, 0xa3, 0x59, 0xa8, 0x07, 0x95, 0x4a, 0x3b, 0xa8, 0xb7, 0x1b, 0xfd, 0xa0, 0xfd, 0x20, 0x2b, 0x63, 0x60, 0xe5, 0x29,
		0x7d, 0x64, 0xbe, 0x00, 0xf7, 0x6a, 0xbb, 0xb6, 0xff, 0x01, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00,
		0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xff, 0x6b, 0x0c, 0xe9, 0xcd, 0x01, 0x00, 0x00, 0xb5, 0x03, 0x00, 0x00, 0x18, 0x00,
		0x00, 0x00, 0x78, 0x6c, 0x2f, 0x77, 0x6f, 0x72, 0x6b, 0x73, 0x68, 0x65, 0x65, 0x74, 0x73, 0x2f, 0x73, 0x68, 0x65, 0x65, 0x74, 0x31,
		0x2e, 0x78, 0x6d, 0x6c, 0x9c, 0x93, 0x4d, 0x8b, 0xdb, 0x30, 0x10, 0x86, 0xef, 0x85, 0xfe, 0x07, 0xa1, 0xbb, 0x2d, 0xdb, 0xf1, 0x3a,
		0x89, 0x89, 0xb3, 0x6c, 0x36, 0x0d, 0xdd, 0x43, 0xa1, 0xf4, 0xf3, 0x2c, 0xcb, 0x63, 0x5b, 0xc4, 0x92, 0x8c, 0xa4, 0x6c, 0x12, 0x4a,
		0xff, 0x7b, 0xc7, 0x0e, 0xf1, 0x16, 0x72, 0x09, 0x0b, 0x16, 0x68, 0xc6, 0x9a, 0x67, 0x66, 0xa4, 0x77, 0x56, 0x8f, 0x27, 0xd5, 0x91,
		0x57, 0xb0, 0x4e, 0x1a, 0x5d, 0xd0, 0x38, 0x8c, 0x28, 0x01, 0x2d, 0x4c, 0x25, 0x75, 0x53, 0xd0, 0x9f, 0x3f, 0x76, 0xc1, 0x82, 0x12,
		0xe7, 0xb9, 0xae, 0x78, 0x67, 0x34, 0x14, 0xf4, 0x0c, 0x8e, 0x3e, 0xae, 0x3f, 0x7e, 0x58, 0x1d, 0x8d, 0xdd, 0xbb, 0x16, 0xc0, 0x13,
		0x24, 0x68, 0x57, 0xd0, 0xd6, 0xfb, 0x3e, 0x67, 0xcc, 0x89, 0x16, 0x14, 0x77, 0xa1, 0xe9, 0x41, 0xe3, 0x9f, 0xda, 0x58, 0xc5, 0x3d,
		0x9a, 0xb6, 0x61, 0xae, 0xb7, 0xc0, 0xab, 0x31, 0x48, 0x75, 0x2c, 0x89, 0xa2, 0x8c, 0x29, 0x2e, 0x35, 0xbd, 0x10, 0x72, 0x7b, 0x0f,
		0xc3, 0xd4, 0xb5, 0x14, 0xb0, 0x35, 0xe2, 0xa0, 0x40, 0xfb, 0x0b, 0xc4, 0x42, 0xc7, 0x3d, 0xd6, 0xef, 0x5a, 0xd9, 0xbb, 0x2b, 0x4d,
		0x89, 0x7b, 0x70, 0x8a, 0xdb, 0xfd, 0xa1, 0x0f, 0x84, 0x51, 0x3d, 0x22, 0x4a, 0xd9, 0x49, 0x7f, 0x1e, 0xa1, 0x94, 0x28, 0x91, 0xbf,
		0x34, 0xda, 0x58, 0x5e, 0x76, 0xd8, 0xf7, 0x29, 0x4e, 0xb9, 0x20, 0x27, 0x8b, 0x5f, 0x82, 0x6b, 0x76, 0x4d, 0x33, 0xfa, 0x6f, 0x32,
		0x29, 0x29, 0xac, 0x71, 0xa6, 0xf6, 0x21, 0x92, 0xd9, 0xa5, 0xe6, 0xdb, 0xf6, 0x97, 0x6c, 0xc9, 0xb8, 0x98, 0x48, 0xb7, 0xfd, 0xdf,
		0x85, 0x89, 0x53, 0x66, 0xe1, 0x55, 0x0e, 0x0f, 0xf8, 0x86, 0x4a, 0xde, 0x57, 0x52, 0xfc, 0x30, 0xb1, 0x92, 0x37, 0xd8, 0xec, 0x9d,
		0xb0, 0x6c, 0x82, 0x0d, 0xd7, 0x65, 0xf3, 0x83, 0xac, 0x0a, 0xfa, 0x27, 0x4a, 0x97, 0xbb, 0x4d, 0xbc, 0x49, 0x83, 0xe4, 0x29, 0x8b,
		0x83, 0x34, 0x4b, 0x67, 0xc1, 0x62, 0x11, 0x67, 0xc1, 0x62, 0x33, 0xcf, 0x3e, 0x3d, 0xcf, 0xb2, 0xdd, 0x36, 0xdd, 0xfc, 0xa5, 0xeb,
		0x55, 0x25, 0xf1, 0x85, 0x87, 0xae, 0x88, 0x85, 0xba, 0xa0, 0x4f, 0x31, 0x65, 0xeb, 0xd5, 0x28, 0x9e, 0x5f, 0x12, 0x8e, 0xee, 0xbf,
		0x3d, 0xf1, 0xbc, 0xfc, 0x0e, 0x1d, 0x08, 0x0f, 0x98, 0x20, 0xa6, 0x64, 0xd0, 0x66, 0x69, 0xcc, 0x7e, 0x38, 0xf8, 0x82, 0xae, 0x68,
		0x08, 0x65, 0x37, 0xb1, 0xbb, 0x51, 0x9b, 0x5f, 0x2d, 0x29, 0xb9, 0x83, 0x67, 0xd3, 0xfd, 0x96, 0x95, 0x6f, 0x11, 0x80, 0x33, 0x50,
		0x41, 0xcd, 0x0f, 0x9d, 0xff, 0x66, 0x8e, 0x9f, 0x41, 0x36, 0xad, 0x47, 0x6f, 0x86, 0x3d, 0x0c, 0x22, 0xc8, 0xab, 0xf3, 0x16, 0x9c,
		0x40, 0xf5, 0x21, 0x38, 0x4c, 0xa6, 0xaa, 0xb6, 0xdc, 0x73, 0x4c, 0xd3, 0xf3, 0x06, 0xbe, 0x70, 0xdb, 0x48, 0xed, 0x48, 0x07, 0xf5,
		0x78, 0x68, 0x4e, 0x89, 0xbd, 0x50, 0xa2, 0x10, 0xf7, 0xde, 0xf4, 0x43, 0xe8, 0xfc, 0x81, 0x92, 0xd2, 0x78, 0x6f, 0xd4, 0xd5, 0x6a,
		0x71, 0x3c, 0x00, 0x65, 0x10, 0x85, 0x78, 0x61, 0xb5, 0x31, 0xfe, 0x6a, 0x0c, 0xe5, 0x4f, 0x03, 0xb7, 0xfe, 0x07, 0x00, 0x00, 0xff,
		0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xc2, 0x5e, 0x59, 0x08, 0x90,
		0x01, 0x00, 0x00, 0x1b, 0x03, 0x00, 0x00, 0x10, 0x00, 0x08, 0x01, 0x64, 0x6f, 0x63, 0x50, 0x72, 0x6f, 0x70, 0x73, 0x2f, 0x61, 0x70,
		0x70, 0x2e, 0x78, 0x6d, 0x6c, 0x20, 0xa2, 0x04, 0x01, 0x28, 0xa0, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x92, 0x4d, 0x6f, 0xdb, 0x30, 0x0c, 0x86, 0xef, 0x03, 0xfa, 0x1f, 0x0c, 0xdd, 0x1b, 0x39, 0x6d,
		0x51, 0x0c, 0x81, 0xac, 0x62, 0x48, 0x57, 0xf4, 0xb0, 0x62, 0x01, 0x92, 0x76, 0x67, 0x4e, 0xa6, 0x63, 0xa1, 0xb2, 0x24, 0x88, 0xac,
		0x91, 0xec, 0xd7, 0x4f, 0xb6, 0xd1, 0xd4, 0xd9, 0x76, 0xda, 0x8d, 0x1f, 0x2f, 0x5e, 0x3e, 0xa2, 0xa8, 0xee, 0x0e, 0x9d, 0x2b, 0x7a,
		0x4c, 0x64, 0x83, 0xaf, 0xc4, 0x72, 0x51, 0x8a, 0x02, 0xbd, 0x09, 0xb5, 0xf5, 0xfb, 0x4a, 0x3c, 0xef, 0x1e, 0x2e, 0x3f, 0x8b, 0x82,
		0x18, 0x7c, 0x0d, 0x2e, 0x78, 0xac, 0xc4, 0x11, 0x49, 0xdc, 0xe9, 0x8b, 0x4f, 0x6a, 0x93, 0x42, 0xc4, 0xc4, 0x16, 0xa9, 0xc8, 0x16,
		0x9e, 0x2a, 0xd1, 0x32, 0xc7, 0x95, 0x94, 0x64, 0x5a, 0xec, 0x80, 0x16, 0xb9, 0xed, 0x73, 0xa7, 0x09, 0xa9, 0x03, 0xce, 0x69, 0xda,
		0xcb, 0xd0, 0x34, 0xd6, 0xe0, 0x7d, 0x30, 0x6f, 0x1d, 0x7a, 0x96, 0x57, 0x65, 0x79, 0x2b, 0xf1, 0xc0, 0xe8, 0x6b, 0xac, 0x2f, 0xe3,
		0xc9, 0x50, 0x4c, 0x8e, 0xab, 0x9e, 0xff, 0xd7, 0xb4, 0x0e, 0x66, 0xe0, 0xa3, 0x97, 0xdd, 0x31, 0x66, 0x60, 0xad, 0xbe, 0xc4, 0xe8,
		0xac, 0x01, 0xce, 0xaf, 0xd4, 0x4f, 0xd6, 0xa4, 0x40, 0xa1, 0xe1, 0xe2, 0x09, 0x8c, 0xf5, 0x1c, 0xa8, 0x2d, 0xbe, 0x1e, 0x0c, 0x3a,
		0x25, 0xe7, 0x32, 0x95, 0x39, 0xb7, 0x68, 0xde, 0x92, 0xe5, 0xa3, 0x2e, 0x95, 0x9c, 0xa7, 0x6a, 0x6b, 0xc0, 0xe1, 0x3a, 0x8f, 0xd0,
		0x0d, 0x38, 0x42, 0x25, 0x3f, 0x0a, 0xea, 0x11, 0x61, 0x58, 0xdf, 0x06, 0x6c, 0x22, 0xad, 0x7a, 0x5e, 0xf5, 0x68, 0x38, 0xa4, 0x82,
		0xec, 0xaf, 0xbc, 0xc0, 0x2b, 0x51, 0xfc, 0x04, 0xc2, 0x01, 0xac, 0x12, 0x3d, 0x24, 0x0b, 0x9e, 0x33, 0xe0, 0x20, 0x9b, 0x92, 0x31,
		0x76, 0x91, 0x38, 0xe9, 0x1f, 0x21, 0xbd, 0x52, 0x8b, 0xc8, 0xa4, 0x64, 0x16, 0x4c, 0xc5, 0x31, 0x9c, 0x6b, 0xe7, 0xb1, 0xbd, 0xd1,
		0xcb, 0x51, 0x90, 0x83, 0x73, 0xe1, 0x60, 0x30, 0x81, 0xe4, 0xc6, 0x39, 0xe2, 0xce, 0xb2, 0x43, 0xfa, 0xde, 0x6c, 0x20, 0xf1, 0x3f,
		0x88, 0x97, 0x73, 0xe2, 0x91, 0x61, 0xe2, 0x9d, 0x70, 0xb6, 0x03, 0xdf, 0x34, 0x73, 0xce, 0x37, 0x3e, 0x39, 0x4f, 0xfa, 0xc3, 0x7b,
		0x1d, 0xba, 0x08, 0xfe, 0x98, 0x1b, 0xa7, 0xe8, 0x9b, 0xf5, 0xaf, 0xf4, 0x1c, 0x77, 0xe1, 0x1e, 0x18, 0xdf, 0xd7, 0x79, 0x5e, 0x54,
		0xdb, 0x16, 0x12, 0xd6, 0xf9, 0x07, 0x4e, 0xeb, 0x3e, 0x15, 0xd4, 0x63, 0xde, 0x64, 0x72, 0x83, 0xc9, 0xba, 0x05, 0xbf, 0xc7, 0xfa,
		0x5d, 0xf3, 0x77, 0x63, 0x38, 0x83, 0x97, 0xe9, 0xd6, 0xf5, 0xf2, 0x76, 0x51, 0x5e, 0x97, 0xf9, 0x5f, 0x67, 0x35, 0x25, 0x3f, 0xae,
		0x5a, 0xff, 0x06, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21,
		0x00, 0xbd, 0xde, 0xac, 0xb8, 0x45, 0x01, 0x00, 0x00, 0x6f, 0x02, 0x00, 0x00, 0x11, 0x00, 0x08, 0x01, 0x64, 0x6f, 0x63, 0x50, 0x72,
		0x6f, 0x70, 0x73, 0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x78, 0x6d, 0x6c, 0x20, 0xa2, 0x04, 0x01, 0x28, 0xa0, 0x00, 0x01, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x92, 0x5f, 0x4b, 0xc3, 0x30, 0x14, 0xc5, 0xdf, 0x05,
		0xbf, 0x43, 0xc9, 0x7b, 0x9b, 0x66, 0x1b, 0x63, 0x96, 0xb6, 0x83, 0x29, 0x7b, 0xd1, 0x81, 0xe0, 0x44, 0xf1, 0x2d, 0x24, 0x77, 0x5b,
		0xb0, 0xf9, 0x43, 0x12, 0xd7, 0xed, 0xdb, 0x9b, 0xb6, 0x5b, 0xad, 0xd4, 0x17, 0x1f, 0x93, 0x73, 0xee, 0x2f, 0xe7, 0x5c, 0x92, 0x2f,
		0x4f, 0xb2, 0x8a, 0x8e, 0x60, 0x9d, 0xd0, 0xaa, 0x40, 0x24, 0x49, 0x51, 0x04, 0x8a, 0x69, 0x2e, 0xd4, 0xbe, 0x40, 0xaf, 0xdb, 0x75,
		0xbc, 0x40, 0x91, 0xf3, 0x54, 0x71, 0x5a, 0x69, 0x05, 0x05, 0x3a, 0x83, 0x43, 0xcb, 0xf2, 0xf6, 0x26, 0x67, 0x26, 0x63, 0xda, 0xc2,
		0xb3, 0xd5, 0x06, 0xac, 0x17, 0xe0, 0xa2, 0x40, 0x52, 0x2e, 0x63, 0xa6, 0x40, 0x07, 0xef, 0x4d, 0x86, 0xb1, 0x63, 0x07, 0x90, 0xd4,
		0x25, 0xc1, 0xa1, 0x82, 0xb8, 0xd3, 0x56, 0x52, 0x1f, 0x8e, 0x76, 0x8f, 0x0d, 0x65, 0x9f, 0x74, 0x0f, 0x78, 0x92, 0xa6, 0x73, 0x2c,
		0xc1, 0x53, 0x4e, 0x3d, 0xc5, 0x0d, 0x30, 0x36, 0x3d, 0x11, 0x5d, 0x90, 0x9c, 0xf5, 0x48, 0xf3, 0x65, 0xab, 0x16, 0xc0, 0x19, 0x86,
		0x0a, 0x24, 0x28, 0xef, 0x30, 0x49, 0x08, 0xfe, 0xf1, 0x7a, 0xb0, 0xd2, 0xfd, 0x39, 0xd0, 0x2a, 0x03, 0xa7, 0x14, 0xfe, 0x6c, 0x42,
		0xa7, 0x4b, 0xdc, 0x21, 0x9b, 0xb3, 0x4e, 0xec, 0xdd, 0x27, 0x27, 0x7a, 0x63, 0x5d, 0xd7, 0x49, 0x3d, 0x6d, 0x63, 0x84, 0xfc, 0x04,
		0xbf, 0x6f, 0x9e, 0x5e, 0xda, 0xaa, 0xb1, 0x50, 0xcd, 0xae, 0x18, 0xa0, 0x32, 0xe7, 0x2c, 0x63, 0x16, 0xa8, 0xd7, 0xb6, 0x7c, 0x04,
		0xa5, 0xc0, 0x1f, 0xa2, 0x15, 0xad, 0x5c, 0x05, 0xc7, 0x1c, 0x0f, 0xb4, 0x66, 0x8f, 0x15, 0x75, 0x7e, 0x13, 0x56, 0xbe, 0x13, 0xc0,
		0x57, 0xe7, 0xb1, 0x7d, 0x6c, 0x09, 0xf4, 0xb6, 0x4c, 0xf7, 0x04, 0xf0, 0x28, 0xc4, 0xcb, 0xba, 0x32, 0x57, 0xe5, 0x6d, 0x7a, 0xff,
		0xb0, 0x5d, 0xa3, 0x72, 0x92, 0x92, 0xbb, 0x38, 0x5d, 0xc4, 0x64, 0xbe, 0x4d, 0xd3, 0x6c, 0x3a, 0xcb, 0xc8, 0xec, 0xa3, 0x49, 0xf0,
		0x6b, 0xbe, 0x89, 0xdb, 0x5d, 0xc8, 0x4b, 0x8e, 0xff, 0x10, 0x27, 0xf3, 0x01, 0xf1, 0x0a, 0x28, 0x73, 0x3c, 0xfa, 0x22, 0xe5, 0x37,
		0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0x9a, 0xab, 0x0f, 0x4f, 0x5e,
		0xff, 0xec, 0xaa, 0x93, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x73, 0x68, 0x61, 0x72,
		0x65, 0x64, 0x53, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x73, 0x2e, 0x78, 0x6d, 0x6c, 0x35, 0x8d, 0x41, 0x0a, 0xc2, 0x30, 0x10, 0x45, 0xf7,
		0x82, 0x77, 0x08, 0xb3, 0xd7, 0xa9, 0x2e, 0x44, 0x24, 0x49, 0x17, 0x82, 0x27, 0xd0, 0x03, 0x84, 0x76, 0x6c, 0x03, 0xcd, 0xa4, 0x66,
		0xa6, 0xa2, 0xb7, 0x37, 0x5d, 0xb8, 0xf8, 0xf0, 0x1f, 0x9f, 0xcf, 0xb3, 0xed, 0x27, 0x4d, 0xe6, 0x4d, 0x45, 0x62, 0x66, 0x07, 0x87,
		0x7d, 0x03, 0x86, 0xb8, 0xcb, 0x7d, 0xe4, 0xc1, 0xc1, 0xe3, 0x7e, 0xdb, 0x9d, 0xc1, 0x88, 0x06, 0xee, 0xc3, 0x94, 0x99, 0x1c, 0x7c,
		0x49, 0xa0, 0xf5, 0xdb, 0x8d, 0x15, 0x51, 0x53, 0xbf, 0x2c, 0x0e, 0x46, 0xd5, 0xf9, 0x82, 0x28, 0xdd, 0x48, 0x29, 0xc8, 0x3e, 0xcf,
		0xc4, 0x75, 0x79, 0xe6, 0x92, 0x82, 0x56, 0x2c, 0x03, 0xca, 0x5c, 0x28, 0xf4, 0x32, 0x12, 0x69, 0x9a, 0xf0, 0xd8, 0x34, 0x27, 0x4c,
		0x21, 0x32, 0x98, 0x2e, 0x2f, 0xac, 0xd5, 0x0b, 0x66, 0xe1, 0xf8, 0x5a, 0xe8, 0xfa, 0x67, 0x6f, 0x25, 0x7a, 0xab, 0xde, 0xe2, 0x9a,
		0xb5, 0x63, 0x35, 0x56, 0xf1, 0x0f, 0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0x99, 0x64, 0xef, 0x50, 0xd9, 0x19,
		0xd7, 0x73, 0x3f, 0x01, 0x00, 0x00, 0x7e, 0x04, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x5b, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74,
		0x5f, 0x54, 0x79, 0x70, 0x65, 0x73, 0x5d, 0x2e, 0x78, 0x6d, 0x6c, 0xad, 0x93, 0xcd, 0x4e, 0xc3, 0x30, 0x10, 0x84, 0xef, 0x3c, 0x45,
		0xe4, 0x2b, 0x4a, 0xdc, 0x72, 0x40, 0x08, 0x35, 0xed, 0x81, 0x9f, 0x23, 0x54, 0xa2, 0x3c, 0x80, 0xb1, 0x37, 0x8d, 0xd5, 0xf8, 0x47,
		0xbb, 0x6e, 0x69, 0xdf, 0x9e, 0x4d, 0x52, 0x21, 0x40, 0x55, 0x03, 0xb4, 0x97, 0x58, 0xf1, 0xce, 0xcc, 0x37, 0x3e, 0xec, 0x64, 0xb6,
		0x75, 0x4d, 0xb6, 0x01, 0x24, 0x1b, 0x7c, 0x29, 0xc6, 0xc5, 0x48, 0x64, 0xe0, 0x75, 0x30, 0xd6, 0x2f, 0x4b, 0xf1, 0xba, 0x78, 0xcc,
		0x6f, 0xc4, 0x6c, 0x3a, 0x59, 0xec, 0x22, 0x50, 0xc6, 0x52, 0x4f, 0xa5, 0xa8, 0x53, 0x8a, 0xb7, 0x52, 0x92, 0xae, 0xc1, 0x29, 0x2a,
		0x42, 0x04, 0xcf, 0x93, 0x2a, 0xa0, 0x53, 0x89, 0x7f, 0x71, 0x29, 0xa3, 0xd2, 0x2b, 0xb5, 0x04, 0x79, 0x35, 0x1a, 0x5d, 0x4b, 0x1d,
		0x7c, 0x02, 0x9f, 0xf2, 0xd4, 0x66, 0x88, 0xe9, 0xe4, 0x1e, 0x2a, 0xb5, 0x6e, 0x52, 0xf6, 0xb0, 0xe5, 0xeb, 0x1e, 0x8b, 0xd0, 0x90,
		0xc8, 0xee, 0x7a, 0x61, 0xcb, 0x2a, 0x85, 0x8a, 0xb1, 0xb1, 0x5a, 0x25, 0x9e, 0xcb, 0x8d, 0x37, 0x3f, 0x28, 0xf9, 0x9e, 0x50, 0xb0,
		0xb3, 0xd3, 0x50, 0x6d, 0x23, 0x5d, 0xb2, 0x40, 0xc8, 0x83, 0x04, 0x9e, 0x1c, 0x01, 0xec, 0x7d, 0xcf, 0x1b, 0x40, 0xb4, 0x06, 0xb2,
		0xb9, 0xc2, 0xf4, 0xa4, 0x1c, 0xab, 0xe4, 0xb6, 0x91, 0xef, 0x01, 0x57, 0x6f, 0x21, 0xac, 0x0a, 0x96, 0xfd, 0xad, 0x65, 0xa8, 0x2a,
		0xab, 0xc1, 0x04, 0xbd, 0x76, 0x6c, 0x29, 0x28, 0x22, 0x28, 0x43, 0x35, 0x40, 0x72, 0x4d, 0xd1, 0x9d, 0x85, 0x53, 0xd6, 0x5f, 0x0e,
		0xf3, 0x3b, 0x31, 0xc9, 0xee, 0x18, 0x9f, 0xb9, 0xc8, 0x67, 0xfe, 0x40, 0x8f, 0x54, 0x83, 0x83, 0xfe, 0x7b, 0x7a, 0x85, 0x2e, 0x66,
		0x00, 0x48, 0x69, 0xd7, 0x00, 0x9d, 0x8c, 0xfa, 0xfe, 0xda, 0x3e, 0x74, 0x88, 0x5c, 0x2b, 0x04, 0xf3, 0x92, 0x90, 0xd7, 0xe0, 0xec,
		0x05, 0xbe, 0x64, 0x1f, 0xed, 0xc1, 0xfe, 0x39, 0x86, 0x48, 0x52, 0x07, 0x84, 0xb6, 0xc4, 0xff, 0x56, 0xa4, 0x75, 0xe7, 0x11, 0x79,
		0x8a, 0xc9, 0xc2, 0xef, 0x88, 0x1c, 0x7d, 0xf2, 0xab, 0xa1, 0xdd, 0x3e, 0x03, 0xe6, 0x00, 0x5b, 0xb6, 0x79, 0x34, 0xbd, 0xf8, 0x00,
		0x50, 0x4b, 0x03, 0x04, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0xa7, 0x64, 0xef, 0x50, 0x59, 0xaf, 0xf9, 0x51, 0xdc, 0x00, 0x00, 0x00,
		0xa8, 0x02, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x5f, 0x72, 0x65, 0x6c, 0x73, 0x2f, 0x77, 0x6f, 0x72, 0x6b, 0x62,
		0x6f, 0x6f, 0x6b, 0x2e, 0x78, 0x6d, 0x6c, 0x2e, 0x72, 0x65, 0x6c, 0x73, 0xad, 0x92, 0xcd, 0x6a, 0xc3, 0x30, 0x10, 0x84, 0xef, 0x7d,
		0x0a, 0xb1, 0xf7, 0x5a, 0x76, 0x5a, 0x4a, 0x29, 0x51, 0x72, 0x29, 0x85, 0x5c, 0xdb, 0xf4, 0x01, 0x84, 0xbc, 0xb6, 0x4c, 0x6c, 0x49,
		0xec, 0x6e, 0x7f, 0xf2, 0xf6, 0x11, 0x0e, 0x24, 0x31, 0x84, 0x90, 0x83, 0x4f, 0x62, 0x46, 0xda, 0x99, 0x4f, 0xb0, 0xcb, 0xf5, 0xff,
		0xd0, 0xab, 0x5f, 0x24, 0xee, 0x62, 0x30, 0x50, 0x15, 0x25, 0x28, 0x0c, 0x2e, 0xd6, 0x5d, 0x68, 0x0d, 0x7c, 0x6f, 0x3f, 0x1e, 0x5f,
		0x61, 0xbd, 0x5a, 0x7e, 0x62, 0x6f, 0x25, 0xbf, 0x60, 0xdf, 0x25, 0x56, 0x79, 0x24, 0xb0, 0x01, 0x2f, 0x92, 0xde, 0xb4, 0x66, 0xe7,
		0x71, 0xb0, 0x5c, 0xc4, 0x84, 0x21, 0xdf, 0x34, 0x91, 0x06, 0x2b, 0x59, 0x52, 0xab, 0x93, 0x75, 0x3b, 0xdb, 0xa2, 0x5e, 0x94, 0xe5,
		0x8b, 0xa6, 0xcb, 0x0c, 0x98, 0x66, 0xaa, 0x4d, 0x6d, 0x80, 0x36, 0xf5, 0x33, 0xa8, 0xed, 0x3e, 0xe1, 0x3d, 0xd9, 0xb1, 0x69, 0x3a,
		0x87, 0xef, 0xd1, 0xfd, 0x0c, 0x18, 0xe4, 0x4a, 0x85, 0x66, 0x6f, 0x09, 0xeb, 0x2f, 0xa1, 0xfc, 0x17, 0xce, 0xc1, 0x96, 0x5a, 0x14,
		0x03, 0x13, 0xbb, 0xc8, 0xa9, 0xa0, 0xaf, 0xc3, 0x3c, 0xcd, 0x0a, 0x23, 0xfb, 0x1e, 0x2f, 0x28, 0x46, 0x7d, 0xb3, 0x7e, 0x31, 0x67,
		0xbd, 0xe4, 0x59, 0x3c, 0xb7, 0x8f, 0xf2, 0x68, 0x56, 0xb7, 0x18, 0xaa, 0x39, 0x19, 0xfe, 0x22, 0xed, 0xd8, 0x23, 0xca, 0x99, 0xe3,
		0x64, 0xb1, 0x1e, 0x8f, 0x13, 0x8c, 0x9e, 0x6c, 0xdc, 0xea, 0xe1, 0x00, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xb5, 0x55, 0x30, 0x23, 0xf4, 0x00, 0x00, 0x00, 0x4c, 0x02, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x72, 0x65, 0x6c, 0x73, 0x2f, 0x2e, 0x72,
		0x65, 0x6c, 0x73, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0x47, 0x88, 0xbc,
		0xe2, 0x5d, 0x03, 0x00, 0x00, 0x35, 0x08, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x25, 0x03, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x77, 0x6f, 0x72, 0x6b, 0x62, 0x6f, 0x6f, 0x6b, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b,
		0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xf0, 0x08, 0x58, 0xf4, 0xa5, 0x02, 0x00, 0x00,
		0x52, 0x06, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xaf, 0x06, 0x00, 0x00,
		0x78, 0x6c, 0x2f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x73, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06,
		0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xc1, 0x17, 0x10, 0xbe, 0x4e, 0x07, 0x00, 0x00, 0xc6, 0x20, 0x00, 0x00, 0x13, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x09, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x74, 0x68, 0x65, 0x6d,
		0x65, 0x2f, 0x74, 0x68, 0x65, 0x6d, 0x65, 0x31, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xff, 0x6b, 0x0c, 0xe9, 0xcd, 0x01, 0x00, 0x00, 0xb5, 0x03, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x10, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x77, 0x6f, 0x72, 0x6b, 0x73,
		0x68, 0x65, 0x65, 0x74, 0x73, 0x2f, 0x73, 0x68, 0x65, 0x65, 0x74, 0x31, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00,
		0x14, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xc2, 0x5e, 0x59, 0x08, 0x90, 0x01, 0x00, 0x00, 0x1b, 0x03, 0x00, 0x00,
		0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x13, 0x00, 0x00, 0x64, 0x6f, 0x63, 0x50,
		0x72, 0x6f, 0x70, 0x73, 0x2f, 0x61, 0x70, 0x70, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x2d, 0x00, 0x14, 0x00, 0x06, 0x00,
		0x08, 0x00, 0x00, 0x00, 0x21, 0x00, 0xbd, 0xde, 0xac, 0xb8, 0x45, 0x01, 0x00, 0x00, 0x6f, 0x02, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x15, 0x00, 0x00, 0x64, 0x6f, 0x63, 0x50, 0x72, 0x6f, 0x70, 0x73,
		0x2f, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0x9a,
		0xab, 0x0f, 0x4f, 0x5e, 0xff, 0xec, 0xaa, 0x93, 0x00, 0x00, 0x00, 0xb2, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0xb6, 0x81, 0x43, 0x18, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x73, 0x68, 0x61, 0x72, 0x65, 0x64, 0x53, 0x74,
		0x72, 0x69, 0x6e, 0x67, 0x73, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0x99,
		0x64, 0xef, 0x50, 0xd9, 0x19, 0xd7, 0x73, 0x3f, 0x01, 0x00, 0x00, 0x7e, 0x04, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0xb6, 0x81, 0x08, 0x19, 0x00, 0x00, 0x5b, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x5f, 0x54, 0x79,
		0x70, 0x65, 0x73, 0x5d, 0x2e, 0x78, 0x6d, 0x6c, 0x50, 0x4b, 0x01, 0x02, 0x14, 0x03, 0x14, 0x00, 0x02, 0x00, 0x08, 0x00, 0xa7, 0x64,
		0xef, 0x50, 0x59, 0xaf, 0xf9, 0x51, 0xdc, 0x00, 0x00, 0x00, 0xa8, 0x02, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x00, 0x00, 0x00, 0xb6, 0x81, 0x78, 0x1a, 0x00, 0x00, 0x78, 0x6c, 0x2f, 0x5f, 0x72, 0x65, 0x6c, 0x73, 0x2f, 0x77, 0x6f, 0x72,
		0x6b, 0x62, 0x6f, 0x6f, 0x6b, 0x2e, 0x78, 0x6d, 0x6c, 0x2e, 0x72, 0x65, 0x6c, 0x73, 0x50, 0x4b, 0x05, 0x06, 0x00, 0x00, 0x00, 0x00,
		0x0a, 0x00, 0x0a, 0x00, 0x80, 0x02, 0x00, 0x00, 0x8c, 0x1b, 0x00, 0x00, 0x00, 0x00
	};
}

XLDocument::XLDocument(const IZipArchive& zipArchive) : m_xmlSavingDeclaration{}, m_archive(zipArchive) 
{
}
/**
 * @details An alternative constructor, taking a std::string with the path to the .xlsx package as an argument.
 */
XLDocument::XLDocument(const std::string& docPath, const IZipArchive & zipArchive) : m_xmlSavingDeclaration{}, m_archive(zipArchive)
{
	open(docPath);
}
/**
 * @details The destructor calls the closeDocument method before the object is destroyed.
 */
XLDocument::~XLDocument()
{
	if(isOpen())  
		close();// 2024-05-31 prevent double-close if document has been manually closed before
}
/**
 * @details disable m_suppressWarnings
 */
void XLDocument::showWarnings() { m_suppressWarnings = false; }

/**
 * @details enable m_suppressWarnings
 */
void XLDocument::suppressWarnings() { m_suppressWarnings = true; }

/**
 * @details The openDocument method opens the .xlsx package in the following manner:
 * - Check if a document is already open. If yes, close it.
 * - Create a temporary folder for the contents of the .xlsx package
 * - Unzip the contents of the package to the temporary folder.
 * - load the contents into the data structure for manipulation.
 */
void XLDocument::open(const std::string & fileName)
{
	// Check if a document is already open. If yes, close it.
	if(m_archive.isOpen())
		close();// TBD: consider throwing if a file is already open.
	m_filePath = fileName;
	m_archive.open(m_filePath);
	// ===== Add and open the Relationships and [Content_Types] files for the document level.
	std::string relsFilename = "_rels/.rels";
	m_data.emplace_back(this, "[Content_Types].xml");
	m_data.emplace_back(this, relsFilename);
	m_contentTypes     = XLContentTypes(getXmlData("[Content_Types].xml"));
	m_docRelationships = XLRelationships(getXmlData(relsFilename), relsFilename);
	std::string workbookPath = "xl/workbook.xml";
	bool workbookAdded = false;
	for(auto& item : m_docRelationships.relationships()) {
		if(item.type() == XLRelationshipType::Workbook) {
			workbookPath = item.target();
			if(workbookPath[ 0 ] == '/')
				workbookPath = workbookPath.substr(1);// NON STANDARD FORMATS: strip leading '/'
			m_data.emplace_back(this, workbookPath, item.id(), XLContentType::Workbook);
			workbookAdded = true;
			break;
		}
	}
	// ===== Determine workbook relationships path based on workbookPath, and construct m_wbkRelationships
	size_t path_pos = workbookPath.find_last_of('/');
	if(path_pos == std::string::npos) {
		using namespace std::literals::string_literals;
		throw XLInputError(std::string("workbook path from "s + relsFilename + " has no folder name: "s) + workbookPath);
	}
	std::string workbookRelsFilename = std::string("xl/_rels/") + workbookPath.substr(path_pos + 1) + std::string(".rels");
	m_data.emplace_back(this, workbookRelsFilename); // m_data.emplace_back(this, "xl/_rels/workbook.xml.rels");
	m_wbkRelationships = XLRelationships(getXmlData(workbookRelsFilename), workbookRelsFilename);
	// ===== Create xl/styles.xml if missing
	if(!m_archive.hasEntry("xl/styles.xml"))  
		execCommand(XLCommand(XLCommandType::AddStyles));
	// ===== Create xl/sharedStrings.xml from scratch if missing
	if(!m_archive.hasEntry("xl/sharedStrings.xml"))  
		execCommand(XLCommand(XLCommandType::AddSharedStrings));
	// ===== Add remaining spreadsheet elements to the vector of XLXmlData objects.
	for(auto& item : m_contentTypes.getContentItems()) {
		// ===== 2024-07-26 BUGFIX: ignore content item entries for relationship files, that have already been read above
		if(item.path().substr(1) == relsFilename)
			continue;    // always ignore relsFilename - would have thrown above if not found
		if(item.path().substr(1) == workbookRelsFilename)
			continue;// always ignore workbookRelsFilename - would have thrown
		// ===== Test if item is not in a known and handled subfolder (e.g. /customXml/*)
		if(size_t pos = item.path().substr(1).find_first_of('/'); pos != std::string::npos) {
			std::string subdirectory = item.path().substr(1, pos);
			if(subdirectory != "xl" && subdirectory != "docProps")
				continue;// ignore items in unhandled subfolders
		}
		bool isWorkbookPath = (item.path().substr(1) == workbookPath); // determine once, use thrice
		if(!isWorkbookPath && item.path().substr(0, 4) == "/xl/") {
			if((item.path().substr(4, 7) == "comment") || (item.path().substr(4, 12) == "tables/table") || 
				(item.path().substr(4, 19) == "drawings/vmlDrawing") || (item.path().substr(4, 22) == "worksheets/_rels/sheet")) {
				// no-op - worksheet dependencies will be loaded on access through the worksheet
			}
			else if((item.path().substr(4, 16) == "worksheets/sheet") || (item.path().substr(4) == "sharedStrings.xml") ||(item.path().substr(4) == "styles.xml") || (item.path().substr(4, 11) == "theme/theme")) {
				m_data.emplace_back(/* parentDoc */this, /*xmlPath*/item.path().substr(1), /*xmlID*/m_wbkRelationships.relationshipByTarget(item.path().substr(4)).id(),
				    /*xmlType*/item.type());
			}
			else {
				if(!m_suppressWarnings)
					std::cout << "ignoring currently unhandled workbook item " << item.path() << std::endl;
			}
		}
		else if(!isWorkbookPath || !workbookAdded) { // do not re-add workbook if it was previously added via m_docRelationships
			if(isWorkbookPath) { // if workbook is found but workbook relationship did not exist in m_docRelationships
				workbookAdded = true; // 2024-09-30 bugfix: was set to true after checking item.path() == workbookPath, not item.path().substr(1) as above
				std::cerr << "adding missing workbook relationship to _rels/.rels" << std::endl;
				m_docRelationships.addRelationship(XLRelationshipType::Workbook, workbookPath); // Pull request #185: Fix missing workbook relationship
			}
			m_data.emplace_back(/* parentDoc */ this,
			    /* xmlPath   */ item.path().substr(1),
			    /* xmlID     */ m_docRelationships.relationshipByTarget(item.path().substr(1)).id(),
			    /* xmlType   */ item.type());
		}
	}

	// ===== Read shared strings table.
	OXlXmlDoc * sharedStrings = getXmlData("xl/sharedStrings.xml")->getXmlDocument();
	if(not sharedStrings->document_element().attribute("uniqueCount").empty())
		sharedStrings->document_element().remove_attribute("uniqueCount"); // pull request #192 -> remove count & uniqueCount as they are optional
	if(not sharedStrings->document_element().attribute("count").empty())
		sharedStrings->document_element().remove_attribute("count"); // pull request #192 -> remove count & uniqueCount as they are optional
	XMLNode node = sharedStrings->document_element().first_child_of_type(pugi::node_element); // pull request #186: Skip non-element nodes in sst.
	while(!node.empty()) {
		// ===== Validate si node name.
		using namespace std::literals::string_literals;
		if(node.name() != "si"s)  
			throw XLInputError("xl/sharedStrings.xml sst node name \""s + node.name() + "\" is not \"si\""s);
		// ===== 2024-09-01 Refactored code to tolerate a mix of <t> and <r> tags within a shared string entry.
		// This simplifies the loop while not doing any harm (obsolete inner loops for rich text and text elements removed).

		// ===== Find first node_element child of si node.
		XMLNode elem = node.first_child_of_type(pugi::node_element);
		std::string result{}; // assemble a shared string entry here
		while(!elem.empty()) {
			// 2024-09-01: support a string composed of multiple <t> nodes in the same way as rich text <r> nodes, because LibreOffice accepts it
			std::string elementName = elem.name(); // assign name to a string once, for string comparisons using operator==
			if(elementName == "t")         // If elem is a regular string
				result += elem.text().get(); // append the tag value to result
			else if(elementName == "r")    // If elem is rich text
				result += elem.child("t").text().get(); // append the <t> node value to result
			// ===== Ignore phonetic property tags
			else if(elementName == "rPh" || elementName == "phoneticPr") {
			}
			else // For all other (unexpected) tags, throw an exception
				throw XLInputError("xl/sharedStrings.xml si node \""s + elementName + "\" is none of \"r\", \"t\", \"rPh\", \"phoneticPr\""s);

			elem = elem.next_sibling_of_type(pugi::node_element); // advance to next child of <si>
		}
		// ===== Append an empty string even if elem.empty(), to keep the index aligned with the <si> tag index in the shared strings table <sst>
		m_sharedStringCache.emplace_back(result); // 2024-09-01 TBC BUGFIX: previously, a shared strings table entry that had neither <t> nor
		/**/                              //     <r> nodes would not have appended to m_sharedStringCache, causing an index misalignment

		node = node.next_sibling_of_type(pugi::node_element);
	}
	// ===== Open the workbook and document property items
	m_workbook = XLWorkbook(getXmlData(workbookPath));
	// 2024-05-31: moved XLWorkbook object creation up in code worksheets info can be used for XLAppProperties generation from scratch
	// ===== 2024-06-03: creating core and extended properties if they do not exist
	execCommand(XLCommand(XLCommandType::CheckAndFixCoreProperties));  // checks & fixes consistency of docProps/core.xml related data
	execCommand(XLCommand(XLCommandType::CheckAndFixExtendedProperties)); // checks & fixes consistency of docProps/app.xml related data
	if(!hasXmlData("docProps/core.xml") || !hasXmlData("docProps/app.xml"))
		throw XLInternalError("Failed to repair docProps (core.xml and/or app.xml)");
	m_coreProperties = XLProperties(getXmlData("docProps/core.xml"));
	m_appProperties  = XLAppProperties(getXmlData("docProps/app.xml"), m_workbook.xmlDocument());
	// ===== 2024-09-02: ensure that all worksheets are contained in app.xml <TitlesOfParts> and reflected in <HeadingPairs> value for Worksheets
	m_appProperties.alignWorksheets(m_workbook.sheetNames());
	m_sharedStrings  = XLSharedStrings(getXmlData("xl/sharedStrings.xml"), &m_sharedStringCache);
	m_styles         = XLStyles(getXmlData("xl/styles.xml"), m_suppressWarnings);// 2024-10-14: forward supress warnings setting to XLStyles
}

namespace {
	// 
	// @brief Test if path exists as either a file or a directory
	// @param path Check for existence of this
	// @return true if path exists as a file or directory
	// 
	bool pathExists(const std::string& path)
	{
		STATSTRUCT info;
		if(STAT(path.c_str(), &info) == 0) // test if path exists
			return true;
		return false;
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wunused-function"
	#endif // __GNUC__
	/* @sobolev
	// 
	// @brief Test if fileName exists and is not a directory
	// @param fileName The path to check for existence (as a file)
	// @return true if fileName exists and is a file, otherwise false
	// 
	bool fileExists(const std::string& fileName)
	{
		STATSTRUCT info;
		if(STAT(fileName.c_str(), &info) == 0) // test if path exists
			if((info.st_mode & S_IFDIR) == 0) // test if it is NOT a directory
				return true;
		return false;
	}*/
	bool isDirectory(const std::string& fileName)
	{
		STATSTRUCT info;
		if(STAT(fileName.c_str(), &info) == 0)      // test if path exists
			if((info.st_mode & S_IFDIR) != 0)       // test if it is a directory
				return true;
		return false;
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
		#pragma GCC diagnostic pop
	#endif // __GNUC__
} // anonymous namespace

/**
 * @details Create a new document. This is done by saving the data in XLTemplate.h in binary format.
 */
void XLDocument::create(const std::string & fileName, bool forceOverwrite)
{
	// 2024-07-26: prevent silent overwriting of existing files
	if(!forceOverwrite && pathExists(fileName)) {
		using namespace std::literals::string_literals;
		throw XLException("XLDocument::create: refusing to overwrite existing file "s + fileName);
	}
	// ===== Create a temporary output file stream.
#ifdef ENABLE_NOWIDE
	nowide::ofstream outfile(fileName, std::ios::binary);
#else
	std::ofstream outfile(fileName, std::ios::binary);
#endif

	// ===== Stream the binary data for an empty workbook to the output file.
	// ===== Casting, in particular reinterpret_cast, is discouraged, but in this case it is unfortunately unavoidable.
	outfile.write(reinterpret_cast<const char*>(templateData), templateSize);
	outfile.close();
	open(fileName);
}
/**
 * @details Legacy function to create a new document
 * @deprecated use instead XLDocument::create(const std::string& fileName, bool forceOverwrite)
 * @warning This deprecated function overwrites an existing file without prompt
 */
// @sobolev void XLDocument::create(const std::string & fileName) { create(fileName, XLForceOverwrite); }

/**
 * @details The document is closed by deleting the temporary folder structure.
 */
void XLDocument::close()
{
	if(m_archive.isValid())  
		m_archive.close();
	// m_suppressWarnings shall remain in the configured setting
	m_filePath.clear();
	m_xmlSavingDeclaration = XLXmlSavingDeclaration();
	m_data.clear();
	m_sharedStringCache.clear();         // 2024-12-18 BUGFIX: clear shared strings cache - addresses issue #283
	m_sharedStrings    = XLSharedStrings();//
	m_docRelationships = XLRelationships();
	m_wbkRelationships = XLRelationships();
	m_contentTypes     = XLContentTypes();
	m_appProperties    = XLAppProperties();
	m_coreProperties   = XLProperties();
	m_styles           = XLStyles();
	m_workbook         = XLWorkbook();
	// m_archive          = IZipArchive(); // keep IZipArchive class intact throughout close/open
}
/**
 * @details Save the document with the same name. The existing file will be overwritten.
 */
void XLDocument::save() 
{ 
	saveAs(m_filePath, XLForceOverwrite); 
}
/**
 * @details Save the document with a new name. If present, the 'calcChain.xml file will be ignored. The reason for this
 * is that changes to the document may invalidate the calcChain.xml file. Deleting will force Excel to re-create the
 * file. This will happen automatically, without the user noticing.
 */
void XLDocument::saveAs(const std::string& fileName, bool forceOverwrite)
{
	// 2024-07-26: prevent silent overwriting of existing files
	if(!forceOverwrite && pathExists(fileName)) {
		using namespace std::literals::string_literals;
		throw XLException("XLDocument::saveAs: refusing to overwrite existing file "s + fileName);
	}
	m_filePath = fileName;
	// ===== Delete the calcChain.xml file in order to force re-calculation of the sheet
	// TODO: Is this the best way to do it? Maybe there is a flag that can be set, that forces re-calculalion.
	execCommand(XLCommand(XLCommandType::ResetCalcChain));
	// ===== Add all xml items to archive and save the archive.
	for(auto & item : m_data) {
		bool xmlIsStandalone = m_xmlSavingDeclaration.standalone_as_bool();
		if((item.getXmlPath() == "docProps/core.xml") || (item.getXmlPath() == "docProps/app.xml"))
			xmlIsStandalone = XLXmlStandalone;
		m_archive.addEntry(item.getXmlPath(), item.getRawData(XLXmlSavingDeclaration(m_xmlSavingDeclaration.version(), m_xmlSavingDeclaration.encoding(), xmlIsStandalone)));
	}
	m_archive.save(m_filePath);
}

/**
 * @details Legacy function to save the document with a new name
 * @deprecated use instead void XLDocument::saveAs(const std::string& fileName, bool forceOverwrite)
 * @warning This deprecated function overwrites an existing file without prompt
 */
//void XLDocument::saveAs(const std::string& fileName) { saveAs(fileName, XLForceOverwrite); }

std::string XLDocument::name() const
{
	size_t pos = m_filePath.find_last_of('/');
	if(pos != std::string::npos)
		return m_filePath.substr(pos + 1);
	else
		return m_filePath;
}

const std::string& XLDocument::path() const { return m_filePath; }
/**
 * @details Get a const pointer to the underlying XLWorkbook object.
 */
XLWorkbook XLDocument::workbook() const { return m_workbook; }
/**
 * @details Get the value for a property.
 */
std::string XLDocument::property(XLProperty prop) const
{
	switch(prop) {
		case XLProperty::Application: return m_appProperties.property("Application");
		case XLProperty::AppVersion: return m_appProperties.property("AppVersion");
		case XLProperty::Category: return m_coreProperties.property("cp:category");
		case XLProperty::Company: return m_appProperties.property("Company");
		case XLProperty::CreationDate: return m_coreProperties.property("dcterms:created");
		case XLProperty::Creator: return m_coreProperties.property("dc:creator");
		case XLProperty::Description: return m_coreProperties.property("dc:description");
		case XLProperty::DocSecurity: return m_appProperties.property("DocSecurity");
		case XLProperty::HyperlinkBase: return m_appProperties.property("HyperlinkBase");
		case XLProperty::HyperlinksChanged: return m_appProperties.property("HyperlinksChanged");
		case XLProperty::Keywords: return m_coreProperties.property("cp:keywords");
		case XLProperty::LastModifiedBy: return m_coreProperties.property("cp:lastModifiedBy");
		case XLProperty::LastPrinted: return m_coreProperties.property("cp:lastPrinted");
		case XLProperty::LinksUpToDate: return m_appProperties.property("LinksUpToDate");
		case XLProperty::Manager: return m_appProperties.property("Manager");
		case XLProperty::ModificationDate: return m_coreProperties.property("dcterms:modified");
		case XLProperty::ScaleCrop: return m_appProperties.property("ScaleCrop");
		case XLProperty::SharedDoc: return m_appProperties.property("SharedDoc");
		case XLProperty::Subject: return m_coreProperties.property("dc:subject");
		case XLProperty::Title: return m_coreProperties.property("dc:title");
		default: return ""; // To silence compiler warning.
	}
}
/**
 * @brief extract an integer major version v1 and minor version v2 from a string
 * @details trims all whitespaces from begin and end of versionString and attempts to extract two integers from the format
 * [0-9]{1,2}\.[0-9]{1,4}  (Example: 14.123)
 * @param versionString the string to process
 * @param majorVersion by reference: store the major version here
 * @param minorVersion by reference: store the minor version here
 * @return true if string adheres to format & version numbers could be extracted
 * @return false in case of failure
 */
bool getAppVersion(const std::string& versionString, int& majorVersion, int& minorVersion)
{
	// ===== const expressions for hardcoded version limits
	constexpr int minMajorV = 0, maxMajorV = 99;  // allowed value range for major version number
	constexpr int minMinorV = 0, maxMinorV = 9999; // "          for minor   "
	const size_t begin  = versionString.find_first_not_of(" \t");
	const size_t dotPos = versionString.find_first_of('.');
	// early failure if string is only blanks or does not contain a dot
	if(begin == std::string::npos || dotPos == std::string::npos)
		return false;
	const size_t end = versionString.find_last_not_of(" \t");
	if(begin != std::string::npos && dotPos != std::string::npos) {
		const std::string strMajorVersion = versionString.substr(begin, dotPos - begin);
		const std::string strMinorVersion = versionString.substr(dotPos + 1, end - dotPos);
		try {
			size_t pos;
			majorVersion = std::stoi(strMajorVersion, &pos);
			if(pos != strMajorVersion.length())  
				throw 1;
			minorVersion = std::stoi(strMinorVersion, &pos);
			if(pos != strMinorVersion.length())  
				throw 1;
		}
		catch(...) {
			return false; // conversion failed or did not convert the full string
		}
	}
	if(majorVersion < minMajorV || majorVersion > maxMajorV || minorVersion < minMinorV || minorVersion > maxMinorV) // final range check
		return false;
	return true;
}
/**
 * @details Set the value for a property.
 *
 * If the property is a datetime, it must be in the W3CDTF format, i.e. YYYY-MM-DDTHH:MM:SSZ. Also, the time should
 * be GMT. Creating a time point in this format can be done as follows:
 * ```cpp
 * #include <iostream>
 * #include <iomanip>
 * #include <ctime>
 * #include <sstream>
 *
 * // other stuff here
 *
 * std::time_t t = std::time(nullptr);
 * std::tm tm = *std::gmtime(&t);
 *
 * std::stringstream ss;
 * ss << std::put_time(&tm, "%FT%TZ");
 * auto datetime = ss.str();
 *
 * // datetime now is a string with the datetime in the right format.
 *
 * ```
 */
void XLDocument::setProperty(XLProperty prop, const std::string& value)
{
	switch(prop) {
		case XLProperty::Application:
		    m_appProperties.setProperty("Application", value);
		    break;
		case XLProperty::AppVersion: // ===== 2025-05-02 done: section cleaned up, pull request #174: Fixing discarding return value of
		                             // function with 'nodiscard' attribute
		    int minorVersion, majorVersion;
		    // ===== Check for the format "XX.XXXX", with X being a number.
		    if(!getAppVersion(value, majorVersion, minorVersion))  
				throw XLPropertyError("Invalid property value: " + std::string(value));
		    m_appProperties.setProperty("AppVersion",
			std::to_string(majorVersion) + "." +
			std::to_string(minorVersion)); // re-assemble version string for a clean property setting
		    break;
		case XLProperty::Category:
		    m_coreProperties.setProperty("cp:category", value);
		    break;
		case XLProperty::Company:
		    m_appProperties.setProperty("Company", value);
		    break;
		case XLProperty::CreationDate:
		    m_coreProperties.setProperty("dcterms:created", value);
		    break;
		case XLProperty::Creator:
		    m_coreProperties.setProperty("dc:creator", value);
		    break;
		case XLProperty::Description:
		    m_coreProperties.setProperty("dc:description", value);
		    break;
		case XLProperty::DocSecurity:
		    if(value == "0" || value == "1" || value == "2" || value == "4" || value == "8")
			    m_appProperties.setProperty("DocSecurity", value);
		    else
			    throw XLPropertyError("Invalid property value");
		    break;
		case XLProperty::HyperlinkBase:
		    m_appProperties.setProperty("HyperlinkBase", value);
		    break;
		case XLProperty::HyperlinksChanged:
		    if(value == "true" || value == "false")
			    m_appProperties.setProperty("HyperlinksChanged", value);
		    else
			    throw XLPropertyError("Invalid property value");
		    break;
		case XLProperty::Keywords:
		    m_coreProperties.setProperty("cp:keywords", value);
		    break;
		case XLProperty::LastModifiedBy:
		    m_coreProperties.setProperty("cp:lastModifiedBy", value);
		    break;
		case XLProperty::LastPrinted:
		    m_coreProperties.setProperty("cp:lastPrinted", value);
		    break;
		case XLProperty::LinksUpToDate:
		    if(value == "true" || value == "false")
			    m_appProperties.setProperty("LinksUpToDate", value);
		    else
			    throw XLPropertyError("Invalid property value");
		    break;
		case XLProperty::Manager:
		    m_appProperties.setProperty("Manager", value);
		    break;
		case XLProperty::ModificationDate:
		    m_coreProperties.setProperty("dcterms:modified", value);
		    break;
		case XLProperty::ScaleCrop:
		    if(value == "true" || value == "false")
			    m_appProperties.setProperty("ScaleCrop", value);
		    else
			    throw XLPropertyError("Invalid property value");
		    break;
		case XLProperty::SharedDoc:
		    if(value == "true" || value == "false")
			    m_appProperties.setProperty("SharedDoc", value);
		    else
			    throw XLPropertyError("Invalid property value");
		    break;
		case XLProperty::Subject:
		    m_coreProperties.setProperty("dc:subject", value);
		    break;
		case XLProperty::Title:
		    m_coreProperties.setProperty("dc:title", value);
		    break;
	}
}
/**
 * @details Delete a property
 */
void XLDocument::deleteProperty(XLProperty theProperty) { setProperty(theProperty, ""); }
XLDocument::operator bool() const { return m_archive.isValid(); }
bool XLDocument::isOpen() const { return this->operator bool(); }
/**
 * @details fetch a reference to m_styles
 */
XLStyles& XLDocument::styles() { return m_styles; }

/**
 * @details determine - without creation - whether the document contains a sheet relationships file for sheet with sheetXmlNo
 */
bool XLDocument::hasSheetRelationships(uint16 sheetXmlNo) const
{
	using namespace std::literals::string_literals;
	return m_archive.hasEntry("xl/worksheets/_rels/sheet"s + std::to_string(sheetXmlNo) + ".xml.rels"s);
}

/**
 * @details determine - without creation - whether the document contains a VML drawing file for sheet with sheetXmlNo
 */
bool XLDocument::hasSheetVmlDrawing(uint16 sheetXmlNo) const
{
	using namespace std::literals::string_literals;
	return m_archive.hasEntry("xl/drawings/vmlDrawing"s + std::to_string(sheetXmlNo) + ".vml"s);
}

/**
 * @details determine - without creation - whether the document contains a comments file for sheet with sheetXmlNo
 */
bool XLDocument::hasSheetComments(uint16 sheetXmlNo) const
{
	using namespace std::literals::string_literals;
	return m_archive.hasEntry("xl/comments"s + std::to_string(sheetXmlNo) + ".xml"s);
}

/**
 * @details determine - without creation - whether the document contains a table(s) file for sheet with sheetXmlNo
 */
bool XLDocument::hasSheetTables(uint16 sheetXmlNo) const
{
	using namespace std::literals::string_literals;
	return m_archive.hasEntry("xl/tables/table"s + std::to_string(sheetXmlNo) + ".xml"s);
}

/**
 * @details return an XLRelationships item for sheet with sheetXmlNo - create the underlying XML and add it to the archive if needed
 */
XLRelationships XLDocument::sheetRelationships(uint16 sheetXmlNo)
{
	using namespace std::literals::string_literals;
	std::string relsFilename = "xl/worksheets/_rels/sheet"s + std::to_string(sheetXmlNo) + ".xml.rels"s;

	if(!m_archive.hasEntry(relsFilename)) {
		// ===== Create the sheet relationships file within the archive
		m_archive.addEntry(relsFilename, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // empty XML file, class constructor will do the rest
		m_contentTypes.addOverride("/" + relsFilename, XLContentType::Relationships);               // add content types entry
	}
	constexpr const bool DO_NOT_THROW = true;
	XLXmlData * xmlData = getXmlData(relsFilename, DO_NOT_THROW);
	if(xmlData == nullptr) // if not yet managed: add the sheet relationships file to the managed files
		xmlData = &m_data.emplace_back(this, relsFilename, "", XLContentType::Relationships);

	return XLRelationships(xmlData, relsFilename);
}

/**
 * @details return an XLVmlDrawing item for sheet with sheetXmlNo - create the underlying XML and add it to the archive if needed
 */
XLVmlDrawing XLDocument::sheetVmlDrawing(uint16 sheetXmlNo)
{
	using namespace std::literals::string_literals;
	std::string vmlDrawingFilename = "xl/drawings/vmlDrawing"s + std::to_string(sheetXmlNo) + ".vml"s;

	if(!m_archive.hasEntry(vmlDrawingFilename)) {
		// ===== Create the sheet drawing file within the archive
		m_archive.addEntry(vmlDrawingFilename, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // empty XML file, class constructor will do the rest
		m_contentTypes.addOverride("/" + vmlDrawingFilename, XLContentType::VMLDrawing);                  // add content types entry
	}
	constexpr const bool DO_NOT_THROW = true;
	XLXmlData * xmlData = getXmlData(vmlDrawingFilename, DO_NOT_THROW);
	if(xmlData == nullptr) // if not yet managed: add the sheet drawing file to the managed files
		xmlData = &m_data.emplace_back(this, vmlDrawingFilename, "", XLContentType::VMLDrawing);

	return XLVmlDrawing(xmlData);
}

/**
 * @details return an XLComments item for sheet with sheetXmlNo - create the underlying XML and add it to the archive if needed
 */
XLComments XLDocument::sheetComments(uint16 sheetXmlNo)
{
	using namespace std::literals::string_literals;
	std::string commentsFilename = "xl/comments"s + std::to_string(sheetXmlNo) + ".xml"s;

	if(!m_archive.hasEntry(commentsFilename)) {
		// ===== Create the sheet comments file within the archive
		m_archive.addEntry(commentsFilename, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // empty XML file, class constructor will do the rest
		m_contentTypes.addOverride("/" + commentsFilename, XLContentType::Comments);                   // add content types entry
	}
	constexpr const bool DO_NOT_THROW = true;
	XLXmlData * xmlData = getXmlData(commentsFilename, DO_NOT_THROW);
	if(xmlData == nullptr) // if not yet managed: add the sheet comments file to the managed files
		xmlData = &m_data.emplace_back(this, commentsFilename, "", XLContentType::Comments);

	return XLComments(xmlData);
}

/**
 * @details return an XLTables item for sheet with sheetXmlNo - create the underlying XML and add it to the archive if needed
 */
XLTables XLDocument::sheetTables(uint16 sheetXmlNo)
{
	using namespace std::literals::string_literals;
	std::string tablesFilename = "xl/tables/table"s + std::to_string(sheetXmlNo) + ".xml"s;

	if(!m_archive.hasEntry(tablesFilename)) {
		// ===== Create the sheet tables file within the archive
		m_archive.addEntry(tablesFilename, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // empty XML file, class constructor will do the rest
		m_contentTypes.addOverride("/" + tablesFilename, XLContentType::Table);                        // add content types entry
	}
	constexpr const bool DO_NOT_THROW = true;
	XLXmlData * xmlData = getXmlData(tablesFilename, DO_NOT_THROW);
	if(xmlData == nullptr) // if not yet managed: add the sheet tables file to the managed files
		xmlData = &m_data.emplace_back(this, tablesFilename, "", XLContentType::Table);

	return XLTables(xmlData);
}

/**
 * @details Worksheet names cannot:
 *     Be blank.
 *     Contain more than 31 characters.
 *     Contain any of the following characters: / \ ? * : [ ]
 *     For example, 02/17/2016 would not be a valid worksheet name, but 02-17-2016 would work fine.
 *     Begin or end with an apostrophe ('), but they can be used in between text or numbers in a name.
 *     Be named "History". This is a reserved word Excel uses internally.
 */
constexpr const bool THROW_ON_INVALID = true;
bool XLDocument::validateSheetName(std::string sheetName, bool throwOnInvalid)
{
	using namespace std::literals::string_literals;
	bool valid = true;
	try {
		if(sheetName.length() > 31)
			throw "contain more than 31 characters"s;
		size_t pos = 0;
		while(sheetName[pos] == ' ' || sheetName[pos] == '\t')
			++pos;// aborts on sheetName[ sheetName.length() ], guaranteed to be \0
		if(pos == sheetName.length())
			throw "be blank"s;
		if(sheetName.front() == '\'' || sheetName.back() == '\'')
			throw "begin or end with an apostrophe (')"s;
		if(sheetName == "History")
			throw "be named \"History\" (Excel reserves this word for internal use)"s;
		for(pos = 0; pos < sheetName.length(); ++pos) {
			switch(sheetName[pos]) {
				// test for disallowed characters:
				case '/': [[fallthrough]];
				case '\\': [[fallthrough]];
				case '?': [[fallthrough]];
				case '*': [[fallthrough]];
				case ':': [[fallthrough]];
				case '[': [[fallthrough]];
				case ']':
				    throw "contain any of the following characters: / \\ ? * : [ ]"s;
				default:; // no-op
			}
		}
		// if execution gets here, sheetName is valid
	}
	catch(std::string const & err) {
		if(throwOnInvalid)
			throw XLInputError("Sheet name \""s + sheetName + "\" violates naming rules: sheet name can not "s + err);
		valid = false;
	}
	return valid;
}
/**
 * @details return value defaults to true, false only where the XLCommandType implements it
 */
bool XLDocument::execCommand(const XLCommand& command)
{
	switch(command.type()) {
		case XLCommandType::SetSheetName:
		    validateSheetName(command.getParam<std::string>("newName"), THROW_ON_INVALID);
		    m_appProperties.setSheetName(command.getParam<std::string>("sheetName"), command.getParam<std::string>("newName"));
		    m_workbook.setSheetName(command.getParam<std::string>("sheetID"), command.getParam<std::string>("newName"));
		    break;
		case XLCommandType::SetSheetColor:
		    // TODO: To be implemented
		    break;
		case XLCommandType::SetSheetVisibility:
		    m_workbook.setSheetVisibility(command.getParam<std::string>("sheetID"), command.getParam<std::string>("sheetVisibility"));
		    break;
		case XLCommandType::SetSheetIndex: 
			{
				XLQuery qry(XLQueryType::QuerySheetName);
				const auto sheetName = execQuery(qry.setParam("sheetID", command.getParam<std::string>("sheetID"))).result<std::string>();
				m_workbook.setSheetIndex(sheetName, command.getParam<uint16>("sheetIndex"));
			} 
			break;
		case XLCommandType::SetSheetActive: return m_workbook.setSheetActive(command.getParam<std::string>("sheetID"));
		case XLCommandType::ResetCalcChain: 
			{
				m_archive.deleteEntry("xl/calcChain.xml");
				const auto item = std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& theItem) {
					return theItem.getXmlPath() == "xl/calcChain.xml";
				});
				if(item != m_data.end())  
					m_data.erase(item);
			} 
			break;
		case XLCommandType::CheckAndFixCoreProperties: { // does nothing if core properties are in good shape
		    // ===== If _rels/.rels has no entry for docProps/core.xml
		    if(!m_docRelationships.targetExists("docProps/core.xml"))
			    m_docRelationships.addRelationship(XLRelationshipType::CoreProperties, "docProps/core.xml"); // Fix m_docRelationships

		    // ===== If docProps/core.xml is missing
		    if(!m_archive.hasEntry("docProps/core.xml"))
			    m_archive.addEntry("docProps/core.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // create empty docProps/core.xml
		    // ===== XLProperties constructor will take care of adding template content

		    // ===== If [Content Types].xml has no relationship for docProps/core.xml
		    if(!hasXmlData("docProps/core.xml")) {
			    m_contentTypes.addOverride("/docProps/core.xml", XLContentType::CoreProperties); // add content types entry
			    m_data.emplace_back(                                                    // store new entry in m_data
				    /* parentDoc */ this,
				    /* xmlPath   */ "docProps/core.xml",
				    /* xmlID     */ m_docRelationships.relationshipByTarget("docProps/core.xml").id(),
				    /* xmlType   */ XLContentType::CoreProperties);
		    }
	    } break;
		case XLCommandType::CheckAndFixExtendedProperties: { // does nothing if extended properties are in good shape
		    // ===== If _rels/.rels has no entry for docProps/app.xml
		    if(!m_docRelationships.targetExists("docProps/app.xml"))
			    m_docRelationships.addRelationship(XLRelationshipType::ExtendedProperties, "docProps/app.xml"); // Fix m_docRelationships

		    // ===== If docProps/app.xml is missing
		    if(!m_archive.hasEntry("docProps/app.xml"))
			    m_archive.addEntry("docProps/app.xml", "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"); // create empty docProps/app.xml
		    // ===== XLAppProperties constructor will take care of adding template content

		    // ===== If [Content Types].xml has no relationship for docProps/app.xml
		    if(!hasXmlData("docProps/app.xml")) {
			    m_contentTypes.addOverride("/docProps/app.xml", XLContentType::ExtendedProperties); // add content types entry
			    m_data.emplace_back(                                                       // store new entry in m_data
				    /* parentDoc */ this,
				    /* xmlPath   */ "docProps/app.xml",
				    /* xmlID     */ m_docRelationships.relationshipByTarget("docProps/app.xml").id(),
				    /* xmlType   */ XLContentType::ExtendedProperties);
		    }
	    } break;
		case XLCommandType::AddSharedStrings: {
		    m_contentTypes.addOverride("/xl/sharedStrings.xml", XLContentType::SharedStrings);
		    m_wbkRelationships.addRelationship(XLRelationshipType::SharedStrings, "sharedStrings.xml");
		    // ===== Add empty archive entry for shared strings, XLSharedStrings constructor will create a default document when no document element is found
		    m_archive.addEntry("xl/sharedStrings.xml", "");
	    } break;
		case XLCommandType::AddWorksheet: {
		    validateSheetName(command.getParam<std::string>("sheetName"), THROW_ON_INVALID);
		    const std::string emptyWorksheet {
			    "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
			    "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
			    " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
			    " xmlns:mc=\"http://schemas.openxmlformats.org/markup-compatibility/2006\" mc:Ignorable=\"x14ac\""
			    " xmlns:x14ac=\"http://schemas.microsoft.com/office/spreadsheetml/2009/9/ac\">"
			    "<dimension ref=\"A1\"/>"
			    "<sheetViews>"
			    "<sheetView workbookViewId=\"0\"/>"
			    "</sheetViews>"
			    "<sheetFormatPr baseColWidth=\"10\" defaultRowHeight=\"16\" x14ac:dyDescent=\"0.2\"/>"
			    "<sheetData/>"
			    "<pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" header=\"0.3\" footer=\"0.3\"/>"
			    "</worksheet>"
		    };
		    m_contentTypes.addOverride(command.getParam<std::string>("sheetPath"), XLContentType::Worksheet);
		    m_wbkRelationships.addRelationship(XLRelationshipType::Worksheet, command.getParam<std::string>("sheetPath").substr(4));
		    m_appProperties.appendSheetName(command.getParam<std::string>("sheetName"));
		    m_archive.addEntry(command.getParam<std::string>("sheetPath").substr(1), emptyWorksheet);
		    m_data.emplace_back(
			    /* parentDoc */ this,
			    /* xmlPath   */ command.getParam<std::string>("sheetPath").substr(1),
			    /* xmlID     */ m_wbkRelationships.relationshipByTarget(command.getParam<std::string>("sheetPath").substr(4)).id(),
			    /* xmlType   */ XLContentType::Worksheet);
	    } break;
		case XLCommandType::AddChartsheet:
		    // TODO: To be implemented
		    break;
		case XLCommandType::DeleteSheet: {
		    m_appProperties.deleteSheetName(command.getParam<std::string>("sheetName"));
		    std::string sheetPath = m_wbkRelationships.relationshipById(command.getParam<std::string>("sheetID")).target();
		    if(sheetPath.substr(0, 4) != "/xl/")  sheetPath = "/xl/" + sheetPath;// 2024-12-15: respect absolute sheet path
		    m_archive.deleteEntry(sheetPath.substr(1));
		    m_contentTypes.deleteOverride(sheetPath);
		    m_wbkRelationships.deleteRelationship(command.getParam<std::string>("sheetID"));
		    m_data.erase(std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& item) {
				return item.getXmlPath() == sheetPath.substr(1);
			}));
	    } break;
		case XLCommandType::CloneSheet: {
		    validateSheetName(command.getParam<std::string>("cloneName"), THROW_ON_INVALID);
		    const auto internalID = m_workbook.createInternalSheetID();
		    const auto sheetPath  = "/xl/worksheets/sheet" + std::to_string(internalID) + ".xml";
		    if(m_workbook.sheetExists(command.getParam<std::string>("cloneName")))
			    throw XLInternalError("Sheet named \"" + command.getParam<std::string>("cloneName") + "\" already exists.");

		    // ===== 2024-12-15: handle absolute sheet path: ensure relative sheet path
		    std::string sheetToClonePath = m_wbkRelationships.relationshipById(command.getParam<std::string>("sheetID")).target();
		    if(sheetToClonePath.substr(0, 4) == "/xl/")  
				sheetToClonePath = sheetToClonePath.substr(4);
		    if(m_wbkRelationships.relationshipById(command.getParam<std::string>("sheetID")).type() == XLRelationshipType::Worksheet) {
			    m_contentTypes.addOverride(sheetPath, XLContentType::Worksheet);
			    m_wbkRelationships.addRelationship(XLRelationshipType::Worksheet, sheetPath.substr(4));
			    m_appProperties.appendSheetName(command.getParam<std::string>("cloneName"));
			    m_archive.addEntry(sheetPath.substr(1),
				std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& data) {
					return data.getXmlPath().substr(3) == sheetToClonePath; // 2024-12-15: ensure relative sheet path
				})->getRawData());
			    m_data.emplace_back(
				    /* parentDoc */ this,
				    /* xmlPath   */ sheetPath.substr(1),
				    /* xmlID     */ m_wbkRelationships.relationshipByTarget(sheetPath.substr(4)).id(),
				    /* xmlType   */ XLContentType::Worksheet);
		    }
		    else {
			    m_contentTypes.addOverride(sheetPath, XLContentType::Chartsheet);
			    m_wbkRelationships.addRelationship(XLRelationshipType::Chartsheet, sheetPath.substr(4));
			    m_appProperties.appendSheetName(command.getParam<std::string>("cloneName"));
			    m_archive.addEntry(sheetPath.substr(1),
				std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& data) {
					return data.getXmlPath().substr(3) == sheetToClonePath; // 2024-12-15: ensure relative sheet path
				})->getRawData());
			    m_data.emplace_back(
				    /* parentDoc */ this,
				    /* xmlPath   */ sheetPath.substr(1),
				    /* xmlID     */ m_wbkRelationships.relationshipByTarget(sheetPath.substr(4)).id(),
				    /* xmlType   */ XLContentType::Chartsheet);
		    }

		    m_workbook.prepareSheetMetadata(command.getParam<std::string>("cloneName"), internalID);
	    } break;
		case XLCommandType::AddStyles: {
		    m_contentTypes.addOverride("/xl/styles.xml", XLContentType::Styles);
		    m_wbkRelationships.addRelationship(XLRelationshipType::Styles, "styles.xml");
		    // ===== Add empty archive entry for styles, XLStyles constructor will create a default document when no document element is found
		    m_archive.addEntry("xl/styles.xml", "");
	    } break;
	}

	return true; // default: command claims success unless otherwise implemented in switch-clauses
}

XLQuery XLDocument::execQuery(const XLQuery& query) const
{
	switch(query.type()) {
		case XLQueryType::QuerySheetName:
		    return XLQuery(query).setResult(m_workbook.sheetName(query.getParam<std::string>("sheetID")));

		case XLQueryType::QuerySheetIndex: { // 2025-01-13: implemented query - previously no index was determined at all
		    std::string queriedSheetName = m_workbook.sheetName(query.getParam<std::string>("sheetID"));
		    for( uint16 sheetIndex = 1; sheetIndex <= workbook().sheetCount(); ++sheetIndex ) {
			    if(workbook().sheet(sheetIndex).name() == queriedSheetName)
				    return XLQuery(query).setResult(std::to_string(sheetIndex));
		    }

		    { // if loop failed to locate queriedSheetName:
			    using namespace std::literals::string_literals;
			    throw XLInternalError("Could not determine a sheet index for sheet named \"" + queriedSheetName + "\"");
		    }
	    }
		case XLQueryType::QuerySheetVisibility:
		    return XLQuery(query).setResult(m_workbook.sheetVisibility(query.getParam<std::string>("sheetID")));

		case XLQueryType::QuerySheetType: {
		    const XLRelationshipType t = m_wbkRelationships.relationshipById(query.getParam<std::string>("sheetID")).type();
		    if(t == XLRelationshipType::Worksheet)
			    return XLQuery(query).setResult(XLContentType::Worksheet);
		    if(t == XLRelationshipType::Chartsheet)
			    return XLQuery(query).setResult(XLContentType::Chartsheet);
		    return XLQuery(query).setResult(XLContentType::Unknown);
	    }
		case XLQueryType::QuerySheetIsActive:
		    return XLQuery(query).setResult(m_workbook.sheetIsActive(query.getParam<std::string>("sheetID")));
		case XLQueryType::QuerySheetID:
		    return XLQuery(query).setResult(m_workbook.sheetVisibility(query.getParam<std::string>("sheetID")));
		case XLQueryType::QuerySheetRelsID:
		    return XLQuery(query).setResult(m_wbkRelationships.relationshipByTarget(query.getParam<std::string>("sheetPath").substr(4)).id());
		case XLQueryType::QuerySheetRelsTarget:
		    // ===== 2024-12-15: XLRelationshipItem::target() returns the unmodified Relationship "Target" property
		    //                     - can be absolute or relative and must be handled by the caller
		    //                   The only invocation as of today is in XLWorkbook::sheet(const std::string& sheetName) and handles this
		    return XLQuery(query).setResult(m_wbkRelationships.relationshipById(query.getParam<std::string>("sheetID")).target());
		case XLQueryType::QuerySharedStrings:
		    return XLQuery(query).setResult(m_sharedStrings);
		case XLQueryType::QueryXmlData: {
		    const auto result = std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& item) {
				return item.getXmlPath() == query.getParam<std::string>("xmlPath");
			});
		    if(result == m_data.end())
			    throw XLInternalError("Path does not exist in zip archive (" + query.getParam<std::string>("xmlPath") + ")");
		    return XLQuery(query).setResult(&*result);
	    }
		default:
		    throw XLInternalError("XLDocument::execQuery: unknown query type " + std::to_string(static_cast<uint8>(query.type())));
	}
	return query; // Needed in order to suppress compiler warning
}

XLQuery XLDocument::execQuery(const XLQuery& query) { return static_cast<const XLDocument&>(*this).execQuery(query); }
/**
 * @details assign savingDeclaration to m_xmlSavingDeclaration
 */
void XLDocument::setSavingDeclaration(XLXmlSavingDeclaration const& savingDeclaration) { m_xmlSavingDeclaration = savingDeclaration; }
/**
 * @details iterate over all worksheets, all rows, all columns and re-create the shared strings table in that order based on first use
 */
void XLDocument::cleanupSharedStrings()
{
	int32_t oldStringCount = m_sharedStringCache.size();
	std::vector< int32_t > indexMap(oldStringCount, -1);  // indexMap[ oldIndex ] :== newIndex, -1 = not yet assigned
	int32_t newStringCount = 1; // reserve index 0 for empty string, count here +1 for each unique shared string index that is in use in the worksheet
	uint worksheetCount = m_workbook.worksheetCount();
	for(uint16 wIndex = 1; wIndex <= worksheetCount; ++wIndex) {
		XLWorksheet wks = m_workbook.worksheet(wIndex);
		XLCellRange cellRange = wks.range();
		for(XLCellIterator cellIt = cellRange.begin(); cellIt != cellRange.end(); ++cellIt) {
			if(!cellIt.cellExists())  
				continue;// prevent cell creation by access for non-existing cells
			// ===== Cell exists: check for shared strings & update index as needed
			XLCell& cell = *cellIt;
			if(cell.value().type() == XLValueType::String) {
				XLCellValueProxy val = cell.value();
				int32_t si = val.stringIndex();
				if(indexMap[si] == -1) { // shared string was not yet flagged as "in use"
					if(m_sharedStringCache[si].length() > 0) // if shared string is not empty
						indexMap[si] = newStringCount++; // add this shared string to the end of the new cache being rewritten and increment the counter
					else                   // else
						indexMap[si] = 0; // assign the hardcoded index 0 reserved for the empty string in newStringCache
				}
				if(indexMap[si] != si) // if the index changed
					val.setStringIndex(indexMap[si]); // then update it for the cell
			}
		}
	}

	// ===== After all cells have been reindexed, newStringCount is now the exact amount of remaining strings,
	//        and indexMap now contains the mapping to applied for reindexing.

	// ===== Create a new shared strings cache.
	std::vector<std::string> newStringCache(newStringCount); // store the re-indexed strings here
	// NOTE: newStringCache is vector because m_sharedStringCache may eventually be changed from std::deque to something else (std::map) for performance
	newStringCache[0] = ""; // store empty string in first position
	for(int32_t oldIdx = 0; oldIdx < oldStringCount; ++oldIdx) { // "steal" all std::strings that are still in use from existing string cache
		if(int32_t newIdx = indexMap[oldIdx]; newIdx > 0)    // if string is still in use
			newStringCache[newIdx] = std::move(m_sharedStringCache[oldIdx]); // NOTE: std::move invalidates the shared string cache -> not thread safe
	}
	m_sharedStringCache.clear(); // TBD: is this safe with strings that were std::move assigned to newStringCache?
	// refill m_sharedStringCache cache from newStringCache
	std::move(newStringCache.begin(), newStringCache.end(), std::back_inserter(m_sharedStringCache));
	if(static_cast<int32_t>(newStringCache.size()) != m_sharedStrings.rewriteXmlFromCache())
		throw XLInternalError("XLDocument::cleanupSharedStrings: failed to rewrite shared string table - document would be corrupted");
}

std::string XLDocument::extractXmlFromArchive(const std::string& path)
{
	return (m_archive.hasEntry(path) ? m_archive.getEntry(path) : "");
}

XLXmlData* XLDocument::getXmlData(const std::string& path, bool doNotThrow)
{
	// avoid duplication of code: use const_cast to invoke the const function overload and return a non-const value
	return const_cast<XLXmlData *>(const_cast<XLDocument const *>(this)->getXmlData(path, doNotThrow));
}

const XLXmlData* XLDocument::getXmlData(const std::string& path, bool doNotThrow) const
{
	std::list<XLXmlData>::iterator result = std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& item) {
		return item.getXmlPath() == path;
	});
	if(result == m_data.end()) {
		if(doNotThrow)  
			return nullptr;// use with caution
		else 
			throw XLInternalError("Path " + path + " does not exist in zip archive.");
	}
	return &*result;
}

bool XLDocument::hasXmlData(const std::string& path) const
{
	return std::find_if(m_data.begin(), m_data.end(), [&](const XLXmlData& item) {
		return item.getXmlPath() == path;
	}) != m_data.end();
}

namespace OpenXLSX {
	//
	// Global utility functions
	//
	/**
	 * @brief Return a hexadecimal digit as character that is the equivalent of value
	 * @param value The number to convert, must be 0 <= value <= 15
	 * @return 0 if value > 15, otherwise the hex digit equivalent to value, as a character
	 */
	char hexDigit(uint value)
	{
		if(value > 0xf)
			return 0;
		if(value < 0xa)
			return static_cast<char>(value + '0'); // return value as number digit
		return static_cast<char>((value - 0xa) + 'a'); // return value as letter digit
	}
	/**
	 * @details create a hex string from data - this function does the
	 */
	std::string BinaryAsHexString(const void * data, const size_t size)
	{
		// ===== Allocate memory for string assembly - each byte takes two hex digits = 2 characters in string
		std::string strAssemble(size * 2, 0); // zero-initialize (alternative would be to default-construct a string and .reserve(size * 2);

		const uint8 * dataBytePtr = reinterpret_cast< const uint8 * >( data );
		// ===== assemble a string of hex digits
		for(size_t pos = 0; pos < size * 2; ++pos) {
			int valueByte = dataBytePtr[pos / 2];
			int valueHalfByte = (valueByte & (pos & 1 ? 0x0f : 0xf0)) >> (pos & 1 ? 0 : 4);
			strAssemble[pos] = hexDigit(valueHalfByte); // convert each half-byte into a hex digit
		}
		return strAssemble;
	}

	/**
	 * @details apply the XLSX password hashing algorithm to password
	 */
	uint16 ExcelPasswordHash(std::string password)
	{
		uint16 wPasswordHash = 0;
		uint16 cchPassword = static_cast<uint16>(password.length());
		for(uint16 pos = 0; pos < cchPassword; ++pos) {
			uint32 byteHash = password[pos] << ((pos + 1) % 15);
			byteHash = (byteHash >> 15) | (byteHash & 0x7fff);
			wPasswordHash ^= static_cast<uint16>(byteHash);
		}
		wPasswordHash ^= cchPassword ^ 0xce4b;
		// wPasswordHash ^= (0x8000 | ('N' << 8) | 'K'); // 'N' = 0x4e, 'K' = 0x4b, 0x8000 | 0x4e000 | 0x004b == 0xce4b, XOR'ed above

		return wPasswordHash;
	}

	/**
	 * @details same as ExcelPasswordHash but return the result as a hex string
	 */
	std::string ExcelPasswordHashAsString(std::string password)
	{
		uint16 pw = ExcelPasswordHash(password);
		uint8 hashData[2];
		hashData[0] = pw >> 8;   // MSB first
		hashData[1] = pw & 0xff; // LSB second
		return BinaryAsHexString(hashData, 2);
	}
	/**
	 * @brief local function: split a path into a vector of strings each containing a subdirectory (or finally: a filename) - ignore leading and trailing slashes
	 * @param path split this path by '/' characters
	 * @param eliminateDots if true (default), will ignore "." entries and will pop a subdirectory from the vector for each ".." entry
	 * @return a vector of non-empty subdirectories
	 * @throw XLInternalError upon invalid path - e.g. containing "//" or trying to escape via ".." beyond the context of path
	 */
	constexpr const bool DISASSEMBLE_PATH_ELIMINATE_DOTS = true;      // helper constants for code readability
	constexpr const bool DISASSEMBLE_PATH_KEEP_DOTS      = false;     //
	std::vector<std::string> disassemblePath(std::string const& path, bool eliminateDots = DISASSEMBLE_PATH_ELIMINATE_DOTS)
	{
		std::vector< std::string > result;
		size_t startpos = (path[ 0 ] == '/' ? 1 : 0); // skip a leading slash
		size_t pos;
		do {
			pos = path.find('/', startpos);
			if(pos == startpos)
				throw XLInternalError("eliminateDotAndDotDotFromPath: path must not contain two subsequent forward slashes");
			else {
				std::string dirEntry = path.substr(startpos, pos - startpos); // get folder name
				if(dirEntry.length() > 0) {
					if(eliminateDots) {
						// handle . and .. folders
						if(dirEntry == ".") {
						} // no-op
						else if(dirEntry == "..") {
							if(result.size() > 0)
								result.pop_back(); // remove previous folder from result
							else 
								throw std::string("eliminateDotAndDotDotFromPath: no remaining directory to exit with ..");
						}
						else
							result.push_back(dirEntry);
					}
					else 
						result.push_back(dirEntry);
				}
				startpos = pos + 1;
			}
		} while(pos != std::string::npos);
		return result;
	}

	std::string getPathARelativeToPathB(std::string const& pathA, std::string const& pathB)
	{
		size_t startpos = 0;
		while(pathA[startpos] == pathB[startpos])
			++startpos;          // find position where pathA and pathB differ
		while(startpos > 0 && pathA[startpos - 1] != '/')
			--startpos;  // then iterate back to last slash before that position
		if(startpos == 0)
			throw XLInternalError("getPathARelativeToPathB: pathA and pathB have no common beginning");
		std::vector<std::string> dirEntriesB = disassemblePath(pathB.substr(startpos));   // disassemble unique part of pathB into a vector of strings
		if(dirEntriesB.size() > 0 && pathB.back() != '/')
			dirEntriesB.pop_back();        // a filename in pathB isn't needed for the relative path generation
		std::string result("");                                                           // assemble result:
		for(auto it = dirEntriesB.rbegin(); it != dirEntriesB.rend(); ++it)               // for each subdirectory unique to pathB
			result += "../";                                                             // add one ../ to escape it
		result += pathA.substr(startpos);                                                 // finally, append unique part of pathA
		return result;
	}

	std::string eliminateDotAndDotDotFromPath(const std::string& path)
	{
		std::vector <std::string> dirEntries = disassemblePath(path);   // disassemble path into a vector of strings with subdirectory names
		// assemble path from dirEntries
		std::string result = path.front() == '/' ? "/" : "";
		if(dirEntries.size() > 0) {
			auto it = dirEntries.begin();
			result += *it;
			while(++it != dirEntries.end() ) {
				result += "/" + *it;
			}                                                      // concatenate dirnames
		}
		// in return value: avoid appending a trailing slash if a path was already reduced to "/"
		return ((result.length() > 1 || result.front() != '/') && path.back() == '/') ? result + "/" : result;
	}
} // namespace OpenXLSX
//
// XLDrawing.cpp
//
namespace OpenXLSX {
	const std::string ShapeNodeName = "v:shape";
	const std::string ShapeTypeNodeName = "v:shapetype";

	// utility functions

	/**
	 * @brief test if v:shapetype attribute id already exists at begin of file
	 * @param rootNode the parent that holds all v:shapetype nodes to be compared
	 * @param shapeTypeNode the node to check for a duplicate id
	 * @return true if node would be a duplicate (and can be deleted), false if node is unique (so far)
	 */
	bool wouldBeDuplicateShapeType(XMLNode const& rootNode, XMLNode const& shapeTypeNode)
	{
		std::string id = shapeTypeNode.attribute("id").value();
		XMLNode node = rootNode.first_child_of_type(pugi::node_element);
		using namespace std::literals::string_literals;
		while(node != shapeTypeNode && node.raw_name() == ShapeTypeNodeName) {  // do not compare shapeTypeNode with itself, and abort on first non-shapetype node
			if(node.attribute("id").value() == id)  return true;           // found an existing shapetype with the same id
			node = node.next_sibling_of_type(pugi::node_element);
		}
		return false;
	}

	/**
	 * @brief move an xml node to a new position within the same root node
	 * @param rootNode the parent that can perform inserts / deletes
	 * @param node the node to move
	 * @param insertAfter the reference for the new location, if insertAfter.empty(): move node to first position in rootNode
	 * @param withWhitespaces if true, move all node_pcdata nodes preceeding node, together with the node
	 * @return the node as inserted at the destination - empty node if insertion can't be performed (rootNode or node are empty)
	 */
	XMLNode moveNode(XMLNode& rootNode, XMLNode& node, XMLNode const& insertAfter, bool withWhitespaces = true)
	{
		if(rootNode.empty() || node.empty())  return XMLNode{}; // can't perform move
		if(node == insertAfter || node == insertAfter.next_sibling())  return node; // nothing to do

		XMLNode newNode{};
		if(insertAfter.empty())
			newNode = rootNode.prepend_copy(node);
		else
			newNode = rootNode.insert_copy_after(node, insertAfter);
		if(withWhitespaces) {
			copyLeadingWhitespaces(rootNode, node, newNode);        // copy leading whitespaces
			while(node.previous_sibling().type() == pugi::node_pcdata) // then remove whitespaces that were just copied to newNode
				rootNode.remove_child(node.previous_sibling());
		}
		rootNode.remove_child(node); // remove node that was just copied to new location
		return newNode;
	}

	XLShapeTextVAlign XLShapeTextVAlignFromString(std::string const& vAlignString)
	{
		if(vAlignString == "Center")
			return XLShapeTextVAlign::Center;
		if(vAlignString == "Top")     
			return XLShapeTextVAlign::Top;
		if(vAlignString == "Bottom")  
			return XLShapeTextVAlign::Bottom;
		std::cerr << __func__ << ": invalid XLShapeTextVAlign setting" << vAlignString << std::endl;
		return XLShapeTextVAlign::Invalid;
	}

	std::string XLShapeTextVAlignToString(XLShapeTextVAlign vAlign)
	{
		switch(vAlign) {
			case XLShapeTextVAlign::Center:  return "Center";
			case XLShapeTextVAlign::Top:     return "Top";
			case XLShapeTextVAlign::Bottom:  return "Bottom";
			case XLShapeTextVAlign::Invalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLShapeTextHAlign XLShapeTextHAlignFromString(std::string const& hAlignString)
	{
		if(hAlignString == "Left")    
			return XLShapeTextHAlign::Left;
		if(hAlignString == "Right")   
			return XLShapeTextHAlign::Right;
		if(hAlignString == "Center")  
			return XLShapeTextHAlign::Center;
		std::cerr << __func__ << ": invalid XLShapeTextHAlign setting" << hAlignString << std::endl;
		return XLShapeTextHAlign::Invalid;
	}

	std::string XLShapeTextHAlignToString(XLShapeTextHAlign hAlign)
	{
		switch(hAlign) {
			case XLShapeTextHAlign::Left:    return "Left";
			case XLShapeTextHAlign::Right:   return "Right";
			case XLShapeTextHAlign::Center:  return "Center";
			case XLShapeTextHAlign::Invalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}
} // namespace OpenXLSX

XLShapeClientData::XLShapeClientData() : m_clientDataNode(std::make_unique<XMLNode>())
{
}

XLShapeClientData::XLShapeClientData(const XMLNode& node) : m_clientDataNode(std::make_unique<XMLNode>(node))
{
}
/**
 * @details XLShapeClientData getter functions
 */
bool elementTextAsBool(XMLNode const& node) 
{
	if(node.empty())  
		return false;      // node doesn't exist: false
	if(node.text().empty())  
		return true;// node exists but has no setting: true
	char c = node.text().get()[0];       // node exists and has a setting:
	if(c == 't' || c == 'T')  
		return true;//   check true setting on first letter only
	return false;                        //   otherwise return false
}

std::string XLShapeClientData::objectType()    const { return appendAndGetAttribute(*m_clientDataNode, "ObjectType", "Note").value(); }
bool XLShapeClientData::moveWithCells() const { return elementTextAsBool(m_clientDataNode->child("x:MoveWithCells")); }
bool XLShapeClientData::sizeWithCells() const { return elementTextAsBool(m_clientDataNode->child("x:SizeWithCells")); }
std::string XLShapeClientData::anchor()        const { return m_clientDataNode->child("x:Anchor").value(); }
bool XLShapeClientData::autoFill()      const { return elementTextAsBool(m_clientDataNode->child("x:AutoFill")); }
XLShapeTextVAlign XLShapeClientData::textVAlign() const 
{
	return XLShapeTextVAlignFromString(m_clientDataNode->child("x:TextVAlign").text().get());
	// XMLNode vAlign = m_clientDataNode->child("x:TextVAlign");
	// if (vAlign.empty()) return XLDefaultShapeTextVAlign;
	// return XLShapeTextVAlignFromString(vAlign.text().get());
}

XLShapeTextHAlign XLShapeClientData::textHAlign() const 
{
	return XLShapeTextHAlignFromString(m_clientDataNode->child("x:TextHAlign").text().get());
	// XMLNode hAlign = m_clientDataNode->child("x:TextHAlign");
	// if (hAlign.empty()) return XLDefaultShapeTextHAlign;
	// return XLShapeTextHAlignFromString(hAlign.text().get());
}

uint32 XLShapeClientData::row() const 
{
	return appendAndGetNode(*m_clientDataNode, "x:Row",    m_nodeOrder, XLForceNamespace).text().as_uint(0);
}

uint16 XLShapeClientData::column() const 
{
	return static_cast<uint16>(appendAndGetNode(*m_clientDataNode, "x:Column", m_nodeOrder, XLForceNamespace).text().as_uint(0));
}
/**
 * @details XLShapeClientData setter functions
 */
bool XLShapeClientData::setObjectType(std::string newObjectType) { return appendAndSetAttribute(*m_clientDataNode, "ObjectType", newObjectType).empty() == false; }
bool XLShapeClientData::setMoveWithCells(bool set)
{
	return appendAndGetNode(*m_clientDataNode, "x:MoveWithCells", m_nodeOrder, XLForceNamespace).text().set(set ? "" : "False");
	// XMLNode moveWithCellsNode = appendAndGetNode(*m_clientDataNode, "x:MoveWithCells", m_nodeOrder, XLForceNamespace);
	// if (moveWithCellsNode.empty()) return false;
	// return moveWithCellsNode.text().set(set ? "True" : "False");
}

bool XLShapeClientData::setSizeWithCells(bool set)
{ return appendAndGetNode(*m_clientDataNode, "x:SizeWithCells", m_nodeOrder, XLForceNamespace).text().set(set ? "" : "False"); }

bool XLShapeClientData::setAnchor(std::string newAnchor)
{ return appendAndGetNode(*m_clientDataNode, "x:Anchor",        m_nodeOrder, XLForceNamespace).text().set(newAnchor.c_str()); }

bool XLShapeClientData::setAutoFill(bool set)
{ return appendAndGetNode(*m_clientDataNode, "x:AutoFill",      m_nodeOrder, XLForceNamespace).text().set(set ? "True" : "False"); }

bool XLShapeClientData::setTextVAlign(XLShapeTextVAlign newTextVAlign)
{ return appendAndGetNode(*m_clientDataNode, "x:TextVAlign",    m_nodeOrder, XLForceNamespace).text().set(XLShapeTextVAlignToString(newTextVAlign).c_str()); }

bool XLShapeClientData::setTextHAlign(XLShapeTextHAlign newTextHAlign)
{ return appendAndGetNode(*m_clientDataNode, "x:TextHAlign",    m_nodeOrder, XLForceNamespace).text().set(XLShapeTextHAlignToString(newTextHAlign).c_str()); }

bool XLShapeClientData::setRow(uint32 newRow)
{ return appendAndGetNode(*m_clientDataNode, "x:Row",           m_nodeOrder, XLForceNamespace).text().set(newRow); }

bool XLShapeClientData::setColumn(uint16 newColumn)
{ return appendAndGetNode(*m_clientDataNode, "x:Column",        m_nodeOrder, XLForceNamespace).text().set(newColumn); }

// ========== XLShape Member Functions

XLShapeStyle::XLShapeStyle() : m_style(""), m_styleAttribute(std::make_unique<XMLAttribute>())
{
	setPosition("absolute");
	setMarginLeft(100);
	setMarginTop(0);
	setWidth(80);
	setHeight(50);
	setMsoWrapStyle("none");
	setVTextAnchor("middle");
	hide();
}

XLShapeStyle::XLShapeStyle(const std::string& styleAttribute) : m_style(styleAttribute), m_styleAttribute(std::make_unique<XMLAttribute>())
{
}

XLShapeStyle::XLShapeStyle(const XMLAttribute& styleAttribute) : m_style(""), m_styleAttribute(std::make_unique<XMLAttribute>(styleAttribute))
{
}
/**
 * @details find attributeName in m_nodeOrder, then return index
 */
int16 XLShapeStyle::attributeOrderIndex(std::string const& attributeName) const
{
// std::string m_style{"position:absolute;margin-left:100pt;margin-top:0pt;width:50pt;height:50.0pt;mso-wrap-style:none;v-text-anchor:middle;visibility:hidden"};
	auto attributeIterator = std::find(m_nodeOrder.begin(), m_nodeOrder.end(), attributeName);
	if(attributeIterator == m_nodeOrder.end())
		return -1;
	return static_cast<int16>(attributeIterator - m_nodeOrder.begin());
}

XLShapeStyleAttribute XLShapeStyle::getAttribute(std::string const& attributeName, std::string const& valIfNotFound) const
{
	if(attributeOrderIndex(attributeName) == -1) {
		using namespace std::literals::string_literals;
		throw XLInternalError("XLShapeStyle.getAttribute: attribute "s + attributeName + " is not defined in class"s);
	}
	// if attribute is linked, re-read m_style each time in case the underlying XML has changed
	if(not m_styleAttribute->empty())  
		m_style = std::string(m_styleAttribute->value());
	size_t lastPos = 0;
	XLShapeStyleAttribute result;
	result.name = ""; // indicates "not found"
	result.value = ""; // default in case attribute name is found but has no value
	do {
		size_t pos = m_style.find(';', lastPos);
		std::string attrPair = m_style.substr(lastPos, pos - lastPos);
		if(attrPair.length() > 0) {
			size_t separatorPos = attrPair.find(':');
			if(attributeName == attrPair.substr(0, separatorPos)) { // found!
				result.name = attributeName;
				if(separatorPos != std::string::npos)
					result.value = attrPair.substr(separatorPos + 1);
				break;
			}
		}
		lastPos = pos+1;
	} while(lastPos < m_style.length());
	if(lastPos >= m_style.length()) // attribute was not found
		result.value = valIfNotFound; // -> return default value
	return result;
}

bool XLShapeStyle::setAttribute(std::string const& attributeName, std::string const& attributeValue)
{
	int16 attrIndex = attributeOrderIndex(attributeName);
	if(attrIndex == -1) {
		using namespace std::literals::string_literals;
		throw XLInternalError("XLShapeStyle.setAttribute: attribute "s + attributeName + " is not defined in class"s);
	}

	// if attribute is linked, re-read m_style each time in case the underlying XML has changed
	if(not m_styleAttribute->empty())  m_style = std::string(m_styleAttribute->value());

	size_t lastPos = 0;
	size_t appendPos = 0;
	do {
		size_t pos = m_style.find(';', lastPos);

		std::string attrPair = m_style.substr(lastPos, pos - lastPos);
		if(attrPair.length() > 0) {
			size_t separatorPos = attrPair.find(':');
			int16 thisAttrIndex = attributeOrderIndex(attrPair.substr(0, separatorPos));
			if(thisAttrIndex >= attrIndex) { // can insert or update
				appendPos = (thisAttrIndex == attrIndex ? pos : lastPos); // if match: append from following attribute, if not found, append from current (insert)
				break;
			}
		}
		if(pos == std::string::npos)
			lastPos = pos;
		else
			lastPos = pos + 1;
	} while(lastPos < m_style.length());
	if(lastPos >= m_style.length() || appendPos > m_style.length()) { // if attribute needs to be appended or was found at last position
		appendPos = m_style.length();                             // then nothing remains from m_style to follow (appended) attribute

		// for semi-colon logic, lastPos needs to be capped at string length (so that it can be 0 for empty string)
		if(lastPos > m_style.length())  lastPos = m_style.length();
	}

	using namespace std::literals::string_literals;
	m_style = m_style.substr(0, lastPos) + ((lastPos != 0 && appendPos == m_style.length()) ? ";"s : ""s) // prepend ';' if attribute is appended to non-empty string
	    /**/      + attributeName + ":"s + attributeValue                                               // insert attribute:value pair
	    /**/      + (appendPos < m_style.length() ? ";"s : ""s) + m_style.substr(appendPos);            // append ';' if attribute is inserted before other data

	// if attribute is linked, update it with the new style value
	if(not m_styleAttribute->empty())  m_styleAttribute->set_value(m_style.c_str());

	return true;
}

/**
 * @details XLShapeStyle getter functions
 */
std::string XLShapeStyle::position() const { return getAttribute("position").value; }
uint16 XLShapeStyle::marginLeft() const { return static_cast<uint16>(std::stoi(getAttribute("margin-left").value)); }
uint16 XLShapeStyle::marginTop() const { return static_cast<uint16>(std::stoi(getAttribute("margin-top").value)); }
uint16 XLShapeStyle::width() const { return static_cast<uint16>(std::stoi(getAttribute("width").value)); }
uint16 XLShapeStyle::height() const { return static_cast<uint16>(std::stoi(getAttribute("height").value)); }
std::string XLShapeStyle::msoWrapStyle() const { return getAttribute("mso-wrap-style").value; }
std::string XLShapeStyle::vTextAnchor() const { return getAttribute("v-text-anchor").value; }
bool XLShapeStyle::hidden() const { return                    ("hidden" == getAttribute("visibility").value ); }
bool XLShapeStyle::visible() const { return !hidden(); }

/**
 * @details XLShapeStyle setter functions
 */
bool XLShapeStyle::setPosition(std::string newPosition)     { return setAttribute("position",                      newPosition); }
bool XLShapeStyle::setMarginLeft(uint16 newMarginLeft)      { return setAttribute("margin-left",    std::to_string(newMarginLeft) + std::string("pt")); }
bool XLShapeStyle::setMarginTop(uint16 newMarginTop)       { return setAttribute("margin-top",     std::to_string(newMarginTop)  + std::string("pt")); }
bool XLShapeStyle::setWidth(uint16 newWidth)           { return setAttribute("width",          std::to_string(newWidth)      + std::string("pt")); }
bool XLShapeStyle::setHeight(uint16 newHeight)          { return setAttribute("height",         std::to_string(newHeight)     + std::string("pt")); }
bool XLShapeStyle::setMsoWrapStyle(std::string newMsoWrapStyle) { return setAttribute("mso-wrap-style",                newMsoWrapStyle); }
bool XLShapeStyle::setVTextAnchor(std::string newVTextAnchor)  { return setAttribute("v-text-anchor",                 newVTextAnchor); }
bool XLShapeStyle::hide()                            { return setAttribute("visibility",                    "hidden"); }
bool XLShapeStyle::show()                            { return setAttribute("visibility",                    "visible"); }

// ========== XLShape Member Functions
XLShape::XLShape() : m_shapeNode(std::make_unique<XMLNode>(XMLNode())) {}

XLShape::XLShape(const XMLNode& node) : m_shapeNode(std::make_unique<XMLNode>(node)) {}

/**
 * @details getter functions: return the shape's attributes
 */
std::string XLShape::shapeId()     const { return m_shapeNode->attribute("id").value(); }                                                     // const for user, managed by parent
std::string XLShape::fillColor()   const { return appendAndGetAttribute(*m_shapeNode, "fillcolor",     "#ffffc0").value(); }
bool XLShape::stroked()     const { return appendAndGetAttribute(*m_shapeNode, "stroked",       "t").as_bool(); }
std::string XLShape::type()        const { return appendAndGetAttribute(*m_shapeNode, "type",          "").value(); }
bool XLShape::allowInCell() const { return appendAndGetAttribute(*m_shapeNode, "o:allowincell", "f").as_bool(); }
XLShapeStyle XLShape::style()             { return XLShapeStyle(appendAndGetAttribute(*m_shapeNode, "style",         "")          ); }

XLShapeClientData XLShape::clientData() {
	return XLShapeClientData(appendAndGetNode(*m_shapeNode, "x:ClientData", m_nodeOrder, XLForceNamespace));
}

/**
 * @details setter functions: assign the shape's attributes
 */
bool XLShape::setFillColor(std::string const& newFillColor) { return appendAndSetAttribute(*m_shapeNode, "fillcolor",     newFillColor).empty() == false; }
bool XLShape::setStroked(bool set)                        { return appendAndSetAttribute(*m_shapeNode, "stroked",       (set ? "t" : "f")).empty() == false; }
bool XLShape::setType(std::string const& newType)      { return appendAndSetAttribute(*m_shapeNode, "type",          newType).empty() == false; }
bool XLShape::setAllowInCell(bool set)                        { return appendAndSetAttribute(*m_shapeNode, "o:allowincell", (set ? "t" : "f")).empty() == false; }
bool XLShape::setStyle(std::string const& newStyle)     { return appendAndSetAttribute(*m_shapeNode, "style",         newStyle).empty() == false; }
bool XLShape::setStyle(XLShapeStyle const& newStyle)    { return setStyle(newStyle.raw() ); }

// ========== XLVmlDrawing Member Functions

/**
 * @details The constructor creates an instance of the superclass, XLXmlFile
 */
XLVmlDrawing::XLVmlDrawing(XLXmlData* xmlData)
	: XLXmlFile(xmlData)
{
	if(xmlData->getXmlType() != XLContentType::VMLDrawing)
		throw XLInternalError("XLVmlDrawing constructor: Invalid XML data.");

	OXlXmlDoc & doc = xmlDocument();
	if(doc.document_element().empty()) // handle a bad (no document element) drawing XML file
		doc.load_string(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<xml"
			" xmlns:v=\"urn:schemas-microsoft-com:vml\""
			" xmlns:o=\"urn:schemas-microsoft-com:office:office\""
			" xmlns:x=\"urn:schemas-microsoft-com:office:excel\""
			" xmlns:w10=\"urn:schemas-microsoft-com:office:word\""
			">"
			"\n</xml>",
			pugi_parse_settings
			);

	// ===== Re-sort the document: move all v:shapetype nodes to the beginning of the XML document element and eliminate duplicates
	// ===== Also: determine highest used shape id, regardless of basename (pattern [^0-9]*[0-9]*) and m_shapeCount
	using namespace std::literals::string_literals;

	XMLNode rootNode = doc.document_element();
	XMLNode node = rootNode.first_child_of_type(pugi::node_element);
	XMLNode lastShapeTypeNode{};
	while(!node.empty()) {
		XMLNode nextNode = node.next_sibling_of_type(pugi::node_element); // determine next node early because node may be invalidated by moveNode
		if(node.raw_name() == ShapeTypeNodeName) {
			if(wouldBeDuplicateShapeType(rootNode, node)) { // if shapetype attribute id already exists at begin of file
				while(node.previous_sibling().type() == pugi::node_pcdata) // delete preceeding whitespaces
					rootNode.remove_child(node.previous_sibling()); //  ...
				rootNode.remove_child(node);              // and the v:shapetype node, as it can not be referenced for lack of a unique id
			}
			else
				lastShapeTypeNode = moveNode(rootNode, node, lastShapeTypeNode); // move node to end of continuous list of shapetype nodes
		}
		else if(node.raw_name() == ShapeNodeName) {
			++m_shapeCount;
			std::string strNodeId = node.attribute("id").value();
			size_t pos = strNodeId.length();
			uint32 nodeId = 0;
			while(pos > 0 && std::isdigit(strNodeId[--pos])) // determine any trailing nodeId
				nodeId = 10*nodeId + strNodeId[pos] - '0';
			m_lastAssignedShapeId = std::max(m_lastAssignedShapeId, nodeId);
		}
		node = nextNode;
	}
	// Henceforth: assume that it is safe to consider shape nodes a continuous list (well - unless there are other node types as well)

	XMLNode shapeTypeNode{};
	if(not lastShapeTypeNode.empty()) {
		shapeTypeNode = rootNode.first_child_of_type(pugi::node_element);
		while(!shapeTypeNode.empty() && shapeTypeNode.raw_name() != ShapeTypeNodeName)
			shapeTypeNode = shapeTypeNode.next_sibling_of_type(pugi::node_element);
	}
	if(shapeTypeNode.empty()) {
		shapeTypeNode = rootNode.prepend_child(ShapeTypeNodeName.c_str(), XLForceNamespace);
		rootNode.prepend_child(pugi::node_pcdata).set_value("\n\t");
	}
	if(shapeTypeNode.first_child().empty())
		shapeTypeNode.append_child(pugi::node_pcdata).set_value("\n\t"); // insert indentation if node was empty

	// ===== Ensure that attributes exist for first shapetype node, default-initialize if needed:
	m_defaultShapeTypeId = appendAndGetAttribute(shapeTypeNode, "id",        "_x0000_t202").value();
	appendAndGetAttribute(shapeTypeNode, "coordsize", "21600,21600");
	appendAndGetAttribute(shapeTypeNode, "o:spt",     "202");
	appendAndGetAttribute(shapeTypeNode, "path",      "m,l,21600l21600,21600l21600,xe");

	XMLNode strokeNode = shapeTypeNode.child("v:stroke");
	if(strokeNode.empty()) {
		strokeNode = shapeTypeNode.prepend_child("v:stroke", XLForceNamespace);
		shapeTypeNode.prepend_child(pugi::node_pcdata).set_value("\n\t\t");
	}
	appendAndGetAttribute(strokeNode, "joinstyle", "miter");

	XMLNode pathNode = shapeTypeNode.child("v:path");
	if(pathNode.empty()) {
		pathNode = shapeTypeNode.insert_child_after("v:path", strokeNode, XLForceNamespace);
		copyLeadingWhitespaces(shapeTypeNode, strokeNode, pathNode);
	}
	appendAndGetAttribute(pathNode, "gradientshapeok", "t");
	appendAndGetAttribute(pathNode, "o:connecttype", "rect");
}

/**
 * @details get first shape node in document_element
 * @return first shape node, empty node if none found
 */
XMLNode XLVmlDrawing::firstShapeNode() const
{
	using namespace std::literals::string_literals;
	XMLNode node = xmlDocumentC().document_element().first_child_of_type(pugi::node_element);
	while(!node.empty() && node.raw_name() != ShapeNodeName) // skip non shape nodes
		node = node.next_sibling_of_type(pugi::node_element);
	return node;
}
/**
 * @details get last shape node in document_element
 * @return last shape node, empty node if none found
 */
XMLNode XLVmlDrawing::lastShapeNode() const
{
	using namespace std::literals::string_literals;
	XMLNode node = xmlDocumentC().document_element().last_child_of_type(pugi::node_element);
	while(!node.empty() && node.raw_name() != ShapeNodeName)
		node = node.previous_sibling_of_type(pugi::node_element);
	return node;
}

/**
 * @details get last shape node at index in document_element
 * @return shape node at index - throws if index is out of bounds
 */
XMLNode XLVmlDrawing::shapeNode(uint32 index) const
{
	using namespace std::literals::string_literals;
	XMLNode node{}; // scope declaration, ensures node.empty() when index >= m_shapeCount
	if(index < m_shapeCount) {
		uint16 i = 0;
		node = firstShapeNode();
		while(i != index && not node.empty() && node.raw_name() == ShapeNodeName) { // follow shape index
			++i;
			node = node.next_sibling_of_type(pugi::node_element);
		}
	}
	if(node.empty() || node.raw_name() != ShapeNodeName)
		throw XLException("XLVmlDrawing: shape index "s + std::to_string(index) + " is out of bounds"s);

	return node;
}

XMLNode XLVmlDrawing::shapeNode(std::string const& cellRef) const
{
	XLCellReference destRef(cellRef);
	uint32 destRow = destRef.row() - 1; // for accessing a shape: x:Row and x:Column are zero-indexed
	uint16 destCol = destRef.column() - 1; // ..

	XMLNode node = firstShapeNode();
	while(!node.empty()) {
		if((destRow == node.child("x:ClientData").child("x:Row").text().as_uint())
		    &&(destCol == node.child("x:ClientData").child("x:Column").text().as_uint()))
			break; // found shape for cellRef

		do { // locate next shape node
			node = node.next_sibling_of_type(pugi::node_element);
		} while(!node.empty() && node.name() != ShapeNodeName);
	}
	return node;
}

uint32 XLVmlDrawing::shapeCount() const { return m_shapeCount; }
XLShape XLVmlDrawing::shape(uint32 index) const { return XLShape(shapeNode(index)); }

bool XLVmlDrawing::deleteShape(uint32 index)
{
	XMLNode rootNode = xmlDocument().document_element();
	XMLNode node = shapeNode(index); // returns a valid node or throws
	--m_shapeCount;                // if shapeNode(index) did not throw: decrement shape count
	while(node.previous_sibling().type() == pugi::node_pcdata) // remove leading whitespaces
		rootNode.remove_child(node.previous_sibling());
	rootNode.remove_child(node);                            // then remove shape node itself

	return true;
}

bool XLVmlDrawing::deleteShape(std::string const& cellRef)
{
	XMLNode rootNode = xmlDocument().document_element();
	XMLNode node = shapeNode(cellRef);
	if(node.empty())  return false;// nothing found to delete

	--m_shapeCount;                // if shapeNode(cellRef) returned a non-empty node: decrement shape count
	while(node.previous_sibling().type() == pugi::node_pcdata) // remove leading whitespaces
		rootNode.remove_child(node.previous_sibling());
	rootNode.remove_child(node);                            // then remove shape node itself

	return true;
}
/**
 * @details insert shape and return index
 */
XLShape XLVmlDrawing::createShape([[maybe_unused]] const XLShape& shapeTemplate)
{
	XMLNode rootNode = xmlDocument().document_element();
	XMLNode node = lastShapeNode();
	if(node.empty()) {
		node = rootNode.last_child_of_type(pugi::node_element);
	}
	if(not node.empty()) {                                                   // default case: a previous element node exists
		node = rootNode.insert_child_after(ShapeNodeName.c_str(), node, XLForceNamespace); // insert the node after the last shape node if any
		copyLeadingWhitespaces(rootNode, node.previous_sibling(), node);             // copy whitespaces from node after which this one was just inserted
	}
	else {                                                                   // else: shouldn't happen - but just in case
		node = rootNode.prepend_child(ShapeNodeName.c_str(), XLForceNamespace);      // insert node prior to trailing whitespaces
		rootNode.prepend_child(pugi::node_pcdata).set_value("\n\t");                 // prefix new node with whitespaces
	}

	// ===== Assign a new shape id & account for it in m_lastAssignedShapeId
	using namespace std::literals::string_literals;
	node.prepend_attribute("id").set_value(("shape_"s + std::to_string(m_lastAssignedShapeId++)).c_str());
	node.append_attribute("type").set_value(("#"s + m_defaultShapeTypeId).c_str());

	m_shapeCount++;
	return XLShape(node); // return object to manipulate new shape
}

/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLVmlDrawing::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }
//
// XLFormula.cpp
//
XLFormula::XLFormula() = default;
XLFormula::XLFormula(const XLFormula& other) = default;
XLFormula::XLFormula(XLFormula&& other) noexcept = default;
XLFormula::~XLFormula() = default;
XLFormula& XLFormula::operator=(const XLFormula& other) = default;
XLFormula& XLFormula::operator=(XLFormula&& other) noexcept = default;
std::string XLFormula::get() const { return m_formulaString; }

XLFormula & XLFormula::clear()
{
	m_formulaString = "";
	return *this;
}

XLFormula::operator std::string() const { return get(); }

XLFormulaProxy::XLFormulaProxy(XLCell* cell, XMLNode* cellNode) : m_cell(cell), m_cellNode(cellNode)
{
	assert(cell);
}

XLFormulaProxy::~XLFormulaProxy() = default;
XLFormulaProxy::XLFormulaProxy(const XLFormulaProxy& other) = default;
XLFormulaProxy::XLFormulaProxy(XLFormulaProxy&& other) noexcept = default;
/**
 * @details Calls the templated string assignment operator.
 */
XLFormulaProxy& XLFormulaProxy::operator=(const XLFormulaProxy& other)
{
	if(&other != this) {
		*this = other.getFormula();
	}
	return *this;
}
/**
 * @details Move assignment operator. Default implementation.
 */
XLFormulaProxy& XLFormulaProxy::operator=(XLFormulaProxy&& other) noexcept = default;

XLFormulaProxy::operator std::string() const { return get(); }
/**
 * @details Returns the underlying XLFormula object, by calling getFormula().
 */
XLFormulaProxy::operator XLFormula() const { return getFormula(); }
/**
 * @details Call the .get() function in the underlying XLFormula object.
 */
std::string XLFormulaProxy::get() const { return getFormula().get(); }
/**
 * @details If a formula node exists, it will be erased.
 */
XLFormulaProxy& XLFormulaProxy::clear()
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	// ===== Remove the value node.
	if(not m_cellNode->child("f").empty())  m_cellNode->remove_child("f");
	return *this;
}
/**
 * @details Convenience function for setting the formula. This method is called from the templated
 * string assignment operator.
 */
void XLFormulaProxy::setFormulaString(const char* formulaString, bool resetValue)
{
	// ===== Check that the m_cellNode is valid.
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	if(formulaString[0] == 0) { // if formulaString is empty
		m_cellNode->remove_child("f"); // clear the formula node
		return;                   // and exit
	}
	// ===== If the cell node doesn't have formula or value child nodes, create them.
	if(m_cellNode->child("f").empty())  m_cellNode->append_child("f");
	if(m_cellNode->child("v").empty())  m_cellNode->append_child("v");

	// ===== Remove the formula type and shared index attributes, if they exist.
	m_cellNode->child("f").remove_attribute("t");
	m_cellNode->child("f").remove_attribute("si");

	// ===== Set the text of the formula and value nodes.
	m_cellNode->child("f").text().set(formulaString);
	if(resetValue)  m_cellNode->child("v").text().set(0);

	// BEGIN pull request #189
	// ===== Remove cell type attribute so that it can be determined by Office Suite when next calculating the formula.
	m_cellNode->remove_attribute("t");

	// ===== Remove inline string <is> tag, in case previous type was "inlineStr".
	m_cellNode->remove_child("is");

	// ===== Ensure that the formula node <f> is the first child, listed before the value <v> node.
	m_cellNode->prepend_move(m_cellNode->child("f"));
	// END pull request #189
}
/**
 * @details Creates and returns an XLFormula object, based on the formula string in the underlying
 * XML document.
 */
XLFormula XLFormulaProxy::getFormula() const
{
	assert(m_cellNode != nullptr);
	assert(not m_cellNode->empty());
	const auto formulaNode = m_cellNode->child("f");
	// ===== If the formula node doesn't exist, return an empty XLFormula object.
	if(formulaNode.empty())  
		return XLFormula();
	// ===== If the formula type is 'shared' or 'array', throw an exception.
	if(not formulaNode.attribute("t").empty() ) { // 2024-05-28: de-duplicated check (only relevant for performance,
		                                      //  xml_attribute::value() returns an empty string for empty attributes)
		if(std::string(formulaNode.attribute("t").value()) == "shared")
			throw XLFormulaError("Shared formulas not supported.");
		if(std::string(formulaNode.attribute("t").value()) == "array")
			throw XLFormulaError("Array formulas not supported.");
	}
	return XLFormula(formulaNode.text().get());
}
//
// XLMergeCells.cpp
//
/**
 * @details Constructs an uninitialized XLMergeCells object
 */
XLMergeCells::XLMergeCells() = default;

/**
 * @details Constructs a new XLMergeCells object. Invoked by XLWorksheet::mergeCells / ::unmergeCells
 * @note Unfortunately, there is no easy way to persist the reference cache, this could be optimized - however, references access shouldn't
 *       be much of a performance issue
 */
XLMergeCells::XLMergeCells(const XMLNode& rootNode, std::vector< std::string_view > const & nodeOrder) : m_rootNode(std::make_unique<XMLNode>(rootNode)),
	m_nodeOrder(nodeOrder), m_mergeCellsNode() // std::unique_ptr initializes to nullptr
{
	if(m_rootNode->empty())
		throw XLInternalError("XLMergeCells constructor: can not construct with an empty XML root node");
	m_mergeCellsNode = std::make_unique<XMLNode>(m_rootNode->child("mergeCells"));
	XMLNode mergeNode = m_mergeCellsNode->first_child_of_type(pugi::node_element);
	while(!mergeNode.empty()) {
		bool invalidNode = true;
		// ===== For valid mergeCell nodes, add the reference to the reference cache
		if(std::string(mergeNode.name()) == "mergeCell") {
			std::string ref = mergeNode.attribute("ref").value();
			if(ref.length() > 0) {
				m_referenceCache.emplace_back(ref);
				invalidNode = false;
			}
		}
		// ===== Determine next element mergeNode
		XMLNode nextNode = mergeNode.next_sibling_of_type(pugi::node_element);
		// ===== In case of an invalid XML element: print an error and remove it from the XML, including whitespaces to the next sibling
		if(invalidNode) { // if mergeNode is not named mergeCell or does not have a valid ref attribute: remove it from the XML
			std::cerr << "XLMergeCells constructor: invalid child element, either name is not mergeCell or reference is invalid:" << std::endl;
			mergeNode.print(std::cerr);
			if(not nextNode.empty()) {
				// delete whitespaces between mergeNode and nextNode
				while(mergeNode.next_sibling() != nextNode)  
					m_mergeCellsNode->remove_child(mergeNode.next_sibling());
			}
			m_mergeCellsNode->remove_child(mergeNode);
		}
		// ===== Advance to next element mergeNode
		mergeNode = nextNode;
	}
	if(m_referenceCache.size() > 0) {
		// ===== Ensure initial array count attribute / issue #351
		XMLAttribute attr = m_mergeCellsNode->attribute("count");
		if(attr.empty())  attr = m_mergeCellsNode->append_attribute("count");
		attr.set_value(m_referenceCache.size());
	}
	else // no merges left
		deleteAll(); // delete mergeCells element & re-initialize m_mergeCellsNode to a default-constructed XMLNode()
}

XLMergeCells::~XLMergeCells() = default;

XLMergeCells::XLMergeCells(const XLMergeCells& other)
{
	m_rootNode = other.m_rootNode ? std::make_unique<XMLNode>(*other.m_rootNode) : std::unique_ptr<XMLNode> {};
	m_nodeOrder = other.m_nodeOrder;
	m_mergeCellsNode = other.m_mergeCellsNode ? std::make_unique<XMLNode>(*other.m_mergeCellsNode) : std::unique_ptr<XMLNode> {};
	m_referenceCache = other.m_referenceCache;
}

XLMergeCells::XLMergeCells(XLMergeCells&& other)
{
	m_rootNode = std::move(other.m_rootNode);
	m_nodeOrder = std::move(other.m_nodeOrder);
	m_mergeCellsNode = std::move(other.m_mergeCellsNode);
	m_referenceCache = std::move(other.m_referenceCache);
}

XLMergeCells& XLMergeCells::operator=(const XLMergeCells& other)
{
	m_rootNode = other.m_rootNode ? std::make_unique<XMLNode>(*other.m_rootNode) : std::unique_ptr<XMLNode> {};
	m_nodeOrder = other.m_nodeOrder;
	m_mergeCellsNode = other.m_mergeCellsNode ? std::make_unique<XMLNode>(*other.m_mergeCellsNode) : std::unique_ptr<XMLNode> {};
	m_referenceCache = other.m_referenceCache;
	return *this;
}

XLMergeCells& XLMergeCells::operator=(XLMergeCells&& other)
{
	m_rootNode = std::move(other.m_rootNode);
	m_nodeOrder = std::move(other.m_nodeOrder);
	m_mergeCellsNode = std::move(other.m_mergeCellsNode);
	m_referenceCache = std::move(other.m_referenceCache);
	return *this;
}

bool XLMergeCells::valid() const { return ( m_rootNode != nullptr && not m_rootNode->empty() ); }

namespace { // anonymous namespace: do not export any symbols from here
/**
 * @brief Test if (range) reference overlaps with the cell window defined by topRow, firstCol, bottomRow, lastCol
 * @return true in case of overlap, false if no overlap
 */
bool XLReferenceOverlaps(std::string reference, uint32 topRow, uint16 firstCol, uint32 bottomRow, uint16 lastCol)
{
	using namespace std::literals::string_literals;
	size_t pos = reference.find_first_of(':'); // find split mark between top left and bottom right cell
	if(pos < 2 || pos + 2 >= reference.length())  // range reference must have at least 2 characters before and after the colon
		throw XLInputError("XLMergeCells::"s + __func__ + ": not a valid range reference: \""s + reference + "\""s);
	XLCellReference refTL(reference.substr(0, pos));  // get top left cell reference
	XLCellReference refBR(reference.substr(pos + 1)); // get bottom right cell reference
	uint32 refTopRow    = refTL.row();
	uint16 refFirstCol  = refTL.column();
	uint32 refBottomRow = refBR.row();
	uint16 refLastCol   = refBR.column();
	if(refBottomRow < refTopRow || refLastCol < refFirstCol || (refBottomRow == refTopRow && refLastCol == refFirstCol))
		throw XLInputError("XLMergeCells::"s + __func__ + ": not a valid range reference: \""s + reference + "\""s);
// std::cout << __func__ << ":" << " reference is " << reference
//    << " refTopRow is " << refTopRow << " refBottomRow is " << refBottomRow << " refFirstCol is " << refFirstCol << " refLastCol is " << refLastCol
//    << " topRow is " << topRow << " bottomRow is " << bottomRow << " firstCol is " << firstCol << " lastCol is " << lastCol
//    << std::endl;
	// overlap
	if(refTopRow <= bottomRow && refBottomRow >= topRow/*vertical overlap*/ && refFirstCol <= lastCol && refLastCol >= firstCol/*horizontal overlap*/) 
		return true;
	return false; // otherwise: no overlap
}
} // anonymous namespace

/**
 * @details Look up a merge index by the reference. If the reference does not exist, the returned index is XLMergeNotFound (-1).
 */
XLMergeIndex XLMergeCells::findMerge(const std::string& reference) const
{
	const auto iter = std::find_if(m_referenceCache.begin(), m_referenceCache.end(), [&](const std::string& ref) {
		return reference == ref;
	});
	return iter == m_referenceCache.end() ? XLMergeNotFound : static_cast<XLMergeIndex>(std::distance(m_referenceCache.begin(), iter));
}

bool XLMergeCells::mergeExists(const std::string& reference) const { return findMerge(reference) >= 0; }

/**
 * @details Find the index of the merge of which cellRef is a part. If no such merge exists, the returned index is XLMergeNotFound (-1).
 */
XLMergeIndex XLMergeCells::findMergeByCell(const std::string& cellRef) const { return findMergeByCell(XLCellReference(cellRef)); }
XLMergeIndex XLMergeCells::findMergeByCell(XLCellReference cellRef) const
{
	const auto iter = std::find_if(m_referenceCache.begin(), m_referenceCache.end(),
	        /**/ [&](const std::string& ref) {               // use XLReferenceOverlaps with a "range" that only contains cellRef
		/**/ return XLReferenceOverlaps(ref, cellRef.row(), cellRef.column(), cellRef.row(), cellRef.column());/**/});
	return iter == m_referenceCache.end() ? XLMergeNotFound : static_cast<XLMergeIndex>(std::distance(m_referenceCache.begin(), iter));
}

size_t XLMergeCells::count() const { return m_referenceCache.size(); }

const char* XLMergeCells::merge(XLMergeIndex index) const
{
	if(index < 0 || static_cast<uint32>(index) >= m_referenceCache.size()) {
		using namespace std::literals::string_literals;
		throw XLInputError("XLMergeCells::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	}
	return m_referenceCache[index].c_str();
}
/**
 * @details Append a mergeCell by creating a new node in the XML file and adding the string to it. The index to the
 * appended merge is returned
 * Before appending a mergeCell entry with reference, check that reference does not overlap with any existing references
 */
XLMergeIndex XLMergeCells::appendMerge(const std::string& reference)
{
	using namespace std::literals::string_literals;
	size_t referenceCacheSize = m_referenceCache.size();
	if(referenceCacheSize >= XLMaxMergeCells)
		throw XLInputError("XLMergeCells::"s + __func__ + ": exceeded max merge cells count "s + std::to_string(XLMaxMergeCells));
	size_t pos = reference.find_first_of(':'); // find split mark between top left and bottom right cell
	if(pos < 2 || pos + 2 >= reference.length()) // range reference must have at least 2 characters before and after the colon
		throw XLInputError("XLMergeCells::"s + __func__ + ": not a valid range reference: \""s + reference + "\""s);
	XLCellReference refTL(reference.substr(0, pos)); // get top left cell reference
	XLCellReference refBR(reference.substr(pos + 1)); // get bottom right cell reference
	uint32 refTopRow    = refTL.row();
	uint16 refFirstCol  = refTL.column();
	uint32 refBottomRow = refBR.row();
	uint16 refLastCol   = refBR.column();
	if(refBottomRow < refTopRow || refLastCol < refFirstCol || (refBottomRow == refTopRow && refLastCol == refFirstCol))
		throw XLInputError("XLMergeCells::"s + __func__ + ": not a valid range reference: \""s + reference + "\""s);
	for(std::string ref : m_referenceCache) {
		if(XLReferenceOverlaps(ref, refTopRow, refFirstCol, refBottomRow, refLastCol))
			throw XLInputError("XLMergeCells::"s + __func__ + ": reference \""s + reference /**/+ "\" overlaps with existing reference \""s + ref + "\""s);
	}
	// if execution gets here: no overlaps
	if(m_mergeCellsNode->empty()) // create mergeCells element if needed
		m_mergeCellsNode = std::make_unique<XMLNode>(appendAndGetNode(*m_rootNode, "mergeCells", m_nodeOrder));
	// append new mergeCell element and set attribute ref
	XMLNode insertAfter = m_mergeCellsNode->last_child_of_type(pugi::node_element);
	XMLNode newMerge{};
	if(insertAfter.empty())  
		newMerge = m_mergeCellsNode->prepend_child("mergeCell");
	else 
		newMerge = m_mergeCellsNode->insert_child_after("mergeCell", insertAfter);
	if(newMerge.empty())
		throw XLInternalError("XLMergeCells::"s + __func__ + ": failed to insert reference: \""s + reference + "\""s);
	newMerge.append_attribute("ref").set_value(reference.c_str());
	m_referenceCache.emplace_back(newMerge.attribute("ref").value()); // index of this element = previous referenceCacheSize
	// ===== Update the array count attribute
	XMLAttribute attr = m_mergeCellsNode->attribute("count");
	if(attr.empty())
		attr = m_mergeCellsNode->append_attribute("count");
	attr.set_value(m_referenceCache.size());
	return static_cast<XLMergeIndex>(referenceCacheSize);
}
/**
 * @details Delete the merge at the given index
 */
void XLMergeCells::deleteMerge(XLMergeIndex index)
{
	using namespace std::literals::string_literals;
	if(index < 0 || static_cast<uint32>(index) >= m_referenceCache.size())
		throw XLInputError("XLMergeCells::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	XLMergeIndex curIndex = 0;
	XMLNode node = m_mergeCellsNode->first_child_of_type(pugi::node_element);
	while(curIndex < index && !node.empty()) {
		node = node.next_sibling_of_type(pugi::node_element);
		++curIndex;
	}
	if(node.empty())
		throw XLInternalError("XLMergeCells::"s + __func__ + ": mismatch between size of mergeCells XML node and internal reference cache"s);
	// ===== node was found: delete preceeding whitespace nodes and the node itself
	while(node.previous_sibling().type() == pugi::node_pcdata)
		m_mergeCellsNode->remove_child(node.previous_sibling());
	m_mergeCellsNode->remove_child(node);
	m_referenceCache.erase(m_referenceCache.begin() + curIndex);
	if(m_referenceCache.size() > 0) {
		// ===== Update the array count attribute
		XMLAttribute attr = m_mergeCellsNode->attribute("count");
		if(attr.empty())
			attr = m_mergeCellsNode->append_attribute("count");
		attr.set_value(m_referenceCache.size()); // update the array count attribute
	}
	else // no merges left
		deleteAll(); // delete mergeCells element & re-initialize m_mergeCellsNode to a default-constructed XMLNode()
}

void XLMergeCells::deleteAll()
{
	m_referenceCache.clear();
	m_rootNode->remove_child(*m_mergeCellsNode);
	m_mergeCellsNode = std::make_unique<XMLNode>(XMLNode());
}

/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLMergeCells::print(std::basic_ostream<char>& ostr) const { m_mergeCellsNode->print(ostr); }
//
// XLProperties.cpp
//
namespace { // anonymous namespace for module local functions
	/**
	 * @brief insert node with nodeNameToInsert before insertBeforeThis and copy all the consecutive whitespace nodes preceeding insertBeforeThis
	 * @param nodeNameToInsert create a new element node with this name
	 * @param insertBeforeThis self-explanatory, isn't it? :)
	 * @returns the inserted XMLNode
	 * @throws XLInternalError if insertBeforeThis has no parent node
	 */
	XMLNode prettyInsertXmlNodeBefore(std::string nodeNameToInsert, XMLNode insertBeforeThis)
	{
		XMLNode parentNode = insertBeforeThis.parent();
		if(parentNode.empty())
			throw XLInternalError("prettyInsertXmlNodeBefore can not insert before a node that has no parent");
		// ===== Find potential whitespaces preceeding insertBeforeThis
		XMLNode whitespaces = insertBeforeThis;
		while(whitespaces.previous_sibling().type() == pugi::node_pcdata)
			whitespaces = whitespaces.previous_sibling();
		// ===== Insert the nodeNameToInsert before insertBeforeThis, "hijacking" the preceeding whitespace nodes
		XMLNode insertedNode = parentNode.insert_child_before(nodeNameToInsert.c_str(), insertBeforeThis);
		// ===== Copy all hijacked whitespaces to also preceed (again) the node insertBeforeThis
		while(whitespaces.type() == pugi::node_pcdata) {
			parentNode.insert_copy_before(whitespaces, insertBeforeThis);
			whitespaces = whitespaces.next_sibling();
		}
		return insertedNode;
	}

	/**
	 * @brief fetch the <HeadingPairs><vt:vector> node from docNode, create if not existing
	 * @param docNode the xml document from which to fetch
	 * @returns an XMLNode for the <vt:vector> child of <HeadingPairs>
	 * @throws XLInternalError in case either XML element does not exist and fails to create
	 */
	XMLNode headingPairsNode(XMLNode docNode)
	{
		XMLNode headingPairs = docNode.child("HeadingPairs");
		if(headingPairs.empty()) {
			XMLNode titlesOfParts = docNode.child("TitlesOfParts"); // attempt to locate TitlesOfParts to insert HeadingPairs right before
			if(titlesOfParts.empty() )
				headingPairs = docNode.append_child("HeadingPairs");
			else
				headingPairs = prettyInsertXmlNodeBefore("HeadingPairs", titlesOfParts);
			if(headingPairs.empty())
				throw XLInternalError("headingPairsNode was unable to create XML element HeadingPairs");
		}
		XMLNode node = headingPairs.first_child_of_type(pugi::node_element);
		if(node.empty()) {  // heading pairs child "vt:vector" does not exist
			node = headingPairs.append_child("vt:vector", XLForceNamespace);
			if(node.empty())
				throw XLInternalError("headingPairsNode was unable to create HeadingPairs child element <vt:vector>");
			node.append_attribute("size") = 0; // NOTE: size counts each heading pair as 2 - but for now, there are no entries
			node.append_attribute("baseType") = "variant";
		}
		// ===== Finally, return what should be a good "vt:vector" node with all the heading pairs
		return node;
	}

	XMLAttribute headingPairsSize(XMLNode docNode)
	{
		return docNode.child("HeadingPairs").first_child_of_type(pugi::node_element).attribute("size");
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-function"
	#endif // __GNUC__
	std::vector<std::string> headingPairsCategoriesStrings(XMLNode docNode)
	{
		// 2024-05-28 DONE: tested this code with two pairs in headingPairsNode
		std::vector<std::string> result;
		XMLNode item = headingPairsNode(docNode).first_child_of_type(pugi::node_element);
		while(!item.empty()) {
			result.push_back(item.first_child_of_type(pugi::node_element).child_value());
			item = item.next_sibling_of_type(pugi::node_element)
				.next_sibling_of_type(pugi::node_element);       // advance two elements to skip count node
		}
		return result; // 2024-05-28: std::move should not be used when the operand of a return statement is the name of a local variable
					   // as this can prevent named return value optimization (NRVO, copy elision)
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
	#pragma GCC diagnostic pop
	#endif // __GNUC__

	/**
	 * @brief fetch the <TitlesOfParts><vt:vector> XML node, create if not existing
	 * @param docNode the xml document from which to fetch
	 * @returns an XMLNode for the <vt:vector> child of <TitlesOfParts>
	 * @throws XLInternalError in case either XML element does not exist and fails to create
	 */
	XMLNode sheetNames(XMLNode docNode)
	{
		XMLNode titlesOfParts = docNode.child("TitlesOfParts");
		if(titlesOfParts.empty()) {
			// ===== Attempt to find a good position to insert TitlesOfParts
			XMLNode node = docNode.first_child_of_type(pugi::node_element);
			std::string nodeName = node.name();
			while(!node.empty() && oneof7(nodeName, "Application", "AppVersion", "DocSecurity", "ScaleCrop", "Template", "TotalTime", "HeadingPairs")) {
				node = node.next_sibling_of_type(pugi::node_element);
				nodeName = node.name();
			}
			if(node.empty())
				titlesOfParts = docNode.append_child("TitlesOfParts");
			else
				titlesOfParts = prettyInsertXmlNodeBefore("TitlesOfParts", node);
		}
		XMLNode vtVector = titlesOfParts.first_child_of_type(pugi::node_element);
		if(vtVector.empty()) {
			vtVector = titlesOfParts.prepend_child("vt:vector", XLForceNamespace);
			if(vtVector.empty())
				throw XLInternalError("sheetNames was unable to create TitlesOfParts child element <vt:vector>");
			vtVector.append_attribute("size") = 0;
			vtVector.append_attribute("baseType") = "lpstr";
		}
		return vtVector;
	}

	XMLAttribute sheetCount(XMLNode docNode) { return sheetNames(docNode).attribute("size"); }

	/**
	 * @brief from the HeadingPairs node, look up name and return the next sibling (which should be the associated value node)
	 * @param docNode the XML document giving access to the <HeadingPairs> node
	 * @param name the value pair whose attribute to return
	 * @returns XMLNode where the value associated with name is stored (the next element sibling from the node matching name)
	 * @returns XMLNode{} (empty) if name is not found or has no next element sibling
	 */
	XMLNode getHeadingPairsValue(XMLNode docNode, std::string name)
	{
		XMLNode item = headingPairsNode(docNode).first_child_of_type(pugi::node_element);
		while(!item.empty() && item.first_child_of_type(pugi::node_element).child_value() != name)
			item = item.next_sibling_of_type(pugi::node_element).next_sibling_of_type(pugi::node_element); // advance two elements to skip count node
		if(not item.empty())     // if name was found
			item = item.next_sibling_of_type(pugi::node_element); // advance once more to the value node

		// ===== Return the first element child of the <vt:variant> node containing the heading pair value
		//       (returns an empty node if item or first_child_of_type is empty)
		return item.first_child_of_type(pugi::node_element);
	}
}    // anonymous namespace

void XLProperties::createFromTemplate()
{
	// std::cout << "XLProperties created with empty docProps/core.xml, creating from scratch!" << std::endl;
	if(m_xmlData == nullptr)
		throw XLInternalError("XLProperties m_xmlData is nullptr");

	// ===== OpenXLSX_XLRelationships::GetStringFromType yields almost the string needed here, with added /relationships
	//       TBD: use hardcoded string?
	std::string xmlns = OpenXLSX_XLRelationships::GetStringFromType(XLRelationshipType::CoreProperties);
	const std::string rels = "/relationships/";
	size_t pos = xmlns.find(rels);
	if(pos != std::string::npos)
		xmlns.replace(pos, rels.size(), "/");
	else
		xmlns = "http://schemas.openxmlformats.org/package/2006/metadata/core-properties"; // fallback to hardcoded string

	XMLNode props = xmlDocument().prepend_child("cp:coreProperties");
	props.append_attribute("xmlns:cp") = xmlns.c_str();
	props.append_attribute("xmlns:dc") = "http://purl.org/dc/elements/1.1/";
	props.append_attribute("xmlns:dcterms") = "http://purl.org/dc/terms/";
	props.append_attribute("xmlns:dcmitype") = "http://purl.org/dc/dcmitype/";
	props.append_attribute("xmlns:xsi") = "http://www.w3.org/2001/XMLSchema-instance";

	props.append_child("dc:creator").text().set("Kenneth Balslev");
	props.append_child("cp:lastModifiedBy").text().set("Kenneth Balslev");

	XMLNode prop {};
	prop = props.append_child("dcterms:created");
	prop.append_attribute("xsi:type") = "dcterms:W3CDTF";
	prop.text().set("2019-08-16T00:34:14Z");
	prop = props.append_child("dcterms:modified");
	prop.append_attribute("xsi:type") = "dcterms:W3CDTF";
	prop.text().set("2019-08-16T00:34:26Z");
}

XLProperties::XLProperties(XLXmlData* xmlData) : XLXmlFile(xmlData)
{
	XMLNode doc = xmlData->getXmlDocument()->document_element();
	XMLNode child = doc.first_child_of_type(pugi::node_element);
	size_t childCount = 0;
	while(!child.empty()) {
		++childCount;
		child = child.next_sibling_of_type(pugi::node_element);
		break; // one child is enough to determine document is not empty.
	}
	if(!childCount)   
		createFromTemplate();
}

XLProperties::~XLProperties() = default;

void XLProperties::setProperty(const std::string& name, const std::string& value)
{
	if(m_xmlData) {
		XMLNode node = xmlDocument().document_element().child(name.c_str());
		if(node.empty())
			node = xmlDocument().document_element().append_child(name.c_str()); // .append_child, to be in line with ::property behavior
		node.text().set(value.c_str());
	}
}

void XLProperties::setProperty(const std::string& name, int value) { setProperty(name, std::to_string(value)); }
void XLProperties::setProperty(const std::string& name, double value) { setProperty(name, std::to_string(value)); }

std::string XLProperties::property(const std::string& name) const
{
	if(m_xmlData == nullptr)  
		return "";
	else {
		XMLNode property = xmlDocumentC().document_element().child(name.c_str());
		if(property.empty())  
			property = xmlDocumentC().document_element().append_child(name.c_str());
		return property.text().get();
	}
}

void XLProperties::deleteProperty(const std::string& name)
{
	if(m_xmlData) {
		if(const XMLNode property = xmlDocument().document_element().child(name.c_str()); not property.empty())
			xmlDocument().document_element().remove_child(property);
	}
}

void XLAppProperties::createFromTemplate(OXlXmlDoc const & workbookXml)
{
	// std::cout << "XLAppProperties created with empty docProps/app.xml, creating from scratch!" << std::endl;
	if(m_xmlData == nullptr)
		throw XLInternalError("XLAppProperties m_xmlData is nullptr");
	std::map< uint32, std::string > sheetsOrderedById;
	auto sheet = workbookXml.document_element().child("sheets").first_child_of_type(pugi::node_element);
	while(!sheet.empty()) {
		std::string sheetName = sheet.attribute("name").as_string();
		uint32 sheetId = sheet.attribute("sheetId").as_uint();
		sheetsOrderedById.insert(std::pair<uint32, std::string>(sheetId, sheetName));
		sheet = sheet.next_sibling_of_type();
	}
	uint32 worksheetCount = 0;
	for(const auto & [key, value] : sheetsOrderedById) {
		if(key != ++worksheetCount)
			throw XLInputError("xl/workbook.xml is missing sheet with sheetId=\"" + std::to_string(worksheetCount) + "\"");
	}
	// ===== OpenXLSX_XLRelationships::GetStringFromType yields almost the string needed here, with added /relationships
	//       TBD: use hardcoded string?
	std::string xmlns = OpenXLSX_XLRelationships::GetStringFromType(XLRelationshipType::ExtendedProperties);
	const std::string rels = "/relationships/";
	size_t pos = xmlns.find(rels);
	if(pos != std::string::npos)
		xmlns.replace(pos, rels.size(), "/");
	else
		xmlns = "http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"; // fallback to hardcoded string
	XMLNode props = xmlDocument().prepend_child("Properties");
	props.append_attribute("xmlns") = xmlns.c_str();
	props.append_attribute("xmlns:vt") = "http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes";
	XMLNode prop {};
	props.append_child("Application").text().set("Microsoft Macintosh Excel");
	props.append_child("DocSecurity").text().set(0);
	props.append_child("ScaleCrop").text().set(false);
	XMLNode headingPairs = props.append_child("HeadingPairs");
	XMLNode vecHP = headingPairs.append_child("vt:vector", XLForceNamespace);
	vecHP.append_attribute("size") = 2;
	vecHP.append_attribute("baseType") = "variant";
	vecHP.append_child("vt:variant", XLForceNamespace).append_child("vt:lpstr", XLForceNamespace).text().set("Worksheets");
	// ===== 2024-09-02 minor bugfix issue #220, "vt:i4" value of "Worksheets" pair should be sheet count
	vecHP.append_child("vt:variant", XLForceNamespace).append_child("vt:i4", XLForceNamespace).text().set(worksheetCount);

	XMLNode sheetsVector = props.append_child("TitlesOfParts").append_child("vt:vector", XLForceNamespace); // 2024-08-17 BUGFIX: TitlesOfParts was wrongly appended to headingPairs
	sheetsVector.append_attribute("size") = worksheetCount;
	sheetsVector.append_attribute("baseType") = "lpstr";
	for(const auto & [key, value] : sheetsOrderedById)
		sheetsVector.append_child("vt:lpstr", XLForceNamespace).text().set(value.c_str());

	// NOTE 2024-07-24: for an empty string, .text().set("") would create a "<Company></Company>" node that
	// would get reduced to <Company/> on a simple open + save operation and thereby cause the *only* diff
	// to the uncompressed workbook. For this cosmetic reasons, text().set was removed in this case
	props.append_child("Company"); // .text().set("");
	props.append_child("LinksUpToDate").text().set(false);
	props.append_child("SharedDoc").text().set(false);
	props.append_child("HyperlinksChanged").text().set(false);
	props.append_child("AppVersion").text().set("16.0300");
}

XLAppProperties::XLAppProperties(XLXmlData* xmlData, const OXlXmlDoc & workbookXml) : XLXmlFile(xmlData)
{
	XMLNode doc = xmlData->getXmlDocument()->document_element();
	XMLNode child = doc.first_child_of_type(pugi::node_element);
	size_t childCount = 0;
	while(!child.empty()) {
		++childCount;
		child = child.next_sibling_of_type(pugi::node_element);
		break; // one child is enough to determine document is not empty.
	}
	if(!childCount)
		createFromTemplate(workbookXml);//    create fresh docProps/app.xml
}

XLAppProperties::XLAppProperties(XLXmlData* xmlData) : XLXmlFile(xmlData) {}
XLAppProperties::~XLAppProperties() = default;

/**
 * @details
 * @note increment == 0 is tolerated for now, used by alignWorksheets to add the value to HeadingPairs if not present
 *       However, generally this function should only be called with increment == -1 or increment == +1
 */
void XLAppProperties::incrementSheetCount(int16 increment)
{
	int32_t newCount = sheetCount(xmlDocument().document_element()).as_int() + increment;
	if(newCount < 1)
		throw XLInternalError("must not decrement sheet count below 1");
	sheetCount(xmlDocument().document_element()).set_value(newCount);
	XMLNode headingPairWorksheetsValue = getHeadingPairsValue(xmlDocument().document_element(), "Worksheets");
	if(headingPairWorksheetsValue.empty()) // seems heading pair does not exist
		addHeadingPair("Worksheets", newCount);
	else
		headingPairWorksheetsValue.text().set(newCount);
}

/**
 * @details ensure that <TitlesOfParts> contains the correct workbook sheet names and <HeadingPairs> Worksheets entry
 *          has the correct worksheet count
 */
void XLAppProperties::alignWorksheets(std::vector<std::string> const & workbookSheetNames)
{
	XMLNode titlesOfParts = sheetNames(xmlDocument().document_element()); // creates <TitlesOfParts><vt:vector> if not existing
	XMLNode sheetName = titlesOfParts.first_child_of_type(pugi::node_element);
	// size_t sheetCount = workbookSheetNames.size();
	for(std::string workbookSheetName : workbookSheetNames) {
		if(sheetName.empty())
			sheetName = titlesOfParts.append_child("vt:lpstr", XLForceNamespace);
		sheetName.text().set(workbookSheetName.c_str());
		sheetName = sheetName.next_sibling_of_type(pugi::node_element); // advance to next entry in <TitlesOfParts>, potentially becomes an empty XMLNode()
	}

	// ===== If there are any excess sheet names, clean up.
	if(not sheetName.empty()) {
		// ===== Delete all whitespace nodes prior to the sheet names to be deleted
		while(sheetName.previous_sibling().type() == pugi::node_pcdata)  
			titlesOfParts.remove_child(sheetName.previous_sibling());
		XMLNode lastSheetName = titlesOfParts.last_child_of_type(pugi::node_element); // delete up to & including this node, preserve whitespaces after
		if(sheetName != lastSheetName) { // If lastSheetName is a node *behind* sheetName
			// ===== Delete all nodes (elements and whitespace) up until lastSheetName
			while(sheetName.next_sibling() != lastSheetName)
				titlesOfParts.remove_child(sheetName.next_sibling());
			// ===== Delete lastSheetName
			titlesOfParts.remove_child(lastSheetName);
		}
		// ===== Delete sheetName
		titlesOfParts.remove_child(sheetName);
	}

	sheetCount(xmlDocument().document_element()).set_value(workbookSheetNames.size()); // save size of <TitlesOfParts><vt:vector>
	incrementSheetCount(0); // ensure that the sheet count is correctly reflected in <HeadingPairs> Worksheets entry
}

void XLAppProperties::addSheetName(const std::string& title)
{
	if(m_xmlData) {
		XMLNode theNode = sheetNames(xmlDocument().document_element()).append_child("vt:lpstr", XLForceNamespace);
		theNode.text().set(title.c_str());
		incrementSheetCount(+1);
	}
}

void XLAppProperties::deleteSheetName(const std::string& title)
{
	if(m_xmlData) {
		XMLNode theNode = sheetNames(xmlDocument().document_element()).first_child_of_type(pugi::node_element);
		while(!theNode.empty()) {
			if(theNode.child_value() == title) {
				sheetNames(xmlDocument().document_element()).remove_child(theNode);
				incrementSheetCount(-1);
				return;
			}
			theNode = theNode.next_sibling_of_type(pugi::node_element);
		}
	}
}

void XLAppProperties::setSheetName(const std::string& oldTitle, const std::string& newTitle)
{
	if(m_xmlData) {
		XMLNode theNode = sheetNames(xmlDocument().document_element()).first_child_of_type(pugi::node_element);
		while(!theNode.empty()) {
			if(theNode.child_value() == oldTitle) {
				theNode.text().set(newTitle.c_str());
				return;
			}
			theNode = theNode.next_sibling_of_type(pugi::node_element);
		}
	}
}

void XLAppProperties::addHeadingPair(const std::string& name, int value)
{
	if(m_xmlData) {
		XMLNode HeadingPairsNode = headingPairsNode(xmlDocument().document_element());
		XMLNode item = HeadingPairsNode.first_child_of_type(pugi::node_element);
		while(!item.empty() && item.first_child_of_type(pugi::node_element).child_value() != name)
			item = item.next_sibling_of_type(pugi::node_element).next_sibling_of_type(pugi::node_element);   // advance two elements to skip count node
		XMLNode pairCategory = item; // could be an empty node
		XMLNode pairValue {};       // initialize to empty node
		if(!pairCategory.empty())
			pairValue = pairCategory.next_sibling_of_type(pugi::node_element).first_child_of_type(pugi::node_element);
		else {
			item = HeadingPairsNode.last_child_of_type(pugi::node_element);
			if(!item.empty())
				pairCategory = HeadingPairsNode.insert_child_after("vt:variant", item, XLForceNamespace);
			else
				pairCategory = HeadingPairsNode.append_child("vt:variant", XLForceNamespace);
			XMLNode categoryName = pairCategory.append_child("vt:lpstr", XLForceNamespace);
			categoryName.text().set(name.c_str());
			XMLNode pairValueParent = HeadingPairsNode.insert_child_after("vt:variant", pairCategory, XLForceNamespace);
			pairValue = pairValueParent.append_child("vt:i4", XLForceNamespace);
		}
		if(!pairValue.empty())
			pairValue.text().set(std::to_string(value).c_str());
		else {
			using namespace std::literals::string_literals;
			throw XLInternalError("XLAppProperties::addHeadingPair: found no matching pair count value to name "s + name);
		}
		headingPairsSize(xmlDocument().document_element()).set_value(HeadingPairsNode.child_count_of_type());
	}
}

void XLAppProperties::deleteHeadingPair(const std::string& name)
{
	if(m_xmlData) {
		XMLNode HeadingPairsNode = headingPairsNode(xmlDocument().document_element());
		XMLNode item = HeadingPairsNode.first_child_of_type(pugi::node_element);
		while(!item.empty() && item.first_child_of_type(pugi::node_element).child_value() != name)
			item = item.next_sibling_of_type(pugi::node_element).next_sibling_of_type(pugi::node_element);   // advance two elements to skip count node
		// ===== If item with name was found, remove pair and update headingPairsSize
		if(!item.empty()) {
			const XMLNode count = item.next_sibling_of_type(pugi::node_element);
			// ===== 2024-05-28: delete all (non-element) nodes between item and count node, *then* delete non-element nodes following a count node
			if(!count.empty()) {
				while(item.next_sibling() != count)
					HeadingPairsNode.remove_child(item.next_sibling()); // remove nodes between item & count nodes to be deleted jointly
				// ===== Delete all non-element nodes following the count node
				while(!count.next_sibling().empty() && (count.next_sibling().type() != pugi::node_element))
					HeadingPairsNode.remove_child(count.next_sibling()); // remove all non-element nodes following a count node
				HeadingPairsNode.remove_child(count);
			}
			// REMOVED: formatting doesn't get prettier by removing whitespaces on both sides of a pair
			// while (item.previous_sibling().type() == pugi::node_pcdata) HeadingPairsNode.remove_child(item.previous_sibling());
			HeadingPairsNode.remove_child(item);
			headingPairsSize(xmlDocument().document_element()).set_value(HeadingPairsNode.child_count_of_type());
		}
	}
}

void XLAppProperties::setHeadingPair(const std::string& name, int newValue)
{
	if(m_xmlData) {
		XMLNode pairValue = getHeadingPairsValue(xmlDocument().document_element(), name);
		using namespace std::literals::string_literals;
		if(not pairValue.empty() && (pairValue.raw_name() == "vt:i4"s))
			pairValue.text().set(std::to_string(newValue).c_str());
		else
			throw XLInternalError("XLAppProperties::setHeadingPair: found no matching pair count value to name "s + name);
	}
}

void XLAppProperties::setProperty(const std::string& name, const std::string& value)
{
	if(m_xmlData) {
		auto property = xmlDocument().document_element().child(name.c_str());
		if(property.empty())  
			xmlDocument().document_element().append_child(name.c_str());
		property.text().set(value.c_str());
	}
}

std::string XLAppProperties::property(const std::string& name) const
{
	if(m_xmlData == nullptr)  
		return "";
	else {
		XMLNode property = xmlDocumentC().document_element().child(name.c_str());
		if(property.empty())
			property = xmlDocumentC().document_element().append_child(name.c_str()); // BUGFIX 2024-05-21: re-assign the newly created node to property, so that .text().get() is defined behavior
		return property.text().get();
		// NOTE 2024-05-21: this was previously defined behavior because XMLNode::text() called from an empty xml_node returns an xml_text node
		// constructed on an empty _root pointer, while in turn xml_text::get() returns a PUGIXML_TEXT("") for an empty xml_text node
		// However, relying on all this implicit functionality was really ugly ;)
	}
}

void XLAppProperties::deleteProperty(const std::string & name)
{
	if(m_xmlData) {
		const auto property = xmlDocument().document_element().child(name.c_str());
		if(!property.empty())  
			xmlDocument().document_element().remove_child(property);
	}
}

void XLAppProperties::appendSheetName(const std::string& sheetName)
{
	if(m_xmlData) {
		auto theNode = sheetNames(xmlDocument().document_element()).append_child("vt:lpstr", XLForceNamespace);
		theNode.text().set(sheetName.c_str());
		incrementSheetCount(+1);
	}
}

void XLAppProperties::prependSheetName(const std::string& sheetName)
{
	if(m_xmlData) {
		auto theNode = sheetNames(xmlDocument().document_element()).prepend_child("vt:lpstr", XLForceNamespace);
		theNode.text().set(sheetName.c_str());
		incrementSheetCount(+1);
	}
}

void XLAppProperties::insertSheetName(const std::string& sheetName, uint index)
{
	if(m_xmlData == nullptr)  
		return;
	if(index <= 1) {
		prependSheetName(sheetName);
		return;
	}
	// ===== If sheet needs to be appended...
	if(index > sheetCount(xmlDocument().document_element()).as_uint()) {
		// ===== If at least one sheet node exists, apply some pretty formatting by appending new sheet name between last sheet and trailing
		// whitespaces.
		const XMLNode lastSheet = sheetNames(xmlDocument().document_element()).last_child_of_type(pugi::node_element);
		if(!lastSheet.empty()) {
			XMLNode theNode = sheetNames(xmlDocument().document_element()).insert_child_after("vt:lpstr", lastSheet, XLForceNamespace);
			theNode.text().set(sheetName.c_str());
			// ===== Update sheet count before return statement.
			incrementSheetCount(+1);
		}
		else
			appendSheetName(sheetName);
		return;
	}
	XMLNode curNode = sheetNames(xmlDocument().document_element()).first_child_of_type(pugi::node_element);
	uint idx = 1;
	while(!curNode.empty()) {
		if(idx == index)  
			break;
		curNode = curNode.next_sibling_of_type(pugi::node_element);
		++idx;
	}
	if(curNode.empty()) {
		appendSheetName(sheetName);
		return;
	}
	XMLNode theNode = sheetNames(xmlDocument().document_element()).insert_child_before("vt:lpstr", curNode, XLForceNamespace);
	theNode.text().set(sheetName.c_str());
	incrementSheetCount(+1);
}
//
// XLRelationships.cpp
//
namespace { // anonymous namespace: do not export these symbols
	bool RandomIDs{false};                 // default: use sequential IDs
	bool RandomizerInitialized{false};     // will be initialized by GetNewRelsID, if XLRand32 / XLRand64 are used elsewhere, user should invoke XLInitRandom
}

namespace OpenXLSX {
	/**
	 * @details Set the module-local status variable RandomIDs to true
	 */
	void UseRandomIDs() { RandomIDs = true; }
	/**
	 * @details Set the module-local status variable RandomIDs to false
	 */
	void UseSequentialIDs() { RandomIDs = false; }
	/**
	 * @details Use the std::mt19937 default return on operator()
	 */
	std::mt19937 Rand32(0);
	/**
	 * @details Combine values from two subsequent invocations of Rand32()
	 */
	uint64 Rand64() { return (static_cast<uint64>(Rand32()) << 32) + Rand32(); }     // 2024-08-18 BUGFIX: left-shift does *not* do integer promotion to long on the left operand
	/**
	 * @details Initialize the module's random functions
	 */
	void InitRandom(bool pseudoRandom)
	{
		uint64 rdSeed;
		if(pseudoRandom)  
			rdSeed = 3744821255L; // in pseudo-random mode, always use the same seed
		else {
			std::random_device rd;
			rdSeed = rd();
		}
		Rand32.seed(static_cast<uint>(rdSeed));
		RandomizerInitialized = true;
	}
} // namespace OpenXLSX

namespace {
	const std::string relationshipDomainOpenXml2006          = "http://schemas.openxmlformats.org/officeDocument/2006";
	const std::string relationshipDomainOpenXml2006CoreProps = "http://schemas.openxmlformats.org/package/2006";
	const std::string relationshipDomainMicrosoft2006        = "http://schemas.microsoft.com/office/2006";
	const std::string relationshipDomainMicrosoft2011        = "http://schemas.microsoft.com/office/2011";

	/**
	 * @note 2024-08-31: Included a "dumb" fallback solution in relationship tests to support
	 *          previously unknown relationship domains, e.g. type="http://purl.oclc.org/ooxml/officeDocument/relationships/worksheet"
	 */
	XLRelationshipType GetRelationshipTypeFromString(const std::string& typeString)
	{
		// TODO 2024-08-09: support dumb applications that implemented relationship Type in different case (e.g. vmldrawing instead of vmlDrawing)
		//                  easy approach: convert typestring and comparison string to all lower characters
		size_t comparePos = 0; // start by comparing full relationship type strings
		do {
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/extended-properties")
				return XLRelationshipType::ExtendedProperties;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/custom-properties")
				return XLRelationshipType::CustomProperties;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/officeDocument")
				return XLRelationshipType::Workbook;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/worksheet")
				return XLRelationshipType::Worksheet;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/styles")
				return XLRelationshipType::Styles;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/sharedStrings")
				return XLRelationshipType::SharedStrings;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/calcChain")
				return XLRelationshipType::CalculationChain;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/externalLink")
				return XLRelationshipType::ExternalLink;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/theme")
				return XLRelationshipType::Theme;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/chartsheet")
				return XLRelationshipType::Chartsheet;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/drawing")
				return XLRelationshipType::Drawing;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/image")
				return XLRelationshipType::Image;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/chart")
				return XLRelationshipType::Chart;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/externalLinkPath")
				return XLRelationshipType::ExternalLinkPath;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/printerSettings")
				return XLRelationshipType::PrinterSettings;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/vmlDrawing")
				return XLRelationshipType::VMLDrawing;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/ctrlProp")
				return XLRelationshipType::ControlProperties;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006CoreProps) + "/relationships/metadata/core-properties")
				return XLRelationshipType::CoreProperties;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainMicrosoft2006) + "/relationships/vbaProject")
				return XLRelationshipType::VBAProject;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainMicrosoft2011) + "/relationships/chartStyle")
				return XLRelationshipType::ChartStyle;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainMicrosoft2011) + "/relationships/chartColorStyle")
				return XLRelationshipType::ChartColorStyle;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/comments")
				return XLRelationshipType::Comments;
			if(typeString.substr(comparePos) == (comparePos ? "" : relationshipDomainOpenXml2006) + "/relationships/table")
				return XLRelationshipType::Table;

			// ===== relationship could not be identified
			if(comparePos == 0)  // If fallback solution has not yet been tried
				comparePos = typeString.find("/relationships/"); // attempt to find the relationships section of the type string, regardless of domain
			else                 // If fallback solution was tried & unsuccessful
				comparePos = 0;                             // trigger loop exit
		} while(comparePos > 0 && comparePos != std::string::npos);
		// ===== loop exits if comparePos is not within typeString (= fallback solution failed or not possible)

		return XLRelationshipType::Unknown; // default: relationship could not be identified
	}
} // namespace

namespace OpenXLSX_XLRelationships { // make GetStringFromType accessible throughout the project (for use by XLAppProperties)
	std::string GetStringFromType(XLRelationshipType type)
	{
		switch(type) {
			case XLRelationshipType::ExtendedProperties: return relationshipDomainOpenXml2006 + "/relationships/extended-properties";
			case XLRelationshipType::CustomProperties:   return relationshipDomainOpenXml2006 + "/relationships/custom-properties";
			case XLRelationshipType::Workbook:           return relationshipDomainOpenXml2006 + "/relationships/officeDocument";
			case XLRelationshipType::Worksheet:          return relationshipDomainOpenXml2006 + "/relationships/worksheet";
			case XLRelationshipType::Styles:             return relationshipDomainOpenXml2006 + "/relationships/styles";
			case XLRelationshipType::SharedStrings:      return relationshipDomainOpenXml2006 + "/relationships/sharedStrings";
			case XLRelationshipType::CalculationChain:   return relationshipDomainOpenXml2006 + "/relationships/calcChain";
			case XLRelationshipType::ExternalLink:       return relationshipDomainOpenXml2006 + "/relationships/externalLink";
			case XLRelationshipType::Theme:              return relationshipDomainOpenXml2006 + "/relationships/theme";
			case XLRelationshipType::Chartsheet:         return relationshipDomainOpenXml2006 + "/relationships/chartsheet";
			case XLRelationshipType::Drawing:            return relationshipDomainOpenXml2006 + "/relationships/drawing";
			case XLRelationshipType::Image:              return relationshipDomainOpenXml2006 + "/relationships/image";
			case XLRelationshipType::Chart:              return relationshipDomainOpenXml2006 + "/relationships/chart";
			case XLRelationshipType::ExternalLinkPath:   return relationshipDomainOpenXml2006 + "/relationships/externalLinkPath";
			case XLRelationshipType::PrinterSettings:    return relationshipDomainOpenXml2006 + "/relationships/printerSettings";
			case XLRelationshipType::VMLDrawing:         return relationshipDomainOpenXml2006 + "/relationships/vmlDrawing";
			case XLRelationshipType::ControlProperties:  return relationshipDomainOpenXml2006 + "/relationships/ctrlProp";
			case XLRelationshipType::CoreProperties:     return relationshipDomainOpenXml2006CoreProps + "/relationships/metadata/core-properties";
			case XLRelationshipType::VBAProject:         return relationshipDomainMicrosoft2006 + "/relationships/vbaProject";
			case XLRelationshipType::ChartStyle:         return relationshipDomainMicrosoft2011 + "/relationships/chartStyle";
			case XLRelationshipType::ChartColorStyle:    return relationshipDomainMicrosoft2011 + "/relationships/chartColorStyle";
			case XLRelationshipType::Comments:           return relationshipDomainOpenXml2006 + "/relationships/comments";
			case XLRelationshipType::Table:              return relationshipDomainOpenXml2006 + "/relationships/table";
			default:
				throw XLInternalError("RelationshipType not recognized!");
		}
	}
}

namespace { //    re-open anonymous namespace
/**
 * @brief Get a new, unique relationship ID
 * @param relationshipsNode the XML node that contains the document relationships
 * @return A 64 bit integer with a relationship ID
 */
uint64 GetNewRelsID(XMLNode relationshipsNode)
{
	if(RandomIDs) {
		if(!RandomizerInitialized)  InitRandom();
		return Rand64();
	}
	using namespace std::literals::string_literals;
	// ===== workaround for pugi::xml_node currently not having an iterator for node_element only
	XMLNode relationship = relationshipsNode.first_child_of_type(pugi::node_element);
	uint64 newId = 1;    // default
	while(!relationship.empty()) {
		uint64 id;
		try {
			id = std::stoi(std::string(relationship.attribute("Id").value()).substr(3));
		}
		catch(std::invalid_argument const & e) { // expected stoi exception
			throw XLInputError("GetNewRelsID could not convert attribute Id to uint32 ("s + e.what() + ")"s);
		}
		catch(...) { // catch all other errors during conversion of attribute to uint32
			throw XLInputError("GetNewRelsID could not convert attribute Id to uint32"s);
		}
		if(id >= newId)  newId = id + 1;
		relationship = relationship.next_sibling_of_type(pugi::node_element);
	}
	return newId;
}

/**
 * @brief Get a string representation of GetNewRelsID
 * @param relationshipsNode pass-through parameter to GetNewRelsID
 * @return A std::string that can be used as relationship ID
 * @note The format of the returned value will be a base-16 number if RandomIDs is true, otherwise a base-10 number
 */
std::string GetNewRelsIDString(XMLNode relationshipsNode) {
	uint64 newId = GetNewRelsID(relationshipsNode);
	if(RandomIDs)  return "R" + BinaryAsHexString(&newId, sizeof(newId));
	return "rId" + std::to_string(newId);
}
}    // anonymous namespace

XLRelationshipItem::XLRelationshipItem() : m_relationshipNode(std::make_unique<XMLNode>()) {}

XLRelationshipItem::XLRelationshipItem(const XMLNode& node) : m_relationshipNode(std::make_unique<XMLNode>(node)) 
{
}

XLRelationshipItem::~XLRelationshipItem() = default;

XLRelationshipItem::XLRelationshipItem(const XLRelationshipItem& other) : m_relationshipNode(std::make_unique<XMLNode>(*other.m_relationshipNode))
{
}

XLRelationshipItem& XLRelationshipItem::operator=(const XLRelationshipItem& other)
{
	if(&other != this)  
		*m_relationshipNode = *other.m_relationshipNode;
	return *this;
}
/**
 * @details Returns the m_relationshipType member variable by getValue.
 */
XLRelationshipType XLRelationshipItem::type() const { return GetRelationshipTypeFromString(m_relationshipNode->attribute("Type").value()); }

/**
 * @details Returns the m_relationshipTarget member variable by getValue.
 */
std::string XLRelationshipItem::target() const
{
	// 2024-12-15 Returned to old behavior: do not strip leading slashes as this loses info about path being absolute.
	//            Instead, treat absolute vs. relative path distinction in caller
	return m_relationshipNode->attribute("Target").value();
}

/**
 * @details Returns the m_relationshipId member variable by getValue.
 */
std::string XLRelationshipItem::id() const { return m_relationshipNode->attribute("Id").value(); }

/**
 * @details Returns the m_relationshipNode->empty() status
 */
bool XLRelationshipItem::empty() const { return m_relationshipNode->empty(); }

/**
 * @details Creates a XLRelationships object, which will read the XML file with the given path
 *  The pathTo the relationships XML file will be verified & stored in m_path, which is subsequently
 *   used to return all relationship targets as absolute paths within the XLSX archive
 */
XLRelationships::XLRelationships(XLXmlData* xmlData, std::string pathTo) : XLXmlFile(xmlData)
{
	constexpr const char * relFolder = "_rels/"; // all relationships are stored in a (sub-)folder named "_rels/"
	static const size_t relFolderLen = strlen(relFolder); // 2024-08-23: strlen seems to not be accepted in a constexpr in VS2019 with c++17

	bool addFirstSlash = (pathTo[0] != '/'); // if first character of pathTo is NOT a slash, then addFirstSlash = true
	size_t pathToEndsAt = pathTo.find_last_of('/');
	if((pathToEndsAt != std::string::npos) && (pathToEndsAt + 1 >= relFolderLen)
	    && (pathTo.substr(pathToEndsAt + 1 - relFolderLen, relFolderLen) == relFolder)) // nominal
		m_path = (addFirstSlash ? "/" : "") + pathTo.substr(0, pathToEndsAt - relFolderLen + 1);
	else {
		using namespace std::literals::string_literals;
		throw XLException("XLRelationships constructor: pathTo \""s + pathTo + "\" does not point to a file in a folder named \"" + relFolder + "\""s);
	}
	OXlXmlDoc & doc = xmlDocument();
	if(doc.document_element().empty()) // handle a bad (no document element) relationships XML file
		doc.load_string(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"\n"
			"</Relationships>",
			pugi_parse_settings);
}

XLRelationships::~XLRelationships() = default;

/**
 * @details Returns the XLRelationshipItem with the given ID, by looking it up in the m_relationships map.
 */
XLRelationshipItem XLRelationships::relationshipById(const std::string& id) const
{
	return XLRelationshipItem(xmlDocumentC().document_element().find_child_by_attribute("Id", id.c_str()));
}
/**
 * @details Returns the XLRelationshipItem with the requested target, by iterating through the items.
 * @note 2024-07-24: perform a match that is agnostic to relative vs. absolute (leading slash) paths
 */
XLRelationshipItem XLRelationships::relationshipByTarget(const std::string& target, bool throwIfNotFound) const
{
	// turn relative path into an absolute and resolve . and .. entries
	std::string absoluteTarget = eliminateDotAndDotDotFromPath(target[0] == '/' ? target : m_path + target);
	XMLNode relationshipNode = xmlDocumentC().document_element().first_child_of_type(pugi::node_element);
	while(!relationshipNode.empty()) {
		std::string relationTarget = relationshipNode.attribute("Target").value();
		if(relationTarget[0] != '/')
			relationTarget = m_path + relationTarget;// turn relative path into an absolute
		relationTarget = eliminateDotAndDotDotFromPath(relationTarget);    // and resolve . and .. entries, if any
		// ===== Attempt to match absoluteTarget & relationTarget
		if(absoluteTarget == relationTarget)
			return XLRelationshipItem(relationshipNode);// found!
		relationshipNode = relationshipNode.next_sibling_of_type(pugi::node_element);
	}
	if(throwIfNotFound) {
		using namespace std::literals::string_literals;
		throw XLException("XLRelationships::"s + __func__ + ": relationship with target \""s + target + "\" (absolute: \""s + absoluteTarget + "\" does not exist!"s);
	}
	return XLRelationshipItem(); // fail with an empty XLRelationshipItem return value -> can be tested for ::empty()
}
/**
 * @details Returns a const reference to the internal datastructure (std::vector)
 */
std::vector<XLRelationshipItem> XLRelationships::relationships() const
{
	// ===== workaround for pugi::xml_node currently not having an iterator for node_element only
	auto result = std::vector<XLRelationshipItem>();
	XMLNode item = xmlDocumentC().document_element().first_child_of_type(pugi::node_element);
	while(!item.empty()) {
		result.emplace_back(XLRelationshipItem(item));
		item = item.next_sibling_of_type(pugi::node_element);
	}
	// ===== if a node_element iterator can be implemented for pugi::xml_node, the below code can be used again
	// for (const auto& item : xmlDocument().document_element().children()) result.emplace_back(XLRelationshipItem(item));
	return result;
}

void XLRelationships::deleteRelationship(const std::string& relID)
{
	xmlDocument().document_element().remove_child(xmlDocument().document_element().find_child_by_attribute("Id", relID.c_str()));
}

void XLRelationships::deleteRelationship(const XLRelationshipItem& item) { deleteRelationship(item.id()); }

/**
 * @details Adds a new relationship by creating new XML node in the .rels file and creating a new XLRelationshipItem
 * based on the newly created node.
 * @note 2024-07-22: added more intelligent whitespace support
 */
XLRelationshipItem XLRelationships::addRelationship(XLRelationshipType type, const std::string& target)
{
	const std::string typeString = OpenXLSX_XLRelationships::GetStringFromType(type);
	// const std::string id = "rId" + std::to_string(GetNewRelsID(xmlDocument().document_element()));
	const std::string id = GetNewRelsIDString(xmlDocument().document_element());// 2024-07-24: wrapper for relationship IDs with support for 64 bit random IDs
	XMLNode lastRelationship = xmlDocument().document_element().last_child_of_type(pugi::node_element); // see if there's a last element
	XMLNode node{}; // scope declaration
	// Create new node in the .rels file
	if(lastRelationship.empty())
		node = xmlDocument().document_element().prepend_child("Relationship");
	else { // if last element found
		// ===== Insert node after previous relationship
		node = xmlDocument().document_element().insert_child_after("Relationship", lastRelationship);
		// ===== Using whitespace nodes prior to lastRelationship as a template, insert whitespaces between lastRelationship and the new node
		XMLNode copyWhitespaceFrom = lastRelationship; // start looking for whitespace nodes before previous relationship
		XMLNode insertBefore = node;              // start inserting the same whitespace nodes before new relationship
		while(copyWhitespaceFrom.previous_sibling().type() == pugi::node_pcdata) { // empty node returns pugi::node_null
			// Advance to previous "template" whitespace node, ensured to exist in while-condition
			copyWhitespaceFrom = copyWhitespaceFrom.previous_sibling();
			// ===== Insert a whitespace node
			insertBefore = xmlDocument().document_element().insert_child_before(pugi::node_pcdata, insertBefore);
			insertBefore.set_value(copyWhitespaceFrom.value()); // copy the whitespace node value in sequence
		}
	}
	node.append_attribute("Id").set_value(id.c_str());
	node.append_attribute("Type").set_value(typeString.c_str());
	node.append_attribute("Target").set_value(target.c_str());
	if(type == XLRelationshipType::ExternalLinkPath) {
		node.append_attribute("TargetMode").set_value("External");
	}
	return XLRelationshipItem(node);
}
/**
 * @details check whether relationshipByTarget finds an XLRelationshipItem or returns an empty one
 * @note 2024-07-24: let relationshipByTarget perform the find with a match that is agnostic to a potential leading slash
 */
bool XLRelationships::targetExists(const std::string& target) const
{
	constexpr const bool DO_NOT_THROW = false; // const for code readability
	return not relationshipByTarget(target, DO_NOT_THROW).empty(); // target exists if relationshipByTarget.empty() returns false
	// return xmlDocument().document_element().find_child_by_attribute("Target", target.c_str()) != nullptr;
}

bool XLRelationships::idExists(const std::string& id) const
{
	return xmlDocumentC().document_element().find_child_by_attribute("Id", id.c_str()) != nullptr;
}
/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLRelationships::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }
//
// XLRow.cpp
//
namespace OpenXLSX {
	XLRow::XLRow() : m_rowNode(nullptr), m_sharedStrings(XLSharedStringsDefaulted), m_rowDataProxy(this, m_rowNode.get())
	{
	}
	/**
	 * @details Constructs a new XLRow object from information in the underlying XML file. A pointer to the corresponding
	 * node in the underlying XML file must be provided.
	 */
	XLRow::XLRow(const XMLNode& rowNode, const XLSharedStrings& sharedStrings) : m_rowNode(std::make_unique<XMLNode>(rowNode)),
		m_sharedStrings(sharedStrings), m_rowDataProxy(this, m_rowNode.get())
	{
	}

	XLRow::XLRow(const XLRow& other) : m_rowNode(other.m_rowNode ? std::make_unique<XMLNode>(*other.m_rowNode) : nullptr),
		m_sharedStrings(other.m_sharedStrings), m_rowDataProxy(this, m_rowNode.get())
	{
	}
	/**
	 * @details Because the m_rowDataProxy variable is tied to an exact XLRow object, the move operation is
	 * not a 'pure' move, as a new XLRowDataProxy has to be constructed.
	 */
	XLRow::XLRow(XLRow&& other) noexcept : m_rowNode(std::move(other.m_rowNode)), m_sharedStrings(std::move(other.m_sharedStrings)), m_rowDataProxy(this, m_rowNode.get())
	{
	}

	XLRow::~XLRow() = default;

	XLRow& XLRow::operator=(const XLRow& other)
	{
		if(&other != this) {
			auto temp = XLRow(other);
			std::swap(*this, temp);
		}
		return *this;
	}
	/**
	 * @details Because the m_rowDataProxy variable is tied to an exact XLRow object, the move operation is
	 * not a 'pure' move, as a new XLRowDataProxy has to be constructed.
	 */
	XLRow& XLRow::operator=(XLRow&& other) noexcept
	{
		if(&other != this) {
			m_rowNode = std::move(other.m_rowNode);
			m_sharedStrings = std::move(other.m_sharedStrings);
			m_rowDataProxy = XLRowDataProxy(this, m_rowNode.get());
		}
		return *this;
	}

	bool XLRow::empty() const { return (!m_rowNode) || m_rowNode->empty(); }
	XLRow::operator bool() const { return m_rowNode && (not m_rowNode->empty() ); }
	/**
	 * @details Returns the m_height member by getValue.
	 */
	double XLRow::height() const { return m_rowNode->attribute("ht").as_double(15.0); }
	/**
	 * @details Set the height of the row. This is done by setting the getValue of the 'ht' attribute and setting the
	 * 'customHeight' attribute to true.
	 */
	void XLRow::setHeight(float height)
	{
		// Set the 'ht' attribute for the Cell. If it does not exist, create it.
		if(m_rowNode->attribute("ht").empty())
			m_rowNode->append_attribute("ht") = height;
		else
			m_rowNode->attribute("ht").set_value(height);
		// Set the 'customHeight' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("customHeight").empty())
			m_rowNode->append_attribute("customHeight") = 1;
		else
			m_rowNode->attribute("customHeight").set_value(1);
	}
	/**
	 * @details Return the m_descent member by getValue.
	 */
	float XLRow::descent() const { return m_rowNode->attribute("x14ac:dyDescent").as_float(0.25); }
	/**
	 * @details Set the descent by setting the 'x14ac:dyDescent' attribute in the XML file
	 */
	void XLRow::setDescent(float descent)
	{
		// Set the 'x14ac:dyDescent' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("x14ac:dyDescent").empty())
			m_rowNode->append_attribute("x14ac:dyDescent") = descent;
		else
			m_rowNode->attribute("x14ac:dyDescent") = descent;
	}
	/**
	 * @details Determine if the row is hidden or not.
	 */
	bool XLRow::isHidden() const { return m_rowNode->attribute("hidden").as_bool(false); }
	/**
	 * @details Set the hidden state by setting the 'hidden' attribute to true or false.
	 */
	void XLRow::setHidden(bool state)
	{
		// Set the 'hidden' attribute. If it does not exist, create it.
		if(m_rowNode->attribute("hidden").empty())
			m_rowNode->append_attribute("hidden") = static_cast<int>(state);
		else
			m_rowNode->attribute("hidden").set_value(static_cast<int>(state));
	}
	/**
	 * @details
	 * @note 2024-08-18: changed return type of rowNumber to uint32
	 *       CAUTION: there is no validity check on the underlying XML (nor was there ever one in case a value was inconsistent with OpenXLSX::MAX_ROWS)
	 */
	uint32 XLRow::rowNumber() const { return static_cast<uint32>(m_rowNode->attribute("r").as_ullong()); }
	/**
	 * @details Get the number of cells in the row, by returning the size of the m_cells vector.
	 */
	uint16 XLRow::cellCount() const
	{
		const auto node = m_rowNode->last_child_of_type(pugi::node_element);
		return node.empty() ? 0 : XLCellReference(node.attribute("r").value()).column();
	}

	XLRowDataProxy & XLRow::values() { return m_rowDataProxy; }
	const XLRowDataProxy & XLRow::values() const { return m_rowDataProxy; }

	XLRowDataRange XLRow::cells() const
	{
		const XMLNode node = m_rowNode->last_child_of_type(pugi::node_element);
		if(node.empty())
			return XLRowDataRange()/*empty range*/;
		return XLRowDataRange(*m_rowNode, 1, XLCellReference(node.attribute("r").value()).column(), m_sharedStrings.get());
	}

	XLRowDataRange XLRow::cells(uint16 cellCount) const { return XLRowDataRange(*m_rowNode, 1, cellCount, m_sharedStrings.get()); }

	XLRowDataRange XLRow::cells(uint16 firstCell, uint16 lastCell) const
	{
		return XLRowDataRange(*m_rowNode, firstCell, lastCell, m_sharedStrings.get());
	}
	/**
	 * @details Find & return a cell in indicated column
	 */
	XLCell XLRow::findCell(uint16 columnNumber)
	{
		if(m_rowNode->empty())  
			return XLCell{};
		XMLNode cellNode = m_rowNode->last_child_of_type(pugi::node_element);
		// ===== If there are no cells in the current row, or the requested cell is beyond the last cell in the row...
		if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
			return XLCell{}; // fail

		// ===== If the requested node is closest to the end, start from the end and search backwards...
		if(XLCellReference(cellNode.attribute("r").value()).column() - columnNumber < columnNumber) {
			while(!cellNode.empty() && (XLCellReference(cellNode.attribute("r").value()).column() > columnNumber))
				cellNode = cellNode.previous_sibling_of_type(pugi::node_element);
			// ===== If the backwards search failed to locate the requested cell
			if(cellNode.empty() || (XLCellReference(cellNode.attribute("r").value()).column() < columnNumber))
				return XLCell{}; // fail
		}
		// ===== Otherwise, start from the beginning
		else {
			// ===== At this point, it is guaranteed that there is at least one node_element in the row that is not empty.
			cellNode = m_rowNode->first_child_of_type(pugi::node_element);
			// ===== It has been verified above that the requested columnNumber is <= the column number of the last node_element, therefore this loop will halt:
			while(XLCellReference(cellNode.attribute("r").value()).column() < columnNumber)
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
			// ===== If the forwards search failed to locate the requested cell
			if(XLCellReference(cellNode.attribute("r").value()).column() > columnNumber)
				return XLCell{}; // fail
		}
		return XLCell(cellNode, m_sharedStrings.get());
	}
	/**
	 * @details Determine the value of the style attribute "s" - if attribute does not exist, return default value
	 */
	XLStyleIndex XLRow::format() const { return m_rowNode->attribute("s").as_uint(XLDefaultCellFormat); }
	/**
	 * @brief Set the row style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
	 *        If the style attribute "s" does not exist, create it
	 */
	bool XLRow::setFormat(XLStyleIndex cellFormatIndex)
	{
		XMLAttribute customFormatAtt = m_rowNode->attribute("customFormat");
		if(cellFormatIndex != XLDefaultCellFormat) {
			if(customFormatAtt.empty()) {
				customFormatAtt = m_rowNode->append_attribute("customFormat");
				if(customFormatAtt.empty())  return false;// fail if missing customFormat attribute could not be created
			}
			customFormatAtt.set_value("true");
		}
		else { // cellFormatIndex is XLDefaultCellFormat
			if(not customFormatAtt.empty())  m_rowNode->remove_attribute(customFormatAtt);// an existing customFormat attribute should be deleted
		}
		XMLAttribute styleAtt = m_rowNode->attribute("s");
		if(styleAtt.empty()) {
			styleAtt = m_rowNode->append_attribute("s");
			if(styleAtt.empty())  return false;
		}
		styleAtt.set_value(cellFormatIndex);
		return true;
	}

	bool XLRow::isEqual(const XLRow& lhs, const XLRow& rhs)
	{
		// 2024-05-28 BUGFIX: (!lhs.m_rowNode && rhs.m_rowNode) was not evaluated, triggering a segmentation fault on dereferencing
		if(static_cast<bool>(lhs.m_rowNode) != static_cast<bool>(rhs.m_rowNode))  
			return false;
		// ===== If execution gets here, row nodes are BOTH valid or BOTH invalid / empty
		if(not lhs.m_rowNode)  
			return true;    // checking one for being empty is enough to know both are empty
		return *lhs.m_rowNode == *rhs.m_rowNode;
	}

	bool XLRow::isLessThan(const XLRow& lhs, const XLRow& rhs) { return *lhs.m_rowNode < *rhs.m_rowNode; }
	//
	//
	//
	XLRowIterator::XLRowIterator(const XLRowRange& rowRange, XLIteratorLocation loc) : m_dataNode(std::make_unique<XMLNode>(*rowRange.m_dataNode)),
		m_firstRow(rowRange.m_firstRow), m_lastRow(rowRange.m_lastRow), m_currentRow(), m_sharedStrings(rowRange.m_sharedStrings), m_endReached(false),
		m_hintRow(), m_hintRowNumber(0), m_currentRowStatus(XLNotLoaded), m_currentRowNumber(0)
	{
		if(loc == XLIteratorLocation::End)
			m_endReached = true;
		else {
			m_currentRowNumber = m_firstRow;
		}
	}

	XLRowIterator::~XLRowIterator() = default;

	XLRowIterator::XLRowIterator(const XLRowIterator& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)), m_firstRow(other.m_firstRow),
		m_lastRow(other.m_lastRow), m_currentRow(other.m_currentRow), m_sharedStrings(other.m_sharedStrings), m_endReached(other.m_endReached),
		m_hintRow(other.m_hintRow), m_hintRowNumber(other.m_hintRowNumber), m_currentRowStatus(other.m_currentRowStatus), m_currentRowNumber(other.m_currentRowNumber)
	{
	}

	XLRowIterator::XLRowIterator(XLRowIterator&& other) noexcept = default;

	XLRowIterator& XLRowIterator::operator=(const XLRowIterator& other)
	{
		if(&other != this) {
			auto temp = XLRowIterator(other);
			std::swap(*this, temp);
		}
		return *this;
	}

	XLRowIterator& XLRowIterator::operator=(XLRowIterator&& other) noexcept = default;
	/**
	 * @brief update m_currentRow by fetching (or inserting) a row at m_currentRowNumber
	 */
	void XLRowIterator::updateCurrentRow(bool createIfMissing)
	{
		// ===== Quick exit checks - can't be true when m_endReached
		if(m_currentRowStatus == XLLoaded)  
			return;                           // nothing to do, row is already loaded
		if(!createIfMissing && m_currentRowStatus == XLNoSuchRow)  
			return;    // nothing to do, row has already been determined as missing
		// ===== At this stage, m_currentRowStatus is XLUnloaded or XLNoSuchRow and createIfMissing == true
		if(m_endReached)
			throw XLInputError("XLRowIterator updateCurrentRow: iterator should not be dereferenced when endReached() == true");
		// ===== Row needs to be updated
		if(m_hintRow.empty()) {   // no hint has been established: fetch first row node the "tedious" way
			if(createIfMissing)  // getRowNode creates missing rows
				m_currentRow = XLRow(getRowNode(*m_dataNode, m_currentRowNumber), m_sharedStrings.get());
			else                // findRowNode returns an empty row for missing rows
				m_currentRow = XLRow(findRowNode(*m_dataNode, m_currentRowNumber), m_sharedStrings.get());
		}
		else {
			// ===== Find or create, and fetch an XLRow at m_currentRowNumber
			if(m_currentRowNumber > m_hintRowNumber) {
				// ===== Start from m_hintRow and search forwards...
				XMLNode rowNode = m_hintRow.next_sibling_of_type(pugi::node_element);
				uint32 rowNo = 0;
				while(!rowNode.empty()) {
					rowNo = static_cast<uint32>(rowNode.attribute("r").as_ullong());
					if(rowNo >= m_currentRowNumber)
						break;// if desired row was reached / passed, break before incrementing rowNode
					rowNode = rowNode.next_sibling_of_type(pugi::node_element);
				}
				if(rowNo != m_currentRowNumber)  
					rowNode = XMLNode{};// if a higher row number was found, set empty node (means: "missing")
				// ===== Create missing row node if createIfMissing == true
				if(createIfMissing && rowNode.empty()) {
					rowNode = m_dataNode->insert_child_after("row", m_hintRow);
					rowNode.append_attribute("r").set_value(m_currentRowNumber);
				}
				if(rowNode.empty()) // if row could not be found / created
					m_currentRow = XLRow{}; // make sure m_currentRow is set to an empty cell
				else
					m_currentRow = XLRow(rowNode, m_sharedStrings.get());
			}
			else
				throw XLInternalError("XLRowIterator::updateCurrentRow: an internal error occured (m_currentRowNumber <= m_hintRowNumber)");
		}

		if(m_currentRow.empty())    // if row is confirmed missing
			m_currentRowStatus = XLNoSuchRow; // mark this status for further calls to updateCurrentRow()
		else {
			// ===== If the current row exists, update the hints
			m_hintRow = *m_currentRow.m_rowNode;// don't store a full XLRow, just the XMLNode, for better performance
			m_hintRowNumber = m_currentRowNumber;
			m_currentRowStatus = XLLoaded;              // mark row status for further calls to updateCurrentRow()
		}
	}

	XLRowIterator& XLRowIterator::operator++()        // 2024-04-29: patched for whitespace
	{
		if(m_endReached)
			throw XLInputError("XLRowIterator: tried to increment beyond end operator");
		if(m_currentRowNumber < m_lastRow)
			++m_currentRowNumber;
		else
			m_endReached = true;
		m_currentRowStatus = XLNotLoaded; // trigger a new attempt to locate / create the row via updateRowCell
		return *this;
	}

	XLRowIterator XLRowIterator::operator++(int)
	{
		auto oldIter(*this);
		++(*this);
		return oldIter;
	}

	XLRow& XLRowIterator::operator*()
	{
		updateCurrentRow(XLCreateIfMissing);
		return m_currentRow;
	}

	XLRowIterator::pointer XLRowIterator::operator->()
	{
		updateCurrentRow(XLCreateIfMissing);
		return &m_currentRow;
	}

	bool XLRowIterator::operator==(const XLRowIterator& rhs) const
	{
		if(m_endReached && rhs.m_endReached)  return true;    // If both iterators are end iterators

		if(m_currentRowNumber != rhs.m_currentRowNumber)      // If iterators point to a different row
			return false;                                     // that means no match

		// CAUTION: for performance reasons, disabled all checks whether this and rhs are iterators on the same worksheet & row range
		return true;

		// if (*m_dataNode != *rhs.m_dataNode) return false;     // TBD: iterators over different worksheets may never match
		// TBD if iterators shall be considered not equal if they were created on different XLRowRanges
		// this would require checking the m_firstRow and m_lastRow, potentially costing CPU time

		// return m_currentRow == rhs.m_currentRow;   // match only if row nodes are equal
		// CAUTION: in the current code, that means iterators that point to the same row in different worksheets,
		// and rows that do not exist in both sheets, will be considered equal
	}

	bool XLRowIterator::operator!=(const XLRowIterator& rhs) const { return !(*this == rhs); }

	XLRowIterator::operator bool() const { return false; }

	bool XLRowIterator::rowExists()
	{
		// ===== Update m_currentRow once so that rowExists will always test the correct cell (an empty row if current row doesn't exist)
		updateCurrentRow(XLDoNotCreateIfMissing);
		return not m_currentRow.empty();
	}
	//
	//
	//
	XLRowRange::XLRowRange(const XMLNode& dataNode, uint32 first, uint32 last, const XLSharedStrings& sharedStrings) : 
		m_dataNode(std::make_unique<XMLNode>(dataNode)), m_firstRow(first), m_lastRow(last), m_sharedStrings(sharedStrings)
	{
	}

	XLRowRange::XLRowRange(const XLRowRange& other) : m_dataNode(std::make_unique<XMLNode>(*other.m_dataNode)), m_firstRow(other.m_firstRow),
		m_lastRow(other.m_lastRow), m_sharedStrings(other.m_sharedStrings)
	{
	}

	XLRowRange::XLRowRange(XLRowRange&& other) noexcept = default;
	XLRowRange::~XLRowRange() = default;

	XLRowRange& XLRowRange::operator=(const XLRowRange& other)
	{
		if(&other != this) {
			auto temp = XLRowRange(other);
			std::swap(*this, temp);
		}
		return *this;
	}
	XLRowRange& XLRowRange::operator=(XLRowRange&& other) noexcept = default;
	uint32 XLRowRange::rowCount() const { return m_lastRow - m_firstRow + 1; }
	XLRowIterator XLRowRange::begin() { return XLRowIterator(*this, XLIteratorLocation::Begin); }
	XLRowIterator XLRowRange::end() { return XLRowIterator(*this, XLIteratorLocation::End); }
}
//
// XLRowData.cpp
//
namespace OpenXLSX {
	/**
	 * @details Constructor.
	 * @pre The given range and location are both valid.
	 * @note 2024-05-28: added support for constructing with an empty m_cellNode from an empty rowDataRange which will allow obtaining
	 *       an XLIteratorLocation::End for such a range so that iterations can fail in a controlled manner
	 */
	XLRowDataIterator::XLRowDataIterator(const XLRowDataRange& rowDataRange, XLIteratorLocation loc) : 
		m_dataRange(std::make_unique<XLRowDataRange>(rowDataRange)),
		m_cellNode(std::make_unique<XMLNode>(getCellNode((m_dataRange->size() ? *m_dataRange->m_rowNode : XMLNode {}), m_dataRange->m_firstCol))),
		m_currentCell(loc == XLIteratorLocation::End ? XLCell() : XLCell(*m_cellNode, m_dataRange->m_sharedStrings.get()))
	{
	}
	XLRowDataIterator::~XLRowDataIterator() = default;
	/**
	 * @details Copy constructor. Trivial implementation with deep copy of pointer members.
	 */
	XLRowDataIterator::XLRowDataIterator(const XLRowDataIterator& other) : m_dataRange(std::make_unique<XLRowDataRange>(*other.m_dataRange)),
		m_cellNode(std::make_unique<XMLNode>(*other.m_cellNode)), m_currentCell(other.m_currentCell)
	{
	}

	XLRowDataIterator::XLRowDataIterator(XLRowDataIterator&& other) noexcept = default;

	XLRowDataIterator& XLRowDataIterator::operator=(const XLRowDataIterator& other)
	{
		if(&other != this) {
			XLRowDataIterator temp = other;
			std::swap(temp, *this);
		}
		return *this;
	}

	XLRowDataIterator& XLRowDataIterator::operator=(XLRowDataIterator&& other) noexcept = default;
	/**
	 * @details Pre-increment operator. Advances the iterator one element.
	 */
	XLRowDataIterator& XLRowDataIterator::operator++()
	{
		// ===== Compute the column number, and move the m_cellNode to the next sibling.
		const uint16 cellNumber = m_currentCell.cellReference().column() + 1;
		XMLNode cellNode = m_currentCell.m_cellNode->next_sibling_of_type(pugi::node_element);
		// ===== If the cellNumber exceeds the last column in the range,
		// ===== m_currentCell is set to an empty XLCell, indicating the end of the range has been reached.
		if(cellNumber > m_dataRange->m_lastCol)  
			m_currentCell = XLCell();
		// ====== If the cellNode is empty (i.e. no more children in the current row node) or the column number of the cell node
		// ====== is higher than the computed column number, then insert the node.
		// BUG BUGFIX 2024-04-26: check was for m_cellNode->empty(), allowing an invalid test for the attribute r, discovered
		//       because the modified XLCellReference throws an exception on invalid parameter
		else if(cellNode.empty() || XLCellReference(cellNode.attribute("r").value()).column() > cellNumber) {
			cellNode = m_dataRange->m_rowNode->insert_child_after("c", *m_currentCell.m_cellNode);
			setDefaultCellAttributes(cellNode, XLCellReference(static_cast<uint32>(m_dataRange->m_rowNode->attribute("r").as_ullong()), cellNumber).address(),
				/**/ *m_dataRange->m_rowNode, cellNumber);
			m_currentCell = XLCell(cellNode, m_dataRange->m_sharedStrings.get());
		}
		// ===== Otherwise, the cell node and the column number match.
		else {
			assert(XLCellReference(cellNode.attribute("r").value()).column() == cellNumber);
			m_currentCell = XLCell(cellNode, m_dataRange->m_sharedStrings.get());
		}
		return *this;
	}
	/**
	 * @details Post-increment operator. Implemented in terms of the pre-increment operator.
	 */
	XLRowDataIterator XLRowDataIterator::operator++(int)
	{
		auto oldIter(*this);
		++(*this);
		return oldIter;
	}
	/**
	 * @details Dereferencing operator.
	 */
	XLCell& XLRowDataIterator::operator*() { return m_currentCell; }
	/**
	 * @details Arrow operator.
	 */
	XLRowDataIterator::pointer XLRowDataIterator::operator->() { return &m_currentCell; }
	/**
	 * @details Equality comparison operator.
	 */
	bool XLRowDataIterator::operator==(const XLRowDataIterator& rhs) const
	{
		// 2024-05-28 BUGFIX: (!m_currentCell && rhs.m_currentCell) was not evaluated, triggering a segmentation fault on dereferencing
		if(static_cast<bool>(m_currentCell) != static_cast<bool>(rhs.m_currentCell))  
			return false;
		// ===== If execution gets here, current cells are BOTH valid or BOTH invalid / empty
		if(!m_currentCell)  
			return true; // checking one for being empty is enough to know both are empty
		return m_currentCell == rhs.m_currentCell;
	}
	/**
	 * @details Non-equality comparison operator.
	 */
	bool XLRowDataIterator::operator!=(const XLRowDataIterator& rhs) const { return !(*this == rhs); }

	XLRowDataRange::XLRowDataRange(const XMLNode& rowNode, uint16 firstColumn, uint16 lastColumn, const XLSharedStrings& sharedStrings) : 
		m_rowNode(std::make_unique<XMLNode>(rowNode)), m_firstCol(firstColumn), m_lastCol(lastColumn), m_sharedStrings(sharedStrings)
	{
		if(lastColumn < firstColumn) {
			m_firstCol = 1;
			m_lastCol  = 1;
			throw XLOverflowError("lastColumn is less than firstColumn.");
		}
	}
	/**
	 * @details Constructs an empty XLDataRange, whose size() will return 0. To be used as return value in functions that shall fail without exception.
	 */
	XLRowDataRange::XLRowDataRange() : m_rowNode(nullptr), m_firstCol(1)/*first col of 1*/,
		m_lastCol(0)/*and last col of 0 will ensure that size returns 0*/, m_sharedStrings(XLSharedStringsDefaulted)
	{
	}
	/**
	 * @details Copy constructor. Trivial implementation.
	 */
	XLRowDataRange::XLRowDataRange(const XLRowDataRange& other)
		: m_rowNode((other.m_rowNode != nullptr) ? std::make_unique<XMLNode>(*other.m_rowNode) : nullptr),    // 2024-05-28: support for copy-construction from an empty XLDataRange
		m_firstCol(other.m_firstCol), m_lastCol(other.m_lastCol), m_sharedStrings(other.m_sharedStrings)
	{
	}
	/**
	 * @details Move constructor. Default implementation.
	 */
	XLRowDataRange::XLRowDataRange(XLRowDataRange&& other) noexcept = default;
	XLRowDataRange::~XLRowDataRange() = default;
	/**
	 * @details Copy assignment operator. Implemented in terms of copy-and-swap.
	 */
	XLRowDataRange& XLRowDataRange::operator=(const XLRowDataRange& other)
	{
		if(&other != this) {
			XLRowDataRange temp(other);
			std::swap(temp, *this);
		}
		return *this;
	}
	/**
	 * @details Move assignment operator. Default implementation.
	 */
	XLRowDataRange& XLRowDataRange::operator=(XLRowDataRange&& other) noexcept = default;
	/**
	 * @details Calculates the size (number of cells) in the range.
	 */
	uint16 XLRowDataRange::size() const { return m_lastCol - m_firstCol + 1; }
	/**
	 * @details Get an iterator to the first cell in the range.
	 * @note 2024-05-28: enhanced ::begin() to return an end iterator for an empty range
	 */
	XLRowDataIterator XLRowDataRange::begin() { return XLRowDataIterator { *this, (size() > 0 ? XLIteratorLocation::Begin : XLIteratorLocation::End) }; }
	/**
	 * @details Get an iterator to (one past) the last cell in the range.
	 */
	XLRowDataIterator XLRowDataRange::end() { return XLRowDataIterator { *this, XLIteratorLocation::End }; }
	XLRowDataProxy::~XLRowDataProxy() = default;
	/**
	 * @details Copy constructor. This is not a 'true' copy constructor, as it is the row values that will
	 * be copied, not the XLRowDataProxy member variables (pointers to the XLRow and row node objects).
	 */
	XLRowDataProxy& XLRowDataProxy::operator=(const XLRowDataProxy& other)
	{
		if(&other != this) {
			*this = other.getValues();
		}
		return *this;
	}
	/**
	 * @details Constructor
	 */
	XLRowDataProxy::XLRowDataProxy(XLRow* row, XMLNode* rowNode) : m_row(row), m_rowNode(rowNode) 
	{
	}
	/**
	 * @details Copy constructor. Default implementation.
	 */
	XLRowDataProxy::XLRowDataProxy(const XLRowDataProxy& other) = default;
	/**
	 * @details Move constructor. Default implementation.
	 */
	XLRowDataProxy::XLRowDataProxy(XLRowDataProxy&& other) noexcept = default;
	/**
	 * @details Move assignment operator. Default implementation.
	 */
	XLRowDataProxy& XLRowDataProxy::operator=(XLRowDataProxy&& other) noexcept = default;
	/**
	 * @details Assignment operator taking a std::vector of XLCellValue objects as an argument. Other container types
	 * and/or value types will be handled by the templated operator=. However, because assigning a std::vector of
	 * XLCellValue object is the most common case, this case is handled separately for higher performance.
	 */
	XLRowDataProxy& XLRowDataProxy::operator=(const std::vector<XLCellValue>& values) // 2024-04-30: whitespace support
	{
		if(values.size() > MAX_COLS)  
			throw XLOverflowError("vector<XLCellValue> size exceeds maximum number of columns.");
		if(values.empty())  
			return *this;
		deleteCellValues(static_cast<uint16>(values.size()));    // 2024-04-30: whitespace support
		// ===== prepend new cell nodes to current row node
		XMLNode curNode{};
		uint16 colNo = static_cast<uint16>(values.size());
		for(auto value = values.rbegin(); value != values.rend(); ++value) {
			curNode = m_rowNode->prepend_child("c");
			setDefaultCellAttributes(curNode, XLCellReference(static_cast<uint32>(m_row->rowNumber()), colNo).address(), *m_rowNode, colNo);
			XLCell(curNode, m_row->m_sharedStrings.get()).value() = *value;
			--colNo;
		}
		return *this;
	}
	/**
	 * @details Assignment operator taking a std::vector of bool values as an argument. Under most circumstances,
	 * one of the templated assignment operators should do the job. However, because std::vector<bool> is handled
	 * differently than other value types, some compilers don't play well with using std::vector<bool> in a
	 * template function. Therefore this edge case is handled separately.
	 */
	XLRowDataProxy& XLRowDataProxy::operator=(const std::vector<bool>& values)        // 2024-04-30: whitespace support
	{
		if(values.size() > MAX_COLS)  throw XLOverflowError("vector<bool> size exceeds maximum number of columns.");
		if(values.empty())  return *this;
		auto range = XLRowDataRange(*m_rowNode, 1, static_cast<uint16>(values.size()), m_row->m_sharedStrings.get());
		auto dst = range.begin(); // 2024-04-30: whitespace support: safe because XLRowDataRange::begin invokes whitespace-safe getCellNode for column 1
		auto src = values.begin();
		while(true) {
			dst->value() = static_cast<bool>(*src);
			++src;
			if(src == values.end())  
				break;
			++dst; // 2024-04-30: whitespace support: XLRowDataIterator::operator++ is whitespace-safe
		}
		return *this;
	}
	/**
	 * @details This function simply calls the getValues() function, which returns a std::vector of XLCellValues as required.
	 */
	XLRowDataProxy::operator std::vector<XLCellValue>() const { return getValues(); }
	/**
	 * @details Calls the convertContainer convenience function with a std::deque of XLCellValues as an argument.
	 */
	XLRowDataProxy::operator std::deque<XLCellValue>() const { return convertContainer<std::deque<XLCellValue> >(); }
	/**
	 * @details Calls the convertContainer convenience function with a std::list of XLCellValues as an argument.
	 */
	XLRowDataProxy::operator std::list<XLCellValue>() const { return convertContainer<std::list<XLCellValue> >(); }
	/**
	 * @details Iterates through the cell values (if any) for the current row, and copies them to an output std::vector of XLCellValues.
	 */
	std::vector<XLCellValue> XLRowDataProxy::getValues() const        // 2024-04-30: whitespace support
	{
		// ===== Determine the number of cells in the current row. Create a std::vector of the same size.
		const XMLNode lastElementChild = m_rowNode->last_child_of_type(pugi::node_element);
		const uint16 numCells = (lastElementChild.empty() ? 0 : XLCellReference(lastElementChild.attribute("r").value()).column());
		std::vector<XLCellValue> result(static_cast<uint64>(numCells));
		// ===== If there are one or more cells in the current row, iterate through them and add the value to the container.
		if(numCells > 0) {
			XMLNode node = lastElementChild; // avoid unneeded call to first_child_of_type by iterating backwards, vector is random access so it doesn't matter
			while(!node.empty()) {
				result[XLCellReference(node.attribute("r").value()).column() - 1] = XLCell(node, m_row->m_sharedStrings.get()).value();
				node = node.previous_sibling_of_type(pugi::node_element);
			}
		}
		return result; // ===== Return the resulting container.
	}
	/**
	 * @details The function returns a reference to the XLSharedStrings object embedded in the m_row member.
	 * This is required because the XLRow class internals is not visible in the header file.
	 */
	const XLSharedStrings& XLRowDataProxy::getSharedStrings() const { return m_row->m_sharedStrings.get(); }
	/**
	 * @details The deleteCellValues is a convenience function used solely by the templated operator= function.
	 * The purpose of a separate function is to keep details of xml_node out of the header file.
	 */
	void XLRowDataProxy::deleteCellValues(uint16 count) // 2024-04-30: whitespace support
	{
		// ===== Mark cell nodes for deletion
		std::vector<XMLNode> toBeDeleted;
		XMLNode cellNode = m_rowNode->first_child_of_type(pugi::node_element);
		while(!cellNode.empty()) {
			if(XLCellReference(cellNode.attribute("r").value()).column() <= count) {
				toBeDeleted.emplace_back(cellNode);
				XMLNode nextNode = cellNode.next_sibling(); // get next "regular" sibling (any type) before advancing cellNode
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
				// ===== Iterate over non-element nodes and mark them for deletion
				while(nextNode != cellNode) { // this also works with the empty node returned past last sibling, as for XMLNode a{}, b{}, ( a == b ) is true
					toBeDeleted.emplace_back(nextNode);
					nextNode = nextNode.next_sibling();
				}
			}
			else
				cellNode = cellNode.next_sibling_of_type(pugi::node_element);
		}
		// ===== Delete selected cell nodes
		for(auto cellNodeToDelete : toBeDeleted)  m_rowNode->remove_child(cellNodeToDelete);
	}
	/**
	 * @details The prependCellValue is a convenience function used solely by the templated operator= function.
	 * The purpose of a separate function is to keep details of xml_node out of the header file.
	 * Note that no checking on the column number is made.
	 */
	void XLRowDataProxy::prependCellValue(const XLCellValue& value, uint16 col) // 2024-04-30: whitespace support
	{
		// ===== (disabled) Pretty formatting by inserting before an existing first child
		// XMLNode first_child = m_rowNode->first_child_of_type(pugi::node_element);
		// XMLNode curNode{};
		// if (first_child.empty())
		//     curNode = m_rowNode->prepend_child("c");
		// else
		//     curNode = m_rowNode->insert_child_before("c", first_child);

		XMLNode curNode = m_rowNode->prepend_child("c");    // this will correctly insert a new cell directly at the beginning of the row
		setDefaultCellAttributes(curNode, XLCellReference(static_cast<uint32>(m_row->rowNumber()), col).address(), *m_rowNode, col);
		XLCell(curNode, m_row->m_sharedStrings.get()).value() = value;
	}

	void XLRowDataProxy::clear() { m_rowNode->remove_children(); }
}
//
// XLSharedStrings.cpp
//
namespace OpenXLSX {
	const XLSharedStrings XLSharedStringsDefaulted{};
}
/**
 * @details Constructs a new XLSharedStrings object. Only one (common) object is allowed per XLDocument instance.
 * A filepath to the underlying XML file must be provided.
 */
XLSharedStrings::XLSharedStrings(XLXmlData* xmlData, std::deque<std::string>* stringCache) : XLXmlFile(xmlData), m_stringCache(stringCache)
{
	OXlXmlDoc & doc = xmlDocument();
	if(doc.document_element().empty()) // handle a bad (no document element) xl/sharedStrings.xml
		doc.load_string(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n" // pull request #192 -> remove count &
			                                                                              // uniqueCount as they are optional
			// 2024-09-03: removed empty string entry on creation, as it appears to just waste a string index that will never be used
			// "  <si>\n"
			// "    <t/>\n"
			// "  </si>\n"
			"</sst>",
			pugi_parse_settings);
}

XLSharedStrings::~XLSharedStrings() = default;
/**
 * @details Look up a string index by the string content. If the string does not exist, the returned index is -1.
 */
int32_t XLSharedStrings::getStringIndex(const std::string& str) const
{
	const auto iter = std::find_if(m_stringCache->begin(), m_stringCache->end(), [&](const std::string& s) { return str == s; });
	return iter == m_stringCache->end() ? -1 : static_cast<int32_t>(std::distance(m_stringCache->begin(), iter));
}

bool XLSharedStrings::stringExists(const std::string& str) const { return getStringIndex(str) >= 0; }

const char* XLSharedStrings::getString(int32_t index) const
{
	if(index < 0 || static_cast<size_t>(index) >= m_stringCache->size()) { // 2024-04-30: added range check
		using namespace std::literals::string_literals;
		throw XLInternalError("XLSharedStrings::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	}
	return (*m_stringCache)[index].c_str();
}
/**
 * @details Append a string by creating a new node in the XML file and adding the string to it. The index to the
 * shared string is returned
 */
int32_t XLSharedStrings::appendString(const std::string& str) const
{
	// size_t stringCacheSize = std::distance(m_stringCache->begin(), m_stringCache->end()); // any reason why .size() would not work?
	size_t stringCacheSize = m_stringCache->size(); // 2024-05-31: analogous with already added range check in getString
	if(stringCacheSize >= XLMaxSharedStrings) {    // 2024-05-31: added range check
		using namespace std::literals::string_literals;
		throw XLInternalError("XLSharedStrings::"s + __func__ + ": exceeded max strings count "s + std::to_string(XLMaxSharedStrings));
	}
	auto textNode = xmlDocumentC().document_element().append_child("si").append_child("t");
	if((!str.empty()) && (str.front() == ' ' || str.back() == ' '))
		textNode.append_attribute("xml:space").set_value("preserve"); // pull request #161
	textNode.text().set(str.c_str());
	m_stringCache->emplace_back(textNode.text().get()); // index of this element = previous stringCacheSize
	return static_cast<int32_t>(stringCacheSize);
}
/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLSharedStrings::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }

/**
 * @details Clear the string at the given index. This will affect the entire spreadsheet; everywhere the shared string
 * is used, it will be erased.
 * @note: 2024-05-31 DONE: index now int32_t everywhere, 2 billion shared strings should be plenty
 */
void XLSharedStrings::clearString(int32_t index) const   // 2024-04-30: whitespace support
{
	if(index < 0 || static_cast<size_t>(index) >= m_stringCache->size()) { // 2024-04-30: added range check
		using namespace std::literals::string_literals;
		throw XLInternalError("XLSharedStrings::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	}
	(*m_stringCache)[index] = "";
	// auto iter = xmlDocument().document_element().children().begin();
	// std::advance(iter, index);
	// iter->text().set(""); // 2024-04-30: BUGFIX: this was never going to work, <si> entries can be plenty that need to be cleared,
	// including formatting

	/* 2024-04-30 CAUTION: performance critical - with whitespace support, the function can no longer know the exact iterator position of
	 *   the shared string to be cleared - TBD what to do instead?
	 * Potential solution: store the XML child position with each entry in m_stringCache in a std::deque<struct entry>
	 *   with struct entry { std::string s; uint64 xmlChildIndex; };
	 */
	XMLNode sharedStringNode = xmlDocumentC().document_element().first_child_of_type(pugi::node_element);
	int32_t sharedStringPos = 0;
	while(sharedStringPos < index && not sharedStringNode.empty()) {
		sharedStringNode = sharedStringNode.next_sibling_of_type(pugi::node_element);
		++sharedStringPos;
	}
	if(not sharedStringNode.empty()) { // index was found
		sharedStringNode.remove_children(); // clear all data and formatting
		sharedStringNode.append_child("t"); // append an empty text node
	}
}

int32_t XLSharedStrings::rewriteXmlFromCache()
{
	int32_t writtenStrings = 0;
	xmlDocument().document_element().remove_children(); // clear all existing XML
	for(std::string& s : *m_stringCache) {
		XMLNode textNode = xmlDocument().document_element().append_child("si").append_child("t");
		if((!s.empty()) && (s.front() == ' ' || s.back() == ' '))
			textNode.append_attribute("xml:space").set_value("preserve"); // preserve spaces at begin/end of string
		textNode.text().set(s.c_str());
		++writtenStrings;
	}
	return writtenStrings;
}
//
// XLSheet.cpp
//
namespace OpenXLSX {
	// // Forward declaration. Implementation is in the XLUtilities.hpp file
	// XMLNode getRowNode(XMLNode sheetDataNode, uint32 rowNumber);
	/**
	 * @brief Function for setting tab color.
	 */
	void setTabColor(const OXlXmlDoc& xmlDocument, const XLColor& color)
	{
		if(!xmlDocument.document_element().child("sheetPr"))  
			xmlDocument.document_element().prepend_child("sheetPr");
		if(!xmlDocument.document_element().child("sheetPr").child("tabColor"))
			xmlDocument.document_element().child("sheetPr").prepend_child("tabColor");
		auto colorNode = xmlDocument.document_element().child("sheetPr").child("tabColor");
		for(auto attr : colorNode.attributes())  
			colorNode.remove_attribute(attr);
		colorNode.prepend_attribute("rgb").set_value(color.hex().c_str());
	}
	/**
	 * @brief Set the tab selected property to desired value
	 */
	void setTabSelected(const OXlXmlDoc& xmlDocument, bool selected)
	{        // 2024-04-30: whitespace support
		uint value = (selected ? 1 : 0);
		XMLNode sheetView = xmlDocument.document_element().child("sheetViews").first_child_of_type(pugi::node_element);
		XMLAttribute tabSelected = sheetView.attribute("tabSelected");
		if(tabSelected.empty())
			tabSelected = sheetView.prepend_attribute("tabSelected"); // BUGFIX 2025-03-15 issue #337: assign tabSelected value with newly created attribute if it didn't exist
		tabSelected.set_value(value);
	}
	/**
	 * @brief Function for checking if the tab is selected.
	 * @param xmlDocument
	 * @return
	 * @note pugi::xml_attribute::as_bool defaults to false for a non-existing (= empty) attribute
	 */
	bool tabIsSelected(const OXlXmlDoc& xmlDocument)
	{        // 2024-04-30: whitespace support
		return xmlDocument.document_element()
			   .child("sheetViews")
			   .first_child_of_type(pugi::node_element)
			   .attribute("tabSelected")
			   .as_bool(); // BUGFIX 2024-05-01: .value() "0" was evaluating to true
	}
	/**
	 * @brief get the correct XLCfType from the OOXML cfRule type attribute string
	 * @param typeString the string as used in the OOXML
	 * @return the corresponding XLCfType enum value
	 */
	XLCfType XLCfTypeFromString(std::string const& typeString)
	{
		if(typeString == "expression")         return XLCfType::Expression;
		if(typeString == "cellIs")             return XLCfType::CellIs;
		if(typeString == "colorScale")         return XLCfType::ColorScale;
		if(typeString == "dataBar")            return XLCfType::DataBar;
		if(typeString == "iconSet")            return XLCfType::IconSet;
		if(typeString == "top10")              return XLCfType::Top10;
		if(typeString == "uniqueValues")       return XLCfType::UniqueValues;
		if(typeString == "duplicateValues")    return XLCfType::DuplicateValues;
		if(typeString == "containsText")       return XLCfType::ContainsText;
		if(typeString == "notContainsText")    return XLCfType::NotContainsText;
		if(typeString == "beginsWith")         return XLCfType::BeginsWith;
		if(typeString == "endsWith")           return XLCfType::EndsWith;
		if(typeString == "containsBlanks")     return XLCfType::ContainsBlanks;
		if(typeString == "notContainsBlanks")  return XLCfType::NotContainsBlanks;
		if(typeString == "containsErrors")     return XLCfType::ContainsErrors;
		if(typeString == "notContainsErrors")  return XLCfType::NotContainsErrors;
		if(typeString == "timePeriod")         return XLCfType::TimePeriod;
		if(typeString == "aboveAverage")       return XLCfType::AboveAverage;
		return XLCfType::Invalid;
	}

	/**
	 * @brief inverse of XLCfTypeFromString
	 * @param cfType the type for which to get the OOXML string
	 */
	std::string XLCfTypeToString(XLCfType cfType)
	{
		switch(cfType) {
			case XLCfType::Expression:         return "expression";
			case XLCfType::CellIs:             return "cellIs";
			case XLCfType::ColorScale:         return "colorScale";
			case XLCfType::DataBar:            return "dataBar";
			case XLCfType::IconSet:            return "iconSet";
			case XLCfType::Top10:              return "top10";
			case XLCfType::UniqueValues:       return "uniqueValues";
			case XLCfType::DuplicateValues:    return "duplicateValues";
			case XLCfType::ContainsText:       return "containsText";
			case XLCfType::NotContainsText:    return "notContainsText";
			case XLCfType::BeginsWith:         return "beginsWith";
			case XLCfType::EndsWith:           return "endsWith";
			case XLCfType::ContainsBlanks:     return "containsBlanks";
			case XLCfType::NotContainsBlanks:  return "notContainsBlanks";
			case XLCfType::ContainsErrors:     return "containsErrors";
			case XLCfType::NotContainsErrors:  return "notContainsErrors";
			case XLCfType::TimePeriod:         return "timePeriod";
			case XLCfType::AboveAverage:       return "aboveAverage";
			case XLCfType::Invalid:            [[fallthrough]];
			default:                           return "(invalid)";
		}
	}

	/**
	 * @brief get the correct XLCfOperator from the OOXML cfRule operator attribute string
	 * @param operatorString the string as used in the OOXML
	 * @return the corresponding XLCfOperator enum value
	 */
	XLCfOperator XLCfOperatorFromString(std::string const& operatorString)
	{
		if(operatorString == "lessThan")            return XLCfOperator::LessThan;
		if(operatorString == "lessThanOrEqual")     return XLCfOperator::LessThanOrEqual;
		if(operatorString == "equal")               return XLCfOperator::Equal;
		if(operatorString == "notEqual")            return XLCfOperator::NotEqual;
		if(operatorString == "greaterThanOrEqual")  return XLCfOperator::GreaterThanOrEqual;
		if(operatorString == "greaterThan")         return XLCfOperator::GreaterThan;
		if(operatorString == "between")             return XLCfOperator::Between;
		if(operatorString == "notBetween")          return XLCfOperator::NotBetween;
		if(operatorString == "containsText")        return XLCfOperator::ContainsText;
		if(operatorString == "notContains")         return XLCfOperator::NotContains;
		if(operatorString == "beginsWith")          return XLCfOperator::BeginsWith;
		if(operatorString == "endsWith")            return XLCfOperator::EndsWith;
		return XLCfOperator::Invalid;
	}

	/**
	 * @brief inverse of XLCfOperatorFromString
	 * @param cfOperator the XLCfOperator for which to get the OOXML string
	 */
	std::string XLCfOperatorToString(XLCfOperator cfOperator)
	{
		switch(cfOperator) {
			case XLCfOperator::LessThan:            return "lessThan";
			case XLCfOperator::LessThanOrEqual:     return "lessThanOrEqual";
			case XLCfOperator::Equal:               return "equal";
			case XLCfOperator::NotEqual:            return "notEqual";
			case XLCfOperator::GreaterThanOrEqual:  return "greaterThanOrEqual";
			case XLCfOperator::GreaterThan:         return "greaterThan";
			case XLCfOperator::Between:             return "between";
			case XLCfOperator::NotBetween:          return "notBetween";
			case XLCfOperator::ContainsText:        return "containsText";
			case XLCfOperator::NotContains:         return "notContains";
			case XLCfOperator::BeginsWith:          return "beginsWith";
			case XLCfOperator::EndsWith:            return "endsWith";
			case XLCfOperator::Invalid:             [[fallthrough]];
			default:                                return "(invalid)";
		}
	}

	/**
	 * @brief get the correct XLCfTimePeriod from the OOXML cfRule timePeriod attribute string
	 * @param operatorString the string as used in the OOXML
	 * @return the corresponding XLCfTimePeriod enum value
	 */
	XLCfTimePeriod XLCfTimePeriodFromString(std::string const& timePeriodString)
	{
		if(timePeriodString == "today")      return XLCfTimePeriod::Today;
		if(timePeriodString == "yesterday")  return XLCfTimePeriod::Yesterday;
		if(timePeriodString == "tomorrow")   return XLCfTimePeriod::Tomorrow;
		if(timePeriodString == "last7Days")  return XLCfTimePeriod::Last7Days;
		if(timePeriodString == "thisMonth")  return XLCfTimePeriod::ThisMonth;
		if(timePeriodString == "lastMonth")  return XLCfTimePeriod::LastMonth;
		if(timePeriodString == "nextMonth")  return XLCfTimePeriod::NextMonth;
		if(timePeriodString == "thisWeek")   return XLCfTimePeriod::ThisWeek;
		if(timePeriodString == "lastWeek")   return XLCfTimePeriod::LastWeek;
		if(timePeriodString == "nextWeek")   return XLCfTimePeriod::NextWeek;
		return XLCfTimePeriod::Invalid;
	}

	/**
	 * @brief inverse of XLCfOperatorFromString
	 * @param cfOperator the XLCfTimePeriod for which to get the OOXML string
	 */
	std::string XLCfTimePeriodToString(XLCfTimePeriod cfTimePeriod)
	{
		switch(cfTimePeriod) {
			case XLCfTimePeriod::Today:     return "today";
			case XLCfTimePeriod::Yesterday: return "yesterday";
			case XLCfTimePeriod::Tomorrow:  return "tomorrow";
			case XLCfTimePeriod::Last7Days: return "last7Days";
			case XLCfTimePeriod::ThisMonth: return "thisMonth";
			case XLCfTimePeriod::LastMonth: return "lastMonth";
			case XLCfTimePeriod::NextMonth: return "nextMonth";
			case XLCfTimePeriod::ThisWeek:  return "thisWeek";
			case XLCfTimePeriod::LastWeek:  return "lastWeek";
			case XLCfTimePeriod::NextWeek:  return "nextWeek";
			case XLCfTimePeriod::Invalid:   [[fallthrough]];
			default:                        return "(invalid)";
		}
	}
} // namespace OpenXLSX

// ========== XLSheet Member Functions

/**
 * @details The constructor begins by constructing an instance of its superclass, XLAbstractXMLFile. The default
 * sheet type is WorkSheet and the default sheet state is Visible.
 */
XLSheet::XLSheet(XLXmlData* xmlData) : XLXmlFile(xmlData)
{
	if(xmlData->getXmlType() == XLContentType::Worksheet)
		m_sheet = XLWorksheet(xmlData);
	else if(xmlData->getXmlType() == XLContentType::Chartsheet)
		m_sheet = XLChartsheet(xmlData);
	else
		throw XLInternalError("Invalid XML data.");
}

/**
 * @details This method uses visitor pattern to return the name() member variable of the underlying
 * sheet object (XLWorksheet or XLChartsheet).
 */
std::string XLSheet::name() const
{
	return std::visit([](auto&& arg) {
		return arg.name();
	}, m_sheet);
}

/**
 * @details This method sets the name of the sheet to a new name, by calling the setName()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
void XLSheet::setName(const std::string& name)
{
	std::visit([&](auto&& arg) {
		return arg.setName(name);
	}, m_sheet);
}

/**
 * @details This method uses visitor pattern to return the visibility() member variable of the underlying
 * sheet object (XLWorksheet or XLChartsheet).
 */
XLSheetState XLSheet::visibility() const
{
	return std::visit([](auto&& arg) {
		return arg.visibility();
	}, m_sheet);
}

/**
 * @details This method sets the visibility state of the sheet, by calling the setVisibility()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
void XLSheet::setVisibility(XLSheetState state)
{
	std::visit([&](auto&& arg) {
		return arg.setVisibility(state);
	}, m_sheet);
}

/**
 * @details This method uses visitor pattern to return the color() member variable of the underlying
 * sheet object (XLWorksheet or XLChartsheet).
 */
XLColor XLSheet::color() const
{
	return std::visit([](auto&& arg) {
		return arg.color();
	}, m_sheet);
}

/**
 * @details This method sets the color of the sheet, by calling the setColor()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
void XLSheet::setColor(const XLColor& color)
{
	std::visit([&](auto&& arg) {
		return arg.setColor(color);
	}, m_sheet);
}

/**
 * @details This reports the selection state of the sheet, by calling the isSelected()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
bool XLSheet::isSelected() const
{
	return std::visit([](auto&& arg) {
		return arg.isSelected();
	}, m_sheet);
}

/**
 * @details This method sets the selection state of the sheet, by calling the setSelected()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
void XLSheet::setSelected(bool selected)
{
	std::visit([&](auto&& arg) {
		return arg.setSelected(selected);
	}, m_sheet);
}

/**
 * @details This reports the active state of the sheet, by calling the isActive()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
bool XLSheet::isActive() const
{
	return std::visit([](auto&& arg) {
		return arg.isActive();
	}, m_sheet);
}

/**
 * @details This method sets the active state of the sheet, by calling the setActive()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
bool XLSheet::setActive()
{
	return std::visit([&](auto&& arg) {
		return arg.setActive();
	}, m_sheet);
}

/**
 * @details Clones the sheet by calling the clone() method in the underlying sheet object
 * (XLWorksheet or XLChartsheet), using the visitor pattern.
 */
void XLSheet::clone(const std::string& newName)
{
	std::visit([&](auto&& arg) {
		arg.clone(newName);
	}, m_sheet);
}

/**
 * @details Get the index of the sheet, by calling the index() method in the underlying
 * sheet object (XLWorksheet or XLChartsheet), using the visitor pattern.
 */
uint16 XLSheet::index() const
{
	return std::visit([](auto&& arg) {
		return arg.index();
	}, m_sheet);
}

/**
 * @details This method sets the index of the sheet (i.e. move the sheet), by calling the setIndex()
 * member function of the underlying sheet object (XLWorksheet or XLChartsheet).
 */
void XLSheet::setIndex(uint16 index)
{
	std::visit([&](auto&& arg) {
		arg.setIndex(index);
	}, m_sheet);
}

/**
 * @details Implicit conversion operator to XLWorksheet. Calls the get<>() member function, with XLWorksheet as
 * the template argument.
 */
XLSheet::operator XLWorksheet() const {
	return this->get<XLWorksheet>();
}

/**
 * @details Implicit conversion operator to XLChartsheet. Calls the get<>() member function, with XLChartsheet as
 * the template argument.
 */
XLSheet::operator XLChartsheet() const {
	return this->get<XLChartsheet>();
}

/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLSheet::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }

// ========== BEGIN <conditionalFormatting> related member function definitions

XLCfRule::XLCfRule() : m_cfRuleNode(std::make_unique<XMLNode>()) 
{
}

XLCfRule::XLCfRule(const XMLNode& node) : m_cfRuleNode(std::make_unique<XMLNode>(node)) 
{
}

XLCfRule::XLCfRule(const XLCfRule& other) : m_cfRuleNode(std::make_unique<XMLNode>(*other.m_cfRuleNode))
{
}

XLCfRule::~XLCfRule() = default;

XLCfRule& XLCfRule::operator=(const XLCfRule& other)
{
	if(&other != this)  
		*m_cfRuleNode = *other.m_cfRuleNode;
	return *this;
}

bool XLCfRule::empty() const { return m_cfRuleNode->empty(); }

std::string XLCfRule::formula() const
{
	// XMLNode formulaTextNode =
	return m_cfRuleNode->child("formula").first_child_of_type(pugi::node_pcdata).value();
}
/**
 * @details Unsupported element getter functions
 */
XLUnsupportedElement XLCfRule::colorScale() const { return XLUnsupportedElement(); } // <cfRule><colorScale>...</colorScale></cfRule>
XLUnsupportedElement XLCfRule::dataBar()    const { return XLUnsupportedElement(); } // <cfRule><dataBar>...</dataBar></cfRule>
XLUnsupportedElement XLCfRule::iconSet()    const { return XLUnsupportedElement(); } // <cfRule><iconSet>...</iconSet></cfRule>
XLUnsupportedElement XLCfRule::extLst()     const { return XLUnsupportedElement{}; } // <cfRule><extLst>...</extLst></cfRule>

/**
 * @details Attribute getter functions
 */
XLCfType XLCfRule::type() const { return XLCfTypeFromString(m_cfRuleNode->attribute("type").value()); }
XLStyleIndex XLCfRule::dxfId() const { return m_cfRuleNode->attribute("dxfId").as_uint(XLInvalidStyleIndex); }
uint16 XLCfRule::priority() const { return static_cast<uint16>(m_cfRuleNode->attribute("priority").as_uint(XLPriorityNotSet)); }
bool XLCfRule::stopIfTrue() const { return m_cfRuleNode->attribute("stopIfTrue").as_bool(false); }
// TODO TBD: how does true default value manifest itself for aboveAverage? Evaluate as true if aboveAverage=""?
bool XLCfRule::aboveAverage() const { return m_cfRuleNode->attribute("aboveAverage").as_bool(false); }
bool XLCfRule::percent() const { return m_cfRuleNode->attribute("percent").as_bool(false); }
bool XLCfRule::bottom() const { return m_cfRuleNode->attribute("bottom").as_bool(false); }
XLCfOperator XLCfRule::Operator() const { return XLCfOperatorFromString(m_cfRuleNode->attribute("operator").value()); }
std::string XLCfRule::text() const { return m_cfRuleNode->attribute("text").value(); }
XLCfTimePeriod XLCfRule::timePeriod()   const { return XLCfTimePeriodFromString(m_cfRuleNode->attribute("timePeriod").value()); }
uint16 XLCfRule::rank() const { return static_cast<uint16>(m_cfRuleNode->attribute("rank").as_uint()); }
int16 XLCfRule::stdDev() const { return static_cast<uint16>(m_cfRuleNode->attribute("stdDev").as_int()); }
bool XLCfRule::equalAverage() const { return m_cfRuleNode->attribute("equalAverage").as_bool(false); }

/**
 * @brief Element setter functions
 */
bool XLCfRule::setFormula(std::string const& newFormula)
{
	XMLNode formula = appendAndGetNode(*m_cfRuleNode, "formula", m_nodeOrder);
	if(formula.empty())  return false;
	formula.remove_children(); // no-op if no children
	return formula.append_child(pugi::node_pcdata).set_value(newFormula.c_str());
}

/**
 * @brief Unsupported element setter function
 */
bool XLCfRule::setColorScale(XLUnsupportedElement const& newColorScale) { OpenXLSX::ignore(newColorScale); return false; }
bool XLCfRule::setDataBar(XLUnsupportedElement const& newDataBar) { OpenXLSX::ignore(newDataBar); return false; }
bool XLCfRule::setIconSet(XLUnsupportedElement const& newIconSet) { OpenXLSX::ignore(newIconSet); return false; }
bool XLCfRule::setExtLst(XLUnsupportedElement const& newExtLst) { OpenXLSX::ignore(newExtLst); return false; }

/**
 * @details Attribute setter functions
 */
bool XLCfRule::setType(XLCfType newType)         { return appendAndSetAttribute(*m_cfRuleNode, "type",         XLCfTypeToString(newType)).empty() == false; }
bool XLCfRule::setDxfId(XLStyleIndex newDxfId)        { return appendAndSetAttribute(*m_cfRuleNode, "dxfId",        std::to_string(newDxfId)).empty() == false; }
bool XLCfRule::setPriority(uint16 newPriority)         { return appendAndSetAttribute(*m_cfRuleNode, "priority",     std::to_string(newPriority)).empty() == false; }
// TODO TBD whether true / false work with MS Office booleans here, or if 1 and 0 are mandatory
bool XLCfRule::setStopIfTrue(bool set)                     {
	return appendAndSetAttribute(*m_cfRuleNode, "stopIfTrue",   (set ? "true" : "false")               ).empty() == false;
}
bool XLCfRule::setAboveAverage(bool set)                     {
	return appendAndSetAttribute(*m_cfRuleNode, "aboveAverage", (set ? "true" : "false")               ).empty() == false;
}
bool XLCfRule::setPercent(bool set)                     { return appendAndSetAttribute(*m_cfRuleNode, "percent",      (set ? "true" : "false")               ).empty() == false; }
bool XLCfRule::setBottom(bool set)                     { return appendAndSetAttribute(*m_cfRuleNode, "bottom",       (set ? "true" : "false")               ).empty() == false; }
bool XLCfRule::setOperator(XLCfOperator newOperator)     { return appendAndSetAttribute(*m_cfRuleNode, "operator",     XLCfOperatorToString(newOperator)).empty() == false; }
bool XLCfRule::setText(std::string const& newText)   { return appendAndSetAttribute(*m_cfRuleNode, "text",                                newText.c_str() ).empty() == false; }
bool XLCfRule::setTimePeriod(XLCfTimePeriod newTimePeriod) {
	return appendAndSetAttribute(*m_cfRuleNode, "timePeriod",   XLCfTimePeriodToString(newTimePeriod)  ).empty() == false;
}
bool XLCfRule::setRank(uint16 newRank)             { return appendAndSetAttribute(*m_cfRuleNode, "rank",         std::to_string(newRank)).empty() == false; }
bool XLCfRule::setStdDev(int16 newStdDev)            { return appendAndSetAttribute(*m_cfRuleNode, "stdDev",       std::to_string(newStdDev)).empty() == false; }
bool XLCfRule::setEqualAverage(bool set)                     {
	return appendAndSetAttribute(*m_cfRuleNode, "equalAverage", (set ? "true" : "false")               ).empty() == false;
}

/**
 * @details assemble a string summary about the conditional formatting
 */
std::string XLCfRule::summary() const
{
	using namespace std::literals::string_literals;
	XLStyleIndex dxfIndex = dxfId();
	return "formula is "s + formula()
	       + ", type is "s + XLCfTypeToString(type())
	       + ", dxfId is "s + (dxfIndex == XLInvalidStyleIndex ? "(invalid)"s : std::to_string(dxfId()))
	       + ", priority is "s + std::to_string(priority())
	       + ", stopIfTrue: "s + (stopIfTrue() ? "true"s : "false"s)
	       + ", aboveAverage: "s + (aboveAverage() ? "true"s : "false"s)
	       + ", percent: "s + (percent() ? "true"s : "false"s)
	       + ", bottom: "s + (bottom() ? "true"s : "false"s)
	       + ", operator is "s + XLCfOperatorToString(Operator())
	       + ", text is \""s + text() + "\""s
	       + ", timePeriod is "s + XLCfTimePeriodToString(timePeriod())
	       + ", rank is "s + std::to_string(rank())
	       + ", stdDev is "s + std::to_string(stdDev())
	       + ", equalAverage: "s + (equalAverage() ? "true"s : "false"s)
	;
}

// ========== XLCfRules member functions, parent of XLCfRule
/**
 * @details Constructor. Initializes an empty XLCfRules object
 */
XLCfRules::XLCfRules() : m_conditionalFormattingNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellStyle object.
 */
XLCfRules::XLCfRules(const XMLNode& node) : m_conditionalFormattingNode(std::make_unique<XMLNode>(node)) {}

XLCfRules::XLCfRules(const XLCfRules& other) : m_conditionalFormattingNode(std::make_unique<XMLNode>(*other.m_conditionalFormattingNode))
{
}

XLCfRules::~XLCfRules() = default;

XLCfRules& XLCfRules::operator=(const XLCfRules& other)
{
	if(&other != this)  
		*m_conditionalFormattingNode = *other.m_conditionalFormattingNode;
	return *this;
}

bool XLCfRules::empty() const { return m_conditionalFormattingNode->empty(); }
/**
 * @details Returns the maximum numerical priority value that a cfRule is using (= lowest rule priority)
 */
uint16 XLCfRules::maxPriorityValue() const
{
	XMLNode node = m_conditionalFormattingNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "cfRule")
		node = node.next_sibling_of_type(pugi::node_element);
	uint16 maxPriority = XLPriorityNotSet;
	while(!node.empty() && std::string(node.name()) == "cfRule") {
		maxPriority = std::max(maxPriority, XLCfRule(node).priority());
		node = node.next_sibling_of_type(pugi::node_element);
	}
	return maxPriority;
}

/**
 * @details ensures that newPriority is unique (by incrementing all existing priorities >= newPriority),
 *  then assigns newPriority to the rule with cfRuleIndex
 * @note has a check for no-op if desired priority is already set
 */
bool XLCfRules::setPriority(size_t cfRuleIndex, uint16 newPriority)
{
	XLCfRule affectedRule = cfRuleByIndex(cfRuleIndex);    // throws for cfRuleIndex out of bounds
	if(newPriority == affectedRule.priority())  return true;// no-op if priority is already set

	XMLNode node = m_conditionalFormattingNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "cfRule") // skip past non cfRule elements
		node = node.next_sibling_of_type(pugi::node_element);

	// iterate once over rules to check if newPriority is in use by another rule
	XMLNode node2 = node;
	bool newPriorityFree = true;
	while(newPriorityFree && not node2.empty() && std::string(node2.name()) == "cfRule") {
		if(XLCfRule(node2).priority() == newPriority)  newPriorityFree = false;// check if newPriority is in use
		node2 = node2.next_sibling_of_type(pugi::node_element);
	}

	if(!newPriorityFree) { // need to free up newPriority
		size_t index = 0;
		while(!node.empty() && std::string(node.name()) == "cfRule") { // loop over cfRule elements
			if(index != cfRuleIndex) { // for all rules that are not at cfRuleIndex: increase priority if >= newPriority
				XLCfRule rule(node);
				uint16 prio = rule.priority();
				if(prio >= newPriority)  rule.setPriority(prio + 1);
			}
			node = node.next_sibling_of_type(pugi::node_element);
			++index;
		}
	}
	return affectedRule.setPriority(newPriority);
}

/**
 * @details sort cfRule entries by priority and assign a continuous sequence of values using a given increment
 * @note this mimics the BASIC functionality of line renumbering
 */
void XLCfRules::renumberPriorities(uint16 increment)
{
	if(increment == 0) // not allowed
		throw XLException("XLCfRules::renumberPriorities: increment must not be 0");
	std::multimap< uint16, XLCfRule > rules;
	XMLNode node = m_conditionalFormattingNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "cfRule") // skip past non cfRule elements
		node = node.next_sibling_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) == "cfRule") { // loop over cfRule elements
		XLCfRule rule(node);
		rules.insert(std::pair(rule.priority(), std::move(rule)));
		node = node.next_sibling_of_type(pugi::node_element);
	}
	if(rules.size() * increment > std::numeric_limits< uint16 >::max()) { // first rule always gets assigned "1*increment"
		using namespace std::literals::string_literals;
		throw XLException("XLCfRules::renumberPriorities: amount of rules "s + std::to_string(rules.size())
		          /**/ + " with given increment "s + std::to_string(increment) + " exceeds max range of uint16"s);
	}
	uint16 prio = 0;
	for(auto& [key, rule]: rules) {
		prio += increment;
		rule.setPriority(prio);
	}
	rules.clear();
}
/**
 * @details Returns the amount of cfRule entries held by the class
 */
size_t XLCfRules::count() const
{
	XMLNode node = m_conditionalFormattingNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "cfRule")
		node = node.next_sibling_of_type(pugi::node_element);
	size_t count = 0;
	while(!node.empty() && std::string(node.name()) == "cfRule") {
		++count;
		node = node.next_sibling_of_type(pugi::node_element);
	}
	return count;
}

/**
 * @details fetch XLCfRule from m_conditionalFormattingNode by index
 */
XLCfRule XLCfRules::cfRuleByIndex(size_t index) const
{
	XMLNode node = m_conditionalFormattingNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "cfRule")
		node = node.next_sibling_of_type(pugi::node_element);

	if(not node.empty()) {
		size_t count = 0;
		while(!node.empty() && std::string(node.name()) == "cfRule" && count < index) {
			++count;
			node = node.next_sibling_of_type(pugi::node_element);
		}
		if(count == index && std::string(node.name()) == "cfRule")
			return XLCfRule(node);
	}
	using namespace std::literals::string_literals;
	throw XLException("XLCfRules::"s + __func__ + ": cfRule with index "s + std::to_string(index) + " does not exist");
}

/**
 * @details append a new XLCfRule to m_conditionalFormattingNode, based on copyFrom
 */
size_t XLCfRules::create([[maybe_unused]] XLCfRule copyFrom, std::string cfRulePrefix)
{
	uint16 maxPrio = maxPriorityValue();
	if(maxPrio == std::numeric_limits< uint16 >::max()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCfRules::"s + __func__
			  + ": can not create a new cfRule entry: no available priority value - please renumberPriorities or otherwise free up the highest value"s);
	}

	size_t index = count(); // index for the cfRule to be created
	XMLNode newNode{};    // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	if(index == 0)  newNode = appendAndGetNode(*m_conditionalFormattingNode, "cfRule", m_nodeOrder);
	else {
		XMLNode lastCfRule = *cfRuleByIndex(index - 1).m_cfRuleNode;
		if(not lastCfRule.empty())
			newNode = m_conditionalFormattingNode->insert_child_after("cfRule", lastCfRule);
	}
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCfRules::"s + __func__ + ": failed to create a new cfRule entry");
	}

	// copyXMLNode(newNode, *copyFrom.m_cfRuleNode); // will use copyFrom as template, does nothing if copyFrom is empty
	m_conditionalFormattingNode->insert_child_before(pugi::node_pcdata, newNode).set_value(cfRulePrefix.c_str()); // prefix the new node with

	// regardless of whether a template was provided or not: set the newest rule to the lowest possible priority == highest value in use plus 1
	cfRuleByIndex(index).setPriority(maxPrio + 1);

	return index;
}
/**
 * @details assemble a string summary about the conditional formatting rules
 */
std::string XLCfRules::summary() const
{
	using namespace std::literals::string_literals;
	size_t rulesCount = count();
	if(rulesCount == 0)
		return "(no cfRule entries)";
	std::string result = "";
	for(size_t idx = 0; idx < rulesCount; ++idx) {
		using namespace std::literals::string_literals;
		result += "cfRule["s + std::to_string(idx) + "] "s + cfRuleByIndex(idx).summary();
		if(idx + 1 < rulesCount)  result += ", ";
	}
	return result;
}

// ========== XLConditionalFormat Member Functions
/**
 * @details Constructor. Initializes an empty XLConditionalFormat object
 */
XLConditionalFormat::XLConditionalFormat() : m_conditionalFormattingNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellStyle object.
 */
XLConditionalFormat::XLConditionalFormat(const XMLNode& node) : m_conditionalFormattingNode(std::make_unique<XMLNode>(node)) {}

XLConditionalFormat::XLConditionalFormat(const XLConditionalFormat& other) : m_conditionalFormattingNode(std::make_unique<XMLNode>(*other.m_conditionalFormattingNode))
{
}

XLConditionalFormat::~XLConditionalFormat() = default;

XLConditionalFormat& XLConditionalFormat::operator=(const XLConditionalFormat& other)
{
	if(&other != this)  *m_conditionalFormattingNode = *other.m_conditionalFormattingNode;
	return *this;
}

bool XLConditionalFormat::empty() const { return m_conditionalFormattingNode->empty(); }
std::string XLConditionalFormat::sqref() const { return m_conditionalFormattingNode->attribute("sqref").value(); }
XLCfRules XLConditionalFormat::cfRules() const { return XLCfRules(*m_conditionalFormattingNode);                 }
bool XLConditionalFormat::setSqref(std::string newSqref) { return appendAndSetAttribute(*m_conditionalFormattingNode, "sqref", newSqref).empty() == false; }
bool XLConditionalFormat::setExtLst(XLUnsupportedElement const& newExtLst) { OpenXLSX::ignore(newExtLst); return false; }
/**
 * @details assemble a string summary about the conditional formatting
 */
std::string XLConditionalFormat::summary() const
{
	using namespace std::literals::string_literals;
	return "sqref is "s + sqref() + ", cfRules: "s + cfRules().summary();
}
//
// XLConditionalFormats member functions, parent of XLConditionalFormat
//
XLConditionalFormats::XLConditionalFormats() : m_sheetNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLConditionalFormats object.
 */
XLConditionalFormats::XLConditionalFormats(const XMLNode& sheet) : m_sheetNode(std::make_unique<XMLNode>(sheet))
{
}

XLConditionalFormats::~XLConditionalFormats() 
{
}

XLConditionalFormats::XLConditionalFormats(const XLConditionalFormats& other) : m_sheetNode(std::make_unique<XMLNode>(*other.m_sheetNode))
{
}

XLConditionalFormats::XLConditionalFormats(XLConditionalFormats&& other) : m_sheetNode(std::move(other.m_sheetNode))
{
}

XLConditionalFormats& XLConditionalFormats::operator=(const XLConditionalFormats& other)
{
	if(&other != this) {
		*m_sheetNode = *other.m_sheetNode;
	}
	return *this;
}

bool XLConditionalFormats::empty() const { return m_sheetNode->empty(); }
/**
 * @details Returns the amount of conditionalFormatting entries held by the class
 */
size_t XLConditionalFormats::count() const
{
	XMLNode node = m_sheetNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "conditionalFormatting")
		node = node.next_sibling_of_type(pugi::node_element);
	size_t count = 0;
	while(!node.empty() && std::string(node.name()) == "conditionalFormatting") {
		++count;
		node = node.next_sibling_of_type(pugi::node_element);
	}
	return count;
}

/**
 * @details fetch XLConditionalFormat from m_sheetNode by index
 */
XLConditionalFormat XLConditionalFormats::conditionalFormatByIndex(size_t index) const
{
	XMLNode node = m_sheetNode->first_child_of_type(pugi::node_element);
	while(!node.empty() && std::string(node.name()) != "conditionalFormatting")
		node = node.next_sibling_of_type(pugi::node_element);

	if(not node.empty()) {
		size_t count = 0;
		while(!node.empty() && std::string(node.name()) == "conditionalFormatting" && count < index) {
			++count;
			node = node.next_sibling_of_type(pugi::node_element);
		}
		if(count == index && std::string(node.name()) == "conditionalFormatting")
			return XLConditionalFormat(node);
	}
	using namespace std::literals::string_literals;
	throw XLException("XLConditionalFormats::"s + __func__ + ": conditional format with index "s + std::to_string(index) + " does not exist");
}

/**
 * @details append a new XLConditionalFormat to m_sheetNode, based on copyFrom
 */
size_t XLConditionalFormats::create([[maybe_unused]] XLConditionalFormat copyFrom, std::string conditionalFormattingPrefix)
{
	size_t index = count(); // index for the conditional formatting to be created
	XMLNode newNode{};    // scope declaration
	// ===== Append new node prior to final whitespaces, if any
	if(index == 0)
		newNode = appendAndGetNode(*m_sheetNode, "conditionalFormatting", m_nodeOrder);
	else {
		XMLNode lastConditionalFormat = *conditionalFormatByIndex(index - 1).m_conditionalFormattingNode;
		if(not lastConditionalFormat.empty())
			newNode = m_sheetNode->insert_child_after("conditionalFormatting", lastConditionalFormat);
	}
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLConditionalFormats::"s + __func__ + ": failed to create a new conditional formatting entry");
	}

	// copyXMLNode(newNode, *copyFrom.m_conditionalFormattingNode); // will use copyFrom as template, does nothing if copyFrom is empty
	m_sheetNode->insert_child_before(pugi::node_pcdata, newNode).set_value(conditionalFormattingPrefix.c_str()); // prefix the new node with conditionalFormattingPrefix

	return index;
}

/**
 * @details assemble a string summary about the conditional formattings
 */
std::string XLConditionalFormats::summary() const
{
	using namespace std::literals::string_literals;
	size_t conditionalFormatsCount = count();
	if(conditionalFormatsCount == 0)
		return "(no conditionalFormatting entries)";
	std::string result = "";
	for(size_t idx = 0; idx < conditionalFormatsCount; ++idx) {
		using namespace std::literals::string_literals;
		result += "conditionalFormatting["s + std::to_string(idx) + "] "s + conditionalFormatByIndex(idx).summary();
		if((idx + 1) < conditionalFormatsCount)  
			result += ", ";
	}
	return result;
}

// ========== END <conditionalFormatting> related member function definitions

// ========== XLWorksheet Member Functions

/**
 * @details The constructor does some slight reconfiguration of the XML file, in order to make parsing easier.
 * For example, columns with identical formatting are by default grouped under the same node. However, this makes it more difficult to
 * parse, so the constructor reconfigures it so each column has it's own formatting.
 */
XLWorksheet::XLWorksheet(XLXmlData* xmlData) : XLSheetBase(xmlData)
{
	// ===== Read the dimensions of the Sheet and set data members accordingly.
	if(const std::string dimensions = xmlDocument().document_element().child("dimension").attribute("ref").value();
	    dimensions.find(':') == std::string::npos)
		xmlDocument().document_element().child("dimension").set_value("A1");
	else
		xmlDocument().document_element().child("dimension").set_value(dimensions.substr(dimensions.find(':') + 1).c_str());

	// If Column properties are grouped, divide them into properties for individual Columns.
	if(xmlDocument().document_element().child("cols").type() != pugi::node_null) {
		auto currentNode = xmlDocument().document_element().child("cols").first_child_of_type(pugi::node_element);
		while(!currentNode.empty()) {
			uint16 min {};
			uint16 max {};
			try {
				min = static_cast<uint16>(std::stoi(currentNode.attribute("min").value()));
				max = static_cast<uint16>(std::stoi(currentNode.attribute("max").value()));
			}
			catch(...) {
				throw XLInternalError("Worksheet column min and/or max attributes are invalid.");
			}
			if(min != max) {
				currentNode.attribute("min").set_value(max);
				for(uint16 i = min; i < max; i++) {
					auto newnode = xmlDocument().document_element().child("cols").insert_child_before("col", currentNode);
					auto attr = currentNode.first_attribute();
					while(!attr.empty()) {
						newnode.append_attribute(attr.name()) = attr.value();
						attr = attr.next_attribute();
					}
					newnode.attribute("min") = i;
					newnode.attribute("max") = i;
				}
			}
			currentNode = currentNode.next_sibling_of_type(pugi::node_element);
		}
	}
}

/**
 * @details copy-construct an XLWorksheet object from other
 */
XLWorksheet::XLWorksheet(const XLWorksheet& other) : XLSheetBase<XLWorksheet>(other)
{
	m_relationships = other.m_relationships; // invoke XLRelationships copy assignment operator
	m_merges        = other.m_merges;     //  "     XLMergeCells       "
	m_vmlDrawing    = other.m_vmlDrawing; //  "     XLVmlDrawing
	m_comments      = other.m_comments;   //  "     XLComments         "
	m_tables        = other.m_tables;     //  "     XLTables           "
}

/**
 * @details move-construct an XLWorksheet object from other
 */
XLWorksheet::XLWorksheet(XLWorksheet&& other) : XLSheetBase< XLWorksheet >(other)
{
	m_relationships = std::move(other.m_relationships); // invoke XLRelationships move assignment operator
	m_merges        = std::move(other.m_merges);     //  "     XLMergeCells       "
	m_vmlDrawing    = std::move(other.m_vmlDrawing); //  "     XLVmlDrawing
	m_comments      = std::move(other.m_comments);   //  "     XLComments         "
	m_tables        = std::move(other.m_tables);     //  "     XLTables           "
}

/**
 * @details copy-assign an XLWorksheet from other
 */
XLWorksheet& XLWorksheet::operator=(const XLWorksheet& other)
{
	XLSheetBase<XLWorksheet>::operator=(other); // invoke base class copy assignment operator
	m_relationships = other.m_relationships;
	m_merges        = other.m_merges;
	m_vmlDrawing    = other.m_vmlDrawing;
	m_comments      = other.m_comments;
	m_tables        = other.m_tables;
	return *this;
}

/**
 * @details move-assign an XLWorksheet from other
 */
XLWorksheet& XLWorksheet::operator=(XLWorksheet&& other)
{
	XLSheetBase<XLWorksheet>::operator=(other); // invoke base class move assignment operator
	m_relationships = std::move(other.m_relationships);
	m_merges        = std::move(other.m_merges);
	m_vmlDrawing    = std::move(other.m_vmlDrawing);
	m_comments      = std::move(other.m_comments);
	m_tables        = std::move(other.m_tables);
	return *this;
}

XLColor XLWorksheet::getColor_impl() const { return XLColor(xmlDocumentC().document_element().child("sheetPr").child("tabColor").attribute("rgb").value()); }
void XLWorksheet::setColor_impl(const XLColor& color) { setTabColor(xmlDocument(), color); }
bool XLWorksheet::isSelected_impl() const { return tabIsSelected(xmlDocumentC()); }
void XLWorksheet::setSelected_impl(bool selected) { setTabSelected(xmlDocument(), selected); }
bool XLWorksheet::isActive_impl() const { return parentDoc().execQuery(XLQuery(XLQueryType::QuerySheetIsActive).setParam("sheetID", relationshipID())).result<bool>(); }
bool XLWorksheet::setActive_impl() { return parentDoc().execCommand(XLCommand(XLCommandType::SetSheetActive).setParam("sheetID", relationshipID())); }
XLCellAssignable XLWorksheet::cell(const std::string & ref) const { return cell(XLCellReference(ref)); }
XLCellAssignable XLWorksheet::cell(const XLCellReference & ref) const { return cell(ref.row(), ref.column()); }
/**
 * @details This function returns a pointer to an XLCell object in the worksheet. This particular overload
 * also serves as the main function, called by the other overloads.
 */
XLCellAssignable XLWorksheet::cell(uint32 rowNumber, uint16 columnNumber) const
{
	const XMLNode rowNode  = getRowNode(xmlDocumentC().document_element().child("sheetData"), rowNumber);
	const XMLNode cellNode = getCellNode(rowNode, columnNumber, rowNumber);
	// ===== Move-construct XLCellAssignable from temporary XLCell
	return XLCellAssignable(XLCell(cellNode, parentDoc().sharedStrings()));
}

XLCellAssignable XLWorksheet::findCell(const std::string& ref) const { return findCell(XLCellReference(ref)); }
XLCellAssignable XLWorksheet::findCell(const XLCellReference& ref) const { return findCell(ref.row(), ref.column()); }
/**
 * @details This function attempts to find a cell, but creates neither the row nor the cell XML if missing - and returns an empty XLCellAssignable instead
 */
XLCellAssignable XLWorksheet::findCell(uint32 rowNumber, uint16 columnNumber) const
{
	return XLCellAssignable(XLCell(findCellNode(findRowNode(xmlDocumentC().document_element().child("sheetData"), rowNumber), columnNumber), parentDoc().sharedStrings()));
}

XLCellRange XLWorksheet::range() const { return range(XLCellReference("A1"), lastCell()); }

XLCellRange XLWorksheet::range(const XLCellReference& topLeft, const XLCellReference& bottomRight) const
{
	return XLCellRange(xmlDocumentC().document_element().child("sheetData"), topLeft, bottomRight, parentDoc().sharedStrings());
}
/**
 * @details Get a range based on two cell reference strings
 */
XLCellRange XLWorksheet::range(std::string const& topLeft, std::string const& bottomRight) const
{
	return range(XLCellReference(topLeft), XLCellReference(bottomRight));
}
/**
 * @details Get a range based on a reference string
 */
XLCellRange XLWorksheet::range(std::string const& rangeReference) const
{
	size_t pos = rangeReference.find_first_of(':');
	return range(rangeReference.substr(0, pos), rangeReference.substr(pos + 1, std::string::npos));
}

XLRowRange XLWorksheet::rows() const    // 2024-04-29: patched for whitespace
{
	const auto sheetDataNode = xmlDocumentC().document_element().child("sheetData");
	return XLRowRange(sheetDataNode, 1, (sheetDataNode.last_child_of_type(pugi::node_element).empty() ? 1 : 
		static_cast<uint32>(sheetDataNode.last_child_of_type(pugi::node_element).attribute("r").as_ullong())), parentDoc().sharedStrings());
}

XLRowRange XLWorksheet::rows(uint32 rowCount) const
{
	return XLRowRange(xmlDocumentC().document_element().child("sheetData"), 1, rowCount, parentDoc().sharedStrings());
}

XLRowRange XLWorksheet::rows(uint32 firstRow, uint32 lastRow) const
{
	return XLRowRange(xmlDocumentC().document_element().child("sheetData"), firstRow, lastRow, parentDoc().sharedStrings());
}

/**
 * @details Get the XLRow object corresponding to the given row number. In the XML file, all cell data are stored under
 * the corresponding row, and all rows have to be ordered in ascending order. If a row have no data, there may not be a
 * node for that row.
 */
XLRow XLWorksheet::row(uint32 rowNumber) const
{
	return XLRow { getRowNode(xmlDocumentC().document_element().child("sheetData"), rowNumber), parentDoc().sharedStrings() };
}

/**
 * @details Get the XLColumn object corresponding to the given column number. In the underlying XML data structure,
 * column nodes do not hold any cell data. Columns are used solely to hold data regarding column formatting.
 * @todo Consider simplifying this function. Can any standard algorithms be used?
 */
XLColumn XLWorksheet::column(uint16 columnNumber) const
{
	using namespace std::literals::string_literals;

	if(columnNumber < 1 || columnNumber > OpenXLSX::MAX_COLS) // 2024-08-05: added range check
		throw XLException("XLWorksheet::column: columnNumber "s + std::to_string(columnNumber) + " is outside allowed range [1;"s + std::to_string(MAX_COLS) + "]"s);

	// If no columns exists, create the <cols> node in the XML document.
	if(xmlDocumentC().document_element().child("cols").empty())
		xmlDocumentC().document_element().insert_child_before("cols", xmlDocumentC().document_element().child("sheetData"));

	// ===== Find the column node, if it exists
	auto columnNode = xmlDocumentC().document_element().child("cols").find_child([&](const XMLNode node) {
		return (columnNumber >= node.attribute("min").as_int() && columnNumber <= node.attribute("max").as_int()) ||
		       node.attribute("min").as_int() > columnNumber;
	});

	uint16 minColumn {};
	uint16 maxColumn {};
	if(not columnNode.empty()) {
		minColumn = static_cast<uint16>(columnNode.attribute("min").as_int()); // only look it up once for multiple access
		maxColumn = static_cast<uint16>(columnNode.attribute("max").as_int()); //   "
	}
	// ===== If the node exists for the column, and only spans that column, then continue...
	if(not columnNode.empty() && (minColumn == columnNumber) && (maxColumn == columnNumber)) {
	}
	// ===== If the node exists for the column, but spans several columns, split it into individual nodes, and set columnNode to the right
	// one...
	// BUGFIX 2024-04-27 - old if condition would split a multi-column setting even if columnNumber is < minColumn (see lambda return value
	// above)
	// NOTE 2024-04-27: the column splitting for loaded files is already handled in the constructor, technically this code is not necessary
	// here
	else if(not columnNode.empty() && (columnNumber >= minColumn) && (minColumn != maxColumn)) {
		// ===== Split the node in individual columns...
		columnNode.attribute("min").set_value(maxColumn); // Limit the original node to a single column
		for(int i = minColumn; i < maxColumn; ++i) {
			auto node = xmlDocumentC().document_element().child("cols").insert_copy_before(columnNode, columnNode);
			node.attribute("min").set_value(i);
			node.attribute("max").set_value(i);
		}
		// BUGFIX 2024-04-27: the original node should not be deleted, but - in line with XLWorksheet constructor - is now limited above to
		// min = max attribute
		// // ===== Delete the original node
		// columnNode = columnNode.previous_sibling_of_type(pugi::node_element); // due to insert loop, previous node should be guaranteed
		// // to be an element node
		// xmlDocument().document_element().child("cols").remove_child(columnNode.next_sibling());

		// ===== Find the node corresponding to the column number - BUGFIX 2024-04-27: loop should abort on empty node
		while(!columnNode.empty() && columnNode.attribute("min").as_int() != columnNumber)
			columnNode = columnNode.previous_sibling_of_type(pugi::node_element);
		if(columnNode.empty())
			throw XLInternalError("XLWorksheet::"s + __func__ + ": column node for index "s + std::to_string(columnNumber) +
				  "not found after splitting column nodes"s);
	}

	// ===== If a node for the column does NOT exist, but a node for a higher column exists...
	else if(not columnNode.empty() && minColumn > columnNumber) {
		columnNode                                 = xmlDocumentC().document_element().child("cols").insert_child_before("col", columnNode);
		columnNode.append_attribute("min")         = columnNumber;
		columnNode.append_attribute("max")         = columnNumber;
		columnNode.append_attribute("width")       = 9.8;
		columnNode.append_attribute("customWidth") = 0;
	}
	// ===== Otherwise, the end of the list is reached, and a new node is appended
	else if(columnNode.empty()) {
		columnNode                                 = xmlDocumentC().document_element().child("cols").append_child("col");
		columnNode.append_attribute("min")         = columnNumber;
		columnNode.append_attribute("max")         = columnNumber;
		columnNode.append_attribute("width")       = 9.8;
		columnNode.append_attribute("customWidth") = 0;
	}

	if(columnNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLInternalError("XLWorksheet::"s + __func__ + ": was unable to find or create node for column "s +
			  std::to_string(columnNumber));
	}
	return XLColumn(columnNode);
}

/**
 * @details Get the column with the given column reference by converting the columnRef into a column number
 * and forwarding the implementation to XLColumn XLWorksheet::column(uint16 columnNumber) const
 */
XLColumn XLWorksheet::column(std::string const& columnRef) const { return column(XLCellReference::columnAsNumber(columnRef)); }

/**
 * @details Returns an XLCellReference to the last cell using rowCount() and columnCount() methods.
 */
XLCellReference XLWorksheet::lastCell() const noexcept { return { rowCount(), columnCount() }; }

/**
 * @details Iterates through the rows and finds the maximum number of cells.
 */
uint16 XLWorksheet::columnCount() const noexcept
{
	uint16 maxCount = 0; // Pull request: Update XLSheet.cpp with correct type #176, Explicitely cast to unsigned short int #163
	XLRowRange rowsRange = rows();
	for(XLRowIterator rowIt = rowsRange.begin(); rowIt != rowsRange.end(); ++rowIt ) {
		if(rowIt.rowExists()) {
			uint16 cellCount = rowIt->cellCount();
			maxCount = std::max(cellCount, maxCount);
		}
	}
	return maxCount;
}

/**
 * @details Finds the last row (node) and returns the row number.
 */
uint32 XLWorksheet::rowCount() const noexcept
{
	return static_cast<uint32>(
		xmlDocumentC().document_element().child("sheetData").last_child_of_type(pugi::node_element).attribute("r").as_ullong());
}

/**
 * @details finds a given row and deletes it
 */
bool XLWorksheet::deleteRow(uint32 rowNumber)
{
	XMLNode row = xmlDocument().document_element().child("sheetData").first_child_of_type(pugi::node_element);
	XMLNode lastRow = xmlDocument().document_element().child("sheetData").last_child_of_type(pugi::node_element);
	// ===== Fail if rowNumber is not in XML
	if(rowNumber < row.attribute("r").as_ullong() || rowNumber > lastRow.attribute("r").as_ullong())  
		return false;
	// ===== If rowNumber is closer to first (existing) row than to last row, search forwards
	if(rowNumber - row.attribute("r").as_ullong() < lastRow.attribute("r").as_ullong() - rowNumber)
		while(!row.empty() && (row.attribute("r").as_ullong() < rowNumber))  
			row = row.next_sibling_of_type(pugi::node_element);
	// ===== Otherwise, search backwards
	else {
		row = lastRow;
		while(!row.empty() && (row.attribute("r").as_ullong() > rowNumber))  
			row = row.previous_sibling_of_type(pugi::node_element);
	}
	if(row.attribute("r").as_ullong() != rowNumber)  
		return false;// row not found in XML
	// ===== If row was located: remove it
	return xmlDocument().document_element().child("sheetData").remove_child(row);
}

void XLWorksheet::updateSheetName(const std::string& oldName, const std::string& newName)    // 2024-04-29: patched for whitespace
{
	// ===== Set up temporary variables
	std::string oldNameTemp = oldName;
	std::string newNameTemp = newName;
	std::string formula;

	// ===== If the sheet name contains spaces, it should be enclosed in single quotes (')
	if(oldName.find(' ') != std::string::npos)  oldNameTemp = "\'" + oldName + "\'";
	if(newName.find(' ') != std::string::npos)  newNameTemp = "\'" + newName + "\'";

	// ===== Ensure only sheet names are replaced (references to sheets always ends with a '!')
	oldNameTemp += '!';
	newNameTemp += '!';

	// ===== Iterate through all defined names
	XMLNode row = xmlDocument().document_element().child("sheetData").first_child_of_type(pugi::node_element);
	for(; not row.empty(); row = row.next_sibling_of_type(pugi::node_element)) {
		for(XMLNode cell = row.first_child_of_type(pugi::node_element);
		    not cell.empty();
		    cell         = cell.next_sibling_of_type(pugi::node_element)) {
			if(!XLCell(cell, parentDoc().sharedStrings()).hasFormula())  continue;

			formula = XLCell(cell, parentDoc().sharedStrings()).formula().get();

			// ===== Skip if formula contains a '[' and ']' (means that the defined refers to external workbook)
			if(formula.find('[') == std::string::npos && formula.find(']') == std::string::npos) {
				// ===== For all instances of the old sheet name in the formula, replace with the new name.
				while(formula.find(oldNameTemp) != std::string::npos) {
					formula.replace(formula.find(oldNameTemp), oldNameTemp.length(), newNameTemp);
				}
				XLCell(cell, parentDoc().sharedStrings()).formula() = formula;
			}
		}
	}
}

/**
 * @details upon first access, ensure that the worksheet's <mergeCells> tag exists, and create an XLMergeCells object
 */
XLMergeCells & XLWorksheet::merges()
{
	if(!m_merges.valid())
		m_merges = XLMergeCells(xmlDocument().document_element(), m_nodeOrder);
	return m_merges;
}

/**
 * @details create a new mergeCell element in the worksheet mergeCells array, with the only
 *          attribute being ref="A1:D6" (with the top left and bottom right cells of the range)
 *          The mergeCell element otherwise remains empty (no value)
 *          The mergeCells attribute count will be increased by one
 *          If emptyHiddenCells is true (XLEmptyHiddenCells), the values of all but the top left cell of the range
 *          will be deleted (not from shared strings)
 *          If any cell within the range is the sheet's active cell, the active cell will be set to the
 *          top left corner of rangeToMerge
 */
void XLWorksheet::mergeCells(XLCellRange const& rangeToMerge, bool emptyHiddenCells)
{
	if(rangeToMerge.numRows() * rangeToMerge.numColumns() < 2) {
		using namespace std::literals::string_literals;
		throw XLInputError("XLWorksheet::"s + __func__ + ": rangeToMerge must comprise at least 2 cells"s);
	}

	merges().appendMerge(rangeToMerge.address());
	if(emptyHiddenCells) {
		// ===== Iterate over rangeToMerge, delete values & attributes (except r and s) for all but the first cell in the range
		XLCellIterator it = rangeToMerge.begin();
		++it; // leave first cell untouched
		while(it != rangeToMerge.end()) {
			// ===== When merging with emptyHiddenCells in LO, subsequent unmerging restores the cell styles -> so keep them here as well
			it->clear(XLKeepCellStyle); // clear cell contents except style
			++it;
		}
	}
}

/**
 * @details Convenience wrapper for the previous function, using a std::string range reference
 */
void XLWorksheet::mergeCells(const std::string& rangeReference, bool emptyHiddenCells) {
	mergeCells(range(rangeReference), emptyHiddenCells);
}

/**
 * @details check if rangeToUnmerge exists in mergeCells array & remove it
 */
void XLWorksheet::unmergeCells(XLCellRange const& rangeToUnmerge)
{
	int32_t mergeIndex = merges().findMerge(rangeToUnmerge.address());
	if(mergeIndex != -1)
		merges().deleteMerge(mergeIndex);
	else {
		using namespace std::literals::string_literals;
		throw XLInputError("XLWorksheet::"s + __func__ + ": merged range "s + rangeToUnmerge.address() + " does not exist"s);
	}
}

/**
 * @details Convenience wrapper for the previous function, using a std::string range reference
 */
void XLWorksheet::unmergeCells(const std::string& rangeReference)
{
	unmergeCells(range(rangeReference));
}

/**
 * @details Retrieve the column's format
 */
XLStyleIndex XLWorksheet::getColumnFormat(uint16 columnNumber) const { return column(columnNumber).format(); }
XLStyleIndex XLWorksheet::getColumnFormat(const std::string& columnNumber) const { return getColumnFormat(XLCellReference::columnAsNumber(columnNumber)); }

/**
 * @details Set the style for the identified column,
 *          then iterate over all rows to set the same style for existing cells in that column
 */
bool XLWorksheet::setColumnFormat(uint16 columnNumber, XLStyleIndex cellFormatIndex)
{
	if(!column(columnNumber).setFormat(cellFormatIndex)) // attempt to set column format
		return false; // early fail

	XLRowRange allRows = rows();
	for(XLRowIterator rowIt = allRows.begin(); rowIt != allRows.end(); ++rowIt) {
		XLCell curCell = rowIt->findCell(columnNumber);
		if(curCell && !curCell.setCellFormat(cellFormatIndex)) // attempt to set cell format for a non empty cell
			return false; // failure to set cell format
	}
	return true; // if loop finished nominally: success
}

bool XLWorksheet::setColumnFormat(const std::string& columnNumber, XLStyleIndex cellFormatIndex) {
	return setColumnFormat(XLCellReference::columnAsNumber(columnNumber), cellFormatIndex);
}

/**
 * @details Retrieve the row's format
 */
XLStyleIndex XLWorksheet::getRowFormat(uint16 rowNumber) const { return row(rowNumber).format(); }

/**
 * @details Set the style for the identified row,
 *          then iterate over all existing cells in the row to set the same style
 */
bool XLWorksheet::setRowFormat(uint32 rowNumber, XLStyleIndex cellFormatIndex)
{
	if(!row(rowNumber).setFormat(cellFormatIndex)) // attempt to set row format
		return false; // early fail

	XLRowDataRange rowCells = row(rowNumber).cells();
	for(XLRowDataIterator cellIt = rowCells.begin(); cellIt != rowCells.end(); ++cellIt)
		if(!cellIt->setCellFormat(cellFormatIndex)) // attempt to set cell format
			return false;
	return true; // if loop finished nominally: success
}

/**
 * @details Provide access to worksheet conditional formats
 */
XLConditionalFormats XLWorksheet::conditionalFormats() const { return XLConditionalFormats(xmlDocumentC().document_element()); }

/**
 * @brief Set the <sheetProtection> attributes sheet, objects and scenarios respectively
 */
bool XLWorksheet::protectSheet(bool set) 
{
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "sheet", (set ? "true" : "false"), /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::protectObjects(bool set) 
{
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "objects",   (set ? "true" : "false"), /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::protectScenarios(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "scenarios", (set ? "true" : "false"),
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

/**
 * @brief Set the <sheetProtection> attributes insertColumns, insertRows, deleteColumns, deleteRows, selectLockedCells, selectUnlockedCells
 */
bool XLWorksheet::allowInsertColumns(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "insertColumns",       (!set ? "true" : "false"),// invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::allowInsertRows(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "insertRows",          (!set ? "true" : "false"),// invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::allowDeleteColumns(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "deleteColumns",       (!set ? "true" : "false"),// invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::allowDeleteRows(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "deleteRows",          (!set ? "true" : "false"),// invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::allowSelectLockedCells(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "selectLockedCells",   (!set ? "true" : "false"),// invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

bool XLWorksheet::allowSelectUnlockedCells(bool set) {
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "selectUnlockedCells", (!set ? "true" : "false"), // invert allow set(ting) for the protect setting
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

/**
 * @details store the password hash to the underlying XML node
 */
bool XLWorksheet::setPasswordHash(std::string hash)
{
	XMLNode sheetNode = xmlDocument().document_element();
	return appendAndSetNodeAttribute(sheetNode, "sheetProtection", "password", hash,
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;
}

/**
 * @details calculate the password hash and store it to the underlying XML node
 */
bool XLWorksheet::setPassword(std::string password) {
	return setPasswordHash(password.length() ? ExcelPasswordHashAsString(password) : std::string(""));
}

bool XLWorksheet::clearPassword() { return setPasswordHash(""); }
bool XLWorksheet::clearSheetProtection() { return xmlDocument().document_element().remove_child("sheetProtection"); }
bool XLWorksheet::sheetProtected()     const { return xmlDocumentC().document_element().child("sheetProtection").attribute("sheet").as_bool(false);     }
bool XLWorksheet::objectsProtected()   const { return xmlDocumentC().document_element().child("sheetProtection").attribute("objects").as_bool(false);   }
bool XLWorksheet::scenariosProtected() const { return xmlDocumentC().document_element().child("sheetProtection").attribute("scenarios").as_bool(false); }
/**
 * @details the following settings are inverted to get from "protected" meaning in OOXML to "allowed" meaning in the OpenXLSX API
 * @details insertColumns, insertRows, deleteColumns, deleteRows are protected by default
 * @details selectLockedCells, selectUnlockedCells are allowed by default
 */
bool XLWorksheet::insertColumnsAllowed()       const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("insertColumns").as_bool(true);        }
bool XLWorksheet::insertRowsAllowed()          const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("insertRows").as_bool(true);           }
bool XLWorksheet::deleteColumnsAllowed()       const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("deleteColumns").as_bool(true);        }
bool XLWorksheet::deleteRowsAllowed()          const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("deleteRows").as_bool(true);           }
bool XLWorksheet::selectLockedCellsAllowed()   const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("selectLockedCells").as_bool(false);   }
bool XLWorksheet::selectUnlockedCellsAllowed() const { return not xmlDocumentC().document_element().child("sheetProtection").attribute("selectUnlockedCells").as_bool(false); }
std::string XLWorksheet::passwordHash()        const { return xmlDocumentC().document_element().child("sheetProtection").attribute("password").value();                   }
bool XLWorksheet::passwordIsSet()              const { return passwordHash().length() > 0; }

std::string XLWorksheet::sheetProtectionSummary() const
{
	using namespace std::literals::string_literals;
	return "sheet is"s               + ( sheetProtected()             ? ""s : " not"s ) + " protected"s
	       + ", objects are"s            + ( objectsProtected()           ? ""s : " not"s ) + " protected"s
	       + ", scenarios are"s          + ( scenariosProtected()         ? ""s : " not"s ) + " protected"s
	       + ", insertColumns is"s       + ( insertColumnsAllowed()       ? ""s : " not"s ) + " allowed"s
	       + ", insertRows is"s          + ( insertRowsAllowed()          ? ""s : " not"s ) + " allowed"s
	       + ", deleteColumns is"s       + ( deleteColumnsAllowed()       ? ""s : " not"s ) + " allowed"s
	       + ", deleteRows is"s          + ( deleteRowsAllowed()          ? ""s : " not"s ) + " allowed"s
	       + ", selectLockedCells is"s   + ( selectLockedCellsAllowed()   ? ""s : " not"s ) + " allowed"s
	       + ", selectUnlockedCells is"s + ( selectUnlockedCellsAllowed() ? ""s : " not"s ) + " allowed"s
	       + ", password is"s + ( passwordIsSet() ? ""s : " not"s ) + " set"s
	       + ( passwordIsSet() ? ( ", passwordHash is "s + passwordHash() ) : ""s )
	;
}

/**
 * @details functions to test whether a VMLDrawing / Comments / Tables entry exists for this worksheet (without creating those entries)
 */
bool XLWorksheet::hasRelationships() const { return parentDoc().hasSheetRelationships(sheetXmlNumber()); }
bool XLWorksheet::hasVmlDrawing()    const { return parentDoc().hasSheetVmlDrawing(sheetXmlNumber()); }
bool XLWorksheet::hasComments()      const { return parentDoc().hasSheetComments(sheetXmlNumber()); }
bool XLWorksheet::hasTables()        const { return parentDoc().hasSheetTables(sheetXmlNumber()); }

/**
 * @details fetches XLVmlDrawing for the sheet - creates & assigns the class if empty
 */
XLVmlDrawing& XLWorksheet::vmlDrawing()
{
	if(!m_vmlDrawing.valid()) {
		// ===== Append xdr namespace attribute to worksheet if not present
		XMLNode docElement = xmlDocument().document_element();
		XMLAttribute xdrNamespace = appendAndGetAttribute(docElement, "xmlns:xdr", "");
		xdrNamespace = "http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing";

		std::ignore = relationships(); // create sheet relationships if not existing

		// ===== Trigger parentDoc to create drawing XML file and return it
		uint16 sheetXmlNo = sheetXmlNumber();
		m_vmlDrawing = parentDoc().sheetVmlDrawing(sheetXmlNo); // fetch drawing for this worksheet
		if(!m_vmlDrawing.valid())
			throw XLException("XLWorksheet::vmlDrawing(): could not create drawing XML");
		std::string drawingRelativePath = getPathARelativeToPathB(m_vmlDrawing.getXmlPath(), getXmlPath() );
		XLRelationshipItem vmlDrawingRelationship;
		if(!m_relationships.targetExists(drawingRelativePath))
			vmlDrawingRelationship = m_relationships.addRelationship(XLRelationshipType::VMLDrawing, drawingRelativePath);
		else
			vmlDrawingRelationship = m_relationships.relationshipByTarget(drawingRelativePath);
		if(vmlDrawingRelationship.empty())
			throw XLException("XLWorksheet::vmlDrawing(): could not add determine sheet relationship for VML Drawing");
		XMLNode legacyDrawing = appendAndGetNode(docElement, "legacyDrawing", m_nodeOrder);
		if(legacyDrawing.empty())
			throw XLException("XLWorksheet::vmlDrawing(): could not add <legacyDrawing> element to worksheet XML");
		appendAndSetAttribute(legacyDrawing, "r:id", vmlDrawingRelationship.id());
	}

	return m_vmlDrawing;
}

/**
 * @details fetches XLComments for the sheet - creates & assigns the class if empty
 */
XLComments& XLWorksheet::comments()
{
	if(!m_comments.valid()) {
		std::ignore = relationships(); // create sheet relationships if not existing
		// ===== Unfortunately, xl/vmlDrawing#.vml is needed to support comments: append xdr namespace attribute to worksheet if not present
		std::ignore = vmlDrawing(); // create sheet VML drawing if not existing

		// ===== Trigger parentDoc to create comment XML file and return it
		uint16 sheetXmlNo = sheetXmlNumber();
		// std::cout << "worksheet comments for sheetId " << sheetXmlNo << std::endl;
		m_comments = parentDoc().sheetComments(sheetXmlNo); // fetch comments for this worksheet
		if(!m_comments.valid())
			throw XLException("XLWorksheet::comments(): could not create comments XML");
		m_comments.setVmlDrawing(m_vmlDrawing); // link XLVmlDrawing object to the comments so it can be modified from there
		std::string commentsRelativePath = getPathARelativeToPathB(m_comments.getXmlPath(), getXmlPath() );
		if(!m_relationships.targetExists(commentsRelativePath))
			m_relationships.addRelationship(XLRelationshipType::Comments, commentsRelativePath);
	}

	return m_comments;
}

/**
 * @details fetches XLTables for the sheet - creates & assigns the class if empty
 */
XLTables& XLWorksheet::tables()
{
	if(!m_tables.valid()) {
		std::ignore = relationships(); // create sheet relationships if not existing

		// ===== Trigger parentDoc to create tables XML file and return it
		uint16 sheetXmlNo = sheetXmlNumber();
		// std::cout << "worksheet tables for sheetId " << sheetXmlNo << std::endl;
		m_tables = parentDoc().sheetTables(sheetXmlNo); // fetch tables for this worksheet
		if(!m_tables.valid())
			throw XLException("XLWorksheet::tables(): could not create tables XML");
		std::string tablesRelativePath = getPathARelativeToPathB(m_tables.getXmlPath(), getXmlPath() );
		if(!m_relationships.targetExists(tablesRelativePath))
			m_relationships.addRelationship(XLRelationshipType::Table, tablesRelativePath);
	}

	return m_tables;
}
/**
 * @details perform a pattern matching on getXmlPath for (regex) .*xl/worksheets/sheet([0-9]*)\.xml$ and extract the numeric part \1
 */
uint16 XLWorksheet::sheetXmlNumber() const
{
	constexpr const char * searchPattern = "xl/worksheets/sheet";
	std::string xmlPath = getXmlPath();
	size_t pos = xmlPath.find(searchPattern);                 // ensure compatibility with expected path
	if(pos == std::string::npos)  return 0;
	pos += strlen(searchPattern);
	size_t pos2 = pos;
	while(std::isdigit(xmlPath[pos2]))
		++pos2; // find sheet number in xmlPath - aborts on end of string
	if(pos2 == pos || xmlPath.substr(pos2) != ".xml")
		return 0;// ensure compatibility with expected path
	// success: convert the sheet number part of xmlPath to uint16
	return static_cast<uint16>(std::stoi(xmlPath.substr(pos, pos2 - pos)));
}
/**
 * @details fetches XLRelationships for the sheet - creates & assigns the class if empty
 */
XLRelationships& XLWorksheet::relationships()
{
	if(!m_relationships.valid()) {
		// trigger parentDoc to create relationships XML file and relationship and return it
		m_relationships = parentDoc().sheetRelationships(sheetXmlNumber()); // fetch relationships for this worksheet
	}
	if(!m_relationships.valid())
		throw XLException("XLWorksheet::relationships(): could not create relationships XML");
	return m_relationships;
}

XLChartsheet::XLChartsheet(XLXmlData* xmlData) : XLSheetBase(xmlData) {}
XLChartsheet::~XLChartsheet() = default;

XLColor XLChartsheet::getColor_impl() const
{
	return XLColor(xmlDocumentC().document_element().child("sheetPr").child("tabColor").attribute("rgb").value());
}
/**
 * @details Calls the setTabColor() free function.
 */
void XLChartsheet::setColor_impl(const XLColor& color) { setTabColor(xmlDocument(), color); }
/**
 * @details Calls the tabIsSelected() free function.
 */
bool XLChartsheet::isSelected_impl() const { return tabIsSelected(xmlDocumentC()); }
/**
 * @details Calls the setTabSelected() free function.
 */
void XLChartsheet::setSelected_impl(bool selected) { setTabSelected(xmlDocument(), selected); }
//
// XLStyles.cpp
//
namespace { // anonymous namespace for module local functions
	enum XLStylesEntryType : uint8 {
		XLStylesNumberFormats    =   0,
		XLStylesFonts            =   1,
		XLStylesFills            =   2,
		XLStylesBorders          =   3,
		XLStylesCellStyleFormats =   4,
		XLStylesCellFormats      =   5,
		XLStylesCellStyles       =   6,
		XLStylesColors           =   7,
		XLStylesDiffCellFormats  =   8,
		XLStylesTableStyles      =   9,
		XLStylesExtLst           =  10,
		XLStylesInvalid          = 255
	};

	XLStylesEntryType XLStylesEntryTypeFromString(std::string name)
	{
		if(name == "numFmts")       return XLStylesNumberFormats;
		if(name == "fonts")         return XLStylesFonts;
		if(name == "fills")         return XLStylesFills;
		if(name == "borders")       return XLStylesBorders;
		if(name == "cellStyleXfs")  return XLStylesCellStyleFormats;
		if(name == "cellXfs")       return XLStylesCellFormats;
		if(name == "cellStyles")    return XLStylesCellStyles;
		if(name == "colors")        return XLStylesColors;
		if(name == "dxfs")          return XLStylesDiffCellFormats;
		if(name == "tableStyles")   return XLStylesTableStyles;
		if(name == "extLst")        return XLStylesExtLst;
		return XLStylesEntryType::XLStylesInvalid;
	}

	std::string XLStylesEntryTypeToString(XLStylesEntryType entryType)
	{
		switch(entryType) {
			case XLStylesNumberFormats:    return "numFmts";
			case XLStylesFonts:            return "fonts";
			case XLStylesFills:            return "fills";
			case XLStylesBorders:          return "borders";
			case XLStylesCellStyleFormats: return "cellStyleXfs";
			case XLStylesCellFormats:      return "cellXfs";
			case XLStylesCellStyles:       return "cellStyles";
			case XLStylesColors:           return "colors";
			case XLStylesDiffCellFormats:  return "dxfs";
			case XLStylesTableStyles:      return "tableStyles";
			case XLStylesExtLst:           return "extLst";
			case XLStylesInvalid:          [[fallthrough]];
			default:                       return "(invalid)";
		}
	}

	XLUnderlineStyle XLUnderlineStyleFromString(std::string underline)
	{
		if(underline == ""
			|| underline == "none")   return XLUnderlineNone;
		if(underline == "single")  return XLUnderlineSingle;
		if(underline == "double")  return XLUnderlineDouble;
		std::cerr << __func__ << ": invalid underline style " << underline << std::endl;
		return XLUnderlineInvalid;
	}

	std::string XLUnderlineStyleToString(XLUnderlineStyle underline)
	{
		switch(underline) {
			case XLUnderlineNone: return "none";
			case XLUnderlineSingle: return "single";
			case XLUnderlineDouble: return "double";
			case XLUnderlineInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLFontSchemeStyle XLFontSchemeStyleFromString(std::string fontScheme)
	{
		if(fontScheme == "" || fontScheme == "none")  return XLFontSchemeNone;
		if(fontScheme == "major")  return XLFontSchemeMajor;
		if(fontScheme == "minor")  return XLFontSchemeMinor;
		std::cerr << __func__ << ": invalid font scheme " << fontScheme << std::endl;
		return XLFontSchemeInvalid;
	}

	std::string XLFontSchemeStyleToString(XLFontSchemeStyle fontScheme)
	{
		switch(fontScheme) {
			case XLFontSchemeNone: return "none";
			case XLFontSchemeMajor: return "major";
			case XLFontSchemeMinor: return "minor";
			case XLFontSchemeInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLVerticalAlignRunStyle XLVerticalAlignRunStyleFromString(std::string vertAlign)
	{
		if(vertAlign == "" || vertAlign == "baseline")    
			return XLBaseline;
		if(vertAlign == "subscript")    
			return XLSubscript;
		if(vertAlign == "superscript")  
			return XLSuperscript;
		std::cerr << __func__ << ": invalid font vertical align run style " << vertAlign << std::endl;
		return XLVerticalAlignRunInvalid;
	}

	std::string XLVerticalAlignRunStyleToString(XLVerticalAlignRunStyle vertAlign)
	{
		switch(vertAlign) {
			case XLBaseline: return "baseline";
			case XLSubscript: return "subscript";
			case XLSuperscript: return "superscript";
			case XLVerticalAlignRunInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLFillType XLFillTypeFromString(std::string fillType)
	{
		if(fillType == "gradientFill")  
			return XLGradientFill;
		if(fillType == "patternFill")   
			return XLPatternFill;
		if(fillType != "")     // suppress error message for empty fillType (= no node exists yet)
			std::cerr << __func__ << ": invalid fillType \"" << fillType << "\"" << std::endl;
		return XLFillTypeInvalid;
	}

	std::string XLFillTypeToString(XLFillType fillType)
	{
		switch(fillType) {
			case XLGradientFill: return "gradientFill";
			case XLPatternFill: return "patternFill";
			case XLFillTypeInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLGradientType XLGradientTypeFromString(std::string gradientType)
	{
		if(gradientType == "linear")  
			return XLGradientLinear;
		if(gradientType == "path")    
			return XLGradientPath;
		std::cerr << __func__ << ": invalid gradient type " << gradientType << std::endl;
		return XLGradientTypeInvalid;
	}

	std::string XLGradientTypeToString(XLGradientType gradientType)
	{
		switch(gradientType) {
			case XLGradientLinear: return "linear";
			case XLGradientPath: return "path";
			case XLGradientTypeInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLPatternType XLPatternTypeFromString(std::string patternType)
	{
		if(patternType == "" || patternType == "none") return XLPatternNone;
		if(patternType == "solid")            return XLPatternSolid;
		if(patternType == "mediumGray")       return XLPatternMediumGray;
		if(patternType == "darkGray")         return XLPatternDarkGray;
		if(patternType == "lightGray")        return XLPatternLightGray;
		if(patternType == "darkHorizontal")   return XLPatternDarkHorizontal;
		if(patternType == "darkVertical")     return XLPatternDarkVertical;
		if(patternType == "darkDown")         return XLPatternDarkDown;
		if(patternType == "darkUp")           return XLPatternDarkUp;
		if(patternType == "darkGrid")         return XLPatternDarkGrid;
		if(patternType == "darkTrellis")      return XLPatternDarkTrellis;
		if(patternType == "lightHorizontal")  return XLPatternLightHorizontal;
		if(patternType == "lightVertical")    return XLPatternLightVertical;
		if(patternType == "lightDown")        return XLPatternLightDown;
		if(patternType == "lightUp")          return XLPatternLightUp;
		if(patternType == "lightGrid")        return XLPatternLightGrid;
		if(patternType == "lightTrellis")     return XLPatternLightTrellis;
		if(patternType == "gray125")          return XLPatternGray125;
		if(patternType == "gray0625")         return XLPatternGray0625;
		std::cerr << __func__ << ": invalid patternType " << patternType << std::endl;
		return XLPatternTypeInvalid;
	}

	std::string XLPatternTypeToString(XLPatternType patternType)
	{
		switch(patternType) {
			case XLPatternNone: return "none";
			case XLPatternSolid: return "solid";
			case XLPatternMediumGray: return "mediumGray";
			case XLPatternDarkGray: return "darkGray";
			case XLPatternLightGray: return "lightGray";
			case XLPatternDarkHorizontal: return "darkHorizontal";
			case XLPatternDarkVertical: return "darkVertical";
			case XLPatternDarkDown: return "darkDown";
			case XLPatternDarkUp: return "darkUp";
			case XLPatternDarkGrid: return "darkGrid";
			case XLPatternDarkTrellis: return "darkTrellis";
			case XLPatternLightHorizontal: return "lightHorizontal";
			case XLPatternLightVertical: return "lightVertical";
			case XLPatternLightDown: return "lightDown";
			case XLPatternLightUp: return "lightUp";
			case XLPatternLightGrid: return "lightGrid";
			case XLPatternLightTrellis: return "lightTrellis";
			case XLPatternGray125: return "gray125";
			case XLPatternGray0625: return "gray0625";
			case XLPatternTypeInvalid: [[fallthrough]];
			default:                       return "(invalid)";
		}
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-function"
	#endif // __GNUC__
	XLLineType XLLineTypeFromString(std::string lineType)
	{
		if(lineType == "left")        return XLLineLeft;
		if(lineType == "right")       return XLLineRight;
		if(lineType == "top")         return XLLineTop;
		if(lineType == "bottom")      return XLLineBottom;
		if(lineType == "diagonal")    return XLLineDiagonal;
		if(lineType == "vertical")    return XLLineVertical;
		if(lineType == "horizontal")  return XLLineHorizontal;
		std::cerr << __func__ << ": invalid line type" << lineType << std::endl;
		return XLLineInvalid;
	}

	#ifdef __GNUC__    // conditionally enable GCC specific pragmas to suppress unused function warning
	#pragma GCC diagnostic pop
	#endif // __GNUC__

	std::string XLLineTypeToString(XLLineType lineType)
	{
		switch(lineType) {
			case XLLineLeft:       return "left";
			case XLLineRight:      return "right";
			case XLLineTop:        return "top";
			case XLLineBottom:     return "bottom";
			case XLLineDiagonal:   return "diagonal";
			case XLLineVertical:   return "vertical";
			case XLLineHorizontal: return "horizontal";
			case XLLineInvalid:    [[fallthrough]];
			default:               return "(invalid)";
		}
	}

	XLLineStyle XLLineStyleFromString(std::string style)
	{
		if(style == "")                  return XLLineStyleNone;
		if(style == "thin")              return XLLineStyleThin;
		if(style == "medium")            return XLLineStyleMedium;
		if(style == "dashed")            return XLLineStyleDashed;
		if(style == "dotted")            return XLLineStyleDotted;
		if(style == "thick")             return XLLineStyleThick;
		if(style == "double")            return XLLineStyleDouble;
		if(style == "hair")              return XLLineStyleHair;
		if(style == "mediumDashed")      return XLLineStyleMediumDashed;
		if(style == "dashDot")           return XLLineStyleDashDot;
		if(style == "mediumDashDot")     return XLLineStyleMediumDashDot;
		if(style == "dashDotDot")        return XLLineStyleDashDotDot;
		if(style == "mediumDashDotDot")  return XLLineStyleMediumDashDotDot;
		if(style == "slantDashDot")      return XLLineStyleSlantDashDot;
		std::cerr << __func__ << ": invalid line style " << style << std::endl;
		return XLLineStyleInvalid;
	}

	std::string XLLineStyleToString(XLLineStyle style)
	{
		switch(style) {
			case XLLineStyleNone: return "";
			case XLLineStyleThin: return "thin";
			case XLLineStyleMedium: return "medium";
			case XLLineStyleDashed: return "dashed";
			case XLLineStyleDotted: return "dotted";
			case XLLineStyleThick: return "thick";
			case XLLineStyleDouble: return "double";
			case XLLineStyleHair: return "hair";
			case XLLineStyleMediumDashed: return "mediumDashed";
			case XLLineStyleDashDot: return "dashDot";
			case XLLineStyleMediumDashDot: return "mediumDashDot";
			case XLLineStyleDashDotDot: return "dashDotDot";
			case XLLineStyleMediumDashDotDot: return "mediumDashDotDot";
			case XLLineStyleSlantDashDot: return "slantDashDot";
			case XLLineStyleInvalid: [[fallthrough]];
			default: return "(invalid)";
		}
	}

	XLAlignmentStyle XLAlignmentStyleFromString(std::string alignment)
	{
		if(alignment == ""
			|| alignment == "general")          return XLAlignGeneral;
		if(alignment == "left")              return XLAlignLeft;
		if(alignment == "right")             return XLAlignRight;
		if(alignment == "center")            return XLAlignCenter;
		if(alignment == "top")               return XLAlignTop;
		if(alignment == "bottom")            return XLAlignBottom;
		if(alignment == "fill")              return XLAlignFill;
		if(alignment == "justify")           return XLAlignJustify;
		if(alignment == "centerContinuous")  return XLAlignCenterContinuous;
		if(alignment == "distributed")       return XLAlignDistributed;
		std::cerr << __func__ << ": invalid alignment style " << alignment << std::endl;
		return XLAlignInvalid;
	}

	std::string XLAlignmentStyleToString(XLAlignmentStyle alignment)
	{
		switch(alignment) {
			case XLAlignGeneral: return "";
			case XLAlignLeft: return "left";
			case XLAlignRight: return "right";
			case XLAlignCenter: return "center";
			case XLAlignTop: return "top";
			case XLAlignBottom: return "bottom";
			case XLAlignFill: return "fill";
			case XLAlignJustify: return "justify";
			case XLAlignCenterContinuous: return "centerContinuous";
			case XLAlignDistributed: return "distributed";
			case XLAlignInvalid: [[fallthrough]];
			default: return "(unknown)";
		}
	}

	std::string XLReadingOrderToString(uint32 readingOrder)
	{
		switch(readingOrder) {
			case XLReadingOrderContextual: return "contextual";
			case XLReadingOrderLeftToRight: return "left-to-right";
			case XLReadingOrderRightToLeft: return "right-to-left";
			default: return "(unknown)";
		}
	}

	void copyXMLNode(XMLNode & destination, XMLNode & source)
	{
		if(not source.empty()) {
			// ===== Copy all XML child nodes
			for(XMLNode child = source.first_child(); not child.empty(); child = child.next_sibling())
				destination.append_copy(child);
			// ===== Copy all XML attributes
			for(XMLAttribute attr = source.first_attribute(); not attr.empty(); attr = attr.next_attribute())
				destination.append_copy(attr);
		}
	}

	void wrapNode(XMLNode parentNode, XMLNode & node, std::string const & prefix)
	{
		if(not node.empty() && prefix.length() > 0) {
			parentNode.insert_child_before(pugi::node_pcdata, node).set_value(prefix.c_str()); // insert prefix before node opening tag
			node.append_child(pugi::node_pcdata).set_value(prefix.c_str());                   // insert prefix before node closing tag (within node)
		}
	}

	/**
	 * @brief Format val as a string with decimalPlaces
	 * @param val The value to format
	 * @param decimalPlaces The amount of digits following the decimal separator that shall be included in the formatted string
	 * @return The value formatted as a string with the desired amount of decimal places
	 */
	std::string formatDoubleAsString(double val, int decimalPlaces = 2)
	{
		std::string result = std::to_string(val);
		size_t decimalPos = result.find_first_of('.');
		if(decimalPos >= result.length())
			throw XLException("formatDoubleAsString: return value of std::to_string(double val) contains no decimal separator - this should never happen");

		// ===== Return the string representation of val with the decimal separator and decimalPlaces digits following
		return result.substr(0, decimalPos + 1 + decimalPlaces);
	}

	/**
	 * @brief Check that a double value is within range, and format it as a string with decimalPlaces
	 * @param val The value to check & format
	 * @param min The lower bound of the valid value range
	 * @param max The upper bound of the valid value range 0 must be larger than min
	 * @param absTolerance The tolerance for rounding errors when checking the valid range - must be positive
	 * @param decimalPlaces The amount of digits following the decimal separator that shall be included in the formatted string
	 * @return The value formatted as a string with the desired amount of decimal places
	 * @return An empty string ""s if the value falls outside the valid range
	 * @note values that fall outside the range but within the tolerance will be rounded to the nearest range bound
	 */
	std::string checkAndFormatDoubleAsString(double val, double min, double max, double absTolerance, int decimalPlaces = 2)
	{
		if(max <= min || absTolerance < 0.0)  
			throw XLException("checkAndFormatDoubleAsString: max must be greater than min and absTolerance must be >= 0.0");
		if(val < min - absTolerance || val > max + absTolerance)  
			return ""; // range check
		if(val < min)  
			val = min; 
		else if(val > max)  
			val = max; // fix rounding errors within tolerance
		return formatDoubleAsString(val, decimalPlaces);
	}
}
/**
 * @details Constructor. Initializes an empty XLNumberFormat object
 */
XLNumberFormat::XLNumberFormat() : m_numberFormatNode(std::make_unique<XMLNode>()) 
{
}
/**
 * @details Constructor. Initializes the member variables for the new XLNumberFormat object.
 */
XLNumberFormat::XLNumberFormat(const XMLNode& node) : m_numberFormatNode(std::make_unique<XMLNode>(node)) 
{
}

XLNumberFormat::~XLNumberFormat() = default;

XLNumberFormat::XLNumberFormat(const XLNumberFormat& other)
	: m_numberFormatNode(std::make_unique<XMLNode>(*other.m_numberFormatNode))
{}

XLNumberFormat& XLNumberFormat::operator=(const XLNumberFormat& other)
{
	if(&other != this)  *m_numberFormatNode = *other.m_numberFormatNode;
	return *this;
}

/**
 * @details Returns the numFmtId value
 */
uint32 XLNumberFormat::numberFormatId() const { return m_numberFormatNode->attribute("numFmtId").as_uint(XLInvalidUInt32); }

/**
 * @details Returns the formatCode value
 */
std::string XLNumberFormat::formatCode() const { return m_numberFormatNode->attribute("formatCode").value(); }

/**
 * @details Setter functions
 */
bool XLNumberFormat::setNumberFormatId(uint32 newNumberFormatId)
{ return appendAndSetAttribute(*m_numberFormatNode, "numFmtId",   std::to_string(newNumberFormatId)).empty() == false; }
bool XLNumberFormat::setFormatCode(std::string newFormatCode)
{ return appendAndSetAttribute(*m_numberFormatNode, "formatCode", newFormatCode.c_str()            ).empty() == false; }

/**
 * @details assemble a string summary about the number format
 */
std::string XLNumberFormat::summary() const
{
	using namespace std::literals::string_literals;
	return "numFmtId="s + std::to_string(numberFormatId())
	       + ", formatCode="s + formatCode();
}

// ===== XLNumberFormats, parent of XLNumberFormat

/**
 * @details Constructor. Initializes an empty XLNumberFormats object
 */
XLNumberFormats::XLNumberFormats() : m_numberFormatsNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLNumberFormats object.
 */
XLNumberFormats::XLNumberFormats(const XMLNode& numberFormats)
	: m_numberFormatsNode(std::make_unique<XMLNode>(numberFormats))
{
	// initialize XLNumberFormat entries and m_numberFormats here
	XMLNode node = numberFormats.first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "numFmt")
			m_numberFormats.push_back(XLNumberFormat(node));
		else
			std::cerr << "WARNING: XLNumberFormats constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLNumberFormats::~XLNumberFormats()
{
	m_numberFormats.clear(); // delete vector with all children
}

XLNumberFormats::XLNumberFormats(const XLNumberFormats& other)
	: m_numberFormatsNode(std::make_unique<XMLNode>(*other.m_numberFormatsNode)),
	m_numberFormats(other.m_numberFormats)
{}

XLNumberFormats::XLNumberFormats(XLNumberFormats&& other)
	: m_numberFormatsNode(std::move(other.m_numberFormatsNode)),
	m_numberFormats(std::move(other.m_numberFormats))
{}

XLNumberFormats& XLNumberFormats::operator=(const XLNumberFormats& other)
{
	if(&other != this) {
		*m_numberFormatsNode = *other.m_numberFormatsNode;
		m_numberFormats.clear();
		m_numberFormats = other.m_numberFormats;
	}
	return *this;
}

/**
 * @details Returns the amount of numberFormats held by the class
 */
size_t XLNumberFormats::count() const { return m_numberFormats.size(); }

/**
 * @details fetch XLNumberFormat from m_numberFormats by index
 */
XLNumberFormat XLNumberFormats::numberFormatByIndex(XLStyleIndex index) const
{
	if(index >= m_numberFormats.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLNumberFormats::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	}
	return m_numberFormats.at(index);
}

/**
 * @details fetch XLNumberFormat from m_numberFormats by its numberFormatId
 */
XLNumberFormat XLNumberFormats::numberFormatById(uint32 numberFormatId) const
{
	for(XLNumberFormat fmt : m_numberFormats)
		if(fmt.numberFormatId() == numberFormatId)
			return fmt;
	using namespace std::literals::string_literals;
	throw XLException("XLNumberFormats::"s + __func__ + ": numberFormatId "s + std::to_string(numberFormatId) + " not found"s);
}

/**
 * @details fetch a numFmtId from m_numberFormats by index
 */
uint32 XLNumberFormats::numberFormatIdFromIndex(XLStyleIndex index) const
{
	if(index >= m_numberFormats.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLNumberFormats::"s + __func__ + ": index "s + std::to_string(index) + " is out of range"s);
	}
	return m_numberFormats[ index ].numberFormatId();
}

/**
 * @details append a new XLNumberFormat to m_numberFormats and m_numberFormatsNode, based on copyFrom
 */
XLStyleIndex XLNumberFormats::create(XLNumberFormat copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the number format to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_numberFormatsNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_numberFormatsNode->prepend_child("numFmt");
	else newNode = m_numberFormatsNode->insert_child_after("numFmt", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLNumberFormats::"s + __func__ + ": failed to append a new numFmt node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_numberFormatsNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLNumberFormat newNumberFormat(newNode);
	if(copyFrom.m_numberFormatNode->empty()) { // if no template is given
		// ===== Create a number format with default values
		newNumberFormat.setNumberFormatId(0);
		newNumberFormat.setFormatCode("General");
	}
	else
		copyXMLNode(newNode, *copyFrom.m_numberFormatNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_numberFormats.push_back(newNumberFormat);
	appendAndSetAttribute(*m_numberFormatsNode, "count", std::to_string(m_numberFormats.size())); // update array count in XML
	return index;
}

/**
 * @details Constructor. Initializes an empty XLFont object
 */
XLFont::XLFont() : m_fontNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLFont object.
 */
XLFont::XLFont(const XMLNode& node) : m_fontNode(std::make_unique<XMLNode>(node)) {}

XLFont::~XLFont() = default;

XLFont::XLFont(const XLFont& other)
	: m_fontNode(std::make_unique<XMLNode>(*other.m_fontNode))
{}

XLFont& XLFont::operator=(const XLFont& other)
{
	if(&other != this)  *m_fontNode = *other.m_fontNode;
	return *this;
}

/**
 * @details Returns the font name property
 */
std::string XLFont::fontName() const
{
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "name", "val", OpenXLSX::XLDefaultFontName);
	return attr.value();
}

/**
 * @details Returns the font charset property
 */
size_t XLFont::fontCharset() const
{
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "charset", "val", std::to_string(OpenXLSX::XLDefaultFontCharset));
	return attr.as_uint();
}

/**
 * @details Returns the font family property
 */
size_t XLFont::fontFamily() const
{
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "family", "val", std::to_string(OpenXLSX::XLDefaultFontFamily));
	return attr.as_uint();
}

/**
 * @details Returns the font size property
 */
size_t XLFont::fontSize() const
{
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "sz", "val", std::to_string(OpenXLSX::XLDefaultFontSize));
	return attr.as_uint();
}

/**
 * @details Returns the font color property
 */
XLColor XLFont::fontColor() const
{
	using namespace std::literals::string_literals;
	// XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "color", "theme", OpenXLSX::XLDefaultFontColorTheme);
	// TBD what "theme" is and whether it should be supported at all
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fontNode, "color", "rgb", XLDefaultFontColor);
	return XLColor(attr.value());
}
/**
 * @details getter functions: return the font's bold, italic, underline, strikethrough status
 */
bool XLFont::bold()   const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "b", "val"); }
bool XLFont::italic() const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "i", "val"); }
bool XLFont::strikethrough() const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "strike", "val"); }
XLUnderlineStyle XLFont::underline() const { return XLUnderlineStyleFromString(appendAndGetNodeAttribute(*m_fontNode, "u", "val", "none").value()  ); }
XLFontSchemeStyle XLFont::scheme() const { return XLFontSchemeStyleFromString(appendAndGetNodeAttribute(*m_fontNode, "scheme", "val", "none").value()  ); }
XLVerticalAlignRunStyle XLFont::vertAlign() const { return XLVerticalAlignRunStyleFromString(appendAndGetNodeAttribute(*m_fontNode, "vertAlign", "val", "baseline").value()); }
bool XLFont::outline()  const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "outline",  "val"); }
bool XLFont::shadow()   const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "shadow",   "val"); }
bool XLFont::condense() const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "condense", "val"); }
bool XLFont::extend()   const { return getBoolAttributeWhenOmittedMeansTrue(*m_fontNode, "extend",   "val"); }
/**
 * @details Setter functions
 */
bool XLFont::setFontName(std::string newName) { return appendAndSetNodeAttribute(*m_fontNode, "name", "val", newName.c_str()).empty() == false; }
bool XLFont::setFontCharset(size_t newCharset) { return appendAndSetNodeAttribute(*m_fontNode, "charset", "val", std::to_string(newCharset)).empty() == false; }
bool XLFont::setFontFamily(size_t newFamily) { return appendAndSetNodeAttribute(*m_fontNode, "family", "val", std::to_string(newFamily)).empty() == false; }
bool XLFont::setFontSize(size_t newSize) { return appendAndSetNodeAttribute(*m_fontNode, "sz", "val", std::to_string(newSize)).empty() == false; }
bool XLFont::setFontColor(XLColor newColor) { return appendAndSetNodeAttribute(*m_fontNode, "color", "rgb", newColor.hex(), XLRemoveAttributes).empty() == false; }
bool XLFont::setBold(bool set) { return appendAndSetNodeAttribute(*m_fontNode, "b", "val", (set ? "true" : "false")).empty() == false; }
bool XLFont::setItalic(bool set) { return appendAndSetNodeAttribute(*m_fontNode, "i", "val", (set ? "true" : "false")).empty() == false; }
bool XLFont::setStrikethrough(bool set)               
	{ return appendAndSetNodeAttribute(*m_fontNode, "strike", "val", (set ? "true" : "false")).empty() == false; }
bool XLFont::setUnderline(XLUnderlineStyle style)
	{ return appendAndSetNodeAttribute(*m_fontNode, "u", "val", XLUnderlineStyleToString(style).c_str()).empty() == false; }
bool XLFont::setScheme(XLFontSchemeStyle newScheme)
	{ return appendAndSetNodeAttribute(*m_fontNode, "scheme", "val", XLFontSchemeStyleToString(newScheme).c_str()).empty() == false; }
bool XLFont::setVertAlign(XLVerticalAlignRunStyle newVertAlign)
	{ return appendAndSetNodeAttribute(*m_fontNode, "vertAlign", "val", XLVerticalAlignRunStyleToString(newVertAlign).c_str()).empty() == false; }
bool XLFont::setOutline(bool set) { return appendAndSetNodeAttribute(*m_fontNode, "outline",  "val", (set ? "true" : "false")               ).empty() == false; }
bool XLFont::setShadow(bool set) { return appendAndSetNodeAttribute(*m_fontNode, "shadow",   "val", (set ? "true" : "false")               ).empty() == false; }
bool XLFont::setCondense(bool set) 
{
	return appendAndSetNodeAttribute(*m_fontNode, "condense", "val", (set ? "true" : "false")).empty() == false;
}

bool XLFont::setExtend(bool set) { return appendAndSetNodeAttribute(*m_fontNode, "extend", "val", (set ? "true" : "false")).empty() == false; }

/**
 * @details assemble a string summary about the font
 */
std::string XLFont::summary() const
{
	using namespace std::literals::string_literals;
	return "font name is "s + fontName()
	       + ", charset: "s + std::to_string(fontCharset())
	       + ", font family: "s + std::to_string(fontFamily())
	       + ", size: "s + std::to_string(fontSize())
	       + ", color: "s + fontColor().hex()
	       + (bold() ? ", +bold"s : ""s)
	       + (italic() ? ", +italic"s : ""s)
	       + (strikethrough() ? ", +strikethrough"s : ""s)
	       + (underline() != XLUnderlineNone ? ", underline: "s + XLUnderlineStyleToString(underline()) : ""s)
	       + (scheme() != XLFontSchemeNone ? ", scheme: "s + XLFontSchemeStyleToString(scheme()) : ""s)
	       + (vertAlign() != XLBaseline ? ", vertAlign: "s + XLVerticalAlignRunStyleToString(vertAlign()) : ""s)
	       + (outline() ? ", +outline"s : ""s)
	       + (shadow() ? ", +shadow"s : ""s)
	       + (condense() ? ", +condense"s : ""s)
	       + (extend() ? ", +extend"s : ""s);
}

// ===== XLFonts, parent of XLFont

/**
 * @details Constructor. Initializes an empty XLFonts object
 */
XLFonts::XLFonts() : m_fontsNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLFonts object.
 */
XLFonts::XLFonts(const XMLNode& fonts)
	: m_fontsNode(std::make_unique<XMLNode>(fonts))
{
	// initialize XLFonts entries and m_fonts here
	XMLNode node = m_fontsNode->first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "font")
			m_fonts.push_back(XLFont(node));
		else
			std::cerr << "WARNING: XLFonts constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLFonts::~XLFonts()
{
	m_fonts.clear(); // delete vector with all children
}

XLFonts::XLFonts(const XLFonts& other) : m_fontsNode(std::make_unique<XMLNode>(*other.m_fontsNode)), m_fonts(other.m_fonts)
{
}

XLFonts::XLFonts(XLFonts&& other) : m_fontsNode(std::move(other.m_fontsNode)), m_fonts(std::move(other.m_fonts))
{
}

XLFonts& XLFonts::operator=(const XLFonts& other)
{
	if(&other != this) {
		*m_fontsNode = *other.m_fontsNode;
		m_fonts.clear();
		m_fonts = other.m_fonts;
	}
	return *this;
}

/**
 * @details Returns the amount of fonts held by the class
 */
size_t XLFonts::count() const { return m_fonts.size(); }

/**
 * @details fetch XLFont from m_Fonts by index
 */
XLFont XLFonts::fontByIndex(XLStyleIndex index) const
{
	if(index >= m_fonts.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLFonts::"s + __func__ + ": attempted to access index "s + std::to_string(index) + " with count "s + std::to_string(m_fonts.size()));
	}
	return m_fonts.at(index);
}

/**
 * @details append a new XLFont to m_fonts and m_fontsNode, based on copyFrom
 */
XLStyleIndex XLFonts::create(XLFont copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the font to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_fontsNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_fontsNode->prepend_child("font");
	else newNode = m_fontsNode->insert_child_after("font", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLFonts::"s + __func__ + ": failed to append a new fonts node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_fontsNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLFont newFont(newNode);
	if(copyFrom.m_fontNode->empty()) { // if no template is given
		// ===== Create a font with default values
		// TODO: implement font defaults
		// newFont.setProperty(defaultValue);
		// ...
	}
	else
		copyXMLNode(newNode, *copyFrom.m_fontNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_fonts.push_back(newFont);
	appendAndSetAttribute(*m_fontsNode, "count", std::to_string(m_fonts.size())); // update array count in XML
	return index;
}

// ===== XLDataBarColor, used by XLFills gradientFill and by XLLine (to be implemented)

/**
 * @details Constructor. Initializes an empty XLDataBarColor object
 */
XLDataBarColor::XLDataBarColor() : m_colorNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLDataBarColor object.
 */
XLDataBarColor::XLDataBarColor(const XMLNode& node) : m_colorNode(std::make_unique<XMLNode>(node)) {}

/**
 * @details Copy constructor - initializes the member variables from other
 */
XLDataBarColor::XLDataBarColor(const XLDataBarColor& other)
	: m_colorNode(std::make_unique<XMLNode>(*other.m_colorNode))
{}

/**
 * @details Assign values of other to this
 */
XLDataBarColor& XLDataBarColor::operator=(const XLDataBarColor& other)
{
	if(&other != this)  
		*m_colorNode = *other.m_colorNode;
	return *this;
}
/**
 * @details Getter functions
 */
XLColor XLDataBarColor::rgb()    const { return XLColor(m_colorNode->attribute("rgb").as_string("ffffffff")); }
double XLDataBarColor::tint()    const { return m_colorNode->attribute("tint").as_double(0.0); }
bool XLDataBarColor::automatic() const { return m_colorNode->attribute("auto").as_bool(); }
uint32 XLDataBarColor::indexed() const { return m_colorNode->attribute("indexed").as_uint(); }
uint32 XLDataBarColor::theme()   const { return m_colorNode->attribute("theme").as_uint(); }
/**
 * @details Setter functions
 */
bool XLDataBarColor::setRgb(XLColor newColor)
{ 
	return appendAndSetAttribute(*m_colorNode, "rgb", newColor.hex()).empty() == false; 
}

bool XLDataBarColor::setTint(double newTint)
{
	std::string tintString = "";
	if(newTint != 0.0) {
		tintString = checkAndFormatDoubleAsString(newTint, -1.0, +1.0, 0.01);
		if(tintString.length() == 0) {
			using namespace std::literals::string_literals;
			throw XLException("XLDataBarColor::setTint: color tint "s + std::to_string(newTint) + " is not in range [-1.0;+1.0]"s);
		}
	}
	if(tintString.length() == 0)  
		return m_colorNode->remove_attribute("tint");       // remove tint attribute for a value 0
	return (appendAndSetAttribute(*m_colorNode, "tint", tintString).empty() == false); // else: set tint attribute
}

bool XLDataBarColor::setAutomatic(bool set)        { return appendAndSetAttribute(*m_colorNode, "auto",      (set ? "true" : "false")     ).empty() == false; }
bool XLDataBarColor::setIndexed(uint32 newIndex) { return appendAndSetAttribute(*m_colorNode, "indexed",   std::to_string(newIndex)     ).empty() == false; }
bool XLDataBarColor::setTheme(uint32 newTheme)   {
	if(newTheme == XLDeleteProperty)   return m_colorNode->remove_attribute("theme");
	return appendAndSetAttribute(*m_colorNode, "theme",     std::to_string(newTheme)     ).empty() == false;
}

/**
 * @details assemble a string summary about the color
 */
std::string XLDataBarColor::summary() const
{
	using namespace std::literals::string_literals;
	return "rgb is "s + rgb().hex()
	       + ", tint is "s + formatDoubleAsString(tint())
	       + (automatic() ? ", +automatic"s : ""s)
	       + (indexed() ? (", index is "s + std::to_string(indexed())) : "")
	       + (theme()   ? (", theme is "s + std::to_string(theme()  )) : "")
	;
}

/**
 * @details Constructor. Initializes an empty XLGradientStop object
 */
XLGradientStop::XLGradientStop() : m_stopNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLGradientStop object.
 */
XLGradientStop::XLGradientStop(const XMLNode& node) : m_stopNode(std::make_unique<XMLNode>(node)) {}

/**
 * @details Copy constructor - initializes the member variables from other
 */
XLGradientStop::XLGradientStop(const XLGradientStop& other)
	: m_stopNode(std::make_unique<XMLNode>(*other.m_stopNode))
{}

/**
 * @details Assign values of other to this
 */
XLGradientStop& XLGradientStop::operator=(const XLGradientStop& other)
{
	if(&other != this)  *m_stopNode = *other.m_stopNode;
	return *this;
}

/**
 * @details Getter functions
 */
XLDataBarColor XLGradientStop::color() const
{
	XMLNode color = appendAndGetNode(*m_stopNode, "color");
	if(color.empty())  return XLDataBarColor{};
	return XLDataBarColor(color);
}

double XLGradientStop::position() const
{
	XMLAttribute attr = m_stopNode->attribute("position");
	return attr.as_double(0.0);
}

/**
 * @details Setter functions
 */
bool XLGradientStop::setPosition(double newPosition) { return appendAndSetAttribute(*m_stopNode, "position", formatDoubleAsString(newPosition)).empty() == false; }

/**
 * @details assemble a string summary about the stop
 */
std::string XLGradientStop::summary() const
{
	using namespace std::literals::string_literals;
	return "stop position is "s + formatDoubleAsString(position())
	       + ", " + color().summary();
}

// ===== XLGradientStops, parent of XLGradientStop

/**
 * @details Constructor. Initializes an empty XLGradientStops object
 */
XLGradientStops::XLGradientStops() : m_gradientNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLGradientStops object.
 */
XLGradientStops::XLGradientStops(const XMLNode& gradient)
	: m_gradientNode(std::make_unique<XMLNode>(gradient))
{
	// initialize XLGradientStops entries and m_gradientStops here
	XMLNode node = m_gradientNode->first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "stop")
			m_gradientStops.push_back(XLGradientStop(node));
		else
			std::cerr << "WARNING: XLGradientStops constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLGradientStops::~XLGradientStops()
{
	m_gradientStops.clear(); // delete vector with all children
}

XLGradientStops::XLGradientStops(const XLGradientStops& other)
	: m_gradientNode(std::make_unique<XMLNode>(*other.m_gradientNode)),
	m_gradientStops(other.m_gradientStops)
{}

XLGradientStops::XLGradientStops(XLGradientStops&& other)
	: m_gradientNode(std::move(other.m_gradientNode)),
	m_gradientStops(std::move(other.m_gradientStops))
{}

XLGradientStops& XLGradientStops::operator=(const XLGradientStops& other)
{
	if(&other != this) {
		*m_gradientNode = *other.m_gradientNode;
		m_gradientStops.clear();
		m_gradientStops = other.m_gradientStops;
	}
	return *this;
}

/**
 * @details Returns the amount of gradient stops held by the class
 */
size_t XLGradientStops::count() const { return m_gradientStops.size(); }

/**
 * @details fetch XLGradientStop from m_gradientStops by index
 */
XLGradientStop XLGradientStops::stopByIndex(XLStyleIndex index) const
{
	if(index >= m_gradientStops.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLGradientStops::"s + __func__ + ": attempted to access index "s + std::to_string(index) + " with count "s +
			  std::to_string(m_gradientStops.size()));
	}
	return m_gradientStops.at(index);
}

/**
 * @details append a new XLGradientStop to m_gradientStops m_gradientNode, based on copyFrom
 */
XLStyleIndex XLGradientStops::create(XLGradientStop copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the gradient stop to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_gradientNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_gradientNode->prepend_child("stop");
	else newNode = m_gradientNode->insert_child_after("stop", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLGradientStops::"s + __func__ + ": failed to append a new stop node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_gradientNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLGradientStop newStop(newNode);
	if(copyFrom.m_stopNode->empty()) { // if no template is given
		// ===== Create a stop node with default values
		// TODO: implement stop defaults
		// newStop.setProperty(defaultValue);
		// ...
	}
	else
		copyXMLNode(newNode, *copyFrom.m_stopNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_gradientStops.push_back(newStop);
	appendAndSetAttribute(*m_gradientNode, "count", std::to_string(m_gradientStops.size())); // update array count in XML
	return index;
}

std::string XLGradientStops::summary() const
{
	std::string result{};
	for(XLGradientStop stop : m_gradientStops)
		result += stop.summary() + ", ";
	if(result.length() < 2)  return "";// if no stop summary was created - return empty string
	return result.substr(0, result.length() - 2);
}

/**
 * @details Constructor. Initializes an empty XLFill object
 */
XLFill::XLFill() : m_fillNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLFill object.
 */
XLFill::XLFill(const XMLNode& node) : m_fillNode(std::make_unique<XMLNode>(node)) {}

XLFill::~XLFill() = default;

XLFill::XLFill(const XLFill& other)
	: m_fillNode(std::make_unique<XMLNode>(*other.m_fillNode))
{}

XLFill& XLFill::operator=(const XLFill& other)
{
	if(&other != this)  *m_fillNode = *other.m_fillNode;
	return *this;
}

/**
 * @details Returns the name of the first element child of fill
 * @note an empty node ::name() returns an empty string "", leading to XLFillTypeInvalid
 */
XLFillType XLFill::fillType() const { return XLFillTypeFromString(m_fillNode->first_child_of_type(pugi::node_element).name()); }

/**
 * @details set the fill type for a fill node - if force is true, delete any existing fill properties
 */
bool XLFill::setFillType(XLFillType newFillType, bool force)
{
	XLFillType ft = fillType(); // determine once, use twice
	// ===== If desired filltype is already set
	if(ft == newFillType)  
		return true;// nothing to do
	// ===== If force == true or fillType is just not set at all, delete existing child nodes, otherwise throw
	if(!force && ft != XLFillTypeInvalid) {
		using namespace std::literals::string_literals;
		throw XLException("XLFill::setFillType: can not change the fillType from "s + XLFillTypeToString(fillType())
			  + " to "s + XLFillTypeToString(newFillType) + " - invoke with force == true if override is desired"s);
	}
	// ===== At this point, m_fillNode needs to be emptied for a setting / force-change of the fill type

	// ===== Delete all fill node children and insert a new node for the newFillType
	m_fillNode->remove_children();
	return (m_fillNode->append_child(XLFillTypeToString(newFillType).c_str()).empty() == false);
}
/**
 * @details Throw an XLException on a fill of typeToThrowOn
 */
void XLFill::throwOnFillType(XLFillType typeToThrowOn, const char * functionName) const
{
	using namespace std::literals::string_literals;
	if(fillType() == typeToThrowOn)
		throw XLException("XLFill::"s + functionName + " must not be invoked for a "s + XLFillTypeToString(typeToThrowOn));
}
/**
 * @details get the fill element XML, create element with default XLFillType if none exists
 */
XMLNode XLFill::getValidFillDescription(XLFillType fillTypeIfEmpty, const char * functionName)
{
	XLFillType throwOnThis = XLFillTypeInvalid;
	switch(fillTypeIfEmpty) {
		case XLGradientFill: throwOnThis = XLPatternFill; break; // throw on non-matching fill type
		case XLPatternFill: throwOnThis = XLGradientFill; break; //   "
		default: throw XLException("XLFill::getValidFillDescription was not invoked with XLPatternFill or XLGradientFill");
	}
	throwOnFillType(throwOnThis, functionName);
	XMLNode fillDescription = m_fillNode->first_child_of_type(pugi::node_element); // fetch an existing fill description
	if(fillDescription.empty() && setFillType(fillTypeIfEmpty))                // if none exists, attempt to create a description
		fillDescription = m_fillNode->first_child_of_type(pugi::node_element); // fetch newly inserted description
	return fillDescription;
}
/**
 * @details Getter functions for gradientFill
 */
XLGradientType XLFill::gradientType() { return XLGradientTypeFromString(getValidFillDescription(XLGradientFill, __func__).attribute("type").value()     ); }
double XLFill::degree()       { return getValidFillDescription(XLGradientFill, __func__).attribute("degree").as_double(0); }
double XLFill::left()         { return getValidFillDescription(XLGradientFill, __func__).attribute("left").as_double(0); }
double XLFill::right()        { return getValidFillDescription(XLGradientFill, __func__).attribute("right").as_double(0); }
double XLFill::top()          { return getValidFillDescription(XLGradientFill, __func__).attribute("top").as_double(0); }
double XLFill::bottom()       { return getValidFillDescription(XLGradientFill, __func__).attribute("bottom").as_double(0); }
XLGradientStops XLFill::stops()        { return XLGradientStops(getValidFillDescription(XLGradientFill, __func__)                                 ); }

/**
 * @details Getter functions for patternFill
 */
XLPatternType XLFill::patternType()
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  return XLDefaultPatternType;// if no description could be fetched: fail
	XMLAttribute attr = appendAndGetNodeAttribute(*m_fillNode, XLFillTypeToString(XLPatternFill), "patternType", XLPatternTypeToString(XLDefaultPatternType));
	return XLPatternTypeFromString(attr.value());
}

XLColor XLFill::color()
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  
		return XLColor{};// if no description could be fetched: fail
	XMLAttribute fgColorRGB = appendAndGetNodeAttribute(fillDescription, "fgColor", "rgb", XLDefaultPatternFgColor);
	return XLColor(fgColorRGB.value());
}

XLColor XLFill::backgroundColor()
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  
		return XLColor{};// if no description could be fetched: fail
	XMLAttribute bgColorRGB = appendAndGetNodeAttribute(fillDescription, "bgColor", "rgb", XLDefaultPatternBgColor);
	return XLColor(bgColorRGB.value());
}
/**
 * @details Setter functions for gradientFill
 */
bool XLFill::setGradientType(XLGradientType newType)
{
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "type", XLGradientTypeToString(newType)).empty() == false;
}

bool XLFill::setDegree(double newDegree)
{
	std::string degreeString = checkAndFormatDoubleAsString(newDegree, 0.0, 360.0, 0.01);
	if(degreeString.length() == 0) {
		using namespace std::literals::string_literals;
		throw XLException("XLFill::setDegree: gradientFill degree value "s + std::to_string(newDegree) + " is not in range [0.0;360.0]"s);
	}
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "degree", degreeString).empty() == false;
}

bool XLFill::setLeft(double newLeft)
{
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "left",   formatDoubleAsString(newLeft).c_str()).empty() == false;
}

bool XLFill::setRight(double newRight)
{
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "right",  formatDoubleAsString(newRight).c_str()).empty() == false;
}

bool XLFill::setTop(double newTop)
{
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "top",    formatDoubleAsString(newTop).c_str()).empty() == false;
}

bool XLFill::setBottom(double newBottom)
{
	XMLNode fillDescription = getValidFillDescription(XLGradientFill, __func__);
	return appendAndSetAttribute(fillDescription, "bottom", formatDoubleAsString(newBottom).c_str()).empty() == false;
}
/**
 * @details Setter functions for patternFill
 */
bool XLFill::setPatternType(XLPatternType newFillPattern)
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  
		return false;// if no description could be fetched: fail
	return appendAndSetAttribute(fillDescription, "patternType", XLPatternTypeToString(newFillPattern)).empty() == false;
}

bool XLFill::setColor(XLColor newColor)
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  
		return false;// if no description could be fetched: fail
	return appendAndSetNodeAttribute(fillDescription, "fgColor", "rgb", newColor.hex(), XLRemoveAttributes).empty() == false;
}

bool XLFill::setBackgroundColor(XLColor newBgColor)
{
	XMLNode fillDescription = getValidFillDescription(XLPatternFill, __func__);
	if(fillDescription.empty())  
		return false;// if no description could be fetched: fail
	return appendAndSetNodeAttribute(fillDescription, "bgColor", "rgb", newBgColor.hex(), XLRemoveAttributes).empty() == false;
}

/**
 * @details assemble a string summary about the fill
 */
std::string XLFill::summary()
{
	using namespace std::literals::string_literals;
	switch(fillType()) {
		case XLGradientFill:
		    return "fill type is "s + XLFillTypeToString(fillType())
			   + ", gradient type is "s + XLGradientTypeToString(gradientType())
			   + ", degree is: "s + formatDoubleAsString(degree())
			   + ", left is: "s   + formatDoubleAsString(left())
			   + ", right is: "s  + formatDoubleAsString(right())
			   + ", top is: "s    + formatDoubleAsString(top())
			   + ", bottom is: "s + formatDoubleAsString(bottom())
			   + ", stops: "s + stops().summary();
		case XLPatternFill:
		    return "fill type is "s + XLFillTypeToString(fillType())
			   + ", pattern type is "s + XLPatternTypeToString(patternType())
			   + ", fgcolor is: "s + color().hex() + ", bgcolor: "s + backgroundColor().hex();
		case XLFillTypeInvalid: [[fallthrough]];
		default:
		    return "fill type is invalid!"s;
	}
}

// ===== XLFills, parent of XLFill

/**
 * @details Constructor. Initializes an empty XLFills object
 */
XLFills::XLFills() : m_fillsNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLFills object.
 */
XLFills::XLFills(const XMLNode& fills)
	: m_fillsNode(std::make_unique<XMLNode>(fills))
{
	// initialize XLFills entries and m_fills here
	XMLNode node = fills.first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "fill")
			m_fills.push_back(XLFill(node));
		else
			std::cerr << "WARNING: XLFills constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLFills::~XLFills()
{
	m_fills.clear(); // delete vector with all children
}

XLFills::XLFills(const XLFills& other)
	: m_fillsNode(std::make_unique<XMLNode>(*other.m_fillsNode)),
	m_fills(other.m_fills)
{}

XLFills::XLFills(XLFills&& other)
	: m_fillsNode(std::move(other.m_fillsNode)),
	m_fills(std::move(other.m_fills))
{}

XLFills& XLFills::operator=(const XLFills& other)
{
	if(&other != this) {
		*m_fillsNode = *other.m_fillsNode;
		m_fills.clear();
		m_fills = other.m_fills;
	}
	return *this;
}

/**
 * @details Returns the amount of fills held by the class
 */
size_t XLFills::count() const { return m_fills.size(); }

/**
 * @details fetch XLFill from m_fills by index
 */
XLFill XLFills::fillByIndex(XLStyleIndex index) const
{
	if(index >= m_fills.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLFills::"s + __func__ + ": attempted to access index "s + std::to_string(index) + " with count "s + std::to_string(m_fills.size()));
	}
	return m_fills.at(index);
}

/**
 * @details append a new XLFill to m_fills and m_fillsNode, based on copyFrom
 */
XLStyleIndex XLFills::create(XLFill copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the fill to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_fillsNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_fillsNode->prepend_child("fill");
	else newNode = m_fillsNode->insert_child_after("fill", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLFills::"s + __func__ + ": failed to append a new fill node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_fillsNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLFill newFill(newNode);
	if(copyFrom.m_fillNode->empty()) { // if no template is given
		// ===== Create a fill with default values
		// TODO: implement fill defaults
		// newFill.setProperty(defaultValue);
		// ...
	}
	else
		copyXMLNode(newNode, *copyFrom.m_fillNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_fills.push_back(newFill);
	appendAndSetAttribute(*m_fillsNode, "count", std::to_string(m_fills.size())); // update array count in XML
	return index;
}

/**
 * @details Constructor. Initializes an empty XLLine object
 */
XLLine::XLLine() : m_lineNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLLine object.
 */
XLLine::XLLine(const XMLNode& node) : m_lineNode(std::make_unique<XMLNode>(node)) {}

XLLine::~XLLine() = default;

XLLine::XLLine(const XLLine& other)
	: m_lineNode(std::make_unique<XMLNode>(*other.m_lineNode))
{}

XLLine& XLLine::operator=(const XLLine& other)
{
	if(&other != this)  *m_lineNode = *other.m_lineNode;
	return *this;
}

/**
 * @details Returns the line style (XLLineStyleNone if line is not set)
 */
XLLineStyle XLLine::style() const
{
	if(m_lineNode->empty())  return XLLineStyleNone;
	XMLAttribute attr = appendAndGetAttribute(*m_lineNode, "style", OpenXLSX::XLDefaultLineStyle);
	return XLLineStyleFromString(attr.value());
}

/**
 * @details check if line is used (set) or not
 */
XLLine::operator bool() const {
	return (style() != XLLineStyleNone);
}

/**
 * @details Returns the line data bar color object
 */
XLDataBarColor XLLine::color() const
{
	XMLNode color = appendAndGetNode(*m_lineNode, "color");
	if(color.empty())  return XLDataBarColor{};
	return XLDataBarColor(color);
}

// XLColor XLLine::color() const
// {
//     XMLNode colorDetails = m_lineNode->child("color");
//     if (colorDetails.empty()) return XLColor("ffffffff");
//     XMLAttribute colorRGB = colorDetails.attribute("rgb");
//     if (colorRGB.empty()) return XLColor("ffffffff");
//     return XLColor(colorRGB.value());
// }
//
// /**
//  * @details Returns the line color tint
//  */
// double XLLine::colorTint() const
// {
//     XMLNode colorDetails = m_lineNode->child("color");
//     if (not colorDetails.empty()) {
//         XMLAttribute colorTint = colorDetails.attribute("tint");
//         if (not colorTint.empty())
//             return colorTint.as_double();
//     }
//     return 0.0;
// }

/**
 * @details assemble a string summary about the fill
 */
std::string XLLine::summary() const
{
	using namespace std::literals::string_literals;
	return "line style is "s + XLLineStyleToString(style()) + ", "s + color().summary();
	// double tint = colorTint();
	// std::string tintSummary = "colorTint is "s + (tint == 0.0 ? "(none)"s : std::to_string(tint));
	// size_t tintDecimalPos = tintSummary.find_last_of('.');
	// if (tintDecimalPos != std::string::npos) tintSummary = tintSummary.substr(0, tintDecimalPos + 3); // truncate colorTint double output 2 digits after the decimal separator
	// return "line style is "s + XLLineStyleToString(style()) + ", color is "s + color().hex() + ", "s + tintSummary;
}

/**
 * @details Constructor. Initializes an empty XLBorder object
 */
XLBorder::XLBorder() : m_borderNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLBorder object.
 */
XLBorder::XLBorder(const XMLNode& node) : m_borderNode(std::make_unique<XMLNode>(node)) {}

XLBorder::~XLBorder() = default;

XLBorder::XLBorder(const XLBorder& other)
	: m_borderNode(std::make_unique<XMLNode>(*other.m_borderNode))
{}

XLBorder& XLBorder::operator=(const XLBorder& other)
{
	if(&other != this)  
		*m_borderNode = *other.m_borderNode;
	return *this;
}
/**
 * @details determines whether the diagonalUp property is set
 */
bool XLBorder::diagonalUp() const { return m_borderNode->attribute("diagonalUp").as_bool(); }
/**
 * @details determines whether the diagonalDown property is set
 */
bool XLBorder::diagonalDown() const { return m_borderNode->attribute("diagonalDown").as_bool(); }
/**
 * @details determines whether the outline property is set
 */
bool XLBorder::outline() const { return m_borderNode->attribute("outline").as_bool(); }
/**
 * @details fetch lines
 */
XLLine XLBorder::left()       const { return XLLine(m_borderNode->child("left")      ); }
XLLine XLBorder::right()      const { return XLLine(m_borderNode->child("right")     ); }
XLLine XLBorder::top()        const { return XLLine(m_borderNode->child("top")       ); }
XLLine XLBorder::bottom()     const { return XLLine(m_borderNode->child("bottom")    ); }
XLLine XLBorder::diagonal()   const { return XLLine(m_borderNode->child("diagonal")  ); }
XLLine XLBorder::vertical()   const { return XLLine(m_borderNode->child("vertical")  ); }
XLLine XLBorder::horizontal() const { return XLLine(m_borderNode->child("horizontal")); }

/**
 * @details Setter functions
 */
bool XLBorder::setDiagonalUp(bool set) { return appendAndSetAttribute(*m_borderNode, "diagonalUp",   (set ? "true" : "false")).empty() == false; }
bool XLBorder::setDiagonalDown(bool set) { return appendAndSetAttribute(*m_borderNode, "diagonalDown", (set ? "true" : "false")).empty() == false; }
bool XLBorder::setOutline(bool set) { return appendAndSetAttribute(*m_borderNode, "outline",      (set ? "true" : "false")).empty() == false; }
bool XLBorder::setLine(XLLineType lineType, XLLineStyle lineStyle, XLColor lineColor, double lineTint)
{
	XMLNode lineNode = appendAndGetNode(*m_borderNode, XLLineTypeToString(lineType), m_nodeOrder); // generate line node if not present
	// 2024-12-19: non-existing lines are added using an ordered insert to address issue #304
	bool success = (lineNode.empty() == false);
	if(success)  success = (appendAndSetAttribute(lineNode, "style", XLLineStyleToString(lineStyle)).empty() == false);// set style attribute
	XMLNode colorNode{}; // empty node
	if(success)  colorNode = appendAndGetNode(lineNode, "color");                    // generate color node if not present
	XLDataBarColor colorObject{colorNode};
	success = (colorNode.empty() == false);
	if(success)
		success = colorObject.setRgb(lineColor);
	if(success)  
		success = colorObject.setTint(lineTint);
	return success;
}

bool XLBorder::setLeft(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineLeft, lineStyle, lineColor, lineTint); }
bool XLBorder::setRight(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineRight, lineStyle, lineColor, lineTint); }
bool XLBorder::setTop(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineTop, lineStyle, lineColor, lineTint); }
bool XLBorder::setBottom(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineBottom, lineStyle, lineColor, lineTint); }
bool XLBorder::setDiagonal(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineDiagonal, lineStyle, lineColor, lineTint); }
bool XLBorder::setVertical(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineVertical, lineStyle, lineColor, lineTint); }
bool XLBorder::setHorizontal(XLLineStyle lineStyle, XLColor lineColor, double lineTint) { return setLine(XLLineHorizontal, lineStyle, lineColor, lineTint); }
/**
 * @details assemble a string summary about the fill
 */
std::string XLBorder::summary() const
{
	using namespace std::literals::string_literals;
	std::string lineInfo{};
	lineInfo += ", left: "s + left().summary() + ", right: " + right().summary()
	    + ", top: "s + top().summary() + ", bottom: " + bottom().summary()
	    + ", diagonal: "s + diagonal().summary()
	    + ", vertical: "s + vertical().summary()
	    + ", horizontal: "s + horizontal().summary();
	return "diagonalUp: "s   + (diagonalUp()   ? "true"s : "false"s)
	       + ", diagonalDown: "s + (diagonalDown() ? "true"s : "false"s)
	       + ", outline: "s      + (outline()      ? "true"s : "false"s)
	       + lineInfo;
}
//
// XLBorders, parent of XLBorder
//
/**
 * @details Constructor. Initializes an empty XLBorders object
 */
XLBorders::XLBorders() : m_bordersNode(std::make_unique<XMLNode>()) 
{
}
/**
 * @details Constructor. Initializes the member variables for the new XLBorders object.
 */
XLBorders::XLBorders(const XMLNode & borders) : m_bordersNode(std::make_unique<XMLNode>(borders))
{
	// initialize XLBorders entries and m_borders here
	XMLNode node = borders.first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "border")
			m_borders.push_back(XLBorder(node));
		else
			std::cerr << "WARNING: XLBorders constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLBorders::~XLBorders()
{
	m_borders.clear(); // delete vector with all children
}

XLBorders::XLBorders(const XLBorders& other) : m_bordersNode(std::make_unique<XMLNode>(*other.m_bordersNode)), m_borders(other.m_borders)
{
}

XLBorders::XLBorders(XLBorders&& other) : m_bordersNode(std::move(other.m_bordersNode)), m_borders(std::move(other.m_borders))
{
}

XLBorders& XLBorders::operator=(const XLBorders& other)
{
	if(&other != this) {
		*m_bordersNode = *other.m_bordersNode;
		m_borders.clear();
		m_borders = other.m_borders;
	}
	return *this;
}
/**
 * @details Returns the amount of border descriptions held by the class
 */
size_t XLBorders::count() const { return m_borders.size(); }
/**
 * @details fetch XLBorder from m_borders by index
 */
XLBorder XLBorders::borderByIndex(XLStyleIndex index) const
{
	if(index >= m_borders.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLBorders::"s + __func__ + ": attempted to access index "s + std::to_string(index) + " with count "s + std::to_string(m_borders.size()));
	}
	return m_borders.at(index);
}
/**
 * @details append a new XLBorder to m_borders and m_bordersNode, based on copyFrom
 */
XLStyleIndex XLBorders::create(XLBorder copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the border to be created
	XMLNode newNode{};           // scope declaration
	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_bordersNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  
		newNode = m_bordersNode->prepend_child("border");
	else 
		newNode = m_bordersNode->insert_child_after("border", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLBorders::"s + __func__ + ": failed to append a new border node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_bordersNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix
	XLBorder newBorder(newNode);
	if(copyFrom.m_borderNode->empty()) { // if no template is given
		// ===== Create a border with default values
		// TODO: implement border defaults
		// newBorder.setProperty(defaultValue);
		// ...
	}
	else
		copyXMLNode(newNode, *copyFrom.m_borderNode); // will use copyFrom as template, does nothing if copyFrom is empty
	m_borders.push_back(newBorder);
	appendAndSetAttribute(*m_bordersNode, "count", std::to_string(m_borders.size())); // update array count in XML
	return index;
}
/**
 * @details Constructor. Initializes an empty XLAlignment object
 */
XLAlignment::XLAlignment() : m_alignmentNode(std::make_unique<XMLNode>()) 
{
}
/**
 * @details Constructor. Initializes the member variables for the new XLAlignment object.
 */
XLAlignment::XLAlignment(const XMLNode& node) : m_alignmentNode(std::make_unique<XMLNode>(node)) 
{
}

XLAlignment::~XLAlignment() = default;

XLAlignment::XLAlignment(const XLAlignment& other) : m_alignmentNode(std::make_unique<XMLNode>(*other.m_alignmentNode))
{
}

XLAlignment& XLAlignment::operator=(const XLAlignment& other)
{
	if(&other != this)  
		*m_alignmentNode = *other.m_alignmentNode;
	return *this;
}
/**
 * @details Returns the horizontal alignment style
 */
XLAlignmentStyle XLAlignment::horizontal() const { return XLAlignmentStyleFromString(m_alignmentNode->attribute("horizontal").value()); }
/**
 * @details Returns the vertical alignment style
 */
XLAlignmentStyle XLAlignment::vertical() const { return XLAlignmentStyleFromString(m_alignmentNode->attribute("vertical").value()); }
uint16 XLAlignment::textRotation() const { return static_cast<uint16>(m_alignmentNode->attribute("textRotation").as_uint()); }
/**
 * @details check if text wrapping is enabled
 */
bool XLAlignment::wrapText() const { return m_alignmentNode->attribute("wrapText").as_bool(); }
/**
 * @details Returns the indent setting
 */
uint32 XLAlignment::indent() const { return m_alignmentNode->attribute("indent").as_uint(); }

/**
 * @details Returns the relative indent setting
 */
int32_t XLAlignment::relativeIndent() const { return m_alignmentNode->attribute("relativeIndent").as_int(); }

/**
 * @details check if justification of last line is enabled
 */
bool XLAlignment::justifyLastLine() const { return m_alignmentNode->attribute("justifyLastLine").as_bool(); }

/**
 * @details check if shrink to fit is enabled
 */
bool XLAlignment::shrinkToFit() const { return m_alignmentNode->attribute("shrinkToFit").as_bool(); }

/**
 * @details Returns the reading order setting
 */
uint32 XLAlignment::readingOrder() const { return m_alignmentNode->attribute("readingOrder").as_uint(); }

/**
 * @details Setter functions
 */
bool XLAlignment::setHorizontal(XLAlignmentStyle newStyle) {
	return appendAndSetAttribute(*m_alignmentNode, "horizontal",      XLAlignmentStyleToString(newStyle).c_str()).empty() == false;
}
bool XLAlignment::setVertical(XLAlignmentStyle newStyle) {
	return appendAndSetAttribute(*m_alignmentNode, "vertical",        XLAlignmentStyleToString(newStyle).c_str()).empty() == false;
}
bool XLAlignment::setTextRotation(uint16 newRotation)      {
	return appendAndSetAttribute(*m_alignmentNode, "textRotation",    std::to_string(newRotation)               ).empty() == false;
}
bool XLAlignment::setWrapText(bool set)                  {
	return appendAndSetAttribute(*m_alignmentNode, "wrapText",        (set ? "true" : "false")                  ).empty() == false;
}
bool XLAlignment::setIndent(uint32 newIndent)        {
	return appendAndSetAttribute(*m_alignmentNode, "indent",          std::to_string(newIndent)                 ).empty() == false;
}
bool XLAlignment::setRelativeIndent(int32_t newRelativeIndent) {
	return appendAndSetAttribute(*m_alignmentNode, "relativeIndent",  std::to_string(newRelativeIndent)         ).empty() == false;
}
bool XLAlignment::setJustifyLastLine(bool set)                  {
	return appendAndSetAttribute(*m_alignmentNode, "justifyLastLine", (set ? "true" : "false")                  ).empty() == false;
}
bool XLAlignment::setShrinkToFit(bool set)                  {
	return appendAndSetAttribute(*m_alignmentNode, "shrinkToFit",     (set ? "true" : "false")                  ).empty() == false;
}
bool XLAlignment::setReadingOrder(uint32 newReadingOrder)  {
	return appendAndSetAttribute(*m_alignmentNode, "readingOrder",    std::to_string(newReadingOrder)           ).empty() == false;
}

/**
 * @details assemble a string summary about the fill
 */
std::string XLAlignment::summary() const
{
	using namespace std::literals::string_literals;
	return "alignment horizontal="s + XLAlignmentStyleToString(horizontal()) + ", vertical="s + XLAlignmentStyleToString(vertical())
	       + ", textRotation="s + std::to_string(textRotation())
	       + ", wrapText="s + (wrapText() ? "true" : "false")
	       + ", indent="s + std::to_string(indent())
	       + ", relativeIndent="s + std::to_string(relativeIndent())
	       + ", justifyLastLine="s + (justifyLastLine() ? "true" : "false")
	       + ", shrinkToFit="s + (shrinkToFit() ? "true" : "false")
	       + ", readingOrder="s + XLReadingOrderToString(readingOrder());
}

/**
 * @details Constructor. Initializes an empty XLCellFormat object
 */
XLCellFormat::XLCellFormat() : m_cellFormatNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellFormat object.
 */
XLCellFormat::XLCellFormat(const XMLNode& node, bool permitXfId)
	: m_cellFormatNode(std::make_unique<XMLNode>(node)),
	m_permitXfId(permitXfId)
{}

XLCellFormat::~XLCellFormat() = default;

XLCellFormat::XLCellFormat(const XLCellFormat& other)
	: m_cellFormatNode(std::make_unique<XMLNode>(*other.m_cellFormatNode)),
	m_permitXfId(other.m_permitXfId)
{}

XLCellFormat& XLCellFormat::operator=(const XLCellFormat& other)
{
	if(&other != this) {
		*m_cellFormatNode = *other.m_cellFormatNode;
		m_permitXfId = other.m_permitXfId;
	}
	return *this;
}

/**
 * @details determines the numberFormatId
 * @note returns XLInvalidUInt32 if attribute is not defined / set / empty
 */
uint32 XLCellFormat::numberFormatId() const { return m_cellFormatNode->attribute("numFmtId").as_uint(XLInvalidUInt32); }

/**
 * @details determines the fontIndex
 * @note returns XLInvalidStyleIndex if attribute is not defined / set / empty
 */
XLStyleIndex XLCellFormat::fontIndex() const { return m_cellFormatNode->attribute("fontId").as_uint(XLInvalidStyleIndex); }

/**
 * @details determines the fillIndex
 * @note returns XLInvalidStyleIndex if attribute is not defined / set / empty
 */
XLStyleIndex XLCellFormat::fillIndex() const { return m_cellFormatNode->attribute("fillId").as_uint(XLInvalidStyleIndex); }

/**
 * @details determines the borderIndex
 * @note returns XLInvalidStyleIndex if attribute is not defined / set / empty
 */
XLStyleIndex XLCellFormat::borderIndex() const { return m_cellFormatNode->attribute("borderId").as_uint(XLInvalidStyleIndex); }

/**
 * @details Returns the xfId value
 */
XLStyleIndex XLCellFormat::xfId() const
{
	if(m_permitXfId)  return m_cellFormatNode->attribute("xfId").as_uint(XLInvalidStyleIndex);
	throw XLException("XLCellFormat::xfId not permitted when m_permitXfId is false");
}

/**
 * @details determine the applyNumberFormat,applyFont,applyFill,applyBorder,applyAlignment,applyProtection status
 */
bool XLCellFormat::applyNumberFormat() const { return m_cellFormatNode->attribute("applyNumberFormat").as_bool(); }
bool XLCellFormat::applyFont()         const { return m_cellFormatNode->attribute("applyFont").as_bool();         }
bool XLCellFormat::applyFill()         const { return m_cellFormatNode->attribute("applyFill").as_bool();         }
bool XLCellFormat::applyBorder()       const { return m_cellFormatNode->attribute("applyBorder").as_bool();       }
bool XLCellFormat::applyAlignment()    const { return m_cellFormatNode->attribute("applyAlignment").as_bool();    }
bool XLCellFormat::applyProtection()   const { return m_cellFormatNode->attribute("applyProtection").as_bool();   }

/**
 * @details determine the quotePrefix, pivotButton status
 */
bool XLCellFormat::quotePrefix()       const { return m_cellFormatNode->attribute("quotePrefix").as_bool();       }
bool XLCellFormat::pivotButton()       const { return m_cellFormatNode->attribute("pivotButton").as_bool();       }

/**
 * @details determine the protection:locked,protection:hidden status
 */
bool XLCellFormat::locked() const { return m_cellFormatNode->child("protection").attribute("locked").as_bool(); }
bool XLCellFormat::hidden() const { return m_cellFormatNode->child("protection").attribute("hidden").as_bool(); }

/**
 * @details fetch alignment object
 */
XLAlignment XLCellFormat::alignment(bool createIfMissing) const
{
	XMLNode nodeAlignment = m_cellFormatNode->child("alignment");
	if(nodeAlignment.empty() && createIfMissing)
		nodeAlignment = appendAndGetNode(*m_cellFormatNode, "alignment", m_nodeOrder); // 2024-12-19: ordered insert to address issue #305
	return XLAlignment(nodeAlignment);
}

/**
 * @details Setter functions
 */
bool XLCellFormat::setNumberFormatId(uint32 newNumFmtId)        { return appendAndSetAttribute(*m_cellFormatNode, "numFmtId", std::to_string(newNumFmtId)).empty() == false; }
bool XLCellFormat::setFontIndex(XLStyleIndex newXfIndex)     { return appendAndSetAttribute(*m_cellFormatNode, "fontId",   std::to_string(newXfIndex)).empty() == false; }
bool XLCellFormat::setFillIndex(XLStyleIndex newFillIndex)   { return appendAndSetAttribute(*m_cellFormatNode, "fillId",   std::to_string(newFillIndex)).empty() == false; }
bool XLCellFormat::setBorderIndex(XLStyleIndex newBorderIndex) { return appendAndSetAttribute(*m_cellFormatNode, "borderId", std::to_string(newBorderIndex)).empty() == false; }
bool XLCellFormat::setXfId(XLStyleIndex newXfId)
{
	if(m_permitXfId)                                                   return appendAndSetAttribute(*m_cellFormatNode, "xfId",     std::to_string(newXfId)).empty() == false;
	throw XLException("XLCellFormat::setXfId not permitted when m_permitXfId is false");
}

bool XLCellFormat::setApplyNumberFormat(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "applyNumberFormat",           (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setApplyFont(bool set)            { return appendAndSetAttribute(*m_cellFormatNode, "applyFont",                   (set ? "true" : "false")).empty() == false; }
bool XLCellFormat::setApplyFill(bool set)            { return appendAndSetAttribute(*m_cellFormatNode, "applyFill",                   (set ? "true" : "false")).empty() == false; }
bool XLCellFormat::setApplyBorder(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "applyBorder",                 (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setApplyAlignment(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "applyAlignment",              (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setApplyProtection(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "applyProtection",             (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setQuotePrefix(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "quotePrefix",                 (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setPivotButton(bool set)            {
	return appendAndSetAttribute(*m_cellFormatNode, "pivotButton",                 (set ? "true" : "false")).empty() == false;
}
bool XLCellFormat::setLocked(bool set)
{
	return appendAndSetNodeAttribute(*m_cellFormatNode, "protection", "locked", (set ? "true" : "false"),
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;               // 2024-12-19: ordered insert to address issue #305
}

bool XLCellFormat::setHidden(bool set)
{
	return appendAndSetNodeAttribute(*m_cellFormatNode, "protection", "hidden", (set ? "true" : "false"),
	           /**/ XLKeepAttributes, m_nodeOrder).empty() == false;               // 2024-12-19: ordered insert to address issue #305
}

/**
 * @brief Unsupported setter function
 */
bool XLCellFormat::setExtLst(XLUnsupportedElement const& newExtLst) { OpenXLSX::ignore(newExtLst); return false; }

/**
 * @details assemble a string summary about the cell format
 */
std::string XLCellFormat::summary() const
{
	using namespace std::literals::string_literals;
	return "numberFormatId="s + std::to_string(numberFormatId())
	       + ", fontIndex="s + std::to_string(fontIndex())
	       + ", fillIndex="s + std::to_string(fillIndex())
	       + ", borderIndex="s + std::to_string(borderIndex())
	       + ( m_permitXfId ? (", xfId: "s + std::to_string(xfId())) : ""s)
	       + ", applyNumberFormat: "s + (applyNumberFormat() ? "true"s : "false"s)
	       + ", applyFont: "s + (applyFont() ? "true"s : "false"s)
	       + ", applyFill: "s + (applyFill() ? "true"s : "false"s)
	       + ", applyBorder: "s + (applyBorder() ? "true"s : "false"s)
	       + ", applyAlignment: "s + (applyAlignment() ? "true"s : "false"s)
	       + ", applyProtection: "s + (applyProtection() ? "true"s : "false"s)
	       + ", quotePrefix: "s + (quotePrefix() ? "true"s : "false"s)
	       + ", pivotButton: "s + (pivotButton() ? "true"s : "false"s)
	       + ", locked: "s + (locked() ? "true"s : "false"s)
	       + ", hidden: "s + (hidden() ? "true"s : "false"s)
	       + ", " + alignment().summary();
}
//
// XLCellFormats, one parent of XLCellFormat (the other being XLCellFormats)
//
/**
 * @details Constructor. Initializes an empty XLCellFormats object
 */
XLCellFormats::XLCellFormats() : m_cellFormatsNode(std::make_unique<XMLNode>()) 
{
}
/**
 * @details Constructor. Initializes the member variables for the new XLCellFormats object.
 */
XLCellFormats::XLCellFormats(const XMLNode& cellStyleFormats, bool permitXfId) : 
	m_cellFormatsNode(std::make_unique<XMLNode>(cellStyleFormats)), m_permitXfId(permitXfId)
{
	// initialize XLCellFormats entries and m_cellFormats here
	XMLNode node = cellStyleFormats.first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "xf")
			m_cellFormats.push_back(XLCellFormat(node, m_permitXfId));
		else
			std::cerr << "WARNING: XLCellFormats constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLCellFormats::~XLCellFormats()
{
	m_cellFormats.clear(); // delete vector with all children
}

XLCellFormats::XLCellFormats(const XLCellFormats& other)
	: m_cellFormatsNode(std::make_unique<XMLNode>(*other.m_cellFormatsNode)),
	m_cellFormats(other.m_cellFormats),
	m_permitXfId(other.m_permitXfId)
{}

XLCellFormats::XLCellFormats(XLCellFormats&& other) : m_cellFormatsNode(std::move(other.m_cellFormatsNode)),
	m_cellFormats(std::move(other.m_cellFormats)), m_permitXfId(other.m_permitXfId)
{
}

XLCellFormats& XLCellFormats::operator=(const XLCellFormats& other)
{
	if(&other != this) {
		*m_cellFormatsNode = *other.m_cellFormatsNode;
		m_cellFormats.clear();
		m_cellFormats = other.m_cellFormats;
		m_permitXfId = other.m_permitXfId;
	}
	return *this;
}

/**
 * @details Returns the amount of cell format descriptions held by the class
 */
size_t XLCellFormats::count() const { return m_cellFormats.size(); }

/**
 * @details fetch XLCellFormat from m_cellFormats by index
 */
XLCellFormat XLCellFormats::cellFormatByIndex(XLStyleIndex index) const
{
	if(index >= m_cellFormats.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCellFormats::"s + __func__ + ": attempted to access index "s + std::to_string(index)
			  + " with count "s + std::to_string(m_cellFormats.size()));
	}
	return m_cellFormats.at(index);
}

/**
 * @details append a new XLCellFormat to m_cellFormats and m_cellFormatsNode, based on copyFrom
 */
XLStyleIndex XLCellFormats::create(XLCellFormat copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the cell format to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_cellFormatsNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_cellFormatsNode->prepend_child("xf");
	else newNode = m_cellFormatsNode->insert_child_after("xf", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCellFormats::"s + __func__ + ": failed to append a new xf node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_cellFormatsNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLCellFormat newCellFormat(newNode, m_permitXfId);
	if(copyFrom.m_cellFormatNode->empty()) { // if no template is given
		// ===== Create a cell format with default values
		// default index 0 for other style elements should protect from exceptions
		newCellFormat.setNumberFormatId(0);
		newCellFormat.setFontIndex(0);
		newCellFormat.setFillIndex(0);
		newCellFormat.setBorderIndex(0);
		newCellFormat.setXfId(0);
		newCellFormat.setApplyNumberFormat(false);
		newCellFormat.setApplyFont(false);
		newCellFormat.setApplyFill(false);
		newCellFormat.setApplyBorder(false);
		newCellFormat.setApplyAlignment(false);
		newCellFormat.setApplyProtection(false);
		newCellFormat.setQuotePrefix(false);
		newCellFormat.setPivotButton(false);
		newCellFormat.setLocked(false);
		newCellFormat.setHidden(false);
		// Unsupported setter
		newCellFormat.setExtLst(XLUnsupportedElement {});
	}
	else
		copyXMLNode(newNode, *copyFrom.m_cellFormatNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_cellFormats.push_back(newCellFormat);
	appendAndSetAttribute(*m_cellFormatsNode, "count", std::to_string(m_cellFormats.size())); // update array count in XML
	return index;
}

/**
 * @details Constructor. Initializes an empty XLCellStyle object
 */
XLCellStyle::XLCellStyle() : m_cellStyleNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellStyle object.
 */
XLCellStyle::XLCellStyle(const XMLNode& node) : m_cellStyleNode(std::make_unique<XMLNode>(node)) {}

XLCellStyle::~XLCellStyle() = default;

XLCellStyle::XLCellStyle(const XLCellStyle& other)
	: m_cellStyleNode(std::make_unique<XMLNode>(*other.m_cellStyleNode))
{}

XLCellStyle& XLCellStyle::operator=(const XLCellStyle& other)
{
	if(&other != this)  *m_cellStyleNode = *other.m_cellStyleNode;
	return *this;
}

/**
 * @details Returns the style empty status
 */
bool XLCellStyle::empty() const { return m_cellStyleNode->empty(); }

/**
 * @details Getter functions
 */
std::string XLCellStyle::name() const { return m_cellStyleNode->attribute("name").value();                      }
XLStyleIndex XLCellStyle::xfId() const { return m_cellStyleNode->attribute("xfId").as_uint(XLInvalidStyleIndex); }
uint32 XLCellStyle::builtinId() const { return m_cellStyleNode->attribute("builtinId").as_uint(XLInvalidUInt32);     }
uint32 XLCellStyle::outlineStyle() const { return m_cellStyleNode->attribute("iLevel").as_uint(XLInvalidUInt32);     }
bool XLCellStyle::hidden() const { return m_cellStyleNode->attribute("hidden").as_bool();                    }
bool XLCellStyle::customBuiltin() const { return m_cellStyleNode->attribute("customBuiltin").as_bool();                    }

/**
 * @details Setter functions
 */
bool XLCellStyle::setName(std::string newName)      { return appendAndSetAttribute(*m_cellStyleNode, "name",          newName).empty() == false;                         }
bool XLCellStyle::setXfId(XLStyleIndex newXfId)     { return appendAndSetAttribute(*m_cellStyleNode, "xfId",          std::to_string(newXfId)).empty() == false;         }
bool XLCellStyle::setBuiltinId(uint32 newBuiltinId)    { return appendAndSetAttribute(*m_cellStyleNode, "builtinId",     std::to_string(newBuiltinId)).empty() == false;    }
bool XLCellStyle::setOutlineStyle(uint32 newOutlineStyle) { return appendAndSetAttribute(*m_cellStyleNode, "iLevel",        std::to_string(newOutlineStyle)).empty() == false; }
bool XLCellStyle::setHidden(bool set)                 { return appendAndSetAttribute(*m_cellStyleNode, "hidden",        (set ? "true" : "false")).empty() == false;        }
bool XLCellStyle::setCustomBuiltin(bool set)                 { return appendAndSetAttribute(*m_cellStyleNode, "customBuiltin", (set ? "true" : "false")).empty() == false;        }

/**
 * @brief Unsupported setter function
 */
bool XLCellStyle::setExtLst(XLUnsupportedElement const& newExtLst) { OpenXLSX::ignore(newExtLst); return false; }

/**
 * @details assemble a string summary about the cell style
 */
std::string XLCellStyle::summary() const
{
	using namespace std::literals::string_literals;
	uint32 iLevel = outlineStyle();
	return "name="s + name()
	       + ", xfId="s + std::to_string(xfId())
	       + ", builtinId="s + std::to_string(builtinId())
	       + (iLevel != XLInvalidUInt32 ? ", iLevel="s + std::to_string(outlineStyle()) : ""s)
	       + (hidden() ? ", hidden=true"s : ""s)
	       + (customBuiltin() ? ", customBuiltin=true"s : ""s);
}

// ===== XLCellStyles, parent of XLCellStyle

/**
 * @details Constructor. Initializes an empty XLCellStyles object
 */
XLCellStyles::XLCellStyles() : m_cellStylesNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellStyles object.
 */
XLCellStyles::XLCellStyles(const XMLNode& cellStyles)
	: m_cellStylesNode(std::make_unique<XMLNode>(cellStyles))
{
	// initialize XLCellStyles entries and m_cellStyles here
	XMLNode node = cellStyles.first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "cellStyle")
			m_cellStyles.push_back(XLCellStyle(node));
		else
			std::cerr << "WARNING: XLCellStyles constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLCellStyles::~XLCellStyles()
{
	m_cellStyles.clear(); // delete vector with all children
}

XLCellStyles::XLCellStyles(const XLCellStyles& other)
	: m_cellStylesNode(std::make_unique<XMLNode>(*other.m_cellStylesNode)),
	m_cellStyles(other.m_cellStyles)
{}

XLCellStyles::XLCellStyles(XLCellStyles&& other)
	: m_cellStylesNode(std::move(other.m_cellStylesNode)),
	m_cellStyles(std::move(other.m_cellStyles))
{}

XLCellStyles& XLCellStyles::operator=(const XLCellStyles& other)
{
	if(&other != this) {
		*m_cellStylesNode = *other.m_cellStylesNode;
		m_cellStyles.clear();
		m_cellStyles = other.m_cellStyles;
	}
	return *this;
}

/**
 * @details Returns the amount of numberFormats held by the class
 */
size_t XLCellStyles::count() const { return m_cellStyles.size(); }

/**
 * @details fetch XLCellStyle from m_cellStyles by index
 */
XLCellStyle XLCellStyles::cellStyleByIndex(XLStyleIndex index) const
{
	if(index >= m_cellStyles.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCellStyles::"s + __func__ + ": attempted to access index "s + std::to_string(index)
			  + " with count "s + std::to_string(m_cellStyles.size()));
	}
	return m_cellStyles.at(index);
}

/**
 * @details append a new XLCellStyle to m_cellStyles and m_cellStyleNode, based on copyFrom
 */
XLStyleIndex XLCellStyles::create(XLCellStyle copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the cell style to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastStyle = m_cellStylesNode->last_child_of_type(pugi::node_element);
	if(lastStyle.empty())  newNode = m_cellStylesNode->prepend_child("cellStyle");
	else newNode = m_cellStylesNode->insert_child_after("cellStyle", lastStyle);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLCellStyles::"s + __func__ + ": failed to append a new cellStyle node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_cellStylesNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLCellStyle newCellStyle(newNode);
	if(copyFrom.m_cellStyleNode->empty()) { // if no template is given
		// ===== Create a cell style with default values
		// TODO: implement cell style defaults
		// newCellStyle.setProperty(defaultValue);
		// ...
	}
	else
		copyXMLNode(newNode, *copyFrom.m_cellStyleNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_cellStyles.push_back(newCellStyle);
	appendAndSetAttribute(*m_cellStylesNode, "count", std::to_string(m_cellStyles.size())); // update array count in XML
	return index;
}

/**
 * @details Constructor. Initializes an empty XLDiffCellFormat object
 */
XLDiffCellFormat::XLDiffCellFormat() : m_diffCellFormatNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLDiffCellFormat object.
 */
XLDiffCellFormat::XLDiffCellFormat(const XMLNode& node) : m_diffCellFormatNode(std::make_unique<XMLNode>(node)) {}

XLDiffCellFormat::~XLDiffCellFormat() = default;

XLDiffCellFormat::XLDiffCellFormat(const XLDiffCellFormat& other)
	: m_diffCellFormatNode(std::make_unique<XMLNode>(*other.m_diffCellFormatNode))
{}

XLDiffCellFormat& XLDiffCellFormat::operator=(const XLDiffCellFormat& other)
{
	if(&other != this)  *m_diffCellFormatNode = *other.m_diffCellFormatNode;
	return *this;
}

/**
 * @details Returns the differential cell format empty status
 */
bool XLDiffCellFormat::empty() const { return m_diffCellFormatNode->empty(); }

/**
 * @details Getter functions
 */
XLFont XLDiffCellFormat::font() const
{
	XMLNode fontNode = appendAndGetNode(*m_diffCellFormatNode, "font");
	if(fontNode.empty())  return XLFont{};
	return XLFont(fontNode);
}

XLNumberFormat XLDiffCellFormat::numFmt() const
{
	XMLNode numFmtNode = appendAndGetNode(*m_diffCellFormatNode, "numFmt");
	if(numFmtNode.empty())  
		return XLNumberFormat{};
	return XLNumberFormat(numFmtNode);
}

XLFill XLDiffCellFormat::fill() const
{
	XMLNode fillNode = appendAndGetNode(*m_diffCellFormatNode, "fill");
	if(fillNode.empty())  
		return XLFill{};
	return XLFill(fillNode);
}

XLAlignment XLDiffCellFormat::alignment() const
{
	XMLNode alignmentNode = appendAndGetNode(*m_diffCellFormatNode, "alignment");
	if(alignmentNode.empty())  
		return XLAlignment{};
	return XLAlignment(alignmentNode);
}

XLBorder XLDiffCellFormat::border() const
{
	XMLNode borderNode = appendAndGetNode(*m_diffCellFormatNode, "border");
	if(borderNode.empty())  
		return XLBorder{};
	return XLBorder(borderNode);
}

/**
 * @details Setter functions
 */
// bool XLDiffCellFormat::setName         (std::string newName)      { return appendAndSetAttribute(*m_diffCellFormatNode, "name", newName).empty() == false; }

/**
 * @brief Unsupported setter function
 */
bool XLDiffCellFormat::setExtLst(XLUnsupportedElement const& newExtLst) { OpenXLSX::ignore(newExtLst); return false; }

/**
 * @details assemble a string summary about the differential cell format
 */
std::string XLDiffCellFormat::summary() const
{
	using namespace std::literals::string_literals;
	return ""; // TODO: write this
}

// ===== XLDiffCellFormats, parent of XLDiffCellFormat

/**
 * @details Constructor. Initializes an empty XLCellStyles object
 */
XLDiffCellFormats::XLDiffCellFormats() : m_diffCellFormatsNode(std::make_unique<XMLNode>()) {}

/**
 * @details Constructor. Initializes the member variables for the new XLCellStyles object.
 */
XLDiffCellFormats::XLDiffCellFormats(const XMLNode& diffCellFormats)
	: m_diffCellFormatsNode(std::make_unique<XMLNode>(diffCellFormats))
{
	// initialize XLCellStyles entries and m_cellStyles here
	XMLNode node = m_diffCellFormatsNode->first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		std::string nodeName = node.name();
		if(nodeName == "dxf")
			m_diffCellFormats.push_back(XLDiffCellFormat(node));
		else
			std::cerr << "WARNING: XLDiffCellFormats constructor: unknown subnode " << nodeName << std::endl;
		node = node.next_sibling_of_type(pugi::node_element);
	}
}

XLDiffCellFormats::~XLDiffCellFormats()
{
	m_diffCellFormats.clear(); // delete vector with all children
}

XLDiffCellFormats::XLDiffCellFormats(const XLDiffCellFormats& other)
	: m_diffCellFormatsNode(std::make_unique<XMLNode>(*other.m_diffCellFormatsNode)),
	m_diffCellFormats(other.m_diffCellFormats)
{}

XLDiffCellFormats::XLDiffCellFormats(XLDiffCellFormats&& other)
	: m_diffCellFormatsNode(std::move(other.m_diffCellFormatsNode)),
	m_diffCellFormats(std::move(other.m_diffCellFormats))
{}

XLDiffCellFormats& XLDiffCellFormats::operator=(const XLDiffCellFormats& other)
{
	if(&other != this) {
		*m_diffCellFormatsNode = *other.m_diffCellFormatsNode;
		m_diffCellFormats.clear();
		m_diffCellFormats = other.m_diffCellFormats;
	}
	return *this;
}

/**
 * @details Returns the amount of differential cell formats held by the class
 */
size_t XLDiffCellFormats::count() const { return m_diffCellFormats.size(); }

/**
 * @details fetch XLDiffCellFormat from m_diffCellFormats by index
 */
XLDiffCellFormat XLDiffCellFormats::diffCellFormatByIndex(XLStyleIndex index) const
{
	if(index >= m_diffCellFormats.size()) {
		using namespace std::literals::string_literals;
		throw XLException("XLDiffCellFormats::"s + __func__ + ": attempted to access index "s + std::to_string(index)
			  + " with count "s + std::to_string(m_diffCellFormats.size()));
	}
	return m_diffCellFormats.at(index);
}

/**
 * @details append a new XLDiffCellFormat to m_diffCellFormats and m_diffCellFormatsNode, based on copyFrom
 */
XLStyleIndex XLDiffCellFormats::create(XLDiffCellFormat copyFrom, std::string styleEntriesPrefix)
{
	XLStyleIndex index = count(); // index for the cell style to be created
	XMLNode newNode{};           // scope declaration

	// ===== Append new node prior to final whitespaces, if any
	XMLNode lastDiffCellFormat = m_diffCellFormatsNode->last_child_of_type(pugi::node_element);
	if(lastDiffCellFormat.empty())  newNode = m_diffCellFormatsNode->prepend_child("dxf");
	else newNode = m_diffCellFormatsNode->insert_child_after("dxf", lastDiffCellFormat);
	if(newNode.empty()) {
		using namespace std::literals::string_literals;
		throw XLException("XLDiffCellFormats::"s + __func__ + ": failed to append a new dxf node"s);
	}
	if(styleEntriesPrefix.length() > 0) // if a whitespace prefix is configured
		m_diffCellFormatsNode->insert_child_before(pugi::node_pcdata, newNode).set_value(styleEntriesPrefix.c_str()); // prefix the new node with styleEntriesPrefix

	XLDiffCellFormat newDiffCellFormat(newNode);
	if(copyFrom.m_diffCellFormatNode->empty()) { // if no template is given
		// no-op: differential cell format entries have NO default values
	}
	else
		copyXMLNode(newNode, *copyFrom.m_diffCellFormatNode); // will use copyFrom as template, does nothing if copyFrom is empty

	m_diffCellFormats.push_back(newDiffCellFormat);
	appendAndSetAttribute(*m_diffCellFormatsNode, "count", std::to_string(m_diffCellFormats.size())); // update array count in XML
	return index;
}
//
// XLStyles, master class
//
XLStyles::XLStyles()  // TBD if defaulting this constructor again would reintroduce issue #310
{
} 
/**
 * @details Creates an XLStyles object, which will initialize from the given xmlData
 */
XLStyles::XLStyles(XLXmlData* xmlData, bool suppressWarnings, std::string stylesPrefix) : XLXmlFile(xmlData), m_suppressWarnings(suppressWarnings)
{
	OXlXmlDoc & doc = xmlDocument();
	if(doc.document_element().empty()) // handle a bad (no document element) xl/styles.xml
		doc.load_string(
			"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
			"<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">\n"
			"</styleSheet>",
			pugi_parse_settings);
	XMLNode node = doc.document_element().first_child_of_type(pugi::node_element);
	while(!node.empty()) {
		XLStylesEntryType e = XLStylesEntryTypeFromString(node.name());
		// std::cout << "node.name() is " << node.name() << ", resulting XLStylesEntryType is " << std::to_string(e) << std::endl;
		switch(e) {
			case XLStylesNumberFormats:
			    // std::cout << "found XLStylesNumberFormats, node name is " << node.name() << std::endl;
			    m_numberFormats = std::make_unique<XLNumberFormats>(node);
			    break;
			case XLStylesFonts:
			    // std::cout << "found XLStylesFonts, node name is " << node.name() << std::endl;
			    m_fonts = std::make_unique<XLFonts>(node);
			    break;
			case XLStylesFills:
			    // std::cout << "found XLStylesFills, node name is " << node.name() << std::endl;
			    m_fills = std::make_unique<XLFills>(node);
			    break;
			case XLStylesBorders:
			    // std::cout << "found XLStylesBorders, node name is " << node.name() << std::endl;
			    m_borders = std::make_unique<XLBorders>(node);
			    break;
			case XLStylesCellStyleFormats:
			    // std::cout << "found XLStylesCellStyleFormats, node name is " << node.name() << std::endl;
			    m_cellStyleFormats = std::make_unique<XLCellFormats>(node);
			    break;
			case XLStylesCellFormats:
			    // std::cout << "found XLStylesCellFormats, node name is " << node.name() << std::endl;
			    m_cellFormats = std::make_unique<XLCellFormats>(node, XLPermitXfID);
			    break;
			case XLStylesCellStyles:
			    // std::cout << "found XLStylesCellStyles, node name is " << node.name() << std::endl;
			    m_cellStyles = std::make_unique<XLCellStyles>(node);
			    break;
			case XLStylesDiffCellFormats:
			    // std::cout << "found XLStylesDiffCellFormats, node name is " << node.name() << std::endl;
			    m_diffCellFormats = std::make_unique<XLDiffCellFormats>(node);
			    break;
			case XLStylesColors:      [[fallthrough]];
			case XLStylesTableStyles: [[fallthrough]];
			case XLStylesExtLst:
			    if(!m_suppressWarnings)
				    std::cout << "XLStyles: Ignoring currently unsupported <" << XLStylesEntryTypeToString(e) + "> node" << std::endl;
			    break;
			case XLStylesInvalid: [[fallthrough]];
			default:
			    if(!m_suppressWarnings)
				    std::cerr << "WARNING: XLStyles constructor: invalid styles subnode \"" << node.name() << "\"" << std::endl;
			    break;
		}
		node = node.next_sibling_of_type(pugi::node_element);
	}

	// ===== Fallbacks: create root style nodes (in reverse order, using prepend_child)
	if(!m_diffCellFormats) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesDiffCellFormats).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_diffCellFormats = std::make_unique<XLDiffCellFormats>(node);
	}
	if(!m_cellStyles) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesCellStyles).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_cellStyles = std::make_unique<XLCellStyles>(node);
	}
	if(!m_cellFormats) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesCellFormats).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_cellFormats = std::make_unique<XLCellFormats>(node, XLPermitXfID);
	}
	if(m_cellFormats->count() == 0) { // if the cell formats array is empty
		// ===== Create a default empty cell format with ID 0 (== XLDefaultCellFormat) because when XLDefaultCellFormat
		//        is assigned to an XLRow, the intention is interpreted as "set the cell back to default formatting",
		//        which does not trigger setting the attribute customFormat="true".
		//       To avoid confusing the user when the first style created does not work for rows, and setting a cell's
		//        format back to XLDefaultCellFormat would cause an actual formatting (if assigned format ID 0), this
		//        initial entry with no properties is created and henceforth ignored
		m_cellFormats->create();
	}

	if(!m_cellStyleFormats) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesCellStyleFormats).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_cellStyleFormats = std::make_unique<XLCellFormats>(node);
	}
	if(!m_borders) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesBorders).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_borders = std::make_unique<XLBorders>(node);
	}
	if(!m_fills) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesFills).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_fills = std::make_unique<XLFills>(node);
	}
	if(!m_fonts) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesFonts).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_fonts = std::make_unique<XLFonts>(node);
	}
	if(!m_numberFormats) {
		node = doc.document_element().prepend_child(XLStylesEntryTypeToString(XLStylesNumberFormats).c_str());
		wrapNode(doc.document_element(), node, stylesPrefix);
		m_numberFormats = std::make_unique<XLNumberFormats>(node);
	}
}

XLStyles::~XLStyles()
{
}

XLStyles::XLStyles(XLStyles&& other) noexcept : XLXmlFile(other), m_suppressWarnings(other.m_suppressWarnings),
	m_numberFormats(std::move(other.m_numberFormats)),
	m_fonts(std::move(other.m_fonts)),
	m_fills(std::move(other.m_fills)),
	m_borders(std::move(other.m_borders)),
	m_cellStyleFormats(std::move(other.m_cellStyleFormats)),
	m_cellFormats(std::move(other.m_cellFormats)),
	m_cellStyles(std::move(other.m_cellStyles)),
	m_diffCellFormats(std::move(other.m_diffCellFormats))
{
}

XLStyles::XLStyles(const XLStyles& other) : XLXmlFile(other), m_suppressWarnings(other.m_suppressWarnings),
	m_numberFormats(std::make_unique<XLNumberFormats  >(*other.m_numberFormats)),
	m_fonts(std::make_unique<XLFonts>(*other.m_fonts)),
	m_fills(std::make_unique<XLFills>(*other.m_fills)           ),
	m_borders(std::make_unique<XLBorders>(*other.m_borders)         ),
	m_cellStyleFormats(std::make_unique<XLCellFormats>(*other.m_cellStyleFormats)),
	m_cellFormats(std::make_unique<XLCellFormats>(*other.m_cellFormats)     ),
	m_cellStyles(std::make_unique<XLCellStyles>(*other.m_cellStyles)      ),
	m_diffCellFormats(std::make_unique<XLDiffCellFormats>(*other.m_diffCellFormats) )
{
}

/**
 * @details move-assign an XLStyles object
 */
XLStyles& XLStyles::operator=(XLStyles&& other) noexcept
{
	if(&other != this) {
		XLXmlFile::operator=(std::move(other));
		m_suppressWarnings = other.m_suppressWarnings;
		m_numberFormats    = std::move(other.m_numberFormats);
		m_fonts            = std::move(other.m_fonts);
		m_fills            = std::move(other.m_fills);
		m_borders          = std::move(other.m_borders);
		m_cellStyleFormats = std::move(other.m_cellStyleFormats);
		m_cellFormats      = std::move(other.m_cellFormats);
		m_cellStyles       = std::move(other.m_cellStyles);
		m_diffCellFormats  = std::move(other.m_diffCellFormats);
	}
	return *this;
}

/**
 * @details copy-assign an XLStyles object
 */
XLStyles& XLStyles::operator=(const XLStyles& other)
{
	if(&other != this) {
		XLStyles temp = other; // copy-construct
		*this = std::move(temp); // move-assign & invalidate temp
	}
	return *this;
}

/**
 * @details return a handle to the underlying number formats
 */
XLNumberFormats& XLStyles::numberFormats() const { return *m_numberFormats; }

/**
 * @details return a handle to the underlying fonts
 */
XLFonts& XLStyles::fonts() const { return *m_fonts; }

/**
 * @details return a handle to the underlying fills
 */
XLFills& XLStyles::fills() const { return *m_fills; }

/**
 * @details return a handle to the underlying borders
 */
XLBorders& XLStyles::borders() const { return *m_borders; }

/**
 * @details return a handle to the underlying cell style formats
 */
XLCellFormats& XLStyles::cellStyleFormats() const { return *m_cellStyleFormats; }

/**
 * @details return a handle to the underlying cell formats
 */
XLCellFormats& XLStyles::cellFormats() const { return *m_cellFormats; }

/**
 * @details return a handle to the underlying cell styles
 */
XLCellStyles& XLStyles::cellStyles() const { return *m_cellStyles; }

/**
 * @details return a handle to the underlying differential cell formats
 */
XLDiffCellFormats& XLStyles::diffCellFormats() const { return *m_diffCellFormats; }
//
// XLTables.cpp
//
namespace OpenXLSX {
	// placeholder for utility functions
}

// ========== XLTables Member Functions

/**
 * @details The constructor creates an instance of the superclass, XLXmlFile
 */
XLTables::XLTables(XLXmlData* xmlData) : XLXmlFile(xmlData)
{
	if(xmlData->getXmlType() != XLContentType::Table)
		throw XLInternalError("XLTables constructor: Invalid XML data.");
}
/**
 * @details getters
 */
/*
std::string XLTables::get(std::string cellRef) const
{
	OpenXLSX::ignore(cellRef);
	return "";
}
 */
/**
 * @details setters
 */
/*
bool XLTables::set(std::string cellRef)
{
	OpenXLSX::ignore(cellRef);
	return false;
}
 */
/**
 * @details Print the underlying XML using pugixml::xml_node::print
 */
void XLTables::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }
//
// XLWorkbook.cpp
//
namespace {
	XMLNode sheetsNode(const OXlXmlDoc & doc) { return doc.document_element().child("sheets"); }
}

/**
 * @details The constructor initializes the member variables and calls the loadXMLData from the
 * XLAbstractXMLFile base class.
 */
XLWorkbook::XLWorkbook(XLXmlData* xmlData) : XLXmlFile(xmlData) {}
XLWorkbook::~XLWorkbook() = default;

XLSheet XLWorkbook::sheet(const std::string& sheetName)
{
	// ===== First determine if the sheet exists.
	if(xmlDocument().document_element().child("sheets").find_child_by_attribute("name", sheetName.c_str()) == nullptr)
		throw XLInputError("Sheet \"" + sheetName + "\" does not exist");

	// ===== Find the sheet data corresponding to the sheet with the requested name
	const std::string xmlID = xmlDocumentC().document_element().child("sheets").find_child_by_attribute("name", sheetName.c_str()).attribute("r:id").value();

	XLQuery pathQuery(XLQueryType::QuerySheetRelsTarget);
	pathQuery.setParam("sheetID", xmlID);
	auto xmlPath = parentDoc().execQuery(pathQuery).result<std::string>();

	// Some spreadsheets use absolute rather than relative paths in relationship items.
	if(xmlPath.substr(0, 4) == "/xl/")  xmlPath = xmlPath.substr(4);

	XLQuery xmlQuery(XLQueryType::QueryXmlData);
	xmlQuery.setParam("xmlPath", "xl/" + xmlPath);
	return XLSheet(parentDoc().execQuery(xmlQuery).result<XLXmlData*>());
}

/**
 * @details iterate over sheetsNode and count element nodes until index, get sheet name and return the corresponding sheet object
 *
 * @old-details Create a vector with sheet nodes, retrieve the node at the requested index, get sheet name and return the
 * corresponding sheet object.
 */
XLSheet XLWorkbook::sheet(uint16 index)    // 2024-04-30: whitespace support
{
	if(index < 1 || index > sheetCount())  
		throw XLInputError("Sheet index is out of bounds");
	// ===== Find the n-th node_element that corresponds to index
	uint16 curIndex = 0;
	for(XMLNode node = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element); not node.empty(); node = node.next_sibling_of_type(pugi::node_element)) {
		if(++curIndex == index)  
			return sheet(node.attribute("name").as_string());
	}
	// ===== If execution gets here, there are less element nodes than index in sheetsNode, this should never happen
	using namespace std::literals::string_literals;
	throw XLInternalError("Sheet index "s + std::to_string(index) + " is out of bounds"s);
}

XLWorksheet XLWorkbook::worksheet(const std::string& sheetName) { return sheet(sheetName).get<XLWorksheet>(); }
XLWorksheet XLWorkbook::worksheet(uint16 index) { return sheet(index).get<XLWorksheet>(); }
XLChartsheet XLWorkbook::chartsheet(const std::string& sheetName) { return sheet(sheetName).get<XLChartsheet>(); }
XLChartsheet XLWorkbook::chartsheet(uint16 index) { return sheet(index).get<XLChartsheet>(); }

// bool XLWorkbook::hasSharedStrings() const
// {
//     return true;    // always true
// }
//
// XLSharedStrings XLWorkbook::sharedStrings()
// {
//     const XLQuery query(XLQueryType::QuerySharedStrings);
//     return parentDoc().execQuery(query).result<XLSharedStrings>();
// }
//
void XLWorkbook::deleteNamedRanges()
{
	xmlDocument()
	.document_element()
	.child("definedNames")
	.remove_children();    // 2024-05-02: why loop if pugixml has a function for "delete all children"?
	// for (auto& child : xmlDocument().document_element().child("definedNames").children()) child.parent().remove_child(child);
}

void XLWorkbook::deleteSheet(const std::string& sheetName)    // 2024-05-02: whitespace support
                                                              // CAUTION: execCommand on underlying XML with whitespaces not verified
{
	// ===== Determine ID and type of sheet, as well as current worksheet count.
	std::string sheetID = sheetsNode(xmlDocumentC()).find_child_by_attribute("name", sheetName.c_str()).attribute("r:id").value();
	if(sheetID.length() == 0) { // 2025-01-12 BUGFIX: prevent segfault by throwing
		using namespace std::literals::string_literals;
		throw XLException("XLWorkbook::deleteSheet: workbook has no sheet with name \""s + sheetName + "\""s);
	}
	XLQuery sheetTypeQuery(XLQueryType::QuerySheetType);
	sheetTypeQuery.setParam("sheetID", std::string(sheetID)); // BUGFIX 2024-05-02: was using relationshipID() instead of sheetID,
	                                                          // leading to a bad sheetType & a failed check to not delete last worksheet
	const auto sheetType = parentDoc().execQuery(sheetTypeQuery).result<XLContentType>();

	// 2024-04-30: this should be whitespace-safe due to lambda expression checking for an attribute that non element nodes can not have
	const auto worksheetCount =
	    std::count_if(sheetsNode(xmlDocumentC()).children().begin(), sheetsNode(xmlDocumentC()).children().end(), [&](const XMLNode& item) {
		if(item.type() != pugi::node_element)  return false;// 2024-05-02 this avoids the unnecessary query below
		XLQuery query(XLQueryType::QuerySheetType);
		query.setParam("sheetID", std::string(item.attribute("r:id").value()));
		return parentDoc().execQuery(query).result<XLContentType>() == XLContentType::Worksheet;
	});

	// ===== If this is the last worksheet in the workbook, throw an exception.
	if(worksheetCount == 1 && sheetType == XLContentType::Worksheet)
		throw XLInputError("Invalid operation. There must be at least one worksheet in the workbook.");
	// ===== Delete the sheet data as well as the sheet node from Workbook.xml
	parentDoc().execCommand(XLCommand(XLCommandType::DeleteSheet).setParam("sheetID", std::string(sheetID)).setParam("sheetName", sheetName));
	XMLNode sheet = sheetsNode(xmlDocumentC()).find_child_by_attribute("name", sheetName.c_str());
	if(not sheet.empty()) {
		// ===== Delete all non element nodes (comments, whitespaces) following the sheet being deleted from workbook.xml <sheets> node
		XMLNode nonElementNode = sheet.next_sibling();
		while(!nonElementNode.empty() && nonElementNode.type() != pugi::node_element) {
			sheetsNode(xmlDocumentC()).remove_child(nonElementNode);
			nonElementNode = nonElementNode.next_sibling();
		}
		sheetsNode(xmlDocumentC()).remove_child(sheet); // delete the actual sheet entry
	}

	if(sheetIsActive(sheetID))
		xmlDocument().document_element().child("bookViews").first_child_of_type(pugi::node_element).remove_attribute("activeTab");
}

void XLWorkbook::addWorksheet(const std::string& sheetName)
{
	// ===== If a sheet with the given name already exists, throw an exception.
	if(xmlDocument().document_element().child("sheets").find_child_by_attribute("name", sheetName.c_str()))
		throw XLInputError("Sheet named \"" + sheetName + "\" already exists.");
	// ===== Create new internal (workbook) ID for the sheet
	auto internalID = createInternalSheetID();
	// ===== Create xml file for new worksheet and add metadata to the workbook file.
	parentDoc().execCommand(XLCommand(XLCommandType::AddWorksheet)
	    .setParam("sheetName", sheetName)
	    .setParam("sheetPath", "/xl/worksheets/sheet" + std::to_string(internalID) + ".xml"));
	prepareSheetMetadata(sheetName, internalID);
}

/**
 * @details
 * @todo If the original sheet's tabSelected attribute is set, ensure it is un-set in the clone.
 *       TBD: See comment in XLWorkbook::setSheetActive - should the tabSelected actually be un-set? It's not the same as the active tab,
 *        which does not need to be selected
 */
void XLWorkbook::cloneSheet(const std::string& existingName, const std::string& newName)
{
	parentDoc().execCommand(XLCommand(XLCommandType::CloneSheet).setParam("sheetID", sheetID(existingName)).setParam("cloneName", newName));
}

uint16 XLWorkbook::createInternalSheetID()    // 2024-04-30: whitespace support
{
	XMLNode sheet = xmlDocument().document_element().child("sheets").first_child_of_type(pugi::node_element);
	uint32 maxSheetIdFound = 0;
	while(!sheet.empty()) {
		uint32 thisSheetId = sheet.attribute("sheetId").as_uint();
		if(thisSheetId > maxSheetIdFound)  
			maxSheetIdFound = thisSheetId;
		sheet = sheet.next_sibling_of_type(pugi::node_element);
	}
	return static_cast<uint16>(maxSheetIdFound + 1);
}

std::string XLWorkbook::sheetID(const std::string& sheetName)
{
	return xmlDocument().document_element().child("sheets").find_child_by_attribute("name", sheetName.c_str()).attribute("r:id").value();
}

std::string XLWorkbook::sheetName(const std::string& sheetID) const
{
	return xmlDocumentC().document_element().child("sheets").find_child_by_attribute("r:id", sheetID.c_str()).attribute("name").value();
}

std::string XLWorkbook::sheetVisibility(const std::string& sheetID) const
{
	return xmlDocumentC().document_element().child("sheets").find_child_by_attribute("r:id", sheetID.c_str()).attribute("state").value();
}

void XLWorkbook::prepareSheetMetadata(const std::string& sheetName, uint16 internalID)
{
	// ===== Add new child node to the "sheets" node.
	auto node = sheetsNode(xmlDocument()).append_child("sheet");
	// ===== append the required attributes to the newly created sheet node.
	std::string sheetPath            = "/xl/worksheets/sheet" + std::to_string(internalID) + ".xml";
	node.append_attribute("name")    = sheetName.c_str();
	node.append_attribute("sheetId") = std::to_string(internalID).c_str();
	XLQuery query(XLQueryType::QuerySheetRelsID);
	query.setParam("sheetPath", sheetPath);
	node.append_attribute("r:id") = parentDoc().execQuery(query).result<std::string>().c_str();
}

void XLWorkbook::setSheetName(const std::string& sheetRID, const std::string& newName)
{
	auto sheetName = xmlDocument().document_element().child("sheets").find_child_by_attribute("r:id", sheetRID.c_str()).attribute("name");
	updateSheetReferences(sheetName.value(), newName);
	sheetName.set_value(newName.c_str());
}

void XLWorkbook::setSheetVisibility(const std::string& sheetRID, const std::string& state)    // 2024-04-30: whitespace support
{
	// ===== First, determine if there are other sheets that are visible
	int visibleSheets = 0;
	{
		// for (const auto& item : xmlDocument().document_element().child("sheets").children()) {
		XMLNode item = xmlDocument().document_element().child("sheets").first_child_of_type(pugi::node_element);
		while(!item.empty()) {
			if(std::string(item.attribute("r:id").value()) != sheetRID) {
				if(isVisible(item))  
					++visibleSheets;
			}
			item = item.next_sibling_of_type(pugi::node_element);
		}
	}
	bool hideSheet = not isVisibleState(state); // 2024-04-30: save for later use on activating another sheet if needed
	// ===== If there are no other visible sheets, and the current sheet is to be made hidden, throw an exception.
	if(hideSheet && visibleSheets == 0)
		throw XLSheetError("At least one sheet must be visible.");
	// ===== Then, retrieve or create the visibility ("state") attribute for the sheet, and set it to the "state" value
	auto stateAttribute = xmlDocument().document_element().child("sheets").find_child_by_attribute("r:id", sheetRID.c_str()).attribute("state");
	if(stateAttribute.empty()) {
		stateAttribute = xmlDocument().document_element().child("sheets").find_child_by_attribute("r:id", sheetRID.c_str()).prepend_attribute("state");
	}
	stateAttribute.set_value(state.c_str());
	// Next, find the index of the sheet...
	std::string name = xmlDocument().document_element().child("sheets").find_child_by_attribute("r:id", sheetRID.c_str()).attribute("name").value();
	auto index = indexOfSheet(name) - 1; // 2024-05-01: activeTab property stores an array index, NOT the value of r_ID?
	// ...and determine the index of the active sheet
	XMLNode workbookView = xmlDocument().document_element().child("bookViews").first_child_of_type(pugi::node_element);
	auto activeTabAttribute = workbookView.attribute("activeTab");
	if(activeTabAttribute.empty()) {
		activeTabAttribute = workbookView.append_attribute("activeTab");
		activeTabAttribute.set_value(0);
	}
	const auto activeTabIndex = activeTabAttribute.as_uint();
	// Finally, if the current sheet is the active one, set the "activeTab" attribute to the first visible sheet in the workbook
	if(hideSheet && activeTabIndex == index) { // BUGFIX 2024-04-30: previously, the active tab was re-set even if the current sheet was being set to "visible" (when already being visible)
		XMLNode item = xmlDocument().document_element().child("sheets").first_child_of_type(pugi::node_element);
		while(!item.empty()) {
			if(isVisible(item)) { // BUGFIX 2024-05-01: old check was testing state != "hidden" || != "veryHidden", which was always true
				activeTabAttribute.set_value(indexOfSheet(item.attribute("name").value()) - 1);
				break;
			}
			item = item.next_sibling_of_type(pugi::node_element);
		}
	}
}
/**
 * @details
 * @done In some cases (eg. if a sheet is moved to the position before the selected sheet), multiple sheets are selected when opened
 * in Excel.
 */
void XLWorkbook::setSheetIndex(const std::string& sheetName, uint index) // 2024-05-01: whitespace support
{
	// ===== Determine the index of the active tab, if any
	const XMLNode workbookView = xmlDocumentC().document_element().child("bookViews").first_child_of_type(pugi::node_element);
	const uint32 activeSheetIndex = 1 + workbookView.attribute("activeTab").as_uint(0); // use index in the 1.. range

	// ===== Attempt to locate the sheet with sheetName, and look out for the sheet @index, and the sheet @activeIndex while iterating over
	// the sheets
	XMLNode sheetToMove      {};     // determine the sheet matching sheetName, if any
	uint sheetToMoveIndex = 0;
	XMLNode existingSheet    {};     // determine the sheet at index, if any
	std::string activeSheet_rId  {}; // determine the r:id of the sheet at activeIndex, if any
	uint sheetIndex   = 1;
	XMLNode curSheet = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	int thingsToFind = (activeSheetIndex > 0) ? 3 : 2;         // if there is no active tab configured, no need to search for its name
	while(!curSheet.empty() && thingsToFind > 0) { // permit early loop exit when all sheets are located
		if(sheetToMove.empty() && (curSheet.attribute("name").value() == sheetName)) {
			sheetToMoveIndex = sheetIndex;
			sheetToMove      = curSheet;
			--thingsToFind;
		}
		if(existingSheet.empty() && (sheetIndex == index)) {
			existingSheet = curSheet;
			--thingsToFind;
		}
		if(activeSheet_rId.empty() && (sheetIndex == activeSheetIndex)) { // if no active sheet: activeSheetIndex 0 never matches
			activeSheet_rId = curSheet.attribute("r:id").value();
			--thingsToFind;
		}
		++sheetIndex;
		curSheet = curSheet.next_sibling_of_type(pugi::node_element);
	}
	// ===== If a sheet with sheetName was not found
	if(sheetToMove.empty())  
		throw XLInputError(std::string(__func__) + std::string(": no worksheet exists with name ") + sheetName);
	// ==== name was matched

	// ===== If the new index is equal to the current, don't do anything
	if(index == sheetToMoveIndex)  
		return;
	// ===== Modify the node in the XML file
	if(existingSheet.empty()) // new index is beyond last sheet
		sheetsNode(xmlDocumentC()).append_move(sheetToMove);
	else { // existingSheet was found
		if(sheetToMoveIndex < index)
			sheetsNode(xmlDocumentC()).insert_move_after(sheetToMove, existingSheet);
		else // sheetToMoveIndex > index, because if equal, function never gets here
			sheetsNode(xmlDocumentC()).insert_move_before(sheetToMove, existingSheet);
	}
	// ===== Updated defined names with worksheet scopes. TBD what this does
	XMLNode definedName = xmlDocumentC().document_element().child("definedNames").first_child_of_type(pugi::node_element);
	while(!definedName.empty()) {
		// TBD: is the current definedName actually associated with the sheet that was moved?
		definedName.attribute("localSheetId").set_value(sheetToMoveIndex - 1);
		definedName = definedName.next_sibling_of_type(pugi::node_element);
	}
	// ===== Update the activeTab attribute.
	if((activeSheetIndex < std::min(index, sheetToMoveIndex)) ||
	    (activeSheetIndex > std::max(index, sheetToMoveIndex))) // if active sheet was not within the set of sheets affected by the move
		return; // nothing to do
	if(activeSheet_rId.length() > 0)  
		setSheetActive(activeSheet_rId);
}

uint XLWorkbook::indexOfSheet(const std::string& sheetName) const    // 2024-05-01: whitespace support
{
	// ===== Iterate through sheet nodes. When a match is found, return the index;
	uint index = 1;
	for(XMLNode sheet = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	    not sheet.empty();
	    sheet         = sheet.next_sibling_of_type(pugi::node_element)) {
		if(sheetName == sheet.attribute("name").value())  
			return index;
		index++;
	}
	// ===== If a match is not found, throw an exception.
	throw XLInputError("Sheet does not exist");
}

XLSheetType XLWorkbook::typeOfSheet(const std::string& sheetName) const
{
	if(!sheetExists(sheetName))  
		throw XLInputError("Sheet with name \"" + sheetName + "\" doesn't exist.");
	if(worksheetExists(sheetName))  
		return XLSheetType::Worksheet;
	return XLSheetType::Chartsheet;
}

XLSheetType XLWorkbook::typeOfSheet(uint index) const    // 2024-05-01: whitespace support
{
	uint thisIndex = 1;
	for(XMLNode sheet = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	    not sheet.empty();
	    sheet         = sheet.next_sibling_of_type(pugi::node_element)) {
		if(thisIndex == index)  
			return typeOfSheet(sheet.attribute("name").as_string());
		++thisIndex;
	}
	using namespace std::literals::string_literals;
	throw XLInputError(std::string(__func__) + ": index "s + std::to_string(index) + " is out of range"s);
}

uint XLWorkbook::sheetCount() const    // 2024-04-30: whitespace support
{
	uint count = 0;
	for(XMLNode node = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element); not node.empty(); node = node.next_sibling_of_type(pugi::node_element))
		++count;
	return count;
}

uint XLWorkbook::worksheetCount() const { return static_cast<uint>(worksheetNames().size()); }
uint XLWorkbook::chartsheetCount() const { return static_cast<uint>(chartsheetNames().size()); }

std::vector<std::string> XLWorkbook::sheetNames() const    // 2024-05-01: whitespace support
{
	std::vector<std::string> results;
	for(XMLNode item = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element); not item.empty(); item = item.next_sibling_of_type(pugi::node_element))
		results.emplace_back(item.attribute("name").value());
	return results;
}

std::vector<std::string> XLWorkbook::worksheetNames() const    // 2024-05-01: whitespace support
{
	std::vector<std::string> results;
	for(XMLNode item = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	    not item.empty();
	    item = item.next_sibling_of_type(pugi::node_element)) {
		XLQuery query(XLQueryType::QuerySheetType);
		query.setParam("sheetID", std::string(item.attribute("r:id").value()));
		if(parentDoc().execQuery(query).result<XLContentType>() == XLContentType::Worksheet)
			results.emplace_back(item.attribute("name").value());
	}

	return results;
}

std::vector<std::string> XLWorkbook::chartsheetNames() const    // 2024-05-01: whitespace support
{
	std::vector<std::string> results;
	for(XMLNode item = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	    not item.empty();
	    item         = item.next_sibling_of_type(pugi::node_element)) {
		XLQuery query(XLQueryType::QuerySheetType);
		query.setParam("sheetID", std::string(item.attribute("r:id").value()));
		if(parentDoc().execQuery(query).result<XLContentType>() == XLContentType::Chartsheet)
			results.emplace_back(item.attribute("name").value());
	}

	return results;
}

bool XLWorkbook::sheetExists(const std::string& sheetName) const { return chartsheetExists(sheetName) || worksheetExists(sheetName); }

bool XLWorkbook::worksheetExists(const std::string& sheetName) const
{
	auto wksNames = worksheetNames();
	return std::find(wksNames.begin(), wksNames.end(), sheetName) != wksNames.end();
}

bool XLWorkbook::chartsheetExists(const std::string& sheetName) const
{
	auto chsNames = chartsheetNames();
	return std::find(chsNames.begin(), chsNames.end(), sheetName) != chsNames.end();
}

/**
 * @details The UpdateSheetName member function searches throug the usages of the old name and replaces with the
 * new sheet name.
 * @todo Currently, this function only searches through defined names. Consider using this function to update the
 * actual sheet name as well.
 */
void XLWorkbook::updateSheetReferences(const std::string& oldName,
    const std::string& newName)    // 2024-05-01: whitespace support with TBD to verify definedNames logic
{
	//        for (auto& sheet : m_sheets) {
	//            if (sheet.sheetType == XLSheetType::WorkSheet)
	//                Worksheet(sheet.sheetNode.attribute("name").getValue())->UpdateSheetName(oldName, newName);
	//        }

	// ===== Set up temporary variables
	std::string oldNameTemp = oldName;
	std::string newNameTemp = newName;
	std::string formula;

	// ===== If the sheet name contains spaces, it should be enclosed in single quotes (')
	if(oldName.find(' ') != std::string::npos)  oldNameTemp = "\'" + oldName + "\'";
	if(newName.find(' ') != std::string::npos)  newNameTemp = "\'" + newName + "\'";

	// ===== Ensure only sheet names are replaced (references to sheets always ends with a '!')
	oldNameTemp += '!';
	newNameTemp += '!';

	// ===== Iterate through all defined names // TODO 2024-05-01: verify definedNames logic
	XMLNode definedName = xmlDocument().document_element().child("definedNames").first_child_of_type(pugi::node_element);
	for(; not definedName.empty(); definedName = definedName.next_sibling_of_type(pugi::node_element)) {
		formula = definedName.text().get();

		// ===== Skip if formula contains a '[' and ']' (means that the defined refers to external workbook)
		if(formula.find('[') == std::string::npos && formula.find(']') == std::string::npos) {
			// ===== For all instances of the old sheet name in the formula, replace with the new name.
			while(formula.find(oldNameTemp) != std::string::npos) {
				formula.replace(formula.find(oldNameTemp), oldNameTemp.length(), newNameTemp);
			}
			definedName.text().set(formula.c_str());
		}
	}
}

void XLWorkbook::setFullCalculationOnLoad()
{
	XMLNode calcPr = xmlDocument().document_element().child("calcPr");
	auto getOrCreateAttribute = [&calcPr](const char* attributeName) {
		    XMLAttribute attr = calcPr.attribute(attributeName);
		    if(attr.empty())  attr = calcPr.append_attribute(attributeName);
		    return attr;
	    };

	getOrCreateAttribute("forceFullCalc").set_value(true);
	getOrCreateAttribute("fullCalcOnLoad").set_value(true);
}

void XLWorkbook::print(std::basic_ostream<char>& ostr) const { xmlDocumentC().document_element().print(ostr); }

bool XLWorkbook::sheetIsActive(const std::string& sheetRID) const    // 2024-04-30: whitespace support
{
	const XMLNode workbookView = xmlDocumentC().document_element().child("bookViews").first_child_of_type(pugi::node_element);
	const XMLAttribute activeTabAttribute = workbookView.attribute("activeTab");
	const int32_t activeTabIndex     = (not activeTabAttribute.empty() ? activeTabAttribute.as_int() : -1);  // 2024-05-29 BUGFIX: activeTabAttribute was being read as_uint
	if(activeTabIndex == -1)  
		return false;// 2024-05-29 early exit: no need to try and match sheetRID if there *is* no active tab
	int32_t index = 0; // 2024-06-04 BUGFIX: index should support -1 as 2024-05-29 change below sets it to -1 for preventing a match with activeTabIndex
	XMLNode item  = sheetsNode(xmlDocumentC()).first_child_of_type(pugi::node_element);
	while(!item.empty()) {
		if(std::string(item.attribute("r:id").value()) == sheetRID)  break;
		++index;
		item = item.next_sibling_of_type(pugi::node_element);
	}
	if(item.empty())  index = -1;// 2024-05-29: prevent a match if activeTabIndex invalidly points to a non-existing sheet

	return index == activeTabIndex;
}
/**
 * @details
 * @done: no exception if setSheetActive fails, instead return false
 * @done: fail by returning false if sheetRID is either not found or belongs to a sheet that is not visible
 * @note: this makes some bug fixes from 2024-05-29 obsolete
 * @note: changed behavior: attempting to setSheetActive on a non-existing or non-visible sheet will no longer unselect the active sheet
 */
bool XLWorkbook::setSheetActive(const std::string& sheetRID)    // 2024-04-30: whitespace support
{
	XMLNode workbookView       = xmlDocument().document_element().child("bookViews").first_child_of_type(pugi::node_element);
	const XMLAttribute activeTabAttribute = workbookView.attribute("activeTab");
	int32_t activeTabIndex     = -1;           // negative == no active tab identified
	if(not activeTabAttribute.empty())  activeTabIndex = activeTabAttribute.as_int();

	int32_t index = 0; // index should have the same data type as activeTabIndex for comparisons
	XMLNode item  = sheetsNode(xmlDocument()).first_child_of_type(pugi::node_element);
	while(!item.empty() && (std::string(item.attribute("r:id").value()) != sheetRID)) {
		++index;
		item = item.next_sibling_of_type(pugi::node_element);
	}
	// ===== 2024-06-19: Fail without action if sheet is not found or sheet is not visible
	if(item.empty() || !isVisible(item))  
		return false;
	// NOTE: XLSheet XLWorkbook::sheet(uint16 index) is using a 1-based index, while the workbookView attribute activeTab is using a
	// 0-based index
	// ===== If an active sheet was found, but sheetRID is not the same sheet: attempt to unselect the old active sheet.
	if((activeTabIndex != -1) && (index != activeTabIndex))
		sheet(static_cast<uint16>(activeTabIndex + 1)).setSelected(false);// see NOTE above
	// ===== Set the activeTab property for the workbook.xml sheets node
	if(workbookView.attribute("activeTab").empty())
		workbookView.append_attribute("activeTab");
	workbookView.attribute("activeTab").set_value(index);
	// sheet(index + 1).setSelected(true);  // it appears that an active sheet does not have to be selected
	return true; // success
}

/**
 * @details evaluate a sheet node state attribute where "hidden" or "veryHidden" means not visible
 * @note 2024-05-01 BUGFIX: veryHidden was not checked (in setSheetActive)
 */
bool XLWorkbook::isVisibleState(std::string const& state) const { return (state != "hidden" && state != "veryHidden"); }

/**
 * @details function only returns meaningful information when used with a sheet node
 *          (or nodes with state attribute allowing values visible, hidden, veryHidden)
 */
bool XLWorkbook::isVisible(XMLNode const& sheetNode) const
{
	if(sheetNode.empty())
		return false; // empty nodes can't be visible
	if(sheetNode.attribute("state").empty())
		return true;// no state attribute means no hidden-ness can be flagged
	// ===== Attribute exists and value must be checked
	return isVisibleState(sheetNode.attribute("state").value());
}
//
// XLXmlData.cpp
//
XLXmlData::XLXmlData(XLDocument* parentDoc, const std::string& xmlPath, const std::string& xmlId, XLContentType xmlType) : m_parentDoc(parentDoc),
	m_xmlPath(xmlPath), m_xmlID(xmlId), m_xmlType(xmlType), m_xmlDoc(std::make_unique<OXlXmlDoc>())
{
	m_xmlDoc->reset();
}

XLXmlData::~XLXmlData() = default;

void XLXmlData::setRawData(const std::string& data)
{
	m_xmlDoc->load_string(data.c_str(), pugi_parse_settings);
}

/**
 * @details
 * @note Default encoding for pugixml xml_document::save is pugi::encoding_auto, becomes pugi::encoding_utf8
 */
std::string XLXmlData::getRawData(XLXmlSavingDeclaration savingDeclaration) const
{
	OXlXmlDoc * doc = const_cast<OXlXmlDoc *>(getXmlDocumentC());
	// ===== 2024-07-08: ensure that the default encoding UTF-8 is explicitly written to the XML document with a custom saving declaration
	XMLNode saveDeclaration = doc->first_child();
	if(saveDeclaration.empty() || saveDeclaration.type() != pugi::node_declaration) { // if saving declaration node does not exist
		doc->prepend_child(pugi::node_pcdata).set_value("\n");        // prepend a line break
		saveDeclaration = doc->prepend_child(pugi::node_declaration); // prepend a saving declaration
	}
	// ===== If a node_declaration could be fetched or created
	if(not saveDeclaration.empty()) {
		// ===== Fetch or create saving declaration attributes
		XMLAttribute attrVersion = saveDeclaration.attribute("version");
		if(attrVersion.empty())
			attrVersion = saveDeclaration.append_attribute("version");
		XMLAttribute attrEncoding = saveDeclaration.attribute("encoding");
		if(attrEncoding.empty())
			attrEncoding = saveDeclaration.append_attribute("encoding");
		XMLAttribute attrStandalone = saveDeclaration.attribute("standalone");
		if(attrStandalone.empty() && savingDeclaration.standalone_as_bool()) // only if standalone is set in passed savingDeclaration
			attrStandalone = saveDeclaration.append_attribute("standalone"); // then make sure it exists

		// ===== Set saving declaration attribute values (potentially overwriting existing values)
		attrVersion = savingDeclaration.version().c_str();      // version="1.0" is XML default
		attrEncoding = savingDeclaration.encoding().c_str();    // encoding="UTF-8" is XML default

		if(not attrStandalone.empty()) // only save standalone attribute if previously existing or newly set to standalone="yes"
			attrStandalone = savingDeclaration.standalone().c_str(); // standalone="no" is XML default
	}

	std::ostringstream ostr;
	doc->save(ostr, "", pugi::format_raw);
	return ostr.str();
}

XLDocument * XLXmlData::getParentDoc() { return m_parentDoc; }
const XLDocument * XLXmlData::getParentDoc() const { return m_parentDoc; }
std::string XLXmlData::getXmlPath() const { return m_xmlPath; }
std::string XLXmlData::getXmlID() const { return m_xmlID; }
XLContentType XLXmlData::getXmlType() const { return m_xmlType; }

OXlXmlDoc * XLXmlData::getXmlDocument()
{
	if(!m_xmlDoc->document_element())
		m_xmlDoc->load_string(m_parentDoc->extractXmlFromArchive(m_xmlPath).c_str(), pugi_parse_settings);
	return m_xmlDoc.get();
}

const OXlXmlDoc * XLXmlData::getXmlDocumentC() const
{
	if(!m_xmlDoc->document_element())
		m_xmlDoc->load_string(m_parentDoc->extractXmlFromArchive(m_xmlPath).c_str(), pugi_parse_settings);
	return m_xmlDoc.get();
}
//
// XLXmlFile.cpp
//
/**
 * @details The constructor creates a new object with the parent XLDocument and the file path as input, with
 * an optional input being a std::string with the XML data. If the XML data is provided by a string, any file with
 * the same path in the .zip file will be overwritten upon saving of the document. If no xmlData is provided,
 * the data will be read from the .zip file, using the given path.
 */
XLXmlFile::XLXmlFile(XLXmlData* xmlData) : m_xmlData(xmlData) {}

XLXmlFile::~XLXmlFile() = default;
/**
 * @details This method sets the XML data with a std::string as input. The underlying OXlXmlDoc reads the data.
 * When envoking the load_string method in PugiXML, the flag 'parse_ws_pcdata' is passed along with the default flags.
 * This will enable parsing of whitespace characters. If not set, Excel cells with only spaces will be returned as
 * empty strings, which is not what we want. The downside is that whitespace characters such as \\n and \\t in the
 * input xml file may mess up the parsing.
 */
void XLXmlFile::setXmlData(const std::string& xmlData)
{
	m_xmlData->setRawData(xmlData);
}
/**
 * @details This method retrieves the underlying XML data as a std::string.
 */
std::string XLXmlFile::xmlData() const { return m_xmlData->getRawData(); }
const XLDocument& XLXmlFile::parentDoc() const { return *m_xmlData->getParentDoc(); }
XLDocument& XLXmlFile::parentDoc() { return *m_xmlData->getParentDoc(); }
std::string XLXmlFile::relationshipID() const { return m_xmlData->getXmlID(); }
/**
 * @details This method returns a pointer to the underlying OXlXmlDoc resource.
 */
OXlXmlDoc & XLXmlFile::xmlDocument()
{
	return const_cast<OXlXmlDoc&>(static_cast<const XLXmlFile *>(this)->xmlDocumentC());
	// return *m_xmlData->getXmlDocument();    // <- why not this easy version?
}
/**
 * @details This method returns a pointer to the underlying OXlXmlDoc resource as const.
 */
const OXlXmlDoc & XLXmlFile::xmlDocumentC() const { return *m_xmlData->getXmlDocument(); }
/**
 * @details provide access to the underlying XLXmlData::getXmlPath() function
 */
std::string XLXmlFile::getXmlPath() const { return m_xmlData == nullptr ? "" : m_xmlData->getXmlPath(); }
//
// XLXmlParser.cpp
//
namespace OpenXLSX {
	// ===== Copy definition of PUGI_IMPL_NODETYPE, which is defined in pugixml.cpp, within namespace pugi::impl(?), and somehow doesn't work here
	#define PUGI_IMPL_NODETYPE(n) static_cast<pugi::xml_node_type>((n)->header & pugi::impl::xml_memory_page_type_mask)

	bool NO_XML_NS = true;     // default: no XML namespaces
	/**
	 * @details this function is meaningless when PUGI_AUGMENTED is not defined / used
	 */
	bool enable_xml_namespaces()
	{
	#ifdef PUGI_AUGMENTED
		NO_XML_NS = false;
		return true;
	#else
		return false;
	#endif
	}
	/**
	 * @details this function is meaningless when PUGI_AUGMENTED is not defined / used
	 */
	bool disable_xml_namespaces()
	{
	#ifdef PUGI_AUGMENTED
		NO_XML_NS = true;
		return true;
	#else
		return false;
	#endif
	}
	/**
	 * @details return the node name without a potentially(!) existing namespace
	 */
	const pugi::char_t* XMLNode::name_without_namespace(const pugi::char_t* name_) const
	{
		if(NO_XML_NS)
			return name_; // if node namespaces are not stripped: return immediately
		int pos = 0;
		while(name_[pos] && name_[pos] != ':')
			++pos; // find namespace delimiter
		if(!name_[pos])
			return name_; // if no delimiter found: return unmodified name
		return name_ + pos + 1; // if delimiter was found: return the name minus the namespace
	}
	/**
	 * @details for creation of node children: copy this node's namespace without thread safety, using
	 *  a static character array to avoid smart pointer performance impact
	 */
	const pugi::char_t* XMLNode::namespaced_name_char(const pugi::char_t* name_, bool force_ns) const
	{
		// ===== If node has no namespace: Early pass-through return
		if(!name_begin || force_ns)
			return name_;
		if(name_begin + sstrlen(name_) > XLMaxNamespacedNameLen) {
			using namespace std::literals::string_literals;
			throw XLException("OpenXLSX_xml_node::"s + __func__ + ": strlen of "s + name_ + " exceeds XLMaxNamespacedNameLen "s + std::to_string(XLMaxNamespacedNameLen));
		}
		static pugi::char_t namespaced_name_[ XLMaxNamespacedNameLen + 1 ]; // static memory allocation for concatenating node namespace and name_
		// ===== If node has a namespace: create a namespaced version of name_
		memcpy(namespaced_name_, xml_node::name(), name_begin);    // copy the node namespace
		strcpy(namespaced_name_ + name_begin, name_);              // concatenate the name_ including the terminating zero
		return namespaced_name_;
	}
	/**
	 * @details for creation of node children: copy this node's namespace with thread safety, using
	 *  smart pointer with a trade-off for ~15-20% performance impact (increased runtime)
	 * @note // 2024-08-18: made lambda parameter unnamed to eliminate -Wunused-parameter
	 */
	std::shared_ptr<pugi::char_t> XMLNode::namespaced_name_shared_ptr(const pugi::char_t* name_, bool force_ns) const
	{
		// ===== If node has no namespace: Early pass-through return with noop-deleter
		if(!name_begin || force_ns) 
			return std::shared_ptr<pugi::char_t>(const_cast<pugi::char_t*>(name_), [](pugi::char_t*){});
		// ===== If node has a namespace: allocate memory for concatenation and create a namespaced version of name_
		std::shared_ptr<pugi::char_t> namespaced_name_(new pugi::char_t[name_begin + sstrlen(name_) + 1], std::default_delete<pugi::char_t[]>());
		memcpy(namespaced_name_.get(), xml_node::name(), name_begin);    // copy the node namespace
		strcpy(namespaced_name_.get() + name_begin, name_);          // concatenate the name_ with terminating zero
		return namespaced_name_;
	}
	/**
	 * @details determine the first xml_node child whose xml_node_type matches type_
	 * @date 2024-04-25
	 */
	XMLNode XMLNode::first_child_of_type(pugi::xml_node_type type_) const
	{
		if(_root) {
			XMLNode x = first_child();
			XMLNode l = last_child();
			while(x != l && x.type() != type_)  
				x = x.next_sibling();
			if(x.type() == type_)
				return XMLNode(x);
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
	/**
	 * @details determine the last xml_node child whose xml_node_type matches type_
	 * @date 2024-04-25
	 */
	XMLNode XMLNode::last_child_of_type(pugi::xml_node_type type_) const
	{
		if(_root) {
			XMLNode f = first_child();
			XMLNode x = last_child();
			while(x != f && x.type() != type_)  
				x = x.previous_sibling();
			if(x.type() == type_)
				return XMLNode(x);
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
	/**
	 * @details determine amount of xml_node children child whose xml_node_type matches type_
	 * @date 2024-04-28
	 */
	size_t XMLNode::child_count_of_type(pugi::xml_node_type type_) const
	{
		size_t counter = 0;
		if(_root) {
			XMLNode c = first_child_of_type(type_);
			while(!c.empty()) {
				++counter;
				c = c.next_sibling_of_type(type_);
			}
		}
		return counter;
	}
	/**
	 * @details determine the next xml_node sibling whose xml_node_type matches type_
	 * @date 2024-04-26
	 */
	XMLNode XMLNode::next_sibling_of_type(pugi::xml_node_type type_) const
	{
		if(_root) {
			pugi::xml_node_struct* next = _root->next_sibling;
			while(next && (PUGI_IMPL_NODETYPE(next) != type_))
				next = next->next_sibling;
			if(next)
				return XMLNode(next);
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
	/**
	 * @details determine the previous xml_node sibling whose xml_node_type matches type_
	 * @date 2024-04-26
	 */
	XMLNode XMLNode::previous_sibling_of_type(pugi::xml_node_type type_) const
	{
		if(_root) {
			pugi::xml_node_struct* prev = _root->prev_sibling_c;
			while(prev->next_sibling && (PUGI_IMPL_NODETYPE(prev) != type_))  prev = prev->prev_sibling_c;
			if(prev->next_sibling)
				return XMLNode(prev);
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
	/**
	 * @details determine the next xml_node sibling whose name() matches name_ and xml_node_type matches type_
	 * @date 2024-04-26
	 */
	XMLNode XMLNode::next_sibling_of_type(const pugi::char_t* name_, pugi::xml_node_type type_) const
	{
		if(_root) {
			for(pugi::xml_node_struct* i = _root->next_sibling; i; i = i->next_sibling) {
				const pugi::char_t* iname = i->name;
				if(iname && pugi::impl::strequal(name_, iname) && (PUGI_IMPL_NODETYPE(i) == type_))
					return XMLNode(i);
			}
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
	/**
	 * @details determine the previous xml_node sibling whose name() matches name_ and xml_node_type matches type_
	 * @date 2024-04-26
	 */
	XMLNode XMLNode::previous_sibling_of_type(const pugi::char_t* name_, pugi::xml_node_type type_) const
	{
		if(_root) {
			for(pugi::xml_node_struct* i = _root->prev_sibling_c; i->next_sibling; i = i->prev_sibling_c) {
				const pugi::char_t* iname = i->name;
				if(iname && pugi::impl::strequal(name_, iname) && (PUGI_IMPL_NODETYPE(i) == type_))
					return XMLNode(i);
			}
		}
		return XMLNode();    // if no node matching type_ was found: return an empty node
	}
}
//
// XLZipArchive.cpp
//
XLZipArchive::XLZipArchive() : m_archive(nullptr) 
{
}

XLZipArchive::~XLZipArchive() = default;

XLZipArchive::operator bool() const { return isValid(); }
bool XLZipArchive::isValid() const { return m_archive != nullptr; }
bool XLZipArchive::isOpen() const { return m_archive && m_archive->IsOpen(); }

void XLZipArchive::open(const std::string& fileName)
{
	m_archive = std::make_shared<Zippy::ZipArchive>();
	try {
		m_archive->Open(fileName);
	}
	catch(...) {  // catch all exceptions
		m_archive.reset(); // make m_archive invalid again
		throw;        // re-throw
	}
}

void XLZipArchive::close()
{
	m_archive->Close();
	m_archive.reset();
}

void XLZipArchive::save(const std::string& path) { m_archive->Save(path); }
void XLZipArchive::addEntry(const std::string& name, const std::string& data) { m_archive->AddEntry(name, data); }
void XLZipArchive::deleteEntry(const std::string& entryName) { m_archive->DeleteEntry(entryName); }
std::string XLZipArchive::getEntry(const std::string& name) const { return m_archive->GetEntry(name).GetDataAsString(); }
bool XLZipArchive::hasEntry(const std::string& entryName) const { return m_archive->HasEntry(entryName); }
