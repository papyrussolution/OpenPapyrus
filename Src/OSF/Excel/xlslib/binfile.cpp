/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* This file is part of xlslib -- A multiplatform, C/C++ library
* for dynamic generation of Excel(TM) files.
*
* Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
* Copyright 2008-2013 David Hoerl All Rights Reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are
* permitted provided that the following conditions are met:
*
*    1. Redistributions of source code must retain the above copyright notice, this list of
*       conditions and the following disclaimer.
*
*    2. Redistributions in binary form must reproduce the above copyright notice, this list
*       of conditions and the following disclaimer in the documentation and/or other materials
*       provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY David Hoerl ''AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
* FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL David Hoerl OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "xlslib-internal.h"
#pragma hdrstop

using namespace xlslib_core;
using namespace xlslib_strings;

/*
 ******************************
 * CBinFile class Implementation
 ******************************
 */
CBinFile::CBinFile() : m_File()
{
}

CBinFile::~CBinFile()
{
	Close();
}

int CBinFile::Open(const string& file_name)
{
	Close();
	m_File.open(file_name.c_str(), std::ios_base::binary|std::ios_base::out);
	return m_File.good() ? NO_ERRORS : FILE_ERROR;
}

int CBinFile::Close(void)
{
	if(m_File.is_open()) {
		m_File.close();
	}
	return NO_ERRORS;
}

uint32 CBinFile::Position(void)
{
	uint32 pt = 0;
	if(m_File.is_open()) {
		pt = (uint32)m_File.tellp();
	}
	return pt;
}

int CBinFile::Write(uint8 * data, size_t size)
{
	int errcode = write_service((const char*)data, size);
	return errcode;
}

int CBinFile::Write(CUnit& data_unit)
{
	int errcode = write_service((const char*)data_unit.GetBuffer(), data_unit.GetDataSize());
	return errcode;
}

int CBinFile::WriteByte(uint8 byte)
{
	int errcode = write_service((const char*)&byte, 1);
	return errcode;
}

int CBinFile::WriteUnsigned16(uint16 data)
{
	int errcode = WriteByte(BYTE_0(data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_1(data));
	return errcode;
}

int CBinFile::WriteUnsigned32(uint32 data)
{
	int errcode = WriteByte(BYTE_0(data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_1(data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_2(data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_3(data));
	return errcode;
}

int CBinFile::WriteSigned16(int16 data)
{
	int errcode = WriteByte(BYTE_0((uint32)data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_1((uint32)data));
	return errcode;
}

int CBinFile::WriteSigned32(int32 data)
{
	int errcode = WriteByte(BYTE_0((uint32)data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_1((uint32)data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_2((uint32)data));
	if(errcode != NO_ERRORS) {
		return errcode;
	}
	errcode = WriteByte(BYTE_3((uint32)data));
	return errcode;
}

int CBinFile::WriteByteArray(const uint8 * data, size_t size)
{
	int errcode = write_service((const char*)data, size);
	return errcode;
}

int CBinFile::SerializeFixedArray(const uint8 data, size_t size)
{
	int errcode = NO_ERRORS;
	for(size_t i = 0; i<size; i++) {
		errcode = WriteByte(data);
		if(errcode != NO_ERRORS) {
			return errcode;
		}
	}
	return errcode;
}

int CBinFile::write_service(const char * buffer, size_t size)
{
	if(m_File.is_open()) {
		if(size > 1) {
			m_File.write(buffer, static_cast<std::streamsize>(size));
		}
		else if(size == 1)        {
			m_File.put(*buffer);
		}
	}
	return m_File.good() ? NO_ERRORS : FILE_ERROR;
}
