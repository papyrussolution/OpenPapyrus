/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef inferH
#define inferH

//#include "cppcheck-config.h"
#include "mathlib.h"
#include "valueflow.h"

struct Interval;

template <class T> class ValuePtr;

struct InferModel {
	virtual bool match(const ValueFlow::Value& value) const = 0;
	virtual ValueFlow::Value yield(MathLib::bigint value) const = 0;
	virtual ~InferModel() 
	{
	}
};

std::vector<ValueFlow::Value> infer(const ValuePtr<InferModel>& model,
    const std::string& op,
    std::list<ValueFlow::Value> lhsValues,
    std::list<ValueFlow::Value> rhsValues);

std::vector<ValueFlow::Value> infer(const ValuePtr<InferModel>& model,
    const std::string& op,
    MathLib::bigint lhs,
    std::list<ValueFlow::Value> rhsValues);

std::vector<ValueFlow::Value> infer(const ValuePtr<InferModel>& model,
    const std::string& op,
    std::list<ValueFlow::Value> lhsValues,
    MathLib::bigint rhs);

CPPCHECKLIB std::vector<MathLib::bigint> getMinValue(const ValuePtr<InferModel>& model, const std::list<ValueFlow::Value>& values);
std::vector<MathLib::bigint> getMaxValue(const ValuePtr<InferModel>& model, const std::list<ValueFlow::Value>& values);

std::string toString(const Interval& i);

#endif
