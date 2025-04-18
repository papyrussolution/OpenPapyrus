// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
// Copyright (C) 2006-2016, International Business Machines Corporation and others. All Rights Reserved.
//
#include <icu-internal.h>
#pragma hdrstop

#if !UCONFIG_NO_BREAK_ITERATION

#include "dictbe.h"
#include "utracimp.h"
#include "dictionarydata.h"

U_NAMESPACE_BEGIN

DictionaryBreakEngine::DictionaryBreakEngine() 
{
}

DictionaryBreakEngine::~DictionaryBreakEngine() 
{
}

bool DictionaryBreakEngine::handles(UChar32 c) const { return fSet.contains(c); }

int32_t DictionaryBreakEngine::findBreaks(UText * text, int32_t startPos, int32_t endPos, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) 
		return 0;
	(void)startPos;        // TODO: remove this param?
	int32_t result = 0;
	// Find the span of characters included in the set.
	//   The span to break begins at the current position in the text, and
	//   extends towards the start or end of the text, depending on 'reverse'.

	int32_t start = (int32_t)utext_getNativeIndex(text);
	int32_t current;
	int32_t rangeStart;
	int32_t rangeEnd;
	UChar32 c = utext_current32(text);
	while((current = (int32_t)utext_getNativeIndex(text)) < endPos && fSet.contains(c)) {
		utext_next32(text); // TODO:  recast loop for postincrement
		c = utext_current32(text);
	}
	rangeStart = start;
	rangeEnd = current;
	result = divideUpDictionaryRange(text, rangeStart, rangeEnd, foundBreaks, status);
	utext_setNativeIndex(text, current);
	return result;
}

void DictionaryBreakEngine::setCharacters(const UnicodeSet & set) 
{
	fSet = set;
	// Compact for caching
	fSet.compact();
}
// 
// PossibleWord
// 
// Helper class for improving readability of the Thai/Lao/Khmer word break
// algorithm. The implementation is completely inline.

// List size, limited by the maximum number of words in the dictionary
// that form a nested sequence.
static const int32_t POSSIBLE_WORD_LIST_MAX = 20;

class PossibleWord {
private:
	// list of word candidate lengths, in increasing length order
	// TODO: bytes would be sufficient for word lengths.
	int32_t count;    // Count of candidates
	int32_t prefix;   // The longest match with a dictionary word
	int32_t offset;   // Offset in the text of these candidates
	int32_t mark;     // The preferred candidate's offset
	int32_t current;  // The candidate we're currently looking at
	int32_t cuLengths[POSSIBLE_WORD_LIST_MAX]; // Word Lengths, in code units.
	int32_t cpLengths[POSSIBLE_WORD_LIST_MAX]; // Word Lengths, in code points.

public:
	PossibleWord() : count(0), prefix(0), offset(-1), mark(0), current(0) 
	{
	}
	~PossibleWord() 
	{
	}
	// Fill the list of candidates if needed, select the longest, and return the number found
	int32_t candidates(UText * text, DictionaryMatcher * dict, int32_t rangeEnd);
	// Select the currently marked candidate, point after it in the text, and invalidate self
	int32_t acceptMarked(UText * text);
	// Back up from the current candidate to the next shorter one; return TRUE if that exists
	// and point the text after it
	bool     backUp(UText * text);
	// Return the longest prefix this candidate location shares with a dictionary word
	// Return value is in code points.
	int32_t longestPrefix() const { return prefix; }
	// Mark the current candidate as the one we like
	void   markCurrent() 
	{
		mark = current;
	}
	// Get length in code points of the marked word.
	int32_t markedCPLength() const { return cpLengths[mark]; }
};

int32_t PossibleWord::candidates(UText * text, DictionaryMatcher * dict, int32_t rangeEnd) 
{
	// TODO: If getIndex is too slow, use offset < 0 and add discardAll()
	int32_t start = (int32_t)utext_getNativeIndex(text);
	if(start != offset) {
		offset = start;
		count = dict->matches(text, rangeEnd-start, SIZEOFARRAYi(cuLengths), cuLengths, cpLengths, NULL, &prefix);
		// Dictionary leaves text after longest prefix, not longest word. Back up.
		if(count <= 0) {
			utext_setNativeIndex(text, start);
		}
	}
	if(count > 0) {
		utext_setNativeIndex(text, start+cuLengths[count-1]);
	}
	current = count-1;
	mark = current;
	return count;
}

int32_t PossibleWord::acceptMarked(UText * text) 
{
	utext_setNativeIndex(text, offset + cuLengths[mark]);
	return cuLengths[mark];
}

bool PossibleWord::backUp(UText * text) 
{
	if(current > 0) {
		utext_setNativeIndex(text, offset + cuLengths[--current]);
		return TRUE;
	}
	return FALSE;
}
// 
// ThaiBreakEngine
// 
static const int32_t THAI_LOOKAHEAD = 3; // How many words in a row are "good enough"?
static const int32_t THAI_ROOT_COMBINE_THRESHOLD = 3; // Will not combine a non-word with a preceding dictionary word longer than this
static const int32_t THAI_PREFIX_COMBINE_THRESHOLD = 3; // Will not combine a non-word that shares at least this much prefix with a dictionary word, with a preceding word
static const int32_t THAI_PAIYANNOI = 0x0E2F; // Elision character
static const int32_t THAI_MAIYAMOK = 0x0E46; // Repeat character
static const int32_t THAI_MIN_WORD = 2; // Minimum word size
static const int32_t THAI_MIN_WORD_SPAN = THAI_MIN_WORD * 2; // Minimum number of characters for two words

ThaiBreakEngine::ThaiBreakEngine(DictionaryMatcher * adoptDictionary, UErrorCode & status) : DictionaryBreakEngine(), fDictionary(adoptDictionary)
{
	UTRACE_ENTRY(UTRACE_UBRK_CREATE_BREAK_ENGINE);
	UTRACE_DATA1(UTRACE_INFO, "dictbe=%s", "Thai");
	fThaiWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]]"), status);
	if(U_SUCCESS(status)) {
		setCharacters(fThaiWordSet);
	}
	fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Thai:]&[:LineBreak=SA:]&[:M:]]"), status);
	fMarkSet.add(0x0020);
	fEndWordSet = fThaiWordSet;
	fEndWordSet.remove(0x0E31);         // MAI HAN-AKAT
	fEndWordSet.remove(0x0E40, 0x0E44); // SARA E through SARA AI MAIMALAI
	fBeginWordSet.add(0x0E01, 0x0E2E);  // KO KAI through HO NOKHUK
	fBeginWordSet.add(0x0E40, 0x0E44);  // SARA E through SARA AI MAIMALAI
	fSuffixSet.add(THAI_PAIYANNOI);
	fSuffixSet.add(THAI_MAIYAMOK);

	// Compact for caching.
	fMarkSet.compact();
	fEndWordSet.compact();
	fBeginWordSet.compact();
	fSuffixSet.compact();
	UTRACE_EXIT_STATUS(status);
}

ThaiBreakEngine::~ThaiBreakEngine() 
{
	delete fDictionary;
}

int32_t ThaiBreakEngine::divideUpDictionaryRange(UText * text, int32_t rangeStart, int32_t rangeEnd, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) 
		return 0;
	utext_setNativeIndex(text, rangeStart);
	utext_moveIndex32(text, THAI_MIN_WORD_SPAN);
	if(utext_getNativeIndex(text) >= rangeEnd) {
		return 0; // Not enough characters for two words
	}
	utext_setNativeIndex(text, rangeStart);
	uint32_t wordsFound = 0;
	int32_t cpWordLength = 0; // Word Length in Code Points.
	int32_t cuWordLength = 0; // Word length in code units (UText native indexing)
	int32_t current;
	PossibleWord words[THAI_LOOKAHEAD];
	utext_setNativeIndex(text, rangeStart);
	while(U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
		cpWordLength = 0;
		cuWordLength = 0;
		// Look for candidate words at the current position
		int32_t candidates = words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
		// If we found exactly one, use that
		if(candidates == 1) {
			cuWordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % THAI_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// If there was more than one, see which one can take us forward the most words
		else if(candidates > 1) {
			// If we're already at the end of the range, we're done
			if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
				goto foundBest;
			}
			do {
				if(words[(wordsFound + 1) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
					// Followed by another dictionary word; mark first word as a good candidate
					words[wordsFound%THAI_LOOKAHEAD].markCurrent();
					// If we're already at the end of the range, we're done
					if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
						goto foundBest;
					}
					// See if any of the possible second words is followed by a third word
					do {
						// If we find a third word, stop right away
						if(words[(wordsFound + 2) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
							words[wordsFound % THAI_LOOKAHEAD].markCurrent();
							goto foundBest;
						}
					} while(words[(wordsFound + 1) % THAI_LOOKAHEAD].backUp(text));
				}
			} while(words[wordsFound % THAI_LOOKAHEAD].backUp(text));
foundBest:
			// Set UText position to after the accepted word.
			cuWordLength = words[wordsFound % THAI_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % THAI_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// We come here after having either found a word or not. We look ahead to the
		// next word. If it's not a dictionary word, we will combine it with the word we
		// just found (if there is one), but only if the preceding word does not exceed
		// the threshold.
		// The text iterator should now be positioned at the end of the word we found.
		UChar32 uc = 0;
		if((int32_t)utext_getNativeIndex(text) < rangeEnd &&  cpWordLength < THAI_ROOT_COMBINE_THRESHOLD) {
			// if it is a dictionary word, do nothing. If it isn't, then if there is
			// no preceding word, or the non-word shares less than the minimum threshold
			// of characters with a dictionary word, then scan to resynchronize
			if(words[wordsFound % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0 && 
				(cuWordLength == 0 || words[wordsFound%THAI_LOOKAHEAD].longestPrefix() < THAI_PREFIX_COMBINE_THRESHOLD)) {
				// Look for a plausible word boundary
				int32_t remaining = rangeEnd - (current+cuWordLength);
				UChar32 pc;
				int32_t chars = 0;
				for(;;) {
					int32_t pcIndex = (int32_t)utext_getNativeIndex(text);
					pc = utext_next32(text);
					int32_t pcSize = (int32_t)utext_getNativeIndex(text) - pcIndex;
					chars += pcSize;
					remaining -= pcSize;
					if(remaining <= 0) {
						break;
					}
					uc = utext_current32(text);
					if(fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
						// Maybe. See if it's in the dictionary.
						// NOTE: In the original Apple code, checked that the next
						// two characters after uc were not 0x0E4C THANTHAKHAT before
						// checking the dictionary. That is just a performance filter,
						// but it's not clear it's faster than checking the trie.
						int32_t num_candidates = words[(wordsFound + 1) % THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
						utext_setNativeIndex(text, current + cuWordLength + chars);
						if(num_candidates > 0) {
							break;
						}
					}
				}
				// Bump the word count if there wasn't already one
				if(cuWordLength <= 0) {
					wordsFound += 1;
				}
				// Update the length with the passed-over characters
				cuWordLength += chars;
			}
			else {
				// Back up to where we were for next iteration
				utext_setNativeIndex(text, current+cuWordLength);
			}
		}
		// Never stop before a combining mark.
		int32_t currPos;
		while((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
			utext_next32(text);
			cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
		}
		// Look ahead for possible suffixes if a dictionary word does not follow.
		// We do this in code rather than using a rule so that the heuristic
		// resynch continues to function. For example, one of the suffix characters
		// could be a typo in the middle of a word.
		if((int32_t)utext_getNativeIndex(text) < rangeEnd && cuWordLength > 0) {
			if(words[wordsFound%THAI_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0 && fSuffixSet.contains(uc = utext_current32(text))) {
				if(uc == THAI_PAIYANNOI) {
					if(!fSuffixSet.contains(utext_previous32(text))) {
						// Skip over previous end and PAIYANNOI
						utext_next32(text);
						int32_t paiyannoiIndex = (int32_t)utext_getNativeIndex(text);
						utext_next32(text);
						cuWordLength += (int32_t)utext_getNativeIndex(text) - paiyannoiIndex; // Add PAIYANNOI to word
						uc = utext_current32(text); // Fetch next character
					}
					else {
						utext_next32(text); // Restore prior position
					}
				}
				if(uc == THAI_MAIYAMOK) {
					if(utext_previous32(text) != THAI_MAIYAMOK) {
						// Skip over previous end and MAIYAMOK
						utext_next32(text);
						int32_t maiyamokIndex = (int32_t)utext_getNativeIndex(text);
						utext_next32(text);
						cuWordLength += (int32_t)utext_getNativeIndex(text) - maiyamokIndex; // Add MAIYAMOK to word
					}
					else {
						// Restore prior position
						utext_next32(text);
					}
				}
			}
			else {
				utext_setNativeIndex(text, current+cuWordLength);
			}
		}
		// Did we find a word on this iteration? If so, push it on the break stack
		if(cuWordLength > 0) {
			foundBreaks.push((current+cuWordLength), status);
		}
	}
	// Don't return a break for the end of the dictionary range if there is one there.
	if(foundBreaks.peeki() >= rangeEnd) {
		(void)foundBreaks.popi();
		wordsFound -= 1;
	}
	return wordsFound;
}
//
// LaoBreakEngine
//
static const int32_t LAO_LOOKAHEAD = 3; // How many words in a row are "good enough"?
static const int32_t LAO_ROOT_COMBINE_THRESHOLD = 3; // Will not combine a non-word with a preceding dictionary word longer than this
static const int32_t LAO_PREFIX_COMBINE_THRESHOLD = 3; // Will not combine a non-word that shares at least this much prefix with a dictionary word, with a preceding word
static const int32_t LAO_MIN_WORD = 2; // Minimum word size
static const int32_t LAO_MIN_WORD_SPAN = LAO_MIN_WORD * 2; // Minimum number of characters for two words

LaoBreakEngine::LaoBreakEngine(DictionaryMatcher * adoptDictionary, UErrorCode & status) : DictionaryBreakEngine(), fDictionary(adoptDictionary)
{
	UTRACE_ENTRY(UTRACE_UBRK_CREATE_BREAK_ENGINE);
	UTRACE_DATA1(UTRACE_INFO, "dictbe=%s", "Laoo");
	fLaoWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Laoo:]&[:LineBreak=SA:]]"), status);
	if(U_SUCCESS(status)) {
		setCharacters(fLaoWordSet);
	}
	fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Laoo:]&[:LineBreak=SA:]&[:M:]]"), status);
	fMarkSet.add(0x0020);
	fEndWordSet = fLaoWordSet;
	fEndWordSet.remove(0x0EC0, 0x0EC4); // prefix vowels
	fBeginWordSet.add(0x0E81, 0x0EAE);  // basic consonants (including holes for corresponding Thai characters)
	fBeginWordSet.add(0x0EDC, 0x0EDD);  // digraph consonants (no Thai equivalent)
	fBeginWordSet.add(0x0EC0, 0x0EC4);  // prefix vowels
	// Compact for caching.
	fMarkSet.compact();
	fEndWordSet.compact();
	fBeginWordSet.compact();
	UTRACE_EXIT_STATUS(status);
}

LaoBreakEngine::~LaoBreakEngine() 
{
	delete fDictionary;
}

int32_t LaoBreakEngine::divideUpDictionaryRange(UText * text, int32_t rangeStart, int32_t rangeEnd, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) 
		return 0;
	if((rangeEnd - rangeStart) < LAO_MIN_WORD_SPAN) {
		return 0; // Not enough characters for two words
	}
	uint32_t wordsFound = 0;
	int32_t cpWordLength = 0;
	int32_t cuWordLength = 0;
	int32_t current;
	PossibleWord words[LAO_LOOKAHEAD];
	utext_setNativeIndex(text, rangeStart);
	while(U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
		cuWordLength = 0;
		cpWordLength = 0;
		// Look for candidate words at the current position
		int32_t candidates = words[wordsFound%LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);

		// If we found exactly one, use that
		if(candidates == 1) {
			cuWordLength = words[wordsFound % LAO_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % LAO_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// If there was more than one, see which one can take us forward the most words
		else if(candidates > 1) {
			// If we're already at the end of the range, we're done
			if(utext_getNativeIndex(text) >= rangeEnd) {
				goto foundBest;
			}
			do {
				if(words[(wordsFound + 1) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
					// Followed by another dictionary word; mark first word as a good candidate
					words[wordsFound%LAO_LOOKAHEAD].markCurrent();
					// If we're already at the end of the range, we're done
					if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
						goto foundBest;
					}
					// See if any of the possible second words is followed by a third word
					do {
						// If we find a third word, stop right away
						if(words[(wordsFound + 2) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
							words[wordsFound % LAO_LOOKAHEAD].markCurrent();
							goto foundBest;
						}
					} while(words[(wordsFound + 1) % LAO_LOOKAHEAD].backUp(text));
				}
			} while(words[wordsFound % LAO_LOOKAHEAD].backUp(text));
foundBest:
			cuWordLength = words[wordsFound % LAO_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % LAO_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// We come here after having either found a word or not. We look ahead to the
		// next word. If it's not a dictionary word, we will combine it with the word we
		// just found (if there is one), but only if the preceding word does not exceed
		// the threshold.
		// The text iterator should now be positioned at the end of the word we found.
		if((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < LAO_ROOT_COMBINE_THRESHOLD) {
			// if it is a dictionary word, do nothing. If it isn't, then if there is
			// no preceding word, or the non-word shares less than the minimum threshold
			// of characters with a dictionary word, then scan to resynchronize
			if(words[wordsFound % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0 && (cuWordLength == 0 || words[wordsFound%LAO_LOOKAHEAD].longestPrefix() < LAO_PREFIX_COMBINE_THRESHOLD)) {
				// Look for a plausible word boundary
				int32_t remaining = rangeEnd - (current + cuWordLength);
				UChar32 pc;
				UChar32 uc;
				int32_t chars = 0;
				for(;;) {
					int32_t pcIndex = (int32_t)utext_getNativeIndex(text);
					pc = utext_next32(text);
					int32_t pcSize = (int32_t)utext_getNativeIndex(text) - pcIndex;
					chars += pcSize;
					remaining -= pcSize;
					if(remaining <= 0) {
						break;
					}
					uc = utext_current32(text);
					if(fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
						// Maybe. See if it's in the dictionary.
						// TODO: this looks iffy; compare with old code.
						int32_t num_candidates = words[(wordsFound + 1) % LAO_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
						utext_setNativeIndex(text, current + cuWordLength + chars);
						if(num_candidates > 0) {
							break;
						}
					}
				}
				// Bump the word count if there wasn't already one
				if(cuWordLength <= 0) {
					wordsFound += 1;
				}
				cuWordLength += chars; // Update the length with the passed-over characters
			}
			else {
				utext_setNativeIndex(text, current + cuWordLength); // Back up to where we were for next iteration
			}
		}
		// Never stop before a combining mark.
		int32_t currPos;
		while((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
			utext_next32(text);
			cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
		}
		// Look ahead for possible suffixes if a dictionary word does not follow.
		// We do this in code rather than using a rule so that the heuristic
		// resynch continues to function. For example, one of the suffix characters
		// could be a typo in the middle of a word.
		// NOT CURRENTLY APPLICABLE TO LAO

		// Did we find a word on this iteration? If so, push it on the break stack
		if(cuWordLength > 0) {
			foundBreaks.push((current+cuWordLength), status);
		}
	}
	// Don't return a break for the end of the dictionary range if there is one there.
	if(foundBreaks.peeki() >= rangeEnd) {
		(void)foundBreaks.popi();
		wordsFound -= 1;
	}
	return wordsFound;
}
// 
// BurmeseBreakEngine
// 
static const int32_t BURMESE_LOOKAHEAD = 3; // How many words in a row are "good enough"?
static const int32_t BURMESE_ROOT_COMBINE_THRESHOLD = 3; // Will not combine a non-word with a preceding dictionary word longer than this
static const int32_t BURMESE_PREFIX_COMBINE_THRESHOLD = 3; // Will not combine a non-word that shares at least this much prefix with a dictionary word, with a preceding word
static const int32_t BURMESE_MIN_WORD = 2; // Minimum word size
static const int32_t BURMESE_MIN_WORD_SPAN = BURMESE_MIN_WORD * 2; // Minimum number of characters for two words

BurmeseBreakEngine::BurmeseBreakEngine(DictionaryMatcher * adoptDictionary, UErrorCode & status) : DictionaryBreakEngine(), fDictionary(adoptDictionary)
{
	UTRACE_ENTRY(UTRACE_UBRK_CREATE_BREAK_ENGINE);
	UTRACE_DATA1(UTRACE_INFO, "dictbe=%s", "Mymr");
	fBurmeseWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Mymr:]&[:LineBreak=SA:]]"), status);
	if(U_SUCCESS(status)) {
		setCharacters(fBurmeseWordSet);
	}
	fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Mymr:]&[:LineBreak=SA:]&[:M:]]"), status);
	fMarkSet.add(0x0020);
	fEndWordSet = fBurmeseWordSet;
	fBeginWordSet.add(0x1000, 0x102A);  // basic consonants and independent vowels

	// Compact for caching.
	fMarkSet.compact();
	fEndWordSet.compact();
	fBeginWordSet.compact();
	UTRACE_EXIT_STATUS(status);
}

BurmeseBreakEngine::~BurmeseBreakEngine() 
{
	delete fDictionary;
}

int32_t BurmeseBreakEngine::divideUpDictionaryRange(UText * text, int32_t rangeStart, int32_t rangeEnd, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) return 0;
	if((rangeEnd - rangeStart) < BURMESE_MIN_WORD_SPAN) {
		return 0; // Not enough characters for two words
	}
	uint32_t wordsFound = 0;
	int32_t cpWordLength = 0;
	int32_t cuWordLength = 0;
	int32_t current;
	PossibleWord words[BURMESE_LOOKAHEAD];
	utext_setNativeIndex(text, rangeStart);
	while(U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
		cuWordLength = 0;
		cpWordLength = 0;
		// Look for candidate words at the current position
		int32_t candidates = words[wordsFound%BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
		// If we found exactly one, use that
		if(candidates == 1) {
			cuWordLength = words[wordsFound % BURMESE_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % BURMESE_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// If there was more than one, see which one can take us forward the most words
		else if(candidates > 1) {
			// If we're already at the end of the range, we're done
			if(utext_getNativeIndex(text) >= rangeEnd) {
				goto foundBest;
			}
			do {
				if(words[(wordsFound + 1) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
					// Followed by another dictionary word; mark first word as a good candidate
					words[wordsFound%BURMESE_LOOKAHEAD].markCurrent();
					// If we're already at the end of the range, we're done
					if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
						goto foundBest;
					}
					// See if any of the possible second words is followed by a third word
					do {
						// If we find a third word, stop right away
						if(words[(wordsFound + 2) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
							words[wordsFound % BURMESE_LOOKAHEAD].markCurrent();
							goto foundBest;
						}
					} while(words[(wordsFound + 1) % BURMESE_LOOKAHEAD].backUp(text));
				}
			} while(words[wordsFound % BURMESE_LOOKAHEAD].backUp(text));
foundBest:
			cuWordLength = words[wordsFound % BURMESE_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % BURMESE_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// We come here after having either found a word or not. We look ahead to the
		// next word. If it's not a dictionary word, we will combine it with the word we
		// just found (if there is one), but only if the preceding word does not exceed
		// the threshold.
		// The text iterator should now be positioned at the end of the word we found.
		if((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < BURMESE_ROOT_COMBINE_THRESHOLD) {
			// if it is a dictionary word, do nothing. If it isn't, then if there is
			// no preceding word, or the non-word shares less than the minimum threshold
			// of characters with a dictionary word, then scan to resynchronize
			if(words[wordsFound % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0 && (cuWordLength == 0 || words[wordsFound%BURMESE_LOOKAHEAD].longestPrefix() < BURMESE_PREFIX_COMBINE_THRESHOLD)) {
				// Look for a plausible word boundary
				int32_t remaining = rangeEnd - (current + cuWordLength);
				UChar32 pc;
				UChar32 uc;
				int32_t chars = 0;
				for(;;) {
					int32_t pcIndex = (int32_t)utext_getNativeIndex(text);
					pc = utext_next32(text);
					int32_t pcSize = (int32_t)utext_getNativeIndex(text) - pcIndex;
					chars += pcSize;
					remaining -= pcSize;
					if(remaining <= 0) {
						break;
					}
					uc = utext_current32(text);
					if(fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
						// Maybe. See if it's in the dictionary.
						// TODO: this looks iffy; compare with old code.
						int32_t num_candidates = words[(wordsFound + 1) % BURMESE_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
						utext_setNativeIndex(text, current + cuWordLength + chars);
						if(num_candidates > 0) {
							break;
						}
					}
				}
				// Bump the word count if there wasn't already one
				if(cuWordLength <= 0) {
					wordsFound += 1;
				}
				cuWordLength += chars; // Update the length with the passed-over characters
			}
			else {
				utext_setNativeIndex(text, current + cuWordLength); // Back up to where we were for next iteration
			}
		}
		// Never stop before a combining mark.
		int32_t currPos;
		while((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
			utext_next32(text);
			cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
		}
		// Look ahead for possible suffixes if a dictionary word does not follow.
		// We do this in code rather than using a rule so that the heuristic
		// resynch continues to function. For example, one of the suffix characters
		// could be a typo in the middle of a word.
		// NOT CURRENTLY APPLICABLE TO BURMESE

		// Did we find a word on this iteration? If so, push it on the break stack
		if(cuWordLength > 0) {
			foundBreaks.push((current+cuWordLength), status);
		}
	}
	// Don't return a break for the end of the dictionary range if there is one there.
	if(foundBreaks.peeki() >= rangeEnd) {
		(void)foundBreaks.popi();
		wordsFound -= 1;
	}
	return wordsFound;
}
// 
// KhmerBreakEngine
// 
static const int32_t KHMER_LOOKAHEAD = 3; // How many words in a row are "good enough"?
static const int32_t KHMER_ROOT_COMBINE_THRESHOLD = 3; // Will not combine a non-word with a preceding dictionary word longer than this
static const int32_t KHMER_PREFIX_COMBINE_THRESHOLD = 3; // Will not combine a non-word that shares at least this much prefix with a dictionary word, with a preceding word
static const int32_t KHMER_MIN_WORD = 2; // Minimum word size
static const int32_t KHMER_MIN_WORD_SPAN = KHMER_MIN_WORD * 2; // Minimum number of characters for two words

KhmerBreakEngine::KhmerBreakEngine(DictionaryMatcher * adoptDictionary, UErrorCode & status) : DictionaryBreakEngine(), fDictionary(adoptDictionary)
{
	UTRACE_ENTRY(UTRACE_UBRK_CREATE_BREAK_ENGINE);
	UTRACE_DATA1(UTRACE_INFO, "dictbe=%s", "Khmr");
	fKhmerWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Khmr:]&[:LineBreak=SA:]]"), status);
	if(U_SUCCESS(status)) {
		setCharacters(fKhmerWordSet);
	}
	fMarkSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Khmr:]&[:LineBreak=SA:]&[:M:]]"), status);
	fMarkSet.add(0x0020);
	fEndWordSet = fKhmerWordSet;
	fBeginWordSet.add(0x1780, 0x17B3);
	//fBeginWordSet.add(0x17A3, 0x17A4);      // deprecated vowels
	//fEndWordSet.remove(0x17A5, 0x17A9);     // Khmer independent vowels that can't end a word
	//fEndWordSet.remove(0x17B2);             // Khmer independent vowel that can't end a word
	fEndWordSet.remove(0x17D2);         // KHMER SIGN COENG that combines some following characters
	//fEndWordSet.remove(0x17B6, 0x17C5);     // Remove dependent vowels
//    fEndWordSet.remove(0x0E31);             // MAI HAN-AKAT
//    fEndWordSet.remove(0x0E40, 0x0E44);     // SARA E through SARA AI MAIMALAI
//    fBeginWordSet.add(0x0E01, 0x0E2E);      // KO KAI through HO NOKHUK
//    fBeginWordSet.add(0x0E40, 0x0E44);      // SARA E through SARA AI MAIMALAI
//    fSuffixSet.add(THAI_PAIYANNOI);
//    fSuffixSet.add(THAI_MAIYAMOK);

	// Compact for caching.
	fMarkSet.compact();
	fEndWordSet.compact();
	fBeginWordSet.compact();
//    fSuffixSet.compact();
	UTRACE_EXIT_STATUS(status);
}

KhmerBreakEngine::~KhmerBreakEngine() 
{
	delete fDictionary;
}

int32_t KhmerBreakEngine::divideUpDictionaryRange(UText * text, int32_t rangeStart, int32_t rangeEnd, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) 
		return 0;
	if((rangeEnd - rangeStart) < KHMER_MIN_WORD_SPAN) {
		return 0; // Not enough characters for two words
	}
	uint32_t wordsFound = 0;
	int32_t cpWordLength = 0;
	int32_t cuWordLength = 0;
	int32_t current;
	PossibleWord words[KHMER_LOOKAHEAD];
	utext_setNativeIndex(text, rangeStart);
	while(U_SUCCESS(status) && (current = (int32_t)utext_getNativeIndex(text)) < rangeEnd) {
		cuWordLength = 0;
		cpWordLength = 0;
		// Look for candidate words at the current position
		int32_t candidates = words[wordsFound%KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
		// If we found exactly one, use that
		if(candidates == 1) {
			cuWordLength = words[wordsFound % KHMER_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % KHMER_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}
		// If there was more than one, see which one can take us forward the most words
		else if(candidates > 1) {
			// If we're already at the end of the range, we're done
			if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
				goto foundBest;
			}
			do {
				if(words[(wordsFound + 1) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) > 0) {
					// Followed by another dictionary word; mark first word as a good candidate
					words[wordsFound % KHMER_LOOKAHEAD].markCurrent();

					// If we're already at the end of the range, we're done
					if((int32_t)utext_getNativeIndex(text) >= rangeEnd) {
						goto foundBest;
					}

					// See if any of the possible second words is followed by a third word
					do {
						// If we find a third word, stop right away
						if(words[(wordsFound + 2) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd)) {
							words[wordsFound % KHMER_LOOKAHEAD].markCurrent();
							goto foundBest;
						}
					}
					while(words[(wordsFound + 1) % KHMER_LOOKAHEAD].backUp(text));
				}
			}
			while(words[wordsFound % KHMER_LOOKAHEAD].backUp(text));
foundBest:
			cuWordLength = words[wordsFound % KHMER_LOOKAHEAD].acceptMarked(text);
			cpWordLength = words[wordsFound % KHMER_LOOKAHEAD].markedCPLength();
			wordsFound += 1;
		}

		// We come here after having either found a word or not. We look ahead to the
		// next word. If it's not a dictionary word, we will combine it with the word we
		// just found (if there is one), but only if the preceding word does not exceed
		// the threshold.
		// The text iterator should now be positioned at the end of the word we found.
		if((int32_t)utext_getNativeIndex(text) < rangeEnd && cpWordLength < KHMER_ROOT_COMBINE_THRESHOLD) {
			// if it is a dictionary word, do nothing. If it isn't, then if there is
			// no preceding word, or the non-word shares less than the minimum threshold
			// of characters with a dictionary word, then scan to resynchronize
			if(words[wordsFound % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
			 && (cuWordLength == 0
			 || words[wordsFound % KHMER_LOOKAHEAD].longestPrefix() < KHMER_PREFIX_COMBINE_THRESHOLD)) {
				// Look for a plausible word boundary
				int32_t remaining = rangeEnd - (current+cuWordLength);
				UChar32 pc;
				UChar32 uc;
				int32_t chars = 0;
				for(;;) {
					int32_t pcIndex = (int32_t)utext_getNativeIndex(text);
					pc = utext_next32(text);
					int32_t pcSize = (int32_t)utext_getNativeIndex(text) - pcIndex;
					chars += pcSize;
					remaining -= pcSize;
					if(remaining <= 0) {
						break;
					}
					uc = utext_current32(text);
					if(fEndWordSet.contains(pc) && fBeginWordSet.contains(uc)) {
						// Maybe. See if it's in the dictionary.
						int32_t num_candidates = words[(wordsFound + 1) % KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd);
						utext_setNativeIndex(text, current+cuWordLength+chars);
						if(num_candidates > 0) {
							break;
						}
					}
				}

				// Bump the word count if there wasn't already one
				if(cuWordLength <= 0) {
					wordsFound += 1;
				}

				// Update the length with the passed-over characters
				cuWordLength += chars;
			}
			else {
				// Back up to where we were for next iteration
				utext_setNativeIndex(text, current+cuWordLength);
			}
		}

		// Never stop before a combining mark.
		int32_t currPos;
		while((currPos = (int32_t)utext_getNativeIndex(text)) < rangeEnd && fMarkSet.contains(utext_current32(text))) {
			utext_next32(text);
			cuWordLength += (int32_t)utext_getNativeIndex(text) - currPos;
		}

		// Look ahead for possible suffixes if a dictionary word does not follow.
		// We do this in code rather than using a rule so that the heuristic
		// resynch continues to function. For example, one of the suffix characters
		// could be a typo in the middle of a word.
//        if((int32_t)utext_getNativeIndex(text) < rangeEnd && wordLength > 0) {
//            if(words[wordsFound%KHMER_LOOKAHEAD].candidates(text, fDictionary, rangeEnd) <= 0
//    && fSuffixSet.contains(uc = utext_current32(text))) {
//                if(uc == KHMER_PAIYANNOI) {
//                    if(!fSuffixSet.contains(utext_previous32(text))) {
//                        // Skip over previous end and PAIYANNOI
//                        utext_next32(text);
//                        utext_next32(text);
//                        wordLength += 1;            // Add PAIYANNOI to word
//                        uc = utext_current32(text);     // Fetch next character
//                    }
//                    else {
//                        // Restore prior position
//                        utext_next32(text);
//                    }
//                }
//                if(uc == KHMER_MAIYAMOK) {
//                    if(utext_previous32(text) != KHMER_MAIYAMOK) {
//                        // Skip over previous end and MAIYAMOK
//                        utext_next32(text);
//                        utext_next32(text);
//                        wordLength += 1;            // Add MAIYAMOK to word
//                    }
//                    else {
//                        // Restore prior position
//                        utext_next32(text);
//                    }
//                }
//            }
//            else {
//                utext_setNativeIndex(text, current+wordLength);
//            }
//        }

		// Did we find a word on this iteration? If so, push it on the break stack
		if(cuWordLength > 0) {
			foundBreaks.push((current+cuWordLength), status);
		}
	}
	// Don't return a break for the end of the dictionary range if there is one there.
	if(foundBreaks.peeki() >= rangeEnd) {
		(void)foundBreaks.popi();
		wordsFound -= 1;
	}
	return wordsFound;
}

#if !UCONFIG_NO_NORMALIZATION
// 
// CjkBreakEngine
// 
static const uint32_t kuint32max = 0xFFFFFFFF;

CjkBreakEngine::CjkBreakEngine(DictionaryMatcher * adoptDictionary, LanguageType type, UErrorCode & status) : DictionaryBreakEngine(), fDictionary(adoptDictionary) 
{
	UTRACE_ENTRY(UTRACE_UBRK_CREATE_BREAK_ENGINE);
	UTRACE_DATA1(UTRACE_INFO, "dictbe=%s", "Hani");
	// Korean dictionary only includes Hangul syllables
	fHangulWordSet.applyPattern(UNICODE_STRING_SIMPLE("[\\uac00-\\ud7a3]"), status);
	fHanWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Han:]"), status);
	fKatakanaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[[:Katakana:]\\uff9e\\uff9f]"), status);
	fHiraganaWordSet.applyPattern(UNICODE_STRING_SIMPLE("[:Hiragana:]"), status);
	nfkcNorm2 = Normalizer2::getNFKCInstance(status);

	if(U_SUCCESS(status)) {
		// handle Korean and Japanese/Chinese using different dictionaries
		if(type == kKorean) {
			setCharacters(fHangulWordSet);
		}
		else { //Chinese and Japanese
			UnicodeSet cjSet;
			cjSet.addAll(fHanWordSet);
			cjSet.addAll(fKatakanaWordSet);
			cjSet.addAll(fHiraganaWordSet);
			cjSet.add(0xFF70); // HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK
			cjSet.add(0x30FC); // KATAKANA-HIRAGANA PROLONGED SOUND MARK
			setCharacters(cjSet);
		}
	}
	UTRACE_EXIT_STATUS(status);
}

CjkBreakEngine::~CjkBreakEngine() 
{
	delete fDictionary;
}

// The katakanaCost values below are based on the length frequencies of all
// katakana phrases in the dictionary
static const int32_t kMaxKatakanaLength = 8;
static const int32_t kMaxKatakanaGroupLength = 20;
static const uint32_t maxSnlp = 255;

static inline uint32_t getKatakanaCost(int32_t wordLength) 
{
	//TODO: fill array with actual values from dictionary!
	static const uint32_t katakanaCost[kMaxKatakanaLength + 1] = {8192, 984, 408, 240, 204, 252, 300, 372, 480};
	return (wordLength > kMaxKatakanaLength) ? 8192 : katakanaCost[wordLength];
}

static inline bool isKatakana(UChar32 value) 
{
	return (value >= 0x30A1 && value <= 0x30FE && value != 0x30FB) || (value >= 0xFF66 && value <= 0xFF9f);
}

// Function for accessing internal utext flags.
//   Replicates an internal UText function.
static inline int32_t utext_i32_flag(int32_t bitIndex) { return (int32_t)1 << bitIndex; }

/*
 * @param text A UText representing the text
 * @param rangeStart The start of the range of dictionary characters
 * @param rangeEnd The end of the range of dictionary characters
 * @param foundBreaks vector <int32> to receive the break positions
 * @return The number of breaks found
 */
int32_t CjkBreakEngine::divideUpDictionaryRange(UText * inText, int32_t rangeStart, int32_t rangeEnd, UVector32 & foundBreaks, UErrorCode & status) const 
{
	if(U_FAILURE(status)) return 0;
	if(rangeStart >= rangeEnd) {
		return 0;
	}
	// UnicodeString version of input UText, NFKC normalized if necessary.
	UnicodeString inString;
	// inputMap[inStringIndex] = corresponding native index from UText inText.
	// If NULL then mapping is 1:1
	LocalPointer<UVector32>     inputMap;

	// if UText has the input string as one contiguous UTF-16 chunk
	if((inText->providerProperties & utext_i32_flag(UTEXT_PROVIDER_STABLE_CHUNKS)) && inText->chunkNativeStart <= rangeStart &&
	    inText->chunkNativeLimit >= rangeEnd && inText->nativeIndexingLimit >= rangeEnd - inText->chunkNativeStart) {
		// Input UText is in one contiguous UTF-16 chunk.
		// Use Read-only aliasing UnicodeString.
		inString.setTo(FALSE, inText->chunkContents + rangeStart - inText->chunkNativeStart, rangeEnd - rangeStart);
	}
	else {
		// Copy the text from the original inText (UText) to inString (UnicodeString).
		// Create a map from UnicodeString indices -> UText offsets.
		utext_setNativeIndex(inText, rangeStart);
		int32_t limit = rangeEnd;
		assert(limit <= utext_nativeLength(inText));
		if(limit > utext_nativeLength(inText)) {
			limit = (int32_t)utext_nativeLength(inText);
		}
		inputMap.adoptInsteadAndCheckErrorCode(new UVector32(status), status);
		if(U_FAILURE(status)) {
			return 0;
		}
		while(utext_getNativeIndex(inText) < limit) {
			int32_t nativePosition = (int32_t)utext_getNativeIndex(inText);
			UChar32 c = utext_next32(inText);
			assert(c != U_SENTINEL);
			inString.append(c);
			while(inputMap->size() < inString.length()) {
				inputMap->addElement(nativePosition, status);
			}
		}
		inputMap->addElement(limit, status);
	}
	if(!nfkcNorm2->isNormalized(inString, status)) {
		UnicodeString normalizedInput;
		//  normalizedMap[normalizedInput position] ==  original UText position.
		LocalPointer<UVector32> normalizedMap(new UVector32(status), status);
		if(U_FAILURE(status)) {
			return 0;
		}
		UnicodeString fragment;
		UnicodeString normalizedFragment;
		for(int32_t srcI = 0; srcI < inString.length();) { // Once per normalization chunk
			fragment.remove();
			int32_t fragmentStartI = srcI;
			UChar32 c = inString.char32At(srcI);
			for(;;) {
				fragment.append(c);
				srcI = inString.moveIndex32(srcI, 1);
				if(srcI == inString.length()) {
					break;
				}
				c = inString.char32At(srcI);
				if(nfkcNorm2->hasBoundaryBefore(c)) {
					break;
				}
			}
			nfkcNorm2->normalize(fragment, normalizedFragment, status);
			normalizedInput.append(normalizedFragment);

			// Map every position in the normalized chunk to the start of the chunk
			//   in the original input.
			int32_t fragmentOriginalStart = inputMap.isValid() ? inputMap->elementAti(fragmentStartI) : fragmentStartI+rangeStart;
			while(normalizedMap->size() < normalizedInput.length()) {
				normalizedMap->addElement(fragmentOriginalStart, status);
				if(U_FAILURE(status)) {
					break;
				}
			}
		}
		assert(normalizedMap->size() == normalizedInput.length());
		int32_t nativeEnd = inputMap.isValid() ? inputMap->elementAti(inString.length()) : inString.length()+rangeStart;
		normalizedMap->addElement(nativeEnd, status);
		inputMap = std::move(normalizedMap);
		inString = std::move(normalizedInput);
	}
	int32_t numCodePts = inString.countChar32();
	if(numCodePts != inString.length()) {
		// There are supplementary characters in the input.
		// The dictionary will produce boundary positions in terms of code point indexes,
		//   not in terms of code unit string indexes.
		// Use the inputMap mechanism to take care of this in addition to indexing differences
		//    from normalization and/or UTF-8 input.
		bool hadExistingMap = inputMap.isValid();
		if(!hadExistingMap) {
			inputMap.adoptInsteadAndCheckErrorCode(new UVector32(status), status);
			if(U_FAILURE(status)) {
				return 0;
			}
		}
		int32_t cpIdx = 0;
		for(int32_t cuIdx = 0;; cuIdx = inString.moveIndex32(cuIdx, 1)) {
			assert(cuIdx >= cpIdx);
			if(hadExistingMap) {
				inputMap->setElementAt(inputMap->elementAti(cuIdx), cpIdx);
			}
			else {
				inputMap->addElement(cuIdx+rangeStart, status);
			}
			cpIdx++;
			if(cuIdx == inString.length()) {
				break;
			}
		}
	}
	// bestSnlp[i] is the snlp of the best segmentation of the first i
	// code points in the range to be matched.
	UVector32 bestSnlp(numCodePts + 1, status);
	bestSnlp.addElement(0, status);
	for(int32_t i = 1; i <= numCodePts; i++) {
		bestSnlp.addElement(kuint32max, status);
	}
	// prev[i] is the index of the last CJK code point in the previous word in
	// the best segmentation of the first i characters.
	UVector32 prev(numCodePts + 1, status);
	for(int32_t i = 0; i <= numCodePts; i++) {
		prev.addElement(-1, status);
	}
	const int32_t maxWordSize = 20;
	UVector32 values(numCodePts, status);
	values.setSize(numCodePts);
	UVector32 lengths(numCodePts, status);
	lengths.setSize(numCodePts);

	UText fu = UTEXT_INITIALIZER;
	utext_openUnicodeString(&fu, &inString, &status);

	// Dynamic programming to find the best segmentation.

	// In outer loop, i  is the code point index,
	//                ix is the corresponding string (code unit) index.
	//    They differ when the string contains supplementary characters.
	int32_t ix = 0;
	bool is_prev_katakana = false;
	for(int32_t i = 0; i < numCodePts; ++i, ix = inString.moveIndex32(ix, 1)) {
		if((uint32_t)bestSnlp.elementAti(i) == kuint32max) {
			continue;
		}

		int32_t count;
		utext_setNativeIndex(&fu, ix);
		count = fDictionary->matches(&fu, maxWordSize, numCodePts,
			NULL, lengths.getBuffer(), values.getBuffer(), NULL);
		// Note: lengths is filled with code point lengths
		//       The NULL parameter is the ignored code unit lengths.

		// if there are no single character matches found in the dictionary
		// starting with this character, treat character as a 1-character word
		// with the highest value possible, i.e. the least likely to occur.
		// Exclude Korean characters from this treatment, as they should be left
		// together by default.
		if((count == 0 || lengths.elementAti(0) != 1) &&
		    !fHangulWordSet.contains(inString.char32At(ix))) {
			values.setElementAt(maxSnlp, count); // 255
			lengths.setElementAt(1, count++);
		}

		for(int32_t j = 0; j < count; j++) {
			uint32_t newSnlp = (uint32_t)bestSnlp.elementAti(i) + (uint32_t)values.elementAti(j);
			int32_t ln_j_i = lengths.elementAti(j) + i;
			if(newSnlp < (uint32_t)bestSnlp.elementAti(ln_j_i)) {
				bestSnlp.setElementAt(newSnlp, ln_j_i);
				prev.setElementAt(i, ln_j_i);
			}
		}

		// In Japanese,
		// Katakana word in single character is pretty rare. So we apply
		// the following heuristic to Katakana: any continuous run of Katakana
		// characters is considered a candidate word with a default cost
		// specified in the katakanaCost table according to its length.

		bool is_katakana = isKatakana(inString.char32At(ix));
		int32_t katakanaRunLength = 1;
		if(!is_prev_katakana && is_katakana) {
			int32_t j = inString.moveIndex32(ix, 1);
			// Find the end of the continuous run of Katakana characters
			while(j < inString.length() && katakanaRunLength < kMaxKatakanaGroupLength &&
			    isKatakana(inString.char32At(j))) {
				j = inString.moveIndex32(j, 1);
				katakanaRunLength++;
			}
			if(katakanaRunLength < kMaxKatakanaGroupLength) {
				uint32_t newSnlp = bestSnlp.elementAti(i) + getKatakanaCost(katakanaRunLength);
				if(newSnlp < (uint32_t)bestSnlp.elementAti(i+katakanaRunLength)) {
					bestSnlp.setElementAt(newSnlp, i+katakanaRunLength);
					prev.setElementAt(i, i+katakanaRunLength); // prev[j] = i;
				}
			}
		}
		is_prev_katakana = is_katakana;
	}
	utext_close(&fu);

	// Start pushing the optimal offset index into t_boundary (t for tentative).
	// prev[numCodePts] is guaranteed to be meaningful.
	// We'll first push in the reverse order, i.e.,
	// t_boundary[0] = numCodePts, and afterwards do a swap.
	UVector32 t_boundary(numCodePts+1, status);

	int32_t numBreaks = 0;
	// No segmentation found, set boundary to end of range
	if((uint32_t)bestSnlp.elementAti(numCodePts) == kuint32max) {
		t_boundary.addElement(numCodePts, status);
		numBreaks++;
	}
	else {
		for(int32_t i = numCodePts; i > 0; i = prev.elementAti(i)) {
			t_boundary.addElement(i, status);
			numBreaks++;
		}
		assert(prev.elementAti(t_boundary.elementAti(numBreaks - 1)) == 0);
	}

	// Add a break for the start of the dictionary range if there is not one
	// there already.
	if(foundBreaks.size() == 0 || foundBreaks.peeki() < rangeStart) {
		t_boundary.addElement(0, status);
		numBreaks++;
	}

	// Now that we're done, convert positions in t_boundary[] (indices in
	// the normalized input string) back to indices in the original input UText
	// while reversing t_boundary and pushing values to foundBreaks.
	int32_t prevCPPos = -1;
	int32_t prevUTextPos = -1;
	for(int32_t i = numBreaks-1; i >= 0; i--) {
		int32_t cpPos = t_boundary.elementAti(i);
		assert(cpPos > prevCPPos);
		int32_t utextPos =  inputMap.isValid() ? inputMap->elementAti(cpPos) : cpPos + rangeStart;
		assert(utextPos >= prevUTextPos);
		if(utextPos > prevUTextPos) {
			// Boundaries are added to foundBreaks output in ascending order.
			assert(foundBreaks.size() == 0 || foundBreaks.peeki() < utextPos);
			foundBreaks.push(utextPos, status);
		}
		else {
			// Normalization expanded the input text, the dictionary found a boundary
			// within the expansion, giving two boundaries with the same index in the
			// original text. Ignore the second. See ticket #12918.
			--numBreaks;
		}
		prevCPPos = cpPos;
		prevUTextPos = utextPos;
	}
	(void)prevCPPos; // suppress compiler warnings about unused variable
	// inString goes out of scope
	// inputMap goes out of scope
	return numBreaks;
}

#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
