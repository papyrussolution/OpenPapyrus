// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop

using namespace OpenXLSX;

XLXmlData::XLXmlData(XLDocument* parentDoc, const std::string& xmlPath, const std::string& xmlId, XLContentType xmlType) : m_parentDoc(parentDoc),
	m_xmlPath(xmlPath), m_xmlID(xmlId), m_xmlType(xmlType), m_xmlDoc(std::make_unique<OXlXmlDoc>())
{
	m_xmlDoc->reset();
}

XLXmlData::~XLXmlData() = default;

void XLXmlData::setRawData(const std::string& data) // NOLINT
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
		doc->prepend_child(pugi::node_pcdata).set_value("\n");                        // prepend a line break
		saveDeclaration = doc->prepend_child(pugi::node_declaration);                 // prepend a saving declaration
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
