/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#ifndef FORMULA_H
#define FORMULA_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	class cell_t;
    class estimated_formula_result_t;
    class formula_t;
    class expression_node_t;

	class formula_cell_t : public cell_t
	{
	public:
		formula_cell_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval,
				  expression_node_t* ast, bool auto_destruct_expression_tree = false,
				  xf_t* pxfval = NULL);
		formula_cell_t(CGlobalRecords& gRecords, uint32 rowval, uint32 colval,
                formula_t* stack, xf_t* pxfval = NULL);
		virtual ~formula_cell_t();

    private:
		virtual size_t GetSize(void) const;
		virtual CUnit* GetData(CDataStorage &datastore) const;
		const expression_node_t *GetAST() const {return ast; }

	private:
		expression_node_t *ast;
		bool auto_destruct_expression_tree;

        formula_t *stack;

	public:
        void DumpData(CUnit &dst) const;
		void GetResultEstimate(estimated_formula_result_t &dst) const;
	};

	class CFormula : public CRecord {
		friend class CDataStorage;
	protected:
		CFormula(CDataStorage &datastore, const formula_cell_t& ast);
	private:
		virtual ~CFormula();
	};
}

// #include "xls_poppack.h"

#endif
