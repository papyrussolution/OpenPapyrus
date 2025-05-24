// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLTABLES_HPP
#define OPENXLSX_XLTABLES_HPP

#include <cstdint>
#include <ostream>
// #include <type_traits>
// #include <variant>
// #include "XLDocument.hpp"
#include "XLException.hpp"
#include "XLXmlData.hpp"
#include "XLXmlFile.hpp"

namespace OpenXLSX {
/**
 * @brief The XLTables class is the base class for worksheet tables
 */
class XLTables : public XLXmlFile {
	friend class XLWorksheet;   // for access to XLXmlFile::getXmlPath
public:
	/**
	 * @brief Constructor
	 */
	XLTables() : XLXmlFile(nullptr) {};

	/**
	 * @brief The constructor.
	 * @param xmlData the source XML of the table file
	 */
	XLTables(XLXmlData* xmlData);

	/**
	 * @brief The copy constructor.
	 * @param other The object to be copied.
	 * @note The default copy constructor is used, i.e. only shallow copying of pointer data members.
	 */
	XLTables(const XLTables& other) = default;

	/**
	 * @brief
	 * @param other
	 */
	XLTables(XLTables&& other) noexcept = default;

	/**
	 * @brief The destructor
	 * @note The default destructor is used, since cleanup of pointer data members is not required.
	 */
	~XLTables() = default;

	/**
	 * @brief Assignment operator
	 * @return A reference to the new object.
	 * @note The default assignment operator is used, i.e. only shallow copying of pointer data members.
	 */
	XLTables& operator=(const XLTables&) = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLTables& operator=(XLTables&& other) noexcept = default;

	// /**
	//  * @brief getters
	//  */
	// std::string get(std::string cellRef) const;
	//
	// /**
	//  * @brief setters
	//  */
	// bool set(std::string cellRef);

	/**
	 * @brief Print the XML contents of this XLTables instance using the underlying XMLNode print function
	 */
	void print(std::basic_ostream<char>& ostr) const;
};
}    // namespace OpenXLSX

#endif    // OPENXLSX_XLTABLES_HPP
