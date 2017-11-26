/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010 Ger Hobbelt All Rights Reserved.
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

#ifndef XL_ERR_H
#define XL_ERR_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	typedef enum {
		XLERR_NULL  = 0x00, // #NULL!
		XLERR_DIV0  = 0x07, // #DIV/0!
		XLERR_VALUE = 0x0F, // #VALUE!
		XLERR_REF   = 0x17, // #REF!
		XLERR_NAME  = 0x1D, // #NAME?
		XLERR_NUM   = 0x24, // #NUM!
		XLERR_N_A   = 0x2A, // #N/A!
		// since Excel 2010:
		XLERR_GETTINGDATA = 0x2B, // #DATA!
	} errcode_t;

	class err_t : public cell_t {
		friend class worksheet;
	private:
		err_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval, errcode_t value, xf_t* pxfval = NULL);
		virtual ~err_t(){}
	public:
		virtual size_t GetSize(void) const {return 12; }
		virtual CUnit* GetData(CDataStorage &datastore) const;
	private:
		errcode_t ecode;
	public:
		uint8 GetErr(void) const {return ecode; }
	};

	class CErr : public CRecord {
		friend class CDataStorage;
	protected:
		CErr(CDataStorage &datastore, const err_t& errdef);
	private:
		virtual ~CErr();
	};
}

// #include "xls_poppack.h"

#endif
