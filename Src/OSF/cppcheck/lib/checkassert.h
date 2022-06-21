/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#ifndef checkassertH
#define checkassertH

class ErrorLogger;
class Scope;
class Settings;
class Token;
class Tokenizer;

/// @addtogroup Checks
/// @{

/**
 * @brief Checking for side effects in assert statements
 */

class CPPCHECKLIB CheckAssert : public Check {
public:
	CheckAssert() : Check(myName()) {
	}

	CheckAssert(const Tokenizer * tokenizer, const Settings * settings, ErrorLogger * errorLogger)
		: Check(myName(), tokenizer, settings, errorLogger) {
	}

	/** run checks, the token list is not simplified */
	void runChecks(const Tokenizer * tokenizer, const Settings * settings, ErrorLogger * errorLogger) override {
		CheckAssert checkAssert(tokenizer, settings, errorLogger);
		checkAssert.assertWithSideEffects();
	}

	void assertWithSideEffects();

protected:
	void checkVariableAssignment(const Token* assignTok, const Scope * assertionScope);
	static bool inSameScope(const Token* returnTok, const Token* assignTok);

private:
	void sideEffectInAssertError(const Token * tok, const std::string& functionName);
	void assignmentInAssertError(const Token * tok, const std::string &varname);

	void getErrorMessages(ErrorLogger * errorLogger, const Settings * settings) const override {
		CheckAssert c(nullptr, settings, errorLogger);
		c.sideEffectInAssertError(nullptr, "function");
		c.assignmentInAssertError(nullptr, "var");
	}

	static std::string myName() {
		return "Assert";
	}

	std::string classInfo() const override {
		return
			"Warn if there are side effects in assert statements (since this cause different behaviour in debug/release builds).\n";
	}
};

/// @}
//---------------------------------------------------------------------------
#endif // checkassertH
