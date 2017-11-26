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

#ifndef FORMULAST_H
#define FORMULAST_H

namespace xlslib_core
{
	/*
	 *  @return a bit for each number of arguments supported by this function.
	 *
	 *  @note
	 *  Bit 0 (0x0001U) indicates whether the function accepts @e zero (0) arguments.
	 *
	 *  Bit 1..14 indicate whether the function accepts 1..14 arguments.
	 *
	 *  Bit 15 (0x8000U) indicates the function accepts more than 14 arguments.
	 */
	uint16 NumberOfArgsForExcelFunction(expr_function_code_t func);


    class formula_t
    {
		friend class cell_deref_node_t;
		friend class cellarea_deref_node_t;

    public:
        formula_t(CGlobalRecords& glbl, worksheet* ws);
        virtual ~formula_t();

        void DumpData(CUnit& dst) const;
        size_t GetSize() const;
        void GetResultEstimate(estimated_formula_result_t &dst) const;

        int8 PushBoolean(bool value);
        int8 PushMissingArgument();
        int8 PushError(uint8 value);
        int8 PushInteger(int32 value);
        int8 PushFloatingPoint(double value);
        int8 PushOperator(expr_operator_code_t op);
        int8 PushCellReference(const cell_t& cell, cell_addr_mode_t opt);
        int8 PushCellAreaReference(const cell_t& upper_left_cell, const cell_t& lower_right_cell, cell_addr_mode_t opt);
        int8 PushFunction(expr_function_code_t func);
        int8 PushFunction(expr_function_code_t func, size_t argcount);
        int8 PushText(const std::string& v);
        int8 PushText(const xlslib_strings::ustring& v);
#if !defined(__FRAMEWORK__)
        int8 PushText(const xlslib_strings::u16string& value);
#endif
        int8 PushTextArray(const std::vector<std::string>& vec);
        int8 PushTextArray(const std::vector<xlslib_strings::ustring>& vec);
        int8 PushFloatingPointArray(const std::vector<double>& vec);

    protected:
        CDataStorage *data_storage;
		CGlobalRecords& m_GlobalRecords;
        worksheet *m_Worksheet;

        CUnit *main_data;
        CUnit *aux_data;

	private:
		int8 PushReference(uint32 row, uint32 col, uint32 idx, cell_addr_mode_t opt);
        int8 PushAreaReference(uint32 ul_row, uint32 ul_col, uint32 ul_idx, uint32 lr_row, uint32 lr_col, uint32 lr_idx, cell_addr_mode_t opt);
    };
}

#endif
