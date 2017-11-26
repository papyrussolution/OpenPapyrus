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

#ifndef FORMULA_EXPR_H
#define FORMULA_EXPR_H

// #include "xls_pshpack2.h"

namespace xlslib_core
{
	class worksheet;
    class estimated_formula_result_t;
    class formula_t;

	class expression_node_t
	{
	public:
		expression_node_t(CGlobalRecords& gRecords);
		virtual ~expression_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void DestroyAST(void);
		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;
	};


	class terminal_node_t : public expression_node_t
	{
	public:
		terminal_node_t(CGlobalRecords& gRecords);
		virtual ~terminal_node_t();
	};

	class boolean_value_node_t : public terminal_node_t
	{
	public:
		boolean_value_node_t(CGlobalRecords& gRecords, bool value);
		virtual ~boolean_value_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		bool value;
	};

	class integer_value_node_t : public terminal_node_t
	{
	public:
		integer_value_node_t(CGlobalRecords& gRecords, int32 value);
		virtual ~integer_value_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		int32 value;
	};

	class float_value_node_t : public terminal_node_t
	{
	public:
		float_value_node_t(CGlobalRecords& gRecords, double value);
		virtual ~float_value_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		double value;
	};

	class error_value_node_t : public terminal_node_t
	{
	public:
		error_value_node_t(CGlobalRecords& gRecords, errcode_t value);
		virtual ~error_value_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		errcode_t value;
	};

	class missing_arg_node_t : public terminal_node_t
	{
	public:
		missing_arg_node_t(CGlobalRecords& gRecords);
		virtual ~missing_arg_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;
	};

	class text_value_node_t : public terminal_node_t
	{
	public:
		text_value_node_t(CGlobalRecords& gRecords, const std::string& value);
		text_value_node_t(CGlobalRecords& gRecords, const xlslib_strings::ustring& value);
#ifndef __FRAMEWORK__
		text_value_node_t(CGlobalRecords& gRecords, const xlslib_strings::u16string& value);
#endif
		virtual ~text_value_node_t();

	private:
		/* MSVC2005: C4512: 'xlslib_core::text_value_node_t' : assignment operator could not be generated */
		text_value_node_t &operator =(const text_value_node_t &src);

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		xlslib_strings::u16string value;

		CGlobalRecords& m_GlobalRecords;

	public:
		CGlobalRecords& GetGlobalRecords(void) const { return m_GlobalRecords; }
	};

	class cell_deref_node_t : public terminal_node_t
	{
	public:
		cell_deref_node_t(CGlobalRecords& gRecords, const cell_t& value, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		cell_deref_node_t(CGlobalRecords& gRecords, const cell_t& value, const worksheet* ws, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		virtual ~cell_deref_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		uint32 row_, col_, idx_;
		const worksheet* worksheet_ref;
		cell_addr_mode_t attr;
		cell_op_class_t operand_class;
	};

	class cellarea_deref_node_t : public cell_deref_node_t
	{
	public:
		cellarea_deref_node_t(CGlobalRecords& gRecords, const cell_t& upper_left_corner, const cell_t& lower_right_corner, cell_addr_mode_t attr,
							  cell_op_class_t opclass = CELLOP_AS_VALUE);
		cellarea_deref_node_t(CGlobalRecords& gRecords, const cell_t& upper_left_corner, const cell_t& lower_right_corner, const worksheet* ws,
							  cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		virtual ~cellarea_deref_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		// parent class' [const cell_t* value] ~ upper_left_corner;
		uint32 lrrow_, lrcol_, lridx_;
	};



	class operator_basenode_t : public expression_node_t
	{
	public:
		operator_basenode_t(CGlobalRecords& gRecords, expr_operator_code_t op);
		virtual ~operator_basenode_t();

	public:
		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		expr_operator_code_t op;
	};

	class unary_op_node_t : public operator_basenode_t
	{
	public:
		unary_op_node_t(CGlobalRecords& gRecords, expr_operator_code_t op, expression_node_t* arg);
		virtual ~unary_op_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

	protected:
		expression_node_t *arg;
	};

	class binary_op_node_t : public operator_basenode_t
	{
	public:
		binary_op_node_t(CGlobalRecords& gRecords, expr_operator_code_t op, expression_node_t* arg1, expression_node_t* arg2);
		virtual ~binary_op_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

	protected:
		expression_node_t *args[2];
	};


	class function_basenode_t : public expression_node_t
	{
	public:
		function_basenode_t(CGlobalRecords& gRecords, expr_function_code_t func);
		virtual ~function_basenode_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		expr_function_code_t func;
	};

	class z_ary_func_node_t : public function_basenode_t
	{
	public:
		z_ary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op);
		virtual ~z_ary_func_node_t();

	public:
		//virtual size_t GetSize(bool include_subtree = false) const;
		//virtual int8 DumpData(CUnit &dst, bool include_subtree = false) const;
	};

	class unary_func_node_t : public function_basenode_t
	{
	public:
		unary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op, expression_node_t* arg);
		virtual ~unary_func_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		//virtual size_t GetSize(bool include_subtree = false) const;
		//virtual int8 DumpData(CUnit &dst, bool include_subtree = false) const;

	protected:
		expression_node_t *arg;
	};

	class binary_func_node_t : public function_basenode_t
	{
	public:
		binary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op, expression_node_t* arg1, expression_node_t* arg2);
		virtual ~binary_func_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		//virtual size_t GetSize(bool include_subtree = false) const;
		//virtual int8 DumpData(CUnit &dst, bool include_subtree = false) const;

	protected:
		expression_node_t *args[2];
	};

	class n_ary_func_node_t : public function_basenode_t
	{
	public:
		n_ary_func_node_t(CGlobalRecords& gRecords, expr_function_code_t op, size_t argcount = 0, expression_node_t** arg_arr = NULL);
		virtual ~n_ary_func_node_t();

	public:
		virtual expression_node_t* GetChild(uint16 index) const;
		virtual uint16 GetNumberOfChilds(void) const;

		virtual function_basenode_t& PushArg(expression_node_t* arg);

		//virtual size_t GetSize(bool include_subtree = false) const;
		//virtual int8 DumpData(CUnit &dst, bool include_subtree = false) const;

	protected:
		uint16 arg_arrsize;
		uint16 arg_count;
		expression_node_t** arg_arr;
	};

	class userdef_func_node_t : public n_ary_func_node_t
	{
	public:
		userdef_func_node_t(CGlobalRecords& gRecords, int expr_user_function, size_t argcount = 0, expression_node_t** arg_arr = NULL);
		virtual ~userdef_func_node_t();

	public:
		virtual size_t GetSize(bool include_subtree = false) const;
		virtual int8 DumpData(formula_t &stack, bool include_subtree = false) const;

		virtual void GetResultEstimate(estimated_formula_result_t &dst) const;

	protected:
		int expr_user_function_code;
	};


	class expression_node_factory_t
	{
		friend class workbook;

	private:
		expression_node_factory_t(CGlobalRecords& gRecords);
		virtual ~expression_node_factory_t();
		/* MSVC2005: C4512: 'xlslib_core::expression_node_factory_t' : assignment operator could not be generated */
		expression_node_factory_t &operator =(const expression_node_factory_t &src);

	public:
		boolean_value_node_t *boolean(bool value);
		integer_value_node_t *integer(int32 value);
		float_value_node_t *floating_point(double value);
		error_value_node_t *error_value(errcode_t value);
		missing_arg_node_t *missing_arg(void);
		text_value_node_t *text(const std::string& value);
		text_value_node_t *text(const xlslib_strings::u16string& value);
		
		cell_deref_node_t *cell(const cell_t& value, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		cell_deref_node_t *cell(const cell_t& value, const worksheet* ws, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		cellarea_deref_node_t *area(const cell_t& upper_left_corner, const cell_t& lower_right_corner, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		cellarea_deref_node_t *area(const cell_t& upper_left_corner, const cell_t& lower_right_corner, const worksheet* ws, cell_addr_mode_t attr, cell_op_class_t opclass = CELLOP_AS_VALUE);
		unary_op_node_t *op(expr_operator_code_t op, expression_node_t* arg);
		binary_op_node_t *op(expr_operator_code_t op, expression_node_t* arg1, expression_node_t* arg2);
		z_ary_func_node_t *f(expr_function_code_t func);
		unary_func_node_t *f(expr_function_code_t func, expression_node_t* arg);
		binary_func_node_t *f(expr_function_code_t func, expression_node_t* arg1, expression_node_t* arg2);
		n_ary_func_node_t *f(expr_function_code_t func, size_t argcount, expression_node_t** arg_arr = NULL);
		userdef_func_node_t *udf(int expr_user_function, size_t argcount = 0, expression_node_t** arg_arr = NULL);

	protected:
		CGlobalRecords& m_GlobalRecords;
	};
}

#endif
