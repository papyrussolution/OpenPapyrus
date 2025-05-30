// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/*
 *   Copyright (C) 1999-2011, International Business Machines Corporation and others.  All Rights Reserved.
 *   Date        Name        Description
 *   11/17/99    aliu        Creation.
 **********************************************************************
 */
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_TRANSLITERATION

#include "rbt_set.h"
#include "rbt_rule.h"

U_CDECL_BEGIN
static void U_CALLCONV _deleteRule(void * rule) 
{
	delete (icu::TransliterationRule*)rule;
}

U_CDECL_END

//----------------------------------------------------------------------
// BEGIN Debugging support
//----------------------------------------------------------------------

// #define DEBUG_RBT

#ifdef DEBUG_RBT
/**
 * @param appendTo result is appended to this param.
 * @param input the string being transliterated
 * @param pos the index struct
 */
static UnicodeString & _formatInput(UnicodeString & appendTo,
    const UnicodeString & input,
    const UTransPosition& pos) {
	// Output a string of the form aaa{bbb|ccc|ddd}eee, where
	// the {} indicate the context start and limit, and the ||
	// indicate the start and limit.
	if(0 <= pos.contextStart &&
	    pos.contextStart <= pos.start &&
	    pos.start <= pos.limit &&
	    pos.limit <= pos.contextLimit &&
	    pos.contextLimit <= input.length()) {
		UnicodeString a, b, c, d, e;
		input.extractBetween(0, pos.contextStart, a);
		input.extractBetween(pos.contextStart, pos.start, b);
		input.extractBetween(pos.start, pos.limit, c);
		input.extractBetween(pos.limit, pos.contextLimit, d);
		input.extractBetween(pos.contextLimit, input.length(), e);
		appendTo.append(a).append((char16_t)123 /*{*/).append(b).
		append((char16_t)124 /*|*/).append(c).append((char16_t)124 /*|*/).append(d).
		append((char16_t)125 /*}*/).append(e);
	}
	else {
		appendTo.append("INVALID UTransPosition");
		//appendTo.append((UnicodeString)"INVALID UTransPosition {cs=" +
		//                pos.contextStart + ", s=" + pos.start + ", l=" +
		//                pos.limit + ", cl=" + pos.contextLimit + "} on " +
		//                input);
	}
	return appendTo;
}

// Append a hex string to the target
UnicodeString & _appendHex(uint32_t number,
    int32_t digits,
    UnicodeString & target) {
	static const char16_t digitString[] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
		0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0
	};
	while(digits--) {
		target += digitString[(number >> (digits*4)) & 0xF];
	}
	return target;
}

// Replace nonprintable characters with unicode escapes
UnicodeString & _escape(const UnicodeString & source,
    UnicodeString & target) {
	for(int32_t i = 0; i < source.length();) {
		UChar32 ch = source.char32At(i);
		i += U16_LENGTH(ch);
		if(ch < 0x09 || (ch > 0x0A && ch < 0x20)|| ch > 0x7E) {
			if(ch <= 0xFFFF) {
				target += "\\u";
				_appendHex(ch, 4, target);
			}
			else {
				target += "\\U";
				_appendHex(ch, 8, target);
			}
		}
		else {
			target += ch;
		}
	}
	return target;
}

inline void _debugOut(const char * msg, TransliterationRule* rule,
    const Replaceable& theText, UTransPosition& pos) {
	UnicodeString buf(msg, "");
	if(rule) {
		UnicodeString r;
		rule->toRule(r, TRUE);
		buf.append((char16_t)32).append(r);
	}
	buf.append(UnicodeString(" => ", ""));
	UnicodeString * text = (UnicodeString *)&theText;
	_formatInput(buf, *text, pos);
	UnicodeString esc;
	_escape(buf, esc);
	CharString cbuf(esc);
	printf("%s\n", (const char *)cbuf);
}

#else
#define _debugOut(msg, rule, theText, pos)
#endif

//----------------------------------------------------------------------
// END Debugging support
//----------------------------------------------------------------------

// Fill the precontext and postcontext with the patterns of the rules
// that are masking one another.
static void maskingError(const icu::TransliterationRule& rule1,
    const icu::TransliterationRule& rule2,
    UParseError& parseError) {
	icu::UnicodeString r;
	int32_t len;

	parseError.line = parseError.offset = -1;

	// for pre-context
	rule1.toRule(r, FALSE);
	len = smin(r.length(), U_PARSE_CONTEXT_LEN-1);
	r.extract(0, len, parseError.preContext);
	parseError.preContext[len] = 0;

	//for post-context
	r.truncate(0);
	rule2.toRule(r, FALSE);
	len = smin(r.length(), U_PARSE_CONTEXT_LEN-1);
	r.extract(0, len, parseError.postContext);
	parseError.postContext[len] = 0;
}

U_NAMESPACE_BEGIN

/**
 * Construct a new empty rule set.
 */
TransliterationRuleSet::TransliterationRuleSet(UErrorCode & status) : UMemory() {
	ruleVector = new UVector(&_deleteRule, NULL, status);
	if(U_FAILURE(status)) {
		return;
	}
	if(ruleVector == NULL) {
		status = U_MEMORY_ALLOCATION_ERROR;
	}
	rules = NULL;
	maxContextLength = 0;
}

/**
 * Copy constructor.
 */
TransliterationRuleSet::TransliterationRuleSet(const TransliterationRuleSet& other) :
	UMemory(other),
	ruleVector(0),
	rules(0),
	maxContextLength(other.maxContextLength) {
	int32_t i, len;
	memcpy(index, other.index, sizeof(index));
	UErrorCode status = U_ZERO_ERROR;
	ruleVector = new UVector(&_deleteRule, NULL, status);
	if(other.ruleVector != 0 && ruleVector != 0 && U_SUCCESS(status)) {
		len = other.ruleVector->size();
		for(i = 0; i < len && U_SUCCESS(status); ++i) {
			TransliterationRule * tempTranslitRule =
			    new TransliterationRule(*(TransliterationRule*)other.ruleVector->elementAt(i));
			// Null pointer test
			if(tempTranslitRule == NULL) {
				status = U_MEMORY_ALLOCATION_ERROR;
				break;
			}
			ruleVector->addElementX(tempTranslitRule, status);
			if(U_FAILURE(status)) {
				break;
			}
		}
	}
	if(other.rules != 0 && U_SUCCESS(status)) {
		UParseError p;
		freeze(p, status);
	}
}

/**
 * Destructor.
 */
TransliterationRuleSet::~TransliterationRuleSet() {
	delete ruleVector; // This deletes the contained rules
	uprv_free(rules);
}

void TransliterationRuleSet::setData(const TransliterationRuleData* d) {
	/**
	 * We assume that the ruleset has already been frozen.
	 */
	int32_t len = index[256]; // see freeze()
	for(int32_t i = 0; i < len; ++i) {
		rules[i]->setData(d);
	}
}

/**
 * Return the maximum context length.
 * @return the length of the longest preceding context.
 */
int32_t TransliterationRuleSet::getMaximumContextLength() const {
	return maxContextLength;
}

/**
 * Add a rule to this set.  Rules are added in order, and order is
 * significant.  The last call to this method must be followed by
 * a call to <code>freeze()</code> before the rule set is used.
 *
 * <p>If freeze() has already been called, calling addRule()
 * unfreezes the rules, and freeze() must be called again.
 *
 * @param adoptedRule the rule to add
 */
void TransliterationRuleSet::addRule(TransliterationRule* adoptedRule, UErrorCode & status) 
{
	if(U_FAILURE(status)) {
		delete adoptedRule;
		return;
	}
	ruleVector->addElementX(adoptedRule, status);
	int32_t len;
	if((len = adoptedRule->getContextLength()) > maxContextLength) {
		maxContextLength = len;
	}

	uprv_free(rules);
	rules = 0;
}

/**
 * Check this for masked rules and index it to optimize performance.
 * The sequence of operations is: (1) add rules to a set using
 * <code>addRule()</code>; (2) freeze the set using
 * <code>freeze()</code>; (3) use the rule set.  If
 * <code>addRule()</code> is called after calling this method, it
 * invalidates this object, and this method must be called again.
 * That is, <code>freeze()</code> may be called multiple times,
 * although for optimal performance it shouldn't be.
 */
void TransliterationRuleSet::freeze(UParseError& parseError, UErrorCode & status) {
	/* Construct the rule array and index table.  We reorder the
	 * rules by sorting them into 256 bins.  Each bin contains all
	 * rules matching the index value for that bin.  A rule
	 * matches an index value if string whose first key character
	 * has a low byte equal to the index value can match the rule.
	 *
	 * Each bin contains zero or more rules, in the same order
	 * they were found originally.  However, the total rules in
	 * the bins may exceed the number in the original vector,
	 * since rules that have a variable as their first key
	 * character will generally fall into more than one bin.
	 *
	 * That is, each bin contains all rules that either have that
	 * first index value as their first key character, or have
	 * a set containing the index value as their first character.
	 */
	int32_t n = ruleVector->size();
	int32_t j;
	int16 x;
	UVector v(2*n, status); // heuristic; adjust as needed

	if(U_FAILURE(status)) {
		return;
	}

	/* Precompute the index values.  This saves a LOT of time.
	 * Be careful not to call malloc(0).
	 */
	int16* indexValue = (int16*)uprv_malloc(sizeof(int16) * (n > 0 ? n : 1));
	/* test for NULL */
	if(indexValue == 0) {
		status = U_MEMORY_ALLOCATION_ERROR;
		return;
	}
	for(j = 0; j<n; ++j) {
		TransliterationRule* r = (TransliterationRule*)ruleVector->elementAt(j);
		indexValue[j] = r->getIndexValue();
	}
	for(x = 0; x<256; ++x) {
		index[x] = v.size();
		for(j = 0; j<n; ++j) {
			if(indexValue[j] >= 0) {
				if(indexValue[j] == x) {
					v.addElementX(ruleVector->elementAt(j), status);
				}
			}
			else {
				// If the indexValue is < 0, then the first key character is
				// a set, and we must use the more time-consuming
				// matchesIndexValue check.  In practice this happens
				// rarely, so we seldom treat this code path.
				TransliterationRule* r = (TransliterationRule*)ruleVector->elementAt(j);
				if(r->matchesIndexValue((uint8)x)) {
					v.addElementX(r, status);
				}
			}
		}
	}
	uprv_free(indexValue);
	index[256] = v.size();

	/* Freeze things into an array.
	 */
	uprv_free(rules); // Contains alias pointers

	/* You can't do malloc(0)! */
	if(v.size() == 0) {
		rules = NULL;
		return;
	}
	rules = (TransliterationRule**)uprv_malloc(v.size() * sizeof(TransliterationRule *));
	/* test for NULL */
	if(rules == 0) {
		status = U_MEMORY_ALLOCATION_ERROR;
		return;
	}
	for(j = 0; j<v.size(); ++j) {
		rules[j] = (TransliterationRule*)v.elementAt(j);
	}

	// TODO Add error reporting that indicates the rules that
	//      are being masked.
	//UnicodeString errors;

	/* Check for masking.  This is MUCH faster than our old check,
	 * which was each rule against each following rule, since we
	 * only have to check for masking within each bin now.  It's
	 * 256*O(n2^2) instead of O(n1^2), where n1 is the total rule
	 * count, and n2 is the per-bin rule count.  But n2<<n1, so
	 * it's a big win.
	 */
	for(x = 0; x<256; ++x) {
		for(j = index[x]; j<index[x+1]-1; ++j) {
			TransliterationRule* r1 = rules[j];
			for(int32_t k = j+1; k<index[x+1]; ++k) {
				TransliterationRule* r2 = rules[k];
				if(r1->masks(*r2)) {
//|                 if(errors == null) {
//|                     errors = new StringBuffer();
//|                 } else {
//|                     errors.append("\n");
//|                 }
//|                 errors.append("Rule " + r1 + " masks " + r2);
					status = U_RULE_MASK_ERROR;
					maskingError(*r1, *r2, parseError);
					return;
				}
			}
		}
	}

	//if(errors != null) {
	//    throw new IllegalArgumentException(errors.toString());
	//}
}

/**
 * Transliterate the given text with the given UTransPosition
 * indices.  Return TRUE if the transliteration should continue
 * or FALSE if it should halt (because of a U_PARTIAL_MATCH match).
 * Note that FALSE is only ever returned if isIncremental is TRUE.
 * @param text the text to be transliterated
 * @param pos the position indices, which will be updated
 * @param incremental if TRUE, assume new text may be inserted
 * at index.limit, and return FALSE if there is a partial match.
 * @return TRUE unless a U_PARTIAL_MATCH has been obtained,
 * indicating that transliteration should stop until more text
 * arrives.
 */
bool TransliterationRuleSet::transliterate(Replaceable& text,
    UTransPosition& pos,
    bool incremental) {
	int16 indexByte = (int16)(text.char32At(pos.start) & 0xFF);
	for(int32_t i = index[indexByte]; i<index[indexByte+1]; ++i) {
		UMatchDegree m = rules[i]->matchAndReplace(text, pos, incremental);
		switch(m) {
			case U_MATCH:
			    _debugOut("match", rules[i], text, pos);
			    return TRUE;
			case U_PARTIAL_MATCH:
			    _debugOut("partial match", rules[i], text, pos);
			    return FALSE;
			default: /* Ram: added default to make GCC happy */
			    break;
		}
	}
	// No match or partial match from any rule
	pos.start += U16_LENGTH(text.char32At(pos.start));
	_debugOut("no match", NULL, text, pos);
	return TRUE;
}

/**
 * Create rule strings that represents this rule set.
 */
UnicodeString & TransliterationRuleSet::toRules(UnicodeString & ruleSource,
    bool escapeUnprintable) const {
	int32_t i;
	int32_t count = ruleVector->size();
	ruleSource.truncate(0);
	for(i = 0; i<count; ++i) {
		if(i != 0) {
			ruleSource.append((char16_t)0x000A /*\n*/);
		}
		TransliterationRule * r =
		    (TransliterationRule*)ruleVector->elementAt(i);
		r->toRule(ruleSource, escapeUnprintable);
	}
	return ruleSource;
}

/**
 * Return the set of all characters that may be modified
 * (getTarget=false) or emitted (getTarget=true) by this set.
 */
UnicodeSet & TransliterationRuleSet::getSourceTargetSet(UnicodeSet & result, bool getTarget) const
{
	result.clear();
	int32_t count = ruleVector->size();
	for(int32_t i = 0; i<count; ++i) {
		TransliterationRule* r = (TransliterationRule*)ruleVector->elementAt(i);
		if(getTarget) {
			r->addTargetSetTo(result);
		}
		else {
			r->addSourceSetTo(result);
		}
	}
	return result;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
