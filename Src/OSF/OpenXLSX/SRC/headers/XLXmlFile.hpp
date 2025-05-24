// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLXMLFILE_HPP
#define OPENXLSX_XLXMLFILE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER
#include "XLXmlParser.hpp"

namespace OpenXLSX {
class XLXmlData;
class XLDocument;

/**
 * @brief The XLUnsupportedElement class provides a stub implementation that can be used as function
 *  parameter or return type for currently unsupported XML features.
 */
class XLUnsupportedElement
{
public:
	XLUnsupportedElement() {}
	bool empty() const { return true; }
	std::string summary() const { return "XLUnsupportedElement"; }
};

/**
 * @brief The XLXmlFile class provides an interface for derived classes to use.
 * It functions as an ancestor to all classes which are represented by an .xml file in an .xlsx package.
 * @warning The XLXmlFile class is not intended to be instantiated on it's own, but to provide an interface for
 * derived classes. Also, it should not be used polymorphically. For that reason, the destructor is not declared virtual.
 */
class XLXmlFile {
public:        // ===== PUBLIC MEMBER FUNCTIONS
	XLXmlFile() = default;
	explicit XLXmlFile(XLXmlData* xmlData);
	XLXmlFile(const XLXmlFile& other) = default;
	XLXmlFile(XLXmlFile&& other) noexcept = default;
	~XLXmlFile();
	/**
	 * @brief check whether class is linked to a valid XML file
	 * @return true if the class should have a link to valid data
	 * @return false if accessing any other sheet properties / methods could cause a segmentation fault
	 * @note for example, if an XLSheet is created with a default constructor, XLSheetBase::valid() (derived from XLXmlFile) would return false
	 */
	bool valid() const { return m_xmlData != nullptr; }
	/**
	 * @brief The copy assignment operator. The default implementation has been used.
	 * @param other The object to copy.
	 * @return A reference to the new object.
	 */
	XLXmlFile & operator=(const XLXmlFile& other) = default;
	/**
	 * @brief The move assignment operator. The default implementation has been used.
	 * @param other The object to move.
	 * @return A reference to the new object.
	 */
	XLXmlFile & operator=(XLXmlFile&& other) noexcept = default;
protected:
	/**
	 * @brief Method for getting the XML data represented by the object.
	 * @return A std::string with the XML data.
	 */
	std::string xmlData() const;
	/**
	 * @brief Provide the XML data represented by the object.
	 * @param xmlData A std::string with the XML data.
	 */
	void setXmlData(const std::string& xmlData);
	/**
	 * @brief This function returns the relationship ID (the ID used in the XLRelationships objects) for the object.
	 * @return A std::string with the ID. Not all spreadsheet objects may have a relationship ID. In those cases an empty string is
	 * returned.
	 */
	std::string relationshipID() const;
	/**
	 * @brief This function provides access to the parent XLDocument object.
	 * @return A reference to the parent XLDocument object.
	 */
	XLDocument & parentDoc();
	/**
	 * @brief This function provides access to the parent XLDocument object.
	 * @return A const reference to the parent XLDocument object.
	 */
	const XLDocument & parentDoc() const;
	/**
	 * @brief This function provides access to the underlying OXlXmlDoc object.
	 * @return A reference to the OXlXmlDoc object.
	 */
	OXlXmlDoc & xmlDocument();
	/**
	 * @brief This function provides access to the underlying OXlXmlDoc object.
	 * @return A const reference to the OXlXmlDoc object.
	 */
	const OXlXmlDoc & xmlDocumentC() const;
	/**
	 * @brief Retrieve the path of the XML data in the .xlsx zip archive via m_xmlData->getXmlPath
	 * @return A std::string with the path. Empty string if m_xmlData is nullptr
	 */
	std::string getXmlPath() const;
protected:                                // ===== PROTECTED MEMBER VARIABLES
	XLXmlData * m_xmlData { nullptr }; /**< The underlying XML data object. */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLXMLFILE_HPP
