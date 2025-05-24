// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#ifndef OPENXLSX_XLZIPARCHIVE_HPP
#define OPENXLSX_XLZIPARCHIVE_HPP

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(push)
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#endif // _MSC_VER

namespace Zippy {
class ZipArchive;
}    // namespace Zippy

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
	std::shared_ptr<Zippy::ZipArchive> m_archive; /**< */
};
}    // namespace OpenXLSX

#ifdef _MSC_VER    // conditionally enable MSVC specific pragmas to avoid other compilers warning about unknown pragmas
#pragma warning(pop)
#endif // _MSC_VER

#endif    // OPENXLSX_XLZIPARCHIVE_HPP
