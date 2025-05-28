// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_OPENXLSX_HPP
#define OPENXLSX_OPENXLSX_HPP

#include <sl-packing-set_compiler_default.h>
#include <string_view>
#include <any>
#include <variant>

#include <../osf/pugixml/pugixml.hpp>
//
#ifdef _MSC_VER // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
	#pragma warning(push)
	#pragma warning(disable : 4251) // #first
	#pragma warning(disable : 4275)
#endif // _MSC_VER
	//
	//#include "XLConstants.hpp"
	namespace OpenXLSX {
		inline constexpr uint16 MAX_COLS = 16'384;
		inline constexpr uint32 MAX_ROWS = 1'048'576;
		// anchoring a comment shape below these values was not possible in LibreOffice - TBC with MS Office
		inline constexpr uint16 MAX_SHAPE_ANCHOR_COLUMN = 13067;      // column "SHO"
		inline constexpr uint32 MAX_SHAPE_ANCHOR_ROW    = 852177;
		//
		//#include "XLIterator.hpp"
		enum class XLIteratorDirection { 
			Forward, 
			Reverse 
		};
		enum class XLIteratorLocation { 
			Begin, 
			End 
		};
		//
		//#include "XLDateTime.hpp"
		class XLDateTime {
		public:
			XLDateTime();
			/**
			 * @brief Constructor taking an Excel time point serial number as an argument.
			 * @param serial Excel time point serial number.
			 */
			explicit XLDateTime(double serial);
			/**
			 * @brief Constructor taking a std::tm struct as an argument.
			 * @param timepoint A std::tm struct.
			 */
			explicit XLDateTime(const std::tm& timepoint);
			/**
			 * @brief Constructor taking a unixtime format (seconds since 1/1/1970) as an argument.
			 * @param unixtime A time_t number.
			 */
			explicit XLDateTime(time_t unixtime);
			XLDateTime(const XLDateTime& other);
			XLDateTime(XLDateTime&& other) noexcept;
			~XLDateTime();
			/**
			 * @brief Copy assignment operator.
			 * @param other Object to be copied.
			 * @return Reference to the copied-to object.
			 */
			XLDateTime& operator=(const XLDateTime& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Object to be moved.
			 * @return Reference to the moved-to object.
			 */
			XLDateTime& operator=(XLDateTime&& other) noexcept;
			/**
			 * @brief Assignment operator taking an Excel date/time serial number as an argument.
			 * @param serial A floating point value with the serial number.
			 * @return Reference to the copied-to object.
			 */
			XLDateTime& operator=(double serial);
			/**
			 * @brief Assignment operator taking a std::tm object as an argument.
			 * @param timepoint std::tm object with the time point
			 * @return Reference to the copied-to object.
			 */
			XLDateTime & operator=(const std::tm& timepoint);
			/**
			 * @brief Implicit conversion to Excel date/time serial number (any floating point type).
			 * @tparam T Type to convert to (any floating point type).
			 * @return Excel date/time serial number.
			 */
			template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T> > >
			operator T() const { return serial(); }
			operator std::tm() const;
			/**
			 * @brief Get the date/time in the form of an Excel date/time serial number.
			 * @return A double with the serial number.
			 */
			double serial() const;
			std::tm tm() const;
		private:
			double m_serial { 1.0 };
		};
		//
		//#include "XLException.hpp"
		class XLException : public std::runtime_error {
		public:
			explicit XLException(const std::string& err) : runtime_error(err) 
			{
			}
		};
		class XLOverflowError : public XLException {
		public:
			explicit XLOverflowError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLValueTypeError : public XLException {
		public:
			explicit XLValueTypeError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLCellAddressError : public XLException {
		public:
			explicit XLCellAddressError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLInputError : public XLException {
		public:
			explicit XLInputError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLInternalError : public XLException {
		public:
			explicit XLInternalError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLPropertyError : public XLException {
		public:
			explicit XLPropertyError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLSheetError : public XLException {
		public:
			explicit XLSheetError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLDateTimeError : public XLException {
		public:
			explicit XLDateTimeError(const std::string& err) : XLException(err) 
			{
			}
		};
		class XLFormulaError : public XLException {
		public:
			explicit XLFormulaError(const std::string& err) : XLException(err) 
			{
			}
		};
	}
	//
	//#include "XLXmlParser.hpp"
	namespace { // anonymous namespace to define constants / functions that shall not be exported from this module
		constexpr const int XLMaxNamespacedNameLen = 100;
	}

	namespace OpenXLSX {
		#define ENABLE_XML_NAMESPACES 1    // disable this line to control behavior via compiler flag
		#define NO_MULTITHREADING_SAFETY 1 // if this is defined, the function namespaced_name_char will be used for XML namespace support,
		//                                    //  using a static character array for improved performance when multithreading conflicts are not a risk
		#ifdef ENABLE_XML_NAMESPACES
		// ===== Macro for NAMESPACED_NAME when node names might need to be prefixed with the current node's namespace
			#ifdef NO_MULTITHREADING_SAFETY
				#define NAMESPACED_NAME(name_, force_ns_) namespaced_name_char(name_, force_ns_)
			#else
				#define NAMESPACED_NAME(name_, force_ns_) namespaced_name_shared_ptr(name_, force_ns_).get()
			#endif
		#else
			// ===== Optimized version when no namespace support is desired - ignores force_ns_ setting
			#define NAMESPACED_NAME(name_, force_ns_) name_
		#endif

		constexpr const bool XLForceNamespace = true;
		/**
		 * With namespace support: where OpenXLSX already addresses nodes by their namespace, doubling the namespace in a node name
		 *   upon node create can be avoided by passing the optional parameter XLForceNamespace - this will use the node name passed to the
		 *   insertion function "as-is".
		 * Affected XMLNode methods: ::set_name, ::append_child, ::prepend_child, ::insert_child_after, ::insert_child_before
		 */
		extern bool NO_XML_NS;    // defined in XLXmlParser.cpp - default: no XML namespaces
		/**
		 * @brief Set NO_XML_NS to false
		 * @return true if PUGI_AUGMENTED is defined (success), false if PUGI_AUGMENTED is not in use (function would be pointless)
		 * @note CAUTION: this setting should be established before any other OpenXLSX function is used
		 */
		bool enable_xml_namespaces();
		/**
		 * @brief Set NO_XML_NS to true
		 * @return true if PUGI_AUGMENTED is defined (success), false if PUGI_AUGMENTED is not in use (function would be pointless)
		 * @note CAUTION: this setting should be established before any other OpenXLSX function is used
		 */
		bool disable_xml_namespaces();

		// disable this line to use original (non-augmented) pugixml - as of 2024-07-29 this is no longer a realistic option
		#define PUGI_AUGMENTED

		// ===== Using statements to switch between pugixml and augmented pugixml implementation
		#ifdef PUGI_AUGMENTED
			// ===== Forward declarations for using statements below
			class OpenXLSX_xml_node;
			class OpenXLSX_xml_document;

			using XMLNode      = OpenXLSX_xml_node;
			using XMLAttribute = pugi::xml_attribute;
			using OXlXmlDoc  = OpenXLSX_xml_document;
		#else
			using XMLNode      = pugi::xml_node;
			using XMLAttribute = pugi::xml_attribute;
			using OXlXmlDoc  = pugi::xml_document;
		#endif

		// ===== Custom OpenXLSX_xml_node to add functionality to pugi::xml_node
		class OpenXLSX_xml_node : public pugi::xml_node {
		public:
			/**
			 * @brief Default constructor. Constructs a null object.
			 */
			OpenXLSX_xml_node() : pugi::xml_node(), name_begin(0) 
			{
			}
			/**
			 * @brief Inherit all constructors with parameters from pugi::xml_node
			 */
			template <class base>
			// explicit OpenXLSX_xml_node(base b) : pugi::xml_node(b), name_begin(0) // TBD on explicit keyword
			OpenXLSX_xml_node(base b) : pugi::xml_node(b), name_begin(0)
			{
				if(NO_XML_NS)  return;
				const char * name = xml_node::name();
				int pos = 0;
				while(name[pos] && name[pos] != ':')  ++pos;// find name delimiter
				if(name[pos] == ':')  name_begin = pos + 1;// if delimiter was found: update name_begin to point behind that position
			}
			/**
			 * @brief Strip any namespace from name_
			 * @param name_ A node name which may be prefixed with any namespace like so "namespace:nodename"
			 * @return The name_ stripped of a namespace prefix
			 */
			const pugi::char_t* name_without_namespace(const pugi::char_t* name_) const;
			/**
			 * @brief add this node's namespace to name_
			 * @param name_ a node name which shall be prefixed with this node's current namespace
			 * @param force_ns if true, will return name_ unmodified
			 * @return this node's current namespace + ":" + name_ as a const pugi::char_t *
			 */
			const pugi::char_t* namespaced_name_char(const pugi::char_t* name_, bool force_ns) const;
			/**
			 * @brief add this node's namespace to name_
			 * @param name_ a node name which shall be prefixed with this node's current namespace
			 * @param force_ns if true, will return name_ unmodified
			 * @return this node's current namespace + ":" + name_ as a shared_ptr to pugi::char_t
			 */
			std::shared_ptr<pugi::char_t> namespaced_name_shared_ptr(const pugi::char_t* name_, bool force_ns) const;

			// ===== BEGIN: Wrappers for xml_node member functions to ensure OpenXLSX_xml_node return values
			//                and overrides for xml_node member functions to support ignoring the node namespace
			// ===== CAUTION: this section might be incomplete, only functions actually used by OpenXLSX to date have been checked
			/**
			 * @brief get node name while stripping namespace, if so configured (name_begin > 0)
			 * @return the node name without a namespace
			 */
			const pugi::char_t* name() const { return &xml_node::name()[name_begin]; }
			/**
			 * @brief get actual node name from pugi::xml_node::name, including namespace, if any
			 * @return the raw node name
			 */
			const pugi::char_t* raw_name() const { return xml_node::name(); }
			/**
			 * @brief for all functions returning xml_node: invoke base class function, but with a return type of XMLNode (OpenXLSX_xml_node)
			 */
			XMLNode parent() { return pugi::xml_node::parent(); }
			template <typename Predicate> XMLNode find_child(Predicate pred) const { return pugi::xml_node::find_child(pred); }
			XMLNode child(const pugi::char_t* name_) const { return xml_node::child(NAMESPACED_NAME(name_, false)); }
			XMLNode next_sibling(const pugi::char_t* name_) const { return xml_node::next_sibling(NAMESPACED_NAME(name_, false)); }
			XMLNode next_sibling() const { return xml_node::next_sibling(); }
			XMLNode previous_sibling(const pugi::char_t* name_) const { return xml_node::previous_sibling(NAMESPACED_NAME(name_, false)); }
			XMLNode previous_sibling() const { return xml_node::previous_sibling(); }
			const pugi::char_t* child_value() const { return xml_node::child_value(); }
			const pugi::char_t* child_value(const pugi::char_t* name_) const { return xml_node::child_value(NAMESPACED_NAME(name_, false)); }
			bool set_name(const pugi::char_t* rhs, bool force_ns = false) { return xml_node::set_name(NAMESPACED_NAME(rhs, force_ns)); }
			bool set_name(const pugi::char_t* rhs, size_t size, bool force_ns = false) { return xml_node::set_name(NAMESPACED_NAME(rhs, force_ns), size + name_begin); }
			XMLNode append_child(pugi::xml_node_type type_) { return xml_node::append_child(type_); }
			XMLNode prepend_child(pugi::xml_node_type type_) { return xml_node::prepend_child(type_); }
			XMLNode append_child(const pugi::char_t* name_, bool force_ns = false) { return xml_node::append_child(NAMESPACED_NAME(name_, force_ns)); }
			XMLNode prepend_child(const pugi::char_t* name_, bool force_ns = false) { return xml_node::prepend_child(NAMESPACED_NAME(name_, force_ns)); }
			XMLNode insert_child_after(pugi::xml_node_type type_, const xml_node& node) { return xml_node::insert_child_after(type_, node); }
			XMLNode insert_child_before(pugi::xml_node_type type_, const xml_node& node) { return xml_node::insert_child_before(type_, node); }
			XMLNode insert_child_after(const pugi::char_t* name_, const xml_node& node, bool force_ns = false)
				{ return xml_node::insert_child_after(NAMESPACED_NAME(name_, force_ns), node); }
			XMLNode insert_child_before(const pugi::char_t* name_, const xml_node& node, bool force_ns = false)
				{ return xml_node::insert_child_before(NAMESPACED_NAME(name_, force_ns), node); }
			bool remove_child(const pugi::char_t* name_) { return xml_node::remove_child(NAMESPACED_NAME(name_, false)); }
			bool remove_child(const xml_node& n) { return xml_node::remove_child(n); }
			XMLNode find_child_by_attribute(const pugi::char_t* name_, const pugi::char_t* attr_name, const pugi::char_t* attr_value) const
				{ return xml_node::find_child_by_attribute(NAMESPACED_NAME(name_, false), attr_name, attr_value); }
			XMLNode find_child_by_attribute(const pugi::char_t* attr_name, const pugi::char_t* attr_value) const
				{ return xml_node::find_child_by_attribute(attr_name, attr_value); }
			// DISCLAIMER: unused by OpenXLSX, but theoretically impacted by xml namespaces:
			// PUGI_IMPL_FN xml_node xml_node::first_element_by_path(const pugi::char_t* path_, pugi::char_t delimiter) const
			// ===== END: Wrappers for xml_node member functions
			/**
			 * @brief get first node child that matches type
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid child matching the node type or an empty XMLNode
			 */
			XMLNode first_child_of_type(pugi::xml_node_type type_ = pugi::node_element) const;
			/**
			 * @brief get last node child that matches type
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid child matching the node type or an empty XMLNode
			 */
			XMLNode last_child_of_type(pugi::xml_node_type type_ = pugi::node_element) const;
			/**
			 * @brief count node children that match type
			 * @param type_ the pugi::xml_node_type to match
			 * @return the amount of node children matching type
			 */
			size_t child_count_of_type(pugi::xml_node_type type_ = pugi::node_element) const;
			/**
			 * @brief get next node sibling that matches type
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid sibling matching the node type or an empty XMLNode
			 */
			XMLNode next_sibling_of_type(pugi::xml_node_type type_ = pugi::node_element) const;
			/**
			 * @brief get previous node sibling that matches type
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid sibling matching the node type or an empty XMLNode
			 */
			XMLNode previous_sibling_of_type(pugi::xml_node_type type_ = pugi::node_element) const;

			/**
			 * @brief get next node sibling that matches name_ and type
			 * @param name_ the xml_node::name() to match
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid sibling matching the node type or an empty XMLNode
			 */
			XMLNode next_sibling_of_type(const pugi::char_t* name_, pugi::xml_node_type type_ = pugi::node_element) const;

			/**
			 * @brief get previous node sibling that matches name_ and type
			 * @param name_ the xml_node::name() to match
			 * @param type_ the pugi::xml_node_type to match
			 * @return a valid sibling matching the node type or an empty XMLNode
			 */
			XMLNode previous_sibling_of_type(const pugi::char_t* name_, pugi::xml_node_type type_ = pugi::node_element) const;
		private:
			int name_begin;  // nameBegin holds the position in xml_node::name() where the actual node name begins - 0 for non-namespaced nodes
							 // for nodes with a namespace: the position following namespace + delimiter colon, e.g. "x:c" -> nameBegin = 2
		};

		// ===== Custom OpenXLSX_xml_document to override relevant pugi::xml_document member functions with OpenXLSX_xml_node return value
		class OpenXLSX_xml_document : public pugi::xml_document {
		public:
			/**
			 * @brief Default constructor. Constructs a null object.
			 */
			OpenXLSX_xml_document() : pugi::xml_document() 
			{
			}
			/**
			 * @brief Inherit all constructors with parameters from pugi::xml_document
			 */
			template <class base>
			// explicit OpenXLSX_xml_document(base b) : xml_document(b) // TBD
			OpenXLSX_xml_document(base b) : pugi::xml_document(b)
			{}

			// ===== BEGIN: Wrappers for xml_document member functions to ensure OpenXLSX_xml_node return values
			// ===== CAUTION: this section is incomplete, only implementing those functions actually used by OpenXLSX to date
			/**
			 * @brief for all functions: invoke the base class function, but with a return type of OpenXLSX_xml_node
			 */
			XMLNode document_element() const { return pugi::xml_document::document_element(); }
			// ===== END: Wrappers for xml_document member functions
		};
		//
		//#include "XLFormula.hpp"
		constexpr bool XLResetValue    = true;
		constexpr bool XLPreserveValue = false;

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
			operator std::string() const;
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
			operator std::string() const;
			/**
			 * @brief Implicit conversion to XLFormula object.
			 * @return Returns the corresponding XLFormula object.
			 */
			operator XLFormula() const;
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

			XLCell*  m_cell;     /**< Pointer to the owning XLCell object. */
			XMLNode* m_cellNode; /**< Pointer to corresponding XML cell node. */
		};

		inline bool operator==(const XLFormula& lhs, const XLFormula& rhs) { return lhs.m_formulaString == rhs.m_formulaString; }
		inline bool operator!=(const XLFormula& lhs, const XLFormula& rhs) { return lhs.m_formulaString != rhs.m_formulaString; }
		/**
		 * @brief send a formula string to an ostream
		 * @param os the output destination
		 * @param value the formula to send to os
		 */
		inline std::ostream& operator<<(std::ostream& os, const XLFormula& value) { return os << value.m_formulaString; }
		/**
		 * @brief send a formula string to an ostream
		 * @param os the output destination
		 * @param value the formula proxy whose formula to send to os
		 */
		inline std::ostream& operator<<(std::ostream& os, const XLFormulaProxy& value) { return os << value.get(); }
		//
		//#include "XLColor.hpp"
		class XLColor {
			friend bool operator==(const XLColor& lhs, const XLColor& rhs);
			friend bool operator!=(const XLColor& lhs, const XLColor& rhs);
		public:
			XLColor();
			XLColor(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);
			XLColor(uint8_t red, uint8_t green, uint8_t blue);
			explicit XLColor(const std::string& hexCode);
			XLColor(const XLColor& other);
			XLColor(XLColor&& other) noexcept;
			~XLColor();
			XLColor& operator=(const XLColor& other);
			XLColor& operator=(XLColor&& other) noexcept;
			void set(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);
			void set(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0);
			void set(const std::string& hexCode);
			uint8_t alpha() const;
			uint8_t red() const;
			uint8_t green() const;
			uint8_t blue() const;
			std::string hex() const;
		private:
			uint8_t m_alpha { 255 };
			uint8_t m_red { 0 };
			uint8_t m_green { 0 };
			uint8_t m_blue { 0 };
		};

		inline bool operator==(const XLColor& lhs, const XLColor& rhs)
		{
			return lhs.alpha() == rhs.alpha() && lhs.red() == rhs.red() && lhs.green() == rhs.green() && lhs.blue() == rhs.blue();
		}

		inline bool operator!=(const XLColor& lhs, const XLColor& rhs) { return !(lhs == rhs); }
		//
		//#include "XLXmlFile.hpp"
		class XLXmlData;
		class XLDocument;
		/**
		 * @brief The XLUnsupportedElement class provides a stub implementation that can be used as function
		 *  parameter or return type for currently unsupported XML features.
		 */
		class XLUnsupportedElement {
		public:
			XLUnsupportedElement() 
			{
			}
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
		public:
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
		//
		//#include "XLStyles.hpp"
		using namespace std::literals::string_view_literals;    // enables sv suffix only
		using XLStyleIndex = size_t;     // custom data type for XLStyleIndex

		constexpr const uint32 XLInvalidUInt16 = 0xffff;         // used to signal "value not defined" for uint16 return types
		constexpr const uint32 XLInvalidUInt32 = 0xffffffff;     // used to signal "value not defined" for uint32 return types
		constexpr const uint32 XLDeleteProperty = XLInvalidUInt32;      // when 0 or "" is not the same as "property does not exist", this value
			//  can be passed to setter functions to delete the property from XML
			//  currently supported in: XLDataBarColor::setTheme
		constexpr const bool XLPermitXfID      = true;             // use with XLCellFormat constructor to enable xfId() getter and setXfId() setter
		constexpr const bool XLCreateIfMissing = true;             // use with XLCellFormat::alignment(XLCreateIfMissing)
		constexpr const bool XLDoNotCreate     = false;            // use with XLCellFormat::alignment(XLDoNotCreate)
		constexpr const bool XLForceFillType   = true;
		constexpr const char * XLDefaultStylesPrefix       = "\n\t";       // indentation to use for newly created root level style node tags
		constexpr const char * XLDefaultStyleEntriesPrefix = "\n\t\t";     // indentation to use for newly created style entry nodes
		constexpr const XLStyleIndex XLDefaultCellFormat = 0;              // default cell format index in xl/styles.xml:<styleSheet>:<cellXfs>

		// ===== As pugixml attributes are not guaranteed to support value range of XLStyleIndex, use 32 bit unsigned int
		constexpr const XLStyleIndex XLInvalidStyleIndex = XLInvalidUInt32;        // as a function return value, indicates no valid index

		constexpr const uint32 XLDefaultFontSize       = 12;             //
		constexpr const char *   XLDefaultFontColor      = "ff000000";     // default font color
		constexpr const char *   XLDefaultFontColorTheme = "";             // TBD what this means / how it is used
		constexpr const char *   XLDefaultFontName       = "Arial";        //
		constexpr const uint32 XLDefaultFontFamily     = 0;              // TBD what this means / how it is used
		constexpr const uint32 XLDefaultFontCharset    = 1;              // TBD what this means / how it is used

		constexpr const char * XLDefaultLineStyle = "";     // empty string = line not set

		// forward declarations of all classes in this header
		class XLNumberFormat;
		class XLNumberFormats;
		class XLFont;
		class XLFonts;
		class XLFill;
		class XLFills;
		class XLLine;
		class XLBorder;
		class XLBorders;
		class XLAlignment;
		class XLCellFormat;
		class XLCellFormats;
		class XLCellStyle;
		class XLCellStyles;
		class XLStyles;

		enum XLUnderlineStyle : uint8_t {
			XLUnderlineNone    = 0,
			XLUnderlineSingle  = 1,
			XLUnderlineDouble  = 2,
			XLUnderlineInvalid = 255
		};
		enum XLFontSchemeStyle : uint8_t {
			XLFontSchemeNone    =   0, // <scheme val="none"/>
			XLFontSchemeMajor   =   1, // <scheme val="major"/>
			XLFontSchemeMinor   =   2, // <scheme val="minor"/>
			XLFontSchemeInvalid = 255  // all other values
		};
		enum XLVerticalAlignRunStyle : uint8_t {
			XLBaseline                =   0, // <vertAlign val="baseline"/>
			XLSubscript               =   1, // <vertAlign val="subscript"/>
			XLSuperscript             =   2, // <vertAlign val="superscript"/>
			XLVerticalAlignRunInvalid = 255
		};
		enum XLFillType : uint8_t {
			XLGradientFill     =   0,    // <gradientFill />
			XLPatternFill      =   1,    // <patternFill />
			XLFillTypeInvalid  = 255,    // any child of <fill> that is not one of the above
		};
		enum XLGradientType : uint8_t {
			XLGradientLinear      =   0,
			XLGradientPath        =   1,
			XLGradientTypeInvalid = 255
		};
		enum XLPatternType: uint8_t {
			XLPatternNone            =   0,   // "none"
			XLPatternSolid           =   1,   // "solid"
			XLPatternMediumGray      =   2,   // "mediumGray"
			XLPatternDarkGray        =   3,   // "darkGray"
			XLPatternLightGray       =   4,   // "lightGray"
			XLPatternDarkHorizontal  =   5,   // "darkHorizontal"
			XLPatternDarkVertical    =   6,   // "darkVertical"
			XLPatternDarkDown        =   7,   // "darkDown"
			XLPatternDarkUp          =   8,   // "darkUp"
			XLPatternDarkGrid        =   9,   // "darkGrid"
			XLPatternDarkTrellis     =  10,   // "darkTrellis"
			XLPatternLightHorizontal =  11,   // "lightHorizontal"
			XLPatternLightVertical   =  12,   // "lightVertical"
			XLPatternLightDown       =  13,   // "lightDown"
			XLPatternLightUp         =  14,   // "lightUp"
			XLPatternLightGrid       =  15,   // "lightGrid"
			XLPatternLightTrellis    =  16,   // "lightTrellis"
			XLPatternGray125         =  17,   // "gray125"
			XLPatternGray0625        =  18,   // "gray0625"
			XLPatternTypeInvalid     = 255    // any patternType that is not one of the above
		};

		constexpr const XLFillType XLDefaultFillType       = XLPatternFill;    // node name for the pattern description is derived from this
		constexpr const XLPatternType XLDefaultPatternType = XLPatternNone; // attribute patternType default value: no fill
		constexpr const char * XLDefaultPatternFgColor = "ffffffff"; // child node fgcolor attribute rgb value
		constexpr const char * XLDefaultPatternBgColor = "ff000000"; // child node bgcolor attribute rgb value

		enum XLLineType: uint8_t {
			XLLineLeft       =   0,
			XLLineRight      =   1,
			XLLineTop        =   2,
			XLLineBottom     =   3,
			XLLineDiagonal   =   4,
			XLLineVertical   =   5,
			XLLineHorizontal =   6,
			XLLineInvalid    = 255
		};

		enum XLLineStyle : uint8_t {
			XLLineStyleNone             =   0,
			XLLineStyleThin             =   1,
			XLLineStyleMedium           =   2,
			XLLineStyleDashed           =   3,
			XLLineStyleDotted           =   4,
			XLLineStyleThick            =   5,
			XLLineStyleDouble           =   6,
			XLLineStyleHair             =   7,
			XLLineStyleMediumDashed     =   8,
			XLLineStyleDashDot          =   9,
			XLLineStyleMediumDashDot    =  10,
			XLLineStyleDashDotDot       =  11,
			XLLineStyleMediumDashDotDot =  12,
			XLLineStyleSlantDashDot     =  13,
			XLLineStyleInvalid          = 255
		};

		enum XLAlignmentStyle : uint8_t {
			XLAlignGeneral          =   0, // value="general",          horizontal only
			XLAlignLeft             =   1, // value="left",             horizontal only
			XLAlignRight            =   2, // value="right",            horizontal only
			XLAlignCenter           =   3, // value="center",           both
			XLAlignTop              =   4, // value="top",              vertical only
			XLAlignBottom           =   5, // value="bottom",           vertical only
			XLAlignFill             =   6, // value="fill",             horizontal only
			XLAlignJustify          =   7, // value="justify",          both
			XLAlignCenterContinuous =   8, // value="centerContinuous", horizontal only
			XLAlignDistributed      =   9, // value="distributed",      both
			XLAlignInvalid          = 255  // all other values
		};

		enum XLReadingOrder : uint32 {
			XLReadingOrderContextual  = 0,
			XLReadingOrderLeftToRight = 1,
			XLReadingOrderRightToLeft = 2
		};
		/**
		 * @brief An encapsulation of a number format (numFmt) item
		 */
		class XLNumberFormat {
			friend class XLNumberFormats;    // for access to m_numberFormatNode in XLNumberFormats::create
		public:
			XLNumberFormat();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the styles item. If no input is provided, a null node is used.
			 */
			explicit XLNumberFormat(const XMLNode& node);
			XLNumberFormat(const XLNumberFormat& other);
			XLNumberFormat(XLNumberFormat&& other) noexcept = default;
			~XLNumberFormat();
			XLNumberFormat & operator=(const XLNumberFormat& other);
			XLNumberFormat & operator=(XLNumberFormat&& other) noexcept = default;
			/**
			 * @brief Get the id of the number format
			 * @return The id for this number format
			 */
			uint32 numberFormatId() const;
			/**
			 * @brief Get the code of the number format
			 * @return The format code for this number format
			 */
			std::string formatCode() const;
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setNumberFormatId(uint32 newNumberFormatId);
			bool setFormatCode(std::string newFormatCode);
			/**
			 * @brief Return a string summary of the number format
			 * @return string with info about the number format object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_numberFormatNode; /**< An XMLNode object with the number format item */
		};
		/**
		 * @brief An encapsulation of the XLSX number formats (numFmts)
		 */
		class XLNumberFormats {
		public:
			XLNumberFormats();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the styles item. If no input is provided, a null node is used.
			 */
			explicit XLNumberFormats(const XMLNode& node);
			XLNumberFormats(const XLNumberFormats& other);
			XLNumberFormats(XLNumberFormats&& other);
			~XLNumberFormats();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLNumberFormats& operator=(const XLNumberFormats& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLNumberFormats& operator=(XLNumberFormats&& other) noexcept = default;

			/**
			 * @brief Get the count of number formats
			 * @return The amount of entries in the number formats
			 */
			size_t count() const;
			/**
			 * @brief Get the number format identified by index
			 * @param index an array index within XLStyles::numberFormats()
			 * @return An XLNumberFormat object
			 * @throw XLException when index is out of m_numberFormats range
			 */
			XLNumberFormat numberFormatByIndex(XLStyleIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to numberFormatByIndex
			 */
			XLNumberFormat operator[](XLStyleIndex index) const { return numberFormatByIndex(index); }
			/**
			 * @brief Get the number format identified by numberFormatId
			 * @param numberFormatId a numFmtId reference to a number format
			 * @return An XLNumberFormat object
			 * @throw XLException if no match for numberFormatId is found within m_numberFormats
			 */
			XLNumberFormat numberFormatById(uint32 numberFormatId) const;
			/**
			 * @brief Get the numFmtId from the number format identified by index
			 * @param index an array index within XLStyles::numberFormats()
			 * @return the uint32 numFmtId corresponding to index
			 * @throw XLException when index is out of m_numberFormats range
			 */
			uint32 numberFormatIdFromIndex(XLStyleIndex index) const;
			/**
			 * @brief Append a new XLNumberFormat, based on copyFrom, and return its index in numFmts node
			 * @param copyFrom Can provide an XLNumberFormat to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 * @todo: TBD assign a unique, non-reserved uint32 numFmtId. Alternatively, the user should configure it manually via setNumberFormatId
			 * @todo: TBD implement a "getFreeNumberFormatId()" method that skips reserved identifiers and iterates over m_numberFormats to avoid
			 *         all existing number format Ids.
			 */
			XLStyleIndex create(XLNumberFormat copyFrom = XLNumberFormat{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_numberFormatsNode; /**< An XMLNode object with the number formats item */
			std::vector<XLNumberFormat> m_numberFormats;
		};
		/**
		 * @brief An encapsulation of a font item
		 */
		class XLFont {
			friend class XLFonts;    // for access to m_fontNode in XLFonts::create
		public:
			XLFont();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the fonts XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLFont(const XMLNode& node);
			XLFont(const XLFont& other);
			XLFont(XLFont&& other) noexcept = default;
			~XLFont();
			XLFont& operator=(const XLFont& other);
			XLFont& operator=(XLFont&& other) noexcept = default;
			/**
			 * @brief Get the font name
			 * @return The font name
			 */
			std::string fontName() const;
			/**
			 * @brief Get the font charset
			 * @return The font charset id
			 */
			size_t fontCharset() const;
			/**
			 * @brief Get the font family
			 * @return The font family id
			 */
			size_t fontFamily() const;
			/**
			 * @brief Get the font size
			 * @return The font size
			 */
			size_t fontSize() const;
			/**
			 * @brief Get the font color
			 * @return The font color
			 */
			XLColor fontColor() const;
			/**
			 * @brief Get the font bold status
			 * @return true = bold, false = not bold
			 */
			bool bold() const;
			/**
			 * @brief Get the font italic (cursive) status
			 * @return true = italic, false = not italice
			 */
			bool italic() const;
			/**
			 * @brief Get the font strikethrough status
			 * @return true = strikethrough, false = no strikethrough
			 */
			bool strikethrough() const;
			/**
			 * @brief Get the font underline status
			 * @return An XLUnderlineStyle value
			 */
			XLUnderlineStyle underline() const;
			/**
			 * @brief get the font scheme: none, major or minor
			 * @return An XLFontSchemeStyle
			 */
			XLFontSchemeStyle scheme() const;
			/**
			 * @brief get the font vertical alignment run style: baseline, subscript or superscript
			 * @return An XLVerticalAlignRunStyle
			 */
			XLVerticalAlignRunStyle vertAlign() const;
			/**
			 * @brief Get the outline status
			 * @return a TBD bool
			 * @todo need to find a use case for this
			 */
			bool outline() const;
			/**
			 * @brief Get the shadow status
			 * @return a TBD bool
			 * @todo need to find a use case for this
			 */
			bool shadow() const;
			/**
			 * @brief Get the condense status
			 * @return a TBD bool
			 * @todo need to find a use case for this
			 */
			bool condense() const;
			/**
			 * @brief Get the extend status
			 * @return a TBD bool
			 * @todo need to find a use case for this
			 */
			bool extend() const;
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setFontName(std::string newName);
			bool setFontCharset(size_t newCharset);
			bool setFontFamily(size_t newFamily);
			bool setFontSize(size_t newSize);
			bool setFontColor(XLColor newColor);
			bool setBold(bool set = true);
			bool setItalic(bool set = true);
			bool setStrikethrough(bool set = true);
			bool setUnderline(XLUnderlineStyle style = XLUnderlineSingle);
			bool setScheme(XLFontSchemeStyle newScheme);
			bool setVertAlign(XLVerticalAlignRunStyle newVertAlign);
			bool setOutline(bool set = true);
			bool setShadow(bool set = true);
			bool setCondense(bool set = true);
			bool setExtend(bool set = true);
			/**
			 * @brief Return a string summary of the font properties
			 * @return string with info about the font object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_fontNode;         /**< An XMLNode object with the font item */
		};
		/**
		 * @brief An encapsulation of the XLSX fonts
		 */
		class XLFonts {
		public:
			XLFonts();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the styles item. If no input is provided, a null node is used.
			 */
			explicit XLFonts(const XMLNode& node);
			XLFonts(const XLFonts& other);
			XLFonts(XLFonts&& other);
			~XLFonts();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLFonts& operator=(const XLFonts& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLFonts& operator=(XLFonts&& other) noexcept = default;
			/**
			 * @brief Get the count of fonts
			 * @return The amount of font entries
			 */
			size_t count() const;
			/**
			 * @brief Get the font identified by index
			 * @param index The index within the XML sequence
			 * @return An XLFont object
			 */
			XLFont fontByIndex(XLStyleIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to fontByIndex
			 * @param index The index within the XML sequence
			 * @return An XLFont object
			 */
			XLFont operator[](XLStyleIndex index) const { return fontByIndex(index); }
			/**
			 * @brief Append a new XLFont, based on copyFrom, and return its index in fonts node
			 * @param copyFrom Can provide an XLFont to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLFont copyFrom = XLFont{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_fontsNode;        /**< An XMLNode object with the fonts item */
			std::vector<XLFont> m_fonts;
		};
		/**
		 * @brief An encapsulation of an XLSX Data Bar Color (CT_Color) item
		 */
		class XLDataBarColor {
		public:
			XLDataBarColor();
			/**
			 * @brief Constructor. New items should only be created through an XLGradientStop or XLLine object.
			 * @param node An XMLNode object with a data bar color XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLDataBarColor(const XMLNode& node);
			XLDataBarColor(const XLDataBarColor& other);
			/**
			 * @brief Move Constructor.
			 * @param other Object to be moved.
			 */
			XLDataBarColor(XLDataBarColor&& other) noexcept = default;
			~XLDataBarColor() = default;
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLDataBarColor& operator=(const XLDataBarColor& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLDataBarColor& operator=(XLDataBarColor&& other) noexcept = default;
			/**
			 * @brief Get the line color from the rgb attribute
			 * @return An XLColor object
			 */
			XLColor rgb() const;
			/**
			 * @brief Get the line color tint
			 * @return A double value as stored in the "tint" attribute (should be between [-1.0;+1.0]), 0.0 if attribute does not exist
			 */
			double tint() const;
			/**
			 * @brief currently unsupported getter stubs
			 */
			bool     automatic() const; // <color auto="true" />
			uint32 indexed()   const; // <color indexed="1" />
			uint32 theme()     const; // <color theme="1" />
			/**
			 * @brief Setter functions for data bar color parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setRgb(XLColor newColor);
			bool set(XLColor newColor) { return setRgb(newColor); }          // alias for setRgb
			bool setTint(double newTint);
			bool setAutomatic(bool set = true);
			bool setIndexed(uint32 newIndex);
			bool setTheme(uint32 newTheme);
			/**
			 * @brief Return a string summary of the color properties
			 * @return string with info about the color object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_colorNode;        /**< An XMLNode object with the color item */
		};
		/**
		 * @brief An encapsulation of an fill::gradientFill::stop item
		 */
		class XLGradientStop {
			friend class XLGradientStops;    // for access to m_stopNode in XLGradientStops::create
		public:
			XLGradientStop();
			/**
			 * @brief Constructor. New items should only be created through an XLGradientStops object.
			 * @param node An XMLNode object with the gradient stop XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLGradientStop(const XMLNode& node);
			XLGradientStop(const XLGradientStop& other);
			XLGradientStop(XLGradientStop&& other) noexcept = default;
			~XLGradientStop() = default;
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLGradientStop& operator=(const XLGradientStop& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLGradientStop& operator=(XLGradientStop&& other) noexcept = default;
			/**
			 * @brief Getter functions
			 * @return The requested gradient stop property
			 */
			XLDataBarColor color() const; // <stop><color /></stop>
			double position()      const; // <stop position="1.2" />
			/**
			 * @brief Setter functions
			 * @param value that shall be set
			 * @return true for success, false for failure
			 * @note for color setters, use the color() getter with the XLDataBarColor setter functions
			 */
			bool setPosition(double newPosition);
			/**
			 * @brief Return a string summary of the stop properties
			 * @return string with info about the stop object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_stopNode;         /**< An XMLNode object with the stop item */
		};
		/**
		 * @brief An encapsulation of an array of fill::gradientFill::stop items
		 */
		class XLGradientStops {
		public:
			XLGradientStops();
			/**
			 * @brief Constructor. New items should only be created through an XLFill object.
			 * @param node An XMLNode object with the gradientFill item. If no input is provided, a null node is used.
			 */
			explicit XLGradientStops(const XMLNode& node);
			XLGradientStops(const XLGradientStops& other);
			XLGradientStops(XLGradientStops&& other);
			~XLGradientStops();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLGradientStops& operator=(const XLGradientStops& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLGradientStops& operator=(XLGradientStops&& other) noexcept = default;

			/**
			 * @brief Get the count of gradient stops
			 * @return The amount of stop entries
			 */
			size_t count() const;

			/**
			 * @brief Get the gradient stop entry identified by index
			 * @param index The index within the XML sequence
			 * @return An XLGradientStop object
			 */
			XLGradientStop stopByIndex(XLStyleIndex index) const;

			/**
			 * @brief Operator overload: allow [] as shortcut access to stopByIndex
			 * @param index The index within the XML sequence
			 * @return An XLGradientStop object
			 */
			XLGradientStop operator[](XLStyleIndex index) const { return stopByIndex(index); }

			/**
			 * @brief Append a new XLGradientStop, based on copyFrom, and return its index in fills node
			 * @param copyFrom Can provide an XLGradientStop to use as template for the new style
			 * @param stopEntriesPrefix Prefix the newly created stop XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLGradientStop copyFrom = XLGradientStop{}, std::string styleEntriesPrefix = "");

			std::string summary() const;

		private:
			std::unique_ptr<XMLNode> m_gradientNode;        /**< An XMLNode object with the gradientFill item */
			std::vector<XLGradientStop> m_gradientStops;
		};

		/**
		 * @brief An encapsulation of a fill item
		 */
		class XLFill {
			friend class XLFills; // for access to m_fillNode in XLFills::create
		public:
			XLFill();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the fill XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLFill(const XMLNode& node);
			XLFill(const XLFill& other);
			XLFill(XLFill&& other) noexcept = default;
			~XLFill();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLFill& operator=(const XLFill& other);

			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLFill& operator=(XLFill&& other) noexcept = default;
			/**
			 * @brief Get the name of the element describing a fill
			 * @return The XLFillType derived from the name of the first child element of the fill node
			 */
			XLFillType fillType() const;
			/**
			 * @brief Create & set the base XML element describing the fill
			 * @param newFillType that shall be set
			 * @param force erase an existing fillType() if not equal newFillType
			 * @return true for success, false for failure
			 */
			bool setFillType(XLFillType newFillType, bool force = false);
		private: // ---------- Switch to private context for two Methods  --------- //
			/**
			 * @brief Throw XLException if fillType() matches typeToThrowOn
			 * @param typeToThrowOn throw on this
			 * @param functionName include this (calling function name) in the exception
			 */
			void throwOnFillType(XLFillType typeToThrowOn, const char * functionName) const;
			/**
			 * @brief Fetch a valid first element child of m_fillNode. Create with default if needed
			 * @param fillTypeIfEmpty if no conflicting fill type exists, create a node with this fill type
			 * @param functionName include this (calling function name) in a potential exception
			 * @return An XMLNode containing a fill description
			 * @throw XLException if fillTypeIfEmpty is in conflict with a current fillType()
			 */
			XMLNode getValidFillDescription(XLFillType fillTypeIfEmpty, const char * functionName);
		public:                                              // ---------- Switch back to public context ---------------------- //
			/**
			 * @brief Getter functions for gradientFill - will throwOnFillType(XLPatternFill, __func__)
			 * @return The requested gradientFill property
			 */
			XLGradientType gradientType(); // <gradientFill type="path" />
			double degree();
			double left();
			double right();
			double top();
			double bottom();
			XLGradientStops stops();
			/**
			 * @brief Getter functions for patternFill - will throwOnFillType(XLGradientFill, __func__)
			 * @return The requested patternFill property
			 */
			XLPatternType patternType();
			XLColor color();
			XLColor backgroundColor();

			/**
			 * @brief Setter functions for gradientFill - will throwOnFillType(XLPatternFill, __func__)
			 * @param value that shall be set
			 * @return true for success, false for failure
			 * @note for gradient stops, use the stops() getter with the XLGradientStops access functions (create, stopByIndex, [])
			 *       and the XLGradientStop setter functions
			 */
			bool setGradientType(XLGradientType newType);
			bool setDegree(double newDegree);
			bool setLeft(double newLeft);
			bool setRight(double newRight);
			bool setTop(double newTop);
			bool setBottom(double newBottom);

			/**
			 * @brief Setter functions for patternFill - will throwOnFillType(XLGradientFill, __func__)
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setPatternType(XLPatternType newPatternType);
			bool setColor(XLColor newColor);
			bool setBackgroundColor(XLColor newBgColor);

			/**
			 * @brief Return a string summary of the fill properties
			 * @return string with info about the fill object
			 */
			std::string summary();
		private:
			std::unique_ptr<XMLNode> m_fillNode;         /**< An XMLNode object with the fill item */
		};
			/**
		 * @brief An encapsulation of the XLSX fills
		 */
		class XLFills {
		public:
			XLFills();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the fills item. If no input is provided, a null node is used.
			 */
			explicit XLFills(const XMLNode& node);
			XLFills(const XLFills& other);
			XLFills(XLFills&& other);
			~XLFills();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLFills& operator=(const XLFills& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLFills& operator=(XLFills&& other) noexcept = default;
			/**
			 * @brief Get the count of fills
			 * @return The amount of fill entries
			 */
			size_t count() const;
			/**
			 * @brief Get the fill entry identified by index
			 * @param index The index within the XML sequence
			 * @return An XLFill object
			 */
			XLFill fillByIndex(XLStyleIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to fillByIndex
			 * @param index The index within the XML sequence
			 * @return An XLFill object
			 */
			XLFill operator[](XLStyleIndex index) const { return fillByIndex(index); }
			/**
			 * @brief Append a new XLFill, based on copyFrom, and return its index in fills node
			 * @param copyFrom Can provide an XLFill to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLFill copyFrom = XLFill{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_fillsNode;        /**< An XMLNode object with the fills item */
			std::vector<XLFill> m_fills;
		};
		/**
		 * @brief An encapsulation of a line item
		 */
		class XLLine {
			// friend class TBD: XLBorder or XLBorders;    // for access to m_lineNode in TBD
		public:
			XLLine();
			/**
			 * @brief Constructor. New items should only be created through an XLBorder object.
			 * @param node An XMLNode object with the line XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLLine(const XMLNode& node);
			XLLine(const XLLine& other);
			XLLine(XLLine&& other) noexcept = default;
			~XLLine();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLLine& operator=(const XLLine& other);

			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLLine& operator=(XLLine&& other) noexcept = default;

			/**
			 * @brief Get the line style
			 * @return An XLLineStyle enum
			 */
			XLLineStyle style() const;

			/**
			 * @brief Evaluate XLLine as bool
			 * @return true if line is set, false if not
			 */
			explicit operator bool() const;

			XLDataBarColor color() const; // <line><color /></line> where node can be left, right, top, bottom, diagonal, vertical, horizontal

			/**
			 * @note Regarding setter functions for style parameters:
			 * @note Please refer to XLBorder setLine / setLeft / setRight / setTop / setBottom / setDiagonal
			 * @note  and XLDataBarColor setter functions that can be invoked via color()
			 */

			/**
			 * @brief Return a string summary of the line properties
			 * @return string with info about the line object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_lineNode;         /**< An XMLNode object with the line item */
		};
		/**
		 * @brief An encapsulation of a border item
		 */
		class XLBorder {
			friend class XLBorders;    // for access to m_borderNode in XLBorders::create
		public:
			XLBorder();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the border XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLBorder(const XMLNode& node);
			XLBorder(const XLBorder& other);
			XLBorder(XLBorder&& other) noexcept = default;
			~XLBorder();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLBorder& operator=(const XLBorder& other);

			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLBorder& operator=(XLBorder&& other) noexcept = default;

			/**
			 * @brief Get the diagonal up property
			 * @return true if set, otherwise false
			 */
			bool diagonalUp() const;

			/**
			 * @brief Get the diagonal down property
			 * @return true if set, otherwise false
			 */
			bool diagonalDown() const;

			/**
			 * @brief Get the outline property
			 * @return true if set, otherwise false
			 */
			bool outline() const;

			/**
			 * @brief Get the left line property
			 * @return An XLLine object
			 */
			XLLine left() const;

			/**
			 * @brief Get the left line property
			 * @return An XLLine object
			 */
			XLLine right() const;

			/**
			 * @brief Get the left line property
			 * @return An XLLine object
			 */
			XLLine top() const;
			/**
			 * @brief Get the bottom line property
			 * @return An XLLine object
			 */
			XLLine bottom() const;
			/**
			 * @brief Get the diagonal line property
			 * @return An XLLine object
			 */
			XLLine diagonal() const;
			/**
			 * @brief Get the vertical line property
			 * @return An XLLine object
			 */
			XLLine vertical() const;
			/**
			 * @brief Get the horizontal line property
			 * @return An XLLine object
			 */
			XLLine horizontal() const;
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @param value2 (optional) that shall be set
			 * @return true for success, false for failure
			 */
			bool setDiagonalUp(bool set = true);
			bool setDiagonalDown(bool set = true);
			bool setOutline(bool set = true);
			bool setLine(XLLineType lineType, XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setLeft(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setRight(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setTop(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setBottom(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setDiagonal(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setVertical(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);
			bool setHorizontal(XLLineStyle lineStyle, XLColor lineColor, double lineTint = 0.0);

			/**
			 * @brief Return a string summary of the font properties
			 * @return string with info about the font object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_borderNode;       /**< An XMLNode object with the font item */
			inline static const std::vector< std::string_view > m_nodeOrder = { "left", "right", "top", "bottom", "diagonal", "vertical", "horizontal" };
		};
		/**
		 * @brief An encapsulation of the XLSX borders
		 */
		class XLBorders {
		public:
			XLBorders();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the borders item. If no input is provided, a null node is used.
			 */
			explicit XLBorders(const XMLNode& node);
			XLBorders(const XLBorders& other);
			XLBorders(XLBorders&& other);
			~XLBorders();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLBorders& operator=(const XLBorders& other);

			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLBorders& operator=(XLBorders&& other) noexcept = default;

			/**
			 * @brief Get the count of border descriptions
			 * @return The amount of border description entries
			 */
			size_t count() const;
			/**
			 * @brief Get the border description identified by index
			 * @param index The index within the XML sequence
			 * @return An XLBorder object
			 */
			XLBorder borderByIndex(XLStyleIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to borderByIndex
			 * @param index The index within the XML sequence
			 * @return An XLBorder object
			 */
			XLBorder operator[](XLStyleIndex index) const { return borderByIndex(index); }
			/**
			 * @brief Append a new XLBorder, based on copyFrom, and return its index in borders node
			 * @param copyFrom Can provide an XLBorder to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLBorder copyFrom = XLBorder{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_bordersNode;      /**< An XMLNode object with the borders item */
			std::vector<XLBorder> m_borders;
		};
		/**
		 * @brief An encapsulation of an alignment item
		 */
		class XLAlignment {
			// friend class TBD: XLCellFormat or XLCellFormats;    // for access to m_alignmentNode in TBD
		public:
			XLAlignment();
			/**
			 * @brief Constructor. New items should only be created through an XLBorder object.
			 * @param node An XMLNode object with the alignment XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLAlignment(const XMLNode& node);
			XLAlignment(const XLAlignment& other);
			XLAlignment(XLAlignment&& other) noexcept = default;
			~XLAlignment();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLAlignment& operator=(const XLAlignment& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLAlignment& operator=(XLAlignment&& other) noexcept = default;
			/**
			 * @brief Get the horizontal alignment
			 * @return An XLAlignmentStyle
			 */
			XLAlignmentStyle horizontal() const;
			/**
			 * @brief Get the vertical alignment
			 * @return An XLAlignmentStyle
			 */
			XLAlignmentStyle vertical() const;
			/**
			 * @brief Get the text rotation
			 * @return A value in degrees (TBD: clockwise? counter-clockwise?)
			 */
			uint16 textRotation() const;
			/**
			 * @brief Check whether text wrapping is enabled
			 * @return true if enabled, false otherwise
			 */
			bool wrapText() const;
			/**
			 * @brief Get the indent setting
			 * @return An integer value, where an increment of 1 represents 3 spaces.
			 */
			uint32 indent() const;
			/**
			 * @brief Get the relative indent setting
			 * @return An integer value, where an increment of 1 represents 1 space, in addition to indent()*3 spaces
			 * @note can be negative
			 */
			int32_t relativeIndent() const;
			/**
			 * @brief Check whether justification of last line is enabled
			 * @return true if enabled, false otherwise
			 */
			bool justifyLastLine() const;
			/**
			 * @brief Check whether shrink to fit is enabled
			 * @return true if enabled, false otherwise
			 */
			bool shrinkToFit() const;
			/**
			 * @brief Get the reading order setting
			 * @return An integer value: 0 - Context Dependent, 1 - Left-to-Right, 2 - Right-to-Left (any other value should be invalid)
			 */
			uint32 readingOrder() const;
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setHorizontal(XLAlignmentStyle newStyle);
			bool setVertical(XLAlignmentStyle newStyle);
			/**
			 * @details on setTextRotation from XLSX specification:
			 * Text rotation in cells. Expressed in degrees. Values range from 0 to 180. The first letter of the text
			 *  is considered the center-point of the arc.
			 * For 0 - 90, the value represents degrees above horizon. For 91-180 the degrees below the horizon is calculated as:
			 * [degrees below horizon] = 90 - [newRotation].
			 * Examples: setTextRotation( 45): / (text is formatted along a line from lower left to upper right)
			 *           setTextRotation(135): \ (text is formatted along a line from upper left to lower right)
			 */
			bool setTextRotation(uint16 newRotation);
			bool setWrapText(bool set = true);
			bool setIndent(uint32 newIndent);
			bool setRelativeIndent(int32_t newIndent);
			bool setJustifyLastLine(bool set = true);
			bool setShrinkToFit(bool set = true);
			bool setReadingOrder(uint32 newReadingOrder);    // can be used with XLReadingOrderContextual, XLReadingOrderLeftToRight, XLReadingOrderRightToLeft
			/**
			 * @brief Return a string summary of the alignment properties
			 * @return string with info about the alignment object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_alignmentNode;    /**< An XMLNode object with the alignment item */
		};
		/**
		 * @brief An encapsulation of a cell format item
		 */
		class XLCellFormat {
			friend class XLCellFormats;    // for access to m_cellFormatNode in XLCellFormats::create
		public:
			XLCellFormat();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the xf XMLNode. If no input is provided, a null node is used.
			 * @param permitXfId true (XLPermitXfID) -> getter xfId and setter setXfId are enabled, otherwise will throw XLException if invoked
			 */
			explicit XLCellFormat(const XMLNode& node, bool permitXfId);
			XLCellFormat(const XLCellFormat& other);
			XLCellFormat(XLCellFormat&& other) noexcept = default;
			~XLCellFormat();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCellFormat& operator=(const XLCellFormat& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCellFormat& operator=(XLCellFormat&& other) noexcept = default;
			/**
			 * @brief Get the number format id
			 * @return The identifier of a number format, built-in (predefined by office) or defind in XLNumberFormats
			 */
			uint32 numberFormatId() const;
			/**
			 * @brief Get the font index
			 * @return The index(!) of a font as defined in XLFonts
			 */
			XLStyleIndex fontIndex() const;
			/**
			 * @brief Get the fill index
			 * @return The index(!) of a fill as defined in XLFills
			 */
			XLStyleIndex fillIndex() const;
			/**
			 * @brief Get the border index
			 * @return The index(!) of a border as defined in XLBorders
			 */
			XLStyleIndex borderIndex() const;
			/**
			 * @brief Get the id of a referred <xf> entry
			 * @return The id referring to an index in cell style formats (cellStyleXfs)
			 * @throw XLException when invoked from cellStyleFormats
			 * @note - only permitted for cellFormats
			 */
			XLStyleIndex xfId() const;
			/**
			 * @brief Report whether number format is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyNumberFormat() const;
			/**
			 * @brief Report whether font is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyFont() const;
			/**
			 * @brief Report whether fill is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyFill() const;
			/**
			 * @brief Report whether border is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyBorder() const;
			/**
			 * @brief Report whether alignment is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyAlignment() const;
			/**
			 * @brief Report whether protection is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool applyProtection() const;
			/**
			 * @brief Report whether quotePrefix is enabled
			 * @return true for a setting enabled, or false if disabled
			 * @note from documentation: A boolean value indicating whether the text string in a cell should be prefixed by a single quote mark
			 *       (e.g., 'text). In these cases, the quote is not stored in the Shared Strings Part.
			 */
			bool quotePrefix() const;

			/**
			 * @brief Report whether pivot button is applied
			 * @return true for a setting enabled, or false if disabled
			 * @note from documentation: A boolean value indicating whether the cell rendering includes a pivot table dropdown button.
			 * @todo need to find a use case for this
			 */
			bool pivotButton() const;

			/**
			 * @brief Report whether protection locked is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool locked() const;

			/**
			 * @brief Report whether protection hidden is applied
			 * @return true for a setting enabled, or false if disabled
			 */
			bool hidden() const;

			/**
			 * @brief Return a reference to applicable alignment
			 * @param createIfMissing triggers creation of alignment node - should be used with setter functions of XLAlignment
			 * @return An XLAlignment object reference
			 */
			XLAlignment alignment(bool createIfMissing = false) const;
			/**
			 * @brief Unsupported getter
			 */
			XLUnsupportedElement extLst() const { return XLUnsupportedElement{}; } // <xf><extLst>...</extLst></xf>
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setNumberFormatId(uint32 newNumFmtId);
			bool setFontIndex(XLStyleIndex newFontIndex);
			bool setFillIndex(XLStyleIndex newFillIndex);
			bool setBorderIndex(XLStyleIndex newBorderIndex);
			bool setXfId(XLStyleIndex newXfId);              // NOTE: throws when invoked from cellStyleFormats
			bool setApplyNumberFormat(bool set = true);
			bool setApplyFont(bool set = true);
			bool setApplyFill(bool set = true);
			bool setApplyBorder(bool set = true);
			bool setApplyAlignment(bool set = true);
			bool setApplyProtection(bool set = true);
			bool setQuotePrefix(bool set = true);
			bool setPivotButton(bool set = true);
			bool setLocked(bool set = true);
			bool setHidden(bool set = true);
			/**
			 * @brief Unsupported setter
			 */
			bool setExtLst(XLUnsupportedElement const& newExtLst);
			/**
			 * @brief Return a string summary of the cell format properties
			 * @return string with info about the cell format object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_cellFormatNode;   /**< An XMLNode object with the cell format (xf) item */
			bool m_permitXfId{false};
			inline static const std::vector< std::string_view > m_nodeOrder = { "alignment", "protection" };
		};
		/**
		 * @brief An encapsulation of the XLSX cell style formats
		 */
		class XLCellFormats {
		public:
			XLCellFormats();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the cell formats (cellXfs or cellStyleXfs) item. If no input is provided, a null node is used.
			 * @param permitXfId Pass-through to XLCellFormat constructor: true (XLPermitXfID) -> setter setXfId is enabled, otherwise throws
			 */
			explicit XLCellFormats(const XMLNode& node, bool permitXfId = false);
			XLCellFormats(const XLCellFormats& other);
			XLCellFormats(XLCellFormats&& other);
			~XLCellFormats();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCellFormats& operator=(const XLCellFormats& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCellFormats& operator=(XLCellFormats&& other) noexcept = default;
			/**
			 * @brief Get the count of cell style format descriptions
			 * @return The amount of cell style format description entries
			 */
			size_t count() const;
			/**
			 * @brief Get the cell style format description identified by index
			 * @param index The index within the XML sequence
			 * @return An XLCellFormat object
			 */
			XLCellFormat cellFormatByIndex(XLStyleIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to cellFormatByIndex
			 * @param index The index within the XML sequence
			 * @return An XLCellFormat object
			 */
			XLCellFormat operator[](XLStyleIndex index) const { return cellFormatByIndex(index); }
			/**
			 * @brief Append a new XLCellFormat, based on copyFrom, and return its index
			 *       in cellXfs (for XLStyles::cellFormats) or cellStyleXfs (for XLStyles::cellStyleFormats)
			 * @param copyFrom Can provide an XLCellFormat to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLCellFormat copyFrom = XLCellFormat{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_cellFormatsNode;  /**< An XMLNode object with the cell formats item */
			std::vector<XLCellFormat> m_cellFormats;
			bool m_permitXfId{false};
		};
		/**
		 * @brief An encapsulation of a cell style item
		 */
		class XLCellStyle {
			friend class XLCellStyles;    // for access to m_cellStyleNode in XLCellStyles::create
		public:
			XLCellStyle();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the cellStyle item. If no input is provided, a null node is used.
			 */
			explicit XLCellStyle(const XMLNode& node);
			XLCellStyle(const XLCellStyle& other);
			XLCellStyle(XLCellStyle&& other) noexcept = default;
			~XLCellStyle();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCellStyle& operator=(const XLCellStyle& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCellStyle& operator=(XLCellStyle&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Get the name of the cell style
			 * @return The name for this cell style entry
			 */
			std::string name() const;
			/**
			 * @brief Get the id of the cell style format
			 * @return The id referring to an index in cell style formats (cellStyleXfs) - TBD to be confirmed
			 */
			XLStyleIndex xfId() const;
			/**
			 * @brief Get the built-in id of the cell style
			 * @return The built-in id of the cell style
			 * @todo need to find a use case for this
			 */
			uint32 builtinId() const;
			/**
			 * @brief Get the outline style id (attribute iLevel) of the cell style
			 * @return The outline style id of the cell style
			 * @todo need to find a use case for this
			 */
			uint32 outlineStyle() const;
			/**
			 * @brief Get the hidden flag of the cell style
			 * @return The hidden flag status (true: applications should not show this style)
			 */
			bool hidden() const;
			/**
			 * @brief Get the custom buildin flag
			 * @return true if this cell style shall customize a built-in style
			 */
			bool customBuiltin() const;
			/**
			 * @brief Unsupported getter
			 */
			XLUnsupportedElement extLst() const { return XLUnsupportedElement{}; } // <cellStyle><extLst>...</extLst></cellStyle>
			/**
			 * @brief Setter functions for style parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setName(std::string newName);
			bool setXfId(XLStyleIndex newXfId);
			bool setBuiltinId(uint32 newBuiltinId);
			bool setOutlineStyle(uint32 newOutlineStyle);
			bool setHidden(bool set = true);
			bool setCustomBuiltin(bool set = true);
			/**
			 * @brief Unsupported setter
			 */
			bool setExtLst(XLUnsupportedElement const& newExtLst);
			/**
			 * @brief Return a string summary of the cell style properties
			 * @return string with info about the cell style object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_cellStyleNode;    /**< An XMLNode object with the cell style item */
		};
		/**
		 * @brief An encapsulation of the XLSX cell styles
		 */
		class XLCellStyles {
		public:
			XLCellStyles();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the cellStyles item. If no input is provided, a null node is used.
			 */
			explicit XLCellStyles(const XMLNode& node);
			XLCellStyles(const XLCellStyles& other);
			XLCellStyles(XLCellStyles&& other);
			~XLCellStyles();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCellStyles& operator=(const XLCellStyles& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCellStyles& operator=(XLCellStyles&& other) noexcept = default;

			/**
			 * @brief Get the count of cell styles
			 * @return The amount of entries in the cell styles
			 */
			size_t count() const;

			/**
			 * @brief Get the cell style identified by index
			 * @return An XLCellStyle object
			 */
			XLCellStyle cellStyleByIndex(XLStyleIndex index) const;

			/**
			 * @brief Operator overload: allow [] as shortcut access to cellStyleByIndex
			 * @param index The index within the XML sequence
			 * @return An XLCellStyle object
			 */
			XLCellStyle operator[](XLStyleIndex index) const { return cellStyleByIndex(index); }

			/**
			 * @brief Append a new XLCellStyle, based on copyFrom, and return its index in cellStyles node
			 * @param copyFrom Can provide an XLCellStyle to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new style as used by operator[]
			 */
			XLStyleIndex create(XLCellStyle copyFrom = XLCellStyle{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_cellStylesNode;   /**< An XMLNode object with the cell styles item */
			std::vector<XLCellStyle> m_cellStyles;
		};
		/**
		 * @brief An encapsulation of a differential cell format item
		 */
		class XLDiffCellFormat {
			friend class XLDiffCellFormats;    // for access to m_diffCellFormatNode in XLDiffCellFormats::create
		public:
			XLDiffCellFormat();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the dxf item. If no input is provided, a null node is used.
			 */
			explicit XLDiffCellFormat(const XMLNode& node);
			XLDiffCellFormat(const XLDiffCellFormat& other);
			XLDiffCellFormat(XLDiffCellFormat&& other) noexcept = default;
			~XLDiffCellFormat();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLDiffCellFormat& operator=(const XLDiffCellFormat& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLDiffCellFormat& operator=(XLDiffCellFormat&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Getter functions, will create empty object on access and can be used to manipulate underlying setters
			 */
			XLFont font() const;
			XLNumberFormat numFmt() const;
			XLFill fill() const;
			XLAlignment alignment() const;
			XLBorder border() const;
			// setProtection   ();
			/**
			 * @brief Unsupported getter
			 */
			XLUnsupportedElement extLst() const { return XLUnsupportedElement{}; } // <cellStyle><extLst>...</extLst></cellStyle>
			/**
			 * @brief Setter functions for differential cell format protection settings
			 * @return true for success, false for failure
			 */
			// bool setProtection   ();
			/**
			 * @brief Unsupported setter
			 */
			bool setExtLst(XLUnsupportedElement const& newExtLst);
			/**
			 * @brief Return a string summary of the differential cell format properties
			 * @return string with info about the differential cell format object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_diffCellFormatNode;    /**< An XMLNode object with the cell style item */
		};
		/**
		 * @brief An encapsulation of the XLSX differential cell formats
		 */
		class XLDiffCellFormats {
		public:
			XLDiffCellFormats();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the dxfs item. If no input is provided, a null node is used.
			 */
			explicit XLDiffCellFormats(const XMLNode& node);
			XLDiffCellFormats(const XLDiffCellFormats& other);
			XLDiffCellFormats(XLDiffCellFormats&& other);
			~XLDiffCellFormats();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLDiffCellFormats& operator=(const XLDiffCellFormats& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLDiffCellFormats& operator=(XLDiffCellFormats&& other) noexcept = default;
			/**
			 * @brief Get the count of differential cell formats
			 * @return The amount of entries in the differential cell formats
			 */
			size_t count() const;
			/**
			 * @brief Get the differential cell format identified by index
			 * @return An XLDiffCellFormat object
			 */
			XLDiffCellFormat diffCellFormatByIndex(XLStyleIndex index) const;

			/**
			 * @brief Operator overload: allow [] as shortcut access to diffCellFormatByIndex
			 * @param index The index within the XML sequence
			 * @return An XLDiffCellFormat object
			 */
			XLDiffCellFormat operator[](XLStyleIndex index) const { return diffCellFormatByIndex(index); }

			/**
			 * @brief Append a new XLDiffCellFormat, based on copyFrom, and return its index in dxfs node
			 * @param copyFrom Can provide an XLDiffCellFormat to use as template for the new style
			 * @param styleEntriesPrefix Prefix the newly created cell style XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new differential cell format as used by operator[]
			 */
			XLStyleIndex create(XLDiffCellFormat copyFrom = XLDiffCellFormat{}, std::string styleEntriesPrefix = XLDefaultStyleEntriesPrefix);
		private:
			std::unique_ptr<XMLNode> m_diffCellFormatsNode;   /**< An XMLNode object with the cell styles item */
			std::vector<XLDiffCellFormat> m_diffCellFormats;
		};
		/**
		 * @brief An encapsulation of the styles file (xl/styles.xml) in an Excel document package.
		 */
		class XLStyles : public XLXmlFile {
		public:
			XLStyles();
			/**
			 * @param suppressWarnings if true (SUPRESS_WARNINGS), messages such as "XLStyles: Ignoring currently unsupported <dxfs> node" will be silenced
			 * @param stylesPrefix Prefix any newly created root style nodes with this text as pugi::node_pcdata
			 */
			explicit XLStyles(XLXmlData* xmlData, bool suppressWarnings = false, std::string stylesPrefix = XLDefaultStylesPrefix);
			~XLStyles();
			/**
			 * @brief The move constructor.
			 * @param other an existing styles object other will be assigned to this
			 */
			XLStyles(XLStyles&& other) noexcept;
			/**
			 * @brief The copy constructor.
			 * @param other an existing styles object other will be also referred by this
			 */
			XLStyles(const XLStyles& other);
			XLStyles& operator=(XLStyles&& other) noexcept;
			XLStyles& operator=(const XLStyles& other);
			/**
			 * @brief Get the number formats object
			 * @return An XLNumberFormats object
			 */
			XLNumberFormats& numberFormats() const;
			/**
			 * @brief Get the fonts object
			 * @return An XLFonts object
			 */
			XLFonts& fonts() const;
			/**
			 * @brief Get the fills object
			 * @return An XLFills object
			 */
			XLFills& fills() const;
			/**
			 * @brief Get the borders object
			 * @return An XLBorders object
			 */
			XLBorders& borders() const;
			/**
			 * @brief Get the cell style formats object
			 * @return An XLCellFormats object
			 */
			XLCellFormats& cellStyleFormats() const;
			/**
			 * @brief Get the cell formats object
			 * @return An XLCellFormats object
			 */
			XLCellFormats& cellFormats() const;

			/**
			 * @brief Get the cell styles object
			 * @return An XLCellStyles object
			 */
			XLCellStyles& cellStyles() const;
			/**
			 * @brief Get the differential cell formats object
			 * @return An XLDiffCellFormats object
			 */
			XLDiffCellFormats& diffCellFormats() const;
		private:
			bool m_suppressWarnings;                                // if true, will suppress output of warnings where supported
			std::unique_ptr<XLNumberFormats>    m_numberFormats;    // handle to the underlying number formats
			std::unique_ptr<XLFonts>            m_fonts;            // handle to the underlying fonts
			std::unique_ptr<XLFills>            m_fills;            // handle to the underlying fills
			std::unique_ptr<XLBorders>          m_borders;          // handle to the underlying border descriptions
			std::unique_ptr<XLCellFormats>      m_cellStyleFormats; // handle to the underlying cell style formats descriptions
			std::unique_ptr<XLCellFormats>      m_cellFormats;      // handle to the underlying cell formats descriptions
			std::unique_ptr<XLCellStyles>       m_cellStyles;       // handle to the underlying cell styles
			std::unique_ptr<XLDiffCellFormats>  m_diffCellFormats;  // handle to the underlying differential cell formats
		};
	}
	//
	//#include "XLCellValue.hpp"
	typedef std::variant<std::string, int64_t, double, bool> XLCellValueType; // TBD: typedef std::variant< std::string, int64_t, double, bool, struct timestamp > XLCellValueType;

	namespace OpenXLSX {
		class XLCellValueProxy;
		class XLCell;
		/**
		 * @brief Enum defining the valid value types for a an Excel spreadsheet cell.
		 */
		enum class XLValueType { Empty, Boolean, Integer, Float, Error, String };
		//
		// Private Struct to enable XLValueType conversion to double
		//
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
		//
		// Private Struct to enable XLValueType conversion to std::string
		//
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
			XLCellValue(T value)
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
			XLCellValue& operator=(const XLCellValue& other);
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
			template <typename T, 
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
			double getDouble() const 
			{
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
			~XLCellValueProxy();
			XLCellValueProxy& operator=(const XLCellValueProxy& other);
			/**
			 * @brief Templated assignment operator
			 * @tparam T The type of numberValue assigned to the object.
			 * @param value The value.
			 * @return A reference to the current object.
			 */
			template <typename T, typename = std::enable_if_t<
					std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
					std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
					std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLCellValue> || std::is_same_v<T, XLDateTime> > >
			XLCellValueProxy& operator=(T value)
			{
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
						case XLValueType::Boolean: setBoolean(value.template get<bool>()); break;
						case XLValueType::Integer: setInteger(value.template get<int64_t>()); break;
						case XLValueType::Float: setFloat(value.template get<double>()); break;
						case XLValueType::String: setString(value.template privateGet<const char*>()); break;
						case XLValueType::Empty: clear(); break;
						default: setError("#N/A"); break;
					}
				}
				return *this;
			}
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
			operator XLCellValue() const;
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

			XLCell*  m_cell;     /**< Pointer to the owning XLCell object. */
			XMLNode* m_cellNode; /**< Pointer to corresponding XML cell node. */
		};
		//
		// TODO: Consider comparison operators on fundamental datatypes
		//
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
	}

	template <> struct std::hash<OpenXLSX::XLCellValue>
	{
		std::size_t operator()(const OpenXLSX::XLCellValue& value) const noexcept
		{
			return std::hash<XLCellValueType> {}(value.m_value);
		}
	};
	//
	//#include "XLCellReference.hpp"
	namespace OpenXLSX {
		using XLCoordinates = std::pair<uint32, uint16>;

		class XLCellReference final {
			friend bool operator==(const XLCellReference& lhs, const XLCellReference& rhs);
			friend bool operator!=(const XLCellReference& lhs, const XLCellReference& rhs);
			friend bool operator<(const XLCellReference& lhs, const XLCellReference& rhs);
			friend bool operator>(const XLCellReference& lhs, const XLCellReference& rhs);
			friend bool operator<=(const XLCellReference& lhs, const XLCellReference& rhs);
			friend bool operator>=(const XLCellReference& lhs, const XLCellReference& rhs);
		public:
			/**
			 * @brief Constructor taking a cell address as argument.
			 * @param cellAddress The address of the cell, e.g. 'A1'.
			 * @details The constructor creates a new XLCellReference from a string, e.g. 'A1'. If there's no input,
			 * the default reference will be cell A1.
			 */
			XLCellReference(const std::string& cellAddress = "");
			/**
			 * @brief Constructor taking the cell coordinates as arguments.
			 * @param row The row number of the cell.
			 * @param column The column number of the cell.
			 */
			XLCellReference(uint32 row, uint16 column);
			/**
			 * @brief Constructor taking the row number and the column letter as arguments.
			 * @param row The row number of the cell.
			 * @param column The column letter of the cell.
			 */
			XLCellReference(uint32 row, const std::string& column);
			/**
			 * @brief Copy constructor
			 * @param other The object to be copied.
			 */
			XLCellReference(const XLCellReference& other);
			XLCellReference(XLCellReference&& other) noexcept;
			~XLCellReference();
			/**
			 * @brief Assignment operator.
			 * @param other The object to be copied/assigned.
			 * @return A reference to the new object.
			 */
			XLCellReference& operator=(const XLCellReference& other);
			XLCellReference& operator=(XLCellReference&& other) noexcept;
			XLCellReference& operator++();
			XLCellReference operator++(int);
			XLCellReference& operator--();
			XLCellReference operator--(int);
			/**
			 * @brief Get the row number of the XLCellReference.
			 * @return The row.
			 */
			uint32 row() const;
			/**
			 * @brief Set the row number for the XLCellReference.
			 * @param row The row number.
			 */
			void setRow(uint32 row);
			/**
			 * @brief Get the column number of the XLCellReference.
			 * @return The column number.
			 */
			uint16 column() const;
			/**
			 * @brief Set the column number of the XLCellReference.
			 * @param column The column number.
			 */
			void setColumn(uint16 column);
			/**
			 * @brief Set both row and column number of the XLCellReference.
			 * @param row The row number.
			 * @param column The column number.
			 */
			void setRowAndColumn(uint32 row, uint16 column);
			/**
			 * @brief Get the address of the XLCellReference
			 * @return The address, e.g. 'A1'
			 */
			std::string address() const;
			/**
			 * @brief Set the address of the XLCellReference
			 * @param address The address, e.g. 'A1'
			 * @pre The address input string must be a valid Excel cell reference. Otherwise the behaviour is undefined.
			 */
			void setAddress(const std::string& address);
			// private:
			static std::string rowAsString(uint32 row);
			static uint32 rowAsNumber(const std::string& row);
			/**
			 * @brief Static helper function to convert column number to column letter (e.g. column 1 becomes 'A')
			 * @param column The column number.
			 * @return The column letter
			 */
			static std::string columnAsString(uint16 column);
			/**
			 * @brief Static helper function to convert column letter to column number (e.g. column 'A' becomes 1)
			 * @param column The column letter, e.g. 'A'
			 * @return The column number.
			 */
			static uint16 columnAsNumber(const std::string& column);
			/**
			 * @brief Static helper function to convert cell address to coordinates.
			 * @param address The address to be converted, e.g. 'A1'
			 * @return A std::pair<row, column>
			 */
			static XLCoordinates coordinatesFromAddress(const std::string& address);
		private:
			uint32 m_row { 1 };               /**< The row */
			uint16 m_column { 1 };            /**< The column */
			std::string m_cellAddress { "A1" }; /**< The address, e.g. 'A1' */
		};
		/**
		 * @brief Helper function to check equality between two XLCellReferences.
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return true if equal; otherwise false.
		 */
		inline bool operator==(const XLCellReference& lhs, const XLCellReference& rhs)
		{
			return lhs.row() == rhs.row() && lhs.column() == rhs.column();
		}
		/**
		 * @brief Helper function to check for in-equality between two XLCellReferences
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return false if equal; otherwise true.
		 */
		inline bool operator!=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs == rhs); }
		/**
		 * @brief Helper function to check if one XLCellReference is smaller than another.
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return true if lhs < rhs; otherwise false.
		 */
		inline bool operator<(const XLCellReference& lhs, const XLCellReference& rhs)
		{
			return lhs.row() < rhs.row() || (lhs.row() <= rhs.row() && lhs.column() < rhs.column());
		}
		/**
		 * @brief Helper function to check if one XLCellReference is larger than another.
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return true if lhs > rhs; otherwise false.
		 */
		inline bool operator>(const XLCellReference& lhs, const XLCellReference& rhs) { return (rhs < lhs); }
		/**
		 * @brief Helper function to check if one XLCellReference is smaller than or equal to another.
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return true if lhs <= rhs; otherwise false
		 */
		inline bool operator<=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs > rhs); }
		/**
		 * @brief Helper function to check if one XLCellReference is larger than or equal to another.
		 * @param lhs The first XLCellReference
		 * @param rhs The second XLCellReference
		 * @return true if lhs >= rhs; otherwise false.
		 */
		inline bool operator>=(const XLCellReference& lhs, const XLCellReference& rhs) { return !(lhs < rhs); }
		//
		//#include "XLSharedStrings.hpp"
		constexpr size_t XLMaxSharedStrings = (std::numeric_limits< int32_t >::max)();       // pull request #261: wrapped max in parentheses to prevent expansion of windows.h "max" macro

		class XLSharedStrings; // forward declaration
		typedef std::reference_wrapper< const XLSharedStrings > XLSharedStringsRef;

		extern const XLSharedStrings XLSharedStringsDefaulted;     // to be used for default initialization of all references of type XLSharedStrings
		/**
		 * @brief This class encapsulate the Excel concept of Shared Strings. In Excel, instead of havig individual strings
		 * in each cell, cells have a reference to an entry in the SharedStrings register. This results in smalle file
		 * sizes, as repeated strings are referenced easily.
		 */
		class XLSharedStrings : public XLXmlFile {
			friend class XLDocument; // for access to protected function rewriteXmlFromCache
		public:
			XLSharedStrings() = default;
			explicit XLSharedStrings(XLXmlData* xmlData, std::deque<std::string>* stringCache);
			~XLSharedStrings();
			XLSharedStrings(const XLSharedStrings& other) = default;
			XLSharedStrings(XLSharedStrings&& other) noexcept = default;
			XLSharedStrings& operator=(const XLSharedStrings& other) = default;
			XLSharedStrings& operator=(XLSharedStrings&& other) noexcept = default;
			/**
			 * @brief return the amount of shared string entries currently in the cache
			 */
			int32_t stringCount() const { return m_stringCache->size(); }
			int32_t getStringIndex(const std::string& str) const;
			bool stringExists(const std::string& str) const;
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
		//
		//#include "XLCell.hpp"
		// ===== Flags that can be passed to XLCell::clear as parameter keep, flags can be combined with bitwise OR
		//                                  // Do not clear the cell's:
		constexpr const uint32 XLKeepCellStyle   =  1;         // style (attribute s)
		constexpr const uint32 XLKeepCellType    =  2;    // type (attribute t)
		constexpr const uint32 XLKeepCellValue   =  4;     // value (child node v)
		constexpr const uint32 XLKeepCellFormula =  8;     // formula (child node f)

		class XLCellRange;
		/**
		 * @brief An implementation class encapsulating the properties and behaviours of a spreadsheet cell.
		 */
		class XLCell {
			friend class XLCellIterator;
			friend class XLCellValueProxy;
			friend class XLRowDataIterator;
			friend bool operator==(const XLCell& lhs, const XLCell& rhs);
			friend bool operator!=(const XLCell& lhs, const XLCell& rhs);
		public:
			XLCell();
			XLCell(const XMLNode& cellNode, const XLSharedStrings& sharedStrings);
			/**
			 * @brief Copy constructor
			 * @param other The XLCell object to be copied.
			 * @note The copy constructor has been deleted, as it makes no sense to copy a cell. If the objective is to
			 * copy the getValue, create the the target object and then use the copy assignment operator.
			 */
			XLCell(const XLCell& other);
			/**
			 * @brief Move constructor
			 * @param other The XLCell object to be moved
			 * @note The move constructor has been deleted, as it makes no sense to move a cell.
			 */
			XLCell(XLCell&& other) noexcept;
			virtual ~XLCell();
			/**
			 * @brief Copy assignment operator
			 * @param other The XLCell object to be copy assigned
			 * @return A reference to the new object
			 * @note Copies only the cell contents, not the pointer to parent worksheet etc.
			 */
			virtual XLCell& operator=(const XLCell& other);
			/**
			 * @brief Move assignment operator
			 * @param other The XLCell object to be move assigned
			 * @return A reference to the new object
			 * @note The move assignment constructor has been deleted, as it makes no sense to move a cell.
			 */
			virtual XLCell& operator=(XLCell&& other) noexcept;
			/**
			 * @brief Copy contents of a cell, value & formula
			 * @param other The XLCell object from which to copy
			 */
			void copyFrom(XLCell const& other);
			/**
			 * @brief test if cell object has no (valid) content
			 */
			bool empty() const;
			/**
			 * @brief opposite of empty()
			 */
			explicit operator bool() const;
			/**
			 * @brief clear all cell content and attributes except for the cell reference (attribute r)
			 * @param keep do not clear cell properties whose flags are set in keep (XLKeepCellStyle, XLKeepCellType,
			 *              XLKeepCellValue, XLKeepCellFormula), flags can be combined with bitwise OR
			 * @note due to the way OOXML separates comments from the cells, this function will *not* clear a cell comment - refer to XLComments& XLSheet::comments() for that
			 */
			void clear(uint32 keep);
			XLCellValueProxy& value();
			const XLCellValueProxy& value() const;
			/**
			 * @brief get the XLCellReference object for the cell.
			 * @return A reference to the cells' XLCellReference object.
			 */
			XLCellReference cellReference() const;
			/**
			 * @brief get the XLCell object from the current cell offset
			 * @return A reference to the XLCell object.
			 */
			XLCell offset(uint16 rowOffset, uint16 colOffset) const;
			/**
			 * @brief test if cell has a formula (XML) node, even if it is an empty string
			 * @return true if XML has a formula node, empty or not - otherwise false
			 */
			bool hasFormula() const;
			XLFormulaProxy& formula();
			const XLFormulaProxy& formula() const;
			std::string getString() const { return value().getString(); }
			/**
			 * @brief Get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style used in this cell.
			 *        This value is stored in the s attribute of a cell like so: s="2"
			 * @returns The index of the applicable format style
			 */
			XLStyleIndex cellFormat() const;
			/**
			 * @brief Set the cell style (attribute s) with a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
			 * @param cellFormatIndex The style to set, corresponding to the nidex of XLStyles::cellStyles()
			 * @returns True on success, false on failure
			 */
			bool setCellFormat(XLStyleIndex cellFormatIndex);
			/**
			 * @brief Print the XML contents of the XLCell using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			static bool isEqual(const XLCell& lhs, const XLCell& rhs);

			std::unique_ptr<XMLNode> m_cellNode;      /**< A pointer to the root XMLNode for the cell. */
			XLSharedStringsRef m_sharedStrings;
			XLCellValueProxy m_valueProxy;
			XLFormulaProxy m_formulaProxy;
		};

		class XLCellAssignable : public XLCell {
		public:
			/**
			 * @brief Default constructor. Constructs a null object.
			 */
			XLCellAssignable() : XLCell() {}
			XLCellAssignable (XLCell const & other);
			XLCellAssignable (XLCell && other);
			// /**
			//  * @brief Inherit all constructors with parameters from XLCell
			//  */
			// template<class base>
			// // explicit XLCellAssignable(base b) : XLCell(b)
			// // NOTE: BUG: explicit keyword triggers tons of compiler errors when << operator attempts to use an XLCell (implicit conversion works because << is overloaded for XLCellAssignable)
			// XLCellAssignable(base b) : XLCell(b)
			// {}

			/**
			 * @brief Copy assignment operator
			 * @param other The XLCell object to be copy assigned
			 * @return A reference to the new object
			 * @note Copies only the cell contents, not the pointer to parent worksheet etc.
			 */
			XLCellAssignable& operator=(const XLCell& other) override;
			XLCellAssignable& operator=(const XLCellAssignable& other);
			/**
			 * @brief Move assignment operator -> overrides XLCell copy operator, becomes a copy operator
			 * @param other The XLCell object to be copy assigned
			 * @return A reference to the new object
			 * @note Copies only the cell contents, not the pointer to parent worksheet etc.
			 */
			XLCellAssignable& operator=(XLCell&& other) noexcept override;
			XLCellAssignable& operator=(XLCellAssignable&& other) noexcept;
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
			XLCellAssignable& operator=(T value)
			{
				XLCell::value() = value; // forward implementation to templated XLCellValue& XLCellValue::operator=(T value)
				return *this;
			}
		};
	}

	namespace OpenXLSX {
		inline bool operator==(const XLCell& lhs, const XLCell& rhs) { return XLCell::isEqual(lhs, rhs); }
		inline bool operator!=(const XLCell& lhs, const XLCell& rhs) { return !XLCell::isEqual(lhs, rhs); }
		/**
		 * @brief      ostream output of XLCell content as string
		 * @param os   the ostream destination
		 * @param c    the cell to output to the stream
		 */
		inline std::ostream& operator<<(std::ostream& os, const XLCell& c)
		{
			os << c.getString();
			// TODO: send to stream different data types based on cell data type
			return os;
		}
		/**
		 * @brief      ostream output of XLCellAssignable content as string
		 * @param os   the ostream destination
		 * @param c    the cell to output to the stream
		 */
		inline std::ostream& operator<<(std::ostream& os, const XLCellAssignable& c)
		{
			os << c.getString();
			// TODO: send to stream different data types based on cell data type
			return os;
		}
		//
		//#include "XLCellIterator.hpp"
		/**
		 * @brief locate the XML row node within sheetDataNode for the row at rowNumber
		 * @param sheetDataNode the XML sheetData node to search in
		 * @param rowNumber the number of the row to locate
		 * @return the XMLNode pointing to the row, or an empty XMLNode if the row does not exist
		 */
		XMLNode findRowNode(XMLNode sheetDataNode, uint32 rowNumber);
		/**
		 * @brief locate the XML cell node within rownode for the cell at columnNumber
		 * @param rowNode the XML node of the row to search in
		 * @param columnNumber the column number of the cell to locate
		 * @return the XMLNode pointing to the cell, or an empty XMLNode if the cell does not exist
		 */
		XMLNode findCellNode(XMLNode rowNode, uint16 columnNumber);

		class XLCellIterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type        = XLCell;
			using difference_type   = int64_t;
			using pointer           = XLCell*;
			using reference         = XLCell&;
			/**
			 * @param colStyles is an optional vector with all column styles configured for the worksheet when the iterator is created.
			 *                  This vector - if provided - will be used to get default cell styles for newly created cells, instead of performing
			 *                   a potentially expensive lookup in <cols>
			 */
			explicit XLCellIterator(const XLCellRange& cellRange, XLIteratorLocation loc, std::vector<XLStyleIndex> const * colStyles);
			~XLCellIterator();
			XLCellIterator(const XLCellIterator& other);
			[[maybe_unused]] XLCellIterator(XLCellIterator&& other) noexcept;
			XLCellIterator& operator=(const XLCellIterator& other);
			XLCellIterator& operator=(XLCellIterator&& other) noexcept;
		private:        // ===== Switch to private method that is used by the XLIterator increment operator++ and the dereference operators * and ->
			static constexpr const bool XLCreateIfMissing      = true;     // code readability for updateCurrentCell parameter createIfMissing
			static constexpr const bool XLDoNotCreateIfMissing = false;    //   "
			/**
			 * @brief update m_currentCell by fetching (or inserting) a cell at m_currentRow, m_currentColumn
			 * @param createIfMissing m_currentCell will only be inserted if createIfMissing is true
			 */
			void updateCurrentCell(bool createIfMissing);
		public:
			XLCellIterator& operator++();
			XLCellIterator operator++(int);
			reference operator*();
			pointer operator->();
			bool operator==(const XLCellIterator& rhs) const;
			bool operator!=(const XLCellIterator& rhs) const;
			/**
			 * @brief determine whether the cell that the iterator points to exists (cell at m_currentRow, m_currentColumn)
			 * @return true if XML already has an entry for that cell, otherwise false
			 */
			bool cellExists();
			/**
			 * @brief determine whether iterator is at 1 beyond the last cell in range
			 * @note 28-07-2024: Removed const from return type (Troldal)
			 */
			bool endReached() const { return m_endReached; }
			uint64_t distance(const XLCellIterator& last);
			/**
			 * @brief get the XLCellReference::address corresponding to the current iterator position
			 * @return an XLCellReference::address, with m_bottomRight.col() + 1 for the beyond-the-end iterator
			 * @note 28-07-2024: Removed const from return type (Troldal)
			 */
			std::string address() const;
		private:
			std::unique_ptr<XMLNode> m_dataNode;
			XLCellReference m_topLeft;           /**< The cell reference of the first cell in the range */
			XLCellReference m_bottomRight;       /**< The cell reference of the last cell in the range */
			XLSharedStringsRef m_sharedStrings;
			bool m_endReached;
			XMLNode m_hintNode;                  /**< The cell node of the last existing cell found up to current iterator position */
			uint32 m_hintRow;                  /**<   the row number for m_hintCell */
			XLCell m_currentCell;                /**< The cell to which the iterator is currently pointing, if it exists, otherwise an empty XLCell */
			static constexpr const int XLNotLoaded  = 0; // code readability for m_currentCellStatus
			static constexpr const int XLNoSuchCell = 1;
			static constexpr const int XLLoaded     = 2;
			int m_currentCellStatus;             /**< Status of m_currentCell: XLNotLoaded, XLNoSuchCell or XLLoaded */
			uint32 m_currentRow;
			uint16 m_currentColumn;
			std::vector<XLStyleIndex> const * m_colStyles;
		};
		/**
		 * @brief      ostream output of XLIterator position as XLCellReference::address
		 * @param os   the ostream destination
		 * @param it    the XLIterator whose position to send to the stream
		 */
		inline std::ostream& operator<<(std::ostream& os, const XLCellIterator& it)
		{
			os << it.address();
			return os;
		}
	}

	// ===== Template specialization for std::distance.
	namespace std {
		using OpenXLSX::XLCellIterator;
		template <> inline std::iterator_traits<XLCellIterator>::difference_type distance<XLCellIterator>(XLCellIterator first, XLCellIterator last)
		{
			return static_cast<std::iterator_traits<XLCellIterator>::difference_type>(first.distance(last));
		}
	}
	//
	//#include "XLCellRange.hpp"
	namespace OpenXLSX {
		/**
		 * @brief This class encapsulates the concept of a cell range, i.e. a square area
		 * (or subset) of cells in a spreadsheet.
		 */
		class XLCellRange {
			friend class XLCellIterator;
		public:
			XLCellRange();
			explicit XLCellRange(const XMLNode & dataNode, const XLCellReference& topLeft, const XLCellReference& bottomRight, const XLSharedStrings& sharedStrings);
			XLCellRange(const XLCellRange& other);
			XLCellRange(XLCellRange&& other) noexcept;
			~XLCellRange();
			/**
			 * @brief The copy assignment operator [default]
			 * @param other The range object to be copied and assigned.
			 * @return A reference to the new object.
			 * @throws A std::range_error if the source range and destination range are of different size and shape.
			 * @note This implements the default copy assignment operator.
			 */
			XLCellRange& operator=(const XLCellRange& other);
			/**
			 * @brief The move assignment operator [default].
			 * @param other The range object to be moved and assigned.
			 * @return A reference to the new object.
			 * @note This implements the default move assignment operator.
			 */
			XLCellRange& operator=(XLCellRange&& other) noexcept;
			/**
			 * @brief populate the m_columnStyles
			 * @return a const XLCellReference
			 */
			void fetchColumnStyles();
			/**
			 * @brief get the top left cell
			 * @return a const XLCellReference
			 */
			const XLCellReference topLeft() const;
			/**
			 * @brief get the bottom right cell
			 * @return a const XLCellReference
			 */
			const XLCellReference bottomRight() const;
			/**
			 * @brief get the string reference that corresponds to the represented cell range
			 * @return a std::string range reference, e.g. "A2:Z5"
			 */
			std::string address() const;
			/**
			 * @brief Get the number of rows in the range.
			 * @return The number of rows.
			 */
			uint32 numRows() const;
			/**
			 * @brief Get the number of columns in the range.
			 * @return The number of columns.
			 */
			uint16 numColumns() const;
			XLCellIterator begin() const;
			XLCellIterator end() const;
			void clear();
			/**
			 * @brief Templated assignment operator - assign value to a range of cells
			 * @tparam T The type of the value argument.
			 * @param value The value.
			 * @return A reference to the assigned-to object.
			 */
			template <typename T, typename = std::enable_if_t<
					std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
					std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
					std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
			XLCellRange& operator=(T value)
			{
				// forward implementation to templated XLCellValue& XLCellValue::operator=(T value)
				for(auto it = begin(); it != end(); ++it)  
					it->value() = value;
				return *this;
			}
			/**
			 * @brief Set cell format for a range of cells
			 * @param cellFormatIndex The style to set, corresponding to the nidex of XLStyles::cellStyles()
			 * @returns true on success, false on failure
			 */
			bool setFormat(XLStyleIndex cellFormatIndex);
		private:
			std::unique_ptr<XMLNode>  m_dataNode;
			XLCellReference m_topLeft;                 /**< reference to the first cell in the range */
			XLCellReference m_bottomRight;             /**< reference to the last cell in the range */
			XLSharedStringsRef m_sharedStrings;        /**< reference to the document shared strings table */
			std::vector<XLStyleIndex> m_columnStyles;  /**< quick access to column styles in the range - populated by fetchColumnStyles() */
		};
		//
		//#include "XLColumn.hpp"
		class XLColumn {
		public:
			explicit XLColumn(const XMLNode& columnNode);
			XLColumn(const XLColumn& other);
			XLColumn(XLColumn&& other) noexcept;
			~XLColumn();
			XLColumn& operator=(const XLColumn& other);
			XLColumn& operator=(XLColumn&& other) noexcept = default;
			float width() const;
			void setWidth(float width);
			bool isHidden() const;
			void setHidden(bool state);
			XMLNode & columnNode() const;
			/**
			 * @brief Get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style assigned to the column.
			 *        This value is stored in the col attributes like so: style="2"
			 * @returns The index of the applicable format style
			 */
			XLStyleIndex format() const;
			/**
			 * @brief Set the column style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
			 * @param cellFormatIndex The style to set, corresponding to the index of XLStyles::cellStyles()
			 * @returns true on success, false on failure
			 */
			bool setFormat(XLStyleIndex cellFormatIndex);
		private:
			std::unique_ptr<XMLNode> m_columnNode; /**< A pointer to the XMLNode object for the column. */
		};
		//
		//#include "IZipArchive.hpp"
		/**
		 * @brief This class functions as a wrapper around any class that provides the necessary functionality for
		 * a zip archive.
		 * @details This class works by applying 'type erasure'. This enables the use of objects of any class, the only
		 * requirement being that it provides the right interface. No inheritance from a base class is needed.
		 */
		class IZipArchive {
		public:
			IZipArchive() : m_zipArchive() 
			{
			}
			/**
			 * @brief Constructor, taking the target object as an argument.
			 * @tparam T The type of the target object (will be auto deducted)
			 * @param x The target object
			 * @note This method is deliberately not marked 'explicit', because as a templated constructor, it should be able
			 * to take any type as an argument. However, only objects that satisfy the required interface can be used.
			 */
			template <typename T> IZipArchive(const T& zipArchive) : m_zipArchive{std::make_unique<Model<T> >(zipArchive)} 
			{
			}
			IZipArchive(const IZipArchive& other) : m_zipArchive(other.m_zipArchive ? other.m_zipArchive->clone() : nullptr) 
			{
			}
			IZipArchive(IZipArchive && other) noexcept = default;
			~IZipArchive() = default;
			template <typename T> inline IZipArchive& operator=(const T& zipArchive)
			{
				m_zipArchive = std::make_unique<Model<T> >(zipArchive);
				return *this;
			}
			inline IZipArchive& operator=(const IZipArchive& other)
			{
				IZipArchive copy(other);
				*this = std::move(copy);
				return *this;
			}
			inline IZipArchive& operator=(IZipArchive&& other) noexcept = default;
			inline explicit operator bool() const { return isValid(); }
			inline bool isValid() const { return m_zipArchive->isValid(); }
			inline bool isOpen() const { return m_zipArchive->isOpen(); }
			inline void open(const std::string& fileName) { m_zipArchive->open(fileName); }
			inline void close() const { m_zipArchive->close(); }
			inline void save(const std::string& path) { m_zipArchive->save(path); }
			inline void addEntry(const std::string& name, const std::string& data) { m_zipArchive->addEntry(name, data); }
			inline void deleteEntry(const std::string& entryName) { m_zipArchive->deleteEntry(entryName); }
			inline std::string getEntry(const std::string& name) { return m_zipArchive->getEntry(name); }
			inline bool hasEntry(const std::string& entryName) const { return m_zipArchive->hasEntry(entryName); }
		private:
			struct Concept {
			public:
				Concept() = default;
				Concept(const Concept&) = default;
				Concept(Concept&&) noexcept = default;
				virtual ~Concept() = default;
				inline Concept& operator=(const Concept&) = default;
				inline Concept& operator=(Concept&&) noexcept = default;
				inline virtual std::unique_ptr<Concept> clone() const = 0;
				inline virtual bool isValid() const = 0;
				inline virtual bool isOpen() const = 0;
				inline virtual void open(const std::string& fileName) = 0;
				inline virtual void close() = 0;
				inline virtual void save(const std::string& path) = 0;
				inline virtual void addEntry(const std::string& name, const std::string& data) = 0;
				inline virtual void deleteEntry(const std::string& entryName) = 0;
				inline virtual std::string getEntry(const std::string& name) = 0;
				inline virtual bool hasEntry(const std::string& entryName) const = 0;
			};

			template <typename T> struct Model : Concept {
			public:
				explicit Model(const T& x) : ZipType(x) 
				{
				}
				Model(const Model& other) = default;
				Model(Model&& other) noexcept = default;
				~Model() override = default;
				inline Model& operator=(const Model& other) = default;
				inline Model& operator=(Model&& other) noexcept = default;
				inline std::unique_ptr<Concept> clone() const override { return std::make_unique<Model<T> >(ZipType); }
				inline bool isValid() const override { return ZipType.isValid(); }
				inline bool isOpen() const override { return ZipType.isOpen(); }
				inline void open(const std::string& fileName) override { ZipType.open(fileName); }
				inline void close() override { ZipType.close(); }
				inline void save(const std::string& path) override { ZipType.save(path); }
				inline void addEntry(const std::string& name, const std::string& data) override { ZipType.addEntry(name, data); }
				inline void deleteEntry(const std::string& entryName) override { ZipType.deleteEntry(entryName); }
				inline std::string getEntry(const std::string& name) override { return ZipType.getEntry(name); }
				inline bool hasEntry(const std::string& entryName) const override { return ZipType.hasEntry(entryName); }
		private:
				T ZipType;
			};

			std::unique_ptr<Concept> m_zipArchive;
		};
		//
		//#include "XLCommandQuery.hpp"
		enum class XLCommandType : uint8_t {
			SetSheetName,
			SetSheetColor,
			SetSheetVisibility,
			SetSheetIndex,
			SetSheetActive,
			ResetCalcChain,
			CheckAndFixCoreProperties,
			CheckAndFixExtendedProperties,
			AddSharedStrings,
			AddWorksheet,
			AddChartsheet,
			DeleteSheet,
			CloneSheet,
			AddStyles
		};

		class XLCommand {
		public:
			explicit XLCommand(XLCommandType type) : m_type(type) 
			{
			}
			template <typename T> XLCommand& setParam(const std::string& param, T value)
			{
				m_params[param] = value;
				return *this;
			}
			template <typename T> T getParam(const std::string& param) const
			{
				return std::any_cast<T>(m_params.at(param));
			}
			XLCommandType type() const { return m_type; }
		private:
			XLCommandType m_type;
			std::map<std::string, std::any> m_params;
		};

		enum class XLQueryType : uint8_t {
			QuerySheetName,
			QuerySheetIndex,
			QuerySheetVisibility,
			QuerySheetIsActive,
			QuerySheetType,
			QuerySheetID,
			QuerySheetRelsID,
			QuerySheetRelsTarget,
			QuerySharedStrings,
			QueryXmlData
		};

		class XLQuery {
		public:
			explicit XLQuery(XLQueryType type) : m_type(type) {}
			template <typename T> XLQuery& setParam(const std::string& param, T value)
			{
				m_params[param] = value;
				return *this;
			}
			template <typename T> T getParam(const std::string& param) const { return std::any_cast<T>(m_params.at(param)); }
			template <typename T> XLQuery& setResult(T value)
			{
				m_result = value;
				return *this;
			}
			template <typename T> T result() const { return std::any_cast<T>(m_result); }
			XLQueryType type() const { return m_type; }
		private:
			XLQueryType m_type;
			std::any m_result;
			std::map<std::string, std::any> m_params;
		};
		//
		//#include "XLContentTypes.hpp"
		enum class XLContentType : uint8_t {
			Workbook,
			Relationships,
			WorkbookMacroEnabled,
			Worksheet,
			Chartsheet,
			ExternalLink,
			Theme,
			Styles,
			SharedStrings,
			Drawing,
			Chart,
			ChartStyle,
			ChartColorStyle,
			ControlProperties,
			CalculationChain,
			VBAProject,
			CoreProperties,
			ExtendedProperties,
			CustomProperties,
			Comments,
			Table,
			VMLDrawing,
			Unknown
		};
		/**
		 * @brief utility function: determine the name of an XLContentType value
		 * @param type the XLContentType to get a name for
		 * @return a string with the name of type
		 */
		std::string XLContentTypeToString(XLContentType type);

		class XLContentItem {
			friend class XLContentTypes;
		public:
			XLContentItem();
			explicit XLContentItem(const XMLNode& node);
			~XLContentItem();
			XLContentItem(const XLContentItem& other);
			XLContentItem(XLContentItem&& other) noexcept;
			XLContentItem& operator=(const XLContentItem& other);
			XLContentItem& operator=(XLContentItem&& other) noexcept;
			XLContentType type() const;
			std::string path() const;
		private:
			std::unique_ptr<XMLNode> m_contentNode;
		};
		/**
		 * @brief The purpose of this class is to load, store add and save item in the [Content_Types].xml file.
		 */
		class XLContentTypes : public XLXmlFile {
		public:
			XLContentTypes();
			explicit XLContentTypes(XLXmlData* xmlData);
			~XLContentTypes();
			XLContentTypes(const XLContentTypes& other);
			XLContentTypes(XLContentTypes&& other) noexcept;
			XLContentTypes& operator=(const XLContentTypes& other);
			XLContentTypes& operator=(XLContentTypes&& other) noexcept;
			/**
			 * @brief Add a new override key/getValue pair to the data store.
			 * @param path The key
			 * @param type The getValue
			 */
			void addOverride(const std::string& path, XLContentType type);
			void deleteOverride(const std::string& path);
			void deleteOverride(const XLContentItem& item);
			XLContentItem contentItem(const std::string& path);
			std::vector<XLContentItem> getContentItems();
		};
		//
		//#include "XLXmlData.hpp"
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
			XLXmlSavingDeclaration() : m_version(XLXmlDefaultVersion), m_encoding(XLXmlDefaultEncoding), m_standalone(XLXmlNotStandalone) 
			{
			}
			XLXmlSavingDeclaration(XLXmlSavingDeclaration const & other) = default; // copy constructor
			XLXmlSavingDeclaration(std::string version, std::string encoding, bool standalone = XLXmlNotStandalone) : 
				m_version(version), m_encoding(encoding), m_standalone(standalone) 
			{
			}
			~XLXmlSavingDeclaration() 
			{
			}
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
		class XLXmlData final {
		public:
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
			XLXmlData(XLDocument * parentDoc, const std::string& xmlPath, const std::string& xmlId = "", XLContentType xmlType = XLContentType::Unknown);
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
			XLDocument * m_parentDoc {}; /**< A pointer to the parent XLDocument object. >*/
			std::string m_xmlPath {};                            /**< The path of the XML data in the .xlsx zip archive. >*/
			std::string m_xmlID {};                              /**< The relationship ID of the XML data. >*/
			XLContentType m_xmlType {};                          /**< The type represented by the XML data. >*/
			mutable std::unique_ptr<OXlXmlDoc> m_xmlDoc;       /**< The underlying OXlXmlDoc object. >*/
		};
		//
		//#include "XLDrawing.hpp"
		// 
		// <v:fill o:detectmouseclick="t" type="solid" color2="#00003f"/>
		// <v:shadow on="t" obscured="t" color="black"/>
		// <v:stroke color="#3465a4" startarrow="block" startarrowwidth="medium" startarrowlength="medium" joinstyle="round" endcap="flat"/>
		// <v:path o:connecttype="none"/>
		// <v:textbox style="mso-direction-alt:auto;mso-fit-shape-to-text:t;">
		// 	<div style="text-align:left;"/>
		// </v:textbox>

		extern const std::string ShapeNodeName;         // = "v:shape"
		extern const std::string ShapeTypeNodeName;     // = "v:shapetype"

		// NOTE: numerical values of XLShapeTextVAlign and XLShapeTextHAlign are shared with the same alignments from XLAlignmentStyle (XLStyles.hpp)
		enum class XLShapeTextVAlign : uint8_t {
			Center           =   3, // value="center",           both
			Top              =   4, // value="top",              vertical only
			Bottom           =   5, // value="bottom",           vertical only
			Invalid          = 255  // all other values
		};

		constexpr const XLShapeTextVAlign XLDefaultShapeTextVAlign = XLShapeTextVAlign::Top;

		enum class XLShapeTextHAlign : uint8_t {
			Left             =   1, // value="left",             horizontal only
			Right            =   2, // value="right",            horizontal only
			Center           =   3, // value="center",           both
			Invalid          = 255  // all other values
		};

		constexpr const XLShapeTextHAlign XLDefaultShapeTextHAlign = XLShapeTextHAlign::Left;

		/**
		 * @brief An encapsulation of a shape client data element x:ClientData
		 */
		class XLShapeClientData {
		public:
			XLShapeClientData();
			/**
			 * @brief Constructor. New items should only be created through an XLShape object.
			 * @param node An XMLNode object with the x:ClientData XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLShapeClientData(const XMLNode& node);
			XLShapeClientData(const XLShapeClientData& other) = default;
			XLShapeClientData(XLShapeClientData&& other) noexcept = default;
			~XLShapeClientData() = default;
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLShapeClientData& operator=(const XLShapeClientData& other) = default;
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLShapeClientData& operator=(XLShapeClientData&& other) noexcept = default;
			/**
			 * @brief Getter functions
			 */
			std::string objectType() const; // attribute ObjectType, value "Note"
			bool moveWithCells() const;     // element x:MoveWithCells - true = present or lowercase node_pcdata "true", false = not present or lowercase node_pcdata "false"
			bool sizeWithCells() const;     // element x:SizeWithCells - logic as in MoveWithCells
			std::string anchor() const;     // element x:Anchor - Example node_pcdata: "3, 23, 0, 0, 4, 25, 3, 5" - no idea what any number means - TBD
			bool autoFill() const;          // element x:AutoFill - logic as in MoveWithCells
			XLShapeTextVAlign textVAlign() const;   // element x:TextVAlign - Top, ???
			XLShapeTextHAlign textHAlign() const;   // element x:TextHAlign - Left, ???
			uint32 row() const;           // element x:Row, 0-indexed row of cell to which this shape is linked
			uint16 column() const;        // element x:Column, 0-indexed column of cell to which this shape is linked
			/**
			 * @brief Setter functions
			 */
			bool setObjectType(std::string newObjectType);
			bool setMoveWithCells(bool set = true);
			bool setSizeWithCells(bool set = true);
			bool setAnchor(std::string newAnchor);
			bool setAutoFill(bool set = true);
			bool setTextVAlign(XLShapeTextVAlign newTextVAlign);
			bool setTextHAlign(XLShapeTextHAlign newTextHAlign);
			bool setRow(uint32 newRow);
			bool setColumn(uint16 newColumn);
			// /**
			//  * @brief Return a string summary of the x:ClientData properties
			//  * @return string with info about the x:ClientData object
			//  */
			// std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_clientDataNode;   /**< An XMLNode object with the x:ClientData item */
			inline static const std::vector< std::string_view > m_nodeOrder = {
				"x:MoveWithCells",
				"x:SizeWithCells",
				"x:Anchor",
				"x:AutoFill",
				"x:TextVAlign",
				"x:TextHAlign",
				"x:Row",
				"x:Column"
			};
		};

		struct XLShapeStyleAttribute {
			std::string name;
			std::string value;
		};

		class XLShapeStyle {
		public:
			XLShapeStyle();
			/**
			 * @brief Constructor. Init XLShapeStyle properties from styleAttribute
			 * @param styleAttribute a string with the value of the style attribute of a v:shape element
			 */
			explicit XLShapeStyle(const std::string& styleAttribute);
			/**
			 * @brief Constructor. Init XLShapeStyle properties from styleAttribute and link to the attribute so that setter functions directly modify it
			 * @param styleAttribute an XMLAttribute constructed with the style attribute of a v:shape element
			 */
			explicit XLShapeStyle(const XMLAttribute& styleAttribute);
		private:
			/**
			 * @brief get index of an attribute name within m_nodeOrder
			 * @return index of attribute in m_nodeOrder
			 * @return -1 if not found
			 */
			int16 attributeOrderIndex(std::string const& attributeName) const;
			/**
			 * @brief XLShapeStyle internal generic getter & setter functions
			 */
			XLShapeStyleAttribute getAttribute(std::string const& attributeName, std::string const& valIfNotFound = "") const;
			bool setAttribute(std::string const& attributeName, std::string const& attributeValue);
			// bool setAttribute(XLShapeStyleAttribute const& attribute);
		public:
			/**
			 * @brief XLShapeStyle getter functions
			 */
			std::string position() const;
			uint16 marginLeft() const;
			uint16 marginTop() const;
			uint16 width() const;
			uint16 height() const;
			std::string msoWrapStyle() const;
			std::string vTextAnchor() const;
			bool hidden() const;
			bool visible() const;
			std::string raw() const { return m_style; }
			/**
			 * @brief XLShapeStyle setter functions
			 */
			bool setPosition(std::string newPosition);
			bool setMarginLeft(uint16 newMarginLeft);
			bool setMarginTop(uint16 newMarginTop);
			bool setWidth(uint16 newWidth);
			bool setHeight(uint16 newHeight);
			bool setMsoWrapStyle(std::string newMsoWrapStyle);
			bool setVTextAnchor(std::string newVTextAnchor);
			bool hide(); // set visibility:hidden
			bool show(); // set visibility:visible
			bool setRaw(std::string newStyle) { m_style = newStyle; return true; }
		private:
			mutable std::string m_style; // mutable so getter functions can update it from m_styleAttribute if the latter is not empty
			std::unique_ptr<XMLAttribute> m_styleAttribute;
			inline static const std::vector< std::string_view > m_nodeOrder = {
				"position",
				"margin-left",
				"margin-top",
				"width",
				"height",
				"mso-wrap-style",
				"v-text-anchor",
				"visibility"
			};
		};

		class XLShape {
			friend class XLVmlDrawing;    // for access to m_shapeNode in XLVmlDrawing::addShape
		public:
			XLShape();
			/**
			 * @brief Constructor. New items should only be created through an XLStyles object.
			 * @param node An XMLNode object with the styles item. If no input is provided, a null node is used.
			 */
			explicit XLShape(const XMLNode& node);
			XLShape(const XLShape& other) = default;
			XLShape(XLShape&& other) noexcept = default;
			~XLShape() = default;
			XLShape& operator=(const XLShape& other);
			XLShape& operator=(XLShape&& other) noexcept = default;
			std::string shapeId() const;   // v:shape attribute id - shape_# - can't be set by the user
			std::string fillColor() const; // v:shape attribute fillcolor, #<3 byte hex code>, e.g. #ffffc0
			bool stroked() const;          // v:shape attribute stroked "t" ("f"?)
			std::string type() const;      // v:shape attribute type, link to v:shapetype attribute id
			bool allowInCell() const;      // v:shape attribute o:allowincell "f"
			XLShapeStyle style();          // v:shape attribute style, but constructed from the XMLAttribute

			// XLShapeShadow& shadow();          // v:shape subnode v:shadow
			// XLShapeFill& fill();              // v:shape subnode v:fill
			// XLShapeStroke& stroke();          // v:shape subnode v:stroke
			// XLShapePath& path();              // v:shape subnode v:path
			// XLShapeTextbox& textbox();        // v:shape subnode v:textbox
			XLShapeClientData clientData();  // v:shape subnode x:ClientData
			/**
			 * @brief Setter functions
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			// NOTE: setShapeId is not available because shape id is managed by the parent class in createShape
			bool setFillColor(std::string const& newFillColor);
			bool setStroked(bool set);
			bool setType(std::string const& newType);
			bool setAllowInCell(bool set);
			bool setStyle(std::string const& newStyle);
			bool setStyle(XLShapeStyle const& newStyle);
		private:
			std::unique_ptr<XMLNode> m_shapeNode;        /**< An XMLNode object with the v:shape item */
			inline static const std::vector< std::string_view > m_nodeOrder = {
				"v:shadow",
				"v:fill",
				"v:stroke",
				"v:path",
				"v:textbox",
				"x:ClientData"
			};
		};
		/**
		 * @brief The XLVmlDrawing class is the base class for worksheet comments
		 */
		class XLVmlDrawing : public XLXmlFile {
			friend class XLWorksheet;   // for access to XLXmlFile::getXmlPath
			friend class XLComments;    // for access to firstShapeNode
		public:
			XLVmlDrawing() : XLXmlFile(nullptr) 
			{
			}
			XLVmlDrawing(XLXmlData* xmlData);
			XLVmlDrawing(const XLVmlDrawing& other) = default;
			XLVmlDrawing(XLVmlDrawing&& other) noexcept = default;
			~XLVmlDrawing() = default;
			XLVmlDrawing& operator=(const XLVmlDrawing&) = default;
			XLVmlDrawing& operator=(XLVmlDrawing&& other) noexcept = default;
		private:
			XMLNode firstShapeNode() const;
			XMLNode lastShapeNode() const;
			XMLNode shapeNode(uint32 index) const;
		public:
			/**
			 * @brief Get the shape XML node that is associated with the cell indicated by cellRef
			 * @param cellRef the reference to the cell for which a shape shall be found
			 * @return the XMLNode that contains the desired shape, or an empty XMLNode if not found
			 */
			XMLNode shapeNode(std::string const& cellRef) const;
			uint32 shapeCount() const;
			XLShape shape(uint32 index) const;
			bool deleteShape(uint32 index);
			bool deleteShape(std::string const& cellRef);
			XLShape createShape(const XLShape& shapeTemplate = XLShape());
			/**
			 * @brief Print the XML contents of this XLVmlDrawing instance using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			uint32 m_shapeCount{0};
			uint32 m_lastAssignedShapeId{0};
			std::string m_defaultShapeTypeId{};
		};
		//
		//#include "XLComments.hpp"
		// TODO:
		//   add to [Content_Types].xml:
		//     <Override PartName="/xl/comments1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.comments+xml"/>
		/**
		 * @brief An encapsulation of a comment element
		 */
		class XLComment {
		public:
			XLComment() = delete; // do not allow default constructor (for now) - could still be constructed with an empty XMLNode
			/**
			 * @brief Constructor. New items should only be created through an XLComments object.
			 * @param node An XMLNode object with the comment XMLNode. If no input is provided, a null node is used.
			 */
			explicit XLComment(const XMLNode& node);
			XLComment(const XLComment& other) = default;
			XLComment(XLComment&& other) noexcept = default;
			~XLComment() = default;
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLComment& operator=(const XLComment& other) = default;
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLComment& operator=(XLComment&& other) noexcept = default;
			/**
			 * @brief Test if XLComment is linked to valid XML
			 * @return true if comment was constructed on a valid XML node, otherwise false
			 */
			bool valid() const;
			/**
			 * @brief Getter functions
			 */
			std::string ref() const; // the cell reference of the comment
			std::string text() const;
			uint16 authorId() const;
			/**
			 * @brief Setter functions
			 */
			bool setText(std::string newText);
			bool setAuthorId(uint16 newAuthorId);
			// /**
			//  * @brief Return a string summary of the comment properties
			//  * @return string with info about the comment object
			//  */
			// std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_commentNode;      /**< An XMLNode object with the comment item */
		};
		/**
		 * @brief The XLComments class is the base class for worksheet comments
		 */
		class XLComments : public XLXmlFile {
			friend class XLWorksheet;   // for access to XLXmlFile::getXmlPath
		public:
			XLComments();
			XLComments(XLXmlData* xmlData);
			XLComments(const XLComments& other);
			XLComments(XLComments&& other) noexcept;
			~XLComments() = default;
			XLComments& operator=(XLComments&& other) noexcept;
			XLComments& operator=(const XLComments&);
			/**
			 * @brief associate the worksheet's VML drawing object with the comments so it can be modified from here
			 * @param vmlDrawing the worksheet's previously created XLVmlDrawing object
			 * @return true upon success
			 */
			bool setVmlDrawing(XLVmlDrawing &vmlDrawing);
		private:
			XMLNode authorNode(uint16 index) const;
			XMLNode commentNode(size_t index) const;
			XMLNode commentNode(const std::string& cellRef) const;
		public:
			uint16 authorCount() const;
			std::string author(uint16 index) const;
			bool deleteAuthor(uint16 index);
			uint16 addAuthor(const std::string& authorName);
			/**
			 * @brief get the amount of comments
			 * @return the amount of comments for the worksheet
			 */
			size_t count() const;
			uint16 authorId(const std::string& cellRef) const;
			bool deleteComment(const std::string& cellRef);
			/**
			 * @brief get a comment by its index in the comment list
			 * @param index the index of the comment as per XML sequence, no guarantee about cell reference being in sequence
			 * @return the comment at index - will throw if index is out of bounds (>=count())
			 */
			XLComment get(size_t index) const;
			/**
			 * @brief get the comment (if any) for the referenced cell
			 * @param cellRef the cell address to check
			 * @return the comment for this cell - an empty string if no comment is set
			 */
			std::string get(std::string const& cellRef) const;
			/**
			 * @brief set the comment for the referenced cell
			 * @param cellRef the cell address to set
			 * @param comment set this text as comment for the cell
			 * @param authorId_ set this author (underscore to avoid conflict with function name)
			 * @return true upon success, false on failure
			 */
			bool set(std::string const& cellRef, std::string const& comment, uint16 authorId_ = 0);
			/**
			 * @brief get the XLShape object for this comment
			 */
			XLShape shape(std::string const& cellRef);
			/**
			 * @brief Print the XML contents of this XLComments instance using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			XMLNode m_authors{};
			XMLNode m_commentList{};
			std::unique_ptr<XLVmlDrawing> m_vmlDrawing;
			mutable XMLNode m_hintNode{};                 // the last comment XML Node accessed by index is stored here, if any - will be reset when comments are inserted or deleted
			mutable size_t m_hintIndex{0};                // this has the index at which m_hintNode was accessed, only valid if not m_hintNode.empty()
			inline static const std::vector< std::string_view > m_nodeOrder = {      // comments XML node required child sequence
				"authors",
				"commentList"
			};
		};
		//
		//#include "XLProperties.hpp"
		class XLProperties : public XLXmlFile {
		private:
			/**
			 * @brief constructor helper function: create core.xml content from template
			 */
			void createFromTemplate();
		public:
			XLProperties() = default;
			explicit XLProperties(XLXmlData* xmlData);
			XLProperties(const XLProperties& other) = default;
			XLProperties(XLProperties&& other) noexcept = default;
			~XLProperties();
			XLProperties& operator=(const XLProperties& other) = default;
			XLProperties& operator=(XLProperties&& other) = default;
			void setProperty(const std::string& name, const std::string& value);
			void setProperty(const std::string& name, int value);
			void setProperty(const std::string& name, double value);
			std::string property(const std::string& name) const;
			void deleteProperty(const std::string& name);
		};
		/**
		 * @brief This class is a specialization of the XLAbstractXMLFile, with the purpose of the representing the
		 * document app properties in the app.xml file (docProps folder) in the .xlsx package.
		 */
		class XLAppProperties : public XLXmlFile {
		private:
			/**
			 * @brief constructor helper function: create app.xml content from template
			 */
			void createFromTemplate(OXlXmlDoc const & workbookXml);
		public:
			XLAppProperties() = default;
			/**
			 * @brief enable XLAppProperties to re-create a worksheet list in docProps/app.xml <TitlesOfParts> element from workbookXml
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
			void incrementSheetCount(int16 increment);
			/**
			 * @brief initialize <TitlesOfParts> to contain all and only entries from workbookSheetNames & ensure HeadingPairs entry for Worksheets has the correct count
			 * @param workbookSheetNames the vector of sheet names as returned by XLWorkbook::sheetNames()
			 * @throws XLInternalError thrown by the underlying sheetNames call upon failure
			 */
			void alignWorksheets(std::vector<std::string> const & workbookSheetNames);
			void addSheetName(const std::string& title);
			void deleteSheetName(const std::string& title);
			void setSheetName(const std::string& oldTitle, const std::string& newTitle);
			void addHeadingPair(const std::string& name, int value);
			void deleteHeadingPair(const std::string& name);
			void setHeadingPair(const std::string& name, int newValue);
			void setProperty(const std::string& name, const std::string& value);
			std::string property(const std::string& name) const;
			void deleteProperty(const std::string& name);
			void appendSheetName(const std::string& sheetName);
			void prependSheetName(const std::string& sheetName);
			void insertSheetName(const std::string& sheetName, unsigned int index);
		};
		//
		//#include "XLRelationships.hpp"
		/**
		 * @brief Enable use of random (relationship) IDs
		 */
		void UseRandomIDs();
		/**
		 * @brief Disable use of random (relationship) IDs (default behavior)
		 */
		void UseSequentialIDs();
		/**
		 * @brief Return a 32 bit random value
		 * @return A 32 bit random value
		 */
		extern std::mt19937 Rand32;
		/**
		 * @brief Return a 64 bit random value (by invoking Rand32 twice)
		 * @return A 64 bit random value
		 */
		uint64_t Rand64();
		/**
		 * @brief Initialize XLRand32 data source
		 * @param pseudoRandom If true, sequence will be reproducible with a constant seed
		 */
		void InitRandom(bool pseudoRandom = false);

		class XLRelationships;
		class XLRelationshipItem;
		/**
		 * @brief An enum of the possible relationship (or XML document) types used in relationship (.rels) XML files.
		 */
		enum class XLRelationshipType {
			CoreProperties,
			ExtendedProperties,
			CustomProperties,
			Workbook,
			Worksheet,
			Chartsheet,
			Dialogsheet,
			Macrosheet,
			CalculationChain,
			ExternalLink,
			ExternalLinkPath,
			Theme,
			Styles,
			Chart,
			ChartStyle,
			ChartColorStyle,
			Image,
			Drawing,
			VMLDrawing,
			SharedStrings,
			PrinterSettings,
			VBAProject,
			ControlProperties,
			Comments,
			Table,
			Unknown
		};
	}

	namespace OpenXLSX_XLRelationships { // special namespace to avoid naming conflict with another GetStringFromType function
		using namespace OpenXLSX;
		/**
		 * @brief helper function, used only within module and from XLProperties.cpp / XLAppProperties::createFromTemplate
		 * @param type the XLRelationshipType for which to return the correct XML string
		 */
		std::string GetStringFromType(XLRelationshipType type);
	}

	namespace OpenXLSX {
		/**
		 * @brief An encapsulation of a relationship item, i.e. an XML file in the document, its type and an ID number.
		 */
		class XLRelationshipItem {
		public:
			XLRelationshipItem();
			explicit XLRelationshipItem(const XMLNode& node);
			XLRelationshipItem(const XLRelationshipItem& other);
			XLRelationshipItem(XLRelationshipItem&& other) noexcept = default;
			~XLRelationshipItem();
			XLRelationshipItem& operator=(const XLRelationshipItem& other);
			XLRelationshipItem& operator=(XLRelationshipItem&& other) noexcept = default;
			/**
			 * @brief Get the type of the current relationship item.
			 * @return An XLRelationshipType enum object, corresponding to the type.
			 */
			XLRelationshipType type() const;
			/**
			 * @brief Get the target, i.e. the path to the XML file the relationship item refers to.
			 * @return An XMLAttribute object containing the Target getValue.
			 */
			std::string target() const;
			/**
			 * @brief Get the id of the relationship item.
			 * @return An XMLAttribute object containing the Id getValue.
			 */
			std::string id() const;
			/**
			 * @brief Test if relationship item is empty (== m_relationshipNode->empty())
			 * @return true if this is an empty relationship item
			 */
			bool empty() const;
		private:
			std::unique_ptr<XMLNode> m_relationshipNode; /**< An XMLNode object with the relationship item */
		};
		/**
		 * @brief An encapsulation of relationship files (.rels files) in an Excel document package.
		 */
		class XLRelationships : public XLXmlFile {
		public:
			XLRelationships() = default;
			/**
			 * @param pathTo Initialize m_path from this: the path to the relationships file origin of xmlData
			 * @note m_path is used to resolve relative relationship target paths to an absolute
			 */
			explicit XLRelationships(XLXmlData* xmlData, std::string pathTo);
			~XLRelationships();
			XLRelationships(const XLRelationships& other) = default;
			XLRelationships(XLRelationships&& other) noexcept = default;
			XLRelationships& operator=(const XLRelationships& other) = default;
			XLRelationships& operator=(XLRelationships&& other) noexcept = default;
			/**
			 * @brief Look up a relationship item by ID.
			 * @param id The ID string of the relationship item to retrieve.
			 * @return An XLRelationshipItem object.
			 */
			XLRelationshipItem relationshipById(const std::string& id) const;
			/**
			 * @brief Look up a relationship item by Target.
			 * @param target The Target string of the relationship item to retrieve.
			 * @param throwIfNotFound Throw an XLException when target is not found, default: true
			 *                        when false, XLRelationshipItem::empty() can be tested on the return value
			 * @return An XLRelationshipItem object.
			 */
			XLRelationshipItem relationshipByTarget(const std::string& target, bool throwIfNotFound = true) const;
			/**
			 * @brief Get the std::map with the relationship items, ordered by ID.
			 * @return A const reference to the std::map with relationship items.
			 */
			std::vector<XLRelationshipItem> relationships() const;
			void deleteRelationship(const std::string& relID);
			/**
			 * @brief Delete an item from the Relationships register
			 * @param item The XLRelationshipItem object to delete.
			 */
			void deleteRelationship(const XLRelationshipItem& item);
			/**
			 * @brief Add a new relationship item to the XLRelationships object.
			 * @param type The type of the new relationship item.
			 * @param target The target (or path) of the XML file for the relationship item.
			 */
			XLRelationshipItem addRelationship(XLRelationshipType type, const std::string& target);
			/**
			 * @brief Check if a XLRelationshipItem with the given Target string exists.
			 * @param target The Target string to look up.
			 * @return true if the XLRelationshipItem exists; otherwise false.
			 */
			bool targetExists(const std::string& target) const;
			/**
			 * @brief Check if a XLRelationshipItem with the given Id string exists.
			 * @param id The Id string to look up.
			 * @return true if the XLRelationshipItem exists; otherwise false.
			 */
			bool idExists(const std::string& id) const;
			/**
			 * @brief print the XML contents of the relationships document using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			std::string m_path; // the path - within the XLSX file - to the relationships file on which this object is instantiated
		};
		//
		//#include "XLTables.hpp"
		/**
		 * @brief The XLTables class is the base class for worksheet tables
		 */
		class XLTables : public XLXmlFile {
			friend class XLWorksheet;   // for access to XLXmlFile::getXmlPath
		public:
			XLTables() : XLXmlFile(nullptr) 
			{
			}
			XLTables(XLXmlData * xmlData);
			XLTables(const XLTables& other) = default;
			XLTables(XLTables&& other) noexcept = default;
			~XLTables() = default;
			XLTables& operator=(const XLTables&) = default;
			XLTables& operator=(XLTables&& other) noexcept = default;
			/**
			 * @brief Print the XML contents of this XLTables instance using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		};
		//
		//#include "XLWorkbook.hpp"
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
		public:
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
			XLSheet sheet(uint16 index);
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
			XLWorksheet worksheet(uint16 index);
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
			XLChartsheet chartsheet(uint16 index);
			/**
			 * @brief Delete sheet (worksheet or chartsheet) from the workbook.
			 * @param sheetName Name of the sheet to delete.
			 * @throws XLException An exception will be thrown if trying to delete the last worksheet in the workbook
			 * @warning A workbook must contain at least one worksheet. Trying to delete the last worksheet from the
			 * workbook will trow an exception.
			 */
			void deleteSheet(const std::string& sheetName);
			void addWorksheet(const std::string& sheetName);
			void cloneSheet(const std::string& existingName, const std::string& newName);
			/**
			 * @param index The index (1-based) where the sheet shall be moved to
			 */
			void setSheetIndex(const std::string& sheetName, unsigned int index);
			/**
			 * @return The index (1-based) of the sheet with sheetName
			 */
			unsigned int indexOfSheet(const std::string& sheetName) const;
			XLSheetType typeOfSheet(const std::string& sheetName) const;
			/**
			 * @param index The index (1-based) at which the desired sheet is located.
			 */
			XLSheetType typeOfSheet(unsigned int index) const;
			unsigned int sheetCount() const;
			unsigned int worksheetCount() const;
			unsigned int chartsheetCount() const;
			std::vector<std::string> sheetNames() const;
			std::vector<std::string> worksheetNames() const;
			std::vector<std::string> chartsheetNames() const;
			bool sheetExists(const std::string& sheetName) const;
			bool worksheetExists(const std::string& sheetName) const;
			bool chartsheetExists(const std::string& sheetName) const;
			void updateSheetReferences(const std::string& oldName, const std::string& newName);
			// XLSharedStrings sharedStrings();
			// bool hasSharedStrings() const;
			void deleteNamedRanges();
			/**
			 * @brief set a flag to force full calculation upon loading the file in Excel
			 */
			void setFullCalculationOnLoad();
			/**
			 * @brief print the XML contents of the workbook.xml using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			uint16 createInternalSheetID();
			std::string sheetID(const std::string& sheetName);
			std::string sheetName(const std::string& sheetID) const;
			std::string sheetVisibility(const std::string& sheetID) const;
			void prepareSheetMetadata(const std::string& sheetName, uint16 internalID);
			void setSheetName(const std::string& sheetRID, const std::string& newName);
			void setSheetVisibility(const std::string& sheetRID, const std::string& state);
			bool sheetIsActive(const std::string& sheetRID) const;
			/**
			 * @return true if sheed with sheedRID could be set to active (or was already active), otherwise false
			 */
			bool setSheetActive(const std::string& sheetRID);
			/**
			 * @brief Check whether attribute string state matches a value that is considered not visible
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
	}
	//
	//#include "XLZipArchive.hpp"
	namespace Zippy {
		class ZipArchive;
	}

	namespace OpenXLSX {
		class XLZipArchive {
		public:
			XLZipArchive();
			XLZipArchive(const XLZipArchive& other) = default;
			XLZipArchive(XLZipArchive&& other) = default;
			~XLZipArchive();
			XLZipArchive& operator=(const XLZipArchive& other) = default;
			XLZipArchive& operator=(XLZipArchive&& other) = default;
			explicit operator bool() const;
			bool isValid() const;
			bool isOpen() const;
			void open(const std::string& fileName);
			void close();
			void save(const std::string& path = "");
			void addEntry(const std::string& name, const std::string& data);
			void deleteEntry(const std::string& entryName);
			std::string getEntry(const std::string& name) const;
			bool hasEntry(const std::string& entryName) const;
		private:
			std::shared_ptr<Zippy::ZipArchive> m_archive;
		};
		//
		//#include "XLDocument.hpp"
		constexpr const unsigned int pugi_parse_settings = pugi::parse_default | pugi::parse_ws_pcdata;     // TBD: | pugi::parse_comments
		constexpr const bool XLForceOverwrite = true;        // readability constant for 2nd parameter of XLDocument::saveAs
		constexpr const bool XLDoNotOverwrite = false;       //  "
		/**
		 * @brief The XLDocumentProperties class is an enumeration of the possible properties (metadata) that can be set
		 * for a XLDocument object (and .xlsx file)
		 */
		enum class XLProperty {
			Title,
			Subject,
			Creator,
			Keywords,
			Description,
			LastModifiedBy,
			LastPrinted,
			CreationDate,
			ModificationDate,
			Category,
			Application,
			DocSecurity,
			ScaleCrop,
			Manager,
			Company,
			LinksUpToDate,
			SharedDoc,
			HyperlinkBase,
			HyperlinksChanged,
			AppVersion
		};
		/**
		 * @brief This class encapsulates the concept of an excel file. It is different from the XLWorkbook, in that an
		 * XLDocument holds an XLWorkbook together with its metadata, as well as methods for opening,
		 * closing and saving the document.\n<b><em>The XLDocument is the entrypoint for clients
		 * using the RapidXLSX library.</em></b>
		 */
		class XLDocument final {
			friend class XLXmlFile;
			friend class XLWorkbook;
			friend class XLSheet;
			friend class XLXmlData;
		public:
			explicit XLDocument(const IZipArchive& zipArchive = XLZipArchive());
			/**
			 * @brief Constructor. An alternative constructor, taking the path to the .xlsx file as an argument.
			 * @param docPath A std::string with the path to the .xlsx file.
			 * @param zipArchive
			 */
			explicit XLDocument(const std::string& docPath, const IZipArchive& zipArchive = XLZipArchive());
			/**
			 * @brief Copy constructor
			 * @param other The object to copy
			 * @note Copy constructor explicitly deleted.
			 */
			XLDocument(const XLDocument& other) = delete;
			XLDocument(XLDocument&& other) noexcept = default;
			~XLDocument();
			XLDocument& operator=(const XLDocument& other) = delete;
			XLDocument& operator=(XLDocument&& other) noexcept = default;
			/**
			 * @brief ensure that warnings are shown (default setting)
			 */
			void showWarnings();
			/**
			 * @brief ensure that warnings are suppressed where this parameter is supported (currently only XLStyles)
			 */
			void suppressWarnings();
			/**
			 * @brief Open the .xlsx file with the given path
			 * @param fileName The path of the .xlsx file to open
			 */
			void open(const std::string& fileName);
			/**
			 * @brief Create a new .xlsx file with the given name.
			 * @param fileName The path of the new .xlsx file.
			 * @param forceOverwrite If not true (XLForceOverwrite) and fileName exists, create will throw an exception
			 * @throw XLException (OpenXLSX failed checks)
			 * @throw ZipRuntimeError (zippy failed archive / file access)
			 */
			void create(const std::string& fileName, bool forceOverwrite);
			/**
			 * @brief Create a new .xlsx file with the given name. Legacy interface, invokes create( fileName, XLForceOverwrite )
			 * @param fileName The path of the new .xlsx file.
			 * @deprecated use instead void create(const std::string& fileName, bool forceOverwrite)
			 * @warning Overwriting an existing file is retained as legacy behavior, but can lead to data loss!
			 */
			[[deprecated]] void create(const std::string& fileName);
			/**
			 * @brief Close the current document
			 */
			void close();
			/**
			 * @brief Save the current document using the current filename, overwriting the existing file.
			 * @throw XLException (OpenXLSX failed checks)
			 * @throw ZipRuntimeError (zippy failed archive / file access)
			 */
			void save();
			/**
			 * @brief Save the document with a new name. If a file exists with that name, it will be overwritten.
			 * @param fileName The path of the file
			 * @param forceOverwrite If not true (XLForceOverwrite) and fileName exists, saveAs will throw an exception
			 * @throw XLException (OpenXLSX failed checks)
			 * @throw ZipRuntimeError (zippy failed archive / file access)
			 */
			void saveAs(const std::string& fileName, bool forceOverwrite);
			/**
			 * @brief Save the document with a new name. Legacy interface, invokes saveAs( fileName, XLForceOverwrite )
			 * @param fileName The path of the file
			 * @deprecated use instead void saveAs(const std::string& fileName, bool forceOverwrite)
			 * @warning Overwriting an existing file is retained as legacy behavior, but can lead to data loss!
			 */
			[[deprecated]] void saveAs(const std::string& fileName);
			/**
			 * @brief Get the filename of the current document, e.g. "spreadsheet.xlsx".
			 * @return A std::string with the filename.
			 * @note 2024-06-03: function can't return as reference to const because filename as a substr of m_filePath can be a temporary
			 * @note 2024-07-28: Removed const from return type
			 */
			std::string name() const;
			/**
			 * @brief Get the full path of the current document, e.g. "drive/blah/spreadsheet.xlsx"
			 * @return A std::string with the path.
			 */
			const std::string& path() const;
			/**
			 * @brief Get the underlying workbook object, as a const object.
			 * @return A const pointer to the XLWorkbook object.
			 */
			XLWorkbook workbook() const;
			/**
			 * @brief Get the requested document property.
			 * @param prop The name of the property to get.
			 * @return The property as a string
			 */
			std::string property(XLProperty prop) const;
			/**
			 * @brief Set a property
			 * @param prop The property to set.
			 * @param value The getValue of the property, as a string
			 */
			void setProperty(XLProperty prop, const std::string& value);
			/**
			 * @brief Delete the property from the document
			 * @param theProperty The property to delete from the document
			 */
			void deleteProperty(XLProperty theProperty);
			explicit operator bool() const;
			bool isOpen() const;
			/**
			 * @brief return a handle on the workbook's styles
			 * @return a reference to m_styles
			 */
			XLStyles& styles();
			/**
			 * @brief determine whether a worksheet relationships file exists for sheetXmlNo
			 * @param sheetXmlNo check for this sheet number # (xl/worksheets/_reals/sheet#.xml.rels)
			 * @return true if relationships file exists
			 */
			bool hasSheetRelationships(uint16 sheetXmlNo) const;
			/**
			 * @brief determine whether a worksheet vml drawing file exists for sheetXmlNo
			 * @param sheetXmlNo check for this sheet number # (xl/drawings/vmlDrawing#.xml)
			 * @return true if vml drawing file exists
			 */
			bool hasSheetVmlDrawing(uint16 sheetXmlNo) const;
			/**
			 * @brief determine whether a worksheet comments file exists for sheetXmlNo
			 * @param sheetXmlNo check for this sheet number # (xl/comments#.xml)
			 * @return true if comments file exists
			 */
			bool hasSheetComments(uint16 sheetXmlNo) const;
			/**
			 * @brief determine whether a worksheet table(s) file exists for sheetXmlNo
			 * @param sheetXmlNo check for this sheet number # (xl/tables/table#.xml)
			 * @return true if table(s) file exists
			 */
			bool hasSheetTables(uint16 sheetXmlNo) const;
			/**
			 * @brief fetch the worksheet relationships for sheetXmlNo, create the file if it does not exist
			 * @param sheetXmlNo fetch for this sheet #
			 * @return an XLRelationships object initialized with the sheet relationships
			 */
			XLRelationships sheetRelationships(uint16 sheetXmlNo);
			/**
			 * @brief fetch the worksheet VML drawing for sheetXmlNo, create the file if it does not exist
			 * @param sheetXmlNo fetch for this sheet #
			 * @return an XLVmlDrawing object initialized with the sheet drawing
			 */
			XLVmlDrawing sheetVmlDrawing(uint16 sheetXmlNo);
			/**
			 * @brief fetch the worksheet comments for sheetXmlNo, create the file if it does not exist
			 * @param sheetXmlNo fetch for this sheet #
			 * @return an XLComments object initialized with the sheet comments
			 */
			XLComments sheetComments(uint16 sheetXmlNo);
			/**
			 * @brief fetch the worksheet tables for sheetXmlNo, create the file if it does not exist
			 * @param sheetXmlNo fetch for this sheet #
			 * @return an XLTables object initialized with the sheet tables
			 */
			XLTables sheetTables(uint16 sheetXmlNo);
		public:
			/**
			 * @brief validate whether sheetName is a valid Excel worksheet name
			 * @param sheetName the desired name
			 * @param throwOnInvalid (default: false) if true, invalid sheetName will throw exception
			 * @return true if sheetName can be used, otherwise false
			 */
			bool validateSheetName(std::string sheetName, bool throwOnInvalid = false);
			/**
			 * @return for XLCommandType::SetSheetActive: execution success, otherwise always true
			 */
			bool execCommand(const XLCommand& command);
			XLQuery execQuery(const XLQuery& query) const;
			XLQuery execQuery(const XLQuery& query);
			/**
			 * @brief configure an alternative XML saving declaration to be used with pugixml
			 * @param savingDeclaration An XLXmlSavingDeclaration object with the configuration to use
			 */
			void setSavingDeclaration(XLXmlSavingDeclaration const& savingDeclaration);
			const XLSharedStrings& sharedStrings() const { return m_sharedStrings; }
			/**
			 * @brief rewrite the shared strings cache (and update all cells referencing an index from the shared strings), dropping unused strings
			 * @note potentially time-intensive (on documents with many strings or many cells referring shared strings)
			 */
			void cleanupSharedStrings();
		protected:
			/**
			 * @brief Get an XML file from the .xlsx archive.
			 * @param path The relative path of the file.
			 * @return A std::string with the content of the file
			 */
			std::string extractXmlFromArchive(const std::string& path);
			/**
			 * @brief fetch the XLXmlData object as stored in m_data, throw XLInternalError if path is not found
			 * @param path The relative path of the file.
			 * @param doNotThrow if true, will return a nullptr if path is not found
			 * @return a pointer to the XLXmlData object stored in m_data (or nullptr, see doNotThrow)
			 */
			XLXmlData* getXmlData(const std::string& path, bool doNotThrow = false);
			/**
			 * @brief const overload of getXmlData
			 */
			const XLXmlData* getXmlData(const std::string& path, bool doNotThrow = false) const;
			bool hasXmlData(const std::string& path) const;
		private:
			bool m_suppressWarnings {true}; /**< If true, will suppress output of warnings where supported */
			std::string m_filePath {};      /**< The path to the original file*/
			XLXmlSavingDeclaration m_xmlSavingDeclaration;  /**< The xml saving declaration that will be passed to pugixml before generating the XML output data*/
			mutable std::list<XLXmlData>    m_data {};
			mutable std::deque<std::string> m_sharedStringCache {};
			mutable XLSharedStrings m_sharedStrings {};

			XLRelationships m_docRelationships {}; /**< A pointer to the document relationships object*/
			XLRelationships m_wbkRelationships {}; /**< A pointer to the document relationships object*/
			XLContentTypes m_contentTypes {};      /**< A pointer to the content types object*/
			XLAppProperties m_appProperties {};    /**< A pointer to the App properties object */
			XLProperties m_coreProperties {};      /**< A pointer to the Core properties object*/
			XLStyles m_styles {};                  /**< A pointer to the document styles object*/
			XLWorkbook m_workbook {};              /**< A pointer to the workbook object */
			IZipArchive m_archive {};
		};
		/**
		 * @brief Get a hexadecimal representation of size bytes, starting at data
		 * @param data A pointer to the data bytes to format
		 * @param size The amount of data bytes to format
		 * @return A string with the base-16 representation of the data bytes
		 * @note 2024-08-18 BUGFIX: replaced char array with std::string, as ISO C++ standard does not permit variable size arrays
		 */
		std::string BinaryAsHexString(const void * data, const size_t size);
		/**
		 * @brief Calculate the two-byte XLSX password hash for password
		 * @param password the string to hash
		 * @return the two byte value calculated according to the XLSX password hashing algorithm
		 */
		uint16 ExcelPasswordHash(std::string password);
		/**
		 * @brief Same as ExcelPasswordHash but format the output as a 4-digit hexadecimal string
		 * @param password the string to hash
		 * @return a string that can be stored in OOXML as a password hash
		 */
		std::string ExcelPasswordHashAsString(std::string password);
		/**
		 * @brief eliminate from pathA leading subdirectories shared with pathB and find a path from pathB to pathA destination
		 * @param pathA return a relative path to here
		 * @param pathB escape this path via "../" until the common branch with pathA is reached
		 * @return a string that leads via a relative path from pathA to pathB
		 * @throw XLInternalError if pathA and pathB have no common leading (sub)directory
		 */
		std::string getPathARelativeToPathB(std::string const& pathA, std::string const& pathB);
		/**
		 * @brief eliminate from path any . and .. subdirectories by ignoring them (.) or escaping to the parent directory (..)
		 * @param path the path to normalize in this way
		 * @return a normalized path (no longer contains . or .. entries)
		 * @throw XLInternalError upon invalid path - e.g. containing "//" or trying to escape via ".." beyond the context of path
		 */
		std::string eliminateDotAndDotDotFromPath(const std::string& path);
		//
		//#include "XLRowData.hpp"
		class XLRow;
		class XLRowDataRange;
		/**
		 * @brief This class encapsulates a (non-const) iterator, for iterating over the cells in a row.
		 * @todo Consider implementing a const iterator also
		 */
		class XLRowDataIterator {
			friend class XLRowDataRange;
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type        = XLCell;
			using difference_type   = int64_t;
			using pointer           = XLCell*;
			using reference         = XLCell&;

			~XLRowDataIterator();
			XLRowDataIterator(const XLRowDataIterator& other);
			XLRowDataIterator(XLRowDataIterator&& other) noexcept;
			XLRowDataIterator& operator=(const XLRowDataIterator& other);
			XLRowDataIterator& operator=(XLRowDataIterator&& other) noexcept;
			/**
			 * @brief Pre-increment of the iterator.
			 * @return Reference to the iterator object.
			 */
			XLRowDataIterator& operator++();
			/**
			 * @brief Post-increment of the iterator.
			 * @return Reference to the iterator object.
			 */
			XLRowDataIterator operator++(int);
			/**
			 * @brief Dereferencing operator.
			 * @return Reference to the object pointed to by the iterator.
			 */
			reference operator*();
			/**
			 * @brief Arrow operator.
			 * @return Pointer to the object pointed to by the iterator.
			 */
			pointer operator->();
			/**
			 * @brief Equality operator.
			 * @param rhs XLRowDataIterator to compare to.
			 * @return true if equal, otherwise false.
			 */
			bool operator==(const XLRowDataIterator& rhs) const;
			/**
			 * @brief Non-equality operator.
			 * @param rhs XLRowDataIterator to compare to.
			 * @return false if equal, otherwise true.
			 */
			bool operator!=(const XLRowDataIterator& rhs) const;
		private:
			/**
			 * @param rowDataRange The range to iterate over.
			 * @param loc The location of the iterator (begin or end).
			 */
			XLRowDataIterator(const XLRowDataRange& rowDataRange, XLIteratorLocation loc);

			std::unique_ptr<XLRowDataRange> m_dataRange;   /**< A pointer to the range to iterate over. */
			std::unique_ptr<XMLNode>        m_cellNode;    /**< The XML node representing the cell currently pointed at. */
			XLCell m_currentCell;                          /**< The XLCell currently pointed at. */
		};
		/**
		 * @brief This class encapsulates the concept of a contiguous range of cells in a row.
		 */
		class XLRowDataRange {
			friend class XLRowDataIterator;
			friend class XLRowDataProxy;
			friend class XLRow;
		public:
			XLRowDataRange(const XLRowDataRange& other);
			XLRowDataRange(XLRowDataRange&& other) noexcept;
			~XLRowDataRange();
			/**
			 * @brief Copy assignment operator.
			 * @param other Object to be copied.
			 * @return A reference to the copied-to object.
			 */
			XLRowDataRange& operator=(const XLRowDataRange& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Object to be moved.
			 * @return A reference to the moved-to object.
			 */
			XLRowDataRange& operator=(XLRowDataRange&& other) noexcept;
			/**
			 * @brief Get the size (length) of the range.
			 * @return The size of the range.
			 */
			uint16 size() const;
			/**
			 * @brief Get an iterator to the first element.
			 * @return An XLRowDataIterator pointing to the first element.
			 */
			XLRowDataIterator begin();
			/**
			 * @brief Get an iterator to (one-past) the last element.
			 * @return An XLRowDataIterator pointing to (one past) the last element.
			 */
			XLRowDataIterator end();
			/**
			 * @brief Templated assignment operator - assign value to all existing cells in the row
			 * @note CAUTION: non-existing cells will not be assigned
			 * @tparam T The type of the value argument.
			 * @param value The value.
			 * @return A reference to the assigned-to object.
			 */
			template <typename T, typename = std::enable_if_t<
					std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<std::decay_t<T>, std::string> ||
					std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::decay_t<T>, const char*> ||
					std::is_same_v<std::decay_t<T>, char*> || std::is_same_v<T, XLDateTime> > >
			XLRowDataRange& operator=(T value)
			{
				// forward implementation to templated XLCellValue& XLCellValue::operator=(T value)
				for(auto it = begin(); it != end(); ++it)  it->value() = value;
				return *this;
			}
		private:
			/**
			 * @param rowNode XMLNode representing the row of the range.
			 * @param firstColumn The index of the first column.
			 * @param lastColumn The index of the last column.
			 * @param sharedStrings A pointer to the shared strings repository.
			 */
			explicit XLRowDataRange(const XMLNode& rowNode, uint16 firstColumn, uint16 lastColumn, const XLSharedStrings& sharedStrings);
			explicit XLRowDataRange();

			std::unique_ptr<XMLNode> m_rowNode;
			uint16 m_firstCol { 1 };  /**< The cell reference of the first cell in the range */
			uint16 m_lastCol { 1 };   /**< The cell reference of the last cell in the range */
			XLSharedStringsRef m_sharedStrings;
		};
		/**
		 * @brief The XLRowDataProxy is used as a proxy object when getting or setting row data. The class facilitates easy conversion
		 * to/from containers.
		 */
		class XLRowDataProxy {
			friend class XLRow;
		public:
			~XLRowDataProxy();
			/**
			 * @brief Copy assignment operator.
			 * @param other Object to be copied.
			 * @return A reference to the copied-to object.
			 */
			XLRowDataProxy& operator=(const XLRowDataProxy& other);
			/**
			 * @brief Assignment operator taking a std::vector of XLCellValues as an argument.
			 * @param values A std::vector of XLCellValues representing the values to be assigned.
			 * @return A reference to the copied-to object.
			 */
			XLRowDataProxy& operator=(const std::vector<XLCellValue>& values);
			/**
			 * @brief Assignment operator taking a std::vector of bool values as an argument.
			 * @param values A std::vector of bool values representing the values to be assigned.
			 * @return A reference to the copied-to object.
			 */
			XLRowDataProxy& operator=(const std::vector<bool>& values);
			/**
			 * @brief Templated assignment operator taking any container supporting bidirectional iterators.
			 * @tparam T The container and value type (will be auto deducted by the compiler).
			 * @param values The container of the values to be assigned.
			 * @return A reference to the copied-to object.
			 * @throws XLOverflowError if size of container exceeds maximum number of columns.
			 */
			template <typename T,
				typename = std::enable_if_t<!std::is_same_v<T, XLRowDataProxy> &&
				std::is_base_of_v<std::bidirectional_iterator_tag,
				typename std::iterator_traits<typename T::iterator>::iterator_category>,
				T> >
			XLRowDataProxy& operator=(const T& values)    // 2024-04-30: whitespace support
			{
				if(values.size() > MAX_COLS)  
					throw XLOverflowError("Container size exceeds maximum number of columns.");
				if(values.size() == 0)  
					return *this;
				// ===== If the container value_type is XLCellValue, the values can be copied directly.
				if constexpr(std::is_same_v<typename T::value_type, XLCellValue>) {
					// ===== First, delete the values in the first N columns.
					deleteCellValues(values.size()); // 2024-04-30: whitespace support
					// ===== Then, prepend new cell nodes to current row node
					auto colNo = values.size();
					for(auto value = values.rbegin(); value != values.rend(); ++value) {
						prependCellValue(*value, colNo); // 2024-04-30: whitespace support: this is safe because only prependCellValue (with
														 // whitespace support) touches the row data
						--colNo;
					}
				}
				// ===== If the container value_type is a POD type, use the overloaded operator= on each cell.
				else {
					auto range = XLRowDataRange(*m_rowNode, 1, values.size(), getSharedStrings());
					auto dst   = range.begin();// 2024-04-30: whitespace support: safe because XLRowDataRange::begin invokes whitespace-safe getCellNode for column 1
					auto src = values.begin();
					while(true) {
						dst->value() = *src;
						++src;
						if(src == values.end())  break;
						++dst; // 2024-04-30: whitespace support: XLRowDataIterator::operator++ is whitespace-safe
					}
				}
				return *this;
			}
			// // BEGIN working template header
			//         template<
			//             typename T,
			//             typename std::enable_if<
			//                 !std::is_same_v< T, XLRowDataProxy >
			//                  && std::is_base_of_v< XMLNode, T >,
			//                 T
			//             >::type* = nullptr
			//         >
			// // END working template header
			//         XLRowDataProxy& operator=(const T& values)
			//         {
			// using namespace std::literals::string_literals;
			// throw XLInternalError( "templated XLRowDataProxy& operator=(const T& values) instantiated for an XMLNode ("s+ typeid(T).name() +
			// "), this function must be implemented then"s );
			//
			//         }
			/**
			 * @brief Implicit conversion to std::vector of XLCellValues.
			 * @return A std::vector of XLCellValues.
			 */
			operator std::vector<XLCellValue>() const;
			/**
			 * @brief Implicit conversion to std::deque of XLCellValues.
			 * @return A std::deque of XLCellValues.
			 */
			operator std::deque<XLCellValue>() const;
			/**
			 * @brief Implicit conversion to std::list of XLCellValues.
			 * @return A std::list of XLCellValues.
			 */
			operator std::list<XLCellValue>() const;
			/**
			 * @brief Explicit conversion operator.
			 * @details This function calls the convertContainer template function to convert the row data to the container
			 * stipulated by the client. The reason that this function is marked explicit is that the implicit conversion operators
			 * above will be ambiguous.
			 * @tparam Container The container (and value) type to convert the row data to.
			 * @return The required container with the row data.
			 */
			template <typename Container, typename =
				std::enable_if_t<!std::is_same_v<Container, XLRowDataProxy> &&
				std::is_base_of_v<std::bidirectional_iterator_tag,
				typename std::iterator_traits<typename Container::iterator>::iterator_category>,
				Container> >
			explicit operator Container() const { return convertContainer<Container>(); }
			/**
			 * @brief Clears all values for the current row.
			 */
			void clear();
		private:
			/**
			 * @param row Pointer to the parent XLRow object.
			 * @param rowNode Pointer to the underlying XML node representing the row.
			 */
			XLRowDataProxy(XLRow* row, XMLNode* rowNode);
			/**
			 * @brief Copy constructor.
			 * @param other Object to be copied.
			 * @note The copy constructor is private in order to prevent use of the auto keyword in client code.
			 */
			XLRowDataProxy(const XLRowDataProxy& other);
			/**
			 * @brief Move constructor.
			 * @param other Object to be moved.
			 * @note Made private, as move construction should only be allowed when the parent object is moved. Disallowed for client code.
			 */
			XLRowDataProxy(XLRowDataProxy&& other) noexcept;
			/**
			 * @brief Move assignment operator.
			 * @param other Object to be moved.
			 * @return Reference to the moved-to object.
			 * @note Made private, as move assignment is disallowed for client code.
			 */
			XLRowDataProxy& operator=(XLRowDataProxy&& other) noexcept;
			/**
			 * @brief Get the cell values for the row.
			 * @return A std::vector of XLCellValues.
			 */
			std::vector<XLCellValue> getValues() const;
			/**
			 * @brief Helper function for getting a reference to the shared strings repository.
			 * @return A reference to the XLSharedStrings object.
			 * @note needed for templated XLRowDataProxy& operator=
			 */
			const XLSharedStrings& getSharedStrings() const;
			/**
			 * @brief Convenience function for erasing the first 'count' numbers of values in the row.
			 * @param count The number of values to erase.
			 */
			void deleteCellValues(uint16 count);
			/**
			 * @brief Convenience function for prepending a row value with a given column number.
			 * @param value The XLCellValue object.
			 * @param col The column of the value.
			 */
			void prependCellValue(const XLCellValue& value, uint16 col);
			/**
			 * @brief Convenience function for converting the row data to a user-supplied container.
			 * @details This function can convert row data to any user-supplied container that adheres to the design
			 * of STL containers and supports bidirectional iterators. This could be std::vector, std::deque, or
			 * std::list, but any container with the same interface should work.
			 * @tparam Container The container (and value) type to be returned.
			 * @return The row data in the required format.
			 * @throws bad_variant_access if Container::value type is not XLCellValue and does not match the type contained.
			 */
			template <typename Container,
				typename =
				std::enable_if_t<!std::is_same_v<Container, XLRowDataProxy> &&
				std::is_base_of_v<std::bidirectional_iterator_tag,
				typename std::iterator_traits<typename Container::iterator>::iterator_category>,
				Container> >
			Container convertContainer() const    // 2024-04-30: whitespace support
			{
				Container c;
				auto it = std::inserter(c, c.end());
				for(const auto& v : getValues()) {
					// ===== If the value_type of the container is XLCellValue, the value can be assigned directly.
					if constexpr(std::is_same_v<typename Container::value_type, XLCellValue>)  *it++ = v;

					// ===== If the value_type is something else, the underlying value has to be extracted from the XLCellValue object.
					// ===== Note that if the type contained in the XLCellValue object does not match the value_type, a bad_variant_access
					// ===== exception will be thrown.
					else
						*it++ = v.get<typename Container::value_type>();
				}
				return c;
			}
			XLRow * m_row { nullptr };     /**< Pointer to the parent XLRow object. */
			XMLNode * m_rowNode { nullptr }; /**< Pointer the the XML node representing the row. */
		};
		//
		//#include "XLRow.hpp"
		class XLRowRange;
		/**
		 * @brief The XLRow class represent a row in an Excel spreadsheet. Using XLRow objects, various row formatting
		 * options can be set and modified.
		 */
		class XLRow {
			friend class XLRowIterator;
			friend class XLRowDataProxy;
			friend bool operator==(const XLRow& lhs, const XLRow& rhs);
			friend bool operator!=(const XLRow& lhs, const XLRow& rhs);
			friend bool operator<(const XLRow& lhs, const XLRow& rhs);
			friend bool operator>(const XLRow& lhs, const XLRow& rhs);
			friend bool operator<=(const XLRow& lhs, const XLRow& rhs);
			friend bool operator>=(const XLRow& lhs, const XLRow& rhs);
		public:
			XLRow();
			XLRow(const XMLNode& rowNode, const XLSharedStrings& sharedStrings);
			XLRow(const XLRow& other);
			XLRow(XLRow&& other) noexcept;
			~XLRow();
			XLRow& operator=(const XLRow& other);
			XLRow& operator=(XLRow&& other) noexcept;
			/**
			 * @brief test if row object has no (valid) content
			 */
			bool empty() const;
			/**
			 * @brief opposite of empty()
			 */
			explicit operator bool() const;
			/**
			 * @brief Get the height of the row.
			 * @return the row height.
			 */
			double height() const;
			/**
			 * @brief Set the height of the row.
			 * @param height The height of the row.
			 */
			void setHeight(float height);
			/**
			 * @brief Get the descent of the row, which is the vertical distance in pixels from the bottom of the cells
			 * in the current row to the typographical baseline of the cell content.
			 * @return The row descent.
			 */
			float descent() const;
			/**
			 * @brief Set the descent of the row, which is he vertical distance in pixels from the bottom of the cells
			 * in the current row to the typographical baseline of the cell content.
			 * @param descent The row descent.
			 */
			void setDescent(float descent);
			/**
			 * @brief Is the row hidden?
			 * @return The state of the row.
			 */
			bool isHidden() const;
			/**
			 * @brief Set the row to be hidden or visible.
			 * @param state The state of the row.
			 */
			void setHidden(bool state);
			uint32 rowNumber() const;
			/**
			 * @brief Get the number of cells in the row.
			 * @return The number of cells in the row.
			 */
			uint16 cellCount() const;
			XLRowDataProxy& values();
			const XLRowDataProxy& values() const;
			template <typename T> T values() const { return static_cast<T>(values()); }
			XLRowDataRange cells() const;
			XLRowDataRange cells(uint16 cellCount) const;
			XLRowDataRange cells(uint16 firstCell, uint16 lastCell) const;
			/**
			 * @brief Find a cell at columNumber, or return an empty cell
			 * @param columNumber The column at which to check for an existing cell
			 * @return An XLCell object (that bool-evaluates to false if cell was not found)
			 */
			XLCell findCell(uint16 columNumber);
			/**
			 * @brief Get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style assigned to the row.
			 *        This value is stored in the row attributes like so: s="2"
			 * @returns The index of the applicable format style
			 */
			XLStyleIndex format() const;
			/**
			 * @brief Set the row style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
			 * @param cellFormatIndex The style to set, corresponding to the index of XLStyles::cellStyles()
			 * @returns true on success, false on failure
			 */
			bool setFormat(XLStyleIndex cellFormatIndex);
		private:
			static bool isEqual(const XLRow& lhs, const XLRow& rhs);
			static bool isLessThan(const XLRow& lhs, const XLRow& rhs);

			std::unique_ptr<XMLNode> m_rowNode;       /**< The XMLNode object for the row. */
			XLSharedStringsRef m_sharedStrings;
			XLRowDataProxy m_rowDataProxy;
		};

		class XLRowIterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using value_type        = XLRow;
			using difference_type   = int64_t;
			using pointer           = XLRow*;
			using reference         = XLRow&;
			explicit XLRowIterator(const XLRowRange& rowRange, XLIteratorLocation loc);
			~XLRowIterator();
			XLRowIterator(const XLRowIterator& other);
			XLRowIterator(XLRowIterator&& other) noexcept;
			XLRowIterator& operator=(const XLRowIterator& other);
			XLRowIterator& operator=(XLRowIterator&& other) noexcept;

		private:        // ===== Switch to private method that is used by the XLRowIterator increment operator++ and the dereference operators * and ->
			static constexpr const bool XLCreateIfMissing      = true;     // code readability for updateCurrentRow parameter createIfMissing
			static constexpr const bool XLDoNotCreateIfMissing = false;    //   "
			/**
			 * @brief update m_currentRow by fetching (or inserting) a row at m_currentRowNumber
			 * @param createIfMissing m_currentRow will only be inserted if createIfMissing is true
			 */
			void updateCurrentRow(bool createIfMissing);

		public:         // ===== Switch back to public methods
			XLRowIterator& operator++();
			XLRowIterator operator++(int);
			reference operator*();
			pointer operator->();
			bool operator==(const XLRowIterator& rhs) const;
			bool operator!=(const XLRowIterator& rhs) const;
			explicit operator bool() const;
			/**
			 * @brief determine whether the row that the iterator points to exists (m_currentRowNumber)
			 * @return true if XML already has an entry for that cell, otherwise false
			 */
			bool rowExists();
			/**
			 * @brief determine whether iterator is at 1 beyond the last row in range
			 * @return
			 */
			bool endReached() const { return m_endReached; }

			/**
			 * @brief get the row number corresponding to the current iterator position
			 * @return a row number, with m_lastRow + 1 for the beyond-the-end iterator
			 */
			uint32 rowNumber() const { return m_endReached ? m_lastRow + 1 : m_currentRowNumber; }
		private:
			std::unique_ptr<XMLNode> m_dataNode;
			uint32 m_firstRow { 1 }; /**< The cell reference of the first cell in the range */
			uint32 m_lastRow { 1 };  /**< The cell reference of the last cell in the range */
			XLRow m_currentRow;
			XLSharedStringsRef m_sharedStrings;

			// helper variables for non-creating iterator functionality
			bool m_endReached;
			XMLNode m_hintRow;                               /**< The cell node of the last existing row found up to current iterator position */
			uint32 m_hintRowNumber;                        /**<   the row number for m_hintRow */
			static constexpr const int XLNotLoaded  = 0;    // code readability for m_currentRowStatus
			static constexpr const int XLNoSuchRow  = 1;    //   "
			static constexpr const int XLLoaded     = 2;    //   "
			int m_currentRowStatus;                         /**< Status of m_currentRow: XLNotLoaded, XLNoSuchRow or XLLoaded */
			uint32 m_currentRowNumber;
		};

		class XLRowRange {
			friend class XLRowIterator;
		public:
			explicit XLRowRange(const XMLNode& dataNode, uint32 first, uint32 last, const XLSharedStrings& sharedStrings);
			XLRowRange(const XLRowRange& other);
			XLRowRange(XLRowRange&& other) noexcept;
			~XLRowRange();
			XLRowRange& operator=(const XLRowRange& other);
			XLRowRange& operator=(XLRowRange&& other) noexcept;
			uint32 rowCount() const;
			XLRowIterator begin();
			XLRowIterator end();
		private:
			std::unique_ptr<XMLNode> m_dataNode;
			uint32 m_firstRow;                      /**< The cell reference of the first cell in the range */
			uint32 m_lastRow;                       /**< The cell reference of the last cell in the range */
			XLSharedStringsRef m_sharedStrings;
		};

		inline bool operator==(const XLRow& lhs, const XLRow& rhs) { return XLRow::isEqual(lhs, rhs); }
		inline bool operator!=(const XLRow& lhs, const XLRow& rhs) { return !(lhs.m_rowNode == rhs.m_rowNode); }
		inline bool operator<(const XLRow& lhs, const XLRow& rhs) { return XLRow::isLessThan(lhs, rhs); }
		inline bool operator>(const XLRow& lhs, const XLRow& rhs) { return (rhs < lhs); }
		inline bool operator<=(const XLRow& lhs, const XLRow& rhs) { return !(lhs > rhs); }
		inline bool operator>=(const XLRow& lhs, const XLRow& rhs) { return !(lhs < rhs); }
		//
		//#include "XLMergeCells.hpp"
		typedef int32_t XLMergeIndex;
		constexpr const XLMergeIndex XLMergeNotFound = -1;

		// pull request #261: wrapped max in parentheses to prevent expansion of windows.h "max" macro
		constexpr size_t XLMaxMergeCells = (std::numeric_limits< XLMergeIndex >::max)();
		/**
		 * @brief This class encapsulate the Excel concept of <mergeCells>. Each worksheet that has merged cells has a list of
		 * (empty) <mergeCell> elements within that array, with a sole attribute ref="..." with ... being a range reference, e.g. A1:B5
		 * Unfortunately, since an empty <mergeCells> element is not allowed, the class must have access to the worksheet root node and
		 *  delete the <mergeCells> element each time the merge count is zero
		 */
		class XLMergeCells {
		public:
			XLMergeCells();
			/**
			 * @param node The root node of the worksheet document - must not be an empty node
			 * @param nodeOrder the worksheet node sequence to respect when inserting <mergeCells> node
			 */
			explicit XLMergeCells(const XMLNode& rootNode, std::vector< std::string_view > const & nodeOrder);
			~XLMergeCells();
			XLMergeCells(const XLMergeCells& other);
			XLMergeCells(XLMergeCells&& other);
			XLMergeCells& operator=(const XLMergeCells& other);
			XLMergeCells& operator=(XLMergeCells&& other);
			/**
			 * @brief test if XLMergeCells has been initialized with a valid XMLNode
			 * @return true if m_rootNode is neither nullptr nor an empty XMLNode
			 */
			bool valid() const;
			/**
			 * @brief get the index of a <mergeCell> entry by its reference
			 * @param reference the reference to search for
			 * @return XLMergeNotFound (-1) if no such reference exists, 0-based index otherwise
			 */
			XLMergeIndex findMerge(const std::string& reference) const;
			/**
			 * @brief test if a mergeCell with reference exists, equivalent to findMerge(reference) >= 0
			 * @param reference the reference to find
			 * @return true if reference exists, false otherwise
			 */
			bool mergeExists(const std::string& reference) const;
			/**
			 * @brief get the index of a <mergeCell> entry of which cellReference is a part
			 * @param cellRef the cell reference (string or XLCellReference) to search for in the merged ranges
			 * @return XLMergeNotFound (-1) if no such reference exists, 0-based index otherwise
			 */
			XLMergeIndex findMergeByCell(const std::string& cellRef) const;
			XLMergeIndex findMergeByCell(XLCellReference cellRef) const;
			/**
			 * @brief get the amount of entries in <mergeCells>
			 * @return the count of defined cell merges
			 */
			size_t count() const;
			/**
			 * @brief return the cell reference string for the given index
			 */
			const char* merge(XLMergeIndex index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to merge
			 */
			const char* operator[](XLMergeIndex index) const { return merge(index); }
			/**
			 * @brief Append a new merge to the list of merges
			 * @param reference The reference to append.
			 * @return An XLMergeIndex with the index of the appended merge
			 * @throws XLInputException if the reference would overlap with an existing reference
			 */
			XLMergeIndex appendMerge(const std::string& reference);
			/**
			 * @brief Delete the merge at the given index.
			 * @param index The index to delete
			 * @note Previously obtained merge indexes will be invalidated when calling deleteMerge
			 * @throws XLInputException if the index does not exist
			 */
			void deleteMerge(XLMergeIndex index);
			/**
			 * @brief Delete all merges of the worksheet
			 */
			void deleteAll();
			/**
			 * @brief print the XML contents of the mergeCells array using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			std::unique_ptr<XMLNode> m_rootNode;         /**< An XMLNode object with the worksheet root node (document element) */
			std::vector< std::string_view > m_nodeOrder; /**< worksheet XML root node required child sequence as passed into constructor */
			std::unique_ptr<XMLNode> m_mergeCellsNode; /**< An XMLNode object with the mergeCells item */
			std::deque<std::string> m_referenceCache;
		};
		//
		//#include "XLSheet.hpp"
		constexpr const bool XLEmptyHiddenCells = true;            // readability constant for XLWorksheet::mergeCells parameter emptyHiddenCells
		/**
		 * @brief The XLSheetState is an enumeration of the possible (visibility) states, e.g. Visible or Hidden.
		 */
		enum class XLSheetState : uint8_t { 
			Visible, 
			Hidden, 
			VeryHidden 
		};

		constexpr const uint16 XLPriorityNotSet = 0;     // readability constant for XLCfRule::priority()

		enum class XLCfType : uint8_t {
			Expression        =   0,
			CellIs            =   1,
			ColorScale        =   2,
			DataBar           =   3,
			IconSet           =   4,
			Top10             =   5,
			UniqueValues      =   6,
			DuplicateValues   =   7,
			ContainsText      =   8,
			NotContainsText   =   9,
			BeginsWith        =  10,
			EndsWith          =  11,
			ContainsBlanks    =  12,
			NotContainsBlanks =  13,
			ContainsErrors    =  14,
			NotContainsErrors =  15,
			TimePeriod        =  16,
			AboveAverage      =  17,
			Invalid           = 255
		};

		enum class XLCfOperator : uint8_t {
			LessThan           =   0,
			LessThanOrEqual    =   1,
			Equal              =   2,
			NotEqual           =   3,
			GreaterThanOrEqual =   4,
			GreaterThan        =   5,
			Between            =   6,
			NotBetween         =   7,
			ContainsText       =   8,
			NotContains        =   9,
			BeginsWith         =  10,
			EndsWith           =  11,
			Invalid            = 255
		};

		enum class XLCfTimePeriod : uint8_t {
			Today     =   0,
			Yesterday =   1,
			Tomorrow  =   2,
			Last7Days =   3,
			ThisMonth =   4,
			LastMonth =   5,
			NextMonth =   6,
			ThisWeek  =   7,
			LastWeek  =   8,
			NextWeek  =   9,
			Invalid   = 255
		};

		const std::vector< std::string_view > XLWorksheetNodeOrder = { // worksheet XML root node required child sequence
			"sheetPr",
			"dimension",
			"sheetViews",
			"sheetFormatPr",
			"cols",
			"sheetData",
			"sheetCalcPr",
			"sheetProtection",
			"protectedRanges",
			"scenarios",
			"autoFilter",
			"sortState",
			"dataConsolidate",
			"customSheetViews",
			"mergeCells",
			"phoneticPr",
			"conditionalFormatting",
			"dataValidations",
			"hyperlinks",
			"printOptions",
			"pageMargins",
			"pageSetup",
			"headerFooter",
			"rowBreaks",
			"colBreaks",
			"customProperties",
			"cellWatches",
			"ignoredErrors",
			"smartTags",
			"drawing",
			"legacyDrawing",
			"legacyDrawingHF",
			"picture",
			"oleObjects",
			"controls",
			"webPublishItems",
			"tableParts",
			"extLst"
		};     // END: const std::vector< std::string_view > XLWorksheetNodeOrder

		//
		// Converter functions between OpenXLSX class specific enum class types and OOXML values
		//
		XLCfType       XLCfTypeFromString(std::string const& typeString);
		std::string    XLCfTypeToString(XLCfType cfType);
		XLCfOperator   XLCfOperatorFromString(std::string const& operatorString);
		std::string    XLCfOperatorToString(XLCfOperator cfOperator);
		XLCfTimePeriod XLCfTimePeriodFromString(std::string const& timePeriodString);
		std::string    XLCfTimePeriodToString(XLCfTimePeriod cfTimePeriod);

		/**
		 * @brief The XLSheetBase class is the base class for the XLWorksheet and XLChartsheet classes. However,
		 * it is not a base class in the traditional sense. Rather, it provides common functionality that is
		 * inherited via the CRTP (Curiously Recurring Template Pattern) pattern.
		 * @tparam T Type that will inherit functionality. Restricted to types XLWorksheet and XLChartsheet.
		 */
		template <typename T, typename = std::enable_if_t<std::is_same_v<T, XLWorksheet> || std::is_same_v<T, XLChartsheet> > >
		class XLSheetBase : public XLXmlFile {
		public:
			XLSheetBase() : XLXmlFile(nullptr) 
			{
			}
			/**
			 * @brief The constructor. There are no default constructor, so all parameters must be provided for
			 * constructing an XLAbstractSheet object. Since this is a pure abstract class, instantiation is only
			 * possible via one of the derived classes.
			 * @param xmlData
			 */
			explicit XLSheetBase(XLXmlData* xmlData) : XLXmlFile(xmlData) {}
			/**
			 * @brief The copy constructor.
			 * @param other The object to be copied.
			 * @note The default copy constructor is used, i.e. only shallow copying of pointer data members.
			 */
			XLSheetBase(const XLSheetBase& other) = default;
			XLSheetBase(XLSheetBase&& other) noexcept = default;
			~XLSheetBase() = default;
			/**
			 * @brief Assignment operator
			 * @return A reference to the new object.
			 * @note The default assignment operator is used, i.e. only shallow copying of pointer data members.
			 */
			XLSheetBase& operator=(const XLSheetBase&) = default;
			XLSheetBase& operator=(XLSheetBase&& other) noexcept = default;
			XLSheetState visibility() const
			{
				XLQuery query(XLQueryType::QuerySheetVisibility);
				query.setParam("sheetID", relationshipID());
				const auto state  = parentDoc().execQuery(query).template result<std::string>();
				auto result = XLSheetState::Visible;
				if(state == "visible" || state.empty()) {
					result = XLSheetState::Visible;
				}
				else if(state == "hidden") {
					result = XLSheetState::Hidden;
				}
				else if(state == "veryHidden") {
					result = XLSheetState::VeryHidden;
				}
				return result;
			}
			void setVisibility(XLSheetState state)
			{
				auto stateString = std::string();
				switch(state) {
					case XLSheetState::Visible: stateString = "visible"; break;
					case XLSheetState::Hidden: stateString = "hidden"; break;
					case XLSheetState::VeryHidden: stateString = "veryHidden"; break;
				}
				parentDoc().execCommand(XLCommand(XLCommandType::SetSheetVisibility).setParam("sheetID", relationshipID()).setParam("sheetVisibility", stateString));
			}
			/**
			 * @todo To be implemented.
			 */
			XLColor color() const { return static_cast<const T&>(*this).getColor_impl(); }
			void setColor(const XLColor& color) { static_cast<T&>(*this).setColor_impl(color); }
			/**
			 * @brief look up sheet name via relationshipID, then attempt to match that to a sheet in XLWorkbook::sheet(uint16 index), looping over index
			 * @return the index by which the sheet could be accessed from XLWorkbook::sheet
			 */
			uint16 index() const
			{
				XLQuery query(XLQueryType::QuerySheetIndex);
				query.setParam("sheetID", relationshipID());
				return static_cast<uint16>(std::stoi(parentDoc().execQuery(query).template result<std::string>()));
			}
			void setIndex(uint16 index)
			{
				parentDoc().execCommand(XLCommand(XLCommandType::SetSheetIndex).setParam("sheetID", relationshipID()).setParam("sheetIndex", index));
			}
			/**
			 * @brief Method to retrieve the name of the sheet.
			 * @return A std::string with the sheet name.
			 */
			std::string name() const
			{
				XLQuery query(XLQueryType::QuerySheetName);
				query.setParam("sheetID", relationshipID());
				return parentDoc().execQuery(query).template result<std::string>();
			}
			/**
			 * @brief Method for renaming the sheet.
			 * @param sheetName A std::string with the new name.
			 */
			void setName(const std::string & sheetName)
			{
				parentDoc().execCommand(XLCommand(XLCommandType::SetSheetName).setParam("sheetID", relationshipID()).setParam("sheetName", name()).setParam("newName", sheetName));
			}
			bool isSelected() const { return static_cast<const T&>(*this).isSelected_impl(); }
			void setSelected(bool selected)
			{
				static_cast<T&>(*this).setSelected_impl(selected);
			}
			bool isActive() const
			{
				return static_cast<const T&>(*this).isActive_impl();
			}
			bool setActive()
			{
				return static_cast<T&>(*this).setActive_impl();
			}
			/**
			 * @brief Method for cloning the sheet.
			 * @param newName A std::string with the name of the clone
			 * @return A pointer to the cloned object.
			 * @note This is a pure abstract method. I.e. it is implemented in subclasses.
			 */
			void clone(const std::string& newName)
			{
				parentDoc().execCommand(XLCommand(XLCommandType::CloneSheet).setParam("sheetID", relationshipID()).setParam("cloneName", newName));
			}
		};
		/**
		 * @brief An encapsulation of a cfRule item
		 */
		class XLCfRule {
			friend class XLCfRules;    // for access to m_cfRuleNode in XLCfRules::create
		public:
			XLCfRule();
			/**
			 * @brief Constructor. New items should only be created through an XLCfRules object.
			 * @param node An XMLNode object with the <cfRule> item. If no input is provided, a null node is used.
			 */
			explicit XLCfRule(const XMLNode& node);
			XLCfRule(const XLCfRule& other);
			XLCfRule(XLCfRule&& other) noexcept = default;
			~XLCfRule();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCfRule& operator=(const XLCfRule& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCfRule& operator=(XLCfRule&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Element getter functions
			 */
			std::string formula() const;
			XLUnsupportedElement colorScale() const;
			XLUnsupportedElement dataBar()    const;
			XLUnsupportedElement iconSet()    const;
			XLUnsupportedElement extLst()     const;
			/**
			 * @brief Attribute getter functions
			 */
			XLCfType type() const;
			XLStyleIndex dxfId() const;
			uint16 priority() const; // >= 1
			bool stopIfTrue() const;   // default: false
			bool aboveAverage() const; // default: true
			bool percent() const; // default: false
			bool bottom() const; // default: false
			XLCfOperator Operator() const; // Valid only when @type = cellIs
			std::string text() const; // The text value in a "text contains" conditional formatting rule. Valid only for @type = containsText.
			XLCfTimePeriod timePeriod() const; // The applicable time period in a "date occurring..." conditional formatting rule. Valid only for @type = timePeriod.
			uint16 rank() const; // The value of "n" in a "top/bottom n" conditional formatting rule. Valid only for @type = top10.
			int16 stdDev() const; // The number of standard deviations to include above or below the average in the conditional formatting rule. Valid only for @type = aboveAverage
			bool equalAverage() const; // default: false. Flag indicating whether the 'aboveAverage' and 'belowAverage' criteria
			//                             is inclusive of the average itself, or exclusive of that value. '1' indicates to include
			//                             the average value in the criteria. Valid only for @type = aboveAverage.
			/**
			 * @brief Element setter functions
			 */
			bool setFormula(std::string const& newFormula);
			bool setColorScale(XLUnsupportedElement const& newColorScale); // unsupported
			bool setDataBar(XLUnsupportedElement const& newDataBar);       // unsupported
			bool setIconSet(XLUnsupportedElement const& newIconSet);       // unsupported
			bool setExtLst(XLUnsupportedElement const& newExtLst);         // unsupported
			/**
			 * @brief Attribute setter functions
			 */
			bool setType(XLCfType newType);
			bool setDxfId(XLStyleIndex newDxfId);
		private:     // Protect setPriority from access by any but the parent XLCfRules class
			bool setPriority(uint16 newPriority); // internal function, user access through XLCfRules::setPriority
		public:
			bool setStopIfTrue(bool set = true);
			bool setAboveAverage(bool set = true);
			bool setPercent(bool set = true);
			bool setBottom(bool set = true);
			bool setOperator(XLCfOperator newOperator);
			bool setText(std::string const& newText);
			bool setTimePeriod(XLCfTimePeriod newTimePeriod);
			bool setRank(uint16 newRank);
			bool setStdDev(int16 newStdDev);
			bool setEqualAverage(bool set = true);
			/**
			 * @brief Return a string summary of the cfRule properties
			 * @return string with info about the cfRule object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_cfRuleNode;  /**< An XMLNode object with the conditional formatting item */
			inline static const std::vector< std::string_view > m_nodeOrder = {      // cfRule XML node required child sequence
				"formula", // TODO: maxOccurs = 3!
				"colorScale",
				"dataBar",
				"iconSet",
				"extLst"
			};
		};

		constexpr const char * XLDefaultCfRulePrefix = "\n\t\t";     // indentation to use for newly created cfRule nodes
		/**
		 * @brief An encapsulation of a conditional formatting rules item
		 */
		class XLCfRules {
		public:
			XLCfRules();
			/**
			 * @brief Constructor. New items should only be created through an XLConditionalFormat object.
			 * @param node An XMLNode object with the conditionalFormatting item. If no input is provided, a null node is used.
			 */
			explicit XLCfRules(const XMLNode& node);
			XLCfRules(const XLCfRules& other);
			XLCfRules(XLCfRules&& other) noexcept = default;
			~XLCfRules();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLCfRules& operator=(const XLCfRules& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLCfRules& operator=(XLCfRules&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Get the count highest priority value currently in use by a rule
			 * @return The highest value that a cfRule has set for attribute priority
			 */
			uint16 maxPriorityValue() const;
			/**
			 * @brief Provide a duplication-free interface to setting rule priorities: if a newPriority value exists, increment all existing priorities >= newPriority by 1
			 * @param cfRuleIndex the index of the rule for which to set the priority
			 * @param newPriority the priority value to assign
			 * @return true on success, false if ruleIndex does not exist
			 */
			bool setPriority(size_t cfRuleIndex, uint16 newPriority);
			/**
			 * @brief Renumber all existing rule priorities to a clean sequence in steps of increment
			 * @param increment increase the priority value by this much between rules
			 */
			void renumberPriorities(uint16 increment = 1);
			/**
			 * @brief Get the count of rules for this conditional formatting
			 * @return The amount of <cfRule> nodes in the <conditionalFormatting> node
			 */
			size_t count() const;
			/**
			 * @brief Get the conditional format identified by index
			 * @return An XLCfRule object
			 */
			XLCfRule cfRuleByIndex(size_t index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to cfRuleByIndex
			 * @param index The index within the XML sequence
			 * @return An XLCfRule object
			 */
			XLCfRule operator[](size_t index) const { return cfRuleByIndex(index); }
			/**
			 * @brief Append a new XLCfRule, based on copyFrom, and return its index in the <conditionalFormatting> list of <cfRule> entries
			 * @param copyFrom Can provide an XLCfRule to use as template for the new condition
			 * @param conditionalFormattingPrefix Prefix the newly created conditionalFormatting XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new conditional formatting as used by operator[]
			 */
			size_t create(XLCfRule copyFrom = XLCfRule{}, std::string cfRulePrefix = XLDefaultCfRulePrefix);
			/**
			 * @brief Return a string summary of the conditional formatting rules properties
			 * @return string with info about the conditional formatting rules object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_conditionalFormattingNode;  /**< An XMLNode object with the conditional formatting item */
			// TODO: pass in m_nodeOrder from XLConditionalFormat
			inline static const std::vector< std::string_view > m_nodeOrder = {      // conditionalFormatting XML node required child sequence
				"cfRule",
				"extLst"
			};
		};
		/**
		 * @brief An encapsulation of a conditional formatting item
		 */
		class XLConditionalFormat {
			friend class XLConditionalFormats;    // for access to m_conditionalFormattingNode in XLConditionalFormats::create
		public:
			XLConditionalFormat();
			/**
			 * @brief Constructor. New items should only be created through an XLWorksheet object.
			 * @param node An XMLNode object with the conditionalFormatting item. If no input is provided, a null node is used.
			 */
			explicit XLConditionalFormat(const XMLNode& node);
			XLConditionalFormat(const XLConditionalFormat& other);
			XLConditionalFormat(XLConditionalFormat&& other) noexcept = default;
			~XLConditionalFormat();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLConditionalFormat& operator=(const XLConditionalFormat& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLConditionalFormat& operator=(XLConditionalFormat&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Get the sqref property - the range over which these conditional formatting rules apply
			 * @return string of the sqref property
			 */
			std::string sqref() const;
			/**
			 * @brief Get the conditional formatting rules to be applied to sqref
			 * @return the conditional formatting rules applicable to sqref
			 */
			XLCfRules cfRules() const;
			/**
			 * @brief Unsupported getter
			 */
			XLUnsupportedElement extLst() const { return XLUnsupportedElement{}; } // <conditionalFormatting><extLst>...</extLst></conditionalFormatting>
			/**
			 * @brief Setter functions for conditional formatting parameters
			 * @param value that shall be set
			 * @return true for success, false for failure
			 */
			bool setSqref(std::string newSqref);
			/**
			 * @brief Unsupported setter
			 */
			bool setExtLst(XLUnsupportedElement const& newExtLst);
			/**
			 * @brief Return a string summary of the conditional format properties
			 * @return string with info about the conditional formatting object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_conditionalFormattingNode;  /**< An XMLNode object with the conditional formatting item */
			inline static const std::vector< std::string_view > m_nodeOrder = {   // conditionalFormatting XML node required child sequence
				"cfRule",
				"extLst"
			};
		};

		constexpr const char * XLDefaultConditionalFormattingPrefix = "\n\t";     // indentation to use for newly created conditional formatting nodes
		/**
		 * @brief An encapsulation of the XLSX conditional formats
		 */
		class XLConditionalFormats {
		public:
			XLConditionalFormats();
			/**
			 * @brief Constructor. New items should only be created through an XLWorksheet object.
			 * @param node An XMLNode object with the worksheet root node. Required to access / manipulate any conditional formats
			 */
			explicit XLConditionalFormats(const XMLNode& node);
			XLConditionalFormats(const XLConditionalFormats& other);
			XLConditionalFormats(XLConditionalFormats&& other);
			~XLConditionalFormats();
			/**
			 * @brief Copy assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to the lhs object.
			 */
			XLConditionalFormats& operator=(const XLConditionalFormats& other);
			/**
			 * @brief Move assignment operator.
			 * @param other Right hand side of assignment operation.
			 * @return A reference to lhs object.
			 */
			XLConditionalFormats& operator=(XLConditionalFormats&& other) noexcept = default;
			/**
			 * @brief Test if this is an empty node
			 * @return true if underlying XMLNode is empty
			 */
			bool empty() const;
			/**
			 * @brief Get the count of conditional formatting settings
			 * @return The amount of <conditionalFormatting> nodes in the worksheet
			 */
			size_t count() const;
			/**
			 * @brief Get the conditional format identified by index
			 * @return An XLConditionalFormat object
			 */
			XLConditionalFormat conditionalFormatByIndex(size_t index) const;
			/**
			 * @brief Operator overload: allow [] as shortcut access to conditionalFormatByIndex
			 * @param index The index within the XML sequence
			 * @return An XLConditionalFormat object
			 */
			XLConditionalFormat operator[](size_t index) const { return conditionalFormatByIndex(index); }
			/**
			 * @brief Append a new XLConditionalFormat, based on copyFrom, and return its index in the worksheet's conditional formats
			 * @param copyFrom Can provide an XLConditionalFormat to use as template for the new condition
			 * @param conditionalFormattingPrefix Prefix the newly created conditionalFormatting XMLNode with this pugi::node_pcdata text
			 * @returns The index of the new conditional formatting as used by operator[]
			 */
			size_t create(XLConditionalFormat copyFrom = XLConditionalFormat{}, std::string conditionalFormattingPrefix = XLDefaultConditionalFormattingPrefix);
			/**
			 * @brief Return a string summary of the conditional formattings properties
			 * @return string with info about the conditional formattings object
			 */
			std::string summary() const;
		private:
			std::unique_ptr<XMLNode> m_sheetNode;   /**< An XMLNode object with the sheet item */
			const std::vector< std::string_view >& m_nodeOrder = XLWorksheetNodeOrder;  // worksheet XML root node required child sequence
		};
		/**
		 * @brief A class encapsulating an Excel worksheet. Access to XLWorksheet objects should be via the workbook object.
		 */
		class XLWorksheet final : public XLSheetBase<XLWorksheet> {
			friend class XLCell;
			friend class XLRow;
			friend class XLWorkbook;
			friend class XLSheetBase<XLWorksheet>;
		public:
			XLWorksheet() : XLSheetBase(nullptr) {};
			explicit XLWorksheet(XLXmlData* xmlData);
			~XLWorksheet() = default;
			XLWorksheet(const XLWorksheet& other);
			XLWorksheet(XLWorksheet&& other);
			XLWorksheet& operator=(const XLWorksheet& other);
			XLWorksheet& operator=(XLWorksheet&& other);
			XLCellAssignable cell(const std::string& ref) const;
			/**
			 * @brief Get an XLCell object for the given cell reference.
			 * @param ref An XLCellReference object with the address of the cell to get.
			 * @return A reference to the requested XLCell object.
			 */
			XLCellAssignable cell(const XLCellReference& ref) const;
			/**
			 * @brief Get the cell at the given coordinates. Create row & cell XML if missing.
			 * @param rowNumber The row number (index base 1).
			 * @param columnNumber The column number (index base 1).
			 * @return A reference to the XLCell object at the given coordinates.
			 */
			XLCellAssignable cell(uint32 rowNumber, uint16 columnNumber) const;
			/**
			 * @brief overload for findCell(uint32 rowNumber, uint16 columnNumber)
			 * @param ref string with the address of the cell to find
			 */
			XLCellAssignable findCell(const std::string& ref) const;
			/**
			 * @brief overload for findCell(uint32 rowNumber, uint16 columnNumber)
			 * @param ref An XLCellReference object with the address of the cell to find
			 */
			XLCellAssignable findCell(const XLCellReference& ref) const;
			/**
			 * @brief Find the cell at the given coordinates. Do *not* create row & cell XML if missing and return an empty XLCellAssignable instead
			 * @param rowNumber The row number (index base 1).
			 * @param columnNumber The column number (index base 1).
			 * @return A reference to the XLCell object at the given coordinates or an empty XLCell object
			 * @note Must test (XLCell::empty() == false) before accessing any other methods of the returned object.
			 * @warning This is a usability feature with bad performance. When testing a large amounts of cells or working with large worksheets,
			 *           it is better to use an XLCellIterator with the cell range of interest.
			 */
			XLCellAssignable findCell(uint32 rowNumber, uint16 columnNumber) const;
			/**
			 * @brief Get a range for the area currently in use (i.e. from cell A1 to the last cell being in use).
			 * @return An XLCellRange object with the entire range.
			 */
			XLCellRange range() const;
			/**
			 * @brief Get a range with the given coordinates.
			 * @param topLeft An XLCellReference object with the coordinates to the top left cell.
			 * @param bottomRight An XLCellReference object with the coordinates to the bottom right cell.
			 * @return An XLCellRange object with the requested range.
			 */
			XLCellRange range(const XLCellReference& topLeft, const XLCellReference& bottomRight) const;

			/**
			 * @brief Get a range with the given coordinates.
			 * @param topLeft A std::string that is convertible to an XLCellReference to the top left cell
			 * @param bottomRight A std::string that is convertible to an XLCellReference to the bottom right cell.
			 * @return An XLCellRange object with the requested range.
			 */
			XLCellRange range(std::string const& topLeft, std::string const& bottomRight) const;
			/**
			 * @brief Get a range with the given coordinates.
			 * @param rangeReference A std::string that contains two XLCellReferences separated by a delimiter ':'
			 *                       Example "A2:B5"
			 * @return An XLCellRange object with the requested range.
			 */
			XLCellRange range(std::string const& rangeReference) const;
			XLRowRange rows() const;
			XLRowRange rows(uint32 rowCount) const;
			XLRowRange rows(uint32 firstRow, uint32 lastRow) const;
			/**
			 * @brief Get the row with the given row number.
			 * @param rowNumber The number of the row to retrieve.
			 * @return A reference to the XLRow object.
			 */
			XLRow row(uint32 rowNumber) const;
			/**
			 * @brief Get the column with the given column number.
			 * @param columnNumber The number of the column to retrieve.
			 * @return A reference to the XLColumn object.
			 */
			XLColumn column(uint16 columnNumber) const;
			/**
			 * @brief Get the column with the given column reference
			 * @param columnRef The letters referencing the given column
			 * @return A reference to the XLColumn object.
			 */
			XLColumn column(std::string const& columnRef) const;
			/**
			 * @brief Get an XLCellReference to the last (bottom right) cell in the worksheet.
			 * @return An XLCellReference for the last cell.
			 */
			XLCellReference lastCell() const noexcept;
			/**
			 * @brief Get the number of columns in the worksheet.
			 * @return The number of columns.
			 */
			uint16 columnCount() const noexcept;
			/**
			 * @brief Get the number of rows in the worksheet.
			 * @return The number of rows.
			 */
			uint32 rowCount() const noexcept;
			/**
			 * @brief Delete the row with the given row number from the underlying OOXML.
			 * @return true if a row entry existed in OOXML & was deleted, otherwise false
			 * @warning this will break all existing references to that row (formulas etc)
			 */
			bool deleteRow(uint32 rowNumber);
			void updateSheetName(const std::string& oldName, const std::string& newName);
			/**
			 * @brief get an XLMergeCells object to directly access the member functions
			 * @returns an XLMergeCells object for this worksheet
			 */
			XLMergeCells & merges();
			/**
			 * @brief merge the cells indicated by range
			 * @param rangeToMerge the XLCellRange to merge, can be obtained from XLWorksheet::range functions
			 * @param emptyHiddenCells if true (XLEmptyHiddenCells), the values of hidden cells will be deleted
			 *                         (only from the cells, not from the shared strings table, if used)
			 * @throws XLInputException if range comprises < 2 cells or any cell within rangeToMerge is already part of an existing range
			 */
			void mergeCells(XLCellRange const& rangeToMerge, bool emptyHiddenCells = false);
			/**
			 * @brief Convenience wrapper for mergeCells with a std::string range reference
			 * @param rangeReference A range reference string for the cells to merge
			 * @param emptyHiddenCells as above
			 */
			void mergeCells(const std::string& rangeReference, bool emptyHiddenCells = false);
			/**
			 * @brief remove the merge setting for the indicated range
			 * @param rangeToUnmerge the XLCellRange to unmerge, can be obtained from XLWorksheet::range functions
			 * @throws XLInputException if no merged range exists that matches rangeToMerge precisely
			 */
			void unmergeCells(XLCellRange const& rangeToUnmerge);
			/**
			 * @brief Convenience wrapper for unmergeCells with a std::string range reference
			 * @param rangeReference A range reference string for the cells to unmerge
			 */
			void unmergeCells(const std::string& rangeReference);
			/**
			 * @brief Get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style assigned to a column.
			 *        This value is stored in the col attributes like so: style="2"
			 * @param column The column letter(s) or index (1-based)
			 * @returns The index of the applicable format style
			 */
			XLStyleIndex getColumnFormat(uint16 column) const;
			XLStyleIndex getColumnFormat(const std::string& column) const;
			/**
			 * @brief set the column style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
			 *        Subsequently, set the same style for all existing(!) cells in that column
			 * @param column the column letter(s) or index (1-based)
			 * @param cellFormatIndex the style to set, corresponding to the index of XLStyles::cellStyles()
			 * @returns true on success, false on failure
			 */
			bool setColumnFormat(uint16 column, XLStyleIndex cellFormatIndex);
			bool setColumnFormat(const std::string& column, XLStyleIndex cellFormatIndex);
			/**
			 * @brief get the array index of xl/styles.xml:<styleSheet>:<cellXfs> for the style assigned to a row
			 *        this value is stored in the row attributes like so: s="2"
			 * @param row the row index (1-based)
			 * @returns the index of the applicable format style
			 */
			XLStyleIndex getRowFormat(uint16 row) const;
			/**
			 * @brief set the row style as a reference to the array index of xl/styles.xml:<styleSheet>:<cellXfs>
			 *        Subsequently, set the same style for all existing(!) cells in that row
			 * @param row the row index (1-based)
			 * @param cellFormatIndex the style to set, corresponding to the index of XLStyles::cellStyles()
			 * @returns true on success, false on failure
			 */
			bool setRowFormat(uint32 row, XLStyleIndex cellFormatIndex);
			/**
			 * @brief Get the conditional formats object
			 * @return An XLConditionalFormats object
			 */
			XLConditionalFormats conditionalFormats() const;
			/**
			 * @brief Set the <sheetProtection> attributes sheet, objects and scenarios respectively
			 * @return true upon success, false in case of any failure
			 */
			bool protectSheet(bool set = true);
			bool protectObjects(bool set = true);
			bool protectScenarios(bool set = true);
			/**
			 * @brief Set the XML properties that allow the according modification of the worksheet
			 * @param set if true, action will be allowed despite sheet protection, if false, action will be denied / protected
			 * @note #1 default for all (if attribute is not present) is "true" = the action is protected / not allowed
			 * @note #2 none of the sheet protection settings have any effect if protectSheet is not set
			 * @note (library internal) counter-intuitively, the underlying bool attributes need to be set to "false" to "disable protection" for these actions
			 */
			bool allowInsertColumns(bool set = true);       // default: not allowed in a protected worksheet
			bool allowInsertRows(bool set = true);          //   "    :  "
			bool allowDeleteColumns(bool set = true);       //   "    :  "
			bool allowDeleteRows(bool set = true);          //   "    :  "
			bool allowSelectLockedCells(bool set = true);   // default: allowed in a protected worksheet
			bool allowSelectUnlockedCells(bool set = true); //   "    :  "
			/**
			 * @brief Convenience shortcuts for allow<*>( false );
			 */
			bool denyInsertColumns()       { return allowInsertColumns(false); }
			bool denyInsertRows()          { return allowInsertRows(false); }
			bool denyDeleteColumns()       { return allowDeleteColumns(false); }
			bool denyDeleteRows()          { return allowDeleteRows(false); }
			bool denySelectLockedCells()   { return allowSelectLockedCells(false); }
			bool denySelectUnlockedCells() { return allowSelectUnlockedCells(false); }
			/**
			 * @brief Set the sheetProtection password attribute
			 * @param hash directly stores a password hash to the password attribute, for use cases where the password shall not be used in clear text in the API
			 * @note ExcelPasswordHash or ExcelPasswordHashAsString (defined in XLDocument header) can be used to calculate the hash
			 * @note an empty password hash can be used to clear any password protection
			 * @return true upon success, false in case of any failure
			 */
			bool setPasswordHash(std::string hash);
			/**
			 * @brief Set the sheetProtection password attribute
			 * @param password the XLSX password hash for this string will be stored in the password attribute
			 * @note an empty password clears the password attribute
			 * @return true upon success, false in case of any failure
			 */
			bool setPassword(std::string password);
			/**
			 * @brief Clear the sheetProtection password attribute
			 */
			bool clearPassword();
			/**
			 * @brief Clear the sheetProtection completely
			 */
			bool clearSheetProtection();
			/**
			 * @brief getter functions for sheet protection settings
			 * @return true if setting is "action allowed"
			 */
			bool sheetProtected()     const;
			bool objectsProtected()   const;
			bool scenariosProtected() const;
			/**
			 * @brief getter functions for detailed sheet protections
			 * @note none of the sheet protection settings have any effect if protectSheet is not set
			 */
			bool insertColumnsAllowed()       const;
			bool insertRowsAllowed()          const;
			bool deleteColumnsAllowed()       const;
			bool deleteRowsAllowed()          const;
			bool selectLockedCellsAllowed()   const;
			bool selectUnlockedCellsAllowed() const;
			/**
			 * @brief get sheet password hash
			 */
			std::string passwordHash() const;
			/**
			 * @brief check sheet password protection
			 */
			bool passwordIsSet() const;
			/**
			 * @brief assemble a string summary about the sheet protection settings
			 */
			std::string sheetProtectionSummary() const;
			/**
			 * @brief test whether a worksheet relationships XML file exists for this worksheet
			 */
			bool hasRelationships() const;
			/**
			 * @brief test whether a VML drawing XML file exists for this worksheet
			 */
			bool hasVmlDrawing() const;
			/**
			 * @brief test whether a comments XML file exists for this worksheet
			 */
			bool hasComments() const;
			/**
			 * @brief test whether a tables XML file exists for this worksheet
			 */
			bool hasTables() const;
			/**
			 * @brief fetch a reference to the worksheet VML drawing object
			 */
			XLVmlDrawing& vmlDrawing();
			/**
			 * @brief fetch a reference to the worksheet comments
			 */
			XLComments& comments();
			/**
			 * @brief fetch a reference to the worksheet tables
			 */
			XLTables& tables();
		private:
			/**
			 * @brief fetch the # number from the xml path xl/worksheets/sheet#.xml
			 */
			uint16 sheetXmlNumber() const;
			/**
			 * @brief fetch a reference to the worksheet relationships
			 * @note private because transparent to the user
			 */
			XLRelationships& relationships();
			XLColor getColor_impl() const;
			void setColor_impl(const XLColor& color);
			bool isSelected_impl() const;
			void setSelected_impl(bool selected);
			bool isActive_impl() const;
			bool setActive_impl();
		private:
			XLRelationships m_relationships{};    /**< class handling the worksheet relationships */
			XLMergeCells m_merges{};              /**< class handling the <mergeCells> */
			XLVmlDrawing m_vmlDrawing{};          /**< class handling the worksheet VML drawing object */
			XLComments m_comments{};              /**< class handling the worksheet comments */
			XLTables m_tables{};                  /**< class handling the worksheet table settings */
			const std::vector< std::string_view >& m_nodeOrder = XLWorksheetNodeOrder;  // worksheet XML root node required child sequence
		};
		/**
		 * @brief Class representing the an Excel chartsheet.
		 * @todo This class is largely unimplemented and works just as a placeholder.
		 */
		class XLChartsheet final : public XLSheetBase<XLChartsheet> {
			friend class XLSheetBase<XLChartsheet>;
		public:
			XLChartsheet() : XLSheetBase(nullptr) {};
			explicit XLChartsheet(XLXmlData* xmlData);
			XLChartsheet(const XLChartsheet& other) = default;
			XLChartsheet(XLChartsheet&& other) noexcept = default;
			~XLChartsheet();
			XLChartsheet& operator=(const XLChartsheet& other) = default;
			XLChartsheet& operator=(XLChartsheet&& other) noexcept = default;
		private:
			XLColor getColor_impl() const;
			void setColor_impl(const XLColor& color);
			bool isSelected_impl() const;
			void setSelected_impl(bool selected);
			/**
			 * @brief test issue #316 resolution
			 */
			bool isActive_impl() const { return false; } // TODO: implement actual function body, if possible for chartsheets
			/**
			 * @brief test issue #316 resolution
			 */
			bool setActive_impl() { return true; } // TODO: implement actual function body, if possible for chartsheets
		};
		/**
		 * @brief The XLAbstractSheet is a generalized sheet class, which functions as superclass for specialized classes,
		 * such as XLWorksheet. It implements functionality common to all sheet types. This is a pure abstract class,
		 * so it cannot be instantiated.
		 */
		class XLSheet final : public XLXmlFile {
		public:
			/**
			 * @brief The constructor. There are no default constructor, so all parameters must be provided for
			 * constructing an XLAbstractSheet object. Since this is a pure abstract class, instantiation is only
			 * possible via one of the derived classes.
			 * @param xmlData
			 */
			explicit XLSheet(XLXmlData* xmlData);
			/**
			 * @brief The copy constructor.
			 * @param other The object to be copied.
			 * @note The default copy constructor is used, i.e. only shallow copying of pointer data members.
			 */
			XLSheet(const XLSheet& other) = default;
			XLSheet(XLSheet&& other) noexcept = default;
			/**
			 * @brief The destructor
			 * @note The default destructor is used, since cleanup of pointer data members is not required.
			 */
			~XLSheet() = default;
			/**
			 * @brief Assignment operator
			 * @return A reference to the new object.
			 * @note The default assignment operator is used, i.e. only shallow copying of pointer data members.
			 */
			XLSheet& operator=(const XLSheet& other) = default;
			XLSheet& operator=(XLSheet&& other) noexcept = default;
			/**
			 * @brief Method for getting the current visibility state of the sheet.
			 * @return An XLSheetState enum object, with the current sheet state.
			 */
			XLSheetState visibility() const;
			/**
			 * @brief Method for setting the state of the sheet.
			 * @param state An XLSheetState enum object with the new state.
			 * @bug For some reason, this method doesn't work. The data is written correctly to the xml file, but the sheet
			 * is not hidden when opening the file in Excel.
			 */
			void setVisibility(XLSheetState state);
			XLColor color() const;
			void setColor(const XLColor& color);
			/**
			 * @brief Method for getting the index of the sheet.
			 * @return An int with the index of the sheet.
			 */
			uint16 index() const;
			/**
			 * @brief Method for setting the index of the sheet. This effectively moves the sheet to a different position.
			 */
			void setIndex(uint16 index);
			/**
			 * @brief Method to retrieve the name of the sheet.
			 * @return A std::string with the sheet name.
			 */
			std::string name() const;
			/**
			 * @brief Method for renaming the sheet.
			 * @param name A std::string with the new name.
			 */
			void setName(const std::string& name);
			/**
			 * @brief Determine whether the sheet is selected
			 */
			bool isSelected() const;
			void setSelected(bool selected);
			bool isActive() const;
			bool setActive();
			/**
			 * @brief Method to get the type of the sheet.
			 * @return An XLSheetType enum object with the sheet type.
			 */
			template <typename SheetType,
				typename = std::enable_if_t<std::is_same_v<SheetType, XLWorksheet> || std::is_same_v<SheetType, XLChartsheet> > >
			bool isType() const
			{
				return std::holds_alternative<SheetType>(m_sheet);
			}
			/**
			 * @brief Method for cloning the sheet.
			 * @param newName A std::string with the name of the clone
			 * @return A pointer to the cloned object.
			 * @note This is a pure abstract method. I.e. it is implemented in subclasses.
			 */
			void clone(const std::string& newName);

			template <typename T, typename = std::enable_if_t<std::is_same_v<T, XLWorksheet> || std::is_same_v<T, XLChartsheet> > >
			T get() const
			{
				try {
					if constexpr(std::is_same<T, XLWorksheet>::value)
						return std::get<XLWorksheet>(m_sheet);
					else if constexpr(std::is_same<T, XLChartsheet>::value)
						return std::get<XLChartsheet>(m_sheet);
				}
				catch(const std::bad_variant_access&) {
					throw XLSheetError("XLSheet object does not contain the requested sheet type.");
				}
			}
			operator XLWorksheet() const;
			operator XLChartsheet() const;
			/**
			 * @brief Print the XML contents of the XLSheet using the underlying XMLNode print function
			 */
			void print(std::basic_ostream<char>& ostr) const;
		private:
			std::variant<XLWorksheet, XLChartsheet> m_sheet;
		};
	}

#ifdef _MSC_VER // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
	#pragma warning(pop)
#endif // _MSC_VER
	//
#include <sl-packing-reset.h>

#endif    // OPENXLSX_OPENXLSX_HPP
