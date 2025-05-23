// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCELLVALUE_HPP
#define OPENXLSX_XLCELLVALUE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include "XLDateTime.hpp"
#include "XLException.hpp"
#include "XLXmlParser.hpp"

typedef std::variant<std::string, int64_t, double, bool> XLCellValueType; // TBD: typedef std::variant< std::string, int64_t, double, bool, struct timestamp > XLCellValueType;

// ========== CLASS AND ENUM TYPE DEFINITIONS ========== //
namespace OpenXLSX {
//---------- Forward Declarations ----------//
class XLCellValueProxy;
class XLCell;

/**
 * @brief Enum defining the valid value types for a an Excel spreadsheet cell.
 */
enum class XLValueType { Empty, Boolean, Integer, Float, Error, String };

//---------- Private Struct to enable XLValueType conversion to double ---------- //
struct VisitXLCellValueTypeToDouble {
	std::string packageName = "VisitXLCellValueTypeToDouble";
	double operator()(int64_t v) const { return static_cast<double>(v); }
	double operator()(double v) const { return v; }
	double operator()(bool v) const { return v; }
	// double operator()( struct timestamp v ) { /* to be implemented if this type ever gets supported */ }
	double operator()(std::string v) const 
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
};

//---------- Private Struct to enable XLValueType conversion to std::string ---------- //
struct VisitXLCellValueTypeToString {
	std::string packageName = "VisitXLCellValueTypeToString";
	std::string operator()(int64_t v) const { return std::to_string(v); }
	std::string operator()(double v) const { return std::to_string(v); }
	std::string operator()(bool v) const { return v ? "true" : "false"; }
	// std::string operator()( struct timestamp v ) { return timestampString( v.seconds, v.microseconds, WITH_MS ); }
	std::string operator()(std::string v) const { return v; }
};

/**
 * @brief Class encapsulating a cell value.
 */
class XLCellValue {
	//---------- Friend Declarations ----------//
	friend class XLCellValueProxy;    // to allow access to m_value
	// TODO: Consider template functions to compare to ints, floats etc.
	friend bool operator==(const XLCellValue& lhs, const XLCellValue& rhs);
	friend bool operator!=(const XLCellValue& lhs, const XLCellValue& rhs);
	friend bool operator<(const XLCellValue& lhs, const XLCellValue& rhs);
	friend bool operator>(const XLCellValue& lhs, const XLCellValue& rhs);
	friend bool operator<=(const XLCellValue& lhs, const XLCellValue& rhs);
	friend bool operator>=(const XLCellValue& lhs, const XLCellValue& rhs);
	friend std::ostream& operator<<(std::ostream& os, const XLCellValue& value);
	friend std::hash<OpenXLSX::XLCellValue>;
public:
	/**
	 * @brief Default constructor
	 */
	XLCellValue();
	/**
	 * @brief A templated constructor. Any value convertible to a valid cell value can be used as argument.
	 * @tparam T The type of the argument (will be automatically deduced).
	 * @param value The value.
	 * @todo Consider changing the enable_if statement to check for objects with a .c_str() member function.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	XLCellValue(T value)    // NOLINT
	{
		// ===== If the argument is a bool, set the m_type attribute to Boolean.
		if constexpr(std::is_integral_v<T> && std::is_same_v<T, bool>) {
			m_type  = XLValueType::Boolean;
			m_value = value;
		}
		// ===== If the argument is an integral type, set the m_type attribute to Integer.
		else if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>) {
			m_type  = XLValueType::Integer;
			m_value = static_cast<int64_t>(value);
		}
		// ===== If the argument is a string type (i.e. is constructable from *char),
		// ===== set the m_type attribute to String.
		else if constexpr(std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> ||
		    (std::is_same_v<std::decay_t<T>, char*> && !std::is_same_v<T, bool>)) {
			m_type  = XLValueType::String;
			m_value = std::string(value);
		}
		// ===== If the argument is an XLDateTime, set the value to the date/time serial number.
		else if constexpr(std::is_same_v<T, XLDateTime>) {
			m_type  = XLValueType::Float;
			m_value = value.serial();
		}
		// ===== If the argument is a floating point type, set the m_type attribute to Float.
		// ===== If not, a static_assert will result in compilation error.
		else {
			static_assert(std::is_floating_point_v<T>, "Invalid argument for constructing XLCellValue object");
			if(std::isfinite(value)) {
				m_type  = XLValueType::Float;
				m_value = static_cast<double>(value);
			}
			else {
				m_type  = XLValueType::Error;
				m_value = std::string("#NUM!");
			}
		}
	}
	XLCellValue(const XLCellValue& other);
	XLCellValue(XLCellValue&& other) noexcept;
	~XLCellValue();
	/**
	 * @brief Copy assignment operator.
	 * @param other Object to be copied.
	 * @return Reference to the copied-to object.
	 */
	XLCellValue& operator=(const XLCellValue& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Object to be moved.
	 * @return Reference to the moved-to object.
	 */
	XLCellValue& operator=(XLCellValue&& other) noexcept;

	/**
	 * @brief Templated assignment operator.
	 * @tparam T The type of the value argument.
	 * @param value The value.
	 * @return A reference to the assigned-to object.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	XLCellValue& operator=(T value)
	{
		// ===== Implemented using copy-and-swap.
		XLCellValue temp(value);
		std::swap(*this, temp);
		return *this;
	}

	/**
	 * @brief Templated setter for integral and bool types.
	 * @tparam T The type of the value argument.
	 * @param numberValue The value
	 */
	template <
		typename T,
		typename = std::enable_if_t<std::is_same_v<T, XLCellValue> || std::is_integral_v<T> || std::is_floating_point_v<T> ||
		std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*> ||
		std::is_same_v<T, XLDateTime> > >
	void set(T numberValue)
	{
		// ===== Implemented using the assignment operator.
		*this = numberValue;
	}

	/**
	 * @brief Templated getter.
	 * @tparam T The type of the value to be returned.
	 * @return The value as a type T object.
	 * @throws XLValueTypeError if the XLCellValue object does not contain a compatible type.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	T get() const
	{
		try {
			// BUGFIX 2025-01-10: can not return const char* of a temporary object - use a static variable as workaround
			// CAUTION: This is not thread-safe and this template return type really shouldn't be used.
			//          Accordingly, an exception should be thrown in the future
			if constexpr(std::is_same_v<std::decay_t<T>, std::string_view> ||
			    std::is_same_v<std::decay_t<T>, const char*> ||
			    (std::is_same_v<std::decay_t<T>, char*> && !std::is_same_v<T, bool>)) {
				// throw XLValueTypeError("(temporary) XLCellValue should not be requested as a reference type (string_view, (const) char*) - please fetch std::string");
				static std::string s = std::get<std::string>(m_value);
				return s.c_str();
			}
			// for all other template types, use the private getter:
			return privateGet<T>();
		}

		catch(const std::bad_variant_access&) {
			throw XLValueTypeError("XLCellValue object does not contain the requested type.");
		}
	}

private:
	/**
	 * @brief private templated getter - only to be used by functions that know for certain that *this is valid until the return value has been used
	 * @tparam T The type of the value to be returned.
	 * @return The value as a type T object.
	 * @throws XLValueTypeError if the XLCellValue object does not contain a compatible type.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	T privateGet() const
	{
		try {
			if constexpr(std::is_integral_v<T> && std::is_same_v<T, bool>)  return std::get<bool>(m_value);

			if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>)  return static_cast<T>(std::get<int64_t>(m_value));

			if constexpr(std::is_floating_point_v<T>) {
				return static_cast<T>(getDouble()); // 2025-01-10: allow implicit conversion of int and bool to double (string conversion disabled for now)
				// if (m_type == XLValueType::Error) return static_cast<T>(std::nan("1"));
				// return static_cast<T>(std::get<double>(m_value));
			}

			if constexpr(std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
			    std::is_same_v<std::decay_t<T>, const char*> ||
			    (std::is_same_v<std::decay_t<T>, char*> && !std::is_same_v<T, bool>))
				return std::get<std::string>(m_value).c_str();

			if constexpr(std::is_same_v<T, XLDateTime>)
				return XLDateTime(getDouble()); // 2025-01-10: allow implicit conversion of int and bool to double (string conversion disabled for now)
		}

		catch(const std::bad_variant_access&) {
			throw XLValueTypeError("XLCellValue object does not contain the requested type.");
		}
	}

public:

	/**
	 * @brief get the cell value as a double, regardless of value type
	 * @return A double representation of value
	 * @throws XLValueTypeError if the XLCellValue object is not convertible to double.
	 */
	double getDouble() const {
		if(m_type == XLValueType::Error)  return static_cast<double>(std::nan("1"));
		try {
			return std::visit(VisitXLCellValueTypeToDouble(), m_value);
		}
		catch(...) {
			throw XLValueTypeError("XLCellValue object is not convertible to double.");
		}
	}

	/**
	 * @brief get the cell value as a std::string, regardless of value type
	 * @return A std::string representation of value
	 * @throws XLValueTypeError if the XLCellValue object is not convertible to string.
	 */
	std::string getString()    // pull request #158 is covered by this
	{
		try {
			return std::visit(VisitXLCellValueTypeToString(), m_value);
		}
		catch(...) { // 2024-05-27: was catch( string s ) - must have been a typo, currently nothing throws a string here
			throw XLValueTypeError("XLCellValue object is not convertible to string.");
		}
	}
	/**
	 * @brief get the cell value as a std::variant of XLCellValueType
	 * @return a const reference to m_value
	 */
	const XLCellValueType& getVariant() const { return m_value; } // pull request #127
	/**
	 * @brief Explicit conversion operator for easy conversion to supported types.
	 * @tparam T The type to cast to.
	 * @return The XLCellValue object cast to requested type.
	 * @throws XLValueTypeError if the XLCellValue object does not contain a compatible type.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	operator T() const
	{
		return this->get<T>();
	}

	/**
	 * @brief Clears the contents of the XLCellValue object.
	 * @return Returns a reference to the current object.
	 */
	XLCellValue& clear();
	/**
	 * @brief Sets the value type to XLValueType::Error.
	 * @return Returns a reference to the current object.
	 */
	XLCellValue& setError(const std::string& error);
	/**
	 * @brief Get the value type of the current object.
	 * @return An XLValueType for the current object.
	 */
	XLValueType type() const;
	/**
	 * @brief Get the value type of the current object, as a string representation
	 * @return A std::string representation of the value type.
	 */
	std::string typeAsString() const;
private:
	//---------- Private Member Variables ---------- //

	XLCellValueType m_value { std::string("") };   /**< The value contained in the cell. */
	XLValueType m_type { XLValueType::Empty };     /**< The value type of the cell. */
};

/**
 * @brief The XLCellValueProxy class is used for proxy (or placeholder) objects for XLCellValue objects.
 * @details The purpose is to enable implicit conversion during assignment operations. XLCellValueProxy objects
 * can not be constructed manually by the user, only through XLCell objects.
 */
class XLCellValueProxy {
	friend class XLCell;
	friend class XLCellValue;
	friend class XLDocument; // for reindexing shared strings
public:
	/**
	 * @brief Destructor
	 */
	~XLCellValueProxy();
	/**
	 * @brief Copy assignment operator.
	 * @param other XLCellValueProxy object to be copied.
	 * @return A reference to the current object.
	 */
	XLCellValueProxy& operator=(const XLCellValueProxy& other);

	/**
	 * @brief Templated assignment operator
	 * @tparam T The type of numberValue assigned to the object.
	 * @param value The value.
	 * @return A reference to the current object.
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLCellValue> || std::is_same_v<T, XLDateTime> > >
	XLCellValueProxy& operator=(T value)
	{    // NOLINT
		if constexpr(std::is_integral_v<T> && std::is_same_v<T, bool>) // if bool
			setBoolean(value);

		else if constexpr(std::is_integral_v<T> && !std::is_same_v<T, bool>) // if integer
			setInteger(value);

		else if constexpr(std::is_floating_point_v<T>) // if floating point
			setFloat(value);

		else if constexpr(std::is_same_v<T, XLDateTime>)
			setFloat(value.serial());

		else if constexpr(std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> ||
		    std::is_same_v<std::decay_t<T>, const char*> ||
		    (std::is_same_v<std::decay_t<T>, char*> && !std::is_same_v<T, bool> && !std::is_same_v<T, XLCellValue>)) {
			if constexpr(std::is_same_v<std::decay_t<T>, const char*> || std::is_same_v<std::decay_t<T>, char*>)
				setString(value);
			else if constexpr(std::is_same_v<std::decay_t<T>, std::string_view>)
				setString(std::string(value).c_str());
			else
				setString(value.c_str());
		}

		if constexpr(std::is_same_v<T, XLCellValue>) {
			switch(value.type()) {
				case XLValueType::Boolean:
				    setBoolean(value.template get<bool>());
				    break;
				case XLValueType::Integer:
				    setInteger(value.template get<int64_t>());
				    break;
				case XLValueType::Float:
				    setFloat(value.template get<double>());
				    break;
				case XLValueType::String:
				    setString(value.template privateGet<const char*>());
				    break;
				case XLValueType::Empty:
				    clear();
				    break;
				default:
				    setError("#N/A");
				    break;
			}
		}

		return *this;
	}

	/**
	 * @brief
	 * @tparam T
	 * @param value
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLCellValue> || std::is_same_v<T, XLDateTime> > >
	void set(T value)
	{
		*this = value;
	}

	/**
	 * @brief
	 * @tparam T
	 * @return
	 * @todo Is an explicit conversion operator needed as well?
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	T get() const
	{
		return getValue().get<T>();
	}

	/**
	 * @brief Clear the contents of the cell.
	 * @return A reference to the current object.
	 */
	XLCellValueProxy& clear();

	/**
	 * @brief Set the cell value to a error state.
	 * @return A reference to the current object.
	 */
	XLCellValueProxy& setError(const std::string& error);

	/**
	 * @brief Get the value type for the cell.
	 * @return An XLCellValue corresponding to the cell value.
	 */
	XLValueType type() const;

	/**
	 * @brief Get the value type of the current object, as a string representation
	 * @return A std::string representation of the value type.
	 */
	std::string typeAsString() const;

	/**
	 * @brief Implicitly convert the XLCellValueProxy object to a XLCellValue object.
	 * @return An XLCellValue object, corresponding to the cell value.
	 */
	operator XLCellValue() const;    // NOLINT

	/**
	 * @brief
	 * @tparam T
	 * @return
	 */
	template <typename T,
	    typename = std::enable_if_t<
		    std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
		    std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
		    std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
	operator T() const
	{
		return getValue().get<T>();
	}

	/**
	 * @brief get the cell value as a std::string, regardless of value type
	 * @return A std::string representation of value
	 * @throws XLValueTypeError if the XLCellValue object is not convertible to string.
	 */
	std::string getString() const    // pull request #158 is covered by this
	{
		try {
			return std::visit(VisitXLCellValueTypeToString(), getValue().m_value);
		}
		catch(std::string s) {
			throw XLValueTypeError("XLCellValue object is not convertible to string.");
		}
	}

private:
	//---------- Private Member Functions ---------- //

	/**
	 * @brief Constructor
	 * @param cell Pointer to the parent XLCell object.
	 * @param cellNode Pointer to the corresponding XMLNode object.
	 */
	XLCellValueProxy(XLCell* cell, XMLNode* cellNode);
	XLCellValueProxy(const XLCellValueProxy& other);
	XLCellValueProxy(XLCellValueProxy&& other) noexcept;
	/**
	 * @brief Move assignment operator
	 * @param other Object to be moved
	 * @return Reference to moved-to pbject.
	 */
	XLCellValueProxy& operator=(XLCellValueProxy&& other) noexcept;
	/**
	 * @brief Set cell to an integer value.
	 * @param numberValue The value to be set.
	 */
	void setInteger(int64_t numberValue);
	/**
	 * @brief Set the cell to a bool value.
	 * @param numberValue The value to be set.
	 */
	void setBoolean(bool numberValue);
	/**
	 * @brief Set the cell to a floating point value.
	 * @param numberValue The value to be set.
	 */
	void setFloat(double numberValue);
	/**
	 * @brief Set the cell to a string value.
	 * @param stringValue The value to be set.
	 */
	void setString(const char* stringValue);
	/**
	 * @brief Get a copy of the XLCellValue object for the cell.
	 * @return An XLCellValue object.
	 */
	XLCellValue getValue() const;

	/**
	 * @brief get the shared string index of value
	 * @return the index in the shared strings table
	 * @return -1 if cell value is not a shared string
	 */
	int32_t stringIndex() const;

	/**
	 * @brief directly set the shared string index for cell, bypassing XLSharedStrings
	 * @return true if newIndex could be set
	 * @return false if newIndex < 0 or value is not already a shared string
	 */
	bool setStringIndex(int32_t newIndex);

	//---------- Private Member Variables ---------- //

	XLCell*  m_cell;     /**< Pointer to the owning XLCell object. */
	XMLNode* m_cellNode; /**< Pointer to corresponding XML cell node. */
};
}    // namespace OpenXLSX

// TODO: Consider comparison operators on fundamental datatypes
// ========== FRIEND FUNCTION IMPLEMENTATIONS ========== //
namespace OpenXLSX {
inline bool operator==(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value == rhs.m_value; }
inline bool operator!=(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value != rhs.m_value; }
inline bool operator<(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value < rhs.m_value; }
inline bool operator>(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value > rhs.m_value; }
inline bool operator<=(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value <= rhs.m_value; }
inline bool operator>=(const XLCellValue& lhs, const XLCellValue& rhs) { return lhs.m_value >= rhs.m_value; }

inline std::ostream& operator<<(std::ostream& os, const XLCellValue& value)
{
	switch(value.type()) {
		case XLValueType::Empty: return os << "";
		case XLValueType::Boolean: return os << value.get<bool>();
		case XLValueType::Integer: return os << value.get<int64_t>();
		case XLValueType::Float: return os << value.get<double>();
		case XLValueType::String: return os << value.get<std::string>(); // 2025-01-10 BUGFIX: for temporary value objects, this was undefined behavior due to returning a string_view
		default: return os << "";
	}
}

inline std::ostream& operator<<(std::ostream& os, const XLCellValueProxy& value)
{
	switch(value.type()) {
		case XLValueType::Empty: return os << "";
		case XLValueType::Boolean: return os << value.get<bool>();
		case XLValueType::Integer: return os << value.get<int64_t>();
		case XLValueType::Float: return os << value.get<double>();
		case XLValueType::String: return os << value.get<std::string>(); // 2025-01-10 BUGFIX: for temporary value objects, this was undefined behavior due to returning a string_view
		default: return os << "";
	}
}
}    // namespace OpenXLSX

template <> struct std::hash<OpenXLSX::XLCellValue>  // NOLINT
{
	std::size_t operator()(const OpenXLSX::XLCellValue& value) const noexcept
	{
		return std::hash<XLCellValueType> {}(value.m_value);
	}
};

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCELLVALUE_HPP
