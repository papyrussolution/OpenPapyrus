/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2004 Yeico S. A. de C. V. All Rights Reserved.
 * Copyright 2008-2011 David Hoerl All Rights Reserved.
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

#ifndef BINFILE_H
#define BINFILE_H

namespace xlslib_core
{
	/*
	 ******************************
	 * CBinFile class declaration
	 ******************************
	 */
	 
	class CUnit;

	class CBinFile
	{
	private:
		int write_service(const char *buffer, size_t size);
	//	static const int STROKE_CHUNK_SIZE; now unused apparently

	public:
		CBinFile();
//      CBinFile(const std::string& name);
		virtual ~CBinFile();
		int Open(const std::string& name);
		int Close(void);
		int Write(uint8 * data, size_t size);
		int Write(CUnit& data_unit);
		int WriteByte(uint8 byte);
		int WriteUnsigned16(uint16 data);
		int WriteUnsigned32(uint32 data);
		int WriteSigned16(int16 data);
		int WriteSigned32(int32 data);
		int WriteByteArray(const uint8 *data, size_t size);
		int SerializeFixedArray(const uint8 data, size_t size);
		uint32 Position(); // debugging, position of stream
	protected:
		std::ofstream m_File;
	};
}

#endif
