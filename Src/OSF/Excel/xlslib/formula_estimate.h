/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * This file is part of xlslib -- A multiplatform, C/C++ library
 * for dynamic generation of Excel(TM) files.
 *
 * Copyright 2010-2013 Ger Hobbelt All Rights Reserved.
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

#ifndef FORMULA_ESTIMATE_H
#define FORMULA_ESTIMATE_H

namespace xlslib_core
{

	class estimated_formula_result_t
	{
	public:
		estimated_formula_result_t(CGlobalRecords& gRecords);
		virtual ~estimated_formula_result_t();

		void SetCalcOnLoad(void);
		void SetCalcAlways(void);

		uint16 GetOptionFlags(void) const;

		bool SetBoolean(bool value);
		int32 SetInteger(int32 value);
		double SetFloatingPoint(double value);
		const xlslib_strings::u16string& SetText(const std::string& value);
		const xlslib_strings::u16string& SetText(const xlslib_strings::ustring& value);
#ifndef __FRAMEWORK__
		const xlslib_strings::u16string& SetText(const xlslib_strings::u16string& value);
#endif
		errcode_t SetErrorCode(errcode_t value);

		uint64 GetEncodedValue(void) const;
		bool EncodedValueIsString(void) const;
		const xlslib_strings::u16string* GetStringValue(void) const;

	protected:
		union
		{
			bool b;
			int32 i;
			double f;
			xlslib_strings::u16string *s;
			errcode_t e;
		} value;
		enum estval_type_t
		{
			ESTVAL_UNKNOWN = 0,
			ESTVAL_BOOLEAN,
			ESTVAL_INTEGER,
			ESTVAL_FLOATINGPOINT,
			ESTVAL_STRING,
			ESTVAL_ERRORCODE,
		} value_type;

		unsigned always_calc : 1;
		unsigned calc_on_load : 1;

		CGlobalRecords& m_GlobalRecords;

		/* MSVC2005: C4512: 'xlslib_core::estimated_formula_result_t' : assignment operator could not be generated */
		estimated_formula_result_t &operator =(const estimated_formula_result_t &src);

		void clear_value(estval_type_t incoming_type);
	};

}

#endif
