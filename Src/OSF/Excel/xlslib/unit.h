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

#ifndef UNIT_H
#define UNIT_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
#define UNIT_MAX_SIZE  0xFFFF

	// Error codes
#define ERR_DATASTORAGE_EMPTY            (-2)
#define ERR_INVALID_INDEX                (-3)
#define ERR_UNABLE_TOALLOCATE_MEMORY     (-4) // [i_a]

	//Block definitions
#define BIG_BLOCK_SIZE     0x200
#define SMALL_BLOCK_SIZE   0x040
#define PROP_BLOCK_SIZE    0x080

	class CUnit {
		// Attributes
	protected:

#define INVALID_STORE_INDEX			((int32)0x80000000) // marks a 'not yet set up' unit store

		friend class CDataStorage;

		CDataStorage &m_Store;
		int32 m_Index; // positive numbers index space in the associated storage; negative numbers are that index, but the data has been marked as 'sticky' in the associated storage

		uint16 m_Backpatching_Level; // 0: requires no backpatching, 1: innermost backpatching (DBCELL), ...

		// Static attributes
		static const size_t DefaultInflateSize;
		// needed for SST labels (maybe others in the future)
		void SetAlreadyContinued(bool val) { m_AlreadyContinued = val; }
		// Operations
	protected: // deny these operations to others...
		CUnit(CDataStorage &datastore);
	private: // deny these operations to others...
		bool m_AlreadyContinued;
		CUnit(const CUnit& orig);
		CUnit& operator=(const CUnit& right);
	public:
		// primarily for Continue Records
		CDataStorage & DataStore() { return m_Store; }
		uint16 BackPatchingLevel() const { return m_Backpatching_Level; }
		virtual ~CUnit();
		uint8& operator[](const size_t index) const;
		CUnit&   operator+=(const CUnit& from);
		CUnit&   operator+=(uint8 from);
	protected:
		void ResetDataStorage(void);
	public:
		size_t GetSize(void) const;
		size_t GetDataSize(void) const;
		const uint8* GetBuffer(void) const;
		bool AlreadyContinued() const { return m_AlreadyContinued; }
//  protected:
//    uint8* GetBuffer(void);

	protected:
		int8 Init(const uint8 * data, const size_t size, const uint32 datasz);
	public:
		int8 AddDataArray(const uint8* newdata, size_t size);
		int8 AddFixedDataArray(const uint8 value, size_t size);

		enum XlsUnicodeStringFormat_t {
			LEN2_FLAGS_UNICODE=1,			// RECTYPE_FORMAT, RECTYPE_LABEL
			LEN2_NOFLAGS_PADDING_UNICODE,	// RECTYPE_NOTE (RECTYPE_TXO)
			LEN1_FLAGS_UNICODE,				// RECTYPE_BOUNDSHEET
			NOLEN_FLAGS_UNICODE,			// RECTYPE_NAME
		};

		int8 AddUnicodeString(CGlobalRecords& gRecords, const std::string& str, XlsUnicodeStringFormat_t fmt /* = LEN2_FLAGS_UNICODE */ );
		int8 AddUnicodeString(const xlslib_strings::u16string& newdata, XlsUnicodeStringFormat_t fmt /* = LEN2_FLAGS_UNICODE */ );
		size_t UnicodeStringLength(const xlslib_strings::u16string& str16, size_t& strLen, bool& isAscii, XlsUnicodeStringFormat_t fmt /* = LEN2_FLAGS_UNICODE */ );
		int8 GetValue16From(uint16 * val, uint32 index) const;
		int8 GetValue32From(uint32* val, uint32 index) const;
		int8 GetValue8From(uint8* data, uint32 index) const;
		int8 Append(const CUnit& newunit);
		int8 AddValue8(uint8 newdata);
		int8 AddValue16(uint16 newval);
		int8 AddValue32(uint32 newval);
		int8 AddValue64(uint64 newval);
		int8 AddValue64FP(double newval);
		int8 SetValueAt8(uint8 newval, uint32 index);                      // Modify specific position
		int8 SetValueAt16(uint16 newval, uint32 index);
		int8 SetValueAt32(uint32 newval, uint32 index);
		int8 SetArrayAt(const uint8* newdata, size_t size, uint32 index);
		// int8 GetData(uint8** data, uint32 from, uint32 to );
		static uint64 EncodeFP2I64(double newval);
	public:
		int8 Inflate(size_t newsize);
		int32 GetIndex() const { return m_Index; }
	protected:
		int8 InitFill(uint8 data, size_t size);
	};
}

// #include "xls_poppack.h"

#endif
