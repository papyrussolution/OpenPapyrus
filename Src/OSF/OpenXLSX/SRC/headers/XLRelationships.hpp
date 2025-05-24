// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLRELATIONSHIPS_HPP
#define OPENXLSX_XLRELATIONSHIPS_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <random>       // std::mt19937
#include <string>
#include <vector>
#include "XLXmlFile.hpp"
#include "XLXmlParser.hpp"

namespace OpenXLSX {
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
} //     namespace OpenXLSX

namespace OpenXLSX_XLRelationships { // special namespace to avoid naming conflict with another GetStringFromType function
using namespace OpenXLSX;
/**
 * @brief helper function, used only within module and from XLProperties.cpp / XLAppProperties::createFromTemplate
 * @param type the XLRelationshipType for which to return the correct XML string
 */
std::string GetStringFromType(XLRelationshipType type);
} //    namespace OpenXLSX_XLRelationships

namespace OpenXLSX {
/**
 * @brief An encapsulation of a relationship item, i.e. an XML file in the document, its type and an ID number.
 */
class XLRelationshipItem
{
public:        // ---------- Public Member Functions ---------- //
	/**
	 * @brief
	 */
	XLRelationshipItem();

	/**
	 * @brief Constructor. New items should only be created through an XLRelationship object.
	 * @param node An XMLNode object with the relationship item. If no input is provided, a null node is used.
	 */
	explicit XLRelationshipItem(const XMLNode& node);

	/**
	 * @brief Copy Constructor.
	 * @param other Object to be copied.
	 */
	XLRelationshipItem(const XLRelationshipItem& other);

	/**
	 * @brief Move Constructor.
	 * @param other Object to be moved.
	 */
	XLRelationshipItem(XLRelationshipItem&& other) noexcept = default;

	/**
	 * @brief
	 */
	~XLRelationshipItem();

	/**
	 * @brief Copy assignment operator.
	 * @param other Right hand side of assignment operation.
	 * @return A reference to the lhs object.
	 */
	XLRelationshipItem& operator=(const XLRelationshipItem& other);

	/**
	 * @brief Move assignment operator.
	 * @param other Right hand side of assignment operation.
	 * @return A reference to lhs object.
	 */
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

private:                                             // ---------- Private Member Variables ---------- //
	std::unique_ptr<XMLNode> m_relationshipNode; /**< An XMLNode object with the relationship item */
};

// ================================================================================
// XLRelationships Class
// ================================================================================

/**
 * @brief An encapsulation of relationship files (.rels files) in an Excel document package.
 */
class XLRelationships : public XLXmlFile
{
public:        // ---------- Public Member Functions ---------- //
	/**
	 * @brief
	 */
	XLRelationships() = default;

	/**
	 * @brief
	 * @param xmlData
	 * @param pathTo Initialize m_path from this: the path to the relationships file origin of xmlData
	 * @note m_path is used to resolve relative relationship target paths to an absolute
	 */
	explicit XLRelationships(XLXmlData* xmlData, std::string pathTo);

	/**
	 * @brief Destructor
	 */
	~XLRelationships();

	/**
	 * @brief
	 * @param other
	 */
	XLRelationships(const XLRelationships& other) = default;

	/**
	 * @brief
	 * @param other
	 */
	XLRelationships(XLRelationships&& other) noexcept = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
	XLRelationships& operator=(const XLRelationships& other) = default;

	/**
	 * @brief
	 * @param other
	 * @return
	 */
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

	/**
	 * @brief
	 * @param relID
	 */
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

	// ---------- Protected Member Functions ---------- //
protected:

	//----------------------------------------------------------------------------------------------------------------------
	//           Private Member Variables
	//----------------------------------------------------------------------------------------------------------------------
private:
	std::string m_path; // the path - within the XLSX file - to the relationships file on which this object is instantiated
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLRELATIONSHIPS_HPP
