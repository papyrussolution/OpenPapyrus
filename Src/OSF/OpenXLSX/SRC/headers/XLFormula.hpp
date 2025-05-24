// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLFORMULA_HPP
#define OPENXLSX_XLFORMULA_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <iostream>
#include <string>
#include <variant>
#include "XLXmlParser.hpp"

// ========== CLASS AND ENUM TYPE DEFINITIONS ========== //
namespace OpenXLSX {
constexpr bool XLResetValue    = true;
constexpr bool XLPreserveValue = false;

//---------- Forward Declarations ----------//
class XLFormulaProxy;
class XLCell;

/**
 * @brief The XLFormula class encapsulates the concept of an Excel formula. The class is essentially
 * a wrapper around a std::string.
 * @warning This class currently only supports simple formulas. Array formulas and shared formulas are
 * not supported. Unfortunately, many spreadsheets have shared formulas, so this class is probably
 * best used for adding formulas, not reading them from an existing spreadsheet.
 * @todo Enable handling of shared and array formulas.
 */
class XLFormula {
	friend bool operator==(const XLFormula& lhs, const XLFormula& rhs);
	friend bool operator!=(const XLFormula& lhs, const XLFormula& rhs);
	friend std::ostream& operator<<(std::ostream& os, const XLFormula& value);
public:
	XLFormula();
	/**
	 * @brief Constructor, taking a string-type argument
	 * @tparam T Type of argument used. Must be string-type.
	 * @param formula The formula to initialize the object with.
	 */
	template <typename T, typename = std::enable_if_t<
		    std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*> > >
	explicit XLFormula(T formula)
	{
		// ===== If the argument is a const char *, use the argument directly; otherwise, assume it has a .c_str() function.
		if constexpr(std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>)
			m_formulaString = formula;
		else if constexpr(std::is_same_v<std::decay_t<T>, std::string_view>)
			m_formulaString = std::string(formula);
		else
			m_formulaString = formula.c_str();
	}
	XLFormula(const XLFormula& other);
	XLFormula(XLFormula&& other) noexcept;
	~XLFormula();
	/**
	 * @brief Copy assignment operator.
	 * @param other Object to be copied.
	 * @return Reference to copied-to object.
	 */
	XLFormula& operator=(const XLFormula& other);
	/**
	 * @brief Move assignment operator.
	 * @param other Object to be moved.
	 * @return Reference to moved-to object.
	 */
	XLFormula& operator=(XLFormula&& other) noexcept;

	/**
	 * @brief Templated assignment operator, taking a string-type object as an argument.
	 * @tparam T Type of argument (only string-types are allowed).
	 * @param formula String containing the formula.
	 * @return Reference to the assigned-to object.
	 */
	template <typename T, typename = std::enable_if_t<
		    std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*> > >
	XLFormula& operator=(T formula)
	{
		XLFormula temp(formula);
		std::swap(*this, temp);
		return *this;
	}
	/**
	 * @brief Templated setter function, taking a string-type object as an argument.
	 * @tparam T Type of argument (only string-types are allowed).
	 * @param formula String containing the formula.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*> > >
	void set(T formula)
	{
		*this = formula;
	}
	/**
	 * @brief Get the formula as a std::string.
	 * @return A std::string with the formula.
	 */
	std::string get() const;
	/**
	 * @brief Conversion operator, for converting object to a std::string.
	 * @return The formula as a std::string.
	 */
	operator std::string() const;    // NOLINT
	/**
	 * @brief Clear the formula.
	 * @return Return a reference to the cleared object.
	 */
	XLFormula& clear();
private:
	std::string m_formulaString; /**< A std::string, holding the formula string.*/
};
/**
 * @brief The XLFormulaProxy serves as a placeholder for XLFormula objects. This enable
 * getting and setting formulas through the same interface.
 */
class XLFormulaProxy {
	friend class XLCell;
	friend class XLFormula;
public:
	~XLFormulaProxy();
	XLFormulaProxy& operator=(const XLFormulaProxy& other);
	/**
	 * @brief Templated assignment operator, taking a string-type argument.
	 * @tparam T Type of argument.
	 * @param formula The formula string to be assigned.
	 * @return A reference to the copied-to object.
	 */
	template <typename T,
		typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, XLFormula> || std::is_same_v<std::decay_t<T>, std::string> ||
		std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		std::is_same_v<std::decay_t<T>, char*> > >
	XLFormulaProxy& operator=(T formula)
	{
		if constexpr(std::is_same_v<std::decay_t<T>, XLFormula>)
			setFormulaString(formula.get().c_str());
		else if constexpr(std::is_same_v<std::decay_t<T>, std::string>)
			setFormulaString(formula.c_str());
		else if constexpr(std::is_same_v<std::decay_t<T>, std::string_view>)
			setFormulaString(std::string(formula).c_str());
		else
			setFormulaString(formula);
		return *this;
	}
	/**
	 * @brief Templated setter, taking a string-type argument.
	 * @tparam T Type of argument.
	 * @param formula The formula string to be assigned.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*> > >
	void set(T formula)
	{
		*this = formula;
	}
	/**
	 * @brief Get the formula as a std::string.
	 * @return A std::string with the formula.
	 */
	std::string get() const;
	/**
	 * @brief Clear the formula.
	 * @return Return a reference to the cleared object.
	 */
	XLFormulaProxy& clear();

	/**
	 * @brief Conversion operator, for converting the object to a std::string.
	 * @return The formula as a std::string.
	 */
	operator std::string() const;    // NOLINT

	/**
	 * @brief Implicit conversion to XLFormula object.
	 * @return Returns the corresponding XLFormula object.
	 */
	operator XLFormula() const;    // NOLINT
private:
	/**
	 * @brief Constructor, taking pointers to the cell and cell node objects.
	 * @param cell Pointer to the associated cell object.
	 * @param cellNode Pointer to the associated cell node object.
	 */
	XLFormulaProxy(XLCell* cell, XMLNode* cellNode);
	XLFormulaProxy(const XLFormulaProxy& other);
	XLFormulaProxy(XLFormulaProxy&& other) noexcept;

	/**
	 * @brief Move assignment operator.
	 * @param other Object to be moved.
	 * @return A reference to the moved-to object.
	 */
	XLFormulaProxy& operator=(XLFormulaProxy&& other) noexcept;

	/**
	 * @brief Set the formula to the given string.
	 * @param formulaString String holding the formula.
	 * @param resetValue if true (XLResetValue), the cell value will be set to 0, if false (XLPreserveValue), it will remain unchanged
	 */
	void setFormulaString(const char* formulaString, bool resetValue = XLResetValue);

	/**
	 * @brief Get the underlying XLFormula object.
	 * @return A XLFormula object.
	 * @throw XLFormulaError if the formula is of 'shared' or 'array' types.
	 */
	XLFormula getFormula() const;

	//---------- Private Member Variables ---------- //
	XLCell*  m_cell;     /**< Pointer to the owning XLCell object. */
	XMLNode* m_cellNode; /**< Pointer to corresponding XML cell node. */
};
}    // namespace OpenXLSX

// ========== FRIEND FUNCTION IMPLEMENTATIONS ========== //
namespace OpenXLSX {
inline bool operator==(const XLFormula& lhs, const XLFormula& rhs) { return lhs.m_formulaString == rhs.m_formulaString; }
inline bool operator!=(const XLFormula& lhs, const XLFormula& rhs) { return lhs.m_formulaString != rhs.m_formulaString; }
/**
 * @brief send a formula string to an ostream
 * @param os the output destination
 * @param value the formula to send to os
 * @return
 */
inline std::ostream& operator<<(std::ostream& os, const XLFormula& value) { return os << value.m_formulaString; }
/**
 * @brief send a formula string to an ostream
 * @param os the output destination
 * @param value the formula proxy whose formula to send to os
 * @return
 */
inline std::ostream& operator<<(std::ostream& os, const XLFormulaProxy& value) { return os << value.get(); }
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLFORMULA_HPP
