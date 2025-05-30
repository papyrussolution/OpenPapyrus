// rbtz.cpp
// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2007-2013, International Business Machines Corporation and others. All Rights Reserved.
//
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_FORMATTING

#include "gregoimp.h"

U_NAMESPACE_BEGIN

/**
 * A struct representing a time zone transition
 */
struct Transition : public UMemory {
	UDate time;
	TimeZoneRule* from;
	TimeZoneRule* to;
};

U_CDECL_BEGIN
static void U_CALLCONV deleteTransition(void * obj) { delete static_cast<Transition *>(obj); }
U_CDECL_END

static bool compareRules(UVector* rules1, UVector* rules2) 
{
	if(rules1 == NULL && rules2 == NULL) {
		return TRUE;
	}
	else if(rules1 == NULL || rules2 == NULL) {
		return FALSE;
	}
	int32_t size = rules1->size();
	if(size != rules2->size()) {
		return FALSE;
	}
	for(int32_t i = 0; i < size; i++) {
		TimeZoneRule * r1 = (TimeZoneRule*)rules1->elementAt(i);
		TimeZoneRule * r2 = (TimeZoneRule*)rules2->elementAt(i);
		if(*r1 != *r2) {
			return FALSE;
		}
	}
	return TRUE;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RuleBasedTimeZone)

RuleBasedTimeZone::RuleBasedTimeZone(const UnicodeString & id, InitialTimeZoneRule* initialRule) : 
	BasicTimeZone(id), fInitialRule(initialRule), fHistoricRules(NULL), fFinalRules(NULL), fHistoricTransitions(NULL), fUpToDate(FALSE) 
{
}

RuleBasedTimeZone::RuleBasedTimeZone(const RuleBasedTimeZone& source) : BasicTimeZone(source), fInitialRule(source.fInitialRule->clone()),
	fHistoricTransitions(NULL), fUpToDate(FALSE) 
{
	fHistoricRules = copyRules(source.fHistoricRules);
	fFinalRules = copyRules(source.fFinalRules);
	if(source.fUpToDate) {
		UErrorCode status = U_ZERO_ERROR;
		complete(status);
	}
}

RuleBasedTimeZone::~RuleBasedTimeZone() 
{
	deleteTransitions();
	deleteRules();
}

RuleBasedTimeZone & RuleBasedTimeZone::operator = (const RuleBasedTimeZone & right) 
{
	if(*this != right) {
		BasicTimeZone::operator = (right);
		deleteRules();
		fInitialRule = right.fInitialRule->clone();
		fHistoricRules = copyRules(right.fHistoricRules);
		fFinalRules = copyRules(right.fFinalRules);
		deleteTransitions();
		fUpToDate = FALSE;
	}
	return *this;
}

bool RuleBasedTimeZone::operator == (const TimeZone& that) const 
{
	if(this == &that) {
		return true;
	}
	if(typeid(*this) != typeid(that) || !BasicTimeZone::operator == (that)) {
		return false;
	}
	RuleBasedTimeZone * rbtz = (RuleBasedTimeZone*)&that;
	if(*fInitialRule != *(rbtz->fInitialRule)) {
		return false;
	}
	if(compareRules(fHistoricRules, rbtz->fHistoricRules)
	 && compareRules(fFinalRules, rbtz->fFinalRules)) {
		return true;
	}
	return false;
}

bool RuleBasedTimeZone::operator != (const TimeZone& that) const {
	return !operator == (that);
}

void RuleBasedTimeZone::addTransitionRule(TimeZoneRule* rule, UErrorCode & status) {
	LocalPointer<TimeZoneRule>lpRule(rule);
	if(U_FAILURE(status)) {
		return;
	}
	AnnualTimeZoneRule* atzrule = dynamic_cast<AnnualTimeZoneRule*>(rule);
	if(atzrule != nullptr && atzrule->getEndYear() == AnnualTimeZoneRule::MAX_YEAR) {
		// A final rule
		if(fFinalRules == nullptr) {
			LocalPointer<UVector> lpFinalRules(new UVector(uprv_deleteUObject, nullptr, status), status);
			if(U_FAILURE(status)) {
				return;
			}
			fFinalRules = lpFinalRules.orphan();
		}
		else if(fFinalRules->size() >= 2) {
			// Cannot handle more than two final rules
			status = U_INVALID_STATE_ERROR;
			return;
		}
		fFinalRules->adoptElement(lpRule.orphan(), status);
	}
	else {
		// Non-final rule
		if(fHistoricRules == nullptr) {
			LocalPointer<UVector> lpHistoricRules(new UVector(uprv_deleteUObject, nullptr, status), status);
			if(U_FAILURE(status)) {
				return;
			}
			fHistoricRules = lpHistoricRules.orphan();
		}
		fHistoricRules->adoptElement(lpRule.orphan(), status);
	}
	// Mark dirty, so transitions are recalculated at next complete() call
	fUpToDate = FALSE;
}

void RuleBasedTimeZone::completeConst(UErrorCode & status) const {
	static UMutex gLock;
	if(U_FAILURE(status)) {
		return;
	}
	umtx_lock(&gLock);
	if(!fUpToDate) {
		RuleBasedTimeZone * ncThis = const_cast<RuleBasedTimeZone*>(this);
		ncThis->complete(status);
	}
	umtx_unlock(&gLock);
}

void RuleBasedTimeZone::complete(UErrorCode & status) {
	if(U_FAILURE(status)) {
		return;
	}
	if(fUpToDate) {
		return;
	}
	// Make sure either no final rules or a pair of AnnualTimeZoneRules
	// are available.
	if(fFinalRules != NULL && fFinalRules->size() != 2) {
		status = U_INVALID_STATE_ERROR;
		return;
	}

	// Create a TimezoneTransition and add to the list
	if(fHistoricRules != NULL || fFinalRules != NULL) {
		TimeZoneRule * curRule = fInitialRule;
		UDate lastTransitionTime = MIN_MILLIS;

		// Build the transition array which represents historical time zone
		// transitions.
		if(fHistoricRules != NULL && fHistoricRules->size() > 0) {
			int32_t i;
			int32_t historicCount = fHistoricRules->size();
			LocalMemory<bool> done((bool*)uprv_malloc(sizeof(bool) * historicCount));
			if(done == NULL) {
				status = U_MEMORY_ALLOCATION_ERROR;
				goto cleanup;
			}
			for(i = 0; i < historicCount; i++) {
				done[i] = false;
			}
			while(true) {
				int32_t curStdOffset = curRule->getRawOffset();
				int32_t curDstSavings = curRule->getDSTSavings();
				UDate nextTransitionTime = MAX_MILLIS;
				TimeZoneRule * nextRule = NULL;
				TimeZoneRule * r = NULL;
				bool avail;
				UDate tt;
				UnicodeString curName, name;
				curRule->getName(curName);

				for(i = 0; i < historicCount; i++) {
					if(done[i]) {
						continue;
					}
					r = (TimeZoneRule*)fHistoricRules->elementAt(i);
					avail = r->getNextStart(lastTransitionTime, curStdOffset, curDstSavings, false, tt);
					if(!avail) {
						// No more transitions from this rule - skip this rule next time
						done[i] = true;
					}
					else {
						r->getName(name);
						if(*r == *curRule ||
						    (name == curName && r->getRawOffset() == curRule->getRawOffset()
						 && r->getDSTSavings() == curRule->getDSTSavings())) {
							continue;
						}
						if(tt < nextTransitionTime) {
							nextTransitionTime = tt;
							nextRule = r;
						}
					}
				}

				if(nextRule ==  NULL) {
					// Check if all historic rules are done
					bool bDoneAll = TRUE;
					for(int32_t j = 0; j < historicCount; j++) {
						if(!done[j]) {
							bDoneAll = FALSE;
							break;
						}
					}
					if(bDoneAll) {
						break;
					}
				}
				if(fFinalRules) {
					// Check if one of final rules has earlier transition date
					for(i = 0; i < 2 /* fFinalRules->size() */; i++) {
						TimeZoneRule * fr = (TimeZoneRule*)fFinalRules->elementAt(i);
						if(*fr == *curRule) {
							continue;
						}
						r = (TimeZoneRule*)fFinalRules->elementAt(i);
						avail = r->getNextStart(lastTransitionTime, curStdOffset, curDstSavings, false, tt);
						if(avail) {
							if(tt < nextTransitionTime) {
								nextTransitionTime = tt;
								nextRule = r;
							}
						}
					}
				}
				if(!nextRule) {
					// Nothing more
					break;
				}
				if(fHistoricTransitions == NULL) {
					LocalPointer<UVector> lpHistoricTransitions(new UVector(deleteTransition, nullptr, status), status);
					if(U_FAILURE(status)) {
						goto cleanup;
					}
					fHistoricTransitions = lpHistoricTransitions.orphan();
				}
				LocalPointer<Transition> trst(new Transition, status);
				if(U_FAILURE(status)) {
					goto cleanup;
				}
				trst->time = nextTransitionTime;
				trst->from = curRule;
				trst->to = nextRule;
				fHistoricTransitions->adoptElement(trst.orphan(), status);
				if(U_FAILURE(status)) {
					goto cleanup;
				}
				lastTransitionTime = nextTransitionTime;
				curRule = nextRule;
			}
		}
		if(fFinalRules) {
			if(fHistoricTransitions == NULL) {
				LocalPointer<UVector> lpHistoricTransitions(new UVector(deleteTransition, nullptr, status), status);
				if(U_FAILURE(status)) {
					goto cleanup;
				}
				fHistoricTransitions = lpHistoricTransitions.orphan();
			}
			// Append the first transition for each
			TimeZoneRule * rule0 = (TimeZoneRule*)fFinalRules->elementAt(0);
			TimeZoneRule * rule1 = (TimeZoneRule*)fFinalRules->elementAt(1);
			UDate tt0, tt1;
			bool avail0 = rule0->getNextStart(lastTransitionTime, curRule->getRawOffset(), curRule->getDSTSavings(), false, tt0);
			bool avail1 = rule1->getNextStart(lastTransitionTime, curRule->getRawOffset(), curRule->getDSTSavings(), false, tt1);
			if(!avail0 || !avail1) {
				// Should not happen, because both rules are permanent
				status = U_INVALID_STATE_ERROR;
				goto cleanup;
			}
			LocalPointer<Transition> final0(new Transition, status);
			LocalPointer<Transition> final1(new Transition, status);
			if(U_FAILURE(status)) {
				goto cleanup;
			}
			if(tt0 < tt1) {
				final0->time = tt0;
				final0->from = curRule;
				final0->to = rule0;
				rule1->getNextStart(tt0, rule0->getRawOffset(), rule0->getDSTSavings(), false, final1->time);
				final1->from = rule0;
				final1->to = rule1;
			}
			else {
				final0->time = tt1;
				final0->from = curRule;
				final0->to = rule1;
				rule0->getNextStart(tt1, rule1->getRawOffset(), rule1->getDSTSavings(), false, final1->time);
				final1->from = rule1;
				final1->to = rule0;
			}
			fHistoricTransitions->adoptElement(final0.orphan(), status);
			fHistoricTransitions->adoptElement(final1.orphan(), status);
			if(U_FAILURE(status)) {
				goto cleanup;
			}
		}
	}
	fUpToDate = TRUE;
	return;
cleanup:
	deleteTransitions();
	fUpToDate = FALSE;
}

RuleBasedTimeZone* RuleBasedTimeZone::clone() const { return new RuleBasedTimeZone(*this); }

int32_t RuleBasedTimeZone::getOffset(uint8 era, int32_t year, int32_t month, int32_t day,
    uint8 dayOfWeek, int32_t millis, UErrorCode & status) const {
	if(U_FAILURE(status)) {
		return 0;
	}
	if(month < UCAL_JANUARY || month > UCAL_DECEMBER) {
		status = U_ILLEGAL_ARGUMENT_ERROR;
		return 0;
	}
	else {
		return getOffset(era, year, month, day, dayOfWeek, millis,
			   Grego::monthLength(year, month), status);
	}
}

int32_t RuleBasedTimeZone::getOffset(uint8 era, int32_t year, int32_t month, int32_t day, uint8 /*dayOfWeek*/, int32_t millis, int32_t /*monthLength*/, UErrorCode & status) const 
{
	// dayOfWeek and monthLength are unused
	if(U_FAILURE(status)) {
		return 0;
	}
	if(era == GregorianCalendar::BC) {
		// Convert to extended year
		year = 1 - year;
	}
	int32_t rawOffset, dstOffset;
	UDate time = (UDate)Grego::fieldsToDay(year, month, day) * U_MILLIS_PER_DAY + millis;
	getOffsetInternal(time, TRUE, kDaylight, kStandard, rawOffset, dstOffset, status);
	if(U_FAILURE(status)) {
		return 0;
	}
	return (rawOffset + dstOffset);
}

void RuleBasedTimeZone::getOffset(UDate date, bool local, int32_t& rawOffset, int32_t& dstOffset, UErrorCode & status) const 
{
	getOffsetInternal(date, local, kFormer, kLatter, rawOffset, dstOffset, status);
}

void RuleBasedTimeZone::getOffsetFromLocal(UDate date, UTimeZoneLocalOption nonExistingTimeOpt, 
	UTimeZoneLocalOption duplicatedTimeOpt, int32_t& rawOffset, int32_t& dstOffset, UErrorCode & status) const 
{
	getOffsetInternal(date, TRUE, nonExistingTimeOpt, duplicatedTimeOpt, rawOffset, dstOffset, status);
}

/*
 * The internal getOffset implementation
 */
void RuleBasedTimeZone::getOffsetInternal(UDate date, bool local, int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt, int32_t& rawOffset, int32_t& dstOffset, UErrorCode & status) const 
{
	rawOffset = 0;
	dstOffset = 0;
	if(U_FAILURE(status)) {
		return;
	}
	if(!fUpToDate) {
		// Transitions are not yet resolved.  We cannot do it here
		// because this method is const.  Thus, do nothing and return
		// error status.
		status = U_INVALID_STATE_ERROR;
		return;
	}
	const TimeZoneRule * rule = NULL;
	if(fHistoricTransitions == NULL) {
		rule = fInitialRule;
	}
	else {
		UDate tstart = getTransitionTime((Transition*)fHistoricTransitions->elementAt(0),
			local, NonExistingTimeOpt, DuplicatedTimeOpt);
		if(date < tstart) {
			rule = fInitialRule;
		}
		else {
			int32_t idx = fHistoricTransitions->size() - 1;
			UDate tend = getTransitionTime((Transition*)fHistoricTransitions->elementAt(idx),
				local, NonExistingTimeOpt, DuplicatedTimeOpt);
			if(date > tend) {
				if(fFinalRules) {
					rule = findRuleInFinal(date, local, NonExistingTimeOpt, DuplicatedTimeOpt);
				}
				if(rule == NULL) {
					// no final rules or the given time is before the first transition
					// specified by the final rules -> use the last rule
					rule = ((Transition*)fHistoricTransitions->elementAt(idx))->to;
				}
			}
			else {
				// Find a historical transition
				while(idx >= 0) {
					if(date >= getTransitionTime((Transition*)fHistoricTransitions->elementAt(idx),
					    local, NonExistingTimeOpt, DuplicatedTimeOpt)) {
						break;
					}
					idx--;
				}
				rule = ((Transition*)fHistoricTransitions->elementAt(idx))->to;
			}
		}
	}
	if(rule) {
		rawOffset = rule->getRawOffset();
		dstOffset = rule->getDSTSavings();
	}
}

void RuleBasedTimeZone::setRawOffset(int32_t /*offsetMillis*/) 
{
	// We don't support this operation at this moment.
	// Nothing to do!
}

int32_t RuleBasedTimeZone::getRawOffset() const 
{
	// Note: This implementation returns standard GMT offset
	// as of current time.
	UErrorCode status = U_ZERO_ERROR;
	int32_t raw, dst;
	getOffset(uprv_getUTCtime() * U_MILLIS_PER_SECOND, FALSE, raw, dst, status);
	return raw;
}

bool RuleBasedTimeZone::useDaylightTime() const {
	// Note: This implementation returns true when
	// daylight saving time is used as of now or
	// after the next transition.
	UErrorCode status = U_ZERO_ERROR;
	UDate now = uprv_getUTCtime() * U_MILLIS_PER_SECOND;
	int32_t raw, dst;
	getOffset(now, FALSE, raw, dst, status);
	if(dst != 0) {
		return TRUE;
	}
	// If DST is not used now, check if DST is used after the next transition
	UDate time;
	TimeZoneRule * from, * to;
	bool avail = findNext(now, FALSE, time, from, to);
	if(avail && to->getDSTSavings() != 0) {
		return TRUE;
	}
	return FALSE;
}

bool RuleBasedTimeZone::inDaylightTime(UDate date, UErrorCode & status) const {
	if(U_FAILURE(status)) {
		return FALSE;
	}
	int32_t raw, dst;
	getOffset(date, FALSE, raw, dst, status);
	if(dst != 0) {
		return TRUE;
	}
	return FALSE;
}

bool RuleBasedTimeZone::hasSameRules(const TimeZone& other) const {
	if(this == &other) {
		return TRUE;
	}
	if(typeid(*this) != typeid(other)) {
		return FALSE;
	}
	const RuleBasedTimeZone& that = (const RuleBasedTimeZone&)other;
	if(*fInitialRule != *(that.fInitialRule)) {
		return FALSE;
	}
	if(compareRules(fHistoricRules, that.fHistoricRules)
	 && compareRules(fFinalRules, that.fFinalRules)) {
		return TRUE;
	}
	return FALSE;
}

bool RuleBasedTimeZone::getNextTransition(UDate base, bool inclusive, TimeZoneTransition& result) const {
	UErrorCode status = U_ZERO_ERROR;
	completeConst(status);
	if(U_FAILURE(status)) {
		return FALSE;
	}
	UDate transitionTime;
	TimeZoneRule * fromRule, * toRule;
	bool found = findNext(base, inclusive, transitionTime, fromRule, toRule);
	if(found) {
		result.setTime(transitionTime);
		result.setFrom((const TimeZoneRule&)*fromRule);
		result.setTo((const TimeZoneRule&)*toRule);
		return TRUE;
	}
	return FALSE;
}

bool RuleBasedTimeZone::getPreviousTransition(UDate base, bool inclusive, TimeZoneTransition& result) const 
{
	UErrorCode status = U_ZERO_ERROR;
	completeConst(status);
	if(U_FAILURE(status)) {
		return FALSE;
	}
	UDate transitionTime;
	TimeZoneRule * fromRule, * toRule;
	bool found = findPrev(base, inclusive, transitionTime, fromRule, toRule);
	if(found) {
		result.setTime(transitionTime);
		result.setFrom((const TimeZoneRule&)*fromRule);
		result.setTo((const TimeZoneRule&)*toRule);
		return TRUE;
	}
	return FALSE;
}

int32_t RuleBasedTimeZone::countTransitionRules(UErrorCode & /*status*/) const 
{
	int32_t count = 0;
	if(fHistoricRules != NULL) {
		count += fHistoricRules->size();
	}
	if(fFinalRules) {
		count += fFinalRules->size();
	}
	return count;
}

void RuleBasedTimeZone::getTimeZoneRules(const InitialTimeZoneRule*& initial, const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode & status) const 
{
	if(U_FAILURE(status)) {
		return;
	}
	// Initial rule
	initial = fInitialRule;

	// Transition rules
	int32_t cnt = 0;
	if(fHistoricRules != NULL && cnt < trscount) {
		int32_t historicCount = fHistoricRules->size();
		int32_t idx = 0;
		while(cnt < trscount && idx < historicCount) {
			trsrules[cnt++] = (const TimeZoneRule*)fHistoricRules->elementAt(idx++);
		}
	}
	if(fFinalRules != NULL && cnt < trscount) {
		int32_t finalCount = fFinalRules->size();
		int32_t idx = 0;
		while(cnt < trscount && idx < finalCount) {
			trsrules[cnt++] = (const TimeZoneRule*)fFinalRules->elementAt(idx++);
		}
	}
	// Set the result length
	trscount = cnt;
}

void RuleBasedTimeZone::deleteRules() 
{
	ZDELETE(fInitialRule);
	ZDELETEFAST(fHistoricRules);
	ZDELETEFAST(fFinalRules);
}

void RuleBasedTimeZone::deleteTransitions() 
{
	ZDELETEFAST(fHistoricTransitions);
}

UVector* RuleBasedTimeZone::copyRules(UVector* source) 
{
	if(source == nullptr) {
		return nullptr;
	}
	UErrorCode ec = U_ZERO_ERROR;
	int32_t size = source->size();
	LocalPointer<UVector> rules(new UVector(uprv_deleteUObject, nullptr, size, ec), ec);
	if(U_FAILURE(ec)) {
		return nullptr;
	}
	int32_t i;
	for(i = 0; i < size; i++) {
		LocalPointer<TimeZoneRule> rule(((TimeZoneRule*)source->elementAt(i))->clone(), ec);
		rules->adoptElement(rule.orphan(), ec);
		if(U_FAILURE(ec)) {
			return nullptr;
		}
	}
	return rules.orphan();
}

TimeZoneRule* RuleBasedTimeZone::findRuleInFinal(UDate date, bool local, int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const 
{
	if(fFinalRules == NULL) {
		return NULL;
	}
	AnnualTimeZoneRule* fr0 = (AnnualTimeZoneRule*)fFinalRules->elementAt(0);
	AnnualTimeZoneRule* fr1 = (AnnualTimeZoneRule*)fFinalRules->elementAt(1);
	if(fr0 == NULL || fr1 == NULL) {
		return NULL;
	}
	UDate start0, start1;
	int32_t localDelta;
	UDate base = date;
	if(local) {
		localDelta = getLocalDelta(fr1->getRawOffset(), fr1->getDSTSavings(),
			fr0->getRawOffset(), fr0->getDSTSavings(),
			NonExistingTimeOpt, DuplicatedTimeOpt);
		base -= localDelta;
	}
	bool avail0 = fr0->getPreviousStart(base, fr1->getRawOffset(), fr1->getDSTSavings(), TRUE, start0);
	base = date;
	if(local) {
		localDelta = getLocalDelta(fr0->getRawOffset(), fr0->getDSTSavings(),
			fr1->getRawOffset(), fr1->getDSTSavings(),
			NonExistingTimeOpt, DuplicatedTimeOpt);
		base -= localDelta;
	}
	bool avail1 = fr1->getPreviousStart(base, fr0->getRawOffset(), fr0->getDSTSavings(), TRUE, start1);

	if(!avail0 || !avail1) {
		if(avail0) {
			return fr0;
		}
		else if(avail1) {
			return fr1;
		}
		// Both rules take effect after the given time
		return NULL;
	}

	return (start0 > start1) ? fr0 : fr1;
}

bool RuleBasedTimeZone::findNext(UDate base, bool inclusive, UDate& transitionTime,
    TimeZoneRule*& fromRule, TimeZoneRule*& toRule) const {
	if(fHistoricTransitions == NULL) {
		return FALSE;
	}
	bool isFinal = FALSE;
	bool found = FALSE;
	Transition result;
	Transition * tzt = (Transition*)fHistoricTransitions->elementAt(0);
	UDate tt = tzt->time;
	if(tt > base || (inclusive && tt == base)) {
		result = *tzt;
		found = TRUE;
	}
	else {
		int32_t idx = fHistoricTransitions->size() - 1;
		tzt = (Transition*)fHistoricTransitions->elementAt(idx);
		tt = tzt->time;
		if(inclusive && tt == base) {
			result = *tzt;
			found = TRUE;
		}
		else if(tt <= base) {
			if(fFinalRules) {
				// Find a transion time with finalRules
				TimeZoneRule * r0 = (TimeZoneRule*)fFinalRules->elementAt(0);
				TimeZoneRule * r1 = (TimeZoneRule*)fFinalRules->elementAt(1);
				UDate start0, start1;
				bool avail0 = r0->getNextStart(base, r1->getRawOffset(), r1->getDSTSavings(), inclusive, start0);
				bool avail1 = r1->getNextStart(base, r0->getRawOffset(), r0->getDSTSavings(), inclusive, start1);
				//  avail0/avail1 should be always TRUE
				if(!avail0 && !avail1) {
					return FALSE;
				}
				if(!avail1 || start0 < start1) {
					result.time = start0;
					result.from = r1;
					result.to = r0;
				}
				else {
					result.time = start1;
					result.from = r0;
					result.to = r1;
				}
				isFinal = TRUE;
				found = TRUE;
			}
		}
		else {
			// Find a transition within the historic transitions
			idx--;
			Transition * prev = tzt;
			while(idx > 0) {
				tzt = (Transition*)fHistoricTransitions->elementAt(idx);
				tt = tzt->time;
				if(tt < base || (!inclusive && tt == base)) {
					break;
				}
				idx--;
				prev = tzt;
			}
			result.time = prev->time;
			result.from = prev->from;
			result.to = prev->to;
			found = TRUE;
		}
	}
	if(found) {
		// For now, this implementation ignore transitions with only zone name changes.
		if(result.from->getRawOffset() == result.to->getRawOffset()
		 && result.from->getDSTSavings() == result.to->getDSTSavings()) {
			if(isFinal) {
				return FALSE;
			}
			else {
				// No offset changes.  Try next one if not final
				return findNext(result.time, FALSE /* always exclusive */,
					   transitionTime, fromRule, toRule);
			}
		}
		transitionTime = result.time;
		fromRule = result.from;
		toRule = result.to;
		return TRUE;
	}
	return FALSE;
}

bool RuleBasedTimeZone::findPrev(UDate base, bool inclusive, UDate& transitionTime,
    TimeZoneRule*& fromRule, TimeZoneRule*& toRule) const {
	if(fHistoricTransitions == NULL) {
		return FALSE;
	}
	bool found = FALSE;
	Transition result;
	Transition * tzt = (Transition*)fHistoricTransitions->elementAt(0);
	UDate tt = tzt->time;
	if(inclusive && tt == base) {
		result = *tzt;
		found = TRUE;
	}
	else if(tt < base) {
		int32_t idx = fHistoricTransitions->size() - 1;
		tzt = (Transition*)fHistoricTransitions->elementAt(idx);
		tt = tzt->time;
		if(inclusive && tt == base) {
			result = *tzt;
			found = TRUE;
		}
		else if(tt < base) {
			if(fFinalRules) {
				// Find a transion time with finalRules
				TimeZoneRule * r0 = (TimeZoneRule*)fFinalRules->elementAt(0);
				TimeZoneRule * r1 = (TimeZoneRule*)fFinalRules->elementAt(1);
				UDate start0, start1;
				bool avail0 = r0->getPreviousStart(base, r1->getRawOffset(), r1->getDSTSavings(), inclusive, start0);
				bool avail1 = r1->getPreviousStart(base, r0->getRawOffset(), r0->getDSTSavings(), inclusive, start1);
				//  avail0/avail1 should be always TRUE
				if(!avail0 && !avail1) {
					return FALSE;
				}
				if(!avail1 || start0 > start1) {
					result.time = start0;
					result.from = r1;
					result.to = r0;
				}
				else {
					result.time = start1;
					result.from = r0;
					result.to = r1;
				}
			}
			else {
				result = *tzt;
			}
			found = TRUE;
		}
		else {
			// Find a transition within the historic transitions
			idx--;
			while(idx >= 0) {
				tzt = (Transition*)fHistoricTransitions->elementAt(idx);
				tt = tzt->time;
				if(tt < base || (inclusive && tt == base)) {
					break;
				}
				idx--;
			}
			result = *tzt;
			found = TRUE;
		}
	}
	if(found) {
		// For now, this implementation ignore transitions with only zone name changes.
		if(result.from->getRawOffset() == result.to->getRawOffset()
		 && result.from->getDSTSavings() == result.to->getDSTSavings()) {
			// No offset changes.  Try next one if not final
			return findPrev(result.time, FALSE /* always exclusive */,
				   transitionTime, fromRule, toRule);
		}
		transitionTime = result.time;
		fromRule = result.from;
		toRule = result.to;
		return TRUE;
	}
	return FALSE;
}

UDate RuleBasedTimeZone::getTransitionTime(Transition* transition, bool local,
    int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const {
	UDate time = transition->time;
	if(local) {
		time += getLocalDelta(transition->from->getRawOffset(), transition->from->getDSTSavings(),
			transition->to->getRawOffset(), transition->to->getDSTSavings(),
			NonExistingTimeOpt, DuplicatedTimeOpt);
	}
	return time;
}

int32_t RuleBasedTimeZone::getLocalDelta(int32_t rawBefore, int32_t dstBefore, int32_t rawAfter, int32_t dstAfter,
    int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const {
	int32_t delta = 0;

	int32_t offsetBefore = rawBefore + dstBefore;
	int32_t offsetAfter = rawAfter + dstAfter;

	bool dstToStd = (dstBefore != 0) && (dstAfter == 0);
	bool stdToDst = (dstBefore == 0) && (dstAfter != 0);

	if(offsetAfter - offsetBefore >= 0) {
		// Positive transition, which makes a non-existing local time range
		if(((NonExistingTimeOpt & kStdDstMask) == kStandard && dstToStd)
		 || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
			delta = offsetBefore;
		}
		else if(((NonExistingTimeOpt & kStdDstMask) == kStandard && stdToDst)
		 || ((NonExistingTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
			delta = offsetAfter;
		}
		else if((NonExistingTimeOpt & kFormerLatterMask) == kLatter) {
			delta = offsetBefore;
		}
		else {
			// Interprets the time with rule before the transition,
			// default for non-existing time range
			delta = offsetAfter;
		}
	}
	else {
		// Negative transition, which makes a duplicated local time range
		if(((DuplicatedTimeOpt & kStdDstMask) == kStandard && dstToStd)
		 || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && stdToDst)) {
			delta = offsetAfter;
		}
		else if(((DuplicatedTimeOpt & kStdDstMask) == kStandard && stdToDst)
		 || ((DuplicatedTimeOpt & kStdDstMask) == kDaylight && dstToStd)) {
			delta = offsetBefore;
		}
		else if((DuplicatedTimeOpt & kFormerLatterMask) == kFormer) {
			delta = offsetBefore;
		}
		else {
			// Interprets the time with rule after the transition,
			// default for duplicated local time range
			delta = offsetAfter;
		}
	}
	return delta;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
