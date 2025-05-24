// OpenXLSX
// Copyright (c) 2018, Kenneth Troldal Balslev All rights reserved.
//
#include <OpenXLSX-internal.hpp>
#pragma hdrstop
#include <zippy.hpp>
#include "XLZipArchive.hpp"

using namespace OpenXLSX;

XLZipArchive::XLZipArchive() : m_archive(nullptr) 
{
}

XLZipArchive::~XLZipArchive() = default;

XLZipArchive::operator bool() const { return isValid(); }
bool XLZipArchive::isValid() const { return m_archive != nullptr; }
bool XLZipArchive::isOpen() const { return m_archive && m_archive->IsOpen(); }

void XLZipArchive::open(const std::string& fileName)
{
	m_archive = std::make_shared<Zippy::ZipArchive>();
	try {
		m_archive->Open(fileName);
	}
	catch(...) {  // catch all exceptions
		m_archive.reset(); // make m_archive invalid again
		throw;        // re-throw
	}
}

void XLZipArchive::close()
{
	m_archive->Close();
	m_archive.reset();
}

void XLZipArchive::save(const std::string& path) // NOLINT
{
	m_archive->Save(path);
}

void XLZipArchive::addEntry(const std::string& name, const std::string& data) // NOLINT
{
	m_archive->AddEntry(name, data);
}

void XLZipArchive::deleteEntry(const std::string& entryName) // NOLINT
{
	m_archive->DeleteEntry(entryName);
}

std::string XLZipArchive::getEntry(const std::string& name) const { return m_archive->GetEntry(name).GetDataAsString(); }
bool XLZipArchive::hasEntry(const std::string& entryName) const { return m_archive->HasEntry(entryName); }
