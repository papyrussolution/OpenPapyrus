/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2009 David Hoerl All Rights Reserved.
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
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * File description:
 *
 *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HPSF_H
#define HPSF_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
#define SUMMARY_SIZE		4096
// one big blocks

#define FILETIME2UNIX_NS	11644473600000000ll
// from the web

	extern const uint32 summaryFormat[4], docSummaryFormat[4], hpsfValues[];
	
	typedef enum _docType_t {
		HPSF_SUMMARY,
		HPSF_DOCSUMMARY
	} docType_t;

	typedef enum
	{
		HPSF_STRING=0,
		HPSF_BOOL,
		HPSF_INT16,
		HPSF_INT32,
		HPSF_INT64
	} hpsf_t;
	typedef union {
		std::string		*str;
		bool isOn;
		uint16 val16;
		uint32 val32;
		uint64 val64;
	} hValue;

	class HPSFitem
	{
		friend class insertsort2;
	private:
		uint16 propID;
		hpsf_t variant;
		hValue value;
		size_t offset;

	public:
		HPSFitem(uint16 type, const std::string& str);
		HPSFitem(uint16 type, bool val);
		HPSFitem(uint16 type, uint16 val);
		HPSFitem(uint16 type, uint32 val);
		HPSFitem(uint16 type, uint64 val);
		~HPSFitem();
		hValue				GetValue() const {return value; }
		uint16		GetPropID() const {return propID; }
		uint16		GetVariant() const {return (uint16)variant; }
		void SetOffset(size_t of) { offset=of; }
		size_t GetOffset() const { return offset; }
		size_t GetSize();   // actual length rounded up to 4 bytes
		bool operator< (const HPSFitem& rhs) const { return variant < rhs.variant; }
	};

	class insertsort2 {
	public:
		bool operator()(HPSFitem* a, HPSFitem* b) const { return a->propID < b->propID; }
	};

	typedef std::set<xlslib_core::HPSFitem*, insertsort2 XLSLIB_DFLT_ALLOCATOR> HPSF_Set_t;
	typedef HPSF_Set_t::iterator HPSF_Set_Itor_t;
	typedef HPSF_Set_t::const_iterator HPSF_Set_ConstItor_t;

	class hpsf_doc_t {
		friend class CHPSFdoc;
	private:
		void insert(HPSFitem *item);

	public:
		hpsf_doc_t(docType_t dt);
		virtual ~hpsf_doc_t();

	public:
		void addItem(uint16 key, uint16 val) {insert(new HPSFitem(key, val)); }
		void addItem(uint16 key, const std::string& str) {insert(new HPSFitem(key, str)); }

		void addItem(uint16 key, bool val) {insert(new HPSFitem(key, val)); }
		void addItem(uint16 key, uint32 val) {insert(new HPSFitem(key, val)); }
		void addItem(uint16 key, uint64 val) {insert(new HPSFitem(key, val)); }

		//uint32	NumProperties() const {return numProperties;};
		uint64 unix2mstime(time_t unixTime);

	public:
		virtual size_t GetSize(void) const;
		virtual CUnit* GetData(CDataStorage &datastore) const;

	private:
		docType_t docType;
		HPSF_Set_t itemList;
		//uint32	numProperties;
	};

	class CHPSFdoc : public CUnit {
		friend class CSummaryInfo;
		friend class CDocSummaryInfo;
		friend class CDataStorage;
	protected:
		CHPSFdoc(CDataStorage &datastore, const hpsf_doc_t& docdef);
	private:
		virtual ~CHPSFdoc();
	};
}


// #include "xls_poppack.h"

#endif
