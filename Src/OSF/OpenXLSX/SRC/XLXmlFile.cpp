// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

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
