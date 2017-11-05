// UCHARDET-INTERNAL.H
//
// BEGIN LICENSE BLOCK
// Version: MPL 1.1/GPL 2.0/LGPL 2.1
// 
// The contents of this file are subject to the Mozilla Public License Version 1.1 (the "License"); 
// you may not use this file except in compliance with the License. 
// You may obtain a copy of the License at http://www.mozilla.org/MPL/
// 
// Software distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
// for the specific language governing rights and limitations under the License.
// 
// The Original Code is Mozilla Communicator client code.
// 
// The Initial Developer of the Original Code is Netscape Communications Corporation.
// Portions created by the Initial Developer are Copyright (C) 1998 the Initial Developer. All Rights Reserved.
// 
// Contributor(s):
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
// END LICENSE BLOCK
//
#ifndef UCHARDET_INTERNAL_H // {
#define UCHARDET_INTERNAL_H

#include <slib.h>
#include <uchardet.h>

class nsCharSetProber;

#define nsnull 0

enum nsresult {
	NS_OK,
	NS_ERROR_OUT_OF_MEMORY
};
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

#define PCK16BITS(a, b)            ((uint32)(((b) << 16) | (a)))
#define PCK8BITS(a, b, c, d)         PCK16BITS(((uint32)(((b) << 8) | (a))), ((uint32)(((d) << 8) | (c))))
#define PCK4BITS(a, b, c, d, e, f, g, h) PCK8BITS(((uint32)(((b) << 4)|(a))), ((uint32)(((d) << 4)|(c))), ((uint32)(((f) << 4)|(e))), ((uint32)(((h) << 4)|(g))))
#define GETFROMPCK(i, c) (((((c).data)[(i)>>(c).idxsft])>>(((i)&(c).sftmsk)<<(c).bitsft))&(c).unitmsk)
//
//#include <nsUniversalDetector.h>
//
#define NUM_OF_CHARSET_PROBERS  3

enum nsInputState {
	ePureAscii = 0,
	eEscAscii  = 1,
	eHighbyte  = 2
};

#define NS_FILTER_CHINESE_SIMPLIFIED  0x01
#define NS_FILTER_CHINESE_TRADITIONAL 0x02
#define NS_FILTER_JAPANESE            0x04
#define NS_FILTER_KOREAN              0x08
#define NS_FILTER_NON_CJK             0x10
#define NS_FILTER_ALL                 0x1F
#define NS_FILTER_CHINESE (NS_FILTER_CHINESE_SIMPLIFIED | NS_FILTER_CHINESE_TRADITIONAL)
#define NS_FILTER_CJK (NS_FILTER_CHINESE_SIMPLIFIED | NS_FILTER_CHINESE_TRADITIONAL | NS_FILTER_JAPANESE | NS_FILTER_KOREAN)

class nsUniversalDetector {
public:
	nsUniversalDetector(uint32 aLanguageFilter);
	virtual ~nsUniversalDetector();
	virtual nsresult HandleData(const char* aBuf, uint32 aLen);
	virtual void DataEnd();
protected:
	virtual void Report(const char* aCharset) = 0;
	virtual void Reset();
	nsInputState mInputState;
	//bool   mNbspFound;
	//bool   mDone;
	//bool   mInTag;
	//bool   mStart;
	//bool   mGotData;
	enum {
		fNbspFound = 0x0001,
		fDone      = 0x0002,
		fInTag     = 0x0004,
		fStart     = 0x0008,
		fGotData   = 0x0010
	};
	uint   Flags;
	const  char * mDetectedCharset;
	int32  mBestGuess;
	uint32 mLanguageFilter;
	nsCharSetProber  * mCharSetProbers[NUM_OF_CHARSET_PROBERS];
	nsCharSetProber  * mEscCharSetProber;
	char   mLastChar;
	uint8  Reserve[3]; // @alignment
};
//
//#include <CharDistribution.h>
//
#define ENOUGH_DATA_THRESHOLD 1024
#define MINIMUM_DATA_THRESHOLD  4

class CharDistributionAnalysis {
public:
	CharDistributionAnalysis() 
	{
		Reset(false);
	}
	//
	// feed a block of data and do distribution analysis
	//
	void HandleData(const char* aBuf, uint32 aLen) 
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
			if((uint32)order < mTableSize) {
				if(512 > mCharToFreqOrder[order])
					mFreqChars++;
			}
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
	EUCTWDistributionAnalysis();
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
	EUCKRDistributionAnalysis();
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
	GB2312DistributionAnalysis();
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
	Big5DistributionAnalysis();
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
	SJISDistributionAnalysis();
protected:
	//for sjis encoding, we are interested
	//  first  byte range: 0x81 -- 0x9f , 0xe0 -- 0xfe
	//  second byte range: 0x40 -- 0x7e,  0x81 -- oxfe
	//no validation needed here. State machine has done that
	int32 GetOrder(const char* str) const
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
	EUCJPDistributionAnalysis();
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
//
//#include <JpCntx.h>
//
#define NUM_OF_CATEGORY      6
#define ENOUGH_REL_THRESHOLD 100
#define MAX_REL_THRESHOLD    1000

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
	bool   GotEnoughData() 
	{
		return (mTotalRel > ENOUGH_REL_THRESHOLD);
	}
protected:
	virtual int32 GetOrder(const char * str, uint32 * charLen) const = 0;
	virtual int32 GetOrder(const char * str) const = 0;
	
	uint32 mRelSample[NUM_OF_CATEGORY]; //category counters, each integer counts sequences in its category
	uint32 mTotalRel; //total sequence received
	uint32 mDataThreshold; //Number of sequences needed to trigger detection
	int32  mLastCharOrder; //The order of previous char
	//if last byte in current buffer is not the last byte of a character, we
	//need to know how many byte to skip in next buffer.
	uint32 mNeedToSkipCharNum;
	bool   mDone; //If this flag is set to true, detection is done and conclusion has been made
};

class SJISContextAnalysis : public JapaneseContextAnalysis {
	//SJISContextAnalysis(){};
protected:
	int32 GetOrder(const char * str, uint32 * charLen) const;
	int32 GetOrder(const char * str) const
	{
		//We only interested in Hiragana, so first byte is '\202'
		return (*str == '\202' && (uchar)*(str+1) >= (uchar)0x9f && (uchar)*(str+1) <= (uchar)0xf1) ? ((uchar)*(str+1) - (uchar)0x9f) : -1;
	}
};

class EUCJPContextAnalysis : public JapaneseContextAnalysis {
protected:
	int32 GetOrder(const char* str, uint32 * charLen) const;
	int32 GetOrder(const char * str) const
	{
		// We only interested in Hiragana, so first byte is '\244'
		return (*str == '\244' && (uchar)*(str+1) >= (uchar)0xa1 && (uchar)*(str+1) <= (uchar)0xf3) ? ((uchar)*(str+1) - (uchar)0xa1) : -1;
	}
};
//
//#include <nsCharSetProber.h>
//
//#define DEBUG_chardet // Uncomment this for debug dump.

enum nsProbingState {
	eDetecting = 0, //We are still detecting, no sure answer yet, but caller can ask for confidence.
	eFoundIt = 1, //That's a positive answer
	eNotMe = 2  //Negative answer
};

#define SHORTCUT_THRESHOLD 0.95f

class nsCharSetProber {
public:
	virtual ~nsCharSetProber() 
	{
	}
	virtual const char * GetCharSetName() = 0;
	virtual nsProbingState HandleData(const char* aBuf, uint32 aLen) = 0;
	virtual nsProbingState GetState() const = 0;
	virtual void  Reset()  = 0;
	virtual float GetConfidence() const = 0;
#ifdef DEBUG_chardet
	virtual void  DumpStatus() 
	{
	};
#endif
	// Helper functions used in the Latin1 and Group probers.
	// both functions Allocate a new buffer for newBuf. This buffer should be
	// freed by the caller using PR_FREEIF.
	// Both functions return false in case of memory allocation failure.
	static bool FilterWithoutEnglishLetters(const char* aBuf, uint32 aLen, char** newBuf, uint32& newLen);
	static bool FilterWithEnglishLetters(const char* aBuf, uint32 aLen, char** newBuf, uint32& newLen);
};
//
//#include <nsSBCharSetProber.h>
//
/** Codepoints **/
#define ILL 255 /* Illegal codepoints.*/
#define CTR 254 /* Control character. */
#define SYM 253 /* Symbols and punctuation that does not belong to words. */
#define RET 252 /* Return/Line feeds. */
#define NUM 251 /* Numbers 0-9. */

#define SB_ENOUGH_REL_THRESHOLD  1024
#define POSITIVE_SHORTCUT_THRESHOLD  (float)0.95
#define NEGATIVE_SHORTCUT_THRESHOLD  (float)0.05
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
	virtual nsProbingState HandleData(const char* aBuf, uint32 aLen);
	virtual nsProbingState GetState() const
	{
		return mState;
	}
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
	nsProbingState mState;
	const SequenceModel* const mModel;
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
	uint32 GetCurrentCharLen() const;
	void   Reset();
	const  char * GetCodingStateMachine() const;
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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "BIG5"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void      Reset();
	float     GetConfidence() const;
protected:
	void      GetDistribution(uint32 aCharLen, const char* aStr);

	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	//Big5ContextAnalysis mContextAnalyser;
	Big5DistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};
//
//#include <nsEscCharsetProber.h>
//
#define NUM_OF_ESC_CHARSETS   4

class nsEscCharSetProber : public nsCharSetProber {
public:
	nsEscCharSetProber(uint32 aLanguageFilter);
	virtual ~nsEscCharSetProber();
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return mDetectedCharset; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const
	{
		return (float)0.99;
	}
protected:
	void   GetDistribution(uint32 aCharLen, const char* aStr);

	nsCodingStateMachine* mCodingSM[NUM_OF_ESC_CHARSETS];
	uint32 mActiveSM;
	nsProbingState mState;
	const char *  mDetectedCharset;
};
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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-JP"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	EUCJPContextAnalysis mContextAnalyser;
	EUCJPDistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};
//
//#include <nsEUCKRProber.h>
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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-KR"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	void   GetDistribution(uint32 aCharLen, const char* aStr);

	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	//EUCKRContextAnalysis mContextAnalyser;
	EUCKRDistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};
//
//#include <nsEUCTWProber.h>
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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "EUC-TW"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	void   GetDistribution(uint32 aCharLen, const char* aStr);

	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	//EUCTWContextAnalysis mContextAnalyser;
	EUCTWDistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
};
//
//#include <nsGB2312Prober.h>
//
// We use GB18030 to replace GB2312, because 18030 is a superset.

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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "GB18030"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	void      GetDistribution(uint32 aCharLen, const char* aStr);

	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	//GB2312ContextAnalysis mContextAnalyser;
	GB2312DistributionAnalysis mDistributionAnalyser;
	char mLastChar[2];
	bool mIsPreferredLanguage;
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
	virtual nsProbingState HandleData(const char* aBuf, uint32 aLen);
	virtual const char * GetCharSetName();
	virtual void Reset();
	virtual nsProbingState GetState() const;
	virtual float GetConfidence() const 
	{
		return (float)0.0;
	}
	void SetModelProbers(nsCharSetProber * logicalPrb, nsCharSetProber * visualPrb)
	{
		mLogicalProb = logicalPrb; mVisualProb = visualPrb;
	}
#ifdef DEBUG_chardet
	virtual void  DumpStatus();
#endif
protected:
	static bool isFinal(char c);
	static bool isNonFinal(char c);
	int32 mFinalCharLogicalScore, mFinalCharVisualScore;
	char mPrev, mBeforePrev; // The two last characters seen in the previous buffer.
	nsCharSetProber * mLogicalProb, * mVisualProb; // These probers are owned by the group prober.
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
 * what little evidence I could find, it seems that its general directionality
 * is Logical.
 *
 * To sum up all of the above, the Hebrew probing mechanism knows about two
 * charsets:
 * Visual Hebrew - "ISO-8859-8" - backwards text - Words and sentences are
 *    backwards while line order is natural. For charset recognition purposes
 *    the line order is unimportant (In fact, for this implementation, even
 *    word order is unimportant).
 * Logical Hebrew - "windows-1255" - normal, naturally ordered text.
 *
 * "ISO-8859-8-I" is a subset of windows-1255 and doesn't need to be
 *    specifically identified.
 * "x-mac-hebrew" is also identified as windows-1255. A text in x-mac-hebrew
 *    that contain special punctuation marks or diacritics is displayed with
 *    some unconverted characters showing as question marks. This problem might
 *    be corrected using another model prober for x-mac-hebrew. Due to the fact
 *    that x-mac-hebrew texts are so rare, writing another model prober isn't
 *    worth the effort and performance hit.
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
#define FREQ_CAT_NUM    4

class nsLatin1Prober : public nsCharSetProber {
public:
	nsLatin1Prober()
	{
		Reset();
	}
	virtual ~nsLatin1Prober()
	{
	}
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "WINDOWS-1252"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	virtual void  DumpStatus();
#endif
protected:
	nsProbingState mState;
	char mLastCharClass;
	uint32 mFreqCounter[FREQ_CAT_NUM];
};
//
//#include <nsSJISProber.h>
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
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() { return "SHIFT_JIS"; }
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	SJISContextAnalysis mContextAnalyser;
	SJISDistributionAnalysis mDistributionAnalyser;
	char   mLastChar[2];
	bool   mIsPreferredLanguage;
	uint8  Reserve; // @alignment
};
//
//#include <nsUTF8Prober.h>
//
class nsUTF8Prober : public nsCharSetProber {
public:
	nsUTF8Prober()
	{
		mNumOfMBChar = 0;
		mCodingSM = new nsCodingStateMachine(&UTF8SMModel);
		Reset();
	}
	virtual ~nsUTF8Prober()
	{
		delete mCodingSM;
	}
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName() 
	{
		return "UTF-8";
	}
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
protected:
	nsCodingStateMachine* mCodingSM;
	nsProbingState mState;
	uint32 mNumOfMBChar;
};
//
//#include <nsMBCSGroupProber.h>
//
#define NUM_OF_PROBERS    7

class nsMBCSGroupProber : public nsCharSetProber {
public:
	nsMBCSGroupProber(uint32 aLanguageFilter);
	virtual ~nsMBCSGroupProber();
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName();
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	void  DumpStatus();
#endif
#ifdef DEBUG_jgmyers
	void GetDetectorState(nsUniversalDetector::DetectorState(&states)[nsUniversalDetector::NumDetectors], uint32 &offset);
#endif
protected:
	nsProbingState mState;
	nsCharSetProber * mProbers[NUM_OF_PROBERS];
	mutable int32 mBestGuess; // GetCofident modifies this member
	uint32 mActiveNum;
	uint32 mKeepNext;
	bool   mIsActive[NUM_OF_PROBERS];
};
//
//#include <nsSBCSGroupProber.h>
//
#define NUM_OF_SBCS_PROBERS 35

class nsSBCSGroupProber : public nsCharSetProber {
public:
	nsSBCSGroupProber();
	virtual ~nsSBCSGroupProber();
	nsProbingState HandleData(const char* aBuf, uint32 aLen);
	const char * GetCharSetName();
	nsProbingState GetState() const
	{
		return mState;
	}
	void   Reset();
	float  GetConfidence() const;
#ifdef DEBUG_chardet
	void  DumpStatus();
#endif
protected:
	nsProbingState mState;
	nsCharSetProber * mProbers[NUM_OF_SBCS_PROBERS];
	mutable int32  mBestGuess; // GetConfident modifies this member
	uint32 mActiveNum;
	bool   mIsActive[NUM_OF_SBCS_PROBERS];
};
//
/*
extern const SequenceModel Windows_1256ArabicModel;
extern const SequenceModel Iso_8859_6ArabicModel;
extern const SequenceModel Koi8rRussianModel;
extern const SequenceModel Win1251RussianModel;
extern const SequenceModel Latin5RussianModel;
extern const SequenceModel MacCyrillicRussianModel;
extern const SequenceModel Ibm866RussianModel;
extern const SequenceModel Ibm855RussianModel;
extern const SequenceModel Iso_8859_7GreekModel;
extern const SequenceModel Windows_1253GreekModel;
extern const SequenceModel Latin5BulgarianModel;
extern const SequenceModel Win1251BulgarianModel;
extern const SequenceModel Iso_8859_2HungarianModel;
extern const SequenceModel Windows_1250HungarianModel;
extern const SequenceModel Win1255Model;
extern const SequenceModel Tis_620ThaiModel;
extern const SequenceModel Iso_8859_11ThaiModel;
extern const SequenceModel Iso_8859_15FrenchModel;
extern const SequenceModel Iso_8859_1FrenchModel;
extern const SequenceModel Windows_1252FrenchModel;
extern const SequenceModel Iso_8859_15SpanishModel;
extern const SequenceModel Iso_8859_1SpanishModel;
extern const SequenceModel Windows_1252SpanishModel;
extern const SequenceModel Iso_8859_1GermanModel;
extern const SequenceModel Windows_1252GermanModel;
extern const SequenceModel Iso_8859_3EsperantoModel;
extern const SequenceModel Iso_8859_3TurkishModel;
extern const SequenceModel Iso_8859_9TurkishModel;
extern const SequenceModel VisciiVietnameseModel;
extern const SequenceModel Windows_1258VietnameseModel;
extern const SequenceModel Iso_8859_15DanishModel;
extern const SequenceModel Iso_8859_1DanishModel;
extern const SequenceModel Windows_1252DanishModel;
*/
//
#endif // } UCHARDET_INTERNAL_H
