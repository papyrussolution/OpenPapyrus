// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLWORKBOOK_HPP
#define OPENXLSX_XLWORKBOOK_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <ostream>    // std::basic_ostream
#include <vector>
#include "XLXmlFile.hpp"

namespace OpenXLSX {
// class XLSharedStrings;
class XLSheet;
class XLWorksheet;
class XLChartsheet;
/**
 * @brief The XLSheetType class is an enumeration of the available sheet types, e.g. Worksheet (ordinary
 * spreadsheets), and Chartsheet (sheets with only a chart).
 */
enum class XLSheetType { 
	Worksheet, 
	Chartsheet, 
	Dialogsheet, 
	Macrosheet 
};

/**
 * @brief This class encapsulates the concept of a Workbook. It provides access to the individual sheets
 * (worksheets or chartsheets), as well as functionality for adding, deleting, moving and renaming sheets.
 */
class XLWorkbook : public XLXmlFile {
	friend class XLSheet;
	friend class XLDocument;

public:        // ---------- Public Member Functions ---------- //
	/**
	 * @brief Default constructor. Creates an empty ('null') XLWorkbook object.
	 */
	XLWorkbook() = default;

	/**
	 * @brief Constructor. Takes a pointer to an XLXmlData object (stored in the parent XLDocument object).
	 * @param xmlData A pointer to the underlying XLXmlData object, which holds the XML data.
	 * @note Do not create an XLWorkbook object directly. Access via XLDocument::workbook().
	 */
	explicit XLWorkbook(XLXmlData* xmlData);

	/**
	 * @brief Copy Constructor.
	 * @param other The XLWorkbook object to be copied.
	 * @note The copy constructor has been explicitly defaulted.
	 */
	XLWorkbook(const XLWorkbook& other) = default;

	/**
	 * @brief Move constructor.
	 * @param other The XLWorkbook to be moved.
	 * @note The move constructor has been explicitly defaulted.
	 */
	XLWorkbook(XLWorkbook&& other) = default;

	/**
	 * @brief Destructor
	 * @note Default destructor specified
	 */
	~XLWorkbook();

	/**
	 * @brief Copy assignment operator.
	 * @param other The XLWorkbook object to be assigned to the current.
	 * @return A reference to *this
	 * @note The copy assignment operator has been explicitly deleted, as XLWorkbook objects should not be copied.
	 */
	XLWorkbook& operator=(const XLWorkbook& other) = default;

	/**
	 * @brief Move assignment operator.
	 * @param other The XLWorkbook to be move assigned.
	 * @return A reference to *this
	 * @note The move assignment operator has been explicitly deleted, as XLWorkbook objects should not be moved.
	 */
	XLWorkbook& operator=(XLWorkbook&& other) = default;

	/**
	 * @brief Get the sheet (worksheet or chartsheet) at the given index.
	 * @param index The index at which the desired sheet is located.
	 * @return A pointer to an XLAbstractSheet with the sheet at the index.
	 * @note The index must be 1-based (rather than 0-based) as this is the default for Excel spreadsheets.
	 */
	XLSheet sheet(uint16_t index);

	/**
	 * @brief Get the sheet (worksheet or chartsheet) with the given name.
	 * @param sheetName The name of the desired sheet.
	 * @return A pointer to an XLAbstractSheet with the sheet at the index.
	 */
	XLSheet sheet(const std::string& sheetName);

	/**
	 * @brief Get the worksheet with the given name.
	 * @param sheetName The name of the desired worksheet.
	 * @return
	 */
	XLWorksheet worksheet(const std::string& sheetName);

	/**
	 * @brief Get the worksheet at the given index.
	 * @param index The index (1-based) at which the desired sheet is located.
	 * @return
	 */
	XLWorksheet worksheet(uint16_t index);

	/**
	 * @brief Get the chartsheet with the given name.
	 * @param sheetName The name of the desired chartsheet.
	 * @return
	 */
	XLChartsheet chartsheet(const std::string& sheetName);

	/**
	 * @brief Get the chartsheet at the given index.
	 * @param index The index (1-based) at which the desired sheet is located.
	 * @return
	 */
	XLChartsheet chartsheet(uint16_t index);

	/**
	 * @brief Delete sheet (worksheet or chartsheet) from the workbook.
	 * @param sheetName Name of the sheet to delete.
	 * @throws XLException An exception will be thrown if trying to delete the last worksheet in the workbook
	 * @warning A workbook must contain at least one worksheet. Trying to delete the last worksheet from the
	 * workbook will trow an exception.
	 */
	void deleteSheet(const std::string& sheetName);

	/**
	 * @brief
	 * @param sheetName
	 */
	void addWorksheet(const std::string& sheetName);

	/**
	 * @brief
	 * @param existingName
	 * @param newName
	 */
	void cloneSheet(const std::string& existingName, const std::string& newName);

	/**
	 * @brief
	 * @param sheetName
	 * @param index The index (1-based) where the sheet shall be moved to
	 */
	void setSheetIndex(const std::string& sheetName, unsigned int index);

	/**
	 * @brief
	 * @param sheetName
	 * @return The index (1-based) of the sheet with sheetName
	 */
	unsigned int indexOfSheet(const std::string& sheetName) const;

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	XLSheetType typeOfSheet(const std::string& sheetName) const;

	/**
	 * @brief
	 * @param index The index (1-based) at which the desired sheet is located.
	 * @return
	 */
	XLSheetType typeOfSheet(unsigned int index) const;

	/**
	 * @brief
	 * @return
	 */
	unsigned int sheetCount() const;

	/**
	 * @brief
	 * @return
	 */
	unsigned int worksheetCount() const;

	/**
	 * @brief
	 * @return
	 */
	unsigned int chartsheetCount() const;

	/**
	 * @brief
	 * @return
	 */
	std::vector<std::string> sheetNames() const;

	/**
	 * @brief
	 * @return
	 */
	std::vector<std::string> worksheetNames() const;

	/**
	 * @brief
	 * @return
	 */
	std::vector<std::string> chartsheetNames() const;

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	bool sheetExists(const std::string& sheetName) const;

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	bool worksheetExists(const std::string& sheetName) const;

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	bool chartsheetExists(const std::string& sheetName) const;

	/**
	 * @brief
	 * @param oldName
	 * @param newName
	 */
	void updateSheetReferences(const std::string& oldName, const std::string& newName);

	// /**
	//  * @brief
	//  * @return
	//  */
	// XLSharedStrings sharedStrings();
	//
	// /**
	//  * @brief
	//  * @return
	//  */
	// bool hasSharedStrings() const;
	//
	/**
	 * @brief
	 */
	void deleteNamedRanges();

	/**
	 * @brief set a flag to force full calculation upon loading the file in Excel
	 */
	void setFullCalculationOnLoad();

	/**
	 * @brief print the XML contents of the workbook.xml using the underlying XMLNode print function
	 */
	void print(std::basic_ostream<char>& ostr) const;

private:        // ---------- Private Member Functions ---------- //
	/**
	 * @brief
	 * @return
	 */
	uint16_t createInternalSheetID();

	/**
	 * @brief
	 * @param sheetName
	 * @return
	 */
	std::string sheetID(const std::string& sheetName);

	/**
	 * @brief
	 * @param sheetID
	 * @return
	 */
	std::string sheetName(const std::string& sheetID) const;

	/**
	 * @brief
	 * @param sheetID
	 * @return
	 */
	std::string sheetVisibility(const std::string& sheetID) const;

	/**
	 * @brief
	 * @param sheetName
	 * @param internalID
	 */
	void prepareSheetMetadata(const std::string& sheetName, uint16_t internalID);

	/**
	 * @brief
	 * @param sheetRID
	 * @param newName
	 */
	void setSheetName(const std::string& sheetRID, const std::string& newName);

	/**
	 * @brief
	 * @param sheetRID
	 * @param state
	 */
	void setSheetVisibility(const std::string& sheetRID, const std::string& state);

	/**
	 * @brief
	 * @param sheetRID
	 * @return
	 */
	bool sheetIsActive(const std::string& sheetRID) const;

	/**
	 * @brief
	 * @param sheetRID
	 * @return true if sheed with sheedRID could be set to active (or was already active), otherwise false
	 */
	bool setSheetActive(const std::string& sheetRID);

	/**
	 * @brief Check whether attribute string state matches a value that is considered not visible
	 * @param state
	 * @return true if state does not match a value that is considered not visible (hidden, veryHidden), otherwise false
	 */
	bool isVisibleState(std::string const& state) const;

	/**
	 * @brief Check whether sheetNode is not empty, and in case it has an attribute "state", that the state does not reflect hidden-ness
	 * @param sheetNode
	 * @return true if sheetNode can be considered visible (and could be activated)
	 */
	bool isVisible(XMLNode const& sheetNode) const;
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLWORKBOOK_HPP
