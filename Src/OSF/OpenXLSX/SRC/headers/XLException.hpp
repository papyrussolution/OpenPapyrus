// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLEXCEPTION_HPP
#define OPENXLSX_XLEXCEPTION_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

#include <stdexcept>    // std::runtime_error
#include <string>       // std::string - Issue #278 should be resolved by this

namespace OpenXLSX {
	class XLException : public std::runtime_error {
	public:
		explicit XLException(const std::string& err) : runtime_error(err) {};
	};

	class XLOverflowError : public XLException {
	public:
		explicit XLOverflowError(const std::string& err) : XLException(err) {};
	};

	class XLValueTypeError : public XLException {
	public:
		explicit XLValueTypeError(const std::string& err) : XLException(err) {};
	};

	class XLCellAddressError : public XLException {
	public:
		explicit XLCellAddressError(const std::string& err) : XLException(err) {};
	};

	class XLInputError : public XLException {
	public:
		explicit XLInputError(const std::string& err) : XLException(err) {};
	};

	class XLInternalError : public XLException {
	public:
		explicit XLInternalError(const std::string& err) : XLException(err) {};
	};

	class XLPropertyError : public XLException {
	public:
		explicit XLPropertyError(const std::string& err) : XLException(err) {};
	};

	class XLSheetError : public XLException {
	public:
		explicit XLSheetError(const std::string& err) : XLException(err) {};
	};

	class XLDateTimeError : public XLException {
	public:
		explicit XLDateTimeError(const std::string& err) : XLException(err) {};
	};

	class XLFormulaError : public XLException {
	public:
		explicit XLFormulaError(const std::string& err) : XLException(err) {};
	};
}

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLEXCEPTION_HPP
