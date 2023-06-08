// 
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
// The contents of this file are subject to the Mozilla Public License Version
// 1.1 (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at http://www.mozilla.org/MPL/
// 
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the License.
// 
// The Original Code is Mozilla Universal charset detector code.
// 
// The Initial Developer of the Original Code is Netscape Communications Corporation.
// Portions created by the Initial Developer are Copyright (C) 2001
// the Initial Developer. All Rights Reserved.
// 
// Contributor(s): BYVoid <byvoid.kcp@gmail.com>
// 
// Alternatively, the contents of this file may be used under the terms of
// either the GNU General Public License Version 2 or later (the "GPL"), or
// the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
// in which case the provisions of the GPL or the LGPL are applicable instead
// of those above. If you wish to allow use of your version of this file only
// under the terms of either the GPL or the LGPL, and not to allow others to
// use your version of this file under the terms of the MPL, indicate your
// decision by deleting the provisions above and replace them with the notice
// and other provisions required by the GPL or the LGPL. If you do not delete
// the provisions above, a recipient may use your version of this file under
// the terms of any one of the MPL, the GPL or the LGPL.
// 
#include <slib-internal.h>
#pragma hdrstop
#include <uchardet.h>

#define nsnull               0
#define ENOUGH_REL_THRESHOLD 100
#define MAX_REL_THRESHOLD    1000
#define MINIMUM_THRESHOLD    0.20f
#define SHORTCUT_THRESHOLD   0.95f
#define SURE_YES             0.99f
#define SURE_NO              0.01f
#define DONT_KNOW            -1.0f
#define ONE_CHAR_PROB        0.50f
#define NUM_OF_PROBERS           7
#define NUM_OF_SBCS_PROBERS     35
#define ENOUGH_DATA_THRESHOLD 1024
#define MINIMUM_DATA_THRESHOLD   4

#define PCK16BITS(a, b)            ((uint32)(((b) << 16) | (a)))
#define PCK8BITS(a, b, c, d)         PCK16BITS(((uint32)(((b) << 8) | (a))), ((uint32)(((d) << 8) | (c))))
#define PCK4BITS(a, b, c, d, e, f, g, h) PCK8BITS(((uint32)(((b) << 4)|(a))), ((uint32)(((d) << 4)|(c))), ((uint32)(((f) << 4)|(e))), ((uint32)(((h) << 4)|(g))))
#define GETFROMPCK(i, c) (((((c).data)[(i)>>(c).idxsft])>>(((i)&(c).sftmsk)<<(c).bitsft))&(c).unitmsk)
//
//#include <nsPkgInt.h>
//
enum nsIdxSft {
	eIdxSft4bits  = 3,
	eIdxSft8bits  = 2,
	eIdxSft16bits = 1
};

enum nsSftMsk {
	eSftMsk4bits  = 7,
	eSftMsk8bits  = 3,
	eSftMsk16bits = 1
};

enum nsBitSft {
	eBitSft4bits  = 2,
	eBitSft8bits  = 3,
	eBitSft16bits = 4
};

enum nsUnitMsk {
	eUnitMsk4bits  = 0x0000000FL,
	eUnitMsk8bits  = 0x000000FFL,
	eUnitMsk16bits = 0x0000FFFFL
};

struct nsPkgInt {
	nsIdxSft idxsft;
	nsSftMsk sftmsk;
	nsBitSft bitsft;
	nsUnitMsk unitmsk;
	const uint32 * const data;
};
//
//
//
//#define DEBUG_chardet // Uncomment this for debug dump.
enum nsProbingState {
	eDetecting = 0, //We are still detecting, no sure answer yet, but caller can ask for confidence.
	eFoundIt = 1, //That's a positive answer
	eNotMe = 2  //Negative answer
};

class nsCharSetProber {
public:
	nsCharSetProber() : mState(eDetecting)
	{
	}
	virtual ~nsCharSetProber() 
	{
	}
	virtual const char * GetCharSetName() = 0;
	virtual nsProbingState HandleData(const char * aBuf, uint32 aLen) = 0;
	virtual nsProbingState GetState() const { return mState; }
	virtual void  Reset()  = 0;
	virtual float GetConfidence() const = 0;
#ifdef DEBUG_chardet
	virtual void  DumpStatus() 
	{
	}
#endif
	// Helper functions used in the Latin1 and Group probers.
	// both functions Allocate a new buffer for newBuf. This buffer should be
	// freed by the caller using PR_FREEIF.
	// Both functions return false in case of memory allocation failure.
	static bool FilterWithoutEnglishLetters(const char * aBuf, uint32 aLen, char ** newBuf, uint32& newLen);
	static bool FilterWithEnglishLetters(const char * aBuf, uint32 aLen, char ** newBuf, uint32& newLen);
protected:
	class JapaneseContextAnalysis {
	public:
		JapaneseContextAnalysis() 
		{
			Reset(false);
		}
		void   HandleData(const char * aBuf, uint32 aLen);
		void   HandleOneChar(const char * aStr, uint32 aCharLen);
		float  GetConfidence() const;
		void   Reset(bool aIsPreferredLanguage);
		bool   GotEnoughData() const { return (mTotalRel > ENOUGH_REL_THRESHOLD); }
	protected:
		virtual int32 GetOrder(const char * str, uint32 * charLen) const = 0;
		virtual int32 GetOrder(const char * str) const = 0;
		uint32 mRelSample[6]; //category counters, each integer counts sequences in its category
		uint32 mTotalRel; //total sequence received
		uint32 mDataThreshold; //Number of sequences needed to trigger detection
		int32  mLastCharOrder; //The order of previous char
		//if last byte in current buffer is not the last byte of a character, we
		//need to know how many byte to skip in next buffer.
		uint32 mNeedToSkipCharNum;
		bool   mDone; //If this flag is set to true, detection is done and conclusion has been made
	};
	nsProbingState mState;
};
//
//
//

//
// Codepoints 
//
#define ILL 255 /* Illegal codepoints.*/
#define CTR 254 /* Control character. */
#define SYM 253 /* Symbols and punctuation that does not belong to words. */
#define RET 252 /* Return/Line feeds. */
#define NUM 251 /* Numbers 0-9. */

#define SB_ENOUGH_REL_THRESHOLD  1024
#define POSITIVE_SHORTCUT_THRESHOLD  0.95f
#define NEGATIVE_SHORTCUT_THRESHOLD  0.05f
#define SYMBOL_CAT_ORDER  250

#define NUMBER_OF_SEQ_CAT 4
#define POSITIVE_CAT   (NUMBER_OF_SEQ_CAT-1)
#define PROBABLE_CAT   (NUMBER_OF_SEQ_CAT-2)
#define NEUTRAL_CAT    (NUMBER_OF_SEQ_CAT-3)
#define NEGATIVE_CAT   0

struct SequenceModel {
	const  uchar * const charToOrderMap; /* [256] table mapping codepoints to chararacter orders. */
	const  uint8 * const precedenceMatrix; /* freqCharCount x freqCharCount table of 2-char sequence's frequencies. */
	int    freqCharCount; /* The count of frequent characters. */
	float  mTypicalPositiveRatio; // = freqSeqs / totalSeqs
	bool   keepEnglishLetter;   // says if this script contains English characters (not implemented)
	const  char * const charsetName;
};

class nsSingleByteCharSetProber : public nsCharSetProber {
public:
	nsSingleByteCharSetProber(const SequenceModel * model);
	nsSingleByteCharSetProber(const SequenceModel * model, bool reversed, nsCharSetProber* nameProber);
	virtual const char * GetCharSetName();
	virtual nsProbingState HandleData(const char * aBuf, uint32 aLen);
	virtual void  Reset();
	virtual float GetConfidence() const;
	// This feature is not implemented yet. any current language model
	// contain this parameter as false. No one is looking at this
	// parameter or calling this method.
	// Moreover, the nsSBCSGroupProber which calls the HandleData of this
	// prober has a hard-coded call to FilterWithoutEnglishLetters which gets rid
	// of the English letters.
	bool KeepEnglishLetters() 
	{
		return mModel->keepEnglishLetter;
	} // (not implemented)
#ifdef DEBUG_chardet
	virtual void  DumpStatus();
#endif
protected:
	const SequenceModel * const mModel;
	const bool mReversed; // true if we need to reverse every pair in the model lookup
	//char order of last character
	uchar  mLastOrder;
	uint32 mTotalSeqs;
	uint32 mSeqCounters[NUMBER_OF_SEQ_CAT];
	uint32 mTotalChar;
	uint32 mCtrlChar;
	uint32 mFreqChar; //characters that fall in our sampling range
	nsCharSetProber* mNameProber; // Optional auxiliary prober for name decision. created and destroyed by the GroupProber
};
//
//#include <nsCodingStateMachine.h>
//
enum nsSMState {
	eStart = 0,
	eError = 1,
	eItsMe = 2
};

//state machine model
struct SMModel {
	nsPkgInt classTable;
	uint32 classFactor;
	nsPkgInt stateTable;
	const uint32 * charLenTable;
	const char * name;
};

class nsCodingStateMachine {
public:
	nsCodingStateMachine(const SMModel * sm);
	nsSMState NextState(char c);
	void   Reset() 
	{
		mCurrentState = eStart;
	}
	const  char * GetCodingStateMachine() const { return mModel->name; }
	uint32 GetCurrentCharLen() const { return mCurrentCharLen; }
protected:
	nsSMState mCurrentState;
	uint32 mCurrentCharLen;
	uint32 mCurrentBytePos;
	const SMModel * mModel;
};

extern const SMModel UTF8SMModel;
extern const SMModel Big5SMModel;
extern const SMModel EUCJPSMModel;
extern const SMModel EUCKRSMModel;
extern const SMModel EUCTWSMModel;
extern const SMModel GB18030SMModel;
extern const SMModel SJISSMModel;
extern const SMModel HZSMModel;
extern const SMModel ISO2022CNSMModel;
extern const SMModel ISO2022JPSMModel;
extern const SMModel ISO2022KRSMModel;
//
//#include <nsEscCharsetProber.h>
//
class nsEscCharSetProber : public nsCharSetProber {
public:
	nsEscCharSetProber(uint32 aLanguageFilter);
	virtual ~nsEscCharSetProber();
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return mDetectedCharset; }
	void   Reset();
	float  GetConfidence() const
	{
		return 0.99f;
	}
protected:
	//void   GetDistribution(uint32 aCharLen, const char * aStr);
	//#define NUM_OF_ESC_CHARSETS  4
	nsCodingStateMachine * mCodingSM[4];
	uint32 mActiveSM;
	const char * mDetectedCharset;
};
//
//#include <nsHebrewProber.h>
//
// This prober doesn't actually recognize a language or a charset.
// It is a helper prober for the use of the Hebrew model probers
class nsHebrewProber : public nsCharSetProber {
public:
	nsHebrewProber() : mLogicalProb(0), mVisualProb(0) 
	{
		Reset();
	}
	virtual ~nsHebrewProber() 
	{
	}
	virtual nsProbingState HandleData(const char * aBuf, uint32 aLen);
	virtual const char * GetCharSetName();
	virtual void Reset();
	virtual nsProbingState GetState() const;
	virtual float GetConfidence() const { return 0.0f; }
	void SetModelProbers(nsCharSetProber * logicalPrb, nsCharSetProber * visualPrb)
	{
		mLogicalProb = logicalPrb; 
		mVisualProb = visualPrb;
	}
#ifdef DEBUG_chardet
	virtual void  DumpStatus();
#endif
protected:
	static bool FASTCALL isFinal(char c);
	static bool FASTCALL isNonFinal(char c);
	int32  mFinalCharLogicalScore;
	int32  mFinalCharVisualScore;
	// The two last characters seen in the previous buffer.
	char   mPrev;
	char   mBeforePrev; 
	uint8  Reserve[2]; // @alignment
	// These probers are owned by the group prober.
	nsCharSetProber * mLogicalProb;
	nsCharSetProber * mVisualProb; 
};
/**
 * ** General ideas of the Hebrew charset recognition **
 *
 * Four main charsets exist in Hebrew:
 * "ISO-8859-8" - Visual Hebrew
 * "windows-1255" - Logical Hebrew
 * "ISO-8859-8-I" - Logical Hebrew
 * "x-mac-hebrew" - ?? Logical Hebrew ??
 *
 * Both "ISO" charsets use a completely identical set of code points, whereas
 * "windows-1255" and "x-mac-hebrew" are two different proper supersets of
 * these code points. windows-1255 defines additional characters in the range
 * 0x80-0x9F as some misc punctuation marks as well as some Hebrew-specific
 * diacritics and additional 'Yiddish' ligature letters in the range 0xc0-0xd6.
 * x-mac-hebrew defines similar additional code points but with a different
 * mapping.
 *
 * As far as an average Hebrew text with no diacritics is concerned, all four
 * charsets are identical with respect to code points. Meaning that for the
 * main Hebrew alphabet, all four map the same values to all 27 Hebrew letters
 * (including final letters).
 *
 * The dominant difference between these charsets is their directionality.
 * "Visual" directionality means that the text is ordered as if the renderer is
 * not aware of a BIDI rendering algorithm. The renderer sees the text and
 * draws it from left to right. The text itself when ordered naturally is read
 * backwards. A buffer of Visual Hebrew generally looks like so:
 * "[last word of first line spelled backwards] [whole line ordered backwards
 * and spelled backwards] [first word of first line spelled backwards]
 * [end of line] [last word of second line] ... etc' "
 * adding punctuation marks, numbers and English text to visual text is
 * naturally also "visual" and from left to right.
 *
 * "Logical" directionality means the text is ordered "naturally" according to
 * the order it is read. It is the responsibility of the renderer to display
 * the text from right to left. A BIDI algorithm is used to place general
 * punctuation marks, numbers and English text in the text.
 *
 * Texts in x-mac-hebrew are almost impossible to find on the Internet. From
 * what little evidence I could find, it seems that its general directionality is Logical.
 *
 * To sum up all of the above, the Hebrew probing mechanism knows about two
 * charsets:
 * Visual Hebrew - "ISO-8859-8" - backwards text - Words and sentences are
 *  backwards while line order is natural. For charset recognition purposes
 *  the line order is unimportant (In fact, for this implementation, even
 *  word order is unimportant).
 * Logical Hebrew - "windows-1255" - normal, naturally ordered text.
 *
 * "ISO-8859-8-I" is a subset of windows-1255 and doesn't need to be
 *  specifically identified.
 * "x-mac-hebrew" is also identified as windows-1255. A text in x-mac-hebrew
 *  that contain special punctuation marks or diacritics is displayed with
 *  some unconverted characters showing as question marks. This problem might
 *  be corrected using another model prober for x-mac-hebrew. Due to the fact
 *  that x-mac-hebrew texts are so rare, writing another model prober isn't
 *  worth the effort and performance hit.
 *
 * *** The Prober ***
 *
 * The prober is divided between two nsSBCharSetProbers and an nsHebrewProber,
 * all of which are managed, created, fed data, inquired and deleted by the
 * nsSBCSGroupProber. The two nsSBCharSetProbers identify that the text is in
 * fact some kind of Hebrew, Logical or Visual. The final decision about which
 * one is it is made by the nsHebrewProber by combining final-letter scores
 * with the scores of the two nsSBCharSetProbers to produce a final answer.
 *
 * The nsSBCSGroupProber is responsible for stripping the original text of HTML
 * tags, English characters, numbers, low-ASCII punctuation characters, spaces
 * and new lines. It reduces any sequence of such characters to a single space.
 * The buffer fed to each prober in the SBCS group prober is pure text in
 * high-ASCII.
 * The two nsSBCharSetProbers (model probers) share the same language model:
 * Win1255Model.
 * The first nsSBCharSetProber uses the model normally as any other
 * nsSBCharSetProber does, to recognize windows-1255, upon which this model was
 * built. The second nsSBCharSetProber is told to make the pair-of-letter
 * lookup in the language model backwards. This in practice exactly simulates
 * a visual Hebrew model using the windows-1255 logical Hebrew model.
 *
 * The nsHebrewProber is not using any language model. All it does is look for
 * final-letter evidence suggesting the text is either logical Hebrew or visual
 * Hebrew. Disjointed from the model probers, the results of the nsHebrewProber
 * alone are meaningless. nsHebrewProber always returns 0.00 as confidence
 * since it never identifies a charset by itself. Instead, the pointer to the
 * nsHebrewProber is passed to the model probers as a helper "Name Prober".
 * When the Group prober receives a positive identification from any prober,
 * it asks for the name of the charset identified. If the prober queried is a
 * Hebrew model prober, the model prober forwards the call to the
 * nsHebrewProber to make the final decision. In the nsHebrewProber, the
 * decision is made according to the final-letters scores maintained and Both
 * model probers scores. The answer is returned in the form of the name of the
 * charset identified, either "windows-1255" or "ISO-8859-8".
 *
 */
//
//#include <nsLatin1Prober.h>
//
class nsLatin1Prober : public nsCharSetProber {
public:
	nsLatin1Prober()
	{
		Reset();
	}
	virtual ~nsLatin1Prober()
	{
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "WINDOWS-1252"; }
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	virtual void  DumpStatus();
#endif
protected:
	char   mLastCharClass;
	uint32 mFreqCounter[/*FREQ_CAT_NUM*/4];
};
//
//#include <nsMBCSGroupProber.h>
//
class nsMBCSGroupProber : public nsCharSetProber {
public:
	nsMBCSGroupProber(uint32 aLanguageFilter);
	virtual ~nsMBCSGroupProber();
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName();
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	void  DumpStatus();
#endif
#ifdef DEBUG_jgmyers
	void GetDetectorState(nsUniversalDetector::DetectorState(&states)[nsUniversalDetector::NumDetectors], uint32 &offset);
#endif
protected:
	nsCharSetProber * mProbers[NUM_OF_PROBERS];
	mutable int32 mBestGuess; // GetCofident modifies this member
	uint32 mActiveNum;
	uint32 mKeepNext;
	bool   mIsActive[NUM_OF_PROBERS];
};
//
//#include <nsSBCSGroupProber.h>
//
class nsSBCSGroupProber : public nsCharSetProber {
public:
	nsSBCSGroupProber();
	virtual ~nsSBCSGroupProber();
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName();
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	void  DumpStatus();
#endif
protected:
	nsCharSetProber * mProbers[NUM_OF_SBCS_PROBERS];
	mutable int32  mBestGuess; // GetConfident modifies this member
	uint32 mActiveNum;
	bool   mIsActive[NUM_OF_SBCS_PROBERS];
};
//
#include "uchardet-tab\JISFreq.tab"
#include "uchardet-tab\Big5Freq.tab"
#include "uchardet-tab\EUCKRFreq.tab"
#include "uchardet-tab\EUCTWFreq.tab"
#include "uchardet-tab\GB2312Freq.tab"
//
// Language model for: Arabic
//
// Generated by BuildLangModel.py On: 2015-12-13 18:33:58.848027
//
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_6_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 52, 72, 61, 68, 74, 69, 59, 78, 60, 90, 86, 67, 65, 71, 75, /* 4X */
	64, 85, 76, 55, 57, 79, 81, 70, 82, 87, 91, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 37, 58, 49, 47, 38, 54, 66, 46, 39, 88, 63, 45, 51, 43, 40, /* 6X */
	62, 89, 42, 44, 41, 50, 77, 73, 83, 56, 80, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, ILL, ILL, ILL, SYM, ILL, ILL, ILL, ILL, ILL, ILL, ILL, SYM, SYM, ILL, ILL, /* AX */
	ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, SYM, ILL, ILL, ILL, SYM, /* BX */
	ILL, 32, 34, 15, 35, 22, 31,  0,  9,  8,  7, 27, 19, 18, 25, 11, /* CX */
	30,  5, 26, 12, 21, 23, 28, SYM, 33, 10, 29, ILL, ILL, ILL, ILL, ILL, /* DX */
	36, 13, 14, 17,  1,  3,  6, 16,  4, 24,  2, SYM, SYM, SYM, SYM, SYM, /* EX */
	SYM, SYM, SYM, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, ILL, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Windows_1256_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 52, 72, 61, 68, 74, 69, 59, 78, 60, 90, 86, 67, 65, 71, 75, /* 4X */
	64, 85, 76, 55, 57, 79, 81, 70, 82, 87, 91, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 37, 58, 49, 47, 38, 54, 66, 46, 39, 88, 63, 45, 51, 43, 40, /* 6X */
	62, 89, 42, 44, 41, 50, 77, 73, 83, 56, 80, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, 48, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 95, SYM, 96, 92, 97, 98, /* 8X */
	53, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 84, SYM, 99, SYM, 100, SYM, SYM, 101, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 102, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	103, 32, 34, 15, 35, 22, 31,  0,  9,  8,  7, 27, 19, 18, 25, 11, /* CX */
	30,  5, 26, 12, 21, 23, 28, SYM, 20, 33, 10, 29, 36, 13, 14, 17, /* DX */
	104,  1, 93,  3,  6, 16,  4, 105, 106, 94, 107, 108, 24,  2, 109, 110, /* EX */
	SYM, SYM, SYM, SYM, 111, SYM, SYM, SYM, SYM, 112, SYM, 113, 114, SYM, SYM, 115, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 1479
 * First 512 sequences: 0.9696025116913417
 * Next 512 sequences (512-1024): 0.029166911858880054
 * Rest: 0.0012305764497782395
 * Negative sequences: TODO
 */
static const uint8 ArabicLangModel[] =
{
	2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 1, 3, 1, 3, 3, 3, 3, 2, 2, 3,
	3, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2,
	1, 2, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 0, 3, 1, 3, 3, 3, 3, 2, 2, 3,
	2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 1, 3, 2, 3, 3, 3, 2, 2, 2, 2,
	0, 2, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2,
	2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 2, 3, 2, 3, 2, 3, 3, 2, 3,
	1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 2, 3, 3, 3, 3, 0, 3, 2, 2, 3, 2, 2, 2, 3, 2,
	0, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 2, 3, 3, 2, 2,
	0, 3, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2,
	1, 2, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 0, 3, 2, 0, 2, 2, 3, 0, 3, 2, 0, 3, 3, 3, 0, 2, 0,
	0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 2, 0, 2, 0, 0, 3, 3, 2, 3, 0, 2, 0, 2,
	2, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 3, 3, 2, 3, 3, 1, 0, 0, 2, 2, 0, 1, 0, 1, 0, 1,
	0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 2, 2,
	1, 3, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 2, 0, 2, 1, 3, 2, 0, 3, 2, 0, 2, 0, 3, 0, 2, 0,
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 0, 3, 2, 3, 2, 3, 2, 3, 2, 2,
	0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 1, 3, 2, 1, 2, 0, 2, 2, 0, 3, 2, 2, 0, 0, 2, 0, 2, 1, 2, 0, 3, 0,
	0, 1, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 2, 3, 3, 0, 1, 3, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 1, 3, 3, 3, 3, 0, 2, 3, 0, 3, 2, 2, 0, 3, 2, 0, 3, 2, 3, 0, 2, 0,
	0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 2, 3, 1, 2, 1, 0, 1, 0, 0, 1, 0, 3, 2, 0, 2, 2, 2,
	0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 3, 2, 2, 0, 2, 1, 2, 1, 1, 0, 2, 1, 0, 0, 0, 1, 0, 2,
	1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 1, 2, 2, 2, 3, 3, 2, 2, 2, 0, 0, 0, 2, 3, 1, 0, 0, 2, 1, 2,
	0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 3, 3, 1, 2, 3, 2, 0, 2, 3, 3, 3, 2, 3, 0, 2, 2, 2, 3, 2, 2, 0, 3, 0, 2, 2, 2, 3, 2, 3, 1,
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 0, 3, 2, 0, 2, 1, 3, 0, 2, 0, 0, 2, 2, 2, 0, 0, 0, 2, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 3, 2, 2, 0, 0, 2, 0, 0, 1, 3, 2, 0, 3, 0, 1, 2, 0, 2, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 2, 0, 2, 2, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 2, 2, 0, 0, 1, 0, 2,
	2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 1, 1, 2, 3, 1, 2, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 2, 0, 2, 0, 1, 2, 0, 2, 1, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 0, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 1, 2, 2, 2, 0, 0, 2, 0, 0, 2, 2, 1, 0, 2, 1, 0, 2, 0, 2, 0, 2, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 2, 0, 3, 3, 0, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 1, 0, 1, 2, 0, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 1, 1, 1, 0, 0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 2, 3, 2, 2, 1, 2, 3, 2, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 3, 2, 2, 2, 3, 2, 2, 0, 2, 0, 2, 2, 2, 2, 0, 1, 2, 1, 1, 0, 2, 0, 1, 0, 3, 1, 2, 0, 1, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 3, 2, 1, 2, 1, 1, 0, 2, 2, 0, 2, 0, 2, 2, 0, 0, 0, 2, 0, 0, 2, 2, 1, 2, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 0, 2, 1, 2, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 1, 1, 2, 2, 2, 2, 2, 0, 2, 0, 2, 1, 2, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 1, 2, 2, 2, 2, 2, 2, 0, 2, 0, 2, 1, 2, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 1, 2, 2, 2, 2, 2, 1, 1, 2, 0, 2, 2, 2, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 1, 0, 1, 1, 1, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 1, 2, 2, 2, 0, 1, 0, 2, 1, 2, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 1, 2, 1, 1, 2, 0, 2, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 1, 2, 0, 0, 2, 1, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 0, 0, 1, 2, 0, 2, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 1, 0, 2, 2, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 1, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 2, 2, 1, 0, 2, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	2, 2, 2, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 1, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 0, 2, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
};
// 
// CTR: Control characters that usually does not exist in any text
// RET: Carriage/Return
// SYM: symbol (punctuation) that does not belong to word
// NUM: 0 - 9
// 
//Character Mapping Table:
//this talbe is modified base on win1251BulgarianCharToOrderMap, so
//only number <64 is sure valid
// 
static const uchar Latin5_BulgarianCharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 77, 90, 99, 100, 72, 109, 107, 101, 79, 185, 81, 102, 76, 94, 82, //40
	110, 186, 108, 91, 74, 119, 84, 96, 111, 187, 115, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 65, 69, 70, 66, 63, 68, 112, 103, 92, 194, 104, 95, 86, 87, 71, //60
	116, 195, 85, 93, 97, 113, 196, 197, 198, 199, 200, SYM, SYM, SYM, SYM, SYM, //70
	194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, //80
	210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, //90
	81, 226, 227, 228, 229, 230, 105, 231, 232, 233, 234, 235, 236, 45, 237, 238, //a0
	31, 32, 35, 43, 37, 44, 55, 47, 40, 59, 33, 46, 38, 36, 41, 30, //b0
	39, 28, 34, 51, 48, 49, 53, 50, 54, 57, 61, 239, 67, 240, 60, 56, //c0
	1, 18,  9, 20, 11,  3, 23, 15,  2, 26, 12, 10, 14,  6,  4, 13, //d0
	7,  8,  5, 19, 29, 25, 22, 21, 27, 24, 17, 75, 52, 241, 42, 16, //e0
	62, 242, 243, 244, 58, 245, 98, 246, 247, 248, 249, 250, 251, 91, NUM, SYM, //f0
};

static const uchar win1251BulgarianCharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 77, 90, 99, 100, 72, 109, 107, 101, 79, 185, 81, 102, 76, 94, 82, //40
	110, 186, 108, 91, 74, 119, 84, 96, 111, 187, 115, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 65, 69, 70, 66, 63, 68, 112, 103, 92, 194, 104, 95, 86, 87, 71, //60
	116, 195, 85, 93, 97, 113, 196, 197, 198, 199, 200, SYM, SYM, SYM, SYM, SYM, //70
	206, 207, 208, 209, 210, 211, 212, 213, 120, 214, 215, 216, 217, 218, 219, 220, //80
	221, 78, 64, 83, 121, 98, 117, 105, ILL, 223, 224, 225, 226, 227, 228, 229, //90
	88, 230, 231, 232, 233, 122, 89, 106, 234, 235, 236, 237, 238, 45, 239, 240, //a0
	73, 80, 118, 114, 241, 242, 243, 244, 245, 62, 58, 246, 247, 248, 249, 250, //b0
	31, 32, 35, 43, 37, 44, 55, 47, 40, 59, 33, 46, 38, 36, 41, 30, //c0
	39, 28, 34, 51, 48, 49, 53, 50, 54, 57, 61, 251, 67, NUM, 60, 56, //d0
	1, 18,  9, 20, 11,  3, 23, 15,  2, 26, 12, 10, 14,  6,  4, 13, //e0
	7,  8,  5, 19, 29, 25, 22, 21, 27, 24, 17, 75, 52, SYM, 42, 16, //f0
};
// 
// Model Table:
// total sequences: 100%
// first 512 sequences: 96.9392%
// first 1024 sequences:3.0618%
// rest  sequences:     0.2992%
// negative sequences:  0.0020%
static const uint8 BulgarianLangModel[] =
{
	0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 2, 2, 3, 2, 2, 1, 2, 2,
	3, 1, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 1, 0,
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 1, 3, 3, 3, 3, 2, 2, 2, 1, 1, 2, 0, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 1, 1, 2, 3, 3, 2, 3, 3, 3, 3, 2, 1, 2, 0, 2, 0, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 1, 3, 0, 3, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 2, 3, 3, 3, 1, 3, 3, 2, 3, 2, 2, 2, 0, 0, 2, 0, 2, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 2, 2, 3, 3, 3, 1, 2, 2, 3, 2, 1, 1, 2, 0, 2, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 1, 2, 3, 2, 2, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1, 2, 0, 2, 1, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1, 2, 0, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 2, 2, 1, 3, 1, 3, 2, 2, 3, 0, 0, 1, 0, 1, 0, 1, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 2, 2, 3, 2, 2, 3, 1, 2, 1, 1, 1, 2, 3, 1, 3, 1, 2, 2, 0, 1, 1, 1, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 1, 3, 2, 2, 3, 3, 1, 2, 3, 1, 1, 3, 3, 3, 3, 1, 2, 2, 1, 1, 1, 0, 2, 0, 2, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 2, 2, 3, 3, 3, 2, 2, 1, 1, 2, 0, 2, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 0, 1, 2, 1, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 2, 1, 0, 3, 1, 2, 1, 2, 1, 2, 3, 2, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 1, 3, 3, 2, 3, 3, 2, 2, 2, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 2, 1, 1, 2, 1, 3, 3, 0, 3, 1, 1, 1, 1, 3, 2, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 1, 3, 3, 2, 3, 2, 2, 2, 3, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 2, 0, 3, 2, 0, 3, 0, 2, 0, 0, 2, 1, 3, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 2, 1, 3, 1, 1, 2, 1, 3, 2, 1, 1, 0, 1, 2, 3, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 2, 2, 1, 0, 1, 0, 0, 1, 0, 0, 0, 2, 1, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 2, 3, 2, 3, 3, 1, 3, 2, 1, 1, 1, 2, 1, 1, 2, 1, 3, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 1, 2, 2, 3, 3, 2, 3, 2, 2, 2, 3, 1, 2, 2, 1, 1, 2, 1, 1, 2, 2, 0, 1, 1, 0, 1, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 1, 3, 1, 0, 2, 2, 1, 3, 2, 1, 0, 0, 2, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 3, 3, 1, 2, 0, 2, 3, 1, 2, 3, 2, 0, 1, 3, 1, 2, 1, 1, 1, 0, 0, 1, 0, 0, 2, 2, 2, 3,
	2, 2, 2, 2, 1, 2, 1, 1, 2, 2, 1, 1, 2, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1,
	3, 3, 3, 3, 3, 2, 1, 2, 2, 1, 2, 0, 2, 0, 1, 0, 1, 2, 1, 2, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 2, 3, 3, 1, 1, 3, 1, 0, 3, 2, 1, 0, 0, 0, 1, 2, 0, 2, 0, 1, 0, 0, 0, 1, 0, 1, 2, 1, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 2, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0,
	3, 1, 0, 1, 0, 2, 3, 2, 2, 2, 3, 2, 2, 2, 2, 2, 1, 0, 2, 1, 2, 1, 1, 1, 0, 1, 2, 1, 2, 2, 2, 1,
	1, 1, 2, 2, 2, 2, 1, 2, 1, 1, 0, 1, 2, 1, 2, 2, 2, 1, 1, 1, 0, 1, 1, 1, 1, 2, 0, 1, 0, 0, 0, 0,
	2, 3, 2, 3, 3, 0, 0, 2, 1, 0, 2, 1, 0, 0, 0, 0, 2, 3, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 1, 2,
	2, 1, 2, 1, 2, 2, 1, 1, 1, 2, 1, 1, 1, 0, 1, 2, 2, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 2, 0, 0,
	3, 3, 2, 2, 3, 0, 2, 3, 1, 1, 2, 0, 0, 0, 1, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0, 1, 0, 1, 2, 0, 2, 2,
	1, 1, 1, 1, 2, 1, 0, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
	2, 3, 2, 3, 3, 0, 0, 3, 0, 1, 1, 0, 1, 0, 0, 0, 2, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 2,
	2, 2, 1, 1, 1, 1, 1, 2, 2, 2, 1, 0, 2, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0,
	3, 3, 3, 3, 2, 2, 2, 2, 2, 0, 2, 1, 1, 1, 1, 2, 1, 2, 1, 1, 0, 2, 0, 1, 0, 1, 0, 0, 2, 0, 1, 2,
	1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 0, 2, 0, 1, 0, 2, 0, 0, 1, 1, 1, 0, 0, 2, 0, 0, 0, 1, 1, 0, 0,
	2, 3, 3, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 2, 0, 1, 2,
	2, 2, 2, 1, 1, 2, 1, 1, 2, 2, 2, 1, 2, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0,
	2, 3, 3, 3, 3, 0, 2, 2, 0, 2, 1, 0, 0, 0, 1, 1, 1, 2, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 2, 0, 2, 2,
	1, 1, 1, 2, 1, 2, 1, 1, 2, 2, 2, 1, 2, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 2, 1, 0, 0, 0, 1, 1, 0, 0,
	2, 3, 3, 3, 3, 0, 2, 1, 0, 0, 2, 0, 0, 0, 0, 0, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 2,
	1, 1, 1, 2, 1, 1, 1, 1, 2, 2, 2, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0,
	3, 3, 2, 2, 3, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 2,
	1, 1, 1, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0,
	3, 1, 0, 1, 0, 2, 2, 2, 2, 3, 2, 1, 1, 1, 2, 3, 0, 0, 1, 0, 2, 1, 1, 0, 1, 1, 1, 1, 2, 1, 1, 1,
	1, 2, 2, 1, 2, 1, 2, 2, 1, 1, 0, 1, 2, 1, 2, 2, 1, 1, 1, 0, 0, 1, 1, 1, 2, 1, 0, 1, 0, 0, 0, 0,
	2, 1, 0, 1, 0, 3, 1, 2, 2, 2, 2, 1, 2, 2, 1, 1, 1, 0, 2, 1, 2, 2, 1, 1, 2, 1, 1, 0, 2, 1, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 0, 1, 1, 0, 2, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,
	2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 1, 2, 1, 2, 3, 2, 2, 1, 1, 1, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 3, 2, 0, 1, 2, 0, 1, 2, 1, 1, 0, 1, 0, 1, 2, 1, 2, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 2,
	1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 2, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,
	2, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1,
	1, 2, 2, 2, 2, 1, 1, 2, 1, 2, 1, 1, 1, 0, 2, 1, 2, 1, 1, 1, 0, 2, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
	1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 3, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 2,
	1, 1, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1,
	2, 3, 1, 2, 1, 0, 1, 1, 0, 2, 2, 2, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 2,
	1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0,
	2, 2, 2, 2, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2,
	1, 1, 1, 1, 1, 0, 0, 1, 2, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 0, 0, 2, 0, 1, 1, 0, 0, 0, 1, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 3, 2, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2,
	1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 2, 2, 2, 1, 2, 1, 2, 2, 1, 1, 2, 1, 1, 1, 0, 1, 1, 1, 1, 2, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1,
	1, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 3, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 1, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1,
	0, 2, 0, 1, 0, 0, 1, 1, 2, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 0, 1, 1, 0, 2, 1, 0, 1, 1, 1, 0, 0, 1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 1, 0, 0, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
	1, 1, 2, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 1, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 2, 0, 0, 2, 0, 1, 0, 0, 1, 0, 0, 1,
	1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
	1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 0, 0, 0, 2, 1, 0, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
};
//
// Language model for: Danish 
// 
// Generated by BuildLangModel.py On: 2016-02-19 17:56:42.163975
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_15_Danish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 4X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 6X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, 39, SYM, 39, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, 53, 42, SYM, SYM, 54, SYM, SYM, SYM, 55, 56, 57, SYM, /* BX */
	58, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 59, 34, 60, 50, /* CX */
	43, 47, 51, 36, 52, 61, 30, SYM, 19, 62, 37, 44, 31, 46, 63, 48, /* DX */
	64, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 65, 34, 66, 50, /* EX */
	43, 47, 51, 36, 52, 67, 30, SYM, 19, 68, 37, 44, 31, 46, 69, 70, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_1_Danish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 4X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 6X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 42, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	71, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 72, 34, 73, 50, /* CX */
	43, 47, 51, 36, 52, 74, 30, SYM, 19, 75, 37, 44, 31, 46, 76, 48, /* DX */
	77, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 78, 34, 79, 50, /* EX */
	43, 47, 51, 36, 52, 80, 30, SYM, 19, 81, 37, 44, 31, 46, 82, 83, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Windows_1252_Danish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 4X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  4, 15, 24,  7,  0, 13, 10, 18,  5, 23, 11,  8, 12,  2,  9, /* 6X */
	17, 29,  1,  6,  3, 16, 14, 25, 27, 20, 26, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, 84, SYM, SYM, SYM, SYM, SYM, SYM, 39, SYM, 85, ILL, 86, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 39, SYM, 87, ILL, 88, 89, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 42, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	90, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 91, 34, 92, 50, /* CX */
	43, 47, 51, 36, 52, 93, 30, SYM, 19, 94, 37, 44, 31, 46, 95, 48, /* DX */
	96, 33, 40, 35, 32, 21, 22, 38, 41, 28, 49, 45, 97, 34, 98, 50, /* EX */
	43, 47, 51, 36, 52, 99, 30, SYM, 19, 100, 37, 44, 31, 46, 101, 102, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 964
 * First 512 sequences: 0.9968082796759031
 * Next 512 sequences (512-1024): 0.0031917203240968304
 * Rest: 3.903127820947816e-17
 * Negative sequences: TODO
 */
static const uint8 DanishLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 3, 2, 3, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 2, 3, 3, 3, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 2, 3, 3, 2, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 3, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 2,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 2, 3, 2, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 2, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 3, 3, 2, 2, 0, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 3, 3, 3, 3, 3, 2, 2, 0, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 0, 2, 2, 3, 2, 3, 3, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 0, 2, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 2, 3, 2, 3, 2, 3, 2, 2, 3, 3, 3, 3, 3, 2, 3, 2, 2, 2, 0,
	3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 0, 2, 2, 2, 2, 0, 0, 3, 0, 0, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 2, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 2, 0, 3, 3, 3, 2, 3, 3, 2, 2, 3, 3, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 0, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
	3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 3, 3, 2, 3, 2, 2, 0, 0, 0, 0, 0,
	3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 2, 2, 0, 2, 3, 2, 3, 0, 3, 0, 0, 2, 3, 2, 2, 0, 2, 2,
	3, 2, 2, 2, 3, 3, 2, 2, 2, 3, 0, 2, 2, 2, 0, 2, 2, 0, 2, 0, 2, 0, 0, 0, 2, 2, 2, 0, 0, 0,
	3, 2, 2, 2, 3, 3, 2, 2, 0, 3, 0, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0, 2, 0, 0, 0,
	3, 2, 0, 2, 2, 3, 2, 0, 2, 2, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 2, 0,
	2, 3, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 3, 2, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
};
//
// Language model for: Esperanto 
//
// 
// Generated by BuildLangModel.py On: 2015-12-04 01:27:38.177516
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_3_Esperanto_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  0, 18, 17, 10,  2, 19, 15, 21,  3, 11,  9,  7, 13,  4,  1, /* 4X */
	14, 32,  5,  8,  6, 12, 16, 27, 33, 25, 20, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  0, 18, 17, 10,  2, 19, 15, 21,  3, 11,  9,  7, 13,  4,  1, /* 6X */
	14, 32,  5,  8,  6, 12, 16, 27, 33, 25, 20, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, 56, SYM, SYM, SYM, ILL, 34, SYM, SYM, 57, 53, 58, 28, SYM, ILL, 40, /* AX */
	SYM, 59, SYM, SYM, SYM, SYM, 34, SYM, SYM, 60, 53, 61, 28, SYM, ILL, 40, /* BX */
	44, 29, 46, ILL, 43, 62, 24, 38, 41, 31, 48, 50, 54, 35, 49, 52, /* CX */
	ILL, 42, 63, 30, 47, 64, 36, SYM, 22, 51, 39, 55, 37, 23, 26, 45, /* DX */
	44, 29, 46, ILL, 43, 65, 24, 38, 41, 31, 48, 50, 54, 35, 49, 52, /* EX */
	ILL, 42, 66, 30, 47, 67, 36, SYM, 22, 51, 39, 55, 37, 23, 26, SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 989
 * First 512 sequences: 0.9942980632768038
 * Next 512 sequences (512-1024): 0.0057019367231962385
 * Rest: -5.0306980803327406e-17
 * Negative sequences: TODO
 */
static const uint8 EsperantoLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 2, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 0, 0, 0, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 0, 2, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2, 3, 2, 2, 2, 3, 0, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 2, 2, 3, 2, 3, 2, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 0, 3, 3, 3, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 0, 0, 2, 3, 2, 2, 2, 3, 3, 2, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2, 2, 3, 2, 2, 0, 3, 3, 3, 2, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 3, 0, 2, 0, 3, 2, 3, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 0, 0, 0, 3, 2, 0, 2, 3, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 0, 0, 2, 3, 0, 3, 2, 2, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 3, 3, 2, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 3, 2, 3, 2, 0, 0, 0, 2, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 0, 0, 0, 2, 3, 2, 2, 0, 3, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 2, 3, 2, 0, 2, 2, 2, 2, 3, 0, 0, 0, 2, 2, 0, 0, 3, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 0, 3, 3, 2, 2, 3, 2, 2, 2, 2, 3, 0, 2, 2, 3, 2, 2, 2, 2, 2, 3, 0, 2, 0,
	3, 3, 3, 3, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 0, 0, 2, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 3, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 2, 0, 3, 2, 2, 2, 0, 3, 2, 2, 3, 3, 0, 0, 0, 3, 0, 0, 0, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 2, 3, 2, 0, 2, 0, 0, 0, 3, 2, 0, 0, 3, 3, 3, 0, 0, 0,
	3, 3, 3, 3, 0, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 3, 2, 0, 2, 3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 3, 0,
	3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 0, 0, 0, 0, 3, 2, 2, 0, 2, 2, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0, 3, 3, 2, 0, 2, 0, 2, 2, 0, 2, 0, 0, 0, 2, 0, 2, 0, 2, 2, 2, 0, 2, 0,
	3, 3, 3, 3, 0, 0, 2, 3, 0, 0, 2, 2, 3, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 2, 0, 2, 2, 3, 2, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 0, 0, 2, 2, 0, 2, 3, 2, 3, 3, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0,
	3, 3, 3, 3, 2, 2, 3, 2, 0, 2, 0, 2, 3, 2, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 2, 2, 2, 3, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 2, 0, 3, 0, 0, 2, 0, 0, 0, 0,
	3, 3, 2, 2, 2, 2, 0, 2, 0, 2, 0, 0, 3, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 3, 3, 3, 3, 3, 2, 3, 0, 0, 2, 2, 2, 2, 3, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 3, 3, 2, 2, 2, 2, 2, 2, 0, 0, 2, 2, 2, 0, 2, 2, 3, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 0, 3, 3, 3, 3, 3, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
	2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 3, 0, 0, 2, 2, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 2, 0, 2, 0, 0, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: French
// 
// Generated by BuildLangModel.py On: 2015-12-03 21:10:27.685575
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Windows_1252_French_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 4X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 6X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, 56, SYM, SYM, SYM, SYM, SYM, SYM, 51, SYM, 35, ILL, 57, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 51, SYM, 35, ILL, 58, 59, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 60, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	24, 38, 32, 46, 49, 61, 47, 27, 23, 14, 28, 41, 62, 39, 33, 36, /* CX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 63, /* DX */
	24, 38, 32, 46, 49, 64, 47, 27, 23, 14, 28, 41, 65, 39, 33, 36, /* EX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 66, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_1_French_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 4X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 6X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 67, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	24, 38, 32, 46, 49, 68, 47, 27, 23, 14, 28, 41, 69, 39, 33, 36, /* CX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 70, /* DX */
	24, 38, 32, 46, 49, 71, 47, 27, 23, 14, 28, 41, 72, 39, 33, 36, /* EX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 73, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_15_French_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 4X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  2, 18, 11, 10,  0, 17, 15, 19,  4, 25, 26,  7, 13,  3,  8, /* 6X */
	12, 20,  5,  1,  6,  9, 16, 30, 21, 22, 29, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, 51, SYM, 51, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, 74, 75, SYM, SYM, 76, SYM, SYM, SYM, 35, 35, 77, SYM, /* BX */
	24, 38, 32, 46, 49, 78, 47, 27, 23, 14, 28, 41, 79, 39, 33, 36, /* CX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 80, /* DX */
	24, 38, 32, 46, 49, 81, 47, 27, 23, 14, 28, 41, 82, 39, 33, 36, /* EX */
	48, 45, 54, 40, 31, 55, 42, SYM, 52, 37, 43, 34, 44, 53, 50, 83, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 914
 * First 512 sequences: 0.997057879992383
 * Next 512 sequences (512-1024): 0.002942120007616917
 * Rest: 3.8163916471489756e-17
 * Negative sequences: TODO
 */
static const uint8 FrenchLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 3, 3, 0, 3, 3, 0, 0, 0, 2, 0, 2, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 0, 3, 3, 0, 0, 3, 0, 0, 2, 3, 0, 0, 0, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 3, 3, 0, 3, 3, 2, 2, 3, 0, 0, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 3, 3, 3, 2, 3, 2, 0, 2, 2, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 2, 3, 0, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 2, 3, 3, 3, 2, 3, 3, 3, 0, 2, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 3, 3, 2, 0, 2, 0, 3, 3, 2, 3, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 3, 3, 3, 2, 3, 0, 0, 2, 2, 2, 2, 0, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 0, 0, 3, 3, 0, 0, 2, 3, 0, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 0, 3, 3, 2, 3, 3, 2, 0, 0, 0, 0, 0, 2, 0,
	3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 2, 3, 0, 0, 3, 3, 0, 3, 0, 0, 2, 2, 3, 2, 2, 2, 3, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 3, 3, 0, 3, 3, 0, 0, 3, 0, 2, 2, 2, 3, 2, 0, 0, 2, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 0, 0, 2, 2, 3, 0, 0, 3, 3, 0, 0, 2, 2, 3, 2, 2, 3, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 3, 3, 3, 3, 2, 0, 2, 3, 2, 0, 0, 3, 3, 0, 2, 2, 0, 3, 0, 2, 2, 3, 0, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 0, 0, 3, 2, 2, 0, 3, 0, 0, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 3, 2, 2, 3, 3, 0, 2, 3, 3, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 3, 3, 0, 2, 3, 3, 0, 0, 0, 2, 3, 0, 2, 2, 0, 0, 0, 0, 2, 3, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 2, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 0, 3, 0, 0, 0, 0, 0, 3, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 0, 3, 2, 0, 0, 3, 2, 0, 3, 0, 0, 0, 0, 0, 0, 3, 2, 0, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 0, 3, 3, 0, 0, 2, 2, 0, 0, 0, 3, 3, 0, 2, 2, 0, 2, 2, 2, 3, 3, 0, 0, 2, 0, 0,
	0, 0, 2, 0, 0, 0, 0, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 0, 3, 0, 3, 2, 3, 2, 2, 3, 3, 2, 3, 0, 3, 2, 2, 2, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 2, 2, 2, 0, 3, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 0, 3, 0, 3, 3, 3, 0, 0, 3, 3, 2, 3, 0, 3, 3, 2, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 0, 0, 2, 3, 2, 2, 2, 2, 2, 3, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 3, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 2, 0, 0, 3, 2, 0, 0, 0, 3, 0, 3, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 2, 3, 2, 0, 2, 3, 3, 0, 2, 0, 2, 2, 2, 0, 0, 2, 2, 2, 0, 3, 0, 0, 0, 2, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 3, 2, 2, 2, 3, 2, 0, 2, 0, 0, 2, 0, 0, 2, 2, 2, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 2, 0, 3, 0, 0, 3, 3, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 2, 0, 2, 2, 0, 3, 3, 0, 0, 0, 3, 2, 2, 0, 3, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 3, 0, 0, 3, 3, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 2, 0, 0, 2, 0, 2, 2, 0, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 0, 2, 2, 3, 0, 0, 2, 2, 0, 2, 0, 2, 0, 2, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: German 
// 
// Generated by BuildLangModel.py On: 2015-12-03 22:50:46.518374
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Windows_1252_German_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  5, 15, 12,  8,  0, 17, 14,  7,  3, 23, 16,  9, 13,  2, 11, /* 4X */
	18, 30,  1,  4,  6, 10, 21, 19, 28, 25, 20, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  5, 15, 12,  8,  0, 17, 14,  7,  3, 23, 16,  9, 13,  2, 11, /* 6X */
	18, 30,  1,  4,  6, 10, 21, 19, 28, 25, 20, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, 59, SYM, SYM, SYM, SYM, SYM, SYM, 36, SYM, 54, ILL, 42, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 36, SYM, 54, ILL, 42, 56, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 60, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	41, 31, 37, 44, 22, 49, 50, 35, 32, 29, 48, 43, 57, 33, 47, 52, /* CX */
	53, 39, 51, 34, 40, 55, 26, SYM, 38, 58, 46, 61, 24, 45, 62, 27, /* DX */
	41, 31, 37, 44, 22, 49, 50, 35, 32, 29, 48, 43, 57, 33, 47, 52, /* EX */
	53, 39, 51, 34, 40, 55, 26, SYM, 38, 58, 46, 63, 24, 45, 64, 56, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_1_German_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  5, 15, 12,  8,  0, 17, 14,  7,  3, 23, 16,  9, 13,  2, 11, /* 4X */
	18, 30,  1,  4,  6, 10, 21, 19, 28, 25, 20, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  5, 15, 12,  8,  0, 17, 14,  7,  3, 23, 16,  9, 13,  2, 11, /* 6X */
	18, 30,  1,  4,  6, 10, 21, 19, 28, 25, 20, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 65, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	41, 31, 37, 44, 22, 49, 50, 35, 32, 29, 48, 43, 57, 33, 47, 52, /* CX */
	53, 39, 51, 34, 40, 55, 26, SYM, 38, 58, 46, 66, 24, 45, 67, 27, /* DX */
	41, 31, 37, 44, 22, 49, 50, 35, 32, 29, 48, 43, 57, 33, 47, 52, /* EX */
	53, 39, 51, 34, 40, 55, 26, SYM, 38, 58, 46, 68, 24, 45, 69, 56, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 1188
 * First 512 sequences: 0.9934041448127945
 * Next 512 sequences (512-1024): 0.006482829516922903
 * Rest: 0.0001130256702826099
 * Negative sequences: TODO
 */
static const uint8 GermanLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 3, 3, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 2, 2, 3, 3, 2, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 0, 0, 3, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 3, 0, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 0, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 3, 3, 3, 0, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 3, 2, 2, 3, 2, 3, 3, 2, 0, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0, 3, 3, 3, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 0, 3, 0, 3, 3, 1, 2,
	3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 1, 3, 2, 0, 1, 2, 3,
	3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 2, 2, 3, 2, 3, 3, 3, 0, 0, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2, 2, 3, 2, 3, 2, 3, 3, 2, 0, 2, 2, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 3, 2, 3, 3, 3, 0, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 1, 3, 2, 2, 3, 3, 3, 2, 2, 2, 3, 2, 3, 3, 3, 0, 1, 2, 1,
	3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 3, 2, 3, 3, 2, 2, 2, 2, 3, 2, 3, 2, 3, 0, 0, 2, 0,
	3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 2, 2, 3, 2, 2, 2, 2, 0, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 3, 0, 0, 0, 0,
	3, 2, 2, 3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 2, 3, 2, 2, 2, 3, 3, 2, 2, 2, 3, 3, 3, 0, 0, 2, 2,
	3, 2, 2, 3, 2, 3, 2, 0, 2, 2, 2, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 2, 3, 0, 0, 2, 1,
	2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2,
	3, 2, 2, 3, 2, 3, 2, 2, 2, 2, 3, 3, 2, 2, 2, 1, 2, 1, 2, 0, 2, 0, 3, 2, 3, 2, 2, 0, 0, 2, 0,
	2, 3, 3, 0, 3, 1, 3, 3, 3, 3, 0, 0, 3, 2, 3, 3, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 2, 2, 2, 3, 2, 3, 3, 3, 2, 2, 3, 2, 3, 2, 2, 2, 0, 2, 2, 2, 1, 0, 0, 1, 0,
	2, 3, 3, 2, 3, 0, 3, 3, 2, 3, 0, 1, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 0, 0, 0, 0, 1, 3, 1, 0, 0,
	3, 2, 2, 3, 2, 2, 3, 2, 1, 2, 2, 2, 0, 2, 2, 3, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 2, 3, 1, 3, 3, 2, 1, 2, 2, 2, 2, 0, 0, 2, 2, 2, 3, 2, 0, 2, 0, 0, 0, 2, 0, 0, 2, 2, 0,
	2, 3, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 1, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2,
	0, 1, 0, 2, 0, 2, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: Greek 
// 
// Generated by BuildLangModel.py On: 2016-05-25 15:21:50.073117
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Windows_1253_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 32, 46, 41, 40, 30, 52, 48, 42, 33, 56, 49, 39, 44, 36, 34, /* 4X */
	47, 59, 35, 38, 37, 43, 54, 50, 58, 53, 57, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 32, 46, 41, 40, 30, 52, 48, 42, 33, 56, 49, 39, 44, 36, 34, /* 6X */
	47, 59, 35, 38, 37, 43, 54, 50, 58, 53, 57, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, ILL, SYM, ILL, ILL, ILL, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, ILL, SYM, ILL, ILL, ILL, ILL, /* 9X */
	SYM, SYM, 17, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 62, SYM, SYM, 19, 22, 15, SYM, 16, SYM, 24, 28, /* BX */
	55,  0, 25, 18, 20,  5, 29, 10, 26,  3,  8, 14, 13,  4, 31,  1, /* CX */
	11,  6, ILL,  7,  2, 12, 27, 23, 45, 21, 51, 60, 17, 19, 22, 15, /* DX */
	61,  0, 25, 18, 20,  5, 29, 10, 26,  3,  8, 14, 13,  4, 31,  1, /* EX */
	11,  6,  9,  7,  2, 12, 27, 23, 45, 21, 51, 60, 16, 24, 28, ILL, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_7_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 32, 46, 41, 40, 30, 52, 48, 42, 33, 56, 49, 39, 44, 36, 34, /* 4X */
	47, 59, 35, 38, 37, 43, 54, 50, 58, 53, 57, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 32, 46, 41, 40, 30, 52, 48, 42, 33, 56, 49, 39, 44, 36, 34, /* 6X */
	47, 59, 35, 38, 37, 43, 54, 50, 58, 53, 57, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, SYM, 17, SYM, 19, 22, 15, SYM, 16, SYM, 24, 28, /* BX */
	55,  0, 25, 18, 20,  5, 29, 10, 26,  3,  8, 14, 13,  4, 31,  1, /* CX */
	11,  6, ILL,  7,  2, 12, 27, 23, 45, 21, 51, 60, 17, 19, 22, 15, /* DX */
	61,  0, 25, 18, 20,  5, 29, 10, 26,  3,  8, 14, 13,  4, 31,  1, /* EX */
	11,  6,  9,  7,  2, 12, 27, 23, 45, 21, 51, 60, 16, 24, 28, ILL, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 1579
 * First 512 sequences: 0.958419074626211
 * Next 512 sequences (512-1024): 0.03968891876305471
 * Rest: 0.0018920066107342773
 * Negative sequences: TODO
 */
static const uint8 GreekLangModel[] =
{
	1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 1, 2,
	3, 3, 3, 3, 3, 1, 3, 0, 3, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 2,
	2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 1, 2, 3, 2, 3, 1, 2,
	3, 3, 3, 3, 3, 2, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 2,
	3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 1, 3, 2, 2, 3, 3, 3, 2, 3, 0, 3, 3,
	2, 2, 2, 2, 2, 3, 3, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 1, 3, 3, 3, 3, 3, 3, 2,
	3, 1, 3, 3, 2, 3, 3, 0, 2, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 1, 3, 2, 3, 2, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 1, 3, 3, 1,
	3, 3, 3, 3, 3, 2, 2, 0, 3, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 2, 3, 2, 3, 2, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 3, 2, 3, 2, 3, 3, 0, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 3, 3,
	3, 3, 2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 3, 2, 3, 2, 3, 3, 3, 3, 1, 3, 3, 3, 3,
	2, 3, 2, 2, 2, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
	1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	1, 1, 3, 0, 3, 2, 3, 3, 3, 3, 0, 3, 0, 3, 3, 1, 0, 0, 3, 1, 2, 0, 0,
	2, 1, 1, 3, 2, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2,
	3, 3, 3, 3, 2, 3, 3, 2, 1, 1, 3, 2, 3, 1, 3, 3, 3, 3, 1, 3, 0, 3, 3,
	1, 2, 1, 1, 1, 2, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 3, 2, 3, 3, 3, 3, 2, 3, 0, 3, 3, 2, 2, 3, 3, 2, 3, 1, 2,
	3, 0, 3, 3, 2, 1, 3, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 1, 3, 2, 3, 1, 2, 1, 2, 3, 3, 2, 3, 1, 3, 3, 3, 1, 3, 1, 3, 3,
	1, 2, 3, 0, 3, 2, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 3, 3, 2, 3, 1, 2, 2, 2, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3,
	2, 3, 2, 2, 2, 3, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 2, 3, 0, 3, 3, 0, 0, 0, 3, 0, 3, 3, 0,
	3, 0, 2, 3, 2, 0, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	2, 2, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 1, 3, 3, 0, 0, 0, 3, 0, 3, 1, 0,
	3, 1, 2, 2, 3, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	2, 2, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 1, 3, 3, 0, 0, 0, 3, 0, 3, 1, 0,
	3, 0, 3, 3, 3, 0, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 0, 3, 3, 3, 3, 0, 3, 0, 3, 0, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3,
	2, 2, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 1, 3, 3, 0, 0, 0, 3, 0, 3, 3, 0,
	3, 0, 3, 3, 3, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 1, 3, 2, 3, 3, 1, 0, 0, 3, 0, 3, 1, 0, 3, 3, 3, 0, 3, 0, 3, 3,
	0, 3, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 3, 2, 3, 1, 3, 3, 2, 3, 1, 3, 1, 3, 2, 2, 1, 2, 3, 1, 2, 0, 2,
	2, 0, 3, 3, 2, 1, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 3, 1, 3, 1, 3, 3, 3, 3, 1, 2, 0, 3, 3, 0, 0, 0, 2, 0, 2, 1, 0,
	2, 0, 1, 3, 2, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 3, 1, 2, 2, 2, 3, 2, 3, 0, 3, 0, 3, 3,
	0, 2, 1, 3, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 3, 0, 3, 3, 0, 0, 0, 3, 0, 2, 1, 0,
	2, 0, 2, 3, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2,
	3, 3, 1, 3, 2, 3, 3, 1, 1, 1, 2, 1, 2, 0, 3, 3, 3, 3, 2, 3, 2, 2, 2,
	0, 2, 2, 0, 0, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 0, 3, 3, 3, 3, 1, 1, 0, 3, 0, 3, 3, 3, 2, 2, 3, 1, 3, 0, 2, 3,
	0, 2, 0, 0, 1, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 2, 3, 1, 3, 3, 3, 2, 0, 3, 1, 3, 1, 2, 3, 3, 3, 2, 3, 0, 3, 3,
	0, 2, 0, 2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 3, 2, 3, 0, 3, 3, 2, 3, 2, 3, 0, 3, 2, 0, 0, 0, 1, 0, 2, 1, 0,
	1, 0, 2, 2, 1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 1, 3, 1, 3, 1, 1, 1, 0, 2, 0, 2, 2, 1, 2, 2, 2, 1, 2, 0, 3, 2,
	0, 2, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 2, 2, 3, 3, 2, 3, 2, 2, 2, 2, 2, 2, 0,
	3, 3, 1, 3, 1, 3, 0, 0, 1, 0, 3, 1, 2, 1, 1, 2, 2, 3, 1, 2, 0, 2, 2,
	0, 3, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 2, 0, 2, 2, 1, 3, 3, 3, 2, 3, 2, 2, 2, 2, 2, 0,
	0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 1, 0, 0, 0, 0, 2, 0, 3, 2, 3, 2, 3, 3, 3, 2, 2, 3, 1, 2, 2, 0,
	0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 1, 0, 1, 0, 0, 0, 2, 0, 2, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 0,
	0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 3, 0, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 0,
	0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 2, 2, 1, 2, 2, 2, 2, 3, 2, 1, 2, 1, 0,
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 1, 3, 0, 3, 3, 3, 2, 1, 2, 2, 2, 1, 1, 3, 2, 2, 0,
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 2, 1, 1, 3, 2, 2, 1, 2, 2, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 3, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 2, 2, 2, 1, 1, 2, 2, 1, 2, 1, 2, 1, 0,
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 0,
	1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 2, 2, 1, 1, 1, 2, 2, 1, 1, 1, 2, 2, 0,
	2, 2, 0, 2, 0, 3, 0, 0, 0, 0, 3, 0, 2, 0, 0, 2, 1, 1, 0, 1, 0, 1, 2,
	0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Hebrew
//
/****************************************************************
   CTR: Control characters that usually does not exist in any text
   RET: Carriage/Return
   SYM: symbol (punctuation) that does not belong to word
   NUM: 0 - 9

 *****************************************************************/

//Windows-1255 language model
//Character Mapping Table:
static const uchar win1255_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 69, 91, 79, 80, 92, 89, 97, 90, 68, 111, 112, 82, 73, 95, 85, //40
	78, 121, 86, 71, 67, 102, 107, 84, 114, 103, 115, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 50, 74, 60, 61, 42, 76, 70, 64, 53, 105, 93, 56, 65, 54, 49, //60
	66, 110, 51, 43, 44, 63, 81, 77, 98, 75, 108, SYM, SYM, SYM, SYM, SYM, //70
	124, ILL, 203, 204, 205, 40, 58, 206, 207, 208, ILL, 210, ILL, ILL, ILL, ILL,
	ILL, 83, 52, 47, 46, 72, 32, 94, 216, 113, ILL, 109, ILL, ILL, ILL, ILL,
	34, 116, 222, 118, 100, 223, 224, 117, 119, 104, 125, 225, 226, 87, 99, 227,
	106, 122, 123, 228, 55, 229, 230, 101, 231, 232, 120, 233, 48, 39, 57, 234,
	30, 59, 41, 88, 33, 37, 36, 31, 29, 35, 235, 62, 28, 236, 126, 237,
	238, 38, 45, 239, 240, 241, 242, 243, 127, ILL, ILL, ILL, ILL, ILL, ILL, ILL,
	9,  8, 20, 16,  3,  2, 24, 14, 22,  1, 25, 15,  4, 11,  6, 23,
	12, 19, 13, 26, 18, 27, 21, 17,  7, 10,  5, ILL, ILL, 128, 96, ILL,
};

//Model Table:
//total sequences: 100%
//first 512 sequences: 98.4004%
//first 1024 sequences: 1.5981%
//rest  sequences:      0.087%
//negative sequences:   0.0015%
static const uint8 HebrewLangModel[] =
{
	0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 1, 2, 0, 1, 0, 0,
	3, 0, 3, 1, 0, 0, 1, 3, 2, 0, 1, 1, 2, 0, 2, 2, 2, 1, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 2, 2, 0, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2,
	1, 2, 1, 2, 1, 2, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2,
	1, 2, 1, 3, 1, 1, 0, 0, 2, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 1, 2, 2, 1, 3,
	1, 2, 1, 1, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 3, 2,
	1, 2, 1, 2, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 2, 3, 2, 2, 2, 1, 2, 2, 2, 2,
	1, 2, 1, 1, 2, 2, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 2, 2, 2, 2, 2,
	0, 2, 0, 2, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 2, 2, 2,
	0, 2, 1, 2, 2, 2, 0, 0, 2, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 2, 3, 2, 2, 2,
	1, 2, 1, 2, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 2, 0, 2,
	0, 2, 1, 2, 2, 2, 0, 0, 1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 2, 3, 2, 1, 2, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 3, 0, 1, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2,
	0, 2, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 1, 2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 1, 2, 0, 2, 1, 2,
	0, 2, 0, 2, 2, 2, 0, 0, 1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 1, 2, 2, 3, 3, 2, 3, 2, 3, 2, 2, 3, 1, 2, 2, 0, 2, 2, 2,
	0, 2, 1, 2, 2, 2, 0, 0, 1, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 3, 3, 3, 3, 1, 3, 2, 2, 2,
	0, 2, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 2, 2, 2, 1, 2, 2, 0, 2, 2, 2, 2,
	0, 2, 0, 2, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 1, 3, 2, 3, 3, 2, 3, 3, 2, 2, 1, 2, 2, 2, 2, 2, 2,
	0, 2, 1, 2, 1, 2, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 1,
	0, 2, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 1, 2, 3, 0, 2, 1, 2, 2,
	0, 2, 1, 1, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 1, 3, 1, 2, 2, 2, 1, 2, 3, 3, 1, 2, 1, 2, 2, 2, 2,
	0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 3, 3, 3, 1, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2,
	0, 2, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 3, 2, 1, 2, 3, 2, 3, 2, 2, 2, 2, 1, 2, 1, 1, 1, 2, 2,
	0, 2, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 1, 2, 2, 2, 2, 3, 2, 3, 1, 1, 2, 2, 1, 2, 2, 1, 1, 0, 2, 2, 2, 2,
	0, 1, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 3, 3, 3,
	0, 3, 0, 2, 2, 2, 2, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 1, 1, 2, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 0, 2, 1, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 0, 1, 1, 1, 1, 0,
	0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 1, 2, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 2, 1, 1, 2, 1, 1, 1, 2, 1, 2, 1, 2, 0, 1, 0, 1,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 1, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 0, 1, 0, 1,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2,
	0, 2, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0,
	0, 1, 1, 1, 2, 1, 2, 2, 2, 0, 2, 0, 2, 0, 1, 1, 2, 1, 1, 1, 1, 2, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 2, 2, 0, 1, 0, 0, 1, 1, 2, 2, 1, 2, 0, 2, 0, 0, 0, 1, 2, 0, 1,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 1, 2, 0, 2, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1, 1, 1, 2, 2, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1,
	1, 1, 2, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1,
	0, 2, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
	2, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 2, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 0, 1,
	1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 0, 0, 0, 0, 2, 1, 1, 2, 0, 2, 0, 0, 0, 1, 1, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 1, 1, 0, 1, 0, 0, 2, 2, 1, 2, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
	2, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 0, 0, 0, 0, 2, 1, 1, 1, 0, 2, 1, 1, 0, 0, 0, 2, 1, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 0, 0, 1, 1, 0, 2, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1,
	2, 2, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 2, 1, 0, 2, 0, 0, 0, 1, 1, 0, 1,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 2, 0, 2, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 2, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 2, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1,
	0, 1, 1, 1, 2, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0,
};
//
// Language model for: Hungarian
//
// Generated by BuildLangModel.py On: 2015-12-12 18:02:46.730481
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_2_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  1, 15, 23, 16,  0, 24, 13, 20,  7, 22,  9,  4, 12,  6,  8, /* 4X */
	21, 34,  5,  3,  2, 19, 17, 32, 33, 18, 10, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  1, 15, 23, 16,  0, 24, 13, 20,  7, 22,  9,  4, 12,  6,  8, /* 6X */
	21, 34,  5,  3,  2, 19, 17, 32, 33, 18, 10, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, 55, SYM, 42, SYM, 56, 46, SYM, SYM, 37, 52, 57, 58, SYM, 48, 59, /* AX */
	SYM, 60, SYM, 42, SYM, 61, 46, SYM, SYM, 37, 52, 62, 63, SYM, 48, 64, /* BX */
	65, 11, 40, 36, 35, 66, 38, 39, 41, 14, 50, 67, 53, 28, 45, 68, /* CX */
	49, 43, 54, 26, 69, 27, 25, SYM, 44, 70, 30, 31, 29, 47, 51, 71, /* DX */
	72, 11, 40, 36, 35, 73, 38, 39, 41, 14, 50, 74, 53, 28, 45, 75, /* EX */
	49, 43, 54, 26, 76, 27, 25, SYM, 44, 77, 30, 31, 29, 47, 51, SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Windows_1250_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  1, 15, 23, 16,  0, 24, 13, 20,  7, 22,  9,  4, 12,  6,  8, /* 4X */
	21, 34,  5,  3,  2, 19, 17, 32, 33, 18, 10, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  1, 15, 23, 16,  0, 24, 13, 20,  7, 22,  9,  4, 12,  6,  8, /* 6X */
	21, 34,  5,  3,  2, 19, 17, 32, 33, 18, 10, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, ILL, SYM, SYM, SYM, SYM, ILL, SYM, 37, SYM, 46, 78, 48, 79, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, 37, SYM, 46, 80, 48, 81, /* 9X */
	SYM, SYM, SYM, 42, SYM, 82, SYM, SYM, SYM, SYM, 52, SYM, SYM, SYM, SYM, 83, /* AX */
	SYM, SYM, SYM, 42, SYM, SYM, SYM, SYM, SYM, 84, 52, SYM, 85, SYM, 86, 87, /* BX */
	88, 11, 40, 36, 35, 89, 38, 39, 41, 14, 50, 90, 53, 28, 45, 91, /* CX */
	49, 43, 54, 26, 92, 27, 25, SYM, 44, 93, 30, 31, 29, 47, 51, 94, /* DX */
	95, 11, 40, 36, 35, 96, 38, 39, 41, 14, 50, 97, 53, 28, 45, 98, /* EX */
	49, 43, 54, 26, 99, 27, 25, SYM, 44, 100, 30, 31, 29, 47, 51, SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 1084
 * First 512 sequences: 0.9748272224933486
 * Next 512 sequences (512-1024): 0.024983863604162403
 * Rest: 0.0001889139024889644
 * Negative sequences: TODO
 */
static const uint8 HungarianLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 0, 2, 2, 0, 0,
	3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 2, 2, 1, 2, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 3, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 2, 2, 3, 3, 3, 3, 3, 2,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 3, 2, 3, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 1, 3, 3, 3, 2, 3, 3, 2, 3, 0, 2, 2, 2, 2,
	3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 3, 3, 0, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 0, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 2, 2, 2, 2, 2, 3, 3, 2, 3, 3, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 2, 2, 3, 3, 3, 3, 3, 2, 2,
	1, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 3, 3, 2, 3, 3, 3, 2, 2, 2, 3, 3, 3, 2, 0, 0, 0, 2, 0, 0, 0,
	3, 3, 3, 2, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 2, 2, 3, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 2, 3, 2, 2, 2,
	0, 1, 3, 3, 3, 3, 3, 2, 2, 3, 3, 0, 3, 3, 2, 3, 3, 3, 0, 0, 2, 3, 2, 3, 0, 0, 0, 0, 0, 2, 0, 0,
	3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 1, 3, 3, 3, 2, 2, 3, 1, 2, 2, 2, 2, 2, 3, 3, 3, 2, 2, 2,
	3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 3, 3, 2, 3, 2, 2, 3, 2, 3, 2, 2, 3, 2, 2, 3, 3, 3, 3, 2, 2, 2,
	3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 0, 3, 0, 2, 3, 2, 2, 2, 1, 2, 2, 0, 2, 1, 2, 3, 2, 3, 3, 2, 2, 2,
	3, 3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 3, 1, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
	3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 3, 3, 0, 3, 3, 2, 2, 2, 2, 2, 2, 3, 2, 0, 0, 0, 1, 0, 0, 0,
	3, 3, 2, 2, 2, 2, 2, 3, 3, 2, 0, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2, 0, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 3, 1, 2, 3, 2, 2, 2, 2, 3, 2, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 0,
	3, 3, 3, 2, 2, 2, 3, 2, 3, 2, 2, 3, 2, 2, 3, 2, 3, 2, 0, 3, 2, 2, 2, 2, 2, 2, 3, 0, 2, 2, 3, 2,
	3, 3, 2, 3, 2, 2, 2, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 2, 2, 2, 3, 0, 0, 2, 2, 2, 2, 0, 3, 0, 0, 0,
	3, 3, 2, 2, 2, 3, 2, 3, 3, 0, 0, 2, 2, 2, 3, 2, 2, 2, 2, 3, 0, 2, 2, 2, 2, 3, 2, 3, 2, 3, 2, 2,
	2, 0, 3, 3, 3, 3, 3, 0, 0, 3, 3, 0, 2, 3, 0, 3, 3, 3, 0, 0, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 3, 0, 0, 2, 3, 3, 2, 2, 2, 0, 0, 1, 2, 2, 0,
	2, 2, 3, 3, 3, 3, 2, 3, 2, 3, 3, 2, 2, 2, 2, 3, 3, 2, 0, 0, 2, 2, 3, 2, 2, 1, 0, 0, 1, 2, 1, 0,
	0, 2, 3, 2, 2, 3, 3, 2, 2, 2, 3, 0, 3, 3, 0, 2, 2, 3, 0, 2, 1, 2, 3, 2, 2, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 3, 2, 3, 2, 3, 0, 0, 3, 2, 0, 2, 3, 0, 0, 2, 2, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 3, 3, 3, 2, 3, 0, 0, 2, 2, 0, 0, 3, 0, 2, 2, 2, 0, 0, 2, 2, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 3, 2, 2, 2, 0, 3, 2, 0, 2, 2, 0, 2, 2, 3, 0, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
};
//
// Russian
//
//KOI8-R language model
//Character Mapping Table:
static const uchar KOI8R_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, //80
	207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, //90
	223, 224, 225, 68, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, //a0
	238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, NUM, SYM, //b0
	27,  3, 21, 28, 13,  2, 39, 19, 26,  4, 23, 11,  8, 12,  5,  1, //c0
	15, 16,  9,  7,  6, 14, 24, 10, 17, 18, 20, 25, 30, 29, 22, 54, //d0
	59, 37, 44, 58, 41, 48, 53, 46, 55, 42, 60, 36, 49, 38, 31, 34, //e0
	35, 43, 45, 32, 40, 52, 56, 33, 61, 62, 51, 57, 47, 63, 50, 70, //f0
};

static const uchar win1251_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 210, 211, 212, 213, 214, ILL, 216, 217, 218, 219, 220, 221, 222,
	223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
	239, 240, 241, 242, 243, 244, 245, 246, 68, 247, 248, 249, 250, 251, NUM, SYM,
	37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
	45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
	3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
	9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
};

static const uchar latin5_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
	223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
	37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
	45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
	3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
	9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
	239, 68, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, NUM, CTR,
};

static const uchar macCyrillic_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
	45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
	223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
	239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, NUM, 68, 16,
	3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
	9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, CTR,
};

static const uchar IBM855_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	191, 192, 193, 194, 68, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
	206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 27, 59, 54, 70,
	3, 37, 21, 44, 28, 58, 13, 41,  2, 48, 39, 53, 19, 46, 218, 219,
	220, 221, 222, 223, 224, 26, 55,  4, 42, 225, 226, 227, 228, 23, 60, 229,
	230, 231, 232, 233, 234, 235, 11, 36, 236, 237, 238, 239, 240, 241, 242, 243,
	8, 49, 12, 38,  5, 31,  1, 34, 15, 244, 245, 246, 247, 35, 16, 248,
	43,  9, 45,  7, 32,  6, 40, 14, 52, 24, 56, 10, 33, 17, 61, 249,
	250, 18, 62, 20, 51, 25, 57, 30, 47, 29, 63, 22, 50, 251, NUM, CTR,
};

static const uchar IBM866_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, //00
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, //10
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, //20
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, //30
	SYM, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 74, 153, 75, 154, //40
	155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, SYM, SYM, SYM, SYM, SYM, //50
	SYM, 71, 172, 66, 173, 65, 174, 76, 175, 64, 176, 177, 77, 72, 178, 69, //60
	67, 179, 78, 73, 180, 181, 79, 182, 183, 184, 185, SYM, SYM, SYM, SYM, SYM, //70
	37, 44, 33, 46, 41, 48, 56, 51, 42, 60, 36, 49, 38, 31, 34, 35,
	45, 32, 40, 52, 53, 55, 58, 50, 57, 63, 70, 62, 61, 47, 59, 43,
	3, 21, 10, 19, 13,  2, 24, 20,  4, 23, 11,  8, 12,  5,  1, 15,
	191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
	207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222,
	223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238,
	9,  7,  6, 14, 39, 26, 28, 22, 25, 29, 54, 18, 17, 30, 27, 16,
	239, 68, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, NUM, CTR,
};

//Model Table:
//total sequences: 100%
//first 512 sequences: 97.6601%
//first 1024 sequences: 2.3389%
//rest  sequences:      0.1237%
//negative sequences:   0.0009%
static const uint8 RussianLangModel[] =
{
	0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 3, 3, 3, 3, 1, 3, 3, 3, 2, 3, 2, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2, 2, 2, 2, 2, 0, 0, 2,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1,
	0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 1, 3, 3, 1, 3, 3, 3, 3, 2, 2, 3, 0, 2, 2, 2, 3, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 3, 3, 2, 1, 2, 2, 0, 1, 2, 2, 2, 2, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 0, 2, 2, 3, 3, 2, 1, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 1, 2, 3, 2, 2, 3, 2, 3, 3, 3, 3, 2, 2, 3, 0, 3, 2, 2, 3, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 0, 3, 3, 3, 2, 2, 2, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 0, 1, 3, 2, 1, 2, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 1, 3, 0, 1, 1, 1, 1, 2, 1, 1, 0, 2, 2, 2, 1, 2, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 2, 2, 1, 3, 2, 3, 2, 3, 2, 1, 2, 2, 0, 1, 1, 2, 1, 2, 1, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 3, 3, 2, 2, 2, 2, 0, 2, 2, 2, 2, 3, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 2, 0, 0, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 3, 2, 2, 3, 3, 0, 2, 1, 0, 3, 2, 3, 2, 3, 0, 0, 1, 2, 0, 0, 1, 0, 1, 2, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 0, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 1, 2, 2, 0, 0, 2, 3, 2, 2, 2, 3, 2, 3, 2, 2, 3, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 0, 2, 3, 2, 3, 0, 1, 2, 3, 3, 2, 0, 2, 3, 0, 0, 2, 3, 2, 2, 0, 1, 3, 1, 3, 2, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 3, 0, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 1, 3, 2, 0, 0, 2, 2, 3, 3, 3, 2, 3, 3, 0, 2, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 2, 2, 2, 3, 3, 0, 0, 1, 1, 1, 1, 1, 2, 0, 0, 1, 1, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2, 3, 3, 2, 3, 2, 0, 2, 1, 0, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 3, 1, 3, 2, 3, 1, 1, 2, 1, 0, 2, 2, 2, 2, 1, 3, 1, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 3, 3, 3, 3, 3, 1, 2, 2, 1, 3, 1, 0, 3, 0, 0, 3, 0, 0, 0, 1, 1, 0, 1, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 2, 1, 1, 3, 3, 3, 2, 2, 1, 2, 2, 3, 1, 1, 2, 0, 0, 2, 2, 1, 3, 0, 0, 2, 1, 1, 2, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 3, 3, 1, 2, 2, 2, 1, 2, 1, 3, 3, 1, 1, 2, 1, 2, 1, 2, 2, 0, 2, 0, 0, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 3, 2, 1, 3, 2, 2, 3, 2, 0, 3, 2, 0, 3, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 2, 3, 3, 3, 2, 2, 2, 3, 3, 1, 2, 1, 2, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0, 2, 1, 1, 1, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 1, 2, 1, 2, 3, 3, 2, 2, 1, 2, 2, 3, 0, 2, 1, 0, 0, 2, 2, 3, 2, 1, 2, 2, 2, 2, 2, 3, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 1, 1, 0, 1, 1, 2, 2, 1, 1, 3, 0, 0, 1, 3, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 3, 3, 3, 2, 0, 0, 0, 2, 1, 0, 1, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 1, 0, 0, 2, 3, 2, 2, 2, 1, 2, 2, 2, 1, 2, 1, 0, 0, 1, 1, 1, 0, 2, 0, 1, 1, 1, 0, 0, 1, 1,
	1, 0, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 3, 0, 0, 0, 0, 1, 0, 0, 0, 0, 3, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
	1, 0, 1, 0, 1, 2, 0, 0, 1, 1, 2, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0,
	2, 2, 3, 2, 2, 2, 3, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0,
	3, 3, 3, 2, 2, 2, 2, 3, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 3, 1, 2, 1, 2, 0, 0, 1, 1, 0, 1, 0, 2, 1,
	1, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0,
	2, 0, 0, 1, 0, 3, 2, 2, 2, 2, 1, 2, 1, 2, 1, 2, 0, 0, 0, 2, 1, 2, 2, 1, 1, 2, 2, 0, 1, 1, 0, 2,
	1, 1, 1, 1, 1, 0, 1, 1, 1, 2, 1, 1, 1, 2, 1, 0, 1, 2, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0, 1,
	1, 3, 2, 2, 2, 1, 1, 1, 2, 3, 0, 0, 0, 0, 2, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1,
	1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 2, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
	2, 3, 2, 3, 2, 1, 2, 2, 2, 2, 1, 0, 0, 0, 2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 2, 1,
	1, 1, 2, 1, 0, 2, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	3, 0, 0, 1, 0, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 1, 2, 1, 1, 1, 2, 2, 0, 0, 0, 1, 2,
	1, 1, 1, 1, 1, 0, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
	2, 3, 2, 3, 3, 2, 0, 1, 1, 1, 0, 0, 1, 0, 2, 0, 1, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
	2, 3, 3, 3, 3, 1, 2, 2, 2, 2, 0, 1, 1, 0, 2, 1, 1, 1, 2, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 3, 3, 2, 0, 0, 1, 1, 2, 2, 1, 0, 0, 2, 0, 1, 1, 3, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 2, 1,
	1, 1, 2, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0,
	1, 3, 2, 3, 2, 1, 0, 0, 2, 2, 2, 0, 1, 0, 2, 0, 1, 1, 1, 0, 1, 0, 0, 0, 3, 0, 1, 1, 0, 0, 2, 1,
	1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 2, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0,
	3, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0, 1, 2, 1, 0, 1, 0, 1,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0, 1,
	3, 0, 0, 0, 0, 2, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1,
	1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1,
	1, 3, 3, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 1, 2, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 2, 1,
	0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	2, 3, 2, 3, 2, 0, 0, 0, 0, 1, 1, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1,
	1, 1, 2, 0, 1, 2, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0,
	1, 3, 2, 2, 2, 1, 0, 0, 2, 2, 1, 0, 1, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1,
	0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 0, 2, 3, 1, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0, 1, 0, 1, 0, 2, 1, 1, 1, 0, 0, 0, 0, 1,
	1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	2, 0, 2, 0, 0, 1, 0, 3, 2, 1, 2, 1, 2, 2, 0, 1, 0, 0, 0, 2, 1, 0, 0, 2, 1, 1, 1, 1, 0, 2, 0, 2,
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1,
	1, 2, 2, 2, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 2, 0, 0, 2, 0,
	1, 0, 1, 1, 1, 2, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 0,
	2, 1, 2, 2, 2, 0, 3, 0, 1, 1, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	1, 2, 2, 3, 2, 2, 0, 0, 1, 1, 2, 0, 1, 2, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
	2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 0, 1, 0, 0, 0, 1, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 2, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1,
	1, 2, 2, 2, 2, 0, 1, 0, 2, 2, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
	0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 0, 0, 0, 2, 2, 2, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 0, 0, 1, 1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 2, 0, 0, 0, 1,
	0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 1, 2, 0, 2, 1, 1, 1, 1, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1,
	0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 2, 1, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	1, 0, 0, 0, 0, 2, 0, 1, 2, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0,
	0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
};
//
// Language model for: Spanish 
//
// Generated by BuildLangModel.py On: 2015-12-12 18:39:02.290370
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_1_Spanish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 4X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 6X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 52, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 53, 22, 41, 43, /* CX */
	49, 29, 38, 19, 50, 54, 34, SYM, 44, 51, 30, 55, 32, 42, 56, 57, /* DX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 58, 22, 41, 43, /* EX */
	49, 29, 38, 19, 50, 59, 34, SYM, 44, 51, 30, 60, 32, 42, 61, 62, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_15_Spanish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 4X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 6X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, 63, SYM, 64, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, 65, 66, SYM, SYM, 67, SYM, SYM, SYM, 68, 69, 70, SYM, /* BX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 71, 22, 41, 43, /* CX */
	49, 29, 38, 19, 50, 72, 34, SYM, 44, 51, 30, 73, 32, 42, 74, 75, /* DX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 76, 22, 41, 43, /* EX */
	49, 29, 38, 19, 50, 77, 34, SYM, 44, 51, 30, 78, 32, 42, 79, 80, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Windows_1252_Spanish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 4X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  1, 14, 10,  8,  0, 16, 15, 20,  5, 23, 27,  7, 12,  3,  2, /* 6X */
	13, 21,  6,  4,  9, 11, 18, 31, 28, 17, 24, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, 81, SYM, SYM, SYM, SYM, SYM, SYM, 82, SYM, 83, ILL, 84, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 85, SYM, 86, ILL, 87, 88, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 89, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 90, 22, 41, 43, /* CX */
	49, 29, 38, 19, 50, 91, 34, SYM, 44, 51, 30, 92, 32, 42, 93, 94, /* DX */
	33, 25, 39, 46, 37, 45, 47, 35, 36, 26, 48, 40, 95, 22, 41, 43, /* EX */
	49, 29, 38, 19, 50, 96, 34, SYM, 44, 51, 30, 97, 32, 42, 98, 99, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 897
 * First 512 sequences: 0.9970385677528184
 * Next 512 sequences (512-1024): 0.0029614322471815486
 * Rest: 4.597017211338539e-17
 * Negative sequences: TODO
 */
static const uint8 SpanishLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 3, 3, 3, 2, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 2, 3, 3, 2, 2, 3, 3, 2, 2, 3, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 3, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 0, 2, 3, 3, 3, 0, 0, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 3, 3, 3, 2, 0, 3, 2, 2,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 2, 3, 3, 2, 2, 0, 2, 2, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 3, 2, 3, 3, 3, 3, 2, 3, 2, 2, 3, 3, 2, 0, 0, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 3, 2, 2, 3, 2, 3, 3, 0, 3, 2, 2, 3, 3, 0, 0, 0, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 2, 3, 0, 3, 3, 2, 3, 0, 2, 3, 3, 3, 0, 0, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 3, 3, 3, 2, 2, 3, 0, 3, 2, 0, 3, 2, 0, 3, 3, 2, 2, 0, 3, 2, 2,
	3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 0, 2, 2, 2, 3, 3, 0, 3, 2, 0, 3, 3, 2, 0, 0, 3, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 2, 3, 2, 3, 2, 2, 3, 3, 0, 3, 2, 2, 0, 0, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 0, 2, 2, 2, 2, 2, 3, 3, 0, 3, 2, 2, 2, 3, 2, 0, 0, 3, 2, 3,
	3, 3, 3, 2, 2, 3, 3, 3, 2, 3, 2, 3, 2, 2, 2, 2, 3, 2, 0, 3, 0, 0, 3, 2, 0, 2, 2, 2, 0, 0, 3, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 0, 3, 2, 0, 0, 2, 2, 2, 2, 2, 0, 0, 2, 2, 0,
	3, 3, 3, 2, 2, 3, 2, 2, 2, 0, 2, 3, 0, 2, 0, 2, 2, 2, 2, 3, 0, 0, 3, 0, 0, 2, 3, 2, 0, 0, 0, 0, 0,
	0, 0, 0, 3, 3, 0, 3, 3, 3, 3, 3, 0, 3, 3, 2, 3, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 0, 2, 0, 2, 3, 2, 2, 2, 0, 3, 2, 2, 2, 3, 0, 2, 0, 2, 2, 2,
	2, 3, 2, 0, 2, 2, 0, 2, 2, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 0, 2, 2, 3, 3, 3, 2, 3, 2, 3, 3, 3, 0, 2, 0, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 0, 3, 2, 2, 2, 2, 0, 3, 2, 2, 0, 0, 0, 0, 0, 3, 0, 0, 2, 2, 0, 2, 3, 0, 0, 0, 2, 0, 2,
	3, 3, 3, 2, 0, 3, 2, 0, 2, 2, 2, 3, 2, 2, 2, 3, 0, 2, 0, 3, 2, 3, 2, 0, 3, 3, 2, 2, 0, 0, 2, 0, 0,
	2, 0, 0, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 2, 0, 2, 2, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0,
	2, 3, 2, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 2, 2, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 3, 0, 2, 2, 2, 3, 2, 0, 2, 0, 2, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0,
	3, 3, 3, 2, 2, 3, 2, 2, 2, 3, 3, 3, 2, 3, 2, 0, 2, 2, 3, 2, 2, 2, 0, 2, 0, 2, 2, 2, 3, 0, 0, 2, 0,
	3, 3, 3, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	2, 3, 2, 3, 3, 0, 2, 3, 2, 3, 2, 0, 3, 2, 3, 0, 2, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2, 0, 0, 0,
	3, 3, 3, 3, 2, 3, 2, 2, 2, 2, 2, 2, 0, 0, 2, 0, 2, 2, 0, 0, 2, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 2, 0,
	3, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: Thai
//
// Generated by BuildLangModel.py On: 2015-12-04 03:05:06.182099
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Tis_620_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 66, 70, 67, 80, 78, 87, 85, 73, 79, 93, 88, 84, 68, 77, 81, /* 4X */
	75, 101, 74, 61, 71, 86, 96, 90, 103, 100, 99, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 35, 64, 48, 52, 32, 60, 65, 54, 36, 97, 76, 46, 56, 41, 40, /* 6X */
	59, 104, 43, 45, 44, 55, 72, 82, 94, 57, 92, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	ILL,  3, 23, 105, 15, 106, 89,  5, 21, 63, 26, 31, 102, 42, 69, 58, /* AX */
	49, 91, 83, 34,  9, 17, 30, 12, 39,  1, 16, 19, 33, 62, 22, 47, /* BX */
	38,  7, 10,  2, 50, 11, 107,  8, 28, 37, 13, 18, 98,  4, 53, 95, /* CX */
	14, SYM,  0, 29, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, ILL, ILL, ILL, SYM, /* DX */
	6, 20, 27, 24, 25, 108, 51, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 109, /* EX */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, 110, 111, ILL, ILL, ILL, ILL, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_11_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM, 66, 70, 67, 80, 78, 87, 85, 73, 79, 93, 88, 84, 68, 77, 81, /* 4X */
	75, 101, 74, 61, 71, 86, 96, 90, 103, 100, 99, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM, 35, 64, 48, 52, 32, 60, 65, 54, 36, 97, 76, 46, 56, 41, 40, /* 6X */
	59, 104, 43, 45, 44, 55, 72, 82, 94, 57, 92, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM,  3, 23, 112, 15, 113, 89,  5, 21, 63, 26, 31, 102, 42, 69, 58, /* AX */
	49, 91, 83, 34,  9, 17, 30, 12, 39,  1, 16, 19, 33, 62, 22, 47, /* BX */
	38,  7, 10,  2, 50, 11, 114,  8, 28, 37, 13, 18, 98,  4, 53, 95, /* CX */
	14, SYM,  0, 29, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, ILL, ILL, ILL, SYM, /* DX */
	6, 20, 27, 24, 25, 115, 51, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, 116, /* EX */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, 117, 118, ILL, ILL, ILL, ILL, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

/* Model Table:
 * Total sequences: 2324
 * First 512 sequences: 0.8815720594354438
 * Next 512 sequences (512-1024): 0.0920860122682917
 * Rest: 0.026341928296264486
 * Negative sequences: TODO
 */
static const uint8 ThaiLangModel[] =
{
	0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 3,
	0, 2, 3, 0, 0, 3, 2, 3, 0, 0, 2, 0, 0, 0, 0, 2, 0, 1, 1, 1, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3,
	0, 3, 0, 0, 0, 1, 3, 3, 0, 0, 1, 0, 0, 0, 0, 2, 0, 2, 1, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 1, 3, 2,
	0, 2, 3, 0, 0, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 2, 3,
	0, 2, 1, 0, 0, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 3, 1, 0, 1, 0, 0, 0, 0, 3, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 2, 2, 1, 2, 2, 2,
	0, 2, 0, 0, 0, 0, 2, 2, 0, 0, 1, 0, 0, 0, 0, 2, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2,
	0, 3, 0, 0, 0, 1, 2, 2, 0, 0, 1, 0, 0, 0, 0, 2, 0, 1, 1, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1,
	0, 3, 3, 3, 3, 2, 0, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 0, 3, 3, 3, 0, 0, 3, 0, 3, 0, 1, 3,
	0, 2, 0, 0, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3,
	3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 1, 0, 2, 1,
	0, 2, 2, 0, 1, 2, 2, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 3, 2, 2, 2, 3, 3, 3, 2, 2, 2, 2, 2, 2, 0, 2, 2,
	0, 1, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 3, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 0, 3, 2, 3, 2, 2, 3, 2, 2, 3, 3, 3, 2, 2, 1, 3, 2, 1,
	0, 1, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 1, 2, 2,
	0, 2, 0, 0, 0, 0, 3, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 3, 2, 2, 2, 2, 1, 3, 2, 2, 2, 2, 1, 3, 1, 2,
	0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	3, 3, 3, 1, 2, 1, 2, 1, 2, 3, 3, 1, 1, 2, 2, 3, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, 3, 0, 1,
	0, 0, 0, 0, 0, 1, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 2, 2, 3, 3, 3, 2, 2, 1, 1, 1, 2, 2, 1, 2, 1, 3, 3, 2,
	0, 1, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 3, 3, 3, 3, 1, 3, 3, 3, 3, 3, 2, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 2, 0, 2, 2,
	0, 2, 1, 0, 0, 0, 2, 2, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1,
	3, 3, 3, 1, 3, 2, 2, 3, 3, 2, 2, 3, 1, 1, 2, 2, 1, 2, 1, 2, 1, 3, 1, 1, 1, 1, 1, 2, 0, 3, 0, 1,
	0, 0, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 1, 3, 2, 3, 3, 2, 3, 3, 3, 1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 2, 2, 2, 2, 2, 2,
	0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1,
	3, 3, 3, 3, 3, 1, 2, 1, 2, 1, 3, 2, 2, 2, 3, 1, 2, 2, 1, 1, 2, 1, 1, 2, 2, 1, 1, 2, 1, 3, 3, 1,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 1, 2, 1, 0, 3, 3, 1, 2, 3, 1, 1, 1, 0, 0, 3, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 3, 3, 1, 2, 1, 2, 2, 2, 3, 2, 2, 2, 1, 1, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 1, 1, 0, 2, 1,
	0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0,
	0, 3, 3, 3, 3, 1, 0, 3, 2, 2, 2, 3, 3, 3, 0, 3, 3, 3, 3, 3, 0, 1, 2, 2, 0, 0, 1, 0, 0, 0, 3, 3,
	0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 3, 3, 3, 1, 3, 2, 2, 2, 1, 1, 2, 2, 3, 2, 1, 2, 1, 1, 2, 3, 3, 2, 2, 2, 1, 2, 0, 3, 1, 2,
	0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 1, 3, 2, 3, 1, 2, 2, 3, 2, 3, 3, 3, 2, 0, 1, 3, 1, 1, 1, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0,
	0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 1, 1, 3, 0, 1, 1, 2, 1, 2, 1, 2, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
	0, 0, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 0, 3, 0, 0, 0, 0, 0, 2, 1, 0, 0, 2, 0, 1, 1, 3, 3, 1, 0, 3, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 3, 2, 2, 0, 0, 3, 3, 3, 0, 2, 3, 1, 0, 2, 2, 2, 2, 3, 0, 1, 1, 3, 0, 0, 1, 0, 0, 0, 1, 2,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 1, 2, 3, 1, 2, 2, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 3, 2, 3, 0, 0, 2, 1, 3, 2, 3, 3, 1, 0, 3, 2, 3, 1, 2, 0, 2, 2, 1, 0, 0, 1, 0, 1, 0, 1, 2,
	0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	3, 3, 2, 2, 2, 0, 2, 2, 2, 1, 2, 1, 2, 2, 0, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 1, 0, 1, 1,
	0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 3, 3, 3, 2, 2, 3, 2, 2, 2, 1, 3, 2, 2, 0, 3, 2, 2, 3, 1, 3, 1, 2, 2, 3, 2, 1, 2, 1, 0, 2, 1,
	0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 2, 1, 1, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 0, 1, 0,
	0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	3, 3, 1, 1, 3, 2, 2, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 2, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1,
	0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 2, 2, 0, 0, 0, 2, 3, 0, 3, 2, 3, 3, 0, 2, 0, 0, 0, 2, 0, 1, 2, 2, 1, 0, 2, 2, 1, 0, 0,
	1, 2, 0, 1, 0, 1, 1, 1, 1, 1, 2, 3, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 3, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 2, 0, 0, 0, 1, 3, 0, 3, 3, 2, 3, 0, 2, 0, 0, 0, 2, 0, 1, 1, 2, 2, 0, 2, 1, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 3, 1, 0, 0, 0, 3, 3, 0, 2, 3, 3, 2, 0, 3, 0, 0, 0, 2, 0, 1, 1, 2, 0, 0, 1, 1, 0, 0, 0,
	3, 1, 1, 2, 1, 0, 1, 1, 1, 1, 2, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1,
	0, 1, 3, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0,
	3, 0, 2, 1, 1, 0, 0, 1, 0, 0, 1, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 1, 3, 1, 2, 1, 1, 2, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 1, 1, 0, 0, 0, 1, 3, 0, 3, 2, 2, 2, 0, 2, 0, 0, 0, 2, 0, 1, 2, 2, 1, 0, 2, 3, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 1, 3, 2, 1, 0, 2, 0, 0, 0, 3, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0,
	3, 1, 1, 1, 1, 0, 2, 1, 1, 0, 0, 1, 2, 1, 0, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 3, 3, 0, 0, 0, 2, 2, 0, 2, 2, 2, 1, 0, 2, 0, 0, 0, 2, 0, 1, 1, 1, 2, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 2, 3, 0, 0, 0, 2, 1, 0, 2, 2, 2, 1, 0, 1, 0, 0, 0, 1, 0, 3, 2, 1, 2, 0, 1, 1, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 1, 2, 0, 0, 0, 2, 1, 0, 1, 3, 2, 1, 0, 2, 0, 0, 0, 1, 0, 2, 1, 1, 1, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0,
	1, 1, 3, 2, 2, 0, 2, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 1, 2, 1, 1, 0, 1, 0, 0, 0, 0, 0, 2, 1, 0, 1, 0, 0, 0, 0, 0, 0,
	3, 1, 1, 1, 2, 0, 1, 2, 1, 0, 0, 0, 1, 2, 0, 1, 2, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1,
	0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 3, 0, 0, 0, 0, 0, 2, 0, 0, 1, 0, 0, 1, 0, 2, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0,
	0, 0, 0, 0, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 2, 0, 0, 2, 0, 0, 0,
	2, 1, 1, 0, 2, 0, 2, 1, 1, 1, 1, 2, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 2, 2, 0, 0, 0, 2, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 1, 0, 0, 0, 0, 2, 0, 2, 2, 2, 2, 0, 2, 0, 0, 0, 2, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 2, 2, 0, 0, 0, 2, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 2, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,
	1, 0, 1, 2, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 2, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 1, 2, 0, 0, 0, 2, 1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 2, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0,
	0, 1, 2, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: Turkish
//
// Generated by BuildLangModel.py On: 2015-12-04 02:24:44.730727
// 
/* Character Mapping Table:
 * ILL: illegal character.
 * CTR: control character specific to the charset.
 * RET: carriage/return.
 * SYM: symbol (punctuation) that does not belong to word.
 * NUM: 0 - 9.
 *
 * Other characters are ordered by probabilities
 * (0 is the most common character in the language).
 *
 * Orders are generic to a language. So the codepoint with order X in
 * CHARSET1 maps to the same character as the codepoint with the same
 * order X in CHARSET2 for the same language.
 * As such, it is possible to get missing order. For instance the
 * ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
 * even though they are both used for French. Same for the euro sign.
 */
static const uchar Iso_8859_3_Turkish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  0, 15, 21,  7,  1, 26, 22, 19,  6, 28,  9,  5, 11,  3, 14, /* 4X */
	23, 34,  4, 10,  8, 12, 20, 29, 32, 13, 18, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  0, 15, 21,  7,  1, 26, 22, 19,  2, 28,  9,  5, 11,  3, 14, /* 6X */
	23, 34,  4, 10,  8, 12, 20, 29, 32, 13, 18, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, 48, SYM, SYM, SYM, ILL, 49, SYM, SYM,  2, 17, 25, 50, SYM, ILL, 51, /* AX */
	SYM, 52, SYM, SYM, SYM, SYM, 53, SYM, SYM,  6, 17, 25, 54, SYM, ILL, 55, /* BX */
	41, 36, 30, ILL, 39, 56, 57, 24, 42, 33, 58, 45, 59, 37, 31, 60, /* CX */
	ILL, 47, 61, 38, 62, 63, 27, SYM, 64, 65, 40, 35, 16, 66, 67, 68, /* DX */
	41, 36, 30, ILL, 39, 69, 70, 24, 42, 33, 71, 45, 72, 37, 31, 73, /* EX */
	ILL, 47, 74, 38, 75, 76, 27, SYM, 77, 78, 40, 35, 16, 79, 80, SYM, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Iso_8859_9_Turkish_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  0, 15, 21,  7,  1, 26, 22, 19,  6, 28,  9,  5, 11,  3, 14, /* 4X */
	23, 34,  4, 10,  8, 12, 20, 29, 32, 13, 18, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  0, 15, 21,  7,  1, 26, 22, 19,  2, 28,  9,  5, 11,  3, 14, /* 6X */
	23, 34,  4, 10,  8, 12, 20, 29, 32, 13, 18, SYM, SYM, SYM, SYM, CTR, /* 7X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 8X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 81, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	41, 36, 30, 44, 39, 82, 46, 24, 42, 33, 83, 45, 84, 37, 31, 85, /* CX */
	25, 47, 86, 38, 87, 88, 27, SYM, 43, 89, 40, 35, 16,  2, 17, 90, /* DX */
	41, 36, 30, 44, 39, 91, 46, 24, 42, 33, 92, 45, 93, 37, 31, 94, /* EX */
	25, 47, 95, 38, 96, 97, 27, SYM, 43, 98, 40, 35, 16,  6, 17, 99, /* FX */
};
// X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF 
// 
// Model Table: Total sequences: 935; First 512 sequences: 0.991865243864388; Next 512 sequences (512-1024): 0.008134756135611957; 
//   Rest: 2.949029909160572e-17; Negative sequences: TODO
// 
static const uint8 TurkishLangModel[] =
{
	3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 0, 2, 2, 2, 2, 0,
	3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 2, 0, 3, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 2, 2, 0, 2, 0, 2, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, 0, 3, 2, 2, 2, 2, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2, 3, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 2, 3, 3, 2, 3, 0, 3, 2, 2, 2, 2, 3, 0, 2, 2, 2,
	3, 2, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 0, 3, 3, 2, 3, 3, 2, 3, 2, 3, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 2, 3, 3, 3, 2, 3, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 3, 2, 3, 2, 2, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 3, 3, 2, 2, 2, 3, 0, 2, 3, 2, 2, 3, 2, 2, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 2, 3, 2, 2, 2, 2, 3, 0, 2, 3, 2, 2, 3, 0, 0, 0, 0, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 3, 3, 3, 2, 3, 3, 0, 2, 3, 0, 2, 2, 0, 0, 2, 2, 2,
	3, 3, 3, 2, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 0, 3, 2, 3, 2, 0, 2, 2, 0, 2, 3, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 2, 3, 2, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 0, 2, 2, 0, 0, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 0, 2, 3, 0, 2, 2, 0, 0, 2, 0, 2,
	3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 0, 2, 2, 2, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 3, 0, 2, 2, 3, 3, 3, 3, 3, 3, 0, 2, 2, 2, 2, 0, 2, 0, 0, 0, 3, 2, 2, 2, 0, 0, 2, 0, 0,
	2, 2, 2, 3, 3, 3, 0, 3, 3, 3, 3, 3, 0, 3, 2, 3, 0, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 0, 2, 3, 3, 2, 3, 3, 2, 3, 3, 2, 2, 3, 3, 2, 0, 2, 2, 2, 2, 2, 3, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0,
	3, 3, 3, 2, 2, 3, 3, 3, 2, 2, 0, 3, 3, 3, 3, 2, 3, 0, 2, 2, 0, 3, 3, 0, 0, 0, 0, 2, 0, 0, 2, 2, 0, 0, 0, 0,
	3, 3, 3, 3, 3, 3, 3, 2, 3, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 0, 2, 3, 0, 2, 0, 0, 2, 3, 2, 0, 2, 0, 2,
	3, 3, 3, 2, 3, 3, 2, 2, 0, 2, 3, 2, 3, 3, 3, 2, 2, 2, 2, 2, 3, 2, 2, 0, 0, 0, 2, 0, 0, 0, 2, 2, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 3, 2, 3, 0, 2, 3, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 2, 2, 2,
	3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 0, 3, 2, 3, 0, 3, 0, 2, 3, 2, 0, 2, 2, 0, 0, 2, 3, 2, 2, 2, 0, 0, 2, 0, 0,
	3, 3, 3, 0, 3, 3, 3, 2, 3, 2, 3, 3, 3, 2, 3, 2, 2, 0, 2, 3, 0, 2, 2, 3, 2, 0, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0,
	3, 3, 3, 0, 2, 3, 3, 2, 3, 2, 0, 3, 3, 2, 3, 2, 3, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 0, 3, 3, 3, 3, 0, 0, 0, 3, 3, 0, 0, 2, 3, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 3, 3, 3, 2, 3, 2, 2, 0, 3, 3, 3, 2, 2, 0, 0, 2, 0, 2, 2, 0, 2, 0, 2, 2, 2, 0, 2, 2, 0, 0, 0, 0,
	0, 0, 0, 3, 3, 3, 0, 3, 3, 3, 3, 3, 0, 3, 0, 2, 0, 2, 3, 2, 2, 0, 0, 2, 3, 3, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	3, 3, 3, 0, 0, 2, 2, 2, 0, 2, 0, 0, 3, 0, 3, 0, 2, 0, 0, 0, 0, 2, 2, 2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0,
	3, 3, 3, 2, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 3, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0,
	0, 0, 2, 3, 3, 3, 0, 3, 2, 2, 2, 2, 0, 2, 0, 2, 0, 2, 2, 3, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 2, 0, 2, 0, 2, 2, 0, 0, 2, 0, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	2, 0, 2, 2, 2, 2, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 2, 0, 2, 0, 0, 2, 2, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
//
// Language model for: Vietnamese
//
// Generated by BuildLangModel.py On: 2016-02-13 03:42:06.561440
// 
// Character Mapping Table:
// ILL: illegal character.
// CTR: control character specific to the charset.
// RET: carriage/return.
// SYM: symbol (punctuation) that does not belong to word.
// NUM: 0 - 9.
// 
// Other characters are ordered by probabilities (0 is the most common character in the language).
// 
// Orders are generic to a language. So the codepoint with order X in
// CHARSET1 maps to the same character as the codepoint with the same order X in CHARSET2 for the same language.
// As such, it is possible to get missing order. For instance the
// ligature of 'o' and 'e' exists in ISO-8859-15 but not in ISO-8859-1
// even though they are both used for French. Same for the euro sign.
// 
static const uchar Windows_1258_CharToOrderMap[] =
{
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  6, 17,  3, 22, 21, 66,  5,  1,  4, 75, 24, 14,  8,  0,  9, /* 4X */
	16, 36, 11, 19,  2,  7, 13, 69, 54, 20, 82, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  6, 17,  3, 22, 21, 66,  5,  1,  4, 75, 24, 14,  8,  0,  9, /* 6X */
	16, 36, 11, 19,  2,  7, 13, 69, 54, 20, 82, SYM, SYM, SYM, SYM, CTR, /* 7X */
	SYM, ILL, SYM, 101, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, 100, ILL, ILL, ILL, /* 8X */
	ILL, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, ILL, SYM, 100, ILL, ILL, 102, /* 9X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* AX */
	SYM, SYM, SYM, SYM, SYM, 103, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* BX */
	12, 15, 25, 51, 97, 104, 98, 91, 90, 62, 27, 105, SYM, 47, 106, 107, /* CX */
	10, 108, SYM, 33, 29, 46, 93, SYM, 94, 58, 67, 109, 96, 18, SYM, 99, /* DX */
	12, 15, 25, 51, 97, 110, 98, 91, 90, 62, 27, 111, SYM, 47, 112, 113, /* EX */
	10, 114, SYM, 33, 29, 46, 93, SYM, 94, 58, 67, 115, 96, 18, 116, 117, /* FX */
};
/*X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF */

static const uchar Viscii_CharToOrderMap[] =
{
	CTR, CTR, 88, CTR, CTR, 95, 77, CTR, CTR, CTR, RET, CTR, CTR, RET, CTR, CTR, /* 0X */
	CTR, CTR, CTR, CTR, 80, CTR, CTR, CTR, CTR, 79, CTR, CTR, CTR, CTR, 92, CTR, /* 1X */
	SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, SYM, /* 2X */
	NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, NUM, SYM, SYM, SYM, SYM, SYM, SYM, /* 3X */
	SYM,  6, 17,  3, 22, 21, 66,  5,  1,  4, 75, 24, 14,  8,  0,  9, /* 4X */
	16, 36, 11, 19,  2,  7, 13, 69, 54, 20, 82, SYM, SYM, SYM, SYM, SYM, /* 5X */
	SYM,  6, 17,  3, 22, 21, 66,  5,  1,  4, 75, 24, 14,  8,  0,  9, /* 6X */
	16, 36, 11, 19,  2,  7, 13, 69, 54, 20, 82, SYM, SYM, SYM, SYM, CTR, /* 7X */
	30, 57, 71, 65, 41, 43, 78, 49, 83, 89, 23, 45, 39, 74, 28, 32, /* 8X */
	53, 60, 84, 31, 37, 40, 38, 59, 42, 81, 44, 73, 35, 72, 48, 76, /* 9X */
	86, 57, 71, 65, 41, 43, 78, 49, 83, 89, 23, 45, 39, 74, 28, 32, /* AX */
	53, 60, 84, 87, 46, 31, 38, 59, 42, 56, 52, 55, 70, 46, 40, 18, /* BX */
	12, 15, 25, 61, 34, 51, 88, 95, 90, 62, 27, 85, 50, 47, 64, 76, /* CX */
	10, 52, 63, 33, 29, 30, 80, 55, 70, 58, 67, 79, 92, 68, 87, 18, /* DX */
	12, 15, 25, 61, 34, 51, 26, 77, 90, 62, 27, 85, 50, 47, 64, 73, /* EX */
	10, 56, 63, 33, 29, 86, 81, 44, 48, 58, 67, 72, 35, 68, 37, 26, /* FX */
};
// X0  X1  X2  X3  X4  X5  X6  X7  X8  X9  XA  XB  XC  XD  XE  XF 
// 
// Model Table: Total sequences: 1494; First 512 sequences: 0.9321889118082535; Next 512 sequences (512-1024): 0.06092051479986333;
//   Rest: 0.0068905733918831966; Negative sequences: TODO
// 
static const uint8 VietnameseLangModel[] =
{
	3, 3, 3, 3, 3, 3, 3, 2, 2, 3, 0, 2, 3, 1, 1, 1, 1, 2, 3, 3, 2, 3, 3, 3, 2, 1, 2,
	3, 0, 3, 2, 2, 2, 3, 1, 0, 1, 1, 2, 0, 0, 1, 0, 1, 0, 2, 2, 1, 0, 0, 0, 3, 0, 0, 2,
	2, 1, 2, 0, 3, 0, 3, 3, 2, 3, 0, 2, 3, 0, 2, 3, 0, 0, 3, 1, 3, 3, 1, 3, 1, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 3, 3, 2, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 2, 0,
	2, 3, 2, 2, 3, 1, 3, 3, 1, 3, 1, 3, 3, 2, 2, 3, 2, 0, 3, 2, 2, 3, 1, 3, 0, 3, 0,
	3, 1, 3, 3, 3, 3, 2, 3, 2, 0, 0, 2, 1, 2, 2, 2, 2, 0, 0, 1, 3, 2, 3, 2, 2, 2, 2, 0,
	2, 3, 2, 2, 3, 0, 3, 3, 2, 3, 0, 2, 2, 1, 2, 3, 1, 1, 2, 2, 2, 3, 1, 0, 2, 2, 0,
	0, 0, 3, 2, 3, 2, 3, 3, 3, 1, 1, 2, 0, 0, 2, 0, 3, 0, 0, 2, 0, 2, 2, 0, 2, 3, 1, 1,
	3, 1, 3, 3, 3, 3, 3, 2, 3, 3, 1, 3, 2, 2, 3, 3, 2, 2, 0, 3, 1, 3, 3, 3, 2, 0, 3,
	3, 3, 1, 0, 0, 3, 1, 3, 0, 2, 0, 2, 3, 3, 2, 0, 0, 2, 3, 0, 0, 0, 1, 0, 1, 0, 0, 2,
	2, 3, 2, 2, 3, 1, 3, 3, 1, 3, 0, 3, 3, 0, 2, 2, 0, 1, 3, 2, 2, 3, 1, 1, 1, 2, 3,
	0, 0, 3, 3, 1, 2, 2, 0, 1, 0, 2, 2, 0, 0, 1, 1, 3, 3, 0, 0, 0, 1, 1, 2, 1, 0, 3, 0,
	3, 2, 3, 3, 3, 2, 2, 3, 3, 3, 0, 3, 0, 2, 3, 0, 2, 3, 0, 3, 3, 2, 3, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2,
	3, 1, 3, 2, 3, 2, 3, 1, 3, 2, 0, 3, 1, 2, 3, 2, 2, 2, 0, 3, 3, 3, 2, 2, 2, 3, 0,
	2, 1, 3, 1, 3, 3, 0, 2, 0, 0, 0, 1, 0, 1, 3, 0, 3, 0, 0, 2, 2, 0, 3, 0, 2, 0, 3, 1,
	2, 1, 0, 2, 3, 0, 3, 3, 2, 3, 0, 0, 3, 0, 2, 3, 2, 2, 3, 2, 2, 3, 2, 0, 0, 1, 0,
	0, 2, 3, 3, 3, 2, 2, 1, 0, 0, 0, 2, 0, 3, 3, 0, 1, 2, 2, 0, 0, 3, 2, 2, 1, 2, 1, 1,
	3, 2, 3, 2, 3, 2, 3, 3, 3, 2, 0, 3, 3, 2, 3, 3, 2, 3, 0, 3, 2, 2, 3, 0, 2, 0, 0,
	0, 0, 0, 3, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2,
	0, 0, 0, 0, 3, 0, 3, 2, 0, 3, 0, 1, 3, 0, 0, 3, 0, 1, 3, 0, 0, 1, 0, 3, 0, 3, 0,
	2, 3, 3, 3, 3, 3, 3, 3, 2, 0, 1, 3, 3, 1, 3, 3, 3, 3, 3, 2, 2, 0, 1, 2, 2, 3, 3, 0,
	3, 2, 3, 2, 3, 2, 3, 3, 2, 3, 0, 3, 2, 2, 3, 2, 1, 2, 3, 3, 3, 3, 3, 0, 2, 1, 2,
	3, 1, 2, 2, 3, 2, 0, 2, 0, 0, 2, 2, 1, 0, 3, 3, 2, 3, 0, 1, 2, 2, 2, 3, 3, 1, 2, 0,
	3, 0, 0, 0, 3, 0, 0, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 3, 0, 3, 3, 0, 2, 0, 1, 3, 0, 1, 1, 0, 0, 2, 1, 1, 3, 1, 1, 0, 2, 1,
	2, 1, 2, 1, 0, 1, 0, 0, 0, 0, 2, 1, 0, 3, 2, 3, 3, 1, 3, 0, 3, 2, 3, 3, 3, 0, 0, 0,
	0, 2, 2, 1, 3, 2, 3, 3, 2, 3, 0, 0, 3, 2, 3, 2, 2, 2, 3, 2, 2, 3, 2, 1, 1, 2, 1,
	3, 2, 2, 3, 3, 2, 1, 0, 0, 0, 3, 2, 0, 3, 2, 3, 2, 1, 0, 1, 2, 2, 3, 0, 2, 0, 0, 1,
	3, 0, 3, 3, 3, 1, 0, 2, 3, 3, 0, 1, 0, 0, 1, 0, 3, 0, 0, 1, 3, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 3, 2, 0, 3, 0, 3, 2, 1, 3, 0, 3, 0, 0, 2, 0, 2, 1, 0, 2, 2, 3, 1, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	2, 1, 0, 2, 3, 1, 3, 3, 0, 3, 0, 3, 3, 0, 3, 3, 0, 3, 1, 2, 2, 3, 1, 1, 1, 0, 0,
	2, 1, 0, 2, 3, 3, 2, 3, 0, 0, 0, 1, 0, 2, 2, 3, 2, 0, 1, 0, 2, 1, 2, 3, 0, 2, 3, 0,
	3, 0, 1, 1, 2, 0, 3, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 3, 3, 3, 3, 1, 3, 3, 2, 3, 0, 1, 2, 0, 2, 3, 2, 2, 2, 3, 2, 3, 2, 0, 2, 2, 0,
	0, 0, 2, 1, 0, 3, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 0, 1, 0, 0, 1, 2, 1, 0,
	2, 0, 1, 2, 1, 0, 2, 2, 1, 2, 0, 2, 0, 0, 1, 1, 2, 1, 0, 2, 0, 2, 1, 3, 1, 0, 0,
	3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 2, 3, 2, 2, 2, 3, 2, 3, 3, 0, 3, 0, 2, 3, 1, 2, 2, 0, 3, 2, 3, 3, 0, 2, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
	1, 1, 1, 2, 3, 1, 3, 3, 0, 3, 0, 3, 3, 1, 2, 1, 0, 0, 3, 2, 2, 3, 2, 0, 1, 3, 1,
	1, 0, 0, 3, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 3, 3, 2, 1, 0, 1, 0, 3, 2, 1, 1, 2, 1, 0,
	3, 0, 3, 2, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 3, 1, 0, 3, 1, 3, 2, 0, 2, 0, 2, 0, 1, 2, 0, 0, 1, 0, 2, 2, 2, 0, 3, 1, 0, 0,
	2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 2, 0, 0, 0, 1,
	3, 0, 1, 1, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 1, 0, 0, 0, 3, 3, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 3, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 2, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 3, 3, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 3, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 3, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 2, 3, 0, 3, 0, 2, 0, 0, 1, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 3, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 3, 3, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 3, 3, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 3, 0, 0, 3, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 1, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 3, 3, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 0, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 1, 3, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 3, 0, 0, 2, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 3, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 3, 1, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 0, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 1, 1, 1, 0, 0, 0, 3, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 2, 3, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	3, 0, 0, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 2, 1, 2, 0, 3, 3, 0, 1, 0, 0, 0, 2, 0, 3, 1, 2, 2, 0, 1, 3, 0, 2, 0, 2, 0,
	2, 0, 2, 1, 1, 0, 1, 2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 2, 0, 0, 1, 1, 2, 0, 2,
};

static const SequenceModel Windows_1252DanishModel = { Windows_1252_Danish_CharToOrderMap, DanishLangModel, 30, 0.9968082796759031f, true, "WINDOWS-1252" };
static const SequenceModel Iso_8859_6ArabicModel = { Iso_8859_6_CharToOrderMap, ArabicLangModel, 64, 0.9696025116913417f, false, "ISO-8859-6" };
static const SequenceModel Windows_1256ArabicModel = { Windows_1256_CharToOrderMap, ArabicLangModel, 64, 0.9696025116913417f, false, "WINDOWS-1256" };
static const SequenceModel Latin5BulgarianModel = { Latin5_BulgarianCharToOrderMap, BulgarianLangModel, 64, 0.969392f, false, "ISO-8859-5" };
static const SequenceModel Win1251BulgarianModel = { win1251BulgarianCharToOrderMap, BulgarianLangModel, 64, 0.969392f, false, "WINDOWS-1251" };
static const SequenceModel Iso_8859_1GermanModel = { Iso_8859_1_German_CharToOrderMap, GermanLangModel, 31, 0.9934041448127945f, true, "ISO-8859-1" };
static const SequenceModel Windows_1253GreekModel = { Windows_1253_CharToOrderMap, GreekLangModel, 46, 0.958419074626211f, false, "WINDOWS-1253" };
static const SequenceModel Iso_8859_7GreekModel = { Iso_8859_7_CharToOrderMap, GreekLangModel, 46, 0.958419074626211f, false, "ISO-8859-7" };
static const SequenceModel Win1255Model = { win1255_CharToOrderMap, HebrewLangModel, 64, 0.984004f, false, "WINDOWS-1255" };
static const SequenceModel Iso_8859_2HungarianModel = { Iso_8859_2_CharToOrderMap, HungarianLangModel, 32, 0.9748272224933486f, false, "ISO-8859-2" };
static const SequenceModel Windows_1250HungarianModel = { Windows_1250_CharToOrderMap, HungarianLangModel, 32, 0.9748272224933486f, false, "WINDOWS-1250" };
static const SequenceModel Koi8rRussianModel       = { KOI8R_CharToOrderMap,       RussianLangModel, 64, 0.976601f, false, "KOI8-R" };
static const SequenceModel Win1251RussianModel     = { win1251_CharToOrderMap,     RussianLangModel, 64, 0.976601f, false, "WINDOWS-1251" };
static const SequenceModel Latin5RussianModel      = { latin5_CharToOrderMap,      RussianLangModel, 64, 0.976601f, false, "ISO-8859-5" };
static const SequenceModel MacCyrillicRussianModel = { macCyrillic_CharToOrderMap, RussianLangModel, 64, 0.976601f, false, "MAC-CYRILLIC" };
static const SequenceModel Ibm866RussianModel      = { IBM866_CharToOrderMap,      RussianLangModel, 64, 0.976601f, false, "IBM866" };
static const SequenceModel Ibm855RussianModel      = { IBM855_CharToOrderMap,      RussianLangModel, 64, 0.976601f, false, "IBM855" };
static const SequenceModel Iso_8859_15DanishModel = { Iso_8859_15_Danish_CharToOrderMap, DanishLangModel, 30, 0.9968082796759031f, true, "ISO-8859-15" };
static const SequenceModel Iso_8859_1DanishModel = { Iso_8859_1_Danish_CharToOrderMap, DanishLangModel, 30, 0.9968082796759031f, true, "ISO-8859-1" };
static const SequenceModel Iso_8859_1FrenchModel = { Iso_8859_1_French_CharToOrderMap, FrenchLangModel, 38, 0.997057879992383f, true, "ISO-8859-1" };
static const SequenceModel Iso_8859_15FrenchModel = { Iso_8859_15_French_CharToOrderMap, FrenchLangModel, 38, 0.997057879992383f, true, "ISO-8859-15" };
static const SequenceModel Iso_8859_1SpanishModel   = { Iso_8859_1_Spanish_CharToOrderMap, SpanishLangModel, 33, 0.9970385677528184f, true, "ISO-8859-1" };
static const SequenceModel Iso_8859_15SpanishModel  = { Iso_8859_15_Spanish_CharToOrderMap, SpanishLangModel, 33, 0.9970385677528184f, true, "ISO-8859-15" };
static const SequenceModel Windows_1252FrenchModel = { Windows_1252_French_CharToOrderMap, FrenchLangModel, 38, 0.997057879992383f, true, "WINDOWS-1252" };
static const SequenceModel Windows_1252GermanModel = { Windows_1252_German_CharToOrderMap, GermanLangModel, 31, 0.9934041448127945f, true, "WINDOWS-1252" };
static const SequenceModel Windows_1252SpanishModel = { Windows_1252_Spanish_CharToOrderMap, SpanishLangModel, 33, 0.9970385677528184f, true, "WINDOWS-1252" };
static const SequenceModel Tis_620ThaiModel = { Tis_620_CharToOrderMap, ThaiLangModel, 64, 0.8815720594354438f, false, "TIS-620" };
static const SequenceModel Iso_8859_11ThaiModel = { Iso_8859_11_CharToOrderMap, ThaiLangModel, 64, 0.8815720594354438f, false, "ISO-8859-11" };
static const SequenceModel Iso_8859_3EsperantoModel = { Iso_8859_3_Esperanto_CharToOrderMap, EsperantoLangModel, 35, 0.9942980632768038f, false, "ISO-8859-3" };
static const SequenceModel Iso_8859_3TurkishModel = { Iso_8859_3_Turkish_CharToOrderMap, TurkishLangModel, 36, 0.991865243864388f, false, "ISO-8859-3" };
static const SequenceModel Iso_8859_9TurkishModel = { Iso_8859_9_Turkish_CharToOrderMap, TurkishLangModel, 36, 0.991865243864388f, false, "ISO-8859-9" };
static const SequenceModel Windows_1258VietnameseModel = { Windows_1258_CharToOrderMap, VietnameseLangModel, 55, 0.9321889118082535f, false, "WINDOWS-1258" };
static const SequenceModel VisciiVietnameseModel = { Viscii_CharToOrderMap, VietnameseLangModel, 55, 0.9321889118082535f, false, "VISCII" };
//
// nsSBCSGroupProber.cpp
//
nsSBCSGroupProber::nsSBCSGroupProber()
{
	mProbers[0] = new nsSingleByteCharSetProber(&Win1251RussianModel);
	mProbers[1] = new nsSingleByteCharSetProber(&Koi8rRussianModel);
	mProbers[2] = new nsSingleByteCharSetProber(&Latin5RussianModel);
	mProbers[3] = new nsSingleByteCharSetProber(&MacCyrillicRussianModel);
	mProbers[4] = new nsSingleByteCharSetProber(&Ibm866RussianModel);
	mProbers[5] = new nsSingleByteCharSetProber(&Ibm855RussianModel);
	mProbers[6] = new nsSingleByteCharSetProber(&Iso_8859_7GreekModel);
	mProbers[7] = new nsSingleByteCharSetProber(&Windows_1253GreekModel);
	mProbers[8] = new nsSingleByteCharSetProber(&Latin5BulgarianModel);
	mProbers[9] = new nsSingleByteCharSetProber(&Win1251BulgarianModel);
	nsHebrewProber * hebprober = new nsHebrewProber();
	// Notice: Any change in these indexes - 10,11,12 must be reflected
	// in the code below as well.
	mProbers[10] = hebprober;
	mProbers[11] = new nsSingleByteCharSetProber(&Win1255Model, false, hebprober); // Logical Hebrew
	mProbers[12] = new nsSingleByteCharSetProber(&Win1255Model, true, hebprober); // Visual Hebrew
	// Tell the Hebrew prober about the logical and visual probers
	if(mProbers[10] && mProbers[11] && mProbers[12]) { // all are not null
		hebprober->SetModelProbers(mProbers[11], mProbers[12]);
	}
	else { // One or more is null. avoid any Hebrew probing, null them all
		for(uint32 i = 10; i <= 12; ++i) {
			delete mProbers[i];
			mProbers[i] = 0;
		}
	}
	mProbers[13] = new nsSingleByteCharSetProber(&Tis_620ThaiModel);
	mProbers[14] = new nsSingleByteCharSetProber(&Iso_8859_11ThaiModel);
	mProbers[15] = new nsSingleByteCharSetProber(&Iso_8859_1FrenchModel);
	mProbers[16] = new nsSingleByteCharSetProber(&Iso_8859_15FrenchModel);
	mProbers[17] = new nsSingleByteCharSetProber(&Windows_1252FrenchModel);
	mProbers[18] = new nsSingleByteCharSetProber(&Iso_8859_1SpanishModel);
	mProbers[19] = new nsSingleByteCharSetProber(&Iso_8859_15SpanishModel);
	mProbers[20] = new nsSingleByteCharSetProber(&Windows_1252SpanishModel);
	mProbers[21] = new nsSingleByteCharSetProber(&Iso_8859_2HungarianModel);
	mProbers[22] = new nsSingleByteCharSetProber(&Windows_1250HungarianModel);
	mProbers[23] = new nsSingleByteCharSetProber(&Iso_8859_1GermanModel);
	mProbers[24] = new nsSingleByteCharSetProber(&Windows_1252GermanModel);
	mProbers[25] = new nsSingleByteCharSetProber(&Iso_8859_3EsperantoModel);
	mProbers[26] = new nsSingleByteCharSetProber(&Iso_8859_3TurkishModel);
	mProbers[27] = new nsSingleByteCharSetProber(&Iso_8859_9TurkishModel);
	mProbers[28] = new nsSingleByteCharSetProber(&Iso_8859_6ArabicModel);
	mProbers[29] = new nsSingleByteCharSetProber(&Windows_1256ArabicModel);
	mProbers[30] = new nsSingleByteCharSetProber(&VisciiVietnameseModel);
	mProbers[31] = new nsSingleByteCharSetProber(&Windows_1258VietnameseModel);
	mProbers[32] = new nsSingleByteCharSetProber(&Iso_8859_15DanishModel);
	mProbers[33] = new nsSingleByteCharSetProber(&Iso_8859_1DanishModel);
	mProbers[34] = new nsSingleByteCharSetProber(&Windows_1252DanishModel);
	Reset();
}

nsSBCSGroupProber::~nsSBCSGroupProber()
{
	for(uint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++)
		delete mProbers[i];
}

const char * nsSBCSGroupProber::GetCharSetName()
{
	if(mBestGuess == -1) { // if we have no answer yet
		GetConfidence();
		if(mBestGuess == -1) // no charset seems positive
			mBestGuess = 0; // we will use default.
	}
	return mProbers[mBestGuess]->GetCharSetName();
}

void nsSBCSGroupProber::Reset()
{
	mActiveNum = 0;
	for(uint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++) {
		if(mProbers[i]) { // not null
			mProbers[i]->Reset();
			mIsActive[i] = true;
			++mActiveNum;
		}
		else
			mIsActive[i] = false;
	}
	mBestGuess = -1;
	mState = eDetecting;
}

nsProbingState nsSBCSGroupProber::HandleData(const char * aBuf, uint32 aLen)
{
	char * newBuf1 = 0;
	uint32 newLen1 = 0;
	//
	//apply filter to original buffer, and we got new buffer back
	//depend on what script it is, we will feed them the new buffer
	//we got after applying proper filter
	//this is done without any consideration to KeepEnglishLetters
	//of each prober since as of now, there are no probers here which
	//recognize languages with English characters.
	//
	if(FilterWithoutEnglishLetters(aBuf, aLen, &newBuf1, newLen1) && newLen1) { // Nothing to see here, move on.
		for(uint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++) {
			if(mIsActive[i]) {
				const nsProbingState st = mProbers[i]->HandleData(newBuf1, newLen1);
				if(st == eFoundIt) {
					mBestGuess = i;
					mState = eFoundIt;
					break;
				}
				else if(st == eNotMe) {
					mIsActive[i] = false;
					mActiveNum--;
					if(mActiveNum <= 0) {
						mState = eNotMe;
						break;
					}
				}
			}
		}
	}
	SAlloc::F(newBuf1);
	return mState;
}

float nsSBCSGroupProber::GetConfidence() const
{
	float bestConf = 0.0;
	switch(mState) {
		case eFoundIt: return 0.99f; //sure yes
		case eNotMe: return 0.01f; //sure no
		default:
			{
				for(uint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++) {
					if(mIsActive[i]) {
						const float cf = mProbers[i]->GetConfidence();
						if(bestConf < cf) {
							bestConf = cf;
							mBestGuess = i;
						}
					}
				}
			}
			break;
	}
	return bestConf;
}

#ifdef DEBUG_chardet
void nsSBCSGroupProber::DumpStatus()
{
	uint32 i;
	float cf = GetConfidence();
	printf(" SBCS Group Prober --------begin status \r\n");
	for(i = 0; i < NUM_OF_SBCS_PROBERS; i++) {
		if(!mIsActive[i])
			printf("  inactive: [%s] (i.e. confidence is too low).\r\n", mProbers[i]->GetCharSetName());
		else
			mProbers[i]->DumpStatus();
	}
	printf(" SBCS Group found best match [%s] confidence %f.\r\n", mProbers[mBestGuess]->GetCharSetName(), cf);
}
#endif
//
//
//
//
//#include <CharDistribution.h>
//
class CharDistributionAnalysis {
public:
	CharDistributionAnalysis() 
	{
		Reset(false);
	}
	//
	// feed a block of data and do distribution analysis
	//
	void HandleData(const char * aBuf, uint32 aLen) 
	{
	}
	//
	// Feed a character with known length
	//
	void HandleOneChar(const char * aStr, uint32 aCharLen)
	{
		//we only care about 2-bytes character in our distribution analysis
		const int32 order = (aCharLen == 2) ? GetOrder(aStr) : -1;
		if(order >= 0) {
			mTotalChars++;
			//order is valid
			if((uint32)order < mTableSize)
				if(mCharToFreqOrder[order] < 512)
					mFreqChars++;
		}
	}
	//
	// return confidence base on existing data
	//
	float  GetConfidence() const;
	//
	// Reset analyser, clear any state
	//
	void   Reset(bool aIsPreferredLanguage)
	{
		mDone = false;
		mTotalChars = 0;
		mFreqChars = 0;
		mDataThreshold = aIsPreferredLanguage ? 0 : MINIMUM_DATA_THRESHOLD;
	}
	//
	// It is not necessary to receive all data to draw conclusion. For charset detection,
	// certain amount of data is enough
	//
	bool GotEnoughData() const 
	{
		return (mTotalChars > ENOUGH_DATA_THRESHOLD);
	}
protected:
	//
	// we do not handle character base on its original encoding string, but
	// convert this encoding string to a number, here called order.
	// This allow multiple encoding of a language to share one frequency table
	//
	virtual int32 GetOrder(const char * str) const
	{
		return -1;
	}
	uint32 mFreqChars; //The number of characters whose frequency order is less than 512
	uint32 mTotalChars; //Total character encounted.
	uint32 mDataThreshold; //Number of hi-byte characters needed to trigger detection
	const  int16  * mCharToFreqOrder; //Mapping table to get frequency order from char order (get from GetOrder())
	uint32 mTableSize; //Size of above table
	// This is a constant value varies from language to language, it is used in
	// calculating confidence. See my paper for further detail.
	float  mTypicalDistributionRatio;
	bool   mDone;      // If this flag is set to true, detection is done and conclusion has been made
	uint8  Reserve[3]; // @alignment
};

class EUCTWDistributionAnalysis : public CharDistributionAnalysis {
public:
	EUCTWDistributionAnalysis()
	{
		mCharToFreqOrder = EUCTWCharToFreqOrder;
		mTableSize = EUCTW_TABLE_SIZE;
		mTypicalDistributionRatio = EUCTW_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//for EUC-TW encoding, we are interested
	//  first  byte range: 0xc4 -- 0xfe
	//  second byte range: 0xa1 -- 0xfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char * str) const
	{
		return ((uchar)*str >= (uchar)0xc4) ? (94*((uchar)str[0]-(uchar)0xc4) + (uchar)str[1] - (uchar)0xa1) : -1;
	}
};

class EUCKRDistributionAnalysis : public CharDistributionAnalysis {
public:
	EUCKRDistributionAnalysis()
	{
		mCharToFreqOrder = EUCKRCharToFreqOrder;
		mTableSize = EUCKR_TABLE_SIZE;
		mTypicalDistributionRatio = EUCKR_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//
	// for euc-KR encoding, we are interested
	//   first  byte range: 0xb0 -- 0xfe
	//   second byte range: 0xa1 -- 0xfe
	// no validation needed here. State machine has done that
	//
	int32 GetOrder(const char * str) const
	{
		return ((uchar)*str >= (uchar)0xb0) ? (94*((uchar)str[0]-(uchar)0xb0) + (uchar)str[1] - (uchar)0xa1) : -1;
	}
};

class GB2312DistributionAnalysis : public CharDistributionAnalysis {
public:
	GB2312DistributionAnalysis()
	{
		mCharToFreqOrder = GB2312CharToFreqOrder;
		mTableSize = GB2312_TABLE_SIZE;
		mTypicalDistributionRatio = GB2312_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//for GB2312 encoding, we are interested
	//  first  byte range: 0xb0 -- 0xfe
	//  second byte range: 0xa1 -- 0xfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char * str) const
	{
		return ((uchar)*str >= (uchar)0xb0 && (uchar)str[1] >= (uchar)0xa1) ? (94*((uchar)str[0]-(uchar)0xb0) + (uchar)str[1] - (uchar)0xa1) : -1;
	}
};

class Big5DistributionAnalysis : public CharDistributionAnalysis {
public:
	Big5DistributionAnalysis()
	{
		mCharToFreqOrder = Big5CharToFreqOrder;
		mTableSize = BIG5_TABLE_SIZE;
		mTypicalDistributionRatio = BIG5_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//for big5 encoding, we are interested
	//  first  byte range: 0xa4 -- 0xfe
	//  second byte range: 0x40 -- 0x7e , 0xa1 -- 0xfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char * str) const
	{
		if((uchar)*str >= (uchar)0xa4)
			if((uchar)str[1] >= (uchar)0xa1)
				return 157*((uchar)str[0]-(uchar)0xa4) + (uchar)str[1] - (uchar)0xa1 +63;
			else
				return 157*((uchar)str[0]-(uchar)0xa4) + (uchar)str[1] - (uchar)0x40;
		else
			return -1;
	}
};

class SJISDistributionAnalysis : public CharDistributionAnalysis {
public:
	SJISDistributionAnalysis()
	{
		mCharToFreqOrder = JISCharToFreqOrder;
		mTableSize = JIS_TABLE_SIZE;
		mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//for sjis encoding, we are interested
	//  first  byte range: 0x81 -- 0x9f , 0xe0 -- 0xfe
	//  second byte range: 0x40 -- 0x7e,  0x81 -- oxfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char * str) const
	{
		int32 order;
		if((uchar)*str >= (uchar)0x81 && (uchar)*str <= (uchar)0x9f)
			order = 188 * ((uchar)str[0]-(uchar)0x81);
		else if((uchar)*str >= (uchar)0xe0 && (uchar)*str <= (uchar)0xef)
			order = 188 * ((uchar)str[0]-(uchar)0xe0 + 31);
		else
			return -1;
		order += (uchar)*(str+1) - 0x40;
		if((uchar)str[1] > (uchar)0x7f)
			order--;
		return order;
	}
};

class EUCJPDistributionAnalysis : public CharDistributionAnalysis {
public:
	EUCJPDistributionAnalysis()
	{
		mCharToFreqOrder = JISCharToFreqOrder;
		mTableSize = JIS_TABLE_SIZE;
		mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
	}
protected:
	//for euc-JP encoding, we are interested
	//  first  byte range: 0xa0 -- 0xfe
	//  second byte range: 0xa1 -- 0xfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char * str) const
	{
		return ((uchar)*str >= (uchar)0xa0) ? (94*((uchar)str[0]-(uchar)0xa1) + (uchar)str[1] - (uchar)0xa1) : -1;
	}
};

//return confidence base on received data
float CharDistributionAnalysis::GetConfidence() const
{
	// if we didn't receive any character in our consideration range, or the
	// number of frequent characters is below the minimum threshold, return negative answer
	if(mTotalChars <= 0 || mFreqChars <= mDataThreshold)
		return SURE_NO;
	else {
		if(mTotalChars != mFreqChars) {
			float r = mFreqChars / ((mTotalChars - mFreqChars) * mTypicalDistributionRatio);
			if(r < SURE_YES)
				return r;
		}
		return SURE_YES; // normalize confidence, (we don't want to be 100% sure)
	}
}
//
//
//
//
// This is hiragana 2-char sequence table, the number in each cell represents its frequency category
//
static const uint8 jp2CharContext[83][83] =
{
	{ 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, },
	{ 2, 4, 0, 4, 0, 3, 0, 4, 0, 3, 4, 4, 4, 2, 4, 3, 3, 4, 3, 2, 3, 3, 4, 2, 3, 3, 3, 2, 4, 1, 4, 3, 3, 1, 5, 4, 3, 4, 3, 4, 3, 5, 3,
	  0, 3, 5, 4, 2, 0, 3, 1, 0, 3, 3, 0, 3, 3, 0, 1, 1, 0, 4, 3, 0, 3, 3, 0, 4, 0, 2, 0, 3, 5, 5, 5, 5, 4, 0, 4, 1, 0, 3, 4, },
	{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, },
	{ 0, 4, 0, 5, 0, 5, 0, 4, 0, 4, 5, 4, 4, 3, 5, 3, 5, 1, 5, 3, 4, 3, 4, 4, 3, 4, 3, 3, 4, 3, 5, 4, 4, 3, 5, 5, 3, 5, 5, 5, 3, 5, 5,
	  3, 4, 5, 5, 3, 1, 3, 2, 0, 3, 4, 0, 4, 2, 0, 4, 2, 1, 5, 3, 2, 3, 5, 0, 4, 0, 2, 0, 5, 4, 4, 5, 4, 5, 0, 4, 0, 0, 4, 4, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 0, 3, 0, 4, 0, 3, 0, 3, 0, 4, 5, 4, 3, 3, 3, 3, 4, 3, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 4, 4, 4, 4, 5, 3, 4, 4, 3, 4, 5, 5, 4, 5, 5,
	  1, 4, 5, 4, 3, 0, 3, 3, 1, 3, 3, 0, 4, 4, 0, 3, 3, 1, 5, 3, 3, 3, 5, 0, 4, 0, 3, 0, 4, 4, 3, 4, 3, 3, 0, 4, 1, 1, 3, 4, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 0, 4, 0, 3, 0, 3, 0, 4, 0, 3, 4, 4, 3, 2, 2, 1, 2, 1, 3, 1, 3, 3, 3, 3, 3, 4, 3, 1, 3, 3, 5, 3, 3, 0, 4, 3, 0, 5, 4, 3, 3, 5, 4,
	  4, 3, 4, 4, 5, 0, 1, 2, 0, 1, 2, 0, 2, 2, 0, 1, 0, 0, 5, 2, 2, 1, 4, 0, 3, 0, 1, 0, 4, 4, 3, 5, 4, 3, 0, 2, 1, 0, 4, 3, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 0, 3, 0, 5, 0, 4, 0, 2, 1, 4, 4, 2, 4, 1, 4, 2, 4, 2, 4, 3, 3, 3, 4, 3, 3, 3, 3, 1, 4, 2, 3, 3, 3, 1, 4, 4, 1, 1, 1, 4, 3, 3, 2,
	  0, 2, 4, 3, 2, 0, 3, 3, 0, 3, 1, 1, 0, 0, 0, 3, 3, 0, 4, 2, 2, 3, 4, 0, 4, 0, 3, 0, 4, 4, 5, 3, 4, 4, 0, 3, 0, 0, 1, 4, },
	{ 1, 4, 0, 4, 0, 4, 0, 4, 0, 3, 5, 4, 4, 3, 4, 3, 5, 4, 3, 3, 4, 3, 5, 4, 4, 4, 4, 3, 4, 2, 4, 3, 3, 1, 5, 4, 3, 2, 4, 5, 4, 5, 5,
	  4, 4, 5, 4, 4, 0, 3, 2, 2, 3, 3, 0, 4, 3, 1, 3, 2, 1, 4, 3, 3, 4, 5, 0, 3, 0, 2, 0, 4, 5, 5, 4, 5, 4, 0, 4, 0, 0, 5, 4, },
	{ 0, 5, 0, 5, 0, 4, 0, 3, 0, 4, 4, 3, 4, 3, 3, 3, 4, 0, 4, 4, 4, 3, 4, 3, 4, 3, 3, 1, 4, 2, 4, 3, 4, 0, 5, 4, 1, 4, 5, 4, 4, 5, 3,
	  2, 4, 3, 4, 3, 2, 4, 1, 3, 3, 3, 2, 3, 2, 0, 4, 3, 3, 4, 3, 3, 3, 4, 0, 4, 0, 3, 0, 4, 5, 4, 4, 4, 3, 0, 4, 1, 0, 1, 3, },
	{ 0, 3, 1, 4, 0, 3, 0, 2, 0, 3, 4, 4, 3, 1, 4, 2, 3, 3, 4, 3, 4, 3, 4, 3, 4, 4, 3, 2, 3, 1, 5, 4, 4, 1, 4, 4, 3, 5, 4, 4, 3, 5, 5,
	  4, 3, 4, 4, 3, 1, 2, 3, 1, 2, 2, 0, 3, 2, 0, 3, 1, 0, 5, 3, 3, 3, 4, 3, 3, 3, 3, 4, 4, 4, 4, 5, 4, 2, 0, 3, 3, 2, 4, 3, },
	{ 0, 2, 0, 3, 0, 1, 0, 1, 0, 0, 3, 2, 0, 0, 2, 0, 1, 0, 2, 1, 3, 3, 3, 1, 2, 3, 1, 0, 1, 0, 4, 2, 1, 1, 3, 3, 0, 4, 3, 3, 1, 4, 3,
	  3, 0, 3, 3, 2, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, 0, 0, 0, 4, 1, 0, 2, 3, 2, 2, 2, 1, 3, 3, 3, 4, 4, 3, 2, 0, 3, 1, 0, 3, 3, },
	{ 0, 4, 0, 4, 0, 3, 0, 3, 0, 4, 4, 4, 3, 3, 3, 3, 3, 3, 4, 3, 4, 2, 4, 3, 4, 3, 3, 2, 4, 3, 4, 5, 4, 1, 4, 5, 3, 5, 4, 5, 3, 5, 4,
	  0, 3, 5, 5, 3, 1, 3, 3, 2, 2, 3, 0, 3, 4, 1, 3, 3, 2, 4, 3, 3, 3, 4, 0, 4, 0, 3, 0, 4, 5, 4, 4, 5, 3, 0, 4, 1, 0, 3, 4, },
	{ 0, 2, 0, 3, 0, 3, 0, 0, 0, 2, 2, 2, 1, 0, 1, 0, 0, 0, 3, 0, 3, 0, 3, 0, 1, 3, 1, 0, 3, 1, 3, 3, 3, 1, 3, 3, 3, 0, 1, 3, 1, 3, 4,
	  0, 0, 3, 1, 1, 0, 3, 2, 0, 0, 0, 0, 1, 3, 0, 1, 0, 0, 3, 3, 2, 0, 3, 0, 0, 0, 0, 0, 3, 4, 3, 4, 3, 3, 0, 3, 0, 0, 2, 3, },
	{ 2, 3, 0, 3, 0, 2, 0, 1, 0, 3, 3, 4, 3, 1, 3, 1, 1, 1, 3, 1, 4, 3, 4, 3, 3, 3, 0, 0, 3, 1, 5, 4, 3, 1, 4, 3, 2, 5, 5, 4, 4, 4, 4,
	  3, 3, 4, 4, 4, 0, 2, 1, 1, 3, 2, 0, 1, 2, 0, 0, 1, 0, 4, 1, 3, 3, 3, 0, 3, 0, 1, 0, 4, 4, 4, 5, 5, 3, 0, 2, 0, 0, 4, 4, },
	{ 0, 2, 0, 1, 0, 3, 1, 3, 0, 2, 3, 3, 3, 0, 3, 1, 0, 0, 3, 0, 3, 2, 3, 1, 3, 2, 1, 1, 0, 0, 4, 2, 1, 0, 2, 3, 1, 4, 3, 2, 0, 4, 4,
	  3, 1, 3, 1, 3, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 4, 1, 1, 1, 2, 0, 3, 0, 0, 0, 3, 4, 2, 4, 3, 2, 0, 1, 0, 0, 3, 3, },
	{ 0, 1, 0, 4, 0, 5, 0, 4, 0, 2, 4, 4, 2, 3, 3, 2, 3, 3, 5, 3, 3, 3, 4, 3, 4, 2, 3, 0, 4, 3, 3, 3, 4, 1, 4, 3, 2, 1, 5, 5, 3, 4, 5,
	  1, 3, 5, 4, 2, 0, 3, 3, 0, 1, 3, 0, 4, 2, 0, 1, 3, 1, 4, 3, 3, 3, 3, 0, 3, 0, 1, 0, 3, 4, 4, 4, 5, 5, 0, 3, 0, 1, 4, 5, },
	{ 0, 2, 0, 3, 0, 3, 0, 0, 0, 2, 3, 1, 3, 0, 4, 0, 1, 1, 3, 0, 3, 4, 3, 2, 3, 1, 0, 3, 3, 2, 3, 1, 3, 0, 2, 3, 0, 2, 1, 4, 1, 2, 2,
	  0, 0, 3, 3, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0, 2, 2, 0, 3, 2, 1, 3, 3, 0, 2, 0, 2, 0, 0, 3, 3, 1, 2, 4, 0, 3, 0, 2, 2, 3, },
	{ 2, 4, 0, 5, 0, 4, 0, 4, 0, 2, 4, 4, 4, 3, 4, 3, 3, 3, 1, 2, 4, 3, 4, 3, 4, 4, 5, 0, 3, 3, 3, 3, 2, 0, 4, 3, 1, 4, 3, 4, 1, 4, 4,
	  3, 3, 4, 4, 3, 1, 2, 3, 0, 4, 2, 0, 4, 1, 0, 3, 3, 0, 4, 3, 3, 3, 4, 0, 4, 0, 2, 0, 3, 5, 3, 4, 5, 2, 0, 3, 0, 0, 4, 5, },
	{ 0, 3, 0, 4, 0, 1, 0, 1, 0, 1, 3, 2, 2, 1, 3, 0, 3, 0, 2, 0, 2, 0, 3, 0, 2, 0, 0, 0, 1, 0, 1, 1, 0, 0, 3, 1, 0, 0, 0, 4, 0, 3, 1,
	  0, 2, 1, 3, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 2, 2, 3, 1, 0, 3, 0, 0, 0, 1, 4, 4, 4, 3, 0, 0, 4, 0, 0, 1, 4, },
	{ 1, 4, 1, 5, 0, 3, 0, 3, 0, 4, 5, 4, 4, 3, 5, 3, 3, 4, 4, 3, 4, 1, 3, 3, 3, 3, 2, 1, 4, 1, 5, 4, 3, 1, 4, 4, 3, 5, 4, 4, 3, 5, 4,
	  3, 3, 4, 4, 4, 0, 3, 3, 1, 2, 3, 0, 3, 1, 0, 3, 3, 0, 5, 4, 4, 4, 4, 4, 4, 3, 3, 5, 4, 4, 3, 3, 5, 4, 0, 3, 2, 0, 4, 4, },
	{ 0, 2, 0, 3, 0, 1, 0, 0, 0, 1, 3, 3, 3, 2, 4, 1, 3, 0, 3, 1, 3, 0, 2, 2, 1, 1, 0, 0, 2, 0, 4, 3, 1, 0, 4, 3, 0, 4, 4, 4, 1, 4, 3,
	  1, 1, 3, 3, 1, 0, 2, 0, 0, 1, 3, 0, 0, 0, 0, 2, 0, 0, 4, 3, 2, 4, 3, 5, 4, 3, 3, 3, 4, 3, 3, 4, 3, 3, 0, 2, 1, 0, 3, 3, },
	{ 0, 2, 0, 4, 0, 3, 0, 2, 0, 2, 5, 5, 3, 4, 4, 4, 4, 1, 4, 3, 3, 0, 4, 3, 4, 3, 1, 3, 3, 2, 4, 3, 0, 3, 4, 3, 0, 3, 4, 4, 2, 4, 4,
	  0, 4, 5, 3, 3, 2, 2, 1, 1, 1, 2, 0, 1, 5, 0, 3, 3, 2, 4, 3, 3, 3, 4, 0, 3, 0, 2, 0, 4, 4, 3, 5, 5, 0, 0, 3, 0, 2, 3, 3, },
	{ 0, 3, 0, 4, 0, 3, 0, 1, 0, 3, 4, 3, 3, 1, 3, 3, 3, 0, 3, 1, 3, 0, 4, 3, 3, 1, 1, 0, 3, 0, 3, 3, 0, 0, 4, 4, 0, 1, 5, 4, 3, 3, 5,
	  0, 3, 3, 4, 3, 0, 2, 0, 1, 1, 1, 0, 1, 3, 0, 1, 2, 1, 3, 3, 2, 3, 3, 0, 3, 0, 1, 0, 1, 3, 3, 4, 4, 1, 0, 1, 2, 2, 1, 3, },
	{ 0, 1, 0, 4, 0, 4, 0, 3, 0, 1, 3, 3, 3, 2, 3, 1, 1, 0, 3, 0, 3, 3, 4, 3, 2, 4, 2, 0, 1, 0, 4, 3, 2, 0, 4, 3, 0, 5, 3, 3, 2, 4, 4,
	  4, 3, 3, 3, 4, 0, 1, 3, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 4, 2, 3, 3, 3, 0, 3, 0, 0, 0, 4, 4, 4, 5, 3, 2, 0, 3, 3, 0, 3, 5, },
	{ 0, 2, 0, 3, 0, 0, 0, 3, 0, 1, 3, 0, 2, 0, 0, 0, 1, 0, 3, 1, 1, 3, 3, 0, 0, 3, 0, 0, 3, 0, 2, 3, 1, 0, 3, 1, 0, 3, 3, 2, 0, 4, 2,
	  2, 0, 2, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 0, 1, 0, 1, 0, 0, 0, 1, 3, 1, 2, 0, 0, 0, 1, 0, 0, 1, 4, },
	{ 0, 3, 0, 3, 0, 5, 0, 1, 0, 2, 4, 3, 1, 3, 3, 2, 1, 1, 5, 2, 1, 0, 5, 1, 2, 0, 0, 0, 3, 3, 2, 2, 3, 2, 4, 3, 0, 0, 3, 3, 1, 3, 3,
	  0, 2, 5, 3, 4, 0, 3, 3, 0, 1, 2, 0, 2, 2, 0, 3, 2, 0, 2, 2, 3, 3, 3, 0, 2, 0, 1, 0, 3, 4, 4, 2, 5, 4, 0, 3, 0, 0, 3, 5, },
	{ 0, 3, 0, 3, 0, 3, 0, 1, 0, 3, 3, 3, 3, 0, 3, 0, 2, 0, 2, 1, 1, 0, 2, 0, 1, 0, 0, 0, 2, 1, 0, 0, 1, 0, 3, 2, 0, 0, 3, 3, 1, 2, 3,
	  1, 0, 3, 3, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 2, 3, 1, 2, 3, 0, 3, 0, 1, 0, 3, 2, 1, 0, 4, 3, 0, 1, 1, 0, 3, 3, },
	{ 0, 4, 0, 5, 0, 3, 0, 3, 0, 4, 5, 5, 4, 3, 5, 3, 4, 3, 5, 3, 3, 2, 5, 3, 4, 4, 4, 3, 4, 3, 4, 5, 5, 3, 4, 4, 3, 4, 4, 5, 4, 4, 4,
	  3, 4, 5, 5, 4, 2, 3, 4, 2, 3, 4, 0, 3, 3, 1, 4, 3, 2, 4, 3, 3, 5, 5, 0, 3, 0, 3, 0, 5, 5, 5, 5, 4, 4, 0, 4, 0, 1, 4, 4, },
	{ 0, 4, 0, 4, 0, 3, 0, 3, 0, 3, 5, 4, 4, 2, 3, 2, 5, 1, 3, 2, 5, 1, 4, 2, 3, 2, 3, 3, 4, 3, 3, 3, 3, 2, 5, 4, 1, 3, 3, 5, 3, 4, 4,
	  0, 4, 4, 3, 1, 1, 3, 1, 0, 2, 3, 0, 2, 3, 0, 3, 0, 0, 4, 3, 1, 3, 4, 0, 3, 0, 2, 0, 4, 4, 4, 3, 4, 5, 0, 4, 0, 0, 3, 4, },
	{ 0, 3, 0, 3, 0, 3, 1, 2, 0, 3, 4, 4, 3, 3, 3, 0, 2, 2, 4, 3, 3, 1, 3, 3, 3, 1, 1, 0, 3, 1, 4, 3, 2, 3, 4, 4, 2, 4, 4, 4, 3, 4, 4,
	  3, 2, 4, 4, 3, 1, 3, 3, 1, 3, 3, 0, 4, 1, 0, 2, 2, 1, 4, 3, 2, 3, 3, 5, 4, 3, 3, 5, 4, 4, 3, 3, 0, 4, 0, 3, 2, 2, 4, 4, },
	{ 0, 2, 0, 1, 0, 0, 0, 0, 0, 1, 2, 1, 3, 0, 0, 0, 0, 0, 2, 0, 1, 2, 1, 0, 0, 1, 0, 0, 0, 0, 3, 0, 0, 1, 0, 1, 1, 3, 1, 0, 0, 0, 1,
	  1, 0, 1, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 0, 3, 4, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, },
	{ 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 4, 0, 4, 1, 4, 0, 3, 0, 4, 0, 3, 0, 4, 0, 3, 0, 3, 0, 4, 1, 5, 1, 4, 0, 0, 3, 0, 5, 0, 5, 2, 0, 1,
	  0, 0, 0, 2, 1, 4, 0, 1, 3, 0, 0, 3, 0, 0, 3, 1, 1, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 1, 4, 0, 5, 0, 3, 0, 2, 0, 3, 5, 4, 4, 3, 4, 3, 5, 3, 4, 3, 3, 0, 4, 3, 3, 3, 3, 3, 3, 2, 4, 4, 3, 1, 3, 4, 4, 5, 4, 4, 3, 4, 4,
	  1, 3, 5, 4, 3, 3, 3, 1, 2, 2, 3, 3, 1, 3, 1, 3, 3, 3, 5, 3, 3, 4, 5, 0, 3, 0, 3, 0, 3, 4, 3, 4, 4, 3, 0, 3, 0, 2, 4, 3, },
	{ 0, 1, 0, 4, 0, 0, 0, 0, 0, 1, 4, 0, 4, 1, 4, 2, 4, 0, 3, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 3, 1, 1, 1, 0, 3, 0, 0, 0, 1, 2, 1, 0,
	  0, 1, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 3, 0, 0, 0, 0, 3, 2, 0, 2, 2, 0, 1, 0, 0, 0, 2, 3, 2, 3, 3, 0, 0, 0, 0, 2, 1, 0, },
	{ 0, 5, 1, 5, 0, 3, 0, 3, 0, 5, 4, 4, 5, 1, 5, 3, 3, 0, 4, 3, 4, 3, 5, 3, 4, 3, 3, 2, 4, 3, 4, 3, 3, 0, 3, 3, 1, 4, 4, 3, 4, 4, 4,
	  3, 4, 5, 5, 3, 2, 3, 1, 1, 3, 3, 1, 3, 1, 1, 3, 3, 2, 4, 5, 3, 3, 5, 0, 4, 0, 3, 0, 4, 4, 3, 5, 3, 3, 0, 3, 4, 0, 4, 3, },
	{ 0, 5, 0, 5, 0, 3, 0, 2, 0, 4, 4, 3, 5, 2, 4, 3, 3, 3, 4, 4, 4, 3, 5, 3, 5, 3, 3, 1, 4, 0, 4, 3, 3, 0, 3, 3, 0, 4, 4, 4, 4, 5, 4,
	  3, 3, 5, 5, 3, 2, 3, 1, 2, 3, 2, 0, 1, 0, 0, 3, 2, 2, 4, 4, 3, 1, 5, 0, 4, 0, 3, 0, 4, 3, 1, 3, 2, 1, 0, 3, 3, 0, 3, 3, },
	{ 0, 4, 0, 5, 0, 5, 0, 4, 0, 4, 5, 5, 5, 3, 4, 3, 3, 2, 5, 4, 4, 3, 5, 3, 5, 3, 4, 0, 4, 3, 4, 4, 3, 2, 4, 4, 3, 4, 5, 4, 4, 5, 5,
	  0, 3, 5, 5, 4, 1, 3, 3, 2, 3, 3, 1, 3, 1, 0, 4, 3, 1, 4, 4, 3, 4, 5, 0, 4, 0, 2, 0, 4, 3, 4, 4, 3, 3, 0, 4, 0, 0, 5, 5, },
	{ 0, 4, 0, 4, 0, 5, 0, 1, 1, 3, 3, 4, 4, 3, 4, 1, 3, 0, 5, 1, 3, 0, 3, 1, 3, 1, 1, 0, 3, 0, 3, 3, 4, 0, 4, 3, 0, 4, 4, 4, 3, 4, 4,
	  0, 3, 5, 4, 1, 0, 3, 0, 0, 2, 3, 0, 3, 1, 0, 3, 1, 0, 3, 2, 1, 3, 5, 0, 3, 0, 1, 0, 3, 2, 3, 3, 4, 4, 0, 2, 2, 0, 4, 4, },
	{ 2, 4, 0, 5, 0, 4, 0, 3, 0, 4, 5, 5, 4, 3, 5, 3, 5, 3, 5, 3, 5, 2, 5, 3, 4, 3, 3, 4, 3, 4, 5, 3, 2, 1, 5, 4, 3, 2, 3, 4, 5, 3, 4,
	  1, 2, 5, 4, 3, 0, 3, 3, 0, 3, 2, 0, 2, 3, 0, 4, 1, 0, 3, 4, 3, 3, 5, 0, 3, 0, 1, 0, 4, 5, 5, 5, 4, 3, 0, 4, 2, 0, 3, 5, },
	{ 0, 5, 0, 4, 0, 4, 0, 2, 0, 5, 4, 3, 4, 3, 4, 3, 3, 3, 4, 3, 4, 2, 5, 3, 5, 3, 4, 1, 4, 3, 4, 4, 4, 0, 3, 5, 0, 4, 4, 4, 4, 5, 3,
	  1, 3, 4, 5, 3, 3, 3, 3, 3, 3, 3, 0, 2, 2, 0, 3, 3, 2, 4, 3, 3, 3, 5, 3, 4, 1, 3, 3, 5, 3, 2, 0, 0, 0, 0, 4, 3, 1, 3, 3, },
	{ 0, 1, 0, 3, 0, 3, 0, 1, 0, 1, 3, 3, 3, 2, 3, 3, 3, 0, 3, 0, 0, 0, 3, 1, 3, 0, 0, 0, 2, 2, 2, 3, 0, 0, 3, 2, 0, 1, 2, 4, 1, 3, 3,
	  0, 0, 3, 3, 3, 0, 1, 0, 0, 2, 1, 0, 0, 3, 0, 3, 1, 0, 3, 0, 0, 1, 3, 0, 2, 0, 1, 0, 3, 3, 1, 3, 3, 0, 0, 1, 1, 0, 3, 3, },
	{ 0, 2, 0, 3, 0, 2, 1, 4, 0, 2, 2, 3, 1, 1, 3, 1, 1, 0, 2, 0, 3, 1, 2, 3, 1, 3, 0, 0, 1, 0, 4, 3, 2, 3, 3, 3, 1, 4, 2, 3, 3, 3, 3,
	  1, 0, 3, 1, 4, 0, 1, 1, 0, 1, 2, 0, 1, 1, 0, 1, 1, 0, 3, 1, 3, 2, 2, 0, 1, 0, 0, 0, 2, 3, 3, 3, 1, 0, 0, 0, 0, 0, 2, 3, },
	{ 0, 5, 0, 4, 0, 5, 0, 2, 0, 4, 5, 5, 3, 3, 4, 3, 3, 1, 5, 4, 4, 2, 4, 4, 4, 3, 4, 2, 4, 3, 5, 5, 4, 3, 3, 4, 3, 3, 5, 5, 4, 5, 5,
	  1, 3, 4, 5, 3, 1, 4, 3, 1, 3, 3, 0, 3, 3, 1, 4, 3, 1, 4, 5, 3, 3, 5, 0, 4, 0, 3, 0, 5, 3, 3, 1, 4, 3, 0, 4, 0, 1, 5, 3, },
	{ 0, 5, 0, 5, 0, 4, 0, 2, 0, 4, 4, 3, 4, 3, 3, 3, 3, 3, 5, 4, 4, 4, 4, 4, 4, 5, 3, 3, 5, 2, 4, 4, 4, 3, 4, 4, 3, 3, 4, 4, 5, 5, 3,
	  3, 4, 3, 4, 3, 3, 4, 3, 3, 3, 3, 1, 2, 2, 1, 4, 3, 3, 5, 4, 4, 3, 4, 0, 4, 0, 3, 0, 4, 4, 4, 4, 4, 1, 0, 4, 2, 0, 2, 4, },
	{ 0, 4, 0, 4, 0, 3, 0, 1, 0, 3, 5, 2, 3, 0, 3, 0, 2, 1, 4, 2, 3, 3, 4, 1, 4, 3, 3, 2, 4, 1, 3, 3, 3, 0, 3, 3, 0, 0, 3, 3, 3, 5, 3,
	  3, 3, 3, 3, 2, 0, 2, 0, 0, 2, 0, 0, 2, 0, 0, 1, 0, 0, 3, 1, 2, 2, 3, 0, 3, 0, 2, 0, 4, 4, 3, 3, 4, 1, 0, 3, 0, 0, 2, 4, },
	{ 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2, 0, 0, 0, 0, 0, 1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 3, 1, 3, 0, 3, 2, 0, 0, 0, 1, 0, 3, 2,
	  0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 0, 2, 0, 0, 0, 0, 0, 0, 2, },
	{ 0, 2, 1, 3, 0, 2, 0, 2, 0, 3, 3, 3, 3, 1, 3, 1, 3, 3, 3, 3, 3, 3, 4, 2, 2, 1, 2, 1, 4, 0, 4, 3, 1, 3, 3, 3, 2, 4, 3, 5, 4, 3, 3,
	  3, 3, 3, 3, 3, 0, 1, 3, 0, 2, 0, 0, 1, 0, 0, 1, 0, 0, 4, 2, 0, 2, 3, 0, 3, 3, 0, 3, 3, 4, 2, 3, 1, 4, 0, 1, 2, 0, 2, 3, },
	{ 0, 3, 0, 3, 0, 1, 0, 3, 0, 2, 3, 3, 3, 0, 3, 1, 2, 0, 3, 3, 2, 3, 3, 2, 3, 2, 3, 1, 3, 0, 4, 3, 2, 0, 3, 3, 1, 4, 3, 3, 2, 3, 4,
	  3, 1, 3, 3, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 4, 1, 1, 0, 3, 0, 3, 1, 0, 2, 3, 3, 3, 3, 3, 1, 0, 0, 2, 0, 3, 3, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 3, 0, 3, 0, 3, 1, 0, 1, 0, 1, 0, 0, 1,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, },
	{ 0, 2, 0, 3, 1, 3, 0, 3, 0, 2, 3, 3, 3, 1, 3, 1, 3, 1, 3, 1, 3, 3, 3, 1, 3, 0, 2, 3, 1, 1, 4, 3, 3, 2, 3, 3, 1, 2, 2, 4, 1, 3, 3,
	  0, 1, 4, 2, 3, 0, 1, 3, 0, 3, 0, 0, 1, 3, 0, 2, 0, 0, 3, 3, 2, 1, 3, 0, 3, 0, 2, 0, 3, 4, 4, 4, 3, 1, 0, 3, 0, 0, 3, 3, },
	{ 0, 2, 0, 1, 0, 2, 0, 0, 0, 1, 3, 2, 2, 1, 3, 0, 1, 1, 3, 0, 3, 2, 3, 1, 2, 0, 2, 0, 1, 1, 3, 3, 3, 0, 3, 3, 1, 1, 2, 3, 2, 3, 3,
	  1, 2, 3, 2, 0, 0, 1, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 2, 1, 2, 1, 3, 0, 3, 0, 0, 0, 3, 4, 4, 4, 3, 2, 0, 2, 0, 0, 2, 4, },
	{ 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 1, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 0, 0, 3, },
	{ 0, 3, 0, 3, 0, 2, 0, 3, 0, 3, 3, 3, 2, 3, 2, 2, 2, 0, 3, 1, 3, 3, 3, 2, 3, 3, 0, 0, 3, 0, 3, 2, 2, 0, 2, 3, 1, 4, 3, 4, 3, 3, 2,
	  3, 1, 5, 4, 4, 0, 3, 1, 2, 1, 3, 0, 3, 1, 1, 2, 0, 2, 3, 1, 3, 1, 3, 0, 3, 0, 1, 0, 3, 3, 4, 4, 2, 1, 0, 2, 1, 0, 2, 4, },
	{ 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 4, 2, 5, 1, 4, 0, 2, 0, 2, 1, 3, 1, 4, 0, 2, 1, 0, 0, 2, 1, 4, 1, 1, 0, 3, 3, 0, 5, 1, 3, 2, 3, 3,
	  1, 0, 3, 2, 3, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 4, 0, 1, 0, 3, 0, 2, 0, 1, 0, 3, 3, 3, 4, 3, 3, 0, 0, 0, 0, 2, 3, },
	{ 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 1, 0, 0, 0, 0, 0, 3, },
	{ 0, 1, 0, 3, 0, 4, 0, 3, 0, 2, 4, 3, 1, 0, 3, 2, 2, 1, 3, 1, 2, 2, 3, 1, 1, 1, 2, 1, 3, 0, 1, 2, 0, 1, 3, 2, 1, 3, 0, 5, 5, 1, 0,
	  0, 1, 3, 2, 1, 0, 3, 0, 0, 1, 0, 0, 0, 0, 0, 3, 4, 0, 1, 1, 1, 3, 2, 0, 2, 0, 1, 0, 2, 3, 3, 1, 2, 3, 0, 1, 0, 1, 0, 4, },
	{ 0, 0, 0, 1, 0, 3, 0, 3, 0, 2, 2, 1, 0, 0, 4, 0, 3, 0, 3, 1, 3, 0, 3, 0, 3, 0, 1, 0, 3, 0, 3, 1, 3, 0, 3, 3, 0, 0, 1, 2, 1, 1, 1,
	  0, 1, 2, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 2, 0, 0, 2, 0, 0, 0, 0, 2, 3, 3, 3, 3, 0, 0, 0, 0, 1, 4, },
	{ 0, 0, 0, 3, 0, 3, 0, 0, 0, 0, 3, 1, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 2, 0, 2, 3, 0, 0, 2, 2, 3, 1, 2,
	  0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 2, 0, 0, 0, 0, 2, 3, },
	{ 2, 4, 0, 5, 0, 5, 0, 4, 0, 3, 4, 3, 3, 3, 4, 3, 3, 3, 4, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 0, 5, 5, 4, 1, 5, 4, 3, 1, 5, 4, 3, 4, 4,
	  3, 3, 4, 3, 3, 0, 3, 2, 0, 2, 3, 0, 3, 0, 0, 3, 3, 0, 5, 3, 2, 3, 3, 0, 3, 0, 3, 0, 3, 4, 5, 4, 5, 3, 0, 4, 3, 0, 3, 4, },
	{ 0, 3, 0, 3, 0, 3, 0, 3, 0, 3, 3, 4, 3, 2, 3, 2, 3, 0, 4, 3, 3, 3, 3, 3, 3, 3, 3, 0, 3, 2, 4, 3, 3, 1, 3, 4, 3, 4, 4, 4, 3, 4, 4,
	  3, 2, 4, 4, 1, 0, 2, 0, 0, 1, 1, 0, 2, 0, 0, 3, 1, 0, 5, 3, 2, 1, 3, 0, 3, 0, 1, 2, 4, 3, 2, 4, 3, 3, 0, 3, 2, 0, 4, 4, },
	{ 0, 3, 0, 3, 0, 1, 0, 0, 0, 1, 4, 3, 3, 2, 3, 1, 3, 1, 4, 2, 3, 2, 4, 2, 3, 4, 3, 0, 2, 2, 3, 3, 3, 0, 3, 3, 3, 0, 3, 4, 1, 3, 3,
	  0, 3, 4, 3, 3, 0, 1, 1, 0, 1, 0, 0, 0, 4, 0, 3, 0, 0, 3, 1, 2, 1, 3, 0, 4, 0, 1, 0, 4, 3, 3, 4, 3, 3, 0, 2, 0, 0, 3, 3, },
	{ 0, 3, 0, 4, 0, 1, 0, 3, 0, 3, 4, 3, 3, 0, 3, 3, 3, 1, 3, 1, 3, 3, 4, 3, 3, 3, 0, 0, 3, 1, 5, 3, 3, 1, 3, 3, 2, 5, 4, 3, 3, 4, 5,
	  3, 2, 5, 3, 4, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 0, 4, 2, 2, 1, 3, 0, 3, 0, 2, 0, 4, 4, 3, 5, 3, 2, 0, 1, 1, 0, 3, 4, },
	{ 0, 5, 0, 4, 0, 5, 0, 2, 0, 4, 4, 3, 3, 2, 3, 3, 3, 1, 4, 3, 4, 1, 5, 3, 4, 3, 4, 0, 4, 2, 4, 3, 4, 1, 5, 4, 0, 4, 4, 4, 4, 5, 4,
	  1, 3, 5, 4, 2, 1, 4, 1, 1, 3, 2, 0, 3, 1, 0, 3, 2, 1, 4, 3, 3, 3, 4, 0, 4, 0, 3, 0, 4, 4, 4, 3, 3, 3, 0, 4, 2, 0, 3, 4, },
	{ 1, 4, 0, 4, 0, 3, 0, 1, 0, 3, 3, 3, 1, 1, 3, 3, 2, 2, 3, 3, 1, 0, 3, 2, 2, 1, 2, 0, 3, 1, 2, 1, 2, 0, 3, 2, 0, 2, 2, 3, 3, 4, 3,
	  0, 3, 3, 1, 2, 0, 1, 1, 3, 1, 2, 0, 0, 3, 0, 1, 1, 0, 3, 2, 2, 3, 3, 0, 3, 0, 0, 0, 2, 3, 3, 4, 3, 3, 0, 1, 0, 0, 1, 4, },
	{ 0, 4, 0, 4, 0, 4, 0, 0, 0, 3, 4, 4, 3, 1, 4, 2, 3, 2, 3, 3, 3, 1, 4, 3, 4, 0, 3, 0, 4, 2, 3, 3, 2, 2, 5, 4, 2, 1, 3, 4, 3, 4, 3,
	  1, 3, 3, 4, 2, 0, 2, 1, 0, 3, 3, 0, 0, 2, 0, 3, 1, 0, 4, 4, 3, 4, 3, 0, 4, 0, 1, 0, 2, 4, 4, 4, 4, 4, 0, 3, 2, 0, 3, 3, },
	{ 0, 0, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 2, 0, 0, 1, 0, 0, 0, 1,
	  0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, },
	{ 0, 2, 0, 3, 0, 4, 0, 4, 0, 1, 3, 3, 3, 0, 4, 0, 2, 1, 2, 1, 1, 1, 2, 0, 3, 1, 1, 0, 1, 0, 3, 1, 0, 0, 3, 3, 2, 0, 1, 1, 0, 0, 0,
	  0, 0, 1, 0, 2, 0, 2, 2, 0, 3, 1, 0, 0, 1, 0, 1, 1, 0, 1, 2, 0, 3, 0, 0, 0, 0, 1, 0, 0, 3, 3, 4, 3, 1, 0, 1, 0, 3, 0, 2, },
	{ 0, 0, 0, 3, 0, 5, 0, 0, 0, 0, 1, 0, 2, 0, 3, 1, 0, 1, 3, 0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 4, 0, 0, 0, 2, 3, 0, 1, 4,
	  1, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 3, 0, 0, 0, 0, 0, 3, },
	{ 0, 2, 0, 5, 0, 5, 0, 1, 0, 2, 4, 3, 3, 2, 5, 1, 3, 2, 3, 3, 3, 0, 4, 1, 2, 0, 3, 0, 4, 0, 2, 2, 1, 1, 5, 3, 0, 0, 1, 4, 2, 3, 2,
	  0, 3, 3, 3, 2, 0, 2, 4, 1, 1, 2, 0, 1, 1, 0, 3, 1, 0, 1, 3, 1, 2, 3, 0, 2, 0, 0, 0, 1, 3, 5, 4, 4, 4, 0, 3, 0, 0, 1, 3, },
	{ 0, 4, 0, 5, 0, 4, 0, 4, 0, 4, 5, 4, 3, 3, 4, 3, 3, 3, 4, 3, 4, 4, 5, 3, 4, 5, 4, 2, 4, 2, 3, 4, 3, 1, 4, 4, 1, 3, 5, 4, 4, 5, 5,
	  4, 4, 5, 5, 5, 2, 3, 3, 1, 4, 3, 1, 3, 3, 0, 3, 3, 1, 4, 3, 4, 4, 4, 0, 3, 0, 4, 0, 3, 3, 4, 4, 5, 0, 0, 4, 3, 0, 4, 5, },
	{ 0, 4, 0, 4, 0, 3, 0, 3, 0, 3, 4, 4, 4, 3, 3, 2, 4, 3, 4, 3, 4, 3, 5, 3, 4, 3, 2, 1, 4, 2, 4, 4, 3, 1, 3, 4, 2, 4, 5, 5, 3, 4, 5,
	  4, 1, 5, 4, 3, 0, 3, 2, 2, 3, 2, 1, 3, 1, 0, 3, 3, 3, 5, 3, 3, 3, 5, 4, 4, 2, 3, 3, 4, 3, 3, 3, 2, 1, 0, 3, 2, 1, 4, 3, },
	{ 0, 4, 0, 5, 0, 4, 0, 3, 0, 3, 5, 5, 3, 2, 4, 3, 4, 0, 5, 4, 4, 1, 4, 4, 4, 3, 3, 3, 4, 3, 5, 5, 2, 3, 3, 4, 1, 2, 5, 5, 3, 5, 5,
	  2, 3, 5, 5, 4, 0, 3, 2, 0, 3, 3, 1, 1, 5, 1, 4, 1, 0, 4, 3, 2, 3, 5, 0, 4, 0, 3, 0, 5, 4, 3, 4, 3, 0, 0, 4, 1, 0, 4, 4, },
	{ 1, 3, 0, 4, 0, 2, 0, 2, 0, 2, 5, 5, 3, 3, 3, 3, 3, 0, 4, 2, 3, 4, 4, 4, 3, 4, 0, 0, 3, 4, 5, 4, 3, 3, 3, 3, 2, 5, 5, 4, 5, 5, 5,
	  4, 3, 5, 5, 5, 1, 3, 1, 0, 1, 0, 0, 3, 2, 0, 4, 2, 0, 5, 2, 3, 2, 4, 1, 3, 0, 3, 0, 4, 5, 4, 5, 4, 3, 0, 4, 2, 0, 5, 4, },
	{ 0, 3, 0, 4, 0, 5, 0, 3, 0, 3, 4, 4, 3, 2, 3, 2, 3, 3, 3, 3, 3, 2, 4, 3, 3, 2, 2, 0, 3, 3, 3, 3, 3, 1, 3, 3, 3, 0, 4, 4, 3, 4, 4,
	  1, 1, 4, 4, 2, 0, 3, 1, 0, 1, 1, 0, 4, 1, 0, 2, 3, 1, 3, 3, 1, 3, 4, 0, 3, 0, 1, 0, 3, 1, 3, 0, 0, 1, 0, 2, 0, 0, 4, 4, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
	{ 0, 3, 0, 3, 0, 2, 0, 3, 0, 1, 5, 4, 3, 3, 3, 1, 4, 2, 1, 2, 3, 4, 4, 2, 4, 4, 5, 0, 3, 1, 4, 3, 4, 0, 4, 3, 3, 3, 2, 3, 2, 5, 3,
	  4, 3, 2, 2, 3, 0, 0, 3, 0, 2, 1, 0, 1, 2, 0, 0, 0, 0, 2, 1, 1, 3, 1, 0, 2, 0, 4, 0, 3, 4, 4, 4, 5, 2, 0, 2, 0, 0, 1, 3, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 4, 2, 1, 1, 0, 1, 0, 3, 2, 0, 0, 3, 1,
	  1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1, 4, 0, 4, 2, 1, 0, 0, 0, 0, 0, 1, },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 3, 1, 0, 0, 0, 2, 0, 2, 1, 0, 0, 1, 2,
	  1, 0, 1, 1, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 1, 0, 0, 0, 0, 0, 1, 0, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, },
	{ 0, 4, 0, 4, 0, 4, 0, 3, 0, 4, 4, 3, 4, 2, 4, 3, 2, 0, 4, 4, 4, 3, 5, 3, 5, 3, 3, 2, 4, 2, 4, 3, 4, 3, 1, 4, 0, 2, 3, 4, 4, 4, 3,
	  3, 3, 4, 4, 4, 3, 4, 1, 3, 4, 3, 2, 1, 2, 1, 3, 3, 3, 4, 4, 3, 3, 5, 0, 4, 0, 3, 0, 4, 3, 3, 3, 2, 1, 0, 3, 0, 0, 3, 3, },
	{ 0, 4, 0, 3, 0, 3, 0, 3, 0, 3, 5, 5, 3, 3, 3, 3, 4, 3, 4, 3, 3, 3, 4, 4, 4, 3, 3, 3, 3, 4, 3, 5, 3, 3, 1, 3, 2, 4, 5, 5, 5, 5, 4,
	  3, 4, 5, 5, 3, 2, 2, 3, 3, 3, 3, 2, 3, 3, 1, 2, 3, 2, 4, 3, 3, 3, 4, 0, 4, 0, 2, 0, 4, 3, 2, 2, 1, 2, 0, 3, 0, 0, 4, 1, },
};

void nsCharSetProber::JapaneseContextAnalysis::HandleData(const char * aBuf, uint32 aLen)
{
	if(!mDone) {
		// The buffer we got is byte oriented, and a character may span in more than one
		// buffers. In case the last one or two byte in last buffer is not complete, we
		// record how many byte needed to complete that character and skip these bytes here.
		// We can choose to record those bytes as well and analyse the character once it
		// is complete, but since a character will not make much difference, by simply skipping
		// this character will simply our logic and improve performance.
		for(uint32 i = mNeedToSkipCharNum; i < aLen;) {
			uint32 charLen;
			const int32 order = GetOrder(aBuf+i, &charLen);
			i += charLen;
			if(i > aLen) {
				mNeedToSkipCharNum = i - aLen;
				mLastCharOrder = -1;
			}
			else {
				if(order != -1 && mLastCharOrder != -1) {
					mTotalRel++;
					if(mTotalRel > MAX_REL_THRESHOLD) {
						mDone = true;
						break;
					}
					mRelSample[jp2CharContext[mLastCharOrder][order]]++;
				}
				mLastCharOrder = order;
			}
		}
	}
}

void nsCharSetProber::JapaneseContextAnalysis::HandleOneChar(const char * aStr, uint32 aCharLen)
{
	//if we received enough data, stop here
	if(mTotalRel > MAX_REL_THRESHOLD) 
		mDone = true;
	if(!mDone) {
		// Only 2-bytes characters are of our interest
		int32 order = (aCharLen == 2) ? GetOrder(aStr) : -1;
		if(order != -1 && mLastCharOrder != -1) {
			mTotalRel++;
			//count this sequence to its category counter
			mRelSample[jp2CharContext[mLastCharOrder][order]]++;
		}
		mLastCharOrder = order;
	}
}

void nsCharSetProber::JapaneseContextAnalysis::Reset(bool aIsPreferredLanguage)
{
	mTotalRel = 0;
	memzero(mRelSample, sizeof(mRelSample));
	mNeedToSkipCharNum = 0;
	mLastCharOrder = -1;
	mDone = false;
	mDataThreshold = aIsPreferredLanguage ? 0 : MINIMUM_DATA_THRESHOLD;
}

float nsCharSetProber::JapaneseContextAnalysis::GetConfidence() const
{
	//This is just one way to calculate confidence. It works well for me.
	return (mTotalRel > mDataThreshold) ? (((float)(mTotalRel - mRelSample[0]))/mTotalRel) : DONT_KNOW;
}
//
// This filter applies to all scripts which do not use English characters
//
bool nsCharSetProber::FilterWithoutEnglishLetters(const char * aBuf, uint32 aLen, char ** newBuf, uint32& newLen)
{
	char * newptr = static_cast<char *>(SAlloc::M(aLen));
	*newBuf = newptr;
	if(!newptr)
		return false;
	else {
		bool meetMSB = false;
		char * prevPtr, * curPtr;
		for(curPtr = prevPtr = (char *)aBuf; curPtr < aBuf+aLen; curPtr++) {
			if(*curPtr & 0x80) {
				meetMSB = true;
			}
			else if(*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') {
				//current char is a symbol, most likely a punctuation. we treat it as segment delimiter
				if(meetMSB && curPtr > prevPtr) {
					//this segment contains more than single symbol, and it has upper ASCII, we need to keep
					// it
					while(prevPtr < curPtr) 
						*newptr++ = *prevPtr++;
					prevPtr++;
					*newptr++ = ' ';
					meetMSB = false;
				}
				else //ignore current segment. (either because it is just a symbol or just an English word)
					prevPtr = curPtr+1;
			}
		}
		if(meetMSB && curPtr > prevPtr)
			while(prevPtr < curPtr) 
				*newptr++ = *prevPtr++;
		newLen = static_cast<uint32>(newptr - *newBuf);
		return true;
	}
}

//This filter applies to all scripts which contain both English characters and upper ASCII characters.
bool nsCharSetProber::FilterWithEnglishLetters(const char * aBuf, uint32 aLen, char ** newBuf, uint32& newLen)
{
	//do filtering to reduce load to probers
	char * prevPtr, * curPtr;
	bool isInTag = false;
	char * newptr = static_cast<char *>(SAlloc::M(aLen));
	*newBuf = newptr;
	if(!newptr)
		return false;
	for(curPtr = prevPtr = (char *)aBuf; curPtr < aBuf+aLen; curPtr++) {
		if(*curPtr == '>')
			isInTag = false;
		else if(*curPtr == '<')
			isInTag = true;
		if(!(*curPtr & 0x80) && (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z')) {
			if(curPtr > prevPtr && !isInTag) { // Current segment contains more than just a symbol and it is not inside a tag, keep it.
				while(prevPtr < curPtr) 
					*newptr++ = *prevPtr++;
				prevPtr++;
				*newptr++ = ' ';
			}
			else
				prevPtr = curPtr+1;
		}
	}
	// If the current segment contains more than just a symbol
	// and it is not inside a tag then keep it.
	if(!isInTag)
		while(prevPtr < curPtr)
			*newptr++ = *prevPtr++;
	newLen = static_cast<uint32>(newptr - *newBuf);
	return true;
}
//
//#include <nsBig5Prober.h>
//
class nsBig5Prober : public nsCharSetProber {
public:
	nsBig5Prober(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&Big5SMModel);
		Reset();
	}
	virtual ~nsBig5Prober()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "BIG5"; }
	void      Reset();
	float     GetConfidence() const;
protected:
	//void      GetDistribution(uint32 aCharLen, const char * aStr);

	nsCodingStateMachine * mCodingSM;
	//Big5ContextAnalysis mContextAnalyser;
	Big5DistributionAnalysis mDistributionAnalyser;
	char   mLastChar[2];
	bool   mIsPreferredLanguage;
	uint8  Reserve; // @alignment
};

void nsBig5Prober::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsBig5Prober::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			const uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mDistributionAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
	return mState;
}

float nsBig5Prober::GetConfidence() const
{
	return mDistributionAnalyser.GetConfidence();
}
//
// for japanese encoding, obeserve characteristic:
// 1, kana character (or hankaku?) often have hight frequency of appereance
// 2, kana character often exist in group
// 3, certain combination of kana is never used in japanese language
//
//
//#include <nsEUCJPProber.h>
//
class nsEUCJPProber : public nsCharSetProber {
public:
	nsEUCJPProber(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&EUCJPSMModel);
		Reset();
	}
	virtual ~nsEUCJPProber()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-JP"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	class EUCJPContextAnalysis : public JapaneseContextAnalysis {
	protected:
		int32 GetOrder(const char * str, uint32 * charLen) const;
		int32 GetOrder(const char * str) const
		{
			// We only interested in Hiragana, so first byte is '\244'
			return (*str == '\244' && (uchar)*(str+1) >= (uchar)0xa1 && (uchar)*(str+1) <= (uchar)0xf3) ? ((uchar)*(str+1) - (uchar)0xa1) : -1;
		}
	};
	nsCodingStateMachine * mCodingSM;
	EUCJPContextAnalysis mContextAnalyser;
	EUCJPDistributionAnalysis mDistributionAnalyser;
	char   mLastChar[2];
	bool   mIsPreferredLanguage;
	uint8  Reserve; // @alignment
};

int32 nsEUCJPProber::EUCJPContextAnalysis::GetOrder(const char * str, uint32 * charLen) const
{
	//find out current char's byte length
	if((uchar)*str == (uchar)0x8e || ((uchar)*str >= (uchar)0xa1 && (uchar)*str <= (uchar)0xfe))
		*charLen = 2;
	else if((uchar)*str == (uchar)0x8f)
		*charLen = 3;
	else
		*charLen = 1;
	//return its order if it is hiragana
	if((uchar)*str == (uchar)0xa4 && (uchar)*(str+1) >= (uchar)0xa1 && (uchar)*(str+1) <= (uchar)0xf3)
		return (uchar)*(str+1) - (uchar)0xa1;
	return -1;
}

void nsEUCJPProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mContextAnalyser.Reset(mIsPreferredLanguage);
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsEUCJPProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			const uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mContextAnalyser.HandleOneChar(mLastChar, charLen);
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else {
				mContextAnalyser.HandleOneChar(aBuf+i-1, charLen);
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
			}
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mContextAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
	return mState;
}

float nsEUCJPProber::GetConfidence() const
{
	return smax(mContextAnalyser.GetConfidence(), mDistributionAnalyser.GetConfidence());
}
//
//
//
class nsEUCKRProber : public nsCharSetProber {
public:
	nsEUCKRProber(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&EUCKRSMModel);
		Reset();
	}
	virtual ~nsEUCKRProber()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-KR"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	//void   GetDistribution(uint32 aCharLen, const char * aStr);

	nsCodingStateMachine* mCodingSM;
	//EUCKRContextAnalysis mContextAnalyser;
	EUCKRDistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};

void nsEUCKRProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
	//mContextAnalyser.Reset();
}

nsProbingState nsEUCKRProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			const uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mDistributionAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
//    else
//      mDistributionAnalyser.HandleData(aBuf, aLen);

	return mState;
}

float nsEUCKRProber::GetConfidence() const
{
	return mDistributionAnalyser.GetConfidence();
}
//
//
//
class nsEUCTWProber : public nsCharSetProber {
public:
	nsEUCTWProber(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&EUCTWSMModel);
		Reset();
	}
	virtual ~nsEUCTWProber()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-TW"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	//void   GetDistribution(uint32 aCharLen, const char * aStr);

	nsCodingStateMachine* mCodingSM;
	//EUCTWContextAnalysis mContextAnalyser;
	EUCTWDistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};

void nsEUCTWProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
	//mContextAnalyser.Reset();
}

nsProbingState nsEUCTWProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			const uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mDistributionAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
//    else
//      mDistributionAnalyser.HandleData(aBuf, aLen);
	return mState;
}

float nsEUCTWProber::GetConfidence() const
{
	return mDistributionAnalyser.GetConfidence();
}
//
// We use GB18030 to replace GB2312, because 18030 is a superset.
//
class nsGB18030Prober : public nsCharSetProber {
public:
	nsGB18030Prober(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&GB18030SMModel);
		Reset();
	}
	virtual ~nsGB18030Prober()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "GB18030"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	//void      GetDistribution(uint32 aCharLen, const char * aStr);

	nsCodingStateMachine* mCodingSM;
	//GB2312ContextAnalysis mContextAnalyser;
	GB2312DistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};
//
// for S-JIS encoding, obeserve characteristic:
// 1, kana character (or hankaku?) often have hight frequency of appereance
// 2, kana character often exist in group
// 3, certain combination of kana is never used in japanese language
//
void nsGB18030Prober::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
	//mContextAnalyser.Reset();
}

nsProbingState nsGB18030Prober::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			const uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mDistributionAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
//    else
//      mDistributionAnalyser.HandleData(aBuf, aLen);

	return mState;
}

float nsGB18030Prober::GetConfidence() const
{
	return mDistributionAnalyser.GetConfidence();
}
//
//
//
// windows-1255 / ISO-8859-8 code points of interest
#define FINAL_KAF ('\xea')
#define NORMAL_KAF ('\xeb')
#define FINAL_MEM ('\xed')
#define NORMAL_MEM ('\xee')
#define FINAL_NUN ('\xef')
#define NORMAL_NUN ('\xf0')
#define FINAL_PE ('\xf3')
#define NORMAL_PE ('\xf4')
#define FINAL_TSADI ('\xf5')
#define NORMAL_TSADI ('\xf6')

// Minimum Visual vs Logical final letter score difference.
// If the difference is below this, don't rely solely on the final letter score distance.
#define MIN_FINAL_CHAR_DISTANCE (5)
// Minimum Visual vs Logical model score difference.
// If the difference is below this, don't rely at all on the model score distance.
#define MIN_MODEL_DISTANCE (0.01)
#define VISUAL_HEBREW_NAME ("ISO-8859-8")
#define LOGICAL_HEBREW_NAME ("WINDOWS-1255")

bool FASTCALL nsHebrewProber::isFinal(char c)
{
	return oneof5(c, FINAL_KAF, FINAL_MEM, FINAL_NUN, FINAL_PE, FINAL_TSADI);
}

bool FASTCALL nsHebrewProber::isNonFinal(char c)
{
	return oneof4(c, NORMAL_KAF, NORMAL_MEM, NORMAL_NUN, NORMAL_PE);
	// The normal Tsadi is not a good Non-Final letter due to words like
	// 'lechotet' (to chat) containing an apostrophe after the tsadi. This
	// apostrophe is converted to a space in FilterWithoutEnglishLetters causing
	// the Non-Final tsadi to appear at an end of a word even though this is not
	// the case in the original text.
	// The letters Pe and Kaf rarely display a related behavior of not being a
	// good Non-Final letter. Words like 'Pop', 'Winamp' and 'Mubarak' for
	// example legally end with a Non-Final Pe or Kaf. However, the benefit of
	// these letters as Non-Final letters outweighs the damage since these words
	// are quite rare.
}

/** HandleData
 * Final letter analysis for logical-visual decision.
 * Look for evidence that the received buffer is either logical Hebrew or
 * visual Hebrew.
 * The following cases are checked:
 * 1) A word longer than 1 letter, ending with a final letter. This is an
 *  indication that the text is laid out "naturally" since the final letter
 *  really appears at the end. +1 for logical score.
 * 2) A word longer than 1 letter, ending with a Non-Final letter. In normal
 *  Hebrew, words ending with Kaf, Mem, Nun, Pe or Tsadi, should not end with
 *  the Non-Final form of that letter. Exceptions to this rule are mentioned
 *  above in isNonFinal(). This is an indication that the text is laid out
 *  backwards. +1 for visual score
 * 3) A word longer than 1 letter, starting with a final letter. Final letters
 *  should not appear at the beginning of a word. This is an indication that
 *  the text is laid out backwards. +1 for visual score.
 *
 * The visual score and logical score are accumulated throughout the text and
 * are finally checked against each other in GetCharSetName().
 * No checking for final letters in the middle of words is done since that case
 * is not an indication for either Logical or Visual text.
 *
 * The input buffer should not contain any white spaces that are not (' ')
 * or any low-ascii punctuation marks.
 */
nsProbingState nsHebrewProber::HandleData(const char * aBuf, uint32 aLen)
{
	// Both model probers say it's not them. No reason to continue.
	if(GetState() == eNotMe)
		return eNotMe;
	else {
		const char * endPtr = aBuf+aLen;
		for(const char * curPtr = (char *)aBuf; curPtr < endPtr; ++curPtr) {
			const char cur = *curPtr;
			if(cur == ' ') { // We stand on a space - a word just ended
				if(mBeforePrev != ' ') { // *(curPtr-2) was not a space so prev is not a 1 letter word
					if(isFinal(mPrev)) // case (1) [-2:not space][-1:final letter][cur:space]
						++mFinalCharLogicalScore;
					else if(isNonFinal(mPrev)) // case (2) [-2:not space][-1:Non-Final letter][cur:space]
						++mFinalCharVisualScore;
				}
			}
			else { // Not standing on a space
				if((mBeforePrev == ' ') && (isFinal(mPrev)) && (cur != ' ')) // case (3) [-2:space][-1:final letter][cur:not space]
					++mFinalCharVisualScore;
			}
			mBeforePrev = mPrev;
			mPrev = cur;
		}
		// Forever detecting, till the end or until both model probers return eNotMe (handled above).
		return eDetecting;
	}
}

// Make the decision: is it Logical or Visual?
const char * nsHebrewProber::GetCharSetName()
{
	// If the final letter score distance is dominant enough, rely on it.
	int32 finalsub = mFinalCharLogicalScore - mFinalCharVisualScore;
	if(finalsub >= MIN_FINAL_CHAR_DISTANCE)
		return LOGICAL_HEBREW_NAME;
	else if(finalsub <= -(MIN_FINAL_CHAR_DISTANCE))
		return VISUAL_HEBREW_NAME;
	else {
		// It's not dominant enough, try to rely on the model scores instead.
		float modelsub = mLogicalProb->GetConfidence() - mVisualProb->GetConfidence();
		if(modelsub > MIN_MODEL_DISTANCE)
			return LOGICAL_HEBREW_NAME;
		else if(modelsub < -(MIN_MODEL_DISTANCE))
			return VISUAL_HEBREW_NAME;
		else if(finalsub < 0) // Still no good, back to final letter distance, maybe it'll save the day.
			return VISUAL_HEBREW_NAME;
		else // (finalsub > 0 - Logical) or (don't know what to do) default to Logical.
			return LOGICAL_HEBREW_NAME;
	}
}

void nsHebrewProber::Reset()
{
	mFinalCharLogicalScore = 0;
	mFinalCharVisualScore = 0;
	// mPrev and mBeforePrev are initialized to space in order to simulate a word
	// delimiter at the beginning of the data
	mPrev = ' ';
	mBeforePrev = ' ';
}

nsProbingState nsHebrewProber::GetState() const
{
	// Remain active as long as any of the model probers are active.
	return ((mLogicalProb->GetState() == eNotMe) && (mVisualProb->GetState() == eNotMe)) ? eNotMe : eDetecting;
}

#ifdef DEBUG_chardet
void nsHebrewProber::DumpStatus()
{
	printf("  HEB: %d - %d [Logical-Visual score]\r\n", mFinalCharLogicalScore, mFinalCharVisualScore);
}
#endif
//
//
//
#define UDF    0        // undefined
#define OTH    1        //other
#define ASC    2        // ascii capital letter
#define ASS    3        // ascii small letter
#define ACV    4        // accent capital vowel
#define ACO    5        // accent capital other
#define ASV    6        // accent small vowel
#define ASO    7        // accent small other
#define CLASS_NUM   8    // total classes

static const uchar Latin1_CharToClass[] =
{
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 00 - 07
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 08 - 0F
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 10 - 17
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 18 - 1F
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 20 - 27
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 28 - 2F
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 30 - 37
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 38 - 3F
	OTH, ASC, ASC, ASC, ASC, ASC, ASC, ASC, // 40 - 47
	ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC, // 48 - 4F
	ASC, ASC, ASC, ASC, ASC, ASC, ASC, ASC, // 50 - 57
	ASC, ASC, ASC, OTH, OTH, OTH, OTH, OTH, // 58 - 5F
	OTH, ASS, ASS, ASS, ASS, ASS, ASS, ASS, // 60 - 67
	ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS, // 68 - 6F
	ASS, ASS, ASS, ASS, ASS, ASS, ASS, ASS, // 70 - 77
	ASS, ASS, ASS, OTH, OTH, OTH, OTH, OTH, // 78 - 7F
	OTH, UDF, OTH, ASO, OTH, OTH, OTH, OTH, // 80 - 87
	OTH, OTH, ACO, OTH, ACO, UDF, ACO, UDF, // 88 - 8F
	UDF, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // 90 - 97
	OTH, OTH, ASO, OTH, ASO, UDF, ASO, ACO, // 98 - 9F
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // A0 - A7
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // A8 - AF
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // B0 - B7
	OTH, OTH, OTH, OTH, OTH, OTH, OTH, OTH, // B8 - BF
	ACV, ACV, ACV, ACV, ACV, ACV, ACO, ACO, // C0 - C7
	ACV, ACV, ACV, ACV, ACV, ACV, ACV, ACV, // C8 - CF
	ACO, ACO, ACV, ACV, ACV, ACV, ACV, OTH, // D0 - D7
	ACV, ACV, ACV, ACV, ACV, ACO, ACO, ACO, // D8 - DF
	ASV, ASV, ASV, ASV, ASV, ASV, ASO, ASO, // E0 - E7
	ASV, ASV, ASV, ASV, ASV, ASV, ASV, ASV, // E8 - EF
	ASO, ASO, ASV, ASV, ASV, ASV, ASV, OTH, // F0 - F7
	ASV, ASV, ASV, ASV, ASV, ASO, ASO, ASO, // F8 - FF
};

/* 0 : illegal
   1 : very unlikely
   2 : normal
   3 : very likely
 */
static const uchar Latin1ClassModel[] =
{
/* UDF OTH ASC ASS ACV ACO ASV ASO  */
/*UDF*/ 0,  0,  0,  0,  0,  0,  0,  0,
/*OTH*/ 0,  3,  3,  3,  3,  3,  3,  3,
/*ASC*/ 0,  3,  3,  3,  3,  3,  3,  3,
/*ASS*/ 0,  3,  3,  3,  1,  1,  3,  3,
/*ACV*/ 0,  3,  3,  3,  1,  2,  1,  2,
/*ACO*/ 0,  3,  3,  3,  3,  3,  3,  3,
/*ASV*/ 0,  3,  1,  3,  1,  1,  1,  3,
/*ASO*/ 0,  3,  1,  3,  1,  1,  3,  3,
};

void nsLatin1Prober::Reset()
{
	mState = eDetecting;
	mLastCharClass = OTH;
	memzero(mFreqCounter, sizeof(mFreqCounter));
}

nsProbingState nsLatin1Prober::HandleData(const char * aBuf, uint32 aLen)
{
	char * newBuf1 = 0;
	uint32 newLen1 = 0;
	if(!FilterWithEnglishLetters(aBuf, aLen, &newBuf1, newLen1)) {
		newBuf1 = (char *)aBuf;
		newLen1 = aLen;
	}
	for(uint32 i = 0; i < newLen1; i++) {
		const uchar charClass = Latin1_CharToClass[(uchar)newBuf1[i]];
		const uchar freq = Latin1ClassModel[mLastCharClass*CLASS_NUM + charClass];
		if(freq == 0) {
			mState = eNotMe;
			break;
		}
		mFreqCounter[freq]++;
		mLastCharClass = charClass;
	}
	if(newBuf1 != aBuf)
		SAlloc::F(newBuf1);
	return mState;
}

float nsLatin1Prober::GetConfidence() const
{
	float confidence = 0.0f;
	if(mState == eNotMe)
		confidence = 0.01f;
	else {
		uint32 total = 0;
		for(int32 i = 0; i < SIZEOFARRAY(mFreqCounter); i++)
			total += mFreqCounter[i];
		if(!total)
			confidence = 0.0f;
		else {
			confidence = mFreqCounter[3]*1.0f / total;
			confidence -= mFreqCounter[1]*20.0f / total;
		}
		SETMAX(confidence, 0.0f);
		// lower the confidence of latin1 so that other more accurate detector
		// can take priority.
		confidence *= 0.50f;
	}
	return confidence;
}

#ifdef DEBUG_chardet
void nsLatin1Prober::DumpStatus()
{
	printf(" Latin1Prober: %1.3f [%s]\r\n", GetConfidence(), GetCharSetName());
}
#endif
//
//
//
class nsSJISProber : public nsCharSetProber {
public:
	nsSJISProber(bool aIsPreferredLanguage) : mIsPreferredLanguage(aIsPreferredLanguage)
	{
		mCodingSM = new nsCodingStateMachine(&SJISSMModel);
		Reset();
	}
	virtual ~nsSJISProber()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "SHIFT_JIS"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	class SJISContextAnalysis : public JapaneseContextAnalysis {
		//SJISContextAnalysis() {};
	protected:
		int32 GetOrder(const char * str, uint32 * charLen) const;
		int32 GetOrder(const char * str) const
		{
			//We only interested in Hiragana, so first byte is '\202'
			return (*str == '\202' && (uchar)*(str+1) >= (uchar)0x9f && (uchar)*(str+1) <= (uchar)0xf1) ? ((uchar)*(str+1) - (uchar)0x9f) : -1;
		}
	};
	nsCodingStateMachine * mCodingSM;
	SJISContextAnalysis mContextAnalyser;
	SJISDistributionAnalysis mDistributionAnalyser;
	char   mLastChar[2];
	bool   mIsPreferredLanguage;
	uint8  Reserve; // @alignment
};

int32 nsSJISProber::SJISContextAnalysis::GetOrder(const char * str, uint32 * charLen) const
{
	const uchar c0 = (uchar)str[0];
	const uchar c1 = (uchar)str[1];
	// find out current char's byte length
	*charLen = ((c0 >= (uchar)0x81 && c0 <= (uchar)0x9f) || (c0 >= (uchar)0xe0 && c0 <= (uchar)0xfc)) ? 2 : 1;
	// return its order if it is hiragana
	return (c0 == '\202' && c1 >= (uchar)0x9f && c1 <= (uchar)0xf1) ? (c1 - (uchar)0x9f) : -1;
}
//
// for S-JIS encoding, obeserve characteristic:
// 1, kana character (or hankaku?) often have hight frequency of appereance
// 2, kana character often exist in group
// 3, certain combination of kana is never used in japanese language
//
void nsSJISProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mContextAnalyser.Reset(mIsPreferredLanguage);
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsSJISProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(!i) {
				mLastChar[1] = aBuf[0];
				mContextAnalyser.HandleOneChar(mLastChar+2-charLen, charLen);
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else {
				mContextAnalyser.HandleOneChar(aBuf+i+1-charLen, charLen);
				mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
			}
		}
	}
	mLastChar[0] = aBuf[aLen-1];
	if(mState == eDetecting)
		if(mContextAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
	return mState;
}

float nsSJISProber::GetConfidence() const
{
	return smax(mContextAnalyser.GetConfidence(), mDistributionAnalyser.GetConfidence());
}
//
//
//
nsSingleByteCharSetProber::nsSingleByteCharSetProber(const SequenceModel * model) : mModel(model), mReversed(false), mNameProber(0) 
{
	Reset();
}

nsSingleByteCharSetProber::nsSingleByteCharSetProber(const SequenceModel * model, bool reversed, nsCharSetProber* nameProber) : 
	mModel(model), mReversed(reversed), mNameProber(nameProber) 
{
	Reset();
}

nsProbingState nsSingleByteCharSetProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const uchar order = mModel->charToOrderMap[(uchar)aBuf[i]];
		if(order < SYMBOL_CAT_ORDER)
			mTotalChar++;
		else if(order == ILL) {
			// When encountering an illegal codepoint, no need to continue analyzing data. 
			mState = eNotMe;
			break;
		}
		else if(order == CTR) {
			mCtrlChar++;
		}
		if(order < mModel->freqCharCount) {
			mFreqChar++;
			if(mLastOrder < mModel->freqCharCount) {
				mTotalSeqs++;
				if(!mReversed)
					++(mSeqCounters[mModel->precedenceMatrix[mLastOrder*mModel->freqCharCount+order]]);
				else // reverse the order of the letters in the lookup
					++(mSeqCounters[mModel->precedenceMatrix[order*mModel->freqCharCount+mLastOrder]]);
			}
		}
		mLastOrder = order;
	}
	if(mState == eDetecting)
		if(mTotalSeqs > SB_ENOUGH_REL_THRESHOLD) {
			const float cf = GetConfidence();
			if(cf > POSITIVE_SHORTCUT_THRESHOLD)
				mState = eFoundIt;
			else if(cf < NEGATIVE_SHORTCUT_THRESHOLD)
				mState = eNotMe;
		}
	return mState;
}

void nsSingleByteCharSetProber::Reset()
{
	mState = eDetecting;
	mLastOrder = 255;
	memzero(mSeqCounters, sizeof(mSeqCounters));
	mTotalSeqs = 0;
	mTotalChar = 0;
	mCtrlChar  = 0;
	mFreqChar = 0;
}

//#define NEGATIVE_APPROACH 1

float nsSingleByteCharSetProber::GetConfidence() const
{
#ifdef NEGATIVE_APPROACH
	if(mTotalSeqs > 0)
		if(mTotalSeqs > mSeqCounters[NEGATIVE_CAT]*10)
			return ((float)(mTotalSeqs - mSeqCounters[NEGATIVE_CAT]*10))/mTotalSeqs * mFreqChar / mTotalChar;
	return 0.01f;
#else  //POSITIVE_APPROACH
	float r;

	if(mTotalSeqs > 0) {
		r = ((float)1.0) * mSeqCounters[POSITIVE_CAT] / mTotalSeqs / mModel->mTypicalPositiveRatio;
		/* Multiply by a ratio of positive sequences per characters.
		 * This would help in particular to distinguish close winners.
		 * Indeed if you add a letter, you'd expect the positive sequence count
		 * to increase as well. If it doesn't, it may mean that this new codepoint
		 * may not have been a letter, but instead a symbol (or some other
		 * character). This could make the difference between very closely related
		 * charsets used for the same language.
		 */
		r = r * (mSeqCounters[POSITIVE_CAT] + (float)mSeqCounters[PROBABLE_CAT] / 4) / mTotalChar;
		/* The more control characters (proportionnaly to the size of the text), the
		 * less confident we become in the current charset.
		 */
		r = r * (mTotalChar - mCtrlChar) / mTotalChar;
		r = r*mFreqChar/mTotalChar;
		if(r >= 1.0f)
			r = 0.99f;
		return r;
	}
	return 0.01f;
#endif
}

const char * nsSingleByteCharSetProber::GetCharSetName()
{
	return mNameProber ? mNameProber->GetCharSetName() : mModel->charsetName;
}

#ifdef DEBUG_chardet
void nsSingleByteCharSetProber::DumpStatus()
{
	printf("  SBCS: %1.3f [%s]\r\n", GetConfidence(), GetCharSetName());
}
#endif
//
//#include <nsUTF8Prober.h>
//
class nsUTF8Prober : public nsCharSetProber {
public:
	nsUTF8Prober() : mNumOfMBChar(0), mCodingSM(new nsCodingStateMachine(&UTF8SMModel))
	{
		Reset();
	}
	virtual ~nsUTF8Prober()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char * aBuf, uint32 aLen);
	const char * GetCharSetName() { return "UTF-8"; }
	void   Reset();
	float  GetConfidence() const;
protected:
	nsCodingStateMachine* mCodingSM;
	uint32 mNumOfMBChar;
};

void nsUTF8Prober::Reset()
{
	mCodingSM->Reset();
	mNumOfMBChar = 0;
	mState = eDetecting;
}

nsProbingState nsUTF8Prober::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		else if(codingState == eStart) {
			if(mCodingSM->GetCurrentCharLen() >= 2)
				mNumOfMBChar++;
		}
	}
	if(mState == eDetecting)
		if(GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
	return mState;
}

float nsUTF8Prober::GetConfidence() const
{
	float unlike = 0.99f;
	if(mNumOfMBChar < 6) {
		for(uint32 i = 0; i < mNumOfMBChar; i++)
			unlike *= ONE_CHAR_PROB;
		return 1.0f - unlike;
	}
	else
		return 0.99f;
}
//
//
//
#if defined(DEBUG_chardet) || defined(DEBUG_jgmyers)
	const char * ProberName[] = { "UTF-8", "SJIS", "EUC-JP", "GB18030", "EUC-KR", "Big5", "EUC-TW", };
#endif

nsMBCSGroupProber::nsMBCSGroupProber(uint32 aLanguageFilter)
{
	for(uint32 i = 0; i < NUM_OF_PROBERS; i++)
		mProbers[i] = nsnull;
	mProbers[0] = new nsUTF8Prober();
	if(aLanguageFilter & NS_FILTER_JAPANESE) {
		mProbers[1] = new nsSJISProber(aLanguageFilter == NS_FILTER_JAPANESE);
		mProbers[2] = new nsEUCJPProber(aLanguageFilter == NS_FILTER_JAPANESE);
	}
	if(aLanguageFilter & NS_FILTER_CHINESE_SIMPLIFIED)
		mProbers[3] = new nsGB18030Prober(aLanguageFilter == NS_FILTER_CHINESE_SIMPLIFIED);
	if(aLanguageFilter & NS_FILTER_KOREAN)
		mProbers[4] = new nsEUCKRProber(aLanguageFilter == NS_FILTER_KOREAN);
	if(aLanguageFilter & NS_FILTER_CHINESE_TRADITIONAL) {
		mProbers[5] = new nsBig5Prober(aLanguageFilter == NS_FILTER_CHINESE_TRADITIONAL);
		mProbers[6] = new nsEUCTWProber(aLanguageFilter == NS_FILTER_CHINESE_TRADITIONAL);
	}
	Reset();
}

nsMBCSGroupProber::~nsMBCSGroupProber()
{
	for(uint32 i = 0; i < NUM_OF_PROBERS; i++)
		delete mProbers[i];
}

const char * nsMBCSGroupProber::GetCharSetName()
{
	if(mBestGuess == -1) {
		GetConfidence();
		if(mBestGuess == -1)
			mBestGuess = 0;
	}
	return mProbers[mBestGuess]->GetCharSetName();
}

void nsMBCSGroupProber::Reset()
{
	mActiveNum = 0;
	for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
		if(mProbers[i]) {
			mProbers[i]->Reset();
			mIsActive[i] = true;
			++mActiveNum;
		}
		else
			mIsActive[i] = false;
	}
	mBestGuess = -1;
	mState = eDetecting;
	mKeepNext = 0;
}

nsProbingState nsMBCSGroupProber::HandleData(const char * aBuf, uint32 aLen)
{
	nsProbingState st;
	uint32 start = 0;
	uint32 keepNext = mKeepNext;
	//do filtering to reduce load to probers
	for(uint32 pos = 0; pos < aLen; ++pos) {
		if(aBuf[pos] & 0x80) {
			if(!keepNext)
				start = pos;
			keepNext = 2;
		}
		else if(keepNext) {
			if(--keepNext == 0) {
				for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
					if(mIsActive[i]) {
						st = mProbers[i]->HandleData(aBuf + start, pos + 1 - start);
						if(st == eFoundIt) {
							mBestGuess = i;
							mState = eFoundIt;
							return mState;
						}
					}
				}
			}
		}
	}
	if(keepNext) {
		for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
			if(mIsActive[i]) {
				st = mProbers[i]->HandleData(aBuf + start, aLen - start);
				if(st == eFoundIt) {
					mBestGuess = i;
					mState = eFoundIt;
					return mState;
				}
			}
		}
	}
	mKeepNext = keepNext;
	return mState;
}

float nsMBCSGroupProber::GetConfidence() const
{
	float bestConf = 0.0;
	switch(mState) {
		case eFoundIt: return 0.99f;
		case eNotMe:   return 0.01f;
		default:
			{
				for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
					if(mIsActive[i]) {
						const float cf = mProbers[i]->GetConfidence();
						if(bestConf < cf) {
							bestConf = cf;
							mBestGuess = i;
						}
					}
				}
			}
			break;
	}
	return bestConf;
}

#ifdef DEBUG_chardet
void nsMBCSGroupProber::DumpStatus()
{
	float cf;
	GetConfidence();
	for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
		if(!mIsActive[i])
			printf("  MBCS inactive: [%s] (confidence is too low).\r\n", ProberName[i]);
		else {
			cf = mProbers[i]->GetConfidence();
			printf("  MBCS %1.3f: [%s]\r\n", cf, ProberName[i]);
		}
	}
}
#endif

#ifdef DEBUG_jgmyers
void nsMBCSGroupProber::GetDetectorState(nsUniversalDetector::DetectorState(&states)[nsUniversalDetector::NumDetectors], uint32 &offset)
{
	for(uint32 i = 0; i < NUM_OF_PROBERS; ++i) {
		states[offset].name = ProberName[i];
		states[offset].isActive = mIsActive[i];
		states[offset].confidence = mIsActive[i] ? mProbers[i]->GetConfidence() : 0.0;
		++offset;
	}
}
#endif /* DEBUG_jgmyers */
//
//
//
/*
   Modification from frank tang's original work:
   . 0x00 is allowed as a legal character. Since some web pages contains this char in
   text stream.
 */
// BIG5
//
static const uint32 BIG5_cls [ 256 / 8 ] = {
//PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 00 - 07    //allow 0x00 as legal value
	PCK4BITS(1, 1, 1, 1, 1, 1, 0, 0), // 08 - 0f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 10 - 17
	PCK4BITS(1, 1, 1, 0, 1, 1, 1, 1), // 18 - 1f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 20 - 27
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 28 - 2f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 30 - 37
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 38 - 3f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 40 - 47
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 48 - 4f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 50 - 57
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 58 - 5f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 60 - 67
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 68 - 6f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 70 - 77
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 1), // 78 - 7f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 80 - 87
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 88 - 8f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 90 - 97
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 98 - 9f
	PCK4BITS(4, 3, 3, 3, 3, 3, 3, 3), // a0 - a7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // a8 - af
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // b0 - b7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // b8 - bf
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // c0 - c7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // c8 - cf
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // d0 - d7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // d8 - df
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // e0 - e7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // e8 - ef
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // f0 - f7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 0) // f8 - ff
};

static const uint32 BIG5_st[3] = {
	PCK4BITS(eError, eStart, eStart,     3, eError, eError, eError, eError), //00-07
	PCK4BITS(eError, eError, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eError), //08-0f
	PCK4BITS(eError, eStart, eStart, eStart, eStart, eStart, eStart, eStart) //10-17
};

static const uint32 Big5CharLenTable[] = {0, 1, 1, 2, 0};

SMModel const Big5SMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, BIG5_cls },
	5,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, BIG5_st },
	Big5CharLenTable, "BIG5",
};

static const uint32 EUCJP_cls [ 256 / 8 ] = {
//PCK4BITS(5,4,4,4,4,4,4,4),  // 00 - 07
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 00 - 07
	PCK4BITS(4, 4, 4, 4, 4, 4, 5, 5), // 08 - 0f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 10 - 17
	PCK4BITS(4, 4, 4, 5, 4, 4, 4, 4), // 18 - 1f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 20 - 27
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 28 - 2f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 30 - 37
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 38 - 3f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 40 - 47
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 48 - 4f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 50 - 57
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 58 - 5f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 60 - 67
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 68 - 6f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 70 - 77
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 78 - 7f
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // 80 - 87
	PCK4BITS(5, 5, 5, 5, 5, 5, 1, 3), // 88 - 8f
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // 90 - 97
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // 98 - 9f
	PCK4BITS(5, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // e0 - e7
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // e8 - ef
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // f0 - f7
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 5) // f8 - ff
};

static const uint32 EUCJP_st [ 5] = {
	PCK4BITS(3,     4,     3,     5, eStart, eError, eError, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eItsMe, eItsMe, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eStart, eError, eStart, eError, eError, eError), //10-17
	PCK4BITS(eError, eError, eStart, eError, eError, eError,     3, eError), //18-1f
	PCK4BITS(3, eError, eError, eError, eStart, eStart, eStart, eStart) //20-27
};

static const uint32 EUCJPCharLenTable[] = {2, 2, 2, 3, 1, 0};

const SMModel EUCJPSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_cls },
	6,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_st },
	EUCJPCharLenTable, "EUC-JP",
};

static const uint32 EUCKR_cls[256 / 8] = {
//PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 0, 0), // 08 - 0f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 10 - 17
	PCK4BITS(1, 1, 1, 0, 1, 1, 1, 1), // 18 - 1f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 20 - 27
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 28 - 2f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 30 - 37
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 38 - 3f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 40 - 47
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 48 - 4f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 50 - 57
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 58 - 5f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 60 - 67
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 68 - 6f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 70 - 77
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 78 - 7f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 80 - 87
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 88 - 8f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 90 - 97
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 98 - 9f
	PCK4BITS(0, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 3, 3, 3), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 3, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e0 - e7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e8 - ef
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // f0 - f7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 0) // f8 - ff
};

static const uint32 EUCKR_st [ 2] = {
	PCK4BITS(eError, eStart,     3, eError, eError, eError, eError, eError), //00-07
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eError, eError, eStart, eStart) //08-0f
};

static const uint32 EUCKRCharLenTable[] = {0, 1, 2, 0};

const SMModel EUCKRSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCKR_cls },
	4,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCKR_st },
	EUCKRCharLenTable, "EUC-KR",
};

static const uint32 EUCTW_cls[256 / 8] = {
//PCK4BITS(0,2,2,2,2,2,2,2),  // 00 - 07
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 00 - 07
	PCK4BITS(2, 2, 2, 2, 2, 2, 0, 0), // 08 - 0f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 10 - 17
	PCK4BITS(2, 2, 2, 0, 2, 2, 2, 2), // 18 - 1f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 20 - 27
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 28 - 2f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 30 - 37
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 38 - 3f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 40 - 47
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 48 - 4f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 50 - 57
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 58 - 5f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 60 - 67
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 68 - 6f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 70 - 77
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 78 - 7f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 80 - 87
	PCK4BITS(0, 0, 0, 0, 0, 0, 6, 0), // 88 - 8f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 90 - 97
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 98 - 9f
	PCK4BITS(0, 3, 4, 4, 4, 4, 4, 4), // a0 - a7
	PCK4BITS(5, 5, 1, 1, 1, 1, 1, 1), // a8 - af
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // b0 - b7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // b8 - bf
	PCK4BITS(1, 1, 3, 1, 3, 3, 3, 3), // c0 - c7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // c8 - cf
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // d0 - d7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // d8 - df
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // e0 - e7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // e8 - ef
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // f0 - f7
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 0) // f8 - ff
};

static const uint32 EUCTW_st [ 6] = {
	PCK4BITS(eError, eError, eStart,     3,     3,     3,     4, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eError, eError, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eError, eStart, eError), //10-17
	PCK4BITS(eStart, eStart, eStart, eError, eError, eError, eError, eError), //18-1f
	PCK4BITS(5, eError, eError, eError, eStart, eError, eStart, eStart), //20-27
	PCK4BITS(eStart, eError, eStart, eStart, eStart, eStart, eStart, eStart) //28-2f
};

static const uint32 EUCTWCharLenTable[] = {0, 0, 1, 2, 2, 2, 3};

const SMModel EUCTWSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCTW_cls },
	7,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCTW_st },
	EUCTWCharLenTable, "EUC-TW",
};

/* obsolete GB2312 by GB18030
   static uint32 GB2312_cls [ 256 / 8 ] = {
   //PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07
   PCK4BITS(1,1,1,1,1,1,1,1),  // 00 - 07
   PCK4BITS(1,1,1,1,1,1,0,0),  // 08 - 0f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 10 - 17
   PCK4BITS(1,1,1,0,1,1,1,1),  // 18 - 1f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 20 - 27
   PCK4BITS(1,1,1,1,1,1,1,1),  // 28 - 2f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 30 - 37
   PCK4BITS(1,1,1,1,1,1,1,1),  // 38 - 3f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 40 - 47
   PCK4BITS(1,1,1,1,1,1,1,1),  // 48 - 4f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 50 - 57
   PCK4BITS(1,1,1,1,1,1,1,1),  // 58 - 5f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 60 - 67
   PCK4BITS(1,1,1,1,1,1,1,1),  // 68 - 6f
   PCK4BITS(1,1,1,1,1,1,1,1),  // 70 - 77
   PCK4BITS(1,1,1,1,1,1,1,1),  // 78 - 7f
   PCK4BITS(1,0,0,0,0,0,0,0),  // 80 - 87
   PCK4BITS(0,0,0,0,0,0,0,0),  // 88 - 8f
   PCK4BITS(0,0,0,0,0,0,0,0),  // 90 - 97
   PCK4BITS(0,0,0,0,0,0,0,0),  // 98 - 9f
   PCK4BITS(0,2,2,2,2,2,2,2),  // a0 - a7
   PCK4BITS(2,2,3,3,3,3,3,3),  // a8 - af
   PCK4BITS(2,2,2,2,2,2,2,2),  // b0 - b7
   PCK4BITS(2,2,2,2,2,2,2,2),  // b8 - bf
   PCK4BITS(2,2,2,2,2,2,2,2),  // c0 - c7
   PCK4BITS(2,2,2,2,2,2,2,2),  // c8 - cf
   PCK4BITS(2,2,2,2,2,2,2,2),  // d0 - d7
   PCK4BITS(2,2,2,2,2,2,2,2),  // d8 - df
   PCK4BITS(2,2,2,2,2,2,2,2),  // e0 - e7
   PCK4BITS(2,2,2,2,2,2,2,2),  // e8 - ef
   PCK4BITS(2,2,2,2,2,2,2,2),  // f0 - f7
   PCK4BITS(2,2,2,2,2,2,2,0)   // f8 - ff
   };


   static uint32 GB2312_st [ 2] = {
   PCK4BITS(eError,eStart,     3,eError,eError,eError,eError,eError),//00-07
   PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eError,eError,eStart,eStart) //08-0f
   };

   static const uint32 GB2312CharLenTable[] = {0, 1, 2, 0};

   SMModel GB2312SMModel = {
   {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB2312_cls },
   4,
   {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB2312_st },
   GB2312CharLenTable, "GB2312",
   };
 */

// the following state machine data was created by perl script in
// intl/chardet/tools. It should be the same as in PSM detector.
static const uint32 GB18030_cls[256 / 8] = {
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 0, 0), // 08 - 0f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 10 - 17
	PCK4BITS(1, 1, 1, 0, 1, 1, 1, 1), // 18 - 1f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 20 - 27
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 28 - 2f
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // 30 - 37
	PCK4BITS(3, 3, 1, 1, 1, 1, 1, 1), // 38 - 3f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 40 - 47
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 48 - 4f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 50 - 57
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 58 - 5f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 60 - 67
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 68 - 6f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 70 - 77
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 4), // 78 - 7f
	PCK4BITS(5, 6, 6, 6, 6, 6, 6, 6), // 80 - 87
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // 88 - 8f
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // 90 - 97
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // 98 - 9f
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // a0 - a7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // a8 - af
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // b0 - b7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // b8 - bf
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // c0 - c7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // c8 - cf
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // d0 - d7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // d8 - df
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // e0 - e7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // e8 - ef
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // f0 - f7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 0) // f8 - ff
};

static const uint32 GB18030_st [ 6] = {
	PCK4BITS(eError, eStart, eStart, eStart, eStart, eStart,     3, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eError, eError, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eError, eError, eStart), //10-17
	PCK4BITS(4, eError, eStart, eStart, eError, eError, eError, eError), //18-1f
	PCK4BITS(eError, eError,     5, eError, eError, eError, eItsMe, eError), //20-27
	PCK4BITS(eError, eError, eStart, eStart, eStart, eStart, eStart, eStart) //28-2f
};

// To be accurate, the length of class 6 can be either 2 or 4.
// But it is not necessary to discriminate between the two since
// it is used for frequency analysis only, and we are validing
// each code range there as well. So it is safe to set it to be
// 2 here.
static const uint32 GB18030CharLenTable[] = {0, 1, 1, 1, 1, 1, 2};

const SMModel GB18030SMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB18030_cls },
	7,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB18030_st },
	GB18030CharLenTable, "GB18030",
};

// sjis

static const uint32 SJIS_cls [ 256 / 8 ] = {
//PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 0, 0), // 08 - 0f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 10 - 17
	PCK4BITS(1, 1, 1, 0, 1, 1, 1, 1), // 18 - 1f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 20 - 27
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 28 - 2f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 30 - 37
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 38 - 3f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 40 - 47
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 48 - 4f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 50 - 57
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 58 - 5f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 60 - 67
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 68 - 6f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 70 - 77
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 1), // 78 - 7f
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // 80 - 87
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // 88 - 8f
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // 90 - 97
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // 98 - 9f
//0xa0 is illegal in sjis encoding, but some pages does
//contain such byte. We need to be more error forgiven.
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(3, 3, 3, 3, 3, 3, 3, 3), // e0 - e7
	PCK4BITS(3, 3, 3, 3, 3, 4, 4, 4), // e8 - ef
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // f0 - f7
	PCK4BITS(4, 4, 4, 4, 4, 0, 0, 0) // f8 - ff
};

static const uint32 SJIS_st [ 3] = {
	PCK4BITS(eError, eStart, eStart,     3, eError, eError, eError, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eItsMe, eItsMe, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eError, eError, eStart, eStart, eStart, eStart) //10-17
};

static const uint32 SJISCharLenTable[] = {0, 1, 1, 2, 0, 0};

const SMModel SJISSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_cls },
	6,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_st },
	SJISCharLenTable, "SHIFT_JIS",
};

static const uint32 UTF8_cls [ 256 / 8 ] = {
//PCK4BITS(0,1,1,1,1,1,1,1),  // 00 - 07
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 00 - 07  //allow 0x00 as a legal value
	PCK4BITS(1, 1, 1, 1, 1, 1, 0, 0), // 08 - 0f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 10 - 17
	PCK4BITS(1, 1, 1, 0, 1, 1, 1, 1), // 18 - 1f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 20 - 27
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 28 - 2f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 30 - 37
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 38 - 3f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 40 - 47
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 48 - 4f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 50 - 57
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 58 - 5f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 60 - 67
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 68 - 6f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 70 - 77
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 78 - 7f
	PCK4BITS(2, 2, 2, 2, 3, 3, 3, 3), // 80 - 87
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 88 - 8f
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 90 - 97
	PCK4BITS(4, 4, 4, 4, 4, 4, 4, 4), // 98 - 9f
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // a0 - a7
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // a8 - af
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // b0 - b7
	PCK4BITS(5, 5, 5, 5, 5, 5, 5, 5), // b8 - bf
	PCK4BITS(0, 0, 6, 6, 6, 6, 6, 6), // c0 - c7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // c8 - cf
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // d0 - d7
	PCK4BITS(6, 6, 6, 6, 6, 6, 6, 6), // d8 - df
	PCK4BITS(7, 8, 8, 8, 8, 8, 8, 8), // e0 - e7
	PCK4BITS(8, 8, 8, 8, 8, 9, 8, 8), // e8 - ef
	PCK4BITS(10, 11, 11, 11, 11, 11, 11, 11), // f0 - f7
	PCK4BITS(12, 13, 13, 13, 14, 15, 0, 0) // f8 - ff
};

static const uint32 UTF8_st [ 26] = {
	PCK4BITS(eError, eStart, eError, eError, eError, eError,     12,     10), //00-07
	PCK4BITS(9,     11,     8,     7,     6,     5,     4,     3), //08-0f
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //10-17
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //18-1f
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe), //20-27
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe), //28-2f
	PCK4BITS(eError, eError,     5,     5,     5,     5, eError, eError), //30-37
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //38-3f
	PCK4BITS(eError, eError, eError,     5,     5,     5, eError, eError), //40-47
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //48-4f
	PCK4BITS(eError, eError,     7,     7,     7,     7, eError, eError), //50-57
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //58-5f
	PCK4BITS(eError, eError, eError, eError,     7,     7, eError, eError), //60-67
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //68-6f
	PCK4BITS(eError, eError,     9,     9,     9,     9, eError, eError), //70-77
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //78-7f
	PCK4BITS(eError, eError, eError, eError, eError,     9, eError, eError), //80-87
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //88-8f
	PCK4BITS(eError, eError,     12,     12,     12,     12, eError, eError), //90-97
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //98-9f
	PCK4BITS(eError, eError, eError, eError, eError,     12, eError, eError), //a0-a7
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //a8-af
	PCK4BITS(eError, eError,     12,     12,     12, eError, eError, eError), //b0-b7
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError), //b8-bf
	PCK4BITS(eError, eError, eStart, eStart, eStart, eStart, eError, eError), //c0-c7
	PCK4BITS(eError, eError, eError, eError, eError, eError, eError, eError) //c8-cf
};

static const uint32 UTF8CharLenTable[] = {0, 1, 0, 0, 0, 0, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6 };

const SMModel UTF8SMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_cls },
	16,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_st },
	UTF8CharLenTable, "UTF-8",
};
//
//
//
static const uint32 HZ_cls[ 256 / 8 ] = {
	PCK4BITS(1, 0, 0, 0, 0, 0, 0, 0), // 00 - 07
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 08 - 0f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 10 - 17
	PCK4BITS(0, 0, 0, 1, 0, 0, 0, 0), // 18 - 1f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 20 - 27
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 28 - 2f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 30 - 37
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 38 - 3f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 40 - 47
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 48 - 4f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 50 - 57
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 58 - 5f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 60 - 67
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 68 - 6f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 70 - 77
	PCK4BITS(0, 0, 0, 4, 0, 5, 2, 0), // 78 - 7f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 80 - 87
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 88 - 8f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 90 - 97
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // 98 - 9f
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // a0 - a7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // a8 - af
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // b0 - b7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // b8 - bf
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // c0 - c7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // c8 - cf
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // d0 - d7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // d8 - df
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // e0 - e7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // e8 - ef
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1), // f0 - f7
	PCK4BITS(1, 1, 1, 1, 1, 1, 1, 1) // f8 - ff
};

static const uint32 HZ_st [ 6] = {
	PCK4BITS(eStart, eError,     3, eStart, eStart, eStart, eError, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eItsMe, eItsMe, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eError, eError, eStart, eStart,     4, eError), //10-17
	PCK4BITS(5, eError,     6, eError,     5,     5,     4, eError), //18-1f
	PCK4BITS(4, eError,     4,     4,     4, eError,     4, eError), //20-27
	PCK4BITS(4, eItsMe, eStart, eStart, eStart, eStart, eStart, eStart) //28-2f
};

static const uint32 HZCharLenTable[] = {0, 0, 0, 0, 0, 0};

const SMModel HZSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, HZ_cls },
	6,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, HZ_st },
	HZCharLenTable, "HZ-GB-2312",
};

static const uint32 ISO2022CN_cls [ 256 / 8 ] = {
	PCK4BITS(2, 0, 0, 0, 0, 0, 0, 0), // 00 - 07
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 08 - 0f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 10 - 17
	PCK4BITS(0, 0, 0, 1, 0, 0, 0, 0), // 18 - 1f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 20 - 27
	PCK4BITS(0, 3, 0, 0, 0, 0, 0, 0), // 28 - 2f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 30 - 37
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 38 - 3f
	PCK4BITS(0, 0, 0, 4, 0, 0, 0, 0), // 40 - 47
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 48 - 4f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 50 - 57
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 58 - 5f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 60 - 67
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 68 - 6f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 70 - 77
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 78 - 7f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 80 - 87
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 88 - 8f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 90 - 97
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 98 - 9f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e0 - e7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e8 - ef
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // f0 - f7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2) // f8 - ff
};

static const uint32 ISO2022CN_st [ 8] = {
	PCK4BITS(eStart,     3, eError, eStart, eStart, eStart, eStart, eStart), //00-07
	PCK4BITS(eStart, eError, eError, eError, eError, eError, eError, eError), //08-0f
	PCK4BITS(eError, eError, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe), //10-17
	PCK4BITS(eItsMe, eItsMe, eItsMe, eError, eError, eError,     4, eError), //18-1f
	PCK4BITS(eError, eError, eError, eItsMe, eError, eError, eError, eError), //20-27
	PCK4BITS(5,     6, eError, eError, eError, eError, eError, eError), //28-2f
	PCK4BITS(eError, eError, eError, eItsMe, eError, eError, eError, eError), //30-37
	PCK4BITS(eError, eError, eError, eError, eError, eItsMe, eError, eStart) //38-3f
};

static const uint32 ISO2022CNCharLenTable[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

const SMModel ISO2022CNSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022CN_cls },
	9,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022CN_st },
	ISO2022CNCharLenTable, "ISO-2022-CN",
};

static const uint32 ISO2022JP_cls [ 256 / 8 ] = {
	PCK4BITS(2, 0, 0, 0, 0, 0, 0, 0), // 00 - 07
	PCK4BITS(0, 0, 0, 0, 0, 0, 2, 2), // 08 - 0f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 10 - 17
	PCK4BITS(0, 0, 0, 1, 0, 0, 0, 0), // 18 - 1f
	PCK4BITS(0, 0, 0, 0, 7, 0, 0, 0), // 20 - 27
	PCK4BITS(3, 0, 0, 0, 0, 0, 0, 0), // 28 - 2f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 30 - 37
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 38 - 3f
	PCK4BITS(6, 0, 4, 0, 8, 0, 0, 0), // 40 - 47
	PCK4BITS(0, 9, 5, 0, 0, 0, 0, 0), // 48 - 4f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 50 - 57
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 58 - 5f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 60 - 67
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 68 - 6f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 70 - 77
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 78 - 7f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 80 - 87
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 88 - 8f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 90 - 97
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 98 - 9f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e0 - e7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e8 - ef
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // f0 - f7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2) // f8 - ff
};

static const uint32 ISO2022JP_st [ 9] = {
	PCK4BITS(eStart,     3, eError, eStart, eStart, eStart, eStart, eStart), //00-07
	PCK4BITS(eStart, eStart, eError, eError, eError, eError, eError, eError), //08-0f
	PCK4BITS(eError, eError, eError, eError, eItsMe, eItsMe, eItsMe, eItsMe), //10-17
	PCK4BITS(eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eItsMe, eError, eError), //18-1f
	PCK4BITS(eError,     5, eError, eError, eError,     4, eError, eError), //20-27
	PCK4BITS(eError, eError, eError,     6, eItsMe, eError, eItsMe, eError), //28-2f
	PCK4BITS(eError, eError, eError, eError, eError, eError, eItsMe, eItsMe), //30-37
	PCK4BITS(eError, eError, eError, eItsMe, eError, eError, eError, eError), //38-3f
	PCK4BITS(eError, eError, eError, eError, eItsMe, eError, eStart, eStart) //40-47
};

static const uint32 ISO2022JPCharLenTable[] = {0, 0, 0, 0, 0, 0, 0, 0};

const SMModel ISO2022JPSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022JP_cls },
	10,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022JP_st },
	ISO2022JPCharLenTable, "ISO-2022-JP",
};

static const uint32 ISO2022KR_cls [ 256 / 8 ] = {
	PCK4BITS(2, 0, 0, 0, 0, 0, 0, 0), // 00 - 07
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 08 - 0f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 10 - 17
	PCK4BITS(0, 0, 0, 1, 0, 0, 0, 0), // 18 - 1f
	PCK4BITS(0, 0, 0, 0, 3, 0, 0, 0), // 20 - 27
	PCK4BITS(0, 4, 0, 0, 0, 0, 0, 0), // 28 - 2f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 30 - 37
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 38 - 3f
	PCK4BITS(0, 0, 0, 5, 0, 0, 0, 0), // 40 - 47
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 48 - 4f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 50 - 57
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 58 - 5f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 60 - 67
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 68 - 6f
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 70 - 77
	PCK4BITS(0, 0, 0, 0, 0, 0, 0, 0), // 78 - 7f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 80 - 87
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 88 - 8f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 90 - 97
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // 98 - 9f
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a0 - a7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // a8 - af
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b0 - b7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // b8 - bf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c0 - c7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // c8 - cf
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d0 - d7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // d8 - df
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e0 - e7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // e8 - ef
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2), // f0 - f7
	PCK4BITS(2, 2, 2, 2, 2, 2, 2, 2) // f8 - ff
};

static const uint32 ISO2022KR_st [ 5] = {
	PCK4BITS(eStart,     3, eError, eStart, eStart, eStart, eError, eError), //00-07
	PCK4BITS(eError, eError, eError, eError, eItsMe, eItsMe, eItsMe, eItsMe), //08-0f
	PCK4BITS(eItsMe, eItsMe, eError, eError, eError,     4, eError, eError), //10-17
	PCK4BITS(eError, eError, eError, eError,     5, eError, eError, eError), //18-1f
	PCK4BITS(eError, eError, eError, eItsMe, eStart, eStart, eStart, eStart) //20-27
};

static const uint32 ISO2022KRCharLenTable[] = {0, 0, 0, 0, 0, 0};

const SMModel ISO2022KRSMModel = {
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022KR_cls },
	6,
	{eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022KR_st },
	ISO2022KRCharLenTable, "ISO-2022-KR",
};
//
//
//
nsCodingStateMachine::nsCodingStateMachine(const SMModel* sm) : mModel(sm), mCurrentState(eStart)
{
}

nsSMState nsCodingStateMachine::NextState(char c)
{
#define GETCLASS(c) GETFROMPCK(((uchar)(c)), mModel->classTable)
	//for each byte we get its class , if it is first byte, we also get byte length
	const uint32 byteCls = GETCLASS(c);
	if(mCurrentState == eStart) {
		mCurrentBytePos = 0;
		mCurrentCharLen = mModel->charLenTable[byteCls];
	}
	//from byte's class and stateTable, we get its next state
	mCurrentState = (nsSMState)GETFROMPCK(mCurrentState*(mModel->classFactor)+byteCls, mModel->stateTable);
	mCurrentBytePos++;
	return mCurrentState;
#undef GETCLASS
}

nsEscCharSetProber::nsEscCharSetProber(uint32 aLanguageFilter)
{
	for(uint32 i = 0; i < SIZEOFARRAY(mCodingSM); i++)
		mCodingSM[i] = nsnull;
	if(aLanguageFilter & NS_FILTER_CHINESE_SIMPLIFIED) {
		mCodingSM[0] = new nsCodingStateMachine(&HZSMModel);
		mCodingSM[1] = new nsCodingStateMachine(&ISO2022CNSMModel);
	}
	if(aLanguageFilter & NS_FILTER_JAPANESE)
		mCodingSM[2] = new nsCodingStateMachine(&ISO2022JPSMModel);
	if(aLanguageFilter & NS_FILTER_KOREAN)
		mCodingSM[3] = new nsCodingStateMachine(&ISO2022KRSMModel);
	mActiveSM = SIZEOFARRAY(mCodingSM);
	mState = eDetecting;
	mDetectedCharset = nsnull;
}

nsEscCharSetProber::~nsEscCharSetProber()
{
	for(uint32 i = 0; i < SIZEOFARRAY(mCodingSM); i++)
		delete mCodingSM[i];
}

void nsEscCharSetProber::Reset()
{
	mState = eDetecting;
	for(uint32 i = 0; i < SIZEOFARRAY(mCodingSM); i++)
		if(mCodingSM[i])
			mCodingSM[i]->Reset();
	mActiveSM = SIZEOFARRAY(mCodingSM);
	mDetectedCharset = nsnull;
}

nsProbingState nsEscCharSetProber::HandleData(const char * aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen && mState == eDetecting; i++) {
		for(int32 j = mActiveSM-1; j>= 0; j--) {
			if(mCodingSM[j]) {
				const nsSMState codingState = mCodingSM[j]->NextState(aBuf[i]);
				if(codingState == eItsMe) {
					mState = eFoundIt;
					mDetectedCharset = mCodingSM[j]->GetCodingStateMachine();
					return mState;
				}
			}
		}
	}
	return mState;
}
//
//
//
nsUniversalDetector::nsUniversalDetector(uint32 aLanguageFilter)
{
	//mNbspFound = false;
	//mDone = false;
	//mInTag = false;
	//mStart = true;
	//mGotData = false;
	Flags = fStart;
	mBestGuess = -1; //illegal value as signal
	mEscCharSetProber = nsnull;
	mDetectedCharset = nsnull;
	mInputState = ePureAscii;
	mLastChar = '\0';
	mLanguageFilter = aLanguageFilter;
	for(uint32 i = 0; i < SIZEOFARRAY(mCharSetProbers); i++)
		mCharSetProbers[i] = nsnull;
}

nsUniversalDetector::~nsUniversalDetector()
{
	for(int32 i = 0; i < SIZEOFARRAY(mCharSetProbers); i++)
		delete mCharSetProbers[i];
	delete mEscCharSetProber;
}

void nsUniversalDetector::Reset()
{
	//mNbspFound = false;
	//mDone = false;
	//mInTag = false;
	//mStart = true;
	//mGotData = false;
	Flags = fStart;
	mBestGuess = -1; // illegal value as signal
	mDetectedCharset = nsnull;
	mInputState = ePureAscii;
	mLastChar = '\0';
	if(mEscCharSetProber)
		mEscCharSetProber->Reset();
	for(uint32 i = 0; i < SIZEOFARRAY(mCharSetProbers); i++)
		if(mCharSetProbers[i])
			mCharSetProbers[i]->Reset();
}

nsresult nsUniversalDetector::HandleData(const char * aBuf, uint32 aLen)
{
	if(Flags & fDone)
		return NS_OK;
	if(aLen > 0)
		Flags |= fGotData;
	// If the data starts with BOM, we know it is UTF
	if(Flags & fStart) {
		Flags &= ~fStart;
		if(aLen > 2)
			switch(aBuf[0]) {
				case '\xEF':
				    if(('\xBB' == aBuf[1]) && ('\xBF' == aBuf[2])) // EF BB BF: UTF-8 encoded BOM
					    mDetectedCharset = "UTF-8";
				    break;
				case '\xFE':
				    if('\xFF' == aBuf[1]) // FE FF: UTF-16, big endian BOM
					    mDetectedCharset = "UTF-16";
				    break;
				case '\xFF':
				    if('\xFE' == aBuf[1]) {
					    if(aLen > 3 && aBuf[2] == '\x00' && aBuf[3] == '\x00') // FF FE 00 00: UTF-32 (LE)
						    mDetectedCharset = "UTF-32";
					    else // FF FE: UTF-16, little endian BOM
						    mDetectedCharset = "UTF-16";
				    }
				    break;
				case '\x00':
				    if(aLen > 3 && aBuf[1] == '\x00' && aBuf[2] == '\xFE' && aBuf[3] == '\xFF') // 00 00 FE FF: UTF-32 (BE)
					    mDetectedCharset = "UTF-32";
				    break;
			}

		if(mDetectedCharset) {
			Flags |= fDone;
			return NS_OK;
		}
	}
	uint32 i;
	for(i = 0; i < aLen; i++) {
		// If every other character is ASCII or 0xA0, we don't run charset probers.
		// 0xA0 (NBSP in a few charset) is apparently a rare exception
		// of non-ASCII character often contained in nearly-ASCII text.
		if(aBuf[i] & '\x80' && aBuf[i] != '\xA0') {
			// We got a non-ASCII byte (high-byte) 
			if(mInputState != eHighbyte) {
				//adjust state
				mInputState = eHighbyte;
				//kill mEscCharSetProber if it is active
				ZDELETE(mEscCharSetProber);
				//start multibyte and singlebyte charset prober
				if(!mCharSetProbers[0]) {
					mCharSetProbers[0] = new nsMBCSGroupProber(mLanguageFilter);
					if(!mCharSetProbers[0])
						return NS_ERROR_OUT_OF_MEMORY;
				}
				if(!mCharSetProbers[1] && (mLanguageFilter & NS_FILTER_NON_CJK)) {
					mCharSetProbers[1] = new nsSBCSGroupProber;
					if(!mCharSetProbers[1])
						return NS_ERROR_OUT_OF_MEMORY;
				}
				if(!mCharSetProbers[2]) {
					mCharSetProbers[2] = new nsLatin1Prober;
					if(!mCharSetProbers[2])
						return NS_ERROR_OUT_OF_MEMORY;
				}
			}
		}
		else { // Just pure ASCII or NBSP so far. 
			if(aBuf[i] == '\xA0') {
				// ASCII with the only exception of NBSP seems quite common.
				// I doubt it is really necessary to train a model here, so let's just make an exception.
				Flags |= fNbspFound;
			}
			else if(mInputState == ePureAscii && (aBuf[i] == '\033' || (aBuf[i] == '{' && mLastChar == '~'))) {
				// We found an escape character or HZ "~{". 
				mInputState = eEscAscii;
			}
			mLastChar = aBuf[i];
		}
	}
	nsProbingState st;
	switch(mInputState) {
		case eEscAscii:
		    if(!mEscCharSetProber) {
			    mEscCharSetProber = new nsEscCharSetProber(mLanguageFilter);
			    if(!mEscCharSetProber)
				    return NS_ERROR_OUT_OF_MEMORY;
		    }
		    st = mEscCharSetProber->HandleData(aBuf, aLen);
		    if(st == eFoundIt) {
				Flags |= fDone;
			    mDetectedCharset = mEscCharSetProber->GetCharSetName();
		    }
		    else if(Flags & fNbspFound)
			    mDetectedCharset = "ISO-8859-1";
		    else
			    mDetectedCharset = "ASCII"; // ASCII with the ESC character (or the sequence "~{") is still ASCII until proven otherwise. 
		    break;
		case eHighbyte:
		    for(i = 0; i < SIZEOFARRAY(mCharSetProbers); i++) {
			    if(mCharSetProbers[i]) {
				    st = mCharSetProbers[i]->HandleData(aBuf, aLen);
				    if(st == eFoundIt) {
						Flags |= fDone;
					    mDetectedCharset = mCharSetProbers[i]->GetCharSetName();
					    return NS_OK;
				    }
			    }
		    }
		    break;
		default:
		    if(Flags & fNbspFound) // ISO-8859-1 is a good result candidate for ASCII + NBSP. (though it could have been any ISO-8859 encoding).
			    mDetectedCharset = "ISO-8859-1";
		    else // Pure ASCII 
			    mDetectedCharset = "ASCII";
		    break;
	}
	return NS_OK;
}

void nsUniversalDetector::DataEnd()
{
	if(Flags & fGotData) { // if we haven't got any data yet, return immediately caller program sometimes call DataEnd before anything has been sent to detector
		if(mDetectedCharset) {
			Flags |= fDone;
			Report(mDetectedCharset);
		}
		else {
			switch(mInputState) {
				case eHighbyte:
					{
						float maxProberConfidence = 0.0f;
						int32 maxProber = 0;
						for(int32 i = 0; i < SIZEOFARRAY(mCharSetProbers); i++) {
							if(mCharSetProbers[i]) {
								const float proberConfidence = mCharSetProbers[i]->GetConfidence();
								if(proberConfidence > maxProberConfidence) {
									maxProberConfidence = proberConfidence;
									maxProber = i;
								}
							}
						}
						//do not report anything because we are not confident of it, that's in fact a negative answer
						if(maxProberConfidence > MINIMUM_THRESHOLD)
							Report(mCharSetProbers[maxProber]->GetCharSetName());
					}
					break;
				case eEscAscii:
					break;
				default:
					;
			}
		}
	}
}

class HandleUniversalDetector : public nsUniversalDetector {
public:
	HandleUniversalDetector() : nsUniversalDetector(NS_FILTER_ALL), m_charset(0)
	{
	}
	virtual ~HandleUniversalDetector()
	{
		SAlloc::F(m_charset);
	}
	virtual void Report(const char * charset)
	{
		SAlloc::F(m_charset);
		m_charset = sstrdup(charset);
	}
	virtual void Reset()
	{
		nsUniversalDetector::Reset();
		SAlloc::F(m_charset);
		m_charset = sstrdup("");
	}
	const char * GetCharset() const { return NZOR(m_charset, ""); }
protected:
	char * m_charset;
};

uchardet_t uchardet_new() { return reinterpret_cast<uchardet_t>(new HandleUniversalDetector()); }
void uchardet_delete(uchardet_t ud) { delete reinterpret_cast<HandleUniversalDetector*>(ud); }

int uchardet_handle_data(uchardet_t ud, const char * data, size_t len) 
{
	nsresult ret = reinterpret_cast<HandleUniversalDetector*>(ud)->HandleData(data, (uint32)len);
	return (ret != NS_OK);
}

void uchardet_data_end(uchardet_t ud) { reinterpret_cast<HandleUniversalDetector*>(ud)->DataEnd(); }
void uchardet_reset(uchardet_t ud) { reinterpret_cast<HandleUniversalDetector*>(ud)->Reset(); }
const char * uchardet_get_charset(uchardet_t ud) { return reinterpret_cast<HandleUniversalDetector*>(ud)->GetCharset(); }
