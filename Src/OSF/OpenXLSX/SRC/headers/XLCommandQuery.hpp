// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLCOMMANDQUERY_HPP
#define OPENXLSX_XLCOMMANDQUERY_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

// ===== External Includes ===== //
#include <any>
#include <cstdint> // uint8_t
#include <map>
#include <string>

namespace OpenXLSX
{
/**
 * @brief
 */
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
	explicit XLCommand(XLCommandType type) : m_type(type) {}

	/**
	 * @brief
	 * @tparam T
	 * @param param
	 * @param value
	 * @return
	 */
	template <typename T>
	XLCommand& setParam(const std::string& param, T value)
	{
		m_params[param] = value;
		return *this;
	}

	/**
	 * @brief
	 * @tparam T
	 * @param param
	 * @return
	 */
	template <typename T>
	T getParam(const std::string& param) const
	{
		return std::any_cast<T>(m_params.at(param));
	}

	/**
	 * @brief
	 * @return
	 */
	XLCommandType type() const { return m_type; }

private:
	XLCommandType m_type;                     /*< */
	std::map<std::string, std::any> m_params; /*< */
};

/**
 * @brief
 */
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

/**
 * @brief
 */
class XLQuery
{
public:
	/**
	 * @brief
	 * @param type
	 */
	explicit XLQuery(XLQueryType type) : m_type(type) {}

	/**
	 * @brief
	 * @tparam T
	 * @param param
	 * @param value
	 * @return
	 */
	template <typename T>
	XLQuery& setParam(const std::string& param, T value)
	{
		m_params[param] = value;
		return *this;
	}

	/**
	 * @brief
	 * @tparam T
	 * @param param
	 * @return
	 */
	template <typename T>
	T getParam(const std::string& param) const
	{
		return std::any_cast<T>(m_params.at(param));
	}

	/**
	 * @brief
	 * @tparam T
	 * @param value
	 * @return
	 */
	template <typename T>
	XLQuery& setResult(T value)
	{
		m_result = value;
		return *this;
	}

	/**
	 * @brief
	 * @tparam T
	 * @return
	 */
	template <typename T>
	T result() const
	{
		return std::any_cast<T>(m_result);
	}

	/**
	 * @brief
	 * @return
	 */
	XLQueryType type() const { return m_type; }

private:
	XLQueryType m_type;                       /*< */
	std::any m_result;                        /*< */
	std::map<std::string, std::any> m_params; /*< */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLCOMMANDQUERY_HPP
