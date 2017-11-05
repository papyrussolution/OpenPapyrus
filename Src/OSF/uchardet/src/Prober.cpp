// Prober.cpp
//
#include <uchardet-internal.h>
#pragma hdrstop
//
// This filter applies to all scripts which do not use English characters
//
bool nsCharSetProber::FilterWithoutEnglishLetters(const char* aBuf, uint32 aLen, char** newBuf, uint32& newLen)
{
	char * newptr;
	char * prevPtr, * curPtr;
	bool meetMSB = false;
	newptr = *newBuf = (char*)SAlloc::M(aLen);
	if(!newptr)
		return false;
	for(curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++) {
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
	newLen = (uint32)(newptr - *newBuf);
	return true;
}

//This filter applies to all scripts which contain both English characters and upper ASCII characters.
bool nsCharSetProber::FilterWithEnglishLetters(const char* aBuf, uint32 aLen, char** newBuf, uint32& newLen)
{
	//do filtering to reduce load to probers
	char * prevPtr, * curPtr;
	bool isInTag = false;
	char * newptr = (char*)SAlloc::M(aLen);
	*newBuf = newptr;
	if(!newptr)
		return false;
	for(curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++) {
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
	newLen = (uint32)(newptr - *newBuf);
	return true;
}
//
//
//
void nsBig5Prober::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsBig5Prober::HandleData(const char* aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
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
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (float)distribCf;
}
//
// for japanese encoding, obeserve characteristic:
// 1, kana character (or hankaku?) often have hight frequency of appereance
// 2, kana character often exist in group
// 3, certain combination of kana is never used in japanese language
//
void nsEUCJPProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mContextAnalyser.Reset(mIsPreferredLanguage);
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsEUCJPProber::HandleData(const char* aBuf, uint32 aLen)
{
	nsSMState codingState;
	for(uint32 i = 0; i < aLen; i++) {
		codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
				mLastChar[1] = aBuf[0];
				mContextAnalyser.HandleOneChar(mLastChar, charLen);
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else{
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
	float contxtCf = mContextAnalyser.GetConfidence();
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (contxtCf > distribCf ? contxtCf : distribCf);
}
//
//
//
void nsEUCKRProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
	//mContextAnalyser.Reset();
}

nsProbingState nsEUCKRProber::HandleData(const char* aBuf, uint32 aLen)
{
	nsSMState codingState;
	for(uint32 i = 0; i < aLen; i++) {
		codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
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
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (float)distribCf;
}
//
//
//
void nsEUCTWProber::Reset()
{
	mCodingSM->Reset();
	mState = eDetecting;
	mDistributionAnalyser.Reset(mIsPreferredLanguage);
	//mContextAnalyser.Reset();
}

nsProbingState nsEUCTWProber::HandleData(const char* aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
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
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (float)distribCf;
}
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

nsProbingState nsGB18030Prober::HandleData(const char* aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen; i++) {
		const nsSMState codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
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
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (float)distribCf;
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

bool nsHebrewProber::isFinal(char c)
{
	return ((c == FINAL_KAF) || (c == FINAL_MEM) || (c == FINAL_NUN) || (c == FINAL_PE) || (c == FINAL_TSADI));
}

bool nsHebrewProber::isNonFinal(char c)
{
	return ((c == NORMAL_KAF) || (c == NORMAL_MEM) || (c == NORMAL_NUN) || (c == NORMAL_PE));
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
 *    indication that the text is laid out "naturally" since the final letter
 *    really appears at the end. +1 for logical score.
 * 2) A word longer than 1 letter, ending with a Non-Final letter. In normal
 *    Hebrew, words ending with Kaf, Mem, Nun, Pe or Tsadi, should not end with
 *    the Non-Final form of that letter. Exceptions to this rule are mentioned
 *    above in isNonFinal(). This is an indication that the text is laid out
 *    backwards. +1 for visual score
 * 3) A word longer than 1 letter, starting with a final letter. Final letters
 *    should not appear at the beginning of a word. This is an indication that
 *    the text is laid out backwards. +1 for visual score.
 *
 * The visual score and logical score are accumulated throughout the text and
 * are finally checked against each other in GetCharSetName().
 * No checking for final letters in the middle of words is done since that case
 * is not an indication for either Logical or Visual text.
 *
 * The input buffer should not contain any white spaces that are not (' ')
 * or any low-ascii punctuation marks.
 */
nsProbingState nsHebrewProber::HandleData(const char* aBuf, uint32 aLen)
{
	// Both model probers say it's not them. No reason to continue.
	if(GetState() == eNotMe)
		return eNotMe;
	else {
		const char * endPtr = aBuf+aLen;
		for(const char * curPtr = (char *)aBuf; curPtr < endPtr; ++curPtr) {
			char cur = *curPtr;
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
/*      UDF OTH ASC ASS ACV ACO ASV ASO  */
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
	for(int i = 0; i < FREQ_CAT_NUM; i++)
		mFreqCounter[i] = 0;
}

nsProbingState nsLatin1Prober::HandleData(const char* aBuf, uint32 aLen)
{
	char * newBuf1 = 0;
	uint32 newLen1 = 0;
	if(!FilterWithEnglishLetters(aBuf, aLen, &newBuf1, newLen1)) {
		newBuf1 = (char*)aBuf;
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
		for(int32 i = 0; i < FREQ_CAT_NUM; i++)
			total += mFreqCounter[i];
		if(!total)
			confidence = 0.0f;
		else {
			confidence = mFreqCounter[3]*1.0f / total;
			confidence -= mFreqCounter[1]*20.0f/total;
		}
		if(confidence < 0.0f)
			confidence = 0.0f;
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

nsProbingState nsSJISProber::HandleData(const char* aBuf, uint32 aLen)
{
	nsSMState codingState;
	for(uint32 i = 0; i < aLen; i++) {
		codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			uint32 charLen = mCodingSM->GetCurrentCharLen();
			if(i == 0) {
				mLastChar[1] = aBuf[0];
				mContextAnalyser.HandleOneChar(mLastChar+2-charLen, charLen);
				mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
			}
			else{
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
	float contxtCf = mContextAnalyser.GetConfidence();
	float distribCf = mDistributionAnalyser.GetConfidence();
	return (contxtCf > distribCf ? contxtCf : distribCf);
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

nsProbingState nsSingleByteCharSetProber::HandleData(const char* aBuf, uint32 aLen)
{
	unsigned char order;
	for(uint32 i = 0; i < aLen; i++) {
		order = mModel->charToOrderMap[(uchar)aBuf[i]];
		if(order < SYMBOL_CAT_ORDER) {
			mTotalChar++;
		}
		else if(order == ILL) {
			/* When encountering an illegal codepoint, no need
			 * to continue analyzing data. */
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
			float cf = GetConfidence();
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
	for(uint32 i = 0; i < NUMBER_OF_SEQ_CAT; i++)
		mSeqCounters[i] = 0;
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
	return (float)0.01;
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
		if(r >= (float)1.00)
			r = (float)0.99;
		return r;
	}
	return (float)0.01;
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
//
//
void nsUTF8Prober::Reset()
{
	mCodingSM->Reset();
	mNumOfMBChar = 0;
	mState = eDetecting;
}

nsProbingState nsUTF8Prober::HandleData(const char* aBuf, uint32 aLen)
{
	nsSMState codingState;
	for(uint32 i = 0; i < aLen; i++) {
		codingState = mCodingSM->NextState(aBuf[i]);
		if(codingState == eItsMe) {
			mState = eFoundIt;
			break;
		}
		if(codingState == eStart) {
			if(mCodingSM->GetCurrentCharLen() >= 2)
				mNumOfMBChar++;
		}
	}
	if(mState == eDetecting)
		if(GetConfidence() > SHORTCUT_THRESHOLD)
			mState = eFoundIt;
	return mState;
}

#define ONE_CHAR_PROB   (float)0.50

float nsUTF8Prober::GetConfidence() const
{
	float unlike = (float)0.99;
	if(mNumOfMBChar < 6) {
		for(uint32 i = 0; i < mNumOfMBChar; i++)
			unlike *= ONE_CHAR_PROB;
		return (float)1.0 - unlike;
	}
	else
		return (float)0.99;
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
	for(uint32 i = 0; i < NUM_OF_PROBERS; i++) {
		delete mProbers[i];
	}
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

nsProbingState nsMBCSGroupProber::HandleData(const char* aBuf, uint32 aLen)
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
		case eFoundIt: return (float)0.99;
		case eNotMe:   return (float)0.01;
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
	uint32 i;
	float cf;
	GetConfidence();
	for(i = 0; i < NUM_OF_PROBERS; i++) {
		if(!mIsActive[i])
			printf("  MBCS inactive: [%s] (confidence is too low).\r\n", ProberName[i]);
		else{
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
nsCodingStateMachine::nsCodingStateMachine(const SMModel* sm) : mModel(sm) 
{
	mCurrentState = eStart;
}

nsSMState nsCodingStateMachine::NextState(char c)
{
	#define GETCLASS(c) GETFROMPCK(((uchar)(c)), mModel->classTable)
	//for each byte we get its class , if it is first byte, we also get byte length
	uint32 byteCls = GETCLASS(c);
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

uint32 nsCodingStateMachine::GetCurrentCharLen() const
{
	return mCurrentCharLen;
}

void nsCodingStateMachine::Reset() 
{
	mCurrentState = eStart;
}

const char * nsCodingStateMachine::GetCodingStateMachine() const
{
	return mModel->name;
}

nsEscCharSetProber::nsEscCharSetProber(uint32 aLanguageFilter)
{
	for(uint32 i = 0; i < NUM_OF_ESC_CHARSETS; i++)
		mCodingSM[i] = nsnull;
	if(aLanguageFilter & NS_FILTER_CHINESE_SIMPLIFIED) {
		mCodingSM[0] = new nsCodingStateMachine(&HZSMModel);
		mCodingSM[1] = new nsCodingStateMachine(&ISO2022CNSMModel);
	}
	if(aLanguageFilter & NS_FILTER_JAPANESE)
		mCodingSM[2] = new nsCodingStateMachine(&ISO2022JPSMModel);
	if(aLanguageFilter & NS_FILTER_KOREAN)
		mCodingSM[3] = new nsCodingStateMachine(&ISO2022KRSMModel);
	mActiveSM = NUM_OF_ESC_CHARSETS;
	mState = eDetecting;
	mDetectedCharset = nsnull;
}

nsEscCharSetProber::~nsEscCharSetProber()
{
	for(uint32 i = 0; i < NUM_OF_ESC_CHARSETS; i++)
		delete mCodingSM[i];
}

void nsEscCharSetProber::Reset()
{
	mState = eDetecting;
	for(uint32 i = 0; i < NUM_OF_ESC_CHARSETS; i++)
		if(mCodingSM[i])
			mCodingSM[i]->Reset();
	mActiveSM = NUM_OF_ESC_CHARSETS;
	mDetectedCharset = nsnull;
}

nsProbingState nsEscCharSetProber::HandleData(const char* aBuf, uint32 aLen)
{
	for(uint32 i = 0; i < aLen && mState == eDetecting; i++) {
		for(int32 j = mActiveSM-1; j>= 0; j--) {
			if(mCodingSM[j]) {
				nsSMState codingState = mCodingSM[j]->NextState(aBuf[i]);
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
