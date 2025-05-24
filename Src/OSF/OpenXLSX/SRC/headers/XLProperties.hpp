// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLPROPERTIES_HPP
#define OPENXLSX_XLPROPERTIES_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <string>
#include "XLXmlFile.hpp"

namespace OpenXLSX {
class XLProperties : public XLXmlFile {
private:
	/**
	 * @brief constructor helper function: create core.xml content from template
	 * @param workbook
	 */
	void createFromTemplate();

	//----------------------------------------------------------------------------------------------------------------------
	//           Public Member Functions
	//----------------------------------------------------------------------------------------------------------------------

public:
	/**
	 * @brief
	 */
	XLProperties() = default;

	/**
	 * @brief
	 * @param xmlData
	 */
	explicit XLProperties(XLXmlData* xmlData);

	/**
	 * @brief
	 * @param other
	 */
	XLProperties(const XLProperties& other) = default;

	/**
	 * @brief
	 * @param other
	 */
	XLProperties(XLProperties&& other) noexcept = default;

	/**
	 * @brief
	 */
	~XLProperties();

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLProperties& operator=(const XLProperties& other) = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLProperties& operator=(XLProperties&& other) = default;

	/**
	 * @brief
	 * @param name
	 * @param value
	 * @return
	 */
	void setProperty(const std::string& name, const std::string& value);

	/**
	 * @brief
	 * @param name
	 * @param value
	 * @return
	 */
	void setProperty(const std::string& name, int value);

	/**
	 * @brief
	 * @param name
	 * @param value
	 * @return
	 */
	void setProperty(const std::string& name, double value);

	/**
	 * @brief
	 * @param name
	 * @return
	 */
	std::string property(const std::string& name) const;

	/**
	 * @brief
	 * @param name
	 */
	void deleteProperty(const std::string& name);

	//----------------------------------------------------------------------------------------------------------------------
	//           Protected Member Functions
	//----------------------------------------------------------------------------------------------------------------------
};

/**
 * @brief This class is a specialization of the XLAbstractXMLFile, with the purpose of the representing the
 * document app properties in the app.xml file (docProps folder) in the .xlsx package.
 */
class XLAppProperties : public XLXmlFile {
private:
	/**
	 * @brief constructor helper function: create app.xml content from template
	 * @param workbook
	 */
	void createFromTemplate(OXlXmlDoc const & workbookXml);
public:
	XLAppProperties() = default;
	/**
	 * @brief enable XLAppProperties to re-create a worksheet list in docProps/app.xml <TitlesOfParts> element from workbookXml
	 * @param xmlData
	 * @param workbook
	 */
	explicit XLAppProperties(XLXmlData* xmlData, const OXlXmlDoc & workbookXml);
	explicit XLAppProperties(XLXmlData* xmlData);
	XLAppProperties(const XLAppProperties& other) = default;
	XLAppProperties(XLAppProperties&& other) noexcept = default;
	~XLAppProperties();
	XLAppProperties& operator=(const XLAppProperties& other) = default;
	XLAppProperties& operator=(XLAppProperties&& other) noexcept = default;
	/**
	 * @brief update the "HeadingPairs" entry for "Worksheets" *and* the "TitlesOfParts" vector size
	 * @param increment change the sheet count by this (negative = decrement)
	 * @throws XLInternalError when sheet count would become < 1
	 */
	void incrementSheetCount(int16_t increment);

	/**
	 * @brief initialize <TitlesOfParts> to contain all and only entries from workbookSheetNames & ensure HeadingPairs entry for Worksheets has the correct count
	 * @param workbookSheetNames the vector of sheet names as returned by XLWorkbook::sheetNames()
	 * @throws XLInternalError thrown by the underlying sheetNames call upon failure
	 */
	void alignWorksheets(std::vector<std::string> const & workbookSheetNames);

	/**
	 * @brief
	 * @param title
	 * @return
	 */
	void addSheetName(const std::string& title);

	/**
	 * @brief
	 * @param title
	 */
	void deleteSheetName(const std::string& title);

	/**
	 * @brief
	 * @param oldTitle
	 * @param newTitle
	 */
	void setSheetName(const std::string& oldTitle, const std::string& newTitle);

	/**
	 * @brief
	 * @param name
	 * @param value
	 */
	void addHeadingPair(const std::string& name, int value);

	/**
	 * @brief
	 * @param name
	 */
	void deleteHeadingPair(const std::string& name);

	/**
	 * @brief
	 * @param name
	 * @param newValue
	 */
	void setHeadingPair(const std::string& name, int newValue);

	/**
	 * @brief
	 * @param name
	 * @param value
	 * @return
	 */
	void setProperty(const std::string& name, const std::string& value);

	/**
	 * @brief
	 * @param name
	 * @return
	 */
	std::string property(const std::string& name) const;

	/**
	 * @brief
	 * @param name
	 */
	void deleteProperty(const std::string& name);

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	void appendSheetName(const std::string& sheetName);

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	void prependSheetName(const std::string& sheetName);

	/**
	 * @brief
	 * @param sheetName
	 * @param index
	 * @return
	 */
	void insertSheetName(const std::string& sheetName, unsigned int index);
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLPROPERTIES_HPP
