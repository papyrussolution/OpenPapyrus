/*
 * Copyright 2016 Huy Cuong Nguyen
 * Copyright 2016 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop

namespace ZXing {
	ReedSolomonEncoder::ReedSolomonEncoder(const GenericGF& field) : _field(&field)
	{
		_cachedGenerators.push_back(GenericGFPoly(field, { 1 }));
	}

	const GenericGFPoly&ReedSolomonEncoder::buildGenerator(int degree)
	{
		int cachedGenSize = Size(_cachedGenerators);
		if(degree >= cachedGenSize) {
			GenericGFPoly lastGenerator = _cachedGenerators.back();
			for(int d = cachedGenSize; d <= degree; d++) {
				lastGenerator.multiply(GenericGFPoly(*_field, { 1, _field->exp(d - 1 + _field->generatorBase()) }));
				_cachedGenerators.push_back(lastGenerator);
			}
		}
		return *std::next(_cachedGenerators.begin(), degree);
	}

	void ReedSolomonEncoder::encode(std::vector<int>& message, const int numECCodeWords)
	{
		if(numECCodeWords == 0 || numECCodeWords >= Size(message))
			throw std::invalid_argument("Invalid number of error correction code words");
		GenericGFPoly info = GenericGFPoly(*_field, std::vector<int>(message.begin(), message.end() - numECCodeWords));
		info.multiplyByMonomial(1, numECCodeWords);
		GenericGFPoly _;
		info.divide(buildGenerator(numECCodeWords), _);
		auto & coefficients = info.coefficients();
		int numZeroCoefficients = numECCodeWords - Size(coefficients);
		std::fill_n(message.end() - numECCodeWords, numZeroCoefficients, 0);
		std::copy(coefficients.begin(), coefficients.end(), message.end() - numECCodeWords + numZeroCoefficients);
	}
	//
	//
	//
	static bool RunEuclideanAlgorithm(const GenericGF& field, std::vector<int>&& rCoefs, GenericGFPoly& sigma, GenericGFPoly& omega)
	{
		int R = Size(rCoefs); // == numECCodeWords
		GenericGFPoly r(field, std::move(rCoefs));
		GenericGFPoly& tLast = omega.setField(field);
		GenericGFPoly& t = sigma.setField(field);
		ZX_THREAD_LOCAL GenericGFPoly q, rLast;
		rLast.setField(field);
		q.setField(field);
		rLast.setMonomial(1, R);
		tLast.setMonomial(0);
		t.setMonomial(1);
		// Assume r's degree is < rLast's
		if(r.degree() >= rLast.degree())
			swap(r, rLast);
		// Run Euclidean algorithm until r's degree is less than R/2
		while(r.degree() >= R / 2) {
			swap(tLast, t);
			swap(rLast, r);
			// Divide rLastLast by rLast, with quotient in q and remainder in r
			if(rLast.isZero())
				return false; // Oops, Euclidean algorithm already terminated?
			r.divide(rLast, q);
			q.multiply(tLast);
			q.addOrSubtract(t);
			swap(t, q); // t = q
			if(r.degree() >= rLast.degree())
				throw std::runtime_error("Division algorithm failed to reduce polynomial?");
		}
		int sigmaTildeAtZero = t.constant();
		if(sigmaTildeAtZero == 0)
			return false;
		int inverse = field.inverse(sigmaTildeAtZero);
		t.multiplyByMonomial(inverse);
		r.multiplyByMonomial(inverse);
		// sigma is t
		omega = std::move(r);
		return true;
	}

	static std::vector<int>FindErrorLocations(const GenericGF& field, const GenericGFPoly& errorLocator)
	{
		// This is a direct application of Chien's search
		int numErrors = errorLocator.degree();
		std::vector<int> res;
		res.reserve(numErrors);
		for(int i = 1; i < field.size() && Size(res) < numErrors; i++)
			if(errorLocator.evaluateAt(i) == 0)
				res.push_back(field.inverse(i));
		if(Size(res) != numErrors)
			return {}; // Error locator degree does not match number of roots
		return res;
	}

	static std::vector<int> FindErrorMagnitudes(const GenericGF& field, const GenericGFPoly& errorEvaluator, const std::vector<int>& errorLocations)
	{
		// This is directly applying Forney's Formula
		int s = Size(errorLocations);
		std::vector<int> res(s);
		for(int i = 0; i < s; ++i) {
			int xiInverse = field.inverse(errorLocations[i]);
			int denom = 1;
			for(int j = 0; j < s; ++j)
				if(i != j)
					denom = field.multiply(denom, 1 ^ field.multiply(errorLocations[j], xiInverse));
			res[i] = field.multiply(errorEvaluator.evaluateAt(xiInverse), field.inverse(denom));
			if(field.generatorBase() != 0)
				res[i] = field.multiply(res[i], xiInverse);
		}
		return res;
	}

	bool ReedSolomonDecode(const GenericGF& field, std::vector<int>& message, int numECCodeWords)
	{
		GenericGFPoly poly(field, message);
		std::vector<int> syndromes(numECCodeWords);
		for(int i = 0; i < numECCodeWords; i++)
			syndromes[numECCodeWords - 1 - i] = poly.evaluateAt(field.exp(i + field.generatorBase()));
		// if all syndromes are 0 there is no error to correct
		if(std::all_of(syndromes.begin(), syndromes.end(), [](int c) { return c == 0; }))
			return true;
		ZX_THREAD_LOCAL GenericGFPoly sigma, omega;
		if(!RunEuclideanAlgorithm(field, std::move(syndromes), sigma, omega))
			return false;
		auto errorLocations = FindErrorLocations(field, sigma);
		if(errorLocations.empty())
			return false;
		auto errorMagnitudes = FindErrorMagnitudes(field, omega, errorLocations);
		int msgLen = Size(message);
		for(int i = 0; i < Size(errorLocations); ++i) {
			int position = msgLen - 1 - field.log(errorLocations[i]);
			if(position < 0)
				return false;

			message[position] ^= errorMagnitudes[i];
		}
		return true;
	}
} // ZXing
