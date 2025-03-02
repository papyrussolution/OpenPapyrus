// EDLIB.CPP
//
#include <slib-internal.h>
#pragma hdrstop
#include "edlib.h"
//#include <stdint.h>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <cstring>
#include <string>

//using namespace std;

//typedef uint64_t Word_Removed;
static constexpr int    WORD_SIZE = sizeof(uint64) * 8; // Size of uint64 in bits
static constexpr uint64 WORD_1 = static_cast<uint64>(1);
static constexpr uint64 HIGH_BIT_MASK = WORD_1 << (WORD_SIZE - 1);  // 100..00
static constexpr int    MAX_UCHAR = 255;

struct EdLibBlock {
	EdLibBlock() : P(0), M(0), Score(0)
	{
	}
	EdLibBlock(uint64 p, uint64 m, int s) : P(p), M(m), Score(s) 
	{
	}
	EdLibBlock(const EdLibBlock & rS) : P(rS.P), M(rS.M), Score(rS.Score) 
	{
	}
	EdLibBlock & FASTCALL operator = (const EdLibBlock & rS)
	{
		P = rS.P;
		M = rS.M;
		Score = rS.Score;
		return *this;
	}
	/**
	 * Corresponds to Advance_Block function from Myers.
	 * Calculates one word(block), which is part of a column.
	 * Highest bit of word (one most to the left) is most bottom cell of block from column.
	 * Pv[i] and Mv[i] define vin of cell[i]: vin = cell[i] - cell[i-1].
	 * @param [in] Pv  Bitset, Pv[i] == 1 if vin is +1, otherwise Pv[i] == 0.
	 * @param [in] Mv  Bitset, Mv[i] == 1 if vin is -1, otherwise Mv[i] == 0.
	 * @param [in] Eq  Bitset, Eq[i] == 1 if match, 0 if mismatch.
	 * @param [in] hin  Will be +1, 0 or -1.
	 * @param [out] PvOut  Bitset, PvOut[i] == 1 if vout is +1, otherwise PvOut[i] == 0.
	 * @param [out] MvOut  Bitset, MvOut[i] == 1 if vout is -1, otherwise MvOut[i] == 0.
	 * @param [out] hout  Will be +1, 0 or -1.
	 */
	int CalculateBlock(/*uint64 Pv, uint64 Mv,*/uint64 Eq, const int hin/*, uint64 & PvOut, uint64 & MvOut*/) 
	{
		// hin can be 1, -1 or 0.
		// 1  -> 00...01
		// 0  -> 00...00
		// -1 -> 11...11 (2-complement)
		uint64 hin_is_neg = static_cast<uint64>(hin >> 2) & WORD_1; // 00...001 if hin is -1, 00...000 if 0 or 1
		uint64 xv = Eq | M;
		// This is instruction below written using 'if': if (hin < 0) Eq |= (uint64)1;
		Eq |= hin_is_neg;
		uint64 xh = (((Eq & P) + P) ^ P) | Eq;
		uint64 ph = M | ~(xh | P);
		uint64 mh = P & xh;
		int hout = 0;
		// This is instruction below written using 'if': if (Ph & HIGH_BIT_MASK) hout = 1;
		hout = (ph & HIGH_BIT_MASK) >> (WORD_SIZE - 1);
		// This is instruction below written using 'if': if (Mh & HIGH_BIT_MASK) hout = -1;
		hout -= (mh & HIGH_BIT_MASK) >> (WORD_SIZE - 1);
		ph <<= 1;
		mh <<= 1;
		// This is instruction below written using 'if': if (hin < 0) Mh |= (uint64)1;
		mh |= hin_is_neg;
		// This is instruction below written using 'if': if (hin > 0) Ph |= (uint64)1;
		ph |= static_cast<uint64>((hin + 1) >> 1);
		P = mh | ~(xv | ph);
		M = ph & xv;
		return hout;
	}
	bool AllBlockCellsLarger(const int k) const;
	std::vector <int> GetBlockCellValues() const;
	void ReadBlock(int * pDest) const;
	void ReadBlockReverse(int * pDest) const;
	uint64 P; // Pvin
	uint64 M; // Mvin
	int    Score; // score of last cell in block;
};
//
// Data needed to find alignment.
//
struct AlignmentData {
	AlignmentData(int maxNumBlocks, int targetLength) 
	{
		// We build a complete table and mark first and last block for each column
		// (because algorithm is banded so only part of each columns is used).
		// TODO: do not build a whole table, but just enough blocks for each column.
		//Ps     = new uint64[maxNumBlocks * targetLength];
		//Ms     = new uint64[maxNumBlocks * targetLength];
		//P_Scores = new  int[maxNumBlocks * targetLength];
		P_BlkList = new EdLibBlock[maxNumBlocks * targetLength];
		P_FirstBlocks = new int[targetLength];
		P_LastBlocks  = new int[targetLength];
	}
	~AlignmentData() 
	{
		//delete[] Ps;
		//delete[] Ms;
		//delete[] P_Scores;
		delete [] P_BlkList;
		delete[] P_FirstBlocks;
		delete[] P_LastBlocks;
	}
	int    ObtainAlignmentTraceback(const int queryLength, const int targetLength, const int bestScore, uchar ** const ppAlignment, int* const alignmentLength) const;
	EdLibBlock * P_BlkList; // @todo (in order to replace Ps, Ms, P_Scores) 
	//uint64 * Ps;
	//uint64 * Ms;
	//int * P_Scores;
	int * P_FirstBlocks;
	int * P_LastBlocks;
};
// 
// Defines equality relation on alphabet characters.
// By default each character is always equal only to itself, but you can also provide additional equalities.
// 
class EqualityDefinition {
private:
	bool   Matrix[MAX_UCHAR+1][MAX_UCHAR+1];
public:
	EqualityDefinition(const std::string & alphabet, const EdlibEqualityPair* additionalEqualities = NULL, const int additionalEqualitiesLength = 0) 
	{
		for(int i = 0; i < static_cast<int>(alphabet.size()); i++) {
			for(int j = 0; j < static_cast<int>(alphabet.size()); j++) {
				Matrix[i][j] = (i == j);
			}
		}
		if(additionalEqualities) {
			for(int i = 0; i < additionalEqualitiesLength; i++) {
				size_t first_transformed = alphabet.find(additionalEqualities[i].first);
				size_t second_transformed = alphabet.find(additionalEqualities[i].second);
				if(first_transformed != std::string::npos && second_transformed != std::string::npos) {
					Matrix[first_transformed][second_transformed] = Matrix[second_transformed][first_transformed] = true;
				}
			}
		}
	}
	//
	// @param a  Element from transformed sequence.
	// @param b  Element from transformed sequence.
	// @return True if a and b are defined as equal, false otherwise.
	// 
	bool   AreEqual(uchar a, uchar b) const { return Matrix[a][b]; }
	uint64 * BuildPeq(uint alphabetLength, const uchar * pQuery, int queryLength) const;
	int    ObtainAlignment(const uchar * query, const uchar * rQuery, int queryLength,
		const uchar * target, const uchar * rTarget, int targetLength,
		int alphabetLength, int bestScore, uchar ** alignment, int * alignmentLength) const;
};

static int MyersCalcEditDistanceSemiGlobal(const uint64* Peq, int W, int maxNumBlocks, int queryLength, 
	const uchar* target, int targetLength, int k, EdlibAlignMode mode, int* bestScore_, int** positions_, int* numPositions_);
static int MyersCalcEditDistanceNW(const uint64* Peq, int W, int maxNumBlocks, int queryLength, 
	const uchar* target, int targetLength, int k, int* bestScore_, int* position_, bool findAlignment, AlignmentData** alignData, int targetStopPosition);
//static int ObtainAlignment(const uchar* query, const uchar* rQuery, int queryLength, const uchar* target, const uchar* rTarget, int targetLength,
    //const EqualityDefinition& equalityDefinition, int alphabetLength, int bestScore, uchar** alignment, int* alignmentLength);
// 
// Does ceiling division x / y.
// Note: x and y must be non-negative and x + y must not overflow.
// 
// @sobolev (replaced with idivroundup) static inline int ceilDiv(const int x, const int y) { return (x % y) ? (x / y + 1) : (x / y); }
// 
// Build Peq table for given query and alphabet.
// Peq is table of dimensions alphabetLength+1 x maxNumBlocks.
// Bit i of Peq[s * maxNumBlocks + b] is 1 if i-th symbol from block b of query equals symbol s, otherwise it is 0.
// NOTICE: free returned array with delete[]!
// 
//static uint64 * BuildPeq(uint alphabetLength, const uchar * query, int queryLength, const EqualityDefinition & equalityDefinition) 
uint64 * EqualityDefinition::BuildPeq(uint alphabetLength, const uchar * query, int queryLength) const
{
	int maxNumBlocks = idivroundup(queryLength, WORD_SIZE);
	// table of dimensions alphabetLength+1 x maxNumBlocks. Last symbol is wildcard.
	uint64 * Peq = new uint64[(alphabetLength + 1) * maxNumBlocks];
	// Build Peq (1 is match, 0 is mismatch). NOTE: last column is wildcard(symbol that matches anything) with just 1s
	for(uint symbol = 0; symbol <= alphabetLength; symbol++) {
		for(int b = 0; b < maxNumBlocks; b++) {
			if(symbol < alphabetLength) {
				Peq[symbol * maxNumBlocks + b] = 0;
				for(int r = (b+1) * WORD_SIZE - 1; r >= b * WORD_SIZE; r--) {
					Peq[symbol * maxNumBlocks + b] <<= 1;
					// NOTE: We pretend like query is padded at the end with W wildcard symbols
					if(r >= queryLength || AreEqual(query[r], symbol))
						Peq[symbol * maxNumBlocks + b] += 1;
				}
			}
			else { // Last symbol is wildcard, so it is all 1s
				Peq[symbol * maxNumBlocks + b] = _FFFF64;
			}
		}
	}
	return Peq;
}
// 
// Returns new sequence that is reverse of given sequence.
// Free returned array with delete[].
// 
static uchar * CreateReverseCopy(const uchar * pSeq, uint length) 
{
	uchar * p_rseq = new uchar[length];
	for(uint i = 0; i < length; i++) {
		p_rseq[i] = pSeq[length-i-1];
	}
	return p_rseq;
}
// 
// Takes char query and char target, recognizes alphabet and transforms them into uchar sequences
// where elements in sequences are not any more letters of alphabet, but their index in alphabet.
// Most of internal edlib functions expect such transformed sequences.
// This function will allocate queryTransformed and targetTransformed, so make sure to free them when done.
// Example:
//   Original sequences: "ACT" and "CGT".
//   Alphabet would be recognized as "ACTG". Alphabet length = 4.
//   Transformed sequences: [0, 1, 2] and [1, 3, 2].
// @param [in] queryOriginal
// @param [in] queryLength
// @param [in] targetOriginal
// @param [in] targetLength
// @param [out] queryTransformed  It will contain values in range [0, alphabet length - 1].
// @param [out] targetTransformed  It will contain values in range [0, alphabet length - 1].
// @return  Alphabet as a string of unique characters, where index of each character is its value in transformed sequences.
// 
static std::string TransformSequences(const char * pQueryOriginal, uint queryLength,
    const char * pTargetOriginal, uint targetLength, uchar ** ppQueryTransformed, uchar ** ppTargetTransformed) 
{
	// Alphabet is constructed from letters that are present in sequences.
	// Each letter is assigned an ordinal number, starting from 0 up to alphabetLength - 1,
	// and new query and target are created in which letters are replaced with their ordinal numbers.
	// This query and target are used in all the calculations later.
	*ppQueryTransformed = static_cast<uchar *>(SAlloc::M(sizeof(uchar) * queryLength));
	*ppTargetTransformed = static_cast<uchar *>(SAlloc::M(sizeof(uchar) * targetLength));
	std::string alphabet = "";
	// Alphabet information, it is constructed on fly while transforming sequences.
	// letterIdx[c] is index of letter c in alphabet.
	uchar letterIdx[MAX_UCHAR + 1];
	bool inAlphabet[MAX_UCHAR + 1]; // inAlphabet[c] is true if c is in alphabet
	memzero(inAlphabet, sizeof(inAlphabet));
	//for(int i = 0; i < MAX_UCHAR + 1; i++) 
		//inAlphabet[i] = false;
	for(uint i = 0; i < queryLength; i++) {
		uchar c = static_cast<uchar>(pQueryOriginal[i]);
		if(!inAlphabet[c]) {
			inAlphabet[c] = true;
			letterIdx[c] = static_cast<uchar>(alphabet.size());
			alphabet += pQueryOriginal[i];
		}
		(*ppQueryTransformed)[i] = letterIdx[c];
	}
	for(uint i = 0; i < targetLength; i++) {
		const uchar c = static_cast<uchar>(pTargetOriginal[i]);
		if(!inAlphabet[c]) {
			inAlphabet[c] = true;
			letterIdx[c] = static_cast<uchar>(alphabet.size());
			alphabet += pTargetOriginal[i];
		}
		(*ppTargetTransformed)[i] = letterIdx[c];
	}
	return alphabet;
}
// 
// Main edlib method.
// 
EdlibAlignResult EdlibAlign(const char * pQueryOriginal, uint queryLength, const char * pTargetOriginal, uint targetLength, const EdlibAlignConfig config) 
{
	EdlibAlignResult result;
	result.status = EDLIB_STATUS_OK;
	result.editDistance = -1;
	result.P_EndLocations = result.P_StartLocations = NULL;
	result.numLocations = 0;
	result.P_Alignment = NULL;
	result.alignmentLength = 0;
	result.alphabetLength = 0;
	/*------------ TRANSFORM SEQUENCES AND RECOGNIZE ALPHABET -----------*/
	uchar * query;
	uchar * target;
	std::string alphabet = TransformSequences(pQueryOriginal, queryLength, pTargetOriginal, targetLength, &query, &target);
	result.alphabetLength = static_cast<int>(alphabet.size());
	// Handle special situation when at least one of the sequences has length 0.
	if(queryLength == 0 || targetLength == 0) {
		if(config.Mode == EDLIB_MODE_NW) {
			result.editDistance = smax(queryLength, targetLength);
			result.P_EndLocations = static_cast<int *>(SAlloc::M(sizeof(int) * 1));
			result.P_EndLocations[0] = targetLength - 1;
			result.numLocations = 1;
		}
		else if(oneof2(config.Mode, EDLIB_MODE_SHW, EDLIB_MODE_HW)) {
			result.editDistance = queryLength;
			result.P_EndLocations = static_cast<int *>(SAlloc::M(sizeof(int) * 1));
			result.P_EndLocations[0] = -1;
			result.numLocations = 1;
		}
		else {
			result.status = EDLIB_STATUS_ERROR;
		}
		SAlloc::F(query);
		SAlloc::F(target);
		return result;
	}
	/*--------------------- INITIALIZATION ------------------*/
	int maxNumBlocks = idivroundup(static_cast<int>(queryLength), WORD_SIZE); // bmax in Myers
	int W = maxNumBlocks * WORD_SIZE - queryLength; // number of redundant cells in last level blocks
	EqualityDefinition equalityDefinition(alphabet, config.additionalEqualities, config.additionalEqualitiesLength);
	uint64 * Peq = equalityDefinition.BuildPeq(alphabet.size(), query, queryLength);
	/*------------------ MAIN CALCULATION -------------------*/
	// TODO: Store alignment data only after k is determined? That could make things faster.
	int positionNW; // Used only when mode is NW.
	AlignmentData * alignData = NULL;
	bool dynamicK = false;
	int k = config.K;
	if(k < 0) { // If valid k is not given, auto-adjust k until solution is found.
		dynamicK = true;
		k = WORD_SIZE; // Gives better results than smaller k.
	}
	do {
		if(config.Mode == EDLIB_MODE_HW || config.Mode == EDLIB_MODE_SHW) {
			MyersCalcEditDistanceSemiGlobal(Peq, W, maxNumBlocks, queryLength, target, targetLength, k, config.Mode, &(result.editDistance), &(result.P_EndLocations), &(result.numLocations));
		}
		else { // mode == EDLIB_MODE_NW
			MyersCalcEditDistanceNW(Peq, W, maxNumBlocks, queryLength, target, targetLength, k, &(result.editDistance), &positionNW, false, &alignData, -1);
		}
		k *= 2;
	} while(dynamicK && result.editDistance == -1);
	if(result.editDistance >= 0) { // If there is solution.
		// If NW mode, set end location explicitly.
		if(config.Mode == EDLIB_MODE_NW) {
			result.P_EndLocations = static_cast<int *>(SAlloc::M(sizeof(int) * 1));
			result.P_EndLocations[0] = targetLength - 1;
			result.numLocations = 1;
		}
		// Find starting locations.
		if(oneof2(config.Task, EDLIB_TASK_LOC, EDLIB_TASK_PATH)) {
			result.P_StartLocations = static_cast<int *>(SAlloc::M(result.numLocations * sizeof(int)));
			if(config.Mode == EDLIB_MODE_HW) { // If HW, I need to calculate start locations.
				const uchar * rTarget = CreateReverseCopy(target, targetLength);
				const uchar * rQuery  = CreateReverseCopy(query, queryLength);
				// Peq for reversed query.
				uint64 * rPeq = equalityDefinition.BuildPeq(alphabet.size(), rQuery, queryLength);
				for(int i = 0; i < result.numLocations; i++) {
					int endLocation = result.P_EndLocations[i];
					if(endLocation == -1) {
						// NOTE: Sometimes one of optimal solutions is that query starts before
						// target, like this:
						//                       AAGG <- target
						//                   CCTT     <- query
						//   It will never be only optimal solution and it does not happen
						// often, however it is
						//   possible and in that case end location will be -1. What should we
						// do with that?
						//   Should we just skip reporting such end location, although it is a
						// solution?
						//   If we do report it, what is the start location? -4? -1? Nothing?
						// TODO: Figure this out. This has to do in general with how we think
						// about start
						//   and end locations.
						//   Also, we have alignment later relying on this locations to limit
						// the space of it's
						//   search -> how can it do it right if these locations are negative or
						// incorrect?
						result.P_StartLocations[i] = 0; // I put 0 for now, but it does not make much sense.
					}
					else {
						int bestScoreSHW;
						int numPositionsSHW;
						int * positionsSHW = 0;
						MyersCalcEditDistanceSemiGlobal(rPeq, W, maxNumBlocks,
							queryLength, rTarget + targetLength - endLocation - 1, endLocation + 1,
							result.editDistance, EDLIB_MODE_SHW, &bestScoreSHW, &positionsSHW, &numPositionsSHW);
						// Taking last location as start ensures that alignment will not start
						// with insertions
						// if it can start with mismatches instead.
						result.P_StartLocations[i] = endLocation - positionsSHW[numPositionsSHW - 1];
						SAlloc::F(positionsSHW);
					}
				}
				delete[] rTarget;
				delete[] rQuery;
				delete[] rPeq;
			}
			else { // If mode is SHW or NW
				for(int i = 0; i < result.numLocations; i++) {
					result.P_StartLocations[i] = 0;
				}
			}
		}
		// Find alignment -> all comes down to finding alignment for NW.
		// Currently we return alignment only for first pair of locations.
		if(config.Task == EDLIB_TASK_PATH) {
			int alnStartLocation = result.P_StartLocations[0];
			int alnEndLocation = result.P_EndLocations[0];
			const uchar * alnTarget = target + alnStartLocation;
			const int alnTargetLength = alnEndLocation - alnStartLocation + 1;
			const uchar * rAlnTarget = CreateReverseCopy(alnTarget, alnTargetLength);
			const uchar * rQuery  = CreateReverseCopy(query, queryLength);
			equalityDefinition.ObtainAlignment(query, rQuery, queryLength, alnTarget, rAlnTarget, alnTargetLength,
			    static_cast<int>(alphabet.size()), result.editDistance, &(result.P_Alignment), &(result.alignmentLength));
			delete[] rAlnTarget;
			delete[] rQuery;
		}
	}
	delete[] Peq;
	SAlloc::F(query);
	SAlloc::F(target);
	delete alignData;
	return result;
}

char* edlibAlignmentToCigar(const uchar * const alignment, const int alignmentLength, const EdlibCigarFormat cigarFormat) 
{
	if(cigarFormat != EDLIB_CIGAR_EXTENDED && cigarFormat != EDLIB_CIGAR_STANDARD) {
		return 0;
	}
	// Maps move code from alignment to char in cigar.
	//                        0    1    2    3
	char moveCodeToChar[] = {'=', 'I', 'D', 'X'};
	if(cigarFormat == EDLIB_CIGAR_STANDARD) {
		moveCodeToChar[0] = moveCodeToChar[3] = 'M';
	}
	std::vector <char>* cigar = new std::vector <char>();
	char lastMove = 0; // Char of last move. 0 if there was no previous move.
	int numOfSameMoves = 0;
	for(int i = 0; i <= alignmentLength; i++) {
		// if new sequence of same moves started
		if(i == alignmentLength || (moveCodeToChar[alignment[i]] != lastMove && lastMove != 0)) {
			// Write number of moves to cigar string.
			int numDigits = 0;
			for(; numOfSameMoves; numOfSameMoves /= 10) {
				cigar->push_back('0' + numOfSameMoves % 10);
				numDigits++;
			}
			reverse(cigar->end() - numDigits, cigar->end());
			// Write code of move to cigar string.
			cigar->push_back(lastMove);
			// If not at the end, start new sequence of moves.
			if(i < alignmentLength) {
				// Check if alignment has valid values.
				if(alignment[i] > 3) {
					delete cigar;
					return 0;
				}
				numOfSameMoves = 0;
			}
		}
		if(i < alignmentLength) {
			lastMove = moveCodeToChar[alignment[i]];
			numOfSameMoves++;
		}
	}
	cigar->push_back(0); // Null character termination.
	char* cigar_ = static_cast<char *>(SAlloc::M(cigar->size() * sizeof(char)));
	memcpy(cigar_, &(*cigar)[0], cigar->size() * sizeof(char));
	delete cigar;
	return cigar_;
}

//static inline int min(const int x, const int y) { return x < y ? x : y; }
//static inline int max(const int x, const int y) { return x > y ? x : y; }
// 
// @param [in] block
// @return Values of cells in block, starting with bottom cell in block.
// 
std::vector <int> EdLibBlock::GetBlockCellValues() const
{
	std::vector <int> scores(WORD_SIZE);
	int    score = Score;
	const auto _p = P;
	const auto _m = M;
	uint64 mask = HIGH_BIT_MASK;
	for(int i = 0; i < (WORD_SIZE-1); i++) {
		scores[i] = score;
		if(_p & mask) 
			score--;
		if(_m & mask) 
			score++;
		mask >>= 1;
	}
	scores[WORD_SIZE-1] = score;
	return scores;
}
/**
 * Writes values of cells in block into given array, starting with first/top cell.
 * @param [in] block
 * @param [out] dest  Array into which cell values are written. Must have size of at least WORD_SIZE.
 */
void EdLibBlock::ReadBlock(int * pDest) const
{
	int score = Score;
	uint64 mask = HIGH_BIT_MASK;
	for(int i = 0; i < WORD_SIZE-1; i++) {
		pDest[WORD_SIZE-1-i] = score;
		if(P & mask) 
			score--;
		if(M & mask) 
			score++;
		mask >>= 1;
	}
	pDest[0] = score;
}
/**
 * Writes values of cells in block into given array, starting with last/bottom cell.
 * @param [in] block
 * @param [out] dest  Array into which cell values are written. Must have size of at least WORD_SIZE.
 */
void EdLibBlock::ReadBlockReverse(int * pDest) const
{
	int  score = Score;
	uint64 mask = HIGH_BIT_MASK;
	for(int i = 0; i < WORD_SIZE - 1; i++) {
		pDest[i] = score;
		if(P & mask) 
			score--;
		if(M & mask) 
			score++;
		mask >>= 1;
	}
	pDest[WORD_SIZE-1] = score;
}
/**
 * @param [in] block
 * @param [in] k
 * @return True if all cells in block have value larger than k, otherwise false.
 */
//static bool AllBlockCellsLarger(const EdLibBlock & rBlock, const int k) 
bool EdLibBlock::AllBlockCellsLarger(const int k) const
{
	std::vector <int> scores = GetBlockCellValues();
	for(int i = 0; i < WORD_SIZE; i++) {
		if(scores[i] <= k) 
			return false;
	}
	return true;
}

/**
 * Uses Myers' bit-vector algorithm to find edit distance for one of semi-global alignment methods.
 * @param [in] Peq  Query profile.
 * @param [in] W  Size of padding in last block.
 *                TODO: Calculate this directly from query, instead of passing it.
 * @param [in] maxNumBlocks  Number of blocks needed to cover the whole query.
 *                           TODO: Calculate this directly from query, instead of passing it.
 * @param [in] queryLength
 * @param [in] target
 * @param [in] targetLength
 * @param [in] k
 * @param [in] mode  EDLIB_MODE_HW or EDLIB_MODE_SHW
 * @param [out] bestScore_  Edit distance.
 * @param [out] positions_  Array of 0-indexed positions in target at which best score was found.
                            Make sure to free this array with free().
 * @param [out] numPositions_  Number of positions in the positions_ array.
 * @return Status.
 */
static int MyersCalcEditDistanceSemiGlobal(const uint64* const Peq, const int W, const int maxNumBlocks, const int queryLength,
    const uchar * const target, const int targetLength, int k, const EdlibAlignMode mode, int * const bestScore_, int ** const positions_, int* const numPositions_) 
{
	*positions_ = NULL;
	*numPositions_ = 0;
	// firstBlock is 0-based index of first block in Ukkonen band.
	// lastBlock is 0-based index of last block in Ukkonen band.
	int firstBlock = 0;
	int lastBlock = smin(idivroundup(k + 1, WORD_SIZE), maxNumBlocks) - 1; // y in Myers
	EdLibBlock * bl; // Current block
	EdLibBlock * blocks = new EdLibBlock[maxNumBlocks];
	// For HW, solution will never be larger then queryLength.
	if(mode == EDLIB_MODE_HW) {
		k = smin(queryLength, k);
	}
	// Each STRONG_REDUCE_NUM column is reduced in more expensive way.
	// This gives speed up of about 2 times for small k.
	const int STRONG_REDUCE_NUM = 2048;
	// Initialize P, M and score
	bl = blocks;
	for(int b = 0; b <= lastBlock; b++) {
		bl->P = _FFFF64; // All 1s
		bl->M = 0ULL;
		bl->Score = (b + 1) * WORD_SIZE;
		bl++;
	}
	int bestScore = -1;
	std::vector <int> positions; // TODO: Maybe put this on heap?
	const int startHout = mode == EDLIB_MODE_HW ? 0 : 1; // If 0 then gap before query is not penalized;
	const uchar * targetChar = target;
	for(int c = 0; c < targetLength; c++) { // for each column
		const uint64 * Peq_c = Peq + (*targetChar) * maxNumBlocks;
		//----------------------- Calculate column -------------------------//
		int hout = startHout;
		bl = blocks + firstBlock;
		Peq_c += firstBlock;
		for(int b = firstBlock; b <= lastBlock; b++) {
			hout = bl->CalculateBlock(*Peq_c, hout);
			bl->Score += hout;
			bl++; Peq_c++;
		}
		bl--; 
		Peq_c--;
		//------------------------------------------------------------------//
		//---------- Adjust number of blocks according to Ukkonen ----------//
		if((lastBlock < maxNumBlocks - 1) && (bl->Score - hout <= k) // bl is pointing to last block
		    && ((*(Peq_c + 1) & WORD_1) || hout < 0)) { // Peq_c is pointing to last block
			// If score of left block is not too big, calculate one more block
			lastBlock++; 
			bl++; 
			Peq_c++;
			bl->P = _FFFF64; // All 1s
			bl->M = static_cast<uint64>(0);
			bl->Score = (bl - 1)->Score - hout + WORD_SIZE + bl->CalculateBlock(*Peq_c, hout);
		}
		else {
			while(lastBlock >= firstBlock && bl->Score >= k + WORD_SIZE) {
				lastBlock--; 
				bl--; 
				Peq_c--;
			}
		}
		// Every some columns, do some expensive but also more efficient block reducing.
		// This is important!
		//
		// Reduce the band by decreasing last block if possible.
		if(c % STRONG_REDUCE_NUM == 0) {
			while(lastBlock >= 0 && lastBlock >= firstBlock && bl->AllBlockCellsLarger(k)) {
				lastBlock--; 
				bl--; 
				Peq_c--;
			}
		}
		// For HW, even if all cells are > k, there still may be solution in next
		// column because starting conditions at upper boundary are 0.
		// That means that first block is always candidate for solution,
		// and we can never end calculation before last column.
		if(mode == EDLIB_MODE_HW && lastBlock == -1) {
			lastBlock++; 
			bl++; 
			Peq_c++;
		}
		// Reduce band by increasing first block if possible. Not applicable to HW.
		if(mode != EDLIB_MODE_HW) {
			while(firstBlock <= lastBlock && blocks[firstBlock].Score >= k + WORD_SIZE) {
				firstBlock++;
			}
			if(c % STRONG_REDUCE_NUM == 0) { // Do strong reduction every some blocks
				while(firstBlock <= lastBlock && blocks[firstBlock].AllBlockCellsLarger(k)) {
					firstBlock++;
				}
			}
		}
		// If band stops to exist finish
		if(lastBlock < firstBlock) {
			*bestScore_ = bestScore;
			if(bestScore != -1) {
				*positions_ = static_cast<int *>(SAlloc::M(sizeof(int) * static_cast<int>(positions.size())));
				*numPositions_ = static_cast<int>(positions.size());
				copy(positions.begin(), positions.end(), *positions_);
			}
			delete[] blocks;
			return EDLIB_STATUS_OK;
		}
		//------------------------- Update best score ----------------------//
		if(lastBlock == maxNumBlocks - 1) {
			int colScore = bl->Score;
			if(colScore <= k) { // Scores > k dont have correct values (so we cannot use them), but are certainly > k.
				// NOTE: Score that I find in column c is actually score from column c-W
				if(bestScore == -1 || colScore <= bestScore) {
					if(colScore != bestScore) {
						positions.clear();
						bestScore = colScore;
						// Change k so we will look only for equal or better
						// scores then the best found so far.
						k = bestScore;
					}
					positions.push_back(c - W);
				}
			}
		}
		targetChar++;
	}
	// Obtain results for last W columns from last column.
	if(lastBlock == maxNumBlocks - 1) {
		std::vector <int> blockScores = bl->GetBlockCellValues();
		for(int i = 0; i < W; i++) {
			const int colScore = blockScores[i + 1];
			if(colScore <= k && (bestScore == -1 || colScore <= bestScore)) {
				if(colScore != bestScore) {
					positions.clear();
					k = bestScore = colScore;
				}
				positions.push_back(targetLength - W + i);
			}
		}
	}
	*bestScore_ = bestScore;
	if(bestScore != -1) {
		*positions_ = static_cast<int *>(SAlloc::M(sizeof(int) * static_cast<int>(positions.size())));
		*numPositions_ = static_cast<int>(positions.size());
		copy(positions.begin(), positions.end(), *positions_);
	}
	delete [] blocks;
	return EDLIB_STATUS_OK;
}
/**
 * Uses Myers' bit-vector algorithm to find edit distance for global(NW) alignment method.
 * @param [in] Peq  Query profile.
 * @param [in] W  Size of padding in last block.
 *                TODO: Calculate this directly from query, instead of passing it.
 * @param [in] maxNumBlocks  Number of blocks needed to cover the whole query.
 *                           TODO: Calculate this directly from query, instead of passing it.
 * @param [in] queryLength
 * @param [in] target
 * @param [in] targetLength
 * @param [in] k
 * @param [out] bestScore_  Edit distance.
 * @param [out] position_  0-indexed position in target at which best score was found.
 * @param [in] findAlignment  If true, whole matrix is remembered and alignment data is returned.
 *                            Quadratic amount of memory is consumed.
 * @param [out] alignData  Data needed for alignment traceback (for reconstruction of alignment).
 *                         Set only if findAlignment is set to true, otherwise it is NULL.
 *                         Make sure to free this array using delete[].
 * @param [out] targetStopPosition  If set to -1, whole calculation is performed normally, as expected.
 *         If set to p, calculation is performed up to position p in target (inclusive)
 *         and column p is returned as the only column in alignData.
 * @return Status.
 */
static int MyersCalcEditDistanceNW(const uint64* const Peq, const int W, const int maxNumBlocks, const int queryLength,
    const uchar * const target, const int targetLength, int k, int * pBestScore, int * pPosition, const bool findAlignment,
    AlignmentData ** const ppAlignData, const int targetStopPosition) 
{
	if(targetStopPosition > -1 && findAlignment) {
		// They can not be both set at the same time!
		return EDLIB_STATUS_ERROR;
	}
	// Each STRONG_REDUCE_NUM column is reduced in more expensive way.
	const int STRONG_REDUCE_NUM = 2048; // TODO: Choose this number dinamically (based on query and target lengths?), so it does not affect speed of computation
	if(k < abs(targetLength - queryLength)) {
		*pBestScore = *pPosition = -1;
		return EDLIB_STATUS_OK;
	}
	k = smin(k, smax(queryLength, targetLength)); // Upper bound for k
	// firstBlock is 0-based index of first block in Ukkonen band.
	// lastBlock is 0-based index of last block in Ukkonen band.
	int firstBlock = 0;
	// This is optimal now, by my formula.
	int lastBlock = smin(maxNumBlocks, idivroundup(smin(k, (k + queryLength - targetLength) / 2) + 1, WORD_SIZE)) - 1;
	EdLibBlock * bl; // Current block
	EdLibBlock * p_blocks = new EdLibBlock[maxNumBlocks];
	// Initialize P, M and score
	bl = p_blocks;
	for(int b = 0; b <= lastBlock; b++) {
		bl->P = _FFFF64; // All 1s
		bl->M = 0ULL;
		bl->Score = (b + 1) * WORD_SIZE;
		bl++;
	}
	// If we want to find alignment, we have to store needed data.
	if(findAlignment)
		*ppAlignData = new AlignmentData(maxNumBlocks, targetLength);
	else if(targetStopPosition > -1)
		*ppAlignData = new AlignmentData(maxNumBlocks, 1);
	else
		*ppAlignData = NULL;
	const uchar * targetChar = target;
	for(int c = 0; c < targetLength; c++) { // for each column
		const uint64 * Peq_c = Peq + *targetChar * maxNumBlocks;
		//----------------------- Calculate column -------------------------//
		int hout = 1;
		bl = p_blocks + firstBlock;
		for(int b = firstBlock; b <= lastBlock; b++) {
			hout = bl->CalculateBlock(Peq_c[b], hout);
			bl->Score += hout;
			bl++;
		}
		bl--;
		//------------------------------------------------------------------//
		// bl now points to last block

		// Update k. I do it only on end of column because it would slow calculation too much otherwise.
		// NOTICE: I add W when in last block because it is actually result from W cells to the left and W cells
		// up.
		k = smin(k, bl->Score + smax(targetLength - c - 1, queryLength - ((1 + lastBlock) * WORD_SIZE - 1) - 1) + (lastBlock == maxNumBlocks - 1 ? W : 0));
		//---------- Adjust number of blocks according to Ukkonen ----------//
		//--- Adjust last block ---//
		// If block is not beneath band, calculate next block. Only next because others are certainly beneath
		// band.
		if(lastBlock + 1 < maxNumBlocks && 
			!(//score[lastBlock] >= k + WORD_SIZE ||  // NOTICE: this condition could be satisfied if above
		         // block also!
			    ((lastBlock + 1) * WORD_SIZE - 1 > k - bl->Score + 2 * WORD_SIZE - 2 - targetLength + c + queryLength))) {
			lastBlock++; 
			bl++;
			bl->P = _FFFF64; // All 1s
			bl->M = 0ULL;
			int newHout = bl->CalculateBlock(Peq_c[lastBlock], hout);
			bl->Score = (bl - 1)->Score - hout + WORD_SIZE + newHout;
			hout = newHout;
		}
		// While block is out of band, move one block up.
		// NOTE: Condition used here is more loose than the one from the article, since I simplified the max()
		// part of it.
		// I could consider adding that max part, for optimal performance.
		while(lastBlock >= firstBlock && (bl->Score >= k + WORD_SIZE || ((lastBlock + 1) * WORD_SIZE - 1 > k - bl->Score + 2 * WORD_SIZE - 2 - targetLength + c + queryLength + 1))) { 
			// TODO: Does not work if do not put +1! Why???
			lastBlock--; 
			bl--;
		}
		//--- Adjust first block ---//
		// While outside of band, advance block
		while(firstBlock <= lastBlock && (p_blocks[firstBlock].Score >= k + WORD_SIZE || ((firstBlock + 1) * WORD_SIZE - 1 < p_blocks[firstBlock].Score - k - targetLength + queryLength + c))) {
			firstBlock++;
		}
		// TODO: consider if this part is useful, it does not seem to help much
		if(c % STRONG_REDUCE_NUM == 0) { // Every some columns do more expensive but more efficient reduction
			while(lastBlock >= firstBlock) {
				// If all cells outside of band, remove block
				std::vector <int> scores = bl->GetBlockCellValues();
				int numCells = lastBlock == maxNumBlocks - 1 ? WORD_SIZE - W : WORD_SIZE;
				int r = lastBlock * WORD_SIZE + numCells - 1;
				bool reduce = true;
				for(int i = WORD_SIZE - numCells; i < WORD_SIZE; i++) {
					// TODO: Does not work if do not put +1! Why???
					if(scores[i] <= k && r <= k - scores[i] - targetLength + c + queryLength + 1) {
						reduce = false;
						break;
					}
					r--;
				}
				if(!reduce) 
					break;
				lastBlock--; 
				bl--;
			}
			while(firstBlock <= lastBlock) {
				// If all cells outside of band, remove block
				std::vector <int> scores = p_blocks[firstBlock].GetBlockCellValues();
				int numCells = firstBlock == maxNumBlocks - 1 ? WORD_SIZE - W : WORD_SIZE;
				int r = firstBlock * WORD_SIZE + numCells - 1;
				bool reduce = true;
				for(int i = WORD_SIZE - numCells; i < WORD_SIZE; i++) {
					if(scores[i] <= k && r >= scores[i] - k - targetLength + c + queryLength) {
						reduce = false;
						break;
					}
					r--;
				}
				if(!reduce) 
					break;
				firstBlock++;
			}
		}
		// If band stops to exist finish
		if(lastBlock < firstBlock) {
			*pBestScore = *pPosition = -1;
			delete[] p_blocks;
			return EDLIB_STATUS_OK;
		}
		// Save column so it can be used for reconstruction
		if(findAlignment && c < targetLength) {
			bl = p_blocks + firstBlock;
			for(int b = firstBlock; b <= lastBlock; b++) {
				//(*ppAlignData)->Ps[maxNumBlocks * c + b] = bl->P;
				//(*ppAlignData)->Ms[maxNumBlocks * c + b] = bl->M;
				//(*ppAlignData)->P_Scores[maxNumBlocks * c + b] = bl->Score;
				(*ppAlignData)->P_BlkList[maxNumBlocks * c + b] = *bl;
				bl++;
			}
			(*ppAlignData)->P_FirstBlocks[c] = firstBlock;
			(*ppAlignData)->P_LastBlocks[c] = lastBlock;
		}
		//
		// If this is stop column, save it and finish
		//
		if(c == targetStopPosition) {
			for(int b = firstBlock; b <= lastBlock; b++) {
				//(*ppAlignData)->Ps[b] = (p_blocks + b)->P;
				//(*ppAlignData)->Ms[b] = (p_blocks + b)->M;
				//(*ppAlignData)->P_Scores[b] = (p_blocks + b)->Score;
				(*ppAlignData)->P_BlkList[b] = p_blocks[b];
			}
			(*ppAlignData)->P_FirstBlocks[0] = firstBlock;
			(*ppAlignData)->P_LastBlocks[0] = lastBlock;
			*pBestScore = -1;
			*pPosition = targetStopPosition;
			delete[] p_blocks;
			return EDLIB_STATUS_OK;
		}
		targetChar++;
	}
	if(lastBlock == maxNumBlocks - 1) { // If last block of last column was calculated
		// Obtain best score from block -> it is complicated because query is padded with W cells
		int best_score = p_blocks[lastBlock].GetBlockCellValues()[W];
		if(best_score <= k) {
			*pBestScore = best_score;
			*pPosition = targetLength - 1;
			delete[] p_blocks;
			return EDLIB_STATUS_OK;
		}
	}
	*pBestScore = *pPosition = -1;
	delete[] p_blocks;
	return EDLIB_STATUS_OK;
}
/**
 * Finds one possible alignment that gives optimal score by moving back through the dynamic programming matrix,
 * that is stored in alignData. Consumes large amount of memory: O(queryLength * targetLength).
 * @param [in] queryLength  Normal length, without W.
 * @param [in] targetLength  Normal length, without W.
 * @param [in] bestScore  Best score.
 * @param [in] alignData  Data obtained during finding best score that is useful for finding alignment.
 * @param [out] alignment  Alignment.
 * @param [out] alignmentLength  Length of alignment.
 * @return Status code.
 */
int AlignmentData::ObtainAlignmentTraceback(const int queryLength, const int targetLength, const int bestScore,
    uchar ** const ppAlignment, int* const alignmentLength) const
{
	const int max_num_blocks = idivroundup(queryLength, WORD_SIZE);
	const int W = max_num_blocks * WORD_SIZE - queryLength;
	*ppAlignment = static_cast<uchar*>(SAlloc::M((queryLength + targetLength - 1) * sizeof(uchar)));
	*alignmentLength = 0;
	int c = targetLength - 1; // index of column
	int b = max_num_blocks - 1; // index of block in column
	int curr_score = bestScore; // Score of current cell
	int l_score  = -1;// Score of left cell
	int u_score  = -1;// Score of upper cell
	int ul_score = -1; // Score of upper left cell
	uint64 currP = P_BlkList[c * max_num_blocks + b].P; // P of current block
	uint64 currM = P_BlkList[c * max_num_blocks + b].M; // M of current block
	// True if block to left exists and is in band
	bool thereIsLeftBlock = c > 0 && b >= P_FirstBlocks[c-1] && b <= P_LastBlocks[c-1];
	// We set initial values of lP and lM to 0 only to avoid compiler warnings, they should not affect the
	// calculation as both lP and lM should be initialized at some moment later (but compiler can not
	// detect it since this initialization is guaranteed by "business" logic).
	uint64 lP = 0, lM = 0;
	if(thereIsLeftBlock) {
		lP = P_BlkList[(c - 1) * max_num_blocks + b].P; // P of block to the left
		lM = P_BlkList[(c - 1) * max_num_blocks + b].M; // M of block to the left
	}
	currP <<= W;
	currM <<= W;
	int block_pos = WORD_SIZE - W - 1; // 0 based index of current cell in blockPos
	// TODO(martin): refactor this whole piece of code. There are too many if-else statements,
	// it is too easy for a bug to hide and to hard to effectively cover all the edge-cases.
	// We need better separation of logic and responsibilities.
	while(true) {
		if(c == 0) {
			thereIsLeftBlock = true;
			l_score = b * WORD_SIZE + block_pos + 1;
			ul_score = l_score - 1;
		}
		// TODO: improvement: calculate only those cells that are needed,
		//       for example if I calculate upper cell and can move up,
		//       there is no need to calculate left and upper left cell
		//---------- Calculate scores ---------//
		if(l_score == -1 && thereIsLeftBlock) {
			l_score = P_BlkList[(c - 1) * max_num_blocks + b].Score; // score of block to the left
			for(int i = 0; i < WORD_SIZE - block_pos - 1; i++) {
				if(lP & HIGH_BIT_MASK) 
					l_score--;
				if(lM & HIGH_BIT_MASK) 
					l_score++;
				lP <<= 1;
				lM <<= 1;
			}
		}
		if(ul_score == -1) {
			if(l_score != -1) {
				ul_score = l_score;
				if(lP & HIGH_BIT_MASK) 
					ul_score--;
				if(lM & HIGH_BIT_MASK) 
					ul_score++;
			}
			else if(c > 0 && b-1 >= P_FirstBlocks[c-1] && b-1 <= P_LastBlocks[c-1]) {
				// This is the case when upper left cell is last cell in block,
				// and block to left is not in band so lScore is -1.
				ul_score = P_BlkList[(c - 1) * max_num_blocks + b - 1].Score;
			}
		}
		if(u_score == -1) {
			u_score = curr_score;
			if(currP & HIGH_BIT_MASK) 
				u_score--;
			if(currM & HIGH_BIT_MASK) 
				u_score++;
			currP <<= 1;
			currM <<= 1;
		}
		//-------------------------------------//

		// TODO: should I check if there is upper block?

		//-------------- Move --------------//
		// Move up - insertion to target - deletion from query
		if(u_score != -1 && (u_score + 1) == curr_score) {
			curr_score = u_score;
			l_score = ul_score;
			u_score = ul_score = -1;
			if(block_pos == 0) { // If entering new (upper) block
				if(b == 0) { // If there are no cells above (only boundary cells)
					(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_INSERT; // Move up
					for(int i = 0; i < c + 1; i++) // Move left until end
						(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_DELETE;
					break;
				}
				else {
					block_pos = WORD_SIZE - 1;
					b--;
					currP = P_BlkList[c * max_num_blocks + b].P;
					currM = P_BlkList[c * max_num_blocks + b].M;
					if(c > 0 && b >= P_FirstBlocks[c-1] && b <= P_LastBlocks[c-1]) {
						thereIsLeftBlock = true;
						lP = P_BlkList[(c - 1) * max_num_blocks + b].P; // TODO: improve this, too many operations
						lM = P_BlkList[(c - 1) * max_num_blocks + b].M;
					}
					else {
						thereIsLeftBlock = false;
						// TODO(martin): There may not be left block, but there can be left
						// boundary - do we
						// handle this correctly then? Are l and ul score set correctly? I
						// should check that / refactor this.
					}
				}
			}
			else {
				block_pos--;
				lP <<= 1;
				lM <<= 1;
			}
			// Mark move
			(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_INSERT;
		}
		// Move left - deletion from target - insertion to query
		else if(l_score != -1 && (l_score + 1) == curr_score) {
			curr_score = l_score;
			u_score = ul_score;
			l_score = ul_score = -1;
			c--;
			if(c == -1) { // If there are no cells to the left (only boundary cells)
				(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_DELETE; // Move left
				const int num_up = b * WORD_SIZE + block_pos + 1;
				for(int i = 0; i < num_up; i++) // Move up until end
					(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_INSERT;
				break;
			}
			currP = lP;
			currM = lM;
			if(c > 0 && b >= P_FirstBlocks[c-1] && b <= P_LastBlocks[c-1]) {
				thereIsLeftBlock = true;
				lP = P_BlkList[(c - 1) * max_num_blocks + b].P;
				lM = P_BlkList[(c - 1) * max_num_blocks + b].M;
			}
			else {
				if(c == 0) { // If there are no cells to the left (only boundary cells)
					thereIsLeftBlock = true;
					l_score = b * WORD_SIZE + block_pos + 1;
					ul_score = l_score - 1;
				}
				else {
					thereIsLeftBlock = false;
				}
			}
			// Mark move
			(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_DELETE;
		}
		// Move up left - (mis)match
		else if(ul_score != -1) {
			const uchar move_code = (ul_score == curr_score) ? EDLIB_EDOP_MATCH : EDLIB_EDOP_MISMATCH;
			curr_score = ul_score;
			u_score = l_score = ul_score = -1;
			c--;
			if(c == -1) { // If there are no cells to the left (only boundary cells)
				(*ppAlignment)[(*alignmentLength)++] = move_code; // Move left
				int numUp = b * WORD_SIZE + block_pos;
				for(int i = 0; i < numUp; i++) // Move up until end
					(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_INSERT;
				break;
			}
			if(block_pos == 0) { // If entering upper left block
				if(b == 0) { // If there are no more cells above (only boundary cells)
					(*ppAlignment)[(*alignmentLength)++] = move_code; // Move up left
					for(int i = 0; i < c + 1; i++) // Move left until end
						(*ppAlignment)[(*alignmentLength)++] = EDLIB_EDOP_DELETE;
					break;
				}
				block_pos = WORD_SIZE - 1;
				b--;
				currP = P_BlkList[c * max_num_blocks + b].P;
				currM = P_BlkList[c * max_num_blocks + b].M;
			}
			else { // If entering left block
				block_pos--;
				currP = lP;
				currM = lM;
				currP <<= 1;
				currM <<= 1;
			}
			// Set new left block
			if(c > 0 && b >= P_FirstBlocks[c-1] && b <= P_LastBlocks[c-1]) {
				thereIsLeftBlock = true;
				lP = P_BlkList[(c - 1) * max_num_blocks + b].P;
				lM = P_BlkList[(c - 1) * max_num_blocks + b].M;
			}
			else {
				if(c == 0) { // If there are no cells to the left (only boundary cells)
					thereIsLeftBlock = true;
					l_score = b * WORD_SIZE + block_pos + 1;
					ul_score = l_score - 1;
				}
				else {
					thereIsLeftBlock = false;
				}
			}
			// Mark move
			(*ppAlignment)[(*alignmentLength)++] = move_code;
		}
		else {
			// Reached end - finished!
			break;
		}
	}
	*ppAlignment = static_cast<uchar*>(SAlloc::R(*ppAlignment, (*alignmentLength) * sizeof(uchar)));
	std::reverse(*ppAlignment, *ppAlignment + (*alignmentLength));
	return EDLIB_STATUS_OK;
}
// 
// Finds one possible alignment that gives optimal score (bestScore).
// Uses Hirschberg's algorithm to split problem into two sub-problems, solve them and combine them together.
// @param [in] query
// @param [in] rQuery  Reversed query.
// @param [in] queryLength
// @param [in] target
// @param [in] rTarget  Reversed target.
// @param [in] targetLength
// @param [in] alphabetLength
// @param [in] bestScore  Best(optimal) score.
// @param [out] alignment  Sequence of edit operations that make target equal to query.
// @param [out] alignmentLength  Length of alignment.
// @return Status code.
// 
static int ObtainAlignmentHirschberg(const uchar * pQuery, const uchar * pRevQuery, int queryLength,
    const uchar * pTarget, const uchar * pRevTarget, int targetLength,
    const EqualityDefinition & equalityDefinition, int alphabetLength, int bestScore,
    uchar ** ppAlignment, int * pAlignmentLength) 
{
	const int max_num_blocks = idivroundup(queryLength, WORD_SIZE);
	const int W = max_num_blocks * WORD_SIZE - queryLength;
	uint64 * Peq = equalityDefinition.BuildPeq(alphabetLength, pQuery, queryLength);
	uint64 * rPeq = equalityDefinition.BuildPeq(alphabetLength, pRevQuery, queryLength);
	// Used only to call functions.
	int score_;
	int endLocation_;
	// Divide dynamic matrix into two halfs, left and right.
	const int leftHalfWidth = targetLength / 2;
	const int rightHalfWidth = targetLength - leftHalfWidth;
	// Calculate left half.
	AlignmentData * alignDataLeftHalf = NULL;
	int leftHalfCalcStatus = MyersCalcEditDistanceNW(Peq, W, max_num_blocks, queryLength, pTarget, targetLength, bestScore, &score_, &endLocation_, false, &alignDataLeftHalf, leftHalfWidth-1);
	// Calculate right half.
	AlignmentData * alignDataRightHalf = NULL;
	int rightHalfCalcStatus = MyersCalcEditDistanceNW(rPeq, W, max_num_blocks, queryLength, pRevTarget, targetLength, bestScore, &score_, &endLocation_, false, &alignDataRightHalf, rightHalfWidth-1);
	delete[] Peq;
	delete[] rPeq;
	if(leftHalfCalcStatus == EDLIB_STATUS_ERROR || rightHalfCalcStatus == EDLIB_STATUS_ERROR) {
		delete alignDataLeftHalf;
		delete alignDataRightHalf;
		return EDLIB_STATUS_ERROR;
	}
	// Unwrap the left half.
	int firstBlockIdxLeft = alignDataLeftHalf->P_FirstBlocks[0];
	int lastBlockIdxLeft = alignDataLeftHalf->P_LastBlocks[0];
	// TODO: avoid this allocation by using some shared array?
	// scoresLeft contains scores from left column, starting with scoresLeftStartIdx row (query index)
	// and ending with scoresLeftEndIdx row (0-indexed).
	int scoresLeftLength = (lastBlockIdxLeft - firstBlockIdxLeft + 1) * WORD_SIZE;
	int * scoresLeft = new int[scoresLeftLength];
	for(int blockIdx = firstBlockIdxLeft; blockIdx <= lastBlockIdxLeft; blockIdx++) {
		alignDataLeftHalf->P_BlkList[blockIdx].ReadBlock(scoresLeft + (blockIdx - firstBlockIdxLeft) * WORD_SIZE);
	}
	int scoresLeftStartIdx = firstBlockIdxLeft * WORD_SIZE;
	// If last block contains padding, shorten the length of scores for the length of padding.
	if(lastBlockIdxLeft == (max_num_blocks - 1)) {
		scoresLeftLength -= W;
	}
	// Unwrap the right half (I also reverse it while unwraping).
	int firstBlockIdxRight = alignDataRightHalf->P_FirstBlocks[0];
	int lastBlockIdxRight = alignDataRightHalf->P_LastBlocks[0];
	int scoresRightLength = (lastBlockIdxRight - firstBlockIdxRight + 1) * WORD_SIZE;
	int * p_scores_right = new int[scoresRightLength];
	int * p_scores_right_original_start = p_scores_right;
	for(int blockIdx = firstBlockIdxRight; blockIdx <= lastBlockIdxRight; blockIdx++) {
		alignDataRightHalf->P_BlkList[blockIdx].ReadBlockReverse(p_scores_right + (lastBlockIdxRight - blockIdx) * WORD_SIZE);
	}
	int scoresRightStartIdx = queryLength - (lastBlockIdxRight + 1) * WORD_SIZE;
	// If there is padding at the beginning of scoresRight (that can happen because of reversing that we do),
	// move pointer forward to remove the padding (that is why we remember originalStart).
	if(scoresRightStartIdx < 0) {
		//assert(scoresRightStartIdx == -1 * W);
		p_scores_right += W;
		scoresRightStartIdx += W;
		scoresRightLength -= W;
	}
	delete alignDataLeftHalf;
	delete alignDataRightHalf;
	//
	//       Find the best move
	// Find the query/row index of cell in left column which together with its lower right neighbour
	// from right column gives the best score (when summed). We also have to consider boundary cells
	// (those cells at -1 indexes).
	//  x|
	//  -+-
	//   |x
	//
	int queryIdxLeftStart = smax(scoresLeftStartIdx, scoresRightStartIdx - 1);
	int queryIdxLeftEnd = smin(scoresLeftStartIdx + scoresLeftLength - 1, scoresRightStartIdx + scoresRightLength - 2);
	int leftScore = -1;
	int rightScore = -1;
	int queryIdxLeftAlignment = -1; // Query/row index of cell in left column where alignment is passing through.
	bool queryIdxLeftAlignmentFound = false;
	for(int queryIdx = queryIdxLeftStart; queryIdx <= queryIdxLeftEnd; queryIdx++) {
		leftScore = scoresLeft[queryIdx - scoresLeftStartIdx];
		rightScore = p_scores_right[queryIdx + 1 - scoresRightStartIdx];
		if(leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = queryIdx;
			queryIdxLeftAlignmentFound = true;
			break;
		}
	}
	// Check boundary cells.
	if(!queryIdxLeftAlignmentFound && scoresLeftStartIdx == 0 && scoresRightStartIdx == 0) {
		leftScore = leftHalfWidth;
		rightScore = p_scores_right[0];
		if(leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = -1;
			queryIdxLeftAlignmentFound = true;
		}
	}
	if(!queryIdxLeftAlignmentFound && scoresLeftStartIdx + scoresLeftLength == queryLength && scoresRightStartIdx + scoresRightLength == queryLength) {
		leftScore = scoresLeft[scoresLeftLength - 1];
		rightScore = rightHalfWidth;
		if(leftScore + rightScore == bestScore) {
			queryIdxLeftAlignment = queryLength - 1;
			queryIdxLeftAlignmentFound = true;
		}
	}
	delete[] scoresLeft;
	delete[] p_scores_right_original_start;
	if(queryIdxLeftAlignmentFound == false) {
		// If there was no move that is part of optimal alignment, then there is no such alignment
		// or given bestScore is not correct!
		return EDLIB_STATUS_ERROR;
	}
	//
	// Calculate alignments for upper half of left half (upper left - ul)
	// and lower half of right half (lower right - lr).
	//
	const int ulHeight = queryIdxLeftAlignment + 1;
	const int lrHeight = queryLength - ulHeight;
	const int ulWidth = leftHalfWidth;
	const int lrWidth = rightHalfWidth;
	uchar * ulAlignment = NULL; 
	int ulAlignmentLength;
	int ulStatusCode = equalityDefinition.ObtainAlignment(pQuery, pRevQuery + lrHeight, ulHeight, pTarget, pRevTarget + lrWidth, ulWidth,
		alphabetLength, leftScore, &ulAlignment, &ulAlignmentLength);
	uchar * lrAlignment = NULL; 
	int lrAlignmentLength;
	int lrStatusCode = equalityDefinition.ObtainAlignment(pQuery + ulHeight, pRevQuery, lrHeight, pTarget + ulWidth, pRevTarget, lrWidth,
		alphabetLength, rightScore, &lrAlignment, &lrAlignmentLength);
	if(ulStatusCode == EDLIB_STATUS_ERROR || lrStatusCode == EDLIB_STATUS_ERROR) {
		SAlloc::F(ulAlignment);
		SAlloc::F(lrAlignment);
		return EDLIB_STATUS_ERROR;
	}
	// Build alignment by concatenating upper left alignment with lower right alignment.
	*pAlignmentLength = ulAlignmentLength + lrAlignmentLength;
	*ppAlignment = static_cast<uchar *>(SAlloc::M((*pAlignmentLength) * sizeof(uchar)));
	memcpy(*ppAlignment, ulAlignment, ulAlignmentLength);
	memcpy(*ppAlignment + ulAlignmentLength, lrAlignment, lrAlignmentLength);
	SAlloc::F(ulAlignment);
	SAlloc::F(lrAlignment);
	return EDLIB_STATUS_OK;
}
/**
 * Finds one possible alignment that gives optimal score (bestScore).
 * It will split problem into smaller problems using Hirschberg's algorithm and when they are small enough,
 * it will solve them using traceback algorithm.
 * @param [in] query
 * @param [in] rQuery  Reversed query.
 * @param [in] queryLength
 * @param [in] target
 * @param [in] rTarget  Reversed target.
 * @param [in] targetLength
 * @param [in] equalityDefinition
 * @param [in] alphabetLength
 * @param [in] bestScore  Best(optimal) score.
 * @param [out] alignment  Sequence of edit operations that make target equal to query.
 * @param [out] alignmentLength  Length of alignment.
 * @return Status code.
 */
int EqualityDefinition::ObtainAlignment(const uchar * pQuery, const uchar * pRevQuery, int queryLength,
    const uchar * target, const uchar * rTarget, int targetLength,
    int alphabetLength, int bestScore, uchar ** ppAlignment, int * alignmentLength) const
{
	// Handle special case when one of sequences has length of 0.
	if(queryLength == 0 || targetLength == 0) {
		*alignmentLength = targetLength + queryLength;
		*ppAlignment = static_cast<uchar *>(SAlloc::M((*alignmentLength) * sizeof(uchar)));
		for(int i = 0; i < *alignmentLength; i++) {
			(*ppAlignment)[i] = (queryLength == 0) ? EDLIB_EDOP_DELETE : EDLIB_EDOP_INSERT;
		}
		return EDLIB_STATUS_OK;
	}
	const int max_num_blocks = idivroundup(queryLength, WORD_SIZE);
	const int W = max_num_blocks * WORD_SIZE - queryLength;
	int statusCode;
	// TODO: think about reducing number of memory allocations in alignment functions, probably
	// by sharing some memory that is allocated only once. That refers to: Peq, columns in Hirschberg,
	// and it could also be done for alignments - we could have one big array for alignment that would be
	// sparsely populated by each of steps in recursion, and at the end we would just consolidate those results.

	// If estimated memory consumption for traceback algorithm is smaller than 1MB use it,
	// otherwise use Hirschberg's algorithm. By running few tests I choose boundary of 1MB as optimal.
	long long alignment_data_size = (2ll * sizeof(uint64) + sizeof(int)) * max_num_blocks * targetLength + 2ll * sizeof(int) * targetLength;
	if(alignment_data_size < 1024 * 1024) {
		int score_;
		int endLocation_; // Used only to call function.
		AlignmentData * p_align_data = NULL;
		uint64 * Peq = BuildPeq(alphabetLength, pQuery, queryLength);
		MyersCalcEditDistanceNW(Peq, W, max_num_blocks, queryLength, target, targetLength, bestScore, &score_, &endLocation_, true, &p_align_data, -1);
		//assert(score_ == bestScore);
		//assert(endLocation_ == targetLength - 1);
		statusCode = p_align_data->ObtainAlignmentTraceback(queryLength, targetLength, bestScore, ppAlignment, alignmentLength);
		delete p_align_data;
		delete [] Peq;
	}
	else {
		statusCode = ObtainAlignmentHirschberg(pQuery, pRevQuery, queryLength, target, rTarget, targetLength,
			*this, alphabetLength, bestScore, ppAlignment, alignmentLength);
	}
	return statusCode;
}

EdlibAlignConfig edlibNewAlignConfig(int k, EdlibAlignMode mode, EdlibAlignTask task, const EdlibEqualityPair * additionalEqualities, int additionalEqualitiesLength) 
{
	EdlibAlignConfig config;
	config.K = k;
	config.Mode = mode;
	config.Task = task;
	config.additionalEqualities = additionalEqualities;
	config.additionalEqualitiesLength = additionalEqualitiesLength;
	return config;
}

EdlibAlignConfig edlibDefaultAlignConfig() 
{
	return edlibNewAlignConfig(-1, EDLIB_MODE_NW, EDLIB_TASK_DISTANCE, NULL, 0);
}

void EdlibFreeAlignResult(EdlibAlignResult * pResult)
{
	if(pResult) {
		ZFREE(pResult->P_EndLocations);
		ZFREE(pResult->P_StartLocations);
		ZFREE(pResult->P_Alignment);
	}
}
