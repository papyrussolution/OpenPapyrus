// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLSHAREDSTRINGS_HPP
#define OPENXLSX_XLSHAREDSTRINGS_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <deque>
#include <functional> // std::reference_wrapper
#include <limits>     // std::numeric_limits
#include <ostream>    // std::basic_ostream
#include <string>
#include "XLXmlFile.hpp"

namespace OpenXLSX {
constexpr size_t XLMaxSharedStrings = (std::numeric_limits< int32_t >::max)();       // pull request #261: wrapped max in parentheses to prevent expansion of windows.h "max" macro

class XLSharedStrings;     // forward declaration
typedef std::reference_wrapper< const XLSharedStrings > XLSharedStringsRef;

extern const XLSharedStrings XLSharedStringsDefaulted;     // to be used for default initialization of all references of type XLSharedStrings

/**
 * @brief This class encapsulate the Excel concept of Shared Strings. In Excel, instead of havig individual strings
 * in each cell, cells have a reference to an entry in the SharedStrings register. This results in smalle file
 * sizes, as repeated strings are referenced easily.
 */
class XLSharedStrings : public XLXmlFile
{
	//---------- Friend Declarations ----------//
	friend class XLDocument; // for access to protected function rewriteXmlFromCache

	//----------------------------------------------------------------------------------------------------------------------
	//           Public Member Functions
	//----------------------------------------------------------------------------------------------------------------------

public:
	/**
	 * @brief
	 */
	XLSharedStrings() = default;

	/**
	 * @brief
	 * @param xmlData
	 * @param stringCache
	 */
	explicit XLSharedStrings(XLXmlData* xmlData, std::deque<std::string>* stringCache);

	/**
	 * @brief Destructor
	 */
	~XLSharedStrings();

	/**
	 * @brief
	 * @param other
	 */
	XLSharedStrings(const XLSharedStrings& other) = default;

	/**
	 * @brief
	 * @param other
	 */
	XLSharedStrings(XLSharedStrings&& other) noexcept = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLSharedStrings& operator=(const XLSharedStrings& other) = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLSharedStrings& operator=(XLSharedStrings&& other) noexcept = default;

	/**
	 * @brief return the amount of shared string entries currently in the cache
	 * @return
	 */
	int32_t stringCount() const { return m_stringCache->size(); }

	/**
	 * @brief
	 * @param str
	 * @return
	 */
	int32_t getStringIndex(const std::string& str) const;

	/**
	 * @brief
	 * @param str
	 * @return
	 */
	bool stringExists(const std::string& str) const;

	/**
	 * @brief
	 * @param index
	 * @return
	 */
	const char* getString(int32_t index) const;

	/**
	 * @brief Append a new string to the list of shared strings.
	 * @param str The string to append.
	 * @return An int32_t with the index of the appended string
	 */
	int32_t appendString(const std::string& str) const;

	/**
	 * @brief Clear the string at the given index.
	 * @param index The index to clear.
	 * @note There is no 'deleteString' member function, as deleting a shared string node will invalidate the
	 * shared string indices for the cells in the spreadsheet. Instead use this member functions, which clears
	 * the contents of the string, but keeps the XMLNode holding the string.
	 */
	void clearString(int32_t index) const;

	// 2024-06-18 TBD if this is ever needed
	// /**
	//  * @brief check m_stringCache is initialized
	//  * @return true if m_stringCache != nullptr, false otherwise
	//  * @note 2024-05-28 added function to enable other classes to check m_stringCache status
	//  */
	// bool initialized() const { return m_stringCache != nullptr; }

	/**
	 * @brief print the XML contents of the shared strings document using the underlying XMLNode print function
	 */
	void print(std::basic_ostream<char>& ostr) const;

protected:
	/**
	 * @brief clear & rewrite the full shared strings XML from the shared strings cache
	 * @return the amount of strings written to XML (should be equal to m_stringCache->size())
	 */
	int32_t rewriteXmlFromCache();

private:
	std::deque<std::string>* m_stringCache {}; /** < Each string must have an unchanging memory address; hence the use of std::deque */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLSHAREDSTRINGS_HPP
