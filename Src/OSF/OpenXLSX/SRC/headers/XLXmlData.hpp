// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLXMLDATA_HPP
#define OPENXLSX_XLXMLDATA_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <memory>
#include <string>
#include "XLContentTypes.hpp"
#include "XLXmlParser.hpp"

namespace OpenXLSX {
constexpr const char * XLXmlDefaultVersion = "1.0";
constexpr const char * XLXmlDefaultEncoding = "UTF-8";
constexpr const bool XLXmlStandalone = true;
constexpr const bool XLXmlNotStandalone = false;
/**
 * @brief The XLXmlSavingDeclaration class encapsulates the properties of an XML saving declaration,
 * that can be used in calls to XLXmlData::getRawData to enforce specific settings
 */
class XLXmlSavingDeclaration {
public:
	// ===== PUBLIC MEMBER FUNCTIONS ===== //
	XLXmlSavingDeclaration() : m_version(XLXmlDefaultVersion), m_encoding(XLXmlDefaultEncoding), m_standalone(XLXmlNotStandalone) {}
	XLXmlSavingDeclaration(XLXmlSavingDeclaration const & other) = default; // copy constructor
	XLXmlSavingDeclaration(std::string version, std::string encoding, bool standalone = XLXmlNotStandalone)
		: m_version(version), m_encoding(encoding), m_standalone(standalone) {}
	~XLXmlSavingDeclaration() {}

	/**
	 * @brief: getter functions: version, encoding, standalone
	 */
	std::string const & version() const { return m_version; }
	std::string const & encoding() const { return m_encoding; }
	bool standalone_as_bool() const { return m_standalone; }
	std::string const standalone() const { return m_standalone ? "yes" : "no"; }
private:
	// ===== PRIVATE MEMBER VARIABLES ===== //
	std::string m_version;
	std::string m_encoding;
	bool m_standalone;
};

/**
 * @brief The XLXmlData class encapsulates the properties and behaviour of the .xml files in an .xlsx file zip
 * package. Objects of the XLXmlData type are intended to be stored centrally in an XLDocument object, from where
 * they can be retrieved by other objects that encapsulates the behaviour of Excel elements, such as XLWorkbook
 * and XLWorksheet.
 */
class XLXmlData final
{
public:
	// ===== PUBLIC MEMBER FUNCTIONS ===== //

	/**
	 * @brief Default constructor. All member variables are default constructed. Except for
	 * the raw XML data, none of the member variables can be modified after construction. Hence, objects created
	 * using the default constructor can only serve as null objects and targets for the move assignemnt operator.
	 */
	XLXmlData() = default;

	/**
	 * @brief Constructor. This constructor creates objects with the given parameters. the xmlId and the xmlType
	 * parameters have default values. These are only useful for relationship (.rels) files and the
	 * [Content_Types].xml file located in the root directory of the zip package.
	 * @param parentDoc A pointer to the parent XLDocument object.
	 * @param xmlPath A std::string with the file path in zip package.
	 * @param xmlId A std::string with the relationship ID of the file (used in the XLRelationships class)
	 * @param xmlType The type of object the XML file represents, e.g. XLWorkbook or XLWorksheet.
	 */
	XLXmlData(XLDocument*        parentDoc,
	    const std::string& xmlPath,
	    const std::string& xmlId   = "",
	    XLContentType xmlType = XLContentType::Unknown);

	/**
	 * @brief Default destructor. The XLXmlData does not manage any dynamically allocated resources, so a default
	 * destructor will suffice.
	 */
	~XLXmlData();

	/**
	 * @brief check whether class is linked to a valid XML document
	 * @return true if the class should have a link to valid data
	 * @return false if accessing any other properties / methods could cause a segmentation fault
	 */
	bool valid() const { return m_xmlDoc != nullptr; }

	/**
	 * @brief Copy constructor. The m_xmlDoc data member is a OXlXmlDoc object, which is non-copyable. Hence,
	 * the XLXmlData objects have a explicitly deleted copy constructor.
	 * @param other
	 */
	XLXmlData(const XLXmlData& other) = delete;

	/**
	 * @brief Move constructor. All data members are trivially movable. Hence an explicitly defaulted move
	 * constructor is sufficient.
	 * @param other
	 */
	XLXmlData(XLXmlData&& other) noexcept = default;

	/**
	 * @brief Copy assignment operator. The m_xmlDoc data member is a OXlXmlDoc object, which is non-copyable.
	 * Hence, the XLXmlData objects have a explicitly deleted copy assignment operator.
	 */
	XLXmlData& operator=(const XLXmlData& other) = delete;

	/**
	 * @brief Move assignment operator. All data members are trivially movable. Hence an explicitly defaulted move
	 * constructor is sufficient.
	 * @param other the XLXmlData object to be moved from.
	 * @return A reference to the moved-to object.
	 */
	XLXmlData& operator=(XLXmlData&& other) noexcept = default;

	/**
	 * @brief Set the raw data for the underlying XML document. Being able to set the XML data directly is useful
	 * when creating a new file using a XML file template. E.g., when creating a new worksheet, the XML code for
	 * a minimum viable XLWorksheet object can be added using this function.
	 * @param data A std::string with the raw XML text.
	 */
	void setRawData(const std::string& data);

	/**
	 * @brief Get the raw data for the underlying XML document. This function will retrieve the raw XML text data
	 * from the underlying OXlXmlDoc object. This will mainly be used when saving data to the .xlsx package
	 * using the save function in the XLDocument class.
	 * @param savingDeclaration @optional specify an XML saving declaration to use
	 * @return A std::string with the raw XML text data.
	 */
	std::string getRawData(XLXmlSavingDeclaration savingDeclaration = XLXmlSavingDeclaration{}) const;

	/**
	 * @brief Access the parent XLDocument object.
	 * @return A pointer to the parent XLDocument object.
	 */
	XLDocument* getParentDoc();

	/**
	 * @brief Access the parent XLDocument object.
	 * @return A const pointer to the parent XLDocument object.
	 */
	const XLDocument* getParentDoc() const;

	/**
	 * @brief Retrieve the path of the XML data in the .xlsx zip archive.
	 * @return A std::string with the path.
	 */
	std::string getXmlPath() const;

	/**
	 * @brief Retrieve the relationship ID of the XML file.
	 * @return A std::string with the relationship ID.
	 */
	std::string getXmlID() const;
	/**
	 * @brief Retrieve the type represented by the XML data.
	 * @return A XLContentType getValue representing the type.
	 */
	XLContentType getXmlType() const;
	/**
	 * @brief Access the underlying OXlXmlDoc object.
	 * @return A pointer to the OXlXmlDoc object.
	 */
	OXlXmlDoc * getXmlDocument();
	/**
	 * @brief Access the underlying OXlXmlDoc object.
	 * @return A const pointer to the OXlXmlDoc object.
	 */
	const OXlXmlDoc * getXmlDocumentC() const;
	/**
	 * @brief Test whether there is an XML file linked to this object
	 * @return true if there is no underlying XML file, otherwise false
	 */
	bool empty() const;
private:
	// ===== PRIVATE MEMBER VARIABLES ===== //

	XLDocument*                          m_parentDoc {}; /**< A pointer to the parent XLDocument object. >*/
	std::string m_xmlPath {};                            /**< The path of the XML data in the .xlsx zip archive. >*/
	std::string m_xmlID {};                              /**< The relationship ID of the XML data. >*/
	XLContentType m_xmlType {};                          /**< The type represented by the XML data. >*/
	mutable std::unique_ptr<OXlXmlDoc> m_xmlDoc;       /**< The underlying OXlXmlDoc object. >*/
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLXMLDATA_HPP
